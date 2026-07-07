#include "compiler/compiler.hpp"
#include "ast/node.hpp"
#include "compiler/bytecode.hpp"
#include "vm/object.hpp"
#include <cassert>
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
    return CompileBinOp(kind, stmt);
  default:
    assert(0 && "Unreachable at CompileStatement");
    break;
  }
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
  default:
    assert(0 && "Unreachable at CompileBinOp");
    break;
  }
  block_.PushInstruction(inst);

  FreeRegs(reg_result + 1);

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

void Compiler::FreeRegs(uint32_t keep) { next_reg_ = keep; }
