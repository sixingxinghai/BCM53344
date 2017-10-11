/* Copyright (C) 2008  All Rights Reserved.

 This file contains the functions related rpvst+ */

#include "pal.h"
#include "log.h"
#include "l2lib.h"
#include "l2_debug.h"
#include "l2_timer.h"
#include "l2_llc.h"

#include "mstpd.h"
#include "mstp_config.h"
#include "mstp_types.h"
#include "mstp_main.h"
#include "mstp_transmit.h"
#include "mstp_bpdu.h"
#include "mstp_cist_proto.h"
#include "mstp_msti_proto.h"
#include "mstp_sock.h"
#include "mstp_port.h"
#include "mstp_bridge.h"
#include "mstp_bpdu.h"
#include "mstp_api.h"
#include "mstp_rpvst.h"
#include "table.h"
#include "mstp_rlist.h"


#ifdef HAVE_HA
#include "mstp_cal.h"
#endif /* HAVE_HA */

#ifdef HAVE_RPVST_PLUS
static inline int
mstp_tx_rpvst_sstp(struct mstp_instance_port *inst_port, 
                   struct mstp_port *port);
inline int
rpvst_built_cist_vlan(struct mstp_port *port);

static void 
mstp_check_topology_change_msti(struct mstp_instance_port *inst_port, 
                                struct mstp_instance_bpdu *inst_bpdu);

/* Transmit 802.1D and SSTP bpdus for RPVST bridge equal to mstp_tx_mstp */
int
mstp_tx_rpvst (struct mstp_port *port)
{
  int ret = 0;
  struct mstp_instance_port *inst_port = NULL;

  if (port->rpvst_event == RPVST_PLUS_RCVD_EVENT) 
    {
      if (port->rpvst_bpdu_type == RPVST_PLUS_BPDU_TYPE_CIST) 
        {
          if (port->newInfoCist && port->default_vlan !=
              MSTP_VLAN_DEFAULT_VID)
            {
              ret = rpvst_built_cist(port);
            }
          else if ((port->newInfoCist) &&
              (port->default_vlan == MSTP_VLAN_DEFAULT_VID))
            {
              port->rpvst_bpdu_type = RPVST_PLUS_BPDU_TYPE_SSTP;
              ret = rpvst_built_cist(port);
            }
        }
      else if (port->rpvst_bpdu_type == RPVST_PLUS_BPDU_TYPE_SSTP)
        {
          /* for each instance present copy instance specific info */
          for (inst_port = port->instance_list; inst_port ;
              inst_port = inst_port->next_inst)
            {
              /* send bpdu only for vlan for which this bpdu received */
              if (inst_port->newInfoSstp)
                ret = mstp_tx_rpvst_sstp(inst_port, port);
            }
        }
    }
  else
    {
      if (port->newInfoCist)
        {
          port->rpvst_bpdu_type = RPVST_PLUS_BPDU_TYPE_CIST;
          ret = rpvst_built_cist(port);
          port->default_vlan = MSTP_VLAN_DEFAULT_VID;
          port->rpvst_bpdu_type = RPVST_PLUS_BPDU_TYPE_SSTP;
          ret = rpvst_built_cist(port);
      }

      port->rpvst_bpdu_type = RPVST_PLUS_BPDU_TYPE_SSTP;
      /* for each instance present copy instance specific info */
      for (inst_port = port->instance_list; inst_port ;
           inst_port = inst_port->next_inst)
        {
          if (inst_port->newInfoSstp)
            ret = mstp_tx_rpvst_sstp(inst_port, port);
        }
    }

  port->tc_ack = PAL_FALSE;
  port->rpvst_bpdu_type = 0;
  port->default_vlan = 0;
  port->newInfoCist = PAL_FALSE;

  return ret;
}

/* Send BPDUs for CIST instance */ 
int
rpvst_built_cist (struct mstp_port *port) 
{
  u_char buf[BR_RPVST_BPDU_FRAME_SIZE];
  u_char *bufptr = buf;
  int len;
  int ret = 0;
  int vid = 0;
  struct rpvst_vlan_tlv tlv;

  if (!port)
    return RESULT_ERROR;

  /* Format a config BPDU */
  len = BR_RST_BPDU_FRAME_SIZE;

  if (port->rpvst_bpdu_type == RPVST_PLUS_BPDU_TYPE_CIST)
    {
    bufptr = l2_llcFormat (bufptr);
    len = BR_RST_BPDU_FRAME_SIZE;
    }
  else
    {
      bufptr = l2_llcformat_rpvst(bufptr);
      bufptr = l2_snap_format(bufptr);
      len = BR_RPVST_BPDU_FRAME_SIZE;
    }

  *bufptr++ = '\0';
  *bufptr++ = '\0';
  *bufptr++ = BR_VERSION_RSTP;

  *bufptr++ = BPDU_TYPE_RST;

  /* Clear the flags before setting */ 
  *bufptr = 0;

  /* tc flag bit 1
   * proposal 2
   * port role 3 and 4 
   * learning 5
   * forwarding 6
   * agreement 7
   * tca =0 8 */

  *bufptr++ = ((l2_is_timer_running (port->tc_timer) ? 0x01 : 0) |
      (port->send_proposal << 1) |
      ((port->cist_role & 0x03)  << 2)|
      (((port->cist_state == STATE_LEARNING) || 
        (port->cist_state == STATE_FORWARDING)) << 4) |
      ((port->cist_state == STATE_FORWARDING) << 5) |
      (port->agree << 6));

  *bufptr++ = port->cist_root.prio[0];
  *bufptr++ = port->cist_root.prio[1];
  *bufptr++ = port->cist_root.addr[0];
  *bufptr++ = port->cist_root.addr[1];
  *bufptr++ = port->cist_root.addr[2];
  *bufptr++ = port->cist_root.addr[3];
  *bufptr++ = port->cist_root.addr[4];
  *bufptr++ = port->cist_root.addr[5];
  *bufptr++ = (port->cist_external_rpc >> 24) & 0xff;
  *bufptr++ = (port->cist_external_rpc >> 16) & 0xff;
  *bufptr++ = (port->cist_external_rpc >> 8) & 0xff;
  *bufptr++ = port->cist_external_rpc & 0xff;

  *bufptr++ = port->cist_reg_root.prio[0];
  *bufptr++ = port->cist_reg_root.prio[1];
  *bufptr++ = port->cist_reg_root.addr[0];
  *bufptr++ = port->cist_reg_root.addr[1];
  *bufptr++ = port->cist_reg_root.addr[2];
  *bufptr++ = port->cist_reg_root.addr[3];
  *bufptr++ = port->cist_reg_root.addr[4];
  *bufptr++ = port->cist_reg_root.addr[5];

  *bufptr++ = (port->cist_port_id >> 8) & 0xff;
  *bufptr++ = port->cist_port_id & 0xff;

  /* Each timer takes two octets. */
  bufptr = mstp_format_timer(bufptr, port->cist_message_age);
  bufptr = mstp_format_timer(bufptr, port->cist_max_age);
  bufptr = mstp_format_timer(bufptr, port->cist_hello_time);
  bufptr = mstp_format_timer(bufptr, port->cist_fwd_delay);

  /* Version 1 len */
  *bufptr++ = BR_VERSION_ONE_LEN;

  if (port->rpvst_bpdu_type == RPVST_PLUS_BPDU_TYPE_SSTP)
    { 
      vid = MSTP_VLAN_DEFAULT_VID;

      mstp_rpvst_encode_vlan_tlv(vid, &tlv);

      mstp_rpvst_add_vlan_tlv(bufptr, &tlv);

      port->vid_tag = vid;
      port->default_vlan = 0;
    }

  ret = mstp_port_send(port, buf, len);

 /* Increment the statistics */
  
  if (l2_is_timer_running (port->tc_timer))
    {
      port->br->tc_flag = PAL_TRUE;
      port->tcn_bpdu_sent++;
      port->br->topology_change_detected = PAL_TRUE;
      port->br->tc_initiator = port->ifindex;
    }
  else
    {
      port->br->tc_flag = PAL_FALSE;
      port->conf_bpdu_sent++;
    }

  port->total_tx_count++;
  return ret;
}

