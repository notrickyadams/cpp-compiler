#include "test_runner.hpp"
#include "../src/ir/IRFunction.hpp"
#include "../src/codegen/AssemblyGenerator.hpp"
#include "../src/driver/Toolchain.hpp"
#include <cstdio>
#include <string>

// ============================================================
//  Helper: a minimal "return CONST" IRProgram — Toolchain's job
//  starts after codegen, so these tests don't need a real
//  source-to-IR pipeline (that's test_codegen.cpp's job). This
//  isolates what's actually under test: file I/O + the system
//  assembler/linker + cleanup, not instruction selection.
// ============================================================
static AssemblyProgram simpleProgramReturning(int value) {
    IRFunction fn;
    fn.name = "main";
    fn.instructions.push_back(IRInstruction::makeReturn(IRValue::makeConst(value)));
    IRProgram p;
    p.functions.push_back(fn);

    AssemblyGenerator gen;
    return gen.generate(p);
}

static bool fileExists(const std::string& path) {
    if (FILE* f = std::fopen(path.c_str(), "rb")) {
        std::fclose(f);
        return true;
    }
    return false;
}

TEST("toolchain: builds a real executable and reports the output path back", [](){
    auto asmProg = simpleProgramReturning(33);
    Toolchain toolchain;
    auto result = toolchain.buildExecutable(asmProg, "build/toolchain_basic.exe");

    ASSERT_TRUE(result.isOk());
    ASSERT_EQ(result.get(), std::string("build/toolchain_basic.exe"));
    ASSERT_TRUE(fileExists("build/toolchain_basic.exe"));
});

TEST("toolchain: the built executable actually runs and exits with the right code", [](){
    auto asmProg = simpleProgramReturning(33);
    Toolchain toolchain;
    toolchain.buildExecutable(asmProg, "build/toolchain_run.exe");

    // Backslash path, not forward slash: cmd.exe (which system() shells
    // out to) parses a bare "build/x.exe" command token as the
    // unrelated MASM32 "build" command plus a flag-shaped argument —
    // discovered and fixed the same way in test_codegen.cpp's
    // assembleLinkRun helper.
    int exitCode = std::system("build\\toolchain_run.exe");
    ASSERT_EQ(exitCode, 33);
});

TEST("toolchain: intermediate .s and .o files are cleaned up after a successful build", [](){
    auto asmProg = simpleProgramReturning(1);
    Toolchain toolchain;
    toolchain.buildExecutable(asmProg, "build/toolchain_cleanup.exe");

    ASSERT_TRUE(fileExists("build/toolchain_cleanup.exe"));
    ASSERT_TRUE(!fileExists("build/toolchain_cleanup.exe.s"));
    ASSERT_TRUE(!fileExists("build/toolchain_cleanup.exe.o"));
});

TEST("toolchain: unwritable output path fails with Result::isErr(), not a crash", [](){
    auto asmProg = simpleProgramReturning(0);
    Toolchain toolchain;
    // "nonexistent_dir" is never created, so the intermediate .s
    // write fails immediately — deterministic, no need to fake a
    // real g++ failure to exercise the error path.
    auto result = toolchain.buildExecutable(asmProg, "build/nonexistent_dir/x.exe");

    ASSERT_TRUE(result.isErr());
    ASSERT_TRUE(!result.error().empty());
});

int main() {
    return RUN_ALL_TESTS();
}
