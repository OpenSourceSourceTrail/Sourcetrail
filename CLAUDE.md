# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

Sourcetrail is a free, offline, cross-platform (Windows/Linux) C/C++ source explorer built on Qt6. It indexes source code (via Clang/LibTooling when `BUILD_CXX_LANGUAGE_PACKAGE` is enabled) into a SQLite-backed graph database and lets users interactively explore symbols, references, and call graphs.

The GUI is **Qt Quick / QML**, and it links the engine as a library: one process owns the scene, the business logic and the SQLite index. Indexing is the one thing that stays out of process — per-job `sourcetrail_indexer` workers, driven over gRPC. A separate `sourcetrail_engine` daemon still exists and still speaks **HTTP + JSON** (so a web app can drive the same index headlessly), but the GUI is no longer one of its clients. See *Process model* below before making any change that crosses those boundaries.

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

Requires LLVM/Clang 23 or newer (developed against 23.1.0) built with `-DLLVM_ENABLE_PROJECTS=clang -DLLVM_ENABLE_RTTI=ON` (plus `-DCLANG_LINK_CLANG_DYLIB=ON -DLLVM_LINK_LLVM_DYLIB=ON` on Unix).

On Linux, `scripts/build_llvm_conan.sh` produces that build through Conan from the recipe in `.conan/recipes/llvm-clang/`, then symlinks `<repo>/external` at the resulting package:
```
./scripts/build_llvm_conan.sh
cmake --preset=ci_gnu_release_build_cxx
```
That is a **second, separate** `conan install` (into `.conan/llvm/`); it deliberately stays out of the unified GCC/Release graph in `.conan/gcc/` so the main dependency set and its package IDs are unaffected. The first run compiles LLVM from source and takes hours; later runs are cache hits.

To skip that first build, restore the package CI publishes — `.github/workflows/llvm.yml` builds it once and uploads it as a GitHub Release asset on the `llvm-clang-23.1.0` tag:
```
gh release download llvm-clang-23.1.0 -p 'llvm-clang-23.1.0-linux-x86_64.tgz'
conan cache restore llvm-clang-23.1.0-linux-x86_64.tgz
./scripts/build_llvm_conan.sh   # now a cache hit; still makes the external/ symlink
```

With a hand-built LLVM, point at it instead:
```
cmake --preset=ci_gnu_release_build_cxx -DClang_DIR=<path/to/llvm_build>/lib/cmake/clang
```
The `build_cxx` presets default `Clang_DIR` to `<repo>/external/lib/cmake/clang/`. This turns on `indexers/cxx` (the language package plus the indexer worker) and installs the built-in C/C++ indexer plugin manifest. Without it, no indexer worker binary is built at all.

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
- **Unit tests** (`ENABLE_UNIT_TEST`): `src/lib/lib/tests`, `src/lib/lib_qml/tests`, `src/lib/client/tests`, `src/lib/core/tests`, `src/lib/messaging/tests`, `indexers/cxx/lib/tests` (only with `BUILD_CXX_LANGUAGE_PACKAGE=ON`).
- **Integration tests** (`ENABLE_INTEGRATION_TEST`): `tests/integration/{lib,lib_cxx,messaging}` — real SQLite round-trips, `MessageQueue`.

```
ctest --test-dir build
ctest --test-dir build -R "unittests\.lib\."
```
Registered prefixes: `unittests.lib.`, `unittests.lib_qml.`, `unittests.client.`, `unittests.core.`, `integration.lib.`, `integration.lib_cxx.`, `integration.messaging`.

New test targets go through the `add_sourcetrail_test()` helper (`cmake/add_sourcetrail_test.cmake`), not raw `add_executable` — it wires up `Sourcetrail::gtest_main` and `gtest_discover_tests`; `SR_SAN` applies sanitizer flags build-wide, not per-target. Copy the calling convention from `src/lib/core/tests/CMakeLists.txt`.

## Formatting / Linting

