#include "ast/ast.hpp"
#include "lexer/lexer.hpp"
#include "parser/parser.hpp"
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>

std::string ReadFile(std::filesystem::path path) {
  auto stream = std::ifstream(path);
  return std::string(std::istreambuf_iterator<char>(stream),
                     std::istreambuf_iterator<char>());
}

int main(int argc, char **argv) {
  if (argc > 1) {
    std::string source = ReadFile(argv[1]);
    Lexer l{source};
    std::vector<Token> tokens = l.ScanTokens();
    Parser p{source, tokens};
    Ast ast = p.Parse();
    if (ast.CheckErrors()) {
      return 1;
    }
  } else {
    std::cout << "Missing file\n";
  }

  return 0;
}
