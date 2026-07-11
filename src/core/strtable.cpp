#include "strtable/strtable.hpp"
#include "vm/object.hpp"

namespace {
constexpr uint32_t kMinTableSize = 64;
}

StringTable::StringTable() : buckets_{kMinTableSize, nullptr}, count_{0} {};

Object::String *StringTable::Intern(std::string_view sv) {
  uint32_t hash = Object::HashBytes(sv.data(), sv.size());
  Object::String *found = Lookup(hash, sv);
  if (found) return found;
  Object::String *s = Object::String::Create(sv, hash);
  Insert(s);
  return s;
}

void StringTable::Insert(Object::String *s) {
  MaybeGrow();
  Object::String **bucket = &buckets_[s->hash & (buckets_.size() - 1)];
  s->next = *bucket;
  *bucket = s;
  count_++;
}

Object::String *StringTable::Lookup(uint32_t hash, std::string_view sv) const {
  using Str = Object::String;
  // 64      = 0b01000000
  // 64 - 1  = 0b00111111
  // hash    = 0b10110101_11001010
  // & 63    = 0b00000000_00001010 -> the 10th bucket
  // By keeping the size a power of two we can do hash % 2^k without having to
  // do a modulo op since it is incredibly slow
  for (Str *s = buckets_[hash & (buckets_.size() - 1)]; s; s = s->next) {
    if (s->hash == hash && s->view() == sv) return s;
  }
  return nullptr;
}

void StringTable::MaybeGrow() {
  using Str = Object::String;
  if (count_ < buckets_.size()) return;

  std::vector<Str *> old = std::move(buckets_);
  // double the vector to keep to the power of 2
  buckets_.assign(old.size() * 2, nullptr);

  for (Str *s : old) {
    while (s) {
      Str *rest = s->next; // save before clobbering
      Str **bucket = &buckets_[s->hash & (buckets_.size() - 1)];
      s->next = *bucket; // re-link
      *bucket = s;
      s = rest;
    }
  }
}

Object::String *StringTable::Concat(Object::String *a, Object::String *b) {
  using Str = Object::String;
  if (a->len == 0) return b;
  if (b->len == 0) return a;

  uint32_t len = a->len + b->len;
  uint32_t hash = Object::HashBytes(b->data(), b->len, a->hash);

  for (Str *s = buckets_[hash & (buckets_.size() - 1)]; s != nullptr;
       s = s->next) {
    if (s->hash == hash && s->len == len &&
        std::memcmp(s->data(), a->data(), a->len) == 0 &&
        std::memcmp(s->data() + a->len, b->data(), b->len) == 0)
      return s;
  }

  Object::String *s = Object::String::Create(a->view(), b->view(), hash);
  Insert(s);
  return s;
}
