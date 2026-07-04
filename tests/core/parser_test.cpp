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

TEST(Parser, ParseBinops) {
  std::vector<Node::Kind> kCases = {
      Node::Kind::Root,
      Node::Kind::Int,
      Node::Kind::Int,
      Node::Kind::Add,
  };

  auto ast = RunParser("1 + 1");

  ASSERT_EQ(kCases.size(), ast.Nodes().Kinds().size());
}
