/* Copyright 2003-2004  All Rights Reserved.
 
Multiple Spanning tree protocol; Port related functions
  
*/

#include "pal.h"
#include "lib.h"
#include "thread.h"
#include "l2lib.h"

#include "mstpd.h"
#include "mstp_config.h"
#include "mstp_types.h"
#include "mstp_bpdu.h"
#include "mstp_port.h"
#include "l2_timer.h"
#include "mstp_transmit.h"
#include "mstp_cist_proto.h"
#include "mstp_msti_proto.h"
#include "mstp_bridge.h"
#include "mstp_api.h"

#include "nsm_message.h"
#include "nsm_client.h"

#ifdef HAVE_HA
#include "mstp_cal.h"
#endif /* HAVE_HA */

#ifdef HAVE_SNMP
#include "mstp_snmp.h"
#endif /* HAVE_SNMP */

char *infotype_str[4] = 
  {
    "INFO_TYPE_DISABLED",
    "INFO_TYPE_AGED",
    "INFO_TYPE_MINE",
    "INFO_TYPE_RECEIVED"
  };


char *mstprole_str[6] = 
  {
    "ROLE_MASTERPORT",
    "ROLE_ALTERNATE",
    "ROLE_ROOTPORT",
    "ROLE_DESIGNATED",
    "ROLE_DISABLED",
    "ROLE_BACKUP"
  };

char *mstpstate_str[3] = 
  {
    "STATE_DISCARDING",
    "STATE_LEARNING",
    "STATE_FORWARDING"
  };

#define BRIDGE_VERSION_GET(B)                                                \
        ((IS_BRIDGE_MSTP (B)) ? BR_VERSION_MSTP : (IS_BRIDGE_STP (B)) ?      \
         BR_VERSION_STP : (IS_BRIDGE_RSTP (B)) ?                             \
         BR_VERSION_RSTP : (IS_BRIDGE_RPVST_PLUS(B)) ?                       \
         BR_VERSION_MSTP : (BRIDGE_TYPE_PROVIDER(B)) ?                       \
         BR_VERSION_RSTP : BR_VERSION_INVALID)

u_int32_t
mstp_map_port2msg_state(int state)
{
  switch(state)
    {
    case STATE_DISCARDING:
      return NSM_BRIDGE_PORT_STATE_DISCARDING;
      break;
    case STATE_LISTENING:
      return NSM_BRIDGE_PORT_STATE_LISTENING;
      break;
    case STATE_LEARNING:
      return NSM_BRIDGE_PORT_STATE_LEARNING;
      break;
    case STATE_FORWARDING:
      return NSM_BRIDGE_PORT_STATE_FORWARDING;
      break;
    case STATE_BLOCKING:
      return NSM_BRIDGE_PORT_STATE_BLOCKING;
      break;
    default:
      return 0;
      break;
    }
}

struct mstp_interface *
mstp_interface_new (struct interface *ifp)
{
  struct mstp_interface *mstpif;

  /* allocate memory for mstp_interface */
  mstpif = XCALLOC (MTYPE_XSTP_INTERFACE, sizeof(struct mstp_interface));

  if (mstpif == NULL) { 
    zlog_err (ifp->vr->zg, "Could not allocate memory for mstp interface\n");
    return NULL;
  }

  mstpif->ifp =  ifp;

  mstpif->active = PAL_FALSE;

  return mstpif;

}

int 
mstp_interface_add (struct interface *ifp)
{
  struct mstp_interface *mstpif = NULL;

  mstpif = ifp->info;
  if (!mstpif)
    return RESULT_ERROR;

  if (mstpif->active)
    return 0;

  mstp_activate_interface (mstpif);
 
  mstpif->active = PAL_TRUE;
 
  /* Free the memory allocated for storing instance related
   * information if any.
   */

  return 0;
}

void 
mstpif_free_instance_list (struct mstp_interface *mstpif)
{ 
  struct port_instance_list *pinst_list = NULL;
  struct port_instance_list  *next_pinst_list;

  if (mstpif->instance_list != NULL)
    for (pinst_list = mstpif->instance_list; pinst_list;
         pinst_list = next_pinst_list)
      {
        next_pinst_list = pinst_list->next;
        XFREE (MTYPE_PORT_INSTANCE_LIST, pinst_list);
      }
  mstpif->instance_list = NULL;
  return;
}

void 
mstp_activate_interface (struct mstp_interface *mstpif)
{
  struct port_instance_list *pinst_list = NULL;

  /* Start loading the interface configurations for the protocol processing */

  if (! CHECK_FLAG (mstpif->config, MSTP_IF_CONFIG_BRIDGE_GROUP))
    return;

  if (mstpif->instance_list != NULL)
    for (pinst_list = mstpif->instance_list; pinst_list;
         pinst_list = pinst_list->next)
      if (CHECK_FLAG (pinst_list->instance_info.config,
                      MSTP_IF_CONFIG_BRIDGE_INSTANCE_PRIORITY))
        mstp_api_set_msti_port_priority (mstpif->bridge_group,
                                         mstpif->ifp->name,
                                         pinst_list->instance_info.instance,
                                         pinst_list->instance_info.port_instance_priority);

  if (CHECK_FLAG (mstpif->config, MSTP_IF_CONFIG_BRIDGE_PRIORITY))
    mstp_api_set_port_priority (mstpif->bridge_group, mstpif->ifp->name,
                                MSTP_VLAN_NULL_VID, mstpif->bridge_priority);

  if (CHECK_FLAG (mstpif->config, MSTP_IF_CONFIG_BRIDGE_PATHCOST))
    mstp_api_set_port_path_cost (mstpif->bridge_group, mstpif->ifp->name,
                                 MSTP_VLAN_NULL_VID,
                                 mstpif->bridge_pathcost);

  if (mstpif->instance_list != NULL)
    for (pinst_list = mstpif->instance_list; pinst_list;
         pinst_list = pinst_list->next)
      if (CHECK_FLAG (pinst_list->instance_info.config,
                      MSTP_IF_CONFIG_INSTANCE_RESTRICTED_TCN))
        mstp_api_set_msti_instance_restricted_tcn (mstpif->bridge_group,
                                          mstpif->ifp->name,
                                          pinst_list->instance_info.instance,
                                          pinst_list->instance_info.restricted_tcn);

  if (mstpif->instance_list != NULL)
    for (pinst_list = mstpif->instance_list; pinst_list;
         pinst_list = pinst_list->next)
      if (CHECK_FLAG (pinst_list->instance_info.config,
                      MSTP_IF_CONFIG_INSTANCE_RESTRICTED_ROLE))
        mstp_api_set_msti_instance_restricted_role (mstpif->bridge_group,
                                          mstpif->ifp->name,
                                          pinst_list->instance_info.instance,
                                          pinst_list->instance_info.restricted_role);

  if (mstpif->instance_list != NULL)
    for (pinst_list = mstpif->instance_list; pinst_list;
         pinst_list = pinst_list->next)
      if (CHECK_FLAG (pinst_list->instance_info.config,
                      MSTP_IF_CONFIG_BRIDGE_INSTANCE_PATHCOST))
        mstp_api_set_msti_port_path_cost (mstpif->bridge_group,
                                          mstpif->ifp->name,
                                          pinst_list->instance_info.instance,
                                          pinst_list->instance_info.port_instance_pathcost);



  if (CHECK_FLAG (mstpif->config, MSTP_IF_CONFIG_LINKTYPE))
    mstp_api_set_port_p2p (mstpif->ifp->bridge_name, mstpif->ifp->name,
                           mstpif->port_p2p);  

  if (CHECK_FLAG (mstpif->config, MSTP_IF_CONFIG_PORT_HELLO_TIME))
    mstp_api_set_port_hello_time (mstpif->ifp->bridge_name, mstpif->ifp->name,
                           mstpif->hello_time);

  if (CHECK_FLAG (mstpif->config, MSTP_IF_CONFIG_RESTRICTED_ROLE))
    mstp_api_set_port_restricted_role (mstpif->ifp->bridge_name, mstpif->ifp->name,
                           mstpif->restricted_role);

  if (CHECK_FLAG (mstpif->config, MSTP_IF_CONFIG_RESTRICTED_TCN))
    mstp_api_set_port_restricted_tcn (mstpif->ifp->bridge_name, mstpif->ifp->name,
                           mstpif->restricted_tcn);


  if (CHECK_FLAG (mstpif->config, MSTP_IF_CONFIG_PORTFAST))
    mstp_api_set_port_edge (mstpif->ifp->bridge_name, mstpif->ifp->name,
                            mstpif->port_edge, mstpif->portfast_conf);

  if (CHECK_FLAG (mstpif->config, MSTP_IF_CONFIG_FORCE_VERSION))
    mstp_api_set_port_forceversion (mstpif->ifp->bridge_name, 
                                    mstpif->ifp->name,
                                    mstpif->version);

  if (CHECK_FLAG (mstpif->config, MSTP_IF_CONFIG_BPDUGUARD))
    mstp_api_set_port_bpduguard (mstpif->ifp->bridge_name, mstpif->ifp->name,
                                 mstpif->bpduguard);

  if (CHECK_FLAG (mstpif->config, MSTP_IF_CONFIG_ROOTGUARD))
    mstp_api_set_port_rootguard (mstpif->ifp->bridge_name, mstpif->ifp->name,
                                 PAL_TRUE);

  if (CHECK_FLAG (mstpif->config, MSTP_IF_CONFIG_BPDUFILTER))
    mstp_api_set_port_bpdufilter (mstpif->ifp->bridge_name, mstpif->ifp->name,
                                  mstpif->bpdu_filter);

  if (CHECK_FLAG (mstpif->config, MSTP_IF_CONFIG_AUTOEDGE))
    mstp_api_set_auto_edge (mstpif->ifp->bridge_name, mstpif->ifp->name,
                            PAL_TRUE);
#ifdef HAVE_L2GP
  if (CHECK_FLAG (mstpif->config, MSTP_IF_CONFIG_L2GP_PORT))
    mstp_api_set_l2gp_port(mstpif->ifp->bridge_name, mstpif->ifp->name,
                           mstpif->isL2gp, mstpif->enableBPDUrx,
                           &mstpif->psuedoRootId);
#endif /* HAVE_L2GP */


  return;

}

