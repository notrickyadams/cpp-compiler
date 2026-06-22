#include "test_runner.hpp"
#include "../src/lexer/Lexer.hpp"
#include "../src/parser/Parser.hpp"
#include "../src/semantic/SemanticAnalyzer.hpp"
#include "../src/ir/IRGenerator.hpp"
#include "../src/optimizer/Optimizer.hpp"
#include "../src/optimizer/ConstantFoldingPass.hpp"
#include "../src/optimizer/CopyPropagationPass.hpp"
#include "../src/optimizer/DeadCodeEliminationPass.hpp"
#include <memory>
#include <string>

// ============================================================
//  Helper: full pipeline lex -> parse -> semantic -> IR -> optimize.
// ============================================================
struct OptResult {
    IRProgram                        ir;
    std::vector<OptimizationReport>  reports;
};

static OptResult optimizeSource(const std::string& src) {
    Lexer lexer(src);
    auto lexOut = lexer.tokenize();
    Parser parser(lexOut.output);
    auto parseOut = parser.parse();
    SemanticAnalyzer analyzer;
    analyzer.analyze(*parseOut.output);
    IRGenerator irgen;
    IRProgram ir = irgen.generate(*parseOut.output);

    Optimizer optimizer;
    auto reports = optimizer.optimize(ir);
    return { std::move(ir), std::move(reports) };
}

// ── Integration: full pipeline optimization ───────────────────

TEST("optimizer: return 2+3 folds to a single return 5", [](){
    auto r = optimizeSource("int main() { return 2 + 3; }");
    auto& instrs = r.ir.functions[0].instructions;
    ASSERT_EQ((int)instrs.size(), 1);
    ASSERT_EQ(instrs[0].toString(), std::string("return 5"));
});

TEST("optimizer: (2+3)*4 fully collapses via fixed-point iteration", [](){
    auto r = optimizeSource("int main() { return (2 + 3) * 4; }");
    auto& instrs = r.ir.functions[0].instructions;
    ASSERT_EQ((int)instrs.size(), 1);
    ASSERT_EQ(instrs[0].toString(), std::string("return 20"));
    // Proves the SINGLE-PASS theory would be insufficient —
    // this genuinely requires multiple rounds to converge.
    ASSERT_TRUE(r.reports[0].iterations >= 2);
});

TEST("optimizer: already-minimal IR is left completely unchanged", [](){
    auto r = optimizeSource("int main() { int x = 5; return x + 2; }");
    auto& instrs = r.ir.functions[0].instructions;
    ASSERT_EQ((int)instrs.size(), 3);
    ASSERT_EQ(instrs[0].toString(), std::string("x = 5"));
    ASSERT_EQ(instrs[1].toString(), std::string("t0 = x + 2"));
    ASSERT_EQ(instrs[2].toString(), std::string("return t0"));
});

TEST("optimizer: report records correct before/after instruction counts", [](){
    auto r = optimizeSource("int main() { return 2 + 3; }");
    ASSERT_EQ(r.reports[0].instructionsBefore, 2);
    ASSERT_EQ(r.reports[0].instructionsAfter,  1);
});

TEST("optimizer: named variable's value is never substituted across uses", [](){
    // x is a Var, not a Temp -- copy propagation must not inline it,
    // even though in THIS program it's never reassigned.
    auto r = optimizeSource("int main() { int x = 5; return x + 2; }");
    auto& instrs = r.ir.functions[0].instructions;
    bool foundXPlus2 = false;
    for (auto& i : instrs) if (i.toString() == "t0 = x + 2") foundXPlus2 = true;
    ASSERT_TRUE(foundXPlus2);
});

// ── Unit: ConstantFoldingPass in isolation ─────────────────────

TEST("ConstantFolding: 2+3 becomes a Copy of the literal 5", [](){
    IRFunction fn;
    fn.instructions.push_back(IRInstruction::makeBinary(
        IROp::Add, IRValue::makeTemp(0), IRValue::makeConst(2), IRValue::makeConst(3)));

    ConstantFoldingPass pass;
    bool changed = pass.run(fn);

    ASSERT_TRUE(changed);
    ASSERT_EQ(fn.instructions[0].toString(), std::string("t0 = 5"));
});

TEST("ConstantFolding: does nothing when an operand is a variable", [](){
    IRFunction fn;
    fn.instructions.push_back(IRInstruction::makeBinary(
        IROp::Add, IRValue::makeTemp(0), IRValue::makeVar("x"), IRValue::makeConst(2)));

    ConstantFoldingPass pass;
    bool changed = pass.run(fn);

    ASSERT_TRUE(!changed);
    ASSERT_EQ(fn.instructions[0].toString(), std::string("t0 = x + 2"));
});

