#pragma once

#include <algorithm>
#include <meta>
#include <optional>
#include <ranges>
#include <string_view>

namespace metalib::details {
    template<typename Char>
    consteval bool is_ascii(std::basic_string_view<Char> text) {
        return std::ranges::all_of(text, [](Char c) { return static_cast<unsigned>(c) < 0x80; });
    }

    consteval std::optional<std::string_view> u8_to_ascii(std::u8string_view text) {
        if (!is_ascii(text)) {
            return std::nullopt;
        }
        return std::string_view(
            std::define_static_string(text | std::views::transform([](char8_t c) { return static_cast<char>(c); })),
            text.size());
    }

    consteval std::optional<std::u8string_view> ascii_to_u8(std::string_view text) {
        if (!is_ascii(text)) {
            return std::nullopt;
        }
        return std::u8string_view(
            std::define_static_string(text | std::views::transform([](char c) { return static_cast<char8_t>(c); })),
            text.size());
    }

    consteval std::u8string_view static_u8(std::u8string_view text) {
        return std::u8string_view(std::define_static_string(text), text.size());
    }
}
