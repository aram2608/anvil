#ifndef AST_HPP_
#define AST_HPP_
#include "soa/soa.hpp"
#include <string_view>
#include <vector>

struct ParseError {
  enum class Kind { UnexpectedToken };

  Kind kind;
  Node::TokenIndex token;
};

class Ast {
  std::string_view source_; // Extern. owned data
  std::vector<ParseError> errors_;
  TokenBuffer tokens_;
  NodeBuffer nodes_;

public:
  Ast(std::string_view source, TokenBuffer tokens, NodeBuffer nodes,
      std::vector<ParseError> errors);

  const TokenBuffer &Tokens() const { return tokens_; }
  const NodeBuffer &Nodes() const { return nodes_; }
};

#endif
