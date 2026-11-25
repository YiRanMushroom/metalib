+++
title = "template for (P1306) on GCC 16 vs clang-p2996: what compiles"

[[links]]
path = "src/test/enumerator_test.cpp"
snippet = "template for (constexpr auto e : std::define_static_array(std::meta::enumerators_of(^^E)))"
+++

Measured 2026-11-24. GCC: local MinGW GCC 16.1 with `-freflection`. Fork: the Compiler Explorer build
`clang_bb_p2996` with `-freflection-latest -stdlib=libc++`.

| expansion-initializer | GCC 16.1 | clang-p2996 |
|---|---|---|
| `{1, 2, 3}` (expansion-init-list) | ok | ok |
| `std::tuple{...}` / aggregate struct (destructuring) | ok | ok |
| `static constexpr std::array` (range) | ok | ok |
| `std::define_static_array(std::meta::enumerators_of(^^T))` | **only inside a constexpr function template** | ok |
| `[: std::meta::reflect_constant_array(...) :]` (C array) | ok everywhere | ok |
| `std::meta::enumerators_of(^^T)` directly (std::vector) | error | error (ill-formed in C++26 anyway) |

On GCC 16 the define_static_array form fails in a non-template constexpr function and in a non-constexpr template:
"consteval-only expressions are only allowed in a constant-evaluated context". The reflect_constant_array splice form
works in both places. The name_of example in src/test/enumerator_test.cpp is a constexpr function template (the old enum.hpp sample was removed 2026-11-25).

The fork's P2996.md still says expansion statements are partial ("expansions over constexpr ranges are not
supported"; feature list: expansion-init-lists and destructurable expressions). The current fork build iterates
std::array and define_static_array spans fine, so that text looks older than the build.
