#!/bin/env bash

find . -type f -name "*.cpp" -or -name "*.h" -or -name "*.hpp" -exec clang-format -i {} \;
