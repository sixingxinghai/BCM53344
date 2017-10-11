/*
   Copyright (C) 2003  All Rights Reserved.

   LAYER 2 BRIDGE

   This module declares the interface to the Bridge
   functions.

 */
#include "pal.h"
#include "lib.h"
#include "l2_timer.h"
#include "l2lib.h"

#include "nsm_client.h"

#include "mstp_config.h"
#include "mstpd.h"
#include "mstp_types.h"
#include "mstp_bpdu.h"
#include "mstp_cist_proto.h"
#include "mstp_msti_proto.h"
#include "mstp_transmit.h"
#include "mstpd.h"
#include "mstp_bridge.h"
#include "mstp_port.h"

#ifdef HAVE_RPVST_PLUS
#include "mstp_rpvst.h"
#endif /* HAVE_RPVST_PLUS */

#ifdef HAVE_SMI
#include "lib_fm_api.h"
#include "smi_message.h"
#include "smi_mstp_fm.h"
#endif /* HAVE_SMI */

#ifdef HAVE_HA
#include "mstp_cal.h"
#endif /* HAVE_HA */


static inline void mstp_build_br_id (struct bridge_id * br, u_char *addr,
                                     char prio, short system_id)
{

    br->prio[0] = (prio & 0xf0) | ((system_id >> 8) & 0x0f);
    br->prio[1] = system_id & 0xff ;
    pal_mem_cpy (br->addr , addr, 6);
}

/* This is a utility function to convert from STP BPDU time to ticks. */

inline s_int32_t
mstp_parse_timer (u_char *dest)
{
  return (dest[0] << 8) + dest[1];
}

static void
mstp_parse_msti_msg (u_char *bufptr, struct mstp_bpdu *bpdu, int len,
                     unsigned char admin_cisco)
{
  struct mstp_instance_bpdu *inst_bpdu ;
  struct mstp_instance_bpdu *prev = NULL;
  int num_instances = 0;
  int index;

  if (bpdu->cisco_bpdu && admin_cisco)
    num_instances = (len - BR_MST_MIN_VER_THREE_LEN)/ MSTI_CISCO_BPDU_FRAME_SIZE;
  else
    num_instances = (len - BR_MST_MIN_VER_THREE_LEN)/ MSTI_BPDU_FRAME_SIZE;


  if (num_instances > MAX_MSTI)
    num_instances = MAX_MSTI;

  if (LAYER2_DEBUG(proto, PROTO_DETAIL))
    {
        zlog_debug (mstpm,"mstp_parse_msti_msg: Number of instances in the message %d  "
                    "Admin CISCO %d  Length %d",
                    num_instances, admin_cisco, len);
    }

  for (index = 0; index < num_instances ; index++)
    {
      inst_bpdu = XCALLOC(MTYPE_MSTP_BPDU_INST,
                  sizeof (struct mstp_instance_bpdu)) ;
      if (!prev)
        bpdu->instance_list = inst_bpdu;
      else
        prev->next = inst_bpdu;

      if (bpdu->cisco_bpdu && admin_cisco)
        {
          /* CISCO Terminates the CIST and each MSTI message with a 0 */
          bufptr++;

          /* CISCO also starts each MSTI with instance id
           * We derive the instance ID from the MSTI Root ID.
           */

          bufptr++;
        }

      /* Parse Flags */
      inst_bpdu->msti_topology_change = (*bufptr & 0x01);
      inst_bpdu->msti_proposal = (*bufptr & (0x01 << 1)) ? PAL_TRUE : PAL_FALSE;
      inst_bpdu->msti_port_role = ((*bufptr & (0x03 << 2)) >> 2) ;
      inst_bpdu->msti_learning = (*bufptr & (0x01 << 4)) ? PAL_TRUE : PAL_FALSE;
      inst_bpdu->msti_forwarding = (*bufptr & (0x01 << 5)) ?
                                                    PAL_TRUE : PAL_FALSE;
      inst_bpdu->msti_agreement = (*bufptr & (0x01 << 6)) ?
                                                    PAL_TRUE : PAL_FALSE;
      inst_bpdu->msti_master = ((*bufptr++ & (0x01 << 7)) ? 1 :0) ;


      inst_bpdu->msti_reg_root.prio[0] = *bufptr++;
      inst_bpdu->msti_reg_root.prio[1] = *bufptr++;
      inst_bpdu->msti_reg_root.addr[0] = *bufptr++;
      inst_bpdu->msti_reg_root.addr[1] = *bufptr++;
      inst_bpdu->msti_reg_root.addr[2] = *bufptr++;
      inst_bpdu->msti_reg_root.addr[3] = *bufptr++;
      inst_bpdu->msti_reg_root.addr[4] = *bufptr++;
      inst_bpdu->msti_reg_root.addr[5] = *bufptr++;
      inst_bpdu->msti_internal_rpc |= (*bufptr++ << 24);
      inst_bpdu->msti_internal_rpc |= (*bufptr++ << 16);
      inst_bpdu->msti_internal_rpc |= (*bufptr++ << 8);
      inst_bpdu->msti_internal_rpc |= *bufptr++;

      if (bpdu->cisco_bpdu && admin_cisco)
        {
          inst_bpdu->bridge_id.prio[0] = *bufptr++;
          inst_bpdu->bridge_id.prio[1] = *bufptr++;
          inst_bpdu->bridge_id.addr[0] = *bufptr++;
          inst_bpdu->bridge_id.addr[1] = *bufptr++;
          inst_bpdu->bridge_id.addr[2] = *bufptr++;
          inst_bpdu->bridge_id.addr[3] = *bufptr++;
          inst_bpdu->bridge_id.addr[4] = *bufptr++;
          inst_bpdu->bridge_id.addr[5] = *bufptr++;
          inst_bpdu->msti_port_id = *bufptr++ << 8;
          inst_bpdu->msti_port_id += *bufptr++;
        }
      else
        {
          inst_bpdu->msti_bridgeid_prio = (*bufptr++) & 0xF0;
          inst_bpdu->msti_portid_prio = (*bufptr++ ) & 0xF0;
        }

      inst_bpdu->rem_hops = *bufptr++;

      inst_bpdu->instance_id = (inst_bpdu->msti_reg_root.prio[0] << 8) & 0x0f;
      inst_bpdu->instance_id += (inst_bpdu->msti_reg_root.prio[1]);

      if (! (bpdu->cisco_bpdu && admin_cisco ) )
        {
          /*construct the bridge id here */
          mstp_build_br_id (&inst_bpdu->bridge_id,bpdu->cist_bridge_id.addr,
                            inst_bpdu->msti_bridgeid_prio, inst_bpdu->instance_id);

          /* Construct the port id here */
          inst_bpdu->msti_port_id =
                      (u_int16_t)((bpdu->cist_port_id & 0x0fff) |
                      ((inst_bpdu->msti_portid_prio << 8) & 0xf000));
        }

    if (LAYER2_DEBUG(proto, PROTO_DETAIL))
      {
        zlog_debug (mstpm,"mstp_parse_msti_msg: "
            "msti port id(%x) " , inst_bpdu->msti_port_id );
        zlog_debug (mstpm,"mstp_parse_msti_msg: "
            "msti prio (%d) cist portid (%x)" ,inst_bpdu->msti_portid_prio,
            bpdu->cist_port_id);
        zlog_debug (mstpm,"mstp_parse_msti_msg: Parsing Instance %d",
                    inst_bpdu->instance_id) ;
      }

      prev = inst_bpdu;



    }

  return;

}

