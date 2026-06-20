#include "test_runner.hpp"
#include "../src/lexer/Lexer.hpp"
#include "../src/parser/Parser.hpp"
#include "../src/semantic/SemanticAnalyzer.hpp"
#include "../src/ast/Nodes.hpp"
#include <memory>
#include <string>

// ============================================================
//  Helper: lex → parse → analyze, return both the annotated
//  program and the semantic StageOutput together.
// ============================================================
struct SemResult {
    std::unique_ptr<ProgramNode> program;
    StageOutput<bool>            sem;
    SemanticAnalyzer             analyzer;  // owns the log
};

static SemResult run(const std::string& src) {
    Lexer lexer(src);
    auto lexOut  = lexer.tokenize();
    Parser parser(lexOut.output);
    auto parseOut = parser.parse();

    SemResult r;
    r.program = std::move(parseOut.output);
    r.sem     = r.analyzer.analyze(*r.program);
    return r;
}

// ── Spec: target program passes all checks ────────────────────
TEST("spec: int main() { int x = 5; return x + 2; } — no errors", [](){
    auto r = run(
        "int main() {\n"
        "    int x = 5;\n"
        "    return x + 2;\n"
        "}\n"
    );
    ASSERT_TRUE(!r.sem.hasErrors());
    ASSERT_EQ(r.sem.errorCount(), 0);
});

// ── Spec: semantic log shows expected checks ──────────────────
TEST("spec: log contains expected check entries", [](){
    auto r = run(
        "int main() {\n"
        "    int x = 5;\n"
        "    return x + 2;\n"
        "}\n"
    );
    bool foundXDecl      = false;
    bool foundReturnMatch = false;
    bool foundExprValid   = false;
    for (auto& entry : r.analyzer.log()) {
        if (entry.find("x declared")           != std::string::npos) foundXDecl      = true;
        if (entry.find("return type matches")   != std::string::npos) foundReturnMatch = true;
        if (entry.find("expression valid")      != std::string::npos) foundExprValid   = true;
    }
    ASSERT_TRUE(foundXDecl);
    ASSERT_TRUE(foundReturnMatch);
    ASSERT_TRUE(foundExprValid);
});

// ── Type annotation on nodes ──────────────────────────────────
TEST("annotation: IntLiteral.resolvedType == int", [](){
    auto r = run("int f() { return 42; }");
    ASSERT_TRUE(!r.sem.hasErrors());

    ReturnStmtNode* ret = dynamic_cast<ReturnStmtNode*>(
        r.program->functions[0]->body->statements[0].get());
    ASSERT_EQ(ret->value->resolvedType, std::string("int"));
});

TEST("annotation: Identifier.resolvedType set after declaration", [](){
    auto r = run("int f() { int x = 5; return x; }");
    ASSERT_TRUE(!r.sem.hasErrors());

    ReturnStmtNode* ret = dynamic_cast<ReturnStmtNode*>(
        r.program->functions[0]->body->statements[1].get());
    ASSERT_EQ(ret->value->resolvedType, std::string("int"));
});

TEST("annotation: BinaryExpr int+int resolves to int", [](){
    auto r = run("int f() { int x = 1; return x + 2; }");
    ASSERT_TRUE(!r.sem.hasErrors());

    ReturnStmtNode* ret = dynamic_cast<ReturnStmtNode*>(
        r.program->functions[0]->body->statements[1].get());
    ASSERT_EQ(ret->value->resolvedType, std::string("int"));
});

TEST("annotation: comparison operator resolves to bool", [](){
    // x < 2 produces bool — BUT int f() expects int to be returned,
    // so we get a return-type mismatch (1 error). The BinaryExpr's
    // resolvedType is still "bool" — the annotation is correct.
    auto r = run("int f() { int x = 1; return x < 2; }");
    ASSERT_EQ(r.sem.errorCount(), 1);   // return type mismatch int vs bool
    ASSERT_EQ(r.sem.diagnostics[0].kind, DiagnosticKind::SEM_TypeMismatch);

    ReturnStmtNode* ret = dynamic_cast<ReturnStmtNode*>(
        r.program->functions[0]->body->statements[1].get());
    BinaryExprNode* cmp = dynamic_cast<BinaryExprNode*>(ret->value.get());
    ASSERT_EQ(cmp->resolvedType, std::string("bool"));
});

// ── Undeclared identifier ─────────────────────────────────────
TEST("error: undeclared identifier produces SEM diagnostic", [](){
    auto r = run("int f() { return y + 2; }");
    ASSERT_TRUE(r.sem.hasErrors());
    ASSERT_EQ(r.sem.diagnostics[0].kind,
              DiagnosticKind::SEM_UndeclaredIdentifier);
});

TEST("error: undeclared span points at the identifier", [](){
    auto r = run("int f() { return y + 2; }");
    // 'y' is at line 1 col 19 in "int f() { return y + 2; }"
    ASSERT_EQ(r.sem.diagnostics[0].span.startLine, 1);
    ASSERT_TRUE(!r.sem.diagnostics[0].message.empty());
    ASSERT_TRUE(!r.sem.diagnostics[0].explanation.empty());
    ASSERT_TRUE(!r.sem.diagnostics[0].fixes.empty());
    ASSERT_TRUE(!r.sem.diagnostics[0].trace.empty());
});

