#!/bin/bash
source ~/env/bin/activate
conan install . -s build_type=Release -of .conan/gcc   --build=missing -pr:a .conan/gcc/profile
conan install . -s build_type=Debug   -of .conan/gcc   --build=missing -pr:a .conan/gcc/profile
