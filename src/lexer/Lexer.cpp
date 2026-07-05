#include "Lexer.hpp"
#include <cctype>

Lexer::Lexer(std::string source)
    : source_(std::move(source))
    , pos_(0)
    , line_(1)
    , col_(1)
{}

// ────────────────────────────────────────────────────────────
//  tokenize() — public entry point
// ────────────────────────────────────────────────────────────
StageOutput<std::vector<Token>> Lexer::tokenize() {
    // (Re-)attach here rather than in the constructor: entry-point
    // attachment keeps the engine's recorder pointer valid even if
    // this Lexer object was moved after construction.
    engine_.attachRecorder(&recorder_);
    pendingDiagnostics_.clear();
    std::vector<Token> tokens;

    TRACE_SCOPE(recorder_,"Lexer::tokenize()");

    while (true) {
        Token tok = nextToken();
        tokens.push_back(tok);
        if (tok.type == TokenType::END_OF_FILE) break;
    }

    StageOutput<std::vector<Token>> out;
    out.output      = std::move(tokens);
    out.diagnostics = std::move(pendingDiagnostics_);
    lastOutput_     = out;   // shallow copy for hasErrors()/errors() helpers
    return out;              // NRVO: returned directly, no extra copy
}

// ────────────────────────────────────────────────────────────
//  nextToken
// ────────────────────────────────────────────────────────────
Token Lexer::nextToken() {
    skipWhitespaceAndComments();
    if (isAtEnd()) return makeToken(TokenType::END_OF_FILE, "", line_, col_);

    TRACE_SCOPE(recorder_,"Lexer::nextToken()",
                  TraceDetail::lineCol("at line ", line_, col_));

    char c = peek();
    if (std::isalpha(c) || c == '_') return lexIdentifierOrKeyword();
    if (std::isdigit(c))             return lexNumber();
    return lexSymbol();
}

// ────────────────────────────────────────────────────────────
//  skipWhitespaceAndComments
//  v2 addition: diagnoses unterminated block comments
// ────────────────────────────────────────────────────────────
void Lexer::skipWhitespaceAndComments() {
    while (!isAtEnd()) {
        char c = peek();

        if (c == ' ' || c == '\t' || c == '\r') {
            advance();
        } else if (c == '\n') {
            advance(); line_++; col_ = 1;
        } else if (c == '/' && peekNext() == '/') {
            while (!isAtEnd() && peek() != '\n') advance();
        } else if (c == '/' && peekNext() == '*') {
            int startLine = line_;
            int startCol  = col_;
            TRACE_SCOPE(recorder_,"Lexer::skipWhitespaceAndComments()",
                          TraceDetail::lineCol(
                              "inside block comment opened at line ",
                              startLine, startCol));
            advance(); advance(); // consume '/*'
            bool closed = false;
            while (!isAtEnd()) {
                if (peek() == '\n')          { advance(); line_++; col_ = 1; }
                else if (peek() == '*' && peekNext() == '/') {
                    advance(); advance(); // consume '*/'
                    closed = true;
                    break;
                } else { advance(); }
            }
            if (!closed) {
                // Unterminated block comment — diagnose and stop scanning
                SourceSpan span = SourceSpan::token(startLine, startCol, 2);
                pendingDiagnostics_.push_back(
                    engine_.unterminatedComment(span));
            }
        } else {
            break;
        }
    }
}

// ────────────────────────────────────────────────────────────
//  lexIdentifierOrKeyword
// ────────────────────────────────────────────────────────────
Token Lexer::lexIdentifierOrKeyword() {
    int startLine = line_;
    int startCol  = col_;
    std::string lexeme;

    while (!isAtEnd() && (std::isalnum(peek()) || peek() == '_')) {
        lexeme += advance();
    }

    const auto& kw = keywords();
    auto it = kw.find(lexeme);
    TokenType type = (it != kw.end()) ? it->second : TokenType::IDENTIFIER;
    return makeToken(type, lexeme, startLine, startCol);
}

