#include <catch2/catch_test_macros.hpp>
#include <metalib/metalib.hpp>
#include <string>
#include <span>
#include <string_view>
#include <vector>
namespace mb = metalib;
using namespace std::meta;

struct B1 { int b1; };
struct B2 { virtual ~B2() = default; };
struct S : B1, private virtual B2 {
    int a;
    int : 3;
    unsigned bits : 5;
    static int sd;
    static constexpr int sc = 7;
    mutable double m = 1.0;
    S() {}
    explicit S(int x, int y = 2) {}
    ~S() override {}
    void f() & {}
    static void g() {}
    void h(this S const &) {}
    template<typename T> void t(T) {}
    int operator+(int) const { return 0; }
    static int operator()(int) { return 1; }
    explicit operator bool() const { return true; }
    virtual void v() const {}
    struct nested {};
    enum class color { red };
    using alias = int;
    friend consteval std::size_t from_friend();
private:
    int p;
};
using SA = S;
constexpr access_context unchecked = access_context::unchecked();
constexpr mb::class_type cs = mb::type{^^S}.try_into<mb::class_type>().unwrap();

template<typename L>
consteval bool same(L lit, std::vector<info> expected) {
    if (lit.size() != expected.size()) return false;
    for (std::size_t i = 0; i < expected.size(); ++i) if (lit[i].info != expected[i]) return false;
    return true;
}
consteval std::vector<info> std_filtered(bool (*pred)(info)) {
    std::vector<info> out;
    for (info m : members_of(^^S, unchecked)) if (pred(m)) out.push_back(m);
    return out;
}

// each query returns exactly what the corresponding std::meta call returns, in the same order
static_assert(same(cs.members<mb::data_member>(unchecked), nonstatic_data_members_of(^^S, unchecked)));
static_assert(cs.members<mb::data_member>(unchecked).size() == 4);
static_assert(same(cs.members<mb::static_data_member>(unchecked), static_data_members_of(^^S, unchecked)));
static_assert(same(cs.members(unchecked), members_of(^^S, unchecked)));
static_assert(same(cs.members<mb::member_function>(unchecked), std_filtered(mb::member_function::holds)));
static_assert(same(cs.members<mb::static_member_function>(unchecked), std_filtered(mb::static_member_function::holds)));
static_assert(same(cs.members<mb::constructor>(unchecked), std_filtered(is_constructor)));
static_assert(same(cs.bases(unchecked), bases_of(^^S, unchecked)));
static_assert(cs.members<mb::static_member_function>(unchecked).size() == 2);

// access context defaults to the caller
static_assert(cs.members<mb::data_member>().size() == 3);
consteval std::size_t from_friend() { return cs.members<mb::data_member>().size(); }
static_assert(from_friend() == 4);
static_assert(cs.has_inaccessible_nonstatic_data_members() && !cs.has_inaccessible_nonstatic_data_members(unchecked));

// aliases name the same class
static_assert(mb::type{^^SA}.try_into<mb::class_type>().unwrap().members<mb::data_member>(unchecked).size() == 4);
static_assert(!mb::type{^^int}.try_into<mb::class_type>().has_value());

// predicates and transforms
using namespace mb::predicate;
static_assert(cs.members<mb::member_function>(unchecked, of_name("f")).size() == 1);
static_assert(cs.members<mb::member_function>(unchecked, of_name("f") || of_operator(op_plus)).size() == 2);
static_assert(cs.members<mb::member_function>(unchecked,
    of_name("f").or_(of_operator(op_plus)).and_(mb::predicate::from([](mb::member_function f) { return !f.is_const(); }))).size() == 1);
