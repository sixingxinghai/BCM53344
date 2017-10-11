/* Copyright (C) 2002  All Rights Reserved. */


#include "pal.h"
#include "lib.h"
#include "tlv.h"
#include "thread.h"
#include "checksum.h"

#include "vty.h"
#include "linklist.h"
#include "prefix.h"
#include "table.h"
#include "hash.h"

#include "if.h"
#include "ptree.h"
#include "mpls.h"
#include "mpls_common.h"
#include "mpls_client.h"
#ifdef HAVE_DIFFSERV
#include "diffserv.h"
#endif /* HAVE_DIFFSERV */
#include "nsmd.h"
#ifdef HAVE_DSTE
#include "dste.h"
#include "nsm_dste.h"
#endif /* HAVE_DSTE */

#include "nsmd.h"
#include "nsm_debug.h"
#include "nsm_mpls.h"
#include "nsm_mpls_vc.h"
#include "nsm_interface.h"
#include "nsm_table.h"
#include "rib.h"
#include "nsm_server.h"
#include "nsm_mpls_rib.h"
#include "nsm_mpls_api.h"
#include "nsm_mpls_oam.h"
#include "nsm_mpls_oam_network.h"
#include "nsm_mpls_oam_packet.h"


#ifdef HAVE_VPLS
#include "nsm_vpls.h"
#endif /* HAVE_VPLS */
#include "nsm_router.h"

static u_int32_t oamInstCount;

bool_t oam_reply_tlv = NSM_FALSE; 
bool_t oam_req_dsmap_tlv = NSM_FALSE; 

static int
oam_udp_checksum (struct pal_udp_header *udph, struct pal_in4_header *iphdr,
                  u_char *buf, u_int16_t len)
{
  u_char *p = buf;

  IPV4_ADDR_COPY (p, &iphdr->ip_src);
  p += 4;
  IPV4_ADDR_COPY (p, &iphdr->ip_dst);
  p += 5;
  pal_mem_cpy (p, &iphdr->ip_p, 1);
  p += 1;
  pal_mem_cpy (p, &udph->uh_ulen, 2);
  p += 2;

  udph->uh_sum = in_checksum ((u_int16_t *)buf, len + UDP_PSEUDO_HEADER_SIZE);

  /* set checksum in the buffer */
  pal_mem_cpy (buf + UDP_PSEUDO_HEADER_SIZE + 6, &udph->uh_sum, 2);

  return NSM_SUCCESS;
}

void
nsm_mpls_oam_echo_pkt_header_dump (struct mpls_oam_hdr *hdr, bool_t send)
{
  if (hdr->msg_type == MPLS_ECHO_REQUEST_MESSAGE)
    zlog_info (NSM_ZG, " OAM request packet %s \n", 
	       (send ? "send" : "receive"));
  else
    zlog_info (NSM_ZG, " OAM reply packet %s \n",
	       (send ? "send" : "receive"));

  zlog_info (NSM_ZG, "  OAM header \n");
  zlog_info (NSM_ZG, "    version : %d \n", hdr->ver_no);
  zlog_info (NSM_ZG, "    flags resvd : %d \n", hdr->u.flags.resvd);
  zlog_info (NSM_ZG, "    flags fec : %d \n", hdr->u.flags.fec);
  zlog_info (NSM_ZG, "    l_packet : %d \n", hdr->u.l_packet);
  zlog_info (NSM_ZG, "    msg_type : %s \n", 
	     (hdr->msg_type == MPLS_ECHO_REQUEST_MESSAGE ? 
	      "MPLS ECHO REQUEST" : "MPLS ECHO REPLY"));
  zlog_info (NSM_ZG, "    reply_mode : %d \n", hdr->reply_mode);
  zlog_info (NSM_ZG, "    return_code : %d \n", hdr->return_code);
  zlog_info (NSM_ZG, "    ret_subcode : %d \n", hdr->ret_subcode);
  zlog_info (NSM_ZG, "    sender_handle : %d \n", hdr->sender_handle);
  zlog_info (NSM_ZG, "    seq_number : %d \n", hdr->seq_number);
  zlog_info (NSM_ZG, "    ts_tx_sec : %d \n", hdr->ts_tx_sec);
  zlog_info (NSM_ZG, "    ts_tx_usec : %d \n", hdr->ts_tx_usec);
  zlog_info (NSM_ZG, "    ts_rx_sec : %d \n", hdr->ts_rx_sec);
  zlog_info (NSM_ZG, "    ts_rx_usec : %d \n", hdr->ts_rx_usec);
}

void
nsm_mpls_oam_echo_pkt_fec_tlv_dump (struct mpls_oam_target_fec_stack *fec)
{
  zlog_info (NSM_ZG, "  OAM Target FEC Stack \n");
  zlog_info (NSM_ZG, "    Length : %d \n", fec->tlv_h.length);
  zlog_info (NSM_ZG, "    flags : %d \n", fec->flags);
  if (fec->ldp.tlv_h.type == MPLS_OAM_LDP_IPv4_PREFIX_TLV)
    {
      zlog_info (NSM_ZG, "    MPLS OAM LDP IPv4 PREFIX TLV \n");
      zlog_info (NSM_ZG, "      length : %d \n", fec->ldp.tlv_h.length);
      zlog_info (NSM_ZG, "      ip address : %r \n", &fec->ldp.ip_addr);
      zlog_info (NSM_ZG, "      prefix length : %d \n", fec->ldp.u.l_packet);
    }
  if (fec->rsvp.tlv_h.type == MPLS_OAM_RSVP_IPv4_PREFIX_TLV)
    {
      zlog_info (NSM_ZG, "    MPLS OAM RSVP IPv4 PREFIX TLV \n");
      zlog_info (NSM_ZG, "      length : %d \n", fec->rsvp.tlv_h.length);
      zlog_info (NSM_ZG, "      ip address : %r \n", &fec->rsvp.ip_addr); 
      zlog_info (NSM_ZG, "      tunnle ID : %d \n", fec->rsvp.tunnel_id); 
      zlog_info (NSM_ZG, "      ext tunnel ID : %r \n", 
		 &fec->rsvp.ext_tunnel_id);
      zlog_info (NSM_ZG, "      sender address : %r \n", &fec->rsvp.send_addr);
      zlog_info (NSM_ZG, "      lsp ID : %d \n", fec->rsvp.lsp_id);
    }
  if (fec->l3vpn.tlv_h.type == MPLS_OAM_IPv4_VPN_TLV)
    {
      zlog_info (NSM_ZG, "    MPLS OAM IPv4 VPN TLV \n");
      zlog_info (NSM_ZG, "      length : %d \n", fec->l3vpn.tlv_h.length);
      if (fec->l3vpn.rd.vpnrd_type == VPN_RD_TYPE_AS)
	{
	  zlog_info (NSM_ZG, "      rd : %d:%d \n", 
		     fec->l3vpn.rd.vpnrd_as, fec->l3vpn.rd.vpnrd_asnum);
	}
      else
	{
	  zlog_info (NSM_ZG, "      rd: %r:%d \n",
		     fec->l3vpn.rd.vpnrd_ip, fec->l3vpn.rd.vpnrd_ipnum);

	}
      zlog_info (NSM_ZG, "      VPN prefix : %r \n", &fec->l3vpn.vpn_pfx);
      zlog_info (NSM_ZG, "      VPN prefix length: %d \n", 
		 fec->l3vpn.u.l_packet);

    }
  if (fec->l2_dep.tlv_h.type == MPLS_OAM_FEC128_DEP_TLV)
    {
      zlog_info (NSM_ZG, "    MPLS OAM FEC128 DEP TLV \n");
      zlog_info (NSM_ZG, "      length : %d \n", fec->l2_dep.tlv_h.length);
      zlog_info (NSM_ZG, "      remote address : %r \n", &fec->l2_dep.remote);
      zlog_info (NSM_ZG, "      vc ID : %u \n", fec->l2_dep.vc_id);
      zlog_info (NSM_ZG, "      pw type : %d \n", fec->l2_dep.pw_type);
    }
  if (fec->l2_curr.tlv_h.type == MPLS_OAM_FEC128_CURR_TLV)
    {
      zlog_info (NSM_ZG, "    MPLS OAM FEC128 CURR TLV \n");
      zlog_info (NSM_ZG, "      length : %d \n", fec->l2_curr.tlv_h.length);
      zlog_info (NSM_ZG, "      source address : %r \n", &fec->l2_curr.source);
      zlog_info (NSM_ZG, "      remote address : %r \n", &fec->l2_curr.remote); 
      zlog_info (NSM_ZG, "      vc ID : %u \n", fec->l2_curr.vc_id);
      zlog_info (NSM_ZG, "      pw type : %d \n", fec->l2_curr.pw_type);
    }
  if (fec->pw_tlv.tlv_h.type == MPLS_OAM_FEC129_PW_TLV)
    {
      zlog_info (NSM_ZG, "    MPLS OAM FEC129 PW TLV \n");
      zlog_info (NSM_ZG, "      length : %d \n", fec->pw_tlv.tlv_h.length);
      zlog_info (NSM_ZG, "      source address : %r \n", &fec->pw_tlv.source);
      zlog_info (NSM_ZG, "      remote address : %r \n", &fec->pw_tlv.remote);
      zlog_info (NSM_ZG, "      pw type : %d \n", fec->pw_tlv.pw_type);
      zlog_info (NSM_ZG, "      agi type : %d \n", fec->pw_tlv.agi_type);
      zlog_info (NSM_ZG, "      agi length : %d \n", fec->pw_tlv.agi_length);
      if (fec->pw_tlv.agi_value)
	zlog_info (NSM_ZG, "      agi value : %s \n", fec->pw_tlv.agi_value);
      zlog_info (NSM_ZG, "      saii type : %d \n", fec->pw_tlv.saii_type); 
      zlog_info (NSM_ZG, "      saii length : %d \n", fec->pw_tlv.saii_length); 
      if (fec->pw_tlv.saii_value)
	zlog_info (NSM_ZG, "      saii value : %s \n", fec->pw_tlv.saii_value); 
      zlog_info (NSM_ZG, "      taii type : %d \n", fec->pw_tlv.taii_type); 
      zlog_info (NSM_ZG, "      taii length : %d \n", fec->pw_tlv.taii_length); 
      if (fec->pw_tlv.taii_value)
	zlog_info (NSM_ZG, "      taii value : %s \n", fec->pw_tlv.taii_value); 
    }
  if (fec->ipv4.tlv_h.type == MPLS_OAM_GENERIC_FEC_TLV)
    {
      zlog_info (NSM_ZG, "    MPLS OAM GENERIC FEC TLV \n");
      zlog_info (NSM_ZG, "      length : %d \n", fec->ipv4.tlv_h.length);
      zlog_info (NSM_ZG, "      ipv4 address : %r \n", &fec->ipv4.ipv4_addr); 
      zlog_info (NSM_ZG, "      ipv4 prefix length : %d \n", 
		 fec->ipv4.u.l_packet);
    }
  if (fec->nil_fec.tlv_h.type == MPLS_OAM_NIL_FEC_TLV)
    {
      zlog_info (NSM_ZG, "    MPLS OAM NIL FEC TLV \n");
      zlog_info (NSM_ZG, "      length : %d \n", fec->nil_fec.tlv_h.length);
      zlog_info (NSM_ZG, "      label : %d \n", fec->nil_fec.u.bits.label); 
      zlog_info (NSM_ZG, "      mbz : %d \n", fec->nil_fec.u.bits.mbz); 
      zlog_info (NSM_ZG, "      l_packet : %d \n", fec->nil_fec.u.l_packet); 
    }
}

void
nsm_mpls_oam_echo_pkt_dsmap_dump (struct mpls_oam_downstream_map *dsmap)
{
  struct shimhdr *shim;
  struct listnode *node;

  zlog_info (NSM_ZG, "  OAM DSMAP TLV\n");
  zlog_info (NSM_ZG, "    length : %d \n", dsmap->tlv_h.length);
  zlog_info (NSM_ZG, "    dsmap data mtu : %d \n", dsmap->u.data.mtu);
  zlog_info (NSM_ZG, "    dsmap data family : %d \n", dsmap->u.data.family);
  zlog_info (NSM_ZG, "    dsmap data flags : %d \n", dsmap->u.data.flags);
  zlog_info (NSM_ZG, "    dsmap l_packet : %d \n", dsmap->u.l_packet);
  zlog_info (NSM_ZG, "    dsmap ds_ip : %r \n", &dsmap->ds_ip);
  zlog_info (NSM_ZG, "    dsmap ds_if_ip : %r \n", &dsmap->ds_if_ip);
  zlog_info (NSM_ZG, "    dsmap multipath_type : %d \n", dsmap->multipath_type);
  zlog_info (NSM_ZG, "    dsmap depth : %d \n", dsmap->depth);
  zlog_info (NSM_ZG, "    dsmap multipath_len : %d \n", dsmap->multipath_len);
  if (dsmap->multi_info)
    zlog_info (NSM_ZG, "    dsmap multi_info : %s \n", dsmap->multi_info);
  if (dsmap->ds_label)
    {
      LIST_LOOP (dsmap->ds_label, shim, node)
	{
	  zlog_info (NSM_ZG, "   dsmap label : %d \n", 
		     get_label_net(shim->shim));
	}
    }
}

void
nsm_mpls_oam_echo_pkt_tos_reply_dump (struct mpls_oam_reply_tos *tos_reply)
{
  zlog_info (NSM_ZG, "  OAM TOS REPLY TLV\n");
  zlog_info (NSM_ZG, "    length : %d \n", tos_reply->tlv_h.length);
  zlog_info (NSM_ZG, "    tos_byte : %d \n", tos_reply->u.tos.tos_byte);
  zlog_info (NSM_ZG, "    mbz : %d \n", tos_reply->u.tos.mbz);
  zlog_info (NSM_ZG, "    l_packet : %d \n", tos_reply->u.l_packet);
}

void
nsm_mpls_oam_echo_pkt_pad_tlv_dump (struct mpls_oam_pad_tlv *pad) 
{
  zlog_info (NSM_ZG, "  OAM PAD TLV\n");
  zlog_info (NSM_ZG, "    length : %d \n", pad->tlv_h.length);
  zlog_info (NSM_ZG, "    value : %d \n", pad->val);
}

void
nsm_mpls_oam_echo_pkt_if_lbl_stk_dump (struct mpls_oam_if_lbl_stk *if_lbl_stk)
{
  int i;

  zlog_info (NSM_ZG, "  OAM Interface and Label Stack \n");
  zlog_info (NSM_ZG, "    length : %d \n", if_lbl_stk->tlv_h.length);
  zlog_info (NSM_ZG, "    family : %d \n", if_lbl_stk->u.fam.family);
  zlog_info (NSM_ZG, "    address : %r \n", &if_lbl_stk->addr);
  zlog_info (NSM_ZG, "    ifindex : %d \n", if_lbl_stk->ifindex);
  zlog_info (NSM_ZG, "    depth : %d \n", if_lbl_stk->depth);

  for (i = 0; i < MAX_LABEL_STACK_DEPTH; i++)
    zlog_info (NSM_ZG, "    out_stack[%d] : %d \n", i, if_lbl_stk->out_stack[i]);
}

