#include "CopyPropagationPass.hpp"

IRValue CopyPropagationPass::substitute(const IRValue& v,
                                         const std::unordered_map<int, IRValue>& subs) {
    if (v.kind != IRValueKind::Temp) return v;
    auto it = subs.find(v.tempId);
    return (it != subs.end()) ? it->second : v;
}

bool CopyPropagationPass::run(IRFunction& fn) {
    bool changed = false;
    std::unordered_map<int, IRValue> subs;  // tempId -> known simple value

    for (auto& instr : fn.instructions) {
        if (instr.hasSrc1) {
            IRValue updated = substitute(instr.src1, subs);
            if (updated != instr.src1) { instr.src1 = updated; changed = true; }
        }
        if (instr.hasSrc2) {
            IRValue updated = substitute(instr.src2, subs);
            if (updated != instr.src2) { instr.src2 = updated; changed = true; }
        }

        // Only a Copy into a TEMP destination registers a new
        // substitution — binary ops compute new values, they
        // don't alias an existing one, and Var destinations are
        // excluded per the class-level note above.
        if (instr.op == IROp::Copy && instr.hasDest &&
            instr.dest.kind == IRValueKind::Temp) {
            subs[instr.dest.tempId] = instr.src1;  // src1 already substituted above
        }
    }

    return changed;
}
