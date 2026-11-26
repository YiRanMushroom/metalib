#pragma once

#include <algorithm>
#include <compare>
#include <concepts>
#include <cstddef>
#include <functional>
#include <meta>
#include <metalib/encoding.hpp>
#include <metalib/literal.hpp>
#include <metalib/meta_result.hpp>
#include <optional>
#include <ranges>
#include <source_location>
#include <string>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

#ifndef METALIB_HAS_META_EXCEPTION
#if defined(__clang__)
#define METALIB_HAS_META_EXCEPTION 0
#else
#define METALIB_HAS_META_EXCEPTION 1
#endif
#endif

namespace metalib {
    template<typename... Abilities>
    struct basic_interface;

    namespace details {
        struct unchecked_tag {
            explicit unchecked_tag() = default;
        };

        template<typename K>
        consteval K reflect_unchecked(std::meta::info r);

        consteval bool type_less(std::meta::info a, std::meta::info b) {
#if defined(__cpp_lib_type_order) && !defined(__CLION_IDE__)
            return std::meta::type_order(a, b) < 0;
#else
            return std::meta::display_string_of(a) < std::meta::display_string_of(b);
#endif
        }

        consteval std::u8string quoted(std::meta::info r) {
            std::u8string text = u8"'";
            text += std::meta::u8display_string_of(r);
            text += u8"'";
            return text;
        }

        consteval void flatten_abilities(std::vector<std::meta::info> &out, std::meta::info ability) {
            std::meta::info t = std::meta::dealias(ability);
            if (std::meta::has_template_arguments(t)
                && std::meta::template_of(t) == ^^std::tuple) {
                for (std::meta::info inner: std::meta::template_arguments_of(t)) {
                    flatten_abilities(out, inner);
                }
            } else {
                out.push_back(t);
            }
        }

        consteval std::vector<std::meta::info> canonical_abilities(std::vector<std::meta::info> const &abilities) {
            std::vector<std::meta::info> flat;
            for (std::meta::info ability: abilities) {
                flatten_abilities(flat, ability);
            }
            std::ranges::sort(flat, type_less);
            auto [first, last] = std::ranges::unique(flat);
            flat.erase(first, last);
            return flat;
        }

        consteval std::meta::info canonical_set(std::vector<std::meta::info> const &abilities) {
            return std::meta::substitute(^^std::tuple, canonical_abilities(abilities));
        }

        consteval std::meta::info canonical_interface(std::vector<std::meta::info> const &abilities) {
            return std::meta::substitute(^^basic_interface, canonical_abilities(abilities));
        }

        consteval std::meta::info set_except(std::meta::info set, std::vector<std::meta::info> const &removed) {
            std::vector<std::meta::info> kept = canonical_abilities({set});
            std::vector<std::meta::info> const dropped = canonical_abilities(removed);
            std::erase_if(kept, [&](std::meta::info ability) { return std::ranges::contains(dropped, ability); });
            return std::meta::substitute(^^std::tuple, kept);
        }
    }

    template<typename... Abilities>
    using ability_set = typename [:details::canonical_set({^^Abilities...}):];

    template<typename Set, typename... Removed>
    using except = typename [:details::set_except(^^Set, {^^Removed...}):];

    class handle;

    namespace details {
        template<typename K>
        concept kind = requires(handle const &h) {
            { K::try_from(h) } -> std::same_as<meta_result<K>>;
        };
    }

    class handle {
    public:
        std::meta::info info{};

        consteval operator std::meta::info() const {
            return info;
        }

        template<details::kind K>
        consteval meta_result<K> try_into(this handle const &self) {
            return K::try_from(self);
        }

        template<details::kind K>
        consteval K into(this handle const &self) {
            return K::from(self);
        }
    };

    template<details::kind K>
    consteval meta_result<K> try_reflect(std::meta::info r) {
        return K::try_from(handle{r});
    }

    template<details::kind K>
    consteval K reflect(std::meta::info r) {
        return K::from(handle{r});
    }

    namespace details {
        template<typename Ability>
        consteval bool ability_holds(std::meta::info r) {
            if constexpr (requires { Ability::check(r); }) {
                return Ability::check(r);
            } else {
                return true;
            }
        }

        template<typename Ability>
        consteval void first_unmet_requirement(std::u8string &unmet, std::meta::info r) {
            if constexpr (requires { Ability::check(r); }) {
                if (unmet.empty() && !Ability::check(r)) {
                    unmet = u8"reflection ";
                    unmet += quoted(r);
                    unmet += u8" is not ";
                    unmet += Ability::requirement;
                }
            }
        }

        template<typename Ability>
        consteval void first_equality(bool &found, bool &equal, std::meta::info a, std::meta::info b) {
            if constexpr (requires { Ability::equal(a, b); }) {
                if (!found) {
                    found = true;
                    equal = Ability::equal(a, b);
                }
            }
        }

        template<typename... Abilities>
        consteval bool equal_by(std::meta::info a, std::meta::info b) {
            bool found = false;
            bool equal = false;
            (first_equality<Abilities>(found, equal, a, b), ...);
            return found ? equal : a == b;
        }
    }

    template<typename... Abilities>
    struct basic_interface : handle, Abilities... {
        using abilities = std::tuple<Abilities...>;

        template<typename... More>
        using with_ability = typename [:details::canonical_interface({^^abilities, ^^More...}):];

        template<typename... Removed>
        using without_ability = typename [:details::canonical_interface({^^except<abilities, Removed...>}):];

        basic_interface() = default;

        template<typename... Others>
            requires (std::derived_from<basic_interface<Others...>, Abilities> && ...)
        consteval basic_interface(basic_interface<Others...> const &other) : handle{other.info} {
        }

        static consteval bool holds(std::meta::info r) {
            return (details::ability_holds<Abilities>(r) && ...);
        }

        static consteval meta_result<basic_interface> try_from(handle const &h) {
            std::u8string unmet;
            (details::first_unmet_requirement<Abilities>(unmet, h.info), ...);
            if (!unmet.empty()) {
                return meta_result<basic_interface>::failure(unmet, h.info);
            }
            return basic_interface(details::unchecked_tag{}, h.info);
        }

        static consteval basic_interface from(handle const &h) {
            return try_from(h).unwrap();
        }

        template<typename... More>
        consteval meta_result<with_ability<More...>> try_attach_ability(this basic_interface const &self) {
            return self.template try_into<with_ability<More...>>();
        }

        template<typename... More>
        consteval with_ability<More...> attach_ability(this basic_interface const &self) {
            return self.template into<with_ability<More...>>();
        }

        template<typename... Others>
        friend consteval bool operator==(basic_interface const &a, basic_interface<Others...> const &b) {
            return details::equal_by<Abilities..., Others...>(a.info, b.info);
        }

        friend bool operator==(basic_interface const &, std::meta::info) = delete;

    private:
        consteval basic_interface(details::unchecked_tag, std::meta::info r) : handle{r} {
        }

        template<typename K>
        friend consteval K details::reflect_unchecked(std::meta::info r);
    };

    namespace details {
        template<typename... Abilities>
        consteval basic_interface<Abilities...> attach_ability_unchecked(handle const &h) {
            return reflect_unchecked<basic_interface<Abilities...>>(h.info);
        }
    }

    namespace details {
        template<typename K>
        consteval K reflect_unchecked(std::meta::info r) {
            if constexpr (std::same_as<K, handle>) {
                return handle{r};
            } else {
                return K(unchecked_tag{}, r);
            }
        }
    }

    namespace ability {
        struct identifiable;
        struct displayable;
        struct sized;
        struct type_traits;
        struct type_transforms;
        struct templated;
        struct annotatable;
        struct constant;
        struct typed;
        struct type_entity;
        struct arithmetic;
        struct enumeration;
        struct record;
        struct class_record;
        struct union_record;
        struct accessible;
        struct class_member;
        struct callable;
        struct signature;
        struct virtual_specifier;
        struct override_specifier;
        struct explicit_specifier;
        struct nonstatic_data;
        struct static_data;
        struct nonstatic_function;
        struct static_function;
        struct constructor_entity;
        struct destructor_entity;
        struct base_entity;
        struct enumerator_entity;
        struct parameter_entity;
        struct type_alias;
        struct non_alias;
        struct namespace_entity;
        struct namespace_alias_entity;
        struct namespace_member_entity;
        struct variable_entity;
        struct function_entity;
        struct linkage;
        struct storage_duration;
        template<typename T>
        struct typed_as;
        struct constant_entity;
        struct value_entity;
        struct object_entity;
        struct annotation_entity;
        struct structured_binding_entity;
        struct template_entity;
        struct data_member_description_entity;

        struct parented;

        using overridable = ability_set<virtual_specifier, override_specifier>;
    }

