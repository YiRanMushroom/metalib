# metalib reference

Written against `src/lib/include/metalib/handle.hpp`. Every kind is
`basic_interface<>::with_ability<details::ability_sets::X>`; the sets below are written as in the header (a bold name
is another kind's ability set, flattened in).

## Kinds

| kind | ability set |
|---|---|
| `type` | `parented`, `identifiable`, `displayable`, `sized`, `type_traits`, `type_transforms`, `templated`, `annotatable`, `type_entity` |
| `arithmetic_type` | **type**, `arithmetic` |
| `enum_type` | **type**, `enumeration` |
| `class_type` | **type**, `record`, `class_record` |
| `union_type` | **type**, `record`, `union_record` |
| `parameter` | `parented`, `identifiable`, `displayable`, `annotatable`, `typed`, `parameter_entity` |
| `enumerator` | `identifiable`, `displayable`, `annotatable`, `typed`, `constant`, `enumerator_entity`, `parented` |
| `member` | `parented`, `identifiable`, `displayable`, `annotatable`, `accessible`, `class_member` |
| `data_member` | **member**, `typed`, `sized`, `nonstatic_data`, `constant` |
| `static_data_member` | **member**, `templated`, `typed`, `sized`, `linkage`, `storage_duration`, `static_data`, `constant` |
| `member_function` | **member**, `templated`, `typed`, `callable`, `signature`, `overridable`, `explicit_specifier`, `nonstatic_function`, `constant` |
| `static_member_function` | **member**, `templated`, `typed`, `callable`, `signature`, `static_function`, `constant` |
| `constructor` | `parented`, `displayable`, `annotatable`, `accessible`, `callable`, `explicit_specifier`, `constructor_entity` |
| `destructor` | `parented`, `displayable`, `annotatable`, `accessible`, `callable`, `overridable`, `destructor_entity` |
| `base` | `displayable`, `annotatable`, `typed`, `accessible`, `virtual_specifier`, `base_entity`, `parented` |
| `nested_type` | **type**, **member** |
| `nested_class` | **nested_type**, **class_type**, `non_alias` |
| `nested_union` | **nested_type**, **union_type**, `non_alias` |
| `nested_enum` | **nested_type**, **enum_type**, `non_alias` |
| `nested_alias` | **nested_type**, `type_alias` |
| `namespace_` | `parented`, `identifiable`, `displayable`, `annotatable`, `namespace_entity` |
| `namespace_alias` | `parented`, `identifiable`, `displayable`, `namespace_alias_entity` |
| `namespace_member` | `parented`, `identifiable`, `displayable`, `annotatable`, `namespace_member_entity` |
| `variable` | **namespace_member**, `templated`, `typed`, `sized`, `linkage`, `storage_duration`, `variable_entity`, `constant` |
| `function` | **namespace_member**, `templated`, `typed`, `callable`, `signature`, `linkage`, `function_entity`, `constant` |
| `constant` | `displayable`, `typed`, `constant`, `constant_entity` |
| `typed_constant<T>`, `typed_object<T>`, `typed_annotation<T>` | **constant** / **object** / **annotation** without `constant`, plus `typed_as<T>` |
| `value` | `displayable`, `typed`, `constant`, `value_entity` |
| `object` | `displayable`, `typed`, `constant`, `object_entity` |
| `annotation` | `displayable`, `typed`, `constant`, `annotation_entity` |
| `template_` | `parented`, `identifiable`, `displayable`, `linkage`, `template_entity` |
| `member_template` | **template_**, `accessible`, `class_member` |
| `data_member_description` | `identifiable`, `typed`, `data_member_description_entity` |
| `structured_binding` | `parented`, `identifiable`, `displayable`, `structured_binding_entity` |

## Narrowing

An operation that can fail comes as a pair: `try_<verb>()` returns `meta_result<T>`, and the plain name returns `T` (it
is `try_<verb>().unwrap()`, so a failure is a compile error showing the message). Names are verbs, with `get_`, `find_`
or `make_` before a noun (`try_get_parent` / `get_parent`): `get_` for something that already exists, `find_` for
something computed, `make_` for something constructed. An operation that cannot fail has only the plain form
(`get_members`, `get_u8display_string`). Narrowing follows the same rule:

| Call | Result |
|---|---|
| `try_reflect<K>(r)` | `meta_result<K>`: `handle{r}.try_into<K>()` |
| `reflect<K>(r)` | `K`: `try_reflect<K>(r).unwrap()` |
| `h.try_into<K>()`, `K::try_from(h)` | `meta_result<K>`, failing with the first unmet requirement |
| `h.into<K>()`, `K::from(h)` | `K`: `h.try_into<K>().unwrap()` |
| `result.try_into<K>()`, `result.into<K>()` | the same on a `meta_result` holding a handle; a failed `result` keeps its error |
| `k.try_attach_ability<A...>()`, `k.attach_ability<A...>()` | `meta_result<K::with_ability<A...>>` / `K::with_ability<A...>`: more abilities on the same reflection |
| `K::without_ability<A...>`, `except<Set, A...>` | the kind / ability set without those abilities |
| `K::holds(r)` | `bool` |
| `K{}` | a null reflection |
| `details::reflect_unchecked<K>(r)` | internal: unchecked construction (the friend of a private constructor) |

## Abilities

An ability with a requirement takes part in `holds`, `try_from` / `from`, `try_into` / `into` and
`try_reflect` / `reflect`; the failure message is "reflection '<display string>' is not <requirement>". Functions are
members of every kind that has the ability; a function that can fail is listed as a pair, `try_` form first.

### `identifiable`
`has_identifier`, `has_u8identifier`, `try_get_identifier` / `get_identifier`,
`try_get_u8identifier` / `get_u8identifier`

### `displayable`
`try_get_display_string` / `get_display_string`, `get_u8display_string`,
`get_source_location`

### `sized`
`try_get_size` / `get_size`, `try_get_alignment` / `get_alignment`

### `type_traits`
`is_complete`, `is_enumerable`, `is_void`, `is_null_pointer`, `is_integral`, `is_floating_point`, `is_array`,
`is_pointer`, `is_lvalue_reference`, `is_rvalue_reference`, `is_member_object_pointer`, `is_member_function_pointer`,
`is_enum`, `is_union`, `is_class`, `is_function`, `is_reflection`, `is_reference`, `is_arithmetic`, `is_fundamental`,
`is_object`, `is_scalar`, `is_compound`, `is_member_pointer`, `is_const`, `is_volatile`, `is_trivially_copyable`,
`is_standard_layout`, `is_empty`, `is_polymorphic`, `is_abstract`, `is_final`, `is_aggregate`, `is_structural`,
`is_signed`, `is_unsigned`, `is_bounded_array`, `is_unbounded_array`, `is_scoped_enum`, `is_default_constructible`,
`is_copy_constructible`, `is_move_constructible`, `is_copy_assignable`, `is_move_assignable`, `is_swappable`,
`is_destructible`, `is_trivially_default_constructible`, `is_trivially_copy_constructible`,
`is_trivially_move_constructible`, `is_trivially_copy_assignable`, `is_trivially_move_assignable`,
`is_trivially_destructible`, `is_nothrow_default_constructible`, `is_nothrow_copy_constructible`,
`is_nothrow_move_constructible`, `is_nothrow_copy_assignable`, `is_nothrow_move_assignable`, `is_nothrow_swappable`,
`is_nothrow_destructible`, `is_implicit_lifetime`, `has_virtual_destructor`, `has_unique_object_representations`,
`get_rank`, `get_extent`, `is_base_of`, `is_virtual_base_of`, `is_convertible_to`, `is_nothrow_convertible_to`,
`is_layout_compatible_with`, `is_pointer_interconvertible_base_of`, `is_assignable_from`,
`is_trivially_assignable_from`, `is_nothrow_assignable_from`, `is_swappable_with`, `is_nothrow_swappable_with`,
`is_constructible_from`, `is_trivially_constructible_from`, `is_nothrow_constructible_from`, `is_invocable_with`,
`is_nothrow_invocable_with`, `is_invocable_r_with`, `is_nothrow_invocable_r_with`,
`reference_constructs_from_temporary`, `reference_converts_from_temporary`, `try_get_tuple_size` / `get_tuple_size`,
`try_get_variant_size` / `get_variant_size`

### `type_transforms`
`remove_const`, `remove_volatile`, `remove_cv`, `add_const`, `add_volatile`, `add_cv`, `remove_reference`,
`add_lvalue_reference`, `add_rvalue_reference`, `remove_extent`, `remove_all_extents`, `remove_pointer`, `add_pointer`,
`remove_cvref`, `decay`, `unwrap_reference`, `unwrap_ref_decay`, `try_get_tuple_element` / `get_tuple_element`,
`try_get_variant_alternative` / `get_variant_alternative`, `try_find_common_type_with` / `find_common_type_with`,
`try_find_common_reference_with` / `find_common_reference_with`,
`try_find_invoke_result_with` / `find_invoke_result_with`

### `templated`
`has_template_arguments`, `try_get_template` / `get_template`, `try_get_template_arguments` / `get_template_arguments`

### `annotatable`
`get_annotations`, `get_annotations_with_type(type)` (returns `annotation`s), `get_annotations_with_type<T>()` (returns
`typed_annotation<T>`s)

### `constant`
`try_extract` / `extract`

### `typed`
`try_get_type` / `get_type`

### `type_entity`
Requirement: a type.
`is_alias`, `is`

### `arithmetic`
Requirement: an arithmetic type.
`try_make_signed` / `make_signed`, `try_make_unsigned` / `make_unsigned`

### `enumeration`
Requirement: an enumeration type.
`get_underlying_type`, `make_signed`, `make_unsigned`, `try_get_enumerators` / `get_enumerators`

### `parented`
`has_parent`, `try_get_parent` / `get_parent`

### `record`
Requirement: a complete class or union type.
`get_members`, `try_get_destructor` / `get_destructor`, `has_inaccessible_nonstatic_data_members`

### `class_record`
Requirement: a class type.
`get_bases`, `has_inaccessible_bases`, `has_inaccessible_subobjects`, `get_subobjects`

### `union_record`
Requirement: a union type.
(check only)

### `accessible`
`is_public`, `is_protected`, `is_private`, `is_accessible`

### `type_alias`
Requirement: a type alias.
(check only)

### `non_alias`
Requirement: the type's own declaration.
(check only)

### `class_member`
Requirement: a class member.
(check only)

### `callable`
`get_parameters`, `is_noexcept`, `is_deleted`, `is_defaulted`, `is_user_provided`, `is_user_declared`, `is_vararg`

### `signature`
`try_get_return_type` / `get_return_type`, `is_operator`, `try_get_operator_kind` / `get_operator_kind`,
`is_literal_operator`,
`try_get_operator_symbol` / `get_operator_symbol`, `try_get_u8operator_symbol` / `get_u8operator_symbol`

### `overridable`
Ability set: `virtual_specifier`, `override_specifier`.

### `virtual_specifier`
`is_virtual`

### `override_specifier`
`is_pure_virtual`, `is_override`, `is_final`

### `explicit_specifier`
`is_explicit`

### `nonstatic_data`
Requirement: a non-static data member.
`get_offset`, `get_bit_size`, `is_bit_field`, `is_mutable`, `has_default_member_initializer`

### `static_data`
Requirement: a static data member.
(check only)

### `storage_duration`
`has_static_storage_duration`, `has_thread_storage_duration`, `has_automatic_storage_duration`,
`try_get_object` / `get_object`

### `linkage`
`has_linkage`, `has_internal_linkage`, `has_external_linkage`, `has_module_linkage`, `has_c_language_linkage`

### `namespace_entity`
Requirement: a namespace.
`get_members`

### `namespace_alias_entity`
Requirement: a namespace alias.
`get_target`

### `namespace_member_entity`
Requirement: a namespace member.
(check only)

### `variable_entity`
Requirement: a variable.
(check only)

### `function_entity`
Requirement: a function.
(check only)

### `typed_as<T>`
Requirement: of type `T` (cv-qualification ignored).
`try_extract` / `extract` (no template argument; returns `T`)

### `constant_entity`
Requirement: a constant.
`is_value_constant`, `is_object_constant`

### `value_entity`
Requirement: a value.
(check only)

### `object_entity`
Requirement: an object.
(check only)

### `annotation_entity`
Requirement: an annotation.
`get_constant` (returns `constant`)

### `structured_binding_entity`
Requirement: a structured binding.
(check only)

### `data_member_description_entity`
Requirement: a data member description.
`get_bit_size`, `is_bit_field`

### `template_entity`
Requirement: a template.
`is_function_template`, `is_variable_template`, `is_class_template`, `is_alias_template`,
`is_conversion_function_template`, `is_operator_function_template`, `is_literal_operator_template`,
`is_constructor_template`, `is_concept`, `try_get_operator_kind` / `get_operator_kind`, `can_substitute`,
`try_substitute` / `substitute`

### `nonstatic_function`
Requirement: a non-static member function.
`has_explicit_object_parameter`, `is_const`, `is_volatile`, `is_lvalue_reference_qualified`,
`is_rvalue_reference_qualified`, `is_conversion`, `is_special_member`, `is_copy_assignment`, `is_move_assignment`,
`is_assignment`

### `static_function`
Requirement: a static member function.
(check only)

### `constructor_entity`
Requirement: a constructor.
`is_default_constructor`, `is_copy_constructor`, `is_move_constructor`

### `destructor_entity`
Requirement: a destructor.
(check only)

### `base_entity`
Requirement: a base class relationship.
`try_get_offset` / `get_offset`

### `enumerator_entity`
Requirement: an enumerator.
(check only)

### `parameter_entity`
Requirement: a function parameter.
`has_default_argument`, `is_explicit_object_parameter`, `try_get_variable` / `get_variable`

### Notes on `extract<T>()`

`try_extract<T>()` returns `meta_result<T>` (failure message: `unable to extract<T>() from 'r' of type '...'`) and
`extract<T>()` returns `T`. Member and function pointers come straight from data members and functions
(`d.extract<int S::*>()`, `f.extract<int (S::*)() const>()`); a reference to a variable needs the object first
(`v.get_object().extract<int &>()`); reading the value of a variable that is not usable in constant expressions is a
hard compile error, not a failure.

## Free functions

| function | result |
|---|---|
| `flatten(items...)` | `std::vector<handle>`: handles and (nested) ranges of handles, in order |
| `reflect_constant(v)` | `typed_constant<T>` for a non-class `v` (a value), `typed_object<T>` for a class-type `v` (its template parameter object, a different object from `v` itself) |
| `reflect_object(o)` | `typed_object<T>`: `o` itself (o needs static storage duration) |
| `reflect_function(f)` | `handle` |
| `reflect_constant_string(r)`, `reflect_constant_array(r)` | `object` (the array as a template parameter object) |
| `try_get_current_function(ctx = current())` | `meta_result<handle>` |
| `get_current_function(ctx = current())` | `handle` |
| `try_get_current_class(ctx = current())` | `meta_result<type>` |
| `get_current_class(ctx = current())` | `type` |
| `get_current_namespace(ctx = current())` | `namespace_` |
| `try_make_data_member_spec(type, options = {})` | `meta_result<data_member_description>` |
| `make_data_member_spec(type, options = {})` | `data_member_description` |
| `try_define_aggregate(type, descriptions)` | `meta_result<type>` (call from a `consteval {}` block) |
| `define_aggregate(type, descriptions)` | `type` (call from a `consteval {}` block) |
| `global_namespace` | the `namespace_` for `^^::` (a constant) |

## Cached queries: `metalib::cache` (cache.hpp)

| variable template | value |
|---|---|
| `members<Scope, Kind = void, A = access::unprivileged>` | `array_literal` of `Scope.get_members<Kind>(ctx)` (`Kind = void`: the scope's default kind) |
| `bases<Class, A>`, `subobjects<Class, A>` | `Class.get_bases(ctx)`, `Class.get_subobjects(ctx)` |
| `enumerators<Enum>` | `Enum.get_enumerators()` |
| `parameters<Function>` | `Function.get_parameters()` |
| `template_arguments<Specialization>` | `Specialization.get_template_arguments()` |

`access::unprivileged` uses `access_context::unprivileged()`, `access::unchecked` uses `access_context::unchecked()`.

## Messages (message.hpp)

| function | result |
|---|---|
| `to_string(h)`, `to_u8string(h)` | display string of a handle |
| `message(parts...)` | `std::string` from text (anything convertible to `string_view`), handles, integers, bools, chars |

## Queries: `metalib::predicate` and `metalib::transform` (query.hpp)

- `predicate::from(f)` wraps any invocable (lambdas, `&kind::fn` member function pointers); predicates combine with
  `.and_()`, `.or_()`, `.not_()` and `&&`, `||`, `!`.
- `predicate::of_name("x")`, `predicate::of_operator(std::meta::op_plus)`, `predicate::of_type(type)`,
  `predicate::of_type<T>()`.
- `transform::from(f)`, `.and_then(g)`.

## Errors: meta_error

`meta_result<T>::error()` returns a `meta_error` (a value, not an exception):

| Member | Meaning |
|---|---|
| `u8message()`, `message()` | the message (`message()` is ASCII) |
| `u8action()` | the failed `std::meta` call with its operands, e.g. `extract<long int*>()`; empty otherwise |
| `from()`, `where()` | the reflection and the source location |
| `throw_()` | throws `meta_exception(*this)` |

`meta_exception` derives from `std::exception`; `what()` is the message and `error()` the `meta_error`. `unwrap()` calls
`error().throw_()` on failure. The plain form of a pair (`get_parent()`) is its `try_` form followed by `unwrap()`
(`try_get_parent().unwrap()`), so both give the same message. Every message names the reflection by its display string.
Kind checks read "reflection '<display>' is not <requirement>"; preconditions a query checks itself read like
"reflection '<display>' has no parent" or "type '<display>' is not tuple-like"; failed `std::meta` calls read
`unable to <verb> <call> <preposition> '<display string>'` (plus ` of type '<type>'` when the reflection has a type);
`u8action()` is the call:

| Operation | Message |
|---|---|
| `extract<T>()` | `unable to extract<T>() from 'r'` |
| `substitute(args)` | `unable to substitute(args) into '<template head>'` |
| `find_common_type_with(ts)`, `find_common_reference_with(ts)`, `find_invoke_result_with(ts)` | `unable to find common_type_with(ts) from 't'` (and so on) |
| `get_variable()`, `get_object()` | `unable to get variable() from 'r'` (and so on) |
| `get_size()`, `get_type()`, `get_alignment()`, `get_return_type()`, `get_enumerators()`, `get_offset()` of a base | `unable to get size() from 'r'` (and so on) |
| `make_data_member_spec(t, options)` | `unable to make data_member_spec() from 't'` |
| `define_aggregate(t, specs)` | `unable to apply define_aggregate(specs) to 't'` |

## Literals (literal.hpp)

- Queries returning several elements return `std::vector<K>`; materialize with `array_literal` when the result must
  outlive constant evaluation (constexpr variables, `template for`, template arguments).
- `array_literal<T>`: structural view over a `define_static_array` array (deduces `T` from a range); usable as a
  template argument and in `template for`; converts to `std::span<T const>`.
- `string_literal` / `u8string_literal`: structural null-terminated strings over `define_static_string`;
  convert to `std::string_view` / `std::u8string_view`, compare with string views.
