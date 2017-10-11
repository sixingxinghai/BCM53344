/* Copyright (C) 2004  All Rights Reserved. */

#include "pal.h"
#ifdef HAVE_HAL
#include "hal_incl.h"
#endif /* HAVE_HAL */

#include "lib.h"
#include "if.h"
#include "cli.h"
#include "table.h"
#include "linklist.h"
#include "snprintf.h"
#include "bitmap.h"
#include "nsm_message.h"
#include "avl_tree.h"

#include "nsmd.h"
#include "nsm_server.h"
#include "nsm_interface.h"
#include "nsm_api.h"
#include "nsm_router.h"
#include "nsm_debug.h"
#include "nsm/nsm_fea.h"
#include "nsm_lacp.h"
#include "nsm_static_aggregator_cli.h"
#ifdef HAVE_L2
#ifdef HAVE_VLAN
#include "nsm_vlan.h"
#include "nsm_bridge.h"
#endif /* HAVE_VLAN */
#endif /* HAVE_L2 */

#ifdef HAVE_HA
#include "nsm_cal.h"
#include "nsm_lacp_cal.h"
#include "lib_cal.h"
#endif /* HAVE_HA */

#ifdef HAVE_ONMD
#include "nsm_oam.h"
#endif /* HAVE_ONMD */

#ifdef HAVE_PROVIDER_BRIDGE
#include "nsm_pro_vlan.h"
#endif /* HAVE_PROVIDER_BRIDGE */

/* API for cli */

/* Send aggregator configuration to LACP */
int 
nsm_lacp_send_aggregator_config (u_int32_t ifindex, u_int32_t key, 
                                 char activate, int add_agg)
{
  struct nsm_msg_lacp_aggregator_config msg;
  struct nsm_server_client *nsc = NULL;
  struct nsm_server_entry *nse = NULL;
  int i, len;

  pal_mem_set (&msg, 0, sizeof (struct nsm_msg_lacp_aggregator_config));
  msg.key = key;
  msg.ifindex = ifindex;
  msg.chan_activate = activate;
  msg.add_agg = add_agg;

  NSM_SERVER_CLIENT_LOOP (ng, i, nsc, nse)
    if (nsm_service_check (nse, NSM_SERVICE_INTERFACE) &&
        nse->service.protocol_id == APN_PROTO_LACP)
      {
        nse->send.pnt = nse->send.buf + NSM_MSG_HEADER_SIZE;
        nse->send.size = nse->send.len - NSM_MSG_HEADER_SIZE;
        len = nsm_encode_lacp_send_agg_config (&nse->send.pnt,
                                               &nse->send.size, &msg);
        nsm_server_send_message (nse, 0, 0, NSM_MSG_LACP_AGGREGATOR_CONFIG,
                                 0, len);
        return 0;
      }
  return 0;

}

/* Aggregate add API. */
int 
nsm_lacp_add_aggregator (struct nsm_master *nm, char *ifname, 
                         u_char type, u_char agg_type)
{
  struct interface *agg_ifp;
  struct nsm_if *agg_zif;

  agg_ifp = ifg_lookup_by_name (&nzg->ifg, ifname);
  if (! agg_ifp)
    {
      agg_ifp = ifg_get_by_name (&nzg->ifg, ifname);
      SET_FLAG (agg_ifp->status, NSM_INTERFACE_MAPPED);
#ifdef HAVE_INTERFACE_NAME_MAPPING
      pal_strncpy (agg_ifp->orig, ifname, INTERFACE_NAMSIZ);
#endif /* HAVE_INTERFACE_NAME_MAPPING. */

      agg_ifp->hw_type = IF_TYPE_AGGREGATE;
      agg_ifp->type = type;
      agg_ifp->vr = nm->vr;
      listnode_add (agg_ifp->vr->ifm.if_list, agg_ifp);
      agg_ifp->vrf = apn_vrf_lookup_default (nm->vr);

      nsm_if_add (agg_ifp);

      agg_zif = (struct nsm_if *)agg_ifp->info;
      agg_zif->agg_config_type = agg_type;
      agg_zif->type = type;
      agg_zif->agg.type = NSM_IF_AGGREGATOR;
      agg_zif->agg.mismatch_list = list_new ();
      agg_zif->agg.psc  = NSM_LACP_PSC_DEFAULT;
      /* the aggregator interface has been created in nsm only */
      agg_zif->agg_oper_state = PAL_FALSE;

      if (type == NSM_IF_TYPE_L2)
        NSM_INTERFACE_SET_L2_MEMBERSHIP(agg_ifp);
      else
        NSM_INTERFACE_SET_L3_MEMBERSHIP(agg_ifp);

      /* Increment the aggregator count*/
      ng->agg_cnt++;
#ifdef HAV_HA
      nsm_cal_modify_nsm_globals (ng);
      lib_cal_modify_interface (ng, agg_ifp);
      nsm_lacp_cal_modify_nsm_if_lacp (agg_zif);
#endif /* HAVE_HA */
    }

  return 0;
}

/* Aggregator member add API. */
int
nsm_lacp_add_aggregator_member (struct nsm_master *nm, 
                                struct interface *agg_ifp,
                                struct interface *mem_ifp,
                                u_char agg_type,
                                u_int32_t key)
{
  struct nsm_if *agg_zif, *mem_zif;
  struct nsm_server_entry *nse;
  struct nsm_server_client *nsc;
#ifdef HAVE_SWFWDR
  int toggle_interface = 0;
#endif /* HAVE_SWFWDR */
  int i;
  
  agg_zif = (struct nsm_if *)agg_ifp->info;
  mem_zif = (struct nsm_if *)mem_ifp->info;

  if (! agg_zif || ! mem_zif)
    return -1;

  /* Add interface to aggregator member list. */
  if (! agg_zif->agg.info.memberlist && ! mem_zif->agg.info.parent)
    {
      agg_zif->agg.info.memberlist = list_new ();
      listnode_add (agg_zif->agg.info.memberlist, mem_ifp);

     
#ifdef HAVE_L2 
      if ( mem_zif->bridge )
        {
          pal_strncpy (agg_ifp->bridge_name , mem_zif->bridge->name, 
                       NSM_BRIDGE_NAMSIZ);
          nsm_bridge_api_set_port_state_all (nm, mem_zif->bridge->name, mem_ifp,
                                             NSM_BRIDGE_PORT_STATE_BLOCKING);
        } 
#endif /* HAVE_L2 */
    }
  else
    {
      if (mem_zif->agg.info.parent)
        return NSM_LACP_ERROR_CONFIGURED;
 
      if (agg_zif->agg.info.memberlist->count < NSM_MAX_AGGREGATOR_LINKS)
        listnode_add (agg_zif->agg.info.memberlist, mem_ifp);
    }

  /* Add aggregator as the aggregated parent. */
  mem_zif->agg.info.parent = agg_ifp;
  mem_zif->agg.type = NSM_IF_AGGREGATED;
  mem_zif->agg_config_type = agg_type;
  mem_zif->agg_oper_state = PAL_FALSE;
  mem_zif->opcode = NSM_LACP_AGG_ADD;
  mem_ifp->lacp_agg_key = key;


#ifdef HAVE_HA
  nsm_lacp_cal_create_agg_associate (mem_zif);
#endif /* HAVE_HA */
  /* Send interface delete to PMs. */
  NSM_SERVER_CLIENT_LOOP (ng, i, nsc, nse)
    if (nsm_service_check (nse, NSM_SERVICE_INTERFACE))
      {
        if (agg_type == AGG_CONFIG_STATIC)
          {
            if (NSM_INTERFACE_CHECK_MEMBERSHIP(mem_ifp, nse->service.protocol_id)
                && nse->service.protocol_id != APN_PROTO_ONM)
              {
                nsm_server_send_vr_unbind (nse, mem_ifp);
                nsm_server_send_link_delete (nse, mem_ifp, 1);
              }
          }
        else
          {
            if (NSM_INTERFACE_CHECK_MEMBERSHIP(mem_ifp, nse->service.protocol_id)
                && nse->service.protocol_id != APN_PROTO_LACP
                && nse->service.protocol_id != APN_PROTO_ONM)
              {
                nsm_server_send_vr_unbind (nse, mem_ifp);
                nsm_server_send_link_delete (nse, mem_ifp, 1);
              }

          }

        if (nsm_service_check (nse, NSM_SERVICE_INTERFACE) && 
            (nse->service.protocol_id == APN_PROTO_ONM))
          {
            /* Presently only to onmd this message is being sent which will
             * initmate that this port has been aggregate to this agg_ifp */
            if (mem_zif->bridge)
              {
                nsm_lacp_send_aggregate_member (mem_zif->bridge->name, mem_ifp,
                                                PAL_TRUE);
              }
          }
      }

#ifdef HAVE_SWFWDR

  /* Set the all multi flags for this member to receive lacp pdus */
  if (if_is_up(mem_ifp))
    toggle_interface = 1;

  /* If IFF_UP is not given port will be down in hardware */
  if (toggle_interface)
    nsm_fea_if_flags_set (mem_ifp, IFF_ALLMULTI | IFF_UP);
  else
    nsm_fea_if_flags_set (mem_ifp, IFF_ALLMULTI);

  nsm_fea_if_flags_get (mem_ifp);

  if (toggle_interface && (! if_is_running(mem_ifp)))
    nsm_if_flag_up_set (mem_ifp->vr->id, mem_ifp->name, PAL_TRUE);

#endif /* HAVE_SWFWDR */

#ifdef HAVE_HA
  lib_cal_modify_interface (nzg, mem_ifp);
  lib_cal_modify_interface (nzg, agg_ifp);
  nsm_lacp_cal_modify_nsm_if_lacp (mem_zif); 
#endif /* HAVE_HA */
  return 0;
}
#ifdef HAVE_VLAN
/* Function tells that the Mismatch was due to
 * Bandwdith or Duplex or some other Mismatch.
 */
