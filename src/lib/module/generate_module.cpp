#include <metalib/metalib.hpp>

#include <algorithm>
#include <cstdio>
#include <meta>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace {
    consteval std::string qualified_name(std::meta::info scope) {
        std::string name;
        for (std::meta::info r = scope; r != ^^::; r = std::meta::parent_of(r)) {
            name = std::string(std::meta::identifier_of(r)) + (name.empty() ? "" : "::") + name;
        }
        return name;
    }

    consteval std::optional<bool> mark_of(std::meta::info r) {
        try {
            std::vector<std::meta::info> const marks = std::meta::annotations_of_with_type(r, ^^metalib::exported_t);
            if (!marks.empty()) {
                return std::meta::extract<metalib::exported_t>(marks.back()).value;
            }
        } catch (std::meta::exception const &) {
        }
        return std::nullopt;
    }

    consteval void add_exports(std::string &text, std::meta::info scope, bool exported) {
        std::string const name = qualified_name(scope);
        std::string declarations;
        std::vector<std::string_view> named;
        std::vector<std::pair<std::meta::info, bool>> nested;
        for (std::meta::info member: std::meta::members_of(scope, std::meta::access_context::unchecked())) {
            if (!std::meta::has_identifier(member) || std::meta::identifier_of(member).starts_with("__")) {
                continue;
            }
            std::string_view const identifier = std::meta::identifier_of(member);
            bool const member_exported = mark_of(member).value_or(exported);
            if (std::meta::is_namespace(member) && !std::meta::is_namespace_alias(member)) {
                nested.emplace_back(member, member_exported);
            } else if (member_exported && !std::ranges::contains(named, identifier)) {
                named.push_back(identifier);
                if (std::meta::is_namespace_alias(member)) {
                    declarations += "    namespace " + std::string(identifier) + " = ::"
                                    + qualified_name(std::meta::dealias(member)) + ";\n";
                } else {
                    declarations += "    using ::" + name + "::" + std::string(identifier) + ";\n";
                }
            }
        }
        if (!declarations.empty()) {
            text += "\nexport namespace " + name + " {\n" + declarations + "}\n";
        }
        for (auto const &[inner, inner_exported]: nested) {
            add_exports(text, inner, inner_exported);
        }
    }

    consteval std::string module_text() {
        std::string text = "module;\n\n#include <metalib/metalib.hpp>\n\nexport module metalib;\n";
        add_exports(text, ^^metalib, mark_of(^^metalib).value_or(false));
        text += "\n// names the specialization that collects from the built-in ability registry, so that it is\n"
                "// instantiated in this translation unit and importers use that instantiation\n"
                "namespace metalib::details {\n"
                "    consteval std::meta::info builtin_registry_kind(std::meta::info r) {\n"
                "        return collected_kind<::metalib::builtin_auto_ability_registry>(r);\n"
                "    }\n"
                "}\n";
        return text;
    }

    constexpr std::string_view text = std::define_static_string(module_text());
}

int main(int argc, char **argv) {
    if (argc != 2) {
        std::fputs("usage: generate_module <output file>\n", stderr);
        return 1;
    }
    std::FILE *const output = std::fopen(argv[1], "wb");
    if (output == nullptr) {
        std::fprintf(stderr, "cannot write %s\n", argv[1]);
        return 1;
    }
    bool const written = std::fwrite(text.data(), 1, text.size(), output) == text.size();
    return std::fclose(output) == 0 && written ? 0 : 1;
}
