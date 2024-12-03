#!/bin/env bash

src_files=$(find . -type f -name "CMakeLists.txt" -o -iname "*.cmake" -not -path "./.conan/*")
for file in $src_files; do
  cmake-format -i $file
done