int
nsm_lacp_check_link_mismatch (struct nsm_master *nm,
                              struct interface *first_child,
                              struct interface *candidate_ifp)
{
  int ret = NSM_LACP_ERROR_ADMIN_KEY_MISMATCH;
  struct nsm_bridge_port *br_port = NULL;
  struct nsm_if *nsmif = NULL;
  struct nsm_lacp_admin_key_element *ake = NULL;
  struct nsm_lacp_admin_key_element *tmp_ake = NULL;
  float32_t bandwidth = 0.0;
  int duplex = 0;

  if ((first_child == NULL) || (candidate_ifp == NULL))
    return ret;

  nsmif = candidate_ifp->info;

  if (nsmif && nsmif->switchport)
    br_port = nsmif->switchport;
  
  ake = nsm_lacp_admin_key_element_lookup (nm, first_child->lacp_admin_key);

  if (ake == NULL)
    return ret;

  /* Check if Bandwidth and Duplex are the
   * only differentiating Factor.
   */
  bandwidth = candidate_ifp->bandwidth;
  duplex = candidate_ifp->duplex;

  /* Make the Interface B/W and Duplex same as ake
   * Bandwidth and Duplex and check if tmp_ake returned
   * is same as ake.
   */
  if ((candidate_ifp->duplex != NSM_IF_FULL_DUPLEX) &&
      (ake->duplex != NSM_IF_FULL_DUPLEX))
    {
      candidate_ifp->bandwidth = ake->bandwidth;
      candidate_ifp->duplex = ake->duplex;
    }

  tmp_ake = nsm_lacp_admin_key_element_lookup_by_ifp (nm, candidate_ifp);

  /* Revert the Bandwidth & Duplex  Parameters. */
  candidate_ifp->bandwidth = bandwidth;
  candidate_ifp->duplex = duplex;

  if (tmp_ake == ake)
    {
      if ((candidate_ifp->bandwidth == ake->bandwidth) &&
          (candidate_ifp->duplex == ake->duplex))
        return RESULT_OK;
      else
        return NSM_LACP_ERROR_MISMATCH;
    }

  return ret;
}
#endif /* HAVE_VLAN */

 
/* API called fro channel-group command on interface */
int
nsm_lacp_api_add_aggregator_member (struct nsm_master *nm,
                                    struct interface *mem_ifp, u_int32_t key,
                                    char activate, bool_t notify_lacp,
                                    u_char agg_type)
{
  struct interface *agg_ifp;
  struct interface *tmp_ifp;
  struct nsm_if *mem_zif;
  struct nsm_if *agg_zif;
  char agg_name[INTERFACE_NAMSIZ];
  int ret;
  bool_t first_child = PAL_FALSE;
  int status = RESULT_OK;

  mem_zif = (struct nsm_if *)mem_ifp->info;
  if (! mem_zif)
    return -1;

  if (agg_type == AGG_CONFIG_LACP)
    pal_snprintf (agg_name, INTERFACE_NAMSIZ, "po%d", key);
  else
    pal_snprintf (agg_name, INTERFACE_NAMSIZ, "sa%d", key); 
   
  agg_ifp = ifg_lookup_by_name (&nzg->ifg, agg_name);
  if (agg_ifp == NULL)
    {
      /* Add aggregate member. */
      nsm_lacp_add_aggregator (nm, agg_name, 
                               mem_zif->type, agg_type);      

      /* Get ifp for aggregator. */
      agg_ifp = ifg_lookup_by_name (&nzg->ifg, agg_name);
      if (agg_ifp == NULL)
        return -1;

#ifdef HAVE_HAL
      /* create aggregator interface in forwarder */
      ret = hal_lacp_add_aggregator (agg_name, mem_ifp->hw_addr,
                                     mem_zif->type == NSM_IF_TYPE_L3 
                                     ? HAL_IF_TYPE_IP
                                     : HAL_IF_TYPE_ETHERNET);

      if (ret < 0)
        {
          return ret;
        }
#endif /* HAVE_HAL */

     /* First Child being added to the aggregator. */
     first_child = PAL_TRUE;
    }
  else
    {
      agg_zif = (struct nsm_if *)agg_ifp->info;

      /* Get first member of the aggregator, and compare admin keys. */
      if (agg_zif && agg_zif->agg.info.memberlist &&
          agg_zif->agg.info.memberlist->count > 0)
        {
          tmp_ifp = listnode_head(agg_zif->agg.info.memberlist);
          if (tmp_ifp->lacp_admin_key != mem_ifp->lacp_admin_key)
              {
               /* Check for Duplex ir B/W Mismatch. */
              status = nsm_lacp_check_link_mismatch(nm, tmp_ifp, mem_ifp);

              /* Any Mismatch other then Duplex and BW function
               * function will be returning error.
               */
              if (status == NSM_LACP_ERROR_ADMIN_KEY_MISMATCH)
                {
                  listnode_add (agg_zif->agg.mismatch_list, mem_ifp);
                  return status;
                }

              }
        } 
    }

  /* Add member. */
  ret = nsm_lacp_add_aggregator_member (nm, agg_ifp, mem_ifp, agg_type, key);
  if (ret < 0)
    return ret;
 
  mem_ifp->lacp_agg_key = key;

  if (notify_lacp)
    {
      nsm_lacp_send_aggregator_config (mem_ifp->ifindex, key,
                                       activate, PAL_TRUE);

#ifdef HAVE_ONMD
      nsm_oam_lldp_if_set_protocol_list (mem_ifp, NSM_LLDP_PROTO_LACP,
                                         PAL_TRUE);
#endif /* HAVE_ONMD */
    }
   
  return 0;
}

