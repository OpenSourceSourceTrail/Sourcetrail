# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

Sourcetrail is a free, offline, cross-platform (Windows/Linux) C/C++ source explorer built on Qt6. It indexes source code (via Clang/LibTooling when `BUILD_CXX_LANGUAGE_PACKAGE` is enabled) into a SQLite-backed graph database and lets users interactively explore symbols, references, and call graphs.

It is **not** a single process: a Qt GUI, a headless engine daemon that owns the database, and per-job indexer workers. The GUI talks to the engine over **HTTP + JSON** (so a web app can speak it too); the engine talks to indexer workers over gRPC. See *Process model* below before making any change that crosses those boundaries.

## Build System

CMake (>= 3.23) + Conan 2 + Ninja. C++20 / C17. Qt 6.10 (CI pins 6.10.3; 6.11 works on Linux but
aqtinstall 3.3.0 cannot resolve it on Windows, so CI stays on 6.10).

The tracked presets are the CI ones — `ci_gnu_release`, `ci_clang_release`, `ci_msvc_release`, each with a `_build_cxx` variant that additionally enables the C/C++ indexer — plus `gnu_debug` for local Linux Debug builds. All of them enable unit + GUI + integration tests (the hidden `all-tests` fragment). The `ci_*` presets configure into `<repo>/build/`; `gnu_debug` uses `<repo>/build-debug/` so the two trees never invalidate each other. Use `CMakeUserPresets.json` for local, uncommitted tweaks (inherit from the tracked presets rather than copying them).

### Linux

Linux needs **exactly one** Conan install — GCC, Release. Every local configuration, Debug
included, reuses that dependency set; never run `conan install -s build_type=Debug`.

```
conan install . -s build_type=Release -of .conan/gcc/ -b missing -pr:a .conan/gcc/profile
cmake --preset=ci_gnu_release
cmake --build build
```

For a Debug build of the application against those same Release dependencies:
```
cmake --preset=gnu_debug
cmake --build build-debug
```
`gnu_debug` points at the Release `conan_toolchain.cmake` and sets
`CMAKE_MAP_IMPORTED_CONFIG_DEBUG=Release;RelWithDebInfo;` so the Conan imported targets
resolve. `scripts/run_conan.sh` performs the one install.

### Windows (MSVC)
```
conan install . --build missing -s build_type=Release -s compiler.cppstd=20 -c tools.cmake.cmaketoolchain:generator=Ninja -of .conan/msvc/
cmake --preset=ci_msvc_release
cmake --build build
```

### Enabling C/C++ language support (the indexer)

Requires LLVM/Clang 22 or newer (developed against 22.1.8) built with `-DLLVM_ENABLE_PROJECTS=clang -DLLVM_ENABLE_RTTI=ON` (plus `-DCLANG_LINK_CLANG_DYLIB=ON -DLLVM_LINK_LLVM_DYLIB=ON` on Unix).

On Linux, `scripts/build_llvm_conan.sh` produces that build through Conan from the recipe in `.conan/recipes/llvm-clang/`, then symlinks `<repo>/external` at the resulting package:
```
./scripts/build_llvm_conan.sh
cmake --preset=ci_gnu_release_build_cxx
```
That is a **second, separate** `conan install` (into `.conan/llvm/`); it deliberately stays out of the unified GCC/Release graph in `.conan/gcc/` so the main dependency set and its package IDs are unaffected. The first run compiles LLVM from source and takes hours; later runs are cache hits.

To skip that first build, restore the package CI publishes — `.github/workflows/llvm.yml` builds it once and uploads it as a GitHub Release asset on the `llvm-clang-22.1.8` tag:
```
gh release download llvm-clang-22.1.8 -p 'llvm-clang-22.1.8-linux-x86_64.tgz'
conan cache restore llvm-clang-22.1.8-linux-x86_64.tgz
./scripts/build_llvm_conan.sh   # now a cache hit; still makes the external/ symlink
```

With a hand-built LLVM, point at it instead:
```
cmake --preset=ci_gnu_release_build_cxx -DClang_DIR=<path/to/llvm_build>/lib/cmake/clang
```
The `build_cxx` presets default `Clang_DIR` to `<repo>/external/lib/cmake/clang/`. This turns on `src/lib/lib_cxx` and installs the built-in C/C++ indexer plugin manifest.

### Running

Everything lands in `build/app/`, alongside the `data`, `user` and `plugins` directories CMake symlinks/generates there. Run `build/app/Sourcetrail` from that directory — it spawns `sourcetrail_engine` itself.

