/* Copyright (C) 2004   All Rights Reserved.*/

#include "pal.h"

#include "nsm_message.h"
#include "nsm_client.h"
#include "table.h"
#include "if.h"
#include "hash.h"

#ifdef HAVE_IMI
#include "imi_client.h"
#endif

#include "lacp_types.h"
#include "lacpd.h"
#include "lacp_link.h"
#include "lacp_debug.h"
#include "lacp_selection_logic.h"
#include "lacp_nsm.h"
#include "lacp_cli.h"
#include "lacp_rcv.h"
#include "lacp_periodic_tx.h"
#include "lacp_mux.h"

#ifdef HAVE_HA
#include "lacp_cal.h"
#include "cal_api.h"
#endif /* HAVE_HA */

#ifdef HAVE_SMI
#include "lib_fm_api.h"
#include "smi_lacp_fm.h"
#endif /* HAVE_SMI */

#ifdef HAVE_IMI
#include "imi_client.h"
#endif /* HAVE_IMI */

int
lacp_nsm_recv_interface_add (struct interface *ifp)
{
  struct lacp_link *link;

  if (if_is_loopback (ifp))
    return 0;

  /* set system mac address */
  if (! FLAG_ISSET (lacp_instance->flags, LACP_SYSTEM_ID_ACTIVE))
    {
      lacp_system_mac_address_set (ifp->hw_addr);
    }

  /* get lacp link */
  link = lacp_find_link_by_name (ifp->name);
  if (link)
    {
#ifdef HAVE_HA
      if (FLAG_ISSET (link->flags, LACP_HA_LINK_STALE_FLAG))
        {
          UNSET_FLAG (link->flags, LACP_HA_LINK_STALE_FLAG);
        }
      else
#endif /* HAVE_HA */
        {
          lacp_link_port_attach (link, ifp);
        }
#ifdef HAVE_HA
      lacp_cal_modify_lacp_link (link);
#endif /* HAVE_HA */
    }
  else
    {
      struct lacp_aggregator *agg;

      /* lookup aggragator */
      agg = lacp_find_aggregator_by_name (ifp->name);
      if (agg != NULL)
        {
          agg->agg_ix = ifp->ifindex;
          agg->ifp = ifp;
#ifdef HAVE_HA
          UNSET_FLAG (agg->flag, LACP_HA_AGG_STALE_FLAG);
          lacp_cal_modify_lacp_agg (agg);
#endif /* HAVE_HA */
        }
    }

#ifdef HAVE_HA
  lacp_cal_modify_instance ();
#endif /* HAVE_HA */

  return 0;
}

int
lacp_nsm_recv_interface_delete (struct interface *ifp)
{
  struct lacp_link *link;

  if (if_is_loopback (ifp))
    return 0;

  /* update system mac address */
  if (FLAG_ISSET (lacp_instance->flags, LACP_SYSTEM_ID_ACTIVE) &&
      pal_mem_cmp (ifp->hw_addr, LACP_SYSTEM_ID, LACP_GRP_ADDR_LEN) == 0)
    {
      /* reset system mac */
      lacp_system_mac_address_unset ();

      /* get a new system mac addr */
      lacp_system_mac_address_update ();
    }

  /* detach port from corresponding lacp link */
  link = lacp_find_link (ifp->ifindex);
  if (link != NULL)
    {
      lacp_delete_link (link, PAL_FALSE);
    }
  else
    {
      struct lacp_aggregator *agg;

      /* lookup aggragator */
      agg = lacp_find_aggregator_by_name (ifp->name);
      if (agg != NULL)
        {
          agg->ifp = NULL;
          UNSET_FLAG (agg->flag, LACP_AGG_FLAG_INSTALLED);
          lacp_aggregator_delete_process (agg, PAL_FALSE);
        }
    }

#ifdef HAVE_HA
  lacp_cal_modify_instance ();
#endif /* HAVE_HA */

  return 0;
}

