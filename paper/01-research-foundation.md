# Phase 1 — Research Foundation (approved)

## Problem statement

In conventional compilers, the evidence behind a diagnostic is
discarded before the diagnostic is emitted: the compiler knew which
function it was analysing, which grammar rule it was in, what it had
successfully consumed, and which check failed — but what survives to
the user is a rendered string plus a location. Explanation is a
presentation-layer afterthought, not an architectural obligation.

Motivating defect class (first-hand, verified in this repository's
history): the missing-return case — the analyzer logged the failure
internally, the user saw nothing, the binary returned garbage. The
compiler knew; the user didn't.

NOT the framing: "compilers have bad error messages" — production
compilers invest heavily there and the effectiveness literature is
contested (see Phase 2). The gap is architectural.

## Research question

**RQ:** What architectural mechanisms are required for a compiler to
treat explanation as a first-class, testable artifact — such that
(a) every diagnostic carries a trace recorded from actual execution,
(b) every pipeline stage preserves source provenance through to
assembly, and (c) diagnostic-quality properties are enforced by
automated tests?

- RQ1 (mechanism): which mechanisms suffice; how invasive? —
  answerable from the implementation (E5).
- RQ2 (cost): runtime/memory overhead of always-on recording — E1/E2.
- RQ3 (quality properties): cascade-freedom and trace accuracy as
  regression-tested invariants — implemented; residual measured (E4).
- RQ4 (comparative structural completeness): component presence vs a
  baseline compiler on identical inputs — E3. Presence, not quality.
- RQ5 (human benefit): FUTURE WORK ONLY — requires the user study.

## Hypotheses

- **H1:** achievable with localized, low-invasiveness mechanisms.
  Evidence: ~320 LOC / 5.6% of compiler source for both mechanisms (E5).
- **H2:** recorded traces are execution-accurate (contain runtime
  facts unobtainable by static per-kind lookup). Evidence: tests.
- **H3:** always-on overhead is small. **Final verdict (interleaved
  protocol): REFUTED for naive eager-string recording (+143% at 20k
  lines); REPAIRED by lazy detail formatting (D14) to 1-2% with CIs
  spanning zero (~60x reduction, byte-identical output). Provenance
  is NOT free — a steady 12-17% (both blocked campaigns' readings
  were drift artifacts) — so the full architecture costs ~15-19%
  wall time and <=1.3% memory.** H3 final form: "recording is free
  after a lazy-formatting design; the residual cost of the
  architecture is provenance's per-instruction fields, quantified
  and attributable."
- **H4:** (with a study) explanations aid users — NOT claimed.

## Contributions (post-Phase-2 revision, adversarially narrowed)

- **C1** — architectural pattern: the five-role diagnostic core +
  StageOutput pipeline contract; framed as a distilled synthesis of
  what GHC/Rust converged toward by retrofit (convergence = evidence).
- **C2** — execution-recorded reasoning traces (RAII scopes,
  snapshot-at-creation, curated fallback for recorder-less use).
  Strongest novelty claim; survived the term/concept search.
- **C3** — end-to-end provenance under the same contract, CONCEDING
  LLVM remarks as production-scale prior art for optimizer notes;
  distinct residue: unified contract + the DCE non-transitivity
  finding.
- **C4** — diagnostic-quality PROPERTIES as executable invariants
  (vs rustc's golden-text snapshots); E4 measures the residual
  frontier honestly (3% of single-fault mutants still cascade >=3).

## User decisions (recorded 2026-07-05)

1. **Venue/framing:** architecture-first; education is an
   application, not the contribution. Keep broad until lit review
   settles positioning (it did: SLE/Onward! primary).
2. **User study:** none for v1; design as future work; all claims
   must stand without it.
3. **Overhead evaluation:** mandatory; reproducible measurements
   before the evaluation section is written; literature review run
   adversarially against our own novelty.

## Anticipated objections (with response strategy)

| Objection | Response |
|---|---|
| Toy language, won't scale | Pattern demonstration + mechanism inventory; scaling limits discussed, not claimed away |
| Rust/Elm already do this | Fragment matrix (Phase 2): message quality != recorded traces != tested invariants |
| No user evaluation | Claims scoped to capability + measurable properties; study designed in Future Work |
| Traces selective | Placement is a stated rule + inspectable data; omission bounds resolution, cannot fabricate provenance |
| Overhead | Measured, factorially attributed, naive design refuted and repaired with numbers |
