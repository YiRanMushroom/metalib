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
constexpr mb::class_type cs = mb::reflect<^^S>.into<mb::class_type>();

consteval bool same(auto const &handles, std::vector<info> const &expected) {
    if (handles.size() != expected.size()) return false;
    for (std::size_t i = 0; i < expected.size(); ++i) if (handles[i].info != expected[i]) return false;
    return true;
}
consteval std::vector<info> std_filtered(bool (*pred)(info)) {
    std::vector<info> out;
    for (info m : members_of(^^S, unchecked)) if (pred(m)) out.push_back(m);
    return out;
}

// each query returns exactly what the corresponding std::meta call returns, in the same order
static_assert(same(cs.get_members<mb::data_member>(unchecked), nonstatic_data_members_of(^^S, unchecked)));
static_assert(cs.get_members<mb::data_member>(unchecked).size() == 4);
static_assert(same(cs.get_members<mb::static_data_member>(unchecked), static_data_members_of(^^S, unchecked)));
static_assert(same(cs.get_members(unchecked), members_of(^^S, unchecked)));
static_assert(same(cs.get_members<mb::member_function>(unchecked), std_filtered(mb::member_function::holds)));
static_assert(same(cs.get_members<mb::static_member_function>(unchecked),
                  std_filtered(mb::static_member_function::holds)));
static_assert(same(cs.get_members<mb::constructor>(unchecked), std_filtered(is_constructor)));
static_assert(same(cs.get_bases(unchecked), bases_of(^^S, unchecked)));
static_assert(cs.get_members<mb::static_member_function>(unchecked).size() == 2);

// access context defaults to the caller
static_assert(cs.get_members<mb::data_member>().size() == 3);
consteval std::size_t from_friend() { return cs.get_members<mb::data_member>().size(); }
static_assert(from_friend() == 4);
static_assert(cs.has_inaccessible_nonstatic_data_members() && !cs.has_inaccessible_nonstatic_data_members(unchecked));

// aliases name the same class
static_assert(mb::reflect<^^SA>.into<mb::class_type>().get_members<mb::data_member>(unchecked).size() == 4);
static_assert(!mb::reflect<^^int>.try_into<mb::class_type>().has_value());

// predicates and transforms
using namespace mb::predicate;
static_assert(cs.get_members<mb::member_function>(unchecked, of_name("f")).size() == 1);
static_assert(cs.get_members<mb::member_function>(unchecked, of_name("f") || of_operator(op_plus)).size() == 2);
static_assert(cs.get_members<mb::member_function>(unchecked,
                  of_name("f").or_(of_operator(op_plus)).and_(
                      mb::predicate::from([](mb::member_function f) { return !f.is_const(); }))).size() == 1);
static_assert(cs.get_members<mb::data_member>(unchecked, of_type(mb::reflect<^^int>)).size() == 2);
static_assert(cs.get_members<mb::data_member>(unchecked, of_type<int>() || of_type<double>()).size() == 3);
static_assert(cs.get_members<mb::member_function>(unchecked,
                  mb::predicate::from(&mb::member_function::has_explicit_object_parameter)).size() == 1);
static_assert(cs.get_members<mb::member_function>(unchecked,
                  mb::predicate::from(&mb::member_function::is_const).not_().and_(
                      mb::predicate::from(&mb::member_function::is_special_member).not_())).size() == 2);
static_assert(cs.get_members<mb::data_member>(unchecked, of_type<int>(),
                  mb::transform::from(&mb::data_member::get_bit_size).and_then([](std::size_t n) { return n + 1; }))
                  .size() == 2);
static_assert(cs.get_members<mb::data_member>(unchecked, !of_type(mb::reflect<^^int>)).size() == 2);
constexpr mb::array_literal int_names =
    cs.get_members<mb::data_member>(unchecked, of_type(mb::reflect<^^int>),
    mb::transform::from([](auto const &x) { return x.get_identifier(); }));
static_assert(int_names[0] == "a" && int_names[1] == "p");
constexpr mb::array_literal type_names = cs.get_members<mb::data_member>(unchecked, {},
    mb::transform::from([](mb::data_member d) { return d.get_type(); })
        .and_then([](mb::type t) { return t.get_display_string(); }));
static_assert(type_names[0] == "int" && type_names[2] == "double");
static_assert(cs.get_members<mb::static_member_function>(unchecked, of_operator(op_parentheses)).size() == 1);

// nested types
constexpr mb::array_literal nts = cs.get_members<mb::nested_type>(unchecked);
static_assert(nts.size() == 3 && nts[0].get_identifier() == "nested" && nts[2].is_alias() && !nts[0].is_alias());
static_assert(nts[2] == mb::reflect<^^int> && nts[2].get_parent().info == cs.info &&
              nts[1].try_into<mb::enum_type>().has_value());
static_assert(cs.get_members<mb::nested_type>(unchecked, [](mb::nested_type t) { return !t.is_alias(); }).size() == 2);

