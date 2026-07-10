#include "strpool/strpool.hpp"

using namespace Anvil;

StrPool::Index StrPool::InternOrGet(std::string_view str) {
  auto [it, interned] = pooled_.try_emplace(str, StrPool::Index{tracked_});
  if (interned) tracked_++;
  return it->second;
}

std::optional<StrPool::Index> StrPool::Get(std::string_view str) {
  auto it = pooled_.find(str);
  if (it == pooled_.end()) return std::nullopt;
  return it->second;
}
