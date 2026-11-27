+++
title = "reflect<R>: the kind collected from a registry of namespaces"

[[links]]
path = "src/lib/include/metalib/reflect.hpp"
snippet = "consteval void collect_from(bool *present, std::vector<std::meta::info> &substituted, std::meta::info r) {"

[[links]]
path = "src/lib/include/metalib/reflect.hpp"
snippet = "namespace auto_ability_root {"

[[links]]
path = "src/test/reflect_test.cpp"
snippet = "consteval std::string mismatches() {"
+++

Decided with the user 2026-11-26. `mb::reflect<^^x>` (variable template, reflect.hpp) is the handle with every
ability the built-in registry gives; `make_reflection<K>(r)` / `try_make_reflection<K>(r)` (the former reflect /
try_reflect) stay for a reflection that is not a constant expression (function parameter, ordinary loop variable).

The registry (the user's design; wanted because it documents which abilities belong together, not for speed)
- `reflect<R>` = `collect_abilities<R, builtin_auto_ability_registry>`. A registry is written as a namespace and
  read by build_registry_from_namespace (see "Built registries" below): a function marked
  `[[=auto_ability_checker]]` is its gate; a type alias is an ability, kept when its own check holds; a nested
  namespace or namespace alias is entered in turn (aliases reuse the blocks builtin::named_entities / class_members /
  namespace_members); an alias template with one std::meta::info parameter is an ability computed from R, kept when
  can_substitute succeeds (its requires-clause is its check): this is how typed_as<T> gets in.
- The tree holds only aliases; abilities stay defined in metalib::ability, so their types and names do not change. The
  per-class [[=auto_ability]] mark of the first version is gone: the tree is the registry.
- Every ability's own check stays complete: make_reflection / holds / the get_members filter call it with no registry.
  A gate only groups and skips. The one deliberate exception is ability::constant, which the tree offers to constants
  only when they have array type (the others get typed_as<T>, whose extract() would clash with extract<T>()).
- Cycles: the namespaces must not form a cycle whatever the gates return (the user's rule). build_registry checks
  it while it walks (path of namespaces, error names the cycle); a built registry is a finite type, so it cannot
  hold a cycle and collecting keeps no visited state at all.

Built registries (the user's design, 2026-11-26; replaced lazy scanning and the define_aggregate freezing of the same day)
- What the collector scans is a class, never a namespace: `build_registry_from_namespace(^^ns)` returns the
  reflection of `details::built_registry<^^entry...>`. Entries: the gate function (only a function marked
  auto_ability_checker is kept, so "the function entry" is the gate), ability types (dealiased), alias templates, and
  for each nested namespace the built registry of that namespace. The class is only named (substitute), never
  defined or instantiated; entries are read back with template_arguments_of + extract<info>.
- `inline constexpr std::meta::info builtin_auto_ability_registry = build_registry_from_namespace(^^builtin::
  auto_ability_root);` in reflect.hpp. Why this is right everywhere: the initializer of a non-template variable is
  evaluated where the variable is defined, not where it is used or where a template is instantiated. So the registry
  does not depend on the user's translation unit, and an importer of the named module (where members_of sees nothing
  of the unexported metalib::builtin, [[build.compile-cost]]) just uses the value. No injected declaration, no
  reachability question.
- collect_abilities<R, Registry> takes only a built registry; a namespace is an error ("'x' is not an ability
  registry made by build_registry_from_namespace").
- Composition: a namespace entry that is a `constexpr std::meta::info` variable holding a built registry is entered
  like a nested namespace. This is how a user root contains the built-in one, also through the module (module_test);
  `namespace builtin = metalib::builtin::auto_ability_root;` still works with the header.
- A namespace without entries is an error of the build ("ability registry 'x' has no entries").
- Trap: `^^std::meta::info` reflects the alias; compare a type with `dealias(^^std::meta::info)`.
- Cost (GCC trunk): include-only 230M -> 238M; 300 reflections 301M -> 319M; 30 mixed reflections 291M. The
  define_aggregate version measured 239M / 322M.

Implementation notes (measured on GCC trunk)
- collect_from<Registry, Root> is one function template per namespace; the template for over its members is expanded
  once per translation unit. Each type entry knows at compile time its index in registered_abilities<Root> (all
  abilities of the root, sorted once with type_less) and sets a flag; the kind is the flagged abilities in that order,
  so nothing is sorted per reflection. Only a result with substituted (alias template) entries goes through
  canonical_interface.
- Costs for 300 distinct variables, -fsyntax-only: no reflection 228M; make_reflection<mb::variable> 245M;
  reflect<^^v> 301M / about 2.4 s. Sorting each result with canonical_interface cost 572M, std::vector<bool> flags
  444M, a plain bool array 297M. 30 reflections of different categories: 274M (named kinds 250M): the registry has a
  fixed cost of about 25M per translation unit that uses reflect.
- The flat first version (scan of marked classes) measured 292M for the same file: the tree is not faster.
- Messages: an ability has either `requirement` ("is not <requirement>") or `capability` ("cannot <capability>");
  try_from reports an unmet requirement before an unmet capability, so make_reflection<mb::variable>(^^S) still says
  "is not a variable". Abilities that say what a reflection is use requirement; abilities that say what it can do use
  capability (otherwise the reported one would depend on type_order). A check with neither text is reported as
  "does not have the ability '<ability>'".

Why the checks are what they are (GCC trunk, probed)
- annotations_of throws for templates, concepts, values, objects, data member descriptions and the null reflection, so
  `member` and `namespace_member` (which hold templates too) lost annotatable; the kinds built on them add it.
- is_namespace_member is true for ^^int, ^^int * and for reflect_object(x) on GCC; none has a parent, so
  namespace_member_entity also requires has_parent. Unscoped enumerators and structured bindings at namespace scope
  are namespace members (the completeness test found both missing from the tree).
- non_alias used to hold for every non-type (it was only "not a type alias"); it now requires is_type.
- size_of works on types, objects, values, variables, non-static data members, bases and data member descriptions;
  type_of throws for constructors and destructors.
- An object of array type keeps ability::constant (typed_as<int[3]> would declare a function returning an array);
  reflect_object(array) returns object for the same reason (it did not compile before).

Tests (reflect_test.cpp)
- mismatches(): for about 50 sample reflections the collected kind must equal "every registered ability whose check
  holds" (minus constant where typed_as is given). Add a sample when adding a category.
- every_ability_is_registered(): each class of metalib::ability is in the tree.
- names_are_unique: abilities share member names (get_members, make_signed, is_const, is_final, get_offset,
  get_bit_size, get_operator_kind); no collected kind may get the same name from two abilities.
- A static_assert message that is not a constant variable is re-evaluated per character by GCC: build it once with
  std::define_static_string, or a long message takes minutes and gigabytes.
