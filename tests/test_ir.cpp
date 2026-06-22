#include "test_runner.hpp"
#include "../src/lexer/Lexer.hpp"
#include "../src/parser/Parser.hpp"
#include "../src/semantic/SemanticAnalyzer.hpp"
#include "../src/ir/IRGenerator.hpp"
#include <memory>
#include <string>

// ============================================================
//  Helper: lex -> parse -> semantic-annotate -> generate IR.
//  Returns both the IR and the semantic StageOutput so tests
//  can optionally assert the input was semantically valid.
// ============================================================
struct IRResult {
    IRProgram         ir;
    StageOutput<bool> sem;
};

static IRResult genIR(const std::string& src) {
    Lexer lexer(src);
    auto lexOut = lexer.tokenize();
    Parser parser(lexOut.output);
    auto parseOut = parser.parse();

    SemanticAnalyzer analyzer;
    auto semOut = analyzer.analyze(*parseOut.output);

    IRGenerator irgen;
    IRProgram ir = irgen.generate(*parseOut.output);

    return { std::move(ir), std::move(semOut) };
}

// ── Spec: target program ───────────────────────────────────────
TEST("spec: int main(){int x=5; return x+2;} produces correct IR", [](){
    auto r = genIR(
        "int main() {\n"
        "    int x = 5;\n"
        "    return x + 2;\n"
        "}\n"
    );
    ASSERT_TRUE(!r.sem.hasErrors());
    ASSERT_EQ((int)r.ir.functions.size(), 1);

    auto& instrs = r.ir.functions[0].instructions;
    ASSERT_EQ((int)instrs.size(), 3);
    ASSERT_EQ(instrs[0].toString(), std::string("x = 5"));
    ASSERT_EQ(instrs[1].toString(), std::string("t0 = x + 2"));
    ASSERT_EQ(instrs[2].toString(), std::string("return t0"));
});

// ── Constants and variables need no temp ────────────────────────
TEST("constant assignment needs no temp: x = 5 not t0=5;x=t0", [](){
    auto r = genIR("int f() { int x = 5; return 0; }");
    auto& instrs = r.ir.functions[0].instructions;
    ASSERT_EQ((int)instrs.size(), 2);
    ASSERT_EQ(instrs[0].toString(), std::string("x = 5"));
    ASSERT_EQ(instrs[1].toString(), std::string("return 0"));
});

// ── Binary expression creates exactly one temp ──────────────────
TEST("binary expr creates exactly one temp", [](){
    auto r = genIR("int f() { return 1 + 2; }");
    auto& instrs = r.ir.functions[0].instructions;
    ASSERT_EQ((int)instrs.size(), 2);
    ASSERT_EQ(instrs[0].toString(), std::string("t0 = 1 + 2"));
    ASSERT_EQ(instrs[1].toString(), std::string("return t0"));
});

// ── Operator precedence produces correctly ordered temps ────────
TEST("precedence: a + b * c lowers inner op first", [](){
    auto r = genIR("int f(int a, int b, int c) { return a + b * c; }");
    auto& instrs = r.ir.functions[0].instructions;
    ASSERT_EQ((int)instrs.size(), 3);
    ASSERT_EQ(instrs[0].toString(), std::string("t0 = b * c"));
    ASSERT_EQ(instrs[1].toString(), std::string("t1 = a + t0"));
    ASSERT_EQ(instrs[2].toString(), std::string("return t1"));
});

// ── Each function resets its own temp counter ───────────────────
TEST("each function resets temp counter independently", [](){
    auto r = genIR(
        "int f() { return 1 + 2; }\n"
        "int g() { return 3 + 4; }\n"
    );
    ASSERT_EQ((int)r.ir.functions.size(), 2);
    ASSERT_EQ(r.ir.functions[0].instructions[0].toString(), std::string("t0 = 1 + 2"));
    ASSERT_EQ(r.ir.functions[1].instructions[0].toString(), std::string("t0 = 3 + 4"));
});

// ── All arithmetic operators map correctly ───────────────────────
TEST("operator mapping: - * /", [](){
    ASSERT_EQ(genIR("int f() { return 1 - 2; }").ir.functions[0].instructions[0].toString(),
              std::string("t0 = 1 - 2"));
    ASSERT_EQ(genIR("int f() { return 1 * 2; }").ir.functions[0].instructions[0].toString(),
              std::string("t0 = 1 * 2"));
    ASSERT_EQ(genIR("int f() { return 1 / 2; }").ir.functions[0].instructions[0].toString(),
              std::string("t0 = 1 / 2"));
});

