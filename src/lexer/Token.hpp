#pragma once
#include "../core/SourceSpan.hpp"
#include "../core/Json.hpp"
#include <string>
#include <unordered_map>

// ============================================================
//  TokenType — every token kind the lexer can produce.
// ============================================================
enum class TokenType {
    // ── Literals ─────────────────────────────────────────
    INTEGER_LITERAL,

    // ── Identifiers ──────────────────────────────────────
    IDENTIFIER,

    // ── Keywords ─────────────────────────────────────────
    KW_INT,
    KW_RETURN,

    // ── Operators ────────────────────────────────────────
    PLUS,
    MINUS,
    STAR,
    SLASH,
    ASSIGN,
    EQUAL,
    NOT_EQUAL,
    LESS,
    GREATER,

    // ── Delimiters ───────────────────────────────────────
    SEMICOLON,
    LPAREN,
    RPAREN,
    LBRACE,
    RBRACE,
    COMMA,

    // ── Special ──────────────────────────────────────────
    END_OF_FILE,
    UNKNOWN
};

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
//  v2 change: 'line' and 'column' are kept as direct fields
//  for backward compatibility with existing tests. A span()
//  accessor derives the full SourceSpan for the diagnostic
//  system. Future stages will migrate to span() directly.
// ============================================================
struct Token {
    TokenType   type;
    std::string lexeme;
    int         line;    // 1-based start line
    int         column;  // 1-based start column

    // Derives a SourceSpan from the token's position + lexeme length.
    SourceSpan span() const {
        int len = lexeme.empty() ? 1 : (int)lexeme.size();
        return SourceSpan::token(line, column, len);
    }

    std::string toString() const {
        std::string s = tokenTypeName(type);
        if (!lexeme.empty() &&
            (type == TokenType::IDENTIFIER || type == TokenType::INTEGER_LITERAL)) {
            s += "(" + lexeme + ")";
        }
        return s;
    }

    // For the Stage 8 visualizer: line/column let the frontend
    // highlight the exact source span a token came from.
    std::string toJson() const {
        return "{\"type\":\"" + jsonEscape(tokenTypeName(type)) +
               "\",\"lexeme\":\"" + jsonEscape(lexeme) +
               "\",\"line\":" + std::to_string(line) +
               ",\"column\":" + std::to_string(column) + "}";
    }
};

// ============================================================
//  Keyword table
// ============================================================
inline const std::unordered_map<std::string, TokenType>& keywords() {
    static const std::unordered_map<std::string, TokenType> table = {
        { "int",    TokenType::KW_INT    },
        { "return", TokenType::KW_RETURN },
    };
    return table;
}
