#include "emit.hpp"
#include <cstdio>

namespace emit {

internal auto append_escaped(std::string *out, const char *start, s32 len) -> void {
  for (s32 i = 0; i < len; i++) {
    char c = start[i];
    switch (c) {
      case '&': out->append("&amp;"); break;
      case '<': out->append("&lt;"); break;
      case '>': out->append("&gt;"); break;
      case '"': out->append("&quot;"); break;
      default: out->push_back(c); break;
    }
  }
}

internal auto append_escaped(std::string *out, const char *s) -> void {
  append_escaped(out, s, (s32)std::strlen(s));
}

internal auto emit_inline(std::string *out, ir::InlineNode *node, const eval::StyleCtx &style) -> void {
  for (; node; node = node->next) {
    std::visit(overloaded {
      [&](const ir::Text &t) {
        append_escaped(out, t.start, t.len);
      },
      [&](const ir::Strong &s) {
        out->append("<strong>");
        emit_inline(out, s.content, style);
        out->append("</strong>");
      },
      [&](const ir::Emph &e) {
        out->append("<em>");
        emit_inline(out, e.content, style);
        out->append("</em>");
      },
      [&](const ir::Link &l) {
        out->append("<a class=\"mark-link\" href=\"");
        append_escaped(out, l.url);
        out->append("\">");
        emit_inline(out, l.label, style);
        out->append("</a>");
      },
      [&](const ir::Image &img) {
        out->append("<img class=\"mark-image\" src=\"");
        append_escaped(out, img.src);
        out->append("\" alt=\"");
        append_escaped(out, img.alt);
        out->append("\" />");
      },
      [&](const ir::Ref &r) {
        out->append("<a class=\"mark-ref\" href=\"#");
        append_escaped(out, r.label);
        out->append("\">");
        append_escaped(out, r.label);
        out->append("</a>");
      },
      [&](const ir::FuncCall &f) {
        emit_inline(out, f.content, style);
      },
    }, node->item);
  }
}

internal auto emit_css(std::string *css, const eval::StyleCtx &style) -> void {
  css->append("body { font-family: ");
  css->append(style.font_family);
  css->append("; font-size: ");
  css->append(style.font_size);
  css->append("; color: ");
  css->append(style.text_color);
  css->append("; line-height: 1.5; max-width: 720px; margin: 2em auto; padding: 0 1em; }\n");
  css->append(".mark-link, .mark-ref { color: ");
  css->append(style.link_color);
  css->append("; text-decoration: none; }\n");
  css->append(".mark-link:hover, .mark-ref:hover { text-decoration: underline; }\n");
  css->append(".mark-heading { margin-top: 1.5em; margin-bottom: 0.5em; }\n");
  css->append(".mark-para { margin: 0.75em 0; }\n");
  css->append(".mark-list { margin: 0.75em 0; padding-left: 1.5em; }\n");
  css->append(".mark-table { border-collapse: collapse; width: 100%; margin: 1em 0; }\n");
  css->append(".mark-table td, .mark-table th { border: 1px solid #ccc; padding: 0.5em 0.75em; }\n");
  css->append(".mark-image { max-width: 100%; height: auto; }\n");
}

internal auto emit_block(std::string *out, ir::BlockNode *node, const eval::StyleCtx &style) -> void {
  for (; node; node = node->next) {
    std::visit(overloaded {
      [&](const ir::Heading &h) {
        char tag[4] = "h1";
        s32 lvl = h.level < 1 ? 1 : (h.level > 6 ? 6 : h.level);
        tag[1] = (char)('0' + lvl);
        out->append("<");
        out->append(tag);
        out->append(" class=\"mark-heading\"");
        if (h.label) {
          out->append(" id=\"");
          append_escaped(out, h.label);
          out->append("\"");
        }
        out->append(">");
        emit_inline(out, h.content, style);
        out->append("</");
        out->append(tag);
        out->append(">\n");
      },
      [&](const ir::Paragraph &p) {
        out->append("<p class=\"mark-para\">");
        emit_inline(out, p.content, style);
        out->append("</p>\n");
      },
      [&](const ir::List &l) {
        out->append("<ul class=\"mark-list\">\n");
        for (auto *item = l.first; item; item = item->next) {
          out->append("<li>");
          emit_inline(out, item->content, style);
          out->append("</li>\n");
        }
        out->append("</ul>\n");
      },
      [&](const ir::Table &t) {
        out->append("<table class=\"mark-table\">\n");
        for (auto *row = t.first; row; row = row->next) {
          out->append("<tr>");
          for (auto *cell = row->first; cell; cell = cell->next) {
            out->append("<td>");
            emit_inline(out, cell->content, style);
            out->append("</td>");
          }
          out->append("</tr>\n");
        }
        out->append("</table>\n");
      },
      [&](const ir::SetRule &) {},
      [&](const ir::FuncCall &f) {
        out->append("<div class=\"mark-func\">");
        emit_inline(out, f.content, style);
        out->append("</div>\n");
      },
    }, node->item);
  }
}

auto emit_document(const ir::Document &doc, EmitCtx *ctx) -> void {
  emit_css(&ctx->css, ctx->style);
  ctx->html.append("<!DOCTYPE html>\n<html lang=\"en\">\n<head>\n<meta charset=\"utf-8\" />\n");
  ctx->html.append("<style>\n");
  ctx->html.append(ctx->css);
  ctx->html.append("</style>\n<title>Mark</title>\n</head>\n<body>\n<article class=\"mark-doc\">\n");
  emit_block(&ctx->html, doc.first, ctx->style);
  ctx->html.append("</article>\n</body>\n</html>\n");
}

auto emit_html_string(const ir::Document &doc, const eval::StyleCtx &style) -> std::string {
  EmitCtx ctx { .style = style };
  emit_document(doc, &ctx);
  return ctx.html;
}

} // namespace emit