static void
mstp_parse_bpdu ( u_char *buf, struct mstp_bpdu *bpdu, int len,
                  unsigned char admin_cisco, struct mstp_port *port)
{

   /* We skip the LLC as it has already been checked */
  register u_char *bufptr = buf;
  /* Parse Header Info. */

  /*clear memory to be on safer side */
  pal_mem_set (bpdu, 0, sizeof (struct mstp_bpdu));

  bpdu->proto_id = *bufptr++ << 8 ;
  bpdu->proto_id += *bufptr++;

  bpdu->version  = *bufptr++;
  bpdu->type     = *bufptr++;

  /* If its is a TCN, then there is nothing else to parse */
  if (bpdu->type == BPDU_TYPE_TCN)
    return;

  /* Parse Flags */
  bpdu->cist_topology_change = (*bufptr & 0x01);
   /* If the recieved version is greater than RSTP i.e version 0x2
    * treat it as RSTP frame, and if the version is less than 2 treat it as STP.
   */
  if (bpdu->version >= BR_VERSION_RSTP)
    {
      bpdu->cist_proposal   = (*bufptr & (0x01 << 1)) ? PAL_TRUE : PAL_FALSE ;
      bpdu->cist_port_role  = ((*bufptr & (0x03 << 2)) >> 2) ;
      bpdu->cist_learning   = (*bufptr & (0x01 << 4)) ? PAL_TRUE : PAL_FALSE;
      bpdu->cist_forwarding = (*bufptr & (0x01 << 5)) ? PAL_TRUE : PAL_FALSE;
      bpdu->cist_agreement  = (*bufptr & (0x01 << 6)) ? PAL_TRUE : PAL_FALSE  ;
    }

  bpdu->cist_topology_change_ack = ((*bufptr++ & (0x01 << 7)) ? 1 :0) ;


  /* Parse Message Priority Vector. */
  bpdu->cist_root.prio[0] = *bufptr++;
  bpdu->cist_root.prio[1] = *bufptr++;
  bpdu->cist_root.addr[0] = *bufptr++;
  bpdu->cist_root.addr[1] = *bufptr++;
  bpdu->cist_root.addr[2] = *bufptr++;
  bpdu->cist_root.addr[3] = *bufptr++;
  bpdu->cist_root.addr[4] = *bufptr++;
  bpdu->cist_root.addr[5] = *bufptr++;
  bpdu->external_rpc |= (*bufptr++ << 24);
  bpdu->external_rpc |= (*bufptr++ << 16);
  bpdu->external_rpc |= (*bufptr++ << 8);
  bpdu->external_rpc |= *bufptr++;
  bpdu->cist_reg_root.prio[0] = *bufptr++;
  bpdu->cist_reg_root.prio[1] = *bufptr++;
  bpdu->cist_reg_root.addr[0] = *bufptr++;
  bpdu->cist_reg_root.addr[1] = *bufptr++;
  bpdu->cist_reg_root.addr[2] = *bufptr++;
  bpdu->cist_reg_root.addr[3] = *bufptr++;
  bpdu->cist_reg_root.addr[4] = *bufptr++;
  bpdu->cist_reg_root.addr[5] = *bufptr++;
  bpdu->cist_port_id = *bufptr++ << 8;
  bpdu->cist_port_id += *bufptr++;

  /* Parse Message Times variable . */
  bpdu->message_age = mstp_parse_timer (bufptr); bufptr+=2;
  bpdu->max_age = mstp_parse_timer (bufptr); bufptr+=2;

  bpdu->hello_time = mstp_parse_timer (bufptr); bufptr += 2;
  bpdu->fwd_delay = mstp_parse_timer (bufptr); bufptr += 2;

  bpdu->version_one_len = *bufptr++;

  if (bpdu->version >= BR_VERSION_MSTP)
    {
      bpdu->version_three_len = (*bufptr++ << 8);
      bpdu->version_three_len  |= *bufptr++;

      if (admin_cisco)
        bpdu->cisco_bpdu = PAL_TRUE;
      else if (bpdu->version_three_len == 0)
        return;

      /* decode config identifier */
      bpdu->rcv_cfg.cfg_format_id = *bufptr++;

      pal_mem_cpy (bpdu->rcv_cfg.cfg_name, bufptr,MSTP_CONFIG_NAME_LEN);
      bufptr += MSTP_CONFIG_NAME_LEN;

      bpdu->rcv_cfg.cfg_revision_lvl = (*bufptr++ << 8);
      bpdu->rcv_cfg.cfg_revision_lvl |= *bufptr++;

      pal_mem_cpy (bpdu->rcv_cfg.cfg_digest, bufptr ,
                                  MSTP_CONFIG_DIGEST_LEN);
      bufptr += MSTP_CONFIG_DIGEST_LEN;


     /* In CISCO BPDU the bridge ID is populated first and internal RPC
      * next.
      */

      if (bpdu->cisco_bpdu)
        {
          /* In CISCO Regional root and bridge id are interchanged */
          pal_mem_cpy (&bpdu->cist_bridge_id, &bpdu->cist_reg_root,
                       sizeof (struct bridge_id));
          bpdu->cist_reg_root.prio[0] = *bufptr++;
          bpdu->cist_reg_root.prio[1] = *bufptr++;
          bpdu->cist_reg_root.addr[0] = *bufptr++;
          bpdu->cist_reg_root.addr[1] = *bufptr++;
          bpdu->cist_reg_root.addr[2] = *bufptr++;
          bpdu->cist_reg_root.addr[3] = *bufptr++;
          bpdu->cist_reg_root.addr[4] = *bufptr++;
          bpdu->cist_reg_root.addr[5] = *bufptr++;

          bpdu->internal_rpc |= (*bufptr++ << 24);
          bpdu->internal_rpc |= (*bufptr++ << 16);
          bpdu->internal_rpc |= (*bufptr++ << 8);
          bpdu->internal_rpc |= *bufptr++;
        }
      else
        {
          bpdu->internal_rpc |= (*bufptr++ << 24);
          bpdu->internal_rpc |= (*bufptr++ << 16);
          bpdu->internal_rpc |= (*bufptr++ << 8);
          bpdu->internal_rpc |= *bufptr++;

          /* octet 94-101 convey cist bridge id of transmitting bridge */
          bpdu->cist_bridge_id.prio[0] = *bufptr++;
          bpdu->cist_bridge_id.prio[1] = *bufptr++;
          bpdu->cist_bridge_id.addr[0] = *bufptr++;
          bpdu->cist_bridge_id.addr[1] = *bufptr++;
          bpdu->cist_bridge_id.addr[2] = *bufptr++;
          bpdu->cist_bridge_id.addr[3] = *bufptr++;
          bpdu->cist_bridge_id.addr[4] = *bufptr++;
          bpdu->cist_bridge_id.addr[5] = *bufptr++;
        }

      bpdu->buf = buf;
      bpdu->len = len;

      bpdu->hop_count = *bufptr++;
      if (bpdu->cisco_bpdu && admin_cisco)
        {
          mstp_parse_msti_msg ( bufptr, bpdu, (len - BR_CONFIG_BPDU_FRAME_SIZE),
                                admin_cisco);
        }
      else
        {
          mstp_parse_msti_msg ( bufptr, bpdu, bpdu->version_three_len, admin_cisco);
        }

    }
  else
    {
      pal_mem_cpy (&bpdu->cist_bridge_id, &bpdu->cist_reg_root, sizeof (struct bridge_id));
    }


}

