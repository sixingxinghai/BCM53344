/* Copyright (C) 2003  All Rights Reserved.  */

#include "pal.h"
#include "lib.h"
#include "thread.h"
#include "log.h"
#include "table.h"
#include "nsm_client.h"
#include "l2lib.h"

#include "mstp_config.h"
#include "mstp_types.h"
#include "mstpd.h"
#include "mstp_bridge.h"
#include "mstp_port.h"
#include "mstp_api.h"
#include "l2_debug.h"

#ifdef HAVE_HA
#include "mstp_cal.h"
#endif /* HAVE_HA */

#ifdef HAVE_SMI
#include "lib_fm_api.h"
#include "smi_mstp_fm.h"
#endif /* HAVE_SMI */

/* This function calculates the default path cost for a port
   based on the link speed.  See Table 8-5 802.1t.  */
u_int32_t
mstp_nsm_get_default_port_path_cost (const float bandwidth)
{
  /* Bandwidth is in bytes/sec - Table 8-5 is in bits/sec.
     N.B. We handle floating port roundoff by adding a fudge factor
     to the constants. Values 2 thru 5 should be changed to 65535
     to support bridges which only support 16 bit path cost values. */

  if (bandwidth * 8 <= 1.0 )
    return L2_DEFAULT_PATH_COST;
  else if (bandwidth * 8 <= 100000.0)
    return 200000000;
  else if (bandwidth * 8 <= 1000000.0)
    return 20000000;
  else if (bandwidth * 8 <= 10000000.0)
    return 2000000;
  else if (bandwidth * 8 <= 100000000.0 )
    return 200000;
  else if (bandwidth * 8 <= 1000000000.0)
    return 20000;
  else if (bandwidth * 8 <= 10000000000.0)
    return 2000;
  else if (bandwidth * 8 <= 100000000000.0)
    return 200;
  else if (bandwidth * 8 <= 1000000000000.0)
    return 20;
  else
    return 2;

}

u_int32_t
mstp_nsm_get_default_port_path_cost_short_method (const float bandwidth)
{
  /* Bandwidth is in bytes/sec - Get the IEEE 802.1d 1998 Table 8-5
     path cost values. */
  if (bandwidth * 8 <= 1.0)
    return L2_DEFAULT_PATH_COST_SHORT_METHOD;
  else if (bandwidth * 8 <= 4000000.0)
    return 250; /* For 4 Mb/s */
  else if (bandwidth * 8 <= 10000000.0)
    return 100; /* For 10 Mb/s */
  else if (bandwidth * 8 <= 16000000.0)
    return 62;  /* For 62 Mb/s */
  else if (bandwidth * 8 <= 100000000.0 )
    return 19;  /* For 100 Mb/s */
  else if (bandwidth * 8 <= 1000000000.0)
    return 4;   /* For 1 Gb/s */
  else
    return 2;   /* For 10 Gb/s */
}

u_int32_t
mstp_nsm_get_port_path_cost (struct lib_globals *zg, const u_int32_t ifindex)
{
  struct apn_vr *vr = apn_vr_get_privileged (zg);
  struct interface * ifp = NULL;
  struct mstp_port *port = NULL;

  /* Get the bandwidth of the interface. */
  ifp = if_lookup_by_index (&vr->ifm, ifindex);
  if (!ifp)
    {
      /* Default path cost */
      return L2_DEFAULT_PATH_COST;
    }
   port = ifp->port;
  if (!port || !(port->br))
    return L2_DEFAULT_PATH_COST;

  if (port->br->path_cost_method == MSTP_PATHCOST_LONG)
    /* Now get the default path cost as specified by IEEE 802.1t Table 8-5. */
    return mstp_nsm_get_default_port_path_cost (ifp->bandwidth);
  else
    /* Now get the default pathcost as specified by IEEE 802.1d 1998 Table 8-5*/
    return mstp_nsm_get_default_port_path_cost_short_method (ifp->bandwidth);
}

#ifdef HAVE_IMI
static int
mstp_nsm_recv_msg_imi_sync_done (struct nsm_msg_header *header,
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
  if (LAYER2_DEBUG(proto, PROTO))
    zlog_debug (zg,"NSM_MSG_VR_SYNC_DONE MSG received start reading IMI configuration\n");

  /* Calling IMI to send the MSTP configuration again */
  imi_client_send_config_request (zg->vr_in_cxt);
#endif /* HAVE_IMI */

  return 0;
}
#endif /* HAVE_IMI */

/*Message from NSM to update the ageing time*/
static int
mstp_nsm_recv_bridge_ageing_time_set (struct nsm_msg_header *header,
                         void *arg, void *message)
{
  struct nsm_client * nc;
  struct lib_globals *zg;
  struct nsm_client_handler * nch;
  struct nsm_msg_bridge *msg;
  struct mstp_bridge *br = NULL;

  msg = message;
  nch = arg;
  nc = nch->nc;
  zg = nc->zg;

  br = ( struct mstp_bridge * ) mstp_find_bridge ( msg->bridge_name );

  if ( br == NULL )
    {
      return RESULT_ERROR;
    }

  br->ageing_time = msg->ageing_time;

#ifdef HAVE_HA
  mstp_cal_modify_mstp_bridge (br);
#endif /* HAVE_HA */

  return RESULT_OK;
}

/* Interface update message from NSM.  */
static int
mstp_nsm_recv_interface_update (struct interface *ifp)
{
  struct apn_vr *vr = ifp->vr;

  if (LAYER2_DEBUG(proto, PROTO))
    zlog_debug (vr->zg, "Interface update for port %s", ifp->name);

  /* Activate the new interface */
  mstp_interface_add (ifp);

  return RESULT_OK;
}

/* Interface delete message from NSM.  */
static int
mstp_nsm_recv_interface_delete (struct interface *ifp)
{
  struct apn_vr *vr = ifp->vr;
  struct mstp_bridge *bridge;
  struct mstp_port *port;

  if (LAYER2_DEBUG(proto, PROTO))
    zlog_debug (vr->zg, "Interface delete for port %s", ifp->name);

  port = ifp->port;

  /* Delete the customer edge bridge associated with the
   * customer edge port.
   */

  if (port != NULL
      && port->port_type == MSTP_VLAN_PORT_MODE_CE)
    mstp_bridge_delete (NULL, port);

  for (bridge = mstp_get_first_bridge (); bridge; bridge = bridge->next)
    {
      if (mstp_find_cist_port (bridge, ifp->ifindex) != NULL)
        {
          mstp_delete_port (bridge->name, ifp->name, MSTP_VLAN_NULL_VID,
                            MST_INSTANCE_IST, PAL_TRUE, PAL_FALSE);
          break;
        }
    }

  return RESULT_OK;
}

