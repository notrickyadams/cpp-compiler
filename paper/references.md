# Reference registry — numbered, verification-tracked

RULES. (1) Only entries verified by live search during this project
carry a number; anything less sits in the debt section, unnumbered,
and MUST NOT be cited in drafts. (2) Numbers are provisional,
assigned by first use across drafted sections (§3 → §6 → §9, then
the §5 pool); mechanical renumbering happens at assembly (session F)
when all sections exist. (3) "Verified:" records what was actually
confirmed (title/venue/DOI vs. full bibliographic record) — partial
verification is stated, not hidden.

## Numbered references

[1] P. Denny, A. Luxton-Reilly, D. Carpenter. "Enhancing Syntax
    Error Messages Appears Ineffectual." ITiCSE 2014, pp. 273-278.
    DOI 10.1145/2591708.2591748.
    Verified: authors/title/venue/pages/DOI (2026-07-11).
    Cited in: §3, §9.3.

[2] R. Pettit, J. Homer, R. Gee. "Do Enhanced Compiler Error
    Messages Help Students? Results Inconclusive." SIGCSE 2017.
    DOI 10.1145/3017680.3017768.
    Verified: authors/title/venue/DOI (2026-07-11); pages at session E.
    Cited in: §3, §9.3.

[3] Rust RFC 1644, "Default and expanded rustc errors."
    https://rust-lang.github.io/rfcs/1644-default-and-expanded-rustc-errors.html
    Verified: URL/title (Phase 2). Type: design document.
    Cited in: §3, §6 D3.

[4] GHC development documentation: "Errors as (structured) values"
    (GHC wiki) and issue #18516 "Move to a more structured error
    representation." gitlab.haskell.org/ghc/ghc.
    Verified: URLs/titles (Phase 2). Type: development documentation.
    Cited in: §3, §6 D3/D4/D6.

[5] LLVM Project. "Remarks" (llvm.org/docs/Remarks.html) and Clang
    User's Manual, -fsave-optimization-record; llvm-opt-report.
    Verified: URLs/content (Phase 2). Type: official documentation.
    Cited in: §3 C3, §6 D11.

[6] L. Diekmann, L. Tratt. "Don't Panic! Better, Fewer, Syntax
    Errors for LR Parsers." ECOOP 2020, LIPIcs.
    DOI 10.4230/LIPIcs.ECOOP.2020.6 (also arXiv:1804.07133).
    Verified: authors/title/venue/DOI (Phase 2).
    Cited in: §6 D1, §9.3 E4.

[7] LLVM Project. "LLVM Programmer's Manual" — error handling:
    recoverable errors, Expected<T>/Error.
    https://llvm.org/docs/ProgrammersManual.html
    Verified: URL/content incl. recoverable-error definition
    (2026-07-05). Type: official documentation.
    Cited in: §6 D2.

[8] The Haskell Error Index. https://errors.haskell.org (error codes
    [GHC-12345] since GHC 9.6.1).
    Verified: URL/content (Phase 2). Type: community documentation.
    Cited in: §6 D3.

[9] Well-Typed. "The new GHC diagnostic infrastructure" (2021).
    https://well-typed.com/blog/2021/08/the-new-ghc-diagnostic-infrastructure/
    Verified: URL/title (Phase 2). Type: practitioner blog by the
    implementers — cite as implementation report.
    Cited in: §6 D4.

[10] Rust Compiler Development Guide, diagnostics chapter (structured
     suggestions API; Fluent-based translation) and UI-tests chapter
     (.stderr golden snapshots).
     rustc-dev-guide.rust-lang.org/diagnostics.html, /tests/ui.html
     Verified: URLs/content (Phase 2). Type: official documentation.
     Cited in: §6 D4, §5 (C4 concession).

[11] A. J. Ko, B. A. Myers. "Debugging Reinvented: Asking and
     Answering Why and Why Not Questions about Program Behavior."
     ICSE 2008. DOI 10.1145/1368088.1368130. (ICSE 2018 Most
     Influential Paper.)
     Verified: authors/title/venue/DOI (Phase 2).
     Cited in: §6 D7.

