#include "compiler/compiler.hpp"
#include "ast/node.hpp"
#include "compiler/bytecode.hpp"
#include "strpool/strpool.hpp"
#include "strtable/strtable.hpp"
#include "utils/die.hpp"
#include "vm/builtins.hpp"
#include "vm/object.hpp"
#include <cassert>
#include <cerrno>
#include <charconv>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <iterator>
#include <optional>
#include <string>
#include <string_view>
#include <variant>

using namespace Anvil;

Compiler::Compiler(std::string_view source, Ast ast, StringTable &str_table)
    : source_{source}, ast_{ast}, str_table_{str_table} {}

Block Compiler::Compile() {
  CompileRoot();
  return block_;
}

void Compiler::CompileRoot() {
  auto root = ast_.Nodes().Datas()[0];
  auto range = std::get<Node::ExtraRange>(root);
  ExprResult v = CompileExpressions(range);
  block_.PushInstruction(
      Code::CreateABCk(Code::Op::Ret, ExprToAnyReg(v), 2, 0));
}

ExprResult Compiler::CompileExpressions(Node::ExtraRange range) {
  auto children = ast_.Children(range);

  if (children.empty()) {
    return EmitLoadK(Object::mkVoid({}));
  }

  for (size_t i = 0; i < children.size() - 1; i++) {
    FreeExpr(CompileExpression(children[i]));
    // make sure the register shrinks with the locals
    assert(next_reg_ == locals_.size());
  }

  return CompileExpression(children[children.size() - 1]);
}

ExprResult Compiler::CompileExpression(uint32_t stmt) {
  Node::Kind kind = ast_.Nodes().Kinds()[stmt];

  switch (kind) {
  case Node::Kind::Ident:
    return CompileIdent(stmt);
  case Node::Kind::Int:
    return CompileIntLiteral(stmt);
  case Node::Kind::Float:
    return CompileFltLiteral(stmt);
  case Node::Kind::String:
    return CompileStringLiteral(stmt);
  case Node::Kind::Assign:
    return CompileAssign(stmt);
  case Node::Kind::True:
    return CompileTrue(stmt);
  case Node::Kind::False:
    return CompileFalse(stmt);
  case Node::Kind::Add:
  case Node::Kind::Sub:
  case Node::Kind::Div:
  case Node::Kind::Mult:
  case Node::Kind::LesserThan:
  case Node::Kind::LesserEqual:
  case Node::Kind::GreaterEqual:
  case Node::Kind::GreaterThan:
  case Node::Kind::Equal:
  case Node::Kind::NotEqual:
    return CompileBinOp(kind, stmt);
  case Node::Kind::IfFull:
    return CompileIfFull(stmt);
  case Node::Kind::IfSimple:
    return CompileIfSimple(stmt);
  case Node::Kind::Block:
    return CompileBlock(stmt);
  case Node::Kind::BuiltinCall:
    return CompileBuiltinCall(stmt);
  case Node::Kind::ReturnSimple:
    return CompileReturnSimple(stmt);
  default:
    DieLoudly("Unreachable at CompileExpression");
  }
}

ExprResult Compiler::CompileBuiltinCall(uint32_t call) {
  Node::Data data = ast_.Nodes().Datas()[call];
  Node::TokenIndex main_token = ast_.Nodes().MainTokens()[call];
  std::string_view name = SliceFromToken(main_token);
  Node::ExtraRange arg_range = std::get<Node::ExtraRange>(data);
  uint32_t argc = arg_range.end - arg_range.start;
  // We assume that the max u32 value will never be a valid
  // index into the builtins
  // that is 4,294,967,295 total functions
  uint32_t idx = LookupNative(name.substr(1, name.size())).value_or(~0);
  if (idx == 0xFFFFFFFF) {
    DieLoudly("Builtin function not found");
  }

  uint32_t base = next_reg_; // args start here
  for (size_t i = arg_range.start; i < arg_range.end; i++) {
    ExprResult arg = CompileExpression(ast_.ExtraData()[i]);
    ExprToNextReg(arg);
  }

  // TODO: Consider adding a Sema pass or something to try and catch problems
  // there instead of compile time
  // with a sema we can start making the language more strongly typed
  // and that opens up using pointers and stuff which could be pretty neat
  // in a scripting language
  Arity arity = kNatives[idx].arity;
  if (arity != Arity::Variadic && ToU32(arity) != argc) {
    DieLoudly("Mismatched arity at call site");
  }

  // CallB base, argc, idx
  block_.PushInstruction(Code::CreateABCk(Code::Op::CallB, base, argc, idx));
  FreeReg(base + 1);
  return ExprResult{ExprResult::Kind::Register, base};
}

