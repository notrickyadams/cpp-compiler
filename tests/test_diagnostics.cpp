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

TEST("collector: render full output contains WHY section", [](){
    DiagnosticEngine engine;
    DiagnosticCollector collector;
    std::string src = "int @x;";
    collector.add(engine.unexpectedChar('@', SourceSpan::point(1, 5)));

    std::ostringstream oss;
    collector.render(oss, src);
    std::string out = oss.str();

    ASSERT_TRUE(out.find("Why this happened") != std::string::npos);
    ASSERT_TRUE(out.find("How to fix")        != std::string::npos);
    ASSERT_TRUE(out.find("Internal trace")    != std::string::npos);
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
