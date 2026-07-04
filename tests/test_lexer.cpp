#include "test_runner.hpp"
#include "../src/lexer/Lexer.hpp"
#include <vector>

// ============================================================
//  Helpers — tokenize and extract just types or lexemes.
//  v2: tokenize() returns StageOutput, so we call .output
// ============================================================
static std::vector<TokenType> types(const std::string& src) {
    Lexer lexer(src);
    auto result = lexer.tokenize();
    std::vector<TokenType> out;
    for (auto& t : result.output)
        if (t.type != TokenType::END_OF_FILE) out.push_back(t.type);
    return out;
}

static std::vector<std::string> lexemes(const std::string& src) {
    Lexer lexer(src);
    auto result = lexer.tokenize();
    std::vector<std::string> out;
    for (auto& t : result.output)
        if (t.type != TokenType::END_OF_FILE) out.push_back(t.lexeme);
    return out;
}

// ── Required output (from spec) ──────────────────────────────
TEST("required_output: return x + 5 ;", [](){
    Lexer lexer("return x + 5;");
    auto result = lexer.tokenize();
    auto& toks = result.output;
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
    ASSERT_TRUE(!result.hasErrors());
});

// ── Keywords ──────────────────────────────────────────────────
TEST("keywords: int and return", [](){
    auto t = types("int return");
    ASSERT_EQ((int)t.size(), 2);
    ASSERT_EQ(t[0], TokenType::KW_INT);
    ASSERT_EQ(t[1], TokenType::KW_RETURN);
});

// ── Maximal munch ─────────────────────────────────────────────
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

// ── Integers ──────────────────────────────────────────────────
TEST("integer: single digit", [](){
    auto t = types("0");
    ASSERT_EQ((int)t.size(), 1);
    ASSERT_EQ(t[0], TokenType::INTEGER_LITERAL);
});

TEST("integer: multi digit", [](){
    ASSERT_EQ(lexemes("12345")[0], std::string("12345"));
});

// ── Operators ─────────────────────────────────────────────────
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

// ── Whitespace ────────────────────────────────────────────────
TEST("whitespace: tabs newlines ignored", [](){
    auto t = types("int\n\t x\r\n  =\t5 ;");
    ASSERT_EQ((int)t.size(), 5);
    ASSERT_EQ(t[0], TokenType::KW_INT);
    ASSERT_EQ(t[1], TokenType::IDENTIFIER);
    ASSERT_EQ(t[2], TokenType::ASSIGN);
    ASSERT_EQ(t[3], TokenType::INTEGER_LITERAL);
    ASSERT_EQ(t[4], TokenType::SEMICOLON);
});

// ── Comments ──────────────────────────────────────────────────
TEST("comment: single-line // stripped", [](){
    auto t = types("int x; // this is a comment\nreturn x;");
    ASSERT_EQ((int)t.size(), 6);
    ASSERT_EQ(t[3], TokenType::KW_RETURN);
});

TEST("comment: block /* */ stripped", [](){
    auto t = types("int /* hello world */ x;");
    ASSERT_EQ((int)t.size(), 3);
    ASSERT_EQ(t[0], TokenType::KW_INT);
    ASSERT_EQ(t[1], TokenType::IDENTIFIER);
    ASSERT_EQ(t[2], TokenType::SEMICOLON);
});

// ── Source location ───────────────────────────────────────────
TEST("source_location: line and column correct", [](){
    Lexer lexer("int\nreturn");
    auto result = lexer.tokenize();
    ASSERT_EQ(result.output[0].line, 1);
    ASSERT_EQ(result.output[1].line, 2);
    ASSERT_EQ(result.output[1].column, 1);
});

TEST("source_location: span() matches line and column", [](){
    Lexer lexer("return");
    auto result = lexer.tokenize();
    SourceSpan sp = result.output[0].span();
    ASSERT_EQ(sp.startLine, 1);
    ASSERT_EQ(sp.startCol,  1);
    ASSERT_EQ(sp.length,    6); // "return" is 6 chars
});

// ── Full program ──────────────────────────────────────────────
TEST("full_program: int main(){ int x=5; return x+2; }", [](){
    std::string src =
        "int main() {\n"
        "    int x = 5;\n"
        "    return x + 2;\n"
        "}\n";
    Lexer lexer(src);
    auto result = lexer.tokenize();
    ASSERT_TRUE(!result.hasErrors());
    ASSERT_EQ((int)result.output.size(), 17);  // 16 tokens + EOF
    ASSERT_EQ(result.output[0].type,  TokenType::KW_INT);
    ASSERT_EQ(result.output[1].type,  TokenType::IDENTIFIER);
    ASSERT_EQ(result.output[1].lexeme, std::string("main"));
    ASSERT_EQ(result.output[16].type, TokenType::END_OF_FILE);
});

// ── Error recovery ────────────────────────────────────────────
TEST("error: unknown character @ reported as Diagnostic", [](){
    Lexer lexer("int @x;");
    auto result = lexer.tokenize();
    ASSERT_TRUE(result.hasErrors());
    ASSERT_EQ((int)result.diagnostics.size(), 1);
    // Diagnostic has the right kind
    ASSERT_EQ(result.diagnostics[0].kind,
              DiagnosticKind::LEX_UnexpectedCharacter);
    // Lexer continues — still produces int, @(unknown), x, ;, EOF
    bool foundIdent = false;
    for (auto& t : result.output)
        if (t.type == TokenType::IDENTIFIER) foundIdent = true;
    ASSERT_TRUE(foundIdent);
});

