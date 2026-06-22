#pragma once
#include "OptimizationPass.hpp"
#include <unordered_map>

// ============================================================
//  CopyPropagationPass — replaces uses of a temp with its known
//  value, when that temp was defined by a simple Copy.
//
//      t0 = 5
//      t1 = t0 + 2     ->     t1 = 5 + 2
//
//  Restricted to TEMPORARIES only — never named variables. A
//  temp has exactly one static definition by construction (see
//  IRGenerator::newTemp()); a named variable could be reassigned
//  by a future language feature, which would make propagating
//  it unsound. This restriction is what makes the pass correct
//  without needing full liveness/def-use analysis.
//
//  Processes instructions top-to-bottom, updating the
//  substitution map as it goes — this is why a single call to
//  run() resolves chains like "t0=5; t1=t0; t2=t1+1" all the
//  way through to "t2 = 5 + 1" in one pass.
// ============================================================
class CopyPropagationPass : public OptimizationPass {
public:
    bool run(IRFunction& fn) override;
    std::string name() const override { return "CopyPropagation"; }

private:
    static IRValue substitute(const IRValue& v,
                               const std::unordered_map<int, IRValue>& subs);
};
