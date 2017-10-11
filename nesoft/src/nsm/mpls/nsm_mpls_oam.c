/* Copyright (C) 2009-10  All Rights Reserved. */

#include "pal.h"
#include "lib.h"

#ifdef HAVE_MPLS_OAM
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
#include "mpls_util.h"
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

#include "oam_mpls_msg.h"
#include "bfd_message.h"


#ifdef HAVE_VPLS
#include "nsm_vpls.h"
#endif /* HAVE_VPLS */
#include "nsm_router.h"

/* Process the OAM echo Request Message and send Response/Error. */
int
nsm_mpls_oam_process_lsp_ping_req (struct nsm_master *nm,
                                   struct nsm_server_entry *nse,
                                   struct nsm_msg_oam_lsp_ping_req_process *msg)
{
#ifdef HAVE_MPLS_VC
  struct nsm_mpls_circuit *vc = NULL;
#ifdef HAVE_VPLS
  struct nsm_vpls *vpls = NULL;
#endif /* HAVE_VPLS */
#endif /* HAVE_MPLS_VC */
#ifdef HAVE_VRF
  struct nsm_vrf *n_vrf = NULL;
#endif /* HAVE_VRF */
  struct ftn_entry *ftn = NULL;
  struct prefix pfx;
  struct fec_gen_entry fec;
  struct gmpls_gen_label tmp_lbl;
  u_char *rsvp;
  int ret = NSM_FAILURE;

  pal_mem_set (&pfx, 0, sizeof (struct prefix));
  pfx.family = AF_INET;
  pfx.prefixlen = IPV4_MAX_PREFIXLEN;

  /* Get FTN pointer to get labels relevant to the FEC Stack
   * Need to identify the correct FTN table in case of VPN */
  if (msg->type == MPLSONM_OPTION_STATIC)
    {
      pfx.prefixlen = msg->u.fec.prefixlen;
      IPV4_ADDR_COPY (&pfx.u.prefix4, &msg->u.fec.ip_addr);
      NSM_MPLS_PREFIX_TO_FEC_GEN_ENTRY (&fec, &pfx);
      ftn = nsm_gmpls_get_static_ftn (nm, &fec);
      if (! ftn)
        return MPLS_OAM_ERR_NO_FTN_FOUND;

      if (ftn->xc && ftn->xc->nhlfe)
        gmpls_nhlfe_outgoing_label (ftn->xc->nhlfe, &tmp_lbl);

      if (! ftn->xc || 
          tmp_lbl.u.pkt == LABEL_IMPLICIT_NULL)
         return MPLS_OAM_ERR_EXPLICIT_NULL;

      ret = nsm_mpls_oam_send_lsp_ping_req_resp_gen (nm, nse, msg, ftn);
      if (ret == NSM_SUCCESS)
        {
          /* Set the flag in FTN to send further updates. */
          ftn->is_oam_enabled = PAL_TRUE;
        }
    }
#ifdef HAVE_VRF
  else if (msg->type == MPLSONM_OPTION_L3VPN)
    {
      pfx.prefixlen = msg->u.fec.prefixlen;
      IPV4_ADDR_COPY (&pfx.u.prefix4, &msg->u.fec.vpn_prefix);

      n_vrf = nsm_vrf_lookup_by_name (nm, msg->u.fec.vrf_name);
      if (! n_vrf)
        return MPLS_OAM_ERR_NO_VPN_FOUND;

      ftn = nsm_gmpls_ftn_lookup_installed (nm, n_vrf->vrf->id, &pfx);
      if (!ftn)
        return MPLS_OAM_ERR_NO_FTN_FOUND;

      if (ftn->xc && ftn->xc->nhlfe)
        gmpls_nhlfe_outgoing_label (ftn->xc->nhlfe, &tmp_lbl);

      if (! ftn->xc || 
          tmp_lbl.u.pkt == LABEL_IMPLICIT_NULL)
        return MPLS_OAM_ERR_EXPLICIT_NULL;

      ret = nsm_mpls_oam_send_lsp_ping_req_resp_l3vpn (nm, nse, msg, 
                                                       n_vrf, ftn);
      if (ret == NSM_SUCCESS)
        {
          /* Set the flag in FTN to send further updates. */
          ftn->is_oam_enabled = PAL_TRUE;
        }
    }
#endif /* HAVE_VRF */
  else if (msg->type == MPLSONM_OPTION_LDP)
    {
      pfx.prefixlen = msg->u.fec.prefixlen;
      IPV4_ADDR_COPY (&pfx.u.prefix4,&msg->u.fec.ip_addr);
      ftn = nsm_gmpls_get_ldp_ftn (nm, &pfx);
      if (! ftn)
        return MPLS_OAM_ERR_NO_FTN_FOUND;

      if (ftn->xc && ftn->xc->nhlfe)
        gmpls_nhlfe_outgoing_label (ftn->xc->nhlfe, &tmp_lbl);

      if (! ftn->xc || 
          tmp_lbl.u.pkt == LABEL_IMPLICIT_NULL)
        return MPLS_OAM_ERR_EXPLICIT_NULL;

       ret = nsm_mpls_oam_send_lsp_ping_req_resp_ldp (nm, nse, msg, ftn);
      if (ret == NSM_SUCCESS)
        {
          /* Set the flag in FTN to send further updates. */
          ftn->is_oam_enabled = PAL_TRUE;
        }
    }
  else if (msg->type == MPLSONM_OPTION_RSVP)
    {
      if (NSM_CHECK_CTYPE (msg->cindex, NSM_CTYPE_MSG_OAM_OPTION_EGRESS))
        {
          IPV4_ADDR_COPY (&pfx.u.prefix4, &msg->u.rsvp.addr);
          NSM_MPLS_PREFIX_TO_FEC_GEN_ENTRY (&fec, &pfx);
          ftn = nsm_gmpls_get_rsvp_ftn (nm, &fec);
        }
      else if (NSM_CHECK_CTYPE (msg->cindex,
                                NSM_CTYPE_MSG_OAM_OPTION_TUNNELNAME))
        {
          rsvp = XCALLOC (MTYPE_TMP, msg->u.rsvp.tunnel_len+1);
          pal_strncpy (rsvp, msg->u.rsvp.tunnel_name, msg->u.rsvp.tunnel_len);
          ftn = nsm_gmpls_get_rsvp_ftn_by_tunnel (nm, rsvp,
                                                 RSVP_TUNNEL_NAME);
          XFREE (MTYPE_TMP, rsvp);
          rsvp = NULL;
        }

      if (! ftn)
        return MPLS_OAM_ERR_NO_FTN_FOUND;

      if (ftn->xc && ftn->xc->nhlfe)
        gmpls_nhlfe_outgoing_label (FTN_NHLFE (ftn), &tmp_lbl);
 
      if (! ftn->xc || 
          tmp_lbl.u.pkt  == LABEL_IMPLICIT_NULL)
        return MPLS_OAM_ERR_EXPLICIT_NULL;

      ret = nsm_mpls_oam_send_lsp_ping_req_resp_rsvp (nm, nse, msg, ftn);
      if (ret == NSM_SUCCESS)
        {
          /* Set the flag in FTN to send further updates. */
          ftn->is_oam_enabled = PAL_TRUE;
        }
    }
#ifdef HAVE_MPLS_VC
  else if (msg->type == MPLSONM_OPTION_L2CIRCUIT)
    {
      vc = nsm_mpls_l2_circuit_lookup_by_id (nm,
                                      msg->u.l2_data.vc_id);

      if (! vc || !vc->vc_fib)
        return MPLS_OAM_ERR_NO_VC_FOUND;

      pfx = vc->address;

      /* This gets a Tunnel for the VC. Try to get MIB code*/
      ftn = nsm_gmpls_ftn_lookup_installed (nm, GLOBAL_FTN_ID, &pfx);
      if (!ftn)
        return MPLS_OAM_ERR_NO_FTN_FOUND;

      if (ftn->xc && ftn->xc->nhlfe)
        gmpls_nhlfe_outgoing_label (FTN_NHLFE (ftn), &tmp_lbl);
      if (! ftn->xc || 
          tmp_lbl.u.pkt == LABEL_IMPLICIT_NULL)
        return MPLS_OAM_ERR_EXPLICIT_NULL;

      ret = nsm_mpls_oam_send_lsp_ping_req_resp_l2vc (nm, nse, msg, vc, 
#ifdef HAVE_VPLS
                                                      NULL, 
#endif /* HAVE_VPLS */
                                                      ftn);
      if (ret == NSM_SUCCESS)
        {
          /* Set the flag in FTN & VC Fib to send further updates. */
          ftn->is_oam_enabled = PAL_TRUE;
          vc->vc_fib->is_oam_enabled = PAL_TRUE;
        }
    }
#ifdef HAVE_VPLS
  else if (msg->type == MPLSONM_OPTION_VPLS)
    {
      IPV4_ADDR_COPY (&pfx.u.prefix4, &msg->u.l2_data.vc_peer);
      vpls = nsm_vpls_lookup (nm, msg->u.l2_data.vc_id,
                              NSM_FALSE);

      if (! vpls)
        return MPLS_OAM_ERR_NO_VPLS_FOUND;

      ret = nsm_mpls_oam_send_lsp_ping_req_resp_l2vc (nm, nse, msg, NULL, vpls, NULL);
    }
#endif /* HAVE_VPLS */
#endif /* HAVE_MPLS_VC */

