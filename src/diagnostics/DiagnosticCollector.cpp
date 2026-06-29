#include "DiagnosticCollector.hpp"
#include "../core/Json.hpp"
#include <iostream>
#include <sstream>
#include <algorithm>
#include <cctype>

// ─────────────────────────────────────────────────────────────
//  toSentence — cosmetic only: capitalise the first letter and
//  ensure a trailing period, for the ROOT CAUSE line. The actual
//  strings returned by ExplanationBuilder stay lowercase/period-free
//  (renderJson() and direct callers rely on that exact text) — this
//  normalises purely at the point of human-readable text display.
// ─────────────────────────────────────────────────────────────
namespace {
std::string toSentence(const std::string& s) {
    if (s.empty()) return s;
    std::string out = s;
    out[0] = static_cast<char>(std::toupper(static_cast<unsigned char>(out[0])));
    char last = out.back();
    if (last != '.' && last != '!' && last != '?') out += '.';
    return out;
}
}  // namespace

// ─────────────────────────────────────────────────────────────
//  Collection
// ─────────────────────────────────────────────────────────────

void DiagnosticCollector::add(Diagnostic d) {
    diagnostics_.push_back(std::move(d));
}

void DiagnosticCollector::addAll(const std::vector<Diagnostic>& ds) {
    for (const auto& d : ds) diagnostics_.push_back(d);
}

bool DiagnosticCollector::hasErrors() const {
    for (const auto& d : diagnostics_) if (d.isError()) return true;
    return false;
}

int DiagnosticCollector::errorCount() const {
    int n = 0;
    for (const auto& d : diagnostics_) if (d.isError()) ++n;
    return n;
}

// ─────────────────────────────────────────────────────────────
//  Source helpers
// ─────────────────────────────────────────────────────────────

std::string DiagnosticCollector::extractLine(const std::string& source,
                                              int lineNum) {
    int line = 1;
    std::string result;
    for (std::size_t i = 0; i < source.size(); ++i) {
        if (line == lineNum) {
            if (source[i] == '\n') break;
            result += source[i];
        }
        if (source[i] == '\n') ++line;
    }
    return result;
}

// col is 1-based; we output (col-1) spaces then marker + tildes
std::string DiagnosticCollector::makeCaret(int col, int length, char marker) {
    std::string s(col - 1, ' ');
    if (length <= 0) length = 1;
    s += marker;
    if (length > 1) s += std::string(length - 1, '~');
    return s;
}

// ─────────────────────────────────────────────────────────────
//  Full render (the flagship output)
//
//  Visual format — one named section per line of inquiry, in the
//  order a developer actually asks them: what is it, where is it,
//  why did it happen, what did the compiler see, what should it
//  have seen, how do I fix it, how did the compiler get here.
//
//  ============================================================
//  ERROR TYPE:
//  Malformed Expression
//
//  LOCATION:
//  line 3, column 15
//
//      3 | return x 2;
//        |          ^
//
//  ROOT CAUSE:
//  Two expressions appeared consecutively with no operator.
//
//  WHAT HAPPENED:
//  The parser successfully read:
//
//  return x
//
//  After completing the return expression,
//  it encountered another value:
//
//  2
//
//  C++ requires an operator between values.
//
//  INVALID:
//
//  x 2
//
//  VALID:
//
//  x + 2
//  x * 2
//  x = 2
//
//  HOW TO FIX:
//  Insert an operator between x and 2
//  or remove the extra value
//
//  TRACE:
//  Parser
//  → Parser::parseReturnStmt()
//  → Parser::isExpressionStart()
//  → DiagnosticEngine::malformedExpression()
//
//  INVALID/VALID only appear when the Diagnostic actually carries
//  example text (Diagnostic::invalidExample non-empty) — most kinds
//  leave WHAT HAPPENED/ROOT CAUSE/HOW TO FIX/TRACE to carry the full
//  explanation and skip the code-comparison sections entirely.
// ─────────────────────────────────────────────────────────────

void DiagnosticCollector::render(std::ostream& out,
                                  const std::string& source,
                                  const std::string& filename) const {
    for (const auto& d : diagnostics_) {
        renderOne(out, d, source, filename);
        out << "\n";
    }

    if (!diagnostics_.empty()) {
        int errs  = errorCount();
        int warns = (int)diagnostics_.size() - errs;
        out << "Compilation result: ";
        if (errs > 0)  out << errs  << " error(s)";
        if (warns > 0) out << (errs ? ", " : "") << warns << " warning(s)";
        out << "\n";
    }
}

