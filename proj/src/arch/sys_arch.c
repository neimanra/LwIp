
#include <string.h>
#include <pthread.h>
#include <rte_config.h>
#include <rte_common.h>
#include <rte_log.h>
#include <rte_memory.h>
#include <rte_memzone.h>
#include <rte_eal.h>
#include <rte_per_lcore.h>
#include <rte_launch.h>
#include <rte_atomic.h>
#include <rte_cycles.h>
#include <rte_prefetch.h>
#include <rte_lcore.h>
#include <rte_per_lcore.h>
#include <rte_branch_prediction.h>
#include <rte_interrupts.h>
#include <rte_pci.h>
#include <rte_random.h>
#include <rte_debug.h>
#include <rte_ether.h>
#include <rte_ethdev.h>
#include <rte_ring.h>
#include <rte_mempool.h>
#include <rte_mbuf.h>
#include <rte_spinlock.h>
#include <rte_malloc.h>

#include "cc.h"
#include "sys_arch.h"
#include "lwip/err.h"
#include "lwip/sys.h"
#include "lwip/pbuf.h"
#include "dpdkif.h"

#define US_SLEEP_VAL (100)

#define NB_MBUF      (8192)
#define MBUF_SIZE    (2048 + sizeof(struct rte_mbuf) + RTE_PKTMBUF_HEADROOM)
#define MBUF_PBUF_OFFSET (RTE_PKTMBUF_HEADROOM - sizeof(struct pbuf))

#define RX_DESC_DEFAULT (128)
#define TX_DESC_DEFAULT (512)

struct sys_mbox 
{
   struct  rte_ring * ring;
};

/*
struct sys_sem 
{
   rte_spinlock_t spinlock;
};
*/

struct sys_sem {
  unsigned int c;
  pthread_cond_t cond;
  pthread_mutex_t mutex;
};

struct sys_thread 
{
  struct sys_thread *next;
  pthread_t pthread;
};

static u64_t clock_ticks_sec;
static u64_t clock_ticks_msec;

static struct sys_thread *threads = NULL;
static pthread_mutex_t threads_mutex = PTHREAD_MUTEX_INITIALIZER;

static s8_t *dpdk_argv[] = {"LwIp", "-c", "0x3", "-n", "2", NULL};
static s32_t dpdk_argc = sizeof(dpdk_argv) / sizeof(char*) - 1;

struct rte_mempool * dpdk_pktmbuf_pool = NULL;

static const struct rte_eth_conf port_conf = 
{
	.rxmode = 
    {
		.split_hdr_size = 0,
		.header_split   = 0, /**< Header Split disabled */
		.hw_ip_checksum = 0, /**< IP checksum offload disabled */
		.hw_vlan_filter = 0, /**< VLAN filtering disabled */
		.jumbo_frame    = 0, /**< Jumbo Frame Support disabled */
		.hw_strip_crc   = 0, /**< CRC stripped by hardware */
	},
	.txmode = 
    {
		.mq_mode = ETH_MQ_TX_NONE,
	},
};


