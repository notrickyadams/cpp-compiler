#!/usr/bin/env python3
"""
Markdown-section -> LaTeX converter for the paper (stdlib only).

Deliberately narrow: it handles exactly the constructs the section
drafts use, and passes anything else through untouched for the
session-G manual pass. Reproducible assembly: prose edits happen in
paper/sections/*.md, then this regenerates paper/latex/sections/.

Handled: HTML working-note comments (stripped), #/##/### headers
(-> section/subsection/paragraph with stable labels), **bold**,
*emph*, `code`, [n]/[n, m] citations (-> \\cite{...} via the registry
map), markdown pipe tables (-> booktabs tabular), fenced code blocks
(-> verbatim), en/em dashes and the arrow/section glyphs, bullet
lists, and unicode used by the drafts.
"""
import os
import re

REFKEYS = {
    1: "denny2014enhancing", 2: "pettit2017enhanced", 3: "rust-rfc1644",
    4: "ghc-structured-errors", 5: "llvm-remarks", 6: "diekmann2020dont",
    7: "llvm-programmers-manual", 8: "haskell-error-index",
    9: "welltyped2021ghc", 10: "rustc-dev-guide", 11: "ko2008debugging",
    12: "rustc-json", 13: "marceau2011measuring", 14: "becker2019unhelpful",
    15: "barik2017developers", 16: "denny2021designing",
    17: "zhang2014general", 18: "zhang2017sherrloc",
    19: "zhang2015diagnosing", 20: "pavlinovic2015smt",
    21: "ko2010extracting", 22: "almeida2008vast",
    23: "rodriguez2013simulations", 24: "dalbo2025jitscope",
    25: "taylor2024dcc", 26: "santos2024silver", 27: "clang-internals",
    28: "andrews1988uw", 29: "aho2006compilers", 30: "suzuki2017tracediff",
}

SEC_LABELS = {
    "03": "sec:intro", "04": "sec:background", "05": "sec:related",
    "06": "sec:architecture", "07": "sec:visualizer", "08": "sec:impl",
    "09": "sec:eval", "10": "sec:discussion", "11": "sec:threats",
    "12": "sec:limitations", "13": "sec:future", "14": "sec:conclusion",
}


def esc(text):
    # order matters; backslash first
    text = text.replace("\\", r"\textbackslash{}")
    for ch, rep in (("&", r"\&"), ("%", r"\%"), ("$", r"\$"),
                    ("#", r"\#"), ("_", r"\_"), ("~", r"\textasciitilde{}"),
                    ("^", r"\textasciicircum{}")):
        text = text.replace(ch, rep)
    return text


def inline(text):
    # protect code spans before escaping
    codes = []
    def stash(m):
        codes.append(m.group(1))
        return "\x00%d\x00" % (len(codes) - 1)
    text = re.sub(r"`([^`]+)`", stash, text)
    text = esc(text)
    text = re.sub(r"\*\*([^*]+)\*\*", r"\\textbf{\1}", text)
    text = re.sub(r"(?<!\w)\*([^*\n]+)\*(?!\w)", r"\\emph{\1}", text)
    # citations [n] / [n, m] / [n, m, k] — but ONLY when every number
    # is a registry key: integer confidence intervals like [130, 168]
    # (section 6, D14) must pass through untouched. Decimal CIs never
    # match this pattern in the first place.
    def cite(m):
        nums = [int(x) for x in re.split(r",\s*", m.group(1))]
        if not all(n in REFKEYS for n in nums):
            return m.group(0)
        return r"\cite{" + ",".join(REFKEYS[n] for n in nums) + "}"
    text = re.sub(r"\[(\d+(?:,\s*\d+)*)\]", cite, text)
    # unicode used by drafts
    for ch, rep in (("—", "---"), ("–", "--"), ("→", r"$\rightarrow$"),
                    ("≈", r"$\approx$"), ("≥", r"$\geq$"), ("≤", r"$\leq$"),
                    ("×", r"$\times$"), ("±", r"$\pm$"), ("⁵", r"$^5$"),
                    ("⁶", r"$^6$"), ("¹", r"$^1$"), ("⁰", r"$^0$"),
                    ("−", "--"), ("†", r"$\dagger$"), ("✓", r"\checkmark{}"),
                    ("§§", "Sections~"), ("§", "\\S")):
        text = text.replace(ch, rep)
    for i, c in enumerate(codes):
        text = text.replace("\x00%d\x00" % i, r"\texttt{" + esc(c) + "}")
    return text


