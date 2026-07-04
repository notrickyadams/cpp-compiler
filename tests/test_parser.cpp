#include "test_runner.hpp"
#include "../src/lexer/Lexer.hpp"
#include "../src/parser/Parser.hpp"
#include "../src/ast/Nodes.hpp"
#include "../src/ast/ASTPrinter.hpp"
#include <sstream>
#include <memory>

// ============================================================
//  Helpers
// ============================================================

// Lex + parse a source string, return the StageOutput
static StageOutput<std::unique_ptr<ProgramNode>>
parseSource(const std::string& src) {
    Lexer lexer(src);
    auto lexOut = lexer.tokenize();
    Parser parser(lexOut.output);
    return parser.parse();
}

// Lex + parse, return the printed AST string
static std::string astString(const std::string& src) {
    auto out = parseSource(src);
    if (!out.output) return "<null>";
    std::ostringstream oss;
    ASTPrinter printer(oss);
    out.output->accept(printer);
    return oss.str();
}

// ── Required spec output ──────────────────────────────────────
TEST("spec: target program parses to correct AST shape", [](){
    auto out = parseSource(
        "int main() {\n"
        "    int x = 5;\n"
        "    return x + 2;\n"
        "}\n"
    );
    ASSERT_TRUE(!out.hasErrors());
    ASSERT_TRUE(out.output != nullptr);

    ProgramNode* prog = out.output.get();
    ASSERT_EQ((int)prog->functions.size(), 1);

    FunctionDeclNode* fn = prog->functions[0].get();
    ASSERT_EQ(fn->name,       std::string("main"));
    ASSERT_EQ(fn->returnType, std::string("int"));
    ASSERT_TRUE(fn->body != nullptr);
    ASSERT_EQ((int)fn->body->statements.size(), 2);
});

// ── VarDecl ───────────────────────────────────────────────────
TEST("vardecl: int x = 5 parsed correctly", [](){
    auto out = parseSource("int f() { int x = 5; }");
    ASSERT_TRUE(!out.hasErrors());

    auto& stmts = out.output->functions[0]->body->statements;
    ASSERT_EQ((int)stmts.size(), 1);

    VarDeclNode* decl = dynamic_cast<VarDeclNode*>(stmts[0].get());
    ASSERT_TRUE(decl != nullptr);
    ASSERT_EQ(decl->typeName, std::string("int"));
    ASSERT_EQ(decl->name,     std::string("x"));
    ASSERT_TRUE(decl->initializer != nullptr);

    IntLiteralNode* lit = dynamic_cast<IntLiteralNode*>(decl->initializer.get());
    ASSERT_TRUE(lit != nullptr);
    ASSERT_EQ(lit->value, 5);
});

TEST("vardecl: int x without init is valid", [](){
    auto out = parseSource("int f() { int x; }");
    ASSERT_TRUE(!out.hasErrors());

    auto& stmts = out.output->functions[0]->body->statements;
    VarDeclNode* decl = dynamic_cast<VarDeclNode*>(stmts[0].get());
    ASSERT_TRUE(decl != nullptr);
    ASSERT_TRUE(decl->initializer == nullptr);
});

// ── ReturnStmt ────────────────────────────────────────────────
TEST("return: return integer literal", [](){
    auto out = parseSource("int f() { return 42; }");
    ASSERT_TRUE(!out.hasErrors());

    auto& stmts = out.output->functions[0]->body->statements;
    ReturnStmtNode* ret = dynamic_cast<ReturnStmtNode*>(stmts[0].get());
    ASSERT_TRUE(ret != nullptr);

    IntLiteralNode* lit = dynamic_cast<IntLiteralNode*>(ret->value.get());
    ASSERT_TRUE(lit != nullptr);
    ASSERT_EQ(lit->value, 42);
});

TEST("return: return identifier", [](){
    auto out = parseSource("int f() { return x; }");
    ASSERT_TRUE(!out.hasErrors());

    auto& stmts = out.output->functions[0]->body->statements;
    ReturnStmtNode* ret = dynamic_cast<ReturnStmtNode*>(stmts[0].get());
    IdentifierNode* id  = dynamic_cast<IdentifierNode*>(ret->value.get());
    ASSERT_TRUE(id != nullptr);
    ASSERT_EQ(id->name, std::string("x"));
});