static int
mstp_is_rcvd_bpdu_valid (struct mstp_port *port,
                         struct mstp_bpdu *bpdu, int len,
                         u_int8_t admin_cisco)
{

  /* 802-1D 2004 Section 9.3.4 */

  /* 802-1Q 2005 Section 14.4 Note 3. Loopback
   * check is not there for MSTP.
   */

  if (bpdu->version != BR_VERSION_MSTP
      && bpdu->cist_port_id == port->cist_port_id
      && port->br != NULL
      && pal_mem_cmp (&bpdu->cist_bridge_id, &port->br->cist_bridge_id,
                      MSTP_BRIDGE_ID_LEN) == 0)
    return PAL_FALSE;

  if (bpdu->proto_id == BR_STP_PROTOID)
    {
      if ((bpdu->type == BPDU_TYPE_CONFIG) &&
          (len >= BR_CBPDU_MINLEN)) 
          /*&&  
          (bpdu->port_id != port->port_id) ||
          (pal_mem_cmp (&bpdu->cist_reg_root, &port->designated_bridge, 
           MSTP_BRIDGE_ID_LEN)) ) */
        {
          bpdu->valid_type = BPDU_TYPE_CONFIG;
          return PAL_TRUE;
        }

      if (bpdu->type == BPDU_TYPE_TCN)
        {
          bpdu->valid_type = BPDU_TYPE_TCN;
          return PAL_TRUE;
        }

      if ((bpdu->type == BPDU_TYPE_RST) &&
          (len >= BR_RSTBPDU_MINLEN)  &&
          (bpdu->version == BR_VERSION_RSTP))
        {
          bpdu->valid_type = BPDU_TYPE_RST;
          return PAL_TRUE;
        }

      if ((bpdu->type == BPDU_TYPE_RST) &&
          (bpdu->version >= BR_VERSION_MSTP) &&
          (len >= BR_CBPDU_MINLEN))
        {
          /* Ensure that we receive integral number of msti frames */
          int msti_len = (bpdu->version_three_len  - BR_MST_MIN_VER_THREE_LEN)
                          % MSTI_BPDU_FRAME_SIZE ;

          /* Now we will return TRUE
           * but we still need to determine if this pdu is to be treated
           * as valid mst bpdu or rst bpdu. */

            if ((len > BR_MST_MIN_VER_THREE_LEN) &&
                (bpdu->version_one_len == 0) &&
                (msti_len == 0))
              {
                bpdu->valid_type = BPDU_TYPE_MST;
              }
            else
              {
               if (admin_cisco)
                  bpdu->valid_type = BPDU_TYPE_MST;
               else
                  bpdu->valid_type = BPDU_TYPE_RST;
              }

            return PAL_TRUE;

        }

           /*
            && (len <= BR_MST_MIN_VER_THREE_LEN)) &&
          (bpdu->version_one_len != 0) &&

         )*/


  } /* If proto id is br_proto_id */

  return PAL_FALSE;


}


/* This function implements the Port Receive State Machine
 * Sec 13.28 IEEE 802.1s. */
