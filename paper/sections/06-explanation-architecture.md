# §6 The Explanation Architecture — draft v2.1

<!-- v1: initial D-block draft. v2: revisions R1-R6 from peer-review
     pass. v2.1: D14 (lazy trace details) added after E1 refuted H3
     for the eager design; D7's cost discussion updated accordingly.
     v3 planned: final numbers from §9 wired into D7/D14; ⚠ items. -->

The architecture rests on a single obligation: *no stage may discard
the evidence a future explanation would need*. This section presents
the four mechanism groups that discharge that obligation — the stage
contract (§6.1), the diagnostic core (§6.2), recorded traces (§6.3),
and cross-stage provenance (§6.4) — each as an engineering decision
weighed against at least one rejected alternative.

## 6.1 Stage contract and the taxonomy of failure

Every pipeline stage returns `StageOutput<T>`: its product *and* a
list of structured diagnostics, always both. A stage that finds
errors still returns its partial product; the driver, not the stage,
decides whether to continue.

**D1 — Partial-success contract vs. fail-fast propagation.** The
conventional alternatives are exceptions or an `Either`-style
short-circuit (`Result<T,E>` end to end). We rejected both for the
*pipeline* path because explanation quality depends on surviving the
first error: a fail-fast parser cannot report the second, independent
mistake, and — more damaging for this architecture — it discards the
partially built AST that later explanation surfaces (the visualizer
deliberately renders the recovered-but-erroneous tree, because
*where the parser got confused* is itself explanatory). The cost of
the choice is real: every stage must define what "partial product"
means (the parser synthesizes placeholder nodes; the analyzer types
what it can and poisons the rest with `unknown`), and downstream code
must tolerate it. Production compilers pay the same cost for the same
reason — multi-error reporting has been standard since panic-mode
recovery; see [6] for the modern treatment and its quantified
failure mode, cascading. Evidence: `StageOutput<T>` in
`src/diagnostics/Diagnostic.hpp`; the parser's recovery paths and the
cascade-freedom tests they carry.

**D2 — Three failure currencies vs. one.** A uniform "everything is
a `Diagnostic`" design was rejected because it conflates three
audiences. User-source mistakes become `Diagnostic` (they have a
`SourceSpan`; there is something to explain). Environment failures —
assembler missing, disk full — become `Result<T,E>`
(`src/driver/Toolchain.cpp`): they have no span, and routing them
through the explanation machinery would manufacture explanations for
facts about the machine, not the program. Violated *compiler
invariants* are `assert`s (`src/ir/IRGenerator.cpp`): a bug in us,
not in the user, and dressing it as a user-facing diagnostic would be
a category error with a friendly font. The cost: three idioms to
learn, and the boundary must be policed in review. The split mirrors
LLVM's separation of recoverable errors ("an error in the program's
environment... a missing file", handled via `Expected<T>`/`Error`)
from its diagnostic subsystem [7]. Our own history enforces the boundary's
value: the one failure that *was* misfiled (missing-return, logged
internally as neither diagnostic nor error) produced the
silent-garbage defect that motivates §3.

## 6.2 The five-role diagnostic core

A diagnostic here is one struct carrying kind, severity, span,
message, root cause, explanation, ordered fixes, trace, and optional
invalid/valid example pairs. Five roles cooperate to build and
deliver it: a **kind taxonomy**, a **factory** (`DiagnosticEngine`),
a **knowledge base** (`ExplanationBuilder`), a **collector** with
three renderers, and the stage contract of §6.1 as transport.

