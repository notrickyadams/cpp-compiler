#include "Parser.hpp"

// ────────────────────────────────────────────────────────────
//  exprToSourceText — reconstruct source-like text from an
//  already-parsed expression, for diagnostics that need to show
//  the user what was read so far (e.g. PARSE_MalformedExpression's
//  "The parser successfully read: return x").
//
//  AST nodes are pure data with no behaviour (see Nodes.hpp), so
//  this lives as a free function using the nodeType()+static_cast
//  dispatch idiom already used by ASTPrinter.cpp, rather than as
//  a virtual method on ASTNode.
// ────────────────────────────────────────────────────────────
namespace {
std::string exprToSourceText(const ASTNode& n) {
    const std::string kind = n.nodeType();

    if (kind == "IntLiteral") {
        return std::to_string(static_cast<const IntLiteralNode&>(n).value);
    }
    if (kind == "Identifier") {
        return static_cast<const IdentifierNode&>(n).name;
    }
    if (kind == "BinaryExpr") {
        const auto& b = static_cast<const BinaryExprNode&>(n);
        return exprToSourceText(*b.left) + " " + b.op + " " + exprToSourceText(*b.right);
    }
    if (kind == "AssignmentExpr") {
        const auto& a = static_cast<const AssignmentExprNode&>(n);
        return a.name + " = " + exprToSourceText(*a.value);
    }
    return "<expr>";
}
}  // namespace

Parser::Parser(std::vector<Token> tokens)
    : tokens_(std::move(tokens))
    , pos_(0)
{}

// ────────────────────────────────────────────────────────────
//  parse() — entry point
//
//  Parses:  function_decl* EOF
//  Returns: ProgramNode owning the full tree
// ────────────────────────────────────────────────────────────
StageOutput<std::unique_ptr<ProgramNode>> Parser::parse() {
    // Entry-point attachment (not constructor): keeps the engine's
    // recorder pointer valid even if this Parser object was moved.
    engine_.attachRecorder(&recorder_);

    auto program = std::make_unique<ProgramNode>();
    program->span = SourceSpan::point(1, 1);

    TRACE_SCOPE(recorder_,"Parser::parse()");

    while (!isAtEnd()) {
        // Top level: only function declarations are valid
        if (isTypeName()) {
            auto fn = parseFunctionDecl();
            if (fn) program->functions.push_back(std::move(fn));
        } else {
            emitError("a type keyword to start a function declaration",
                      current().span());
            synchronize();
        }
    }

    StageOutput<std::unique_ptr<ProgramNode>> out;
    out.output      = std::move(program);
    out.diagnostics = std::move(diagnostics_);
    return out;
}

// ────────────────────────────────────────────────────────────
//  parseFunctionDecl
//
//  Grammar: type IDENT '(' param_list ')' block
// ────────────────────────────────────────────────────────────
std::unique_ptr<FunctionDeclNode> Parser::parseFunctionDecl() {
    SourceSpan startSpan = current().span();
    TRACE_SCOPE(recorder_,"Parser::parseFunctionDecl()",
                  "starting at line " + std::to_string(startSpan.startLine));

    std::string retType = parseTypeName();

    Token nameTok = expect(TokenType::IDENTIFIER,
                           "function name after return type");

    expect(TokenType::LPAREN, "'(' after function name");
    auto params = parseParamList();
    expect(TokenType::RPAREN, "')' after parameter list");

    auto body = parseBlock();
    if (!body) return nullptr;

    auto node         = std::make_unique<FunctionDeclNode>();
    node->span        = SourceSpan::range(startSpan.startLine,
                                          startSpan.startCol,
                                          body->span.endLine,
                                          body->span.endCol);
    node->returnType  = retType;
    node->name        = nameTok.lexeme;
    node->params      = std::move(params);
    node->body        = std::move(body);
    return node;
}

