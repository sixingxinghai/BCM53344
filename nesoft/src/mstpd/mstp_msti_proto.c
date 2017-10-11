/* Copyright (C) 2003-2004  All Rights Reserved. 

MSTP Protocol State Machine for each instance 

*/
#include "pal.h"
#include "l2lib.h"
#include "mstp_config.h"
#include "mstp_types.h"
#include "mstp_bpdu.h"
#include "mstpd.h"
#include "mstp_msti_proto.h"
#include "l2_timer.h"
#include "mstp_transmit.h" 
#include "mstp_port.h"
#include "mstp_bridge.h"
#include "mstp_api.h"

#include "nsm_message.h"
#include "nsm_client.h"

#ifdef HAVE_RPVST_PLUS
#include "mstp_rpvst.h"
#endif /* HAVE_RPVST_PLUS */

#ifdef HAVE_HA
#include "mstp_cal.h"
#endif /* HAVE_HA */


static int mstp_msti_assign_role (struct mstp_instance_port *inst_port);
static void 
mstp_msti_root_port_selection (struct mstp_bridge_instance *br_inst);
static struct mstp_instance_port *
mstp_msti_cmp_ports (struct mstp_instance_port *inst_port,
                     struct mstp_instance_port *candidate_port);


int
better_or_same_info_msti_mine (struct mstp_instance_port *inst_port,
                               struct mstp_bridge_instance *br_inst)
{
  int root_cmp;
  int designated_br_cmp;

  root_cmp = pal_mem_cmp (&br_inst->msti_designated_root,
                          &inst_port->msti_root,
                          MSTP_BRIDGE_ID_LEN);

  designated_br_cmp = pal_mem_cmp (&br_inst->msti_bridge_id,
                                   &inst_port->designated_bridge,
                                   MSTP_BRIDGE_ID_LEN);

  if (root_cmp < 0
      || (root_cmp == 0
          && br_inst->internal_root_path_cost < inst_port->internal_root_path_cost)
      || (root_cmp == 0
          && br_inst->internal_root_path_cost == inst_port->internal_root_path_cost
          && designated_br_cmp <= 0))
    {
      return PAL_TRUE;
    }

  return PAL_FALSE;

}

int
better_or_same_info_msti_rcvd (struct mstp_instance_port *inst_port,
                               struct mstp_instance_bpdu *inst_bpdu)
{
  int root_cmp;
  int designated_br_cmp;

  root_cmp = pal_mem_cmp (&inst_bpdu->msti_reg_root,
                          &inst_port->msti_root,
                          MSTP_BRIDGE_ID_LEN);

  designated_br_cmp = pal_mem_cmp (&inst_bpdu->bridge_id,
                                   &inst_port->designated_bridge,
                                   MSTP_BRIDGE_ID_LEN);

  if (root_cmp < 0
      || (root_cmp == 0
          && inst_bpdu->msti_internal_rpc < inst_port->internal_root_path_cost)
      || (root_cmp == 0
          && inst_bpdu->msti_internal_rpc == inst_port->internal_root_path_cost
          && designated_br_cmp < 0)
      || (root_cmp == 0
          && inst_bpdu->msti_internal_rpc == inst_port->internal_root_path_cost
          && designated_br_cmp == 0
          && inst_bpdu->msti_port_id <= inst_port->designated_port_id))
    {
      return PAL_TRUE;
    }

  return PAL_FALSE;

}

void
mstp_msti_handle_masterport_transition (struct mstp_instance_port *inst_port)
{
  if (inst_port->state != STATE_FORWARDING)
    {
       if (LAYER2_DEBUG(proto, PROTO_DETAIL))
         zlog_debug (mstpm,"mstp_msti_handle_masterport_transition:");

       mstp_msti_set_port_state (inst_port, STATE_FORWARDING);
    }
  
  return;
}



static void print_macaddr (char *title, struct bridge_id *addr)
{

  zlog_debug (mstpm, "%s - %.2x:%.2x:%.2x:%.2x:%.2x:%.2x",
              title,
              addr->addr[0],
              addr->addr[1],
              addr->addr[2],
              addr->addr[3],
              addr->addr[4],
              addr->addr[5]);
}

/* Sec 13.26.24 */
static inline s_int32_t
mstp_msti_update_message_age (struct mstp_instance_port *inst_port,
                              struct mstp_instance_bpdu *inst_bpdu)
{
  /* decrement the remaining hops */
  inst_port->hop_count = inst_bpdu->rem_hops - 1;

#ifdef HAVE_HA
  mstp_cal_modify_mstp_instance_port (inst_port);
#endif /* HAVE_HA */

  if (inst_port->hop_count > 0)
    {
      return 3*inst_port->cst_port->cist_hello_time;
    }
      
  return 0;
}


/* IEEE 802.1w-2001 sec 17.19.8 */
int
mstp_handle_rcvd_msti_bpdu (struct mstp_instance_port *inst_port, 
                            struct mstp_instance_bpdu *inst_bpdu)
{
  int root_cmp, designated_br_cmp; 
  char *bridge_name;
  struct mstp_instance_port *tmp_inst_port;
  struct mstp_bridge_instance *br_inst;
  struct mstp_bridge *bridge;

  pal_assert (inst_port);
  pal_assert (inst_port->br);
  pal_assert (inst_port->br->bridge);
  br_inst = inst_port->br;
  bridge = inst_port->br->bridge;

  bridge_name = inst_port->br->bridge->name;

  root_cmp = pal_mem_cmp (&inst_bpdu->msti_reg_root, 
                          &inst_port->msti_root, MSTP_BRIDGE_ID_LEN);

  designated_br_cmp  = pal_mem_cmp (&inst_bpdu->bridge_id, 
                                    &inst_port->designated_bridge,
                                    MSTP_BRIDGE_ID_LEN);

  if (LAYER2_DEBUG(proto, PROTO_DETAIL))
    {
      zlog_debug (mstpm,"mstp_handle_rcvd_msti_bpdu: bridge(%s) port(%u) "
                  "root path cost(%u)", bridge_name,
                  inst_port->ifindex, inst_port->internal_root_path_cost);

      zlog_debug (mstpm,"mstp_handle_rcvd_msti_bpdu: designated port(%x) "
                  "port p2p(%d)", inst_port->designated_port_id,
                  inst_port->cst_port->oper_p2p_mac);
      
      zlog_debug (mstpm,"mstp_handle_rcvd_msti_bpdu:"
                  "bpdu role(%d)", inst_bpdu->msti_port_role);

      zlog_debug (mstpm,"mstp_handle_rcvd_msti_bpdu: bpdu port id(%d) "
                  "agreement(%d)", inst_bpdu->msti_port_id, 
                  inst_bpdu->msti_agreement);

      zlog_debug (mstpm,"mstp_handle_rcvd_msti_bpdu: root_cmp (%d)" 
                  "desig br cmp (%d)", root_cmp, designated_br_cmp);

      zlog_debug (mstpm,"mstp_handle_rcvd_msti_bpdu: bpdu internal rpc (%d)",
                  inst_bpdu->msti_internal_rpc);

    }

