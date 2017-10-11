/* Copyright (C) 2003  All Rights Reserved.

MSTP Protocol State Machine

*/

#include "pal.h"
#include "l2lib.h"
#include "l2_timer.h"

#include "mstp_config.h"
#include "mstp_types.h"
#include "mstpd.h"
#include "mstp_bpdu.h"
#include "mstp_cist_proto.h"
#include "mstp_msti_proto.h"
#include "mstp_transmit.h"
#include "mstp_port.h"
#include "mstp_bridge.h"
#include "mstp_api.h"

#include "nsm_message.h"
#include "nsm_client.h"

#ifdef HAVE_SMI
#include "lib_fm_api.h"
#include "smi_message.h"
#include "smi_mstp_fm.h"
#endif

#ifdef HAVE_HA
#include "mstp_cal.h"
#endif /* HAVE_HA */

#define ROUND(A)         ((A) + 0.5)
static int mstp_cist_assign_role (struct mstp_port *port);
static void mstp_cist_root_port_selection (struct mstp_bridge *br);
static struct mstp_port * mstp_cist_cmp_ports (struct mstp_port *port,
                                               struct mstp_port *candidate_port);

extern int mstp_cist_forward_delay_timer_handler (struct thread *);
extern int mstp_cist_tc_timer_handler (struct thread *);


static void print_macaddr (char *title, struct bridge_id *addr)
{

  zlog_debug (mstpm, "%s -%.2x:%.2x ::  %.2x:%.2x:%.2x:%.2x:%.2x:%.2x",
              title,
              addr->prio[0],
              addr->prio[1],
              addr->addr[0],
              addr->addr[1],
              addr->addr[2],
              addr->addr[3],
              addr->addr[4],
              addr->addr[5]);
}


/* TODO */
bool_t
better_or_same_info_cist_mine (struct mstp_port *port, struct mstp_bridge *br)
{
  int root_cmp;
  int reg_root_cmp;
  int designated_br_cmp;

  root_cmp = pal_mem_cmp (&br->cist_designated_root, &port->cist_root,
                          MSTP_BRIDGE_ID_LEN);

  reg_root_cmp = pal_mem_cmp (&br->cist_reg_root, &port->cist_reg_root,
                              MSTP_BRIDGE_ID_LEN);

  designated_br_cmp = pal_mem_cmp (&br->cist_bridge_id,
                                   &port->cist_designated_bridge,
                                   MSTP_BRIDGE_ID_LEN);


  if (root_cmp < 0
      || (root_cmp == 0
          && br->external_root_path_cost < port->cist_external_rpc)
      || (root_cmp == 0
          && br->external_root_path_cost == port->cist_external_rpc
          && reg_root_cmp < 0)
      || (root_cmp == 0
          && br->external_root_path_cost == port->cist_external_rpc
          && reg_root_cmp == 0
          && br->internal_root_path_cost < port->cist_internal_rpc)
      || (root_cmp == 0
          && br->external_root_path_cost == port->cist_external_rpc
          && reg_root_cmp == 0
          && br->internal_root_path_cost == port->cist_internal_rpc
          && designated_br_cmp <= 0))
    {
      return PAL_TRUE;
    }

  return PAL_FALSE;
}

bool_t
better_or_same_info_cist_rcvd (struct mstp_port *port, struct mstp_bpdu *bpdu)
{
  int root_cmp;
  int reg_root_cmp;
  int designated_br_cmp;

  root_cmp = pal_mem_cmp (&bpdu->cist_root, &port->cist_root,
                          MSTP_BRIDGE_ID_LEN);

  reg_root_cmp = pal_mem_cmp (&bpdu->cist_reg_root, &port->cist_reg_root,
                              MSTP_BRIDGE_ID_LEN);

  designated_br_cmp = pal_mem_cmp (&bpdu->cist_bridge_id,
                                   &port->cist_designated_bridge,
                                   MSTP_BRIDGE_ID_LEN);

  if (root_cmp < 0
      || (root_cmp == 0
          && bpdu->external_rpc < port->cist_external_rpc)
      || (root_cmp == 0
          && bpdu->external_rpc == port->cist_external_rpc
          && reg_root_cmp < 0)
      || (root_cmp == 0
          && bpdu->external_rpc == port->cist_external_rpc
          && reg_root_cmp == 0
          && bpdu->internal_rpc < port->cist_internal_rpc)
      || (root_cmp == 0
          && bpdu->external_rpc == port->cist_external_rpc
          && reg_root_cmp == 0
          && bpdu->internal_rpc == port->cist_internal_rpc
          && designated_br_cmp < 0)
      || (root_cmp == 0
          && bpdu->external_rpc == port->cist_external_rpc
          && reg_root_cmp == 0
          && bpdu->internal_rpc == port->cist_internal_rpc
          && designated_br_cmp == 0
          && bpdu->cist_port_id <= port->cist_designated_port_id))
    {
      return PAL_TRUE;
    }

  return PAL_FALSE;
}


/* IEEE 802.1Q 2005 Section 13.26.22 */
static inline s_int32_t
mstp_cist_update_message_age (struct mstp_port* port,
                              struct mstp_bpdu *bpdu)
{
  s_int32_t effective_age = 0;
  s_int32_t incr;
  s_int32_t ret_age;
  s_int32_t cist_max_age = port->cist_max_age/L2_TIMER_SCALE_FACT;
  s_int32_t cist_message_age = port->cist_message_age/L2_TIMER_SCALE_FACT;
  s_int32_t cist_hello_time = port->cist_hello_time/L2_TIMER_SCALE_FACT;

  /* Not sure if we need bpdu as an argument */

  if (!port->info_internal)
    {
      incr = ( (cist_max_age / 16) > 1) ?  cist_max_age / 16 : 1 ;

      effective_age = cist_message_age + incr;

      if (effective_age > cist_max_age)
        {
          ret_age = 0;
        }
      else
        {
          if (IS_BRIDGE_STP (port->br))
            {
              ret_age = (port->cist_max_age - bpdu->message_age)
                / L2_TIMER_SCALE_FACT;
              port->cist_message_age = bpdu->message_age;
            }
          else
            {
               ret_age = cist_hello_time * 3;
            }
        }

      if (LAYER2_DEBUG(proto, PROTO_DETAIL))
        {
          zlog_info (mstpm, "mstp_cist_update_message_age: "
                     "port->max age %d ret_age %d incr %d effective_age %d",
                     port->cist_max_age, ret_age, incr, effective_age);
        }

      /* The info is external . so set the value of hops to max hops */
      port->hop_count = port->br->bridge_max_hops;
    }
  else
    {
      /* set message age to one received and hop_count */
      port->cist_message_age = bpdu->message_age;

      port->hop_count = bpdu->hop_count - 1 ;
      if ((port->cist_message_age > port->cist_max_age ) ||
          (port->hop_count <= 0 ))
        ret_age = 0;
      else
        {
          ret_age = 3 * cist_hello_time ;
        }

      if (LAYER2_DEBUG(proto, PROTO_DETAIL))
        {
          zlog_info (mstpm, "mstp_cist_update_message_age: "
                     "port->max age %d ret_age %d hop_count %d",
                     port->cist_max_age, ret_age, port->hop_count);
        }

    }

  return SECS_TO_TICS(ret_age);
}



