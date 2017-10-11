/* Copyright (C) 2001-2003  All Rights Reserved.  */

/* netlink interface.  */

#include "mpls_fwd.h"

#include "mpls_common.h"
#include "mpls_client.h"

#include "mpls_fib.h"
#include "mpls_hash.h"
#include "mpls_table.h"
#include "fncdef.h"
#include <net/net_namespace.h>
#include <linux/nsproxy.h>


/* Externed calls */
extern struct net_device *dev_get_by_name  (struct net *net, const char *name);
extern struct net_device *dev_get_by_index (struct net *net, int ifindex);
extern struct fib_handle *fib_handle;
extern struct dst_ops *dst_ops;
extern struct mutex rtnl_mutex;

/* The following is the static def. for the netlink socket used */
static struct sock *netlink_socket = NULL;

extern struct dst_ops *dst_ops;

struct bypass_tunnel_msg bt_msg;

uint32 nsm_client_pid = 0;
uint32 oam_client_pid = 0;


/*
 * The following routine creates a netlink user socket to be used
 * by the user-space protocols to talk to the MPLS Forwarder.
 */
struct sock *
mpls_netlink_sock_create ()
{
  struct netlink_sock *nlk; 

  netlink_socket = netlink_kernel_create (&init_net, NETLINK_USERSOCK, 0,
                                          mpls_netlink_rcv_func, &rtnl_mutex, 
					  THIS_MODULE);

  return netlink_socket;
}

/*
 * The following routine destroys the kernel netlink socket.
 */
void
mpls_netlink_sock_close ()
{
  sock_release (netlink_socket->sk_socket);

  /* Done */
}

/*
 * The following routine receives all packets that are meant
 * for the netlink user socket.
 */

void
mpls_netlink_rcv_func (struct sk_buff *skb)
{
  int ret = 0;

  if (!rtnl_trylock())
    return;   

  ret = mpls_netlink_rcv_and_process_skb (skb); 

  if (ret < 0)
    { 
      /* Let the netlink client know this happened */
      /* Not doing it right now */
      ;
    }

  rtnl_unlock ();
}

/*
 * The following routine receives and processes the sk buffer that
 * was received by the netlink socket.
 */
int
mpls_netlink_rcv_and_process_skb (struct sk_buff *skb)
{
  struct nlmsghdr *nlh;
  uint32 seq;
  int ret;

  /* Get the Netlink message header from the buffer */
  nlh = (struct nlmsghdr *)skb->data;

  if (!NLMSG_OK(nlh, skb->len))
    {
      PRINTK_WARNING ("  Received corrupt netlink message \n");
      return FAILURE;
    }

  /* Get the sequence number */
  seq = nlh->nlmsg_seq;
  
  if ((nlh->nlmsg_len < sizeof (*nlh)) ||
      (skb->len < nlh->nlmsg_len) ||
      (!(nlh->nlmsg_flags & NLM_F_REQUEST)) ||
      (seq != MPLS_SEQ_NUM)) 
    {
      PRINTK_WARNING ("Invalid data received on netlink socket\n");
      return FAILURE;
    }

  /* Call the function corresponding to the type, or report error */
  ret = mpls_netlink_process_message_type (nlh, skb);
  
  switch (ret)
    {
    case (SUCCESS):
      goto done;
    case (UNKNOWN_TYPE):
      {
        PRINTK_WARNING ("Invalid message received by netlink socket\n");
        goto done_with_err;
      }
    default:
      goto done_with_err;
    }

 done_with_err:
  return FAILURE;
  
 done:  
  return SUCCESS;
}

int
mpls_netlink_send (struct sk_buff *skb, struct mpls_oam_ctrl_data *ctrl_data,
                   int type)
{
  struct nlmsghdr *nlmh;
  int total_len = 0;
  int len = skb->len;
  int ret = 0;
  
  skb_push (skb, NLMSG_ALIGN (sizeof (struct nlmsghdr)));
  nlmh = (struct nlmsghdr *) skb->data;
  nlmh->nlmsg_len = NLMSG_LENGTH (len);
  nlmh->nlmsg_type = type;

  /* send all the oamc packets to the OAM client */
  if (oam_client_pid)
    ret = netlink_unicast (netlink_socket, skb, oam_client_pid, MSG_DONTWAIT); 

  return 0;
}


/*
 * The following routine processes the type of netlink message
 * received.
 */