TEST("error: unterminated block comment diagnosed", [](){
    Lexer lexer("int x; /* never closed");
    auto result = lexer.tokenize();
    ASSERT_TRUE(result.hasErrors());
    ASSERT_EQ(result.diagnostics[0].kind,
              DiagnosticKind::LEX_UnterminatedBlockComment);
});

TEST("error: integer literal past INT_MAX diagnosed as out of range", [](){
    Lexer lexer("int x = 99999999999;");
    auto result = lexer.tokenize();
    ASSERT_TRUE(result.hasErrors());
    ASSERT_EQ(result.diagnostics[0].kind,
              DiagnosticKind::LEX_IntegerOutOfRange);
    // The token itself is still produced so the parser can continue
    bool foundLiteral = false;
    for (auto& t : result.output)
        if (t.type == TokenType::INTEGER_LITERAL) foundLiteral = true;
    ASSERT_TRUE(foundLiteral);
});

TEST("integer range: 2147483647 (exactly INT_MAX) is accepted", [](){
    Lexer lexer("int x = 2147483647;");
    auto result = lexer.tokenize();
    ASSERT_TRUE(!result.hasErrors());
});

TEST("integer range: 2147483648 (INT_MAX + 1) is rejected", [](){
    Lexer lexer("int x = 2147483648;");
    auto result = lexer.tokenize();
    ASSERT_TRUE(result.hasErrors());
    ASSERT_EQ(result.diagnostics[0].kind, DiagnosticKind::LEX_IntegerOutOfRange);
});

TEST("integer range: leading zeros don't fool the length check", [](){
    // 19 characters long, but the VALUE is 5 — must be accepted
    Lexer lexer("int x = 0000000000000000005;");
    auto result = lexer.tokenize();
    ASSERT_TRUE(!result.hasErrors());
});

// ── Diagnostic metadata quality ──────────────────────────────
TEST("diagnostic: message is non-empty", [](){
    Lexer lexer("@");
    auto result = lexer.tokenize();
    ASSERT_TRUE(!result.diagnostics.empty());
    ASSERT_TRUE(!result.diagnostics[0].message.empty());
});

TEST("diagnostic: explanation is non-empty", [](){
    Lexer lexer("@");
    auto result = lexer.tokenize();
    ASSERT_TRUE(!result.diagnostics[0].explanation.empty());
});

TEST("diagnostic: fixes are present", [](){
    Lexer lexer("@");
    auto result = lexer.tokenize();
    ASSERT_TRUE(!result.diagnostics[0].fixes.empty());
});

TEST("diagnostic: trace is present", [](){
    Lexer lexer("@");
    auto result = lexer.tokenize();
    ASSERT_TRUE(!result.diagnostics[0].trace.empty());
});

TEST("diagnostic: source span is correct", [](){
    Lexer lexer("int @x;");
    auto result = lexer.tokenize();
    // '@' is at col 5 of line 1
    ASSERT_EQ(result.diagnostics[0].span.startLine, 1);
    ASSERT_EQ(result.diagnostics[0].span.startCol,  5);
});

// ── Edge cases ────────────────────────────────────────────────
TEST("edge: empty source yields only EOF", [](){
    Lexer lexer("");
    auto result = lexer.tokenize();
    ASSERT_EQ((int)result.output.size(), 1);
    ASSERT_EQ(result.output[0].type, TokenType::END_OF_FILE);
});

TEST("edge: only whitespace yields only EOF", [](){
    Lexer lexer("   \n\t  ");
    auto result = lexer.tokenize();
    ASSERT_EQ((int)result.output.size(), 1);
    ASSERT_EQ(result.output[0].type, TokenType::END_OF_FILE);
});

// ── Recorded traces (trace-recording work) ────────────────────
TEST("trace: diagnostic carries the lexer's recorded scan path", [](){
    Lexer lexer("int @x;");
    auto result = lexer.tokenize();
    ASSERT_TRUE(!result.diagnostics.empty());

    // One declaration per statement: a comma at brace depth inside a
    // TEST body splits the macro's arguments (recurring gotcha).
    bool sawTokenize  = false;
    bool sawNextToken = false;
    bool sawLexSymbol = false;
    for (const auto& s : result.diagnostics[0].trace) {
        if (s.component == "Lexer::tokenize()")  sawTokenize  = true;
        if (s.component == "Lexer::nextToken()") sawNextToken = true;
        if (s.component == "Lexer::lexSymbol()") sawLexSymbol = true;
    }
    ASSERT_TRUE(sawTokenize);
    ASSERT_TRUE(sawNextToken);
    ASSERT_TRUE(sawLexSymbol);
});

TEST("trace: recorded lexer frame carries the real scan position", [](){
    Lexer lexer("int @x;");
    auto result = lexer.tokenize();
    // '@' is at line 1, col 5 — a position only a trace recorded at
    // runtime can contain (the curated chains have no positions).
    bool sawPosition = false;
    for (const auto& s : result.diagnostics[0].trace)
        if (s.detail.find("line 1, col 5") != std::string::npos) sawPosition = true;
    ASSERT_TRUE(sawPosition);
});

// ── main ──────────────────────────────────────────────────────
int main() {
    std::cout << "=== Lexer Unit Tests (v2 — with Diagnostic system) ===\n\n";
    return RUN_ALL_TESTS();
}
