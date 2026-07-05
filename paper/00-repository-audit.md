# Phase 0 — Repository Audit (approved; snapshot at commit 48b5054)

Basis: direct read of all tracked files; full suite executed;
defects reproduced live. Numbers updated where later phases changed
them (LOC/test counts move with the repo; verify at write-time).

## Inventory (at audit time)

~5.4K LOC compiler source; diagnostics subsystem largest at ~27-28%
(quantitative support for "diagnostics are first-class"). Tests
~2.5K LOC across 9 suites, incl. end-to-end suites that assemble,
link and RUN produced binaries, asserting process exit codes.
Zero external dependencies (hand-rolled test framework, JSON
helpers, stdlib-only visualizer server).

## Source language (honesty item)

NOT "a C++ subset" in any meaningful sense: a C-like kernel — int
only, functions with parameters (no calls), arithmetic/comparisons,
right-associative assignment expressions, return. No control flow,
strings, arrays, pointers, unary ops. C++14 is the IMPLEMENTATION
language. The paper must say "C-like kernel language".

## Architecture (verified)

Acyclic layering: core <- diagnostics <- every stage; stages depend
only downward. Three-way failure taxonomy consistently applied:
Diagnostic (user-source mistakes, has span), Result<T,E> (environment
failures, Toolchain only), assert (compiler invariants). Diagnostic
core in five roles: Kind taxonomy / Engine factory / ExplanationBuilder
knowledge base / Collector with three renderers / StageOutput<T>
transport.

## The critical findings that drove the work plan

1. **Traces were curated, not recorded** — a static per-kind lookup;
   "compiler reasoning reconstruction" was overstated. -> Fixed by
   TraceRecorder (Workstream B) — now the paper's C2.
2. **Provenance died at semantic analysis** — IR/asm carried no
   spans. -> Fixed by span+note threading (Workstream B) — C3.
3. Two spec'd components never existed (DiagnosticContext,
   ErrorTraceGraph) — the paper describes the implemented
   architecture only.
4. Five diagnostic kinds defined but unreachable — marked [reserved]
   in the header; tests must not expect them.
5. The genuinely dynamic artifact pre-B was the semantic check log —
   under-weighted by the original framing.

## Testing / build

Strengths: per-stage suites + true end-to-end; empirical toolchain
probing methodology. Gaps (disclosed, some later closed): no CI, no
coverage numbers, no fuzzing; mutation testing added later (E4);
Windows-locked Makefile; single toolchain (GCC 6.3.0 MinGW 32-bit).

## Verdict that shaped Phase 1

Strongest honest framing: an architectural pattern for explanation-
oriented diagnostics in a teaching-scale compiler, with testable
quality invariants and full-pipeline visualization — an experience/
architecture paper, NOT a PLDI technique paper. User chose option
(b): make the overstated claims true in code (recorded traces +
provenance) rather than soften them — done in Workstream B.
