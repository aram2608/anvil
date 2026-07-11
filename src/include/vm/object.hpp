#ifndef OBJECT_HPP_
#define OBJECT_HPP_

#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <string>
#include <string_view>
#include <type_traits>

namespace Object {

uint32_t HashBytes(const char *c, size_t len,
                   uint32_t hash = 2166136261U /* FNV offset basis */);

enum class Kind : uint8_t {
  Void = 0,
  Int = 1,
  Float = 2,
  Bool = 3,
  String = 4,
};

struct Void {};

struct String {
  String *next;
  uint32_t len;
  uint32_t hash;

  const char *data() const {
    // Data exists at the end of the header
    return reinterpret_cast<const char *>(this) + sizeof(String);
  }

  std::string_view view() const { return {data(), len}; }

  static String *Create(std::string_view sv, uint32_t hash);
  static String *Create(std::string_view a, std::string_view b, uint32_t hash);
};

// Need to keep track of padding
// if this changes the book keeping above needs to change a bit
// garbage collection may require a new field so adjust accordingly then
static_assert(sizeof(String) == 16);

struct Value {
  union {
    int64_t i;
    double f;
    bool b;
    Void v;
    String *s;
  } as;
  Kind kind;

  inline int64_t asInt() const { return as.i; }
  inline double asFloat() const { return as.f; }
  inline bool asBool() const { return as.b; }
  inline String *asString() const { return as.s; }
};

static_assert(sizeof(Value) == 16, "8B payload + tag padded to 16");
// needs to stay small so the VM can rip through values easy peasy
static_assert(std::is_trivially_copyable_v<Value>);

inline bool checkType(const Value &v, Kind k) { return v.kind == k; }

inline std::string ToRepr(const Value &v) {
  switch (v.kind) {
  case Kind::Void:
    return "void";
  case Kind::Int:
    return std::to_string(v.asInt());
  case Kind::Float:
    return std::to_string(v.asFloat());
  case Kind::Bool:
    return v.asBool() ? "true" : "false";
  case Kind::String:
    return v.asString()->data();
  }
}

inline Value mkInt(int64_t x) {
  return Value{.as = {.i = x}, .kind = Kind::Int};
}

inline Value mkFloat(double x) {
  return Value{.as = {.f = x}, .kind = Kind::Float};
}

inline Value mkBool(bool x) {
  return Value{.as = {.b = x}, .kind = Kind::Bool};
}

inline Value mkString(String *s) {
  return Value{.as = {.s = s}, .kind = Kind::String};
}

inline Value mkVoid(Void x) {
  return Value{.as = {.v = x}, .kind = Kind::Void};
}

inline bool isBool(const Value &v) { return v.kind == Kind::Bool; }
inline bool isInt(const Value &v) { return v.kind == Kind::Int; }
inline bool isFloat(const Value &v) { return v.kind == Kind::Float; }
inline bool isVoid(const Value &v) { return v.kind == Kind::Void; }
inline bool isString(const Value &v) { return v.kind == Kind::String; }
inline bool isNumeric(const Value &v) { return isFloat(v) || isInt(v); }

// TODO: Find a better solution for this
// This should ideally only handle numeric types, perhaps we throw an error at
// the site of use if the kind is not numeric, not sure
inline bool isZero(const Value &v) {
  switch (v.kind) {
  case Kind::Float:
    return v.asFloat() == 0;
  case Kind::Int:
    return v.asInt() == 0;
  case Kind::String:
    return v.asString()->len == 0;
  case Kind::Void:
  case Kind::Bool:
    return false;
  }
}

} // namespace Object

#endif
