#pragma once

#include <exception>
#include <meta>
#include <metalib/encoding.hpp>
#include <optional>
#include <source_location>
#include <string_view>
#include <utility>

#ifndef METALIB_HAS_META_EXCEPTION
#if defined(__clang__)
#define METALIB_HAS_META_EXCEPTION 0
#else
#define METALIB_HAS_META_EXCEPTION 1
#endif
#endif

namespace metalib {
    class meta_exception : public std::exception {
    public:
        consteval meta_exception(std::u8string_view what, std::meta::info from,
                                 std::source_location where = std::source_location::current())
            : m_u8what(details::static_u8(what)), m_what(details::u8_to_ascii(what)), m_from(from), m_where(where) {
        }

        consteval meta_exception(std::string_view what, std::meta::info from,
                                 std::source_location where = std::source_location::current())
            : m_u8what(details::ascii_to_u8(what).value_or(u8"metalib::meta_exception")),
              m_what(details::u8_to_ascii(m_u8what)), m_from(from), m_where(where) {
        }

        constexpr char const *what() const noexcept override {
            return m_what ? m_what->data() : "metalib::meta_exception";
        }

        constexpr std::u8string_view u8what() const noexcept {
            return m_u8what;
        }

        constexpr std::meta::info from() const noexcept {
            return m_from;
        }

        constexpr std::source_location where() const noexcept {
            return m_where;
        }

    private:
        std::u8string_view m_u8what;
        std::optional<std::string_view> m_what;
        std::meta::info m_from;
        std::source_location m_where;
    };

    template<typename T>
    class meta_result {
    public:
        constexpr meta_result(T value) : m_value(std::forward<T>(value)) {
        }

        constexpr meta_result(meta_exception error) : m_error(std::move(error)) {
        }

        static consteval meta_result failure(std::u8string_view what, std::meta::info from,
                                             std::source_location where = std::source_location::current()) {
            return meta_result(meta_exception(what, from, where));
        }

        constexpr bool has_value() const {
            return m_value.has_value();
        }

        constexpr explicit operator bool() const {
            return has_value();
        }

        constexpr T unwrap() const {
            if (!m_value) {
#if METALIB_HAS_META_EXCEPTION
                if consteval {
                    throw std::meta::exception(m_error->u8what(), m_error->from(), m_error->where());
                }
#endif
                throw *m_error;
            }
            return *m_value;
        }

        constexpr T unwrap_or(T fallback) const {
            return m_value ? *m_value : std::forward<T>(fallback);
        }

        constexpr meta_exception const &error() const {
            return *m_error;
        }

    private:
        std::optional<T> m_value;
        std::optional<meta_exception> m_error;
    };
}
