#!/usr/bin/env python3
"""
Analysis for the overhead experiments (E1/E2) — stdlib only.

Reads experiments/out/timing.csv and memory.csv (produced by
tools/measure.ps1) and reports, per corpus size:

  * median wall time + MAD per config
  * overhead of each mechanism vs the ablated baseline:
        full     - baseline  -> total cost of explanation machinery
        noprov   - baseline  -> trace recording alone
        notrace  - baseline  -> provenance alone
    as a percentage of the baseline median, with a 95% bootstrap
    confidence interval (10,000 resamples of each group's runs,
    percentile method on the median-ratio statistic).
  * median peak working set per config.

Medians + MAD rather than mean + SD: wall-time distributions on a
desktop OS are right-skewed by scheduler/AV interference, and the
median is robust to those spikes. The bootstrap makes no normality
assumption for the interval either.

Usage:  python tools/analyze.py  [--resamples 10000]
Writes: experiments/out/results.md (and prints it)
"""
import argparse
import csv
import random
import statistics
from collections import defaultdict


def load(path, value_field, cast):
    rows = defaultdict(list)
    with open(path, newline="") as f:
        for row in csv.DictReader(f):
            rows[(row["config"], int(row["size"]))].append(cast(row[value_field]))
    return rows


def mad(xs):
    m = statistics.median(xs)
    return statistics.median([abs(x - m) for x in xs])


def bootstrap_overhead_ci(sample_a, sample_b, resamples, rng):
    """95% CI for (median(a) - median(b)) / median(b), percentile method."""
    stats = []
    for _ in range(resamples):
        ra = [rng.choice(sample_a) for _ in sample_a]
        rb = [rng.choice(sample_b) for _ in sample_b]
        mb = statistics.median(rb)
        if mb == 0:
            continue
        stats.append((statistics.median(ra) - mb) / mb * 100.0)
    stats.sort()
    lo = stats[int(0.025 * len(stats))]
    hi = stats[int(0.975 * len(stats))]
    return lo, hi


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--resamples", type=int, default=10000)
    args = ap.parse_args()
    rng = random.Random(42)  # fixed seed: the analysis itself is reproducible

    timing = load("experiments/out/timing.csv", "ms", float)
    memory = load("experiments/out/memory.csv", "peakBytes", int)

    sizes = sorted({s for (_, s) in timing})
    configs = ["baseline", "notrace", "noprov", "full"]
    lines = []
    w = lines.append

    w("# Overhead results (E1/E2)\n")
    w("Runs per cell: %d (timing). Statistic: median; spread: MAD; "
      "CIs: 95%% bootstrap (%d resamples), percentile method on the "
      "median-overhead ratio.\n" % (
          len(next(iter(timing.values()))), args.resamples))

    w("\n## E1 — wall time (ms)\n")
    w("| size (lines) | " + " | ".join(configs) + " |")
    w("|---|" + "---|" * len(configs))
    for s in sizes:
        cells = []
        for c in configs:
            xs = timing[(c, s)]
            cells.append("%.1f ± %.1f" % (statistics.median(xs), mad(xs)))
        w("| %d | %s |" % (s, " | ".join(cells)))

    w("\n## E1 — overhead vs baseline (%), 95% CI\n")
    w("| size (lines) | full (total) | noprov (= trace only) | notrace (= provenance only) |")
    w("|---|---|---|---|")
    for s in sizes:
        base = timing[("baseline", s)]
        row = ["%d" % s]
        for c in ("full", "noprov", "notrace"):
            xs = timing[(c, s)]
            pt = (statistics.median(xs) - statistics.median(base)) \
                 / statistics.median(base) * 100.0
            lo, hi = bootstrap_overhead_ci(xs, base, args.resamples, rng)
            row.append("%.1f%% [%.1f, %.1f]" % (pt, lo, hi))
        w("| " + " | ".join(row) + " |")

    w("\n## E2 — peak working set (MiB, median)\n")
    w("| size (lines) | " + " | ".join(configs) + " |")
    w("|---|" + "---|" * len(configs))
    for s in sizes:
        cells = []
        for c in configs:
            xs = memory[(c, s)]
            cells.append("%.2f" % (statistics.median(xs) / (1024.0 * 1024.0)))
        w("| %d | %s |" % (s, " | ".join(cells)))

    text = "\n".join(lines) + "\n"
    with open("experiments/out/results.md", "w", newline="\n") as f:
        f.write(text)
    print(text)


if __name__ == "__main__":
    main()
