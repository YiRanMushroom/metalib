#pragma once

#include <concepts>
#include <exception>
#include <meta>
#include <metalib/encoding.hpp>
#include <optional>
#include <source_location>
#include <string_view>
#include <type_traits>
#include <utility>

namespace metalib {
    class meta_error {
    public:
        consteval meta_error(std::u8string_view message, std::meta::info from,
                             std::source_location where = std::source_location::current())
            : meta_error(message, {}, from, where) {
        }

        consteval meta_error(std::u8string_view message, std::u8string_view action, std::meta::info from,
                             std::source_location where = std::source_location::current())
            : m_u8message(details::static_u8(message)), m_u8action(details::static_u8(action)),
              m_message(details::u8_to_ascii(m_u8message)), m_from(from), m_where(where) {
        }

        constexpr std::u8string_view u8message() const noexcept {
            return m_u8message;
        }

        constexpr std::string_view message() const noexcept {
            return m_message.value_or("metalib::meta_error (message is not ASCII)");
        }

        constexpr std::u8string_view u8action() const noexcept {
            return m_u8action;
        }

        constexpr std::meta::info from() const noexcept {
            return m_from;
        }

        constexpr std::source_location where() const noexcept {
            return m_where;
        }

        [[noreturn]] constexpr void throw_() const;

    private:
        std::u8string_view m_u8message;
        std::u8string_view m_u8action;
        std::optional<std::string_view> m_message;
        std::meta::info m_from;
        std::source_location m_where;
    };

    class meta_exception : public std::exception {
    public:
        constexpr explicit meta_exception(meta_error const &error) : m_error(error) {
        }

        constexpr char const *what() const noexcept override {
            // message() views a null-terminated static string (define_static_string or a string literal).
            return m_error.message().data();
        }

        constexpr meta_error const &error() const noexcept {
            return m_error;
        }

    private:
        meta_error m_error;
    };

    constexpr void meta_error::throw_() const {
        throw meta_exception(*this);
    }

    template<typename T>
    class meta_result;

    namespace details {
        template<typename T>
        inline constexpr bool is_meta_result = false;

        template<typename T>
        inline constexpr bool is_meta_result<meta_result<T>> = true;
    }

    template<typename T>
    class [[nodiscard]] meta_result {
    public:
        template<typename U = std::remove_cv_t<T>>
            requires (!details::is_meta_result<std::remove_cvref_t<U>> &&
                      !std::same_as<std::remove_cvref_t<U>, meta_error> && std::constructible_from<T, U> &&
                      !std::reference_constructs_from_temporary_v<T, U>)
        constexpr explicit(!std::convertible_to<U, T>) meta_result(U &&value)
            : m_value(std::in_place, std::forward<U>(value)) {
        }

        constexpr meta_result(meta_error const &error) : m_error(error) {
        }

        static consteval meta_result failure(std::u8string_view message, std::meta::info from,
                                             std::source_location where = std::source_location::current()) {
            return meta_result(meta_error(message, from, where));
        }

        constexpr bool has_value() const noexcept {
            return m_value.has_value();
        }

        constexpr explicit operator bool() const noexcept {
            return has_value();
        }

        template<typename Self>
        constexpr T unwrap(this Self &&self) {
            if (!self.m_value) {
                self.m_error->throw_();
            }
            return *std::forward<Self>(self).m_value;
        }

        template<typename Self>
        constexpr T unwrap_or(this Self &&self, T fallback) {
            if (self.m_value) {
                return *std::forward<Self>(self).m_value;
            }
            return fallback;
        }

        constexpr meta_error const &error() const noexcept {
            return *m_error;
        }

        template<typename K>
            requires requires(T const &value) { value.template try_into<K>(); }
        consteval meta_result<K> try_into() const {
            if (!m_value) {
                return meta_result<K>(*m_error);
            }
            return m_value->template try_into<K>();
        }

        template<typename K>
            requires requires(T const &value) { value.template into<K>(); }
        consteval K into() const {
            return unwrap().template into<K>();
        }

    private:
        std::optional<T> m_value;
        std::optional<meta_error> m_error;
    };
}
