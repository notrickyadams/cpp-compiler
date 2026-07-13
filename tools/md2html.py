#!/usr/bin/env python3
"""
Print-PDF assembly: paper/sections/*.md + paper/references.md
-> paper/preview/paper.html, styled for headless-Edge print-to-PDF.

This is the PREVIEW rendering (real content, academic print CSS,
numbered references resolved from the registry). Venue typography
(acmart two-column) remains the Overleaf build of paper/latex/.
Same source-of-truth discipline as md2tex.py: prose edits happen in
the .md files; this regenerates.
"""
import os
import re

ORDER = ["03-introduction", "04-background", "05-related-work",
         "06-explanation-architecture", "07-visualization",
         "08-implementation", "09-evaluation", "10-discussion",
         "11-threats", "12-limitations", "13-future-work",
         "14-conclusion"]

TITLE = ("Explanation as Architecture: Recorded Traces, Provenance, "
         "and Testable Diagnostics in Compiler Construction")
AUTHOR = "Lojan Essam ElDin Farouk"


def esc(t):
    return t.replace("&", "&amp;").replace("<", "&lt;").replace(">", "&gt;")


def inline(t):
    codes = []
    def stash(m):
        codes.append(m.group(1))
        return "\x00%d\x00" % (len(codes) - 1)
    t = re.sub(r"`([^`]+)`", stash, t)
    t = esc(t)
    t = re.sub(r"\*\*([^*]+)\*\*", r"<b>\1</b>", t)
    t = re.sub(r"(?<!\w)\*([^*\n]+)\*(?!\w)", r"<i>\1</i>", t)
    def cite(m):
        nums = [int(x) for x in re.split(r",\s*", m.group(1))]
        if not all(1 <= n <= 30 for n in nums):
            return m.group(0)
        return "&nbsp;[" + ", ".join(
            '<a href="#ref%d">%d</a>' % (n, n) for n in nums) + "]"
    t = re.sub(r"\[(\d+(?:,\s*\d+)*)\]", cite, t)
    for i, c in enumerate(codes):
        t = t.replace("\x00%d\x00" % i, "<code>" + esc(c) + "</code>")
    return t


def sec_to_html(md):
    md = re.sub(r"<!--.*?-->", "", md, flags=re.S)
    md = re.split(r"\n#+ Working notes.*", md)[0]
    lines = md.split("\n")
    out, i, first = [], 0, True
    para = []
    def flush():
        if para:
            out.append("<p>" + inline(" ".join(para)) + "</p>")
            para.clear()
    while i < len(lines):
        ln = lines[i]
        if ln.startswith("```"):
            flush(); block = []; i += 1
            while i < len(lines) and not lines[i].startswith("```"):
                block.append(lines[i]); i += 1
            out.append("<pre>" + esc("\n".join(block)) + "</pre>")
        elif ln.startswith("|") and i + 1 < len(lines) and \
                re.match(r"^\|[-\s|:]+\|?$", lines[i + 1].strip()):
            flush()
            rows = [ln]
            i += 1
            while i < len(lines) and lines[i].strip().startswith("|"):
                rows.append(lines[i]); i += 1
            i -= 1
            cells = [[c.strip() for c in r.strip().strip("|").split("|")]
                     for r in rows]
            html = ["<table>", "<tr>" + "".join(
                "<th>" + inline(c) + "</th>" for c in cells[0]) + "</tr>"]
            for r in cells[2:]:
                html.append("<tr>" + "".join(
                    "<td>" + inline(c) + "</td>" for c in r) + "</tr>")
            html.append("</table>")
            out.append("\n".join(html))
        elif ln.startswith("### "):
            flush(); out.append("<h4>" + inline(ln[4:]) + "</h4>")
        elif ln.startswith("## "):
            flush(); out.append("<h3>" + inline(ln[3:]) + "</h3>")
        elif ln.startswith("# "):
            flush()
            if first:
                t = re.sub(r"\s*[—-]+\s*draft.*$", "", ln[2:], flags=re.I)
                out.append("<h2>" + inline(t) + "</h2>")
                first = False
            else:
                out.append("<h3>" + inline(ln[2:]) + "</h3>")
        elif ln.startswith("- ") or (ln.startswith("  ") and out and
                                      para == [] and "<li>" in (out[-1] if out else "")):
            flush()
            items = []
            while i < len(lines) and (lines[i].startswith("- ") or
                                       (lines[i].startswith("  ") and items)):
                if lines[i].startswith("- "):
                    items.append(lines[i][2:])
                else:
                    items[-1] += " " + lines[i].strip()
                i += 1
            i -= 1
            out.append("<ul>" + "".join(
                "<li>" + inline(it) + "</li>" for it in items) + "</ul>")
        elif re.match(r"^\d+\.\s", ln):
            flush()
            items = []
            while i < len(lines) and (re.match(r"^\d+\.\s", lines[i]) or
                                       (lines[i].startswith("   ") and items)):
                if re.match(r"^\d+\.\s", lines[i]):
                    items.append(re.sub(r"^\d+\.\s", "", lines[i]))
                else:
                    items[-1] += " " + lines[i].strip()
                i += 1
            i -= 1
            out.append("<ol>" + "".join(
                "<li>" + inline(it) + "</li>" for it in items) + "</ol>")
        elif ln.strip() == "" or ln.strip() == "---":
            flush()
        else:
            para.append(ln.strip())
        i += 1
    flush()
    return "\n".join(out)