void sys_init(void)
{
    static u8_t initialized = 0;
    u8_t  nb_ports, nb_cores, portid, queueid;
    struct ether_addr port_eth_addr;

    if (0 == initialized) 
    {
        initialized = 1;

        s32_t res = rte_eal_init(dpdk_argc, dpdk_argv);

        if (res < 0)
        {
            LWIP_PLATFORM_DIAG(("Invalid EAL arguments\n"));
            abort();
        }

        /*Initialize system time constants*/
        clock_ticks_sec = rte_get_timer_hz();
        clock_ticks_msec = clock_ticks_sec / 1000;

        LWIP_PLATFORM_DIAG(("CPU Hz: %llu\n", clock_ticks_sec));

        dpdk_pktmbuf_pool = rte_mempool_create("mbuf_pool", 
                                               NB_MBUF,
                                               MBUF_SIZE, 
                                               0,
                                               sizeof(struct rte_pktmbuf_pool_private),
                                               rte_pktmbuf_pool_init, NULL,
                                               rte_pktmbuf_init, NULL,
                                               rte_socket_id(), 0);

        if (dpdk_pktmbuf_pool == NULL)
        {
            LWIP_PLATFORM_DIAG(("Cannot init mbuf pool\n"));
            abort();
        }

        nb_cores = rte_lcore_count();
        LWIP_PLATFORM_DIAG(("Detected %hhu cores\n", nb_cores));

        nb_ports = rte_eth_dev_count();

        if (nb_ports == 0)
        {
            LWIP_PLATFORM_DIAG(("Error: no ethernet ports detected\n"));
            abort();
        }

        else
        {
            LWIP_PLATFORM_DIAG(("Detected %hhu ports\n", nb_ports));
        }

        for (portid = 0; portid < nb_ports; portid++) 
        {
            LWIP_PLATFORM_DIAG(("Initializing port %hhu... ", portid));

            res = rte_eth_dev_configure(portid, 1, 1, &port_conf);

            if (res < 0)
            {
                LWIP_PLATFORM_DIAG(("Cannot configure device: err=%d, port=%hhu\n", res, portid));
                abort();
            }

            rte_eth_macaddr_get(portid, &port_eth_addr);

            for (queueid = 0; queueid < 1; queueid++) 
            {
                res = rte_eth_rx_queue_setup(portid, queueid, RX_DESC_DEFAULT,
                                             rte_eth_dev_socket_id(portid),
                                             NULL,
                                             dpdk_pktmbuf_pool);

                if (res < 0)
                {
                    LWIP_PLATFORM_DIAG(("rte_eth_rx_queue_setup:err=%d, port=%hhu\n", res, portid));
                    abort();
                }

                res = rte_eth_tx_queue_setup(portid, queueid, TX_DESC_DEFAULT,
                                             rte_eth_dev_socket_id(portid),
                                             NULL);

                if (res < 0)
                {
                    LWIP_PLATFORM_DIAG(("rte_eth_tx_queue_setup:err=%d, port=%hhu\n", res, portid));
                    abort();
                }

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
            }
        }
   }
}

/*
err_t sys_sem_new(sys_sem_t *sem, u8_t count)
{
   struct sys_sem * isem = (struct sys_sem *)rte_malloc("NORM", sizeof(struct sys_sem), 0);

   if (NULL != isem) 
   {
      rte_spinlock_init(&isem->spinlock);

      if (0 == count) 
      {
         rte_spinlock_lock (&isem->spinlock);
      }

      *sem = isem;

      return ERR_OK;
   }

   return ERR_MEM;
}


void sys_sem_free(sys_sem_t *sem)
{
   rte_free(*sem);
}

void sys_sem_signal(sys_sem_t *sem)
{
   struct sys_sem * isem = *sem;
   rte_spinlock_unlock (&isem->spinlock);
}

u32_t sys_arch_sem_wait(sys_sem_t *sem, u32_t timeout)
{
   struct sys_sem * isem = *sem;
   u64_t start_ticks = rte_get_timer_cycles();

   if (0 != timeout) 
   {
      u64_t curr_ticks = start_ticks;
      u64_t end_ticks = start_ticks + (timeout * clock_ticks_msec);

      while (curr_ticks < end_ticks) 
      {
         if(0 == rte_spinlock_trylock(&isem->spinlock))
         {
            rte_delay_us(US_SLEEP_VAL);
            curr_ticks = rte_get_timer_cycles();
         }

         else
         {
            return (curr_ticks - start_ticks) / clock_ticks_msec;
         }
      }

      return SYS_ARCH_TIMEOUT;
   }

   else
   {
      rte_spinlock_lock (&isem->spinlock);
      return (rte_get_timer_cycles() - start_ticks) / clock_ticks_msec;
   }
}
*/

err_t sys_mbox_new(sys_mbox_t *mbox, int size)
{
    s32_t res;
   struct sys_mbox * imbox = rte_malloc("NORM", sizeof(struct sys_mbox), 0);

   if (NULL != imbox)
   {
      size = rte_align32pow2((u32_t)size);	
      ssize_t mem_size = rte_ring_get_memsize((unsigned)size);	
      imbox->ring = rte_malloc("NORM", mem_size, 0);

      if (NULL != imbox->ring) 
      {
         res = rte_ring_init (imbox->ring, "NORM", (unsigned)size, 0);

         if (0 != res) 
         {
             rte_free(imbox->ring);
             rte_free(imbox);
             return ERR_MEM;
         }
         *mbox = imbox;
         return ERR_OK;
      }

      else
      {
         rte_free(imbox);
         return ERR_MEM;
      }
   }

   else
   {
      return ERR_MEM;
   }	
}

