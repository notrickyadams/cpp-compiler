#pragma once
#include "Type.hpp"
#include "../core/SourceSpan.hpp"
#include <string>
#include <vector>
#include <unordered_map>

// ============================================================
//  Symbol — one entry in the symbol table.
// ============================================================
struct Symbol {
    std::string name;
    Type        type;
    SourceSpan  declaredAt;
    bool        isFunction = false;
};

// ============================================================
//  SymbolTable — scope-aware name → Symbol mapping.
//
//  Implemented as a STACK of hash maps (one per scope level).
//  Operations:
//    pushScope()            — enter a new scope (e.g. '{')
//    popScope()             — leave current scope  (e.g. '}')
//    declare(sym)           — add to current scope, false if duplicate
//    lookup(name)           — search inner→outer, nullptr = not found
//    lookupCurrentScope()   — only current scope (for redecl check)
//
//  Why stack of maps (not a flat map with scope markers)?
//    • popScope() is O(1) — just pop the top map.
//    • No need to remove individual symbols on scope exit.
//    • lookup() traverses top→bottom, giving correct shadowing.
//
//  Industry reference:
//    GCC's cp/name-lookup.c uses the same design.
//    Clang's Sema uses a DeclContext tree instead (handles
//    C++ templates which need non-linear scoping).
// ============================================================
class SymbolTable {
public:
    SymbolTable()  { pushScope(); }  // always has a global scope
    ~SymbolTable() { }

    void pushScope() {
        scopes_.emplace_back();
    }

    void popScope() {
        if (scopes_.size() > 1) scopes_.pop_back();
    }

    int depth() const { return (int)scopes_.size(); }

    // Declare in current (innermost) scope.
    // Returns false if name already declared in THIS scope.
    bool declare(const Symbol& sym) {
        auto& top = scopes_.back();
        if (top.count(sym.name)) return false;
        top[sym.name] = sym;
        return true;
    }

    // Search all scopes from innermost outward.
    // Returns nullptr if not found.
    const Symbol* lookup(const std::string& name) const {
        for (int i = (int)scopes_.size() - 1; i >= 0; --i) {
            auto it = scopes_[i].find(name);
            if (it != scopes_[i].end()) return &it->second;
        }
        return nullptr;
    }

    // Search ONLY the current scope (for redeclaration checks).
    const Symbol* lookupCurrentScope(const std::string& name) const {
        if (scopes_.empty()) return nullptr;
        auto it = scopes_.back().find(name);
        if (it != scopes_.back().end()) return &it->second;
        return nullptr;
    }

private:
    std::vector<std::unordered_map<std::string, Symbol>> scopes_;
};
