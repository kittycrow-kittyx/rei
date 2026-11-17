// Raspberry Pi hardware adapter using Circle's public driver interfaces.
// Thistle, not Circle, owns all guest kernel behaviour.
#include <circle/actled.h>
#include <circle/alloc.h>
#include <circle/devicenameservice.h>
#include <circle/exceptionhandler.h>
#include <circle/interrupt.h>
#include <circle/koptions.h>
#include <circle/logger.h>
#include <circle/screen.h>
#include <circle/serial.h>
#include <circle/startup.h>
#include <circle/timer.h>
#include <circle/usb/usbhcidevice.h>
#include <circle/usb/usbkeyboard.h>
#include <SDCard/emmc.h>
#include "console.h"
#include "storage.h"
#ifdef REI_ROOT_SD
#include <fatfs/ff.h>
static_assert(FF_FS_READONLY == 1, "SD root must be read-only");
#endif

extern "C" void rei_native_start(void);

namespace {
constexpr unsigned QueueSize = 256;
unsigned char queue[QueueSize];
unsigned head, tail;
CUSBKeyboardDevice *keyboard;
CScreenDevice *screen;
CSerialDevice *serial;
CUSBHCIDevice *usb;
CEMMCDevice *sd;
bool sd_ready;
bool sd_attempted;

bool initialise_sd() {
    if (!sd_attempted) {
        sd_attempted = true;
        sd_ready = sd && sd->Initialize();
    }
    return sd_ready;
}

void key_pressed(const char *text) {
    while (*text) {
        const unsigned write = __atomic_load_n(&head, __ATOMIC_RELAXED);
        const unsigned next = (write + 1) % QueueSize;
        if (next == __atomic_load_n(&tail, __ATOMIC_ACQUIRE)) return;
        queue[write] = static_cast<unsigned char>(*text++);
        __atomic_store_n(&head, next, __ATOMIC_RELEASE);
    }
}

void keyboard_removed(CDevice *, void *) { keyboard = nullptr; }

class Hardware {
    // Circle requires construction in dependency order.
    CActLED led;
    CKernelOptions options;
    CDeviceNameService names;
    CScreenDevice display;
    CSerialDevice uart;
    CExceptionHandler exceptions;
    CInterruptSystem interrupts;
    CTimer timer;
    CLogger logger;
    CUSBHCIDevice controller;
    CEMMCDevice card;
public:
    Hardware() : display(options.GetWidth(), options.GetHeight()),
        timer(&interrupts), logger(options.GetLogLevel(), &timer),
        controller(&interrupts, &timer, TRUE),
        card(&interrupts, &timer, &led, CEMMCDevice::SDCardOnBoard) {}
    bool initialise() {
        if (display.Initialize()) screen = &display;
        if (uart.Initialize(115200)) serial = &uart;
        CDevice *target = serial ? static_cast<CDevice *>(serial)
                                 : static_cast<CDevice *>(screen);
        if (!target || !logger.Initialize(target)) return false;
        if (!interrupts.Initialize() || !timer.Initialize()) return false;
        rei_console_puts(screen ? "REI: HDMI SCREEN READY\n" :
                                  "REI: HDMI UNAVAILABLE\n");
        if (controller.Initialize()) {
            usb = &controller;
            rei_console_puts("REI: USB HOST READY\n");
        } else {
            rei_console_puts("REI: USB HOST UNAVAILABLE\n");
        }
        sd = &card;
        return true;
    }
};
}

extern "C" void *rei_platform_allocate(size_t size) { return malloc(size); }
extern "C" void rei_console_init(void) {}

extern "C" void rei_console_char(char c) {
    if (screen) screen->Write(&c, 1);
    if (serial) serial->Write(&c, 1);
}
extern "C" void rei_console_puts(const char *s) {
    while (*s) rei_console_char(*s++);
}
extern "C" void rei_console_number(unsigned long long value) {
    char digits[21]; unsigned n = 0;
    do { digits[n++] = char('0' + value % 10); value /= 10; } while (value);
    while (n) rei_console_char(digits[--n]);
}
extern "C" void rei_console_poll(void) {
    if (!usb) return;
    usb->UpdatePlugAndPlay();
    if (!keyboard) {
        keyboard = static_cast<CUSBKeyboardDevice *>(
            CDeviceNameService::Get()->GetDevice("ukbd1", FALSE));
        if (keyboard) {
            keyboard->RegisterRemovedHandler(keyboard_removed);
            keyboard->RegisterKeyPressedHandler(key_pressed);
            rei_console_puts("REI: USB KEYBOARD READY\n");
        }
    }
    if (keyboard) keyboard->UpdateLEDs();
}
extern "C" int rei_console_read(void) {
    const unsigned read = __atomic_load_n(&tail, __ATOMIC_RELAXED);
    if (read != __atomic_load_n(&head, __ATOMIC_ACQUIRE)) {
        const unsigned char c = queue[read];
        __atomic_store_n(&tail, (read + 1) % QueueSize, __ATOMIC_RELEASE);
        return c;
    }
    unsigned char c;
    if (serial && serial->Read(&c, 1) == 1) return c;
    return -1;
}
extern "C" uint64_t rei_sd_capacity(void) {
    return sd_ready ? sd->GetSize() : 0;
}

