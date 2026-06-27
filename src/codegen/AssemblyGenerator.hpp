#pragma once
#include "../ir/IRFunction.hpp"
#include "AssemblyProgram.hpp"
#include "StackFrameLayout.hpp"

// ============================================================
//  AssemblyGenerator — lowers optimized IR into 32-bit x86
//  AT&T-syntax assembly text.
//
//  Precondition: the IR was produced by IRGenerator from an AST
//  that already passed SemanticAnalyzer — same contract
//  IRGenerator itself documents. No Diagnostic ever originates
//  here; an unrecognised IROp is a compiler bug (assert), not a
//  user error.
//
//  No Visitor: IR is already a flat instruction list (see
//  IRFunction.hpp's own comment for why), so a straight loop
//  over instructions is the natural fit — the same flat-list
//  shape Optimizer already reuses for the same reason.
//
//  Target/ABI — validated empirically against the installed
//  toolchain BEFORE writing this class (see build/spike_test*.s
//  and build/probe*.c/.s, kept as scratch evidence of the
//  investigation):
//    - This MinGW.org GCC 6.3.0 toolchain is i686 (32-bit x86),
//      NOT x86-64 — x86-64 register names assemble fine but
//      target the wrong machine; 32-bit registers (%eax, %ebp,
//      %esp, ...) are the correct choice here.
//    - Every function gets symbol `_<name>` (leading underscore —
//      this toolchain's C symbol convention).
//    - The function named "main" is special-cased: its CRT
//      startup expects a symbol literally `_main`, and immediately
//      after the prologue it must `call ___main` (triple
//      underscore — runs C++ static initializers) with the stack
//      16-byte aligned first (`andl $-16, %esp`). Skipping this
//      does not just mis-optimize — it fails to LINK at all
//      ("undefined reference to WinMain@16"), because without a
//      `_main` symbol the CRT falls back to looking for a GUI
//      entry point instead.
//    - Parameters: standard cdecl — caller pushes arguments
//      right-to-left and cleans up the stack after the call;
//      callee reads them at 8(%ebp), 12(%ebp), ...
// ============================================================
class AssemblyGenerator {
public:
    AssemblyProgram generate(const IRProgram& program);

private:
    void generateFunction(const IRFunction& fn, AssemblyProgram& out);
    void generateInstruction(const IRInstruction& instr,
                              const StackFrameLayout& layout,
                              AssemblyProgram& out);

    // "$5" for a Const, or its stack slot ("-8(%ebp)") for Var/Temp.
    std::string operand(const IRValue& value, const StackFrameLayout& layout) const;

    // Emits `movl <operand>, %eax` — the universal "get this value
    // into the accumulator" step every instruction selection below
    // starts from.
    void loadIntoEax(const IRValue& value, const StackFrameLayout& layout,
                      AssemblyProgram& out);
};
