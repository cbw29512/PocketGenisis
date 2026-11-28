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
    # FastNoiseLite's audited Perlin hash uses intentional signed wrap.
    # Apply flags only to the isolated candidate; keep all other tests strict.
    extra=()
    if [[ "$name" == terrain_probe_test || "$name" == terrain_ecology_test || "$name" == freshwater_test || "$name" == soil_fertility_test || "$name" == fatigue_test || "$name" == world_admission_test || "$name" == hydrology_test || "$name" == integrated_world_test || "$name" == integrated_checkpoint_test ]]; then
        extra=(-fwrapv -ffp-contract=off)
    fi
    "${CXX:-g++}" -std=c++17 -Wall -Wextra -Werror -pedantic "${extra[@]}" \
        -fsanitize=undefined -fno-sanitize-recover=undefined -Icore "$source" -o "$build_dir/$name"
    "$build_dir/$name"
done
