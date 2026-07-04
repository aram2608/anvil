#ifndef NODE_HPP_
#define NODE_HPP_
#include <variant>

struct Node {
  struct Index {
    unsigned int value;
    operator unsigned int() const { return value; }
  };

  struct TokenIndex {
    unsigned int value;
    operator unsigned int() const { return value; }
  };

  struct NodeAndNode {
    Index first;
    Index second;
  };

  struct TokenAndNode {
    TokenIndex token;
    Index node;
  };

  struct TokenAndToken {
    TokenIndex first;
    TokenIndex second;
  };

  struct ExtraRange {
    uint32_t start;
    uint32_t end;
  };

  // clang-format off
  using Data = std::variant<
    Index,         // node
    TokenIndex,    // token
    NodeAndNode,   // node_and_node
    TokenAndNode,  // token_and_node
    TokenAndToken, // token_and_token
    ExtraRange,    // extra_range
    std::nullptr_t // Undefined, acts as a sentinel of sorts
    >;
  // clang-format on

  enum class Kind {
    Root,
    Add,
    Sub,
    Mult,
    Div,
    Int,
    Float,
    Reassign,
    Assign,
    Equal,
    NotEqual,
    LesserThan,
    GreaterThan,
    LesserEqual,
    GreaterEqual,
    BoolNot,
    Negate,
    Undefined,
  };

  Kind kind;
  TokenIndex main_token;
  Data data;
};

#endif
