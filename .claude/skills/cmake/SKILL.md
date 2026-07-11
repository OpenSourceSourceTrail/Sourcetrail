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
- Optional: LLVM/Clang 19.1.7 (for C/C++ indexing support)

## Conan setup (first time, Linux/GCC)

```bash
conan install . -s build_type=Release -of .conan/gcc/ -b missing -pr:a .conan/gcc/profile
```

## Configure & build

```bash
# Release build with C/C++ indexing support (preset from CMakeUserPresets.json)
cmake --preset=gnu_release_build_cxx
cd ../build/gnu_release_build_cxx
ninja
```

## Key CMake options

| Option | Default | Description |
|---|---|---|
| `BUILD_CXX_LANGUAGE_PACKAGE` | OFF | Enable C/C++ indexer via LLVM/Clang |
| `ENABLE_UNIT_TEST` | OFF | Build unit tests |
| `ENABLE_INTEGRATION_TEST` | OFF | Build integration tests |
| `ENABLE_SANITIZER_ADDRESS` | OFF | AddressSanitizer |
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
