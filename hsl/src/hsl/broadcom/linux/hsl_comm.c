/* Copyright 2003  All Rights Reserved.  */

#include "config.h"
#include "hsl_os.h"

#include "hsl_types.h"

/* Broadcom includes. */
#include "bcm_incl.h"

/* HAL includes. */
#include "hal_netlink.h"
#include "hal_msg.h"
#include "hal_car.h"
#include "hal_ptp.h"

/* HSL includes.*/
#include "hsl_logger.h"
#include "hsl_ether.h"
#include "hsl.h"
#include "hsl_ifmgr.h"
#include "hsl_if_hw.h"
#include "hsl_if_os.h"
#include "hsl_bcm_pkt.h"
#include "hsl_comm.h"
#include "hsl_msg.h"

/* Forward declarations. */
int hsl_sock_if_event (int cmd, void *param1, void *param2);

/* List of all HSL backend sockets. */
static struct hsl_sock *hsl_socklist = 0;
static rwlock_t hsl_socklist_lock = RW_LOCK_UNLOCKED;
static struct hsl_if_notifier_chain hsl_comm_cb =
  {
    notifier: hsl_sock_if_event,
  };

/* Forward declarations for static calls only. */
static void _hsl_sock_destruct (struct sock *sk);
#if (LINUX_VERSION_CODE >= KERNEL_VERSION (2,6,27))
static int _hsl_sock_create (struct net *net, struct socket *sock, int protocol);
#else
static int _hsl_sock_create (struct socket *sock, int protocol);
#endif
#ifdef LINUX_KERNEL_2_6
static int _hsl_sock_sendmsg (struct kiocb *iocb, struct socket *sock, struct msghdr *msg, size_t len);
static int _hsl_sock_recvmsg (struct kiocb *iocb, struct socket *sock, struct msghdr *msg, size_t len, int flags);
#else
static int _hsl_sock_sendmsg (struct socket *sock, struct msghdr *msg, int len, struct scm_cookie *scm);
static int _hsl_sock_recvmsg (struct socket *sock, struct msghdr *msg, int len, int flags, struct scm_cookie *scm);
#endif

static int _hsl_sock_bind (struct socket *sock, struct sockaddr *sockaddr, int sockaddr_len);
static int _hsl_sock_getname (struct socket *sock, struct sockaddr *saddr, int *len, int peer);

/* Private packet socket structures. */

#include <linux/autoconf.h>

static struct proto_ops SOCKOPS_WRAPPED (hsl_ops) = {
  family:       AF_HSL,

  release:      hsl_sock_release,
  bind:         _hsl_sock_bind,
  connect:      sock_no_connect,
  socketpair:   sock_no_socketpair,
  accept:       sock_no_accept,
  getname:      _hsl_sock_getname,
  poll:         datagram_poll,
  ioctl:        sock_no_ioctl,
  listen:       sock_no_listen,
  shutdown:     sock_no_shutdown,
  setsockopt:   sock_no_setsockopt,
  getsockopt:   sock_no_getsockopt,
  sendmsg:      _hsl_sock_sendmsg,
  recvmsg:      _hsl_sock_recvmsg,
  mmap:         sock_no_mmap,
  sendpage:     sock_no_sendpage,
};

static struct net_proto_family hsl_family_ops = {
  family:               AF_HSL,
  create:               _hsl_sock_create,
};

/* Destruct socket. */
static void 
_hsl_sock_destruct (struct sock *sk)
{
  struct hsl_sock *hsk, *phsk;

  if (!sk)
    return;

  /* Write lock. */
  write_lock_bh (&hsl_socklist_lock);

  phsk = NULL;
  for (hsk = hsl_socklist; hsk; hsk = hsk->next) 
    {
      if (hsk->sk == sk) 
        {
          if (phsk)
            phsk->next = hsk->next;
          else
            hsl_socklist = hsk->next;
  
          /* Free hsk. */
          kfree (hsk);
          break;
        }

      phsk = hsk;
    }

  /* Write unlock. */
  write_unlock_bh (&hsl_socklist_lock);

  /* Now the socket is dead. No more input will appear.*/
  sock_orphan (sk);

  /* Purge queues */
#ifdef LINUX_KERNEL_2_6
  skb_queue_purge (&sk->sk_receive_queue);
#else
  skb_queue_purge (&sk->receive_queue);
#endif
  sock_put (sk);
}

/* Release socket. */
int 
hsl_sock_release (struct socket *sock)
{
  struct sock *sk = sock->sk;

  /* Destruct socket. */
  _hsl_sock_destruct (sk);

  sock->sk = NULL;

  return 0;
}

#ifdef LINUX_KERNEL_2_6
static struct proto _prot = {
        .name     = "HSL_COMM",
        .owner    = THIS_MODULE,
        .obj_size = sizeof(struct sock),
};
#endif

/* Create socket. */
#if (LINUX_VERSION_CODE >= KERNEL_VERSION (2,6,27))
static int
_hsl_sock_create (struct net *net, struct socket *sock, int protocol)
#else /* ! 2.6.27*/
static int 
_hsl_sock_create (struct socket *sock, int protocol)
#endif
{
  struct sock *sk = NULL;
  struct hsl_sock *hsk = NULL;

  HSL_FN_ENTER ();

  sock->state = SS_UNCONNECTED;

#ifdef LINUX_KERNEL_2_6
#if LINUX_VERSION_CODE >= KERNEL_VERSION (2,6,27)
  sk = sk_alloc (current->nsproxy->net_ns, AF_HSL, GFP_KERNEL, &_prot);
#else
  sk = sk_alloc (AF_HSL, GFP_KERNEL, &_prot, 1);
#endif
#else
  sk = sk_alloc (AF_HSL, GFP_KERNEL, 1);
#endif
  if (sk == NULL)
    {
      HSL_FN_EXIT(-ENOBUFS);
    }

  sock->ops = &hsl_ops;
  sock_init_data (sock,sk);

  sock_hold (sk);

  /* Write lock. */
  write_lock_bh (&hsl_socklist_lock);

  hsk = kmalloc (sizeof (struct hsl_sock), GFP_KERNEL);
  if (! hsk)
    {
      write_unlock_bh (&hsl_socklist_lock);
      goto ERR;
    }
  hsk->next = hsl_socklist;
  hsl_socklist = hsk;

  /* Set sk. */
  hsk->sk = sk;

  /* Reset multicast group and PID. */
  hsk->groups = 0;
  hsk->pid = 0;

  /* Write unlock. */
  write_unlock_bh (&hsl_socklist_lock);

  HSL_FN_EXIT(0);
 ERR:
  if (sk)
    sk_free (sk);
  HSL_FN_EXIT(-ENOMEM);
}

/*
  HSL socket getname.
*/
static int
_hsl_sock_getname (struct socket *sock, struct sockaddr *saddr,
                   int *len, int peer)
{
  struct hsl_sock *hsk;
  struct hal_sockaddr_nl *snl = (struct hal_sockaddr_nl *) saddr;
  struct sock *sk;
  
  sk = sock->sk;
  if (! sk)
    return -EINVAL;

  /* Read lock. */
  read_lock_bh (&hsl_socklist_lock);

  for (hsk = hsl_socklist; hsk; hsk = hsk->next)
    {
      if (hsk->sk == sk)
        {
          /* Set multicast group. */
          snl->nl_pid    = hsk->pid;
          snl->nl_groups = hsk->groups;
          break;
        }
    }

  /* Read unlock. */
  read_unlock_bh (&hsl_socklist_lock);  

  if (len)
    *len = sizeof (struct hal_sockaddr_nl);

  return 0;
}

