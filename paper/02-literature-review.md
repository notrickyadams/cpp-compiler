# Phase 2 — Literature Review (adversarial; approved)

All entries verified by live web search during the review session.
DOIs/page numbers to be finalised at References time. Nothing below
may be cited from memory alone.

## Novelty threat assessment (the verdict per contribution)

- **C3 (optimizer notes / provenance): seriously threatened —
  reframed.** LLVM remarks infrastructure is direct production prior
  art: per-transformation records with source locations, YAML
  serialization, opt-viewer HTML. Sources: llvm.org/docs/Remarks.html,
  llvm.org/docs/CommandGuide/llvm-opt-report.html, Clang User's
  Manual (-fsave-optimization-record). Residue we keep: unified
  explanation contract + in-text readable provenance + the DCE
  non-transitivity finding (locked by tests).
- **C4 (tested diagnostics): threatened — narrowed.** rustc UI tests
  snapshot exact diagnostic output in .stderr golden files
  (rustc-dev-guide tests/ui, compiletest; rust-lang/rust #45516).
  Surviving claim: PROPERTY-level invariants (one-mistake-one-
  diagnostic; trace contains runtime facts) vs golden-text snapshots.
  The distinction must be drawn in-text with the rustc citation.
- **C2 (recorded traces): survives.** "Explainable compiler" /
  "self-explaining compiler" term search: no established claimant.
  Nearest neighbour: Whyline (Ko & Myers, ICSE 2008, "Debugging
  Reinvented"; ICSE 2018 Most Influential Paper) — why/why-not for
  PROGRAM behaviour, not the compiler's own decisions. Residual risk:
  pre-submission sweep of VL/HCC + ICSME/SANER proceedings.
- **C1 (five-role architecture): partially anticipated — reframed as
  convergent-evolution synthesis.** GHC "errors as structured values"
  (GHC wiki; issue #18516; Well-Typed 2021 blog; Haskell Error Index,
  errors.haskell.org; [GHC-12345] codes since 9.6.1) and Rust
  structured diagnostics (RFC 1644; rustc JSON output docs).
  Retrofit pain there = evidence the designed-in pattern matters.

## Verified bibliography by theme

**Error-message quality & contested effectiveness**
- Becker et al., "Compiler Error Messages Considered Unhelpful: The
  Landscape of Text-Based Programming Error Message Research",
  ITiCSE-WGR 2019 (ACM DL 10.1145/3344429.3372508). Field survey.
- Barik et al., "Do Developers Read Compiler Error Messages?",
  ICSE 2017 (10.1109/ICSE.2017.59). Eye-tracking, N=56; 13–25% of
  task time on errors; reading difficulty predicts performance.
- Marceau, Fisler, Krishnamurthi, "Measuring the Effectiveness of
  Error Messages Designed for Novice Programmers", SIGCSE 2011
  (Best Paper). THE rubric instrument (E3's ancestor).
- Denny, Luxton-Reilly, Carpenter, "Enhancing Syntax Error Messages
  Appears Ineffectual", ITiCSE 2014. CONTRA enhanced messages.
- Pettit, Homer, Gee, "Do Enhanced Compiler Error Messages Help
  Students? Results Inconclusive", SIGCSE 2017. CONTRA/inconclusive.
- Becker et al., "On Designing Programming Error Messages for
  Novices: Readability and its Constituent Factors", CHI 2021
  (10.1145/3411764.3445696).

**Error localization**
- Zhang & Myers, "Toward General Diagnosis of Static Errors",
  POPL 2014; SHErrLoc, TOPLAS 39(4) 2017 (10.1145/3121137).
- "Diagnosing Type Errors with Class", PLDI 2015 (10.1145/2737924.2738009).
- "Practical SMT-Based Type Error Localization", ICFP 2015
  (10.1145/2784731.2784765).
Relation: they compute WHERE; we architect HOW explanation is
assembled/delivered. Complementary.

**Cascading errors / recovery**
- Diekmann & Tratt, "Don't Panic! Better, Fewer, Syntax Errors for
  LR Parsers", ECOOP 2020 (LIPIcs; arXiv 1804.07133). Panic mode:
  ~981K vs CPCT+ ~436K error locations on 200K invalid Java files.
  Framing citation for E4; our contribution is a tested property on
  recursive-descent recovery, not a recovery algorithm.

**Interrogative debugging**
- Ko & Myers, "Debugging Reinvented", ICSE 2008 (10.1145/1368088.1368130);
  Java Whyline, TOSEM 2010.

**Structured diagnostics in production (documentation sources)**
- GHC: wiki "Errors as (structured) values"; GitLab #18516;
  Well-Typed "The new GHC diagnostic infrastructure" (2021);
  Haskell Error Index. Haskell Foundation tech proposal 024.
- Rust: RFC 1644; rustc JSON docs; rustc-dev-guide diagnostics +
  UI-test chapters.
- LLVM: Remarks docs; Programmer's Manual (recoverable errors /
  Expected<T> — load-bearing citation for the Result-vs-Diagnostic
  failure taxonomy, D2).

**Educational compiler visualization (supporting artifact only)**
- VAST, ACM SoftVis 2008 (10.1145/1409720.1409759).
- "Interactive educational simulations ... compiler construction",
  ITiCSE 2013 (10.1145/2462476.2462498).
- JITScope, arXiv 2505.21599 (2025).
- "UW illustrated compiler": secondhand references only — DO NOT
  CITE until a primary source is located.

**Practitioner (footnote-only, if at all)**
- Elm "Compiler Errors for Humans" (blog) — verify before any use.

## Gap statement

No verified work unifies: (i) per-diagnostic execution-recorded
traces exposed to users, (ii) a root-cause-indexed explanation
knowledge base isolated from all stages, (iii) an output+diagnostics
contract on every stage, (iv) cross-stage provenance under the same
contract, (v) property-level diagnostic-quality tests — in one
minimal, fully-tested, whole-studyable architecture. Production
systems hold fragments; none presents the synthesis.

## Revised contribution statement (approved)

We present an architecture for explanation-oriented compilation in
which every diagnostic is a structured artifact carrying an
execution-recorded trace of the compiler's decision path,
explanations are assembled from a root-cause-indexed knowledge base
isolated from all pipeline stages, source provenance survives to
emitted assembly under the same contract, and diagnostic-quality
properties — including cascade-freedom and trace accuracy — are
enforced as executable invariants. We validate the architecture with
a complete, zero-dependency reference compiler (~5.4K LOC, 200+
tests), measure its overhead, and characterize two limitations
honestly: per-instruction provenance is not transitive under dead-
code elimination, and naive always-on trace recording costs ~2.4x
compile time — motivating the lazy-formatting design we then
implement and re-measure. Production compilers have independently
converged on fragments of this design; we contribute the unified
pattern, its minimal reference realization, and its measured costs.

## Outstanding verification debt

- Final DOIs/pages for every entry.
- UW illustrated compiler primary source, or drop.
- C2 counterexample sweep (VL/HCC, ICSME/SANER) before submission.
