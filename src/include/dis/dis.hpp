#ifndef DIS_HPP_
#define DIS_HPP_

#include "compiler/block.hpp"
#include "compiler/bytecode.hpp"

namespace Dis {
std::string Disassemble(const Block &block);
}

#endif
