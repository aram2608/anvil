#include "compiler/block.hpp"
#include "compiler/bytecode.hpp"
#include "compiler/compiler.hpp"
#include "lexer/lexer.hpp"
#include "parser/parser.hpp"
#include "vm/object.hpp"
#include "vm/vm.hpp"
#include <gtest/gtest.h>
#include <vector>

Object::Value RunMockVM(const char *src) {
  Lexer l{src};

  auto tokens = l.ScanTokens();

  Parser p{src, tokens};

  Compiler c{src, p.Parse()};

  VM vm{c.Compile()};
  return vm.MockRun();
}

TEST(VM, Binops) {
  ASSERT_EQ(2, RunMockVM("1 + 1;").as.i);
  ASSERT_EQ(0, RunMockVM("1 - 1;").as.i);
  ASSERT_EQ(1.0, RunMockVM("0 + 1.0;").as.f);
}