void
nsm_mpls_oam_echo_pkt_err_tlv_dump (struct mpls_oam_errored_tlv *err_tlv)
{
  zlog_info (NSM_ZG, "  OAM ERROR TLV\n");
  zlog_info (NSM_ZG, "    length : %d \n", err_tlv->tlv_h.length);

  if (err_tlv->buf)
    zlog_info (NSM_ZG, "    buf : %s \n", &err_tlv->buf);

}

void
nsm_mpls_oam_echo_request_dump (struct mpls_oam_echo_request *oam_req, 
				bool_t send)
{
  nsm_mpls_oam_echo_pkt_header_dump (&oam_req->omh, send);

  if (oam_req->fec_tlv.tlv_h.type == MPLS_OAM_FEC_TLV)
    nsm_mpls_oam_echo_pkt_fec_tlv_dump (&oam_req->fec_tlv);
  
  zlog_info (NSM_ZG, "  OAM flags %d \n", oam_req->flags);

  if (oam_req->dsmap.tlv_h.type == MPLS_OAM_DSMAP_TLV)
    nsm_mpls_oam_echo_pkt_dsmap_dump (&oam_req->dsmap); 

  if (oam_req->tos_reply.tlv_h.type == MPLS_OAM_REPLY_TOS_TLV)
    nsm_mpls_oam_echo_pkt_tos_reply_dump (&oam_req->tos_reply); 

  if (oam_req->pad_tlv.tlv_h.type == MPLS_OAM_PAD_TLV)
    nsm_mpls_oam_echo_pkt_pad_tlv_dump (&oam_req->pad_tlv); 
}

void
nsm_mpls_oam_echo_reply_dump (struct mpls_oam_echo_reply *oam_reply, 
			      bool_t send)
{
  nsm_mpls_oam_echo_pkt_header_dump (&oam_reply->omh, send);

  zlog_info (NSM_ZG, " OAM state : %d \n", oam_reply->state); 

  if (oam_reply->fec_tlv.tlv_h.type == MPLS_OAM_FEC_TLV)
    nsm_mpls_oam_echo_pkt_fec_tlv_dump (&oam_reply->fec_tlv);

  zlog_info (NSM_ZG, "  OAM flags %d \n", oam_reply->flags);

  if (oam_reply->dsmap.tlv_h.type == MPLS_OAM_DSMAP_TLV)
    nsm_mpls_oam_echo_pkt_dsmap_dump (&oam_reply->dsmap); 

  if (oam_reply->if_lbl_stk.tlv_h.type == MPLS_OAM_IF_LBL_STACK)
    nsm_mpls_oam_echo_pkt_if_lbl_stk_dump (&oam_reply->if_lbl_stk);
  
  if (oam_reply->err_tlv.tlv_h.type == MPLS_OAM_ERROR_TLVS)
    nsm_mpls_oam_echo_pkt_err_tlv_dump (&oam_reply->err_tlv);

  if (oam_reply->pad_tlv.tlv_h.type == MPLS_OAM_PAD_TLV)
    nsm_mpls_oam_echo_pkt_pad_tlv_dump (&oam_reply->pad_tlv); 
}

int
nsm_mpls_oam_recv_echo_req (struct nsm_master *nm,
                            struct mpls_oam_hdr *oam_hdr,
                            struct mpls_oam_echo_request *oam_req,
                            struct pal_timeval *rx_time,
                            struct mpls_oam_ctrl_data *ctrl_data)
{
  /* Now that we have the Request message we can process it.
   * The first task is the see if we are an egress for the advertised FEC
   */
  int ret = 0;

  if (CHECK_FLAG (oam_req->fec_tlv.flags, MPLS_OAM_MPLS_VPN_PREFIX))
    {
      ret = nsm_mpls_oam_process_l3vpn_tlv (nm, oam_hdr, oam_req, rx_time,
                                            ctrl_data);

      if (ret < 0)
        return ret;
    }
#ifdef HAVE_MPLS_VC
  else if (CHECK_FLAG (oam_req->fec_tlv.flags, MPLS_OAM_L2_CIRCUIT_PREFIX_CURR))
    {
      /* Function will take care of transit processing in case of outer tunnel
       * as well */

      ret = nsm_mpls_oam_process_l2data_tlv (nm, oam_hdr, oam_req, rx_time,
                                             ctrl_data);

      if (ret < 0)
        return ret;
    }
  /* Note that although the Deprecated TLV is processed only the CURRENT TLV is
   * used for replies */
  else if (CHECK_FLAG (oam_req->fec_tlv.flags, MPLS_OAM_L2_CIRCUIT_PREFIX_DEP))
    {
      ret = nsm_mpls_oam_process_l2data_tlv (nm, oam_hdr, oam_req, rx_time,
                                             ctrl_data);

      if (ret < 0)
        return ret;
    }
#endif /* HAVE_MPLS_VC */
  else if (CHECK_FLAG (oam_req->fec_tlv.flags, MPLS_OAM_LDP_IPv4_PREFIX))
    {
      ret = nsm_mpls_oam_process_ldp_ipv4_tlv (nm, oam_hdr, oam_req, rx_time,
                                               ctrl_data, NSM_FALSE);

      if (ret < 0)
        return ret;
    }
  else if (CHECK_FLAG (oam_req->fec_tlv.flags, MPLS_OAM_RSVP_IPv4_PREFIX))
    {
      ret = nsm_mpls_oam_process_rsvp_ipv4_tlv (nm, oam_hdr, oam_req, rx_time,
                                                ctrl_data, NSM_FALSE);

      if (ret < 0)
        return ret;
    }
  else if (CHECK_FLAG (oam_req->fec_tlv.flags, MPLS_OAM_GENERIC_IPv4_PREFIX))
    {
      ret = nsm_mpls_oam_process_gen_ipv4_tlv (nm, oam_hdr, oam_req, rx_time,
                                               ctrl_data, NSM_FALSE);

      if (ret < 0)
        return ret;
    }

  return 1;
}

int
nsm_mpls_oam_echo_request_send (struct nsm_mpls_oam *oam, u_char *sendbuf,
                                u_int32_t length)
{
  int ret = 0;
  /* Send the echo request */

  if (oam->msg.type != MPLSONM_OPTION_L2CIRCUIT &&
      oam->msg.type != MPLSONM_OPTION_VPLS &&
      oam->msg.type != MPLSONM_OPTION_L3VPN)
    {
      ret = apn_mpls_oam_send_echo_request (oam->ftn->ftn_ix, 0, oam->fwd_flags,
                                            oam->msg.ttl, length, sendbuf);
    }
#ifdef HAVE_MPLS_VC
  else if (oam->msg.type == MPLSONM_OPTION_L2CIRCUIT)
    {
      ret = apn_mpls_oam_vc_send_echo_request (oam->u.vc->vc_fib->ac_if_ix,
                                               oam->u.vc->id, oam->fwd_flags,
                                               oam->msg.ttl, length, sendbuf);
    }
#ifdef HAVE_VPLS
  else if (oam->msg.type == MPLSONM_OPTION_VPLS)
    {
      /* Function to process echo request for Spoke and Mesh VCs */
      ret = nsm_mpls_oam_send_vpls_echo_request (oam->u.vpls,
                                                 &(oam->msg.u.l2_data.vc_peer),
                                                 oam->fwd_flags,
                                                 oam->msg.ttl, length,
                                                 sendbuf);

    }
#endif /* HAVE_VPLS */
#endif /* HAVE_MPLS_VC */
#ifdef HAVE_VRF
  else if (oam->msg.type == MPLSONM_OPTION_L3VPN)
    {
      ret =  apn_mpls_oam_send_echo_request (oam->ftn->ftn_ix,
                                             oam->n_vrf->vrf->id,
                                             oam->fwd_flags, oam->msg.ttl,
                                             length, sendbuf);
    }
#endif /* HAVE_VRF */
  if (ret < 0)
    return ret; /* Add code to send NSM PING ERROR */

  /*  start the Packet interval thread to schedule the next ping */
  if ((oam->count < oam->msg.repeat) &&
      (oam->msg.req_type == MPLSONM_OPTION_PING))
    {
      struct pal_timeval tv;
      tv.tv_sec = (oam->msg.interval) /1000;
      tv.tv_usec = (oam->msg.interval % 1000) * 1000;
      oam->oam_echo_interval =
               thread_add_timer_timeval (NSM_ZG,
                            nsm_mpls_oam_echo_request_create_send, oam, tv);
    }

  return NSM_SUCCESS;
}

int
nsm_mpls_oam_prep_req_for_network (struct nsm_mpls_oam *oam,
                                   u_char *pnt, u_int16_t len,
                                   u_char *sendbuf)
{
  u_char buf[OAM_DATA_SIZE];
  u_int16_t iph_len;
  u_char iph_with_ra[sizeof (struct pal_in4_header) + OAM_IPOPT_RA_LEN];
  u_char opt[OAM_IPOPT_RA_LEN]={OAM_IPOPT_RA, OAM_IPOPT_RA_LEN, 0, 0};
  struct pal_in4_header *iph;

  pal_mem_set (buf, 0, OAM_DATA_SIZE);
  pal_mem_set (iph_with_ra, 0,
               sizeof (struct pal_in4_header) + OAM_IPOPT_RA_LEN);

  oam->udph.uh_sum = 0;

  oam->iph.ip_sum = 0;

  iph_len = sizeof (struct pal_in4_header) + OAM_IPOPT_RA_LEN;
  pal_mem_cpy (iph_with_ra, &oam->iph, sizeof (struct pal_in4_header));
  pal_mem_cpy (&iph_with_ra[sizeof (struct pal_in4_header)],
               &opt, OAM_IPOPT_RA_LEN);
  iph = (struct pal_in4_header *) iph_with_ra;
  iph->ip_hl = iph_len >> 2;
  oam->iph.ip_hl = iph->ip_hl;
  /* Total Packet Size */
  oam->udph.uh_ulen = pal_hton16(len+sizeof (struct pal_udp_header));

  pal_mem_cpy (buf + UDP_PSEUDO_HEADER_SIZE,
               &oam->udph, sizeof (struct pal_udp_header));
  pal_mem_cpy ((buf + UDP_PSEUDO_HEADER_SIZE + sizeof (struct pal_udp_header)),
                pnt, len);

  pal_in4_ip_packet_len_set (&oam->iph, (len +
                            (sizeof (struct pal_udp_header))));

  oam_udp_checksum (&oam->udph, &oam->iph, buf,
                    len+sizeof (struct pal_udp_header));

  iph->ip_len = pal_hton16 (oam->iph.ip_len);
  iph->ip_sum = in_checksum ((u_int16_t *) iph, iph_len);

  pal_mem_cpy (sendbuf, iph, iph_len);
  pal_mem_cpy ((sendbuf + iph_len),
                buf+UDP_PSEUDO_HEADER_SIZE,
                len+sizeof (struct pal_udp_header));

  return (len+ sizeof (struct pal_udp_header) + iph_len);
}

int
nsm_mpls_oam_echo_request_create_send (struct thread *t)
{
  struct nsm_mpls_oam *oam;
  int ret;
  u_char pnt[OAM_DATA_SIZE];
  u_char *pnt1;
  u_char sendbuf[OAM_DATA_SIZE];
  u_int16_t size;
  struct nsm_mpls_oam_data *data;
  u_int16_t len;
  u_int16_t pkt_size;
  struct  nsm_msg_mpls_tracert_reply tmsg;
  struct shimhdr *shim;
  struct listnode *node = NULL;
  bool_t trace = NSM_FALSE;

  oam = THREAD_ARG (t);
  data = XMALLOC (MTYPE_TMP, sizeof (struct nsm_mpls_oam_data));
  pal_mem_set (data, 0, sizeof (struct nsm_mpls_oam_data));
  pal_mem_set (pnt, 0, OAM_DATA_SIZE);
  pal_mem_set (sendbuf, 0, OAM_DATA_SIZE);

  pnt1=pnt;
  data->oam = oam;
  /* Get Current time */
  pal_time_tzcurrent (&data->sent_time, NULL);

  oam->count++;
  data->pkt_count = oam->count;
  oam->oam_echo_interval = NULL;
  /* function to set the TLV's in the echo request message */
  size = nsm_mpls_set_echo_request_data (oam, data);
  if (size < MPLS_OAM_ECHO_REQUEST_MIN_SIZE)
   {
     XFREE (MTYPE_TMP, data);
     nsm_mpls_oam_cleanup (oam);
     return NSM_FAILURE;
   }
  len = size;

  if (oam->msg.req_type == MPLSONM_OPTION_TRACE)
    trace = NSM_TRUE;

  ret = nsm_mpls_oam_encode_echo_request (&pnt1, &size, &oam->req, trace);
  if (ret < 0 )
    {
      send_nsm_mpls_oam_ping_error(oam, MPLS_OAM_ERR_PACKET_SEND);
      XFREE (MTYPE_TMP, data);
      nsm_mpls_oam_cleanup (oam);
      return NSM_FAILURE;
    }

  /* Set Checksum and correct IP header/UDP values */
  pkt_size = nsm_mpls_oam_prep_req_for_network (oam, pnt, len, sendbuf);

  if (IS_NSM_DEBUG_EVENT)
    nsm_mpls_oam_echo_request_dump (&oam->req, NSM_TRUE);

  ret = nsm_mpls_oam_echo_request_send (oam, sendbuf, pkt_size);
  if (ret < 0)
    {
      send_nsm_mpls_oam_ping_error(oam, MPLS_OAM_ERR_PACKET_SEND);
      XFREE (MTYPE_TMP, data);
      nsm_mpls_oam_cleanup (oam);
      return NSM_FAILURE;
    }
  else
    {
      /* Store the data pointers in a list to retrieve the required message 
         later */
      listnode_add (oam->oam_req_list, data);

      data->pkt_timeout = NULL;
      THREAD_TIMER_ON (oam->nm->zg, data->pkt_timeout,
                       nsm_mpls_oam_echo_timeout,
                       data, oam->msg.timeout);
    }

  /* Send Initial Trace Detail */
  if ((oam->msg.req_type  == MPLSONM_OPTION_TRACE) &&
      (oam->msg.ttl == 1))
    {
      pal_mem_set (&tmsg, 0, sizeof (struct nsm_msg_mpls_tracert_reply));
      ret = nsm_mpls_oam_set_trace_reply (&tmsg, NULL, NULL, oam,
                                          NULL, NSM_FALSE);
      if (ret < 0)
        return NSM_FAILURE;

      ret = nsm_encode_mpls_onm_trace_reply (&oam->nse->send.pnt,
                                             &oam->nse->send.size,
                                             &tmsg);
      if (ret < 0)
        return NSM_FAILURE;

      nsm_server_send_message (oam->nse, 0, 0, NSM_MSG_MPLS_TRACERT_REPLY,
                               0, ret);
      if (tmsg.ds_label)
        {
          if (!LIST_ISEMPTY (tmsg.ds_label))
            {
              LIST_LOOP (tmsg.ds_label, shim, node)
                {
                  listnode_delete (tmsg.ds_label, shim);
                  XFREE (MTYPE_TMP, shim);
                  node = tmsg.ds_label->head;
                  if (LIST_ISEMPTY (tmsg.ds_label))
                     break;
                }
            }
           list_delete(tmsg.ds_label);
        }
     }
  return NSM_SUCCESS;
}

