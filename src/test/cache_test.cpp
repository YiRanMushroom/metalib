#include <catch2/catch_test_macros.hpp>
#include <metalib/metalib.hpp>
#include <type_traits>
namespace mb = metalib;
using mb::cache::access;
struct B { int b; };
struct S : B {
    int a;
    double c;
    void f(int x, int y);
private:
    int hidden;
};
enum class color { red, green };
namespace N { int v; void g(); }
template<typename T, int K = 2> struct box { T value[K]; };
constexpr mb::class_type cs{^^S};
// members: same elements as the direct query, public only by default, everything with access::unchecked
static_assert(mb::cache::members<cs, mb::data_member>.size() == 2);
static_assert(mb::cache::members<cs, mb::data_member, access::unchecked>.size() == 3);
static_assert(mb::cache::members<cs, mb::data_member>[1] == cs.members<mb::data_member>(std::meta::access_context::unprivileged())[1]);
static_assert(std::is_same_v<std::remove_cvref_t<decltype(mb::cache::members<cs, mb::data_member>)>, mb::array_literal<mb::data_member>>);
static_assert(mb::cache::members<cs>.size() == cs.members(std::meta::access_context::unprivileged()).size());
static_assert(mb::cache::members<mb::namespace_{^^N}, mb::variable>.size() == 1);
// the cached array is one object: every use refers to the same storage
static_assert(&mb::cache::members<cs, mb::data_member> == &mb::cache::members<cs, mb::data_member>);
static_assert(mb::cache::members<cs, mb::data_member>.data() == mb::cache::members<cs, mb::data_member>.data());
// other cached queries
static_assert(mb::cache::bases<cs>.size() == 1 && mb::cache::subobjects<cs, access::unchecked>.size() == 4);
static_assert(mb::cache::enumerators<mb::enum_type{^^color}>[1].ascii_identifier().unwrap() == "green");
static_assert(mb::cache::parameters<mb::cache::members<cs, mb::member_function>[0]>.size() == 2);
static_assert(mb::cache::template_arguments<mb::class_type{^^box<int>}>.unwrap().size() == 2);
// cached results work in expansion statements
consteval int sum_offsets() {
    int total = 0;
    template for (constexpr mb::data_member m : mb::cache::members<cs, mb::data_member>) {
        total += static_cast<int>(m.offset().bytes);
    }
    return total;
}
static_assert(sum_offsets() == static_cast<int>(sizeof(int) + sizeof(int) + sizeof(int)));
TEST_CASE("cache: compile-time checks") {
    SUCCEED("all checks are static_asserts");
}