- `scripts/run_clang_format.sh` — formats all C/C++ sources in place (`.clang-format`, Google fallback style).
- `scripts/run_cmake_format.sh` — formats all `CMakeLists.txt`/`*.cmake` (`.cmake-format.yaml`).
- `.clang-tidy` is enforced via `cmake/clang-tidy.cmake`; `cmake/cppcheck.cmake` wires up cppcheck. Both run in CI (`.github/workflows/clang_tidy.yml`, `cppcheck.yaml`).
- `.pre-commit-config.yaml` runs clang-format 22.1.8 and cmakelang 0.6.13 (pinned to match CI) on staged files; run `pip install pre-commit && pre-commit install` once per clone to activate it.

## High-Level Architecture

### Process model

| Binary (`build/app/`) | Source | Role |
| --- | --- | --- |
| `Sourcetrail` | `src/app/qml_gui` + `src/lib/lib_qml` | Qt Quick GUI **and** the engine. Owns the SQLite index, the project state machine and the indexing task graph, in process. |
| `sourcetrail_engine` | `src/app/engine` | Headless HTTP daemon (`EngineHttpService`). Owns the SQLite index, the project state machine and the indexing task graph. |
| `sourcetrail_indexer` | `indexers/cxx/indexer` | Indexer worker process, spawned per indexing job. Built only with `BUILD_CXX_LANGUAGE_PACKAGE`. |
| `Sourcetrail_cli` | `src/app/cli` | Headless, Qt-free front end. |

The GUI, the CLI and the daemon are three front ends onto the same two libraries. `Sourcetrail_lib`
holds the UI-agnostic business logic; `Sourcetrail_lib_engine` holds everything that touches SQLite
(`PersistentStorage`, `Sqlite*Storage`, migrations, `Project`, `IndexTaskBuilder`,
`RefreshInfoGenerator`). **Do not add a SQLite-touching header to `Sourcetrail_lib`** — the split is
what lets the daemon and the worker be built without a front end.

What decides which one you get is `lib::IFactory`, handed to `Application::createInstance`:
`lib::Factory` builds a `Project` that owns a `PersistentStorage` (the GUI, the CLI, the daemon),
`client::ClientFactory` builds an `HttpProject` that owns nothing and asks a daemon over HTTP.
Everything above `IFactory` is identical either way.

`Application` reaches the user interface only through `lib::IAppShell` (`src/lib/lib/app/`), a
thirteen-method interface with no toolkit in it. `AppShell` in `lib_qml` is the only implementation;
passing `nullptr` is what makes an Application headless, as the CLI and the daemon do.

`sourcetrail_engine` still prints `ENGINE_PORT <n> <token>` on its first stdout line so a parent
process can learn the ephemeral port and the per-run bearer credential every request must present: a
loopback HTTP port is reachable by any local process and by any page the user's browser loads.
Requests carrying an un-allow-listed `Origin` are refused outright.

### Source layout

`src/app/` holds the executables. `src/lib/` holds the libraries:

