#ifndef BUILTIN_HPP_
#define BUILTIN_HPP_

#include "vm/object.hpp"
#include <cstdint>
#include <ios>
#include <iostream>
#include <span>

// Funtion pointer taking a span of Values as args
using BuiltinFn = Object::Value (*)(std::span<Object::Value> args);

enum class Arity : uint8_t { Variadic = 255 };

struct Builtin {
  std::string_view name; // Extern. owned data
  BuiltinFn fn;
  Arity arity;
};

// For now we can be naive
// later on we can create a global StringStream or other efficient
// object to get stuff to the terminal
// For now we just take the object and dump it to the stdout
// but ideally we serialize using formatted strings per type
// "{v}" can forcefully call an objects __repr metamethod or something
// and if it is not defined then bail out with an error
// but we can deal with that once objects are a thing
inline Object::Value BuiltinPrintf(std::span<Object::Value> args) {
  for (const auto a : args) {
    switch (a.kind) {
    case Object::Kind::Void:
      std::cout << "void";
      break;
    case Object::Kind::Int:
      std::cout << a.asInt();
      break;
    case Object::Kind::Float:
      std::cout << a.asFloat();
      break;
    case Object::Kind::Bool:
      std::cout << std::boolalpha << a.asBool();
      break;
    case Object::Kind::String:
      std::cout << a.asString()->view();
      break;
    }
    std::cout << "\n";
  }
  return Object::mkVoid({}); // in C apis printf actually returns the number
                             // of printed bytes, Python returns None
                             // i suppose we can decide that later
}

// index is the builtin id baked into the instruction
inline constexpr std::array<Builtin, 1> kNatives = {{
    {"printf", BuiltinPrintf, Arity::Variadic},
}};

#endif