  return ret;
}

/* Send OAM Response with LDP FEC info. */
int
nsm_mpls_oam_send_lsp_ping_req_resp_ldp (struct nsm_master *nm,
                                         struct nsm_server_entry *nse, 
                                         struct nsm_msg_oam_lsp_ping_req_process *msg,
                                         struct ftn_entry *ftn)
{
  struct nsm_msg_oam_lsp_ping_req_resp_ldp oam_resp;
  struct gmpls_gen_label tmp_lbl;
  int len = 0;
  int ret;

  pal_mem_set (&oam_resp, 0, sizeof (struct nsm_msg_oam_lsp_ping_req_resp_ldp));

  /* Fill the OAM Resonse data with LDP info. */
  nsm_mpls_oam_resp_ldp_ipv4_data_set (msg, NULL, &oam_resp.data);

  /* Fill Dsmap info, if requested. */
  if (msg->is_dsmap)
    {
      ret = nsm_mpls_oam_resp_dsmap_data_set (nm, ftn, &oam_resp.dsmap);
      if (ret < 0)
        return MPLS_OAM_ERR_PACKET_SEND;

      oam_resp.is_dsmap = PAL_TRUE;
    }

  oam_resp.seq_no = msg->seq_no;

  gmpls_nhlfe_outgoing_label (FTN_NHLFE (ftn), &tmp_lbl);
#ifdef HAVE_GMPLS
  oam_resp.oif_ix = nsm_gmpls_get_ifindex_from_gifindex (nm, 
                                     gmpls_nhlfe_outgoing_if (FTN_NHLFE (ftn)));
#else
  oam_resp.oif_ix = gmpls_nhlfe_outgoing_if (FTN_NHLFE (ftn));
#endif /* HAVE_GMPLS */
  set_label_net (oam_resp.label.shim, tmp_lbl.u.pkt);
  oam_resp.ftn_ix = ftn->ftn_ix;

  /* Set nse pointer and size. */
  nse->send.pnt = nse->send.buf + NSM_MSG_HEADER_SIZE;
  nse->send.size = nse->send.len - NSM_MSG_HEADER_SIZE;

  len = nsm_encode_oam_ping_req_resp_ldp (&nse->send.pnt, &nse->send.size, 
                                          &oam_resp);
  if (len < 0)
    return MPLS_OAM_ERR_PACKET_SEND;

  nsm_server_send_message (nse, 0, 0, NSM_MSG_OAM_LSP_PING_REQ_RESP_LDP,
                           0, len);  

  return NSM_SUCCESS;
}

/* Send OAM Response with RSVP FEC info. */
int
nsm_mpls_oam_send_lsp_ping_req_resp_rsvp (struct nsm_master *nm,
                                          struct nsm_server_entry *nse,
                                          struct nsm_msg_oam_lsp_ping_req_process *msg,
                                          struct ftn_entry *ftn)
{
  struct nsm_msg_oam_lsp_ping_req_resp_rsvp oam_resp;
  struct gmpls_gen_label tmp_lbl;
  int len = 0;
  int ret;

  pal_mem_set (&oam_resp, 0, sizeof (struct nsm_msg_oam_lsp_ping_req_resp_rsvp));

  /* Fill the OAM Resonse data with RSVP FEC info. */
  nsm_mpls_oam_resp_rsvp_ipv4_data_set (ftn, &oam_resp.data);

  /* Fill Dsmap info, if requested. */
  if (msg->is_dsmap)
    {
      ret = nsm_mpls_oam_resp_dsmap_data_set (nm, ftn, &oam_resp.dsmap);
      if (ret < 0)
        return MPLS_OAM_ERR_PACKET_SEND;

      oam_resp.is_dsmap = PAL_TRUE;
    }

  oam_resp.seq_no = msg->seq_no;
#ifdef HAVE_GMPLS
  oam_resp.oif_ix = nsm_gmpls_get_ifindex_from_gifindex (nm, 
                                      gmpls_nhlfe_outgoing_if (ftn->xc->nhlfe));
#else
  oam_resp.oif_ix = gmpls_nhlfe_outgoing_if (ftn->xc->nhlfe);
#endif /* HAVE_GMPLS */
  gmpls_nhlfe_outgoing_label (FTN_NHLFE (ftn), &tmp_lbl);

  set_label_net (oam_resp.label.shim, tmp_lbl.u.pkt);
  oam_resp.ftn_ix = ftn->ftn_ix;

  /* Set nse pointer and size. */
  nse->send.pnt = nse->send.buf + NSM_MSG_HEADER_SIZE;
  nse->send.size = nse->send.len - NSM_MSG_HEADER_SIZE;

  len = nsm_encode_oam_ping_req_resp_rsvp (&nse->send.pnt, &nse->send.size,
                                           &oam_resp);
  if (len < 0)
    return MPLS_OAM_ERR_PACKET_SEND;

  nsm_server_send_message (nse, 0, 0, NSM_MSG_OAM_LSP_PING_REQ_RESP_RSVP,
                           0, len);

  return NSM_SUCCESS;
}

/* Send OAM Response with Static FEC info. */
int
nsm_mpls_oam_send_lsp_ping_req_resp_gen (struct nsm_master *nm,
                                         struct nsm_server_entry *nse,
                                         struct nsm_msg_oam_lsp_ping_req_process *msg,
                                         struct ftn_entry *ftn)
{
  struct nsm_msg_oam_lsp_ping_req_resp_gen oam_resp;
  struct gmpls_gen_label tmp_lbl;
  int len = 0;
  int ret;

  pal_mem_set (&oam_resp, 0, sizeof (struct nsm_msg_oam_lsp_ping_req_resp_gen));

  /* Fill the OAM Resonse data with Static FEC info. */
  nsm_mpls_oam_resp_gen_ipv4_data_set (msg, NULL, &oam_resp.data);

  /* Fill Dsmap info, if requested. */
  if (msg->is_dsmap)
    {
      ret = nsm_mpls_oam_resp_dsmap_data_set (nm, ftn, &oam_resp.dsmap);
      if (ret < 0)
        return MPLS_OAM_ERR_PACKET_SEND;

      oam_resp.is_dsmap = PAL_TRUE;
    }

  oam_resp.seq_no = msg->seq_no;
#ifdef HAVE_GMPLS
  oam_resp.oif_ix = nsm_gmpls_get_ifindex_from_gifindex (nm, 
                                      gmpls_nhlfe_outgoing_if (ftn->xc->nhlfe));
#else
  oam_resp.oif_ix = gmpls_nhlfe_outgoing_if (ftn->xc->nhlfe);
#endif /* HAVE_GMPLS */
  gmpls_nhlfe_outgoing_label (FTN_NHLFE (ftn), &tmp_lbl); 
  set_label_net (oam_resp.label.shim, tmp_lbl.u.pkt);
  oam_resp.ftn_ix = ftn->ftn_ix;

  /* Set nse pointer and size. */
  nse->send.pnt = nse->send.buf + NSM_MSG_HEADER_SIZE;
  nse->send.size = nse->send.len - NSM_MSG_HEADER_SIZE;

  len = nsm_encode_oam_ping_req_resp_gen (&nse->send.pnt, &nse->send.size,
                                          &oam_resp);
  if (len < 0)
    return MPLS_OAM_ERR_PACKET_SEND;

