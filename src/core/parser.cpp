#include "parser/parser.hpp"
#include "ast/ast.hpp"
#include "ast/node.hpp"
#include "token/token.hpp"
#include <cassert>
#include <cstddef>

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

OprData GetPrec(Token::Kind kind) {
  return kPrecs[static_cast<std::size_t>(kind)];
}

} // namespace

Parser::Parser(std::string source, std::vector<Token> tokens)
    : source_{source}, tokens_{tokens}, current_{0} {}

Ast Parser::Parse() {
  ParseRoot();
  return {source_, tokens_, NodeBuffer{nodes_}, extra_data_, errors_};
}

void Parser::ParseRoot() {
  Node::Index root = AddNode({
      .kind = Node::Kind::Root,
      .main_token = {0},
      .data = std::monostate{},
  });

  const int scratch_top = scratch_.size();

  while (!IsEnd()) {
    ParseStatements();
  }

  nodes_[root].data = CommitScratch(scratch_top);
}

// stmts <- (SEMICOLON | block | expr)* EOF
void Parser::ParseStatements() {
  switch (TokenKind(current_)) {
  case Token::Kind::LeftBrace:
    // TOOD: Add blocks
    break;
  default:
    // TODO: A bit of a hack to get expressions going
    // fix later on
    scratch_.push_back(ParseExpression());
    break;
  }
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
  case Token::Kind::Int:
    return AddNode({
        .kind = Node::Kind::Int,
        .main_token = Advance(),
        .data = std::monostate{},
    });
    break;
  case Token::Kind::Float:
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
  unsigned int start = static_cast<unsigned int>(extra_data_.size());
  extra_data_.reserve(extra_data_.size() + items.size());

  for (const auto &idx : items) {
    extra_data_.push_back(static_cast<uint32_t>(idx));
  }

  unsigned int end = static_cast<unsigned int>(extra_data_.size());
  scratch_.resize(top);
  return Node::ExtraRange{.start = start, .end = end};
}

Node::TokenIndex Parser::Advance() {
  current_ += 1;
  return {current_ - 1};
}

Node::TokenIndex Parser::Previous() { return {current_ - 1}; }

Node::Index Parser::AddNode(Node node) {
  nodes_.push_back(node);
  return {static_cast<unsigned int>(nodes_.size() - 1)};
}

bool Parser::IsEnd() {
  return tokens_.Kinds()[current_] == Token::Kind::EndOfFile;
}

Token::Kind Parser::TokenKind(const unsigned int idx) {
  assert(tokens_.Kinds().size() > idx);
  return tokens_.Kinds()[idx];
}
