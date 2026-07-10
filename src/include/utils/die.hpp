#ifndef DIE_HPP_
#define DIE_HPP_
#include <cstdlib>
#include <iostream>

class DieLoudly {
  const char *msg;

public:
  explicit DieLoudly(const char *msg) : msg(msg) {}
  [[noreturn]] void operator()() const {
    std::cerr << msg << std::endl;
    std::abort();
  }
};

#endif
