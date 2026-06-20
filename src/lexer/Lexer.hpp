#pragma once
#include "Token.hpp"
#include <string>
#include <vector>

// ============================================================
//  LexError — structured error reported by the lexer.
//
//  Separate from std::exception so the caller can collect ALL
//  errors in one pass instead of stopping at the first one.
//  (GCC's -fmax-errors and Clang's -ferror-limit do the same.)
// ============================================================
struct LexError {
    std::string message;
    int         line;
    int         column;

    std::string toString() const {
        return "[LexError] line " + std::to_string(line) +
               ", col "          + std::to_string(column) +
               ": "              + message;
    }
};

// ============================================================
//  Lexer — transforms a source string into a token stream.
//
//  Public API (minimal, intentional):
//    tokenize()   — scan entire source, return all tokens
//    hasErrors()  — true if any LexErrors were recorded
//    errors()     — the error list
//
//  Design decisions:
//    • Single-pass, O(n) — one character consumed per iteration
//    • Cursor-based (pos_ / line_ / col_) — no regex, no tables
//    • Non-throwing — errors collected, not thrown
//    • const-correct — source held by value (owned)
//
//  Alternatives considered:
//    • Table-driven DFA — faster but much harder to read/maintain
//    • Flex/re2c generated — industry standard, but hides logic
//    • Regex-based — simple but 10-100× slower for large files
// ============================================================
class Lexer {
public:
    explicit Lexer(std::string source);

    // Tokenise the entire source and return the token stream.
    // Always ends with an END_OF_FILE token.
    std::vector<Token> tokenize();

    bool                     hasErrors() const { return !errors_.empty(); }
    const std::vector<LexError>& errors() const { return errors_; }

private:
    // ── Core scanning helpers ─────────────────────────────
    Token  nextToken();

    void   skipWhitespaceAndComments();

    Token  lexIdentifierOrKeyword();
    Token  lexNumber();
    Token  lexSymbol();

    // ── Character-level primitives ────────────────────────
    char   peek()              const;   // look at current char
    char   peekNext()          const;   // look one ahead
    char   advance();                   // consume & return current char
    bool   isAtEnd()           const;
    bool   match(char expected);        // consume only if matches

    // ── Token constructors ────────────────────────────────
    Token  makeToken(TokenType type, const std::string& lexeme) const;
    Token  errorToken(const std::string& msg);

    // ── State ─────────────────────────────────────────────
    std::string          source_;
    std::size_t          pos_;     // current position in source_
    int                  line_;    // current line (1-based)
    int                  col_;     // current column (1-based)

    std::vector<LexError> errors_;
};
