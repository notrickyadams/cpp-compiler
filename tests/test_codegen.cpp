#include "test_runner.hpp"
#include "../src/lexer/Lexer.hpp"
#include "../src/parser/Parser.hpp"
#include "../src/semantic/SemanticAnalyzer.hpp"
#include "../src/ir/IRGenerator.hpp"
#include "../src/optimizer/Optimizer.hpp"
#include "../src/codegen/StackFrameLayout.hpp"
#include "../src/codegen/AssemblyGenerator.hpp"
#include <fstream>
#include <cstdlib>
#include <string>

// ============================================================
//  Helper: full pipeline lex -> parse -> semantic -> IR ->
//  optimize -> generate assembly text.
// ============================================================
static AssemblyProgram genAsmFromSource(const std::string& src) {
    Lexer lexer(src);
    auto lexOut = lexer.tokenize();
    Parser parser(lexOut.output);
    auto parseOut = parser.parse();
    SemanticAnalyzer analyzer;
    analyzer.analyze(*parseOut.output);
    IRGenerator irgen;
    IRProgram ir = irgen.generate(*parseOut.output);
    Optimizer optimizer;
    optimizer.optimize(ir);
    AssemblyGenerator gen;
    return gen.generate(ir);
}

// ============================================================
//  Helper: writes generated assembly to build/<baseName>.s,
//  assembles and links it with the SAME real toolchain this
//  project targets, runs the result, and returns its process
//  exit code.
//
//  This is the same empirical standard the Stage 6 spikes were
//  validated against (see build/spike_test*.s) — proving the
//  generator's output is not just well-formed text, but actually
//  correct, runnable machine code. On Windows, system()'s return
//  value IS the child process's exit code directly (no
//  WEXITSTATUS bit-shifting, unlike POSIX).
//
//  exePath deliberately uses a BACKSLASH, not forward slash.
//  system() shells out to cmd.exe, and this machine has a MASM32
//  install on PATH with its own "build.bat" (C:\masm32\bin) —
//  cmd.exe parses a bare "build/x.exe" command token as "build"
//  (matching that unrelated batch file) plus a "/x.exe"-shaped
//  argument, NOT as one literal relative path. "build\x.exe" is
//  unambiguous. g++'s own arguments below are unaffected — they're
//  arguments to "g++", not the command token cmd.exe resolves.
// ============================================================
static int assembleLinkRun(const AssemblyProgram& asmProg, const std::string& baseName) {
    std::string sPath   = "build/" + baseName + ".s";
    std::string oPath   = "build/" + baseName + ".o";
    std::string exePath = "build\\" + baseName + ".exe";

    std::ofstream f(sPath);
    f << asmProg.toString();
    f.close();

    std::system(("g++ -c " + sPath + " -o " + oPath + " > NUL 2>&1").c_str());
    std::system(("g++ " + oPath + " -o " + exePath + " > NUL 2>&1").c_str());
    return std::system(exePath.c_str());
}

// ============================================================
//  StackFrameLayout — unit tests (no assembly involved)
// ============================================================

TEST("layout: parameters get cdecl offsets 8, 12, 16...", [](){
    IRFunction fn;
    fn.paramNames.push_back("a");
    fn.paramNames.push_back("b");
    fn.paramNames.push_back("c");
    StackFrameLayout layout(fn);
    ASSERT_EQ(layout.slotOf(IRValue::makeVar("a")), std::string("8(%ebp)"));
    ASSERT_EQ(layout.slotOf(IRValue::makeVar("b")), std::string("12(%ebp)"));
    ASSERT_EQ(layout.slotOf(IRValue::makeVar("c")), std::string("16(%ebp)"));
    ASSERT_EQ(layout.frameSize(), 0);  // params cost no LOCAL frame space
});

TEST("layout: locals get negative offsets in first-seen order", [](){
    IRFunction fn;
    fn.instructions.push_back(IRInstruction::makeCopy(IRValue::makeVar("x"), IRValue::makeConst(1)));
    fn.instructions.push_back(IRInstruction::makeCopy(IRValue::makeVar("y"), IRValue::makeConst(2)));
    StackFrameLayout layout(fn);
    ASSERT_EQ(layout.slotOf(IRValue::makeVar("x")), std::string("-4(%ebp)"));
    ASSERT_EQ(layout.slotOf(IRValue::makeVar("y")), std::string("-8(%ebp)"));
    ASSERT_EQ(layout.frameSize(), 8);
});

TEST("layout: temps get their own negative offsets, separate from named vars", [](){
    IRFunction fn;
    fn.instructions.push_back(IRInstruction::makeBinary(
        IROp::Add, IRValue::makeTemp(0), IRValue::makeConst(1), IRValue::makeConst(2)));
    StackFrameLayout layout(fn);
    ASSERT_EQ(layout.slotOf(IRValue::makeTemp(0)), std::string("-4(%ebp)"));
    ASSERT_EQ(layout.frameSize(), 4);
});

