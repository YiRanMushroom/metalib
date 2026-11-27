#pragma once

#include <metalib/core.hpp>

#include <algorithm>
#include <cstddef>
#include <cstdio>
#include <filesystem>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace metalib::codegen {
    inline namespace generation {
        struct virtual_file {
            string_literal path;
            string_literal text;
        };

        namespace details {
            consteval std::string get_qualified_name(namespace_ scope) {
                std::string name(scope.get_identifier().view());
                for (handle parent = scope.get_parent(); parent.info != global_namespace.info;
                     parent = parent.into<namespace_>().get_parent()) {
                    name = std::string(parent.into<namespace_>().get_identifier().view()) + "::" + name;
                }
                return name;
            }

            // the marks of type Mark the entity carries; none for an entity that cannot carry annotations
            template<typename Mark>
            consteval std::vector<Mark> get_marks(handle entity) {
                using annotatable = basic_interface<ability::annotatable>;
                std::vector<Mark> marks;
                if (annotatable::holds(entity.info)) {
                    for (auto const &mark: entity.into<annotatable>().template get_annotations_with_type<Mark>()) {
                        marks.push_back(mark.extract());
                    }
                }
                return marks;
            }

            consteval std::optional<bool> find_export_mark(handle entity) {
                std::vector<details::exported_t> const marks = get_marks<details::exported_t>(entity);
                return marks.empty() ? std::nullopt : std::optional(marks.back().value);
            }

            struct module_draft {
                std::string name;
                std::string exports;
            };

            struct injection {
                std::string module;
                inject mark;
                std::string origin;
            };

            struct interface_draft {
                std::vector<module_draft> modules;
                std::vector<injection> injections;
            };

            // the module of that name, added when it is new
            consteval module_draft &get_module(interface_draft &draft, std::string_view name) {
                auto found = std::ranges::find(draft.modules, name, &module_draft::name);
                if (found == draft.modules.end()) {
                    draft.modules.push_back({std::string(name), {}});
                    return draft.modules.back();
                }
                return *found;
            }

            // "a.b" for "a.b.c"; empty for a name without a dot
            consteval std::string_view get_parent_name(std::string_view module) {
                std::size_t const dot = module.rfind('.');
                return dot == std::string_view::npos ? std::string_view() : module.substr(0, dot);
            }

            consteval bool is_inside(std::string_view module, std::string_view ancestor) {
                return module.size() > ancestor.size() && module.starts_with(ancestor)
                       && module[ancestor.size()] == '.';
            }

            consteval void add_injections(interface_draft &draft, std::string &declarations, std::string const &module,
                                          handle entity, std::string const &origin) {
                for (inject const &mark: get_marks<inject>(entity)) {
                    if (mark.at == inject::in_place) {
                        declarations += mark.content.view();
                    } else {
                        draft.injections.push_back({module, mark, origin});
                    }
                }
            }

            // the name the namespace is reopened with: each part marked inlined is written "inline part"; the first
            // part can be written so only when it is the whole name
            consteval std::string get_reopening_name(namespace_ scope) {
                auto const is_inlined = [](namespace_ part) { return !get_marks<details::inlined_t>(part).empty(); };
                auto const is_outermost = [](namespace_ part) {
                    return part.get_parent().info == global_namespace.info;
                };
                if (is_outermost(scope)) {
                    std::string const keyword = is_inlined(scope) ? "inline namespace " : "namespace ";
                    return keyword + std::string(scope.get_identifier().view());
                }
                std::string name;
                for (namespace_ part = scope;; part = part.get_parent().into<namespace_>()) {
                    std::string const spelled(part.get_identifier().view());
                    if (is_outermost(part)) {
                        return "namespace " + spelled + name;
                    }
                    name = (is_inlined(part) ? "::inline " : "::") + spelled + name;
                }
            }

            consteval void add_namespace(interface_draft &draft, namespace_ scope, bool exported,
                                         std::string const &enclosing) {
                std::string const name = get_qualified_name(scope);
                std::string const reopening = get_reopening_name(scope);
                std::string module = enclosing;
                std::vector<export_module_name> const names = get_marks<export_module_name>(scope);
                if (!names.empty()) {
                    export_module_name const mark = names.back();
                    module = std::string(mark.name.view());
                    if (!enclosing.empty() && mark.place == export_module_name::append) {
                        module = enclosing + "." + module;
                    }
                } else if (enclosing.empty()) {
                    std::u8string message = u8"namespace ";
                    message += metalib::details::quoted(scope.info);
                    message += u8" has no export_module_name";
                    meta_error(message, scope.info).throw_();
                }
                get_module(draft, module);

                std::string declarations;
                add_injections(draft, declarations, module, scope, "namespace " + name);
                std::vector<std::string_view> named;
                std::vector<std::pair<namespace_, bool>> nested;
                for (namespace_member member: scope.get_members()) {
                    if (!member.has_identifier() || member.get_identifier().view().starts_with("__")) {
                        continue;
                    }
                    std::string_view const identifier = member.get_identifier().view();
                    if (variable::holds(member.info) && !get_marks<inject>(member).empty()) {
                        add_injections(draft, declarations, module, member, name + "::" + std::string(identifier));
                        continue;
                    }
                    bool const member_exported = find_export_mark(member).value_or(exported);
                    if (namespace_::holds(member.info)) {
                        nested.emplace_back(member.into<namespace_>(), member_exported);
                    } else if (member_exported && !std::ranges::contains(named, identifier)) {
                        named.push_back(identifier);
                        if (namespace_alias::holds(member.info)) {
                            declarations += "    namespace " + std::string(identifier) + " = ::"
                                            + get_qualified_name(member.into<namespace_alias>().get_target()) + ";\n";
                        } else {
                            declarations += "    using ::" + name + "::" + std::string(identifier) + ";\n";
                        }
                    }
                }
                if (!declarations.empty()) {
                    get_module(draft, module).exports += "\nexport " + reopening + " {\n" + declarations + "}\n";
                }
                for (auto const &[inner, inner_exported]: nested) {
                    add_namespace(draft, inner, inner_exported, module);
                }
            }

            consteval std::string injected_text(interface_draft const &draft, std::string_view module,
                                                inject::position at) {
                std::string text;
                for (injection const &injected: draft.injections) {
                    bool const reaches =
                        injected.module == module
                        || (injected.mark.propagate == inject::down && is_inside(module, injected.module))
                        || (injected.mark.propagate == inject::up && is_inside(injected.module, module));
                    if (injected.mark.at == at && reaches) {
                        text += "\n// from " + injected.origin + "\n" + std::string(injected.mark.content.view());
                    }
                }
                return text;
            }
        }

        class module_generator {
        public:
            consteval module_generator &append_namespace(namespace_ library) {
                bool const exported = details::find_export_mark(library).value_or(false);
                details::add_namespace(m_draft, library, exported, {});
                return *this;
            }

            consteval std::vector<virtual_file> build() const {
                details::interface_draft draft = m_draft;
                for (std::size_t i = 0; i < draft.modules.size(); ++i) {
                    std::string const parent(details::get_parent_name(draft.modules[i].name));
                    if (!parent.empty()) {
                        details::get_module(draft, parent);
                    }
                }
                std::vector<virtual_file> files;
                for (details::module_draft const &drafted: draft.modules) {
                    std::string text = "module;\n";
                    text += details::injected_text(draft, drafted.name, inject::at_front);
                    text += "\nexport module " + drafted.name + ";\n";
                    std::string imports;
                    for (details::module_draft const &other: draft.modules) {
                        if (details::get_parent_name(other.name) == drafted.name) {
                            imports += "export import " + other.name + ";\n";
                        }
                    }
                    if (!imports.empty()) {
                        text += "\n" + imports;
                    }
                    text += details::injected_text(draft, drafted.name, inject::before_exports);
                    text += drafted.exports + details::injected_text(draft, drafted.name, inject::at_back);
                    std::string path = drafted.name + ".cppm";
                    std::ranges::replace(path.begin(), path.end() - 5, '.', '/');
                    files.push_back({string_literal(std::string_view(path)), string_literal(std::string_view(text))});
                }
                return files;
            }

        private:
            details::interface_draft m_draft;
        };

        inline bool write_directory(std::span<virtual_file const> files, std::filesystem::path const &root) {
            for (virtual_file const &file: files) {
                std::filesystem::path const path = root / file.path.view();
                std::filesystem::create_directories(path.parent_path());
                std::FILE *const output = std::fopen(path.string().c_str(), "wb");
                if (output == nullptr) {
                    return false;
                }
                bool const written = std::fwrite(file.text.data(), 1, file.text.size(), output) == file.text.size();
                if (std::fclose(output) != 0 || !written) {
                    return false;
                }
            }
            return true;
        }
    }

    inline namespace [[=inject{.at = inject::at_front,
                               .content = "#include <metalib/codegen/module_interface.hpp>\n"}]] generation {
    }
}
