#include "lexer/lexer.hpp"
#include "token/token.hpp"
#include <gtest/gtest.h>
#include <vector>

using namespace Anvil;

std::vector<Token> RunLexer(std::string source) {
  Lexer lex{source};
  return lex.ScanTokens();
}

void ExpectKinds(const char *src, std::vector<Token::Kind> expected) {
  std::vector<Token> tests = RunLexer(src);

  ASSERT_EQ(expected.size(), tests.size());

  for (size_t i = 0; i < expected.size(); i++) {
    EXPECT_EQ(expected[i], tests[i].kind);
  }
}

TEST(Lexer, LexBangs) {
  ExpectKinds("! !=", {
                          Token::Kind::Bang,
                          Token::Kind::BangEqual,
                          Token::Kind::EndOfFile,
                      });
}

TEST(Lexer, LexBinops) {
  ExpectKinds("+ - * /", {

                             Token::Kind::Plus,
                             Token::Kind::Minus,
                             Token::Kind::Star,
                             Token::Kind::Slash,
                             Token::Kind::EndOfFile,
                         });
}

TEST(Lexer, LexBitwise) {
  ExpectKinds("& ^ | ~", {

                             Token::Kind::Ampersand,
                             Token::Kind::Caret,
                             Token::Kind::Bar,
                             Token::Kind::Tilde,
                             Token::Kind::EndOfFile,
                         });
}

TEST(Lexer, LexAssignBinops) {
  ExpectKinds("+= -= *= /=", {
                                 Token::Kind::PlusEqual,
                                 Token::Kind::MinusEqual,
                                 Token::Kind::StarEqual,
                                 Token::Kind::SlashEqual,
                                 Token::Kind::EndOfFile,
                             });
}

TEST(Lexer, LexIdents) {
  ExpectKinds("foo if else while for", {
                                           Token::Kind::Identifier,
                                           Token::Kind::If,
                                           Token::Kind::Else,
                                           Token::Kind::While,
                                           Token::Kind::For,
                                           Token::Kind::EndOfFile,
                                       });
}

TEST(Lexer, LexNumbers) {
  ExpectKinds("bar 1 1.0", {
                               Token::Kind::Identifier,
                               Token::Kind::IntLiteral,
                               Token::Kind::FloatLiteral,
                               Token::Kind::EndOfFile,
                           });
}

TEST(Lexer, LexContainers) {
  ExpectKinds("( ) { } [ ]", {
                                 Token::Kind::LeftParen,
                                 Token::Kind::RightParen,
                                 Token::Kind::LeftBrace,
                                 Token::Kind::RightBrace,
                                 Token::Kind::LeftBracket,
                                 Token::Kind::RightBracket,
                                 Token::Kind::EndOfFile,
                             });
}
