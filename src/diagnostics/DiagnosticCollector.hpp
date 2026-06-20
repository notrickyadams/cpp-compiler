#pragma once
#include "Diagnostic.hpp"
#include <vector>
#include <string>
#include <ostream>

// ============================================================
//  DiagnosticCollector — accumulates diagnostics across stages
//  and renders them as human-readable terminal output.
//
//  Responsibilities:
//    • add()    — accept a Diagnostic from any stage
//    • render() — produce formatted terminal output with:
//                   - source line extract + caret underline
//                   - WHY / FIX / TRACE sections
//
//  Ownership:
//    The collector owns its Diagnostic list. Caller owns the
//    source string (passed by reference to render, not stored).
//
//  Design note:
//    render() is kept separate from add() so the collector can
//    be used in non-terminal contexts (JSON export, LSP, etc.)
//    without modification.
// ============================================================
class DiagnosticCollector {
public:
    void add(Diagnostic d);
    void addAll(const std::vector<Diagnostic>& ds);

    bool hasErrors()   const;
    int  errorCount()  const;
    int  totalCount()  const { return (int)diagnostics_.size(); }

    const std::vector<Diagnostic>& all() const { return diagnostics_; }

    // ── Rendering ────────────────────────────────────────

    // Full multi-section render (WHY / FIX / TRACE)
    void render(std::ostream& out, const std::string& source,
                const std::string& filename = "<input>") const;

    // Compact one-line render (like traditional compiler output)
    void renderCompact(std::ostream& out,
                       const std::string& filename = "<input>") const;

    // JSON render (for the visualizer)
    void renderJson(std::ostream& out, const std::string& source) const;

private:
    std::vector<Diagnostic> diagnostics_;

    // Source line extraction helper
    static std::string extractLine(const std::string& source, int lineNum);

    // Underline string: "    ^~~~" under the error span
    static std::string makeCaret(int col, int length, char marker = '^');

    // Render a single diagnostic
    void renderOne(std::ostream& out,
                   const Diagnostic& d,
                   const std::string& source,
                   const std::string& filename) const;

    // Render the trace section
    static void renderTrace(std::ostream& out,
                             const std::vector<TraceStep>& trace);
};