  if ((inst_bpdu->msti_port_role == ROLE_DESIGNATED) ||
      (inst_bpdu->msti_port_role == ROLE_UNKNOWN))
    {
    
      if (LAYER2_DEBUG(proto, PROTO_DETAIL))
        zlog_debug (mstpm,"mstp_handle_rcvd_msti_bpdu: "
                    "Processing Config BPDU on port(%u) for bridge %s",
                    inst_port->ifindex, bridge_name);

      if ((inst_port->cst_port->oper_p2p_mac)&&
          (inst_bpdu->msti_agreement))
        inst_port->agreed = PAL_TRUE;
      else
        inst_port->agreed = PAL_FALSE;

#ifdef HAVE_HA
      mstp_cal_modify_mstp_instance_port (inst_port);
#endif /* HAVE_HA */
 
      /* See section 17.4.2.2 */
      if (root_cmp < 0)
        {
          /* Strictly better */
          if (LAYER2_DEBUG(proto, PROTO_DETAIL))
            zlog_debug(mstpm,"mstp_handle_rcvd_msti_bpdu: "
                       "Received root id is better on port(%u) ",
                       inst_port->ifindex);
        
          return MSG_TYPE_SUPERIOR_DESIGNATED;
        }
      else if (root_cmp == 0)
        {
          if (inst_bpdu->msti_internal_rpc < inst_port->internal_root_path_cost)
            {
              /* Strictly better */
              if (LAYER2_DEBUG(proto, PROTO_DETAIL))
                zlog_debug(mstpm,"mstp_handle_rcvd_msti_bpdu: "
                           "Received path cost is better on port(%u) "
                           "bpdu-rpc (%u)  port-rpc (%u)",
                           inst_port->ifindex, inst_bpdu->msti_internal_rpc, 
                           inst_port->internal_root_path_cost);
              return MSG_TYPE_SUPERIOR_DESIGNATED;
            }
          else if (inst_bpdu->msti_internal_rpc == 
                   inst_port->internal_root_path_cost)
            {
              if (designated_br_cmp < 0)
                {
                  /* Strictly better */
                  if (LAYER2_DEBUG(proto, PROTO_DETAIL))
                    zlog_debug(mstpm,"mstp_handle_rcvd_msti_bpdu:"
                               " Received desig br id is better on" 
                               "port(%u) ", inst_port->ifindex);
                  return MSG_TYPE_SUPERIOR_DESIGNATED;
                }
              else if (designated_br_cmp == 0)
                {
                  if (inst_bpdu->msti_port_id < inst_port->designated_port_id)
                    {
                      /* Strictly better */
                      if (LAYER2_DEBUG(proto, PROTO_DETAIL))
                        zlog_debug(mstpm,"mstp_handle_rcvd_msti_bpdu:"
                                   " Received desig port id is worse on" 
                                   "port(%u) ", inst_port->ifindex);
                      return MSG_TYPE_SUPERIOR_DESIGNATED;
                    }
                } /* cmp_val == 0 */
            } /*rpc == rootpathcost */
        } 
    
      if ((!designated_br_cmp) && (inst_bpdu->msti_port_id == 
                                   inst_port->designated_port_id))
        {     
          /* If the designated port and designated bridge are the same 
             and the message priority vector as a whole or any of the msg 
             timers is different return superior message - 
             Section 17.19.8 pp. 55 */

         /* According to the IEEE 802.1Q 2003 Section 13.15 Message is
          * superior when hop count in the BPDU is different from that
          * of the port. But according to 13.26.24 updtRcvdInfoWhileMsti
          * the port hop count is decremented by 1. This is contradictory 
          * since if we satisfy both the sections, the BPDU hop count
          * will not be same as the port hop count and hence we will be 
          * returning superior BPDU always even though the BPDU is conveying 
          * the same information. Hence added this work around that we 
          * BPDU hop count with the port hop count incremented by one.
          */

          if ((root_cmp != 0) ||
              (inst_bpdu->msti_internal_rpc != 
               inst_port->internal_root_path_cost) ||
              (inst_bpdu->rem_hops != (inst_port->hop_count + 1) )) 
            {
              /* if timer values are diff msg is superior */
              if (LAYER2_DEBUG(proto, PROTO_DETAIL))
                zlog_debug(mstpm,"mstp_handle_rcvd_msti_bpdu: "
                           "one of received time variables is different");

              if ((inst_port->role == ROLE_ALTERNATE)&&
                  (bridge->topology_type == TOPOLOGY_RING))
                {
                  inst_port->selected_role = ROLE_DESIGNATED;
#ifdef HAVE_HA
                  mstp_cal_modify_mstp_instance_port (inst_port);
#endif /* HAVE_HA */

                  mstp_msti_set_port_state (inst_port, STATE_FORWARDING);

                  return MSG_TYPE_INFERIOR_DESIGNATED;
                }
              else if((inst_port->role == ROLE_ROOTPORT)&&
                      (bridge->topology_type == TOPOLOGY_RING))
                {
                  for (tmp_inst_port = br_inst->port_list;
                       tmp_inst_port; tmp_inst_port = tmp_inst_port->next)
                   {
                    if (tmp_inst_port->role == ROLE_DESIGNATED)
                      {
                        mstp_port_send (tmp_inst_port->cst_port,
                                        inst_bpdu->bpdu->buf,
                                        inst_bpdu->bpdu->len);
                      }
                   }
                }

              return MSG_TYPE_SUPERIOR_DESIGNATED;
            }
          else
            {
              if (LAYER2_DEBUG(proto, PROTO_DETAIL))
                zlog_debug(mstpm,"mstp_handle_rcvd_msti_bpdu:"
                           " received BPDU is repeated message");
              return MSG_TYPE_REPEATED_DESIGNATED;
            }
        }
    }

  if (inst_port->cst_port->oper_p2p_mac
      && (inst_bpdu->msti_port_role == ROLE_ROOTPORT
          || inst_bpdu->msti_port_role == ROLE_ALTERNATE
          || inst_bpdu->msti_port_role == ROLE_BACKUP))
    {

      if (inst_bpdu->msti_agreement)
        inst_port->agreed = PAL_TRUE;
      else
        inst_port->agreed = PAL_FALSE;

#ifdef HAVE_HA
      mstp_cal_modify_mstp_instance_port (inst_port);
#endif /* HAVE_HA */

      return MSG_TYPE_INFERIOR_ALTERNATE;

    }

  if (inst_bpdu->msti_port_role == ROLE_DESIGNATED)
    {

       if (inst_bpdu->msti_learning)
         inst_port->disputed = PAL_TRUE;

#ifdef HAVE_HA
       mstp_cal_modify_mstp_instance_port (inst_port);
#endif /* HAVE_HA */

       return MSG_TYPE_INFERIOR_DESIGNATED;
    }


  if (LAYER2_DEBUG(proto, PROTO_DETAIL))
    zlog_debug(mstpm,"mstp_handle_rcvd_msti_bpdu: "
               "received BPDU has inferior information");
  
  return MSG_TYPE_OTHER_MSG;
}

int
mstp_msti_port_role_selection (struct mstp_bridge_instance *br_inst)
{

  /* Update roles selection for each port 
     for ports which have different selected roles and port roles
     run the port role transition state machine. */
  struct mstp_instance_port *inst_port = NULL;
  pal_assert (br_inst);
  pal_assert (br_inst->bridge);
 
  /* Recompute the roles  if any port asks us to i.e. sets reselect */      
  if (LAYER2_DEBUG(proto, PROTO))
    zlog_debug (mstpm, "mstp_msti_port_role_selection: bridge %s", 
                br_inst->bridge->name);
      
#ifdef HAVE_HA
  mstp_cal_modify_mstp_bridge_instance (br_inst);
#endif /* HAVE_HA */

  mstp_msti_clear_reselect_bridge (br_inst);
  mstp_msti_root_port_selection (br_inst);

  /* Set the port roles */
  
  for (inst_port = br_inst->port_list; 
       inst_port; inst_port = inst_port->next)
    {
      mstp_msti_assign_role (inst_port);
            
      if (inst_port->selected_role != inst_port->role)
        mstp_msti_set_port_role (inst_port, inst_port->selected_role);

#ifdef HAVE_HA
      mstp_cal_modify_mstp_instance_port (inst_port);
#endif /* HAVE_HA */

    }

  mstp_msti_set_selected_bridge (br_inst);

  /* Receive Proposal and set the ReRoot for recent
   * Roots.
   */

  for (inst_port = br_inst->port_list; inst_port; inst_port = inst_port->next)
    {
      /* Port Role Transition State Machine. */
      switch (inst_port->selected_role)
        {
        case ROLE_ROOTPORT:
          mstp_msti_handle_rootport_transition (inst_port);
          break;
         
        case ROLE_MASTERPORT:
          mstp_msti_handle_masterport_transition (inst_port);
          break;
              
        case ROLE_ALTERNATE:
        case ROLE_BACKUP:

          /* Figure 13-19. If the port role is not root or
           * desiganted, Put the TC state to INACTIVE.
           */
          inst_port->br->topology_change_detected = PAL_FALSE;
          inst_port->msti_tc_state = TC_INACTIVE;
          mstp_msti_set_port_state (inst_port, STATE_DISCARDING);

          /* 802.1 D 2004 Section 17.29.4 */

          if (inst_port->rcv_proposal)
            {
              /* ALTERNATE_AGREED */
              if (inst_port->agree)
                {
                  inst_port->cst_port->newInfoMsti = PAL_TRUE;
#ifdef HAVE_RPVST_PLUS
                  inst_port->newInfoSstp = PAL_TRUE;
#endif /* HAVE_RPVST_PLUS */

#ifdef HAVE_HA
                  mstp_cal_modify_mstp_instance_port (inst_port);
#endif /* HAVE_HA */
                }
              else /* ALTERNATE_PROPOSED */
                {
                  mstp_msti_rcv_proposal (inst_port);
                }
            }

          break;

        default:
          break;
        } /* Switch statement */
    } /* For Port loop */

  for (inst_port = br_inst->port_list; inst_port; inst_port = inst_port->next)
    {
      /* Port Role Transition State Machine. */
      switch (inst_port->selected_role)
        {
        case ROLE_DESIGNATED:
          mstp_msti_handle_designatedport_transition (inst_port);
          break;

        default:
          break;
        } /* Switch statement */
    }

              
  if (LAYER2_DEBUG(proto,PROTO_DETAIL))
    zlog_debug (mstpm, "mstp_msti_port_role_selection: End");

  return RESULT_OK;
}/* End Function */

