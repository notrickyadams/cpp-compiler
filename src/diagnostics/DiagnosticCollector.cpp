#include "DiagnosticCollector.hpp"
#include <iostream>
#include <sstream>
#include <algorithm>

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
//  Visual format:
//
//  ============================================================
//  error[UnexpectedCharacter]: unexpected character '@'
//  ============================================================
//
//    --> input:2:9
//
//    Source
//    ------
//    2 | int @x = 5;
//        ----^
//
//    Root cause
//    ----------
//    '@' is not the start of any valid token
//
//    Why this happened
//    -----------------
//    The lexer scans source text character by character...
//
//    How to fix
//    ----------
//    1. Remove the '@' character
//    2. ...
//
//    Internal trace
//    --------------
//    [ok]  Lexer::tokenize()
//    [ok]  Lexer::nextToken()
//    [FAIL] Lexer::lexSymbol() -- switch default
//    [FAIL] DiagnosticEngine::unexpectedChar()
//
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
                                     const std::string& filename) const {
    const std::string sep(62, '=');
    const std::string thin(62, '-');

    // ── Header ───────────────────────────────────────────
    out << sep << "\n";
    out << severityName(d.severity) << "["
        << diagnosticKindName(d.kind) << "]: "
        << d.message << "\n";
    out << sep << "\n\n";

    // ── Location ─────────────────────────────────────────
    out << "  --> " << filename << ":"
        << d.span.startLine << ":" << d.span.startCol << "\n\n";

    // ── Source extract + caret ────────────────────────────
    if (!source.empty()) {
        std::string line = extractLine(source, d.span.startLine);
        std::string lineNum = std::to_string(d.span.startLine);
        // Pad to 3 chars for alignment
        while ((int)lineNum.size() < 3) lineNum = " " + lineNum;

        out << "  Source\n";
        out << "  " << thin.substr(0, 24) << "\n";
        out << "  " << lineNum << " | " << line << "\n";

        std::string caret = makeCaret(d.span.startCol,
                                       std::max(1, d.span.length));
        out << "       | " << caret << "\n";
        out << "       | ";
        // Arrow label under caret
        for (int i = 1; i < d.span.startCol; ++i) out << " ";
        out << "+----- " << d.rootCause << "\n";
        out << "\n";
    }

    // ── Root cause (one sentence) ─────────────────────────
    out << "  Root cause\n";
    out << "  " << thin.substr(0, 14) << "\n";
    out << "  " << d.rootCause << "\n\n";

    // ── Explanation (WHY) ─────────────────────────────────
    out << "  Why this happened\n";
    out << "  " << thin.substr(0, 20) << "\n";
    // Wrap and indent explanation
    std::istringstream ss(d.explanation);
    std::string sentence;
    while (std::getline(ss, sentence)) {
        if (!sentence.empty()) out << "  " << sentence << "\n";
    }
    out << "\n";

    // ── Fix suggestions ───────────────────────────────────
    if (!d.fixes.empty()) {
        out << "  How to fix\n";
        out << "  " << thin.substr(0, 13) << "\n";
        for (std::size_t i = 0; i < d.fixes.size(); ++i) {
            out << "  " << (i + 1) << ". " << d.fixes[i].description << "\n";
        }
        out << "\n";
    }

    // ── Internal trace ────────────────────────────────────
    if (!d.trace.empty()) {
        out << "  Internal trace\n";
        out << "  " << thin.substr(0, 16) << "\n";
        renderTrace(out, d.trace);
        out << "\n";
    }
}

void DiagnosticCollector::renderTrace(std::ostream& out,
                                       const std::vector<TraceStep>& trace) {
    for (std::size_t i = 0; i < trace.size(); ++i) {
        const auto& step = trace[i];
        std::string prefix = (i + 1 < trace.size()) ? "  |-- " : "  `-- ";
        std::string status = step.ok ? "[ok  ]" : "[FAIL]";
        out << "  " << status << " " << step.component;
        if (!step.detail.empty()) out << "  -- " << step.detail;
        out << "\n";
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
//  JSON render (for visualizer — Stage 8 of project)
// ─────────────────────────────────────────────────────────────

static std::string jsonEscape(const std::string& s) {
    std::string out;
    for (char c : s) {
        if (c == '"')  out += "\\\"";
        else if (c == '\\') out += "\\\\";
        else if (c == '\n') out += "\\n";
        else if (c == '\r') out += "\\r";
        else if (c == '\t') out += "\\t";
        else out += c;
    }
    return out;
}

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
