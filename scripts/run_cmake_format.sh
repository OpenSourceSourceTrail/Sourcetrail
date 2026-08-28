#!/bin/env bash
# Formats every tracked CMakeLists.txt / *.cmake in place. Tracked only: build/ and .conan/ hold
# generated files that must not be rewritten.
set -euo pipefail

cd "$(dirname "$0")/.."
git ls-files -z '*CMakeLists.txt' '*.cmake' | xargs -0 cmake-format -i
