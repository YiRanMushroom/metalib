#include <catch2/catch_test_macros.hpp>
#include <metalib/metalib.hpp>
#include <vector>
namespace mb = metalib;
template<typename T, int N = 3> struct CT { T value[N]; };
template<typename T> T twice(T x) { return x + x; }
template<typename T> constexpr T zero = T{};
template<typename T> using ptr = T *;
template<typename T> concept small = sizeof(T) <= 4;
struct S {
    template<typename T> void mf(T) {}
    template<typename T> bool operator==(T const &) const { return true; }
    template<typename T> S(T) {}
    void plain() {}
};
constexpr auto ctx = std::meta::access_context::unchecked();
constexpr mb::template_ ct = mb::handle{^^CT}.try_into<mb::template_>().unwrap();
static_assert(ct.is_class_template() && ct.ascii_identifier().unwrap() == "CT" && ct.parent().unwrap().info == ^^::);
static_assert(!mb::handle{^^S::plain}.try_into<mb::template_>().has_value());
// substitute: result is untyped, narrow with try_into
constexpr auto cti = ct.substitute({mb::type{^^int}}).unwrap().try_into<mb::class_type>().unwrap();
static_assert(cti == mb::type{^^CT<int, 3>} && cti.template_arguments().unwrap().size() == 2);
static_assert(cti.template_().unwrap() == ct && cti.template_arguments().unwrap()[1].try_into<mb::value>().unwrap().extract<int>().unwrap() == 3);
static_assert(!ct.can_substitute({mb::reflect_constant(1)}) && !ct.substitute({}).has_value());
static_assert(ct.substitute({}).error().u8what() == u8"can_substitute returned false");
constexpr auto tw = mb::template_{^^twice}.substitute({mb::type{^^long}}).unwrap().try_into<mb::function>().unwrap();
static_assert(tw.return_type() == mb::type{^^long} && tw.template_().unwrap() == mb::template_{^^twice});
static_assert(mb::template_{^^zero}.substitute({mb::type{^^int}}).unwrap().try_into<mb::variable>().unwrap().template_arguments().unwrap().size() == 1);
static_assert(mb::template_{^^ptr}.substitute({mb::type{^^int}}).unwrap().try_into<mb::type>().unwrap() == mb::type{^^int *});
static_assert(mb::template_{^^small}.is_concept() && mb::template_{^^small}.substitute({mb::type{^^int}}).unwrap().try_into<mb::value>().unwrap().extract<bool>().unwrap());
// member templates
constexpr auto mts = mb::class_type{^^S}.members<mb::member_template>(ctx);
static_assert(mts.size() == 3 && mts[0].is_function_template() && mts[0].is_public());
static_assert(mts[1].operator_kind().unwrap() == std::meta::op_equals_equals && mts[2].is_constructor_template() && !mts[0].operator_kind().has_value());
TEST_CASE("template: compile-time checks") {
    SUCCEED("all checks are static_asserts");
}
