# Figures F2–F5 — diagram sources (fact-anchored)

Each diagram is specified as source (Mermaid) plus a fact ledger
tying every node and edge to code. Rendering to the venue's style
happens at assembly (session F, TikZ or exported SVG); the *content*
is frozen here and any drift between these ledgers and the code is a
bug in one of them.

---

## F2 — pipeline with the diagnostics spine (§6.1)

```mermaid
flowchart LR
  SRC[source text] --> LX[Lexer]
  LX -- "StageOutput&lt;vector&lt;Token&gt;&gt;" --> PS[Parser]
  PS -- "StageOutput&lt;ProgramNode&gt;" --> SA[Semantic Analysis]
  SA --> IR[IR Generation]
  IR --> OPT[Optimizer ×3 to fixed point]
  OPT --> CG[Codegen x86]
  CG --> TC[Toolchain g++]
  TC --> EXE[executable]
  LX -. diagnostics .-> DC[DiagnosticCollector]
  PS -. diagnostics .-> DC
  SA -. "diagnostics (errors + warnings)" .-> DC
  DC --> R1[full text render]
  DC --> R2[compact render]
  DC --> R3[JSON render → visualizer §7]
  IR -. "span+note provenance" .-> CG
```

Fact ledger: stage list and order = main.cpp compileFile/checkFile;
StageOutput on every lexer/parser/semantic edge = Diagnostic.hpp
contract; IR/OPT/CG emit NO diagnostics (drawn without spine edges —
that asymmetry is D2's point, assert-not-Diagnostic); provenance
edge = IRInstruction span/note (§6.4); three renderers = D6;
Toolchain outside the diagnostics spine (Result<T,E>, D2).

Caption draft: *The pipeline with its diagnostics spine drawn as a
first-class channel. Every front-end stage returns output AND
diagnostics (§6.1, D1); the back half emits none by design (D2 —
invariant violations are assertions, environment failures are
Result values in the Toolchain); provenance rides the IR into
codegen (§6.4).*

---

## F3 — the five-role diagnostic core (§6.2)

```mermaid
flowchart TB
  K[DiagnosticKind taxonomy<br/>one enum per root cause<br/>17 kinds, 5 reserved] --> E
  KB[ExplanationBuilder<br/>knowledge base: rootCause,<br/>explanation, fixes, curated chains] --> E
  TR[TraceRecorder per stage<br/>RAII scopes, lazy details] -- "snapshot at creation" --> E
  E[DiagnosticEngine factory<br/>16 methods: runtime context → Diagnostic] --> D[Diagnostic value<br/>kind severity span message rootCause<br/>explanation fixes examples trace]
  D --> C[DiagnosticCollector]
  C --> O1[full text]
  C --> O2[compact]
  C --> O3[JSON]
  ST[StageOutput&lt;T&gt; transport] -.-> D
```

Fact ledger: role names/files = §6.2; 17/5 = DiagnosticKind.hpp;
16 methods = grep count (§6 D5); snapshot-at-creation + fallback
arrow (KB→E) = D7/D8; lazy details = D14.

Caption draft: *Five roles, one value. The taxonomy indexes the
knowledge base; the factory assembles runtime context, knowledge-
base prose, and the recorder's execution snapshot into one
Diagnostic; three renderers consume it unchanged (D6). The KB→
factory edge also carries the curated fallback chains — unreachable
in the pipeline (D8) but present for direct engine use.*

---

## F4 — recording a trace (sequence, §6.3)

```mermaid
sequenceDiagram
  participant P as Parser
  participant R as TraceRecorder
  participant En as DiagnosticEngine
  P->>R: push parse()
  P->>R: push parseFunctionDecl() [starting at line 1]
  P->>R: push parseBlock()
  P->>R: push parseStatement() [at 'return' (line 3)]
  P->>R: push parseReturnStmt() [at 'return' (line 3)]
  Note over P: detects "return x 2;" (isExpressionStart)
  P->>En: malformedExpression(...)
  En->>R: snapshot()
  R-->>En: 5 open frames, deepest marked failing
  En-->>P: Diagnostic{..., trace = frames + factory step}
  Note over P,R: scopes pop on return (RAII) — frames never read after close
```

Fact ledger: frame names and bracketed details are the VERBATIM
recorded output of the F1 program (captured session B, full build);
"5 open frames" matches; factory-step append = D8; the closing note
is D14's lifetime argument. Regenerate alongside F1 at submission.

Caption draft: *Recording the F1 diagnostic's trace. Each stage
method opens an RAII scope; the engine snapshots the frames open at
creation time — details (function name, current token, line) are
stored as pointers/values at entry and formatted only here (D14).*

---

## F5 — provenance through optimization, and where it dies (§6.4)

```
source (line 2):  return (2 + 3) * 4;

IR before                     after ConstantFolding      after CopyPropagation      after DCE
[2] t0 = 2 + 3            →   [2] t0 = 5                 [2] t0 = 5                 (removed — dead,
                              ; folded from 't0 = 2+3'   ; folded from 't0 = 2+3'    note dies with it)
[2] t1 = t0 * 4           →   [2] t1 = t0 * 4        →   [2] t1 = 5 * 4         →   [2] t1 = 20 †
                                                         ; t0 -> 5 (CopyProp)       ; (folded †, prior note kept)
[2] return t1                 [2] return t1              [2] return t1          →   [2] return 20
                                                                                    ; t1 -> 20 (CopyProp)
```

Fact ledger: [2] = span.startLine stamped at lowering and preserved
by every rewrite (D10, quoted listing); note texts are the exact
appendNote strings (ConstantFoldingPass/CopyPropagationPass); the
final surviving history shows ONLY propagation — the folded t0's
note was removed WITH t0 by DCE (the † instruction's own fold note
survives because t1 itself survives one more round). The
non-transitivity finding (§6.4, §12 item 3) is the red annotation at
assembly. Iteration structure = the fixed-point walkthrough
validated by test_optimizer's pipeline test; regenerate the exact
final-state notes from `--json` irDetail at submission (a live probe
during drafting matched this ledger for the surviving instructions).

Caption draft: *Provenance under optimization for (2+3)*4. Source
line [2] survives every rewrite; transformation notes accumulate —
until dead-code elimination removes an instruction, taking its
history with it. Per-instruction provenance is honest but not
transitive (§12, item 3).*
