#include "IRGenerator.hpp"
#include <cassert>

// ────────────────────────────────────────────────────────────
//  Public entry point
// ────────────────────────────────────────────────────────────
IRProgram IRGenerator::generate(ProgramNode& program) {
    program.accept(*this);
    return program_;
}

// ────────────────────────────────────────────────────────────
//  Program — lower each function in turn
// ────────────────────────────────────────────────────────────
void IRGenerator::visit(ProgramNode& n) {
    for (auto& fn : n.functions) fn->accept(*this);
}

// ────────────────────────────────────────────────────────────
//  FunctionDecl — start a new IRFunction, reset temp numbering
//
//  Temp counters reset PER FUNCTION (not globally). t0 in
//  main() and t0 in helper() are unrelated — exactly like real
//  register allocators that work one function at a time.
// ────────────────────────────────────────────────────────────
void IRGenerator::visit(FunctionDeclNode& n) {
    IRFunction func;
    func.name       = n.name;
    func.returnType = n.returnType;
    for (auto& p : n.params) func.paramNames.push_back(p.name);

    program_.functions.push_back(std::move(func));
    currentFunction_ = &program_.functions.back();
    tempCounter_     = 0;

    if (n.body) n.body->accept(*this);
}

// ────────────────────────────────────────────────────────────
//  Block — lower each statement in source order
// ────────────────────────────────────────────────────────────
void IRGenerator::visit(BlockStmtNode& n) {
    for (auto& stmt : n.statements) stmt->accept(*this);
}

// ────────────────────────────────────────────────────────────
//  VarDecl
//
//  int x = 5;   ->  x = 5            (Copy, constant used directly)
//  int x = a+b; ->  t0 = a + b
//                   x = t0           (Copy of the binary op's temp)
//  int x;       ->  (nothing emitted — no initial value to assign)
// ────────────────────────────────────────────────────────────
void IRGenerator::visit(VarDeclNode& n) {
    if (n.initializer) {
        IRValue val = evalExpr(*n.initializer);
        emit(IRInstruction::makeCopy(IRValue::makeVar(n.name), val));
    }
}

// ────────────────────────────────────────────────────────────
//  ReturnStmt
//
//  return x + 2;  ->  t0 = x + 2
//                     return t0
//  return 42;     ->  return 42      (constant used directly)
//  return;        ->  return         (bare — no operand)
// ────────────────────────────────────────────────────────────
void IRGenerator::visit(ReturnStmtNode& n) {
    if (n.value) {
        IRValue val = evalExpr(*n.value);
        emit(IRInstruction::makeReturn(val));
    } else {
        emit(IRInstruction::makeReturnVoid());
    }
}

// ────────────────────────────────────────────────────────────
//  BinaryExpr — the only node that creates a NEW temporary.
//
//  Post-order: evaluate left, evaluate right, THEN combine.
//  This is why nested expressions naturally produce
//  correctly-ordered instructions — see Stage 4 Q&A on
//  "a + b * c" for the worked example.
// ────────────────────────────────────────────────────────────
void IRGenerator::visit(BinaryExprNode& n) {
    IRValue left  = evalExpr(*n.left);
    IRValue right = evalExpr(*n.right);
    IRValue temp  = newTemp();

    emit(IRInstruction::makeBinary(opFromString(n.op), temp, left, right));
    currentValue_ = temp;
}

// ────────────────────────────────────────────────────────────
//  AssignmentExpr — x = value
//
//  x = 3;        ->  x = 3                (Copy, same shape as VarDeclNode)
//  x = y + 1;    ->  t0 = y + 1
//                    x = t0
//  return x = 3; ->  x = 3
//                    return x             (re-reads the Var, no new temp —
//                                           see currentValue_ below)
//
//  currentValue_ is set to Var(name), NOT the right-hand value
//  directly: the expression's value is "whatever x holds now",
//  and x is already a valid, free operand — manufacturing a temp
//  to hold a second copy of the same value would be redundant.
//  This also makes chained assignment (a = b = c) fall out for
//  free: the inner assignment's currentValue_ (Var b) becomes the
//  outer assignment's right-hand operand directly.
// ────────────────────────────────────────────────────────────
void IRGenerator::visit(AssignmentExprNode& n) {
    IRValue value = evalExpr(*n.value);
    emit(IRInstruction::makeCopy(IRValue::makeVar(n.name), value));
    currentValue_ = IRValue::makeVar(n.name);
}

// ────────────────────────────────────────────────────────────
//  Identifier — a variable reference IS an operand; no instruction.
// ────────────────────────────────────────────────────────────
void IRGenerator::visit(IdentifierNode& n) {
    currentValue_ = IRValue::makeVar(n.name);
}

// ────────────────────────────────────────────────────────────
//  IntLiteral — a constant IS an operand; no instruction.
// ────────────────────────────────────────────────────────────
void IRGenerator::visit(IntLiteralNode& n) {
    currentValue_ = IRValue::makeConst(n.value);
}

// ────────────────────────────────────────────────────────────
//  Helpers
// ────────────────────────────────────────────────────────────

IRValue IRGenerator::evalExpr(ASTNode& node) {
    node.accept(*this);
    return currentValue_;
}

IRValue IRGenerator::newTemp() {
    return IRValue::makeTemp(tempCounter_++);
}

void IRGenerator::emit(IRInstruction instr) {
    currentFunction_->instructions.push_back(std::move(instr));
}

IROp IRGenerator::opFromString(const std::string& op) const {
    if (op == "+")  return IROp::Add;
    if (op == "-")  return IROp::Sub;
    if (op == "*")  return IROp::Mul;
    if (op == "/")  return IROp::Div;
    if (op == "==") return IROp::Eq;
    if (op == "!=") return IROp::Ne;
    if (op == "<")  return IROp::Lt;
    if (op == ">")  return IROp::Gt;

    // Unreachable if the parser's grammar contract holds: it only
    // ever constructs BinaryExprNode with one of the 8 operators
    // above. This is an internal invariant, not a user-facing
    // error, so we assert rather than emit a Diagnostic.
    assert(false && "IRGenerator: unknown operator — parser invariant violated");
    return IROp::Add;
}
