#include "test_runner.hpp"
#include "../src/core/Json.hpp"
#include "../src/lexer/Token.hpp"
#include "../src/lexer/Lexer.hpp"
#include "../src/parser/Parser.hpp"
#include "../src/semantic/SemanticAnalyzer.hpp"
#include "../src/ast/ASTJsonPrinter.hpp"
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <string>

// ============================================================
//  Json.hpp helpers
// ============================================================

TEST("json: jsonEscape handles quotes, backslashes, and control chars", [](){
    ASSERT_EQ(jsonEscape("a\"b"),  std::string("a\\\"b"));
    ASSERT_EQ(jsonEscape("a\\b"),  std::string("a\\\\b"));
    ASSERT_EQ(jsonEscape("a\nb"),  std::string("a\\nb"));
    ASSERT_EQ(jsonEscape("plain"), std::string("plain"));
});

TEST("json: jsonArray joins fragments without quoting them", [](){
    std::vector<std::string> items;
    items.push_back("1");
    items.push_back("{\"a\":2}");
    ASSERT_EQ(jsonArray(items), std::string("[1,{\"a\":2}]"));
    ASSERT_EQ(jsonArray(std::vector<std::string>()), std::string("[]"));
});

TEST("json: jsonStringArray quotes and escapes plain strings", [](){
    std::vector<std::string> items;
    items.push_back("a");
    items.push_back("b\"c");
    ASSERT_EQ(jsonStringArray(items), std::string("[\"a\",\"b\\\"c\"]"));
});

// ============================================================
//  Token::toJson()
// ============================================================

TEST("token: toJson includes type, lexeme, line, column", [](){
    Token tok;
    tok.type   = TokenType::IDENTIFIER;
    tok.lexeme = "x";
    tok.line   = 3;
    tok.column = 12;

    std::string j = tok.toJson();
    ASSERT_TRUE(j.find("\"type\":\"IDENTIFIER\"") != std::string::npos);
    ASSERT_TRUE(j.find("\"lexeme\":\"x\"") != std::string::npos);
    ASSERT_TRUE(j.find("\"line\":3") != std::string::npos);
    ASSERT_TRUE(j.find("\"column\":12") != std::string::npos);
});

// ============================================================
//  ASTJsonPrinter — shape checks (substring-based, not a full
//  JSON parse: this project has no JSON library on purpose, see
//  core/Json.hpp's class comment, so tests match the existing
//  pragmatic string-assertion style used in test_codegen.cpp).
// ============================================================

static std::string astJsonFor(const std::string& src) {
    Lexer lexer(src);
    auto lexOut = lexer.tokenize();
    Parser parser(lexOut.output);
    auto parseOut = parser.parse();
    SemanticAnalyzer analyzer;
    analyzer.analyze(*parseOut.output);

    ASTJsonPrinter printer;
    return printer.toJson(*parseOut.output);
}

TEST("ast json: target program produces correctly nested, annotated tree", [](){
    std::string j = astJsonFor(
        "int main() {\n"
        "    int x = 5;\n"
        "    return x + 2;\n"
        "}\n"
    );
    ASSERT_TRUE(j.find("\"node\":\"Program\"") != std::string::npos);
    ASSERT_TRUE(j.find("\"node\":\"FunctionDecl\"") != std::string::npos);
    ASSERT_TRUE(j.find("\"name\":\"main\"") != std::string::npos);
    ASSERT_TRUE(j.find("\"node\":\"VarDecl\"") != std::string::npos);
    ASSERT_TRUE(j.find("\"node\":\"BinaryExpr\"") != std::string::npos);
    ASSERT_TRUE(j.find("\"op\":\"+\"") != std::string::npos);
    // resolvedType annotations from SemanticAnalyzer made it into the JSON
    ASSERT_TRUE(j.find("\"resolvedType\":\"int\"") != std::string::npos);
});

TEST("ast json: uninitialised var decl emits initializer:null, not a missing key", [](){
    std::string j = astJsonFor("int f() { int x; return 0; }");
    ASSERT_TRUE(j.find("\"initializer\":null") != std::string::npos);
});

TEST("ast json: function params appear as name/type pairs", [](){
    std::string j = astJsonFor("int f(int a, int b) { return a; }");
    ASSERT_TRUE(j.find("\"params\":[{\"name\":\"a\",\"type\":\"int\"},{\"name\":\"b\",\"type\":\"int\"}]")
                != std::string::npos);
});

// ============================================================
//  End-to-end: the real --json CLI mode, via the real compiled
//  binary, the same empirical standard test_codegen.cpp and
//  test_driver.cpp use for their end-to-end checks.
// ============================================================

static std::string runJsonCli(const std::string& sourcePath, const std::string& jsonOutPath) {
    // compiler.exe as a bare backslash path: the same cmd.exe "build/x"
    // vs "build\x" parsing quirk discovered in test_codegen.cpp's
    // assembleLinkRun (this machine's MASM32 build.bat shadows the
    // forward-slash form). sourcePath is just an ARGUMENT here, not
    // the resolved command token, so it's unaffected either way.
    std::string cmd = "build\\compiler.exe " + sourcePath + " --json > " + jsonOutPath + " 2>&1";
    std::system(cmd.c_str());

    std::ifstream f(jsonOutPath);
    std::stringstream ss;
    ss << f.rdbuf();
    return ss.str();
}

