#include <catch2/catch_test_macros.hpp>
#include <metalib/handle.hpp>
#include <metalib/reflect.hpp>
#include <type_traits>
namespace mb = metalib;
namespace ab = metalib::ability;
struct S { int x; };
enum class E : short { a, b };
enum class Opaque : int;
using Alias = E;
constexpr mb::type s = mb::reflect<^^S>, i = mb::reflect<^^int>,
                   e = mb::reflect<^^Alias>, f = mb::reflect<^^float>;
static_assert(i.try_into<mb::arithmetic_type>().has_value() && !s.try_into<mb::arithmetic_type>());
constexpr mb::enum_type en = e.into<mb::enum_type>();
static_assert(en.get_underlying_type() == mb::reflect<^^short> &&
              en.make_unsigned() == mb::reflect<^^unsigned short>);
static_assert(en.get_enumerators().size() == 2 && en.get_enumerators()[1].get_identifier() == "b");
static_assert(mb::reflect<^^Opaque>.try_into<mb::enum_type>().has_value());
constexpr mb::arithmetic_type ai = i.into<mb::arithmetic_type>();
static_assert(ai.make_unsigned() == mb::reflect<^^unsigned> &&
              !f.into<mb::arithmetic_type>().try_make_signed() &&
              !mb::reflect<^^bool>.into<mb::arithmetic_type>().try_make_unsigned());
static_assert(en == e && en == mb::reflect<^^E> && !(en == s));
constexpr mb::type back = en;
static_assert(back == en);
constexpr mb::basic_interface<>::with_ability<ab::enumeration, ab::identifiable> view = en;
static_assert(view.get_underlying_type() == mb::reflect<^^short> && view.get_identifier() == "Alias");
static_assert(!std::is_convertible_v<mb::type, mb::enum_type>);
static_assert(mb::type::try_from(mb::handle{^^S}).has_value() && !mb::type::try_from(mb::handle{^^S::x}));
static_assert(mb::try_make_reflection<mb::enum_type>(^^E).has_value());
template <class A, class B> concept eq = requires (A a, B b) { a == b; };
static_assert(!eq<mb::enum_type, std::meta::info>);
TEST_CASE("kinds: compile-time checks") {
    SUCCEED("all checks are static_asserts");
}