    namespace details::ability_sets {
        using type = ability_set<ability::parented, ability::identifiable, ability::displayable, ability::sized,
            ability::type_traits, ability::type_transforms, ability::templated, ability::annotatable,
            ability::type_entity>;
        using arithmetic_type = ability_set<type, ability::arithmetic>;
        using enum_type = ability_set<type, ability::enumeration>;
        using class_type = ability_set<type, ability::record, ability::class_record>;
        using union_type = ability_set<type, ability::record, ability::union_record>;
        using parameter = ability_set<ability::parented, ability::identifiable, ability::displayable,
            ability::annotatable, ability::typed, ability::parameter_entity>;
        using enumerator = ability_set<ability::identifiable, ability::displayable, ability::annotatable,
            ability::typed, ability::constant, ability::enumerator_entity, ability::parented>;
        using member = ability_set<ability::parented, ability::identifiable, ability::displayable, ability::annotatable,
            ability::accessible, ability::class_member>;
        using data_member = ability_set<member, ability::typed, ability::sized, ability::nonstatic_data,
            ability::constant>;
        using static_data_member = ability_set<member, ability::templated, ability::typed, ability::sized,
            ability::linkage, ability::storage_duration, ability::static_data, ability::constant>;
        using member_function = ability_set<member, ability::templated, ability::typed, ability::callable,
            ability::signature, ability::overridable, ability::explicit_specifier, ability::nonstatic_function,
            ability::constant>;
        using static_member_function = ability_set<member, ability::templated, ability::typed, ability::callable,
            ability::signature, ability::static_function, ability::constant>;
        using constructor = ability_set<ability::parented, ability::displayable, ability::annotatable,
            ability::accessible, ability::callable, ability::explicit_specifier, ability::constructor_entity>;
        using destructor = ability_set<ability::parented, ability::displayable, ability::annotatable,
            ability::accessible, ability::callable, ability::overridable, ability::destructor_entity>;
        using base = ability_set<ability::displayable, ability::annotatable, ability::typed, ability::accessible,
            ability::virtual_specifier, ability::base_entity, ability::parented>;
        using nested_type = ability_set<type, member>;
        using nested_class = ability_set<nested_type, class_type, ability::non_alias>;
        using nested_union = ability_set<nested_type, union_type, ability::non_alias>;
        using nested_enum = ability_set<nested_type, enum_type, ability::non_alias>;
        using nested_alias = ability_set<nested_type, ability::type_alias>;
        using namespace_ = ability_set<ability::parented, ability::identifiable, ability::displayable,
            ability::annotatable, ability::namespace_entity>;
        using namespace_alias = ability_set<ability::parented, ability::identifiable, ability::displayable,
            ability::namespace_alias_entity>;
        using namespace_member = ability_set<ability::parented, ability::identifiable, ability::displayable,
            ability::annotatable, ability::namespace_member_entity>;
        using variable = ability_set<namespace_member, ability::templated, ability::typed, ability::sized,
            ability::linkage, ability::storage_duration, ability::variable_entity, ability::constant>;
        using function = ability_set<namespace_member, ability::templated, ability::typed, ability::callable,
            ability::signature, ability::linkage, ability::function_entity, ability::constant>;
        using constant = ability_set<ability::displayable, ability::typed, ability::constant,
            ability::constant_entity>;
        using value = ability_set<ability::displayable, ability::typed, ability::constant, ability::value_entity>;
        using object = ability_set<ability::displayable, ability::typed, ability::constant, ability::object_entity>;
        using annotation = ability_set<ability::displayable, ability::typed, ability::constant,
            ability::annotation_entity>;
        using template_ = ability_set<ability::parented, ability::identifiable, ability::displayable, ability::linkage,
                                      ability::template_entity>;
        using member_template = ability_set<template_, ability::accessible, ability::class_member>;
        using data_member_description = ability_set<ability::identifiable, ability::typed,
                                                    ability::data_member_description_entity>;
        using structured_binding = ability_set<ability::parented, ability::identifiable, ability::displayable,
            ability::structured_binding_entity>;
    }

    using type = basic_interface<>::with_ability<details::ability_sets::type>;
    using arithmetic_type = basic_interface<>::with_ability<details::ability_sets::arithmetic_type>;
    using enum_type = basic_interface<>::with_ability<details::ability_sets::enum_type>;
    using class_type = basic_interface<>::with_ability<details::ability_sets::class_type>;
    using union_type = basic_interface<>::with_ability<details::ability_sets::union_type>;
    using parameter = basic_interface<>::with_ability<details::ability_sets::parameter>;
    using enumerator = basic_interface<>::with_ability<details::ability_sets::enumerator>;
    using member = basic_interface<>::with_ability<details::ability_sets::member>;
    using data_member = basic_interface<>::with_ability<details::ability_sets::data_member>;
    using static_data_member = basic_interface<>::with_ability<details::ability_sets::static_data_member>;
    using member_function = basic_interface<>::with_ability<details::ability_sets::member_function>;
    using static_member_function = basic_interface<>::with_ability<details::ability_sets::static_member_function>;
    using constructor = basic_interface<>::with_ability<details::ability_sets::constructor>;
    using destructor = basic_interface<>::with_ability<details::ability_sets::destructor>;
    using base = basic_interface<>::with_ability<details::ability_sets::base>;
    using nested_type = basic_interface<>::with_ability<details::ability_sets::nested_type>;
    using nested_class = basic_interface<>::with_ability<details::ability_sets::nested_class>;
    using nested_union = basic_interface<>::with_ability<details::ability_sets::nested_union>;
    using nested_enum = basic_interface<>::with_ability<details::ability_sets::nested_enum>;
    using nested_alias = basic_interface<>::with_ability<details::ability_sets::nested_alias>;
    using namespace_ = basic_interface<>::with_ability<details::ability_sets::namespace_>;
    using namespace_alias = basic_interface<>::with_ability<details::ability_sets::namespace_alias>;
    using namespace_member = basic_interface<>::with_ability<details::ability_sets::namespace_member>;
    using variable = basic_interface<>::with_ability<details::ability_sets::variable>;
    using function = basic_interface<>::with_ability<details::ability_sets::function>;
    using constant = basic_interface<>::with_ability<details::ability_sets::constant>;
    using value = basic_interface<>::with_ability<details::ability_sets::value>;
    using object = basic_interface<>::with_ability<details::ability_sets::object>;
    using annotation = basic_interface<>::with_ability<details::ability_sets::annotation>;

    template<typename T>
    using typed_constant =
        basic_interface<>::with_ability<except<details::ability_sets::constant, ability::constant>, ability::typed_as<T>>;

    template<typename T>
    using typed_object =
        basic_interface<>::with_ability<except<details::ability_sets::object, ability::constant>, ability::typed_as<T>>;

    template<typename T>
    using typed_annotation =
        basic_interface<>::with_ability<except<details::ability_sets::annotation, ability::constant>, ability::typed_as<T>>;
    using structured_binding = basic_interface<>::with_ability<details::ability_sets::structured_binding>;
    using template_ = basic_interface<>::with_ability<details::ability_sets::template_>;
    using data_member_description = basic_interface<>::with_ability<details::ability_sets::data_member_description>;
    using member_template = basic_interface<>::with_ability<details::ability_sets::member_template>;

    namespace details {
        consteval std::u8string display_list(std::vector<std::meta::info> const &operands) {
            std::u8string text;
            for (std::size_t i = 0; i < operands.size(); ++i) {
                if (i != 0) {
                    text += u8", ";
                }
                text += std::meta::u8display_string_of(operands[i]);
            }
            return text;
        }

        consteval std::u8string call_of(std::u8string_view function,
                                        std::vector<std::meta::info> const &arguments = {}) {
            return std::u8string(function) + u8"(" + display_list(arguments) + u8")";
        }

        consteval std::u8string template_call_of(std::u8string_view function,
                                                 std::vector<std::meta::info> const &template_arguments) {
            return std::u8string(function) + u8"<" + display_list(template_arguments) + u8">()";
        }

        consteval std::optional<std::meta::info> type_if_any([[maybe_unused]] std::meta::info r) {
#if METALIB_HAS_META_EXCEPTION
            try {
                return std::meta::type_of(r);
            } catch (std::meta::exception const &) {
            }
#endif
            return std::nullopt;
        }

        consteval meta_error unable(std::u8string_view verb, std::u8string_view action, std::u8string_view preposition,
                                    std::meta::info subject,
                                    std::source_location where = std::source_location::current()) {
            std::u8string message = u8"unable to ";
            if (!verb.empty()) {
                message += verb;
                message += u8" ";
            }
            message += action;
            message += u8" ";
            message += preposition;
            message += u8" ";
            message += quoted(subject);
            if (std::optional<std::meta::info> subject_type = type_if_any(subject)) {
                message += u8" of type '";
                message += std::meta::u8display_string_of(*subject_type);
                message += u8"'";
            }
            return meta_error(message, action, subject, where);
        }

        template<typename T, typename F, typename Describe>
        consteval meta_result<T> attempt(F f, [[maybe_unused]] Describe describe) {
#if METALIB_HAS_META_EXCEPTION
            try {
                return meta_result<T>(f());
            } catch (std::meta::exception const &) {
                return meta_result<T>(describe());
            }
#else
            return meta_result<T>(f());
#endif
        }

        consteval bool identifier_is_ascii(std::meta::info r) {
            return std::meta::has_identifier(r) && is_ascii(std::meta::u8identifier_of(r));
        }

        consteval bool is_tuple_like(std::meta::info type) {
            return std::meta::is_complete_type(
                std::meta::substitute(^^std::tuple_size, {std::meta::remove_cv(type)}));
        }

        consteval bool is_variant(std::meta::info type) {
            return std::meta::is_complete_type(
                std::meta::substitute(^^std::variant_size, {std::meta::remove_cv(type)}));
        }

        consteval bool is_integral_other_than_bool(std::meta::info type) {
            return std::meta::is_integral_type(type) && !std::meta::is_same_type(std::meta::remove_cv(type), ^^bool);
        }

        template<typename H = handle>
        consteval std::vector<H> handles_of(std::vector<std::meta::info> const &infos) {
            std::vector<H> handles(infos.size());
            for (std::size_t i = 0; i < infos.size(); ++i) {
                handles[i] = reflect_unchecked<H>(infos[i]);
            }
            return handles;
        }

        template<typename T>
        consteval std::vector<std::meta::info> infos_of(std::vector<T> const &handles) {
            std::vector<std::meta::info> infos(handles.size());
            for (std::size_t i = 0; i < handles.size(); ++i) {
                infos[i] = handles[i].info;
            }
            return infos;
        }

        struct accept_all {};

        template<typename Element, typename Filter, typename Map>
        consteval auto select(std::vector<std::meta::info> const &infos, Filter filter, Map map) {
            std::vector<Element> elements = handles_of<Element>(infos);
            if constexpr (!std::same_as<Filter, accept_all>) {
                std::erase_if(elements, [&filter](Element const &element) { return !std::invoke(filter, element); });
            }
            if constexpr (std::same_as<Map, std::identity>) {
                return elements;
            } else {
                using result = std::remove_cvref_t<std::invoke_result_t<Map &, Element const &>>;
                std::vector<result> mapped;
                if constexpr (std::default_initializable<result>) {
                    mapped.resize(elements.size());
                    for (std::size_t i = 0; i < elements.size(); ++i) {
                        mapped[i] = std::invoke(map, std::as_const(elements[i]));
                    }
                } else {
                    mapped.reserve(elements.size());
                    for (Element const &element: elements) {
                        mapped.push_back(std::invoke(map, element));
                    }
                }
                return mapped;
            }
        }