/* 
   HSL process message from client. 
*/
int
hsl_sock_process_msg (struct socket *sock, char *buf, int buflen)
{
  struct hal_nlmsghdr *hdr;
  char *msgbuf;
   int i = 0;
  
  hdr = (struct hal_nlmsghdr *)buf;
  msgbuf = buf + sizeof (struct hal_nlmsghdr);

  HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO, 
	"\n\rsock_sendmsg got a msg: %d.........len = %d\n" , hdr->nlmsg_type, hdr->nlmsg_len);
     #if 0
     HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO, "    ");
      for(i = 0;  i < (hdr->nlmsg_len - sizeof(struct hal_nlmsghdr)); i++)
      {
               if(*((char *)msgbuf +i) < 0x10)
               {
                      HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO, "0");
               	}
			   
	       HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO, "%X", *((char *)msgbuf +i));

		if(((i % 16) == 0) && ((i / 16) >= 1))
		{
		      HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO, "\n\r");
		      HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO, "    ");
		}
      	}

  HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO, "\n\r");	  
  #endif

  switch (hdr->nlmsg_type)
    {
      /* HAL initialization. */
    case HAL_MSG_INIT:
      return hsl_msg_recv_init (sock, hdr, msgbuf);
    case HAL_MSG_DEINIT:
      return hsl_msg_recv_deinit (sock, hdr, msgbuf);

      /* Interface manager messages. */
    case HAL_MSG_IF_GETLINK:
      return hsl_msg_recv_if_getlink (sock, hdr, msgbuf);
    case HAL_MSG_IF_GET_METRIC:
      return hsl_msg_recv_if_get_metric (sock, hdr, msgbuf);
    case HAL_MSG_IF_GET_MTU:
      return hsl_msg_recv_if_get_mtu (sock, hdr, msgbuf);
    case HAL_MSG_IF_SET_MTU:
      return hsl_msg_recv_if_set_mtu (sock, hdr, msgbuf);
    case HAL_MSG_IF_CLEANUP_DONE: 
      return hsl_msg_recv_if_delete_done(sock, hdr, msgbuf);
#ifdef HAVE_L3
    case HAL_MSG_IF_GET_ARP_AGEING_TIMEOUT:
      return hsl_msg_recv_if_get_arp_ageing_timeout (sock, hdr, msgbuf);
    case HAL_MSG_IF_SET_ARP_AGEING_TIMEOUT:
      return hsl_msg_recv_if_set_arp_ageing_timeout (sock, hdr, msgbuf);
#endif /* HAVE_L3 */
    case HAL_MSG_IF_GET_DUPLEX:
      return hsl_msg_recv_if_get_duplex (sock, hdr, msgbuf);
    case HAL_MSG_IF_SET_DUPLEX:
      return hsl_msg_recv_if_set_duplex (sock, hdr, msgbuf);
    case HAL_MSG_IF_SET_AUTONEGO:
      return hsl_msg_recv_if_set_autonego (sock, hdr, msgbuf);
    case HAL_MSG_IF_GET_HWADDR:
      return hsl_msg_recv_if_get_hwaddr (sock, hdr, msgbuf);
    case HAL_MSG_IF_SET_HWADDR:
      return hsl_msg_recv_if_set_hwaddr (sock, hdr, msgbuf);
#ifdef HAVE_L3
    case HAL_MSG_IF_SET_SEC_HWADDRS:
      return hsl_msg_recv_if_set_sec_hwaddrs (sock, hdr, msgbuf);
    case HAL_MSG_IF_ADD_SEC_HWADDRS:
      return hsl_msg_recv_if_add_sec_hwaddrs (sock, hdr, msgbuf);
    case HAL_MSG_IF_DELETE_SEC_HWADDRS:
      return hsl_msg_recv_if_delete_sec_hwaddrs (sock, hdr, msgbuf);
#endif /* HAVE_L3 */
    case HAL_MSG_IF_GET_FLAGS:
      return hsl_msg_recv_if_flags_get (sock, hdr, msgbuf);
    case HAL_MSG_IF_SET_FLAGS:
      return hsl_msg_recv_if_flags_set (sock, hdr, msgbuf);
    case HAL_MSG_IF_UNSET_FLAGS:
      return hsl_msg_recv_if_flags_unset (sock, hdr, msgbuf);
    case HAL_MSG_IF_GET_BW:
      return hsl_msg_recv_if_get_bw (sock, hdr, msgbuf);
    case HAL_MSG_IF_SET_BW:
      return hsl_msg_recv_if_set_bw (sock, hdr, msgbuf);
    case HAL_MSG_IF_COUNTERS_GET:
      return hsl_msg_recv_if_get_counters (sock, hdr, msgbuf);
    case HAL_MSG_IF_CLEAR_COUNTERS:
       return hsl_msg_recv_if_clear_counters (sock, hdr, msgbuf);
#ifdef HAVE_L3
    case HAL_MSG_IF_L3_INIT:
      return hsl_msg_recv_if_init_l3 (sock, hdr, msgbuf);
    case HAL_MSG_IF_L3_DEINIT:
      return hsl_msg_recv_if_deinit_l3 (sock, hdr, msgbuf);
    case HAL_MSG_IF_L3_STATUS:
      return hsl_msg_recv_if_l3_forwarding(sock, hdr, msgbuf);
#ifdef HAVE_IPV6
    case HAL_MSG_IF_IPV6_L3_STATUS:
      return hsl_msg_recv_if_ipv6_l3_forwarding(sock, hdr, msgbuf);
#endif
#ifdef HAVE_VRRP
    case HAL_MSG_VRRP_UPDATE:
      return hsl_msg_recv_vrrp_update (sock, hdr, msgbuf);
#endif /* HAVE_VRRP */
#endif /* HAVE_L3 */
#if defined (HAVE_L2) && defined (HAVE_L3)
    case HAL_MSG_IF_CREATE_SVI:
      return hsl_msg_recv_if_create_svi (sock, hdr, msgbuf);
    case HAL_MSG_IF_DELETE_SVI:
      return hsl_msg_recv_if_delete_svi (sock, hdr, msgbuf);
#endif /* HAVE_L2 && HAVE_L3 */

#ifdef HAVE_L3   
      /* L3 interface manager messages. */
    case HAL_MSG_IF_SET_PORT_TYPE:
      return hsl_msg_recv_if_set_port_type (sock, hdr, msgbuf);
    case HAL_MSG_IF_IPV4_NEWADDR:
      return hsl_msg_recv_if_ipv4_newaddr (sock, hdr, msgbuf);
    case HAL_MSG_IF_IPV4_DELADDR:
      return hsl_msg_recv_if_ipv4_deladdr (sock, hdr, msgbuf);
#ifdef HAVE_IPV6
    case HAL_MSG_IF_IPV6_ADDRESS_ADD:
      return hsl_msg_recv_if_ipv6_newaddr (sock, hdr, msgbuf);
    case HAL_MSG_IF_IPV6_ADDRESS_DELETE:
      return hsl_msg_recv_if_ipv6_deladdr (sock, hdr, msgbuf);
#endif /* HAVE_IPV6 */

      /* L3 If FIB bind/unbind messages */
    case HAL_MSG_IF_FIB_BIND:
      return hsl_msg_recv_if_bind_fib (sock, hdr, msgbuf);
    case HAL_MSG_IF_FIB_UNBIND:
      return hsl_msg_recv_if_unbind_fib (sock, hdr, msgbuf);

      /* L3 route table management messages. */
    case HAL_MSG_IPV4_INIT:
      return hsl_msg_recv_ipv4_init (sock, hdr, msgbuf);
    case HAL_MSG_IPV4_DEINIT:
      return hsl_msg_recv_ipv4_deinit (sock, hdr, msgbuf);
    case HAL_MSG_FIB_CREATE:
      return hsl_msg_recv_fib_create (sock, hdr, msgbuf);
    case HAL_MSG_FIB_DELETE:
      return hsl_msg_recv_fib_delete (sock, hdr, msgbuf);
    case HAL_MSG_IPV4_UC_ADD:
      return hsl_msg_recv_ipv4_uc_add (sock, hdr, msgbuf);
    case HAL_MSG_IPV4_UC_DELETE:
      return hsl_msg_recv_ipv4_uc_delete (sock, hdr, msgbuf);
    case HAL_MSG_IPV4_UC_UPDATE:
      return hsl_msg_recv_ipv4_uc_update (sock, hdr, msgbuf);
    case HAL_MSG_GET_MAX_MULTIPATH:
      return hsl_msg_recv_get_max_multipath (sock, hdr, msgbuf);


#ifdef HAVE_MCAST_IPV4
    case HAL_MSG_IPV4_MC_INIT:
      return hsl_msg_recv_ipv4_mc_init(sock, hdr, msgbuf);
    case HAL_MSG_IPV4_MC_DEINIT:
      return hsl_msg_recv_ipv4_mc_deinit(sock, hdr, msgbuf);
    case HAL_MSG_IPV4_MC_PIM_INIT:
      return hsl_msg_recv_ipv4_mc_pim_init(sock, hdr, msgbuf);
    case HAL_MSG_IPV4_MC_PIM_DEINIT:
      return hsl_msg_recv_ipv4_mc_pim_deinit(sock, hdr, msgbuf);
    case HAL_MSG_IPV4_MC_VIF_ADD:
      return hsl_msg_recv_ipv4_mc_vif_add(sock, hdr, msgbuf);
    case HAL_MSG_IPV4_MC_VIF_DEL:
      return hsl_msg_recv_ipv4_mc_vif_del(sock, hdr, msgbuf);
    case HAL_MSG_IPV4_MC_MRT_ADD:
      return hsl_msg_recv_ipv4_mc_route_add(sock, hdr, msgbuf);
    case HAL_MSG_IPV4_MC_MRT_DEL:
      return hsl_msg_recv_ipv4_mc_route_del(sock, hdr, msgbuf);
    case HAL_MSG_IPV4_MC_SG_STAT:
      return hsl_msg_recv_ipv4_mc_stat_get(sock, hdr, msgbuf);
#endif /* HAVE_MCAST_IPV4 */

#ifdef HAVE_MCAST_IPV6
    case HAL_MSG_IPV6_MC_INIT:
      return hsl_msg_recv_ipv6_mc_init(sock, hdr, msgbuf);
    case HAL_MSG_IPV6_MC_DEINIT:
      return hsl_msg_recv_ipv6_mc_deinit(sock, hdr, msgbuf);
    case HAL_MSG_IPV6_MC_PIM_INIT:
      return hsl_msg_recv_ipv6_mc_pim_init(sock, hdr, msgbuf);
    case HAL_MSG_IPV6_MC_PIM_DEINIT:
      return hsl_msg_recv_ipv6_mc_pim_deinit(sock, hdr, msgbuf);
    case HAL_MSG_IPV6_MC_VIF_ADD:
      return hsl_msg_recv_ipv6_mc_vif_add(sock, hdr, msgbuf);
    case HAL_MSG_IPV6_MC_VIF_DEL:
      return hsl_msg_recv_ipv6_mc_vif_del(sock, hdr, msgbuf);
    case HAL_MSG_IPV6_MC_MRT_ADD:
      return hsl_msg_recv_ipv6_mc_route_add(sock, hdr, msgbuf);
    case HAL_MSG_IPV6_MC_MRT_DEL:
      return hsl_msg_recv_ipv6_mc_route_del(sock, hdr, msgbuf);
    case HAL_MSG_IPV6_MC_SG_STAT:
      return hsl_msg_recv_ipv6_mc_stat_get(sock, hdr, msgbuf);
#endif /* HAVE_MCAST_IPV6 */

#ifdef HAVE_IPV6
    case HAL_MSG_IPV6_INIT:
      return hsl_msg_recv_ipv6_init (sock, hdr, msgbuf);
    case HAL_MSG_IPV6_DEINIT:
      return hsl_msg_recv_ipv6_deinit (sock, hdr, msgbuf);
    case HAL_MSG_IPV6_UC_INIT:
      return hsl_msg_recv_ipv6_uc_init (sock, hdr, msgbuf);
    case HAL_MSG_IPV6_UC_DEINIT:
      return hsl_msg_recv_ipv6_uc_deinit (sock, hdr, msgbuf);
    case HAL_MSG_IPV6_UC_ADD:
      return hsl_msg_recv_ipv6_uc_add (sock, hdr, msgbuf);
    case HAL_MSG_IPV6_UC_DELETE:
      return hsl_msg_recv_ipv6_uc_delete (sock, hdr, msgbuf);
    case HAL_MSG_IPV6_UC_UPDATE:
      return hsl_msg_recv_ipv6_uc_update (sock, hdr, msgbuf);
#endif /* HAVE_IPV6 */
#endif /* HAVE_L3 */
#ifdef HAVE_L2LERN
    case HAL_MSG_MAC_SET_ACCESS_GROUP:
      return hsl_msg_recv_mac_access_grp_set (sock, hdr, msgbuf);
#ifdef HAVE_VLAN
    case HAL_MSG_VLAN_SET_ACCESS_MAP:
      return hsl_msg_recv_vlan_access_map_set (sock, hdr, msgbuf);
#endif /* HAVE_VLAN */
#endif /* HAVE_L2LERN */

#ifdef HAVE_L2
    case HAL_MSG_IF_L2_INIT:
      return hsl_msg_recv_if_init_l2 (sock, hdr, msgbuf);
      /* Bridge messages. */
    case HAL_MSG_BRIDGE_INIT:
      return hsl_msg_recv_bridge_init (sock, hdr, msgbuf);
    case HAL_MSG_BRIDGE_DEINIT:
      return hsl_msg_recv_bridge_deinit (sock, hdr, msgbuf);
    case HAL_MSG_BRIDGE_ADD:
      return hsl_msg_recv_bridge_add (sock, hdr, msgbuf);
    case HAL_MSG_BRIDGE_DELETE:
      return hsl_msg_recv_bridge_delete (sock, hdr, msgbuf);
    case HAL_MSG_BRIDGE_SET_AGEING_TIME:
      return hsl_msg_recv_set_ageing_time (sock, hdr, msgbuf);
    case HAL_MSG_BRIDGE_SET_LEARNING:
      return hsl_msg_recv_set_learning (sock, hdr, msgbuf);
    case HAL_MSG_BRIDGE_ADD_PORT:
      return hsl_msg_recv_bridge_add_port (sock, hdr, msgbuf);
    case HAL_MSG_BRIDGE_DELETE_PORT:
      return hsl_msg_recv_bridge_delete_port (sock, hdr, msgbuf);
    case HAL_MSG_BRIDGE_ADD_INSTANCE:
      return hsl_msg_recv_bridge_add_instance (sock, hdr, msgbuf);
    case HAL_MSG_BRIDGE_DELETE_INSTANCE:
      return hsl_msg_recv_bridge_delete_instance (sock, hdr, msgbuf);
    case HAL_MSG_BRIDGE_ADD_VLAN_TO_INSTANCE:
      return hsl_msg_recv_bridge_add_vlan_to_instance (sock, hdr, msgbuf);
    case HAL_MSG_BRIDGE_DELETE_VLAN_FROM_INSTANCE:
      return hsl_msg_recv_bridge_delete_vlan_from_instance (sock, hdr, msgbuf);
    case HAL_MSG_BRIDGE_SET_PORT_STATE:
      return hsl_msg_recv_set_port_state (sock, hdr, msgbuf);
    case HAL_MSG_BRIDGE_CHANGE_VLAN_TYPE:
      return hsl_msg_recv_bridge_change_vlan_type (sock, hdr, msgbuf);

#ifdef HAVE_VLAN
      /* VLAN messages. */
    case HAL_MSG_VLAN_INIT:
      return hsl_msg_recv_vlan_init (sock, hdr, msgbuf);
    case HAL_MSG_VLAN_DEINIT:
      return hsl_msg_recv_vlan_deinit (sock, hdr, msgbuf);
    case HAL_MSG_VLAN_ADD:
      return hsl_msg_recv_vlan_add (sock, hdr, msgbuf);
    case HAL_MSG_VLAN_DELETE:
      return hsl_msg_recv_vlan_delete (sock, hdr, msgbuf);
    case HAL_MSG_VLAN_SET_PORT_TYPE:
      return hsl_msg_recv_vlan_set_port_type (sock, hdr, msgbuf);
    case HAL_MSG_VLAN_SET_DEFAULT_PVID:
      return hsl_msg_recv_vlan_set_default_pvid (sock, hdr, msgbuf);
    case HAL_MSG_VLAN_ADD_VID_TO_PORT:
      return hsl_msg_recv_vlan_add_vid_to_port (sock, hdr, msgbuf);
    case HAL_MSG_VLAN_DELETE_VID_FROM_PORT:
      return hsl_msg_recv_vlan_delete_vid_from_port (sock, hdr, msgbuf);
#endif /* HAVE_VLAN */

#ifdef HAVE_VLAN_CLASS
    case HAL_MSG_VLAN_CLASSIFIER_ADD:
      return hsl_msg_recv_vlan_classifier_add (sock, hdr, msgbuf);
    case HAL_MSG_VLAN_CLASSIFIER_DELETE:
      return hsl_msg_recv_vlan_classifier_delete (sock, hdr, msgbuf);
#endif /* HAVE_VLAN_CLASS */

#ifdef HAVE_VLAN_STACK
    case HAL_MSG_VLAN_STACKING_ENABLE:
      return hsl_msg_recv_vlan_stacking_enable (sock, hdr, msgbuf);
    case HAL_MSG_VLAN_STACKING_DISABLE:
      return hsl_msg_recv_vlan_stacking_disable (sock, hdr, msgbuf);
#endif /* HAVE_VLAN_STACK */
#ifdef HAVE_PROVIDER_BRIDGE
    case HAL_MSG_VLAN_ADD_VLAN_REG_TAB_ENT:
      return hsl_msg_recv_cvlan_reg_tab_add (sock, hdr, msgbuf);
    case HAL_MSG_VLAN_DEL_VLAN_REG_TAB_ENT:
      return hsl_msg_recv_cvlan_reg_tab_del (sock, hdr, msgbuf);
    case HAL_MSG_BRIDGE_PROTO_PROCESS:
      return hsl_msg_recv_protocol_process (sock, hdr, msgbuf);
#endif /* HAVE_PROVIDER_BRIDGE */
  
   /* DCB messages */
#ifdef HAVE_DCB
    case HAL_MSG_DCB_INIT:
      return hsl_msg_recv_dcb_init (sock, hdr, msgbuf);
    case HAL_MSG_DCB_DEINIT:
      return hsl_msg_recv_dcb_deinit (sock, hdr, msgbuf);
    case HAL_MSG_DCB_ENABLE:
      return hsl_msg_recv_dcb_enable (sock, hdr, msgbuf);
    case HAL_MSG_DCB_DISABLE:
      return hsl_msg_recv_dcb_disable (sock, hdr, msgbuf);
    case HAL_MSG_DCB_ETS_ENABLE:
      return hsl_msg_recv_dcb_ets_enable (sock, hdr, msgbuf);
    case HAL_MSG_DCB_ETS_DISABLE:
      return hsl_msg_recv_dcb_ets_disable (sock, hdr, msgbuf);
    case HAL_MSG_DCB_IF_ENABLE:
      return hsl_msg_recv_dcb_if_enable (sock, hdr, msgbuf);
    case HAL_MSG_DCB_IF_DISABLE:
      return hsl_msg_recv_dcb_if_disable (sock, hdr, msgbuf);
    case HAL_MSG_DCB_ETS_IF_ENABLE:
      return hsl_msg_recv_dcb_ets_if_enable (sock, hdr, msgbuf);
    case HAL_MSG_DCB_ETS_IF_DISABLE:
      return hsl_msg_recv_dcb_ets_if_disable (sock, hdr, msgbuf);
    case HAL_MSG_DCB_ETS_SELECT_MODE:
      return hsl_msg_recv_dcb_ets_select_mode (sock, hdr, msgbuf);
    case HAL_MSG_DCB_ETS_ADD_PRI_TO_TCG:
      return hsl_msg_recv_dcb_ets_add_pri_to_tcg (sock, hdr, msgbuf);
    case HAL_MSG_DCB_ETS_REMOVE_PRI_FROM_TCG:
      return hsl_msg_recv_dcb_ets_remove_pri_from_tcg (sock, hdr, msgbuf);
    case HAL_MSG_DCB_ETS_TCG_BW_SET:
      return hsl_msg_recv_dcb_ets_tcg_bw_set (sock, hdr, msgbuf);
    case HAL_MSG_DCB_ETS_APP_PRI_SET:
      return hsl_msg_recv_dcb_ets_app_pri_set (sock, hdr, msgbuf);
    case HAL_MSG_DCB_ETS_APP_PRI_UNSET:
      return hsl_msg_recv_dcb_ets_app_pri_unset (sock, hdr, msgbuf);
    case HAL_MSG_DCB_PFC_ENABLE:
      return hsl_msg_recv_dcb_pfc_enable (sock, hdr, msgbuf);
    case HAL_MSG_DCB_PFC_DISABLE:
      return hsl_msg_recv_dcb_pfc_disable (sock, hdr, msgbuf);
    case HAL_MSG_DCB_PFC_IF_ENABLE:
      return hsl_msg_recv_dcb_pfc_if_enable (sock, hdr, msgbuf);
    case HAL_MSG_DCB_PFC_SELECT_MODE:
      return hsl_msg_recv_dcb_pfc_select_mode (sock, hdr, msgbuf);
    case HAL_MSG_DCB_PFC_SET_CAP:
      return hsl_msg_recv_dcb_pfc_set_cap (sock, hdr, msgbuf);
    case HAL_MSG_DCB_PFC_SET_LDA:
      return hsl_msg_recv_dcb_pfc_set_lda (sock, hdr, msgbuf);

#endif /* HAVE_DCB */

      /* Flow control messages. */
    case HAL_MSG_FLOW_CONTROL_INIT:
      return hsl_msg_recv_flow_control_init (sock, hdr, msgbuf);
    case HAL_MSG_FLOW_CONTROL_DEINIT:
      return hsl_msg_recv_flow_control_deinit (sock, hdr, msgbuf);
    case HAL_MSG_FLOW_CONTROL_SET:
      return hsl_msg_recv_flow_control_set (sock, hdr, msgbuf);
    case HAL_MSG_FLOW_CONTROL_STATISTICS:
      return hsl_msg_recv_flow_control_statistics (sock, hdr, msgbuf);

      /* L2 QoS messages. */
    case HAL_MSG_L2_QOS_INIT:
      return hsl_msg_recv_l2_qos_init (sock, hdr, msgbuf);
    case HAL_MSG_L2_QOS_DEINIT:
      return hsl_msg_recv_l2_qos_deinit (sock, hdr, msgbuf);
    case HAL_MSG_L2_QOS_DEFAULT_USER_PRIORITY_SET:
      return hsl_msg_recv_l2_qos_default_user_priority_set (sock, hdr, msgbuf);
    case HAL_MSG_L2_QOS_DEFAULT_USER_PRIORITY_GET:
      return hsl_msg_recv_l2_qos_default_user_priority_set (sock, hdr, msgbuf);
    case HAL_MSG_L2_QOS_REGEN_USER_PRIORITY_SET:
      return hsl_msg_recv_l2_qos_regen_user_priority_set (sock, hdr, msgbuf);
    case HAL_MSG_L2_QOS_REGEN_USER_PRIORITY_GET:
      return hsl_msg_recv_l2_qos_regen_user_priority_get (sock, hdr, msgbuf);
    case HAL_MSG_L2_QOS_TRAFFIC_CLASS_SET:
      return hsl_msg_recv_l2_qos_traffic_class_set (sock, hdr, msgbuf);
    case HAL_MSG_L2_QOS_NUM_COSQ_GET:
      return hsl_msg_recv_l2_qos_num_cosq_get (sock, hdr, msgbuf);
    case HAL_MSG_L2_QOS_NUM_COSQ_SET:
      return hsl_msg_recv_l2_qos_num_cosq_set (sock, hdr, msgbuf);

      /* Ratelimiting messages. */
    case HAL_MSG_RATELIMIT_INIT:
      return hsl_msg_recv_ratelimit_init(sock, hdr, msgbuf);
    case HAL_MSG_RATELIMIT_DEINIT:
      return hsl_msg_recv_ratelimit_deinit(sock, hdr, msgbuf);
    case HAL_MSG_RATELIMIT_BCAST:
      return hsl_msg_recv_ratelimit_bcast (sock, hdr, msgbuf);
    case HAL_MSG_RATELIMIT_BCAST_DISCARDS_GET:
      return hsl_msg_recv_bcast_discards_get (sock, hdr, msgbuf);
    case HAL_MSG_RATELIMIT_MCAST:
      return hsl_msg_recv_ratelimit_mcast (sock, hdr, msgbuf);
    case HAL_MSG_RATELIMIT_MCAST_DISCARDS_GET:
      return hsl_msg_recv_mcast_discards_get (sock, hdr, msgbuf);
    case HAL_MSG_RATELIMIT_DLF_BCAST:
      return hsl_msg_recv_ratelimit_dlf_bcast (sock, hdr, msgbuf);
    case HAL_MSG_RATELIMIT_DLF_BCAST_DISCARDS_GET:
      return hsl_msg_recv_dlf_bcast_discards_get (sock, hdr, msgbuf);

#ifdef HAVE_IGMP_SNOOP
      /* IGMP Snooping messages. */
    case HAL_MSG_IGMP_SNOOPING_INIT:
       return hsl_recv_msg_igmp_snooping_init (sock, hdr, msgbuf);
    case HAL_MSG_IGMP_SNOOPING_DEINIT:
      return hsl_recv_msg_igmp_snooping_deinit (sock, hdr, msgbuf);
    case HAL_MSG_IGMP_SNOOPING_ENABLE:
      return hsl_msg_recv_igmp_snooping_enable (sock, hdr, msgbuf);
    case HAL_MSG_IGMP_SNOOPING_DISABLE:
      return hsl_msg_igmp_snooping_disable (sock, hdr, msgbuf);
    case HAL_MSG_IGMP_SNOOPING_ENTRY_ADD: 
      return hsl_msg_igmp_snooping_add_entry(sock, hdr, msgbuf);
    case HAL_MSG_IGMP_SNOOPING_ENTRY_DELETE: 
      return hsl_msg_igmp_snooping_del_entry(sock, hdr, msgbuf);
#endif /* HAVE_IGMP_SNOOP */

#ifdef HAVE_MLD_SNOOP

   /* MLD Snooping messages. */
    case HAL_MSG_MLD_SNOOPING_INIT:
      return hsl_recv_msg_mld_snooping_init (sock, hdr, msgbuf);
    case HAL_MSG_MLD_SNOOPING_DEINIT:
      return hsl_recv_msg_mld_snooping_deinit (sock, hdr, msgbuf);
    case HAL_MSG_MLD_SNOOPING_ENABLE:
      return hsl_msg_recv_mld_snooping_enable (sock, hdr, msgbuf);
    case HAL_MSG_MLD_SNOOPING_DISABLE:
      return hsl_msg_mld_snooping_disable (sock, hdr, msgbuf);
    case HAL_MSG_MLD_SNOOPING_ENTRY_ADD:
      return hsl_msg_mld_snooping_add_entry(sock, hdr, msgbuf);
    case HAL_MSG_MLD_SNOOPING_ENTRY_DELETE:
      return hsl_msg_mld_snooping_del_entry(sock, hdr, msgbuf);              
#endif /* HAVE_MLD_SNOOP */

#if defined HAVE_IGMP_SNOOP || defined HAVE_MLD_SNOOP
    case HAL_MSG_L2_UNKNOWN_MCAST_MODE_DISCARD:
      return hsl_msg_l2_unknown_mcast_discard_mode (sock, hdr, msgbuf);
    case HAL_MSG_L2_UNKNOWN_MCAST_MODE_FLOOD:
      return hsl_msg_l2_unknown_mcast_flood_mode (sock, hdr, msgbuf);                
#endif /* HAVE_IGMP_SNOOP || HAVE_MLD_SNOOP */

      /* L2 FDB messages. */
    case HAL_MSG_L2_FDB_INIT:
      return hsl_msg_recv_l2_fdb_init (sock, hdr, msgbuf);
    case HAL_MSG_L2_FDB_DEINIT:
      return hsl_msg_recv_l2_fdb_deinit (sock, hdr, msgbuf);
    case HAL_MSG_L2_FDB_ADD:
      return hsl_msg_recv_l2_fdb_add (sock, hdr, msgbuf);
    case HAL_MSG_L2_FDB_DELETE:
      return hsl_msg_recv_l2_fdb_delete (sock, hdr, msgbuf);
    case HAL_MSG_L2_FDB_UNICAST_GET:
      return hsl_msg_recv_l2_fdb_unicast_get (sock, hdr, msgbuf);
    case HAL_MSG_L2_FDB_MULTICAST_GET:
      return hsl_msg_recv_l2_fdb_multicast_get (sock, hdr, msgbuf);
    case HAL_MSG_L2_FDB_FLUSH_PORT:
      return hsl_msg_recv_l2_fdb_flush (sock, hdr, msgbuf);
    case HAL_MSG_L2_FLUSH_FDB_BY_MAC:
      return hsl_msg_recv_l2_fdb_flush_by_mac (sock, hdr, msgbuf);
#endif /* HAVE_L2 */
      
#ifdef HAVE_ONMD 
    case HAL_MSG_EFM_SET_FRAME_PERIOD_WINDOW:
      return hsl_msg_recv_set_frame_period_window(sock, hdr, msgbuf);  
    case HAL_MSG_EFM_SET_SYMBOL_PERIOD_WINDOW:
      return hsl_msg_recv_set_symbol_period_window(sock, hdr, msgbuf);
    case HAL_MSG_EFM_FRAME_ERROR_COUNT:
      return hsl_msg_recv_get_frame_error_count(sock, hdr, msgbuf);
    case HAL_MSG_EFM_FRAME_ERROR_SECONDS_COUNT:
      return hsl_msg_recv_get_frame_error_seconds_count(sock, hdr, msgbuf);
    case HAL_MSG_EFM_SET_PORT_STATE:
      return hsl_msg_recv_efm_set_port_state (sock, hdr, msgbuf);
#endif /*HAVE_ONMD*/

      /* Port mirroring messages. */
    case HAL_MSG_PMIRROR_INIT:
      return hsl_msg_recv_pmirror_init (sock, hdr, msgbuf);
    case HAL_MSG_PMIRROR_DEINIT:
      return hsl_msg_recv_pmirror_deinit (sock, hdr, msgbuf);
    case HAL_MSG_PMIRROR_SET:
      return hsl_msg_recv_pmirror_set (sock, hdr, msgbuf);
    case HAL_MSG_PMIRROR_UNSET:
      return hsl_msg_recv_pmirror_unset (sock, hdr, msgbuf);

#ifdef HAVE_QOS
    case HAL_MSG_QOS_INIT:
      return hsl_msg_recv_qos_init (sock, hdr, msgbuf);
    case HAL_MSG_QOS_DEINIT:
      return hsl_msg_recv_qos_deinit (sock, hdr, msgbuf);
    case HAL_MSG_QOS_ENABLE:
      return hsl_msg_recv_qos_enable (sock, hdr, msgbuf);
    case HAL_MSG_QOS_DISABLE:
      return hsl_msg_recv_qos_disable (sock, hdr, msgbuf);
    case HAL_MSG_QOS_WRR_QUEUE_LIMIT:
      return hsl_msg_recv_qos_wrr_queue_limit (sock, hdr, msgbuf);
    case HAL_MSG_QOS_WRR_TAIL_DROP_THRESHOLD:
      return hsl_msg_recv_qos_wrr_tail_drop_threshold (sock, hdr, msgbuf);
    case HAL_MSG_QOS_WRR_THRESHOLD_DSCP_MAP:
      return hsl_msg_recv_qos_wrr_threshold_dscp_map (sock, hdr, msgbuf);
    case HAL_MSG_QOS_WRR_WRED_DROP_THRESHOLD:
      return hsl_msg_recv_qos_wrr_wred_drop_threshold (sock, hdr, msgbuf);
    case HAL_MSG_QOS_WRR_SET_BANDWIDTH:
      return hsl_msg_recv_qos_wrr_set_bandwidth (sock, hdr, msgbuf);
    case HAL_MSG_QOS_WRR_QUEUE_COS_MAP_SET:
      return hsl_msg_recv_qos_wrr_queue_cos_map_set (sock, hdr, msgbuf);
    case HAL_MSG_QOS_WRR_QUEUE_COS_MAP_UNSET:
      return hsl_msg_recv_qos_wrr_queue_cos_map_unset (sock, hdr, msgbuf);
    case HAL_MSG_QOS_WRR_QUEUE_MIN_RESERVE:
      return hsl_msg_recv_qos_wrr_queue_min_reserve (sock, hdr, msgbuf);
    case HAL_MSG_QOS_SET_TRUST_STATE:
      return hsl_msg_recv_qos_set_trust_state (sock, hdr, msgbuf);
    case HAL_MSG_QOS_SET_DEFAULT_COS:
      return hsl_msg_recv_qos_set_default_cos (sock, hdr, msgbuf);
    case HAL_MSG_QOS_SET_DSCP_MAP_TBL:
      return hsl_msg_recv_qos_set_dscp_mapping_tbl (sock, hdr, msgbuf);
    case HAL_MSG_QOS_SET_CLASS_MAP:
      return hsl_msg_recv_qos_set_class_map (sock, hdr, msgbuf);
    case HAL_MSG_QOS_SET_CMAP_COS_INNER:
      return hsl_msg_recv_qos_set_cmap_cos_inner (sock, hdr, msgbuf);
    case HAL_MSG_QOS_SET_POLICY_MAP:
      return hsl_msg_recv_qos_set_policy_map (sock, hdr, msgbuf);
#endif /* HAVE_QOS */
    case HAL_MSG_IP_SET_ACCESS_GROUP:
      return hsl_msg_recv_ip_set_acl_filter (sock, hdr, msgbuf);
    case HAL_MSG_IP_UNSET_ACCESS_GROUP:
      return hsl_msg_recv_ip_unset_acl_filter (sock, hdr, msgbuf);
    case HAL_MSG_IP_SET_ACCESS_GROUP_INTERFACE:
      return hsl_msg_recv_ip_set_acl_filter_interface (sock, hdr, msgbuf);
    case HAL_MSG_IP_UNSET_ACCESS_GROUP_INTERFACE:
      return hsl_msg_recv_ip_unset_acl_filter_interface (sock, hdr, msgbuf);
#ifdef HAVE_LACPD
      /* Link aggregation. */
    case HAL_MSG_LACP_INIT:
      return hsl_msg_recv_lacp_init (sock, hdr, msgbuf);
    case HAL_MSG_LACP_DEINIT:
      return hsl_msg_recv_lacp_deinit (sock, hdr, msgbuf);
    case HAL_MSG_LACP_ADD_AGGREGATOR:
      return hsl_msg_recv_lacp_add_aggregator (sock, hdr, msgbuf);
    case HAL_MSG_LACP_DELETE_AGGREGATOR:
      return hsl_msg_recv_lacp_delete_aggregator (sock, hdr, msgbuf);
    case HAL_MSG_LACP_ATTACH_MUX_TO_AGGREGATOR:
      return hsl_msg_recv_lacp_attach_mux_to_aggregator (sock, hdr, msgbuf);
    case HAL_MSG_LACP_DETACH_MUX_FROM_AGGREGATOR:
      return hsl_msg_recv_lacp_detach_mux_from_aggregator (sock, hdr, msgbuf);
    case HAL_MSG_LACP_PSC_SET:
      return hsl_msg_recv_lacp_psc_set (sock, hdr, msgbuf);
    case HAL_MSG_LACP_COLLECTING:
      return hsl_msg_recv_lacp_collecting (sock, hdr, msgbuf);
    case HAL_MSG_LACP_DISTRIBUTING:
      return hsl_msg_recv_lacp_distributing (sock, hdr, msgbuf);
    case HAL_MSG_LACP_COLLECTING_DISTRIBUTING:
      return hsl_msg_recv_lacp_collecting_distributing (sock, hdr, msgbuf);
#endif /* HAVE_LACPD */

#ifdef HAVE_PVLAN
    case HAL_MSG_PVLAN_SET_VLAN_TYPE:
      return hsl_msg_recv_pvlan_set_vlan_type (sock, hdr, msgbuf);
    case HAL_MSG_PVLAN_VLAN_ASSOCIATE:
      return hsl_msg_recv_pvlan_vlan_associate (sock, hdr, msgbuf);
    case HAL_MSG_PVLAN_VLAN_DISSOCIATE:
      return hsl_msg_recv_pvlan_vlan_dissociate (sock, hdr, msgbuf);
    case HAL_MSG_PVLAN_PORT_ADD:
      return hsl_msg_recv_pvlan_port_add (sock, hdr, msgbuf);
    case HAL_MSG_PVLAN_PORT_DELETE:
      return hsl_msg_recv_pvlan_port_delete (sock, hdr, msgbuf);
    case HAL_MSG_PVLAN_SET_PORT_MODE:
      return hsl_msg_recv_pvlan_set_port_mode (sock, hdr, msgbuf);
#endif /* HAVE_PVLAN */

#ifdef HAVE_AUTHD
    case HAL_MSG_8021x_INIT:
      return hsl_msg_recv_auth_init (sock, hdr, msgbuf);
    case HAL_MSG_8021x_DEINIT:
      return hsl_msg_recv_auth_deinit (sock, hdr, msgbuf);
    case HAL_MSG_8021x_PORT_STATE:
      return hsl_msg_recv_auth_set_port_state (sock, hdr, msgbuf);
#ifdef HAVE_MAC_AUTH
    case HAL_MSG_AUTH_MAC_PORT_STATE:
      return hsl_msg_recv_auth_mac_set_port_state (sock, hdr, msgbuf);
#endif
#endif /* HAVE_AUTHD */


#ifdef HAVE_L3
    case HAL_MSG_ARP_ADD:
      return hsl_msg_recv_arp_add (sock, hdr, msgbuf);
    case HAL_MSG_ARP_DEL:
      return hsl_msg_recv_arp_del (sock, hdr, msgbuf);
    case HAL_MSG_ARP_CACHE_GET:
      return hsl_msg_recv_arp_cache_get(sock, hdr, msgbuf);
    case HAL_MSG_ARP_DEL_ALL:
      return hsl_msg_recv_arp_del_all (sock, hdr, msgbuf);
#ifdef HAVE_IPV6
    case HAL_MSG_IPV6_NBR_ADD:
      return hsl_msg_recv_ipv6_nbr_add (sock, hdr, msgbuf);
    case HAL_MSG_IPV6_NBR_DEL:
      return hsl_msg_recv_ipv6_nbr_del (sock, hdr, msgbuf);
    case HAL_MSG_IPV6_NBR_DEL_ALL:
      return hsl_msg_recv_ipv6_nbr_del_all (sock, hdr, msgbuf);
    case HAL_MSG_IPV6_NBR_CACHE_GET:
      return hsl_msg_recv_ipv6_nbr_cache_get (sock, hdr, msgbuf);
#endif /* HAVE_IPV6 */
#endif /* HAVE_L3 */
#ifdef HAVE_MPLS
    case HAL_MSG_MPLS_INIT:
      return hsl_msg_recv_mpls_init (sock, hdr, msgbuf);
    case HAL_MSG_MPLS_IF_INIT:
      return hsl_msg_recv_mpls_if_init (sock, hdr, msgbuf);
    case HAL_MSG_MPLS_IF_END:
      return hsl_msg_recv_mpls_if_deinit (sock, hdr, msgbuf);
    case HAL_MSG_MPLS_VRF_END:
      return hsl_msg_recv_mpls_vrf_deinit (sock, hdr, msgbuf);
    case HAL_MSG_MPLS_VRF_INIT:
      return hsl_msg_recv_mpls_vrf_init (sock, hdr, msgbuf);
    case HAL_MSG_MPLS_NEWILM:
      return hsl_msg_recv_mpls_ilm_add (sock, hdr, msgbuf);
    case HAL_MSG_MPLS_DELILM:
      return hsl_msg_recv_mpls_ilm_del (sock, hdr, msgbuf);
    case HAL_MSG_MPLS_NEWFTN:
      return hsl_msg_recv_mpls_ftn_add (sock, hdr, msgbuf);
    case HAL_MSG_MPLS_DELFTN:
      return hsl_msg_recv_mpls_ftn_del (sock, hdr, msgbuf);
#ifdef HAVE_MPLS_VC
    case HAL_MSG_MPLS_VC_INIT:
      return hsl_msg_recv_mpls_vc_init (sock, hdr, msgbuf);
    case HAL_MSG_MPLS_VC_END:
      return hsl_msg_recv_mpls_vc_deinit (sock, hdr, msgbuf);
    case HAL_MSG_MPLS_VC_FIB_ADD:
      return hsl_msg_recv_mpls_vc_fib_add (sock, hdr, msgbuf);
    case HAL_MSG_MPLS_VC_FIB_DEL:
      return hsl_msg_recv_mpls_vc_fib_del (sock, hdr, msgbuf);
#endif /* HAVE_MPLS_VC */
#ifdef HAVE_VPLS
    case HAL_MSG_MPLS_VPLS_ADD:
      return hsl_msg_recv_mpls_vpls_add (sock, hdr, msgbuf);
    case HAL_MSG_MPLS_VPLS_DEL:
      return hsl_msg_recv_mpls_vpls_del (sock, hdr, msgbuf);
    case HAL_MSG_MPLS_VPLS_IF_BIND:
      return hsl_msg_recv_mpls_vpls_if_bind (sock, hdr, msgbuf);
    case HAL_MSG_MPLS_VPLS_IF_UNBIND:
      return hsl_msg_recv_mpls_vpls_if_unbind (sock, hdr, msgbuf);
#endif /* HAVE_VPLS */
#endif /* HAVE_MPLS */

    /* CPU related information used for stacking */
    case HAL_MSG_CPU_GET_NUM:
      return hsl_msg_recv_cpu_get_num (sock, hdr, msgbuf);
    case HAL_MSG_CPU_GETDB_INFO:
      return hsl_msg_recv_cpu_getdb_info (sock, hdr, msgbuf);
    case HAL_MSG_CPU_GET_MASTER:
      return hsl_msg_recv_cpu_get_master (sock, hdr, msgbuf);
    case HAL_MSG_CPU_SET_MASTER:
      return hsl_msg_recv_cpu_set_master (sock, hdr, msgbuf);
    case HAL_MSG_CPU_GETDB_INDEX:
      return hsl_msg_recv_cpu_get_info_index (sock, hdr, msgbuf);
    case HAL_MSG_CPU_GET_LOCAL:
      return hsl_msg_recv_cpu_get_local (sock, hdr, msgbuf);
    case HAL_MSG_CPU_GETDUMP_INDEX:
      return hsl_msg_recv_cpu_get_dump_index (sock, hdr, msgbuf);
    /* djg */
	case HAL_MSG_PTP_CLK_TYPE_SET:	       
	    return hsl_msg_recv_ptp_set_clock_type (sock, hdr, msgbuf);
	case HAL_MSG_PTP_DOMAIN_CREATE:
		return hsl_msg_recv_ptp_create_domain (sock, hdr, msgbuf);
	case HAL_MSG_PTP_DOMAIN_MPORT_ADD:	 
		return hsl_msg_recv_ptp_domain_add_mport (sock, hdr, msgbuf);
	case HAL_MSG_PTP_DOMAIN_SPORT_ADD:	 
		return hsl_msg_recv_ptp_domain_add_sport (sock, hdr, msgbuf);
    case HAL_MSG_PTP_DELAY_MECH_SET:	        
		return hsl_msg_recv_ptp_set_delay_mech (sock, hdr, msgbuf);
    case HAL_MSG_PTP_PHYPORT_SET:           
		return hsl_msg_recv_ptp_set_phyport (sock, hdr, msgbuf);
    case HAL_MSG_PTP_PHYPORT_CLEAR:         
		return hsl_msg_recv_ptp_clear_phyport (sock, hdr, msgbuf);
    case HAL_MSG_PTP_PORT_ENABLE:           
		return hsl_msg_recv_ptp_enable_port (sock, hdr, msgbuf);
    case HAL_MSG_PTP_PORT_DISABLE:              
		return hsl_msg_recv_ptp_disable_port (sock, hdr, msgbuf);
    case HAL_MSG_PTP_PORT_DELAY_MECH:                
		return hsl_msg_recv_ptp_port_delay_mech (sock, hdr, msgbuf);
    case HAL_MSG_PTP_REQ_INTERVAL:                      
		return hsl_msg_recv_ptp_set_req_interval (sock, hdr, msgbuf);
    case HAL_MSG_PTP_P2P_INTERVAL:                      
		return hsl_msg_recv_ptp_set_p2p_interval (sock, hdr, msgbuf);
    case HAL_MSG_PTP_P2P_MEANDELAY:
		return hsl_msg_recv_ptp_set_p2p_meandelay (sock, hdr, msgbuf);
    case HAL_MSG_PTP_P2P_ASYMDELAY:
		return hsl_msg_recv_ptp_set_asym_delay (sock, hdr, msgbuf);

#ifdef HAVE_TUNNEL
    /* Tunnel */
    case HAL_MSG_TUNNEL_ADD:
      return hsl_msg_recv_tunnel_add (sock, hdr, msgbuf);
    case HAL_MSG_TUNNEL_DELETE:
      return hsl_msg_recv_tunnel_delete (sock, hdr, msgbuf);
    case HAL_MSG_TUNNEL_INIT_SET:
      return hsl_msg_recv_tunnel_initiator_set (sock, hdr, msgbuf);
    case HAL_MSG_TUNNEL_INIT_CLEAR:
      return hsl_msg_recv_tunnel_initiator_clear (sock, hdr, msgbuf);
    case HAL_MSG_TUNNEL_TERM_SET:
      return hsl_msg_recv_tunnel_terminator_set (sock, hdr, msgbuf);
    case HAL_MSG_TUNNEL_TERM_CLEAR:
      return hsl_msg_recv_tunnel_terminator_clear (sock, hdr, msgbuf);
#endif /* HAVE_TUNNEL*/

#if 0
#ifdef HAVE_VPWS   /*cai 2011-10 vpws module*/
    case HAL_MSG_VPWS_ADD:
      return hsl_msg_vpws_add (sock, hdr, msgbuf);	
    case HAL_MSG_VPWS_DELETE:
      return hsl_msg_vpws_delete (sock, hdr, msgbuf);
#endif /* Have_vpws*/
    case HAL_MSG_VLL_INGRESS:	
      return hsl_msg_vpws_pe_create (sock, hdr, msgbuf);
    case HAL_MSG_VLL_EGRESS:	
      return 0;
    case HAL_MSG_VLL_INTERMEDIATE:
      return hsl_msg_vpws_p_create (sock, hdr, msgbuf);
    case HAL_MSG_VLL_DELETE:      
      return hsl_msg_vpws_delete (sock, hdr, msgbuf);

#include "hal_vpls.h"
    case HAL_MSG_VPLS_VPN_ADD:	
		return hsl_msg_vpls_vpn_add (sock, hdr, msgbuf);
    case HAL_MSG_VPLS_VPN_DEL:
      return hsl_msg_vpls_vpn_del (sock, hdr, msgbuf);		
    case HAL_MSG_VPLS_PORT_ADD:
      return hsl_msg_vpls_port_add (sock, hdr, msgbuf);		
    case HAL_MSG_VPLS_PORT_DEL:		
      return hsl_msg_vpls_port_del (sock, hdr, msgbuf);
#endif 

/* L2 vpn 操作(vpls， vpws) */
    case HAL_MSG_VPN_ADD:
	  return hsl_msg_vpn_add (sock, hdr, msgbuf);
	 
    case HAL_MSG_VPN_DEL:
	  return hsl_msg_vpn_del (sock, hdr, msgbuf);
	 
    case HAL_MSG_VPN_PORT_ADD:
	  return hsl_msg_vpn_port_add (sock, hdr, msgbuf);
	 
    case HAL_MSG_VPN_PORT_DEL:
	  return hsl_msg_vpn_port_del (sock, hdr, msgbuf);

	  /* vpls 静态mac地址操作*/
    case HAL_MSG_VPN_STATIC_MAC_ADD:
	 return hsl_msg_vpn_static_mac_add(sock, hdr, msgbuf);

    case HAL_MSG_VPN_STATIC_MAC_DEL:
	 return hsl_msg_vpn_static_mac_del(sock, hdr, msgbuf);
	 

/*隧道操作*/
    case	HAL_MSG_LSP_ADD:
	  	return hsl_msg_lsp_add (sock, hdr, msgbuf);
	 	
   	 case   HAL_MSG_LSP_DEL:
	  	return hsl_msg_lsp_del (sock, hdr, msgbuf);

	 case HAL_MSG_LSP_GET_LINK_STATUS:
        return hsl_msg_get_lsp_status(sock, hdr, msgbuf);
	  
	 case HAL_MSG_LSP_PROTECT_GRP_ADD:
        return hsl_msg_lsp_protect_group_add (sock, hdr, msgbuf);
            
    case HAL_MSG_LSP_PROTECT_GRP_DEL:
        return hsl_msg_lsp_protect_group_del (sock, hdr, msgbuf);
        
    case HAL_MSG_LSP_PROTECT_GRP_MOD_PARAM:
        return hsl_msg_lsp_protect_group_mod_param (sock, hdr, msgbuf);
        
    case HAL_MSG_LSP_PROTECT_GRP_SWITCH:
        return hsl_msg_lsp_protect_group_switch (sock, hdr, msgbuf);
        
    case HAL_MSG_LSP_GET_PROTECT_GRP_INFO:
            return hsl_msg_lsp_get_protect_grp_info (sock, hdr, msgbuf);


/*vpn 组播组操作*/
     case  HAL_MSG_MULTICAST_GROUP_ADD:
          return hsl_msg_vpn_multicast_group_create (sock, hdr, msgbuf);
		  
     case HAL_MSG_MULTICAST_GROUP_DEL:
	  return hsl_msg_vpn_multicast_group_del (sock, hdr, msgbuf);
	  
     case HAL_MSG_MULTICAST_GRP_PORT_ADD:
	  return hsl_msg_vpn_multicast_group_port_add (sock, hdr, msgbuf);
	 
     case HAL_MSG_MULTICAST_GRP_PORT_DEL:
	  return  hsl_msg_vpn_multicast_group_port_del (sock, hdr, msgbuf);
/*oam */
	case HAL_MSG_OAM_ADD:
		return hsl_msg_oam_add(sock, hdr, msgbuf);
	case HAL_MSG_OAM_DEL:
		return hsl_msg_oam_del(sock, hdr, msgbuf);
			
	case HAL_MSG_OAM_GET_STATUS:
		return hsl_msg_oam_get_status(sock, hdr, msgbuf);


/* car */
     case HAL_MSG_CAR_PORT_RATE_LIMIT:
	  return hsl_msg_port_rate_limit (sock, hdr, msgbuf);
	 
     case HAL_MSG_CAR_PORT_SHAPPING:
	  return  hsl_msg_port_shapping (sock, hdr, msgbuf);

     case HAL_MSG_CAR_VPN_AC:
	  return hsl_msg_ac_car_set (sock, hdr, msgbuf);
	 
     case HAL_MSG_CAR_VPN_MPLS:
	  return  hsl_msg_mpls_car_set (sock, hdr, msgbuf);
	  
	 case HAL_MSG_PRIORITY_VPN_AC:
	  return  hsl_msg_ac_priority_set (sock, hdr, msgbuf);

	case HAL_MSG_WRR_CAR_VPN_AC:
		return  hsl_msg_recv_qos_port_cosq_bandwidth_set (sock, hdr, msgbuf);
		
	case HAL_MSG_WRR_WRED_VPN_AC:
		return  hsl_msg_recv_qos_port_cosq_wred_drop_set (sock, hdr, msgbuf);
		
	case HAL_MSG_WRR_WEIGHT_VPN_AC:
		return  hsl_msg_recv_qos_port_cosq_weight_set (sock, hdr, msgbuf);
	  


    default:
      HSL_MSG_PROCESS_RETURN (sock, hdr, -ENOTSUPP);
      return 0;
    }

  return 0;
}

