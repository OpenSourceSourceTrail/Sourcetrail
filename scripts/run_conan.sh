#!/bin/bash
source ~/env/bin/activate
# Linux needs exactly one dependency set: GCC / Release. Debug builds reuse it through the
# `gnu_debug` CMake preset -- do not add a `-s build_type=Debug` install here.
conan install . -s build_type=Release -of .conan/gcc --build=missing -pr:a .conan/gcc/profile
