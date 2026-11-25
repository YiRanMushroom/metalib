#include <catch2/catch_test_macros.hpp>
#include <metalib/metalib.hpp>
#include <string_view>
namespace mb = metalib;
struct S { int x; unsigned bits : 3; };
constexpr mb::data_member x{^^S::x};
static_assert(mb::to_string(x) == "S::x" && mb::to_string(mb::type{^^int}) == "int");
static_assert(mb::to_u8string(mb::type{^^S}) == u8"S");
// handles, text, integers, bools and chars
static_assert(mb::message("member ", x, " of ", mb::type{^^S}, " has type ", x.type()) == "member S::x of S has type int");
static_assert(mb::message("bits=", mb::data_member{^^S::bits}.bit_size(), " offset=", 0, " neg=", -42, " ok=", true, ' ', 'c') == "bits=3 offset=0 neg=-42 ok=true c");
static_assert(mb::message(std::string_view("view"), mb::string_literal("literal")) == "viewliteral");
static_assert(mb::message() == "");
// usable directly as a static_assert message
static_assert(x.type() == mb::type{^^int}, mb::message("member ", x, " must have type int, has ", x.type()));
// narrowing failures name the reflection
static_assert(mb::class_type::from(mb::handle{^^int}).error().u8what() == u8"reflection 'int' is not a complete class or union type"
              || mb::class_type::from(mb::handle{^^int}).error().u8what() == u8"reflection 'int' is not a class type");
static_assert(mb::data_member::from(mb::handle{^^S}).error().u8what().starts_with(u8"reflection 'S' is not "));
TEST_CASE("message: compile-time checks") {
    SUCCEED("all checks are static_asserts");
}
