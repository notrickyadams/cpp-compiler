#include "SemanticAnalyzer.hpp"

// ────────────────────────────────────────────────────────────
//  Public entry point
// ────────────────────────────────────────────────────────────
StageOutput<bool> SemanticAnalyzer::analyze(ProgramNode& program) {
    program.accept(*this);

    StageOutput<bool> out;
    out.output      = diagnostics_.empty();
    out.diagnostics = std::move(diagnostics_);
    return out;
}

// ────────────────────────────────────────────────────────────
//  Program — register all function names first (two-pass)
//  so functions can reference each other (mutual recursion).
// ────────────────────────────────────────────────────────────
void SemanticAnalyzer::visit(ProgramNode& n) {
    // Pass 1: hoist all function signatures into global scope
    for (auto& fn : n.functions) {
        Symbol sym;
        sym.name       = fn->name;
        sym.type       = typeFromName(fn->returnType);
        sym.declaredAt = fn->span;
        sym.isFunction = true;
        symbols_.declare(sym);
    }
    // Pass 2: analyse each function body
    for (auto& fn : n.functions) {
        fn->accept(*this);
    }
}

// ────────────────────────────────────────────────────────────
//  FunctionDecl — push param scope, analyse body
// ────────────────────────────────────────────────────────────
void SemanticAnalyzer::visit(FunctionDeclNode& n) {
    currentFunctionName_       = n.name;
    currentFunctionReturnType_ = typeFromName(n.returnType);

    symbols_.pushScope();  // parameter scope

    for (auto& p : n.params) {
        Symbol sym;
        sym.name       = p.name;
        sym.type       = typeFromName(p.typeName);
        sym.declaredAt = p.span;
        symbols_.declare(sym);
        logOk("param " + p.name + " declared (" + sym.type.name() + ")");
    }

    if (n.body) n.body->accept(*this);

    symbols_.popScope();
}

// ────────────────────────────────────────────────────────────
//  Block — push a new scope so inner declarations don't leak
// ────────────────────────────────────────────────────────────
void SemanticAnalyzer::visit(BlockStmtNode& n) {
    symbols_.pushScope();
    for (auto& stmt : n.statements) stmt->accept(*this);
    symbols_.popScope();
}

// ────────────────────────────────────────────────────────────
//  VarDecl
//
//  Checks:
//    1. Not already declared in the same scope (redeclaration)
//    2. Initialiser type matches declared type (if present)
//
//  Then declares the variable in the current scope.
// ────────────────────────────────────────────────────────────
void SemanticAnalyzer::visit(VarDeclNode& n) {
    Type varType = typeFromName(n.typeName);

    // ── Check 1: redeclaration ────────────────────────────
    if (symbols_.lookupCurrentScope(n.name)) {
        diagnostics_.push_back(
            engine_.redeclaredVariable(n.name, n.span));
        logFail(n.name + " already declared in this scope");
        // Continue analysis to report further errors — don't return early
    } else {
        // ── Resolve initialiser type ─────────────────────
        if (n.initializer) {
            Type initType = resolveType(*n.initializer);

            // Check 2: type match
            if (!initType.isUnknown() && initType != varType) {
                diagnostics_.push_back(
                    engine_.typeMismatch(varType.name(),
                                         initType.name(),
                                         n.span));
                logFail(n.name + ": initialiser type " + initType.name() +
                        " does not match declared type " + varType.name());
            } else {
                logOk(n.name + " declared (" + varType.name() + ")" +
                      (n.initializer ? " = " + initType.name() : ""));
            }
        } else {
            logOk(n.name + " declared (" + varType.name() + ")");
        }

        // Declare in current scope regardless of init errors
        Symbol sym;
        sym.name       = n.name;
        sym.type       = varType;
        sym.declaredAt = n.span;
        symbols_.declare(sym);
    }

    n.resolvedType = varType.name();
}

// ────────────────────────────────────────────────────────────
//  ReturnStmt — resolve return-value type, check vs function
// ────────────────────────────────────────────────────────────
void SemanticAnalyzer::visit(ReturnStmtNode& n) {
    if (!n.value) {
        // bare 'return;'
        if (!currentFunctionReturnType_.isVoid()) {
            logFail("bare return in non-void function '" +
                    currentFunctionName_ + "'");
        } else {
            logOk("return (void)");
        }
        return;
    }

    Type retType = resolveType(*n.value);
    n.resolvedType = retType.name();

    if (!retType.isUnknown()) {
        if (retType != currentFunctionReturnType_) {
            diagnostics_.push_back(
                engine_.typeMismatch(currentFunctionReturnType_.name(),
                                      retType.name(),
                                      n.span));
            logFail("return type mismatch: expected " +
                    currentFunctionReturnType_.name() +
                    ", got " + retType.name());
        } else {
            logOk("return type matches (" + retType.name() + ")");
        }
    }
}

// ────────────────────────────────────────────────────────────
//  BinaryExpr — resolve both operand types, check compatibility
//
//  Key design: if either operand is Unknown (from an earlier
//  error), we propagate Unknown silently to avoid cascading
//  errors. Only emit a new error when BOTH sides resolved.
// ────────────────────────────────────────────────────────────
void SemanticAnalyzer::visit(BinaryExprNode& n) {
    Type leftType  = resolveType(*n.left);
    Type rightType = resolveType(*n.right);

    Type result = operatorResultType(n.op, leftType, rightType);

    if (result.isUnknown() && !leftType.isUnknown() && !rightType.isUnknown()) {
        // Both sides known but incompatible — real error
        diagnostics_.push_back(
            engine_.typeMismatch(leftType.name(), rightType.name(), n.span));
        logFail("type mismatch: " + leftType.name() + " " + n.op +
                " " + rightType.name());
    } else if (!result.isUnknown()) {
        logOk("expression valid (" + leftType.name() + " " +
              n.op + " " + rightType.name() + " -> " + result.name() + ")");
    }

    currentExprType_  = result;
    n.resolvedType    = result.name();
}

// ────────────────────────────────────────────────────────────
//  Identifier — look up in symbol table
// ────────────────────────────────────────────────────────────
void SemanticAnalyzer::visit(IdentifierNode& n) {
    const Symbol* sym = symbols_.lookup(n.name);

    if (!sym) {
        diagnostics_.push_back(
            engine_.undeclaredIdentifier(n.name, n.span));
        logFail(n.name + " is not declared");
        currentExprType_ = Type::Unknown();
        n.resolvedType   = "unknown";
        return;
    }

    currentExprType_ = sym->type;
    n.resolvedType   = sym->type.name();
}

// ────────────────────────────────────────────────────────────
//  IntLiteral — always type int
// ────────────────────────────────────────────────────────────
void SemanticAnalyzer::visit(IntLiteralNode& n) {
    currentExprType_ = Type::Int();
    n.resolvedType   = "int";
}

// ────────────────────────────────────────────────────────────
//  Helpers
// ────────────────────────────────────────────────────────────

Type SemanticAnalyzer::resolveType(ASTNode& node) {
    node.accept(*this);
    return currentExprType_;
}

void SemanticAnalyzer::logOk(const std::string& msg) {
    log_.push_back("  [check] pass: " + msg);
}

void SemanticAnalyzer::logFail(const std::string& msg) {
    log_.push_back("  [check] FAIL: " + msg);
}