void 
mstpif_config_add_instance (struct mstp_interface *mstpif,
                            struct port_instance_list *instance_list)
{
  struct port_instance_list *tmp = NULL;

  if (mstpif->instance_list != NULL)
    tmp = mstpif->instance_list;
  mstpif->instance_list = instance_list,
    mstpif->instance_list->next = tmp;

  return;
}

struct port_instance_info * 
mstpif_get_instance_info (struct mstp_interface *mstpif,
                          s_int32_t instance)
{
  struct port_instance_list *pinst_list = NULL;

  for (pinst_list = mstpif->instance_list; pinst_list;
       pinst_list = pinst_list->next)
    {
      if (pinst_list->instance_info.instance == instance)
        return &(pinst_list->instance_info);
    }

  return NULL;
}

int
mstp_cist_set_ring_port_state (struct mstp_port *port)
{

  struct nsm_msg_bridge_port msg;

  /* Send messsage to nsm to set port state and learn. */
  pal_mem_set(&msg, 0, sizeof(struct nsm_msg_bridge_port));
  msg.cindex = 0;
  msg.ifindex = port->ifindex;
  msg.num = 2;
  msg.instance = XCALLOC(MTYPE_TMP, msg.num * sizeof(u_int32_t));
  msg.state = XCALLOC(MTYPE_TMP, msg.num * sizeof(u_int32_t));
  /* For CIST, the instance is 0 */
  msg.state[0] = mstp_map_port2msg_state(STATE_LEARNING);
  msg.state[1] = mstp_map_port2msg_state(STATE_FORWARDING);
  pal_strncpy(msg.bridge_name, port->br->name, NSM_BRIDGE_NAMSIZ);
  NSM_SET_CTYPE(msg.cindex, NSM_BRIDGE_CTYPE_PORT_STATE);

  /* Send port state msg to NSM */
  nsm_client_send_bridge_port_msg(mstpm->nc, &msg,
                                  NSM_MSG_BRIDGE_PORT_STATE);


  XFREE(MTYPE_TMP, msg.instance);
  XFREE(MTYPE_TMP, msg.state);

  return RESULT_OK;

}

int
mstp_cist_ring_handle_topology_change (struct mstp_port *port)
{

  if (! l2_is_timer_running (port->tc_timer))
    {
      port->tc_timer = l2_start_timer(mstp_tc_timer_handler, port,
                                      mstp_cist_get_tc_time(port),mstpm);

#ifdef HAVE_HA
          mstp_cal_create_mstp_tc_timer (port);
#endif /* HAVE_HA */

      port->newInfoCist = PAL_TRUE;

      if (LAYER2_DEBUG(proto, PROTO))
        zlog_debug (mstpm, "Starting cist tc timer for port %u",
                    port->ifindex);
    }

  /* If Restricted TCN is TRUE - No topology changes are propagated */
  if (port->restricted_tcn == PAL_TRUE)
    {
      zlog_info (mstpm,"mstp_cist_set_tc: "
                 "Restricted TCN set "
                 "Ignoring Rcvd tc for port(%u)",
                 port->ifindex);
    }
  else
    mstp_cist_propogate_topology_change (port);

  return RESULT_OK;

}

/* This function should be called only from disable_bridge when called
   with bridge_forward set to true */
int
mstp_cist_set_port_state_forward (struct mstp_port *port) 
{
  struct nsm_msg_bridge_port msg;
  
  /* Don't do anything if we are already in that state and
     spanning tree is disabled. */

  if (port->spanning_tree_disable == PAL_FALSE
      && port->cist_state == STATE_FORWARDING)
    return RESULT_OK;
  
  if (LAYER2_DEBUG(proto, PROTO))
    zlog_info(mstpm,"mstp_cist_set_port_state:"
              " Bridge (%s) Port(%u) state changed from %d to %d",
              port->br->name, port->ifindex, port->cist_state, 
              STATE_FORWARDING);
  
  port->cist_state = STATE_FORWARDING;

  /* Figure 13-19. We generate TC only when a port is added to 
   * active topology, meaning only when the role changes from 
   * Alternate/ Backup to Root/ Designated and port moves to
   * forwarding when the protocol version is MSTP.
   */

  if (port->cist_tc_state == TC_INACTIVE
      && !port->oper_edge)
    {
      /* Start topology change timer 
       * propogate the topology change */
      if (l2_is_timer_running (port->tc_timer))
        {
#ifdef HAVE_HA
          mstp_cal_delete_mstp_tc_timer (port);
#endif /* HAVE_HA */

          l2_stop_timer(&port->tc_timer);
        }
      
      if (LAYER2_DEBUG(proto, PROTO))
        zlog_debug (mstpm, "Starting cist tc timer for port %u",
                    port->ifindex);

      port->cist_tc_state = TC_ACTIVE;
    }


  port->cist_forward_transitions++;
  
  /* Send messsage to nsm to set port state and learn. */
  pal_mem_set(&msg, 0, sizeof(struct nsm_msg_bridge_port));
  msg.cindex = 0;
  msg.ifindex = port->ifindex;
  msg.num = 1;
  msg.instance = XCALLOC(MTYPE_TMP, msg.num * sizeof(u_int32_t));
  msg.state = XCALLOC(MTYPE_TMP, msg.num * sizeof(u_int32_t));
  /* For CIST, the instance is 0 */
  msg.state[0] = mstp_map_port2msg_state(port->cist_state);
  pal_strncpy(msg.bridge_name, port->br->name, NSM_BRIDGE_NAMSIZ);
  NSM_SET_CTYPE(msg.cindex, NSM_BRIDGE_CTYPE_PORT_STATE);

  nsm_client_send_bridge_port_msg(mstpm->nc, &msg,
                                  NSM_MSG_BRIDGE_PORT_STATE);
  
  XFREE(MTYPE_TMP, msg.instance);
  XFREE(MTYPE_TMP, msg.state);

#ifdef HAVE_HA
  mstp_cal_modify_mstp_port (port);
#endif /* HAVE_HA */
 
  /* Send port state msg to NSM */
  return RESULT_OK;
}