### Key CMake options (see root `CMakeLists.txt`)
- `BUILD_CXX_LANGUAGE_PACKAGE`, `BUILD_JAVA_INDEXER` (auto-on when `mvn` is on `PATH`), `BUILD_DOC`
- `ENABLE_UNIT_TEST` / `ENABLE_GUI_TEST` / `ENABLE_INTEGRATION_TEST`
- `SR_SAN` — comma-separated sanitizers applied build-wide (`address`, `undefined`, `thread`, `memory`; `memory` is Clang-only, GNU+Clang otherwise)
- `ENABLE_COVERAGE` (run with `ninja coverage`)
- `SOURCETRAIL_WARNING_AS_ERROR`, `SOURCETRAIL_USE_LIBCPP`, `USE_ALTERNATE_LINKER`

## Tests

GTest/GMock through CTest.
- **Unit tests** (`ENABLE_UNIT_TEST`): `src/lib/lib/tests`, `src/lib/lib_gui/tests`, `src/lib/client/tests`, `src/lib/core/tests`, `src/lib/messaging/tests`, `src/lib/lib_cxx/tests` (only with `BUILD_CXX_LANGUAGE_PACKAGE=ON`).
- **Integration tests** (`ENABLE_INTEGRATION_TEST`): `tests/integration/{lib,lib_cxx,messaging}` — real SQLite round-trips, `MessageQueue`.
- **GUI tests** (`ENABLE_GUI_TEST`): `src/lib/lib_gui/tests/gui`.

```
ctest --test-dir build
ctest --test-dir build -R "unittests\.lib\."
```
Registered prefixes: `unittests.lib.`, `unittests.lib_gui.`, `unittests.client.`, `unittests.core.`, `integration.lib.`, `integration.lib_cxx.`, `integration.messaging`.

New test targets go through the `add_sourcetrail_test()` helper (`cmake/add_sourcetrail_test.cmake`), not raw `add_executable` — it wires up `Sourcetrail::gtest_main` and `gtest_discover_tests`; `SR_SAN` applies sanitizer flags build-wide, not per-target. Copy the calling convention from `src/lib/lib_gui/tests/CMakeLists.txt`.

## Formatting / Linting

- `scripts/run_clang_format.sh` — formats all C/C++ sources in place (`.clang-format`, Google fallback style).
- `scripts/run_cmake_format.sh` — formats all `CMakeLists.txt`/`*.cmake` (`.cmake-format.yaml`).
- `.clang-tidy` is enforced via `cmake/clang-tidy.cmake`; `cmake/cppcheck.cmake` wires up cppcheck. Both run in CI (`.github/workflows/clang_tidy.yml`, `cppcheck.yaml`).
- `.pre-commit-config.yaml` runs clang-format 18.1.8 and cmakelang 0.6.13 (pinned to match CI) on staged files; run `pip install pre-commit && pre-commit install` once per clone to activate it.

## High-Level Architecture

### Process model

| Binary (`build/app/`) | Source | Role |
| --- | --- | --- |
| `Sourcetrail` | `src/app/gui` | Qt GUI. **Owns no database** — reads everything through the engine. |
| `sourcetrail_engine` | `src/app/engine` | Headless HTTP daemon (`EngineHttpService`). Owns the SQLite index, the project state machine and the indexing task graph. |
| `sourcetrail_indexer` | `src/app/indexer` | Indexer worker process, spawned per indexing job. |
| `Sourcetrail_cli` | `src/app/cli` | Headless, Qt-free front end. |

`QtEngineSupervisor` (`src/lib/lib_gui`) spawns the engine with `--port 0`; the engine prints
`ENGINE_PORT <n> <token>` on its first stdout line, so instances can run side by side. The token is a
per-run bearer credential every request must present: a loopback HTTP port is reachable by any local
process and by any page the user's browser loads, which the gRPC port it replaced was not. Requests
carrying an un-allow-listed `Origin` are refused outright. A dead engine is respawned with
exponential backoff (max 5 attempts); while it is down, `HttpStorageAccess` returns empty results
instead of failing, so the GUI stays alive and read-only.

The GUI/engine split is enforced at link time: `Sourcetrail_lib` holds the UI-agnostic business logic, `Sourcetrail_lib_engine` holds everything that touches SQLite (`PersistentStorage`, `Sqlite*Storage`, migrations, `Project`, `IndexTaskBuilder`, `RefreshInfoGenerator`). Only the engine and indexer link the latter. **Do not add a SQLite-touching header to `Sourcetrail_lib`.**

### Source layout

`src/app/` holds the four executables. `src/lib/` holds the libraries:

