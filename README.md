# metalib

Strongly typed wrappers over C++26 static reflection (P2996, `<meta>`). A reflection is held in a small handle
whose **kind** says what it reflects (a class type, a data member, a namespace, a template, ...); each kind offers
exactly the queries that make sense for it, as member functions.

```cpp
#include <metalib/metalib.hpp>
namespace mb = metalib;

struct point { int x; int y; double weight; };

constexpr mb::class_type cls = mb::reflect<^^point>;
constexpr mb::array_literal ints = cls.get_members<mb::data_member>(std::meta::access_context::current(),
                                                                mb::predicate::of_type<int>());
static_assert(ints.size() == 2 && ints[1].get_identifier() == "y");
static_assert(ints[0].get_type() == mb::reflect<^^int> && ints[0].get_offset().bytes == 0);
```

Header-only, one include: `<metalib/metalib.hpp>`. The complete list of kinds, abilities and functions is in
[docs/reference.md](docs/reference.md).

## Requirements

- A compiler implementing P2996 with the **consteval-only values** model (P4101, accepted as a defect report in June
  2026). Development and all tests use **GCC trunk (GCC 17)** with `-std=c++26 -freflection`.
- GCC 16.x and the bloomberg clang-p2996 fork implement the older consteval-only *type* model and reject parts of
  the library (results that mix reflections with runtime values). The CMake target still has a clang setup
  (`-freflection-latest -stdlib=libc++`) and metalib has a path for compilers without
  `std::meta::exception` (`METALIB_HAS_META_EXCEPTION=0`), but only GCC trunk is tested.
- CMake 4.3+.

## Using it

```cmake
add_subdirectory(metalib)            # or FetchContent
target_link_libraries(app PRIVATE metalib::metalib)   # adds -freflection for GCC
```

## Concepts

### Handles and kinds

`metalib::handle` holds one `std::meta::info`. A kind is a `handle` plus a set of **abilities**; each ability is an
empty struct contributing member functions. Kinds are aliases:

```cpp
using class_type = basic_interface<>::with_ability<details::ability_sets::class_type>;
// ability_sets::class_type = ability_set<type, ability::record, ability::class_record>
```

- `ability_set<A...>` flattens nested sets, sorts by `std::meta::type_order` and removes duplicates, so the same
  abilities always give the same type, whatever the order or nesting.
- `X::abilities` is a kind's ability set; `X::with_ability<A...>` adds abilities (or other kinds' `::abilities`):
  `mb::class_type::with_ability<mb::ability::non_alias>`, `mb::type::with_ability<mb::member::abilities>`.
- A kind converts implicitly to any kind with a subset of its abilities (`class_type` → `type`).
- Ability functions take `this handle const &self`, so `&mb::member_function::is_const` is an ordinary function
  pointer usable as a predicate.

### Reflecting: `mb::reflect<^^x>`

`mb::reflect<R>` (a variable template, in `<metalib/reflect.hpp>`) takes the reflection as a template argument and
gives a handle with **every ability that holds for it**, so the kind does not have to be named:

```cpp
static_assert(mb::reflect<^^point::x>.get_offset().bytes == 0);   // has the abilities of a data member
constexpr mb::class_type cls = mb::reflect<^^point>;              // converts to any kind it satisfies
// constexpr mb::variable v = mb::reflect<^^point::x>;            // a type error: not a variable
```

- The result converts implicitly to the named kinds the reflection satisfies; its exact type is an implementation
  detail (it changes when abilities are added), so name a kind where a type is needed.
- It works wherever the reflection is a constant: a `^^` expression, a `constexpr std::meta::info`, the element of
  a `template for`. A reflection held in a function parameter or an ordinary variable is not a constant: use
  `mb::make_reflection<K>(r)` there.
- A value, an object or an annotation gives the typed kind (`typed_constant<T>`, `typed_object<T>`,
  `typed_annotation<T>`), whose `extract()` needs no template argument. Like every typed kind it does not convert
  to the untyped `constant`, `value`, `object` or `annotation`.
- Which abilities are tried: every class in `metalib::ability` marked `[[=auto_ability]]`, each asked through its
  `static consteval bool check(std::meta::info)`. An ability of your own takes part when it is declared in that
  namespace with the mark, before `mb::reflect` is first used in the translation unit (and the same in every
  translation unit).

### Narrowing

- Checked: `mb::try_make_reflection<K>(r)` (from a `std::meta::info`), `h.try_into<K>()` and `K::try_from(h)` return
  `meta_result<K>`; `K::holds(info)` tests. The failure message names the reflection and the first unmet requirement,
  e.g. `reflection 'int' is not a class type`, or `reflection '...' cannot have annotations` when only a capability
  is missing.
- `mb::make_reflection<K>(r)`, `h.into<K>()` and `K::from(h)` return `K`; each is its `try_` form followed by
  `.unwrap()` (`h.into<K>()` is `h.try_into<K>().unwrap()`), so a failure is a compile error showing the message.
