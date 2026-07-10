#include "soa/soa.hpp"

using namespace Anvil;

TokenBuffer::TokenBuffer(std::vector<Token> &tokens) {
  kind_.resize(tokens.size());
  start_.resize(tokens.size());
  len_.resize(tokens.size());
  line_.resize(tokens.size());

  for (std::size_t i = 0; i < tokens.size(); ++i) {
    kind_[i] = tokens[i].kind;
    start_[i] = tokens[i].start;
    len_[i] = tokens[i].len;
    line_[i] = tokens[i].line;
  }
}

NodeBuffer::NodeBuffer(std::vector<Node> &nodes) {
  kind_.resize(nodes.size());
  main_token_.resize(nodes.size());
  data_.resize(nodes.size());

  for (std::size_t i = 0; i < nodes.size(); ++i) {
    kind_[i] = nodes[i].kind;
    main_token_[i] = nodes[i].main_token;
    data_[i] = nodes[i].data;
  }
}
