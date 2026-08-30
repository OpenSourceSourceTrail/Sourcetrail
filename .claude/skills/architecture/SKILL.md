---
name: architecture
description: Sourcetrail architecture map — source tree layout, messaging pub/sub bus, storage layer, task scheduler, C/C++ indexer. Use to orient in the codebase or when a change spans multiple modules.
---

# Architecture

## Source tree (`src/`)

```
src/
  app/                  Executable entry points
    qml_gui/            Main GUI application binary (Sourcetrail) -- Qt Quick shell + in-process engine
    cli/                Headless, Qt-free CLI client (Sourcetrail_cli)
    engine/             HTTP+JSON service impl for the engine daemon (Sourcetrail_engine)
  lib/                  Libraries
    proto/              Payload schema (*.proto) + Convert helpers (storage <-> proto) + ProtoJson
    core/                Fine-grained utility libraries (file system, logging, config,
                         text codec, migration, string utils, UUIDs, etc.)
    external/            Vendored third-party code (CppSQLite3, etc.)
    lib/                 Core business logic (Qt-free):
      app/               Application singleton, IAppShell (the only seam to a user interface),
                         LanguagePackageManager
      component/         MVC-like Components (each owns a Controller + View pair). Controllers hold
                         the logic and are toolkit-free; views are abstract interfaces.
      data/              Domain model — graph, storage, search, full-text index
      factory/           Abstract factories (IFactory, NetworkFactory). lib::Factory is the
                         in-process one (owns PersistentStorage); client::ClientFactory talks HTTP.
      project/           Project lifecycle (load, refresh, build index)
      settings/          ProjectSettings and source group settings
      utility/           Lib-specific helpers
    lib_qml/             Qt Quick / QML GUI: AppShell (IAppShell + QML singleton), view-models,
                         GuiThread.h (non-blocking bus-thread -> GUI-thread hop), qml/ scene
    messaging/           Pub/sub message bus
    scheduling/          Behavior-tree task scheduler

indexers/               Indexer plugins, one directory per plugin
  cxx/                  Built-in C/C++ indexer (BUILD_CXX_LANGUAGE_PACKAGE)
    indexer/            Worker process (Sourcetrail_indexer)
    lib/                Clang/LibTooling language package (LanguagePackageCxx)
  java/                 Maven-built JVM gRPC worker (BUILD_JAVA_INDEXER)
```

## Messaging (`src/lib/messaging/`)

Decoupled pub/sub bus. `MessageQueue` is the central singleton. Senders call `Message::dispatch()`. Receivers inherit `MessageListener<T>` and implement `handleMessage(T&)`. All message types live under `messaging/type/`. Primary cross-cutting communication mechanism (in-process). Cross-process: the QML GUI has no engine boundary -- it links `lib_engine` and owns the index. The
daemon's HTTP + JSON boundary (`src/lib/core/http`, `EngineHttpService`, server-sent events) is still
there for headless and web clients; the engine <-> indexer boundary is gRPC — see `grpc-ipc` skill.

Views are called from the message-queue thread. In `lib_qml` every one of those calls hops to the GUI
thread with `qml::postToGui` (queued, non-blocking) carrying an owned value snapshot -- never a
reference into bus-thread data, and never a storage query on the GUI thread.

## Storage layer (`src/lib/lib/data/storage/`)

- `IntermediateStorage` — in-memory buffer used during indexing; filled by parser/indexer.
- `PersistentStorage` — delegates to `SqliteIndexStorage` (symbols, edges, locations) and `SqliteBookmarkStorage`. Inherits `StorageCache` for frequently accessed node/file lookups.
- `StorageAccessProxy` — wraps `PersistentStorage` (or `HttpStorageAccess`) for read access; `StorageCache` in front of it is what every controller actually holds.

## Task scheduler (`src/lib/scheduling/`)

Behavior-tree–style task system. `TaskRunner` drives execution; `Blackboard` passes data between tasks.

- Composites: `TaskGroupSequence`, `TaskGroupParallel`, `TaskGroupSelector`
- Decorators: `TaskDecoratorRepeat`, `TaskDecoratorDelay`
- Indexing pipelines are assembled as task trees via `ITaskFactory` (`lib/lib/project/ITaskFactory.h`); default implementation `IndexTaskBuilder` (`lib/lib/project/IndexTaskBuilder.{h,cpp}`) — extracted from the former `Project::createIndexTasks()`.

## C/C++ indexing (`indexers/cxx/lib/`)

`LanguagePackageCxx` registers the CXX source groups and indexer. Uses Clang's LibTooling/LibASTMatchers to traverse the AST and populate `IntermediateStorage`. Only compiled when `BUILD_CXX_LANGUAGE_PACKAGE=ON`.
