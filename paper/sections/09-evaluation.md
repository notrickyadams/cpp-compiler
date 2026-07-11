# §9 Evaluation — draft v1

<!-- Data sources: docs/experiments.md (protocol),
     docs/results-naive.md (eager-recorder E1/E2),
     experiments/out/results.md (lazy-recorder E1/E2 — ⚠ pending),
     experiments/out/quality.md (E3/E4), git show --stat 48b5054 (E5).
     Every number regenerates from the commands in docs/experiments.md. -->

## 9.1 Methodology

We evaluate three measurable questions from §3: the invasiveness of
the mechanisms (H1/E5), their always-on runtime and memory cost
(H3/E1/E2), and the structural-quality properties of the diagnostics
they enable (E3/E4). Human benefit is explicitly *not* evaluated
(§13 designs that study; §11 explains why the claim is withheld).

**Workload.** Deterministic, seeded, *valid* programs at 100 / 1,000
/ 5,000 / 20,000 lines (many 20–40-statement functions of mixed-depth
integer arithmetic). Valid programs are the fair worst case for this
architecture: every mechanism pays its cost and no diagnostic ever
redeems it.

**Measurement path.** `--check`: every stage through assembly text,
no JSON serialization, no toolchain subprocess, nothing printed on
success — the stopwatch sees the compiler and only the compiler. This
mode exists because our first campaign timed `--json` and measured
the harness instead (§9.2, "a discarded campaign").

**Design.** Factorial ablation over two compile-time flags
(recording, provenance), giving four binaries: FULL, NOTRACE,
NOPROV, BASELINE — each mechanism's cost is attributed separately.
Ablation is by macro so argument expressions vanish too; a no-op
class would understate the cost it exists to measure (§6.3, D13).

**Protocol.** Per (config, size): 3 warmups, 30 timed runs, 5
peak-working-set runs (sampled live; the counter is unreadable after
exit). Binaries built `-O2`. Statistics: median and MAD per cell —
wall times on a desktop OS are right-skewed by scheduler and
antivirus interference, and medians resist those spikes; overhead is
(median_config − median_baseline)/median_baseline with a 95%
bootstrap CI (10,000 resamples, percentile method, fixed seed).
Alternatives rejected: mean±SD assumes symmetry the data does not
have; comparing against production-compiler *speed* would be
meaningless at this scale, so all comparisons are self-vs-self.

**Environment.** Intel Core i7-8665U @ 1.90 GHz, 32 GiB RAM,
Windows 11 Pro (build 26200), GCC 6.3.0 (MinGW.org, 32-bit target).
Single machine, single toolchain: an external-validity limit stated
in §11, not an inconvenience to hide.

## 9.2 Overhead (E1/E2)

**A discarded campaign (methodological note).** Our first E1
collection timed `--json` with output piped through the measurement
shell. The numbers failed the coherence check we ran before accepting
any of them: ablated configurations measured *slower* than the full
build, orderings flipped between sizes, and "baseline" was nine
seconds for a 20k-line file through an O(n) pipeline. The harness was
ingesting megabytes of serialized pipeline state per run — we had
measured PowerShell. The dataset was deleted, the `--check` path
built, and the campaign rerun. We report this because measurement
pitfalls that are survived silently get repeated by readers.

**Naive recorder (eager detail strings) — the result that refuted
H3.** Median ± MAD, ms:

| lines | baseline | notrace | noprov | full |
|---|---|---|---|---|
| 100 | 46.9 ± 2.1 | 53.4 ± 2.5 | 58.3 ± 3.4 | 73.8 ± 4.1 |
| 1,000 | 154.8 ± 14.9 | 188.7 ± 8.8 | 278.4 ± 5.2 | 335.6 ± 11.4 |
| 5,000 | 570.9 ± 7.1 | 824.3 ± 48.3 | 1,569.9 ± 59.0 | 1,493.6 ± 53.5 |
| 20,000 | 2,266.0 ± 133.0 | 2,237.9 ± 67.5 | 5,498.2 ± 312.7 | 5,824.2 ± 221.1 |

