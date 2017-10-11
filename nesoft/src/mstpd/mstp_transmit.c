/* Copyright (C) 2003  All Rights Reserved. 

This file contains the transmit related functions
Specifically it implements the Port Protocol Migration and
Port Transmit State Machines.
*/


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
#include "mstp_sock.h"
#include "mstp_port.h"
#include "mstp_bridge.h"

#ifdef HAVE_RPVST_PLUS
#include "mstp_rpvst.h"
#endif /* HAVE_RPVST_PLUS */

#ifdef HAVE_HA
#include "mstp_cal.h"
#endif /* HAVE_HA */

const char bridge_grp_add[6] = { 0x01, 0x80, 0xc2, 0x00, 0x00, 0x00};

const char pro_bridge_grp_add[6] = { 0x01, 0x80, 0xc2, 0x00, 0x00, 0x08};

const char rpvst_bridge_grp_addr[6] = { 0x01, 0x00, 0x0c, 0xcc, 0xcc, 0xcd};

const char pro_backbone_bridge_oui[]={ 0x01, 0x1e, 0x83 };

/* This is a utility function to convert from ticks (256/sec)
   to STP BPDU time. */

#if defined (HAVE_L2GP) || defined (HAVE_RPVST_PLUS) 
 inline u_char *
mstp_format_timer (u_char *dest, int tics)
#else
static inline u_char *
mstp_format_timer (u_char *dest, int tics)
#endif /* HAVE_L2GP */
{
  *dest++ = (tics >> 8) & 0xff;  /* Handle the seconds part. */
  *dest++ = tics & 0xff;         /* Now the 256's of a second part. */
  return dest;
}



static inline void
mstp_bld_msti_info (struct mstp_instance_port *inst_port, u_char **buf,
                    unsigned char transmit_cisco)
{
  u_char *bufptr = *buf;
  u_char *temp_buf;
  enum port_role role = (inst_port->role == ROLE_BACKUP) ? 
    ROLE_ALTERNATE : inst_port->role ;


  /* If the port role for the instance is root or designated and 
   * bridge has selected one of its ports as master port OR
   * msti mastered is set for this msti for any other bridge port
   * with designated port role or root port role - set the master flag; */
  int master  = ((((inst_port->role == ROLE_DESIGNATED) 
                   || (inst_port->role == ROLE_ROOTPORT)) && 
                  inst_port->br->master ) || 
                 (inst_port->br->msti_mastered));

  if (transmit_cisco)
    {
      *bufptr++ = inst_port->instance_id; 
    }

  *bufptr = 0; 
  
  *bufptr++ = ((l2_is_timer_running (inst_port->tc_timer) ? 0x01 : 0) |
               (inst_port->send_proposal << 1) |
               ((role & 0x03)  << 2)|
               (((inst_port->state == STATE_LEARNING) || 
                 (inst_port->state == STATE_FORWARDING)) << 4) |
               ((inst_port->state == STATE_FORWARDING) << 5) |
               (inst_port->agree << 6) |
               (master << 7));
  
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

  /* For CISCO the whole Brideg ID and Port ID has to be sent.
   */

  if (transmit_cisco)
    {
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
    }
  else
    {
      /* copy msti bridge prio */
      *bufptr++ = (inst_port->br->msti_bridge_priority >> 8) & 0xf0 ;
      /* copy msti port prio */
      *bufptr++ = inst_port->msti_priority & 0xf0 ;
    } 

  /* copy rem hops */
  *bufptr++ = inst_port->hop_count;

  if (l2_is_timer_running (inst_port->tc_timer))
    {
      inst_port->br->tc_flag = PAL_TRUE; 
      inst_port->br->topology_change_detected = PAL_TRUE;
      inst_port->br->tc_initiator = inst_port->ifindex;
      inst_port->tcn_bpdu_sent++;
    }
  else
    {  
      inst_port->br->topology_change_detected = PAL_FALSE;
      inst_port->br->tc_flag = PAL_FALSE; 
      inst_port->conf_bpdu_sent++;
    }
  
  if (transmit_cisco)
    {
      *bufptr = 0; 
    }

  temp_buf = *buf;


}
/* This function copies the MSTP specific information in the buffer
 *  provided and returns the length of info populated */
