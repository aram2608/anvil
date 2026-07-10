#ifndef OBJECT_HPP_
#define OBJECT_HPP_

#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <string_view>
#include <type_traits>

namespace Object {

enum class Kind : uint8_t {
  Void = 0,
  Int = 1,
  Float = 2,
  Bool = 3,
  String = 4,
};

struct Void {};

using String = const char;

struct StringHeader {
  uint32_t len;
  uint32_t hash;
};

inline uint32_t GetStringHash(const String *s) {
  const StringHeader *header =
      (const StringHeader *)((const char *)s - sizeof(StringHeader));
  return header->hash;
}

inline uint32_t GetStringLen(const String *s) {
  const StringHeader *header =
      (const StringHeader *)((const char *)s - sizeof(StringHeader));
  return header->len;
}

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

inline Value mkString(std::string_view sv) {
  // Allocate a memory block big enough for the header and the string
  char *block = (char *)std::malloc(sizeof(StringHeader) + sv.size() + 1);

  auto hasher = [&]() {
    // 32-bit FNV offset basis
    uint32_t hash = 2166136261U;
    // 32-bit FNV prime
    const uint32_t fnv_prime = 16777619U;

    for (char c : sv) {
      // XOR the bottom with the current byte
      hash ^= static_cast<uint8_t>(c);
      // Multiply by the FNV prime
      hash *= fnv_prime;
    }

    return hash;
  };

  // type punt to get the header from the block
  StringHeader *header = (StringHeader *)block;
  header->len = sv.size();
  header->hash = hasher();

  // [HEADER______START OF STRING]
  char *data_ptr = block + sizeof(StringHeader);

  std::memcpy(data_ptr, sv.data(), sv.length());

  return Value{.as = {.s = data_ptr}, .kind = Kind::String};
}

inline Value mkVoid(Void x) {
  return Value{.as = {.v = x}, .kind = Kind::Void};
}

inline Value ConcatString(String *a, String *b) {
  uint32_t len_a = GetStringLen(a);
  uint32_t len_b = GetStringLen(b);

  auto hasher = [&]() {
    // 32-bit FNV offset basis
    uint32_t hash = 2166136261U;
    // 32-bit FNV prime
    const uint32_t fnv_prime = 16777619U;

    for (size_t i = 0; i < len_a; ++i) {
      // XOR the bottom with the current byte
      hash ^= static_cast<uint8_t>(a[i]);
      // Multiply by the FNV prime
      hash *= fnv_prime;
    }

    for (size_t i = 0; i < len_b; ++i) {
      // XOR the bottom with the current byte
      hash ^= static_cast<uint8_t>(b[i]);
      // Multiply by the FNV prime
      hash *= fnv_prime;
    }

    return hash;
  };

  char *block = (char *)std::malloc(sizeof(StringHeader) + len_a + len_b + 1);
  StringHeader *header = (StringHeader *)block;
  header->len = len_a + len_b;
  header->hash = hasher();

  char *data_ptr = block + sizeof(StringHeader);
  std::memcpy(data_ptr, a, len_a);
  std::memcpy(data_ptr + len_a, b, len_b);

  return Value{.as = {.s = data_ptr}, .kind = Kind::String};
}

inline bool isBool(const Value &v) { return v.kind == Kind::Bool; }
inline bool isInt(const Value &v) { return v.kind == Kind::Int; }
inline bool isFloat(const Value &v) { return v.kind == Kind::Float; }
inline bool isVoid(const Value &v) { return v.kind == Kind::Void; }
inline bool isString(const Value &v) { return v.kind == Kind::String; }

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
  case Kind::String:
    return GetStringLen(v.as.s) == 0;
    return false;
  }
}

} // namespace Object

#endif
