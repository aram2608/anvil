#ifndef STRTABLE_HPP_
#define STRTABLE_HPP_

#include "vm/object.hpp"
#include <cstddef>
#include <cstdint>
#include <string_view>
#include <vector>

class StringTable {
  std::vector<Object::String *> buckets_; // has to be a power of two
  size_t count_;
  void Insert(Object::String *s);
  Object::String *Lookup(uint32_t hash, std::string_view sv) const;
  void MaybeGrow();

public:
  StringTable();
  Object::String *Intern(std::string_view sv);
  Object::String *Concat(Object::String *a, Object::String *b);
};

#endif
