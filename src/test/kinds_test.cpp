#include <catch2/catch_test_macros.hpp>
#include <metalib/handle.hpp>
namespace mb = metalib;
namespace ab = metalib::ability;
struct S { int x; };
enum class E : short { a, b };
enum class Opaque : int;
using Alias = E;
constexpr mb::type s{^^S}, i{^^int}, e{^^Alias}, f{^^float};
static_assert(i.try_into<mb::arithmetic_type>().has_value() && !s.try_into<mb::arithmetic_type>());
constexpr mb::enum_type en = e.try_into<mb::enum_type>().unwrap();
static_assert(en.underlying_type() == mb::type{^^short} && en.make_unsigned() == mb::type{^^unsigned short});
static_assert(en.enumerators().size() == 2 && en.enumerators()[1].ascii_identifier().unwrap() == "b");
static_assert(mb::type{^^Opaque}.try_into<mb::enum_type>().has_value());
constexpr mb::arithmetic_type ai = i.try_into<mb::arithmetic_type>().unwrap();
static_assert(ai.make_unsigned().unwrap() == mb::type{^^unsigned} && !f.try_into<mb::arithmetic_type>().unwrap().make_signed() && !mb::type{^^bool}.try_into<mb::arithmetic_type>().unwrap().make_unsigned());
static_assert(en == e && en == mb::type{^^E} && !(en == s));
constexpr mb::type back = en;
static_assert(back == en);
constexpr mb::basic_interface<>::with_ability<ab::enumeration, ab::identifiable> view = en;
static_assert(view.underlying_type() == mb::type{^^short} && view.ascii_identifier().unwrap() == "Alias");
static_assert(!std::is_convertible_v<mb::type, mb::enum_type>);
static_assert(mb::type::from(mb::handle{^^S}).has_value() && !mb::type::from(mb::handle{^^S::x}));
static_assert(mb::handle{^^E}.try_into<mb::enum_type>().has_value());
template <class A, class B> concept eq = requires (A a, B b) { a == b; };
static_assert(!eq<mb::enum_type, std::meta::info>);
TEST_CASE("kinds: compile-time checks") {
    SUCCEED("all checks are static_asserts");
}