static_assert(cs.members<mb::data_member>(unchecked, of_type(mb::type{^^int})).size() == 2);
static_assert(cs.members<mb::data_member>(unchecked, of_type<int>() || of_type<double>()).size() == 3);
static_assert(cs.members<mb::member_function>(unchecked, mb::predicate::from(&mb::member_function::has_explicit_object_parameter)).size() == 1);
static_assert(cs.members<mb::member_function>(unchecked, mb::predicate::from(&mb::member_function::is_const).not_()).size() >= 2);
static_assert(cs.members<mb::data_member>(unchecked, of_type<int>(), mb::transform::from(&mb::data_member::bit_size).and_then([](std::size_t n) { return n + 1; })).size() == 2);
static_assert(cs.members<mb::data_member>(unchecked, !of_type(mb::type{^^int})).size() == 2);
constexpr auto int_names = cs.members<mb::data_member>(unchecked, of_type(mb::type{^^int}), mb::transform::from([](auto const &x) { return x.ascii_identifier().unwrap(); }));
static_assert(int_names[0] == "a" && int_names[1] == "p");
constexpr auto type_names = cs.members<mb::data_member>(unchecked, {},
    mb::transform::from([](mb::data_member d) { return d.type(); }).and_then([](mb::type t) { return t.ascii_display_string().unwrap(); }));
static_assert(type_names[0] == "int" && type_names[2] == "double");
static_assert(cs.members<mb::static_member_function>(unchecked, of_operator(op_parentheses)).size() == 1);

// nested types
constexpr auto nts = cs.members<mb::nested_type>(unchecked);
static_assert(nts.size() == 3 && nts[0].ascii_identifier().unwrap() == "nested" && nts[2].is_alias() && !nts[0].is_alias());
static_assert(nts[2] == mb::type{^^int} && nts[2].parent().unwrap().info == cs.info && nts[1].try_into<mb::enum_type>().has_value());
static_assert(cs.members<mb::nested_type>(unchecked, [](mb::nested_type t) { return !t.is_alias(); }).size() == 2);

// unions and anonymous unions
union U { int i; float f; U() {} ~U() {} };
static_assert(!mb::type{^^U}.try_into<mb::class_type>().has_value() && !mb::type{^^S}.try_into<mb::union_type>().has_value());
constexpr mb::union_type uu = mb::type{^^U}.try_into<mb::union_type>().unwrap();
static_assert(uu.members<mb::data_member>(unchecked).size() == 2 && uu.destructor(unchecked).has_value());
static_assert(uu.members<mb::data_member>(unchecked)[1].offset().bytes == 0);
struct A { union { int x; float y; }; int after; };
constexpr mb::class_type ca = mb::type{^^A}.try_into<mb::class_type>().unwrap();
constexpr auto anon = ca.members<mb::data_member>(unchecked)[0];
static_assert(!anon.has_u8_identifier() && ca.members<mb::data_member>(unchecked).size() == 2);
static_assert(anon.type().try_into<mb::union_type>().unwrap().members<mb::data_member>(unchecked, {}, mb::transform::from([](auto const &x) { return x.ascii_identifier().unwrap(); }))[1] == "y");
static_assert(ca.members<mb::nested_type>(unchecked).size() == 1 && !ca.members<mb::nested_type>(unchecked)[0].has_u8_identifier());
static_assert(anon.parent().unwrap().info == ca.info && ca.members<mb::nested_type>(unchecked)[0].parent().unwrap().info == ca.info);

// filter and map
static_assert(cs.members<mb::data_member>(unchecked, [](mb::data_member d) { return d.is_bit_field(); }).size() == 1);
constexpr auto names = cs.members<mb::data_member>(unchecked, {}, [](mb::data_member d) { return d.ascii_identifier().unwrap(); });
static_assert(names.size() == 4 && names[0] == "a" && names[1] == "bits" && names[3] == "p");