/* Aggregator member delete API. */
int
nsm_lacp_api_delete_aggregator_member (struct nsm_master *nm, 
                                       struct interface *mem_ifp,
                                       bool_t notify_lacp)
{
  struct nsm_bridge_master *master = NULL;
  struct nsm_bridge *bridge = NULL;
  struct interface *agg_ifp;
  char agg_name[INTERFACE_NAMSIZ];
  struct nsm_if *agg_zif, *mem_zif;
  struct nsm_server_entry *nse;
  struct nsm_server_client *nsc;
  int i;
  int ret;
  u_char agg_type = AGG_CONFIG_NONE;
#ifdef HAVE_G8031
 struct g8031_protection_group *pg = NULL;
 struct avl_node *pg_node = NULL;
 struct interface *ifp_p = NULL;                
#endif /* HAVE_G8031 */

  master = nsm_bridge_get_master(nm);
  if (master)
    bridge = master->bridge_list;

  if (!mem_ifp || !(mem_zif = (struct nsm_if *)mem_ifp->info) )
    return -1;

  if ( mem_zif->agg_config_type == AGG_CONFIG_LACP )
    pal_snprintf(agg_name, INTERFACE_NAMSIZ, "po%d", mem_ifp->lacp_agg_key);
  else
    pal_snprintf(agg_name, INTERFACE_NAMSIZ, "sa%d", mem_ifp->lacp_agg_key);
  
  agg_ifp = ifg_lookup_by_name (&nzg->ifg, agg_name);
  if (!agg_ifp)
    return -1;

  agg_zif = (struct nsm_if *)agg_ifp->info;

  if (! agg_zif || ! mem_zif)
    return -1;

  if (bridge)
    {
#ifdef HAVE_G8031
      if (bridge->eps_tree != NULL)
        AVL_TREE_LOOP (bridge->eps_tree, pg, pg_node)
          {
            ifp_p = ifg_lookup_by_index (&nm->zg->ifg, pg->ifindex_p);
            if ( ifp_p && pal_strncmp (ifp_p->name, agg_name, 3) == 0)
              return NSM_LACP_IF_CONSISTS_PROTECTION_GRP;
          }
#endif /* HAVE_G8031 */ 
    }

  mem_zif->opcode = NSM_LACP_AGG_DEL;
  agg_zif->agg_mode = PAL_FALSE;
  mem_ifp->lacp_agg_key = 0;
  agg_type = mem_zif->agg_config_type;
  
#ifdef HAVE_HA
  nsm_lacp_cal_delete_agg_associate (mem_zif);
#endif /* HAVE_HA */

  nsm_lacp_process_delete_aggregator_member (nm, agg_ifp->name, mem_ifp->ifindex); 

  /* Delete interface from aggregator member list. */
  if (agg_zif->agg.info.memberlist)
    listnode_delete (agg_zif->agg.info.memberlist, mem_ifp);

  /* Delete parent from aggregated port. */
  mem_zif->agg.info.parent = NULL;
  mem_zif->agg.type = NSM_IF_NONAGGREGATED;
  mem_zif->agg_config_type = AGG_CONFIG_NONE;

  /* Send interface add message to PM. If it is a static aggregator send
   * interface add message to LACP also.
   */

  if (!CHECK_FLAG (mem_ifp->status, NSM_INTERFACE_DELETE))
    NSM_SERVER_CLIENT_LOOP (ng, i, nsc, nse)
      if (nsm_service_check (nse, NSM_SERVICE_INTERFACE))
        {
          if (agg_type == AGG_CONFIG_STATIC)
            {
              if (NSM_INTERFACE_CHECK_MEMBERSHIP(mem_ifp, nse->service.protocol_id)
                  && nse->service.protocol_id != APN_PROTO_ONM)
                {
                  nsm_server_send_link_add (nse, mem_ifp);
                  nsm_server_send_vr_bind (nse, mem_ifp);
                }
            }
          else
            {
              if (NSM_INTERFACE_CHECK_MEMBERSHIP(mem_ifp, nse->service.protocol_id)
                  && nse->service.protocol_id != APN_PROTO_LACP
                  && nse->service.protocol_id != APN_PROTO_ONM)
                {
                  nsm_server_send_link_add (nse, mem_ifp);
                  nsm_server_send_vr_bind (nse, mem_ifp);
                }
            }

          if (nsm_service_check (nse, NSM_SERVICE_INTERFACE) && 
              (nse->service.protocol_id == APN_PROTO_ONM))
            {
              /* Presently only to onmd this message is being sent which will
               * initmate that this port has been aggregate to this agg_ifp */
              if (mem_zif->bridge)
                {
                  nsm_lacp_send_aggregate_member (mem_zif->bridge->name, mem_ifp,
                                                  PAL_FALSE);
                }
            }

        }


#ifdef HAVE_L2

  /* If the interface is explicitly bridge grouped, send a bridge add interface 
   * message to spanning tree protocols. If the interface is bridge grouped by the
   * virtue of it belonging to the aggregator, then remove it from bridge.
   * For nsm_bridge_port_delete pass iterate_members as PAL_FALSE and notify_kernel
   * as PAL_TRUE.
   */

  if (mem_zif->bridge && nm->bridge)
    {
      if (mem_zif->exp_bridge_grpd)
        {
          nsm_bridge_if_add_send_notification (mem_zif->bridge, mem_ifp); 
          nsm_bridge_vlan_port_membership_sync (nm, mem_zif->bridge, mem_ifp);
          nsm_bridge_vlan_port_type_sync (nm, mem_zif->bridge, mem_ifp);
#ifdef HAVE_PROVIDER_BRIDGE
          nsm_pro_vlan_ce_port_sync (nm, mem_ifp);
#endif
        }
      else
        {
          nsm_bridge_port_delete (nm->bridge, mem_zif->bridge->name, mem_ifp, 
                                  PAL_FALSE, PAL_TRUE);
        }
    }

  if ((listcount(agg_zif->agg.info.memberlist) == 0) && agg_zif->bridge)
    nsm_bridge_port_delete (nm->bridge, agg_zif->bridge->name, agg_ifp,
                            PAL_FALSE, PAL_TRUE);

#endif /* HAVE_L2 */
  
#ifdef HAVE_HA
  nsm_lacp_cal_modify_nsm_if_lacp (mem_zif);
  nsm_lacp_cal_modify_nsm_if_lacp (agg_zif);
#endif /* HAVE_HA */

  if (notify_lacp)
    { 
      ret = nsm_lacp_send_aggregator_config (mem_ifp->ifindex, 0, PAL_FALSE,
                                             PAL_FALSE);
      if (ret < 0)
        return ret;

#ifdef HAVE_ONMD
      nsm_oam_lldp_if_set_protocol_list (mem_ifp, NSM_LLDP_PROTO_LACP,
                                         PAL_FALSE);
#endif /* HAVE_ONMD */
    }

  /* Shutdown empty aggregator */
  if (listcount(agg_zif->agg.info.memberlist) == 0)
    {
      if (if_is_up (agg_ifp)
          && agg_zif->agg_oper_state == PAL_TRUE)
        {
          nsm_if_flag_up_unset (agg_ifp->vr->id, agg_ifp->name, PAL_TRUE);
          if (agg_zif->agg.info.memberlist)
            {
              list_delete (agg_zif->agg.info.memberlist);
              agg_zif->agg.info.memberlist = NULL;
            }
        }
      else
        {
          /* Aggregator is shutdown so we won't get delete_aggregator 
             member from lacpd. Hence delete the aggregator interface here */
          if (agg_zif->agg.info.memberlist)
            {
              list_delete (agg_zif->agg.info.memberlist);
              agg_zif->agg.info.memberlist = NULL;
            }

#ifdef HAVE_HAL

          /* If agg_z_delete_aggregator will
           * delete it from hardware.
           */

          if (agg_zif->agg_oper_state == PAL_FALSE)
            hal_lacp_delete_aggregator (agg_ifp->name, 0);

#endif /* HAVE_HAL */

          nsm_lacp_process_delete_aggregator(nm, agg_ifp);
        }
    } 

#ifdef HAVE_SWFWDR

  /* Unset the all multi flags for this member to receive lacp pdus */
  nsm_fea_if_flags_unset (mem_ifp, IFF_ALLMULTI);

#endif /* HAVE_SWFWDR */

  return 0;
}

/***********************************************/
/* Message handling API                        */
/**********************************************/
/* Aggregator delete message API. */
int 
nsm_lacp_process_delete_aggregator (struct nsm_master *nm, struct interface *agg_ifp)
{
  struct nsm_if *zif;

  zif = (struct nsm_if *)agg_ifp->info;
  if (! zif)
    return -1;

  zif->agg.info.memberlist = NULL;

  if (zif->agg_oper_state)
    {
      zif->agg_oper_state = PAL_FALSE;
#ifdef HAVE_HAL
      /* delete aggregator from hw */
      hal_lacp_delete_aggregator (agg_ifp->name, 0);
#endif /* HAVE_HAL */

      pal_mem_set (zif->agg.agg_mac_addr, 0, ETHER_ADDR_LEN);
    }

#ifdef  HAVE_SWFWDR
  /* Update PMs. */
  if (agg_ifp->ifindex <= 0)
    {
      /* if ifindex is 0 if_vr_unbind will not clean the list */
      if (agg_ifp->vr != NULL
          && agg_ifp->vr->ifm.if_list != NULL)
        listnode_delete (agg_ifp->vr->ifm.if_list, agg_ifp);

      nsm_if_delete_update (agg_ifp);
    }
#endif /* HAVE_SWFWDR */

  /* Decrement the aggregator count*/
  ng->agg_cnt--;

#ifdef HAVE_HA
  nsm_cal_modify_nsm_globals (ng);
#endif /* HAVE_HA */

  return 0;
}

/* Aggregate add API. */
int 
nsm_lacp_process_add_aggregator (struct nsm_master *nm, char *name, 
                                 u_char *mac_addr, u_char type, u_char agg_type)
{
  struct interface *agg_ifp;
  char ifname[INTERFACE_NAMSIZ + 1];
  struct nsm_if *agg_zif;
  
  pal_mem_set(ifname, 0, INTERFACE_NAMSIZ+1);
  zsnprintf (ifname, INTERFACE_NAMSIZ, "%s", name);

  agg_ifp = ifg_lookup_by_name (&nzg->ifg, ifname);
  if (! agg_ifp)
    return -1;

  agg_zif = (struct nsm_if *)agg_ifp->info;
  
  if (!agg_zif->agg_oper_state)
    { 
      pal_mem_cpy (agg_zif->agg.agg_mac_addr, mac_addr, ETHER_ADDR_LEN);
      /* Aggregator is now actually created in the hardware */
      agg_zif->agg_oper_state = PAL_TRUE;
    }
  
#ifdef HAVE_HA
  nsm_lacp_cal_modify_nsm_if_lacp (agg_zif);
#endif /* HAVE_HA */
  return 0;
}
 
