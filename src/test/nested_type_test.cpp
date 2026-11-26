#include <catch2/catch_test_macros.hpp>
#include <initializer_list>
#include <metalib/metalib.hpp>
#include <string_view>
namespace mb = metalib;
struct S {
    struct defined { int q; };
    struct declared_only;
    enum class color { red };
    typedef int td;
    using alias = defined;
    union u { int i; float f; };
    union { int x; float y; };
private:
    struct hidden {};
};
constexpr auto ctx = std::meta::access_context::unchecked();
constexpr mb::class_type cs = mb::reflect<mb::type>(^^S).into<mb::class_type>();
consteval bool names(auto const &list, std::initializer_list<std::string_view> expected) {
    if (list.size() != expected.size()) return false;
    std::size_t i = 0;
    for (std::string_view e : expected) {
        auto k = list[i++];
        if (e.empty() ? k.has_u8identifier() : (!k.has_identifier() || k.get_identifier() != e))
            return false;
    }
    return true;
}
// every nested type, in declaration order (the unnamed one is the anonymous union's type)
static_assert(names(cs.get_members<mb::nested_type>(ctx),
                  {"defined", "declared_only", "color", "td", "alias", "u", "", "hidden"}));
static_assert(names(cs.get_members<mb::nested_class>(ctx), {"defined", "hidden"}));
static_assert(names(cs.get_members<mb::nested_union>(ctx), {"u", ""}));
static_assert(names(cs.get_members<mb::nested_enum>(ctx), {"color"}));
static_assert(names(cs.get_members<mb::nested_alias>(ctx), {"td", "alias"}));
// results keep their parent and their kind abilities
constexpr mb::array_literal classes = cs.get_members<mb::nested_class>(ctx);
static_assert(classes[0].get_parent().info == cs.info && classes[1].is_private() &&
              classes[0].get_members<mb::data_member>(ctx).size() == 1);
static_assert(cs.get_members<mb::nested_enum>(ctx)[0].get_enumerators().size() == 1);
static_assert(cs.get_members<mb::nested_alias>(ctx)[1] == mb::reflect<mb::type>(^^S::defined));
// checked narrowing
static_assert(mb::try_reflect<mb::nested_class>(^^S::defined).has_value());
static_assert(!mb::try_reflect<mb::nested_class>(^^S::alias).has_value());
static_assert(mb::nested_class::try_from(mb::handle{^^S::alias}).error().u8message().ends_with(
                  u8"' is not the type's own declaration"));
static_assert(!mb::try_reflect<mb::nested_class>(^^S::declared_only).has_value() &&
              mb::try_reflect<mb::nested_type>(^^S::declared_only).has_value());
static_assert(!mb::try_reflect<mb::nested_union>(^^S::defined).has_value());
TEST_CASE("nested type: compile-time checks") {
    SUCCEED("all checks are static_asserts");
}
