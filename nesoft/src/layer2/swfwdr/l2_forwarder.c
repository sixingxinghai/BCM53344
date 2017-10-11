/* Copyright 2004  All Rights Reserved.
                                                                          
HAL - An implementation of the HAL sublayer for Linux

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version
2 of the License, or (at your option) any later version.
                                                                          
*/
#include <linux/autoconf.h> 
#include <linux/types.h> 
#include <linux/sched.h> 
#include <linux/fcntl.h> 
#include <linux/socket.h> 
#include <linux/in.h> 
#include <linux/inet.h> 
#include <linux/netdevice.h> 
#include <linux/kmod.h> 
#include <net/ip.h> 
#include <net/protocol.h> 
#include <linux/skbuff.h> 
#include <net/sock.h> 
#include <linux/errno.h> 
#include <linux/timer.h> 
#include <asm/system.h> 
#include <asm/uaccess.h> 
#include <asm/ioctls.h> 
#include <linux/proc_fs.h> 
#include <linux/poll.h> 
#include <linux/module.h> 
#include <linux/init.h> 

#include <linux/kernel.h> 
#include <linux/inetdevice.h> 
#include <asm/uaccess.h> 
#include <net/net_namespace.h> 
#include <linux/nsproxy.h>
#include "bdebug.h"
#include "config.h"

#ifdef HAVE_L2
#include "if_ipifwd.h"
#endif /* HAVE_L2 */

#ifdef HAVE_AUTHD
#include "if_eapol.h"
#endif /* HAVE_AUTHD */

#ifdef HAVE_LACPD
#include "if_lacp.h"
#endif /* HAVE_LACPD */

#ifdef HAVE_EFM
#include "if_efm.h"
#endif /* HAVE_EFM */


void
l2_mod_dec_use_count ()
{
  module_put (THIS_MODULE);
}
                                                                         
void
l2_mod_inc_use_count ()
{
  try_module_get (THIS_MODULE);
}



/* List of all hal sockets. */
static struct hlist_head hal_sklist;
static rwlock_t hal_sklist_lock = RW_LOCK_UNLOCKED;
static atomic_t hal_socks_nr;
static struct proto_ops hal_ops;
int hal_create (struct net *net, struct socket *sock, int protocol);


static struct proto _proto = {
        .name     = "HSL",
        .owner    = THIS_MODULE,
        .obj_size = sizeof(struct sock),
};  


void 
hal_sock_destruct (struct sock *sk)
{
  BDEBUG ("HAL socket %p free - %d are alive\n", sk, 
          atomic_read (&hal_socks_nr));
}

/*
 *      Create a packet of type SOCK_HAL. 
 */

int 
hal_create (struct net *net, struct socket *sock, int protocol)
{
  struct sock *sk;

  BDEBUG ("protocol %d socket addr %p\n",protocol,sock);
  if (!capable (CAP_NET_RAW))
    return -EPERM;
  if (sock->type  != SOCK_RAW)
    return -ESOCKTNOSUPPORT;

  sock->state = SS_UNCONNECTED;

  sk = sk_alloc (current->nsproxy->net_ns, PF_HAL, GFP_KERNEL, &_proto);
 
  if (sk == NULL)
    {
      return -ENOBUFS;
    }

  BDEBUG ("sock %p\n",sk);
  sock->ops = &hal_ops;
  sock_init_data (sock,sk);

  sk->sk_family = PF_HAL;
  sk->sk_protocol = protocol;
  sk->sk_destruct = hal_sock_destruct;

  if (atomic_read (&hal_socks_nr) == 0)
    {
      l2_mod_inc_use_count ();
    }
  atomic_inc (&hal_socks_nr);

  BR_WRITE_LOCK_BH (&hal_sklist_lock);
  sk_add_node (sk, &hal_sklist);
  BR_WRITE_UNLOCK_BH (&hal_sklist_lock);

  return 0;
}

/*
 *      Close a HAL socket. This is fairly simple. We immediately go
 *      to 'closed' state and remove our protocol entry in the device list.
 */

static int 
hal_release (struct socket *sock)
{
  struct sock *sk = sock->sk;
  struct sock **skp;
  BDEBUG ("sock %p\n", sk);

  if (!sk)
    return 0;

  BR_WRITE_LOCK_BH (&hal_sklist_lock);
  sk_del_node_init (sk);
  BR_WRITE_UNLOCK_BH (&hal_sklist_lock);
  
  br_cleanup_bridges(sk);

  /*
   *    Now the socket is dead. No more input will appear.
   */

  sock_orphan (sk);
  sock->sk = NULL;

  /* Purge queues */
  skb_queue_purge (&sk->sk_receive_queue);

  if (atomic_dec_and_test (&hal_socks_nr))
    {
      l2_mod_dec_use_count ();
    }

  sock_put (sk);

  return 0;
}

