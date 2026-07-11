# §10 Discussion — draft v1

## 10.1 Where the knowledge base stops scaling

The switch-indexed knowledge base (D4) is honest about its ceiling,
and we hit the first warning shot ourselves: the `detail`/`detail2`
parameterization ran out of arity the first time a diagnostic needed
three runtime values, and the factory works around it by re-deriving
one field (§6.2). At seventeen kinds the workaround is a documented
oddity; at hundreds it would be a code smell; at Clang's or Rust's
scale it is exactly why those projects hold messages in TableGen and
Fluent respectively [10, ref: E-debt Clang]. The pattern's
load-bearing property — one place where all prose lives, indexed by
root cause — survives that migration; what changes is the storage
(data files instead of a switch) and the parameterization (named
placeholders instead of positional details). We read our capacity
limit as evidence *for* the industrial designs, reached from the
opposite direction.

## 10.2 Trace depth in bigger languages

Our recorded traces are short because the language is flat: no
recursion in the grammar beyond expressions, no template
instantiation, no macro expansion. In a production front end the
active-stack snapshot could be hundreds of frames deep (consider a
C++ template error's instantiation stack — which production
compilers already summarize, badly or well, because they face the
same problem). The scope-placement rule (D9) generalizes — scopes
where diagnostics can fire, plus structural landmarks — but a
production adoption would need a *budget policy* the rule does not
yet have: depth windows, frame sampling, or collapse-repeated-frames
summarization. We flag this as the first question a scaling attempt
must answer, not as a solved problem.

## 10.3 Provenance's price in context

The 12–17% always-on cost (§9.2) looks alarming until placed against
practice: production compilers already carry per-instruction source
locations when building with debug info, and LLVM's remark
infrastructure [5] attaches transformation records opt-in. Our
number is what *always-on, value-carried* provenance costs in a
compiler whose instructions were previously 116 bytes of pure
computation — a worst-case framing, since any compiler with existing
debug-location machinery has already paid most of the memory-layout
bill. The measured reduction paths are concrete: packing the span
(20 bytes of four ints could be an interned 4-byte index), interning
note strings, or gating note construction the way recording gates
detail construction (D14 — the same fix, one field over). We did not
apply them because 12–17% at teaching scale does not obstruct the
thesis, and an optimization pass without a driving requirement is
how measurement artifacts get built.

## 10.4 Two observability modes, one architecture

The implementation carries both a *narrating* channel (the semantic
check log: one line per check performed, always on, educational) and
an *evidentiary* channel (recorded traces: silent until a diagnostic
fires). They answer different questions — "what is the compiler
doing?" versus "why did THIS happen?" — and their coexistence is not
redundancy but a division the architecture makes cheap: both are
consumers of the same stage-owned state. The duality suggests the
general shape of compiler observability: continuous narration for
learning contexts, on-demand evidence for diagnosis, one contract
underneath.

## 10.5 Retrofit versus designed-in

GHC's and Rust's structured-diagnostics migrations [4, 9, 3] are the
strongest external evidence this architecture addresses a real need
— and the strongest evidence about cost asymmetry. Their retrofits
are multi-year, multi-release migrations across entrenched
string-based call sites [4, 9]; our designed-in version cost ≈320
lines and 5.6% of a (small) codebase (§8.2). The honest generalization is not "designing
in is cheap" — our codebase was shaped to receive it — but
narrower: the *marginal* cost of the explanation contract is small
when the failure-handling architecture is decided early, and grows
with every string-typed diagnostic call site added before the
decision.

## 10.6 Evidence for the LLM era

Generated error explanations are being deployed now, with mixed
results [25, 26]. Whatever position one takes on wording generation,
an LLM explainer needs *grounding* — the actual construct, the
actual path, the actual transformation history — precisely the
evidence conventional pipelines discard and this architecture
preserves as structured values. We speculate, and mark it as
speculation, that recorded traces and provenance form a natural
retrieval substrate for grounded generation; nothing in this paper
tests that.

<!-- Working notes: 10.3's "already paid most of the memory bill"
     is an inference about debug-info builds, not a measurement —
     phrased as context, not claim. 10.6 kept to three sentences,
     explicitly speculative. All [n] per registry. -->