/* Interface up/down message from NSM.  */
int
lacp_nsm_recv_interface_state (struct nsm_msg_header *header,
                               void *arg, void *message)
{
  struct nsm_client *nc;
  struct lib_globals *zg;
  struct nsm_client_handler * nch;
  struct nsm_msg_link *msg;
  struct interface *ifp, if_tmp;
  struct lacp_link *link;
  int old_state, new_state;

  msg = message;
  nch = arg;
  nc = nch->nc;
  zg = nc->zg;

  /* Look up interface */
  ifp = NULL;
  if (msg->ifname)
    {
      ifp = ifg_lookup_by_name (&zg->ifg, msg->ifname);
    }
  else
    {
      ifp = ifg_lookup_by_index (&zg->ifg, msg->ifindex);
    }

  if (! ifp)
    return 0;

  /* Drop the message for unbound interfaces.  */
  if (ifp->vr == NULL)
    return 0;

  if (LACP_DEBUG(EVENT))
    {
      zlog_debug (LACPM,
                  "Interface state update for port %s - %s",
                  ifp->name,
                  (header->type == NSM_MSG_LINK_UP) ? "up" : "down");
    }

  /* store old link operational status */
  old_state = if_is_up (ifp);

  /* copy interface */
  pal_mem_cpy (&if_tmp, ifp, sizeof (struct interface));

  /* Set new value to interface structure.  */
  nsm_util_interface_state (msg, &zg->ifg);

  link = lacp_find_link (ifp->ifindex);
  if (link != NULL)
    {
      /* new port operational status */
      new_state = if_is_up (ifp);

      /* enable/disable lacp link status on change in underlying port status */
      if (old_state != new_state)
        {
          if (new_state)
            {
              link->port_enabled = PAL_TRUE;
              lacp_selection_logic_add_link (link);
              lacp_initialize_link (link);
              /* lacp_attempt_to_aggregate_link (link); */
            }
          else
            {
              bool_t link_enabled;

              link_enabled = IS_LACP_LINK_ENABLED (link);
              link_cleanup (link, PAL_TRUE);
              link->port_enabled = PAL_FALSE;
              lacp_rcv_sm (link);
              lacp_periodic_tx_sm (link);
              lacp_mux_sm (link);

              if (link_enabled)
                lacp_update_selection_logic ();
            }
        }

      /* set new link admin key */
      if (if_tmp.lacp_admin_key != ifp->lacp_admin_key)
        {
          if (link->aggregator)
            {
        if (ifp->agg_param_update == 0)
                lacp_disaggregate_link (link, PAL_TRUE);
              link->ntt = PAL_TRUE;
            }
        }
#ifdef HAVE_HA
      lacp_cal_modify_lacp_link (link);
#endif /* HAVE_HA */
    }

#ifdef HAVE_HA
  lacp_cal_modify_instance ();
#endif /* HAVE_HA */

  return 0;
}

/* Callback function for updating the LACP aggregator count when a static */
/* aggregator is added to nsm so that the total aggregator count in nsm */
/* and LACP remains the same*/
int
lacp_nsm_recv_static_agg_cnt_update (struct nsm_msg_header *header,
                                     void *arg, void *message)
{
   struct nsm_msg_agg_cnt *msg = NULL;

   msg = message;

   if (!msg)
     return 0;
   if (msg->agg_msg_type == NSM_MSG_INCR_AGG_CNT)
     {
       lacp_instance->agg_cnt++;
     }
   else
     {
       if (lacp_instance->agg_cnt > 0)
         lacp_instance->agg_cnt--;
     }

#ifdef HAVE_HA
  lacp_cal_modify_instance ();
#endif /* HAVE_HA */

   return 0;
}