TEST("end-to-end: --json CLI produces valid-shaped output for a clean program", [](){
    std::ofstream src("build/viz_clean.cpp");
    src << "int main() {\n    int x = 5;\n    return x + 2;\n}\n";
    src.close();

    std::string j = runJsonCli("build/viz_clean.cpp", "build/viz_clean.json");

    ASSERT_TRUE(j.find("\"tokens\":") != std::string::npos);
    ASSERT_TRUE(j.find("\"node\":\"Program\"") != std::string::npos);
    ASSERT_TRUE(j.find("\"irAfter\":") != std::string::npos);
    ASSERT_TRUE(j.find("return t0") != std::string::npos);
    ASSERT_TRUE(j.find("_main") != std::string::npos);
    // No diagnostic objects were emitted at all — each one would
    // contain a "kind" field, so its absence is a robust, whitespace-
    // independent way to assert the diagnostics array is empty.
    ASSERT_TRUE(j.find("\"kind\"") == std::string::npos);
});

TEST("end-to-end: --json CLI reports diagnostics and stops before IR on error", [](){
    std::ofstream src("build/viz_bad.cpp");
    src << "int main() {\n    return y + 2;\n}\n";
    src.close();

    std::string j = runJsonCli("build/viz_bad.cpp", "build/viz_bad.json");

    ASSERT_TRUE(j.find("UndeclaredIdentifier") != std::string::npos);
    ASSERT_TRUE(j.find("\"irBefore\": \"\"") != std::string::npos);
    ASSERT_TRUE(j.find("\"assembly\": \"\"") != std::string::npos);
});

TEST("end-to-end: --json exports structured IR provenance (line + note)", [](){
    std::ofstream src("build/viz_prov.cpp");
    src << "int main() {\n    return (2 + 3) * 4;\n}\n";
    src.close();

    std::string j = runJsonCli("build/viz_prov.cpp", "build/viz_prov.json");

    ASSERT_TRUE(j.find("\"irDetailBefore\":") != std::string::npos);
    ASSERT_TRUE(j.find("\"irDetailAfter\":")  != std::string::npos);
    // Every instruction of this program comes from source line 2.
    ASSERT_TRUE(j.find("\"line\":2")          != std::string::npos);
    // The optimizer's transformation notes flow through to the JSON.
    // The SURVIVING instruction's history is the propagation note —
    // the ConstantFolding-noted instructions get DCE'd once their
    // temps go unused, taking their notes with them (same behaviour
    // test_optimizer's full-pipeline provenance test locks in).
    ASSERT_TRUE(j.find("CopyPropagation")     != std::string::npos);
});

TEST("end-to-end: irDetail fields are empty arrays when IR never ran", [](){
    std::ofstream src("build/viz_bad2.cpp");
    src << "int main() {\n    return y;\n}\n";
    src.close();

    std::string j = runJsonCli("build/viz_bad2.cpp", "build/viz_bad2.json");

    ASSERT_TRUE(j.find("\"irDetailBefore\": []") != std::string::npos);
    ASSERT_TRUE(j.find("\"irDetailAfter\": []")  != std::string::npos);
});

// ============================================================
//  --check mode: full pipeline, no artifacts, silent on success.
//  This is the overhead experiments' measurement target, so its
//  contract (exit code + silence) is load-bearing — a stray print
//  on the success path would contaminate every timing run.
// ============================================================

TEST("end-to-end: --check exits 0 and prints nothing for a valid program", [](){
    std::ofstream src("build/viz_chk_ok.cpp");
    src << "int main() {\n    int x = 5;\n    return x + 2;\n}\n";
    src.close();

    int exit = std::system("build\\compiler.exe build/viz_chk_ok.cpp --check "
                           "> build/viz_chk_ok.txt 2>&1");
    ASSERT_EQ(exit, 0);
    std::ifstream f("build/viz_chk_ok.txt");
    std::stringstream ss;
    ss << f.rdbuf();
    ASSERT_TRUE(ss.str().empty());
});

TEST("end-to-end: --check exits nonzero with full diagnostics for an invalid program", [](){
    std::ofstream src("build/viz_chk_bad.cpp");
    src << "int main() {\n    return y;\n}\n";
    src.close();

    int exit = std::system("build\\compiler.exe build/viz_chk_bad.cpp --check "
                           "> build/viz_chk_bad.txt 2>&1");
    ASSERT_TRUE(exit != 0);
    std::ifstream f("build/viz_chk_bad.txt");
    std::stringstream ss;
    ss << f.rdbuf();
    ASSERT_TRUE(ss.str().find("ERROR TYPE:") != std::string::npos);
    ASSERT_TRUE(ss.str().find("Undeclared Identifier") != std::string::npos);
});

int main() {
    return RUN_ALL_TESTS();
}