[12] The rustc book, "JSON Output."
     https://doc.rust-lang.org/rustc/json.html
     Verified: URL/content (Phase 2). Type: official documentation.
     Cited in: §6 D6.

[13] G. Marceau, K. Fisler, S. Krishnamurthi. "Measuring the
     Effectiveness of Error Messages Designed for Novice
     Programmers." SIGCSE 2011, pp. 499-504.
     DOI 10.1145/1953163.1953308. (Best Paper.)
     Verified: authors/title/venue/pages/DOI (2026-07-11).
     Cited in: §9.1/§9.3 (E3 instrument ancestry).

## §5 pool (verified, numbered on first use when §5 is drafted)

[14] B. A. Becker, P. Denny, R. Pettit, D. Bouchard, D. J. Bouvier,
     B. Harrington, A. Kamil, A. Karkare, C. McDonald, P.-M. Osera,
     J. L. Pearce, J. Prather. "Compiler Error Messages Considered
     Unhelpful: The Landscape of Text-Based Programming Error
     Message Research." ITiCSE-WGR 2019.
     DOI 10.1145/3344429.3372508.
     Verified: full author list/title/venue/DOI (Phase 2).

[15] T. Barik, J. Smith, K. Lubick, E. Holmes, J. Feng,
     E. Murphy-Hill, C. Parnin. "Do Developers Read Compiler Error
     Messages?" ICSE 2017. DOI 10.1109/ICSE.2017.59.
     Verified: authors/title/venue/DOI (Phase 2).

[16] P. Denny, J. Prather, B. A. Becker, C. Mooney, J. Homer,
     Z. C. Albrecht, G. B. Powell. "On Designing Programming Error
     Messages for Novices: Readability and its Constituent Factors."
     CHI 2021. DOI 10.1145/3411764.3445696.
     Verified: FULL RECORD (2026-07-13). NOTE: registry previously
     said "Becker et al." — Denny is first author; corrected.

[17] D. Zhang, A. C. Myers. "Toward General Diagnosis of Static
     Errors." POPL 2014, pp. 569-582. DOI 10.1145/2535838.2535870.
     Verified: authors/title/venue/pages/DOI (2026-07-11).

[18] D. Zhang, A. C. Myers, D. Vytiniotis, S. Peyton-Jones.
     "SHErrLoc: A Static Holistic Error Locator." ACM TOPLAS 39(4),
     Article 18, Aug 2017. DOI 10.1145/3121137.
     Verified: FULL RECORD (2026-07-13).

[19] D. Zhang, A. C. Myers, D. Vytiniotis, S. Peyton Jones.
     "Diagnosing Type Errors with Class." PLDI 2015.
     DOI 10.1145/2737924.2738009.
     Verified: FULL RECORD (2026-07-13); pages at BibTeX export.

[20] Z. Pavlinovic, T. King, T. Wies. "Practical SMT-Based Type
     Error Localization." ICFP 2015. DOI 10.1145/2784731.2784765.
     Verified: FULL RECORD (2026-07-13); pages at BibTeX export.

[21] A. J. Ko, B. A. Myers. "Extracting and Answering Why and Why
     Not Questions about Java Program Output." ACM TOSEM 20(2),
     Article 4, Aug 2010. DOI 10.1145/1824760.1824761.
     Verified: FULL RECORD (2026-07-13).

[22] F. J. Almeida-Martínez, J. Á. Velázquez-Iturbide (+ possibly
     J. Urquiza-Fuentes). "VAST: Visualization of Abstract Syntax
     Trees within Language Processors Courses." ACM SoftVis 2008.
     DOI 10.1145/1409720.1409759.
     Verified: two authors + title/venue/DOI (2026-07-13); exact
     2008 author list from the ACM landing page at BibTeX export
     (the third name is attributed on related papers).

[23] D. Rodríguez-Cerezo, M. Gómez-Albarrán, J.-L. Sierra-Rodríguez.
     "Interactive educational simulations for promoting the
     comprehension of basic compiler construction concepts."
     ITiCSE 2013. DOI 10.1145/2462476.2462498.
     Verified: FULL RECORD (2026-07-13).