/* Callback for configuration of lacp aggregator from nsm */
int
lacp_nsm_recv_aggregator_config (struct nsm_msg_header *header,
                               void *arg, void *message)
{
  struct nsm_msg_lacp_aggregator_config *msg;
  struct interface *ifp = NULL;
  struct nsm_client *nc;
  struct lib_globals *zg;
  struct nsm_client_handler * nch;
  int ret = 0;
  struct lacp_link *link;

  msg = message;
  nch = arg;
  nc = nch->nc;
  zg = nc->zg;

  if (!msg)
    return 0;

  ifp = ifg_lookup_by_index (&zg->ifg, msg->ifindex);
  if (! ifp)
    return -1;

  if (msg->add_agg)
    {
     /* get lacp link */
      link = lacp_find_link_by_name (ifp->name);
      if (link == NULL)
        {
          ret = lacp_add_link (ifp->name, &link);
          if (ret != RESULT_OK || link == NULL)
            return -1;
        }
#ifdef HAVE_HA
      else
        {
          if (FLAG_ISSET (link->flags, LACP_HA_LINK_STALE_FLAG))
            {
              UNSET_FLAG (link->flags, LACP_HA_LINK_STALE_FLAG);
            }
          if ((link->aggregator != NULL)
              && (FLAG_ISSET (link->aggregator->flag, LACP_HA_AGG_STALE_FLAG)))
            {
              UNSET_FLAG (link->aggregator->flag, LACP_HA_AGG_STALE_FLAG);
            }
        }
#endif /* HAVE_HA */

     /* set channel mode */
     ret = lacp_set_channel_activity (link, msg->chan_activate);
     if (ret != RESULT_OK)
       return -1;

#ifdef HAVE_HA
     if ((link->actor_admin_port_key == msg->key))
       return RESULT_OK;
#endif /* HAVE_HA */

     if (lacp_set_channel_admin_key (link, msg->key) != RESULT_OK)
       return -1;

     if (ifp && IS_LACP_LINK_ENABLED (link))
       {
         lacp_update_selection_logic ();
         lacp_initialize_link (link);
       }
#ifdef HAVE_HA
     lacp_cal_modify_lacp_link (link);
#endif /* HAVE_HA */

    }
  else
    {
      /* get lacp link */
      link = lacp_find_link_by_name (ifp->name);
      if (link)
        {
          if (lacp_delete_link (link, IS_LACP_LINK_ENABLED (link)) != RESULT_OK)
            return -1;
        }
    }

  return 0;
}

int
lacp_nsm_recv_msg_done (struct nsm_msg_header *header,
                        void *arg, void *message)
{
  struct nsm_client * nc;
  struct lib_globals *zg;
  struct nsm_client_handler * nch;

  if (arg == NULL)
      return -1;

  nch = arg;
  nc = nch->nc;
  zg = nc->zg;

#ifdef HAVE_IMI
  /* Calling the imi to send the ONMD configuration again */
  imi_client_send_config_request(zg->vr_in_cxt);
#endif /* HAVE_IMI  */
  return 0;
}

int
lacp_nsm_aggregator_add (struct lacp_aggregator *agg)
{
  int ret;
  struct nsm_msg_lacp_agg_add msg;

  pal_mem_set (&msg, 0, sizeof (struct nsm_msg_lacp_agg_add));

  pal_strncpy (msg.agg_name, agg->name, LACP_AGG_NAME_LEN);

  if (agg->linkCnt > 0)
    {
      int i;

      msg.add_count = agg->linkCnt;

      msg.ports_added = XCALLOC (MTYPE_TMP,
                                 agg->linkCnt * sizeof (u_int32_t));
      if (! msg.ports_added)
        return LACP_ERR_MEM_ALLOC_FAILURE;

      for (i = 0; i < agg->linkCnt;)
        {
          if (agg->link[i] == NULL)
            continue;

          msg.ports_added[i] = agg->link[i]->ifp->ifindex;
          i++;
        }

      NSM_SET_CTYPE (msg.cindex, NSM_LACP_AGG_CTYPE_PORTS_ADDED);
    }

  /* send aggregator add msg */
  ret = nsm_client_send_lacp_aggregator_add (LACP_NSM_CLIENT, &msg);
  if (ret < 0)
    return ret;

  return RESULT_OK;
}


int
lacp_nsm_aggregator_delete (struct lacp_aggregator *agg)
{
  int ret;

  /* send aggregator del msg to nsm */
  ret = nsm_client_send_lacp_aggregator_del (LACP_NSM_CLIENT, agg->name);

  return ret;
}