/* IEEE 802.1w-2001 sec 17.19.8 */
int
mstp_handle_rcvd_cist_bpdu (struct mstp_port *port, struct mstp_bpdu *bpdu)
{
  struct mstp_port *alt_port;
  struct mstp_bridge *br;
  int designated_br_cmp;
  int reg_root_cmp;
  int root_cmp;

  br = port->br;
  root_cmp = pal_mem_cmp (&bpdu->cist_root,
                          &port->cist_root, MSTP_BRIDGE_ID_LEN);

  designated_br_cmp  = pal_mem_cmp (&bpdu->cist_bridge_id,
                                        &port->cist_designated_bridge, MSTP_BRIDGE_ID_LEN);

  /* For External messages, if the received message is superior
   * in mstp_cist_record_priority_vector the port's bridge id s copied as the
   * regional root. If these two are equal it means that port is on region
   * boundary. For external messages, bpdu->cist_reg_root and
   * port->cist_reg_root should not be compared as the regional root is
   * significant within the region only otherwise the check will always fail.
   */

   reg_root_cmp = pal_mem_cmp (&bpdu->cist_reg_root,
                               &port->cist_rcvd_reg_root, MSTP_BRIDGE_ID_LEN);

  /* set msti mstered for each inst_port */

  if (LAYER2_DEBUG(proto, PROTO_DETAIL))
    {
      zlog_debug (mstpm,"mstp_handle_rcvd_cist_bpdu: bridge(%s) port(%u) "
                  "external root path cost(%u) internal rpc (%u)",
                  port->br->name, port->ifindex,
                  port->cist_external_rpc, port->cist_internal_rpc);

      zlog_debug (mstpm,"mstp_handle_rcvd_cist_bpdu: designated port(%d) "
                  "port p2p(%d)", port->cist_designated_port_id, port->oper_p2p_mac);

      zlog_debug (mstpm,"mstp_handle_rcvd_cist_bpdu: bpdu type (%d) "
                  "bpdu role(%d)", bpdu->type, bpdu->cist_port_role);

      zlog_debug (mstpm,"mstp_handle_rcvd_cist_bpdu: bpdu external rpc  (%d) "
                  "bdpu internal rpc (%d)", bpdu->external_rpc, bpdu->internal_rpc);

      zlog_debug (mstpm,"mstp_handle_rcvd_cist_bpdu: bdpu port id(%d)"
                  "agreement(%d)", bpdu->cist_port_id, bpdu->cist_agreement);

      zlog_debug (mstpm,"mstp_handle_rcvd_cist_bpdu: root_cmp (%d)"
                  " desig br cmp (%d)", root_cmp, designated_br_cmp);

    }


  /* If the role is unknown we effectively treat it as a config bpdu */
  if (((bpdu->type == BPDU_TYPE_RST) &&
       ((bpdu->cist_port_role == ROLE_DESIGNATED) ||
        (bpdu->cist_port_role == ROLE_UNKNOWN))) ||
      (bpdu->type == BPDU_TYPE_CONFIG))
    {

      if (LAYER2_DEBUG(proto, PROTO_DETAIL))
        zlog_debug (mstpm,"mstp_handle_rcvd_cist_bpdu: "
                    "Processing Config BPDU on port(%u) for bridge %s",
                    port->ifindex, port->br->name);

      if ((port->oper_p2p_mac == PAL_TRUE)&&
          (bpdu->cist_agreement == PAL_TRUE))
        port->agreed = PAL_TRUE;
      else
        port->agreed = PAL_FALSE;


      /* See section 17.4.2.2 */
      if (root_cmp < 0)
        {
          /* Strictly better */
          if (LAYER2_DEBUG(proto, PROTO_DETAIL))
            zlog_debug(mstpm,"mstp_handle_rcvd_cist_bpdu: "
                       "Received root id is better on port(%u) ",
                       port->ifindex);

          return MSG_TYPE_SUPERIOR_DESIGNATED;
        }
      else if (root_cmp == 0)
        {

          if (bpdu->external_rpc < port->cist_external_rpc)
            {
              /* Strictly better */
              if (LAYER2_DEBUG(proto, PROTO_DETAIL))
                zlog_debug(mstpm,"mstp_handle_rcvd_cist_bpdu: "
                           "Received path cost is better on port(%u) "
                           "bpdu-rpc (%u)  port-rpc (%u)",
                           port->ifindex, bpdu->external_rpc, port->cist_external_rpc);
              return MSG_TYPE_SUPERIOR_DESIGNATED;
            }
          else if (bpdu->external_rpc == port->cist_external_rpc)
            {
              if (reg_root_cmp < 0)
                {
                  return MSG_TYPE_SUPERIOR_DESIGNATED;
                }
              else if (reg_root_cmp == 0)
                {
                  if (bpdu->internal_rpc < port->cist_internal_rpc)
                    {
                      return MSG_TYPE_SUPERIOR_DESIGNATED;
                    }
                  else if (bpdu->internal_rpc == port->cist_internal_rpc)
                    {
                      if (designated_br_cmp < 0)
                        {
                          /* Strictly better */
                          if (LAYER2_DEBUG(proto, PROTO_DETAIL))
                            zlog_debug(mstpm,"mstp_handle_rcvd_cist_bpdu:"
                                       " Received desig br id is better on"
                                       "port(%u) ", port->ifindex);
                          return MSG_TYPE_SUPERIOR_DESIGNATED;
                        }
                      else if (designated_br_cmp == 0)
                        {
                          if (bpdu->cist_port_id < port->cist_designated_port_id)
                            {
                              /* Strictly better */
                              if (LAYER2_DEBUG(proto, PROTO_DETAIL))
                                zlog_debug(mstpm,"mstp_handle_rcvd_cist_bpdu:"
                                           " Received desig port id is worse on"
                                           "port(%u) ", port->ifindex);
                              return MSG_TYPE_SUPERIOR_DESIGNATED;
                            }
                        } /* cmp_val == 0 */

                    }/* int_rpc == port->cist_int_rpc */

                }/* reg_root_cmp == 0 */

            } /*external rpc == rootpathcost */

        }/* root_cmp == 0 */

      if (( !designated_br_cmp) &&
          (bpdu->cist_port_id == port->cist_designated_port_id))
        {
          /* If the designated port and designated bridge are the same
             and the message priority vector as a whole or any of the msg
             timers is different return superior message -
             Section 17.19.8 pp. 55 */

          /* Added a check to see if the internal rpc differs in case of
           * internal message since for external messages the ports
           * internal rpc will be zero.
           */

         /* According to the IEEE 802.1Q 2003 Section 13.15 Message is
          * superior when hop count in the BPDU is different from that
          * of the port. But according to 13.26.23 updtRcvdInfoWhileCist
          * the port hop count is decremented by 1 when the information
          * is internal and set to MaxHops when the information is external.
          * This is contradictory since if we satisfy both the sections,
          * the BPDU hop count will not be same as the port hop count and
          * hence we will be returning superior BPDU always even though
          * the BPDU is conveying the same information. Hence added this
          * work around that we compare BPDU hop count with the port hop
          * count incremented by one when the information is internal.
          * For external messages we ignore the HOP count since, the
          * for external messages the message age is significant and
          * port's hop count will be reset to the MaxHops.
          */

          /* Like the same way how it is done for internal messages, for
           * external messages, the bpdu->message_age should be compared with
           * the increment of port->cist_message_age as it is incremented once
           * if it it superior designated information
           */
           s_int32_t port_cist_message_age = port->cist_message_age/L2_TIMER_SCALE_FACT;
           s_int32_t bpdu_cist_message_age = bpdu->message_age/L2_TIMER_SCALE_FACT;

          if ((root_cmp != 0)
              || (bpdu->external_rpc != port->cist_external_rpc)
              || ( (port->info_internal) &&
                   (bpdu->internal_rpc != port->cist_internal_rpc) )
              || (bpdu_cist_message_age != (port_cist_message_age))
              || (bpdu->max_age != port->cist_max_age)
              || (bpdu->fwd_delay != port->cist_fwd_delay)
              || (bpdu->hello_time != port->cist_hello_time)
              || ((port->info_internal) &&
                  (bpdu->hop_count != (port->hop_count + 1))))
            {
              /* if timer values are diff msg is superior */
              if (LAYER2_DEBUG(proto, PROTO_DETAIL))
                zlog_debug(mstpm,"mstp_handle_rcvd_cist_bpdu: "
                           "one of received time variables is different");

              if ((port->cist_role == ROLE_ALTERNATE)&&(br->topology_type == TOPOLOGY_RING))
                {
                  mstp_cist_set_port_role (port, ROLE_DESIGNATED);

                  mstp_cist_set_port_state (port, STATE_LEARNING);
                  mstp_cist_set_port_state (port, STATE_FORWARDING);

                  return MSG_TYPE_INFERIOR_DESIGNATED;
                }
              else if((port->cist_role == ROLE_ROOTPORT)&&(br->topology_type == TOPOLOGY_RING))
                {
                   struct mstp_port *tmp_port;

                   if (port->br != NULL
                       && (alt_port = port->br->alternate_port) != NULL)
                     {
                        alt_port->cist_tc_state = TC_ACTIVE;
                        mstp_cist_set_port_role (alt_port, ROLE_ROOTPORT);
                        mstp_cist_set_ring_port_state (alt_port);
                        mstp_cist_set_port_state (alt_port, STATE_FORWARDING);
                        mstp_cist_ring_handle_topology_change (alt_port);
                     }

                   BRIDGE_CIST_PORTLIST_LOOP(tmp_port, br)
                   {
                    if (tmp_port->cist_role == ROLE_DESIGNATED)
                      {
                        mstp_port_send (tmp_port, bpdu->buf, bpdu->len);

                      }
                   }
                }

              return MSG_TYPE_SUPERIOR_DESIGNATED;
            }
          else
            {
              if (LAYER2_DEBUG(proto, PROTO_DETAIL))
                zlog_debug(mstpm,"mstp_handle_rcvd_cist_bpdu:"
                           " received BPDU is repeated message");
              return MSG_TYPE_REPEATED_DESIGNATED;
            }
        }
    } /* bpdu is rst or cbpdu */


  if (LAYER2_DEBUG(proto, PROTO_DETAIL))
    zlog_debug(mstpm,"mstp_handle_rcvd_cist_bpdu: received BPDU has"
               " inferior information");

  if (bpdu->type == BPDU_TYPE_RST
      && port->oper_p2p_mac
      && (bpdu->cist_port_role == ROLE_ROOTPORT
          || bpdu->cist_port_role == ROLE_ALTERNATE
          || bpdu->cist_port_role == ROLE_BACKUP))
    {

      if (bpdu->cist_agreement)
        {
          port->agreed = PAL_TRUE;
        }
      else
        {
          port->agreed = PAL_FALSE;
        }

      return MSG_TYPE_INFERIOR_ALTERNATE;

    }

  if (((bpdu->type == BPDU_TYPE_RST) &&
       ((bpdu->cist_port_role == ROLE_DESIGNATED)
        || (bpdu->cist_port_role == ROLE_UNKNOWN)))
      || (bpdu->type == BPDU_TYPE_CONFIG))
    {

       if (bpdu->type == BPDU_TYPE_RST
           && bpdu->cist_learning)
         port->disputed = PAL_TRUE;

       return MSG_TYPE_INFERIOR_DESIGNATED;
    }

  return MSG_TYPE_OTHER_MSG;
}

int
mstp_cist_port_role_selection (struct mstp_bridge *br)
{

  /* Update roles selection for each port
     for ports which have different selected roles and port roles
     run the port role transition state machine. */

  struct mstp_port *port;
  pal_assert (br);

#ifdef HAVE_HA
  mstp_cal_modify_mstp_bridge (br);
#endif /* HAVE_HA */

  /* Recompute the roles  if any port asks us to i.e. sets reselect */
  if (LAYER2_DEBUG(proto, PROTO))
    zlog_debug (mstpm, "mstp_cist_port_role_selection: bridge %s", br->name);

  mstp_cist_clear_reselect_bridge (br);
  mstp_cist_root_port_selection (br);

  /* Set the port roles */

  for (port = br->port_list; port; port = port->next)
    {
      mstp_cist_assign_role (port);

      if (port->cist_selected_role != port->cist_role)
        mstp_cist_set_port_role (port, port->cist_selected_role);

#ifdef HAVE_HA
      /* Update the port data once for the port for all port_role_selection */
      mstp_cal_modify_mstp_port (port);
#endif /* HAVE_HA */
    }

  mstp_cist_set_selected_bridge (br);

  /* Receive Proposal and set the ReRoot for recent
   * Roots.
   */

  for (port = br->port_list; port; port = port->next)
    {
      switch (port->cist_selected_role)
        {
        case ROLE_ROOTPORT:
          mstp_cist_handle_rootport_transition (port);
          break;

        case ROLE_ALTERNATE:

          /* If the CIST port role is alternate and the port
           * information is derived from a bridge in another
           * region set the port role of all the MSTI instances
           * for this port to alternare Section 13.26.26 point (h)
           */

          if (!port->info_internal)
            mstp_msti_set_port_role_all (port, ROLE_ALTERNATE);

        case ROLE_BACKUP:


          if (IS_BRIDGE_STP (br))
            {
              port->config_bpdu_pending = PAL_FALSE;
              port->tc_ack = PAL_FALSE;

              if (port->cist_state == STATE_LEARNING
                  || port->cist_state == STATE_FORWARDING)
              {
                /* Section 8.6.14.2.3 Case 3 */
                br->num_topo_changes++;
                br->time_last_topo_change = pal_time_sys_current (NULL);
                br->total_num_topo_changes++;
                stp_topology_change_detection (port->br);
              }
            }

          /* Figure 13-19. If the port role is not root or
           * desiganted, Put the TC state to INACTIVE.
           */

          port->cist_tc_state = TC_INACTIVE;
          mstp_cist_set_port_state (port, STATE_DISCARDING);

          /* 802.1 D 2004 Section 17.29.4 */

          if (port->rcv_proposal)
            {
              /* ALTERNATE_AGREED */
              if (port->agree)
                {
                  port->newInfoCist = PAL_TRUE;
                }
              else /* ALTERNATE_PROPOSED */
                {
                  mstp_cist_rcv_proposal (port);
                }

              port->rcv_proposal = PAL_FALSE;
            }

          break;

        default:
          break;
        } /* Switch statement */
    }

  for (port = br->port_list; port; port = port->next)
    {
      switch (port->cist_selected_role)
        {
        case ROLE_DESIGNATED:
          mstp_cist_handle_designatedport_transition (port);
          break;

        default:
          break;
        } /* Switch statement */
    }

  if (LAYER2_DEBUG(proto,PROTO_DETAIL))
    zlog_debug (mstpm, "mstp_cist_port_role_selection: End");

  return RESULT_OK;
}/* End Function */

