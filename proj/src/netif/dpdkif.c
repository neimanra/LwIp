
#include <string.h>
#include <stdlib.h>
#include <rte_config.h>
#include <rte_ethdev.h>
#include <rte_mempool.h>
#include <rte_mbuf.h>

#include "include/arch/cc.h"

#include "lwip/debug.h"
#include "lwip/opt.h"
#include "lwip/def.h"
#include "lwip/ip.h"
#include "lwip/mem.h"
#include "lwip/pbuf.h"
#include "lwip/sys.h"
#include "lwip/arch.h"
#include "netif/etharp.h"


/* Compare 2 16bit aligned MAC addresses */
#define CMP_MAC_ADDR(MAC1, MAC2) \ 
(((*MAC1) == (*MAC2)) && (*(MAC1 + 1) == *(MAC2 + 1)) && (*(MAC1 + 2) == *(MAC2 + 2)))

#define ever (;;)

#define IFNAME0 'D' /* Stands for "DPDK" */
#define IFNAME1 'I' /* Stands for "Interface" */
#define DPDK_MAX_RX_BURST (32)

struct dpdkif 
{
    u8_t   portid;
};

extern struct rte_mempool * dpdk_pktmbuf_pool;
static struct netif * port2netif_map[RTE_MAX_ETHPORTS] = {NULL};


static err_t
low_level_output(struct netif *netif, struct pbuf *p)
{
    u8_t * pkt_ptr;
    struct pbuf * q = p;
    struct rte_mbuf * mbuf = rte_pktmbuf_alloc(dpdk_pktmbuf_pool);
    
    if (NULL == mbuf) 
    {
        return ERR_MEM;
    }

    pkt_ptr = rte_pktmbuf_mtod(mbuf, u8_t *);

    for (q = p; NULL != q; q = q->next) 
    {
        rte_memcpy(pkt_ptr, q->payload, q->len);
        pkt_ptr += q->len;
    }

    rte_pktmbuf_append(mbuf, p->tot_len);

    if(1 == rte_eth_tx_burst(((struct dpdkif*)(netif->state))->portid, 0 /*TODO*/, &mbuf, 1))
    {
        return ERR_OK;
    }

    else
    {
        rte_pktmbuf_free(mbuf);
        return ERR_IF;
    }
}


err_t
dpdkif_init(struct netif *netif)
{
    u8_t nb_ports, portid;
    struct ether_addr port_eth_addr;
    struct dpdkif * dpdkif;
    u16_t * mac1p, * mac2p;
    s32_t res;

    nb_ports = rte_eth_dev_count();

    for (portid = 0; portid < nb_ports; portid++) 
    {
        rte_eth_macaddr_get(portid, &port_eth_addr);
        mac1p = (u16_t*) &port_eth_addr.addr_bytes[0];
        mac2p = (u16_t*) &netif->hwaddr[0];

        if (CMP_MAC_ADDR(mac1p, mac2p)) 
        {
            break;
        }
    }

    if (portid < nb_ports) 
    {
        dpdkif = mem_malloc(sizeof(struct dpdkif));

        if (NULL == dpdkif) 
        {
            return ERR_MEM;
        }

        dpdkif->portid = portid;

        netif->state = dpdkif;
        netif->name[0] = IFNAME0;
        netif->name[1] = IFNAME1;
        netif->mtu = 1500;
        netif->hwaddr_len = 6;
        netif->flags = NETIF_FLAG_BROADCAST | NETIF_FLAG_ETHARP | NETIF_FLAG_IGMP;
        netif->linkoutput = low_level_output;
        netif->output = etharp_output;
        
        port2netif_map[portid] = netif;

/*
        res = rte_eth_dev_start(portid);

        if (res < 0)
        {
            LWIP_PLATFORM_DIAG(("rte_eth_dev_start:err=%d, port=%hhu\n", res, portid));
            abort();
        }

        LWIP_PLATFORM_DIAG(("done: \n"));

        rte_eth_promiscuous_enable(portid);

        LWIP_PLATFORM_DIAG(("Port %hhu, MAC address: %02X:%02X:%02X:%02X:%02X:%02X\n\n",
                           portid,
                           port_eth_addr.addr_bytes[0],
                           port_eth_addr.addr_bytes[1],
                           port_eth_addr.addr_bytes[2],
                           port_eth_addr.addr_bytes[3],
                           port_eth_addr.addr_bytes[4],
                           port_eth_addr.addr_bytes[5]));
*/
    }

    else
    {
        LWIP_PLATFORM_DIAG(("Interface with MAC address: %02X:%02X:%02X:%02X:%02X:%02X was not found!\n\n",
                          port_eth_addr.addr_bytes[0],
                          port_eth_addr.addr_bytes[1],
                          port_eth_addr.addr_bytes[2],
                          port_eth_addr.addr_bytes[3],
                          port_eth_addr.addr_bytes[4],
                          port_eth_addr.addr_bytes[5]));
        abort();  
    }

    return ERR_OK;
}


