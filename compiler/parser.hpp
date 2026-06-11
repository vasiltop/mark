#pragma once

#include "arena.hpp"
#include "ir.hpp"
#include "lexer.hpp"

namespace parser {

struct ParseError {
  s32 line;
  s32 col;
  const char *msg;
};

struct ParseResult {
  ir::Document doc;
  ParseError error;
  bool ok;
};

struct Parser {
  lexer::Lexer *lex;
  arena::Arena *arena;
  lexer::Token cur;
  lexer::Token peek_tok;
  bool has_error;
  ParseError error;
};

auto parse_document(Parser *p) -> ParseResult;

} // namespace parser
