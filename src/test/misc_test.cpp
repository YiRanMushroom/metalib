#include <catch2/catch_test_macros.hpp>
#include <metalib/metalib.hpp>
#include <type_traits>
#include <utility>
#include <variant>
namespace mb = metalib;
struct [[=1, =2.5]] A { int x; };
struct B : A { double y; B &operator=(B const &) = default; };
struct P { int a; double b; };
auto [s1, s2] = P{1, 2.0};
constexpr int cv = 3;
int gv;
int operator""_lit(unsigned long long) { return 0; }
void takes(int named);
constexpr auto ctx = std::meta::access_context::unchecked();
// type traits and transforms
constexpr mb::type tint = mb::reflect<^^int>,
                   tpair = mb::reflect<^^std::pair<int, char>>,
                   tvar = mb::reflect<^^std::variant<int, A>>;
static_assert(tpair.get_tuple_size() == 2 && !tint.try_get_tuple_size().has_value());
static_assert(tpair.get_tuple_element(1) == mb::reflect<^^char> &&
              !tpair.try_get_tuple_element(2).has_value());
static_assert(tvar.get_variant_size() == 2 && tvar.get_variant_alternative(1) == mb::reflect<^^A> &&
              !tint.try_get_variant_size().has_value());
static_assert(tint.find_common_type_with({mb::reflect<^^long>}) == mb::reflect<^^long>);
static_assert(!tint.try_find_common_type_with({mb::reflect<^^A>}).has_value());
static_assert(tint.try_find_common_type_with({mb::reflect<^^A>}).error().u8message() ==
              u8"unable to find common_type_with(" + std::u8string(std::meta::u8display_string_of(^^A)) + u8") from '" +
                  std::u8string(std::meta::u8display_string_of(^^int)) + u8"'");
static_assert(mb::reflect<^^int &>.find_common_reference_with({mb::reflect<^^int const &>}) ==
              mb::reflect<^^int const &>);
static_assert(mb::reflect<^^int (*)(char)>.find_invoke_result_with({mb::reflect<^^char>}) == tint);
static_assert(!mb::reflect<^^int (*)(char)>
                  .try_find_invoke_result_with({mb::reflect<^^A>}).has_value());
static_assert(mb::reflect<^^int const &>.reference_constructs_from_temporary(mb::reflect<^^long>));
// annotations, values, objects
constexpr mb::array_literal anns = mb::reflect<^^A>.get_annotations();
static_assert(anns.size() == 2 && anns[0].extract<int>() == 1 && anns[1].get_type() == mb::reflect<^^double>);
static_assert(anns[0].get_constant().into<mb::value>().extract<int>() == 1);
static_assert(mb::try_make_reflection<mb::annotation>(anns[0].info).has_value() &&
              !mb::try_make_reflection<mb::annotation>(^^A).has_value());
static_assert(std::is_same_v<decltype(mb::reflect_constant(42)), mb::typed_constant<int>> &&
              std::is_same_v<decltype(mb::reflect_constant(P{1, 2.0})), mb::typed_object<P>> &&
              std::is_same_v<decltype(mb::reflect_object(gv)), mb::typed_object<int>>);
static_assert(mb::reflect_constant(42).extract() == 42 && mb::reflect_constant(P{1, 2.0}).extract().a == 1);
static_assert(mb::reflect_constant(42).into<mb::constant>().extract<int>() == 42 &&
              !std::is_convertible_v<mb::constant, mb::typed_constant<int>>);
static_assert(std::is_same_v<mb::except<mb::value::abilities, mb::ability::constant, mb::ability::typed>,
                             mb::ability_set<mb::ability::displayable, mb::ability::value_entity>> &&
              std::is_same_v<mb::typed_constant<int>::without_ability<mb::ability::typed_as<int>>,
                             mb::constant::without_ability<mb::ability::constant>>);
// typed annotations
constexpr mb::array_literal int_anns = mb::reflect<^^A>.get_annotations_with_type<int>();
static_assert(int_anns.size() == 1 && int_anns[0].extract() == 1);
static_assert(anns[1].into<mb::typed_annotation<double>>().extract() == 2.5 &&
              !anns[1].try_into<mb::typed_annotation<int>>().has_value());
static_assert(anns[1].try_into<mb::typed_annotation<int>>().error().u8message().ends_with(
    u8"is not of type " + std::u8string(std::meta::u8display_string_of(^^int))));