ExprResult Compiler::CompileAssign(uint32_t stmt) {
  Node::Data data = ast_.Nodes().Datas()[stmt];
  Node::NodeAndNode assign = std::get<Node::NodeAndNode>(data);
  uint32_t ident_idx = ToU32(assign.first);

  if (ast_.Nodes().Kinds()[ident_idx] != Node::Kind::Ident) {
    // TODO: Push an error here later, for now just yell then die
    DieLoudly("TODO: Add error handling at compile time");
  }

  Node::TokenIndex ident_tok = ast_.Nodes().MainTokens()[ident_idx];
  StrPool::Index string_idx = strings_.InternOrGet(SliceFromToken(ident_tok));

  for (int i = static_cast<int>(locals_.size()) - 1; i >= 0; --i) {
    if (locals_[i].idx == string_idx) {
      DieLoudly("Cannot rebind variable, use `=` to mutate");
    }
  }

  ExprResult rhs = CompileExpression(ToU32(assign.second));
  uint32_t slot = ExprToNextReg(rhs);
  assert(slot == locals_.size());
  locals_.push_back({.idx = string_idx, .depth = depth_});
  return {ExprResult::Kind::Local, slot};
}

ExprResult Compiler::CompileIfFull(uint32_t stmt) {
  Node::Data data = ast_.Nodes().Datas()[stmt];
  Node::NodeAndExtra if_node = std::get<Node::NodeAndExtra>(data);

  uint32_t extra = ToU32(if_node.extra);
  uint32_t then_expr = ast_.ExtraData()[extra];
  uint32_t else_expr = ast_.ExtraData()[extra + 1];

  uint32_t base = next_reg_;

  ExprResult cond = CompileExpression(ToU32(if_node.node));
  uint32_t rc = ExprToAnyReg(cond);
  block_.PushInstruction(Code::CreateABCk(Code::Op::Test, rc, 0, 0, 0));
  uint32_t jmp_to_else = EmitJump();

  // Only one jump is possible so the branches can share registers
  // The Inst needs to Move to the same location for both (ie the base) so the
  // VM knows where the if-expression's value lives
  FreeReg(base);
  DischargeToReg(CompileExpression(then_expr), base);
  uint32_t jmp_to_end = EmitJump();
  PatchJumpToHere(jmp_to_else);

  // See comment above ^
  FreeReg(base);
  DischargeToReg(CompileExpression(else_expr), base);
  PatchJumpToHere(jmp_to_end);

  FreeReg(base + 1);
  return ExprResult{ExprResult::Kind::Register, base};
}

ExprResult Compiler::CompileIfSimple(uint32_t stmt) {
  Node::Data data = ast_.Nodes().Datas()[stmt];
  Node::NodeAndNode if_node = std::get<Node::NodeAndNode>(data);

  uint32_t base = next_reg_;

  ExprResult cond = CompileExpression(ToU32(if_node.first));
  uint32_t rc = ExprToAnyReg(cond);
  block_.PushInstruction(Code::CreateABCk(Code::Op::Test, rc, 0, 0, 0));
  uint32_t jmp_to_else = EmitJump();

  FreeReg(base);
  DischargeToReg(CompileExpression(ToU32(if_node.second)), base);
  uint32_t jmp_to_end = EmitJump();
  PatchJumpToHere(jmp_to_else);

  // The else branch needs a synthetic `Void` branch
  FreeReg(base);
  DischargeToReg(EmitLoadK(Object::mkVoid({})), base);
  PatchJumpToHere(jmp_to_end);

  FreeReg(base + 1);
  return ExprResult{ExprResult::Kind::Register, base};
}

ExprResult Compiler::CompileReturnSimple(uint32_t stmt) {
  Node::Data ret = ast_.Nodes().Datas()[stmt];
  Node::Index expr = std::get<Node::Index>(ret);
  ExprResult v = CompileExpression(ToU32(expr));
  // Lua convention: B = nresults + 1, so B=2 returns the one value in R[A]
  // B = 0 -> dynamic result
  // B = 1 -> returns 0 values
  // B = 2 -> returns one value, in R[A]
  block_.PushInstruction(Code::CreateABCk(Code::Op::Ret, v.idx, 2, 0));
  return ExprResult{ExprResult::Kind::Register, v.idx};
}

