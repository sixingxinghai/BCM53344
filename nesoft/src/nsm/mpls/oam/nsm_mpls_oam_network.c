/* Copyright (C) 2001-2003  All Rights Reserved. */

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
#include "nsmd.h"
#ifdef HAVE_DIFFSERV
#include "diffserv.h"
#endif /* HAVE_DIFFSERV */
#ifdef HAVE_DSTE
#include "dste.h"
#include "nsm_dste.h"
#endif /* HAVE_DSTE */

#include "nsm_mpls.h"
#include "nsm_mpls_vc.h"
#include "nsm_interface.h"
#include "nsm_mpls_rib.h"
#include "nsm_mpls_oam.h"
#include "nsm_mpls_oam_network.h"
#include "nsm_mpls_oam_packet.h"

#ifdef HAVE_VPLS
#include "nsm_vpls.h"
#endif /* HAVE_VPLS */
#include "nsm_router.h"

int
nsm_mpls_oam_set_tlv_hdr (struct mpls_oam_tlv_hdr *tlv_h, u_int16_t type,
                          u_int16_t len)
{
  tlv_h->type = type;
  tlv_h->length = len;

  return NSM_SUCCESS;
}

int
nsm_mpls_set_echo_request_hdr (struct nsm_mpls_oam *oam)
{

  oam->req.omh.ver_no = MPLS_OAM_VERSION;
  oam->req.omh.u.flags.fec = oam->msg.flags;
  oam->req.omh.msg_type = MPLS_ECHO_REQUEST_MESSAGE;
  oam->req.omh.reply_mode = oam->msg.reply_mode;
  oam->req.omh.return_code = NSM_FALSE;
  oam->req.omh.ret_subcode = NSM_FALSE;
  oam->req.omh.sender_handle = oam->sock;

  oam->req.omh.ts_rx_sec = 0;
  oam->req.omh.ts_rx_usec = 0;

  return NSM_SUCCESS;
}

int
nsm_mpls_set_echo_reply_hdr (struct mpls_oam_hdr *req_hdr,
                             struct mpls_oam_hdr *reply_hdr,
                             struct pal_timeval *rx_time,
                             u_char return_code,
                             u_char ret_sub_code)
{

  reply_hdr->ver_no = MPLS_OAM_VERSION;
  reply_hdr->u.flags.fec = NSM_FALSE;
  reply_hdr->msg_type = MPLS_ECHO_REPLY_MESSAGE;
  reply_hdr->reply_mode = MPLS_OAM_IP_UDP_REPLY;
  reply_hdr->return_code = return_code;
  reply_hdr->ret_subcode = ret_sub_code;
  reply_hdr->seq_number = req_hdr->seq_number;
  reply_hdr->sender_handle = req_hdr->sender_handle;

  reply_hdr->ts_tx_sec = req_hdr->ts_tx_sec;
  reply_hdr->ts_tx_usec = req_hdr->ts_tx_usec;
  reply_hdr->ts_rx_sec = rx_time->tv_sec;
  reply_hdr->ts_rx_usec = rx_time->tv_usec;

  return MPLS_OAM_HDR_LEN;
}

int
nsm_mpls_oam_ldp_ivp4_tlv_set (struct nsm_mpls_oam *oam,
                               struct prefix *pfx, 
                               struct ldp_ipv4_prefix *ldp)
{
  int len = 0;

  if ( pfx)
    { 
      /* LDP is being used as an outer tunnel */
      ldp->ip_addr.s_addr = pfx->u.prefix4.s_addr;
      ldp->u.val.len = IPV4_MAX_PREFIXLEN;
    }
  else 
    { 
      ldp->ip_addr.s_addr = oam->msg.u.fec.ip_addr.s_addr;
      ldp->u.val.len = oam->msg.u.fec.prefixlen;
    }

  nsm_mpls_oam_set_tlv_hdr (&ldp->tlv_h,
                            MPLS_OAM_LDP_IPv4_PREFIX_TLV,
                            MPLS_OAM_LDP_IPv4_FIX_LEN);
  len+=(MPLS_OAM_LDP_IPv4_FIX_LEN +  MPLS_OAM_TLV_HDR_FIX_LEN);

