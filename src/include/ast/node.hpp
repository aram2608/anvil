#ifndef NODE_HPP_
#define NODE_HPP_
#include <cstdint>
#include <variant>

namespace Anvil {

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

  struct NodeAndExtra {
    Index node;
    ExtraIndex extra;
  };

  // clang-format off
  using Data = std::variant<
    Index,         // node
    TokenIndex,    // token
    NodeAndNode,   // node_and_node
    TokenAndNode,  // token_and_node
    TokenAndToken, // token_and_token
    NodeAndExtra,  // node_and_extra
    ExtraRange,    // extra_range
    std::monostate // Undefined, acts as a sentinel of sorts
    >;
  // clang-format on

  enum class Kind {
    Root,
    IfSimple,
    IfFull,
    ReturnSimple,
    ReturnMulti,
    Ident,
    Block,
    Add,
    Sub,
    Mult,
    Div,
    Int,
    Float,
    String,
    True,
    False,
    Reassign,
    Assign,
    BuiltinCall,
    Equal,
    NotEqual,
    LesserThan,
    GreaterThan,
    LesserEqual,
    GreaterEqual,
    BoolNot,
    Negate,
    LogicalAnd,
    LogicalOr,
    BitAnd,
    BitOr,
    BitXor,
    BitNot,
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

} // namespace Anvil

#endif
