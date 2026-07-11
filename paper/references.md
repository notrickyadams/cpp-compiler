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

[16] B. A. Becker et al. "On Designing Programming Error Messages
     for Novices: Readability and its Constituent Factors."
     CHI 2021. DOI 10.1145/3411764.3445696.
     Verified: title/venue/DOI (Phase 2); full author list session E.

[17] D. Zhang, A. C. Myers. "Toward General Diagnosis of Static
     Errors." POPL 2014, pp. 569-582. DOI 10.1145/2535838.2535870.
     Verified: authors/title/venue/pages/DOI (2026-07-11).

[18] D. Zhang, A. C. Myers, et al. "SHErrLoc: A Static Holistic
     Error Locator." ACM TOPLAS 39(4), 2017. DOI 10.1145/3121137.
     Verified: title/venue/DOI (Phase 2); full author list session E.

[19] "Diagnosing Type Errors with Class." PLDI 2015.
     DOI 10.1145/2737924.2738009.
     Verified: title/venue/DOI (Phase 2). AUTHORS NOT YET VERIFIED —
     confirm at session E before any author-name mention in prose.

[20] "Practical SMT-Based Type Error Localization." ICFP 2015.
     DOI 10.1145/2784731.2784765.
     Verified: title/venue/DOI (Phase 2). AUTHORS NOT YET VERIFIED.

[21] A. J. Ko, B. A. Myers. "Extracting and Answering Why and Why
     Not Questions about Java Program Output." ACM TOSEM, 2010.
     DOI 10.1145/1824760.1824761.
     Verified: authors/title/DOI (Phase 2); volume/issue session E.

[22] "VAST: Visualization of Abstract Syntax Trees within Language
     Processors Courses." ACM SoftVis 2008.
     DOI 10.1145/1409720.1409759.
     Verified: venue/DOI (Phase 2). Title wording + AUTHORS session E.

[23] "Interactive educational simulations for promoting the
     comprehension of basic compiler construction concepts."
     ITiCSE 2013. DOI 10.1145/2462476.2462498.
     Verified: title/venue/DOI (Phase 2). AUTHORS session E.

[24] "JITScope: Interactive Visualization of JIT Compiler IR
     Transformations." arXiv:2505.21599 (2025).
     Verified: title/identifier (Phase 2). AUTHORS session E.
     Preprint — cite as such.

[25] "dcc --help: Transforming the Role of the Compiler by Generating
     Context-Aware Error Explanations with Large Language Models."
     SIGCSE 2024. DOI 10.1145/3626252.3630822.
     Verified: title/venue/DOI (2026-07-11). AUTHORS session E.
     Candidate for §5's LLM-era paragraph.

[26] "Not the Silver Bullet: LLM-enhanced Programming Error Messages
     are Ineffective in Practice." UKICER 2024.
     DOI 10.1145/3689535.3689554.
     Verified: title/venue/DOI (2026-07-11). AUTHORS session E.
     Candidate: extends the contested-effectiveness line into the
     LLM era — strengthens our claim discipline.

## Debt — UNVERIFIED, unnumbered, must not be cited yet

- Clang Internals Manual / TableGen diagnostics (.td) — needed by
  §6 D4's Clang mention; marker "[ref: E-debt Clang]" in the draft.
- Elm "Compiler Errors for Humans" (Czaplicki, 2015, blog) — only if
  Elm re-enters §5; §3 was reworded to not require it.
- "UW illustrated compiler" — secondhand only; locate primary
  (likely a late-1980s SIGCSE/CACM item) or drop.
- Textbook citation for §4 pipeline background (Aho et al. "Dragon
  Book" edition TBD) — verify edition/ISBN at session E.