int
mstp_cist_set_port_state (struct mstp_port *port, 
                          enum port_state state)
{
  struct nsm_msg_bridge_port msg;
#ifdef HAVE_PROVIDER_BRIDGE
  struct apn_vr *vr = apn_vr_get_privileged (mstpm);
  struct mstp_port *ce_port;
  struct mstp_bridge *br;
  struct interface *ifp;
#endif /* HAVE_PROVIDER_BRIDGE */
  
  /* Don't do anything if we are already in that state. */
  if (port->cist_state == state)
    return RESULT_OK;
  
  /* Stop the recent root timer if its running for 
     change to disscarding state. */
  if (state == STATE_DISCARDING)
    {
      if (l2_is_timer_running (port->recent_root_timer))
        {
#ifdef HAVE_HA
          mstp_cal_delete_mstp_rr_timer (port);
#endif /* HAVE_HA */

          l2_stop_timer(&port->recent_root_timer);
          port->recent_root_timer = NULL;
          port->br->br_all_rr_timer_cnt--;
        }
    }
  if ((port->cist_role == ROLE_ALTERNATE || port->cist_role == ROLE_DISABLED ||
       port->cist_role == ROLE_BACKUP) && state == STATE_FORWARDING)
    {
      return RESULT_OK;
    }

  if (state != STATE_DISCARDING
      && port->admin_rootguard == PAL_TRUE)
    port->oper_rootguard = PAL_FALSE;

 
  /* Update port operational rootguard. */
  if (port->admin_rootguard != PAL_FALSE)
    {  
       if ((port->cist_role == ROLE_DESIGNATED) && (state == STATE_FORWARDING))
         {
           port->oper_rootguard = PAL_TRUE;
         }
       else 
         {
           port->oper_rootguard = PAL_TRUE;
         }
    }

  port->cist_forward_transitions += (state == STATE_FORWARDING);

  
  if (LAYER2_DEBUG(proto, PROTO))
    zlog_info(mstpm,"mstp_cist_set_port_state:"
              " Bridge (%s) Port(%u) state changed from %d to %d",
              port->br->name, port->ifindex, port->cist_state, 
              state);
 
#ifdef HAVE_SNMP
  /* send topology change trap */
  mstp_snmp_topology_change (port, state);
#endif /* HAVE_SNMP */
 
  port->cist_state = state;

  /* DESIGNATED_FORWARD Fig 17-7 802.1 D 2004 */

  if (port->cist_role == ROLE_DESIGNATED)
    {
      port->agreed = port->send_mstp;
    }

  /* Send messsage to nsm to set port state and learn. */
  pal_mem_set(&msg, 0, sizeof(struct nsm_msg_bridge_port));
  msg.cindex = 0;
  msg.ifindex = port->ifindex;
  msg.num = 1;
  msg.instance = XCALLOC(MTYPE_TMP, msg.num * sizeof(u_int32_t));
  msg.state = XCALLOC(MTYPE_TMP, msg.num * sizeof(u_int32_t));

  /* For edge ports, if it is set as discarding, fdb should not be flushed
   * Section 17.10 of 802.1W-2001 */

  if ((port->oper_edge)
      && (state == STATE_DISCARDING))
    {
      /* For CIST, the instance is 0 */
      msg.state[0] = NSM_BRIDGE_PORT_STATE_DISCARDING_EDGE;
    }
  else
    {
      /* For CIST, the instance is 0 */
      msg.state[0] = mstp_map_port2msg_state(port->cist_state);
    }

#ifdef HAVE_PROVIDER_BRIDGE
  if (BRIDGE_TYPE_CVLAN_COMPONENT (port->br))
    {
      br = NULL;
      ce_port = NULL;
      ifp = NULL;

      if (vr != NULL)
	ifp = if_lookup_by_index (&vr->ifm, port->ifindex);

      if (ifp != NULL && ifp->port != NULL)
	  ce_port = ifp->port;

       if (ce_port != NULL)
	   br = ce_port->br;

       if (br != NULL)
	  pal_strncpy(msg.bridge_name, br->name, NSM_BRIDGE_NAMSIZ);
    }
  else
    pal_strncpy(msg.bridge_name, port->br->name, NSM_BRIDGE_NAMSIZ);
#else
  pal_strncpy(msg.bridge_name, port->br->name, NSM_BRIDGE_NAMSIZ);
#endif /*HAVE_PROVIDER_BRIDGE */

  NSM_SET_CTYPE(msg.cindex, NSM_BRIDGE_CTYPE_PORT_STATE);

  /* Send port state msg to NSM */
  if ((mstpm->nc != NULL) &&
      (!CHECK_FLAG (mstpm->nc->nsm_server_flags, NSM_SERVER_SHUTDOWN)))
      nsm_client_send_bridge_port_msg(mstpm->nc, &msg,
                                      NSM_MSG_BRIDGE_PORT_STATE);

  XFREE(MTYPE_TMP, msg.instance);
  XFREE(MTYPE_TMP, msg.state);                                                  

  /* Figure 13-19. We generate TC only when a port is added to 
   * active topology, meaning only when the role changes from 
   * Alternate/ Backup to Root/ Designated and port moves to
   * forwarding when the protocol version is MSTP.
   */

  if (state == STATE_FORWARDING
      && ! port->oper_edge
      && port->cist_tc_state == TC_INACTIVE)
    {

      if (! l2_is_timer_running (port->tc_timer))
        {
          port->tc_timer = l2_start_timer(mstp_tc_timer_handler, port,
                                          mstp_cist_get_tc_time(port),mstpm);
#ifdef HAVE_HA
          mstp_cal_create_mstp_tc_timer (port);
#endif /* HAVE_HA */

          port->newInfoCist = PAL_TRUE;

          if (LAYER2_DEBUG(proto, PROTO))
            zlog_debug (mstpm, "Starting cist tc timer for port %u",
                        port->ifindex);
        }

      /* If Restricted TCN is TRUE - No topology changes are propagated */
      if (port->restricted_tcn == PAL_TRUE)
        {
          zlog_info (mstpm,"mstp_cist_set_tc: "
                     "Restricted TCN set "
                     "Ignoring Rcvd tc for port(%u)",
                     port->ifindex);
        }
      else
        mstp_cist_propogate_topology_change (port);

      port->cist_tc_state = TC_ACTIVE;
    }

#ifdef HAVE_HA
  mstp_cal_modify_mstp_port (port);
#endif /* HAVE_HA */
  
  return RESULT_OK;
}

int
mstp_bridge_port_state_send (char * name, const u_int32_t ifindex)
{
  struct mstp_bridge *br = ( struct mstp_bridge * ) mstp_find_bridge ( name );
  struct mstp_port *port;
  struct mstp_instance_port *inst_port;
  struct nsm_msg_bridge_port msg;
  int i;
                                                                                
  if ( !br )
    return RESULT_ERROR;
                                                                                
  port = mstp_find_cist_port ( br, ifindex );
  if ( port )
    {
      pal_mem_set(&msg, 0, sizeof(struct nsm_msg_bridge_port));
      msg.cindex = 0;
      msg.ifindex = port->ifindex;
      msg.num = 1;
      inst_port = port->instance_list;
      while (inst_port)
        {
          msg.num++;
          inst_port = inst_port->next_inst;
        } 
      msg.instance = XCALLOC(MTYPE_TMP, msg.num * sizeof(u_int32_t));
      msg.state = XCALLOC(MTYPE_TMP, msg.num * sizeof(u_int32_t));
      if ((!msg.instance) || (!msg.state))
        {
          if (msg.instance)
            XFREE(MTYPE_TMP, msg.instance);
          if (msg.state)
            XFREE(MTYPE_TMP, msg.state);
          return RESULT_ERROR;
        }
      msg.state[0] = mstp_map_port2msg_state(port->cist_state);
      inst_port = port->instance_list;
      i = 1;
      while (inst_port)
        {
          msg.instance[i] = inst_port->instance_id;
          msg.state[i] = mstp_map_port2msg_state(inst_port->state);
          i++;
          inst_port = inst_port->next_inst;
        }
      pal_strncpy(msg.bridge_name, port->br->name, NSM_BRIDGE_NAMSIZ);
      NSM_SET_CTYPE(msg.cindex, NSM_BRIDGE_CTYPE_PORT_STATE);
                                                                                
      /* Send port state msg to NSM */
      nsm_client_send_bridge_port_msg(mstpm->nc, &msg,
                                      NSM_MSG_BRIDGE_PORT_STATE);
      XFREE(MTYPE_TMP, msg.instance);
      XFREE(MTYPE_TMP, msg.state);
    }
  return RESULT_OK;
}

