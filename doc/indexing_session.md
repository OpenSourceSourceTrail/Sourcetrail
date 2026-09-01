# Indexing: where the wall clock actually goes

Notes from an optimization pass over the *indexing* path, the sequel to `sqlite_session.md`. Same
format: what was wrong, what was measured, what was deliberately left alone.

The short version: `sqlite_session.md` concluded that "indexing wall time is dominated by Clang
parsing". That was wrong. Indexing was dominated by the **engine-side merge/inject pipeline**, and
roughly a third of the wall clock was a `sleep_for` that fired after every successful iteration.

## The arithmetic that did not add up

`sqlite_session.md` reported ~14s of `parse_ms` per worker across ~40 workers (~560s of parse CPU)
and 90-150s of `receive_ms`, against a 155s wall on a 64-core machine. That is ~4.4x effective
parallelism on 64 cores: the workers were idle most of their lives. Something serialized.

Nothing in the tree could say what. `parse_ms` / `serialize_ms` / `push_ms` / `receive_ms` are
process-lifetime sums; the merge stage, the inject stage, cache building and `VACUUM` were not
measured at all.

## Instrumentation first

`data/IndexingPhaseStats.{h,cpp}` adds counters for the stages nobody could see, printed as two more
`INDEXER_TIMING` lines that `scripts/bench_index.sh` already greps for:

```
INDEXER_TIMING pipeline merges=N merge_ms=… merge_idle_ms=… injects=N inject_ms=… inject_idle_ms=… max_queue_depth=N
INDEXER_TIMING caches file_path_maps_ms=… search_index_ms=… member_edge_order_ms=… hierarchy_ms=… full_text_ms=… optimize_ms=…
```

`GrpcIndexer` gained `pull_ms` (time a worker sat blocked in `PullCommand`), so a worker's whole life
is now accounted for. `Project::index` prints the lines after the final `buildCaches`, the only point
where everything — merge, inject, caches, `VACUUM` — has finished.

The first run answered the question immediately. Cold full index of this repository, 435 source
files, 64-core Linux box:

| stage | |
|---|---|
| wall | 143.4s |
| merge | 205 merges, 62.5s of work, **51.3s of sleep** |
| inject | 231 injects, 91.5s of work, 5.8s of sleep |
| max storage queue depth | 298 |
| all four cache builders | 2.1s |
| `VACUUM` (`optimizeMemory`) | 2.9s |

Injection alone is 64% of the wall clock, on one thread. Cache building and `VACUUM` — the phases
`sqlite_session.md` flagged for a later look — are 5s of 143s together, and are not worth touching.

## Fix 1: the repeat decorator throttled the work path

`TaskDecoratorRepeat::doUpdate` sleeps its delay after **every** iteration, successful ones included.
The merge loop was built with 250ms and the inject loops with 25ms
(`IndexTaskBuilder::addIndexerPipeline`), so merges were capped at 4/s and injections at 40/s no
matter how fast they actually ran.

The delay is only there to stop a *failing* child from spinning. The selector those loops sit under
returns `SUCCESS` both when the stage did work and when there was nothing to do, so the decorator
cannot tell them apart — but the stage itself can: `TaskMergeStorages` / `TaskInjectStorage` return
`FAILURE` exactly when they found no work. The backoff moved there, and the three
`createRepeat` delays became 0.

`TaskDecoratorRepeat` itself is unchanged: other call sites still rely on its delay.

| | wall | merge | inject |
|---|---|---|---|
| before | 143.4s | 62.5s + 51.3s sleep | 91.5s + 5.8s sleep |
| after | **123.5s** | 77.5s | 71.2s |

## Fix 2: one merger cannot feed the injector

With the throttle gone, merging became the top cost: one merge thread doing 77.5s of work while the
injector did 71.2s, with the storage queue still 281 deep.

First the obvious question — is merging worth anything at all? It exists only to reduce the number
of `PersistentStorage::inject` calls, and it re-copies every symbol it touches. Removing the stage
entirely answered it:

| | wall | injects | inject_ms |
|---|---|---|---|
| no merge stage | 195.8s | 436 | 155.7s |

So merging pays for itself several times over; it just needed more than one thread. `StorageProvider`
already guards its deque with a mutex and each merge takes its own pair out of it, so running four
merge branches instead of one needs no locking change — only `MergeBranchCount` and a loop in
`addIndexerPipeline`. The taskflow executor already floors its pool at
`max(hardware_concurrency, 8)`, and each branch holds a worker for the whole run, which is why this
number cannot grow freely.

| | wall | merges | merge_ms | injects | inject_ms | max queue depth |
|---|---|---|---|---|---|---|
| 1 branch | 123.5s | 363 | 77.5s | 73 | 71.2s | 281 |
| 4 branches | **104.7s** | 392 | 144.6s | 44 | 71.3s | 114 |

## Result

| | wall | indexing phase |
|---|---|---|
| before | 143.4s | 02:23 |
| after | **103.8s** median of 104.7 / 103.8 / 100.3 | 01:32-01:36 |

**27% off the wall clock**, from scheduling changes only. No schema change, no `kStorageVersion`
bump, no SQLite tuning, nothing touching what gets written.

## Deliberately not done

| Skipped | Why | Revisit when |
|---|---|---|
| **Parallelising injection** | 71.3s on one thread is now 74% of the indexing phase and the hard floor — but it is one SQLite connection inside one transaction per storage. Splitting it is a real design change, not a scheduling tweak. | It is the only thing left that matters. Start with `Storage::inject`'s two `std::map<Id, Id>` remap tables and `SqliteIndexStorage`'s lazy full-table temp indexes. |
| **More merge branches** | `merge_idle_ms` was already 89.6s across the four, so they are partly starved; and no amount of merging gets below the injector's 71s. | Only alongside a faster injector. |
| **`buildCaches`, `VACUUM`** | 5s of 143s together, measured. `sqlite_session.md` listed both as unmeasured unknowns; they are now measured and small. | Not worth it. |
| **The unary per-file gRPC boundary** | 3-4 round trips per file, no streaming. `push_ms` is 1-4s per worker process against 7-10s of `parse_ms`, so it is not the bottleneck. | If the injector ever stops dominating. |
| **A profiler** | The phase counters answered the question outright. `perf` was ready to go (`-g -fno-omit-frame-pointer`, since no preset builds with symbols) and was never needed. | If a fix ever needs to know *why* a stage is slow, rather than *which* stage is slow. |

## Reproducing

```
scripts/bench_index.sh <build-dir> <project>.srctrlprj 1
```

It deletes the `.srctrldb` before each run, so use a scratch copy of a project directory. The three
`INDEXER_TIMING` groups — per worker process, `engine`, `pipeline`/`caches` — should sum to roughly
the wall time it prints.

Note that the indexed-file and error counts of this repository drift run to run (102-121 fatal
errors across the runs above) because some translation units crash the worker nondeterministically.
That predates this change; do not read it as a regression signal. Row counts are stable to within
those error rows -- 281,515-281,516 nodes, 954,974 edges, 2,014,294-2,014,295 occurrences across
three runs -- because each recorded error is itself an element.
