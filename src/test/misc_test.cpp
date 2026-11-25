#include <catch2/catch_test_macros.hpp>
#include <metalib/metalib.hpp>
#include <tuple>
#include <utility>
#include <variant>
namespace mb = metalib;
namespace ab = mb::ability;
struct [[=1, =2.5]] A { int x; };
struct B : A { double y; B &operator=(B const &) = default; };
struct P { int a; double b; };
auto [s1, s2] = P{1, 2.0};
constexpr int cv = 3;
int gv;
thread_local int tv;
int operator""_lit(unsigned long long) { return 0; }
void takes(int named);
constexpr auto ctx = std::meta::access_context::unchecked();
// type traits and transforms
constexpr mb::type tint{^^int}, tpair{^^std::pair<int, char>}, tvar{^^std::variant<int, A>};
static_assert(tpair.tuple_size().unwrap() == 2 && !tint.tuple_size().has_value());
static_assert(tpair.tuple_element(1).unwrap() == mb::type{^^char} && !tpair.tuple_element(2).has_value());
static_assert(tvar.variant_size().unwrap() == 2 && tvar.variant_alternative(1).unwrap() == mb::type{^^A} && !tint.variant_size().has_value());
static_assert(tint.common_type_with({mb::type{^^long}}).unwrap() == mb::type{^^long});
static_assert(!tint.common_type_with({mb::type{^^A}}).has_value());
static_assert(tint.common_type_with({mb::type{^^A}}).error().u8what().size() > 0);
static_assert(mb::type{^^int &}.common_reference_with({mb::type{^^int const &}}).unwrap() == mb::type{^^int const &});
static_assert(mb::type{^^int (*)(char)}.invoke_result_with({mb::type{^^char}}).unwrap() == tint);
static_assert(!mb::type{^^int (*)(char)}.invoke_result_with({mb::type{^^A}}).has_value());
static_assert(mb::type{^^int const &}.reference_constructs_from_temporary(mb::type{^^long}));
// annotations, values, objects
constexpr auto anns = mb::type{^^A}.annotations();
static_assert(anns.size() == 2 && anns[0].extract<int>().unwrap() == 1 && anns[1].type() == mb::type{^^double});
static_assert(anns[0].constant_of().try_into<mb::value>().unwrap().extract<int>().unwrap() == 1);
static_assert(mb::handle{anns[0].info}.try_into<mb::annotation>().has_value() && !mb::handle{^^A}.try_into<mb::annotation>().has_value());
static_assert(mb::reflect_constant(42).try_into<mb::value>().unwrap().extract<int>().unwrap() == 42);
static_assert(mb::reflect_object(gv).type() == tint);
static_assert(mb::reflect_function(takes).try_into<mb::function>().has_value());
static_assert(mb::variable{^^gv}.object().unwrap() == mb::reflect_object(gv));
// extract: member and function pointers directly, references to variables through object()
struct E { int x = 1; static int sv; int f() const { return 3; } };
int E::sv = 7;
constexpr auto ex = mb::class_type{^^E}.members<mb::data_member>(ctx)[0];
static_assert(ex.extract<int E::*>().unwrap() == &E::x && !ex.extract<long E::*>().has_value());
static_assert(mb::class_type{^^E}.members<mb::member_function>(ctx)[0].extract<int (E::*)() const>().unwrap() == &E::f);
static_assert(mb::function{^^takes}.extract<void (*)(int)>().unwrap() == &takes);
static_assert(!mb::variable{^^gv}.extract<int &>().has_value() && &mb::variable{^^gv}.object().unwrap().extract<int &>().unwrap() == &gv);
static_assert(&mb::static_data_member{^^E::sv}.object().unwrap().extract<int &>().unwrap() == &E::sv);
static_assert(mb::variable{^^cv}.extract<int>().unwrap() == 3);
// structured bindings, subobjects, parameters, source locations, operators
static_assert(mb::handle{^^s1}.try_into<mb::structured_binding>().unwrap().ascii_identifier().unwrap() == "s1");
static_assert(mb::class_type{^^B}.subobjects(ctx).size() == 2);
static_assert(mb::function{^^takes}.parameters()[0].ascii_identifier().unwrap() == "named");
static_assert(mb::type{^^A}.source_location().line() + 1 == mb::type{^^B}.source_location().line());
constexpr auto assign = mb::class_type{^^B}.members<mb::member_function>(ctx, mb::predicate::of_operator(std::meta::op_equals))[0];
static_assert(assign.is_assignment() && assign.operator_symbol().unwrap() == "=" && assign.is_copy_assignment());
static_assert(mb::function{^^operator""_lit}.is_literal_operator());
static_assert(assign.u8_operator_symbol().unwrap() == u8"=");
static_assert(mb::data_member_spec(tint, {.name = "n", .bit_width = 3}).unwrap().bit_size() == 3);
static_assert(mb::data_member_spec(tint, {.name = "n"}).unwrap().ascii_identifier().unwrap() == "n" && mb::data_member_spec(tint, {.name = "n"}).unwrap().type() == tint);
// current context
consteval bool in_function() {
    return mb::current_function().unwrap().info == mb::reflect_function(in_function).info && mb::current_namespace() == mb::global_namespace
        && !mb::current_class().has_value();
}
static_assert(in_function());
namespace N { struct K { static consteval std::meta::info where(std::meta::access_context c = std::meta::access_context::current()) { return c.scope(); } static consteval bool check() { return mb::current_class().unwrap() == mb::type{^^K} && mb::current_namespace() == mb::namespace_{^^N}; } }; }
static_assert(N::K::check());
namespace N { consteval std::meta::info scope_of_k() { return K::where(); } }
static_assert(!mb::current_function().has_value());
// code generation
struct generated;
consteval {
    mb::define_aggregate(mb::type{^^generated}, {mb::data_member_spec(tint, {.name = "first"}).unwrap(),
                                                  mb::data_member_spec(mb::type{^^double}, {.name = "second"}).unwrap()}).unwrap();
}
static_assert(mb::class_type{^^generated}.members<mb::data_member>(ctx).size() == 2 && generated{1, 2.0}.first == 1);
TEST_CASE("misc: compile-time checks") {
    SUCCEED("all checks are static_asserts");
}
