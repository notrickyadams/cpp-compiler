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
//  Cost design (measured, not guessed):
//    The first implementation built the detail STRING eagerly at
//    every scope entry. The overhead experiment (E1) measured that
//    at ~2.4x total compile time on 20k-line inputs — the lexer
//    alone opens two scopes per token, so a clean compile paid for
//    hundreds of thousands of string constructions whose text
//    nobody would ever read. Frames now store lazily-formattable
//    POD (const char* text, a pointer to caller-owned strings,
//    ints captured by value); the string is materialized only in
//    snapshot(), i.e. only when a diagnostic actually fires.
//
//    Why storing pointers is safe: snapshot() is only ever called
//    while the frames are still OPEN (DiagnosticEngine::build()
//    runs inside the stage method that owns the scope), so any
//    caller-owned string a frame points to — a token's lexeme, an
//    AST node's name — is still alive at formatting time. Frames
//    are popped on scope exit and never read afterwards.
//
//    Ints are captured BY VALUE at scope entry on purpose: the
//    lexer's line_/col_ advance during the scope, and the frame
//    must describe where the scope was OPENED (the previous eager
//    strings had exactly those entry-time semantics).
// ============================================================

// One lazily-formattable detail. Shapes cover every call site's
// text pattern; Eager is the escape hatch for rare paths whose text
// needs machinery this header must not depend on (e.g. the parser's
// empty-lexeme case needs tokenTypeName() — a lexer-layer concern).
struct TraceDetail {
    enum class Kind : unsigned char {
        None, Lit, Line, LineCol, Char, Str, StrLine, Eager
    };
    Kind               kind = Kind::None;
    const char*        a    = "";       // prefix text
    const char*        b    = "";       // mid/suffix text
    const char*        c    = "";       // final suffix (StrLine)
    const std::string* s    = nullptr;  // caller-owned; see lifetime note
    int                i1   = 0;
    int                i2   = 0;
    char               ch   = 0;
    std::string        eager;           // Kind::Eager only (empty = no alloc)

    static TraceDetail none() { return TraceDetail(); }

    static TraceDetail lit(const char* text) {
        TraceDetail d; d.kind = Kind::Lit; d.a = text; return d;
    }
    // "<prefix><i1>"                     e.g. "starting at line 3"
    static TraceDetail line(const char* prefix, int l) {
        TraceDetail d; d.kind = Kind::Line; d.a = prefix; d.i1 = l; return d;
    }
    // "<prefix><i1>, col <i2>"           e.g. "at line 3, col 7"
    static TraceDetail lineCol(const char* prefix, int l, int col) {
        TraceDetail d; d.kind = Kind::LineCol; d.a = prefix;
        d.i1 = l; d.i2 = col; return d;
    }
    // "<prefix><ch>'"                    e.g. "dispatching on '+'"
    static TraceDetail chr(const char* prefix, char c0) {
        TraceDetail d; d.kind = Kind::Char; d.a = prefix; d.ch = c0; return d;
    }
    // "<prefix><*s><suffix>"             e.g. "function 'main'"
    static TraceDetail str(const char* prefix, const std::string& s0,
                           const char* suffix) {
        TraceDetail d; d.kind = Kind::Str; d.a = prefix;
        d.s = &s0; d.b = suffix; return d;
    }
    static TraceDetail str(const char*, std::string&&, const char*) = delete;

    // "<prefix><*s><mid><i1><suffix>"    e.g. "at 'x' (line 3)"
    static TraceDetail strLine(const char* prefix, const std::string& s0,
                               const char* mid, int l, const char* suffix) {
        TraceDetail d; d.kind = Kind::StrLine; d.a = prefix; d.s = &s0;
        d.b = mid; d.i1 = l; d.c = suffix; return d;
    }
    static TraceDetail strLine(const char*, std::string&&,
                               const char*, int, const char*) = delete;

