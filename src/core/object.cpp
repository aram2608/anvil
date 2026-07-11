#include "vm/object.hpp"
#include <cstring>

Object::String *Object::String::Create(std::string_view sv, uint32_t hash) {
  void *block = std::malloc(sizeof(String) + sv.size());
  String *s =
      new (block) String{nullptr, static_cast<uint32_t>(sv.size()), hash};
  std::memcpy(reinterpret_cast<char *>(block) + sizeof(String), sv.data(),
              sv.size());
  return s;
}

Object::String *Object::String::Create(std::string_view a, std::string_view b,
                                       uint32_t hash) {
  void *block = std::malloc(sizeof(String) + a.size() + b.size());
  String *s = new (block)
      String{nullptr, static_cast<uint32_t>(a.size() + b.size()), hash};
  char *dst = reinterpret_cast<char *>(block) + sizeof(String);
  std::memcpy(dst, a.data(), a.size());
  std::memcpy(dst + a.size(), b.data(), b.size());

  return s;
}

uint32_t Object::HashBytes(const char *c, size_t len, uint32_t hash) {
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