static void
mstp_process_bpdu (struct mstp_port *port, struct mstp_bpdu *bpdu)
{
  struct mstp_bridge *br =  NULL;
  struct mstp_instance_bpdu *inst_bpdu = NULL;
  struct mstp_instance_port *inst_port = NULL;

  pal_assert (port);
  br = port->br;

  /* STP/RSTP Bridges may not proceess bpdus of higher version of STP */

  if ((IS_BRIDGE_STP (br))
      && ((bpdu->type == BPDU_TYPE_RST) || (bpdu->type == BPDU_TYPE_MST)))
    return;

  if (((IS_BRIDGE_RSTP (br)) || (IS_BRIDGE_RPVST_PLUS(br))) 
         && (bpdu->type == BPDU_TYPE_MST))
    return;


  /* update bpdu version */
  if (!l2_is_timer_running(port->migrate_timer))
    {
      /* Update BPDU version */
      port->rcvd_mstp =  ((bpdu->type == BPDU_TYPE_RST)
                          && (port->force_version >= BR_VERSION_RSTP)
                          && (bpdu->cist_port_role != ROLE_UNKNOWN));

      port->rcvd_rstp = ((bpdu->version == BR_VERSION_RSTP)
                         && (port->force_version >= BR_VERSION_RSTP)
                         && (bpdu->cist_port_role != ROLE_UNKNOWN));

      /* An unknown value of port role cannot be generated by a
        valid implementation. Hence we treat it as a Config BPDU. */
      port->rcvd_stp = ((bpdu->type == BPDU_TYPE_CONFIG) ||
                        (bpdu->type == BPDU_TYPE_TCN) ||
                        ((bpdu->type == BPDU_TYPE_RST)
                        && bpdu->cist_port_role == ROLE_UNKNOWN));

      /* rcvd_mstp or rcvd_stp indicates  valid bpdu rcvd .
       * If neither is turned on - do not process the BPDU further */
      if (!port->rcvd_stp && !port->rcvd_mstp)
      {
        zlog_err (mstpm,"Could not validate BPDU version");
        return;
      }
    }

  if (port->send_mstp != port->rcvd_mstp)
    {
      port->send_mstp ? mstp_sendstp (port) : mstp_sendmstp (port);
    }


  /* The recieved info is from the same region if force version is mstp
   * and the configuration digest matches.
   * For CISCO BPDUs we can make use of only the Region name and revision level
   * since the format id and config digest that we get from CISCO BPDU is
   * not correct.
   */

  if ((port->force_version == BR_VERSION_MSTP) &&
      (bpdu->valid_type == BPDU_TYPE_MST))
    {
     if (bpdu->cisco_bpdu && br->admin_cisco)
       {
         if (!pal_mem_cmp (bpdu->rcv_cfg.cfg_name ,br->config.cfg_name, MSTP_CONFIG_NAME_LEN) &&
             (bpdu->rcv_cfg.cfg_revision_lvl == br->config.cfg_revision_lvl ))
           {
             if (LAYER2_DEBUG(proto, PROTO_DETAIL))
               {
                 zlog_debug (mstpm, "mstp_process_bpdu: Received cisco information "
                                     "internal to region");
               }
             /* We need to worry about only the internal messages since information
              * received from external to the region are processed correctly
              */
             port->info_internal = PAL_TRUE;
             pal_mem_cpy (br->config.cfg_digest, bpdu->rcv_cfg.cfg_digest, MSTP_CONFIG_DIGEST_LEN);
             port->cisco_cfg_format_id =   bpdu->rcv_cfg.cfg_format_id;
             br->oper_cisco = PAL_TRUE;
           }
         else
           {
              if (LAYER2_DEBUG(proto, PROTO_DETAIL))
                {
                  zlog_debug (mstpm, "mstp_process_bpdu: Received CISCO information"
                                     "external to region");
                }
             port->info_internal = PAL_FALSE;
           }
       }
     else
       {
         if (!pal_mem_cmp (&bpdu->rcv_cfg,&br->config, 52))
           {
             if (LAYER2_DEBUG(proto, PROTO_DETAIL))
               {
                 zlog_debug (mstpm, "mstp_process_bpdu: Received information "
                                     "internal to region");
               }
             port->info_internal = PAL_TRUE;
           }
         else
           {
              if (LAYER2_DEBUG(proto, PROTO_DETAIL))
                {
                  zlog_debug (mstpm, "mstp_process_bpdu: Received information"
                                     "external to region");
                }
             port->info_internal = PAL_FALSE;
           }
       }
    }
  else
    {
      if (LAYER2_DEBUG(proto, PROTO_DETAIL))
        {
          zlog_debug (mstpm, "mstp_process_bpdu: Received information"
                             "external to region");
        }
      port->info_internal = PAL_FALSE;
    }

  /* set tc flags*/
  mstp_check_topology_change (port, bpdu);
      
  /* Set recvd msgs */
  /* hand over the message to cist port info state machine
   * if info internal  then for each incoming msti hand over the message
   * to the msti port information state machine
   * */
  mstp_cist_process_message (port, bpdu);


  if (port->info_internal)
    {

      inst_bpdu = bpdu->instance_list;

      while (inst_bpdu)
        {
          inst_port = mstp_find_msti_port_in_inst_list (port ,
                                            inst_bpdu->instance_id);
          if (inst_port
#if defined (HAVE_PROVIDER_BRIDGE) || defined (HAVE_B_BEB)
               && (inst_port->br->mstp_enabled == PAL_TRUE)
#endif /*(HAVE_PROVIDER_BRIDGE) || (HAVE_B_BEB)*/
             )  
            mstp_msti_process_message (inst_port,inst_bpdu);

          inst_bpdu = inst_bpdu->next;
        }
    }
  else
    {
      inst_port = port->instance_list;

      while (inst_port)
        {
          if (inst_port->rcv_proposal
#if defined (HAVE_PROVIDER_BRIDGE) || defined (HAVE_B_BEB)
              && (inst_port->br->mstp_enabled == PAL_TRUE)
#endif /*(HAVE_PROVIDER_BRIDGE) || (HAVE_B_BEB) */
             )
            {
              mstp_msti_rcv_proposal (inst_port);
              inst_port->rcv_proposal = PAL_FALSE;
              mstp_msti_send_proposal (inst_port);
            }

          inst_port = inst_port->next_inst;
        }
    }

  mstp_tx_bridge (br);

#ifdef HAVE_HA
  mstp_cal_modify_mstp_port (port);
#endif /* HAVE_HA */

  return;
}


/* Release the resources used by inst BPDU */
void
mstp_free_inst_bpdu (struct mstp_bpdu *bpdu)
{
  struct mstp_instance_bpdu *inst_bpdu;
  while ((inst_bpdu = bpdu->instance_list))
    {
      bpdu->instance_list = inst_bpdu->next;

      XFREE (MTYPE_MSTP_BPDU_INST, inst_bpdu);

    }


}