    static TraceDetail eagerStr(std::string text) {
        TraceDetail d; d.kind = Kind::Eager; d.eager = std::move(text); return d;
    }

    // Materialize — called only from snapshot(), i.e. only when a
    // diagnostic fires. The formats reproduce the previous eager
    // strings byte-for-byte (the trace-content tests pin them).
    std::string format() const {
        switch (kind) {
            case Kind::None:    return "";
            case Kind::Lit:     return a;
            case Kind::Line:    return std::string(a) + std::to_string(i1);
            case Kind::LineCol: return std::string(a) + std::to_string(i1) +
                                       ", col " + std::to_string(i2);
            case Kind::Char:    return std::string(a) + ch + "'";
            case Kind::Str:     return std::string(a) + *s + b;
            case Kind::StrLine: return std::string(a) + *s + b +
                                       std::to_string(i1) + c;
            case Kind::Eager:   return eager;
        }
        return "";
    }
};

class TraceRecorder {
public:
    explicit TraceRecorder(std::string stage) : stage_(std::move(stage)) {
        frames_.reserve(32);  // deeper than any real descent; no growth in steady state
    }

    void push(const char* component, TraceDetail detail) {
        frames_.push_back(Frame{component, std::move(detail)});
    }

    void pop() {
        if (!frames_.empty()) frames_.pop_back();
    }

    bool empty() const { return frames_.empty(); }

    // The frames currently open — i.e. the real call path at the
    // moment of the snapshot. The deepest frame is marked as the
    // failure point (it is where the diagnostic fired). This is the
    // ONLY place details are formatted into strings.
    std::vector<TraceStep> snapshot() const {
        std::vector<TraceStep> t;
        t.reserve(frames_.size());
        for (const auto& f : frames_) {
            TraceStep step;
            step.stage     = stage_;
            step.component = f.component;
            step.detail    = f.detail.format();
            step.ok        = true;
            t.push_back(std::move(step));
        }
        if (!t.empty()) t.back().ok = false;
        return t;
    }

private:
    struct Frame {
        const char* component;
        TraceDetail detail;
    };
    std::string        stage_;
    std::vector<Frame> frames_;
};

// RAII guard: push on construction, pop on scope exit. Non-copyable
// so a frame can never be double-popped. Takes const char* (string
// literals at every call site) and a TraceDetail by value — entry
// cost is a struct copy into a pre-reserved vector, no heap work.
class TraceScope {
public:
    TraceScope(TraceRecorder& r, const char* component,
               TraceDetail detail = TraceDetail::none())
        : r_(r) {
        r_.push(component, std::move(detail));
    }
    ~TraceScope() { r_.pop(); }

    TraceScope(const TraceScope&)            = delete;
    TraceScope& operator=(const TraceScope&) = delete;

private:
    TraceRecorder& r_;
};

// ── TRACE_SCOPE — the only sanctioned way to open a frame ────
//
// Call sites use this macro rather than declaring a TraceScope
// directly, for exactly one reason: honest ablation. The overhead
// experiments (docs/experiments.md, E1/E2) compare builds with
// recording compiled OUT via -DCPPC_NO_TRACE. A no-op TraceScope
// class would still evaluate its argument expressions — the detail
// strings built at every call site ("at '" + lexeme + "' ...") —
// and so would understate the mechanism's true cost. The macro
// erases the arguments too. Uglier than plain RAII at the call
// site; chosen because measurement honesty outranks style here.
#define CPPC_TRACE_CONCAT_IMPL(a, b) a##b
#define CPPC_TRACE_CONCAT(a, b) CPPC_TRACE_CONCAT_IMPL(a, b)
#ifndef CPPC_NO_TRACE
#define TRACE_SCOPE(...) \
    TraceScope CPPC_TRACE_CONCAT(_traceScope_, __LINE__)(__VA_ARGS__)
#else
#define TRACE_SCOPE(...) ((void)0)
#endif