void DiagnosticCollector::renderOne(std::ostream& out,
                                     const Diagnostic& d,
                                     const std::string& source,
                                     const std::string& /*filename*/) const {
    const std::string sep(62, '=');

    out << sep << "\n";

    // ── ERROR TYPE ───────────────────────────────────────
    out << "ERROR TYPE:\n" << diagnosticKindDisplayName(d.kind) << "\n\n";

    // ── LOCATION (+ source extract/caret) ─────────────────
    out << "LOCATION:\n";
    out << "line " << d.span.startLine << ", column " << d.span.startCol << "\n";
    if (!source.empty()) {
        std::string line    = extractLine(source, d.span.startLine);
        std::string lineNum = std::to_string(d.span.startLine);
        std::string caret   = makeCaret(d.span.startCol, std::max(1, d.span.length));
        out << "\n    " << lineNum << " | " << line << "\n";
        out << "    " << std::string(lineNum.size(), ' ') << " | " << caret << "\n";
    }
    out << "\n";

    // ── ROOT CAUSE (one sentence) ─────────────────────────
    out << "ROOT CAUSE:\n" << toSentence(d.rootCause) << "\n\n";

    // ── WHAT HAPPENED (the WHY, may be multi-paragraph) ───
    out << "WHAT HAPPENED:\n";
    std::istringstream ss(d.explanation);
    std::string lineText;
    while (std::getline(ss, lineText)) {
        out << lineText << "\n";
    }
    out << "\n";

    // ── INVALID / VALID (only when populated) ─────────────
    if (!d.invalidExample.empty()) {
        out << "INVALID:\n\n" << d.invalidExample << "\n\n";
    }
    if (!d.validExamples.empty()) {
        out << "VALID:\n\n";
        for (const auto& v : d.validExamples) out << v << "\n";
        out << "\n";
    }

    // ── HOW TO FIX ─────────────────────────────────────────
    if (!d.fixes.empty()) {
        out << "HOW TO FIX:\n";
        for (const auto& f : d.fixes) out << f.description << "\n";
        out << "\n";
    }

    // ── TRACE (arrow chain, no [ok]/[FAIL] brackets) ──────
    if (!d.trace.empty()) {
        out << "TRACE:\n";
        renderTrace(out, d.trace);
    }
}

void DiagnosticCollector::renderTrace(std::ostream& out,
                                       const std::vector<TraceStep>& trace) {
    if (trace.empty()) return;
    out << trace.front().stage << "\n";
    for (const auto& step : trace) {
        out << "→ " << step.component << "\n";
    }
}

// ─────────────────────────────────────────────────────────────
//  Compact render (traditional one-line style)
// ─────────────────────────────────────────────────────────────

void DiagnosticCollector::renderCompact(std::ostream& out,
                                         const std::string& filename) const {
    for (const auto& d : diagnostics_) {
        out << filename << ":"
            << d.span.startLine << ":" << d.span.startCol << ": "
            << severityName(d.severity) << ": "
            << d.message << "\n";
    }
}

// ─────────────────────────────────────────────────────────────
//  JSON render (for the Stage 8 visualizer — jsonEscape() lives
//  in core/Json.hpp now, shared with every other stage that
//  feeds the visualizer's JSON export)
// ─────────────────────────────────────────────────────────────

void DiagnosticCollector::renderJson(std::ostream& out,
                                      const std::string& /*source*/) const {
    out << "{\n  \"diagnostics\": [\n";
    for (std::size_t i = 0; i < diagnostics_.size(); ++i) {
        const auto& d = diagnostics_[i];
        out << "    {\n";
        out << "      \"kind\": \""     << jsonEscape(diagnosticKindName(d.kind)) << "\",\n";
        out << "      \"severity\": \"" << jsonEscape(severityName(d.severity))   << "\",\n";
        out << "      \"span\": { \"startLine\": " << d.span.startLine
            << ", \"startCol\": " << d.span.startCol
            << ", \"endLine\": "  << d.span.endLine
            << ", \"endCol\": "   << d.span.endCol  << " },\n";
        out << "      \"message\": \""     << jsonEscape(d.message)     << "\",\n";
        out << "      \"rootCause\": \""   << jsonEscape(d.rootCause)   << "\",\n";
        out << "      \"explanation\": \"" << jsonEscape(d.explanation) << "\",\n";
        out << "      \"invalidExample\": \"" << jsonEscape(d.invalidExample) << "\",\n";
        out << "      \"validExamples\": [";
        for (std::size_t j = 0; j < d.validExamples.size(); ++j) {
            out << "\"" << jsonEscape(d.validExamples[j]) << "\"";
            if (j + 1 < d.validExamples.size()) out << ", ";
        }
        out << "],\n";
        out << "      \"fixes\": [";
        for (std::size_t j = 0; j < d.fixes.size(); ++j) {
            out << "\"" << jsonEscape(d.fixes[j].description) << "\"";
            if (j + 1 < d.fixes.size()) out << ", ";
        }
        out << "],\n";
        out << "      \"trace\": [";
        for (std::size_t j = 0; j < d.trace.size(); ++j) {
            out << "{\"component\":\"" << jsonEscape(d.trace[j].component)
                << "\",\"ok\":" << (d.trace[j].ok ? "true" : "false") << "}";
            if (j + 1 < d.trace.size()) out << ", ";
        }
        out << "]\n";
        out << "    }";
        if (i + 1 < diagnostics_.size()) out << ",";
        out << "\n";
    }
    out << "  ]\n}\n";
}
