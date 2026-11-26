#include <catch2/catch_test_macros.hpp>
#include <metalib/metalib.hpp>
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
constexpr mb::type tint = mb::reflect<mb::type>(^^int),
                   tpair = mb::reflect<mb::type>(^^std::pair<int, char>),
                   tvar = mb::reflect<mb::type>(^^std::variant<int, A>);
static_assert(tpair.get_tuple_size() == 2 && !tint.try_get_tuple_size().has_value());
static_assert(tpair.get_tuple_element(1) == mb::reflect<mb::type>(^^char) &&
              !tpair.try_get_tuple_element(2).has_value());
static_assert(tvar.get_variant_size() == 2 && tvar.get_variant_alternative(1) == mb::reflect<mb::type>(^^A) &&
              !tint.try_get_variant_size().has_value());
static_assert(tint.find_common_type_with({mb::reflect<mb::type>(^^long)}) == mb::reflect<mb::type>(^^long));
static_assert(!tint.try_find_common_type_with({mb::reflect<mb::type>(^^A)}).has_value());
static_assert(tint.try_find_common_type_with({mb::reflect<mb::type>(^^A)}).error().u8message() ==
              u8"unable to find common_type_with(" + std::u8string(std::meta::u8display_string_of(^^A)) + u8") from '" +
                  std::u8string(std::meta::u8display_string_of(^^int)) + u8"'");
static_assert(mb::reflect<mb::type>(^^int &).find_common_reference_with({mb::reflect<mb::type>(^^int const &)}) ==
              mb::reflect<mb::type>(^^int const &));
static_assert(mb::reflect<mb::type>(^^int (*)(char)).find_invoke_result_with({mb::reflect<mb::type>(^^char)}) == tint);
static_assert(!mb::reflect<mb::type>(^^int (*)(char))
                  .try_find_invoke_result_with({mb::reflect<mb::type>(^^A)}).has_value());
static_assert(mb::reflect<mb::type>(^^int const &).reference_constructs_from_temporary(mb::reflect<mb::type>(^^long)));
// annotations, values, objects
constexpr mb::array_literal anns = mb::reflect<mb::type>(^^A).get_annotations();
static_assert(anns.size() == 2 && anns[0].extract<int>() == 1 && anns[1].get_type() == mb::reflect<mb::type>(^^double));
static_assert(anns[0].get_constant().into<mb::value>().extract<int>() == 1);
static_assert(mb::try_reflect<mb::annotation>(anns[0].info).has_value() &&
              !mb::try_reflect<mb::annotation>(^^A).has_value());
static_assert(mb::reflect_constant(42).into<mb::value>().extract<int>() == 42);
static_assert(mb::reflect_object(gv).get_type() == tint);
static_assert(mb::reflect_function(takes).try_into<mb::function>().has_value());
static_assert(mb::reflect<mb::variable>(^^gv).get_object() == mb::reflect_object(gv));
// extract: member and function pointers directly, references to variables through get_object()
struct E { int x = 1; static int sv; int f() const { return 3; } };
int E::sv = 7;
constexpr auto ex = mb::reflect<mb::class_type>(^^E).get_members<mb::data_member>(ctx)[0];
static_assert(ex.extract<int E::*>() == &E::x && !ex.try_extract<long E::*>().has_value());
consteval std::u8string expected_extract_action() {
    return u8"extract<" + std::u8string(std::meta::u8display_string_of(^^long E::*)) + u8">()";
}
static_assert(ex.try_extract<long E::*>().error().u8action() == expected_extract_action());
static_assert(ex.try_extract<long E::*>().error().u8message() ==
              u8"unable to " + expected_extract_action() + u8" from '" +
                  std::u8string(std::meta::u8display_string_of(ex.info)) + u8"' of type '" +
                  std::u8string(std::meta::u8display_string_of(^^int)) + u8"'");
static_assert(mb::reflect<mb::class_type>(^^E).get_members<mb::member_function>(ctx, mb::predicate::of_name("f"))[0]
                  .extract<int (E::*)() const>() == &E::f);
static_assert(mb::reflect<mb::function>(^^takes).extract<void (*)(int)>() == &takes);
static_assert(!mb::reflect<mb::variable>(^^gv).try_extract<int &>().has_value() &&
              &mb::reflect<mb::variable>(^^gv).get_object().extract<int &>() == &gv);
static_assert(&mb::reflect<mb::static_data_member>(^^E::sv).get_object().extract<int &>() == &E::sv);
static_assert(mb::reflect<mb::variable>(^^cv).extract<int>() == 3);
// structured bindings, subobjects, parameters, source locations, operators
static_assert(mb::reflect<mb::structured_binding>(^^s1).get_identifier() == "s1");
static_assert(mb::reflect<mb::class_type>(^^B).get_subobjects(ctx).size() == 2);
static_assert(mb::reflect<mb::function>(^^takes).get_parameters()[0].get_identifier() == "named");
static_assert(mb::reflect<mb::type>(^^A).get_source_location().line() + 1 ==
              mb::reflect<mb::type>(^^B).get_source_location().line());
constexpr auto assign =
    mb::reflect<mb::class_type>(^^B).get_members<mb::member_function>(ctx,
        mb::predicate::of_operator(std::meta::op_equals))[0];
static_assert(assign.is_assignment() && assign.get_operator_symbol() == "=" && assign.is_copy_assignment());
static_assert(mb::reflect<mb::function>(^^operator""_lit).is_literal_operator());
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
            return mb::get_current_class() == mb::reflect<mb::type>(^^K) &&
                   mb::get_current_namespace() == mb::reflect<mb::namespace_>(^^N);
        }
    };
}
static_assert(N::K::check());
static_assert(!mb::try_get_current_function().has_value());
// code generation
struct generated;
consteval {
    mb::define_aggregate(mb::reflect<mb::type>(^^generated),
                         {mb::make_data_member_spec(tint, {.name = "first"}),
                          mb::make_data_member_spec(mb::reflect<mb::type>(^^double), {.name = "second"})});
}
static_assert(mb::reflect<mb::class_type>(^^generated).get_members<mb::data_member>(ctx).size() == 2 &&
              generated{1, 2.0}.first == 1);
TEST_CASE("misc: compile-time checks") {
    SUCCEED("all checks are static_asserts");
}
