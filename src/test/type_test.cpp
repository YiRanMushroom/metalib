#include <catch2/catch_test_macros.hpp>
#include <metalib/handle.hpp>
namespace mb = metalib;
struct S { int x; };
struct D : S {};
enum class E : short { a };
struct [[=1]] A {};
using Alias = S;
template <class T> struct box {};
constexpr mb::type s{^^S}, alias{^^Alias}, i{^^int}, v{^^void}, ref{^^S&}, d{^^D}, l{^^long};
static_assert(s == alias && s.is(alias) && !(s == i));
template <class A, class B> concept eq = requires (A a, B b) { a == b; };
static_assert(!eq<mb::type, std::meta::info> && !eq<std::meta::info, mb::type>);
static_assert(s.size() == sizeof(S) && s.alignment() == alignof(S));
static_assert(s.is_class() && s.is_aggregate() && s.is_trivially_copyable() && !i.is_class());
static_assert(s.is_base_of(d) && i.is_convertible_to(l) && s.is_constructible_from({i}));
static_assert(ref.remove_cvref() == s && i.add_const().is_const());
constexpr mb::type b{^^box<int>};
static_assert(b.has_template_arguments() && b.template_().unwrap().info == ^^box && b.template_arguments().unwrap()[0].info == (^^int) && !s.has_template_arguments());
static_assert(mb::type{^^A}.annotations().size() == 1 && s.annotations().size() == 0);
static_assert(mb::type{^^A}.annotations_with_type(i).size() == 1);
constexpr auto stored = s.template_arguments();
static_assert(!stored);
TEST_CASE("type: compile-time checks") {
    SUCCEED("all checks are static_asserts");
}
