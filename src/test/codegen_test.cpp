#include <metalib/metalib.hpp>

#include <algorithm>
#include <meta>
#include <string>
#include <string_view>

namespace cg = metalib::codegen;

namespace [[=cg::exported, =cg::export_module_name("toy")]] toy {
    int value = 0;
    template<typename T> struct box {};
    namespace [[=cg::exported(false)]] hidden { int secret = 0; }
    namespace [[=cg::export_module_name("core")]] core {
        int inner = 0;
        namespace deep { int deeper = 0; }
    }
    namespace [[=cg::export_module_name("core")]] more_core { int extra = 0; }
    namespace [[=cg::export_module_name("toy_tools", cg::export_module_name::as_root)]] tools { int tool = 0; }
    namespace [[=cg::export_module_name("toy.plugins.audio", cg::export_module_name::as_root)]] audio {
        int volume = 0;
    }
    namespace short_core = core;
    [[=cg::inject{.content = "    using number = int;\n"}]] inline constexpr int number_alias = 0;
    inline namespace [[=cg::inlined, =cg::export_module_name("versioned")]] v1 {
        int stable = 0;
        inline namespace [[=cg::inlined]] patch { int fixed = 0; }
    }
}
namespace toy {
    inline namespace [[=cg::inject{.at = cg::inject::before_exports, .content = "import toy_tools;\n"}]] v1 {
    }
}
namespace [[=cg::inject{.at = cg::inject::at_front,
                        .content = "#include <toy.hpp>\n",
                        .propagate = cg::inject::down}]] toy {
}
namespace [[=cg::inject{.at = cg::inject::at_back, .content = "// end of toy\n"}]] toy {
}
namespace toy {
    namespace [[=cg::inject{.at = cg::inject::at_back,
                            .content = "// end of core\n",
                            .propagate = cg::inject::up}]] core {
    }
}
namespace [[=cg::exported, =cg::export_module_name("toy.extras")]] toy_extras { int bonus = 0; }
namespace unnamed_library { int x = 0; }

constexpr auto files = std::define_static_array(cg::module_generator()
                                                    .append_namespace(metalib::reflect<^^toy>)
                                                    .append_namespace(metalib::reflect<^^toy_extras>)
                                                    .build());

consteval std::string_view text_of(std::string_view path) {
    auto const file = std::ranges::find(files, path, [](cg::virtual_file const &f) { return f.path.view(); });
    return file == files.end() ? std::string_view() : file->text.view();
}

// toy, toy.core (two namespaces), toy_tools, toy.plugins.audio, toy.versioned, toy.extras, and toy.plugins, which no
// namespace names
static_assert(files.size() == 7);

constexpr std::string_view toy_text = text_of("toy.cppm");
static_assert(toy_text.starts_with(
    "module;\n"
    "\n"
    "// from namespace toy\n"
    "#include <toy.hpp>\n"
    "\n"
    "export module toy;\n"
    "\n"
    "export import toy.core;\n"
    "export import toy.versioned;\n"
    "export import toy.extras;\n"
    "export import toy.plugins;\n"
    "\n"
    "export namespace toy {\n"));
static_assert(toy_text.contains("    using ::toy::value;\n") && toy_text.contains("    using ::toy::box;\n") &&
              toy_text.contains("    namespace short_core = ::toy::core;\n") &&
              toy_text.contains("    using number = int;\n"));
static_assert(!toy_text.contains("secret") && !toy_text.contains("number_alias") && !toy_text.contains("inner") &&
              !toy_text.contains("toy_tools") && !toy_text.contains("audio") && !toy_text.contains("bonus"));
static_assert(toy_text.ends_with(
    "}\n"
    "\n"
    "// from namespace toy\n"
    "// end of toy\n"
    "\n"
    "// from namespace toy::core\n"
    "// end of core\n"));

static_assert(text_of("toy/core.cppm") ==
    "module;\n"
    "\n"
    "// from namespace toy\n"
    "#include <toy.hpp>\n"
    "\n"
    "export module toy.core;\n"
    "\n"
    "export namespace toy::core {\n"
    "    using ::toy::core::inner;\n"
    "}\n"
    "\n"
    "export namespace toy::core::deep {\n"
    "    using ::toy::core::deep::deeper;\n"
    "}\n"
    "\n"
    "export namespace toy::more_core {\n"
    "    using ::toy::more_core::extra;\n"
    "}\n"
    "\n"
    "// from namespace toy::core\n"
    "// end of core\n");