/* Built and send SSTP BPDUs  */ 
static int
mstp_tx_rpvst_sstp (struct mstp_instance_port *inst_port, 
                    struct mstp_port *port)
{

  u_char buf[BR_RPVST_BPDU_FRAME_SIZE];
  u_char *bufptr = buf;
  struct rpvst_vlan_tlv tlv;
  struct mstp_bridge_instance *br_instance = NULL;
  short len = BR_MST_MIN_VER_THREE_LEN;
  struct rlist_info *vlan_list = NULL;
  int vid = 0;
  int ret = 0;
  enum port_role role;

  if (!inst_port || !(inst_port->br) || !port)
    return RESULT_ERROR;

  role = (inst_port->role == ROLE_BACKUP) ? ROLE_ALTERNATE : inst_port->role;

  br_instance = inst_port->br;

  if (!br_instance)
    return RESULT_ERROR;

  pal_mem_set(buf, 0, BR_RPVST_BPDU_FRAME_SIZE);

  /* Format a SSTP BPDU */
  len = BR_RPVST_BPDU_FRAME_SIZE;

  bufptr = l2_llcformat_rpvst(bufptr);

  bufptr = l2_snap_format(bufptr);

  /* Vlan tag to be added by HW/SwFwdr */

  *bufptr++ = '\0';   /* 2 bytes protocol Identifier */
  *bufptr++ = '\0';
  *bufptr++ = BR_VERSION_RSTP;
  *bufptr++ = BPDU_TYPE_RST;

  /* Clear the flags before setting */ 
  *bufptr = 0;

  /* tc flag bit 1
   * proposal 2
   * port role 3 and 4 
   * learning 5
   * forwarding 6
   * agreement 7
   * tca =0 8 */

  /* If the port role for the instance is root or designated and 
   * bridge has selected one of its ports as master port OR
   * msti mastered is set for this msti for any other bridge port
   * with designated port role or root port role - set the master flag; */

  *bufptr++ = ((l2_is_timer_running (inst_port->tc_timer) ? 0x01 : 0) |
      (inst_port->send_proposal << 1) |
      ((role & 0x03)  << 2)|
      (((inst_port->state == STATE_LEARNING) || 
        (inst_port->state == STATE_FORWARDING)) << 4) |
      ((inst_port->state == STATE_FORWARDING) << 5) |
      (inst_port->agree << 6)); 

  *bufptr++ = inst_port->msti_root.prio[0];
  *bufptr++ = inst_port->msti_root.prio[1];
  *bufptr++ = inst_port->msti_root.addr[0];
  *bufptr++ = inst_port->msti_root.addr[1];
  *bufptr++ = inst_port->msti_root.addr[2];
  *bufptr++ = inst_port->msti_root.addr[3];
  *bufptr++ = inst_port->msti_root.addr[4];
  *bufptr++ = inst_port->msti_root.addr[5];
  *bufptr++ = (inst_port->internal_root_path_cost >> 24) & 0xff;
  *bufptr++ = (inst_port->internal_root_path_cost >> 16) & 0xff;
  *bufptr++ = (inst_port->internal_root_path_cost >> 8) & 0xff;
  *bufptr++ = inst_port->internal_root_path_cost & 0xff;

  *bufptr++ = inst_port->br->msti_bridge_id.prio[0];
  *bufptr++ = inst_port->br->msti_bridge_id.prio[1];
  *bufptr++ = inst_port->br->msti_bridge_id.addr[0];
  *bufptr++ = inst_port->br->msti_bridge_id.addr[1];
  *bufptr++ = inst_port->br->msti_bridge_id.addr[2];
  *bufptr++ = inst_port->br->msti_bridge_id.addr[3];
  *bufptr++ = inst_port->br->msti_bridge_id.addr[4];
  *bufptr++ = inst_port->br->msti_bridge_id.addr[5];

  *bufptr++ = (inst_port->msti_port_id >> 8) & 0xff;
  *bufptr++ = inst_port->msti_port_id & 0xff;

  /* Each timer takes two octets. */
  bufptr = mstp_format_timer(bufptr, port->cist_message_age);
  bufptr = mstp_format_timer(bufptr, port->cist_max_age);
  bufptr = mstp_format_timer(bufptr, port->cist_hello_time);
  bufptr = mstp_format_timer(bufptr, port->cist_fwd_delay);

  /* version 1 length */
  *bufptr++ = BR_VERSION_ONE_LEN;

  vlan_list = br_instance->vlan_list;
  if (!vlan_list)
    return RESULT_ERROR;