/* Sendmsg. */
static int 
#ifdef LINUX_KERNEL_2_6
_hsl_sock_sendmsg (struct kiocb *iocb, struct socket *sock, 
                   struct msghdr *msg, size_t len)
#else
_hsl_sock_sendmsg (struct socket *sock, struct msghdr *msg, int len,
                   struct scm_cookie *scm)
#endif
{
  u_char *buf = NULL;
  int err;

  /* Allocate work memory. */
  buf = (u_char *) kmalloc (len, GFP_KERNEL);
  if (! buf)
    goto ERR;

  /* Returns -EFAULT on error */
  err = memcpy_fromiovec (buf, msg->msg_iov, len);
  if (err)
    goto ERR;

  /* Process message. */
  hsl_sock_process_msg (sock, (char*)buf, len);

  /* Free buf. */
  if (buf)
    kfree (buf);

  return len;

 ERR:
  if (buf)
    kfree (buf); 

  return -1;
}

/* Recvmsg. */
static int
#ifdef LINUX_KERNEL_2_6
_hsl_sock_recvmsg (struct kiocb *iocb, struct socket *sock, 
                   struct msghdr *msg, size_t len, int flags)
#else
_hsl_sock_recvmsg (struct socket *sock, struct msghdr *msg, int len,
                   int flags, struct scm_cookie *scm)
