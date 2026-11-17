# Native ARM64 boot

This experimental target compiles all committed Croissant-generated modules
and links Rei's ARM64 runtime. It embeds a VFS image supplied by mikuOS,
initialises the canonical Thistle kernel and attempts to execute its
existing hello programme through the generated RV64 instruction engine.

    bash scripts/build-arm64.sh raspi5 /path/to/root.vfs
    bash scripts/qemu-native-root.sh /path/to/root.vfs

The Pi 5 output is build/native/raspi5/rei-raspi5.img, linked at 0x80000.
The console uses the BCM2712 dedicated debug PL011 at 0x107d001000,
115200 8N1. It is not the 40-pin RP1 UART. Physical testing is required.

The runtime is a single-hart prototype: the pinned generated buffer ABI
lacks pointer identity, so it accepts only the distinct 8-byte floating
scratch buffer and 64 MiB kernel arena, rejecting repeated allocations.
The arena is fixed at physical 64 MiB, ending at 128 MiB, and requires
that range to be free. The imul adapter returns the unsigned pattern
required by the sole generated call, whose C cast otherwise loses a
negative result. The compiler needs general fixes for these semantics.
The host bridge supports stdout and stderr writes only. Guest input,
entropy, networking, persistence and a shell are not implemented.
