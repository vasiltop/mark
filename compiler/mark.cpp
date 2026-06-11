#include <fstream>
#include <iostream>
#include <sstream>
#include <vector>

#include "compile.hpp"

auto read_file(const char *path) -> std::string {
  std::ifstream in(path, std::ios::binary);
  if (!in) return {};
  std::ostringstream ss;
  ss << in.rdbuf();
  return ss.str();
}

auto compile_source(const char *source, s32 len) -> bool {
  arena::Arena *arena = arena::alloc();
  if (!arena) {
    std::cerr << "arena alloc failed\n";
    return false;
  }

  lexer::Lexer lex {};
  lexer::lex_init(&lex, source, len);

  parser::Parser p {
    .lex = &lex,
    .arena = arena,
    .has_error = false,
  };

  auto result = parser::parse_document(&p);
  if (!result.ok) {
    std::cerr << result.error.line << ':' << result.error.col << ": " << result.error.msg << '\n';
    arena::release(arena);
    return false;
  }

  auto evaluated = eval::eval_document(result.doc);
  emit::EmitCtx ctx { .style = evaluated.style };
  emit::emit_document(evaluated.doc, &ctx);
  std::cout << ctx.html;

  arena::release(arena);
  return true;
}

auto main(int argc, char **argv) -> int {
  std::string source;
  if (argc > 1) {
    source = read_file(argv[1]);
    if (source.empty()) {
      std::cerr << "failed to read: " << argv[1] << '\n';
      return 1;
    }
  } else {
    source.assign(std::istreambuf_iterator<char>(std::cin), {});
  }

  if (!compile_source(source.c_str(), (s32)source.size())) return 1;
  return 0;
}
