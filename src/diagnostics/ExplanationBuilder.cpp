#include "ExplanationBuilder.hpp"

// ─────────────────────────────────────────────────────────────
//  rootCause — one sentence, no period, suitable for a subtitle
// ─────────────────────────────────────────────────────────────
std::string ExplanationBuilder::rootCause(DiagnosticKind kind,
                                           const std::string& detail,
                                           const std::string& detail2) {
    switch (kind) {
        case DiagnosticKind::LEX_UnexpectedCharacter:
            return "'" + detail + "' is not the start of any valid token";

        case DiagnosticKind::LEX_UnterminatedBlockComment:
            return "block comment opened with '/*' was never closed with '*/'";

        case DiagnosticKind::LEX_UnterminatedString:
            return "string literal opened with '\"' was never closed";

        case DiagnosticKind::LEX_InvalidNumberLiteral:
            return "numeric literal '" + detail + "' has invalid syntax";

        case DiagnosticKind::PARSE_UnexpectedToken:
            // 'detail' here is the pre-formatted FOUND token description
            // (e.g. "'-'" or "end of input") — not the expected one, which
            // is already part of the top-level Diagnostic::message.
            return "found " + detail + " where a different token was expected";

        case DiagnosticKind::PARSE_MissingToken:
            return "expected '" + detail + "' but reached end of input or wrong token";

        case DiagnosticKind::PARSE_UnbalancedBrace:
            return "opening '{' has no matching closing '}'";

        case DiagnosticKind::PARSE_InvalidAssignmentTarget:
            // 'detail' here is the nodeType() of whatever was parsed on
            // the left of '=' (e.g. "BinaryExpr", "IntLiteral") — not a
            // token, since this is only detected after a full
            // sub-expression has already been parsed.
            return "the left side of '=' (" + detail + ") is not something that can be assigned to";

        case DiagnosticKind::PARSE_MalformedExpression:
            // Stays generic — the specific values ('detail'/'detail2')
            // are spelled out in explain()'s narrative instead, so this
            // one-line summary reads naturally on its own.
            return "two expressions appeared consecutively with no operator between them";

        case DiagnosticKind::SEM_UndeclaredIdentifier:
            return "'" + detail + "' is used before it is declared";

        case DiagnosticKind::SEM_TypeMismatch:
            return "operands have incompatible types: " + detail;

        case DiagnosticKind::SEM_RedeclaredVariable:
            return "'" + detail + "' is declared more than once in the same scope";

        case DiagnosticKind::SEM_ReturnTypeMismatch:
            // detail = function name, detail2 = its declared return type.
            return "'" + detail + "' is declared to return '" + detail2 +
                   "' but this return statement provides no value";

        case DiagnosticKind::SEM_VoidReturn:
            return "void function cannot return a value";

        default:
            return "unknown error";
    }
}

