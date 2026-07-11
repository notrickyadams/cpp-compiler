# §11 Threats to Validity — draft v1

**Construct validity.** The central operationalization is stated in
bold because it bounds every quality claim: **component presence is
not helpfulness.** E3 detects whether explanation components exist,
by author-defined mechanical rules; whether they help anyone is a
human question this paper does not answer, and the literature warns
loudly against assuming it [1, 2]. Likewise "explanation" is
operationalized as preserved structure (spans, traces, provenance,
root causes) — a reader who means something richer by the word
should read our claims as being about the structure. The E4 cascade
metric assumes one token edit equals one fault; the mutation
operators were restricted (declaration sites excluded) specifically
to protect that assumption, but the assumption itself is a modeling
choice.

**Internal validity.** The ablation compares four *binaries*, so
config deltas fold in code-layout differences, not only mechanism
work; two observations bound the concern — the NOPROV-vs-BASELINE
deltas have CIs spanning zero at every size (a layout effect would
have to be conveniently near-null four times), and the provenance
delta has an independent structural witness (the +38% instruction
size, §9.2). Temporal drift dominated one whole campaign before the
interleaved protocol; interleaving spreads residual drift across
configs but cannot remove it. The bootstrap uses a fixed seed
(reproducibility over seed-sensitivity; re-running with other seeds
is one flag). The measurement harness itself produced four Windows-
specific artifacts before yielding trustworthy data (§8.4, §9.2) —
we found each by coherence checking, and cannot rule out a fifth we
did not trigger.

**External validity.** One tiny language, one implementation, one
machine, one OS, one 2016 toolchain. The corpus is author-generated,
arithmetic-heavy, and error-free by design (it measures always-on
cost, not diagnostic-path cost); the mutant population derives from
a single seed program; the E3 baseline is the GCC the compiler
targets, not current GCC or Clang. Every generalization beyond this
artifact is either explicitly argued in §10 or not made.

**Conclusion validity.** 30 runs per timing cell; medians with MAD;
bootstrap CIs reported wherever a comparison is made. The widest
intervals (20k, FULL: [11.8, 28.3]) are printed next to the claims
they qualify. Where a result contradicted physical sense —
orderings flipping, ablated configs "slower" — we discarded the
data and said so, rather than narrating it.

<!-- Working notes: internal-validity layout argument is the one
     genuinely new analysis here; keep the two bounding observations
     tight. All numbers cross-checked against §9 v1.1. -->