// destructor and bases
static_assert(cs.destructor(unchecked).unwrap().is_override() && cs.destructor(unchecked).unwrap().is_virtual());
constexpr auto bs = cs.bases(unchecked);
static_assert(bs[0].type() == mb::type{^^B1} && !bs[0].is_virtual() && bs[0].is_public());
static_assert(bs[1].type() == mb::type{^^B2} && bs[1].is_virtual() && bs[1].is_private());
static_assert(bs[0].parent().unwrap().info == ^^S);

// data members
constexpr auto dms = cs.members<mb::data_member>(unchecked);
static_assert(dms[1].bit_size() == 5 && dms[2].is_mutable() && dms[2].has_default_member_initializer());
static_assert(dms[0].parent().unwrap().info == cs.info && dms[0].type() == mb::type{^^int} && dms[3].is_private());

// member functions
consteval mb::member_function fn(std::string_view name) {
    for (auto f : cs.members<mb::member_function>(unchecked))
        if (f.has_u8_identifier() && f.ascii_identifier().unwrap() == name) return f;
    return {};
}
static_assert(fn("h").has_explicit_object_parameter() && !fn("f").has_explicit_object_parameter());
static_assert(fn("f").is_lvalue_reference_qualified() && fn("v").is_virtual() && fn("v").is_const());
constexpr auto plus = cs.members<mb::member_function>(unchecked, [](mb::member_function f) { return f.is_operator(); })[0];
static_assert(plus.operator_kind().unwrap() == op_plus && plus.return_type() == mb::type{^^int});
static_assert(!fn("f").operator_kind().has_value());
constexpr auto conv = cs.members<mb::member_function>(unchecked, [](mb::member_function f) { return f.is_conversion(); });
static_assert(conv.size() == 1 && conv[0].is_explicit() && conv[0].return_type() == mb::type{^^bool});
static_assert(cs.members<mb::static_member_function>(unchecked, [](auto f) { return f.is_operator(); }).size() == 1);

// constructors and parameters
constexpr auto two = cs.members<mb::constructor>(unchecked, [](mb::constructor c) { return c.parameters().size() == 2; })[0];
static_assert(two.is_explicit() && two.parameters()[0].ascii_identifier().unwrap() == "x");
static_assert(!two.parameters()[0].has_default_argument() && two.parameters()[1].has_default_argument());
static_assert(two.parameters()[1].type() == mb::type{^^int});

// narrowing by try_into
static_assert(mb::handle{^^S::a}.try_into<mb::data_member>().has_value() && !mb::handle{^^S::a}.try_into<mb::static_data_member>());
static_assert(mb::handle{^^S::sd}.try_into<mb::static_data_member>().has_value());
static_assert(mb::handle{^^S::g}.try_into<mb::static_member_function>().has_value() && !mb::handle{^^S::g}.try_into<mb::member_function>());
static_assert(mb::handle{^^S::f}.try_into<mb::member_function>().has_value() && mb::handle{^^S::f}.try_into<mb::member>().has_value());

// literals
constexpr mb::string_literal sl = "abc";
static_assert(sl == "abc" && std::string_view(sl).size() == 3 && sl.c_str()[3] == 0 && sl.size() == 3);
constexpr mb::string_literal none;
static_assert(none.empty() && none.c_str()[0] == 0);
template<mb::string_literal L> constexpr std::size_t literal_size = L.size();
static_assert(literal_size<"hello"> == 5);
constexpr mb::array_literal<int> al = std::vector{1, 2, 3};
static_assert(al.size() == 3 && al[2] == 3 && std::span<int const>(al).size() == 3);
template<mb::array_literal<int> A> constexpr int first = A[0];
static_assert(first<al> == 1);
constexpr mb::u8string_literal u8 = u8"ü";
static_assert(u8.size() == 2);

TEST_CASE("class queries: results usable at runtime") {
    std::string joined;
    template for (constexpr mb::string_literal n : names) {
        joined += n.c_str();
        joined += ' ';
    }
    REQUIRE(joined == "a bits m p ");
    std::string_view s = names[1];
    REQUIRE(s.size() == 4);
}
