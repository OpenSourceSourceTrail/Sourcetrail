---
name: architecture
description: Sourcetrail architecture map — source tree layout, messaging pub/sub bus, storage layer, task scheduler, C/C++ indexer. Use to orient in the codebase or when a change spans multiple modules.
---

# Architecture

## Source tree (`src/`)

```
src/
  app/                  Executable entry points
    gui/                Main GUI application binary (Sourcetrail)
    cli/                Headless, Qt-free CLI client (Sourcetrail_cli)
    engine/             The engine as a library (EngineHost, EngineHttpService) plus the
                         standalone daemon binary (Sourcetrail_engine); the GUI links the same library
  lib/                  Libraries
    proto/              Payload schema (*.proto) + Convert helpers (storage <-> proto) + ProtoJson
    core/                Fine-grained utility libraries (file system, logging, config,
                         text codec, migration, string utils, UUIDs, etc.)
    external/            Vendored third-party code (CppSQLite3, etc.)
    lib/                 Core business logic (Qt-free):
      app/               Application singleton, LanguagePackageManager
      component/         MVC-like Components (each owns a Controller + View pair)
      data/              Domain model — graph, storage, search, full-text index
      factory/           Abstract factories (IFactory, IViewFactory, INetworkFactory)
      project/           Project lifecycle (load, refresh, build index)
      settings/          ProjectSettings and source group settings
      utility/           Lib-specific helpers
    lib_gui/             Qt 6 GUI implementation (QtApplication, Qt views/windows)
    messaging/           Pub/sub message bus
    scheduling/          Behavior-tree task scheduler

indexers/               Indexer plugins, one directory per plugin
  cxx/                  Built-in C/C++ indexer (BUILD_CXX_LANGUAGE_PACKAGE)
    indexer/            Worker process (Sourcetrail_indexer)
    lib/                Clang/LibTooling language package (LanguagePackageCxx)
  java/                 Maven-built JVM gRPC worker (BUILD_JAVA_INDEXER)
```

## Messaging (`src/lib/messaging/`)

Decoupled pub/sub bus. `MessageQueue` is the central singleton. Senders call `Message::dispatch()`. Receivers inherit `MessageListener<T>` and implement `handleMessage(T&)`. All message types live under `messaging/type/`. Primary cross-cutting communication mechanism (in-process), and the only one the GUI needs by default: it hosts the engine itself, so progress and status messages are already on its own bus. Cross-process: `EngineHttpService` still serves HTTP + JSON (`src/lib/core/http`, server-sent events for engine->client push) for the standalone daemon, the MCP server and a GUI started with `--engine`; the engine <-> indexer boundary is always gRPC — see `grpc-ipc` skill.

## Storage layer (`src/lib/lib/data/storage/`)

- `IntermediateStorage` — in-memory buffer used during indexing; filled by parser/indexer.
- `PersistentStorage` — delegates to `SqliteIndexStorage` (symbols, edges, locations) and `SqliteBookmarkStorage`. Inherits `StorageCache` for frequently accessed node/file lookups.
- `StorageAccessProxy` — wraps `PersistentStorage` for read access; used by GUI components to query data without coupling to the full storage API.

## Task scheduler (`src/lib/scheduling/`)

Behavior-tree–style task system. `TaskRunner` drives execution; `Blackboard` passes data between tasks.

- Composites: `TaskGroupSequence`, `TaskGroupParallel`, `TaskGroupSelector`
- Decorators: `TaskDecoratorRepeat`, `TaskDecoratorDelay`
- Indexing pipelines are assembled as task trees via `ITaskFactory` (`lib/lib/project/ITaskFactory.h`); default implementation `IndexTaskBuilder` (`lib/lib/project/IndexTaskBuilder.{h,cpp}`) — extracted from the former `Project::createIndexTasks()`.

## C/C++ indexing (`indexers/cxx/lib/`)

`LanguagePackageCxx` registers the CXX source groups and indexer. Uses Clang's LibTooling/LibASTMatchers to traverse the AST and populate `IntermediateStorage`. Only compiled when `BUILD_CXX_LANGUAGE_PACKAGE=ON`.