void
mstp_cist_set_port_role (struct mstp_port *port, enum port_role role)
{
  struct nsm_msg_bridge_if msg;

  if (port == NULL || port->br == NULL)
    return;

  /* Do not do anything if we have the same role */
  if (port->cist_role == role)
    return;

  /* Start The recent root timer if the prev role was root port. */
  if (port->cist_role == ROLE_ROOTPORT)
    {
      if ( !l2_is_timer_running (port->recent_root_timer))
        {
          port->recent_root_timer = l2_start_timer (mstp_rr_timer_handler,
                                    port, port->br->cist_forward_delay,mstpm);
          port->br->br_all_rr_timer_cnt++;
#ifdef HAVE_HA
          mstp_cal_create_mstp_rr_timer (port);
          mstp_cal_modify_mstp_bridge (port->br);
#endif /* HAVE_HA */
        }
    }
  else if (port->cist_role == ROLE_BACKUP)
    {
      /* If the previous role was backup start 
         the recent backup timer. */

      if (IS_BRIDGE_MSTP (port->br) || IS_BRIDGE_RPVST_PLUS(port->br))
        port->recent_backup_timer = l2_start_timer (mstp_rb_timer_handler,
                                                    port,
                                                    2 * port->cist_hello_time,
                                                    mstpm);
      else
        port->recent_backup_timer = l2_start_timer (mstp_rb_timer_handler,
                                                    port,
                                                    2 * port->br->cist_hello_time,
                                                    mstpm);
#ifdef HAVE_HA
      mstp_cal_create_mstp_rb_timer (port);
#endif /* HAVE_HA */
    }
  else if (port->cist_role == ROLE_ALTERNATE
           && port->br != NULL
           && port->br->alternate_port == port)
    port->br->alternate_port = NULL;

  if (role == ROLE_ALTERNATE)
    port->br->alternate_port = port;
    

  if (role == ROLE_DISABLED
      && port->admin_rootguard == PAL_TRUE)
    port->oper_rootguard = PAL_FALSE;

  /* If the role is not Disabled Alternate or Backup
     start the forawrd delay timer so that the port can
     transition to forwarding state. */
  if ((role == ROLE_ROOTPORT) 
      || (role == ROLE_DESIGNATED))
    {
      if (l2_is_timer_running (port->forward_delay_timer))
        {
#ifdef HAVE_HA
          mstp_cal_delete_mstp_forward_delay_timer (port);
#endif /* HAVE_HA */

          l2_stop_timer(&port->forward_delay_timer);
        }

      if (IS_BRIDGE_STP (port->br))
        {
          if (port->oper_edge == PAL_FALSE)
            {
              port->forward_delay_timer = 
                l2_start_timer (mstp_forward_delay_timer_handler,
                                port, mstp_cist_get_fdwhile_time (port), mstpm);
             }
        }
      else
        {
          if (port->force_version < BR_VERSION_RSTP
              || port->oper_edge == PAL_FALSE)
            {
              /* IEEE 802.1D 2004 std section 17.29.4 */
              if (IS_BRIDGE_RSTP (port->br) && 
                  ((port->cist_role == ROLE_ALTERNATE) ||
                   (port->cist_role == ROLE_BACKUP)) &&
                   (port->cist_state == STATE_DISCARDING))
                {
                   port->forward_delay_timer = 
                   l2_start_timer (mstp_forward_delay_timer_handler,
                                    port, port->br->cist_forward_delay, mstpm);
                }
               else 
                 {
                    port->forward_delay_timer = 
                    l2_start_timer (mstp_forward_delay_timer_handler,
                                port, mstp_cist_get_fdwhile_time (port), mstpm);
                 }
             }
        }
#ifdef HAVE_HA
      mstp_cal_create_mstp_forward_delay_timer (port);
#endif /* HAVE_HA */

      if (LAYER2_DEBUG(proto,PROTO))
        zlog_info (mstpm,"mstp_cist_set_port_role: Forward delay timer started\n ");

    }
  else
    {
      /* Else if the role is not root or designated remove any 
         learned entries on that interface IEEE 802.1w-2001
         Section 17.25 active state, unless it's an edge port. 
         802.1w-2001 Section 17.10 */

      if (port->oper_edge == PAL_FALSE)
        {
          if (LAYER2_DEBUG(proto, PROTO))
            zlog_debug (mstpm," calling flush_fdb_by_port %d role %s", 
                        port->ifindex,mstprole_str[port->cist_role]);
          /* Send message to nsm to flush fdb. */
          pal_mem_set(&msg, 0, sizeof(struct nsm_msg_bridge_if));
          pal_strncpy(msg.bridge_name, port->br->name, NSM_BRIDGE_NAMSIZ);
          msg.num = 1;
          /* Allocate ifindex array. */
          msg.ifindex = XCALLOC(MTYPE_TMP, msg.num * sizeof(u_int32_t));
          if (!msg.ifindex)
            return;
          msg.ifindex[0] = port->ifindex;

          /* Check if NSM is shutdown or not*/
          if ((mstpm->nc != NULL) &&
              (!CHECK_FLAG (mstpm->nc->nsm_server_flags, NSM_SERVER_SHUTDOWN)))
                nsm_client_send_bridge_if_msg(mstpm->nc, &msg,
                                              NSM_MSG_BRIDGE_PORT_FLUSH_FDB);
          /* Free ifindex array. */
          if (msg.ifindex)
            XFREE(MTYPE_TMP,msg.ifindex);

          if (l2_is_timer_running (port->recent_root_timer))
            {
#ifdef HAVE_HA
              mstp_cal_delete_mstp_rr_timer (port);
#endif /* HAVE_HA */
              l2_stop_timer(&port->recent_root_timer);
              port->recent_root_timer = NULL;
              port->br->br_all_rr_timer_cnt--;
#ifdef HAVE_HA
              mstp_cal_modify_mstp_bridge (port->br);
#endif /* HAVE_HA */
            }
        }

      if (l2_is_timer_running(port->tc_timer))
        {
#ifdef HAVE_HA
          mstp_cal_delete_mstp_tc_timer (port);
#endif /* HAVE_HA */ 

          l2_stop_timer (&port->tc_timer);
        }
      port->tc_ack = PAL_FALSE;
    }
  
  if (LAYER2_DEBUG(proto,PROTO))
    zlog_info (mstpm,"mstp_cist_set_port_role:"
               " Bridge (%s) Port (%u) role changed from %s to %s ",
               port->br->name, port->ifindex, mstprole_str[port->cist_role], mstprole_str[role]);
  
  port->cist_role = role;

#ifdef HAVE_HA
  mstp_cal_modify_mstp_port (port);
#endif /* HAVE_HA */

}

/* This function initializes the port structure. 
   The associated timers are cleared. */



