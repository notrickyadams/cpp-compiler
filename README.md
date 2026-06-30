# cpp-compiler

A C++ compiler built from scratch!! not just a translator, but a **compiler designed to explain itself**.

Every error includes: where it happened, why it's a problem, how to fix it, and the compiler's internal reasoning chain. Built stage by stage with production-quality architecture, unit tests, and clean separation of concerns.

---

## Pipeline status

| Stage | Status | What it does |
|---|---|---|
| Lexer | Done | Raw source text -> typed token stream |
| Diagnostic System | Done | Structured errors with WHY / FIX / TRACE, built in from day one |
| Parser + AST | Done | Tokens -> Abstract Syntax Tree with full operator precedence |
| Semantic Analysis | Done | Type checking, scope resolution, symbol table |
| IR Generation | Done | AST  -> flat three-address intermediate representation |
| Optimization | Done | Constant folding, copy propagation, dead code elimination  -> run to a fixed point |
| Assembly Generation | Done | IR  -> real 32-bit x86 AT&T assembly; tests assemble/link/run the output and check process exit codes |
| Executable | Done | `./compiler input.cpp -o out.exe` builds a real, runnable binary via the system assembler/linker |
| Visualizer | Done | `./compiler input.cpp --json` + a small local web UI: source → tokens → AST → IR → assembly |

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

