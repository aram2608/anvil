#ifndef AST_HPP_
#define AST_HPP_
#include "soa/soa.hpp"
#include <cstddef>
#include <cstdint>
#include <span>
#include <string_view>
#include <vector>

namespace Anvil {

struct ParseError {
  enum class Kind {
    UnexpectedToken,
    MissingSemicolon,
    MissingClosingBrace,
    ExpectedLeftParen,
    ExpectedComma,
    ExpectedCommaOrRightParen
  };

  Kind kind;
  Node::TokenIndex token;
};

class Ast {
  std::string_view source_; // Extern. owned data
  std::vector<ParseError> errors_;
  std::vector<uint32_t> extra_data_;
  TokenBuffer tokens_;
  NodeBuffer nodes_;

public:
  Ast(std::string_view source, TokenBuffer tokens, NodeBuffer nodes,
      std::vector<uint32_t> extra_data, std::vector<ParseError> errors);

  const TokenBuffer &Tokens() const { return tokens_; }
  const NodeBuffer &Nodes() const { return nodes_; }
  // Surely theres a smarter way to do this
  std::span<const uint32_t> Children(Node::ExtraRange r) const {
    return std::span{extra_data_}.subspan(r.start, r.end - r.start);
  }
  bool CheckErrors() const;
  std::span<const uint32_t> ExtraData() const { return extra_data_; }
  std::span<const ParseError> Errors() const { return errors_; }
};

} // namespace Anvil

#endif