  return  (MPLS_OAM_PAD_LEN_BY (len,3));
}

int
nsm_mpls_oam_rsvp_ipv4_tlv_set (struct nsm_mpls_oam *oam,
                                struct ftn_entry *ftn,
                                struct rsvp_ipv4_prefix *rsvp)
{
  int len = 0;

  rsvp->ip_addr.s_addr = ftn->owner.u.r_key.u.ipv4.egr.s_addr;
  rsvp->tunnel_id =  ftn->owner.u.r_key.u.ipv4.trunk_id;
  rsvp->ext_tunnel_id.s_addr = ftn->owner.u.r_key.u.ipv4.ingr.s_addr;
  rsvp->send_addr.s_addr = ftn->owner.u.r_key.u.ipv4.ingr.s_addr;
  rsvp->lsp_id = ftn->owner.u.r_key.u.ipv4.lsp_id;
  nsm_mpls_oam_set_tlv_hdr (&rsvp->tlv_h,
                            MPLS_OAM_RSVP_IPv4_PREFIX_TLV,
                            MPLS_OAM_RSVP_IPv4_FIX_LEN);

  len+=(MPLS_OAM_RSVP_IPv4_FIX_LEN + MPLS_OAM_TLV_HDR_FIX_LEN);

  return len;
}

#ifdef HAVE_VRF
int
nsm_mpls_oam_l3vpn_tlv_set (struct nsm_mpls_oam *oam,
                            struct ftn_entry *ftn,
                            struct mpls_oam_target_fec_stack *fec_tlv)
{
  bool_t vpn_found = NSM_FALSE;
  struct listnode *ln;
  struct nsm_msg_vpn_rd *rd_node;
  struct ftn_entry *t_ftn = NULL;
  struct prefix pfx;
  int len = 0;
  int ret = 0;
  /* We need to send the FEC and the RD of the VPN
   * We DO NOT send the VRF id. Also the RD is only of local
   * significance i.e. Routers not part of the VPN cannot understand it
   */

  LIST_LOOP (oam->nm->nmpls->mpls_vpn_list, rd_node, ln)
    {
      if (rd_node->vrf_id == oam->n_vrf->vrf->id)
        {
          vpn_found = NSM_TRUE;
          pal_mem_cpy (&fec_tlv->l3vpn.rd ,&rd_node->rd, sizeof
                      (struct vpn_rd));
          fec_tlv->l3vpn.vpn_pfx.s_addr = oam->msg.u.fec.vpn_prefix.s_addr;
          fec_tlv->l3vpn.u.val.len = oam->msg.u.fec.prefixlen;
          SET_FLAG (fec_tlv->flags, MPLS_OAM_MPLS_VPN_PREFIX);
          nsm_mpls_oam_set_tlv_hdr (&fec_tlv->l3vpn.tlv_h,
                                    MPLS_OAM_IPv4_VPN_TLV,
                                    MPLS_OAM_IPv4_VPN_FIX_LEN);
          len+=(MPLS_OAM_TLV_HDR_FIX_LEN + MPLS_OAM_IPv4_VPN_FIX_LEN);
          MPLS_OAM_PAD_LEN_BY (len,3);

          /* Set outer Tunnel info - We will always send it */
          pfx.family = AFI_IP;
          pfx.prefixlen = IPV4_MAX_PREFIXLEN;
          pal_mem_cpy (&pfx.u.prefix4, &ftn->owner.u.b_key.u.ipv4.peer,
                       sizeof (struct pal_in4_addr));
          t_ftn = nsm_gmpls_ftn_lookup_installed (oam->nm, GLOBAL_FTN_ID,
                                                &pfx);
          if (t_ftn)
            {
              if (t_ftn->owner.owner == MPLS_LDP)
                {
                  ret = nsm_mpls_oam_ldp_ivp4_tlv_set (oam, &pfx, 
                                                       &fec_tlv->ldp);
                  len+=ret;
                  SET_FLAG (fec_tlv->flags, MPLS_OAM_LDP_IPv4_PREFIX);
                }
              else if (t_ftn->owner.owner == MPLS_RSVP)
                {
                  ret = nsm_mpls_oam_rsvp_ipv4_tlv_set (oam, t_ftn, 
                                                        &fec_tlv->rsvp);
                  len+=ret;
                  SET_FLAG (fec_tlv->flags, MPLS_OAM_RSVP_IPv4_PREFIX);
                }
            }
          break;
        }
    }
  if (! vpn_found)
    return MPLS_OAM_ERR_NO_VPN_FOUND;

