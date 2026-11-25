# metalib

Strongly typed wrappers over C++26 static reflection (P2996, `<meta>`). A reflection is held in a small handle
whose **kind** says what it reflects (a class type, a data member, a namespace, a template, ...); each kind offers
exactly the queries that make sense for it, as member functions.

```cpp
#include <metalib/metalib.hpp>
namespace mb = metalib;

struct point { int x; int y; double weight; };

constexpr mb::class_type cls = mb::type{^^point}.try_into<mb::class_type>().unwrap();
constexpr auto ints = cls.members<mb::data_member>(std::meta::access_context::current(),
                                                   mb::predicate::of_type<int>());
static_assert(ints.size() == 2 && ints[1].ascii_identifier().unwrap() == "y");
static_assert(ints[0].type() == mb::type{^^int} && ints[0].offset().bytes == 0);
```

Header-only, one include: `<metalib/metalib.hpp>`. The complete list of kinds, abilities and functions is in
[docs/reference.md](docs/reference.md).

## Requirements

- A compiler implementing P2996 with the **consteval-only values** model (P4101, accepted as a defect report in June
  2026). Development and all tests use **GCC trunk (GCC 17)** with `-std=c++26 -freflection`.
- GCC 16.x and the bloomberg clang-p2996 fork implement the older consteval-only *type* model and reject parts of
  the library (results that mix reflections with runtime values). The CMake target still has a clang setup
  (`-freflection-latest -stdlib=libc++`) and `meta_result` has a path for compilers without
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

### Narrowing

- Checked: `h.try_into<K>()` / `K::from(h)` return `meta_result<K>`; `K::holds(info)` tests. The failure
  message names the reflection and the first unmet requirement, e.g. `reflection 'int' is not a class type`.
- Unchecked: the explicit constructor `K{info}`, for when the kind is certain.

### Equality

`==` compares handles of any two kinds. Kinds with the type ability compare dealiased (`type{^^alias} ==
type{^^target}`); others compare the reflections. Comparing with a raw `std::meta::info` is deleted.

### Failures: `meta_result<T>`

A query returns `meta_result<T>` exactly when the underlying `std::meta` call can fail (throw or be ill-formed);
calls that cannot fail return the value. `unwrap()` throws `std::meta::exception` during constant evaluation (a
compile error with the message) and `metalib::meta_exception` at runtime; also `has_value()`, `unwrap_or(v)`,
`error()` (`u8what()`, `from()`, `where()`). Thrown `std::meta::exception`s are converted with their message.

### Results that outlive constant evaluation

Ranges come back as `array_literal<K>` and strings as `string_literal` / `u8string_literal`: static storage,
structural, usable in `template for`, as template arguments and at runtime.

```cpp
template<class E>
constexpr std::string_view name_of(E v) {
    template for (constexpr mb::enumerator e : mb::enum_type{^^E}.enumerators()) {
        if (v == e.extract<E>().unwrap()) return e.ascii_identifier().unwrap();
    }
    return {};
}
```

### Queries: access context, filter, map

Member queries take `(ctx, filter, map)`, all defaulted: `ctx = std::meta::access_context::current()` (the
*caller's* scope), `filter` accepts everything, `map` is identity. The element kind is a template argument and
the order is exactly the `std::meta` order.

```cpp
using namespace mb::predicate;
cls.members<mb::member_function>(ctx, of_name("f") || of_operator(std::meta::op_plus));
cls.members<mb::data_member>(ctx, {}, mb::transform::from(&mb::data_member::type).and_then(...));
ns.members<mb::variable>();        // namespaces work the same way
```

### Cached queries (cache.hpp)

`metalib::cache::members<Scope, Kind = void, access = access::unprivileged>` is a variable template holding the
result of `Scope.members<Kind>(ctx)`; it is computed once per translation unit and every use refers to the same
array, which also cuts compile time when the same query is repeated (300 repeated queries: 4.0 s direct, 2.5 s
cached, GCC trunk). `Scope` is a kind value (handles are structural), e.g.
`mb::cache::members<mb::class_type{^^S}, mb::data_member>`. A variable template cannot see the caller's scope, so the
access is `access::unprivileged` (public members) or `access::unchecked` (everything). Also `cache::bases`,
`cache::subobjects`, `cache::enumerators`, `cache::parameters`, `cache::template_arguments`. Annotations are
not cached because their result depends on where they are queried.

### Messages (message.hpp)

`metalib::message(parts...)` builds a `std::string` from text, handles (their display string), integers, bools and
chars; it can be used directly as a C++26 `static_assert` message. `to_string(h)` / `to_u8string(h)` give one
handle's display string.

```cpp
static_assert(m.type() == mb::type{^^int}, mb::message("member ", m, " must have type int, has ", m.type()));
// error: static assertion failed: member S::x must have type int, has double
```

### Parents

Every declaration kind has `has_parent()` and `parent()` → `meta_result<handle>`; narrow it yourself:
`d.parent().unwrap().try_into<mb::class_type>()`. Types like `int` and the global namespace have no parent.

### Templates

Templates are deliberately type-erased: reflection can tell a template's kind, name, location, parent, access and
linkage, and substitute it, but not its parameter list or signature.
`template_::can_substitute(args)`, `template_::substitute(args)` → `meta_result<handle>`, then `try_into` the
expected kind (class template → `type`, function template → `function`, concept → bool `value`).
Specializations have `template_()` and `template_arguments()`.

### Names

`ascii_*` functions return `string_literal` and fail for non-ASCII text; `u8_*` functions return
`u8string_literal`. `has_ascii_identifier()` / `has_u8_identifier()` test.

## Caveats worth knowing

- Annotations depend on where you ask: `annotations()` sees the declarations reachable from the point of the
  outermost constant evaluation. A parameter's annotations live on `function.parameters()[i]`, never on its type.
- `^^S && x` parses as `^^(S&&)`; write `(^^S) && x`.
- GCC trunk issues seen: `template_arguments_of` on a member function template specialization named by a qualified
  name crashes the compiler (reach it through `members()`); an `extern "C"` function reports no parent.

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
- `.agent-toolkit/index/entries/`: design notes and investigation results kept with the project (`status.md` is
  the current state).
