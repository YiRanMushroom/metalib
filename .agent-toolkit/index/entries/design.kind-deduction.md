+++
title = "reflect<R>: the kind deduced from a reflection passed as a template argument"

[[links]]
path = "src/lib/include/metalib/reflect.hpp"
snippet = "consteval std::meta::info deduced_kind(std::meta::info r) {"

[[links]]
path = "src/lib/include/metalib/handle.hpp"
snippet = "inline constexpr auto_ability_t auto_ability{};"

[[links]]
path = "src/test/reflect_test.cpp"
snippet = "consteval bool names_are_unique(std::meta::info kind) {"
+++

Decided with the user 2026-11-26. `mb::reflect<^^x>` (variable template, reflect.hpp) is the handle with every
ability that holds; `make_reflection<K>(r)` / `try_make_reflection<K>(r)` (the former reflect / try_reflect) stay for
a reflection that is not a constant expression (function parameter, ordinary loop variable).

How it works
- Every ability has `static consteval bool check(std::meta::info)` and is marked `[[=auto_ability]]`
  (`typed_as<T>` is a template and is not marked). The scan takes the marked classes of namespace metalib::ability,
  sorts them with type_less (canonical order, so the kept subset needs no sorting) and substitutes basic_interface.
- A value, object or annotation gets typed_as<remove_cv(type_of)> in place of ability::constant, so the result converts
  to typed_constant / typed_object / typed_annotation (the user asked for this instead of deleting those cases).
- The `template for` over the abilities sits in `deduced_kind<Marker>(r)`: the range depends on Marker, so it is
  expanded once per translation unit, at the first use. Abilities declared after that are not seen; a user ability
  must be declared in metalib::ability before the first use, the same in every TU.
- Messages: an ability has either `requirement` ("is not <requirement>") or `capability` ("cannot <capability>");
  try_from reports an unmet requirement before an unmet capability, so make_reflection<mb::variable>(^^S) still says
  "is not a variable". Abilities that say what a reflection is use requirement; abilities that say what it can do use
  capability (otherwise the reported one would depend on type_order). A check with neither text is reported as
  "does not have the ability '<ability>'".
- An object of array type keeps ability::constant (typed_as<int[3]> would declare a function returning an array);
  reflect_object(array) returns object for the same reason (it did not compile before).

Why the checks are what they are (GCC trunk, probed)
- annotations_of throws for templates, concepts, values, objects, data member descriptions and the null reflection, so
  `member` and `namespace_member` (which hold templates too) lost annotatable; the kinds built on them add it.
- is_namespace_member is true for ^^int, ^^int * and for reflect_object(x) on GCC; none has a parent, so
  namespace_member_entity also requires has_parent.
- non_alias used to hold for every non-type (it was only "not a type alias"); it now requires is_type.
- size_of works on types, objects, values, variables, non-static data members, bases and data member descriptions;
  type_of throws for constructors and destructors.
- A check must accept everything each named set containing the ability accepts (make_reflection<K> and the
  Kind::holds filter of get_members use it), and must not accept a reflection on which a plain (non-try_) function of
  the ability throws.
- Abilities share member names (get_members, make_signed, is_const, is_final, get_offset, get_bit_size,
  get_operator_kind); reflect_test checks through reflection that no deduced kind has the same name from two abilities.

Measured (GCC trunk, -fsyntax-only, 300 distinct variables)
- no reflection 1.9 s / 228M; make_reflection<mb::variable> 2.1-2.3 s / 244M; reflect<^^v> 2.4-2.8 s / 292M
  (about 0.2 MB and under 2 ms per distinct reflection).
- Of the 60M over the floor (no checks at all: 232M), scanning half of the abilities costs 259M: an early-exit
  hierarchy of checks could save at most about 0.1 MB per distinct reflection, and no measurable time.
- Rejected implementations: one variable template per (ability, R) or substitute + extract per ability
  (1.6-2.1 GB for the same file), per-R expansion of the template for (1.6 GB), evaluating the kind twice.
- Other cost against HEAD: include-only 228M -> 230M; filtering 600 members 264M -> 267M.
- A static_assert message that is not a constant variable is re-evaluated per character by GCC: build it once with
  std::define_static_string, or a long message takes minutes and gigabytes.

Open (discussed, not decided): arranging the abilities in nested namespaces with one check per namespace so that
whole groups are skipped; typed_as for variables and enumerators too.