  return len;
}
#endif /* HAVE_VRF */

#ifdef HAVE_MPLS_VC
int
nsm_mpls_oam_l2_data_tlv_set (struct nsm_mpls_oam *oam,
                              struct ftn_entry *ftn,
                              struct mpls_oam_target_fec_stack *fec_tlv)

{
  int len = 0;
  int ret = 0;
  struct pal_in4_addr addr;

  /* if VC is not Active or ftn is Null, we need to prevent crash in NSM */
  if ((!oam->u.vc->vc_fib) || (!oam->u.vc->vc_info) || (!ftn))
    return -1;

  pal_mem_set (&fec_tlv->l2_curr.source, 0, sizeof (struct pal_in4_addr));
  pal_mem_set (&fec_tlv->l2_curr.remote, 0, sizeof (struct pal_in4_addr));
  addr.s_addr = pal_hton32(oam->u.vc->vc_fib->lsr_id);
  pal_mem_cpy (&fec_tlv->l2_curr.source, &addr, sizeof (struct pal_in4_addr));
  pal_mem_cpy (&fec_tlv->l2_curr.remote, &oam->u.vc->address.u.prefix4,
                sizeof (struct pal_in4_addr));
  fec_tlv->l2_curr.vc_id = oam->u.vc->id;
  fec_tlv->l2_curr.pw_type = oam->u.vc->vc_info->vc_type;
  SET_FLAG (fec_tlv->flags, MPLS_OAM_L2_CIRCUIT_PREFIX_CURR);
  nsm_mpls_oam_set_tlv_hdr (&fec_tlv->l2_curr.tlv_h,
                            MPLS_OAM_FEC128_CURR_TLV,
                            MPLS_OAM_FEC128_CURR_FIX_LEN);
  len+=(MPLS_OAM_FEC128_CURR_FIX_LEN + MPLS_OAM_TLV_HDR_FIX_LEN);
  MPLS_OAM_PAD_LEN_BY (len, 2);

  /* Set outer Tunnel info */
  if (ftn->owner.owner == MPLS_LDP)
    {
      ret = nsm_mpls_oam_ldp_ivp4_tlv_set (oam, &oam->u.vc->address,
                                           &fec_tlv->ldp);
      len+=ret;
      SET_FLAG (fec_tlv->flags, MPLS_OAM_LDP_IPv4_PREFIX);
    }
  else if (ftn->owner.owner == MPLS_RSVP)
    {
      ret = nsm_mpls_oam_rsvp_ipv4_tlv_set (oam, ftn, &fec_tlv->rsvp);
      len+=ret;
      SET_FLAG (fec_tlv->flags, MPLS_OAM_RSVP_IPv4_PREFIX);
    }

  return len;
}


#ifdef HAVE_VPLS 
/* Note that the L2-VPN Endpoint TLV is not supported as that is for BGP VPLS
 * We are using the standard L2-Circuit TLV for VPLS. */
int
nsm_mpls_oam_vpls_tlv_set (struct nsm_mpls_oam *oam,
                           struct ftn_entry *ftn,
                           struct mpls_oam_target_fec_stack *fec_tlv)
{
  int len = 0;

