#include "SemanticAnalyzer.hpp"

// ────────────────────────────────────────────────────────────
//  Public entry point
// ────────────────────────────────────────────────────────────
StageOutput<bool> SemanticAnalyzer::analyze(ProgramNode& program) {
    program.accept(*this);

    StageOutput<bool> out;
    out.diagnostics = std::move(diagnostics_);
    // "true means no errors" (see header) — warnings don't count, or a
    // mere -Wreturn-type-style warning would falsely read as failure.
    out.output      = !out.hasErrors();
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
//
//  The body's statements are analysed IN the parameter scope,
//  not in a nested one: C++ places parameters in the function's
//  outermost block scope, which is what makes
//  "int f(int a) { int a; }" a redeclaration error rather than
//  silent shadowing. (Silent shadowing here would be worse than
//  cosmetic — both names would lower to the same IR Var and
//  share one stack slot.)
// ────────────────────────────────────────────────────────────
void SemanticAnalyzer::visit(FunctionDeclNode& n) {
    currentFunctionName_       = n.name;
    currentFunctionReturnType_ = typeFromName(n.returnType);

    symbols_.pushScope();  // parameter + body scope (see above)

    for (auto& p : n.params) {
        Symbol sym;
        sym.name       = p.name;
        sym.type       = typeFromName(p.typeName);
        sym.declaredAt = p.span;
        symbols_.declare(sym);
        logOk("param " + p.name + " declared (" + sym.type.name() + ")");
    }

    if (n.body) {
        for (auto& stmt : n.body->statements) stmt->accept(*this);
    }

    // With no control flow in the language, "contains no ReturnStmt"
    // is exactly "never returns" — no reachability analysis needed
    // yet. Warning, not error (GCC's -Wreturn-type choice): C/C++
    // call this undefined behaviour, not ill-formed, so the build
    // still proceeds and codegen's safety-net epilogue handles it.
    if (!currentFunctionReturnType_.isVoid()) {
        bool hasReturn = false;
        if (n.body) {
            for (auto& stmt : n.body->statements) {
                if (stmt->nodeType() == "ReturnStmt") { hasReturn = true; break; }
            }
        }
        if (!hasReturn) {
            diagnostics_.push_back(engine_.missingReturn(
                n.name, currentFunctionReturnType_.name(), n.span));
            logFail("function '" + n.name + "' never returns a value");
        }
    }

    symbols_.popScope();
}

// ────────────────────────────────────────────────────────────
//  Block — push a new scope so inner declarations don't leak.
//
//  NOTE: function bodies do NOT come through here — FunctionDecl
//  iterates its body's statements directly so params and body
//  locals share one scope (see visit(FunctionDeclNode&)). This
//  override exists for the ASTVisitor contract and for any future
//  free-standing nested block ({ ... } inside a body), which the
//  grammar cannot produce yet.
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
        // Still analyse the initializer: it may contain its own
        // independent errors (undeclared names, type mismatches),
        // and skipping it would also leave its subtree without
        // resolvedType annotations for the visualizer.
        if (n.initializer) resolveType(*n.initializer);
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
            diagnostics_.push_back(
                engine_.missingReturnValue(currentFunctionName_,
                                            currentFunctionReturnType_.name(),
                                            n.span));
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
//  AssignmentExpr — x = value
//
//  Checks:
//    1. The target name must already be declared (assigning to
//       an undeclared name is SEM_UndeclaredIdentifier, the same
//       kind a bare undeclared read produces — the underlying
//       problem is identical).
//    2. The value's type must match the target's declared type.
//
//  The expression's own type (for further use, e.g. nested
//  assignments or a future "used as a sub-expression" case) is
//  the TARGET's type, mirroring real C++ (an assignment
//  expression's type/value is that of its left-hand side after
//  the write).
// ────────────────────────────────────────────────────────────
void SemanticAnalyzer::visit(AssignmentExprNode& n) {
    const Symbol* sym = symbols_.lookup(n.name);

    if (!sym) {
        diagnostics_.push_back(engine_.undeclaredIdentifier(n.name, n.span));
        logFail(n.name + " is not declared (cannot assign)");
        resolveType(*n.value);  // still analyse the RHS for further errors
        currentExprType_ = Type::Unknown();
        n.resolvedType    = "unknown";
        return;
    }

    // "main = 5;" — assigning INTO a function name. The parser can't
    // reject this (the target is a syntactically valid identifier);
    // only the symbol table knows the name belongs to a function.
    if (sym->isFunction) {
        diagnostics_.push_back(engine_.functionUsedAsValue(n.name, n.span));
        logFail(n.name + " is a function, not an assignable variable");
        resolveType(*n.value);  // still analyse the RHS for further errors
        currentExprType_ = Type::Unknown();
        n.resolvedType    = "unknown";
        return;
    }

    Type targetType = sym->type;
    Type valueType  = resolveType(*n.value);

    if (!valueType.isUnknown() && valueType != targetType) {
        diagnostics_.push_back(
            engine_.typeMismatch(targetType.name(), valueType.name(), n.span));
        logFail("assignment type mismatch: " + n.name + " is " + targetType.name() +
                ", value is " + valueType.name());
    } else if (!valueType.isUnknown()) {
        logOk("assignment valid (" + targetType.name() + " <- " + valueType.name() + ")");
    }

    currentExprType_ = targetType;
    n.resolvedType    = targetType.name();
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

    // A function name resolving here means it's being READ as a value
    // ("return main;") — meaningless without call syntax or function
    // pointers, and it would silently lower to a read of an
    // uninitialised stack slot named after the function.
    if (sym->isFunction) {
        diagnostics_.push_back(
            engine_.functionUsedAsValue(n.name, n.span));
        logFail(n.name + " is a function, not a variable");
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
