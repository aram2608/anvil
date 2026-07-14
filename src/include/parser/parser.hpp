#ifndef PARSER_HPP_
#define PARSER_HPP_
#include "ast/ast.hpp"
#include "ast/node.hpp"
#include "soa/soa.hpp"
#include "token/token.hpp"
#include <cstdint>
#include <cstring>
#include <string_view>
#include <vector>

namespace Anvil {

class Parser {
  std::string_view source_;
  TokenBuffer tokens_;
  uint32_t current_;
  std::vector<Node> nodes_;
  std::vector<Node::Index> scratch_;
  std::vector<uint32_t> extra_data_;
  std::vector<ParseError> errors_;

  Node::Index AddNode(Node node);

  template <typename T>
  void PushExtraValue(T value) {
    static_assert(sizeof(T) == 4, "Only 32-bit types allowed in extra data");
    uint32_t raw;
    std::memcpy(&raw, &value, 4); // type-punning since we use a NewType pattern
    extra_data_.push_back(raw);
  }

  bool IsEnd();
  Node::ExtraRange CommitScratch(const int top);
  void ParseRoot();
  Node::Index ParseStatements();
  Node::Index ParseBlock();
  Node::Index ParseReturn();
  Node::Index ParseIf();
  Node::Index ParseExpressionStatement();
  Node::Index ParseExpression();
  Node::Index ParseExpressionPrecedence(const int min);
  Node::Index ParsePrefix();
  Node::Index ParsePostfix();
  Node::ExtraRange ParseParams();
  Node::Index ParseProto();
  Node::Index ParseAtom();
  Node::TokenIndex Advance();
  Token::Kind Peek();
  Node::TokenIndex Previous();
  Token::Kind TokenKind(const uint32_t idx);
  void ExpectSemicolon(const size_t errs);
  Node::TokenIndex EatToken(Token::Kind);
  Node::ExtraIndex AddExtra(Node::IfExtra extra);
  Node::ExtraIndex AddExtra(Node::ProtoExtra extra);
  Node::ExtraIndex AddExtra(Node::CallExtra extra);
  void Synchronize();

public:
  Parser(std::string_view source, std::vector<Token> tokens);

  Ast Parse();
};

} // namespace Anvil
#endif
