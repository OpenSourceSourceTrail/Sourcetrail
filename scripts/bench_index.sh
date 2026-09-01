#!/usr/bin/env bash
# Times a full index and prints the per-phase split the INDEXER_TIMING lines carry.
#
#   scripts/bench_index.sh <build-dir> <project.srctrlprj> [runs]
#
# The database is deleted before each run, so every run is a cold full index. Sourcetrail_cli hosts
# the indexer gRPC server itself, exactly as the engine does, which is why it is the harness here.
set -euo pipefail

build="$(realpath "$1")"
project="$(realpath "$2")"
runs="${3:-3}"

database="${project%.srctrlprj}.srctrldb"
cli="${build}/app/sourcetrail_cli"

for run in $(seq 1 "$runs"); do
  rm -f "${database}" "${database}_tmp"
  log="$(mktemp)"
  start=$(date +%s.%N)
  (cd "${build}/app" && "${cli}" index --full "${project}") > "${log}" 2>&1 || true
  end=$(date +%s.%N)

  printf 'run %s: %.1fs\n' "${run}" "$(echo "${end} - ${start}" | bc)"
  grep 'INDEXER_TIMING' "${log}" || echo '  (no INDEXER_TIMING lines -- was this built with the instrumentation?)'
  grep -i 'Finished indexing' "${log}" || true
  rm -f "${log}"
done