// ────────────────────────────────────────────────────────────
//  parseParamList
//
//  Grammar: (type IDENT (',' type IDENT)*)?
// ────────────────────────────────────────────────────────────
std::vector<ParamNode> Parser::parseParamList() {
    std::vector<ParamNode> params;

    if (!isTypeName()) return params;  // empty param list

    do {
        ParamNode p;
        p.span     = current().span();
        p.typeName = parseTypeName();
        Token nameTok = expect(TokenType::IDENTIFIER,
                                "parameter name after type");
        p.name = nameTok.lexeme;
        params.push_back(std::move(p));
    } while (match(TokenType::COMMA));

    return params;
}

// ────────────────────────────────────────────────────────────
//  parseBlock
//
//  Grammar: '{' statement* '}'
// ────────────────────────────────────────────────────────────
std::unique_ptr<BlockStmtNode> Parser::parseBlock() {
    TRACE_SCOPE(recorder_,"Parser::parseBlock()");
    Token lbrace = expect(TokenType::LBRACE, "'{'");

    auto block      = std::make_unique<BlockStmtNode>();
    block->span     = lbrace.span();

    while (!isAtEnd() && !check(TokenType::RBRACE)) {
        auto stmt = parseStatement();
        if (stmt) block->statements.push_back(std::move(stmt));
    }

    Token rbrace = expect(TokenType::RBRACE, "'}' to close block");
    block->span  = SourceSpan::range(lbrace.span().startLine,
                                      lbrace.span().startCol,
                                      rbrace.span().startLine,
                                      rbrace.span().endCol);
    return block;
}

// ────────────────────────────────────────────────────────────
//  parseStatement
//
//  Grammar: var_decl | return_stmt
// ────────────────────────────────────────────────────────────
std::unique_ptr<ASTNode> Parser::parseStatement() {
    TRACE_SCOPE(recorder_,"Parser::parseStatement()", here());
    if (isTypeName())                     return parseVarDecl();
    if (check(TokenType::KW_RETURN))      return parseReturnStmt();

    // Unknown token at statement level — diagnose and recover
    emitError("a statement (type name or 'return')",
              current().span());
    synchronize();
    return nullptr;
}

// ────────────────────────────────────────────────────────────
//  parseVarDecl
//
//  Grammar: type IDENT ('=' expression)? ';'
//  Example: int x = 5;
//           int y;      ← uninitialised (valid)
// ────────────────────────────────────────────────────────────
std::unique_ptr<VarDeclNode> Parser::parseVarDecl() {
    SourceSpan startSpan = current().span();
    TRACE_SCOPE(recorder_,"Parser::parseVarDecl()", here());

    std::string typeName  = parseTypeName();
    Token       nameTok   = expect(TokenType::IDENTIFIER,
                                   "variable name after type");

    auto node       = std::make_unique<VarDeclNode>();
    node->typeName  = typeName;
    node->name      = nameTok.lexeme;

    if (match(TokenType::ASSIGN)) {
        node->initializer = parseExpression();

        // Same two-values-no-operator shape parseReturnStmt() detects:
        // "int x = 5 5;" — without this, the generic expect(SEMICOLON)
        // failure below leaves the stray token unconsumed and the block
        // loop re-diagnoses it, cascading 2 diagnostics for 1 mistake.
        if (!check(TokenType::SEMICOLON) && isExpressionStart(current().type)) {
            SourceSpan  straySpan = current().span();
            std::string leftText  = exprToSourceText(*node->initializer);
            std::string strayText = current().lexeme;
            diagnostics_.push_back(engine_.malformedExpression(
                "int " + node->name + " = " + leftText, leftText, strayText,
                straySpan, "Parser::parseVarDecl()"));
            skipStrayExpressionTokens();
        }
    }

    Token semi     = expect(TokenType::SEMICOLON, "';' after variable declaration");
    node->span     = SourceSpan::range(startSpan.startLine,
                                       startSpan.startCol,
                                       semi.span().endLine,
                                       semi.span().endCol);
    return node;
}