/* Aggregator member add API. */
int
nsm_lacp_process_add_aggregator_member (struct nsm_master *nm, char *name, 
                                        u_int32_t ifindex, u_char agg_type)
{
  struct interface *agg_ifp, *mem_ifp;
  struct nsm_if *agg_zif, *mem_zif;
  float32_t bw;
  cindex_t cindex = 0;
  int ret;

  agg_ifp = ifg_lookup_by_name (&nzg->ifg, name);
  if (! agg_ifp)
    return -1;

  mem_ifp = ifg_lookup_by_index (&nzg->ifg, ifindex);
  if (! mem_ifp)
    return -1;

  agg_zif = (struct nsm_if *)agg_ifp->info;
  mem_zif = (struct nsm_if *)mem_ifp->info;

  if (! agg_zif || ! mem_zif)
    return -1;

#ifdef HAVE_L2 

  /* If the aggregator is bridge grouped and interface is not bridge grouped
   * add the member interface to bridge. Pass iterate_members as PAL_FALSE
   * and notify kernel as PAL_TRUE. Pass exp_bridge_grpd as false since
   * the port is added to bridge since it belongs to aggregator.
   */

  if ( agg_zif->bridge && !mem_zif->bridge && nm->bridge)
    {
      nsm_bridge_api_port_add (nm->bridge, agg_zif->bridge->name, mem_ifp, PAL_FALSE, 
                               PAL_TRUE, PAL_FALSE, PAL_FALSE);
    }
#endif /* HAVE_L2 */

#ifdef HAVE_HAL
  /* send agg port attachment to hw */
  ret = hal_lacp_attach_mux_to_aggregator (agg_ifp->name, 0,
                                           mem_ifp->name, 
                                           ifindex);
#endif /* HAVE_HAL */
  if (ret < 0)
    return ret;

  /* The port is now operationally part of the aggregator */
  mem_zif->agg_oper_state = PAL_TRUE;
  
/* If aggregator not up, bring it up */
/* In order to validate the fix we need to run the lacp-ha regression
 * With the fix we need to verify that it is not affecting static and HA LACP
 * functionality when we merge this into main branch.
 */
  if (!if_is_up (agg_ifp))
    nsm_if_flag_up_set (agg_ifp->vr->id, agg_ifp->name, PAL_TRUE);
 
  bw = agg_ifp->bandwidth;
  nsm_lacp_aggregator_bw_update (agg_ifp);
  
  if (agg_ifp->bandwidth > 0 && bw != agg_ifp->bandwidth)
    {
      NSM_SET_CTYPE (cindex, NSM_LINK_CTYPE_BANDWIDTH);
      
      if (if_is_running (agg_ifp))
        nsm_server_if_up_update (agg_ifp, cindex);
      else
        nsm_server_if_down_update (agg_ifp, cindex);
    }
  
#ifdef HAVE_L2 
  if (agg_zif->bridge)
    nsm_bridge_if_send_state_sync_req (agg_zif->bridge, agg_ifp);
#endif

#ifdef HAVE_ONMD
  nsm_lldp_set_agg_port_id (mem_ifp, agg_ifp->ifindex);
#endif /* HAVE_ONMD */


#ifdef HAVE_HA
  lib_cal_modify_interface (nzg, mem_ifp);
  nsm_lacp_cal_modify_nsm_if_lacp (mem_zif);
#endif /* HAVE_HA */

  return 0;
}


/* Aggregator member delete API. */
int
nsm_lacp_process_delete_aggregator_member (struct nsm_master *nm, char *name,
                                           u_int32_t ifindex)
{
  struct interface *agg_ifp, *mem_ifp;
  struct nsm_if *agg_zif, *mem_zif;
  float32_t bw;
  cindex_t cindex = 0;
  int ret;

  agg_ifp = ifg_lookup_by_name (&nzg->ifg, name);
  if (! agg_ifp)
    return -1;

  mem_ifp = ifg_lookup_by_index (&nzg->ifg, ifindex);
  if (! mem_ifp)
    return -1;

  agg_zif = (struct nsm_if *)agg_ifp->info;
  mem_zif = (struct nsm_if *)mem_ifp->info;

  if (! agg_zif || ! mem_zif)
    return -1;
  
  if (mem_zif->agg_oper_state)
    {
      mem_zif->agg_oper_state = PAL_FALSE;
 
      bw = agg_ifp->bandwidth;
      nsm_lacp_aggregator_bw_update (agg_ifp);

      if (agg_ifp->ifindex > 0 && bw != agg_ifp->bandwidth)
        {
          NSM_SET_CTYPE (cindex, NSM_LINK_CTYPE_BANDWIDTH);
      
          if (if_is_running (agg_ifp))
            nsm_server_if_up_update (agg_ifp, cindex);
          else
            nsm_server_if_down_update (agg_ifp, cindex);
        }
      
      if (mem_zif->bridge)
        nsm_bridge_api_set_port_state_all (nm, mem_zif->bridge->name, mem_ifp,
                                           NSM_BRIDGE_PORT_STATE_BLOCKING);
      
#ifdef HAVE_L2
      /* Delete the multicast entries belonging to this AGG. 
         As one member got deleted from the AGG, there might be some multicast
         entries hanging off this member. Lower layer can then add this entries
         to another member. This is only for cases, where the hardware doesn't
         have capability to have layer-2 multicast forwarding based on the trunk id.
         For other hardware this will be a expense but not very frequent. */
      if (NSM_INTF_TYPE_L2 (agg_ifp))
        {
           nsm_bridge_sync_multicast_entries (agg_zif->bridge, agg_ifp, 0);
        }
#endif /* HAVE_L2 */

#ifdef HAVE_HAL
      /* send agg port detachment to hw */
      ret = hal_lacp_detach_mux_from_aggregator (agg_ifp->name, 0,
                                                 mem_ifp->name, ifindex);
#endif /* HAVE_HAL */

#ifdef HAVE_L2
      /* Add the multicast entries belonging to this AGG. 
         As one member got deleted from the AGG, there might be some multicast
         entries hanging off this member. Lower layer can then add this entries
         to another member. This is only for cases, where the hardware doesn't
         have capability to have layer-2 multicast forwarding based on the trunk id.
         For other hardware this will be a expense but not very frequent. */
      if (NSM_INTF_TYPE_L2 (agg_ifp))
        {
          /* Sync. */
          nsm_bridge_sync_multicast_entries (agg_zif->bridge, agg_ifp, 1);
        }
#endif /* HAVE_L2 */

      /* Return from HAL call. */
      if (ret < 0)
        {
#ifdef HAVE_HA
          nsm_lacp_cal_modify_nsm_if_lacp (mem_zif);
#endif /* HAVE_HA */
          return ret;
        }
    }

#ifdef HAVE_ONMD
  nsm_lldp_set_agg_port_id (mem_ifp, NSM_LLDP_AGG_PORT_INDEX_NONE);
#endif /* HAVE_ONMD */


#ifdef HAVE_HA
  nsm_lacp_cal_modify_nsm_if_lacp (mem_zif);
  lib_cal_modify_interface (nzg, mem_ifp);
#endif /* HAVE_HA */

  return 0;
}

void
nsm_lacp_cleanup_aggregator (struct nsm_master *nm, struct interface *agg_ifp)
{
  struct nsm_if *agg_zif;
  struct listnode *ln;
  struct interface *ifp;

  agg_zif = (struct nsm_if *)agg_ifp->info;
  if (! agg_zif)
    return;
        
  if (agg_zif->agg.info.memberlist)
    {
      LIST_LOOP (agg_zif->agg.info.memberlist, ifp, ln)
        {
          /* Delete aggregate member. */
          nsm_lacp_process_delete_aggregator_member (nm, agg_ifp->name,
                                                     ifp->ifindex);
        }
    }
}

#ifdef HAVE_ONMD
void
nsm_lacp_update_lldp_agg_port_id (struct interface *agg_ifp)
{
  struct nsm_if *zif;
  struct listnode *ln;
  struct interface *mem_ifp;

  zif = (struct nsm_if *)agg_ifp->info;

  if (! zif)
    return;

  if (zif->agg.info.memberlist)
    {
      LIST_LOOP (zif->agg.info.memberlist, mem_ifp, ln)
        {
          nsm_lldp_set_agg_port_id (mem_ifp, agg_ifp->ifindex);
        }
    }

}
#endif /* HAVE_ONMD */

int
nsm_lacp_aggregator_add_msg_process (struct nsm_master *nm,
                                     struct nsm_msg_lacp_agg_add *msg)
{
  int i;
  bool_t create_agg = NSM_FALSE;
  struct interface *mem_ifp, *agg_ifp;
  struct nsm_if *mem_zif = NULL;
  struct nsm_if *agg_zif = NULL;
  struct interface *tmp_ifp = NULL;
  struct nsm_if *tmp_zif = NULL;
  struct listnode *ln = NULL;

  agg_ifp = ifg_lookup_by_name (&nzg->ifg, msg->agg_name);
  if (agg_ifp == NULL)
    create_agg = NSM_TRUE;
  
  if (agg_ifp)
    { 
      agg_zif = (struct nsm_if *)agg_ifp->info; 
      if (!CHECK_FLAG (agg_ifp->status, NSM_INTERFACE_ACTIVE))
        create_agg = NSM_TRUE;
    }
   
