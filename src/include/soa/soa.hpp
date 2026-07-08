#ifndef SOA_HPP_
#define SOA_HPP_

#include "ast/node.hpp"
#include "token/token.hpp"

#include <span>
#include <vector>

class TokenBuffer {
  std::vector<Token::Kind> kind_;
  std::vector<int> start_;
  std::vector<int> len_;
  std::vector<int> line_;

public:
  explicit TokenBuffer(std::vector<Token> &tokens);

  std::span<const Token::Kind> Kinds() const { return kind_; }
  std::span<const int> Starts() const { return start_; }
  std::span<const int> Lens() const { return len_; }
  std::span<const int> Lines() const { return line_; }
};

class NodeBuffer {
  std::vector<Node::Kind> kind_;
  std::vector<Node::TokenIndex> main_token_;
  std::vector<Node::Data> data_;

public:
  explicit NodeBuffer(std::vector<Node> &nodes);

  std::span<const Node::Kind> Kinds() const { return kind_; }
  std::span<const Node::TokenIndex> MainTokens() const { return main_token_; }
  std::span<const Node::Data> Datas() const { return data_; }
};

#endif