        template<typename Kind>
        consteval std::vector<std::meta::info> members_for(std::meta::info cls, std::meta::access_context ctx) {
            std::vector<std::meta::info> found;
            if constexpr (std::derived_from<Kind, ability::nonstatic_data>) {
                found = std::meta::nonstatic_data_members_of(cls, ctx);
            } else if constexpr (std::derived_from<Kind, ability::static_data>) {
                found = std::meta::static_data_members_of(cls, ctx);
            } else {
                found = std::meta::members_of(cls, ctx);
            }
            std::erase_if(found, [](std::meta::info r) { return !Kind::holds(r); });
            return found;
        }
    }

    namespace ability {
        struct identifiable {
            consteval bool has_identifier(this handle const &self) {
                return details::identifier_is_ascii(self.info);
            }

            consteval bool has_u8identifier(this handle const &self) {
                return std::meta::has_identifier(self.info);
            }

            consteval meta_result<string_literal> try_get_identifier(this handle const &self) {
                if (!std::meta::has_identifier(self.info)) {
                    return meta_result<string_literal>::failure(
                        u8"reflection " + details::quoted(self.info) + u8" has no identifier", self.info);
                }
                if (!details::is_ascii(std::meta::u8identifier_of(self.info))) {
                    return meta_result<string_literal>::failure(
                        u8"identifier of " + details::quoted(self.info) + u8" is not ASCII", self.info);
                }
                return string_literal{std::meta::identifier_of(self.info)};
            }

            consteval string_literal get_identifier(this handle const &self) {
                return details::attach_ability_unchecked<identifiable>(self).try_get_identifier().unwrap();
            }

            consteval meta_result<u8string_literal> try_get_u8identifier(this handle const &self) {
                if (!std::meta::has_identifier(self.info)) {
                    return meta_result<u8string_literal>::failure(
                        u8"reflection " + details::quoted(self.info) + u8" has no identifier", self.info);
                }
                return u8string_literal{std::meta::u8identifier_of(self.info)};
            }

            consteval u8string_literal get_u8identifier(this handle const &self) {
                return details::attach_ability_unchecked<identifiable>(self).try_get_u8identifier().unwrap();
            }
        };

        struct displayable {
            consteval meta_result<string_literal> try_get_display_string(this handle const &self) {
                if (!details::is_ascii(std::meta::u8display_string_of(self.info))) {
                    return meta_result<string_literal>::failure(
                        u8"display string of " + details::quoted(self.info) + u8" is not ASCII", self.info);
                }
                return string_literal{std::meta::display_string_of(self.info)};
            }

            consteval string_literal get_display_string(this handle const &self) {
                return details::attach_ability_unchecked<displayable>(self).try_get_display_string().unwrap();
            }

            consteval u8string_literal get_u8display_string(this handle const &self) {
                return u8string_literal{std::meta::u8display_string_of(self.info)};
            }

            consteval std::source_location get_source_location(this handle const &self) {
                return std::meta::source_location_of(self.info);
            }
        };

        struct sized {
            consteval meta_result<std::size_t> try_get_size(this handle const &self) {
                return details::attempt<std::size_t>(
                    [&] { return std::meta::size_of(self.info); },
                    [&] { return details::unable(u8"get", details::call_of(u8"size"), u8"from", self.info); });
            }

            consteval std::size_t get_size(this handle const &self) {
                return details::attach_ability_unchecked<sized>(self).try_get_size().unwrap();
            }

            consteval meta_result<std::size_t> try_get_alignment(this handle const &self) {
                return details::attempt<std::size_t>(
                    [&] { return std::meta::alignment_of(self.info); },
                    [&] { return details::unable(u8"get", details::call_of(u8"alignment"), u8"from", self.info); });
            }

            consteval std::size_t get_alignment(this handle const &self) {
                return details::attach_ability_unchecked<sized>(self).try_get_alignment().unwrap();
            }
        };

        struct type_traits {
            consteval bool is_complete(this handle const &self) {
                return std::meta::is_complete_type(self.info);
            }

            consteval bool is_enumerable(this handle const &self) {
                return std::meta::is_enumerable_type(self.info);
            }

            consteval bool is_void(this handle const &self) {
                return std::meta::is_void_type(self.info);
            }

            consteval bool is_null_pointer(this handle const &self) {
                return std::meta::is_null_pointer_type(self.info);
            }

            consteval bool is_integral(this handle const &self) {
                return std::meta::is_integral_type(self.info);
            }

            consteval bool is_floating_point(this handle const &self) {
                return std::meta::is_floating_point_type(self.info);
            }

            consteval bool is_array(this handle const &self) {
                return std::meta::is_array_type(self.info);
            }

            consteval bool is_pointer(this handle const &self) {
                return std::meta::is_pointer_type(self.info);
            }

            consteval bool is_lvalue_reference(this handle const &self) {
                return std::meta::is_lvalue_reference_type(self.info);
            }

            consteval bool is_rvalue_reference(this handle const &self) {
                return std::meta::is_rvalue_reference_type(self.info);
            }

            consteval bool is_member_object_pointer(this handle const &self) {
                return std::meta::is_member_object_pointer_type(self.info);
            }

            consteval bool is_member_function_pointer(this handle const &self) {
                return std::meta::is_member_function_pointer_type(self.info);
            }

            consteval bool is_enum(this handle const &self) {
                return std::meta::is_enum_type(self.info);
            }

            consteval bool is_union(this handle const &self) {
                return std::meta::is_union_type(self.info);
            }

            consteval bool is_class(this handle const &self) {
                return std::meta::is_class_type(self.info);
            }

            consteval bool is_function(this handle const &self) {
                return std::meta::is_function_type(self.info);
            }

            consteval bool is_reflection(this handle const &self) {
                return std::meta::is_reflection_type(self.info);
            }

            consteval bool is_reference(this handle const &self) {
                return std::meta::is_reference_type(self.info);
            }

            consteval bool is_arithmetic(this handle const &self) {
                return std::meta::is_arithmetic_type(self.info);
            }

            consteval bool is_fundamental(this handle const &self) {
                return std::meta::is_fundamental_type(self.info);
            }

            consteval bool is_object(this handle const &self) {
                return std::meta::is_object_type(self.info);
            }

            consteval bool is_scalar(this handle const &self) {
                return std::meta::is_scalar_type(self.info);
            }

            consteval bool is_compound(this handle const &self) {
                return std::meta::is_compound_type(self.info);
            }

            consteval bool is_member_pointer(this handle const &self) {
                return std::meta::is_member_pointer_type(self.info);
            }

            consteval bool is_const(this handle const &self) {
                return std::meta::is_const_type(self.info);
            }

            consteval bool is_volatile(this handle const &self) {
                return std::meta::is_volatile_type(self.info);
            }

            consteval bool is_trivially_copyable(this handle const &self) {
                return std::meta::is_trivially_copyable_type(self.info);
            }

            consteval bool is_standard_layout(this handle const &self) {
                return std::meta::is_standard_layout_type(self.info);
            }

            consteval bool is_empty(this handle const &self) {
                return std::meta::is_empty_type(self.info);
            }

            consteval bool is_polymorphic(this handle const &self) {
                return std::meta::is_polymorphic_type(self.info);
            }

            consteval bool is_abstract(this handle const &self) {
                return std::meta::is_abstract_type(self.info);
            }

            consteval bool is_final(this handle const &self) {
                return std::meta::is_final_type(self.info);
            }

            consteval bool is_aggregate(this handle const &self) {
                return std::meta::is_aggregate_type(self.info);
            }

            consteval bool is_structural(this handle const &self) {
                return std::meta::is_structural_type(self.info);
            }

            consteval bool is_signed(this handle const &self) {
                return std::meta::is_signed_type(self.info);
            }

            consteval bool is_unsigned(this handle const &self) {
                return std::meta::is_unsigned_type(self.info);
            }

            consteval bool is_bounded_array(this handle const &self) {
                return std::meta::is_bounded_array_type(self.info);
            }

            consteval bool is_unbounded_array(this handle const &self) {
                return std::meta::is_unbounded_array_type(self.info);
            }

            consteval bool is_scoped_enum(this handle const &self) {
                return std::meta::is_scoped_enum_type(self.info);
            }

            consteval bool is_default_constructible(this handle const &self) {
                return std::meta::is_default_constructible_type(self.info);
            }

            consteval bool is_copy_constructible(this handle const &self) {
                return std::meta::is_copy_constructible_type(self.info);
            }

            consteval bool is_move_constructible(this handle const &self) {
                return std::meta::is_move_constructible_type(self.info);
            }

            consteval bool is_copy_assignable(this handle const &self) {
                return std::meta::is_copy_assignable_type(self.info);
            }

            consteval bool is_move_assignable(this handle const &self) {
                return std::meta::is_move_assignable_type(self.info);
            }

            consteval bool is_swappable(this handle const &self) {
                return std::meta::is_swappable_type(self.info);
            }

            consteval bool is_destructible(this handle const &self) {
                return std::meta::is_destructible_type(self.info);
            }

            consteval bool is_trivially_default_constructible(this handle const &self) {
                return std::meta::is_trivially_default_constructible_type(self.info);
            }

            consteval bool is_trivially_copy_constructible(this handle const &self) {
                return std::meta::is_trivially_copy_constructible_type(self.info);
            }

            consteval bool is_trivially_move_constructible(this handle const &self) {
                return std::meta::is_trivially_move_constructible_type(self.info);
            }

            consteval bool is_trivially_copy_assignable(this handle const &self) {
                return std::meta::is_trivially_copy_assignable_type(self.info);
            }

            consteval bool is_trivially_move_assignable(this handle const &self) {
                return std::meta::is_trivially_move_assignable_type(self.info);
            }

            consteval bool is_trivially_destructible(this handle const &self) {
                return std::meta::is_trivially_destructible_type(self.info);
            }

            consteval bool is_nothrow_default_constructible(this handle const &self) {
                return std::meta::is_nothrow_default_constructible_type(self.info);
            }

            consteval bool is_nothrow_copy_constructible(this handle const &self) {
                return std::meta::is_nothrow_copy_constructible_type(self.info);
            }