// ── BinaryExpr ────────────────────────────────────────────────
TEST("binary: x + 2 produces BinaryExprNode with op +", [](){
    auto out = parseSource("int f() { return x + 2; }");
    ASSERT_TRUE(!out.hasErrors());

    ReturnStmtNode* ret = dynamic_cast<ReturnStmtNode*>(
        out.output->functions[0]->body->statements[0].get());
    BinaryExprNode* bin = dynamic_cast<BinaryExprNode*>(ret->value.get());

    ASSERT_TRUE(bin != nullptr);
    ASSERT_EQ(bin->op, std::string("+"));

    IdentifierNode* left = dynamic_cast<IdentifierNode*>(bin->left.get());
    IntLiteralNode* right = dynamic_cast<IntLiteralNode*>(bin->right.get());
    ASSERT_TRUE(left  != nullptr);
    ASSERT_TRUE(right != nullptr);
    ASSERT_EQ(left->name,  std::string("x"));
    ASSERT_EQ(right->value, 2);
});

TEST("binary: operators - * / parsed", [](){
    auto out1 = parseSource("int f() { return a - b; }");
    auto out2 = parseSource("int f() { return a * b; }");
    auto out3 = parseSource("int f() { return a / b; }");
    ASSERT_TRUE(!out1.hasErrors());
    ASSERT_TRUE(!out2.hasErrors());
    ASSERT_TRUE(!out3.hasErrors());
});

// ── Operator precedence (critical correctness test) ──────────
TEST("precedence: a + b * 3 => Add(a, Mul(b,3)) not Mul(Add(a,b),3)", [](){
    auto out = parseSource("int f() { return a + b * 3; }");
    ASSERT_TRUE(!out.hasErrors());

    ReturnStmtNode* ret = dynamic_cast<ReturnStmtNode*>(
        out.output->functions[0]->body->statements[0].get());
    BinaryExprNode* root = dynamic_cast<BinaryExprNode*>(ret->value.get());

    // Root should be +
    ASSERT_TRUE(root != nullptr);
    ASSERT_EQ(root->op, std::string("+"));

    // Left of + should be identifier 'a'
    IdentifierNode* leftId = dynamic_cast<IdentifierNode*>(root->left.get());
    ASSERT_TRUE(leftId != nullptr);
    ASSERT_EQ(leftId->name, std::string("a"));

    // Right of + should be * (higher precedence binds first)
    BinaryExprNode* rightMul = dynamic_cast<BinaryExprNode*>(root->right.get());
    ASSERT_TRUE(rightMul != nullptr);
    ASSERT_EQ(rightMul->op, std::string("*"));
});

TEST("precedence: 1 + 2 + 3 is left-associative: (1+2)+3", [](){
    auto out = parseSource("int f() { return 1 + 2 + 3; }");
    ASSERT_TRUE(!out.hasErrors());

    ReturnStmtNode* ret = dynamic_cast<ReturnStmtNode*>(
        out.output->functions[0]->body->statements[0].get());
    BinaryExprNode* root = dynamic_cast<BinaryExprNode*>(ret->value.get());

    // Root is +, its LEFT child should also be a + (left-assoc)
    ASSERT_TRUE(root != nullptr);
    ASSERT_EQ(root->op, std::string("+"));
    BinaryExprNode* leftPlus = dynamic_cast<BinaryExprNode*>(root->left.get());
    ASSERT_TRUE(leftPlus != nullptr);
    ASSERT_EQ(leftPlus->op, std::string("+"));
});