/* Interface up/down message from NSM.  */
static int
mstp_nsm_recv_interface_state (struct nsm_msg_header *header,
                               void *arg, void *message)
{
  int up;
  int changed;
  struct nsm_client * nc;
  struct lib_globals *zg;
  struct interface * ifp;
  struct mstp_bridge *br;
  struct mstp_port *port;
  struct nsm_msg_link *msg;
  struct nsm_client_handler * nch;

  msg = message;
  nch = arg;
  nc = nch->nc;
  zg = nc->zg;

  /* Look up interface so that check current status.  */
  ifp = ifg_lookup_by_index (&zg->ifg, msg->ifindex);
  if (!ifp)
    return 0;

  /* Drop the message for unbound interfaces.  */
  if (ifp->vr == NULL)
    return 0;

  /* Record the interface is up or not.  */
  up = if_is_up (ifp);

  /* Set new value to interface structure.  */
  nsm_util_interface_state (msg, &zg->ifg);

  changed = (if_is_up (ifp) != up);

  if (LAYER2_DEBUG(proto, PROTO))
    {
      zlog_debug (zg,
                  "Interface state update for port %s - %s - bw %f - changed %s",
                  ifp->name,
                  (header->type == NSM_MSG_LINK_UP) ? "up" : "down",
                  ifp->bandwidth, changed ? "yes" : "no");
    }

  /* Elide non-bridge ports. */
  if (!ifp->port)
    return 0;

  port = ifp->port;

  if (header->type == NSM_MSG_LINK_UP)
    {
      /* Get the path cost values for the new bandwidth
       * and set it for the bridge associated with this port. */
      if (!port->pathcost_configured)
         mstp_set_cist_port_path_cost (ifp->port);

      mstp_set_msti_port_path_cost (port);

      if (port->port_type == MSTP_VLAN_PORT_MODE_CE
          && port->ce_br != NULL)
        {
          mstp_enable_bridge (port->ce_br);

#ifdef HAVE_PROVIDER_BRIDGE
          if (port->ce_br->cust_bpdu_process
                      != NSM_MSG_BRIDGE_PROTO_PEER)
            mstp_disable_bridge (port->ce_br, PAL_TRUE);
#endif /* HAVE_PROVIDER_BRIDGE */

        }
      else if (port->spanning_tree_disable
              || ((br = port->br) != NULL
                  && br->mstp_enabled == PAL_FALSE
                  && br->mstp_brforward == PAL_TRUE))
        mstp_cist_handle_port_forward (port);
      else
        mstp_cist_enable_port(ifp->port);

      if (port->admin_p2p_mac == MSTP_ADMIN_LINK_TYPE_AUTO)
        mstp_api_set_port_p2p (ifp->bridge_name, ifp->name,
                               MSTP_ADMIN_LINK_TYPE_AUTO);

      zlog_warn (zg, "Port up notification received for port %s ",
                 ifp->name);
    }
  else if (header->type == NSM_MSG_LINK_DOWN)
    {
      if (!port->pathcost_configured)
        {
         if (port->br->path_cost_method == MSTP_PATHCOST_SHORT)
           {
             /* Get the 802.1d recommended path cost values for the new bandwidth
              * and set it for the bridge associated with this port. */
             port->cist_path_cost =
                   mstp_nsm_get_default_port_path_cost_short_method (ifp->bandwidth);
           }
         else
           {
             /* Get the 802.1t recommended path cost values for the new bandwidth
              * and set it for the bridge associated with this port*/
             port->cist_path_cost =
                   mstp_nsm_get_default_port_path_cost (ifp->bandwidth);
           }
        }

#ifdef HAVE_HA
      mstp_cal_modify_mstp_port (port);
#endif /* HAVE_HA */

      if (port->port_type == MSTP_VLAN_PORT_MODE_CE
          && port->ce_br != NULL)
        mstp_disable_bridge (port->ce_br, PAL_FALSE);
      else
        mstp_cist_disable_port (ifp->port);

      zlog_warn (zg, "Port down notification received for port %s ",
                 ifp->name);
      if (port->admin_p2p_mac == MSTP_ADMIN_LINK_TYPE_AUTO)
        mstp_api_set_port_p2p (ifp->bridge_name, ifp->name,
                               MSTP_ADMIN_LINK_TYPE_AUTO);
    }
  else
    {
      zlog_warn (zg, "Unknown msg_type received for port %s - %d",
                 ifp->name, header->type);
    }

  return 0;
}

/* Bridge add message from NSM. */
static int
mstp_nsm_recv_bridge_add (struct nsm_msg_header *header,
                          void *arg, void *message)
{
  struct nsm_client * nc;
  struct lib_globals *zg;
  struct nsm_client_handler * nch;
  struct nsm_msg_bridge *msg;
  int ret;

  msg = message;
  nch = arg;
  nc = nch->nc;
  zg = nc->zg;

#if (defined HAVE_I_BEB)
  ret = mstp_bridge_add (msg->bridge_name, msg->type,
                         PAL_FALSE, PAL_FALSE, msg->is_default,
                         msg->topology_type, NULL);
#else
  ret = mstp_bridge_add (msg->bridge_name, msg->type,
                         PAL_FALSE, msg->is_default,
                         msg->topology_type, NULL);
#endif

  if (ret == RESULT_OK)
    mstp_bridge_configuration_activate (msg->bridge_name, msg->type);

  return 0;
}

/* Bridge delete message from NSM. */
static int
mstp_nsm_recv_bridge_delete (struct nsm_msg_header *header,
                             void *arg, void *message)
{
  struct nsm_client * nc;
  struct lib_globals *zg;
  struct nsm_client_handler * nch;
  struct nsm_msg_bridge *msg;

  msg = message;
  nch = arg;
  nc = nch->nc;
  zg = nc->zg;

  mstp_bridge_delete (msg->bridge_name, NULL);

  return 0;
}

/* Bridge add if message from NSM. */
static int
mstp_nsm_recv_bridge_add_if (struct nsm_msg_header *header,
                             void *arg, void *message)
{
  struct nsm_client * nc;
  struct lib_globals *zg;
  struct nsm_client_handler * nch;
  struct nsm_msg_bridge_if *msg;
  struct interface * ifp;
  int i;
  struct port_instance_list *pinst_list = NULL;
  struct mstp_interface *mstpif ;
#ifdef HAVE_RPVST_PLUS
  struct mstp_bridge * br = NULL;
#endif /* HAVE_RPVST_PLUS */

  msg = message;
  nch = arg;
  nc = nch->nc;
  zg = nc->zg;

  if (NSM_CHECK_CTYPE(msg->cindex, NSM_MSG_BRIDGE_CTYPE_PORT))
    {
      for (i = 0; i < msg->num; i++)
        {
          if ((ifp = ifg_lookup_by_index(&mstpm->ifg, msg->ifindex[i])))
            {
              /* Add bridge port. */
              mstp_api_add_port (msg->bridge_name, ifp->name,
                                 MSTP_VLAN_NULL_VID, MST_INSTANCE_IST,
                                 msg->spanning_tree_disable);

              mstpif = ifp->info;
              if (!mstpif)
                break;

              SET_FLAG (mstpif->config, MSTP_IF_CONFIG_BRIDGE_GROUP);

              mstp_activate_interface (mstpif);

              /* Add all the stored instances to the port */
              if (pal_mem_cmp (mstpif->bridge_group, msg->bridge_name,L2_BRIDGE_NAME_LEN) == 0)
                {
                  if (CHECK_FLAG (mstpif->config, MSTP_IF_CONFIG_BRIDGE_GROUP_INSTANCE))
                    if (mstpif->instance_list != NULL)
                      for (pinst_list = mstpif->instance_list; pinst_list;
                                pinst_list = pinst_list->next)
                          {
#ifdef HAVE_RPVST_PLUS
                         br = mstp_find_bridge (msg->bridge_name);
                         if (br != NULL && br->type == NSM_BRIDGE_TYPE_RPVST_PLUS)
                           {
                             mstp_api_add_port (mstpif->bridge_group,
                                                mstpif->ifp->name,
                                                pinst_list->instance_info.vlan_id,
                                                pinst_list->instance_info.instance,
                                                msg->spanning_tree_disable);
                           }
                        else
#endif /* HAVE_RPVST_PLUS */
                         mstp_api_add_port (mstpif->bridge_group,
                                            mstpif->ifp->name,
                                            MSTP_VLAN_NULL_VID,
                                            pinst_list->instance_info.instance,
                                            msg->spanning_tree_disable);
                          }

                  mstpif->active = PAL_TRUE;

                  /* Free the memory allocated for storing instance related
                   * information if any.
                   */
                  if (mstpif)
                    mstpif_free_instance_list (mstpif);

                }


            }
        }
    }

  if (msg->ifindex)
    XFREE (MTYPE_TMP, msg->ifindex);

  return 0;
}

