#ifndef COMPILER_HPP_
#define COMPILER_HPP_

#include "ast/ast.hpp"
#include "ast/node.hpp"
#include "compiler/block.hpp"
#include "compiler/bytecode.hpp"
#include <cstdint>
#include <string_view>

struct ExprResult {
  enum class Kind { Constant, Register, /* Jump, Local, Global, ... */ };
  Kind kind;
  uint32_t idx; // k-index or register, depending on kind
};

class Compiler {
  std::string_view source_;
  Ast ast_;
  Block block_;
  uint32_t next_reg_ = 0;

  void CompileRoot();
  uint32_t CompileStatements(Node::ExtraRange range);
  uint32_t CompileStatement(unsigned int stmt);
  uint32_t CompileBinOp(Node::Kind kind, unsigned int expr);
  uint32_t CompileIntLiteral(unsigned int lit);
  uint32_t CompileFltLiteral(unsigned int lit);
  std::string SliceFromToken(size_t token);
  uint32_t AllocateReg();
  void FreeRegs(uint32_t keep);

public:
  Compiler(std::string_view source, Ast ast);
  Block Compile();
};

#endif