void
mstp_handle_bpdu (struct mstp_port *port, u_char *buf, int len)
{
  struct mstp_bridge *br;
  struct mstp_bpdu bpdu;

  br = port->br;
  br->bpdu_recv_port_ifindex = port->ifindex;

  if (LAYER2_DEBUG(proto, PROTO))
    zlog_debug (mstpm,"mstp_handle_bpdu: Recvd BPDU on port %u",port->ifindex);

#ifdef HAVE_HA
  mstp_cal_modify_mstp_port (port);
#endif /* HAVE_HA */

  /* If the port is an edge port and we are receiving bpdu on it
   * then shutdown the port if bpdu-guard feature is enabled.
   */
  if (port->oper_edge)
    {
      if (port->oper_bpduguard)
        {

#ifdef HAVE_SMI
          smi_capture_mstp_info (SMI_STP_BPDU_GUARD_VIOLATE_SET,
                                 port->name, __LINE__, __FILE__);
#endif /* HAVE_SMI */

          /* shutdown the port as we received a BPDU on it*/
          if ((mstp_nsm_send_port_state (port->ifindex,
                                        MSTP_INTERFACE_SHUTDOWN)) != RESULT_OK)
            {
              if (LAYER2_DEBUG(proto, PROTO))
                zlog_debug (mstpm, "mstp_handle_bpdu: "
                            "Could not shutdown the portfast interface %u on"
                            "bridge %s after receving bpdu", port->ifindex,
                             port->br->name);
             return ;
          }
          zlog_warn (mstpm, "mstp_handle_bpdu: "
                    "Received BPDU on PortFast enable port. shutting down %u"
                    " bridge %s", port->ifindex, port->br->name);

          /* start the errdisable timer if enabled */
          if (port->br->errdisable_timeout_enable)
            {
              port->errdisable_timer =
                                   l2_start_timer (mstp_errdisable_timer_handler,
                                      port, port->br->errdisable_timeout_interval,
                                      mstpm);
#ifdef HAVE_HA
              mstp_cal_create_mstp_errdisable_timer (port);
#endif /* HAVE_HA */
            }
          return ;
        }
        /* If port is a bpdu filer port then simply discard this
         * bpdu without processing.
         */
        else if (port->oper_bpdufilter)
          {
             if (LAYER2_DEBUG(proto, PROTO))
               zlog_debug (mstpm,"rstp_handle_bpdu: Not processing received BPDU"
                           " for bpdu-filter port %u ", port->ifindex);
#ifdef HAVE_SMI
             smi_capture_mstp_info (SMI_STP_BPDU_FILTER_VIOLATE_SET,
                                    port->name, __LINE__, __FILE__);
#endif /* HAVE_SMI */
             return ;
          }
        else
          {
            port->oper_edge = PAL_FALSE;
          }
    }

  /* parse the frame */
#ifdef HAVE_RPVST_PLUS
    /* parse the frame, modify parsing for RPVST */
   if (port->br->type == NSM_BRIDGE_TYPE_RPVST_PLUS)
        mstp_rpvst_plus_parse_bpdu(buf, &bpdu, len, br->admin_cisco, port);
   else if (port->br->type != NSM_BRIDGE_TYPE_RPVST_PLUS)
#endif /* HAVE_RPVST_PLUS */
  mstp_parse_bpdu (buf , &bpdu, len, br->admin_cisco, port);

  /* perform validation checks */
  if (!mstp_is_rcvd_bpdu_valid (port, &bpdu ,len,br->admin_cisco))
    {
      zlog_err (mstpm,"Invalid BPDU recieved on port %u ", port->ifindex);
      return ;
    }

  /* Increment the statistics */
  if ((bpdu.type == BPDU_TYPE_TCN) || bpdu.cist_topology_change)
    port->tcn_bpdu_rcvd++;
  else
    port->conf_bpdu_rcvd++;

#ifdef HAVE_HA
  mstp_cal_delete_mstp_edge_delay_timer (port);
#endif /* HAVE_HA */
  l2_stop_timer (&port->edge_delay_timer);

  port->edge_delay_timer = l2_start_timer (mstp_edge_delay_timer_handler,
                              port, MSTP_MIGRATE_TIME * L2_TIMER_SCALE_FACT,
                              mstpm);

#ifdef HAVE_HA
  mstp_cal_create_mstp_edge_delay_timer (port);
#endif /* HAVE_HA */

  /* start receive state machine */
#ifdef HAVE_RPVST_PLUS
  if (port->br->type == NSM_BRIDGE_TYPE_RPVST_PLUS)
   {
    mstp_process_rpvst_plus_bpdu(port, &bpdu);
    mstp_free_inst_bpdu (&bpdu);
    return;
   }
#endif /* HAVE_RPVST_PLUS */

#ifdef HAVE_L2GP
   if ( port->isL2gp && port->enableBPDUrx)
      mstp_l2gp_chk_bpdu_consistency(port,&bpdu);
   else if ( !port->isL2gp  && (port->br->type != NSM_BRIDGE_TYPE_RPVST_PLUS))
#endif /* HAVE_L2GP */
      mstp_process_bpdu (port ,&bpdu);
   if (pal_mem_cmp (port->dev_addr, &bpdu.cist_bridge_id.addr, 
                    ETHER_ADDR_LEN) == 0)
       port->src_mac_count++;
     port->total_src_mac_count++;
 
    mstp_free_inst_bpdu (&bpdu);
}



inline void
mstp_msti_set_tc (struct mstp_instance_port *inst_port)
{

  if (LAYER2_DEBUG(proto, PROTO_DETAIL))
    {
      zlog_info (mstpm,"mstp_msti_set_tc: "
          "Rcvd tc for port(%u) role (%d) instance (%d)",
          inst_port->ifindex,inst_port->role,inst_port->instance_id);
    }

   if (inst_port->msti_tc_state == TC_INACTIVE)
     {
#ifdef HAVE_HA
       mstp_cal_delete_mstp_inst_tc_timer (inst_port);
#endif /* HAVE_HA */

       inst_port->br->topology_change_detected = PAL_FALSE;
       l2_stop_timer (&inst_port->tc_timer);
       return;
     }

   if ((inst_port->role != ROLE_DESIGNATED) &&
       (inst_port->role != ROLE_ROOTPORT) &&
       (inst_port->role != ROLE_MASTERPORT) )
    {
#ifdef HAVE_HA
      mstp_cal_delete_mstp_inst_tc_timer (inst_port);
#endif /* HAVE_HA */

      /* Inactive State for the MSTI */
      l2_stop_timer (&inst_port->tc_timer);
    }
  else
    {
      /* If Restricted TCN is TRUE - No topology changes are propagated */
      if (inst_port->restricted_tcn == PAL_TRUE)
        {
          zlog_info (mstpm,"mstp_msti_set_tc: "
                     "Restricted TCN set "
                     "Ignoring Rcvd tc for port(%u) instance (%d)",
                      inst_port->ifindex, inst_port->instance_id);
          return;
        }

      mstp_msti_propogate_topology_change (inst_port);
    }

  return;

}


