#include "ast/ast.hpp"
#include <iostream>

Ast::Ast(std::string_view source, TokenBuffer tokens, NodeBuffer nodes,
         std::vector<unsigned int> extra_data, std::vector<ParseError> errors)
    : source_{source}, tokens_{tokens}, nodes_{nodes}, extra_data_{extra_data},
      errors_{errors} {}

bool Ast::CheckErrors() const {
  if (errors_.size() > 0) {
    for (auto &err : errors_) {
      const auto start = tokens_.Starts()[err.token];
      const auto len = tokens_.Lens()[err.token];
      switch (err.kind) {
      case ParseError::Kind::UnexpectedToken:
        std::cout << "Unexpected Token: " << source_.substr(start, len) << "\n";
        break;
      }
    }

    return true;
  }

  return false;
}
