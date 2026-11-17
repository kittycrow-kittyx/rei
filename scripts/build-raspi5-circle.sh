#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "${BASH_SOURCE[0]}")/.."
repo="$PWD"
root="$(realpath "${1:?Usage: $0 ROOT.vfs}")"
test -s "$root"
source_mode="${2:-embedded}"
case "$source_mode" in
  embedded) board=raspi5-circle; root_define= ;;
  sd) board=raspi5-circle-sd; root_define=-DREI_ROOT_SD ;;
  *) echo "Root source must be embedded or sd" >&2; exit 2 ;;
esac
prefix="${PREFIX64:-aarch64-none-elf-}"
jobs="${REI_JOBS:-2}"
[[ "$jobs" =~ ^[1-4]$ ]] || { echo "REI_JOBS must be 1 to 4" >&2; exit 2; }
for tool in "${prefix}gcc" "${prefix}g++" "${prefix}ld" "${prefix}nm" clang make; do
  command -v "$tool" >/dev/null || { echo "Missing tool: $tool" >&2; exit 1; }
done

source scripts/raspi5-driver-bundle.sh
circle_prepare
case "${REI_BUILD_DRIVERS:-0}" in
  0)
    if [[ -e "$repo/prebuilt/raspi5/SHA256SUMS" ]]; then
      bundle="$repo/prebuilt/raspi5"
    else
      bundle="$repo/build/prebuilt/raspi5"
      # First migration: reuse the already compiled archives if available.
      if [[ ! -e "$bundle/SHA256SUMS" ]]; then circle_export "$bundle"; fi
    fi
    ;;
  1)
    circle_compile
    bundle="$repo/build/prebuilt/raspi5"
    circle_export "$bundle"
    ;;
  *) echo "REI_BUILD_DRIVERS must be 0 or 1" >&2; exit 2 ;;
esac
circle_verify "$bundle"
echo "REI: using verified precompiled Circle drivers ($bundle)"
bash scripts/generate-native.sh
generated="$repo/build/native-generated"
out="$repo/build/native/$board"
mkdir -p "$out"
cp "$bundle/circle-drivers.o" "$bundle/driver-archives.txt" "$out/"
if [[ "$root" != "$out/root.vfs" ]]; then cp "$root" "$out/root.vfs"; fi
cp platform/raspi5/circle/{console.cpp,storage.h,network.cpp,network.h} "$out/"
if [[ "$source_mode" == embedded ]]; then
  printf '.section .rodata.root,"a"\n.global rei_root_start,rei_root_end\n.balign 16\nrei_root_start:\n.incbin "root.vfs"\nrei_root_end:\n' > "$out/root.S"
else
  printf '.section .rodata.root,"a"\n.global rei_root_start,rei_root_end\n.balign 16\nrei_root_start:\nrei_root_end:\n' > "$out/root.S"
fi
flags=(--target=aarch64-none-elf -mcpu=cortex-a76 -DREI_PI5 -DREI_CIRCLE -DREI_NATIVE_SHELL -O2 -ffreestanding -fno-builtin -fno-stack-protector -fno-pic -fno-pie -fno-unwind-tables -fno-asynchronous-unwind-tables -mno-outline-atomics -mstrict-align)
if [[ "$source_mode" != embedded ]]; then flags+=(-DREI_ROOT_SD); fi
objects=(console.o network.o root.o circle-drivers.o)
while IFS= read -r -d '' source; do
  relative="${source#"$generated"/}"
  object="rei/${relative%.*}.o"
  mkdir -p "$out/$(dirname "$object")"
  nice -n 10 clang "${flags[@]}" -I"$generated/include" -I"$repo/platform/arm64" -I"$repo/platform/raspi5/circle" -c "$source" -o "$out/$object"
  objects+=("$object")
done < <(find "$generated/src" -type f -name '*.c' -print0 | sort -z; printf '%s\0' runtime/arm64/intrinsics.c platform/arm64/entry.c)
rm -f "$out/console.o" "$out/network.o" "$out/root.o"
nice -n 10 make -C "$out" -f "$repo/platform/raspi5/circle/Makefile" \
  CIRCLEHOME="$circle" REI_DRIVER_BUNDLE="$bundle" EXTRAINCLUDE="-I$repo/platform/arm64 $root_define" \
  OBJS="${objects[*]}" TARGET="rei-$board" -j "$jobs" "rei-$board.img"

end="$("${prefix}nm" "$out/rei-$board.elf" | awk '$3 == "_end" {print $1}')"
[[ "$end" =~ ^[0-9a-fA-F]+$ ]] && (( 16#$end <= 0x80000 + 16777216 )) || {
  echo "Circle kernel exceeds reserved image memory" >&2; exit 1;
}
cp "$generated/revisions.txt" "$out/revisions.txt"
printf 'circle %s\nroot-source %s\n' "$pinned" "$source_mode" >> "$out/revisions.txt"
printf 'circle-bundle %s\n' "$(sha256sum "$bundle/SHA256SUMS" | cut -d' ' -f1)" >> "$out/revisions.txt"
(cd "$out"; sha256sum rei-$board.img rei-$board.elf root.vfs revisions.txt circle-drivers.o driver-archives.txt > SHA256SUMS)
echo "Circle-backed Pi 5 image: $out/rei-$board.img"