            consteval bool is_nothrow_move_constructible(this handle const &self) {
                return std::meta::is_nothrow_move_constructible_type(self.info);
            }

            consteval bool is_nothrow_copy_assignable(this handle const &self) {
                return std::meta::is_nothrow_copy_assignable_type(self.info);
            }

            consteval bool is_nothrow_move_assignable(this handle const &self) {
                return std::meta::is_nothrow_move_assignable_type(self.info);
            }

            consteval bool is_nothrow_swappable(this handle const &self) {
                return std::meta::is_nothrow_swappable_type(self.info);
            }

            consteval bool is_nothrow_destructible(this handle const &self) {
                return std::meta::is_nothrow_destructible_type(self.info);
            }

            consteval bool is_implicit_lifetime(this handle const &self) {
                return std::meta::is_implicit_lifetime_type(self.info);
            }

            consteval bool has_virtual_destructor(this handle const &self) {
                return std::meta::has_virtual_destructor(self.info);
            }

            consteval bool has_unique_object_representations(this handle const &self) {
                return std::meta::has_unique_object_representations(self.info);
            }

            consteval std::size_t get_rank(this handle const &self) {
                return std::meta::rank(self.info);
            }

            consteval std::size_t get_extent(this handle const &self, unsigned dimension = 0) {
                return std::meta::extent(self.info, dimension);
            }

            consteval bool is_base_of(this handle const &self, type const &other);

            consteval bool is_virtual_base_of(this handle const &self, type const &other);

            consteval bool is_convertible_to(this handle const &self, type const &other);

            consteval bool is_nothrow_convertible_to(this handle const &self, type const &other);

            consteval bool is_layout_compatible_with(this handle const &self, type const &other);

            consteval bool is_pointer_interconvertible_base_of(this handle const &self, type const &other);

            consteval bool is_assignable_from(this handle const &self, type const &other);

            consteval bool is_trivially_assignable_from(this handle const &self, type const &other);

            consteval bool is_nothrow_assignable_from(this handle const &self, type const &other);

            consteval bool is_swappable_with(this handle const &self, type const &other);

            consteval bool is_nothrow_swappable_with(this handle const &self, type const &other);

            consteval bool is_constructible_from(this handle const &self, std::vector<type> const &arguments);

            consteval bool is_trivially_constructible_from(this handle const &self, std::vector<type> const &arguments);

            consteval bool is_nothrow_constructible_from(this handle const &self, std::vector<type> const &arguments);

            consteval bool is_invocable_with(this handle const &self, std::vector<type> const &arguments);

            consteval bool is_nothrow_invocable_with(this handle const &self, std::vector<type> const &arguments);

            consteval bool is_invocable_r_with(this handle const &self, type const &result,
                                               std::vector<type> const &arguments);

            consteval bool is_nothrow_invocable_r_with(this handle const &self, type const &result,
                                                       std::vector<type> const &arguments);

            consteval bool reference_constructs_from_temporary(this handle const &self, type const &source);

            consteval bool reference_converts_from_temporary(this handle const &self, type const &source);

            consteval meta_result<std::size_t> try_get_tuple_size(this handle const &self) {
                if (!details::is_tuple_like(self.info)) {
                    return meta_result<std::size_t>::failure(
                        u8"type " + details::quoted(self.info) + u8" is not tuple-like", self.info);
                }
                return std::meta::tuple_size(self.info);
            }

            consteval std::size_t get_tuple_size(this handle const &self) {
                return details::attach_ability_unchecked<type_traits>(self).try_get_tuple_size().unwrap();
            }

            consteval meta_result<std::size_t> try_get_variant_size(this handle const &self) {
                if (!details::is_variant(self.info)) {
                    return meta_result<std::size_t>::failure(
                        u8"type " + details::quoted(self.info) + u8" is not a variant", self.info);
                }
                return std::meta::variant_size(self.info);
            }

            consteval std::size_t get_variant_size(this handle const &self) {
                return details::attach_ability_unchecked<type_traits>(self).try_get_variant_size().unwrap();
            }
        };

        struct type_transforms {
            consteval type remove_const(this handle const &self);

            consteval type remove_volatile(this handle const &self);

            consteval type remove_cv(this handle const &self);

            consteval type add_const(this handle const &self);

            consteval type add_volatile(this handle const &self);

            consteval type add_cv(this handle const &self);

            consteval type remove_reference(this handle const &self);

            consteval type add_lvalue_reference(this handle const &self);

            consteval type add_rvalue_reference(this handle const &self);

            consteval type remove_extent(this handle const &self);

            consteval type remove_all_extents(this handle const &self);

            consteval type remove_pointer(this handle const &self);

            consteval type add_pointer(this handle const &self);

            consteval type remove_cvref(this handle const &self);

            consteval type decay(this handle const &self);

            consteval type unwrap_reference(this handle const &self);

            consteval type unwrap_ref_decay(this handle const &self);

            consteval meta_result<type> try_get_tuple_element(this handle const &self, std::size_t index);

            consteval type get_tuple_element(this handle const &self, std::size_t index);

            consteval meta_result<type> try_get_variant_alternative(this handle const &self, std::size_t index);

            consteval type get_variant_alternative(this handle const &self, std::size_t index);

            consteval meta_result<type> try_find_common_type_with(this handle const &self,
                                                                  std::vector<type> const &others);

            consteval type find_common_type_with(this handle const &self, std::vector<type> const &others);

            consteval meta_result<type> try_find_common_reference_with(this handle const &self,
                                                                       std::vector<type> const &others);

            consteval type find_common_reference_with(this handle const &self, std::vector<type> const &others);

            consteval meta_result<type> try_find_invoke_result_with(this handle const &self,
                                                                    std::vector<type> const &arguments);

            consteval type find_invoke_result_with(this handle const &self, std::vector<type> const &arguments);
        };

        struct templated {
            consteval bool has_template_arguments(this handle const &self) {
                return std::meta::has_template_arguments(self.info);
            }

            consteval meta_result<template_> try_get_template(this handle const &self);

            consteval template_ get_template(this handle const &self);

            consteval meta_result<std::vector<handle>> try_get_template_arguments(this handle const &self) {
                if (!std::meta::has_template_arguments(self.info)) {
                    return meta_result<std::vector<handle>>::failure(
                        u8"reflection " + details::quoted(self.info) + u8" is not a template specialization",
                        self.info);
                }
                return details::handles_of(std::meta::template_arguments_of(self.info));
            }

            consteval std::vector<handle> get_template_arguments(this handle const &self) {
                return details::attach_ability_unchecked<templated>(self).try_get_template_arguments().unwrap();
            }
        };

        struct annotatable {
            consteval std::vector<annotation> get_annotations(this handle const &self);

            consteval std::vector<annotation> get_annotations_with_type(this handle const &self,
                                                                          type const &annotation_type);

            template<typename T>
            consteval auto get_annotations_with_type(this handle const &self);
        };

        template<typename T>
        struct typed_as {
            static constexpr std::u8string_view requirement =
                std::define_static_string(u8"of type " + std::u8string(std::meta::u8display_string_of(^^T)));

            static consteval bool check(std::meta::info r) {
                std::optional<std::meta::info> const found = details::type_if_any(r);
                return found && std::meta::dealias(std::meta::remove_cv(*found)) == std::meta::dealias(^^T);
            }

            consteval meta_result<T> try_extract(this handle const &self) {
                return details::attempt<T>(
                    [&]() -> T { return std::meta::extract<T>(self.info); },
                    [&] { return details::unable({}, details::template_call_of(u8"extract", {^^T}), u8"from", self.info); });
            }

            consteval T extract(this handle const &self) {
                return details::attach_ability_unchecked<typed_as>(self).try_extract().unwrap();
            }
        };

        struct constant {
            template<typename T>
            consteval meta_result<T> try_extract(this handle const &self) {
                return details::attempt<T>(
                    [&]() -> T { return std::meta::extract<T>(self.info); },
                    [&] {
                        return details::unable({}, details::template_call_of(u8"extract", {^^T}), u8"from", self.info);
                    });
            }

            template<typename T>
            consteval T extract(this handle const &self) {
                return details::attach_ability_unchecked<constant>(self).template try_extract<T>().unwrap();
            }
        };

        struct typed {
            consteval meta_result<metalib::type> try_get_type(this handle const &self);

            consteval metalib::type get_type(this handle const &self);
        };

        struct type_entity {
            static constexpr std::u8string_view requirement = u8"a type";

            static consteval bool check(std::meta::info r) {
                return std::meta::is_type(r);
            }

            static consteval bool equal(std::meta::info a, std::meta::info b) {
                return std::meta::dealias(a) == std::meta::dealias(b);
            }

            consteval bool is_alias(this handle const &self) {
                return std::meta::is_type_alias(self.info);
            }

            consteval bool is(this handle const &self, type const &other);
        };

        struct arithmetic {
            static constexpr std::u8string_view requirement = u8"an arithmetic type";

            static consteval bool check(std::meta::info r) {
                return std::meta::is_type(r) && std::meta::is_arithmetic_type(r);
            }

            consteval meta_result<type> try_make_signed(this handle const &self);

            consteval type make_signed(this handle const &self);

            consteval meta_result<type> try_make_unsigned(this handle const &self);

            consteval type make_unsigned(this handle const &self);
        };

        struct enumeration {
            static constexpr std::u8string_view requirement = u8"an enumeration type";

            static consteval bool check(std::meta::info r) {
                return std::meta::is_type(r) && std::meta::is_enum_type(r);
            }

            consteval type get_underlying_type(this handle const &self);

            consteval type make_signed(this handle const &self);

            consteval type make_unsigned(this handle const &self);

            consteval meta_result<std::vector<enumerator>> try_get_enumerators(this handle const &self);

            consteval std::vector<enumerator> get_enumerators(this handle const &self);
        };

        struct parented {
            consteval bool has_parent(this handle const &self) {
                return std::meta::has_parent(self.info);
            }

            consteval meta_result<handle> try_get_parent(this handle const &self) {
                if (!std::meta::has_parent(self.info)) {
                    return meta_result<handle>::failure(
                        u8"reflection " + details::quoted(self.info) + u8" has no parent", self.info);
                }
                return handle{std::meta::parent_of(self.info)};
            }

