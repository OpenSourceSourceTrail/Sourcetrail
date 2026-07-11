# Java Language Support — Increment 1 (Plumbing via Indexer Plugin System)

## Summary

First increment of Java language support, built on top of the runtime indexer plugin system (`docs/features/indexer-plugin-system.md`). Java is not a compiled-in language package like C/C++ — it is an **external worker executable + XML manifest** discovered at runtime under `<sharedData>/plugins/<id>/manifest.xml`. This increment lands all the in-tree C++ plumbing needed to define a Java project, generate/serialize Java indexer commands, and launch a discovered Java plugin executable, proven end-to-end with a throwaway stub worker. No real Java parsing yet.

**Branch:** `taskflow` (uncommitted working tree at time of writing)

---

## Motivation

The user wants Java indexing support, built incrementally starting from the indexer. Since C/C++ support is gated behind `BUILD_CXX_LANGUAGE_PACKAGE` and requires LLVM/Clang linked into the binary, and the historical Sourcetrail Java indexer (JavaSymbolSolver-based) has been fully removed from the tree, Java needed a fresh design. The indexer plugin system landed just before this work started, which changed the shape of the problem: instead of a new compiled-in `LanguagePackageJava`, Java fits naturally as a **pure-JVM gRPC worker** shipped as a plugin — no JNI, no bundling a JVM into Sourcetrail itself.

---

## Design

- **In-tree, always compiled:** enums, `IndexerCommandJava`, `SourceGroupJavaEmpty`/settings, proto serialization, and wizard wiring are compiled into the core lib unconditionally (no `BUILD_JAVA_LANGUAGE_PACKAGE` macro). This is deliberate — it is lightweight (no heavy dependency), and matches the plugin system's runtime-discovery philosophy: drop a Java plugin manifest into `plugins/`, no rebuild required.
- **External worker, not in-process:** unlike CXX (which has both a compiled-in `LanguagePackageCxx` and can run in-process via `LanguagePackageManager`), Java has **no in-process indexer at all**. The real Java parser (a later increment) will be a standalone JVM program speaking the existing `IndexerWorkerService` gRPC protocol directly — no C++/JNI bridge.
- **Rides the existing gRPC pipeline:** `IndexTaskBuilder::buildIndexerCommandProviders` already routes every non-`SOURCE_GROUP_CUSTOM_COMMAND` source group to the "standard" gRPC worker path (`project/IndexTaskBuilder.cpp:58-75`). Java commands ride this for free — no new task-tree wiring needed.
- **Single-language assumption:** this increment does not solve mixed C++/Java command routing (one shared PullCommand queue, workers of different types). `TaskBuildIndex` resolves the worker executable from the *project's* dominant command type. Mixed-language projects are a later increment.

---

## New Files

### `src/lib/lib/data/indexer/IndexerCommandJava.{h,cpp}`
`IndexerCommand` subclass carrying `classPaths` (`std::set<FilePath>`) and `languageStandard` (Java version, e.g. `"17"`). Lives in core lib next to `IndexerCommandCustom`, not a separate module — mirrors how Custom Command support is structured, since Java (like Custom) has no heavy in-tree parser dependency.

### `src/lib/lib/project/SourceGroupJavaEmpty.{h,cpp}` / `SourceGroupFactoryModuleJava.{h,cpp}`
Source group for a flat list of Java source paths. Reuses `MemoryIndexerCommandProvider` (`data/indexer/MemoryIndexerCommandProvider.h`) instead of writing a new dedicated provider — it was already generic enough. `SourceGroupFactoryModuleJava::supports()` covers `SOURCE_GROUP_JAVA_EMPTY`.

### `src/lib/lib/settings/source_group/type/SourceGroupSettingsJavaEmpty.h`
Composes `SourceGroupSettingsWithSourcePaths`, `SourceGroupSettingsWithExcludeFilters`, a new `SourceGroupSettingsWithSourceExtensionsJava` (fixed default `.java`), and a new `component/java/SourceGroupSettingsWithJavaClassPath.{h,cpp}` (classpath entries + language standard, defaults to `"17"`).

