#!/usr/bin/env bash
set -euo pipefail

cd "$(dirname "${BASH_SOURCE[0]}")/.."

for dependency in thistle bake croissant; do
  case "$dependency" in
    thistle) label=Thistle ;;
    bake) label=Bake ;;
    croissant) label=Croissant ;;
  esac

  pinned="$(git ls-tree HEAD "$dependency" | awk '$1 == "160000" && $2 == "commit" { print $3 }')"
  if [[ ! "$pinned" =~ ^[a-f0-9]{40}$ ]]; then
    echo "Missing pinned Git submodule: $dependency" >&2
    exit 1
  fi

  if ! grep -Fq "| $label | $pinned |" docs/provenance.md; then
    echo "Provenance mismatch: $dependency has gitlink $pinned" >&2
    exit 1
  fi

  if ! grep -Fq "path = $dependency" .gitmodules; then
    echo "Missing .gitmodules path: $dependency" >&2
    exit 1
  fi

  if [[ -e "$dependency/.git" ]]; then
    checked="$(git -C "$dependency" rev-parse HEAD)"
    if [[ "$checked" != "$pinned" ]]; then
      echo "Checked out $dependency at $checked, expected $pinned" >&2
      exit 1
    fi
  fi

  printf '%-10s %s\n' "$dependency" "$pinned"
done

# Verify the contract gitlinks, not the latest revision of KITTYX.
abi=
for dependency in thistle bake croissant; do
  if [[ "$dependency" == thistle ]]; then contract=kittyx; else contract=vendor/kittyx; fi
  # The initial provenance check also runs before submodule initialisation.
  [[ -e "$dependency/.git" ]] || continue
  revision="$(git -C "$dependency" ls-tree HEAD "$contract" | awk '$1 == "160000" && $2 == "commit" { print $3 }')"
  [[ "$revision" =~ ^[a-f0-9]{40}$ ]] || { echo "Missing KITTYX gitlink: $dependency/$contract" >&2; exit 1; }
  grep -Fq "| KITTYX | $revision |" docs/provenance.md || { echo "KITTYX provenance mismatch: $dependency" >&2; exit 1; }
  if [[ -n "$abi" && "$abi" != "$revision" ]]; then
    echo "Incompatible KITTYX pins: $dependency" >&2
    exit 1
  fi
  abi="$revision"
  if [[ -e "$dependency/$contract/.git" ]]; then
    [[ "$(git -C "$dependency/$contract" rev-parse HEAD)" == "$abi" ]] || { echo "Incorrect KITTYX checkout: $dependency" >&2; exit 1; }
    git -C "$dependency/$contract" diff --quiet
    git -C "$dependency/$contract" diff --cached --quiet
  fi
done
if [[ -n "$abi" ]]; then printf '%-10s %s\n' kittyx "$abi"; fi
