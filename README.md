# cpp-compiler

I wanted to actually understand how a compiler works, not the "read a blog post about lexers" kind of understanding, the "write one from scratch and watch it break in weird ways" kind. So this is a small C++ subset compiler, built one stage at a time: lexer → parser → semantic checks → IR → optimizer → real x86 assembly → a linked, runnable executable.

It's deliberately small. `int` is the only type you can declare, there's no control flow, no function calls, no arrays or pointers. The point was depth, not coverage. I'd rather have a tiny language with a compiler that actually understands itself than a big one that's just a translator.

That second part is the actual point of the project. Every error carries its location, a root cause, a plain-English explanation of what happened, a couple of concrete fixes, and the real internal call chain that produced it: not a canned message, the actual `Parser::method()` → `DiagnosticEngine::thing()` path the compiler walked to get there. That part ended up being most of the work.

## what it actually understands

- `int` variables, with or without an initializer
- arithmetic (`+ - * /`) and comparisons (`< > == !=`)
- assignment as an expression: `return x = 3;` and `a = b = c` both work, right-associative, same as real C++
- functions with parameters, one per declaration

That's the whole language. No `if`, no loops, no strings (the lexer doesn't even tokenize `"`, it just rejects it as an unknown character), and functions can't call each other yet (`test_codegen.cpp` has a test literally named `"known gap: call expressions are not yet parseable"`). That's an honest label, not a TODO I forgot to delete.

## what an error actually looks like

This is real output, not a mockup:

```
ERROR TYPE:
Malformed Expression

LOCATION:
line 3, column 14

    3 |     return x 2;
      |              ^

ROOT CAUSE:
Two expressions appeared consecutively with no operator between them.

WHAT HAPPENED:
The parser successfully read:

return x

After completing the return expression,
it encountered another value:

2

C++ requires an operator between values.

INVALID:

x 2

VALID:

x + 2
x * 2
x = 2

HOW TO FIX:
Insert an operator between x and 2
or remove the extra value

TRACE:
Parser
→ Parser::parseReturnStmt()
→ Parser::isExpressionStart()
→ DiagnosticEngine::malformedExpression()
```

Every diagnostic in the compiler goes through the same renderer. This isn't a special case for one error type, it's just what errors look like here.

## building it

You need g++ with C++14 support. I'm targeting 32-bit x86 MinGW (GCC 6.3.0) specifically for codegen: the calling convention, stack alignment, and CRT startup sequence were worked out empirically against this exact toolchain, not copied from a spec. The front end (lexer through optimizer) doesn't care what platform you're on; only assembly generation and the real executable build do.

```bash
mingw32-make all         # build/compiler + every test binary
mingw32-make run-tests   # all nine suites
```

Compile something for real:

```bash
./build/compiler myprogram.cpp -o myprogram.exe
./myprogram.exe; echo $?
```

Or dump the whole pipeline as JSON. This is what feeds the visualizer:

```bash
./build/compiler myprogram.cpp --json
```

### the visualizer

```bash
python visualizer/server.py
# http://127.0.0.1:8000, type some source, click Compile
```

Stdlib-only Python (`http.server` + `subprocess`, no pip install) shelling out to the already-built `compiler.exe --json` per request. Tabs for tokens, AST, semantic log, IR before/after optimization, and final assembly.

## tests

```
test_lexer        31/31   (91  assertions)
test_diagnostics  21/21   (81  assertions)
test_parser       38/38   (145 assertions)
test_semantic     36/36   (83  assertions)
test_ir           20/20   (67  assertions)
test_optimizer    21/21   (48  assertions)
test_codegen      21/21   (39  assertions)
test_driver        4/4    (9   assertions)
test_visualizer   11/11   (35  assertions)
                 ───────────────────────
                 203/203  (598 assertions)   0 failures
```

`test_codegen`, `test_driver`, and `test_visualizer` don't mock anything: they shell out to real `g++`, assemble and link actual `.s` files, run the resulting `.exe`, and check its process exit code. If generated assembly is wrong, these fail because a real binary does the wrong thing, not because some abstraction disagreed with itself.

## layout

```
src/
  core/
    SourceSpan.hpp       line/col/length for one chunk of source
    Result.hpp           Result<T, E> for environment failures (g++ missing,
                          disk full), distinct from Diagnostic, which is for
                          mistakes in the user's source
    Json.hpp             hand-rolled jsonEscape/jsonArray, shared by every
                          stage that feeds the visualizer

  diagnostics/
    DiagnosticKind.hpp   every distinct error type, one enum value per root
                          cause (not per message)
    Diagnostic.hpp       the Diagnostic struct + StageOutput<T>
    DiagnosticEngine     factory: runtime context in, structured Diagnostic out
    ExplanationBuilder   the actual knowledge base: root cause / explanation
                          / fixes / trace text for every kind, in one place
    DiagnosticCollector  accumulates diagnostics, renders them three ways
                          (full text, compact one-liner, JSON)

  lexer/    Token.hpp + Lexer, single-pass scanner
  ast/      Nodes.hpp (pure data, no behaviour) + four Visitors:
            ASTPrinter, ASTJsonPrinter, SemanticAnalyzer, IRGenerator
  parser/   Parser, recursive descent, panic-mode recovery
  semantic/ Type.hpp, SymbolTable, SemanticAnalyzer
  ir/       IRValue/IRInstruction/IRFunction, flat three-address IR
  optimizer/ constant folding, copy propagation, dead code elim,
             run together to a fixed point
  codegen/  StackFrameLayout + AssemblyGenerator, IR to real x86 AT&T text
  driver/   Toolchain, the only thing that talks to the OS (shells out to
            g++ to assemble/link, cleans up after itself)

visualizer/
  server.py              stdlib-only, serves the frontend + shells out to
                          compiler.exe --json
  index.html/style.css/app.js   plain JS, no framework, no build step
```

