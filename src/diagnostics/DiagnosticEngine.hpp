#pragma once
#include "Diagnostic.hpp"
#include "ExplanationBuilder.hpp"
#include <string>

// ============================================================
//  DiagnosticEngine — factory for Diagnostic objects.
//
//  Each compiler stage holds or calls the DiagnosticEngine
//  whenever something goes wrong. The engine:
//    1. Receives raw context (char, span, token name, etc.)
//    2. Delegates to ExplanationBuilder for all prose
//    3. Returns a complete Diagnostic
//
//  Why separate from ExplanationBuilder?
//    ExplanationBuilder is a KNOWLEDGE BASE (static text).
//    DiagnosticEngine is a FACTORY (takes runtime values,
//    builds the struct). One is about what to say;
//    the other is about assembling the final object.
//
//  Usage:
//    DiagnosticEngine engine;
//    Diagnostic d = engine.unexpectedChar('@', span);
// ============================================================
class DiagnosticEngine {
public:
    // ── Lexer diagnostics ─────────────────────────────────

    Diagnostic unexpectedChar(char c, SourceSpan span) const;

    Diagnostic unterminatedComment(SourceSpan span) const;

    Diagnostic unterminatedString(SourceSpan span) const;

    Diagnostic invalidNumberLiteral(const std::string& literal,
                                    SourceSpan span) const;

    // ── Parser diagnostics (stubs — implemented in Stage 2) ──

    Diagnostic unexpectedToken(const std::string& found,
                                const std::string& expected,
                                SourceSpan span) const;

    Diagnostic missingToken(const std::string& expected,
                             SourceSpan span) const;

    // ── Semantic diagnostics (stubs — implemented in Stage 4) ─

    Diagnostic typeMismatch(const std::string& leftType,
                             const std::string& rightType,
                             SourceSpan span) const;

    Diagnostic undeclaredIdentifier(const std::string& name,
                                     SourceSpan span) const;

    Diagnostic redeclaredVariable(const std::string& name,
                                   SourceSpan span) const;

private:
    // Shared assembly helper
    Diagnostic build(DiagnosticKind kind,
                      Severity severity,
                      SourceSpan span,
                      const std::string& message,
                      const std::string& detail) const;
};
