#include "compiler/compiler.hpp"
#include "ast/node.hpp"
#include "compiler/bytecode.hpp"
#include "strpool/strpool.hpp"
#include "utils/die.hpp"
#include "vm/object.hpp"
#include <cassert>
#include <cerrno>
#include <charconv>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <string>
#include <string_view>
#include <variant>

// TODO: Find a way to intern constants
// ideally we can store the object's register index so it can be reused

using namespace Anvil;

Compiler::Compiler(std::string_view source, Ast ast)
    : source_{source}, ast_{ast} {}

Block Compiler::Compile() {
  CompileRoot();
  return block_;
}

void Compiler::CompileRoot() {
  auto root = ast_.Nodes().Datas()[0];
  auto range = std::get<Node::ExtraRange>(root);
  uint32_t v = CompileExpressions(range);
  // Lua convention: B = nresults + 1, so B=2 returns the one value in R[A]
  // B = 0 -> dynamic result
  // B = 1 -> returns 0 values
  // B = 2 -> returns one value, in R[A]
  block_.PushInstruction(Code::CreateABCk(Code::Op::Ret, v, 2, 0));
}

uint32_t Compiler::CompileExpressions(Node::ExtraRange range) {
  auto children =
      ast_.ExtraData().subspan(range.start, range.end - range.start);

  if (children.empty()) {
    return EmitLoadK(Object::mkVoid({}));
  }

  for (size_t i = 0; i < children.size() - 1; i++) {
    CompileExpression(children[i]);
  }

  return CompileExpression(children[children.size() - 1]);
}

uint32_t Compiler::CompileExpression(uint32_t stmt) {
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
  case Node::Kind::ReturnSimple:
    return CompileReturnSimple(stmt);
  default:
    DieLoudly{"Unreachable at CompileExpression"}();
  }
}

uint32_t Compiler::CompileAssign(uint32_t stmt) {
  Node::Data data = ast_.Nodes().Datas()[stmt];
  Node::NodeAndNode assign = std::get<Node::NodeAndNode>(data);
  uint32_t ident_idx = ToU32(assign.first);

  if (ast_.Nodes().Kinds()[ident_idx] != Node::Kind::Ident) {
    // TODO: Push an error here later, for now just yell then die
    DieLoudly{"TODO: Add error handling at compile time"}();
  }

  Node::TokenIndex ident_tok = ast_.Nodes().MainTokens()[ident_idx];
  StrPool::Index string_idx = strings_.InternOrGet(SliceFromToken(ident_tok));

  for (int i = static_cast<int>(locals_.size()) - 1; i >= 0; --i) {
    if (locals_[i].idx == string_idx) {
      DieLoudly{"Cannot rebind variable, use `=` to mutate"}();
    }
  }

  uint32_t target_reg = next_reg_;
  uint32_t reg_rhs = CompileExpression(ToU32(assign.second));

  if (reg_rhs != target_reg) {
    block_.PushInstruction(
        Code::CreateABCk(Code::Op::Move, target_reg, reg_rhs, 0));
  }

  FreeReg(target_reg + 1);
  locals_.push_back({.idx = string_idx, .depth = depth_});

  return target_reg;
}

uint32_t Compiler::CompileIfFull(uint32_t stmt) {
  Node::Data data = ast_.Nodes().Datas()[stmt];
  Node::NodeAndExtra if_node = std::get<Node::NodeAndExtra>(data);

  uint32_t extra = ToU32(if_node.extra);
  uint32_t then_expr = ast_.ExtraData()[extra];
  uint32_t else_expr = ast_.ExtraData()[extra + 1];

  uint32_t base = next_reg_;

  uint32_t reg_cond = CompileExpression(ToU32(if_node.node));
  block_.PushInstruction(Code::CreateABCk(Code::Op::Test, reg_cond, 0, 0, 0));
  uint32_t jmp_to_else = EmitJump();

  // Only one jump is possible so the branches can share registers
  // The Inst needs to Move to the same location for both (ie the base) so the
  // VM knows where the if-expression's value lives
  FreeReg(base);
  uint32_t reg_then = CompileExpression(then_expr);
  block_.PushInstruction(Code::CreateABCk(Code::Op::Move, base, reg_then, 0));
  uint32_t jmp_to_end = EmitJump();
  PatchJumpToHere(jmp_to_else);

  // See comment above ^
  FreeReg(base);
  uint32_t reg_else = CompileExpression(else_expr);
  block_.PushInstruction(Code::CreateABCk(Code::Op::Move, base, reg_else, 0));
  PatchJumpToHere(jmp_to_end);

  FreeReg(base + 1);
  return base;
}

uint32_t Compiler::CompileIfSimple(uint32_t stmt) {
  Node::Data data = ast_.Nodes().Datas()[stmt];
  Node::NodeAndNode if_node = std::get<Node::NodeAndNode>(data);

  uint32_t base = next_reg_;

  uint32_t reg_cond = CompileExpression(ToU32(if_node.first));
  block_.PushInstruction(Code::CreateABCk(Code::Op::Test, reg_cond, 0, 0, 0));
  uint32_t jmp_to_else = EmitJump();

  FreeReg(base);
  uint32_t reg_then = CompileExpression(ToU32(if_node.second));
  block_.PushInstruction(Code::CreateABCk(Code::Op::Move, base, reg_then, 0));
  uint32_t jmp_to_end = EmitJump();
  PatchJumpToHere(jmp_to_else);

  // The else branch needs a synthetic `Void` branch
  FreeReg(base);
  EmitLoadK(Object::mkVoid({}));
  PatchJumpToHere(jmp_to_end);

  FreeReg(base + 1);
  return base;
}

