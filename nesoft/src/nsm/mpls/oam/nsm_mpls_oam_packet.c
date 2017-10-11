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
#include "nsm_mpls_oam.h"
#include "nsm_mpls_oam_network.h"
#include "nsm_mpls_oam_packet.h"

#ifdef HAVE_VPLS
#include "nsm_vpls.h"
#endif /* HAVE_VPLS */
#include "nsm_router.h"

int
nsm_mpls_oam_decode_tlv_header (u_char **pnt, u_int16_t *size,
                                struct mpls_oam_tlv_hdr *header)
{
  if (*size < MPLS_OAM_TLV_HDR_FIX_LEN)
    return MPLS_OAM_ERR_PACKET_TOO_SMALL;

  DECODE_GETW (&header->type);
  DECODE_GETW (&header->length);

  header->type = pal_ntoh16 (header->type);
  header->length = pal_ntoh16 (header->length);

  return MPLS_OAM_TLV_HDR_FIX_LEN;
}

int
nsm_mpls_oam_ldp_ipv4_decode (u_char **pnt, u_int16_t *size,
                              struct ldp_ipv4_prefix *ldp,
                              struct mpls_oam_tlv_hdr *header)
{
  if (*size < MPLS_OAM_TLV_HDR_FIX_LEN)
    return MPLS_OAM_ERR_PACKET_TOO_SMALL;

  ldp->tlv_h = *header;

  DECODE_GETL (&ldp->ip_addr);
  DECODE_GETL (&ldp->u.l_packet);

  ldp->u.l_packet = pal_ntoh32 (ldp->u.l_packet);

  return (MPLS_OAM_LDP_IPv4_FIX_LEN + MPLS_OAM_PAD_TLV_3);
}

int
nsm_mpls_oam_rsvp_ipv4_decode (u_char **pnt, u_int16_t *size,
                               struct rsvp_ipv4_prefix *rsvp,
                               struct mpls_oam_tlv_hdr *header)
{

  if (*size < MPLS_OAM_TLV_HDR_FIX_LEN)
    return MPLS_OAM_ERR_PACKET_TOO_SMALL;

  rsvp->tlv_h = *header;

  DECODE_GETL (&rsvp->ip_addr);
  DECODE_GETW_EMPTY ();
  DECODE_GETW (&rsvp->tunnel_id);
  DECODE_GETL (&rsvp->ext_tunnel_id);
  DECODE_GETL (&rsvp->send_addr);
  DECODE_GETW_EMPTY ();
  DECODE_GETW (&rsvp->lsp_id);

  rsvp->tunnel_id = pal_ntoh16 (rsvp->tunnel_id);
  rsvp->lsp_id = pal_ntoh16 (rsvp->lsp_id);

  return MPLS_OAM_RSVP_IPv4_FIX_LEN;
}

int
nsm_mpls_oam_l3vpn_ipv4_decode (u_char **pnt, u_int16_t *size,
                                struct l3vpn_ipv4_prefix *l3vpn,
                                struct mpls_oam_tlv_hdr *header)
{
  int cnt = 0;
  if (*size < MPLS_OAM_TLV_HDR_FIX_LEN)
    return MPLS_OAM_ERR_PACKET_TOO_SMALL;

  l3vpn->tlv_h = *header;

  for (cnt = 0; cnt < VPN_RD_SIZE ; cnt++)
     DECODE_GETC (&l3vpn->rd.u.rd_val[cnt]);
  DECODE_GETL (&l3vpn->vpn_pfx);
  DECODE_GETL (&l3vpn->u.l_packet);

  return (MPLS_OAM_IPv4_VPN_FIX_LEN + MPLS_OAM_PAD_TLV_3);
}

int
nsm_mpls_oam_l2dep_ipv4_decode (u_char **pnt, u_int16_t *size,
                                struct l2_circuit_tlv_dep *l2_dep,
                                struct mpls_oam_tlv_hdr *header)
{
  if (*size < MPLS_OAM_TLV_HDR_FIX_LEN)
    return MPLS_OAM_ERR_PACKET_TOO_SMALL;

  l2_dep->tlv_h = *header;

  DECODE_GETL (&l2_dep->remote);
  DECODE_GETL (&l2_dep->vc_id);
  DECODE_GETW (&l2_dep->pw_type);

  DECODE_GETW_EMPTY ();

  return (MPLS_OAM_FEC128_DEP_FIX_LEN+MPLS_OAM_PAD_TLV_2);
}

int
nsm_mpls_oam_l2curr_ipv4_decode (u_char **pnt, u_int16_t *size,
                                 struct l2_circuit_tlv_curr *l2_curr,
                                 struct mpls_oam_tlv_hdr *header)
{
  if (*size < MPLS_OAM_TLV_HDR_FIX_LEN)
    return MPLS_OAM_ERR_PACKET_TOO_SMALL;

  l2_curr->tlv_h = *header;

  DECODE_GETL (&l2_curr->source);
  DECODE_GETL (&l2_curr->remote);
  DECODE_GETL (&l2_curr->vc_id);
  DECODE_GETW (&l2_curr->pw_type);
  DECODE_GETW_EMPTY ();

  return (MPLS_OAM_FEC128_CURR_FIX_LEN + MPLS_OAM_PAD_TLV_2);
}

int
nsm_mpls_oam_pw_ipv4_decode (u_char **pnt, u_int16_t *size,
                             struct mpls_pw_tlv *pw_tlv,
                             struct mpls_oam_tlv_hdr *header)
{
  int pad = 0;
  int data_len = 0;

  if (*size < MPLS_OAM_TLV_HDR_FIX_LEN)
    return MPLS_OAM_ERR_PACKET_TOO_SMALL;

  pw_tlv->tlv_h = *header;

  DECODE_GETL (&pw_tlv->source);
  DECODE_GETL (&pw_tlv->remote);
  DECODE_GETW (&pw_tlv->pw_type);
  DECODE_GETC (&pw_tlv->agi_type);
  DECODE_GETC (&pw_tlv->agi_length);
  data_len =  pw_tlv->agi_length;
  pad = GET_ENCODE_PAD_TLV_4 (data_len);
  if (pad != 0)
    DECODE_GET_EMPTY (pad);

  /* AGI value not used. Length assumed to be 0 */
  DECODE_GET_EMPTY (pw_tlv->agi_length);
  DECODE_GETC (&pw_tlv->saii_type);
  DECODE_GETC (&pw_tlv->saii_length);
  DECODE_GETW_EMPTY ();
  if (pw_tlv->saii_length > MPLS_OAM_PAD_TLV_2)
    {
      data_len  =  pw_tlv->saii_length-MPLS_OAM_PAD_TLV_2;
      DECODE_GET_EMPTY (data_len);
      if ((pad = GET_ENCODE_PAD_TLV_4(data_len))!= 0)
          DECODE_GET_EMPTY (pad);
    }

