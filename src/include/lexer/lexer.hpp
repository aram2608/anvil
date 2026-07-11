#ifndef LEXER_HPP_
#define LEXER_HPP_
#include "token/token.hpp"
#include <string_view>
#include <vector>

namespace Anvil {

class Lexer {
private:
  std::vector<Token> tokens_;
  std::string_view source_; // Extern. owned data
  int current_{0};
  int start_{0};
  int line_{1};

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
  std::string_view Slice();

public:
  Lexer(std::string_view sourc);
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
  friend void DispatchLexLeftParen(Lexer &lex);
  friend void DispatchLexLeftBracket(Lexer &lex);
  friend void DispatchLexLeftBrace(Lexer &lex);
  friend void DispatchLexRightParen(Lexer &lex);
  friend void DispatchLexRightBracket(Lexer &lex);
  friend void DispatchLexRightBrace(Lexer &lex);
  friend void DispatchLexSemiColon(Lexer &lex);
  friend void DispatchLexBang(Lexer &lex);
  friend void DispatchLexEqual(Lexer &lex);
  friend void DispatchLexGreater(Lexer &lex);
  friend void DispatchLexLesser(Lexer &lex);
  friend void DispatchLexComma(Lexer &lex);
  friend void DispatchLexDoubleQuote(Lexer &lex);
  friend void DispatchLexTilde(Lexer &lex);
  friend void DispatchLexAmp(Lexer &lex);
  friend void DispatchLexCaret(Lexer &lex);
  friend void DispatchLexBar(Lexer &lex);
  friend void DispatchLexAt(Lexer &lex);
};

} // namespace Anvil

#endif
