#ifndef __ARCH_SYS_ARCH_H__
#define __ARCH_SYS_ARCH_H__

struct sys_sem;
typedef struct sys_sem * sys_sem_t;

struct sys_mbox;
typedef struct sys_mbox * sys_mbox_t;

struct sys_thread;
typedef struct sys_thread * sys_thread_t;

#define LWIP_COMPAT_MUTEX 1

struct pbuf * sys_arch_allocate_pbuf();
void sys_arch_free_pbuf(struct pbuf * pbuf);

#endif /* __ARCH_SYS_ARCH_H__ */