  /* SAI value not used. Length assumed to be 0 */
  DECODE_GETC (&pw_tlv->taii_type);
  DECODE_GETC (&pw_tlv->taii_length);
  DECODE_GETW_EMPTY ();
  if (pw_tlv->taii_length > MPLS_OAM_PAD_TLV_2)
    {
      data_len = pw_tlv->taii_length-MPLS_OAM_PAD_TLV_2;
      DECODE_GET_EMPTY (data_len);

      if ((pad = GET_ENCODE_PAD_TLV_4(data_len))!= 0)
          DECODE_GET_EMPTY (pad);
    }

  pad = GET_ENCODE_PAD_TLV_4(header->length);

  return (header->length + pad);
}

int
nsm_mpls_oam_gen_ipv4_decode (u_char **pnt, u_int16_t *size,
                              struct generic_ipv4_prefix *ipv4,
                              struct mpls_oam_tlv_hdr *header)
{
  if (*size < MPLS_OAM_TLV_HDR_FIX_LEN)
    return MPLS_OAM_ERR_PACKET_TOO_SMALL;

  ipv4->tlv_h = *header;

  DECODE_GETL (&ipv4->ipv4_addr);
  DECODE_GETL (&ipv4->u.l_packet);

  return (MPLS_OAM_GENERIC_IPv4_FIX_LEN + MPLS_OAM_PAD_TLV_3);
}

int
nsm_mpls_oam_decode_unknown_tlv (u_char **pnt, u_int16_t *size,
                                 struct mpls_oam_tlv_hdr *header)
{
  int len;

  len = header->length + GET_ENCODE_PAD_TLV_4 (header->length);

  if (*size < len)
    return MPLS_OAM_ERR_PACKET_TOO_SMALL;

  DECODE_GET_EMPTY (len);

  /* check vendor private ranges */
  if ((header->type <= MPLS_OAM_VENDOR_NUMBER2 &&
       header->type >= MPLS_OAM_VENDOR_NUMBER1) ||
      (header->type >= MPLS_OAM_VENDOR_NUMBER3))
    {
      return len;
    }

  /* Return an error, since the peer needs to be notified of this */
  return MPLS_OAM_ERR_UNKNOWN_TLV;
}

int
nsm_mpls_oam_decode_fec_tlv (u_char **pnt, u_int16_t *size,
                             struct mpls_oam_target_fec_stack *fec_tlv,
                             struct mpls_oam_tlv_hdr *tlv_h, int *err)
{
  struct mpls_oam_tlv_hdr header;
  int len = tlv_h->length;
  int ret;

  /* The FEC Stack can contain many FEC's. This is indicated by the TLV's
   * length field. We need to loop through the entire length to get all the
   * sub TLV's
   */
  fec_tlv->tlv_h = *tlv_h;

  while (len > 0)
    {

      ret = nsm_mpls_oam_decode_tlv_header (pnt, size, &header);
      if (ret < 0)
        return ret;

      len -= MPLS_OAM_TLV_HDR_FIX_LEN ;/* TLV Header length */

      switch (header.type)
        {
        case MPLS_OAM_LDP_IPv4_PREFIX_TLV:
          ret = nsm_mpls_oam_ldp_ipv4_decode (pnt, size, &fec_tlv->ldp,
                                              &header);
          if (ret < 0)
            return ret;

          len -=ret;

          SET_FLAG (fec_tlv->flags, MPLS_OAM_LDP_IPv4_PREFIX);
          break;

        case MPLS_OAM_RSVP_IPv4_PREFIX_TLV:
          ret = nsm_mpls_oam_rsvp_ipv4_decode (pnt, size, &fec_tlv->rsvp,
                                               &header);

          if (ret < 0)
            return ret;

          len-=ret;

          SET_FLAG (fec_tlv->flags, MPLS_OAM_RSVP_IPv4_PREFIX);
          break;

        case MPLS_OAM_IPv4_VPN_TLV:
          ret = nsm_mpls_oam_l3vpn_ipv4_decode (pnt, size, &fec_tlv->l3vpn,
                                                &header);
          if (ret < 0)
            return ret;

          len-=ret;

          SET_FLAG (fec_tlv->flags, MPLS_OAM_MPLS_VPN_PREFIX);
          break;

        case MPLS_OAM_FEC128_DEP_TLV :
          ret = nsm_mpls_oam_l2dep_ipv4_decode (pnt, size, &fec_tlv->l2_dep,
                                                &header);
          if (ret < 0)
            return ret;
          len-=ret;
          SET_FLAG (fec_tlv->flags, MPLS_OAM_L2_CIRCUIT_PREFIX_DEP);
          break;

        case MPLS_OAM_FEC128_CURR_TLV :
          ret = nsm_mpls_oam_l2curr_ipv4_decode (pnt, size, &fec_tlv->l2_curr,
                                                 &header);
          if (ret < 0)
            return ret;
          len-=ret;
          SET_FLAG (fec_tlv->flags, MPLS_OAM_L2_CIRCUIT_PREFIX_CURR);
          break;

        case MPLS_OAM_FEC129_PW_TLV:
          ret = nsm_mpls_oam_pw_ipv4_decode (pnt, size, &fec_tlv->pw_tlv,
                                             &header);
          if (ret < 0)
            return ret;
          len-=ret;
          SET_FLAG (fec_tlv->flags, MPLS_OAM_GEN_PW_ID_PREFIX);
          break;

        case MPLS_OAM_GENERIC_FEC_TLV:
          ret = nsm_mpls_oam_gen_ipv4_decode (pnt, size, &fec_tlv->ipv4,
                                              &header);
          if (ret < 0)
            return ret;
          len-=ret;
          SET_FLAG (fec_tlv->flags, MPLS_OAM_GENERIC_IPv4_PREFIX);
          break;

        case MPLS_OAM_NIL_FEC_TLV:
          DECODE_GETL (&fec_tlv->nil_fec.u.l_packet);
          SET_FLAG (fec_tlv->flags, MPLS_OAM_NIL_FEC_PREFIX);
          len-=MPLS_OAM_NIL_FEC_FIX_LEN;
          break;

        default:
          ret = nsm_mpls_oam_decode_unknown_tlv (pnt, size, &header);
          if (ret < 0)
            {
              if (header.type < MPLS_OAM_MAX_TLV_VALUE)
                *err = MPLS_OAM_ERRORED_TLV;

              return ret;
            }

          len-= ret;
          break;
        }
    }

  return MPLS_OAM_FEC_TLV_MIN_LEN;
}

