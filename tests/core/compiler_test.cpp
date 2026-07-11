#include "compiler/block.hpp"
#include "compiler/bytecode.hpp"
#include "compiler/compiler.hpp"
#include "lexer/lexer.hpp"
#include "parser/parser.hpp"
#include "strtable/strtable.hpp"
#include <gtest/gtest.h>
#include <vector>

using namespace Anvil;

StringTable compile_table;

Block RunCompiler(const char *src) {
  Lexer l{src};

  auto tokens = l.ScanTokens();

  Parser p{src, tokens};

  Compiler c{src, p.Parse(), compile_table};

  return c.Compile();
}

TEST(Compiler, CompileBinOps) {
  Block b = RunCompiler("1 + 1;");

  ASSERT_EQ(b.ConstantsSize(), 2);
  ASSERT_EQ(b.OpcodesSize(), 4);

  Code::Inst ret = b.get_code()[b.OpcodesSize() - 1];
  ASSERT_EQ(Code::GetOp(ret), Code::Op::Ret);
  ASSERT_EQ(Code::GetA(ret), 0);
  ASSERT_EQ(Code::GetB(ret), 2); // nresults + 1
}