int
mpls_netlink_process_message_type (struct nlmsghdr *nlh, struct sk_buff *skb)
{
  int type;
  int ret;

  /* Get the type from the message */
  type = nlh->nlmsg_type;

  switch (type)
    {
    case (USERSOCK_MPLS_INIT):
      ret = mpls_netlink_usock_mpls_ctrl (nlh, skb, type);
      break;
    case (USERSOCK_MPLS_END):
      oam_client_pid = 0;
      ret = mpls_netlink_usock_mpls_ctrl (nlh, skb, type);
        break;
    case (USERSOCK_MPLS_IF_INIT):
    case (USERSOCK_MPLS_IF_END):
      ret = mpls_netlink_usock_mpls_if_ctrl (nlh, type);
      break;
    case (USERSOCK_MPLS_VRF_INIT):
    case (USERSOCK_MPLS_VRF_END):
      ret = mpls_netlink_usock_mpls_vrf_ctrl (nlh, type);
      break;
    case (USERSOCK_MPLS_VC_INIT):
    case (USERSOCK_MPLS_VC_END):
      ret = mpls_netlink_usock_mpls_vc_ctrl (nlh, type);
      break;
    case (USERSOCK_NEWILM):
      ret = mpls_netlink_usock_newilm (nlh);
      break;
    case (USERSOCK_DELILM):
      ret = mpls_netlink_usock_delilm (nlh);
      break;
    case (USERSOCK_NEWFTN):
      ret = mpls_netlink_usock_newftn (nlh);
      break;
    case (USERSOCK_DELFTN):
      ret = mpls_netlink_usock_delftn (nlh);
      break;
    case (USERSOCK_NEW_VC_FTN):
      ret = mpls_netlink_usock_new_vc_ftn (nlh);
      break;
    case (USERSOCK_DEL_VC_FTN):
      ret = mpls_netlink_usock_del_vc_ftn (nlh);
      break;
    case (USERSOCK_FIB_CLEAN):
      ret = mpls_netlink_usock_clean_fib (nlh);
      break;
    case (USERSOCK_VRF_CLEAN):
      ret = mpls_netlink_usock_clean_vrf (nlh);
      break;
    case (USERSOCK_IF_UPDATE):
      ret = mpls_netlink_usock_mpls_if_update (nlh);
      break;
    case (USERSOCK_TTL_HANDLING):
      ret = mpls_netlink_usock_ttl_handling (nlh);
      break;
    case (USERSOCK_LOCAL_PKT):
      ret = mpls_netlink_usock_local_pkt_handling (nlh);
      break;
    case (USERSOCK_DEBUGGING):
      ret = mpls_netlink_usock_debug_handling (nlh);
      break;
    case (USERSOCK_STATS_CLEAR):
      ret = mpls_netlink_usock_stats_clear_handling (nlh);
      break;
    case (USERSOCK_BYPASS_SEND):
      ret = mpls_netlink_usock_bypass_tunnel_send (nlh);
      break;
    case (USERSOCK_OAM_SEND) :
      ret = mpls_netlink_usock_oam_send (nlh, skb);
      break;
    case (USERSOCK_OAM_VC_SEND) :
      ret = mpls_netlink_usock_oam_vc_send (nlh);
      break;
    default:
      ret = UNKNOWN_TYPE;
    }
  
  return ret;
}

/*
 * The following routine initializes of disables the MPLS Forwarder,
 * as per the request.
 */
int
mpls_netlink_usock_mpls_ctrl (struct nlmsghdr *nlh, struct sk_buff *skb, int type)
{
  struct mpls_control *req = NLMSG_DATA(nlh);
  
  MPLS_ASSERT (req != NULL);

  /* Make sure the protocol passed in is a valid protocol. */
  MPLS_ASSERT (req->protocol != APN_PROTO_UNSPEC);

  if (type == USERSOCK_MPLS_INIT)
    { 
      /* Special handling for NSM binds. The assumption here is that the NSM
         is the FIRST daemon that will be started up. */
      if (req->protocol == APN_PROTO_NSM)
        {
          /* Clean up for this protocol in global tables */
          mpls_destroy_ftn (fib_handle->ftn, req->protocol);

          /* Clean up for this protocol in interface ILM's if any */
          mpls_ilm_clean_all (req->protocol);

          /* Clean up for this protocol in all VRF tables. */
          mpls_vrf_clean_all (req->protocol);

          nsm_client_pid = NETLINK_CB (skb).pid; 

          /* Delete all VRF tables. */
          mpls_vrf_delete_all (fib_handle->vrf_list);

          /* Delete all interfaces. */
          mpls_hash_free_all ((void (*) (void *)) mpls_if_delete_only);
        }
      else if (req->protocol = APN_PROTO_OAM)
        {
          oam_client_pid = NETLINK_CB (skb).pid;
        }

      /* Set the bit for this protocol in refcount */
      set_mpls_refcount (req->protocol);

      return (SUCCESS);
    }
  else if (type == USERSOCK_MPLS_END)
    {
      /* Special handling for NSM unbinds. The assumption here is that the NSM
         is the LAST daemon that will be killed. */
      if (req->protocol == APN_PROTO_NSM)
        {
          /* Clean up for this protocol */
          mpls_destroy_ftn (fib_handle->ftn, req->protocol);

          /* Clean up for this protocol in interface ILM's if any */
          mpls_ilm_clean_all (req->protocol);

          /* Clean up for this protocol in all VRF tables. */
          mpls_vrf_clean_all (req->protocol);

          nsm_client_pid = 0;

          /* Delete all VRF tables. */
          mpls_vrf_delete_all (fib_handle->vrf_list);

          /* Delete all interfaces. */
          mpls_hash_free_all ((void (*) (void *)) mpls_if_delete_only);

          /* Set TTL parameters to default */
          propagate_ttl = TTL_PROPAGATE_ENABLED;
          lsp_model     = LSP_MODEL_UNIFORM; 
        }
      else if (req->protocol == APN_PROTO_OAM)
        {
          oam_client_pid = 0;
        }

      /* Unset the bit for this protocol in refcount */
      unset_mpls_refcount (req->protocol);

      return (SUCCESS);
    }

  return (FAILURE);
}

/*
 * The following routine initializes or disables an interface for
 * MPLS Forwarding, as per the request.
 */
int
mpls_netlink_usock_mpls_if_ctrl (struct nlmsghdr *nlh, int type)
{
  struct mpls_if *req = NLMSG_DATA(nlh);
  struct net_device *dev;
  struct mpls_interface *ifp;
  uint32 ifindex;
  
  MPLS_ASSERT (req != NULL);

  if (req->ifindex == -1)
    {
      /* Ifname must have been passed */
      dev = dev_get_by_name (&init_net, req->ifname);
      if (! dev)
        {
          PRINTK_WARNING ("Interface with name %s does not exist\n",
                          req->ifname);
          goto failure;
        }
      
      /* Get the correct ifindex from dev */
      ifindex = dev->ifindex;
    }
  else
    {
      /* Copy over the ifindex from req */
      ifindex = req->ifindex;
    }

  if (type == USERSOCK_MPLS_IF_INIT)
    {
      /* Cannot pass vrf in the create if routine */
      MPLS_ASSERT (req->vrf_ident == 0);

      ifp = mpls_if_create (ifindex, req->label_space, MPLS_ENABLED);
      if (! ifp)
        {
          PRINTK_WARNING ("Could not create MPLS interface "
                          "with ifindex %d labelspace %d\n",
                          (int)ifindex, (int)req->label_space);
        }
      else
        {
          return (SUCCESS);
        }
    }
  else if (type == USERSOCK_MPLS_IF_END)
    {
      MPLS_ASSERT (req->label_space == 0 && req->vrf_ident == 0);

      /* First get the ifp */
      ifp = mpls_if_lookup (ifindex);
      if (ifp)
        {
          mpls_if_delete (ifp);
          return (SUCCESS);
        }
      else
        {
          PRINTK_WARNING ("Delete failed. Could not find "
                          "MPLS interface with ifindex %d\n",
                          (int)ifindex);
        }
    }

 failure:
  return (FAILURE);
}

