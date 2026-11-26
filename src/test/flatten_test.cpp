#include <catch2/catch_test_macros.hpp>
#include <metalib/metalib.hpp>
#include <vector>
namespace mb = metalib;
struct B1 {}; struct B2 {};
struct S : B1, B2 { int a; double b; };
constexpr auto ctx = std::meta::access_context::unchecked();
constexpr auto cs = mb::reflect<mb::class_type>(^^S);
constexpr mb::array_literal all = mb::flatten(cs, cs.get_bases(ctx), cs.get_members<mb::data_member>(ctx));
static_assert(all.size() == 5 && all[0].info == (^^S) && all[3].info == (^^S::a) && all[4].info == (^^S::b));
consteval std::size_t nested() {
    std::vector<mb::array_literal<mb::data_member>> groups{cs.get_members<mb::data_member>(ctx),
                                                           cs.get_members<mb::data_member>(ctx)};
    return mb::flatten(groups, mb::reflect<mb::type>(^^int)).size();
}
static_assert(nested() == 5);
static_assert(mb::flatten().empty());
static_assert(mb::flatten(mb::flatten(cs), cs.get_bases(ctx))[2].try_into<mb::base>().has_value());
TEST_CASE("flatten: compile-time checks") {
    SUCCEED("all checks are static_asserts");
}
