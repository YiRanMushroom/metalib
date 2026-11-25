+++
title = "Naming convention: private data members"
+++

- Private data members are named `m_member` (e.g. `m_value`, `m_error`), never `member_`.
- Public data members keep plain names (e.g. `handle::info`).
- Applied in [[src/lib/include/metalib/meta_result.hpp]] (meta_exception, meta_result).
