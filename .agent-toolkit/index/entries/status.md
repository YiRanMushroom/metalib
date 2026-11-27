+++
title = "Status"
+++

## Where it stands (2026-11-26)
- Library complete for the basic reflection API: every std::meta query is wrapped (the only ones not wrapped are
  access_context members scope/via/designating_class, std::meta::exception members, std::is_string_literal).
  Code: src/lib/include/metalib/ (handle.hpp, reflect.hpp, meta_result.hpp, literal.hpp, query.hpp, encoding.hpp,
  metalib.hpp).
  enum.hpp (old sample) was removed.
- Design: [[design.flat-abilities]] (kinds = ability sets, layout of handle.hpp, untyped parent, naming),
  [[design.class-types]], [[design.namespaces]], [[design.misc-queries]], [[design.templates]],
  [[design.annotations]], [[design.kind-deduction]]; older background: [[design.handle-abilities]], [[reflection.universal-queries]],
  [[reflection.handle-prototype]], [[reflection.expansion-statements]]. Build: [[build.cmake-setup]].
- User docs: README.md and docs/reference.md (written against handle.hpp; edit them when kinds/abilities change).

## Verified
- GCC trunk (C:/mingw64-gcc-trunk, gcc-17-20261122) on Windows: CMake build + ctest 18/18 (src/test, one executable
  per file, mostly static_asserts; identifier_test also built with METALIB_HAS_META_EXCEPTION=0), 2026-11-26.
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
- Overloaded names: ^^f on an overload set is a hard error; select one overload by name (members + of_name) or pass
  an expression designating one function to reflect_function (e.g. through a cast to the function pointer type).
- Measured 2026-11-25 (GCC trunk, -fsyntax-only): <meta> alone 1.0 s, + metalib.hpp 1.6 s (parsing 84%, constant
  evaluation ~0.24 s). CLion's analysis is the slow part (60-280 s per test file, often times out).
- Code review pass 2026-11-25: std algorithms / views where they pay off (measured, see [[build.compile-cost]]),
  array_literal accepts ranges whose element type only converts to T, layout-independent tests.
- Queries returning several elements return std::vector (never static storage); materialize with array_literal;
  see [[build.compile-cost]] (2026-11-25).
