+++
title = "Templates: what reflection can tell, and the type-erased wrapper"
+++

Code: [[src/lib/include/metalib/handle.hpp]] (ability::template_entity, ability::templated, kinds template_,
member_template). Probes: scratchpad templates/ (probe1-5.cpp, *.out.txt, hard/results.txt); check:
template_check.cpp. GCC trunk snapshot 17-20261122, 2026-11-25.

Wrapper (simple translation, decided with the user):
- template_ = parented, identifiable, displayable, linkage, template_entity (check is_template: the is_*_template
  predicates, is_concept, operator_kind() via is_operator_function_template, can_substitute(handles),
  substitute(handles) -> meta_result<handle>, failing with "can_substitute returned false" when can_substitute is
  false). member_template = template_ + accessible + class_member (members<member_template>()).
- The substitute result is untyped; narrow with try_into (class template -> type, function templates -> function,
  variable template -> variable, alias template -> type alias, concept -> bool value).
- templated (has_template_arguments, template_() -> meta_result<template_>, template_arguments) is on type,
  function, member_function, static_member_function, variable, static_data_member.

What can be known:
- On the template: exact kind (predicates never throw; is_function/is_type/is_variable are false on templates),
  name (none for constructor / conversion / operator / generic-lambda call templates), display string (no
  defaults), source location, parent, member-ness, static-ness, access, linkage, identity, operator_of.
- Only on a specialization: signature (parameters_of, return_type_of), noexcept/explicit/const, annotations,
  members_of, template_of (always the primary template) and template_arguments_of (defaults filled in; type args
  are types, non-type args values, template template args the template; packs flattened).
- Not answerable: template parameter list (no template_parameters_of; only heuristic can_substitute probing),
  function template signature, deleted-ness (can_substitute false, ^^del<int> hard error), members of an
  unspecialized class template, partial/explicit specializations as entities (never listed), why a substitution
  failed, deducing-this templates vs other function templates (only after substitute).
- On a template, is_noexcept / is_deleted / is_explicit / is_const silently return false; type_of, parameters_of,
  return_type_of, annotations_of, template_arguments_of, members_of, size_of throw. Not wrapped on template_.
- Alias template specialization ^^AT<int> is a type alias with template arguments (not dealiased first); a plain
  alias of a specialization has none (dealias first).
- can_substitute is only safe for immediate-context failures: for function templates it instantiates the body (a
  broken body is a hard error); class templates are not instantiated until is_complete_type / members_of; variable
  templates fail later at extract.
- GCC ICE: template_arguments_of on a member function template specialization whose template was named by a
  qualified name (^^S::mf) crashes (reflect.cc:3933); the same template reached via members_of worked.
