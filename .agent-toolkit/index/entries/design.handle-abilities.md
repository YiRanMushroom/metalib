+++
title = "metalib type system: handle, abilities, interface<...>, meta_result"

[[links]]
path = "src/lib/include/metalib/handle.hpp"
snippet = "struct basic_interface : handle, Abilities..."

[[links]]
path = "src/lib/include/metalib/meta_result.hpp"
snippet = "class meta_result"

[[links]]
path = "src/lib/include/metalib/encoding.hpp"
snippet = "consteval bool ordinary_literal_encoding_is_utf8()"
+++

Decisions made with the user (2026-11-24):

- **Typed handles only.** The goal is that raw `std::meta::info` does not flow through the API. Parameters are typed
  handles (`type const &`, `std::vector<type>`), never `info`. The only entry point that takes `info` is the explicit
  constructor. `handle` keeps a consteval implicit conversion *to* info, so handles work with std::meta directly.
  Mixed `type == info` is deleted, because the implicit conversion would otherwise fall back to the built-in info
  comparison.
- **Abilities + erasure.** `ability::*` are empty mixins. Their members use deducing this (`this auto const &self`) and
  read `self.info`; there is no CRTP parameter. `basic_interface<A...> : handle, A...` holds the single info.
  `interface<A...>` canonicalizes the ability list with `std::meta::type_order` (P2830), falling back to
  display_string_of when `__cpp_lib_type_order` is missing, and removes duplicates. Conversion is implicit to a subset
  of abilities only; there is no conversion to a superset and no `into`. Kinds are deliberately erased: two kinds
  with the same abilities are the same type.
- **Kinds are not distinguished beyond abilities.** A type alias is just a `type`; `==`/`is` dealias both sides.
  A future alias-to-class gets the class abilities. Prefer ability composition over C++ inheritance (no diamonds;
  literal types cannot have virtual bases).
- **Signatures that mention `type`** are declared in the ability and defined out of line after `struct type`.
  Otherwise the incomplete `type` is an error even inside deducing-this templates.
- **meta_result<T>.** It holds `optional<T>` + `optional<metalib::meta_exception>` and offers unwrap / unwrap_or /
  has_value / error / failure. Failure is decided with the standard's has_*/is_* predicates, never try/catch. Only
  queries with a precondition return it. Those without one (display_string_of, the type traits) return the plain value.
- **metalib::meta_exception** mirrors std::meta::exception (ctors from u8string_view and string_view, plus what,
  u8what, from, where). Text lives in static storage via define_static_string, so a failed result can sit in a
  constexpr variable; GCC's std::meta::exception uses the heap and cannot. unwrap() throws std::meta::exception where it
  exists (for GCC's clean diagnostic) and the own type otherwise (clang-p2996 has no std::meta::exception, and the
  throw is then just a non-constant error). Test the clang path on GCC with `-DMETALIB_HAS_META_EXCEPTION=0`.
- **Identifiers / encoding: only u8 and ASCII, the word "ordinary" is not used.** `has_u8identifier`/`u8identifier` follow std
  has_identifier. `has_identifier`/`identifier` additionally require the identifier to be ASCII. encoding.hpp has only
  is_ascii, u8_to_ascii, ascii_to_u8 and static_u8; there is no literal-encoding detection.
- **Concrete kinds are hand-written, not interface<...>:** `struct type : handle, ability::...`, and
  `arithmetic_type : type, ability::arithmetic` / `enum_type : type, ability::enumeration` (class_type later). A derived kind
  only adds abilities its base lacks; repeating one would make an ambiguous duplicate base. `interface<...>` stays as the
  erased view. Its converting ctor accepts any `Source` derived from handle and from every target ability, so hand-written
  kinds convert to any subset view too.
- **Narrowing is a query:** `handle::try_into<T>()` calls `T::from(handle const &)` and returns `meta_result<T>`; there is no
  implicit superset conversion. Kind split follows big ability differences only: arithmetic (integral, floating, bool,
  char), enum, class/union. Pointers, references, arrays, function types, member pointers, void, nullptr_t and info
  are plain `type`. try_into keeps the original info (an alias stays an alias: identifier() gives the alias name).
- **Ability names so far:** identifiable, displayable, sized, type_traits, type_transforms, templated (the user rejected
  "specialization"), annotatable.
- **GCC 16 quirks met:** `size_of(^^S&)` is not sizeof(S&); the wrapper passes it through unchanged (thin translation). libstdc++'s
  std::strong_ordering is not structural, so extract<strong_ordering> fails (moot now that std::meta::type_order is
  used). `^^int && x` parses as the type `int&&`; write `(^^int) && x`.
- **Thin translation (user rule):** wrappers call the std::meta function directly. No extra validation helpers
  (no sizeof/sign/annotation/enumerable checks), no special-casing such as remove_reference before size_of: assume an
  experienced user who checks completeness etc. first. Rule for return types: if the std::meta call can fail (throw, i.e. a compile error), return meta_result;
  if it silently passes the input through, return the plain value. Measured on GCC 16 (scratchpad/trans_matrix.cpp):
  every cv/ref/extent/pointer/cvref/decay/unwrap transform is total (e.g. remove_reference(int) == int,
  add_lvalue_reference(void) == void). make_signed/make_unsigned throw for anything but non-bool integral types and
  enums; underlying_type throws for non-enums. So arithmetic_type::make_signed/make_unsigned return meta_result
  (bool and floating point fail). enum_type's three return plain type (they fail only for an incomplete enum, i.e.
  inside its own enumerator list). meta_result is also used for identifier/u8identifier,
  template_of/template_arguments and the kind check in `T::from`.
