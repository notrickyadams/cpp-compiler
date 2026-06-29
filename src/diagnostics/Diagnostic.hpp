#pragma once
#include "DiagnosticKind.hpp"
#include "../core/SourceSpan.hpp"
#include <string>
#include <vector>

// ============================================================
//  Severity — how serious is the diagnostic?
// ============================================================
enum class Severity {
    Note,     // informational — not a problem
    Warning,  // compiles but likely wrong
    Error,    // will not compile
    Fatal     // abort compilation immediately
};

inline std::string severityName(Severity s) {
    switch (s) {
        case Severity::Note:    return "note";
        case Severity::Warning: return "warning";
        case Severity::Error:   return "error";
        case Severity::Fatal:   return "fatal";
        default: return "?";
    }
}

// ============================================================
//  TraceStep — one step in the compiler's internal call chain.
//
//  Shows the user HOW the compiler arrived at this diagnostic.
//  Example chain for UnexpectedCharacter:
//    Lexer::nextToken()
//    └─ Lexer::lexSymbol()
//       └─ switch(c) default branch
//          └─ DiagnosticEngine::unexpectedChar()
// ============================================================
struct TraceStep {
    std::string stage;      // "Lexer", "Parser", etc.
    std::string component;  // "Lexer::lexSymbol()"
    std::string detail;     // "switch default — unrecognised char"
    bool        ok;         // false = this step is where it failed
};

// ============================================================
//  FixSuggestion — one actionable thing the user can do.
// ============================================================
struct FixSuggestion {
    std::string description; // human text
    // Future: SourceEdit { span, replacement } for IDE auto-fix
};

// ============================================================
//  Diagnostic — the core data structure of the entire project.
//
//  Every error, warning, or note in this compiler is a
//  Diagnostic. Downstream stages (visualizer, LSP, JSON export)
//  all read from this single struct — nothing else.
//
//  Fields:
//    kind        — machine-readable classification (for tests)
//    severity    — note / warning / error / fatal
//    span        — exact source location
//    message     — short, one-line summary (like GCC's output)
//    rootCause   — single sentence: the actual underlying reason
//    explanation — 2-4 sentences: WHY this is a problem
//    fixes       — ordered list of fix suggestions
//    trace       — compiler's internal decision chain
//
//  Industry reference:
//    Clang's Diagnostic class + DiagnosticConsumer
//    Rust's Diagnostic (in rustc_errors)
// ============================================================
struct Diagnostic {
    DiagnosticKind          kind;
    Severity                severity;
    SourceSpan              span;

    std::string             message;      // "unexpected character '@'"
    std::string             rootCause;    // "'@' is not a valid token start"
    std::string             explanation;  // 2-4 sentences
    std::vector<FixSuggestion> fixes;
    std::vector<TraceStep>  trace;

    // Optional INVALID/VALID code comparison, shown by
    // DiagnosticCollector only when invalidExample is non-empty.
    // Most kinds leave these empty — they exist for syntax-SHAPE
    // errors (e.g. PARSE_MalformedExpression) where showing the
    // exact wrong snippet next to a few corrected forms is clearer
    // than prose alone.
    std::string              invalidExample;
    std::vector<std::string> validExamples;

    // Convenience: is this an error-level or worse?
    bool isError() const {
        return severity == Severity::Error || severity == Severity::Fatal;
    }
};

// ============================================================
//  StageOutput<T> — what every compiler stage returns:
//    output      — the stage product (always present, even on error)
//    diagnostics — zero or more collected errors/warnings
//  Defined here (not in Result.hpp) because it depends on Diagnostic.
// ============================================================
template<typename T>
struct StageOutput {
    T                       output;
    std::vector<Diagnostic> diagnostics;

    bool hasErrors() const {
        for (const auto& d : diagnostics) {
            if (d.isError()) return true;
        }
        return false;
    }

    // Convenience: count of error-level diagnostics
    int errorCount() const {
        int n = 0;
        for (const auto& d : diagnostics) {
            if (d.isError()) ++n;
        }
        return n;
    }
};
