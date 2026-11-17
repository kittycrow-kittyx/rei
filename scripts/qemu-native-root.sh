#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "${BASH_SOURCE[0]}")/.."
bash scripts/build-arm64.sh raspi3b "${1:?root image required}"
out=build/native/raspi3b
: > "$out/serial.log"
set +e
(ulimit -f 8192; timeout -k 2s 20s nice -n 10 qemu-system-aarch64 -M raspi3b -accel tcg,thread=single -display none -monitor none -serial "file:$out/serial.log" -d int,guest_errors -D "$out/exception.log" -kernel "$out/rei-raspi3b.img" </dev/null > "$out/qemu.log" 2>&1)
s=$?
set -e
cat "$out/serial.log"
cat "$out/qemu.log"
head -n 80 "$out/exception.log"
if ! grep -Fq "REI: CANONICAL KERNEL INIT PASS" "$out/serial.log"; then
  llvm-objdump -d "$out/rei-raspi3b.elf" | grep -A 18 -m 1 "<croissant_init_globals>:" || true
fi
test "$s" = 124 || test "$s" = 0
grep -F 'REI: CANONICAL KERNEL INIT PASS' "$out/serial.log"
grep -F 'REI: MIKUOS ROOT MOUNT PASS' "$out/serial.log"
grep -F 'REI: MIKUOS MOTD READ PASS' "$out/serial.log"
# Guest execution must not masquerade as an interactive OS.
grep -F 'REI: MIKUOS HELLO EXIT PASS' "$out/serial.log"