static int
mstp_bld_mst_info (struct mstp_port *port, u_char **buf)
{
  struct mstp_bridge *br = port->br;
  struct mstp_instance_port *inst_port;
  u_char *bufptr = *buf;
  u_char *format_id_ptr = NULL;
  unsigned char transmit_cisco = PAL_FALSE;
  short len = BR_MST_MIN_VER_THREE_LEN;
  
  /* skip the len field as we will only know later */ 
  bufptr += 2;
  
  /* Copy config information 51 octets */
  /* TODO   pal_mem_cpy (bufptr, (char *)&br->config,51);
     bufptr += 51;
  */

  if (br->admin_cisco)
    {
      format_id_ptr = bufptr;
      bufptr++;
      transmit_cisco = PAL_TRUE;
    }
  else
    {
      *bufptr++ = br->config.cfg_format_id ;
      transmit_cisco = PAL_FALSE;
    }

  pal_mem_cpy (bufptr, br->config.cfg_name,MSTP_CONFIG_NAME_LEN);
  bufptr += MSTP_CONFIG_NAME_LEN;
  *bufptr++ = (br->config.cfg_revision_lvl >> 8) & 0xff;
  *bufptr++ = (br->config.cfg_revision_lvl) & 0xff;
  pal_mem_cpy (bufptr, br->config.cfg_digest,MSTP_CONFIG_DIGEST_LEN);
  bufptr += MSTP_CONFIG_DIGEST_LEN;

  /* For CISCO the Bridge ID is encoded first and then the
   * internal root patch cost.
   */

  if (transmit_cisco)
    {
     *bufptr++ = br->cist_reg_root.prio[0];
     *bufptr++ = br->cist_reg_root.prio[1];
     *bufptr++ = br->cist_reg_root.addr[0];
     *bufptr++ = br->cist_reg_root.addr[1];
     *bufptr++ = br->cist_reg_root.addr[2];
     *bufptr++ = br->cist_reg_root.addr[3];
     *bufptr++ = br->cist_reg_root.addr[4];
     *bufptr++ = br->cist_reg_root.addr[5];
     *bufptr++ = (port->cist_internal_rpc >> 24) & 0xff;
     *bufptr++ = (port->cist_internal_rpc >> 16) & 0xff;
     *bufptr++ = (port->cist_internal_rpc >> 8) & 0xff;
     *bufptr++ = port->cist_internal_rpc & 0xff;
    }
  else
    {
      /* copy common info */
     *bufptr++ = (port->cist_internal_rpc >> 24) & 0xff;
     *bufptr++ = (port->cist_internal_rpc >> 16) & 0xff;
     *bufptr++ = (port->cist_internal_rpc >> 8) & 0xff;
     *bufptr++ = port->cist_internal_rpc & 0xff;
     *bufptr++ = br->cist_bridge_id.prio[0];
     *bufptr++ = br->cist_bridge_id.prio[1];
     *bufptr++ = br->cist_bridge_id.addr[0];
     *bufptr++ = br->cist_bridge_id.addr[1];
     *bufptr++ = br->cist_bridge_id.addr[2];
     *bufptr++ = br->cist_bridge_id.addr[3];
     *bufptr++ = br->cist_bridge_id.addr[4];
     *bufptr++ = br->cist_bridge_id.addr[5];
   } 
  *bufptr++ = port->hop_count ; 


  /* Even though CISCO BPDU does not need the len variable.
   * It is maintained for future use.
   */

  if (transmit_cisco)
    {
      *bufptr++ = 0;
       len++;
    }

  
  /* for each instance present copy instance specific info */
  for (inst_port = port->instance_list; inst_port ; 
       inst_port = inst_port->next_inst)
    {
#if defined (HAVE_PROVIDER_BRIDGE) || defined (HAVE_B_BEB)
      if (inst_port->br->mstp_enabled == PAL_FALSE) 
        continue;
#endif /*(HAVE_PROVIDER_BRIDGE) || (HAVE_B_BEB)*/

      mstp_bld_msti_info (inst_port,&bufptr, transmit_cisco);

      if (transmit_cisco)
        {
          bufptr += MSTI_CISCO_BPDU_FRAME_SIZE;
          len += MSTI_CISCO_BPDU_FRAME_SIZE;
        }
      else
        {
          bufptr += MSTI_BPDU_FRAME_SIZE;
          len += MSTI_BPDU_FRAME_SIZE;
        }
    }


  if (transmit_cisco)
    {
      /* calculate the len copied and populate  octes 37-38 */
     *(*buf)++ = (len >> 8) & 0xff;
     *(*buf)++ = len & 0xff;
     *format_id_ptr = port->cisco_cfg_format_id;
    }
  else
    {
      /* calculate the len copied and populate  octes 37-38 */
     *(*buf)++ = (len >> 8) & 0xff;
     *(*buf)++ = len & 0xff;
    }
  
  /* include the bits to store the len field */
  return (len +2) ;
  
}