## notes to future me

A handful of things that actually went wrong, in case I (or anyone else reading this) hits them again:

- **Assignment used to just not parse.** `return x = 3;` failed outright because the grammar jumped straight from `expression` to `equality`, skipping the precedence level `=` actually lives at. The failure was worse than a clean rejection, too: it cascaded into two unrelated-looking errors for one mistake, because `Parser::expect()` doesn't consume the token it fails on, so the next statement's parse attempt re-tripped on the same leftover `=`. The fix was adding the missing grammar rule properly, not patching around the symptom.

- **MinGW's CRT wants a function named `_main`, not `main`.** Found this by getting `undefined reference to WinMain@16` with zero indication that the real problem was the entry symbol name. The actual fix (`call ___main` right after the prologue, only on the function literally named `main`) came from diffing against real `gcc -S` output on a throwaway probe file, not from reading documentation that doesn't really exist for this anymore.

- **Running a built `.exe` via `system("build/foo.exe")` would just hang**, with a bizarre phantom warning about an invalid MASM command-line option. Took embarrassingly long to realize some unrelated MASM32 SDK sitting on my PATH has its own `build` command, and `cmd.exe` matches that ahead of the literal path I gave it. Backslash paths sidestep it. The Python visualizer server never hits this at all, since `subprocess.run([...])` calls `CreateProcess` directly and skips `cmd.exe`'s parsing entirely.

- **The optimizer has to run to a fixed point, not once.** `(2+3)*4` needs three passes: fold `2+3` into `5`, propagate that `5` into `5*4`, then fold *that*. A single linear pass stops after step one and leaves `t1 = 5 * 4` sitting there unfolded. It looks finished, it isn't.

- **Copy propagation only ever substitutes temps, never named variables.** A temp has exactly one definition by construction, so inlining its value anywhere it's used is always safe. A variable could be reassigned somewhere downstream, and inlining it would need real liveness analysis I haven't written, so it just doesn't touch them.

- **The TRACE section used to be a hand-written lookup, not a recording.** Early on, each error kind had a curated "here's how the compiler gets here" chain written by hand in the knowledge base. It looked dynamic; it wasn't — it could never tell you *which* function was being analysed or *which* token was being read, because that only exists at runtime. Now each stage owns a `TraceRecorder` and its methods open RAII scopes, so the trace attached to a diagnostic is a snapshot of the frames actually open when the error fired — `visit(FunctionDeclNode&) [function 'main']`, `parseStatement() [at 'return' (line 3)]`. The curated chains still exist as the fallback when an engine is used directly with no recorder (unit tests do this), and their final step is appended to recorded chains so every trace ends at the factory method that created the diagnostic.

- **A bare `return;` inside a non-void function used to compile clean.** The semantic analyzer had already noticed: there was a log line that said `FAIL: bare return in non-void function`, but nothing ever turned that into an actual diagnostic. So the pipeline kept going, and codegen happily emitted `leave; ret` without ever loading anything into `%eax`. The program would "successfully" exit with whatever garbage happened to already be sitting in that register. I only found it by writing twenty adversarial test programs by hand and running them through the real CLI instead of trusting the existing unit tests to have caught everything.

## before / after optimization

```cpp
int main() { return (2 + 3) * 4; }
```

| before | after | iterations |
|---|---|---|
| `t0 = 2 + 3`<br>`t1 = t0 * 4`<br>`return t1` | `return 20` | 3 |

The middle pass is the interesting one. Folding `2+3` into `5` doesn't make `t1 = t0 * 4` foldable by itself. Copy propagation has to substitute `t0 → 5` first, which creates a *new* `5 * 4` that didn't exist a moment ago, and constant folding catches that on the next round. Stop after one linear pass and you'd miss it.

## generated assembly

This is real output, assembled, linked, and run to confirm it actually exits with 7:

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
    # line 2: x = 5
    movl    $5, -4(%ebp)
    # line 3: t0 = x + 2
    movl    -4(%ebp), %eax
    addl    $2, %eax
    movl    %eax, -8(%ebp)
    # line 3: return t0
    movl    -8(%ebp), %eax
    leave
    ret
```

The `# line N:` prefixes are source provenance: every IR instruction carries the span of the AST node it was lowered from, the optimizer passes preserve it (and append a note describing any rewrite they perform), and codegen prints it above each instruction block — the same idea as the `.loc` directives real compilers emit for debuggers, in a form you can read.

`x` and `t0` each get their own 4-byte stack slot. The `andl`/`call ___main` pair only shows up on `main`; every other function gets a plain symbol and skips both.
