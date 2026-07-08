#include "vm/vm.hpp"
#include "compiler/bytecode.hpp"
#include "vm/object.hpp"
#include <cassert>
#include <functional>
#include <iostream>
#include <stdexcept>

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

template <typename Fn>
inline Object::Value Comp(const Object::Value &a, const Object::Value &b,
                          Fn op) {
  if (Object::isInt(a) && Object::isInt(b)) {
    return Object::mkBool(op(a.as.i, b.as.i));
  } else if (Object::isFloat(a) && Object::isFloat(b)) {
    return Object::mkBool(op(a.as.f, b.as.f));
  } else if (Object::isFloat(a) && Object::isInt(b)) {
    return Object::mkBool(op(a.as.f, b.as.i));
  } else if (Object::isInt(a) && Object::isFloat(b)) {
    return Object::mkBool(op(a.as.i, b.as.f));
  }
  assert(0 && "Unreachable at Arith");
}

inline bool isTruthy(const Object::Value &x) {
  if (Object::isInt(x)) {
    return x.as.i != 0;
  } else if (Object::isFloat(x)) {
    return x.as.f != 0;
  } else if (Object::isBool(x)) {
    return x.as.b;
  }
  assert(0 && "Unreachable at isTruthy");
}

class RunTimeError : public std::runtime_error {
public:
  explicit RunTimeError(std::string message)
      : std::runtime_error{message.c_str()} {}
};

} // namespace

VM::VM(Block block) : block_{block} {}

// Temporary function before printing can happen
// returns the last value from the Ret instruction
// assumes no errors will happen during excecution
Object::Value VM::MockRun() {
  auto code = block_.get_code();
  for (;;) {
    Code::Inst i = code[pc_++];

    Code::Op op = Code::GetOp(i);

    switch (op) {
    case Code::Op::Test:
      if (isTruthy(regs_[Code::GetA(i)]) != static_cast<bool>(Code::GetK(i)))
        pc_++;
      break;
    case Code::Op::Jmp:
      pc_ += Code::GetsJ(i); // Signed jump
      break;
    case Code::Op::Move:
      regs_[Code::GetA(i)] = regs_[Code::GetB(i)];
      break;
    case Code::Op::LoadK:
      regs_[Code::GetA(i)] = block_.get_constants()[Code::GetBx(i)];
      break;
    case Code::Op::Lt:
      regs_[Code::GetA(i)] =
          Comp(regs_[Code::GetB(i)], regs_[Code::GetC(i)], std::less<>{});
      break;
    case Code::Op::Le:
      regs_[Code::GetA(i)] =
          Comp(regs_[Code::GetB(i)], regs_[Code::GetC(i)], std::less_equal<>{});
      break;
    case Code::Op::Gt:
      regs_[Code::GetA(i)] =
          Comp(regs_[Code::GetB(i)], regs_[Code::GetC(i)], std::greater<>{});
      break;
    case Code::Op::Ge:
      regs_[Code::GetA(i)] = Comp(regs_[Code::GetB(i)], regs_[Code::GetC(i)],
                                  std::greater_equal<>{});
      break;
    case Code::Op::Eq:
      regs_[Code::GetA(i)] =
          Comp(regs_[Code::GetB(i)], regs_[Code::GetC(i)], std::equal_to<>{});
      break;
    case Code::Op::Ne:
      regs_[Code::GetA(i)] = Comp(regs_[Code::GetB(i)], regs_[Code::GetC(i)],
                                  std::not_equal_to<>{});
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
      if (Object::isZero(b))
        throw RunTimeError{"Division by zero"}; // TODO: Construct error string
                                                // from line loc
      regs_[Code::GetA(i)] =
          Arith(regs_[Code::GetB(i)], regs_[Code::GetC(i)], std::divides<>{});
    } break;
    case Code::Op::Ret:
      return regs_[Code::GetA(i)];
    }
  }
}

void VM::Run() {
  try {
    MockRun();
  } catch (const RunTimeError &e) {
    std::cout << e.what() << "\n";
  }
}
