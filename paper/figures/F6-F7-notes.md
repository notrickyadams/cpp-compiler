# F6 spec + F7 wrapper

## F6 — visualizer screenshot (capture protocol)

Content: split view, Diagnostics tab + IR (after) tab, for the SAME
program as F1 (`return x 2;` variant and the provenance program) so
the figure chain stays on one example family.

Capture protocol (do at session F, on the CURRENT build):
1. `mingw32-make compiler` (full config — an ablated binary shows
   fallback traces; this exact mistake happened once, see
   F1-diagnostic.md verification note).
2. `python visualizer/server.py`; open http://127.0.0.1:8000.
3. Paste the F1 program; Compile; screenshot the Diagnostics panel
   showing ERROR TYPE→TRACE with recorded frames + details.
4. Paste `int main() { return (2 + 3) * 4; }`; Compile; screenshot
   IR (after) with line gutters + the CopyPropagation note (F5's
   surviving history).
5. Verify the trace text in the screenshot is byte-identical to F1;
   crop to panel; PNG at 2x.

Caption draft: *The visualizer rendering the same diagnostic value
as Fig. F1 (§7): identical sections, identical recorded trace,
because both surfaces are renderers over one struct (D6). Right: IR
panel with per-instruction source-line gutters and optimizer notes.*

## F7 — overhead figure (generated, self-checking)

File: `F7-overhead.svg`. Generator: `tools/plot_overhead.py`
(stdlib; deterministic). Properties, in order of importance:

1. **Self-checking**: the script recomputes all 24 medians from the
   raw campaign CSVs and HARD-ASSERTS them against the section-9
   tables (±0.05) — the figure cannot silently diverge from the
   paper. Bootstrap whiskers replicate analyze.py's seed and
   iteration order, so CI endpoints equal the published intervals
   exactly (spot-verified: naive full 20k [143.6, 203.5]; lazy
   trace 100 [−3.8, 4.0]).
2. Two small-multiple panels, ONE shared y-axis (−20…215%, sized to
   contain the tallest CI) — the visually flat lazy panel is the
   finding.
3. Identity never color-alone: fixed palette order (blue/aqua/
   yellow, reference palette), plus dash patterns (solid/dashed/
   dotted), plus marker shapes (circle/square/triangle), plus legend
   and collision-resolved end labels — print- and grayscale-safe.
4. Render-and-look pass performed structurally (whisker extents and
   label separations verified numerically after two layout bugs were
   caught: a whisker escaping the panel top, and 3px end-label
   overlap); embed-time visual pass repeats at session F.

Caption draft: *Always-on overhead of the explanation architecture
versus the ablated baseline, before and after the lazy-formatting
repair (D14), across program sizes (log scale). Whiskers: 95%
bootstrap CIs (§9.1). Left: eager detail strings cost up to ~175%
(trace) and ~160% (full). Right, same scale: recording collapses to
CIs spanning zero; the remaining 12–17% is provenance (§9.2).*

Regenerate: `python tools/plot_overhead.py` (fails loudly on any
figure/paper mismatch).
