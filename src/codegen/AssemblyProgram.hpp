#pragma once
#include <string>
#include <vector>

// ============================================================
//  AssemblyProgram — the final stage's output: a flat list of
//  assembly text lines.
//
//  Why not a structured AssemblyInstruction hierarchy (mirroring
//  IRInstruction)? Nothing downstream TRANSFORMS this output —
//  IR needed structure because the Optimizer rewrites it and
//  AssemblyGenerator reads its operands apart. Assembly text is
//  the end of the pipeline: an external assembler consumes it as
//  plain text. A vector<string> is exactly as much structure as
//  that job needs — anything richer would be ceremony with no
//  reader. (Same reasoning IRFunction.hpp gives for skipping a
//  Visitor: don't add structure nothing will use.)
//
//  Lines are kept separate (not pre-joined) so tests can assert
//  on individual instructions without string-splitting.
// ============================================================
struct AssemblyProgram {
    std::vector<std::string> lines;

    void emit(const std::string& line) {
        lines.push_back(line);
    }

    // Blank line — used to visually separate functions/sections.
    void emitBlank() {
        lines.push_back("");
    }

    std::string toString() const {
        std::string s;
        for (const auto& line : lines) {
            s += line;
            s += "\n";
        }
        return s;
    }
};
