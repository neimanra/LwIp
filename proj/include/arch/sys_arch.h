#ifndef __ARCH_SYS_ARCH_H__
#define __ARCH_SYS_ARCH_H__

struct sys_sem;
typedef struct sys_sem * sys_sem_t;

struct sys_mbox;
typedef struct sys_mbox * sys_mbox_t;

struct sys_thread;
typedef struct sys_thread * sys_thread_t;

#define LWIP_COMPAT_MUTEX 1

#define sys_sem_valid(sem) (((sem) != NULL) && (*(sem) != NULL))
#define sys_sem_set_invalid(sem) do { if((sem) != NULL) { *(sem) = NULL; }}while(0)

#define sys_mbox_valid(mbox) (((mbox) != NULL) && (*(mbox) != NULL))
#define sys_mbox_set_invalid(mbox) do { if((mbox) != NULL) { *(mbox) = NULL; }}while(0) 

struct pbuf * sys_arch_allocate_pbuf();
void sys_arch_free_pbuf(struct pbuf * pbuf);
void sys_arch_free_pbuf_ref(struct pbuf * pbuf);

#endif /* __ARCH_SYS_ARCH_H__ */

