#include "ConstantFoldingPass.hpp"

bool ConstantFoldingPass::foldConstants(IROp op, int left, int right, int& result) {
    switch (op) {
        case IROp::Add: result = left + right; return true;
        case IROp::Sub: result = left - right; return true;
        case IROp::Mul: result = left * right; return true;
        case IROp::Div:
            if (right == 0) return false;  // avoid UB — leave unfolded
            result = left / right;
            return true;
        case IROp::Eq: result = (left == right) ? 1 : 0; return true;
        case IROp::Ne: result = (left != right) ? 1 : 0; return true;
        case IROp::Lt: result = (left <  right) ? 1 : 0; return true;
        case IROp::Gt: result = (left >  right) ? 1 : 0; return true;
        default: return false;
    }
}

bool ConstantFoldingPass::run(IRFunction& fn) {
    bool changed = false;

    for (auto& instr : fn.instructions) {
        // Only binary arithmetic/comparison instructions are foldable —
        // skip Copy (dest = single value, nothing to fold) and Return.
        if (instr.op == IROp::Copy || instr.op == IROp::Return) continue;
        if (!instr.hasSrc1 || !instr.hasSrc2) continue;
        if (instr.src1.kind != IRValueKind::Const) continue;
        if (instr.src2.kind != IRValueKind::Const) continue;

        int result;
        if (foldConstants(instr.op, instr.src1.constValue,
                                     instr.src2.constValue, result)) {
            IRValue dest = instr.dest;
            instr = IRInstruction::makeCopy(dest, IRValue::makeConst(result));
            changed = true;
        }
    }

    return changed;
}