int
nsm_mpls_oam_decode_dsmap_tlv (u_char **pnt, u_int16_t *size,
                               struct mpls_oam_downstream_map *dsmap,
                               struct mpls_oam_tlv_hdr *tlv_h,
                               u_char msg_type)
{
  int pad;
  int len = 0;
  struct shimhdr *shim;
  int cnt = 0;

  if (*size < MPLS_OAM_DSMAP_TLV_IPv4_FIX_LEN)
    return MPLS_OAM_ERR_PACKET_TOO_SMALL;

  dsmap->tlv_h = *tlv_h;
  /* Decode all values */
  DECODE_GETL (&dsmap->u.l_packet);
  dsmap->u.l_packet = pal_ntoh32 (dsmap->u.l_packet);
  len+=4;
  if ((dsmap->u.data.family == MPLS_OAM_IPv4_NUMBERED) ||
      (dsmap->u.data.family == MPLS_OAM_IPv4_UNNUMBERED))
    {
      DECODE_GETL (&dsmap->ds_ip);
      DECODE_GETL (&dsmap->ds_if_ip);
      len+= DSMAP_IPV4_NUMBERED_PAD;
    }
  else if (dsmap->u.data.family == MPLS_OAM_IPv6_NUMBERED)
    {
      DECODE_GET_EMPTY (DSMAP_IPV6_NUMBERED_PAD);
      len += DSMAP_IPV6_NUMBERED_PAD;
    }
  else if (dsmap->u.data.family == MPLS_OAM_IPv6_UNNUMBERED)
    {
      DECODE_GET_EMPTY (DSMAP_IPV6_UNNUMBERED_PAD);
      len += DSMAP_IPV6_UNNUMBERED_PAD;
    }

  DECODE_GETC (&dsmap->multipath_type);
  DECODE_GETC (&dsmap->depth);
  DECODE_GETW (&dsmap->multipath_len);
  len+=4;
  if (dsmap->multipath_len > 0 )
    {
      dsmap->multi_info = XMALLOC (MTYPE_TMP, dsmap->multipath_len);
      DECODE_GET (&dsmap->multi_info, dsmap->multipath_len);
      len+=dsmap->multipath_len;
      if ((pad = GET_ENCODE_PAD_TLV_4(dsmap->multipath_len))!= 0)
        DECODE_GET_EMPTY (pad);

      len+=pad;
    }
  cnt = tlv_h->length - len;
  dsmap->ds_label = NULL;
  if (cnt >= DSMAP_LABEL_DATA_LEN)
    {
      dsmap->ds_label = list_new ();
      cnt = cnt/4; /* get the no. of dsmap label; data */
      for (;cnt > 0; cnt--)
        {
          shim = XMALLOC (MTYPE_TMP, sizeof (struct shimhdr));
          DECODE_GETL (&shim->shim);
	  shim->shim = pal_ntoh32 (shim->shim);
          listnode_add (dsmap->ds_label, shim);
          len+=4;
        }
    }

  len += MPLS_OAM_TLV_HDR_FIX_LEN;
  return len;

}

int
nsm_mpls_oam_decode_if_lbl_stack_tlv (u_char **pnt, u_int16_t *size,
                                      struct mpls_oam_if_lbl_stk *if_label,
                                      struct mpls_oam_tlv_hdr *tlv_h)
{
  int i;
  int len = MPLS_OAM_TLV_HDR_FIX_LEN;
  int cnt = 0;

  if_label->tlv_h = *tlv_h;
  /* Encode all values and increment length on the fly */
  DECODE_GETL (&if_label->u.l_packet);
  len+=4;
  if ((if_label->u.fam.family == MPLS_OAM_IPv4_NUMBERED) ||
      (if_label->u.fam.family == MPLS_OAM_IPv4_UNNUMBERED))
    {
      DECODE_GETL (&if_label->addr);
      DECODE_GETL (&if_label->ifindex);
      len+= IFLABEL_IPV4_NUMBERED_PAD;
    }
  else if (if_label->u.fam.family == MPLS_OAM_IPv6_NUMBERED)
    {
      DECODE_GET_EMPTY (IFLABEL_IPV6_NUMBERED_PAD);
      len += IFLABEL_IPV6_NUMBERED_PAD;
    }
  else if (if_label->u.fam.family == MPLS_OAM_IPv6_UNNUMBERED)
    {
      DECODE_GET_EMPTY (IFLABEL_IPV6_UNNUMBERED_PAD);
      len += IFLABEL_IPV6_UNNUMBERED_PAD;
    }

  cnt = tlv_h->length - len;
  cnt=cnt/4; /* set the word size */
  for (i = 0; i < cnt ; i++)
    {
      DECODE_GETL (&if_label->out_stack[i]);
      if_label->out_stack[i]= pal_ntoh32(if_label->out_stack[i]);
      len+=4;
    }
  return len;
}

int
nsm_mpls_oam_decode_errored_tlv (u_char **pnt, u_int16_t *size,
                                 struct mpls_oam_errored_tlv *err_tlv,
                                 struct mpls_oam_tlv_hdr *tlv_h)
{
  int len = 0;
  int pad  = 0;

  err_tlv->buf = XMALLOC (MTYPE_TMP, tlv_h->length);
  for (len = 0; len < tlv_h->length; len++)
    DECODE_GETC((err_tlv->buf+len));
  pad = GET_ENCODE_PAD_TLV_4 (tlv_h->length);
  if (pad != 0)
    DECODE_GET_EMPTY (pad);
  return len;
}

int
nsm_mpls_oam_decode_pad_tlv (u_char **pnt, u_int16_t *size,
                             struct mpls_oam_pad_tlv *pad_tlv,
                             struct mpls_oam_tlv_hdr *tlv_h)
{
  int pad  = 0;

  pad_tlv->tlv_h = *tlv_h;

  DECODE_GETC(&pad_tlv->val);
  pad = tlv_h->length - 1;
  DECODE_GET_EMPTY (pad);

  pad = GET_ENCODE_PAD_TLV_4 (tlv_h->length);
  if (pad != 0)
    DECODE_GET_EMPTY (pad);

  return tlv_h->length;
}

int
nsm_mpls_oam_decode_rtos_tlv (u_char **pnt, u_int16_t *size,
                              struct mpls_oam_reply_tos *reply_tos,
                              struct mpls_oam_tlv_hdr *tlv_h)
{

  if (*size < MPLS_OAM_REPLY_TOS_TLV_FIX_LEN)
    return MPLS_OAM_ERR_PACKET_TOO_SMALL;

  reply_tos->tlv_h = *tlv_h;

  DECODE_GETL (reply_tos->u.l_packet);
  reply_tos->u.l_packet = pal_ntoh32 (reply_tos->u.l_packet);

  return MPLS_OAM_REPLY_TOS_TLV_FIX_LEN;
}

int
nsm_mpls_oam_header_decode (u_char **pnt, u_int16_t *size,
                            struct mpls_oam_hdr *omh)
{
  if (*size < MPLS_OAM_HDR_LEN)
    return MPLS_OAM_ERR_PACKET_TOO_SMALL;

  /* Decode entire header */
  DECODE_GETW (&omh->ver_no);
  DECODE_GETW (&omh->u.l_packet);
  DECODE_GETC (&omh->msg_type);
  DECODE_GETC (&omh->reply_mode);
  DECODE_GETC (&omh->return_code);
  DECODE_GETC (&omh->ret_subcode);
  DECODE_GETL (&omh->sender_handle);
  DECODE_GETL (&omh->seq_number);
  DECODE_GETL (&omh->ts_tx_sec);
  DECODE_GETL (&omh->ts_tx_usec);
  DECODE_GETL (&omh->ts_rx_sec);
  DECODE_GETL (&omh->ts_rx_usec);

