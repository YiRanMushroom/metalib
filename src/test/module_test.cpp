#include <meta>
#include <string_view>
import metalib;
namespace mb = metalib;
struct point { int x; int y; double weight; };
enum class Color { red, green };
namespace outer { int inner = 0; }
template<typename T> struct box {};
constexpr int five = 5;
constexpr std::meta::info forty_two = std::meta::reflect_constant(42);

constexpr mb::class_type cls = mb::reflect<^^point>;
constexpr mb::array_literal ints =
    cls.get_members<mb::data_member>(std::meta::access_context::unchecked(), mb::predicate::of_type<int>());
static_assert(ints.size() == 2 && ints[1].get_identifier() == "y");
static_assert(ints[0].get_type() == mb::reflect<^^int> && ints[0].get_offset().bytes == 0);
static_assert(mb::reflect<^^point::x>.get_parent().into<mb::class_type>() == cls);
static_assert(mb::reflect<^^Color>.get_enumerators().size() == 2);
static_assert(mb::reflect<^^outer>.get_members().size() == 1 && mb::reflect<^^outer::inner>.has_external_linkage());
static_assert(mb::reflect<^^box>.is_class_template() && mb::reflect<^^box<int>>.get_template() == mb::reflect<^^box>);
static_assert(mb::reflect<forty_two>.extract() == 42 && mb::reflect_constant(five).extract() == 5);
static_assert(mb::make_reflection<mb::variable>(^^five).get_type() == mb::reflect<^^int const>);
static_assert(!mb::try_make_reflection<mb::variable>(^^point).has_value());
static_assert(mb::try_make_reflection<mb::variable>(^^point).error().u8message() ==
              u8"reflection 'point' is not a variable");
static_assert(mb::cache::members<mb::make_reflection<mb::class_type>(^^point), mb::data_member>.size() == 3);

namespace mine {
    struct single_letter {
        static constexpr std::u8string_view requirement = u8"an entity with a one-letter name";
        static consteval bool check(std::meta::info r) {
            return std::meta::has_identifier(r) && std::meta::identifier_of(r).size() == 1;
        }
        consteval bool has_single_letter(this mb::handle const &) { return true; }
    };
    namespace abilities {
        constexpr std::meta::info builtin = mb::builtin_auto_ability_registry;
        using single_letter = mine::single_letter;
    }
    constexpr std::meta::info registry = mb::build_registry_from_namespace(^^abilities);
    int x = 0;
}
static_assert(mb::collect_abilities<^^mine::x, mine::registry>.has_single_letter());
static_assert(mb::collect_abilities<^^mine::x, mine::registry>.get_type() == mb::reflect<^^int>);

int main() {
}
