#pragma once

#include <meta>
#include <metalib/handle.hpp>
#include <type_traits>

namespace metalib {
    namespace cache {
        enum class access {
            unprivileged,
            unchecked,
        };
    }

    namespace details {
        consteval std::meta::access_context cache_context(cache::access a) {
            return a == cache::access::unchecked ? std::meta::access_context::unchecked()
                                                 : std::meta::access_context::unprivileged();
        }

        template<auto Scope, typename Kind, cache::access A>
        consteval auto cached_members() {
            if constexpr (std::is_void_v<Kind>) {
                return Scope.members(cache_context(A));
            } else {
                return Scope.template members<Kind>(cache_context(A));
            }
        }
    }

    namespace cache {
        template<auto Scope, typename Kind = void, access A = access::unprivileged>
        inline constexpr auto members = details::cached_members<Scope, Kind, A>();

        template<auto Class, access A = access::unprivileged>
        inline constexpr auto bases = Class.bases(details::cache_context(A));

        template<auto Class, access A = access::unprivileged>
        inline constexpr auto subobjects = Class.subobjects(details::cache_context(A));

        template<auto Enum>
        inline constexpr auto enumerators = Enum.enumerators();

        template<auto Function>
        inline constexpr auto parameters = Function.parameters();

        template<auto Specialization>
        inline constexpr auto template_arguments = Specialization.template_arguments();
    }
}