            consteval handle get_parent(this handle const &self) {
                return details::attach_ability_unchecked<parented>(self).try_get_parent().unwrap();
            }
        };

        struct record {
            static constexpr std::u8string_view requirement = u8"a complete class or union type";

            static consteval bool check(std::meta::info r) {
                return std::meta::is_type(r) && (std::meta::is_class_type(r) || std::meta::is_union_type(r))
                       && std::meta::is_complete_type(r);
            }

            template<typename Kind = member, typename Filter = details::accept_all, typename Map = std::identity>
            consteval auto get_members(this handle const &self,
                                       std::meta::access_context ctx = std::meta::access_context::current(),
                                       Filter filter = {}, Map map = {}) {
                return details::select<Kind>(details::members_for<Kind>(self.info, ctx), std::move(filter),
                                             std::move(map));
            }

            consteval meta_result<destructor> try_get_destructor(
                this handle const &self, std::meta::access_context ctx = std::meta::access_context::current());

            consteval destructor get_destructor(
                this handle const &self, std::meta::access_context ctx = std::meta::access_context::current());

            consteval bool has_inaccessible_nonstatic_data_members(
                this handle const &self, std::meta::access_context ctx = std::meta::access_context::current()) {
                return std::meta::has_inaccessible_nonstatic_data_members(self.info, ctx);
            }
        };

        struct class_record {
            static constexpr std::u8string_view requirement = u8"a class type";

            static consteval bool check(std::meta::info r) {
                return std::meta::is_type(r) && std::meta::is_class_type(r);
            }

            template<typename Filter = details::accept_all, typename Map = std::identity>
            consteval auto get_bases(this handle const &self,
                                     std::meta::access_context ctx = std::meta::access_context::current(),
                                     Filter filter = {},
                                     Map map = {}) {
                return details::select<metalib::base>(std::meta::bases_of(self.info, ctx), std::move(filter),
                                                      std::move(map));
            }

            consteval bool has_inaccessible_bases(this handle const &self,
                                                  std::meta::access_context ctx =
                                                          std::meta::access_context::current()) {
                return std::meta::has_inaccessible_bases(self.info, ctx);
            }

            consteval bool has_inaccessible_subobjects(
                this handle const &self, std::meta::access_context ctx = std::meta::access_context::current()) {
                return std::meta::has_inaccessible_subobjects(self.info, ctx);
            }

            consteval std::vector<handle> get_subobjects(this handle const &self,
                                                           std::meta::access_context ctx =
                                                                   std::meta::access_context::current()) {
                return details::handles_of(std::meta::subobjects_of(self.info, ctx));
            }
        };

        struct union_record {
            static constexpr std::u8string_view requirement = u8"a union type";

            static consteval bool check(std::meta::info r) {
                return std::meta::is_type(r) && std::meta::is_union_type(r);
            }
        };

        struct accessible {
            consteval bool is_public(this handle const &self) {
                return std::meta::is_public(self.info);
            }

            consteval bool is_protected(this handle const &self) {
                return std::meta::is_protected(self.info);
            }

            consteval bool is_private(this handle const &self) {
                return std::meta::is_private(self.info);
            }

            consteval bool is_accessible(this handle const &self,
                                         std::meta::access_context ctx = std::meta::access_context::current()) {
                return std::meta::is_accessible(self.info, ctx);
            }
        };

        struct type_alias {
            static constexpr std::u8string_view requirement = u8"a type alias";

            static consteval bool check(std::meta::info r) {
                return std::meta::is_type_alias(r);
            }
        };

        struct non_alias {
            static constexpr std::u8string_view requirement = u8"the type's own declaration";

            static consteval bool check(std::meta::info r) {
                return !std::meta::is_type_alias(r);
            }
        };

        struct class_member {
            static constexpr std::u8string_view requirement = u8"a class member";

            static consteval bool check(std::meta::info r) {
                return std::meta::is_class_member(r);
            }
        };

        struct callable {
            consteval std::vector<parameter> get_parameters(this handle const &self);

            consteval bool is_noexcept(this handle const &self) {
                return std::meta::is_noexcept(self.info);
            }

            consteval bool is_deleted(this handle const &self) {
                return std::meta::is_deleted(self.info);
            }

            consteval bool is_defaulted(this handle const &self) {
                return std::meta::is_defaulted(self.info);
            }

            consteval bool is_user_provided(this handle const &self) {
                return std::meta::is_user_provided(self.info);
            }

            consteval bool is_user_declared(this handle const &self) {
                return std::meta::is_user_declared(self.info);
            }

            consteval bool is_vararg(this handle const &self) {
                return std::meta::is_vararg_function(self.info);
            }
        };

        struct signature {
            consteval meta_result<type> try_get_return_type(this handle const &self);

            consteval type get_return_type(this handle const &self);

            consteval bool is_operator(this handle const &self) {
                return std::meta::is_operator_function(self.info);
            }

            consteval meta_result<std::meta::operators> try_get_operator_kind(this handle const &self) {
                if (!std::meta::is_operator_function(self.info)) {
                    return meta_result<std::meta::operators>::failure(
                        u8"function " + details::quoted(self.info) + u8" is not an operator function", self.info);
                }
                return std::meta::operator_of(self.info);
            }

            consteval std::meta::operators get_operator_kind(this handle const &self) {
                return details::attach_ability_unchecked<signature>(self).try_get_operator_kind().unwrap();
            }

            consteval bool is_literal_operator(this handle const &self) {
                return std::meta::is_literal_operator(self.info);
            }

            consteval meta_result<string_literal> try_get_operator_symbol(this handle const &self) {
                if (!std::meta::is_operator_function(self.info)) {
                    return meta_result<string_literal>::failure(
                        u8"function " + details::quoted(self.info) + u8" is not an operator function", self.info);
                }
                return string_literal{std::meta::symbol_of(std::meta::operator_of(self.info))};
            }

            consteval string_literal get_operator_symbol(this handle const &self) {
                return details::attach_ability_unchecked<signature>(self).try_get_operator_symbol().unwrap();
            }

            consteval meta_result<u8string_literal> try_get_u8operator_symbol(this handle const &self) {
                if (!std::meta::is_operator_function(self.info)) {
                    return meta_result<u8string_literal>::failure(
                        u8"function " + details::quoted(self.info) + u8" is not an operator function", self.info);
                }
                return u8string_literal{std::meta::u8symbol_of(std::meta::operator_of(self.info))};
            }

            consteval u8string_literal get_u8operator_symbol(this handle const &self) {
                return details::attach_ability_unchecked<signature>(self).try_get_u8operator_symbol().unwrap();
            }
        };

        struct virtual_specifier {
            consteval bool is_virtual(this handle const &self) {
                return std::meta::is_virtual(self.info);
            }
        };

        struct override_specifier {
            consteval bool is_pure_virtual(this handle const &self) {
                return std::meta::is_pure_virtual(self.info);
            }

            consteval bool is_override(this handle const &self) {
                return std::meta::is_override(self.info);
            }

            consteval bool is_final(this handle const &self) {
                return std::meta::is_final(self.info);
            }
        };

        struct explicit_specifier {
            consteval bool is_explicit(this handle const &self) {
                return std::meta::is_explicit(self.info);
            }
        };

        struct nonstatic_data {
            static constexpr std::u8string_view requirement = u8"a non-static data member";

            static consteval bool check(std::meta::info r) {
                return std::meta::is_nonstatic_data_member(r);
            }

            consteval std::meta::member_offset get_offset(this handle const &self) {
                return std::meta::offset_of(self.info);
            }

            consteval std::size_t get_bit_size(this handle const &self) {
                return std::meta::bit_size_of(self.info);
            }

            consteval bool is_bit_field(this handle const &self) {
                return std::meta::is_bit_field(self.info);
            }

            consteval bool is_mutable(this handle const &self) {
                return std::meta::is_mutable_member(self.info);
            }

            consteval bool has_default_member_initializer(this handle const &self) {
                return std::meta::has_default_member_initializer(self.info);
            }
        };

        struct static_data {
            static constexpr std::u8string_view requirement = u8"a static data member";

            static consteval bool check(std::meta::info r) {
                return std::meta::is_variable(r) && std::meta::is_static_member(r);
            }
        };

        struct storage_duration {
            consteval bool has_static_storage_duration(this handle const &self) {
                return std::meta::has_static_storage_duration(self.info);
            }

            consteval bool has_thread_storage_duration(this handle const &self) {
                return std::meta::has_thread_storage_duration(self.info);
            }

            consteval bool has_automatic_storage_duration(this handle const &self) {
                return std::meta::has_automatic_storage_duration(self.info);
            }

            consteval meta_result<object> try_get_object(this handle const &self);

            consteval object get_object(this handle const &self);
        };

        struct linkage {
            consteval bool has_linkage(this handle const &self) {
                return std::meta::has_linkage(self.info);
            }

            consteval bool has_internal_linkage(this handle const &self) {
                return std::meta::has_internal_linkage(self.info);
            }

            consteval bool has_external_linkage(this handle const &self) {
                return std::meta::has_external_linkage(self.info);
            }

            consteval bool has_module_linkage(this handle const &self) {
                return std::meta::has_module_linkage(self.info);
            }

            consteval bool has_c_language_linkage(this handle const &self) {
                return std::meta::has_c_language_linkage(self.info);
            }
        };

        struct namespace_entity {
            static constexpr std::u8string_view requirement = u8"a namespace";

            static consteval bool check(std::meta::info r) {
                return std::meta::is_namespace(r) && !std::meta::is_namespace_alias(r);
            }

            template<typename Kind = metalib::namespace_member, typename Filter = details::accept_all,
                typename Map = std::identity>
            consteval auto get_members(this handle const &self,
                                       std::meta::access_context ctx = std::meta::access_context::current(),
                                       Filter filter = {}, Map map = {}) {
                return details::select<Kind>(details::members_for<Kind>(self.info, ctx), std::move(filter),
                                             std::move(map));
            }
        };

        struct namespace_alias_entity {
            static constexpr std::u8string_view requirement = u8"a namespace alias";