int
mstp_port_send (struct mstp_port *port, unsigned char *data, int length)
{
  s_int32_t ret;
  struct apn_vr *vr;
  struct interface *ifp;
  struct mstp_port *ceport;
  struct mstp_port *cn_port;
  struct mstp_bridge *bridge;
  struct mstp_port *svlan_port;
#ifdef HAVE_I_BEB
  struct mstp_port *pip_port = NULL, *vip_port = NULL;
  uint32_t isid = 0;
  char customer_src_mac[ETHER_ADDR_LEN],customer_dst_mac[ETHER_ADDR_LEN];
#endif /* HAVE_I_BEB */

  vr = apn_vr_get_privileged (mstpm);

  ifp = if_lookup_by_index ( &vr->ifm, port->ifindex );

  if (port->port_type != MSTP_VLAN_PORT_MODE_PE 
#if defined(HAVE_I_BEB)
      && port->port_type != MSTP_VLAN_PORT_MODE_CNP
      && port->port_type != MSTP_VLAN_PORT_MODE_PIP
#endif /* HAVE_I_BEB */
#if defined (HAVE_B_BEB)
      && port->port_type != MSTP_VLAN_PORT_MODE_CBP
#endif /* HAVE_B_BEB */
      )
    {
#ifdef HAVE_RPVST_PLUS
    if (port->br->type == MSTP_BRIDGE_TYPE_RPVST_PLUS)
      return mstp_rpvst_send(port, data, length);
    else 
#endif /* HAVE_RPVST_PLUS */
      return mstp_send (port->br->name, port->dev_addr,
                        BRIDGE_TYPE_PROVIDER (port->br) ?
                        pro_bridge_grp_add:
                        bridge_grp_add, MSTP_VLAN_NULL_VID,
                        port->ifindex, data, length);
    }

  cn_port = ceport = ifp->port;

  if (!ceport || !cn_port)
    return -1;

  bridge = ceport->br;

  if (!bridge)
    return -1;

#ifdef HAVE_I_BEB
  /* 
   * extracting isid from tunneled packet, since isid size is 4 bytes, copying
   * four bytes.
   */
  pal_mem_cpy(&isid, data+MSTP_PBB_ISID_OFFSET,4);
  
  BRIDGE_CIST_PORTLIST_LOOP( pip_port, bridge)
   {
      if (pip_port->port_type == MSTP_VLAN_PORT_MODE_CNP)
        {
            BRIDGE_CIST_PORTLIST_LOOP(vip_port,pip_port->cn_br) 
             {
                  if ( pal_hton32(isid) == vip_port->isid )
                    {
                       /*
                        * assume that received packet contails TPID & Itag also.
                        * need to skip TPID & Itag.
                        * extract CDA, CSA and STAG, have over the same to
                        * mstp_send.
                        * Replicate a copy of BPDU.
                        */

                        pal_mem_cpy(customer_src_mac, \
                                             data+CUSTOMER_SRC_MAC_OFFSET, \
                                             ETHER_ADDR_LEN);
                        pal_mem_cpy(customer_dst_mac, \
                                             data+CUSTOMER_DST_MAC_OFFSET, \
                                             ETHER_ADDR_LEN);

                        /* skipping six bytes (TPID & ITAG) */
                        return mstp_send (port->br->name,customer_src_mac,
                                       customer_dst_mac,MSTP_VLAN_NULL_VID,
                        port->ifindex, data+MSTP_PB_PACKET_OFFSET,\
                                        length-MSTP_PB_PACKET_OFFSET);
                     }

             }
        }
   }
#endif

  BRIDGE_CIST_PORTLIST_LOOP (svlan_port, bridge)
    {
      if ((svlan_port->port_type != NSM_MSG_VLAN_PORT_MODE_CN
           && svlan_port->port_type != NSM_MSG_VLAN_PORT_MODE_PN)
          || ! MSTP_VLAN_BMP_IS_MEMBER (svlan_port->vlanMemberBmp,
                                        port->svid))
        continue;

      ret = mstp_send (port->br->name, ceport->dev_addr, bridge_grp_add,
                       port->svid, svlan_port->ifindex, data, length);

      if (ret < 0)
        return ret;
    }

  return 0;
}

