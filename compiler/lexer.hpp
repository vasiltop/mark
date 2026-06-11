#pragma once

#include "core.hpp"

namespace lexer {

enum class Mode {
  Markup,
  Code,
};

enum class TokenKind {
  Eof,
  Newline,
  Text,
  Hash,
  Equals,
  Star,
  Underscore,
  Minus,
  Plus,
  LBracket,
  RBracket,
  LParen,
  RParen,
  LBrace,
  RBrace,
  Comma,
  Colon,
  Dot,
  At,
  Lt,
  Gt,
  Backslash,
  String,
  Ident,
  Int,
  Slash,
  Semicolon,
};

struct Token {
  TokenKind kind;
  s32 line;
  s32 col;
  const char *start;
  s32 len;
};

struct Lexer {
  const char *src;
  const char *end;
  const char *cur;
  s32 line;
  s32 col;
  Mode mode;
  bool at_line_start;
};

auto lex_init(Lexer *lex, const char *src, s32 len) -> void;
auto lex_next(Lexer *lex) -> Token;
auto lex_peek(Lexer *lex) -> Token;

} // namespace lexer