- A `meta_result` holding a handle has `try_into<K>()` (keeps the original error) and `into<K>()` too:
  `t.try_substitute({mb::reflect<^^int>}).try_into<mb::class_type>()`, `d.try_get_parent().into<mb::namespace_>()`. The
  plain forms return a `handle`, narrowed the same way: `t.substitute({mb::reflect<^^int>}).into<mb::class_type>()`,
  `d.get_parent().into<mb::namespace_>()`.
- `k.try_attach_ability<A...>()` returns `meta_result<K::with_ability<A...>>` and `k.attach_ability<A...>()` the kind
  itself: the same handle with more abilities, checked against their requirements.
- A kind has no public constructor from a reflection: `reflect<R>` and `make_reflection<K>(r)` are the ways in.
  Unchecked construction is internal to metalib (`details::reflect_unchecked`, the friend of a private constructor).
  `K{}` holds a null reflection.

### Equality

`==` compares handles of any two kinds. If either kind has the type ability, the comparison is dealiased
(`mb::reflect<^^alias> == mb::reflect<^^target>`); otherwise it compares the reflections. Comparing with a raw
`std::meta::info` is deleted. The untyped `handle` has no `==`: narrow it first
(`d.get_parent().into<mb::class_type>() == cs`) or compare `.info`.

### Failures: `meta_result<T>` and `meta_error`

An operation that can fail comes as a pair: `try_<verb>()` returns `meta_result<T>`, and the plain name returns `T`: it
is `try_<verb>().unwrap()`, so a failure is a compile error showing the message. Names are verbs: `try_extract<T>()` /
`extract<T>()`, and `get_`, `find_` or `make_` before a noun (`try_get_parent()` / `get_parent()`,
`try_get_size()` / `get_size()`, `try_find_common_type_with()` / `find_common_type_with()`,
`try_make_data_member_spec()` / `make_data_member_spec()`): `get_` for something that already exists, `find_` for
something computed, `make_` for something constructed. An operation that cannot fail has only the plain form:
`get_members()`, `get_u8display_string()`, `get_current_namespace()`.

A failed result holds a `metalib::meta_error`, a plain value the caller can inspect: `error().message()` /
`u8message()`, `u8action()` (the failed `std::meta` call, e.g. `extract<long int*>()`; empty otherwise), `from()` (the
reflection), `where()`. `error().throw_()` throws it as `metalib::meta_exception` (whose `error()` gives the
`meta_error` back); `unwrap()` does that on failure, which during constant evaluation is a compile error showing the
message. Also `has_value()` and `unwrap_or(v)`. A failed result holds a reflection, so it is a consteval-only value and
cannot reach runtime; successful results of runtime-usable types can.

metalib writes every message itself and never passes on a `std::meta::exception` (it only catches them). Every
message names the reflection by its display string:

- a kind check: `reflection 'int' is not a class type`;
- a precondition a query checks itself: `reflection 'int' has no parent`, `type 'int' is not tuple-like`;
- a failed `std::meta` call: `unable to <verb> <call> <preposition> '<reflection>'`, the call with its operands (as in
  the metalib call), then the reflection and, when it has one, its type:

```text
unable to extract<long int*>() from 'seven' of type 'const int'
unable to substitute(long int) into 'template<class T>  requires  sizeof (T) == 1 struct small_box'
unable to find common_type_with(A) from 'int'
unable to make data_member_spec() from 'void'
error: uncaught exception of type 'metalib::meta_exception'; 'what()': 'unable to extract<long int*>() from 'seven' of type 'const int''
```

GCC's display string of a template is its template head (parameters and requires-clause, without default arguments), so
a substitution failure shows what the template expects. GCC substitutes silently, so why a substitution fails
(unsatisfied constraint, wrong argument count) is not available as text; to see the compiler's reason, write the
template-id itself, e.g. `typename [: t.info :]<long>`, where it is a constant.

### Results: vectors, materialized when needed

Queries that return several elements return `std::vector<K>`: nothing is promoted to static storage, which keeps
constant evaluation cheap. Use the result directly inside a consteval function or a constant expression
(`cls.get_members<mb::data_member>().size()`, `[0]`, range-for). A `std::vector` cannot outlive constant evaluation, so
materialize it once when it must: `array_literal` (and `string_literal` / `u8string_literal` for strings) holds the
data in static storage, is structural, and works in `template for`, as a template argument and at runtime.
`array_literal` deduces its element type from the range.

```cpp
constexpr mb::array_literal fields = cls.get_members<mb::data_member>();   // constexpr variable: materialize

template<class E>
constexpr std::string_view name_of(E v) {
    template for (constexpr mb::enumerator e : mb::array_literal(mb::reflect<^^E>.get_enumerators())) {
        if (v == e.extract<E>()) return e.get_identifier();
    }
    return {};
}
```

`metalib::cache` variable templates (below) hold materialized results.

### Queries: access context, filter, map

