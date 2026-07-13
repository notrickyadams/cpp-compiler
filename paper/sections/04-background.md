# §4 Background — draft v1

## 4.1 Vocabulary

Four terms carry precise meanings in this paper. A **diagnostic** is
a structured value describing one user-source mistake: kind,
severity, source span, message, root cause, explanation, ordered
fixes, optional invalid/valid examples, and a trace. A **trace** is
the sequence of compiler activities that led to a diagnostic's
creation; this paper distinguishes *recorded* traces (snapshots of
the frames actually open at creation time) from *curated* ones
(hand-written reference chains — the fallback, §6.3). **Provenance**
is the backward link from a compiled artifact to its origin: which
source line an IR instruction or assembly block derives from, and
which optimization decisions transformed it. **Explanation** is the
umbrella for all of the above — preserved compiler evidence, not
machine-learning explainability and not post-hoc rationalization.

## 4.2 The source language

The vehicle is a C-like kernel language — deliberately not "a C++
subset" in any meaningful sense, though its programs are valid C.
The complete grammar (verbatim from the parser's specification
comment):

```
program       → function_decl*  EOF
function_decl → type IDENT '(' param_list ')' block
param_list    → (type IDENT (',' type IDENT)*)  |  ε
block         → '{' statement* '}'
statement     → var_decl | return_stmt
var_decl      → type IDENT ('=' expression)? ';'
return_stmt   → 'return' expression? ';'
expression    → assignment
assignment    → IDENTIFIER '=' assignment | equality
equality      → comparison (('=='|'!=') comparison)*
comparison    → addition   (('<'|'>') addition)*
addition      → multiply   (('+'|'-') multiply)*
multiply      → unary      (('*'|'/') unary)*
unary         → primary
primary       → INTEGER_LITERAL | IDENTIFIER | '(' expression ')'
type          → 'int'
```

Table T1 (feature inventory): **supported** — `int` variables with
optional initializers; arithmetic and comparison operators with C
precedence; assignment as a right-associative expression
(`a = b = c`); multi-parameter functions; multi-error reporting with
panic-mode recovery. **Absent by design** — control flow, function
calls, strings, arrays, pointers, unary operators, additional types.
Internally the type lattice contains `int`/`bool`/`unknown` (`bool`
arises only as a comparison result; `unknown` is the error-recovery
type that suppresses cascading type errors) and `void` (reserved).

The smallness is the method, not a concession: every architectural
mechanism must be visible whole, every diagnostic kind enumerable
(seventeen, of which five are explicitly reserved-unreachable), and
every claim checkable against a codebase a reviewer can actually
read (~5.9K lines, §8).

## 4.3 The pipeline

Compilation proceeds through eight stages: lexing (single-pass,
maximal-munch), recursive-descent parsing to an AST whose nodes are
pure data traversed by four independent visitors, semantic analysis
(scope-stack symbol table; type resolution annotating the AST),
lowering to a flat three-address IR, three optimization passes
(constant folding, copy propagation, dead-code elimination) run
together to a fixed point, code generation to 32-bit x86 assembly
text, and an OS-facing driver that assembles and links a real
executable via the system toolchain. The construction follows
standard practice throughout [29]; nothing in the
pipeline itself is claimed as a contribution. What §6 adds is the
explanation obligation threaded through every one of these stages —
and §9 the bill for it.

<!-- Working notes: grammar copied verbatim from Parser.hpp (checked
     this session); kind counts from DiagnosticKind.hpp (17/5
     reserved); LOC from session-B measurement (5,943). Textbook
     citation pending (registry debt). T1 rendered inline as prose
     inventory; promote to a floated table at assembly if space
     allows. -->