// ────────────────────────────────────────────────────────────
//  parseReturnStmt
//
//  Grammar: 'return' expression? ';'
//
//  After parsing the return expression, a well-formed statement
//  must see ';' next. If instead the current token ALSO starts a
//  valid expression (e.g. "return x 2;"), that is not a generic
//  "missing token" error — the two expressions are individually
//  valid, just missing an operator between them. Diagnosing that
//  shape specifically (PARSE_MalformedExpression) lets the message
//  show both values and suggest an operator, instead of the
//  generic "expected ';', found '2'".
// ────────────────────────────────────────────────────────────
std::unique_ptr<ReturnStmtNode> Parser::parseReturnStmt() {
    TRACE_SCOPE(recorder_,"Parser::parseReturnStmt()", here());
    Token kwTok = expect(TokenType::KW_RETURN, "'return'");

    auto node  = std::make_unique<ReturnStmtNode>();

    if (!check(TokenType::SEMICOLON)) {
        node->value = parseExpression();

        if (!check(TokenType::SEMICOLON) && isExpressionStart(current().type)) {
            SourceSpan  straySpan = current().span();
            std::string leftText  = exprToSourceText(*node->value);
            std::string strayText = current().lexeme;
            diagnostics_.push_back(engine_.malformedExpression(
                "return " + leftText, leftText, strayText, straySpan,
                "Parser::parseReturnStmt()"));
            skipStrayExpressionTokens();
        }
    }

    Token semi = expect(TokenType::SEMICOLON, "';' after return value");
    node->span = SourceSpan::range(kwTok.span().startLine,
                                    kwTok.span().startCol,
                                    semi.span().endLine,
                                    semi.span().endCol);
    return node;
}

// ────────────────────────────────────────────────────────────
//  Expression parsing — precedence encoded in call hierarchy
//
//  Each level handles operators of the same precedence
//  by looping (left-associative), then calling the next
//  higher-precedence level.
//
//  Example: 1 + 2 * 3
//    parseAddition calls parseMultiplication for '1'
//    sees '+', calls parseMultiplication again for '2 * 3'
//    parseMultiplication sees '*', gives back Mult(2,3)
//    parseAddition assembles Add(1, Mult(2,3))
// ────────────────────────────────────────────────────────────

std::unique_ptr<ASTNode> Parser::parseExpression() {
    return parseAssignment();
}

// ────────────────────────────────────────────────────────────
//  parseAssignment
//
//  Grammar: IDENTIFIER '=' assignment | equality
//
//  Lowest precedence, right-associative — same shape as real
//  C++ (a = b = c parses as a = (b = c)). Parsed by first
//  trying the NEXT-higher level (equality) for the left side,
//  then checking whether '=' follows:
//    - if it does, 'left' must already be a bare IdentifierNode
//      (this language has no pointers/arrays, so a name is the
//      only valid assignment target) — anything else is
//      PARSE_InvalidAssignmentTarget, not a generic token error,
//      because the shape that was found IS a valid expression,
//      just not an assignable one.
//    - if it doesn't, 'left' is returned unchanged, so plain
//      "x + 2" still costs nothing extra beyond one wasted check.
//
//  Recursing into parseAssignment() (not parseEquality()) for
//  the right-hand side is what makes this right-associative.
// ────────────────────────────────────────────────────────────
std::unique_ptr<ASTNode> Parser::parseAssignment() {
    TRACE_SCOPE(recorder_,"Parser::parseAssignment()");
    auto left = parseEquality();

    if (check(TokenType::ASSIGN)) {
        SourceSpan opSpan = current().span();

        if (left->nodeType() != "Identifier") {
            diagnostics_.push_back(
                engine_.invalidAssignmentTarget(left->nodeType(), opSpan));
            // Consume '=' and its right-hand side too (parsed, then
            // discarded) so the token stream still lands on ';' —
            // otherwise the caller's expect(SEMICOLON) immediately
            // re-fails on the same unconsumed '=', cascading into two
            // more spurious diagnostics for one underlying mistake.
            advance();
            parseAssignment();
            return left;
        }

        std::string name = static_cast<IdentifierNode*>(left.get())->name;
        advance();  // consume '='
        auto right = parseAssignment();

        auto node   = std::make_unique<AssignmentExprNode>();
        node->span  = opSpan;
        node->name  = name;
        node->value = std::move(right);
        return node;
    }

    return left;
}

