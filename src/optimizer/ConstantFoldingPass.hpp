#pragma once
#include "OptimizationPass.hpp"

// ============================================================
//  ConstantFoldingPass — dest = CONST op CONST  ->  dest = RESULT
//
//  Only fires when BOTH operands are already literal constants
//  in THIS instruction. It deliberately does NOT look through
//  temporaries (e.g. "t1 = t0 * 4" where t0 happens to always be
//  5) — that is CopyPropagationPass's job. Keeping each pass
//  narrowly scoped is what makes the fixed-point loop in
//  Optimizer.cpp necessary AND sufficient: each pass is simple
//  and provably correct on its own; iterating them together
//  produces the compound effect.
// ============================================================
class ConstantFoldingPass : public OptimizationPass {
public:
    bool run(IRFunction& fn) override;
    std::string name() const override { return "ConstantFolding"; }

private:
    // Computes op(left, right). Returns false (declines to fold)
    // for divide-by-zero — folding it would require deciding on
    // a poison value or raising a diagnostic, both out of scope
    // for this pass. The instruction is left unfolded; a future
    // "divide by zero" diagnostic stage would catch it instead.
    static bool foldConstants(IROp op, int left, int right, int& result);
};