/* Transmit an MSTP PDU */

static int
mstp_tx_mstp (struct mstp_port *port)
{
  u_char buf[BR_MST_BPDU_MAX_FRAME_SIZE+VLAN_HEADER_SIZE];
  struct mstp_bridge *br;
  u_char * bufptr = buf;
  int len;
  int ret;
  
  br = port->br;
 
   if (br == NULL)
     return 0;
 
  /* Format a config BPDU */
  len = BR_RST_BPDU_FRAME_SIZE;

  bufptr = l2_llcFormat (bufptr);

  *bufptr++ = '\0';
  *bufptr++ = '\0';
  *bufptr++ =  (port->force_version >= BR_VERSION_MSTP) ? 
    BR_VERSION_MSTP : BR_VERSION_RSTP;
  *bufptr++ = BPDU_TYPE_RST;
 
  /* Clear the falgs before setting */ 
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
  if (br->admin_cisco
       && port->force_version >= BR_VERSION_MSTP)
     {
  	*bufptr++ = br->cist_bridge_id.prio[0];
	*bufptr++ = br->cist_bridge_id.prio[1];
	*bufptr++ = br->cist_bridge_id.addr[0];
	*bufptr++ = br->cist_bridge_id.addr[1];
	*bufptr++ = br->cist_bridge_id.addr[2];
	*bufptr++ = br->cist_bridge_id.addr[3];
	*bufptr++ = br->cist_bridge_id.addr[4];
	*bufptr++ = br->cist_bridge_id.addr[5];
     }
   else
     {
	*bufptr++ = port->cist_reg_root.prio[0];
        *bufptr++ = port->cist_reg_root.prio[1];
        *bufptr++ = port->cist_reg_root.addr[0];
        *bufptr++ = port->cist_reg_root.addr[1];
        *bufptr++ = port->cist_reg_root.addr[2];
        *bufptr++ = port->cist_reg_root.addr[3];
        *bufptr++ = port->cist_reg_root.addr[4];
        *bufptr++ = port->cist_reg_root.addr[5];
     }
  *bufptr++ = (port->cist_port_id >> 8) & 0xff;
  *bufptr++ = port->cist_port_id & 0xff;
  
  /* Each timer takes two octets. */
  bufptr = mstp_format_timer (bufptr, port->cist_message_age);
  bufptr = mstp_format_timer (bufptr, port->cist_max_age);
  bufptr = mstp_format_timer (bufptr, port->cist_hello_time);
  bufptr = mstp_format_timer (bufptr, port->cist_fwd_delay);

  *bufptr++ = BR_VERSION_ONE_LEN;


  if (port->force_version >= BR_VERSION_MSTP)
    {
      /* Encode MSTP Parameters */
      len += mstp_bld_mst_info (port,&bufptr);
    }

  ret = mstp_port_send (port, buf, len);

  if (l2_is_timer_running (port->tc_timer))
    {
      port->br->tc_flag =  PAL_TRUE; 
      port->tcn_bpdu_sent++;
      port->br->tc_initiator = port->ifindex;
      port->br->topology_change_detected = PAL_TRUE;
    }
  else
    {
     port->br->tc_flag =  PAL_FALSE; 
     port->conf_bpdu_sent++;
    }

  port->tx_count++;
  port->total_tx_count++;
  port->newInfoCist = PAL_FALSE;
  port->newInfoMsti = PAL_FALSE;
  port->tc_ack = PAL_FALSE;
  return ret;
}