/* Once a Request is received and parsed from a utility instance
 * a relevant main structure is created and added to the MPLS OAM
 * List. This list is referenced using the socket id of the utility process */
int
nsm_mpls_oam_process_echo_request (struct nsm_master *nm,
                                   struct nsm_server_entry *nse,
                                   struct nsm_msg_mpls_ping_req *msg)
{
  struct nsm_mpls_oam *oam;
  int ret = 0;

  oam = XCALLOC (MTYPE_TMP, sizeof (struct nsm_mpls_oam));
  if (oam == NULL)
    return NSM_FAILURE;
  pal_mem_set (oam, 0, sizeof (struct nsm_mpls_oam));
  /* Set the unique handle for OAM Instance */
  oam->sock = oamInstCount++;

  oam->nm = nm;
  oam->nse = nse;
 /* Add Struct to master list */
  listnode_add (NSM_MPLS->mpls_oam_list, oam);

  oam->oam_req_list = list_new();

  /* We need to check the received message and set default values if reqd */
  if (nsm_mpls_oam_set_defaults (oam, msg, nm) != NSM_SUCCESS)
    {
      nsm_mpls_oam_cleanup (oam);
      return NSM_FAILURE;
    }

  if (nsm_mpls_oam_set_ip_udp_hdr (oam, msg, nm) != NSM_SUCCESS)
    {
      nsm_mpls_oam_cleanup (oam);
      return NSM_FAILURE;
    }

  /* Set FTN Data */
  if ((ret = nsm_mpls_oam_get_tunnel_info (oam, msg)) < 0)
    {
      send_nsm_mpls_oam_ping_error(oam, ret);
      nsm_mpls_oam_cleanup (oam);
      return NSM_FAILURE;
    }

  if (nsm_mpls_set_echo_request_hdr (oam) != NSM_SUCCESS)
    {
      nsm_mpls_oam_cleanup (oam);
      return NSM_FAILURE;
    }

  /*  packet is sent without an interval */
  oam->count = 0;
  oam->oam_echo_interval =
               thread_add_timer (NSM_ZG, nsm_mpls_oam_echo_request_create_send,
                                 oam, 0);
  return NSM_SUCCESS;
}

int
nsm_mpls_oam_packet_read (struct nsm_master *nm, u_char *buf, int len,
                          struct mpls_oam_ctrl_data *ctrl_data,
                          struct pal_udp_ctrl_info *c_info)
 {
  int ret;
  int err_code = 0;
  struct mpls_oam_hdr *oam_hdr = NULL;
  struct mpls_oam_echo_request *oam_req = NULL;
  struct mpls_oam_echo_reply *oam_reply = NULL;
  struct pal_timeval current;
  u_char *pnt;
  u_int16_t size;
  u_char reply = NSM_FALSE;
  u_char trace_send = NSM_TRUE;

  oam_hdr = XMALLOC (MTYPE_TMP, sizeof (struct mpls_oam_hdr));
  if (! oam_hdr)
    {
      ret = NSM_ERR_MEM_ALLOC_FAILURE;
      goto err_ret;
    }

  oam_req = XMALLOC (MTYPE_TMP, sizeof (struct mpls_oam_echo_request));
  if (! oam_req)
    {
      ret = NSM_ERR_MEM_ALLOC_FAILURE;
      goto err_ret;
    }

  oam_reply = XMALLOC (MTYPE_TMP, sizeof (struct mpls_oam_echo_reply));
  if (! oam_reply)
    {
      ret = NSM_ERR_MEM_ALLOC_FAILURE;
      goto err_ret;
    }

  pal_mem_set (oam_reply, 0, sizeof (struct mpls_oam_echo_reply));
  pal_mem_set (oam_req, 0, sizeof (struct mpls_oam_echo_request));
  pal_mem_set (oam_hdr, 0, sizeof (struct mpls_oam_hdr));

  pal_time_tzcurrent (&current, NULL);

  if (len < MPLS_OAM_HDR_LEN)
    {
      zlog_err (NSM_ZG, "Packet size %d is incorrect", len);
      ret = -1;
      goto err_ret;
    }

  /* Now that the packet is here we need to identify it as an Echo Request
   * or a Echo reply and continue with the rest of the decode procedures
   * To that effect we decode the OAM header part first.
   */

  pnt = buf;
  size = len;

  ret = nsm_mpls_oam_header_decode (&pnt, &size, oam_hdr);
  if (ret < 0)
   {
      zlog_err (NSM_ZG, "Packet header decode failure");
      goto err_ret;;
   }

   if (oam_hdr->return_code == MPLS_OAM_RTR_IS_EGRESS_AT_DEPTH)
     trace_send = NSM_FALSE;

  /* Confirm that the version sent in the OAM Header is correct */
  if (oam_hdr->ver_no != MPLS_OAM_VERSION)
    {
      zlog_err (NSM_ZG, "Version mismatch found in echo packet received");
      ret = -1;
      goto err_ret;
    }

  /* Now we have a pnt with a fixed length of octets. Identify the message
   * type and proceed
   */

  if (oam_hdr->msg_type == MPLS_ECHO_REQUEST_MESSAGE)
    {
      if (( c_info) &&
          (!IN_LOOPBACK (pal_hton32(c_info->dst_addr.s_addr))))
       {
         ret = NSM_FAILURE;
         goto err_ret;
       }

      ret = nsm_mpls_oam_decode_echo_request (&pnt, &size, oam_hdr,
                                              oam_req, &err_code);

      if (ret < 0 || err_code > 0)
        {
          zlog_err (NSM_ZG, "OAM Packet Decode/Unkown TLV Error ");
          ret = nsm_mpls_oam_echo_reply_process (nm, oam_hdr, oam_req, &current,
                                                 NULL, err_code, 0, NULL,
                                                 ctrl_data);

           goto err_ret;
        }

      if (IS_NSM_DEBUG_EVENT)
	nsm_mpls_oam_echo_request_dump (oam_req, NSM_FALSE);

      nsm_mpls_oam_recv_echo_req (nm, oam_hdr, oam_req, &current, ctrl_data);
    }
  else if (oam_hdr->msg_type == MPLS_ECHO_REPLY_MESSAGE)
    {
      ret = nsm_mpls_oam_decode_echo_reply (&pnt, &size, oam_hdr,
                                            oam_reply, &reply);

      if (ret < 0)
        {
          zlog_err (NSM_ZG, " OAM REPLY Packet Decode Error");
          goto err_ret;
        }

      if (IS_NSM_DEBUG_EVENT)
	nsm_mpls_oam_echo_reply_dump (oam_reply, NSM_FALSE);

      nsm_mpls_oam_recv_echo_reply (nm, oam_reply, trace_send, &current, c_info);
    }

  XFREE (MTYPE_TMP, oam_hdr);
  XFREE (MTYPE_TMP, oam_req);
  XFREE (MTYPE_TMP, oam_reply);
  return 0;

err_ret:
  if (oam_hdr)
    XFREE (MTYPE_TMP, oam_hdr);

  if (oam_req)
    XFREE (MTYPE_TMP, oam_req);

  if (oam_reply)
    XFREE (MTYPE_TMP, oam_reply);

  return ret;
}

int
nsm_mpls_oam_udp_read (struct thread *t)
{
  u_char buf[UDP_BUFSIZ];
  struct mpls_oam_ctrl_data ctrl_data;
  struct pal_udp_ctrl_info c_info;
  int len =0;
  int sock;
  struct nsm_master *nm;
  sock = THREAD_FD (t);
  nm = THREAD_ARG (t);

  nm->nmpls->mpls_oam_read = NULL;
  THREAD_READ_ON (NSM_ZG, nm->nmpls->mpls_oam_read, nsm_mpls_oam_udp_read,
                  nm, sock);
  pal_mem_set (&c_info, 0, sizeof (struct pal_udp_ctrl_info));
  pal_mem_set (&ctrl_data, 0, sizeof (struct mpls_oam_ctrl_data));

  len = pal_in4_udp_recv_packet (sock, buf, &c_info);
  if (len < 0)
     {
       zlog_err (NSM_ZG, "%s: Error reading data on udp socket %d, errno = %s",
                 __FUNCTION__, MPLS_OAM_DEFAULT_PORT_UDP, pal_strerror(errno));
      return -1;
    }

  if (! CHECK_FLAG (c_info.c_flag, PAL_RECV_SRCADDR))
    {
      zlog_err (NSM_ZG, "%s: Warning : Source address not known", __FUNCTION__);
    }
  ctrl_data.src_addr = c_info.src.sin_addr;
  ctrl_data.in_ifindex = c_info.in_ifindex;
 
  nsm_mpls_oam_packet_read (nm, buf, len, &ctrl_data, &c_info);
  
  return NSM_SUCCESS;
}

int
nsm_mpls_oam_recv_echo_reply (struct nsm_master *nm,
                              struct mpls_oam_echo_reply *reply,
                              u_char trace_reply,
                              struct pal_timeval *rx_time,
                              struct pal_udp_ctrl_info *c_info)
{
  struct listnode *ln = NULL, *ln1 = NULL;
  struct nsm_mpls_oam *oam = NULL;
  struct nsm_mpls_oam_data *data = NULL;
  struct nsm_msg_mpls_ping_reply msg;
  int len = 0;
  struct nsm_msg_mpls_tracert_reply tmsg;
  bool_t last = NSM_FALSE;
  bool_t send_nsm = NSM_FALSE;
  bool_t trace_loop = NSM_FALSE;

  /* We have the list of messages initiated from different Utility instances
   * Need to find the exact instance that caused this reply to be sent
   */

  LIST_LOOP (NSM_MPLS->mpls_oam_list, oam, ln)
     {
       if (oam->sock == reply->omh.sender_handle)
         {
           /* Store incoming Source address for Reference */
           if (oam->r_src.s_addr == c_info->src.sin_addr.s_addr)
             {
               trace_loop = NSM_TRUE;
               trace_reply = NSM_FALSE;
             }
           else
             oam->r_src.s_addr = c_info->src.sin_addr.s_addr;

           /* From the main structure it will be possible to get the
            * sequence match of the echo request sent.
            */
           LIST_LOOP (oam->oam_req_list, data, ln1)
             {
               if (data->pkt_count == reply->omh.seq_number)
                 {
                   THREAD_TIMER_OFF (data->pkt_timeout);
                   oam->trace_timeout = NSM_FALSE;
                   send_nsm = NSM_TRUE;
                   break;
                 }
             }
           if (send_nsm)
             break;
         }
     }

   if(!send_nsm)
     return NSM_FAILURE;

   /* Delayed response so no data available */
   if (data == NULL)
     return NSM_FAILURE;
   /* Now that we have the message and the data we can send a reply to the
    * utility
    */
  if (oam->msg.req_type == MPLSONM_OPTION_PING)
    {
      pal_mem_set (&msg, 0, sizeof (struct nsm_msg_mpls_ping_reply));
      nsm_mpls_oam_set_ping_reply (&msg, reply, data, oam, rx_time);
      len = nsm_encode_mpls_onm_ping_reply (&oam->nse->send.pnt,
                                            &oam->nse->send.size,
                                            &msg);
      if (len < 0)
        return len;

      nsm_server_send_message (oam->nse, 0, 0, NSM_MSG_MPLS_PING_REPLY,
                               0, len);

      listnode_delete (oam->oam_req_list, data);
      XFREE (MTYPE_TMP, data);

     /* Now that the Reply has been received and sent to the utility */
     if (LIST_ISEMPTY(oam->oam_req_list) &&
         oam->count >= oam->msg.repeat)
       {
         nsm_mpls_oam_cleanup(oam);
       }

    }
  else
    {
     /* Now if reply is set then this is a Trace message so need to ping
       * again. This time with an incremented ttl value.
       */
      if ((oam->msg.ttl < oam->max_ttl) && (trace_reply))
        {
          oam->msg.ttl++;

          if (nsm_mpls_set_echo_request_hdr (oam) != NSM_SUCCESS)
            {
              nsm_mpls_oam_cleanup (oam);
              return -1;
            }

         /* Need to send the new DSMAP TLV - else DSMAP MISTMATCH occurs */
         if (oam_req_dsmap_tlv == NSM_TRUE)
           pal_mem_cpy (&oam->req.dsmap, &reply->dsmap,
                        sizeof (struct mpls_oam_downstream_map));

         oam->oam_echo_interval =
               thread_add_timer (NSM_ZG, nsm_mpls_oam_echo_request_create_send,
                                 oam, 0);
        }
      else
        last = NSM_TRUE;

      if (! trace_loop)
        {
          pal_mem_set (&tmsg, 0, sizeof (struct nsm_msg_mpls_tracert_reply));
          len = nsm_mpls_oam_set_trace_reply (&tmsg, reply, data, oam,
                                              rx_time, last);
          if (len < 0)
            return len;
          
          len = nsm_encode_mpls_onm_trace_reply (&oam->nse->send.pnt,
                                                 &oam->nse->send.size,
                                                 &tmsg);
          if (len < 0)
            return len;

          nsm_server_send_message (oam->nse, 0, 0, NSM_MSG_MPLS_TRACERT_REPLY,
                               0, len);
          listnode_delete (oam->oam_req_list, data);
          XFREE (MTYPE_TMP, data);
        }
      else
        {
          send_nsm_mpls_oam_ping_error (oam, MPLS_OAM_ERR_TRACE_LOOP);
        }
      if (last)
      {
        nsm_mpls_oam_cleanup(oam);
      }
    }

  return NSM_SUCCESS;
}

