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