TEST("ConstantFolding: skips divide-by-zero, leaves it unfolded", [](){
    IRFunction fn;
    fn.instructions.push_back(IRInstruction::makeBinary(
        IROp::Div, IRValue::makeTemp(0), IRValue::makeConst(5), IRValue::makeConst(0)));

    ConstantFoldingPass pass;
    bool changed = pass.run(fn);

    ASSERT_TRUE(!changed);
    ASSERT_EQ(fn.instructions[0].toString(), std::string("t0 = 5 / 0"));
});

TEST("ConstantFolding: comparison operators fold to 0/1", [](){
    IRFunction fn;
    fn.instructions.push_back(IRInstruction::makeBinary(
        IROp::Lt, IRValue::makeTemp(0), IRValue::makeConst(2), IRValue::makeConst(5)));

    ConstantFoldingPass pass;
    pass.run(fn);
    ASSERT_EQ(fn.instructions[0].toString(), std::string("t0 = 1"));
});

// ── Unit: CopyPropagationPass in isolation ─────────────────────

TEST("CopyPropagation: substitutes a temp's known constant at its use site", [](){
    IRFunction fn;
    fn.instructions.push_back(IRInstruction::makeCopy(
        IRValue::makeTemp(0), IRValue::makeConst(5)));
    fn.instructions.push_back(IRInstruction::makeBinary(
        IROp::Add, IRValue::makeTemp(1), IRValue::makeTemp(0), IRValue::makeConst(2)));

    CopyPropagationPass pass;
    bool changed = pass.run(fn);

    ASSERT_TRUE(changed);
    ASSERT_EQ(fn.instructions[1].toString(), std::string("t1 = 5 + 2"));
});

TEST("CopyPropagation: never substitutes a named variable", [](){
    IRFunction fn;
    fn.instructions.push_back(IRInstruction::makeCopy(
        IRValue::makeVar("x"), IRValue::makeConst(5)));
    fn.instructions.push_back(IRInstruction::makeBinary(
        IROp::Add, IRValue::makeTemp(0), IRValue::makeVar("x"), IRValue::makeConst(2)));

    CopyPropagationPass pass;
    bool changed = pass.run(fn);

    ASSERT_TRUE(!changed);
    ASSERT_EQ(fn.instructions[1].toString(), std::string("t0 = x + 2"));
});

TEST("CopyPropagation: resolves a chain of temp copies in a single pass", [](){
    IRFunction fn;
    fn.instructions.push_back(IRInstruction::makeCopy(IRValue::makeTemp(0), IRValue::makeConst(5)));
    fn.instructions.push_back(IRInstruction::makeCopy(IRValue::makeTemp(1), IRValue::makeTemp(0)));
    fn.instructions.push_back(IRInstruction::makeBinary(
        IROp::Add, IRValue::makeTemp(2), IRValue::makeTemp(1), IRValue::makeConst(1)));

    CopyPropagationPass pass;
    pass.run(fn);

    ASSERT_EQ(fn.instructions[2].toString(), std::string("t2 = 5 + 1"));
});

// ── Unit: DeadCodeEliminationPass in isolation ──────────────────

TEST("DCE: removes an instruction whose temp dest is never used", [](){
    IRFunction fn;
    fn.instructions.push_back(IRInstruction::makeBinary(
        IROp::Add, IRValue::makeTemp(0), IRValue::makeConst(1), IRValue::makeConst(2)));
    fn.instructions.push_back(IRInstruction::makeReturn(IRValue::makeConst(99)));

    DeadCodeEliminationPass pass;
    bool changed = pass.run(fn);

    ASSERT_TRUE(changed);
    ASSERT_EQ((int)fn.instructions.size(), 1);
    ASSERT_EQ(fn.instructions[0].toString(), std::string("return 99"));
});

TEST("DCE: keeps an instruction whose temp dest IS used", [](){
    IRFunction fn;
    fn.instructions.push_back(IRInstruction::makeBinary(
        IROp::Add, IRValue::makeTemp(0), IRValue::makeConst(1), IRValue::makeConst(2)));
    fn.instructions.push_back(IRInstruction::makeReturn(IRValue::makeTemp(0)));

    DeadCodeEliminationPass pass;
    bool changed = pass.run(fn);

    ASSERT_TRUE(!changed);
    ASSERT_EQ((int)fn.instructions.size(), 2);
});

TEST("DCE: never removes a named-variable destination", [](){
    IRFunction fn;
    fn.instructions.push_back(IRInstruction::makeCopy(
        IRValue::makeVar("x"), IRValue::makeConst(5)));
    fn.instructions.push_back(IRInstruction::makeReturn(IRValue::makeConst(0)));

    DeadCodeEliminationPass pass;
    bool changed = pass.run(fn);

    ASSERT_TRUE(!changed);
    ASSERT_EQ((int)fn.instructions.size(), 2);
});

int main() {
    std::cout << "=== Optimizer Unit Tests ===\n\n";
    return RUN_ALL_TESTS();
}