            static consteval bool check(std::meta::info r) {
                return std::meta::is_namespace_alias(r);
            }

            consteval metalib::namespace_ get_target(this handle const &self);
        };

        struct namespace_member_entity {
            static constexpr std::u8string_view requirement = u8"a namespace member";

            static consteval bool check(std::meta::info r) {
                return std::meta::is_namespace_member(r);
            }
        };

        struct variable_entity {
            static constexpr std::u8string_view requirement = u8"a variable";

            static consteval bool check(std::meta::info r) {
                return std::meta::is_variable(r);
            }
        };

        struct function_entity {
            static constexpr std::u8string_view requirement = u8"a function";

            static consteval bool check(std::meta::info r) {
                return std::meta::is_function(r);
            }
        };

        struct constant_entity {
            static constexpr std::u8string_view requirement = u8"a constant";

            static consteval bool check(std::meta::info r) {
                return std::meta::is_value(r) || std::meta::is_object(r);
            }

            consteval bool is_value_constant(this handle const &self) {
                return std::meta::is_value(self.info);
            }

            consteval bool is_object_constant(this handle const &self) {
                return std::meta::is_object(self.info);
            }
        };

        struct value_entity {
            static constexpr std::u8string_view requirement = u8"a value";

            static consteval bool check(std::meta::info r) {
                return std::meta::is_value(r);
            }
        };

        struct object_entity {
            static constexpr std::u8string_view requirement = u8"an object";

            static consteval bool check(std::meta::info r) {
                return std::meta::is_object(r);
            }
        };

        struct annotation_entity {
            static constexpr std::u8string_view requirement = u8"an annotation";

            static consteval bool check(std::meta::info r) {
                return std::meta::is_annotation(r);
            }

            consteval metalib::constant get_constant(this handle const &self);
        };

        struct structured_binding_entity {
            static constexpr std::u8string_view requirement = u8"a structured binding";

            static consteval bool check(std::meta::info r) {
                return std::meta::is_structured_binding(r);
            }
        };
        struct data_member_description_entity {
            static constexpr std::u8string_view requirement = u8"a data member description";

            static consteval bool check(std::meta::info r) {
                return std::meta::is_data_member_spec(r);
            }

            consteval std::size_t get_bit_size(this handle const &self) {
                return std::meta::bit_size_of(self.info);
            }

            consteval bool is_bit_field(this handle const &self) {
                return std::meta::is_bit_field(self.info);
            }
        };

        struct template_entity {
            static constexpr std::u8string_view requirement = u8"a template";

            static consteval bool check(std::meta::info r) {
                return std::meta::is_template(r);
            }

            consteval bool is_function_template(this handle const &self) {
                return std::meta::is_function_template(self.info);
            }

            consteval bool is_variable_template(this handle const &self) {
                return std::meta::is_variable_template(self.info);
            }

            consteval bool is_class_template(this handle const &self) {
                return std::meta::is_class_template(self.info);
            }

            consteval bool is_alias_template(this handle const &self) {
                return std::meta::is_alias_template(self.info);
            }

            consteval bool is_conversion_function_template(this handle const &self) {
                return std::meta::is_conversion_function_template(self.info);
            }

            consteval bool is_operator_function_template(this handle const &self) {
                return std::meta::is_operator_function_template(self.info);
            }

            consteval bool is_literal_operator_template(this handle const &self) {
                return std::meta::is_literal_operator_template(self.info);
            }

            consteval bool is_constructor_template(this handle const &self) {
                return std::meta::is_constructor_template(self.info);
            }

            consteval bool is_concept(this handle const &self) {
                return std::meta::is_concept(self.info);
            }

            consteval meta_result<std::meta::operators> try_get_operator_kind(this handle const &self) {
                if (!std::meta::is_operator_function_template(self.info)) {
                    return meta_result<std::meta::operators>::failure(
                        u8"template " + details::quoted(self.info) + u8" is not an operator function template",
                        self.info);
                }
                return std::meta::operator_of(self.info);
            }

            consteval std::meta::operators get_operator_kind(this handle const &self) {
                return details::attach_ability_unchecked<template_entity>(self).try_get_operator_kind().unwrap();
            }

            consteval bool can_substitute(this handle const &self, std::vector<handle> const &arguments) {
                return std::meta::can_substitute(self.info, details::infos_of(arguments));
            }

            consteval meta_result<handle> try_substitute(this handle const &self,
                                                         std::vector<handle> const &arguments) {
                std::vector<std::meta::info> infos = details::infos_of(arguments);
                return details::attempt<handle>(
                    [&] { return handle{std::meta::substitute(self.info, infos)}; },
                    [&] { return details::unable({}, details::call_of(u8"substitute", infos), u8"into", self.info); });
            }

            consteval handle substitute(this handle const &self, std::vector<handle> const &arguments) {
                return details::attach_ability_unchecked<template_entity>(self).try_substitute(arguments).unwrap();
            }
        };

        struct nonstatic_function {
            static constexpr std::u8string_view requirement = u8"a non-static member function";

            static consteval bool check(std::meta::info r) {
                return std::meta::is_function(r) && std::meta::is_class_member(r) && !std::meta::is_static_member(r)
                       && !std::meta::is_constructor(r) && !std::meta::is_destructor(r);
            }

            consteval bool has_explicit_object_parameter(this handle const &self) {
                std::vector<std::meta::info> parameters = std::meta::parameters_of(self.info);
                return !parameters.empty() && std::meta::is_explicit_object_parameter(parameters.front());
            }

            consteval bool is_const(this handle const &self) {
                return std::meta::is_const(self.info);
            }

            consteval bool is_volatile(this handle const &self) {
                return std::meta::is_volatile(self.info);
            }

            consteval bool is_lvalue_reference_qualified(this handle const &self) {
                return std::meta::is_lvalue_reference_qualified(self.info);
            }

            consteval bool is_rvalue_reference_qualified(this handle const &self) {
                return std::meta::is_rvalue_reference_qualified(self.info);
            }

            consteval bool is_conversion(this handle const &self) {
                return std::meta::is_conversion_function(self.info);
            }

            consteval bool is_special_member(this handle const &self) {
                return std::meta::is_special_member_function(self.info);
            }

            consteval bool is_copy_assignment(this handle const &self) {
                return std::meta::is_copy_assignment(self.info);
            }

            consteval bool is_move_assignment(this handle const &self) {
                return std::meta::is_move_assignment(self.info);
            }

            consteval bool is_assignment(this handle const &self) {
                return std::meta::is_assignment(self.info);
            }
        };

        struct static_function {
            static constexpr std::u8string_view requirement = u8"a static member function";

            static consteval bool check(std::meta::info r) {
                return std::meta::is_function(r) && std::meta::is_class_member(r) && std::meta::is_static_member(r);
            }
        };

        struct constructor_entity {
            static constexpr std::u8string_view requirement = u8"a constructor";

            static consteval bool check(std::meta::info r) {
                return std::meta::is_constructor(r);
            }

            consteval bool is_default_constructor(this handle const &self) {
                return std::meta::is_default_constructor(self.info);
            }

            consteval bool is_copy_constructor(this handle const &self) {
                return std::meta::is_copy_constructor(self.info);
            }

            consteval bool is_move_constructor(this handle const &self) {
                return std::meta::is_move_constructor(self.info);
            }
        };

        struct destructor_entity {
            static constexpr std::u8string_view requirement = u8"a destructor";

            static consteval bool check(std::meta::info r) {
                return std::meta::is_destructor(r);
            }
        };

        struct base_entity {
            static constexpr std::u8string_view requirement = u8"a base class relationship";

            static consteval bool check(std::meta::info r) {
                return std::meta::is_base(r);
            }

            consteval meta_result<std::meta::member_offset> try_get_offset(this handle const &self) {
                return details::attempt<std::meta::member_offset>(
                    [&] { return std::meta::offset_of(self.info); },
                    [&] { return details::unable(u8"get", details::call_of(u8"offset"), u8"from", self.info); });
            }

            consteval std::meta::member_offset get_offset(this handle const &self) {
                return details::attach_ability_unchecked<base_entity>(self).try_get_offset().unwrap();
            }
        };

        struct enumerator_entity {
            static constexpr std::u8string_view requirement = u8"an enumerator";

            static consteval bool check(std::meta::info r) {
                return std::meta::is_enumerator(r);
            }
        };

        struct parameter_entity {
            static constexpr std::u8string_view requirement = u8"a function parameter";

            static consteval bool check(std::meta::info r) {
                return std::meta::is_function_parameter(r);
            }

            consteval bool has_default_argument(this handle const &self) {
                return std::meta::has_default_argument(self.info);
            }

            consteval bool is_explicit_object_parameter(this handle const &self) {
                return std::meta::is_explicit_object_parameter(self.info);
            }

            consteval meta_result<handle> try_get_variable(this handle const &self) {
                return details::attempt<handle>(
                    [&] { return handle{std::meta::variable_of(self.info)}; },
                    [&] { return details::unable(u8"get", details::call_of(u8"variable"), u8"from", self.info); });
            }

            consteval handle get_variable(this handle const &self) {
                return details::attach_ability_unchecked<parameter_entity>(self).try_get_variable().unwrap();
            }
        };
    }

    consteval bool
    ability::type_traits::is_pointer_interconvertible_base_of(this handle const &self, type const &other) {
        return std::meta::is_pointer_interconvertible_base_of_type(self.info, other.info);
    }

    consteval bool
    ability::type_traits::is_invocable_with(this handle const &self, std::vector<type> const &arguments) {
        return std::meta::is_invocable_type(self.info, details::infos_of(arguments));
    }

    inline constexpr namespace_ global_namespace = details::reflect_unchecked<namespace_>(^^::);

    consteval bool
    ability::type_traits::reference_converts_from_temporary(this handle const &self, type const &source) {
        return std::meta::reference_converts_from_temporary(self.info, source.info);
    }

    consteval constant ability::annotation_entity::get_constant(this handle const &self) {
        return details::reflect_unchecked<metalib::constant>(std::meta::constant_of(self.info));
    }