DECLARE_MUTEX(hal_ioctl_mutex);

/* This function handles all ioctl commands. */
#define MAX_ARGS  7

int 
hal_ioctl (struct socket *sock, unsigned int cmd, unsigned long arg)
{
  int err = 0;
  unsigned long ioctl_cmd;

  BDEBUG ("sock %p cmd %d arg 0x%lx\n", sock, cmd, arg);

  if (cmd != SIOCPROTOPRIVATE)
    {
      /*
      BDEBUG("dev_ioctl: sock %p cmd %d\n", sock, cmd);
      return dev_ioctl (current->nsproxy->net_ns,cmd,(void *)arg);
      */
      return -EINVAL;
    }
  
  if (!capable (CAP_NET_ADMIN))
    {
      BDEBUG("sock %p not capable of ioctl\n",sock);
      return -EPERM;
    }
  
  /* Command is arg 0 */
  if (copy_from_user ((&ioctl_cmd), (void *)arg, (1 * sizeof (unsigned long))))
    {
      BDEBUG ("Unable to get command\n");
      return -EFAULT;
    }

  down (&hal_ioctl_mutex);

  switch (ioctl_cmd) 
    {
#ifdef HAVE_LACPD
    case APNLACP_GET_VERSION:
    case APNLACP_ADD_AGG:
    case APNLACP_DEL_AGG:
    case APNLACP_AGG_LINK:
    case APNLACP_DEAGG_LINK:
    case APNLACP_SET_MACADDR:
      err = lacp_ioctl (sock, cmd, arg);
      break;
#endif /* HAVE_LACPD */
#ifdef HAVE_L2
    case APNBR_GET_VERSION:
    case APNBR_ADD_IF:
    case APNBR_DEL_IF:
    case APNBR_GET_BRIDGE_INFO:
    case APNBR_GET_PORT_LIST:
    case APNBR_SET_PORT_STATE:
    case APNBR_GET_PORT_STATE:
    case APNBR_SET_PORT_FWDER_FLAGS:
    case APNBR_FLUSH_FDB_BY_PORT:
    case APNBR_SET_AGEING_TIME:
    case APNBR_SET_DYNAMIC_AGEING_INTERVAL:
    case APNBR_GET_PORT_INFO:
    case APNBR_SET_BRIDGE_LEARNING:
    case APNBR_GET_DYNFDB_ENTRIES:
    case APNBR_ADD_STATFDB_ENTRY:
    case APNBR_DEL_STATFDB_ENTRY:
    case APNBR_ADD_DYNAMIC_FDB_ENTRY:
    case APNBR_DEL_DYNAMIC_FDB_ENTRY:
    case APNBR_GET_STATFDB_ENTRIES:
    case APNBR_GET_BRIDGES:
    case APNBR_ADD_BRIDGE:
    case APNBR_DEL_BRIDGE:
    case APNBR_GET_DEVADDR:
    case APNBR_ADD_VLAN_TO_INST:
    case APN_VLAN_ADD:
    case APN_VLAN_DEL:
    case APN_VLAN_SET_PORT_TYPE:
    case APN_VLAN_SET_DEFAULT_PVID:
    case APN_VLAN_SET_NATIVE_VID:
    case APN_VLAN_SET_MTU:
    case APN_VLAN_ADD_VID_TO_PORT:
    case APN_VLAN_DEL_VID_FROM_PORT:
    case APNBR_ENABLE_IGMP_SNOOPING:
    case APNBR_DISABLE_IGMP_SNOOPING:
    case APNBR_GET_UNICAST_ENTRIES: 
    case APNBR_GET_MULTICAST_ENTRIES:
    case APNBR_CLEAR_FDB_BY_MAC:
    case APNBR_GARP_SET_BRIDGE_TYPE:
    case APNBR_ADD_GMRP_SERVICE_REQ:
    case APNBR_SET_EXT_FILTER:
    case APNBR_SET_PVLAN_TYPE:
    case APNBR_SET_PVLAN_ASSOCIATE:
    case APNBR_SET_PVLAN_PORT_MODE:
    case APNBR_SET_PVLAN_HOST_ASSOCIATION:
    case APNBR_ADD_CVLAN_REG_ENTRY:
    case APNBR_DEL_CVLAN_REG_ENTRY:
    case APNBR_ADD_VLAN_TRANS_ENTRY:
    case APNBR_DEL_VLAN_TRANS_ENTRY:
    case APNBR_SET_PROTO_PROCESS:
    case APN_VLAN_ADD_PRO_EDGE_PORT:
    case APN_VLAN_DEL_PRO_EDGE_PORT:
    case APN_VLAN_SET_PRO_EDGE_DEFAULT_PVID:
    case APN_VLAN_SET_PRO_EDGE_UNTAGGED_VID:
    case APNBR_CHANGE_VLAN_TYPE:
    case APNBR_GET_IFINDEX_BY_MAC_VID:
      err = br_ioctl_bridge (sock, arg);
      break;
#endif /* HAVE_L2 */
#ifdef HAVE_AUTHD
    case APNEAPOL_GET_VERSION:
    case APNEAPOL_ADD_PORT:
    case APNEAPOL_DEL_PORT:
    case APNEAPOL_SET_PORT_STATE:
    case APNEAPOL_SET_PORT_MACAUTH_STATE:
      err = eapol_ioctl (sock, cmd, arg);
      break;
#endif /* HAVE_AUTHD */
#ifdef HAVE_EFM
    case APNEFM_GET_VERSION:
    case APNEFM_ADD_PORT:
    case APNEFM_DEL_PORT:
    case APNEFM_SET_PORT_STATE:
      err = efm_ioctl (sock, cmd, arg);
      break;
#endif /* HAVE_EFM */
    default:
      {
        BDEBUG("Unknown command - cmd %ld\n",
               ioctl_cmd);
        up (&hal_ioctl_mutex);
        err = -EINVAL;
      }
      break;
    }

  up (&hal_ioctl_mutex);
  
  return err;
}