### `src/app/indexer_java_stub/`
A **stub worker**, not the real Java parser. `IndexerJavaStub : IndexerBase` returns an empty `IntermediateStorage` for any `INDEXER_COMMAND_JAVA` command. Registered via `LanguagePackageJavaStub` into the existing `GrpcIndexer::work()` loop (`data/indexer/grpc/GrpcIndexer.cpp`) — reused as-is, zero protocol code duplicated. Ships with `manifest.xml`; a CMake `POST_BUILD` step places both next to each other at `<build>/app/plugins/java/`, matching `AppPath::getPluginsDirectoryPath()` (`<sharedData>/plugins`) so it is discovered automatically in a dev build with no install step.

### Tests
- `IndexerCommandJavaTestSuite.cpp` — `IndexerCommandJava` round-trips through `proto::convert::toProto`/`fromProto`.
- `IndexerPluginRegistryTestSuite.cpp` — added `parsesJavaManifestAndAnswersCapabilityQueries`.
- `ProjectSettingsTestSuite.cpp` — added `javaSourceGroupRoundTripsThroughSaveAndLoad` (save → load → same source paths / class paths).

---

## Modified Files

- **`LanguageType.{h,cpp}`, `SourceGroupType.{h,cpp}`, `IndexerCommandType.{h,cpp}`** — added `LANGUAGE_JAVA`, `SOURCE_GROUP_JAVA_EMPTY`, `INDEXER_COMMAND_JAVA`, all ungated (outside the `#if BUILD_CXX_LANGUAGE_PACKAGE` blocks).
- **`ProjectSettings.cpp`** — `getAllSourceGroupSettings()` construct-switch now builds `SourceGroupSettingsJavaEmpty` for the Java type, so `.srctrlprj` files round-trip.
- **`sourcetrail_common.proto`** — `IndexerCommand.CommandType` gained `JAVA = 2`; new fields `class_paths` (8), `language_standard` (9). CXX fields untouched for wire compatibility.
- **`proto/Convert.cpp`** — `toProto`/`fromProto` gained an ungated `IndexerCommandJava` branch (parallel to the `#if BUILD_CXX_LANGUAGE_PACKAGE`-gated CXX branch). No `proto/CMakeLists.txt` change needed — `IndexerCommandJava` lives in core lib, already visible to `proto_convert` via the existing `Sourcetrail_lib` include-dir generator expression.
- **`data/indexer/TaskBuildIndex.{h,cpp}`** — closed a real gap: `runIndexerProcess` previously **hardcoded** `INDEXER_COMMAND_CXX` when resolving the plugin executable, so a non-CXX command type could never launch its worker. It now takes an `IndexerCommandType commandType` constructor parameter and resolves `IndexerPluginRegistry::indexerExecutablePathFor(mCommandType)`, falling back to the compiled `AppPath::getCxxIndexerFilePath()` only when `mCommandType == INDEXER_COMMAND_CXX`. Threaded through `ITaskFactory`/`DefaultTaskFactory::createBuildIndex`.
- **`project/IndexTaskBuilder.{h,cpp}`** — added `getStandardCommandType(sourceGroups)` (single-language assumption, documented inline) to pick the command type passed to `createBuildIndex`. Also fixed the `multiProcess` gate: it previously only considered `hasCxxSourceGroup()`, which meant a Java-only project would fall back to **in-process** indexing (`runIndexerThread` → `GrpcIndexer` → `LanguagePackageManager`) — but Java has no in-process indexer, so Java commands would simply never be processed. New `hasJavaSourceGroup` callback forces multi-process whenever Java is present, independent of the "multi-process indexing" application setting.
- **`project/Project.{h,cpp}`** — added `hasJavaSourceGroup()`, mirroring `hasCxxSourceGroup()`, wired into the `IndexTaskBuilder::Callbacks`.
- **`lib_gui/qt/project_wizard/QtProjectWizard.cpp`** — added `addSourceGroupContents<SourceGroupSettingsJavaEmpty>` (Source Paths, Exclude Filters, Extensions pages) and a `SOURCE_GROUP_JAVA_EMPTY` case in `selectedProjectType`.
- **`lib_gui/qt/project_wizard/content/QtProjectWizardContentSelect.cpp`** — Java entry added to the language picker, gated by `pluginRegistry->supportsSourceGroupType(SOURCE_GROUP_JAVA_EMPTY)` — only shown when a Java plugin is actually discovered.
- **4× `app/{gui,cli,engine}/main.cpp`** — `SourceGroupFactory::getInstance()->addModule(make_shared<SourceGroupFactoryModuleJava>())`, ungated. (`app/indexer/main.cpp` unchanged — the worker process never needs `SourceGroupFactory`.)
- **`lib/lib/CMakeLists.txt`, `lib/lib/tests/CMakeLists.txt`** — register new sources/tests in the always-compiled lists (not the `if(BUILD_CXX_LANGUAGE_PACKAGE)` block).
- Root **`CMakeLists.txt`** — `add_subdirectory(src/app/indexer_java_stub)`.

