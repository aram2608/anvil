#ifndef TOKEN_HPP_
#define TOKEN_HPP_

struct Token {
  enum class Kind {
    // Ops
    Plus,
    Minus,
    Slash,
    Star,
    Equal,
    Bang,
    ColonEqual,
    PlusEqual,
    MinusEqual,
    SlashEqual,
    StarEqual,
    // Cmp
    EqualEqual,
    BangEqual,
    Lesser,
    LesserEqual,
    Greater,
    GreaterEqual,

    // Types
    Nil,
    Int,
    Float,
    String,

    // Other
    Colon,
    SemiColon,
    Error,
    Identifier,
    EndOfFile,
    Dot,
    If,
    For,
    Else,
    While,

    LeftParen,
    RightParen,
    LeftBracket,
    RightBracket,
    LeftBrace,
    RightBrace,
    _Count,
  };

  Token(Kind kind, int start, int len) : kind{kind}, start{start}, len{len} {}

  Kind kind;
  int start;
  int len;
};

#endif
