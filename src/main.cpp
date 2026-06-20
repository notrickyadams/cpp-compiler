#include <iostream>
#include <string>
#include "lexer/Lexer.hpp"

// ============================================================
//  main.cpp — interactive driver for the lexer stage.
//
//  Two modes:
//    1. Hardcoded demo (shows the target program tokenized)
//    2. REPL loop (type source, see tokens)
// ============================================================

static void printTokens(const std::vector<Token>& tokens) {
    for (const Token& tok : tokens) {
        std::cout << "  [" << tok.toString() << "]"
                  << "  line=" << tok.line
                  << "  col="  << tok.column
                  << "\n";
    }
}

static void runSource(const std::string& src) {
    std::cout << "\n--- Source ---\n" << src << "\n";
    std::cout << "--- Tokens ---\n";

    Lexer lexer(src);
    std::vector<Token> tokens = lexer.tokenize();

    printTokens(tokens);

    if (lexer.hasErrors()) {
        std::cout << "\n--- Lex Errors ---\n";
        for (const LexError& e : lexer.errors()) {
            std::cout << "  " << e.toString() << "\n";
        }
    }
}

int main() {
    // ── Demo: the target program ──────────────────────────
    std::cout << "=== cpp-compiler  |  Stage 1: Lexer Demo ===\n";

    runSource(
        "int main() {\n"
        "    int x = 5;\n"
        "    return x + 2;\n"
        "}\n"
    );

    // ── Demo: the specific required output ───────────────
    std::cout << "\n=== Required output demo ===\n";
    runSource("return x + 5;");

    // ── REPL ─────────────────────────────────────────────
    std::cout << "\n=== Interactive REPL (Ctrl+C or empty line to quit) ===\n";
    std::string line;
    while (true) {
        std::cout << "lex> ";
        if (!std::getline(std::cin, line) || line.empty()) break;
        runSource(line);
    }

    return 0;
}
