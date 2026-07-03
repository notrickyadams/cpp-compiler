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

// Kinds marked [reserved] are defined (with full ExplanationBuilder
// text) but no stage emits them yet — the language feature that
// would trigger them doesn't exist (strings, hex literals, void
// functions). They are kept so the knowledge base is ready when
// those features land, but tests must not expect to see them.
enum class DiagnosticKind {
    // ── Lexer ─────────────────────────────────────────────
    LEX_UnexpectedCharacter,      // '@', '$', bare '!', etc.
    LEX_UnterminatedBlockComment, // /* with no */
    LEX_UnterminatedString,       // [reserved] no string literals in the language yet
    LEX_InvalidNumberLiteral,     // [reserved] no hex/float syntax to get wrong yet
    LEX_IntegerOutOfRange,        // 99999999999  -- lexically valid, exceeds int

    // ── Parser ────────────────────────────────────────────
    PARSE_UnexpectedToken,        // expected ';', got '}'
    PARSE_MissingToken,           // [reserved] expect() reports PARSE_UnexpectedToken instead
    PARSE_UnbalancedBrace,        // [reserved] same — folded into PARSE_UnexpectedToken today
    PARSE_InvalidAssignmentTarget, // 3 = 5;  or  (a+b) = 5;
    PARSE_MalformedExpression,    // return x 2;  or  int x = 5 5;  -- two values, no operator

    // ── Semantic analysis ─────────────────────────────────
    SEM_UndeclaredIdentifier,     // using x before int x
    SEM_TypeMismatch,             // int vs bool
    SEM_RedeclaredVariable,       // int x; int x;  (also: param redeclared by a local)
    SEM_ReturnTypeMismatch,       // int f() { return; }  -- missing the required value
    SEM_VoidReturn,               // [reserved] void functions can't be declared yet
    SEM_MissingReturn,            // int f() { int x; }  -- warning: no return at all
    SEM_FunctionUsedAsValue,      // return main;  or  main = 5;
};

// Human-readable category name (used in the error header line)
inline std::string diagnosticKindName(DiagnosticKind k) {
    switch (k) {
        case DiagnosticKind::LEX_UnexpectedCharacter:       return "UnexpectedCharacter";
        case DiagnosticKind::LEX_UnterminatedBlockComment:  return "UnterminatedBlockComment";
        case DiagnosticKind::LEX_UnterminatedString:        return "UnterminatedString";
        case DiagnosticKind::LEX_InvalidNumberLiteral:      return "InvalidNumberLiteral";
        case DiagnosticKind::LEX_IntegerOutOfRange:         return "IntegerOutOfRange";
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
        case DiagnosticKind::SEM_MissingReturn:             return "MissingReturn";
        case DiagnosticKind::SEM_FunctionUsedAsValue:       return "FunctionUsedAsValue";
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
        case DiagnosticKind::LEX_IntegerOutOfRange:         return "Integer Out Of Range";
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
        case DiagnosticKind::SEM_MissingReturn:             return "Missing Return";
        case DiagnosticKind::SEM_FunctionUsedAsValue:       return "Function Used As Value";
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
        case DiagnosticKind::LEX_IntegerOutOfRange:
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
        case DiagnosticKind::SEM_MissingReturn:
        case DiagnosticKind::SEM_FunctionUsedAsValue:
            return "SemanticAnalysis";

        default: return "Unknown";
    }
}
