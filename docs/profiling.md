# Profiling an index run with Tracy

The engine already prints coarse `INDEXER_TIMING` lines to stderr (per-worker parse/serialize/push
totals from `GrpcIndexer`, pipeline phase totals from `indexing_stats::print()`). Those give
per-run sums. Tracy gives the timeline: which translation unit, Clang frontend versus our own AST
traversal, and whether the engine's merge/inject is starving the workers.

Off by default. Nothing below affects a normal build.

## Build

```bash
cmake --preset=ci_gnu_release_build_cxx -DENABLE_TRACY=ON
cmake --build build
```

`ENABLE_TRACY` is the only thing in this build that reaches the network at configure time: Tracy
0.14.1 comes from `FetchContent`, because ConanCenter's newest recipe is 0.13.1.

The client is built `TRACY_ON_DEMAND` -- it collects nothing until a profiler attaches, so leaving
the option on does not make an unattended index run swell.

## Build the profiler server once

The server is not part of the build. It comes with the fetched source:

```bash
sudo apt install libglfw3-dev libfreetype-dev libcapstone-dev libdbus-1-dev libtbb-dev
cmake -S build/_deps/tracy-src/profiler -B build/tracy-profiler -DCMAKE_BUILD_TYPE=Release
cmake --build build/tracy-profiler
```

## Capture

Indexing runs several worker processes, and a Tracy client binds exactly one port. **Set indexer
threads to 1** (Preferences -> Indexing) before capturing, so there is one worker to attach to.

Each process picks its own port and prints it as `TRACY_PORT <n>` on stderr:

| Process | Port |
| --- | --- |
| `Sourcetrail` (hosts the engine), `sourcetrail_engine`, `Sourcetrail_cli` | 8086 |
| `sourcetrail_indexer` worker *n* | 8086 + *n*, so 8087 for the first |

Run one profiler instance per process:

```bash
build/tracy-profiler/tracy-profiler -a 127.0.0.1 -p 8086 &   # engine
build/tracy-profiler/tracy-profiler -a 127.0.0.1 -p 8087 &   # worker 1
```

Then start `build/app/Sourcetrail` from `build/app/` and index a project.

## What the zones mean

Worker capture, one frame per translation unit (`file`):

| Zone | Meaning |
| --- | --- |
| `worker/pull` | Blocked waiting for work. `PullCommand` parks server-side, so this is starvation, not polling. |
| `worker/parse` | The whole index of one file; annotated with its path. |
| `cxx/invocation-string` | `ClangInvocationInfo` building a driver invocation of its own, purely for the log line. |
| `cxx/tool.run` | Clang's frontend plus our traversal. |
| `cxx/indexDecl` | Our AST traversal alone, nested inside `cxx/tool.run`. |
| `worker/serialize` | `IntermediateStorage` -> protobuf. |
| `worker/push` | The gRPC call carrying it to the engine. |

`cxx/tool.run` minus `cxx/indexDecl` is the Clang frontend: preprocessing, parsing, sema. That
difference is the whole point of the exercise -- it says whether to attack compiler flags and PCH
or `CxxAstVisitor`.

Engine capture:

| Zone / plot | Meaning |
| --- | --- |
| `engine/recv-fromProto`, `engine/recv-insert` | Decoding a worker's push and queueing it. |
| `engine/merge`, `engine/inject` | The merge and inject pipeline stages. |
| `engine/filePathMaps`, `engine/searchIndex`, `engine/memberEdgeOrder`, `engine/hierarchyCache`, `engine/fullTextIndex`, `engine/optimizeDatabase` | Post-index cache builds, all SQLite-side. |
| `engine/storage-queue-depth` (plot) | A queue that stays full means the workers are ahead and injection is the bottleneck; one that stays empty means the reverse. |

Cross-check the Tracy totals against the `INDEXER_TIMING` stderr lines. They should agree within a
few percent; if they do not, a zone is in the wrong place.

## Adding zones

`src/lib/core/utility/profiling/Profiling.h` wraps the handful of Tracy macros the indexing path
uses (`SR_ZONE_N`, `SR_ZONE_TEXT`, `SR_PLOT`, `SR_FRAME_MARK`, `SR_THREAD_NAME`). They compile to
nothing without `ENABLE_TRACY`. Do not instrument `ParserClientImpl::record*` or
`PreprocessorCallbacks` -- millions of calls per translation unit would drown the capture.

Every executable opens the client itself with a `profiling::Scope` as the first statement in
`main()`, because the client is built `TRACY_MANUAL_LIFETIME` so each process can choose its port.
**A new executable that links a zone needs one too** -- with manual lifetime, a zone reached before
the profiler starts is undefined behaviour, and it crashes rather than being ignored. That is why
`tests/gtest_main.cpp` has one: the test binaries link zones without ever profiling anything.

Tracy's own `TRACY_ENABLE` defaults to *off* since 0.14, so the build forces it on alongside
`ENABLE_TRACY`. Without that the client links but every zone compiles away and captures come back
empty.
