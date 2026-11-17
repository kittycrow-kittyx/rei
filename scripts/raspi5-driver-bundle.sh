#!/usr/bin/env bash
# Shared Pi 5 driver cache and precompiled bundle functions.
driver_archives=(
  lib/libcircle.a
  lib/usb/libusb.a
  lib/input/libinput.a
  lib/sound/libsound.a
  addon/SDCard/libsdcard.a
  addon/nvme/libnvme.a
  addon/wlan/libwlan.a
  addon/rp1dsi/librp1dsi.a
  addon/rtc/librtc.a
  addon/display/libdisplay.a
  addon/gpio/libgpio.a
  addon/sensor/libsensor.a
  addon/OneWire/libonewire.a
  addon/WS28XX/libws28xx.a
  addon/dio/libdio.a
  addon/pico/libpico.a
  addon/microbit/libmicrobit.a
)
support_archives=(
  addon/fatfs/libfatfs.a
  lib/fs/libfs.a
  lib/net/libnet.a
  lib/sched/libsched.a
)
bundle_files=(circle-drivers.o driver-archives.txt provenance.txt LICENSE "${support_archives[@]}")

circle_prepare() {
  pinned="$(git ls-tree HEAD vendor/circle | awk '$1 == "160000" {print $3}')"
  test "$pinned" = "7a81e1b480fe7d19fc1fc695b89bff0a0848f44b"
  git submodule sync -- vendor/circle
  git submodule update --init --jobs "$jobs" -- vendor/circle
  test "$(git -C vendor/circle rev-parse HEAD)" = "$pinned"
  git -C vendor/circle diff --quiet
  git -C vendor/circle diff --cached --quiet
  key="$(printf '%s\n' "$pinned" "$prefix" "$("${prefix}gcc" -dumpfullversion)" 'pi5-16m-v4' | sha256sum | cut -c1-16)"
  circle="$repo/build/circle/$key"
  if [[ ! -f "$circle/Rules.mk" ]]; then
    mkdir -p "$circle"
    git -C vendor/circle archive "$pinned" | tar -xf - -C "$circle"
  fi
  local expected_config
  expected_config="$(cat <<EOF
AARCH = 64
RASPPI = 5
PREFIX64 = $prefix
STDLIB_SUPPORT = 1
DEFINE += -DKERNEL_MAX_SIZE=16777216 -DDEFAULT_KEYMAP=\"UK\"
EOF
)"
  if [[ -f "$circle/Config.mk" ]]; then
    [[ "$(cat "$circle/Config.mk")" == "$expected_config" ]] || {
      echo "Circle cache configuration differs; use a clean cache" >&2
      return 1
    }
  else
    printf '%s\n' "$expected_config" > "$circle/Config.mk"
  fi
  if [[ -e "$circle/Config2.mk" ]]; then
    echo "Unexpected Circle Config2.mk override" >&2
    return 1
  fi
  sed -i 's/^#define FF_FS_READONLY[[:space:]]*0$/#define FF_FS_READONLY 1/' "$circle/addon/fatfs/ffconf.h"
  grep -Eq '^#define FF_FS_READONLY[[:space:]]+1$' "$circle/addon/fatfs/ffconf.h"
}

circle_manifest() {
  printf 'circle %s\nconfiguration pi5-16m-v4\narchitecture aarch64\nboard raspi5\n' "$pinned"
  printf 'gcc %s\ntarget %s\n' "$("${prefix}gcc" -dumpfullversion)" "$("${prefix}gcc" -dumpmachine)"
  printf 'config %s\nfatfs %s\n' \
    "$(sed 's/^PREFIX64 = .*/PREFIX64 = <toolchain>/' "$circle/Config.mk" | sha256sum | cut -d' ' -f1)" \
    "$(sha256sum "$circle/addon/fatfs/ffconf.h" | cut -d' ' -f1)"
}

circle_compile() {
  for archive in "${driver_archives[@]}"; do
    dir="${archive%/*}"
    args=()
    if [[ "$dir" == addon/WS28XX ]]; then
      # Pi 5 has the RP1 SPI controller, not the older BCM SMI implementation.
      args+=(OBJS=ws28xxstripe.o)
    fi
    nice -n 10 make -C "$circle/$dir" -j "$jobs" "${archive##*/}" "${args[@]}"
  done
  for library in lib/fs addon/fatfs lib/sched lib/net; do
    nice -n 10 make -C "$circle/$library" -j "$jobs"
  done
}

circle_export() {
  local dest="$1" archive
  local -a archive_paths=()
  for archive in "${driver_archives[@]}" "${support_archives[@]}"; do
    if [[ ! -s "$circle/$archive" ]]; then
      echo "Missing compiled archive: $circle/$archive" >&2
      echo "Build with REI_BUILD_DRIVERS=1 before exporting." >&2
      return 1
    fi
  done
  mkdir -p "$dest"
  rm -f "$dest/SHA256SUMS"
  for archive in "${driver_archives[@]}"; do archive_paths+=("$circle/$archive"); done
  # Relocatable link only: no source compilation.
  nice -n 10 "${prefix}ld" -r --whole-archive "${archive_paths[@]}" --no-whole-archive -o "$dest/circle-drivers.o"
  for archive in "${support_archives[@]}"; do
    mkdir -p "$dest/$(dirname "$archive")"
    cp "$circle/$archive" "$dest/$archive"
  done
  cp "$circle/LICENSE" "$dest/LICENSE"
  printf '%s\n' "${driver_archives[@]}" > "$dest/driver-archives.txt"
  circle_manifest > "$dest/provenance.txt"
  (cd "$dest" && sha256sum "${bundle_files[@]}" > SHA256SUMS)
}

circle_verify() {
  local dest="$1" file
  for file in "${bundle_files[@]}" SHA256SUMS; do
    [[ -s "$dest/$file" ]] || { echo "Missing precompiled file: $dest/$file" >&2; return 1; }
  done
  cmp -s <(circle_manifest) "$dest/provenance.txt" || {
    echo "Precompiled Circle source, configuration or toolchain mismatch" >&2
    return 1
  }
  cmp -s <(printf '%s\n' "${driver_archives[@]}") "$dest/driver-archives.txt" || {
    echo "Precompiled Circle driver inventory mismatch" >&2
    return 1
  }
  cmp -s "$circle/LICENSE" "$dest/LICENSE" || return 1
  (cd "$dest" && sha256sum "${bundle_files[@]}" | cmp - SHA256SUMS) || {
    echo "Precompiled Circle checksum mismatch" >&2
    return 1
  }
}
