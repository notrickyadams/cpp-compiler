#include "test_runner.hpp"
#include "../src/diagnostics/DiagnosticEngine.hpp"
#include "../src/diagnostics/DiagnosticCollector.hpp"
#include "../src/diagnostics/ExplanationBuilder.hpp"
#include <sstream>

// ============================================================
//  Tests for the Diagnostic system itself.
//  These verify that the explanation infrastructure produces
//  correct, non-empty, structured output — independently of
//  the Lexer.
// ============================================================

// ── DiagnosticEngine builds correct Diagnostic ──────────────
TEST("engine: unexpectedChar fills all fields", [](){
    DiagnosticEngine engine;
    SourceSpan span = SourceSpan::point(3, 7);
    Diagnostic d = engine.unexpectedChar('@', span);

    ASSERT_EQ(d.kind, DiagnosticKind::LEX_UnexpectedCharacter);
    ASSERT_EQ(d.severity, Severity::Error);
    ASSERT_EQ(d.span.startLine, 3);
    ASSERT_EQ(d.span.startCol,  7);
    ASSERT_TRUE(!d.message.empty());
    ASSERT_TRUE(!d.rootCause.empty());
    ASSERT_TRUE(!d.explanation.empty());
    ASSERT_TRUE(!d.fixes.empty());
    ASSERT_TRUE(!d.trace.empty());
    ASSERT_TRUE(d.isError());
});

TEST("engine: unterminatedComment fills all fields", [](){
    DiagnosticEngine engine;
    SourceSpan span = SourceSpan::point(1, 1);
    Diagnostic d = engine.unterminatedComment(span);

    ASSERT_EQ(d.kind, DiagnosticKind::LEX_UnterminatedBlockComment);
    ASSERT_EQ(d.severity, Severity::Error);
    ASSERT_TRUE(!d.message.empty());
    ASSERT_TRUE(!d.fixes.empty());
});

TEST("engine: typeMismatch references both types", [](){
    DiagnosticEngine engine;
    SourceSpan span = SourceSpan::point(5, 12);
    Diagnostic d = engine.typeMismatch("int", "string", span);

    ASSERT_EQ(d.kind, DiagnosticKind::SEM_TypeMismatch);
    // message should mention both types
    ASSERT_TRUE(d.message.find("int") != std::string::npos);
    ASSERT_TRUE(d.message.find("string") != std::string::npos);
});

// ── ExplanationBuilder content quality ──────────────────────
TEST("explanation: rootCause is non-empty for all lexer kinds", [](){
    // Avoid brace-initializer-list inside macro (C preprocessor comma issue)
    ASSERT_TRUE(!ExplanationBuilder::rootCause(DiagnosticKind::LEX_UnexpectedCharacter,      "x").empty());
    ASSERT_TRUE(!ExplanationBuilder::rootCause(DiagnosticKind::LEX_UnterminatedBlockComment, "x").empty());
    ASSERT_TRUE(!ExplanationBuilder::rootCause(DiagnosticKind::LEX_UnterminatedString,       "x").empty());
    ASSERT_TRUE(!ExplanationBuilder::rootCause(DiagnosticKind::LEX_InvalidNumberLiteral,     "x").empty());
});

TEST("explanation: fixes non-empty for LEX_UnexpectedCharacter", [](){
    auto fixes = ExplanationBuilder::fixes(
        DiagnosticKind::LEX_UnexpectedCharacter, "@");
    ASSERT_TRUE(!fixes.empty());
    ASSERT_TRUE(!fixes[0].description.empty());
});

TEST("explanation: trace non-empty for LEX kinds", [](){
    auto trace = ExplanationBuilder::trace(
        DiagnosticKind::LEX_UnexpectedCharacter);
    ASSERT_TRUE(!trace.empty());
    // Last trace step should be the failing one
    ASSERT_TRUE(!trace.back().ok);
});

TEST("explanation: trace for SEM_TypeMismatch includes TypeResolver", [](){
    auto trace = ExplanationBuilder::trace(DiagnosticKind::SEM_TypeMismatch);
    bool found = false;
    for (auto& s : trace)
        if (s.component.find("TypeResolver") != std::string::npos) found = true;
    ASSERT_TRUE(found);
});

