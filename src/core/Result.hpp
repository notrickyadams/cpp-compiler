#pragma once
#include <vector>
#include <cassert>

// ============================================================
//  Result<T, E> — a value that is either Ok(T) or Err(E).
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
//  Note: StageOutput<T> (pipeline stage results) is defined in
//  diagnostics/Diagnostic.hpp because it depends on Diagnostic.
//  Keep this file free of that dependency.
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
