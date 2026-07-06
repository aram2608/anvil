#include "compiler/block.hpp"
#include "compiler/compiler.hpp"
#include "lexer/lexer.hpp"
#include "parser/parser.hpp"
#include <gtest/gtest.h>
#include <vector>

Block RunCompiler(const char *src) {
  Lexer l{src};

  auto tokens = l.ScanTokens();

  Parser p{src, tokens};

  Compiler c{src, p.Parse()};

  return c.Compile();
}

TEST(Compiler, CompileBinOps) {
  Block b = RunCompiler("1 + 1");

  ASSERT_EQ(b.ConstantsSize(), 2);
  ASSERT_EQ(b.OpcodesSize(), 3);
}