int
nsm_mpls_oam_echo_reply_send (struct nsm_master *nm,
                              struct mpls_oam_hdr *omh,
                              struct mpls_oam_ctrl_data *ctrl_data,
                              u_char *pnt, u_int16_t size)
{
  int ret = 0;
  u_char buf[OAM_DATA_SIZE];
  u_char sendbuf[OAM_DATA_SIZE];
  struct pal_in4_header reply_iph;
  struct pal_in4_header *iph;
  struct pal_udp_header reply_udph;
  struct apn_vrf *iv;
  u_char iph_with_ra[sizeof (struct pal_in4_header) + OAM_IPOPT_RA_LEN];
  u_char opt[OAM_IPOPT_RA_LEN]={OAM_IPOPT_RA, OAM_IPOPT_RA_LEN, 0, 0};

  iv = apn_vrf_lookup_default (nm->vr);
  if (!iv)
    return NSM_FAILURE;

  pal_mem_set (sendbuf, 0, OAM_DATA_SIZE);
  pal_mem_set (buf, 0, OAM_DATA_SIZE);
  pal_mem_set (iph_with_ra, 0,
               sizeof (struct pal_in4_header) + OAM_IPOPT_RA_LEN);

  pal_mem_set (&reply_iph, 0, sizeof (struct pal_in4_header));
  reply_iph.ip_v = IPVERSION;
  reply_iph.ip_id = 0;
  reply_iph.ip_off = 0;
  reply_iph.ip_p = IPPROTO_UDP;
  reply_iph.ip_sum = 0;

   /* Set Destination */
  reply_iph.ip_dst.s_addr = ctrl_data->src_addr.s_addr;

  reply_iph.ip_src = iv->router_id;

  reply_iph.ip_ttl = 255;

  /* IP length is reset once we get the total packet length */

  /* Set the UDP header */
  reply_udph.uh_sport = pal_hton16 (MPLS_OAM_DEFAULT_PORT_UDP);
  reply_udph.uh_dport = pal_hton16 (MPLS_OAM_DEFAULT_PORT_UDP);
  reply_udph.uh_ulen = size + (sizeof (struct pal_udp_header));
  reply_udph.uh_ulen = pal_hton16 (reply_udph.uh_ulen);
  reply_udph.uh_sum = 0;

  pal_mem_cpy (buf+UDP_PSEUDO_HEADER_SIZE, &reply_udph,
               sizeof (struct pal_udp_header));
  pal_mem_cpy ((buf+UDP_PSEUDO_HEADER_SIZE+ sizeof (struct pal_udp_header)),
                 pnt, size);

  if (omh->reply_mode == MPLS_OAM_IP_UDP_RA_REPLY)
    {
      pal_in4_ip_hdr_len_set (&reply_iph, OAM_IPOPT_RA_LEN);
      pal_mem_cpy (iph_with_ra, &reply_iph, sizeof (struct pal_in4_header));
      pal_mem_cpy (&iph_with_ra[sizeof (struct pal_in4_header)],
                   &opt, OAM_IPOPT_RA_LEN);
      iph = (struct pal_in4_header *) iph_with_ra;;
    }
  else
    {
      pal_in4_ip_hdr_len_set (&reply_iph, 0);
      iph = &reply_iph;
    }

  pal_in4_ip_packet_len_set (iph, size + sizeof (struct pal_udp_header));

  oam_udp_checksum (&reply_udph, iph, buf,
                    (size + (sizeof (struct pal_udp_header))));

  ret = pal_in4_send_packet (NSM_MPLS->oam_s_sock, iph,
                             sizeof (struct pal_in4_header),
                             buf + UDP_PSEUDO_HEADER_SIZE,
                             size + sizeof (struct pal_udp_header), 0);
  if (ret < 0)
    zlog_warn (NSM_ZG, "SEND packet failed %s", pal_strerror (errno));

 return NSM_SUCCESS;
}

int
nsm_mpls_oam_reply_tlv_set (struct nsm_master *nm,
			    struct mpls_oam_echo_request *oam_req, 
			    struct mpls_oam_echo_reply *oam_reply,
			    int *bytes_to_copy, struct interface *ifp,
			    u_char return_code,
			    struct mpls_label_stack *out_label,
                            struct mpls_oam_ctrl_data *ctrl_data)
{
  int ret = 0;
  struct interface *t_ifp = NULL; 

  if (oam_reply_tlv == NSM_TRUE)
    {
      ret = nsm_mpls_oam_reply_fec_tlv_set (oam_req, &oam_reply->fec_tlv);
      if (ret < 0)
	return ret;
    }

  *bytes_to_copy+=ret;

  if (CHECK_FLAG (oam_req->flags, MPLS_OAM_FLAGS_PAD_TLV))
    {
      if (oam_req->pad_tlv.val == 1)
        {
          ret = nsm_mpls_oam_reply_pad_tlv_set (&oam_req->pad_tlv,
                                                &oam_reply->pad_tlv);
          if (ret < 0)
            return ret;

          *bytes_to_copy+=ret;
          SET_FLAG (oam_reply->flags, MPLS_OAM_FLAGS_PAD_TLV);
        }
    }

  if (return_code > MPLS_OAM_ERRORED_TLV)
    {
      if (CHECK_FLAG (oam_req->flags, MPLS_OAM_FLAGS_DSMAP_TLV))
        {
          ret = nsm_mpls_oam_reply_if_tlv_set (nm, &oam_reply->if_lbl_stk,
                                               ctrl_data);
          if (ret < 0)
            return ret;

          *bytes_to_copy+=ret;
          SET_FLAG (oam_reply->flags, MPLS_OAM_FLAGS_IF_LBL_STK_TLV);
        }

      /* If the Echo Request contained a DSMAP we ned to proceeed based
       * on that */
      if ((return_code != MPLS_OAM_RTR_IS_EGRESS_AT_DEPTH ) &&
         (CHECK_FLAG (oam_req->flags, MPLS_OAM_FLAGS_DSMAP_TLV)))
        {
          if (oam_req->dsmap.ds_if_ip.s_addr == pal_hton32 (INADDR_LOOPBACK))
            {
              return_code = MPLS_OAM_UPSTREAM_IF_UNKNOWN;
              ret = nsm_mpls_oam_reply_dsmap_tlv_set (oam_req, &oam_reply->dsmap,
                                                     out_label, ifp, ctrl_data);
              if (ret < 0)
                return ret;

              *bytes_to_copy+=ret;
              SET_FLAG (oam_reply->flags, MPLS_OAM_FLAGS_DSMAP_TLV);

            }
          else if (oam_req->dsmap.ds_if_ip.s_addr ==
                    pal_hton32 (INADDR_ALLRTRS_GROUP))
            {
              /* NO validations are done */
              ret = nsm_mpls_oam_reply_dsmap_tlv_set (oam_req,
                                                     &oam_reply->dsmap,
                                                     out_label, ifp,
                                                     ctrl_data);
               if (ret < 0)
                 return ret;

               *bytes_to_copy+=ret;
               SET_FLAG (oam_reply->flags, MPLS_OAM_FLAGS_DSMAP_TLV);
            }
          else
            {
              if (! ifp)
                return -1;

              t_ifp = ifg_lookup_by_index (&nm->zg->ifg, ctrl_data->in_ifindex);
              if (! t_ifp)
                return -1;

              /* Do some stack validation ? */
              if (oam_req->dsmap.ds_if_ip.s_addr ==
                  t_ifp->ifc_ipv4->address->u.prefix4.s_addr)
                {
                  ret = nsm_mpls_oam_reply_dsmap_tlv_set (oam_req,
                                                          &oam_reply->dsmap,
                                                          out_label, ifp,
                                                          ctrl_data);
                  if (ret < 0)
                    return ret;

                  *bytes_to_copy+=ret;
                  SET_FLAG (oam_reply->flags, MPLS_OAM_FLAGS_DSMAP_TLV);
                }
              else
                {
                  return_code = MPLS_OAM_DSMAP_MISMATCH;
                  ret = nsm_mpls_oam_reply_dsmap_tlv_set (oam_req,
                                                          &oam_reply->dsmap,
                                                          out_label, ifp,
                                                          ctrl_data);
                  if (ret < 0)
                    return ret;

                  *bytes_to_copy+=ret;
                  SET_FLAG (oam_reply->flags, MPLS_OAM_FLAGS_DSMAP_TLV);
                }
            }
        }
    }

  return 1;
}

int
nsm_mpls_oam_echo_reply_process (struct nsm_master *nm,
                                 struct mpls_oam_hdr *omh,
                                 struct mpls_oam_echo_request *oam_req,
                                 struct pal_timeval *rx_time,
                                 struct interface *ifp,
                                 u_char return_code,
                                 u_char ret_sub_code,
                                 struct mpls_label_stack *out_label,
                                 struct mpls_oam_ctrl_data *ctrl_data)
{
  int ret = 0;
  u_int16_t size;
  int bytes_to_copy = 0;
  u_char pnt[OAM_DATA_SIZE];
  u_char *pnt1;
  struct mpls_oam_echo_reply oam_reply;

  pnt1 = pnt;

  pal_mem_set (pnt, 0, OAM_DATA_SIZE);
  pal_mem_set (&oam_reply, 0 , sizeof (struct mpls_oam_echo_reply));

  ret = nsm_mpls_oam_reply_tlv_set (nm, oam_req, &oam_reply, &bytes_to_copy,
				    ifp, return_code, out_label, ctrl_data);

  if (ret < 0)
    return NSM_FAILURE;

  ret = nsm_mpls_set_echo_reply_hdr (&oam_req->omh, &oam_reply.omh,
                                     rx_time, return_code,
                                     ret_sub_code);
  if (ret < 0)
    return NSM_FAILURE;

  bytes_to_copy+=ret;

  size = bytes_to_copy;
  ret = nsm_mpls_oam_encode_echo_reply (&pnt1, &size, &oam_reply);
  if (ret < 0 )
    return NSM_FAILURE;

  if (IS_NSM_DEBUG_EVENT)
    nsm_mpls_oam_echo_reply_dump (&oam_reply, NSM_TRUE);

  nsm_mpls_oam_echo_reply_send (nm,omh, ctrl_data, pnt, bytes_to_copy);

  return NSM_SUCCESS;
}

int
nsm_mpls_oam_set_ip_udp_hdr (struct nsm_mpls_oam *oam,
                                 struct nsm_msg_mpls_ping_req *msg,
                                 struct nsm_master *nm)
{
  u_int16_t iph_len;

  /* IP Header is created and stored in the main struct */

  iph_len  = sizeof (struct pal_in4_header);

  oam->iph.ip_hl = iph_len >>2;
  oam->iph.ip_v = IPVERSION;
  oam->iph.ip_id = 0;
  oam->iph.ip_off = 0;
  oam->iph.ip_p = IPPROTO_UDP;
  oam->iph.ip_sum = 0;

  oam->iph.ip_src.s_addr = msg->src.s_addr;
  oam->iph.ip_dst.s_addr = msg->dst.s_addr;

  oam->iph.ip_ttl = 1;

  /* UDP Header */
  oam->udph.uh_sport = pal_hton16 (MPLS_OAM_DEFAULT_PORT_UDP);
  oam->udph.uh_dport = pal_hton16 (MPLS_OAM_DEFAULT_PORT_UDP);
  oam->udph.uh_ulen = 0;
  oam->udph.uh_sum = 0;
  return NSM_SUCCESS;

}

void
nsm_mpls_oam_cleanup (struct nsm_mpls_oam *oam)
{
  struct listnode *node;
  struct shimhdr *shim;
  if (! oam)
    return;

  if (oam && oam->ftn && oam->ftn->xc)
    XFREE (MTYPE_TMP, oam->ftn->xc->nhlfe);

  if (oam && oam->ftn)
    XFREE (MTYPE_TMP, oam->ftn->xc);

  if (oam)
    XFREE (MTYPE_TMP, oam->ftn);
  if (oam->req.dsmap.ds_label)
    {
       if (!LIST_ISEMPTY (oam->req.dsmap.ds_label))
        {
          LIST_LOOP (oam->req.dsmap.ds_label, shim, node)
            {
              listnode_delete (oam->req.dsmap.ds_label, shim);
              XFREE (MTYPE_TMP, shim);
              node = oam->req.dsmap.ds_label->head;
              if (LIST_ISEMPTY (oam->req.dsmap.ds_label))
                break;
            }
        }
       list_delete(oam->req.dsmap.ds_label);
    }
  if (oam->req.dsmap.multi_info)
    XFREE (MTYPE_TMP, oam->req.dsmap.multi_info);

   if (oam->msg.type == MPLSONM_OPTION_L3VPN)
     { 
       if (oam->msg.u.fec.vrf_name)
         XFREE (MTYPE_VRF_NAME, oam->msg.u.fec.vrf_name);
     }
   else if (NSM_CHECK_CTYPE (oam->msg.cindex,
                             NSM_CTYPE_MSG_MPLSONM_OPTION_TUNNELNAME))
     { 
       if (oam->msg.u.rsvp.tunnel_name) 
         XFREE (MTYPE_TMP, oam->msg.u.rsvp.tunnel_name);
     }
         
  list_delete (oam->oam_req_list);
  listnode_delete (oam->nm->nmpls->mpls_oam_list, oam);
  XFREE (MTYPE_TMP, oam);
  oam = NULL;
}

int
nsm_mpls_oam_route_ipv4_lookup (struct nsm_master *nm, vrf_id_t vrf_id,
                                struct pal_in4_addr *ipv4_addr,
                                u_char prefixlen)
{
  struct prefix p;
  struct nsm_vrf *vrf = NULL;
  struct nsm_ptree_node *rn;

  vrf = nsm_vrf_lookup_by_id (nm, vrf_id);

  if (! vrf)
    return NSM_FAILURE;

  p.family = AF_INET;
  p.prefixlen = prefixlen;
  IPV4_ADDR_COPY (&p.u.prefix4, ipv4_addr);

  rn = nsm_ptree_node_lookup (vrf->RIB_IPV4_UNICAST,
                             (u_char *) &p.u.prefix4,
                             p.prefixlen);

  if (rn && rn->info != NULL)
    return NSM_SUCCESS;
  else
    return NSM_FAILURE;
}

int
nsm_mpls_oam_transit_stack_lookup (struct nsm_master *nm,
                                   struct mpls_oam_ctrl_data *ctrl_data,
                                   struct mpls_label_stack *local_stack,
                                   struct prefix *p,
                                   u_char *send_msg, u_char owner)
{
  int i;
  struct shimhdr *out_label;
  struct ilm_entry *ilm = NULL;
  struct gmpls_gen_label lbl;

  local_stack->label = list_new ();

  lbl.type = gmpls_entry_type_ip;
  for (i = 0; i < ctrl_data->label_stack_depth; i++)
     {
       lbl.u.pkt = ctrl_data->label_stack[i];
       ilm = nsm_gmpls_ilm_lookup_by_owner (nm, ctrl_data->in_ifindex,
                                            &lbl, owner);
       if (ilm)
         {
           if ((p->family == ilm->family) &&
               (p->prefixlen == ilm->prefixlen))
             {
#ifdef HAVE_IPV6
               if (p->family == AF_INET6)
                 {
                   if (pal_mem_cmp (&p->u.prefix6, ilm->prfx, 
                                    sizeof (struct pal_in6_addr)) != 0)
                     continue;
                 } 
               else
#endif
                 if (pal_mem_cmp (&p->u.prefix4, ilm->prfx, 
                                  sizeof (struct pal_in4_addr)) != 0)
                   continue;

               if (((p->family == AF_INET) && 
                     IPV4_ADDR_SAME(ilm->prfx, &p->u.prefix4))
#ifdef HAVE_IPV6
                  || ((p->family == AF_INET6) && 
                       IPV6_ADDR_SAME(ilm->prfx, &p->u.prefix6))
#endif /* HAVE_IPV6 */                   
                )
                 { 
                   /* Populate the Out labels set */
                   local_stack->seq_num = i;
                   local_stack->ifindex = ilm->xc->nhlfe->key.u.ipv4.oif_ix;
                   out_label = XMALLOC (MTYPE_TMP, sizeof (struct shimhdr));
                   set_label_net (out_label->shim,
                                  ilm->xc->nhlfe->key.u.ipv4.out_label)
                   listnode_add (local_stack->label, out_label);
                   *send_msg = NSM_TRUE;
                   return i;
                 }

             }
           else
             continue;
         }
       else
         {
           zlog_warn (NSM_ZG, "Illegal FEC Ping recevied: %r",
                      &p->u.prefix4);
           *send_msg = NSM_FALSE;
         }

     }
  return -1;
}

int
nsm_mpls_oam_process_l3vpn_tlv (struct nsm_master *nm,
                                struct mpls_oam_hdr *oam_hdr,
                                struct mpls_oam_echo_request *oam_req,
                                struct pal_timeval *rx_time,
                                struct mpls_oam_ctrl_data *ctrl_data)
{
  int ret;
  vrf_id_t vrf_id;
  struct listnode *ln;
  struct nsm_msg_vpn_rd *rd_node = NULL;
  struct mpls_label_stack *local_stack = NULL;
  struct interface *ifp = NULL;

