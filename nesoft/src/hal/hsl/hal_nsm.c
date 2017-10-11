/* Copyright (C) 2004  All Rights Reserved. */

#include "pal.h"
#include "lib.h"
#include "tlv.h"

#include "hal_incl.h"
#include "hal_debug.h"
#include "hal_msg.h"
#include "hal_netlink.h"
#include "hal_comm.h"
#include "hal_nsm.h"

#include "nsm/nsmd.h"

#ifdef HAVE_L3
#include "nsm/nsm_connected.h"
#include "nsm/rib/nsm_redistribute.h"
#endif /* HAVE_L3 */
#include "nsm/nsm_interface.h"
#include "nsm/nsm_server.h"
#include "nsm/nsm_router.h"

#ifdef HAVE_ONMD
#include "nsm_client.h"
#include "nsm_message.h"
#include "nsm/oam/nsm_oam.h"
#endif /* HAVE_ONMD */
#ifdef HAVE_GMPLS
#include "nsm/gmpls/nsm_gmpls.h"
#endif /* HAVE_GMPLS */

extern struct lib_globals *hal_zg;
struct hal_nsm_callbacks *nsm_cb;

/*
   Callbacks to be implemented by NSM.
*/

/* CPU Database and stacking related  */
static int
hal_msg_encode_cpu (u_char **pnt, u_int32_t *size, u_char *msg,
                    int msg_type)
{
   if(*size < sizeof(int))
    return HAL_MSG_PKT_TOO_SMALL;

   switch(msg_type)
   {
      case HAL_MSG_CPU_GET_NUM:
        *size = 0;
        break;

      default:
        if(*size)
            memcpy(*pnt, msg, *size);
        break;
   }
   return *size;
}



static int
_hal_cpu_get_cb (struct hal_nlmsghdr *h, void *data)
{
  u_char       *pnt;
  int          size;

  size = h->nlmsg_len - HAL_NLMSGHDR_SIZE;
  pnt = HAL_NLMSG_DATA (h);

  memcpy((char *)data, pnt, size);
  return 0;
}


int hal_cpu_info_set (int command, u_char *opaque,
    unsigned int *ret_info, unsigned int sz)
{
  int ret;
  struct hal_nlmsghdr *nlh;
  u_char *pnt;
  int size, msgsz;
  struct
  {
    struct hal_nlmsghdr nlh;
    u_char buf[10];
  } req;

  /* Set message. */
  pnt = (u_char *) req.buf;
  size = sizeof (req.buf);
  msgsz = hal_msg_encode_cpu (&pnt, (u_int32_t *)&sz, opaque, command);
  if (msgsz < 0)
    return -1;

  /* Set header. */
  nlh = &req.nlh;
  nlh->nlmsg_len = HAL_NLMSG_LENGTH (msgsz);
  nlh->nlmsg_flags = HAL_NLM_F_CREATE | HAL_NLM_F_REQUEST | HAL_NLM_F_ACK;
  nlh->nlmsg_type = command;
  nlh->nlmsg_seq = ++hallink_cmd.seq;

  /* Send message and parse response */
  ret = hal_talk (&hallink_cmd, nlh, _hal_cpu_get_cb, (void *)ret_info);
  if (ret < 0)
  {
    return ret;
  }

  return 0;
}

int hal_cpu_info_get (unsigned int command, u_char *opaque,
           u_char *ret_info, unsigned int sz)
{
  int ret;
  struct hal_nlmsghdr *nlh;
  u_char *pnt;
  int size, msgsz;
  struct
  {
    struct hal_nlmsghdr nlh;
    char  cpu_info [160];
  } req;

  /* Set message. */
  pnt = (u_char *) &req.cpu_info;
  size = sizeof (req.cpu_info);

  msgsz = hal_msg_encode_cpu (&pnt, (u_int32_t *)&sz, opaque, command);
  if (msgsz < 0)
    return -1;

  /* Set header. */
  nlh = &req.nlh;
  nlh->nlmsg_len = HAL_NLMSG_LENGTH (msgsz);
  nlh->nlmsg_flags = HAL_NLM_F_CREATE | HAL_NLM_F_REQUEST | HAL_NLM_F_ACK;
  nlh->nlmsg_type = command;
  nlh->nlmsg_seq = ++hallink_cmd.seq;

  /* Send message and parse response */
  ret = hal_talk (&hallink_cmd, nlh, _hal_cpu_get_cb, (void *)ret_info);
  if (ret < 0)
  {
    return ret;
  }
  return 0;
}