    template<typename T>
        requires std::is_copy_constructible_v<std::decay_t<T>>
    consteval auto reflect_constant(T const &value) {
        if constexpr (std::is_class_v<T> || std::is_union_v<T>) {
            return details::reflect_unchecked<typed_object<T>>(std::meta::reflect_constant(value));
        } else {
            return details::reflect_unchecked<typed_constant<T>>(std::meta::reflect_constant(value));
        }
    }

    template<typename T>
        requires (!std::is_function_v<T>)
    consteval typed_object<std::remove_cv_t<T>> reflect_object(T &target) {
        return details::reflect_unchecked<typed_object<std::remove_cv_t<T>>>(std::meta::reflect_object(target));
    }

    template<typename T>
    consteval auto ability::annotatable::get_annotations_with_type(this handle const &self) {
        return details::handles_of<typed_annotation<T>>(std::meta::annotations_of_with_type(self.info, ^^T));
    }

    template<typename F>
        requires std::is_function_v<F>
    consteval handle reflect_function(F &target) {
        return handle{std::meta::reflect_function(target)};
    }

    template<std::ranges::input_range R>
    consteval object reflect_constant_string(R &&text) {
        return details::reflect_unchecked<object>(std::meta::reflect_constant_string(std::forward<R>(text)));
    }

    template<std::ranges::input_range R>
    consteval object reflect_constant_array(R &&elements) {
        return details::reflect_unchecked<object>(std::meta::reflect_constant_array(std::forward<R>(elements)));
    }

    consteval meta_result<handle>
    try_get_current_function(std::meta::access_context ctx = std::meta::access_context::current()) {
        if (!std::meta::is_function(ctx.scope())) {
            return meta_result<handle>::failure(
                u8"evaluation scope " + details::quoted(ctx.scope()) + u8" is not a function", ctx.scope());
        }
        return handle{ctx.scope()};
    }

    consteval handle get_current_function(std::meta::access_context ctx = std::meta::access_context::current()) {
        return try_get_current_function(ctx).unwrap();
    }

    consteval meta_result<type>
    try_get_current_class(std::meta::access_context ctx = std::meta::access_context::current()) {
        for (std::meta::info scope = ctx.scope(); !std::meta::is_namespace(scope);
             scope = std::meta::parent_of(scope)) {
            if (std::meta::is_type(scope)) {
                return details::reflect_unchecked<type>(scope);
            }
        }
        return meta_result<type>::failure(
            u8"evaluation scope " + details::quoted(ctx.scope()) + u8" is not inside a class", ctx.scope());
    }

    consteval type get_current_class(std::meta::access_context ctx = std::meta::access_context::current()) {
        return try_get_current_class(ctx).unwrap();
    }

    consteval namespace_ get_current_namespace(std::meta::access_context ctx = std::meta::access_context::current()) {
        std::meta::info scope = ctx.scope();
        while (!std::meta::is_namespace(scope)) {
            scope = std::meta::parent_of(scope);
        }
        return details::reflect_unchecked<namespace_>(scope);
    }

    consteval meta_result<data_member_description> try_make_data_member_spec(
        type const &member_type, std::meta::data_member_options options = {}) {
        return details::attempt<data_member_description>(
            [&] {
                return details::reflect_unchecked<data_member_description>(
                    std::meta::data_member_spec(member_type.info, std::move(options)));
            },
            [&] {
                return details::unable(u8"make", details::call_of(u8"data_member_spec"), u8"from", member_type.info);
            });
    }

    consteval data_member_description make_data_member_spec(type const &member_type,
                                                            std::meta::data_member_options options = {}) {
        return try_make_data_member_spec(member_type, std::move(options)).unwrap();
    }

    consteval meta_result<type> try_define_aggregate(type const &incomplete,
                                                     std::vector<data_member_description> const &specs) {
        std::vector<std::meta::info> infos = details::infos_of(specs);
        return details::attempt<type>(
            [&] { return details::reflect_unchecked<type>(std::meta::define_aggregate(incomplete.info, infos)); },
            [&] {
                return details::unable(u8"apply", details::call_of(u8"define_aggregate", infos), u8"to",
                                       incomplete.info);
            });
    }

    consteval type define_aggregate(type const &incomplete, std::vector<data_member_description> const &specs) {
        return try_define_aggregate(incomplete, specs).unwrap();
    }

    namespace details {
        template<typename T>
        consteval bool flattenable() {
            if constexpr (std::derived_from<T, handle>) {
                return true;
            } else if constexpr (std::ranges::input_range<T const>) {
                return flattenable<std::ranges::range_value_t<T const>>();
            } else {
                return false;
            }
        }

        template<typename T>
        consteval void flatten_into(std::vector<handle> &out, T const &item) {
            if constexpr (std::derived_from<T, handle>) {
                out.push_back(handle{item.info});
            } else {
                for (auto const &element: item) {
                    details::flatten_into(out, element);
                }
            }
        }
    }

    template<typename... Items>
        requires (details::flattenable<Items>() && ...)
    consteval std::vector<handle> flatten(Items const &... items) {
        std::vector<handle> out;
        (details::flatten_into(out, items), ...);
        return out;
    }
    consteval bool ability::type_traits::is_base_of(this handle const &self, type const &other) {
        return std::meta::is_base_of_type(self.info, other.info);
    }

    consteval bool ability::type_traits::is_virtual_base_of(this handle const &self, type const &other) {
        return std::meta::is_virtual_base_of_type(self.info, other.info);
    }

    consteval bool ability::type_traits::is_convertible_to(this handle const &self, type const &other) {
        return std::meta::is_convertible_type(self.info, other.info);
    }

    consteval bool ability::type_traits::is_nothrow_convertible_to(this handle const &self, type const &other) {
        return std::meta::is_nothrow_convertible_type(self.info, other.info);
    }

    consteval bool ability::type_traits::is_layout_compatible_with(this handle const &self, type const &other) {
        return std::meta::is_layout_compatible_type(self.info, other.info);
    }

    consteval bool ability::type_traits::is_assignable_from(this handle const &self, type const &other) {
        return std::meta::is_assignable_type(self.info, other.info);
    }

    consteval bool ability::type_traits::is_trivially_assignable_from(this handle const &self, type const &other) {
        return std::meta::is_trivially_assignable_type(self.info, other.info);
    }

    consteval bool ability::type_traits::is_nothrow_assignable_from(this handle const &self, type const &other) {
        return std::meta::is_nothrow_assignable_type(self.info, other.info);
    }

    consteval bool ability::type_traits::is_swappable_with(this handle const &self, type const &other) {
        return std::meta::is_swappable_with_type(self.info, other.info);
    }

    consteval bool ability::type_traits::is_nothrow_swappable_with(this handle const &self, type const &other) {
        return std::meta::is_nothrow_swappable_with_type(self.info, other.info);
    }

    consteval bool ability::type_traits::is_constructible_from(this handle const &self,
                                                               std::vector<type> const &arguments) {
        return std::meta::is_constructible_type(self.info, details::infos_of(arguments));
    }

    consteval bool ability::type_traits::is_trivially_constructible_from(
        this handle const &self, std::vector<type> const &arguments) {
        return std::meta::is_trivially_constructible_type(self.info, details::infos_of(arguments));
    }

    consteval bool ability::type_traits::is_nothrow_constructible_from(this handle const &self,
                                                                       std::vector<type> const &arguments) {
        return std::meta::is_nothrow_constructible_type(self.info, details::infos_of(arguments));
    }

    consteval bool ability::type_traits::is_nothrow_invocable_with(this handle const &self,
                                                                   std::vector<type> const &arguments) {
        return std::meta::is_nothrow_invocable_type(self.info, details::infos_of(arguments));
    }

    consteval bool ability::type_traits::is_invocable_r_with(this handle const &self, type const &result,
                                                             std::vector<type> const &arguments) {
        return std::meta::is_invocable_r_type(result.info, self.info, details::infos_of(arguments));
    }

    consteval bool ability::type_traits::is_nothrow_invocable_r_with(this handle const &self, type const &result,
                                                                     std::vector<type> const &arguments) {
        return std::meta::is_nothrow_invocable_r_type(result.info, self.info, details::infos_of(arguments));
    }

    consteval bool ability::type_traits::reference_constructs_from_temporary(
        this handle const &self, type const &source) {
        return std::meta::reference_constructs_from_temporary(self.info, source.info);
    }

    consteval type ability::type_transforms::remove_const(this handle const &self) {
        return details::reflect_unchecked<type>(std::meta::remove_const(self.info));
    }

    consteval type ability::type_transforms::remove_volatile(this handle const &self) {
        return details::reflect_unchecked<type>(std::meta::remove_volatile(self.info));
    }

    consteval type ability::type_transforms::remove_cv(this handle const &self) {
        return details::reflect_unchecked<type>(std::meta::remove_cv(self.info));
    }

    consteval type ability::type_transforms::add_const(this handle const &self) {
        return details::reflect_unchecked<type>(std::meta::add_const(self.info));
    }

    consteval type ability::type_transforms::add_volatile(this handle const &self) {
        return details::reflect_unchecked<type>(std::meta::add_volatile(self.info));
    }

    consteval type ability::type_transforms::add_cv(this handle const &self) {
        return details::reflect_unchecked<type>(std::meta::add_cv(self.info));
    }

    consteval type ability::type_transforms::remove_reference(this handle const &self) {
        return details::reflect_unchecked<type>(std::meta::remove_reference(self.info));
    }

    consteval type ability::type_transforms::add_lvalue_reference(this handle const &self) {
        return details::reflect_unchecked<type>(std::meta::add_lvalue_reference(self.info));
    }

    consteval type ability::type_transforms::add_rvalue_reference(this handle const &self) {
        return details::reflect_unchecked<type>(std::meta::add_rvalue_reference(self.info));
    }

    consteval type ability::type_transforms::remove_extent(this handle const &self) {
        return details::reflect_unchecked<type>(std::meta::remove_extent(self.info));
    }

    consteval type ability::type_transforms::remove_all_extents(this handle const &self) {
        return details::reflect_unchecked<type>(std::meta::remove_all_extents(self.info));
    }

    consteval type ability::type_transforms::remove_pointer(this handle const &self) {
        return details::reflect_unchecked<type>(std::meta::remove_pointer(self.info));
    }