TEST("layout: a name used as both dest and later src gets exactly one slot", [](){
    IRFunction fn;
    fn.instructions.push_back(IRInstruction::makeCopy(IRValue::makeVar("x"), IRValue::makeConst(5)));
    fn.instructions.push_back(IRInstruction::makeBinary(
        IROp::Add, IRValue::makeTemp(0), IRValue::makeVar("x"), IRValue::makeConst(2)));
    StackFrameLayout layout(fn);
    // x: one slot. t0: one slot. Total 8 bytes, not more.
    ASSERT_EQ(layout.frameSize(), 8);
    ASSERT_EQ(layout.slotOf(IRValue::makeVar("x")), std::string("-4(%ebp)"));
    ASSERT_EQ(layout.slotOf(IRValue::makeTemp(0)), std::string("-8(%ebp)"));
});

TEST("layout: parameter referenced in body is NOT given a second local slot", [](){
    IRFunction fn;
    fn.paramNames = { "n" };
    fn.instructions.push_back(IRInstruction::makeBinary(
        IROp::Add, IRValue::makeTemp(0), IRValue::makeVar("n"), IRValue::makeVar("n")));
    StackFrameLayout layout(fn);
    ASSERT_EQ(layout.slotOf(IRValue::makeVar("n")), std::string("8(%ebp)"));
    ASSERT_EQ(layout.frameSize(), 4);  // only t0 costs local space
});

// ============================================================
//  AssemblyGenerator — instruction-selection shape tests
//  (direct IR construction, like test_optimizer.cpp's pass
//  tests — sidesteps the front-end's int/bool type rules so
//  comparison codegen can be exercised in isolation; see the
//  "known gap" test below for why source text can't do this).
// ============================================================

TEST("codegen: main gets _main symbol, andl alignment, and call ___main", [](){
    IRFunction fn;
    fn.name = "main";
    fn.instructions.push_back(IRInstruction::makeReturn(IRValue::makeConst(0)));
    IRProgram p; p.functions.push_back(fn);

    AssemblyGenerator gen;
    std::string text = gen.generate(p).toString();
    ASSERT_TRUE(text.find("_main:") != std::string::npos);
    ASSERT_TRUE(text.find("andl    $-16, %esp") != std::string::npos);
    ASSERT_TRUE(text.find("call    ___main") != std::string::npos);
});

TEST("codegen: non-main function gets plain symbol, no andl/call ___main", [](){
    IRFunction fn;
    fn.name = "helper";
    fn.instructions.push_back(IRInstruction::makeReturn(IRValue::makeConst(0)));
    IRProgram p; p.functions.push_back(fn);

    AssemblyGenerator gen;
    std::string text = gen.generate(p).toString();
    ASSERT_TRUE(text.find("_helper:") != std::string::npos);
    ASSERT_TRUE(text.find("andl") == std::string::npos);
    ASSERT_TRUE(text.find("___main") == std::string::npos);
});

TEST("codegen: function with no Return still ends in leave/ret", [](){
    IRFunction fn;
    fn.name = "main";
    fn.instructions.push_back(IRInstruction::makeCopy(IRValue::makeVar("x"), IRValue::makeConst(5)));
    IRProgram p; p.functions.push_back(fn);

    AssemblyGenerator gen;
    auto lines = gen.generate(p).lines;
    // Strip trailing blank lines, then check the last two non-blank lines.
    int i = (int)lines.size() - 1;
    while (i >= 0 && lines[i].empty()) --i;
    ASSERT_TRUE(i >= 1);
    ASSERT_TRUE(lines[i].find("ret") != std::string::npos);
    ASSERT_TRUE(lines[i - 1].find("leave") != std::string::npos);
});

// ── Provenance comments (span threading through codegen) ──────
TEST("provenance: assembly comments carry the source line of each IR instruction", [](){
    auto asmProg = genAsmFromSource(
        "int main() {\n"
        "    int x = 5;\n"
        "    return x + 2;\n"
        "}\n"
    );
    std::string text = asmProg.toString();
    ASSERT_TRUE(text.find("# line 2: x = 5")      != std::string::npos);
    ASSERT_TRUE(text.find("# line 3: t0 = x + 2") != std::string::npos);
    ASSERT_TRUE(text.find("# line 3: return t0")  != std::string::npos);
});