int hal_get_num_cpu_entries(unsigned int *num_cpu)
{
   hal_cpu_info_get (HAL_MSG_CPU_GET_NUM, (u_char *)NULL,
          (unsigned char *)num_cpu, sizeof(int));
   return 0;
}

int hal_stacking_set_entry_master (unsigned char *mac)
{
   hal_cpu_info_set (HAL_MSG_CPU_SET_MASTER, mac, NULL, 6);
   return 0;
}

int hal_get_local_cpu_entry (unsigned char *cpu_entry)
{
   hal_cpu_info_get (HAL_MSG_CPU_GET_LOCAL, (u_char *)NULL,
        (unsigned char *)  cpu_entry, 6);
   return 0;
}

int hal_get_master_cpu_entry (unsigned char *cpu_entry)
{
   hal_cpu_info_get (HAL_MSG_CPU_GET_MASTER, (u_char *)NULL,
        (unsigned char *)  cpu_entry, 6);
   return 0;
}

int hal_get_index_cpu_entry (unsigned int entry_num,
                             unsigned char *cpu_entry)
{
   hal_cpu_info_get (HAL_MSG_CPU_GETDB_INDEX, (u_char *)&entry_num,
        (unsigned char *) cpu_entry, 6);
   return 0;
}

int hal_get_index_dump_cpu_entry (unsigned int entry_num,
       unsigned char *cpu_entry)
{
   hal_cpu_info_get (HAL_MSG_CPU_GETDUMP_INDEX, (u_char *)&entry_num,
        (unsigned char *) cpu_entry, 104);
   return 0;
}

/*
   Get parameters.
*/
int
hal_get_params_cb (struct hal_nlmsghdr *h, struct hal_msg_if *msg)
{
  struct interface *ifp;

  if (h->nlmsg_type == HAL_MSG_IF_UPDATE)
    {
      ifp = ifg_lookup_by_index (&hal_zg->ifg, msg->ifindex);
      if (!ifp)
        {
          zlog_err (hal_zg,
                    "Interface not found for ifindex %d.",msg->ifindex);
          return -1;
        }
    }
  else
    ifp = ifg_lookup_by_name (&hal_zg->ifg, msg->name);

  if (ifp)
    {
      if (CHECK_CINDEX (msg->cindex, HAL_MSG_CINDEX_IF_MTU))
        ifp->mtu = msg->mtu;
      if (CHECK_CINDEX (msg->cindex, HAL_MSG_CINDEX_IF_DUPLEX))
        ifp->duplex = msg->duplex;
      if (CHECK_CINDEX (msg->cindex, HAL_MSG_CINDEX_IF_ARP_AGEING_TIMEOUT))
        ifp->arp_ageing_timeout = msg->arp_ageing_timeout;
      if (CHECK_CINDEX (msg->cindex, HAL_MSG_CINDEX_IF_AUTONEGO))
        ifp->autonego = msg->autonego;
      if (CHECK_CINDEX (msg->cindex, HAL_MSG_CINDEX_IF_METRIC))
        ifp->metric = msg->metric;
      if (CHECK_CINDEX (msg->cindex, HAL_MSG_CINDEX_IF_FLAGS))
        ifp->flags = msg->flags;
      if (CHECK_CINDEX (msg->cindex, HAL_MSG_CINDEX_IF_BANDWIDTH))
        ifp->bandwidth = msg->bandwidth;
      if (CHECK_CINDEX (msg->cindex, HAL_MSG_CINDEX_IF_HW))
        pal_mem_cpy (ifp->hw_addr, msg->hw_addr, HAL_HW_LENGTH);
      if (CHECK_CINDEX (msg->cindex, HAL_MSG_CINDEX_IF_IF_FLAGS))
        {
          if (CHECK_FLAG (msg->if_flags, HAL_IF_INTERNAL))
            SET_FLAG (ifp->status, IF_HIDDEN_FLAG);
        }
    }

  return 0;
}
#ifdef HAVE_EFM

/*
  Dying Gasp Update.

*/
int
hal_efm_dying_gasp_cb (struct hal_nlmsghdr *h)
{
  return nsm_efm_oam_process_local_event (NULL, NSM_EFM_OAM_DYING_GASP_EVENT,
                                          PAL_TRUE);
}


