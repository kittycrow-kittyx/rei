# Native ARM64 boot

Rei compiles the canonical Thistle source through Bake and Croissant.
It owns ARM startup, linking and platform interfaces, not kernel semantics.

Initialise the pinned dependencies before building:

    git submodule update --init --recursive
    bash scripts/build-arm64.sh raspi5 /path/to/root.vfs
    bash scripts/qemu-native-root.sh /path/to/root.vfs

Each build generates fresh C in build/native-generated. The Pi 5 image is
build/native/raspi5/rei-raspi5.img, linked at 0x80000. Its console uses the
BCM2712 dedicated debug PL011 at 0x107d001000 (115200 8N1); physical testing
is required. The exception vectors report EL, ESR, ELR and FAR.

Rei initialises the kernel, mounts the supplied mikuOS VFS, reads its MOTD,
and attempts to run the committed RV64GC hello fixture at
/usr/bin/hello-rv64.39. The legacy /usr/bin/hello.txe is not an RV64GC
image and remains unchanged. The fixture writes to the console and exits;
it is not an interactive shell.

Kernel memory uses the native opaque buffer handle, with a bounded
80 MiB allocation pool at physical 64 MiB. The first kernel allocation is
64 MiB; the rest is available to other allocations. The prototype assumes
this physical range is free and parks the secondary cores. The compiled
kernel uses Thistle's per-hart floating-point scratch instead of a global
buffer. Early ARM64 accesses use strict alignment.

Only stdout/stderr host writes are implemented. HDMI, keyboard input,
networking, entropy, persistence and interactive userspace are unfinished.
Native C numeric semantics still require runtime parity tests.

## Raspberry Pi hardware dependency

Circle 51.1.1 is pinned at `7a81e1b480fe7d19fc1fc695b89bff0a0848f44b` in
`vendor/circle`. It is present only on this hardware feature branch.
Circle provides bare-metal screen, USB HID and supporting hardware services.
It is GPL-3.0 licensed; distributing a linked image requires compliance
with the applicable GPL terms. The direct UART build remains independent
of Circle.