  nsm_server_send_message (nse, 0, 0, NSM_MSG_OAM_LSP_PING_REQ_RESP_GEN,
                           0, len);

  return NSM_SUCCESS;
}

#ifdef HAVE_MPLS_VC
/* Send OAM Response with L2VC data. */
int
nsm_mpls_oam_send_lsp_ping_req_resp_l2vc (struct nsm_master *nm,
                                   struct nsm_server_entry *nse,
                                   struct nsm_msg_oam_lsp_ping_req_process *msg,
                                   struct nsm_mpls_circuit *vc,
#ifdef HAVE_VPLS
                                   struct nsm_vpls *vpls, 
#endif /* HAVE_VPLS */
                                   struct ftn_entry *ftn)
{
  struct nsm_msg_oam_lsp_ping_req_resp_l2vc oam_resp;
  struct gmpls_gen_label tmp_lbl;
  int len = 0;
  int ret;

  pal_mem_set (&oam_resp, 0, sizeof (struct nsm_msg_oam_lsp_ping_req_resp_l2vc));

  /* Currently, only PW_128_CURRENT type is supported. */
  oam_resp.l2_data_type = NSM_OAM_L2_CIRCUIT_PREFIX_CURR;

  if (msg->type == MPLSONM_OPTION_L2CIRCUIT)
    {
      /* Fill the OAM Resonse data with L2VC data. */ 
      ret = nsm_mpls_oam_resp_l2_data_set (msg, vc, ftn, &oam_resp);
      if (ret < 0)
        return MPLS_OAM_ERR_PACKET_SEND;
    }
#ifdef HAVE_VPLS
  else if (msg->type == MPLSONM_OPTION_VPLS)
    {
      /* Fill the OAM Resonse data with VPLS data. */
      ret = nsm_mpls_oam_resp_vpls_data_set (msg, vpls, ftn, &oam_resp);
      if (ret < 0)
        return MPLS_OAM_ERR_PACKET_SEND;
    }
#endif /* HAVE_VPLS */

#ifdef HAVE_VCCV
  /* If VCCV is requested, respond with CC type in use */
  if (msg->u.l2_data.is_vccv)
    {
      oam_resp.cc_type = CC_TYPE_NONE;
      if (vc->vc_fib)
        oam_resp.cc_type = oam_util_get_cctype_in_use (vc->cc_types,
                                             vc->vc_fib->remote_cc_types); 
      /* If VCCV is not in use, return error */
      if (oam_resp.cc_type == CC_TYPE_NONE)
        return MPLS_OAM_ERR_VCCV_NOT_IN_USE;
    }
#endif /* HAVE_VCCV */

  /* Fill Dsmap info, if requested. */
  if (msg->is_dsmap)
    {
      ret = nsm_mpls_oam_resp_dsmap_data_set (nm, ftn, &oam_resp.dsmap);
      if (ret < 0)
        return MPLS_OAM_ERR_PACKET_SEND;

      oam_resp.is_dsmap = PAL_TRUE;
    }

  set_label_net (oam_resp.vc_label.shim, vc->vc_fib->out_label);
  gmpls_nhlfe_outgoing_label (FTN_NHLFE (ftn), &tmp_lbl);
  set_label_net (oam_resp.tunnel_label.shim, tmp_lbl.u.pkt);
  oam_resp.seq_no = msg->seq_no;
#ifdef HAVE_GMPLS
  oam_resp.acc_if_index = nsm_gmpls_get_ifindex_from_gifindex (nm, 
                                                          vc->vc_fib->ac_if_ix);
  oam_resp.oif_ix = nsm_gmpls_get_ifindex_from_gifindex (nm, 
                                      gmpls_nhlfe_outgoing_if (ftn->xc->nhlfe));
#else
  oam_resp.acc_if_index = vc->vc_fib->ac_if_ix;
  oam_resp.oif_ix = gmpls_nhlfe_outgoing_if (ftn->xc->nhlfe);
#endif /* HAVE_GMPLS */

  /* Set nse pointer and size. */
  nse->send.pnt = nse->send.buf + NSM_MSG_HEADER_SIZE;
  nse->send.size = nse->send.len - NSM_MSG_HEADER_SIZE;

  len = nsm_encode_oam_ping_req_resp_l2vc (&nse->send.pnt, &nse->send.size,
                                           &oam_resp);
  if (len < 0)
    return MPLS_OAM_ERR_PACKET_SEND;

  nsm_server_send_message (nse, 0, 0, NSM_MSG_OAM_LSP_PING_REQ_RESP_L2VC,
                           0, len);

  return NSM_SUCCESS;
}
#endif /*  HAVE_MPLS_VC */

#ifdef HAVE_VRF
/* Send OAM Response with L3VPN data. */
int
nsm_mpls_oam_send_lsp_ping_req_resp_l3vpn (struct nsm_master *nm,
                                           struct nsm_server_entry *nse,
                                           struct nsm_msg_oam_lsp_ping_req_process *msg,
                                           struct nsm_vrf *n_vrf,
                                           struct ftn_entry *ftn)
{
  struct nsm_msg_oam_lsp_ping_req_resp_l3vpn oam_resp;
  struct gmpls_gen_label tmp_lbl;
  int len = 0;
  int ret;

  pal_mem_set (&oam_resp, 0, sizeof (struct nsm_msg_oam_lsp_ping_req_resp_l3vpn));

  /* Fill the OAM Resonse data with L3VPN data. */
  ret = nsm_mpls_oam_resp_l3vpn_data_set (nm, msg, n_vrf, ftn, &oam_resp);
  if (ret < 0)
    return MPLS_OAM_ERR_PACKET_SEND;

  /* Fill Dsmap info, if requested. */
  if (msg->is_dsmap)
    {
      ret = nsm_mpls_oam_resp_dsmap_data_set (nm, ftn, &oam_resp.dsmap);
      if (ret < 0)
        return MPLS_OAM_ERR_PACKET_SEND;

      oam_resp.is_dsmap = PAL_TRUE;
    }

  /* TODO TODO  Validate the tunnel label and the vpn label*/
  gmpls_nhlfe_outgoing_label (FTN_NHLFE (ftn), &tmp_lbl);
  set_label_net (oam_resp.tunnel_label.shim, tmp_lbl.u.pkt);
  set_label_net (oam_resp.vpn_label.shim, tmp_lbl.u.pkt);
  oam_resp.ftn_ix = ftn->ftn_ix;
  oam_resp.seq_no = msg->seq_no;
  oam_resp.vrf_id = n_vrf->vrf->id;
#ifdef HAVE_GMPLS
  oam_resp.oif_ix = nsm_gmpls_get_ifindex_from_gifindex (nm, 
                                      gmpls_nhlfe_outgoing_if (ftn->xc->nhlfe));
#else
  oam_resp.oif_ix = gmpls_nhlfe_outgoing_if (ftn->xc->nhlfe);
#endif /* HAVE_GMPLS */

  /* Set nse pointer and size. */
  nse->send.pnt = nse->send.buf + NSM_MSG_HEADER_SIZE;
  nse->send.size = nse->send.len - NSM_MSG_HEADER_SIZE;

  len = nsm_encode_oam_ping_req_resp_l3vpn (&nse->send.pnt, &nse->send.size,
                                            &oam_resp);
  if (len < 0)
    return MPLS_OAM_ERR_PACKET_SEND;

  nsm_server_send_message (nse, 0, 0, NSM_MSG_OAM_LSP_PING_REQ_RESP_L3VPN,
                           0, len);

  return NSM_SUCCESS;
}
#endif /* HAVE_VRF */

/* Send OAM error. */
void
nsm_mpls_oam_send_lsp_ping_req_resp_err (struct nsm_server_entry *nse,
                                         struct nsm_msg_oam_lsp_ping_req_process *msg,
                                         int err_code)
{
  struct nsm_msg_oam_lsp_ping_req_resp_err oam_resp_err;
  int len = 0;

  pal_mem_set (&oam_resp_err, 0, 
               sizeof (struct nsm_msg_oam_lsp_ping_req_resp_err));

  oam_resp_err.err_code = err_code;
  oam_resp_err.seq_no = msg->seq_no;

  /* Set nse pointer and size. */
  nse->send.pnt = nse->send.buf + NSM_MSG_HEADER_SIZE;
  nse->send.size = nse->send.len - NSM_MSG_HEADER_SIZE;

  len = nsm_encode_oam_ping_req_resp_err (&nse->send.pnt,
                                          &nse->send.size,
                                          &oam_resp_err);
  if (len < 0)
    return ;

  nsm_server_send_message (nse, 0, 0, NSM_MSG_OAM_LSP_PING_REQ_RESP_ERR,
                           0, len);
}

