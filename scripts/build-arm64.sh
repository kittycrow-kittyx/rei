#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "${BASH_SOURCE[0]}")/.."
board="${1:-}"
root="${2:-}"
case "$board" in
  raspi3b) target=(-mcpu=cortex-a53) ;;
  raspi5) target=(-mcpu=cortex-a76 -DREI_PI5) ;;
  *) echo "Usage: $0 {raspi3b|raspi5} ROOT.vfs" >&2; exit 2 ;;
esac
test -s "$root"
out="build/native/$board"
mkdir -p "$out"
if [[ "$(realpath "$root")" != "$(realpath "$out/root.vfs")" ]]; then cp "$root" "$out/root.vfs"; fi
printf '.section .rodata.root,"a"\n.global rei_root_start,rei_root_end\n.balign 16\nrei_root_start:\n.incbin "root.vfs"\nrei_root_end:\n' > "$out/root.S"
flags=(--target=aarch64-none-elf "${target[@]}" -O2 -ffreestanding -fno-builtin -fno-stack-protector -fno-pic -fno-pie -fno-unwind-tables -fno-asynchronous-unwind-tables -mno-outline-atomics -mstrict-align -fdata-sections -ffunction-sections)
objects=()
while IFS= read -r -d '' source; do
  object="$out/${source%.*}.o"
  mkdir -p "$(dirname "$object")"
  clang "${flags[@]}" -Iinclude -Iplatform/arm64 -c "$source" -o "$object"
  objects+=("$object")
done < <(find src -name '*.c' -print0 | sort -z; printf '%s\0' runtime/arm64/intrinsics.c platform/arm64/boot.S platform/arm64/entry.c platform/arm64/console.c)
(cd "$out"; clang "${flags[@]}" -c root.S -o root.o)
ld.lld -m aarch64elf -T platform/arm64/linker.ld -o "$out/rei-$board.elf" "${objects[@]}" "$out/root.o"
llvm-objcopy -O binary "$out/rei-$board.elf" "$out/rei-$board.img"
echo "Native image: $out/rei-$board.img"