void sys_mbox_free(sys_mbox_t *mbox)
{
   rte_free((*mbox)->ring);
   rte_free(*mbox);
}

void sys_mbox_post(sys_mbox_t *mbox, void *msg)
{
   s32_t res = rte_ring_enqueue( (*mbox)->ring, msg);

   while (0 != res) 
   {
      //rte_delay_us(US_SLEEP_VAL);
      res = rte_ring_enqueue( (*mbox)->ring, msg);
   }
}


err_t sys_mbox_trypost(sys_mbox_t *mbox, void *msg)
{
   return (0 == rte_ring_enqueue( (*mbox)->ring, msg)) ? ERR_OK : ERR_MEM;
}


u32_t sys_arch_mbox_fetch(sys_mbox_t *mbox, void **msg, u32_t timeout)
{
   struct sys_mbox * imbox = *mbox;
   u64_t start_ticks = rte_get_timer_cycles();

   if (0 != timeout) 
   {
      u64_t curr_ticks = start_ticks;
      u64_t end_ticks = start_ticks + (timeout * clock_ticks_msec);

      while (curr_ticks < end_ticks) 
      {
         if(0 != rte_ring_dequeue(imbox->ring, msg))
         {
            //rte_delay_us(US_SLEEP_VAL);
            curr_ticks = rte_get_timer_cycles();
         }

         else
         {
            return (curr_ticks - start_ticks) / clock_ticks_msec;
         }
      }

      return SYS_ARCH_TIMEOUT;
   }

   else
   {
      while (1) 
      {
         if(0 != rte_ring_dequeue(imbox->ring, msg))
         {
            //rte_delay_us(US_SLEEP_VAL);
         }

         else
         {
            return (rte_get_timer_cycles() - start_ticks) / clock_ticks_msec;
         }
      }
   }
}


u32_t sys_arch_mbox_tryfetch(sys_mbox_t *mbox, void **msg)
{
   return (0 == rte_ring_dequeue( (*mbox)->ring, msg)) ? ERR_OK : SYS_MBOX_EMPTY;
}


static struct sys_thread * introduce_thread(pthread_t id)
{
  struct sys_thread *thread;

  thread = (struct sys_thread *)malloc(sizeof(struct sys_thread));

  if (thread != NULL) {
    pthread_mutex_lock(&threads_mutex);
    thread->next = threads;
    thread->pthread = id;
    threads = thread;
    pthread_mutex_unlock(&threads_mutex);
  }

  return thread;
}


sys_thread_t sys_thread_new(const char *name, lwip_thread_fn function, void *arg, int stacksize, int prio)
{
  int code;
  pthread_t tmp;
  struct sys_thread *st = NULL;
  LWIP_UNUSED_ARG(name);
  LWIP_UNUSED_ARG(stacksize);
  LWIP_UNUSED_ARG(prio);

  code = pthread_create(&tmp,
                        NULL, 
                        (void *(*)(void *)) 
                        function, 
                        arg);
  
  if (0 == code) 
  {
    st = introduce_thread(tmp);
  }
  
  if (NULL == st) 
  {
    LWIP_DEBUGF(SYS_DEBUG, ("sys_thread_new: pthread_create %d, st = 0x%lx",
                       code, (unsigned long)st));
    abort();
  }

  return st;
}


u32_t sys_now(void)
{
   return rte_get_timer_cycles() / clock_ticks_msec;
}


struct pbuf * sys_arch_allocate_pbuf()
{
    struct rte_mbuf * mbuf = rte_pktmbuf_alloc(dpdk_pktmbuf_pool);

    if (NULL != mbuf) 
    {
        return  (struct pbuf*)(((u8_t*)mbuf) + MBUF_PBUF_OFFSET);
    }

    return NULL;
}


void sys_arch_free_pbuf(struct pbuf * pbuf)
{
    rte_pktmbuf_free((((u8_t*)pbuf) - MBUF_PBUF_OFFSET));
}