/* Bridge delete if message from NSM. */
static int
mstp_nsm_recv_bridge_delete_if (struct nsm_msg_header *header,
                                void *arg, void *message)
{
  struct nsm_client * nc;
  struct lib_globals *zg;
  struct nsm_client_handler * nch;
  struct nsm_msg_bridge_if *msg;
  struct interface *ifp;
  int i;

  msg = message;
  nch = arg;
  nc = nch->nc;
  zg = nc->zg;


  msg = message;
  nch = arg;
  nc = nch->nc;
  zg = nc->zg;

  if (NSM_CHECK_CTYPE(msg->cindex, NSM_MSG_BRIDGE_CTYPE_PORT))
    {
      for (i = 0; i < msg->num; i++)
        {
          if ((ifp = ifg_lookup_by_index(&mstpm->ifg, msg->ifindex[i])))
            {
              /* Delete bridge port. */
              mstp_delete_port (msg->bridge_name, ifp->name, MSTP_VLAN_NULL_VID,
                                MST_INSTANCE_IST, PAL_FALSE, PAL_TRUE);
            }
        }
    }

  if (msg->ifindex)
    XFREE (MTYPE_TMP, msg->ifindex);

  return 0;
}

/* PORT update spanning-tree if receive message from NSM */
static int
mstp_nsm_recv_bridge_port_spanning_tree_enable (struct nsm_msg_header *header,
                                                void *arg, void *message)
{
  struct nsm_client *nc = NULL;
  struct lib_globals *zg = NULL;
  struct nsm_client_handler *nch = NULL;
  struct nsm_msg_bridge_if *msg = NULL;
  struct interface *ifp = NULL;
  int i;

  msg = message;
  nch = arg;
  nc = nch->nc;
  zg = nc->zg;

  if (NSM_CHECK_CTYPE(msg->cindex, NSM_MSG_BRIDGE_CTYPE_PORT))
    {
      for (i = 0; i < msg->num; i++)
        {
          /* send the message to enable/disable the spanning-tree*/
          if ((ifp = ifg_lookup_by_index(&mstpm->ifg, msg->ifindex[i])))
            mstp_port_update_spanning_tree_enable (msg->bridge_name, ifp->name, 
                                                   msg->spanning_tree_disable);
        }
    }

  if (msg->ifindex)
    XFREE (MTYPE_TMP, msg->ifindex);

  return RESULT_OK;
}  

/* Bridge if state sync message from NSM. */
static int
mstp_nsm_recv_bridge_if_state_sync_req (struct nsm_msg_header *header,
                                       void *arg, void *message)
{
  struct nsm_client * nc;
  struct lib_globals *zg;
  struct nsm_client_handler * nch;
  struct nsm_msg_bridge_if *msg;
  int i;

  msg = message;
  nch = arg;
  nc = nch->nc;
  zg = nc->zg;

  msg = message;
  nch = arg;
  nc = nch->nc;
  zg = nc->zg;

  if (NSM_CHECK_CTYPE(msg->cindex, NSM_MSG_BRIDGE_CTYPE_PORT))
    {
      for (i = 0; i < msg->num; i++)
        {
          /* bridge port state sync send. */
          mstp_bridge_port_state_send (msg->bridge_name, msg->ifindex[i]);
        }
    }

  if (msg->ifindex)
    XFREE (MTYPE_TMP, msg->ifindex);

  return 0;
}

#ifdef HAVE_VLAN
static int
mstp_nsm_recv_bridge_add_vlan (struct nsm_msg_header *header,
                               void *arg, void *message)
{
  struct nsm_client * nc;
  struct lib_globals *zg;
  struct nsm_client_handler * nch;
  struct nsm_msg_vlan *msg;
  struct mstp_bridge *br;
  struct mstp_vlan *v;
  struct mstp_prefix_vlan p;
  struct route_node *rn;
  int state;
  int ret;

  msg = message;
  nch = arg;
  nc = nch->nc;
  zg = nc->zg;
  
  if (msg == NULL)
    return 0;

  if (msg->flags & NSM_MSG_VLAN_STATE_ACTIVE)
    state = 1;
  else
    state = 0;

  br = mstp_find_bridge (msg->bridge_name);

  if (! br)
    return RESULT_ERROR;

  if (br->vlan_table)
    {
      MSTP_PREFIX_VLAN_SET (&p, msg->vid);
      rn = route_node_lookup (br->vlan_table, (struct prefix *) &p);
      if (rn)
        {
          v = rn->info;
          if (v)
            {
              if (pal_strcmp (v->name, msg->vlan_name) == 0
                  && v->vid == msg->vid && (v->flags & state))
                {
#ifdef HAVE_HA
                  UNSET_FLAG (v->flags, MSTP_VLAN_HA_STALE);
#endif /* HAVE_HA */
                  return 0;
                }
              else if ((pal_strcmp (v->name, msg->vlan_name) != 0)
                       && v->vid == msg->vid && v->flags & state)
                {
                  if (msg)
                    {
                      pal_strcpy (v->name, msg->vlan_name);
                      return 0;
                    }
                }
            }
        }
    }

  /* Add VLAN. */
  ret = mstp_bridge_vlan_add (msg->bridge_name, msg->vid, msg->vlan_name, state);

  if (ret != RESULT_ERROR)
    {
      mstp_vlan_add_event (msg->bridge_name, msg->vid, 0);
      mstp_bridge_instance_activate (msg->bridge_name, msg->vid);
    }

  return 0;
}

static int
mstp_nsm_recv_bridge_delete_vlan (struct nsm_msg_header *header,
                                  void *arg, void *message)
{
  int ret;
  struct nsm_client * nc;
  struct lib_globals *zg;
  struct nsm_client_handler * nch;
  struct nsm_msg_vlan *msg;

  msg = message;
  nch = arg;
  nc = nch->nc;
  zg = nc->zg;

  /* Delete VLAN. */
  ret = mstp_bridge_vlan_delete (msg->bridge_name, msg->vid);
  if (ret == RESULT_OK)
    mstp_vlan_delete_event (msg->bridge_name, msg->vid);

  return 0;
}

static int
mstp_nsm_recv_vlan_add_port (struct nsm_msg_header *header,
                            void *arg, void *message)
{
  int i;
  u_int16_t vid;
  struct interface *ifp;
  struct nsm_client * nc;
  struct lib_globals *zg;
  struct mstp_port *port;
  struct nsm_msg_vlan_port *msg;
  struct nsm_client_handler * nch;

  msg = message;
  nch = arg;
  nc  = nch->nc;
  zg  = nc->zg;

  for (i = 0; i < msg->num; i++)
    {
      if ((ifp = ifg_lookup_by_index(&mstpm->ifg, msg->ifindex))
           && (port = ifp->port))
        {
          /* Extract vid. */
          vid = NSM_VLAN_VID_MASK & msg->vid_info[i];
          MSTP_VLAN_BMP_SET(port->vlanMemberBmp, vid);
        }
    }
  XFREE (MTYPE_TMP, msg->vid_info);

  return 0;
}

