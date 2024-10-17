#!/bin/env bash

CHECK_PATH="."
EXCLUDE_REGEX="bin"
INCLUDE_REGEX='^.*\.((((c|C)(c|pp|xx|\+\+)?$)|((h|H)h?(pp|xx|\+\+)?$))|(ino|pde|proto|cu))$'

src_files=$(find "$CHECK_PATH" -name .git -prune -o -regextype posix-egrep -regex "$INCLUDE_REGEX" -print)
IFS=$'\n' # Loop below should separate on new lines, not spaces.
for file in $src_files; do
	# Only check formatting if the path doesn't match the regex
	if ! [[ ${file} =~ $EXCLUDE_REGEX ]]; then
		clang-format -i --style=file --fallback-style="Google" "${file}"
	fi
done
