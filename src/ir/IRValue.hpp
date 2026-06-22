#pragma once
#include <string>

// ============================================================
//  IRValue — an operand in three-address code.
//
//  Every operand is exactly one of:
//    Temp   — a compiler-generated temporary: t0, t1, ...
//    Var    — a named source variable: x, n, ...
//    Const  — a literal constant: 5, 42, ...
//
//  Design note: we do NOT wrap constants or variables in a
//  temporary. A temp is only created when an OPERATOR computes
//  a brand new value (see IRGenerator::visit(BinaryExprNode&)).
//  This keeps the IR minimal from the start — the optimizer's
//  job in the next stage is to clean up REDUNDANT temps that
//  arise from complex expressions, not ones we created needlessly.
//
//  Industry reference:
//    LLVM: Value/Constant/Instruction class hierarchy (much
//    richer — supports types, metadata, use-lists). We keep a
//    flat struct because we have exactly one type (int) so far.
// ============================================================

enum class IRValueKind { Temp, Var, Const };

struct IRValue {
    IRValueKind kind       = IRValueKind::Const;
    int         tempId     = -1;   // valid when kind == Temp
    std::string varName;            // valid when kind == Var
    int         constValue = 0;     // valid when kind == Const

    static IRValue makeTemp(int id) {
        IRValue v;
        v.kind   = IRValueKind::Temp;
        v.tempId = id;
        return v;
    }

    static IRValue makeVar(const std::string& name) {
        IRValue v;
        v.kind    = IRValueKind::Var;
        v.varName = name;
        return v;
    }

    static IRValue makeConst(int value) {
        IRValue v;
        v.kind       = IRValueKind::Const;
        v.constValue = value;
        return v;
    }

    std::string toString() const {
        switch (kind) {
            case IRValueKind::Temp:  return "t" + std::to_string(tempId);
            case IRValueKind::Var:   return varName;
            case IRValueKind::Const: return std::to_string(constValue);
            default:                 return "?";
        }
    }

    // Needed by the optimizer to detect whether a pass actually
    // changed an operand (vs. substituting a value equal to itself).
    bool operator==(const IRValue& o) const {
        if (kind != o.kind) return false;
        switch (kind) {
            case IRValueKind::Temp:  return tempId == o.tempId;
            case IRValueKind::Var:   return varName == o.varName;
            case IRValueKind::Const: return constValue == o.constValue;
            default:                 return false;
        }
    }
    bool operator!=(const IRValue& o) const { return !(*this == o); }
};
