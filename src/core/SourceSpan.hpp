#pragma once
#include <string>

// ============================================================
//  SourceSpan — precise location of a token or node in source.
//
//  Why not just line/col?
//    A bare line + col tells you WHERE an error starts.
//    A span tells you WHERE it starts AND ends, so we can
//    draw underlines like:
//
//      int @x = 5;
//          ^ unexpected character
//
//    endCol and length are what enable that caret rendering.
//
//  Industry reference:
//    Clang uses SourceLocation (32-bit encoded offset) +
//    SourceRange (pair of locations). We keep it readable.
// ============================================================

struct SourceSpan {
    int startLine;   // 1-based
    int startCol;    // 1-based
    int endLine;     // 1-based (same as startLine for single-line tokens)
    int endCol;      // 1-based, inclusive end column
    int length;      // character count of the span

    // ── Constructors ─────────────────────────────────────

    // A single-character span (e.g. '+', ';')
    static SourceSpan point(int line, int col) {
        SourceSpan s;
        s.startLine = line;
        s.startCol  = col;
        s.endLine   = line;
        s.endCol    = col;
        s.length    = 1;
        return s;
    }

    // A multi-character single-line span (e.g. "return", "123")
    static SourceSpan token(int line, int col, int len) {
        SourceSpan s;
        s.startLine = line;
        s.startCol  = col;
        s.endLine   = line;
        s.endCol    = col + len - 1;
        s.length    = len;
        return s;
    }

    // General range (used for multi-line constructs later)
    static SourceSpan range(int sl, int sc, int el, int ec) {
        SourceSpan s;
        s.startLine = sl;
        s.startCol  = sc;
        s.endLine   = el;
        s.endCol    = ec;
        s.length    = (el == sl) ? (ec - sc + 1) : -1; // -1 = multiline
        return s;
    }

    std::string toString() const {
        if (startLine == endLine && startCol == endCol) {
            return std::to_string(startLine) + ":" + std::to_string(startCol);
        }
        return std::to_string(startLine) + ":" + std::to_string(startCol) +
               "-" + std::to_string(endLine) + ":" + std::to_string(endCol);
    }
};