ExprResult Compiler::CompileBlock(uint32_t stmt) {
  // Blocks are expressions so the last value needs to leave it's
  // value on the stack
  uint32_t base = next_reg_;
  EnterScope();
  Node::Data data = ast_.Nodes().Datas()[stmt];
  Node::ExtraRange range = std::get<Node::ExtraRange>(data);
  ExprResult result = CompileExpressions(range);
  DischargeToReg(result, base);
  ExitScope();
  next_reg_ = base + 1; // the next register is after this blocks value
  return ExprResult{ExprResult::Kind::Register, base};
}

// clang-format off
constexpr Code::Op MapNodeToOp(Node::Kind kind) {
  switch (kind) {
  case Node::Kind::Add:             return Code::Op::Add;
  case Node::Kind::Sub:             return Code::Op::Sub;
  case Node::Kind::Div:             return Code::Op::Div;
  case Node::Kind::Mult:            return Code::Op::Mult;
  case Node::Kind::LesserEqual:     return Code::Op::Le;
  case Node::Kind::LesserThan:      return Code::Op::Lt;
  case Node::Kind::GreaterEqual:    return Code::Op::Ge;
  case Node::Kind::GreaterThan:     return Code::Op::Gt;
  case Node::Kind::Equal:           return Code::Op::Eq;
  case Node::Kind::NotEqual:        return Code::Op::Ne;
  case Node::Kind::BitAnd:          return Code::Op::BAnd;
  case Node::Kind::BitNot:          return Code::Op::BNot;
  case Node::Kind::BitOr:           return Code::Op::BOr;
  case Node::Kind::BitXor:          return Code::Op::BXor;
  default:
    DieLoudly("Unreachable at CompileBinOp Mapping");
  }
}
// clang-format on

ExprResult Compiler::CompileBinOp(Node::Kind kind, uint32_t expr) {
  Node::Data data = ast_.Nodes().Datas()[expr];
  Node::NodeAndNode bin_op = std::get<Node::NodeAndNode>(data);
  ExprResult left = CompileExpression(ToU32(bin_op.first));
  uint32_t rl = ExprToAnyReg(left);
  ExprResult right = CompileExpression(ToU32(bin_op.second));
  uint32_t rr = ExprToAnyReg(right);

  FreeExpr(right); // top temp first
  FreeExpr(left);
  uint32_t dst = AllocateReg();

  Code::Inst inst = Code::CreateABCk(MapNodeToOp(kind), dst, rl, rr);
  block_.PushInstruction(inst);

  return ExprResult{ExprResult::Kind::Register, dst};
}

ExprResult Compiler::CompileIdent(uint32_t stmt) {
  Node::TokenIndex tok = ast_.Nodes().MainTokens()[stmt];
  std::optional<StrPool::Index> string_idx = strings_.Get(SliceFromToken(tok));

  if (!string_idx.has_value()) DieLoudly("Undefined variable");

  for (int i = static_cast<int>(locals_.size()) - 1; i >= 0; --i) {
    if (locals_[i].idx == string_idx.value()) {
      // locals_[i] lives in register i
      return ExprResult{ExprResult::Kind::Local, static_cast<uint32_t>(i)};
    }
  }
  DieLoudly("Undefined variable"); // TODO: real error reporting
}

ExprResult Compiler::CompileFltLiteral(uint32_t lit) {
  Node::Data data = ast_.Nodes().Datas()[lit];
  auto token = ast_.Nodes().MainTokens()[lit];
  return EmitLoadK(Object::mkFloat(FloatFromToken(token)));
}

ExprResult Compiler::CompileIntLiteral(uint32_t lit) {
  Node::Data data = ast_.Nodes().Datas()[lit];
  auto token = ast_.Nodes().MainTokens()[lit];
  return EmitLoadK(Object::mkInt(IntFromToken(token)));
}

ExprResult Compiler::CompileStringLiteral(uint32_t lit) {
  Node::Data data = ast_.Nodes().Datas()[lit];
  auto token = ast_.Nodes().MainTokens()[lit];
  auto start = ast_.Tokens().Starts()[ToU32(token)];
  auto len = ast_.Tokens().Lens()[ToU32(token)];
  // the quotes need stripped
  std::string_view slice = source_.substr(start + 1, len - 2);
  return EmitLoadK(Object::mkString(str_table_.Intern(slice)));
}

ExprResult Compiler::CompileTrue(uint32_t stmt) {
  return EmitLoadK(Object::mkBool(true));
}

ExprResult Compiler::CompileFalse(uint32_t stmt) {
  return EmitLoadK(Object::mkBool(false));
}

