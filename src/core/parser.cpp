#include "parser/parser.hpp"
#include "ast/ast.hpp"
#include "ast/node.hpp"
#include "token/token.hpp"
#include <cassert>
#include <cstddef>
#include <cstdint>

namespace {

struct OprData {
  int8_t left = -1;
  int8_t right = -1;
  Node::Kind kind = Node::Kind::Root;
};

constexpr auto MakePrecedenceTable() {
  std::array<OprData, static_cast<size_t>(Token::Kind::_Count)> table{};

  auto set = [&](Token::Kind token, OprData data) {
    table[static_cast<size_t>(token)] = data;
  };

  set(Token::Kind::Equal, {10, 9, Node::Kind::Reassign});
  set(Token::Kind::ColonEqual, {11, 10, Node::Kind::Assign});
  set(Token::Kind::EqualEqual, {40, 41, Node::Kind::Equal});
  set(Token::Kind::BangEqual, {40, 41, Node::Kind::NotEqual});
  set(Token::Kind::Lesser, {50, 51, Node::Kind::LesserThan});
  set(Token::Kind::Greater, {50, 51, Node::Kind::GreaterThan});
  set(Token::Kind::LesserEqual, {50, 51, Node::Kind::LesserEqual});
  set(Token::Kind::GreaterEqual, {50, 51, Node::Kind::GreaterEqual});
  set(Token::Kind::Plus, {60, 61, Node::Kind::Add});
  set(Token::Kind::Minus, {60, 61, Node::Kind::Sub});
  set(Token::Kind::Star, {70, 71, Node::Kind::Mult});
  set(Token::Kind::Slash, {70, 71, Node::Kind::Div});

  return table;
}

inline constexpr auto kPrecs = MakePrecedenceTable();

OprData GetPrec(Token::Kind kind) { return kPrecs[static_cast<size_t>(kind)]; }

} // namespace

Parser::Parser(std::string_view source, std::vector<Token> tokens)
    : source_{source}, tokens_{tokens}, current_{0} {}

Ast Parser::Parse() {
  ParseRoot();
  return {source_, tokens_, NodeBuffer{nodes_}, extra_data_, errors_};
}

void Parser::ParseRoot() {
  Node::Index root = AddNode({
      .kind = Node::Kind::Root,
      .main_token = Node::TokenIndex{0},
      .data = std::monostate{},
  });

  const int scratch_top = scratch_.size();

  while (!IsEnd()) {
    scratch_.push_back(ParseStatements());
  }

  nodes_[ToU32(root)].data = CommitScratch(scratch_top);
}

// stmts <- (SEMICOLON | block | expr)* EOF
Node::Index Parser::ParseStatements() {
  switch (TokenKind(current_)) {
  case Token::Kind::LeftBrace:
    return ParseBlock();
  case Token::Kind::If:
    return ParseIf();
  case Token::Kind::Return:
    return ParseReturn();
  default:
    return ParseExpressionStatement();
  }
}

Node::Index Parser::ParseExpressionStatement() {
  // We need to keep track of errors while parsing downstream.
  // If the errors_ vector grows, an error was encountered inside
  // ParseExpression itself. This means that we don't need to add the missing
  // semicolon error since at that point its just frivolous.
  const size_t errs = errors_.size();
  Node::Index expr = ParseExpression();
  ExpectSemicolon(errs);
  return expr;
}

Node::Index Parser::ParseBlock() {
  Node::TokenIndex left_brace = EatToken(Token::Kind::LeftBrace);
  auto top = scratch_.size();
  while (true) {
    if (TokenKind(current_) == Token::Kind::RightBrace || IsEnd()) break;
    scratch_.push_back(ParseStatements());
  }
  if (EatToken(Token::Kind::RightBrace) == Node::TokenIndex::None) {
    errors_.push_back({
        .kind = ParseError::Kind::MissingClosingBrace,
        .token = Previous(),
    });
  }
  return AddNode({
      .kind = Node::Kind::Block,
      .main_token = left_brace,
      .data = CommitScratch(top),
  });
}

Node::Index Parser::ParseReturn() {
  Node::TokenIndex return_token = EatToken(Token::Kind::Return);
  const auto top = scratch_.size();
  const size_t errs = errors_.size();

  scratch_.push_back(ParseExpression());
  while (EatToken(Token::Kind::Comma) != Node::TokenIndex::None) {
    scratch_.push_back(ParseExpression());
  }

  ExpectSemicolon(errs);

  if (scratch_.size() > top + 1) {
    // TODO: Implement tuple returns once tuples are in the language
    assert(0 && "Unreachable at ParseReturn.");
    return AddNode({
        .kind = Node::Kind::ReturnMulti,
        .main_token = return_token,
        .data = CommitScratch(top),
    });
  }

  Node::Index expr = scratch_.back();
  scratch_.pop_back();

  return AddNode({
      .kind = Node::Kind::ReturnSimple,
      .main_token = return_token,
      .data = expr,
  });
}

