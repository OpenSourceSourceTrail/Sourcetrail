# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

Scope: the Java indexer plugin (`indexers/java/`). The repo-wide guide is `<repo>/CLAUDE.md`; read it
for the process model, the storage layer and the C++ conventions this worker has to match.

## What this is

A standalone JVM process that indexes `.java` files for Sourcetrail. It speaks the same gRPC contract
as the C/C++ worker (`indexers/cxx/indexer`), so the engine treats both identically — there is no
Java-specific code path on the C++ side.

```
engine  --gRPC-->  Main -> GrpcWorker -> JavaIndexer -> JavaCollector -> Storage -> IntermediateStorage
```

- `Main` — argv contract, byte-for-byte the same as `indexers/cxx/indexer/main.cpp`:
  `<processId> --engine-endpoint <host:port> <sharedDataPath> <userDataPath> [logFilePath]`.
- `GrpcWorker` — mirrors `GrpcIndexer.cpp`: `pullCommand → reportStatus(START_FILE) → index →
  pushIntermediateStorage → reportStatus(FINISH_FILE)`, looped until `commandFound == false` or a
  `watchInterrupt` event arrives; then `reportStatus(PROCESS_DONE)`.
- `JavaIndexer` — parses one file with JavaParser (no classpath, no symbol solver). Any parse or
  runtime failure returns an empty `IntermediateStorage` with `nextId = 1` rather than throwing:
  the engine then marks the file incomplete instead of losing the worker process.
- `JavaCollector` — `VoidVisitorAdapter` walk emitting nodes/edges/locations.
- `NameResolver` — purely lexical FQN resolution from the package declaration + import table.
  Wildcard imports are skipped; unresolved simple names are assumed to be in the current package.
- `Storage` — buffers proto messages, allocates ids from 1 (`FILE_ID = 1` is the file node), and
  builds the `IntermediateStorage`.

## Two contracts that must not drift

1. **`Names`** produces the tab-delimited serialization of `NameHierarchy::serialize`
   (`src/lib/lib/data/name/NameHierarchy.cpp`). The engine dedups nodes at merge time on
   `(type, serializedName)`, so a formatting mismatch silently splits symbols instead of erroring.
2. **`Kinds`** hard-codes the integer values of the C++ enums (`NodeKind.h`, `DefinitionKind.h`,
   `AccessKind.h`, `Edge.h`, `LocationType.h`). Nothing checks these at build time — if you touch
   any of those C++ headers, update `Kinds.java` in the same change.

The protos are not vendored: `pom.xml` points `protoSourceRoot` at `../../../src/lib/proto`, so
`sourcetrail_common.proto` / `indexer_worker.proto` are the single source of truth and Java stubs are
regenerated on every `mvn package`.

## Build & test

The CMake target `Sourcetrail_java_indexer` (`indexers/java/CMakeLists.txt`) just shells out to Maven
and stages `target/sourcetrail_java_indexer.jar` + `manifest.xml` into `build/app/plugins/java/`,
where `IndexerPluginRegistry::discover()` picks them up. It is enabled by `BUILD_JAVA_INDEXER`, which
defaults ON when `mvn` is on `PATH`.

For iterating on Java code, skip CMake and use Maven directly (JDK 21, `maven.compiler.release=21`):

```
mvn -f indexer/pom.xml package          # regenerate stubs, compile, run tests, shade the jar
mvn -f indexer/pom.xml test             # tests only
mvn -f indexer/pom.xml test -Dtest=JavaIndexerTest#recordEmitsClassNode   # one test
```

Tests are JUnit 5 (`JavaIndexerTest`); they index real Java source from a temp file through
`JavaIndexer.index` and assert on the emitted proto — the same path the gRPC worker runs.

The shade plugin's `ServicesResourceTransformer` is load-bearing: without it gRPC's
`META-INF/services` entries collide and channel construction fails with
"Could not find a NameResolverProvider".
