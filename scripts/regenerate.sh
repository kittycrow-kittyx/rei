#!/usr/bin/env bash
set -euo pipefail

cd "$(dirname "${BASH_SOURCE[0]}")/.."

bash scripts/verify-pins.sh
git submodule update --init --recursive
bash scripts/verify-pins.sh

if [[ -n "$(git status --porcelain --untracked-files=no --ignore-submodules=dirty)" ]]; then
  echo "Refusing to regenerate over modifications to tracked files" >&2
  exit 1
fi

command -v bun >/dev/null
command -v node >/dev/null
command -v npm >/dev/null

npm install --prefix bake
npm install --prefix croissant
npm run build --prefix croissant

node croissant/dist/src/thistle-project.js thistle
(
  cd bake
  bun src/cli.ts --project ../thistle/tsconfig.croissant.json \
    --out-dir build/croissant-bake --target=clang --engine=host
)

node croissant/dist/src/cli.js \
  --manifest thistle/build/croissant-bake/bake-manifest.json \
  --out-dir build/croissant-output

# Only files declared by the old generated manifest may be deleted.
# Only the compiler owns the generated kernel tree.
node --input-type=module - build/croissant-output <<'NODE'
import fs from "node:fs";
import path from "node:path";

const root = process.cwd();
const output = path.resolve(process.argv[2]);
const previous = JSON.parse(fs.readFileSync("croissant.manifest.json", "utf8"));
const generated = JSON.parse(
  fs.readFileSync(path.join(output, "croissant.manifest.json"), "utf8")
);
const valid = (relative) => {
  if (!/^(src|include)\/[A-Za-z0-9_./-]+\.(c|h)$/.test(relative) ||
      relative.split("/").includes("..")) {
    throw new Error("Unsafe generated path: " + relative);
  }
  return relative;
};

for (const relative of [...previous.sources, ...previous.headers]) {
  fs.rmSync(path.join(root, valid(relative)), { force: true });
}
for (const relative of [...generated.sources, ...generated.headers]) {
  const destination = path.join(root, valid(relative));
  fs.mkdirSync(path.dirname(destination), { recursive: true });
  fs.copyFileSync(path.join(output, relative), destination);
}

const originalCMake = fs.readFileSync(path.join(output, "CMakeLists.txt"), "utf8");
if (!originalCMake.includes("project(croissant_thistle LANGUAGES C)") ||
    !originalCMake.includes("add_library(croissant_thistle OBJECT")) {
  throw new Error("Unexpected Croissant CMake structure; refusing to rewrite");
}
fs.writeFileSync("CMakeLists.txt", originalCMake
  .replace("project(croissant_thistle LANGUAGES C)", "project(rei LANGUAGES C)")
  .replaceAll("croissant_thistle", "rei_kernel"));
fs.copyFileSync(path.join(output, "croissant.manifest.json"), "croissant.manifest.json");
NODE

cp thistle/build/croissant-bake/bake-manifest.json docs/bake-manifest.json

# Regeneration stops at source. The provenance file is deterministic.
{
  find src include -type f
  printf '%s\n' croissant.manifest.json docs/bake-manifest.json
} | LC_ALL=C sort | while IFS= read -r file; do sha256sum "$file"; done > SHA256SUMS

sha256sum -c SHA256SUMS >/dev/null
bash scripts/verify-pins.sh
echo "Rei source regenerated from pinned dependencies"