At 20,000 lines — the startup-amortized row — trace recording cost
+143% [130, 168] (NOPROV) and the full configuration +157%
[144, 204]. The cause is identifiable, not mysterious: the lexer
opens two scopes per token, each eagerly building detail strings —
order 10⁵–10⁶ heap constructions per clean compile, all discarded.
This campaign also *appeared* to show provenance as free (NOTRACE vs
BASELINE: −1.2%, CI spanning zero) — a reading the interleaved
protocol below corrects; under blocked measurement, whichever config
runs nearest the baseline in time inherits flattering drift.

**A second protocol iteration.** Re-measuring the lazy recorder under
the same blocked protocol produced medians that *decrease
monotonically in run order* across configs (1890 → 1813 → 1532 →
1512 ms at 20k) — the machine sped up as the campaign progressed, and
with mechanism costs now small, that drift dominated the
between-config deltas. The final protocol therefore interleaves
configs round-robin within each run index, rotating cycle order to
cancel position bias. Blocked data is archived, not used.

**Lazy recorder (D14), interleaved protocol — the authoritative
table.** Median ± MAD, ms:

| lines | baseline | notrace | noprov | full |
|---|---|---|---|---|
| 100 | 24.1 ± 0.9 | 27.0 ± 1.9 | 24.4 ± 1.0 | 25.8 ± 0.8 |
| 1,000 | 78.6 ± 2.5 | 89.9 ± 2.6 | 79.9 ± 2.8 | 90.3 ± 2.5 |
| 5,000 | 398.4 ± 12.1 | 467.2 ± 11.0 | 402.7 ± 12.1 | 467.8 ± 10.2 |
| 20,000 | 1,549.5 ± 76.1 | 1,801.6 ± 76.5 | 1,585.8 ± 94.7 | 1,844.5 ± 94.2 |

Overhead vs baseline (95% bootstrap CI):

| lines | full (total) | trace only (NOPROV) | provenance only (NOTRACE) |
|---|---|---|---|
| 100 | 6.7% [2.2, 11.0] | 1.3% [−3.8, 4.0] | 11.9% [4.1, 15.5] |
| 1,000 | 14.9% [10.4, 17.6] | 1.6% [−2.1, 4.8] | 14.3% [10.2, 17.6] |
| 5,000 | 17.4% [13.9, 19.9] | 1.1% [−2.5, 3.2] | 17.2% [14.6, 20.0] |
| 20,000 | 19.0% [11.8, 28.3] | 2.3% [−5.5, 8.3] | 16.3% [8.6, 24.5] |

Three results, with their cross-campaign caveat stated first:
absolute times are not comparable between campaigns (baselines moved
with machine state across sessions); every comparison here is
*within* one internally-controlled campaign, and the naive-vs-lazy
arc compares relative overheads, each measured against its own
contemporaneous baseline.

(1) **Lazy recording is statistically free**: 1–2% at every size
with every CI spanning zero — down from +143%, a ~60× reduction,
achieved with no loss of output (the trace-content tests pin
byte-identical details and passed unchanged, 205/205).
(2) **Provenance costs a steady 12–17%** — the honest correction of
the blocked campaigns' contradictory readings (−1.2% and +19.9% were
both drift). The cost is structural and now grounded, not
conjectured: adding `span` + `note` grew `sizeof(IRInstruction)`
from 116 to 160 bytes (+38%; 20 bytes of span, 24 of MinGW's
`std::string`, measured), so every vector element, copy, and pass
traversal moves more data, and codegen additionally builds
provenance text per instruction. Reduction paths (a compressed span
representation; gating comment emission) are future work with a
target number attached, not hand-waving: the fields are
value-carried by design (D10), and that design's price is measured.
(3) At startup-amortized sizes the costs compose additively
(20k: 19.0 ≈ 2.3 + 16.3; 5k: 17.4 vs 18.3), consistent with
independent mechanisms; the 100-line row does *not* compose (6.7 vs
13.2 summed) — at 25 ms total runtime, process startup dominates and
the CIs are too wide for attribution, which is why the size ladder
exists. The 20k CIs are themselves wide ([11.8, 28.3] for FULL);
tightening them means more runs at the largest size, noted as a
cheap improvement rather than done, since no conclusion here turns
on the difference between 15% and 20%.

