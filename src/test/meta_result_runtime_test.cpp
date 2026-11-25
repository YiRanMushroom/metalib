#include <catch2/catch_test_macros.hpp>
#include <metalib/handle.hpp>
#include <string_view>
enum class color { red, green, blue };
template <class E>
constexpr std::string_view name_of(E v) {
    template for (constexpr metalib::enumerator e : metalib::enum_type{^^E}.enumerators()) {
        if (v == e.extract<E>().unwrap()) return e.ascii_identifier().unwrap();
    }
    return {};
}
template <class E> constexpr std::size_t disp(E v) { if ((int)v) return metalib::enum_type{^^E}.u8_display_string().size(); return 0; }
TEST_CASE("meta_result: values cross into runtime") {
    volatile int index = 1;
    color value = static_cast<color>(index);
    REQUIRE(name_of(value) == "green");
    REQUIRE(disp(value) == 5);
    metalib::meta_result<metalib::string_literal> r = metalib::type{^^color}.ascii_identifier();
    REQUIRE(r.has_value());
    REQUIRE(r.unwrap() == "color");
    REQUIRE(r.unwrap_or("fallback") == "color");
}
