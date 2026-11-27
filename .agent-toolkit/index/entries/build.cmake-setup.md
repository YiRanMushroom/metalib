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

## Named module target and metalib::codegen (2026-11-26, the user's design)
- `METALIB_BUILD_MODULE` (default: PROJECT_IS_TOP_LEVEL) adds `metalib_module` / `metalib::module`: a static library
  whose interfaces are the generated files in `<build>/src/lib/module/interface/`, linking `metalib` PUBLIC. CMake's
  own module support builds it (FILE_SET CXX_MODULES, Ninja, GCC trunk on MinGW: works).
- Generation at configure time: `src/lib/module/generate_module.cpp` (a main that calls
  codegen::write_directory(module_generator().append_namespace(reflect<^^metalib>).build(), argv[1])) is compiled
  with -DMETALIB_CODEGEN and
  run by execute_process into `module/generated/` (the compiler's directory is prepended to PATH for its runtime
  DLLs; files are written in binary mode). Redone when a header (GLOB_RECURSE) or the generator is newer than
  `module/generated.stamp`; the headers are CMAKE_CONFIGURE_DEPENDS. Each generated file is copied to
  `module/interface/` with configure_file(COPYONLY), so its timestamp survives unchanged text; files no longer
  generated are removed. CLion reloads CMake by itself when a header changes.
- Library side: `<metalib/codegen/marks.hpp>` (always included; namespace metalib::codegen: exported,
  export_module_name, inject) and `<metalib/codegen/module_interface.hpp>` (module_generator: append_namespace
  ... build() -> vector of virtual_file {path, text}; write_directory), included by metalib.hpp only under
  METALIB_CODEGEN: in metalib.hpp unconditionally it cost every textual include 238M -> 258M. The module metalib
  exports codegen (module_interface.hpp injects its own #include; module_test uses it through the import).
- Marks are annotations on namespaces (they accumulate over every reopening of the namespace, probed) or, for
  inject, on a namespace-scope variable (which then is a placeholder and is not exported; this is what gives
  in_place a position). An annotation written at global scope must qualify everything
  (`[[=metalib::codegen::inject{.at = metalib::codegen::inject::at_front, ...}]]`). inject::content is a
  string_literal: std::meta::extract rejects std::string_view (not structural) and a `char const *` pointing at a
  string literal.
- Modules (the user's rule, 2026-11-26): the module tree follows the names, not the namespaces. A namespace with
  export_module_name puts itself and what is inside into a module: append = "<module of the enclosing
  namespace>.<name>", as_root = the name as given; equal names share one module. "a" gets `export import` for every
  "a.x"; a missing level is created as a module that only re-exports. inject propagation (down / up) follows the
  names too. Paths: dots become directories (a.b -> a/b.cppm). Positions: at_front (global module fragment),
  before_exports (after the module declaration and the re-exports), in_place, at_back.
- metalib's own modules (2026-11-26 night, the user's design; needs the patched GCC below): namespace metalib
  carries export_module_name("metalib") and holds no declaration directly; everything is in inline namespaces that
  append a name: metalib::core ("core": all core headers open `namespace metalib::inline core`; core.hpp injects
  its include at_front), metalib::reflection ("reflect": reflect.hpp; include at_front, explicit instantiation
  at_back), metalib::codegen::generation ("codegen": marks.hpp and codegen/module_interface.hpp; the marks are
  exported by metalib.codegen, not by the core). So metalib.cppm only re-exports. Every interface includes its header
  in the global module fragment and exports only the names of its own namespace; the details namespaces
  (metalib::core::details, codegen::generation::details) are exported(false). The inline namespaces carry
  [[=codegen::inlined]] (reflection cannot see inline), so blocks read `export namespace metalib::inline core`.
  reflect.hpp's helpers are in reflect_details: a second "details" reachable from metalib through an inline
  namespace would make metalib::details ambiguous. Side effect: display strings and diagnostics spell
  metalib::core::class_type; metalib's own error messages do not contain qualified kind names, no test changed.
- Tried and dropped the same night: submodules that import the core instead of including it (`export import
  metalib.core;`, a macro making the header skip its include, the header inside `extern "C++"` in the purview,
  details exported from the core). It works on stock GCC and is cheaper for importers (import metalib 224M,
  metalib.reflect alone 163M), but the user found it too much of a hack. inject::before_exports was added for it
  and stays as a position (tested in codegen_test).
- GCC limits of the current structure: (1) a module that includes headers and also imports a module including the
  same headers fails with "mangling of ... operator<=> (const __normal_iterator&...) conflicts with a previous
  mangle" (libstdc++ declarations not merged), even with the patch, so metalib.reflect / metalib.codegen cannot
  re-export metalib.core; (2) importers must include <meta>: GCC's std::meta implementation looks up by name, in
  the importing file, std::vector, std::array, std::ranges::{begin,end,data,size,contiguous_range,range_value_t,
  range_reference_t}, std::meta::{exception,operators,reflect_constant_array} and the type traits (reflect.cc).
  Exporting those names from a module (`export namespace std { using ::std::vector; }`) does satisfy the lookups
  (probed), but a file without <meta> cannot name access_context etc. anyway, so nothing is injected.
- The generator uses metalib's own API (namespace_::get_members, holds / into, get_annotations_with_type on
  basic_interface<ability::annotatable>, whose check is false for templates, so no exception handling).
- GCC trunk limitation (minimal repro in the session scratchpad, five spellings tried): an alias template whose
  target is a dependent splice (`using except = [: f(^^Set, {^^Removed...}) :]`) cannot be merged when the header
  reaches a translation unit through two modules, or through #include plus import: "conflicting imported
  declaration". Root cause: structural_comptypes (cp/typeck.cc) has no SPLICE_SCOPE case, and is_matching_decl
  (cp/module.cc) compares the alias targets with same_type_p. Fixed by
  patches/gcc-reflect-splice-scope-comptypes.patch, installed in C:/mingw64-gcc-trunk on 2026-11-26 ([[status]]).
  Not reported to GCC. patches/Dockerfile (written by a subagent; parses, patches verified to apply to the pinned
  commit 871ec62eab32, image never built) builds a Linux compiler with the patches.
- Templates cannot be queried for annotations (annotations_of throws; a specialization can), which is why exported
  goes on namespaces. GCC lists deduction guides as members named `__dguide_<class>`: names starting with two
  underscores are skipped. An unnamed namespace cannot be the marked wrapper (internal linkage).
- Exports reviewed 2026-11-26: tag types are not exported, only values. A mark on a class redeclaration after the
  definition does not work on GCC trunk ("type attributes ignored after type is already defined", annotations_of
  gives 0; before the definition it gives 1; functions accumulate); the standard says the attribute appertains to
  the class ([dcl.type.elab]/2), so this looks like a GCC bug; not reported.
- Still exported although the docs never name them: basic_string_literal, basic_predicate, basic_transform
  (templates, and the types of public values).
- Namespace aliases (probed): `export namespace N { namespace a = ::target; }` is what an importer needs; a
  using-declaration cannot name a namespace; an exported alias shows only the exported members of its target.
- `module_test` (plain executable) imports the module; `codegen_test` (plain executable, METALIB_CODEGEN) checks
  the generator on a small namespace tree. Every check is a static_assert.

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