inline void
mstp_cist_set_tc (struct mstp_port *port)
{
  struct mstp_instance_port *inst_port;

  if (LAYER2_DEBUG(proto, PROTO_DETAIL))
    {
      zlog_info (mstpm,"mstp_check_topology_change: "
          "Rcvd tc for port(%u) role (%d) ",
          port->ifindex,port->cist_role);
    }

  if (port->cist_tc_state == TC_INACTIVE)
    return;

  if (port->cist_role == ROLE_DESIGNATED)
    {
      port->tc_ack = PAL_TRUE;
#ifdef HAVE_HA
      mstp_cal_modify_mstp_port (port);
#endif /* HAVE_HA */
    }

  /* If Restricted TCN is TRUE - No topology changes are propagated */
  if (port->restricted_tcn == PAL_TRUE)
    {
      zlog_info (mstpm,"mstp_cist_set_tc: "
                 "Restricted TCN set "
                 "Ignoring Rcvd tc for port(%u)",
                 port->ifindex);
      return;
    }

  mstp_cist_propogate_topology_change (port);

  inst_port= port->instance_list;
  while (inst_port)
    {
      mstp_msti_set_tc (inst_port);
      inst_port = inst_port->next_inst ;
    }

}

/* STP topology change handlers */

void
stp_topology_change_detection (struct mstp_bridge * br)
{
  struct nsm_msg_bridge msg;
  int ret;

#ifdef HAVE_HA
  mstp_cal_modify_mstp_bridge (br);
#endif /* HAVE_HA */

  if ((pal_mem_cmp (&br->cist_designated_root, &br->cist_bridge_id, MSTP_BRIDGE_ID_LEN) == 0))
    {
      if (LAYER2_DEBUG (proto, PROTO))
        zlog_debug (mstpm, "stp_topology_change_detection: "
                    "Bridge %s handling topology change for root",
                    br->name);

#ifdef HAVE_HA
      mstp_cal_delete_mstp_br_tc_timer (br);
#endif /* HAVE_HA */

      /* Section 8.6.14.3.1 */
      br->topology_change = PAL_TRUE;
      /* Section 8.5.3.13 value of topology timer */
      l2_stop_timer (&br->topology_change_timer);
      br->topology_change_timer =
        l2_start_timer (stp_topology_change_timer_handler,
                        br, br->bridge_forward_delay + br->bridge_max_age,
                        mstpm);
#ifdef HAVE_HA
      mstp_cal_create_mstp_br_tc_timer (br);
#endif /* HAVE_HA */
    }
  else if (!br->topology_change_detected)
    {
      /* Section 8.6.14.3.2 */
      if (LAYER2_DEBUG (proto, PROTO))
        zlog_debug (mstpm, "stp_topology_change_detection: "
                    "Bridge %s handling topology change",
                    br->name);

#ifdef HAVE_HA
      mstp_cal_delete_mstp_tcn_timer (br);
#endif /* HAVE_HA */

      stp_transmit_tcn_bpdu (br);
      l2_stop_timer (&br->tcn_timer);
      br->tcn_timer = l2_start_timer (stp_tcn_timer_handler,
                                      br, br->bridge_hello_time, mstpm);
#ifdef HAVE_HA
      mstp_cal_create_mstp_tcn_timer (br);
#endif /* HAVE_HA */
    }
  /* Section 8.6.14.3.3 */
  br->topology_change_detected = PAL_TRUE;

  /* Section 8.5.1.10 */
  ret = mstp_nsm_send_ageing_time( br->name,
                                   br->cist_forward_delay / MSTP_TIMER_SCALE_FACT);
  if (ret != RESULT_OK)
      zlog_warn (mstpm, "stp_topology_change_detection: "
                  "Bridge %s could not set ageing time to forward delay "
                  "on topology change", br->name);

  br->ageing_time_is_fwd_delay = PAL_TRUE;
  /* Send message to NSM about topology change detection . */
  pal_mem_set(&msg, 0, sizeof(struct nsm_msg_bridge));
  pal_strncpy (msg.bridge_name, br->name, NSM_BRIDGE_NAMSIZ);
  nsm_client_send_bridge_msg(mstpm->nc, &msg, NSM_MSG_BRIDGE_TCN);
}

static void
stp_topology_change_acknowledge (struct mstp_port * port)
{
  if (LAYER2_DEBUG (proto, PROTO))
    zlog_debug (mstpm, "stp_topology_change_acknowledge: "
                "Bridge %s port %u acknowledges topology change",
                port->br->name, port->ifindex);
  /* Section 8.6.16.3.1 */
  port->tc_ack = PAL_TRUE;

  /* Section 8.6.16.3.2 */
  port->newInfoCist = PAL_TRUE;
#ifdef HAVE_HA
  mstp_cal_modify_mstp_port (port);
#endif /* HAVE_HA */
  mstp_tx (port);
}

void
mstp_check_topology_change (struct mstp_port *port,
                                struct mstp_bpdu *bpdu)
{

  struct mstp_instance_port *inst_port;
  struct mstp_instance_bpdu *inst_bpdu;
  struct mstp_bridge *br = port->br;

  if (IS_BRIDGE_STP (port->br))
    {
      if (bpdu->type == BPDU_TYPE_TCN)
        {
         if (LAYER2_DEBUG(proto, PROTO_DETAIL))
           {
             zlog_info (mstpm, "mstp_check_topology_change: Received TCN on "
                        "port %u", port->ifindex);
           }
          
         /* Increment TC counters for STP bridge */
          br->tc_initiator = port->ifindex;
          pal_mem_cpy(&br->tc_last_rcvd_from, &bpdu->cist_bridge_id.addr,
                      ETHER_ADDR_LEN);
          br->num_topo_changes++;
          br->time_last_topo_change = pal_time_sys_current (NULL);
          br->total_num_topo_changes++;

          stp_topology_change_detection (port->br);
          stp_topology_change_acknowledge (port);
        }
       
      return;
    }

  /* Inactive state. */

  if  (port->cist_tc_state == TC_INACTIVE)
    {
      if (LAYER2_DEBUG (proto, PROTO_DETAIL))
        zlog_warn (mstpm, "mstp_check_topology_change "
                     "port %u role %d is not root or designated",
                     port->ifindex,port->cist_role);

      port->tc_ack = PAL_FALSE;

#ifdef HAVE_HA
      if (l2_is_timer_running (port->tc_timer))
        mstp_cal_delete_mstp_tc_timer (port);
#endif /* HAVE_HA */

      l2_stop_timer (&port->tc_timer);
      return;
   }

  if ((IS_BRIDGE_STP (br)) && (port->cist_role != ROLE_DESIGNATED))
    {
      zlog_warn (mstpm, "mstp_check_topology_change "
                        "port role is not designated on a STP Bridge");
      return;
    }

