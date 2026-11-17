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
distributing linked images. The embedded-root build, HDMI output, USB keyboard echo and SD sector
reads were demonstrated on a physical Pi 5. The SD-root and network
paths require separate hardware tests.

## Root on the SD card

The optional SD-root build uses Circle's FatFs add-on in read-only mode:

    REI_JOBS=2 nice -n 10 bash scripts/build-raspi5-circle.sh /absolute/path/root.vfs sd

The output is build/native/raspi5-circle-sd/rei-raspi5-circle-sd.img.
Copy root.vfs to the root of the onboard SD card's FAT boot volume and
select this kernel image in config.txt. This variant does not embed the
root archive. It opens SD:/root.vfs, reads it in bounded chunks and
passes the image to canonical Thistle for VFS loading. It stops if the
file is missing, exceeds 16 MiB, cannot be read or fails kernel VFS
validation. It never falls back to an embedded root.

Expected new output is REI: SD ROOT READ PASS followed by the canonical
root mount and hello checks. The FAT reader cannot write to media. This
is not a persistent writable guest filesystem, and the SD-root path
requires physical testing. The original embedded-root build is retained.

## Network driver acceptance

The idle diagnostic initialises Circle's onboard Pi 5 MACB Ethernet
controller, prints the MAC, reports link changes and polls for frames.
It exposes bounded frame I/O through network.h. For the optional echo test, append rei.netprobe=1 to the SD boot
cmdline.txt (one line). On an isolated test segment, run from a Linux
peer using its wired interface and the MAC
shown by Rei:

    sudo python3 scripts/test-ethernet.py INTERFACE REI_MAC

The peer sends a unicast experimental EtherType 0x88b5 frame containing
a random nonce. Rei responds only to an addressed diagnostic request;
the peer verifies the echoed nonce. A queued reply alone is not proof
of TX. No IP stack, DHCP or guest sockets are provided by this test.

Both embedded and SD-root images link Ethernet, Wi-Fi and their
support libraries by default. There is no separate Wi-Fi kernel.
Copy the three Pi 5 firmware files produced by Circle's pinned
addon/wlan/firmware Makefile (target firmware5) into /firmware on the
SD FAT boot volume. After the guest exits, the adapter mounts the FAT
volume read-only and checks the firmware files before initialising
the BCM4343x driver. Missing firmware skips Wi-Fi initialisation.
Append rei.wifi=0 to cmdline.txt to skip Wi-Fi, or rei.net=0 to skip
both controllers. These options disable initialisation, not linking.
No firmware downloads occur during kernel compilation. Circle scheduler tasks
serve the driver only; Thistle still owns guest scheduling. No access
point association, credentials or WPA supplicant is configured yet.
WPA2-Enterprise is not established by the upstream default sample.

## Build distribution

Keep kernel, compiler and driver source revisions pinned. The intended
release process builds driver-linked Pi kernel artefacts on commits so
consumers can use a prebuilt image instead of compiling Circle. During
hardware bring-up all builds are local; no new CI workflow is enabled.

## Standard hardware driver bundle

Both root variants build the same Pi 5 hardware libraries from the
pinned Circle revision. The build emits circle-drivers.o (all objects
from the listed hardware archives) and driver-archives.txt beside the
kernel image, and retains the driver code at final link. This includes
USB device classes, DMA, GPIO, SPI, I2C, hardware random source, timers,
watchdog, audio, SD, NVMe, Ethernet, WLAN, RTC, DSI panels, auxiliary
displays, sensors, GPIO expanders, OneWire, SPI LED strips, SPI digital
I/O and supported external microcontroller interfaces. The older BCM
SMI LED driver is excluded on Pi 5. Circle's service applications,
GUI libraries, Linux compatibility add-on and full network services
are not part of this hardware bundle. USB Bluetooth controller support
is included; onboard Bluetooth and camera support require separate
assessment and are not claimed here.

Link inclusion is not guest device exposure or hardware validation.
Only drivers with platform adapters and acceptance tests can be
reported as operational. Do not initialise every optional peripheral
unconditionally: bus pins, interrupts and DMA resources may conflict.
The future commit-triggered build can publish these precompiled
artefacts, but no workflow is added during local bring-up.

## Native console startup

Normal C Circle builds launch /bin/thsh-native.39 from the mikuOS
root image. This is a freestanding bring-up shell, not the hosted
thsh implementation. Thistle owns the opt-in stdin read and validates
one-byte completion under the hart lock. Rei polls hardware while the
guest is waiting. Circle services start before the guest prompt.
The direct UART build retains the hello regression by default; setting
REI_NATIVE_SHELL=1 selects the native shell for serial testing.

Onboard Bluetooth and CSI capture are not provided by the pinned Circle
revision. The Pi 5 uses BCM2712 UARTA for onboard HCI and RP1 CFE/CSI-2
for cameras. These require separate freestanding hardware ports with
firmware, clock/reset, interrupt and DMA handling; no Linux kernel
modules are linked. USB Bluetooth is a different device class.
