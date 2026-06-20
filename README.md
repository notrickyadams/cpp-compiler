# cpp-compiler

A C++ compiler built from scratch — lexer → parser → AST → semantic analysis → IR → optimization → assembly.

Built incrementally with production-quality engineering: SOLID architecture, unit tests, error diagnostics, and source-location tracking at every stage.

---

## Current stage: Lexer (Stage 1 of 8)

### Required output

```
Input:   return x + 5;
Output:  RETURN  IDENTIFIER(x)  PLUS  INTEGER_LITERAL(5)  SEMICOLON  EOF
```

### Target program

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
# Build compiler driver
g++ -std=c++14 -Wall -Wextra -Isrc -o build/compiler src/main.cpp src/lexer/Lexer.cpp

# Build + run tests
g++ -std=c++14 -Wall -Wextra -Isrc -Itests -o build/test_lexer tests/test_lexer.cpp src/lexer/Lexer.cpp
./build/test_lexer
```

---

## Test results

```
18/18 tests passed  (66 assertions)
```

---

## Architecture

```
src/
  lexer/
    Token.hpp     — TokenType enum, Token struct, keyword table
    Lexer.hpp     — Lexer class interface
    Lexer.cpp     — single-pass O(n) scanner
  main.cpp        — interactive driver / REPL

tests/
  test_runner.hpp — zero-dependency test framework
  test_lexer.cpp  — 18 unit tests
```

### Design decisions

| Decision | Rationale |
|---|---|
| Cursor-based scanner | O(n), readable, easy to extend — no regex or table overhead |
| Errors collected, not thrown | All errors in one pass; matches Clang's `-fmax-errors` model |
| `TokenType` as `enum class` | Prevents silent int conversion; scoped names |
| Keyword table as `unordered_map` | O(1) average lookup; single place to add keywords |
| Source location on every Token | Required for quality error messages in later stages |

---

## Compiler pipeline (roadmap)

| Stage | Status |
|---|---|
| Lexer | **Done** |
| Parser | Next |
| AST | — |
| Semantic Analysis | — |
| IR | — |
| Optimization | — |
| Assembly Generation | — |
| Executable | — |

---

## Industry comparison

| This project | Industry equivalent |
|---|---|
| `Lexer.cpp` cursor-based scanner | `clang/Lex/Lexer.cpp` (~3 000 lines) |
| `TokenType` enum | `clang/Basic/TokenKinds.def` X-macro table |
| `LexError` struct | Clang `DiagnosticsEngine` |
| `keywords()` unordered_map | GCC `gperf`-generated perfect hash table |
