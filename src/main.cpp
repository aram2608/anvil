#include "lexer/lexer.hpp"

int main() {
  const char *source = "1 + 1";
  Lexer l{source};
  l.ScanTokens();
  return 0;
}
