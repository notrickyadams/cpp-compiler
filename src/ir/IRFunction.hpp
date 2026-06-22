#pragma once
#include "IRInstruction.hpp"
#include <vector>
#include <string>

// ============================================================
//  IRFunction — one function lowered to a flat instruction list.
//  IRProgram  — every function in the compiled translation unit.
//
//  Note there is no separate "IRPrinter" Visitor here, unlike
//  ASTPrinter. The AST needed a Visitor because it's a nested
//  tree — printing it requires recursive traversal. IR is
//  ALREADY flat, so a plain loop + toString() per instruction
//  is the natural fit. Using a Visitor here would be needless
//  ceremony for a linear list.
// ============================================================
struct IRFunction {
    std::string                name;
    std::string                returnType;
    std::vector<std::string>   paramNames;
    std::vector<IRInstruction> instructions;

    std::string toString() const {
        std::string s = "function " + name + ":\n";
        for (const auto& instr : instructions) {
            s += "  " + instr.toString() + "\n";
        }
        return s;
    }
};

struct IRProgram {
    std::vector<IRFunction> functions;

    std::string toString() const {
        std::string s;
        for (const auto& fn : functions) s += fn.toString();
        return s;
    }
};
