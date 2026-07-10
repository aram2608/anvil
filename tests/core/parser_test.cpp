#include "ast/ast.hpp"
#include "ast/node.hpp"
#include "lexer/lexer.hpp"
#include "parser/parser.hpp"
#include <gtest/gtest.h>
#include <vector>

using namespace Anvil;

Ast RunParser(const char *src) {
  Lexer l{src};

  auto tokens = l.ScanTokens();

  Parser p{src, tokens};

  return p.Parse();
}

void ExpectKinds(const char *src, std::vector<Node::Kind> expected) {
  auto ast = RunParser(src);
  auto kinds = ast.Nodes().Kinds();
  EXPECT_TRUE(ast.Errors().empty());

  ASSERT_EQ(expected.size(), kinds.size());
  for (unsigned int i = 0; i < expected.size(); i++) {
    EXPECT_EQ(expected[i], kinds[i]) << "at index " << i;
  }
}

using K = Node::Kind;

TEST(Parser, ParseAddition) {
  ExpectKinds("1 + 1;", {K::Root, K::Int, K::Int, K::Add});
}

TEST(Parser, ParseSubtraction) {
  ExpectKinds("1 - 1;", {K::Root, K::Int, K::Int, K::Sub});
}

TEST(Parser, ParseMultiplication) {
  ExpectKinds("1 * 1;", {K::Root, K::Int, K::Int, K::Mult});
}

TEST(Parser, ParseDivision) {
  ExpectKinds("1 / 1;", {K::Root, K::Int, K::Int, K::Div});
}

TEST(Parser, ParseIfFull) {
  std::string source = R"(
    if 1 == 1 {
        1 + 1;
    } else {
        2 + 2;
    }
  )";
  ExpectKinds(source.c_str(), {
                                  K::Root,
                                  K::Int,
                                  K::Int,
                                  K::Equal,
                                  K::Int,
                                  K::Int,
                                  K::Add,
                                  K::Block,
                                  K::Int,
                                  K::Int,
                                  K::Add,
                                  K::Block,
                                  K::IfFull,
                              });
}

TEST(Parser, ParseIfSimple) {
  std::string source = R"(
    if 1 == 1 {
      1 + 1;
    }
  )";
  ExpectKinds(source.c_str(), {
                                  K::Root,
                                  K::Int,
                                  K::Int,
                                  K::Equal,
                                  K::Int,
                                  K::Int,
                                  K::Add,
                                  K::Block,
                                  K::IfSimple,
                              });
}

TEST(Parser, ParseReturn) {
  std::string source = R"(
    if 1 == 1 {
      return 1 + 1;
    }
  )";
  ExpectKinds(source.c_str(), {
                                  K::Root,
                                  K::Int,
                                  K::Int,
                                  K::Equal,
                                  K::Int,
                                  K::Int,
                                  K::Add,
                                  K::ReturnSimple,
                                  K::Block,
                                  K::IfSimple,
                              });
}

TEST(Parser, ParseAssign) {
  ExpectKinds("a := 4;", {K::Root, K::Ident, K::Int, K::Assign});
}

TEST(Parser, ParseReassign) {
  ExpectKinds("a = 4;", {K::Root, K::Ident, K::Int, K::Reassign});
}