  /* Source Address Validation not done at the moment
   * The complete FEC Stack Struct is used so that the outer tunnel info can be
   * added later when Linux support exists */
  pal_mem_set (&fec_tlv->l2_curr.source, 0, sizeof (struct pal_in4_addr));
  pal_mem_cpy (&fec_tlv->l2_curr.remote, &oam->msg.u.l2_data.vc_peer,
               sizeof (struct pal_in4_addr));
  fec_tlv->l2_curr.vc_id = oam->u.vpls->vpls_id;
  fec_tlv->l2_curr.pw_type = VC_TYPE_ETH_VLAN;
  SET_FLAG (fec_tlv->flags, MPLS_OAM_L2_CIRCUIT_PREFIX_CURR);
  nsm_mpls_oam_set_tlv_hdr (&fec_tlv->l2_curr.tlv_h,
                            MPLS_OAM_FEC128_CURR_TLV,
                            MPLS_OAM_FEC128_CURR_FIX_LEN);
  len+=(MPLS_OAM_FEC128_CURR_FIX_LEN + MPLS_OAM_TLV_HDR_FIX_LEN);

  return (MPLS_OAM_PAD_LEN_BY (len, 2));
}
#endif /* HAVE_VPLS */
#endif /* HAVE_MPLS_VC */

int
nsm_mpls_oam_gen_ipv4_tlv_set (struct nsm_mpls_oam *oam,
                               struct generic_ipv4_prefix *ipv4)
{
  int len = 0;

  ipv4->ipv4_addr.s_addr = oam->msg.u.fec.ip_addr.s_addr;
  ipv4->u.val.len = oam->msg.u.fec.prefixlen;
  nsm_mpls_oam_set_tlv_hdr (&ipv4->tlv_h,
                            MPLS_OAM_GENERIC_FEC_TLV,
                            MPLS_OAM_GENERIC_IPv4_FIX_LEN);
  len+=(MPLS_OAM_GENERIC_IPv4_FIX_LEN + MPLS_OAM_TLV_HDR_FIX_LEN);

  return (MPLS_OAM_PAD_LEN_BY (len, 3));
}

int
nsm_mpls_oam_nil_fec_tlv_set (struct nsm_mpls_oam *oam,
                              struct nil_fec_prefix *nil_fec,
                              u_int32_t resv_label)
{
  int len = 0;

  nil_fec->u.bits.label = resv_label;
  nsm_mpls_oam_set_tlv_hdr (&nil_fec->tlv_h,
                            MPLS_OAM_NIL_FEC_TLV,
                            MPLS_OAM_NIL_FEC_FIX_LEN);
  len+=(MPLS_OAM_NIL_FEC_FIX_LEN+MPLS_OAM_TLV_HDR_FIX_LEN);

  return len;
}

