#pragma once

#include <meta>
#include <metalib/handle.hpp>
#include <metalib/literal.hpp>
#include <metalib/meta_result.hpp>
#include <type_traits>
#include <vector>

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
                return array_literal(Scope.members(cache_context(A)));
            } else {
                return array_literal(Scope.template members<Kind>(cache_context(A)));
            }
        }

        template<typename T>
        consteval meta_result<array_literal<T>> materialize(meta_result<std::vector<T>> const &result) {
            if (!result.has_value()) {
                return meta_result<array_literal<T>>(result.error());
            }
            return array_literal<T>(result.unwrap());
        }
    }

    namespace cache {
        template<auto Scope, typename Kind = void, access A = access::unprivileged>
        inline constexpr auto members = details::cached_members<Scope, Kind, A>();

        template<auto Class, access A = access::unprivileged>
        inline constexpr auto bases = array_literal(Class.bases(details::cache_context(A)));

        template<auto Class, access A = access::unprivileged>
        inline constexpr auto subobjects = array_literal(Class.subobjects(details::cache_context(A)));

        template<auto Enum>
        inline constexpr auto enumerators = array_literal(Enum.enumerators());

        template<auto Function>
        inline constexpr auto parameters = array_literal(Function.parameters());

        template<auto Specialization>
        inline constexpr auto template_arguments = details::materialize(Specialization.template_arguments());
    }
}
