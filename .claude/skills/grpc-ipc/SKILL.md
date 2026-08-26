---
name: grpc-ipc
description: Sourcetrail cross-process IPC — the HTTP+JSON client/engine boundary, the gRPC engine/indexer boundary, engine.proto, indexer_worker.proto, proto Convert helpers, EngineHttpService. Use when touching *.proto files, engine daemon, indexer worker communication, or storage<->proto conversion.
---

# Cross-process IPC

Two boundaries, with **different transports**. Message schemas for both live in `src/lib/proto/*.proto`.

## Boundary A — client (GUI/web app) ↔ engine daemon: HTTP + JSON

Not gRPC: a browser cannot speak it, and the point of this boundary is that a web app can replace or
sit alongside the Qt GUI.

- Routes are registered in `src/app/engine/EngineHttpService.cpp` under `/api/v1/`; transport lives in
  `src/lib/core/http` (Boost.Beast, no new dependency).
- `engine.proto` declares **no service** — it is the payload schema only, encoded with protobuf's
  canonical JSON mapping through `proto/ProtoJson.{h,cpp}`. **uint64 fields render as JSON strings**,
  so ids arrive quoted.
- Endpoints are cut per *use*, not per storage method: opening a file is one request
  (`GET /api/v1/files/{path}`), not four; `POST /api/v1/symbols/resolve` batches the per-id lookups.
  When adding a call, look for an existing endpoint to extend before adding a route.
- Engine → client push is **server-sent events** on `GET /api/v1/events`. Questions the engine blocks
  on go out as a `dialog` event and come back via `POST /api/v1/dialogs/{id}`.
- Every request needs `Authorization: Bearer <token>` — the token from the engine's
  `ENGINE_PORT <n> <token>` handshake line. Requests with an un-allow-listed `Origin` are rejected.
- Client side: `EngineChannel` (one keep-alive connection per calling thread), `EngineCall.h`
  (`call<T>` / `callVoid` — the one place a failure becomes an empty answer), `HttpStorageAccess`,
  `HttpProject`, `EngineEventClient`.

## Boundary B — engine (server) ↔ indexer worker (client): gRPC

Still gRPC (1.54.3), and deliberately so: this is bulk binary process-to-process traffic with streaming,
which is what gRPC is good at. `indexer_worker.proto` is the only file that still generates stubs.

`IndexerWorkerService` (`indexer_worker.proto`): workers `PullCommand`, `PushIntermediateStorage`, `ReportStatus`, and `WatchInterrupt` (server-streaming interrupt signal).

## Shared types & conversion

- Shared message types live in `sourcetrail_common.proto`.
- `proto/Convert.{h,cpp}` (`proto::convert::toProto`/`fromProto`) maps storage POD types and `IntermediateStorage` to/from their proto equivalents.
- **Keep proto conversion helpers dependency-light** — `lib` ↔ `proto_convert` had a circular dependency broken via an include-dir generator expression.

## Processes

- `Sourcetrail` (GUI) — the only client of boundary A. `Sourcetrail_cli` is **not** a client: it opens the database in-process via `lib_engine` and links neither `Sourcetrail_client` nor gRPC.
- `Sourcetrail_engine` — engine daemon (HTTP server for clients, gRPC server for workers; entry point in `src/app/engine/`)
- `Sourcetrail_indexer` — indexer worker process (entry point in `indexers/cxx/indexer/`)