  /* Ports added. */
  if (NSM_CHECK_CTYPE (msg->cindex, NSM_LACP_AGG_CTYPE_PORTS_ADDED))
    {
      for (i = 0; i < msg->add_count; i++)
        {
          if (create_agg)
            {
              mem_ifp = ifg_lookup_by_index (&nzg->ifg, msg->ports_added[i]);
              if (! mem_ifp)
                continue;
              
              mem_zif = (struct nsm_if *)mem_ifp->info;
              NSM_ASSERT (mem_zif != NULL);
              if (! mem_zif)
                continue;
              
              /* Add aggregator. */
              nsm_lacp_process_add_aggregator (nm, msg->agg_name, 
                                               msg->mac_addr, 
                                               mem_zif->type, msg->agg_type);      
              create_agg = NSM_FALSE;
            }

          /* Add member. */
          nsm_lacp_process_add_aggregator_member (nm, msg->agg_name, 
                                                  msg->ports_added[i], 
                                                  msg->agg_type);

          if (agg_ifp)
            {
              if (!if_is_up (agg_ifp) && (agg_ifp->vr))
                nsm_if_flag_up_set (agg_ifp->vr->id,
                    agg_ifp->name, PAL_FALSE);
            }

          if (agg_zif && !agg_zif->agg_oper_state)
            agg_zif->agg_oper_state = PAL_TRUE;

#ifdef HAVE_HA
          nsm_lacp_cal_modify_nsm_if_lacp (agg_zif);
          lib_cal_modify_interface (nzg, agg_ifp);
#endif /* HAVE_HA */
        }
    }

  /* Ports deleted. */
  if (NSM_CHECK_CTYPE (msg->cindex, NSM_LACP_AGG_CTYPE_PORTS_DELETED))
    {
      for (i = 0; i < msg->del_count; i++)
        {
          /* Delete aggregate member. */
          nsm_lacp_process_delete_aggregator_member (nm, msg->agg_name, 
                                                     msg->ports_deleted[i]);
        }
      /* reusing the create_agg flag to find if the aggregator needs to be 
         deleted */
      create_agg = NSM_TRUE; /* in this context set it to true meaning
                                assume the agg needs to be deleted */
      if (agg_zif && agg_zif->agg.info.memberlist)
        {
          LIST_LOOP (agg_zif->agg.info.memberlist, tmp_ifp, ln)
            {
              tmp_zif = (struct nsm_if *)tmp_ifp->info;
              if (tmp_zif && tmp_zif->agg_oper_state)
                {
                  create_agg = NSM_FALSE;
                  break;
                }
            }
        }
      if ((create_agg) && (agg_zif) && (agg_zif->agg_oper_state))
        {
          if (listcount(agg_zif->agg.info.memberlist) == 0)
            { 
              nsm_lacp_process_delete_aggregator(nm, agg_ifp);
            }
          else
            {
              /* Shut down the agg as the members are out of sync */
              if (agg_ifp->vr)
              nsm_if_flag_up_unset (agg_ifp->vr->id, agg_ifp->name, PAL_FALSE);
#ifdef HAVE_HA
              nsm_lacp_cal_modify_nsm_if_lacp (agg_zif);
#endif /* HAVE_HA */
            }
        }
    }

  return 0;
}

/* Aggregate add message handler. */
int
nsm_lacp_recv_aggregator_add (struct nsm_msg_header *header,
                              void *arg, void *message)
{
  struct nsm_master *nm;
  struct nsm_server *ns;
  struct nsm_server_entry *nse;
  struct nsm_msg_lacp_agg_add *msg;

  /* Set connection and message.  */
  nse = arg;
  ns = nse->ns;
  msg = (struct nsm_msg_lacp_agg_add *) message;

#if 0
  /* Dump message.  */
  if (IS_NSM_DEBUG_RECV)
    nsm_agg_add_dump (ns->zg, vfd);
#endif

  nm = nsm_master_lookup_by_id (nzg, header->vr_id);
  if (nm == NULL)
    return 0;

  return nsm_lacp_aggregator_add_msg_process (nm, msg);
}

/* Aggregate delete. */
int
nsm_lacp_recv_aggregator_del (struct nsm_msg_header *header,
                              void *arg, void *message)
{
  struct nsm_master *nm;
  struct nsm_server *ns;
  struct nsm_server_entry *nse;
  char *agg_name;
  struct interface *agg_ifp;

  return 0;

#if 0  
  /* Set connection and message.  */
  nse = arg;
  ns = nse->ns;
  agg_name = (char *)message;

  nm = nsm_master_lookup_by_id (nzg, header->vr_id);
  if (nm == NULL)
    return 0;

  agg_ifp = ifg_lookup_by_name (&nzg->ifg, agg_name);
  if (! agg_ifp)
    return 0;

  nsm_lacp_cleanup_aggregator (nm, agg_ifp);

  /* Delete aggegator. */
  nsm_lacp_process_delete_aggregator (nm, agg_ifp);

  return 0;
#endif /* if 0 */
}

void
nsm_lacp_delete_all_aggregators (struct nsm_master *nm)
{
  struct route_node *rn;
  struct interface *agg_ifp, *mem_ifp;
  struct nsm_if *agg_zif, *mem_zif;
  struct listnode *ln, *ln_next;
  struct nsm_server_client *nsc;
  struct nsm_server_entry *nse;
  u_int32_t i;

  /* delete all aggregators */
  for (rn = route_top (nzg->ifg.if_table); rn; rn = route_next (rn))
    {
      if ((agg_ifp = rn->info) == NULL)
        continue;

      agg_zif = (struct nsm_if *)agg_ifp->info;
      if (! agg_zif || agg_zif->agg.type != NSM_IF_AGGREGATOR)
        continue;

      /* detach aggregator member ports */
      if (agg_zif->agg.info.memberlist)
        {
          for (ln = LISTHEAD (agg_zif->agg.info.memberlist); ln; ln = ln_next)
            {
              ln_next = ln->next;
              
              mem_ifp = GETDATA (ln);

              mem_zif = (struct nsm_if *)mem_ifp->info;

              /* Delete parent from aggregated port. */
              mem_zif->agg.info.parent = NULL;
              mem_zif->agg.type = NSM_IF_NONAGGREGATED;
              
              /* Send interface add message to PM. */
              NSM_SERVER_CLIENT_LOOP (ng, i, nsc, nse)
                if (nsm_service_check (nse, NSM_SERVICE_INTERFACE))
                  {
                    if (NSM_INTERFACE_CHECK_MEMBERSHIP(mem_ifp, 
                                                       nse->service.protocol_id)
                        && nse->service.protocol_id != APN_PROTO_LACP
                        && nse->service.protocol_id != APN_PROTO_ONM)
                      {
                        nsm_server_send_interface_add (nse, agg_ifp);
                      }
                  }
  
              ln->data = NULL;
              list_delete_node (agg_zif->agg.info.memberlist, ln);
            }

#ifdef HAVE_L2
          if ((listcount(agg_zif->agg.info.memberlist) == 0) && agg_zif->bridge)
            nsm_bridge_port_delete (nm->bridge, agg_zif->bridge->name, agg_ifp,
                                    PAL_FALSE, PAL_TRUE);
#endif /* end of HAVE_L2 */

          list_delete (agg_zif->agg.info.memberlist);
          agg_zif->agg.info.memberlist = NULL;
        }

#ifdef HAVE_HAL      
      /* delete aggregator from hw */
      hal_lacp_delete_aggregator (agg_ifp->name, 0);
#endif /* HAVE_HAL */

#ifdef  HAVE_SWFWDR
      /* Update PMs. */
      nsm_if_delete_update (agg_ifp);  
#endif  /* HAVE_SWFWDR */
    }
}

