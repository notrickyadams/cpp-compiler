CXX      := g++

# OPT: extra optimisation/codegen flags. Empty for dev builds; the
# overhead experiments pass OPT=-O2 so measurements reflect an
# optimised binary, not -g -O0 string-handling artefacts.
OPT      ?=
CXXFLAGS := -std=c++14 -Wall -Wextra -Wpedantic -g $(OPT)

# ── Ablation configs (overhead experiments — docs/experiments.md) ──
#   mingw32-make CONFIG=notrace|noprov|baseline OPT=-O2 compiler
#
#   full (default) : recording + provenance ON — the real compiler.
#   notrace        : TRACE_SCOPE compiled out (incl. its argument
#                    expressions)  -> isolates trace-recording cost.
#   noprov         : span stamping + optimizer notes compiled out
#                    -> isolates provenance cost.
#   baseline       : both off -> the conventional-compiler baseline.
#
#   Ablated configs exist ONLY for measurement. Trace-content and
#   provenance tests fail there BY DESIGN (curated fallback replaces
#   recorded traces; spans read 0) — run the test suite against the
#   default config only.
CONFIG ?= full
ifeq ($(CONFIG),notrace)
CXXFLAGS += -DCPPC_NO_TRACE
endif
ifeq ($(CONFIG),noprov)
CXXFLAGS += -DCPPC_NO_PROVENANCE
endif
ifeq ($(CONFIG),baseline)
CXXFLAGS += -DCPPC_NO_TRACE -DCPPC_NO_PROVENANCE
endif

SRC_DIR  := src
TEST_DIR := tests
BUILD    := build

# ── Per-stage source files ───────────────────────────────────
DIAG_SRCS := $(SRC_DIR)/diagnostics/ExplanationBuilder.cpp \
             $(SRC_DIR)/diagnostics/DiagnosticEngine.cpp \
             $(SRC_DIR)/diagnostics/DiagnosticCollector.cpp

LEXER_SRCS     := $(SRC_DIR)/lexer/Lexer.cpp
AST_SRCS       := $(SRC_DIR)/ast/ASTPrinter.cpp \
                   $(SRC_DIR)/ast/ASTJsonPrinter.cpp
PARSER_SRCS    := $(SRC_DIR)/parser/Parser.cpp
SEMANTIC_SRCS  := $(SRC_DIR)/semantic/SemanticAnalyzer.cpp
IR_SRCS        := $(SRC_DIR)/ir/IRGenerator.cpp
OPTIMIZER_SRCS := $(SRC_DIR)/optimizer/ConstantFoldingPass.cpp \
                   $(SRC_DIR)/optimizer/CopyPropagationPass.cpp \
                   $(SRC_DIR)/optimizer/DeadCodeEliminationPass.cpp \
                   $(SRC_DIR)/optimizer/Optimizer.cpp
CODEGEN_SRCS   := $(SRC_DIR)/codegen/AssemblyGenerator.cpp
DRIVER_SRCS    := $(SRC_DIR)/driver/Toolchain.cpp

# Every stage combined — what main.cpp and every test binary links against
ALL_STAGE_SRCS := $(DIAG_SRCS) $(LEXER_SRCS) $(AST_SRCS) $(PARSER_SRCS) \
                   $(SEMANTIC_SRCS) $(IR_SRCS) $(OPTIMIZER_SRCS) $(CODEGEN_SRCS) \
                   $(DRIVER_SRCS)

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
       $(BUILD)/test_codegen $(BUILD)/test_driver $(BUILD)/test_visualizer

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

$(BUILD)/test_driver: $(TEST_DIR)/test_driver.cpp $(CODEGEN_SRCS) $(DRIVER_SRCS)
	@if not exist $(BUILD) mkdir $(BUILD)
	$(CXX) $(CXXFLAGS) -I$(SRC_DIR) -I$(TEST_DIR) -o $@ $^

# Depends on $(BUILD)/compiler too: its end-to-end tests shell out to
# the real compiler binary's --json mode, the same way test_codegen
# and test_driver shell out to g++.
$(BUILD)/test_visualizer: $(TEST_DIR)/test_visualizer.cpp $(ALL_STAGE_SRCS) $(BUILD)/compiler
	@if not exist $(BUILD) mkdir $(BUILD)
	$(CXX) $(CXXFLAGS) -I$(SRC_DIR) -I$(TEST_DIR) -o $@ $(TEST_DIR)/test_visualizer.cpp $(ALL_STAGE_SRCS)

run-tests: tests
	$(BUILD)/test_lexer
	$(BUILD)/test_diagnostics
	$(BUILD)/test_parser
	$(BUILD)/test_semantic
	$(BUILD)/test_ir
	$(BUILD)/test_optimizer
	$(BUILD)/test_codegen
	$(BUILD)/test_driver
	$(BUILD)/test_visualizer

clean:
	@if exist $(BUILD) rmdir /s /q $(BUILD)