#endif
{
  struct sock *sk;
  struct sk_buff *skb;
  struct hal_sockaddr_nl snl;
  int copied;
  struct hsl_sock *hsk;
  int socklen;
  int err;
  
  sk = sock->sk;
  if (! sk)
    return -EINVAL;

  //HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO, "===<---_hsl_sock_recvmsg a msg<---===\n");

  /* Read lock. */
  read_lock_bh (&hsl_socklist_lock);

  for (hsk = hsl_socklist; hsk; hsk = hsk->next)
    {
      if (hsk->sk == sk)
        {
          /* Set multicast group. */
          snl.nl_pid    = hsk->pid;
          snl.nl_groups = hsk->groups;
          break;
        }
    }

  /* Read unlock. */
  read_unlock_bh (&hsl_socklist_lock);
  
  /* Copy hal_sockaddr_nl. */
  socklen = sizeof (struct hal_sockaddr_nl);
  if (msg->msg_name)
    memcpy (msg->msg_name, &snl, socklen);
  msg->msg_namelen = socklen;

  /* Receive one msg from the queue. */
  skb = skb_recv_datagram (sk, flags, flags & MSG_DONTWAIT, &err);
  if (! skb)
    {
      return -EINVAL;
    }

  /* Did user send lesser buffer? */
  copied = skb->len;
  if (copied > len)
    {
      copied = len;
      msg->msg_flags |= MSG_TRUNC;
    }

  /* Copy message. */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,21)
  skb->mac_header = skb->data;
