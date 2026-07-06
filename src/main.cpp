#include "ast/ast.hpp"
#include "compiler/compiler.hpp"
#include "dis/dis.hpp"
#include "lexer/lexer.hpp"
#include "parser/parser.hpp"
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <optional>

std::optional<std::string> ReadFile(std::filesystem::path path) {
  auto stream = std::ifstream(path);
  if (!stream.is_open()) return std::nullopt;

  return std::string(std::istreambuf_iterator<char>(stream),
                     std::istreambuf_iterator<char>());
}

int main(int argc, char **argv) {
  if (argc > 1) {
    std::optional<std::string> source_or_fail = ReadFile(argv[1]);
    if (source_or_fail.has_value()) {
      std::string source = source_or_fail.value();
      Lexer l{source};
      std::vector<Token> tokens = l.ScanTokens();
      Parser p{source, tokens};
      Ast ast = p.Parse();
      if (ast.CheckErrors()) {
        return 1;
      }
      Compiler c{source, ast};
      Block b = c.Compile();
      std::cout << Dis::Disassemble(b);
    } else {
      std::cout << argv[1] << " could not be opened\n";
    }
  } else {
    std::cout << "Missing file\n";
  }

  return 0;
}