  u_int8_t return_code = MPLS_OAM_DEFAULT_RET_CODE;
  u_int8_t ret_sub_code = 0;
  int stack_depth = 1;
  u_char send_msg = NSM_FALSE;

  LIST_LOOP (NSM_MPLS->mpls_vpn_list, rd_node, ln)
    {
      if (VPN_RD_SAME (&rd_node->rd, &oam_req->fec_tlv.l3vpn.rd))
        {
          vrf_id = rd_node->vrf_id;
          ret = nsm_mpls_oam_route_ipv4_lookup (nm, vrf_id,
                                               &oam_req->fec_tlv.l3vpn.vpn_pfx,
                                               oam_req->fec_tlv.l3vpn.u.val.len);

          if (ret == NSM_SUCCESS)
            {
              return_code = MPLS_OAM_RTR_IS_EGRESS_AT_DEPTH;
              ret_sub_code = stack_depth;
              send_msg = NSM_TRUE;
              break;
            }
        }
    }

  /* Outer Tunnel case */
  if (! rd_node)
    {
      if (CHECK_FLAG(oam_req->fec_tlv.flags, MPLS_OAM_RSVP_IPv4_PREFIX))
        {
          /* Outer tunnel is RSVP */
          ret = nsm_mpls_oam_process_rsvp_ipv4_tlv (nm, oam_hdr, oam_req,
                                                    rx_time, ctrl_data,
                                                    NSM_FALSE);
          if (ret < 0)
            {
              zlog_warn (NSM_ZG, " VPN Data and outer tunnel not "
                         "configured on LSR ");
              return ret;
             }
        }
      else if (CHECK_FLAG (oam_req->fec_tlv.flags, MPLS_OAM_LDP_IPv4_PREFIX))
        {
          /* Outer Tunnel is LDP */
          ret = nsm_mpls_oam_process_ldp_ipv4_tlv (nm, oam_hdr, oam_req,
                                                   rx_time, ctrl_data,
                                                   NSM_TRUE);
          if (ret < 0)
            {
              zlog_warn (NSM_ZG, " VPN Data and outer tunnel not "
                         "configured on LSR ");
              return ret;
            }
        }
      else
        {
          /* No Outer Tunnel  Do global Route lookup */
          ret = nsm_mpls_oam_route_ipv4_lookup (nm, 0,
                                               &oam_req->fec_tlv.l3vpn.vpn_pfx,
                                               oam_req->fec_tlv.l3vpn.u.val.len);

          if (ret == NSM_SUCCESS)
            {
              return_code = MPLS_OAM_LBL_SWITCH_AT_DEPTH;
              ret_sub_code = 0;
              send_msg = NSM_TRUE;
            }
          else
            {
              zlog_warn (NSM_ZG, "No Route to Send VPN PING");
            }
        }

    }

  /* Send the echo reply */
  if (send_msg)
    {
      ret = nsm_mpls_oam_echo_reply_process (nm, oam_hdr, oam_req,
                                             rx_time, ifp, return_code,
                                             ret_sub_code, local_stack,
                                             ctrl_data);
      if ( ret < 0)
        return ret;
    }

  return NSM_SUCCESS;
}

int
nsm_mpls_oam_process_ldp_ipv4_tlv (struct nsm_master *nm,
                                   struct mpls_oam_hdr *oam_hdr,
                                   struct mpls_oam_echo_request *oam_req,
                                   struct pal_timeval *rx_time,
                                   struct mpls_oam_ctrl_data *ctrl_data,
                                   u_char lookup)
{
  int ret;
  struct prefix *p, pfx;
  struct ftn_entry *ftn;
  struct mpls_label_stack local_stack;
  struct interface *ifp;

  int stack_depth = 1;
  u_int8_t return_code = MPLS_OAM_DEFAULT_RET_CODE;
  u_int8_t ret_sub_code = 0;
  u_char send_msg = NSM_FALSE;

  pal_mem_set(&local_stack, 0, sizeof (struct mpls_label_stack));
  p = &pfx;
  p->family = AF_INET;
  p->prefixlen = oam_req->fec_tlv.ldp.u.val.len;
  IPV4_ADDR_COPY (&p->u.prefix4, &oam_req->fec_tlv.ldp.ip_addr);

  ftn = nsm_gmpls_get_ldp_ftn (nm, p);
  if (! ftn)
    {
      /* Egress processing */
      /* DO an IP Route lookup */
      ret = nsm_mpls_oam_route_ipv4_lookup (nm, 0,
                                           &oam_req->fec_tlv.ldp.ip_addr,
                                           oam_req->fec_tlv.ldp.u.val.len);
      if (ret == NSM_SUCCESS)
        {
          if (lookup == NSM_FALSE)
            return_code = MPLS_OAM_RTR_IS_EGRESS_AT_DEPTH;
          else 
            return_code = MPLS_OAM_LBL_SWITCH_IP_FWD_AT_DEPTH;

          ret_sub_code = 0;
          send_msg = NSM_TRUE;
         }
       else
         {
           /* There is no Route of any kind on the Machine
            * This is an error condition
            */
           zlog_warn (NSM_ZG, "Illegal FEC Ping recevied: %r",
                      &oam_req->fec_tlv.ldp.ip_addr);
           send_msg = NSM_FALSE;
         }
    }
  else
    {
      /* We have an FTN so we might be a transit
       * Do ILM lookup to confirm
       */
      ret = nsm_mpls_oam_transit_stack_lookup (nm, ctrl_data, &local_stack,
                                               p, &send_msg, MPLS_LDP);
      if (ret >= 0)
        {
          stack_depth = ret;
          return_code = MPLS_OAM_LBL_SWITCH_AT_DEPTH;
          ret_sub_code = stack_depth;
        }
    }

 if (send_msg)
   {
      ifp = ifg_lookup_by_index (&nm->zg->ifg, local_stack.ifindex);
      ret = nsm_mpls_oam_echo_reply_process (nm, oam_hdr, oam_req,
                                             rx_time, ifp, return_code,
                                             ret_sub_code, &local_stack,
                                             ctrl_data);
      if ( ret < 0)
        return ret;
   }

  return NSM_SUCCESS;
}

int
nsm_mpls_oam_process_rsvp_ipv4_tlv (struct nsm_master *nm,
                                    struct mpls_oam_hdr *oam_hdr,
                                    struct mpls_oam_echo_request *oam_req,
                                    struct pal_timeval *rx_time,
                                    struct mpls_oam_ctrl_data *ctrl_data,
                                    u_char lookup)
{
  int ret;
  struct prefix *p, pfx;
  struct mpls_label_stack local_stack;
  struct interface *ifp, *t_ifp;

  int stack_depth = 1;
  u_int8_t return_code = MPLS_OAM_DEFAULT_RET_CODE;
  u_int8_t ret_sub_code = 0;
  u_char send_msg = NSM_FALSE;

  pal_mem_set(&local_stack, 0, sizeof (struct mpls_label_stack));
  p = &pfx;
  p->family = AF_INET;
  p->prefixlen = IPV4_MAX_PREFIXLEN;
  IPV4_ADDR_COPY (&p->u.prefix4, &oam_req->fec_tlv.rsvp.ip_addr);

  t_ifp = if_lookup_by_prefix (&nm->vr->ifm, p);
  if (t_ifp)
    {
      /* Egress Processing */
      return_code = MPLS_OAM_RTR_IS_EGRESS_AT_DEPTH;
      ret_sub_code = stack_depth;
      send_msg = NSM_TRUE;
    }
  else
    {
      /* We dont have an Interface with the Address configured
       * We might be a transit. Do ILM to confirm.
       */
      ret = nsm_mpls_oam_transit_stack_lookup (nm, ctrl_data, &local_stack,
                                               p, &send_msg, MPLS_RSVP);
      if (ret >= 0)
        {
          stack_depth = ret;
          return_code = MPLS_OAM_LBL_SWITCH_AT_DEPTH;
          ret_sub_code = stack_depth;
        }
    }

  if (send_msg && (! lookup))
    {
      ifp = ifg_lookup_by_index (&nm->zg->ifg, local_stack.ifindex);
      ret = nsm_mpls_oam_echo_reply_process (nm, oam_hdr, oam_req,
                                             rx_time, ifp, return_code,
                                             ret_sub_code, &local_stack,
                                             ctrl_data);
      if ( ret < 0)
        return ret;
    }

  return NSM_SUCCESS;
}

#ifdef HAVE_MPLS_VC
int
nsm_mpls_oam_process_l2data_tlv (struct nsm_master *nm,
                                 struct mpls_oam_hdr *oam_hdr,
                                 struct mpls_oam_echo_request *oam_req,
                                 struct pal_timeval *rx_time,
                                 struct mpls_oam_ctrl_data *ctrl_data)
{
  int ret;
  struct nsm_mpls_circuit *vc;
  struct nsm_vpls *vpls = NULL;
  u_int32_t vc_id;
  u_int8_t stack_depth = 1;
  struct addr_in addr;
  struct pal_in4_addr source;
  struct mpls_label_stack *local_stack = NULL;
  struct interface *ifp = NULL;
  bool_t send_msg = NSM_FALSE;

  u_int8_t return_code = MPLS_OAM_DEFAULT_RET_CODE;
  u_int8_t ret_sub_code = 0;
  addr.afi = AFI_IP;

  if (oam_req->fec_tlv.l2_dep.tlv_h.type == MPLS_OAM_FEC128_DEP_TLV)
    {
      vc_id = oam_req->fec_tlv.l2_dep.vc_id;
      IPV4_ADDR_COPY (&addr.u.ipv4, &oam_req->fec_tlv.l2_dep.remote);
      IPV4_ADDR_COPY (&source, &ctrl_data->src_addr);
    }
  else
    {
      vc_id = oam_req->fec_tlv.l2_curr.vc_id;
      IPV4_ADDR_COPY (&addr.u.ipv4, &oam_req->fec_tlv.l2_curr.remote);
      IPV4_ADDR_COPY (&source, &oam_req->fec_tlv.l2_curr.source);
    }

  /* Need to do a VC lookup by id */
  vc = nsm_mpls_l2_circuit_lookup_by_id (nm, vc_id);

  if ( vc)
    {
      if (! IPV4_ADDR_SAME (&source, &vc->address.u.prefix4))
        {
          zlog_warn (NSM_ZG, "VC Ping recived for vc id [%u] with illegal "
                     "Source Addres [%r]", vc_id, &source);

        }
      else
        {
          return_code = MPLS_OAM_RTR_IS_EGRESS_AT_DEPTH;
          ret_sub_code = stack_depth;
          send_msg = NSM_TRUE;
        }
    }
#ifdef HAVE_VPLS
  else
    {
      /* Since VPLS is not separately mapped lets do a lookup for that */
      vpls = nsm_vpls_lookup (nm, vc_id, NSM_FALSE);
      if (! vpls)
        return_code = MPLS_OAM_NO_MAPPING_AT_DEPTH;
      else
        {
          struct nsm_vpls_peer *peer;
          /* Need to find the peer */
          peer = nsm_vpls_mesh_peer_lookup (vpls, &addr, NSM_FALSE);
          if (! peer)
            return_code = MPLS_OAM_NO_MAPPING_AT_DEPTH;
          else
           {
             return_code = MPLS_OAM_RTR_IS_EGRESS_AT_DEPTH;
             send_msg = NSM_TRUE;
           }
        }
    }
#endif /* HAVE_VPLS */

  if (!vc && ! vpls)
    {
      if (CHECK_FLAG (oam_req->fec_tlv.flags, MPLS_OAM_RSVP_IPv4_PREFIX))
        {
          /* Outer tunnel is RSVP */
          ret = nsm_mpls_oam_process_rsvp_ipv4_tlv (nm, oam_hdr, oam_req,
                                                    rx_time, ctrl_data,
                                                    NSM_FALSE);
          if (ret < 0)
            {
              zlog_warn (NSM_ZG, " VC Data and outer tunnel not configured"
                       " on LSR ");
              return ret;
            }
        }
      else if (CHECK_FLAG (oam_req->fec_tlv.flags, MPLS_OAM_LDP_IPv4_PREFIX))
        {
          /* Outer Tunnel is LDP */
          ret = nsm_mpls_oam_process_ldp_ipv4_tlv (nm, oam_hdr, oam_req,
                                                   rx_time, ctrl_data,
                                                   NSM_TRUE);
          if (ret < 0)
            {
              zlog_warn (NSM_ZG, " VC Data and outer tunnel not configured"
                        " on LSR ");
              return ret;
            }
        }
      else
        {
          ret = nsm_mpls_oam_route_ipv4_lookup (nm, 0,
                                                &addr.u.ipv4,
                                                IPV4_MAX_PREFIXLEN);
          if (ret == NSM_SUCCESS)
            {
              return_code = MPLS_OAM_LBL_SWITCH_AT_DEPTH;
              ret_sub_code = 0;
              send_msg = NSM_TRUE;
              ret = nsm_mpls_oam_echo_reply_process (nm, oam_hdr, oam_req,
                                                     rx_time, ifp, return_code,
                                                     ret_sub_code, local_stack,
                                                     ctrl_data);
 	      if (ret < 0)
 		return ret;

             }
           else
             zlog_warn (NSM_ZG, "No Route to Send VC PING");
        }
    }
  else if (send_msg)
    {
      if (vc)
        ifp = ifg_lookup_by_index (&nm->zg->ifg, vc->vc_fib->ac_if_ix);

      ret = nsm_mpls_oam_echo_reply_process (nm, oam_hdr, oam_req,
                                             rx_time, ifp, return_code,
                                             ret_sub_code, local_stack,
                                             ctrl_data);
      if ( ret < 0)
        return ret;
    }

  return NSM_SUCCESS;
}

#ifdef HAVE_VPLS
int
nsm_mpls_oam_send_vpls_echo_request (struct nsm_vpls *vpls,
                                        struct pal_in4_addr *addr,
                                        u_int32_t flags, u_int32_t ttl,
                                        u_int32_t pkt_size, u_char *pkt)
{
  struct nsm_vpls_peer *peer;
  struct nsm_vpls_spoke_vc *svc;
  struct route_node *rn;
  struct listnode *ln;
  int ret = 0;

  if (vpls->mp_table)
    {
      for (rn = route_top (vpls->mp_table); rn; rn = route_next(rn))
        {
          if ((peer = rn->info) == NULL)
            continue;

          if (peer->vc_fib &&
              IPV4_ADDR_SAME (&peer->peer_addr.u.ipv4, addr))
            {
              ret = apn_mpls_oam_vc_send_echo_request (peer->vc_fib->ac_if_ix,
                                                      vpls->vpls_id, flags,
                                                      ttl, pkt_size, pkt);

              return ret;
            }
        }
    }
  if (vpls->svc_list)
    {
      LIST_LOOP (vpls->svc_list, svc, ln)
        {
          if (svc->vc_fib &&
             IPV4_ADDR_SAME (&svc->vc->address.u.prefix4, addr))
            {
              ret = apn_mpls_oam_vc_send_echo_request (svc->vc_fib->ac_if_ix,
                                                       vpls->vpls_id, flags,
                                                       ttl, pkt_size, pkt);
              return ret;
            }
        }
    }
  return ret;
}
#endif /* HAVE_VPLS */
#endif /* HAVE_MPLS_VC */