/* IEEE 802.1w-2001 section 17.19.15  & sec 9.3.1
   This procedure transmits a configuration BPDU.
*/
static int 
mstp_tx_config (struct mstp_port *port)
{

  u_char buf[BR_CONFIG_BPDU_FRAME_SIZE+VLAN_HEADER_SIZE];
  register u_char * bufptr = buf;
  u_int32_t current_age;
  enum bpdu_type type = BPDU_TYPE_CONFIG;
  u_char *temp;
  int len;
  struct mstp_port *root_port;


  len = BR_CONFIG_BPDU_FRAME_SIZE;

  if (IS_BRIDGE_STP (port->br)
      && l2_is_timer_running (port->hold_timer))
    {
      port->config_bpdu_pending = PAL_TRUE;
      return 0;
    }

  root_port = mstp_find_cist_port (port->br, 
                                 port->br->cist_root_port_ifindex);

  /* Format a config BPDU */
  bufptr = l2_llcFormat (bufptr);

  *bufptr++ = '\0';
  *bufptr++ = '\0';
  *bufptr++ = '\0';
  *bufptr++ = type;
  
  temp = bufptr;
  *bufptr = 0;

  if (IS_BRIDGE_STP (port->br))
    {
    *bufptr++ = ((port->br->topology_change ? 0x01 : 0) |
       (port->tc_ack ? 0x80 : 0));

    if (port->br->topology_change)
      {
        port->br->tc_flag = PAL_TRUE;   
        port->tcn_bpdu_sent++;   
        port->br->tc_initiator = port->ifindex;
      }
    else 
      {
        port->br->tc_flag = PAL_FALSE;   
        port->conf_bpdu_sent++;   
      }

   }
  else
   {
    *bufptr++ = (l2_is_timer_running (port->tc_timer) ? 0x01 : 0) |
       (port->tc_ack ? 0x80 : 0);

    if (l2_is_timer_running (port->tc_timer))
      {
        port->br->tc_flag = PAL_TRUE;  
        port->br->topology_change_detected = PAL_TRUE;
        port->tcn_bpdu_sent++;   
        port->br->tc_initiator = port->ifindex;
      }
    else
      {
        port->br->tc_flag = PAL_FALSE;
        port->conf_bpdu_sent++;   
      }
   }
  
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
  *bufptr++ = port->cist_designated_bridge.prio[0];
  *bufptr++ = port->cist_designated_bridge.prio[1];
  *bufptr++ = port->cist_designated_bridge.addr[0];
  *bufptr++ = port->cist_designated_bridge.addr[1];
  *bufptr++ = port->cist_designated_bridge.addr[2];
  *bufptr++ = port->cist_designated_bridge.addr[3];
  *bufptr++ = port->cist_designated_bridge.addr[4];
  *bufptr++ = port->cist_designated_bridge.addr[5];
  *bufptr++ = (port->cist_designated_port_id >> 8) & 0xff;
  *bufptr++ = port->cist_designated_port_id & 0xff;
  
  /* Each timer takes two octets. */
  if (IS_BRIDGE_STP (port->br))
    {
      if (root_port)
        {
          current_age = (port->br->cist_max_age/MSTP_TIMER_SCALE_FACT) -
            l2_timer_get_remaining_secs (root_port->message_age_timer);
          current_age++;
          current_age *= MSTP_TIMER_SCALE_FACT;
          bufptr = mstp_format_timer (bufptr, current_age);
        }
      else
        bufptr = mstp_format_timer (bufptr, 0);
    }
  else
    bufptr = mstp_format_timer (bufptr, port->cist_message_age);

  bufptr = mstp_format_timer (bufptr, port->cist_max_age);
  bufptr = mstp_format_timer (bufptr, port->cist_hello_time);
  bufptr = mstp_format_timer (bufptr, port->cist_fwd_delay);

  mstp_send (port->br->name, port->dev_addr, bridge_grp_add,
             MSTP_VLAN_NULL_VID, port->ifindex, buf, len);
 
  port->newInfoCist = PAL_FALSE;
  port->newInfoMsti = PAL_FALSE; 
  port->tx_count++;
  port->total_tx_count++ ;
  port->tc_ack = PAL_FALSE;
  port->config_bpdu_pending = PAL_FALSE;

  if (IS_BRIDGE_STP (port->br))
    {
      port->hold_timer = l2_start_timer (stp_hold_timer_handler,
                                         (void *)port, STP_HOLD_TIME * MSTP_TIMER_SCALE_FACT,
                                          mstpm);
#ifdef HAVE_HA
      mstp_cal_create_mstp_hold_timer (port);
#endif /* HAVE_HA */
    }

  return RESULT_OK;
  
}

