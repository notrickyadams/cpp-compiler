#include "Optimizer.hpp"
#include "ConstantFoldingPass.hpp"
#include "CopyPropagationPass.hpp"
#include "DeadCodeEliminationPass.hpp"
#include <memory>

namespace {
constexpr int kMaxIterations = 16;
}

std::vector<OptimizationReport> Optimizer::optimize(IRProgram& program) {
    std::vector<OptimizationReport> reports;
    for (auto& fn : program.functions) {
        reports.push_back(optimizeFunction(fn));
    }
    return reports;
}

OptimizationReport Optimizer::optimizeFunction(IRFunction& fn) {
    OptimizationReport report;
    report.functionName       = fn.name;
    report.instructionsBefore = (int)fn.instructions.size();

    std::vector<std::unique_ptr<OptimizationPass>> passes;
    passes.push_back(std::make_unique<ConstantFoldingPass>());
    passes.push_back(std::make_unique<CopyPropagationPass>());
    passes.push_back(std::make_unique<DeadCodeEliminationPass>());

    int  iteration       = 0;
    bool changedThisRound = true;

    while (changedThisRound && iteration < kMaxIterations) {
        changedThisRound = false;
        ++iteration;

        for (auto& pass : passes) {
            if (pass->run(fn)) {
                changedThisRound = true;
                report.passesApplied.push_back(
                    "iteration " + std::to_string(iteration) + ": " + pass->name());
            }
        }
    }

    report.iterations        = iteration;
    report.instructionsAfter = (int)fn.instructions.size();
    return report;
}
