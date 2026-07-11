#include "vm/vm.hpp"
#include "compiler/bytecode.hpp"
#include "utils/die.hpp"
#include "vm/builtins.hpp"
#include "vm/object.hpp"
#include <cassert>
#include <functional>
#include <iostream>
#include <stdexcept>

using namespace Anvil;

namespace {

class RunTimeError : public std::runtime_error {
public:
  explicit RunTimeError(std::string message)
      : std::runtime_error{message.c_str()} {}
};

// TODO: Ponder on this more
// Implicit promotion, perhaps we can error instead not sure
template <typename Fn>
inline Object::Value Arith(const Object::Value &a, const Object::Value &b,
                           Fn op) {
  if (Object::isVoid(a) || Object::isVoid(b)) {
    throw RunTimeError{"Invalid operation for type `Void`"};
  }

  if (Object::isInt(a) && Object::isInt(b)) {
    return Object::mkInt(op(a.asInt(), b.asInt()));
  } else if (Object::isFloat(a) && Object::isFloat(b)) {
    return Object::mkFloat(op(a.asFloat(), b.asFloat()));
  } else if (Object::isFloat(a) && Object::isInt(b)) {
    return Object::mkFloat(op(a.asFloat(), b.asInt()));
  } else if (Object::isInt(a) && Object::isFloat(b)) {
    return Object::mkFloat(op(a.asInt(), b.asFloat()));
  }
  DieLoudly("Unreachable at Arith for ", {a, b});
}

template <typename Fn>
inline Object::Value Comp(const Object::Value &a, const Object::Value &b,
                          Fn op) {

  if (Object::isVoid(a) || Object::isVoid(b)) {
    throw RunTimeError{"Invalid operation for type `Void`"};
  }

  if (Object::isInt(a) && Object::isInt(b)) {
    return Object::mkBool(op(a.asInt(), b.asInt()));
  } else if (Object::isFloat(a) && Object::isFloat(b)) {
    return Object::mkBool(op(a.asFloat(), b.asFloat()));
  } else if (Object::isFloat(a) && Object::isInt(b)) {
    return Object::mkBool(op(a.asFloat(), b.asInt()));
  } else if (Object::isInt(a) && Object::isFloat(b)) {
    return Object::mkBool(op(a.asInt(), b.asFloat()));
  }
  DieLoudly("Unreachable at Arith for ", {a, b});
}

inline bool isTruthy(const Object::Value &x) {
  switch (x.kind) {
  case Object::Kind::Int:
    return x.asInt() != 0;
  case Object::Kind::Float:
    return x.asFloat() != 0;
  case Object::Kind::Bool:
    return x.asBool();
  case Object::Kind::String:
    return x.asString()->len != 0;
  case Object::Kind::Void:
    throw RunTimeError{"Type `Void` is not legal for comparisons"};
  }
  DieLoudly("Unreachable at isTruthy for", {x});
}

inline bool ValueEq(const Object::Value &a, const Object::Value &b) {
  if (Object::isNumeric(a) && Object::isNumeric(b)) {
    std::equal_to<> op;
    if (Object::isInt(a) && Object::isInt(b)) {
      return op(a.asInt(), b.asInt());
    } else if (Object::isFloat(a) && Object::isFloat(b)) {
      return op(a.asFloat(), b.asFloat());
    } else if (Object::isFloat(a) && Object::isInt(b)) {
      return op(a.asFloat(), b.asInt());
    } else if (Object::isInt(a) && Object::isFloat(b)) {
      return op(a.asInt(), b.asFloat());
    }
  }
  if (a.kind != b.kind) return false;
  switch (a.kind) {
  case Object::Kind::Bool:
    return a.asBool() == b.asBool();
  case Object::Kind::String:
    return a.asString() == b.asString();
  case Object::Kind::Void:
    return true;
  default:
    DieLoudly("Unreachable at ValueEq for ", {a, b});
  }
}

} // namespace

VM::VM(Block block, StringTable &str_table)
    : block_{block}, str_table_{str_table} {}

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
    case Code::Op::Eq: {
      const Object::Value a = regs_[Code::GetB(i)];
      const Object::Value b = regs_[Code::GetC(i)];
      regs_[Code::GetA(i)] = Object::mkBool(ValueEq(a, b));
    } break;
    case Code::Op::Ne: {
      const Object::Value a = regs_[Code::GetB(i)];
      const Object::Value b = regs_[Code::GetC(i)];
      regs_[Code::GetA(i)] = Object::mkBool(!ValueEq(a, b));
    } break;
    case Code::Op::Add: {
      const Object::Value a = regs_[Code::GetB(i)];
      const Object::Value b = regs_[Code::GetC(i)];

      if (Object::isString(a) && Object::isString(b)) {
        Object::String *str = str_table_.Concat(a.asString(), b.asString());
        regs_[Code::GetA(i)] = Object::mkString(str);
      } else {
        regs_[Code::GetA(i)] = Arith(a, b, std::plus<>{});
      }

    } break;
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
    case Code::Op::CallB: {
      uint32_t base = Code::GetA(i);
      uint32_t argc = Code::GetB(i);
      regs_[base] = kNatives[Code::GetC(i)].fn(
          std::span<Object::Value>{&regs_[base], argc});
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
