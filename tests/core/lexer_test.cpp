#include "lexer/lexer.hpp"
#include "token/token.hpp"
#include <gtest/gtest.h>
#include <vector>

std::vector<Token> RunLexer(std::string source) {
  Lexer lex{source};
  return lex.ScanTokens();
}

TEST(Lexer, LexBangs) {
  std::vector<Token::Kind> kCases = {
      Token::Kind::Bang,
      Token::Kind::BangEqual,
      Token::Kind::EndOfFile,
  };

  std::vector<Token> tests = RunLexer("! !=");

  ASSERT_EQ(kCases.size(), tests.size());

  for (size_t i = 0; i < kCases.size(); i++) {
    EXPECT_EQ(kCases[i], tests[i].kind);
  }
}

TEST(Lexer, LexBinops) {
  std::vector<Token::Kind> kCases = {
      Token::Kind::Plus,  Token::Kind::Minus,     Token::Kind::Star,
      Token::Kind::Slash, Token::Kind::EndOfFile,
  };

  std::vector<Token> tests = RunLexer("+ - * /");

  ASSERT_EQ(kCases.size(), tests.size());

  for (size_t i = 0; i < kCases.size(); i++) {
    EXPECT_EQ(kCases[i], tests[i].kind);
  }
}

TEST(Lexer, LexAssignBinops) {
  std::vector<Token::Kind> kCases = {
      Token::Kind::PlusEqual,  Token::Kind::MinusEqual, Token::Kind::StarEqual,
      Token::Kind::SlashEqual, Token::Kind::EndOfFile,
  };

  std::vector<Token> tests = RunLexer("+= -= *= /=");

  ASSERT_EQ(kCases.size(), tests.size());

  for (size_t i = 0; i < kCases.size(); i++) {
    EXPECT_EQ(kCases[i], tests[i].kind);
  }
}

TEST(Lexer, LexIdents) {
  std::vector<Token::Kind> kCases{
      Token::Kind::Identifier, Token::Kind::If,  Token::Kind::Else,
      Token::Kind::While,      Token::Kind::For, Token::Kind::EndOfFile,
  };

  std::vector<Token> tests = RunLexer("foo if else while for");

  ASSERT_EQ(kCases.size(), tests.size());

  for (size_t i = 0; i < kCases.size(); i++) {
    EXPECT_EQ(kCases[i], tests[i].kind);
  }
}

TEST(Lexer, LexNumbers) {
  std::vector<Token::Kind> kCases{
      Token::Kind::Identifier,
      Token::Kind::Int,
      Token::Kind::Float,
      Token::Kind::EndOfFile,
  };

  std::vector<Token> tests = RunLexer("bar 1 1.0");

  ASSERT_EQ(kCases.size(), tests.size());

  for (size_t i = 0; i < kCases.size(); i++) {
    EXPECT_EQ(kCases[i], tests[i].kind);
  }
}

TEST(Lexer, LexContainers) {
  std::vector<Token::Kind> kCases{
      Token::Kind::LeftParen,   Token::Kind::RightParen,
      Token::Kind::LeftBrace,   Token::Kind::RightBrace,
      Token::Kind::LeftBracket, Token::Kind::RightBracket,
      Token::Kind::EndOfFile,
  };

  std::vector<Token> tests = RunLexer("( ) { } [ ]");

  ASSERT_EQ(kCases.size(), tests.size());

  for (size_t i = 0; i < kCases.size(); i++) {
    EXPECT_EQ(kCases[i], tests[i].kind);
  }
}
