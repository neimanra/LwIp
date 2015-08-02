
/*
 * Copyright (c) 2001-2003 Swedish Institute of Computer Science.
 * All rights reserved. 
 * 
 * Redistribution and use in source and binary forms, with or without modification, 
 * are permitted provided that the following conditions are met:
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
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT 
 * SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, 
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT 
 * OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS 
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN 
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING 
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY 
 * OF SUCH DAMAGE.
 * 
 * Author: Kieran Mansley <kjm25@cam.ac.uk>
 *
 * $Id: unixlib.c,v 1.10 2010/02/17 16:52:30 goldsimon Exp $
 */

/*-----------------------------------------------------------------------------------*/
/* unixlib.c
 *
 * The initialisation functions for a shared library
 *
 * You may need to configure this file to your own needs - it is only an example
 * of how lwIP can be used as a self initialising shared library.
 *
 * In particular, you should change the gateway, ipaddr, and netmask to be the values
 * you would like the stack to use.
 */
/*-----------------------------------------------------------------------------------*/
#include "lwip/init.h"
#include "lwip/sys.h"
#include "lwip/mem.h"
#include "lwip/memp.h"
#include "lwip/pbuf.h"
#include "lwip/tcp.h"
#include "lwip/tcpip.h"
#include "lwip/netif.h"
#include "lwip/stats.h"
#include "lwip/sockets.h"
#include "netif/etharp.h"
#include "lwip/api.h"

#include <rte_launch.h>
#include <rte_cycles.h>
#include "dpdkif.h"

struct netif netif;

static void
tcpip_init_done(void *arg)
{

  ip_addr_t ipaddr, netmask, gateway;
  sys_sem_t *sem;
  sem = arg;

  netif.hwaddr[0] = 0x8;
  netif.hwaddr[1] = 0x0;
  netif.hwaddr[2] = 0x27;
  netif.hwaddr[3] = 0xd5;
  netif.hwaddr[4] = 0x3;
  netif.hwaddr[5] = 0xf4;
  IP4_ADDR(&gateway, 192,168,56,1);
  IP4_ADDR(&ipaddr, 192,168,56,101);
  IP4_ADDR(&netmask, 255,255,255,0);
  
  netif_set_default(netif_add(&netif, &ipaddr, &netmask, &gateway, NULL, dpdkif_init,
			      tcpip_input/*ethernet_input*/));

  netif_set_up(&netif);
  sys_sem_signal(sem);
}

int main()
{
  sys_sem_t sem;
  ip_addr_t ipaddr, netmask, gateway;
	
  sys_init();

  if(sys_sem_new(&sem, 0) != ERR_OK) {
    LWIP_ASSERT("failed to create semaphore", 0);
  }
  tcpip_init(tcpip_init_done, &sem);

 // rte_eal_mp_remote_launch ((lcore_function_t *)dpdkif_rx_thread_func, NULL, SKIP_MASTER);

  sys_sem_wait(&sem);
  sys_sem_free(&sem);

///////////////////////////////////////////////////////////////////////////////////////////////////

  struct netconn *conn, *newconn;
  err_t err;

  /* Create a new connection identifier. */
  conn = netconn_new(NETCONN_TCP);

  netconn_set_noautorecved(conn, 0);
  tcp_nagle_disable(conn->pcb.tcp);

  /* Bind connection to well known port number 7. */
  netconn_bind(conn, NULL, 80);

  /* Tell connection to go into listening mode. */
  netconn_listen(conn);

  while (1) {

    /* Grab new connection. */
    err = netconn_accept(conn, &newconn);
    printf("accepted new connection %p\n", newconn);
    /* Process the new connection. */
    if (err == ERR_OK) {
      struct netbuf *buf;
      void *data;
      u16_t len;
      uint64_t total_rcvd = 0;
      uint64_t eal_tsc_resolution_hz = rte_get_timer_hz();
      uint64_t  end = get_tsc() + eal_tsc_resolution_hz;

      while ((err = netconn_recv(newconn, &buf)) == ERR_OK) {
          
        //printf("Recved\n");
        //do {
             netbuf_data(buf, &data, &len);

             if (len > 0) 
             {
    			printf("%llu \n", total_rcvd);
    			total_rcvd = 0;
    			end = get_tsc() + eal_tsc_resolution_hz;
             }
             
             //err = netconn_write(newconn, data, len, NETCONN_COPY);
             
#if 0
            if (err != ERR_OK) {
              printf("tcpecho: netconn_write: error \"%s\"\n", lwip_strerr(err));
            }
#endif
        //} while (netbuf_next(buf) >= 0);
        netbuf_delete(buf);
      }
      /*printf("Got EOF, looping\n");*/ 
      /* Close connection and discard connection identifier. */
      netconn_close(newconn);
      netconn_delete(newconn);
    }
  }

  while (1); 
}
//void _fini(void){
//}
