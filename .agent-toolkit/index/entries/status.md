+++
title = "Status"
+++

## Where it stands (2026-11-25)
- Library complete for the basic reflection API: every std::meta query is wrapped (the only ones not wrapped are
  access_context members scope/via/designating_class, std::meta::exception members, std::is_string_literal).
  Code: src/lib/include/metalib/ (handle.hpp, meta_result.hpp, literal.hpp, query.hpp, encoding.hpp, metalib.hpp).
  enum.hpp (old sample) was removed.
- Design: [[design.flat-abilities]] (kinds = ability sets, layout of handle.hpp, untyped parent, naming),
  [[design.class-types]], [[design.namespaces]], [[design.misc-queries]], [[design.templates]],
  [[design.annotations]]; older background: [[design.handle-abilities]], [[reflection.universal-queries]],
  [[reflection.handle-prototype]], [[reflection.expansion-statements]]. Build: [[build.cmake-setup]].
- User docs: README.md and docs/reference.md (generated from handle.hpp by hand-run script; regenerate or edit when
  kinds/abilities change).

## Verified
- GCC trunk (C:/mingw64-gcc-trunk, gcc-17-20261122) on Windows: CMake build + ctest 14/14 (src/test, one executable
  per file, mostly static_asserts; identifier_test also built with METALIB_HAS_META_EXCEPTION=0), 2026-11-25.
- Not verified: any clang (clang-p2996 still uses the old consteval-only type model and is expected to reject parts),
  GCC 16.x (rejects meta_result with info), compile-time cost on large code.

## Toolchains (this machine)
- C:/mingw64-gcc-trunk: GCC 17 trunk cross-built in WSL by ~/wgcc/build.sh (gmp/mpfr/mpc for the build side in
  ~/wgcc/tools, ~/wgcc/nsys/mingw symlink, wipe stale target config.cache), overlaid on the MinGW-Builds 16.2 tree.
- WSL Ubuntu-22.04: ~/opt/gcc-trunk-20261124, ~/opt/cmake-4.5.1-linux-x86_64, ~/opt/ninja-v1.13.2; Catch2 discovery
  needs XDG_RUNTIME_DIR=/tmp. C:/mingw64-gcc-16.2 and 16.1 kept for comparison.
- Windows scheduled task metalib-gcc-trunk (one-time build helper) still exists; delete with
  Unregister-ScheduledTask metalib-gcc-trunk.

## Open / next
- Git: private GitHub repo YiRanMushroom/metalib (personal account; this repo's git config uses the personal email).
- Possible next steps: compile-time measurements; clang support once clang adopts P4101; ergonomic helpers on top
  of the thin layer (the user wants thin translation first).
