---
name: grpc-ipc
description: Sourcetrail gRPC cross-process IPC — engine.proto, indexer_worker.proto, proto Convert helpers, EngineServiceImpl. Use when touching *.proto files, engine daemon, indexer worker communication, or storage<->proto conversion.
---

# gRPC IPC

Cross-process communication uses gRPC (1.54.3) with two boundaries, defined in `src/lib/proto/*.proto`:

## Boundary A — client (GUI/CLI) ↔ engine daemon

`EngineService` (`engine.proto`): project lifecycle (`LoadProject`, `Refresh`), bookmark mutations, and read-only storage/graph queries. Implemented in `engine/EngineServiceImpl.cpp`.

## Boundary B — engine (server) ↔ indexer worker (client)

`IndexerWorkerService` (`indexer_worker.proto`): workers `PullCommand`, `PushIntermediateStorage`, `ReportStatus`, and `WatchInterrupt` (server-streaming interrupt signal).

## Shared types & conversion

- Shared message types live in `sourcetrail_common.proto`.
- `proto/Convert.{h,cpp}` (`proto::convert::toProto`/`fromProto`) maps storage POD types and `IntermediateStorage` to/from their proto equivalents.
- **Keep proto conversion helpers dependency-light** — `lib` ↔ `proto_convert` had a circular dependency broken via an include-dir generator expression.

## Processes

- `Sourcetrail` (GUI) / `Sourcetrail_cli` — clients
- `Sourcetrail_engine` — engine daemon (gRPC server for clients, entry point in `src/app/engine/`)
- `Sourcetrail_indexer` — indexer worker process (entry point in `src/app/indexer/`)
