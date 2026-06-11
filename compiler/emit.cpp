#include "emit.hpp"
#include <cstdio>
#include <cstring>

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
      [&](const ir::Math &m) {
        out->append("<span class=\"mark-math\">");
        append_escaped(out, m.start, m.len);
        out->append("</span>");
      },
      [&](const ir::FuncCall &f) {
        if (f.name && std::strcmp(f.name, "align") == 0) {
          out->append("<span class=\"mark-align\">");
          emit_inline(out, f.content, style);
          out->append("</span>");
          return;
        }
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
  css->append(".mark-grid { display: grid; gap: 1em; margin: 1em 0; }\n");
  css->append(".mark-box { border: 1px solid #ccc; padding: 1em; margin: 1em 0; }\n");
  css->append(".mark-align { text-align: center; margin: 1em 0; }\n");
  css->append(".mark-math { font-family: ui-monospace, monospace; font-style: italic; }\n");
  css->append("@media print { body { max-width: none; margin: 0; } .mark-heading { page-break-after: avoid; } }\n");
}

internal auto find_show_class(const ir::Document &doc, const char *target) -> const char * {
  for (auto *block = doc.first; block; block = block->next) {
    if (auto *show = std::get_if<ir::ShowRule>(&block->item)) {
      if (show->target && target && std::strcmp(show->target, target) == 0)
        return show->class_name;
    }
  }
  return nullptr;
}

internal auto emit_block(std::string *out, ir::BlockNode *node, eval::StyleCtx *style, const ir::Document &doc) -> void {
  for (; node; node = node->next) {
    std::visit(overloaded {
      [&](const ir::Heading &h) {
        char tag[4] = "h1";
        s32 lvl = h.level < 1 ? 1 : (h.level > 6 ? 6 : h.level);
        tag[1] = (char)('0' + lvl);
        out->append("<");
        out->append(tag);
        out->append(" class=\"mark-heading");
        if (auto *show_class = find_show_class(doc, "heading")) {
          out->append(" ");
          out->append(show_class);
        }
        out->append("\"");
        if (h.label) {
          out->append(" id=\"");
          append_escaped(out, h.label);
          out->append("\"");
        }
        out->append(">");
        if (style->heading_numbering) {
          for (s32 i = lvl + 1; i <= 6; i++) style->heading_counters[i] = 0;
          style->heading_counters[lvl]++;
          for (s32 i = 1; i <= lvl; i++) {
            if (i > 1) out->append(".");
            char num[16];
            std::snprintf(num, sizeof(num), "%d", style->heading_counters[i]);
            out->append(num);
          }
          out->append(" ");
        }
        emit_inline(out, h.content, *style);
        out->append("</");
        out->append(tag);
        out->append(">\n");
      },
      [&](const ir::Paragraph &p) {
        out->append("<p class=\"mark-para\">");
        emit_inline(out, p.content, *style);
        out->append("</p>\n");
      },
      [&](const ir::List &l) {
        out->append("<ul class=\"mark-list\">\n");
        for (auto *item = l.first; item; item = item->next) {
          out->append("<li>");
          emit_inline(out, item->content, *style);
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
            emit_inline(out, cell->content, *style);
            out->append("</td>");
          }
          out->append("</tr>\n");
        }
        out->append("</table>\n");
      },
      [&](const ir::SetRule &) {},
      [&](const ir::ShowRule &) {},
      [&](const ir::LayoutBlock &lb) {
        if (lb.kind && std::strcmp(lb.kind, "grid") == 0) {
          out->append("<div class=\"mark-grid\" style=\"grid-template-columns: repeat(");
          char buf[16];
          std::snprintf(buf, sizeof(buf), "%d", lb.columns);
          out->append(buf);
          out->append(", 1fr)\">");
          emit_inline(out, lb.content, *style);
          out->append("</div>\n");
        } else if (lb.kind && std::strcmp(lb.kind, "box") == 0) {
          out->append("<div class=\"mark-box\">");
          emit_inline(out, lb.content, *style);
          out->append("</div>\n");
        } else if (lb.kind && std::strcmp(lb.kind, "align") == 0) {
          out->append("<div class=\"mark-align\">");
          emit_inline(out, lb.content, *style);
          out->append("</div>\n");
        }
      },
      [&](const ir::FuncCall &f) {
        out->append("<div class=\"mark-func\">");
        emit_inline(out, f.content, *style);
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
  emit_block(&ctx->html, doc.first, &ctx->style, doc);
  ctx->html.append("</article>\n</body>\n</html>\n");
}

auto emit_html_string(const ir::Document &doc, const eval::StyleCtx &style) -> std::string {
  EmitCtx ctx { .style = style };
  emit_document(doc, &ctx);
  return ctx.html;
}

} // namespace emit
