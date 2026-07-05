#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include "lexer/Lexer.hpp"
#include "parser/Parser.hpp"
#include "ast/ASTPrinter.hpp"
#include "ast/ASTJsonPrinter.hpp"
#include "semantic/SemanticAnalyzer.hpp"
#include "ir/IRGenerator.hpp"
#include "optimizer/Optimizer.hpp"
#include "codegen/AssemblyGenerator.hpp"
#include "driver/Toolchain.hpp"
#include "diagnostics/DiagnosticCollector.hpp"
#include "core/Json.hpp"

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

    // Warnings (e.g. SEM_MissingReturn) don't stop the build, but a
    // warning nobody sees is a warning that doesn't exist — render
    // them with the same full explanation errors get, then continue.
    if (collector.totalCount() > 0) {
        collector.render(std::cerr, src);
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

// ============================================================
//  checkFile — compile-only mode (gcc's -fsyntax-only analogue,
//  except it runs the WHOLE pipeline through assembly text).
//
//  Exists for two reasons:
//    1. It is the honest measurement target for the overhead
//       experiments (docs/experiments.md, E1/E2). Timing --json
//       measured PowerShell consuming megabytes of serialized
//       pipeline state, not the compiler — the first campaign's
//       numbers were incoherent (ablated configs "slower" than
//       full) and were discarded. --check does every stage's real
//       work and prints NOTHING on success, so a Stopwatch around
//       the process measures the pipeline and only the pipeline.
//    2. It is independently useful: validate a file and get the
//       full diagnostic report without producing artifacts.
//
//  The generated assembly is built and discarded — codegen is part
//  of the pipeline under measurement; only the OS-facing Toolchain
//  step (whose g++ subprocess would dwarf everything) is skipped.
// ============================================================
static int checkFile(const std::string& inputPath) {
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
    if (collector.totalCount() > 0) {
        collector.render(std::cerr, src);  // warnings, same as compileFile
    }

    IRGenerator irgen;
    IRProgram ir = irgen.generate(*parseOut.output);

    Optimizer optimizer;
    optimizer.optimize(ir);

    AssemblyGenerator codegen;
    AssemblyProgram asmProg = codegen.generate(ir);
    (void)asmProg;  // built and discarded — see header comment

    return 0;
}

// ============================================================
//  compileToJson — Stage 8: feeds the visualizer.
//
//  Runs the same pipeline as compileFile(), minus the final
//  Toolchain step — the visualizer shows pipeline STAGES, it has
//  no use for a linked .exe. Stops gathering further stages as
//  soon as collector.hasErrors(), same short-circuit compileFile()
//  uses, but unlike compileFile() it still emits whatever AST
//  exists (even a partially-recovered one after a parse error) —
//  seeing where the parser got confused has real value here.
//
//  Each field is a self-contained JSON value at the point the
//  pipeline stopped: stages that never ran emit "" / [] / null,
//  not absent keys, so the frontend can rely on a fixed shape.
// ============================================================
// Structured IR provenance for the visualizer: one object per
// instruction carrying its stable text form PLUS the source line it
// was lowered from and any optimizer transformation notes. Kept
// separate from the plain irBefore/irAfter strings (stable display/
// test format) — provenance rides alongside the text, not inside it,
// mirroring IRInstruction's own span/note design.
static std::string irToJsonDetail(const IRProgram& ir) {
    std::vector<std::string> fns;
    for (const auto& fn : ir.functions) {
        std::vector<std::string> instrs;
        for (const auto& instr : fn.instructions) {
            instrs.push_back(
                "{\"text\":\"" + jsonEscape(instr.toString()) +
                "\",\"line\":" + std::to_string(instr.span.startLine) +
                ",\"note\":\"" + jsonEscape(instr.note) + "\"}");
        }
        fns.push_back("{\"function\":\"" + jsonEscape(fn.name) +
                      "\",\"instructions\":" + jsonArray(instrs) + "}");
    }
    return jsonArray(fns);
}

static int compileToJson(const std::string& inputPath) {
    std::ifstream in(inputPath);
    if (!in) {
        std::cout << "{\"error\":\"could not open '" << jsonEscape(inputPath) << "'\"}\n";
        return 1;
    }
    std::stringstream buf;
    buf << in.rdbuf();
    const std::string src = buf.str();

    DiagnosticCollector collector;

    std::vector<std::string> tokenJsons;
    std::string              astJson = "null";
    std::vector<std::string> semanticLog;
    std::string              irBefore;
    std::string              irAfter;
    std::string              irDetailBefore = "[]";
    std::string              irDetailAfter  = "[]";
    std::vector<std::string> optReports;
    std::string              assembly;

    Lexer lexer(src);
    auto lexOut = lexer.tokenize();
    collector.addAll(lexOut.diagnostics);
    for (auto& tok : lexOut.output) {
        if (tok.type != TokenType::END_OF_FILE) tokenJsons.push_back(tok.toJson());
    }

    if (!collector.hasErrors()) {
        Parser parser(lexOut.output);
        auto parseOut = parser.parse();
        collector.addAll(parseOut.diagnostics);

        if (!collector.hasErrors() && parseOut.output) {
            SemanticAnalyzer analyzer;
            auto semOut = analyzer.analyze(*parseOut.output);
            collector.addAll(semOut.diagnostics);
            semanticLog = analyzer.log();
        }

        if (parseOut.output) {
            ASTJsonPrinter astPrinter;
            astJson = astPrinter.toJson(*parseOut.output);
        }

        if (!collector.hasErrors() && parseOut.output) {
            IRGenerator irgen;
            IRProgram ir = irgen.generate(*parseOut.output);
            irBefore       = ir.toString();
            irDetailBefore = irToJsonDetail(ir);

            Optimizer optimizer;
            auto reports = optimizer.optimize(ir);
            irAfter       = ir.toString();
            irDetailAfter = irToJsonDetail(ir);

            for (auto& r : reports) {
                optReports.push_back(
                    "Optimization report [" + r.functionName + "]: " +
                    std::to_string(r.instructionsBefore) + " -> " +
                    std::to_string(r.instructionsAfter) + " instructions, " +
                    std::to_string(r.iterations) + " fixed-point iteration(s)");
            }

            AssemblyGenerator codegen;
            AssemblyProgram asmProg = codegen.generate(ir);
            assembly = asmProg.toString();
        }
    }

    std::ostringstream diagJson;
    collector.renderJson(diagJson, src);

    std::ostringstream json;
    json << "{\n";
    json << "  \"source\": \""             << jsonEscape(src)      << "\",\n";
    json << "  \"tokens\": "                << jsonArray(tokenJsons) << ",\n";
    json << "  \"ast\": "                   << astJson               << ",\n";
    json << "  \"semanticLog\": "           << jsonStringArray(semanticLog) << ",\n";
    json << "  \"irBefore\": \""            << jsonEscape(irBefore) << "\",\n";
    json << "  \"irAfter\": \""             << jsonEscape(irAfter)  << "\",\n";
    json << "  \"irDetailBefore\": "        << irDetailBefore       << ",\n";
    json << "  \"irDetailAfter\": "         << irDetailAfter        << ",\n";
    json << "  \"optimizationReports\": "   << jsonStringArray(optReports)  << ",\n";
    json << "  \"assembly\": \""            << jsonEscape(assembly) << "\",\n";
    json << "  \"diagnosticsReport\": "     << diagJson.str()       << "\n";
    json << "}\n";

    std::cout << json.str();
    return 0;
}

int main(int argc, char** argv) {
    if (argc >= 2) {
        std::string inputPath  = argv[1];
        std::string outputPath = "a.exe";
        bool        jsonMode   = false;
        bool        checkMode  = false;
        for (int i = 2; i < argc; ++i) {
            std::string arg = argv[i];
            if (arg == "-o" && i + 1 < argc)      outputPath = argv[++i];
            else if (arg == "--json")             jsonMode = true;
            else if (arg == "--check")            checkMode = true;
        }
        if (checkMode) return checkFile(inputPath);
        if (jsonMode)  return compileToJson(inputPath);
        return compileFile(inputPath, outputPath);
    }

    std::cout << "cpp-compiler  |  Stages 1-8: Lexer + Parser + Semantic + IR + Optimizer + Codegen + Executable + Visualizer\n";
    std::cout << "(pass a source file to compile it directly: ./compiler input.cpp -o output.exe)\n";
    std::cout << "(pass --json instead of -o to dump the full pipeline as JSON for the visualizer)\n";

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
