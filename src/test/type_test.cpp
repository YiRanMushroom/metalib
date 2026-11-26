#include <catch2/catch_test_macros.hpp>
#include <metalib/handle.hpp>
namespace mb = metalib;
struct S { int x; };
struct D : S {};
enum class E : short { a };
struct [[=1]] A {};
using Alias = S;
template <class T> struct box {};
constexpr mb::type s = mb::reflect<mb::type>(^^S), alias = mb::reflect<mb::type>(^^Alias),
                   i = mb::reflect<mb::type>(^^int), ref = mb::reflect<mb::type>(^^S&),
                   d = mb::reflect<mb::type>(^^D), l = mb::reflect<mb::type>(^^long);
static_assert(s == alias && s.is(alias) && !(s == i));
template <class A, class B> concept eq = requires (A a, B b) { a == b; };
static_assert(!eq<mb::type, std::meta::info> && !eq<std::meta::info, mb::type>);
static_assert(s.get_size() == sizeof(S) && s.get_alignment() == alignof(S));
struct Incomplete;
auto undeduced();
static_assert(!mb::reflect<mb::type>(^^Incomplete).try_get_size().has_value() &&
              !mb::reflect<mb::type>(^^Incomplete).try_get_alignment().has_value());
static_assert(mb::reflect<mb::type>(^^Incomplete).try_get_size().error().u8message() ==
              u8"unable to get size() from '" + std::u8string(std::meta::u8display_string_of(^^Incomplete)) + u8"'");
static_assert(!mb::reflect<mb::function>(^^undeduced).try_get_type().has_value() &&
              !mb::reflect<mb::function>(^^undeduced).try_get_return_type().has_value());
static_assert(s.is_class() && s.is_aggregate() && s.is_trivially_copyable() && !i.is_class());
static_assert(s.is_base_of(d) && i.is_convertible_to(l) && s.is_constructible_from({i}));
static_assert(ref.remove_cvref() == s && i.add_const().is_const());
constexpr auto b = mb::reflect<mb::type>(^^box<int>);
static_assert(b.has_template_arguments() && b.get_template().info == ^^box &&
              b.get_template_arguments()[0].info == (^^int) && !s.has_template_arguments());
static_assert(mb::reflect<mb::type>(^^A).get_annotations().size() == 1 && s.get_annotations().size() == 0);
static_assert(mb::reflect<mb::type>(^^A).get_annotations_with_type(i).size() == 1);
constexpr auto stored = s.try_get_template_arguments();
static_assert(!stored);
TEST_CASE("type: compile-time checks") {
    SUCCEED("all checks are static_asserts");
}
