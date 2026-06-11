#include "compile_api.hpp"
#include "arena.hpp"
#include "lexer.hpp"
#include "parser.hpp"
#include "eval.hpp"
#include "emit.hpp"

namespace compile {

auto compile_mark(const char *source, s32 len) -> CompileResult {
  arena::Arena *arena = arena::alloc();
  if (!arena) {
    return CompileResult {
      .ok = false,
      .error = { .line = 0, .col = 0, .msg = "arena alloc failed" },
    };
  }

  lexer::Lexer lex {};
  lexer::lex_init(&lex, source, len);

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
