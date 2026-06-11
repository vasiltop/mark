#include <fstream>
#include <iostream>
#include <sstream>

#include "compile.hpp"

auto read_file(const char *path) -> std::string {
  std::ifstream in(path, std::ios::binary);
  if (!in) return {};
  std::ostringstream ss;
  ss << in.rdbuf();
  return ss.str();
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

  auto result = compile::compile_mark(source.c_str(), (s32)source.size());
  if (!result.ok) {
    std::cerr << result.error.line << ':' << result.error.col << ": " << result.error.msg << '\n';
    return 1;
  }

  std::cout << result.html;
  return 0;
}
