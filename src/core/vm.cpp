#include "vm/vm.hpp"
#include "compiler/block.hpp"
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
  case Object::Kind::Function:
    throw RunTimeError{"Type `Function` is not legal for comparisons"};
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

VM::VM(Module module, StringTable &str_table)
    : module_{module}, str_table_{str_table} {
  globals_.resize(module.num_globals, Object::mkVoid({}));
}

// Temporary function before printing can happen
// returns the last value from the Ret instruction
// assumes no errors will happen during excecution
Object::Value VM::MockRun() {
  auto R = [&](uint32_t r) -> Object::Value & { return regs_[bp_ + r]; };
  const Proto *proto = &module_.funcs[0];
  std::span<const Code::Inst> code = proto->block.get_code();
  std::span<const Object::Value> k = proto->block.get_constants();
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
      R(Code::GetA(i)) = R(Code::GetB(i));
      break;
    case Code::Op::LoadK:
      R(Code::GetA(i)) = proto->block.get_constants()[Code::GetBx(i)];
      break;
    case Code::Op::Lt:
      R(Code::GetA(i)) =
          Comp(R(Code::GetB(i)), R(Code::GetC(i)), std::less<>{});
      break;
    case Code::Op::Le:
      R(Code::GetA(i)) =
          Comp(R(Code::GetB(i)), R(Code::GetC(i)), std::less_equal<>{});
      break;
    case Code::Op::Gt:
      R(Code::GetA(i)) =
          Comp(R(Code::GetB(i)), R(Code::GetC(i)), std::greater<>{});
      break;
    case Code::Op::Ge:
      R(Code::GetA(i)) =
          Comp(R(Code::GetB(i)), R(Code::GetC(i)), std::greater_equal<>{});
      break;
    case Code::Op::Eq: {
      const Object::Value a = R(Code::GetB(i));
      const Object::Value b = R(Code::GetC(i));
      R(Code::GetA(i)) = Object::mkBool(ValueEq(a, b));
    } break;
    case Code::Op::Ne: {
      const Object::Value a = R(Code::GetB(i));
      const Object::Value b = R(Code::GetC(i));
      R(Code::GetA(i)) = Object::mkBool(!ValueEq(a, b));
    } break;
    case Code::Op::Add: {
      const Object::Value a = R(Code::GetB(i));
      const Object::Value b = R(Code::GetC(i));

      if (Object::isString(a) && Object::isString(b)) {
        Object::String *str = str_table_.Concat(a.asString(), b.asString());
        R(Code::GetA(i)) = Object::mkString(str);
      } else {
        R(Code::GetA(i)) = Arith(a, b, std::plus<>{});
      }

    } break;
    case Code::Op::Sub:
      R(Code::GetA(i)) =
          Arith(R(Code::GetB(i)), R(Code::GetC(i)), std::minus<>{});
      break;
    case Code::Op::Mult:
      R(Code::GetA(i)) =
          Arith(R(Code::GetB(i)), R(Code::GetC(i)), std::multiplies<>{});
      break;
    case Code::Op::Div: {
      Object::Value b = R(Code::GetC(i));
      if (Object::isZero(b))
        throw RunTimeError{"Division by zero"}; // TODO: Construct error string
                                                // from line loc
      R(Code::GetA(i)) =
          Arith(R(Code::GetB(i)), R(Code::GetC(i)), std::divides<>{});
    } break;
    case Code::Op::Call: {
      uint32_t a = Code::GetA(i);
      uint32_t argc = Code::GetB(i);
      Object::Value fv = R(a);
      if (!Object::isFunction(fv)) throw RunTimeError{"Value is not callable"};
      const Proto *callee = &module_.funcs[fv.as.fidx];
      if (callee->arity != argc)
        throw RunTimeError{"Wrong number of arguments"};
      if (bp_ + a + 1 + callee->max_regs >= regs_.size())
        throw RunTimeError{"Stack overflow"};

      frames_.push_back({
          .return_pc = pc_,
          .bp = bp_,
          .result_reg = bp_ + a, // absolute
          .proto = proto,
      });
      bp_ = bp_ + a + 1;
      proto = callee;
      code = proto->block.get_code();
      k = proto->block.get_constants();
      pc_ = 0;
    } break;
    case Code::Op::CallB: {
      uint32_t base = Code::GetA(i);
      uint32_t argc = Code::GetB(i);
      regs_[base] = kNatives[Code::GetC(i)].fn(
          std::span<Object::Value>{&regs_[bp_ + base], argc});
    } break;
    case Code::Op::GGet:
      R(Code::GetA(i)) = globals_[Code::GetBx(i)];
      break;
    case Code::Op::GSet:
      globals_[Code::GetBx(i)] = R(Code::GetA(i));
      break;
    case Code::Op::Ret:
      Object::Value result = R(Code::GetA(i));
      if (frames_.empty()) return result; // main returned
      Frame f = frames_.back();
      frames_.pop_back();
      regs_[f.result_reg] = result;
      pc_ = f.return_pc;
      bp_ = f.bp;
      proto = f.proto;
      code = proto->block.get_code();
      k = proto->block.get_constants();
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