def table(lines):
    rows = [[c.strip() for c in ln.strip().strip("|").split("|")]
            for ln in lines]
    header, body = rows[0], rows[2:]
    ncol = len(header)
    out = [r"\begin{table}[t]", r"\centering", r"\small",
           r"\begin{tabular}{" + "l" * ncol + "}", r"\toprule",
           " & ".join(inline(c) for c in header) + r" \\", r"\midrule"]
    for r in body:
        r = (r + [""] * ncol)[:ncol]
        out.append(" & ".join(inline(c) for c in r) + r" \\")
    out += [r"\bottomrule", r"\end{tabular}", r"\end{table}"]
    return out


VERBATIM_ASCII = (("→", "->"), ("ε", "(empty)"), ("—", "--"),
                  ("–", "-"), ("⚠", "(!)"), ("≥", ">="), ("≤", "<="),
                  ("×", "x"), ("±", "+/-"), ("†", "+"), ("’", "'"))


def convert(md, stem):
    md = re.sub(r"<!--.*?-->", "", md, flags=re.S)  # strip HTML notes
    # Working notes as a markdown section (section 6 uses this form):
    # "stripped at submission" is enforced HERE, not by hope.
    md = re.split(r"\n#+ Working notes.*", md)[0]
    lines = md.split("\n")
    out = []
    i = 0
    first_header = True
    while i < len(lines):
        ln = lines[i]
        if ln.startswith("```"):
            block = []
            i += 1
            while i < len(lines) and not lines[i].startswith("```"):
                b = lines[i]
                # pdfLaTeX cannot take Unicode inside verbatim: map to
                # ASCII equivalents (the grammar's arrows/epsilon, the
                # listing comments' dashes).
                for ch, rep in VERBATIM_ASCII:
                    b = b.replace(ch, rep)
                block.append(b)
                i += 1
            out += [r"\begin{verbatim}"] + block + [r"\end{verbatim}"]
        elif ln.startswith("|") and i + 1 < len(lines) and \
                re.match(r"^\|[-\s|:]+\|?$", lines[i + 1].strip()):
            tbl = [ln]
            i += 1
            while i < len(lines) and lines[i].strip().startswith("|"):
                tbl.append(lines[i])
                i += 1
            i -= 1
            out += table(tbl)
        elif ln.startswith("### "):
            out.append(r"\paragraph{" + inline(ln[4:]) + "}")
        elif ln.startswith("## "):
            title = re.sub(r"^\d+(\.\d+)?\s*", "", ln[3:])
            out.append(r"\subsection{" + inline(title) + "}")
        elif ln.startswith("# "):
            if first_header:
                title = re.sub(r"^\S+\s*", "", ln[2:])
                title = re.sub(r"\s*---?\s*draft.*$", "", title, flags=re.I)
                lbl = SEC_LABELS.get(stem[:2], "sec:" + stem)
                out.append(r"\section{" + inline(title) + "}")
                out.append(r"\label{" + lbl + "}")
                first_header = False
            else:
                out.append(r"\subsection{" + inline(ln[2:]) + "}")
        elif ln.startswith("- "):
            items = []
            while i < len(lines) and (lines[i].startswith("- ") or
                                       lines[i].startswith("  ")):
                if lines[i].startswith("- "):
                    items.append(lines[i][2:])
                else:
                    items[-1] += " " + lines[i].strip()
                i += 1
            i -= 1
            out.append(r"\begin{itemize}")
            for it in items:
                out.append(r"\item " + inline(it))
            out.append(r"\end{itemize}")
        else:
            out.append(inline(ln))
        i += 1
    return "\n".join(out) + "\n"


def main():
    src = "paper/sections"
    dst = "paper/latex/sections"
    os.makedirs(dst, exist_ok=True)
    skip = {"02-title-abstract.md"}  # handled in main.tex directly
    for name in sorted(os.listdir(src)):
        if not name.endswith(".md") or name in skip:
            continue
        with open(os.path.join(src, name), encoding="utf-8") as f:
            tex = convert(f.read(), name[:-3])
        outname = name[:-3] + ".tex"
        with open(os.path.join(dst, outname), "w", encoding="utf-8",
                  newline="\n") as f:
            f.write(tex)
        print("converted", outname)


if __name__ == "__main__":
    main()