int
mstp_cist_port_config_update (struct mstp_port *port)
{
  struct mstp_bridge *br = port->br;
  /* copy designated priority and designated times to
   * port priority and times vectors */
  if (LAYER2_DEBUG(proto,PROTO_DETAIL))
    zlog_debug (mstpm,
                "mstp_cist_port_config_update: bridge %s port(%u)",
                br->name, port->ifindex);

  if (!port->updtInfo)
    return RESULT_ERROR;

  port->agreed = port->agreed && better_or_same_info_cist_mine (port, br);
  pal_mem_cpy(&port->cist_root,&br->cist_designated_root, MSTP_BRIDGE_ID_LEN);
  pal_mem_cpy(&port->cist_reg_root, &br->cist_reg_root, MSTP_BRIDGE_ID_LEN);
  pal_mem_cpy(&port->cist_rcvd_reg_root,&br->cist_reg_root,MSTP_BRIDGE_ID_LEN);
  pal_mem_cpy (&port->cist_designated_bridge,
               &br->cist_bridge_id, MSTP_BRIDGE_ID_LEN);
  port->cist_external_rpc = br->external_root_path_cost;
  port->cist_internal_rpc = br->internal_root_path_cost;
  port->cist_designated_port_id = port->cist_port_id;

  port->cist_message_age = br->cist_message_age;
  port->cist_max_age = br->cist_max_age;
  port->cist_fwd_delay = br->cist_forward_delay;

  if (IS_BRIDGE_MSTP (br) || IS_BRIDGE_RPVST_PLUS (br))
    {
      port->cist_hello_time = port->hello_time;
    }
  else if (IS_BRIDGE_RSTP (br))
    {
      /* 802.1 D 17.21.25 Point e */
      port->cist_hello_time = br->bridge_hello_time;
    }
  else
    {
      port->cist_hello_time = br->cist_hello_time;
    }

  port->hop_count = br->hop_count;

  /* updt_info is set to false, agreed sync is set to false
   * proposed and send_proposal are set to false */
  port->send_proposal = PAL_FALSE;
  port->agree = PAL_FALSE;
  port->cist_info_type = INFO_TYPE_MINE;
  port->updtInfo = PAL_FALSE;

  /*TODO
    port->changed_master */
  /*  port->agreed = ((port->agreed) &&
      (better_or_same_info_cist ()) && (!port->changed_master)) ;
  */

  if (LAYER2_DEBUG(proto,PROTO_DETAIL))
    {
      print_macaddr ("mstp_cist_port_config_update: "
                     "CIST root bridge id ", &port->cist_root);
      print_macaddr ("mstp_cist_port_config_update: CIST regional root id ",
                     &port->cist_reg_root);
      print_macaddr ("mstp_cist_port_config_update: designated bridge id ",
                     &port->cist_designated_bridge);
      zlog_debug (mstpm, "mstp_cist_port_config_update: external root path "
                  "cost(%u) internal root path cost (%u)",
                  port->cist_external_rpc, port->cist_internal_rpc);
      zlog_debug (mstpm, "mstp_cist_port_config_update: designated port(%d)",
                  port->cist_designated_port_id);
      zlog_debug (mstpm, "mstp_cist_port_config_update: message age(%d) "
                  "max age(%d) hop_count (%d)",
                  port->cist_message_age/L2_TIMER_SCALE_FACT,
                  port->cist_max_age/L2_TIMER_SCALE_FACT, port->hop_count);
      zlog_debug (mstpm, "mstp_cist_port_config_update: fwd delay(%d) "
                  "hello time(%d)", port->cist_fwd_delay/L2_TIMER_SCALE_FACT,
                  port->cist_hello_time/L2_TIMER_SCALE_FACT);
    }

  port->newInfoCist = PAL_TRUE;
  return RESULT_OK;
}



int
mstp_cist_rcv_proposal (struct mstp_port *curr_port)
{

  struct mstp_bridge *br;
  struct mstp_port *port;
  struct mstp_instance_port *inst_port = NULL;
  pal_assert (curr_port);
  br = curr_port->br;
  pal_assert (br);


  /* Set all ports with designated port roles
   * which are not oper_edge to discarding state.
   * agreed = true;
   */

  if (LAYER2_DEBUG(proto, PROTO_DETAIL))
    zlog_debug (mstpm, "mstp_cist_rcv_proposal: MSTP received proposal for "
                "port (%u) bridge %s ", curr_port->ifindex, br->name );

 if (! BRIDGE_TYPE_CVLAN_COMPONENT (br)
     || curr_port->port_type == MSTP_VLAN_PORT_MODE_CE)
   {
     BRIDGE_CIST_PORTLIST_LOOP(port, br)
       {
         if (port != curr_port &&
             port->cist_role == ROLE_DESIGNATED &&
             ! port->oper_edge &&
             ! curr_port->agreed)
           {
             mstp_cist_set_port_state (port, STATE_DISCARDING);
             port->send_proposal = PAL_TRUE;

             if (LAYER2_DEBUG(proto, TIMER))
               zlog_debug (mstpm,"mstp_cist_rcv_proposal: start fwd delay timer "
                           "after receiving proposal - %d", br->cist_forward_delay);

             if (l2_is_timer_running (port->forward_delay_timer))
               {
#ifdef HAVE_HA
                 mstp_cal_delete_mstp_forward_delay_timer (port);
#endif /* HAVE_HA */
                 l2_stop_timer (&port->forward_delay_timer);
               }

             port->forward_delay_timer =
              l2_start_timer (mstp_forward_delay_timer_handler,
                              port, mstp_cist_get_fdwhile_time (port), mstpm);
#ifdef HAVE_HA
             mstp_cal_create_mstp_forward_delay_timer (port);
#endif /* HAVE_HA */
           }
        }
    }
  else
    {
       port = mstp_find_ce_br_port (br, MSTP_VLAN_NULL_VID);
       if (port == NULL)
          return RESULT_ERROR;

       if (port != curr_port &&
           port->cist_role == ROLE_DESIGNATED &&
           ! port->oper_edge &&
           ! curr_port->agreed)
         {
            mstp_cist_set_port_state (port, STATE_DISCARDING);
            port->send_proposal = PAL_TRUE;

            if (LAYER2_DEBUG(proto, TIMER))
              zlog_debug (mstpm,"mstp_cist_rcv_proposal: start fwd delay timer "
                         "after receiving proposal - %d", br->cist_forward_delay);

            if (l2_is_timer_running (port->forward_delay_timer))
              {
#ifdef HAVE_HA
                mstp_cal_delete_mstp_forward_delay_timer (port);
#endif /* HAVE_HA */
                l2_stop_timer (&port->forward_delay_timer);
              }

            port->forward_delay_timer =
             l2_start_timer (mstp_forward_delay_timer_handler,
                             port, mstp_cist_get_fdwhile_time (port), mstpm);
#ifdef HAVE_HA
             mstp_cal_create_mstp_forward_delay_timer (port);
#endif /* HAVE_HA */
         }
    }

  curr_port->agree = PAL_TRUE;

  if (!curr_port->info_internal)
    {
      inst_port = curr_port->instance_list;
      while (inst_port)
        {
          inst_port->agree = PAL_TRUE;
#ifdef HAVE_HA
          mstp_cal_modify_mstp_instance_port (inst_port);
#endif /* HAVE_HA */
          inst_port = inst_port->next_inst;
        }
    }

  if (LAYER2_DEBUG(proto, PROTO_DETAIL))
    zlog_debug (mstpm,"mstp_cist_rcv_proposal: End  ");

  curr_port->newInfoCist = PAL_TRUE;
  return RESULT_OK;

}

void
mstp_cist_send_proposal (struct mstp_port *curr_port)
{
  struct mstp_bridge *br;
  struct mstp_port *port;
  pal_assert (curr_port);
  br = curr_port->br;
  pal_assert (br);

  if (LAYER2_DEBUG(proto, PROTO_DETAIL))
    zlog_debug (mstpm, "mstp_cist_send_proposal: MSTP received repeated message from "
        "port (%u) bridge %s ", curr_port->ifindex, br->name );

  BRIDGE_CIST_PORTLIST_LOOP(port, br)
    {
      if ((port != curr_port) &&
          (port->cist_role == ROLE_DESIGNATED) && (!port->oper_edge))
        {
          port->send_proposal = PAL_TRUE;
          port->newInfoCist = PAL_TRUE;
        }
    }
  return;
}


/* IEEE 802.1w-2001 sec 17.21
   Implements functionality in SUPERIOR state of Port Information
   State Machine */
void
mstp_cist_record_priority_vector (struct mstp_port *port,
                                  struct mstp_bpdu *bpdu)
{

   /* Check if port is a part of the bridge. */  
  if (port->br == NULL)
   return;

  if (LAYER2_DEBUG(proto, PROTO_DETAIL))
    zlog_debug (mstpm, "mstp_cist_record_priority_vector: "
                "MSTP designated record priority vector for port (%u) bridge %s",
                port->ifindex, port->br->name);
  /* copy port priority (vector) */
  pal_mem_cpy (&port->cist_root, &bpdu->cist_root, MSTP_BRIDGE_ID_LEN);
  if (port->info_internal)
    {
      pal_mem_cpy (&port->cist_reg_root, &bpdu->cist_reg_root,
                   MSTP_BRIDGE_ID_LEN);
      pal_mem_cpy (&port->cist_rcvd_reg_root, &bpdu->cist_reg_root,
                                              MSTP_BRIDGE_ID_LEN);

      port->cist_internal_rpc = bpdu->internal_rpc;
    }
  else
    {
      /* If the message is recvd from another region set the bridge
       * id as regional root and the internal path cost as 0
       * Sec 13.10 IEEE 802.1s */
      port->cist_internal_rpc = 0;
      pal_mem_cpy (&port->cist_reg_root, &port->br->cist_bridge_id,
                   MSTP_BRIDGE_ID_LEN);
      pal_mem_cpy (&port->cist_rcvd_reg_root, &bpdu->cist_reg_root,
                                               MSTP_BRIDGE_ID_LEN);

    }

  port->cist_external_rpc = bpdu->external_rpc;
  pal_mem_cpy (&port->cist_designated_bridge,
               &bpdu->cist_bridge_id,MSTP_BRIDGE_ID_LEN);

  port->cist_designated_port_id = bpdu->cist_port_id;

  /* port times Variable */
  /* Appendix B.3.3.2 Always increment the message age by atleast 1 */

  /* Validation of the Bpdu's Max Age */
  if ((bpdu->max_age > (MSTP_MAX_BRIDGE_MAX_AGE * L2_TIMER_SCALE_FACT)) ||
      (bpdu->max_age < (MSTP_MIN_BRIDGE_MAX_AGE * L2_TIMER_SCALE_FACT)))
     bpdu->max_age = port->br->bridge_max_age; 
       
  /* Validation of the HELLO Time */
  if ((bpdu->hello_time < (MSTP_MIN_BRIDGE_HELLO_TIME * L2_TIMER_SCALE_FACT)) ||
      (bpdu->hello_time > (MSTP_MAX_BRIDGE_HELLO_TIME * L2_TIMER_SCALE_FACT)))
    bpdu->hello_time = port->hello_time;
  
  port->cist_message_age = bpdu->message_age;
  port->cist_max_age = bpdu->max_age;
  port->cist_fwd_delay = bpdu->fwd_delay;

