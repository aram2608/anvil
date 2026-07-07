#include "vm/vm.hpp"
#include "compiler/bytecode.hpp"
#include "vm/object.hpp"
#include <cassert>
#include <functional>

namespace {

// TODO: Ponder on this more
// Implicit promotion, perhaps we can error instead not sure
template <typename Fn>
inline Object::Value Arith(const Object::Value &a, const Object::Value &b,
                           Fn op) {
  if (Object::isInt(a) && Object::isInt(b)) {
    return Object::mkInt(op(a.as.i, b.as.i));
  } else if (Object::isFloat(a) && Object::isFloat(b)) {
    return Object::mkFloat(op(a.as.f, b.as.f));
  } else if (Object::isFloat(a) && Object::isInt(b)) {
    return Object::mkFloat(op(a.as.f, b.as.i));
  } else if (Object::isInt(a) && Object::isFloat(b)) {
    return Object::mkFloat(op(a.as.i, b.as.f));
  }
  assert(0 && "Unreachable at Arith");
}

} // namespace

VM::VM(Block block) : block_{block} {}

// Temporary function before printing can happen
// returns the last value from the Ret instruction
Object::Value VM::MockRun() {
  auto code = block_.get_code();
  for (;;) {
    Code::Inst i = code[pc_++];

    Code::Op op = Code::GetOp(i);

    switch (op) {
    case Code::Op::Move:
      regs_[Code::GetA(i)] = regs_[Code::GetB(i)];
      break;
    case Code::Op::LoadK:
      regs_[Code::GetA(i)] = block_.get_constants()[Code::GetBx(i)];
      break;
    case Code::Op::Add:
      regs_[Code::GetA(i)] =
          Arith(regs_[Code::GetB(i)], regs_[Code::GetC(i)], std::plus<>{});
      break;
    case Code::Op::Sub:
      regs_[Code::GetA(i)] =
          Arith(regs_[Code::GetB(i)], regs_[Code::GetC(i)], std::minus<>{});
      break;
    case Code::Op::Mult:
      regs_[Code::GetA(i)] = Arith(regs_[Code::GetB(i)], regs_[Code::GetC(i)],
                                   std::multiplies<>{});
      break;
    case Code::Op::Div: {
      Object::Value b = regs_[Code::GetC(i)];
      if (Object::isZero(b)) return;
      regs_[Code::GetA(i)] =
          Arith(regs_[Code::GetB(i)], regs_[Code::GetC(i)], std::divides<>{});
    } break;
    case Code::Op::Ret:
      return regs_[Code::GetA(i)];
    }
  }
}

void VM::Run() { MockRun(); }
