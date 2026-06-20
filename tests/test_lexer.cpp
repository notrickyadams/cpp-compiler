#include "test_runner.hpp"
#include "../src/lexer/Lexer.hpp"
#include <vector>

// ============================================================
//  Helper — lex a string and return the token type sequence.
//  Strips the trailing EOF token so tests are less verbose.
// ============================================================
static std::vector<TokenType> types(const std::string& src) {
    Lexer lexer(src);
    std::vector<Token> toks = lexer.tokenize();
    std::vector<TokenType> result;
    for (auto& t : toks) {
        if (t.type != TokenType::END_OF_FILE) result.push_back(t.type);
    }
    return result;
}

static std::vector<std::string> lexemes(const std::string& src) {
    Lexer lexer(src);
    std::vector<Token> toks = lexer.tokenize();
    std::vector<std::string> result;
    for (auto& t : toks) {
        if (t.type != TokenType::END_OF_FILE) result.push_back(t.lexeme);
    }
    return result;
}

// ── Required output test (from spec) ─────────────────────────
TEST("required_output: return x + 5 ;", [](){
    Lexer lexer("return x + 5;");
    auto toks = lexer.tokenize();
    // Expected: RETURN IDENTIFIER(x) PLUS INTEGER(5) SEMICOLON EOF
    ASSERT_EQ((int)toks.size(), 6);
    ASSERT_EQ(toks[0].type, TokenType::KW_RETURN);
    ASSERT_EQ(toks[0].lexeme, std::string("return"));
    ASSERT_EQ(toks[1].type, TokenType::IDENTIFIER);
    ASSERT_EQ(toks[1].lexeme, std::string("x"));
    ASSERT_EQ(toks[2].type, TokenType::PLUS);
    ASSERT_EQ(toks[3].type, TokenType::INTEGER_LITERAL);
    ASSERT_EQ(toks[3].lexeme, std::string("5"));
    ASSERT_EQ(toks[4].type, TokenType::SEMICOLON);
    ASSERT_EQ(toks[5].type, TokenType::END_OF_FILE);
    ASSERT_TRUE(!lexer.hasErrors());
});

// ── Keywords ─────────────────────────────────────────────────
TEST("keywords: int and return", [](){
    auto t = types("int return");
    ASSERT_EQ((int)t.size(), 2);
    ASSERT_EQ(t[0], TokenType::KW_INT);
    ASSERT_EQ(t[1], TokenType::KW_RETURN);
});

// ── Identifier vs keyword (maximal munch) ────────────────────
TEST("maximal_munch: returnx is IDENTIFIER not RETURN", [](){
    auto t = types("returnx");
    ASSERT_EQ((int)t.size(), 1);
    ASSERT_EQ(t[0], TokenType::IDENTIFIER);
});

TEST("maximal_munch: integer42 is IDENTIFIER", [](){
    auto t = types("integer42");
    ASSERT_EQ((int)t.size(), 1);
    ASSERT_EQ(t[0], TokenType::IDENTIFIER);
    ASSERT_EQ(lexemes("integer42")[0], std::string("integer42"));
});

// ── Integers ─────────────────────────────────────────────────
TEST("integer: single digit", [](){
    auto t = types("0");
    ASSERT_EQ((int)t.size(), 1);
    ASSERT_EQ(t[0], TokenType::INTEGER_LITERAL);
});

TEST("integer: multi digit", [](){
    ASSERT_EQ(lexemes("12345")[0], std::string("12345"));
});

// ── Operators ────────────────────────────────────────────────
TEST("operators: + - * /", [](){
    auto t = types("+ - * /");
    ASSERT_EQ((int)t.size(), 4);
    ASSERT_EQ(t[0], TokenType::PLUS);
    ASSERT_EQ(t[1], TokenType::MINUS);
    ASSERT_EQ(t[2], TokenType::STAR);
    ASSERT_EQ(t[3], TokenType::SLASH);
});

TEST("operator: = vs ==", [](){
    auto t = types("= ==");
    ASSERT_EQ((int)t.size(), 2);
    ASSERT_EQ(t[0], TokenType::ASSIGN);
    ASSERT_EQ(t[1], TokenType::EQUAL);
});

TEST("operator: !=", [](){
    auto t = types("!=");
    ASSERT_EQ((int)t.size(), 1);
    ASSERT_EQ(t[0], TokenType::NOT_EQUAL);
});

