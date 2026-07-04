#include "ast/ast.hpp"

Ast::Ast(std::string_view source, TokenBuffer tokens, NodeBuffer nodes,
         std::vector<ParseError> errors)
    : source_{source}, tokens_{tokens}, nodes_{nodes}, errors_{errors} {}
