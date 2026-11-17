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

pinned="$(git ls-tree HEAD vendor/circle | awk '$1 == "160000" {print $3}')"
test "$pinned" = "7a81e1b480fe7d19fc1fc695b89bff0a0848f44b"
git submodule update --init --jobs "$jobs" -- vendor/circle
test "$(git -C vendor/circle rev-parse HEAD)" = "$pinned"
git -C vendor/circle diff --quiet
git -C vendor/circle diff --cached --quiet

key="$(printf '%s\n' "$pinned" "$prefix" "$("${prefix}gcc" -dumpfullversion)" 'pi5-16m-v3' | sha256sum | cut -c1-16)"
circle="$repo/build/circle/$key"
if [[ ! -f "$circle/Rules.mk" ]]; then
  mkdir -p "$circle"
  git -C vendor/circle archive "$pinned" | tar -xf - -C "$circle"
fi
cat > "$circle/Config.mk" <<EOF
AARCH = 64
RASPPI = 5
PREFIX64 = $prefix
STDLIB_SUPPORT = 1
DEFINE += -DKERNEL_MAX_SIZE=16777216 -DDEFAULT_KEYMAP=\"UK\"
EOF

# Configure only the cached Circle build copy, not the submodule.
sed -i 's/^#define FF_FS_READONLY[[:space:]]*0$/#define FF_FS_READONLY 1/' "$circle/addon/fatfs/ffconf.h"
grep -Eq '^#define FF_FS_READONLY[[:space:]]+1$' "$circle/addon/fatfs/ffconf.h"
for library in lib lib/usb lib/input lib/fs addon/SDCard addon/fatfs lib/sched lib/net addon/wlan; do
  nice -n 10 make -C "$circle/$library" -j "$jobs"
done

# Only driver support objects are referenced; no Circle guest IP stack.
wlan_libs="$circle/addon/wlan/libwlan.a $circle/lib/net/libnet.a $circle/lib/sched/libsched.a"
bash scripts/generate-native.sh
generated="$repo/build/native-generated"
out="$repo/build/native/$board"
mkdir -p "$out"
if [[ "$root" != "$out/root.vfs" ]]; then cp "$root" "$out/root.vfs"; fi
cp platform/raspi5/circle/{console.cpp,storage.h,network.cpp,network.h} "$out/"
if [[ "$source_mode" == embedded ]]; then
  printf '.section .rodata.root,"a"\n.global rei_root_start,rei_root_end\n.balign 16\nrei_root_start:\n.incbin "root.vfs"\nrei_root_end:\n' > "$out/root.S"
else
  printf '.section .rodata.root,"a"\n.global rei_root_start,rei_root_end\n.balign 16\nrei_root_start:\nrei_root_end:\n' > "$out/root.S"
fi
flags=(--target=aarch64-none-elf -mcpu=cortex-a76 -DREI_PI5 -DREI_CIRCLE -O2 -ffreestanding -fno-builtin -fno-stack-protector -fno-pic -fno-pie -fno-unwind-tables -fno-asynchronous-unwind-tables -mno-outline-atomics -mstrict-align)
if [[ "$source_mode" != embedded ]]; then flags+=(-DREI_ROOT_SD); fi
objects=(console.o network.o root.o)
while IFS= read -r -d '' source; do
  relative="${source#"$generated"/}"
  object="rei/${relative%.*}.o"
  mkdir -p "$out/$(dirname "$object")"
  nice -n 10 clang "${flags[@]}" -I"$generated/include" -I"$repo/platform/arm64" -I"$repo/platform/raspi5/circle" -c "$source" -o "$out/$object"
  objects+=("$object")
done < <(find "$generated/src" -type f -name '*.c' -print0 | sort -z; printf '%s\0' runtime/arm64/intrinsics.c platform/arm64/entry.c)
rm -f "$out/console.o" "$out/network.o" "$out/root.o"
fatfs_lib="$circle/addon/fatfs/libfatfs.a"
nice -n 10 make -C "$out" -f "$repo/platform/raspi5/circle/Makefile" \
  CIRCLEHOME="$circle" EXTRAINCLUDE="-I$repo/platform/arm64 $root_define" \
  REI_WLAN_LIBS="$wlan_libs" REI_FATFS_LIB="$fatfs_lib" OBJS="${objects[*]}" TARGET="rei-$board" -j "$jobs" "rei-$board.img"

end="$("${prefix}nm" "$out/rei-$board.elf" | awk '$3 == "_end" {print $1}')"
[[ "$end" =~ ^[0-9a-fA-F]+$ ]] && (( 16#$end <= 0x80000 + 16777216 )) || {
  echo "Circle kernel exceeds reserved image memory" >&2; exit 1;
}
cp "$generated/revisions.txt" "$out/revisions.txt"
printf 'circle %s\nroot-source %s\n' "$pinned" "$source_mode" >> "$out/revisions.txt"
(cd "$out"; sha256sum rei-$board.img rei-$board.elf root.vfs revisions.txt > SHA256SUMS)
echo "Circle-backed Pi 5 image: $out/rei-$board.img"