  vid = vlan_list->lo;
  port->vid_tag = vid;

  mstp_rpvst_encode_vlan_tlv(vid, &tlv);

  mstp_rpvst_add_vlan_tlv(bufptr, &tlv);

  ret = mstp_port_send(port, buf, len);

  if (l2_is_timer_running (inst_port->tc_timer))
    {
      inst_port->br->tc_flag = PAL_TRUE;
      inst_port->br->topology_change_detected = PAL_TRUE;
      inst_port->br->tc_initiator = inst_port->ifindex;
      inst_port->tcn_bpdu_sent++;
    }
  else
    {
      inst_port->br->tc_flag = PAL_FALSE;
      inst_port->br->topology_change_detected = PAL_FALSE;
      inst_port->conf_bpdu_sent++;
   }

  port->total_tx_count++;
  inst_port->newInfoSstp = PAL_FALSE;
  inst_port->tc_ack = PAL_FALSE;
  return ret;
}

/* Put tlv into the frame buffer */
void 
mstp_rpvst_add_vlan_tlv (u_char *buf, struct rpvst_vlan_tlv *tlv)
{
  /* TLV type */
  *buf++ = (tlv->tlv_type >> 8) & 0xff ;
  *buf++ = (tlv->tlv_type) & 0xff ;

  /* TLV Length */
  *buf++ = (tlv->tlv_length >> 8) & 0xff ;
  *buf++ = (tlv->tlv_length) & 0xff ;

  /* TLV Data */
  *buf++ = (tlv->vlan_id >> 8) & 0xff ;
  *buf++ = (tlv->vlan_id) & 0xff ;

}

/* Add tlv to the struct */
void
mstp_rpvst_encode_vlan_tlv (u_int16_t vid, struct rpvst_vlan_tlv *tlv)
{
  pal_mem_set (tlv, 0, sizeof (struct rpvst_vlan_tlv));

  tlv->tlv_type = 0;
  tlv->tlv_length = MSTP_RPVST_VLAN_TLV_LEN;
  tlv->vlan_id = vid;

}

int 
mstp_rpvst_send (struct mstp_port *port, unsigned char *data, int length)
{
  s_int32_t ret = 0;

  if (port->rpvst_bpdu_type == RPVST_PLUS_BPDU_TYPE_CIST)
    {
      ret = mstp_send(port->br->name, port->dev_addr, bridge_grp_add,
          MSTP_VLAN_NULL_VID, port->ifindex, data, length);
    }
  else if (port->rpvst_bpdu_type == RPVST_PLUS_BPDU_TYPE_SSTP)
    {
      /* Send Tagged SSTP BPDUs */
      if (port->port_type == MSTP_VLAN_PORT_MODE_HYBRID 
          || port->port_type == MSTP_VLAN_PORT_MODE_TRUNK
          || port->port_type == MSTP_VLAN_PORT_MODE_ACCESS)
        {
          ret = mstp_send(port->br->name, port->dev_addr, 
                          rpvst_bridge_grp_addr,
                          port->vid_tag, port->ifindex, data, length);
        }
    }

  return ret;
}

/* RX part equivalent to mstp_parse_bpdu *
* Translate RPVST Bpdus to mstp bpdus  and handover to MSTP state machine*/
void
mstp_rpvst_plus_parse_bpdu (u_char *buf, struct mstp_bpdu *bpdu, 
                            int len, unsigned char admin_cisco, 
                            struct mstp_port *port) 
{
  struct mstp_instance_bpdu *inst_bpdu = NULL; 

  /* We skip the LLC as it has already been checked */
  register u_char *bufptr = buf;

  /*clear memory to be on safer side */
  pal_mem_set (bpdu, 0, sizeof (struct mstp_bpdu));

  bpdu->proto_id = *bufptr++ << 8 ;
  bpdu->proto_id += *bufptr++;

  bpdu->version  = *bufptr++;
  bpdu->type     = *bufptr++;

  /* If its is a TCN, then there is nothing else to parse */
  if (bpdu->type == BPDU_TYPE_TCN)
    return;

