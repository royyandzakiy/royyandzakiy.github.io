#!/usr/bin/env sh
# Build every demo under _demos/ into assets/demos/<demo>/.
# Needs clang with the wasm32 target and wasm-ld (stock LLVM has both).
set -eu

root=$(cd "$(dirname "$0")/.." && pwd)
CXX=${CXX:-clang++}

for dir in "$root"/_demos/*/; do
    name=$(basename "$dir")
    out="$root/assets/demos/$name"

    # Freestanding wasm: every .cpp in the demo folder becomes one .wasm
    for src in "$dir"*.cpp; do
        [ -e "$src" ] || continue
        mkdir -p "$out"
        wasm="$out/$(basename "$src" .cpp).wasm"
        "$CXX" -std=c++23 --target=wasm32 -O2 -ffreestanding -nostdlib \
            -fno-exceptions -fno-rtti -Wall -Wextra \
            -Wl,--no-entry -Wl,--strip-all \
            -o "$wasm" "$src"
        echo "$name: $(basename "$wasm") ($(wc -c < "$wasm") bytes)"
    done
done
