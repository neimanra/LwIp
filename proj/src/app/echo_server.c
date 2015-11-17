
/****************************************************************//**
 *
 * @file iperf_server.c
 *
 * @author   Logan Gunthorpe <logang@deltatee.com>
 *
 * @brief    Iperf server implementation
 *
 * Copyright (c) Deltatee Enterprises Ltd. 2013
 * All rights reserved.
 *
 ********************************************************************/

/* 
 * Redistribution and use in source and binary forms, with or without
 * modification,are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO
 * EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
 * TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * Author: Logan Gunthorpe <logang@deltatee.com>
 *
 */
 
#include "iperf_server.h"

#include <lwip/tcp.h>
#include <lwip/debug.h>
#include <stdint.h>
#include <unistd.h>
#include "lwip/init.h"
#include "lwip/sys.h"
#include "lwip/mem.h"
#include "lwip/memp.h"
#include "lwip/pbuf.h"
#include "lwip/tcp.h"
#include "lwip/tcpip.h"
#include "lwip/netif.h"
#include "lwip/stats.h"
#include "netif/etharp.h"
#include "lwip/api.h"

#include <rte_launch.h>
#include <rte_cycles.h>
#include "dpdkif.h"

#ifndef IPERF_DEBUG
#define IPERF_DEBUG LWIP_DBG_ON
#endif

static err_t disconnect(struct tcp_pcb *tpcb);

static void print_connection_msg(const char *id, struct tcp_pcb *tpcb)
{
    LWIP_DEBUGF(IPERF_DEBUG | LWIP_DBG_STATE,
                ("iperf: [%s] local ", id));
    ip_addr_debug_print(IPERF_DEBUG | LWIP_DBG_STATE, &tpcb->local_ip);
    LWIP_DEBUGF(IPERF_DEBUG | LWIP_DBG_STATE,
                (" port %d - rem ", tpcb->local_port));
    ip_addr_debug_print(IPERF_DEBUG | LWIP_DBG_STATE, &tpcb->remote_ip);
    LWIP_DEBUGF(IPERF_DEBUG | LWIP_DBG_STATE,
                (" port %d\n", tpcb->remote_port));
}


static err_t disconnect(struct tcp_pcb *tpcb)
{
    err_t ret = ERR_ARG;

    if (tpcb != NULL)
    {
    	ret = tcp_close(tpcb);
    }
    return ret;
}


static err_t recv(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err)
{
    if (p == NULL)
    {
        return disconnect(tpcb);
    }

    tcp_recved(tpcb, p->tot_len);
    tcp_write(tpcb, p->payload, p->tot_len, TCP_WRITE_FLAG_COPY );
    pbuf_free(p);
    return ERR_OK;
}

static err_t accept(void *arg, struct tcp_pcb *newpcb, err_t err)
{
    print_connection_msg("rx", newpcb);

    tcp_recv(newpcb, recv);
    return ERR_OK;
}

int main()
{
    echo_server_init();

    while(1)
    {
    	sleep(1);
    }

    return 0;
}

struct netif netif;
err_t echo_server_init(void)
{
    err_t ret = ERR_OK;
    sys_init();
    lwip_init();

    ///////////////////////////////////////////
    ip_addr_t ipaddr, netmask, gateway;

    dpdkif_get_if_params(&ipaddr, &netmask, &gateway, netif.hwaddr);

    netif_set_default(netif_add(&netif, &ipaddr, &netmask, &gateway, NULL, dpdkif_init, ethernet_input));

    netif_set_up(&netif);

    rte_eal_mp_remote_launch((lcore_function_t *)dpdkif_rx_thread_func, NULL, SKIP_MASTER);
   ////////////////////////////////////////////

    struct tcp_pcb *pcb;
    pcb = tcp_new();

    if (pcb == NULL)
        return ERR_MEM;

    if ((ret = tcp_bind(pcb, IP_ADDR_ANY, IPERF_SERVER_PORT)) != ERR_OK) {
        tcp_close(pcb);
        return ret;
    }

    pcb = tcp_listen(pcb);
    tcp_accept(pcb, accept);

    return ret;
}

