
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
//#include "lwip/sockets.h"
#include "netif/etharp.h"
#include "lwip/api.h"

#include <rte_launch.h>
#include <rte_cycles.h>
#include "dpdkif.h"

#ifndef IPERF_DEBUG
#define IPERF_DEBUG LWIP_DBG_ON
#endif

extern unsigned long ticks;

static unsigned long send_data[TCP_MSS / sizeof(unsigned long)];

static uint64_t ticks_sec;

#define RUN_NOW 0x00000001

struct client_hdr {
    int32_t flags;
    int32_t numThreads;
    int32_t mPort;
    int32_t bufferlen;
    int32_t mWinBand;
    int32_t mAmount;
};

struct iperf_state {
    struct tcp_pcb *server_pcb;
    struct tcp_pcb *client_pcb;
    unsigned long recv_bytes;
    unsigned long sent_bytes;
    unsigned long recv_start_ticks;
    unsigned long recv_end_ticks;
    unsigned long send_start_ticks;
    unsigned long send_end_ticks;
    int32_t flags;
    int32_t amount;
    int valid_hdr;
};

static err_t recv(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err);

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

static const char * calc_prefix(unsigned long *val, int radix)
{
    if (*val > radix*radix*radix) {
        *val /= radix;
        *val *= 10;
        *val /= radix;
        *val /= radix;
        return "G";
    } else if (*val > radix*radix) {
        *val /= radix;
        *val *= 10;
        *val /= radix;
        return "M";
    }  else if (*val > radix) {
        *val *= 10;
        *val /= radix;
        return "K";
    }

    return "";
}

static void print_result(const char *id, unsigned long start_ticks,
                         unsigned long end_ticks, unsigned long bytes)
{
    unsigned long duration = end_ticks - start_ticks;
    duration *= 10;
    duration = (duration + ticks_sec/2) / ticks_sec;

    unsigned long speed = bytes / duration * 8 * 10;

    const char *size_prefix = calc_prefix(&bytes, 1024);
    const char *speed_prefix = calc_prefix(&speed, 1000);

    LWIP_DEBUGF(IPERF_DEBUG | LWIP_DBG_STATE,
                ("iperf: 0.0- %ld.%ld sec  %ld.%ld %sBytes  %ld.%ld %sbits/sec\n",
                    duration / 10, duration % 10, bytes / 10, bytes % 10,
                    size_prefix, speed / 10, speed % 10, speed_prefix));
}


static err_t sent(void *arg, struct tcp_pcb *tpcb, u16_t len)
{
    int space;
    while ((space = tcp_sndbuf(tpcb)) >= sizeof(send_data)) {
        tcp_write(tpcb, send_data, sizeof(send_data), TCP_WRITE_FLAG_COPY);
    }

    return ERR_OK;
}

static err_t connected(void *arg, struct tcp_pcb *tpcb, err_t err)
{
    print_connection_msg("tx", tpcb);
    tcp_sent(tpcb, sent);
    tcp_recv(tpcb, recv);
    sent(NULL, tpcb, 0);

    return ERR_OK;
}

static void connect_error(void *arg, err_t err)
{
    //tcp_close(tpcb);
    printf("Error connecting to server!\n");
}


static err_t recv(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err)
{
    if (NULL != p) 
    {
        tcp_recved(tpcb, p->tot_len);
        pbuf_free(p);
    }

    return ERR_OK;
}


int main()
{
    iperf_server_init();

    while(1)
    {
    	sleep(1);
    }

    return 0;
}
struct netif netif;
err_t iperf_server_init(void)
{
    int i;
    err_t ret = ERR_OK;
    sys_init();
    lwip_init();
    ticks_sec = rte_get_timer_hz();
    ///////////////////////////////////////////
    ip_addr_t ipaddr, netmask, gateway;

    dpdkif_get_if_params(&ipaddr, &netmask, &gateway, netif.hwaddr);

    netif_set_default(netif_add(&netif, &ipaddr, &netmask, &gateway, NULL, dpdkif_init, ethernet_input));

    netif_set_up(&netif);

    rte_eal_remote_launch (dpdkif_rx_thread_func, NULL,6);
   ////////////////////////////////////////////

    for (i = 0; i < sizeof(send_data) / sizeof(*send_data); i++)
        send_data[i] = i;

    struct tcp_pcb *pcb;
    pcb = tcp_new();

    if (pcb == NULL)
        return ERR_MEM;

    tcp_err(pcb, connect_error);
    ip_addr_t server_addr;
    IP4_ADDR(&server_addr,10,120,197,69);
    //sys_restart_timeouts();
    tcp_connect(pcb, &server_addr, IPERF_SERVER_PORT,connected);

    return ret;
}

