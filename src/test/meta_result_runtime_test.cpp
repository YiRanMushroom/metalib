#include <catch2/catch_test_macros.hpp>
#include <metalib/handle.hpp>
#include <string_view>
#include <utility>
enum class color { red, green, blue };
template <class E>
constexpr std::string_view name_of(E v) {
    template for (constexpr metalib::enumerator e :
                  metalib::array_literal(metalib::make_reflection<metalib::enum_type>(^^E).get_enumerators())) {
        if (v == e.extract<E>()) return e.get_identifier();
    }
    return {};
}
template <class E>
constexpr std::size_t display_size_if_nonzero(E v) {
    return std::to_underlying(v) != 0
               ? metalib::make_reflection<metalib::enum_type>(^^E).get_u8display_string().size()
               : 0;
}
TEST_CASE("meta_result: values cross into runtime") {
    volatile int index = 1;
    color value = static_cast<color>(index);
    REQUIRE(name_of(value) == "green");
    REQUIRE(display_size_if_nonzero(value) == std::meta::u8display_string_of(^^color).size());
    metalib::meta_result<metalib::string_literal> r =
        metalib::make_reflection<metalib::type>(^^color).try_get_identifier();
    REQUIRE(r.has_value());
    REQUIRE(r.unwrap() == "color");
    REQUIRE(r.unwrap_or("fallback") == "color");
}
