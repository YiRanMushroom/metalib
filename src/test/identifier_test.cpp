#include <catch2/catch_test_macros.hpp>
#include <metalib/handle.hpp>
struct S {}; int 变量;
constexpr metalib::type t{^^S};
static_assert(t.has_ascii_identifier() && t.has_u8_identifier() && t.ascii_identifier().unwrap() == "S");
static_assert(t.u8_identifier().unwrap() == u8"S");
constexpr metalib::type i{^^int};
static_assert(!i.has_ascii_identifier() && !i.ascii_identifier() && i.ascii_identifier().unwrap_or("?") == "?");
constexpr metalib::basic_interface<>::with_ability<metalib::ability::identifiable> v{^^变量};
static_assert(v.has_u8_identifier() && v.u8_identifier().unwrap() == u8"变量");
static_assert(t.ascii_display_string().unwrap() == "S");
static_assert(!v.has_ascii_identifier() && !v.ascii_identifier());
static_assert(std::u8string_view(i.ascii_identifier().error().u8what()) == u8"reflection has no identifier");
constexpr auto stored = i.ascii_identifier();
static_assert(!stored.has_value());
static_assert(metalib::handle{^^int}.try_into<metalib::type>().unwrap().ascii_identifier().error().from() == ^^int);
TEST_CASE("identifier: compile-time checks") {
    SUCCEED("all checks are static_asserts");
}