int
hal_efm_frame_period_window_expiry_cb(struct hal_nlmsghdr *h, 
                                struct hal_msg_efm_err_frame_secs_resp *msg)
{
  struct interface *ifp;
  
  ifp = ifg_lookup_by_index (&hal_zg->ifg, msg->index);
  if (!ifp)
  {
    zlog_err (hal_zg,
              "Interface not found for ifindex %d.",msg->index);
    return -1;
  }
  
  /* call nsm function to inform */
  nsm_efm_oam_process_period_error_event (ifp,
                                          NSM_EFM_SET_FRAME_PERIOD_ERROR,
                                          *(msg->err_frame_secs));
  
  return 0;
}
#endif /* HAVE_EFM */

/*
  New/Update interface.
*/
int
hal_new_link_cb (struct hal_nlmsghdr *h, struct hal_msg_if *msg)
{
  struct interface *ifp;
  struct nsm_if *zif;
  int type = NSM_IF_TYPE_L3;
  short orig_if_state;
  short new_if_state;

  if (h->nlmsg_type == HAL_MSG_IF_UPDATE)
    {
      ifp = ifg_lookup_by_index (&hal_zg->ifg, msg->ifindex);
      if (!ifp)
        {
          zlog_err (hal_zg,
                    "Interface not found for ifindex %d.",msg->ifindex);
          return -1;
        }
    }
  else
    ifp = ifg_lookup_by_name (&hal_zg->ifg, msg->name);
  if (ifp == NULL ||
      (! CHECK_FLAG (ifp->status, NSM_INTERFACE_ACTIVE)
       && ! CHECK_FLAG (ifp->status, NSM_INTERFACE_DELETE)))
    {
      if (ifp == NULL)
        ifp = ifg_get_by_name (&hal_zg->ifg, msg->name);
        if (ifp == NULL)
          return -1;

      ifp->ifindex = msg->ifindex;
      if (CHECK_CINDEX (msg->cindex, HAL_MSG_CINDEX_IF_MTU))
        ifp->mtu = msg->mtu;
      if (CHECK_CINDEX (msg->cindex, HAL_MSG_CINDEX_IF_DUPLEX))
        ifp->duplex = msg->duplex;
      if (CHECK_CINDEX (msg->cindex, HAL_MSG_CINDEX_IF_ARP_AGEING_TIMEOUT))
        ifp->arp_ageing_timeout = msg->arp_ageing_timeout;
      if (CHECK_CINDEX (msg->cindex, HAL_MSG_CINDEX_IF_AUTONEGO))
        ifp->autonego = msg->autonego;
      if (CHECK_CINDEX (msg->cindex, HAL_MSG_CINDEX_IF_METRIC))
        ifp->metric = msg->metric;
      if (CHECK_CINDEX (msg->cindex, HAL_MSG_CINDEX_IF_FLAGS))
        ifp->flags = msg->flags;
      if (CHECK_CINDEX (msg->cindex, HAL_MSG_CINDEX_IF_BANDWIDTH))
        ifp->bandwidth = msg->bandwidth;
      if (CHECK_CINDEX (msg->cindex, HAL_MSG_CINDEX_IF_IF_FLAGS))
        {
          if (CHECK_FLAG (msg->if_flags, HAL_IF_INTERNAL))
            SET_FLAG (ifp->status, IF_HIDDEN_FLAG);
        }
      if (CHECK_CINDEX (msg->cindex, HAL_MSG_CINDEX_IF_HW))
        {
          if (pal_strncmp (ifp->name, "vlan", 4) == 0)
            {
              ifp->hw_type = IF_TYPE_VLAN;
            }
          else
            {
              if (msg->hw_type == HAL_IF_TYPE_ETHERNET)
                ifp->hw_type = IF_TYPE_ETHERNET;
              else if (msg->hw_type == HAL_IF_TYPE_LOOPBACK)
                ifp->hw_type = IF_TYPE_LOOPBACK;
              else if (msg->hw_type == HAL_IF_TYPE_VLAN)
                ifp->hw_type = IF_TYPE_VLAN;
              else if (msg->hw_type == HAL_IF_TYPE_TUNNEL)
                ifp->hw_type = IF_TYPE_APNP;
              else
                ifp->hw_type = IF_TYPE_UNKNOWN;
            }
          ifp->hw_addr_len = msg->hw_addr_len;
          pal_mem_cpy (ifp->hw_addr, msg->hw_addr, msg->hw_addr_len);
        }

      /* Add to database. */
#ifdef HAVE_GMPLS
      if (ifp->gifindex == 0)
        {
          ifp->gifindex = hal_zg->gifindex++;
        }
#endif /* HAVE_GMPLS */
      ifg_table_add (&hal_zg->ifg, ifp->ifindex, ifp);

      /* Interface type .*/
      if (msg->type == HAL_IF_ROUTER_PORT)
        ifp->type = NSM_IF_TYPE_L3;
      else if (msg->type == HAL_IF_SWITCH_PORT)
        ifp->type = NSM_IF_TYPE_L2;

      /* Notify PMs of new interface. */
      if((nsm_cb) && (nsm_cb->nsm_if_add_update_cb))
        nsm_cb->nsm_if_add_update_cb (ifp, (fib_id_t)FIB_ID_MAIN);
    }
  else
    {
      cindex_t cindex = 0;
      u_char param_changed = 0;

      zif = ifp->info;

      if (zif != NULL)
        zif->nsm_if_link_changed = PAL_TRUE;

      /* Interface type .*/
      if (msg->type == HAL_IF_ROUTER_PORT)
        type = NSM_IF_TYPE_L3;
      else if (msg->type == HAL_IF_SWITCH_PORT)
        type = NSM_IF_TYPE_L2;

      if (ifp->type != type)
        {
          ifp->type = type;
          if(type == NSM_IF_TYPE_L3)
            NSM_INTERFACE_SET_L3_MEMBERSHIP (ifp);
          else if(type == NSM_IF_TYPE_L2)
            NSM_INTERFACE_SET_L2_MEMBERSHIP (ifp);

          zif = ifp->info;
          if (zif)
            {
              zif->type = ifp->type;
            }
        }

      if (ifp->ifindex != msg->ifindex)
        {
          /* Update the interface index.  */
          if((nsm_cb) && (nsm_cb->nsm_if_ifindex_update_cb))
            nsm_cb->nsm_if_ifindex_update_cb (ifp, msg->ifindex);
          
          param_changed = 1;
        }

      if (CHECK_CINDEX (msg->cindex, HAL_MSG_CINDEX_IF_MTU))
        {
          if (ifp->mtu != msg->mtu)
            {
              ifp->mtu = msg->mtu;
              param_changed = 1;
              NSM_SET_CTYPE (cindex, NSM_LINK_CTYPE_MTU);
            }
        }
      if (CHECK_CINDEX (msg->cindex, HAL_MSG_CINDEX_IF_DUPLEX))
        {
          if (ifp->duplex != msg->duplex)
            {
              ifp->duplex = msg->duplex;
              param_changed = 1;
              NSM_SET_CTYPE (cindex, NSM_LINK_CTYPE_DUPLEX);
            }
        }
      if (CHECK_CINDEX (msg->cindex, HAL_MSG_CINDEX_IF_ARP_AGEING_TIMEOUT))
        {
          if (ifp->arp_ageing_timeout != msg->arp_ageing_timeout)
            {
              ifp->arp_ageing_timeout = msg->arp_ageing_timeout;
              param_changed = 1;
              NSM_SET_CTYPE (cindex, NSM_LINK_CTYPE_ARP_AGEING_TIMEOUT);
            }
        }
      if (CHECK_CINDEX (msg->cindex, HAL_MSG_CINDEX_IF_AUTONEGO))
        {
          if (ifp->autonego != msg->autonego)
            {
              ifp->autonego = msg->autonego;
              param_changed = 1;
              NSM_SET_CTYPE (cindex, NSM_LINK_CTYPE_AUTONEGO);
            }
        }
      if (CHECK_CINDEX (msg->cindex, HAL_MSG_CINDEX_IF_METRIC))
        {
          if (ifp->metric != msg->metric)
            {
              ifp->metric = msg->mtu;
              param_changed = 1;
              NSM_SET_CTYPE (cindex, NSM_LINK_CTYPE_METRIC);
            }
        }
      if (CHECK_CINDEX (msg->cindex, HAL_MSG_CINDEX_IF_BANDWIDTH))
        {
          if (ifp->bandwidth != msg->bandwidth)
            {
              ifp->bandwidth = msg->bandwidth;
              param_changed = 1;
              NSM_SET_CTYPE (cindex, NSM_LINK_CTYPE_BANDWIDTH);
            }
        }
      if (CHECK_CINDEX (msg->cindex, HAL_MSG_CINDEX_IF_HW))
        {
          if (pal_strncmp (ifp->name, "vlan", 4) != 0)
            {
              if (msg->hw_type == HAL_IF_TYPE_ETHERNET)
                ifp->hw_type = IF_TYPE_ETHERNET;
              else if (msg->hw_type == HAL_IF_TYPE_LOOPBACK)
                ifp->hw_type = IF_TYPE_LOOPBACK;
              else
                ifp->hw_type = IF_TYPE_UNKNOWN;
            }

          if ((ifp->hw_addr_len != msg->hw_addr_len) ||
              (pal_mem_cmp(ifp->hw_addr, msg->hw_addr, msg->hw_addr_len)))
            {
              ifp->hw_addr_len = msg->hw_addr_len;
              pal_mem_cpy (ifp->hw_addr, msg->hw_addr, msg->hw_addr_len);
              param_changed = 1;
              NSM_SET_CTYPE (cindex, NSM_LINK_CTYPE_HW_ADDRESS);
            }
        }

      if (CHECK_CINDEX (msg->cindex, HAL_MSG_CINDEX_IF_IF_FLAGS))
        {
          if (CHECK_FLAG (msg->if_flags, HAL_IF_INTERNAL))
            SET_FLAG (ifp->status, IF_HIDDEN_FLAG);

          param_changed = 1;
          NSM_SET_CTYPE (cindex, NSM_LINK_CTYPE_STATUS);
        }

      /* Update interface flags */
      if (CHECK_CINDEX (msg->cindex, HAL_MSG_CINDEX_IF_FLAGS))
        {
      /* Preserve original interface up/down state. */
      orig_if_state = (if_is_up (ifp) && if_is_running (ifp));

          ifp->flags = msg->flags;

          /* New interface up/down state. */
          new_if_state = (if_is_up (ifp) && if_is_running (ifp));
          if(new_if_state)
            {
              if (orig_if_state != new_if_state)
                {
                  if((nsm_cb) && (nsm_cb->nsm_if_up_cb))
                    nsm_cb->nsm_if_up_cb (ifp);
                }
              else if(param_changed)
                {
                  if((nsm_cb) && (nsm_cb->nsm_server_if_up_update_cb))
                  nsm_cb->nsm_server_if_up_update_cb (ifp, cindex);
                }
            }
          else
            {
              if (orig_if_state != new_if_state)
                {
                 if((nsm_cb) && (nsm_cb->nsm_if_down_cb))
                    nsm_cb->nsm_if_down_cb (ifp);
                }
              else if(param_changed)
                {
                  if((nsm_cb) && (nsm_cb->nsm_server_if_down_update_cb))
                    nsm_cb->nsm_server_if_down_update_cb (ifp, cindex);
                }
            }
        }
    }

  return 0;
}