/*
 * The following routine initializes or disables an interface for
 * Virtual Circuits, as per the request.
 */
int
mpls_netlink_usock_mpls_vc_ctrl (struct nlmsghdr *nlh, int type)
{
  struct mpls_if *req = NLMSG_DATA(nlh);
  struct net_device *dev;
  int ret;
  
  MPLS_ASSERT (req != NULL);

  dev = NULL;
  if (req->ifindex == -1)
    {
      /* Ifname must have been passed. */
      dev = dev_get_by_name (&init_net, req->ifname);
      if (! dev)
        {
          PRINTK_WARNING ("Interface with name %s does not exist\n",
                          req->ifname);
          goto failure;
        }
    }
  else
    {
      /* Get dev using ifindex. */
      dev = dev_get_by_index (&init_net, req->ifindex);
      if (! dev)
        {
          PRINTK_WARNING ("Interface with ifindex %d does not exist\n",
                          req->ifindex);
          goto failure;
        }
    }

  if (type == USERSOCK_MPLS_VC_INIT)
    {
      /* Cannot pass vrf in the create if routine */
      MPLS_ASSERT (req->vrf_ident == 0 && req->label_space == 0);

      ret = mpls_if_vc_bind (dev, req->vc_id);
      if (ret != SUCCESS)
        {
          PRINTK_WARNING ("Could not bind MPLS interface with ifindex "
                          "%d to Virtual Circuit with id %d\n",
                          (int)dev->ifindex, (int)req->vc_id);
        }
      else
        {
          PRINTK_WARNING ("Successfully bound MPLS interface with ifindex "
                          "%d to Virtual Circuit with id %d\n",
                          (int)dev->ifindex, (int)req->vc_id);
          return (SUCCESS);
        }
    }
  else if (type == USERSOCK_MPLS_VC_END)
    {
      MPLS_ASSERT (req->label_space == 0 && req->vrf_ident == 0);

      /* First get the ifp */
      ret = mpls_if_vc_unbind (dev, req->vc_id);
      if (ret != SUCCESS)
        {
          PRINTK_WARNING ("VC unbind failed: MPLS Interface with ifindex "
                          "%d is not bound to Virtual Circuit with id %d\n",
                          (int)dev->ifindex, (int)req->vc_id);
        }
      else
        {
          PRINTK_WARNING ("VC unbind successful for MPLS Interface with "
                          "ifindex %d and vc id %d\n",
                          (int)dev->ifindex, (int)req->vc_id);
          return (SUCCESS);
        }
    }

 failure:
  return (FAILURE);
}

/*
 * The following routine initializes or disables a VRF in the
 * MPLS Forwarder, as per the request.
 */
int
mpls_netlink_usock_mpls_vrf_ctrl (struct nlmsghdr *nlh, int type)
{
  struct mpls_if *req = NLMSG_DATA(nlh);
  
  MPLS_ASSERT (req != NULL);

  MPLS_ASSERT (req->ifindex == 0 && req->label_space == 0);

  if (type == USERSOCK_MPLS_VRF_INIT)
    { 
      struct mpls_table *vrf;

      vrf = mpls_vrf_create (req->vrf_ident);
      if (! vrf)
        {
          PRINTK_WARNING ("Could not create VRF with vrf ident %d\n",
                          req->vrf_ident);
        }
      else
        {
          return (SUCCESS);
        }
    }
  else if (type == USERSOCK_MPLS_VRF_END)
    {
      mpls_vrf_delete (req->vrf_ident);
    }

  return (FAILURE);
}

/*
 * Update the vrf binding for an MPLS interface.
 */
int
mpls_netlink_usock_mpls_if_update (struct nlmsghdr *nlh)
{
  struct mpls_if *req = NLMSG_DATA(nlh);
  struct mpls_interface *ifp;
  int ret;
  
  MPLS_ASSERT (req != NULL);

  /* Make sure that vrf ident is not zero but labelspace is*/
  MPLS_ASSERT (req->vrf_ident != 0);
  MPLS_ASSERT (req->label_space == 0);

  /* First get the ifp */
  ifp = mpls_if_update_vrf (req->ifindex, req->vrf_ident, &ret);
  if (ret == SUCCESS)
    {
      if (! ifp)
        PRINTK_DEBUG ("Removed VRF table for interface "
                      "with interface index %d and removed the "
                      "interface as well\n", (int)req->ifindex);
      
      return (SUCCESS);
    }
  else
    {
      PRINTK_WARNING ("Could not find MPLS interface with ifindex %d\n",
                      (int)req->ifindex);
    }
  
  return (FAILURE);
}

/*
 * The following routine clean the MPLS FTN/ILM tables for
 * the specified protocol.
 */
int
mpls_netlink_usock_clean_fib (struct nlmsghdr *nlh)
{
  int retval = 0;
  struct mpls_control *req = NLMSG_DATA(nlh);

  MPLS_ASSERT(req != NULL);

  /* Make sure the protocol passed in is a valid protocol. */
  MPLS_ASSERT (req->protocol != APN_PROTO_UNSPEC);

  mpls_destroy_ftn (fib_handle->ftn, req->protocol);
  mpls_ilm_clean_all (req->protocol);
  
  return retval;
}

/*
 * The following routine clean the VRF tables in the MPLS
 * Forwarder.
 *
 * Please be advised that this routine does not take care of
 * resetting the vrf pointers in the vrf-enabled interfaces.
 */
