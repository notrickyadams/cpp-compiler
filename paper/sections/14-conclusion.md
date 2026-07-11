# §14 Conclusion — draft v1

This work began with a compiler that detected a defect and told no
one — its analyzer logged the finding, its user got a green build
and a garbage exit code. The paper's answer is architectural: treat
explanation as an obligation of every stage, and make discarding
evidence a type error rather than a habit. In the resulting design,
every diagnostic is a structured value whose trace was recorded from
the compiler's actual execution (C2), assembled by a five-role core
whose taxonomy doubles as its explanation index (C1), over a
pipeline that carries source provenance all the way into emitted
assembly (C3), with the properties that make diagnostics trustworthy
enforced as executable tests and stress-measured by mutation (C4).

The evidence is a complete, zero-dependency reference compiler and a
measurement campaign that pushed back hard enough to be believed:
one dataset discarded for measuring the harness, one hypothesis
refuted — naive recording cost 2.4× — and repaired by a lazy design
to statistical insignificance, and one comfortable assumption
corrected, since provenance is not free but a steady, mechanically
explained 12–17%. The negative results — non-transitive provenance
under dead-code elimination, a 3% residual cascade the invariant
tests do not cover — are reported as findings because they delimit
what this architecture can promise.

What we hope a reader takes away is smaller and sharper than
"compilers should explain themselves": that the evidence for
explanation is present in every compiler at the moment of failure;
that retaining it is an architectural decision with a measurable,
modest price when made early; and that the difference between a
compiler that knew and a compiler that says is, concretely, about
three hundred lines of discipline.

<!-- Working notes: no new claims; "three hundred lines" = E5's
     ~320 (§8.2), rounded in prose with the exact figure two
     sections back. Deliberately short — half a column. -->