  omh->ver_no = pal_ntoh16 (omh->ver_no);
  omh->u.l_packet = pal_ntoh16 (omh->u.l_packet);
  /* Single Byte values need not be converted to Network order */
  omh->sender_handle = pal_ntoh32 (omh->sender_handle);
  omh->seq_number = pal_ntoh32 (omh->seq_number);
  omh->ts_tx_sec = pal_ntoh32 (omh->ts_tx_sec);
  omh->ts_tx_usec = pal_ntoh32 (omh->ts_tx_usec);
  omh->ts_rx_sec = pal_ntoh32 (omh->ts_rx_sec);
  omh->ts_rx_usec = pal_ntoh32 (omh->ts_rx_usec);

  return MPLS_OAM_HDR_LEN;
}

int
nsm_mpls_oam_decode_echo_request (u_char **pnt, u_int16_t *size,
                                  struct mpls_oam_hdr *omh,
                                  struct mpls_oam_echo_request *oam_req,
                                  int *err_code)
{
  /* We have a fixed number of octets in pnt. The Request message can
   * contain multiple TLV's and sub TLV's. Need to loop till all decoded
   */
  struct mpls_oam_tlv_hdr tlv_h;
  u_char *spnt = *pnt;
  int len = 0;
  int ret = 0;
  u_char fec_tlv_found = NSM_FALSE;

  /* set fec element header */
  pal_mem_cpy (&oam_req->omh, omh, sizeof (struct mpls_oam_hdr));
  /* Set default error code */
  *err_code = MPLS_OAM_MALFORMED_REQUEST;

  len = *size;

  /* Based on the length we can now find if there are mode TLV's to be
   * decoded
   */

  while (*size > 0)
    {
      pal_mem_set (&tlv_h, 0, sizeof (struct mpls_oam_tlv_hdr)); 
      ret = nsm_mpls_oam_decode_tlv_header (pnt, size, &tlv_h);
      if (ret < 0)
        return ret;
      switch (tlv_h.type)
        {
        case MPLS_OAM_FEC_TLV:
          ret = nsm_mpls_oam_decode_fec_tlv (pnt, size, &oam_req->fec_tlv, &tlv_h,
                                             err_code);
          if (ret < 0)
            return ret;

          fec_tlv_found = NSM_TRUE;
          break;

        case MPLS_OAM_DSMAP_TLV:
          ret = nsm_mpls_oam_decode_dsmap_tlv (pnt, size, &oam_req->dsmap,
                                               &tlv_h, omh->msg_type);
          if (ret < 0)
            return ret;

          SET_FLAG (oam_req->flags, MPLS_OAM_FLAGS_DSMAP_TLV);
          break;

        case MPLS_OAM_REPLY_TOS_TLV:
          ret = nsm_mpls_oam_decode_rtos_tlv (pnt, size, &oam_req->tos_reply,
                                              &tlv_h);
          if (ret < 0)
            return ret;

          SET_FLAG (oam_req->flags, MPLS_OAM_FLAGS_REPLY_TOS_TLV);
          break;
        case MPLS_OAM_PAD_TLV:
          ret = nsm_mpls_oam_decode_pad_tlv (pnt, size, &oam_req->pad_tlv,
                                             &tlv_h);
          if (ret < 0)
            return ret;

	  if (oam_req->pad_tlv.val == MPLS_OAM_PAD_VAL_COPY)
	    SET_FLAG (oam_req->flags, MPLS_OAM_FLAGS_PAD_TLV);
          break;
        default:
          ret = nsm_mpls_oam_decode_unknown_tlv (pnt, size, &tlv_h);
          if (ret < 0)
	    {
	      if (ret == MPLS_OAM_ERR_UNKNOWN_TLV)
		{
		  if (tlv_h.type < MPLS_OAM_MAX_TLV_VALUE)
		    {
		      *err_code = MPLS_OAM_ERRORED_TLV;
		      return ret;
		    }
		}
	      else
		return ret;
	    }

          break;
        }
    }

  if(! fec_tlv_found)
    {
       *err_code = MPLS_OAM_MALFORMED_REQUEST;
       return NSM_FAILURE;
    } 

  /* Check packet length. */
  if ((*pnt - spnt) != len)
    return MPLS_OAM_ERR_DECODE_LENGTH_ERROR;

  /* Reset error code */
  *err_code = 0;
  return 0;
}

int
nsm_mpls_oam_decode_echo_reply (u_char **pnt, u_int16_t *size,
                                struct mpls_oam_hdr *omh,
                                struct mpls_oam_echo_reply *oam_reply,
                                u_char *reply)
{
  /* We have a fixed number of octets in pnt. The Request message can
   * contain multiple TLV's and sub TLV's. Need to loop till all decoded
   */
  struct mpls_oam_tlv_hdr tlv_h;
  u_char *spnt = *pnt;
  int len = 0;
  int ret;
  int err_code = 0;

  /* set fec element header */
  pal_mem_cpy (&oam_reply->omh, omh, sizeof (struct mpls_oam_hdr));

  len = *size;

  /* Based on the length we can now find if there are mode TLV's to be
   * decoded
   */

  while (*size > 0)
    {
      ret = nsm_mpls_oam_decode_tlv_header (pnt, size, &tlv_h);
      if (ret < 0)
        return ret;

      switch (tlv_h.type)
        {
        case MPLS_OAM_FEC_TLV:
          ret = nsm_mpls_oam_decode_fec_tlv (pnt, size, &oam_reply->fec_tlv,
                                     &tlv_h, &err_code);
          if (ret < 0)
            return ret;
          break;
        case MPLS_OAM_DSMAP_TLV:
          ret = nsm_mpls_oam_decode_dsmap_tlv (pnt, size, &oam_reply->dsmap,
                                               &tlv_h, omh->msg_type);
          if (ret < 0)
            return ret;
          *reply = NSM_TRUE;
          break;

        case MPLS_OAM_IF_LBL_STACK:
          ret = nsm_mpls_oam_decode_if_lbl_stack_tlv (pnt, size,
                                                      &oam_reply->if_lbl_stk, &tlv_h);
          if (ret <0)
            return ret;
          break;

        default:
          ret = nsm_mpls_oam_decode_unknown_tlv (pnt, size, &tlv_h);
          if (ret < 0)
            return ret;
          /* Need to add the IF and Label Stack TLV for the Transit cases */
        }
    }

  /* Check packet length. */
  if ((*pnt - spnt) != len)
    return MPLS_OAM_ERR_DECODE_LENGTH_ERROR;

  return 0;
}

int
mpls_oam_tlv_header_encode (u_char **pnt, u_int16_t *size,
                            struct mpls_oam_tlv_hdr *tlv_h)
{
  u_int16_t type;
  u_int16_t length;

  if (*size < MPLS_OAM_TLV_HDR_FIX_LEN)
    return MPLS_OAM_ERR_PACKET_TOO_SMALL;

  type = pal_hton16 (tlv_h->type);
  ENCODE_PUTW (&type);

  length = pal_hton16 (tlv_h->length);
  ENCODE_PUTW (&length);

  return MPLS_OAM_TLV_HDR_FIX_LEN;
}