static_assert(text_of("toy_tools.cppm") ==
    "module;\n"
    "\n"
    "export module toy_tools;\n"
    "\n"
    "export namespace toy::tools {\n"
    "    using ::toy::tools::tool;\n"
    "}\n");

static_assert(text_of("toy/plugins.cppm") ==
    "module;\n"
    "\n"
    "// from namespace toy\n"
    "#include <toy.hpp>\n"
    "\n"
    "export module toy.plugins;\n"
    "\n"
    "export import toy.plugins.audio;\n");

static_assert(text_of("toy/plugins/audio.cppm") ==
    "module;\n"
    "\n"
    "// from namespace toy\n"
    "#include <toy.hpp>\n"
    "\n"
    "export module toy.plugins.audio;\n"
    "\n"
    "export namespace toy::audio {\n"
    "    using ::toy::audio::volume;\n"
    "}\n");

static_assert(text_of("toy/extras.cppm") ==
    "module;\n"
    "\n"
    "// from namespace toy\n"
    "#include <toy.hpp>\n"
    "\n"
    "export module toy.extras;\n"
    "\n"
    "export namespace toy_extras {\n"
    "    using ::toy_extras::bonus;\n"
    "}\n");

static_assert(text_of("toy/versioned.cppm") ==
    "module;\n"
    "\n"
    "// from namespace toy\n"
    "#include <toy.hpp>\n"
    "\n"
    "export module toy.versioned;\n"
    "\n"
    "// from namespace toy::v1\n"
    "import toy_tools;\n"
    "\n"
    "export namespace toy::inline v1 {\n"
    "    using ::toy::v1::stable;\n"
    "}\n"
    "\n"
    "export namespace toy::inline v1::inline patch {\n"
    "    using ::toy::v1::patch::fixed;\n"
    "}\n");

consteval std::u8string missing_name_message() {
    try {
        cg::module_generator().append_namespace(metalib::reflect<^^unnamed_library>);
    } catch (metalib::meta_exception const &failure) {
        return std::u8string(failure.error().u8message());
    }
    return {};
}
static_assert(missing_name_message() == u8"namespace 'unnamed_library' has no export_module_name");

constexpr auto library_files =
    std::define_static_array(cg::module_generator().append_namespace(metalib::reflect<^^metalib>).build());

consteval std::string_view library_text_of(std::string_view path) {
    auto const file = std::ranges::find(library_files, path, [](cg::virtual_file const &f) { return f.path.view(); });
    return file == library_files.end() ? std::string_view() : file->text.view();
}

static_assert(library_files.size() == 4);
static_assert(library_text_of("metalib.cppm").starts_with("module;\n\nexport module metalib;\n\nexport import ") &&
              library_text_of("metalib.cppm").contains("export import metalib.core;\n") &&
              library_text_of("metalib.cppm").contains("export import metalib.reflect;\n") &&
              library_text_of("metalib.cppm").contains("export import metalib.codegen;\n") &&
              !library_text_of("metalib.cppm").contains("namespace"));
static_assert(library_text_of("metalib/core.cppm").contains("#include <metalib/core.hpp>\n") &&
              library_text_of("metalib/core.cppm").contains("export module metalib.core;\n") &&
              library_text_of("metalib/core.cppm").contains("export namespace metalib::inline core {\n") &&
              library_text_of("metalib/core.cppm").contains("export namespace metalib::inline core::ability {\n") &&
              !library_text_of("metalib/core.cppm").contains("reflect.hpp") &&
              !library_text_of("metalib/core.cppm").contains("codegen") &&
              !library_text_of("metalib/core.cppm").contains("details"));
static_assert(library_text_of("metalib/reflect.cppm").contains("#include <metalib/reflect.hpp>\n") &&
              library_text_of("metalib/reflect.cppm").contains("export namespace metalib::inline reflection {\n") &&
              library_text_of("metalib/reflect.cppm").contains("    using ::metalib::reflection::reflect;\n") &&
              !library_text_of("metalib/reflect.cppm").contains("builtin::"));
static_assert(library_text_of("metalib/codegen.cppm").contains("#include <metalib/codegen/module_interface.hpp>\n") &&
              library_text_of("metalib/codegen.cppm").contains(
                  "export namespace metalib::codegen::inline generation {\n") &&
              library_text_of("metalib/codegen.cppm").contains(
                  "    using ::metalib::codegen::generation::module_generator;\n") &&
              library_text_of("metalib/codegen.cppm").contains(
                  "    using ::metalib::codegen::generation::exported;\n") &&
              !library_text_of("metalib/codegen.cppm").contains("details"));

int main() {
}
