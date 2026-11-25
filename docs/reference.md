# metalib reference

Generated from `src/lib/include/metalib/handle.hpp`. Every kind is
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
| `value` | `displayable`, `typed`, `constant`, `value_entity` |
| `object` | `displayable`, `typed`, `constant`, `object_entity` |
| `annotation` | `displayable`, `typed`, `constant`, `annotation_entity` |
| `template_` | `parented`, `identifiable`, `displayable`, `linkage`, `template_entity` |
| `member_template` | **template_**, `accessible`, `class_member` |
| `data_member_description` | `identifiable`, `typed`, `data_member_description_entity` |
| `structured_binding` | `parented`, `identifiable`, `displayable`, `structured_binding_entity` |

## Abilities

An ability with a requirement takes part in `holds` / `from` / `try_into`; the failure message is
"reflection '<display string>' is not <requirement>". Functions are members of every kind that has the ability.

### `identifiable`
`has_ascii_identifier`, `has_u8_identifier`, `ascii_identifier`, `u8_identifier`

### `displayable`
`ascii_display_string`, `u8_display_string`, `source_location`

### `sized`
`size`, `alignment`

### `type_traits`
`is_complete`, `is_enumerable`, `is_void`, `is_null_pointer`, `is_integral`, `is_floating_point`, `is_array`, `is_pointer`, `is_lvalue_reference`, `is_rvalue_reference`, `is_member_object_pointer`, `is_member_function_pointer`, `is_enum`, `is_union`, `is_class`, `is_function`, `is_reflection`, `is_reference`, `is_arithmetic`, `is_fundamental`, `is_object`, `is_scalar`, `is_compound`, `is_member_pointer`, `is_const`, `is_volatile`, `is_trivially_copyable`, `is_standard_layout`, `is_empty`, `is_polymorphic`, `is_abstract`, `is_final`, `is_aggregate`, `is_structural`, `is_signed`, `is_unsigned`, `is_bounded_array`, `is_unbounded_array`, `is_scoped_enum`, `is_default_constructible`, `is_copy_constructible`, `is_move_constructible`, `is_copy_assignable`, `is_move_assignable`, `is_swappable`, `is_destructible`, `is_trivially_default_constructible`, `is_trivially_copy_constructible`, `is_trivially_move_constructible`, `is_trivially_copy_assignable`, `is_trivially_move_assignable`, `is_trivially_destructible`, `is_nothrow_default_constructible`, `is_nothrow_copy_constructible`, `is_nothrow_move_constructible`, `is_nothrow_copy_assignable`, `is_nothrow_move_assignable`, `is_nothrow_swappable`, `is_nothrow_destructible`, `is_implicit_lifetime`, `has_virtual_destructor`, `has_unique_object_representations`, `rank`, `extent`, `is_base_of`, `is_virtual_base_of`, `is_convertible_to`, `is_nothrow_convertible_to`, `is_layout_compatible_with`, `is_pointer_interconvertible_base_of`, `is_assignable_from`, `is_trivially_assignable_from`, `is_nothrow_assignable_from`, `is_swappable_with`, `is_nothrow_swappable_with`, `is_constructible_from`, `is_trivially_constructible_from`, `is_nothrow_constructible_from`, `is_invocable_with`, `is_nothrow_invocable_with`, `is_invocable_r_with`, `is_nothrow_invocable_r_with`, `reference_constructs_from_temporary`, `reference_converts_from_temporary`, `tuple_size`, `variant_size`

### `type_transforms`
`remove_const`, `remove_volatile`, `remove_cv`, `add_const`, `add_volatile`, `add_cv`, `remove_reference`, `add_lvalue_reference`, `add_rvalue_reference`, `remove_extent`, `remove_all_extents`, `remove_pointer`, `add_pointer`, `remove_cvref`, `decay`, `unwrap_reference`, `unwrap_ref_decay`, `tuple_element`, `variant_alternative`, `common_type_with`, `common_reference_with`, `invoke_result_with`

### `templated`
`has_template_arguments`, `template_`, `template_arguments`

### `annotatable`
`annotations`, `annotations_with_type`

### `constant`
`extract`

### `typed`
`type`

### `type_entity`
Requirement: a type.
`is_alias`, `is`

### `arithmetic`
Requirement: an arithmetic type.
`make_signed`, `make_unsigned`

### `enumeration`
Requirement: an enumeration type.
`underlying_type`, `make_signed`, `make_unsigned`, `enumerators`

### `parented`
`has_parent`, `parent`

### `record`
Requirement: a complete class or union type.
`members`, `destructor`, `has_inaccessible_nonstatic_data_members`

### `class_record`
Requirement: a class type.
`bases`, `has_inaccessible_bases`, `has_inaccessible_subobjects`, `subobjects`

### `union_record`
Requirement: a union type.
(check only)

### `accessible`
`is_public`, `is_protected`, `is_private`, `is_accessible`