int
nsm_mpls_oam_encode_oam_header (u_char **pnt, u_int16_t *size,
                                struct mpls_oam_hdr *omh,
                                u_char type)
{

 struct mpls_oam_hdr hdr;
 if (*size < MPLS_OAM_HDR_LEN)
   return MPLS_OAM_ERR_PACKET_TOO_SMALL;

  hdr.ver_no = pal_hton16 (omh->ver_no);
  hdr.u.l_packet = pal_hton16 (omh->u.l_packet);
  hdr.msg_type = omh->msg_type;
  hdr.reply_mode = omh->reply_mode;
  hdr.return_code = omh->return_code;
  hdr.ret_subcode = omh->ret_subcode;
  /* Single Byte values need not be converted to Network order */
  hdr.sender_handle = pal_hton32 (omh->sender_handle);
  hdr.seq_number = pal_hton32 (omh->seq_number);
  hdr.ts_tx_sec = pal_hton32 (omh->ts_tx_sec);
  hdr.ts_tx_usec = pal_hton32 (omh->ts_tx_usec);
  hdr.ts_rx_sec = pal_hton32 (omh->ts_rx_sec);
  hdr.ts_rx_usec = pal_hton32 (omh->ts_rx_usec);

  /* Encode entire header */
  ENCODE_PUTW (&hdr.ver_no);
  ENCODE_PUTW (&hdr.u.l_packet);
  ENCODE_PUTC (&hdr.msg_type);
  ENCODE_PUTC (&hdr.reply_mode);
  ENCODE_PUTC (&hdr.return_code);
  ENCODE_PUTC (&hdr.ret_subcode);
  ENCODE_PUTL (&hdr.sender_handle);
  ENCODE_PUTL (&hdr.seq_number);
  ENCODE_PUTL (&hdr.ts_tx_sec);
  ENCODE_PUTL (&hdr.ts_tx_usec);
  ENCODE_PUTL (&hdr.ts_rx_sec);
  ENCODE_PUTL (&hdr.ts_rx_usec);

  return MPLS_OAM_HDR_LEN;
}

int
nsm_mpls_oam_encode_ldp_ipv4_tlv (u_char **pnt, u_int16_t *size,
                                  struct ldp_ipv4_prefix *ldp)
{
  int len = 0;
  u_int32_t l_data;

  mpls_oam_tlv_header_encode (pnt, size, &ldp->tlv_h);
  ENCODE_PUTL (&ldp->ip_addr);
  l_data = pal_hton32(ldp->u.l_packet);
  ENCODE_PUTL (&l_data);

  len+=MPLS_OAM_LDP_IPv4_FIX_LEN;
  return len;
}

int
nsm_mpls_oam_encode_rsvp_ipv4_tlv (u_char **pnt, u_int16_t *size,
                                   struct rsvp_ipv4_prefix *rsvp)
{
  int len = 0;
  u_int16_t tunnel_id;
  u_int16_t lsp_id;

  mpls_oam_tlv_header_encode (pnt, size, &rsvp->tlv_h);
  ENCODE_PUTL (&rsvp->ip_addr);
  ENCODE_PUTW_EMPTY ();
  tunnel_id = pal_hton16 (rsvp->tunnel_id);
  ENCODE_PUTW (&tunnel_id);
  ENCODE_PUTL (&rsvp->ext_tunnel_id);
  ENCODE_PUTL (&rsvp->send_addr);
  ENCODE_PUTW_EMPTY ();
  lsp_id = pal_hton16 (rsvp->lsp_id);
  ENCODE_PUTW (&lsp_id);

  len+=MPLS_OAM_RSVP_IPv4_FIX_LEN;
  return len;
}

int
nsm_mpls_oam_encode_l3vpn_ipv4_tlv (u_char **pnt, u_int16_t *size,
                                    struct l3vpn_ipv4_prefix *l3vpn)
{
  int len = 0;
  int cnt = 0;

  mpls_oam_tlv_header_encode (pnt, size, &l3vpn->tlv_h);
  for (cnt = 0; cnt < VPN_RD_SIZE; cnt++)
     ENCODE_PUTC (&l3vpn->rd.u.rd_val[cnt]);

  ENCODE_PUTL (&l3vpn->vpn_pfx);
  ENCODE_PUTL (&l3vpn->u.l_packet);

  len+=MPLS_OAM_IPv4_VPN_FIX_LEN;
  return len;
}

int
nsm_mpls_oam_encode_l2_dep_tlv (u_char **pnt, u_int16_t *size,
                                struct l2_circuit_tlv_dep *l2_dep)
{
  int len = 0;

  mpls_oam_tlv_header_encode (pnt, size, &l2_dep->tlv_h);

  ENCODE_PUTL (&l2_dep->remote);
  ENCODE_PUTL (&l2_dep->vc_id);
  ENCODE_PUTW (&l2_dep->pw_type);
  ENCODE_PUTW_EMPTY ();

  len+=MPLS_OAM_FEC128_DEP_FIX_LEN;
  return len;
}

int
nsm_mpls_oam_encode_l2_curr_tlv (u_char **pnt, u_int16_t *size,
                                 struct l2_circuit_tlv_curr *l2_curr)
{
  int len = 0;

  mpls_oam_tlv_header_encode (pnt, size, &l2_curr->tlv_h);
  ENCODE_PUTL (&l2_curr->source);
  ENCODE_PUTL (&l2_curr->remote);
  ENCODE_PUTL (&l2_curr->vc_id);
  ENCODE_PUTW (&l2_curr->pw_type);
  ENCODE_PUTW_EMPTY ();

  len+=MPLS_OAM_FEC128_CURR_FIX_LEN;
  return len;
}

int
nsm_mpls_oam_encode_pw_tlv (u_char **pnt, u_int16_t *size,
                            struct mpls_pw_tlv *pw_tlv)
{
  int len = 0;

  mpls_oam_tlv_header_encode (pnt, size, &pw_tlv->tlv_h);
  ENCODE_PUTL (&pw_tlv->source);
  ENCODE_PUTL (&pw_tlv->remote);
  ENCODE_PUTW (&pw_tlv->pw_type);
  ENCODE_PUTC (&pw_tlv->agi_type);
  ENCODE_PUTC (&pw_tlv->agi_length);
  /* AGI value not used. Length assumed to be 0 */
  ENCODE_PUTC (&pw_tlv->saii_type);
  ENCODE_PUTC (&pw_tlv->saii_length);
  ENCODE_PUT_EMPTY (MPLS_OAM_PAD_TLV_2);
  /* SAI value not used. Length assumed to be 0 */
  ENCODE_PUTC (&pw_tlv->taii_type);
  ENCODE_PUTC (&pw_tlv->taii_length);
  ENCODE_PUTW_EMPTY ();
  /* TAI value not used. Length assumed to be 0 */

  len+=MPLS_OAM_FEC129_PW_MIN_LEN;
  return len;
}

int
nsm_mpls_oam_encode_gen_ipv4_tlv (u_char **pnt, u_int16_t *size,
                                  struct generic_ipv4_prefix *ipv4)
{
  int len = 0;

  mpls_oam_tlv_header_encode (pnt, size, &ipv4->tlv_h);
  ENCODE_PUTL (&ipv4->ipv4_addr);
  ENCODE_PUTL (&ipv4->u.l_packet);

  len+=MPLS_OAM_GENERIC_IPv4_FIX_LEN;
  return len;
}