int
mpls_netlink_usock_clean_vrf (struct nlmsghdr *nlh)
{
  int retval = 0;

  MPLS_ASSERT(req != NULL);

  /* Clean all VRF tables */
  mpls_vrf_delete_all (fib_handle->vrf_list);
  
  return retval;
}


/* This function is called when an netlink message is received for
   adding an ILM entry */
int
mpls_netlink_usock_newilm (struct nlmsghdr *nlh)
{
  int retval = 0;
  struct net_device *dev;
  struct ilm_add_struct *req = NLMSG_DATA(nlh);
#ifdef HAVE_SWFWDR 
  struct mpls_interface *ifp;
#endif  
  MPLS_ASSERT(req != NULL);

  /* Make sure the protocol passed in is a valid protocol. */
  MPLS_ASSERT (req->protocol != APN_PROTO_UNSPEC);

  /*
   * outl2id = -1 signifies that the caller has filled the outintf field.
   */
  if (req->outl2id != -1)
    {
      dev = dev_get_by_index(&init_net, req->outl2id);
    }
  else
    {
      dev = dev_get_by_name(&init_net, req->outintf);
    }
  if (!dev)
    {
      return -ENODEV;
    }

#ifdef HAVE_SWFWDR
  ifp = mpls_if_lookup (dev->ifindex);
  if (ifp && ifp->l2_info == NULL)
    {
#endif
      if (dst_ops == NULL)
        dst_data_init (0, dev, SET_DST_OPS, RT_LOOKUP_OUTPUT);


      if (dst_ops == NULL)
      {
        PRINTK_WARNING ("Error : dst ops is not initialized \n");
        return -EINVAL;
      }
#ifdef HAVE_SWFWDR
    }
#endif  
  retval = fib_add_ilmentry(req->protocol, req->in_label, 
                            req->inl2id, &req->fec,
                            req->nexthop, dev->ifindex, req->out_label,
                            req->opcode, req->nhlfe_ix, &req->owner);
  if ((retval != SUCCESS) && (retval != -EEXIST))
    {
      PRINTK_WARNING ("mpls_netlink_usock_newilm : "
                      "Failed to add ILM entry %d --> %d for FEC "
                      "[%d.%d.%d.%d] with err %d\n",
                      (int)req->in_label, (int)req->out_label,
                      NIPQUAD (req->fec.u.prefix4.s_addr), retval);
      goto failed;
    }
  
  return retval;
  
 failed:
  return MSG_PROCESS_ERR;
}
        
/* This function is called when an netlink message is received for
   deleting an ILM entry. This function is also used for solicitation
   and termination message.  */
int
mpls_netlink_usock_delilm (struct nlmsghdr *nlh)
{
  struct ilm_del_struct *req = NLMSG_DATA(nlh);
  struct mpls_interface *ifp;
  int retval;

  MPLS_ASSERT(req != NULL);

  /* Make sure the protocol passed in is a valid protocol. */
  MPLS_ASSERT (req->protocol != APN_PROTO_UNSPEC);

  /*
   * it should come here for delete ILM calls. Right now we don't have 
   * interface specific label space so interface is 0.
   */

  /* Find the inteface */
  ifp = mpls_if_lookup (req->inl2id);
  if (! ifp)
    {
      PRINTK_WARNING ("Failed to lookup interace %d during "
                      "mpls_netlink_usock_delilm\n", (int)req->inl2id);
      return -EINVAL;
    }

  /* Delete this ILM entry */
  fib_find_ilmentry (ifp->ilm, req->protocol, req->in_label,
                     req->inl2id, 1 /* delete */, &retval);
  if (retval != SUCCESS)
    {
      PRINTK_WARNING ("mpls_netlink_usock_delilm : "
                      "Failed to delete ILM entry for incoming label %d "
                      "with retval %d\n",
                      (int)req->in_label, retval);
      return MSG_PROCESS_ERR;
    }
  else
    {
      PRINTK_WARNING ("mpls_netlink_usock_delilm : "
                      "Successfully deleted ILM entry for incoming "
                      "label %d\n", (int)req->in_label);
    }
  
  return SUCCESS;
}

/* This function is called when an netlink message is received for
   adding a VC specific FTN entry.  */
int
mpls_netlink_usock_new_vc_ftn (struct nlmsghdr *nlh)
{
  struct net_device *in_dev, *out_dev;
  struct mpls_interface *in_ifp, *out_ifp;
  struct vc_ftn_add_struct *req;
  int retval = 0;

  /* Set req. */
  req = NLMSG_DATA(nlh);
  MPLS_ASSERT(req != NULL);

  /* Make sure the protocol passed in is a valid protocol. */
  MPLS_ASSERT (req->protocol != APN_PROTO_UNSPEC);

  /* Confirm opcodes. */
  MPLS_ASSERT ((req->opcode == PUSH_FOR_VC) ||
          (req->opcode == PUSH_AND_LOOKUP_FOR_VC));

  /* Get outgoing interface device. */
  if (req->outl2id != -1)
    out_dev = dev_get_by_index(&init_net, req->outl2id);
  else
    out_dev = dev_get_by_name(&init_net, req->outintf);
  if (! out_dev)
    return -ENODEV;

  /* Get incoming interface device. */
  if (req->inl2id != -1)
    in_dev = dev_get_by_index(&init_net, req->inl2id);
  else
    in_dev = dev_get_by_name(&init_net, req->inintf);
  if (! in_dev)
    return -ENODEV;
  
  /* Check if MPLS enabled on incoming interface. */
  in_ifp = mpls_if_lookup (in_dev->ifindex);
  if (! in_ifp)
    {
      PRINTK_ERR ("mpls_netlink_usock_new_vc_ftn : Incoming interface with "
                  "index %d not enabled for MPLS\n", (int)in_dev->ifindex);
      return -ENODEV;
    }

  /* Check if MPLS enabled on outgoing interface for PUSH_FOR_VC case. */
  if (req->opcode == PUSH_FOR_VC)
    {
      out_ifp = mpls_if_lookup (out_dev->ifindex);
      if (! out_ifp)
        {
          PRINTK_ERR ("mpls_netlink_usock_new_vc_ftn : Outgoing interface "
                      "with index %d not enabled for MPLS\n",
                      (int)in_dev->ifindex);
          return -ENODEV;
        }
    }

  /* Compare vc ids. */
  if (in_ifp->vc_id != req->vc_id)
    {
      PRINTK_ERR ("mpls_netlink_usock_new_vc_ftn : Virtual Circuit "
                  "id mismatch. FTN add failure\n");
      return FAILURE;
    }
  
  /* Add entry. */
  retval = mpls_if_add_vc_ftnentry (req->protocol, req->vc_id,
                                    req->out_label, req->nexthop, in_ifp,
                                    out_dev, req->opcode, req->tunnel_ftnix);
  if (retval != SUCCESS)
    {
      PRINTK_WARNING ("mpls_netlink_usock_new_vc_ftn : "
                      "Failed to add Virtual Cirtuit FTN entry "
                      "for interface with index %d  with outgoing "
                      "label %d with a retval of %d\n",
                      (int)in_dev->ifindex, (int)req->out_label, retval);
      
      return MSG_PROCESS_ERR;
    }
  else
    PRINTK_NOTICE ("mpls_netlink_usock_new_vc_ftn : "
                   "Successfully added Virtual Cirtuit FTN entry "
                   "for interface with index %d  with outgoing "
                   "label %d\n", (int)in_dev->ifindex, (int)req->out_label);
  
  return retval;
}

