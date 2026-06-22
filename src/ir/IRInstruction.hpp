#pragma once
#include "IRValue.hpp"
#include <string>

// ============================================================
//  IROp — every operation an IR instruction can perform.
// ============================================================
enum class IROp {
    Add, Sub, Mul, Div,   // arithmetic
    Eq, Ne, Lt, Gt,       // comparison
    Copy,                  // dest = src   (variable initialisation)
    Return                 // return src   (or bare 'return')
};

inline std::string irOpSymbol(IROp op) {
    switch (op) {
        case IROp::Add: return "+";
        case IROp::Sub: return "-";
        case IROp::Mul: return "*";
        case IROp::Div: return "/";
        case IROp::Eq:  return "==";
        case IROp::Ne:  return "!=";
        case IROp::Lt:  return "<";
        case IROp::Gt:  return ">";
        default:        return "?";
    }
}

// ============================================================
//  IRInstruction — one flat three-address-code instruction.
//
//  Three shapes, distinguished by the hasX flags (not a union —
//  plain flags keep toString() simple and the struct trivially
//  copyable):
//
//    Binary:  dest = src1 OP src2     (hasDest, hasSrc1, hasSrc2)
//    Copy:    dest = src1             (hasDest, hasSrc1)
//    Return:  return src1   |  return (hasSrc1 only, or neither)
//
//  Industry reference:
//    LLVM's Instruction class uses a real tagged hierarchy
//    (BinaryOperator, StoreInst, ReturnInst, ...) with RTTI via
//    getOpcode(). We use one struct + flags — simpler, and
//    sufficient because we only have 9 opcodes today.
// ============================================================
struct IRInstruction {
    IROp    op;
    IRValue dest;
    IRValue src1;
    IRValue src2;
    bool    hasDest = false;
    bool    hasSrc1 = false;
    bool    hasSrc2 = false;

    static IRInstruction makeBinary(IROp op, IRValue dest,
                                     IRValue left, IRValue right) {
        IRInstruction i;
        i.op = op; i.dest = dest; i.src1 = left; i.src2 = right;
        i.hasDest = true; i.hasSrc1 = true; i.hasSrc2 = true;
        return i;
    }

    static IRInstruction makeCopy(IRValue dest, IRValue src) {
        IRInstruction i;
        i.op = IROp::Copy; i.dest = dest; i.src1 = src;
        i.hasDest = true; i.hasSrc1 = true;
        return i;
    }

    static IRInstruction makeReturn(IRValue value) {
        IRInstruction i;
        i.op = IROp::Return; i.src1 = value;
        i.hasSrc1 = true;
        return i;
    }

    static IRInstruction makeReturnVoid() {
        IRInstruction i;
        i.op = IROp::Return;
        return i;
    }

    std::string toString() const {
        switch (op) {
            case IROp::Return:
                return hasSrc1 ? ("return " + src1.toString()) : "return";
            case IROp::Copy:
                return dest.toString() + " = " + src1.toString();
            default:
                return dest.toString() + " = " + src1.toString() +
                       " " + irOpSymbol(op) + " " + src2.toString();
        }
    }
};
