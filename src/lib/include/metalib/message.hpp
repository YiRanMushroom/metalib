#pragma once

#include <charconv>
#include <concepts>
#include <limits>
#include <meta>
#include <metalib/handle.hpp>
#include <string>
#include <string_view>

namespace metalib {
    consteval std::string to_string(handle const &h) {
        return std::string(std::meta::display_string_of(h.info));
    }

    consteval std::u8string to_u8string(handle const &h) {
        return std::u8string(std::meta::u8display_string_of(h.info));
    }

    namespace details {
        template<typename T>
        consteval void append_message_part(std::string &out, T const &part) {
            if constexpr (std::derived_from<T, handle>) {
                out += std::meta::display_string_of(part.info);
            } else if constexpr (std::same_as<T, bool>) {
                out += part ? "true" : "false";
            } else if constexpr (std::same_as<T, char>) {
                out += part;
            } else if constexpr (std::integral<T>) {
                char digits[std::numeric_limits<T>::digits10 + 3]{};
                out.append(digits, std::to_chars(digits, digits + sizeof digits, part).ptr);
            } else {
                out += std::string_view(part);
            }
        }
    }

    template<typename... Parts>
        requires ((std::derived_from<Parts, handle> || std::integral<Parts>
                   || std::convertible_to<Parts const &, std::string_view>) && ...)
    consteval std::string message(Parts const &...parts) {
        std::string out;
        (details::append_message_part(out, parts), ...);
        return out;
    }
}