  /* Notified_TCN and Notified_TC state. */
  if (bpdu->type == BPDU_TYPE_TCN)
    {
      if (!l2_is_timer_running (port->tc_timer))
        {
          port->tc_timer = l2_start_timer (mstp_tc_timer_handler,
                              port, mstp_cist_get_tc_time (port),mstpm);
#ifdef HAVE_HA
          mstp_cal_create_mstp_tc_timer (port);
#endif /* HAVE_HA */
          port->newInfoCist = PAL_TRUE;
        }

      /* Set rcvd_tc for each and every msti */
      mstp_cist_set_tc (port);

    }

  if (!port->info_internal)
    {
      /* set rcvd_tc for all instances if rcvd_tc is set for cist */
      if (bpdu->cist_topology_change)
        {
          if (LAYER2_DEBUG(proto, PROTO))
            zlog_debug (mstpm, "tc set for cist");

          mstp_cist_set_tc (port);

          /* Updated statistics flags for CIST */
          port->br->topology_change_detected = PAL_TRUE;
          pal_mem_cpy(&port->br->tc_last_rcvd_from, &bpdu->cist_bridge_id.addr, 
                      ETHER_ADDR_LEN);
        }
    }
  else
    {
      /* Info is from the same region
        Set rcvd tc per instance if rcvd tc is set for corrs inst bpdu */

      if (bpdu->cist_topology_change)
        {
          port->br->topology_change_detected = PAL_TRUE;
          pal_mem_cpy(&port->br->tc_last_rcvd_from, &bpdu->cist_bridge_id.addr, 
                      ETHER_ADDR_LEN);

          if (port->cist_role == ROLE_DESIGNATED)
            port->tc_ack = PAL_TRUE;

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
        }

      inst_bpdu = bpdu->instance_list;
      while (inst_bpdu)
        {
          inst_port = mstp_find_msti_port_in_inst_list (port ,
                                            inst_bpdu->instance_id);

          if (inst_port)
            {
              if (inst_bpdu->msti_topology_change)
                {
                  if (LAYER2_DEBUG(proto, PROTO))
                    zlog_debug (mstpm, "tc set for instance %d",
                        inst_port->instance_id);

                  /* Increment TC counters per instance */
                  inst_port->tcn_bpdu_rcvd++;
                  inst_port->br->topology_change_detected = PAL_TRUE;

                  pal_mem_cpy(&inst_port->br->tc_last_rcvd_from, 
                      &inst_bpdu->bridge_id.addr, ETHER_ADDR_LEN);

                  mstp_msti_set_tc (inst_port);
                }
              else
                {
                  /* Increment Config bpdu counters per instance */
                  inst_port->conf_bpdu_rcvd++;
                }
#ifdef HAVE_RPVST_PLUS           
              if (inst_bpdu->msti_topology_change_ack)
                inst_port->br->topology_change_detected = PAL_FALSE;
#endif /* HAVE_RPVST_PLUS */

            }
          inst_bpdu = inst_bpdu->next;
        }
    }

  /* Acknowledged state. */
  if (bpdu->cist_topology_change_ack)
    {
      port->br->topology_change_detected = PAL_FALSE;
#ifdef HAVE_HA
      mstp_cal_delete_mstp_tc_timer (port);
#endif /* HAVE_HA */

      l2_stop_timer ( &port->tc_timer);
      port->tc_ack = PAL_FALSE;

      if (LAYER2_DEBUG(proto,PROTO))
        zlog_info (mstpm,"mstp_check_topology_change:TC ACK stopping tc timer");
    }

#ifdef HAVE_HA
  mstp_cal_modify_mstp_port (port);
#endif /* HAVE_HA */
}


#ifdef HAVE_L2GP

int
mstp_l2gp_prep_and_send_pseudoinfo (struct mstp_port *port)
{

  u_char buf[BR_MST_BPDU_MAX_FRAME_SIZE+VLAN_HEADER_SIZE];
  u_char * bufptr = buf;
  int len;
  struct mstp_bpdu bpdu;
  u_char flags = 0;


  if (!port)
    {
       return PAL_FALSE;
    }
  
  /* Format a config BPDU */
  len = BR_RST_BPDU_FRAME_SIZE;

  /* 
   * just incase, to verify the BPDU getting built correct or not !!!
   * to make sure, correct BPDU is getting generated, instead of sending
   * BPDU onto MSTP state machine, just send it outside from L2GP interface
   * This can be captured in ehtereal  
   * 
   * inorder to ehtereal understand, just add l2_llcFormat(bufptr). 
   * call mstp_port_send() at the end..
   *
   * bufptr = l2_llcFormat (bufptr); 
   *
   */

  *bufptr++ = '\0';
  *bufptr++ = '\0';
  *bufptr++ =  (port->force_version >= BR_VERSION_MSTP) ?
    BR_VERSION_MSTP : BR_VERSION_RSTP;
  *bufptr++ = BPDU_TYPE_RST;

  /* Clear the falgs before setting */
  *bufptr = 0;

  /* 
   * According to 802.1ah 13.26.7 (d).. CIST flags with portrole flags indicating
   * Designated and the learning and forwarding flags set.
   *
   */

   /* setting port role as designated */
   flags |= ( ROLE_DESIGNATED << 2 );

   /* setting port state as learning */
   flags |= (  1 << 4 );

   /* setting port state as forwarding */
   flags |= ( 1  << 5 );

   *bufptr++ = flags;

  /*
   * CIST Root
   */
  *bufptr++ = port->psuedoRootId.prio[0];
  *bufptr++ = port->psuedoRootId.prio[1];

  *bufptr++ = port->psuedoRootId.addr[0];
  *bufptr++ = port->psuedoRootId.addr[1];
  *bufptr++ = port->psuedoRootId.addr[2];
  *bufptr++ = port->psuedoRootId.addr[3];
  *bufptr++ = port->psuedoRootId.addr[4];
  *bufptr++ = port->psuedoRootId.addr[5];

  /*
   * external root pathcost
   */
  *bufptr++ = 0; 
  *bufptr++ = 0;
  *bufptr++ = 0;
  *bufptr++ = 0;

  /*
   * configured bridge priority
   */
  *bufptr++ = port->psuedoRootId.prio[0];
  *bufptr++ = port->psuedoRootId.prio[1];

  /*
   * configured mac-address
   */
  *bufptr++ = port->psuedoRootId.addr[0];
  *bufptr++ = port->psuedoRootId.addr[1];
  *bufptr++ = port->psuedoRootId.addr[2];
  *bufptr++ = port->psuedoRootId.addr[3];
  *bufptr++ = port->psuedoRootId.addr[4];
  *bufptr++ = port->psuedoRootId.addr[5];

  *bufptr++ = 0;
  *bufptr++ = 0;

  /* 
   * Each timer takes two octets. As per 802.1ah spec, message age and max age values
   * are set to be zero.  to be changed in future.
   */
  bufptr = mstp_format_timer (bufptr, port->cist_message_age);
  bufptr = mstp_format_timer (bufptr, port->cist_max_age);

  bufptr = mstp_format_timer (bufptr, port->cist_hello_time);
  bufptr = mstp_format_timer (bufptr, port->cist_fwd_delay);

  *bufptr++ = BR_VERSION_ONE_LEN;


  if (port->force_version >= BR_VERSION_MSTP)
    {
        /* Encode MSTP Parameters */
        len += l2gp_bld_cist_info(port,&bufptr);
    }

  /*
   * since mstp_parse_bpdu & mstp_process_bpdu not returning any value,  
   * no check is being done
   */
  mstp_parse_bpdu (buf , &bpdu, len, 0, port);
  mstp_process_bpdu (port ,&bpdu);

  /*
   * below function can be called to send psuedo BPDU onto the wire
   * can see the BPDU  from ethereal.. just incase requires of
   * debugging...
   * ret = mstp_port_send (port, buf, len);
   */

  port->tx_count++;
  port->newInfoCist = PAL_FALSE;
  port->newInfoMsti = PAL_FALSE;
  port->tc_ack = PAL_FALSE;

  return len;
}

