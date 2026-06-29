#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include "lexer/Lexer.hpp"
#include "parser/Parser.hpp"
#include "ast/ASTPrinter.hpp"
#include "semantic/SemanticAnalyzer.hpp"
#include "ir/IRGenerator.hpp"
#include "optimizer/Optimizer.hpp"
#include "codegen/AssemblyGenerator.hpp"
#include "driver/Toolchain.hpp"
#include "diagnostics/DiagnosticCollector.hpp"

static void compile(const std::string& src,
                    const std::string& label,
                    bool showFullDiag = false) {
    const std::string bar(62, '=');
    std::cout << "\n" << bar << "\n  " << label << "\n" << bar << "\n";
    std::cout << "Source:\n" << src << "\n";

    DiagnosticCollector collector;

    // ── Stage 1: Lex ────────────────────────────────────
    Lexer lexer(src);
    auto lexOut = lexer.tokenize();
    collector.addAll(lexOut.diagnostics);

    std::cout << "Tokens:\n";
    for (auto& tok : lexOut.output)
        if (tok.type != TokenType::END_OF_FILE)
            std::cout << "  " << tok.toString()
                      << " [" << tok.line << ":" << tok.column << "]\n";

    if (collector.hasErrors()) {
        std::cout << "\n-- Lex errors (stopping before parse) --\n";
        collector.render(std::cout, src);
        return;
    }

    // ── Stage 2: Parse ──────────────────────────────────
    Parser parser(lexOut.output);
    auto parseOut = parser.parse();
    collector.addAll(parseOut.diagnostics);

    std::cout << "\nAST (before semantic analysis):\n";
    if (parseOut.output) {
        ASTPrinter printer(std::cout);
        parseOut.output->accept(printer);
    }

    if (collector.hasErrors()) {
        std::cout << "\n-- Parse errors (stopping before semantic) --\n";
        if (showFullDiag) collector.render(std::cout, src);
        else              collector.renderCompact(std::cout);
        return;
    }

    // ── Stage 3: Semantic Analysis ───────────────────────
    SemanticAnalyzer analyzer;
    auto semOut = analyzer.analyze(*parseOut.output);
    collector.addAll(semOut.diagnostics);

    std::cout << "\nSemantic checks:\n";
    for (auto& entry : analyzer.log())
        std::cout << entry << "\n";

    std::cout << "\nAnnotated AST:\n";
    if (parseOut.output) {
        ASTPrinter printer(std::cout);
        parseOut.output->accept(printer);
    }

    if (collector.hasErrors()) {
        std::cout << "\n";
        if (showFullDiag) collector.render(std::cout, src);
        else              collector.renderCompact(std::cout);
        return;
    }

    std::cout << "\nCompilation OK — all " << (int)analyzer.log().size()
              << " semantic checks passed.\n";

    // ── Stage 4: IR Generation ───────────────────────────
    IRGenerator irgen;
    IRProgram ir = irgen.generate(*parseOut.output);

    std::cout << "\nIR (before optimization):\n" << ir.toString();

    // ── Stage 5: Optimization ─────────────────────────────
    Optimizer optimizer;
    auto reports = optimizer.optimize(ir);   // mutates ir in place

    std::cout << "\nIR (after optimization):\n" << ir.toString();

    for (auto& r : reports) {
        std::cout << "\nOptimization report [" << r.functionName << "]: "
                   << r.instructionsBefore << " -> " << r.instructionsAfter
                   << " instructions, " << r.iterations << " fixed-point iteration(s)\n";
        for (auto& step : r.passesApplied) std::cout << "  " << step << "\n";
    }

    // ── Stage 6: Assembly Generation ─────────────────────
    AssemblyGenerator codegen;
    AssemblyProgram asmProg = codegen.generate(ir);

    std::cout << "\nAssembly (x86, AT&T syntax):\n" << asmProg.toString();
}

