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
| — Reference registry | [references.md](references.md) | numbered [1]-[26]; verified-only numbering; debt list explicit |
| 3 — Outline | [03-outline.md](03-outline.md) | approved (4 section merges) |
| 4 — §1/§2 Title+Abstract | [sections/02-title-abstract.md](sections/02-title-abstract.md) | draft v1 — title picked (veto welcome) |
| 4 — §3 Introduction | [sections/03-introduction.md](sections/03-introduction.md) | draft v1 — reviewed once; F1 now exists |
| 4 — §4 Background | [sections/04-background.md](sections/04-background.md) | draft v1 — grammar verbatim; textbook cite in debt |
| 4 — §5 Related Work | [sections/05-related-work.md](sections/05-related-work.md) | draft v1 — concessions-first; T2 matrix; author-name discipline |
| 4 — §7 Visualization | [sections/07-visualization.md](sections/07-visualization.md) | draft v1 — consumer framing; F6 at session D |
| 4 — §8 Implementation | [sections/08-implementation.md](sections/08-implementation.md) | draft v1 — fresh LOC (5,943; diagnostics 29.3%) |
| — Figure F1 | [figures/F1-diagnostic.md](figures/F1-diagnostic.md) | captured from full build; callout anchors + caption |
| 4 — §6 Architecture | [sections/06-explanation-architecture.md](sections/06-explanation-architecture.md) | draft v3 — D1-D14, final numbers wired |
| 5 — Experiments | [../docs/experiments.md](../docs/experiments.md), [../docs/results-naive.md](../docs/results-naive.md) | E1-E5 complete; 3 protocol iterations (--json discarded → blocked → interleaved) |
| 4 — §9 Evaluation | [sections/09-evaluation.md](sections/09-evaluation.md) | draft v1 complete — final interleaved tables; peer-review round pending |

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

## Road to the final paper (state: 2026-07-11)

Done: all experiments (E1-E5, 3 protocol iterations), §6 v3, §9 v1.1
(each five-hat-reviewed once), evidence docs, this tracker.

| # | Work session | Deliverable |
|---|---|---|
| A | §3 Introduction + §2 Abstract + title pick (coupled; unblocked now that evaluation exists) | sections/02,03 |
| B | §4 Background, §5 Related Work prose, §7 Visualization, §8 Implementation — mostly transcription of verified material | four short sections |
| C | §10 Discussion, §11 Threats, §12 Limitations, §13 Future Work (incl. full user-study design), §14 Conclusion | honesty block |
| D | Figures F1-F7 (F7 scripted from CSVs; diagrams as code) + tables T1-T8 final | figures/ |
| E | References: DOI verification pass, C2 counterexample sweep (VL/HCC, ICSME/SANER), BibTeX | references.bib |
| F | Phase 7 assembly: acmart LaTeX (compile on Overleaf; no local TeX assumed) + cross-consistency pass | paper/latex/ |
| G | Phase 8: full-paper five-hat review + revisions | submission-ready v1 |

Estimate: ~7 focused sessions at the current cadence — roughly one
to two weeks of elapsed collaboration. Explicitly OUTSIDE this
estimate: external advisor/co-author review, venue-specific
formatting quirks, artifact-evaluation packaging (~1 extra session),
and any decision to collect more data (e.g. more 20k runs to tighten
CIs, multi-seed E4).
