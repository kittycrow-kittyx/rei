#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "${BASH_SOURCE[0]}")/.."
bash scripts/verify-pins.sh
for dep in thistle bake croissant; do
  test -e "$dep/.git" || { echo "Initialise submodules first: git submodule update --init --recursive" >&2; exit 1; }
  git -C "$dep" diff --quiet
  git -C "$dep" diff --cached --quiet
done
for dep in bake croissant; do
  if [[ ! -x "$dep/node_modules/.bin/tsc" ]]; then
    if [[ -f "$dep/package-lock.json" ]]; then
      nice -n 10 npm ci --prefix "$dep" --no-audit --no-fund
    else
      nice -n 10 npm install --prefix "$dep" --no-audit --no-fund --package-lock=false
    fi
  fi
done
nice -n 10 npm run build --prefix croissant
node croissant/dist/src/thistle-project.js thistle
(cd bake; nice -n 10 bun src/cli.ts --project ../thistle/tsconfig.croissant.json --out-dir build/croissant-bake --target=clang --engine=host)
nice -n 10 node croissant/dist/src/cli.js --manifest thistle/build/croissant-bake/bake-manifest.json --out-dir build/native-generated
test -s build/native-generated/src/native/kernel.c
grep -q 'uint64_t handle;' build/native-generated/include/croissant/types.h
if grep -q 'floatBuffer' build/native-generated/src/globals.c; then
  echo "Unexpected pre-fix Thistle allocation in generated globals" >&2; exit 1
fi
for dep in thistle bake croissant; do
  printf '%s %s\n' "$dep" "$(git -C "$dep" rev-parse HEAD)"
done > build/native-generated/revisions.txt
