#ifndef VM_HPP_
#define VM_HPP_

#include "compiler/block.hpp"
#include "compiler/bytecode.hpp"
#include "strtable/strtable.hpp"
#include "vm/object.hpp"
#include <array>
#include <cstdint>

namespace Anvil {

struct Frame {
  uint32_t return_pc;
  uint32_t bp;         // regs_[bp + n]
  uint32_t result_reg; // caller register that receives the return value
  const Proto *proto;
};

class VM {
  uint32_t pc_ = 0;
  uint32_t bp_ = 0;
  Module module_;
  std::array<Object::Value, Code::kMaxRegs + 1>
      regs_; // 4kb fixed-width for now
  std::vector<Object::Value> globals_;
  StringTable &str_table_;
  std::vector<Frame> frames_;

public:
  VM(Module module, StringTable &str_table);
  void Run();
  Object::Value MockRun();
};

} // namespace Anvil

#endif