extern "C" int rei_sd_read_sector(uint64_t sector, void *buffer) {
    if (!buffer || !sd_ready || sector > UINT32_MAX ||
        sector >= rei_sd_capacity() / REI_SD_SECTOR_SIZE) return -1;
    alignas(16) unsigned char block[REI_SD_SECTOR_SIZE];
    const uint64_t offset = sector * REI_SD_SECTOR_SIZE;
    if (sd->Seek(offset) != offset ||
        sd->Read(block, sizeof block) != static_cast<int>(sizeof block)) return -1;
    auto *out = static_cast<unsigned char *>(buffer);
    for (unsigned i = 0; i < sizeof block; ++i) out[i] = block[i];
    return 0;
}

static void probe_sd(void) {
    rei_console_puts("REI: SD PROBE START\n");
    if (!initialise_sd()) {
        rei_console_puts("REI: SD UNAVAILABLE\n");
        return;
    }
    rei_console_puts("REI: SD CAPACITY BYTES ");
    rei_console_number(rei_sd_capacity());
    rei_console_char('\n');
    unsigned char first[REI_SD_SECTOR_SIZE], second[REI_SD_SECTOR_SIZE];
    if (rei_sd_read_sector(0, first) || rei_sd_read_sector(0, second)) {
        rei_console_puts("REI: SD SECTOR READ FAILED\n");
        return;
    }
    for (unsigned i = 0; i < sizeof first; ++i) {
        if (first[i] != second[i]) {
            rei_console_puts("REI: SD SECTOR READ MISMATCH\n");
            return;
        }
    }
    rei_console_puts("REI: SD SECTOR 0 READ PASS\n");
    rei_console_puts(first[510] == 0x55 && first[511] == 0xaa
        ? "REI: SD BOOT SIGNATURE 55AA\n" : "REI: SD NO 55AA BOOT SIGNATURE\n");
}

#ifdef REI_ROOT_SD
namespace {
FATFS root_fs;
FIL root_file;
bool root_open;
constexpr char RootDrive[] = "SD:";
constexpr char RootPath[] = "SD:/root.vfs";
}

extern "C" int rei_sd_root_open(uint32_t *size) {
    if (!size || root_open || !initialise_sd()) return -1;
    if (f_mount(&root_fs, RootDrive, 1) != FR_OK) {
        f_unmount(RootDrive);
        return -1;
    }
    if (f_open(&root_file, RootPath, FA_READ | FA_OPEN_EXISTING) != FR_OK) {
        f_unmount(RootDrive);
        return -1;
    }
    const auto length = f_size(&root_file);
    if (length == 0 || length > 16u * 1024u * 1024u) {
        f_close(&root_file);
        f_unmount(RootDrive);
        return -1;
    }
    *size = static_cast<uint32_t>(length);
    root_open = true;
    return 0;
}

extern "C" int rei_sd_root_read(void *buffer, uint32_t length) {
    if (!root_open || !buffer || !length || length > 4096) return -1;
    UINT actual = 0;
    if (f_read(&root_file, buffer, length, &actual) != FR_OK || actual != length)
        return -1;
    return 0;
}

extern "C" int rei_sd_root_close(void) {
    if (!root_open) return -1;
    const auto closed = f_close(&root_file);
    const auto unmounted = f_unmount(RootDrive);
    root_open = false;
    return closed == FR_OK && unmounted == FR_OK ? 0 : -1;
}
#endif

extern "C" void rei_console_idle(void) {
    probe_sd();
    rei_console_puts("REI: Keyboard echo test (not a shell).\n");
    for (;;) {
        rei_console_poll();
        int c = rei_console_read();
        if (c >= 0) rei_console_char(c == '\r' ? '\n' : static_cast<char>(c));
    }
}
extern "C" void rei_panic(const char *s) {
    rei_console_puts(s);
    halt();
    for (;;) __asm__ volatile("wfe");
}

int main(void) {
    Hardware hardware;
    if (!hardware.initialise()) {
        rei_console_puts("REI: Circle platform initialisation failed\n");
        halt();
        return EXIT_HALT;
    }
    rei_native_start();
    halt();
    return EXIT_HALT;
}