static struct proto_ops SOCKOPS_WRAPPED (hal_ops) = {
  family:       PF_HAL,

  release:      hal_release,
  bind:         sock_no_bind,
  connect:      sock_no_connect,
  socketpair:   sock_no_socketpair,
  accept:       sock_no_accept,
  getname:      sock_no_getname,
  poll:         sock_no_poll,
  ioctl:        hal_ioctl,
  listen:       sock_no_listen,
  shutdown:     sock_no_shutdown,
  setsockopt:   sock_no_setsockopt,
  getsockopt:   sock_no_getsockopt,
  sendmsg:      sock_no_sendmsg,
  recvmsg:      sock_no_recvmsg,
  mmap:         sock_no_mmap,
  sendpage:     sock_no_sendpage,
};

static struct net_proto_family hal_family_ops = {
  .family=              PF_HAL,
  .create=              hal_create,
  .owner=                THIS_MODULE,
};

void 
hal_exit (void)
{
  BWARN ("NET4: 7/19/2004 Linux HAL 1.0 for Net4.0 removed\n");
  sock_unregister (PF_HAL);
  return;
}

int  
hal_init (void)
{
  BWARN ("NET4: 7/19/2004 Linux HAL 1.0 for Net4.0\n");
  sock_register (&hal_family_ops);
  return 0;
}

static int __init
l2_module_init (void)
{
  BDEBUG("APN Layer2 Forwarder init\n");

  hal_init ();

#ifdef HAVE_L2
  br_init ();
#endif /* HAVE_L2 */

#ifdef HAVE_AUTHD
  eapol_init ();
#endif /* HAVE_AUTHD */

#ifdef HAVE_LACPD
  lacp_init();
#endif /* HAVE_LACPD */

#ifdef HAVE_LLDP
  lldp_init ();
#endif /* HAVE_LLDP */

#ifdef HAVE_ELMID
  elmi_init ();
#endif /* HAVE_ELMID */

  return 0;
}

static void __exit 
l2_module_exit (void)
{
  BDEBUG("APN Layer2 Forwarder exit\n");

  hal_exit ();

#ifdef HAVE_L2
  br_exit ();
#endif /* HAVE_L2 */

#ifdef HAVE_AUTHD
  eapol_exit ();
#endif /* HAVE_AUTHD */

#ifdef HAVE_LACPD
  lacp_exit();
#endif /* HAVE_LACPD */

#ifdef HAVE_LLDP
  lldp_exit ();
#endif /* HAVE_LLDP */

#ifdef HAVE_ELMID
  elmi_exit ();
#endif /* HAVE_ELMID */

}

module_init(l2_module_init);
module_exit(l2_module_exit);
#ifdef MODULE_LICENSE
MODULE_LICENSE("GPL");
#endif

