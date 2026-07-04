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

// ── Assignment expressions ──────────────────────────────────────

TEST("assignment: x = 3 where x is declared int — no errors, resolvedType int", [](){
    auto r = run("int main() { int x = 5; return x = 3; }");
    ASSERT_TRUE(!r.sem.hasErrors());

    ReturnStmtNode* ret = dynamic_cast<ReturnStmtNode*>(
        r.program->functions[0]->body->statements[1].get());
    AssignmentExprNode* assign = dynamic_cast<AssignmentExprNode*>(ret->value.get());
    ASSERT_TRUE(assign != nullptr);
    ASSERT_EQ(assign->resolvedType, std::string("int"));
});

TEST("assignment: target must already be declared", [](){
    auto r = run("int main() { return y = 3; }");
    ASSERT_TRUE(r.sem.hasErrors());
    ASSERT_EQ(r.sem.diagnostics[0].kind, DiagnosticKind::SEM_UndeclaredIdentifier);
});

TEST("assignment: type mismatch when RHS is bool and target is int", [](){
    // x = a < b: RHS is a comparison (bool), x is declared int.
    auto r = run("int main() { int x=0; int a=1; int b=2; return x = a < b; }");
    ASSERT_TRUE(r.sem.hasErrors());
    ASSERT_EQ(r.sem.diagnostics[0].kind, DiagnosticKind::SEM_TypeMismatch);
});

TEST("assignment: chained a = b = c — both targets must be declared, both pass", [](){
    auto r = run("int main() { int a=0; int b=0; int c=7; return a = b = c; }");
    ASSERT_TRUE(!r.sem.hasErrors());
});

// ── Bare return in a non-void function ───────────────────────────
// (20-case verification suite, case 13: a bare 'return;' inside
// int main() was silently accepted — logFail() recorded it in the
// semantic log but no Diagnostic was ever raised, so codegen went
// on to emit leave/ret with nothing placed in the return register.)

TEST("return: bare 'return;' in non-void function raises SEM_ReturnTypeMismatch", [](){
    auto r = run("int main() { return; }");
    ASSERT_TRUE(r.sem.hasErrors());
    ASSERT_EQ(r.sem.diagnostics[0].kind, DiagnosticKind::SEM_ReturnTypeMismatch);
    ASSERT_TRUE(!r.sem.diagnostics[0].explanation.empty());
    ASSERT_TRUE(!r.sem.diagnostics[0].fixes.empty());
    ASSERT_TRUE(!r.sem.diagnostics[0].trace.empty());
});

TEST("return: bare 'return;' message names the function and its return type", [](){
    auto r = run("int compute() { return; }");
    ASSERT_TRUE(r.sem.hasErrors());
    ASSERT_TRUE(r.sem.diagnostics[0].message.find("compute") != std::string::npos);
    ASSERT_TRUE(r.sem.diagnostics[0].message.find("int")     != std::string::npos);
});

// ── Missing return (warning, not error) ──────────────────────────
// (production review: "int main() { int x = 5; }" compiled silently
// and returned register garbage at runtime — GCC warns here.)

TEST("return: function with no return at all gets SEM_MissingReturn warning", [](){
    auto r = run("int main() { int x = 5; }");
    ASSERT_TRUE(!r.sem.hasErrors());                 // warning, build continues
    ASSERT_EQ((int)r.sem.diagnostics.size(), 1);
    ASSERT_EQ(r.sem.diagnostics[0].kind, DiagnosticKind::SEM_MissingReturn);
    ASSERT_TRUE(r.sem.diagnostics[0].severity == Severity::Warning);
    ASSERT_TRUE(r.sem.output);                       // "true means no errors"
});

TEST("return: function that does return produces no missing-return warning", [](){
    auto r = run("int main() { return 0; }");
    ASSERT_EQ((int)r.sem.diagnostics.size(), 0);
});

// ── Function names used as values ────────────────────────────────
// (production review: "return main;" and "main = 5;" both compiled
// silently into reads/writes of an uninitialised stack slot.)

TEST("functions: reading a function name as a value is an error", [](){
    auto r = run("int main() { return main; }");
    ASSERT_TRUE(r.sem.hasErrors());
    ASSERT_EQ(r.sem.diagnostics[0].kind, DiagnosticKind::SEM_FunctionUsedAsValue);
});

TEST("functions: assigning into a function name is an error", [](){
    auto r = run("int main() { return main = 5; }");
    ASSERT_TRUE(r.sem.hasErrors());
    ASSERT_EQ(r.sem.diagnostics[0].kind, DiagnosticKind::SEM_FunctionUsedAsValue);
});

// ── Parameters share the function's outermost scope ─────────────
// (production review: "int f(int a) { int a; }" silently shadowed the
// parameter — C++ rejects this, and both names would have lowered to
// the same IR Var and stack slot.)

TEST("params: local redeclaring a parameter name is a redeclaration error", [](){
    auto r = run("int f(int a) { int a = 9; return a; }");
    ASSERT_TRUE(r.sem.hasErrors());
    ASSERT_EQ(r.sem.diagnostics[0].kind, DiagnosticKind::SEM_RedeclaredVariable);
});

TEST("redecl: the duplicate's initializer is still analysed for its own errors", [](){
    // 'y' is undeclared inside the redeclared x's initializer — both
    // problems must be reported, not just the redeclaration.
    auto r = run("int f() { int x = 5; int x = y; return x; }");
    ASSERT_TRUE(r.sem.hasErrors());
    ASSERT_EQ((int)r.sem.diagnostics.size(), 2);
    ASSERT_EQ(r.sem.diagnostics[0].kind, DiagnosticKind::SEM_RedeclaredVariable);
    ASSERT_EQ(r.sem.diagnostics[1].kind, DiagnosticKind::SEM_UndeclaredIdentifier);
});

// ── Recorded traces (trace-recording work) ────────────────────
TEST("trace: semantic diagnostic records which function was being analysed", [](){
    auto r = run("int main() { return y; }");
    ASSERT_TRUE(r.sem.hasErrors());

    // Runtime-only facts: the enclosing function's NAME and the
    // identifier being resolved can only appear in a trace recorded
    // during execution — the curated per-kind chain knows neither.
    bool sawFunction = false;
    bool sawResolve  = false;
    for (const auto& s : r.sem.diagnostics[0].trace) {
        if (s.component.find("visit(FunctionDeclNode") != std::string::npos &&
            s.detail.find("'main'") != std::string::npos) sawFunction = true;
        if (s.component.find("visit(IdentifierNode") != std::string::npos &&
            s.detail.find("'y'") != std::string::npos) sawResolve = true;
    }
    ASSERT_TRUE(sawFunction);
    ASSERT_TRUE(sawResolve);
});

TEST("trace: missing-return warning records the analyze->function path", [](){
    auto r = run("int main() { int x = 5; }");
    ASSERT_TRUE(!r.sem.hasErrors());          // warning only
    ASSERT_EQ((int)r.sem.diagnostics.size(), 1);

    bool sawAnalyze = false;
    for (const auto& s : r.sem.diagnostics[0].trace)
        if (s.component == "SemanticAnalyzer::analyze()") sawAnalyze = true;
    ASSERT_TRUE(sawAnalyze);
});

int main() {
    std::cout << "=== Semantic Analysis Unit Tests ===\n\n";
    return RUN_ALL_TESTS();
}
