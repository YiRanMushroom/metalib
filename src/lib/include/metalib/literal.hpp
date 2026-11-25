#pragma once

#include <compare>
#include <concepts>
#include <cstddef>
#include <meta>
#include <ranges>
#include <span>
#include <string_view>
#include <type_traits>

namespace metalib {
    template<typename T>
    struct array_literal : std::ranges::view_interface<array_literal<T>> {
        T const *pointer = nullptr;
        std::size_t length = 0;

        constexpr array_literal() = default;

        template<std::ranges::input_range R>
            requires (!std::same_as<std::remove_cvref_t<R>, array_literal>)
                     && std::convertible_to<std::ranges::range_reference_t<R>, T>
        consteval array_literal(R &&range) {
            std::span<T const> array = std::define_static_array(range);
            pointer = array.data();
            length = array.size();
        }

        constexpr T const *begin() const {
            return pointer;
        }

        constexpr T const *end() const {
            return pointer + length;
        }

        constexpr operator std::span<T const>() const {
            return {pointer, length};
        }
    };

    template<typename CharT>
    struct basic_string_literal {
        CharT const *pointer = null_terminator;
        std::size_t length = 0;

        static constexpr CharT null_terminator[1] = {};

        constexpr basic_string_literal() = default;

        consteval basic_string_literal(std::basic_string_view<CharT> text)
            : pointer(std::define_static_string(text)), length(text.size()) {
        }

        consteval basic_string_literal(CharT const *text) : basic_string_literal(std::basic_string_view<CharT>(text)) {
        }

        constexpr std::basic_string_view<CharT> view() const {
            return {pointer, length};
        }

        constexpr operator std::basic_string_view<CharT>() const {
            return view();
        }

        constexpr CharT const *c_str() const {
            return pointer;
        }

        constexpr CharT const *data() const {
            return pointer;
        }

        constexpr std::size_t size() const {
            return length;
        }

        constexpr bool empty() const {
            return length == 0;
        }

        constexpr CharT const *begin() const {
            return pointer;
        }

        constexpr CharT const *end() const {
            return pointer + length;
        }

        constexpr CharT operator[](std::size_t i) const {
            return pointer[i];
        }

        friend constexpr bool operator==(basic_string_literal a, std::basic_string_view<CharT> b) {
            return a.view() == b;
        }

        friend constexpr auto operator<=>(basic_string_literal a, std::basic_string_view<CharT> b) {
            return a.view() <=> b;
        }
    };

    using string_literal = basic_string_literal<char>;
    using u8string_literal = basic_string_literal<char8_t>;
}

template<typename T>
constexpr bool std::ranges::enable_borrowed_range<metalib::array_literal<T>> = true;

template<typename CharT>
constexpr bool std::ranges::enable_borrowed_range<metalib::basic_string_literal<CharT>> = true;
