#ifndef SEMA_HPP_
#define SEMA_HPP_

#include "ast/ast.hpp"
#include "sema/inst.hpp"
#include <string_view>
#include <vector>

class Sema {
  std::string_view source_;
  std::vector<Inst> instructions_;
  Ast ast_;

public:
  Sema(std::string source, Ast ast);
};

#endif
