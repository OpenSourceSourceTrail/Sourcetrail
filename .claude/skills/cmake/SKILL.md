---
name: cmake
description: Build Sourcetrail — Conan 2 setup, CMake presets, key options, add_sourcetrail_library/add_sourcetrail_test conventions. Use before touching any CMakeLists.txt, adding a module, or building the project.
---

# Building Sourcetrail

## Prerequisites

- CMake ≥ 3.23, Ninja, Git
- Conan 2 (`pip install conan`)
- Qt 6.8.2
- gRPC 1.54.3 + Protobuf (Conan; used for cross-process IPC)
- Optional: LLVM/Clang 22.1.8 (for C/C++ indexing support; `scripts/build_llvm_conan.sh`)

## Conan setup (first time, Linux/GCC)

One install only — GCC, Release. Debug builds reuse it; never run a `build_type=Debug`
install (`scripts/run_conan.sh` does exactly this one command).

```bash
conan install . -s build_type=Release -of .conan/gcc/ -b missing -pr:a .conan/gcc/profile
```

LLVM/Clang for the C/C++ indexer is a separate Conan package built from the in-repo recipe
`.conan/recipes/llvm-clang/` (upstream flags: `LLVM_ENABLE_PROJECTS=clang`,
`LLVM_ENABLE_RTTI=ON`, `CLANG_LINK_CLANG_DYLIB=ON`, `LLVM_LINK_LLVM_DYLIB=ON`,
`LLVM_TARGETS_TO_BUILD=host`). It installs into `.conan/llvm/` — never into `.conan/gcc/` —
and symlinks `external/` at the package so the `build_cxx` presets need no `Clang_DIR`:

```bash
./scripts/build_llvm_conan.sh   # first run builds LLVM from source (hours)
```

## Configure & build

```bash
# Release build with C/C++ indexing support -> build/
cmake --preset=ci_gnu_release_build_cxx
cmake --build build

# Debug build against the same Release Conan deps -> build-debug/
cmake --preset=gnu_debug
cmake --build build-debug
```

## Key CMake options

| Option | Default | Description |
|---|---|---|
| `BUILD_CXX_LANGUAGE_PACKAGE` | OFF | Enable C/C++ indexer via LLVM/Clang |
| `ENABLE_UNIT_TEST` | OFF | Build unit tests |
| `ENABLE_INTEGRATION_TEST` | OFF | Build integration tests |
| `SR_SAN` | "" | Comma-separated sanitizers applied build-wide: `address`, `undefined`, `thread`, `memory` (GNU+Clang; `memory` is Clang-only) |
| `SOURCETRAIL_WARNING_AS_ERROR` | OFF | Treat warnings as errors |
| `USE_ALTERNATE_LINKER` | "" | Use `mold`, `lld`, `gold`, or `bfd` |

## Module conventions

New modules use the `add_sourcetrail_library()` macro (defined in `cmake/add_sourcetrail_library.cmake`):

```cmake
add_sourcetrail_library(
  NAME core::utility::MyUtil
  SOURCES MyUtil.cpp
  PUBLIC_HEADERS MyUtil.h
  PUBLIC_DEPS fmt::fmt
  PRIVATE_DEPS spdlog::spdlog
)
```

This creates target `Sourcetrail_core_utility_MyUtil` with alias `Sourcetrail::core::utility::MyUtil`.

Tests use `add_sourcetrail_test()` (see `cmake/add_sourcetrail_test.cmake`).

**cmake-format** is enforced in CI via `.cmake-format.yaml` — keep CMake files formatted.
