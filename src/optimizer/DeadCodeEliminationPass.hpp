#pragma once
#include "OptimizationPass.hpp"

// ============================================================
//  DeadCodeEliminationPass — removes instructions whose
//  destination is a TEMPORARY that is never read afterward.
//
//  Scope: temporaries ONLY. We deliberately never remove an
//  instruction whose destination is a named variable, even if
//  that variable is seemingly never read again in this
//  function. Proving a named variable is truly dead requires
//  liveness analysis (is it read on any path after this point,
//  including paths we don't have yet — loops, branches, taking
//  its address). Temps need no such analysis: each is defined
//  exactly once and used only within the function that created
//  it, so "is tempId in the used-set" is a complete answer.
//
//  This is a natural extension point: adding liveness analysis
//  would let DCE also remove genuinely-unused variable writes.
// ============================================================
class DeadCodeEliminationPass : public OptimizationPass {
public:
    bool run(IRFunction& fn) override;
    std::string name() const override { return "DeadCodeElimination"; }
};
