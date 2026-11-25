+++
title = "Which std::meta queries work on every reflection (GCC 16 measurement)"
+++

Measured 2026-11-24 on GCC 16.1 (MinGW). The script calls each query on 15 sample reflections inside consteval
try/catch. Source: C:/Users/YIRAN~1.WAN/AppData/Local/Temp/claude/fab643b92482637d/5c8622d6-d483-4c76-b6b6-d72b6b975e9c/scratchpad/matrix.cpp

**These never throw on any sample, the null reflection included:** has_identifier, display_string_of,
source_location_of, has_parent, dealias, has_template_arguments, is_public (access predicates), has_linkage,
has_static_storage_duration, and the is_* predicates.

**These throw on some samples:**
- identifier_of: null, `int`, the global namespace, `tpl<int>` (a specialization) and `reflect_constant(42)`. Guard with has_identifier.
- parent_of: null, `int`, the global namespace and the value. Guard with has_parent.
- type_of: works only for variable, function, data member, enumerator, value, base and parameter. It throws for types,
  aliases, namespaces, templates and null.
- template_of: works only for a specialization (`tpl<int>`). Guard with has_template_arguments.
- annotations_of: throws for null, a template and a value.

Samples: null, int, class S, alias, namespace ns, global ns, variable, function, data member, enumerator, template,
tpl<int>, reflect_constant(42), base (bases_of), parameter (parameters_of).

clang-p2996 (Compiler Explorer) has **no `std::meta::exception`** (errors are not catchable there) and **no `has_parent`**.
Guarding with has_* predicates is therefore the portable style; try/catch probing is GCC-only.

Also: a non-template member function whose signature has `std::meta::info` (e.g. `operator std::meta::info() const`)
must be `consteval` on GCC 16 ("function of consteval-only type must be declared 'consteval'").
