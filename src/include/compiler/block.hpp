#ifndef BLOCK_HPP_
#define BLOCK_HPP_

#include "compiler/bytecode.hpp"
#include "vm/object.hpp"
#include <cstdint>
#include <span>
#include <vector>

class Block {
  std::vector<Object::Value> k_; // Constants
  std::vector<Code::Inst> code_; // Opcodes
  std::vector<int> line_info_;

public:
  void PushConstant(Object::Value value) { k_.push_back(value); }
  void PushInstruction(Code::Inst inst) { code_.push_back(inst); }
  uint32_t ConstantsSize() const { return k_.size(); }
  uint32_t OpcodesSize() const { return code_.size(); }
  std::span<const Code::Inst> get_code() const { return code_; }
  std::span<const Object::Value> get_constants() const { return k_; }
  std::span<const int> get_lines() const { return line_info_; }
};

#endif
