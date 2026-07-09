#ifndef COMPILER_HPP_
#define COMPILER_HPP_

#include "ast/ast.hpp"
#include "ast/node.hpp"
#include "compiler/block.hpp"
#include "compiler/bytecode.hpp"
#include <cstdint>
#include <string_view>

struct ExprResult {
  enum class Kind { Constant, Register, Jmp /* Local, Global, ... */ };
  Kind kind;
  uint32_t idx; // k-index or register, depending on kind
};

class Compiler {
  std::string_view source_;
  Ast ast_;
  Block block_;
  uint32_t next_reg_ = 0;

  void CompileRoot();
  uint32_t CompileExpressions(Node::ExtraRange range);
  uint32_t CompileExpression(uint32_t stmt);
  uint32_t CompileReturnSimple(uint32_t stmt);
  uint32_t CompileIfSimple(uint32_t stmt);
  uint32_t CompileIfFull(uint32_t stmt);
  uint32_t CompileBlock(uint32_t stmt);
  uint32_t CompileBinOp(Node::Kind kind, uint32_t expr);
  uint32_t EmitLoadK(Object::Value v);
  uint32_t CompileIntLiteral(uint32_t lit);
  uint32_t CompileFltLiteral(uint32_t lit);
  std::string SliceFromToken(Node::TokenIndex token);
  uint32_t AllocateReg();
  void FreeReg(uint32_t keep);
  uint32_t EmitJump();
  void PatchJumpToHere(uint32_t jump_idx);

public:
  Compiler(std::string_view source, Ast ast);
  Block Compile();
};

#endif