/* This function is called when an netlink message is received for
   adding an FTN entry.  */
int
mpls_netlink_usock_newftn (struct nlmsghdr *nlh)
{
  int               retval = 0;
  struct net_device *dev;
  struct ftn_add_struct *req = NLMSG_DATA(nlh);
  struct mpls_table *table;
  u32 addr;

  MPLS_ASSERT(req != NULL);

  /* Make sure the protocol passed in is a valid protocol. */
  MPLS_ASSERT (req->protocol != APN_PROTO_UNSPEC);

  /*
   * outl2id = -1 signifies that the caller has filled the outintf field.
   */
  if (req->outl2id != -1)
    {
      dev = dev_get_by_index(&init_net, req->outl2id);
    }
  else
    {
      dev = dev_get_by_name(&init_net, req->outintf);
    }
  if (!dev)
    {
      return -ENODEV;
    }

  MPLS_ASSERT (req->vrf != 0);
  addr = 0;

  if (req->vrf == GLOBAL_TABLE)
    {
      table = fib_handle->ftn;
      addr = req->nexthop;
    }
  else
    {
      table = mpls_vrf_lookup_by_id (req->vrf, 0 /* dont create */);
      
      if ((req->opcode == PUSH) || (req->opcode == PUSH_AND_LOOKUP))
        addr = req->nexthop;
    }  

  if (dst_ops == NULL)
    {
      if (addr == 0)
        dst_data_init (addr, dev, SET_DST_OPS, RT_LOOKUP_OUTPUT);
      else
        dst_data_init (addr, dev, SET_DST_ALL, RT_LOOKUP_INPUT);
    }

  if (dst_ops == NULL)
    {
      PRINTK_ERR ("Error : dst ops not initialized \n");
      return -EINVAL;
    }

  if (table)
    {
      retval = fib_add_ftnentry(table, req->protocol, &(req->fec),
                                req->nexthop, dev->ifindex,
                                req->out_label, req->opcode,
                                req->nhlfe_ix, req->ftn_ix,
				&req->owner, req->bypass_ftn_ix,
				req->lsp_type,
				req->active_head);
      if ((retval != SUCCESS) && (retval != -EEXIST))
        {
          PRINTK_WARNING ("mpls_netlink_usock_newftn : "
                          "Failed to add FTN entry "
                          "for FEC [%d.%d.%d.%d] with outgoing label %d "
                          "with a retval of %d\n",
                          NIPQUAD(req->fec.u.prefix4.s_addr),
                          (int)req->out_label, retval);
              
          goto failed;
        }
    }
  else
    {
      PRINTK_WARNING ("Request to add FTN entry for fec [%d.%d.%d.%d/%d] "
                      "failed because VRF table with ident %d "
                      "does not exist\n",
                      NIPQUAD (req->fec.u.prefix4.s_addr),
                      (int)req->fec.prefixlen,
                      req->vrf);
      goto failed;
    }
  
  return retval;
  
 failed:
  return MSG_PROCESS_ERR;
}

/* This function is called when an netlink message is received for
   deleting a Virtual Circuit specific FTN entry.  */
int
mpls_netlink_usock_del_vc_ftn (struct nlmsghdr *nlh)
{
  struct vc_ftn_del_struct *req;
  struct net_device *in_dev;
  struct mpls_interface *ifp;
  int retval;

  /* Get req. */
  req = NLMSG_DATA(nlh);
  MPLS_ASSERT(req != NULL);

  /* Make sure the protocol passed in is a valid protocol. */
  MPLS_ASSERT (req->protocol != APN_PROTO_UNSPEC);

  /* Get incoming interface device. */
  if (req->inl2id != -1)
    in_dev = dev_get_by_index(&init_net, req->inl2id);
  else
    in_dev = dev_get_by_name(&init_net, req->inintf);
  if (! in_dev)
    return -ENODEV;

  /* Get struct mpls_interface and compare vc id. */
  ifp = mpls_if_lookup (in_dev->ifindex);
  if (! ifp)
    return -ENODEV;

  if (ifp->vc_id != req->vc_id)
    {
      PRINTK_ERR ("mpls_netlink_usock_del_vc_ftn : Virtual Circuit "
                  "id mismatch. FTN delete failure\n");
      return FAILURE;
    }

  /* Remove entry. */
  retval = mpls_if_del_vc_ftnentry (ifp, req->protocol);
  if (retval != SUCCESS)
    {
      PRINTK_WARNING ("mpls_netlink_usock_del_vc_ftn : "
                      "Failed to delete Virtual Cirtuit FTN entry "
                      "for interface with index %d  with VC id "
                      "%d and a retval of %d\n",
                      (int)in_dev->ifindex, (int)req->vc_id, retval);
      
      return MSG_PROCESS_ERR;
    }
  else
    PRINTK_NOTICE ("mpls_netlink_usock_del_vc_ftn : "
                   "Successfully deleted Virtual Cirtuit FTN entry "
                   "for interface with index %d  with VC id %d\n",
                   (int)in_dev->ifindex, (int)req->vc_id);
  
  return retval;
}