int
nsm_update_aggregator_member_state (struct nsm_if *nif)
{
  struct nsm_if_agg agg;
#ifdef HAVE_L2
  struct nsm_if *zif = NULL;
#endif /* HAVE_L2 */
  int ret = 0;

  if (!nif) 
    return -1;

  if (!nif->ifp)
    return -1;

  /* We don't need to update the state for aggregator itself. */
  if(NSM_INTF_TYPE_AGGREGATOR(nif->ifp))
    return ret;

  agg = nif->agg;
  if (nif->agg_config_type == AGG_CONFIG_STATIC)
    {
      if (agg.info.parent && nif->ifp)
        {
          if (if_is_running (nif->ifp))
            {
              if (nif->agg_oper_state != PAL_TRUE)
                {
                  #ifdef HAVE_HAL
                  ret = hal_lacp_attach_mux_to_aggregator (agg.info.parent->name, 0,
                                                           nif->ifp->name,
                                                           nif->ifp->ifindex);
                  #endif /* HAVE_HAL */
                  nif->agg_oper_state = PAL_TRUE;
                }
            }
          else
            {
#ifdef HAVE_L2
              /* Delete the multicast entries belonging to this AGG. 
                 As one member got deleted from the AGG, there might be some multicast
                 entries hanging off this member. Lower layer can then add this entries
                 to another member. This is only for cases, where the hardware doesn't
                 have capability to have layer-2 multicast forwarding based on the trunk id.
                 For other hardware this will be a expense but not very frequent. */
              if (NSM_INTF_TYPE_L2 (agg.info.parent))
                {
                  zif = agg.info.parent->info;
                  if (! zif || ! zif->bridge)
                    return -1;

                  nsm_bridge_sync_multicast_entries (zif->bridge, agg.info.parent, 0);
                }
#endif /* HAVE_L2 */
              
              if (nif->agg_oper_state == PAL_TRUE)
                {
                  #ifdef HAVE_HAL
                  ret = hal_lacp_detach_mux_from_aggregator (agg.info.parent->name, 0,
                                                             nif->ifp->name,
                                                             nif->ifp->ifindex);
                  #endif /* HAVE_HAL */
                  nif->agg_oper_state = PAL_FALSE;
                }
 
#ifdef HAVE_L2
              /* Add the multicast entries belonging to this AGG. 
                 As one member got deleted from the AGG, there might be some multicast
                 entries hanging off this member. Lower layer can then add this entries
                 to another member. This is only for cases, where the hardware doesn't
                 have capability to have layer-2 multicast forwarding based on the trunk id.
                 For other hardware this will be a expense but not very frequent. */
              if (NSM_INTF_TYPE_L2 (agg.info.parent))
                {
                  /* zif is set in the block before the hal_lacp_detach_mux_from_aggregator. */

                  /* Sync. */
                  nsm_bridge_sync_multicast_entries (zif->bridge, agg.info.parent, 1);
                }
#endif /* HAVE_L2 */
            }
        }
    }

#ifdef HAVE_HA
  nsm_lacp_cal_modify_nsm_if_lacp (nif);
#endif /* HAVE_HA */ 
  return ret;  
}


/* Set NSM server callbacks. */
int
nsm_lacp_set_server_callback (struct nsm_server *ns)
{
  nsm_server_set_callback (ns, NSM_MSG_LACP_AGGREGATE_ADD,
                           nsm_parse_lacp_aggregator_add,
                           nsm_lacp_recv_aggregator_add);
  nsm_server_set_callback (ns, NSM_MSG_LACP_AGGREGATE_DEL,
                           nsm_parse_lacp_aggregator_del,
                           nsm_lacp_recv_aggregator_del);
  return 0;
}

struct nsm_lacp_admin_key_element *
nsm_lacp_admin_key_element_new (struct nsm_master *nm,
                                struct interface *ifp)
{
  struct nsm_bridge_port *br_port = NULL;
  struct nsm_lacp_admin_key_element *ake = NULL;
  u_int32_t key;
  struct nsm_if *nsmif = NULL;
  int ret;

  if (! ifp || ! ifp->info)
    return NULL;

  nsmif = ifp->info;

  ake = XCALLOC (MTYPE_LACP_ADMIN_KEY_ELEMENT,
                 sizeof (struct nsm_lacp_admin_key_element));
  if (! ake)
    return NULL;

  ret = bitmap_request_index (nm->lacp_admin_key_mgr, &key);
  if (ret < 0)
    {
      XFREE (MTYPE_LACP_ADMIN_KEY_ELEMENT, ake);
      return NULL;
    }

  ake->key = key;
  ake->bandwidth = ifp->bandwidth;
  ake->mtu = ifp->mtu;
  ake->duplex = ifp->duplex;
  ake->hw_type = ifp->hw_type;
  ake->type = nsmif->type;

#ifdef HAVE_VLAN

  if (nsmif && nsmif->switchport)
    {
      br_port = nsmif->switchport;

      if (br_port->vlan_port.mode != NSM_VLAN_PORT_MODE_INVALID)
        {
          ake->vlan_id = br_port->vlan_port.pvid;
          ake->vlan_port_mode = br_port->vlan_port.mode;
          ake->vlan_port_config = br_port->vlan_port.config;
        }
      
      ake->vlan_bmp_static = XCALLOC (MTYPE_NSM_VLAN_BMP,
                                       sizeof (struct nsm_vlan_bmp));
      if (! ake->vlan_bmp_static)
        {
          bitmap_release_index (nm->lacp_admin_key_mgr, key);
          XFREE (MTYPE_LACP_ADMIN_KEY_ELEMENT, ake);
          return NULL;
        }
  
      ake->vlan_bmp_dynamic = XCALLOC (MTYPE_NSM_VLAN_BMP,
                                   sizeof (struct nsm_vlan_bmp));
      if (! ake->vlan_bmp_dynamic)
        {
          bitmap_release_index (nm->lacp_admin_key_mgr, key);
          XFREE (MTYPE_NSM_VLAN_BMP, ake->vlan_bmp_static);
          XFREE (MTYPE_LACP_ADMIN_KEY_ELEMENT, ake);
          return NULL;
        }
  
      pal_mem_cpy (ake->vlan_bmp_static, &br_port->vlan_port.staticMemberBmp,
                   sizeof (struct nsm_vlan_bmp));

      pal_mem_cpy (ake->vlan_bmp_dynamic,
                   &br_port->vlan_port.dynamicMemberBmp,
                   sizeof (struct nsm_vlan_bmp));

    }
#endif /* HAVE_VLAN */

#ifdef HAVE_L2
  if (nsmif->bridge)
    ake->bridge_id = pal_atoi (nsmif->bridge->name);
#endif /* HAVE_L2 */

#ifdef HAVE_HA
  nsm_lacp_cal_create_ake (ake);
#endif /* HAVE_HA */
  
  return ake;
}

void
nsm_lacp_admin_key_element_free (struct nsm_master *nm,
                                 struct nsm_lacp_admin_key_element *ake)
{
  if (! ake || ! nm)
    return;

#ifdef HAVE_HA
  nsm_lacp_cal_delete_ake (ake);
#endif /* HAVE_HA */

  if (ake->key > 0)
    {
      bitmap_release_index (nm->lacp_admin_key_mgr, ake->key);
      ake->key = 0;
    }

#ifdef HAVE_VLAN
  if (ake->vlan_bmp_static)
    {
      XFREE (MTYPE_NSM_VLAN_BMP, ake->vlan_bmp_static);
      ake->vlan_bmp_static = NULL;
    }

  if (ake->vlan_bmp_dynamic)
    {
      XFREE (MTYPE_NSM_VLAN_BMP, ake->vlan_bmp_dynamic);
      ake->vlan_bmp_static = NULL;
    }
#endif /* HAVE_VLAN */

  XFREE (MTYPE_LACP_ADMIN_KEY_ELEMENT, ake);
}

void
nsm_lacp_admin_key_element_link (struct nsm_lacp_admin_key_element *ake,
                                 struct interface *ifp)
{
  if (! ifp || ! ake)
    return;

  ifp->lacp_admin_key = ake->key;
  ake->refcount++;

#ifdef HAVE_HA
  nsm_lacp_cal_modify_ake (ake);
  lib_cal_modify_interface (nzg, ifp);
#endif /* HAVE_HA */

}

void
nsm_lacp_admin_key_element_unlink (struct nsm_lacp_admin_key_element *ake,
                                   struct interface *ifp)
{
  if (! ifp || ! ake)
    return;

  ifp->lacp_admin_key = 0;
  if (ake->refcount)
    ake->refcount--;

#ifdef HAVE_HA
  nsm_lacp_cal_modify_ake (ake);
  lib_cal_modify_interface (nzg, ifp);
#endif /* HAVE_HA */

}

int
nsm_lacp_admin_key_element_add (struct nsm_master *nm,
                                struct nsm_lacp_admin_key_element *ake)
{
  struct nsm_lacp_admin_key_element *ake_temp;

  if (! ake)
    return NSM_ERR_INVALID_ARGS;

  if (! nm->ake_list || nm->ake_list->key > ake->key)
    {
      ake->next = nm->ake_list;
      nm->ake_list = ake;
      return NSM_SUCCESS;
    }

  for (ake_temp = nm->ake_list; ake_temp; ake_temp = ake_temp->next)
    {
      if (! ake_temp->next || ake_temp->next->key > ake->key)
        {
          ake->next = ake_temp->next;
          ake_temp->next = ake;
          break;
        }
    }

  return NSM_SUCCESS;
}

struct nsm_lacp_admin_key_element *
nsm_lacp_admin_key_element_lookup (struct nsm_master *nm, u_int16_t key)
{
  struct nsm_lacp_admin_key_element *ake = NULL;
  
  for (ake = nm->ake_list; ake && ake->key <= key; ake = ake->next)
    {
      if (ake->key == key)
        {
          return ake;
        } 
    }

  return NULL;
}

