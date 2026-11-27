#pragma once

#include <metalib/literal.hpp>

namespace metalib::codegen::inline generation {
    namespace details {
        struct exported_t {
            bool value = true;

            constexpr exported_t operator()(bool exported) const {
                return {exported};
            }
        };

        struct inlined_t {};
    }

    inline constexpr details::exported_t exported{};
    inline constexpr details::inlined_t inlined{};

    struct export_module_name {
        enum placement { append, as_root };

        string_literal name;
        placement place = append;

        consteval export_module_name(string_literal name, placement place = append) : name(name), place(place) {
        }
    };

    struct inject {
        enum position { at_front, before_exports, in_place, at_back };
        enum propagation { none, up, down };

        position at = in_place;
        string_literal content;
        propagation propagate = none;
    };
}

namespace [[=metalib::codegen::exported, =metalib::codegen::export_module_name("metalib")]] metalib {
    inline namespace [[=codegen::inlined, =codegen::export_module_name("core")]] core {
        namespace [[=codegen::exported(false)]] details {
        }
    }

    namespace codegen {
        inline namespace [[=inlined, =export_module_name("codegen")]] generation {
            namespace [[=exported(false)]] details {
            }
        }
    }
}