/* Process the OAM echo Reply Message for LDP data and send Response. */
void
nsm_mpls_oam_process_lsp_ping_rep_ldp (struct nsm_master *nm,
                                       struct nsm_server_entry *nse,
                                       struct nsm_msg_oam_lsp_ping_rep_process_ldp *msg)
{
  struct nsm_msg_oam_lsp_ping_rep_resp oam_resp;

  pal_mem_set (&oam_resp, 0, sizeof (struct nsm_msg_oam_lsp_ping_rep_resp));
  /* Default Return code specifies error. */
  oam_resp.return_code = MPLS_OAM_DEFAULT_RET_CODE;
  oam_resp.seq_no = msg->seq_no;
  oam_resp.is_dsmap = msg->is_dsmap;

  nsm_mpls_oam_process_ldp_ipv4_data (nm, &msg->data, &msg->ctrl_data, 
                                      &oam_resp, msg->is_dsmap, NSM_FALSE);

  nsm_mpls_oam_send_lsp_ping_rep_resp (nse, &oam_resp);
}

/* Process the OAM echo Reply Message for RSVP FEC data and send Response. */
void
nsm_mpls_oam_process_lsp_ping_rep_rsvp (struct nsm_master *nm,
                                        struct nsm_server_entry *nse,
                                        struct nsm_msg_oam_lsp_ping_rep_process_rsvp *msg)
{
  struct nsm_msg_oam_lsp_ping_rep_resp oam_resp;

  pal_mem_set (&oam_resp, 0, sizeof (struct nsm_msg_oam_lsp_ping_rep_resp));
  /* Default Return code specifies error. */
  oam_resp.return_code = MPLS_OAM_DEFAULT_RET_CODE;
  oam_resp.seq_no = msg->seq_no;
  oam_resp.is_dsmap = msg->is_dsmap;

  nsm_mpls_oam_process_rsvp_ipv4_data (nm, &msg->data, &msg->ctrl_data,
                                       &oam_resp, msg->is_dsmap, NSM_FALSE);

  nsm_mpls_oam_send_lsp_ping_rep_resp (nse, &oam_resp);
}

/* Process the OAM echo Reply Message for Gen FEC data and send Response. */
void
nsm_mpls_oam_process_lsp_ping_rep_gen (struct nsm_master *nm,
                                       struct nsm_server_entry *nse,
                                       struct nsm_msg_oam_lsp_ping_rep_process_gen *msg)
{
  struct nsm_msg_oam_lsp_ping_rep_resp oam_resp;

  pal_mem_set (&oam_resp, 0, sizeof (struct nsm_msg_oam_lsp_ping_rep_resp));
  /* Default Return code specifies error. */
  oam_resp.return_code = MPLS_OAM_DEFAULT_RET_CODE;
  oam_resp.seq_no = msg->seq_no;
  oam_resp.is_dsmap = msg->is_dsmap;

  nsm_mpls_oam_process_gen_ipv4_data (nm, &msg->data, &msg->ctrl_data,
                                      &oam_resp, msg->is_dsmap, NSM_FALSE);

  nsm_mpls_oam_send_lsp_ping_rep_resp (nse, &oam_resp);
}

#ifdef HAVE_MPLS_VC
/* Process the OAM echo Reply Message for L2VC data and send Response. */
void
nsm_mpls_oam_process_lsp_ping_rep_l2vc (struct nsm_master *nm,
                                        struct nsm_server_entry *nse,
                                        struct nsm_msg_oam_lsp_ping_rep_process_l2vc *msg)
{
  struct nsm_msg_oam_lsp_ping_rep_resp oam_resp;

  pal_mem_set (&oam_resp, 0, sizeof (struct nsm_msg_oam_lsp_ping_rep_resp));
  /* Default Return code specifies error. */
  oam_resp.return_code = MPLS_OAM_DEFAULT_RET_CODE;
  oam_resp.seq_no = msg->seq_no;
  oam_resp.is_dsmap = msg->is_dsmap;

  nsm_mpls_oam_process_l2vc_data (nm, msg, &oam_resp, msg->is_dsmap);

  nsm_mpls_oam_send_lsp_ping_rep_resp (nse, &oam_resp);
}
#endif /* HAVE_MPLS_VC */

#ifdef HAVE_VRF
/* Process the OAM echo Reply Message for L3VPN data and send Response. */
void
nsm_mpls_oam_process_lsp_ping_rep_l3vpn (struct nsm_master *nm,
                                         struct nsm_server_entry *nse,
                                         struct nsm_msg_oam_lsp_ping_rep_process_l3vpn *msg)
{
  struct nsm_msg_oam_lsp_ping_rep_resp oam_resp;

  pal_mem_set (&oam_resp, 0, sizeof (struct nsm_msg_oam_lsp_ping_rep_resp));
  /* Default Return code specifies error. */
  oam_resp.return_code = MPLS_OAM_DEFAULT_RET_CODE;
  oam_resp.seq_no = msg->seq_no;
  oam_resp.is_dsmap = msg->is_dsmap;

  nsm_mpls_oam_process_l3vpn_data (nm, msg, &oam_resp, msg->is_dsmap);

  nsm_mpls_oam_send_lsp_ping_rep_resp (nse, &oam_resp);
}
#endif /* HAVE_VRF */

void
nsm_mpls_oam_send_lsp_ping_rep_resp (struct nsm_server_entry *nse,
                                     struct nsm_msg_oam_lsp_ping_rep_resp *msg)
{
  int len = 0;

  /* Set nse pointer and size. */
  nse->send.pnt = nse->send.buf + NSM_MSG_HEADER_SIZE;
  nse->send.size = nse->send.len - NSM_MSG_HEADER_SIZE;

  len = nsm_encode_oam_ping_rep_resp (&nse->send.pnt,
                                      &nse->send.size, msg);
  if (len < 0)
    return ;

  nsm_server_send_message (nse, 0, 0, NSM_MSG_OAM_LSP_PING_REP_RESP,
                           0, len);
}

void
nsm_mpls_oam_send_ftn_update (struct nsm_master *nm, struct ftn_entry *ftn)
{
  struct nsm_server_client *nsc;
  struct nsm_server_entry *nse;
  struct nsm_msg_oam_update msg;
  int nbytes;
  int i;

  pal_mem_set (&msg, 0, sizeof (struct nsm_msg_oam_update));

  /* Filling the FTN update message. */
  msg.param_type = NSM_MSG_OAM_FTN_UPDATE;
  msg.param.ftn_update.ftn_ix = ftn->ftn_ix;
  SET_FLAG (msg.param.ftn_update.update_type, 
            NSM_MSG_OAM_UPDATE_TYPE_DELETE);

  NSM_SERVER_CLIENT_LOOP (ng, i, nsc, nse)
    if (nse->service.protocol_id == APN_PROTO_OAM)
      {
        /* Set nse pointer and size. */
        nse->send.pnt = nse->send.buf + NSM_MSG_HEADER_SIZE;
        nse->send.size = nse->send.len - NSM_MSG_HEADER_SIZE;

        /* Encode FTN Update message. */
        nbytes = nsm_encode_oam_update (&nse->send.pnt, &nse->send.size, &msg);
        if (nbytes < 0)
          return ;

        /* Send OAM Update message. */
        nsm_server_send_message (nse, 0, 0, NSM_MSG_OAM_UPDATE, 0, nbytes);
      }
}

