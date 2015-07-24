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
#endif
