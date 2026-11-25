+++
title = "Flattened abilities: every kind is an alias of basic_interface<>::with_ability"
+++

Code: [[src/lib/include/metalib/handle.hpp]]. Verified on GCC trunk 2026-11-25 (all scratchpad checks, project ctest).

- An ability is an empty struct whose functions take `this handle const &self` (non-template, so
  `&member_function::is_noexcept` is a plain function pointer usable in predicate::from / transform::from, and each
  function compiles once instead of once per kind). Functions returning kinds are declared in the ability and
  defined out of line after basic_interface.
- `ability_set<A...>` is an alias: it flattens std::tuple arguments recursively down to base abilities, sorts
  them by std::meta::type_order, removes duplicates and names std::tuple<...>. `X::with_ability<A...>` does the same
  with X::abilities prepended and names basic_interface<...>. Arguments are abilities or ability sets only, never
  kinds: to add another kind's abilities write `class_type::abilities`. Equal sets give the same type.
- handle is only info + conversion + try_into. basic_interface<A...>::abilities = std::tuple<A...>.
- handle.hpp layout: (1) forward declarations of all abilities and overridable; (2) details::ability_sets: one
  ability_set per kind, composed from earlier sets (class_type = ability_set<type, record, class_record>,
  nested_class = ability_set<nested_type, class_type, non_alias>); (3) every kind in one block,
  `using X = basic_interface<>::with_ability<details::ability_sets::X>;`; (4) all abilities; (5) out-of-line
  definitions. Nothing before (4) instantiates a kind: ability_set only reads template arguments by reflection, and
  basic_interface<> has no ability bases. `X::with_ability` / `X::abilities` would instantiate X and need all of
  X's abilities defined, so the library itself does not use them before (4); users can.
- The template is not renamed to `interface`: Windows headers (MinGW basetyps.h) `#define interface struct`.
- Optional ability members: `static consteval bool check(info)` + `static constexpr u8string_view requirement`
  (kind's holds/from = conjunction; from reports "reflection is not <requirement>" for the first unmet one), and
  `static consteval bool equal(info, info)` (operator== uses the left kind's first equal, else info ==).
  Each check must be safe on its own (e.g. arithmetic checks is_type first), since order is type_order.
- Same-named functions live in one small ability shared through sets: virtual_specifier (is_virtual),
  override_specifier, explicit_specifier; overridable = ability_set<virtual_specifier, override_specifier>.
- Kind-defining abilities: type_entity (is_type, dealias equality, is_alias, is), arithmetic, enumeration, record
  (class or union, complete; members/destructor/has_inaccessible_nonstatic_data_members), class_record (bases ...),
  union_record, class_member, nonstatic_data, static_data, nonstatic_function, static_function, constructor_entity,
  destructor_entity, base_entity, enumerator_entity, parameter_entity.
- Parent is untyped (decided 2026-11-25): ability::parented gives has_parent() and parent() -> meta_result<handle>
  (parent_of throws without a parent: int, the global namespace). Every declaration kind has it; the user checks
  and casts (try_into). Typed parents (a former child_of<Set> ability) were dropped: the parent's own kind and
  its parents cannot all be carried in the type. Queries (members, bases, destructor) return the plain kinds.
  The ability is not named parent because a function parent() inside a struct named parent is a constructor.
- A consteval function's address cannot be stored in a constexpr variable; passing it as an argument works.
- Nested types (2026-11-25): nested_type = type + member abilities (everything, aliases and the unnamed
  anonymous-union type included). nested_class / nested_union / nested_enum = nested_type + class_type / union_type /
  enum_type + non_alias; nested_alias = nested_type + type_alias. non_alias is needed because an alias to a class
  (`using alias = defined;`) also satisfies the dealiasing class checks. Verified by scratchpad nested_check.cpp
  (order, names, parent, kind abilities, checked narrowing messages).
- Member names drop the std "_of" suffix where the member reads naturally (2026-11-25): typed::type(),
  templated::template_(). Inside the ability that declares such a member the kind must be written metalib::type /
  metalib::template_ (the member hides it). annotation keeps constant_of(): a member named constant would be
  ambiguous with the injected class name of ability::constant.