**Requires:** g++ with C++14 support (GCC 6+). The codegen stage additionally
assumes a 32-bit x86 MinGW target, see [Design decisions](#design-decisions).

```bash
mingw32-make all         # builds build/compiler + every test binary
mingw32-make run-tests   # runs all nine test suites
./build/compiler
```

Or, to see exactly what's compiled, the equivalent raw command:

```bash
g++ -std=c++14 -Wall -Wextra -Wpedantic -Isrc \
    -o build/compiler \
    src/main.cpp \
    src/diagnostics/ExplanationBuilder.cpp \
    src/diagnostics/DiagnosticEngine.cpp \
    src/diagnostics/DiagnosticCollector.cpp \
    src/lexer/Lexer.cpp \
    src/ast/ASTPrinter.cpp \
    src/ast/ASTJsonPrinter.cpp \
    src/parser/Parser.cpp \
    src/semantic/SemanticAnalyzer.cpp \
    src/ir/IRGenerator.cpp \
    src/optimizer/ConstantFoldingPass.cpp \
    src/optimizer/CopyPropagationPass.cpp \
    src/optimizer/DeadCodeEliminationPass.cpp \
    src/optimizer/Optimizer.cpp \
    src/codegen/AssemblyGenerator.cpp \
    src/driver/Toolchain.cpp

./build/compiler
```

### Compiling a real program

With no arguments, `./build/compiler` runs the demo suite shown above, then
drops into a REPL. Pass a source file instead and it behaves like a real
compiler, quiet on success, one fully-explained diagnostic report on failure:

```bash
./build/compiler build/sample.cpp -o build/sample.exe
./build/sample.exe; echo $?    # -> 7
```

### Visualizer

`--json` instead of `-o` dumps the entire pipeline (tokens, AST, semantic
log, IR before/after optimization, optimization report, assembly,
diagnostics) as one JSON object, that's what feeds the web UI:

```bash
python visualizer/server.py     # stdlib only, no pip install
# open http://127.0.0.1:8000 — type source, click Compile
```

The server (stdlib `http.server` + `subprocess`, zero dependencies, same
philosophy as the C++ test framework) shells out to `build/compiler.exe
<tempfile> --json` per request and returns the result as-is.

---

## Test results

```
test_lexer       25/25 tests   (79  assertions)
test_diagnostics 15/15 tests   (53  assertions)
test_parser      21/21 tests   (88  assertions)
test_semantic    22/22 tests   (45  assertions)
test_ir          15/15 tests   (49  assertions)
test_optimizer   15/15 tests   (31  assertions)
test_codegen     17/17 tests   (32  assertions)
test_driver       4/4  tests   (9   assertions)
test_visualizer   9/9  tests   (29  assertions)
─────────────────────────────────────────────
Total           158/158 tests  (415 assertions)   0 failures
```

`test_codegen` and `test_driver` shell out to the real toolchain: tests
assemble and link generated `.s` text with `g++`, run the resulting `.exe`,
and assert on its process exit code. `test_visualizer` does the same for the
`--json` CLI mode, it runs `build/compiler.exe <file> --json` as a real
subprocess and checks the output, the same empirical standard rather than
trusting the JSON-building code in isolation.

---

## Architecture

```
src/
  core/
    SourceSpan.hpp       precise source location (line, col, length)
    Result.hpp           Result<T, E>, explicit error handling
    Json.hpp             jsonEscape/jsonArray/jsonStringArray, shared by
                         every stage that feeds the visualizer's JSON export

  diagnostics/
    DiagnosticKind.hpp   enum of every error type
    Diagnostic.hpp       Diagnostic struct + StageOutput<T>
    DiagnosticEngine     factory: context  -> structured Diagnostic
    ExplanationBuilder   knowledge base: kind → WHY + FIX + TRACE text
    DiagnosticCollector  accumulate + render (terminal / compact / JSON)

  lexer/
    Token.hpp            TokenType enum + Token struct + toJson()
    Lexer                single-pass O(n) scanner

  ast/
    ASTNode.hpp          base node + ASTVisitor interface
    Nodes.hpp            all concrete node types
    ASTPrinter           visitor: pretty-print the tree
    ASTJsonPrinter       visitor: serialize the tree to JSON for the
                         visualizer's collapsible tree view

  parser/
    Parser               recursive descent, panic-mode error recovery

  semantic/
    Type.hpp             Type struct (int, bool, void, unknown)
    SymbolTable          scope-aware stack of hash maps
    SemanticAnalyzer     ASTVisitor: type resolution + symbol checks

  ir/
    IRValue.hpp          operand: Temp | Var | Const
    IRInstruction.hpp    one flat instruction: dest = src1 OP src2
    IRFunction.hpp       instruction list per function + IRProgram
    IRGenerator          ASTVisitor: lowers annotated AST to IR

  optimizer/
    OptimizationPass.hpp     Strategy interface: run(fn) -> changed
    ConstantFoldingPass      CONST op CONST -> CONST
    CopyPropagationPass      temp with known value -> substitute at use
    DeadCodeEliminationPass  remove instructions defining unused temps
    Optimizer                runs all passes to a fixed point

  codegen/
    StackFrameLayout.hpp     assigns every Var/Temp a stack slot; resolves
                             params to caller-set cdecl offsets
    AssemblyProgram.hpp      flat vector<string> of emitted assembly lines
    AssemblyGenerator        lowers IR to 32-bit x86 AT&T assembly text

  driver/
    Toolchain                writes the .s, shells out to g++ to assemble +
                             link a real executable, cleans up afterward

visualizer/
  server.py             stdlib-only local server: serves the frontend,
                         shells out to compiler.exe --json for /compile
  index.html, style.css,
  app.js                 plain JS, no framework, no build step
```

---

## Commit history

```
feat: Stage 8 — JSON export + web visualizer
feat: Stage 7 — executable generation via system toolchain
feat: Stage 6 — x86 assembly generation, stack frame layout
feat: Stage 5 — constant folding, copy propagation, DCE
feat: Stage 4 — three-address code IR generation
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
| Diagnostics as first-class architecture | Every stage produces structured errors, not strings, enables the visualizer, JSON export, and IDE integration |
| Visitor pattern for AST | Adding a new operation (type checker, IR generator) = one new class, zero changes to AST nodes |
| Recursive descent parser | Hand-written, readable, matches what GCC and Clang use |
| `StageOutput<T>` return type | Every stage returns output + diagnostics together, pipeline never stops at first error |
| Scope stack for symbol table | O(1) scope entry/exit, correct shadowing, matches GCC's design |
| `Unknown` error-recovery type | Prevents cascading errors when a variable is undeclared |
| IR creates a temp only for operators | Constants/variables used directly as operands — keeps IR minimal so the optimizer's before/after story is honest |
| No diagnostics in IR generation | This stage trusts semantic analysis already validated the program — internal invariant violations use `assert()`, not `Diagnostic` |
| Optimizer passes run to a fixed point, not once | `(2+3)*4` needs 3 iterations — folding the inner expr creates a NEW foldable expr that didn't exist before. Verified by test and hand-traced in commit history |
| Copy propagation never touches named variables | A temp has one static definition by construction (safe to inline); a variable could be reassigned by a future language feature (unsafe without liveness analysis) |
| No structured `AssemblyInstruction` type | Nothing downstream transforms assembly text — an external assembler just consumes it. `vector<string>` is exactly as much structure as that job needs |
| `_main` symbol + `call ___main` for the entry point only | Empirically discovered, not assumed: this legacy MinGW.org CRT's startup expects a symbol literally `_main`; without it, linking fails with `undefined reference to WinMain@16` because the CRT falls back to looking for a GUI entry point. Validated against real `gcc -S` output before writing the generator — see `build/probe*.c/.s` |
| Every function gets a safety-net `leave`/`ret` even with no IR `Return` | The grammar allows a body that never returns (e.g. `int f() { int x = 5; }`), and nothing upstream rejects it — C treats this as UB, not a hard error. Real compilers still emit a valid epilogue rather than falling through into the next function's bytes; this generator does the same |
| Comparison and call-expression codegen tested via direct IR construction | The front end has no implicit bool→int conversion and no call-expression grammar at all, so source text can't express either case end-to-end. Tests build the `IRFunction` by hand instead — the same pattern `test_optimizer.cpp` already uses for pass-level unit tests |
| `Toolchain` lives in `driver/`, not `codegen/` | `AssemblyGenerator` is pure (IR in, text out — no filesystem, no subprocess, trivially unit-testable). `Toolchain` owns 100% of the "talk to the OS" responsibility instead, the same split Clang draws between `CodeGen` and `driver::Toolchain` |
| `Toolchain::buildExecutable` returns `Result<T,E>`, not `Diagnostic` | A failure here (g++ missing, disk full) is an environment failure, not a user source error — it has no `SourceSpan` and nothing for `ExplanationBuilder` to say. `Result<T,E>` was built during the Diagnostics stage for exactly this shape but had no caller until Stage 7 |
| `compileFile()` is separate from `compile()`, not a shared helper with a flag | `compile()` narrates every stage for the portfolio demo; `compileFile()` builds quietly like `g++ -o` does. Their output contracts differ enough that unifying them would mean threading print statements through conditionals, not removing real duplication |
| `ASTJsonPrinter` is a 4th Visitor, not a flag on `ASTPrinter` | Same reasoning as every prior Visitor addition: adding an operation should mean one new class, zero changes to `Nodes.hpp`. Four independent Visitors now share the same node definitions untouched since Stage 2 |
| IR/assembly serialize to JSON as one string, not a structured tree | Unlike the AST, nothing about IR or assembly benefits from being a navigable tree in the UI — they're displayed as text panels. Reusing `IRProgram::toString()`/`AssemblyProgram::toString()` avoids inventing structure nothing renders differently |
| The visualizer backend is Python stdlib only (`http.server` + `subprocess`) | No pip install, no Node/npm build step, matching the C++ side's zero-dependency test framework. It shells out to the already-built `compiler.exe --json` rather than re-implementing the pipeline in Python |
| Visualizer subprocess calls use `shell=False` (Python's default) | Sidesteps a real bug hit while testing: `cmd.exe` (which C++'s `system()` shells out to) parses `"build/x.exe"` as the unrelated MASM32 `build.bat` on this machine's PATH plus a flag-shaped argument. `subprocess.run([...])` without `shell=True` calls `CreateProcess` directly, bypassing `cmd.exe`'s parsing entirely |

---

## Before / after optimization

```cpp
int main() { return (2 + 3) * 4; }
```

| Before | After | Iterations |
|---|---|---|
| `t0 = 2 + 3`<br>`t1 = t0 * 4`<br>`return t1` | `return 20` | 3 |

The middle iteration is the interesting one: folding `2+3` into `5` doesn't immediately make `t1 = t0 * 4` foldable — copy propagation has to substitute `t0 → 5` first, which *creates a new* `5 * 4` opportunity that constant folding then catches on the next round. A single linear pass would have stopped at `t1 = 5 * 4` and missed it.

---

## Generated assembly

The target program, end to end — this is real output from `./build/compiler`,
assembled, linked, and run with `g++` to confirm it actually works (exit code 7):

```cpp
int main() {
    int x = 5;
    return x + 2;
}
```

```asm
    .globl  _main
_main:
    pushl   %ebp
    movl    %esp, %ebp
    andl    $-16, %esp
    subl    $8, %esp
    call    ___main
    # x = 5
    movl    $5, -4(%ebp)
    # t0 = x + 2
    movl    -4(%ebp), %eax
    addl    $2, %eax
    movl    %eax, -8(%ebp)
    # return t0
    movl    -8(%ebp), %eax
    leave
    ret
```

`x` and `t0` each get one 4-byte stack slot (`-4(%ebp)`, `-8(%ebp)`) from
`StackFrameLayout`. The `andl`/`call ___main` pair only appears on `main` — every
other function gets a plain symbol and skips both (see Design decisions).