static int
mstp_nsm_recv_vlan_delete_port (struct nsm_msg_header *header,
                               void *arg, void *message)
{
  int i;
  u_int16_t vid;
  struct interface *ifp;
  struct mstp_port *port;
  struct lib_globals *zg;
  struct nsm_client * nc;
  struct nsm_msg_vlan_port *msg;
  struct nsm_client_handler * nch;

  msg = message;
  nch = arg;
  nc = nch->nc;
  zg = nc->zg;

  for (i = 0; i < msg->num; i++)
    {
      if ((ifp = ifg_lookup_by_index(&mstpm->ifg, msg->ifindex))
           && (port = ifp->port))
        {
          /* Extract vid. */
          vid = NSM_VLAN_VID_MASK & msg->vid_info[i];
          MSTP_VLAN_BMP_UNSET(port->vlanMemberBmp, vid);
        }
    }

  XFREE (MTYPE_TMP, msg->vid_info);

  return 0;
}

static int
mstp_nsm_recv_svlan_add_ce_port (struct nsm_msg_header *header,
                                 void *arg, void *message)
{
  int i;
  u_int16_t vid;
  struct interface *ifp;
  struct nsm_client * nc;
  struct lib_globals *zg;
  struct mstp_port *port;
  struct nsm_msg_vlan_port *msg;
  struct nsm_client_handler * nch;

  msg = message;
  nch = arg;
  nc = nch->nc;
  zg = nc->zg;

  for (i = 0; i < msg->num; i++)
    {
      if ((ifp = ifg_lookup_by_index(&mstpm->ifg, msg->ifindex))
           && (port = ifp->port))
        {
          /* Extract vid. */
          vid = NSM_VLAN_VID_MASK & msg->vid_info[i];
          MSTP_VLAN_BMP_SET(port->svlanMemberBmp, vid);
          mstp_add_ce_br_port (port->ce_br, ifp->name, vid);
        }
    }

  XFREE (MTYPE_TMP, msg->vid_info);

  return 0;
}

static int
mstp_nsm_recv_svlan_delete_ce_port (struct nsm_msg_header *header,
                                    void *arg, void *message)
{
  int i;
  u_int16_t vid;
  struct interface *ifp;
  struct mstp_port *port;
  struct lib_globals *zg;
  struct nsm_client * nc;
  struct nsm_msg_vlan_port *msg;
  struct nsm_client_handler * nch;

  msg = message;
  nch = arg;
  nc = nch->nc;
  zg = nc->zg;

  for (i = 0; i < msg->num; i++)
    {
      if ((ifp = ifg_lookup_by_index(&mstpm->ifg, msg->ifindex))
           && (port = ifp->port))
        {
          /* Extract vid. */
          vid = NSM_VLAN_VID_MASK & msg->vid_info[i];
          MSTP_VLAN_BMP_UNSET(port->svlanMemberBmp, vid);
          mstp_delete_ce_br_port (port->ce_br, ifp->name, vid);
        }
    }

  XFREE (MTYPE_TMP, msg->vid_info);

  return 0;
}

static s_int32_t
mstp_nsm_recv_vlan_port_type (struct nsm_msg_header *header,
                              void *arg, void *message)
{
  s_int32_t ret;
  struct interface *ifp;
  struct nsm_client * nc;
  struct mstp_port *port;
  struct lib_globals *zg;
  struct nsm_client_handler * nch;
  struct nsm_msg_vlan_port_type *msg;
  char   bridge_name [L2_BRIDGE_NAME_LEN+1];

  msg = message;
  nch = arg;
  nc = nch->nc;
  zg = nc->zg;
  pal_mem_set (bridge_name, 0, L2_BRIDGE_NAME_LEN + 1);

  if ((ifp = ifg_lookup_by_index(&mstpm->ifg, msg->ifindex)))
    {

      port = ifp->port;

      if (!port)
        return -1;

      if (port->port_type == msg->port_type)
        return 0;

#ifdef HAVE_I_BEB
      if (port->port_type == MSTP_VLAN_PORT_MODE_CNP)
        {
          mstp_clean_cn_br_port (port);
          if (msg->port_type != MSTP_VLAN_PORT_MODE_CE
#ifdef HAVE_B_BEB
              || msg->port_type != MSTP_VLAN_PORT_MODE_CBP
#endif /* HAVE_B_BEB */
             )
            mstp_cist_enable_port (port);
        }
#endif /* HAVE_I_BEB */
      if (port->port_type == MSTP_VLAN_PORT_MODE_CE)
        {
          mstp_bridge_delete (NULL, port);
          mstp_cist_enable_port (port);
        }

      if (msg->port_type == MSTP_VLAN_PORT_MODE_CE)
        {
           BRIDGE_GET_CE_BR_NAME (msg->ifindex);

           mstp_cist_disable_port (port);

#if (defined HAVE_I_BEB)
           ret = mstp_bridge_add (bridge_name,
                                  MSTP_BRIDGE_TYPE_CE,
                                  PAL_TRUE, PAL_FALSE,
                                  PAL_FALSE,
                                  TOPOLOGY_NONE, port);
#else
           ret = mstp_bridge_add (bridge_name, MSTP_BRIDGE_TYPE_CE,
                                  PAL_TRUE, PAL_FALSE, TOPOLOGY_NONE, port);
#endif

           if (ret == RESULT_OK)
             mstp_add_ce_br_port (port->ce_br, ifp->name, MSTP_VLAN_NULL_VID);
        }
#ifdef HAVE_I_BEB
       else if (msg->port_type == MSTP_VLAN_PORT_MODE_CNP)
         {
           BRIDGE_GET_CNP_BR_NAME (msg->ifindex);
           mstp_cist_disable_port (port);
           mstp_cist_set_port_state_forward (port);

           ret = mstp_bridge_add (bridge_name, MSTP_BRIDGE_TYPE_CNP,
                                  PAL_FALSE, PAL_TRUE,
                                  PAL_FALSE, TOPOLOGY_NONE, port);

           if (ret == RESULT_OK)
             {
              mstp_add_cn_br_port(port->cn_br, ifp->name,
                                               MSTP_VLAN_NULL_VID,
                                               NULL);
             }
         }

       else if (msg->port_type == MSTP_VLAN_PORT_MODE_PIP)
         {
           mstp_cist_disable_port (port);
           mstp_cist_set_port_state_forward (port);
         }
#endif

#ifdef HAVE_B_BEB
       else if (msg->port_type == MSTP_VLAN_PORT_MODE_CBP)
        {
           mstp_cist_disable_port (port);
           mstp_cist_set_port_state_forward(port);
        }
       else
        {
          if ((port->port_type == MSTP_VLAN_PORT_MODE_CBP) &&
               (msg->port_type != MSTP_VLAN_PORT_MODE_CE))
             mstp_cist_enable_port(port);
        }

#endif
      port->port_type =  msg->port_type;

    }

  return 0;
}

#ifdef  HAVE_PROVIDER_BRIDGE

