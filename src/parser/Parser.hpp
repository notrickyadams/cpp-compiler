#pragma once
#include "../lexer/Token.hpp"
#include "../ast/Nodes.hpp"
#include "../diagnostics/Diagnostic.hpp"
#include "../diagnostics/DiagnosticEngine.hpp"
#include "../diagnostics/TraceRecorder.hpp"
#include <vector>
#include <memory>
#include <string>

// ============================================================
//  Parser — transforms a token stream into an AST.
//
//  Algorithm: recursive descent with panic-mode error recovery.
//
//  Grammar (EBNF):
//    program      → function_decl*  EOF
//    function_decl→ type IDENT '(' param_list ')' block
//    param_list   → (type IDENT (',' type IDENT)*)  |  ε
//    block        → '{' statement* '}'
//    statement    → var_decl | return_stmt
//    var_decl     → type IDENT ('=' expression)? ';'
//    return_stmt  → 'return' expression? ';'
//    expression   → assignment
//    assignment   → IDENTIFIER '=' assignment | equality
//    equality     → comparison (('=='|'!=') comparison)*
//    comparison   → addition   (('<'|'>') addition)*
//    addition     → multiply   (('+'|'-') multiply)*
//    multiply     → unary      (('*'|'/') unary)*
//    unary        → primary
//    primary      → INTEGER_LITERAL | IDENTIFIER | '(' expression ')'
//    type         → 'int'
//
//  Error recovery:
//    On parse error, the parser emits a Diagnostic then calls
//    synchronize() which skips tokens until a statement
//    boundary (';', '}', keyword). This lets us report multiple
//    errors in one pass — exactly like GCC/Clang.
//
//  Ownership:
//    parse() returns a unique_ptr<ProgramNode> that owns the
//    entire tree. Caller takes ownership. nullptr is returned
//    only on catastrophic failure (empty token stream).
// ============================================================
class Parser {
public:
    explicit Parser(std::vector<Token> tokens);

    // Primary API
    StageOutput<std::unique_ptr<ProgramNode>> parse();

private:
    // ── Grammar rules ────────────────────────────────────
    std::unique_ptr<FunctionDeclNode> parseFunctionDecl();
    std::vector<ParamNode>            parseParamList();
    std::unique_ptr<BlockStmtNode>    parseBlock();
    std::unique_ptr<ASTNode>          parseStatement();
    std::unique_ptr<VarDeclNode>      parseVarDecl();
    std::unique_ptr<ReturnStmtNode>   parseReturnStmt();

    // Expression precedence chain (low → high priority)
    std::unique_ptr<ASTNode> parseExpression();
    std::unique_ptr<ASTNode> parseAssignment();
    std::unique_ptr<ASTNode> parseEquality();
    std::unique_ptr<ASTNode> parseComparison();
    std::unique_ptr<ASTNode> parseAddition();
    std::unique_ptr<ASTNode> parseMultiplication();
    std::unique_ptr<ASTNode> parsePrimary();

    // Type parsing (returns type name string)
    std::string parseTypeName();
    bool        isTypeName() const;

    // True if a token of this type can begin a new expression
    // (primary's first-set). Used to detect "return x 2;" — two
    // expressions back-to-back with no operator between them.
    bool isExpressionStart(TokenType t) const;

    // ── Token management ─────────────────────────────────
    const Token& current() const;
    Token        advance();
    bool         check(TokenType t) const;
    bool         match(TokenType t);
    // expect: consume token or emit diagnostic
    Token        expect(TokenType t, const std::string& what);
    bool         isAtEnd() const;

    // ── Error recovery ────────────────────────────────────
    void synchronize();
    // After a malformed-expression diagnostic: consume the stray
    // tokens up to (not including) the statement boundary, so the
    // caller's expect(SEMICOLON) sees the ';' instead of re-failing
    // on leftovers like the "+ 3" in "return x 2 + 3;".
    void skipStrayExpressionTokens();
    void emitError(const std::string& message, SourceSpan span);

    // Short "at 'lexeme' (line N)" description of the current token,
    // used as the detail on recorded trace frames.
    TraceDetail hereDetail() const;

    // ── State ─────────────────────────────────────────────
    std::vector<Token>       tokens_;
    std::size_t              pos_ = 0;
    DiagnosticEngine         engine_;
    TraceRecorder            recorder_{"Parser"};
    std::vector<Diagnostic>  diagnostics_;
};