/* This function is called when an netlink message is received for
   deleting an FTN entry.  */
int
mpls_netlink_usock_delftn(struct nlmsghdr *nlh)
{
  int retval;
  struct ftn_del_struct *req = NLMSG_DATA(nlh);
  struct mpls_table *table;

  MPLS_ASSERT(req != NULL);

  /* Make sure the protocol passed in is a valid protocol. */
  MPLS_ASSERT (req->protocol != APN_PROTO_UNSPEC);

  MPLS_ASSERT (req->vrf != 0);

  if (req->vrf == GLOBAL_TABLE)
    table = fib_handle->ftn;
  else
    table = mpls_vrf_lookup_by_id (req->vrf, 0 /* dont create */);

  if (table)
    {
      fib_find_ftnentry (table, req->protocol, &(req->fec), req->ftn_ix,
                         FIB_DELETE, &retval);
      if (retval != SUCCESS)
        {
          PRINTK_WARNING ("mpls_netlink_usock_delftn : "
                          "Failed to delete FTN entry "
                          "for FEC [%d.%d.%d.%d] with retval of %d\n", 
                          NIPQUAD (req->fec.u.prefix4.s_addr), retval);
          return MSG_PROCESS_ERR;
        }
      else
        {
          PRINTK_WARNING ("mpls_netlink_usock_delftn : "
                          "Successfully deleted FTN entry "
                          "for FEC [%d.%d.%d.%d]\n",
                          NIPQUAD (req->fec.u.prefix4.s_addr));
        }
    }
  else
    {
      PRINTK_WARNING ("Request to delete FTN entry for fec [%d.%d.%d.%d/%d] "
                      "failed because VRF table with ident %d "
                      "does not exist\n",
                      NIPQUAD (req->fec.u.prefix4.s_addr),
                      (int)req->fec.prefixlen,
                      req->vrf);

      return MSG_PROCESS_ERR;
    }

  return retval;
}

/* This function updates TTL handling for this LSR */
int
mpls_netlink_usock_ttl_handling (struct nlmsghdr *nlh)
{
  struct mpls_ttl_handling *req = NLMSG_DATA(nlh);
  int retval = 0;

  /* Make sure there is something in REQ */
  MPLS_ASSERT(req != NULL);

  /* Only handling this from configuration utility */
  MPLS_ASSERT (req->protocol == APN_PROTO_NSM);

  /* Check if this is for the ingress of the egress ... */
  if (req->type == MPLS_TTL_VALUE_SET)
    {
      if (req->is_ingress)
        {
          ingress_ttl = req->new_ttl;

          PRINTK_DEBUG ("Ingress TTL value updated to %d\n", ingress_ttl);
        }
      else
        {
          egress_ttl = req->new_ttl;

          PRINTK_DEBUG ("Egress TTL value updated to %d\n", egress_ttl);
        }
    }
  else if (req->type == MPLS_TTL_PROPAGATE_SET)
    {
      propagate_ttl = TTL_PROPAGATE_ENABLED;
    }
  else if (req->type == MPLS_TTL_PROPAGATE_UNSET)
    {
      propagate_ttl = TTL_PROPAGATE_DISABLED;
    }
  else if (req->type == MPLS_LSP_MODEL_PIPE_SET)
    {
      lsp_model = LSP_MODEL_PIPE;
    }
  else if (req->type == MPLS_LSP_MODEL_PIPE_UNSET)
    {
      lsp_model = LSP_MODEL_UNIFORM;
    }

  return retval;
}  

/* This function enables/disables handling of local TCP packets */
int
mpls_netlink_usock_local_pkt_handling (struct nlmsghdr *nlh)
{
  struct mpls_conf_handle *req = NLMSG_DATA(nlh);
  int retval = 0;

  /* Make sure there is something in REQ */
  MPLS_ASSERT(req != NULL);
  
  /* Only handling this from configuration utility */
  MPLS_ASSERT (req->protocol == APN_PROTO_NSM);

  /* Message type MUST be MSG_MAX */
  MPLS_ASSERT (req->msg_type == KERN_MSG_MAX);
  
  /* Enable or disabling ... */
  if ((req->enable == MPLS_INIT) && (local_pkt_handle != MPLS_INIT))
    {
      local_pkt_handle = MPLS_INIT;

      /* Register the output handler */
      mpls_register_output_hdlr (mpls_tcp_output);

      PRINTK_DEBUG ("Enabled labelling of locally generated TCP packets\n");
    }
  else if ((req->enable == MPLS_END) && (local_pkt_handle != MPLS_END))
    {
      local_pkt_handle = MPLS_END;

      /* Unregister the output handler */
      mpls_unregister_output_hdlr ();

      PRINTK_DEBUG ("Disabled labelling of locally generated TCP packets\n");
    }

  return retval;
}

