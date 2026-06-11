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

struct AssetInput {
  const char *path;
  const char *data_base64;
  const char *mime_type;
};

struct CompileOptions {
  const char *context_json;
  s32 context_json_len;
  AssetInput *assets;
  s32 asset_count;
};

auto compile_mark(const char *source, s32 len, const CompileOptions *opts) -> CompileResult;

inline auto compile_mark(const char *source, s32 len) -> CompileResult {
  return compile_mark(source, len, nullptr);
}

} // namespace compile