// ── DiagnosticCollector ──────────────────────────────────────
TEST("collector: add increments count", [](){
    DiagnosticEngine engine;
    DiagnosticCollector collector;
    ASSERT_EQ(collector.totalCount(), 0);
    ASSERT_TRUE(!collector.hasErrors());

    collector.add(engine.unexpectedChar('@', SourceSpan::point(1,1)));
    ASSERT_EQ(collector.totalCount(), 1);
    ASSERT_TRUE(collector.hasErrors());
    ASSERT_EQ(collector.errorCount(), 1);
});

TEST("collector: renderCompact produces filename:line:col format", [](){
    DiagnosticEngine engine;
    DiagnosticCollector collector;
    collector.add(engine.unexpectedChar('$', SourceSpan::point(4, 9)));

    std::ostringstream oss;
    collector.renderCompact(oss, "myfile.cpp");
    std::string out = oss.str();

    ASSERT_TRUE(out.find("myfile.cpp") != std::string::npos);
    ASSERT_TRUE(out.find("4")          != std::string::npos);
    ASSERT_TRUE(out.find("9")          != std::string::npos);
    ASSERT_TRUE(out.find("error")      != std::string::npos);
});

TEST("collector: render full output contains all named sections", [](){
    DiagnosticEngine engine;
    DiagnosticCollector collector;
    std::string src = "int @x;";
    collector.add(engine.unexpectedChar('@', SourceSpan::point(1, 5)));

    std::ostringstream oss;
    collector.render(oss, src);
    std::string out = oss.str();

    ASSERT_TRUE(out.find("ERROR TYPE:")    != std::string::npos);
    ASSERT_TRUE(out.find("LOCATION:")      != std::string::npos);
    ASSERT_TRUE(out.find("ROOT CAUSE:")    != std::string::npos);
    ASSERT_TRUE(out.find("WHAT HAPPENED:") != std::string::npos);
    ASSERT_TRUE(out.find("HOW TO FIX:")    != std::string::npos);
    ASSERT_TRUE(out.find("TRACE:")         != std::string::npos);
    // [ok]/[FAIL] brackets are gone — trace is now a plain arrow chain
    ASSERT_TRUE(out.find("[ok")  == std::string::npos);
    ASSERT_TRUE(out.find("[FAIL") == std::string::npos);
});

TEST("collector: render full output contains source underline", [](){
    DiagnosticEngine engine;
    DiagnosticCollector collector;
    std::string src = "int @x;";
    collector.add(engine.unexpectedChar('@', SourceSpan::point(1, 5)));

    std::ostringstream oss;
    collector.render(oss, src);
    std::string out = oss.str();

    // Should contain the source line
    ASSERT_TRUE(out.find("int @x;") != std::string::npos);
    // Should contain a caret
    ASSERT_TRUE(out.find("^")       != std::string::npos);
});

TEST("collector: renderJson produces valid JSON structure", [](){
    DiagnosticEngine engine;
    DiagnosticCollector collector;
    collector.add(engine.unexpectedChar('@', SourceSpan::point(1, 1)));

    std::ostringstream oss;
    collector.renderJson(oss, "");
    std::string out = oss.str();

    ASSERT_TRUE(out.find("\"diagnostics\"")    != std::string::npos);
    ASSERT_TRUE(out.find("\"kind\"")           != std::string::npos);
    ASSERT_TRUE(out.find("\"severity\"")       != std::string::npos);
    ASSERT_TRUE(out.find("\"message\"")        != std::string::npos);
    ASSERT_TRUE(out.find("\"explanation\"")    != std::string::npos);
    ASSERT_TRUE(out.find("\"fixes\"")          != std::string::npos);
    ASSERT_TRUE(out.find("\"trace\"")          != std::string::npos);
});

TEST("collector: JSON output contains UnexpectedCharacter kind", [](){
    DiagnosticEngine engine;
    DiagnosticCollector collector;
    collector.add(engine.unexpectedChar('@', SourceSpan::point(1, 1)));

    std::ostringstream oss;
    collector.renderJson(oss, "");
    ASSERT_TRUE(oss.str().find("UnexpectedCharacter") != std::string::npos);
});

// ── PARSE_MalformedExpression ("return x 2;") ───────────────
TEST("engine: malformedExpression fills invalidExample/validExamples", [](){
    DiagnosticEngine engine;
    SourceSpan span = SourceSpan::point(3, 15);
    Diagnostic d = engine.malformedExpression("return x", "x", "2", span);

    ASSERT_EQ(d.kind, DiagnosticKind::PARSE_MalformedExpression);
    ASSERT_EQ(d.invalidExample, std::string("x 2"));
    ASSERT_EQ((int)d.validExamples.size(), 3);
    ASSERT_TRUE(!d.rootCause.empty());
    // The narrative shows the full statement context, not just the expr
    ASSERT_TRUE(d.explanation.find("return x") != std::string::npos);
    ASSERT_TRUE(!d.fixes.empty());
    ASSERT_TRUE(!d.trace.empty());
});

