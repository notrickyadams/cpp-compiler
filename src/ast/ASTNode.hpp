#pragma once
#include "../core/SourceSpan.hpp"
#include <string>
#include <memory>

// ============================================================
//  Forward declarations — lets ASTVisitor reference all node
//  types without including their full definitions.
// ============================================================
struct IntLiteralNode;
struct IdentifierNode;
struct BinaryExprNode;
struct VarDeclNode;
struct ReturnStmtNode;
struct BlockStmtNode;
struct FunctionDeclNode;
struct ProgramNode;

// ============================================================
//  ASTVisitor — the Visitor interface.
//
//  Why Visitor pattern?
//    Without it, every operation on the AST (printing,
//    type-checking, code generation) would need a method on
//    every node class. Adding a new operation means editing
//    every node. With Visitor: adding an operation = adding
//    one new class. Adding a node type = adding one method
//    to every existing visitor.
//
//  Industry reference:
//    Clang uses RecursiveASTVisitor for this purpose.
//    LLVM IR uses InstVisitor<T>.
//    Both are templated Visitor variants of this same pattern.
// ============================================================
class ASTVisitor {
public:
    virtual ~ASTVisitor() = default;

    virtual void visit(ProgramNode& n)      = 0;
    virtual void visit(FunctionDeclNode& n) = 0;
    virtual void visit(BlockStmtNode& n)    = 0;
    virtual void visit(VarDeclNode& n)      = 0;
    virtual void visit(ReturnStmtNode& n)   = 0;
    virtual void visit(BinaryExprNode& n)   = 0;
    virtual void visit(IdentifierNode& n)   = 0;
    virtual void visit(IntLiteralNode& n)   = 0;
};

// ============================================================
//  ASTNode — abstract base for every node in the tree.
//
//  Every node carries:
//    span     — exact source location, for diagnostic messages
//    accept() — double-dispatch entry point (Visitor pattern)
//    nodeType() — human-readable name for the printer
//
//  Memory model:
//    Nodes are heap-allocated and owned via unique_ptr.
//    A parent node owns its children. The ProgramNode at the
//    root owns the entire tree. Destroying the root frees
//    everything — no manual cleanup needed.
// ============================================================
struct ASTNode {
    SourceSpan span;

    virtual ~ASTNode() = default;

    // Visitor double-dispatch: calls v.visit(*this)
    virtual void accept(ASTVisitor& v) = 0;

    // For debugging and pretty-printing
    virtual std::string nodeType() const = 0;
};
