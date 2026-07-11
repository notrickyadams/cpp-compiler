# §1 Title + §2 Abstract — draft v1

## Title (decision + rationale)

**Explanation as Architecture: Recorded Traces, Provenance, and
Testable Diagnostics in Compiler Construction**

Rationale: the head names the thesis (explanation is an architectural
obligation, not a rendering concern); the subtitle enumerates the
three concrete mechanisms, which makes the paper searchable and keeps
the title honest — every noun in it is implemented and measured.
Rejected: "…in a Compiler Built to Explain Itself" (flourish edges
toward reflection/self-description claims we do not make) and
"Explainable Compilation" as head (collides with XAI vocabulary and
invites the wrong reviewer expectations — the abstract still defuses
this explicitly). User veto welcome; swap is one line.

## Abstract (~215 words)

Compilers routinely discard the evidence behind their own
diagnostics: the analysis knew which function it was checking, which
grammar rule it was inside, and which check failed, but what survives
to the user is a rendered string and a location. We present an
architecture that treats explanation as a first-class obligation of
every pipeline stage rather than a presentation-layer afterthought.
Every diagnostic is a structured artifact carrying a trace *recorded
from the compiler's actual execution*; all explanatory prose lives in
a root-cause-indexed knowledge base isolated from the stages; source
provenance survives lowering and optimization into the emitted
assembly; and diagnostic-quality properties — cascade-freedom, trace
accuracy — are enforced as executable tests. We validate the
architecture in a complete, zero-dependency compiler for a C-like
kernel language (~5.9K LOC, 205 tests) and measure its costs
factorially: naive always-on trace recording cost 2.4× compile time
and was repaired to statistical insignificance by lazy detail
formatting; provenance costs a steady 12–17%, attributable to
per-instruction metadata growth (116→160 bytes); peak memory is
unaffected. We report negative results as findings, including the
non-transitivity of per-instruction provenance under dead-code
elimination. The term "explanation" here denotes preserved compiler
evidence, not machine-learning explainability, and no claims about
human benefit are made without the user study we design but do not
run.

<!-- verification: every number cross-checked against §9 v1.1;
     "2.4x" = +143% total-recording row (NOPROV, naive, 20k);
     LOC: 5,943 at session B -> "~5.9K" (history: ~5.5K stated,
     corrected to 5,720/"~5.7K" at the 07-11 audit, re-drifted with
     the lazy recorder + --check; cross-section review 07-12 aligned
     abstract with sections 4 and 8); re-measure at submission. -->
