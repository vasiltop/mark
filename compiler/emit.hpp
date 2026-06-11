#pragma once

#include "eval.hpp"
#include <string>

namespace emit {

struct EmitCtx {
  std::string html;
  std::string css;
  eval::StyleCtx style;
};

auto emit_document(const ir::Document &doc, EmitCtx *ctx) -> void;
auto emit_html_string(const ir::Document &doc, const eval::StyleCtx &style) -> std::string;

} // namespace emit
