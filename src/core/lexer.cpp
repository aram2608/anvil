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

using namespace Anvil;

namespace {

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
        {"fn", Token::Kind::FuncLiteral},
        {"else", Token::Kind::Else},
        {"return", Token::Kind::Return},
        {"true", Token::Kind::TrueLiteral},
        {"false", Token::Kind::FalseLiteral},
        {"or", Token::Kind::Or},
        {"and", Token::Kind::And},
    })};

} // namespace

Lexer::Lexer(std::string_view source) : source_{source} {}

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

void Anvil::DispatchLexError(Lexer &lex) { lex.PushToken(Token::Kind::Error); }

void Anvil::DispatchLexSlash(Lexer &lex) {
  if (lex.Peek() == '/') {
    while (!lex.IsEnd() && lex.Peek() != '\n') {
      lex.Advance();
    }
  } else {
    lex.OpAssign(Token::Kind::SlashEqual, Token::Kind::Slash);
  }
}

void Anvil::DispatchLexPlus(Lexer &lex) {
  lex.OpAssign(Token::Kind::PlusEqual, Token::Kind::Plus);
}

void Anvil::DispatchLexStar(Lexer &lex) {
  lex.OpAssign(Token::Kind::StarEqual, Token::Kind::Star);
}

void Anvil::DispatchLexMinus(Lexer &lex) {
  lex.OpAssign(Token::Kind::MinusEqual, Token::Kind::Minus);
}

void Anvil::DispatchLexColon(Lexer &lex) {
  lex.OpAssign(Token::Kind::ColonEqual, Token::Kind::Colon);
}

void Anvil::DispatchLexIdentifier(Lexer &lex) {
  while (!lex.IsEnd() && std::isalnum(static_cast<unsigned char>(lex.Peek()))) {
    lex.Advance();
  }
  auto kind = kKeyWords.at(lex.Slice());
  lex.PushToken(kind.value_or(Token::Kind::Identifier));
}

void Anvil::DispatchLexNumber(Lexer &lex) {
  while (lex.InNumber()) {
    lex.Advance();
  }
  if (lex.Match('.')) {
    while (lex.InNumber()) {
      lex.Advance();
    }
    lex.PushToken(Token::Kind::FloatLiteral);
  } else {
    lex.PushToken(Token::Kind::IntLiteral);
  }
}

void Anvil::DispatchLexDot(Lexer &lex) { lex.PushToken(Token::Kind::Dot); }

void Anvil::DispatchLexWhiteSpace(Lexer &lex) {}

void Anvil::DispatchLexNewLine(Lexer &lex) { lex.line_ += 1; }

void Anvil::DispatchLexLeftParen(Lexer &lex) {
  lex.PushToken(Token::Kind::LeftParen);
}

void Anvil::DispatchLexLeftBracket(Lexer &lex) {
  lex.PushToken(Token::Kind::LeftBracket);
}

void Anvil::DispatchLexLeftBrace(Lexer &lex) {
  lex.PushToken(Token::Kind::LeftBrace);
}

void Anvil::DispatchLexRightParen(Lexer &lex) {
  lex.PushToken(Token::Kind::RightParen);
}

void Anvil::DispatchLexRightBracket(Lexer &lex) {
  lex.PushToken(Token::Kind::RightBracket);
}

void Anvil::DispatchLexRightBrace(Lexer &lex) {
  lex.PushToken(Token::Kind::RightBrace);
}

void Anvil::DispatchLexBang(Lexer &lex) {
  lex.OpAssign(Token::Kind::BangEqual, Token::Kind::Bang);
}

void Anvil::DispatchLexSemiColon(Lexer &lex) {
  lex.PushToken(Token::Kind::SemiColon);
}

void Anvil::DispatchLexEqual(Lexer &lex) {
  lex.OpAssign(Token::Kind::EqualEqual, Token::Kind::Equal);
}

void Anvil::DispatchLexLesser(Lexer &lex) {
  lex.OpAssign(Token::Kind::LesserEqual, Token::Kind::Lesser);
}

void Anvil::DispatchLexGreater(Lexer &lex) {
  lex.OpAssign(Token::Kind::GreaterEqual, Token::Kind::Greater);
}

void Anvil::DispatchLexDoubleQuote(Lexer &lex) {
  while (!lex.IsEnd() && lex.Peek() != '"') lex.Advance();

  if (lex.IsEnd()) {
    lex.PushToken(Token::Kind::Error); // unterminated string
    return;
  }

  lex.Advance();
  lex.PushToken(Token::Kind::StringLiteral);
}

void Anvil::DispatchLexComma(Lexer &lex) { lex.PushToken(Token::Kind::Comma); }

void Anvil::DispatchLexTilde(Lexer &lex) { lex.PushToken(Token::Kind::Tilde); }

void Anvil::DispatchLexAmp(Lexer &lex) {
  lex.PushToken(Token::Kind::Ampersand);
}

void Anvil::DispatchLexAt(Lexer &lex) {
  while (!lex.IsEnd() && std::isalnum(lex.Peek())) lex.Advance();
  lex.PushToken(Token::Kind::AtCall);
}

void Anvil::DispatchLexCaret(Lexer &lex) { lex.PushToken(Token::Kind::Caret); }

void Anvil::DispatchLexBar(Lexer &lex) { lex.PushToken(Token::Kind::Bar); }

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
  table['"'] = &DispatchLexDoubleQuote;
  table['~'] = &DispatchLexTilde;
  table['&'] = &DispatchLexAmp;
  table['^'] = &DispatchLexCaret;
  table['|'] = &DispatchLexBar;
  table['@'] = &DispatchLexAt;

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
  if (current_ + 1 < source_.length()) {
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
