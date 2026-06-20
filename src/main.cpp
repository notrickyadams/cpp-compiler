#include <iostream>
#include <string>
#include "lexer/Lexer.hpp"
#include "parser/Parser.hpp"
#include "ast/ASTPrinter.hpp"
#include "diagnostics/DiagnosticCollector.hpp"

static void compile(const std::string& src,
                    const std::string& label,
                    bool showFullDiag = false) {
    std::cout << "\n" << std::string(62, '=') << "\n";
    std::cout << "  " << label << "\n";
    std::cout << std::string(62, '=') << "\n";
    std::cout << "Source:\n" << src << "\n";

    DiagnosticCollector collector;

    // ── Stage 1: Lex ────────────────────────────────────
    Lexer lexer(src);
    auto lexOut = lexer.tokenize();
    collector.addAll(lexOut.diagnostics);

    std::cout << "\nTokens:\n";
    for (auto& tok : lexOut.output) {
        if (tok.type == TokenType::END_OF_FILE) break;
        std::cout << "  [" << tok.toString() << "]"
                  << "  line=" << tok.line
                  << "  col="  << tok.column << "\n";
    }

    if (collector.hasErrors()) {
        std::cout << "\nLex errors — stopping before parse.\n";
        collector.render(std::cout, src);
        return;
    }

    // ── Stage 2: Parse ──────────────────────────────────
    Parser parser(lexOut.output);
    auto parseOut = parser.parse();
    collector.addAll(parseOut.diagnostics);

    std::cout << "\nAST:\n";
    if (parseOut.output) {
        ASTPrinter printer(std::cout);
        parseOut.output->accept(printer);
    }

    if (collector.hasErrors()) {
        std::cout << "\n";
        if (showFullDiag) {
            collector.render(std::cout, src);
        } else {
            collector.renderCompact(std::cout);
        }
    } else {
        std::cout << "\nCompilation OK — no errors.\n";
    }
}

int main() {
    std::cout << "cpp-compiler  |  Explainable Compiler  |  Stages 1-2: Lexer + Parser\n";

    // ── Demo 1: target program (clean) ──────────────────
    compile(
        "int main() {\n"
        "    int x = 5;\n"
        "    return x + 2;\n"
        "}\n",
        "Target program — clean"
    );

    // ── Demo 2: expression precedence ───────────────────
    compile(
        "int compute() {\n"
        "    int a = 1;\n"
        "    int b = 2;\n"
        "    return a + b * 3;\n"
        "}\n",
        "Operator precedence: a + b * 3"
    );

    // ── Demo 3: parse error — missing semicolon ──────────
    compile(
        "int main() {\n"
        "    int x = 5\n"
        "    return x + 2;\n"
        "}\n",
        "Parse error: missing semicolon",
        true
    );

    // ── Demo 4: parse error — missing closing brace ─────
    compile(
        "int main() {\n"
        "    return 42;\n",
        "Parse error: missing '}'",
        true
    );

    // ── REPL ────────────────────────────────────────────
    std::cout << "\n\n=== Interactive REPL (empty line to quit) ===\n";
    std::string line;
    while (true) {
        std::cout << "compile> ";
        if (!std::getline(std::cin, line) || line.empty()) break;
        compile(line, "input", true);
    }

    return 0;
}