void 
mstp_cist_init_port (struct mstp_port * port)
{
  /* Section 8.8.1.4.2 */

  if (BRIDGE_TYPE_CVLAN_COMPONENT (port->br))
    {
      if (port->port_type == MSTP_VLAN_PORT_MODE_CE)
        port->cist_port_id = 0x0fff | ((port->cist_priority << 8) & 0xf000);
      else
        port->cist_port_id = (port->svid & 0x0fff) |
                             ((port->cist_priority << 8) & 0xf000);
    }
  else
    port->cist_port_id = mstp_cist_make_port_id (port);

  port->cist_info_type = INFO_TYPE_DISABLED; 
  port->force_version = BRIDGE_VERSION_GET (port->br);
 
  
  port->cist_tc_state = TC_INACTIVE;
  port->send_proposal = PAL_FALSE;
  port->rcv_proposal  = PAL_FALSE;
  port->agreed = PAL_FALSE;
  port->agree = PAL_FALSE;

  if (port->spanning_tree_disable == PAL_FALSE)
    mstp_cist_set_port_state (port, STATE_DISCARDING);

  mstp_cist_set_port_role (port, ROLE_DISABLED);

#ifdef HAVE_HA
  if (l2_is_timer_running (port->hold_timer))
    mstp_cal_delete_mstp_hold_timer (port);
  if (l2_is_timer_running (port->message_age_timer))
    mstp_cal_delete_mstp_message_age_timer (port);
  if (l2_is_timer_running (port->forward_delay_timer))
    mstp_cal_delete_mstp_forward_delay_timer (port);
  if (l2_is_timer_running (port->hello_timer))
    mstp_cal_delete_mstp_hello_timer (port);
  if (l2_is_timer_running (port->edge_delay_timer))
    mstp_cal_delete_mstp_edge_delay_timer (port);
#endif /* HAVE_HA */

  l2_stop_timer (&port->hold_timer);  
  /* Section 8.8.1.4.3 */
  l2_stop_timer (&port->message_age_timer);
  /* Section 8.8.1.4.6 */
  l2_stop_timer (&port->forward_delay_timer);
  /* Section 8.8.1.4.7 */
  l2_stop_timer (&port->hello_timer);
  /* Section 8.8.1.4.8 */
  l2_stop_timer (&port->edge_delay_timer);

#ifdef HAVE_HA
  mstp_cal_modify_mstp_port (port);
#endif /* HAVE_HA */

}

/* This function enables a port. The port is initialized 
   and port state selection occurs as a result of calling this 
   function.  Section 8.8.2. */

void 
mstp_cist_enable_port (void *the_port)
{
  struct mstp_port *port =the_port;
  struct mstp_instance_port *inst_port;
  struct mstp_bridge *br;

  if (LAYER2_DEBUG (proto, PROTO))
    zlog_debug (mstpm, "Bridge %s port %u enabled",
                port->br->name,
                port->ifindex);

  if (!port)
    return;

  br = port->br;

  if (!br)
    return;

  /* If spanning-tree is not enabled don't enable the port */
  if (!br->mstp_enabled)
    return;

  /* Check if port is already enabled. */
  if ( port->port_enabled ) 
     return;

  port->port_enabled = PAL_TRUE;
  port->oper_edge = port->admin_edge; 
  port->oper_p2p_mac = port->admin_p2p_mac; 

  /* Start the protocol migration state machine */ 
  (port->force_version >= BR_VERSION_RSTP) ? 
    mstp_sendmstp (port) :
    mstp_sendstp (port);
   
  
  port->cist_info_type = INFO_TYPE_AGED;
  
  mstp_cist_port_role_selection (port->br);


  port->rcvd_stp = PAL_FALSE;
  port->rcvd_mstp = PAL_FALSE;
  port->rcvd_rstp = PAL_FALSE;
     
  port->newInfoCist = PAL_FALSE;
  port->newInfoMsti = PAL_FALSE;
  port->helloWhen = 0;
  
#ifdef HAVE_HA
  if (l2_is_timer_running(port->hello_timer))
    mstp_cal_delete_mstp_hello_timer (port);
  if (l2_is_timer_running(port->port_timer))
    mstp_cal_delete_mstp_port_timer (port);
#endif /* HAVE_HA */

  l2_stop_timer (&port->hello_timer); 

  if (IS_BRIDGE_MSTP (port->br) || IS_BRIDGE_RPVST_PLUS(port->br))
    port->hello_timer = l2_start_timer (mstp_hello_timer_handler,port,
                                        port->cist_hello_time, mstpm);
  else
    port->hello_timer = l2_start_timer (mstp_hello_timer_handler,port,
                                        port->br->cist_hello_time, mstpm);

  l2_stop_timer (&port->port_timer); 
  port->port_timer = l2_start_timer (mstp_port_timer_handler,port, ONE_SEC,mstpm);

#ifdef HAVE_HA
  mstp_cal_modify_mstp_port (port);
  mstp_cal_create_mstp_hello_timer (port);
  mstp_cal_create_mstp_port_timer (port);
#endif /* HAVE_HA */

  inst_port = port->instance_list;
  while (inst_port)
    {
#if defined (HAVE_PROVIDER_BRIDGE) || defined (HAVE_B_BEB)
      if (inst_port->br->mstp_enabled == PAL_FALSE)  
        {
          mstp_msti_set_port_state (inst_port, STATE_FORWARDING);
          inst_port = inst_port->next_inst;
          continue;
        }
#endif /*(HAVE_PROVIDER_BRIDGE) ||(HAVE_B_BEB)*/
      mstp_msti_enable_port (inst_port);
      inst_port = inst_port->next_inst;
    }

#ifdef HAVE_RPVST_PLUS
  if (port->br->type == NSM_BRIDGE_TYPE_RPVST_PLUS)
    port->rpvst_event = RPVST_PLUS_TIMER_EVENT;
#endif

  mstp_tx_bridge (br);
}

/* This function disables a port. Port state selection occurs
   as a result of calling this function. */
void 
mstp_cist_disable_port (void *the_port)
{
  struct mstp_port *port = the_port;
  struct mstp_instance_port *inst_port;
  bool_t is_root = mstp_bridge_is_cist_root (port->br);

  /* Check if port is already disabled. */
  if ( !port->port_enabled ) 
    {
     if (port->spanning_tree_disable == PAL_TRUE)
        port->cist_state = STATE_DISCARDING;
      return;
    }
#ifdef HAVE_HA
  mstp_cal_modify_mstp_port (port);
#endif /* HAVE_HA */

  port->port_enabled = PAL_FALSE;

  /* Set the state in the forwarder. */
  mstp_cist_set_port_state (port, STATE_DISCARDING);
  mstp_cist_set_port_role (port, ROLE_DISABLED);
  port->cist_info_type = INFO_TYPE_DISABLED; 
  port->cist_tc_state = TC_INACTIVE;
  
#ifdef HAVE_HA
  if (l2_is_timer_running (port->message_age_timer))
    mstp_cal_delete_mstp_message_age_timer (port);
  if (l2_is_timer_running (port->forward_delay_timer))
    mstp_cal_delete_mstp_forward_delay_timer (port);
  if (l2_is_timer_running (port->migrate_timer))
    mstp_cal_delete_mstp_migrate_timer (port);
  if (l2_is_timer_running (port->hold_timer))
    mstp_cal_delete_mstp_hold_timer (port);
  if (l2_is_timer_running (port->hello_timer))
    mstp_cal_delete_mstp_hello_timer (port);
  if (l2_is_timer_running (port->port_timer))
    mstp_cal_delete_mstp_port_timer (port);
  if (l2_is_timer_running (port->recent_backup_timer))
    mstp_cal_delete_mstp_rb_timer (port);
  if (l2_is_timer_running (port->tc_timer))
    mstp_cal_delete_mstp_tc_timer (port);
  if (l2_is_timer_running (port->edge_delay_timer))
    mstp_cal_delete_mstp_edge_delay_timer (port);
#endif /* HAVE_HA */

  /* Stop all port timers */
  l2_stop_timer (&port->message_age_timer);
  l2_stop_timer (&port->forward_delay_timer);
  l2_stop_timer (&port->migrate_timer);
  l2_stop_timer (&port->hold_timer);
  l2_stop_timer (&port->hello_timer);
  l2_stop_timer (&port->port_timer);
  l2_stop_timer (&port->recent_backup_timer);
  l2_stop_timer (&port->tc_timer);
  l2_stop_timer (&port->edge_delay_timer);

  if (l2_is_timer_running (port->recent_root_timer))
    {
#ifdef HAVE_HA
      mstp_cal_delete_mstp_rr_timer (port);
#endif /* HAVE_HA */

      l2_stop_timer (&port->recent_root_timer);
      port->recent_root_timer = NULL;
      port->br->br_all_rr_timer_cnt--;
#ifdef HAVE_HA
      mstp_cal_modify_mstp_bridge (port->br);
#endif /* HAVE_HA */
    }

  port->reselect = PAL_TRUE;
  port->selected = PAL_FALSE;

  mstp_cist_port_role_selection (port->br);

  /* QUES :Do we need this here orin a seperate notification function ?? */
  /* Inform each instance of the disabled status */
  inst_port = port->instance_list;
  while (inst_port)
    {
      if (LAYER2_DEBUG(proto, PROTO))
        zlog_debug (mstpm, "mstp_cist_disable_port:");

      mstp_msti_disable_port (inst_port);
      inst_port = inst_port->next_inst;
    }
  
  if (IS_BRIDGE_STP (port->br) && ! is_root 
      && mstp_bridge_is_cist_root (port->br))
    stp_topology_change_detection (port->br);

  mstp_tx_bridge (port->br);
}
u_int16_t
mstp_map_port2msg_next_state(int state)
{
  switch(state)
    {
    case STATE_DISCARDING:
      return NSM_BRIDGE_PORT_STATE_LISTENING;
      break;
    case STATE_LISTENING:
      return NSM_BRIDGE_PORT_STATE_LEARNING;
      break;
    case STATE_LEARNING:
      return NSM_BRIDGE_PORT_STATE_FORWARDING;
      break;
    case STATE_FORWARDING:
      return NSM_BRIDGE_PORT_STATE_DISCARD_BLK;
      break;
    case STATE_BLOCKING:
      return NSM_BRIDGE_PORT_STATE_FORWARDING;
      break;
    default:
      return RESULT_OK;
      break;
    }
}
int
mstp_msti_set_port_state (struct mstp_instance_port *inst_port, 
                          enum port_state state)
{
  struct mstp_port *port = NULL;
  struct nsm_msg_bridge_port msg;
   