// ── Grouped expressions ───────────────────────────────────────
TEST("grouped: (a + b) * 3 forces different tree than a + b * 3", [](){
    auto out = parseSource("int f() { return (a + b) * 3; }");
    ASSERT_TRUE(!out.hasErrors());

    ReturnStmtNode* ret = dynamic_cast<ReturnStmtNode*>(
        out.output->functions[0]->body->statements[0].get());
    BinaryExprNode* root = dynamic_cast<BinaryExprNode*>(ret->value.get());

    // Root should be *
    ASSERT_TRUE(root != nullptr);
    ASSERT_EQ(root->op, std::string("*"));

    // Left of * should be + (from the parenthesised group)
    BinaryExprNode* leftPlus = dynamic_cast<BinaryExprNode*>(root->left.get());
    ASSERT_TRUE(leftPlus != nullptr);
    ASSERT_EQ(leftPlus->op, std::string("+"));
});

// ── Function structure ────────────────────────────────────────
TEST("function: name and return type stored correctly", [](){
    auto out = parseSource("int compute() { return 0; }");
    ASSERT_TRUE(!out.hasErrors());

    FunctionDeclNode* fn = out.output->functions[0].get();
    ASSERT_EQ(fn->name,       std::string("compute"));
    ASSERT_EQ(fn->returnType, std::string("int"));
});

TEST("function: params parsed correctly", [](){
    auto out = parseSource("int add(int a, int b) { return a; }");
    ASSERT_TRUE(!out.hasErrors());

    FunctionDeclNode* fn = out.output->functions[0].get();
    ASSERT_EQ((int)fn->params.size(), 2);
    ASSERT_EQ(fn->params[0].name,     std::string("a"));
    ASSERT_EQ(fn->params[0].typeName, std::string("int"));
    ASSERT_EQ(fn->params[1].name,     std::string("b"));
});

TEST("function: multiple functions in one program", [](){
    auto out = parseSource(
        "int foo() { return 1; }\n"
        "int bar() { return 2; }\n"
    );
    ASSERT_TRUE(!out.hasErrors());
    ASSERT_EQ((int)out.output->functions.size(), 2);
    ASSERT_EQ(out.output->functions[0]->name, std::string("foo"));
    ASSERT_EQ(out.output->functions[1]->name, std::string("bar"));
});

// ── Multiple statements in body ───────────────────────────────
TEST("block: multiple statements parsed in order", [](){
    auto out = parseSource(
        "int main() {\n"
        "    int x = 5;\n"
        "    int y = 10;\n"
        "    return x;\n"
        "}\n"
    );
    ASSERT_TRUE(!out.hasErrors());
    ASSERT_EQ((int)out.output->functions[0]->body->statements.size(), 3);
});

// ── ASTPrinter output ─────────────────────────────────────────
TEST("printer: target program produces expected text", [](){
    std::string txt = astString(
        "int main() {\n"
        "    int x = 5;\n"
        "    return x + 2;\n"
        "}\n"
    );
    ASSERT_TRUE(txt.find("Function: main")  != std::string::npos);
    ASSERT_TRUE(txt.find("VarDecl: int x")  != std::string::npos);
    ASSERT_TRUE(txt.find("Return:")          != std::string::npos);
    ASSERT_TRUE(txt.find("BinaryOp(+)")      != std::string::npos);
    ASSERT_TRUE(txt.find("Identifier(x)")    != std::string::npos);
    ASSERT_TRUE(txt.find("Number(2)")        != std::string::npos);
});

// ── Error cases ───────────────────────────────────────────────
TEST("error: missing semicolon after var decl produces diagnostic", [](){
    auto out = parseSource("int f() { int x = 5 return x; }");
    ASSERT_TRUE(out.hasErrors());
    ASSERT_EQ(out.diagnostics[0].kind, DiagnosticKind::PARSE_UnexpectedToken);
});

TEST("error: missing closing brace produces diagnostic", [](){
    auto out = parseSource("int f() { return 1;");
    ASSERT_TRUE(out.hasErrors());
});

TEST("error: diagnostic has explanation and fixes", [](){
    auto out = parseSource("int f() { int x = 5 return x; }");
    ASSERT_TRUE(!out.diagnostics.empty());
    ASSERT_TRUE(!out.diagnostics[0].explanation.empty());
    ASSERT_TRUE(!out.diagnostics[0].fixes.empty());
    ASSERT_TRUE(!out.diagnostics[0].trace.empty());
});