/* This function enables/disables handling of local TCP packets */
int
mpls_netlink_usock_debug_handling (struct nlmsghdr *nlh)
{
  struct mpls_conf_handle *req = NLMSG_DATA(nlh);
  int retval = 0;

  /* Make sure there is something in REQ */
  MPLS_ASSERT(req != NULL);
  
  /* Only handling this from configuration utility */
  MPLS_ASSERT (req->protocol == APN_PROTO_NSM);

  /* Message type cannot be max */
  MPLS_ASSERT (req->msg_type != KERN_MSG_MAX);

  /* If enabling ... */
  if (req->enable == MPLS_INIT)
    {
      if (req->msg_type & KERN_MSG_ERROR)
        {
          if (show_error_msg != MPLS_INIT)
            {
              /* Enable */
              show_error_msg = MPLS_INIT;
              
              PRINTK_DEBUG ("Enabled logging of ERROR messages for "
                            "MPLS module\n");
            }
        }
      
      if (req->msg_type & KERN_MSG_WARNING)
        {
          if (show_warning_msg != MPLS_INIT)
            {
              /* Enable */
              show_warning_msg = MPLS_INIT;
              
              PRINTK_DEBUG ("Enabled logging of WARNING messages for "
                            "MPLS module\n");
            }
        }
      
      if (req->msg_type & KERN_MSG_DEBUG)
        {
          if (show_debug_msg != MPLS_INIT)
            {
              /* Enable */
              show_debug_msg = MPLS_INIT;
              
              PRINTK_DEBUG ("Enabled logging of DEBUG messages for "
                            "MPLS module\n");
            }
        }
      
      if (req->msg_type & KERN_MSG_NOTICE)
        {
          if (show_notice_msg != MPLS_INIT)
            {
              /* Enable */
              show_notice_msg = MPLS_INIT;
              
              PRINTK_DEBUG ("Enabled logging of NOTICE messages for "
                            "MPLS module\n");
            }     
        }
    }
  else if (req->enable == MPLS_END)
    {
      if (req->msg_type & KERN_MSG_ERROR)
        {
          if (show_error_msg != MPLS_END)
            {
              /* Disable */
              show_error_msg = MPLS_END;
              
              PRINTK_DEBUG ("Disabled logging of ERROR messages for "
                            "MPLS module\n");
            }
        }
      
      if (req->msg_type & KERN_MSG_WARNING)
        {
          if (show_warning_msg != MPLS_END)
            {
              /* Disable */
              show_warning_msg = MPLS_END;
              
              PRINTK_DEBUG ("Disabled logging of WARNING messages for "
                            "MPLS module\n");
            }
        }

      if (req->msg_type & KERN_MSG_DEBUG)
        {
          if (show_debug_msg != MPLS_END)
            {
              /* Disable */
              show_debug_msg = MPLS_END;
              
              PRINTK_DEBUG ("Disabled logging of DEBUG messages for "
                            "MPLS module\n");
            }
        }

      if (req->msg_type & KERN_MSG_NOTICE)
        {
          if (show_notice_msg != MPLS_END)
            {
              /* Disable */
              show_notice_msg = MPLS_END;
              
              PRINTK_DEBUG ("Disabled logging of NOTICE messages for "
                            "MPLS module\n");
            }
        }
    }
  
  return retval;
}

/* This function clears all statistical information for the MPLS Fwder. */
int
mpls_netlink_usock_stats_clear_handling (struct nlmsghdr *nlh)
{
  struct mpls_stat_clear_handle *req = NLMSG_DATA(nlh);

  /* Make sure there is something in REQ */
  MPLS_ASSERT(req != NULL);
  
  /* Only handling this from configuration utility */
  MPLS_ASSERT (req->protocol == APN_PROTO_NSM);

  switch (req->type)
    {
    case MPLS_STAT_CLEAR_TYPE_ALL:
      PRINTK_DEBUG ("Clearing ALL statistics for MPLS...\n");
      mpls_fib_handle_clear_stats (fib_handle);
      mpls_ftn_entry_clear_stats (fib_handle, NULL);
      mpls_ilm_entry_clear_stats (fib_handle, NULL);
      break;
    case MPLS_STAT_CLEAR_TYPE_TOP:
      PRINTK_DEBUG ("Clearing TOP LEVEL statistics for MPLS...\n");
      mpls_fib_handle_clear_stats (fib_handle);
      break;
    case MPLS_STAT_CLEAR_TYPE_FTN:
      PRINTK_DEBUG ("Clearing FTN statistics for MPLS...\n");
      mpls_ftn_entry_clear_stats (fib_handle, NULL);
      break;
    case MPLS_STAT_CLEAR_TYPE_ILM:
      PRINTK_DEBUG ("Clearing ILM statistics for MPLS...\n");
      mpls_ilm_entry_clear_stats (fib_handle, NULL);
      break;
    default:
      PRINTK_WARNING ("Invalid TYPE received in CLEAR STATISTICS request "
                      "for MPLS\n");
      break;
    }

  return 0;
}