#else
  skb->h.raw = skb->data;
#endif /* LINUX_VERSION_CODE KERNEL_VERSION */

  err = skb_copy_datagram_iovec (skb, 0, msg->msg_iov, copied);
  if (err < 0)
    {
      skb_free_datagram (sk, skb);
      release_sock (sk);
      return err;
    }

  sock_recv_timestamp (msg, sk, skb);
  
  /* Free. */
  skb_free_datagram (sk, skb);

  return copied;
}

/* Bind. */
static int
_hsl_sock_bind (struct socket *sock, struct sockaddr *sockaddr, int sockaddr_len)
{
  struct sock *sk = sock->sk;
  struct hsl_sock *hsk;
  struct hal_sockaddr_nl *nl_sockaddr = (struct hal_sockaddr_nl *) sockaddr;

  if (! sk)
    return -EINVAL;

  /* Write lock. */
  write_lock_bh (&hsl_socklist_lock);

  for (hsk = hsl_socklist; hsk; hsk = hsk->next)
    {
      if (hsk->sk == sk)
        {
          /* Set multicast group. */
          hsk->groups = nl_sockaddr->nl_groups;
          hsk->pid = (int)sk;
          break;
        }
    }

  /* Write unlock. */
  write_unlock_bh (&hsl_socklist_lock);

  return 0;
}