### `type_alias`
Requirement: a type alias.
(check only)

### `non_alias`
Requirement: a declaration that is not a type alias.
(check only)

### `class_member`
Requirement: a class member.
(check only)

### `callable`
`parameters`, `is_noexcept`, `is_deleted`, `is_defaulted`, `is_user_provided`, `is_user_declared`, `is_vararg`

### `signature`
`return_type`, `is_operator`, `operator_kind`, `is_literal_operator`, `operator_symbol`, `u8_operator_symbol`

### `virtual_specifier`
`is_virtual`

### `override_specifier`
`is_pure_virtual`, `is_override`, `is_final`

### `explicit_specifier`
`is_explicit`

### `nonstatic_data`
Requirement: a non-static data member.
`offset`, `bit_size`, `is_bit_field`, `is_mutable`, `has_default_member_initializer`

### `static_data`
Requirement: a static data member.
(check only)

### `storage_duration`
`has_static_storage_duration`, `has_thread_storage_duration`, `has_automatic_storage_duration`, `object`

### `linkage`
`has_linkage`, `has_internal_linkage`, `has_external_linkage`, `has_module_linkage`, `has_c_language_linkage`

### `namespace_entity`
Requirement: a namespace.
`members`

### `namespace_alias_entity`
Requirement: a namespace alias.
`target`

### `namespace_member_entity`
Requirement: a namespace member.
(check only)

### `variable_entity`
Requirement: a variable.
(check only)

### `function_entity`
Requirement: a function.
(check only)

### `value_entity`
Requirement: a value.
(check only)

### `object_entity`
Requirement: an object.
(check only)

### `annotation_entity`
Requirement: an annotation.
`constant_of`

### `structured_binding_entity`
Requirement: a structured binding.
(check only)

### `data_member_description_entity`
Requirement: a data member description.
`bit_size`, `is_bit_field`

### `template_entity`
Requirement: a template.
`is_function_template`, `is_variable_template`, `is_class_template`, `is_alias_template`, `is_conversion_function_template`, `is_operator_function_template`, `is_literal_operator_template`, `is_constructor_template`, `is_concept`, `operator_kind`, `can_substitute`, `substitute`

### `nonstatic_function`
Requirement: a non-static member function.
`has_explicit_object_parameter`, `is_const`, `is_volatile`, `is_lvalue_reference_qualified`, `is_rvalue_reference_qualified`, `is_conversion`, `is_special_member`, `is_copy_assignment`, `is_move_assignment`, `is_assignment`

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
`offset`

### `enumerator_entity`
Requirement: an enumerator.
(check only)

### `parameter_entity`
Requirement: a function parameter.
`has_default_argument`, `is_explicit_object_parameter`, `variable`

### Notes on `extract<T>()`

Returns `meta_result<T>` (thrown `std::meta::exception` becomes a failure). Member and function pointers come
straight from data members and functions (`d.extract<int S::*>()`, `f.extract<int (S::*)() const>()`); a reference
to a variable needs the object first (`v.object().unwrap().extract<int &>()`); reading the value of a variable that
is not usable in constant expressions is a hard compile error, not a failure.

## Free functions

| function | result |
|---|---|
| `flatten(items...)` | `array_literal<handle>`: handles and (nested) ranges of handles, in order |
| `reflect_constant(v)` | `handle` (a value for scalars, a template parameter object for class types) |
| `reflect_object(o)` | `object` (o needs static storage duration) |
| `reflect_function(f)` | `handle` |
| `reflect_constant_string(r)`, `reflect_constant_array(r)` | `handle` |
| `current_function(ctx = current())` | `meta_result<handle>` |
| `current_class(ctx = current())` | `meta_result<type>` |
| `current_namespace(ctx = current())` | `namespace_` |
| `data_member_spec(type, options = {})` | `meta_result<data_member_description>` |
| `define_aggregate(type, descriptions)` | `meta_result<type>` (call from a `consteval {}` block) |
| `global_namespace` | `namespace_{^^::}` (a constant) |

## Cached queries: `metalib::cache` (cache.hpp)

| variable template | value |
|---|---|
| `members<Scope, Kind = void, A = access::unprivileged>` | `Scope.members<Kind>(ctx)` (`Kind = void`: the scope's default kind) |
| `bases<Class, A>`, `subobjects<Class, A>` | `Class.bases(ctx)`, `Class.subobjects(ctx)` |
| `enumerators<Enum>` | `Enum.enumerators()` |
| `parameters<Function>` | `Function.parameters()` |
| `template_arguments<Specialization>` | `Specialization.template_arguments()` (a `meta_result`) |

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

## Literals (literal.hpp)

- `array_literal<T>`: structural view over a `define_static_array` array; usable as a template argument and in
  `template for`; converts to `std::span<T const>`.
- `string_literal` / `u8string_literal`: structural null-terminated strings over `define_static_string`;
  convert to `std::string_view` / `std::u8string_view`, compare with string views.
