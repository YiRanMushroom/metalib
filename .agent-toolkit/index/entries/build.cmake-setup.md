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

## Named module target (2026-11-26, the user's design)
- `METALIB_BUILD_MODULE` (default: PROJECT_IS_TOP_LEVEL) adds `metalib_module` / `metalib::module`: a static library
  with one module interface, `<build>/src/lib/module/metalib.cppm`, linking `metalib` PUBLIC. CMake's own module
  support builds it (FILE_SET CXX_MODULES, Ninja, GCC trunk on MinGW: works).
- The interface is generated at configure time: `src/lib/module/generate_module.cpp` is compiled and run by
  execute_process (the compiler's directory is prepended to PATH for its runtime DLLs; the program writes the file
  itself in binary mode, since stdout would turn \n into \r\n). It is redone when a header or the generator is newer
  than `generated.cppm`; the headers are CMAKE_CONFIGURE_DEPENDS, so a build reconfigures after an edit;
  configure_file(COPYONLY) keeps metalib.cppm's timestamp when the text did not change.
- What is exported: the nearest `[[=metalib::exported]]` / `[[=metalib::exported(false)]]` mark, the entity's own or an
  enclosing namespace's (exported.hpp marks `metalib` and unmarks `metalib::details`; reflect.hpp unmarks
  `metalib::builtin`; importers use the value of builtin_auto_ability_registry, see [[design.kind-deduction]]; the
  generator appends `details::builtin_registry_kind`, which only saves importers the instantiation, see
  [[build.compile-cost]]). Templates cannot be queried for
  annotations (annotations_of throws; a specialization can), which is why the mark goes on namespaces. GCC lists
  deduction guides as members named `__dguide_<class>`: names starting with two underscores are skipped.
- An unnamed namespace cannot be the marked wrapper: its members have internal linkage and cannot be exported.
- Exports reviewed 2026-11-26: the tag types are not exported, only their values (`exported`,
  `auto_ability_checker`). auto_ability_checker_t carries `[[=exported(false)]]` itself; exported_t cannot mark
  itself, so it lives in details. A mark on a redeclaration after the definition (`struct [[=exported(false)]]
  exported_t;`) does not work on GCC trunk: "type attributes ignored after type is already defined", annotations_of
  gives 0 (a forward declaration before the definition gives 1; functions accumulate over redeclarations). The
  standard says the attribute appertains to the class ([dcl.type.elab]/2) and annotations_of covers each declaration
  of the entity, so this looks like a GCC bug; not reported.
- Still exported although the docs never name them: basic_string_literal, basic_predicate, basic_transform. They are
  templates (no mark of their own) and the types of public values.
- Namespace aliases (probed): the generator's `export namespace N { namespace a = ::target; }` is what an importer
  needs; without it N::a is not declared; a using-declaration cannot name a namespace; an exported alias shows only
  those members of its target that are exported themselves.
- `module_test` (src/test, plain executable without header units or Catch2) imports the module; every check is a
  static_assert.

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