  port->cist_hello_time = bpdu->hello_time;

  port->hop_count = bpdu->hop_count;

  if (LAYER2_DEBUG(proto, PROTO_DETAIL))
    {
      print_macaddr ("mstp_cist_record_priority_vector: cist root bridge id ",
                     &port->cist_root);
      print_macaddr ("mstp_record_priority_vector: cist regional root id ",
                     &port->cist_root);
      print_macaddr ("mstp_record_priority_vector: cist designated bridge id ",
                     &port->cist_designated_bridge);
      zlog_debug (mstpm, "mstp_record_priority_vector: cist external  root"
                  " path cost(%u)", port->cist_external_rpc);
      zlog_debug (mstpm, "mstp_record_priority_vector: cist internal  root"
                  " path cost(%u)", port->cist_internal_rpc);
      zlog_debug (mstpm, "mstp_record_priority_vector: designated port(%d)",
                  port->cist_designated_port_id);
      zlog_debug (mstpm, "mstp_record_priority_vector: message age(%d) "
                  "max age(%d)", port->cist_message_age/L2_TIMER_SCALE_FACT,
                  port->cist_max_age/L2_TIMER_SCALE_FACT);
      zlog_debug (mstpm, "mstp_record_priority_vector: fwd delay(%d) "
                  "hello time(%d) hop count (%d)",
                  port->cist_fwd_delay/L2_TIMER_SCALE_FACT,
                  port->cist_hello_time/L2_TIMER_SCALE_FACT,port->hop_count);
    }
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

int
dpv_ppv_cmp (struct mstp_port *port)
{
  struct mstp_bridge *br = port->br ;
  int root_cmp, reg_root_cmp, bridge_id_cmp;
  u_int32_t int_cost = 0, ext_cost = 0;

  if (port->info_internal)
    {
      int_cost = br->internal_root_path_cost;
      ext_cost = br->external_root_path_cost;
    }
  else
    ext_cost = br->internal_root_path_cost;

  root_cmp = pal_mem_cmp(&port->cist_root,
                         &br->cist_designated_root, MSTP_BRIDGE_ID_LEN);
  reg_root_cmp = pal_mem_cmp(&port->cist_reg_root,
                             &br->cist_reg_root, MSTP_BRIDGE_ID_LEN);
  bridge_id_cmp = pal_mem_cmp (&port->cist_designated_bridge,
                               &br->cist_bridge_id, MSTP_BRIDGE_ID_LEN);

  if (LAYER2_DEBUG(proto, PROTO))
    {
      if (port->cist_designated_port_id <= port->cist_port_id)
        zlog_debug (mstpm,"designated port id < cist port id  -ok");
      else
        zlog_debug (mstpm,"incorrect port id ");

      if (port->cist_external_rpc <= ext_cost)
        zlog_debug (mstpm,"ext cost  -ok ");
      else
        zlog_debug (mstpm,"incorrect ext cost ");

      if (port->cist_internal_rpc <= int_cost)
        zlog_debug (mstpm,"int cost  -ok ");
      else
        zlog_debug (mstpm,"incorrect internal cost %d %d ",
                    port->cist_internal_rpc,int_cost);
      if ((root_cmp <= 0) && (reg_root_cmp <= 0))
        zlog_debug (mstpm,"roots  -ok ");
      else
        zlog_debug (mstpm,"incorrect roots %d %d",root_cmp,reg_root_cmp);
      if (bridge_id_cmp <= 0)
        zlog_debug (mstpm,"designated bridge id < = 0 -ok ");
      else
        zlog_debug (mstpm,"incorrect desig br id %d ",bridge_id_cmp);
    }

  if (root_cmp < 0)
    return root_cmp;

  /* if ext cost is greater that means dpv is better and we return + value
     else we return -ve meaning ppv is better if both same we continue */
  if ((root_cmp == 0) && (port->cist_external_rpc < ext_cost)){
    return (port->cist_external_rpc - ext_cost);
  }
  if ((root_cmp == 0) && (port->cist_external_rpc == ext_cost) && (bridge_id_cmp < 0)){
    return bridge_id_cmp;
  }

  if ((root_cmp == 0) && (port->cist_external_rpc == ext_cost) && (bridge_id_cmp == 0)
      && (port->cist_designated_port_id <= port->cist_port_id)){
    return port->cist_designated_port_id - port->cist_port_id;
  }

  if (reg_root_cmp < 0){
    return reg_root_cmp;
  }

  if ((reg_root_cmp == 0) && (port->cist_internal_rpc < int_cost)){
    return (port->cist_internal_rpc - int_cost);
  }

  if ((reg_root_cmp == 0) && (port->cist_external_rpc == ext_cost) && (bridge_id_cmp < 0)){
    return bridge_id_cmp;
  }

  if ((reg_root_cmp == 0) && (port->cist_external_rpc == ext_cost) && (bridge_id_cmp == 0)
      &&(port->cist_designated_port_id <= port->cist_port_id)){
    return port->cist_designated_port_id - port->cist_port_id;
  }

  /* port role will be designated */
  return 1;
}

static int
mstp_cist_assign_role (struct mstp_port *port)
{
  struct mstp_bridge * br = port->br;
  int bridge_id_cmp;

  if (LAYER2_DEBUG(proto, PROTO_DETAIL))
    zlog_debug (mstpm,
                "mstp_cist_assign_role: bridge %s port %u - port info type %d (%s)",
                br->name, port->ifindex, port->cist_info_type,
                info_type_str (port->cist_info_type));

  switch (port->cist_info_type)
    {
    case INFO_TYPE_DISABLED:
      {
        /* 802.1w Section 17.19.21.g */
        port->cist_selected_role = ROLE_DISABLED;
        if (LAYER2_DEBUG(proto, PROTO))
          zlog_debug (mstpm,"mstp_cist_assign_role: bridge %s port %u -> "
                      "disabled", br->name, port->ifindex);
        break;
      }
    case INFO_TYPE_AGED:
      {
        /* 802.1w Section 17.19.21.h */
        port->cist_selected_role = ROLE_DESIGNATED;
        port->updtInfo = PAL_TRUE;
        mstp_cist_port_config_update (port);

        /* set update info */
        if (LAYER2_DEBUG(proto, PROTO))
          zlog_debug (mstpm,"mstp_cist_assign_role: bridge %s port %u ->"
                      " designated", br->name, port->ifindex);
        break;
      }
    case INFO_TYPE_MINE:
      {
        /* 802.1w Section 17.19.21.i */
        port->cist_selected_role = ROLE_DESIGNATED;
        /* If port's priority vector differs from designated
         * priority vector or ports timer params are
         * diff from those of root port. set update info */
        if ((pal_mem_cmp (&port->cist_root,
                          &br->cist_designated_root, MSTP_BRIDGE_ID_LEN))
            || (pal_mem_cmp (&port->cist_reg_root,
                             &br->cist_reg_root, MSTP_BRIDGE_ID_LEN))
            || (pal_mem_cmp (&port->cist_designated_bridge,
                             &br->cist_designated_bridge, MSTP_BRIDGE_ID_LEN))
            || (port->cist_port_id != port->cist_designated_port_id)
            || (port->cist_external_rpc != br->external_root_path_cost)
            || (port->cist_internal_rpc != br->internal_root_path_cost)
            || ((! IS_BRIDGE_MSTP (br) || !IS_BRIDGE_RPVST_PLUS(br))
                && port->cist_hello_time != br->cist_hello_time)
            || ((IS_BRIDGE_MSTP (br) || IS_BRIDGE_RPVST_PLUS(br))
                && port->cist_hello_time != port->hello_time)
            || (port->cist_max_age != br->cist_max_age)
            || (port->cist_fwd_delay != br->cist_forward_delay)
            || (port->hop_count != br->hop_count));
          {
            port->updtInfo = PAL_TRUE;
            mstp_cist_port_config_update (port);
          }

        if (LAYER2_DEBUG(proto, PROTO))
          zlog_debug (mstpm,"mstp_cist_assign_role: bridge %s port %u "
                      "-> designated", br->name, port->ifindex);
        break;
      }
    case INFO_TYPE_RECEIVED:
      {
        int root_cmp, reg_root_cmp,desig_bridge_cmp;
        u_int32_t int_cost_incr ;
        u_int32_t ext_cost_incr ;


        root_cmp = pal_mem_cmp(&port->cist_root, &br->cist_designated_root,
                               MSTP_BRIDGE_ID_LEN);
        reg_root_cmp = pal_mem_cmp(&port->cist_reg_root, &br->cist_reg_root,
                                   MSTP_BRIDGE_ID_LEN);
        desig_bridge_cmp = pal_mem_cmp(&port->cist_designated_bridge, &br->cist_designated_bridge, MSTP_BRIDGE_ID_LEN);


        if (port->info_internal)
          {
            int_cost_incr = port->cist_path_cost;
            ext_cost_incr = 0;
          }
        else
          {
            int_cost_incr = 0;
            ext_cost_incr = port->cist_path_cost;

          }

        if (LAYER2_DEBUG(proto, PROTO))
          {
            zlog_debug (mstpm,
                        "mstp_cist_assign_role: cist port id %x designated port id %x",
                        port->cist_port_id, port->cist_designated_port_id);
            zlog_debug (mstpm, "mstp_cist_assign_role: port external root pc %d"
                        "internal rpc %d  int pc incr %d ext pc incr %d",
                        port->cist_external_rpc,port->cist_internal_rpc,
                        int_cost_incr,ext_cost_incr);
            print_macaddr ("mstp_cist_assign_role: portdesig bridge id",
                           &port->cist_designated_bridge);
            print_macaddr ("mstp_cist_assign_role: port root ", &port->cist_root);
            print_macaddr ("mstp_cist_assign_role: cist bridge id ",
                           &br->cist_bridge_id);
            print_macaddr ("mstp_cist_assign_role: bridge cist desig bridge",
                           &br->cist_designated_bridge);
            print_macaddr ("mstp_cist_assign_role: bridge cist root ",
                           &br->cist_designated_root);
            print_macaddr ("mstp_cist_assign_role: bridge cist reg root ",
                           &br->cist_reg_root);
            zlog_debug (mstpm, "mstp_cist_assign_role: bridge external root pc %u"
                        " root port id %x", br->external_root_path_cost,
                        br->cist_root_port_id);
            zlog_debug (mstpm, "mstp_cist_assign_role: br internal rpc %d ",
                        br->internal_root_path_cost);
            zlog_debug (mstpm, "root cmp values root_cmp %d , reg_root_cmp %d "
                        "desig_br_cmp %d ",root_cmp, reg_root_cmp, desig_bridge_cmp);
            /* TODO
               mstp_log_cist_port_pv (port);
               mstp_log_cist_desig_pv (port);
            */
          }
        /* 802.1w Section 17.19.21.j */
        /* If root priority vector is derived from this port.
           This is the root port. */

        if  ((br->cist_root_port_id == port->cist_port_id)
             && (br->external_root_path_cost ==
                 port->cist_external_rpc + ext_cost_incr)
             && (br->internal_root_path_cost ==
                 port->cist_internal_rpc + int_cost_incr)
             && (root_cmp == 0) && (reg_root_cmp == 0)
             && (desig_bridge_cmp== 0))
          {
            port->updtInfo = PAL_FALSE;
            port->cist_selected_role = ROLE_ROOTPORT;
            if (LAYER2_DEBUG(proto, PROTO))
              zlog_debug (mstpm,"mstp_cist_assign_role: bridge %s port %u "
                          "-> root", br->name, port->ifindex);
            /* update info is reset */
          }
        else
          {
            int int_cost = 0, ext_cost = 0;
            if (port->info_internal)
              {
                int_cost = br->internal_root_path_cost;
                ext_cost = br->external_root_path_cost;
              }
            else
              ext_cost = br->external_root_path_cost;

            /* The root priority vector is not derived from
               the port priority vector. */
            bridge_id_cmp = pal_mem_cmp (&port->cist_designated_bridge,
                                         &br->cist_bridge_id, MSTP_BRIDGE_ID_LEN);

            /* 802.1w Section 17.19.21.k */
            /* If the root priority vector is not now derived from it,
               the designated priority vector is not higher than
               the port priority vector and the designated bridge and
               designated port components of the port priority
               vector do not reflect another port on this bridge,
               role is alternate port. */

            if (LAYER2_DEBUG(proto, PROTO))
              {

                if (port->cist_designated_port_id <= port->cist_port_id)
                  zlog_debug (mstpm,"designated port id < cist port id  -ok");
                else
                  zlog_debug (mstpm,"incorrect port id ");
                if (port->cist_external_rpc <= ext_cost)
                  zlog_debug (mstpm,"ext cost  -ok ");
                else
                  zlog_debug (mstpm,"incorrect ext cost ");
                if (port->cist_internal_rpc <= int_cost)
                  zlog_debug (mstpm,"int cost  -ok ");
                else
                  zlog_debug (mstpm,"incorrect internal cost %d %d ",
                              port->cist_internal_rpc,int_cost);
                if ((root_cmp <= 0) && (reg_root_cmp <= 0))
                  zlog_debug (mstpm,"roots  -ok ");
                else
                  zlog_debug (mstpm,"incorrect roots %d %d",root_cmp,reg_root_cmp);
                if (bridge_id_cmp <= 0)
                  zlog_debug (mstpm,"designated bridge id < = 0 -ok ");
                else
                  zlog_debug (mstpm,"incorrect desig br id %d ",bridge_id_cmp);
              }

            reg_root_cmp = pal_mem_cmp(&port->cist_rcvd_reg_root, &br->cist_reg_root,
                                       MSTP_BRIDGE_ID_LEN);

            /* compare designated priority vector to port priority vector */
            if (((root_cmp < 0)
                 || ((root_cmp == 0) && (port->cist_external_rpc < ext_cost)))
                 || (((root_cmp == 0) && (port->cist_external_rpc == ext_cost) &&
                     ((reg_root_cmp < 0)
                  || ((reg_root_cmp == 0) && (port->cist_internal_rpc < int_cost))
                  || ((reg_root_cmp == 0) && (port->cist_internal_rpc == int_cost)
                      && (bridge_id_cmp < 0))
                  || ((reg_root_cmp == 0) && (port->cist_internal_rpc == int_cost)
                      && (bridge_id_cmp == 0) && (port->cist_designated_port_id <= port->cist_port_id))))))
              {
                /* The designated priority vector is not higher than
                   than the port priority vector. */

                /* Does the port priority vector reflect another port
                   on this bridge? */
                if (bridge_id_cmp)
                  {
                    port->updtInfo = PAL_FALSE;

                    if (port->port_type == MSTP_VLAN_PORT_MODE_PE
                        && br->root_port
                        && br->root_port->port_type == MSTP_VLAN_PORT_MODE_PE)
                      {
                         port->cist_selected_role = ROLE_ROOTPORT;
                      }
                    else
                      {
                        port->cist_selected_role = ROLE_ALTERNATE;
                        if (LAYER2_DEBUG(proto, PROTO))
                          zlog_debug (mstpm,"mstp_cist_assign_role: bridge %s "
                                      "port %u svid %u -> alternate", br->name,
                                       port->ifindex, port->svid);
                      }
                  }
                else if ((!bridge_id_cmp)
                         && (port->cist_port_id != port->cist_designated_port_id))
                  {
                    /* 802.1w Section 17.19.21.k */
                    /* ELSE role is backup */
                    port->cist_selected_role = ROLE_BACKUP;
                    port->updtInfo = PAL_FALSE;
                    if (LAYER2_DEBUG(proto, PROTO))
                      zlog_debug (mstpm,"mstp_assign_role: bridge %s port %u"
                                  " -> backup", br->name, port->ifindex);
                  }
              }
            else
              {
                port->cist_selected_role = ROLE_DESIGNATED;
                port->updtInfo = PAL_TRUE;
                if (LAYER2_DEBUG(proto, PROTO))
                  zlog_debug (mstpm,"mstp_assign_role: bridge %s port %u "
                              " -> designated (default)", br->name, port->ifindex);
                mstp_cist_port_config_update (port);

              }
          }
      } /* Case info_type RECEIVED */
    } /* Select info_type */

  if (LAYER2_DEBUG(proto,PROTO_DETAIL))
    zlog_debug (mstpm, "mstp_assign_role: End role assignment for port %u",
                port->ifindex);
  return RESULT_OK;
}


void
mstp_cist_handle_rootport_transition (struct mstp_port *port)
{
  struct mstp_bridge *br = port->br;

#ifdef HAVE_HA
  mstp_cal_modify_mstp_port (port);
#endif /* HAVE_HA */

  if (LAYER2_DEBUG(proto, PROTO))
    zlog_debug (mstpm,
                "mstp_cist_handle_rootport_transition: bridge %s port %u"
                "state %d rcv_proposal %d force version %d",
                port->br->name, port->ifindex,
                port->cist_state,port->rcv_proposal,port->force_version);

  if (IS_BRIDGE_STP (br))
    {
      port->config_bpdu_pending = PAL_FALSE;
      port->tc_ack = PAL_FALSE;
      return;
    }

  if (port->rcv_proposal)
    {

      /* ROOT_AGREED */
      if (port->agree)
        {
          port->newInfoCist = PAL_TRUE;
        }
      else /* ROOT_PROPOSED */
        {
          mstp_cist_rcv_proposal (port);
        }

      port->rcv_proposal = PAL_FALSE;

      if (LAYER2_DEBUG(proto, PROTO))
        zlog_debug (mstpm,"mstp_cist_handle_rootport_transition:"
                    " Received proposal");
    }
  else
    {
      /* ROOT_AGREED */
      if (port->agree == PAL_FALSE)
        {
          port->agree = PAL_TRUE;
          port->newInfoCist = PAL_TRUE;
        }
    }

  if (port->cist_state != STATE_FORWARDING)
    {
      struct mstp_port *tmp_port;
      if (br->recent_root != port->ifindex)
        {
          if (! BRIDGE_TYPE_CVLAN_COMPONENT (br)
               || port->port_type == MSTP_VLAN_PORT_MODE_CE)
            {
              for (tmp_port = br->port_list; tmp_port;
                   tmp_port = tmp_port->next)
                 {
                   /* Set all ports with designated port roles
                    * which are not oper_edge and which are recent roots
                    * to discarding state. */

                   if ((tmp_port->cist_role == ROLE_DESIGNATED) &&
                       (! tmp_port->oper_edge) &&
                       (l2_is_timer_running (tmp_port->recent_root_timer)))
                     {
                       mstp_cist_set_port_state (tmp_port, STATE_DISCARDING);

                       if (l2_is_timer_running (port->forward_delay_timer))
                         {
#ifdef HAVE_HA
                           mstp_cal_delete_mstp_forward_delay_timer (tmp_port);
#endif /* HAVE_HA */
                           l2_stop_timer (&tmp_port->forward_delay_timer);
                         }

                       tmp_port->forward_delay_timer =
                         l2_start_timer (mstp_forward_delay_timer_handler,
                                         tmp_port,
                                         mstp_cist_get_fdwhile_time (tmp_port),
                                         mstpm);
#ifdef HAVE_HA
                       mstp_cal_create_mstp_forward_delay_timer (tmp_port);
#endif /* HAVE_HA */

                       if (l2_is_timer_running (tmp_port->recent_root_timer))
                         {
#ifdef HAVE_HA
                           mstp_cal_delete_mstp_rr_timer (tmp_port);
#endif /* HAVE_HA */
                           l2_stop_timer (&tmp_port->recent_root_timer);
                           tmp_port->recent_root_timer = NULL;
                           tmp_port->br->br_all_rr_timer_cnt--;
                         }

                       tmp_port->agreed = PAL_FALSE;

                       if (LAYER2_DEBUG(proto, PROTO_DETAIL))
                         zlog_debug (mstpm,
                                     "mstp_cist_handle_rootport_transition"
                                     "Designated port %u set to discarding "
                                     "- forward delay %d",
                                     tmp_port->ifindex,
                                     tmp_port->br->cist_forward_delay);
                     }
                 }
            }
          else
            {
              tmp_port = mstp_find_ce_br_port (br, MSTP_VLAN_NULL_VID);

              if (tmp_port && (tmp_port->cist_role == ROLE_DESIGNATED) &&
                  (! tmp_port->oper_edge) &&
                  (l2_is_timer_running (tmp_port->recent_root_timer)))
                {

                  mstp_cist_set_port_state (tmp_port, STATE_DISCARDING);

                  if (l2_is_timer_running (port->forward_delay_timer))
                    {
#ifdef HAVE_HA
                      mstp_cal_delete_mstp_forward_delay_timer (tmp_port);
#endif /* HAVE_HA */

                      l2_stop_timer (&tmp_port->forward_delay_timer);
                    }

                  tmp_port->forward_delay_timer =
                         l2_start_timer (mstp_forward_delay_timer_handler,
                                         tmp_port,
                                         mstp_cist_get_fdwhile_time (tmp_port),
                                         mstpm);
#ifdef HAVE_HA
                   mstp_cal_create_mstp_forward_delay_timer (tmp_port);
#endif /* HAVE_HA */

                  if (l2_is_timer_running (tmp_port->recent_root_timer))
                    {
#ifdef HAVE_HA
                      mstp_cal_delete_mstp_rr_timer (tmp_port);
#endif /* HAVE_HA */
                      l2_stop_timer (&tmp_port->recent_root_timer);
                      tmp_port->recent_root_timer = NULL;
                      tmp_port->br->br_all_rr_timer_cnt--;
                    }

                  tmp_port->agreed = PAL_FALSE;

                  if (LAYER2_DEBUG(proto, PROTO_DETAIL))
                    zlog_debug (mstpm, "mstp_cist_handle_rootport_transition"
                                "Designated port %u set to discarding "
                                "- forward delay %d",
                                tmp_port->ifindex,
                                tmp_port->br->cist_forward_delay);
                }
            }

        }

      /* Transition directly to learning state if version is mstp
         and no recent root timer is running and the
         port is not recent backup -IEEE 802.1w-2001 sec 17.23.2 */

      if ((port->force_version >= BR_VERSION_RSTP)
          && (!l2_is_timer_running (port->recent_backup_timer))
          && (port->cist_state != STATE_LEARNING))
        {
          mstp_cist_set_port_state (port, STATE_LEARNING);
          if (l2_is_timer_running (port->forward_delay_timer))
            {
#ifdef HAVE_HA
              mstp_cal_delete_mstp_forward_delay_timer (port);
#endif /* HAVE_HA */

              l2_stop_timer (&port->forward_delay_timer);
            }
          port->forward_delay_timer =
                   l2_start_timer (mstp_forward_delay_timer_handler,
                                   port, mstp_cist_get_fdwhile_time (port),
                                   mstpm);
#ifdef HAVE_HA
          mstp_cal_create_mstp_forward_delay_timer (port);
#endif /* HAVE_HA */
        }

      /* Transition directly to forwarding state if version is mstp
         and no recent root timer is running and the
         port is not recent backup -IEEE 802.1w-2001 sec 17.23.2 */

      if ((port->force_version >= BR_VERSION_RSTP)
          && (!l2_is_timer_running (port->recent_backup_timer))
          && (port->cist_state == STATE_LEARNING))
        {
          mstp_cist_set_port_state (port, STATE_FORWARDING);
          if (l2_is_timer_running(port->forward_delay_timer))
            {
#ifdef HAVE_HA
              mstp_cal_delete_mstp_forward_delay_timer (port);
#endif /* HAVE_HA */
              l2_stop_timer (&port->forward_delay_timer);
            }
        }
    }

    br->recent_root = port->ifindex;

    /* If the CIST port role is root port and the port
     * information is derived from a bridge in another
     * region (this bridge is the regional root) set
     * the port role of all the MSTI instances for this
     * port to Master Section 13.26.26 point (g)
     */
    if (!port->info_internal)
      {
         mstp_msti_set_port_role_all (port, ROLE_MASTERPORT);
      }

} /* End Function */

void
mstp_cist_handle_designatedport_transition (struct mstp_port *port)
{

  if (LAYER2_DEBUG(proto,PROTO))
    {
      zlog_debug(mstpm,"mstp_cist_handle_designatedport_transition: edge %d "
                 "state %d", port->oper_edge, port->cist_state);
      zlog_debug(mstpm,"mstp_cist_handle_designatedport_transition: "
                 "proposal %d oper p2p mac %d",
                 port->send_proposal, port->oper_p2p_mac);
    }

  if ( (!port->oper_edge)  && (!IS_BRIDGE_STP (port->br)) &&
       (port->cist_state != STATE_FORWARDING) && (!port->send_proposal))
    {
      if (l2_is_timer_running (port->edge_delay_timer))
        {
#ifdef HAVE_HA
          mstp_cal_delete_mstp_edge_delay_timer (port);
#endif /* HAVE_HA */

          /* Section 17.23.3 and figure 17-17, DESIGNATED_PROPOSE */
          l2_stop_timer (&port->edge_delay_timer);
        }

      port->edge_delay_timer = l2_start_timer (mstp_edge_delay_timer_handler,
                                  port, MSTP_MIGRATE_TIME * L2_TIMER_SCALE_FACT,
                                  mstpm);

#ifdef HAVE_HA
      mstp_cal_create_mstp_edge_delay_timer (port);
#endif /* HAVE_HA */

      port->send_proposal = PAL_TRUE;
      port->newInfoCist = PAL_TRUE;

      if (LAYER2_DEBUG(proto, PROTO))
        zlog_debug (mstpm, "mstp_cist_handle_designatedport_transition: "
                    "Proposing designated for port %u", port->ifindex);
      return;
    }

  if ( ((port->admin_edge && IS_BRIDGE_STP (port->br))
         || ((port->oper_edge) && (port->force_version >= BR_VERSION_RSTP)))
      && (port->oper_rootguard == PAL_FALSE))
    {
      mstp_cist_set_port_state (port, STATE_FORWARDING);
      if (l2_is_timer_running(port->forward_delay_timer))
        {
#ifdef HAVE_HA
          mstp_cal_delete_mstp_forward_delay_timer (port);
#endif /* HAVE_HA */
          l2_stop_timer (&port->forward_delay_timer);
        }
    }

  if (port->oper_edge ||
      port->cist_state == STATE_DISCARDING)
    {
      if (l2_is_timer_running (port->recent_root_timer))
        {
          l2_stop_timer(&port->recent_root_timer);
#ifdef HAVE_HA
          mstp_cal_delete_mstp_rr_timer (port);
#endif /* HAVE_HA */
          port->recent_root_timer = NULL;
          port->br->br_all_rr_timer_cnt--;
        }
    }

  if (LAYER2_DEBUG(proto, PROTO))
    zlog_debug (mstpm, "mstp_cist_handle_designated_port_transition: End");

} /* End Function */

static  struct mstp_port *
mstp_cist_cmp_ports (struct mstp_port *port,  struct mstp_port *candidate_port)
{
  int result;
  struct mstp_bridge *br = port->br;
  u_int32_t port_path_cost, candidate_port_path_cost;


  if (LAYER2_DEBUG(proto,PROTO_DETAIL))
    zlog_debug (mstpm,
                "mstp_cist_cmp_ports: Comparing ports on bridge %s - "
                "curr root port %u with %u", br->name, port->ifindex,
                candidate_port ? candidate_port->ifindex:0);

  /* Section 9.2.7 802.1t */
  if (candidate_port == NULL)
    return port;

  /* Section 8.6.8.3.1.a */
  result = pal_mem_cmp (&port->cist_root,
                        &candidate_port->cist_root, MSTP_BRIDGE_ID_LEN);

  if (LAYER2_DEBUG (proto, PROTO_DETAIL))
    {
      print_macaddr ("port->cist_root",&port->cist_root);
      print_macaddr ("candidate_port->cist_root",&candidate_port->cist_root);
      zlog_debug (mstpm,"result of comaprison %d",result);
    }

  if (result < 0)
    return port;
  else if (result > 0)
    return candidate_port;

   port_path_cost = port->info_internal ? port->cist_external_rpc
                                        : (port->cist_external_rpc + port->cist_path_cost);
   candidate_port_path_cost = candidate_port->info_internal ? candidate_port->cist_external_rpc
                                      : (candidate_port->cist_external_rpc + candidate_port->cist_path_cost);

  if (LAYER2_DEBUG (proto, PROTO_DETAIL))
    {

       zlog_debug (mstpm, "port Info internal %d Candidate Info Internal %d",
                    port->info_internal, candidate_port->info_internal);
       zlog_debug (mstpm, "port Configured Path cost %u Candidate Configured patch cost%u",
                    port->cist_external_rpc, candidate_port->cist_external_rpc);
       zlog_debug (mstpm, "port External Path cost %u Candidate Configured External path cost%u",
                    port->cist_path_cost, candidate_port->cist_path_cost);
       zlog_debug (mstpm, "port external root pc %u Candidate port external pc %u",
                   port_path_cost, candidate_port_path_cost);
    }

  /* Section 8.6.8.3.1.b */
  if (port_path_cost < candidate_port_path_cost)
    return port;
  else if (port_path_cost > candidate_port_path_cost)
    return candidate_port;

  result = pal_mem_cmp (&port->cist_reg_root, &candidate_port->cist_reg_root, MSTP_BRIDGE_ID_LEN);

  if (LAYER2_DEBUG (proto, PROTO_DETAIL))
    {
      print_macaddr ("port->cist_reg_root",&port->cist_reg_root);
      print_macaddr ("candidate_port->cist_reg_root",&candidate_port->cist_reg_root);
      zlog_debug (mstpm,"result of comaprison %d",result);
    }

  if (result < 0)
    return port;
  else if (result > 0)
    return candidate_port;

  if (LAYER2_DEBUG (proto, PROTO_DETAIL))
    {
       zlog_debug (mstpm, "port internal root pc %d Candidate port internal pc %d",
                   (port->cist_internal_rpc + port->cist_path_cost),
                   (candidate_port->cist_internal_rpc + candidate_port->cist_path_cost));
    }

  /* Both ports have the same regional root. Hence both the ports belong to the
   * same region. Hence we have to compare the internal path cost offered
   * by each of the port.
   */

  if ( (port->cist_internal_rpc + port->cist_path_cost) <
       (candidate_port->cist_internal_rpc +  candidate_port->cist_path_cost))
    return port;
  else if ( (port->cist_internal_rpc + port->cist_path_cost) >
            (candidate_port->cist_internal_rpc + candidate_port->cist_path_cost))
    return candidate_port;

  /* Section 8.6.8.3.1.c */
  result = pal_mem_cmp (&port->cist_designated_bridge,
                        &candidate_port->cist_designated_bridge, MSTP_BRIDGE_ID_LEN);

  if (LAYER2_DEBUG (proto, PROTO_DETAIL))
    {
      print_macaddr ("port->cist_designated_bridge",&port->cist_designated_bridge);
      print_macaddr ("candidate_port->cist_designated_bridge",&candidate_port->cist_designated_bridge);
      zlog_debug (mstpm,"result of comaprison %d",result);
    }

  if (result < 0)
    return port;
  else if (result > 0)
    return candidate_port;

  /* Section 8.6.8.3.1.d */

  if (LAYER2_DEBUG (proto, PROTO_DETAIL))
    {
       zlog_debug (mstpm, "port Designated port id %d candidate port Designated Port Id %d",
                   port->cist_designated_port_id, candidate_port->cist_designated_port_id);
    }

  if (port->cist_designated_port_id < candidate_port->cist_designated_port_id)
    return port;
  else if (port->cist_designated_port_id >
           candidate_port->cist_designated_port_id)
    return candidate_port;

  if (LAYER2_DEBUG (proto, PROTO_DETAIL))
    {
       zlog_debug (mstpm, "port port id %d candidate port Port Id %d",
                   port->cist_port_id, candidate_port->cist_port_id);
    }

  /* Section 8.6.8.3.1.e */
  if (port->cist_port_id < candidate_port->cist_port_id)
    return port;

  return candidate_port;
}


static void
mstp_cist_root_port_selection (struct mstp_bridge *br)
{
  struct mstp_port *root_port;
  struct mstp_port *port;

  root_port = NULL;

  if (LAYER2_DEBUG(proto,PROTO_DETAIL))
    zlog_debug (mstpm,"mstp_cist_root_port_selection: bridge %s", br->name);

  /* Select the best candidate for root port. */

  for (port = br->port_list; port; port = port->next)
    {
      /* calculate root path priority vectors for all ports which are not
       * disabled and has info recvd andwhose designated bridge is not equal to
       * desig bridge of bridge's own prio vector */

      if ((port->cist_role != ROLE_DISABLED) &&
          (port->cist_info_type == INFO_TYPE_RECEIVED) &&
          (pal_mem_cmp (&port->cist_designated_bridge,&port->br->cist_bridge_id,
                        MSTP_BRIDGE_ID_LEN)))
        {

          /* Ports with Restricted Role not considered for root port selection */
          if (port->restricted_role == PAL_TRUE)
            continue;

          root_port = mstp_cist_cmp_ports (port, root_port);

          if (LAYER2_DEBUG(proto, PROTO_DETAIL))
            zlog_debug (mstpm,"mstp_cist_root_port_selection: "
                        "comparing port %u - current best root %u",
                        port->ifindex, root_port->ifindex);
        }
    }


  if (LAYER2_DEBUG(proto, PROTO))
    zlog_debug (mstpm, "mstp_cist_root_port_selection: Port (%d)"
                " selected as root", root_port?root_port->ifindex:0);

  if (root_port)
    port = root_port;

  /* Root priority vector is the better of the bridge priority vector and the
     best rooth path priority vector. */

  if (port && (pal_mem_cmp (&port->cist_root,
                            &br->cist_bridge_id,MSTP_BRIDGE_ID_LEN) < 0))
    {
      pal_mem_cpy (&br->cist_designated_root, &port->cist_root,
                   MSTP_BRIDGE_ID_LEN);
      pal_mem_cpy (&br->cist_reg_root, &port->cist_reg_root,
                   MSTP_BRIDGE_ID_LEN);
      pal_mem_cpy (&br->cist_designated_bridge,&port->cist_designated_bridge,
                   MSTP_BRIDGE_ID_LEN);
      if (port->info_internal)
        {
          br->internal_root_path_cost =
              port->cist_internal_rpc + port->cist_path_cost;
          br->external_root_path_cost = port->cist_external_rpc;
          br->cist_message_age = port->cist_message_age;
          br->hop_count = port->hop_count;
        }
      else
        {
          br->external_root_path_cost =
            port->cist_external_rpc + port->cist_path_cost;
          br->internal_root_path_cost = 0;

          /* 802.1 Q 2005, Section 13.26.23 point c */
          /* 802.1 D 2004, Section 17.21.25 point c */
          br->cist_message_age = port->cist_message_age +
                                 (1 * L2_TIMER_SCALE_FACT);

          br->hop_count = br->bridge_max_hops;
        }

      br->cist_root_port_id = port->cist_port_id;
      br->cist_root_port_ifindex = port->ifindex;
      br->root_port = port;

      if (IS_BRIDGE_STP (br))
        {
          if (port->ifindex == br->bpdu_recv_port_ifindex
              && port->cist_max_age != br->cist_max_age)
            {
              if (l2_is_timer_running(port->message_age_timer))
                {
#ifdef HAVE_HA
                  mstp_cal_delete_mstp_message_age_timer (port);
#endif /* HAVE_HA */

                  l2_stop_timer (&port->message_age_timer);
                }
              port->msg_age_timer_cnt--;
              port->message_age_timer =
                       l2_start_timer (mstp_message_age_timer_handler,
                             port, port->cist_max_age, mstpm);
#ifdef HAVE_HA
              mstp_cal_create_mstp_message_age_timer (port);
#endif /* HAVE_HA */
            }
        }

      /* populate the timer values for root port */
      br->cist_max_age = port->cist_max_age;
      br->cist_hello_time = port->cist_hello_time;
      br->cist_forward_delay = port->cist_fwd_delay;
    }
  else
    {
      pal_mem_cpy (&br->cist_designated_root, &br->cist_bridge_id,
                   MSTP_BRIDGE_ID_LEN);
      pal_mem_cpy (&br->cist_reg_root, &br->cist_bridge_id,
                   MSTP_BRIDGE_ID_LEN);
      pal_mem_cpy (&br->cist_designated_bridge, &br->cist_bridge_id,
                   MSTP_BRIDGE_ID_LEN);
      br->external_root_path_cost = 0;
      br->internal_root_path_cost = 0;
      br->cist_root_port_id = 0;
      br->cist_root_port_ifindex = 0;

      /* populate the timer values for root port */
      br->cist_max_age = br->bridge_max_age;
      br->cist_hello_time = br->bridge_hello_time;
      br->cist_forward_delay = br->bridge_forward_delay;
      br->cist_message_age = 0;
      br->hop_count = br->bridge_max_hops;
    }
}


/* Implement Topology Change State Machine
 * PROPOGATING state
 * IEEE 802.1w-2001 sec 17.25 */

void
mstp_cist_propogate_topology_change (struct mstp_port *this_port)
{
  struct mstp_port *port;
  struct mstp_bridge *br = this_port->br;
  struct nsm_msg_bridge msg;
  struct nsm_msg_bridge_if msgif;
  int index;

  if (LAYER2_DEBUG(proto,PROTO_DETAIL))
    zlog_debug (mstpm, "mstp_cist_propogate_topology_change: bridge %s",
                br->name);

  br->num_topo_changes++;
  br->total_num_topo_changes++;
   
  br->time_last_topo_change = pal_time_sys_current (NULL);

  pal_mem_set(&msgif, 0, sizeof(struct nsm_msg_bridge_if));
  pal_strncpy(msgif.bridge_name, br->name, NSM_BRIDGE_NAMSIZ);


 if (! BRIDGE_TYPE_CVLAN_COMPONENT (br)
     || this_port->port_type == MSTP_VLAN_PORT_MODE_CE)
   {
     for (port = br->port_list; port; port = port->next)
       {
         if (port != this_port)
           {
             if (port->cist_tc_state == TC_ACTIVE
                 && ! port->oper_edge)
               {
                 port->br->tc_initiator = port->ifindex;
                 if (!l2_is_timer_running (port->tc_timer))
                   {
                     if (LAYER2_DEBUG(proto,PROTO_DETAIL))
                       zlog_debug (mstpm,"mstp_cist_propogate_topology_change "
                                   "starting tc timer for port %u ",
                                   port->ifindex);

                     port->tc_timer = l2_start_timer (
                                               mstp_tc_timer_handler, port,
                                               mstp_cist_get_tc_time (port),
                                               mstpm);
#ifdef HAVE_HA
                     mstp_cal_create_mstp_tc_timer (port);
#endif /* HAVE_HA */
                     port->newInfoCist = PAL_TRUE;
                   }

                 if (LAYER2_DEBUG(proto,PROTO_DETAIL))
                   zlog_debug (mstpm," flushing fdb for port %u", port->ifindex);

                 /* Send message to nsm to flush fdb. */
                 msgif.num++;
              }
           }
       }
    }
  else
    {
      port = mstp_find_ce_br_port (br, MSTP_VLAN_NULL_VID);
      if (port == NULL)
        {
          if (LAYER2_DEBUG(proto,PROTO_DETAIL))
            zlog_debug (mstpm,"port not found\n");
          return;
        }

      if (port->cist_tc_state == TC_ACTIVE
          && ! port->oper_edge)
        {
          if (!l2_is_timer_running (port->tc_timer))
            {
              if (LAYER2_DEBUG(proto,PROTO_DETAIL))
                zlog_debug (mstpm,"mstp_cist_propogate_topology_change "
                            "starting tc timer for port %u ",
                            port->ifindex);

              port->tc_timer = l2_start_timer (
                                        mstp_tc_timer_handler, port,
                                        mstp_cist_get_tc_time (port),
                                        mstpm);

#ifdef HAVE_HA
              mstp_cal_create_mstp_tc_timer (port);
#endif /* HAVE_HA */
            }
        }
    }

  /* If the NSM Client is down, MSTP cann't send the message
   * hence returning from here.
   */
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
      if (! BRIDGE_TYPE_CVLAN_COMPONENT (br)
          || this_port->port_type == MSTP_VLAN_PORT_MODE_CE)
        {
          for (port = br->port_list; port; port = port->next)
             {
               if (port != this_port)
                 {
                   if (port->cist_tc_state == TC_ACTIVE
                       && ! port->oper_edge)
                     {
                       msgif.ifindex[index++] = port->ifindex;
                     }
                 }
             }
        }
      else
        {
          port = mstp_find_ce_br_port (br, MSTP_VLAN_NULL_VID);
          if (port == NULL)
            {
              if (LAYER2_DEBUG(proto,PROTO_DETAIL))
                zlog_debug (mstpm,"port not found\n");
              return;
            }

          if (port->cist_tc_state == TC_ACTIVE
              && ! port->oper_edge)
            {
              msgif.ifindex[index++] = port->ifindex;
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

  /* Send topology changes message to nsm. IGMP Snoop uses this message
     to generate membership query for bridge */

  pal_mem_set(&msg, 0, sizeof(struct nsm_msg_bridge));
  pal_strncpy (msg.bridge_name, br->name, NSM_BRIDGE_NAMSIZ);

  if ((mstpm->nc != NULL) &&
      (!CHECK_FLAG (mstpm->nc->nsm_server_flags, NSM_SERVER_SHUTDOWN)))
        nsm_client_send_bridge_msg(mstpm->nc, &msg, NSM_MSG_BRIDGE_TCN);
}

void
mstp_cist_process_message (struct mstp_port *port,
                           struct mstp_bpdu *bpdu)
{

  int rcvd_msg;
  u_int32_t is_root;
  s_int32_t message_age;
  struct mstp_bridge *br;
  struct mstp_instance_port *inst_port = NULL;

  br = port->br;
  is_root = mstp_bridge_is_cist_root (br);

  rcvd_msg = mstp_handle_rcvd_cist_bpdu (port,bpdu);

  /* recordMasteredCist 13.26.11 */
  if (!port->info_internal)
    {
      inst_port = port->instance_list;
      while (inst_port)
        {
          inst_port->msti_mastered = PAL_FALSE ;
          inst_port = inst_port->next_inst;
        }
    }

  if (LAYER2_DEBUG(proto, PROTO_DETAIL))
    zlog_info (mstpm, "mstp_cist_process_message: bridge %s port (%u) ",
               port->br->name, port->ifindex );

  /* rcv_msg = rcvBPDU() */
  if (rcvd_msg == MSG_TYPE_SUPERIOR_DESIGNATED)
    {
      if (LAYER2_DEBUG(proto, PROTO))
        zlog_info (mstpm,"mstp_cist_process_message: Recvd superior message ");

      if (port->cist_role == ROLE_DISABLED)
        return;

      if (port->admin_rootguard)
        {
          if (LAYER2_DEBUG (proto, PROTO))
            zlog_debug (mstpm, "mstp_cist_process_message: "
                        "received a superior bpdu on a root guard enabled"
                        "port %u. setting port state to root-inconsistent",
                        port->ifindex);

          mstp_cist_set_port_state (port, STATE_DISCARDING);

          if (l2_is_timer_running(port->forward_delay_timer))
            {
#ifdef HAVE_HA
              mstp_cal_delete_mstp_forward_delay_timer (port);
#endif /* HAVE_HA */

              l2_stop_timer (&port->forward_delay_timer);
            }

          port->oper_rootguard = PAL_TRUE;

          mstp_cist_set_port_state (port, STATE_DISCARDING);

          l2_stop_timer (&port->forward_delay_timer);

          /* Section 8.6.12.3.2 */
          port->forward_delay_timer =
            l2_start_timer (mstp_forward_delay_timer_handler,
                            port, mstp_cist_get_fdwhile_time (port),
                            mstpm);

#ifdef HAVE_SMI
          smi_capture_mstp_info (SMI_STP_ROOT_GUARD_VIOLATE_SET,
                                 port->name, __LINE__, __FILE__);
#endif /* HAVE_SMI */

#ifdef HAVE_HA
          mstp_cal_create_mstp_forward_delay_timer (port);
#endif /* HAVE_HA */

          return;
        }

      /* If the received external message is aged and the port is already
       * a Designated port, ignore the message.
       */

      if ( (!port->info_internal) &&
           (bpdu->message_age > port->cist_max_age) &&
           (port->cist_role == ROLE_DESIGNATED ))
        {
          return;
        }
         if (port->info_internal
            && (port->rcvd_internal != port->info_internal))
         {
           inst_port = port->instance_list;
  
           while (inst_port)
             {
               inst_port->info_type = INFO_TYPE_AGED;
               inst_port->reselect = PAL_TRUE;
               inst_port->selected = PAL_FALSE;
               mstp_msti_port_role_selection (inst_port->br);
               inst_port = inst_port->next_inst;
             }
         }
#ifdef HAVE_HA
      mstp_cal_delete_mstp_message_age_timer (port);
#endif /* HAVE_HA */

      l2_stop_timer (&port->message_age_timer);
      port->msg_age_timer_cnt--;
      port->agree = port->agree && better_or_same_info_cist_rcvd (port, bpdu);
      mstp_cist_record_priority_vector (port, bpdu);
      message_age = mstp_cist_update_message_age (port, bpdu);
      port->message_age_timer =
        l2_start_timer (mstp_message_age_timer_handler,
                        port, message_age,mstpm);
       port->msg_age_timer_cnt++;
#ifdef HAVE_HA
      mstp_cal_create_mstp_message_age_timer (port);
#endif /* HAVE_HA */

      /* 802.1 Q 2005 13.26.10, 802.1 D 2004 17.21.11 */
      if (bpdu->cist_proposal)
        port->rcv_proposal = bpdu->cist_proposal;

      if (!port->info_internal)
        {
          inst_port = port->instance_list;

          while (inst_port)
            {
              inst_port->rcv_proposal = port->rcv_proposal;
              inst_port = inst_port->next_inst;
            }
        }

      port->send_proposal = PAL_FALSE;
      port->cist_info_type = INFO_TYPE_RECEIVED;
      port->reselect = PAL_TRUE;
      port->selected = PAL_FALSE;
      port->agreed = PAL_FALSE;

      /* Do not waste a function call if message is abt to expire */
      if (message_age != 0 )
        {
          mstp_cist_port_role_selection (port->br);
        }

      /* Section 8.7.1.1 Case 1 Step 5 */
      if (IS_BRIDGE_STP (br))
        {
          if (is_root && ! mstp_bridge_is_cist_root (br))
            {
              if (br->topology_change_detected)
                {
                  br->num_topo_changes++;
                  br->time_last_topo_change = pal_time_sys_current (NULL);
                  br->total_num_topo_changes++;
                  port->br->tc_initiator = port->ifindex; 
                  pal_mem_cpy(&port->br->tc_last_rcvd_from, 
                              &bpdu->cist_bridge_id.addr,
                              ETHER_ADDR_LEN);
	          
#ifdef HAVE_HA
                  mstp_cal_delete_mstp_br_tc_timer (br);
                  mstp_cal_delete_mstp_tcn_timer (br);
#endif /* HAVE_HA */

                  l2_stop_timer (&br->topology_change_timer);
                  stp_transmit_tcn_bpdu (br);
                  l2_stop_timer (&br->tcn_timer);
                  br->tcn_timer = l2_start_timer (stp_tcn_timer_handler,
                                                  br, br->bridge_hello_time, mstpm);
#ifdef HAVE_HA
                  mstp_cal_create_mstp_tcn_timer (br);
#endif /* HAVE_HA */
                }
            }

          if (port->ifindex == br->cist_root_port_ifindex)
            {
              br->topology_change = bpdu->cist_topology_change;
              stp_generate_bpdu_on_bridge (br);

              if (bpdu->cist_topology_change_ack)
                {
                  if (LAYER2_DEBUG(proto, PROTO_DETAIL))
                    zlog_info (mstpm, "mstp_cist_process_message: Recv TC ACK "
                               "bridge %s port (%u) ", port->br->name, 
                               port->ifindex);
                  br->topology_change_detected = PAL_FALSE;
#ifdef HAVE_HA
                  mstp_cal_delete_mstp_tcn_timer (br);
#endif /* HAVE_HA */

                  l2_stop_timer (&br->tcn_timer);
                  mstp_nsm_send_ageing_time( br->name, port->br->ageing_time);
                  br->ageing_time_is_fwd_delay = PAL_FALSE;
                }

              if (br->topology_change)
                {
                  mstp_nsm_send_ageing_time (br->name,
                     port->br->cist_forward_delay/ MSTP_TIMER_SCALE_FACT);
                  br->ageing_time_is_fwd_delay = PAL_TRUE;
                }

              /* Set the ageing time back to br->ageing_time,
                if it is not same */
              if ((! br->topology_change)
                   && (br->ageing_time_is_fwd_delay))
                {
                  mstp_nsm_send_ageing_time (br->name,
                                             port->br->ageing_time);
                  br->ageing_time_is_fwd_delay = PAL_FALSE;
                }
            }
        }

    }
  else if (rcvd_msg == MSG_TYPE_REPEATED_DESIGNATED)
    {
      port->similar_bpdu_cnt++;
      if (LAYER2_DEBUG(proto, PROTO))
        zlog_info (mstpm,"mstp_cist_process_message: Recvd repeated message");

#ifdef HAVE_HA
      mstp_cal_delete_mstp_message_age_timer (port);
#endif /* HAVE_HA */

      l2_stop_timer (&port->message_age_timer);
      port->msg_age_timer_cnt--;

      /* 802.1 Q 2005 13.26.10, 802.1 D 2004 17.21.11 */
      if (bpdu->cist_proposal)
        port->rcv_proposal = bpdu->cist_proposal;

      if (!port->info_internal)
        {
          inst_port = port->instance_list;
          while (inst_port)
            {
              inst_port->rcv_proposal = port->rcv_proposal;
              inst_port = inst_port->next_inst;
            }
        }

      if (port->rcv_proposal
          && port->cist_role != ROLE_BACKUP)
        {
          mstp_cist_rcv_proposal (port);
          port->rcv_proposal = PAL_FALSE;
          mstp_cist_send_proposal (port);
        }

      message_age = mstp_cist_update_message_age (port, bpdu);
      port->message_age_timer =
                   l2_start_timer (mstp_message_age_timer_handler,
                   port, message_age, mstpm);
      port->msg_age_timer_cnt++;
#ifdef HAVE_HA
      mstp_cal_create_mstp_message_age_timer (port);
#endif /* HAVE_HA */

      if (IS_BRIDGE_STP (br))
        {
          if (port->cist_role == ROLE_DESIGNATED)
            {
              port->newInfoCist = PAL_TRUE;
              mstp_tx (port);
            }

          if (port->ifindex == br->cist_root_port_ifindex)
            {
              br->topology_change = bpdu->cist_topology_change;
              stp_generate_bpdu_on_bridge (br);

              if (bpdu->cist_topology_change_ack)
                {
                  if (LAYER2_DEBUG(proto, PROTO_DETAIL))
                    zlog_info (mstpm, "mstp_cist_process_message: Recv TC ACK "
                               "bridge %s port (%u) ", port->br->name, 
                               port->ifindex);
                  br->topology_change_detected = PAL_FALSE;
                  l2_stop_timer (&port->br->tcn_timer);
                  mstp_nsm_send_ageing_time( port->br->name,
                                             port->br->ageing_time);
                  br->ageing_time_is_fwd_delay = PAL_FALSE;
                }

              if (br->topology_change)
                {
                  mstp_nsm_send_ageing_time (br->name,
                     port->br->cist_forward_delay/ MSTP_TIMER_SCALE_FACT);
                  br->ageing_time_is_fwd_delay = PAL_TRUE;
                }

              /* Set the ageing time back to br->ageing_time,
                if it is not same */
              if ((! br->topology_change)
                   && (br->ageing_time_is_fwd_delay))
                {
                  mstp_nsm_send_ageing_time (br->name,
                                             port->br->ageing_time);
                  br->ageing_time_is_fwd_delay = PAL_FALSE;
                }

            }
        }
    }
  else if (rcvd_msg == MSG_TYPE_INFERIOR_ALTERNATE
           && port->agreed)
    {
      if (LAYER2_DEBUG(proto, PROTO))
        zlog_info (mstpm,"mstp_cist_process_message: Recvd confirmed message");

      if (l2_is_timer_running(port->forward_delay_timer))
        {
#ifdef HAVE_HA
          mstp_cal_delete_mstp_forward_delay_timer (port);
#endif /* HAVE_HA */
          l2_stop_timer (&port->forward_delay_timer);
        }

      /* We have received confirmation from the adjacent bridge
         to rapidly transition to forwarding state */
      port->send_proposal = PAL_FALSE;
      port->agreed = port->send_mstp;
      if (port->cist_state != STATE_FORWARDING)
        {
          if (port->cist_state != STATE_LEARNING)
            {
              if (port->cist_role == ROLE_ALTERNATE)
                 mstp_cist_set_port_state (port, STATE_DISCARDING);
              else
                 mstp_cist_set_port_state (port, STATE_LEARNING);
            }
          if (port->cist_state == STATE_LEARNING)
            mstp_cist_set_port_state (port, STATE_FORWARDING);
          port->newInfoCist = PAL_TRUE;
        }
    }
  else if (rcvd_msg == MSG_TYPE_INFERIOR_DESIGNATED
           && port->disputed
           && port->cist_role == ROLE_DESIGNATED
           && ! port->oper_edge)
    {
      /* Section 17.21.10 Standard is incorrect.
       * agreed should be reset and proposal should
       * be set.
       */

      port->agreed = PAL_FALSE;
      port->send_proposal = PAL_TRUE;
      mstp_cist_set_port_state (port, STATE_DISCARDING);

      inst_port = port->instance_list;
      while (inst_port)
        {
          mstp_msti_set_port_state (inst_port, STATE_DISCARDING);
#ifdef HAVE_HA
          mstp_cal_modify_mstp_instance_port (inst_port);
#endif /* HAVE_HA */
          inst_port = inst_port->next_inst;
        }

      port->disputed = PAL_FALSE;

      if (l2_is_timer_running(port->forward_delay_timer))
        {
#ifdef HAVE_HA
          mstp_cal_delete_mstp_forward_delay_timer (port);
#endif /* HAVE_HA */
          l2_stop_timer (&port->forward_delay_timer);
        }

      port->forward_delay_timer = l2_start_timer
                                     (mstp_forward_delay_timer_handler,
                                      port,
                                      mstp_cist_get_fdwhile_time (port), mstpm);
#ifdef HAVE_HA
       mstp_cal_create_mstp_forward_delay_timer(port);
#endif /* HAVE_HA */
      port->newInfoCist = PAL_TRUE;
    }
  else
    {

      if (IS_BRIDGE_STP (br) &&
          port->cist_role == ROLE_DESIGNATED)
        {
          port->newInfoCist = PAL_TRUE;
          mstp_tx (port);
        }

      if (LAYER2_DEBUG(proto, PROTO))
        zlog_debug (mstpm,"mstp_cist_process_csti_message: "
                    "Recvd inferior message");
    }

}

