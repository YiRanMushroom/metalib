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
