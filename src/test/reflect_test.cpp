#include <catch2/catch_test_macros.hpp>
#include <metalib/metalib.hpp>
#include <concepts>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>
namespace mb = metalib;
namespace ab = metalib::ability;
namespace extra {
    struct single_letter {
        static constexpr std::u8string_view requirement = u8"an entity with a one-letter name";
        static consteval bool check(std::meta::info r) {
            return std::meta::has_identifier(r) && std::meta::identifier_of(r).size() == 1;
        }
        consteval bool has_single_letter(this mb::handle const &) { return true; }
    };
    struct unexplained {
        static consteval bool check(std::meta::info r) { return std::meta::is_type(r); }
    };
    namespace registry {
        namespace builtin = mb::builtin::auto_ability_root;
        using single_letter = extra::single_letter;
    }
    constexpr std::meta::info built = mb::build_registry_from_namespace(^^registry);
    template<std::meta::info R> constexpr auto reflect = mb::collect_abilities<R, built>;
}
namespace metalib::builtin::auto_ability_root { using added_after_the_header = extra::single_letter; }
namespace frozen { using single_letter = extra::single_letter; }
constexpr std::meta::info frozen_registry = mb::build_registry_from_namespace(^^frozen);
namespace composed {
    constexpr std::meta::info builtin = mb::builtin_auto_ability_registry;
    using single_letter = extra::single_letter;
}
constexpr std::meta::info composed_registry = mb::build_registry_from_namespace(^^composed);
namespace composed { int q = 0; }
namespace frozen { using unexplained = extra::unexplained; }
namespace cyclic { namespace inner { namespace back = cyclic; } }
namespace hollow { using single_letter = extra::single_letter; namespace nothing {} }
int x = 0;
constexpr int five = 5;
void fn([[=1]] int first);
enum class Color { red };
struct Base {};
struct [[=7]] Shape : Base {
    int side; static int count; void draw(); static void reset(); struct Nested {}; using Alias = int;
    Shape(); ~Shape(); template<typename> struct Member {};
};
struct Pair { int left; int right; void swap(); };
enum Plain { plain_value };
struct Bits { int flag : 1; enum Inner { inner_value }; };
inline namespace versioned { int inlined = 0; }
namespace { int hidden = 0; }
using Renamed = Shape;
auto [first_part, second_part] = Pair{1, 2};
constexpr int triple[3] = {1, 2, 3};
consteval std::meta::info special_member(bool destructor) {
    for (std::meta::info member: std::meta::members_of(^^Shape, std::meta::access_context::unchecked())) {
        if (destructor ? std::meta::is_destructor(member)
                       : std::meta::is_constructor(member) && std::meta::is_user_provided(member)) {
            return member;
        }
    }
    return {};
}
constexpr std::meta::info constructor_info = special_member(false);
constexpr std::meta::info destructor_info = special_member(true);
constexpr std::meta::info array_info = std::meta::reflect_object(triple);
namespace outer { namespace inner {} }
namespace outer_alias = outer;
union Either { int number; };
template<typename T> T zero = T();
template<typename T> concept Any = true;
constexpr std::meta::info value_info = std::meta::reflect_constant(42);
constexpr std::meta::info object_info = std::meta::reflect_object(five);
constexpr std::meta::info annotation_info = std::meta::annotations_of(^^Shape)[0];
constexpr std::meta::info parameter_info = std::meta::parameters_of(^^fn)[0];
constexpr std::meta::info base_info = std::meta::bases_of(^^Shape, std::meta::access_context::unchecked())[0];
constexpr std::meta::info description_info = std::meta::data_member_spec(^^int, {.name = "added"});
template<std::meta::info R> using deduced = std::remove_cv_t<decltype(mb::reflect<R>)>;
template<std::meta::info R, typename K> concept gives = std::convertible_to<deduced<R>, K>;
template<std::meta::info R> concept has_annotations = requires { mb::reflect<R>.get_annotations(); };
template<std::meta::info R> concept has_identifier = requires { mb::reflect<R>.get_identifier(); };
template<typename K> concept has_single_letter = requires(K kind) { kind.has_single_letter(); };