static s_int32_t
mstp_nsm_recv_bridge_proto_process (struct nsm_msg_header *header,
                                    void *arg, void *message)
{
  struct interface *ifp;
  struct nsm_client * nc;
  struct mstp_port *port;
  struct lib_globals *zg;
  struct nsm_msg_bridge_if *msg = NULL;
  struct nsm_client_handler * nch;
  struct mstp_port *cust_edge_port;
  struct mstp_bridge *cust_edge_bridge = NULL;
  char   bridge_name [L2_BRIDGE_NAME_LEN+1];

  msg = message;
  nch = arg;
  nc = nch->nc;
  zg = nc->zg;
  pal_mem_set (bridge_name, 0, L2_BRIDGE_NAME_LEN + 1);

  if ((msg == NULL) || (msg->ifindex == NULL))
    return 0;

  if ((ifp = ifg_lookup_by_index(&mstpm->ifg, msg->ifindex [0])))
    {

      port = ifp->port;

      if (!port)
        return 0;

      if (port->port_type != MSTP_VLAN_PORT_MODE_CE
#ifdef HAVE_I_BEB
          && port->port_type != MSTP_VLAN_PORT_MODE_CNP
#endif /* HAVE_I_BEB */
         )
        return 0;

      if ( port->port_type == MSTP_VLAN_PORT_MODE_CE)
          if ((cust_edge_bridge = port->ce_br) == NULL)
              return 0;

#if (defined HAVE_I_BEB)
      if ( port->port_type == MSTP_VLAN_PORT_MODE_CNP)
          if ((cust_edge_bridge = port->cn_br) == NULL)
              return 0;
#endif /* HAVE_I_BEB */

      /* In the Customer Edge Bridge customer edge port is
       * added with svid of MSTP_VLAN_NULL_VID.
       */

      if (port->port_type == MSTP_VLAN_PORT_MODE_CE)
        {
          cust_edge_port = mstp_find_ce_br_port (cust_edge_bridge, MSTP_VLAN_NULL_VID);
          if (cust_edge_port == NULL)
            return 0;
        }	  	

#if (defined HAVE_I_BEB)
      if (port->port_type == MSTP_VLAN_PORT_MODE_CNP)
        {
          cust_edge_port = mstp_find_cn_br_port (cust_edge_bridge, MSTP_VLAN_NULL_VID);
          if (cust_edge_port == NULL)
            return 0;
        } 
#endif

      switch (msg->cust_bpdu_process)
        {
          case NSM_MSG_BRIDGE_PROTO_TUNNEL:
          case NSM_MSG_BRIDGE_PROTO_DISCARD:
           mstp_disable_bridge (cust_edge_bridge, PAL_TRUE);
           break;
          case NSM_MSG_BRIDGE_PROTO_PEER:
           mstp_enable_bridge (cust_edge_bridge);
           break;
          default:
           return 0;
        }

      if (cust_edge_bridge && msg )
      cust_edge_bridge->cust_bpdu_process =  msg->cust_bpdu_process;

    }

  XFREE (MTYPE_TMP, msg->ifindex);

  return 0;

}

#endif /* HAVE_PROVIDER_BRIDGE */

#ifdef HAVE_PVLAN
int
mstp_nsm_recv_pvlan_host_associate (struct nsm_msg_header *header,
                                    void *arg, void *message)
{
  struct nsm_client * nc;
  struct lib_globals *zg;
  struct nsm_client_handler * nch;
  struct nsm_msg_pvlan_if *msg;
  struct interface * ifp;
  int portfast = 0;
  int bpdu_guard = 0;
  int ret;

  msg = message;
  nch = arg;
  nc = nch->nc;
  zg = nc->zg;

  if (!msg)
    return 0;

  ifp = ifg_lookup_by_index (&zg->ifg, msg->ifindex);
  if (! ifp)
    return -1;

  if (msg->flags == NSM_PVLAN_ENABLE_PORTFAST_BPDUGUARD)
    {
      portfast = PAL_TRUE;
      bpdu_guard = MSTP_PORT_PORTFAST_BPDUGUARD_ENABLED;
    }
  else if (msg->flags == NSM_PVLAN_DISABLE_PORTFAST_BPDUGUARD)
    {
      portfast = PAL_FALSE;
      bpdu_guard = MSTP_PORT_PORTFAST_BPDUGUARD_DISABLED;
    }

  ret = mstp_api_set_port_edge (msg->bridge_name, ifp->name, portfast, PAL_TRUE);
  ret = mstp_api_set_port_bpduguard (msg->bridge_name, ifp->name, bpdu_guard);

  if (ret < 0)
    return RESULT_ERROR;

  return RESULT_OK;
}

s_int32_t
mstp_nsm_recv_pvlan_configure (struct nsm_msg_header *header,
                               void *arg, void *message)
{
  struct nsm_client * nc;
  struct lib_globals *zg;
  struct nsm_client_handler * nch;
  struct nsm_msg_pvlan_configure *msg;
  struct mstp_bridge *br;
  struct mstp_prefix_vlan p;
  struct mstp_vlan *vlan;
  struct route_node *rn;

  msg = message;
  nch = arg;
  nc = nch->nc;
  zg = nc->zg;

  if (!msg)
    return 0;

  br = mstp_find_bridge (msg->bridge_name);
  if (! br)
    return RESULT_ERROR;

  if (br->vlan_table)
    {
      MSTP_PREFIX_VLAN_SET (&p, msg->vid);
      rn = route_node_lookup (br->vlan_table, (struct prefix *) &p);

      if (!rn)
        return RESULT_ERROR;

      vlan = rn->info;
      route_unlock_node (rn);

      if (!vlan)
        return RESULT_ERROR;

      vlan->pvlan_type = msg->type;
      if (msg->type == NSM_PVLAN_CONFIGURE_PRIMARY)
        MSTP_VLAN_BMP_INIT (vlan->secondary_vlan_bmp);

#ifdef HAVE_HA
      mstp_cal_modify_mstp_vlan (vlan);
#endif /* HAVE_HA */
    }

  return 0;

}



/* When secondary vlan is associated to a primary vlan
 * remove the secondary vlan from its instance add it to
 * the instance of primary vlan */
static s_int32_t
mstp_secondary_vlan_associate (struct mstp_bridge *br,
                               u_int16_t primary_vid,
                               u_int16_t secondary_vid)
{
  struct mstp_vlan *primary_vlan, *secondary_vlan;
  s_int32_t primary_vlan_instance, secondary_vlan_instance;
  s_int32_t ret = 0;

  if (!br)
    return RESULT_ERROR;

  primary_vlan = mstp_bridge_vlan_lookup (br->name, primary_vid);
  secondary_vlan = mstp_bridge_vlan_lookup (br->name, secondary_vid);

  if (!primary_vlan || !secondary_vlan)
    return RESULT_ERROR;

  /* Store the secondary vlan info in primary vlan */
  MSTP_VLAN_BMP_SET(primary_vlan->secondary_vlan_bmp, secondary_vid);

  primary_vlan_instance = primary_vlan->instance;

  secondary_vlan_instance = secondary_vlan->instance;

  if (secondary_vlan_instance > 0)
    {
      /* Delete the secondary vlan from its instance */
      if (mstp_api_delete_instance_vlan (br->name, secondary_vlan_instance,
                                         secondary_vid) != RESULT_OK)
        return RESULT_ERROR;
    }

  /* Add secondary vlan to cist */
  mstp_vlan_add_event (br->name, secondary_vid, 0);

  if (primary_vlan_instance > 0)
    {
      /* Add secondary vlan to primary vlan instance */
      ret = mstp_add_instance (br, primary_vlan_instance, secondary_vid, 0);

      if (ret < 0)
        return ret;
    }

  return 0;
}