/* Transmit a TCN */

static int
mstp_tx_tcn (struct mstp_port *port)
{
 
  u_char buf[STP_TCN_BPDU_SIZE+VLAN_HEADER_SIZE];
  register u_char * bufptr = buf;
  enum bpdu_type type = BPDU_TYPE_TCN;
  int len;
  
  /* We have received  a tc ack or tc while timer has expired */
  if (! IS_BRIDGE_STP (port->br) &&
      ! l2_is_timer_running (port->tc_timer))
    return RESULT_OK;

  len = STP_TCN_BPDU_SIZE;

  bufptr = l2_llcFormat (bufptr);

  *bufptr++ = '\0';
  *bufptr++ = '\0';
  *bufptr++ = '\0';
  *bufptr++ = type;

  mstp_send (port->br->name, port->dev_addr, bridge_grp_add,
             MSTP_VLAN_NULL_VID, port->ifindex, buf, len);
 
  port->newInfoCist = PAL_FALSE;
  port->newInfoMsti = PAL_FALSE; 
  port->tx_count++;
  port->tcn_bpdu_sent++;
  port->br->tc_flag = PAL_TRUE;
  port->br->tc_initiator = port->ifindex;
  port->br->topology_change_detected = PAL_TRUE;
  return RESULT_OK;
}

int 
mstp_tx (struct mstp_port *port)
{
  int ret = RESULT_ERROR;
  struct mstp_bridge *br = port->br;

  if (LAYER2_DEBUG(proto, PACKET_TX))
    {
      zlog_debug (mstpm,"mstp_tx: TX for port %u  role %d selected %d "
                  "tx count %d send mstp %d ",port->ifindex,
                  port->cist_role,port->selected,port->tx_count,
                  port->send_mstp);
    }

  /* We do not transmit BPDUs on a portfast port if it has bpdu filter
   * feature enabled.
   */
  if (port->oper_edge && port->oper_bpdufilter)
    {
      if (LAYER2_DEBUG(proto, PROTO_DETAIL))
        {
          zlog_debug (mstpm,"rstp_tx: Not transmitting BPDU for bpdu-filter "
                      "port %u ", port->ifindex);
        }
      return RESULT_OK;
    }

#ifdef HAVE_L2GP
  /* No BPDUs are transmitted on the l2gp port */
  if (port->isL2gp && !port->enableBPDUtx)
    {
      if (LAYER2_DEBUG(proto, PROTO_DETAIL))
        {
          zlog_debug (mstpm,"rstp_tx: Not transmitting BPDU on l2gp "
                      "port %u ", port->ifindex);
        }
      return RESULT_OK;
    }
#endif /* HAVE_L2GP */

#ifdef HAVE_HA
  mstp_cal_modify_mstp_port (port);
#endif /* HAVE_HA */

  if (((port->tx_count < br->transmit_hold_count) && port->selected)
       ||((br->topology_type == TOPOLOGY_RING)&& (port->critical_bpdu == PAL_TRUE)))
   {
   if (port->send_mstp) 
     {
#ifdef HAVE_RPVST_PLUS
       if (port->br->type == MSTP_BRIDGE_TYPE_RPVST_PLUS)
         {
           if (port->cist_role != ROLE_DISABLED)
             return (mstp_tx_rpvst (port));
         }
       else
#endif /* HAVE_RPVST_PLUS */
         if ((port->newInfoCist
               || port->newInfoMsti)
             && port->cist_role != ROLE_DISABLED)
           return (mstp_tx_mstp (port));
     }
   else {  /* !sendmstp */ 
       if (port->newInfoCist) {
           if (port->cist_role == ROLE_ROOTPORT)
             ret = mstp_tx_tcn (port);
           else 
             if (port->cist_role == ROLE_DESIGNATED)
               ret = mstp_tx_config (port);
       }
   }
 }
  return ret;

}

void
stp_transmit_tcn_bpdu (struct mstp_bridge *br)
{
  /* Section 8.6.6.3 */
  struct mstp_port * port = mstp_find_cist_port (br,
                                                 br->cist_root_port_ifindex);

  if (port == NULL)
    return;
#ifdef HAVE_HA
   mstp_cal_modify_mstp_port (port);
#endif /* HAVE_HA */

  port->newInfoCist = PAL_TRUE;
  mstp_tx_tcn (port);

  if (LAYER2_DEBUG (proto, PROTO))
    zlog_debug (mstpm, "stp_transmit_tcn_bpdu: "
                "Bridge %s sending tcn bpdu on port %u",
                br->name, port->ifindex);
}

