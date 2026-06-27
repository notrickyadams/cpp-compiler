#include "Parser.hpp"

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
    auto program = std::make_unique<ProgramNode>();
    program->span = SourceSpan::point(1, 1);

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

    std::string typeName  = parseTypeName();
    Token       nameTok   = expect(TokenType::IDENTIFIER,
                                   "variable name after type");

    auto node       = std::make_unique<VarDeclNode>();
    node->typeName  = typeName;
    node->name      = nameTok.lexeme;

    if (match(TokenType::ASSIGN)) {
        node->initializer = parseExpression();
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
// ────────────────────────────────────────────────────────────
std::unique_ptr<ReturnStmtNode> Parser::parseReturnStmt() {
    Token kwTok = expect(TokenType::KW_RETURN, "'return'");

    auto node  = std::make_unique<ReturnStmtNode>();

    if (!check(TokenType::SEMICOLON)) {
        node->value = parseExpression();
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
    return parseEquality();
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
    // Integer literal
    if (check(TokenType::INTEGER_LITERAL)) {
        Token tok = advance();
        auto  node  = std::make_unique<IntLiteralNode>();
        node->span  = tok.span();
        node->value = std::stoi(tok.lexeme);
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

// ────────────────────────────────────────────────────────────
//  Token management helpers
// ────────────────────────────────────────────────────────────

const Token& Parser::current() const {
    if (pos_ < tokens_.size()) return tokens_[pos_];
    return tokens_.back();  // always EOF
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

void Parser::emitError(const std::string& message, SourceSpan span) {
    diagnostics_.push_back(
        engine_.unexpectedToken(current().lexeme, message, span));
}
