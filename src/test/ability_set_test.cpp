#include <catch2/catch_test_macros.hpp>
#include <metalib/metalib.hpp>
#include <tuple>
#include <type_traits>
namespace mb = metalib;
namespace ab = mb::ability;
static_assert(std::is_same_v<mb::basic_interface<>::abilities, std::tuple<>>);
static_assert(std::is_same_v<mb::type::with_ability<ab::record, ab::class_record>, mb::class_type>);
static_assert(std::is_same_v<mb::basic_interface<>::with_ability<std::tuple<mb::type::abilities, std::tuple<ab::record>>, ab::class_record>, mb::class_type>);
static_assert(std::is_same_v<mb::member_function::with_ability<ab::virtual_specifier>, mb::member_function>);
static_assert(std::is_same_v<mb::nested_type, mb::member::with_ability<mb::type::abilities>>);
static_assert(std::is_same_v<mb::nested_class, mb::class_type::with_ability<mb::nested_type::abilities, ab::non_alias>>);
struct S { int x; };
static_assert(mb::data_member{^^S::x}.parent().unwrap().try_into<mb::class_type>().has_value());
static_assert(!mb::data_member{^^S::x}.parent().unwrap().try_into<mb::union_type>().has_value());
static_assert(!mb::type{^^int}.parent().has_value() && mb::type{^^int}.parent().error().u8what() == u8"reflection has no parent");
static_assert(std::is_same_v<mb::ability_set<>, std::tuple<>>);
static_assert(std::is_same_v<mb::ability_set<ab::overridable, ab::virtual_specifier>, ab::overridable>);
static_assert(std::is_same_v<mb::ability_set<ab::override_specifier, ab::virtual_specifier>, ab::overridable>);
static_assert(std::is_same_v<mb::ability_set<mb::member_function::abilities>, mb::member_function::abilities>);
TEST_CASE("ability set: compile-time checks") {
    SUCCEED("all checks are static_asserts");
}
