#pragma once
#include "../ast/Nodes.hpp"
#include "IRFunction.hpp"

// ============================================================
//  IRGenerator — lowers an annotated AST into three-address IR.
//
//  Precondition: the AST has already passed SemanticAnalyzer
//  with no errors. IRGenerator does NOT re-validate types or
//  symbols — that contract was already enforced upstream. This
//  mirrors LLVM's IRBuilder, which never re-typechecks either.
//
//  Implements ASTVisitor — the third Visitor over the same AST
//  node files (after ASTPrinter and SemanticAnalyzer). Nodes.hpp
//  has not changed since Stage 2; that is the entire point of
//  the Visitor pattern.
//
//  Threading pattern: same as SemanticAnalyzer::resolveType().
//  Since visit() returns void, each expression visit sets
//  currentValue_; evalExpr(node) calls accept() then reads it
//  back. Two stages now use this exact pattern — it is the
//  standard way to extract a "return value" from a Visitor.
// ============================================================
class IRGenerator : public ASTVisitor {
public:
    IRProgram generate(ProgramNode& program);

    void visit(ProgramNode& n)      override;
    void visit(FunctionDeclNode& n) override;
    void visit(BlockStmtNode& n)    override;
    void visit(VarDeclNode& n)      override;
    void visit(ReturnStmtNode& n)   override;
    void visit(BinaryExprNode& n)   override;
    void visit(AssignmentExprNode& n) override;
    void visit(IdentifierNode& n)   override;
    void visit(IntLiteralNode& n)   override;

private:
    IRProgram   program_;
    IRFunction* currentFunction_ = nullptr;
    int         tempCounter_     = 0;
    IRValue     currentValue_;

    // Evaluate an expression node, returning its IR operand.
    IRValue evalExpr(ASTNode& node);

    // Allocate a fresh temporary (t0, t1, ...) for the current function.
    IRValue newTemp();

    // Append an instruction to the function currently being built.
    void emit(IRInstruction instr);

    // Map AST operator lexeme ("+", "==", ...) to an IROp.
    // Precondition: op is one of the 8 operators the parser accepts —
    // violating this is a compiler bug, not a user error, hence assert
    // rather than a Diagnostic (see IRGenerator.cpp).
    IROp opFromString(const std::string& op) const;
};