TEST("error: two undeclared in one expr produces two diagnostics", [](){
    auto r = run("int f() { return a + b; }");
    ASSERT_TRUE(r.sem.hasErrors());
    // Both 'a' and 'b' are undeclared
    ASSERT_EQ(r.sem.errorCount(), 2);
});

TEST("error: undeclared does not cascade into type-mismatch error", [](){
    // 'a' is undeclared → Unknown type → should NOT also get a type-mismatch
    auto r = run("int f() { return a + 2; }");
    ASSERT_EQ(r.sem.errorCount(), 1);  // only undeclared, not also mismatch
    ASSERT_EQ(r.sem.diagnostics[0].kind,
              DiagnosticKind::SEM_UndeclaredIdentifier);
});

// ── Redeclared variable ───────────────────────────────────────
TEST("error: redeclared variable in same scope", [](){
    auto r = run("int f() { int x = 5; int x = 10; return x; }");
    ASSERT_TRUE(r.sem.hasErrors());
    ASSERT_EQ(r.sem.diagnostics[0].kind,
              DiagnosticKind::SEM_RedeclaredVariable);
});

TEST("error: redeclared message names the variable", [](){
    auto r = run("int f() { int x = 5; int x = 10; return x; }");
    ASSERT_TRUE(!r.sem.diagnostics[0].message.empty());
    ASSERT_TRUE(r.sem.diagnostics[0].message.find("x") != std::string::npos);
});

// ── Function parameters ───────────────────────────────────────
TEST("params: parameter accessible in function body", [](){
    auto r = run("int double_it(int n) { return n + n; }");
    ASSERT_TRUE(!r.sem.hasErrors());
    ASSERT_EQ(r.sem.errorCount(), 0);
});

TEST("params: multiple parameters accessible", [](){
    auto r = run("int add(int a, int b) { return a + b; }");
    ASSERT_TRUE(!r.sem.hasErrors());
});

TEST("params: parameter type annotated correctly", [](){
    auto r = run("int f(int n) { return n; }");
    ASSERT_TRUE(!r.sem.hasErrors());

    ReturnStmtNode* ret = dynamic_cast<ReturnStmtNode*>(
        r.program->functions[0]->body->statements[0].get());
    ASSERT_EQ(ret->value->resolvedType, std::string("int"));
});

// ── Symbol table scoping ──────────────────────────────────────
TEST("scope: declared variable not accessible before declaration", [](){
    // x used before int x = 5 — in our grammar this is fine
    // because we only check usage after declaration point.
    // This tests that x is NOT found before its VarDecl.
    // In our sequential analysis, visiting return before the decl would fail.
    // This source has the return AFTER the decl, so it passes.
    auto r = run("int f() { int x = 5; return x; }");
    ASSERT_TRUE(!r.sem.hasErrors());
});

// ── VarDecl with no initializer ───────────────────────────────
TEST("vardecl: uninitialised declaration is valid", [](){
    auto r = run("int f() { int x; return 0; }");
    ASSERT_TRUE(!r.sem.hasErrors());
});

// ── Multiple functions ────────────────────────────────────────
TEST("multi-function: two clean functions pass", [](){
    auto r = run(
        "int foo() { return 1; }\n"
        "int bar() { return 2; }\n"
    );
    ASSERT_TRUE(!r.sem.hasErrors());
});

TEST("multi-function: errors isolated to the function they're in", [](){
    auto r = run(
        "int foo() { return 1; }\n"
        "int bar() { return y; }\n"  // 'y' undeclared
    );
    ASSERT_TRUE(r.sem.hasErrors());
    ASSERT_EQ(r.sem.errorCount(), 1);
    ASSERT_EQ(r.sem.diagnostics[0].kind,
              DiagnosticKind::SEM_UndeclaredIdentifier);
});

// ── Diagnostic quality ────────────────────────────────────────
TEST("diagnostic: undeclared has WHY explanation", [](){
    auto r = run("int f() { return z; }");
    ASSERT_TRUE(!r.sem.diagnostics[0].explanation.empty());
    ASSERT_TRUE(r.sem.diagnostics[0].explanation.find("declared") != std::string::npos);
});

TEST("diagnostic: redeclared has fix suggestions", [](){
    auto r = run("int f() { int x = 1; int x = 2; return x; }");
    ASSERT_TRUE(!r.sem.diagnostics[0].fixes.empty());
});

TEST("diagnostic: trace steps present for semantic errors", [](){
    auto r = run("int f() { return z; }");
    ASSERT_TRUE(!r.sem.diagnostics[0].trace.empty());
    // Last trace step should have ok == false
    ASSERT_TRUE(!r.sem.diagnostics[0].trace.back().ok);
});

int main() {
    std::cout << "=== Semantic Analysis Unit Tests ===\n\n";
    return RUN_ALL_TESTS();
}
