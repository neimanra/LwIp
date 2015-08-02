#ifndef __DPDKIF_INIT__
#define __DPDKIF_INIT__
#include "lwip/netif.h"
#include "lwip/pbuf.h"
err_t
dpdkif_init(struct netif *netif);

int
dpdkif_rx_thread_func(void * arg);

struct pbuf *
dpdkif_fetch_pkt(struct netif ** inp);

int dpdkif_get_if_params(ip_addr_t* ipaddr, ip_addr_t* netmask, ip_addr_t* gateway, uint8_t * hw_addr);
#endif