static_assert(mb::reflect_constant(42).is_value_constant() && !mb::reflect_constant(42).is_object_constant());
static_assert(std::is_same_v<decltype(mb::reflect_constant_string(std::string_view("hi"))), mb::object>);
static_assert(mb::reflect_constant_string(std::string_view("hi")).get_type() == mb::reflect<^^char const[3]>);
static_assert(mb::try_make_reflection<mb::constant>(mb::reflect_constant(42).info).has_value() &&
              !mb::try_make_reflection<mb::constant>(^^int).has_value());
static_assert(mb::reflect_object(gv).get_type() == tint);
static_assert(mb::reflect_function(takes).try_into<mb::function>().has_value());
static_assert(mb::reflect<^^gv>.get_object() == mb::reflect_object(gv));
// extract: member and function pointers directly, references to variables through get_object()
struct E { int x = 1; static int sv; int f() const { return 3; } };
int E::sv = 7;
constexpr auto ex = mb::reflect<^^E>.get_members<mb::data_member>(ctx)[0];
static_assert(ex.extract<int E::*>() == &E::x && !ex.try_extract<long E::*>().has_value());
consteval std::u8string expected_extract_action() {
    return u8"extract<" + std::u8string(std::meta::u8display_string_of(^^long E::*)) + u8">()";
}
static_assert(ex.try_extract<long E::*>().error().u8action() == expected_extract_action());
static_assert(ex.try_extract<long E::*>().error().u8message() ==
              u8"unable to " + expected_extract_action() + u8" from '" +
                  std::u8string(std::meta::u8display_string_of(ex.info)) + u8"' of type '" +
                  std::u8string(std::meta::u8display_string_of(^^int)) + u8"'");
static_assert(mb::reflect<^^E>.get_members<mb::member_function>(ctx, mb::predicate::of_name("f"))[0]
                  .extract<int (E::*)() const>() == &E::f);
static_assert(mb::reflect<^^takes>.extract<void (*)(int)>() == &takes);
static_assert(!mb::reflect<^^gv>.try_extract<int &>().has_value() &&
              &mb::reflect<^^gv>.get_object().extract<int &>() == &gv);
static_assert(&mb::reflect<^^E::sv>.get_object().extract<int &>() == &E::sv);
static_assert(mb::reflect<^^cv>.extract<int>() == 3);
// structured bindings, subobjects, parameters, source locations, operators
static_assert(mb::reflect<^^s1>.get_identifier() == "s1");
static_assert(mb::reflect<^^B>.get_subobjects(ctx).size() == 2);
static_assert(mb::reflect<^^takes>.get_parameters()[0].get_identifier() == "named");
static_assert(mb::reflect<^^A>.get_source_location().line() + 1 ==
              mb::reflect<^^B>.get_source_location().line());
constexpr auto assign =
    mb::reflect<^^B>.get_members<mb::member_function>(ctx,
        mb::predicate::of_operator(std::meta::op_equals))[0];
static_assert(assign.is_assignment() && assign.get_operator_symbol() == "=" && assign.is_copy_assignment());
static_assert(mb::reflect<^^operator""_lit>.is_literal_operator());
static_assert(assign.get_u8operator_symbol() == u8"=");
static_assert(mb::make_data_member_spec(tint, {.name = "n", .bit_width = 3}).get_bit_size() == 3);
static_assert(mb::make_data_member_spec(tint, {.name = "n"}).get_identifier() == "n" &&
              mb::make_data_member_spec(tint, {.name = "n"}).get_type() == tint);
// current context
consteval bool in_function() {
    return mb::get_current_function().info == mb::reflect_function(in_function).info
        && mb::get_current_namespace() == mb::global_namespace
        && !mb::try_get_current_class().has_value();
}
static_assert(in_function());
namespace N {
    struct K {
        static consteval bool check() {
            return mb::get_current_class() == mb::reflect<^^K> &&
                   mb::get_current_namespace() == mb::reflect<^^N>;
        }
    };
}
static_assert(N::K::check());
static_assert(!mb::try_get_current_function().has_value());
// code generation
struct generated;
consteval {
    mb::define_aggregate(mb::reflect<^^generated>,
                         {mb::make_data_member_spec(tint, {.name = "first"}),
                          mb::make_data_member_spec(mb::reflect<^^double>, {.name = "second"})});
}
static_assert(mb::make_reflection<mb::class_type>(^^generated).get_members<mb::data_member>(ctx).size() == 2 &&
              generated{1, 2.0}.first == 1);
TEST_CASE("misc: compile-time checks") {
    SUCCEED("all checks are static_asserts");
}