def abstract():
    md = open("paper/sections/02-title-abstract.md", encoding="utf-8").read()
    m = re.search(r"## Abstract[^\n]*\n\n(.*?)(\n<!--|\Z)", md, re.S)
    text = re.sub(r"\s+", " ", m.group(1)).strip()
    return inline(text)


def references():
    md = open("paper/references.md", encoding="utf-8").read()
    entries = re.findall(r"^\[(\d+)\]\s(.*?)(?=^\[\d+\]|\n## )", md,
                         re.S | re.M)
    items = []
    for n, body in entries:
        lines = [l.strip() for l in body.strip().split("\n")]
        keep = []
        for l in lines:
            if l.startswith(("Verified:", "Cited in:", "NOTE:")):
                break
            keep.append(l)
        items.append('<li id="ref%s">%s</li>' % (n, inline(" ".join(keep))))
    return "<ol class='refs'>" + "\n".join(items) + "</ol>"


F7_SVG = open("paper/figures/F7-overhead.svg", encoding="utf-8").read() \
    if os.path.exists("paper/figures/F7-overhead.svg") else ""

FIGS = {
    "03-introduction": """
<figure><pre class="fig">
ERROR TYPE:                LOCATION:                 TRACE:
Malformed Expression       line 3, column 14         Parser
                             3 |   return x 2;      -&gt; Parser::parse()
ROOT CAUSE:                    |            ^        -&gt; Parser::parseFunctionDecl()
Two expressions appeared                                  [starting at line 1]
consecutively with no      INVALID:  x 2             -&gt; Parser::parseBlock()
operator between them.     VALID:    x + 2           -&gt; Parser::parseStatement()
                                     x * 2                [at 'return' (line 3)]
WHAT HAPPENED:                       x = 2           -&gt; Parser::parseReturnStmt()
The parser successfully                                   [at 'return' (line 3)]
read: return x             HOW TO FIX:               -&gt; DiagnosticEngine::
After completing that      Insert an operator             malformedExpression()
expression, it encountered between x and 2                [diagnostic created]
another value: 2           or remove the extra value
C++ requires an operator
between values.
</pre><figcaption><b>Figure 1.</b> One diagnostic, verbatim (condensed
two-column arrangement of the CLI's sequential output). Every field is
data on one struct (&sect;6); the trace is recorded from the parser's
actual descent with runtime details in brackets.</figcaption></figure>""",
    "06-explanation-architecture": """
<figure><pre class="fig">
source (line 2):  return (2 + 3) * 4;

before        iteration 1           iteration 2          final
[2] t0 = 2+3  CF: t0 = 5            (t0 gone)
                 ;folded from
                  't0 = 2 + 3'
[2] t1 = t0*4 CP: t1 = 5 * 4        CF: t1 = 20          (t1 gone -- its
                 ;t0-&gt;5 (CopyProp)     ;t0-&gt;5; folded     fold AND prop
              DCE: t0 removed           from 't1 = 5*4'   notes die)
                  (fold note dies)  CP: return 20
[2] return t1     return t1            ;t1-&gt;20 (CopyProp)
                                    DCE: t1 removed      [2] return 20
                                                            ;t1-&gt;20 (CopyProp)
</pre><figcaption><b>Figure 2.</b> Provenance through the fixed point
for <code>(2+3)*4</code>: the source line survives every rewrite;
transformation notes die twice with DCE'd instructions. Per-instruction
provenance is honest but not transitive (&sect;12, item 3).
</figcaption></figure>""",
    "09-evaluation": """
<figure class="wide">""" + F7_SVG + """
<figcaption><b>Figure 3.</b> Always-on overhead vs the ablated
baseline, naive (left) and lazy (right) recorders, shared scale;
whiskers are 95% bootstrap CIs. Generated by a script that
hard-asserts every plotted median against the tables in &sect;9.
</figcaption></figure>""",
}

