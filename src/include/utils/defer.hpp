#ifndef DEFER_HPP_
#define DEFER_HPP_

#include <algorithm>

template <typename F>
struct ScopeDefer {
  F func;
  ScopeDefer(F &&f) : func(std::move(f)) {}
  ~ScopeDefer() { func(); }
};

// Concats to create unique identifier per line
#define CONCAT_INNER(a, b) a##b
#define CONCAT(a, b) CONCAT_INNER(a, b)

#define defer const ScopeDefer CONCAT(defer_var_, __LINE__) = [&]

#endif
