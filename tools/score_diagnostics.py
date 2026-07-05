#!/usr/bin/env python3
"""
Diagnostic scoring for E3 (structural completeness) and E4 (cascade
behaviour). Stdlib only. Runs AFTER the timing campaign — it spawns
compilers, so running it concurrently would contaminate E1.

E3 — component presence, NOT quality.
    Nine fixed single-error programs (drawn from the project's
    20-case verification suite) go through this compiler and
    g++ -fsyntax-only. For each output we detect, MECHANICALLY,
    which explanation components are present. Detection rules are
    literal and listed below — there is no judging of wording, per
    the evidence discipline the paper commits to (component
    presence is claimable without a user study; helpfulness is
    not — cf. Denny et al. 2014, Pettit et al. 2017).

    Components and rules:
      location   ours: "LOCATION:" section     gcc: \d+:\d+: prefix
      cause      ours: "ROOT CAUSE:" section   gcc: error:/warning: line
      expl       ours: "WHAT HAPPENED:" sect.  gcc: "note:" line present
      fix        ours: "HOW TO FIX:" section   gcc: "did you mean" / fix-it
      example    ours: "INVALID:" section      gcc: (no equivalent)
      trace      ours: "TRACE:" section        gcc: (no equivalent)
      caret      ours: " | " gutter + ^        gcc: a line containing '^'

E4 — diagnostics per single fault.
    Every mutant (tools/gen_mutants.py) contains exactly one
    token-level edit. We count error diagnostics each compiler
    reports for it: ours = the N in "Compilation result: N
    error(s)"; gcc = lines matching "error:". Mutants that either
    compiler ACCEPTS are dropped from that compiler's distribution
    (a mutation can be harmless — e.g. duplicating a digit). The
    honest statistic is the distribution, not just the mean.

Usage:
    python tools/score_diagnostics.py \
        [--compiler experiments/bin/compiler_full.exe] \
        [--mutants experiments/mutants]
Writes experiments/out/quality.md (and prints it).
"""
import argparse
import os
import re
import statistics
import subprocess

E3_PROGRAMS = {
    "missing_operator":  'int main() {\n    int x = 5;\n    return x 2;\n}\n',
    "missing_semicolon": 'int main() {\n    int x = 5\n    return x;\n}\n',
    "undeclared":        'int main() {\n    return y + 1;\n}\n',
    "bad_assign_target": 'int main() {\n    return 5 = 2;\n}\n',
    "bare_return":       'int main() {\n    return;\n}\n',
    "missing_paren":     'int main() {\n    return (2 + 3;\n}\n',
    "extra_brace":       'int main() {\n    return 1;\n}}\n',
    "unknown_char":      'int main() {\n    return @;\n}\n',
    "vardecl_two_vals":  'int main() {\n    int x = 5 5;\n    return x;\n}\n',
}

OUR_COMPONENTS = {
    "location": "LOCATION:",
    "cause":    "ROOT CAUSE:",
    "expl":     "WHAT HAPPENED:",
    "fix":      "HOW TO FIX:",
    "example":  "INVALID:",
    "trace":    "TRACE:",
}


def run(cmd):
    p = subprocess.run(cmd, capture_output=True, text=True)
    return p.returncode, (p.stdout or "") + (p.stderr or "")


def score_ours(text):
    comps = {k: (marker in text) for k, marker in OUR_COMPONENTS.items()}
    comps["caret"] = bool(re.search(r"\| +\^", text))
    return comps


def score_gcc(text):
    return {
        "location": bool(re.search(r":\d+:\d+:", text)),
        "cause":    bool(re.search(r"(error|warning):", text)),
        "expl":     "note:" in text,
        "fix":      ("did you mean" in text) or ("fix-it" in text),
        "example":  False,
        "trace":    False,
        "caret":    any(ch.strip() == "^" or ch.strip().startswith("^")
                        for ch in text.splitlines()),
    }


def count_our_errors(text):
    m = re.search(r"Compilation result: (\d+) error", text)
    return int(m.group(1)) if m else 0


