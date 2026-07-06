#ifndef AST_HPP_
#define AST_HPP_
#include "soa/soa.hpp"
#include <cstddef>
#include <span>
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
  std::vector<unsigned int> extra_data_;
  TokenBuffer tokens_;
  NodeBuffer nodes_;

public:
  Ast(std::string_view source, TokenBuffer tokens, NodeBuffer nodes,
      std::vector<unsigned int> extra_data, std::vector<ParseError> errors);

  const TokenBuffer &Tokens() const { return tokens_; }
  const NodeBuffer &Nodes() const { return nodes_; }
  bool CheckErrors() const;
  std::span<unsigned int> ExtraData() { return extra_data_; }
};

#endif
