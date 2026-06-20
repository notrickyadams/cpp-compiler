#pragma once
#include <string>
#include <unordered_map>

// ============================================================
//  TokenType — every token kind the lexer can produce.
//
//  Design note: using a strongly-typed enum (enum class) so
//  TokenType::PLUS can never silently convert to an int.
//  Industry reference: clang/Basic/TokenKinds.def uses X-macros
//  for the same purpose; we keep it simple for clarity.
// ============================================================

enum class TokenType {
    // ── Literals ─────────────────────────────────────────
    INTEGER_LITERAL,     // 42, 0, 123

    // ── Identifiers ──────────────────────────────────────
    IDENTIFIER,          // x, myVar, main

    // ── Keywords ─────────────────────────────────────────
    KW_INT,              // int
    KW_RETURN,           // return

    // ── Operators ────────────────────────────────────────
    PLUS,                // +
    MINUS,               // -
    STAR,                // *
    SLASH,               // /
    ASSIGN,              // =
    EQUAL,               // ==
    NOT_EQUAL,           // !=
    LESS,                // <
    GREATER,             // >

    // ── Delimiters ───────────────────────────────────────
    SEMICOLON,           // ;
    LPAREN,              // (
    RPAREN,              // )
    LBRACE,              // {
    RBRACE,              // }
    COMMA,               // ,

    // ── Special ──────────────────────────────────────────
    END_OF_FILE,
    UNKNOWN              // anything unrecognised — reported as error
};

// ============================================================
//  tokenTypeName — human-readable name for diagnostics/tests.
//  Keeping this as a free function (not a method) lets us use
//  it from anywhere without pulling in the full Lexer header.
// ============================================================
inline std::string tokenTypeName(TokenType t) {
    switch (t) {
        case TokenType::INTEGER_LITERAL: return "INTEGER_LITERAL";
        case TokenType::IDENTIFIER:      return "IDENTIFIER";
        case TokenType::KW_INT:          return "INT";
        case TokenType::KW_RETURN:       return "RETURN";
        case TokenType::PLUS:            return "PLUS";
        case TokenType::MINUS:           return "MINUS";
        case TokenType::STAR:            return "STAR";
        case TokenType::SLASH:           return "SLASH";
        case TokenType::ASSIGN:          return "ASSIGN";
        case TokenType::EQUAL:           return "EQUAL";
        case TokenType::NOT_EQUAL:       return "NOT_EQUAL";
        case TokenType::LESS:            return "LESS";
        case TokenType::GREATER:         return "GREATER";
        case TokenType::SEMICOLON:       return "SEMICOLON";
        case TokenType::LPAREN:          return "LPAREN";
        case TokenType::RPAREN:          return "RPAREN";
        case TokenType::LBRACE:          return "LBRACE";
        case TokenType::RBRACE:          return "RBRACE";
        case TokenType::COMMA:           return "COMMA";
        case TokenType::END_OF_FILE:     return "EOF";
        case TokenType::UNKNOWN:         return "UNKNOWN";
        default:                         return "?";
    }
}

// ============================================================
//  Token — the fundamental unit the lexer emits.
//
//  Holds:
//    type    — what kind of token this is
//    lexeme  — the exact source text ("return", "42", "+")
//    line    — 1-based line number for error messages
//    column  — 1-based column number
//
//  Industry note: Clang's Token is 16 bytes and uses bit-packing
//  for flags. Ours is intentionally readable — the compiler we
//  build here prioritises clarity over micro-optimisation.
// ============================================================
struct Token {
    TokenType   type;
    std::string lexeme;   // exact text from source
    int         line;
    int         column;

    // Convenience: pretty-print for debugging/tests
    std::string toString() const {
        std::string s = tokenTypeName(type);
        if (!lexeme.empty() &&
            (type == TokenType::IDENTIFIER || type == TokenType::INTEGER_LITERAL)) {
            s += "(" + lexeme + ")";
        }
        return s;
    }
};

// ============================================================
//  Keyword table — maps source text → TokenType.
//
//  Kept here (next to Token) so anyone adding a keyword touches
//  one place. Alternative: generate this from a .def file (GCC/
//  Clang approach) to avoid duplication at scale.
// ============================================================
inline const std::unordered_map<std::string, TokenType>& keywords() {
    static const std::unordered_map<std::string, TokenType> table = {
        { "int",    TokenType::KW_INT    },
        { "return", TokenType::KW_RETURN },
    };
    return table;
}