// unions and anonymous unions
union U { int i; float f; U() {} ~U() {} };
static_assert(!mb::reflect<^^U>.try_into<mb::class_type>().has_value() &&
              !mb::reflect<^^S>.try_into<mb::union_type>().has_value());
constexpr mb::union_type uu = mb::reflect<^^U>.into<mb::union_type>();
static_assert(uu.get_members<mb::data_member>(unchecked).size() == 2 && uu.try_get_destructor(unchecked).has_value());
static_assert(uu.get_members<mb::data_member>(unchecked)[1].get_offset().bytes == 0);
struct A { union { int x; float y; }; int after; };
constexpr mb::class_type ca = mb::reflect<^^A>.into<mb::class_type>();
constexpr auto anon = ca.get_members<mb::data_member>(unchecked)[0];
static_assert(!anon.has_u8identifier() && ca.get_members<mb::data_member>(unchecked).size() == 2);
static_assert(anon.get_type().into<mb::union_type>().get_members<mb::data_member>(unchecked, {},
    mb::transform::from([](auto const &x) { return x.get_identifier(); }))[1] == "y");
static_assert(ca.get_members<mb::nested_type>(unchecked).size() == 1 &&
              !ca.get_members<mb::nested_type>(unchecked)[0].has_u8identifier());
static_assert(anon.get_parent().info == ca.info &&
              ca.get_members<mb::nested_type>(unchecked)[0].get_parent().info == ca.info);

// filter and map
static_assert(cs.get_members<mb::data_member>(unchecked,
                  [](mb::data_member d) { return d.is_bit_field(); }).size() == 1);
constexpr mb::array_literal names = cs.get_members<mb::data_member>(unchecked, {},
    [](mb::data_member d) { return d.get_identifier(); });
static_assert(names.size() == 4 && names[0] == "a" && names[1] == "bits" && names[3] == "p");

// destructor and bases
static_assert(cs.get_destructor(unchecked).is_override() && cs.get_destructor(unchecked).is_virtual());
constexpr mb::array_literal bs = cs.get_bases(unchecked);
static_assert(bs[0].get_type() == mb::reflect<^^B1> && !bs[0].is_virtual() && bs[0].is_public());
static_assert(bs[1].get_type() == mb::reflect<^^B2> && bs[1].is_virtual() && bs[1].is_private());
static_assert(bs[0].get_parent().info == ^^S);

// data members
constexpr mb::array_literal dms = cs.get_members<mb::data_member>(unchecked);
static_assert(dms[1].get_bit_size() == 5 && dms[2].is_mutable() && dms[2].has_default_member_initializer());
static_assert(dms[0].get_parent().info == cs.info && dms[0].get_type() == mb::reflect<^^int> &&
              dms[3].is_private());

// member functions
consteval mb::member_function fn(std::string_view name) {
    return cs.get_members<mb::member_function>(unchecked, mb::predicate::of_name(name))[0];
}
static_assert(fn("h").has_explicit_object_parameter() && !fn("f").has_explicit_object_parameter());
static_assert(fn("f").is_lvalue_reference_qualified() && fn("v").is_virtual() && fn("v").is_const());
constexpr auto plus = cs.get_members<mb::member_function>(unchecked, mb::predicate::of_operator(op_plus))[0];
static_assert(plus.get_operator_kind() == op_plus && plus.get_return_type() == mb::reflect<^^int>);
static_assert(!fn("f").try_get_operator_kind().has_value());
constexpr mb::array_literal conv =
    cs.get_members<mb::member_function>(unchecked, [](mb::member_function f) { return f.is_conversion(); });
static_assert(conv.size() == 1 && conv[0].is_explicit() && conv[0].get_return_type() == mb::reflect<^^bool>);
static_assert(cs.get_members<mb::static_member_function>(unchecked,
                  [](auto f) { return f.is_operator(); }).size() == 1);

// constructors and parameters
constexpr auto two =
    cs.get_members<mb::constructor>(unchecked, [](mb::constructor c) { return c.get_parameters().size() == 2; })[0];
static_assert(two.is_explicit() && two.get_parameters()[0].get_identifier() == "x");
static_assert(!two.get_parameters()[0].has_default_argument() && two.get_parameters()[1].has_default_argument());
static_assert(two.get_parameters()[1].get_type() == mb::reflect<^^int>);

// checked narrowing with try_make_reflection
static_assert(mb::try_make_reflection<mb::data_member>(^^S::a).has_value() &&
              !mb::try_make_reflection<mb::static_data_member>(^^S::a));
static_assert(mb::try_make_reflection<mb::static_data_member>(^^S::sd).has_value());
static_assert(mb::try_make_reflection<mb::static_member_function>(^^S::g).has_value() &&
              !mb::try_make_reflection<mb::member_function>(^^S::g));
static_assert(mb::try_make_reflection<mb::member_function>(^^S::f).has_value() &&
              mb::try_make_reflection<mb::member>(^^S::f).has_value());

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
consteval std::size_t converted_size() {
    std::vector<std::string_view> views{"a", "bc"};
    mb::array_literal<mb::string_literal> converted = views;
    return converted[1].size();
}
static_assert(converted_size() == 2);
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