void
mstp_msti_set_port_role_all (struct mstp_port *port,
                             enum port_role role)
{

  struct mstp_instance_port *inst_port;

  if (!port)
    return;

  inst_port = port->instance_list;

  while (inst_port)
    {
      if (inst_port->role != role)
        {
#ifdef HAVE_HA
          mstp_cal_modify_mstp_instance_port (inst_port);
#endif /* HAVE_HA */

          mstp_msti_set_port_role (inst_port, role);

          switch (role)
            {
              case ROLE_MASTERPORT:
                  mstp_msti_handle_masterport_transition (inst_port);
                  break;
              case ROLE_ALTERNATE:
                  mstp_msti_set_port_state (inst_port, STATE_DISCARDING);
                  break;
              default:
                  break;
            }
        }
      inst_port = inst_port->next_inst;
    }
}

/* Implements the UPDATE state of Port Information State Machine.
   IEEE 802.1w-2001 sec 17.21. */
int
mstp_msti_port_config_update (struct mstp_instance_port *inst_port)
{
  struct mstp_bridge_instance *br_inst = inst_port->br;
  /* copy designated priority and designated times to 
   * port priority and times vectors */
  if (LAYER2_DEBUG(proto,PROTO_DETAIL))
    zlog_debug (mstpm, 
                "mstp_msti_port_config_update: bridge %s port(%u)",
                br_inst->bridge->name,
                inst_port->cst_port->ifindex);
  if (!inst_port->updtInfo)
    return RESULT_ERROR;
  
  pal_mem_cpy (&inst_port->msti_root, &br_inst->msti_designated_root, 
               MSTP_BRIDGE_ID_LEN);
  pal_mem_cpy (&inst_port->designated_bridge, 
               &br_inst->msti_bridge_id, MSTP_BRIDGE_ID_LEN);
  inst_port->internal_root_path_cost = br_inst->internal_root_path_cost;
  inst_port->designated_port_id = inst_port->msti_port_id;
  inst_port->hop_count = br_inst->hop_count;
  inst_port->agreed = inst_port->agreed
                      && better_or_same_info_msti_mine (inst_port, br_inst);


#ifdef HAVE_RPVST_PLUS
  inst_port->message_age = br_inst->message_age;
  inst_port->max_age = br_inst->max_age;
  inst_port->fwd_delay = br_inst->fwd_delay;
  inst_port->hello_time = br_inst->hello_time;
#endif /* HAVE_RPVST_PLUS */

  /* updt_info is set to false, agreed sync is set to false
   * proposed and send_proposal are set to false if the
   * port is in forwarding*/
  inst_port->send_proposal = PAL_FALSE;
  inst_port->info_type = INFO_TYPE_MINE;
  inst_port->updtInfo = PAL_FALSE;
  inst_port->agree = PAL_FALSE;

  /*  port->agreed = ((port->agreed) && 
      (better_or_same_info_msti ()) && (!port->changed_master)) ;
  */

  if (LAYER2_DEBUG(proto,PROTO_DETAIL))
    {
      print_macaddr ("mstp_msti_port_config_update: root bridge id ", 
                     &inst_port->msti_root);
      print_macaddr ("mstp_msti_port_config_update: designated bridge id ", 
                     &inst_port->designated_bridge);
      zlog_debug (mstpm, "mstp_msti_port_config_update: root path cost(%d)",
                  inst_port->internal_root_path_cost);
      zlog_debug (mstpm, "mstp_msti_port_config_update: designated port(%d)",
                  inst_port->designated_port_id);
      zlog_debug (mstpm, "mstp_msti_port_config_update: message age(%d) "
                  "max age(%d)", inst_port->message_age, inst_port->max_age);
      zlog_debug (mstpm, "mstp_msti_port_config_update: fwd delay(%d)" 
                  "hello time(%d)", inst_port->fwd_delay,
                  inst_port->hello_time);
    }
 
  inst_port->cst_port->newInfoMsti = PAL_TRUE;
#ifdef HAVE_RPVST_PLUS
  inst_port->newInfoSstp = PAL_TRUE;
#endif /* HAVE_RPVST_PLUS */

#ifdef HAVE_HA
  mstp_cal_modify_mstp_instance_port (inst_port);
#endif /* HAVE_HA */
 
  return RESULT_OK;
}

int
mstp_msti_rcv_proposal (struct mstp_instance_port *curr_port)
{
  struct mstp_bridge_instance *br_inst;
  struct mstp_instance_port *inst_port;
  struct mstp_bridge *br;
  pal_assert (curr_port);
  pal_assert (curr_port->cst_port);
  br_inst = curr_port->br;
  pal_assert (br_inst);
  br = br_inst->bridge;
  pal_assert (br);
  
  /* Set all ports with designated port roles 
   * which are not oper_edge to discarding state. 
   *
   * Synced = true;
   */
  if (LAYER2_DEBUG(proto, PROTO_DETAIL))
    zlog_debug (mstpm, "mstp_msti_rcv_proposal: MSTP received proposal for "
                "port (%u) bridge %s for index %d", 
                curr_port->ifindex, br_inst->bridge->name, 
                curr_port->instance_id);
  for (inst_port = br_inst->port_list;inst_port ;inst_port = inst_port->next)
    {
      if (inst_port != curr_port &&
          inst_port->role == ROLE_DESIGNATED &&
          ! inst_port->cst_port->oper_edge &&
          ! curr_port->agreed)
        {
          inst_port->send_proposal = PAL_TRUE;
          mstp_msti_set_port_state (inst_port, STATE_DISCARDING);

          if (LAYER2_DEBUG(proto, TIMER))
            zlog_debug (mstpm,"mstp_msti_rcv_proposal: start fwd delay timer "
                        "after receiving proposal - %d",
                        br->cist_forward_delay);

#ifdef HAVE_HA
          mstp_cal_delete_mstp_inst_forward_delay_timer (inst_port);
#endif /* HAVE_HA */

          l2_stop_timer (&inst_port->forward_delay_timer);

          inst_port->forward_delay_timer = 
            l2_start_timer (mstp_msti_forward_delay_timer_handler, inst_port,
                            mstp_msti_get_fdwhile_time (inst_port), mstpm);
#ifdef HAVE_HA
          mstp_cal_modify_mstp_instance_port (inst_port);
          mstp_cal_create_mstp_inst_forward_delay_timer (inst_port);
#endif /* HAVE_HA */
        }
    }

  curr_port->agree = PAL_TRUE;
  
  if (LAYER2_DEBUG(proto, PROTO_DETAIL))
    zlog_debug (mstpm,"mstp_msti_rcv_proposal: End  ");
  
  curr_port->cst_port->newInfoMsti = PAL_TRUE;
#ifdef HAVE_RPVST_PLUS
  curr_port->newInfoSstp = PAL_TRUE;
#endif /* HAVE_RPVST_PLUS */

#ifdef HAVE_HA
  mstp_cal_modify_mstp_instance_port (curr_port);
#endif /* HAVE_HA */ 

  return RESULT_OK;
}


void
mstp_msti_send_proposal (struct mstp_instance_port *curr_port)
{
  struct mstp_bridge_instance *br_inst;
  struct mstp_instance_port *inst_port;
  struct mstp_bridge *br;
  pal_assert (curr_port);
  pal_assert (curr_port->cst_port);
  br_inst = curr_port->br;
  pal_assert (br_inst);
  br = br_inst->bridge;
  pal_assert (br);

  if (LAYER2_DEBUG(proto, PROTO_DETAIL))
    zlog_debug (mstpm, "mstp_msti_send_proposal:"
                       " MSTP received repeated message for "
                       "port (%u) bridge %s for index %d",
                       curr_port->ifindex, br_inst->bridge->name,
                       curr_port->instance_id);

  for (inst_port = br_inst->port_list;inst_port ;inst_port = inst_port->next)
    {
      if ((inst_port != curr_port) &&
          (inst_port->role == ROLE_DESIGNATED) &&
          (!inst_port->cst_port->oper_edge))
        {
          inst_port->send_proposal = PAL_TRUE;
          inst_port->cst_port->newInfoMsti = PAL_TRUE;

#ifdef HAVE_RPVST_PLUS
          inst_port->newInfoSstp = PAL_TRUE;
#endif /* HAVE_RPVST_PLUS */
#ifdef HAVE_HA
          mstp_cal_modify_mstp_instance_port (inst_port);
#endif /* HAVE_HA */
        }
    }
  return;
}


