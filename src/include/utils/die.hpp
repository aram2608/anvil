#ifndef DIE_HPP_
#define DIE_HPP_
#include <cstdlib>
#include <iostream>

class DieLoudly {
  const char *msg;

public:
  explicit DieLoudly(const char *msg) : msg(msg) {}
  void operator()() const {
    std::cout << msg << std::endl;
    std::abort();
  }
};

#endif
