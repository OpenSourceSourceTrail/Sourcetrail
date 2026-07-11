---
name: cpp20
description: C++ code style and conventions for Sourcetrail — clang-format, clang-tidy, cpplint rules, header conventions. Use before writing or editing any C++ source.
---

# C++ Style & Conventions

## Formatting & linting

- **clang-format**: Google-based style (`.clang-format`). CI enforces formatting on changed files using `clang-format-18`. Run it on every file you touch.
- **clang-tidy**: Configured in `.clang-tidy` with most checks enabled; run via CI (`clang_tidy.yml`).
- **cpplint**: Configured in `CPPLINT.cfg`; `src/lib/external/` is excluded.

## Conventions

- `#pragma once` preferred over include guards.
- Match the surrounding code's comment density, naming, and idiom.
- Vendored third-party code lives in `src/lib/external/` — never lint or reformat it.
- `src/lib/lib/` is Qt-free core business logic; do not introduce Qt types there. Qt code belongs in `src/lib/lib_gui/`.
- Keep proto conversion helpers (`src/lib/proto/Convert.{h,cpp}`) dependency-light — `lib` ↔ `proto_convert` had a circular dependency broken via an include-dir generator expression.
