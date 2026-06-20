#include "DiagnosticEngine.hpp"
#include <string>

// ─────────────────────────────────────────────────────────────
//  Private: shared assembly helper
// ─────────────────────────────────────────────────────────────
Diagnostic DiagnosticEngine::build(DiagnosticKind kind,
                                    Severity severity,
                                    SourceSpan span,
                                    const std::string& message,
                                    const std::string& detail) const {
    Diagnostic d;
    d.kind        = kind;
    d.severity    = severity;
    d.span        = span;
    d.message     = message;
    d.rootCause   = ExplanationBuilder::rootCause(kind, detail);
    d.explanation = ExplanationBuilder::explain(kind, detail);
    d.fixes       = ExplanationBuilder::fixes(kind, detail);
    d.trace       = ExplanationBuilder::trace(kind);
    return d;
}

// ─────────────────────────────────────────────────────────────
//  Lexer diagnostics
// ─────────────────────────────────────────────────────────────

Diagnostic DiagnosticEngine::unexpectedChar(char c, SourceSpan span) const {
    std::string ch(1, c);
    return build(
        DiagnosticKind::LEX_UnexpectedCharacter,
        Severity::Error,
        span,
        "unexpected character '" + ch + "'",
        ch
    );
}

Diagnostic DiagnosticEngine::unterminatedComment(SourceSpan span) const {
    return build(
        DiagnosticKind::LEX_UnterminatedBlockComment,
        Severity::Error,
        span,
        "unterminated block comment",
        ""
    );
}

Diagnostic DiagnosticEngine::unterminatedString(SourceSpan span) const {
    return build(
        DiagnosticKind::LEX_UnterminatedString,
        Severity::Error,
        span,
        "unterminated string literal",
        ""
    );
}

Diagnostic DiagnosticEngine::invalidNumberLiteral(const std::string& lit,
                                                    SourceSpan span) const {
    return build(
        DiagnosticKind::LEX_InvalidNumberLiteral,
        Severity::Error,
        span,
        "invalid number literal '" + lit + "'",
        lit
    );
}

// ─────────────────────────────────────────────────────────────
//  Parser diagnostics (stubs — wired up fully in Stage 2)
// ─────────────────────────────────────────────────────────────

Diagnostic DiagnosticEngine::unexpectedToken(const std::string& found,
                                              const std::string& expected,
                                              SourceSpan span) const {
    return build(
        DiagnosticKind::PARSE_UnexpectedToken,
        Severity::Error,
        span,
        "expected '" + expected + "', found '" + found + "'",
        found
    );
}

Diagnostic DiagnosticEngine::missingToken(const std::string& expected,
                                           SourceSpan span) const {
    return build(
        DiagnosticKind::PARSE_MissingToken,
        Severity::Error,
        span,
        "missing '" + expected + "'",
        expected
    );
}

// ─────────────────────────────────────────────────────────────
//  Semantic diagnostics (stubs — wired up fully in Stage 4)
// ─────────────────────────────────────────────────────────────

Diagnostic DiagnosticEngine::typeMismatch(const std::string& left,
                                           const std::string& right,
                                           SourceSpan span) const {
    return build(
        DiagnosticKind::SEM_TypeMismatch,
        Severity::Error,
        span,
        "type mismatch: '" + left + "' and '" + right + "'",
        left + " and " + right
    );
}

Diagnostic DiagnosticEngine::undeclaredIdentifier(const std::string& name,
                                                    SourceSpan span) const {
    return build(
        DiagnosticKind::SEM_UndeclaredIdentifier,
        Severity::Error,
        span,
        "undeclared identifier '" + name + "'",
        name
    );
}

Diagnostic DiagnosticEngine::redeclaredVariable(const std::string& name,
                                                  SourceSpan span) const {
    return build(
        DiagnosticKind::SEM_RedeclaredVariable,
        Severity::Error,
        span,
        "'" + name + "' is already declared in this scope",
        name
    );
}