  if (!inst_port)
    return RESULT_ERROR;

  port = inst_port->cst_port;

  if (!port)
    return RESULT_ERROR;

  /* Don't do anything if we are already in that state. */
  if (inst_port->state == state)
    return RESULT_OK;
  
#ifdef HAVE_HA
  mstp_cal_modify_mstp_instance_port (inst_port);
#endif /* HAVE_HA */
 
  /* Stop the recent root timer if its running for 
     change to disscarding state. */
  if (state == STATE_DISCARDING)
    {
      if (l2_is_timer_running (inst_port->recent_root_timer))
        {
#ifdef HAVE_HA
          mstp_cal_delete_mstp_inst_rr_timer (inst_port);
#endif /* HAVE_HA */

          l2_stop_timer(&inst_port->recent_root_timer);
          inst_port->recent_root_timer = NULL;
          inst_port->br->br_inst_all_rr_timer_cnt--;
#ifdef HAVE_HA
          mstp_cal_modify_mstp_bridge_instance (inst_port->br);
#endif /* HAVE_HA */
        }
    }
#if defined (HAVE_PROVIDER_BRIDGE) || defined (HAVE_B_BEB)
  if ((inst_port->role == ROLE_ALTERNATE || inst_port->role == ROLE_DISABLED ||
        inst_port->role == ROLE_BACKUP) && state == STATE_FORWARDING)
    {
      if (inst_port->br->mstp_enabled == PAL_TRUE) 
        return RESULT_OK;
    }
#endif /* HAVE_PROVIDER_BRIDGE || defined (HAVE_BEB) */

  if (LAYER2_DEBUG(proto, PROTO))
    zlog_info(mstpm,"mstp_msti_set_port_state:"
              " Bridge (%s) Port(%u) instance %d state changed from %d to %d",
              port->br->name, port->ifindex, inst_port->instance_id, 
              inst_port->state, state);
  
  inst_port->state = state;
  inst_port->next_state = mstp_map_port2msg_next_state(state);
  /* send message to nsm to set port state and learning. */
  pal_mem_set(&msg, 0, sizeof(struct nsm_msg_bridge_port));
  msg.cindex = 0;
  msg.ifindex = inst_port->ifindex;
  msg.num = 1;
  msg.instance = XCALLOC(MTYPE_TMP, msg.num * sizeof(u_int32_t));
  msg.state = XCALLOC(MTYPE_TMP, msg.num * sizeof(u_int32_t));
  msg.instance[0] = inst_port->instance_id;
  /* For edge ports, if it is set as discarding, fdb should not be flushed
   * Section 17.10 of 802.1W-2001 */

  if ((port->oper_edge)
      && (state == STATE_DISCARDING))
    {
      /* For CIST, the instance is 0 */
      msg.state[0] = NSM_BRIDGE_PORT_STATE_DISCARDING_EDGE;
    }
  else
    {
      msg.state[0] = mstp_map_port2msg_state(inst_port->state);
    }

  pal_strncpy(msg.bridge_name, inst_port->br->bridge->name, NSM_BRIDGE_NAMSIZ);
  NSM_SET_CTYPE(msg.cindex, NSM_BRIDGE_CTYPE_PORT_STATE);
                                                                          
  /* Send port state msg to NSM */
  if ((mstpm->nc != NULL) &&
      (!CHECK_FLAG (mstpm->nc->nsm_server_flags, NSM_SERVER_SHUTDOWN)))
       nsm_client_send_bridge_port_msg(mstpm->nc, &msg,
                                       NSM_MSG_BRIDGE_PORT_STATE);
  
  XFREE(MTYPE_TMP, msg.instance);
  XFREE(MTYPE_TMP, msg.state);                                                                          
  if (state == STATE_FORWARDING
      && !port->oper_edge
      && inst_port->msti_tc_state == TC_INACTIVE)
    {

      if (! l2_is_timer_running (inst_port->tc_timer))
        {
          if (LAYER2_DEBUG(proto, PROTO))
            zlog_debug (mstpm, "Starting msti tc timer for port %u inst %d ",
                        inst_port->ifindex,inst_port->instance_id);

          inst_port->tc_timer = l2_start_timer(mstp_msti_tc_timer_handler,
                                               inst_port,
                                               mstp_msti_get_tc_time(inst_port),
                                               mstpm);
          port->newInfoMsti = PAL_TRUE;
#ifdef HAVE_RPVST_PLUS
          inst_port->newInfoSstp = PAL_TRUE;
#endif /* HAVE_RPVST_PLUS */
#ifdef HAVE_HA
          mstp_cal_modify_mstp_port (port);
          mstp_cal_create_mstp_inst_tc_timer (inst_port);
#endif /* HAVE_HA */
        }

      /* If Restricted TCN is TRUE - No topology changes are propagated */
      if (port->restricted_tcn == PAL_TRUE)
        {
          zlog_info (mstpm,"mstp_cist_set_tc: "
                     "Restricted TCN set "
                     "Ignoring Rcvd tc for port(%u)",
                     port->ifindex);
        }
      else
        mstp_msti_propogate_topology_change (inst_port);

      inst_port->msti_tc_state = TC_ACTIVE;
    }

#ifdef HAVE_HA
  mstp_cal_modify_mstp_instance_port (inst_port);
#endif /* HAVE_HA */

  return RESULT_OK;
}

void
mstp_msti_set_port_role (struct mstp_instance_port *inst_port, 
                         enum port_role role)
{
  struct nsm_msg_bridge_if msg;  
  struct mstp_port *port = NULL;
  int forward = PAL_FALSE;
 
  if (!inst_port)
    return;

  port = inst_port->cst_port;

  if (!port)
    return;

  /* Do not do anything if we have the same role */
  if (inst_port->role == role)
    return;
  
  /* Start The recent root timer if the prev role was root port. */
  if (inst_port->role == ROLE_ROOTPORT)
    {
      port->any_msti_rootport-- ;
#ifdef HAVE_RPVST_PLUS
      inst_port->any_msti_rootport-- ;
#endif /* HAVE_RPVST_PLUS */
      if (!l2_is_timer_running (inst_port->recent_root_timer))
        {
          inst_port->recent_root_timer = l2_start_timer (
                                         mstp_msti_rr_timer_handler, inst_port,
                                         port->br->cist_forward_delay, mstpm);
          inst_port->br->br_inst_all_rr_timer_cnt++;
#ifdef HAVE_HA
          mstp_cal_create_mstp_inst_rr_timer (inst_port);
          mstp_cal_modify_mstp_bridge_instance (inst_port->br);
#endif /* HAVE_HA */
        }
    }
  else if (inst_port->role == ROLE_BACKUP)
    {
      /* If the previous role was backup start 
         the recent backup timer. */
      if (IS_BRIDGE_MSTP (port->br) || IS_BRIDGE_RPVST_PLUS(port->br))
        inst_port->recent_backup_timer = 
                                   l2_start_timer (mstp_msti_rb_timer_handler,
                                   inst_port, 
                                   2 * port->cist_hello_time, mstpm);
      else
        inst_port->recent_backup_timer = 
                                   l2_start_timer (mstp_msti_rb_timer_handler,
                                   inst_port, 
                                   2 * port->br->cist_hello_time, mstpm);
#ifdef HAVE_HA
      mstp_cal_create_mstp_inst_rb_timer (inst_port);
#endif /* HAVE_HA */

    }
  else if (inst_port->role == ROLE_DESIGNATED)
    {
      port->any_msti_desigport-- ;
#ifdef HAVE_RPVST_PLUS
      inst_port->any_msti_desigport-- ;
#endif /* HAVE_RPVST_PLUS */
    }

