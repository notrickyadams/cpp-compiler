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

    // nodeType: the nodeType() of whatever was on the left of '='
    // (e.g. "BinaryExpr", "IntLiteral") — anything other than "Identifier".
    Diagnostic invalidAssignmentTarget(const std::string& nodeType,
                                        SourceSpan span) const;

    // leftText: source text of the expression already parsed (e.g. "x").
    // strayText: lexeme of the next token, which also starts a valid
    // expression but has no operator joining it to leftText (e.g. "2").
    Diagnostic malformedExpression(const std::string& leftText,
                                    const std::string& strayText,
                                    SourceSpan span) const;

    // ── Semantic diagnostics (stubs — implemented in Stage 4) ─

    Diagnostic typeMismatch(const std::string& leftType,
                             const std::string& rightType,
                             SourceSpan span) const;

    Diagnostic undeclaredIdentifier(const std::string& name,
                                     SourceSpan span) const;

    Diagnostic redeclaredVariable(const std::string& name,
                                   SourceSpan span) const;

    // functionName/expectedType: the enclosing function and its
    // declared return type. For a bare 'return;' inside a function
    // whose declared return type is not void — the function promised
    // a value on every path and this statement doesn't provide one.
    Diagnostic missingReturnValue(const std::string& functionName,
                                   const std::string& expectedType,
                                   SourceSpan span) const;

private:
    // Shared assembly helper
    Diagnostic build(DiagnosticKind kind,
                      Severity severity,
                      SourceSpan span,
                      const std::string& message,
                      const std::string& detail,
                      const std::string& detail2 = "") const;
};