std::unique_ptr<ASTNode> Parser::parseEquality() {
    auto left = parseComparison();
    while (check(TokenType::EQUAL) || check(TokenType::NOT_EQUAL)) {
        std::string op = current().lexeme;
        SourceSpan  opSpan = current().span();
        advance();
        auto right = parseComparison();
        auto node       = std::make_unique<BinaryExprNode>();
        node->op        = op;
        node->span      = opSpan;
        node->left      = std::move(left);
        node->right     = std::move(right);
        left = std::move(node);
    }
    return left;
}

std::unique_ptr<ASTNode> Parser::parseComparison() {
    auto left = parseAddition();
    while (check(TokenType::LESS) || check(TokenType::GREATER)) {
        std::string op = current().lexeme;
        SourceSpan  opSpan = current().span();
        advance();
        auto right = parseAddition();
        auto node       = std::make_unique<BinaryExprNode>();
        node->op        = op;
        node->span      = opSpan;
        node->left      = std::move(left);
        node->right     = std::move(right);
        left = std::move(node);
    }
    return left;
}

std::unique_ptr<ASTNode> Parser::parseAddition() {
    auto left = parseMultiplication();
    while (check(TokenType::PLUS) || check(TokenType::MINUS)) {
        std::string op = current().lexeme;
        SourceSpan  opSpan = current().span();
        advance();
        auto right = parseMultiplication();
        auto node       = std::make_unique<BinaryExprNode>();
        node->op        = op;
        node->span      = opSpan;
        node->left      = std::move(left);
        node->right     = std::move(right);
        left = std::move(node);
    }
    return left;
}

std::unique_ptr<ASTNode> Parser::parseMultiplication() {
    auto left = parsePrimary();
    while (check(TokenType::STAR) || check(TokenType::SLASH)) {
        std::string op = current().lexeme;
        SourceSpan  opSpan = current().span();
        advance();
        auto right = parsePrimary();
        auto node       = std::make_unique<BinaryExprNode>();
        node->op        = op;
        node->span      = opSpan;
        node->left      = std::move(left);
        node->right     = std::move(right);
        left = std::move(node);
    }
    return left;
}

std::unique_ptr<ASTNode> Parser::parsePrimary() {
    TRACE_SCOPE(recorder_,"Parser::parsePrimary()", here());

    // Integer literal
    if (check(TokenType::INTEGER_LITERAL)) {
        Token tok = advance();
        auto  node  = std::make_unique<IntLiteralNode>();
        node->span  = tok.span();
        // NOT std::stoi: stoi throws std::out_of_range on values past
        // INT_MAX, which would crash the whole compiler on input like
        // "return 99999999999;". The lexer has already diagnosed
        // out-of-range literals (LEX_IntegerOutOfRange); this clamp
        // only keeps the parser crash-proof when it's driven directly
        // on unvalidated tokens, as the unit tests do.
        long long v = 0;
        for (char ch : tok.lexeme) {
            v = v * 10 + (ch - '0');
            if (v > 2147483647LL) { v = 2147483647LL; break; }
        }
        node->value = (int)v;
        return node;
    }

    // Identifier
    if (check(TokenType::IDENTIFIER)) {
        Token tok  = advance();
        auto  node = std::make_unique<IdentifierNode>();
        node->span = tok.span();
        node->name = tok.lexeme;
        return node;
    }

    // Grouped expression: ( expr )
    if (check(TokenType::LPAREN)) {
        advance();  // consume '('
        auto inner = parseExpression();
        expect(TokenType::RPAREN, "')' to close grouped expression");
        return inner;
    }

    // Nothing matched — diagnose
    emitError("an expression (number, variable, or '(')",
              current().span());

    // Return a dummy node so parsing can continue
    auto dummy  = std::make_unique<IntLiteralNode>();
    dummy->span = current().span();
    dummy->value = 0;
    return dummy;
}

