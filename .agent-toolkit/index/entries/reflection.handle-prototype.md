+++
title = "Class wrappers around std::meta::info: what GCC 16 and clang-p2996 accept"
+++

Measured 2026-11-24 with a prototype in the scratchpad, not in the repo. The prototype:
`struct handle { std::meta::info reflection{}; handle() = default; consteval handle(info); consteval operator info() const; friend bool operator==(handle, handle) = delete; friend bool operator==(handle, info) = delete; }`
plus `struct type_handle : handle` with an explicit consteval ctor that throws when `!is_type`.

Results: GCC = local MinGW GCC 16.1. Fork = Compiler Explorer `clang_bb_p2996`.
- Usable as an NTTP (`template <handle H>`, also `tag<^^shape>`): GCC ok, fork ok. This needs a structural type, so the
  `info` member must be public.
- `std::meta::identifier_of(h)` / `is_enum_type(h)` through the implicit conversion: ok on both.
- `==` blocked for handle/handle, handle/info, info/handle, derived/derived and derived/base, checked with a concept
  `requires (A a, B b) { a == b; }`. `h.reflection == ^^x` still works. Template-argument equivalence does not use `==`.
- `typename [: h :]` (splice through the implicit conversion): GCC ok. Fork **error**: "expressions of consteval-only
  type are only allowed in constant-evaluated contexts". `[: h.reflection :]` and `[: std::meta::info(h) :]` are ok on both.
  An expression splice `[: e :]` with a constexpr handle is ok on both.
- `template for (constexpr handle e : std::define_static_array(vector<handle>))`: GCC ok (constexpr fn template).
  Fork **error**: "pointer into an object of consteval-only type is not a constant expression unless it also has
  consteval-only type". `std::define_static_array(vector<handle>).size()` itself is ok on both.
- `template for (constexpr handle e : [: std::meta::reflect_constant_array(vector<handle>) :])`: ok on both.
- `std::meta::substitute(^^std::vector, std::vector<handle>{...})`: **no matching function** on both. The
  reflection_range concept needs a range of exactly info.
- A failed checked ctor (`throw "metalib::type_handle: not a type"`): GCC prints
  `uncaught exception '(const char*)"metalib::type_handle: not a type"'`. Fork prints only "must be initialized by a
  constant expression / in call to 'type_handle(^^(declaration))'", without the message.
- Literal/structural types cannot have virtual bases, so a diamond (one kind deriving from two kinds) would duplicate
  the handle base.

## Comparison (second prototype)
- `friend consteval bool operator==(handle, handle) = default;` compares the `info` member, i.e. reflection identity.
  Ok on both compilers. Derived/derived, derived/base and cross-kind (type_handle vs member_handle, gives false) all
  compile through it. `std::equality_comparable<handle>` holds, `std::ranges::contains` on vector<handle> works, and
  NTTP use is unaffected. `constexpr` instead of `consteval` on the defaulted one also works.
- `h == ^^int` / `^^int == h` with only the defaulted operator: GCC accepts. Fork: **ambiguous** (handle/handle vs the
  built-in info/info, each needs one user-defined conversion). The fix is
  `friend consteval bool operator==(handle a, std::meta::info b) { return a.reflection == b; }`, and the reversed order
  comes from C++20 rewriting. It **must be consteval**: a non-template `constexpr` function using info is rejected by
  both compilers ("consteval-only expressions are only allowed in a constant-evaluated context").
- Identity, not type equivalence: `handle{^^std::size_t} != handle{^^unsigned long long}` (alias). `dealias` first to
  compare types. size_t is unsigned long long on MinGW, unsigned long on Linux.
- info has no ordering, so no `<`/`<=>`.

## Member templates of a base class (verified on both compilers)
- An explicit specialization written inside the derived class body is an **error** (GCC: "explicit specialization in
  non-namespace scope"; fork: "no function template matches").
- `template <> consteval int base::f<derived>() const` at namespace scope works. It specializes the *base's* template
  for the argument `derived`, and it must come before the first use, otherwise "specialization after instantiation".
  Function templates have no partial specialization.
- A derived class can hide the base's template by declaring its own `f`. `using base::f;` plus a same-signature
  template in the derived class also hides it. A call through `base const&` still picks the base's version (static).
- Deducing this (`template <class Self> consteval int kind(this Self const&)`) dispatches on the static derived type.
  It works with a variable-template trait specialized per derived type, and with a hook detected via
  `requires { s.hook_impl(); }`. After a conversion to the base type it uses the base's behaviour.

## Forwarding a member template to a specializable class template (the user's handle.hpp approach)
`handle::into<T>()` forwards to `details::into_helper<Source>::into(source, into_tag<Target>)`. Class templates can be
partially specialized with a constraint (`template <std::derived_from<handle> T> struct into_helper<T> : into_helper<handle> {}`),
and a full specialization for one derived type beats that partial one. Verified on both compilers, with these requirements:
- Helper functions whose signature has `std::meta::info` must be `consteval`. With `static constexpr`, GCC says "function of
  consteval-only type must be declared 'consteval'", and the fork rejects it too. This already fails when the header is
  only included.
- Key the helper on the source type through deducing this (`template <class T, class Self> constexpr T into(this Self const& self)`).
  Otherwise Self is always `handle` and derived specializations never apply.
- A derived full specialization needs `: into_helper<handle>` plus `using into_helper<handle>::into;` to keep the base conversions.
- `requires { h.template into<X>(); }` is true even for unsupported X, because the body is not checked. A requires-clause on
  `into` makes it detectable.
Working version: C:/Users/YIRAN~1.WAN/AppData/Local/Temp/claude/fab643b92482637d/5c8622d6-d483-4c76-b6b6-d72b6b975e9c/scratchpad/hh_fixed.cpp

See [[reflection.expansion-statements]].