- Identifier predicates are named `has_ascii_identifier()` (identifier exists and is ASCII) and `has_u8_identifier()`
  (any identifier).
- **Runtime use (template for over handles inside a constexpr function called at runtime):** ability members take
  self **by value** (`this auto self` / `this Self self`). A reference to a constexpr *local* such as the `template for`
  variable is not a constant expression, so it escalates the whole enclosing function to consteval. `meta_result::unwrap()`
  returns T by value. Any meta_result temporary in a runtime full-expression escalates (GCC 16 and clang-p2996), e.g.
  `return e.ascii_identifier().unwrap();` or `e.ascii_identifier().has_value()`: meta_result is consteval-only and
  [basic.types.general]/12 requires every consteval-only object to live inside a core constant expression evaluation,
  while the temporary lives to the end of the full-expression. A by-value `this` on unwrap does not help; `constexpr std::string_view name = e.ascii_identifier().unwrap();` then `return name;` works.
  Verified by running scratchpad/enumerator_check.cpp (a name_of over enum_type::enumerators()).
- ascii_identifier/ascii_display_string check ASCII on the u8 form, then return identifier_of/display_string_of directly (no copy).
- **Names (2026-11-24):** ascii_identifier/u8_identifier, has_ascii_identifier/has_u8_identifier,
  ascii_display_string (a meta_result, fails if not ASCII)/u8_display_string. `enumerator : handle, identifiable,
  displayable, annotatable, typed, constant`. `ability::typed::type_of()` returns type; `ability::constant::value<T>()` is
  extract<T>. `enum_type::enumerators()` returns span<enumerator const>.
- **User preferences:** do not test everything with code; reason it out and keep moving. Comments say what the code
  does, not why.

## meta_result without info (2026-11-24)
- meta_exception holds only u8what/what (static strings) and source_location, derives std::exception, and never
  throws std::meta::exception (METALIB_HAS_META_EXCEPTION removed; the user finds the info in errors useless).
  failure(u8 message, source_location = current()). Its constructors are consteval (define_static_string).
- meta_result members are constexpr. meta_result<T> is consteval-only exactly when T is, so meta_result<string_view>
  can reach runtime: has_value/unwrap/unwrap_or work at runtime and unwrap throws metalib::meta_exception
  (scratchpad/mix_rt.cpp runs: "caught: reflection has no identifier").
- Escalation findings (scratchpad tc_*, tc2_*):
  - GCC 16: in a non-template constexpr function `return metalib::type{^^color}.ascii_identifier().unwrap();` works.
    Inside a function template, calling an ability member that returns string_view on a handle escalates even
    without meta_result (`enum_type{^^E}.u8_display_string().size()`), while `std::meta::identifier_of(handle)` is
    fine. clang-p2996 accepts it: a GCC bug.
  - clang-p2996: name_of with `if (v == [:e.info:]) return e.ascii_identifier().unwrap();` works; `e.value<E>()`
    (constant::value<T>, explicit template argument + deducing this) escalates there, but GCC accepts it.
- Minimal repro without metalib (scratchpad dt_*.cpp, dt2_*.cpp; struct H { info; consteval members }): not caused by
  deducing this, since plain members, template members and int returns behave the same. GCC 16: `H{^^color}.plain()` in a
  non-template constexpr function compiles; in a function template it escalates (also with `constexpr H h{^^E}; h.plain()`);
  `free_tmpl(H{^^E})` is fine. That inconsistency is a GCC bug. clang-p2996: non-template is a hard error "expressions
  of consteval-only type are only allowed in constant-evaluated contexts" (pointing at `H{^^color}`); a template
  escalates, also for the free function. Follow-up (scratchpad lt_*.cpp, template cases): clang matches [basic.types.general]/12.3. A reference
  binding to a materialized consteval-only temporary (`C{^^E}.plain()` with a consteval ctor, `H{^^E}.plain()`) escalates,
  because the temporary lives to the end of the full-expression. A constexpr local by reference (`h.plain()`) and
  by-value `this C self` initialized by a consteval ctor are fine; aggregate `H{^^E}` by value still escalates. GCC
  escalates every one of these, including the constexpr local with no temporary, so that is a GCC bug. metalib's
  pattern (consteval ctors and `this auto self` by value) is the clang-OK shape. Portable pattern for runtime-callable code: put the
  results in constexpr variables (`constexpr auto n = h.member();`).
- **P4101R1 "Consteval-only Values for C++26"** (accepted as a DR, June 2026) replaces consteval-only *types* with
  consteval-only *values*; the null reflection is not consteval-only. GCC trunk implements it (PR125820, r17-4197,
  Sept 2026); GCC 16 and the analysis above use the old type model. On GCC trunk (Compiler Explorer gsnapshot) every
  lt_* case passes, including non-template and aggregate ones, and the full metalib name_of with
  `return e.ascii_identifier().unwrap();` in a template gives: OK. No GCC report matches our exact case; related:
  PR125797 (spurious escalation via templates), PR128386 (ubsan regression after P4101).
- Narrowing that cannot fail uses the kind's explicit constructor directly (it does no check); `try_into` / `T::from` are for checked narrowing.