int
nsm_mpls_oam_encode_fec_tlv (u_char **pnt, u_int16_t *size,
                             struct mpls_oam_target_fec_stack *fec_tlv_orig,
                             u_char msg_type)
{
  int ret;
  int len = 0;
  int i = 0;
  u_int16_t flags = 1;
  struct mpls_oam_target_fec_stack fec_tlv;

  pal_mem_set (&fec_tlv, 0 , sizeof (struct mpls_oam_target_fec_stack));
  pal_mem_cpy (&fec_tlv, fec_tlv_orig,
               sizeof (struct mpls_oam_target_fec_stack));

  ret = mpls_oam_tlv_header_encode (pnt, size, &fec_tlv.tlv_h);
  if (ret < 0)
    return ret;

  if (*size < MPLS_OAM_FEC_TLV_MIN_LEN )
    return MPLS_OAM_ERR_PACKET_TOO_SMALL;

  len += ret;
  for (i = 0; i < FEC_FLAGS_SIZE; i++)
     {
       flags = (1 << i);
       if (CHECK_FLAG (fec_tlv.flags, flags))
         {
           switch (flags)
             {
               case MPLS_OAM_LDP_IPv4_PREFIX:
                  ret = nsm_mpls_oam_encode_ldp_ipv4_tlv (pnt, size,
                                                          &fec_tlv.ldp);

                  len+=ret;
                  break;

               case MPLS_OAM_RSVP_IPv4_PREFIX:
                  ret = nsm_mpls_oam_encode_rsvp_ipv4_tlv (pnt, size,
                                                            &fec_tlv.rsvp);

                  len+=ret;
                  break;

               case MPLS_OAM_MPLS_VPN_PREFIX:
                  ret = nsm_mpls_oam_encode_l3vpn_ipv4_tlv (pnt, size,
                                                              &fec_tlv.l3vpn);

                  len+=ret;
                  break;

               case MPLS_OAM_L2_CIRCUIT_PREFIX_DEP:
                  ret = nsm_mpls_oam_encode_l2_dep_tlv (pnt, size,
                                                         &fec_tlv.l2_dep);

                  len+=ret;
                  break;

               case MPLS_OAM_L2_CIRCUIT_PREFIX_CURR:
                  ret = nsm_mpls_oam_encode_l2_curr_tlv (pnt, size,
                                                          &fec_tlv.l2_curr);

                  len+=ret;
                  break;

               case MPLS_OAM_GEN_PW_ID_PREFIX:
                  ret = nsm_mpls_oam_encode_pw_tlv (pnt, size,
                                                     &fec_tlv.pw_tlv);

                  len+=ret;
                  break;

               case MPLS_OAM_GENERIC_IPv4_PREFIX:
                  ret = nsm_mpls_oam_encode_gen_ipv4_tlv (pnt, size,
                                                           &fec_tlv.ipv4);

                  len+=ret;
                  break;

               case MPLS_OAM_NIL_FEC_PREFIX:
                  mpls_oam_tlv_header_encode (pnt, size,
                                              &fec_tlv.nil_fec.tlv_h);
                  ENCODE_PUTL (&fec_tlv.nil_fec.u.l_packet);
                  break;

               default:
                  break;
             }
         }
     }
  return len;
}

int
nsm_mpls_oam_encode_pad_tlv (u_char **pnt, u_int16_t *size,
                             struct mpls_oam_pad_tlv *pad_tlv,
                             u_char msg_type)
{
  int pad  = 0;

  ENCODE_PUTC(&pad_tlv->val);
  pad = pad_tlv->tlv_h.length -1;
  pad = GET_ENCODE_PAD_TLV_4 (pad);
  if (pad != 0)
    ENCODE_PUT_EMPTY (pad);

  return pad_tlv->tlv_h.length;
}


int
nsm_mpls_oam_encode_dsmap_tlv (u_char **pnt, u_int16_t *size,
                               struct mpls_oam_downstream_map *dsmap_orig,
                               u_char msg_type)
{
  int ret;
  int len = 0;
  int pad = 0;
  struct listnode *node;
  struct shimhdr *shim;
  struct mpls_oam_downstream_map *dsmap = NULL;

  dsmap = XMALLOC (MTYPE_TMP, sizeof (struct mpls_oam_downstream_map));
  if (! dsmap)
    return NSM_ERR_MEM_ALLOC_FAILURE;

  pal_mem_cpy (dsmap, dsmap_orig, sizeof (struct mpls_oam_downstream_map));

  ret = mpls_oam_tlv_header_encode (pnt, size, &dsmap->tlv_h);
  if (ret < 0)
    {
      XFREE (MTYPE_TMP, dsmap);
      return ret;
    }

  if (*size < MPLS_OAM_DSMAP_TLV_IPv4_FIX_LEN)
    {
      XFREE (MTYPE_TMP, dsmap);
      return MPLS_OAM_ERR_PACKET_TOO_SMALL;
    }

  len += ret;

  /* Encode all values and increment length on the fly */
  dsmap->u.l_packet = pal_hton32 (dsmap->u.l_packet);
  ENCODE_PUTL (&dsmap->u.l_packet);
  ENCODE_PUTL (&dsmap->ds_ip);
  ENCODE_PUTL (&dsmap->ds_if_ip);
  ENCODE_PUTC (&dsmap->multipath_type);
  ENCODE_PUTC (&dsmap->depth);
  ENCODE_PUTW (&dsmap->multipath_len);
  if (dsmap->multipath_len > 0 )
    {
      ENCODE_PUT (&dsmap->multi_info, dsmap->multipath_len);
      len+=dsmap->multipath_len;
      if ((pad = GET_ENCODE_PAD_TLV_4(dsmap->multipath_len))!= 0)
        ENCODE_PUT_EMPTY (pad);
      len+=pad;
    }
  if (msg_type || dsmap->ds_label)
  {
    u_int32_t dslabel_data;
    /* Encode the Downstream label list */
    LIST_LOOP (dsmap->ds_label, shim, node)
    {
      dslabel_data = pal_hton32 (shim->shim);
      ENCODE_PUTL (&dslabel_data);
      len+= 4;
    }
  }
  len+=MPLS_OAM_DSMAP_TLV_IPv4_FIX_LEN;

  XFREE (MTYPE_TMP, dsmap);
  return len;
}

int
nsm_mpls_oam_encode_if_lbl_stack_tlv (u_char **pnt, u_int16_t *size,
                                      struct mpls_oam_if_lbl_stk *if_label,
                                      u_char msg_type)
{
  int i,ret;
  int len = 0;

  ret = mpls_oam_tlv_header_encode (pnt, size, &if_label->tlv_h);
  if (ret < 0)
    return ret;

  if (*size < MPLS_OAM_IFLABEL_TLV_IPv4_FIX_LEN)
    return MPLS_OAM_ERR_PACKET_TOO_SMALL;

  len += ret;

  /* Encode all values and increment length on the fly */
  ENCODE_PUTL (&if_label->u.l_packet);
  ENCODE_PUTL (&if_label->addr);
  ENCODE_PUTL (&if_label->ifindex);
  len+=MPLS_OAM_IFLABEL_TLV_IPv4_FIX_LEN;

  for (i = 0; i < if_label->depth; i++)
     {
      if_label->out_stack[i] = pal_hton32(if_label->out_stack[i]);
      ENCODE_PUTL (&if_label->out_stack[i]);
      len+=4;
    }

  return len;
}

