#include <iostream>
#include <string>
#include "lexer/Lexer.hpp"
#include "parser/Parser.hpp"
#include "ast/ASTPrinter.hpp"
#include "semantic/SemanticAnalyzer.hpp"
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
    } else {
        std::cout << "\nCompilation OK — all " << (int)analyzer.log().size()
                  << " semantic checks passed.\n";
    }
}

int main() {
    std::cout << "cpp-compiler  |  Stages 1-3: Lexer + Parser + Semantic\n";

    // ── Demo 1: target program — everything passes ───────
    compile(
        "int main() {\n"
        "    int x = 5;\n"
        "    return x + 2;\n"
        "}\n",
        "Target program — full pipeline"
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