static s_int32_t
mstp_secondary_vlan_associate_clear (struct mstp_bridge *br,
                                     u_int16_t primary_vid,
                                     u_int16_t secondary_vid)
{
  struct mstp_vlan *primary_vlan, *secondary_vlan;
  s_int32_t primary_vlan_instance, secondary_vlan_instance;

  if (!br)
    return RESULT_ERROR;

  primary_vlan = mstp_bridge_vlan_lookup (br->name, primary_vid);
  secondary_vlan = mstp_bridge_vlan_lookup (br->name, secondary_vid);

  if (!primary_vlan || !secondary_vlan)
    return RESULT_ERROR;

  /* Remove the secondary vlan info from primary vlan */
  MSTP_VLAN_BMP_UNSET(primary_vlan->secondary_vlan_bmp, secondary_vid);

  primary_vlan_instance = primary_vlan->instance;

  secondary_vlan_instance = secondary_vlan->instance;

  if (secondary_vlan_instance > 0)
    {
      /* Delete the secondary vlan from its instance */
      if (mstp_delete_instance_vlan (br, secondary_vlan_instance,
                                     secondary_vid) != RESULT_OK)
        return RESULT_ERROR;
    }

  /* Add secondary vlan to cist */
  mstp_vlan_add_event (br->name, secondary_vid, 0);

  return 0;
}

int
mstp_nsm_recv_pvlan_secondary_vlan_association (struct nsm_msg_header *header,
                                                void *arg, void *message)
{
  struct nsm_client * nc;
  struct lib_globals *zg;
  struct nsm_client_handler * nch;
  struct nsm_msg_pvlan_association *msg;
  struct mstp_bridge *br;
  struct mstp_vlan *primary_vlan, *secondary_vlan;
  int flags;

  msg = message;
  nch = arg;
  nc = nch->nc;
  zg = nc->zg;

  if (!msg)
    return 0;

  br = mstp_find_bridge (msg->bridge_name);
  if (! br)
    return RESULT_ERROR;

  if (br->vlan_table)
    {
      primary_vlan = mstp_bridge_vlan_lookup (br->name, msg->primary_vid);

      secondary_vlan =  mstp_bridge_vlan_lookup (br->name,
                                                 msg->secondary_vid);
      if (!primary_vlan || !secondary_vlan)
        return RESULT_ERROR;
    }

  flags = msg->flags;

  if (flags == NSM_PVLAN_SECONDARY_ASSOCIATE)
    {
      mstp_secondary_vlan_associate (br, msg->primary_vid, msg->secondary_vid);
    }
  else if (flags == NSM_PVLAN_SECONDARY_ASSOCIATE_CLEAR)
    {
      mstp_secondary_vlan_associate_clear (br, msg->primary_vid, msg->secondary_vid);
    }

  return 0;
}
#endif /* HAVE_PVLAN */


#if (defined HAVE_I_BEB)
static int
mstp_nsm_recv_svlan_add_svid_to_isid_mapping(struct nsm_msg_header *header,
                                             void *arg, void *message)
{

  uint32_t ret;
  struct interface *ifp;
  struct nsm_client * nc;
  struct mstp_port *port;
  struct mstp_bridge *cn_br;
  struct lib_globals *zg;
  struct nsm_client_handler * nch;
  struct nsm_msg_pbb_isid_to_pip * msg;
  char   bridge_name [L2_BRIDGE_NAME_LEN+1];

  msg = message;
  nch = arg;
  nc = nch->nc;
  zg = nc->zg;
  pal_mem_set (bridge_name, 0, L2_BRIDGE_NAME_LEN + 1);

  msg = message;

  if ((ifp = ifg_lookup_by_index(&mstpm->ifg, msg->cnp_ifindex)))
    {
       port = ifp->port;

       if (!port)
         return RESULT_ERROR;

       cn_br = port->cn_br;

       if (!cn_br)
         return RESULT_ERROR;

       if ( cn_br->port_list == NULL )
          cn_br->port_list = (struct mstp_port *)list_new();

       ret = mstp_add_cn_br_port(port->cn_br, ifp->name, msg->isid, msg );
       if (ret != RESULT_OK)
           return RESULT_ERROR;
    }

  return RESULT_OK;
}

static int
mstp_nsm_recv_svlan_del_svid_to_isid_mapping(struct nsm_msg_header *header,
                                             void *arg, void *message)
{
  uint32_t ret;
  struct interface *ifp;
  struct nsm_client * nc;
  struct mstp_port *port;
  struct mstp_bridge *cn_br;
  struct lib_globals *zg;
  struct nsm_client_handler * nch;
  struct nsm_msg_pbb_isid_to_pip * msg;
  char   bridge_name [L2_BRIDGE_NAME_LEN+1];

  msg = message;
  nch = arg;
  nc = nch->nc;
  zg = nc->zg;
  pal_mem_set (bridge_name, 0, L2_BRIDGE_NAME_LEN + 1);

  msg = message;
  if ((ifp = ifg_lookup_by_index(&mstpm->ifg, msg->cnp_ifindex)))
    {
       port = ifp->port;

       if (!port)
         return RESULT_ERROR;

       cn_br = port->cn_br;

       if (!cn_br)
         return RESULT_ERROR;

       ret = mstp_del_cn_br_vip_port(port->cn_br, ifp->name, msg->isid, msg );
       if (ret != RESULT_OK)
           return RESULT_ERROR;
    }

 return RESULT_OK;

}


static int
mstp_nsm_recv_bvlan_add_bvid_to_isid_mapping(struct nsm_msg_header *header,
                                             void *arg, void *message)
{
  struct interface *ifp;
  struct nsm_client * nc;
  struct mstp_port *port;
  struct mstp_bridge *cn_br;
  struct lib_globals *zg;
  struct nsm_client_handler * nch;
  struct nsm_msg_pbb_isid_to_bvid * msg;
  char   bridge_name [L2_BRIDGE_NAME_LEN+1];

  msg = message;
  nch = arg;
  nc = nch->nc;
  zg = nc->zg;
  pal_mem_set (bridge_name, 0, L2_BRIDGE_NAME_LEN + 1);

  msg = message;

  if ((ifp = ifg_lookup_by_index(&mstpm->ifg, msg->cbp_ifindex)))
    {
       port = ifp->port;

       if (!port)
         return RESULT_ERROR;

       cn_br = port->cn_br;

       if (!cn_br)
         return RESULT_ERROR;

       if ( cn_br->port_list == NULL )
         return RESULT_ERROR;

       if ( (mstp_updt_bvid_to_isid_mapping(port->cn_br,msg )) != RESULT_OK )
          return RESULT_ERROR;
    }

  return RESULT_OK;
}

static int
mstp_nsm_recv_pbb_isid_delete(struct nsm_msg_header *header,
                              void *arg, void *message)
{
   return 0;
}

static int
mstp_nsm_recv_bvlan_dispatch_isid_mapping(struct nsm_msg_header *header,
                                          void *arg, void *message)
{

  struct interface *ifp;
  struct nsm_client * nc;
  struct mstp_port *port;
  struct lib_globals *zg;
  struct nsm_client_handler * nch;
  struct nsm_msg_pbb_isid_to_pip * msg;
  char   bridge_name [L2_BRIDGE_NAME_LEN+1];

  msg = message;
  nch = arg;
  nc = nch->nc;
  zg = nc->zg;
  pal_mem_set (bridge_name, 0, L2_BRIDGE_NAME_LEN + 1);

  msg = message;

  if ((ifp = ifg_lookup_by_index(&mstpm->ifg, msg->cnp_ifindex)))
    {
       port = ifp->port;

       if (!port)
         return RESULT_ERROR;

       mstp_cn_br_update_vip_list_status(port->cn_br,msg->isid);

    }

   return 0;
}

