#!/usr/bin/env python3
"""
Corpus generator for the overhead experiments (E1/E2).

Emits syntactically and semantically VALID programs in this
compiler's language (int-only, no calls, no control flow) at a
requested size, deterministically (fixed seed per size), so every
measurement run compiles byte-identical input.

Shape: many small functions (20-40 statements each) rather than one
giant one — mirrors real translation units, keeps stack frames small,
and exercises the per-function paths (temp-counter reset, frame
layout, optimizer fixed-point) proportionally to program size.

Statements reference earlier locals and the two parameters through
mixed-depth arithmetic expressions, so lexer/parser/semantic/IR work
all scale with the line count. Division is by non-zero constants only
(the language would accept /0 syntactically; the optimizer refuses to
fold it, which is fine, but runtime division never executes anyway —
corpus programs are compiled, not run).

Usage:
    python tools/gen_corpus.py --lines 5000 --out experiments/corpus/p5000.cpp
"""
import argparse
import random


def gen_function(idx, n_stmts, rng):
    lines = ["int f%d(int p0, int p1) {" % idx]
    names = ["p0", "p1"]
    for s in range(n_stmts):
        v = "v%d" % s
        a = rng.choice(names)
        b = rng.choice(names)
        op = rng.choice(["+", "-", "*", "+", "-"])  # bias away from *
        c = rng.randint(1, 9)
        # Three expression shapes of increasing nesting depth, chosen
        # pseudo-randomly so parse trees are not uniform.
        shape = rng.randint(0, 2)
        if shape == 0:
            expr = "%s %s %s + %d" % (a, op, b, c)
        elif shape == 1:
            expr = "(%s %s %d) * (%s + %d)" % (a, op, c, b, rng.randint(1, 9))
        else:
            expr = "((%s + %d) %s %s) / %d" % (a, c, op, b, rng.randint(1, 9))
        lines.append("    int %s = %s;" % (v, expr))
        names.append(v)
    lines.append("    return %s + %s;" % (names[-1], names[-2]))
    lines.append("}")
    lines.append("")
    return lines


def gen_program(target_lines, seed):
    rng = random.Random(seed)
    out = []
    fidx = 0
    while len(out) < target_lines - 5:
        fidx += 1
        out += gen_function(fidx, rng.randint(20, 40), rng)
    out += ["int main() {", "    int x = 1;", "    return x + 2;", "}", ""]
    return "\n".join(out)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--lines", type=int, required=True)
    ap.add_argument("--out", required=True)
    ap.add_argument("--seed", type=int, default=None,
                    help="defaults to --lines so each size is deterministic")
    args = ap.parse_args()
    seed = args.seed if args.seed is not None else args.lines
    text = gen_program(args.lines, seed)
    with open(args.out, "w", newline="\n") as f:
        f.write(text)
    print("%s: %d lines (seed %d)" % (args.out, text.count("\n"), seed))


if __name__ == "__main__":
    main()
