#include <iostream>
#include <string>
#include "lexer/Lexer.hpp"
#include "diagnostics/DiagnosticCollector.hpp"

// ============================================================
//  main.cpp — interactive driver.
//
//  Modes:
//    1. Demo: target program (clean)
//    2. Demo: error cases (diagnostic showcase)
//    3. REPL
// ============================================================

static void printTokens(const std::vector<Token>& tokens) {
    for (const Token& tok : tokens) {
        std::cout << "  [" << tok.toString() << "]"
                  << "  line=" << tok.line
                  << "  col="  << tok.column
                  << "\n";
    }
}

static void runSource(const std::string& src,
                      const std::string& label,
                      bool showDiagnosticsFull = false) {
    std::cout << "\n";
    std::cout << "====  " << label << "  ====\n";
    std::cout << "Source:\n" << src << "\n";

    Lexer lexer(src);
    auto result = lexer.tokenize();

    std::cout << "Tokens:\n";
    printTokens(result.output);

    if (!result.diagnostics.empty()) {
        DiagnosticCollector collector;
        collector.addAll(result.diagnostics);

        std::cout << "\n";
        if (showDiagnosticsFull) {
            collector.render(std::cout, src);
        } else {
            collector.renderCompact(std::cout);
        }
    }
}

int main() {
    std::cout << "cpp-compiler  |  Explainable Diagnostics  |  Stage 1: Lexer\n";
    std::cout << std::string(62, '=') << "\n";

    // ── Demo 1: clean program ─────────────────────────────
    runSource(
        "int main() {\n"
        "    int x = 5;\n"
        "    return x + 2;\n"
        "}\n",
        "Target program (clean)"
    );

    // ── Demo 2: required output from spec ─────────────────
    runSource("return x + 5;", "Required spec output");

    // ── Demo 3: EXPLAINABLE diagnostic showcase ───────────
    std::cout << "\n\n";
    std::cout << "====  EXPLAINABLE DIAGNOSTIC SHOWCASE  ====\n";
    std::cout << "This is what separates this compiler from traditional compilers.\n";
    std::cout << "Every error includes: WHERE, WHY, HOW TO FIX, and INTERNAL TRACE.\n";

    {
        std::string src = "int @x = 5;";
        std::cout << "\nSource: " << src << "\n\n";
        Lexer lexer(src);
        auto result = lexer.tokenize();
        DiagnosticCollector collector;
        collector.addAll(result.diagnostics);
        collector.render(std::cout, src);
    }

    {
        std::string src = "int x = 5; /* unterminated comment";
        std::cout << "\nSource: " << src << "\n\n";
        Lexer lexer(src);
        auto result = lexer.tokenize();
        DiagnosticCollector collector;
        collector.addAll(result.diagnostics);
        collector.render(std::cout, src);
    }

    // ── Demo 4: JSON output (for visualizer) ─────────────
    std::cout << "\n====  JSON OUTPUT (visualizer feed)  ====\n";
    {
        std::string src = "int @x;";
        Lexer lexer(src);
        auto result = lexer.tokenize();
        DiagnosticCollector collector;
        collector.addAll(result.diagnostics);
        collector.renderJson(std::cout, src);
    }

    // ── REPL ─────────────────────────────────────────────
    std::cout << "\n====  Interactive REPL (empty line to quit)  ====\n";
    std::string line;
    while (true) {
        std::cout << "lex> ";
        if (!std::getline(std::cin, line) || line.empty()) break;
        runSource(line, "input", true);
    }

    return 0;
}