/*
  Delete interface.
*/
int
hal_del_link_cb (struct hal_msg_if *msg)
{
  struct interface *ifp;

  ifp = ifg_lookup_by_index (&hal_zg->ifg, msg->ifindex);
  if (ifp == NULL)
    {
      return 0;
    }

  if((nsm_cb) && (nsm_cb->nsm_if_delete_update_cb))
    nsm_cb->nsm_if_delete_update_cb (ifp);

  return 0;
}
#ifdef  HAVE_L2
/*
  Stp refresh interface.
*/
int
hal_stp_refresh_link_cb (struct hal_msg_if *msg)
{
  struct interface *ifp;

  ifp = ifg_lookup_by_name (&hal_zg->ifg, msg->name);
  if ((ifp == NULL) ||
      (! CHECK_FLAG (ifp->status, NSM_INTERFACE_ACTIVE)) ||
      (ifp->type != NSM_IF_TYPE_L2 ))
    {
      return RESULT_ERROR;
    }

    if((nsm_cb) && (nsm_cb->nsm_bridge_if_send_state_sync_req_wrap_cb))
      nsm_cb->nsm_bridge_if_send_state_sync_req_wrap_cb (ifp);

  return 0;
}
#endif /* HAVE_L2 */

/*
  Set ECMP max paths.
*/
int
hal_ecmp_cb (int ecmp)
{
  if((nsm_cb) && (nsm_cb->nsm_set_ecmp_cb))
    nsm_cb->nsm_set_ecmp_cb(ecmp);

  return 0;
}
#ifdef HAVE_L3
int
hal_add_ipv4_addr(struct hal_msg_if_ipv4_addr *resp)
{
  struct pal_in4_addr b_addr;
  struct pal_in4_addr addr;
  struct interface *ifp;

  /* Check does this interface exist or not. */
  ifp = ifg_lookup_by_index (&hal_zg->ifg, resp->ifindex);
  if (ifp == NULL)
    {
      zlog_err (hal_zg,
                "Interface not found for ifindex %d.",resp->ifindex);
      return RESULT_ERROR;
    }

  /* Get interface address */
  addr.s_addr = resp->addr.s_addr;

  /* Get broadcast address . */
  get_broadcast_addr(&addr, resp->ipmask,&b_addr);

  if((nsm_cb) && (nsm_cb->nsm_connected_add_ipv4_cb))
    nsm_cb->nsm_connected_add_ipv4_cb (ifp, 0, &addr,resp->ipmask, &b_addr);

  return 0;
}