TEST("provenance: direct-IR (no source) keeps the plain comment form", [](){
    IRFunction fn;
    fn.name = "main";
    fn.instructions.push_back(IRInstruction::makeReturn(IRValue::makeConst(0)));
    IRProgram p; p.functions.push_back(fn);

    AssemblyGenerator gen;
    std::string text = gen.generate(p).toString();
    ASSERT_TRUE(text.find("# return 0") != std::string::npos);
    ASSERT_TRUE(text.find("# line")     == std::string::npos);
});

// ============================================================
//  End-to-end: assemble + link + run real generated code and
//  check the process exit code.
// ============================================================

TEST("end-to-end: target program int main(){int x=5; return x+2;} exits 7", [](){
    auto asmProg = genAsmFromSource(
        "int main() {\n"
        "    int x = 5;\n"
        "    return x + 2;\n"
        "}\n"
    );
    ASSERT_EQ(assembleLinkRun(asmProg, "codegen_target"), 7);
});

TEST("end-to-end: bug report — return x = 3 (assignment expr through the full real pipeline) exits 3", [](){
    auto asmProg = genAsmFromSource(
        "int main() {\n"
        "    int x = 5;\n"
        "    return x = 3;\n"
        "}\n"
    );
    ASSERT_EQ(assembleLinkRun(asmProg, "codegen_assign"), 3);
});

TEST("end-to-end: bug report — a = b = c chained assignment exits 7", [](){
    auto asmProg = genAsmFromSource(
        "int main() {\n"
        "    int a = 0;\n"
        "    int b = 0;\n"
        "    int c = 7;\n"
        "    return a = b = c;\n"
        "}\n"
    );
    ASSERT_EQ(assembleLinkRun(asmProg, "codegen_chain_assign"), 7);
});

TEST("end-to-end: subtraction with constant operands", [](){
    IRFunction fn; fn.name = "main";
    fn.instructions.push_back(IRInstruction::makeBinary(
        IROp::Sub, IRValue::makeTemp(0), IRValue::makeConst(50), IRValue::makeConst(8)));
    fn.instructions.push_back(IRInstruction::makeReturn(IRValue::makeTemp(0)));
    IRProgram p; p.functions.push_back(fn);

    AssemblyGenerator gen;
    ASSERT_EQ(assembleLinkRun(gen.generate(p), "codegen_sub"), 42);
});

TEST("end-to-end: multiplication with a variable operand (imull mem,reg)", [](){
    IRFunction fn; fn.name = "main";
    fn.instructions.push_back(IRInstruction::makeCopy(IRValue::makeVar("x"), IRValue::makeConst(6)));
    fn.instructions.push_back(IRInstruction::makeBinary(
        IROp::Mul, IRValue::makeTemp(0), IRValue::makeVar("x"), IRValue::makeConst(7)));
    fn.instructions.push_back(IRInstruction::makeReturn(IRValue::makeTemp(0)));
    IRProgram p; p.functions.push_back(fn);

    AssemblyGenerator gen;
    ASSERT_EQ(assembleLinkRun(gen.generate(p), "codegen_mul"), 42);
});

TEST("end-to-end: division by a constant (idivl needs a register/mem divisor)", [](){
    IRFunction fn; fn.name = "main";
    fn.instructions.push_back(IRInstruction::makeBinary(
        IROp::Div, IRValue::makeTemp(0), IRValue::makeConst(100), IRValue::makeConst(4)));
    fn.instructions.push_back(IRInstruction::makeReturn(IRValue::makeTemp(0)));
    IRProgram p; p.functions.push_back(fn);

    AssemblyGenerator gen;
    ASSERT_EQ(assembleLinkRun(gen.generate(p), "codegen_div_const"), 25);
});

TEST("end-to-end: division by a variable (idivl mem directly, no temp register needed)", [](){
    IRFunction fn; fn.name = "main";
    fn.instructions.push_back(IRInstruction::makeCopy(IRValue::makeVar("x"), IRValue::makeConst(100)));
    fn.instructions.push_back(IRInstruction::makeCopy(IRValue::makeVar("y"), IRValue::makeConst(4)));
    fn.instructions.push_back(IRInstruction::makeBinary(
        IROp::Div, IRValue::makeTemp(0), IRValue::makeVar("x"), IRValue::makeVar("y")));
    fn.instructions.push_back(IRInstruction::makeReturn(IRValue::makeTemp(0)));
    IRProgram p; p.functions.push_back(fn);

    AssemblyGenerator gen;
    ASSERT_EQ(assembleLinkRun(gen.generate(p), "codegen_div_var"), 25);
});

