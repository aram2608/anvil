#include "ast/ast.hpp"
#include "ast/node.hpp"
#include "lexer/lexer.hpp"
#include "parser/parser.hpp"
#include <gtest/gtest.h>
#include <vector>

Ast RunParser(const char *src) {
  Lexer l{src};

  auto tokens = l.ScanTokens();

  Parser p{src, tokens};

  return p.Parse();
}

void ExpectKinds(const char *src, std::vector<Node::Kind> expected) {
  auto ast = RunParser(src);
  auto kinds = ast.Nodes().Kinds();

  ASSERT_EQ(expected.size(), kinds.size());
  for (unsigned int i = 0; i < expected.size(); i++) {
    EXPECT_EQ(expected[i], kinds[i]) << "at index " << i;
  }
}

using K = Node::Kind;

TEST(Parser, ParseAddition) {
  ExpectKinds("1 + 1", {K::Root, K::Int, K::Int, K::Add});
}

TEST(Parser, ParseSubtraction) {
  ExpectKinds("1 - 1", {K::Root, K::Int, K::Int, K::Sub});
}

TEST(Parser, ParseMultiplication) {
  ExpectKinds("1 * 1", {K::Root, K::Int, K::Int, K::Mult});
}

TEST(Parser, ParseDivision) {
  ExpectKinds("1 / 1", {K::Root, K::Int, K::Int, K::Div});
}