// ────────────────────────────────────────────────────────────
//  Type parsing
// ────────────────────────────────────────────────────────────

std::string Parser::parseTypeName() {
    if (check(TokenType::KW_INT)) {
        advance();
        return "int";
    }
    emitError("a type name (e.g. 'int')", current().span());
    return "unknown";
}

bool Parser::isTypeName() const {
    return check(TokenType::KW_INT);
}

bool Parser::isExpressionStart(TokenType t) const {
    switch (t) {
        case TokenType::INTEGER_LITERAL:
        case TokenType::IDENTIFIER:
        case TokenType::LPAREN:
            return true;
        default:
            return false;
    }
}

// ────────────────────────────────────────────────────────────
//  Token management helpers
// ────────────────────────────────────────────────────────────

const Token& Parser::current() const {
    if (pos_ < tokens_.size()) return tokens_[pos_];
    return tokens_.back();  // always EOF
}

// Detail text for recorded trace frames. TraceScope placement rule:
// every method where a diagnostic can fire gets a scope (statements,
// parseAssignment, parsePrimary), plus the structural landmarks
// (parse/parseFunctionDecl/parseBlock/parseStatement). The middle
// precedence levels (equality..multiplication) cannot fail on their
// own, so they are deliberately not recorded — omitting a frame
// never falsifies the path, it only shortens it.
std::string Parser::here() const {
    const Token& t = current();
    std::string what = t.lexeme.empty()
        ? tokenTypeName(t.type)
        : "'" + t.lexeme + "'";
    return "at " + what + " (line " + std::to_string(t.line) + ")";
}

Token Parser::advance() {
    Token t = current();
    if (!isAtEnd()) ++pos_;
    return t;
}

bool Parser::check(TokenType t) const {
    return current().type == t;
}

bool Parser::match(TokenType t) {
    if (!check(t)) return false;
    advance();
    return true;
}

// expect: if current token is of the right type, consume and
// return it; otherwise emit a diagnostic and return a synthetic
// token so parsing continues without crashing.
Token Parser::expect(TokenType t, const std::string& what) {
    if (check(t)) return advance();

    SourceSpan span = current().span();
    diagnostics_.push_back(
        engine_.unexpectedToken(current().lexeme, what, span));

    // Synthetic token so callers don't crash — parsing continues
    Token fake;
    fake.type   = t;
    fake.lexeme = "";
    fake.line   = span.startLine;
    fake.column = span.startCol;
    return fake;
}

bool Parser::isAtEnd() const {
    return current().type == TokenType::END_OF_FILE;
}

// ────────────────────────────────────────────────────────────
//  Error recovery — panic mode synchronisation
//
//  On error: skip tokens until a statement boundary.
//  This is the same technique GCC and Clang use to recover
//  enough to report additional errors after the first one.
// ────────────────────────────────────────────────────────────
void Parser::synchronize() {
    while (!isAtEnd()) {
        // A semicolon ends a statement — safe to resume after it
        if (check(TokenType::SEMICOLON)) { advance(); return; }
        // A '}' closes a block — resume after it
        if (check(TokenType::RBRACE))    { advance(); return; }
        // A type keyword or 'return' starts a new statement
        if (isTypeName() || check(TokenType::KW_RETURN)) return;
        advance();
    }
}

// Stops at (without consuming) anything that can legitimately end or
// begin a statement — ';', '}', a type keyword, or 'return' — the same
// boundaries synchronize() uses. Unlike synchronize() it does NOT eat
// the ';': the caller's expect(SEMICOLON) should consume it normally
// so the statement's span still ends at the real semicolon.
void Parser::skipStrayExpressionTokens() {
    while (!isAtEnd() &&
           !check(TokenType::SEMICOLON) &&
           !check(TokenType::RBRACE) &&
           !check(TokenType::KW_RETURN) &&
           !isTypeName()) {
        advance();
    }
}

void Parser::emitError(const std::string& message, SourceSpan span) {
    diagnostics_.push_back(
        engine_.unexpectedToken(current().lexeme, message, span));
}