/* IEEE 802.1w-2001 sec 17.21
   Implements functionality in SUPERIOR state of Port Information 
   State Machine */
void 
mstp_msti_record_priority_vector (struct mstp_instance_port *inst_port, 
                                  struct mstp_instance_bpdu *inst_bpdu)
{
  struct mstp_port *port;
  pal_assert (inst_port);
  port = inst_port->cst_port;
  pal_assert (port);
  
  if (LAYER2_DEBUG(proto, PROTO_DETAIL))
    zlog_debug (mstpm, "mstp_msti_record_priority_vector: "
                "MSTP designated record priority vector for port (%u) bridge %s"
                "for instance (%d)", inst_port->ifindex, port->br->name,
                inst_port->instance_id);
  
  /* copy port priority (vector) */
  pal_mem_cpy (&inst_port->msti_root, &inst_bpdu->msti_reg_root, 
               MSTP_BRIDGE_ID_LEN); 
  inst_port->internal_root_path_cost = inst_bpdu->msti_internal_rpc; 
  /* Create and store bridge id ?? */
  pal_mem_cpy (&inst_port->designated_bridge,
               &inst_bpdu->bridge_id,MSTP_BRIDGE_ID_LEN);
  /* create and store port id ?? */
  inst_port->designated_port_id = inst_bpdu->msti_port_id;

  /* port times Variable */
  /* Appendix B.3.3.2 Always increment the message age by atleast 1 */
  
  inst_port->hop_count = inst_bpdu->rem_hops;
#ifdef HAVE_RPVST_PLUS
  inst_port->message_age = inst_bpdu->message_age;
#endif /* HAVE_RPVST_PLUS */

  if (LAYER2_DEBUG(proto, PROTO_DETAIL))
    {
      print_macaddr ("mstp_msti_record_priority_vector: root bridge id ", 
                     &inst_port->msti_root);
      print_macaddr ("mstp_msti_record_priority_vector: designated bridge id ", 
                     &inst_port->designated_bridge);
      zlog_debug (mstpm, "mstp_msti_record_priority_vector: root path cost(%d)",
                  inst_port->internal_root_path_cost);
      zlog_debug (mstpm, "mstp_msti_record_priority_vector: designated port(%d)",
                  inst_port->designated_port_id);
      zlog_debug (mstpm, "mstp_msti_record_priority_vector: hop_count (%d) ",
                  inst_port->hop_count);
    }

#ifdef HAVE_HA
  mstp_cal_modify_mstp_instance_port (inst_port);
#endif /* HAVE_HA */

}

static char *
info_type_str (const int info_type)
{
  switch (info_type)
    {
    case INFO_TYPE_DISABLED:
      return "Disabled";
    case INFO_TYPE_AGED:
      return "Aged";
    case INFO_TYPE_MINE:
      return "Mine";
    case INFO_TYPE_RECEIVED:
      return "Received";
    default:
      return "Unknown";
    }
}

static inline int
mstp_is_priority_vec_diff (struct mstp_instance_port *inst_port,
                           struct mstp_bridge_instance *br_inst)
{
  /* what abt designated bridge id check ? */ 
  return  ((pal_mem_cmp (&inst_port->msti_root, 
                         &br_inst->msti_designated_root, MSTP_BRIDGE_ID_LEN))
           || (pal_mem_cmp (&inst_port->designated_bridge, 
                            &br_inst->msti_designated_bridge,
                            MSTP_BRIDGE_ID_LEN))
           || (inst_port->msti_port_id != inst_port->designated_port_id)
           || (inst_port->internal_root_path_cost != 
               br_inst->internal_root_path_cost)
           || (inst_port->hop_count != br_inst->hop_count));
}

static int
mstp_msti_assign_role (struct mstp_instance_port *inst_port)
{
  struct mstp_port *port = NULL;
  struct mstp_bridge_instance *br_inst = NULL;
  char *bridge_name;
 
  pal_assert (inst_port);
  br_inst = inst_port->br;
  pal_assert (br_inst);
  port = inst_port->cst_port;
  pal_assert (port);
  
  bridge_name = br_inst->bridge->name;
  br_inst->master = PAL_FALSE; /* Clear out the information  */

  if (LAYER2_DEBUG(proto, PROTO_DETAIL))
    zlog_debug (mstpm, 
                "mstp_msti_assign_role: bridge %s port %u - port info type (%s)"
                "instance %d ", bridge_name, inst_port->ifindex, 
                info_type_str (inst_port->info_type), inst_port->instance_id);
  
  if (inst_port->info_type == INFO_TYPE_DISABLED)
    {
      /* 802.1w Section 17.19.21.g */
      inst_port->selected_role = ROLE_DISABLED;
      if (LAYER2_DEBUG(proto, PROTO))
        zlog_debug (mstpm,"mstp_msti_assign_role: bridge %s port %u instance"
                    " %d->" "disabled", bridge_name, inst_port->ifindex, 
                    inst_port->instance_id);
      return RESULT_OK;
    }
  
  if ((port->info_internal == PAL_FALSE) && 
      (port->cist_info_type == INFO_TYPE_RECEIVED))
    {
      
      if (port->cist_selected_role == ROLE_ROOTPORT)
        {
          inst_port->selected_role = ROLE_MASTERPORT;
          /* Notify other ports that bridge has selected master port */
          br_inst->master = PAL_TRUE;
          if (LAYER2_DEBUG(proto, PROTO))
            zlog_debug (mstpm,"mstp_msti_assign_role: bridge %s port %u "
                        "-> master", bridge_name, inst_port->ifindex);
          if ( mstp_is_priority_vec_diff (inst_port, br_inst))
            {
              inst_port->updtInfo = PAL_TRUE;
              mstp_msti_port_config_update (inst_port);
            }
          /* 
             if ((msti port prio vector != desig prio vector )
             || (ports associated timers differ from root port))
             mstp_msti_port_config_update (inst_port);
          */
        }
      else if (port->cist_selected_role == ROLE_ALTERNATE)
        {
          inst_port->selected_role = ROLE_ALTERNATE;
          if ( mstp_is_priority_vec_diff (inst_port, br_inst))
            {
              inst_port->updtInfo = PAL_TRUE;
              mstp_msti_port_config_update (inst_port);
            }
          /* update */
        }
      
    }
  else
    {    
    
      switch (inst_port->info_type)
        {
        case INFO_TYPE_AGED:
          {
            /* 802.1w Section 17.19.21.h */
            inst_port->selected_role = ROLE_DESIGNATED;
            inst_port->updtInfo = PAL_TRUE;
            mstp_msti_port_config_update (inst_port);
            
            /* set update info */
            if (LAYER2_DEBUG(proto, PROTO))
              zlog_debug (mstpm,"mstp_msti_assign_role: bridge %s port %u "
                          "-> designated", bridge_name, inst_port->ifindex);
            break;
          }
        case INFO_TYPE_MINE:
          {
            /* 802.1w Section 17.19.21.i */
            inst_port->selected_role = ROLE_DESIGNATED;
            /* If port's priority vector differs from designated
             * priority vector or ports timer params are
             * diff from those of root port. set update info */
            if ( mstp_is_priority_vec_diff (inst_port, br_inst))
              {
                inst_port->updtInfo = PAL_TRUE;
                mstp_msti_port_config_update (inst_port);
              }

            if (LAYER2_DEBUG(proto, PROTO))
              zlog_debug (mstpm,"mstp_msti_assign_role: bridge %s "
                          "port %u -> designated", bridge_name,
                          inst_port->ifindex);
            break;
          }
        case INFO_TYPE_RECEIVED:
          {
            int root_cmp = pal_mem_cmp (&inst_port->msti_root,
                                        &br_inst->msti_designated_root, 
                                        MSTP_BRIDGE_ID_LEN);
            
            if (LAYER2_DEBUG(proto, PROTO))
              {
                zlog_debug (mstpm,
                            "mstp_msti_assign_role: port id %x designated port id %x"
                            " port root pc %d port pc %d",
                            inst_port->msti_port_id, inst_port->designated_port_id,
                            inst_port->internal_root_path_cost, 
                            inst_port->msti_path_cost);
                print_macaddr ("mstp_msti_assign_role: port designated bridge id", 
                               &inst_port->designated_bridge);
                print_macaddr ("mstp_msti_assign_role: port root ", 
                               &inst_port->msti_root);
                print_macaddr ("mstp_msti_assign_role: bridge id ", 
                               &br_inst->msti_bridge_id);
                print_macaddr ("mstp_msti_assign_role: bridge designated bridge ", 
                               &br_inst->msti_designated_bridge);
                print_macaddr ("mstp_msti_assign_role: bridge root ", 
                               &br_inst->msti_designated_root);
                zlog_debug (mstpm, "mstp_msti_assign_role: bridge root pc %u"
                            " root port id %x", br_inst->internal_root_path_cost, 
                            br_inst->msti_root_port_id);
              }
            /* 802.1w Section 17.19.21.j */
            /* If root priority vector is derived from this port.
               This is the root port. */
            if  ((br_inst->msti_root_port_id == inst_port->msti_port_id)
                 && (br_inst->internal_root_path_cost == 
                     inst_port->internal_root_path_cost
                     + inst_port->msti_path_cost)
                 && (root_cmp == 0)
                 && (pal_mem_cmp(&br_inst->msti_designated_bridge, 
                                 &inst_port->designated_bridge,
                                 MSTP_BRIDGE_ID_LEN) == 0))
              {
                inst_port->selected_role = ROLE_ROOTPORT;
                inst_port->updtInfo = PAL_FALSE;
                if (LAYER2_DEBUG(proto, PROTO))
                  zlog_debug (mstpm,"mstp_msti_assign_role: bridge %s port"
                              " %u -> root", bridge_name, port->ifindex);
                /* update info is reset */
              }
            else 
              {
                /* The root priority vector is not derived from 
                   the port priority vector. */
                int bridge_id_cmp = pal_mem_cmp (&inst_port->designated_bridge, 
                                                 &br_inst->msti_bridge_id,
                                                 MSTP_BRIDGE_ID_LEN); 

                /* 802.1w Section 17.19.21.k */
                /* If the root priority vector is not now derived from it,
                   the designated priority vector is not higher than 
                   the port priority vector and the designated bridge and 
                   designated port components of the port priority
                   vector do not reflect another port on this bridge,
                   role is alternate port. */
                if ((root_cmp < 0) ||
                    ((root_cmp == 0) && (inst_port->internal_root_path_cost <
                                         br_inst->internal_root_path_cost)) ||
                    ((root_cmp == 0) && (inst_port->internal_root_path_cost ==
                                         br_inst->internal_root_path_cost) &&
                                         (bridge_id_cmp < 0)) ||
                    ((root_cmp == 0) && (inst_port->internal_root_path_cost ==
                                         br_inst->internal_root_path_cost) && 
                                         (bridge_id_cmp == 0) &&
                     (inst_port->designated_port_id <= inst_port->msti_port_id)))
                  {
                    /* The designated priority vector is not higher than
                       than the port priority vector. */

                    /* Does the port priority vector reflect another port
                       on this bridge? */
                    if (bridge_id_cmp)
                      {
                        inst_port->selected_role = ROLE_ALTERNATE;
                        inst_port->updtInfo = PAL_FALSE;
                        if (LAYER2_DEBUG(proto, PROTO))
                          zlog_debug (mstpm,"mstp_msti_assign_role: bridge %s "
                                      "port %u -> alternate", bridge_name,
                                      port->ifindex);
                      }
                    else if ((!bridge_id_cmp) && 
                             (inst_port->msti_port_id 
                                     != inst_port->designated_port_id))
                      {
                        /* 802.1w Section 17.19.21.k */
                        /* ELSE role is backup */
                        inst_port->selected_role = ROLE_BACKUP;
                        inst_port->updtInfo = PAL_FALSE;
                        if (LAYER2_DEBUG(proto, PROTO))
                          zlog_debug (mstpm,"mstp_msti_assign_role: bridge %s "
                                      "port %u -> backup", bridge_name, port->ifindex);
                      }
                  }
                else
                  {
                    inst_port->selected_role = ROLE_DESIGNATED;
                    inst_port->updtInfo = PAL_TRUE;
                    if (LAYER2_DEBUG(proto, PROTO))
                      zlog_debug (mstpm,"mstp_msti_assign_role: bridge %s "
                                  "port %u -> designated (default)", 
                                  bridge_name, inst_port->ifindex);
                    mstp_msti_port_config_update (inst_port);
                  }
              }
            break;
          } /* Case info_type RECEIVED */
        default:
          zlog_err (mstpm, "Incorrect info type value recieved %d \n",
                    inst_port->info_type);
        } /* Select info_type */

    }/* port->infotype is info_recvd and info internal is false */

#ifdef HAVE_HA
  mstp_cal_modify_mstp_bridge_instance (br_inst);
  mstp_cal_modify_mstp_instance_port (inst_port);
#endif /* HAVE_HA */ 
  
  if (LAYER2_DEBUG(proto,PROTO_DETAIL))
    zlog_debug (mstpm, "mstp_msti_assign_role: End ");
  return RESULT_OK;
}