int
nsm_mpls_oam_fec_tlv_set (struct nsm_mpls_oam *oam,
                          struct ftn_entry *ftn,
                          struct mpls_oam_target_fec_stack *fec_tlv,
                          u_char type)
{
  int len = 0;
  int ret = 0;

#ifdef HAVE_VRF
  if (oam->msg.type == MPLSONM_OPTION_L3VPN)
    {
      ret = nsm_mpls_oam_l3vpn_tlv_set (oam, ftn, fec_tlv);
      if (ret < 0)
        return ret;

      len+=ret;
    }
#endif /* HAVE_VRF */
  if (oam->msg.type== MPLSONM_OPTION_LDP)
    {
      ret = nsm_mpls_oam_ldp_ivp4_tlv_set (oam, NULL, &fec_tlv->ldp);
      if (ret < 0)
        return ret;

      len+=ret;
      SET_FLAG (fec_tlv->flags, MPLS_OAM_LDP_IPv4_PREFIX);
    }
  if ((NSM_CHECK_CTYPE(oam->msg.cindex, NSM_CTYPE_MSG_MPLSONM_OPTION_EGRESS))||
     (NSM_CHECK_CTYPE(oam->msg.cindex, NSM_CTYPE_MSG_MPLSONM_OPTION_TUNNELNAME)))
    {
      ret = nsm_mpls_oam_rsvp_ipv4_tlv_set (oam, ftn, &fec_tlv->rsvp);
      if (ret < 0)
        return ret;

      len+=ret;
      SET_FLAG (fec_tlv->flags, MPLS_OAM_RSVP_IPv4_PREFIX);
    }
#ifdef HAVE_MPLS_VC
  if (oam->msg.type == MPLSONM_OPTION_L2CIRCUIT)
    {
      ret = nsm_mpls_oam_l2_data_tlv_set (oam, ftn, fec_tlv);
      if (ret < 0)
        return ret;

      len+=ret;
    }
#endif /* HAVE_MPLS_VC */
#ifdef HAVE_VPLS
  if (oam->msg.type == MPLSONM_OPTION_VPLS)
    {
      ret = nsm_mpls_oam_vpls_tlv_set (oam, ftn, fec_tlv);
      if (ret < 0)
        return ret;

      len+=ret;
    }
#endif /* HAVE_VPLS */
  if (oam->msg.type == MPLSONM_OPTION_STATIC)
    {
      ret = nsm_mpls_oam_gen_ipv4_tlv_set (oam, &fec_tlv->ipv4);
      if (ret < 0)
        return ret;

      len+=ret;
      SET_FLAG (fec_tlv->flags, MPLS_OAM_GENERIC_IPv4_PREFIX);
    }
  if (CHECK_FLAG (oam->fwd_flags, MPLSONM_FWD_OPTION_NOPHP))
    {
      ret = nsm_mpls_oam_nil_fec_tlv_set (oam, &fec_tlv->nil_fec, 0);

      len+=ret;
      SET_FLAG (fec_tlv->flags, MPLS_OAM_NIL_FEC_PREFIX);
    }

  /* Need to add code for MPLS VC and VPLS */
  nsm_mpls_oam_set_tlv_hdr (&fec_tlv->tlv_h, MPLS_OAM_FEC_TLV,
                            len);

  len += MPLS_OAM_TLV_HDR_FIX_LEN;
  return len;

}

