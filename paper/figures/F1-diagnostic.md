# Figure F1 — one diagnostic carrying the full explanation contract

Placement: §3 (page 1–2). Caption draft below. Verbatim output of
`compiler.exe input.cpp --check` on the three-line program shown, at
the current head; regenerate before submission (command in
docs/experiments.md preamble). Annotation callouts (a)–(g) are added
typographically at assembly (session F); the anchors are listed here.

```
==============================================================
ERROR TYPE:                                            (a)
Malformed Expression

LOCATION:                                              (b)
line 3, column 14

    3 |     return x 2;
      |              ^

ROOT CAUSE:                                            (c)
Two expressions appeared consecutively with no operator between them.

WHAT HAPPENED:                                         (d)
The parser successfully read:

return x

After completing that expression,
it encountered another value:

2

C++ requires an operator between values.

INVALID:                                               (e)

x 2

VALID:

x + 2
x * 2
x = 2

HOW TO FIX:                                            (f)
Insert an operator between x and 2
or remove the extra value

TRACE:                                                 (g)
Parser
→ Parser::parse()
→ Parser::parseFunctionDecl()   [starting at line 1]
→ Parser::parseBlock()
→ Parser::parseStatement()   [at 'return' (line 3)]
→ Parser::parseReturnStmt()   [at 'return' (line 3)]
→ DiagnosticEngine::malformedExpression()   [diagnostic created]

Compilation result: 1 error(s)
```

Caption draft: *One diagnostic as emitted by the compiler for the
program at (b). Every element is a field of one structured value
(§6.2): (a) root-cause kind; (b) span with source extract; (c) the
kind's root-cause sentence; (d) explanation assembled from the
knowledge base and parameterized by what the parser actually
consumed; (e) invalid/valid examples synthesized from the offending
tokens; (f) ordered fixes; (g) the execution-recorded trace — each
frame was genuinely open when the diagnostic was created, with
runtime details (§6.3) captured lazily (D14).*

Verification: sections (a)–(f) byte-identical across ablation
configs (renderer is config-independent for non-trace content);
trace block captured from the default (full) build at session B —
an earlier capture accidentally used the leftover BASELINE binary
from the measurement campaign and showed the curated fallback,
which is why measure.ps1 now cleans build\ on exit.