**Memory (E2).** Peak working set is unaffected throughout: ≤1.3%
delta at every size in every campaign (interleaved, 20k: 85.8 MiB
baseline → 86.9 MiB full). Open-frame storage and per-instruction
fields are memory-negligible; the cost of provenance is time, not
space.

## 9.3 Diagnostic quality (E3/E4)

**E3 — structural completeness (component presence, not quality).**
E3's epistemic role must be stated precisely: it is a structural
*inventory*, not a comparison study. That our output contains the
sections we designed is near-tautological; the informative content
is the contrast *shape* — which explanation components a
conventional pipeline emits at all — and the confirmation that no
component silently fails to render on any error class. Nine
single-error programs through both compilers on identical inputs;
mechanical detection rules only (the presence-rubric approach
descends from the error-message evaluation instrument of [13],
narrowed from judging responses to detecting components). Ours presents location,
cause, explanation prose, fix suggestions, trace, and caret on 9/9
(plus invalid/valid examples exactly on the two malformed-expression
cases, where the kind defines them); GCC 6.3.0 presents location,
cause, and caret on 9/9, and no note, fix-it, example, or trace on
any of the nine. Two scope statements keep this honest: the baseline
is the 2016 toolchain this compiler targets — no claim is made
against current GCC/Clang without running them — and presence is not
helpfulness; the enhanced-messages literature is contested on the
latter [1, 2], which is precisely why the paper measures the former.

**E4 — cascade behavior under single-fault mutation.** 100 mutants,
each exactly one token-level edit from a valid seed (delete /
duplicate / undeclared-substitution at use sites only — declaration
sites are excluded because corrupting a declaration creates k
*genuine* downstream faults and would poison the one-fault ground
truth). Error diagnostics per rejected mutant:

| compiler | rejected | accepted | distribution |
|---|---|---|---|
| ours | 100 | 0 | median 1, mean 1.32, max 5 (73×1, 24×2, 2×3, 1×5) |
| g++ 6.3 | 90 | 10 | median 1, mean 1.14, max 2 (77×1, 13×2) |

Findings, stated against ourselves first: (1) the shape-specific
cascade-freedom invariants do not generalize for free — 3% of mutants
still produce ≥3 diagnostics, so the tested-invariant approach is
necessary but not sufficient, and mutation testing is how the
residual frontier gets measured (the discovered shapes are enumerable
future fixes); (2) g++ is slightly tighter on this corpus; (3) the
accepted sets differ because the *languages* differ (C permits `;;`
and `{{`; this language does not), so the distributions are not over
identical mutant sets — an intersection-set statistic is listed as a
refinement in §13; (4) all 100 mutants derive from a single 78-line
seed, so mutation diversity is bounded by one program's token
population — a multi-seed corpus is the other §13 refinement, and no
generalization beyond this corpus is claimed. Framing: cascading error reporting is a
quantified problem in the recovery literature [6]; our contribution
here is the *property-test* treatment, not a recovery algorithm.

**E5 — invasiveness.** From the introducing commit's diff, compiler
source only: trace recording ≈200 lines (recorder + three stage
integrations + surfacing), provenance ≈115 lines (IR fields,
generator stamping, two pass-preservation sites, assembly comments,
JSON export) — ≈320 lines across 17 files, 5.6% of compiler source,
supporting H1's "localized mechanisms" claim with a number. The
diagnostics subsystem overall was 28.2% of compiler source at the
mechanism-introducing commit and 29.3% at the time of writing —
§8.1 traces the growth — the architecture's priorities, measured.

## 9.4 The test suite as evidence

205 tests (603 assertions) function as more than regression cover:
the trace-accuracy tests are the *evidence mechanism* for H2 (they
assert runtime facts no static chain could contain, and their
failure at the curated→recorded transition is documented evidence
the traces changed nature); the cascade tests pin the quality
invariants E4 then stress-tests; the byte-stability of golden IR
tests across the provenance change demonstrates the compatibility
cost D10 accepted; and the `--check` silence tests protect the
measurement channel every number in §9.2 depends on.

<!-- Working notes: wire F7 (overhead vs size, both recorders) from
     timing CSVs; environment line; lazy tables; then peer-review
     round for this section. -->
