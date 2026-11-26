#include <catch2/catch_test_macros.hpp>
#include <metalib/metalib.hpp>
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
constexpr mb::class_type cs = mb::type{^^S}.try_into<mb::class_type>().unwrap();
template<typename K> consteval bool names(auto list, std::initializer_list<std::string_view> expected) {
    if (list.size() != expected.size()) return false;
    std::size_t i = 0;
    for (std::string_view e : expected) {
        auto k = list[i++];
        if (e.empty() ? k.has_u8_identifier() : (!k.has_ascii_identifier() || k.ascii_identifier().unwrap() != e)) return false;
    }
    return true;
}
// every nested type, in declaration order (the unnamed one is the anonymous union's type)
static_assert(names<mb::nested_type>(cs.members<mb::nested_type>(ctx), {"defined", "declared_only", "color", "td", "alias", "u", "", "hidden"}));
static_assert(names<mb::nested_class>(cs.members<mb::nested_class>(ctx), {"defined", "hidden"}));
static_assert(names<mb::nested_union>(cs.members<mb::nested_union>(ctx), {"u", ""}));
static_assert(names<mb::nested_enum>(cs.members<mb::nested_enum>(ctx), {"color"}));
static_assert(names<mb::nested_alias>(cs.members<mb::nested_alias>(ctx), {"td", "alias"}));
// results keep their parent and their kind abilities
constexpr mb::array_literal classes = cs.members<mb::nested_class>(ctx);
static_assert(classes[0].parent().unwrap().info == cs.info && classes[1].is_private() && classes[0].members<mb::data_member>(ctx).size() == 1);
static_assert(cs.members<mb::nested_enum>(ctx)[0].enumerators().size() == 1);
static_assert(cs.members<mb::nested_alias>(ctx)[1] == mb::type{^^S::defined});
// checked narrowing
static_assert(mb::handle{^^S::defined}.try_into<mb::nested_class>().has_value());
static_assert(!mb::handle{^^S::alias}.try_into<mb::nested_class>().has_value());
static_assert(mb::nested_class::from(mb::handle{^^S::alias}).error().u8what().ends_with(u8"' is not a declaration that is not a type alias"));
static_assert(!mb::handle{^^S::declared_only}.try_into<mb::nested_class>().has_value() && mb::handle{^^S::declared_only}.try_into<mb::nested_type>().has_value());
static_assert(!mb::handle{^^S::defined}.try_into<mb::nested_union>().has_value());
TEST_CASE("nested type: compile-time checks") {
    SUCCEED("all checks are static_asserts");
}
