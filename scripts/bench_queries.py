#!/usr/bin/env python3
"""Runs Sourcetrail_bench with an engine on the other end.

Spawning the engine, parsing its `ENGINE_PORT <port> <token>` handshake and loading a project into
it is what tests/integration/engine_http/engine_harness.py already does, so this borrows it rather
than repeating the dance in C++.

    scripts/bench_queries.py --build <build-dir> --project <path>.srctrlprj [--iterations N]
"""

import argparse
import os
import subprocess
import sys

HARNESS = os.path.join(
    os.path.dirname(os.path.dirname(os.path.abspath(__file__))),
    "tests", "integration", "engine_http")
sys.path.insert(0, HARNESS)

from engine_harness import EngineProcess    # noqa: E402


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--build", required=True, help="build directory (its app/ holds the binaries)")
    parser.add_argument("--project", required=True, help="an indexed .srctrlprj")
    parser.add_argument("--iterations", type=int, default=20)
    args = parser.parse_args()

    app_dir = os.path.join(os.path.abspath(args.build), "app")
    project = os.path.abspath(args.project)
    database = os.path.splitext(project)[0] + ".srctrldb"
    if not os.path.exists(database):
        sys.exit(f"no index next to the project: {database}")

    engine = EngineProcess(binary=os.path.join(app_dir, "sourcetrail_engine"))
    try:
        response = engine.load_project(project)
        if response.status != 200:
            sys.exit(f"engine refused the project ({response.status}): {response.body}")
        subprocess.run([
            os.path.join(app_dir, "Sourcetrail_bench"),
            "--db", database,
            "--endpoint", f"127.0.0.1:{engine.port}",
            "--token", engine.token,
            "--iterations", str(args.iterations),
        ], check=True)
    finally:
        engine.kill()


if __name__ == "__main__":
    main()
