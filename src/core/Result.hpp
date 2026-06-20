#pragma once
#include <vector>
#include <cassert>

// ============================================================
//  Result<T> — a value that is either Ok(T) or Err(E).
//
//  Motivation:
//    Raw exceptions make it impossible to know at the call site
//    whether a function can fail. Result<T> makes failure
//    explicit in the type system — the caller MUST check.
//
//  Industry reference:
//    Rust: Result<T, E>
//    LLVM: llvm::Expected<T>
//    C++23: std::expected<T, E>
//    We implement a C++14-compatible subset.
//
//  Constraint: T must be default-constructible (C++14 limitation;
//  in C++17 we'd use std::optional or std::variant to avoid this).
// ============================================================

template<typename T, typename E>
class Result {
public:
    // ── Factory constructors ─────────────────────────────
    static Result ok(T value) {
        Result r;
        r.ok_    = true;
        r.value_ = std::move(value);
        return r;
    }

    static Result err(E error) {
        Result r;
        r.ok_    = false;
        r.error_ = std::move(error);
        return r;
    }

    // ── Observers ────────────────────────────────────────
    bool isOk()  const { return ok_;  }
    bool isErr() const { return !ok_; }

    // Precondition: isOk()
    const T& get() const { assert(ok_); return value_; }
    T&       get()       { assert(ok_); return value_; }

    // Precondition: isErr()
    const E& error() const { assert(!ok_); return error_; }
    E&       error()       { assert(!ok_); return error_; }

private:
    bool ok_ = true;
    T    value_{};
    E    error_{};
};

// ============================================================
//  StageOutput<T> — what a compiler stage always returns:
//    output       — the stage's product (tokens, AST nodes, etc.)
//    diagnostics  — zero or more errors/warnings collected
//
//  Key property: output is ALWAYS present, even when there are
//  errors. This lets the pipeline continue and collect more
//  diagnostics instead of stopping at the first one.
//  (Matches GCC -fmax-errors / Clang -ferror-limit behaviour.)
// ============================================================

// Forward declaration — Diagnostic is defined in diagnostics/
struct Diagnostic;

template<typename T>
struct StageOutput {
    T                       output;
    std::vector<Diagnostic> diagnostics;

    bool hasErrors() const;   // defined after Diagnostic is complete
};
