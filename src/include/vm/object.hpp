#ifndef OBJECT_HPP_
#define OBJECT_HPP_

#include <cstdint>
#include <type_traits>

namespace Object {

enum class Kind : uint8_t {
  Void = 0,
  Int = 1,
  Float = 2,
  Bool = 3,
};

struct Void {};

struct Value {
  union {
    int64_t i;
    double f;
    bool b;
    Void v;
  } as;
  Kind kind;
};

static_assert(sizeof(Value) == 16, "8B payload + tag padded to 16");
static_assert(std::is_trivially_copyable_v<Value>);

inline bool checkType(const Value &v, Kind k) { return v.kind == k; }

inline Value mkInt(int64_t x) {
  return Value{.as = {.i = x}, .kind = Kind::Int};
}

inline Value mkFloat(double x) {
  return Value{.as = {.f = x}, .kind = Kind::Float};
}

inline Value mkBool(bool x) {
  return Value{.as = {.b = x}, .kind = Kind::Bool};
}

inline Value mkVoid(Void x) {
  return Value{.as = {.v = x}, .kind = Kind::Void};
}

inline bool isBool(const Value &v) { return v.kind == Kind::Bool; }
inline bool isInt(const Value &v) { return v.kind == Kind::Int; }
inline bool isFloat(const Value &v) { return v.kind == Kind::Float; }

// TODO: Find a better solution for this
// This should ideally only handle numeric types, perhaps we throw an error at
// the site of use if the kind is not numeric, not sure
inline bool isZero(const Value &v) {
  switch (v.kind) {
  case Kind::Float:
    return v.as.f == 0;
  case Kind::Int:
    return v.as.i == 0;
  case Kind::Void:
  case Kind::Bool:
    return false;
  }
}

inline int64_t asInt(const Value &v) { return v.as.i; }
inline double asFloat(const Value &v) { return v.as.f; }

} // namespace Object

#endif
