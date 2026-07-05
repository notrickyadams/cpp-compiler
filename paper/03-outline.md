# Phase 3 — Paper Outline (approved, incl. 4 section merges)

Venue skeleton: SLE/Onward!-class, ~12-14 pages + refs. The
25-item template is consolidated; merges were approved explicitly.

1. **Title** — candidates: (a) "Explanation as Architecture:
   Recorded Traces, Provenance, and Testable Diagnostics in a
   Compiler Built to Explain Itself"; (b) "Diagnostics as
   First-Class Artifacts"; (c) "Compilers That Explain Themselves".
   Risk: "explainable" evoking XAI — defuse in abstract.
2. **Abstract** — problem -> gap -> architecture -> evidence ->
   costs. No user-benefit claims.
3. **Introduction** (merges Motivation, RQ, Contributions) — opens
   with the missing-return anecdote; C1-C4; F1 = annotated real
   diagnostic (money figure).
4. **Background** — grammar verbatim from Parser.hpp; T1 language
   feature inventory; "C-like kernel", never "C++ subset".
5. **Related Work** — five clusters (see 02-literature-review.md);
   T2 fragment matrix (rows: recorded traces, KB isolation,
   StageOutput contract, provenance-in-contract, property tests;
   cols: GHC, rustc, LLVM, this work) — the novelty argument as a
   table. Concede LLVM remarks + rustc UI tests BEFORE the reviewer.
6. **The Explanation Architecture** (merges 4 template chapters;
   ~4pp; THE core) — drafted: sections/06-explanation-architecture.md.
   6.1 stage contract + failure taxonomy (D1-D2); 6.2 five-role core
   (D3-D6); 6.3 recorded traces (D7-D9, D13-D14); 6.4 provenance
   (D10-D12 + DCE negative result). Figures F2-F5.
7. **Visualization Framework** — 1 page, consumer of the
   architecture, no novelty claimed. F6 screenshot.
8. **Implementation** — reproducibility + invasiveness (E5, T4);
   empirical toolchain-probing paragraph; Windows/GCC-6.3 scoping.
9. **Evaluation** (merges Methodology/Evaluation/Performance/
   Quality) — 9.1 methodology + environment; 9.2 overhead E1/E2
   (F7 scaling figure, T5) incl. the DISCARDED-campaign methodology
   note and the naive->lazy arc; 9.3 quality E3 (T6 component
   matrix) + E4 (T7 cascade distributions, accepted-set asymmetry);
   9.4 property tests as evidence class.
10. **Discussion** — KB scaling threshold (D4 -> DSL); trace noise;
    what production adoption needs; convergent evolution.
11. **Threats to Validity** — construct: presence != helpfulness
    (bold, cite Denny/Pettit); single machine/toolchain; author-
    designed corpus; drift between measurement blocks.
12. **Limitations** — curated fallback (unreachable in pipeline,
    verified); selective scoping; DCE note loss; reserved kinds;
    single TU; no user validation.
13. **Future Work** — fully-designed user study (built-in ablation:
    full vs compact renderer); transitive provenance chaining;
    grammar-general malformed detection; E4 intersection statistic.
14. **Conclusion** — restate C1-C4 with evidence types.
15. **References** — 02-literature-review.md; DOI debt listed there.
16. **Appendix/Artifact** — grammar, kind reference, full outputs,
    corpus + scripts, repo tag. Public artifact recommended.

## Figure/Table inventory

F1 annotated diagnostic (return x 2) · F2 pipeline w/ diagnostics
spine · F3 five-role diagram · F4 trace-recording sequence
(scopes + snapshot) · F5 provenance flow w/ DCE note-death ·
F6 visualizer screenshot · F7 overhead vs size.
T1 language features · T2 fragment matrix · T3 kind inventory ·
T4 per-mechanism LOC · T5 overhead numbers · T6 E3 matrix ·
T7 E4 distributions · T8 limitations (appendix).

## Instrumentation prerequisites (all now done)

Ablation flags + TRACE_SCOPE macro; corpus generator; measurement +
analysis scripts; --check mode; mutants + scorer (docs/experiments.md).
