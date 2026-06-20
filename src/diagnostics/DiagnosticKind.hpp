#pragma once
#include <string>

// ============================================================
//  DiagnosticKind — every distinct error/warning type.
//
//  Design principles:
//    • One enum value per distinct root cause (not per message).
//      Two errors with different messages but the same root
//      cause share a kind — the ExplanationBuilder then picks
//      the right text based on extra context.
//    • Grouped by pipeline stage (LEX_, PARSE_, SEM_, etc.)
//      so a reader immediately knows where the problem lives.
//
//  Industry reference:
//    Clang: DiagnosticID enum + .td TableGen files
//    Rust:  ErrorCode enum (E0001–E9999)
//    Ours is intentionally smaller and human-annotated.
// ============================================================

enum class DiagnosticKind {
    // ── Lexer ─────────────────────────────────────────────
    LEX_UnexpectedCharacter,      // '@', '$', etc.
    LEX_UnterminatedBlockComment, // /* with no */
    LEX_UnterminatedString,       // "hello  (no closing ")
    LEX_InvalidNumberLiteral,     // 0x (no hex digits)

    // ── Parser (future) ───────────────────────────────────
    PARSE_UnexpectedToken,        // expected ';', got '}'
    PARSE_MissingToken,           // expected '(' after 'if'
    PARSE_UnbalancedBrace,        // missing closing '}'

    // ── Semantic analysis (future) ────────────────────────
    SEM_UndeclaredIdentifier,     // using x before int x
    SEM_TypeMismatch,             // int + string
    SEM_RedeclaredVariable,       // int x; int x;
    SEM_ReturnTypeMismatch,       // int f() { return "hi"; }
    SEM_VoidReturn,               // void f() { return 5; }
};

// Human-readable category name (used in the error header line)
inline std::string diagnosticKindName(DiagnosticKind k) {
    switch (k) {
        case DiagnosticKind::LEX_UnexpectedCharacter:       return "UnexpectedCharacter";
        case DiagnosticKind::LEX_UnterminatedBlockComment:  return "UnterminatedBlockComment";
        case DiagnosticKind::LEX_UnterminatedString:        return "UnterminatedString";
        case DiagnosticKind::LEX_InvalidNumberLiteral:      return "InvalidNumberLiteral";
        case DiagnosticKind::PARSE_UnexpectedToken:         return "UnexpectedToken";
        case DiagnosticKind::PARSE_MissingToken:            return "MissingToken";
        case DiagnosticKind::PARSE_UnbalancedBrace:         return "UnbalancedBrace";
        case DiagnosticKind::SEM_UndeclaredIdentifier:      return "UndeclaredIdentifier";
        case DiagnosticKind::SEM_TypeMismatch:              return "TypeMismatch";
        case DiagnosticKind::SEM_RedeclaredVariable:        return "RedeclaredVariable";
        case DiagnosticKind::SEM_ReturnTypeMismatch:        return "ReturnTypeMismatch";
        case DiagnosticKind::SEM_VoidReturn:                return "VoidReturn";
        default: return "Unknown";
    }
}

// The pipeline stage that produced this kind
inline std::string diagnosticStage(DiagnosticKind k) {
    switch (k) {
        case DiagnosticKind::LEX_UnexpectedCharacter:
        case DiagnosticKind::LEX_UnterminatedBlockComment:
        case DiagnosticKind::LEX_UnterminatedString:
        case DiagnosticKind::LEX_InvalidNumberLiteral:
            return "Lexer";

        case DiagnosticKind::PARSE_UnexpectedToken:
        case DiagnosticKind::PARSE_MissingToken:
        case DiagnosticKind::PARSE_UnbalancedBrace:
            return "Parser";

        case DiagnosticKind::SEM_UndeclaredIdentifier:
        case DiagnosticKind::SEM_TypeMismatch:
        case DiagnosticKind::SEM_RedeclaredVariable:
        case DiagnosticKind::SEM_ReturnTypeMismatch:
        case DiagnosticKind::SEM_VoidReturn:
            return "SemanticAnalysis";

        default: return "Unknown";
    }
}