void
mstp_msti_handle_rootport_transition (struct mstp_instance_port *inst_port)
{
  struct mstp_bridge_instance *br_inst = NULL;
  struct mstp_port *port = NULL;
  struct mstp_bridge *br = NULL;

  if ((!inst_port) || 
      (!(port = inst_port->cst_port)) ||
      (!(br = port->br)))
    {
      zlog_err (mstpm,"mstp_msti_handle_rootport_transition:Invalid Data");
      return ;
    }

  br_inst = inst_port->br;

  if (LAYER2_DEBUG(proto, PROTO))
    zlog_debug (mstpm,
                "mstp_msti_handle_rootport_transition: bridge %s port %u"
                "state %d rcv_proposal %d force version %d",
                br->name, inst_port->ifindex,
                inst_port->state,inst_port->rcv_proposal,port->force_version);

#ifdef HAVE_HA
  mstp_cal_modify_mstp_instance_port (inst_port);
#endif /* HAVE_HA */
  
  if (inst_port->rcv_proposal)
    {

      /* ROOT_AGREED */
      if (inst_port->agree)
        {
          inst_port->cst_port->newInfoMsti = PAL_TRUE;  
#ifdef HAVE_RPVST_PLUS          
          inst_port->newInfoSstp = PAL_TRUE;  
#endif /* HAVE_RPVST_PLUS */
        }
      else /* ROOT_PROPOSED */
        {
          mstp_msti_rcv_proposal (inst_port);
        }

      inst_port->rcv_proposal = PAL_FALSE; 
      
      if (LAYER2_DEBUG(proto, PROTO))
        zlog_debug (mstpm,"mstp_msti_handle_rootport_transition:"
                    " Received proposal");
    }
  else
    {
      /* ROOT_AGREED */
      if (inst_port->agree == PAL_FALSE)
        {
          inst_port->agree = PAL_TRUE;
          inst_port->cst_port->newInfoMsti = PAL_TRUE;  

#ifdef HAVE_RPVST_PLUS
          inst_port->newInfoSstp = PAL_TRUE;  
#endif /* HAVE_RPVST_PLUS */
        }
    }

  if (inst_port->state != STATE_FORWARDING)
    {
      struct mstp_instance_port *tmp_port;
      if (br_inst->recent_root != inst_port->ifindex)
        {
          for (tmp_port = br_inst->port_list; tmp_port;tmp_port =tmp_port->next)
            {
              /* Set all ports with designated port roles 
               * which are not oper_edge and which are recent roots 
               * to discarding state. */
              if ((tmp_port->role == ROLE_DESIGNATED) && 
                  (!tmp_port->cst_port->oper_edge) &&
                  (l2_is_timer_running (tmp_port->recent_root_timer)))
                {
                   if (LAYER2_DEBUG(proto, PROTO))
                     zlog_debug (mstpm,"mstp_msti_handle_rootport_transition:");

                   mstp_msti_set_port_state (tmp_port, STATE_DISCARDING);

#ifdef HAVE_HA
                   mstp_cal_delete_mstp_inst_forward_delay_timer (tmp_port);
#endif /* HAVE_HA */
                   l2_stop_timer (&tmp_port->forward_delay_timer);

                   tmp_port->forward_delay_timer = 
                   l2_start_timer (mstp_msti_forward_delay_timer_handler,
                                   tmp_port,
                                   mstp_msti_get_fdwhile_time (tmp_port),
                                   mstpm);
#ifdef HAVE_HA
                   mstp_cal_create_mstp_inst_forward_delay_timer (tmp_port);
#endif /* HAVE_HA */

                   if (l2_is_timer_running (tmp_port->recent_root_timer))
                     {
#ifdef HAVE_HA
                       mstp_cal_delete_mstp_inst_rr_timer (tmp_port);
#endif /* HAVE_HA */

                       l2_stop_timer (&tmp_port->recent_root_timer);
                       tmp_port->recent_root_timer = NULL;
                       tmp_port->br->br_inst_all_rr_timer_cnt--;
                     }

                   if (LAYER2_DEBUG(proto, PROTO_DETAIL))
                     zlog_debug (mstpm, 
                                 "mstp_msti_handle_rootport_transition: "
                                 "Designated port %u set to discarding "
                                 "- forward delay %d",
                                 tmp_port->ifindex, br->cist_forward_delay);
                }

            }
        }
     
      /* Transition directly to learning state if version is mstp 
         and no recent root timer is running and the 
         port is not recent backup -IEEE 802.1w-2001 sec 17.23.2 */
      if ((inst_port->cst_port->force_version >= BR_VERSION_RSTP) 
          && (!l2_is_timer_running (inst_port->recent_backup_timer))
          && (inst_port->state != STATE_LEARNING))
        {
          mstp_msti_set_port_state (inst_port, STATE_LEARNING);

#ifdef HAVE_HA
          mstp_cal_delete_mstp_inst_forward_delay_timer (inst_port);
#endif /* HAVE_HA */

          l2_stop_timer (&inst_port->forward_delay_timer);
          inst_port->forward_delay_timer =
                        l2_start_timer (mstp_msti_forward_delay_timer_handler,
                                        inst_port,
                                        mstp_msti_get_fdwhile_time (inst_port),
                                        mstpm);
#ifdef HAVE_HA
          mstp_cal_create_mstp_inst_forward_delay_timer (inst_port);
#endif /* HAVE_HA */
        }

      /* Transition directly to learning state if version is mstp 
         and no recent root timer is running and the 
         port is not recent backup -IEEE 802.1w-2001 sec 17.23.2 */
      if ((inst_port->cst_port->force_version >= BR_VERSION_RSTP) 
          && (!l2_is_timer_running (inst_port->recent_backup_timer))
          && (inst_port->state == STATE_LEARNING))
        {
          mstp_msti_set_port_state (inst_port, STATE_FORWARDING);

#ifdef HAVE_HA
          mstp_cal_delete_mstp_inst_forward_delay_timer (inst_port);
#endif /* HAVE_HA */

          l2_stop_timer (&inst_port->forward_delay_timer);
        }
    }

  br_inst->recent_root = port->ifindex;
#ifdef HAVE_HA
  mstp_cal_modify_mstp_bridge_instance (br_inst);
  mstp_cal_modify_mstp_instance_port (inst_port);
#endif /* HAVE_HA */
   
} /* End Function */