int
nsm_mpls_oam_process_gen_ipv4_tlv (struct nsm_master *nm,
                                   struct mpls_oam_hdr *oam_hdr,
                                   struct mpls_oam_echo_request *oam_req,
                                   struct pal_timeval *rx_time,
                                   struct mpls_oam_ctrl_data *ctrl_data,
                                   u_char lookup)
{
  int ret;
  struct prefix *p, pfx;
  struct mpls_label_stack local_stack;
  struct interface *ifp, *t_ifp;

  int stack_depth = 1;
  u_int8_t return_code = MPLS_OAM_DEFAULT_RET_CODE;
  u_int8_t ret_sub_code = 0;
  u_char send_msg = NSM_FALSE;

  pal_mem_set(&local_stack, 0, sizeof (struct mpls_label_stack));
  p = &pfx;
  p->family = AF_INET;
  p->prefixlen = IPV4_MAX_PREFIXLEN;
  IPV4_ADDR_COPY (&p->u.prefix4, &oam_req->fec_tlv.ipv4.ipv4_addr);

  t_ifp = if_lookup_by_prefix (&nm->vr->ifm, p);
  if (t_ifp)
    {
      /* Egress Processing */
      return_code = MPLS_OAM_RTR_IS_EGRESS_AT_DEPTH;
      ret_sub_code = stack_depth;
      send_msg = NSM_TRUE;
    }
  else
    {
      /* We dont have an Interface with the Address configured
       * We might be a transit. Do ILM to confirm.
       */
      int ret1 = 0;
      ret = nsm_mpls_oam_transit_stack_lookup (nm, ctrl_data, &local_stack,
                                               p, &send_msg, MPLS_SNMP);
      if (ret < 0)
        {
          /* Just Checking for the other CLI cases of LSP's */
          ret1 = nsm_mpls_oam_transit_stack_lookup (nm, ctrl_data, &local_stack,
                                                    p, &send_msg,
                                                    MPLS_OTHER_CLI);

          if (ret1 >= 0)
            {
              stack_depth = ret;
              return_code = MPLS_OAM_LBL_SWITCH_AT_DEPTH;
              ret_sub_code = stack_depth;
            }
        }
      else
        {
          stack_depth = ret;
          return_code = MPLS_OAM_LBL_SWITCH_AT_DEPTH;
          ret_sub_code = stack_depth;
        }
    }

  if (send_msg && (! lookup))
    {
      ifp = ifg_lookup_by_index (&nm->zg->ifg, local_stack.ifindex);
      ret = nsm_mpls_oam_echo_reply_process (nm, oam_hdr, oam_req,
                                             rx_time, ifp, return_code,
                                             ret_sub_code, &local_stack,
                                             ctrl_data);
      if ( ret < 0)
        return ret;
    }

  return NSM_SUCCESS;
}

/* Echo Packet has timer out */
int
nsm_mpls_oam_echo_timeout (struct thread *t)
{
  struct nsm_mpls_oam_data *data;
  struct nsm_mpls_oam *oam;

  int ret = NSM_FALSE;
  data = THREAD_ARG (t);

  oam = data->oam;

  listnode_delete (oam->oam_req_list, data);

  send_nsm_mpls_oam_ping_error (oam, MPLS_OAM_ERR_ECHO_TIMEOUT);
  /* For trace Messages if a transit does not support OAM
   * it will not reply and this can cause a timeout of trace
   * so we send a new trace with an incremented TTL
   */
  if (oam->msg.req_type == MPLSONM_OPTION_TRACE)
    {
      if (oam->msg.ttl < data->oam->max_ttl)
        {
          oam->msg.ttl++;
          oam->trace_timeout = NSM_TRUE;

          if (nsm_mpls_set_echo_request_hdr (oam) == NSM_SUCCESS)
            {
              oam->oam_echo_interval =
               thread_add_timer (NSM_ZG, nsm_mpls_oam_echo_request_create_send,
                                 oam, 0);
              ret = NSM_TRUE;
            }
        }
    }
  else
    ret = NSM_TRUE;

  /* Need to do OAM cleanup here */
  if ((oam->msg.req_type == MPLSONM_OPTION_PING) &&
      (oam->count >= oam->msg.repeat))
    ret = NSM_FALSE;

  if ((LIST_ISEMPTY(data->oam->oam_req_list)) &&
       (ret == NSM_FALSE))
    {
      nsm_mpls_oam_cleanup (data->oam);
      return NSM_SUCCESS;
    }

  XFREE (MTYPE_TMP, data);
  data = NULL;
  return NSM_SUCCESS;
}

int
nsm_mpls_oam_set_defaults (struct nsm_mpls_oam *oam,
                           struct nsm_msg_mpls_ping_req *msg,
                           struct nsm_master *nm)
{
  int ret;

  /* Check the message values and set Defaults */
  /* Note that we will not check the FEC types here */
  struct apn_vrf *iv;

  iv = apn_vrf_lookup_default (nm->vr);
  if (!iv)
    return NSM_FAILURE;

  if (msg->dst.s_addr == 0)
    {
      u_char *temp = "127.0.0.12";
      ret = pal_inet_pton (AF_INET, temp, &msg->dst);
      if (ret <= 0)
        return NSM_FAILURE;
    }

  if (msg->src.s_addr == 0)
    {
      /* Use router-id as source address if not specified by user */
      msg->src = iv->router_id;
    }

  if (msg->interval == 0)
    msg->interval = 1;

  if (msg->timeout == 0)
    msg->timeout = 5;

  if (msg->req_type == MPLSONM_OPTION_TRACE)
    {
      if (msg->ttl == 0)
        {
          msg->ttl = 1;
          oam->max_ttl = MPLS_OAM_DEFAULT_TTL;
        }
      else
        {
          oam->max_ttl = msg->ttl;
          msg->ttl = 1;
        }
    }
  else if (msg->ttl == 0)
    msg->ttl = 255;

  if (msg->repeat == 0)
    msg->repeat = 5;

  if (msg->reply_mode == 0)
    msg->reply_mode = MPLS_OAM_IP_UDP_REPLY;

  pal_mem_cpy (&oam->msg, msg, sizeof (struct nsm_msg_mpls_ping_req));

  /* Set the forwarder flags*/
  if (msg->nophp &&
      ((msg->type == MPLSONM_OPTION_LDP) ||
       (msg->type == MPLSONM_OPTION_RSVP) ||
       (msg->type == MPLSONM_OPTION_STATIC)))
    SET_FLAG (oam->fwd_flags, MPLSONM_FWD_OPTION_NOPHP);

  if (msg->type == MPLSONM_OPTION_L3VPN ||
      msg->type == MPLSONM_OPTION_L2CIRCUIT ||
      msg->type == MPLSONM_OPTION_VPLS)
    SET_FLAG (oam->fwd_flags, MPLSONM_FWD_OPTION_LABEL);

  return NSM_SUCCESS;
}

int
nsm_mpls_oam_get_tunnel_info (struct nsm_mpls_oam *oam,
                              struct nsm_msg_mpls_ping_req *msg)
                                  
{
  struct nsm_master *nm = oam->nm;
  struct prefix pfx;
  struct ftn_entry *ftn = NULL;
  u_char *rsvp;

  pal_mem_set (&pfx, 0, sizeof (struct prefix));
  pfx.family = AF_INET;
  pfx.prefixlen = IPV4_MAX_PREFIXLEN;

  /* Get FTN pointer to get labels relevant to the FEC Stack
   * Need to identify the correct FTN table in case of VPN */
  if (msg->type == MPLSONM_OPTION_STATIC)
    {
      pfx.prefixlen = msg->u.fec.prefixlen;
      IPV4_ADDR_COPY (&pfx.u.prefix4, &msg->u.fec.ip_addr);
      ftn = nsm_gmpls_get_static_ftn_pfx (nm, &pfx);
      if (! ftn)
        return MPLS_OAM_ERR_NO_FTN_FOUND;
    }
#ifdef HAVE_VRF
  else if (msg->type == MPLSONM_OPTION_L3VPN)
    {
      pfx.prefixlen = msg->u.fec.prefixlen;
      IPV4_ADDR_COPY (&pfx.u.prefix4, &msg->u.fec.vpn_prefix);

      oam->n_vrf = nsm_vrf_lookup_by_name (nm, msg->u.fec.vrf_name);
      if (! oam->n_vrf)
        return MPLS_OAM_ERR_NO_VPN_FOUND;
       
      ftn = nsm_gmpls_ftn_lookup_installed (nm, oam->n_vrf->vrf->id, &pfx);
      if (!ftn)
        return MPLS_OAM_ERR_NO_FTN_FOUND;
     }
#endif /* HAVE_VRF */
  else if (msg->type == MPLSONM_OPTION_LDP)
    {
      pfx.prefixlen = msg->u.fec.prefixlen;
      IPV4_ADDR_COPY (&pfx.u.prefix4,&msg->u.fec.ip_addr);
      ftn = nsm_gmpls_get_ldp_ftn (nm, &pfx);
      if (! ftn)
        return MPLS_OAM_ERR_NO_FTN_FOUND;
    }
  else if (NSM_CHECK_CTYPE (msg->cindex, NSM_CTYPE_MSG_MPLSONM_OPTION_EGRESS))
    {
      IPV4_ADDR_COPY (&pfx.u.prefix4, &msg->u.rsvp.addr);
      ftn = nsm_gmpls_get_rsvp_ftn_pfx (nm, &pfx);
      if (! ftn)
        return MPLS_OAM_ERR_NO_FTN_FOUND;
    }
  else if (NSM_CHECK_CTYPE (msg->cindex,
                            NSM_CTYPE_MSG_MPLSONM_OPTION_TUNNELNAME))
    {
      rsvp = XCALLOC (MTYPE_TMP, msg->u.rsvp.tunnel_len+1);
      pal_strncpy (rsvp, msg->u.rsvp.tunnel_name, msg->u.rsvp.tunnel_len);
      ftn = nsm_gmpls_get_rsvp_ftn_by_tunnel (nm, rsvp,
                                             RSVP_TUNNEL_NAME);
      XFREE (MTYPE_TMP, rsvp);
      rsvp = NULL;
      if (! ftn)
        return MPLS_OAM_ERR_NO_FTN_FOUND;
    }
#ifdef HAVE_MPLS_VC
  else if (msg->type == MPLSONM_OPTION_L2CIRCUIT)
    {
      oam->u.vc = nsm_mpls_l2_circuit_lookup_by_id (nm,
                                         msg->u.l2_data.vc_id);

      if (! oam->u.vc)
        return MPLS_OAM_ERR_NO_VC_FOUND;

      pfx = oam->u.vc->address;

      /* This gets a Tunnel for the VC Try to get MIB code*/
      ftn = nsm_gmpls_ftn_lookup_installed (nm, GLOBAL_FTN_ID, &pfx);
      if (!ftn)
        return MPLS_OAM_ERR_NO_FTN_FOUND;
    }
#ifdef HAVE_VPLS
  else if (msg->type == MPLSONM_OPTION_VPLS)
    {
      IPV4_ADDR_COPY (&pfx.u.prefix4, &msg->u.l2_data.vc_peer);
      oam->u.vpls = XCALLOC (MTYPE_NSM_VPLS, sizeof (struct nsm_vpls));
      oam->u.vpls = nsm_vpls_lookup (nm, msg->u.l2_data.vc_id,
                                     NSM_FALSE);
 
      if (! oam->u.vpls)
        return MPLS_OAM_ERR_NO_VPLS_FOUND;

      return NSM_SUCCESS;
    }
#endif /* HAVE_VPLS */
#endif /* HAVE_MPLS_VC */

  if (! ftn)
    return MPLS_OAM_ERR_NO_FTN_FOUND;

  if ((! ftn->xc) || (! ftn->xc->nhlfe) ||
       (ftn->xc->nhlfe->key.u.ipv4.out_label == LABEL_IMPLICIT_NULL))
    return MPLS_OAM_ERR_EXPLICIT_NULL;
  oam->ftn = XMALLOC (MTYPE_TMP, sizeof (struct ftn_entry));
  pal_mem_cpy (oam->ftn, ftn, sizeof (struct ftn_entry));
   
  oam->ftn->xc = XMALLOC (MTYPE_TMP, sizeof (struct xc_entry));
  pal_mem_cpy (oam->ftn->xc, ftn->xc, sizeof (struct xc_entry));

  if (ftn->xc->nhlfe)
   {
     oam->ftn->xc->nhlfe = XMALLOC (MTYPE_TMP, sizeof (struct nhlfe_entry));
     pal_mem_cpy (oam->ftn->xc->nhlfe, ftn->xc->nhlfe, sizeof (struct nhlfe_entry));
   }

  return NSM_SUCCESS;
}

int
nsm_mpls_oam_set_ping_reply (struct nsm_msg_mpls_ping_reply *msg,
                             struct mpls_oam_echo_reply *reply,
                             struct nsm_mpls_oam_data *data,
                             struct nsm_mpls_oam *oam,
                             struct pal_timeval *rx_time)
{

  msg->sent_time_sec = reply->omh.ts_tx_sec;
  msg->sent_time_usec = reply->omh.ts_tx_usec;
  msg->recv_time_sec = rx_time->tv_sec;
  msg->recv_time_usec = rx_time->tv_usec;
  msg->reply = oam->r_src;
  msg->ret_code = reply->omh.return_code;
  msg->ping_count = data->pkt_count;
  msg->recv_count = oam->count;

  NSM_SET_CTYPE (msg->cindex, NSM_CTYPE_MSG_MPLSONM_PING_SENTTIME);
  NSM_SET_CTYPE (msg->cindex, NSM_CTYPE_MSG_MPLSONM_PING_RECVTIME);
  NSM_SET_CTYPE (msg->cindex, NSM_CTYPE_MSG_MPLSONM_PING_RETCODE);
  NSM_SET_CTYPE (msg->cindex, NSM_CTYPE_MSG_MPLSONM_PING_RECVCOUNT);
  NSM_SET_CTYPE (msg->cindex, NSM_CTYPE_MSG_MPLSONM_PING_REPLYADDR);
  NSM_SET_CTYPE (msg->cindex, NSM_CTYPE_MSG_MPLSONM_PING_COUNT);

  return NSM_SUCCESS;
}

void
nsm_mpls_oam_client_disconnect (struct nsm_master *nm,
                                     struct nsm_server_entry *nse)
{
  struct listnode *ln = NULL, *ln1 = NULL;
  struct nsm_mpls_oam *oam = NULL;
  struct nsm_mpls_oam_data *data = NULL;
  /* We have the list of messages initiated from different Utility instances
   * Need to find the exact instance that caused this reply to be sent
   */

  if (LIST_ISEMPTY (NSM_MPLS->mpls_oam_list))
    return;

  LIST_LOOP (NSM_MPLS->mpls_oam_list, oam, ln)
     {
       if (oam->nse == nse)
         {
           /* From the main structure it will be possible to get the
            * sequence match of the echo request sent.
            */
           if (! LIST_ISEMPTY (oam->oam_req_list))
             {
               LIST_LOOP (oam->oam_req_list, data, ln1)
                 {
                   THREAD_TIMER_OFF (data->pkt_timeout);
                   XFREE (MTYPE_TMP, data);
                 }
             }

           if (oam->oam_echo_interval)
             THREAD_TIMER_OFF (oam->oam_echo_interval);


           nsm_mpls_oam_cleanup (oam);
           break;

         }
     }
}

