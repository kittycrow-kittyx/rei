#include <circle/macb.h>
#include <circle/timer.h>
#include <circle/sched/scheduler.h>
#include <circle/koptions.h>
#include <wlan/bcm4343.h>
#include <fatfs/ff.h>
#include "console.h"
#include "network.h"

namespace {
static_assert(REI_NET_FRAME_CAPACITY == FRAME_BUFFER_SIZE, "frame capacity");
CNetDevice *devices[2];
bool link_up[2];
unsigned last_phy;
bool checked_phy;
bool rx_seen;
unsigned last_reply;
bool replied;

bool equal_bytes(const uint8_t *a, const uint8_t *b, unsigned n) {
    for (unsigned i = 0; i < n; ++i) if (a[i] != b[i]) return false;
    return true;
}

void print_mac(unsigned index) {
    uint8_t mac[6];
    if (rei_net_mac(index, mac)) return;
    const char digits[] = "0123456789abcdef";
    for (unsigned i = 0; i < sizeof mac; ++i) {
        if (i) rei_console_char(':');
        rei_console_char(digits[mac[i] >> 4]);
        rei_console_char(digits[mac[i] & 15]);
    }
    rei_console_char('\n');
}

void refresh_link(unsigned index) {
    if (!devices[index]) return;
    // Reclaim completed TX descriptors before polling PHY changes.
    devices[index]->IsSendFrameAdvisable();
    const bool up = devices[index]->IsLinkUp();
    if (up == link_up[index] && checked_phy) return;
    link_up[index] = up;
    rei_console_puts(index == REI_NET_ETHERNET ? "REI: ETH LINK " : "REI: WLAN LINK ");
    rei_console_puts(up ? "UP\n" : "DOWN\n");
    if (up && index == REI_NET_ETHERNET) {
        rei_console_puts("REI: ETH SPEED ");
        rei_console_puts(CNetDevice::GetSpeedString(devices[index]->GetLinkSpeed()));
        rei_console_char('\n');
    }
}

// Diagnostic only: echo an explicitly addressed local experimental frame.
// No broadcast replies, IP addresses, DHCP or unsolicited transmission.
void probe_frame(uint8_t *frame, unsigned length, unsigned now) {
    const uint8_t magic[] = {'R','E','I','-','L','2','-','Q'};
    uint8_t mac[6];
    if (length != 60 || frame[12] != 0x88 || frame[13] != 0xb5 ||
        rei_net_mac(REI_NET_ETHERNET, mac) || !equal_bytes(frame, mac, 6) ||
        (frame[6] & 1) || equal_bytes(frame + 6, mac, 6) ||
        !equal_bytes(frame + 14, magic, sizeof magic)) return;
    if (replied && now - last_reply < HZ) return;
    for (unsigned i = 0; i < 6; ++i) {
        frame[i] = frame[i + 6];
        frame[i + 6] = mac[i];
    }
    frame[21] = 'A';
    if (rei_net_send(REI_NET_ETHERNET, frame, length) == 0) {
        replied = true;
        last_reply = now;
        rei_console_puts("REI: ETH PROBE REPLY QUEUED\n");
    }
}
CScheduler *scheduler;
bool probe_enabled;
}

extern "C" void rei_net_ethernet_init(void) {
    rei_console_puts("REI: ETH INIT START\n");
    probe_enabled = CKernelOptions::Get()->GetAppOptionDecimal("rei.netprobe", 0) == 1;
    auto *ethernet = new CMACBDevice;
    if (!ethernet || !ethernet->Initialize()) {
        // Keep the partially initialised driver alive until halt.
        rei_console_puts("REI: ETH UNAVAILABLE\n");
        return;
    }
    devices[REI_NET_ETHERNET] = ethernet;
    rei_console_puts("REI: ETH MAC ");
    print_mac(REI_NET_ETHERNET);
}

extern "C" void rei_net_wifi_init(void) {
    const char *files[] = {
        "SD:/firmware/brcmfmac43455-sdio.raspberrypi,5-model-b.bin",
        "SD:/firmware/brcmfmac43455-sdio.raspberrypi,5-model-b.txt",
        "SD:/firmware/brcmfmac43455-sdio.raspberrypi,5-model-b.clm_blob"
    };
    for (const char *path : files) {
        FIL file;
        if (f_open(&file, path, FA_READ | FA_OPEN_EXISTING) != FR_OK) {
            rei_console_puts("REI: WLAN FIRMWARE MISSING ");
            rei_console_puts(path);
            rei_console_char('\n');
            return;
        }
        const bool empty = f_size(&file) == 0;
        if (f_close(&file) != FR_OK || empty) {
            rei_console_puts("REI: WLAN FIRMWARE INVALID\n");
            return;
        }
    }
    rei_console_puts("REI: WLAN INIT START\n");
    scheduler = new CScheduler;
    auto *wifi = new CBcm4343Device("SD:/firmware/");
    if (!wifi || !wifi->Initialize()) {
        rei_console_puts("REI: WLAN UNAVAILABLE\n");
        return;
    }
    devices[REI_NET_WIFI] = wifi;
    rei_console_puts("REI: WLAN MAC ");
    print_mac(REI_NET_WIFI);
    rei_console_puts("REI: WLAN INITIALISED (NOT ASSOCIATED)\n");
}

extern "C" int rei_net_mac(unsigned device, uint8_t *mac) {
    if (device >= 2 || !devices[device] || !mac) return -1;
    devices[device]->GetMACAddress()->CopyTo(mac);
    return 0;
}
extern "C" int rei_net_link(unsigned device) {
    if (device >= 2 || !devices[device]) return -1;
    return link_up[device] ? 1 : 0;
}
extern "C" int rei_net_receive(unsigned device, void *frame, unsigned capacity) {
    if (device >= 2 || !devices[device] || !frame || capacity < REI_NET_FRAME_CAPACITY) return -1;
    unsigned length = 0;
    if (!devices[device]->ReceiveFrame(frame, &length)) return 0;
    if (length < 14 || length > REI_NET_FRAME_CAPACITY) return -1;
    return static_cast<int>(length);
}
extern "C" int rei_net_send(unsigned device, const void *frame, unsigned length) {
    if (device >= 2 || !devices[device] || !link_up[device] || !frame ||
        length < 14 || length > REI_NET_FRAME_CAPACITY ||
        !devices[device]->IsSendFrameAdvisable()) return -1;
    return devices[device]->SendFrame(frame, length) ? 0 : -1;
}
extern "C" void rei_net_poll(void) {
    if (scheduler) scheduler->Yield();
    const unsigned now = CTimer::Get()->GetTicks();
    if (!checked_phy || now - last_phy >= 2 * HZ) {
        refresh_link(REI_NET_ETHERNET);
        refresh_link(REI_NET_WIFI);
        checked_phy = true;
        last_phy = now;
    }
    uint8_t frame[REI_NET_FRAME_CAPACITY];
    for (unsigned i = 0; i < 8; ++i) {
        const int length = rei_net_receive(REI_NET_ETHERNET, frame, sizeof frame);
        if (length <= 0) break;
        if (!rx_seen) {
            rei_console_puts("REI: ETH FIRST FRAME BYTES ");
            rei_console_number(length);
            rei_console_char('\n');
            rx_seen = true;
        }
        if (probe_enabled) probe_frame(frame, static_cast<unsigned>(length), now);
    }
}
