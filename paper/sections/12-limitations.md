# §12 Limitations — draft v1

Stated as facts about the artifact, each with its consequence.

1. **The curated fallback exists.** Diagnostics constructed through
   an engine with no recorder attached carry hand-written reference
   chains, not recorded ones. In the compiled pipeline this path is
   unreachable (verified by exhaustive search: three engines, three
   constructor-attached recorders, no other instantiation), but the
   dual source of truth remains a maintenance hazard bounded only by
   tests.
2. **Scope placement is selective.** Recorded traces are coarse to
   the granularity of the placement rule (D9); omission cannot
   fabricate a path, but a frame the rule skips is a frame the user
   never sees.
3. **Per-instruction provenance is not transitive** under dead-code
   elimination (§6.4): a folded instruction's history dies with it
   once propagation makes it dead. Locked in by tests as documented
   semantics; a dataflow-chained design is future work.
4. **Five diagnostic kinds are reserved-unreachable** — defined,
   fully worded, never emitted, because the language cannot yet
   produce them. Tests are forbidden from expecting them; counting
   them as supported would inflate the taxonomy by 29%.
5. **The cascade invariants are shape-specific.** Mutation testing
   measured the residual: 3% of single-fault mutants still produce
   three or more diagnostics (§9.3).
6. **Assembly provenance is comments, not debug info.** Readable by
   humans, invisible to debuggers; no DWARF/`.loc` emission.
7. **The knowledge base centralizes prose but implements no
   internationalization** — centralization makes i18n *possible*
   (one place to translate), and we did none of it.
8. **Single translation unit, no calls, no control flow.** The
   cascade and trace-depth results inherit the language's flatness;
   §10.2 discusses what deeper languages would demand.
9. **The recorder is single-threaded by construction** — one
   recorder per stage, no synchronization; a parallel front end
   would need per-thread recorders and a merge discipline.
10. **Windows-locked build and measurement.** The Makefile's shell
    commands, the harness, and all numbers are from one platform
    (§8.4, §11).

<!-- Working notes: numbered for citability from Discussion/
     Conclusion; each item ends in consequence, not apology.
     Item 4's 29% = 5/17. -->
