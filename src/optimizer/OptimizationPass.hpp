#pragma once
#include "../ir/IRFunction.hpp"
#include <string>

// ============================================================
//  OptimizationPass — the Strategy interface every pass implements.
//
//  Contract:
//    run(fn) mutates fn.instructions IN PLACE and returns true
//    if it changed anything. The Optimizer orchestrator uses
//    this return value to decide whether another iteration is
//    needed (see Optimizer.cpp — passes run to a FIXED POINT,
//    not just once, because one pass's output can create a new
//    opportunity for another pass).
//
//  Industry reference:
//    LLVM's FunctionPass / PassManager. Real pass managers also
//    track pass dependencies and invalidate analysis caches —
//    we skip that because our passes are cheap enough to always
//    rerun from scratch.
// ============================================================
class OptimizationPass {
public:
    virtual ~OptimizationPass() = default;
    virtual bool run(IRFunction& fn) = 0;
    virtual std::string name() const = 0;
};
