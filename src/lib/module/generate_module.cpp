#include <metalib/metalib.hpp>

#include <cstdio>
#include <meta>

namespace {
    constexpr auto module_interfaces = std::define_static_array(
        metalib::codegen::module_generator().append_namespace(metalib::reflect<^^metalib>).build());
}

int main(int argc, char **argv) {
    if (argc != 2) {
        std::fputs("usage: generate_module <output directory>\n", stderr);
        return 1;
    }
    if (!metalib::codegen::write_directory(module_interfaces, argv[1])) {
        std::fprintf(stderr, "cannot write the module interfaces to %s\n", argv[1]);
        return 1;
    }
    return 0;
}
