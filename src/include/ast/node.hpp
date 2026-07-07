#ifndef NODE_HPP_
#define NODE_HPP_
#include <cstdint>
#include <variant>

template <typename T>
inline uint32_t ToU32(T index) {
  return static_cast<uint32_t>(index);
}

struct Node {
  enum class Index : uint32_t { None = 0xFFFFFFFF };
  enum class TokenIndex : uint32_t { None = 0xFFFFFFFF };
  enum class ExtraIndex : uint32_t { None = 0xFFFFFFFF };

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
    std::monostate // Undefined, acts as a sentinel of sorts
    >;
  // clang-format on

  enum class Kind {
    Root,
    IfSimple,
    IfFull,
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

  struct IfExtra {
    Index then_expr;
    Index else_expr;
  };
};

#endif
