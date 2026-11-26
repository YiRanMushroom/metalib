+++
title = "Build setup: header-only metalib, tests with Catch2 + header units, compiler branches"

[[links]]
path = "CMakeLists.txt"
snippet = "if(METALIB_BUILD_TESTS)"

[[links]]
path = "src/lib/CMakeLists.txt"
snippet = "add_library(metalib INTERFACE)"

[[links]]
path = "src/test/CMakeLists.txt"
snippet = "catch_discover_tests(metalib_tests DL_PATHS ${dl_paths})"
+++

Layout: `src/lib` is the header-only INTERFACE target `metalib` (alias `metalib::metalib`). Its headers are in
`src/lib/include/metalib/`. `src/test` holds `metalib_tests` (Catch2 v3.16.1 via FetchContent, registered with CTest by
`catch_discover_tests`).

The user wants consumers to get a very light target. Only `METALIB_BUILD_TESTS` (default: `PROJECT_IS_TOP_LEVEL`)
fetches cmake-header-units and Catch2. With it OFF, configure fetches nothing; this was verified, `_deps` stays empty.
What consumers get: an include dir, `cxx_std_26`, and the reflection flags. Runtime link mode (static/dynamic
libstdc++) is not metalib's business, since it is header-only.

Non-obvious points:

- **HeaderUnits ordering.** `include(HeaderUnits)` (YiRanMushroom/cmake-header-units, `release` branch) must come after
  `project()` and before any `add_subdirectory()` in the top-level file. `FetchContent_MakeAvailable(Catch2)` counts as
  one. `target_auto_header_units` must be called in the directory that defines the target. It needs Ninja.
- **Public headers use `#include`, never `import`.** `import <...>;` is for test sources only.
- **`stdc++exp` is not needed for reflection on GCC 16.** Builds and tests link without it.
- **GCC 16.1 MinGW: no static libstdc++ in C++26.** `-static` or `-static-libstdc++` with `-std=gnu++26` fails with
  "multiple definition of std::logic_error::logic_error(const logic_error&)" (also runtime_error) once any TU copies
  such an exception. A 10-line repro fails; gnu++23 static and gnu++26 dynamic both link. Likely cause:
  libstdc++.a(cow-stdexcept.o) against the copy ctors that C++26 headers now define inline. Catch2 copies these
  exceptions, so `metalib_tests` links dynamically. `DL_PATHS <g++ dir>` puts the MinGW DLLs on PATH for discovery and
  CTest. Run by hand without that PATH, Windows picks Git's `C:\Program Files\Git\mingw64\bin\libstdc++-6.dll` (a
  different version) instead.
- **Compilers.** GNU: `-freflection`. Clang branch, meant for the bloomberg/clang-p2996 fork:
  `-freflection-latest -stdlib=libc++` (flags from the fork's P2996.md). It has not been built locally. Stock LLVM clang
  22.1 has no P2996 and fails at configure (`unknown argument: '-freflection-latest'`), so CLion's clang
  `cmake-build-debug` profile fails. See [[reflection.expansion-statements]] for what each compiler accepts.
- Toolchain verified: C:/mingw64-gcc-16.1 g++ 16.1.0, plus CMake 4.4.2 and ninja bundled with CLion 2026.3.

## CLion code insight (checked through the CLion MCP, 2026-11-25)
- CLion analyses with the MinGW-trunk toolchain (profile Debug-MinGW-trunk, -std=gnu++26 -freflection) and its engine
  (ReSharper C++) does evaluate splices, substitute, dealias, template_arguments_of, display_string_of, size_of,
  std::ranges::sort / stable_sort / unique - but not std::meta::type_order. Every kind is a splice of a sorted ability
  list, so with type_order all of them failed: 113 errors in handle.hpp, all "Cannot substitute template argument X
  for type template parameter ...", plus the same in tests. Probes: a computed splice works, one type_order call
  fails, sorting by display_string_of works.
- Fix: details::type_less uses type_order only when !defined(__CLION_IDE__) (CLion defines it during analysis);
  the IDE orders by display_string_of. Real compiles are unchanged. After it: handle.hpp and most tests 0 errors.
- Reported to JetBrains (2026-11-26, CLion 2026.3.0.2): std::meta::type_order unsupported is RSCPP-37583 (existing,
  untriaged; our comment describes the "Cannot substitute" effect and the __CLION_IDE__ workaround). extract not
  rejecting a wrong type is RSCPP-38174 (ours): the evaluator accepts extract<long E::*> / <double E::*> of an int
  member, extract<long (E::*)()> of int f(), and extract<int &> of a variable; it does reject wrong value types and
  wrong reference types on objects (RSCPP-38103, fixed). Not caused by int and long having the same size on Windows.
  Remove the __CLION_IDE__ branch in details::type_less once RSCPP-37583 is fixed.
- Remaining IDE-only errors are CLion's evaluator disagreeing with GCC: a u8 identifier comparison with non-ASCII
  text, and extract with a wrong type not throwing (misc_test 43, 46, 47). GCC passes these.
- get_file_problems often times out (60-280 s) on larger tests and headers; results can be stale right after an
  edit (query again).
