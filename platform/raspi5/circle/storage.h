#ifndef REI_PI_CIRCLE_STORAGE_H
#define REI_PI_CIRCLE_STORAGE_H

#include <stdint.h>

#define REI_SD_SECTOR_SIZE 512u

#ifdef __cplusplus
extern "C" {
#endif
/* Onboard SD only; call from the platform main loop, not an IRQ. */
uint64_t rei_sd_capacity(void);
/* Buffer must hold 512 bytes. Returns zero on success, -1 on failure. */
int rei_sd_read_sector(uint64_t sector, void *buffer);
#ifdef REI_ROOT_SD
/* Load root.vfs from the onboard SD FAT volume; no write interface. */
int rei_sd_root_open(uint32_t *size);
int rei_sd_root_read(void *buffer, uint32_t length);
int rei_sd_root_close(void);
#endif
#ifdef __cplusplus
}
#endif
#endif
