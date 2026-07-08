#include "compiler/compiler.hpp"
#include "ast/node.hpp"
#include "compiler/bytecode.hpp"
#include "vm/object.hpp"
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>

Compiler::Compiler(std::string_view source, Ast ast)
    : source_{source}, ast_{ast} {}

Block Compiler::Compile() {
  CompileRoot();
  return block_;
}

void Compiler::CompileRoot() {
  auto root = ast_.Nodes().Datas()[0];
  auto range = std::get<Node::ExtraRange>(root);
  uint32_t v = CompileStatements(range);
  // Lua convention: B = nresults + 1, so B=2 returns the one value in R[A]
  // B = 0 -> dynamic result
  // B = 1 -> returns 0 values
  // B = 2 -> returns one value, in R[A]
  block_.PushInstruction(Code::CreateABCk(Code::Op::Ret, v, 2, 0));
}

uint32_t Compiler::CompileStatements(Node::ExtraRange range) {
  auto children =
      ast_.ExtraData().subspan(range.start, range.end - range.start);

  if (children.empty()) return 0;

  for (size_t i = 0; i < children.size() - 1; i++) {
    CompileStatement(children[i]);
  }

  return CompileStatement(children[children.size() - 1]);
}

// TODO: This is a bit of a misnomer, most of these are not statements so either
// change the name or refactor into separate functions
uint32_t Compiler::CompileStatement(uint32_t stmt) {
  Node::Kind kind = ast_.Nodes().Kinds()[stmt];

  switch (kind) {
  case Node::Kind::Int:
    return CompileIntLiteral(stmt);
  case Node::Kind::Float:
    return CompileFltLiteral(stmt);
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
  case Node::Kind::Block:
    return CompileBlock(stmt);
  default:
    assert(0 && "Unreachable at CompileStatement");
    break;
  }
}

uint32_t Compiler::CompileIfFull(uint32_t stmt) {
  Node::Data data = ast_.Nodes().Datas()[stmt];
  Node::NodeAndExtra if_node = std::get<Node::NodeAndExtra>(data);

  uint32_t extra = ToU32(if_node.extra);
  uint32_t then_expr = ast_.ExtraData()[extra];
  uint32_t else_expr = ast_.ExtraData()[extra + 1];

  uint32_t base = next_reg_;

  uint32_t reg_cond = CompileStatement(ToU32(if_node.node));
  block_.PushInstruction(Code::CreateABCk(Code::Op::Test, reg_cond, 0, 0, 0));
  uint32_t jmp_to_else = EmitJump();

  // Only one jump is possible so the branches can share registers
  // The Inst needs to Move to the same location for both (ie the base) so the
  // VM knows where the if-expression's value lives
  FreeReg(base);
  uint32_t reg_then = CompileStatement(then_expr);
  block_.PushInstruction(Code::CreateABCk(Code::Op::Move, base, reg_then, 0));
  uint32_t jmp_to_end = EmitJump();
  PatchJumpToHere(jmp_to_else);

  // See comment above ^
  FreeReg(base);
  uint32_t reg_else = CompileStatement(else_expr);
  block_.PushInstruction(Code::CreateABCk(Code::Op::Move, base, reg_else, 0));
  PatchJumpToHere(jmp_to_end);

  FreeReg(base + 1);
  return base;
}

uint32_t Compiler::CompileBlock(uint32_t stmt) {
  Node::Data data = ast_.Nodes().Datas()[stmt];
  Node::ExtraRange range = std::get<Node::ExtraRange>(data);
  return CompileStatements(range);
}

uint32_t Compiler::CompileBinOp(Node::Kind kind, uint32_t expr) {
  Node::Data data = ast_.Nodes().Datas()[expr];
  Node::NodeAndNode bin_op = std::get<Node::NodeAndNode>(data);
  uint32_t reg_left = CompileStatement(ToU32(bin_op.first));
  uint32_t reg_right = CompileStatement(ToU32(bin_op.second));

  uint32_t reg_result = AllocateReg();

  Code::Inst inst;

  // TODO: Fix this up a bit
  // This is incredibly dumb, either make singleton functions that can handle
  // this in a slicker manner or maybe a function table
  switch (kind) {
  case Node::Kind::Add:
    inst = Code::CreateABCk(Code::Op::Add, reg_result, reg_left, reg_right);
    break;
  case Node::Kind::Sub:
    inst = Code::CreateABCk(Code::Op::Sub, reg_result, reg_left, reg_right);
    break;
  case Node::Kind::Div:
    inst = Code::CreateABCk(Code::Op::Div, reg_result, reg_left, reg_right);
    break;
  case Node::Kind::Mult:
    inst = Code::CreateABCk(Code::Op::Mult, reg_result, reg_left, reg_right);
    break;
  case Node::Kind::LesserEqual:
    inst = Code::CreateABCk(Code::Op::Le, reg_result, reg_left, reg_right);
    break;
  case Node::Kind::LesserThan:
    inst = Code::CreateABCk(Code::Op::Lt, reg_result, reg_left, reg_right);
    break;
  case Node::Kind::GreaterEqual:
    inst = Code::CreateABCk(Code::Op::Ge, reg_result, reg_left, reg_right);
    break;
  case Node::Kind::GreaterThan:
    inst = Code::CreateABCk(Code::Op::Gt, reg_result, reg_left, reg_right);
    break;
  case Node::Kind::Equal:
    inst = Code::CreateABCk(Code::Op::Eq, reg_result, reg_left, reg_right);
    break;
  case Node::Kind::NotEqual:
    inst = Code::CreateABCk(Code::Op::Ne, reg_result, reg_left, reg_right);
    break;
  default:
    assert(0 && "Unreachable at CompileBinOp");
    break;
  }
  block_.PushInstruction(inst);

  return reg_result;
}

uint32_t Compiler::CompileFltLiteral(uint32_t lit) {
  Node::Data data = ast_.Nodes().Datas()[lit];
  auto token = ast_.Nodes().MainTokens()[lit];
  block_.PushConstant(Object::mkFloat(std::stod(SliceFromToken(token))));

  uint32_t const_index = block_.ConstantsSize() - 1;
  uint32_t target_reg = AllocateReg();

  Code::Inst inst = Code::CreateABx(Code::Op::LoadK, target_reg, const_index);
  block_.PushInstruction(inst);

  return target_reg;
}

uint32_t Compiler::CompileIntLiteral(uint32_t lit) {
  Node::Data data = ast_.Nodes().Datas()[lit];
  auto token = ast_.Nodes().MainTokens()[lit];
  block_.PushConstant(Object::mkInt(std::stoi(SliceFromToken(token))));

  uint32_t const_index = block_.ConstantsSize() - 1;
  uint32_t target_reg = AllocateReg();

  Code::Inst inst = Code::CreateABx(Code::Op::LoadK, target_reg, const_index);
  block_.PushInstruction(inst);

  return target_reg;
}

std::string Compiler::SliceFromToken(Node::TokenIndex token) {
  auto start = ast_.Tokens().Starts()[ToU32(token)];
  auto len = ast_.Tokens().Lens()[ToU32(token)];
  // performs a heap allocation which is slightly wasteful
  // the object is small enough so maybe std::from_chars would be better
  return std::string{source_.substr(start, len)};
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