void
mstp_tx_bridge (struct mstp_bridge *br)
{
  struct mstp_port *port;

  if (IS_BRIDGE_STP (br))
    return;

  for (port = br->port_list; port; port = port->next)
    {

#ifdef HAVE_RPVST_PLUS
      if (br->type == MSTP_BRIDGE_TYPE_RPVST_PLUS)
        {
          mstp_tx (port);
        }
       else
#endif /* HAVE_RPVST_PLUS */
         if (port->newInfoCist || port->newInfoMsti)
          {
           if (br->topology_type == TOPOLOGY_RING) 
             port->critical_bpdu = PAL_TRUE;

           mstp_tx (port);
          }
    }
}

/* The following two functions implement the 
 * Port-Protocol Migration State Machine. */

void
mstp_sendmstp (struct mstp_port *port)
{
  if (l2_is_timer_running (port->migrate_timer))
    {
      if (LAYER2_DEBUG(proto, TIMER_DETAIL))
        zlog_debug (mstpm,
                    "mstp_send_mstp: Migrate timer for port (%u) already running ",
                    port->ifindex);
      return;
    } 
  if (LAYER2_DEBUG(proto, TIMER))
    zlog_debug (mstpm,
                "mstp_send_mstp: starting migration timer for bridge %s port (%u)",
                port->br->name, port->ifindex);

#ifdef HAVE_HA
  mstp_cal_modify_mstp_port (port);
#endif /* HAVE_HA */
  
  port->send_mstp = PAL_TRUE;

  /* Start the migrate timer */
  port->migrate_timer = l2_start_timer (mstp_migrate_timer_handler,
                                        (void *)port, MSTP_MIGRATE_TIME * L2_TIMER_SCALE_FACT,
                                        mstpm);
#ifdef HAVE_HA
  mstp_cal_create_mstp_migrate_timer (port);
#endif /* HAVE_HA */

  /* Clear rcvd_mstp and rcvd_stp flags when in send_mstp or send_stp as per
   * protocol migration state machine, Sec 17.26
   */
  port->rcvd_mstp = PAL_FALSE;
  port->rcvd_stp = PAL_FALSE;
  port->rcvd_rstp = PAL_FALSE;

}
  

void
mstp_sendstp (struct mstp_port *port)
{
  if (l2_is_timer_running (port->migrate_timer))
    {
      if (LAYER2_DEBUG(proto, TIMER_DETAIL))
        zlog_debug (mstpm,
                    "mstp_send_stp: Migrate timer for port (%u) already running ",
                    port->ifindex);
      return;
    }
  if (LAYER2_DEBUG(proto, TIMER))
    zlog_debug (mstpm,
                "mstp_send_stp: starting migration timer for bridge %s port (%u)",
                port->br->name, port->ifindex);
  
#ifdef HAVE_HA
  mstp_cal_modify_mstp_port (port);
#endif /* HAVE_HA */

  port->send_mstp = PAL_FALSE;

  /* Start the migrate timer */
  port->migrate_timer = l2_start_timer (mstp_migrate_timer_handler,
                                        (void *)port, MSTP_MIGRATE_TIME * L2_TIMER_SCALE_FACT,
                                        mstpm);
#ifdef HAVE_HA
  mstp_cal_create_mstp_migrate_timer (port);
#endif /* HAVE_HA */

  /* Clear rcvd_mstp and rcvd_stp flags when in send_mstp or send_stp as per
   * protocol migration state machine, Sec 17.26
   */
  port->rcvd_mstp = PAL_FALSE;
  port->rcvd_stp = PAL_FALSE;
  port->rcvd_rstp = PAL_FALSE;
    
}

/* STP transmit Functions */

void
stp_generate_bpdu_on_bridge (struct mstp_bridge *br)
{
  struct mstp_port *port;

  for (port = br->port_list; port; port = port->next)
    {
      if (port->cist_role == ROLE_DESIGNATED)
        {
          port->newInfoCist = PAL_TRUE;

          mstp_tx (port);
        }
    }
}
