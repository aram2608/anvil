#ifndef VM_HPP_
#define VM_HPP_

#include "compiler/block.hpp"
#include "compiler/bytecode.hpp"
#include "strtable/strtable.hpp"
#include "vm/object.hpp"
#include <array>
#include <cstdint>

namespace Anvil {

class VM {
  uint32_t pc_ = 0;
  Block block_;
  std::array<Object::Value, Code::kMaxRegs + 1>
      regs_; // 4kb fixed-width for now
  StringTable &str_table_;

public:
  VM(Block block, StringTable &str_table);
  void Run();
  Object::Value MockRun();
};

} // namespace Anvil

#endif
