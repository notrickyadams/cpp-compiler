#include "DiagnosticEngine.hpp"
#include <string>

// ─────────────────────────────────────────────────────────────
//  Private: shared assembly helper
// ─────────────────────────────────────────────────────────────
Diagnostic DiagnosticEngine::build(DiagnosticKind kind,
                                    Severity severity,
                                    SourceSpan span,
                                    const std::string& message,
                                    const std::string& detail,
                                    const std::string& detail2) const {
    Diagnostic d;
    d.kind        = kind;
    d.severity    = severity;
    d.span        = span;
    d.message     = message;
    d.rootCause   = ExplanationBuilder::rootCause(kind, detail, detail2);
    d.explanation = ExplanationBuilder::explain(kind, detail, detail2);
    d.fixes       = ExplanationBuilder::fixes(kind, detail, detail2);
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

Diagnostic DiagnosticEngine::integerOutOfRange(const std::string& literal,
                                                 SourceSpan span) const {
    return build(
        DiagnosticKind::LEX_IntegerOutOfRange,
        Severity::Error,
        span,
        "integer literal '" + literal + "' is out of range for int",
        literal
    );
}

// ─────────────────────────────────────────────────────────────
//  Parser diagnostics
// ─────────────────────────────────────────────────────────────

Diagnostic DiagnosticEngine::unexpectedToken(const std::string& found,
                                              const std::string& expected,
                                              SourceSpan span) const {
    // 'expected' is already a full descriptive phrase from the call site
    // (e.g. "';' after variable declaration", "an expression"), not a bare
    // token — so it is NOT re-wrapped in quotes here, only 'found' is.
    std::string foundDesc = found.empty() ? "end of input" : ("'" + found + "'");
    return build(
        DiagnosticKind::PARSE_UnexpectedToken,
        Severity::Error,
        span,
        "expected " + expected + ", found " + foundDesc,
        foundDesc
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

Diagnostic DiagnosticEngine::invalidAssignmentTarget(const std::string& nodeType,
                                                       SourceSpan span) const {
    return build(
        DiagnosticKind::PARSE_InvalidAssignmentTarget,
        Severity::Error,
        span,
        "invalid assignment target (" + nodeType + ")",
        nodeType
    );
}

Diagnostic DiagnosticEngine::malformedExpression(const std::string& readSoFar,
                                                   const std::string& leftText,
                                                   const std::string& strayText,
                                                   SourceSpan span,
                                                   const std::string& originMethod) const {
    Diagnostic d = build(
        DiagnosticKind::PARSE_MalformedExpression,
        Severity::Error,
        span,
        "expected an operator or ';', found '" + strayText + "' after '" + leftText + "'",
        leftText,
        strayText
    );
    // This kind needs THREE pieces of context, one more than build()'s
    // detail/detail2 can carry: the narrative wants the full statement
    // text read so far, while the fixes/examples want just the
    // expression. Re-derive the explanation with the statement text.
    d.explanation = ExplanationBuilder::explain(
        DiagnosticKind::PARSE_MalformedExpression, readSoFar, strayText);

    // The static trace names parseReturnStmt; patch in the real call
    // site so the chain stays accurate when parseVarDecl detects it.
    if (!d.trace.empty()) d.trace.front().component = originMethod;

    // '(' as the stray token would make these read as gibberish
    // ("x + ("), so the code-comparison block is only attached when
    // the stray is a value-like token.
    if (strayText != "(") {
        d.invalidExample = leftText + " " + strayText;
        d.validExamples.push_back(leftText + " + " + strayText);
        d.validExamples.push_back(leftText + " * " + strayText);
        d.validExamples.push_back(leftText + " = " + strayText);
    }
    return d;
}

// ─────────────────────────────────────────────────────────────
//  Semantic diagnostics
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

Diagnostic DiagnosticEngine::missingReturnValue(const std::string& functionName,
                                                  const std::string& expectedType,
                                                  SourceSpan span) const {
    return build(
        DiagnosticKind::SEM_ReturnTypeMismatch,
        Severity::Error,
        span,
        "missing return value in function '" + functionName +
            "' (expected '" + expectedType + "')",
        functionName,
        expectedType
    );
}

Diagnostic DiagnosticEngine::missingReturn(const std::string& functionName,
                                             const std::string& returnType,
                                             SourceSpan span) const {
    return build(
        DiagnosticKind::SEM_MissingReturn,
        Severity::Warning,
        span,
        "function '" + functionName + "' is declared to return '" +
            returnType + "' but never returns a value",
        functionName,
        returnType
    );
}

Diagnostic DiagnosticEngine::functionUsedAsValue(const std::string& name,
                                                   SourceSpan span) const {
    return build(
        DiagnosticKind::SEM_FunctionUsedAsValue,
        Severity::Error,
        span,
        "'" + name + "' is a function and cannot be used as a value",
        name
    );
}
