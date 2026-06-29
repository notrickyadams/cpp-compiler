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
    PARSE_InvalidAssignmentTarget, // 3 = 5;  or  (a+b) = 5;
    PARSE_MalformedExpression,    // return x 2;  -- two values, no operator

    // ── Semantic analysis (future) ────────────────────────
    SEM_UndeclaredIdentifier,     // using x before int x
    SEM_TypeMismatch,             // int + string
    SEM_RedeclaredVariable,       // int x; int x;
    SEM_ReturnTypeMismatch,       // int f() { return; }  -- missing the required value
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
        case DiagnosticKind::PARSE_InvalidAssignmentTarget: return "InvalidAssignmentTarget";
        case DiagnosticKind::PARSE_MalformedExpression:     return "MalformedExpression";
        case DiagnosticKind::SEM_UndeclaredIdentifier:      return "UndeclaredIdentifier";
        case DiagnosticKind::SEM_TypeMismatch:              return "TypeMismatch";
        case DiagnosticKind::SEM_RedeclaredVariable:        return "RedeclaredVariable";
        case DiagnosticKind::SEM_ReturnTypeMismatch:        return "ReturnTypeMismatch";
        case DiagnosticKind::SEM_VoidReturn:                return "VoidReturn";
        default: return "Unknown";
    }
}

// Human-readable, SPACED display name for the "ERROR TYPE:" header
// in DiagnosticCollector's full render. Kept separate from
// diagnosticKindName() above: that one's exact string ("UndeclaredIdentifier",
// no space) is relied on verbatim by renderJson() and the visualizer
// frontend/tests — this one exists purely for human prose.
inline std::string diagnosticKindDisplayName(DiagnosticKind k) {
    switch (k) {
        case DiagnosticKind::LEX_UnexpectedCharacter:       return "Unexpected Character";
        case DiagnosticKind::LEX_UnterminatedBlockComment:  return "Unterminated Block Comment";
        case DiagnosticKind::LEX_UnterminatedString:        return "Unterminated String";
        case DiagnosticKind::LEX_InvalidNumberLiteral:      return "Invalid Number Literal";
        case DiagnosticKind::PARSE_UnexpectedToken:         return "Unexpected Token";
        case DiagnosticKind::PARSE_MissingToken:            return "Missing Token";
        case DiagnosticKind::PARSE_UnbalancedBrace:         return "Unbalanced Brace";
        case DiagnosticKind::PARSE_InvalidAssignmentTarget: return "Invalid Assignment Target";
        case DiagnosticKind::PARSE_MalformedExpression:     return "Malformed Expression";
        case DiagnosticKind::SEM_UndeclaredIdentifier:      return "Undeclared Identifier";
        case DiagnosticKind::SEM_TypeMismatch:              return "Type Mismatch";
        case DiagnosticKind::SEM_RedeclaredVariable:        return "Redeclared Variable";
        case DiagnosticKind::SEM_ReturnTypeMismatch:        return "Return Type Mismatch";
        case DiagnosticKind::SEM_VoidReturn:                return "Void Return";
        default: return "Unknown Error";
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
        case DiagnosticKind::PARSE_InvalidAssignmentTarget:
        case DiagnosticKind::PARSE_MalformedExpression:
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