static int
mstp_nsm_recv_bvlan_default_vid(struct nsm_msg_header * header,
                                void *arg, void * message)
{
  struct interface *ifp;
  struct nsm_client * nc;
  struct mstp_port *port;
  struct lib_globals *zg;
  struct nsm_client_handler * nch;
  struct nsm_msg_vlan_port * msg;
  char   bridge_name [L2_BRIDGE_NAME_LEN+1];

  msg = message;
  nch = arg;
  nc = nch->nc;
  zg = nc->zg;
  pal_mem_set (bridge_name, 0, L2_BRIDGE_NAME_LEN + 1);

  msg = message;

  if ((ifp = ifg_lookup_by_index(&mstpm->ifg, msg->ifindex)))
    {
       port = ifp->port;

       if (!port)
         return RESULT_ERROR;

       port->bvid = msg->vid_info[0];
    }

   return RESULT_OK;
}

#endif /* HAVE_I_BEB */

#if defined  HAVE_G8031 || defined HAVE_G8032
int 
mstp_nsm_recv_bridge_add_vlan_to_protection (struct nsm_msg_header *header,
                                             void *arg, void *message)
{
  int instance_index;
  struct nsm_client * nc;
  struct lib_globals *zg;
  struct nsm_client_handler * nch;
  struct nsm_msg_vlan *msg;
  struct mstp_bridge *br;
  struct mstp_prefix_vlan p;
  struct route_node *node;
  struct mstp_vlan *vlan;

  msg = message;
  nch = arg;

  if (!message || !nch || !nch->nc)
    return RESULT_ERROR;

  nc = nch->nc;
  zg = nc->zg;

  br =  mstp_find_bridge (msg->bridge_name);
  if (! br)
    return RESULT_ERROR;

  if (! br->vlan_table)
    return RESULT_ERROR;

  MSTP_PREFIX_VLAN_SET (&p, msg->vid);
  node = route_node_lookup (br->vlan_table, (struct prefix *) &p);

  if (!node)
    return RESULT_ERROR;

  vlan = node->info;

  if (!vlan) 
   return RESULT_ERROR;

  for (instance_index = 1; instance_index < br->max_instances; instance_index++)
    {
      if (!br->instance_list[instance_index])
         continue;
 
      mstp_api_delete_instance_vlan (br->name, instance_index, vlan->vid);
    } 

  SET_FLAG (vlan->flags, MSTP_VLAN_PG_CONFIGURED);

  return 0;
}

#if defined HAVE_G8031 || defined HAVE_G8032
int
mstp_nsm_recv_bridge_add_pg (struct nsm_msg_header *header,
                             void *arg, void *message)
{
  struct nsm_client * nc;
  struct lib_globals *zg;
  struct nsm_client_handler * nch;
  struct nsm_msg_bridge_pg *msg;
  struct interface *ifp;
  struct mstp_bridge *bridge;
  struct mstp_port *port = NULL;
  struct mstp_instance_port *inst_port = NULL;
  bool_t inst_present = PAL_FALSE;
  int i = 0;

  msg = message;
  nch = arg;
  nc = nch->nc;
  zg = nc->zg;

  bridge = mstp_find_bridge (msg->bridge_name);
  
  if (! bridge)
    return RESULT_ERROR;

  MSTP_INSTANCE_BMP_SET(bridge->mstpProtectionBmp, msg->instance);

  for (i = 0; i < msg->num; i++)
    {
      if ((ifp = ifg_lookup_by_index(&mstpm->ifg, msg->ifindex[i])))
        {

          port = mstp_find_cist_port (bridge, ifp->ifindex);
          if (!port)
            continue;

          for (inst_port = port->instance_list; inst_port ;
              inst_port = inst_port->next_inst)
             {
		   if (inst_port->instance_id == 0)
		     continue;
		   else
		     {
		       inst_present = PAL_TRUE;
		       break;     
	    	     }      
		 }

           if (!inst_present) 
             mstp_cist_disable_port (port);
          }
     }

  return RESULT_OK;
}

int
mstp_nsm_recv_bridge_del_pg (struct nsm_msg_header *header,
    void *arg, void *message)
{
  struct nsm_client * nc;
  struct lib_globals *zg;
  struct nsm_client_handler * nch;
  struct nsm_msg_bridge_pg *msg;
  struct interface *ifp;
  struct mstp_bridge *bridge;
  struct mstp_port *port = NULL;
  struct mstp_instance_port *inst_port = NULL;
  bool_t inst_present = PAL_FALSE;
  int i = 0;

  msg = message;
  nch = arg;
  nc = nch->nc;
  zg = nc->zg;

  bridge = mstp_find_bridge (msg->bridge_name);
  
  if (! bridge)
    return RESULT_ERROR;

  MSTP_INSTANCE_BMP_UNSET(bridge->mstpProtectionBmp, msg->instance);

  for (i = 0; i < msg->num; i++)
    {
      if ((ifp = ifg_lookup_by_index(&mstpm->ifg, msg->ifindex[i])))
        {

          port = mstp_find_cist_port (bridge, ifp->ifindex);
          if (!port)
            continue;

          for (inst_port = port->instance_list; inst_port ;
              inst_port = inst_port->next_inst)
             {
                   if (inst_port->instance_id == 0)
                     continue;
                   else
                     {
                       inst_present = PAL_TRUE;
                       break;
                     }
                 }

           if (!inst_present)
             mstp_cist_disable_port (port);
          }
     }
    return RESULT_OK;
}
#endif /* HAVE_G8031 || HAVE_G8032*/

int 
mstp_nsm_recv_bridge_del_vlan_from_protection (struct nsm_msg_header *header,
                                               void *arg, void *message)
{
  struct nsm_client * nc;
  struct lib_globals *zg;
  struct nsm_client_handler * nch;
  struct nsm_msg_vlan *msg;
  struct mstp_bridge *br;
  struct mstp_prefix_vlan p;
  struct route_node *node;
  struct mstp_vlan *vlan;

  msg = message;
  nch = arg;

  if (!message || !nch || !nch->nc)
    return RESULT_ERROR;

  nc = nch->nc;
  zg = nc->zg;

  br =  mstp_find_bridge (msg->bridge_name);
  if (! br)
    return RESULT_ERROR;

  if (! br->vlan_table)
    return RESULT_ERROR;

  MSTP_PREFIX_VLAN_SET (&p, msg->vid);
  node = route_node_lookup (br->vlan_table, (struct prefix *) &p);

  if (!node)
    return RESULT_ERROR;

  vlan = node->info;

  if (!vlan)
   return RESULT_ERROR;

  UNSET_FLAG (vlan->flags, MSTP_VLAN_PG_CONFIGURED);

  return 0;
}
#endif /* HAVE_G8031 || HAVE_G8032*/

#endif /* HAVE_VLAN */


int
mstp_nsm_disconnect_process (void)
{
  struct mstp_bridge *bridge;
  struct mstp_bridge *bridge_next;

  for (bridge = mstp_get_first_bridge (); bridge; bridge = bridge_next)
    {
      bridge_next = bridge->next;
      mstp_bridge_delete (bridge->name, NULL);
    }

#ifdef HAVE_SMI
  smi_record_fault (mstpm, FM_GID_MSTP,
                    MSTP_SMI_ALARM_NSM_SERVER_SOCKET_DISCONNECT,
                    __LINE__, __FILE__, NULL);
#endif /* HAVE_SMI */

  if (mstpm && mstpm->nc)
    nsm_client_reconnect_start(mstpm->nc);

  return 0;
}

