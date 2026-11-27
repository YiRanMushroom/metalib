+++
title = "Compile cost: how to measure, what helps"
+++

Code: [[src/lib/include/metalib/handle.hpp]] (details::static_handles, details::select). GCC trunk, 2026-11-25.

- Wall-clock timing of single compiles is useless here (the same 1.6 s file measured 1.65-5.5 s; CLion's background
  analysis adds more noise). GCC's -ftime-report time columns overflow on Windows. Use the memory column of
  -ftime-report ("constant expression evaluation", "template instantiation", TOTAL): it is identical between runs.
- -fconstexpr-ops-limit bisection does not isolate one query: the limit is hit first by the largest evaluation in
  the header (the kind aliases, ~118k operations), so every probe reports the same number.
- Results (constant-evaluation memory, old -> new): 400 parameters() 240M -> 144M, 300 members<data_member>
  303M -> 248M, 200 members with filter + map 856M -> 727M; include only 224M -> 230M total.
- What helped: building array_literal straight from infos | views::transform (sized, one pass, no vector<H> copy);
  reserve() before push_back loops.
- What hurt: a views::filter pipeline into define_static_array (filtered range is unsized, so it is walked more
  than once and the user's filter runs repeatedly; ~2x slower with a filter that itself queries). select keeps a
  loop so each filter runs once per element.
- Benchmarks: scratchpad bench_params.cpp, bench_direct.cpp, bench_mix.cpp, t_lib.cpp.
- Decision (2026-11-25, with the user): queries returning several elements return std::vector<K> (built with a sized
  vector and indexed assignment; filter via std::erase_if, map into a second sized vector) and are never promoted to
  static storage; users materialize with array_literal (CTAD) when needed; cache:: variable templates materialize.
  Measured on 300 queries: push_back 165M, ranges::to 139M, iterator ctor 112M, sized + indexed 97M, lazy owning
  transform view 89M (std call alone 56M). Result vs the previous static-array design: direct queries 248M -> 173M,
  parameters 144M -> 119M, filter+map 727M -> 633M; include only 230M -> 218M total.
- Rejected: returning lazy views (filter_view::begin is not const, so a constexpr filter view is unusable; views are not
  structural; per-call-site view instantiations: template-instantiation memory 132M -> 1173M on the filter+map bench).

## Header unit and named module (probed 2026-11-26, GCC trunk, nothing in the repo uses this yet)
- metalib.hpp compiles as a header unit and reflection works through it. Include only: 1.8 s / 230M textual, 0.64 s / 5M
  imported. A file with 30 reflect<^^x> of different categories: 2.2 s / 277M textual, 1.2 s / 124M imported.
- Freezing the built-in registry in the header (forcing collected_kind<^^builtin::auto_ability_root> once) changes
  nothing measurable for importers (124M either way) and costs +39M in a textual include that never uses reflect.
- Named module (global module fragment includes metalib.hpp, names exported by using-declarations): members_of in an
  importer sees only exported declarations (the standard: a declaration of an imported unit precedes a point only
  when exported, [basic.lookup.general]/2), so a registry scanned lazily inside templates is empty there and
  reflect<^^x> was silently basic_interface<>. Exporting the registry namespaces fixes it (198M for the
  30-reflection file) but exposes them. Two things work without exporting. (a) A non-template function in the module
  that names collected_kind<^^builtin::auto_ability_root>: the specialization is then instantiated in the module's
  translation unit and importers reuse it (147M, 1.3 s; include 2.1 s / 290M; header unit 1.2 s / 124M); a
  specialization nothing exported refers to is dropped; in the header that function costs every textual include +39M.
  (b) Build the registry in a non-template constexpr variable in the header ([[design.kind-deduction]]): its
  initializer is evaluated at the definition, importers use the value, and a registry of the importer's own can hold
  it. With (b) alone the collector is instantiated in every importer: 197M / 1.4 s for the 30-reflection file, of
  which 39M is constant evaluation done once per translation unit (entry lists per registry node, the sorted ability
  list, each entry's index); a single reflect<^^x> is cheap (30 of them: 5M).
  What keeps that instantiation in the module for importers (probed 2026-11-26, all 147M / 1.1 s): a non-template
  function naming the specialization, or an explicit instantiation
  `template std::meta::info metalib::reflection::reflect_details::collected_kind<...>(std::meta::info);`,
  in the purview or in the header. What does not (197M): a consteval block calling it, in the header or in the
  purview: nothing that stays refers to the specialization, so it is not written to the module.
  The module gets the explicit instantiation in its purview, through an inject at_back annotation in reflect.hpp
  ([[build.cmake-setup]]). In the header it would cost every textual include that never uses reflect +49M
  (238M -> 287M), and an explicit instantiation definition may appear only once in a program.
  Split into metalib.core / metalib.reflect / metalib.codegen under metalib (2026-11-26, [[build.cmake-setup]]),
  each interface including its headers (current): import metalib 266M, import metalib.core + metalib.reflect 193M,
  import metalib.core alone 134M; textual include 301M. Three BMIs each hold the core (12M, 12M, 17M on disk) and the
  importer merges them, which is where the gain over the include goes. Submodules importing the core instead
  (dropped): 224M / 163M. One module holding everything: 182M with the generator in it, 147M without.
- The importer must still provide the std declarations itself ("'std::ranges' is not a namespace" otherwise).
- `export module metalib; export import <metalib/metalib.hpp>;` works and costs the same as the header unit: it gives
  `import metalib;` without generating anything, but exports everything the header declares.
- An annotation can be queried on classes, type aliases, functions, variables and namespaces, not on any template
  (annotations_of throws). 23 of the 81 members of namespace metalib are templates, so a [[=exported]] mark cannot
  drive an export list by itself.