void 
mstp_msti_handle_designatedport_transition (struct mstp_instance_port *inst_port)
{
  struct mstp_port *port = inst_port->cst_port;

  if (LAYER2_DEBUG(proto,PROTO))
    {
      zlog_debug(mstpm,"mstp_msti_handle_designatedport_transition: edge %d "
                 "state %d", port->oper_edge, inst_port->state);
      zlog_debug(mstpm,"mstp_msti_handle_designatedport_transition: ",
                 "proposal %d oper p2p mac %d",
                 inst_port->send_proposal, port->oper_p2p_mac);
    }
  
#ifdef HAVE_HA
  mstp_cal_modify_mstp_instance_port (inst_port);
#endif /* HAVE_HA */

  if ((!port->oper_edge)  &&
      (inst_port->state != STATE_FORWARDING) && (!inst_port->send_proposal))
    {
      /* Section 17.23.3 and figure 17-17, DESIGNATED_PROPOSE */
      inst_port->send_proposal = PAL_TRUE;
      port->newInfoMsti = PAL_TRUE;

#ifdef HAVE_RPVST_PLUS
      inst_port->newInfoSstp = PAL_TRUE;
#endif /* HAVE_RPVST_PLUS */

      if (LAYER2_DEBUG(proto, PROTO))
        zlog_debug (mstpm, "mstp_msti_handle_designatedport_transition: "
                           "Proposing designated for port %u",
                           inst_port->ifindex);
      return;
    }

  /* for msti the version is always going to be > RSTP */
  if (port->oper_edge)
    {
      mstp_msti_set_port_state (inst_port, STATE_FORWARDING);
#ifdef HAVE_HA
      mstp_cal_delete_mstp_inst_forward_delay_timer (inst_port);
#endif /* HAVE_HA */

      l2_stop_timer (&inst_port->forward_delay_timer);
    }
   
  if (port->oper_edge ||
      inst_port->state == STATE_DISCARDING)
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

  if (LAYER2_DEBUG(proto, PROTO))
    zlog_debug (mstpm, "mstp_msti_handle_designated_port_transition: "
                "End mstp designated port transition for port %u",
                inst_port->ifindex);
  
} /* End Function */   
  
static struct mstp_instance_port *
mstp_msti_cmp_ports (struct mstp_instance_port *inst_port,
                     struct mstp_instance_port *candidate_port)
{
  struct mstp_bridge_instance *br_inst;
  int result;

  pal_assert (inst_port);
  br_inst = inst_port->br;

  if (LAYER2_DEBUG(proto,PROTO_DETAIL))
    zlog_debug (mstpm,
                "mstp_msti_cmp_ports: Comparing ports on bridge %s - "
                "curr root port %u with %u", br_inst->bridge->name, 
                inst_port->ifindex,
                candidate_port? candidate_port->ifindex : 0);

  /* Section 9.2.7 802.1t */
  if (! candidate_port)
    return inst_port;

  /* Section 8.6.8.3.1.a */
  result = pal_mem_cmp (&inst_port->msti_root, 
                        &candidate_port->msti_root, MSTP_BRIDGE_ID_LEN);

  if (LAYER2_DEBUG (proto, PROTO_DETAIL))
    {
      print_macaddr ("port->root",&inst_port->msti_root);
      print_macaddr ("candidate_port->root",&candidate_port->msti_root);
      zlog_debug (mstpm,"result of comaprison %d",result);
    }

  if (result < 0)
    return inst_port;
  else if (result > 0)
    return candidate_port;

  /* Section 8.6.8.3.1.b */
  if ((inst_port->internal_root_path_cost + inst_port->msti_path_cost)
      < (candidate_port->internal_root_path_cost + 
         candidate_port->msti_path_cost))
    return inst_port;
  else if ((inst_port->internal_root_path_cost + 
            inst_port->msti_path_cost)
           > (candidate_port->internal_root_path_cost + 
              candidate_port->msti_path_cost))
    return candidate_port;

  /* Section 8.6.8.3.1.c */
  result = pal_mem_cmp (&inst_port->designated_bridge, 
                        &candidate_port->designated_bridge,
                        MSTP_BRIDGE_ID_LEN);
  
  if (result < 0)
    return inst_port;
  else if (result > 0)
    return candidate_port;

  /* Section 8.6.8.3.1.d */

  if (inst_port->designated_port_id < candidate_port->designated_port_id)
    return inst_port;
  else if (inst_port->designated_port_id > candidate_port->designated_port_id)
    return candidate_port;

  /* Section 8.6.8.3.1.e */
  if (inst_port->msti_port_id < candidate_port->msti_port_id)
    return inst_port;

  return candidate_port;
}