// ─────────────────────────────────────────────────────────────
//  explain — 2-4 sentences explaining WHY this is a problem
// ─────────────────────────────────────────────────────────────
std::string ExplanationBuilder::explain(DiagnosticKind kind,
                                         const std::string& detail,
                                         const std::string& detail2) {
    switch (kind) {
        case DiagnosticKind::LEX_UnexpectedCharacter:
            return
                "The lexer scans source text character by character, and for each "
                "character it must decide which token kind to begin.\n"
                "The character '" + detail + "' does not start any recognised token "
                "in this language's grammar.\n"
                "Valid token starts are: letters and underscore [a-zA-Z_] for "
                "identifiers/keywords, digits [0-9] for integer literals, and the "
                "operator/delimiter characters: + - * / = ! < > ( ) { } ; ,";

        case DiagnosticKind::LEX_UnterminatedBlockComment:
            return
                "A block comment begins with /* and must end with */.\n"
                "The lexer reached the end of the file while still inside a comment, "
                "which means the */ closing sequence is missing.\n"
                "Everything from the /* to the end of file is treated as a comment, "
                "which may hide tokens the parser needs.";

        case DiagnosticKind::LEX_UnterminatedString:
            return
                "A string literal begins with a double-quote \" and must end with "
                "a matching double-quote on the same line.\n"
                "The lexer reached a newline or end of file before finding the "
                "closing quote.\n"
                "String literals cannot span multiple lines unless you use "
                "concatenation or raw string syntax.";

        case DiagnosticKind::LEX_InvalidNumberLiteral:
            return
                "The lexer recognised the start of a numeric literal but the "
                "characters that followed do not form a valid number.\n"
                "For example, '0x' starts a hex literal but requires at least one "
                "hex digit [0-9a-fA-F] after it.";

        case DiagnosticKind::PARSE_UnexpectedToken:
            return
                "The parser was reading a grammar rule that requires a specific "
                "token at this position.\n"
                "Instead it found " + detail + ", which means either the preceding "
                "tokens have an error (a missing semicolon, bracket, or operator), "
                "or the syntax of this construct is incorrect.";

        case DiagnosticKind::PARSE_MissingToken:
            return
                "The parser reached the end of a grammar rule but the required "
                "closing token '" + detail + "' was never found.\n"
                "This often means an opening bracket, parenthesis, or keyword "
                "was never matched with its closing counterpart.";

        case DiagnosticKind::PARSE_UnbalancedBrace:
            return
                "Every '{' that opens a block must have a matching '}' to close it.\n"
                "The parser reached the end of the file (or another top-level construct) "
                "without finding the closing '}'.\n"
                "Check for an extra '{' or a missing '}'.";

        case DiagnosticKind::PARSE_InvalidAssignmentTarget:
            return
                "Assignment ('=') requires a variable name on its left side — that "
                "variable's storage is what receives the new value.\n"
                "The left side here was a " + detail + ", which is a computed value "
                "with no storage location of its own, so there is nothing for '=' "
                "to write into.";

        case DiagnosticKind::PARSE_MalformedExpression:
            return
                "The parser successfully read:\n"
                "\n"
                "return " + detail + "\n"
                "\n"
                "After completing the return expression,\n"
                "it encountered another value:\n"
                "\n"
                + detail2 + "\n"
                "\n"
                "C++ requires an operator between values.";

        case DiagnosticKind::SEM_TypeMismatch:
            return
                "Every operator in C++ is only defined for specific combinations of "
                "operand types.\n"
                "The types of the two operands you provided (" + detail + ") do not "
                "match any overload of this operator.\n"
                "The type system enforces this at compile time to prevent "
                "undefined runtime behaviour.";

        case DiagnosticKind::SEM_UndeclaredIdentifier:
            return
                "Every name used in a C++ program must be declared before it is used "
                "(within the same scope or an enclosing one).\n"
                "The name '" + detail + "' has not been declared anywhere visible "
                "from this point.\n"
                "This catches typos and missing includes at compile time.";

        case DiagnosticKind::SEM_RedeclaredVariable:
            return
                "Within a single scope, each variable name must be unique.\n"
                "'" + detail + "' has already been declared in this block.\n"
                "If you intended to assign a new value, use assignment (=) "
                "without the type keyword.";

        case DiagnosticKind::SEM_ReturnTypeMismatch:
            return
                "Every non-void function must produce a value of its declared "
                "type on every path through the function.\n"
                "'" + detail + "' is declared to return '" + detail2 + "', but this "
                "'return;' provides no value at all.\n"
                "At the machine level this means no value is ever placed in the "
                "return register, so the caller would read whatever happened to "
                "already be there — not a value your program computed.";

        default:
            return "An error occurred in the " +
                   diagnosticStage(kind) + " stage.";
    }
}

