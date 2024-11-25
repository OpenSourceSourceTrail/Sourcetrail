#!/bin/env bash

find . -type f -name "" -iname "*.cmake" -exec cmake-format -i {} \;