int
nsm_mpls_oam_set_trace_reply (struct nsm_msg_mpls_tracert_reply *msg,
                              struct mpls_oam_echo_reply *reply,
                              struct nsm_mpls_oam_data *data,
                              struct nsm_mpls_oam *oam,
                              struct pal_timeval *rx_time,
                              bool_t last)
{
  struct listnode *node = NULL;
  struct shimhdr *label = NULL;
  struct interface *ifp = NULL;

  if (reply)
    {
      msg->reply = oam->r_src;
      msg->sent_time_sec = reply->omh.ts_tx_sec;
      msg->sent_time_usec = reply->omh.ts_tx_usec;
      msg->recv_time_sec = rx_time->tv_sec;
      msg->recv_time_usec = rx_time->tv_usec;
      msg->ret_code = reply->omh.return_code;
      msg->recv_count = oam->count;

      NSM_SET_CTYPE (msg->cindex, NSM_CTYPE_MSG_MPLSONM_TRACE_REPLYADDR);
      NSM_SET_CTYPE (msg->cindex, NSM_CTYPE_MSG_MPLSONM_TRACE_SENTTIME);
      NSM_SET_CTYPE (msg->cindex, NSM_CTYPE_MSG_MPLSONM_TRACE_RECVTIME);
      NSM_SET_CTYPE (msg->cindex, NSM_CTYPE_MSG_MPLSONM_TRACE_RETCODE);
      NSM_SET_CTYPE (msg->cindex, NSM_CTYPE_MSG_MPLSONM_TRACE_RECVCOUNT);

      if (reply->dsmap.ds_label)
        {
          if (!LIST_ISEMPTY (reply->dsmap.ds_label))
            {
              msg->ds_label = list_new();

              LIST_LOOP (reply->dsmap.ds_label, label, node)
                {
                  listnode_add (msg->ds_label, label);
                  msg->ds_len++;
                  NSM_SET_CTYPE (msg->cindex, NSM_CTYPE_MSG_MPLSONM_TRACE_DSMAP);               
                }
            }
        }
    }
   else
    {
       ifp = if_lookup_by_index (&oam->nm->vr->ifm,
                                 oam->ftn->xc->nhlfe->key.u.ipv4.oif_ix);
       if (! ifp)
         return NSM_FAILURE;

       msg->reply = ifp->ifc_ipv4->address->u.prefix4;
       set_label_net (msg->out_label,
                              oam->ftn->xc->nhlfe->key.u.ipv4.out_label)
       NSM_SET_CTYPE (msg->cindex, NSM_CTYPE_MSG_MPLSONM_TRACE_REPLYADDR);
       NSM_SET_CTYPE (msg->cindex, NSM_CTYPE_MSG_MPLSONM_TRACE_OUTLABEL);
       
#ifdef HAVE_MPLS_VC
       if (oam->msg.type == MPLSONM_OPTION_L2CIRCUIT)
         {
            msg->ds_label = list_new();
            label = XMALLOC (MTYPE_TMP, sizeof (struct shimhdr));
            set_label_net (label->shim, oam->u.vc->vc_fib->out_label);
            listnode_add (msg->ds_label,label);

            label = XMALLOC (MTYPE_TMP, sizeof (struct shimhdr));
            set_label_net (label->shim, 
                           oam->ftn->xc->nhlfe->key.u.ipv4.out_label);
            listnode_add (msg->ds_label, 
                          label);
            msg->ds_len+=2;
          
            NSM_SET_CTYPE (msg->cindex, NSM_CTYPE_MSG_MPLSONM_TRACE_DSMAP);
         }
#endif /* HAVE_MPLS_VC */
    }

  if ( last)
    {
      NSM_SET_CTYPE (msg->cindex, NSM_CTYPE_MSG_MPLSONM_TRACE_LAST_MSG);
      msg->last = last;
    }

  return NSM_SUCCESS;

}

void
send_nsm_mpls_oam_ping_error (struct nsm_mpls_oam *oam,
                              int err_code)
{
  struct nsm_msg_mpls_ping_error msg;
  int len =0;
  u_char err = 0;

  switch (err_code)
    {
      case MPLS_OAM_ERR_NO_FTN_FOUND:
        err = NSM_MPLSONM_NO_FTN;
        break;
      case MPLS_OAM_ERR_EXPLICIT_NULL:
        err = NSM_MPLSONM_ERR_EXPLICIT_NULL;
        break;
      case MPLS_OAM_ERR_ECHO_TIMEOUT:
        err = NSM_MPLSONM_ECHO_TIMEOUT;
        break;
      case MPLS_OAM_ERR_TRACE_LOOP:
        err = NSM_MPLSONM_TRACE_LOOP;
        break;
      case MPLS_OAM_ERR_PACKET_SEND:
        err = NSM_MPLSONM_PACKET_SEND_ERROR;
        break;
      case MPLS_OAM_ERR_NO_VPN_FOUND:
      case MPLS_OAM_ERR_NO_VC_FOUND: 
        err = NSM_MPLSONM_ERR_NO_CONFIG;
        break;
      default:
        err = NSM_MPLSONM_ERR_UNKNOWN;
        break;             
    }
  pal_mem_set (&msg, 0, sizeof (struct nsm_msg_mpls_ping_error));
  NSM_SET_CTYPE (msg.cindex, NSM_CTYPE_MSG_MPLSONM_ERROR_MSGTYPE);
  msg.msg_type = err;
  msg.recv_count = oam->count;
  NSM_SET_CTYPE (msg.cindex, NSM_CTYPE_MSG_MPLSONM_ERROR_RECVCOUNT);
  len = nsm_encode_mpls_onm_error (&oam->nse->send.pnt,
                                   &oam->nse->send.size,
                                   &msg);
  if (len < 0)
    return ;

  nsm_server_send_message (oam->nse, 0, 0, NSM_MSG_MPLS_OAM_ERROR,
                           0, len);

}


#ifdef HAVE_MPLS_OAM_ITUT
static u_int32_t mplsOamITUTInstCount;

int
nsm_mpls_oam_itut_request_send (struct nsm_mpls_oam_itut *oam, u_char *sendbuf,
                                u_int32_t length)
{
  int ret = 0;

  /* Send the itut request */
  ret = apn_mpls_oam_send_echo_request (oam->ftn->ftn_ix,
                                        /*FTN_NHLFE (oam->ftn)->nhlfe_ix,*/
                                        0, oam->fwd_flags,
                                        oam->msg.ttl, length, sendbuf);
  if (ret < 0)
    return ret; /* Add code to send OAM REQUEST ERROR */

  /*  start the Packet interval thread to schedule the next itut request msg */
  if((oam->function_type == MPLS_OAM_CV_PROCESS_MESSAGE) ||
     (oam->function_type == MPLS_OAM_FFD_PROCESS_MESSAGE) )
   {
     struct pal_timeval tv;
     if(oam->function_type == MPLS_OAM_CV_PROCESS_MESSAGE)
        tv.tv_sec = 1;
     else  
        tv.tv_sec = (oam->frequency) /1000;
     tv.tv_usec = (oam->frequency % 1000) * 1000;
     oam->oam_pkt_interval =  thread_add_timer_timeval (NSM_ZG,
                                nsm_mpls_oam_itut_request_create_send, oam, tv);
   }
  return NSM_SUCCESS;
}

int
nsm_mpls_oam_itut_process_request(struct nsm_master *nm,
                                  struct nsm_server_entry *nse,
                                  struct nsm_msg_mpls_oam_itut_req *msg)
{
  struct nsm_mpls_oam_itut *oam = NULL;
  struct nsm_mpls_oam_itut_data *data=NULL;
  struct listnode *ln = NULL, *ln1 = NULL;
  int ret = 0;

  if (msg->flag == NSM_MPLS_OAM_PKT_STOP)
   {
     if (( msg->pkt_type == NSM_MPLS_CV_PKT) || 
         (msg->pkt_type == NSM_MPLS_FFD_PKT))
       {
         if (!LIST_ISEMPTY(NSM_MPLS->mpls_oam_list))	      
           LIST_LOOP (NSM_MPLS->mpls_oam_itut_list, oam, ln)
             {
               if (! LIST_ISEMPTY (oam->oam_req_list) ) 
	         {
                   LIST_LOOP (oam->oam_req_list, data, ln1)
	             {	   
                       if (data->oam->lsp_ttsi.lsp_id == msg->lsp_id)
                         {
                           THREAD_TIMER_OFF (oam->oam_pkt_interval);
                           THREAD_TIMER_OFF (data->pkt_timeout);
                           XFREE (MTYPE_TMP, data);
                         }
	             }
	          }
               nsm_mpls_oam_itut_cleanup(oam);
               break; 
           }
         return NSM_SUCCESS;
      }
   }

  if (msg->flag == NSM_MPLS_OAM_PKT_START)
   {
     if (( msg->pkt_type == NSM_MPLS_CV_PKT) || 
         (msg->pkt_type == NSM_MPLS_FFD_PKT))
      {
        if (!LIST_ISEMPTY(NSM_MPLS->mpls_oam_list))	      
          LIST_LOOP (NSM_MPLS->mpls_oam_itut_list, oam, ln)
            {
              if (oam->lsp_ttsi.lsp_id == msg->lsp_id)
                {
                  return NSM_FAILURE;
                }
             }
      }
   }
  oam = XCALLOC (MTYPE_TMP, sizeof (struct nsm_mpls_oam_itut));
  
  if (oam == NULL)
   return NSM_FAILURE;
  
  pal_mem_set (oam, 0, sizeof (struct nsm_mpls_oam_itut));
  
  /* Set the unique handle for OAM Instance */
  oam->sock = mplsOamITUTInstCount++;
 
  oam->nm = nm;
  oam->nse = nse;
  /* Add Struct to master list */
  listnode_add (NSM_MPLS->mpls_oam_itut_list, oam);

  oam->oam_req_list = list_new();

  /* We need to check the received message and set default values if reqd */
  if (nsm_mpls_oam_set_itut_req (oam, msg, nm) != NSM_SUCCESS)
  {
    XFREE(MTYPE_TMP,oam);
    return NSM_FAILURE;
  } 

  /*  packet is sent without an interval */
  oam->count = 0;
  if (msg->pkt_type == NSM_MPLS_FFD_PKT)
   {
     switch (msg->frequency)
      {
  	case NSM_MPLS_OAM_FFD_FREQ_RESV:
             oam->frequency = 0;
  	 break;
        case NSM_MPLS_OAM_FFD_FREQ_1:
             oam->frequency = 10;
    	 break;
        case NSM_MPLS_OAM_FFD_FREQ_2:
             oam->frequency = 20;
    	 break;
        case NSM_MPLS_OAM_FFD_FREQ_3: /* default value */
             oam->frequency = 50;
    	 break;
        case NSM_MPLS_OAM_FFD_FREQ_4:
             oam->frequency = 100;
    	 break;
        case NSM_MPLS_OAM_FFD_FREQ_5:
             oam->frequency = 200;   	
    	 break;
        case NSM_MPLS_OAM_FFD_FREQ_6:
             oam->frequency = 500;
    	 break;
      }
   }
   else if (msg->pkt_type == NSM_MPLS_CV_PKT) 
   {
    oam->frequency = 1000;
   }

   if (((oam->function_type == MPLS_OAM_CV_PROCESS_MESSAGE) ||
      (oam->function_type == MPLS_OAM_FFD_PROCESS_MESSAGE)) && 
      (msg->lsr_type == NSM_MPLS_OAM_SOURCE_LSR))
    {
 
     /* Set FTN Data */
     if ((ret = nsm_mpls_oam_itut_get_tunnel_info (oam, msg)) < 0)
      {
        send_nsm_mpls_oam_itut_error(oam, ret);
        nsm_mpls_oam_itut_cleanup(oam);
        return NSM_FAILURE;
      }
      oam->oam_pkt_interval =
               thread_add_timer (NSM_ZG, nsm_mpls_oam_itut_request_create_send,
                                 oam, 1);
    }
   return NSM_SUCCESS;
}

int 
nsm_mpls_oam_stats_display( struct nsm_master *nm, struct nsm_mpls_oam_stats *msg)
{
  struct nsm_mpls_oam_itut *oam = NULL;
  struct listnode *ln = NULL;
  
  if (!LIST_ISEMPTY(NSM_MPLS->mpls_oam_list))	      
  LIST_LOOP (NSM_MPLS->mpls_oam_itut_list, oam, ln)
  {
    if (oam->lsp_ttsi.lsp_id == msg->lsp_id)
    {
      msg->oampkt_counter = oam->count;
      break;
    }
  }
  return NSM_SUCCESS;
}
int
nsm_mpls_oam_set_itut_req( struct nsm_mpls_oam_itut *oam, 
                                 struct nsm_msg_mpls_oam_itut_req *msg,
                                 struct nsm_master *nm)
{

    /* Check the message values and set Defaults */
    struct apn_vrf *iv;
  
    iv = apn_vrf_lookup_default (nm->vr);
  
    /* Use router-id as source address if not specified by user */
    switch (msg->pkt_type)
    {
      case NSM_MPLS_CV_PKT:
      	   msg->frequency  = NSM_MPLS_OAM_FFD_FREQ_RESV;
      	   msg->fwd_lsp_id = 0;
      	   msg->fdi_level.level_no = 0;
      	   oam->function_type = MPLS_OAM_CV_PROCESS_MESSAGE;
      	 break;
      	  	
      case NSM_MPLS_FFD_PKT:
           msg->fwd_lsp_id = 0;
           msg->fdi_level.level_no = 0;
      	   oam->function_type = MPLS_OAM_FFD_PROCESS_MESSAGE;
         break;
         	
      case NSM_MPLS_FDI_PKT:
      	   msg->fwd_lsp_id = 0;
      	   msg->flag = NSM_MPLS_OAM_PKT_START;
      	   msg->frequency = NSM_MPLS_OAM_FFD_FREQ_RESV;
      	   msg->lsr_type = NSM_MPLS_OAM_TRANSIT_LSR;
      	   oam->function_type = MPLS_OAM_FDI_PROCESS_MESSAGE;
      	 break;

      case NSM_MPLS_BDI_PKT:
      	   msg->flag = NSM_MPLS_OAM_PKT_START;
      	   msg->frequency = NSM_MPLS_OAM_FFD_FREQ_RESV;
      	   msg->fdi_level.level_no = 0;
      	   oam->function_type = MPLS_OAM_BDI_PROCESS_MESSAGE;
      	 break;

      default:
      	break;
    }
    if (msg->lsr_type == NSM_MPLS_OAM_SOURCE_LSR)
     {
       pal_mem_cpy (&oam->lsp_ttsi.lsr_addr, &iv->router_id, 4);
     }
    else
     {
       pal_mem_cpy (&oam->lsp_ttsi.lsr_addr, &msg->lsr_addr, 4);
     }

