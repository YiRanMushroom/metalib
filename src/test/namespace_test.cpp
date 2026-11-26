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
constexpr mb::namespace_ n = mb::reflect<^^N>;
static_assert(n.get_members(ctx).size() == 13);
static_assert(n.get_members<mb::namespace_>(ctx).size() == 3);
static_assert(n.get_members<mb::namespace_alias>(ctx).size() == 1 &&
              n.get_members<mb::namespace_alias>(ctx)[0].get_target() == mb::reflect<^^N::inner>);
static_assert(n.get_members<mb::variable>(ctx).size() == 3 && n.get_members<mb::function>(ctx).size() == 3);
static_assert(n.get_members<mb::type>(ctx).size() == 3 &&
              n.get_members<mb::class_type::with_ability<ab::non_alias>>(ctx).size() == 1);
// parents: members found through a namespace know it; nested namespaces chain to the global namespace
constexpr auto inner = n.get_members<mb::namespace_>(ctx)[0];
static_assert(inner.get_parent().info == n.info);
static_assert(n.get_parent().into<mb::namespace_>() == mb::global_namespace);
static_assert(!mb::global_namespace.has_parent() && !mb::global_namespace.try_get_parent().has_value());
static_assert(inner.get_members<mb::variable>(ctx)[0].get_parent().into<mb::namespace_>().get_parent().info == n.info);
static_assert(n.get_members<mb::variable>(ctx)[1].has_internal_linkage() &&
              n.get_members<mb::variable>(ctx)[2].has_thread_storage_duration());
static_assert(n.get_members<mb::function>(ctx)[2].has_internal_linkage() &&
              n.get_members<mb::function>(ctx)[1].get_parameters().size() == 1);
static_assert(mb::global_namespace.get_members<mb::namespace_>(ctx, mb::predicate::of_name("N")).size() == 1);
static_assert(mb::reflect<^^N::annotated>.get_annotations().size() == 1);
// narrowing
static_assert(!mb::try_make_reflection<mb::namespace_>(^^N::alias_ns).has_value() &&
              mb::try_make_reflection<mb::namespace_alias>(^^N::alias_ns).has_value());
static_assert(mb::try_make_reflection<mb::variable>(^^N::var).has_value() &&
              !mb::try_make_reflection<mb::function>(^^N::var).has_value());
static_assert(!mb::try_make_reflection<mb::namespace_member>(^^::).has_value());
// parent chain across kinds: enumerator -> enum -> its enumerators -> parent again
constexpr auto en = mb::reflect<^^N::E>.get_enumerators()[0];
static_assert(en.get_parent().into<mb::enum_type>().get_enumerators()[0].get_parent().info == ^^N::E);
TEST_CASE("namespace: compile-time checks") {
    SUCCEED("all checks are static_asserts");
}
