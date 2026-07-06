#include "dis/dis.hpp"
#include "compiler/bytecode.hpp"
#include <format>

std::string Dis::Disassemble(const Block &block) {
  std::string out;
  auto code = block.get_code();

  for (size_t pc = 0; pc < code.size(); pc++) {
    Code::Inst i = code[pc];
    Code::Op op = Code::GetOp(i);

    std::format_to(std::back_inserter(out), "{:4}  {:<8}", pc,
                   Code::GetOpName(op));

    switch (Code::GetMode(op)) {
    case Code::Mode::iABC:
      std::format_to(std::back_inserter(out), "{:3} {:3} {:3}", Code::GetA(i),
                     Code::GetB(i), Code::GetC(i));
      if (Code::TestK(i)) out += " k";
      break;
    case Code::Mode::iABx:
      std::format_to(std::back_inserter(out), "{:3} {:3}", Code::GetA(i),
                     Code::GetBx(i));
      break;
    case Code::Mode::ivABC:
      std::format_to(std::back_inserter(out), "{:3} {:3} {:3}", Code::GetA(i),
                     Code::GetvB(i), Code::GetvC(i));
      break;
    case Code::Mode::iAsBx:
      std::format_to(std::back_inserter(out), "{:3} {:3}", Code::GetA(i),
                     Code::GetsBx(i));
      break;
    case Code::Mode::iAx:
      std::format_to(std::back_inserter(out), "{:3}", Code::GetAx(i));
      break;
    case Code::Mode::isJ:
      std::format_to(std::back_inserter(out), "{:3}", Code::GetsJ(i));
      break;
    }

    out += '\n';
  }
  return out;
}
