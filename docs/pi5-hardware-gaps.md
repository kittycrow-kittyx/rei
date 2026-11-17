# Pi 5 hardware ports still required

The pinned Circle 51.1.1 tree does not contain an onboard Bluetooth
transport or CSI capture driver. The USB Bluetooth device in Circle
does not cover the onboard controller. DSI display transmission also
does not implement CSI capture.

## Reference source

Use the Raspberry Pi downstream Linux revision
[aa4df0e](https://github.com/raspberrypi/linux/tree/aa4df0e78a0ddf0271de46ea915b54700f145e21)
for register and platform reference. These sources are not linked or
vendored as Linux modules.

| Hardware | Reference |
|---|---|
| BCM2712 UARTA | [8250_bcm7271.c](https://github.com/raspberrypi/linux/blob/aa4df0e78a0ddf0271de46ea915b54700f145e21/drivers/tty/serial/8250/8250_bcm7271.c) |
| Broadcom HCI | [hci_bcm.c](https://github.com/raspberrypi/linux/blob/aa4df0e78a0ddf0271de46ea915b54700f145e21/drivers/bluetooth/hci_bcm.c) |
| RP1 CFE and CSI-2 | [rp1-cfe](https://github.com/raspberrypi/linux/tree/aa4df0e78a0ddf0271de46ea915b54700f145e21/drivers/media/platform/raspberrypi/rp1-cfe) |
| Board description | [bcm2712-rpi-5-b.dts](https://github.com/raspberrypi/linux/blob/aa4df0e78a0ddf0271de46ea915b54700f145e21/arch/arm64/boot/dts/broadcom/bcm2712-rpi-5-b.dts) |

For Bluetooth, initialise the correct UART, clock, power/reset and flow
control, load the controller firmware and verify a real HCI command
response. Do not use the console PL011 as a substitute. The port should
expose HCI packets rather than implement pairing or profiles.

For cameras, implement the RP1 CFE/CSI-2 receiver, D-PHY, capture DMA
and interrupt handling, then a supported sensor's I2C and power sequence.
Validate a captured frame with known dimensions and format. Camera
tuning, application interfaces and image-processing services belong to
the mikuOS Pi port. Do not claim support from a device-tree node alone.

Neither port is implemented or verified. Preserve source licences when
porting; do not assume GPL-2.0-only Linux code can be combined directly
with GPL-3.0 Circle.