int
nsm_mpls_oam_reply_fec_tlv_set (struct mpls_oam_echo_request *oam_req,
                                struct mpls_oam_target_fec_stack *fec_tlv)
 {

  int len = oam_req->fec_tlv.tlv_h.length;

  if (CHECK_FLAG (oam_req->fec_tlv.flags, MPLS_OAM_MPLS_VPN_PREFIX))
    {
      pal_mem_cpy (&fec_tlv->l3vpn, &oam_req->fec_tlv.l3vpn, sizeof
                  (struct l3vpn_ipv4_prefix));
      SET_FLAG (fec_tlv->flags, MPLS_OAM_MPLS_VPN_PREFIX);
    }
  if (CHECK_FLAG (oam_req->fec_tlv.flags, MPLS_OAM_LDP_IPv4_PREFIX))
    {
      pal_mem_cpy (&fec_tlv->ldp, &oam_req->fec_tlv.ldp, sizeof
                  (struct ldp_ipv4_prefix));
      SET_FLAG (fec_tlv->flags, MPLS_OAM_LDP_IPv4_PREFIX);
    }
  if (CHECK_FLAG (oam_req->fec_tlv.flags, MPLS_OAM_RSVP_IPv4_PREFIX))
    {
      pal_mem_cpy (&fec_tlv->rsvp, &oam_req->fec_tlv.rsvp, sizeof
                  (struct rsvp_ipv4_prefix));
      SET_FLAG (fec_tlv->flags, MPLS_OAM_RSVP_IPv4_PREFIX);
    }
  /* Following 2 cases to be used for VPLS as well */
  if (CHECK_FLAG (oam_req->fec_tlv.flags, MPLS_OAM_L2_CIRCUIT_PREFIX_CURR))
    {
      fec_tlv->l2_curr = oam_req->fec_tlv.l2_curr;
      SET_FLAG (fec_tlv->flags, MPLS_OAM_L2_CIRCUIT_PREFIX_CURR);
    }
  if (CHECK_FLAG (oam_req->fec_tlv.flags, MPLS_OAM_L2_CIRCUIT_PREFIX_DEP))
    {
      pal_mem_set (&fec_tlv->l2_curr.source, 0, sizeof (struct pal_in4_addr));
      fec_tlv->l2_curr.remote = oam_req->fec_tlv.l2_dep.remote;
      fec_tlv->l2_curr.vc_id = oam_req->fec_tlv.l2_dep.vc_id;
      fec_tlv->l2_curr.pw_type = oam_req->fec_tlv.l2_dep.pw_type;
      SET_FLAG (fec_tlv->flags, MPLS_OAM_L2_CIRCUIT_PREFIX_CURR);
      nsm_mpls_oam_set_tlv_hdr (&fec_tlv->l2_curr.tlv_h,
                                MPLS_OAM_FEC128_CURR_TLV,
                                MPLS_OAM_FEC128_CURR_FIX_LEN);

      len+=(MPLS_OAM_FEC128_CURR_FIX_LEN + MPLS_OAM_TLV_HDR_FIX_LEN);
      MPLS_OAM_PAD_LEN_BY (len, 2);
    }
  if (CHECK_FLAG (oam_req->fec_tlv.flags, MPLS_OAM_GENERIC_IPv4_PREFIX))
    {
      fec_tlv->ipv4 = oam_req->fec_tlv.ipv4;
      SET_FLAG (fec_tlv->flags, MPLS_OAM_GENERIC_IPv4_PREFIX);
    }
  if (CHECK_FLAG (oam_req->fec_tlv.flags, MPLS_OAM_NIL_FEC_PREFIX))
    {
      nsm_mpls_oam_set_tlv_hdr (&fec_tlv->nil_fec.tlv_h,
                                MPLS_OAM_NIL_FEC_TLV,
                                MPLS_OAM_NIL_FEC_FIX_LEN);
      SET_FLAG (fec_tlv->flags, MPLS_OAM_NIL_FEC_PREFIX);
    }

  nsm_mpls_oam_set_tlv_hdr (&fec_tlv->tlv_h, MPLS_OAM_FEC_TLV,
                            len);

  len += MPLS_OAM_TLV_HDR_FIX_LEN;
  return len;
}

