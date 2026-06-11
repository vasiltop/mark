#include "lexer.hpp"
#include <cctype>

namespace lexer {

internal auto is_ident_start(char c) -> bool {
  return std::isalpha(static_cast<unsigned char>(c)) || c == '_';
}

internal auto is_ident_char(char c) -> bool {
  return is_ident_start(c) || std::isdigit(static_cast<unsigned char>(c));
}

internal auto make_token(Lexer *lex, TokenKind kind, const char *start, s32 len) -> Token {
  return Token {
    .kind = kind,
    .line = lex->line,
    .col = lex->col,
    .start = start,
    .len = len,
  };
}

internal auto advance(Lexer *lex) -> char {
  if (lex->cur >= lex->end) return '\0';
  char c = *lex->cur++;
  if (c == '\n') {
    lex->line++;
    lex->col = 1;
    lex->at_line_start = true;
  } else {
    lex->col++;
    if (c != ' ' && c != '\t') lex->at_line_start = false;
  }
  return c;
}

internal auto peek(Lexer *lex) -> char {
  if (lex->cur >= lex->end) return '\0';
  return *lex->cur;
}

internal auto skip_ws(Lexer *lex) -> void {
  while (true) {
    char c = peek(lex);
    if (c == ' ' || c == '\t' || c == '\r') {
      advance(lex);
      continue;
    }
    if (c == '/' && lex->cur + 1 < lex->end && lex->cur[1] == '/') {
      while (peek(lex) != '\0' && peek(lex) != '\n') advance(lex);
      continue;
    }
    break;
  }
}

internal auto lex_string(Lexer *lex) -> Token {
  auto start = lex->cur;
  advance(lex);
  while (peek(lex) != '\0' && peek(lex) != '"') {
    if (peek(lex) == '\\' && lex->cur + 1 < lex->end) advance(lex);
    advance(lex);
  }
  if (peek(lex) == '"') advance(lex);
  return make_token(lex, TokenKind::String, start, (s32)(lex->cur - start));
}

internal auto lex_ident(Lexer *lex) -> Token {
  auto start = lex->cur;
  advance(lex);
  while (is_ident_char(peek(lex))) advance(lex);
  return make_token(lex, TokenKind::Ident, start, (s32)(lex->cur - start));
}

internal auto lex_number(Lexer *lex) -> Token {
  auto start = lex->cur;
  while (std::isdigit(static_cast<unsigned char>(peek(lex)))) advance(lex);
  return make_token(lex, TokenKind::Int, start, (s32)(lex->cur - start));
}

internal auto lex_text_run(Lexer *lex) -> Token {
  auto start = lex->cur;
  while (true) {
    char c = peek(lex);
    if (c == '\0' || c == '\n') break;
    if (c == '#' || c == '*' || c == '_' || c == '\\' || c == '[' || c == ']') break;
    if (c == '<' && lex->mode == Mode::Markup) break;
    advance(lex);
  }
  return make_token(lex, TokenKind::Text, start, (s32)(lex->cur - start));
}

internal auto lex_code(Lexer *lex) -> Token {
  skip_ws(lex);
  char c = peek(lex);
  if (c == '\0') return make_token(lex, TokenKind::Eof, lex->cur, 0);
  if (c == '\n') { advance(lex); return make_token(lex, TokenKind::Newline, lex->cur - 1, 1); }
  if (c == '"') return lex_string(lex);
  if (is_ident_start(c)) return lex_ident(lex);
  if (std::isdigit(static_cast<unsigned char>(c))) return lex_number(lex);

  advance(lex);
  switch (c) {
    case '(': return make_token(lex, TokenKind::LParen, lex->cur - 1, 1);
    case ')': return make_token(lex, TokenKind::RParen, lex->cur - 1, 1);
    case '[':
      lex->mode = Mode::Markup;
      return make_token(lex, TokenKind::LBracket, lex->cur - 1, 1);
    case ']': return make_token(lex, TokenKind::RBracket, lex->cur - 1, 1);
    case '{': return make_token(lex, TokenKind::LBrace, lex->cur - 1, 1);
    case '}': return make_token(lex, TokenKind::RBrace, lex->cur - 1, 1);
    case ',': return make_token(lex, TokenKind::Comma, lex->cur - 1, 1);
    case ':': return make_token(lex, TokenKind::Colon, lex->cur - 1, 1);
    case '.': return make_token(lex, TokenKind::Dot, lex->cur - 1, 1);
    case ';': return make_token(lex, TokenKind::Semicolon, lex->cur - 1, 1);
    case '@': return make_token(lex, TokenKind::At, lex->cur - 1, 1);
    case '<': return make_token(lex, TokenKind::Lt, lex->cur - 1, 1);
    case '>': return make_token(lex, TokenKind::Gt, lex->cur - 1, 1);
    default: return make_token(lex, TokenKind::Text, lex->cur - 1, 1);
  }
}

internal auto lex_markup(Lexer *lex) -> Token {
  skip_ws(lex);
  char c = peek(lex);
  if (c == '\0') return make_token(lex, TokenKind::Eof, lex->cur, 0);
  if (c == '\n') { advance(lex); return make_token(lex, TokenKind::Newline, lex->cur - 1, 1); }

  if (lex->at_line_start && c == '=') {
    auto start = lex->cur;
    s32 count = 0;
    while (peek(lex) == '=') { advance(lex); count++; }
    return make_token(lex, TokenKind::Equals, start, count);
  }

  if (lex->at_line_start && c == '-') {
    advance(lex);
    return make_token(lex, TokenKind::Minus, lex->cur - 1, 1);
  }

  if (lex->at_line_start && c == '+') {
    advance(lex);
    return make_token(lex, TokenKind::Plus, lex->cur - 1, 1);
  }

  if (c == '#') {
    advance(lex);
    lex->mode = Mode::Code;
    return make_token(lex, TokenKind::Hash, lex->cur - 1, 1);
  }

  if (c == '*') { advance(lex); return make_token(lex, TokenKind::Star, lex->cur - 1, 1); }
  if (c == '_') { advance(lex); return make_token(lex, TokenKind::Underscore, lex->cur - 1, 1); }
  if (c == '\\') { advance(lex); return make_token(lex, TokenKind::Backslash, lex->cur - 1, 1); }
  if (c == '[') { advance(lex); return make_token(lex, TokenKind::LBracket, lex->cur - 1, 1); }
  if (c == ']') { advance(lex); return make_token(lex, TokenKind::RBracket, lex->cur - 1, 1); }
  if (c == '@') { advance(lex); return make_token(lex, TokenKind::At, lex->cur - 1, 1); }
  if (c == '<') { advance(lex); return make_token(lex, TokenKind::Lt, lex->cur - 1, 1); }

  return lex_text_run(lex);
}

auto lex_init(Lexer *lex, const char *src, s32 len) -> void {
  lex->src = src;
  lex->end = src + len;
  lex->cur = src;
  lex->line = 1;
  lex->col = 1;
  lex->mode = Mode::Markup;
  lex->at_line_start = true;
}

auto lex_next(Lexer *lex) -> Token {
  if (lex->mode == Mode::Code) {
    auto tok = lex_code(lex);
    if (tok.kind == TokenKind::RBracket || tok.kind == TokenKind::Newline) {
      lex->mode = Mode::Markup;
    }
    return tok;
  }
  return lex_markup(lex);
}

auto lex_peek(Lexer *lex) -> Token {
  auto saved_cur = lex->cur;
  auto saved_line = lex->line;
  auto saved_col = lex->col;
  auto saved_mode = lex->mode;
  auto saved_at_line_start = lex->at_line_start;
  auto tok = lex_next(lex);
  lex->cur = saved_cur;
  lex->line = saved_line;
  lex->col = saved_col;
  lex->mode = saved_mode;
  lex->at_line_start = saved_at_line_start;
  return tok;
}

} // namespace lexer
