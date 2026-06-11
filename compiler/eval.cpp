#include "eval.hpp"
#include <cstring>
#include <variant>

namespace eval {

auto eval_document(const ir::Document &doc) -> EvalResult {
  StyleCtx style {};
  for (auto *block = doc.first; block; block = block->next) {
    if (auto *rule = std::get_if<ir::SetRule>(&block->item)) {
      if (std::strcmp(rule->target, "text") == 0) {
        style.font_family = "Georgia, serif";
        style.font_size = "12pt";
      }
      if (std::strcmp(rule->target, "link") == 0) {
        style.link_color = "#0066cc";
      }
    }
  }
  return EvalResult { .style = style, .doc = doc };
}

} // namespace eval
