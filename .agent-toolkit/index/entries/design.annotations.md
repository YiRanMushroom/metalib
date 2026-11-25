+++
title = "Annotations: where and how you ask changes the answer"
+++

Probes: scratchpad annotations/ (p0-p11b*.cpp, std_wording_raw.txt). GCC trunk 2026-11-25. Code: annotatable /
annotation in [[src/lib/include/metalib/handle.hpp]].

Standard rule: annotations_of collects the annotations of every declaration reachable from the point of the outer
constant evaluation, in source order (functions/parameters: all declarations of the function or its template; a
base: only its base-specifier). So results depend on where you ask by design; a consteval helper defined early
but called later sees later declarations.

- A parameter's TYPE never carries the parameter's annotations: void g([[=5]] S s) -> annotations_of(type_of(param))
  is S's own; const S& / S* give empty (no declarations: remove_cvref first). GCC type_of drops alias sugar and
  top-level const, so an alias's annotations are lost through type_of(param).
- Parameter annotations accumulate across declarations on parameters_of(^^f)[i]; inside the body ^^z /
  variable_of(param) only has the definition's. In a lambda body ^^p has none.
- annotations_of(alias) gives only the alias's own (no dealias); dealias for the class's. Namespace alias: empty.
- Base specifier and base type are separate: struct D : [[=1]] B -> base [1], type_of(base) B's own.
- Functions, variables, reopened namespaces, out-of-class member definitions accumulate.
- Templates: annotations_of on the template itself throws. Explicit / partial specializations have only their own.
  substitute(^^X,{int}) == ^^X<int>.
- GCC deviations: class redeclarations after the definition drop annotations (warning); opaque enum declaration
  counts as definition; specializations list newest-first; a specialization keeps the annotations it had when first
  named (snapshot); function template specialization parameters see only the definition's annotations;
  annotating / reflecting a block-scope extern's parameter is a hard error.
- annotations_of_with_type ignores const, matches through aliases; int& gives empty; an incomplete type throws.

Library consequences: read parameter annotations from parameters_of(^^f)[i] (metalib: function.parameters()[i]
.annotations()), never from the type; dealias / remove_cvref explicitly depending on which annotations you want;
do not rely on annotation order for template specializations.