static void
mstp_msti_root_port_selection (struct mstp_bridge_instance *br_inst)
{

  struct mstp_instance_port *root_inst_port = NULL;
  struct mstp_instance_port *inst_port = NULL;
  char *br_name = br_inst->bridge->name;

  root_inst_port = NULL;

  if (LAYER2_DEBUG(proto,PROTO_DETAIL))
    zlog_debug (mstpm,"mstp_msti_root_port_selection: bridge %s", br_name);
  
  /* Select the best candidate for root port. */
  for (inst_port = br_inst->port_list; inst_port; inst_port = inst_port->next)
    {
      /* calculate root path priority vectors for all ports which are not 
       * disabled and has info recvd andwhose designated bridge is not equal to
       * desig bridge of bridge's own prio vector */ 

      /* Ports with Restricted Role not considered for root port selection */
      if (inst_port->restricted_role == PAL_TRUE)
          continue;

      if ((inst_port->role != ROLE_DISABLED) &&
          (inst_port->info_type == INFO_TYPE_RECEIVED) &&
          (pal_mem_cmp (&inst_port->designated_bridge,
                        &inst_port->br->msti_bridge_id, MSTP_BRIDGE_ID_LEN)))
        {
          root_inst_port = mstp_msti_cmp_ports (inst_port, root_inst_port);

          if (LAYER2_DEBUG(proto, PROTO_DETAIL))
            zlog_debug (mstpm,"mstp_msti_root_port_selection: "
                        "comparing port %u - current best root %u", 
                        inst_port->ifindex, root_inst_port->ifindex);
        }
    }


  if (LAYER2_DEBUG(proto, PROTO))
    zlog_debug (mstpm, "mstp_msti_root_port_selection: Port (%d)"
                " selected as root", root_inst_port ?
                                     root_inst_port->ifindex : 0);
  
  if (root_inst_port)
    inst_port = root_inst_port;
  
  /* Root priority vector is the better of the bridge priority vector and the 
     best rooth path prioirty vector. */
  
  if (inst_port && (pal_mem_cmp (&inst_port->msti_root,
                                 &br_inst->msti_bridge_id,MSTP_BRIDGE_ID_LEN) < 0))
    {
      pal_mem_cpy (&br_inst->msti_designated_root,
                   &inst_port->msti_root, MSTP_BRIDGE_ID_LEN);
      pal_mem_cpy (&br_inst->msti_designated_bridge,
                   &inst_port->designated_bridge, MSTP_BRIDGE_ID_LEN);
      br_inst->internal_root_path_cost = 
        inst_port->internal_root_path_cost + 
        inst_port->msti_path_cost;
      br_inst->msti_root_port_id = inst_port->msti_port_id;
      br_inst->msti_root_port_ifindex = inst_port->ifindex;
      br_inst->root_inst_port = root_inst_port;

      /* populate the timer values for root port */
      br_inst->hop_count = inst_port->hop_count;
    }
  else
    {
      pal_mem_cpy (&br_inst->msti_designated_root, &br_inst->msti_bridge_id, 
                   MSTP_BRIDGE_ID_LEN);
      pal_mem_cpy (&br_inst->msti_designated_bridge, &br_inst->msti_bridge_id, 
                   MSTP_BRIDGE_ID_LEN);
      br_inst->internal_root_path_cost = 0;
      br_inst->msti_root_port_id = 0;
      br_inst->msti_root_port_ifindex = 0;
      br_inst->root_inst_port = NULL;
      
      /* populate the timer values for root port */
      br_inst->hop_count = br_inst->bridge->bridge_max_hops;
    }

#ifdef HAVE_HA
  mstp_cal_modify_mstp_bridge_instance (br_inst);
#endif /* HAVE_HA */
}


/* Implement Topology Change State Machine 
 * PROPOGATING state 
 * IEEE 802.1w-2001 sec 17.25 */

void
mstp_msti_propogate_topology_change (struct mstp_instance_port *this_port_inst)
{
  struct mstp_instance_port *inst_port;
  struct mstp_bridge_instance *br_inst = this_port_inst->br;
  struct nsm_msg_bridge msg;
  struct nsm_msg_bridge_if msgif;
  int index;
  struct mstp_port *port;

  if (LAYER2_DEBUG(proto,PROTO_DETAIL))
    zlog_debug (mstpm, "mstp_msti_propogate_topology_change: bridge %s", 
                br_inst->bridge->name);

  pal_mem_set(&msgif, 0, sizeof(struct nsm_msg_bridge_if));
  pal_strncpy(msgif.bridge_name, br_inst->bridge->name, NSM_BRIDGE_NAMSIZ);

  /* Update Statistics for Instance/Vlan */
  br_inst->num_topo_changes++;
  br_inst->total_num_topo_changes++;
  br_inst->time_last_topo_change = pal_time_current (0);
  
  for (inst_port = br_inst->port_list; inst_port; inst_port = inst_port->next)
    {
      port = inst_port->cst_port;
      if (inst_port == this_port_inst)
        continue;

      if ( inst_port->msti_tc_state == TC_ACTIVE
          && ! port->oper_edge)
        {
          if (!l2_is_timer_running (inst_port->tc_timer))
            {
              if (LAYER2_DEBUG(proto,PROTO_DETAIL))
                zlog_debug (mstpm,"mstp_msti_propogate_topology_change "
                           "starting tc timer for port %u ",inst_port->ifindex);
              inst_port->tc_timer = l2_start_timer( 
                                              mstp_msti_tc_timer_handler, 
                                              inst_port, 
                                              mstp_msti_get_tc_time (inst_port),
                                              mstpm);
              port->newInfoMsti = PAL_TRUE;
#ifdef HAVE_RPVST_PLUS
              inst_port->newInfoSstp = PAL_TRUE;
#endif /*HAVE_RPVST_PLUS */ 
#ifdef HAVE_HA
              mstp_cal_modify_mstp_port (port);
              mstp_cal_create_mstp_inst_tc_timer (inst_port);
#endif /* HAVE_HA */
            } 

          if (port->oper_edge == PAL_FALSE)
            {
              if (LAYER2_DEBUG(proto,PROTO_DETAIL))
                zlog_debug (mstpm,"calling flush fdb for port %u  instance %d",
                            port->ifindex,inst_port->instance_id);
              /* Send message to nsm to flush fdb. */
              msgif.num++;
            } 
        }
    }
  if (mstpm->nc == NULL)
    return;

  if (msgif.num > 0)
    {
      /* Allocate ifindex array. */
      msgif.ifindex = XCALLOC(MTYPE_TMP, msgif.num * sizeof(u_int32_t));
      if (! msgif.ifindex)
        return;
      index = 0;
      /* send the flush fdb */
      for(inst_port = br_inst->port_list; inst_port; inst_port = inst_port->next)
        {
          port = inst_port->cst_port;
          if (inst_port != this_port_inst)
            {
               if (inst_port->msti_tc_state == TC_ACTIVE
                   && ! port->oper_edge)
                {
                  msgif.ifindex[index++] = port->ifindex;
                }
            }
        }
      if ((mstpm->nc != NULL) &&
          (!CHECK_FLAG (mstpm->nc->nsm_server_flags, NSM_SERVER_SHUTDOWN)))
           nsm_client_send_bridge_if_msg(mstpm->nc, &msgif,
                                          NSM_MSG_BRIDGE_PORT_FLUSH_FDB);
      
      /* Free ifindex array. */
      if (msgif.ifindex)
        XFREE (MTYPE_TMP, msgif.ifindex);
    }
      
  /* Send topology change message to NSM. Nsm can report it to protocols like
     igmp snoop. Igmp snoop should generate membership query for the bridge. */
  pal_mem_set(&msg, 0, sizeof(struct nsm_msg_bridge));
  pal_strncpy (msg.bridge_name, br_inst->bridge->name, NSM_BRIDGE_NAMSIZ);

  if ((mstpm->nc != NULL) &&
      (!CHECK_FLAG (mstpm->nc->nsm_server_flags, NSM_SERVER_SHUTDOWN))) 
     nsm_client_send_bridge_msg(mstpm->nc, &msg, NSM_MSG_BRIDGE_TCN);
}



void
mstp_msti_process_message (struct mstp_instance_port *inst_port, 
                           struct mstp_instance_bpdu *inst_bpdu)
{
  s_int32_t message_age;
  struct mstp_port *port = NULL;
#ifdef HAVE_RPVST_PLUS
  struct mstp_instance_port *inst_port_tmp = NULL;
  struct mstp_bridge_instance *br_inst = NULL;
#endif /* HAVE_RPVST_PLUS */
  int rcvd_msg;
  
  if (inst_port == NULL)
    return;

#ifdef HAVE_RPVST_PLUS
  if (inst_port->br)
    br_inst = inst_port->br;
#endif /* HAVE_RPVST_PLUS */
  
  if (inst_port->cst_port)
    port = inst_port->cst_port;

  if (port == NULL)
    return;

#ifdef HAVE_RPVST_PLUS
  if (br_inst == NULL)
    return;
#endif /* HAVE_RPVST_PLUS */

  if ( !inst_port->br)
    {
      /* No point continuing if the bridge pointer is not valid */
      return;
    }

  rcvd_msg = mstp_handle_rcvd_msti_bpdu (inst_port,inst_bpdu);

  inst_port->br->msti_mastered = port->oper_p2p_mac && 
                                 inst_bpdu->msti_master;

  inst_port->br->msti_mastered |= inst_bpdu->msti_master;

#ifdef HAVE_HA
  mstp_cal_modify_mstp_bridge_instance (inst_port->br);
  mstp_cal_modify_mstp_instance_port (inst_port);
#endif /* HAVE_HA */

  if (LAYER2_DEBUG(proto, PROTO_DETAIL))
    zlog_info (mstpm, "mstp_msti_process_message: bridge %s port (%u)" 
               "for instance (%d)", 
               port->br->name, port->ifindex, inst_port->instance_id);

  /* rcv_msg = rcvBPDU() */