int
mpls_netlink_usock_bypass_tunnel_send (struct nlmsghdr *nlh)
{
  char *data = NLMSG_DATA(nlh);
  uint32  ftn_index, btm_id, total_len, seq_num, pkt_len, flags;

  /* The first field is the if_index. the is label. */  
  memcpy (&ftn_index, data, sizeof (uint32));
  data += sizeof (uint32);

  memcpy (&btm_id, data, sizeof (uint32));
  data += sizeof (uint32);

  memcpy (&flags, data, sizeof (uint32));
  data += sizeof (uint32);

  memcpy (&total_len, data, sizeof (uint32));
  data += sizeof (uint32);

  memcpy (&seq_num, data, sizeof (uint32));
  data += sizeof (uint32);

  memcpy (&pkt_len, data, sizeof (uint32));
  data += sizeof (uint32);

  /* packet has segments */
  if (flags & RSVP_CTRL_PKT_SEG)
    {
      if (total_len < pkt_len)
        {
          memset (&bt_msg, 0, sizeof (struct bypass_tunnel_msg));
          PRINTK_DEBUG ("Receive inconsistent bypass-tunnel-message. \n");
          PRINTK_DEBUG ("Total_len %lu pkt_len %lu. \n", total_len, pkt_len);
          PRINTK_DEBUG ("Drop bypass-tunnel-message. \n");
          return 0;
        }

      /* First segment */
      if (flags & RSVP_CTRL_PKT_BEGIN)
        {
          memset (&bt_msg, 0, sizeof (struct bypass_tunnel_msg));
          bt_msg.ftn_index = ftn_index;
          bt_msg.btm_id = btm_id;
          bt_msg.flags = flags;
          bt_msg.total_len = total_len;
          bt_msg.seq_num = seq_num;
          bt_msg.pkt_len = pkt_len;
          bt_msg.pkt = kmalloc (total_len, GFP_ATOMIC);
          memcpy (bt_msg.pkt, data, pkt_len);
          bt_msg.ptr = bt_msg.pkt + pkt_len;
          PRINTK_DEBUG ("bt_msg.btm_id = %lu \n", 
                        bt_msg.btm_id);
          PRINTK_DEBUG ("bt_msg.ftn_index = %lu  \n", 
                        bt_msg.ftn_index);
          PRINTK_DEBUG ("bt_msg.total_len = %lu \n", 
                        bt_msg.total_len);
          PRINTK_DEBUG ("bt_msg.seq_num = %lu  \n", 
                        bt_msg.seq_num);
          return 0;
        }

      /* Check for inconsistent segment */
      if (bt_msg.btm_id != btm_id ||
          bt_msg.ftn_index != ftn_index ||
          bt_msg.total_len != total_len ||
          (bt_msg.seq_num != (seq_num - 1)) ||
          ((bt_msg.ptr - bt_msg.pkt + pkt_len) > total_len))
        {
          PRINTK_DEBUG ("Receive inconsistent bypass-tunnel-message. \n");
          PRINTK_DEBUG ("bt_msg.btm_id = %lu btm_id = %lu. \n", 
                        bt_msg.btm_id, btm_id);
          PRINTK_DEBUG ("bt_msg.ftn_index = %lu ftn_index = %lu. \n", 
                        bt_msg.ftn_index, ftn_index);
          PRINTK_DEBUG ("bt_msg.total_len = %lu total_len = %lu. \n", 
                        bt_msg.total_len, total_len);
          PRINTK_DEBUG ("bt_msg.seq_num = %lu seq_num = %lu. \n", 
                        bt_msg.seq_num, seq_num);
          PRINTK_DEBUG ("Drop bypass-tunnel-message. \n");
          memset (&bt_msg, 0, sizeof (struct bypass_tunnel_msg));
          return 0;
        }

      bt_msg.seq_num = seq_num;
      memcpy (bt_msg.ptr, data, pkt_len);
      bt_msg.ptr = bt_msg.ptr + pkt_len;

      /* Last segment */
      if (flags & RSVP_CTRL_PKT_END)
        {
          /* Data now points to the start of the MPLS packet */
          /* Now need to create the sk_buff add the IP Header, MPLS Header and
             send the packet to the kernel */
          mpls_send_pkt_over_bypass (bt_msg.ftn_index, bt_msg.total_len,
                                     bt_msg.pkt);
          PRINTK_DEBUG ("mpls_send_pkt_over_bypass\n");
          PRINTK_DEBUG ("total_len = %lu, ftn_index = %lu\n", 
                         bt_msg.total_len, bt_msg.ftn_index); 
          /* Reset bt_msg buffer. */
          memset (&bt_msg, 0, sizeof (struct bypass_tunnel_msg));
          return 0;
        }

      return 0;
    } 

  /* Data now points to the start of the MPLS packet */
  /* Now need to create the sk_buff add the IP Header, MPLS Header and
     send the packet to the kernel */
  mpls_send_pkt_over_bypass (ftn_index, pkt_len, data);
  PRINTK_DEBUG ("mpls_send_pkt_over_bypass pkt_len = %lu, ftn_index = %lu", 
                 pkt_len, ftn_index); 

  return 0;
}

int
mpls_netlink_usock_oam_send (struct nlmsghdr *nlh)
{
  unsigned char *data = NLMSG_DATA(nlh);
  uint32  ftn_index, vrf_id, ttl, pkt_len, flags;

  /* The first field is the if_index. the is label. */  
  memcpy (&ftn_index, data, sizeof (uint32));
  data += sizeof (uint32);

  memcpy (&vrf_id, data, sizeof (uint32));
  data += sizeof (uint32);

  memcpy (&flags, data, sizeof (uint32));
  data += sizeof (uint32);

  memcpy (&ttl, data, sizeof (uint32));
  data += sizeof (uint32);

  memcpy (&pkt_len, data, sizeof (uint32));
  data += sizeof (uint32);

  /* Data now points to the start of the MPLS packet */
  /* Now need to create the sk_buff add the IP Header, MPLS Header and
     send the packet to the kernel */

  mpls_send_pkt_for_oam (ftn_index, pkt_len, data, vrf_id, flags, ttl);
  
  PRINTK_DEBUG ("mpls_send_pkt_for_oam pkt_len = %lu, ftn_index = %lu", 
                 pkt_len, ftn_index); 

  return 0;
}

int
mpls_netlink_usock_oam_vc_send (struct nlmsghdr *nlh)
{
  char *data = NLMSG_DATA(nlh);
  uint32  if_index, vc_id, ttl, pkt_len, flags, cc_type, cc_input;

  /* The first field is the if_index. the is label. */  
  memcpy (&if_index, data, sizeof (uint32));
  data += sizeof (uint32);

  memcpy (&vc_id, data, sizeof (uint32));
  data += sizeof (uint32);

  memcpy (&flags, data, sizeof (uint32));
  data += sizeof (uint32);

  memcpy (&ttl, data, sizeof (uint32));
  data += sizeof (uint32);

  memcpy (&cc_type, data, sizeof (uint32));
  data += sizeof (uint32);

  memcpy (&cc_input, data, sizeof (uint32));
  data += sizeof (uint32);

  memcpy (&pkt_len, data, sizeof (uint32));
  data += sizeof (uint32);

  /* Data now points to the start of the MPLS packet */
  /* Now need to create the sk_buff add the IP Header, MPLS Header and
     send the packet to the kernel */
  mpls_send_pkt_for_oam_vc (vc_id, pkt_len, data, if_index, 
                            cc_type, cc_input,
                            flags, ttl);
  
  return 0;
}
