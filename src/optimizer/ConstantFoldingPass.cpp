#include "ConstantFoldingPass.hpp"

bool ConstantFoldingPass::foldConstants(IROp op, int left, int right, int& result) {
    // Arithmetic is done in long long, then range-checked: folding
    // INT_MAX + INT_MAX with plain int would be signed overflow —
    // undefined behaviour in the COMPILER itself, not the compiled
    // program. long long comfortably holds any int×int product
    // (2^31 · 2^31 = 2^62 < 2^63), so no intermediate can overflow.
    const long long l = left;
    const long long r = right;
    long long res = 0;

    switch (op) {
        case IROp::Add: res = l + r; break;
        case IROp::Sub: res = l - r; break;
        case IROp::Mul: res = l * r; break;
        case IROp::Div:
            if (r == 0) return false;  // avoid UB — leave unfolded
            res = l / r;               // INT_MIN / -1 is fine in long long
            break;
        case IROp::Eq: res = (l == r) ? 1 : 0; break;
        case IROp::Ne: res = (l != r) ? 1 : 0; break;
        case IROp::Lt: res = (l <  r) ? 1 : 0; break;
        case IROp::Gt: res = (l >  r) ? 1 : 0; break;
        default: return false;
    }

    // A result outside int's range is left unfolded rather than wrapped:
    // the program has overflow UB either way, but the compiler must not
    // silently bake one particular wrapped value into the binary.
    if (res < -2147483648LL || res > 2147483647LL) return false;

    result = (int)res;
    return true;
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