int
nsm_mpls_oam_encode_errored_tlv (u_char **pnt, u_int16_t *size,
                                 struct mpls_oam_errored_tlv *err_tlv,
                                 u_char msg_type)
{
  int ret;
  int len = 0;
  int cnt = 0;
  int pad  = 0;

  ret = mpls_oam_tlv_header_encode (pnt, size, &err_tlv->tlv_h);
  if (ret < 0)
    return ret;

  len+= ret;

  /* Encode all values and increment length on the fly */
  for (cnt = 0; cnt < err_tlv->tlv_h.length; cnt++)
    ENCODE_PUTC((err_tlv->buf+cnt));
  pad = GET_ENCODE_PAD_TLV_4 (err_tlv->tlv_h.length);
  if (pad != 0)
    ENCODE_PUT_EMPTY (pad);
  return len;
}

int
nsm_mpls_oam_encode_echo_request (u_char **pnt, u_int16_t *size,
                                  struct mpls_oam_echo_request *req,
				  bool_t trace)
{
  int ret;
  u_char *spnt = *pnt;

  /* Encode the OAM TLV's */
  ret = nsm_mpls_oam_encode_oam_header (pnt, size, &req->omh, MPLS_ECHO);
  if (ret < 0)
    return ret;

  /* Encode the FEC TLV */
  ret = nsm_mpls_oam_encode_fec_tlv (pnt, size, &req->fec_tlv, MPLS_ECHO);
  if (ret < 0)
    return ret;

  if (oam_req_dsmap_tlv == NSM_TRUE)
    {
      ret = nsm_mpls_oam_encode_dsmap_tlv (pnt, size, &req->dsmap, MPLS_ECHO);
      if (ret < 0)
	return ret;
    }

  /* Other TLV's can be added if supported */

  return *pnt - spnt;

}

int
nsm_mpls_oam_encode_echo_reply (u_char **pnt, u_int16_t *size,
                                struct mpls_oam_echo_reply *reply)
{
  int ret;
  u_char *spnt = *pnt;

  /* Encode the OAM TLV's */
  ret = nsm_mpls_oam_encode_oam_header (pnt, size, &reply->omh,
					MPLS_ECHO_REPLY_MESSAGE);

  if (ret < 0)
    return ret;

  /* Encode the FEC TLV */
  if (oam_reply_tlv == NSM_TRUE)
    {
      ret = nsm_mpls_oam_encode_fec_tlv (pnt, size, &reply->fec_tlv,
					 MPLS_ECHO_REPLY_MESSAGE);
      if (ret < 0)
	return ret;
    }

  if (CHECK_FLAG (reply->flags, MPLS_OAM_FLAGS_DSMAP_TLV))
    {
      ret = nsm_mpls_oam_encode_dsmap_tlv (pnt, size, &reply->dsmap,
					   MPLS_ECHO_REPLY_MESSAGE);
      if (ret < 0)
	return ret;
    }

  if (CHECK_FLAG (reply->flags, MPLS_OAM_FLAGS_IF_LBL_STK_TLV))
    {
      ret = nsm_mpls_oam_encode_if_lbl_stack_tlv (pnt, size,
						  &reply->if_lbl_stk,
						  MPLS_ECHO_REPLY_MESSAGE);
      if (ret < 0)
	return ret;
    }

  if (CHECK_FLAG (reply->flags, MPLS_OAM_FLAGS_PAD_TLV))
    {
      ret = nsm_mpls_oam_encode_pad_tlv (pnt, size, &reply->pad_tlv,
					 MPLS_ECHO_REPLY_MESSAGE);
      if (ret < 0)
	return ret;
    }

  /* Other TLV's can be added when supported */
  return *pnt - spnt;
}

#ifdef HAVE_MPLS_OAM_ITUT
int
nsm_mpls_oam_encode_itut_payload (u_char **pnt, u_int16_t *size, 
                                  struct nsm_mpls_oam_itut *req)
{
  struct nsm_mpls_oam_itut oam_payload;
  
  /* Encode the OAM  */
  oam_payload.function_type = req->function_type;
  oam_payload.defect_type = pal_hton16(req->defect_type);
  oam_payload.lsp_ttsi.lsrpad = 0xFFFF;
  oam_payload.lsp_ttsi.lsr_addr = req->lsp_ttsi.lsr_addr;
  oam_payload.lsp_ttsi.lsp_id = pal_hton32(req->lsp_ttsi.lsp_id);
  oam_payload.frequency = req->frequency;
  oam_payload.defect_location = pal_hton32(req->defect_location);
  
  oam_payload.bip16 = pal_hton16(req->bip16);

  ENCODE_PUTC(&oam_payload.function_type);
  ENCODE_PUTC_EMPTY();

  switch (oam_payload.function_type)
  { 
    case MPLS_OAM_CV_PROCESS_MESSAGE:
    case MPLS_OAM_FFD_PROCESS_MESSAGE:
    	 ENCODE_PUTW_EMPTY();
    	break;
    case MPLS_OAM_FDI_PROCESS_MESSAGE:
    case MPLS_OAM_BDI_PROCESS_MESSAGE:
    	 ENCODE_PUTW(&oam_payload.defect_type);
    	break;
    default:
    	 return -1;
  }
  ENCODE_PUTL_EMPTY();
  ENCODE_PUTL_EMPTY();
  ENCODE_PUTW_EMPTY();
  ENCODE_PUTW(&oam_payload.lsp_ttsi.lsrpad);
  ENCODE_PUTL(&oam_payload.lsp_ttsi.lsr_addr);
  ENCODE_PUTL(&oam_payload.lsp_ttsi.lsp_id);


  switch (oam_payload.function_type)
  { 
    case MPLS_OAM_CV_PROCESS_MESSAGE:
    	    ENCODE_PUTL_EMPTY();
    case MPLS_OAM_FFD_PROCESS_MESSAGE:
    	    ENCODE_PUTC(&oam_payload.frequency);
    	    ENCODE_PUTW_EMPTY();
    	break;
    case MPLS_OAM_FDI_PROCESS_MESSAGE:
    case MPLS_OAM_BDI_PROCESS_MESSAGE:
    	 ENCODE_PUTW(&oam_payload.defect_location);
    	break;
    default:
    	 return -1;
  }
  
  ENCODE_PUTL_EMPTY();
  ENCODE_PUTL_EMPTY();
  ENCODE_PUTL_EMPTY();
  ENCODE_PUTW_EMPTY();
  ENCODE_PUTW(&oam_payload.bip16);
  
