#pragma once

#include "ir.hpp"
#include <string>
#include <vector>

namespace eval {

struct StyleCtx {
  std::string font_family {"Georgia, serif"};
  std::string font_size {"12pt"};
  std::string text_color {"#333"};
  std::string link_color {"#0066cc"};
  bool heading_numbering {false};
  s32 heading_counters[7] {};
};

struct EvalResult {
  StyleCtx style;
  ir::Document doc;
};

auto eval_document(const ir::Document &doc) -> EvalResult;

} // namespace eval
