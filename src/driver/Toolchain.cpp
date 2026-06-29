#include "Toolchain.hpp"
#include <fstream>
#include <cstdlib>
#include <cstdio>

Result<std::string, std::string> Toolchain::buildExecutable(
    const AssemblyProgram& asmProg,
    const std::string& outputPath) const {

    const std::string sPath = outputPath + ".s";
    const std::string oPath = outputPath + ".o";

    {
        std::ofstream f(sPath);
        if (!f) {
            return Result<std::string, std::string>::err(
                "could not write intermediate file '" + sPath +
                "' — check that the output directory exists and is writable");
        }
        f << asmProg.toString();
    }

    // g++ is always the resolved command token here, with paths as
    // quoted ARGUMENTS — unlike running a produced .exe directly,
    // this is unaffected by the cmd.exe "build/x.exe" vs "build\x.exe"
    // parsing quirk discovered while testing AssemblyGenerator.
    int asmExit = std::system(("g++ -c \"" + sPath + "\" -o \"" + oPath + "\"").c_str());
    if (asmExit != 0) {
        std::remove(sPath.c_str());
        return Result<std::string, std::string>::err(
            "assembler (g++ -c) exited with code " + std::to_string(asmExit));
    }

    int linkExit = std::system(("g++ \"" + oPath + "\" -o \"" + outputPath + "\"").c_str());

    std::remove(sPath.c_str());
    std::remove(oPath.c_str());

    if (linkExit != 0) {
        return Result<std::string, std::string>::err(
            "linker (g++) exited with code " + std::to_string(linkExit));
    }

    return Result<std::string, std::string>::ok(outputPath);
}