// ── Delimiters ────────────────────────────────────────────────
TEST("delimiters: { ( ) } ;", [](){
    auto t = types("{ ( ) } ;");
    ASSERT_EQ((int)t.size(), 5);
    ASSERT_EQ(t[0], TokenType::LBRACE);
    ASSERT_EQ(t[1], TokenType::LPAREN);
    ASSERT_EQ(t[2], TokenType::RPAREN);
    ASSERT_EQ(t[3], TokenType::RBRACE);
    ASSERT_EQ(t[4], TokenType::SEMICOLON);
});

// ── Whitespace & newlines ignored ────────────────────────────
TEST("whitespace: tabs newlines ignored", [](){
    auto t = types("int\n\t x\r\n  =\t5 ;");
    ASSERT_EQ((int)t.size(), 5);
    ASSERT_EQ(t[0], TokenType::KW_INT);
    ASSERT_EQ(t[1], TokenType::IDENTIFIER);
    ASSERT_EQ(t[2], TokenType::ASSIGN);
    ASSERT_EQ(t[3], TokenType::INTEGER_LITERAL);
    ASSERT_EQ(t[4], TokenType::SEMICOLON);
});

// ── Single-line comment stripped ─────────────────────────────
TEST("comment: single-line // stripped", [](){
    auto t = types("int x; // this is a comment\nreturn x;");
    ASSERT_EQ((int)t.size(), 6); // int x ; return x ;
    ASSERT_EQ(t[3], TokenType::KW_RETURN);
});

// ── Block comment stripped ────────────────────────────────────
TEST("comment: block /* */ stripped", [](){
    auto t = types("int /* hello world */ x;");
    ASSERT_EQ((int)t.size(), 3);
    ASSERT_EQ(t[0], TokenType::KW_INT);
    ASSERT_EQ(t[1], TokenType::IDENTIFIER);
    ASSERT_EQ(t[2], TokenType::SEMICOLON);
});

// ── Source location tracking ─────────────────────────────────
TEST("source_location: line and column correct", [](){
    Lexer lexer("int\nreturn");
    auto toks = lexer.tokenize();
    ASSERT_EQ(toks[0].line, 1);
    ASSERT_EQ(toks[1].line, 2);
    ASSERT_EQ(toks[1].column, 1);
});

// ── Full target program ───────────────────────────────────────
TEST("full_program: int main(){ int x=5; return x+2; }", [](){
    std::string src =
        "int main() {\n"
        "    int x = 5;\n"
        "    return x + 2;\n"
        "}\n";
    Lexer lexer(src);
    auto toks = lexer.tokenize();
    ASSERT_TRUE(!lexer.hasErrors());
    // int main ( ) { int x = 5 ; return x + 2 ; } EOF  = 17 tokens
    ASSERT_EQ((int)toks.size(), 17);
    ASSERT_EQ(toks[0].type,  TokenType::KW_INT);
    ASSERT_EQ(toks[1].type,  TokenType::IDENTIFIER);
    ASSERT_EQ(toks[1].lexeme, std::string("main"));
    ASSERT_EQ(toks[16].type, TokenType::END_OF_FILE);
});

// ── Error recovery ────────────────────────────────────────────
TEST("error: unknown character @ reported", [](){
    Lexer lexer("int @x;");
    auto toks = lexer.tokenize();
    ASSERT_TRUE(lexer.hasErrors());
    ASSERT_EQ((int)lexer.errors().size(), 1);
    // Lexer continues after error — still produces int, x, ;, EOF
    bool foundIdent = false;
    for (auto& t : toks) {
        if (t.type == TokenType::IDENTIFIER) foundIdent = true;
    }
    ASSERT_TRUE(foundIdent);
});

// ── Empty input ───────────────────────────────────────────────
TEST("edge: empty source yields only EOF", [](){
    Lexer lexer("");
    auto toks = lexer.tokenize();
    ASSERT_EQ((int)toks.size(), 1);
    ASSERT_EQ(toks[0].type, TokenType::END_OF_FILE);
});

// ── Only whitespace ───────────────────────────────────────────
TEST("edge: only whitespace yields only EOF", [](){
    Lexer lexer("   \n\t  ");
    auto toks = lexer.tokenize();
    ASSERT_EQ((int)toks.size(), 1);
    ASSERT_EQ(toks[0].type, TokenType::END_OF_FILE);
});

// ── main ─────────────────────────────────────────────────────
int main() {
    std::cout << "=== Lexer Unit Tests ===\n\n";
    return RUN_ALL_TESTS();
}
