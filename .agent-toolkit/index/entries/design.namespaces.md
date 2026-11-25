+++
title = "Namespaces: members, parents, kinds"
+++

Code: [[src/lib/include/metalib/handle.hpp]] (ability::namespace_entity, namespace_alias_entity, namespace_member_entity,
variable_entity, function_entity, linkage, storage_duration). Probe: scratchpad ns_probe.cpp; check: ns_check.cpp.
Verified on GCC trunk 2026-11-25.

members_of(namespace N) lists, all with parent_of == N:
- nested namespaces (named, inline, unnamed: no identifier, internal linkage) and namespace aliases;
- class/union/enum types and type aliases (typedef/using);
- variables (constexpr ones have internal linkage), including the unnamed variable behind a namespace-scope
  structured binding;
- functions, one entry per overload;
- class/function/variable/alias templates and concepts (left for the template round).
Not listed: friend functions, using-declarations, using-directives, static_assert, the structured bindings
themselves, enumerators of unscoped enums (parent is the enum), members of inline namespaces (parent is the
inline namespace; they are in members_of(inline ns)).
- The global namespace has no parent and no identifier.
- is_namespace(alias) is true; dealias(alias) is the target namespace. Annotations on namespaces work.
- GCC trunk: an extern "C" function in a namespace reports has_parent == false.

Kinds:
- namespace_ (not an alias; members<Kind = namespace_member>(ctx, filter, map)), namespace_alias (target() -> namespace_), namespace_member (default query kind),
  variable (namespace_member + typed, sized, linkage, storage_duration), function (namespace_member + typed,
  callable, signature, linkage). static_data_member also uses linkage + storage_duration.
- parent() is untyped (meta_result<handle>); the global namespace has no parent.
- metalib::global_namespace is namespace_{^^::}.
- Types in a namespace are queried with the type kinds: members<type>, members<class_type::with_ability<
  ability::non_alias>>, ...