void
mstp_nsm_init (struct lib_globals * zg)
{
  struct nsm_client * nc;

  nc = nsm_client_create (zg, 0);
  if (!nc)
    return;

  /* Set version and protocol.  */
  nsm_client_set_version (nc, NSM_PROTOCOL_VERSION_1);
  nsm_client_set_protocol (nc, APN_PROTO_MSTP);

  nsm_client_set_service (nc, NSM_SERVICE_INTERFACE);
  nsm_client_set_service (nc, NSM_SERVICE_BRIDGE);

  nsm_client_set_callback (nc, NSM_MSG_LINK_UP, mstp_nsm_recv_interface_state);
  nsm_client_set_callback (nc, NSM_MSG_LINK_DOWN,
                           mstp_nsm_recv_interface_state);
  nsm_client_set_callback (nc, NSM_MSG_BRIDGE_ADD,
                           mstp_nsm_recv_bridge_add);
  nsm_client_set_callback (nc, NSM_MSG_BRIDGE_DELETE,
                           mstp_nsm_recv_bridge_delete);
  nsm_client_set_callback (nc, NSM_MSG_BRIDGE_ADD_PORT,
                           mstp_nsm_recv_bridge_add_if);
  nsm_client_set_callback (nc, NSM_MSG_BRIDGE_DELETE_PORT,
                           mstp_nsm_recv_bridge_delete_if);
  nsm_client_set_callback (nc, NSM_MSG_BRIDGE_PORT_STATE_SYNC_REQ,
                           mstp_nsm_recv_bridge_if_state_sync_req);
  nsm_client_set_callback (nc, NSM_MSG_BRIDGE_SET_AGEING_TIME ,
                           mstp_nsm_recv_bridge_ageing_time_set);
  nsm_client_set_callback (nc, NSM_MSG_BRIDGE_PORT_SPANNING_TREE_ENABLE,
                           mstp_nsm_recv_bridge_port_spanning_tree_enable);


#ifdef HAVE_IMI
  nsm_client_set_callback (nc, NSM_MSG_VR_SYNC_DONE,
                           mstp_nsm_recv_msg_imi_sync_done);
#endif /* HAVE_IMI */

#ifdef HAVE_PROVIDER_BRIDGE
  nsm_client_set_callback (nc, NSM_MSG_SET_BPDU_PROCESS,
                           mstp_nsm_recv_bridge_proto_process);
#endif /* HAVE_PROVIDER_BRIDGE */

#ifdef HAVE_VLAN
  nsm_client_set_callback (nc, NSM_MSG_VLAN_PORT_TYPE,
                           mstp_nsm_recv_vlan_port_type);
  nsm_client_set_service (nc, NSM_SERVICE_VLAN);
  nsm_client_set_callback (nc, NSM_MSG_VLAN_ADD,
                           mstp_nsm_recv_bridge_add_vlan);
  nsm_client_set_callback (nc, NSM_MSG_VLAN_DELETE,
                           mstp_nsm_recv_bridge_delete_vlan);
  nsm_client_set_callback (nc, NSM_MSG_VLAN_ADD_PORT,
                           mstp_nsm_recv_vlan_add_port);
  nsm_client_set_callback (nc, NSM_MSG_VLAN_DELETE_PORT,
                           mstp_nsm_recv_vlan_delete_port);
  nsm_client_set_callback (nc, NSM_MSG_SVLAN_ADD_CE_PORT,
                           mstp_nsm_recv_svlan_add_ce_port);
  nsm_client_set_callback (nc, NSM_MSG_SVLAN_DELETE_CE_PORT,
                           mstp_nsm_recv_svlan_delete_ce_port);

#ifdef HAVE_PVLAN
  nsm_client_set_callback (nc, NSM_MSG_PVLAN_PORT_HOST_ASSOCIATE,
                               mstp_nsm_recv_pvlan_host_associate);
  nsm_client_set_callback  (nc, NSM_MSG_PVLAN_CONFIGURE,
                               mstp_nsm_recv_pvlan_configure);
  nsm_client_set_callback (nc, NSM_MSG_PVLAN_SECONDARY_VLAN_ASSOCIATE,
                               mstp_nsm_recv_pvlan_secondary_vlan_association);
#endif /* HAVE_PVLAN */
#endif /* HAVE_VLAN. */

#if (defined HAVE_I_BEB)
  nsm_client_set_callback (nc, NSM_MSG_PBB_ISID_TO_PIP_ADD,
                           mstp_nsm_recv_svlan_add_svid_to_isid_mapping);
  nsm_client_set_callback (nc, NSM_MSG_PBB_SVID_TO_ISID_DEL,
                           mstp_nsm_recv_svlan_del_svid_to_isid_mapping);
  nsm_client_set_callback (nc, NSM_MSG_PBB_ISID_TO_BVID_ADD,
                           mstp_nsm_recv_bvlan_add_bvid_to_isid_mapping);
  nsm_client_set_callback (nc, NSM_MSG_PBB_DISPATCH,
                           mstp_nsm_recv_bvlan_dispatch_isid_mapping);
  nsm_client_set_callback (nc, NSM_MSG_PBB_ISID_TO_BVID_DEL,
                           mstp_nsm_recv_pbb_isid_delete);
  nsm_client_set_callback (nc,NSM_MSG_VLAN_SET_PVID,
                           mstp_nsm_recv_bvlan_default_vid);
#endif

#if defined HAVE_G8031 || defined HAVE_G8032
  nsm_client_set_callback (nc, NSM_MSG_VLAN_ADD_TO_PROTECTION,
                           mstp_nsm_recv_bridge_add_vlan_to_protection);

  nsm_client_set_callback (nc, NSM_MSG_VLAN_DEL_FROM_PROTECTION,
                           mstp_nsm_recv_bridge_del_vlan_from_protection);

  nsm_client_set_callback (nc, NSM_MSG_BRIDGE_ADD_PG,
                            mstp_nsm_recv_bridge_add_pg);

  nsm_client_set_callback (nc, NSM_MSG_BRIDGE_DEL_PG,
                            mstp_nsm_recv_bridge_del_pg);
#endif /* HAVE_G8031 */

  if_add_hook (&zg->ifg, IF_CALLBACK_VR_BIND, mstp_nsm_recv_interface_update);
  if_add_hook (&zg->ifg, IF_CALLBACK_VR_UNBIND, mstp_nsm_recv_interface_delete);

#ifndef HAVE_HA
  /* Set disconnect handling callback */
  nsm_client_set_disconnect_callback (nc, mstp_nsm_disconnect_process);
#endif /* HAVE_HA */

  nsm_client_start (nc);
}

int mstp_nsm_send_port_state (u_int32_t ifindex, int port_state)
{
  struct nsm_msg_stp msg;

  /* Prepare and send msg to NSM */
  pal_mem_set (&msg, 0, sizeof (struct nsm_msg_stp));

  if (port_state == MSTP_INTERFACE_SHUTDOWN)
    msg.flags |= NSM_MSG_INTERFACE_DOWN;
  else
    msg.flags |= NSM_MSG_INTERFACE_UP;

  msg.ifindex = ifindex;

  /* Send STP message to NSM */
  return (nsm_client_send_stp_message (mstpm->nc, &msg,
                                       NSM_MSG_STP_INTERFACE));
}

int mstp_nsm_send_ageing_time (const char *const br_name,
                               s_int32_t ageing_time)
{
  struct nsm_msg_bridge msg;
  struct lib_globals *protolg;

  /* Prepare and send msg to NSM */
  pal_mem_set (&msg, 0, sizeof (struct nsm_msg_bridge));

  protolg = mstpm;

  msg.ageing_time = ageing_time;
  pal_strcpy (msg.bridge_name, br_name);

  /* Send STP message to NSM */
  return (nsm_client_send_bridge_msg (protolg->nc, &msg,
                                       NSM_MSG_BRIDGE_SET_AGEING_TIME));
}