- Errors (2026-11-26, decided with the user): meta_result holds a metalib::meta_error (plain value: message, action,
  from, where; throw_() throws meta_exception, which carries the meta_error). std::meta::exception is only caught,
  never rethrown or wrapped; failed operations say "unable to <verb> <call(operands)> <preposition> '<display string>' [of type 'T']",
  e.g. "unable to find common_type_with(A) from 'int'" (details::unable / call_of / template_call_of / attempt(f,
  describe); the message is built only on failure; type_if_any catches type_of's exception).
  meta_result: std::expected-style U&& constructor (forwarding, excludes meta_result / meta_error), unwrap() and
  unwrap_or(T) take an explicit object parameter so rvalue results move out; meta_result<T&> keeps working. Rejected: wrapping
  std::meta::exception (its message carries no reason; GCC substitutes with tf_none).
- Narrowing shortcuts (2026-11-26, user request): try_make_reflection<K>(info) -> meta_result<K>, make_reflection<K>(info) -> K
  (named try_reflect / reflect until the same evening);
  h.into<K>() = try_into + unwrap; meta_result<handle kind> has try_into<K>() (keeps the original error) and into<K>().
- Done 2026-11-26 : kind constructor private + friend details::reflect_unchecked, public K{r} checked
  (bench_params constant evaluation 119M -> 135M from the per-construction check; include-only unchanged); review of
  four read-only agents applied: meta_result constructor excludes other meta_results and temporaries for reference T,
  [[nodiscard]], noexcept accessors, consteval try_into/into; details::kind concept for try_into / into / reflect;
  symmetric operator==; precondition messages name the reflection; where() of "unable" errors is the failing wrapper;
  tuple-like / variant checks strip cv; moves of callables and data_member_options; std::forward for ranges;
  redundant dealias dropped; flatten constraint matches its body; reflect_constant / object / function carry std's
  constraints; METALIB_HAS_META_EXCEPTION lives in handle.hpp; ascii_to_u8 removed; string_literal comparisons are
  unambiguous; tests no longer depend on GCC spellings or member order; docs updated.
- Done 2026-11-26 : functions that can fail come as try_<verb> (meta_result) / plain (value) pairs;
  nouns became get_ / find_ / make_ verb phrases; K::try_from / K::from, try_make_reflection / make_reflection; cache::template_arguments
  holds the array ([[conventions.naming]]). Tests, README and reference updated by three parallel agents (10-14 min
  each). Full build + ctest 16/16, README examples compile, include-only compile memory unchanged (217M).
- Done 2026-11-26 , second naming round decided by the user ([[conventions.naming]]): every accessor is
  a verb phrase (get_ existing / find_ computed / make_ constructed), independent of fallibility; get_destructor;
  new fallible pairs try_get_/get_ for type, size, alignment, return_type, enumerators, base offset; the plain form of
  a pair attaches its ability (details::attach_ability_unchecked) and unwraps the try_ form; public
  try_attach_ability / attach_ability on kinds; std-style text names (get_identifier / get_u8identifier, no "ascii"); kinds have no public constructor from info.
  Full build + ctest 16/16, README examples compile, include-only compile memory 223M (was 217M).
- Out-of-line ability definitions in handle.hpp are ordered by ability and declaration order (2026-11-26); they stay
  out of line because the kinds they return are complete only after every ability is declared.
- Done 2026-11-26: kind constant (a value or an object; is_value_constant / is_object_constant); typed kinds
  typed_constant<T> / typed_object<T> / typed_annotation<T> through ability::typed_as<T> (extract() without a template
  argument); except<Set, A...> and K::without_ability<A...>; reflect_constant returns typed_constant or typed_object by
  the argument type, reflect_object typed_object, reflect_constant_string / _array object;
  get_annotations_with_type<T>(). See [[design.misc-queries]]. CLion evaluator bugs reported, see [[build.cmake-setup]].
- Done 2026-11-26 (commit 7b83d5d, pushed): mb::reflect<^^x> deduces the kind (reflect.hpp, [[design.kind-deduction]]);
  the former reflect<K>(r) / try_reflect<K>(r) are make_reflection<K>(r) / try_make_reflection<K>(r); every ability
  has a check; member and namespace_member lost annotatable; tests use reflect<^^x> at
  literal sites; new reflect_test.cpp. Full build + ctest 17/17.
- Done 2026-11-26 (commit 1e919d9, pushed): the abilities tried by reflect<R> are a registry of namespaces,
  metalib::builtin::auto_ability_root (gate functions marked [[=auto_ability_checker]], type aliases to the abilities,
  namespace aliases for shared blocks, an alias template for typed_as); reflect<R> =
  collect_abilities<R, builtin_auto_ability_registry>; the [[=auto_ability]] class mark is gone; users extend with a
  root of their own. Full build + ctest 17/17. See [[design.kind-deduction]].
- Done 2026-11-26 (commit 1e919d9, pushed; export review in the commit after it): named module. `[[=metalib::exported]]` marks (exported.hpp), generator
  src/lib/module/generate_module.cpp run at configure time, target metalib::module (METALIB_BUILD_MODULE),
  module_test. metalib::builtin is not exported; the collector scans a built registry
  (build_registry_from_namespace; builtin_auto_ability_registry is a constexpr variable in reflect.hpp), and a user
  registry holds the built-in one through a constexpr info variable. Full build + ctest 18/18. See [[build.cmake-setup]],
  [[design.kind-deduction]]; measurements in [[build.compile-cost]].
- Idea from the user (2026-11-26): the exported marks and the generator could later help users generate source
  files at compile time for their own code; nothing built for it yet (the generator walks only ^^metalib).
- Clang support once clang adopts P4101.
