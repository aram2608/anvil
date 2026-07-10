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
    IntLiteral,
    FloatLiteral,
    StringLiteral,
    TrueLiteral,
    FalseLiteral,

    // Other
    Colon,
    SemiColon,
    Error,
    Identifier,
    EndOfFile,
    Dot,
    Comma,

    // Control flow
    If,
    For,
    Else,
    While,
    Return,

    FuncDecl,

    LeftParen,
    RightParen,
    LeftBracket,
    RightBracket,
    LeftBrace,
    RightBrace,
    _Count,
  };

  Token(Kind kind, int start, int len, int line)
      : kind{kind}, start{start}, len{len}, line{line} {}

  Kind kind;
  int start;
  int len;
  int line;
};

#endif
