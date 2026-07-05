#!/usr/bin/env python3
"""
Single-fault mutant generator for the cascade study (E4).

Takes a valid seed program (in this compiler's language, which is
also valid C — int-only, no includes, no calls) and produces N
mutants, each differing from the seed by EXACTLY ONE token-level
edit. "Single fault by construction": the ground truth number of
programmer mistakes per mutant is 1, so every diagnostic beyond the
first that a compiler reports for a mutant is cascade noise. That
framing (and the panic-mode numbers motivating it) follows Diekmann
& Tratt, "Don't Panic!" (ECOOP 2020).

Operators (chosen uniformly, seeded):
  delete   — remove one token            (missing ';', missing operand, ...)
  dup      — duplicate one token         ("return x x;", "int int y;", ...)
  undecl   — replace one identifier USE with a fresh, undeclared name

Tokenization is a regex approximation of the language's lexer
(identifiers, integer literals, multi-char then single-char
operators, punctuation). That is fine for mutation purposes: every
produced text differs by one lexical token; whether it still parses
is for the compilers under study to decide. Mutants that happen to
remain valid programs (e.g. duplicating a '(' inside a comment —
impossible here, no comments in seeds; or deleting a redundant
token) are filtered out later by the scorer, which keeps only
mutants BOTH compilers reject.

Usage:
    python tools/gen_mutants.py --seed-file experiments/corpus/seed60.cpp \
        --count 100 --outdir experiments/mutants
"""
import argparse
import os
import random
import re

TOKEN_RE = re.compile(r"==|!=|[A-Za-z_][A-Za-z0-9_]*|\d+|[+\-*/=<>(){};,]")

KEYWORDS = {"int", "return"}


def tokenize_with_spans(text):
    return [(m.group(0), m.start(), m.end()) for m in TOKEN_RE.finditer(text)]


def mutate(text, rng):
    toks = tokenize_with_spans(text)
    if not toks:
        return None, None
    op = rng.choice(["delete", "dup", "undecl"])
    if op == "undecl":
        # USE positions only: an identifier whose PREVIOUS token is
        # 'int' is a declaration site (variable, parameter, or function
        # name). Corrupting a declaration makes every later use of the
        # old name a separate, GENUINE undeclared-identifier fault —
        # k legitimate diagnostics from one edit — which would poison
        # E4's premise that diagnostics beyond the first are cascade
        # noise. Restricting to uses keeps the ground truth at exactly
        # one fault per mutant.
        uses = []
        for i, (tok, s, e) in enumerate(toks):
            if not re.fullmatch(r"[A-Za-z_][A-Za-z0-9_]*", tok):
                continue
            if tok in KEYWORDS:
                continue
            if i > 0 and toks[i - 1][0] == "int":
                continue  # declaration site
            uses.append((tok, s, e))
        if not uses:
            return None, None
        tok, s, e = rng.choice(uses)
        return text[:s] + "zqx_undeclared" + text[e:], "undecl(%s)" % tok
    tok, s, e = rng.choice(toks)
    if op == "delete":
        return text[:s] + text[e:], "delete(%s)" % tok
    return text[:s] + tok + " " + tok + text[s + len(tok):], "dup(%s)" % tok


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--seed-file", required=True)
    ap.add_argument("--count", type=int, default=100)
    ap.add_argument("--outdir", required=True)
    ap.add_argument("--rng-seed", type=int, default=1234)
    args = ap.parse_args()

    with open(args.seed_file) as f:
        seed_text = f.read()
    os.makedirs(args.outdir, exist_ok=True)

    rng = random.Random(args.rng_seed)
    manifest = []
    made = 0
    attempts = 0
    while made < args.count and attempts < args.count * 20:
        attempts += 1
        mutant, desc = mutate(seed_text, rng)
        if mutant is None or mutant == seed_text:
            continue
        name = "m%03d.cpp" % made
        with open(os.path.join(args.outdir, name), "w", newline="\n") as f:
            f.write(mutant)
        manifest.append("%s,%s" % (name, desc))
        made += 1

    with open(os.path.join(args.outdir, "manifest.csv"), "w", newline="\n") as f:
        f.write("file,mutation\n")
        f.write("\n".join(manifest) + "\n")
    print("%d mutants -> %s (from %d attempts)" % (made, args.outdir, attempts))


if __name__ == "__main__":
    main()