static void
dpdkif_input(struct netif *netif, struct pbuf *p)
{

    struct ether_hdr *ethhdr;

    ethhdr = (struct ether_hdr *)p->payload;

    switch(htons(ethhdr->ether_type)) 
    {
    /* IP or ARP packet? */
    case ETHTYPE_IP:
    case ETHTYPE_ARP:
    #if PPPOE_SUPPORT
    /* PPPoE packet? */
    case ETHTYPE_PPPOEDISC:
    case ETHTYPE_PPPOE:
    #endif /* PPPOE_SUPPORT */
    /* full packet send to tcpip_thread to process */
    if (netif->input(p, netif) != ERR_OK) 
    {
        LWIP_DEBUGF(NETIF_DEBUG, ("ethernetif_input: IP input error\n"));
        pbuf_free(p);
        p = NULL;
    }
    break;
    default:
        pbuf_free(p);
    break;
    }
}


inline void
low_level_input(struct netif *netif, struct rte_mbuf ** rx_mbuf_arr, u16_t nb_rx_pkts)
{
    struct pbuf *p, *q;
    u8_t * pkt_ptr, pktid;
    u16_t len;

    for (pktid = 0; pktid < nb_rx_pkts; pktid++) 
    {
        pkt_ptr = rte_pktmbuf_mtod(rx_mbuf_arr[pktid], u8_t *);
        len = rte_pktmbuf_pkt_len(rx_mbuf_arr[pktid]);
        p = pbuf_alloc(PBUF_RAW, len, PBUF_POOL);

        if (NULL != p) 
        {
            for(q = p; q != NULL; q = q->next) 
            {
                rte_memcpy(q->payload, pkt_ptr, q->len);
                pkt_ptr += q->len;
            }
        }

        else
        {
            break;
        }

        /* Pass this packet to stack for processing */
        dpdkif_input(netif, p);
    }

    rte_mempool_put_bulk(dpdk_pktmbuf_pool, rx_mbuf_arr, nb_rx_pkts);
}


int
dpdkif_rx_thread_func(void * arg)
{
    LWIP_UNUSED_ARG(arg);

    struct netif * netif;
    struct rte_mbuf * rx_mbuf_arr[DPDK_MAX_RX_BURST];
    u8_t nb_ports = rte_eth_dev_count(), portid;
    u16_t nb_rx_pkts;
    u32_t core_id = rte_lcore_id();

    for ever
    {
        for (portid = 0; portid < nb_ports; portid++) 
        {
            nb_rx_pkts = rte_eth_rx_burst(portid, 0, rx_mbuf_arr, DPDK_MAX_RX_BURST);

            if (0 != nb_rx_pkts) 
            {
                //LWIP_PLATFORM_DIAG(("Received %hu packets\n", nb_rx_pkts));
                netif = port2netif_map[portid];
                low_level_input(netif, rx_mbuf_arr, nb_rx_pkts);
            }
        }
    }

    return 0;
}
