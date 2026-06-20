#include "ASTPrinter.hpp"
#include <string>

ASTPrinter::ASTPrinter(std::ostream& out) : out_(out) {}

void ASTPrinter::line(const std::string& text) const {
    out_ << std::string(indent_, ' ') << text << "\n";
}

// ── Visitors ─────────────────────────────────────────────────

void ASTPrinter::visit(ProgramNode& n) {
    line("Program");
    push();
    for (auto& fn : n.functions) fn->accept(*this);
    pop();
}

void ASTPrinter::visit(FunctionDeclNode& n) {
    std::string sig = "Function: " + n.name +
                      "  (returns " + n.returnType + ")";
    if (!n.params.empty()) {
        sig += "  params: (";
        for (std::size_t i = 0; i < n.params.size(); ++i) {
            if (i > 0) sig += ", ";
            sig += n.params[i].typeName + " " + n.params[i].name;
        }
        sig += ")";
    }
    line(sig);
    if (n.body) {
        line("Body:");
        push();
        n.body->accept(*this);
        pop();
    }
}

void ASTPrinter::visit(BlockStmtNode& n) {
    for (auto& stmt : n.statements) stmt->accept(*this);
}

void ASTPrinter::visit(VarDeclNode& n) {
    std::string s = "VarDecl: " + n.typeName + " " + n.name;
    if (n.initializer) {
        // inline simple initialisers; nested for complex ones
        if (n.initializer->nodeType() == "IntLiteral") {
            s += " = " + std::to_string(
                static_cast<IntLiteralNode*>(n.initializer.get())->value);
            line(s);
        } else if (n.initializer->nodeType() == "Identifier") {
            s += " = " + static_cast<IdentifierNode*>(
                n.initializer.get())->name;
            line(s);
        } else {
            line(s + " =");
            push();
            n.initializer->accept(*this);
            pop();
        }
    } else {
        line(s);
    }
}

void ASTPrinter::visit(ReturnStmtNode& n) {
    if (!n.value) {
        line("Return: (void)");
        return;
    }
    // Simple return values inline; complex ones nested
    if (n.value->nodeType() == "IntLiteral") {
        line("Return: " + std::to_string(
            static_cast<IntLiteralNode*>(n.value.get())->value));
    } else if (n.value->nodeType() == "Identifier") {
        line("Return: " + static_cast<IdentifierNode*>(n.value.get())->name);
    } else {
        line("Return:");
        push();
        n.value->accept(*this);
        pop();
    }
}

void ASTPrinter::visit(BinaryExprNode& n) {
    line("BinaryOp(" + n.op + ")");
    push();
    if (n.left)  n.left->accept(*this);
    if (n.right) n.right->accept(*this);
    pop();
}

void ASTPrinter::visit(IdentifierNode& n) {
    line("Identifier(" + n.name + ")");
}

void ASTPrinter::visit(IntLiteralNode& n) {
    line("Number(" + std::to_string(n.value) + ")");
}
