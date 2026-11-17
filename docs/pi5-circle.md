# Raspberry Pi hardware build

This hardware branch is intended for the Raspberry Pi family, not
one board generation. The first Circle-backed target is the Pi 5.
Other boards will need their own build and hardware acceptance tests.

Circle 51.1.1 is vendored in vendor/circle. It supplies startup,
screen, interrupt, timer, DMA, USB and onboard SD-card libraries. Thistle remains the
kernel. The direct UART/QEMU build remains independent of Circle.

## Local build

Install the AArch64 GNU bare-metal toolchain and Clang. The default
prefix is aarch64-none-elf-. PREFIX64 can select another compatible
toolchain.

    git submodule update --init --jobs 2 -- vendor/circle
    nice -n 10 bash scripts/build-raspi5-circle.sh /absolute/path/root.vfs

The script builds Circle libraries in build/circle without modifying
the vendor checkout. REI_JOBS defaults to 2. The resulting
build/native/raspi5-circle/rei-raspi5-circle.img embeds the mikuOS root.
The 16 MiB kernel reservation includes the embedded VFS. The native
allocation pool uses Circle's allocator, not a fixed physical address.
The keyboard map defaults to UK.

## Physical acceptance

Use Pi 5 firmware and the board device tree on the SD card. Start from
Circle's boot/config64.txt and set the Pi 5 kernel to the built image.
Avoid a Linux initramfs and Linux graphics overlays for this test.
Keep the NVMe installation unchanged.

Check the real mikuOS MOTD and RV64GC hello output on HDMI, then attach
a USB keyboard and verify the echo test. This is not a shell.
After the guest hello exits, the adapter probes the onboard SD card,
reports its byte capacity and reads sector zero twice. Expect
`REI: SD SECTOR 0 READ PASS` and, on the prepared partitioned card,
`REI: SD BOOT SIGNATURE 55AA`. The probe is read-only. It does not
mount a filesystem, write to the card or access NVMe. A reported SD
probe failure does not prevent the keyboard echo test.
The pinned Thistle source supports host writes but not host reads;
guest stdin requires a canonical Thistle change.

Circle is GPL-3.0 licensed. Review applicable licence obligations before
distributing linked images. This build and physical hardware path are
not yet tested.