/* Post skb to the socket. */
static int
_hsl_sock_post_skb (struct socket *sock, struct sk_buff *skb)
{
  struct sock *sk = sock->sk;
  int ret = 0;

#ifdef LINUX_KERNEL_2_6
  if (atomic_read (&sk->sk_rmem_alloc) + skb->truesize < (unsigned)sk->sk_rcvbuf)
    {
      skb_set_owner_r (skb, sk);
      skb->dev = NULL;
      spin_lock (&sk->sk_receive_queue.lock);
      __skb_queue_tail (&sk->sk_receive_queue, skb);
      spin_unlock (&sk->sk_receive_queue.lock);
      sk->sk_data_ready (sk, skb->len);
    }
#else
  if (atomic_read (&sk->rmem_alloc) + skb->truesize < (unsigned)sk->rcvbuf)
    {
      skb_set_owner_r (skb, sk);
      skb->dev = NULL;
      spin_lock (&sk->receive_queue.lock);
      __skb_queue_tail (&sk->receive_queue, skb);
      spin_unlock (&sk->receive_queue.lock);
      sk->data_ready (sk, skb->len);
    }
#endif
  else
    ret = -1;

  return ret;
}

/* Post buffer to socket. */
int
hsl_sock_post_buffer (struct socket *sock, char *buf, int size)
{
  struct sk_buff *skb = NULL;
  int ret;

  skb = alloc_skb (size, GFP_KERNEL);
  if (! skb)
    return -1;

  /* Copy data. */
  memcpy (skb->data, buf, size);
  skb->len = size;
  skb->truesize = size;

  ret = _hsl_sock_post_skb (sock, skb);
  if (ret < 0)
     kfree_skb (skb);

  return ret;
}

