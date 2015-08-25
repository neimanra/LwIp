
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
#include "lwip/timers.h"
#include "netif/etharp.h"
#include "dpdkif.h"

#define MONITOR_CPU_LOAD (1)

/* Compare 2 16bit aligned MAC addresses */
#define CMP_MAC_ADDR(MAC1, MAC2)\
(((*MAC1) == (*MAC2)) && (*(MAC1 + 1) == *(MAC2 + 1)) && (*(MAC1 + 2) == *(MAC2 + 2)))

#define ever (;;)

#define IFNAME0 'D' /* Stands for "DPDK" */
#define IFNAME1 'I' /* Stands for "Interface" */
#define DPDK_MAX_RX_BURST (32)
#define DPDKIF_MBUF_PBUF_OFFSET (RTE_PKTMBUF_HEADROOM - sizeof(struct pbuf))


struct dpdkif 
{
    u8_t   portid;
    u8_t   pktid;
    u16_t   pkts_ready;
    struct rte_mbuf * pkt_arr[DPDK_MAX_RX_BURST];
};

extern struct rte_mempool * dpdk_pktmbuf_pool;
static struct netif * port2netif_map[RTE_MAX_ETHPORTS] = {NULL};

static inline
struct pbuf *
dpdkif_rx_pkt(struct netif * inp);

static inline
struct pbuf * 
sys_arch_mbuf_to_pbuf(struct rte_mbuf  * mbuf)
{
    return  (struct pbuf*)(((u8_t*)mbuf) + DPDKIF_MBUF_PBUF_OFFSET);
}

static inline
struct rte_mbuf * 
sys_arch_pbuf_to_mbuf(struct pbuf  * pbuf)
{
    return  (struct rte_mbuf*)(((u8_t*)pbuf) - DPDKIF_MBUF_PBUF_OFFSET);
}

static err_t
low_level_output(struct netif *netif, struct pbuf *p)
{
    struct rte_mbuf * mbuf = sys_arch_pbuf_to_mbuf(p);
   
    mbuf->data_off = p->payload - mbuf->buf_addr;
    mbuf->data_len = p->tot_len;

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
        dpdkif->pktid = 0;
        dpdkif->pkts_ready = 0;

        netif->state = dpdkif;
        netif->name[0] = IFNAME0;
        netif->name[1] = IFNAME1;
        netif->mtu = 1500;
        netif->hwaddr_len = 6;
        netif->flags = NETIF_FLAG_BROADCAST | NETIF_FLAG_ETHARP | NETIF_FLAG_IGMP;
        netif->linkoutput = low_level_output;
        netif->output = etharp_output;
        
        port2netif_map[portid] = netif;
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
        //ethernet_input(p, netif);
        //break;
    #if PPPOE_SUPPORT
    /* PPPoE packet? */
    case ETHTYPE_PPPOEDISC:
    case ETHTYPE_PPPOE:
    #endif /* PPPOE_SUPPORT */
    //pbuf_header(p, -((s16_t)sizeof(struct eth_hdr)));
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

    rte_mempool_put_bulk(dpdk_pktmbuf_pool, (void * const *)rx_mbuf_arr, nb_rx_pkts);
}


int
dpdkif_rx_thread_func(void * arg)
{
    LWIP_UNUSED_ARG(arg);

    struct netif * netif;
    struct pbuf * p;
    u8_t dev_idx;
    u8_t dev_count = rte_eth_dev_count();

#ifdef MONITOR_CPU_LOAD
    u64_t hz = rte_get_timer_hz();
    u64_t cycles_end = rte_get_timer_cycles() + hz;
    u64_t busy_start =0, busy_total = 0;
#endif
    for ever
    {
        //Loop through all the interfaces
        for (dev_idx = 0; dev_idx < dev_count; dev_idx++) 
        {
            netif = port2netif_map[dev_idx];

            p = dpdkif_rx_pkt(netif);

            if(p != NULL)
            {
#ifdef MONITOR_CPU_LOAD
                busy_start = rte_get_timer_cycles();
#endif
                if (netif->input(p, netif) != ERR_OK)
                {
                    LWIP_DEBUGF(NETIF_DEBUG, ("ethernetif_input: IP input error\n"));
                    pbuf_free(p);
                    p = NULL;
                }
            }
#ifdef MONITOR_CPU_LOAD
            else if (busy_start) 
            {
                busy_total += rte_get_timer_cycles() - busy_start;
                busy_start = 0;
            }

            if (rte_get_timer_cycles() >= cycles_end)
            {
                if (busy_start) 
                {
                    busy_total += rte_get_timer_cycles() - busy_start;
                    busy_start = 0;
                }

                printf("Utilization:%lf%%\n", (double)(busy_total * 100) / hz);
                busy_total = 0;
                cycles_end = rte_get_timer_cycles() + hz;
            }
#endif
        }

    	sys_check_timeouts();
    }

    return 0;
}


