#include <catch2/catch_test_macros.hpp>
#include <metalib/handle.hpp>
namespace mb = metalib;
namespace ab = metalib::ability;
struct S {};
constexpr mb::type t{^^S};
static_assert(std::is_same_v<mb::basic_interface<>::with_ability<ab::identifiable, ab::displayable>, mb::basic_interface<>::with_ability<ab::displayable, ab::identifiable>>);
static_assert(std::is_same_v<mb::basic_interface<>::with_ability<ab::identifiable, ab::identifiable>, mb::basic_interface<>::with_ability<ab::identifiable>>);
constexpr mb::basic_interface<>::with_ability<ab::identifiable> erased = t;
static_assert(erased.ascii_identifier().unwrap() == "S");
static_assert(!std::is_convertible_v<mb::basic_interface<>::with_ability<ab::identifiable>, mb::basic_interface<>::with_ability<ab::identifiable, ab::displayable>>);
TEST_CASE("ability: compile-time checks") {
    SUCCEED("all checks are static_asserts");
}