// ── Comparison operators map correctly ───────────────────────────
TEST("operator mapping: == != < >", [](){
    ASSERT_EQ(genIR("int f() { return 1 == 2; }").ir.functions[0].instructions[0].toString(),
              std::string("t0 = 1 == 2"));
    ASSERT_EQ(genIR("int f() { return 1 != 2; }").ir.functions[0].instructions[0].toString(),
              std::string("t0 = 1 != 2"));
    ASSERT_EQ(genIR("int f() { return 1 < 2; }").ir.functions[0].instructions[0].toString(),
              std::string("t0 = 1 < 2"));
    ASSERT_EQ(genIR("int f() { return 1 > 2; }").ir.functions[0].instructions[0].toString(),
              std::string("t0 = 1 > 2"));
});

// ── Uninitialised declaration emits nothing ──────────────────────
TEST("uninitialised var decl emits no instruction", [](){
    auto r = genIR("int f() { int x; return 0; }");
    auto& instrs = r.ir.functions[0].instructions;
    ASSERT_EQ((int)instrs.size(), 1);
    ASSERT_EQ(instrs[0].toString(), std::string("return 0"));
});

// ── Bare return (no value) ────────────────────────────────────────
TEST("bare return produces 'return' with no operand", [](){
    auto r = genIR("int f() { return; }");
    auto& instrs = r.ir.functions[0].instructions;
    ASSERT_EQ((int)instrs.size(), 1);
    ASSERT_EQ(instrs[0].toString(), std::string("return"));
});

// ── Parameters are plain Var operands ────────────────────────────
TEST("parameter used directly as Var operand, no temp for the ref itself", [](){
    auto r = genIR("int double_it(int n) { return n + n; }");
    ASSERT_TRUE(!r.sem.hasErrors());
    auto& instrs = r.ir.functions[0].instructions;
    ASSERT_EQ((int)instrs.size(), 2);
    ASSERT_EQ(instrs[0].toString(), std::string("t0 = n + n"));
    ASSERT_EQ(instrs[1].toString(), std::string("return t0"));
});

// ── Multiple statements in sequence ───────────────────────────────
TEST("multiple var decls then a binary return", [](){
    auto r = genIR("int f() { int a = 1; int b = 2; return a + b; }");
    auto& instrs = r.ir.functions[0].instructions;
    ASSERT_EQ((int)instrs.size(), 4);
    ASSERT_EQ(instrs[0].toString(), std::string("a = 1"));
    ASSERT_EQ(instrs[1].toString(), std::string("b = 2"));
    ASSERT_EQ(instrs[2].toString(), std::string("t0 = a + b"));
    ASSERT_EQ(instrs[3].toString(), std::string("return t0"));
});

// ── IRValue::toString ─────────────────────────────────────────────
TEST("IRValue::toString for temp, var, const", [](){
    ASSERT_EQ(IRValue::makeTemp(3).toString(),   std::string("t3"));
    ASSERT_EQ(IRValue::makeVar("foo").toString(), std::string("foo"));
    ASSERT_EQ(IRValue::makeConst(42).toString(),  std::string("42"));
});

// ── IRFunction::toString full format ──────────────────────────────
TEST("IRFunction::toString produces 'function name:' header + body", [](){
    auto r = genIR("int main() { int x = 5; return x + 2; }");
    std::string text = r.ir.functions[0].toString();
    ASSERT_TRUE(text.find("function main:") != std::string::npos);
    ASSERT_TRUE(text.find("x = 5")          != std::string::npos);
    ASSERT_TRUE(text.find("t0 = x + 2")     != std::string::npos);
    ASSERT_TRUE(text.find("return t0")      != std::string::npos);
});

// ── IRProgram::toString concatenates all functions ────────────────
TEST("IRProgram::toString includes every function", [](){
    auto r = genIR(
        "int foo() { return 1; }\n"
        "int bar() { return 2; }\n"
    );
    std::string text = r.ir.toString();
    ASSERT_TRUE(text.find("function foo:") != std::string::npos);
    ASSERT_TRUE(text.find("function bar:") != std::string::npos);
});

// ── No constant folding at this stage (by design) ─────────────────
TEST("IR generation does NOT fold constants — that's the optimizer's job", [](){
    auto r = genIR("int f() { return 2 + 3; }");
    auto& instrs = r.ir.functions[0].instructions;
    // Should still be "t0 = 2 + 3", NOT folded to "return 5"
    ASSERT_EQ(instrs[0].toString(), std::string("t0 = 2 + 3"));
});

int main() {
    std::cout << "=== IR Generation Unit Tests ===\n\n";
    return RUN_ALL_TESTS();
}