ExprResult Compiler::EmitLoadK(Object::Value v) {
  block_.PushConstant(v);

  uint32_t const_index = block_.ConstantsSize() - 1;

  return ExprResult{ExprResult::Kind::Constant, const_index};
}

std::string_view Compiler::SliceFromToken(Node::TokenIndex token) {
  auto start = ast_.Tokens().Starts()[ToU32(token)];
  auto len = ast_.Tokens().Lens()[ToU32(token)];
  return source_.substr(start, len);
}

uint32_t Compiler::AllocateReg() { return next_reg_++; }

void Compiler::FreeReg(uint32_t keep) { next_reg_ = keep; }

void Compiler::FreeExpr(ExprResult e) {
  if (e.kind == ExprResult::Kind::Register && e.idx >= locals_.size()) {
    assert(e.idx == next_reg_ - 1);
    next_reg_--;
  }
}

void Compiler::DischargeToReg(ExprResult e, uint32_t reg) {
  switch (e.kind) {
  // If a register does not match the expression ident
  // it needs to be moved to the correct one
  case ExprResult::Kind::Local:
  case ExprResult::Kind::Register:
    if (e.idx != reg)
      block_.PushInstruction(Code::CreateABCk(Code::Op::Move, reg, e.idx, 0));
    break;
  // constants get pushed irregardles
  case ExprResult::Kind::Constant:
    block_.PushInstruction(Code::CreateABx(Code::Op::LoadK, reg, e.idx));
    break;
  // Jmps need to be patched
  case ExprResult::Kind::Jmp:
    DieLoudly("unimplemented");
  }
}

// Results need to be modifed in order to follow the same Lua
// semantics
uint32_t Compiler::ExprToNextReg(ExprResult &e) {
  FreeExpr(e);
  uint32_t reg = AllocateReg(); // new tmp register at the top
  DischargeToReg(e, reg);
  e = {ExprResult::Kind::Register, reg};
  return reg;
}

uint32_t Compiler::ExprToAnyReg(ExprResult &e) {
  if (e.kind == ExprResult::Kind::Local || e.kind == ExprResult::Kind::Register)
    return e.idx;
  return ExprToNextReg(e);
}

uint32_t Compiler::EmitJump() {
  block_.PushInstruction(Code::CreatesJ(Code::Op::Jmp, 0));
  return block_.OpcodesSize() - 1;
}

void Compiler::PatchJumpToHere(uint32_t jump_idx) {
  // Offsets are signed
  int32_t offset = static_cast<int32_t>(block_.OpcodesSize() - (jump_idx + 1));
  Code::SetsJ(block_.InstAt(jump_idx), offset);
}

void Compiler::EnterScope() { depth_++; }

void Compiler::ExitScope() {
  depth_--;
  while (!locals_.empty() && locals_.back().depth > depth_) {
    locals_.pop_back();
  }
  next_reg_ = static_cast<uint32_t>(locals_.size());
}

int64_t Compiler::IntFromToken(Node::TokenIndex token) {
  int64_t value = 0;
  std::string_view slice = SliceFromToken(token);
  const auto [ptr, ec] =
      std::from_chars(slice.data(), slice.data() + slice.size(), value);

  // TODO: Better error handling
  // once compiler errors are reported replace these
  if (ec == std::errc::invalid_argument) { // this one should be impossible
    DieLoudly("Error: Not a valid number.");
  }

  if (ec == std::errc::result_out_of_range) { // this one can happen
    DieLoudly("Error: Number is too large for the type.");
  }

  if (ptr != slice.data() + slice.size()) { // also should also be impossible
    DieLoudly("Error: Trailing characters found in parsed string");
  }

  return value;
}

double Compiler::FloatFromToken(Node::TokenIndex token) {
  // TODO: macOS only supports std::from_chars for floats in newer versions so
  // we use this for now. we can decide to just ditch older versions in the
  // future since this is just a hobby project
  std::string_view slice = SliceFromToken(token);

  // strtod needs a null terminated string
  // the heap allocation is restrained to this function
  std::string temp = std::string{slice};
  char *endptr;
  errno = 0;

  double value = std::strtod(temp.data(), &endptr);

  // TODO: Better error handling
  // once compiler errors are reported replace this
  if (errno == ERANGE) {
    DieLoudly("The value was too large or too small for a double.");
  }

  return value;
}

std::optional<uint32_t> Compiler::LookupNative(std::string_view name) {
  // O(N) lookup per number of builtins
  // maybe a different data structure could be used here when the number
  // grows
  for (size_t i = 0; i < std::size(kNatives); i++) {
    if (name == kNatives[i].name) return i;
  }
  return std::nullopt;
}
