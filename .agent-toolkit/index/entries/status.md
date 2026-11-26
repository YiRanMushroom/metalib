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
- GCC trunk (C:/mingw64-gcc-trunk, gcc-17-20261122) on Windows: CMake build + ctest 16/16 (src/test, one executable
  per file, mostly static_asserts; identifier_test also built with METALIB_HAS_META_EXCEPTION=0), 2026-11-25.
- Not verified: any clang (clang-p2996 still uses the old consteval-only type model and is expected to reject parts),
  GCC 16.x (rejects meta_result with info), compile-time cost on large code.

## Toolchains (this machine)
- C:/mingw64-gcc-trunk: GCC 17 trunk cross-built in WSL by ~/wgcc/build.sh (gmp/mpfr/mpc for the build side in
  ~/wgcc/tools, ~/wgcc/nsys/mingw symlink, wipe stale target config.cache), overlaid on the MinGW-Builds 16.2 tree.
- WSL Ubuntu-22.04: ~/opt/gcc-trunk-20261124, ~/opt/cmake-4.5.1-linux-x86_64, ~/opt/ninja-v1.13.2; Catch2 discovery
  needs XDG_RUNTIME_DIR=/tmp. C:/mingw64-gcc-16.2 and 16.1 kept for comparison.
- C:/mingw64-gcc-trunk/libexec/gcc/x86_64-w64-mingw32/17.0.0/cc1plus.exe is patched (2026-11-25) with
  patches/gcc-reflect-template-arguments-of-baselink.patch (template_arguments_of ICE on ^^S::mf specializations);
  the stock binary is cc1plus.unpatched.exe next to it. Rebuild after editing ~/wgcc/src/gcc-17-20261122:
  PATH=~/wgcc/tools/bin:~/wgcc/cross/bin:$PATH make -C ~/wgcc/b/native/gcc -j10 cc1plus.exe, then copy cc1plus.exe.
  Verified with the patched compiler: clean rebuild + ctest 16/16, the ICE reproducer compiles.
- Windows scheduled task metalib-gcc-trunk (one-time build helper) still exists; delete with
  Unregister-ScheduledTask metalib-gcc-trunk.

## Open / next
- Git: private GitHub repo YiRanMushroom/metalib (personal account; this repo's git config uses the personal email).
- Helper review with the user (2026-11-25): the thin layer is considered sufficient. Not wanted (easy enough with the
  current API): field access / for_each_field, enum utilities, annotation_of<T> (annotations() + filter),
  is_specialization_of / type_name, aggregate builders (define_aggregate is direct enough).
  Done the same day: metalib::cache (variable templates members / bases / subobjects / enumerators / parameters /
  template_arguments keyed by a kind value; access::unprivileged or access::unchecked since a variable template cannot
  see the caller's scope; annotations not cached) and message.hpp (message(parts...) -> std::string usable as a
  static_assert message; to_string / to_u8string). Narrowing failures now read "reflection '<display>' is not <req>".
  300 repeated member queries: 4.0 s direct vs 2.5 s cached (base include 1.6 s).
- Overloaded names: ^^f on an overload set is a hard error; select one overload by name (members + of_name) or pass
  an expression designating one function to reflect_function (e.g. through a cast to the function pointer type).
- Measured 2026-11-25 (GCC trunk, -fsyntax-only): <meta> alone 1.0 s, + metalib.hpp 1.6 s (parsing 84%, constant
  evaluation ~0.24 s). CLion's analysis is the slow part (60-280 s per test file, often times out).
- Code review pass 2026-11-25: std algorithms / views where they pay off (measured, see [[build.compile-cost]]),
  array_literal accepts ranges whose element type only converts to T, layout-independent tests.
- Queries returning several elements return std::vector (never static storage); materialize with array_literal;
  see [[build.compile-cost]] (2026-11-25).
- Clang support once clang adopts P4101.
