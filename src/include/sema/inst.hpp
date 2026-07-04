#ifndef INST_HPP_
#define INST_HPP_
#include "ast/node.hpp"
#include <variant>

struct Inst {
  struct Bin {
    Node::Index lhs;
    Node::Index rhs;
  };

  struct Unary {
    Node::Index node;
  };

  struct Int {
    long long int value;
  };

  struct Float {
    double value;
  };

  // clang-format off
  using Data = std::variant<
    Node::Index,         // node
    Node::TokenIndex,    // token
    Bin,                 // Binary op, resolved with `Kind`
    Int,                 // 64 bit integer, signed
    Float,               // 64 bit floating point value
    std::nullptr_t       // Undefined, acts as a sentinel of sorts
    >;
  // clang-format on

  enum class Kind {
    // Arithmetic Instructions
    // If the `lhs` and `rhs` types can be resolved at Sema time
    // a defined operation is pushed so the JIT can work on a hot path
    // If any of the operands are ambiguous (ie 1 + call()) a generic op used
    Add,
    AddInt,
    AddFloat,
    Sub,
    SubInt,
    SubFloat,
    Mult,
    MultInt,
    MultFloat,
    Div,
    DivInt,
    DivFloat,
    BoolNot,
    Negate,
  };

  Kind kind;
  Data data;
};

#endif