struct pbuf *
dpdkif_rx_pkt(struct netif * inp)
{
    struct rte_mbuf * mbuf;
    struct dpdkif * dpdkif;
    struct pbuf *p;
    u16_t len;

    dpdkif = inp->state;

    //If we have cached packets - use them
    if(dpdkif->pkts_ready)
    {
    	mbuf = dpdkif->pkt_arr[dpdkif->pktid];
    	dpdkif->pktid++;
    	dpdkif->pkts_ready--;
    }

    //Poll driver otherwise
    else
    {
    	dpdkif->pkts_ready = rte_eth_rx_burst(dpdkif->portid, 0, dpdkif->pkt_arr, DPDK_MAX_RX_BURST);

        if(dpdkif->pkts_ready)
        {
        	mbuf = dpdkif->pkt_arr[0];
        	dpdkif->pktid = 1;
        	dpdkif->pkts_ready--;
        }

        else
        {
        	return NULL;
        }
    }

	len = rte_pktmbuf_pkt_len(mbuf);

	p = sys_arch_mbuf_to_pbuf(mbuf);

	p->tot_len = len;
	/* set the length of the first pbuf in the chain */
	p->len = len;

	/* set reference count (needed here in case we fail) */
	p->ref = 1;
	p->type = PBUF_POOL;
	p->next = NULL;

	/* make the payload pointer point 'offset' bytes into pbuf data memory */
	p->payload = rte_pktmbuf_mtod(mbuf, void *);

	return p;
}


struct pbuf *
dpdkif_fetch_pkt(struct netif ** inp)
{
    struct rte_mbuf * mbuf;
    struct dpdkif * dpdkif;
    struct pbuf *p;
    u16_t len;

    *inp = port2netif_map[0];
    dpdkif = (*inp)->state;

    //If we have cached packets - use them
    if(dpdkif->pkts_ready)
    {
    	mbuf = dpdkif->pkt_arr[dpdkif->pktid];
    	dpdkif->pktid++;
    	dpdkif->pkts_ready--;
    }

    //Poll driver otherwise
    else
    {
    	dpdkif->pkts_ready = rte_eth_rx_burst(0, 0, dpdkif->pkt_arr, DPDK_MAX_RX_BURST);

        if(dpdkif->pkts_ready)
        {
        	mbuf = dpdkif->pkt_arr[0];
        	dpdkif->pktid = 1;
        	dpdkif->pkts_ready--;
        }

        else
        {
        	return NULL;
        }
    }

	len = rte_pktmbuf_pkt_len(mbuf);

	p = sys_arch_mbuf_to_pbuf(mbuf);

	p->tot_len = len;
	/* set the length of the first pbuf in the chain */
	p->len = len;

	/* set reference count (needed here in case we fail) */
	p->ref = 1;
	p->type = PBUF_POOL;
	p->next = NULL;

	/* make the payload pointer point 'offset' bytes into pbuf data memory */
	p->payload = rte_pktmbuf_mtod(mbuf, void *);

	return p;
}

int dpdkif_get_if_params(ip_addr_t* ipaddr, ip_addr_t* netmask, ip_addr_t* gateway, uint8_t * hw_addr)
{
	FILE * fd = fopen("proj/conf/config", "r");
	s8_t * conf_string = malloc(100);
	size_t string_len;
	s8_t bytes_read, values_parsed;
	u8_t ip_arr[4];

	if(NULL == fd)
	{
		rte_exit(-1,"Failed to open config file!\n");
	}

	/* Read first line */
	bytes_read = getline(&conf_string, &string_len, fd);

	if(-1 == bytes_read)
	{
		fclose(fd);
		rte_exit(-1,"Cannot read config file!\n");
	}

	values_parsed = sscanf(conf_string,"%hhu.%hhu.%hhu.%hhu", &ip_arr[0],&ip_arr[1],&ip_arr[2],&ip_arr[3]);

	if(4 != values_parsed)
	{
		fclose(fd);
		rte_exit(-1,"Cannot parse config values!\n");
	}

	IP4_ADDR(ipaddr, ip_arr[0], ip_arr[1], ip_arr[2], ip_arr[3]);

	//Read second line
	bytes_read = getline(&conf_string, &string_len, fd);

	if(-1 == bytes_read)
	{
		fclose(fd);
		rte_exit(-1,"Cannot read config file!\n");
	}

	values_parsed = sscanf(conf_string,"%hhu.%hhu.%hhu.%hhu", &ip_arr[0],&ip_arr[1],&ip_arr[2],&ip_arr[3]);

	if(4 != values_parsed)
	{
		fclose(fd);
		rte_exit(-1,"Cannot parse config values!\n");
	}

	IP4_ADDR(gateway, ip_arr[0], ip_arr[1], ip_arr[2], ip_arr[3]);

	//Read third line
	bytes_read = getline(&conf_string, &string_len, fd);

	if(-1 == bytes_read)
	{
		fclose(fd);
		rte_exit(-1,"Cannot read config file!\n");
	}

	values_parsed = sscanf(conf_string,"%hhu.%hhu.%hhu.%hhu", &ip_arr[0],&ip_arr[1],&ip_arr[2],&ip_arr[3]);

	if(4 != values_parsed)
	{
		fclose(fd);
		rte_exit(-1,"Cannot parse config values!\n");
	}

	IP4_ADDR(netmask, ip_arr[0], ip_arr[1], ip_arr[2], ip_arr[3]);


	//Read hw address
	bytes_read = getline(&conf_string, &string_len, fd);

	if(-1 == bytes_read)
	{
		fclose(fd);
		rte_exit(-1,"Cannot read config file!\n");
	}

	values_parsed = sscanf(conf_string,"%hhx:%hhx:%hhx:%hhx:%hhx:%hhx", &hw_addr[0],&hw_addr[1],&hw_addr[2],&hw_addr[3],&hw_addr[4], &hw_addr[5]);

	if(6 != values_parsed)
	{
		fclose(fd);
		rte_exit(-1,"Cannot parse config values!\n");
	}

	fclose(fd);
	free(conf_string);

	return 0;
}
