#pragma once

#include <meta>
#include <optional>
#include <string>
#include <string_view>

namespace metalib::details {
    template<typename Char>
    consteval bool is_ascii(std::basic_string_view<Char> text) {
        for (Char c: text) {
            if (static_cast<unsigned>(c) >= 0x80) {
                return false;
            }
        }
        return true;
    }

    consteval std::optional<std::string_view> u8_to_ascii(std::u8string_view text) {
        if (!is_ascii(text)) {
            return std::nullopt;
        }
        std::string converted;
        for (char8_t c: text) {
            converted.push_back(static_cast<char>(c));
        }
        return std::string_view(std::define_static_string(converted), text.size());
    }

    consteval std::optional<std::u8string_view> ascii_to_u8(std::string_view text) {
        if (!is_ascii(text)) {
            return std::nullopt;
        }
        std::u8string converted;
        for (char c: text) {
            converted.push_back(static_cast<char8_t>(c));
        }
        return std::u8string_view(std::define_static_string(converted), text.size());
    }

    consteval std::u8string_view static_u8(std::u8string_view text) {
        return std::u8string_view(std::define_static_string(text), text.size());
    }
}