---

## Issues Faced and How They Were Solved

### 1. Plan written before the plugin system landed had to be redesigned mid-flight
The original plan (written in an earlier session) assumed a compiled-in `lib_java` module mirroring `lib_cxx`, gated by a new `BUILD_JAVA_LANGUAGE_PACKAGE` macro. Between planning and implementation, `50548b3f` (runtime indexer plugin system) landed, which replaced compile-time language gating with runtime plugin discovery. The plan was reviewed and rewritten around the new architecture before any code was written — see the plan file history for the full before/after reasoning.

### 2. `TaskBuildIndex` hardcoded CXX — silently broke any non-CXX out-of-process command
Before this change, `runIndexerProcess` always resolved `IndexerPluginRegistry::indexerExecutablePathFor(INDEXER_COMMAND_CXX)`, regardless of what was actually queued. A Java project would have spawned the CXX indexer (or nothing at all) for Java commands. Fixed by threading the actual `IndexerCommandType` through the `ITaskFactory`/`TaskBuildIndex` constructor chain.

### 3. Java-only projects would have silently no-op'd in-process
`IndexTaskBuilder`'s `multiProcess` flag gated purely on `hasCxxSourceGroup()`. Since Java has no in-process indexer (by design — it is an external-only plugin), a Java-only project with the default settings would fall to `runIndexerThread` (in-process `GrpcIndexer` via `LanguagePackageManager`, which never has a Java entry) and every command would fail the `IndexerComposite::index()` type lookup silently. Fixed by adding `hasJavaSourceGroup()` to force multi-process indexing whenever Java is present.

### 4. Proving the plumbing without the real parser
The real Java worker (pure-JVM gRPC client, later increment) doesn't exist yet, but the C++ side of the pipeline (plugin discovery → executable launch → gRPC `PullCommand`/`PushIntermediateStorage`) needed verification now. Rather than block on the JVM worker, `IndexerJavaStub` reuses the *existing* `GrpcIndexer::work()` loop unchanged — it just registers an `IndexerBase` that returns an empty `IntermediateStorage` for `INDEXER_COMMAND_JAVA`. This validated the entire discovery-to-launch path with zero protocol code duplicated, and will be deleted outright once the real JVM worker lands.

### 5. `ConfigManager` XML root and manifest key resolution
Same `<config>` root requirement as the CXX plugin system doc already documented (`ConfigManager::createAndLoad` needs a top-level `<config>` node). The Java manifest's `commandType`/`language`/`source_group_types` strings had to exactly match `indexerCommandTypeToString(INDEXER_COMMAND_JAVA)` (`"indexer_command_java"`), `languageTypeToString(LANGUAGE_JAVA)` (`"Java"`), and `sourceGroupTypeToString(SOURCE_GROUP_JAVA_EMPTY)` (`"Java Source Group"`) — verified against the actual `.cpp` mappings before writing the manifest and tests, not assumed.

---

## Verification

- Full `cmake --preset=gnu_release_build_cxx && ninja` — clean build, all 547+ targets including the four main executables and the new `Sourcetrail_indexer_java_stub`.
- `ctest` — 907/907 tests pass (903 pre-existing + 4 new; no regressions).
- Manual check: `<build>/app/plugins/java/{manifest.xml,sourcetrail_indexer_java_stub}` land automatically via the stub's CMake `POST_BUILD` step, matching `IndexerPluginRegistry`'s expected `<sharedData>/plugins/<id>/manifest.xml` layout with no install step.

---

## Later Increments (out of scope here)

1. Wizard depth (classpath/JDK-version UI), Maven/Gradle source-group types with real classpath resolution.
2. **Real JVM gRPC worker** — a standalone Java program implementing the `IndexerWorkerService` client against the existing `.proto` (Java codegen), driving JavaSymbolSolver/JavaParser to populate `IntermediateStorage`, packaged with its own `manifest.xml` into `plugins/java/`. Replaces `indexer_java_stub` outright.
3. Mixed-language command routing — one shared command queue currently assumes workers all speak the same command type; segregating Java/CXX commands to the right worker pool is unsolved.
