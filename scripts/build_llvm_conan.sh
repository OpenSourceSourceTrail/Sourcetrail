#!/bin/bash
# Builds (or reuses, once cached) LLVM/Clang for Sourcetrail's C/C++ indexer through Conan 2,
# using the same GCC profile as the rest of the project. The first run compiles LLVM from
# source and takes hours; every later run is a cache hit.
set -euo pipefail

REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
RECIPE_DIR="${REPO_ROOT}/.conan/recipes/llvm-clang"
REFERENCE="llvm-clang/23.1.0"
LLVM_BUILD_JOBS="${LLVM_BUILD_JOBS:-}"

cd "${REPO_ROOT}"

mkdir -p .conan/llvm

conan export "${RECIPE_DIR}"
conan install --requires="${REFERENCE}" --build=missing \
  -s build_type=Release -pr:a .conan/gcc/profile -of .conan/llvm \
  ${LLVM_BUILD_JOBS:+-c tools.build:jobs="${LLVM_BUILD_JOBS}"} \
  --format=json >.conan/llvm/graph.json

# `conan cache path <ref>` resolves the *recipe* folder, so take the binary package folder
# out of the install graph instead.
PACKAGE_DIR="$(python3 -c '
import json, sys
graph = json.load(open(".conan/llvm/graph.json"))["graph"]["nodes"]
for node in graph.values() if isinstance(graph, dict) else graph:
    if node.get("ref", "").startswith(sys.argv[1] + "#"):
        print(node.get("package_folder") or "")
        break
' "${REFERENCE}")"

if [[ -z "${PACKAGE_DIR}" || "${PACKAGE_DIR}" == "null" || ! -d "${PACKAGE_DIR}" ]]; then
  echo "error: could not resolve the ${REFERENCE} package folder from .conan/llvm/graph.json" >&2
  exit 1
fi

# The `build_cxx` CMake presets default Clang_DIR to <repo>/external/lib/cmake/clang/, and the
# packaged install tree has exactly that layout -- so pointing `external` at the Conan package
# makes `cmake --preset=ci_gnu_release_build_cxx` work with no extra flags.
ln -sfn "${PACKAGE_DIR}" "${REPO_ROOT}/external"

echo
echo "LLVM/Clang package: ${PACKAGE_DIR}"
echo "Clang_DIR:          ${PACKAGE_DIR}/lib/cmake/clang"
echo "external/ now points at it; 'cmake --preset=ci_gnu_release_build_cxx' needs no -DClang_DIR."
