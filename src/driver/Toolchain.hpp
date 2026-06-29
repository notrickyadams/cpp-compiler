#pragma once
#include "../codegen/AssemblyProgram.hpp"
#include "../core/Result.hpp"
#include <string>

// ============================================================
//  Toolchain — invokes the SYSTEM assembler and linker to turn
//  generated assembly text into a real, runnable executable.
//
//  This lives outside codegen/ on purpose: AssemblyGenerator is
//  pure (IR in, text out — no filesystem, no subprocess, trivially
//  unit-testable). Toolchain is the impure boundary that owns
//  every "talk to the OS" responsibility: writing the .s file,
//  shelling out to g++ to assemble and link, cleaning up.
//
//  Industry reference: Clang's driver::Toolchain plays exactly
//  this role — it knows how to invoke the assembler and linker
//  for a target, while CodeGen only ever produces IR/assembly.
//
//  Why Result<T,E> instead of StageOutput<T>/Diagnostic? A
//  failure here (g++ missing from PATH, disk full, a bad output
//  path) is an ENVIRONMENT failure, not a user source-code error —
//  it has no SourceSpan and nothing for ExplanationBuilder to say
//  about it. Result<T,E> (src/core/Result.hpp) was built for
//  exactly this binary ok/err shape during the Diagnostics stage,
//  but had no caller until now.
// ============================================================
class Toolchain {
public:
    // Assembles and links asmProg into a real executable at
    // outputPath. Intermediate "<outputPath>.s" / ".o" files are
    // written alongside it and removed afterward, win or lose —
    // same UX as `g++ -o prog file.cpp` leaving only `prog` behind.
    // On success, Ok holds outputPath back. On failure, Err holds
    // a human-readable reason (which stage failed, what exit code).
    Result<std::string, std::string> buildExecutable(
        const AssemblyProgram& asmProg,
        const std::string& outputPath) const;
};