int
hal_del_ipv4_addr(struct hal_msg_if_ipv4_addr *resp)
{
  struct pal_in4_addr b_addr;
  struct pal_in4_addr addr;
  struct interface *ifp;

  /* Check does this interface exist or not. */
  ifp = ifg_lookup_by_index (&hal_zg->ifg, resp->ifindex);
  if (ifp == NULL)
    {
      zlog_err (hal_zg,
                "Interface not found for ifindex %d.",resp->ifindex);
      return RESULT_ERROR;
    }

  /* Get interface address */
  addr.s_addr = resp->addr.s_addr;

  /* Get broadcast address . */
  get_broadcast_addr(&addr, resp->ipmask,&b_addr);
  
  if((nsm_cb) && (nsm_cb->nsm_connected_delete_ipv4_cb))
    nsm_cb->nsm_connected_delete_ipv4_cb (ifp, 0, &addr,resp->ipmask,&b_addr);

  return 0;
}

#ifdef HAVE_IPV6

int
hal_add_ipv6_addr(struct hal_msg_if_ipv6_addr *resp)
{
  struct pal_in6_addr b_addr;
  struct pal_in6_addr addr;
  struct interface *ifp;


  /* Check does this interface exist or not. */
  ifp = ifg_lookup_by_index (&hal_zg->ifg, resp->ifindex);
  if (ifp == NULL)
    {
      zlog_err (hal_zg,
                "Interface not found for ifindex %d.",resp->ifindex);
      return RESULT_ERROR;
    }

  /* Get interface address */
  pal_mem_cpy(&addr, &resp->addr, sizeof (struct pal_in6_addr));

  /* Get broadcast address . */
  get_broadcast_addr6(&addr, resp->ipmask,&b_addr);

  if ((nsm_cb) && (nsm_cb->nsm_connected_add_ipv6_cb) )
   nsm_cb->nsm_connected_add_ipv6_cb (ifp, &addr,resp->ipmask, &b_addr);

  return 0;
}

int
hal_del_ipv6_addr(struct hal_msg_if_ipv6_addr *resp)
{
  struct pal_in6_addr b_addr;
  struct pal_in6_addr addr;
  struct interface *ifp;

  /* Check does this interface exist or not. */
  ifp = ifg_lookup_by_index (&hal_zg->ifg, resp->ifindex);
  if (ifp == NULL)
    {
      zlog_err (hal_zg,
                "Interface not found for ifindex %d.",resp->ifindex);
      return RESULT_ERROR;
    }

  /* Get interface address */
  pal_mem_cpy(&addr, &resp->addr, sizeof (struct pal_in6_addr));

  /* Get broadcast address . */
  get_broadcast_addr6(&addr, resp->ipmask,&b_addr);

  if ((nsm_cb) && (nsm_cb->nsm_connected_delete_ipv6_cb))
    nsm_cb->nsm_connected_delete_ipv6_cb (ifp, &addr,resp->ipmask,&b_addr);
  return 0;
}
#endif /* HAVE_IPV6 */
#endif /* HAVE_L3   */

void
hal_nsm_lib_cb_register(struct hal_nsm_callbacks *cb)
{
  nsm_cb = cb;
}

