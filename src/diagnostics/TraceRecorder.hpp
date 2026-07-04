#pragma once
#include "Diagnostic.hpp"
#include <string>
#include <vector>

// ============================================================
//  TraceRecorder — records the compiler's ACTUAL execution path
//  so diagnostics can report where the compiler really was when
//  an error fired, not a reconstruction of where it usually is.
//
//  How it works:
//    Each stage (Lexer, Parser, SemanticAnalyzer) owns one
//    recorder and attaches it to its DiagnosticEngine. Stage
//    methods open a TraceScope on entry; the scope pushes a
//    frame and pops it on exit (RAII, so early returns and
//    error-recovery paths stay balanced automatically).
//
//    When a diagnostic is created, DiagnosticEngine::build()
//    snapshots the frames that are open AT THAT MOMENT — the
//    genuine call path — and marks the deepest frame as the
//    failure point.
//
//  Why this exists (vs ExplanationBuilder::trace):
//    The knowledge-base traces are curated per error KIND — a
//    reference chain written by hand. They can never say which
//    function was being analysed or which token was being read,
//    because that only exists at runtime. A recorded trace can:
//
//      curated:   Parser::parseReturnStmt() -> ...
//      recorded:  Parser::parse()
//                 -> Parser::parseFunctionDecl()  [starting at line 1]
//                 -> Parser::parseReturnStmt()    [at 'return' (line 3)]
//                 -> ...
//
//    The curated chains remain as the fallback for diagnostics
//    constructed directly through DiagnosticEngine with no
//    recorder attached (unit tests, tooling), and their final
//    step (the engine factory method) is appended to recorded
//    traces so every chain still ends at the creation site.
//
//  Cost note:
//    push/pop builds a few small strings per scope even when no
//    diagnostic ever fires. At this project's scale that is
//    negligible; measuring it properly is listed as a planned
//    experiment (diagnostic-generation overhead).
// ============================================================
class TraceRecorder {
public:
    explicit TraceRecorder(std::string stage) : stage_(std::move(stage)) {}

    void push(const std::string& component, const std::string& detail) {
        TraceStep step;
        step.stage     = stage_;
        step.component = component;
        step.detail    = detail;
        step.ok        = true;
        frames_.push_back(std::move(step));
    }

    void pop() {
        if (!frames_.empty()) frames_.pop_back();
    }

    bool empty() const { return frames_.empty(); }

    // The frames currently open — i.e. the real call path at the
    // moment of the snapshot. The deepest frame is marked as the
    // failure point (it is where the diagnostic fired).
    std::vector<TraceStep> snapshot() const {
        std::vector<TraceStep> t = frames_;
        if (!t.empty()) t.back().ok = false;
        return t;
    }

private:
    std::string            stage_;
    std::vector<TraceStep> frames_;
};

// RAII guard: push on construction, pop on scope exit. Non-copyable
// so a frame can never be double-popped.
class TraceScope {
public:
    TraceScope(TraceRecorder& r, const std::string& component,
               const std::string& detail = "")
        : r_(r) {
        r_.push(component, detail);
    }
    ~TraceScope() { r_.pop(); }

    TraceScope(const TraceScope&)            = delete;
    TraceScope& operator=(const TraceScope&) = delete;

private:
    TraceRecorder& r_;
};
