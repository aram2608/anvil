#ifndef DIE_HPP_
#define DIE_HPP_

#include "vm/object.hpp"
#include <cstdlib>
#include <initializer_list>
#include <iostream>

[[noreturn]] inline void DieLoudly(const char *m) {
  std::cerr << m << std::endl;
  std::abort();
}

[[noreturn]] inline void DieLoudly(const char *m,
                                   std::initializer_list<Object::Value> vals) {
  std::string msg{m};
  for (const Object::Value &v : vals) {
    msg += ' ';
    msg += Object::ToRepr(v);
  }
  std::cerr << msg << std::endl;
  std::abort();
}

#endif
