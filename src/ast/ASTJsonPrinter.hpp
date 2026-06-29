#pragma once
#include "Nodes.hpp"
#include <string>

// ============================================================
//  ASTJsonPrinter — Visitor that serializes the AST to a nested
//  JSON object, for the Stage 8 visualizer's frontend to render
//  as a collapsible tree.
//
//  The fourth Visitor over Nodes.hpp (after ASTPrinter,
//  SemanticAnalyzer, IRGenerator) — same as the first three,
//  adding it required zero changes to the AST node definitions.
//  That is the entire point of the Visitor pattern, demonstrated
//  one more time.
//
//  Threading pattern: ASTVisitor::visit() returns void (fixed
//  interface), so each visit() builds its OWN node's complete
//  JSON string — including children, built recursively — into
//  currentJson_. toJson(node) calls accept() then reads it back.
//  Same pattern as SemanticAnalyzer::resolveType() and
//  IRGenerator::evalExpr().
//
//  Output shape is intentionally flat per node: {"node": kind,
//  ...own fields..., ...child nodes embedded by key...}. No
//  separate schema/version field — the frontend and this printer
//  are versioned together as one project, not a public API.
// ============================================================
class ASTJsonPrinter : public ASTVisitor {
public:
    std::string toJson(ASTNode& node);

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
    std::string currentJson_;

    std::string childJson(ASTNode& node);        // recurse, return that node's JSON
    std::string childJsonOrNull(ASTNode* node);   // same, but nullptr -> "null"
};
