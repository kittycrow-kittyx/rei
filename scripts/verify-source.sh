#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "${BASH_SOURCE[0]}")/.."
test -s CMakeLists.txt
test -s croissant.manifest.json
test -d include/croissant
sha256sum -c SHA256SUMS
test -z "$(git ls-files -- dist platform target '*.o' '*.a' '*.so' '*.elf' '*.img' '*.bin' '*.wasm' '*.rlib')"
expected="$(mktemp)"
trap 'rm -f "$expected"' EXIT
{ find src include -type f; printf '%s\n' CMakeLists.txt croissant.manifest.json docs/bake-manifest.json docs/provenance.md; } | LC_ALL=C sort > "$expected"
cut -d ' ' -f 3- SHA256SUMS | LC_ALL=C sort | cmp - "$expected"
