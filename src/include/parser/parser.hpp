#ifndef PARSER_HPP_
#define PARSER_HPP_
#include "ast/ast.hpp"
#include "ast/node.hpp"
#include "soa/soa.hpp"
#include "token/token.hpp"
#include <string>
#include <string_view>
#include <vector>

class Parser {
  std::string_view source_;
  TokenBuffer tokens_;
  unsigned int current_;
  std::vector<Node> nodes_;
  std::vector<Node::Index> scratch_;
  std::vector<unsigned int> extra_data_;
  std::vector<ParseError> errors_;

  Node::Index AddNode(Node node);
  bool IsEnd();
  Node::ExtraRange CommitScratch(const int top);
  void ParseRoot();
  void ParseStatements();
  Node::Index ParseExpression();
  Node::Index ParseExpressionPrecedence(const int min);
  Node::Index ParsePrefix();
  Node::Index ParseAtom();
  Node::TokenIndex Advance();
  Node::TokenIndex Previous();
  Token::Kind TokenKind(const unsigned int idx);

public:
  Parser(std::string source, std::vector<Token> tokens);

  Ast Parse();
};

#endif
