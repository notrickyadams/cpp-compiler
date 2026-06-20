#pragma once
#include <string>

// ============================================================
//  Type — the type system for the compiler.
//
//  Currently supports: int, bool, void, unknown.
//  "unknown" is the error-recovery type: when a variable is
//  undeclared, we assign it Unknown so we don't cascade
//  spurious "type mismatch" errors on top of "undeclared".
//
//  Industry reference:
//    Clang: QualType wraps a Type* + qualifiers (const, volatile)
//    GCC:   tree node with TREE_TYPE
//    We keep it a plain struct for clarity.
//
//  Extension points (marked TODO):
//    - Add pointer types: Type::Pointer(inner)
//    - Add function types: Type::Function(ret, params[])
//    - Add user-defined types: Type::Named(string)
// ============================================================

enum class TypeKind {
    Int,
    Bool,
    Void,
    Unknown   // error-recovery — suppresses cascading errors
};

struct Type {
    TypeKind kind = TypeKind::Unknown;

    // ── Factory constructors ─────────────────────────────
    static Type Int()     { Type t; t.kind = TypeKind::Int;     return t; }
    static Type Bool()    { Type t; t.kind = TypeKind::Bool;    return t; }
    static Type Void()    { Type t; t.kind = TypeKind::Void;    return t; }
    static Type Unknown() { Type t; t.kind = TypeKind::Unknown; return t; }

    // ── Queries ───────────────────────────────────────────
    bool isUnknown() const { return kind == TypeKind::Unknown; }
    bool isVoid()    const { return kind == TypeKind::Void;    }

    std::string name() const {
        switch (kind) {
            case TypeKind::Int:     return "int";
            case TypeKind::Bool:    return "bool";
            case TypeKind::Void:    return "void";
            case TypeKind::Unknown: return "unknown";
            default:                return "?";
        }
    }

    bool operator==(const Type& o) const { return kind == o.kind; }
    bool operator!=(const Type& o) const { return kind != o.kind; }
};

// Parse a type keyword string into a Type.
inline Type typeFromName(const std::string& name) {
    if (name == "int")  return Type::Int();
    if (name == "bool") return Type::Bool();
    if (name == "void") return Type::Void();
    return Type::Unknown();
}

// ============================================================
//  OperatorResult — what type does operator op(left, right)
//  produce? Returns Unknown if the combination is invalid.
//
//  This is a simplified operator table. In a full compiler
//  (Clang) this is a multi-hundred-line lookup through
//  overload resolution. For our subset: only int operands
//  are valid; comparison operators produce bool.
// ============================================================
inline Type operatorResultType(const std::string& op,
                                const Type& left,
                                const Type& right) {
    // Error recovery: if either operand is unknown, don't cascade
    if (left.isUnknown() || right.isUnknown()) return Type::Unknown();

    // Arithmetic operators: both must be int → int
    if (op == "+" || op == "-" || op == "*" || op == "/") {
        if (left == Type::Int() && right == Type::Int()) return Type::Int();
        return Type::Unknown(); // mismatch — caller will emit error
    }

    // Comparison operators: both must be int → bool
    if (op == "==" || op == "!=" || op == "<" || op == ">") {
        if (left == Type::Int() && right == Type::Int()) return Type::Bool();
        return Type::Unknown();
    }

    return Type::Unknown();
}