#ifdef HAVE_MPLS_VC
void
nsm_mpls_oam_send_vc_update (struct nsm_master *nm, struct nsm_mpls_circuit *vc)
{
  struct nsm_server_client *nsc = NULL;
  struct nsm_server_entry *nse = NULL;
  struct nsm_msg_oam_update msg;
  int nbytes;
  int i;

  pal_mem_set (&msg, 0, sizeof (struct nsm_msg_oam_update));

  /* Filling the VC update message. */
  msg.param_type = NSM_MSG_OAM_VC_UPDATE;
  msg.param.vc_update.vc_id = vc->id;
  IPV4_ADDR_COPY (&msg.param.vc_update.peer, &vc->address.u.prefix4);
  SET_FLAG (msg.param.ftn_update.update_type,
            NSM_MSG_OAM_UPDATE_TYPE_DELETE);

  NSM_SERVER_CLIENT_LOOP (ng, i, nsc, nse)
    if (nse->service.protocol_id == APN_PROTO_OAM)
      {
        /* Set nse pointer and size. */
        nse->send.pnt = nse->send.buf + NSM_MSG_HEADER_SIZE;
        nse->send.size = nse->send.len - NSM_MSG_HEADER_SIZE;

        /* Encode VC Update message. */
        nbytes = nsm_encode_oam_update (&nse->send.pnt, &nse->send.size, &msg);
        if (nbytes < 0)
          return ;

        /* Send OAM Update message. */
        nsm_server_send_message (nse, 0, 0, NSM_MSG_OAM_UPDATE, 0, nbytes);
      }
}
#endif /* HAVE_MPLS_VC */

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
                                   u_char owner)
{
  int i;
  struct shimhdr *out_label;
  struct gmpls_gen_label tmp_lbl;
  struct ilm_entry *ilm = NULL;

  local_stack->label = list_new ();

  for (i = 0; i < ctrl_data->label_stack_depth; i++)
     {
       tmp_lbl.type = gmpls_entry_type_ip;
       tmp_lbl.u.pkt = ctrl_data->label_stack[i];

       ilm = nsm_gmpls_ilm_lookup_by_owner (nm, 
#ifdef HAVE_GMPLS
                 nsm_gmpls_get_gifindex_from_ifindex(nm, ctrl_data->in_ifindex),
#else
                 ctrl_data->in_ifindex,
#endif /* HAVE_GMPLS */
                 &tmp_lbl, owner);
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
#ifdef HAVE_GMPLS
                   local_stack->ifindex = 
                      nsm_gmpls_get_ifindex_from_gifindex (nm, 
                                      gmpls_nhlfe_outgoing_if (ilm->xc->nhlfe));
#else
                   local_stack->ifindex = gmpls_nhlfe_outgoing_if (ilm->xc->nhlfe);
#endif /* HAVE_GMPLS */
                   out_label = XMALLOC (MTYPE_TMP, sizeof (struct shimhdr));
                   gmpls_nhlfe_outgoing_label (ilm->xc->nhlfe, &tmp_lbl); 
                   set_label_net (out_label->shim, tmp_lbl.u.pkt);
                   listnode_add (local_stack->label, out_label);
                   return i;
                 }

             }
           else
             continue;
         }
     }
  return -1;
}

void
nsm_mpls_oam_resp_ldp_ipv4_data_set (struct nsm_msg_oam_lsp_ping_req_process *msg,
                                     struct prefix *pfx, 
                                     struct ldp_ipv4_prefix *ldp)
{
  if (pfx)
    { 
      /* LDP is being used as an outer tunnel */
      ldp->ip_addr.s_addr = pfx->u.prefix4.s_addr;
      ldp->u.val.len = IPV4_MAX_PREFIXLEN;
    }
  else 
    { 
      ldp->ip_addr.s_addr = msg->u.fec.ip_addr.s_addr;
      ldp->u.val.len = msg->u.fec.prefixlen;
    }
}

void
nsm_mpls_oam_resp_rsvp_ipv4_data_set (struct ftn_entry *ftn,
                                      struct rsvp_ipv4_prefix *rsvp)
{
  rsvp->ip_addr.s_addr = ftn->owner.u.r_key.u.ipv4.egr.s_addr;
  rsvp->tunnel_id =  ftn->owner.u.r_key.u.ipv4.trunk_id;
  rsvp->ext_tunnel_id.s_addr = ftn->owner.u.r_key.u.ipv4.ingr.s_addr;
  rsvp->send_addr.s_addr = ftn->owner.u.r_key.u.ipv4.ingr.s_addr;
  rsvp->lsp_id = ftn->owner.u.r_key.u.ipv4.lsp_id;
}

#ifdef HAVE_VRF
int
nsm_mpls_oam_resp_l3vpn_data_set (struct nsm_master *nm,
                                  struct nsm_msg_oam_lsp_ping_req_process *msg,
                                  struct nsm_vrf *n_vrf,
                                  struct ftn_entry *ftn,
                                  struct nsm_msg_oam_lsp_ping_req_resp_l3vpn *l3vpn)
{
  bool_t vpn_found = NSM_FALSE;
  struct listnode *ln;
  struct nsm_msg_vpn_rd *rd_node;
  struct ftn_entry *t_ftn = NULL;
  struct prefix pfx;

  /* We need to send the FEC and the RD of the VPN
   * We DO NOT send the VRF id. Also the RD is only of local
   * significance i.e. Routers not part of the VPN cannot understand it
   */

  LIST_LOOP (nm->nmpls->mpls_vpn_list, rd_node, ln)
    {
      if (rd_node->vrf_id == n_vrf->vrf->id)
        {
          vpn_found = NSM_TRUE;
          pal_mem_cpy (&l3vpn->data.rd ,&rd_node->rd, 
                       sizeof (struct vpn_rd));
          l3vpn->data.vpn_pfx.s_addr = msg->u.fec.vpn_prefix.s_addr;
          l3vpn->data.u.val.len = msg->u.fec.prefixlen;

          /* Set outer Tunnel info - We will always send it */
          pal_mem_set (&pfx, 0, sizeof (struct prefix));
          pfx.family = AFI_IP;
          pfx.prefixlen = IPV4_MAX_PREFIXLEN;
          pal_mem_cpy (&pfx.u.prefix4, &ftn->owner.u.b_key.u.ipv4.peer,
                       sizeof (struct pal_in4_addr));
          t_ftn = nsm_gmpls_ftn_lookup_installed (nm, GLOBAL_FTN_ID,
                                                 &pfx);
          if (t_ftn)
            {
              if (t_ftn->owner.owner == MPLS_LDP)
                {
                  l3vpn->tunnel_type = NSM_OAM_LDP_IPv4_PREFIX;

                  nsm_mpls_oam_resp_ldp_ipv4_data_set (msg, &pfx,
                                                  &l3vpn->tunnel_data.ldp);
                }
              else if (t_ftn->owner.owner == MPLS_RSVP)
                {
                  l3vpn->tunnel_type = NSM_OAM_RSVP_IPv4_PREFIX;

                  nsm_mpls_oam_resp_rsvp_ipv4_data_set (ftn, 
                                                   &l3vpn->tunnel_data.rsvp);
                }
              else
                {
                  l3vpn->tunnel_type = NSM_OAM_GEN_IPv4_PREFIX;

                  nsm_mpls_oam_resp_gen_ipv4_data_set (msg, &pfx,
                                                       &l3vpn->tunnel_data.gen); 
                }
            }
          break;
        }
    }

  if (! vpn_found)
    return MPLS_OAM_ERR_NO_VPN_FOUND;

  return NSM_SUCCESS;
}
#endif /* HAVE_VRF */

#ifdef HAVE_MPLS_VC
int
nsm_mpls_oam_resp_l2_data_set (struct nsm_msg_oam_lsp_ping_req_process *msg,
                               struct nsm_mpls_circuit *vc,
                               struct ftn_entry *ftn,
                               struct nsm_msg_oam_lsp_ping_req_resp_l2vc *oam_resp)
{
  struct pal_in4_addr addr;

  /* if VC is not Active or ftn is Null, we need to prevent crash in NSM */
  if ((!vc->vc_fib) || (!vc->vc_info) || (!ftn))
    return NSM_FAILURE;

  pal_mem_set (&oam_resp->l2_data.pw_128_cur.source, 0, sizeof (struct pal_in4_addr));
  pal_mem_set (&oam_resp->l2_data.pw_128_cur.remote, 0, sizeof (struct pal_in4_addr));
  addr.s_addr = pal_hton32(vc->vc_fib->lsr_id);
  pal_mem_cpy (&oam_resp->l2_data.pw_128_cur.source, &addr, 
               sizeof (struct pal_in4_addr));
  pal_mem_cpy (&oam_resp->l2_data.pw_128_cur.remote, &vc->address.u.prefix4,
               sizeof (struct pal_in4_addr));
  oam_resp->l2_data.pw_128_cur.vc_id = vc->id;
  oam_resp->l2_data.pw_128_cur.pw_type = vc->vc_info->vc_type;
  oam_resp->ftn_ix = ftn->ftn_ix;

