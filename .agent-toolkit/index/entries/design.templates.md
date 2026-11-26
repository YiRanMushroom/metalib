+++
title = "Templates: what reflection can tell, and the type-erased wrapper"
+++

Code: [[src/lib/include/metalib/handle.hpp]] (ability::template_entity, ability::templated, kinds template_,
member_template). Probes: scratchpad templates/ (probe1-5.cpp, *.out.txt, hard/results.txt); check:
template_check.cpp. GCC trunk snapshot 17-20261122, 2026-11-25.

Wrapper (simple translation, decided with the user):
- template_ = parented, identifiable, displayable, linkage, template_entity (check is_template: the is_*_template
  predicates, is_concept, get_operator_kind() via is_operator_function_template, can_substitute(handles),
  try_substitute(handles) -> meta_result<handle> (substitute unwraps): calls std::meta::substitute directly; failure message
  "unable to substitute(<argument display strings>) into '<template display string>'"; GCC displays a template as its
  head, e.g. "template<class T>  requires  sizeof (T) == 1 struct small_box", without default arguments).
- GCC substitutes with tf_none (eval_can_substitute / eval_substitute in reflect.cc), so no diagnostic exists for a
  failed substitution; the std::meta::exception only says "can_substitute returned false". The compiler's reason is
  visible only by writing the template-id itself as a constant splice. member_template = template_ + accessible + class_member (members<member_template>()).
- The substitute result is untyped; narrow with into / try_into (class template -> type, function templates -> function,
  variable template -> variable, alias template -> type alias, concept -> bool value).
- templated (has_template_arguments, try_get_template() -> meta_result<template_>, try_get_template_arguments(); the
  get_ forms unwrap) is on type,
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
  qualified name (^^S::mf) hits gcc_assert (args) in eval_template_arguments_of (reflect.cc, gcc-17-20261122).
  Cause (read from the source): a qualified member name is a BASELINK node; eval_substitute passes it to
  lookup_template_function, so the result is still wrapped in a BASELINK. eval_has_template_arguments and
  eval_template_of strip it with MAYBE_BASELINK_FUNCTIONS, eval_template_arguments_of does not, so
  get_template_innermost_arguments sees the BASELINK and returns null. A template reached via members_of is the
  bare TEMPLATE_DECL and works. Fix is one line (strip the BASELINK first): patches/gcc-reflect-template-arguments-of-baselink.patch.
  Applied to this machine's C:/mingw64-gcc-trunk on 2026-11-25 (cc1plus.exe rebuilt in ~/wgcc/b/native/gcc,
  23 s incremental; original kept as cc1plus.unpatched.exe); stock GCC trunk still crashes. Workaround without
  the patch: get member templates through members() / of_name, not ^^S::mf.
