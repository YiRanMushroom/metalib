+++
title = "Remaining std::meta wrappers (everything except templates)"
+++

Code: [[src/lib/include/metalib/handle.hpp]]. Probe: scratchpad misc_probe.cpp; check: misc_check.cpp. GCC trunk
2026-11-25.

Failure handling (meta_result only where the std call can fail):
- details::attempt<T>(f, describe) catches std::meta::exception and returns the meta_error that describe builds
  (on clang, METALIB_HAS_META_EXCEPTION=0, it just calls f). Used for common_type, common_reference, invoke_result,
  object_of, variable_of, data_member_spec, define_aggregate.
- tuple_size(int) / variant_size on a non-variant are hard compile errors in GCC, not throws: checked first with
  is_complete_type(substitute(^^std::tuple_size / ^^std::variant_size, {t})). <variant> is included for that.
- constant_of(non-constexpr variable) is a hard error: not offered on variables; annotation has constant_of().

Added:
- type_traits: reference_constructs/converts_from_temporary, try_get_tuple_size, try_get_variant_size
  (meta_result<size_t>; the get_ forms unwrap).
- type_transforms: get_tuple_element(i), get_variant_alternative(i), find_common_type_with(types),
  find_common_reference_with(types), find_invoke_result_with(args), each with a try_ form returning meta_result<type>.
- displayable: source_location() (line 0 for fundamental types).
- signature: is_literal_operator, try_get_operator_symbol() (meta_result<string_literal>) / get_operator_symbol(); nonstatic_function: is_assignment.
- parameter: try_get_variable() -> meta_result<handle> (variable_of) / get_variable(). class_record: subobjects(ctx) -> handles (bases, then
  nonstatic data members). storage_duration: try_get_object() -> meta_result<object> / get_object().
- Kinds: value (is_value), object (is_object), annotation (is_annotation; typed, value<T>() via extract,
  constant_of() -> handle), structured_binding (identifiable, displayable; type_of throws on GCC).
  annotations()/annotations_with_type() return std::vector<annotation>.
- Free functions: reflect_constant -> handle (value for scalars, object for class types), reflect_object -> object,
  reflect_function -> handle, reflect_constant_string / reflect_constant_array -> handle (GCC: the string result
  is not is_object), get_current_function / get_current_class (with try_ forms) / current_namespace (default ctx = caller; scope walk via
  parent_of; equal to the std versions where probed), make_data_member_spec(type, options) / define_aggregate(type,
  specs), each with a try_ form (usable from a consteval block).

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

extract (probe: scratchpad extract_cases/, 2026-11-25): constant::try_extract<T>() returns meta_result<T> (via attempt; extract<T>() unwraps) and
is on value, object, annotation, enumerator, data_member, static_data_member, member_function,
static_member_function, function, variable. GCC trunk: extract<int S::*>(data member), extract<R (S::*)(..) cv>(member
function), extract<R (*)(..)>(function) work; a wrong type throws; extract<int&>(^^variable) throws, extract<int&>(
object_of(variable)) works; extract<int>/extract<int*> of a non-constexpr variable is a hard error (not catchable).
meta_result<T&> works (libstdc++ has optional<T&>; meta_result forwards instead of moving).

Constants and objects (2026-11-26, decided with the user; verified on GCC trunk, scratchpad vararg_probe/co.cpp):
- reflect_constant(v) returns the kind constant for a non-class v (std: a value) and object for a class-type v (std:
  the template parameter object). That object is not v: reflect_constant(x) != reflect_object(x), its address differs
  from &x, and reflect_constant(x) == reflect_constant(y) when the values are equal. reflect_object(x) is x itself.
- constant (displayable, typed, constant, constant_entity: a value or an object) is also what an annotation's
  get_constant() returns, where the type is not known statically; is_value_constant() / is_object_constant() tell which.
- reflect_constant_string / reflect_constant_array return object. Working draft [meta.reflection] "Promoting to static
  storage": both return ^^P where P is a template parameter object ("a potentially non-unique object"). GCC trunk
  reports is_variable for the result (is_object and is_value false): a GCC deviation, not followed; the kinds are
  built unchecked, so only try_reflect<object>(that info) is affected.
- reflect_function returns handle: a free function is a function, a static member function is a class member.
- Typed kinds (2026-11-26, user decision: keep as much in the type as can be known): ability::typed_as<T> (requirement
  "of type T", cv ignored; try_extract() / extract() without a template argument). typed_constant<T>, typed_object<T>,
  typed_annotation<T> = the untyped kind's set except ability::constant, plus typed_as<T>; the generic extract<T>() is
  removed from them so there is one way to read the value, which also means they do not convert implicitly to the
  untyped kind (use into<constant>()). reflect_constant / reflect_object return them; annotatable has
  get_annotations_with_type<T>(). except<Set, A...> and K::without_ability<A...> remove abilities. Typed kinds must be
  spelled basic_interface<>::with_ability<set, ...>: K::with_ability would need K complete before the abilities are.
  Include-only compile memory 228M (223M before).
