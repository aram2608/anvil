#ifndef OBJECT_HPP_
#define OBJECT_HPP_

#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <string_view>
#include <type_traits>

// TODO: Implementation level details are leaking into this file
// consider leaving declarations here and writing a definition file
// to not break the one def. rule

namespace Object {

inline uint32_t HashBytes(const char *c, size_t len) {
  // 32-bit FNV offset basis
  uint32_t hash = 2166136261U;
  // 32-bit FNV prime
  const uint32_t fnv_prime = 16777619U;

  for (size_t i = 0; i < len; i++) {
    // XOR the bottom with the current byte
    hash ^= static_cast<uint8_t>(c[i]);
    // Multiply by the FNV prime
    hash *= fnv_prime;
  }

  return hash;
}

enum class Kind : uint8_t {
  Void = 0,
  Int = 1,
  Float = 2,
  Bool = 3,
  String = 4,
};

struct Void {};

struct String {
  uint32_t len;
  uint32_t hash;

  const char *data() const {
    // Data exists at the end of the header
    return reinterpret_cast<const char *>(this) + sizeof(String);
  }

  std::string_view view() const { return {data(), len}; }

  static String *Create(std::string_view sv) {
    // Allocate a big chunk of data for the header and the string
    void *block = std::malloc(sizeof(String) + sv.size());
    // cram the entire thing into the block in order
    // the bytes should be ordered as the following
    // len
    // hash
    // data*
    String *s = new (block) String{static_cast<uint32_t>(sv.size()),
                                   HashBytes(sv.data(), sv.size())};
    std::memcpy(reinterpret_cast<char *>(block) + sizeof(String), sv.data(),
                sv.size());
    return s;
  }
};

// Need to keep track of padding
// if this changes the book keeping above needs to change a bit
// garbage collection may require a new field so adjust accordingly then
static_assert(sizeof(String) == 8);

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
  return Value{.as = {.s = String::Create(sv)}, .kind = Kind::String};
}

inline Value mkVoid(Void x) {
  return Value{.as = {.v = x}, .kind = Kind::Void};
}

inline std::string_view asView(String *s) { return s->view(); }

inline Value ConcatString(String *a, String *b) {
  uint32_t len_a = a->len;
  uint32_t len_b = b->len;

  // TODO: pretty sure i can just take the lhs hash and apply
  // the rehash for rhs
  // ConcatHash(lhs hash, rhs hash, char* b, size_t len_b)
  // since the first iteration simply computes the exact same hash
  // that the lhs has
  auto hasher = [&]() {
    // 32-bit FNV offset basis
    uint32_t hash = 2166136261U;
    // 32-bit FNV prime
    const uint32_t fnv_prime = 16777619U;

    for (size_t i = 0; i < len_a; ++i) {
      // XOR the bottom with the current byte
      hash ^= static_cast<uint8_t>(a->data()[i]);
      // Multiply by the FNV prime
      hash *= fnv_prime;
    }

    for (size_t i = 0; i < len_b; ++i) {
      // XOR the bottom with the current byte
      hash ^= static_cast<uint8_t>(b->data()[i]);
      // Multiply by the FNV prime
      hash *= fnv_prime;
    }

    return hash;
  };

  void *block = std::malloc(sizeof(String) + len_a + len_b);

  String *s = new (block) String{len_a + len_b, hasher()};
  char *buffer = reinterpret_cast<char *>(block);
  std::memcpy(buffer + sizeof(String), a, len_a);
  std::memcpy(buffer + sizeof(String) + len_a, b, len_b);

  return Value{.as = {.s = s}, .kind = Kind::String};
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