TEST("end-to-end: all four comparisons combined (cmpl+setX+movzbl)", [](){
    // a=17 b=5: lt=0, gt=1, eq=1 (a==17), ne=1(a!=b) -> sum=3.
    // Built directly in IR because the front-end's type system has
    // no implicit bool->int conversion, so source text cannot
    // express "store a comparison result and combine it" (see the
    // "known gap" test below for the analogous call-expression gap).
    IRFunction fn; fn.name = "main";
    fn.instructions.push_back(IRInstruction::makeCopy(IRValue::makeVar("a"), IRValue::makeConst(17)));
    fn.instructions.push_back(IRInstruction::makeCopy(IRValue::makeVar("b"), IRValue::makeConst(5)));
    fn.instructions.push_back(IRInstruction::makeBinary(IROp::Lt, IRValue::makeVar("lt"), IRValue::makeVar("a"), IRValue::makeVar("b")));
    fn.instructions.push_back(IRInstruction::makeBinary(IROp::Gt, IRValue::makeVar("gt"), IRValue::makeVar("a"), IRValue::makeVar("b")));
    fn.instructions.push_back(IRInstruction::makeBinary(IROp::Eq, IRValue::makeVar("eq"), IRValue::makeVar("a"), IRValue::makeConst(17)));
    fn.instructions.push_back(IRInstruction::makeBinary(IROp::Ne, IRValue::makeVar("ne"), IRValue::makeVar("a"), IRValue::makeVar("b")));
    fn.instructions.push_back(IRInstruction::makeBinary(IROp::Add, IRValue::makeTemp(0), IRValue::makeVar("lt"), IRValue::makeVar("gt")));
    fn.instructions.push_back(IRInstruction::makeBinary(IROp::Add, IRValue::makeTemp(1), IRValue::makeTemp(0), IRValue::makeVar("eq")));
    fn.instructions.push_back(IRInstruction::makeBinary(IROp::Add, IRValue::makeTemp(2), IRValue::makeTemp(1), IRValue::makeVar("ne")));
    fn.instructions.push_back(IRInstruction::makeReturn(IRValue::makeTemp(2)));
    IRProgram p; p.functions.push_back(fn);

    AssemblyGenerator gen;
    ASSERT_EQ(assembleLinkRun(gen.generate(p), "codegen_cmp"), 3);
});

TEST("end-to-end: generated function reads cdecl parameters correctly", [](){
    // Tests param offsets from the CALLEE side using a hand-written
    // caller — this project's IR has no Call opcode yet (see the
    // "known gap" test below), so the caller half is written
    // directly in AT&T syntax, exactly as validated standalone in
    // build/spike_test3.s. add2(12, 30) should return 42.
    IRFunction fn;
    fn.name = "add2";
    fn.paramNames.push_back("a");
    fn.paramNames.push_back("b");
    fn.instructions.push_back(IRInstruction::makeBinary(
        IROp::Add, IRValue::makeTemp(0), IRValue::makeVar("a"), IRValue::makeVar("b")));
    fn.instructions.push_back(IRInstruction::makeReturn(IRValue::makeTemp(0)));
    IRProgram p; p.functions.push_back(fn);

    AssemblyGenerator gen;
    AssemblyProgram asmProg = gen.generate(p);

    asmProg.emit("    .globl  _main");
    asmProg.emit("_main:");
    asmProg.emit("    pushl   %ebp");
    asmProg.emit("    movl    %esp, %ebp");
    asmProg.emit("    andl    $-16, %esp");
    asmProg.emit("    call    ___main");
    asmProg.emit("    pushl   $30");
    asmProg.emit("    pushl   $12");
    asmProg.emit("    call    _add2");
    asmProg.emit("    addl    $8, %esp");
    asmProg.emit("    leave");
    asmProg.emit("    ret");

    ASSERT_EQ(assembleLinkRun(asmProg, "codegen_params"), 42);
});

TEST("end-to-end: two independent functions coexist in one program", [](){
    auto asmProg = genAsmFromSource(
        "int double_it(int n) {\n"
        "    return n + n;\n"
        "}\n"
        "int main() {\n"
        "    return 9;\n"
        "}\n"
    );
    ASSERT_EQ(assembleLinkRun(asmProg, "codegen_multi_fn"), 9);
});

// ============================================================
//  Known scope boundary — documented, not an oversight.
//
//  Function CALL expressions do not exist anywhere in this
//  compiler yet: no grammar rule, no AST node, no IROp::Call.
//  Every function compiles independently; main() can only
//  return its own computed value, never another function's.
//  This test locks that boundary in so a future change adding
//  call expressions does so deliberately, not by accident.
// ============================================================
TEST("known gap: call expressions are not yet parseable", [](){
    Lexer lexer("int f(int n) { return n; } int main() { return f(1); }");
    auto lexOut = lexer.tokenize();
    Parser parser(lexOut.output);
    auto parseOut = parser.parse();
    ASSERT_TRUE(!parseOut.diagnostics.empty());
});

int main() {
    return RUN_ALL_TESTS();
}
