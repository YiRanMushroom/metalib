#include <catch2/catch_test_macros.hpp>
#include <metalib/handle.hpp>
#include <string_view>
namespace mb = metalib;
enum class color { red, green, blue };
constexpr mb::enum_type c{^^color};
static_assert(c.enumerators().size() == 3);
static_assert(c.enumerators()[1].ascii_identifier().unwrap() == "green");
static_assert(c.enumerators()[2].extract<color>().unwrap() == color::blue);
static_assert(c.enumerators()[0].type() == mb::type{^^color});
static_assert(mb::handle{^^color::red}.try_into<mb::enumerator>().has_value() && !mb::handle{^^int}.try_into<mb::enumerator>());
static_assert(c.enumerators()[0].u8_display_string().size() > 0);
template <class E>
constexpr std::string_view name_of(E v) {
    template for (constexpr mb::enumerator e : mb::array_literal(mb::enum_type{^^E}.enumerators())) {
        constexpr std::string_view name = e.ascii_identifier().unwrap();
        if (v == e.extract<E>().unwrap()) {
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
