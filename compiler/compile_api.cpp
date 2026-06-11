#include "compile_api.hpp"
#include "arena.hpp"
#include "lexer.hpp"
#include "parser.hpp"
#include "eval.hpp"
#include "emit.hpp"

#include <fstream>
#include <sstream>
#include <cctype>

namespace compile {

internal auto read_include(const char *path) -> std::string {
  std::ifstream in(path, std::ios::binary);
  if (!in) return {};
  std::ostringstream ss;
  ss << in.rdbuf();
  return ss.str();
}

internal auto expand_includes(const std::string &source, s32 depth) -> std::string {
  if (depth > 8) return source;
  std::string out;
  std::istringstream in(source);
  std::string line;
  while (std::getline(in, line)) {
    auto is_include = line.rfind("#include \"", 0) == 0;
    auto is_import = line.rfind("#import \"", 0) == 0;
    if (is_include || is_import) {
      auto prefix_len = is_include ? 10 : 9;
      auto start = line.find('"') + 1;
      auto end = line.rfind('"');
      if (end != std::string::npos && end > start) {
        auto path = line.substr(start, end - start);
        auto nested = read_include(path.c_str());
        if (!nested.empty()) out += expand_includes(nested, depth + 1);
        continue;
      }
    }
    out += line;
    out += '\n';
  }
  return out;
}

internal auto json_unescape(const std::string &input) -> std::string {
  std::string out;
  out.reserve(input.size());
  for (std::size_t i = 0; i < input.size(); i++) {
    if (input[i] == '\\' && i + 1 < input.size()) {
      out.push_back(input[i + 1]);
      i++;
      continue;
    }
    out.push_back(input[i]);
  }
  return out;
}

internal auto json_lookup(const std::string &json, const std::string &path) -> std::string {
  if (json.empty() || path.empty()) return {};
  std::string current = json;
  std::size_t start = 0;
  while (start < path.size()) {
    auto dot = path.find('.', start);
    auto key = path.substr(start, dot == std::string::npos ? std::string::npos : dot - start);
    auto needle = std::string("\"") + key + "\":";
    auto pos = current.find(needle);
    if (pos == std::string::npos) return {};
    pos += needle.size();
    while (pos < current.size() && (current[pos] == ' ' || current[pos] == '\t')) pos++;
    if (pos >= current.size()) return {};
    if (current[pos] == '"') {
      pos++;
      std::string value;
      for (; pos < current.size(); pos++) {
        if (current[pos] == '\\' && pos + 1 < current.size()) {
          value.push_back(current[pos + 1]);
          pos++;
          continue;
        }
        if (current[pos] == '"') return value;
        value.push_back(current[pos]);
      }
      return {};
    }
    if (current[pos] == '{') {
      s32 depth = 0;
      auto obj_start = pos;
      for (; pos < current.size(); pos++) {
        if (current[pos] == '{') depth++;
        else if (current[pos] == '}') {
          depth--;
          if (depth == 0) {
            current = current.substr(obj_start, pos - obj_start + 1);
            break;
          }
        }
      }
      if (dot == std::string::npos) return {};
      start = dot + 1;
      continue;
    }
    return {};
  }
  return {};
}

internal auto expand_context_refs(const std::string &source, const char *json, s32 json_len) -> std::string {
  std::string json_str;
  if (json && json_len > 0) json_str.assign(json, json_len);
  std::string out;
  std::size_t i = 0;
  while (i < source.size()) {
    auto pos = source.find("#context(", i);
    if (pos == std::string::npos) {
      out += source.substr(i);
      break;
    }
    out += source.substr(i, pos - i);
    pos += 9;
    while (pos < source.size() && source[pos] == ' ') pos++;
    std::string key;
    if (pos < source.size() && source[pos] == '"') {
      pos++;
      while (pos < source.size() && source[pos] != '"') {
        if (source[pos] == '\\' && pos + 1 < source.size()) {
          key.push_back(source[pos + 1]);
          pos += 2;
          continue;
        }
        key.push_back(source[pos++]);
      }
      if (pos < source.size() && source[pos] == '"') pos++;
    } else {
      while (pos < source.size() && (std::isalnum(static_cast<unsigned char>(source[pos])) || source[pos] == '_' || source[pos] == '.'))
        key.push_back(source[pos++]);
    }
    while (pos < source.size() && source[pos] != ')') pos++;
    if (pos < source.size()) pos++;

    auto value = json_str.empty() ? std::string {} : json_lookup(json_str, key);
    out += value;
    i = pos;
  }
  return out;
}

internal auto expand_assets(const std::string &source, AssetInput *assets, s32 count) -> std::string {
  if (!assets || count <= 0) return source;
  std::string out = source;
  for (s32 i = 0; i < count; i++) {
    if (!assets[i].path || !assets[i].data_base64) continue;
    auto needle = std::string("\"") + assets[i].path + "\"";
    const char *mime = assets[i].mime_type ? assets[i].mime_type : "application/octet-stream";
    auto replacement = std::string("\"data:") + mime + ";base64," + assets[i].data_base64 + "\"";
    std::size_t pos = 0;
    while ((pos = out.find(needle, pos)) != std::string::npos) {
      out.replace(pos, needle.size(), replacement);
      pos += replacement.size();
    }
  }
  return out;
}

auto compile_mark(const char *source, s32 len, const CompileOptions *opts) -> CompileResult {
  arena::Arena *arena = arena::alloc();
  if (!arena) {
    return CompileResult {
      .ok = false,
      .error = { .line = 0, .col = 0, .msg = "arena alloc failed" },
    };
  }

  std::string expanded = expand_includes(std::string(source, len), 0);
  expanded = expand_context_refs(expanded, opts ? opts->context_json : nullptr, opts ? opts->context_json_len : 0);
  if (opts && opts->assets && opts->asset_count > 0)
    expanded = expand_assets(expanded, opts->assets, opts->asset_count);

  lexer::Lexer lex {};
  lexer::lex_init(&lex, expanded.c_str(), (s32)expanded.size());

  parser::Parser p {};
  p.lex = &lex;
  p.arena = arena;
  p.has_error = false;

  auto parsed = parser::parse_document(&p);
  if (!parsed.ok) {
    CompileResult out {
      .ok = false,
      .error = {
        .line = parsed.error.line,
        .col = parsed.error.col,
        .msg = parsed.error.msg ? parsed.error.msg : "parse error",
      },
    };
    arena::release(arena);
    return out;
  }

  auto evaluated = eval::eval_document(parsed.doc);
  emit::EmitCtx ctx {};
  ctx.style = evaluated.style;
  emit::emit_document(evaluated.doc, &ctx);

  CompileResult out {
    .ok = true,
    .html = std::move(ctx.html),
    .css = std::move(ctx.css),
  };
  arena::release(arena);
  return out;
}

} // namespace compile
