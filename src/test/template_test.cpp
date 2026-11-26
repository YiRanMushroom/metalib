#include <catch2/catch_test_macros.hpp>
#include <metalib/metalib.hpp>
#include <vector>
namespace mb = metalib;
template<typename T, int N = 3> struct CT { T value[N]; };
template<typename T> T twice(T x) { return x + x; }
template<typename T> constexpr T zero = T{};
template<typename T> using ptr = T *;
template<typename T> concept small = sizeof(T) == 1;
struct S {
    template<typename T> void mf(T) {}
    template<typename T> bool operator==(T const &) const { return true; }
    template<typename T> S(T) {}
    void plain() {}
};
constexpr auto ctx = std::meta::access_context::unchecked();
constexpr mb::template_ ct = mb::reflect<^^CT>;
static_assert(ct.is_class_template() && ct.get_identifier() == "CT" && ct.get_parent().info == ^^::);
static_assert(!mb::try_make_reflection<mb::template_>(^^S::plain).has_value());
// substitute: the result is untyped; narrow it with into / try_into
constexpr auto cti = ct.substitute({mb::reflect<^^int>}).into<mb::class_type>();
static_assert(cti == mb::reflect<^^CT<int, 3>> && cti.get_template_arguments().size() == 2);
static_assert(cti.get_template() == ct && cti.get_template_arguments()[1].into<mb::value>().extract<int>() == 3);
static_assert(!ct.can_substitute({mb::reflect_constant(1)}) && !ct.try_substitute({}).has_value());
static_assert(ct.try_substitute({}).error().u8action() == u8"substitute()");
constexpr auto tw =
    mb::reflect<^^twice>.substitute({mb::reflect<^^long>}).into<mb::function>();
static_assert(tw.get_return_type() == mb::reflect<^^long> &&
              tw.get_template() == mb::reflect<^^twice>);
static_assert(mb::reflect<^^zero>.substitute({mb::reflect<^^int>}).into<mb::variable>()
                  .get_template_arguments().size() == 1);
static_assert(mb::reflect<^^ptr>.substitute({mb::reflect<^^int>}).into<mb::type>() ==
              mb::reflect<^^int *>);
static_assert(mb::reflect<^^small>.is_concept() &&
              mb::reflect<^^small>.substitute({mb::reflect<^^char>})
                  .into<mb::value>().extract<bool>());
// member templates
constexpr mb::array_literal mts = mb::reflect<^^S>.get_members<mb::member_template>(ctx);
static_assert(mts.size() == 3 && mts[0].is_function_template() && mts[0].is_public());
static_assert(mts[1].get_operator_kind() == std::meta::op_equals_equals && mts[2].is_constructor_template() &&
              !mts[0].try_get_operator_kind().has_value());
template<class T> requires (sizeof(T) == 1) struct one_byte_box {};
constexpr auto bad_substitution =
    mb::reflect<^^one_byte_box>.try_substitute({mb::reflect<^^long>});
static_assert(!bad_substitution.has_value());
consteval std::u8string expected_substitution_message() {
    return u8"unable to substitute(" + std::u8string(std::meta::u8display_string_of(^^long)) + u8") into '" +
           std::u8string(std::meta::u8display_string_of(^^one_byte_box)) + u8"'";
}
static_assert(bad_substitution.error().u8message() == expected_substitution_message());
static_assert(bad_substitution.error().u8action() ==
              u8"substitute(" + std::u8string(std::meta::u8display_string_of(^^long)) + u8")");
static_assert(bad_substitution.error().from() == ^^one_byte_box);
static_assert(bad_substitution.try_into<mb::class_type>().error().u8message() == expected_substitution_message());
static_assert(mb::reflect<^^one_byte_box>.try_substitute({mb::reflect<^^char>})
                  .try_into<mb::class_type>().has_value());

TEST_CASE("template: compile-time checks") {
    SUCCEED("all checks are static_asserts");
}
