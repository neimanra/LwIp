#ifndef __DPDKIF_INIT__
#define __DPDKIF_INIT__

err_t
dpdkif_init(struct netif *netif);

int
dpdkif_rx_thread_func(void * arg);
#endif