  if (rcvd_msg == MSG_TYPE_SUPERIOR_DESIGNATED)
    {
      if (LAYER2_DEBUG(proto, PROTO))
        zlog_info (mstpm,"mstp_msti_process_message: Recvd superior message");

      if (inst_port->role == ROLE_DISABLED)
        return;

#ifdef HAVE_HA
      mstp_cal_delete_mstp_inst_message_age_timer (inst_port);
#endif /* HAVE_HA */

      l2_stop_timer (&inst_port->message_age_timer);
      inst_port->agree = inst_port->agree
                        && better_or_same_info_msti_rcvd (inst_port, inst_bpdu);
      mstp_msti_record_priority_vector (inst_port, inst_bpdu);

      message_age = mstp_msti_update_message_age (inst_port ,inst_bpdu);

      inst_port->message_age_timer = 
        l2_start_timer (mstp_msti_message_age_timer_handler,
                        inst_port, message_age, mstpm);

#ifdef HAVE_HA
      mstp_cal_create_mstp_inst_message_age_timer (inst_port);
#endif /* HAVE_HA */

      /* 802.1 Q 2005 13.26.10, 802.1 D 2004 17.21.11 */
      if (inst_bpdu->msti_proposal)
        inst_port->rcv_proposal = inst_bpdu->msti_proposal;

      inst_port->send_proposal = PAL_FALSE;
      inst_port->info_type = INFO_TYPE_RECEIVED;
      inst_port->reselect = PAL_TRUE;
      inst_port->selected = PAL_FALSE;
      inst_port->agreed = PAL_FALSE;

      if (message_age != 0)
        mstp_msti_port_role_selection (inst_port->br);
    }
  else if (rcvd_msg == MSG_TYPE_REPEATED_DESIGNATED)
    {
      inst_port->similar_bpdu_cnt++;
      if (LAYER2_DEBUG(proto, PROTO))
        zlog_info (mstpm,"mstp_msti_process_message: Recvd repeated message");

#ifdef HAVE_HA
      mstp_cal_delete_mstp_inst_message_age_timer (inst_port);
#endif /* HAVE_HA */

      l2_stop_timer (&inst_port->message_age_timer);  

      /* 802.1 Q 2005 13.26.10, 802.1 D 2004 17.21.11 */
      if (inst_bpdu->msti_proposal)
        inst_port->rcv_proposal = inst_bpdu->msti_proposal;

      if (inst_port->rcv_proposal
          && inst_port->role != ROLE_BACKUP)
        {
          mstp_msti_rcv_proposal (inst_port);
          inst_port->rcv_proposal = PAL_FALSE;
          mstp_msti_send_proposal (inst_port);
        }

      message_age = mstp_msti_update_message_age (inst_port, inst_bpdu);

      inst_port->message_age_timer = 
                      l2_start_timer (mstp_msti_message_age_timer_handler,
                      inst_port, message_age, mstpm);
#ifdef HAVE_HA
      mstp_cal_create_mstp_inst_message_age_timer (inst_port);
#endif /* HAVE_HA */
    }
  else if (rcvd_msg == MSG_TYPE_INFERIOR_ALTERNATE
           && inst_port->agreed)
    {
      if (LAYER2_DEBUG(proto, PROTO))
        zlog_info (mstpm,"mstp_msti_process_message: Recvd confirmed message");

#ifdef HAVE_HA
      mstp_cal_delete_mstp_inst_forward_delay_timer (inst_port);
#endif /* HAVE_HA */
      l2_stop_timer (&inst_port->forward_delay_timer);

      /* We have received confirmation from the adjacent bridge
         to rapidly transition to forwarding state */
      inst_port->send_proposal = PAL_FALSE;
      inst_port->agreed = PAL_TRUE;
      if (inst_port && inst_port->state != STATE_FORWARDING) 
        {
          if (inst_port->state != STATE_LEARNING)
            mstp_msti_set_port_state (inst_port, STATE_LEARNING);
          if (inst_port->state == STATE_LEARNING)
            mstp_msti_set_port_state (inst_port, STATE_FORWARDING);
          inst_port->cst_port->newInfoMsti = PAL_TRUE;
        }

#ifdef HAVE_RPVST_PLUS
      inst_port->newInfoSstp = PAL_TRUE;
#endif /* HAVE_RPVST_PLUS */

#ifdef HAVE_HA
      mstp_cal_modify_mstp_port (inst_port->cst_port);
#endif /* HAVE_HA */

      inst_port->send_proposal = PAL_FALSE;
    }
  else if (rcvd_msg == MSG_TYPE_INFERIOR_DESIGNATED
           && inst_port->disputed
         /*  && inst_port->role == ROLE_DESIGNATED */
           && ! inst_port->cst_port->oper_edge)
    {
      /* Section 17.21.10 Standard is incorrect.
       * agreed should be reset and proposal should
       * be set.
       */

    int  designated_br_cmp = 0;

    designated_br_cmp  = pal_mem_cmp (&inst_bpdu->bridge_id,
        &inst_port->designated_bridge,
        MSTP_BRIDGE_ID_LEN);

    if (inst_port->role != ROLE_DESIGNATED &&
        designated_br_cmp > 0 &&
        inst_port->cst_port )
      /* inst_port->cst_port->port_enabled) */
      {
        inst_port->reselect = PAL_TRUE;
        inst_port->selected = PAL_FALSE;
        if (inst_port->role == ROLE_DISABLED)
          inst_port->info_type = INFO_TYPE_DISABLED;
        else  
          {
#ifdef HAVE_RPVST_PLUS
             if (br_inst->bridge->type == NSM_BRIDGE_TYPE_RPVST_PLUS)
               for (inst_port_tmp = br_inst->port_list; inst_port_tmp;
                                   inst_port_tmp = inst_port_tmp->next)
                 {
                   inst_port_tmp->info_type = INFO_TYPE_MINE;
                 }
             else
 #endif /* HAVE_RPVST_PLUS */
               inst_port->info_type = INFO_TYPE_MINE;
         }

        mstp_msti_port_role_selection (inst_port->br);

      }
    else if(inst_port->role == ROLE_DESIGNATED )
      {
                 /* Section 17.21.10 Standard is incorrect.
                  *        * agreed should be reset and proposal should
                  *               * be set.
                  *                      */
      inst_port->agreed = PAL_FALSE;
      inst_port->send_proposal = PAL_TRUE;
      mstp_msti_set_port_state (inst_port, STATE_DISCARDING);
      inst_port->disputed = PAL_FALSE;
#ifdef HAVE_HA
      mstp_cal_delete_mstp_inst_forward_delay_timer (inst_port);
#endif /* HAVE_HA */

      l2_stop_timer (&inst_port->forward_delay_timer);
      inst_port->forward_delay_timer = l2_start_timer (
                                          mstp_msti_forward_delay_timer_handler,
                                          inst_port,
                                          mstp_msti_get_fdwhile_time (inst_port),
                                          mstpm);
#ifdef HAVE_HA
      mstp_cal_create_mstp_inst_forward_delay_timer (inst_port);
#endif /* HAVE_HA */

      inst_port->cst_port->newInfoMsti = PAL_TRUE;

#ifdef HAVE_RPVST_PLUS  
      inst_port->newInfoSstp = PAL_TRUE;
#endif /* HAVE_RPVST_PLUS */
    }
   }
  else 
    {    
      if (LAYER2_DEBUG(proto, PROTO))
        zlog_info (mstpm,"mstp_msti_process_message: Recvd inferior message");
    }

#ifdef HAVE_HA
  mstp_cal_modify_mstp_instance_port (inst_port);
#endif /* HAVE_HA */
}

/* Return 0 if msg == port prio vect 
 * return -1 if msg < port prio vect
 * return +1 if msg > port prio vect */
static inline s_int32_t
mstp_cmp_msti_msg_prio_and_port_prio (struct mstp_instance_bpdu *inst_bpdu,
                                      struct mstp_instance_port *inst_port)
{
  int root_cmp, bridge_cmp;
  s_int32_t desig_port;
  int rpc;
  pal_assert (inst_bpdu);
  pal_assert (inst_port);

  root_cmp = pal_mem_cmp (&inst_bpdu->msti_reg_root,
                          &inst_port->msti_root,MSTP_BRIDGE_ID_LEN);
  if (root_cmp != 0)
    return root_cmp;

  rpc = inst_port->internal_root_path_cost - inst_bpdu->msti_internal_rpc;
  if (rpc > 0)
    return 1;

  if (rpc < 0)
    return -1;

  bridge_cmp = pal_mem_cmp (&inst_bpdu->bridge_id, 
                            &inst_port->designated_bridge, MSTP_BRIDGE_ID_LEN);
      
  if (bridge_cmp != 0)
    return bridge_cmp;
  
  desig_port = inst_bpdu->msti_port_id - inst_port->designated_port_id ;

  if (desig_port > 0)
    return 1;

  if (desig_port < 0)
    return -1;
  
  /* return 0 */
  return desig_port;
  
}