TEST("engine: malformedExpression trace names the real detecting method", [](){
    DiagnosticEngine engine;
    SourceSpan span = SourceSpan::point(1, 1);
    Diagnostic d = engine.malformedExpression("int x = 5", "5", "5", span,
                                              "Parser::parseVarDecl()");
    ASSERT_TRUE(!d.trace.empty());
    ASSERT_EQ(d.trace.front().component, std::string("Parser::parseVarDecl()"));
    ASSERT_TRUE(d.explanation.find("int x = 5") != std::string::npos);
});

TEST("collector: render shows INVALID/VALID only when example is populated", [](){
    DiagnosticEngine engine;

    DiagnosticCollector withExample;
    withExample.add(engine.malformedExpression("return x", "x", "2", SourceSpan::point(1, 9)));
    std::ostringstream oss1;
    withExample.render(oss1, "");
    std::string out1 = oss1.str();
    ASSERT_TRUE(out1.find("INVALID:") != std::string::npos);
    ASSERT_TRUE(out1.find("VALID:")   != std::string::npos);
    ASSERT_TRUE(out1.find("x 2")      != std::string::npos);
    ASSERT_TRUE(out1.find("x + 2")    != std::string::npos);

    DiagnosticCollector withoutExample;
    withoutExample.add(engine.unexpectedChar('@', SourceSpan::point(1, 1)));
    std::ostringstream oss2;
    withoutExample.render(oss2, "");
    std::string out2 = oss2.str();
    ASSERT_TRUE(out2.find("INVALID:") == std::string::npos);
    ASSERT_TRUE(out2.find("VALID:")   == std::string::npos);
});

TEST("collector: TRACE section is an arrow chain starting with the stage name", [](){
    DiagnosticEngine engine;
    DiagnosticCollector collector;
    collector.add(engine.malformedExpression("return x", "x", "2", SourceSpan::point(1, 9)));

    std::ostringstream oss;
    collector.render(oss, "");
    std::string out = oss.str();

    ASSERT_TRUE(out.find("TRACE:\nParser\n") != std::string::npos);
    ASSERT_TRUE(out.find("\xE2\x86\x92 Parser::parseReturnStmt()") != std::string::npos);
});

TEST("collector: warning severity renders WARNING TYPE header, not ERROR TYPE", [](){
    DiagnosticEngine engine;
    DiagnosticCollector collector;
    collector.add(engine.missingReturn("f", "int", SourceSpan::point(1, 1)));

    std::ostringstream oss;
    collector.render(oss, "");
    std::string out = oss.str();

    ASSERT_TRUE(out.find("WARNING TYPE:\nMissing Return") != std::string::npos);
    ASSERT_TRUE(out.find("ERROR TYPE:") == std::string::npos);
    ASSERT_TRUE(out.find("1 warning(s)") != std::string::npos);
});

// ── Curated fallback (no recorder attached) ──────────────────
TEST("trace: direct engine use (no recorder) falls back to the curated chain", [](){
    DiagnosticEngine engine;   // never attached to a TraceRecorder
    Diagnostic d = engine.undeclaredIdentifier("y", SourceSpan::point(1, 1));
    ASSERT_TRUE(!d.trace.empty());
    // Curated reference-chain content — a recorded trace would start
    // with the stage entry point instead.
    ASSERT_EQ(d.trace.front().component,
              std::string("SemanticAnalyzer::visitIdentifier()"));
});

// ── SourceSpan helpers ───────────────────────────────────────
TEST("sourcespan: point has length 1", [](){
    SourceSpan sp = SourceSpan::point(5, 3);
    ASSERT_EQ(sp.startLine, 5);
    ASSERT_EQ(sp.startCol,  3);
    ASSERT_EQ(sp.length,    1);
});

TEST("sourcespan: token has correct length", [](){
    SourceSpan sp = SourceSpan::token(2, 4, 6);
    ASSERT_EQ(sp.length,  6);
    ASSERT_EQ(sp.endCol,  9); // col 4, len 6 → end col = 4+6-1 = 9
});

int main() {
    std::cout << "=== Diagnostic System Unit Tests ===\n\n";
    return RUN_ALL_TESTS();
}
