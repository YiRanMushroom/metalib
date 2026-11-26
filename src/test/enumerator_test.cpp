#include <catch2/catch_test_macros.hpp>
#include <metalib/handle.hpp>
#include <metalib/reflect.hpp>
#include <string_view>
namespace mb = metalib;
enum class color { red, green, blue };
constexpr auto c = mb::reflect<^^color>;
static_assert(c.get_enumerators().size() == 3);
static_assert(c.get_enumerators()[1].get_identifier() == "green");
static_assert(c.get_enumerators()[2].extract<color>() == color::blue);
static_assert(c.get_enumerators()[0].get_type() == mb::reflect<^^color>);
static_assert(mb::try_make_reflection<mb::enumerator>(^^color::red).has_value() &&
              !mb::try_make_reflection<mb::enumerator>(^^int));
static_assert(c.get_enumerators()[0].get_u8display_string() == std::meta::u8display_string_of(^^color::red));
template <class E>
constexpr std::string_view name_of(E v) {
    template for (constexpr mb::enumerator e : mb::array_literal(mb::reflect<^^E>.get_enumerators())) {
        constexpr std::string_view name = e.get_identifier();
        if (v == e.extract<E>()) {
            return name;
        }
    }
    return {};
}
static_assert(name_of(color::green) == "green");
TEST_CASE("enumerator: name of a runtime value") {
    volatile int index = 2;
    REQUIRE(name_of(static_cast<color>(index)) == "blue");
    REQUIRE(name_of(color::red) == "red");
}
