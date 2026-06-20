#pragma once
#include "DiagnosticKind.hpp"
#include "Diagnostic.hpp"
#include <string>
#include <vector>

// ============================================================
//  ExplanationBuilder — knowledge base for compiler diagnostics.
//
//  This is the core of the "explainable" design.
//  For every DiagnosticKind it knows:
//    • a root cause sentence
//    • a multi-sentence explanation (for developers)
//    • a list of fix suggestions
//    • the internal trace (how the compiler got here)
//
//  Design principle: explanation logic is ISOLATED here.
//  The Lexer, Parser, etc. never write English prose — they
//  call ExplanationBuilder, which centralises all user-facing
//  text. This means:
//    • Changing an error message = one place
//    • Adding a new kind = one place
//    • Translating messages later = one place
//
//  Industry reference:
//    Rust's `rustc_error_messages` crate (Fluent-based i18n)
//    Clang's DiagnosticLexKinds.td (TableGen templates)
// ============================================================

class ExplanationBuilder {
public:
    // ── Root cause (single sentence) ─────────────────────
    static std::string rootCause(DiagnosticKind kind,
                                  const std::string& detail = "");

    // ── Explanation (multi-sentence WHY) ─────────────────
    static std::string explain(DiagnosticKind kind,
                                const std::string& detail = "");

    // ── Fix suggestions ───────────────────────────────────
    static std::vector<FixSuggestion> fixes(DiagnosticKind kind,
                                             const std::string& detail = "");

    // ── Compiler internal trace ───────────────────────────
    static std::vector<TraceStep> trace(DiagnosticKind kind);
};