  /* If the role is not Disabled Alternate or Backup
     start the forawrd delay timer so that the port can
     transition to forwarding state. */
  if (role == ROLE_ROOTPORT) 
    {
      port->any_msti_rootport++ ;
      forward = PAL_TRUE;
#ifdef HAVE_RPVST_PLUS
      inst_port->any_msti_rootport++ ;
#endif /* HAVE_RPVST_PLUS */

    }
  else if (role == ROLE_DESIGNATED)
    {
      port->any_msti_desigport++ ;
      forward = PAL_TRUE;
#ifdef HAVE_RPVST_PLUS
      inst_port->any_msti_desigport++ ;
#endif /* HAVE_RPVST_PLUS */
    }
  else if (role == ROLE_MASTERPORT)
    {
      forward = PAL_TRUE; 
    }  
  else
    {
      /* Else if the role is not root or designated remove any 
         learned entries on that interface IEEE 802.1w-2001
         Section 17.25 Inactive state, unless it's an edge port. 
         802.1w-2001 Section 17.10 */

      if (port->oper_edge == PAL_FALSE)
        { 
          if (LAYER2_DEBUG(proto, PROTO))
            zlog_debug (mstpm, " set mstp flush fdb %d  inst %d role %s",
                        port->ifindex,inst_port->instance_id, mstprole_str[role]);

          /* send message to nsm to flush fdb. */
          pal_mem_set(&msg, 0, sizeof(struct nsm_msg_bridge_if));
          pal_strncpy(msg.bridge_name, port->br->name, NSM_BRIDGE_NAMSIZ);
          msg.num = 1;
          /* Allocate ifindex array. */
          msg.ifindex = XCALLOC(MTYPE_TMP, msg.num * sizeof(u_int32_t));
          if (! msg.ifindex)
            return;
          msg.ifindex[0] = port->ifindex;

          /* Check if NSM is shutdown or not*/ 
          if ((mstpm->nc != NULL) &&
              (!CHECK_FLAG (mstpm->nc->nsm_server_flags, NSM_SERVER_SHUTDOWN)))
               nsm_client_send_bridge_if_msg(mstpm->nc, &msg,
                                             NSM_MSG_BRIDGE_PORT_FLUSH_FDB);
          /* Free ifindex array. */
          if (msg.ifindex)
            XFREE (MTYPE_TMP, msg.ifindex);
        }
      /* 802.1w Fig 17-15. rrWhile should be zero */
      if (l2_is_timer_running (inst_port->recent_root_timer))
        {
#ifdef HAVE_HA
          mstp_cal_delete_mstp_inst_rr_timer (inst_port);
#endif /* HAVE_HA */

          l2_stop_timer(&inst_port->recent_root_timer);
          inst_port->recent_root_timer = NULL;
          inst_port->br->br_inst_all_rr_timer_cnt--;
#ifdef HAVE_HA
          mstp_cal_modify_mstp_bridge_instance (inst_port->br);
#endif /* HAVE_HA */
        }

      if (l2_is_timer_running(inst_port->tc_timer))
        {
#ifdef HAVE_HA
          mstp_cal_delete_mstp_inst_tc_timer (inst_port);
#endif /* HAVE_HA */

          l2_stop_timer (&inst_port->tc_timer);
        }

      port->tc_ack = PAL_FALSE;
#ifdef HAVE_RPVST_PLUS
      inst_port->tc_ack = PAL_FALSE;
#endif /* HAVE_RPVST_PLUS */

    }

  if (forward)
    {
      if (l2_is_timer_running (inst_port->forward_delay_timer))
        {
#ifdef HAVE_HA
          mstp_cal_delete_mstp_inst_forward_delay_timer (inst_port);
#endif /* HAVE_HA */

          l2_stop_timer (&inst_port->forward_delay_timer);
        }
      inst_port->forward_delay_timer = 
        l2_start_timer (mstp_msti_forward_delay_timer_handler,
                        inst_port, 
                        mstp_msti_get_fdwhile_time (inst_port), mstpm);
      if (LAYER2_DEBUG(proto,PROTO))
        zlog_info (mstpm,"mstp_msti_set_port_role: Forward delay timer started\n ");

#ifdef HAVE_HA
      mstp_cal_create_mstp_inst_forward_delay_timer (inst_port);
#endif /* HAVE_HA */
    }
  
  if (LAYER2_DEBUG(proto,PROTO))
    zlog_info (mstpm,"mstp_msti_set_port_role:"
               " Bridge (%s) Port (%u) role changed from %s to %s ",
               port->br->name, inst_port->ifindex, mstprole_str[inst_port->role], 
               mstprole_str[role]);
  
  inst_port->role = role;

  if (role == ROLE_ALTERNATE || role == ROLE_BACKUP)
    {
      mstp_msti_set_port_state (inst_port, STATE_DISCARDING);
    }


#ifdef HAVE_HA
      mstp_cal_modify_mstp_port (port);
      mstp_cal_modify_mstp_instance_port (inst_port);
#endif /* HAVE_HA */

}

/* This function initializes the port structure. 
   The associated timers are cleared. */
void 
mstp_msti_init_port (struct mstp_instance_port * inst_port)
{
  /* TODO struct mstp_port *port = inst_port->cst_port;
     port->force_version = BR_DEF_VERSION;
  */
  /* Section 8.8.1.4.2 */
  inst_port->msti_port_id = mstp_msti_make_port_id (inst_port);
  inst_port->info_type = INFO_TYPE_DISABLED; 
 
  if (LAYER2_DEBUG(proto, PROTO))
    zlog_debug (mstpm, "mstp_msti_init_port:");
  
  mstp_msti_set_port_state (inst_port, STATE_DISCARDING);
  mstp_msti_set_port_role (inst_port,ROLE_DISABLED);
  inst_port->msti_tc_state = TC_INACTIVE;
  inst_port->agree = PAL_FALSE;

#ifdef HAVE_HA
  if (l2_is_timer_running (inst_port->message_age_timer))
    mstp_cal_delete_mstp_inst_message_age_timer (inst_port);
  if (l2_is_timer_running (inst_port->forward_delay_timer))
    mstp_cal_delete_mstp_inst_forward_delay_timer (inst_port);
#endif /* HAVE_HA */

  /* Section 8.8.1.4.3 */
  l2_stop_timer (&inst_port->message_age_timer);
  /* Section 8.8.1.4.6 */
  l2_stop_timer (&inst_port->forward_delay_timer);

#ifdef HAVE_HA
  mstp_cal_modify_mstp_instance_port (inst_port);
#endif /* HAVE_HA */

}

/* This function enables a port. The port is initialized 
   and port state selection occurs as a result of calling this 
   function.  Section 8.8.2. */

void 
mstp_msti_enable_port (void *the_port)
{
  struct mstp_instance_port *port = the_port;

  if (LAYER2_DEBUG (proto, PROTO))
    zlog_debug (mstpm, "Bridge %s port %u enabled",
                port->br->bridge->name,
                port->ifindex);

  
  port->info_type = INFO_TYPE_AGED;
 
#ifdef HAVE_HA
  mstp_cal_modify_mstp_instance_port (port);
#endif /* HAVE_HA */
 
  mstp_msti_port_role_selection (port->br);

  if (port->br != NULL
      && port->br->bridge != NULL)
    {
      mstp_tx_bridge (port->br->bridge);
    }

  return;
}

/* This function disables a port. Port state selection occurs
   as a result of calling this function. */
