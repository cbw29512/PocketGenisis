#!/usr/bin/env bash
set -euo pipefail
# Use a temporary output directory so tests never leave binaries in the source tree.
cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.."
build_dir=$(mktemp -d)
trap 'rm -rf -- "$build_dir"' EXIT
trap 'echo "Host test failed at line $LINENO" >&2' ERR
for source in tests/*_test.cpp; do
    name=$(basename "$source" .cpp)
    echo "== $name =="
    "${CXX:-g++}" -std=c++17 -Wall -Wextra -Werror -pedantic \
        -fsanitize=undefined -fno-sanitize-recover=undefined -Icore "$source" -o "$build_dir/$name"
    "$build_dir/$name"
done
