// Reproduce the Rust tables from the unchanged GPL OpenKey C++ data.
// clang++ -std=c++17 scripts/export-rust-tables.cpp Sources/OpenKey/engine/Vietnamese.cpp -o /tmp/openkey-tables
// /tmp/openkey-tables > Sources/OpenKey/engine-rust/src/tables.rs
#include "../Sources/OpenKey/engine/Vietnamese.h"
#include <iostream>
#include <type_traits>
template<class T> void emit(const std::vector<T>&);
template<class K, class V> void emit(const std::map<K,V>&);
template<class T> void emit(const T& x) { std::cout << +x; }
template<class T> void emit(const std::vector<T>& v) {
    std::cout << "&[";
    for (const auto& x : v) { emit(x); std::cout << ','; }
    std::cout << ']';
}
template<class K, class V> void emit(const std::map<K,V>& m) {
    std::cout << "&[";
    for (const auto& p : m) {
        std::cout << '('; emit(p.first); std::cout << ','; emit(p.second); std::cout << "),";
    }
    std::cout << ']';
}
#define TABLE(name, type) std::cout << "pub const " #name ": " type " = "; emit(name); std::cout << ";\n";
int main() {
    std::cout << "// Generated from engine/Vietnamese.cpp by scripts/export-rust-tables.cpp. GPL-3.0.\n";
    TABLE(_vowel, "&[(u32, &[&[u32]])]")
    TABLE(_vowelCombine, "&[(u32, &[&[u32]])]")
    TABLE(_vowelForMark, "&[(u32, &[&[u32]])]")
    TABLE(_consonantD, "&[&[u32]]")
    TABLE(_consonantTable, "&[&[u32]]")
    TABLE(_endConsonantTable, "&[&[u32]]")
    TABLE(_standaloneWbad, "&[u32]")
    TABLE(_doubleWAllowed, "&[&[u32]]")
    TABLE(_quickTelex, "&[(u32, &[u32])]")
    TABLE(_quickStartConsonant, "&[(u32, &[u32])]")
    TABLE(_quickEndConsonant, "&[(u32, &[u32])]")
    TABLE(_characterMap, "&[(u32, u32)]")
    for (int i=0; i<5; ++i) {
        std::cout << "pub const CODE_" << i << ": &[(u32, &[u32])] = ";
        emit(_codeTable[i]); std::cout << ";\n";
    }
    std::cout << "pub const CODES: [&[(u32, &[u32])]; 5] = [CODE_0,CODE_1,CODE_2,CODE_3,CODE_4];\n";
    std::cout << "pub const MARKS: [u32;5] = [769,768,777,771,803];\n";
}
