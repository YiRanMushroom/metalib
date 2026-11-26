#include <catch2/catch_test_macros.hpp>
#include <metalib/handle.hpp>
struct S {}; int 变量;
constexpr auto t = metalib::reflect<metalib::type>(^^S);
static_assert(t.has_identifier() && t.has_u8identifier() && t.get_identifier() == "S");
static_assert(t.get_u8identifier() == u8"S");
constexpr auto i = metalib::reflect<metalib::type>(^^int);
static_assert(!i.has_identifier() && !i.try_get_identifier() &&
              i.try_get_identifier().unwrap_or("?") == "?");
constexpr auto v = metalib::reflect<metalib::basic_interface<>::with_ability<metalib::ability::identifiable>>(^^变量);
static_assert(v.has_u8identifier() && v.get_u8identifier() == u8"变量");
static_assert(t.get_display_string() == "S");
static_assert(!v.has_identifier() && !v.try_get_identifier());
static_assert(i.try_get_identifier().error().u8message() ==
              u8"reflection '" + std::u8string(std::meta::u8display_string_of(^^int)) + u8"' has no identifier");
constexpr auto stored = i.try_get_identifier();
static_assert(!stored.has_value());
static_assert(metalib::reflect<metalib::type>(^^int).try_get_identifier().error().from() == ^^int);
TEST_CASE("identifier: compile-time checks") {
    SUCCEED("all checks are static_asserts");
}
