#include "compile_api.hpp"
#include "arena.hpp"
#include "lexer.hpp"
#include "parser.hpp"
#include "eval.hpp"
#include "emit.hpp"

#include <fstream>
#include <sstream>

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
    if (line.rfind("#include \"", 0) == 0) {
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

auto compile_mark(const char *source, s32 len) -> CompileResult {
  arena::Arena *arena = arena::alloc();
  if (!arena) {
    return CompileResult {
      .ok = false,
      .error = { .line = 0, .col = 0, .msg = "arena alloc failed" },
    };
  }

  std::string expanded = expand_includes(std::string(source, len), 0);

  lexer::Lexer lex {};
  lexer::lex_init(&lex, expanded.c_str(), (s32)expanded.size());

  parser::Parser p {
    .lex = &lex,
    .arena = arena,
    .has_error = false,
  };

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
  emit::EmitCtx ctx { .style = evaluated.style };
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
