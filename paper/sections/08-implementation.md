# §8 Implementation — draft v1

## 8.1 Shape and size

The reference compiler is ~5,943 lines of C++14 (measured at session
B; re-measure at submission) with zero external dependencies — the
test framework, JSON emission, and visualization server are
hand-rolled or stdlib-only, so the artifact builds from a compiler
and Make alone. Distribution by subsystem:

| subsystem | LOC | | subsystem | LOC |
|---|---|---|---|---|
| diagnostics | 1,739 | | ast (4 visitors) | 500 |
| parser | 695 | | main/driver CLI | 499 |
| semantic | 604 | | lexer | 421 |
| ir | 463 | | codegen | 385 |
| optimizer | 370 | | core + toolchain | 267 |

The headline number is the first row: **the diagnostics subsystem is
the largest in the compiler — 29.3% of the source** — which is the
architecture's priority expressed as a measurement rather than a
slogan. (The share has grown across the project: 27% at the initial
audit, 28.2% after trace recording, 29.3% after lazy formatting.)

## 8.2 Mechanism invasiveness (E5)

From the introducing commit's diff, compiler source only: trace
recording cost ≈200 lines (the recorder, three stage integrations,
and surfacing through the renderers), and provenance ≈115 lines (IR
fields, generator stamping, preservation discipline in two passes,
assembly comments, JSON export) — ≈320 lines across 17 files, ~5.6%
of the then-current source, for both mechanisms. The lazy-formatting
repair (D14) later replaced the recorder's internals and touched the
20 scope call sites mechanically. H1's "localized mechanisms" claim
rests on these numbers, with the honest caveat that locality was
measured in a codebase designed to receive the mechanisms; §10
discusses what retrofit would change.

## 8.3 Testing as method

205 tests across nine suites (603 assertions) enforce three
distinct kinds of claim. Unit suites pin stage behavior. End-to-end
suites refuse to mock: generated assembly is assembled and linked
with the real system toolchain, the produced executable is *run*,
and its process exit code is asserted — if codegen is wrong, a real
binary misbehaves, not an abstraction. Property suites encode the
diagnostic-quality invariants of §6 and §9.4, including the
trace-accuracy tests whose runtime-fact assertions distinguish
recorded from curated traces, and the `--check` silence tests that
protect the measurement channel every number in §9 depends on.
There is no CI and no coverage measurement — stated as fact; the
suite runs on one platform, like everything else here.

## 8.4 Empirical construction on a legacy target

The target toolchain (GCC 6.3.0, MinGW.org, 32-bit) predates
reliable documentation for several of its contracts, so codegen
facts were established by probing rather than assumed: the CRT's
entry-point requirements (the `_main` symbol and `call ___main`
sequence) were derived by diffing real `gcc -S` output on probe
files after link failures whose messages pointed nowhere near the
cause. The same empiricism extended to the measurement harness,
where three Windows process-management quirks (a null `ExitCode`
without a cached handle; peak-memory counters unreadable after
exit; `CreateProcess` rejecting relative forward-slash paths)
each produced data that *looked* like compiler behavior until
reproduced in isolation — the §9.2 discarded-campaign note is the
largest instance of the same lesson. We record these because the
paper's numbers pass through this machinery, and a reader repeating
the experiments on Windows will meet the same artifacts.

## 8.5 Availability

The complete source, test suites, corpus generator, measurement and
analysis scripts, mutation tooling, raw result CSVs for every
campaign (including the discarded and superseded ones), and this
paper's working documents are in one public repository, tagged for
artifact evaluation at submission. Every number in §9 regenerates
from three documented commands.

<!-- Working notes: LOC table = session-B wc -l (core 183 + driver
     84 merged as 267); 29.3% = 1739/5943; share history 27/28.2/
     29.3 from audit + 48b5054 + session B. E5 numbers from git
     show --stat 48b5054 partition. "20 scope call sites" grep-
     verified. Tests 205/603 from last gate. No-CI statement true
     at drafting. -->
