#include "ast/ast.hpp"
#include "lexer/lexer.hpp"
#include "parser/parser.hpp"

int main() {
  const char *source = "1 + 1";
  Lexer l{source};
  std::vector<Token> tokens = l.ScanTokens();
  Parser p{source, tokens};
  Ast ast = p.Parse();
  return 0;
}