  if (port->rpvst_bpdu_type == RPVST_PLUS_BPDU_TYPE_CIST)
    { 
       bpdu->cist_topology_change = (*bufptr & 0x01);

      if (bpdu->version >= BR_VERSION_RSTP)
        {            
          bpdu->cist_proposal = (*bufptr & (0x01 << 1)) ? PAL_TRUE : PAL_FALSE ;
          bpdu->cist_port_role = ((*bufptr & (0x03 << 2)) >> 2) ;
          bpdu->cist_learning = (*bufptr & (0x01 << 4)) ? PAL_TRUE : PAL_FALSE;
          bpdu->cist_forwarding = (*bufptr & (0x01 << 5)) ? PAL_TRUE : 
            PAL_FALSE;
          bpdu->cist_agreement = (*bufptr & (0x01 << 6)) ? PAL_TRUE : PAL_FALSE;
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

     if (LAYER2_DEBUG(proto, PROTO_DETAIL))
       {
         zlog_debug(mstpm,"Parse " 
                    "Root  (%.02x%.02x.%.02x%.02x.%.02x%.02x) "
                    "Reg_root(%.02x%.02x.%.02x%.02x.%.02x%.02x) ",
                    bpdu->cist_root.addr[0],  bpdu->cist_root.addr[1],
                    bpdu->cist_root.addr[2],  bpdu->cist_root.addr[3],
                    bpdu->cist_root.addr[4],  bpdu->cist_root.addr[5],
                    bpdu->cist_reg_root.addr[0],  bpdu->cist_reg_root.addr[1], 
                    bpdu->cist_reg_root.addr[2],  bpdu->cist_reg_root.addr[3], 
                    bpdu->cist_reg_root.addr[4],  bpdu->cist_reg_root.addr[5]); 
       }

      /* Parse Message Times variable . */
      bpdu->message_age = mstp_parse_timer(bufptr); 
      bufptr+=2;

      bpdu->max_age = mstp_parse_timer(bufptr); 
      bufptr+=2;

      bpdu->hello_time = mstp_parse_timer(bufptr); 
      bufptr += 2;

      bpdu->fwd_delay = mstp_parse_timer(bufptr); 
      bufptr += 2;

      bpdu->version_one_len = *bufptr++;
      pal_mem_cpy(&bpdu->cist_bridge_id, &bpdu->cist_reg_root, 
          sizeof (struct bridge_id));     

      bpdu->hop_count = MSTP_DEFAULT_HOP_COUNT;

    }
  else if (port->rpvst_bpdu_type == RPVST_PLUS_BPDU_TYPE_SSTP)
    {
      if (LAYER2_DEBUG(proto, PROTO_DETAIL))
        {
          zlog_debug(mstpm,"mstp_rpvst_plus_parse_bpdu: "
                     "Processing sstp BPDUs");
        }

      /* Anyhow only one instance info will be present in the BPDU */
      inst_bpdu = XCALLOC(MTYPE_MSTP_BPDU_INST,
                  sizeof (struct mstp_instance_bpdu)) ;

      if (!inst_bpdu)
          return;

      bpdu->instance_list = inst_bpdu;

      /* Move bufptr till TC bit */
      inst_bpdu->msti_topology_change = (*bufptr & 0x01);
      inst_bpdu->msti_proposal = (*bufptr & (0x01 << 1)) ? PAL_TRUE : PAL_FALSE;
      inst_bpdu->msti_port_role = ((*bufptr & (0x03 << 2)) >> 2) ;
      inst_bpdu->msti_learning = (*bufptr & (0x01 << 4)) ? PAL_TRUE : PAL_FALSE;
      inst_bpdu->msti_forwarding = (*bufptr & (0x01 << 5)) ? 
        PAL_TRUE : PAL_FALSE;
      inst_bpdu->msti_agreement = (*bufptr & (0x01 << 6)) ? 
        PAL_TRUE : PAL_FALSE; 
      inst_bpdu->msti_topology_change_ack = ((*bufptr++ & (0x01 << 7)) ? 1 :0);

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

      /* The bpdu will be in cisco format 
       * (Take bridge id instead of br priority) */

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

      /* Parse Message Times variable . */
      inst_bpdu->message_age = mstp_parse_timer (bufptr); 
      bufptr+=2;
      inst_bpdu->max_age = mstp_parse_timer(bufptr); 
      bufptr+=2;

      inst_bpdu->hello_time = mstp_parse_timer(bufptr); 
      bufptr += 2;
      inst_bpdu->fwd_delay = mstp_parse_timer(bufptr);
      bufptr += 2;

      inst_bpdu->version_one_len = *bufptr++;
      inst_bpdu->rem_hops = MSTP_DEFAULT_HOP_COUNT;

      /* Get Vid from the TLV field */
      /* Based on VID get instance info from the instance list. */
      /* Need to check for TLV type ?? */

      bufptr += 4; 
      inst_bpdu->vlan_id = *bufptr++ << 8; 
      inst_bpdu->vlan_id += *bufptr++; 


      if (inst_bpdu->vlan_id == MSTP_VLAN_DEFAULT_VID)
        port->default_vlan = PAL_TRUE;

      if (LAYER2_DEBUG(proto, PROTO_DETAIL))
        {
          zlog_debug (mstpm, "mstp_rpvst_plus_parse_bpdu: MSTI PARSE "
              "Root (%.02x%.02x.%.02x%.02x.%.02x%.02x) "
              "Designated Br Id(%.02x%.02x.%.02x%.02x.%.02x%.02x) ",
             inst_bpdu->msti_reg_root.addr[0], inst_bpdu->msti_reg_root.addr[1],
             inst_bpdu->msti_reg_root.addr[2], inst_bpdu->msti_reg_root.addr[3],
             inst_bpdu->msti_reg_root.addr[4], inst_bpdu->msti_reg_root.addr[5],
             inst_bpdu->bridge_id.addr[0], inst_bpdu->bridge_id.addr[1], 
             inst_bpdu->bridge_id.addr[2], inst_bpdu->bridge_id.addr[3], 
             inst_bpdu->bridge_id.addr[4], inst_bpdu->bridge_id.addr[5]); 
        }

      /* There is no remaining hops field in rpvst+*/

      if (LAYER2_DEBUG(proto, PROTO_DETAIL))
        {
          zlog_debug(mstpm,"mstp_rpvst_plus_parse_bpdu:" 
              "msti port id (%x)", inst_bpdu->msti_port_id);

          zlog_debug(mstpm,"mstp_rpvst_plus_parse_bpdu: "
              "msti prio (%0X) mist portid (%0x)",
              inst_bpdu->msti_portid_prio, 
              inst_bpdu->msti_port_id);
          zlog_debug(mstpm,"mstp_rpvst_plus_parse_bpdu: " 
              "Parsing Vlan %d", inst_bpdu->vlan_id) ;
        }

    }
  return;
}

/* This function implements the Port Receive State Machine 
* RPVST */
void
mstp_process_rpvst_plus_bpdu (struct mstp_port *port, struct mstp_bpdu *bpdu)
{
  struct mstp_bridge *br =  NULL;
  struct mstp_instance_bpdu *inst_bpdu = NULL;
  struct mstp_instance_port *inst_port = NULL;

  pal_assert (port);
  br = port->br;

  if (!br)
    return;

  /* update bpdu version */
  if (!l2_is_timer_running(port->migrate_timer))
    {
      /* Update BPDU version */
      port->rcvd_mstp = ((bpdu->type == BPDU_TYPE_RST) 
          && (port->force_version >= BR_VERSION_RSTP));

      port->rcvd_rstp = ((bpdu->version == BR_VERSION_RSTP)
          && (port->force_version >= BR_VERSION_RSTP));


      /* An unknown value of port role cannot be generated by a 
         valid implementation. Hence we treat it as a Config BPDU. */  
      port->rcvd_stp = ((bpdu->type == BPDU_TYPE_CONFIG) ||
          (bpdu->type == BPDU_TYPE_TCN) ||
          ((bpdu->type < BPDU_TYPE_RST) 
           && bpdu->cist_port_role == ROLE_UNKNOWN));

      
      if (LAYER2_DEBUG(proto, PROTO_DETAIL))
        {
          zlog_debug(mstpm,"mstp_process_rpvst_plus_bpdu: "
              "bpdu->type=%d bpdu->cist_port_role=%d", 
              bpdu->type, bpdu->cist_port_role);
          zlog_debug(mstpm,"mstp_process_rpvst_plus_bpdu: "
              "port->rcvd_mstp=%d port->send_mstp=%d", 
              port->rcvd_mstp, port->send_mstp);
          zlog_debug(mstpm,"mstp_process_rpvst_plus_bpdu: "
              "port->version=%d", port->force_version);
        }
    }


  if (port->send_mstp != port->rcvd_mstp)
    {
      port->send_mstp ? mstp_sendstp (port) : mstp_sendmstp (port);
    }  

  if (LAYER2_DEBUG(proto, PROTO_DETAIL))
    {
      zlog_debug(mstpm,"mstp_process_rpvst_plus_bpdu: " 
                 "port->send_mstp (%d) port->rcvd_mstp(%d)", 
                 port->send_mstp, port->rcvd_mstp);
    }
  /* hand over the message to cist port info state machine
   * if info internal then for each incoming msti hand over the 
   * message to the msti port information state machine   
   */
   port->rpvst_event = RPVST_PLUS_RCVD_EVENT;

  if (port->rpvst_bpdu_type  == RPVST_PLUS_BPDU_TYPE_CIST)
    {
      if (LAYER2_DEBUG(proto, PROTO_DETAIL))
        {
          zlog_debug(mstpm,"mstp_process_rpvst_plus_bpdu: "
                     "processing CIST BPDU: %d", port->rpvst_bpdu_type);
        }
      port->info_internal = PAL_TRUE;

      mstp_check_topology_change(port, bpdu);

      mstp_cist_process_message(port, bpdu);

      mstp_tx_bridge (br); 

      return;

    }
  else if (port->rpvst_bpdu_type == RPVST_PLUS_BPDU_TYPE_SSTP)
    {
      if (LAYER2_DEBUG(proto, PROTO_DETAIL))
        {
          zlog_debug(mstpm,"mstp_process_rpvst_plus_bpdu: "
                     "processing SSTP BPDU");
        }
      port->info_internal = PAL_TRUE;
      if (port->info_internal)
        {
          inst_bpdu = bpdu->instance_list;

          if (inst_bpdu)
            {
              inst_port = mstp_find_msti_port_in_vlan_list(port, inst_bpdu);
              if (!inst_port)
                {
                  return;
                }

              if (LAYER2_DEBUG(proto, PROTO_DETAIL))
                {
                  zlog_debug(mstpm, "mstp_process_rpvst_plus_bpdu: "
                          "inst_port instance %d\n", inst_port->vlan_id);
                }

                mstp_check_topology_change_msti(inst_port, inst_bpdu);

                mstp_msti_process_message(inst_port,inst_bpdu);

            }
        }
      else
        {
          inst_port = port->instance_list;
          
          while (inst_port)
            {
              if (inst_port->rcv_proposal)
                {
                  mstp_msti_rcv_proposal (inst_port);
                  inst_port->rcv_proposal = PAL_FALSE;
                  mstp_msti_send_proposal (inst_port);
                }
              inst_port->conf_bpdu_rcvd++;
              inst_port = inst_port->next_inst;
            }
        }
      mstp_tx_bridge (br); 

    } /* end of rpvst check */

#ifdef HAVE_HA
  mstp_cal_modify_mstp_port(port);
#endif /* HAVE_HA */

  return;
}


/* Internal function to dump the frame */
void
hexdump (unsigned char *buf, int nbytes)
{
  int i, j;

  for (i = 0; i < nbytes; i += 16) 
    {
      printf("%04X : ", i);
      for (j = 0; j < 16 && i + j < nbytes; j++)
        printf("%2.2X ", buf[i + j]);
      for (; j < 16; j++) {
          printf("   ");
      }
      printf(": ");
      for (j = 0; j < 16 && i + j < nbytes; j++) {
          char c = toascii(buf[i + j]);
          printf("%c", isalnum(c) ? c : '.');
      }
      printf("\n");
    }
}

static  void 
mstp_check_topology_change_msti (struct mstp_instance_port *inst_port, 
                                 struct mstp_instance_bpdu *inst_bpdu)
{
  /* Info is from the same region
   * Set rcvd tc per instance if rcvd tc is set for corrs inst bpdu */

