#ifndef OBJECT_HPP_
#define OBJECT_HPP_

#include <cstdint>
#include <type_traits>

namespace Object {

enum class Kind : uint8_t {
  Void = 0,
  Int = 1,
  Float = 2,
};

struct Value {
  union {
    int64_t i;
    double f;
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

inline bool isInt(const Value &v) { return v.kind == Kind::Int; }
inline bool isFloat(const Value &v) { return v.kind == Kind::Float; }

inline int64_t asInt(const Value &v) { return v.as.i; }
inline double asFloat(const Value &v) { return v.as.f; }

} // namespace Object

#endif
