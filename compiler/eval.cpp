#include "eval.hpp"
#include <cstring>
#include <variant>

namespace eval {

internal auto apply_prop(StyleCtx *style, const char *key, const char *value) -> void {
  if (!key || !value) return;
  if (std::strcmp(key, "font-family") == 0) style->font_family = value;
  else if (std::strcmp(key, "font-size") == 0) style->font_size = value;
  else if (std::strcmp(key, "color") == 0) style->text_color = value;
  else if (std::strcmp(key, "link-color") == 0) style->link_color = value;
}

auto eval_document(const ir::Document &doc) -> EvalResult {
  StyleCtx style {};
  for (auto *block = doc.first; block; block = block->next) {
    if (auto *rule = std::get_if<ir::SetRule>(&block->item)) {
      for (auto *prop = rule->props; prop; prop = prop->next)
        apply_prop(&style, prop->key, prop->value);
    }
  }
  return EvalResult { .style = style, .doc = doc };
}

} // namespace eval
