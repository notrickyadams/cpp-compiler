#pragma once
#include "Diagnostic.hpp"
#include "ExplanationBuilder.hpp"
#include "TraceRecorder.hpp"
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
//
//  Traces:
//    If a stage attaches its TraceRecorder (attachRecorder),
//    every diagnostic built here carries a snapshot of the
//    stage's REAL call path at creation time, with the curated
//    factory step appended. With no recorder attached (direct
//    construction in unit tests / tooling), the curated
//    reference chain from ExplanationBuilder::trace() is used
//    as the fallback.
// ============================================================
class DiagnosticEngine {
public:
    // Observer only — the stage owns the recorder and must outlive
    // any use of this engine. Stages re-attach at the start of
    // their public entry point so the pointer stays valid even if
    // the stage object was moved.
    void attachRecorder(const TraceRecorder* recorder) {
        recorder_ = recorder;
    }

    // ── Lexer diagnostics ─────────────────────────────────

    Diagnostic unexpectedChar(char c, SourceSpan span) const;

    Diagnostic unterminatedComment(SourceSpan span) const;

    Diagnostic unterminatedString(SourceSpan span) const;

    Diagnostic invalidNumberLiteral(const std::string& literal,
                                    SourceSpan span) const;

    // literal: the exact digit string as written (e.g. "99999999999") —
    // lexically valid, but its value exceeds the 32-bit int range.
    Diagnostic integerOutOfRange(const std::string& literal,
                                  SourceSpan span) const;

    // ── Parser diagnostics ────────────────────────────────

    Diagnostic unexpectedToken(const std::string& found,
                                const std::string& expected,
                                SourceSpan span) const;

    Diagnostic missingToken(const std::string& expected,
                             SourceSpan span) const;

    // nodeType: the nodeType() of whatever was on the left of '='
    // (e.g. "BinaryExpr", "IntLiteral") — anything other than "Identifier".
    Diagnostic invalidAssignmentTarget(const std::string& nodeType,
                                        SourceSpan span) const;

    // readSoFar:  everything the parser had successfully consumed when
    //             the stray token appeared, as source text — statement
    //             context included (e.g. "return x" or "int x = 5").
    //             Shown verbatim in the WHAT HAPPENED narrative.
    // leftText:   just the expression part (e.g. "x"), used for the
    //             INVALID/VALID example snippets and the fix text.
    // strayText:  lexeme of the next token, which also starts a valid
    //             expression but has no operator joining it to leftText.
    // originMethod: the parser method that detected it — patched into
    //             the trace so the chain names the real call site
    //             (parseReturnStmt vs parseVarDecl), not a fixed one.
    Diagnostic malformedExpression(const std::string& readSoFar,
                                    const std::string& leftText,
                                    const std::string& strayText,
                                    SourceSpan span,
                                    const std::string& originMethod
                                        = "Parser::parseReturnStmt()") const;

    // ── Semantic diagnostics ──────────────────────────────

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

    // Warning, not error: a non-void function whose body contains no
    // return statement at all. Matches GCC's -Wreturn-type choice —
    // C/C++ call this undefined behaviour, not ill-formed, so the
    // build still succeeds.
    Diagnostic missingReturn(const std::string& functionName,
                              const std::string& returnType,
                              SourceSpan span) const;

    // A function name appearing where a value is required — read in an
    // expression, or written as an assignment target. No call syntax
    // or function pointers exist, so this is always an error.
    Diagnostic functionUsedAsValue(const std::string& name,
                                    SourceSpan span) const;

private:
    // Shared assembly helper
    Diagnostic build(DiagnosticKind kind,
                      Severity severity,
                      SourceSpan span,
                      const std::string& message,
                      const std::string& detail,
                      const std::string& detail2 = "") const;

    const TraceRecorder* recorder_ = nullptr;
};