    oam->lsp_ttsi.lsp_id  = msg->lsp_id;
    oam->frequency = msg->frequency;
    oam->fwd_flags = MPLSONM_FWD_OPTION_NOPHP;
    oam->bip16 = MPLS_OAM_ITUT_REQUEST_MIN_LEN; 
    pal_mem_cpy (&oam->msg, msg, sizeof (struct nsm_msg_mpls_oam_itut_req));
    return NSM_SUCCESS;
}
#if 0
int
nsm_mpls_oam_prep_req_for_ethernet (struct nsm_mpls_oam_itut *oam,
                                   u_char *pnt, u_int16_t len,
                                   u_char *sendbuf)
{
  u_char buf[OAM_DATA_SIZE];
  u_char eth_hdr[sizeof (struct mpls_oam_eth_header)];
  u_int16_t size;
  
  struct mpls_oam_eth_header *eth;

  pal_mem_set (buf, 0, OAM_DATA_SIZE);
  pal_mem_set (eth_hdr, 0,
               sizeof (struct mpls_oam_eth_header));

  eth->type = MPLS_ETHER_TYPE;
  size = sizeof (struct mpls_oam_eth_header);
  
  pal_mem_cpy ((buf + size), pnt, len);

  pal_mem_cpy (sendbuf, eth, size);
  pal_mem_cpy ((sendbuf + size), buf+size, len+sizeof (struct mpls_oam_eth_header));

  return (len+ sizeof (struct mpls_oam_eth_header) + size);
}
#endif
int
nsm_mpls_oam_itut_request_create_send (struct thread *t)
{
  struct nsm_mpls_oam_itut *oam;
  int ret;
  u_char pnt[OAM_DATA_SIZE];
  u_char *pnt1;
  u_char sendbuf[OAM_DATA_SIZE];
  u_int16_t size;
  struct nsm_mpls_oam_itut_data *data;
  u_int16_t len;
    
  oam = THREAD_ARG (t);
  data = XMALLOC (MTYPE_TMP, sizeof (struct nsm_mpls_oam_itut_data));
  pal_mem_set (data, 0, sizeof (struct nsm_mpls_oam_itut_data));
  pal_mem_set (pnt, 0, OAM_DATA_SIZE);
  pal_mem_set (sendbuf, 0, OAM_DATA_SIZE);

  pnt1=pnt;
  data->oam = oam;
  /* Get Current time */
  pal_time_tzcurrent (&data->sent_time, NULL);

  oam->count++;
  data->pkt_count = oam->count;
  oam->oam_pkt_interval = NULL;
  size = oam->bip16;
  
  if (size < MPLS_OAM_ITUT_REQUEST_MIN_LEN)
   {
     XFREE (MTYPE_TMP, data);
     nsm_mpls_oam_itut_cleanup(oam);
     return NSM_FAILURE;
   }
  len = size;

  ret = nsm_mpls_oam_encode_itut_request (&pnt1, &size, oam);
  if (ret < 0 )
   {
     XFREE (MTYPE_TMP, data);
     nsm_mpls_oam_itut_cleanup (oam);
     return NSM_FAILURE;
   }

  pal_mem_cpy (sendbuf, pnt, len);
  ret = nsm_mpls_oam_itut_request_send(oam, sendbuf, len);
  
  if (ret < 0)
   {
     XFREE (MTYPE_TMP, data);
     nsm_mpls_oam_itut_cleanup (oam);
     return NSM_FAILURE;
   }
  else
   {
     long timeout;
     /* Store the data pointers in a list to retrieve the required message  later */
     listnode_add (oam->oam_req_list, data);
     data->pkt_timeout = NULL;
     if (oam->msg.timeout)
        timeout = oam->msg.timeout;
     else
        timeout = oam->msg.ttl;

     THREAD_TIMER_ON (oam->nm->zg, data->pkt_timeout,
                      nsm_mpls_oam_itut_timeout,
                      data, timeout);
   }

  return NSM_SUCCESS;
}


int
nsm_mpls_oam_itut_get_tunnel_info (struct nsm_mpls_oam_itut *oam,
                                   struct nsm_msg_mpls_oam_itut_req *msg)
                                  
{
  struct nsm_master *nm = oam->nm;
  struct ftn_entry *ftn = NULL;
  u_int32_t rsvp;

  /* Get FTN pointer to get labels relevant to the FEC Stack */
  rsvp = msg->lsp_id;
  ftn = nsm_mpls_get_rsvp_ftn_by_lsp (nm, &rsvp);
  
  if (! ftn)
    return MPLS_OAM_ERR_NO_FTN_FOUND;
  oam->ftn = XMALLOC (MTYPE_TMP, sizeof (struct ftn_entry));
  pal_mem_cpy (oam->ftn, ftn, sizeof (struct ftn_entry));
   
  return NSM_SUCCESS;
}



/* itut Packet has timer out */
int
nsm_mpls_oam_itut_timeout (struct thread *t)
{
  struct nsm_mpls_oam_itut_data *data;
  struct nsm_mpls_oam_itut *oam;

  int ret = NSM_FALSE;
  data = THREAD_ARG (t);

  oam = data->oam;

  listnode_delete (oam->oam_req_list, data);

  /* For Messages if a transit does not support OAM
   * it will not reply and this can cause a timeout of 
   * so we send a new Packet with an incremented TTL
   */
   
  if (oam->msg.flag == NSM_MPLS_OAM_PKT_START)
   {
      oam->msg.ttl++;
      oam->oam_pkt_interval = 
      	        thread_add_timer (NSM_ZG, nsm_mpls_oam_itut_request_create_send,
                                          oam, 1);
      ret = NSM_TRUE;
   }
  else
    ret = NSM_TRUE;

  /* Need to do OAM cleanup here */
  if ((LIST_ISEMPTY(data->oam->oam_req_list)) &&
       (ret == NSM_FALSE))
    {
      nsm_mpls_oam_itut_cleanup (data->oam);
      return NSM_SUCCESS;
    }

  XFREE (MTYPE_TMP, data);
  data = NULL;
  return NSM_SUCCESS;
}

void
send_nsm_mpls_oam_itut_error (struct nsm_mpls_oam_itut *oam,
                              int err_code)
{
  u_char err = 0;

  switch (err_code)
    {
      case MPLS_OAM_ERR_NO_FTN_FOUND:
        err = NSM_MPLSONM_NO_FTN;
        break;
      default:
        err = NSM_MPLSONM_ERR_UNKNOWN;
        break;             
    }
}


void
nsm_mpls_oam_itut_cleanup (struct nsm_mpls_oam_itut *oam)
{
  if (! oam)
    return;

  if (oam && oam->ftn && oam->ftn->xc)
    XFREE (MTYPE_TMP, oam->ftn->xc->nhlfe);

  if (oam && oam->ftn)
    XFREE (MTYPE_TMP, oam->ftn->xc);

  if (oam)
    XFREE (MTYPE_TMP, oam->ftn);
  list_delete (oam->oam_req_list);
  listnode_delete (oam->nm->nmpls->mpls_oam_itut_list, oam);
  XFREE (MTYPE_TMP, oam);
  oam = NULL;
}

int
nsm_mpls_oam_itut_packet_read (struct nsm_master *nm, u_char *buf, int len,
                               struct mpls_oam_ctrl_data *ctrl_data)
 {
  int ret;
  int err_code = 0;
  struct nsm_mpls_oam_itut *oam_req;
  struct pal_timeval current;
  struct nsm_mpls_oam_itut *oam = NULL;
  struct listnode *ln = NULL;
  u_char *pnt;
  u_int16_t size;
  
  
  oam_req = XMALLOC (MTYPE_TMP, sizeof (struct nsm_mpls_oam_itut));
  
  pal_mem_set (oam_req, 0, sizeof (struct nsm_mpls_oam_itut));
  
  pal_time_tzcurrent (&current, NULL);

  if (len < MPLS_OAM_ITUT_REQUEST_MIN_LEN)
    {
      zlog_err (NSM_ZG, "Packet size %d is incorrect", len);
      return -1;
    }

  /* Now that the packet is here we need to identify it as an BDI/FDI/
    * CV/FFD and continue with the rest of the decode procedures
    */

  pnt = buf;
  size = len;
  
  ret = nsm_mpls_oam_decode_itut_request (&pnt, &size, oam_req, &err_code);
  
  if (ret < 0 || err_code > 0)
   {
     zlog_err (NSM_ZG, "OAM Packet Decode/Unkown format Error ");
     return ret;
   }
  switch (oam_req->function_type)
   {
     case MPLS_OAM_CV_PROCESS_MESSAGE:
     case MPLS_OAM_FFD_PROCESS_MESSAGE:
       /* Timers needs to be started accordingly FDI/BDI packets should be generated*/
        if (!LIST_ISEMPTY(NSM_MPLS->mpls_oam_list))	      
    	LIST_LOOP (NSM_MPLS->mpls_oam_itut_list, oam, ln)
        {
          if ( pal_mem_cmp( &oam->lsp_ttsi.lsr_addr, &oam_req->lsp_ttsi.lsr_addr,
                              sizeof (struct pal_in4_addr)) == 0)
           {
               if ((oam->msg.lsr_type == NSM_MPLS_OAM_SINK_LSR) &&
                   (oam->msg.flag == NSM_MPLS_OAM_PKT_START))
                 {  
                   oam_req->msg.check_timer =
                     thread_add_timer (NSM_ZG, nsm_mpls_oam_itut_check_tool,
                                       oam_req, 3);
                 }
               break;
            }
         }
      break;
     case MPLS_OAM_FDI_PROCESS_MESSAGE:
    	/* based on the stack depth and label level FDI needs to be sent upper layers*/
        if (!LIST_ISEMPTY(NSM_MPLS->mpls_oam_list))	      
    	LIST_LOOP (NSM_MPLS->mpls_oam_itut_list, oam, ln)
        {
         if ( pal_mem_cmp( &oam->lsp_ttsi.lsr_addr, &oam_req->lsp_ttsi.lsr_addr,
                              sizeof (struct pal_in4_addr)) == 0)
          {
            /* Check for the BDI packet is configured or not */
            if (( oam->msg.lsr_type == NSM_MPLS_OAM_SINK_LSR ) &&
                (oam->msg.fwd_lsp_id != 0 ))
             {
               oam_req->lsp_ttsi.lsp_id = oam->msg.fwd_lsp_id;
               oam_req->function_type = MPLS_OAM_BDI_PROCESS_MESSAGE;
               oam_req->oam_pkt_interval =
                 thread_add_timer (NSM_ZG, nsm_mpls_oam_itut_request_create_send,
                                 oam_req, 3);
             }
            break;
       	  }
        }
      break;
     case MPLS_OAM_BDI_PROCESS_MESSAGE:
    /*this packet will get received only at source LSRs to get mpls oam defect identification */
        if (!LIST_ISEMPTY(NSM_MPLS->mpls_oam_list))	      
    	LIST_LOOP (NSM_MPLS->mpls_oam_itut_list, oam, ln)
        {
          if ( pal_mem_cmp( &oam->lsp_ttsi.lsr_addr, &oam_req->lsp_ttsi.lsr_addr,
                              sizeof (struct pal_in4_addr)) == 0)
           {
              if (oam->msg.lsr_type == NSM_MPLS_OAM_SOURCE_LSR) 
               {
                zlog_info (NSM_ZG, "LSP failed, need to enable protected LSP \n");  
               }
               break;
           } 
        }  
      break;
    	
     default:
    	 return -1;
   }
    
  XFREE (MTYPE_TMP, oam_req);
  return 0;
}

int 
nsm_mpls_oam_itut_process_fdi_request( struct nsm_server_entry *nse, 
                                       struct interface *ifp)
{

  struct nsm_mpls_oam_itut *oam = NULL;
  struct nsm_master *nm=ifp->vr->proto;
  struct ftn_entry *ftn = NULL;
  struct listnode *ln =NULL;
  u_int32_t rsvp;
  
  if (!LIST_ISEMPTY(NSM_MPLS->mpls_oam_list))	      
  LIST_LOOP (NSM_MPLS->mpls_oam_itut_list, oam, ln)
  {
    rsvp = oam->lsp_ttsi.lsp_id;
    ftn = nsm_mpls_get_rsvp_ftn_by_lsp (nm, &rsvp);
    if (ftn)
    	break;
  }
  if (! ftn)
    return MPLS_OAM_ERR_NO_FTN_FOUND;
  oam->ftn = XMALLOC (MTYPE_TMP, sizeof (struct ftn_entry));
  pal_mem_cpy (oam->ftn, ftn, sizeof (struct ftn_entry));
  oam->nm = nm;
  oam->nse = nse;
  oam->function_type = MPLS_OAM_FDI_PROCESS_MESSAGE;
  oam->defect_type = MPLS_DEFECT_TYPE_DSERVER;
  oam->oam_pkt_interval =
               thread_add_timer (NSM_ZG, nsm_mpls_oam_itut_request_create_send,
                                 oam, 1);
  return NSM_SUCCESS;
}

int 
nsm_mpls_oam_itut_check_tool (struct thread *t)
{

  struct nsm_mpls_oam_itut *oam_req;
  struct listnode *ln = NULL;
  struct nsm_master *nm = NULL;
  struct nsm_mpls_oam_itut *oam = NULL;
  struct pal_timeval tv;
  u_int16_t pkt_cnt=0;

  oam_req= THREAD_ARG (t);

  if (!LIST_ISEMPTY(NSM_MPLS->mpls_oam_list))	      
  LIST_LOOP (NSM_MPLS->mpls_oam_itut_list, oam, ln)
  {
    if ((oam->lsp_ttsi.lsp_id == oam_req->lsp_ttsi.lsp_id) && 
        (pal_mem_cmp(&oam->lsp_ttsi.lsr_addr, &oam_req->lsp_ttsi.lsr_addr, 
         sizeof(struct pal_in4_addr))==0))
     {
        oam_req->msg.recv_pkt_cnt++;
        pal_time_tzcurrent(&oam_req->msg.pkt_rcv_time, NULL);
        if ( oam->msg.lsr_type == NSM_MPLS_OAM_SINK_LSR )
         {
           pal_delay(3 * oam_req->frequency);
           oam->msg.recv_pkt_cnt++;
           pal_time_tzcurrent(&oam->msg.pkt_rcv_time, NULL);
           if ((oam->msg.pkt_rcv_time.tv_usec - 
                oam_req->msg.pkt_rcv_time.tv_usec) > 
                (3*oam_req->frequency))
            {
              pkt_cnt = oam_req->msg.recv_pkt_cnt - oam->msg.recv_pkt_cnt;
              if (pkt_cnt == 0)
               {
                 oam_req->defect_type = MPLS_DEFECT_TYPE_DLOCV;
               }
            }
         }
         break;
     }
  }
  if ( ln->data == NULL )
  {
    oam_req->defect_type = MPLS_DEFECT_TYPE_DTTSI_MISMATCH;
    oam_req->function_type = MPLS_OAM_FDI_PROCESS_MESSAGE;
    oam_req->oam_pkt_interval =
               thread_add_timer (NSM_ZG, nsm_mpls_oam_itut_request_create_send,
                                 oam, 1);
  }
  tv.tv_sec = 1;
  tv.tv_usec = 0;

  oam_req->msg.check_timer =  thread_add_timer_timeval (NSM_ZG,
                            nsm_mpls_oam_itut_check_tool, oam_req, tv);

  return NSM_SUCCESS; 
}

#endif /*HAVE_MPLS_OAM_ITUT*/