TEST("error: parser continues after error (error recovery)", [](){
    // Missing semicolon — parser should still find the return statement
    auto out = parseSource(
        "int f() {\n"
        "    int x = 5\n"        // missing ;
        "    return x + 1;\n"
        "}\n"
    );
    // Should have an error
    ASSERT_TRUE(out.hasErrors());
    // But should still produce a program node (not nullptr)
    ASSERT_TRUE(out.output != nullptr);
});

// ── Equality operators ────────────────────────────────────────
TEST("equality: == and != parsed correctly", [](){
    auto out1 = parseSource("int f() { return a == b; }");
    auto out2 = parseSource("int f() { return a != b; }");
    ASSERT_TRUE(!out1.hasErrors());
    ASSERT_TRUE(!out2.hasErrors());

    ReturnStmtNode* ret1 = dynamic_cast<ReturnStmtNode*>(
        out1.output->functions[0]->body->statements[0].get());
    BinaryExprNode* eq = dynamic_cast<BinaryExprNode*>(ret1->value.get());
    ASSERT_TRUE(eq != nullptr);
    ASSERT_EQ(eq->op, std::string("=="));
});

// ── Comparison operators ──────────────────────────────────────
TEST("comparison: < and > parsed correctly", [](){
    auto out = parseSource("int f() { return a < b; }");
    ASSERT_TRUE(!out.hasErrors());

    ReturnStmtNode* ret = dynamic_cast<ReturnStmtNode*>(
        out.output->functions[0]->body->statements[0].get());
    BinaryExprNode* cmp = dynamic_cast<BinaryExprNode*>(ret->value.get());
    ASSERT_TRUE(cmp != nullptr);
    ASSERT_EQ(cmp->op, std::string("<"));
});

// ── Assignment expressions ──────────────────────────────────────
// (bug report: "Parser incorrectly rejects assignment expressions
// inside return statements" — these are the 5 validation cases
// from that report, plus the error path it also specified.)

TEST("assignment: return x = 3 parses to AssignmentExpr(x, 3)", [](){
    auto out = parseSource("int main() { int x = 5; return x = 3; }");
    ASSERT_TRUE(!out.hasErrors());

    ReturnStmtNode* ret = dynamic_cast<ReturnStmtNode*>(
        out.output->functions[0]->body->statements[1].get());
    AssignmentExprNode* assign = dynamic_cast<AssignmentExprNode*>(ret->value.get());
    ASSERT_TRUE(assign != nullptr);
    ASSERT_EQ(assign->name, std::string("x"));
    IntLiteralNode* rhs = dynamic_cast<IntLiteralNode*>(assign->value.get());
    ASSERT_TRUE(rhs != nullptr);
    ASSERT_EQ(rhs->value, 3);
});

TEST("assignment: parenthesised return (x = 3) parses the same as unparenthesised", [](){
    auto out = parseSource("int main() { int x = 5; return (x = 3); }");
    ASSERT_TRUE(!out.hasErrors());
    ReturnStmtNode* ret = dynamic_cast<ReturnStmtNode*>(
        out.output->functions[0]->body->statements[1].get());
    ASSERT_TRUE(dynamic_cast<AssignmentExprNode*>(ret->value.get()) != nullptr);
});

TEST("assignment: return x = y + 1 — RHS can be a full expression", [](){
    auto out = parseSource("int main() { int x=0; int y=0; return x = y + 1; }");
    ASSERT_TRUE(!out.hasErrors());
    ReturnStmtNode* ret = dynamic_cast<ReturnStmtNode*>(
        out.output->functions[0]->body->statements[2].get());
    AssignmentExprNode* assign = dynamic_cast<AssignmentExprNode*>(ret->value.get());
    ASSERT_TRUE(assign != nullptr);
    BinaryExprNode* rhs = dynamic_cast<BinaryExprNode*>(assign->value.get());
    ASSERT_TRUE(rhs != nullptr);
    ASSERT_EQ(rhs->op, std::string("+"));
});