void 
mstp_msti_disable_port (void *the_port)
{
  struct mstp_instance_port *inst_port = the_port;
  
  /* Set the state in the forwarder. */
  if (LAYER2_DEBUG(proto, PROTO))
    zlog_debug (mstpm, "mstp_msti_disable_port:");
  
  mstp_msti_set_port_state (inst_port, STATE_DISCARDING);
  mstp_msti_set_port_role (inst_port, ROLE_DISABLED);
  inst_port->info_type = INFO_TYPE_DISABLED; 
  inst_port->msti_tc_state = TC_INACTIVE;
  
#ifdef HAVE_HA
  mstp_cal_delete_mstp_inst_message_age_timer (inst_port);
  mstp_cal_delete_mstp_inst_forward_delay_timer (inst_port);
  mstp_cal_delete_mstp_inst_rb_timer (inst_port);
  mstp_cal_delete_mstp_inst_tc_timer (inst_port);
#endif /* HAVE_HA */

  /* Stop all port timers */
  l2_stop_timer (&inst_port->message_age_timer);
  l2_stop_timer (&inst_port->forward_delay_timer);
  l2_stop_timer (&inst_port->recent_backup_timer);
  l2_stop_timer (&inst_port->tc_timer);

  if (l2_is_timer_running (inst_port->recent_root_timer))
    {
#ifdef HAVE_HA
      mstp_cal_delete_mstp_inst_rr_timer (inst_port);
#endif /* HAVE_HA */

      l2_stop_timer (&inst_port->recent_root_timer);
      inst_port->recent_root_timer = NULL;
      inst_port->br->br_inst_all_rr_timer_cnt--;
#ifdef HAVE_HA
      mstp_cal_modify_mstp_bridge_instance (inst_port->br);
#endif /* HAVE_HA */
    }

  inst_port->reselect = PAL_TRUE;
  inst_port->selected = PAL_FALSE;

#ifdef HAVE_HA
  mstp_cal_modify_mstp_instance_port (inst_port);
#endif /* HAVE_HA */

  mstp_msti_port_role_selection (inst_port->br);

  if (inst_port->br != NULL
      && inst_port->br->bridge != NULL)
    {
      mstp_tx_bridge (inst_port->br->bridge);
    }

  return;
}


/* This functions sets the port priority for the selected port.
   The designated port may change as a result of calling this function. 
   Section 8.8.5 */
void 
mstp_cist_set_port_priority (struct mstp_port * port, int new_priority)
{
  u_int16_t new_port_id;

  /* Section 8.8.5.1 */
  port->cist_priority = new_priority & 0xff;
  new_port_id = mstp_cist_make_port_id (port);

  if (mstp_cist_is_designated_port (port))
    port->cist_designated_port_id = new_port_id;

  port->cist_port_id = new_port_id;

#ifdef HAVE_HA
  mstp_cal_modify_mstp_port (port);
#endif /* HAVE_HA */

}



/* This functions sets the port priority for the selected port.
   The designated port may change as a result of calling this function. 
   Section 8.8.5 */
void 
mstp_msti_set_port_priority (struct mstp_instance_port * inst_port, 
                             int new_priority)
{
  short new_port_id;

  /* Section 8.8.5.1 */
  inst_port->msti_priority = new_priority & 0xff;
  new_port_id = mstp_msti_make_port_id (inst_port);

  if (mstp_msti_is_designated_port (inst_port))
    inst_port->designated_port_id = new_port_id;

  inst_port->msti_port_id = new_port_id;

#ifdef HAVE_HA
  mstp_cal_modify_mstp_instance_port (inst_port);
#endif /* HAVE_HA */
}

/* This function updates the path cost associated with a port.
   The  port state may change as a result of calling this function.  
   Section 8.8.6 */
void 
mstp_set_cist_port_path_cost (void *the_port)
{
  struct mstp_port *port = the_port;

  /* Section 8.8.6.1 */
  if (port)
  {
    port->cist_path_cost = 
          mstp_nsm_get_port_path_cost (mstpm, port->ifindex);

    mstp_cist_port_role_selection (port->br);

    mstp_tx_bridge (port->br);
#ifdef HAVE_HA
  mstp_cal_modify_mstp_port (port);
#endif /* HAVE_HA */
  }
}


void 
mstp_msti_set_port_path_cost (void *mstp_port, u_int32_t path_cost)
{
  struct mstp_instance_port *inst_port = mstp_port;

  /* Section 8.8.6.1 */
  inst_port->msti_path_cost = path_cost;

  mstp_msti_port_role_selection (inst_port->br);

  mstp_tx_bridge (inst_port->br->bridge);

#ifdef HAVE_HA
  mstp_cal_modify_mstp_instance_port (inst_port);
#endif /* HAVE_HA */
}

/* Set the configured path cost flag */
void
mstp_set_port_pathcost_configured (struct interface *ifp, int flag)
{
  struct mstp_port *port = ifp->port;
  if (port)
    port->pathcost_configured = flag;

#ifdef HAVE_HA
  mstp_cal_modify_mstp_port (port);
#endif /* HAVE_HA */
}


inline u_int16_t
mstp_cist_make_port_id (struct mstp_port * port)
{
  return (u_int16_t) ((port->ifindex & 0x0fff) | 
                      ((port->cist_priority << 8) & 0xf000));
}

int
mstp_is_port_up(const u_int32_t ifindex)
{
  struct apn_vr *vr = apn_vr_get_privileged ( mstpm );
  struct interface *ifp = NULL;

  ifp = if_lookup_by_index (&vr->ifm, ifindex);
  if (ifp)
    {
      if (if_is_running (ifp))
        return PAL_TRUE;
    }
  return PAL_FALSE;

}

#if defined (HAVE_PROVIDER_BRIDGE) || defined (HAVE_B_BEB)
int
mstp_msti_set_port_state_forward (struct mstp_instance_port *port)  
{
   struct nsm_msg_bridge_port msg;

   /* Don't do anything if we are already in that state and
      spanning tree is disabled. */

   if (port->br->mstp_enabled == PAL_FALSE && port->state == STATE_FORWARDING)
        return RESULT_OK;

   /* mstpm is global defined in mstp_main.c */
   if (LAYER2_DEBUG(proto, PROTO))
     zlog_info(mstpm,"mstp_mist_set_port_state_forward:"
               "Bridge (%s) Instance(%u) Port(%u) state changed from %d to %d",
               port->br->bridge->name, port->instance_id, port->ifindex,
               port->state, STATE_FORWARDING);

   port->state = STATE_FORWARDING;

   if (port->msti_tc_state == TC_INACTIVE)
     {
         /* Start topology change timer
          * propogate the topology change */
         if (l2_is_timer_running (port->tc_timer))
           {
#ifdef HAVE_HA
              mstp_cal_delete_mstp_inst_tc_timer (port);
#endif /* HAVE_HA */

              l2_stop_timer(&port->tc_timer);
           }

         if (LAYER2_DEBUG(proto, PROTO))
           zlog_debug (mstpm, "Starting msti tc timer for port %u",
                       port->ifindex);

         port->msti_tc_state = TC_ACTIVE;
     }

   /* Send messsage to nsm to set port state and learn. */
   pal_mem_set(&msg, 0, sizeof(struct nsm_msg_bridge_port));
   msg.cindex = 0;
   msg.ifindex = port->ifindex;
   msg.num = 1;
   msg.instance = XCALLOC(MTYPE_TMP, msg.num * sizeof(u_int32_t));
   msg.state = XCALLOC(MTYPE_TMP, msg.num * sizeof(u_int32_t));

   msg.state[0] = mstp_map_port2msg_state(port->state);
   msg.instance[0] = port->instance_id;
   pal_strncpy(msg.bridge_name, port->br->bridge->name, NSM_BRIDGE_NAMSIZ);
   NSM_SET_CTYPE(msg.cindex, NSM_BRIDGE_CTYPE_PORT_STATE);

   nsm_client_send_bridge_port_msg(mstpm->nc, &msg,NSM_MSG_BRIDGE_PORT_STATE);

   XFREE(MTYPE_TMP, msg.instance);
   XFREE(MTYPE_TMP, msg.state);

#ifdef HAVE_HA
   mstp_cal_modify_mstp_instance_port (port);
#endif /* HAVE_HA */

   return RESULT_OK;
}
#endif /*(HAVE_PROVIDER_BRIDGE)||(HAVE_B_BEB)*/
