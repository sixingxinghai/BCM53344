
/* Copyright (C) 2004  All Rights Reserved. */

#ifndef _HSL_OS_H_
#define _HSL_OS_H_
#include <linux/kconfig.h>
#include <generated/autoconf.h>
#include <linux/semaphore.h>
#include <linux/prefetch.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/proc_fs.h>
#include <linux/netdevice.h>
#ifndef LINUX_KERNEL_2_6 /* not present in kernel 2_6 */
#include <linux/brlock.h>
#endif
#include <linux/ip.h>
#include <net/ip.h>
#include <net/ip_fib.h>
#include <linux/tcp.h>
#include <linux/udp.h>
#include <linux/icmp.h>
#include <linux/in.h>
#include <net/ip_fib.h>
#include <asm/io.h>
#include <asm/uaccess.h>
#include <linux/inetdevice.h>
#include <linux/etherdevice.h>
#include <linux/igmp.h>
#include <linux/mroute.h>
#include <net/icmp.h>
#include <net/protocol.h>
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,21)
#include <net/net_namespace.h>
#include <linux/nsproxy.h>
#endif /* LINUX_VERSION_CODE KERNEL_VERSION */
#ifdef HAVE_IPV6
#include <net/addrconf.h>
#ifndef HAVE_IPNET
#include <net/ip6_route.h>
#endif
#endif /* HAVE_IPV6 */
#include <linux/ctype.h>

#define HSL_ARP_AGEING_THREAD_NAME    "zARPAge"
#define HSL_ARP_AGEING_SEM_NAME       "zARPAge_sem"
#define HSL_ARP_AGEING_THREAD_PRIO    (250)
#define HSL_ARP_AGEING_STACK          8192

#define HSL_IFSTAT_THREAD_NAME        "zIfStat" 
#define HSL_IFSTAT_SEM_NAME           "zIfStat_sem" 
#define HSL_IFSTAT_TIMER_RESOLUTION   1
#define HSL_IFSTAT_THREAD_PRIO        250
#define HSL_IFSTAT_STACK              16384 

#define HSL_IFSTAT_EFM_THREAD_NAME              "zIfEfmStat"
#define HSL_IFSTAT_EFM_SEM_NAME                 "zIfEfmStat_sem"
#define HSL_IFSTAT_EFM_TIMER_RESOLUTION         1
#define HSL_IFSTAT_EFM_TIMER_RESOLUTION_IN_MS   100
#define HSL_IFSTAT_EFM_THREAD_PRIO              250
#define HSL_IFSTAT_EFM_STACK                    16384

/*djg for ptp*/
//extern char PDELAY_REQ_INTVL;
#define HSL_PTP_SEND_THREAD_NAME        "zPTPSend" 
#define HSL_PTP_SEND_SEM_NAME           "zPTPSend_sem" 
#define HSL_PTP_SEND_TIMER_RESOLUTION   1
#define HSL_PTP_SEND_THREAD_PRIO        150//250
#define HSL_PTP_SEND_STACK              16384

#define HSL_WRITE_PROC_LEN            8

struct sal_thread_s;
struct hsl_periodic_task 
{
   char       task_name[100];
   int          task_timeout;
   struct timer_list timer_id;
   int         task_priority; 
   int       task_stack_size;
   char        sem_name[100];
   void              *sem_id;
   struct sal_thread_s *task_id;  
   void (* foo)(void);
};

typedef char mac_addr_t[6];

/* Deal with CONFIG_MODVERSIONS */
#if defined (CONFIG_MODVERSIONS)
#define MODVERSIONS
/* #include <linux/modversions.h> */
#endif

#ifdef CONFIG_KERNEL_ASSERTS
/* kgdb stuff */
#define HSL_ASSERT(p) KERNEL_ASSERT(#p, p)
#else
#define HSL_ASSERT(p) do {  \
        if (!(p)) {     \
                printk(KERN_CRIT "BUG at %s:%d assert(%s)\n",   \
                       __FILE__, __LINE__, #p);                 \
                BUG();  \
        }               \
} while (0)
#endif /* CONFIG_KERNEL_ASSERTS */

int hsl_atoi (char *str);


int ptp_sock_sendmsg_event (char *buf, int len, int port);
int ptp_sock_sendmsg_general (char *buf, int len, int port);
int ptp_sock_peer_sendmsg_event_pdelay ( char *buf, int len, int port);
int ptp_sock_peer_sendmsg_general_pdelay ( char *buf, int len, int port);

void ptp_send_timeout_set( int intvl);

#endif /* _HSL_OS_H_ */