  /* Set outer Tunnel info */
  if (ftn->owner.owner == MPLS_LDP)
    {
      oam_resp->tunnel_type = NSM_OAM_LDP_IPv4_PREFIX;

      nsm_mpls_oam_resp_ldp_ipv4_data_set (msg, &vc->address,
                                           &oam_resp->tunnel_data.ldp);
    }
  else if (ftn->owner.owner == MPLS_RSVP)
    {
      oam_resp->tunnel_type = NSM_OAM_RSVP_IPv4_PREFIX;

      nsm_mpls_oam_resp_rsvp_ipv4_data_set (ftn, &oam_resp->tunnel_data.rsvp);
    }
  else
    {
      oam_resp->tunnel_type = NSM_OAM_GEN_IPv4_PREFIX;

      nsm_mpls_oam_resp_gen_ipv4_data_set (msg, &vc->address,
                                           &oam_resp->tunnel_data.gen);  
    }

  return NSM_SUCCESS;
}

#ifdef HAVE_VPLS 
int
nsm_mpls_oam_resp_vpls_data_set (struct nsm_msg_oam_lsp_ping_req_process *msg,
                                 struct nsm_vpls *vpls,
                                 struct ftn_entry *ftn,
                                 struct nsm_msg_oam_lsp_ping_req_resp_l2vc *oam_resp)
{
  if (!vpls || !ftn)
    return NSM_FAILURE;

  /* Source Address Validation not done at the moment
   * The complete FEC Stack Struct is used so that the outer tunnel info can be
   * added later when Linux support exists */
  pal_mem_set (&oam_resp->l2_data.pw_128_cur.source, 0, sizeof (struct pal_in4_addr));
  pal_mem_cpy (&oam_resp->l2_data.pw_128_cur.remote, &msg->u.l2_data.vc_peer,
               sizeof (struct pal_in4_addr));
  oam_resp->l2_data.pw_128_cur.vc_id = vpls->vpls_id;
  oam_resp->l2_data.pw_128_cur.pw_type = VC_TYPE_ETH_VLAN;

  /* Set outer Tunnel info */
  if (ftn->owner.owner == MPLS_LDP)
    {
      oam_resp->tunnel_type = NSM_OAM_LDP_IPv4_PREFIX;

      nsm_mpls_oam_resp_ldp_ipv4_data_set (msg, NULL,
                                           &oam_resp->tunnel_data.ldp);
    }
  else if (ftn->owner.owner == MPLS_RSVP)
    {
      oam_resp->tunnel_type = NSM_OAM_RSVP_IPv4_PREFIX;

      nsm_mpls_oam_resp_rsvp_ipv4_data_set (ftn, &oam_resp->tunnel_data.rsvp);
    }
  else
    {
      oam_resp->tunnel_type = NSM_OAM_GEN_IPv4_PREFIX;

      nsm_mpls_oam_resp_gen_ipv4_data_set (msg, NULL,
                                           &oam_resp->tunnel_data.gen);
    }

  return NSM_SUCCESS;
}
#endif /* HAVE_VPLS */
#endif /* HAVE_MPLS_VC */

void
nsm_mpls_oam_resp_gen_ipv4_data_set (struct nsm_msg_oam_lsp_ping_req_process *msg,
                                     struct prefix *pfx,              
                                     struct generic_ipv4_prefix *ipv4)
{
  if (pfx)
    {
      /* Static LSP is being used as an outer tunnel */
      ipv4->ipv4_addr.s_addr = pfx->u.prefix4.s_addr;
      ipv4->u.val.len = IPV4_MAX_PREFIXLEN;
    }
  else
    {
      ipv4->ipv4_addr.s_addr = msg->u.fec.ip_addr.s_addr;
      ipv4->u.val.len = msg->u.fec.prefixlen;
    }
}

int
nsm_mpls_oam_resp_dsmap_data_set (struct nsm_master *nm,
                                  struct ftn_entry *ftn,
                                  struct mpls_oam_downstream_map *dsmap)
{
  struct interface *ifp;
  struct shimhdr *shim;
  struct gmpls_gen_label tmp_lbl;
  struct addr_in nh_addr;
  u_char protocol = 0;

#ifdef HAVE_GMPLS
  ifp = if_lookup_by_index (&nm->vr->ifm, 
                            nsm_gmpls_get_ifindex_from_gifindex (nm, 
                                     gmpls_nhlfe_outgoing_if (ftn->xc->nhlfe)));
#else
  ifp = if_lookup_by_index (&nm->vr->ifm, 
                            gmpls_nhlfe_outgoing_if (ftn->xc->nhlfe));
#endif /* HAVE_GMPLS */

  if (! ifp)
    return NSM_FAILURE;

  dsmap->u.data.mtu = ifp->mtu;

  if (!if_is_running (ifp))
    return NSM_FAILURE;

  if (CHECK_FLAG (ifp->status, NSM_INTERFACE_IPV4_UNNUMBERED) &&
      (ftn->xc->nhlfe))
    {
      dsmap->u.data.family = MPLS_OAM_IPv4_UNNUMBERED;
      dsmap->ds_ip.s_addr = INADDR_LOOPBACK;
      dsmap->ds_if_ip.s_addr = INADDR_LOOPBACK;
    }
  else
    {
      dsmap->u.data.family = MPLS_OAM_IPv4_NUMBERED;

      /* Fill with Router id if available */
      if (ftn->owner.owner == MPLS_RSVP)
        {
          gmpls_nhlfe_nh_addr (ftn->xc->nhlfe, &nh_addr);
          dsmap->ds_ip = ftn->owner.u.r_key.u.ipv4.egr;
          IPV4_ADDR_COPY (&dsmap->ds_if_ip, &nh_addr.u.ipv4);
         
        }
      else
        {
          gmpls_nhlfe_nh_addr (ftn->xc->nhlfe, &nh_addr);
          IPV4_ADDR_COPY (&dsmap->ds_if_ip, &nh_addr.u.ipv4);
          IPV4_ADDR_COPY (&dsmap->ds_ip, &nh_addr.u.ipv4);
        }
    }

  SET_FLAG (dsmap->u.data.flags, DS_FLAGS_I_BIT);

  dsmap->multipath_type = 0;
  dsmap->depth = 0;
  dsmap->multipath_len =0;
  dsmap->multi_info = NULL;

  shim = XCALLOC (MTYPE_TMP, sizeof (struct shimhdr));
 
  gmpls_nhlfe_outgoing_label (FTN_NHLFE (ftn), &tmp_lbl); 
  set_label_net (shim->shim, tmp_lbl.u.pkt);

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

  return NSM_SUCCESS;
}

