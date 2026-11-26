#pragma once

#include <metalib/handle.hpp>

#include <algorithm>
#include <meta>
#include <vector>

namespace metalib {
    namespace details {
        consteval std::vector<std::meta::info> abilities_marked(std::meta::info marker) {
            std::vector<std::meta::info> found;
            for (std::meta::info member: std::meta::members_of(^^ability, std::meta::access_context::unchecked())) {
                if (std::meta::is_type(member) && !std::meta::is_type_alias(member)
                    && !std::meta::annotations_of_with_type(member, marker).empty()) {
                    found.push_back(member);
                }
            }
            std::ranges::sort(found, type_less);
            return found;
        }

        template<typename Marker>
        consteval std::meta::info deduced_kind(std::meta::info r) {
            std::vector<std::meta::info> held;
            template for (constexpr std::meta::info candidate:
                          std::define_static_array(abilities_marked(^^Marker))) {
                if (ability_holds<typename [:candidate:]>(r)) {
                    held.push_back(candidate);
                }
            }
            if (is_constant_like(r)) {
                std::meta::info const type = std::meta::remove_cv(std::meta::type_of(r));
                if (!std::meta::is_array_type(type)) {
                    std::erase(held, ^^ability::constant);
                    held.push_back(std::meta::substitute(^^ability::typed_as, {type}));
                    return canonical_interface(held);
                }
            }
            return std::meta::substitute(^^basic_interface, held);
        }
    }

    template<std::meta::info R>
    constexpr auto reflect =
        details::reflect_unchecked<typename [:details::deduced_kind<auto_ability_t>(R):]>(R);
}
