#pragma once

#include "core.hpp"
#include <string>

namespace compile {

struct CompileError {
  s32 line;
  s32 col;
  std::string msg;
};

struct CompileResult {
  bool ok;
  std::string html;
  std::string css;
  CompileError error;
};

auto compile_mark(const char *source, s32 len) -> CompileResult;

} // namespace compile