/*
  Post the (non-multi) message buffer to the socket. 
*/
int
hsl_sock_post_msg (struct socket *sock, int cmd, int flags, int seqno, char *buf, int size)
{
  int totsize;
  struct hal_nlmsghdr *nlh;
  int offset;
  struct sk_buff *skb = NULL;

  /* Total size. */
  totsize = 2 * HAL_NLMSG_ALIGN(HAL_NLMSGHDR_SIZE) + HAL_NLMSG_ALIGN(size);

  skb = alloc_skb (totsize, GFP_KERNEL);
  if (! skb)
    return -1;
  skb->len = totsize;
  skb->truesize = totsize;

  nlh = (struct hal_nlmsghdr *) skb->data;
  nlh->nlmsg_len = HAL_NLMSG_LENGTH (size);
  nlh->nlmsg_type = cmd;
  nlh->nlmsg_seq = seqno;

  /* Total message size. */
  offset = HAL_NLMSG_ALIGN(HAL_NLMSGHDR_SIZE);
  memcpy (skb->data + offset, buf, size);

  /* End message with done. */
  offset = HAL_NLMSG_ALIGN(nlh->nlmsg_len);
  nlh = (struct hal_nlmsghdr *) (skb->data + offset);
  nlh->nlmsg_len = HAL_NLMSG_LENGTH(0);
  nlh->nlmsg_type = HAL_NLMSG_DONE;

  return _hsl_sock_post_skb (sock, skb);
}