// ─────────────────────────────────────────────────────────────
//  fixes — ordered fix suggestions (most likely first)
// ─────────────────────────────────────────────────────────────
std::vector<FixSuggestion>
ExplanationBuilder::fixes(DiagnosticKind kind,
                           const std::string& detail,
                           const std::string& detail2) {
    std::vector<FixSuggestion> out;

    switch (kind) {
        case DiagnosticKind::LEX_UnexpectedCharacter:
            out.push_back({ "Remove the '" + detail + "' character" });
            out.push_back({ "If you meant an identifier, start it with a letter or underscore" });
            out.push_back({ "Check for a copy-paste artefact (e.g. a Unicode quote instead of ASCII)" });
            break;

        case DiagnosticKind::LEX_UnterminatedBlockComment:
            out.push_back({ "Add */ to close the block comment" });
            out.push_back({ "If the comment is not needed, remove the /* entirely" });
            break;

        case DiagnosticKind::LEX_UnterminatedString:
            out.push_back({ "Add a closing \" on the same line" });
            out.push_back({ "If the string spans lines intentionally, split it into adjacent literals" });
            break;

        case DiagnosticKind::PARSE_UnexpectedToken:
            out.push_back({ "Check if a ';' or ')' is missing before this point" });
            out.push_back({ "Verify the spelling of the keyword or operator" });
            out.push_back({ "Found " + detail + " — check what precedes it for the real cause" });
            break;

        case DiagnosticKind::PARSE_MissingToken:
            out.push_back({ "Add the missing '" + detail + "' at the indicated location" });
            out.push_back({ "Check if a matching open/close pair is unbalanced" });
            break;

        case DiagnosticKind::PARSE_UnbalancedBrace:
            out.push_back({ "Add the missing '}' to close the block" });
            out.push_back({ "Check if you have an extra '{' that should be removed" });
            break;

        case DiagnosticKind::PARSE_InvalidAssignmentTarget:
            out.push_back({ "Assign into a plain variable name instead, e.g. x = ..." });
            out.push_back({ "If you meant to compare two values, use '==' instead of '='" });
            break;

        case DiagnosticKind::PARSE_MalformedExpression:
            out.push_back({ "Insert an operator between " + detail + " and " + detail2 });
            out.push_back({ "or remove the extra value" });
            break;

        case DiagnosticKind::SEM_TypeMismatch:
            out.push_back({ "Ensure both operands have the same type" });
            out.push_back({ "Use an explicit cast: static_cast<int>(expr)" });
            out.push_back({ "Change the operator to one that supports mixed types" });
            break;

        case DiagnosticKind::SEM_UndeclaredIdentifier:
            out.push_back({ "Declare '" + detail + "' before this point: int " + detail + " = ...;" });
            out.push_back({ "Check for a typo in the identifier name" });
            out.push_back({ "Verify the variable is in scope (not declared in a different block)" });
            break;

        case DiagnosticKind::SEM_RedeclaredVariable:
            out.push_back({ "Remove the type keyword to assign instead of re-declare" });
            out.push_back({ "Rename one of the declarations to avoid the conflict" });
            break;

        case DiagnosticKind::SEM_ReturnTypeMismatch:
            out.push_back({ "Provide a value: return <" + detail2 + " expression>;" });
            out.push_back({ "Change '" + detail + "'s return type to 'void' if no value should be returned" });
            break;

        default:
            out.push_back({ "Consult the compiler documentation for this error kind" });
            break;
    }

    return out;
}