[24] K. Dalbo, Y. Ahmed, H. Lim. "JITScope: Interactive
     Visualization of JIT Compiler IR Transformations."
     arXiv:2505.21599 (2025). Preprint — cite as such.
     Verified: FULL RECORD (2026-07-13).

[25] A. Taylor, A. Vassar, J. Renzella, H. Pearce. "dcc --help:
     Transforming the Role of the Compiler by Generating
     Context-Aware Error Explanations with Large Language Models."
     SIGCSE 2024. DOI 10.1145/3626252.3630822.
     Verified: FULL RECORD (2026-07-13).

[26] E. A. Santos, B. A. Becker. "Not the Silver Bullet:
     LLM-enhanced Programming Error Messages are Ineffective in
     Practice." UKICER 2024 (Best Paper). DOI 10.1145/3689535.3689554
     (also arXiv:2409.18661).
     Verified: FULL RECORD (2026-07-13). Extends the contested-
     effectiveness line into the LLM era.

[27] Clang project. "'Clang' CFE Internals Manual" — the Diagnostics
     subsystem: kinds defined in clang/Basic/Diagnostic*Kinds.td,
     TableGen-generated IDs/severities/format strings.
     https://clang.llvm.org/docs/InternalsManual.html
     Verified: URL/content incl. the .td mechanism (2026-07-13).
     Type: official documentation. Cited in: §6 D4, §10.1.

[28] K. Andrews, R. R. Henry, W. K. Yamamoto. "Design and
     Implementation of the UW Illustrated Compiler." PLDI 1988
     (SIGPLAN Notices 23), pp. 105-114. DOI 10.1145/960116.54001.
     Verified: authors/title/venue/pages/DOI (2026-07-13); the
     Notices issue number (6 vs 7) conflicts across sources — pin
     from the ACM landing page at BibTeX export. Companion: Henry &
     Whaley, "The University of Washington illustrating compiler,"
     SIGPLAN Notices 1990, DOI 10.1145/93548.93571 (title/DOI
     verified). 1988 ancestry for pipeline-state visualization —
     cite in §5 education cluster and §7.

[29] A. V. Aho, M. S. Lam, R. Sethi, J. D. Ullman. "Compilers:
     Principles, Techniques, and Tools," 2nd ed., Addison-Wesley,
     2006. ISBN 978-0321486813.
     Verified: FULL RECORD (2026-07-13). Cited in: §4 (background
     textbook pointer).

[30] "TraceDiff: Debugging Unexpected Code Behavior Using Trace
     Divergences." VL/HCC 2017.
     Verified: title/venue (2026-07-13 — C2 sweep). AUTHORS + DOI at
     BibTeX export. Adjacent work surfaced by the counterexample
     sweep: execution-trace tooling for STUDENT PROGRAMS (Whyline
     family), not the compiler's own diagnostic path — cite in §5
     interrogative-debugging cluster as the education-flavored
     neighbor.

## C2 counterexample sweep — PERFORMED 2026-07-13, negative result

Targeted searches for user-facing per-diagnostic compiler execution
traces (VL/HCC, ICSME/SANER framing + open queries). Nearest
neighbours found: TraceDiff [30] (traces of student programs, not
of the compiler); GCC's static-analyzer developer debug dumps
(internal tooling for analyzer developers, gcc.gnu.org docs — not
user-facing diagnostic structure; worth a defensive clause in §5).
No counterexample to C2. The §5 claim keeps its "to our knowledge"
form with this sweep as its recorded basis.

## Debt — remaining, small

- Elm "Compiler Errors for Humans" (Czaplicki, 2015, blog) — only if
  Elm re-enters §5; currently unneeded (§3 reworded).
- BibTeX-export items: pages for [2], [19], [20]; exact 2008 author
  list for [22]; Notices issue for [28]; authors + DOI for [30].
- Mechanical renumbering at assembly (session F) once §5 prose
  fixes first-use order.