/*-----------------------------------------------------------------------------------*/
static struct sys_sem *
sys_sem_new_internal(u8_t count)
{
  struct sys_sem *sem;

  sem = (struct sys_sem *)malloc(sizeof(struct sys_sem));
  if (sem != NULL) {
    sem->c = count;
    pthread_cond_init(&(sem->cond), NULL);
    pthread_mutex_init(&(sem->mutex), NULL);
  }
  return sem;
}
/*-----------------------------------------------------------------------------------*/
err_t
sys_sem_new(struct sys_sem **sem, u8_t count)
{
  //SYS_STATS_INC_USED(sem);
  *sem = sys_sem_new_internal(count);
  if (*sem == NULL) {
    return ERR_MEM;
  }
  return ERR_OK;
}
/*-----------------------------------------------------------------------------------*/
static u32_t
cond_wait(pthread_cond_t *cond, pthread_mutex_t *mutex, u32_t timeout)
{
  time_t tdiff;
  time_t sec, usec;
  struct timeval rtime1, rtime2;
  struct timespec ts;
  int retval;

  if (timeout > 0) {
    /* Get a timestamp and add the timeout value. */
    gettimeofday(&rtime1, NULL);
    sec = rtime1.tv_sec;
    usec = rtime1.tv_usec;
    usec += timeout % 1000 * 1000;
    sec += (int)(timeout / 1000) + (int)(usec / 1000000);
    usec = usec % 1000000;
    ts.tv_nsec = usec * 1000;
    ts.tv_sec = sec;

    retval = pthread_cond_timedwait(cond, mutex, &ts);

    if (retval == ETIMEDOUT) {
      return SYS_ARCH_TIMEOUT;
    } else {
      /* Calculate for how long we waited for the cond. */
      gettimeofday(&rtime2, NULL);
      tdiff = (rtime2.tv_sec - rtime1.tv_sec) * 1000 +
        (rtime2.tv_usec - rtime1.tv_usec) / 1000;

      if (tdiff <= 0) {
        return 0;
      }
      return (u32_t)tdiff;
    }
  } else {
    pthread_cond_wait(cond, mutex);
    return 0;
  }
}
/*-----------------------------------------------------------------------------------*/
u32_t
sys_arch_sem_wait(struct sys_sem **s, u32_t timeout)
{
  u32_t time_needed = 0;
  struct sys_sem *sem;
  LWIP_ASSERT("invalid sem", (s != NULL) && (*s != NULL));
  sem = *s;

  pthread_mutex_lock(&(sem->mutex));
  while (sem->c <= 0) {
    if (timeout > 0) {
      time_needed = cond_wait(&(sem->cond), &(sem->mutex), timeout);

      if (time_needed == SYS_ARCH_TIMEOUT) {
        pthread_mutex_unlock(&(sem->mutex));
        return SYS_ARCH_TIMEOUT;
      }
      /*      pthread_mutex_unlock(&(sem->mutex));
              return time_needed; */
    } else {
      cond_wait(&(sem->cond), &(sem->mutex), 0);
    }
  }
  sem->c--;
  pthread_mutex_unlock(&(sem->mutex));
  return (u32_t)time_needed;
}
/*-----------------------------------------------------------------------------------*/
void
sys_sem_signal(struct sys_sem **s)
{
  struct sys_sem *sem;
  LWIP_ASSERT("invalid sem", (s != NULL) && (*s != NULL));
  sem = *s;

  pthread_mutex_lock(&(sem->mutex));
  sem->c++;

  if (sem->c > 1) {
    sem->c = 1;
  }

  pthread_cond_broadcast(&(sem->cond));
  pthread_mutex_unlock(&(sem->mutex));
}
/*-----------------------------------------------------------------------------------*/
static void
sys_sem_free_internal(struct sys_sem *sem)
{
  pthread_cond_destroy(&(sem->cond));
  pthread_mutex_destroy(&(sem->mutex));
  free(sem);
}
/*-----------------------------------------------------------------------------------*/
void
sys_sem_free(struct sys_sem **sem)
{
  if ((sem != NULL) && (*sem != NULL)) {
    //SYS_STATS_DEC(sem.used);
    sys_sem_free_internal(*sem);
  }
}