struct nsm_lacp_admin_key_element *
nsm_lacp_admin_key_element_lookup_by_ifp (struct nsm_master *nm, 
                                          struct interface *ifp)
{
  struct nsm_lacp_admin_key_element *ake = NULL;
#ifdef HAVE_VLAN
  struct nsm_bridge_port *br_port = NULL;
  struct nsm_vlan_port *vlan_port = NULL;
#endif /* HAVE_VLAN */
  struct nsm_if *nsmif;
  
  if (! ifp || ! ifp->info || ! nm)
    return NULL;

  nsmif = ifp->info;

#ifdef HAVE_VLAN
  if (nsmif && nsmif->switchport)
    br_port = nsmif->switchport;

  if (br_port)
       vlan_port = &br_port->vlan_port;
#endif /* HAVE_VLAN */

  for (ake = nm->ake_list; ake; ake = ake->next)
    {
#ifndef HAVE_L2
      if (ake->bandwidth == ifp->bandwidth &&
          ake->mtu == ifp->mtu &&
          ake->type == nsmif->type &&
          ake->duplex == ifp->duplex &&
          ake->hw_type == ifp->hw_type)
        {
          return ake;
        }
#else
#ifdef HAVE_VLAN

     if ((br_port != NULL) && (vlan_port != NULL))
       {
         if (ake->bandwidth == ifp->bandwidth &&
             ake->mtu == ifp->mtu &&
             ake->type == nsmif->type &&
             ake->duplex == ifp->duplex &&
             ake->vlan_id == br_port->vlan_port.pvid &&
             ((br_port->vlan_port.mode == NSM_VLAN_PORT_MODE_INVALID) ||
             (ake->vlan_port_mode == br_port->vlan_port.mode &&
             ake->vlan_port_mode == br_port->vlan_port.mode &&
             ake->vlan_port_config == vlan_port->config &&
             NSM_VLAN_BMP_CMP (ake->vlan_bmp_static,
                               &br_port->vlan_port.staticMemberBmp) == 0 &&
             NSM_VLAN_BMP_CMP (ake->vlan_bmp_dynamic,
                               &br_port->vlan_port.dynamicMemberBmp) == 0)) &&
             ake->bridge_id == (nsmif->bridge
                                ? pal_atoi (nsmif->bridge->name) : 0) &&
             ake->hw_type == ifp->hw_type)
            {
              return ake;
            }
       }
     else if(!ake->bridge_id)
       {
         if (ake->bandwidth == ifp->bandwidth &&
             ake->mtu == ifp->mtu &&
             ake->type == nsmif->type &&
             ake->duplex == ifp->duplex &&
             ake->hw_type == ifp->hw_type)
           {
             return ake;
           }
       }
#else
     if (ake->bandwidth == ifp->bandwidth &&
         ake->mtu == ifp->mtu &&
         ake->type == nsmif->type &&
         ake->duplex == ifp->duplex &&
         ake->bridge_id == (nsmif->bridge
                            ? pal_atoi (nsmif->bridge->name) : 0) &&
         ake->hw_type == ifp->hw_type)
       {
         return ake;
       }

#endif /* HAVE_VLAN */
#endif /* HAVE_L2 */
    }

  return NULL;
}

struct nsm_lacp_admin_key_element *
nsm_lacp_admin_key_element_remove (struct nsm_master *nm, u_int16_t key)
{
  struct nsm_lacp_admin_key_element *ake, *ake_temp = NULL;

  if ((ake_temp = nm->ake_list) != NULL && nm->ake_list->key == key)
    {
      nm->ake_list = nm->ake_list->next;
      ake_temp->next = NULL;
      return ake_temp;
    }
       
  for (ake = nm->ake_list; ake; ake = ake->next)
    {
      if (ake->key > key)
        break;

      if (ake->next && ake->next->key == key)
        {
          ake_temp = ake->next;
          ake->next = ake->next->next;
          ake_temp->next = NULL;
          return ake_temp;
        } 
    }

  return NULL;
}

void
nsm_lacp_trigger_config_read (struct nsm_master *nm)
{
  struct nsm_server_entry *nse = NULL;
  struct nsm_server_client *nsc = NULL;
  u_int32_t i;

  if (nm == NULL)
    return;

  NSM_SERVER_CLIENT_LOOP (ng, i, nsc, nse)
    {
      if (nse->service.protocol_id == APN_PROTO_LACP)
        nsm_server_send_vr_sync_done (nse);
    }

  return;
}

void
nsm_lacp_retry_aggregation (struct interface *ifp)
{
  struct interface *mis_ifp = NULL;
  struct interface *agg_ifp = NULL;
  struct nsm_if *mis_zif = NULL;
  struct nsm_if *agg_zif = NULL;
  struct nsm_master *nm = NULL;
  struct listnode *next = NULL;
  struct listnode *nn = NULL;
  struct nsm_if *zif = NULL;
  u_int32_t cnt = 0;
  s_int32_t ret;

  if (ifp == NULL)
    return;

  zif = ifp->info;

  if (zif == NULL)
    return;

  nm = ifp->vr->proto;;

  if (NSM_INTF_TYPE_AGGREGATED (ifp))
    {
      agg_ifp = zif->agg.info.parent;

      if (agg_ifp != NULL)
       agg_zif = agg_ifp->info;

      if (agg_zif != NULL
          && agg_zif->agg.mismatch_list != NULL)
        {
          LIST_LOOP_DEL (agg_zif->agg.mismatch_list, mis_ifp, nn, next)
          {
             mis_zif = mis_ifp->info;

             if (mis_zif == NULL)
               continue;

           if (mis_zif->conf_key
               && mis_zif->conf_chan_activate
               && mis_zif->conf_agg_config_type)
             {

                 ret = nsm_lacp_api_add_aggregator_member
                                (nm, mis_ifp, (u_int16_t) mis_zif->conf_key,
                                 mis_zif->conf_chan_activate, PAL_TRUE,
                                 mis_zif->conf_agg_config_type);

                 if (ret == 0)
                   {
                     cnt++;
                     mis_zif->conf_key = 0;
                     mis_zif->conf_chan_activate = PAL_FALSE;
                     mis_zif->conf_agg_config_type = AGG_CONFIG_NONE;
                     listnode_delete (zif->agg.mismatch_list, mis_ifp);
                   }
              }
            else
              {
                 listnode_delete (zif->agg.mismatch_list, mis_ifp);
              }
          }

          if (cnt > 0)
            nsm_lacp_trigger_config_read (nm);
        }
    }
  else
    {
       if (zif->conf_key
           && zif->conf_chan_activate
           && zif->conf_agg_config_type)
         {
           ret = nsm_lacp_api_add_aggregator_member
                          (nm, ifp, (u_int16_t) zif->conf_key,
                           zif->conf_chan_activate, PAL_TRUE,
                           zif->conf_agg_config_type);

           if (ret == 0)
             {
               nsm_lacp_trigger_config_read (nm);
               zif->conf_key = 0;
               zif->conf_chan_activate = PAL_FALSE;
               zif->conf_agg_config_type = AGG_CONFIG_NONE;
             }
         }
    }

}

int
nsm_lacp_if_admin_key_set (struct interface *ifp)
{
  int ret;
  u_int16_t key;
  struct nsm_master *nm = NULL;
  struct nsm_lacp_admin_key_element *ake;

  if (ifp && ifp->vr)
    nm = (struct nsm_master *)ifp->vr->proto;
  if (! nm)
    return NSM_ERR_INTERNAL;
 
  key = ifp->lacp_admin_key; 

  /* We don't need to update the key for aggregator itself. */
  if(NSM_INTF_TYPE_AGGREGATOR(ifp))
    return 0;

  if (ifp->lacp_admin_key > 0)
    nsm_lacp_if_admin_key_unset (ifp);

  /* check if interface is aggregatable */
  if (ifp->hw_type == IF_TYPE_VLAN ||
#ifdef HAVE_TUNNEL
      ifp->tunnel_if != NULL ||
#endif /* HAVE_TUNNEL */
      ifp->hw_type == IF_TYPE_AGGREGATE)
    {
      ifp->lacp_admin_key = 0;
#ifdef HAVE_HA
      lib_cal_modify_interface (nzg, ifp);
#endif /* HAVE_HA */
      return 0;
    }
  
  /* search admin key data with the interface information */
  ake = nsm_lacp_admin_key_element_lookup_by_ifp (nm, ifp);
  if (ake)
    {
      /* link admin key element and interface */
      nsm_lacp_admin_key_element_link (ake, ifp);

      if (ifp->lacp_admin_key != key)
        nsm_lacp_retry_aggregation (ifp);

      return NSM_SUCCESS;
    }

  /* create new key element */
  ake = nsm_lacp_admin_key_element_new (nm, ifp);
  if (! ake)
    return -1;

  /* link admin key element and interface */
  nsm_lacp_admin_key_element_link (ake, ifp);
  
  /* add it to list */
  ret = nsm_lacp_admin_key_element_add (nm, ake);
  if (ret < 0)
    {
      nsm_lacp_admin_key_element_unlink (ake, ifp);
      nsm_lacp_admin_key_element_free (nm, ake);
      ake = NULL;
      return ret;
    }

  if (ifp->lacp_admin_key != key)
    nsm_lacp_retry_aggregation (ifp);
  
  return NSM_SUCCESS;
}

