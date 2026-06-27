#pragma once
#include "../ir/IRFunction.hpp"
#include "../ir/IRValue.hpp"
#include <string>
#include <unordered_map>
#include <cassert>

// ============================================================
//  StackFrameLayout — assigns every local Var and Temp in ONE
//  IRFunction a fixed slot in its x86 cdecl stack frame, and
//  resolves parameters to the offsets the CALLER already placed
//  them at before the call.
//
//  Frame shape (32-bit cdecl, addresses grow downward from ebp):
//
//      12(%ebp)   <- 2nd parameter
//       8(%ebp)   <- 1st parameter      (caller-pushed, read-only)
//       4(%ebp)   <- return address
//       0(%ebp)   <- saved %ebp
//      -4(%ebp)   <- 1st local/temp
//      -8(%ebp)   <- 2nd local/temp     (callee-owned)
//
//  Every value gets exactly one 4-byte slot — this language has
//  only `int` so far, so there is no packing/alignment decision
//  to make yet (see AssemblyGenerator's class comment for the
//  toolchain this was validated against).
//
//  Why a separate class instead of folding this into
//  AssemblyGenerator? Single Responsibility: layout is a pure
//  function of one IRFunction's operands (no instruction-
//  selection knowledge needed), so it is independently testable —
//  exactly like SymbolTable is kept separate from SemanticAnalyzer.
// ============================================================
class StackFrameLayout {
public:
    explicit StackFrameLayout(const IRFunction& fn) {
        // Parameters live in the CALLER's frame. cdecl places the
        // first declared parameter at the lowest address above the
        // return address (validated against this toolchain in
        // build/probe_param.s: `a` at 8(%ebp), `b` at 12(%ebp)).
        int paramOffset = 8;
        for (const auto& name : fn.paramNames) {
            paramOffsets_[name] = paramOffset;
            paramOffset += 4;
        }

        // Locals and temps: walk every instruction in order, handing
        // out the next free slot the FIRST time each new Var/Temp is
        // seen as an operand (dest, src1, or src2 — a value can be
        // read before it's ever a dest, e.g. a parameter used in an
        // expression).
        for (const auto& instr : fn.instructions) {
            if (instr.hasDest) visit(instr.dest);
            if (instr.hasSrc1) visit(instr.src1);
            if (instr.hasSrc2) visit(instr.src2);
        }
    }

    // "N(%ebp)" or "-N(%ebp)" for a Var or Temp operand.
    // Constants are never given a slot — they are emitted as
    // immediates ("$N") by the caller, which must branch on
    // IRValue::kind before reaching here.
    std::string slotOf(const IRValue& value) const {
        if (value.kind == IRValueKind::Var) {
            auto p = paramOffsets_.find(value.varName);
            if (p != paramOffsets_.end())
                return std::to_string(p->second) + "(%ebp)";

            auto l = localOffsets_.find(value.varName);
            assert(l != localOffsets_.end() &&
                   "StackFrameLayout: Var seen at use that the "
                   "constructor's instruction scan never assigned");
            return std::to_string(l->second) + "(%ebp)";
        }

        assert(value.kind == IRValueKind::Temp &&
               "StackFrameLayout::slotOf() called on a Const — "
               "constants have no stack slot, emit an immediate instead");
        auto t = tempOffsets_.find(value.tempId);
        assert(t != tempOffsets_.end() &&
               "StackFrameLayout: Temp seen at use that the "
               "constructor's instruction scan never assigned");
        return std::to_string(t->second) + "(%ebp)";
    }

    // Bytes of LOCAL stack space to reserve via `subl` in the
    // prologue. Parameters are excluded — those already live in
    // the caller's frame.
    int frameSize() const { return frameSize_; }

private:
    void visit(const IRValue& v) {
        if (v.kind == IRValueKind::Var) {
            if (paramOffsets_.count(v.varName)) return;  // it's a parameter
            if (localOffsets_.count(v.varName)) return;  // already assigned
            localOffsets_[v.varName] = nextOffset_;
            nextOffset_ -= 4;
            frameSize_ += 4;
        } else if (v.kind == IRValueKind::Temp) {
            if (tempOffsets_.count(v.tempId)) return;
            tempOffsets_[v.tempId] = nextOffset_;
            nextOffset_ -= 4;
            frameSize_ += 4;
        }
        // Const: no slot needed.
    }

    std::unordered_map<std::string, int> paramOffsets_;  // name -> +offset
    std::unordered_map<std::string, int> localOffsets_;  // name -> -offset
    std::unordered_map<int, int>         tempOffsets_;   // id   -> -offset
    int nextOffset_ = -4;
    int frameSize_  = 0;
};
