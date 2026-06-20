#pragma once
#include <iostream>
#include <string>
#include <functional>
#include <vector>

// ============================================================
//  Minimal test framework — no external dependencies.
//
//  API:
//    TEST("name", []{ ASSERT_EQ(a, b); ASSERT_TRUE(x); });
//    RUN_ALL_TESTS();
//
//  Why not GoogleTest/Catch2?
//    For a compiler project you'd absolutely use gtest in
//    production. We avoid the dependency here so the repo
//    builds with zero setup on any machine.
// ============================================================

struct TestCase {
    std::string            name;
    std::function<void()>  fn;
};

inline std::vector<TestCase>& testRegistry() {
    static std::vector<TestCase> reg;
    return reg;
}

// GCC 6: inline variables are C++17. Use a struct singleton instead.
struct TestCounters {
    int assertions = 0;
    int failures   = 0;
    static TestCounters& get() { static TestCounters c; return c; }
};

#define ASSERT_EQ(a, b) do { \
    ++TestCounters::get().assertions; \
    if ((a) != (b)) { \
        ++TestCounters::get().failures; \
        std::cerr << "  FAIL  " << __FILE__ << ":" << __LINE__ \
                  << "  (values not equal)\n"; \
    } \
} while(0)

#define ASSERT_TRUE(expr) do { \
    ++TestCounters::get().assertions; \
    if (!(expr)) { \
        ++TestCounters::get().failures; \
        std::cerr << "  FAIL  " << __FILE__ << ":" << __LINE__ \
                  << "  expression was false: " #expr "\n"; \
    } \
} while(0)

// Double-indirection needed so __LINE__ expands before token-pasting
#define _TEST_CONCAT_IMPL(a, b) a##b
#define _TEST_CONCAT(a, b) _TEST_CONCAT_IMPL(a, b)
#define TEST(name, body) \
    static bool _TEST_CONCAT(_reg_, __LINE__) = []{ \
        testRegistry().push_back({ name, body }); \
        return true; \
    }()

inline int RUN_ALL_TESTS() {
    int passed = 0;
    auto& c = TestCounters::get();
    for (auto& tc : testRegistry()) {
        std::cout << "[ RUN  ] " << tc.name << "\n";
        int failsBefore = c.failures;
        tc.fn();
        if (c.failures == failsBefore) {
            std::cout << "[ PASS ] " << tc.name << "\n";
            ++passed;
        } else {
            std::cout << "[ FAIL ] " << tc.name << "\n";
        }
    }
    int total = (int)testRegistry().size();
    std::cout << "\n" << passed << "/" << total << " tests passed"
              << "  (" << c.assertions << " assertions)\n";
    return (c.failures > 0) ? 1 : 0;
}
