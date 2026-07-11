#!/usr/bin/env python3
"""
Figure F7 generator — always-on overhead of the explanation
architecture, naive vs lazy recorder. Stdlib only; deterministic.

Accuracy contract (the reason this script exists instead of a
drawing): every plotted value is recomputed from the raw campaign
CSVs, the bootstrap replicates tools/analyze.py exactly (same seed,
same resample method, same iteration order per campaign, so the CI
endpoints equal the published tables), and the medians are ASSERTED
against the values printed in the paper's section 9 — if the data
and the figure ever disagree, the build fails rather than lies.

Design (per the data-viz method): two small-multiple panels sharing
one y-axis — the flat lazy panel IS the finding; series identity is
never color-alone (fixed dash pattern + marker shape + legend +
selective direct labels), print/grayscale-safe.

Usage: python tools/plot_overhead.py
Reads: experiments/out/naive/timing.csv, experiments/out/timing.csv
Writes: paper/figures/F7-overhead.svg (+ prints the value table)
"""
import csv
import random
import statistics
from collections import defaultdict

SIZES = [100, 1000, 5000, 20000]
# series key -> (label, config-vs-baseline, palette slot hex, dash, marker)
SERIES = [
    ("full",  "full architecture", "full",    "#2a78d6", "",      "circle"),
    ("trace", "trace only",        "noprov",  "#1baf7a", "6,4",   "square"),
    ("prov",  "provenance only",   "notrace", "#eda100", "2,3",   "triangle"),
]
RESAMPLES = 10000

# The published section-9 medians (% overhead) — the assertion gate.
EXPECTED = {
    "naive": {
        "full":  {100: 57.5, 1000: 116.8, 5000: 161.6, 20000: 157.0},
        "trace": {100: 24.4, 1000: 79.9,  5000: 175.0, 20000: 142.6},
        "prov":  {100: 14.1, 1000: 21.9,  5000: 44.4,  20000: -1.2},
    },
    "lazy": {
        "full":  {100: 6.7,  1000: 14.9,  5000: 17.4,  20000: 19.0},
        "trace": {100: 1.3,  1000: 1.6,   5000: 1.1,   20000: 2.3},
        "prov":  {100: 11.9, 1000: 14.3,  5000: 17.2,  20000: 16.3},
    },
}


def load(path):
    cells = defaultdict(list)
    with open(path, newline="") as f:
        for row in csv.DictReader(f):
            cells[(row["config"], int(row["size"]))].append(float(row["ms"]))
    return cells


def bootstrap_ci(sample_a, sample_b, rng):
    stats = []
    for _ in range(RESAMPLES):
        ra = [rng.choice(sample_a) for _ in sample_a]
        rb = [rng.choice(sample_b) for _ in sample_b]
        mb = statistics.median(rb)
        if mb == 0:
            continue
        stats.append((statistics.median(ra) - mb) / mb * 100.0)
    stats.sort()
    return stats[int(0.025 * len(stats))], stats[int(0.975 * len(stats))]


def compute(cells):
    """Replicates analyze.py's loop order (size-major, then
    full/noprov/notrace) with one Random(42) per campaign so the CI
    endpoints match the published tables exactly."""
    rng = random.Random(42)
    out = {}
    for size in SIZES:
        base = cells[("baseline", size)]
        for key, _, cfg, _, _, _ in SERIES:
            xs = cells[(cfg, size)]
            pt = (statistics.median(xs) - statistics.median(base)) \
                 / statistics.median(base) * 100.0
            lo, hi = bootstrap_ci(xs, base, rng)
            out[(key, size)] = (pt, lo, hi)
    return out


# ── SVG construction ─────────────────────────────────────────────
W, H = 880, 380
PANEL_W, PANEL_H = 340, 250
PANEL_Y = 70
PANELS_X = [90, 500]
YMIN, YMAX = -20.0, 215.0  # must contain the tallest CI (naive full 20k: 203.5)
import math
XPOS = {s: math.log10(s) for s in SIZES}
XMIN, XMAX = math.log10(80), math.log10(28000)


def sx(panel_x, size):
    t = (XPOS[size] - XMIN) / (XMAX - XMIN)
    return panel_x + t * PANEL_W


def sy(v):
    t = (v - YMIN) / (YMAX - YMIN)
    return PANEL_Y + PANEL_H - t * PANEL_H


def marker(shape, x, y, color):
    if shape == "circle":
        return f'<circle cx="{x:.1f}" cy="{y:.1f}" r="4" fill="{color}" stroke="#fcfcfb" stroke-width="2"/>'
    if shape == "square":
        return (f'<rect x="{x-3.5:.1f}" y="{y-3.5:.1f}" width="7" height="7" '
                f'fill="{color}" stroke="#fcfcfb" stroke-width="2"/>')
    return (f'<polygon points="{x:.1f},{y-4.5:.1f} {x-4.5:.1f},{y+3.5:.1f} '
            f'{x+4.5:.1f},{y+3.5:.1f}" fill="{color}" stroke="#fcfcfb" stroke-width="2"/>')


