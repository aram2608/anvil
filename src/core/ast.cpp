#include "ast/ast.hpp"
#include "ast/node.hpp"
#include <iostream>

using namespace Anvil;

Ast::Ast(std::string_view source, TokenBuffer tokens, NodeBuffer nodes,
         std::vector<uint32_t> extra_data, std::vector<ParseError> errors)
    : source_{source}, tokens_{tokens}, nodes_{nodes}, extra_data_{extra_data},
      errors_{errors} {}

bool Ast::CheckErrors() const {
  if (errors_.size() > 0) {
    for (auto &err : errors_) {
      const auto start = tokens_.Starts()[ToU32(err.token)];
      const auto len = tokens_.Lens()[ToU32(err.token)];
      const auto line = tokens_.Lines()[ToU32(err.token)];
      switch (err.kind) {
      case ParseError::Kind::UnexpectedToken:
        std::cout << "Unexpected Token: " << source_.substr(start, len) << "\n";
        break;
      case ParseError::Kind::MissingSemicolon:
        std::cout << "Missing semicolon at line " << line << "\n";
        break;
      case ParseError::Kind::MissingClosingBrace:
        std::cout << "Missing closing '}' at line " << line + 1 << "\n";
        break;
      }
    }
    return true;
  }

  return false;
}