- **`core`** — foundational utilities: `FilePath`, `FileSystem`, `TextAccess`/`TextCodec`, `ConfigManager`, `Migration`/`Migrator`, logging, commandline parsing. Nearly everything links against it.
- **`messaging`** — decoupled pub/sub bus. `Message<T>` dispatches through the singleton `IMessageQueue`; `MessageListener<T>` subclasses receive `handleMessage(T*)`. The primary way GUI, `Application` and controllers communicate.
- **`scheduling`** — `Task`/`TaskGroup` framework (sequence, parallel, selector, delay/repeat decorators) run by `TaskScheduler`/`TaskRunner` over a shared `Blackboard`. Indexing and refresh pipelines are task graphs built from these.
- **`proto`** — the IPC contract: `engine.proto` (client ↔ engine), `indexer_worker.proto` (engine ↔ worker), `sourcetrail_common.proto`, plus the `Convert*.cpp` helpers that map storage/graph types to and from protobuf. `engine.proto` declares **no service**: it is the payload schema for the HTTP boundary, encoded with protobuf's canonical JSON mapping via `ProtoJson.{h,cpp}` (note that uint64 ids arrive as JSON *strings*). Only `indexer_worker.proto` still generates gRPC stubs.
- **`client`** — the daemon-facing client side, for front ends that are not in the engine's process: `EngineChannel` (connection, one keep-alive HTTP connection per calling thread), `HttpStorageAccess` (read path), `HttpProject`, `EngineEventClient` (server-sent events), and `Capabilities`. The QML GUI links none of it; it reads `IndexerPluginRegistry` and `SourceGroupFactory` directly.
- **`lib`** — business logic (see `src/lib/lib/README.md`):
  - `app/Application` — orchestrates `IFactory`, `IProject`, `StorageCache`, `IAppShell`, IDE communication. `app/IndexerPluginRegistry` discovers indexer plugins.
  - `component` — `Component` pairs a `Controller` with a `View`. The controllers hold the real logic and are toolkit-free (`GraphController` and the `*Layouter` helpers compute the whole graph layout); the `*View` classes are the abstract interfaces they push results at. `GraphLayoutService` in the daemon and the QML view-models both drive them, each supplying its own `GraphView`.
  - `project` — `IProject`/`Project` state machine (`NOT_LOADED` → `LOADED`/`OUTDATED`/`NEEDS_MIGRATION`/…) driving refresh/index task-graph construction (`RefreshInfoGenerator`, `SourceGroup*`).
  - `settings` — `Settings`/`ProjectSettings`/`IApplicationSettings`, with `migration/` for versioned upgrades.
  - `data` — storage layer (see `src/lib/lib/data/storage/README.md`): `Storage` → `PersistentStorage` (SQLite, engine-side only) or `IntermediateStorage` (in-memory staging); `StorageAccess`/`StorageAccessProxy`/`StorageCache` in front for cached reads. Also `graph`, `location`, `name`, `search`, `fulltextsearch`, `bookmark`, `indexer`.
  - `data/indexer` — `IndexerCommand`/`IndexerComposite`/`*CommandProvider` describe work; `data/indexer/grpc` (`GrpcIndexer`, `IndexerWorkerServiceImpl`) runs it in worker processes, orchestrated by `TaskBuildIndex`/`TaskFillIndexerCommandQueue`, merged back via `TaskMergeStorages`.
  - `factory` — `IFactory` abstracts construction of GUI-specific objects so `lib` stays toolkit-agnostic.
- **`lib_qml`** — the Qt Quick GUI. `AppShell` (the `lib::IAppShell` implementation and the QML singleton everything binds to), `GuiThread.h` (the queued, **non-blocking** hop from the message-bus thread to the GUI thread — see below), `QmlDialogView` (indexing progress), `QmlGraphViewStyleImpl` (node sizing from real font metrics), `network/` (the IDE plugin protocol), and `qml/` (the scene, compiled into the binary by `qt_add_qml_module`).

  Two rules hold this layer together. **Nothing blocks the bus thread:** the message queue runs on
  its own thread and calls view-models from it, so every update hops to the GUI thread with
  `qml::postToGui`, which queues and returns. **Nothing queries storage from the GUI thread:** read
  on the bus thread, move a value snapshot across the hop.
- **`external`** — vendored third-party code (`sqlite`), wrapped as `CppSQLite::CppSQLite3`.

`indexers/` holds the indexer plugins, one directory per plugin: `indexers/java` (Maven-built JVM gRPC worker) and `indexers/cxx` — the built-in C/C++ one, whose `lib/` is the Clang/LibTooling language package (`LanguagePackageCxx`, gated by `BUILD_CXX_LANGUAGE_PACKAGE`) and whose `indexer/` is the `sourcetrail_indexer` worker executable that hosts it.

### Indexer plugins

Every indexer — including the built-in C/C++ one — is resolved through a manifest at `build/app/plugins/<name>/manifest.xml`, generated from `indexers/cxx/indexer/manifest.xml.in`. `IndexerPluginRegistry::discover()` reads them at engine startup; there is **no special case for the built-in indexer** in `TaskBuildIndex`. The engine reports the resulting source-group types over `GetCapabilities`, so a build without the C/C++ package simply offers fewer project types in the wizard.

### Versioning

The version string is generated from git tags/commits at configure time (`cmake/version.cmake`, `cmake/productVersion.h.in`), not hand-maintained.

## Documentation

`DOCUMENTATION.md` is the end-user manual (features, UI, shortcuts, project setup) — consult it for application *behavior*, not architecture. `CHANGELOG.md` tracks release history. `.claude/skills/` holds task-scoped guides (`architecture`, `cmake`, `cpp20`, `grpc-ipc`, `qt6`, `testing`).