uint32_t Compiler::CompileReturnSimple(uint32_t stmt) {
  Node::Data ret = ast_.Nodes().Datas()[stmt];
  Node::Index expr = std::get<Node::Index>(ret);
  uint32_t v = CompileExpression(ToU32(expr));
  // Lua convention: B = nresults + 1, so B=2 returns the one value in R[A]
  // B = 0 -> dynamic result
  // B = 1 -> returns 0 values
  // B = 2 -> returns one value, in R[A]
  block_.PushInstruction(Code::CreateABCk(Code::Op::Ret, v, 2, 0));
  return v;
}

uint32_t Compiler::CompileBlock(uint32_t stmt) {
  // Blocks are expressions so the last value needs to leave it's
  // value on the stack
  uint32_t base = next_reg_;
  EnterScope();
  Node::Data data = ast_.Nodes().Datas()[stmt];
  Node::ExtraRange range = std::get<Node::ExtraRange>(data);
  uint32_t result = CompileExpressions(range);
  if (result != base) {
    block_.PushInstruction(Code::CreateABCk(Code::Op::Move, base, result, 0));
  }
  ExitScope();
  next_reg_ = base + 1; // the next register is after this blocks value
  return base;
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
  default:
    DieLoudly{"Unreachable at CompileBinOp Mapping"}();
  }
}
// clang-format on

uint32_t Compiler::CompileBinOp(Node::Kind kind, uint32_t expr) {
  Node::Data data = ast_.Nodes().Datas()[expr];
  Node::NodeAndNode bin_op = std::get<Node::NodeAndNode>(data);
  uint32_t reg_left = CompileExpression(ToU32(bin_op.first));
  uint32_t reg_right = CompileExpression(ToU32(bin_op.second));

  uint32_t reg_result = AllocateReg();

  Code::Inst inst =
      Code::CreateABCk(MapNodeToOp(kind), reg_result, reg_left, reg_right);
  block_.PushInstruction(inst);

  return reg_result;
}

uint32_t Compiler::CompileIdent(uint32_t stmt) {
  Node::TokenIndex tok = ast_.Nodes().MainTokens()[stmt];
  std::optional<StrPool::Index> string_idx = strings_.Get(SliceFromToken(tok));

  if (!string_idx.has_value()) DieLoudly{"Undefined variable"}();

  for (int i = static_cast<int>(locals_.size()) - 1; i >= 0; --i) {
    if (locals_[i].idx == string_idx.value()) {
      return static_cast<uint32_t>(i); // locals_[i] lives in register i
    }
  }
  DieLoudly{"Undefined variable"}(); // TODO: real error reporting
}

uint32_t Compiler::CompileFltLiteral(uint32_t lit) {
  Node::Data data = ast_.Nodes().Datas()[lit];
  auto token = ast_.Nodes().MainTokens()[lit];
  return EmitLoadK(Object::mkFloat(FloatFromToken(token)));
}

uint32_t Compiler::CompileIntLiteral(uint32_t lit) {
  Node::Data data = ast_.Nodes().Datas()[lit];
  auto token = ast_.Nodes().MainTokens()[lit];
  return EmitLoadK(Object::mkInt(IntFromToken(token)));
}

uint32_t Compiler::CompileStringLiteral(uint32_t lit) {
  Node::Data data = ast_.Nodes().Datas()[lit];
  auto token = ast_.Nodes().MainTokens()[lit];
  auto start = ast_.Tokens().Starts()[ToU32(token)];
  auto len = ast_.Tokens().Lens()[ToU32(token)];
  // the quotes need stripped
  return EmitLoadK(Object::mkString(source_.substr(start + 1, len - 2)));
}

uint32_t Compiler::CompileTrue(uint32_t stmt) {
  return EmitLoadK(Object::mkBool(true));
}

uint32_t Compiler::CompileFalse(uint32_t stmt) {
  return EmitLoadK(Object::mkBool(false));
}

uint32_t Compiler::EmitLoadK(Object::Value v) {
  block_.PushConstant(v);

  uint32_t const_index = block_.ConstantsSize() - 1;
  uint32_t target_reg = AllocateReg();

  Code::Inst inst = Code::CreateABx(Code::Op::LoadK, target_reg, const_index);
  block_.PushInstruction(inst);

  return target_reg;
}

std::string_view Compiler::SliceFromToken(Node::TokenIndex token) {
  auto start = ast_.Tokens().Starts()[ToU32(token)];
  auto len = ast_.Tokens().Lens()[ToU32(token)];
  return source_.substr(start, len);
}

uint32_t Compiler::AllocateReg() { return next_reg_++; }

void Compiler::FreeReg(uint32_t keep) { next_reg_ = keep; }

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
    DieLoudly{"Error: Not a valid number."}();
  }

  if (ec == std::errc::result_out_of_range) { // this one can happen
    DieLoudly{"Error: Number is too large for the type."}();
  }

  if (ptr != slice.data() + slice.size()) { // also should also be impossible
    DieLoudly{"Error: Trailing characters found in parsed string"}();
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
    DieLoudly{"The value was too large or too small for a double."}();
  }

  return value;
}
