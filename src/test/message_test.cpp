#include <catch2/catch_test_macros.hpp>
#include <metalib/metalib.hpp>
#include <string_view>
namespace mb = metalib;
struct S { int x; unsigned bits : 3; };
constexpr auto x = mb::reflect<mb::data_member>(^^S::x);
static_assert(mb::to_string(x) == std::meta::display_string_of(^^S::x) &&
              mb::to_string(mb::reflect<mb::type>(^^int)) == std::meta::display_string_of(^^int));
static_assert(mb::to_u8string(mb::reflect<mb::type>(^^S)) == std::meta::u8display_string_of(^^S));
// handles, text, integers, bools and chars
static_assert(mb::message("member ", x, " of ", mb::reflect<mb::type>(^^S), " has type ", x.get_type()) ==
              std::string("member ") + std::meta::display_string_of(^^S::x) + " of " +
                  std::meta::display_string_of(^^S) + " has type " + std::meta::display_string_of(^^int));
static_assert(mb::message("bits=", mb::reflect<mb::data_member>(^^S::bits).get_bit_size(), " offset=", 0, " neg=", -42,
                  " ok=", true, ' ', 'c') == "bits=3 offset=0 neg=-42 ok=true c");
static_assert(mb::message(std::string_view("view"), mb::string_literal("literal")) == "viewliteral");
static_assert(mb::message() == "");
// usable directly as a static_assert message
static_assert(x.get_type() == mb::reflect<mb::type>(^^int),
              mb::message("member ", x, " must have type int, has ", x.get_type()));
// narrowing failures name the reflection
static_assert(mb::try_reflect<mb::class_type>(^^int).error().u8message() ==
                  u8"reflection 'int' is not a complete class or union type"
              || mb::try_reflect<mb::class_type>(^^int).error().u8message() ==
                  u8"reflection 'int' is not a class type");
static_assert(mb::try_reflect<mb::data_member>(^^S).error().u8message().starts_with(u8"reflection 'S' is not "));
TEST_CASE("message: compile-time checks") {
    SUCCEED("all checks are static_asserts");
}
