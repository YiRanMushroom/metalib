+++
title = "Remaining std::meta wrappers (everything except templates)"
+++

Code: [[src/lib/include/metalib/handle.hpp]]. Probe: scratchpad misc_probe.cpp; check: misc_check.cpp. GCC trunk
2026-11-25.

Failure handling (meta_result only where the std call can fail):
- details::attempt<T>(f) turns a thrown std::meta::exception into meta_result<T>::failure(u8what, from, where)
  (on clang, METALIB_HAS_META_EXCEPTION=0, it just calls f). Used for common_type, common_reference, invoke_result,
  object_of, variable_of, data_member_spec, define_aggregate.
- tuple_size(int) / variant_size on a non-variant are hard compile errors in GCC, not throws: checked first with
  is_complete_type(substitute(^^std::tuple_size / ^^std::variant_size, {t})). <variant> is included for that.
- constant_of(non-constexpr variable) is a hard error: not offered on variables; annotation has constant_of().

Added:
- type_traits: reference_constructs/converts_from_temporary, tuple_size, variant_size (meta_result<size_t>).
- type_transforms: tuple_element(i), variant_alternative(i), common_type_with(types), common_reference_with(types),
  invoke_result_with(args) (meta_result<type>).
- displayable: source_location() (line 0 for fundamental types).
- signature: is_literal_operator, operator_symbol() (meta_result<string_literal>); nonstatic_function: is_assignment.
- parameter: variable() -> meta_result<handle> (variable_of). class_record: subobjects(ctx) -> handles (bases, then
  nonstatic data members). storage_duration: object() -> meta_result<object>.
- Kinds: value (is_value), object (is_object), annotation (is_annotation; typed, value<T>() via extract,
  constant_of() -> handle), structured_binding (identifiable, displayable; type_of throws on GCC).
  annotations()/annotations_with_type() return array_literal<annotation>.
- Free functions: reflect_constant -> handle (value for scalars, object for class types), reflect_object -> object,
  reflect_function -> handle, reflect_constant_string / reflect_constant_array -> handle (GCC: the string result
  is not is_object), current_function / current_class / current_namespace (default ctx = caller; scope walk via
  parent_of; equal to the std versions where probed), data_member_spec(type, options) / define_aggregate(type,
  specs) (usable from a consteval block).

Meaning (for reference):
- subobjects_of(class) = bases then nonstatic data members, in that order (construction/comparison order); kept as a
  thin translation, elements are handles because they mix base and data_member.
- object_of(variable) = the object the variable designates (for a reference variable: the referenced object);
  lets two references be compared for identity.
- reflect_constant(v) copies a constant value (like a non-type template argument: a value for scalars, a template
  parameter object for class types); reflect_object(o) reflects an existing object with static storage by identity
  (like a reference template argument), so reflect_object(gv) works while reflect_constant(gv) does not.

Structured bindings (probe: scratchpad sb_probe.cpp): a binding is a name for a part of a hidden unnamed variable,
not a variable (is_variable 0). Queryable: identifier, display string, source_location, parent (declaring scope),
splice ([:^^x:] yields the element; for auto& bindings it is the original object). Not available on GCC trunk:
type_of (throws), storage duration / linkage / object_of (belong to the hidden variable), annotations (rejected),
which member or index it binds, the hidden variable; members_of does not list bindings. The kind
structured_binding (identifiable, displayable, parented) covers all of that. type_of throwing is conforming: in the
working draft [meta.reflection.queries] has-type lists value, annotation, object, variable, function, enumerator,
non-static data member, unnamed bit-field, direct base, data member description, function parameter - not
structured bindings - and type_of "Throws: meta::exception unless has-type(r)". So the kind gets no typed.
Little practical need to reflect bindings (like locals); support is mainly for completeness.

extract (probe: scratchpad extract_cases/, 2026-11-25): constant::extract<T>() returns meta_result<T> (via attempt) and
is on value, object, annotation, enumerator, data_member, static_data_member, member_function,
static_member_function, function, variable. GCC trunk: extract<int S::*>(data member), extract<R (S::*)(..) cv>(member
function), extract<R (*)(..)>(function) work; a wrong type throws; extract<int&>(^^variable) throws, extract<int&>(
object_of(variable)) works; extract<int>/extract<int*> of a non-constexpr variable is a hard error (not catchable).
meta_result<T&> works (libstdc++ has optional<T&>; meta_result forwards instead of moving).
