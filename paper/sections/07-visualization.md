# §7 The Visualization Consumer — draft v1

The visualizer exists in this paper as *evidence about the
architecture*, not as a contribution in the visualization lineage it
belongs to (§5.5). Its claim is narrow: because explanation is
preserved as structured data (§6), building a full-pipeline
inspection surface required no new information channels — only
renderers.

One CLI mode (`--json`) emits the entire compilation as a single
JSON object: tokens with positions, the type-annotated AST, the
semantic check log, IR before and after optimization (both as
display text and as structured per-instruction records with source
line and transformation note), the optimization reports, the final
assembly text, and the diagnostics with their recorded traces. The
export has a fixed shape: a stage that never ran contributes an
explicit empty value, never an absent key, so consumers need no
existence logic. A stdlib-only local server shells out to the
compiled binary per request; the frontend is framework-free
JavaScript rendering one tab per stage (Fig. F6). Stages that did
not run say why ("no semantic checks ran — lex/parse stopped
first"), which is itself explanation: the pipeline's control-flow
decisions are user-visible state.

Two properties matter for the thesis. First, **single source of
truth**: the diagnostics tab renders the same struct the CLI
renders — same sections, same recorded trace with the same runtime
details, same invalid/valid examples — because both are renderers
over one value (D6); divergence between the two surfaces was an
actual defect class earlier in this project's history, eliminated
by construction rather than by discipline. Second, **provenance
becomes navigable**: the IR panels carry per-instruction source-line
gutters and optimizer notes from the same fields codegen prints as
assembly comments, so "which line did this instruction come from,
and what happened to it" is answerable in every representation the
compiler has.

The design deliberately avoids claiming more: there is no
cross-panel linked highlighting (clicking an assembly line does not
select its source token), no time-travel through optimizer
iterations, and no user evaluation of the interface — the first two
are engineering the exported data already supports (§13), the third
is out of scope with the rest of the human questions (§11).

<!-- Working notes: JSON contract facts verified against
     compileToJson() (fixed-shape comment, empty-value policy) and
     app.js placeholders; "same struct" claim = renderJson + app.js
     rendering path (verified during the consistency fix). F6
     screenshot to capture at session D — diagnostics tab + IR
     panel, same return-x-2 program as F1 for continuity. -->
