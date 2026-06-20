#pragma once
#include "ASTNode.hpp"
#include <vector>
#include <string>
#include <memory>

// ============================================================
//  All concrete AST node types.
//
//  Hierarchy:
//    ASTNode
//    ├── Expressions
//    │   ├── IntLiteralNode
//    │   ├── IdentifierNode
//    │   └── BinaryExprNode
//    ├── Statements
//    │   ├── VarDeclNode
//    │   ├── ReturnStmtNode
//    │   └── BlockStmtNode
//    └── Declarations
//        ├── ParamNode
//        ├── FunctionDeclNode
//        └── ProgramNode
//
//  Each node's accept() calls v.visit(*this) — the concrete
//  overload is chosen at runtime via virtual dispatch on v.
//
//  Design note: using structs (not classes) because all fields
//  are public by design — AST nodes are data, not objects with
//  behaviour. The Visitor carries the behaviour.
// ============================================================

// ────────────────────────────────────────────────────────────
//  EXPRESSIONS
// ────────────────────────────────────────────────────────────

// Integer constant: 5, 42, 0
struct IntLiteralNode : ASTNode {
    int value = 0;

    void accept(ASTVisitor& v) override { v.visit(*this); }
    std::string nodeType() const override { return "IntLiteral"; }
};

// Variable reference: x, main
struct IdentifierNode : ASTNode {
    std::string name;

    void accept(ASTVisitor& v) override { v.visit(*this); }
    std::string nodeType() const override { return "Identifier"; }
};

// Infix binary expression: x + 2, a == b
// The op field stores the operator lexeme: "+", "-", "==", etc.
struct BinaryExprNode : ASTNode {
    std::string              op;
    std::unique_ptr<ASTNode> left;
    std::unique_ptr<ASTNode> right;

    void accept(ASTVisitor& v) override { v.visit(*this); }
    std::string nodeType() const override { return "BinaryExpr"; }
};

// ────────────────────────────────────────────────────────────
//  STATEMENTS
// ────────────────────────────────────────────────────────────

// Variable declaration: int x = 5;
// initializer may be nullptr for: int x;
struct VarDeclNode : ASTNode {
    std::string              typeName;     // "int"
    std::string              name;         // "x"
    std::unique_ptr<ASTNode> initializer;  // nullptr = uninitialised

    void accept(ASTVisitor& v) override { v.visit(*this); }
    std::string nodeType() const override { return "VarDecl"; }
};

// Return statement: return x + 2;
// value may be nullptr for bare: return;
struct ReturnStmtNode : ASTNode {
    std::unique_ptr<ASTNode> value;

    void accept(ASTVisitor& v) override { v.visit(*this); }
    std::string nodeType() const override { return "ReturnStmt"; }
};

// Block: { stmt; stmt; }
// Owns an ordered list of child statements.
struct BlockStmtNode : ASTNode {
    std::vector<std::unique_ptr<ASTNode>> statements;

    void accept(ASTVisitor& v) override { v.visit(*this); }
    std::string nodeType() const override { return "Block"; }
};

// ────────────────────────────────────────────────────────────
//  DECLARATIONS
// ────────────────────────────────────────────────────────────

// Function parameter: int x
// Not visited independently — FunctionDeclNode exposes them.
struct ParamNode {
    std::string typeName;
    std::string name;
    SourceSpan  span;
};

// Function declaration: int main() { ... }
struct FunctionDeclNode : ASTNode {
    std::string                        returnType;
    std::string                        name;
    std::vector<ParamNode>             params;
    std::unique_ptr<BlockStmtNode>     body;

    void accept(ASTVisitor& v) override { v.visit(*this); }
    std::string nodeType() const override { return "FunctionDecl"; }
};

// Program — the root of the entire tree.
// Owns all top-level function declarations.
struct ProgramNode : ASTNode {
    std::vector<std::unique_ptr<FunctionDeclNode>> functions;

    void accept(ASTVisitor& v) override { v.visit(*this); }
    std::string nodeType() const override { return "Program"; }
};
