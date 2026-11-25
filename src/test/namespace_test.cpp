#include <catch2/catch_test_macros.hpp>
#include <metalib/metalib.hpp>
namespace mb = metalib;
namespace ab = mb::ability;
namespace N {
    namespace inner { int deep; }
    inline namespace v1 { int versioned; }
    namespace alias_ns = inner;
    struct C {};
    enum E { e1 };
    using al = C;
    int var;
    constexpr int cvar = 1;
    thread_local int tvar;
    void fn();
    void fn(int);
    static void sfn() {}
    namespace [[=1]] annotated {}
}
constexpr auto ctx = std::meta::access_context::unchecked();
constexpr mb::namespace_ n = mb::handle{^^N}.try_into<mb::namespace_>().unwrap();
static_assert(n.members(ctx).size() == 13);
static_assert(n.members<mb::namespace_>(ctx).size() == 3);
static_assert(n.members<mb::namespace_alias>(ctx).size() == 1 && n.members<mb::namespace_alias>(ctx)[0].target() == mb::namespace_{^^N::inner});
static_assert(n.members<mb::variable>(ctx).size() == 3 && n.members<mb::function>(ctx).size() == 3);
static_assert(n.members<mb::type>(ctx).size() == 3 && n.members<mb::class_type::with_ability<ab::non_alias>>(ctx).size() == 1);
// parents: members found through a namespace know it; nested namespaces chain to the global namespace
constexpr auto inner = n.members<mb::namespace_>(ctx)[0];
static_assert(inner.parent().unwrap().info == n.info);
static_assert(n.parent().unwrap().try_into<mb::namespace_>().unwrap() == mb::global_namespace);
static_assert(!mb::global_namespace.has_parent() && !mb::global_namespace.parent().has_value());
static_assert(inner.members<mb::variable>(ctx)[0].parent().unwrap().try_into<mb::namespace_>().unwrap().parent().unwrap().info == n.info);
static_assert(n.members<mb::variable>(ctx)[1].has_internal_linkage() && n.members<mb::variable>(ctx)[2].has_thread_storage_duration());
static_assert(n.members<mb::function>(ctx)[2].has_internal_linkage() && n.members<mb::function>(ctx)[1].parameters().size() == 1);
static_assert(mb::global_namespace.members<mb::namespace_>(ctx, mb::predicate::of_name("N")).size() == 1);
static_assert(mb::namespace_{^^N::annotated}.annotations().size() == 1);
// narrowing
static_assert(!mb::handle{^^N::alias_ns}.try_into<mb::namespace_>().has_value() && mb::handle{^^N::alias_ns}.try_into<mb::namespace_alias>().has_value());
static_assert(mb::handle{^^N::var}.try_into<mb::variable>().has_value() && !mb::handle{^^N::var}.try_into<mb::function>().has_value());
static_assert(!mb::handle{^^::}.try_into<mb::namespace_member>().has_value());
// parent chain across kinds: enumerator -> enum -> its enumerators -> parent again
constexpr auto en = mb::enum_type{^^N::E}.enumerators()[0];
static_assert(en.parent().unwrap().try_into<mb::enum_type>().unwrap().enumerators()[0].parent().unwrap().info == ^^N::E);
TEST_CASE("namespace: compile-time checks") {
    SUCCEED("all checks are static_asserts");
}
