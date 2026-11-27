#pragma once

namespace metalib {
    namespace details {
        struct exported_t {
            bool value = true;

            constexpr exported_t operator()(bool exported) const {
                return {exported};
            }
        };
    }

    inline constexpr details::exported_t exported{};
}

namespace [[=metalib::exported]] metalib {
    namespace [[=exported(false)]] details {
    }
}