static_assert(gives<^^x, mb::variable> && gives<^^fn, mb::function> && gives<^^int, mb::arithmetic_type>);
static_assert(gives<^^Color, mb::enum_type> && gives<^^Color::red, mb::enumerator> && gives<^^Shape, mb::class_type>);
static_assert(gives<^^Either, mb::union_type> && gives<^^Shape::side, mb::data_member>);
static_assert(gives<^^Shape::count, mb::static_data_member> && gives<^^Shape::draw, mb::member_function>);
static_assert(gives<^^Shape::reset, mb::static_member_function> && gives<^^Shape::Nested, mb::nested_class>);
static_assert(gives<^^Shape::Alias, mb::nested_alias> && gives<^^Shape::Alias, mb::arithmetic_type>);
static_assert(gives<^^outer, mb::namespace_> && gives<^^outer::inner, mb::namespace_member>);
static_assert(gives<^^outer_alias, mb::namespace_alias> && gives<^^::, mb::namespace_>);
static_assert(gives<^^std::vector, mb::template_> && gives<^^Shape::Member, mb::member_template>);
static_assert(gives<^^zero, mb::template_> && gives<^^Any, mb::template_> && gives<^^zero<int>, mb::variable>);
static_assert(gives<parameter_info, mb::parameter> && gives<base_info, mb::base>);
static_assert(gives<description_info, mb::data_member_description>);
static_assert(gives<constructor_info, mb::constructor> && gives<destructor_info, mb::destructor>);
static_assert(gives<^^first_part, mb::structured_binding> && !gives<constructor_info, mb::member_function>);
static_assert(mb::reflect<constructor_info>.is_default_constructor() && !mb::reflect<destructor_info>.is_virtual());

static_assert(!gives<^^Shape::side, mb::variable> && !gives<^^x, mb::member> && !gives<^^Shape, mb::enum_type>);
static_assert(gives<^^Shape, mb::namespace_member> && !gives<^^int, mb::namespace_member>);
static_assert(!gives<^^::, mb::namespace_member> && !gives<^^Shape::Nested, mb::namespace_member>);
static_assert(!has_annotations<^^std::vector> && !has_annotations<^^Shape::Member> && !has_annotations<value_info>);
static_assert(has_annotations<^^outer> && has_annotations<^^Shape::draw> && has_annotations<^^Shape::Alias>);

static_assert(mb::reflect<^^x>.get_identifier() == "x" && mb::reflect<^^x>.get_type() == mb::reflect<^^int>);
static_assert(mb::reflect<^^x> == mb::make_reflection<mb::variable>(^^x));
static_assert(mb::reflect<^^Shape>.get_bases().size() == 1 && mb::reflect<^^Shape>.get_annotations().size() == 1);
static_assert(mb::reflect<^^Shape::draw>.get_return_type() == mb::reflect<^^void>);
static_assert(mb::reflect<^^Shape::side>.get_parent().into<mb::class_type>() == mb::reflect<^^Shape>);
static_assert(mb::reflect<^^Color>.get_enumerators().size() == 1 &&
              mb::reflect<^^Color::red>.extract<Color>() == Color::red);
static_assert(mb::reflect<^^std::vector>.is_class_template() && mb::reflect<^^fn>.get_parameters().size() == 1);
static_assert(mb::reflect<parameter_info>.get_annotations().size() == 1 && mb::reflect<base_info>.get_size() == 1);
static_assert(mb::reflect<^^outer>.get_members().size() == 1 &&
              mb::reflect<^^outer_alias>.get_target() == mb::reflect<^^outer>);

static_assert(gives<value_info, mb::typed_constant<int>> && mb::reflect<value_info>.extract() == 42);
static_assert(mb::reflect<value_info>.is_value_constant() && mb::reflect<value_info>.get_size() == sizeof(int));
static_assert(gives<object_info, mb::typed_object<int>> && mb::reflect<object_info>.extract() == 5);
static_assert(gives<annotation_info, mb::typed_annotation<int>> && mb::reflect<annotation_info>.extract() == 7);
static_assert(!gives<value_info, mb::typed_constant<long>> && !gives<value_info, mb::typed_object<int>>);
static_assert(!gives<value_info, mb::constant> && !gives<value_info, mb::value> && !gives<object_info, mb::object>);
static_assert(gives<array_info, mb::object> && mb::reflect<array_info>.extract<int const *>()[2] == 3);
static_assert(std::same_as<decltype(mb::reflect_object(triple)), mb::object>);
static_assert(std::same_as<deduced<^^x>, deduced<^^x>::with_ability<>>);
static_assert(std::same_as<deduced<^^Shape::draw>, deduced<^^Shape::draw>::with_ability<>>);
static_assert(std::same_as<deduced<value_info>, deduced<value_info>::with_ability<>>);
static_assert(!has_identifier<value_info> && !has_identifier<base_info> && has_identifier<^^Shape>);

static_assert(std::same_as<deduced<std::meta::info{}>, mb::basic_interface<ab::displayable>>);

static_assert(extra::reflect<^^x>.has_single_letter() && extra::reflect<^^x>.get_identifier() == "x");
static_assert(!has_single_letter<decltype(extra::reflect<^^fn>)> && !has_single_letter<deduced<^^x>>);
static_assert(std::convertible_to<decltype(extra::reflect<^^x>), deduced<^^x>>);

