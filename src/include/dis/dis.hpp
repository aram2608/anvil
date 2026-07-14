#ifndef DIS_HPP_
#define DIS_HPP_

#include "compiler/block.hpp"

namespace Anvil {

namespace Dis {
void DisassembleModule(Module &mod);
std::string Disassemble(const Block &block);
} // namespace Dis

} // namespace Anvil

#endif
