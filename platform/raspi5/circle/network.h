#ifndef REI_PI_CIRCLE_NETWORK_H
#define REI_PI_CIRCLE_NETWORK_H

#include <stdint.h>

#define REI_NET_ETHERNET 0u
#define REI_NET_WIFI 1u
#define REI_NET_FRAME_CAPACITY 1600u

#ifdef __cplusplus
extern "C" {
#endif
/* Platform main loop only; not an IRQ or guest socket interface. */
void rei_net_ethernet_init(void);
void rei_net_wifi_init(void);
void rei_net_poll(void);
int rei_net_mac(unsigned device, uint8_t *mac); /* six bytes */
int rei_net_link(unsigned device);              /* -1 absent, 0 down, 1 up */
int rei_net_receive(unsigned device, void *frame, unsigned capacity);
int rei_net_send(unsigned device, const void *frame, unsigned length);
/* receive: bytes, 0 empty, -1 invalid; send: 0 queued, -1 unavailable. */
#ifdef __cplusplus
}
#endif
#endif