static_assert(mb::try_make_reflection<mb::basic_interface<ab::annotatable>>(^^std::vector).error().u8message()
              .ends_with(u8" cannot have annotations"));
static_assert(mb::try_make_reflection<mb::variable>(^^Shape).error().u8message() ==
              u8"reflection 'Shape' is not a variable");
static_assert(mb::try_make_reflection<mb::namespace_member>(^^int).error().u8message() ==
              u8"reflection 'int' is not a namespace member");
static_assert(mb::try_make_reflection<mb::function>(^^Shape).error().u8message() ==
              u8"reflection 'Shape' is not a function");
static_assert(mb::try_make_reflection<mb::basic_interface<ab::callable>>(^^x).error().u8message() ==
              u8"reflection 'x' cannot be called");
static_assert(mb::try_make_reflection<mb::basic_interface<ab::signature>>(constructor_info).error().u8message()
              .ends_with(u8" cannot have a return type"));
static_assert(!mb::basic_interface<extra::unexplained>::holds(^^x) &&
              mb::try_make_reflection<mb::basic_interface<extra::unexplained>>(^^x).error().u8message() ==
              u8"reflection 'x' does not have the ability 'extra::unexplained'");

consteval int count_with_offset() {
    int count = 0;
    template for (constexpr std::meta::info member:
                  std::define_static_array(std::meta::members_of(^^Pair, std::meta::access_context::unchecked()))) {
        if constexpr (requires { mb::reflect<member>.get_offset(); }) {
            ++count;
        }
    }
    return count;
}
static_assert(count_with_offset() == 2);

consteval std::vector<std::string_view> names_of(std::meta::info ability) {
    std::vector<std::string_view> names;
    for (std::meta::info member: std::meta::members_of(ability, std::meta::access_context::unchecked())) {
        if (std::meta::has_identifier(member) && !std::meta::is_variable(member) && !std::meta::is_type(member)) {
            std::string_view const name = std::meta::identifier_of(member);
            if (name != "check" && name != "equal" && !std::ranges::contains(names, name)) {
                names.push_back(name);
            }
        }
    }
    return names;
}
consteval std::vector<std::meta::info> registered_abilities() {
    return mb::details::registered_in(mb::builtin_auto_ability_registry);
}
consteval bool every_ability_is_registered() {
    std::vector<std::meta::info> const registered = registered_abilities();
    for (std::meta::info member: std::meta::members_of(^^ab, std::meta::access_context::unchecked())) {
        if (std::meta::is_type(member) && !std::meta::is_type_alias(member) &&
            !std::ranges::contains(registered, member)) {
            return false;
        }
    }
    return true;
}
static_assert(every_ability_is_registered() && registered_abilities().size() == 48);
static_assert(mb::details::registered_in(frozen_registry).size() == 1 &&
              mb::details::registered_in(extra::built).size() == 49 &&
              mb::details::registered_in(composed_registry).size() == 49);
static_assert(mb::collect_abilities<^^composed::q, composed_registry> == extra::reflect<^^composed::q>);
static_assert(mb::collect_abilities<^^composed::q, composed_registry>.has_single_letter());
static_assert(std::meta::members_of(^^frozen, std::meta::access_context::unchecked()).size() == 2);

consteval std::string mismatches() {
    std::vector<std::meta::info> const samples = {
        ^^x, ^^five, ^^fn, ^^int, ^^int *, ^^int &, ^^void (), ^^Color, ^^Color::red, ^^Base, ^^Shape, ^^Either,
        ^^Shape::side, ^^Shape::count, ^^Shape::draw, ^^Shape::reset, ^^Shape::Nested, ^^Shape::Alias,
        ^^Shape::Member, ^^Pair::swap, ^^outer, ^^outer::inner, ^^outer_alias, ^^::, ^^std::vector,
        ^^std::vector<int>, ^^zero, ^^Any, ^^zero<int>, ^^first_part, constructor_info, destructor_info, array_info,
        value_info, object_info, annotation_info, parameter_info, base_info, description_info, std::meta::info{},
        ^^plain_value, ^^Plain, ^^Bits::flag, ^^Bits::inner_value, ^^Bits::Inner, ^^versioned, ^^inlined, ^^hidden,
        ^^Renamed, ^^Shape::Nested *, ^^int[3], ^^decltype([] {})};
    auto const is_typed_as = [](std::meta::info ability) {
        return std::meta::has_template_arguments(ability) && std::meta::template_of(ability) == ^^ab::typed_as;
    };
    std::string text;
    for (std::meta::info sample: samples) {
        std::vector<std::meta::info> collected =
            std::meta::template_arguments_of(mb::details::collected_kind<mb::builtin_auto_ability_registry>(sample));
        bool const typed = std::erase_if(collected, is_typed_as) != 0;
        std::vector<std::meta::info> expected;
        template for (constexpr std::meta::info ability:
                      std::define_static_array(mb::details::registered_in(mb::builtin_auto_ability_registry))) {
            if (mb::details::ability_holds<typename [:ability:]>(sample) && !(typed && ability == ^^ab::constant)) {
                expected.push_back(ability);
            }
        }
        if (collected != expected) {
            text += std::meta::display_string_of(sample);
            text += ": collected";
            for (std::meta::info ability: collected) {
                text += " ";
                text += std::meta::display_string_of(ability);
            }
            text += ", expected";
            for (std::meta::info ability: expected) {
                text += " ";
                text += std::meta::display_string_of(ability);
            }
            text += "; ";
        }
    }
    return text;
}
constexpr std::string_view mismatched = std::define_static_string(mismatches());
static_assert(mismatched.empty(), mismatched);

