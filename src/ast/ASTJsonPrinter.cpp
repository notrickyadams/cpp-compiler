#include "ASTJsonPrinter.hpp"
#include "../core/Json.hpp"

std::string ASTJsonPrinter::toJson(ASTNode& node) {
    node.accept(*this);
    return currentJson_;
}

std::string ASTJsonPrinter::childJson(ASTNode& node) {
    node.accept(*this);
    return currentJson_;
}

std::string ASTJsonPrinter::childJsonOrNull(ASTNode* node) {
    if (!node) return "null";
    return childJson(*node);
}

void ASTJsonPrinter::visit(ProgramNode& n) {
    std::vector<std::string> fns;
    for (auto& fn : n.functions) fns.push_back(childJson(*fn));

    currentJson_ = "{\"node\":\"Program\",\"functions\":" + jsonArray(fns) + "}";
}

void ASTJsonPrinter::visit(FunctionDeclNode& n) {
    std::vector<std::string> params;
    for (auto& p : n.params) {
        params.push_back("{\"name\":\"" + jsonEscape(p.name) +
                          "\",\"type\":\"" + jsonEscape(p.typeName) + "\"}");
    }

    currentJson_ = "{\"node\":\"FunctionDecl\",\"name\":\"" + jsonEscape(n.name) +
                    "\",\"returnType\":\"" + jsonEscape(n.returnType) +
                    "\",\"params\":" + jsonArray(params) +
                    ",\"body\":" + childJsonOrNull(n.body.get()) + "}";
}

void ASTJsonPrinter::visit(BlockStmtNode& n) {
    std::vector<std::string> stmts;
    for (auto& s : n.statements) stmts.push_back(childJson(*s));

    currentJson_ = "{\"node\":\"Block\",\"statements\":" + jsonArray(stmts) + "}";
}

void ASTJsonPrinter::visit(VarDeclNode& n) {
    currentJson_ = "{\"node\":\"VarDecl\",\"name\":\"" + jsonEscape(n.name) +
                    "\",\"type\":\"" + jsonEscape(n.typeName) +
                    "\",\"initializer\":" + childJsonOrNull(n.initializer.get()) + "}";
}

void ASTJsonPrinter::visit(ReturnStmtNode& n) {
    currentJson_ = "{\"node\":\"ReturnStmt\",\"value\":" + childJsonOrNull(n.value.get()) + "}";
}

void ASTJsonPrinter::visit(BinaryExprNode& n) {
    currentJson_ = "{\"node\":\"BinaryExpr\",\"op\":\"" + jsonEscape(n.op) +
                    "\",\"resolvedType\":\"" + jsonEscape(n.resolvedType) +
                    "\",\"left\":" + childJson(*n.left) +
                    ",\"right\":" + childJson(*n.right) + "}";
}

void ASTJsonPrinter::visit(IdentifierNode& n) {
    currentJson_ = "{\"node\":\"Identifier\",\"name\":\"" + jsonEscape(n.name) +
                   "\",\"resolvedType\":\"" + jsonEscape(n.resolvedType) + "\"}";
}

void ASTJsonPrinter::visit(IntLiteralNode& n) {
    currentJson_ = "{\"node\":\"IntLiteral\",\"value\":" + std::to_string(n.value) +
                   ",\"resolvedType\":\"" + jsonEscape(n.resolvedType) + "\"}";
}