// ────────────────────────────────────────────────────────────
//  lexNumber
// ────────────────────────────────────────────────────────────
Token Lexer::lexNumber() {
    int startLine = line_;
    int startCol  = col_;
    TRACE_SCOPE(recorder_,"Lexer::lexNumber()", TraceDetail::lit("scanning digits"));
    std::string lexeme;

    while (!isAtEnd() && std::isdigit(peek())) {
        lexeme += advance();
    }

    // Range check: the digits are lexically valid, but the VALUE must
    // fit in a 32-bit int or downstream stages would silently wrap it
    // (and std::stoi in the parser would throw). Compare against
    // INT_MAX textually — leading zeros are stripped first so
    // "0000000001" isn't misjudged by its raw length.
    std::size_t firstNonZero = lexeme.find_first_not_of('0');
    std::string digits = (firstNonZero == std::string::npos)
                             ? "0" : lexeme.substr(firstNonZero);
    if (digits.size() > 10 ||
        (digits.size() == 10 && digits > "2147483647")) {
        pendingDiagnostics_.push_back(engine_.integerOutOfRange(
            lexeme, SourceSpan::token(startLine, startCol, (int)lexeme.size())));
    }

    return makeToken(TokenType::INTEGER_LITERAL, lexeme, startLine, startCol);
}

// ────────────────────────────────────────────────────────────
//  lexSymbol
// ────────────────────────────────────────────────────────────
Token Lexer::lexSymbol() {
    int startLine = line_;
    int startCol  = col_;
    TRACE_SCOPE(recorder_,"Lexer::lexSymbol()",
                  TraceDetail::chr("dispatching on '", peek()));
    char c = advance();

    switch (c) {
        case '+': return makeToken(TokenType::PLUS,      "+", startLine, startCol);
        case '-': return makeToken(TokenType::MINUS,     "-", startLine, startCol);
        case '*': return makeToken(TokenType::STAR,      "*", startLine, startCol);
        case '/': return makeToken(TokenType::SLASH,     "/", startLine, startCol);
        case ';': return makeToken(TokenType::SEMICOLON, ";", startLine, startCol);
        case '(': return makeToken(TokenType::LPAREN,    "(", startLine, startCol);
        case ')': return makeToken(TokenType::RPAREN,    ")", startLine, startCol);
        case '{': return makeToken(TokenType::LBRACE,    "{", startLine, startCol);
        case '}': return makeToken(TokenType::RBRACE,    "}", startLine, startCol);
        case ',': return makeToken(TokenType::COMMA,     ",", startLine, startCol);
        case '<': return makeToken(TokenType::LESS,      "<", startLine, startCol);
        case '>': return makeToken(TokenType::GREATER,   ">", startLine, startCol);

        case '=':
            if (match('=')) return makeToken(TokenType::EQUAL,    "==", startLine, startCol);
            return makeToken(TokenType::ASSIGN, "=", startLine, startCol);

        case '!':
            if (match('=')) return makeToken(TokenType::NOT_EQUAL, "!=", startLine, startCol);
            return errorToken(c);

        default:
            return errorToken(c);
    }
}

// ────────────────────────────────────────────────────────────
//  Primitives
// ────────────────────────────────────────────────────────────

char Lexer::peek() const {
    if (isAtEnd()) return '\0';
    return source_[pos_];
}

char Lexer::peekNext() const {
    if (pos_ + 1 >= source_.size()) return '\0';
    return source_[pos_ + 1];
}

char Lexer::advance() {
    char c = source_[pos_++];
    col_++;
    return c;
}

bool Lexer::isAtEnd() const {
    return pos_ >= source_.size();
}

bool Lexer::match(char expected) {
    if (isAtEnd() || source_[pos_] != expected) return false;
    advance();
    return true;
}

Token Lexer::makeToken(TokenType type, const std::string& lexeme,
                        int startLine, int startCol) const {
    Token t;
    t.type   = type;
    t.lexeme = lexeme;
    t.line   = startLine;
    t.column = startCol;
    return t;
}

Token Lexer::errorToken(char c) {
    SourceSpan span = SourceSpan::point(line_, col_ - 1);
    pendingDiagnostics_.push_back(engine_.unexpectedChar(c, span));

    Token t;
    t.type   = TokenType::UNKNOWN;
    t.lexeme = std::string(1, c);
    t.line   = line_;
    t.column = col_ - 1;
    return t;
}
