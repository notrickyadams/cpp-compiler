#pragma once
#include "Token.hpp"
#include "../diagnostics/Diagnostic.hpp"
#include "../diagnostics/DiagnosticEngine.hpp"
#include "../diagnostics/TraceRecorder.hpp"
#include <string>
#include <vector>

// ============================================================
//  Lexer v2 — integrated with the diagnostic system.
//
//  Changes from v1:
//    • LexError removed — Diagnostic is used throughout
//    • tokenize() returns StageOutput<vector<Token>>
//      (output + diagnostics in one return value)
//    • DiagnosticEngine owned by Lexer (created per instance)
//    • Unterminated block comment now properly diagnosed
//
//  The public API remains small and clear:
//    tokenize()  — scan everything, return output + diagnostics
//
//  Backward-compat helpers for tests:
//    hasErrors() / errors() delegate to the last StageOutput
// ============================================================
class Lexer {
public:
    explicit Lexer(std::string source);

    // Primary API — returns tokens + all diagnostics together
    StageOutput<std::vector<Token>> tokenize();

    // Backward-compatible helpers (delegate to last run's output)
    bool hasErrors() const { return lastOutput_.hasErrors(); }
    const std::vector<Diagnostic>& errors() const {
        return lastOutput_.diagnostics;
    }

private:
    // ── Core scanning ─────────────────────────────────────
    Token  nextToken();
    void   skipWhitespaceAndComments();
    Token  lexIdentifierOrKeyword();
    Token  lexNumber();
    Token  lexSymbol();

    // ── Character primitives ──────────────────────────────
    char   peek()        const;
    char   peekNext()    const;
    char   advance();
    bool   isAtEnd()     const;
    bool   match(char expected);

    // ── Token / diagnostic constructors ───────────────────
    Token  makeToken(TokenType type, const std::string& lexeme,
                     int startLine, int startCol) const;
    Token  errorToken(char c);   // emits diagnostic + returns UNKNOWN token

    // ── State ─────────────────────────────────────────────
    std::string          source_;
    std::size_t          pos_;
    int                  line_;
    int                  col_;

    DiagnosticEngine     engine_;   // stateless factory
    TraceRecorder        recorder_{"Lexer"};  // records the live scan path
    std::vector<Diagnostic> pendingDiagnostics_;  // accumulated during scan

    // Stored result from last tokenize() call (for compat helpers)
    StageOutput<std::vector<Token>> lastOutput_;
};
