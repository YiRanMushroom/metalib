#pragma once

#include <functional>
#include <meta>
#include <metalib/handle.hpp>
#include <metalib/literal.hpp>

namespace metalib {
    template<typename F>
    struct basic_predicate;

    template<typename F>
    struct basic_transform;

    namespace predicate {
        template<typename F>
        consteval basic_predicate<F> from(F test) {
            return basic_predicate<F>{test};
        }
    }

    namespace transform {
        template<typename F>
        consteval basic_transform<F> from(F apply) {
            return basic_transform<F>{apply};
        }
    }

    template<typename F>
    struct basic_predicate {
        F test;

        template<typename T>
        consteval bool operator()(T const &value) const {
            return std::invoke(test, value);
        }

        template<typename G>
        consteval auto and_(G other) const {
            return predicate::from([f = test, g = other](auto const &value) {
                return std::invoke(f, value) && std::invoke(g, value);
            });
        }

        template<typename G>
        consteval auto or_(G other) const {
            return predicate::from([f = test, g = other](auto const &value) {
                return std::invoke(f, value) || std::invoke(g, value);
            });
        }

        consteval auto not_() const {
            return predicate::from([f = test](auto const &value) { return !std::invoke(f, value); });
        }

        template<typename G>
        friend consteval auto operator&&(basic_predicate a, basic_predicate<G> b) {
            return a.and_(b);
        }

        template<typename G>
        friend consteval auto operator||(basic_predicate a, basic_predicate<G> b) {
            return a.or_(b);
        }

        friend consteval auto operator!(basic_predicate a) {
            return a.not_();
        }
    };

    template<typename F>
    struct basic_transform {
        F apply;

        template<typename T>
        consteval auto operator()(T const &value) const {
            return std::invoke(apply, value);
        }

        template<typename G>
        consteval auto and_then(G next) const {
            return transform::from([f = apply, g = next](auto const &value) {
                return std::invoke(g, std::invoke(f, value));
            });
        }
    };

    namespace predicate {
        consteval auto of_name(string_literal name) {
            return from([name](auto const &value) {
                return details::identifier_is_ascii(value.info) && std::meta::identifier_of(value.info) == name.view();
            });
        }

        consteval auto of_operator(std::meta::operators op) {
            return from([op](auto const &value) {
                return std::meta::is_operator_function(value.info) && std::meta::operator_of(value.info) == op;
            });
        }

        consteval auto of_type(type t) {
            return from([t](auto const &value) { return value.type() == t; });
        }

        template<typename T>
        consteval auto of_type() {
            return of_type(type{^^T});
        }
    }
}
