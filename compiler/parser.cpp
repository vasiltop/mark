#include "parser.hpp"
#include <cstring>

namespace parser {

internal auto token_str(const lexer::Token &tok) -> const char * {
  return tok.start;
}

internal auto token_len(const lexer::Token &tok) -> s32 {
  return tok.len;
}

internal auto set_error(Parser *p, const char *msg) -> void {
  if (p->has_error) return;
  p->has_error = true;
  p->error = ParseError { .line = p->cur.line, .col = p->cur.col, .msg = msg };
}

internal auto advance(Parser *p) -> void {
  p->cur = p->peek_tok;
  p->peek_tok = lexer::lex_next(p->lex);
}

internal auto expect(Parser *p, lexer::TokenKind kind) -> bool {
  if (p->cur.kind != kind) {
    set_error(p, "unexpected token");
    return false;
  }
  advance(p);
  return true;
}

internal auto skip_newlines(Parser *p) -> void {
  while (p->cur.kind == lexer::TokenKind::Newline) advance(p);
}

internal auto unquote_string(arena::Arena *arena, const lexer::Token &tok) -> const char * {
  if (tok.len < 2) return arena::push_str(arena, "", 0);
  return arena::push_str(arena, tok.start + 1, tok.len - 2);
}

internal auto push_inline(Parser *p, ir::InlineNode **head, ir::InlineNode **tail, ir::InlineItem item) -> void {
  auto *node = arena::push<ir::InlineNode>(p->arena);
  node->item = item;
  node->next = nullptr;
  if (*tail) (*tail)->next = node;
  else *head = node;
  *tail = node;
}

internal auto parse_inline(Parser *p, ir::InlineNode **head, ir::InlineNode **tail, lexer::TokenKind end1, lexer::TokenKind end2) -> bool;
internal auto parse_content_block(Parser *p, ir::InlineNode **head, ir::InlineNode **tail) -> bool;
internal auto parse_code_expr(Parser *p, ir::InlineNode **head, ir::InlineNode **tail) -> bool;
internal auto parse_func_args(Parser *p, const char **url_out, const char **alt_out, const char **label_out) -> bool;

internal auto parse_inline(Parser *p, ir::InlineNode **head, ir::InlineNode **tail, lexer::TokenKind end1, lexer::TokenKind end2) -> bool {
  while (p->cur.kind != lexer::TokenKind::Eof &&
         p->cur.kind != end1 &&
         p->cur.kind != end2) {

    if (p->cur.kind == lexer::TokenKind::Newline) {
      if (end1 == lexer::TokenKind::Newline || end2 == lexer::TokenKind::Newline) break;
      advance(p);
      continue;
    }

    if (p->cur.kind == lexer::TokenKind::Math && p->cur.len > 0) {
      auto tok = p->cur;
      advance(p);
      push_inline(p, head, tail, ir::Math { .start = tok.start, .len = tok.len });
      continue;
    }

    if (p->cur.kind == lexer::TokenKind::Text && p->cur.len > 0) {
      auto tok = p->cur;
      advance(p);
      push_inline(p, head, tail, ir::Text { .start = tok.start, .len = tok.len });
      continue;
    }

    if (p->cur.kind == lexer::TokenKind::Dot) {
      auto tok = p->cur;
      advance(p);
      push_inline(p, head, tail, ir::Text { .start = tok.start, .len = tok.len });
      continue;
    }

    if (p->cur.kind == lexer::TokenKind::Star) {
      advance(p);
      ir::InlineNode *inner_head = nullptr;
      ir::InlineNode *inner_tail = nullptr;
      if (!parse_inline(p, &inner_head, &inner_tail, lexer::TokenKind::Star, lexer::TokenKind::Eof)) return false;
      if (!expect(p, lexer::TokenKind::Star)) return false;
      push_inline(p, head, tail, ir::Strong { .content = inner_head });
      continue;
    }

    if (p->cur.kind == lexer::TokenKind::Underscore) {
      advance(p);
      ir::InlineNode *inner_head = nullptr;
      ir::InlineNode *inner_tail = nullptr;
      if (!parse_inline(p, &inner_head, &inner_tail, lexer::TokenKind::Underscore, lexer::TokenKind::Eof)) return false;
      if (!expect(p, lexer::TokenKind::Underscore)) return false;
      push_inline(p, head, tail, ir::Emph { .content = inner_head });
      continue;
    }

    if (p->cur.kind == lexer::TokenKind::Hash) {
      advance(p);
      if (!parse_code_expr(p, head, tail)) return false;
      continue;
    }

    if (p->cur.kind == lexer::TokenKind::Backslash) {
      advance(p);
      push_inline(p, head, tail, ir::Text { .start = "<br/>", .len = 5 });
      continue;
    }

    if (p->cur.kind == lexer::TokenKind::At) {
      advance(p);
      if (p->cur.kind != lexer::TokenKind::Ident) {
        set_error(p, "expected label after @");
        return false;
      }
      auto label = p->cur;
      advance(p);
      push_inline(p, head, tail, ir::Ref {
        .label = arena::push_str(p->arena, label.start, label.len),
      });
      continue;
    }

    set_error(p, "unexpected token in inline content");
    return false;
  }
  return true;
}

internal auto parse_content_block(Parser *p, ir::InlineNode **head, ir::InlineNode **tail) -> bool {
  if (p->cur.kind != lexer::TokenKind::LBracket) {
    set_error(p, "expected [");
    return false;
  }
  advance(p);
  p->lex->mode = lexer::Mode::Markup;
  if (!parse_inline(p, head, tail, lexer::TokenKind::RBracket, lexer::TokenKind::Eof)) return false;
  if (!expect(p, lexer::TokenKind::RBracket)) return false;
  return true;
}

internal auto parse_func_args(Parser *p, const char **url_out, const char **alt_out, const char **label_out) -> bool {
  *url_out = nullptr;
  *alt_out = nullptr;
  *label_out = nullptr;
  if (p->cur.kind != lexer::TokenKind::LParen) return true;
  advance(p);
  while (p->cur.kind != lexer::TokenKind::RParen && p->cur.kind != lexer::TokenKind::Eof) {
    if (p->cur.kind == lexer::TokenKind::String) {
      if (!*url_out) *url_out = unquote_string(p->arena, p->cur);
      else *label_out = unquote_string(p->arena, p->cur);
      advance(p);
    } else if (p->cur.kind == lexer::TokenKind::Ident && p->peek_tok.kind == lexer::TokenKind::Colon) {
      auto key = p->cur;
      advance(p);
      advance(p);
      if (p->cur.kind == lexer::TokenKind::String) {
        if (key.len == 3 && std::strncmp(key.start, "alt", 3) == 0)
          *alt_out = unquote_string(p->arena, p->cur);
        advance(p);
      }
    } else if (p->cur.kind == lexer::TokenKind::LBracket) {
      break;
    } else {
      advance(p);
    }
    if (p->cur.kind == lexer::TokenKind::Comma) advance(p);
  }
  if (p->cur.kind == lexer::TokenKind::RParen) advance(p);
  return true;
}

internal auto make_text_node(Parser *p, const char *s) -> ir::InlineNode * {
  if (!s) return nullptr;
  auto len = (s32)std::strlen(s);
  auto *node = arena::push<ir::InlineNode>(p->arena);
  node->item = ir::Text { .start = s, .len = len };
  node->next = nullptr;
  return node;
}

internal auto parse_code_expr(Parser *p, ir::InlineNode **head, ir::InlineNode **tail) -> bool {
  if (p->cur.kind == lexer::TokenKind::Ident && p->cur.len == 3 && std::strncmp(p->cur.start, "set", 3) == 0) {
    advance(p);
    if (p->cur.kind != lexer::TokenKind::Ident) { set_error(p, "expected target after set"); return false; }
    auto target = p->cur;
    advance(p);
    while (p->cur.kind != lexer::TokenKind::Newline && p->cur.kind != lexer::TokenKind::Eof) advance(p);
    // set rules handled at block level — skip inline
    (void)target;
    return true;
  }

  if (p->cur.kind != lexer::TokenKind::Ident) {
    set_error(p, "expected identifier after #");
    return false;
  }

  auto name = p->cur;
  advance(p);

  const char *url = nullptr;
  const char *alt = nullptr;
  const char *label_str = nullptr;
  if (!parse_func_args(p, &url, &alt, &label_str)) return false;

  ir::InlineNode *content_head = nullptr;
  ir::InlineNode *content_tail = nullptr;
  if (p->cur.kind == lexer::TokenKind::LBracket) {
    if (!parse_content_block(p, &content_head, &content_tail)) return false;
  } else if (label_str) {
    content_head = make_text_node(p, label_str);
  }

  if (name.len == 4 && std::strncmp(name.start, "link", 4) == 0) {
    push_inline(p, head, tail, ir::Link { .url = url, .label = content_head });
    p->lex->mode = lexer::Mode::Markup;
    return true;
  }

  if (name.len == 5 && std::strncmp(name.start, "image", 5) == 0) {
    push_inline(p, head, tail, ir::Image { .src = url, .alt = alt ? alt : "" });
    p->lex->mode = lexer::Mode::Markup;
    return true;
  }

  push_inline(p, head, tail, ir::FuncCall {
    .name = arena::push_str(p->arena, name.start, name.len),
    .content = content_head,
  });
  p->lex->mode = lexer::Mode::Markup;
  return true;
}

internal auto parse_label(Parser *p) -> const char * {
  if (p->cur.kind != lexer::TokenKind::Lt) return nullptr;
  advance(p);
  if (p->cur.kind != lexer::TokenKind::Ident) return nullptr;
  auto label = arena::push_str(p->arena, p->cur.start, p->cur.len);
  advance(p);
  if (p->cur.kind == lexer::TokenKind::Gt) advance(p);
  return label;
}

internal auto push_style_prop(Parser *p, ir::StyleProp **head, ir::StyleProp **tail, const char *key, const char *value) -> void {
  auto *prop = arena::push<ir::StyleProp>(p->arena);
  prop->key = key;
  prop->value = value;
  prop->next = nullptr;
  if (*tail) (*tail)->next = prop;
  else *head = prop;
  *tail = prop;
}

internal auto parse_style_props(Parser *p) -> ir::StyleProp * {
  ir::StyleProp *head = nullptr;
  ir::StyleProp *tail = nullptr;
  if (p->cur.kind != lexer::TokenKind::LParen) return nullptr;
  advance(p);
  while (p->cur.kind != lexer::TokenKind::RParen && p->cur.kind != lexer::TokenKind::Eof) {
    if (p->cur.kind == lexer::TokenKind::Newline) {
      advance(p);
      continue;
    }
    if (p->cur.kind == lexer::TokenKind::Ident && p->peek_tok.kind == lexer::TokenKind::Colon) {
      auto key = arena::push_str(p->arena, p->cur.start, p->cur.len);
      advance(p);
      advance(p);
      if (p->cur.kind == lexer::TokenKind::String) {
        auto value = unquote_string(p->arena, p->cur);
        push_style_prop(p, &head, &tail, key, value);
        advance(p);
      }
    } else {
      advance(p);
    }
    if (p->cur.kind == lexer::TokenKind::Comma) advance(p);
  }
  if (p->cur.kind == lexer::TokenKind::RParen) advance(p);
  return head;
}

internal auto push_block(Parser *p, ir::BlockNode **head, ir::BlockNode **tail, ir::BlockItem item) -> void;

internal auto parse_show_rule(Parser *p, ir::BlockNode **head, ir::BlockNode **tail) -> bool {
  advance(p);
  if (p->cur.kind != lexer::TokenKind::Ident) {
    set_error(p, "expected show target");
    return false;
  }
  auto target = arena::push_str(p->arena, p->cur.start, p->cur.len);
  advance(p);
  const char *class_name = nullptr;
  while (p->cur.kind != lexer::TokenKind::Newline && p->cur.kind != lexer::TokenKind::Eof) {
    if (p->cur.kind == lexer::TokenKind::Ident && p->cur.len == 5 && std::strncmp(p->cur.start, "class", 5) == 0) {
      advance(p);
      if (p->cur.kind == lexer::TokenKind::Colon) advance(p);
      if (p->cur.kind == lexer::TokenKind::String) {
        class_name = unquote_string(p->arena, p->cur);
        advance(p);
      }
    } else {
      advance(p);
    }
  }
  push_block(p, head, tail, ir::ShowRule { .target = target, .class_name = class_name });
  return true;
}

internal auto parse_layout_block(Parser *p, const char *kind, ir::BlockNode **head, ir::BlockNode **tail) -> bool {
  advance(p);
  s32 columns = 1;
  if (p->cur.kind == lexer::TokenKind::LParen) {
    advance(p);
    while (p->cur.kind != lexer::TokenKind::RParen && p->cur.kind != lexer::TokenKind::Eof) {
      if (p->cur.kind == lexer::TokenKind::Newline) { advance(p); continue; }
      if (p->cur.kind == lexer::TokenKind::Ident && p->cur.len == 7 && std::strncmp(p->cur.start, "columns", 7) == 0) {
        advance(p);
        if (p->cur.kind == lexer::TokenKind::Colon) advance(p);
        if (p->cur.kind == lexer::TokenKind::Int) {
          columns = 0;
          for (s32 i = 0; i < p->cur.len; i++) columns = columns * 10 + (p->cur.start[i] - '0');
          advance(p);
        }
      } else if (p->cur.kind == lexer::TokenKind::LBracket) {
        break;
      } else {
        advance(p);
      }
      if (p->cur.kind == lexer::TokenKind::Comma) advance(p);
    }
    if (p->cur.kind == lexer::TokenKind::RParen) advance(p);
  }

  ir::InlineNode *content_head = nullptr;
  ir::InlineNode *content_tail = nullptr;
  if (p->cur.kind == lexer::TokenKind::LBracket) {
    if (!parse_content_block(p, &content_head, &content_tail)) return false;
  }

  push_block(p, head, tail, ir::LayoutBlock {
    .kind = arena::push_str(p->arena, kind, (s32)std::strlen(kind)),
    .columns = columns,
    .content = content_head,
  });
  return true;
}

internal auto parse_block(Parser *p, ir::BlockNode **head, ir::BlockNode **tail) -> bool;

internal auto push_block(Parser *p, ir::BlockNode **head, ir::BlockNode **tail, ir::BlockItem item) -> void {
  auto *node = arena::push<ir::BlockNode>(p->arena);
  node->item = item;
  node->next = nullptr;
  if (*tail) (*tail)->next = node;
  else *head = node;
  *tail = node;
}

internal auto parse_list(Parser *p, ir::BlockNode **head, ir::BlockNode **tail) -> bool {
  ir::ListItem *first = nullptr;
  ir::ListItem *last = nullptr;

  while (p->cur.kind == lexer::TokenKind::Minus || p->cur.kind == lexer::TokenKind::Plus) {
    advance(p);
    skip_newlines(p);
    ir::InlineNode *content_head = nullptr;
    ir::InlineNode *content_tail = nullptr;
    if (!parse_inline(p, &content_head, &content_tail, lexer::TokenKind::Newline, lexer::TokenKind::Minus)) return false;

    auto *item = arena::push<ir::ListItem>(p->arena);
    item->content = content_head;
    item->next = nullptr;
    if (last) last->next = item;
    else first = item;
    last = item;

    skip_newlines(p);
  }

  push_block(p, head, tail, ir::List { .first = first });
  return true;
}

internal auto parse_table(Parser *p, ir::BlockNode **head, ir::BlockNode **tail) -> bool {
  advance(p);
  s32 columns = 1;
  if (p->cur.kind != lexer::TokenKind::LParen) {
    set_error(p, "expected ( after table");
    return false;
  }
  advance(p);

  ir::TableRow *row_first = nullptr;
  ir::TableRow *row_last = nullptr;
  ir::TableRow *cur_row = nullptr;
  s32 cell_in_row = 0;

  while (p->cur.kind != lexer::TokenKind::RParen && p->cur.kind != lexer::TokenKind::Eof) {
    if (p->cur.kind == lexer::TokenKind::Newline) {
      advance(p);
      continue;
    }

    if (p->cur.kind == lexer::TokenKind::Text && p->cur.len > 0) {
      auto only_ws = true;
      for (s32 i = 0; i < p->cur.len; i++) {
        char c = p->cur.start[i];
        if (c != ' ' && c != '\t') { only_ws = false; break; }
      }
      if (only_ws) { advance(p); continue; }
    }

    if (p->cur.kind == lexer::TokenKind::Ident && p->cur.len == 7 && std::strncmp(p->cur.start, "columns", 7) == 0) {
      advance(p);
      if (p->cur.kind == lexer::TokenKind::Colon) advance(p);
      if (p->cur.kind == lexer::TokenKind::Int) {
        columns = 0;
        for (s32 i = 0; i < p->cur.len; i++)
          columns = columns * 10 + (p->cur.start[i] - '0');
        advance(p);
      }
      if (p->cur.kind == lexer::TokenKind::Comma) advance(p);
      continue;
    }

    if (p->cur.kind == lexer::TokenKind::LBracket) {
      ir::InlineNode *cell_head = nullptr;
      ir::InlineNode *cell_tail = nullptr;
      if (!parse_content_block(p, &cell_head, &cell_tail)) return false;
      p->lex->mode = lexer::Mode::Code;

      if (!cur_row || cell_in_row >= columns) {
        auto *row = arena::push<ir::TableRow>(p->arena);
        row->first = nullptr;
        row->next = nullptr;
        if (row_last) row_last->next = row;
        else row_first = row;
        row_last = row;
        cur_row = row;
        cell_in_row = 0;
      }

      auto *cell = arena::push<ir::TableCell>(p->arena);
      cell->content = cell_head;
      cell->next = nullptr;
      if (!cur_row->first) {
        cur_row->first = cell;
      } else {
        auto *c = cur_row->first;
        while (c->next) c = c->next;
        c->next = cell;
      }
      cell_in_row++;

      if (p->cur.kind == lexer::TokenKind::Comma) advance(p);
      continue;
    }

    advance(p);
  }

  if (p->cur.kind == lexer::TokenKind::RParen) advance(p);

  push_block(p, head, tail, ir::Table { .columns = columns, .first = row_first });
  return true;
}

internal auto parse_block(Parser *p, ir::BlockNode **head, ir::BlockNode **tail) -> bool {
  skip_newlines(p);

  if (p->cur.kind == lexer::TokenKind::Eof) return true;

  if (p->cur.kind == lexer::TokenKind::Hash) {
    advance(p);
    if (p->cur.kind == lexer::TokenKind::Ident && p->cur.len == 3 && std::strncmp(p->cur.start, "set", 3) == 0) {
      advance(p);
      if (p->cur.kind != lexer::TokenKind::Ident) { set_error(p, "expected target after set"); return false; }
      auto target = arena::push_str(p->arena, p->cur.start, p->cur.len);
      advance(p);
      auto props = parse_style_props(p);
      push_block(p, head, tail, ir::SetRule { .target = target, .props = props });
      skip_newlines(p);
      return parse_block(p, head, tail);
    }

    if (p->cur.kind == lexer::TokenKind::Ident && p->cur.len == 4 && std::strncmp(p->cur.start, "show", 4) == 0) {
      if (!parse_show_rule(p, head, tail)) return false;
      skip_newlines(p);
      return parse_block(p, head, tail);
    }

    if (p->cur.kind == lexer::TokenKind::Ident && p->cur.len == 4 && std::strncmp(p->cur.start, "grid", 4) == 0) {
      if (!parse_layout_block(p, "grid", head, tail)) return false;
      skip_newlines(p);
      return parse_block(p, head, tail);
    }

    if (p->cur.kind == lexer::TokenKind::Ident && p->cur.len == 3 && std::strncmp(p->cur.start, "box", 3) == 0) {
      if (!parse_layout_block(p, "box", head, tail)) return false;
      skip_newlines(p);
      return parse_block(p, head, tail);
    }

    if (p->cur.kind == lexer::TokenKind::Ident && p->cur.len == 5 && std::strncmp(p->cur.start, "align", 5) == 0) {
      if (!parse_layout_block(p, "align", head, tail)) return false;
      skip_newlines(p);
      return parse_block(p, head, tail);
    }

    if (p->cur.kind == lexer::TokenKind::Ident && p->cur.len == 5 && std::strncmp(p->cur.start, "table", 5) == 0) {
      if (!parse_table(p, head, tail)) return false;
      skip_newlines(p);
      return parse_block(p, head, tail);
    }

    ir::InlineNode *inline_head = nullptr;
    ir::InlineNode *inline_tail = nullptr;
    if (!parse_code_expr(p, &inline_head, &inline_tail)) return false;
    push_block(p, head, tail, ir::Paragraph { .content = inline_head });
    skip_newlines(p);
    return parse_block(p, head, tail);
  }

  if (p->cur.kind == lexer::TokenKind::Equals) {
    s32 level = p->cur.len;
    advance(p);
    ir::InlineNode *content_head = nullptr;
    ir::InlineNode *content_tail = nullptr;
    if (!parse_inline(p, &content_head, &content_tail, lexer::TokenKind::Newline, lexer::TokenKind::Lt)) return false;
    const char *label = parse_label(p);
    skip_newlines(p);
    push_block(p, head, tail, ir::Heading { .level = level, .content = content_head, .label = label });
    return parse_block(p, head, tail);
  }

  if (p->cur.kind == lexer::TokenKind::Minus || p->cur.kind == lexer::TokenKind::Plus) {
    if (!parse_list(p, head, tail)) return false;
    return parse_block(p, head, tail);
  }

  ir::InlineNode *content_head = nullptr;
  ir::InlineNode *content_tail = nullptr;
  if (!parse_inline(p, &content_head, &content_tail, lexer::TokenKind::Newline, lexer::TokenKind::Eof)) return false;
  push_block(p, head, tail, ir::Paragraph { .content = content_head });
  skip_newlines(p);
  return parse_block(p, head, tail);
}

auto parse_document(Parser *p) -> ParseResult {
  ir::BlockNode *head = nullptr;
  ir::BlockNode *tail = nullptr;

  p->has_error = false;
  p->peek_tok = lexer::lex_next(p->lex);
  advance(p);

  if (!parse_block(p, &head, &tail)) {
    return ParseResult { .doc = {}, .error = p->error, .ok = false };
  }

  return ParseResult {
    .doc = ir::Document { .first = head },
    .error = {},
    .ok = !p->has_error,
  };
}

} // namespace parser