int 
l2gp_bld_cist_info  (struct mstp_port *port, u_char **buf)
{
  struct mstp_bridge *br = port->br;
  struct mstp_instance_port *inst_port;
  u_char *bufptr = *buf;
  short len = BR_MST_MIN_VER_THREE_LEN;

  /* skip the len field as we will only know later */
  bufptr += 2;

  *bufptr++ = br->config.cfg_format_id ;

  pal_mem_cpy (bufptr, br->config.cfg_name,MSTP_CONFIG_NAME_LEN);
  bufptr += MSTP_CONFIG_NAME_LEN;
  *bufptr++ = (br->config.cfg_revision_lvl >> 8) & 0xff;
  *bufptr++ = (br->config.cfg_revision_lvl) & 0xff;
  pal_mem_cpy (bufptr, br->config.cfg_digest,MSTP_CONFIG_DIGEST_LEN);
  bufptr += MSTP_CONFIG_DIGEST_LEN;

  /* copy common info */
  *bufptr++ = 0;
  *bufptr++ = 0;
  *bufptr++ = 0;
  *bufptr++ = 0; 

  *bufptr++ = port->psuedoRootId.prio[0];
  *bufptr++ = port->psuedoRootId.prio[1];

  *bufptr++ = port->psuedoRootId.addr[0];
  *bufptr++ = port->psuedoRootId.addr[1];
  *bufptr++ = port->psuedoRootId.addr[2];
  *bufptr++ = port->psuedoRootId.addr[3];
  *bufptr++ = port->psuedoRootId.addr[4];
  *bufptr++ = port->psuedoRootId.addr[5];

  *bufptr++ = port->hop_count;

  /* for each instance present copy instance specific info */
  for (inst_port = port->instance_list; inst_port ;
       inst_port = inst_port->next_inst)
     {

          l2gp_bld_msti_info(inst_port,&bufptr,port);

          bufptr += MSTI_BPDU_FRAME_SIZE;
          len += MSTI_BPDU_FRAME_SIZE;
     }


  /* calculate the len copied and populate  octes 37-38 */
   *(*buf)++ = (len >> 8) & 0xff;
   *(*buf)++ = len & 0xff;

  /* include the bits to store the len field */
  return (len +2) ;
}


inline void
l2gp_bld_msti_info (struct mstp_instance_port *inst_port, 
                    u_char **buf, struct mstp_port *port )
{
  u_char *bufptr = *buf;
  u_char flags = 0;

  *bufptr = 0;

  /*
   * According to 802.1ah 13.26.7 (f (2)).. MSTI flags with portrole flags indicating
   * Designated and the learning and forwarding flags set.
   *
   */

  /* setting port role as designated */
  flags |= ( ROLE_DESIGNATED << 2 );

   /* setting port state as learning */
  flags |= (  1 << 4 );

  /* setting port state as forwarding */
  flags |= ( 1  << 5 );

  *bufptr++ = flags;

  *bufptr++ = port->psuedoRootId.prio[0];
  *bufptr++ = port->psuedoRootId.prio[1];

  *bufptr++ = port->psuedoRootId.addr[0];
  *bufptr++ = port->psuedoRootId.addr[1];
  *bufptr++ = port->psuedoRootId.addr[2];
  *bufptr++ = port->psuedoRootId.addr[3];
  *bufptr++ = port->psuedoRootId.addr[4];
  *bufptr++ = port->psuedoRootId.addr[5];


  *bufptr++ = 0; 
  *bufptr++ = 0; 
  *bufptr++ = 0; 
  *bufptr++ = 0; 

  *bufptr++ = 0;
  *bufptr++ = 0;

  /* copy rem hops */
  *bufptr++ = inst_port->hop_count;

}


/*
 * as per 802.1ah-d4-1 13.26.6 checkBPDUConsistency
 */
int  mstp_l2gp_chk_bpdu_consistency (struct mstp_port *port, 
                                     struct mstp_bpdu *bpdu )
{

  struct mstp_instance_port *inst_port = NULL; 
  
  if (( better_or_same_info_cist_rcvd(port,bpdu)) &&
       ( ( bpdu->version == BR_VERSION_STP)  ||
       ( ( bpdu->version == BR_VERSION_RSTP ||
       bpdu->version == BR_VERSION_MSTP ) && 
       ( bpdu->cist_port_role == ROLE_DESIGNATED && 
         bpdu->cist_learning ))) )
     {
       port->agreed= PAL_FALSE;
       port->disputed = PAL_TRUE;

       inst_port = port->instance_list;
       
       while(inst_port)
        {
          inst_port->agreed = PAL_FALSE;
          inst_port->disputed = PAL_TRUE;
          inst_port = inst_port->next_inst;
        }
     }
     return PAL_TRUE;
}

#endif /* HAVE_L2GP */
