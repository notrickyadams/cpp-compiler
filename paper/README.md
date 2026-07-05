# Paper working directory

Research paper on the explainable-compiler architecture, developed
phase-by-phase (workflow and approvals tracked in the project
conversation; artifacts live here so nothing exists only in chat).

## Status

| Phase | Artifact | State |
|---|---|---|
| 0 — Repository audit | [00-repository-audit.md](00-repository-audit.md) | approved |
| 1 — Research foundation | [01-research-foundation.md](01-research-foundation.md) | approved (3 user decisions recorded) |
| 2 — Literature review | [02-literature-review.md](02-literature-review.md) | approved; DOI finalisation pending |
| 3 — Outline | [03-outline.md](03-outline.md) | approved (4 section merges) |
| 4 — §6 Architecture | [sections/06-explanation-architecture.md](sections/06-explanation-architecture.md) | draft v2 + lazy-recorder update; v3 after §9 numbers |
| 5 — Experiments | [../docs/experiments.md](../docs/experiments.md), [../docs/results-naive.md](../docs/results-naive.md) | E1-E5 collected; optimized-recorder rerun in progress |
| 4 — §9 Evaluation | sections/09-evaluation.md | NEXT — blocked on optimized E1/E2 tables |

## Standing rules (from the supervisor mandate)

- Every technical claim classified: verified-by-implementation /
  verified-by-experiment / verified-by-citation / needs-X.
- Every major design decision presented as an engineering tradeoff:
  chosen approach vs at least one alternative, rejection rationale,
  costs of the choice, implementation + literature evidence.
- No user-benefit claims without a user study (Denny 2014 / Pettit
  2017 discipline); the study is designed as future work only.
- Never fabricate design history: "we first built X" only when the
  git history shows it; untried options are design-space comparison.

## Venue targets (Phase 1 decision: architecture-first)

Primary: SLE / Onward!. Education venues (SPLASH-E/SIGCSE) become an
option if the future-work user study is ever run.