int
lacp_nsm_aggregator_link_add (struct lacp_aggregator *agg,
                              struct lacp_link **links,
                              u_int16_t link_count)
{
  int ret;
  int i;
  struct nsm_msg_lacp_agg_add msg;

  pal_mem_set (&msg, 0, sizeof (struct nsm_msg_lacp_agg_add));

  if (!agg)
     return -1;

  pal_strncpy (msg.agg_name, agg->name, LACP_AGG_NAME_LEN);

  pal_mem_cpy (msg.mac_addr, agg->aggregator_mac_address, ETHER_ADDR_LEN);

  NSM_SET_CTYPE (msg.cindex, NSM_LACP_AGG_CTYPE_PORTS_ADDED);
  msg.add_count = link_count;

  msg.ports_added = XCALLOC (MTYPE_TMP,
                             link_count * sizeof (u_int32_t));
  if (! msg.ports_added)
    return LACP_ERR_MEM_ALLOC_FAILURE;

  for (i = 0; i < link_count; i++)
    msg.ports_added[i] = links[i]->actor_port_number;
  msg.agg_type = AGG_CONFIG_LACP;
  /* send aggregator add msg */
  ret = nsm_client_send_lacp_aggregator_add (LACP_NSM_CLIENT, &msg);
  if (ret < 0)
    return ret;

  return RESULT_OK;
}


int
lacp_nsm_enable_collecting (char *agg_name, char *link_name)
{
        return RESULT_OK;
}


int
lacp_nsm_disable_collecting (char *agg_name, char *link_name)
{
        return RESULT_OK;
}


int
lacp_nsm_enable_distributing (char *agg_name, char *link_name)
{
        return RESULT_OK;
}


int
lacp_nsm_disable_distributing (char *agg_name, char *link_name)
{
        return RESULT_OK;
}


int
lacp_nsm_collecting_distributing (u_char *agg_name, u_int32_t ifindex, int enable)
{
  return RESULT_OK;
}


int
lacp_nsm_aggregator_link_del (struct lacp_aggregator *agg,
                              struct lacp_link **links,
                              u_int16_t link_count)
{
  int ret;
  int i;
  struct nsm_msg_lacp_agg_add msg;

  pal_mem_set (&msg, 0, sizeof (struct nsm_msg_lacp_agg_add));

  pal_strncpy (msg.agg_name, agg->name, LACP_AGG_NAME_LEN);

  NSM_SET_CTYPE (msg.cindex, NSM_LACP_AGG_CTYPE_PORTS_DELETED);
  msg.del_count = link_count;

  msg.ports_deleted = XCALLOC (MTYPE_TMP,
                               link_count * sizeof (u_int32_t));
  if (! msg.ports_deleted)
    return LACP_ERR_MEM_ALLOC_FAILURE;

  for (i = 0; i < link_count; i++)
    msg.ports_deleted[i] = links[i]->actor_port_number;

  /* send aggregator add msg */
  ret = nsm_client_send_lacp_aggregator_add (LACP_NSM_CLIENT, &msg);
  if (ret < 0)
    return ret;

  return RESULT_OK;
}


void
lacp_nsm_if_table_cleanup (struct route_table *table)
{
  struct route_node *rn;

  for (rn = route_top (table); rn; rn = route_next (rn))
    {
      if (rn->info == NULL)
        continue;

      rn->info = NULL;
      route_unlock_node (rn);
    }
}