// if_stmt <- KEYWORD_IF expr stmt* (KEYWORD_ELSE stmt*)?
Node::Index Parser::ParseIf() {
  Node::TokenIndex if_token = EatToken(Token::Kind::If);
  Node::Index condition = ParseExpression(); // add an error check here

  Node::Index then_branch = ParseStatements();

  if (EatToken(Token::Kind::Else) == Node::TokenIndex::None) {
    return AddNode({
        .kind = Node::Kind::IfSimple,
        .main_token = if_token,
        .data =
            Node::NodeAndNode{
                condition,
                then_branch,
            },
    });
  }

  Node::Index else_branch = ParseStatements();

  return AddNode({
      .kind = Node::Kind::IfFull,
      .main_token = if_token,
      .data =
          Node::NodeAndExtra{
              condition,
              AddExtra(Node::IfExtra{
                  .then_expr = then_branch,
                  .else_expr = else_branch,
              }),
          },
  });
}

// expr <- prefix (binop expr)*
Node::Index Parser::ParseExpression() { return ParseExpressionPrecedence(0); }

Node::Index Parser::ParseExpressionPrecedence(const int min) {
  Node::Index node = ParsePrefix();

  while (true) {
    const auto kind = TokenKind(current_);
    const OprData data = GetPrec(kind);
    if (data.left < min) {
      break;
    }

    const auto op = Advance();
    const auto rhs = ParseExpressionPrecedence(data.right);

    node = AddNode({
        .kind = data.kind,
        .main_token = op,
        .data = Node::NodeAndNode{.first = node, .second = rhs},
    });
  }

  return node;
}

// prefix <- (BANG | MINUS)* atom
Node::Index Parser::ParsePrefix() {
  switch (TokenKind(current_)) {
  case Token::Kind::Bang:
    return AddNode({
        .kind = Node::Kind::BoolNot,
        .main_token = Advance(),
        .data = Node::Index{ParsePrefix()},
    });
    break;
  case Token::Kind::Minus:
    return AddNode({
        .kind = Node::Kind::Negate,
        .main_token = Advance(),
        .data = Node::Index{ParsePrefix()},
    });
    break;
  default:
    return ParseAtom();
    break;
  }
}

Node::Index Parser::ParseAtom() {
  switch (TokenKind(current_)) {
  case Token::Kind::IntLiteral:
    return AddNode({
        .kind = Node::Kind::Int,
        .main_token = Advance(),
        .data = std::monostate{},
    });
    break;
  case Token::Kind::FloatLiteral:
    return AddNode({
        .kind = Node::Kind::Float,
        .main_token = Advance(),
        .data = std::monostate{},
    });
    break;
  default:
    errors_.push_back({
        .kind = ParseError::Kind::UnexpectedToken,
        .token = Advance(),
    });
    return AddNode({
        .kind = Node::Kind::Undefined,
        .main_token = Previous(),
        .data = std::monostate{},
    });
  }
}

Node::ExtraRange Parser::CommitScratch(const int top) {
  std::span<const Node::Index> items{scratch_.begin() + top, scratch_.end()};
  uint32_t start = static_cast<uint32_t>(extra_data_.size());
  extra_data_.reserve(extra_data_.size() + items.size());

  for (const auto &idx : items) {
    extra_data_.push_back(ToU32(idx));
  }

  uint32_t end = static_cast<uint32_t>(extra_data_.size());
  scratch_.resize(top);
  return Node::ExtraRange{.start = start, .end = end};
}

Node::TokenIndex Parser::Advance() {
  if (Peek() == Token::Kind::EndOfFile) {
    return Node::TokenIndex{current_};
  }
  current_ += 1;
  return Node::TokenIndex{current_ - 1};
}

Token::Kind Parser::Peek() {
  assert(tokens_.Kinds().size() > current_);
  return tokens_.Kinds()[current_];
}

Node::TokenIndex Parser::Previous() { return Node::TokenIndex{current_ - 1}; }

Node::Index Parser::AddNode(Node node) {
  nodes_.push_back(node);
  return Node::Index{static_cast<uint32_t>(nodes_.size() - 1)};
}

Node::TokenIndex Parser::EatToken(Token::Kind kind) {
  if (TokenKind(current_) == kind) {
    return Advance();
  }
  return Node::TokenIndex::None;
}

void Parser::ExpectSemicolon(const size_t errs) {
  if (EatToken(Token::Kind::SemiColon) == Node::TokenIndex::None) {
    if (errors_.size() == errs) {
      errors_.push_back({
          .kind = ParseError::Kind::MissingSemicolon,
          .token = Previous(),
      });
    }
    Synchronize();
  }
}

bool Parser::IsEnd() {
  return tokens_.Kinds()[current_] == Token::Kind::EndOfFile;
}

Token::Kind Parser::TokenKind(const uint32_t idx) {
  assert(tokens_.Kinds().size() > idx);
  return tokens_.Kinds()[idx];
}

Node::ExtraIndex Parser::AddExtra(Node::IfExtra extra) {
  uint32_t result = static_cast<uint32_t>(extra_data_.size());

  // Serialized sequentially
  PushExtraValue(extra.then_expr);
  PushExtraValue(extra.else_expr);

  return Node::ExtraIndex{result};
}

void Parser::Synchronize() {
  while (true) {
    if (IsEnd()) return;
    switch (Peek()) {
    case Token::Kind::SemiColon:
      Advance();
      return;
    case Token::Kind::RightBrace:
      return;
    default:
      Advance();
      break;
    }
  }
}