def count_gcc_errors(text):
    return len(re.findall(r"\berror:", text))


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--compiler", default="experiments/bin/compiler_full.exe")
    ap.add_argument("--gcc", default="g++")
    ap.add_argument("--mutants", default="experiments/mutants")
    args = ap.parse_args()

    # CreateProcess rejects RELATIVE FORWARD-SLASH executable paths
    # (WinError 2 with the file plainly present) — the Python-level
    # cousin of the cmd.exe "build/x.exe" quirk this project hit in
    # its C++ test helpers. Arguments are unaffected; only argv[0]
    # is resolved by CreateProcess. Absolutize it once.
    args.compiler = os.path.abspath(args.compiler)

    os.makedirs("experiments/out", exist_ok=True)
    tmp = "experiments/out/_e3_tmp.cpp"
    lines = []
    w = lines.append

    # ── E3 ────────────────────────────────────────────────────
    w("# Diagnostic quality results (E3/E4)\n")
    w("## E3 — component presence (mechanical detection; presence, not quality)\n")
    keys = ["location", "cause", "expl", "fix", "example", "trace", "caret"]
    w("| program | compiler | " + " | ".join(keys) + " |")
    w("|---|---|" + "---|" * len(keys))
    for name, src in sorted(E3_PROGRAMS.items()):
        with open(tmp, "w", newline="\n") as f:
            f.write(src)
        _, ours_out = run([args.compiler, tmp, "-o", "experiments/out/_e3_x.exe"])
        _, gcc_out = run([args.gcc, "-fsyntax-only", tmp])
        for label, comps in (("ours", score_ours(ours_out)),
                             ("g++",  score_gcc(gcc_out))):
            w("| %s | %s | %s |" % (
                name, label,
                " | ".join("Y" if comps[k] else "-" for k in keys)))

    # ── E4 ────────────────────────────────────────────────────
    w("\n## E4 — error diagnostics per single-fault mutant\n")
    ours_counts, gcc_counts = [], []
    accepted = {"ours": 0, "gcc": 0}
    manifest = os.path.join(args.mutants, "manifest.csv")
    if os.path.exists(manifest):
        files = sorted(f for f in os.listdir(args.mutants) if f.endswith(".cpp"))
        for fname in files:
            path = os.path.join(args.mutants, fname)
            _, ours_out = run([args.compiler, path, "-o", "experiments/out/_e4_x.exe"])
            n_ours = count_our_errors(ours_out)
            if n_ours == 0:
                accepted["ours"] += 1
            else:
                ours_counts.append(n_ours)
            _, gcc_out = run([args.gcc, "-fsyntax-only", path])
            n_gcc = count_gcc_errors(gcc_out)
            if n_gcc == 0:
                accepted["gcc"] += 1
            else:
                gcc_counts.append(n_gcc)

        def dist(xs):
            if not xs:
                return "n/a"
            from collections import Counter
            c = Counter(xs)
            spread = ", ".join("%dx%d" % (c[k], k) for k in sorted(c))
            return "median %.0f, mean %.2f, max %d  (%s)" % (
                statistics.median(xs), statistics.mean(xs), max(xs), spread)

        w("| compiler | rejected mutants | accepted (harmless edit) | errors per rejected mutant |")
        w("|---|---|---|---|")
        w("| ours | %d | %d | %s |" % (len(ours_counts), accepted["ours"], dist(ours_counts)))
        w("| g++  | %d | %d | %s |" % (len(gcc_counts), accepted["gcc"], dist(gcc_counts)))
    else:
        w("_no mutants found — run tools/gen_mutants.py first_")

    for f in ("experiments/out/_e3_tmp.cpp", "experiments/out/_e3_x.exe",
              "experiments/out/_e4_x.exe"):
        if os.path.exists(f):
            os.remove(f)

    text = "\n".join(lines) + "\n"
    with open("experiments/out/quality.md", "w", newline="\n") as f:
        f.write(text)
    print(text)


if __name__ == "__main__":
    main()
