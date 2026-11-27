#pragma once

#include <functional>
#include <meta>
#include <metalib/handle.hpp>
#include <metalib/literal.hpp>
#include <utility>

namespace metalib::inline core {
    template<typename F>
    struct basic_predicate;

    template<typename F>
    struct basic_transform;

    namespace predicate {
        template<typename F>
        consteval basic_predicate<F> from(F test) {
            return basic_predicate<F>{std::move(test)};
        }
    }

    namespace transform {
        template<typename F>
        consteval basic_transform<F> from(F apply) {
            return basic_transform<F>{std::move(apply)};
        }
    }

    template<typename F>
    struct basic_predicate {
        F test;

        template<typename T>
        consteval bool operator()(T const &candidate) const {
            return std::invoke(test, candidate);
        }

        template<typename G>
        consteval auto and_(G other) const {
            return predicate::from([f = test, g = std::move(other)](auto const &candidate) {
                return std::invoke(f, candidate) && std::invoke(g, candidate);
            });
        }

        template<typename G>
        consteval auto or_(G other) const {
            return predicate::from([f = test, g = std::move(other)](auto const &candidate) {
                return std::invoke(f, candidate) || std::invoke(g, candidate);
            });
        }

        consteval auto not_() const {
            return predicate::from([f = test](auto const &candidate) { return !std::invoke(f, candidate); });
        }

        template<typename G>
        friend consteval auto operator&&(basic_predicate a, basic_predicate<G> b) {
            return a.and_(std::move(b));
        }

        template<typename G>
        friend consteval auto operator||(basic_predicate a, basic_predicate<G> b) {
            return a.or_(std::move(b));
        }

        friend consteval auto operator!(basic_predicate a) {
            return a.not_();
        }
    };

    template<typename F>
    struct basic_transform {
        F apply;

        template<typename T>
        consteval auto operator()(T const &candidate) const {
            return std::invoke(apply, candidate);
        }

        template<typename G>
        consteval auto and_then(G next) const {
            return transform::from([f = apply, g = std::move(next)](auto const &candidate) {
                return std::invoke(g, std::invoke(f, candidate));
            });
        }
    };

    namespace predicate {
        consteval auto of_name(string_literal name) {
            return from([name](auto const &candidate) {
                return details::identifier_is_ascii(candidate.info) &&
                       std::meta::identifier_of(candidate.info) == name.view();
            });
        }

        consteval auto of_operator(std::meta::operators op) {
            return from([op](auto const &candidate) {
                return std::meta::is_operator_function(candidate.info) && std::meta::operator_of(candidate.info) == op;
            });
        }

        consteval auto of_type(type t) {
            return from([t](auto const &candidate) { return candidate.get_type() == t; });
        }

        template<typename T>
        consteval auto of_type() {
            return of_type(details::reflect_unchecked<type>(^^T));
        }
    }
}
