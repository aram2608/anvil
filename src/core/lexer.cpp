#include "lexer/lexer.hpp"
#include "token/token.hpp"
#include <algorithm>
#include <array>
#include <cassert>
#include <cctype>
#include <cstddef>
#include <optional>
#include <string_view>
#include <utility>

// We don't need a full hash map for keywords
// So we can make a "simple" wrapper around a key (string)
// and value (Token::Kind) pairs
template <std::size_t Size>
struct KeyWordMap {
  std::array<std::pair<std::string_view, Token::Kind>, Size> data;

  constexpr std::optional<Token::Kind> at(const std::string_view &key) const {
    const auto itr =
        std::find_if(std::begin(data), std::end(data),
                     [&key](const auto &v) { return v.first == key; });
    if (itr != std::end(data)) {
      return itr->second;
    } else {
      return std::nullopt;
    }
  }
};

template <std::size_t Size>
KeyWordMap(std::array<std::pair<std::string_view, Token::Kind>, Size>)
    -> KeyWordMap<Size>;

static constexpr auto kKeyWords =
    KeyWordMap{std::to_array<std::pair<std::string_view, Token::Kind>>({
        {"if", Token::Kind::If},
        {"while", Token::Kind::While},
        {"for", Token::Kind::For},
        {"proc", Token::Kind::FuncDecl},
        {"else", Token::Kind::Else},
        {"return", Token::Kind::Return},
    })};

Lexer::Lexer(std::string_view source)
    : source_{source}, start_(0), current_(0), line_(1) {}

std::vector<Token> Lexer::ScanTokens() {
  while (!IsEnd()) {
    start_ = current_;
    Scan();
  }
  PushToken(Token::Kind::EndOfFile);
  return tokens_;
}

using DispatchFunctionT = void(Lexer &lexer);
using DispatchTableT = std::array<DispatchFunctionT *, 256>;

void DispatchLexError(Lexer &lex) { lex.PushToken(Token::Kind::Error); }

void DispatchLexSlash(Lexer &lex) {
  lex.OpAssign(Token::Kind::SlashEqual, Token::Kind::Slash);
}

void DispatchLexPlus(Lexer &lex) {
  lex.OpAssign(Token::Kind::PlusEqual, Token::Kind::Plus);
}

void DispatchLexStar(Lexer &lex) {
  lex.OpAssign(Token::Kind::StarEqual, Token::Kind::Star);
}

void DispatchLexMinus(Lexer &lex) {
  lex.OpAssign(Token::Kind::MinusEqual, Token::Kind::Minus);
}

void DispatchLexColon(Lexer &lex) {
  lex.OpAssign(Token::Kind::ColonEqual, Token::Kind::Colon);
}

void DispatchLexIdentifier(Lexer &lex) {
  while (!lex.IsEnd() && std::isalnum(static_cast<unsigned char>(lex.Peek()))) {
    lex.Advance();
  }
  auto kind = kKeyWords.at(lex.Slice());
  lex.PushToken(kind.value_or(Token::Kind::Identifier));
}

void DispatchLexNumber(Lexer &lex) {
  while (lex.InNumber()) {
    lex.Advance();
  }
  if (lex.Peek() == '.') {
    lex.Advance(); // eat the '.'
    while (lex.InNumber()) {
      lex.Advance();
    }
    lex.PushToken(Token::Kind::FloatLiteral);
  } else {
    lex.PushToken(Token::Kind::IntLiteral);
  }
}

void DispatchLexDot(Lexer &lex) { lex.PushToken(Token::Kind::Dot); }

void DispatchLexWhiteSpace(Lexer &lex) {}

void DispatchLexNewLine(Lexer &lex) { lex.line_ += 1; }

void DispatchLexLeftParen(Lexer &lex) { lex.PushToken(Token::Kind::LeftParen); }

void DispatchLexLeftBracket(Lexer &lex) {
  lex.PushToken(Token::Kind::LeftBracket);
}

void DispatchLexLeftBrace(Lexer &lex) { lex.PushToken(Token::Kind::LeftBrace); }

void DispatchLexRightParen(Lexer &lex) {
  lex.PushToken(Token::Kind::RightParen);
}