TEST("assignment: a = b = c is right-associative -> Assign(a, Assign(b, c))", [](){
    auto out = parseSource("int main() { int a=0; int b=0; int c=0; return a = b = c; }");
    ASSERT_TRUE(!out.hasErrors());
    ReturnStmtNode* ret = dynamic_cast<ReturnStmtNode*>(
        out.output->functions[0]->body->statements[3].get());
    AssignmentExprNode* outer = dynamic_cast<AssignmentExprNode*>(ret->value.get());
    ASSERT_TRUE(outer != nullptr);
    ASSERT_EQ(outer->name, std::string("a"));
    AssignmentExprNode* inner = dynamic_cast<AssignmentExprNode*>(outer->value.get());
    ASSERT_TRUE(inner != nullptr);
    ASSERT_EQ(inner->name, std::string("b"));
    ASSERT_TRUE(dynamic_cast<IdentifierNode*>(inner->value.get()) != nullptr);
});

TEST("assignment: (x = 2) + 1 — assignment used as a sub-expression", [](){
    auto out = parseSource("int main() { int x=0; return (x = 2) + 1; }");
    ASSERT_TRUE(!out.hasErrors());
    ReturnStmtNode* ret = dynamic_cast<ReturnStmtNode*>(
        out.output->functions[0]->body->statements[1].get());
    BinaryExprNode* plus = dynamic_cast<BinaryExprNode*>(ret->value.get());
    ASSERT_TRUE(plus != nullptr);
    ASSERT_EQ(plus->op, std::string("+"));
    ASSERT_TRUE(dynamic_cast<AssignmentExprNode*>(plus->left.get()) != nullptr);
});

TEST("assignment: plain x + 2 is unaffected by the new precedence level", [](){
    auto out = parseSource("int main() { return x + 2; }");
    ASSERT_TRUE(!out.hasErrors());
    ReturnStmtNode* ret = dynamic_cast<ReturnStmtNode*>(
        out.output->functions[0]->body->statements[0].get());
    ASSERT_TRUE(dynamic_cast<BinaryExprNode*>(ret->value.get()) != nullptr);
});

TEST("assignment: invalid target (a + b = 5) produces exactly one diagnostic, no cascade", [](){
    auto out = parseSource("int main() { int a=1; int b=2; return a + b = 5; }");
    ASSERT_TRUE(out.hasErrors());
    ASSERT_EQ((int)out.diagnostics.size(), 1);
    ASSERT_EQ(out.diagnostics[0].kind, DiagnosticKind::PARSE_InvalidAssignmentTarget);
});

// ── Malformed expression (two values, no operator) ─────────────
// (bug report: "return x 2;" should produce one rich
// PARSE_MalformedExpression diagnostic instead of the generic
// "expected ';' found '2'" PARSE_UnexpectedToken message.)

TEST("malformed: return x 2 produces exactly one diagnostic, no cascade", [](){
    auto out = parseSource("int main() { int x=1; return x 2; }");
    ASSERT_TRUE(out.hasErrors());
    ASSERT_EQ((int)out.diagnostics.size(), 1);
    ASSERT_EQ(out.diagnostics[0].kind, DiagnosticKind::PARSE_MalformedExpression);
});

TEST("malformed: diagnostic carries invalidExample and validExamples", [](){
    auto out = parseSource("int main() { int x=1; return x 2; }");
    ASSERT_TRUE(out.hasErrors());
    const Diagnostic& d = out.diagnostics[0];
    ASSERT_EQ(d.invalidExample, std::string("x 2"));
    ASSERT_EQ((int)d.validExamples.size(), 3);
    ASSERT_EQ(d.validExamples[0], std::string("x + 2"));
    ASSERT_EQ(d.validExamples[1], std::string("x * 2"));
    ASSERT_EQ(d.validExamples[2], std::string("x = 2"));
});

TEST("malformed: parser still recovers and parses the next statement", [](){
    auto out = parseSource(
        "int main() {\n"
        "    int x = 1;\n"
        "    return x 2;\n"
        "}\n"
        "int g() { return 9; }\n"
    );
    ASSERT_TRUE(out.hasErrors());
    ASSERT_EQ((int)out.output->functions.size(), 2);
    ASSERT_EQ(out.output->functions[1]->name, std::string("g"));
});