- **`core`** — foundational utilities: `FilePath`, `FileSystem`, `TextAccess`/`TextCodec`, `ConfigManager`, `Migration`/`Migrator`, logging, commandline parsing. Nearly everything links against it.
- **`messaging`** — decoupled pub/sub bus. `Message<T>` dispatches through the singleton `IMessageQueue`; `MessageListener<T>` subclasses receive `handleMessage(T*)`. The primary way GUI, `Application` and controllers communicate.
- **`scheduling`** — `Task`/`TaskGroup` framework (sequence, parallel, selector, delay/repeat decorators) run by `TaskScheduler`/`TaskRunner` over a shared `Blackboard`. Indexing and refresh pipelines are task graphs built from these.
- **`proto`** — the IPC contract: `engine.proto` (client ↔ engine), `indexer_worker.proto` (engine ↔ worker), `sourcetrail_common.proto`, plus the `Convert*.cpp` helpers that map storage/graph types to and from protobuf. `engine.proto` declares **no service**: it is the payload schema for the HTTP boundary, encoded with protobuf's canonical JSON mapping via `ProtoJson.{h,cpp}` (note that uint64 ids arrive as JSON *strings*). Only `indexer_worker.proto` still generates gRPC stubs.
- **`client`** — the engine-facing client side: `EngineChannel` (connection, one keep-alive HTTP connection per calling thread), `HttpStorageAccess` (read path), `HttpProject`, `EngineEventClient` (server-sent events), and `Capabilities` (what the connected engine can index; the GUI greys out controls from this, it never inspects the plugin directory itself).
- **`lib`** — business logic (see `src/lib/lib/README.md`):
  - `app/Application` — orchestrates `IFactory`, `IProject`, `StorageCache`, `MainView`, IDE communication. `app/IndexerPluginRegistry` discovers indexer plugins.
  - `component` — `Component`/`ComponentFactory`/`ComponentManager`/`Tab`: controller+view pair per UI panel/tab.
  - `project` — `IProject`/`Project` state machine (`NOT_LOADED` → `LOADED`/`OUTDATED`/`NEEDS_MIGRATION`/…) driving refresh/index task-graph construction (`RefreshInfoGenerator`, `SourceGroup*`).
  - `settings` — `Settings`/`ProjectSettings`/`IApplicationSettings`, with `migration/` for versioned upgrades.
  - `data` — storage layer (see `src/lib/lib/data/storage/README.md`): `Storage` → `PersistentStorage` (SQLite, engine-side only) or `IntermediateStorage` (in-memory staging); `StorageAccess`/`StorageAccessProxy`/`StorageCache` in front for cached reads. Also `graph`, `location`, `name`, `search`, `fulltextsearch`, `bookmark`, `indexer`.
  - `data/indexer` — `IndexerCommand`/`IndexerComposite`/`*CommandProvider` describe work; `data/indexer/grpc` (`GrpcIndexer`, `IndexerWorkerServiceImpl`) runs it in worker processes, orchestrated by `TaskBuildIndex`/`TaskFillIndexerCommandQueue`, merged back via `TaskMergeStorages`.
  - `factory` — `IFactory` abstracts construction of GUI-specific objects so `lib` stays toolkit-agnostic.
- **`lib_gui`** — the Qt6 GUI: `qt/window`, `qt/view`, `qt/element`, `qt/graphics`, `qt/network`, `qt/project_wizard`, plus `QtEngineSupervisor`. Platform quirks live behind `platform_includes/includes{Windows,Linux,Mac}.h`.
- **`lib_cxx`** (gated by `BUILD_CXX_LANGUAGE_PACKAGE`) — the C/C++ indexer on Clang/LibTooling; provides `LanguagePackageCxx`.
- **`external`** — vendored third-party code (`sqlite`), wrapped as `CppSQLite::CppSQLite3`.

`indexers/` holds out-of-tree indexer plugins: `indexers/java` (Maven-built JVM gRPC worker), `indexers/cxx` (docs for the built-in one).

### Indexer plugins

Every indexer — including the built-in C/C++ one — is resolved through a manifest at `build/app/plugins/<name>/manifest.xml`, generated from `src/app/indexer/manifest.xml.in`. `IndexerPluginRegistry::discover()` reads them at engine startup; there is **no special case for the built-in indexer** in `TaskBuildIndex`. The engine reports the resulting source-group types over `GetCapabilities`, so a build without the C/C++ package simply offers fewer project types in the wizard.

### Versioning

The version string is generated from git tags/commits at configure time (`cmake/version.cmake`, `cmake/productVersion.h.in`), not hand-maintained.

## Documentation

`DOCUMENTATION.md` is the end-user manual (features, UI, shortcuts, project setup) — consult it for application *behavior*, not architecture. `CHANGELOG.md` tracks release history. `.claude/skills/` holds task-scoped guides (`architecture`, `cmake`, `cpp20`, `grpc-ipc`, `qt6`, `testing`).
