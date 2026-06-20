#include "Lexer.hpp"
#include <cctype>
#include <stdexcept>

// ────────────────────────────────────────────────────────────
//  Constructor
// ────────────────────────────────────────────────────────────
Lexer::Lexer(std::string source)
    : source_(std::move(source))  // move — no copy of potentially large string
    , pos_(0)
    , line_(1)
    , col_(1)
{}

// ────────────────────────────────────────────────────────────
//  tokenize() — public entry point
//
//  Repeatedly calls nextToken() until EOF.
//  Guarantees: result.back().type == END_OF_FILE always.
//
//  Time complexity: O(n) where n = source length.
//  Each character is visited at most twice (peek + advance).
// ────────────────────────────────────────────────────────────
std::vector<Token> Lexer::tokenize() {
    std::vector<Token> tokens;

    while (true) {
        Token tok = nextToken();
        tokens.push_back(tok);
        if (tok.type == TokenType::END_OF_FILE) break;
    }

    return tokens;
}

// ────────────────────────────────────────────────────────────
//  nextToken() — produce exactly one token from current pos_
//
//  Flow:
//    1. Skip whitespace / comments
//    2. Record start position for error messages
//    3. Dispatch on first character
// ────────────────────────────────────────────────────────────
Token Lexer::nextToken() {
    skipWhitespaceAndComments();

    if (isAtEnd()) {
        return makeToken(TokenType::END_OF_FILE, "");
    }

    char c = peek();

    if (std::isalpha(c) || c == '_') return lexIdentifierOrKeyword();
    if (std::isdigit(c))             return lexNumber();
    return lexSymbol();
}

// ────────────────────────────────────────────────────────────
//  skipWhitespaceAndComments()
//
//  Handles:
//    • spaces, tabs, carriage returns
//    • newlines (increments line counter)
//    • // single-line comments
//    • /* */ block comments (nested NOT supported — matches C)
// ────────────────────────────────────────────────────────────
void Lexer::skipWhitespaceAndComments() {
    while (!isAtEnd()) {
        char c = peek();

        if (c == ' ' || c == '\t' || c == '\r') {
            advance();

        } else if (c == '\n') {
            advance();
            line_++;
            col_ = 1;   // reset column on new line

        } else if (c == '/' && peekNext() == '/') {
            // Single-line comment: consume until end of line
            while (!isAtEnd() && peek() != '\n') advance();

        } else if (c == '/' && peekNext() == '*') {
            // Block comment: consume until */
            advance(); advance();  // consume '/' and '*'
            while (!isAtEnd()) {
                if (peek() == '\n') { advance(); line_++; col_ = 1; }
                else if (peek() == '*' && peekNext() == '/') {
                    advance(); advance();  // consume '*' and '/'
                    break;
                } else {
                    advance();
                }
            }
        } else {
            break;  // real token starts here
        }
    }
}

// ────────────────────────────────────────────────────────────
//  lexIdentifierOrKeyword()
//
//  Greedy match: consume [a-zA-Z_][a-zA-Z0-9_]*
//  Then look up in keyword table.
//
//  Industry note: this is called "maximal munch" — the lexer
//  always takes the longest possible match. "returnx" is an
//  IDENTIFIER, not RETURN + IDENTIFIER.
// ────────────────────────────────────────────────────────────
Token Lexer::lexIdentifierOrKeyword() {
    int startLine = line_;
    int startCol  = col_;
    std::string lexeme;

    while (!isAtEnd() && (std::isalnum(peek()) || peek() == '_')) {
        lexeme += advance();
    }

    // Keyword lookup — O(1) average via unordered_map
    const auto& kw = keywords();
    auto it = kw.find(lexeme);
    TokenType type = (it != kw.end()) ? it->second : TokenType::IDENTIFIER;

    // Use makeToken but override line/col to start of lexeme
    Token t;
    t.type   = type;
    t.lexeme = lexeme;
    t.line   = startLine;
    t.column = startCol;
    return t;
}

// ────────────────────────────────────────────────────────────
//  lexNumber()
//
//  Currently handles unsigned decimal integers only.
//  Extension points (marked TODO) show where hex/float/suffix
//  support would be added — as a real compiler would need.
// ────────────────────────────────────────────────────────────
Token Lexer::lexNumber() {
    int startLine = line_;
    int startCol  = col_;
    std::string lexeme;

    while (!isAtEnd() && std::isdigit(peek())) {
        lexeme += advance();
    }

    // TODO: handle '.' for float literals
    // TODO: handle "0x" prefix for hex literals
    // TODO: handle 'u', 'l', 'll' suffixes

    Token t;
    t.type   = TokenType::INTEGER_LITERAL;
    t.lexeme = lexeme;
    t.line   = startLine;
    t.column = startCol;
    return t;
}

// ────────────────────────────────────────────────────────────
//  lexSymbol()
//
//  Single- and double-character operator/punctuation tokens.
//  Uses match() for two-char lookahead (==, !=).
// ────────────────────────────────────────────────────────────
Token Lexer::lexSymbol() {
    int startLine = line_;
    int startCol  = col_;
    char c = advance();

    switch (c) {
        case '+': return makeToken(TokenType::PLUS,      "+");
        case '-': return makeToken(TokenType::MINUS,     "-");
        case '*': return makeToken(TokenType::STAR,      "*");
        case '/': return makeToken(TokenType::SLASH,     "/");
        case ';': return makeToken(TokenType::SEMICOLON, ";");
        case '(': return makeToken(TokenType::LPAREN,    "(");
        case ')': return makeToken(TokenType::RPAREN,    ")");
        case '{': return makeToken(TokenType::LBRACE,    "{");
        case '}': return makeToken(TokenType::RBRACE,    "}");
        case ',': return makeToken(TokenType::COMMA,     ",");
        case '<': return makeToken(TokenType::LESS,      "<");
        case '>': return makeToken(TokenType::GREATER,   ">");

        case '=':
            if (match('=')) return makeToken(TokenType::EQUAL,  "==");
            return makeToken(TokenType::ASSIGN, "=");

        case '!':
            if (match('=')) return makeToken(TokenType::NOT_EQUAL, "!=");
            return errorToken(std::string("unexpected character '") + c + "'");

        default:
            return errorToken(std::string("unexpected character '") + c + "'");
    }
    // Note: startLine/startCol captured but makeToken uses current line_/col_.
    // For single-char tokens they're the same. For multi-char we'd pass them.
    (void)startLine; (void)startCol;
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

// advance() — consumes current character and moves pos_ forward.
// Also increments column counter (newlines handled in skip).
char Lexer::advance() {
    char c = source_[pos_++];
    col_++;
    return c;
}

bool Lexer::isAtEnd() const {
    return pos_ >= source_.size();
}

// match() — conditional advance: only consumes if next char matches.
// Used for two-character tokens like == and !=.
bool Lexer::match(char expected) {
    if (isAtEnd() || source_[pos_] != expected) return false;
    advance();
    return true;
}

Token Lexer::makeToken(TokenType type, const std::string& lexeme) const {
    Token t;
    t.type   = type;
    t.lexeme = lexeme;
    t.line   = line_;
    t.column = col_;
    return t;
}

Token Lexer::errorToken(const std::string& msg) {
    errors_.push_back({ msg, line_, col_ });
    Token t;
    t.type   = TokenType::UNKNOWN;
    t.lexeme = "";
    t.line   = line_;
    t.column = col_;
    return t;
}