TEST("malformed: return x + 2 is unaffected (operator present, no false positive)", [](){
    auto out = parseSource("int f() { return x + 2; }");
    ASSERT_TRUE(!out.hasErrors());
});

TEST("malformed: int x = 5 5; produces exactly one diagnostic, no cascade", [](){
    auto out = parseSource("int main() { int x = 5 5; return x; }");
    ASSERT_TRUE(out.hasErrors());
    ASSERT_EQ((int)out.diagnostics.size(), 1);
    ASSERT_EQ(out.diagnostics[0].kind, DiagnosticKind::PARSE_MalformedExpression);
    // The narrative names the var-decl statement, not "return"
    ASSERT_TRUE(out.diagnostics[0].explanation.find("int x = 5") != std::string::npos);
    // And the trace contains the method that actually detected it.
    // (No longer the FRONT element: traces are now recorded from
    // execution, so the chain begins at the real outermost frame,
    // Parser::parse(), with parseVarDecl mid-chain.)
    bool sawVarDecl = false;
    for (const auto& s : out.diagnostics[0].trace)
        if (s.component == "Parser::parseVarDecl()") sawVarDecl = true;
    ASSERT_TRUE(sawVarDecl);
});

TEST("malformed: multiple stray tokens (return x 2 + 3;) still one diagnostic", [](){
    auto out = parseSource("int main() { int x=1; return x 2 + 3; }");
    ASSERT_TRUE(out.hasErrors());
    ASSERT_EQ((int)out.diagnostics.size(), 1);
    ASSERT_EQ(out.diagnostics[0].kind, DiagnosticKind::PARSE_MalformedExpression);
});

TEST("malformed: var-decl recovery still parses the following statement", [](){
    auto out = parseSource(
        "int main() {\n"
        "    int x = 5 5;\n"
        "    return 9;\n"
        "}\n");
    ASSERT_TRUE(out.hasErrors());
    ASSERT_EQ((int)out.output->functions[0]->body->statements.size(), 2);
});

TEST("robustness: out-of-range literal does not crash the parser", [](){
    // The lexer diagnoses LEX_IntegerOutOfRange; the parser must not
    // throw (std::stoi previously terminated the whole compiler here).
    Lexer lexer("int main() { return 99999999999; }");
    auto lexOut = lexer.tokenize();
    Parser parser(lexOut.output);
    auto out = parser.parse();          // must not crash
    ASSERT_TRUE(out.output != nullptr); // clamped literal, tree intact
});

// ── Recorded traces (trace-recording work) ────────────────────
TEST("trace: parser diagnostics carry the real descent path, not a curated one", [](){
    auto out = parseSource("int main() { int x=1; return x 2; }");
    ASSERT_TRUE(out.hasErrors());

    // The curated chain for MalformedExpression begins at
    // parseReturnStmt — only a trace RECORDED during execution can
    // contain the outer frames the parser actually descended through.
    bool sawParse  = false;
    bool sawFnDecl = false;
    bool sawReturn = false;
    for (const auto& s : out.diagnostics[0].trace) {
        if (s.component == "Parser::parse()")             sawParse  = true;
        if (s.component == "Parser::parseFunctionDecl()") sawFnDecl = true;
        if (s.component == "Parser::parseReturnStmt()")   sawReturn = true;
    }
    ASSERT_TRUE(sawParse);
    ASSERT_TRUE(sawFnDecl);
    ASSERT_TRUE(sawReturn);
});

TEST("trace: recorded chain still ends at the DiagnosticEngine factory step", [](){
    auto out = parseSource("int main() { int x=1; return x 2; }");
    const auto& trace = out.diagnostics[0].trace;
    ASSERT_TRUE(!trace.empty());
    ASSERT_TRUE(trace.back().component.find("DiagnosticEngine::") != std::string::npos);
    ASSERT_TRUE(!trace.back().ok);
});

int main() {
    std::cout << "=== Parser Unit Tests ===\n\n";
    return RUN_ALL_TESTS();
}
