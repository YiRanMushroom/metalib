+++
title = "Class types: kinds, member queries, literals"
+++

Code: [[src/lib/include/metalib/handle.hpp]] (kinds, queries), [[src/lib/include/metalib/literal.hpp]] (array_literal, string_literal).
Verified on GCC trunk 2026-11-25 by scratchpad class_check.cpp (every query compared with the std::meta call, same order).

## Rules the user set
- Be careful about what a query returns: each one must equal a specific std::meta call (below), in the same order.
- Query signature: `members<Kind>(ctx, filter, map)`, all three defaulted; ctx = access_context::current()
  (as a default argument it means the caller's scope: verified, outside sees 3 data members, a friend sees 4).
- Kinds split only where abilities differ a lot. Templates (member templates, deducing-this templates) are not
  handled yet: to be discussed next.
- Narrowing that cannot fail uses the kind's explicit constructor; try_into / T::from are for checked narrowing.

## Record kinds (2026-11-25 update)
- class_type: is_class_type && complete (no unions). union_type: is_union_type && complete. Both get the shared
  ability::record: members<Kind>(ctx, filter, map), destructor(ctx), has_inaccessible_nonstatic_data_members(ctx).
  class_type alone has bases(...), has_inaccessible_bases/subobjects. accessible::parent() returns type (a member's
  parent may be a class or a union); narrow with try_into.
- nested_type : type, accessible. holds: is_type && is_class_member. Covers nested classes (also incomplete,
  declared-only ones), enums, typedef/using aliases, and the unnamed type behind an anonymous union/struct.
  type::is_alias() (is_type_alias) tells alias declarations apart; type comparison still dealiases.
- Anonymous union `union { int x; float y; };` in a class gives two members: an unnamed nested union type and an
  unnamed non-static data member of that type (nonstatic_data_members_of includes it). x and y are members of the
  union type, reached by d.type_of().try_into<union_type>(). GCC's anonymous struct extension is the same shape.
- members_of never lists using-declarations (using B::f), friend declarations or static_assert.
- Helpers in [[src/lib/include/metalib/query.hpp]]: basic_predicate (and_/or_/not_ and && || !; `or`/`and` are
  reserved alternative tokens so cannot be names), predicate::from/of_name/of_operator/of_type(t)/of_type<T>();
  basic_transform with transform::from and and_then only (user decision). Both call through std::invoke, so a member
  function pointer such as &member_function::is_const can be passed. Ability functions that are templates
  (`this auto self`) cannot be passed this way; non-template `this handle const &self` ones can (verified).

## class_type : type (original notes)
- from: is_type && (is_class_type || is_union_type) && is_complete_type. Queries call std::meta with dealias(info).
- members<Kind = member>(ctx, filter, map) -> std::vector<map result> (array_literal before 2026-11-25); Kind must be one of the member kinds below.
  - member: members_of(cls, ctx) (all, includes unnamed bit-fields, member templates, nested types, aliases,
    implicitly declared special members that exist).
  - data_member: nonstatic_data_members_of(cls, ctx) (excludes unnamed bit-fields, so it is NOT members_of filtered).
  - static_data_member: static_data_members_of(cls, ctx).
  - member_function / static_member_function / constructor: members_of(cls, ctx) filtered by Kind::holds.
- bases(ctx, filter, map) -> bases_of(cls, ctx).
- destructor(ctx) -> meta_result<destructor>: the is_destructor entry of members_of (only the selected
  prospective destructor is ever listed), failure if none is listed in that access context.
- has_inaccessible_bases / _nonstatic_data_members / _subobjects(ctx).
- filter takes the Kind by value; map's result must be structural (handles, string_literal, ...), it is stored
  with define_static_array. Passing `{}` for filter keeps the default.

## Member kinds (each has static holds(info) used by from and by the queries)
- parameter: is_function_parameter. identifiable, displayable, annotatable, typed; has_default_argument,
  is_explicit_object_parameter.
- member: is_class_member. identifiable, displayable, annotatable, accessible.
- data_member: is_nonstatic_data_member. + typed, sized; offset (member_offset), bit_size, is_bit_field, is_mutable,
  has_default_member_initializer.
- static_data_member: is_variable && is_static_member. + typed, sized; storage duration and linkage predicates.
- member_function: function, class member, not static, not ctor/dtor (deducing-this functions are here, per the
  standard they are non-static). + typed, callable, signature, overridable; has_explicit_object_parameter
  (parameters_of non-empty and first is_explicit_object_parameter), cv/ref qualifiers, is_explicit, is_conversion,
  is_special_member, is_copy/move_assignment.
- static_member_function: function, class member, static (e.g. static operator()). + typed, callable, signature.
- constructor: is_constructor. displayable, annotatable, accessible, callable; is_explicit, is_default/copy/move_constructor.
  No identifier, no typed (type_of throws "reflection does not have a type"), never take its address (GCC trunk even
  accepts extract of a constructor pointer and then fails to link).
- destructor: is_destructor. displayable, annotatable, accessible, callable, overridable.
- base: is_base. displayable, annotatable, typed (base class type), accessible (parent = derived class); is_virtual, offset.
- Abilities: accessible (is_public/protected/private, is_accessible(ctx), parent() -> class_type),
  callable (parameters() -> std::vector<parameter>, noexcept/deleted/defaulted/user_provided/user_declared/vararg),
  signature (return_type, is_operator, operator_kind -> meta_result<operators>), overridable (virtual/pure/override/final).

## Literals (structural, so usable as template arguments and as map results)
- array_literal<T>: public pointer/length, view_interface (size, [], empty, data), contiguous, borrowed; consteval
  constructor from any range (copied with define_static_array); converts to std::span<T const>.
- basic_string_literal<CharT> (string_literal, u8string_literal): public pointer/length, null-terminated, consteval
  constructor from string_view / CharT const* (copied with define_static_string); converts to string_view; == and <=>
  with string_view; default is an empty null-terminated string.
- ascii_identifier/u8_identifier/ascii_display_string return meta_result<(u8)string_literal>; u8_display_string
  returns u8string_literal. enumerators/annotations/template_arguments return std::vector (see [[build.compile-cost]]).
- std::span and std::string_view are not structural (private members), which is why these exist.