int
nsm_mpls_oam_dsmap_tlv_set (struct nsm_mpls_oam *oam,
                            struct ftn_entry *ftn,
                            struct mpls_oam_downstream_map *dsmap,
                            u_char type)
{
  int len = 0;
  struct interface *ifp;
  struct shimhdr *shim;
  u_char protocol = 0;

  if (! ftn || !ftn->xc || !ftn->xc->nhlfe)
    return -1;

  ifp = if_lookup_by_index (&oam->nm->vr->ifm,
                            ftn->xc->nhlfe->key.u.ipv4.oif_ix);

  if (! ifp)
    return -1;
  /* Next TLV in Echo message is the DSMAP */
  dsmap->u.data.mtu = ifp->mtu;
  if (!if_is_running (ifp))
    return NSM_FALSE;
  if (CHECK_FLAG (ifp->status, NSM_INTERFACE_IPV4_UNNUMBERED) &&
      (ftn->xc->nhlfe))
    dsmap->u.data.family = MPLS_OAM_IPv4_UNNUMBERED;
  else
    dsmap->u.data.family = MPLS_OAM_IPv4_NUMBERED;

  SET_FLAG (dsmap->u.data.flags, DS_FLAGS_I_BIT);
  /* Always considering that RTR knows next hop address */
  if (dsmap->u.data.family == MPLS_OAM_IPv4_NUMBERED)
    {
      /* Fill with Router id if available */
      if (ftn->owner.owner == MPLS_RSVP)
        {
          dsmap->ds_ip = ftn->owner.u.r_key.u.ipv4.egr;
          dsmap->ds_if_ip = ftn->xc->nhlfe->key.u.ipv4.nh_addr;
        }
      else
        {
          dsmap->ds_ip = ftn->xc->nhlfe->key.u.ipv4.nh_addr;
          dsmap->ds_if_ip = ftn->xc->nhlfe->key.u.ipv4.nh_addr;
        }
    }
  else if (oam->trace_timeout)
    {
      dsmap->u.data.family = MPLS_OAM_IPv4_UNNUMBERED;
      dsmap->ds_ip.s_addr = INADDR_ALLRTRS_GROUP;
      dsmap->ds_if_ip.s_addr = INADDR_ALLRTRS_GROUP;
    }
  else
    {
      dsmap->u.data.family = MPLS_OAM_IPv4_UNNUMBERED;
      dsmap->ds_ip.s_addr = INADDR_LOOPBACK;
      dsmap->ds_if_ip.s_addr = INADDR_LOOPBACK;
    }

  dsmap->multipath_type = 0;
  dsmap->depth = 0;
  dsmap->multipath_len =0;
  dsmap->multi_info = NULL;

  shim = XCALLOC (MTYPE_TMP, sizeof (struct shimhdr));
  
  set_label_net (shim->shim, ftn->xc->nhlfe->key.u.ipv4.out_label);

  /* use ttl for protocol type */
  switch (ftn->owner.owner)
  {
    case (MPLS_LDP):
      protocol = MPLS_PROTO_LDP;
      break;
    case (MPLS_RSVP):
      protocol = MPLS_PROTO_RSVP_TE;
      break;
    case (MPLS_OTHER_BGP):
      protocol = MPLS_PROTO_BGP;
      break;
    case (MPLS_OTHER_CLI):
      protocol = MPLS_PROTO_STATIC;
      break;
    default:
      break;
  }
  set_ttl_net (shim->shim, protocol);
  dsmap->ds_label = list_new();
  listnode_add (dsmap->ds_label, shim);

  /* 4 is for ds_label and protocol */
  len+= MPLS_OAM_DSMAP_TLV_IPv4_FIX_LEN + 4;

  /* Set the TLV header with the length of the packet */
  nsm_mpls_oam_set_tlv_hdr (&dsmap->tlv_h, MPLS_OAM_DSMAP_TLV,
                            len);

  len += MPLS_OAM_TLV_HDR_FIX_LEN;
  return len;
}

int
nsm_mpls_oam_pad_tlv_set (struct mpls_oam_echo_request *oam_req,
                          u_char *val, u_int32_t len)
{
  oam_req->pad_tlv.val = *val;

  nsm_mpls_oam_set_tlv_hdr (&oam_req->pad_tlv.tlv_h, MPLS_OAM_PAD_TLV,
                            len);

  return len;
}

int
nsm_mpls_oam_reply_dsmap_tlv_set (struct mpls_oam_echo_request *req,
                                  struct mpls_oam_downstream_map *dsmap,
                                  struct mpls_label_stack *out_label,
                                  struct interface *ifp,
                                  struct mpls_oam_ctrl_data *ctrl_data)
{
  int len = 0;

  if (! ifp)
    pal_mem_cpy (&req->dsmap, dsmap, sizeof (struct mpls_oam_downstream_map));
  else
    {
      dsmap->u.data.mtu = ifp->mtu;
      if (!if_is_running (ifp))
        return NSM_FALSE;
      if (CHECK_FLAG (ifp->status, NSM_INTERFACE_IPV4_UNNUMBERED))
        dsmap->u.data.family = MPLS_OAM_IPv4_UNNUMBERED;
      else
        dsmap->u.data.family = MPLS_OAM_IPv4_NUMBERED;

      SET_FLAG (dsmap->u.data.flags, DS_FLAGS_I_BIT);

      if (dsmap->u.data.family == MPLS_OAM_IPv4_NUMBERED)
        {
          /* Fill with Router id if available */
          dsmap->ds_ip = ifp->ifc_ipv4->address->u.prefix4;
          dsmap->ds_if_ip = ifp->ifc_ipv4->destination->u.prefix4;
        }

      dsmap->multipath_type = 0;
      dsmap->depth = 0;
      dsmap->multipath_len =0;
      dsmap->multi_info = NULL;

      len+=MPLS_OAM_DSMAP_TLV_IPv4_FIX_LEN;

      dsmap->ds_label = out_label->label;
      len+=((dsmap->ds_label->count)*(sizeof (struct shimhdr)));
   }