/* Process OAM echo reply message for LDP data and fill Response data. */
void
nsm_mpls_oam_process_ldp_ipv4_data (struct nsm_master *nm,
                                    struct ldp_ipv4_prefix *ldp_data,
                                    struct mpls_oam_ctrl_data *ctrl_data,
                                    struct nsm_msg_oam_lsp_ping_rep_resp *oam_resp,
                                    bool_t is_dsmap, u_char lookup)
{
  int ret;
  struct prefix *p, pfx;
  struct ftn_entry *ftn;
  struct interface *ifp = NULL;
  struct mpls_label_stack local_stack;
  int stack_depth = 1;

  pal_mem_set(&local_stack, 0, sizeof (struct mpls_label_stack));
  p = &pfx;
  p->family = AF_INET;
  p->prefixlen = ldp_data->u.val.len;
  IPV4_ADDR_COPY (&p->u.prefix4, &ldp_data->ip_addr);

  ftn = nsm_gmpls_get_ldp_ftn (nm, p);
  if (! ftn)
    {
      /* Egress processing */
      /* DO an IP Route lookup */
      ret = nsm_mpls_oam_route_ipv4_lookup (nm, 0,
                                            &ldp_data->ip_addr,
                                            ldp_data->u.val.len);
      if (ret == NSM_SUCCESS)
        {
          if (lookup == NSM_FALSE)
            oam_resp->return_code = MPLS_OAM_RTR_IS_EGRESS_AT_DEPTH;
          else
            oam_resp->return_code = MPLS_OAM_LBL_SWITCH_IP_FWD_AT_DEPTH;

          oam_resp->ret_subcode = 0;
        }
      else
        {
          /* There is no Route of any kind on the Machine
           * This is an error condition
           */
        }
    }
  else
    {
      /* We have an FTN so we might be a transit
       * Do ILM lookup to confirm
       */
      ret = nsm_mpls_oam_transit_stack_lookup (nm, ctrl_data, &local_stack,
                                               p, MPLS_LDP);
      if (ret >= 0)
        {
          stack_depth = ret;
          oam_resp->return_code = MPLS_OAM_LBL_SWITCH_AT_DEPTH;
          oam_resp->ret_subcode = stack_depth;
        }
    }

/* Return code less than MPLS_OAM_ERRORED_TLV means error, 
 * DSMAP data is not processed in this case. 
 */
  if ((oam_resp->return_code > MPLS_OAM_ERRORED_TLV) && 
      (! lookup) && (is_dsmap))
    {
      ifp = ifg_lookup_by_index (&nm->zg->ifg, local_stack.ifindex);

      ret = nsm_mpls_oam_reply_dsmap_data_set (&oam_resp->dsmap, 
                                         &local_stack, ifp);
      if (ret != NSM_SUCCESS)
        oam_resp->is_dsmap = PAL_FALSE;
    }
}

/* Process OAM echo reply message for RSVP data and fill Response data. */
void
nsm_mpls_oam_process_rsvp_ipv4_data (struct nsm_master *nm,
                                     struct rsvp_ipv4_prefix *rsvp_data,
                                     struct mpls_oam_ctrl_data *ctrl_data,
                                     struct nsm_msg_oam_lsp_ping_rep_resp *oam_resp,
                                     bool_t is_dsmap,
                                     u_char lookup)
{
  int ret;
  struct prefix *p, pfx;
  struct mpls_label_stack local_stack;
  struct interface *ifp, *t_ifp;

  int stack_depth = 1;

  ifp = NULL;
  t_ifp = NULL;
  pal_mem_set(&local_stack, 0, sizeof (struct mpls_label_stack));
  p = &pfx;
  p->family = AF_INET;
  p->prefixlen = IPV4_MAX_PREFIXLEN;
  IPV4_ADDR_COPY (&p->u.prefix4, &rsvp_data->ip_addr);

  t_ifp = if_lookup_by_prefix (&nm->vr->ifm, p);
  if (t_ifp)
    {
      /* Egress Processing */
      oam_resp->return_code = MPLS_OAM_RTR_IS_EGRESS_AT_DEPTH;
      oam_resp->ret_subcode = stack_depth;
    }
  else
    {
      /* We dont have an Interface with the Address configured
       * We might be a transit. Do ILM to confirm.
       */
      ret = nsm_mpls_oam_transit_stack_lookup (nm, ctrl_data, &local_stack,
                                               p, MPLS_RSVP);
      if (ret >= 0)
        {
          stack_depth = ret;
          oam_resp->return_code = MPLS_OAM_LBL_SWITCH_AT_DEPTH;
          oam_resp->ret_subcode = stack_depth;
        }
    }

/* Return code less than MPLS_OAM_ERRORED_TLV means error, 
 * DSMAP data is not processed in this case. 
 */
  if ((oam_resp->return_code > MPLS_OAM_ERRORED_TLV) && 
      (! lookup) && 
      (is_dsmap))
    {
      ifp = ifg_lookup_by_index (&nm->zg->ifg, local_stack.ifindex);
     
      ret = nsm_mpls_oam_reply_dsmap_data_set (&oam_resp->dsmap,
                                         &local_stack, ifp);
      if (ret != NSM_SUCCESS)
        oam_resp->is_dsmap = PAL_FALSE;
    }
}

/* Process OAM echo reply message for Gen FEC data and fill Response data. */
void
nsm_mpls_oam_process_gen_ipv4_data (struct nsm_master *nm,
                                    struct generic_ipv4_prefix *gen_data,
                                    struct mpls_oam_ctrl_data *ctrl_data,
                                    struct nsm_msg_oam_lsp_ping_rep_resp *oam_resp,
                                    bool_t is_dsmap,
                                    u_char lookup)
{
  int ret;
  struct prefix *p, pfx;
  struct mpls_label_stack local_stack;
  struct interface *ifp = NULL; 

  int stack_depth = 1;

  pal_mem_set(&local_stack, 0, sizeof (struct mpls_label_stack));
  p = &pfx;
  p->family = AF_INET;
  p->prefixlen = IPV4_MAX_PREFIXLEN;
  IPV4_ADDR_COPY (&p->u.prefix4, &gen_data->ipv4_addr);

  /* We might be a transit. Do ILM to confirm.
  */
  int ret1 = 0;
  int ret2 = 0;
  ret = nsm_mpls_oam_transit_stack_lookup (nm, ctrl_data, &local_stack,
                                           p, MPLS_SNMP);
  if (ret < 0)
    {
      /* Just Checking for the other CLI cases of LSP's */
      ret1 = nsm_mpls_oam_transit_stack_lookup (nm, ctrl_data, &local_stack,
                                                p, MPLS_OTHER_CLI);
       if (ret1 >= 0)
         {
           stack_depth = ret;
           oam_resp->return_code = MPLS_OAM_LBL_SWITCH_AT_DEPTH;
           oam_resp->ret_subcode = stack_depth;
         }
       else
         {
           ret2 = nsm_mpls_oam_route_ipv4_lookup (nm, 0,
                                                 &gen_data->ipv4_addr,
                                                 gen_data->u.val.len);
  
          if (ret2 == NSM_SUCCESS)
            {
              if (lookup == NSM_FALSE)
                oam_resp->return_code = MPLS_OAM_RTR_IS_EGRESS_AT_DEPTH;
              else
                oam_resp->return_code = MPLS_OAM_LBL_SWITCH_IP_FWD_AT_DEPTH;
  
              oam_resp->ret_subcode = 0;
            }
          else
            {
              /* There is no Route of any kind on the Machine
               * This is an error condition
               */
            }

         }      		
    }
  else
    {
      stack_depth = ret;
      oam_resp->return_code = MPLS_OAM_LBL_SWITCH_AT_DEPTH;
      oam_resp->ret_subcode = stack_depth;
    }

/* Return code less than MPLS_OAM_ERRORED_TLV means error, 
 * DSMAP data is not processed in this case. 
 */
  if ((oam_resp->return_code > MPLS_OAM_ERRORED_TLV) && 
      (! lookup) &&
      (is_dsmap))
    {
      ifp = ifg_lookup_by_index (&nm->zg->ifg, local_stack.ifindex);

      ret = nsm_mpls_oam_reply_dsmap_data_set (&oam_resp->dsmap,
                                         &local_stack, ifp);
      if (ret != NSM_SUCCESS)
        oam_resp->is_dsmap = PAL_FALSE;
    }
}

