# §5 Related Work — draft v1

Five clusters position this work; the two concessions come first,
because a reviewer should meet them in our words before theirs.

## 5.1 Structured diagnostics in production compilers

Production compilers have been converging on diagnostics-as-data by
retrofit. Rust formalized its error format and structured
suggestions in RFC 1644 [3], emits machine-consumable JSON
diagnostics [12], and maintains Fluent-based message infrastructure
with a public error index [10]. GHC's migration from `SDoc` strings
to "errors as structured values" — with `diagnosticReason` and
`diagnosticHints` fields and public error codes since 9.6.1 — is
documented across its development tracker and implementers' reports
[4, 9], and the community maintains an error index [8]. **Concession
one:** the five-role core of §6.2 is therefore not a new invention
but a distillation; our claims are that designing it in from the
first commit is cheap (§8), that the isolated knowledge base and
root-cause-granularity taxonomy are the load-bearing choices (§6,
D3–D4), and that the retrofit pain documented by these projects [4,
9] is evidence the pattern matters. To our knowledge, none of these systems
provides the recorded per-diagnostic execution trace of §6.3. That
knowledge is not passive: a targeted counterexample sweep (recorded
in the reference registry) found only adjacent work — trace tooling
for *student programs* [30], and compiler-internal debug dumps
aimed at the compiler's own developers rather than its users — and
if later work surfaces, this sentence and C2 narrow accordingly.

## 5.2 Optimization transparency and provenance

LLVM's remarks infrastructure [5] is production-scale prior art for
per-transformation records: passes emit typed remark objects with
source locations, serialized to YAML and visualized with dedicated
tools. **Concession two:** our per-instruction notes are the same
mechanism at teaching scale. The residue we claim: unification of
provenance with the *diagnostic* architecture under one contract
(one struct family, one renderer discipline, one JSON export), its
measured always-on price (12–17%, §9.2 — LLVM's remarks are opt-in
per build), the human-readable in-text form (`# line N:` comments in
the emitted assembly, by analogy to `.loc` directives but for
readers), and the negative result that per-instruction history is
not transitive under dead-code elimination (§6.4, §9).

## 5.3 Error-message quality and its contested effectiveness

A half-century of work examines programming error messages; Becker
et al.'s working-group report maps the landscape [14]. Eye-tracking
evidence shows developers genuinely read error messages and that
reading difficulty predicts task performance [15]; design guidance
for novice-facing messages continues to develop [16]. Yet controlled
studies of *enhanced* messages report effects from ineffectual [1]
to inconclusive [2] — a contested record extending into the LLM era,
where generated explanations show promise in some deployments [25]
and ineffectiveness in others [26]. This contestation shapes our
claim discipline (§9.3): we measure component *presence* with a
mechanical instrument descended from Marceau et al.'s evaluation
rubric [13], and defer all helpfulness claims to a designed-but-
unexecuted study (§13). Our contribution is orthogonal to wording
quality: an architecture that *retains the evidence* any wording —
human-authored or LLM-generated [25] — would need.

## 5.4 Error localization and interrogative debugging

Constraint-based localization computes *where* the likely mistake
is: general static-error diagnosis via Bayesian-ranked unsatisfiable
constraints [17, 18], type-error diagnosis [19], and SMT-based
localization [20]. These are complementary: they improve one field
of our diagnostic (the span and root-cause selection), while our
architecture governs how the whole explanation is assembled,
preserved, and delivered. The Whyline [11, 21] pioneered
why/why-not questions over *program* executions via tracing and
slicing, a family that extends into education through trace-
divergence debugging of student code [30]; we ask the interrogative
question of the *compiler's own run* — a far shallower trace,
recorded live at negligible cost (§9.2) rather than reconstructed by
post-hoc analysis.

## 5.5 Compiler visualization in education

The lineage is old — the UW Illustrated Compiler graphically
displayed its own control and data structures to students in 1988
[28] — and continues through AST construction animators [22],
interactive stage simulations [23], and, recently, JIT IR-evolution
explorers [24]. Our visualizer (§7)
sits in this lineage as a *consumer* of the architecture rather than
a contribution: its distinguishing property is that every panel
renders the same structured values the CLI renders, because the
architecture guarantees a single source of truth (§6.2, D6).

## Positioning summary (Table T2)

| Capability | GHC [4] | rustc [3,10,12] | LLVM [5] | this work |
|---|---|---|---|---|
| Structured diagnostics as values | retrofit | retrofit | — | designed-in |
| Root-cause-indexed public error index | ✓ [8] | ✓ | — | ✓ (taxonomy = index, D3) |
| Machine-readable diagnostic export | ✓ | ✓ [12] | ✓ (remarks) | ✓ |
| Per-diagnostic **recorded execution trace** | — | — | — | ✓ (§6.3) |
| Optimization provenance records | — | — | ✓ opt-in [5] | ✓ always-on, unified, priced |
| Diagnostic-quality **property** tests | snapshot-style | golden `.stderr` [10] | — | property-level + mutation-measured (§9) |

<!-- Working notes: all [n] per paper/references.md; [17]/[18] both
     cited for the localization line (POPL paper + TOPLAS system);
     [19][20][22][23][24][25][26] have AUTHORS-unverified flags —
     no author names appear in this prose by design until session E.
     Table T2 cells for GHC/rustc "snapshot-style" verified via [10]
     (UI tests) and GHC testsuite practice — the GHC cell needs a
     doc pointer at session E or the cell text softens to "—".
     Reviewer risk: 5.1's "none provides recorded traces" is the C2
     counterexample-sweep dependency (registry debt). -->