void DispatchLexRightBracket(Lexer &lex) {
  lex.PushToken(Token::Kind::RightBracket);
}

void DispatchLexRightBrace(Lexer &lex) {
  lex.PushToken(Token::Kind::RightBrace);
}

void DispatchLexBang(Lexer &lex) {
  lex.OpAssign(Token::Kind::BangEqual, Token::Kind::Bang);
}

void DispatchLexSemiColon(Lexer &lex) { lex.PushToken(Token::Kind::SemiColon); }

void DispatchLexEqual(Lexer &lex) {
  lex.OpAssign(Token::Kind::EqualEqual, Token::Kind::Equal);
}

void DispatchLexLesser(Lexer &lex) {
  lex.OpAssign(Token::Kind::LesserEqual, Token::Kind::Lesser);
}

void DispatchLexGreater(Lexer &lex) {
  lex.OpAssign(Token::Kind::GreaterEqual, Token::Kind::Greater);
}

void DispatchLexComma(Lexer &lex) { lex.PushToken(Token::Kind::Comma); }

static constexpr DispatchTableT kDispatchTable = [] {
  DispatchTableT table{};

  for (int i = 0; i < 256; ++i) {
    table[i] = &DispatchLexError;
  }

  table['/'] = &DispatchLexSlash;
  table['_'] = &DispatchLexIdentifier;
  table['+'] = &DispatchLexPlus;
  table['-'] = &DispatchLexMinus;
  table['*'] = &DispatchLexStar;
  table['.'] = &DispatchLexDot;
  table[':'] = &DispatchLexColon;
  table[' '] = &DispatchLexWhiteSpace;
  table['\t'] = &DispatchLexWhiteSpace;
  table['\n'] = &DispatchLexNewLine;
  table['('] = &DispatchLexLeftParen;
  table[')'] = &DispatchLexRightParen;
  table['['] = &DispatchLexLeftBracket;
  table[']'] = &DispatchLexRightBracket;
  table['{'] = &DispatchLexLeftBrace;
  table['}'] = &DispatchLexRightBrace;
  table[';'] = &DispatchLexSemiColon;
  table['!'] = &DispatchLexBang;
  table['='] = &DispatchLexEqual;
  table['>'] = &DispatchLexGreater;
  table['<'] = &DispatchLexLesser;
  table[','] = &DispatchLexComma;

  // LeftBrace,
  // RightBrace,

  for (unsigned char c = 'a'; c <= 'z'; ++c) {
    table[c] = &DispatchLexIdentifier;
  }

  for (unsigned char c = 'A'; c <= 'Z'; ++c) {
    table[c] = &DispatchLexIdentifier;
  }

  for (unsigned char c = '0'; c <= '9'; ++c) {
    table[c] = &DispatchLexNumber;
  }

  return table;
}();

void Lexer::Scan() {
  const char c = Advance();
  kDispatchTable[c](*this);
}

void Lexer::PushToken(Token::Kind kind) {
  tokens_.push_back({kind, start_, current_ - start_, line_});
}

void Lexer::OpAssign(Token::Kind e, Token::Kind f) {
  if (Match('=')) {
    PushToken(e);
  } else {
    PushToken(f);
  }
}

char Lexer::Advance() {
  if (!IsEnd()) {
    current_ += 1;
    return source_[current_ - 1];
  }
  return '\0';
}

bool Lexer::Match(char c) {
  if (Peek() == c) {
    current_ += 1;
    return true;
  }
  return false;
}

char Lexer::Peek() {
  if (!IsEnd()) {
    return source_[current_];
  }
  return '\0';
}

char Lexer::PeekNext() {
  if (!IsEnd()) {
    return source_[current_ + 1];
  }
  return '\0';
}

bool Lexer::InNumber() {
  return !IsEnd() && std::isdigit(static_cast<unsigned char>(Peek()));
}

bool Lexer::IsEnd() { return current_ >= source_.length(); }

std::string_view Lexer::Slice() {
  int len = current_ - start_;
  return source_.substr(start_, len);
}

char Lexer::LookBack() { return source_[current_ - 1]; }
