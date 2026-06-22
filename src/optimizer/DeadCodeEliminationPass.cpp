#include "DeadCodeEliminationPass.hpp"
#include <unordered_set>
#include <vector>

bool DeadCodeEliminationPass::run(IRFunction& fn) {
    // Step 1: collect every temp ID used as an operand anywhere
    // in the function (including in the final Return).
    std::unordered_set<int> usedTemps;
    for (const auto& instr : fn.instructions) {
        if (instr.hasSrc1 && instr.src1.kind == IRValueKind::Temp)
            usedTemps.insert(instr.src1.tempId);
        if (instr.hasSrc2 && instr.src2.kind == IRValueKind::Temp)
            usedTemps.insert(instr.src2.tempId);
    }

    // Step 2: keep everything except instructions defining an
    // unused temp.
    std::vector<IRInstruction> kept;
    kept.reserve(fn.instructions.size());
    bool changed = false;

    for (const auto& instr : fn.instructions) {
        bool isDeadTempDef = instr.hasDest &&
                              instr.dest.kind == IRValueKind::Temp &&
                              usedTemps.find(instr.dest.tempId) == usedTemps.end();
        if (isDeadTempDef) {
            changed = true;  // dropped — don't add to kept
        } else {
            kept.push_back(instr);
        }
    }

    fn.instructions = std::move(kept);
    return changed;
}