/* Post ACK. */
int
hsl_sock_post_ack (struct socket *sock, struct hal_nlmsghdr *hdr, int flags, int error)
{
  int acksz = 2 * HAL_NLMSG_ALIGN(HAL_NLMSGHDR_SIZE) + HAL_NLMSG_ALIGN(4);
  struct hal_nlmsghdr *nlh;
  int *err;
  struct sk_buff *skb;
  u_char *sp;

  skb = alloc_skb (acksz, GFP_KERNEL);
  if (! skb)
    return -1;
  skb->len = acksz;
  skb->truesize = acksz;

  nlh = (struct hal_nlmsghdr *) skb->data;
  nlh->nlmsg_type = HAL_NLMSG_ERROR;
  nlh->nlmsg_len = HAL_NLMSG_LENGTH(4 + HAL_NLMSGHDR_SIZE);
  nlh->nlmsg_flags = flags;

  sp = skb->data + HAL_NLMSG_ALIGN(HAL_NLMSGHDR_SIZE);
  err = (int *) sp;
  *err = error;

  sp += HAL_NLMSG_ALIGN(4);

  nlh = (struct hal_nlmsghdr *) sp;
  memcpy (nlh, hdr, HAL_NLMSGHDR_SIZE);

  HSL_LOG (HSL_LOG_MSG, HSL_LEVEL_INFO, "Message: HAL acknowledgement posted \n");
  return _hsl_sock_post_skb (sock, skb);
}

/* HSL socket initialization. */
int
hsl_sock_init (void)
{
  int ret = -1;
  ret = sock_register (&hsl_family_ops);
  return 0;
}

/* HSL socket deinitialization. */
int
hsl_sock_deinit (void)
{
  sock_unregister (AF_HSL);
  return 0;
}

/*
  HSL Socket Interface event function.
*/
int
hsl_sock_if_event (int cmd, void *param1, void *param2)
{
  struct hsl_sock *hsk;
  struct socket *sock;

  HSL_FN_ENTER ();
    
  for (hsk = hsl_socklist; hsk; hsk = hsk->next)
    if (hsk->groups & HAL_GROUP_LINK)
      {
#ifdef LINUX_KERNEL_2_6
        sock = hsk->sk->sk_socket;
#else
        sock = hsk->sk->socket;
#endif
        if (! sock)
          continue;

        switch (cmd)
          {
          case HSL_IF_EVENT_IFNEW:
            hsl_msg_ifnew (sock, param1, param2);
            break;
          case HSL_IF_EVENT_IFDELETE:
            hsl_msg_ifdelete (sock, param1, param2);
            break;
          case HSL_IF_EVENT_IFFLAGS:
            hsl_msg_ifflags (sock, param1, param2);
            break;
#ifdef HAVE_L3
          case HSL_IF_EVENT_IFNEWADDR:
            hsl_msg_ifnewaddr(sock, param1, param2);
            break;
          case HSL_IF_EVENT_IFDELADDR:
            hsl_msg_ifdeladdr(sock, param1, param2);
            break;
#endif /* HAVE_L3 */
          case HSL_IF_EVENT_IFMTU:
            hsl_msg_ifmtu(sock, param1, param2);
            break;
          case HSL_IF_EVENT_IFDUPLEX:
            hsl_msg_ifduplex(sock, param1, param2);
            break;
          case HSL_IF_EVENT_IFAUTONEGO:
            hsl_msg_ifautonego(sock, param1, param2);
            break;
          case HSL_IF_EVENT_IFHWADDR:
            hsl_msg_ifhwaddr(sock, param1, param2);
            break;
#ifdef HAVE_ONMD
          case HSL_IF_EVENT_FPWINDOW_EXPIRY:
            hsl_msg_if_fpwindow_expiry(sock, param1, param2);
#endif /* HAVE_ONMD */
          default:
            break;
          }
      }

  HSL_FN_EXIT (0);
}

/*
  HSL Sock interface manager notifier chain registration.
*/
void
hsl_sock_ifmgr_notify_chain_register (void)
{
  /* Initialize interface manager notifier chain. */
  hsl_ifmgr_notify_chain_register (&hsl_comm_cb);
}

/* 
   HSL Sock interface manger notifier chain deregistration.
*/
void
hsl_sock_ifmgr_notify_chain_unregister (void)
{   
  /* Unregister interface manager notifier chain. */
  hsl_ifmgr_notify_chain_unregister (&hsl_comm_cb);
}


