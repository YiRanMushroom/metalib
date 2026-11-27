#pragma once

#include <metalib/core.hpp>

#include <algorithm>
#include <array>
#include <cstddef>
#include <meta>
#include <span>
#include <string>
#include <vector>

namespace metalib {
    inline namespace [[=codegen::inlined, =codegen::export_module_name("reflect")]] reflection {
        struct [[=codegen::exported(false)]] auto_ability_checker_t {
        };

        inline constexpr auto_ability_checker_t auto_ability_checker{};

        namespace [[=codegen::exported(false)]] builtin {
            namespace named_entities {
                using identifiable = ability::identifiable;
                using parented = ability::parented;
            }

            namespace class_members {
                [[=auto_ability_checker]] consteval bool check(std::meta::info r) {
                    return ability::class_member::check(r);
                }

                using class_member = ability::class_member;
                using accessible = ability::accessible;
            }

            namespace namespace_members {
                [[=auto_ability_checker]] consteval bool check(std::meta::info r) {
                    return ability::namespace_member_entity::check(r);
                }

                using namespace_member_entity = ability::namespace_member_entity;
            }

            namespace auto_ability_root {
                using displayable = ability::displayable;

                namespace types {
                    [[=auto_ability_checker]] consteval bool check(std::meta::info r) {
                        return ability::type_entity::check(r);
                    }

                    using type_entity = ability::type_entity;
                    using type_traits = ability::type_traits;
                    using type_transforms = ability::type_transforms;
                    using type_alias = ability::type_alias;
                    using non_alias = ability::non_alias;
                    using arithmetic = ability::arithmetic;
                    using enumeration = ability::enumeration;
                    using sized = ability::sized;
                    using templated = ability::templated;
                    using annotatable = ability::annotatable;
                    namespace named_entities = builtin::named_entities;
                    namespace class_members = builtin::class_members;
                    namespace namespace_members = builtin::namespace_members;

                    namespace class_and_union_types {
                        [[=auto_ability_checker]] consteval bool check(std::meta::info r) {
                            return std::meta::is_class_type(r) || std::meta::is_union_type(r);
                        }

                        using record = ability::record;
                        using class_record = ability::class_record;
                        using union_record = ability::union_record;
                    }
                }

                namespace variables {
                    [[=auto_ability_checker]] consteval bool check(std::meta::info r) {
                        return ability::variable_entity::check(r);
                    }

                    using variable_entity = ability::variable_entity;
                    using static_data = ability::static_data;
                    using typed = ability::typed;
                    using sized = ability::sized;
                    using templated = ability::templated;
                    using annotatable = ability::annotatable;
                    using linkage = ability::linkage;
                    using storage_duration = ability::storage_duration;
                    using constant = ability::constant;
                    namespace named_entities = builtin::named_entities;
                    namespace class_members = builtin::class_members;
                    namespace namespace_members = builtin::namespace_members;
                }

                namespace functions {
                    [[=auto_ability_checker]] consteval bool check(std::meta::info r) {
                        return ability::callable::check(r);
                    }

                    using callable = ability::callable;
                    using signature = ability::signature;
                    using function_entity = ability::function_entity;
                    using typed = ability::typed;
                    using templated = ability::templated;
                    using annotatable = ability::annotatable;
                    using linkage = ability::linkage;
                    using constant = ability::constant;
                    namespace named_entities = builtin::named_entities;
                    namespace namespace_members = builtin::namespace_members;

                    namespace member_functions {
                        [[=auto_ability_checker]] consteval bool check(std::meta::info r) {
                            return ability::class_member::check(r);
                        }

                        using class_member = ability::class_member;
                        using accessible = ability::accessible;
                        using nonstatic_function = ability::nonstatic_function;
                        using static_function = ability::static_function;
                        using constructor_entity = ability::constructor_entity;
                        using destructor_entity = ability::destructor_entity;
                        using virtual_specifier = ability::virtual_specifier;
                        using override_specifier = ability::override_specifier;
                        using explicit_specifier = ability::explicit_specifier;
                    }
                }

                namespace nonstatic_data_members {
                    [[=auto_ability_checker]] consteval bool check(std::meta::info r) {
                        return ability::nonstatic_data::check(r);
                    }

                    using nonstatic_data = ability::nonstatic_data;
                    using typed = ability::typed;
                    using sized = ability::sized;
                    using annotatable = ability::annotatable;
                    using constant = ability::constant;
                    namespace named_entities = builtin::named_entities;
                    namespace class_members = builtin::class_members;
                }

                namespace templates {
                    [[=auto_ability_checker]] consteval bool check(std::meta::info r) {
                        return ability::template_entity::check(r);
                    }

                    using template_entity = ability::template_entity;
                    using linkage = ability::linkage;
                    namespace named_entities = builtin::named_entities;
                    namespace class_members = builtin::class_members;
                    namespace namespace_members = builtin::namespace_members;
                }

                namespace namespaces {
                    [[=auto_ability_checker]] consteval bool check(std::meta::info r) {
                        return std::meta::is_namespace(r);
                    }

                    using namespace_entity = ability::namespace_entity;
                    using namespace_alias_entity = ability::namespace_alias_entity;
                    using annotatable = ability::annotatable;
                    namespace named_entities = builtin::named_entities;
                    namespace namespace_members = builtin::namespace_members;
                }

                namespace enumerators {
                    [[=auto_ability_checker]] consteval bool check(std::meta::info r) {
                        return ability::enumerator_entity::check(r);
                    }

                    using enumerator_entity = ability::enumerator_entity;
                    using typed = ability::typed;
                    using constant = ability::constant;
                    using annotatable = ability::annotatable;
                    namespace named_entities = builtin::named_entities;
                    namespace class_members = builtin::class_members;
                    namespace namespace_members = builtin::namespace_members;
                }

                namespace parameters {
                    [[=auto_ability_checker]] consteval bool check(std::meta::info r) {
                        return ability::parameter_entity::check(r);
                    }

                    using parameter_entity = ability::parameter_entity;
                    using typed = ability::typed;
                    using annotatable = ability::annotatable;
                    namespace named_entities = builtin::named_entities;
                }

                namespace bases {
                    [[=auto_ability_checker]] consteval bool check(std::meta::info r) {
                        return ability::base_entity::check(r);
                    }

                    using base_entity = ability::base_entity;
                    using typed = ability::typed;
                    using sized = ability::sized;
                    using annotatable = ability::annotatable;
                    using accessible = ability::accessible;
                    using virtual_specifier = ability::virtual_specifier;
                    using parented = ability::parented;
                }

                namespace structured_bindings {
                    [[=auto_ability_checker]] consteval bool check(std::meta::info r) {
                        return ability::structured_binding_entity::check(r);
                    }

                    using structured_binding_entity = ability::structured_binding_entity;
                    namespace named_entities = builtin::named_entities;
                    namespace namespace_members = builtin::namespace_members;
                }

                namespace data_member_descriptions {
                    [[=auto_ability_checker]] consteval bool check(std::meta::info r) {
                        return ability::data_member_description_entity::check(r);
                    }

                    using data_member_description_entity = ability::data_member_description_entity;
                    using identifiable = ability::identifiable;
                    using typed = ability::typed;
                    using sized = ability::sized;
                }

                namespace constants {
                    [[=auto_ability_checker]] consteval bool check(std::meta::info r) {
                        return details::is_constant_like(r);
                    }

                    using constant_entity = ability::constant_entity;
                    using value_entity = ability::value_entity;
                    using object_entity = ability::object_entity;
                    using annotation_entity = ability::annotation_entity;
                    using typed = ability::typed;
                    using sized = ability::sized;

                    namespace arrays {
                        [[=auto_ability_checker]] consteval bool check(std::meta::info r) {
                            return std::meta::is_array_type(std::meta::type_of(r));
                        }

                        using constant = ability::constant;
                    }

                    template<std::meta::info R>
                        requires (!std::meta::is_array_type(std::meta::type_of(R)))
                    using typed_as = ability::typed_as<typename [:std::meta::remove_cv(std::meta::type_of(R)):]>;
                }
            }
        }

        namespace [[=codegen::exported(false)]] reflect_details {
            using details::ability_holds;
            using details::canonical_interface;
            using details::quoted;
            using details::type_less;

            template<std::meta::info... Entries>
            struct built_registry;

            consteval bool is_built_registry(std::meta::info r) {
                return std::meta::is_type(r) && std::meta::has_template_arguments(r)
                       && std::meta::template_of(r) == ^^built_registry;
            }

            consteval std::meta::info build_registry(std::vector<std::meta::info> &path, std::meta::info scope) {
                if (auto const first = std::ranges::find(path, scope); first != path.end()) {
                    std::u8string message = u8"ability registry ";
                    message += quoted(path.front());
                    message += u8" has a cycle: ";
                    for (auto step = first; step != path.end(); ++step) {
                        message += quoted(*step);
                        message += u8" contains ";
                    }
                    message += quoted(scope);
                    meta_error(message, scope).throw_();
                }
                std::vector<std::meta::info> const members =
                    std::meta::members_of(scope, std::meta::access_context::unchecked());
                if (members.empty()) {
                    std::u8string message = u8"ability registry ";
                    message += quoted(scope);
                    message += u8" has no entries";
                    meta_error(message, scope).throw_();
                }
                path.push_back(scope);
                std::vector<std::meta::info> entries;
                for (std::meta::info member: members) {
                    if (std::meta::is_namespace(member)) {
                        entries.push_back(build_registry(path, std::meta::dealias(member)));
                    } else if (std::meta::is_type(member)) {
                        entries.push_back(std::meta::dealias(member));
                    } else if (std::meta::is_alias_template(member)) {
                        entries.push_back(member);
                    } else if (std::meta::is_function(member)) {
                        if (!std::meta::annotations_of_with_type(member, ^^auto_ability_checker_t).empty()) {
                            entries.push_back(member);
                        }
                    } else if (std::meta::is_variable(member)
                               && std::meta::remove_cv(std::meta::type_of(member))
                                      == std::meta::dealias(^^std::meta::info)) {
                        if (std::meta::info const held = std::meta::extract<std::meta::info>(member);
                            is_built_registry(held)) {
                            entries.push_back(held);
                        }
                    }
                }
                path.pop_back();
                for (std::meta::info &entry: entries) {
                    entry = std::meta::reflect_constant(entry);
                }
                return std::meta::substitute(^^built_registry, entries);
            }

            consteval std::vector<std::meta::info> registry_entries(std::meta::info registry) {
                if (!is_built_registry(registry)) {
                    std::u8string message = quoted(registry);
                    message += u8" is not an ability registry made by build_registry_from_namespace";
                    meta_error(message, registry).throw_();
                }
                std::vector<std::meta::info> entries;
                for (std::meta::info entry: std::meta::template_arguments_of(registry)) {
                    entries.push_back(std::meta::extract<std::meta::info>(entry));
                }
                return entries;
            }

            template<std::meta::info Registry>
            constexpr auto entries_of = std::define_static_array(registry_entries(Registry));

            consteval std::meta::info checker_among(std::span<std::meta::info const> entries) {
                auto const checker = std::ranges::find_if(entries, std::meta::is_function);
                return checker == entries.end() ? std::meta::info{} : *checker;
            }

            consteval void add_registered(std::vector<std::meta::info> &found, std::meta::info registry) {
                for (std::meta::info entry: registry_entries(registry)) {
                    if (is_built_registry(entry)) {
                        add_registered(found, entry);
                    } else if (std::meta::is_type(entry) && !std::ranges::contains(found, entry)) {
                        found.push_back(entry);
                    }
                }
            }

            consteval std::vector<std::meta::info> registered_in(std::meta::info registry) {
                std::vector<std::meta::info> found;
                add_registered(found, registry);
                std::ranges::sort(found, type_less);
                return found;
            }

            template<std::meta::info Registry>
            constexpr auto registered_abilities = std::define_static_array(registered_in(Registry));

            template<std::meta::info Part, std::meta::info Registry>
            consteval void collect_from(bool *present, std::vector<std::meta::info> &substituted, std::meta::info r) {
                constexpr std::meta::info checker = checker_among(entries_of<Part>);
                if constexpr (checker != std::meta::info{}) {
                    if (![:checker:](r)) {
                        return;
                    }
                }
                template for (constexpr std::meta::info entry: entries_of<Part>) {
                    if constexpr (is_built_registry(entry)) {
                        collect_from<entry, Registry>(present, substituted, r);
                    } else if constexpr (std::meta::is_type(entry)) {
                        if (ability_holds<typename [:entry:]>(r)) {
                            constexpr std::size_t index = std::ranges::find(registered_abilities<Registry>, entry)
                                                          - registered_abilities<Registry>.begin();
                            present[index] = true;
                        }
                    } else if constexpr (std::meta::is_alias_template(entry)) {
                        std::meta::info const argument = std::meta::reflect_constant(r);
                        if (std::meta::can_substitute(entry, {argument})) {
                            substituted.push_back(std::meta::dealias(std::meta::substitute(entry, {argument})));
                        }
                    }
                }
            }

            template<std::meta::info Registry>
            consteval std::meta::info collected_kind(std::meta::info r) {
                std::array<bool, registered_abilities<Registry>.size()> flags{};
                bool *const present = flags.data();
                std::vector<std::meta::info> substituted;
                collect_from<Registry, Registry>(present, substituted, r);
                std::vector<std::meta::info> abilities;
                abilities.reserve(flags.size());
                for (std::size_t i = 0; i < flags.size(); ++i) {
                    if (present[i]) {
                        abilities.push_back(registered_abilities<Registry>[i]);
                    }
                }
                if (substituted.empty()) {
                    return std::meta::substitute(^^basic_interface, abilities);
                }
                abilities.insert(abilities.end(), substituted.begin(), substituted.end());
                return canonical_interface(abilities);
            }
        }

        consteval std::meta::info build_registry_from_namespace(std::meta::info registry_namespace) {
            std::vector<std::meta::info> path;
            return reflect_details::build_registry(path, std::meta::dealias(registry_namespace));
        }

        inline constexpr std::meta::info builtin_auto_ability_registry =
            build_registry_from_namespace(^^builtin::auto_ability_root);

        template<std::meta::info R, std::meta::info Registry>
        constexpr auto collect_abilities =
            details::reflect_unchecked<typename [:reflect_details::collected_kind<Registry>(R):]>(R);

        template<std::meta::info R>
        constexpr auto reflect = collect_abilities<R, builtin_auto_ability_registry>;
    }

    inline namespace [[=codegen::inject{.at = codegen::inject::at_front,
                                        .content = "#include <metalib/reflect.hpp>\n"}]] reflection {
    }

    inline namespace [[=codegen::inject{
        .at = codegen::inject::at_back,
        .content = "template std::meta::info metalib::reflection::reflect_details::collected_kind<\n"
                   "    ::metalib::reflection::builtin_auto_ability_registry>(std::meta::info);\n"}]] reflection {
    }
}
