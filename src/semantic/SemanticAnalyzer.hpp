#pragma once
#include "../ast/Nodes.hpp"
#include "../diagnostics/Diagnostic.hpp"
#include "../diagnostics/DiagnosticEngine.hpp"
#include "SymbolTable.hpp"
#include "Type.hpp"
#include <vector>
#include <string>

// ============================================================
//  SemanticAnalyzer — walks the AST, resolves types, checks
//  semantic rules, and emits structured diagnostics.
//
//  Implements ASTVisitor. Each visit() method:
//    1. Performs the semantic check for that node kind.
//    2. Annotates the node with its resolved type
//       (sets node.resolvedType string for printer/visualizer).
//    3. Records a log entry if the check passed (educational output).
//    4. Emits a Diagnostic if the check failed.
//
//  Key design: the "current expression type" accumulator.
//    visit(expression) always sets currentExprType_.
//    resolveType(node) calls accept() and reads it back.
//    This is the standard approach for compilers using
//    the Visitor pattern — it threads type info "up" the
//    expression tree without changing the visit() signature.
//
//  Industry reference:
//    Clang's Sema.cpp (~11,000 lines) does the same thing
//    but with ActOnBinaryOp(), ActOnDeclStmt(), etc.
//    GCC's c-typeck.c uses recursive functions directly.
// ============================================================
class SemanticAnalyzer : public ASTVisitor {
public:
    // Run semantic analysis on a fully-parsed program.
    // Annotates all expression nodes in-place with resolvedType.
    // Returns StageOutput<bool>: output=true means no errors.
    StageOutput<bool> analyze(ProgramNode& program);

    // Educational output — one line per check performed.
    const std::vector<std::string>& log() const { return log_; }

    // ── ASTVisitor ───────────────────────────────────────
    void visit(ProgramNode& n)      override;
    void visit(FunctionDeclNode& n) override;
    void visit(BlockStmtNode& n)    override;
    void visit(VarDeclNode& n)      override;
    void visit(ReturnStmtNode& n)   override;
    void visit(BinaryExprNode& n)   override;
    void visit(IdentifierNode& n)   override;
    void visit(IntLiteralNode& n)   override;

private:
    SymbolTable       symbols_;
    DiagnosticEngine  engine_;
    std::vector<Diagnostic>  diagnostics_;
    std::vector<std::string> log_;

    // Threading type info up through expression visits
    Type currentExprType_ = Type::Unknown();

    // Context for return-type checking
    std::string currentFunctionName_;
    Type        currentFunctionReturnType_ = Type::Void();

    // Resolve the type of an expression node (calls accept)
    Type resolveType(ASTNode& node);

    // Emit a pass/fail log entry
    void logOk(const std::string& msg);
    void logFail(const std::string& msg);
};