// ─────────────────────────────────────────────────────────────
//  trace — the compiler's internal call chain for this kind
// ─────────────────────────────────────────────────────────────
std::vector<TraceStep>
ExplanationBuilder::trace(DiagnosticKind kind) {
    std::vector<TraceStep> t;

    switch (kind) {
        case DiagnosticKind::LEX_UnexpectedCharacter:
            t.push_back({ "Lexer", "Lexer::tokenize()",             "entry point — begin scanning",  true });
            t.push_back({ "Lexer", "Lexer::nextToken()",            "dispatching on first character", true });
            t.push_back({ "Lexer", "Lexer::skipWhitespace()",       "whitespace consumed",           true });
            t.push_back({ "Lexer", "Lexer::lexSymbol()",            "character not in switch cases", false });
            t.push_back({ "Lexer", "DiagnosticEngine::unexpectedChar()", "diagnostic created",       false });
            break;

        case DiagnosticKind::LEX_UnterminatedBlockComment:
            t.push_back({ "Lexer", "Lexer::tokenize()",                      "entry point",           true });
            t.push_back({ "Lexer", "Lexer::skipWhitespaceAndComments()",     "found '/*' sequence",   true });
            t.push_back({ "Lexer", "Lexer::skipWhitespaceAndComments()",     "reached EOF before '*/'", false });
            t.push_back({ "Lexer", "DiagnosticEngine::unterminatedComment()", "diagnostic created",   false });
            break;

        case DiagnosticKind::PARSE_UnexpectedToken:
            t.push_back({ "Parser", "Parser::parse()",          "scanning top-level construct",       true });
            t.push_back({ "Parser", "Parser::parseStatement()", "dispatching on current token",       true });
            t.push_back({ "Parser", "Parser::expect()",         "required token not found",           false });
            t.push_back({ "Parser", "DiagnosticEngine::unexpectedToken()", "diagnostic created",      false });
            break;

        case DiagnosticKind::PARSE_MissingToken:
            t.push_back({ "Parser", "Parser::parse()",          "scanning construct",                 true });
            t.push_back({ "Parser", "Parser::expect()",         "end of input or wrong token",        false });
            t.push_back({ "Parser", "DiagnosticEngine::missingToken()", "diagnostic created",         false });
            break;

        case DiagnosticKind::PARSE_InvalidAssignmentTarget:
            t.push_back({ "Parser", "Parser::parseAssignment()", "parsed left side, found '=' next",  true });
            t.push_back({ "Parser", "Parser::parseAssignment()", "left side is not an Identifier",     false });
            t.push_back({ "Parser", "DiagnosticEngine::invalidAssignmentTarget()", "diagnostic created", false });
            break;

        case DiagnosticKind::PARSE_MalformedExpression:
            t.push_back({ "Parser", "Parser::parseReturnStmt()",   "parsed return expression",             true  });
            t.push_back({ "Parser", "Parser::isExpressionStart()", "next token also starts an expression", false });
            t.push_back({ "Parser", "DiagnosticEngine::malformedExpression()", "diagnostic created",       false });
            break;

        case DiagnosticKind::SEM_TypeMismatch:
            t.push_back({ "SemanticAnalysis", "SemanticAnalyzer::visitBinaryExpr()", "evaluating operand types",   true });
            t.push_back({ "SemanticAnalysis", "TypeResolver::resolve()",             "resolved left and right types", true });
            t.push_back({ "SemanticAnalysis", "OperatorTable::lookup()",             "no matching overload found", false });
            t.push_back({ "SemanticAnalysis", "DiagnosticEngine::typeMismatch()",    "diagnostic created",         false });
            break;

        case DiagnosticKind::SEM_UndeclaredIdentifier:
            t.push_back({ "SemanticAnalysis", "SemanticAnalyzer::visitIdentifier()", "looking up name in scope",   true });
            t.push_back({ "SemanticAnalysis", "SymbolTable::lookup()",               "name not found in any scope", false });
            t.push_back({ "SemanticAnalysis", "DiagnosticEngine::undeclaredIdent()", "diagnostic created",         false });
            break;

        case DiagnosticKind::SEM_ReturnTypeMismatch:
            t.push_back({ "SemanticAnalysis", "SemanticAnalyzer::visit(ReturnStmtNode&)", "return statement has no value",  true  });
            t.push_back({ "SemanticAnalysis", "Type::isVoid()",                          "declared return type is not void", false });
            t.push_back({ "SemanticAnalysis", "DiagnosticEngine::missingReturnValue()",  "diagnostic created",               false });
            break;

        default:
            t.push_back({ diagnosticStage(kind), diagnosticKindName(kind), "diagnostic created", false });
            break;
    }

    return t;
}
