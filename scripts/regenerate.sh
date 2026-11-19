#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "${BASH_SOURCE[0]}")/.."
if [[ -n "$(git status --porcelain --untracked-files=all -- src include CMakeLists.txt croissant.manifest.json docs/bake-manifest.json docs/provenance.md SHA256SUMS)" ]]; then
  echo "Refusing to overwrite modified generated source" >&2
  exit 1
fi
case "${1:-}" in
  "") git submodule update --init --recursive
      bash scripts/verify-pins.sh
      bash scripts/fetch-upstream.sh
      bun install --cwd build/upstream/croissant --frozen-lockfile
      bun install --cwd build/upstream/croissant/bake --frozen-lockfile ;;
  --source-only) bash scripts/verify-pins.sh ;;
  *) echo "Usage: $0 [--source-only]" >&2; exit 2 ;;
esac
for dep in thistle croissant; do
  [[ "$(cat "build/upstream/$dep/RELEASE_COMMIT")" == "$(git -C "$dep" rev-parse HEAD)" ]]
done
[[ "$(cat build/upstream/croissant/bake/RELEASE_COMMIT)" == "$(git -C croissant/bake rev-parse HEAD)" ]]
rm -rf "build/croissant-output" "build/upstream/thistle/build/croissant-bake"
bun build/upstream/croissant/src/thistle-project.ts build/upstream/thistle
nice -n 10 bun build/upstream/croissant/bake/src/cli.ts --project build/upstream/thistle/tsconfig.croissant.json \
  --out-dir build/croissant-bake --target=clang --engine=host
nice -n 10 bun build/upstream/croissant/src/cli.ts \
  --manifest build/upstream/thistle/build/croissant-bake/bake-manifest.json --out-dir build/croissant-output
test -s build/croissant-output/CMakeLists.txt
test -s build/croissant-output/croissant.manifest.json
test -d build/croissant-output/include/croissant
test -d build/croissant-output/src
grep -Fq 'project(croissant_thistle LANGUAGES C)' build/croissant-output/CMakeLists.txt
grep -Fq 'add_library(croissant_thistle OBJECT' build/croissant-output/CMakeLists.txt
rm -rf src include
cp -R build/croissant-output/src build/croissant-output/include .
sed -e 's/project(croissant_thistle LANGUAGES C)/project(rei LANGUAGES C)/' -e 's/croissant_thistle/rei_kernel/g' build/croissant-output/CMakeLists.txt > CMakeLists.txt
cp build/croissant-output/croissant.manifest.json .
mkdir -p docs
cp build/upstream/thistle/build/croissant-bake/bake-manifest.json docs/bake-manifest.json
{
  printf '# Source provenance\n\n| Component | Git revision |\n|---|---|\n'
  printf '| Thistle | %s |\n' "$(git -C thistle rev-parse HEAD)"
  printf '| Bake | %s |\n' "$(git -C croissant/bake rev-parse HEAD)"
  printf '| Croissant | %s |\n' "$(git -C croissant rev-parse HEAD)"
  printf '\nGenerated source uses the verified releases in upstream.releases.json. Bake is inherited through Croissant.\n'
} > docs/provenance.md
{ find src include -type f; printf '%s\n' CMakeLists.txt croissant.manifest.json docs/bake-manifest.json docs/provenance.md; } | LC_ALL=C sort | xargs sha256sum > SHA256SUMS
sha256sum -c SHA256SUMS
bash scripts/verify-pins.sh
