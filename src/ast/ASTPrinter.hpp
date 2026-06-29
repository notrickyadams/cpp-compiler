#pragma once
#include "Nodes.hpp"
#include <ostream>
#include <string>

// ============================================================
//  ASTPrinter — Visitor that pretty-prints the AST.
//
//  Output format matches the project spec:
//
//    Program
//    Function: main
//    Body:
//      VarDecl: int x = 5
//      Return:
//        BinaryOp(+)
//          Identifier(x)
//          Number(2)
//
//  Usage:
//    ASTPrinter printer(std::cout);
//    program.accept(printer);
// ============================================================
class ASTPrinter : public ASTVisitor {
public:
    explicit ASTPrinter(std::ostream& out);

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
    std::ostream& out_;
    int           indent_ = 0;

    void line(const std::string& text) const;
    void push() { indent_ += 2; }
    void pop()  { indent_ -= 2; }
};