consteval std::u8string not_a_registry_message() {
    try {
        mb::details::registered_in(^^extra::registry);
    } catch (mb::meta_exception const &failure) {
        return std::u8string(failure.error().u8message());
    }
    return {};
}
consteval std::u8string cycle_message(std::meta::info root) {
    try {
        mb::build_registry_from_namespace(root);
    } catch (mb::meta_exception const &failure) {
        return std::u8string(failure.error().u8message());
    }
    return {};
}
consteval std::u8string quoted(std::meta::info r) {
    return u8"'" + std::u8string(std::meta::u8display_string_of(r)) + u8"'";
}
static_assert(cycle_message(^^mb::builtin::auto_ability_root).empty() && cycle_message(^^extra::registry).empty());
static_assert(not_a_registry_message() ==
              quoted(^^extra::registry) + u8" is not an ability registry made by build_registry_from_namespace");
static_assert(cycle_message(^^hollow) == u8"ability registry " + quoted(^^hollow::nothing) + u8" has no entries");
static_assert(cycle_message(^^cyclic) ==
              u8"ability registry " + quoted(^^cyclic) + u8" has a cycle: " + quoted(^^cyclic) + u8" contains " +
              quoted(^^cyclic::inner) + u8" contains " + quoted(^^cyclic));

consteval std::vector<std::meta::info> conflicting_pairs() {
    std::vector<std::meta::info> const abilities = registered_abilities();
    std::vector<std::vector<std::string_view>> names;
    for (std::meta::info ability: abilities) {
        names.push_back(names_of(ability));
    }
    std::vector<std::meta::info> pairs;
    for (std::size_t i = 0; i < abilities.size(); ++i) {
        for (std::size_t j = i + 1; j < abilities.size(); ++j) {
            auto const shared = [&](std::string_view name) { return std::ranges::contains(names[j], name); };
            if (std::ranges::any_of(names[i], shared)) {
                pairs.push_back(abilities[i]);
                pairs.push_back(abilities[j]);
            }
        }
    }
    return pairs;
}
constexpr auto conflicts = std::define_static_array(conflicting_pairs());
consteval bool names_are_unique(std::meta::info kind) {
    std::vector<std::meta::info> const abilities = std::meta::template_arguments_of(kind);
    for (std::size_t i = 0; i < conflicts.size(); i += 2) {
        if (std::ranges::contains(abilities, conflicts[i]) && std::ranges::contains(abilities, conflicts[i + 1])) {
            return false;
        }
    }
    return true;
}
template<std::meta::info... R>
constexpr bool unique_names = (names_are_unique(std::meta::dealias(^^deduced<R>)) && ...);
static_assert(names_of(^^ab::identifiable).size() == 6 && conflicts.size() >= 2 &&
              !names_are_unique(^^mb::basic_interface<ab::record, ab::namespace_entity>));
static_assert(unique_names<^^x, ^^fn, ^^int, ^^Color, ^^Color::red, ^^Shape, ^^Either, ^^Shape::side, ^^Shape::count>);
static_assert(unique_names<^^Shape::draw, ^^Shape::reset, ^^Shape::Nested, ^^Shape::Alias, ^^outer, ^^outer_alias,
                           ^^::>);
static_assert(unique_names<^^std::vector, ^^Shape::Member, ^^zero, ^^Any, ^^zero<int>, parameter_info, base_info>);
static_assert(unique_names<description_info, value_info, object_info, annotation_info, ^^int *, ^^void (), ^^int &>);
static_assert(unique_names<constructor_info, destructor_info, ^^first_part, array_info, std::meta::info{}>);
TEST_CASE("reflect: compile-time checks") {
    SUCCEED("all checks are static_asserts");
}
