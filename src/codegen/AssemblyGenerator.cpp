#include "AssemblyGenerator.hpp"
#include <cassert>

// Right-pads a mnemonic to a fixed column so operands line up.
// Purely cosmetic — GAS ignores whitespace — but generated .s
// files are meant to be read (by us, and by anyone skimming this
// repo), not just assembled.
static std::string emitOp(const std::string& mnemonic, const std::string& operands) {
    if (operands.empty()) return "    " + mnemonic;
    std::string s = "    " + mnemonic;
    while (s.size() < 12) s += ' ';
    return s + operands;
}

AssemblyProgram AssemblyGenerator::generate(const IRProgram& program) {
    AssemblyProgram out;
    out.emit("    .text");
    out.emitBlank();
    for (const auto& fn : program.functions) {
        generateFunction(fn, out);
    }
    return out;
}

void AssemblyGenerator::generateFunction(const IRFunction& fn, AssemblyProgram& out) {
    const bool isMain = (fn.name == "main");
    const std::string symbol = "_" + fn.name;

    out.emit("    .globl  " + symbol);
    out.emit(symbol + ":");
    out.emit(emitOp("pushl", "%ebp"));
    out.emit(emitOp("movl", "%esp, %ebp"));

    // CRT requirement for the entry point only — see class comment.
    if (isMain) {
        out.emit(emitOp("andl", "$-16, %esp"));
    }

    StackFrameLayout layout(fn);
    if (layout.frameSize() > 0) {
        out.emit(emitOp("subl", "$" + std::to_string(layout.frameSize()) + ", %esp"));
    }

    if (isMain) {
        out.emit(emitOp("call", "___main"));
    }

    for (const auto& instr : fn.instructions) {
        generateInstruction(instr, layout, out);
    }

    // The grammar allows a body with no return statement on some
    // (or all) paths — e.g. "int f() { int x = 5; }" — and nothing
    // upstream rejects it (semantic analysis only checks the TYPE of
    // a return that exists, never that one exists). C itself treats
    // this as undefined behaviour, not a hard error, so real
    // compilers still emit a valid epilogue rather than letting
    // control fall through into whatever bytes follow in memory.
    // We do the same: guarantee every function ends in leave/ret.
    if (fn.instructions.empty() || fn.instructions.back().op != IROp::Return) {
        out.emit(emitOp("leave", ""));
        out.emit(emitOp("ret", ""));
    }

    out.emitBlank();
}

std::string AssemblyGenerator::operand(const IRValue& value, const StackFrameLayout& layout) const {
    if (value.kind == IRValueKind::Const) {
        return "$" + std::to_string(value.constValue);
    }
    return layout.slotOf(value);
}

void AssemblyGenerator::loadIntoEax(const IRValue& value, const StackFrameLayout& layout,
                                     AssemblyProgram& out) {
    out.emit(emitOp("movl", operand(value, layout) + ", %eax"));
}

void AssemblyGenerator::generateInstruction(const IRInstruction& instr,
                                             const StackFrameLayout& layout,
                                             AssemblyProgram& out) {
    out.emit("    # " + instr.toString());

    switch (instr.op) {
        case IROp::Copy: {
            if (instr.src1.kind == IRValueKind::Const) {
                out.emit(emitOp("movl", operand(instr.src1, layout) + ", " + layout.slotOf(instr.dest)));
            } else {
                loadIntoEax(instr.src1, layout, out);
                out.emit(emitOp("movl", "%eax, " + layout.slotOf(instr.dest)));
            }
            break;
        }

        case IROp::Add:
        case IROp::Sub:
        case IROp::Mul: {
            loadIntoEax(instr.src1, layout, out);
            const char* mnemonic = (instr.op == IROp::Add) ? "addl" :
                                    (instr.op == IROp::Sub) ? "subl" : "imull";
            out.emit(emitOp(mnemonic, operand(instr.src2, layout) + ", %eax"));
            out.emit(emitOp("movl", "%eax, " + layout.slotOf(instr.dest)));
            break;
        }

        // idivl requires a register/memory divisor — unlike add/sub/imul,
        // there is no immediate form, so a Const divisor must be loaded
        // into a scratch register first. cdq sign-extends %eax into
        // %edx:%eax, which idivl requires as its 64-bit dividend.
        case IROp::Div: {
            loadIntoEax(instr.src1, layout, out);
            out.emit(emitOp("cdq", ""));
            if (instr.src2.kind == IRValueKind::Const) {
                out.emit(emitOp("movl", operand(instr.src2, layout) + ", %ecx"));
                out.emit(emitOp("idivl", "%ecx"));
            } else {
                out.emit(emitOp("idivl", operand(instr.src2, layout)));
            }
            out.emit(emitOp("movl", "%eax, " + layout.slotOf(instr.dest)));
            break;
        }

        // cmpl + setX produces a 1-byte 0/1 result in %al; movzbl
        // zero-extends it to a full 32-bit int (our only type).
        case IROp::Eq:
        case IROp::Ne:
        case IROp::Lt:
        case IROp::Gt: {
            loadIntoEax(instr.src1, layout, out);
            out.emit(emitOp("cmpl", operand(instr.src2, layout) + ", %eax"));
            const char* setMnemonic =
                (instr.op == IROp::Eq) ? "sete" :
                (instr.op == IROp::Ne) ? "setne" :
                (instr.op == IROp::Lt) ? "setl" : "setg";
            out.emit(emitOp(setMnemonic, "%al"));
            out.emit(emitOp("movzbl", "%al, %eax"));
            out.emit(emitOp("movl", "%eax, " + layout.slotOf(instr.dest)));
            break;
        }

        case IROp::Return: {
            if (instr.hasSrc1) {
                loadIntoEax(instr.src1, layout, out);
            }
            out.emit(emitOp("leave", ""));
            out.emit(emitOp("ret", ""));
            break;
        }

        default:
            assert(false && "AssemblyGenerator: unhandled IROp — every IROp must "
                             "be lowered here; reaching this is a compiler bug, "
                             "not a user-facing error");
    }
}
