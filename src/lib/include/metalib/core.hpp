#pragma once

#include <metalib/codegen/marks.hpp>

#include <metalib/cache.hpp>
#include <metalib/handle.hpp>
#include <metalib/literal.hpp>
#include <metalib/message.hpp>
#include <metalib/meta_result.hpp>
#include <metalib/query.hpp>

namespace metalib {
    inline namespace [[=codegen::inject{.at = codegen::inject::at_front,
                                        .content = "#include <metalib/core.hpp>\n"}]] core {
    }
}