#ifdef HAVE_MPLS_VC
/* Process OAM echo reply message for L2VC data and fill Response data. */
void
nsm_mpls_oam_process_l2vc_data (struct nsm_master *nm,
                                struct nsm_msg_oam_lsp_ping_rep_process_l2vc *msg,
                                struct nsm_msg_oam_lsp_ping_rep_resp *oam_resp,
                                bool_t is_dsmap)
{
  struct nsm_mpls_circuit *vc = NULL;
#ifdef HAVE_VPLS
  struct nsm_vpls *vpls = NULL;
#endif /* HAVE_VPLS */
  u_int32_t vc_id;
  u_int8_t stack_depth = 1;
  struct addr_in addr;
  struct pal_in4_addr source;
  struct mpls_label_stack local_stack;
  struct interface *ifp = NULL;
  int ret;

  addr.afi = AFI_IP;
  pal_mem_set(&local_stack, 0, sizeof (struct mpls_label_stack));
/* to be filled */
  /* Label stack not filled any where */

  if (msg->l2_data_type == NSM_OAM_L2_CIRCUIT_PREFIX_DEP)
    {
      vc_id = msg->l2_data.pw_128_dep.vc_id;
      IPV4_ADDR_COPY (&addr.u.ipv4, &msg->l2_data.pw_128_dep.remote);
      IPV4_ADDR_COPY (&source, &msg->ctrl_data.src_addr);
    }
  else
    {
      vc_id = msg->l2_data.pw_128_cur.vc_id;
      IPV4_ADDR_COPY (&addr.u.ipv4, &msg->l2_data.pw_128_cur.remote);
      IPV4_ADDR_COPY (&source, &msg->l2_data.pw_128_cur.source);
    }

  /* Need to do a VC lookup by id */
  vc = nsm_mpls_l2_circuit_lookup_by_id (nm, vc_id);

  if (vc)
    {
      if (IPV4_ADDR_SAME (&source, &vc->address.u.prefix4))
        {
#ifdef HAVE_VCCV
        /* If Received it over VCCV Control Channel, Validate it */
        if (msg->ctrl_data.input1)
          {
            u_int8_t cctype_in_use = CC_TYPE_NONE;

            if (vc->vc_fib)
              cctype_in_use = oam_util_get_cctype_in_use(vc->cc_types, 
                                                   vc->vc_fib->remote_cc_types);
            if(!mpls_util_is_valid_cc_type(cctype_in_use,
                                            msg->ctrl_data.input1))
              {
                oam_resp->err_code = MPLS_OAM_VCCV_CCTYPE_MISMATCH;
                /* Invalid CC Type. Increment the CC Mismatch discards */
                if (NSM_MPLS)
                  NSM_MPLS->vccv_stats.cc_mismatch_discards++;
              }
          }
#endif /* HAVE_VCCV */

        if (!oam_resp->err_code)
          {
            oam_resp->return_code = MPLS_OAM_RTR_IS_EGRESS_AT_DEPTH;
            oam_resp->ret_subcode = stack_depth;
          }
        }
    }
#ifdef HAVE_VPLS
  else
    {
      /* Since VPLS is not separately mapped lets do a lookup for that */
      vpls = nsm_vpls_lookup (nm, vc_id, NSM_FALSE);
      if (! vpls)
        oam_resp->return_code = MPLS_OAM_NO_MAPPING_AT_DEPTH;
      else
        {
          struct nsm_vpls_peer *peer;
          /* Need to find the peer */
          peer = nsm_vpls_mesh_peer_lookup (vpls, &addr, NSM_FALSE);
          if (! peer)
            oam_resp->return_code = MPLS_OAM_NO_MAPPING_AT_DEPTH;
          else
           {
             oam_resp->return_code = MPLS_OAM_RTR_IS_EGRESS_AT_DEPTH;
           }
        }
    }
#endif /* HAVE_VPLS */

  if (!vc 
#ifdef HAVE_VPLS
      && !vpls
#endif /* HAVE_VPLS */
      )
    {
      if (msg->tunnel_type == MPLS_OAM_RSVP_IPv4_PREFIX)
        {
          /* Outer tunnel is RSVP */
          nsm_mpls_oam_process_rsvp_ipv4_data (nm, &msg->tunnel_data.rsvp, 
                                               &msg->ctrl_data, oam_resp,
                                                NSM_FALSE, NSM_TRUE);
        }
      else if (msg->tunnel_type == MPLS_OAM_LDP_IPv4_PREFIX)
        {
          /* Outer Tunnel is LDP */
          nsm_mpls_oam_process_ldp_ipv4_data (nm, &msg->tunnel_data.ldp,
                                              &msg->ctrl_data,
                                              oam_resp, NSM_FALSE, NSM_TRUE);
        }
      else
        {
          ret = nsm_mpls_oam_route_ipv4_lookup (nm, 0,
                                                &addr.u.ipv4,
                                                IPV4_MAX_PREFIXLEN);
          if (ret == NSM_SUCCESS)
            {
              oam_resp->return_code = MPLS_OAM_LBL_SWITCH_AT_DEPTH;
              oam_resp->ret_subcode = 0;
             }
        }
    }

/* Return code less than MPLS_OAM_ERRORED_TLV means error, 
 * DSMAP data is not processed in this case. 
 */
  if (!oam_resp->err_code && 
      (oam_resp->return_code > MPLS_OAM_ERRORED_TLV) && is_dsmap)
    {
      if (vc)
        NSM_MPLS_GET_INDEX_PTR (PAL_FALSE, vc->vc_fib->ac_if_ix, ifp);

      ret = nsm_mpls_oam_reply_dsmap_data_set (&oam_resp->dsmap, 
                                         &local_stack, ifp);
      if (ret != NSM_SUCCESS)
        oam_resp->is_dsmap = PAL_FALSE;
    }
}
#endif /* HAVE_MPLS_VC */

#ifdef HAVE_VRF
/* Process OAM echo reply message for L3VPN data and fill Response data. */
void
nsm_mpls_oam_process_l3vpn_data (struct nsm_master *nm,
                                 struct nsm_msg_oam_lsp_ping_rep_process_l3vpn *msg,
                                 struct nsm_msg_oam_lsp_ping_rep_resp *oam_resp,
                                 bool_t is_dsmap)
{
  vrf_id_t vrf_id;
  struct listnode *ln;
  struct nsm_msg_vpn_rd *rd_node = NULL;
  struct mpls_label_stack local_stack;
  int ret;
  int stack_depth = 1;

  pal_mem_set(&local_stack, 0, sizeof (struct mpls_label_stack));
/* to be filled */
  /* Label stack not filled any where */

  LIST_LOOP (NSM_MPLS->mpls_vpn_list, rd_node, ln)
    {
      if (VPN_RD_SAME (&rd_node->rd, &msg->data.rd))
        {
          vrf_id = rd_node->vrf_id;
          ret = nsm_mpls_oam_route_ipv4_lookup (nm, vrf_id,
                                                &msg->data.vpn_pfx,
                                                msg->data.u.val.len);

          if (ret == NSM_SUCCESS)
            {
              oam_resp->return_code = MPLS_OAM_RTR_IS_EGRESS_AT_DEPTH;
              oam_resp->ret_subcode = stack_depth;
              break;
            }
        }
    }

  /* Outer Tunnel case */
  if (! rd_node)
    {
      if (msg->tunnel_type == MPLS_OAM_RSVP_IPv4_PREFIX)
        {
          /* Outer tunnel is RSVP */
          nsm_mpls_oam_process_rsvp_ipv4_data (nm, &msg->tunnel_data.rsvp,
                                               &msg->ctrl_data, oam_resp,
                                               NSM_FALSE, NSM_TRUE);
        }
      else if (msg->tunnel_type == MPLS_OAM_LDP_IPv4_PREFIX)
        {
          /* Outer Tunnel is LDP */
          nsm_mpls_oam_process_ldp_ipv4_data (nm, &msg->tunnel_data.ldp,
                                              &msg->ctrl_data, oam_resp,
                                              NSM_FALSE, NSM_TRUE);
        }
      else
        {
          /* No Outer Tunnel. Do global Route lookup */
          ret = nsm_mpls_oam_route_ipv4_lookup (nm, 0,
                                                &msg->data.vpn_pfx,
                                                msg->data.u.val.len);

          if (ret == NSM_SUCCESS)
            {
              oam_resp->return_code = MPLS_OAM_LBL_SWITCH_AT_DEPTH;
              oam_resp->ret_subcode = 0;
            }
        }
    }

/* Return code less than MPLS_OAM_ERRORED_TLV means error, 
 * DSMAP data is not processed in this case. 
 */
  if ((oam_resp->return_code > MPLS_OAM_ERRORED_TLV) && is_dsmap)
    {
      ret = nsm_mpls_oam_reply_dsmap_data_set (&oam_resp->dsmap,
                                               &local_stack, NULL);
      if (ret != NSM_SUCCESS)
        oam_resp->is_dsmap = PAL_FALSE;
    }
}
#endif /* HAVE_VRF */

/* Process recieved Dsmap data for echo request
 * and fill the Response Dsmap data for echo reply. 
 */
int
nsm_mpls_oam_reply_dsmap_data_set (struct mpls_oam_downstream_map *dsmap,
                                   struct mpls_label_stack *out_label,
                                   struct interface *ifp)
{
  if (! ifp)
    return NSM_FALSE;
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

      dsmap->ds_label = out_label->label;
   }

  return NSM_SUCCESS;
}
#endif /* HAVE_MPLS_OAM */