// ============================================================
//  compileFile — Stage 7: the compiler's real CLI entry point.
//
//  Deliberately separate from compile() above rather than a
//  shared helper with a verbosity flag: compile() exists to NARRATE
//  every stage for the portfolio demo, while this exists to BUILD
//  an executable quietly, the way `g++ file.cpp -o prog` does —
//  silent on success, one diagnostic-rich report on failure. The
//  output contracts are different enough that sharing one function
//  would mean threading print statements through with conditionals,
//  not actually removing duplication.
//
//  Errors still get the FULL render() (WHY/FIX/TRACE), not the
//  compact form compile()'s demos sometimes use — explaining every
//  error is this project's whole thesis, so the real CLI should not
//  default to less explanation than the demo mode does.
// ============================================================
static int compileFile(const std::string& inputPath, const std::string& outputPath) {
    std::ifstream in(inputPath);
    if (!in) {
        std::cerr << "error: could not open '" << inputPath << "'\n";
        return 1;
    }
    std::stringstream buf;
    buf << in.rdbuf();
    const std::string src = buf.str();

    DiagnosticCollector collector;

    Lexer lexer(src);
    auto lexOut = lexer.tokenize();
    collector.addAll(lexOut.diagnostics);
    if (collector.hasErrors()) {
        collector.render(std::cerr, src);
        return 1;
    }

    Parser parser(lexOut.output);
    auto parseOut = parser.parse();
    collector.addAll(parseOut.diagnostics);
    if (collector.hasErrors()) {
        collector.render(std::cerr, src);
        return 1;
    }

    SemanticAnalyzer analyzer;
    auto semOut = analyzer.analyze(*parseOut.output);
    collector.addAll(semOut.diagnostics);
    if (collector.hasErrors()) {
        collector.render(std::cerr, src);
        return 1;
    }

    IRGenerator irgen;
    IRProgram ir = irgen.generate(*parseOut.output);

    Optimizer optimizer;
    optimizer.optimize(ir);

    AssemblyGenerator codegen;
    AssemblyProgram asmProg = codegen.generate(ir);

    Toolchain toolchain;
    auto built = toolchain.buildExecutable(asmProg, outputPath);
    if (built.isErr()) {
        std::cerr << "error: " << built.error() << "\n";
        return 1;
    }

    std::cout << "Built " << built.get() << "\n";
    return 0;
}

int main(int argc, char** argv) {
    if (argc >= 2) {
        std::string inputPath  = argv[1];
        std::string outputPath = "a.exe";
        for (int i = 2; i + 1 < argc; ++i) {
            if (std::string(argv[i]) == "-o") outputPath = argv[i + 1];
        }
        return compileFile(inputPath, outputPath);
    }

    std::cout << "cpp-compiler  |  Stages 1-7: Lexer + Parser + Semantic + IR + Optimizer + Codegen + Executable\n";
    std::cout << "(pass a source file as an argument to compile it directly: ./compiler input.cpp -o output.exe)\n";

    // ── Demo 1: target program — everything passes ───────
    compile(
        "int main() {\n"
        "    int x = 5;\n"
        "    return x + 2;\n"
        "}\n",
        "Target program — full pipeline"
    );

    // ── Demo: simple constant folding ────────────────────
    compile(
        "int main() {\n"
        "    return 2 + 3;\n"
        "}\n",
        "Optimizer demo: 2 + 3 folds to a single return"
    );

    // ── Demo: the fixed-point case — folding cascades ────
    // t0=2+3 folds first; THEN t1=t0*4 becomes foldable only
    // after copy propagation substitutes t0 -> 5. Needs 3
    // iterations to fully collapse to "return 20".
    compile(
        "int main() {\n"
        "    return (2 + 3) * 4;\n"
        "}\n",
        "Optimizer demo: (2+3)*4 needs multiple fixed-point iterations"
    );

    // ── Demo: nested expression precedence in IR ─────────
    compile(
        "int compute(int a, int b, int c) {\n"
        "    return a + b * c;\n"
        "}\n",
        "IR demo: a + b * c lowers to two temps in the right order"
    );

    // ── Demo 2: undeclared variable ───────────────────────
    compile(
        "int main() {\n"
        "    int x = 5;\n"
        "    return y + 2;\n"
        "}\n",
        "Semantic error: undeclared identifier 'y'",
        true
    );

    // ── Demo 3: redeclared variable ───────────────────────
    compile(
        "int main() {\n"
        "    int x = 5;\n"
        "    int x = 10;\n"
        "    return x;\n"
        "}\n",
        "Semantic error: redeclared variable 'x'",
        true
    );

    // ── Demo 4: multiple errors in one function ───────────
    compile(
        "int main() {\n"
        "    return a + b;\n"
        "}\n",
        "Semantic error: two undeclared identifiers",
        true
    );

    // ── Demo 5: function parameter accessible ─────────────
    compile(
        "int double_it(int n) {\n"
        "    return n + n;\n"
        "}\n",
        "Parameter accessible in body"
    );

    // ── REPL ─────────────────────────────────────────────
    std::cout << "\n=== Interactive REPL (empty line to quit) ===\n";
    std::string line;
    while (true) {
        std::cout << "compile> ";
        if (!std::getline(std::cin, line) || line.empty()) break;
        compile(line, "input", true);
    }

    return 0;
}
