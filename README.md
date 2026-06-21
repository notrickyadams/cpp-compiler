# cpp-compiler

A C++ compiler built from scratch — not just a translator, but a **compiler designed to explain itself**.

Every error includes: where it happened, why it's a problem, how to fix it, and the compiler's internal reasoning chain. Built stage by stage with production-quality architecture, unit tests, and clean separation of concerns.

---

## Pipeline status

| Stage | Status | What it does |
|---|---|---|
| Lexer | ✅ Done | Raw source text → typed token stream |
| Diagnostic System | ✅ Done | Structured errors with WHY / FIX / TRACE — built in from day one |
| Parser + AST | ✅ Done | Tokens → Abstract Syntax Tree with full operator precedence |
| Semantic Analysis | ✅ Done | Type checking, scope resolution, symbol table |
| IR Generation | 🔨 Next | AST → flat 3-address intermediate representation |
| Optimization | — | Constant folding, dead code elimination |
| Assembly Generation | — | IR → real x86 assembly |
| Executable | — | Assembled and linked binary |
| Visualizer | — | Interactive web UI: source → tokens → AST → IR → assembly |

---

## What makes this different

Traditional compilers output this:

```
error: invalid operands to binary expression
```

This compiler outputs this:

```
error[UndeclaredIdentifier]: undeclared identifier 'y'
  --> input:3:12

  3 |     return y + 2;
           ^
           +----- 'y' is used before it is declared

  Why this happened
  -----------------
  Every name used in a C++ program must be declared before it is used.
  The name 'y' has not been declared anywhere visible from this point.
  This catches typos and missing includes at compile time.

  How to fix
  ----------
  1. Declare 'y' before this point: int y = ...;
  2. Check for a typo in the identifier name
  3. Verify the variable is in scope

  Internal trace
  --------------
  [ok  ] SemanticAnalyzer::visitIdentifier()
  [FAIL] SymbolTable::lookup()   -- name not found in any scope
  [FAIL] DiagnosticEngine::undeclaredIdentifier()
```

---

## Target program

```cpp
int main() {
    int x = 5;
    return x + 2;
}
```

---

## Build

**Requires:** g++ with C++14 support (GCC 6+)

```bash
# All source files
g++ -std=c++14 -Wall -Wextra -Isrc \
    -o build/compiler \
    src/main.cpp \
    src/diagnostics/ExplanationBuilder.cpp \
    src/diagnostics/DiagnosticEngine.cpp \
    src/diagnostics/DiagnosticCollector.cpp \
    src/lexer/Lexer.cpp \
    src/ast/ASTPrinter.cpp \
    src/parser/Parser.cpp \
    src/semantic/SemanticAnalyzer.cpp

./build/compiler
```

---

## Test results

```
test_lexer       25/25 tests   (79  assertions)
test_diagnostics 15/15 tests   (53  assertions)
test_parser      21/21 tests   (88  assertions)
test_semantic    22/22 tests   (45  assertions)
─────────────────────────────────────────────
Total            83/83 tests   (265 assertions)   0 failures
```

---

## Architecture

```
src/
  core/
    SourceSpan.hpp       precise source location (line, col, length)
    Result.hpp           Result<T, E> — explicit error handling

  diagnostics/
    DiagnosticKind.hpp   enum of every error type
    Diagnostic.hpp       Diagnostic struct + StageOutput<T>
    DiagnosticEngine     factory: context → structured Diagnostic
    ExplanationBuilder   knowledge base: kind → WHY + FIX + TRACE text
    DiagnosticCollector  accumulate + render (terminal / compact / JSON)

  lexer/
    Token.hpp            TokenType enum + Token struct
    Lexer                single-pass O(n) scanner

  ast/
    ASTNode.hpp          base node + ASTVisitor interface
    Nodes.hpp            all concrete node types
    ASTPrinter           visitor: pretty-print the tree

  parser/
    Parser               recursive descent, panic-mode error recovery

  semantic/
    Type.hpp             Type struct (int, bool, void, unknown)
    SymbolTable          scope-aware stack of hash maps
    SemanticAnalyzer     ASTVisitor: type resolution + symbol checks
```

---

## Commit history

```
fix:  audit and clean up all three stages before IR
feat: Stage 3 — semantic analysis with type resolution
feat: Stage 2 — recursive descent parser + AST + Visitor
feat: first-class explainable diagnostic architecture
feat: Stage 1 — fully working lexer with 18/18 tests
```

---

## Design decisions

| Decision | Why |
|---|---|
| Diagnostics as first-class architecture | Every stage produces structured errors, not strings — enables the visualizer, JSON export, and IDE integration |
| Visitor pattern for AST | Adding a new operation (type checker, IR generator) = one new class, zero changes to AST nodes |
| Recursive descent parser | Hand-written, readable, matches what GCC and Clang use |
| `StageOutput<T>` return type | Every stage returns output + diagnostics together — pipeline never stops at first error |
| Scope stack for symbol table | O(1) scope entry/exit, correct shadowing, matches GCC's design |
| `Unknown` error-recovery type | Prevents cascading errors when a variable is undeclared |