  nsm_mpls_oam_set_tlv_hdr (&dsmap->tlv_h, MPLS_OAM_DSMAP_TLV,
                            len);

  len += MPLS_OAM_TLV_HDR_FIX_LEN;
  return len;
}

int
nsm_mpls_oam_reply_if_tlv_set (struct nsm_master *nm,
                               struct mpls_oam_if_lbl_stk *if_lbl_stk,
                               struct mpls_oam_ctrl_data *ctrl_data)
{
  int i;
  int len = MPLS_OAM_TLV_HDR_FIX_LEN;
  struct interface *ifp;

  ifp = if_lookup_by_index (&nm->vr->ifm, ctrl_data->in_ifindex);

  if (! ifp)
    return -1;

  if_lbl_stk->u.fam.family = MPLS_OAM_IPv4_NUMBERED;
  /* Fill this with the Router ID if available */
  if (nm->vr->router_id.s_addr > 0)
    if_lbl_stk->addr = nm->vr->router_id;
  else
    if_lbl_stk->addr = ifp->ifc_ipv4->address->u.prefix4;

  if_lbl_stk->ifindex = ifp->ifc_ipv4->address->u.prefix4.s_addr;

  len+=MPLS_OAM_IFLABEL_TLV_IPv4_FIX_LEN;
  for (i = 0; i < ctrl_data->label_stack_depth; i++)
     {
       if_lbl_stk->out_stack[i] = ctrl_data->label_stack[i];
       len+=4;
     }
  if_lbl_stk->depth = ctrl_data->label_stack_depth;

  nsm_mpls_oam_set_tlv_hdr (&if_lbl_stk->tlv_h, MPLS_OAM_IF_LBL_STACK,
                            len);
  return len;
}

int
nsm_mpls_oam_reply_pad_tlv_set (struct mpls_oam_pad_tlv *rx_pad,
                                struct mpls_oam_pad_tlv *pad_tlv)
{
  pal_mem_cpy (&pad_tlv->tlv_h, &rx_pad->tlv_h,
               sizeof (struct mpls_oam_tlv_hdr));

  pad_tlv->val = rx_pad->val;

  return rx_pad->tlv_h.length;
}

int
nsm_mpls_set_echo_request_data (struct nsm_mpls_oam *oam,
                                struct nsm_mpls_oam_data *data)
 {
  int ret = 0;
  u_int16_t size = 0;

  /* Set the FEC Stack TLV */
  if (oam->req.fec_tlv.tlv_h.type != MPLS_OAM_FEC_TLV)
    {
      ret = nsm_mpls_oam_fec_tlv_set (oam, oam->ftn, &oam->req.fec_tlv,
                                   MPLS_ECHO);
      if (ret < 0)
        return 0; 
      size = ret;
    }
  else
    size = oam->req.fec_tlv.tlv_h.length + MPLS_OAM_TLV_HDR_FIX_LEN;

  if (oam_req_dsmap_tlv == NSM_TRUE)
    {
      /* Set the DSMAP TLV */
      if ((oam->req.dsmap.tlv_h.type != MPLS_OAM_DSMAP_TLV) ||
	  (oam->trace_timeout))
	{
	  ret = nsm_mpls_oam_dsmap_tlv_set (oam, oam->ftn, &oam->req.dsmap,
					    MPLS_ECHO);
	  if (ret < 0)
	    return 0;
	  size+=ret;
	}
      else
	size+=oam->req.dsmap.tlv_h.length + MPLS_OAM_TLV_HDR_FIX_LEN;
    }

  /* Add time and sequence number */
  oam->req.omh.seq_number = data->pkt_count;
  oam->req.omh.ts_tx_sec = data->sent_time.tv_sec;
  oam->req.omh.ts_tx_usec = data->sent_time.tv_usec;

  size+=MPLS_OAM_HDR_LEN;

  return size;
}



