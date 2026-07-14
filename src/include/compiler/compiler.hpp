#ifndef COMPILER_HPP_
#define COMPILER_HPP_

#include "ast/ast.hpp"
#include "ast/node.hpp"
#include "compiler/block.hpp"
#include "compiler/bytecode.hpp"
#include "strpool/strpool.hpp"
#include "strtable/strtable.hpp"
#include <cstdint>
#include <string_view>
#include <vector>

namespace Anvil {

struct ExprResult {
  enum class Kind { Constant, Register, Jmp, Local, Global };
  Kind kind;
  uint32_t idx; // k-index or register, depending on kind
};

static_assert(sizeof(ExprResult) <= 16);

struct Local {
  StrPool::Index idx;
  uint32_t depth;
};

struct FuncState {
  Block block;
  uint32_t next_reg = 0;
  uint32_t max_regs = 0;
  std::vector<Local> locals;
  uint32_t depth = 0;
};

class Compiler {
  std::string_view source_;
  Ast ast_;
  StrPool strings_;
  StringTable &str_table_;
  std::vector<Proto> funcs_;
  std::vector<StrPool::Index> globals_; // slot = vector index
  uint32_t func_depth_ = 0;             // 0 = compiling main
  Module module_;
  FuncState fs_;

  void CompileRoot();
  ExprResult CompileFuncProto(uint32_t stmt);
  ExprResult CompileExpressions(Node::ExtraRange range);
  ExprResult CompileExpression(uint32_t stmt);
  ExprResult CompileBuiltinCall(uint32_t node);
  ExprResult CompileCall(uint32_t stmt);
  ExprResult CompileAssign(uint32_t stmt);
  ExprResult CompileReturnSimple(uint32_t stmt);
  ExprResult CompileIfSimple(uint32_t stmt);
  ExprResult CompileIfFull(uint32_t stmt);
  ExprResult CompileBlock(uint32_t stmt);
  ExprResult CompileBinOp(Node::Kind kind, uint32_t expr);
  ExprResult CompileIdent(uint32_t ident);
  ExprResult CompileIntLiteral(uint32_t lit);
  ExprResult CompileFltLiteral(uint32_t lit);
  ExprResult CompileStringLiteral(uint32_t stmt);
  ExprResult CompileTrue(uint32_t stmt);
  ExprResult CompileFalse(uint32_t stmt);
  ExprResult EmitLoadK(Object::Value v);
  std::string_view SliceFromToken(Node::TokenIndex token);
  int64_t IntFromToken(Node::TokenIndex token);
  double FloatFromToken(Node::TokenIndex token);
  std::optional<uint32_t> LookupNative(std::string_view name);
  uint32_t AllocateReg();
  uint32_t BindGlobal(StrPool::Index name);
  void FreeReg(uint32_t keep);
  void FreeExpr(ExprResult e);
  uint32_t ExprToNextReg(ExprResult &e);
  uint32_t ExprToAnyReg(ExprResult &e);
  void DischargeToReg(ExprResult e, uint32_t reg);
  uint32_t EmitJump();
  void PatchJumpToHere(uint32_t jump_idx);
  void EnterScope();
  void ExitScope();

public:
  Compiler(std::string_view source, Ast ast, StringTable &str_table);
  Module Compile();
};

} // namespace Anvil

#endif
