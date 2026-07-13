# LaTeX assembly

Build target: Overleaf (pdfLaTeX, shell-escape enabled — required by
the `svg` package for F7). Locally: TeX Live + inkscape on PATH.

## Regeneration pipeline (do not hand-edit generated files)

1. Prose lives in `paper/sections/*.md`. After edits:
   `python tools/md2tex.py` regenerates `sections/*.tex`
   (all except `figures.tex`, which is hand-maintained).
2. F7 regenerates via `python tools/plot_overhead.py` (self-checking
   against §9's tables; copy `paper/figures/F7-overhead.svg` here
   into `figures/` if paths change).
3. `references.bib` is generated from `paper/references.md` by hand;
   entries carry `TODO-export` notes for the five known micro-debts
   (pages ×3, one author-list edge, one Notices issue, TraceDiff
   authors/DOI).

## Session-G punch list (known assembly gaps)

- §ref conversion: prose still says "§9.2"-style plain text; convert
  the load-bearing ones to `\ref` (labels exist: sec:intro,
  sec:background, sec:related, sec:architecture, sec:visualizer,
  sec:impl, sec:eval, sec:discussion, sec:threats, sec:limitations,
  sec:future, sec:conclusion).
- Decision-block names (D1–D14) are plain text by design.
- §4's T1 stayed prose; consider promoting to a floating table.
- F4 folded into F1 at first assembly (space) — ledgered source
  retained if a dedicated figure is wanted.
- F6 placeholder pending screenshot capture.
- First compile on Overleaf: expect Unicode stragglers the converter
  missed — fix in the .md sources + reconvert, never in .tex.
- Citation renumbering is automatic via BibTeX order — the .md [n]
  numbers map to keys, so registry numbering and final numbering may
  differ (expected; the registry is the source of truth for keys).
