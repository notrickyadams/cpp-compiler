CXX      := g++
CXXFLAGS := -std=c++14 -Wall -Wextra -Wpedantic -g

SRC_DIR  := src
TEST_DIR := tests
BUILD    := build

# ── Per-stage source files ───────────────────────────────────
DIAG_SRCS := $(SRC_DIR)/diagnostics/ExplanationBuilder.cpp \
             $(SRC_DIR)/diagnostics/DiagnosticEngine.cpp \
             $(SRC_DIR)/diagnostics/DiagnosticCollector.cpp

LEXER_SRCS     := $(SRC_DIR)/lexer/Lexer.cpp
AST_SRCS       := $(SRC_DIR)/ast/ASTPrinter.cpp
PARSER_SRCS    := $(SRC_DIR)/parser/Parser.cpp
SEMANTIC_SRCS  := $(SRC_DIR)/semantic/SemanticAnalyzer.cpp
IR_SRCS        := $(SRC_DIR)/ir/IRGenerator.cpp
OPTIMIZER_SRCS := $(SRC_DIR)/optimizer/ConstantFoldingPass.cpp \
                   $(SRC_DIR)/optimizer/CopyPropagationPass.cpp \
                   $(SRC_DIR)/optimizer/DeadCodeEliminationPass.cpp \
                   $(SRC_DIR)/optimizer/Optimizer.cpp
CODEGEN_SRCS   := $(SRC_DIR)/codegen/AssemblyGenerator.cpp

# Every stage combined — what main.cpp and every test binary links against
ALL_STAGE_SRCS := $(DIAG_SRCS) $(LEXER_SRCS) $(AST_SRCS) $(PARSER_SRCS) \
                   $(SEMANTIC_SRCS) $(IR_SRCS) $(OPTIMIZER_SRCS) $(CODEGEN_SRCS)

MAIN_SRCS := $(SRC_DIR)/main.cpp $(ALL_STAGE_SRCS)

.PHONY: all compiler tests clean run-tests

all: compiler tests

compiler: $(BUILD)/compiler

$(BUILD)/compiler: $(MAIN_SRCS)
	@if not exist $(BUILD) mkdir $(BUILD)
	$(CXX) $(CXXFLAGS) -I$(SRC_DIR) -o $@ $(MAIN_SRCS)

# ── Test binaries — one per stage ────────────────────────────
tests: $(BUILD)/test_lexer $(BUILD)/test_diagnostics $(BUILD)/test_parser \
       $(BUILD)/test_semantic $(BUILD)/test_ir $(BUILD)/test_optimizer \
       $(BUILD)/test_codegen

$(BUILD)/test_lexer: $(TEST_DIR)/test_lexer.cpp $(DIAG_SRCS) $(LEXER_SRCS)
	@if not exist $(BUILD) mkdir $(BUILD)
	$(CXX) $(CXXFLAGS) -I$(SRC_DIR) -I$(TEST_DIR) -o $@ $^

$(BUILD)/test_diagnostics: $(TEST_DIR)/test_diagnostics.cpp $(DIAG_SRCS) $(LEXER_SRCS)
	@if not exist $(BUILD) mkdir $(BUILD)
	$(CXX) $(CXXFLAGS) -I$(SRC_DIR) -I$(TEST_DIR) -o $@ $^

$(BUILD)/test_parser: $(TEST_DIR)/test_parser.cpp $(ALL_STAGE_SRCS)
	@if not exist $(BUILD) mkdir $(BUILD)
	$(CXX) $(CXXFLAGS) -I$(SRC_DIR) -I$(TEST_DIR) -o $@ $^

$(BUILD)/test_semantic: $(TEST_DIR)/test_semantic.cpp $(ALL_STAGE_SRCS)
	@if not exist $(BUILD) mkdir $(BUILD)
	$(CXX) $(CXXFLAGS) -I$(SRC_DIR) -I$(TEST_DIR) -o $@ $^

$(BUILD)/test_ir: $(TEST_DIR)/test_ir.cpp $(ALL_STAGE_SRCS)
	@if not exist $(BUILD) mkdir $(BUILD)
	$(CXX) $(CXXFLAGS) -I$(SRC_DIR) -I$(TEST_DIR) -o $@ $^

$(BUILD)/test_optimizer: $(TEST_DIR)/test_optimizer.cpp $(ALL_STAGE_SRCS)
	@if not exist $(BUILD) mkdir $(BUILD)
	$(CXX) $(CXXFLAGS) -I$(SRC_DIR) -I$(TEST_DIR) -o $@ $^

$(BUILD)/test_codegen: $(TEST_DIR)/test_codegen.cpp $(ALL_STAGE_SRCS)
	@if not exist $(BUILD) mkdir $(BUILD)
	$(CXX) $(CXXFLAGS) -I$(SRC_DIR) -I$(TEST_DIR) -o $@ $^

run-tests: tests
	$(BUILD)/test_lexer
	$(BUILD)/test_diagnostics
	$(BUILD)/test_parser
	$(BUILD)/test_semantic
	$(BUILD)/test_ir
	$(BUILD)/test_optimizer
	$(BUILD)/test_codegen

clean:
	@if exist $(BUILD) rmdir /s /q $(BUILD)
