#pragma once
#include "../ir/IRFunction.hpp"
#include <vector>
#include <string>

// ============================================================
//  OptimizationReport — what changed, and how long it took to
//  converge. Exists for the portfolio "before/after" story and
//  for the future visualizer (Stage 8) to show pass-by-pass
//  progress, not just a final diff.
// ============================================================
struct OptimizationReport {
    std::string              functionName;
    int                       instructionsBefore = 0;
    int                       instructionsAfter  = 0;
    int                       iterations         = 0;
    std::vector<std::string> passesApplied;  // "iteration N: PassName"
};

// ============================================================
//  Optimizer — runs all passes on every function to a FIXED POINT.
//
//  "Fixed point" means: keep looping over [ConstantFolding,
//  CopyPropagation, DeadCodeElimination] until a full round
//  produces no change from any pass. This is required, not
//  optional — see ConstantFoldingPass.hpp's class comment and
//  the worked (2+3)*4 example in the Stage 5 write-up: folding
//  the inner expression creates a NEW foldable expression that
//  did not exist before that pass ran.
//
//  kMaxIterations is a safety bound, not a correctness
//  requirement — our straight-line (no loops/branches yet) IR
//  always converges in 2-4 iterations in practice.
// ============================================================
class Optimizer {
public:
    std::vector<OptimizationReport> optimize(IRProgram& program);

private:
    OptimizationReport optimizeFunction(IRFunction& fn);
};
