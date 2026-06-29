#pragma once
#include <string>
#include <vector>

// ============================================================
//  Tiny hand-rolled JSON helpers — shared by every stage that
//  feeds the Stage 8 visualizer (and by DiagnosticCollector's
//  JSON export, which predates this file and motivated it).
//
//  Why no real JSON library? Every structure this project emits
//  is built by hand at the call site (object literals with a
//  handful of known keys) — a real library would mean a value
//  type, an AST-of-its-own, and a build dependency for a problem
//  that's actually just "escape this string, join these strings."
//  Matches the same reasoning AssemblyProgram.hpp gives for
//  staying a flat vector<string>: don't add structure nothing
//  downstream will use.
// ============================================================

inline std::string jsonEscape(const std::string& s) {
    std::string out;
    for (char c : s) {
        if (c == '"')       out += "\\\"";
        else if (c == '\\') out += "\\\\";
        else if (c == '\n') out += "\\n";
        else if (c == '\r') out += "\\r";
        else if (c == '\t') out += "\\t";
        else out += c;
    }
    return out;
}

// Joins already-JSON-encoded fragments into a JSON array:
// jsonArray({"1", "{\"a\":2}"}) -> "[1,{\"a\":2}]"
inline std::string jsonArray(const std::vector<std::string>& items) {
    std::string s = "[";
    for (std::size_t i = 0; i < items.size(); ++i) {
        s += items[i];
        if (i + 1 < items.size()) s += ",";
    }
    s += "]";
    return s;
}

// Quotes, escapes, and joins plain strings into a JSON array:
// jsonStringArray({"a", "b\"c"}) -> "[\"a\",\"b\\\"c\"]"
inline std::string jsonStringArray(const std::vector<std::string>& items) {
    std::vector<std::string> quoted;
    quoted.reserve(items.size());
    for (const auto& item : items) {
        quoted.push_back("\"" + jsonEscape(item) + "\"");
    }
    return jsonArray(quoted);
}