**D3 — One kind per root cause vs. one code per message.**
`DiagnosticKind` allocates an enumerator per *distinct underlying
cause*, not per output string — `PARSE_MalformedExpression` covers
both `return x 2;` and `int x = 5 5;` because the mistake (two
expressions, no operator) is one cause with two surface forms. The
granularity rule is operational, not aesthetic: two failures share a
kind **iff** they share a root-cause sentence and a fix set — the
knowledge base's own index structure decides. By contrast, a bare
`return;` in a non-void function and a *missing* return entirely get
distinct kinds (`SEM_ReturnTypeMismatch`, `SEM_MissingReturn`)
despite superficial kinship, because their causes, severities, and
fixes all differ — one is an ill-formed statement (error), the other
a missing obligation (warning, GCC's `-Wreturn-type` choice). The
rejected alternative, message-granularity codes, is the shape
string-based systems drift into; GHC's and Rust's retrofits both
converged on cause-indexed identifiers with public indexes
[8, 4, 3]. Benefit: the
taxonomy *is* the explanation index. Cost, honestly carried in the
header: taxonomy discipline requires admitting unreachable entries;
five kinds are marked `[reserved]` in
`src/diagnostics/DiagnosticKind.hpp` because the language cannot yet
produce them, and tests are forbidden from expecting them.

**D4 — Centralized prose knowledge base vs. prose at the emit site
vs. external message DSL.** All user-facing explanation text lives in
one component, `ExplanationBuilder`, switch-indexed by kind and
parameterized by runtime detail. Emit-site prose (each stage writes
its own sentences) was rejected because it makes consistency
unenforceable and translation impossible — the exact pathology GHC
documents in its `SDoc`-era retrofit discussion [9, 4]. The heavier
alternative — an external message DSL (Clang's TableGen `.td` files
[27]; rustc's Fluent [10]) — was rejected *at this
scale*: it buys tooling and i18n at the price of a build step
and an indirection layer, for seventeen kinds. The cost of our middle
position surfaced measurably: the builder's `detail`/`detail2`
interface hit its capacity limit the first time a kind needed three
context values, and the factory works around it by re-deriving one
field (`src/diagnostics/DiagnosticEngine.cpp`, documented at the
`malformedExpression` site). We report that as evidence about where
this design stops scaling; the industrial DSLs sit exactly where our
workaround points (see Discussion, §10).

**D5 — Factory/knowledge-base split vs. a merged emitter.**
`DiagnosticEngine` (runtime context in, complete struct out) is
separate from `ExplanationBuilder` (static text per kind). A merged
component would be smaller; the split was chosen because the two
change for different reasons — one when *detection* context changes,
one when *wording* changes — and because the knowledge base must stay
stateless and stage-independent for the fallback semantics of §6.3 to
be sound. Cost: one more hop to read, and a factory method per kind
(mechanical boilerplate; sixteen methods at the time of writing).

**D6 — Late rendering, three renderers vs. render-at-creation.**
Diagnostics are stored as data and rendered on demand — full
explanatory text, compact one-liners, or JSON — from the same struct
(`src/diagnostics/DiagnosticCollector.cpp`). Rendering at creation
(the string-concatenation norm this paper argues against) was
rejected as the root discard-point: once prose, always prose. The
measured consequence of late rendering is that CLI and visualizer
provably tell one story (they consume the same struct; Fig. F6), and
machine consumers get fields, not regexes — the property GHC's and
Rust's JSON emitters retrofitted at cost [12, 4]. Cost: the struct is the compatibility surface;
adding a field touches all three renderers (the trace `detail`
field's addition did exactly that, in one commit).

## 6.3 Recorded traces

Every diagnostic carries the call path that produced it. The claim
requiring defense is the word *recorded*.

**D7 — RAII scope-stack snapshot vs. curated per-kind chains vs.
full event log.** The predecessor design — and this is committed
history, not a rhetorical foil — was a *curated* chain: a
hand-written, plausible call path per kind, stored in the knowledge
base. It rendered convincingly and was architecturally cheap, but it
could not name the function being analyzed or the token under the
cursor, because those exist only at runtime; the paper's thesis calls
this fabricated provenance. The replacement: each stage owns a
`TraceRecorder`; methods at decision points open RAII scopes carrying
runtime detail ("function 'main'", "at 'return' (line 3)"); when the
factory builds a diagnostic it snapshots the frames *currently open*
(`src/diagnostics/TraceRecorder.hpp`; Fig. F4 traces the sequence
for the Fig. F1 diagnostic). The third option — logging
every event, not just the live stack — was rejected on cost and noise
grounds: an event log grows with program size whether or not an error
ever occurs, while the active-stack snapshot is O(depth) and paid
only at diagnostic creation; the Whyline demonstrates the power of
full traces for *program* debugging and also their price, a dedicated
post-hoc analysis apparatus [11]. Disclosed cost
of our choice: sibling context is invisible (completed frames are
gone from the stack). The entry cost of open frames is the subject of
D14 and §9.2 — the naive version of this design failed its own
overhead experiment. Evidence that recording is real rather than
curated-with-extra-steps: a pre-existing regression test *failed* at
the transition — it had encoded the curated chain's shape
(`trace.front()` = detecting method), and recorded traces begin at
the true outermost frame instead. The failure was the proof; the test
now asserts containment.

**D8 — Retaining the curated chains as fallback vs. deleting them.**
When an engine is used with no recorder attached — unit tests
construct diagnostics directly — the knowledge base's static chain is
substituted, and its final step (the factory method) is appended to
*recorded* chains too, so every trace terminates identically.
Deleting the curated chains would have simplified the truth story at
the cost of either dragging recorder setup into every unit test or
letting traces go empty off-pipeline. We kept the fallback and
disclose it. In the compiled pipeline the fallback is unreachable:
exactly three `DiagnosticEngine` instances exist (one per
diagnostic-emitting stage), and each attaches its recorder before
parsing/analysis begins; IR generation, optimization, and code
generation emit no diagnostics at all. Every diagnostic a user can
ever see therefore carries a recorded trace (verified by exhaustive
search for engine instantiations). The residual risk we accept is
dual-source drift, mitigated by the shared final step and by tests
that assert *recorded* traces contain runtime facts no static chain
could hold.

**D9 — Selective scope placement vs. instrument-everything.**
Placement follows a stated rule, not taste: a scope is opened (i) in
every method that can itself emit a diagnostic, and (ii) at
structural landmarks that name the enclosing construct (function,
block, statement dispatch). Expression-precedence internals satisfy
neither — their failures surface in `parsePrimary`/`expect`, which
are covered by (i). A skeptic may object that selective scoping makes
the recorded trace "curated by another name" — the author still
chooses what is recordable. The objection conflates two different
failure modes. A curated chain can be *wrong*: it asserts a path that
did not execute (and before this transition, ours did — no curated
chain could have named `function 'main'`). A selectively scoped
recording can only be *coarse*: every frame it reports was genuinely
open, verified by tests asserting runtime facts in the output.
Omission bounds resolution; it cannot manufacture provenance. The
placement policy is itself inspectable data (grep `TRACE_SCOPE`), and
refining it is a local edit, not an architectural change.

**D13 — Ablation by macro vs. no-op class (measurement honesty).**
The overhead experiments compare builds with recording compiled out.
A no-op `TraceScope` class would still evaluate its argument
expressions — the detail context built at every call site — and so
would silently understate the mechanism's true cost. Call sites
therefore use a `TRACE_SCOPE(...)` macro that erases its arguments
entirely under `-DCPPC_NO_TRACE`. Cost: a macro where RAII would be
idiomatic; accepted because a measurement that flatters the design is
worse than an ugly line. (The same flag design gives the factorial
attribution used in §9.2.)

**D14 — Lazy detail formatting vs. eager strings (added after E1
refuted H3 for the eager design).** The first recorder built each
frame's detail *string* at scope entry. E1 measured that at ~2.4×
total compile time on 20k-line inputs — the lexer opens two scopes
per token, so a clean compile paid for hundreds of thousands of
string constructions whose text nobody would ever read. The repaired
design stores lazily-formattable POD in each frame — `const char*`
literals, a pointer to a caller-owned string, ints — and materializes
text only in `snapshot()`, i.e. only when a diagnostic fires. Two
correctness arguments carry the design: (1) *pointer stability* —
snapshots only ever occur while frames are open (the engine runs
inside the stage method that owns the scope), so a frame may safely
point at a token's lexeme or an AST node's name, which provably
outlive it; rvalue overloads are deleted so a temporary cannot be
captured. (2) *Entry-time semantics* — ints (line/column) are copied
by value at scope entry because the lexer's counters advance during
the scope, and the frame must describe where the scope was *opened*,
exactly as the eager strings did. One rare path (the parser's
empty-lexeme case) needs text whose construction would couple the
diagnostics layer to the lexer; it pays for an eager string via an
explicit escape hatch rather than couple the layers. Alternatives
rejected: deferred closures (a `std::function` per frame allocates —
the disease as the cure) and fixed-size `snprintf` buffers (kills the
allocation but keeps per-scope formatting work and truncates
unpredictably). The materialized strings are byte-identical to the
eager versions — the trace-content tests pin them, and passed
unchanged across the transition. Measured effect (§9.2, interleaved
protocol): recording cost fell from +143% [130, 168] to 1–2% with
confidence intervals spanning zero at every corpus size — a ~60×
reduction, leaving recording statistically indistinguishable from
free while producing identical output.

## 6.4 Provenance to emitted assembly

**D10 — Inline provenance fields vs. side tables vs. AST
back-pointers.** Every `IRInstruction` carries a `SourceSpan` and a
transformation `note` as ordinary value fields, stamped at lowering
(`src/ir/IRGenerator.cpp`). Back-pointers into the AST were rejected
on lifetime grounds — they would couple IR validity to AST retention,
which no later consumer otherwise needs. A side table (instruction →
span) was rejected on *identity* grounds: optimizer passes rewrite
instructions in place and rebuild instruction vectors (DCE), so any
table keyed by identity or index desynchronizes precisely when
provenance matters most; inline fields travel with the value through
every rewrite for free. Constant folding's rewrite demonstrates the
discipline the choice demands (quoted from
`src/optimizer/ConstantFoldingPass.cpp`):

```cpp
// Provenance: the replacement must keep the original's source span
// (it still comes from the same source expression) and record what
// it used to be — rebuilding via makeCopy() alone would silently
// wipe both.
IRValue     dest   = instr.dest;
SourceSpan  span   = instr.span;
std::string before = instr.toString();
std::string prior  = instr.note;
instr = IRInstruction::makeCopy(dest, IRValue::makeConst(result));
instr.span = span;  instr.note = prior;
instr.appendNote("folded from '" + before + "' (ConstantFolding)");
```

The four saved locals are the cost of inline provenance made visible:
every rewriting pass owes this discipline. A side table would trade
those four lines for an identity-tracking problem across in-place
rewrites and DCE's vector rebuild — a worse debt, but a real
alternative for pass-heavy compilers. Costs of inline fields, both
measured (§9.2): the enlarged instruction makes every copy, move,
and pass traversal pay — a steady 12–17% wall time — while peak
memory stays ≤1.3%; provenance's price is time, not space. And
`toString()` had to stay byte-stable so thirty golden IR tests
survived; provenance display therefore lives in separate channels
(structured JSON, assembly comments) rather than in the canonical
text.

**D11 — Accumulated string notes vs. structured remark records.**
Passes append human-readable notes ("`t1 -> 20 (CopyPropagation)`").
The structured alternative is exactly what LLVM ships — typed remark
objects serialized to YAML with dedicated viewers [5] — and we
concede it is the scalable form. Strings were chosen because at this scale the consumer
is a human reading a panel or a `.s` file, and a remark schema would
be structure nothing downstream parses. The choice exposed a genuine
finding the structured design would *also* exhibit: per-instruction
history is **not transitive**. When DCE removes a folded instruction
whose constant was already propagated onward, its ConstantFolding
note dies with it; the surviving instruction reports only the
propagation step (Fig. F5 walks the fixed-point iterations for
`(2+3)*4`). Both the optimizer and visualizer suites lock this
behavior in as documented semantics rather than accident — and it
delimits what per-instruction provenance can ever promise without a
dataflow-chained design (Future Work, §13).

**D12 — Comment-based assembly provenance vs. debug-info
directives.** Codegen emits `# line N: <ir> ; <notes>` above each
instruction block. The rejected alternative — `.loc`/DWARF — is the
production answer and is *machine*-consumable by debuggers; it was
rejected here because our consumer is a reader, our 2006-era target
toolchain makes DWARF plumbing a project of its own, and the paper's
claim is about preserving explanation, not shipping debuggers. Cost:
tools cannot consume our provenance from the binary; it lives in the
text artifacts (assembly comments, JSON) only.

---

## Working notes (stripped at submission)

Verification ledger: all v1 ⚠ items resolved (16 factory methods;
LOC share now maintained in §8.1, which supersedes the 28.2%
snapshot noted here earlier; LLVM
Programmer's Manual citation wired into D2; fallback-unreachable
claim verified by grep; D14 numbers wired from the interleaved
campaign). Note for D10's cost sentence: provenance's measured
price is ~12–17% TIME (E2 shows memory ≤1.3%) — keep D10 and §9.2
consistent on this. Open: F4 figure source = recorded-trace probe
output; cross-refs D4→§10, D11→§13 present.

Peer-review round 1 (five-hat) applied as R1-R6 in v2; round 2 due
after v3 numbers land.