  if ((inst_port == NULL) || (inst_bpdu == NULL))
    return;

  if ((inst_port) && (inst_bpdu->msti_topology_change))
    {
      inst_port->tcn_bpdu_rcvd++;
      inst_port->br->topology_change_detected = PAL_TRUE;

      pal_mem_cpy(&inst_port->br->tc_last_rcvd_from, 
                  &inst_bpdu->bridge_id.addr, ETHER_ADDR_LEN);

      if (LAYER2_DEBUG(proto, PROTO))
        zlog_debug (mstpm, "mstp_check_topology_change_msti: "
            "tc set for instance %d", 
            inst_port->instance_id);
      mstp_msti_set_tc (inst_port);
    }
  else
    inst_port->conf_bpdu_rcvd++;

  /* Acknowledged state. */
  if (inst_bpdu->msti_topology_change_ack)
    {
#ifdef HAVE_HA
      mstp_cal_delete_mstp_tc_timer (inst_port->cst_port);
#endif /* HAVE_HA */

      l2_stop_timer ( &inst_port->tc_timer);
      inst_port->tc_ack = PAL_FALSE;
      inst_port->br->topology_change_detected = PAL_FALSE;

      if (LAYER2_DEBUG(proto,PROTO))        
        zlog_info (mstpm,"mstp_check_topology_change_msti: "
                    "TC ACK stopping tc timer");
    }

#ifdef HAVE_HA
  mstp_cal_modify_mstp_port (inst_port->cst_port);
#endif /* HAVE_HA */
}

/* APIs for RPVST+ CLIS */

/* Put a vlan in a spanning-tree instance */ 
int
rpvst_plus_api_add_vlan(char *bridge_name, int vid)
{

  struct mstp_bridge *br = NULL;
  int ret = 0;
  int instance = 0;
  struct mstp_vlan *vlan = NULL;

  br = mstp_find_bridge (bridge_name);

  if (! br)
    {
      ret = MSTP_ERR_BRIDGE_NOT_FOUND;
      return ret;
    }

  if (! IS_BRIDGE_RPVST_PLUS (br))
    {
      ret = MSTP_ERR_NOT_RPVST_BRIDGE;
      return ret;
    }

  vlan = mstp_bridge_vlan_lookup (br->name, vid);
  if (!vlan)
    {
      ret = MSTP_ERR_RPVST_BRIDGE_NO_VLAN;
      return ret;
    }

#ifdef HAVE_PVLAN
  if ((vlan)
      && (vlan->pvlan_type == MSTP_PVLAN_SECONDARY))
    return RESULT_ERROR;
#endif /* HAVE_PVLAN */

  MSTP_SET_VLAN_INSTANCE_MAP(br->vlan_instance_map, vid, instance);

  if (instance == 0)
    {
      ret = MSTP_ERR_RPVST_BRIDGE_MAX_VLAN;
      return ret;
    }
  if (instance < 0)
    {
      ret = MSTP_ERR_RPVST_BRIDGE_VLAN_EXISTS;
      return ret;
    }

  vlan->instance = instance;

  ret = mstp_api_add_instance(bridge_name, instance, vid, 0);

  return ret;
}

void 
rpvst_plus_bridge_vlan_config_add (char *bridge_name, u_int16_t vid)
{
  struct mstp_bridge_info *br_config_info = NULL;
  struct mstp_bridge_config_list *br_conf_list = NULL;
  struct mstp_bridge_instance_info *br_inst_info = NULL;
  struct mstp_bridge_instance_list *br_inst_list = NULL;
  struct mstp_bridge *br = NULL;
  struct mstp_prefix_vlan p;
  struct route_node *rn = NULL;
  struct mstp_vlan *v = NULL;
  int instance = 0;
  u_int16_t invalid_range_idx = 0;

  br = mstp_find_bridge (bridge_name);

  if (br && br->vlan_table)
    {
      MSTP_SET_VLAN_INSTANCE_MAP(br->vlan_instance_map, vid,
	                         instance);
      if (instance < 0)
        {
	  return;
        }

      MSTP_PREFIX_VLAN_SET (&p, vid);
      rn = route_node_lookup (br->vlan_table, (struct prefix *) &p);
      if (rn)
        {
	  v = rn->info;
	  route_unlock_node (rn);
	  if (v && (v->vid == vid) && (v->instance != instance))
	      return ;
        }
    }

  br_config_info = mstp_bridge_config_info_get (bridge_name);

  if (!br_config_info)
    {
      br_conf_list = mstp_bridge_config_list_new (bridge_name);
      if (br_conf_list == NULL)
        {
	  return;
        }
      mstp_bridge_config_list_link (br_conf_list);
      br_config_info = &(br_conf_list->br_info);
    }

  MSTP_SET_VLAN_INSTANCE_MAP(br_config_info->vlan_instance_map, vid,
	                     instance);

  if (instance <= 0)
    return;

  br_inst_info = mstp_br_config_find_instance (br_config_info, instance);

  if (!br_inst_info)
    {
      br_inst_list = mstp_br_config_instance_new (instance);
      if (br_inst_list == NULL)
        return;

      mstp_br_config_link_instance_new (br_config_info, br_inst_list);
      br_inst_info = & (br_inst_list->instance_info);
    }

  if (vid)
    {
      br_inst_info->vlan_id = vid;
      mstp_rlist_delete (&br_inst_info->vlan_list, vid, &invalid_range_idx);
      mstp_rlist_add (&br_inst_info->vlan_list, vid, 0);
      UNSET_FLAG (br_inst_info->config, MSTP_BRIDGE_CONFIG_INSTANCE);
      SET_FLAG (br_inst_info->config, MSTP_BRIDGE_CONFIG_VLAN_INSTANCE);
      SET_FLAG (br_config_info->config, MSTP_BRIDGE_CONFIG_VLAN_INSTANCE);
    }
  else
    {
      SET_FLAG (br_inst_info->config, MSTP_BRIDGE_CONFIG_INSTANCE);
    }
}

/* Delete vlan from the spanning tree */
int
rpvst_plus_api_vlan_delete (char *bridge_name, int vid)
{

  int instance = 0;
  int ret = 0;
  struct mstp_bridge * br = NULL;
  struct mstp_vlan *v = NULL; 

  br = mstp_find_bridge (bridge_name);

  if ((!br) || (! IS_BRIDGE_RPVST_PLUS(br)))
    {
      ret = MSTP_ERR_BRIDGE_NOT_FOUND;
      return ret;
    }

  v = mstp_bridge_vlan_lookup (bridge_name, vid);

  if (! v)
    {
      ret = MSTP_ERR_RPVST_BRIDGE_NO_VLAN;
      return ret;
    }

#ifdef HAVE_PVLAN
  if (v->pvlan_type == MSTP_PVLAN_SECONDARY)
    {
      return CLI_ERROR;
    }
#endif /* HAVE_PVLAN */

  MSTP_GET_VLAN_INSTANCE_MAP(br->vlan_instance_map, vid, instance);
  if (instance <= 0)
    {
      return RESULT_ERROR;
    } 

  MSTP_DEL_VLAN_INSTANCE_MAP(br->vlan_instance_map, vid, instance);

  if (ret < 0)
    {
      return RESULT_ERROR;
    }

  if (CHECK_FLAG (v->flags, MSTP_VLAN_INSTANCE_CONFIGURED))
    {
  /* Instance not alive. Delete VLAN. */
      mstp_bridge_vlan_delete (bridge_name, vid);
      return RESULT_OK;
    }
  /* Delete this instance from VLAN config if present. */
  mstp_bridge_vlan_instance_delete (bridge_name, instance);

  if (mstp_api_delete_instance (bridge_name, instance) != RESULT_OK)
    {
      ret = MSTP_ERROR_GENERAL;
      return ret;
    }

  return ret;

}

/* Set priority of a vlan in a spanning-tree topology */
int
rpvst_plus_api_set_msti_bridge_priority(char *bridge_name,
                                        int vid, u_int32_t priority)
{
  struct mstp_bridge *br = NULL;
  int ret = 0; 
  int instance = 0;

  br = mstp_find_bridge (bridge_name);

  if (! IS_BRIDGE_RPVST_PLUS(br))
    {
     ret = MSTP_ERR_NOT_RPVST_BRIDGE;
     return ret;
    }

  MSTP_GET_VLAN_INSTANCE_MAP(br->vlan_instance_map, vid, instance);

  if (instance <= 0)
    {
      ret = MSTP_ERR_RPVST_BRIDGE_NO_VLAN;
      return ret;
    }

  ret = mstp_api_set_msti_bridge_priority (bridge_name, instance, priority);

  return ret;

}

/* Set restricted_tcn on a vlan */
int
rpvst_plus_api_set_msti_vlan_restricted_tcn(char *br_name, char *ifname,
                                                int vid, bool_t flag)
{
  struct mstp_bridge *br = NULL;
  int instance = 0;
  int ret;
  struct interface *ifp;
  struct apn_vr *vr = apn_vr_get_privileged (mstpm);
  u_int32_t ifindex;
  struct mstp_bridge_instance *br_inst = NULL;

  ifp = if_lookup_by_name (&vr->ifm, ifname);
  if (!ifp)
	  return RESULT_ERROR;

  ifindex = ifp->ifindex;

  br = mstp_find_bridge (br_name);

  if (br == NULL)
    return RESULT_ERROR;

  MSTP_GET_VLAN_INSTANCE_MAP(br->vlan_instance_map, vid, instance);

  if (instance <= 0)
    {
      ret = MSTP_ERR_RPVST_VLAN_CONFIG_ERR;
      return ret;
    }

  if ((instance >= br->max_instances) ||
		  (!(br_inst = br->instance_list[instance])))
    {
      ret = MSTP_ERR_RPVST_VLAN_CONFIG_ERR;
      return ret;
    }

  ret = mstp_api_set_msti_instance_restricted_tcn(ifp->bridge_name, 
		                                  ifp->name, instance, 
                                                  PAL_TRUE);
  return ret;

}

/* Set restricted_Role on a vlan */
int
rpvst_plus_api_set_msti_vlan_restricted_role(char *br_name, char *ifname,
                                                int vid, bool_t flag)
{
  struct mstp_bridge *br = NULL;
  int instance = 0;
  int ret;
  struct interface *ifp = NULL;
  struct apn_vr *vr = apn_vr_get_privileged (mstpm);
  u_int32_t ifindex;
  struct mstp_bridge_instance *br_inst = NULL;

  ifp = if_lookup_by_name (&vr->ifm, ifname);
    if (!ifp)
      return RESULT_ERROR;

  ifindex = ifp->ifindex;

  br = mstp_find_bridge (br_name);

  if (br == NULL)
    {
     ret = MSTP_ERR_BRIDGE_NOT_FOUND;
    return ret;
    }

  MSTP_GET_VLAN_INSTANCE_MAP(br->vlan_instance_map, vid, instance);

  if (instance <= 0)
    {
      ret = MSTP_ERR_RPVST_BRIDGE_NO_VLAN;
      return ret;
    }

  if ((instance >= br->max_instances) ||
      (!(br_inst = br->instance_list[instance])))
    {
      ret = MSTP_ERR_RPVST_VLAN_CONFIG_ERR;
      return ret;
    }
   
  ret = mstp_api_set_msti_instance_restricted_role(ifp->bridge_name, 
                                                 ifp->name, instance, PAL_TRUE);
  return ret;

}

/* Put a port as a of part of spanning-tree instance of a vlan */
int
rpvst_plus_api_add_port(char *br_name, char *ifname, u_int16_t vid,
                        u_int8_t spanning_tree_disable)
{
  struct apn_vr *vr = apn_vr_get_privileged (mstpm);
  struct mstp_bridge *br = NULL;
  struct interface *ifp = NULL;
  int ret = 0;
  int instance = 0;
  struct mstp_interface *mstpif = NULL;
  struct port_instance_list *pinst_list = NULL; 
  char null_str[L2_BRIDGE_NAME_LEN];


  ifp = if_lookup_by_name (&vr->ifm, ifname);

  if (!ifp)
    return ret;

  mstpif = ifp->info;

  if (!mstpif)
    return RESULT_ERROR;

  br = mstp_find_bridge(br_name);

  if (!br) 
    {
      ret = MSTP_ERR_NOT_RPVST_BRIDGE;
      return ret;
    }

  if (!IS_BRIDGE_RPVST_PLUS (br))
    {
      ret = MSTP_ERR_NOT_RPVST_BRIDGE;
      return ret;
    }

  MSTP_GET_VLAN_INSTANCE_MAP(br->vlan_instance_map, vid, instance);

  if (instance <= 0)
    {
      ret = MSTP_ERR_RPVST_VLAN_CONFIG_ERR;
      return ret;
    }

  if (! mstp_find_cist_port (mstp_find_bridge (br_name), ifp->ifindex) )
    {
      pinst_list = XCALLOC(MTYPE_PORT_INSTANCE_LIST,
	                   sizeof (struct port_instance_list));

  if (pinst_list == NULL)
    {
      ret = MSTP_ERR_RPVST_VLAN_MEM_ERR;
      return ret;
    }

    pinst_list->instance_info.instance = instance;
    pinst_list->instance_info.vlan_id = vid;
    mstpif_config_add_instance(mstpif, pinst_list);

    if (mstpif)
    {
      pal_mem_set(null_str, '\0', L2_BRIDGE_NAME_LEN);

      if ((pal_strncmp(mstpif->bridge_group, null_str,
	      L2_BRIDGE_NAME_LEN) !=0)
	  && (pal_strncmp(mstpif->bridge_group, br_name,
          L2_BRIDGE_NAME_LEN) != 0))
        {
	  ret = MSTP_ERR_RPVST_VLAN_BR_GR_ADD;
	  return ret;
        }
    }

    pal_strcpy(mstpif->bridge_group, br_name);
    SET_FLAG(mstpif->config, MSTP_IF_CONFIG_BRIDGE_GROUP_INSTANCE);
    ret = MSTP_ERR_RPVST_VLAN_BR_GR_ASSOCIATE;
    return ret;
  }

  ret = mstp_api_add_port(br_name, ifp->name, vid,
                          instance, PAL_FALSE);

  if (ret < 0)
    {
      mstp_br_config_instance_add_port(br_name, ifp, instance);
      return ret;
    }

  return ret;
}

/* Remove a port from vlan of a spanning-tree */
int
rpvst_plus_delete_port(char *br_name, char *ifname, u_int16_t vid, 
                       int force, bool_t notify_fwd)
{
  struct mstp_bridge *br = NULL;
  int ret = 0; 
  int instance = 0;
  struct interface *ifp = NULL;
  struct apn_vr *vr = apn_vr_get_privileged (mstpm); 

  br = mstp_find_bridge (br_name);

  if (!br)
    {
      ret = MSTP_ERR_BRIDGE_NOT_FOUND;
      return ret;
    }

  MSTP_GET_VLAN_INSTANCE_MAP(br->vlan_instance_map, vid, instance);

  if (instance <= 0)
    {
      ret = MSTP_ERR_RPVST_VLAN_CONFIG_ERR;
      return ret;
    }

  ifp = if_lookup_by_name (&vr->ifm, ifname);

  if (!ifp)
    return RESULT_ERROR;

  ret = mstp_delete_port(br_name, ifp->name, MSTP_VLAN_NULL_VID,
                         instance, PAL_FALSE, PAL_TRUE);

  mstp_bridge_port_instance_config_delete(ifp, br_name, instance);

  return ret;

}

/* Set priority of a port on a vlan */
int
rpvst_plus_api_set_msti_port_priority (char *br_name, 
                                       char *ifname, int vid,
                                       s_int16_t priority)
{
  struct mstp_bridge *br = NULL;
  struct interface *ifp = NULL;
  struct apn_vr *vr = apn_vr_get_privileged (mstpm);
  u_int32_t ifindex;
  int instance = 0;
  int ret = 0;

  ifp = if_lookup_by_name (&vr->ifm, ifname);
  if (!ifp)
    return RESULT_ERROR;

  ifindex = ifp->ifindex;

  br = mstp_find_bridge (br_name);

  if (br == NULL) 
    return MSTP_ERR_BRIDGE_NOT_FOUND;

  MSTP_GET_VLAN_INSTANCE_MAP(br->vlan_instance_map, vid, instance);

  if (instance <= 0)
    {
      ret = MSTP_ERR_RPVST_VLAN_CONFIG_ERR;
      return ret;
    }

  ret = mstp_api_set_msti_port_priority(br_name, ifp->name,
                                        instance,  priority);
  return ret;

}


/* Set priority of a port on a vlan */
int
rpvst_plus_api_set_msti_port_path_cost (char *br_name,
                                        char *ifname, int vid,
                                        u_int32_t path_cost)
{
  struct mstp_bridge *br = NULL;
  struct interface *ifp = NULL;
  struct apn_vr *vr = apn_vr_get_privileged (mstpm);
  u_int32_t ifindex;
  int instance = 0;
  int ret = 0;

  ifp = if_lookup_by_name (&vr->ifm, ifname);
  if (!ifp)
      return RESULT_ERROR;

  ifindex = ifp->ifindex;

  br = mstp_find_bridge (br_name);
  if (!br)
      return MSTP_ERR_BRIDGE_NOT_FOUND;

  MSTP_GET_VLAN_INSTANCE_MAP(br->vlan_instance_map, vid, instance);

  if (instance <= 0)
    {
      ret = MSTP_ERR_RPVST_VLAN_CONFIG_ERR;
      return ret;
    }

  ret = mstp_api_set_msti_port_path_cost(br_name, ifp->name,
	                                 instance, path_cost);
  return ret;

}

void mstp_br_config_vlan_add_port (char *br_name, struct interface *ifp,
                                   u_int16_t vid)
{

  struct mstp_bridge_info *br_config_info = NULL;
  struct mstp_bridge_instance_list *br_inst_list = NULL;
  struct mstp_bridge_instance_info *br_inst_info = NULL;
  struct mstp_bridge_config_list *br_conf_list = NULL;
  int instance = 0;

  br_config_info = mstp_bridge_config_info_get (br_name);

  if (!br_config_info)
    {
      br_conf_list = mstp_bridge_config_list_new (br_name);
      if (!br_conf_list)
        {
          zlog_err (mstpm," Unable to allocate memory for bridge %s",
	            br_name);
          return; 
        }
      mstp_bridge_config_list_link (br_conf_list);
      br_config_info = &(br_conf_list->br_info);
    }

  MSTP_GET_VLAN_INSTANCE_MAP(br_config_info->vlan_instance_map, vid, instance);

  if (instance <= 0)
    {
      return ;
    }

  br_inst_info = mstp_br_config_find_instance (br_config_info, instance);

  if (!br_inst_info)
    {
      br_inst_list = mstp_br_config_instance_new (instance);
      if (!br_inst_list)
        {
          zlog_err (mstpm," Unable to allocate memory for bridge %s",
	            br_name);
          return; 
        }

      mstp_br_config_link_instance_new (br_config_info, br_inst_list);
      br_inst_info = & (br_inst_list->instance_info);
    }

  if (!listnode_lookup (br_inst_info->port_list, ifp))
    {
      listnode_add (br_inst_info->port_list, ifp);
    }

  SET_FLAG (br_inst_info->config, MSTP_BRIDGE_CONFIG_INSTANCE_PORT);

  return;
}

void mstp_bridge_vlan_config_delete (char *bridge_name,
                                     mstp_vid_t vid)
{

  struct mstp_bridge_info *br_config_info = NULL;
  struct mstp_bridge_instance_list *br_inst_list = NULL;
  struct mstp_bridge_instance_info *br_inst_info = NULL;
  int instance = 0;
  u_int16_t invalid_range_idx = 0;

  br_config_info = mstp_bridge_config_info_get (bridge_name);

  if (!br_config_info)
    return;

  MSTP_GET_VLAN_INSTANCE_MAP(br_config_info->vlan_instance_map, vid, instance);

  if (instance <= 0)
    {
      return ;
    } 
  
  MSTP_BRIDGE_INSTANCE_CONFIG_GET (br_config_info->instance_list,
                                   br_inst_list, instance);

  if (!br_inst_list)
    return; 

  br_inst_info = &(br_inst_list->instance_info);

  if (!br_inst_info)
    return; 

  if (br_inst_info->vlan_list && vid)
    mstp_rlist_delete (&br_inst_info->vlan_list, vid, &invalid_range_idx);

  /* If this is the last vlan for this instance, free the instance */

  if ( vid && br_inst_info->vlan_list)
    return;

  mstp_br_config_unlink_instance (br_config_info, br_inst_list);

  if (br_inst_info->port_list)
    list_delete (br_inst_info->port_list); 

  if (br_inst_info->vlan_list)
    mstp_rlist_free (&br_inst_info->vlan_list); 

  XFREE (MTYPE_MSTP_BRIDGE_CONFIG_INSTANCE_LIST, br_inst_list);

  if (!br_config_info->instance_list)
    {
      mstp_bridge_config_list_free (br_config_info); 
    }

  return;

}
#endif /* HAVE_RPVST_PLUS */ 