    consteval type ability::type_transforms::add_pointer(this handle const &self) {
        return details::reflect_unchecked<type>(std::meta::add_pointer(self.info));
    }

    consteval type ability::type_transforms::remove_cvref(this handle const &self) {
        return details::reflect_unchecked<type>(std::meta::remove_cvref(self.info));
    }

    consteval type ability::type_transforms::decay(this handle const &self) {
        return details::reflect_unchecked<type>(std::meta::decay(self.info));
    }

    consteval type ability::type_transforms::unwrap_reference(this handle const &self) {
        return details::reflect_unchecked<type>(std::meta::unwrap_reference(self.info));
    }

    consteval type ability::type_transforms::unwrap_ref_decay(this handle const &self) {
        return details::reflect_unchecked<type>(std::meta::unwrap_ref_decay(self.info));
    }

    consteval meta_result<type> ability::type_transforms::try_get_tuple_element(this handle const &self,
                                                                                std::size_t index) {
        if (!details::is_tuple_like(self.info) || index >= std::meta::tuple_size(self.info)) {
            return meta_result<type>::failure(
                u8"type " + details::quoted(self.info) + u8" is not tuple-like or the index is out of range",
                self.info);
        }
        return details::reflect_unchecked<type>(std::meta::tuple_element(index, self.info));
    }

    consteval type ability::type_transforms::get_tuple_element(this handle const &self, std::size_t index) {
        return details::attach_ability_unchecked<type_transforms>(self).try_get_tuple_element(index).unwrap();
    }

    consteval meta_result<type> ability::type_transforms::try_get_variant_alternative(
        this handle const &self, std::size_t index) {
        if (!details::is_variant(self.info) || index >= std::meta::variant_size(self.info)) {
            return meta_result<type>::failure(
                u8"type " + details::quoted(self.info) + u8" is not a variant or the index is out of range", self.info);
        }
        return details::reflect_unchecked<type>(std::meta::variant_alternative(index, self.info));
    }

    consteval type ability::type_transforms::get_variant_alternative(this handle const &self, std::size_t index) {
        return details::attach_ability_unchecked<type_transforms>(self).try_get_variant_alternative(index).unwrap();
    }

    consteval meta_result<type> ability::type_transforms::try_find_common_type_with(this handle const &self,
                                                                                    std::vector<type> const &others) {
        std::vector<std::meta::info> infos = details::infos_of(others);
        std::vector<std::meta::info> types{self.info};
        types.append_range(infos);
        return details::attempt<type>(
            [&] { return details::reflect_unchecked<type>(std::meta::common_type(types)); },
            [&] {
                return details::unable(u8"find", details::call_of(u8"common_type_with", infos), u8"from", self.info);
            });
    }

    consteval type ability::type_transforms::find_common_type_with(this handle const &self,
                                                                   std::vector<type> const &others) {
        return details::attach_ability_unchecked<type_transforms>(self).try_find_common_type_with(others).unwrap();
    }

    consteval meta_result<type> ability::type_transforms::try_find_common_reference_with(
        this handle const &self, std::vector<type> const &others) {
        std::vector<std::meta::info> infos = details::infos_of(others);
        std::vector<std::meta::info> types{self.info};
        types.append_range(infos);
        return details::attempt<type>(
            [&] { return details::reflect_unchecked<type>(std::meta::common_reference(types)); },
            [&] {
                return details::unable(u8"find", details::call_of(u8"common_reference_with", infos), u8"from",
                                       self.info);
            });
    }

    consteval type ability::type_transforms::find_common_reference_with(this handle const &self,
                                                                        std::vector<type> const &others) {
        return details::attach_ability_unchecked<type_transforms>(self).try_find_common_reference_with(others).unwrap();
    }

    consteval meta_result<type> ability::type_transforms::try_find_invoke_result_with(
        this handle const &self, std::vector<type> const &arguments) {
        std::vector<std::meta::info> infos = details::infos_of(arguments);
        return details::attempt<type>(
            [&] { return details::reflect_unchecked<type>(std::meta::invoke_result(self.info, infos)); },
            [&] {
                return details::unable(u8"find", details::call_of(u8"invoke_result_with", infos), u8"from", self.info);
            });
    }

    consteval type ability::type_transforms::find_invoke_result_with(this handle const &self,
                                                                     std::vector<type> const &arguments) {
        return details::attach_ability_unchecked<type_transforms>(self).try_find_invoke_result_with(arguments).unwrap();
    }

    consteval meta_result<template_> ability::templated::try_get_template(this handle const &self) {
        if (!std::meta::has_template_arguments(self.info)) {
            return meta_result<metalib::template_>::failure(
                u8"reflection " + details::quoted(self.info) + u8" is not a template specialization", self.info);
        }
        return details::reflect_unchecked<metalib::template_>(std::meta::template_of(self.info));
    }

    consteval template_ ability::templated::get_template(this handle const &self) {
        return details::attach_ability_unchecked<templated>(self).try_get_template().unwrap();
    }

    consteval std::vector<annotation> ability::annotatable::get_annotations(this handle const &self) {
        return details::handles_of<annotation>(std::meta::annotations_of(self.info));
    }

    consteval std::vector<annotation> ability::annotatable::get_annotations_with_type(this handle const &self,
            type const &annotation_type) {
        return details::handles_of<
            annotation>(std::meta::annotations_of_with_type(self.info, annotation_type.info));
    }

    consteval meta_result<type> ability::typed::try_get_type(this handle const &self) {
        return details::attempt<type>(
            [&] { return details::reflect_unchecked<type>(std::meta::type_of(self.info)); },
            [&] { return details::unable(u8"get", details::call_of(u8"type"), u8"from", self.info); });
    }

    consteval type ability::typed::get_type(this handle const &self) {
        return details::attach_ability_unchecked<typed>(self).try_get_type().unwrap();
    }

    consteval bool ability::type_entity::is(this handle const &self, type const &other) {
        return equal(self.info, other.info);
    }

    consteval meta_result<type> ability::arithmetic::try_make_signed(this handle const &self) {
        if (!details::is_integral_other_than_bool(self.info)) {
            return meta_result<type>::failure(
                u8"type " + details::quoted(self.info) + u8" is not an integral type other than bool", self.info);
        }
        return details::reflect_unchecked<type>(std::meta::make_signed(self.info));
    }

    consteval type ability::arithmetic::make_signed(this handle const &self) {
        return details::attach_ability_unchecked<arithmetic>(self).try_make_signed().unwrap();
    }

    consteval meta_result<type> ability::arithmetic::try_make_unsigned(this handle const &self) {
        if (!details::is_integral_other_than_bool(self.info)) {
            return meta_result<type>::failure(
                u8"type " + details::quoted(self.info) + u8" is not an integral type other than bool", self.info);
        }
        return details::reflect_unchecked<type>(std::meta::make_unsigned(self.info));
    }

    consteval type ability::arithmetic::make_unsigned(this handle const &self) {
        return details::attach_ability_unchecked<arithmetic>(self).try_make_unsigned().unwrap();
    }

    consteval type ability::enumeration::get_underlying_type(this handle const &self) {
        return details::reflect_unchecked<type>(std::meta::underlying_type(self.info));
    }

    consteval type ability::enumeration::make_signed(this handle const &self) {
        return details::reflect_unchecked<type>(std::meta::make_signed(self.info));
    }

    consteval type ability::enumeration::make_unsigned(this handle const &self) {
        return details::reflect_unchecked<type>(std::meta::make_unsigned(self.info));
    }

    consteval meta_result<std::vector<enumerator>> ability::enumeration::try_get_enumerators(this handle const &self) {
        return details::attempt<std::vector<enumerator>>(
            [&] { return details::handles_of<enumerator>(std::meta::enumerators_of(self.info)); },
            [&] { return details::unable(u8"get", details::call_of(u8"enumerators"), u8"from", self.info); });
    }

    consteval std::vector<enumerator> ability::enumeration::get_enumerators(this handle const &self) {
        return details::attach_ability_unchecked<enumeration>(self).try_get_enumerators().unwrap();
    }

    consteval meta_result<destructor> ability::record::try_get_destructor(this handle const &self,
                                                                           std::meta::access_context ctx) {
        std::vector<std::meta::info> members = std::meta::members_of(self.info, ctx);
        auto found = std::ranges::find_if(members, [](std::meta::info r) { return std::meta::is_destructor(r); });
        if (found != members.end()) {
            return details::reflect_unchecked<metalib::destructor>(*found);
        }
        return meta_result<metalib::destructor>::failure(
            u8"no destructor among the members of " + details::quoted(self.info) + u8" in this access context",
            self.info);
    }

    consteval destructor ability::record::get_destructor(this handle const &self, std::meta::access_context ctx) {
        return details::attach_ability_unchecked<record>(self).try_get_destructor(ctx).unwrap();
    }

    consteval std::vector<parameter> ability::callable::get_parameters(this handle const &self) {
        return details::handles_of<parameter>(std::meta::parameters_of(self.info));
    }

    consteval meta_result<type> ability::signature::try_get_return_type(this handle const &self) {
        return details::attempt<type>(
            [&] { return details::reflect_unchecked<type>(std::meta::return_type_of(self.info)); },
            [&] { return details::unable(u8"get", details::call_of(u8"return_type"), u8"from", self.info); });
    }

    consteval type ability::signature::get_return_type(this handle const &self) {
        return details::attach_ability_unchecked<signature>(self).try_get_return_type().unwrap();
    }

    consteval meta_result<object> ability::storage_duration::try_get_object(this handle const &self) {
        return details::attempt<metalib::object>(
            [&] { return details::reflect_unchecked<metalib::object>(std::meta::object_of(self.info)); },
            [&] { return details::unable(u8"get", details::call_of(u8"object"), u8"from", self.info); });
    }

    consteval object ability::storage_duration::get_object(this handle const &self) {
        return details::attach_ability_unchecked<storage_duration>(self).try_get_object().unwrap();
    }

    consteval namespace_ ability::namespace_alias_entity::get_target(this handle const &self) {
        return details::reflect_unchecked<namespace_>(std::meta::dealias(self.info));
    }

}