void
lacp_nsm_cleanup_interface_list ()
{
  struct interface *ifp;
  struct listnode *ln, *ln_next;
  struct apn_vr *vr;
  struct apn_vrf *vrf, *vrf_next;
  int i;

  lacp_nsm_if_table_cleanup (LACPM->ifg.if_table);

  for (i = 0; i < vector_max (LACPM->vr_vec); i++)
    {
      if ((vr = vector_slot (LACPM->vr_vec, i)) == NULL)
        continue;

      for (vrf = vr->vrf_list; vrf; vrf = vrf_next)
        {
          vrf_next = vrf->next;

          lacp_nsm_if_table_cleanup (vrf->ifv.if_table);
        }

      lacp_nsm_if_table_cleanup (vr->ifm.if_table);

      list_delete_all_node (vr->ifm.if_list);
    }

  for (ln = LISTHEAD (LACPM->ifg.if_list); ln; ln = ln_next)
    {
      ln_next = ln->next;
      ifp = GETDATA (ln);
      hash_release (LACPM->ifg.if_hash, ifp);
      ln->data = NULL;
      list_delete_node (LACPM->ifg.if_list, ln);

      /* Free connected address list.
         ifc_delete_all (ifp); */
      XFREE (MTYPE_IF, ifp);
    }

  LACPM->ifg.if_list->head = LACPM->ifg.if_list->tail = NULL;
}

int
lacp_nsm_disconnect_process ()
{
  /* unset system mac address */
  lacp_system_mac_address_unset ();
  lacp_nsm_deinit ();
  lacp_aggregator_delete_all (PAL_FALSE);
  lacp_link_port_detach_all (PAL_FALSE);

  lacp_nsm_cleanup_interface_list ();

#ifdef HAVE_SMI
  smi_record_fault(LACPM, FM_GID_LACP,
                   LACP_SMI_ALARM_NSM_SERVER_SOCKET_DISCONNECT,
                   __LINE__, __FILE__, NULL);
#endif /* HAVE_SMI */

  lacp_nsm_init ();

  return 0;
}


int
lacp_nsm_init ()
{
  if (LACP_NSM_CLIENT)
    {
      nsm_client_start (LACP_NSM_CLIENT);
      return LACP_SUCCESS;
    }

  LACP_NSM_CLIENT = nsm_client_create (LACPM, 0);
  if (! LACP_NSM_CLIENT)
    return LACP_FAILURE;

  nsm_client_set_version (LACP_NSM_CLIENT, NSM_PROTOCOL_VERSION_1);
  nsm_client_set_protocol (LACP_NSM_CLIENT, APN_PROTO_LACP);
  nsm_client_set_service (LACP_NSM_CLIENT, NSM_SERVICE_INTERFACE);

#ifndef HAVE_HA
  /* If HA is enabled, do not do cleanup when nsm is killed */
  /* Set disconnect handling callback. */
  nsm_client_set_disconnect_callback (LACP_NSM_CLIENT,
                                      lacp_nsm_disconnect_process);
#endif /* HAVE_HA */
  nsm_client_set_callback (LACP_NSM_CLIENT, NSM_MSG_LINK_UP,
                           lacp_nsm_recv_interface_state);
  nsm_client_set_callback (LACP_NSM_CLIENT, NSM_MSG_LINK_DOWN,
                           lacp_nsm_recv_interface_state);
  nsm_client_set_callback (LACP_NSM_CLIENT, NSM_MSG_STATIC_AGG_CNT_UPDATE,
                           lacp_nsm_recv_static_agg_cnt_update);
  nsm_client_set_callback (LACP_NSM_CLIENT, NSM_MSG_LACP_AGGREGATOR_CONFIG,
                           lacp_nsm_recv_aggregator_config);
  nsm_client_set_callback (LACP_NSM_CLIENT, NSM_MSG_VR_SYNC_DONE,
                           lacp_nsm_recv_msg_done);

  if_add_hook (&LACPM->ifg, IF_CALLBACK_VR_BIND,
               lacp_nsm_recv_interface_add);
  if_add_hook (&LACPM->ifg, IF_CALLBACK_VR_UNBIND,
               lacp_nsm_recv_interface_delete);

  nsm_client_start (LACP_NSM_CLIENT);

  return LACP_SUCCESS;
}


void
lacp_nsm_deinit ()
{
  if (! LACP_NSM_CLIENT)
    return;

  nsm_client_stop (LACP_NSM_CLIENT);
  nsm_client_delete (LACP_NSM_CLIENT);
  LACP_NSM_CLIENT = NULL;
}