Member queries take `(ctx, filter, map)`, all defaulted: `ctx = std::meta::access_context::current()` (the
*caller's* scope), `filter` accepts everything, `map` is identity. The element kind is a template argument and
the order is exactly the `std::meta` order.

```cpp
using namespace mb::predicate;
cls.get_members<mb::member_function>(ctx, of_name("f") || of_operator(std::meta::op_plus));
cls.get_members<mb::data_member>(ctx, {}, mb::transform::from(&mb::data_member::get_type).and_then(...));
ns.get_members<mb::variable>();        // namespaces work the same way
```

### Cached queries (cache.hpp)

`metalib::cache::members<Scope, Kind = void, access = access::unprivileged>` is a variable template holding the result
of `Scope.get_members<Kind>(ctx)`; it is computed once per translation unit and every use refers to the same array,
which also saves compile time when the same query is repeated. `Scope` is a kind value (handles are structural), e.g.
`mb::cache::members<mb::reflect<^^S>, mb::data_member>`. A variable template cannot see the caller's scope, so the
access is `access::unprivileged` (public members) or `access::unchecked` (everything). Also `cache::bases`,
`cache::subobjects`, `cache::enumerators`, `cache::parameters`, `cache::template_arguments`. Annotations are not cached
because their result depends on where they are queried.

### Messages (message.hpp)

`metalib::message(parts...)` builds a `std::string` from text, handles (their display string), integers, bools and
chars; it can be used directly as a C++26 `static_assert` message. `to_string(h)` / `to_u8string(h)` give one
handle's display string.

```cpp
static_assert(m.get_type() == mb::reflect<^^int>, mb::message("member ", m, " must have type int, has ", m.get_type()));
// error: static assertion failed: member S::x must have type int, has double
```

### Parents

Every declaration kind has `has_parent()`, `try_get_parent()` → `meta_result<handle>` and `get_parent()` → `handle`;
narrow it yourself: `d.try_get_parent().try_into<mb::class_type>()` or `d.get_parent().into<mb::class_type>()`. Types
like `int` and the global namespace have no parent.

### Templates

Templates are deliberately type-erased: reflection can tell a template's kind, name, location, parent, access and
linkage, and substitute it, but not its parameter list or signature.
`template_::can_substitute(args)`, `template_::try_substitute(args)` → `meta_result<handle>` and
`template_::substitute(args)` → `handle`, then `try_into` or `into` the expected kind (class template → `type`, function
template → `function`, concept → bool `value`).
Specializations have `try_get_template()` / `get_template()` and `try_get_template_arguments()` /
`get_template_arguments()`.

### Names

`try_get_identifier()`, `try_get_display_string()` and `try_get_operator_symbol()` return
`meta_result<string_literal>` and fail for non-ASCII text; `get_identifier()`, `get_display_string()` and
`get_operator_symbol()` return the `string_literal`. `try_get_u8identifier()` and `try_get_u8operator_symbol()` return
`meta_result<u8string_literal>`; `get_u8identifier()` and `get_u8operator_symbol()` return the `u8string_literal`.
`get_u8display_string()` cannot fail and returns `u8string_literal`. `has_identifier()` /
`has_u8identifier()` test.

## Caveats worth knowing

- Annotations depend on where you ask: `get_annotations()` sees the declarations reachable from the point of the
  outermost constant evaluation. A parameter's annotations live on `function.get_parameters()[i]`, never on its type.
- `^^S && x` parses as `^^(S&&)`; write `(^^S) && x`.
- GCC trunk (gcc-17-20261122) crashes (internal compiler error) on `template_arguments_of` of a member function template
  specialization when the template was named by a qualified name: `substitute(^^S::mf, {^^int})`, then
  `get_template_arguments()`. GCC represents the qualified member name as a `BASELINK` node and `substitute` keeps it;
  `has_template_arguments` / `template_of` strip it, `template_arguments_of` does not, so an internal assertion fails.
  Reach member templates through `get_members<mb::member_template>()` / `of_name` instead, or apply the one-line fix in
  [patches/gcc-reflect-template-arguments-of-baselink.patch](patches/gcc-reflect-template-arguments-of-baselink.patch).
- GCC trunk: an `extern "C"` function reports no parent.

## Building and testing

```sh
cmake -S . -B build -G Ninja -DCMAKE_CXX_COMPILER=/path/to/gcc-trunk/bin/g++
cmake --build build
ctest --test-dir build
```

Tests (Catch2, fetched by CMake) live in `src/test`, one executable per file. Almost all checks are
`static_assert`s, so a failing check is a compile error.

## Layout

- `src/lib/include/metalib/`: `handle.hpp` (kinds and abilities), `meta_result.hpp`, `literal.hpp`,
  `query.hpp`, `cache.hpp`, `message.hpp`, `encoding.hpp`, `metalib.hpp`.
- `src/test/`: tests.
- `docs/reference.md`: kinds, abilities and functions.
- `patches/`: fixes for compiler bugs found while developing (GCC trunk).
- `.agent-toolkit/index/entries/`: design notes and investigation results kept with the project (`status.md` is
  the current state).
