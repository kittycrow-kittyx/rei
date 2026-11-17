#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "${BASH_SOURCE[0]}")/.."
repo="$PWD"
prefix="${PREFIX64:-aarch64-none-elf-}"
jobs="${REI_JOBS:-2}"
[[ "$jobs" =~ ^[1-4]$ ]] || exit 2
for tool in "${prefix}gcc" "${prefix}ld"; do command -v "$tool" >/dev/null; done
source scripts/raspi5-driver-bundle.sh
circle_prepare
dest="$repo/prebuilt/raspi5"
circle_export "$dest"
circle_verify "$dest"
echo "Exported existing compiled Circle archives to $dest"
du -h "$dest/circle-drivers.o" "$dest"/*/*/*.a
