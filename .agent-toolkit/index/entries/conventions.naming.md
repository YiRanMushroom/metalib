+++
title = "Naming conventions: data members, functions that can fail"

[[links]]
path = "src/lib/include/metalib/handle.hpp"
snippet = "try_get_parent"
+++

- Private data members are named `m_member` (e.g. `m_value`, `m_error`), never `member_`.
- Public data members keep plain names (e.g. `handle::info`).
- Applied in [[src/lib/include/metalib/meta_result.hpp]] (meta_exception, meta_result).

## Function names (decided with the user, 2026-11-26)
- Every accessor on a handle is a verb phrase: get_ for something that already exists (get_parent, get_type,
  get_members, get_destructor, get_current_namespace), find_ for something computed (find_common_type_with,
  find_invoke_result_with), make_ for something constructed (make_data_member_spec). Verbs keep their name (extract,
  substitute, remove_cvref, make_signed, define_aggregate, make_reflection, K::from). Predicates stay is_ / has_ / can_.
- An operation that can fail is a pair: `try_<verb>(...)` returns `meta_result<T>`, the plain name returns `T` and is
  `try_<verb>(...).unwrap()`. One that cannot fail has only the plain form. The prefix does not depend on fallibility,
  so making an accessor fallible later only adds its try_ form.
- Fallible by probe on GCC trunk (scratchpad fallible_probe): size_of / alignment_of (incomplete type, bit-field),
  type_of / return_type_of (undeduced auto), enumerators_of (opaque enum), offset_of (virtual base of an abstract
  class). Those are pairs; they fail with "unable to get size() from '<display>'".
- Value types (meta_error, meta_result, array_literal, string literals) and cache:: variables keep noun names.
- The plain form of a pair is a non-template `this handle const &self` function that attaches its own ability and calls
  the try_ form: `details::attach_ability_unchecked<typed>(self).try_get_type().unwrap()` (user decision 2026-11-26; a
  deducing-this template looked odd and could not have its address taken). Public counterparts on every kind:
  try_attach_ability<A...>() -> meta_result<with_ability<A...>>, attach_ability<A...>() unwraps.
- Text names follow std: no "ascii" and no underscore after u8 (get_identifier / get_u8identifier, get_display_string /
  get_u8display_string, has_identifier / has_u8identifier, get_operator_symbol / get_u8operator_symbol, message /
  u8message). The plain form requires ASCII and fails otherwise. meta_error::from() keeps its name.
- Kinds have no public constructor from std::meta::info; users write mb::reflect<^^x> (kind deduced,
  [[design.kind-deduction]]) or make_reflection<K>(r) / try_make_reflection<K>(r) for a reflection that is not a
  constant; the library calls details::reflect_unchecked<K>(r). Tests do the same as a user would.