CSS = """
@page { size: letter; margin: 19mm 16mm; }
* { box-sizing: border-box; }
body { font: 9.5pt/1.38 Georgia, 'Times New Roman', serif;
       color: #111; margin: 0; }
.titleblock { column-span: all; text-align: center; margin: 0 0 6mm; }
h1 { font-size: 15.5pt; margin: 0 0 3mm; line-height: 1.25; }
.author { font-size: 10.5pt; margin: 0 0 1mm; }
.affil { font-size: 9pt; color: #444; margin-bottom: 5mm; }
.abstract { text-align: left; max-width: 88%; margin: 0 auto 2mm;
            font-size: 9pt; }
.abstract b { font-variant: small-caps; }
.body { column-count: 2; column-gap: 7mm; text-align: justify;
        hyphens: auto; }
h2 { font-size: 11.5pt; margin: 4mm 0 1.5mm; break-after: avoid; }
h3 { font-size: 10pt; margin: 3mm 0 1mm; break-after: avoid; }
h4 { font-size: 9.5pt; font-style: italic; margin: 2mm 0 0.5mm; }
p { margin: 0 0 1.6mm; }
code, pre { font-family: Consolas, 'Courier New', monospace;
            font-size: 8.2pt; }
pre { background: #f6f6f4; border: 0.3pt solid #ccc; padding: 2mm;
      overflow-x: hidden; white-space: pre-wrap; margin: 1.5mm 0; }
pre.fig { white-space: pre; font-size: 6.9pt; }
table { border-collapse: collapse; font-size: 8.2pt; margin: 2mm auto;
        width: 100%; }
th, td { border-top: 0.3pt solid #999; padding: 0.8mm 1.4mm;
         text-align: left; }
tr:first-child th { border-top: 1pt solid #111;
                    border-bottom: 0.5pt solid #111; }
tr:last-child td { border-bottom: 1pt solid #111; }
figure { margin: 2.5mm 0; break-inside: avoid; }
figure.wide { column-span: all; text-align: center; }
figure.wide svg { max-width: 96%; height: auto; }
figcaption { font-size: 8.3pt; margin-top: 1mm; text-align: justify; }
ul, ol { margin: 1mm 0 2mm; padding-left: 5mm; }
li { margin-bottom: 0.8mm; }
a { color: inherit; text-decoration: none; }
.refs { font-size: 8.2pt; }
.refs li { margin-bottom: 1mm; }
"""


def main():
    os.makedirs("paper/preview", exist_ok=True)
    body = []
    for stem in ORDER:
        with open("paper/sections/%s.md" % stem, encoding="utf-8") as f:
            body.append(sec_to_html(f.read()))
        if stem in FIGS:
            body.append(FIGS[stem])
    html = """<!DOCTYPE html><html><head><meta charset="utf-8">
<title>%s</title><style>%s</style></head><body>
<div class="titleblock"><h1>%s</h1>
<div class="author">%s</div>
<div class="affil">lojanessamfarouk@gmail.com</div>
<div class="abstract"><b>Abstract.</b> %s</div></div>
<div class="body">
%s
<h2>References</h2>
%s
</div></body></html>""" % (esc(TITLE), CSS, esc(TITLE), esc(AUTHOR),
                           abstract(), "\n".join(body), references())
    with open("paper/preview/paper.html", "w", encoding="utf-8",
              newline="\n") as f:
        f.write(html)
    print("wrote paper/preview/paper.html (%d sections, refs resolved)"
          % len(ORDER))


if __name__ == "__main__":
    main()