def panel(px, title, data, direct_labels):
    s = []
    s.append(f'<text x="{px}" y="{PANEL_Y-38}" font-size="13" font-weight="600" fill="#0b0b0b">{title}</text>')
    # gridlines + y ticks (shared scale; labels only on left panel)
    for gv in (0, 50, 100, 150, 200):
        y = sy(gv)
        emph = "#c9c8c3" if gv == 0 else "#e8e7e3"
        s.append(f'<line x1="{px}" y1="{y:.1f}" x2="{px+PANEL_W}" y2="{y:.1f}" stroke="{emph}" stroke-width="1"/>')
        if px == PANELS_X[0]:
            s.append(f'<text x="{px-8}" y="{y+4:.1f}" font-size="11" text-anchor="end" fill="#52514e">{gv}</text>')
    # x ticks
    for size, lbl in ((100, "100"), (1000, "1k"), (5000, "5k"), (20000, "20k")):
        x = sx(px, size)
        s.append(f'<line x1="{x:.1f}" y1="{PANEL_Y+PANEL_H}" x2="{x:.1f}" y2="{PANEL_Y+PANEL_H+4}" stroke="#c9c8c3"/>')
        s.append(f'<text x="{x:.1f}" y="{PANEL_Y+PANEL_H+18}" font-size="11" text-anchor="middle" fill="#52514e">{lbl}</text>')
    # whiskers first (under lines)
    for key, _, _, color, _, _ in SERIES:
        for size in SIZES:
            pt, lo, hi = data[(key, size)]
            x = sx(px, size)
            s.append(f'<line x1="{x:.1f}" y1="{sy(lo):.1f}" x2="{x:.1f}" y2="{sy(hi):.1f}" stroke="{color}" stroke-width="1" opacity="0.55"/>')
            for v in (lo, hi):
                s.append(f'<line x1="{x-3:.1f}" y1="{sy(v):.1f}" x2="{x+3:.1f}" y2="{sy(v):.1f}" stroke="{color}" stroke-width="1" opacity="0.55"/>')
    # lines + markers
    for key, label, _, color, dash, shape in SERIES:
        pts = " ".join(f"{sx(px, s_):.1f},{sy(data[(key, s_)][0]):.1f}" for s_ in SIZES)
        dash_attr = f' stroke-dasharray="{dash}"' if dash else ""
        s.append(f'<polyline points="{pts}" fill="none" stroke="{color}" stroke-width="2"{dash_attr}/>')
        for s_ in SIZES:
            s.append(marker(shape, sx(px, s_), sy(data[(key, s_)][0]), color))
    if direct_labels:
        # Collision-resolved end labels: sort by data y, then push
        # apart to >=14px separation (caught by the render-and-look
        # pass: full 19.0% and prov 16.3% land ~3px apart otherwise).
        ends = sorted(((data[(key, 20000)][0], label)
                       for key, label, _, _, _, _ in SERIES), reverse=True)
        ys = [sy(v) for v, _ in ends]
        for i in range(1, len(ys)):
            if ys[i] - ys[i - 1] < 14:
                ys[i] = ys[i - 1] + 14
        for (v, label), y in zip(ends, ys):
            s.append(f'<text x="{sx(px,20000)+10:.1f}" y="{y+4:.1f}" font-size="11" fill="#0b0b0b">{label}</text>')
    return "\n".join(s)


def main():
    naive = compute(load("experiments/out/naive/timing.csv"))
    lazy = compute(load("experiments/out/timing.csv"))

    # ── accuracy gate ────────────────────────────────────────────
    for camp, data in (("naive", naive), ("lazy", lazy)):
        for key in ("full", "trace", "prov"):
            for size in SIZES:
                got = data[(key, size)][0]
                want = EXPECTED[camp][key][size]
                if abs(got - want) > 0.05:
                    raise SystemExit(
                        f"FIGURE/PAPER MISMATCH: {camp}/{key}/{size}: "
                        f"computed {got:.1f} vs published {want}")
    print("accuracy gate passed: all 24 medians match section 9 tables")
    for camp, data in (("naive", naive), ("lazy", lazy)):
        for key, label, _, _, _, _ in SERIES:
            row = "  ".join(f"{data[(key,s)][0]:6.1f} [{data[(key,s)][1]:.1f},{data[(key,s)][2]:.1f}]" for s in SIZES)
            print(f"{camp:5} {label:17} {row}")

    legend = []
    lx = PANELS_X[0]
    for i, (key, label, _, color, dash, shape) in enumerate(SERIES):
        x0 = lx + i * 190
        dash_attr = f' stroke-dasharray="{dash}"' if dash else ""
        legend.append(f'<line x1="{x0}" y1="{PANEL_Y-16}" x2="{x0+26}" y2="{PANEL_Y-16}" stroke="{color}" stroke-width="2"{dash_attr}/>')
        legend.append(marker(shape, x0 + 13, PANEL_Y - 16, color))
        legend.append(f'<text x="{x0+32}" y="{PANEL_Y-12}" font-size="12" fill="#0b0b0b">{label}</text>')

    svg = f'''<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 {W} {H}" font-family="Helvetica, Arial, sans-serif">
<rect width="{W}" height="{H}" fill="#fcfcfb"/>
<text x="{PANELS_X[0]-60}" y="{PANEL_Y+PANEL_H/2}" font-size="12" fill="#52514e" transform="rotate(-90 {PANELS_X[0]-60} {PANEL_Y+PANEL_H/2})" text-anchor="middle">overhead vs baseline (%)</text>
{"".join(legend)}
{panel(PANELS_X[0], "Naive recorder (eager detail strings)", naive, False)}
{panel(PANELS_X[1], "Lazy recorder (D14)", lazy, True)}
<text x="{PANELS_X[0]+PANEL_W/2}" y="{H-14}" font-size="12" fill="#52514e" text-anchor="middle">program size (lines, log scale)</text>
<text x="{PANELS_X[1]+PANEL_W/2}" y="{H-14}" font-size="12" fill="#52514e" text-anchor="middle">program size (lines, log scale)</text>
</svg>
'''
    with open("paper/figures/F7-overhead.svg", "w", newline="\n") as f:
        f.write(svg)
    print("wrote paper/figures/F7-overhead.svg")


if __name__ == "__main__":
    main()
