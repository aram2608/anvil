#ifndef LEXER_HPP_
#define LEXER_HPP_
#include "token/token.hpp"
#include <string>
#include <vector>

class Lexer {
private:
  std::vector<Token> tokens_;
  std::string source_;
  int current_;
  int start_;

  void Scan();
  char Advance();
  void PushToken(Token::Kind kind);
  bool Match(char c);
  char Peek();
  char PeekNext();
  char LookBack();
  void OpAssign(Token::Kind expected, Token::Kind fallback);
  bool IsEnd();
  bool InNumber();
  std::string Slice();

public:
  Lexer(std::string source);
  std::vector<Token> ScanTokens();

  // OOP is a bit of a scam I suppose
  // These are needed to use internal methods
  // The API should not expose anything other the ScanTokens() so these
  // are basically static
  friend void DispatchLexError(Lexer &lex);
  friend void DispatchLexSlash(Lexer &lex);
  friend void DispatchLexPlus(Lexer &lex);
  friend void DispatchLexIdentifier(Lexer &lex);
  friend void DispatchLexStar(Lexer &lex);
  friend void DispatchLexMinus(Lexer &lex);
  friend void DispatchLexNumber(Lexer &lex);
  friend void DispatchLexDot(Lexer &lex);
  friend void DispatchLexColon(Lexer &lex);
  friend void DispatchLexWhiteSpace(Lexer &lex);
  friend void DispatchLexNewLine(Lexer &lex);
  friend void DispatchLeftParen(Lexer &lex);
  friend void DispatchLeftBracket(Lexer &lex);
  friend void DispatchLeftBrace(Lexer &lex);
  friend void DispatchRightParen(Lexer &lex);
  friend void DispatchRightBracket(Lexer &lex);
  friend void DispatchRightBrace(Lexer &lex);
};

#endif
