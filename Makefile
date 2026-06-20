CXX      := g++
CXXFLAGS := -std=c++14 -Wall -Wextra -Wpedantic -g

SRC_DIR  := src
TEST_DIR := tests
BUILD    := build

LEXER_SRCS := $(SRC_DIR)/lexer/Lexer.cpp
MAIN_SRCS  := $(SRC_DIR)/main.cpp $(LEXER_SRCS)
TEST_SRCS  := $(TEST_DIR)/test_lexer.cpp $(LEXER_SRCS)

.PHONY: all compiler tests clean run-tests

all: compiler tests

compiler: $(BUILD)/compiler

$(BUILD)/compiler: $(MAIN_SRCS)
	@if not exist $(BUILD) mkdir $(BUILD)
	$(CXX) $(CXXFLAGS) -I$(SRC_DIR) -o $@ $(MAIN_SRCS)

tests: $(BUILD)/test_lexer

$(BUILD)/test_lexer: $(TEST_SRCS)
	@if not exist $(BUILD) mkdir $(BUILD)
	$(CXX) $(CXXFLAGS) -I$(SRC_DIR) -I$(TEST_DIR) -o $@ $(TEST_SRCS)

run-tests: tests
	$(BUILD)/test_lexer

clean:
	@if exist $(BUILD) rmdir /s /q $(BUILD)