void
nsm_lacp_if_admin_key_unset (struct interface *ifp)
{
  struct nsm_master *nm = NULL;
  struct nsm_lacp_admin_key_element *ake;

  if (! ifp || ifp->lacp_admin_key <= 0)
    return;

  if (ifp->vr)
    nm = (struct nsm_master *)ifp->vr->proto;
  if (! nm || ! nm->lacp_admin_key_mgr)
    return;
  
  ake = nsm_lacp_admin_key_element_lookup (nm, ifp->lacp_admin_key);
  if (! ake)
    {
      ifp->lacp_admin_key = 0;
#ifdef HAVE_HA
      lib_cal_modify_interface (nzg, ifp);
#endif /* HAVE_HA */
      return;
    }

  nsm_lacp_admin_key_element_unlink (ake, ifp);
  if (ake->refcount == 0)
    {
      nsm_lacp_admin_key_element_remove (nm, ake->key);
      nsm_lacp_admin_key_element_free (nm, ake);
      ake = NULL;
    }
}

void
nsm_lacp_aggregator_bw_update (struct interface *ifp)
{
  struct nsm_if *zif = ifp ? ifp->info : NULL;
  struct nsm_if *mem_zif;
  struct listnode *node;
  struct interface *ifp2;
  float32_t bw = 0.0;

  if (! zif || ! zif->agg.info.memberlist)
    return;
  
#if 0
  node = LISTHEAD (zif->agg.info.memberlist);
  if (! node)
    return;

  ifp2 = GETDATA (node);
  if (ifp2)
    {
      ifp->bandwidth = LISTCOUNT (zif->agg.info.memberlist) * ifp2->bandwidth;
    }
#endif

  /* Only consider oper state TRUE ports for BW calculation. */
  /* FIXME: Get this fix verified from LACP expert */
  LIST_LOOP (zif->agg.info.memberlist, ifp2, node)
    {
      if ((mem_zif = ifp2->info) == NULL)
        continue;

      if (mem_zif->agg_oper_state == PAL_FALSE)
        continue;

      bw += ifp2->bandwidth;
    }

  ifp->bandwidth = bw;

#ifdef HAVE_HA
  lib_cal_modify_interface (NSM_ZG, ifp);
#endif /* HAVE_HA */
}

int
nsm_lacp_aggregator_psc_set (struct interface *ifp, int psc)
{
  int ret;
  struct nsm_if *zif;
 
  if(!ifp)
    {
      return NSM_ERR_INVALID_ARGS;
    }

  zif = (struct nsm_if *)ifp->info;
  if((!zif) || (zif->agg.type != NSM_IF_AGGREGATOR))
    {
      return NSM_ERR_INVALID_ARGS;
    }

  if (zif->agg.psc == psc)
    return NSM_SUCCESS;

#ifdef HAVE_HAL
  ret = hal_lacp_psc_set(ifp->ifindex,psc);
#endif /* HAVE_HAL */

  /* delay psc bit set, if trunk not up */
  if(ret < 0 && ifp->ifindex > 0)
    {
      return NSM_ERR_INTERNAL;
    }

  zif->agg.psc = psc;

#ifdef HAVE_HA
  nsm_lacp_cal_modify_nsm_if_lacp (zif);
#endif /* HAVE_HA */
  return NSM_SUCCESS;
}

char *
nsm_lacp_psc_string (int psc)
{
  switch (psc)
    {
    case HAL_LACP_PSC_DST_MAC:
      return "Destination Mac address";
    case HAL_LACP_PSC_SRC_MAC:
      return "Source Mac address";
    case HAL_LACP_PSC_SRC_DST_MAC:
      return "Source and Destination Mac address";
    case HAL_LACP_PSC_DST_IP:
      return "Destination IP address";
    case HAL_LACP_PSC_SRC_IP:
      return "Source IP address";
    case HAL_LACP_PSC_SRC_DST_IP:
      return "Source and Destination IP address";
    case HAL_LACP_PSC_DST_PORT:
      return "Destination TCP/UDP Port";
    case HAL_LACP_PSC_SRC_PORT:
      return "Source TCP/UDP Port";
    case HAL_LACP_PSC_SRC_DST_PORT:
      return "Source and Destination TCP/UDP Port";
    default :
      break;
    }
  
  return "None";
}
int
nsm_vlan_agg_update_config_flag (struct interface *agg_ifp)
{
  struct nsm_if *agg_nif = NULL;
  struct nsm_vlan_port *agg_vlan_port = NULL;
  struct nsm_bridge_port *agg_br_port = NULL;
  struct interface *memifp = NULL;
  struct nsm_if *mem_nif = NULL;
  struct nsm_vlan_port *mem_vlan_port = NULL;
  struct nsm_bridge_port *mem_br_port = NULL;
  struct listnode *ln = NULL;
  u_int16_t key;

  if (agg_ifp == NULL)
    return RESULT_ERROR;

  agg_nif = (struct nsm_if *) agg_ifp->info;

  if (agg_nif == NULL || agg_nif->switchport == NULL)
    return RESULT_ERROR;

  agg_br_port = agg_nif->switchport;
  agg_vlan_port = &agg_br_port->vlan_port;

  if (NSM_INTF_TYPE_AGGREGATOR(agg_ifp))
    {
      AGGREGATOR_MEMBER_LOOP (agg_nif, memifp, ln)
        {
          mem_nif = memifp->info;

          if ((mem_nif == NULL) || (mem_nif->switchport == NULL))
            continue;

          mem_br_port = mem_nif->switchport;
          mem_vlan_port = &mem_br_port->vlan_port;

          if (mem_vlan_port)
            {
              mem_vlan_port->config = agg_vlan_port->config;
              key = memifp->lacp_admin_key;

              nsm_lacp_if_admin_key_set (memifp);
            if (key != memifp->lacp_admin_key)
               nsm_server_if_lacp_admin_key_update (memifp);
              }
            }
        }
  return RESULT_OK;
}

int
nsm_lacp_send_aggregate_member (char *br_name, struct interface *mem_ifp,
                                bool_t add)
{
  struct nsm_msg_bridge_if msg;
  int nbytes, i;
  struct nsm_server_client *nsc;
  struct nsm_server_entry *nse;

  if (mem_ifp == NULL)
    return -1;

  NSM_SERVER_CLIENT_LOOP (ng, i, nsc, nse)
    if (NSM_CHECK_CTYPE (nse->service.bits, NSM_SERVICE_BRIDGE) &&
        (nse->service.protocol_id == APN_PROTO_ONM))
      {
        pal_strncpy (msg.bridge_name, br_name, NSM_BRIDGE_NAMSIZ);
        msg.num = 1;

        /* Allocate ifindex array. */
        msg.ifindex = XCALLOC (MTYPE_TMP, msg.num * sizeof (u_int32_t));

        /* Member port */
        msg.ifindex[0] = mem_ifp->ifindex;

        /* Set nse pointer and size. */
        nse->send.pnt = nse->send.buf + NSM_MSG_HEADER_SIZE;
        nse->send.size = nse->send.len - NSM_MSG_HEADER_SIZE;
        msg.spanning_tree_disable = PAL_FALSE;
#if defined (HAVE_PROVIDER_BRIDGE) || defined (HAVE_I_BEB)
        msg.cust_bpdu_process = 0;
#endif /* HAVE_PROVIDER_BRIDGE || HAVE_I_BEB */
        /* Encode NSM bridge interface message. */
        nbytes = nsm_encode_bridge_if_msg (&nse->send.pnt, &nse->send.size,
                                           &msg);
        if (nbytes < 0)
          return nbytes;

        /* Send bridge interface message. */
        if (add == PAL_TRUE)
          {
            nsm_server_send_message (nse, 0, 0,
                                     NSM_MSG_LACP_ADD_AGGREGATOR_MEMBER, 0,
                                     nbytes);
          }
        else
          {
            nsm_server_send_message (nse, 0, 0, 
                                     NSM_MSG_LACP_DEL_AGGREGATOR_MEMBER, 0, 
                                     nbytes);
          }

        /* Free ifindex array. */
        XFREE (MTYPE_TMP, msg.ifindex);
      }
  
  return 0;
}

//#ifdef  HAVE_L2
#if 0
void
nsm_bridge_if_send_state_sync_req_wrap ( struct interface *ifp)
{
  struct nsm_if *zif = NULL;

  if ((ifp == NULL) ||
     (! CHECK_FLAG (ifp->status, NSM_INTERFACE_ACTIVE)) ||
     (ifp->type != NSM_IF_TYPE_L2 ))
    {
      return RESULT_ERROR;
    }

  zif = ifp->info;
  if ((zif) && (zif->bridge))
    nsm_bridge_if_send_state_sync_req (zif->bridge, ifp);
}
#endif /* HAVE_L2 */