  return MPLS_OAM_ITUT_REQUEST_MIN_LEN;
}

 int
 nsm_mpls_oam_encode_itut_request (u_char **pnt, u_int16_t *size,
                                   struct nsm_mpls_oam_itut *req)
 {
   int ret,i=0;
   u_char *spnt = *pnt;
   struct oam_label alert_label;
   struct nsm_mpls_oam_itut *oam; 
   struct nsm_master *nm=req->nm;
   struct listnode *ln = NULL;
   u_int32_t label[3];
   u_int16_t level_no;

   /* Encode the OAM Alert label  */
   alert_label.pkt_type = req->function_type;
   ret = nsm_mpls_encode_oam_alert_label (pnt, size, &alert_label);
   if (ret < 0)
     return ret;
   
   if ( req->function_type == MPLS_OAM_FDI_PROCESS_MESSAGE )
    {
      LIST_LOOP (NSM_MPLS->mpls_oam_itut_list, oam, ln)
       {
         if (oam->lsp_ttsi.lsp_id == req->lsp_ttsi.lsp_id)
          {
            if ( oam->msg.fdi_level.level_no > 0)
             {
               level_no = pal_hton16(oam->msg.fdi_level.level_no);  
               ENCODE_PUTW(&level_no); 
               for ( i = 0; i < oam->msg.fdi_level.level_no; i++)
                {
                  label[i] = pal_hton32(oam->msg.fdi_level.lable_id[i]);
                  ENCODE_PUTL(&label[i]);   	   	  	  	  
   	    	}
   	     }
   	    break;
   	  }
       }   	  
    }
    
   /* Encode the OAM payload  */
   ret = nsm_mpls_oam_encode_itut_payload (pnt, size, req);
   if (ret < 0)
     return ret;
   
   return *pnt - spnt;
 
 }
 
 int
 nsm_mpls_encode_oam_alert_label (u_char **pnt, u_int16_t *size, struct oam_label *label_hdr)
 {
    u_int32_t label;
    label_hdr->u.l_packet = MPLS_OAM_ITUT_ALERT_LABEL;
    label = pal_hton32 (label_hdr->u.l_packet);
    ENCODE_PUTL(&label);
    ENCODE_PUTC(&label_hdr->pkt_type);
    return MPLS_OAM_LABEL_LEN;
 }

int
nsm_mpls_oam_decode_itut_request (u_char **pnt, u_int16_t *size,
                                  struct nsm_mpls_oam_itut *oam_req,
                                  int *err_code)
{
  /* We have a fixed number of octets in pnt. 
   */
  struct oam_label alert_lable; 
  u_int32_t label[3];
  u_int16_t level_no;
  u_char *spnt = *pnt;
  int len = 0;
  int i,ret = 0;

  *err_code = MPLS_OAM_MALFORMED_REQUEST;

  len = *size;

  /* Encode OAM Alert Label  */
  ret = nsm_mpls_decode_oam_alert_label (pnt, size, &alert_lable);
  if (ret < 0)
    return ret;

  if (alert_lable.u.l_packet != MPLS_OAM_ITUT_ALERT_LABEL)
    return NSM_FAILURE;
  
  if (alert_lable.pkt_type == MPLS_OAM_FDI_PROCESS_MESSAGE)
   {
     DECODE_GETW(&level_no);
     level_no = pal_ntoh32(level_no);
     if (level_no > 0)
      {
        oam_req->msg.fdi_level.level_no = level_no;
        for (i=0; i < level_no; i++)
        {
          DECODE_GETL(&label[i]);
  	  oam_req->msg.fdi_level.lable_id[i] = pal_ntoh32(label[i]);
  	}
      }
   }




  ret = nsm_mpls_oam_decode_itut_payload (pnt, size, oam_req);

  if (ret < 0)
    return ret;

  /* Check packet length. */
  if ((*pnt - spnt) != len)
    return MPLS_OAM_ERR_DECODE_LENGTH_ERROR;

  /* Reset error code */
  *err_code = 0;
  return 0;
}

int
 nsm_mpls_decode_oam_alert_label (u_char **pnt, u_int16_t *size, struct oam_label *label_hdr)
 {
   if (*size < MPLS_OAM_LABEL_LEN)
       return MPLS_OAM_ERR_PACKET_TOO_SMALL;
  
   DECODE_GETL(&label_hdr->u.l_packet);
   label_hdr->u.l_packet = pal_ntoh32 (label_hdr->u.l_packet);
   
   DECODE_GETC(&label_hdr->pkt_type);
 
   return MPLS_OAM_LABEL_LEN;
 }

int
nsm_mpls_oam_decode_itut_payload (u_char **pnt, u_int16_t *size, struct nsm_mpls_oam_itut *req)
{
  struct nsm_mpls_oam_itut oam_payload;

  if (*size < MPLS_OAM_ITUT_REQUEST_MIN_LEN)
  	  return MPLS_OAM_ERR_PACKET_TOO_SMALL; 
  	
  /* Decode the OAM */
  DECODE_GETC(&oam_payload.function_type);
  DECODE_GETC_EMPTY();

  switch (oam_payload.function_type)
  { 
    case MPLS_OAM_CV_PROCESS_MESSAGE:
    case MPLS_OAM_FFD_PROCESS_MESSAGE:
    	 DECODE_GETW_EMPTY();
    	break;
    case MPLS_OAM_FDI_PROCESS_MESSAGE:
    case MPLS_OAM_BDI_PROCESS_MESSAGE:
    	 DECODE_GETW(&oam_payload.defect_type);
    	break;
    default:
    	 return -1;
  }
  
  DECODE_GETL_EMPTY();
  DECODE_GETL_EMPTY();
  DECODE_GETW_EMPTY();
  DECODE_GETW(&oam_payload.lsp_ttsi.lsrpad);
  DECODE_GETL(&oam_payload.lsp_ttsi.lsr_addr);
  DECODE_GETL(&oam_payload.lsp_ttsi.lsp_id);

  switch (oam_payload.function_type)
  { 
    case MPLS_OAM_CV_PROCESS_MESSAGE:
    	 DECODE_GETL_EMPTY();
    	break;
    case MPLS_OAM_FFD_PROCESS_MESSAGE:
    	 DECODE_GETC(&oam_payload.frequency);
    	 DECODE_GETW_EMPTY();
    	break;
    case MPLS_OAM_FDI_PROCESS_MESSAGE:
    case MPLS_OAM_BDI_PROCESS_MESSAGE:
    	 DECODE_GETW(&oam_payload.defect_location);
    	break;
    default:
    	 return -1;
  }
  DECODE_GETL_EMPTY();
  DECODE_GETL_EMPTY();
  DECODE_GETL_EMPTY();
  DECODE_GETW_EMPTY();
  DECODE_GETW(&oam_payload.bip16);

  req->function_type = oam_payload.function_type;
  req->defect_type   = pal_ntoh16(oam_payload.defect_type);
  req->lsp_ttsi.lsrpad = oam_payload.lsp_ttsi.lsrpad;
  req->lsp_ttsi.lsr_addr = oam_payload.lsp_ttsi.lsr_addr;
  req->lsp_ttsi.lsp_id = pal_ntoh32(oam_payload.lsp_ttsi.lsp_id);
  req->frequency = oam_payload.frequency;
  req->defect_location = pal_ntoh32(oam_payload.defect_location);
  req->bip16 = pal_ntoh16(oam_payload.bip16);
  
  return MPLS_OAM_ITUT_REQUEST_MIN_LEN;
}

#endif 


