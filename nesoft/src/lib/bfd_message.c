/* Copyright (C) 2008  All Rights Reserved. */

/* BFD message implementation.  This implementation is used by both
   server and client side.  */

#include "pal.h"

#if defined (HAVE_BFD) || defined (HAVE_MPLS_OAM)

#include "log.h"
#include "tlv.h"
#include "message.h"
#include "bfd_message.h"

#ifdef HAVE_MPLS_OAM
#include "mpls.h"
#include "mpls_common.h"
#include "oam_mpls_msg.h"
#endif /* HAVE_MPLS_OAM */

/* BFD message strings.  */
static const char *bfd_msg_str[] =
{
  "BFD Service Request",                        /* 0 */
  "BFD Service Reply",                          /* 1 */
  "BFD Session Preserve State",                 /* 2 */
  "BFD Session Stale Remove",                   /* 3 */
  "BFD Session Add",                            /* 4 */
  "BFD Session Delete",                         /* 5 */
  "BFD Session Up",                             /* 6 */
  "BFD Session Down",                           /* 7 */
  "BFD Session State Error",                    /* 8 */
  "BFD Session Attribute Update",               /* 9 */
  "OAM MPLS echo request",                      /* 10 */
  "OAM MPLS ping reply",                        /* 11 */
  "OAM MPLS tracert reply",                     /* 12 */
  "OAM MPLS oam error",                         /* 13 */
};

/* BFD service strings.  */
static const char *bfd_service_str[] =
{
  "Session Service",                            /* 0 */
  "Restart Service",                            /* 1 */
  "OAM Service",                                /* 2 */
};

/* BFD message to string.  */
const char *
bfd_msg_to_str (int type)
{
  if (type < BFD_MSG_MAX)
    return bfd_msg_str [type];
  return "Unknown";
}

/* BFD service to string.  */
const char *
bfd_service_to_str (int service)
{
  if (service < BFD_SERVICE_MAX)
    return bfd_service_str [service];
  return "Unknown Service";
}

void
bfd_header_dump (struct lib_globals *zg, struct bfd_msg_header *header)
{
  zlog_info (zg, "BFD Message Header");
  zlog_info (zg, " VR ID: %lu", header->vr_id);
  zlog_info (zg, " VRF ID: %lu", header->vrf_id);
  zlog_info (zg, " Message type: %s (%d)", bfd_msg_to_str (header->type),
             header->type);
  zlog_info (zg, " Message length: %d", header->length);
  zlog_info (zg, " Message ID: 0x%08x", header->message_id);
}

void
bfd_service_dump (struct lib_globals *zg, struct bfd_msg_service *service)
{
  zlog_info (zg, "BFD Service");
  zlog_info (zg, " Version: %d", service->version);
  zlog_info (zg, " Reserved: %d", service->reserved);
  zlog_info (zg, " Module ID: %s (%d)", modname_strl (service->module_id),
             service->module_id);
  zlog_info (zg, " Bits: %d", service->bits);
  zlog_info (zg, " Restart State: %d", service->restart_state);
  zlog_info (zg, " Restart Grace Periode: %lu", service->grace_period);
}

void
bfd_session_dump (struct lib_globals *zg, struct bfd_msg_session *msg)
{
  zlog_info (zg, "BFD Session");
  if (CHECK_FLAG (msg->flags, BFD_MSG_SESSION_FLAG_MH))
    zlog_info (zg, " Flags: Multihop");
  if (CHECK_FLAG (msg->flags, BFD_MSG_SESSION_FLAG_DC))
    zlog_info (zg, " Flags: Demand Circuit");
  if (CHECK_FLAG (msg->flags, BFD_MSG_SESSION_FLAG_AD))
    zlog_info (zg, " Flags: Admin Down");
  zlog_info (zg, " BFD LL type: %u", msg->ll_type);
  zlog_info (zg, " AFI: %d", msg->afi);
  zlog_info (zg, " Interface index: %d", msg->ifindex);
  if (msg->afi == AFI_IP)
    {
      zlog_info (zg, " Session source: %r", &msg->addr.ipv4.src);
      zlog_info (zg, " Session destination: %r", &msg->addr.ipv4.dst);
    }
#ifdef HAVE_IPV6
  else if (msg->afi == AFI_IP6)
    {
      zlog_info (zg, " Session source: %R", &msg->addr.ipv6.src);
      zlog_info (zg, " Session destination: %R", &msg->addr.ipv6.dst);
    }
#endif /* HAVE_IPV6 */

#ifdef HAVE_MPLS_OAM
  if (msg->ll_type == BFD_MSG_LL_TYPE_MPLS_LSP)
    {
      zlog_info (zg, " MPLS Session FTN Ix: %d", 
                  msg->addl_data.mpls_params.ftn_ix);

      if (msg->addl_data.mpls_params.lsp_type == BFD_MPLS_LSP_TYPE_RSVP)
        zlog_info (zg, " MPLS RSVP Trunk: %s", 
                    msg->addl_data.mpls_params.tun_name);
      else
        zlog_info (zg, " MPLS FEC: %O", &msg->addl_data.mpls_params.fec);

      zlog_info (zg, " LSP Ping Interval: %d", 
                  msg->addl_data.mpls_params.lsp_ping_intvl);
      zlog_info (zg, " Session Min-Tx: %d", msg->addl_data.mpls_params.min_tx);
      zlog_info (zg, " Session Min-Rx: %d", msg->addl_data.mpls_params.min_rx);
      zlog_info (zg, " Session Detect Multiplier: %d", 
                 msg->addl_data.mpls_params.mult);
      if (CHECK_FLAG(msg->addl_data.mpls_params.fwd_flags, 
            MPLSONM_FWD_OPTION_NOPHP))
        zlog_info (zg, " Force explicit NULL label: True\n"); 
      else
        zlog_info (zg, " Force explicit NULL label: False\n"); 
                 
    }
#ifdef HAVE_VCCV
  else if (msg->ll_type == BFD_MSG_LL_TYPE_MPLS_VCCV)
    {
      zlog_info (zg, " MPLS VC ID: %d", &msg->addl_data.vccv_params.vc_id);
      zlog_info (zg, " MPLS VC Incoming VC Label: %d", 
                  &msg->addl_data.vccv_params.in_vc_label);
    }
#endif /* HAVE_VCCV */
#endif /* HAVE_MPLS_OAM */
}

void
bfd_session_manage_dump (struct lib_globals *zg,
                         struct bfd_msg_session_manage *msg)
{
  if (MSG_CHECK_CTYPE (msg->cindex, BFD_SESSION_MANAGE_CTYPE_RESTART_TIME))
    zlog_info (zg, " Restart Time: %lu", msg->restart_time);
}

/* BFD message parser.  */

/* BFD message header encode.  */
int
bfd_encode_header (u_char **pnt, u_int16_t *size,
                   struct bfd_msg_header *header)
{
  u_char *sp = *pnt;

  if (*size < BFD_MSG_HEADER_SIZE)
    return BFD_ERR_PKT_TOO_SMALL;

  TLV_ENCODE_PUTL (header->vr_id);
  TLV_ENCODE_PUTL (header->vrf_id);
  TLV_ENCODE_PUTW (header->type);
  TLV_ENCODE_PUTW (header->length);
  TLV_ENCODE_PUTL (header->message_id);

  return *pnt - sp;
}

/* BFD message header decode.  */
int
bfd_decode_header (u_char **pnt, u_int16_t *size,
                   struct bfd_msg_header *header)
{
  if (*size < BFD_MSG_HEADER_SIZE)
    return BFD_ERR_PKT_TOO_SMALL;

  TLV_DECODE_GETL (header->vr_id);
  TLV_DECODE_GETL (header->vrf_id);
  TLV_DECODE_GETW (header->type);
  TLV_DECODE_GETW (header->length);
  TLV_DECODE_GETL (header->message_id);

  return BFD_MSG_HEADER_SIZE;
}

static int
bfd_encode_tlv (u_char **pnt, u_int16_t *size, u_int16_t type,
                u_int16_t length)
{
  length += BFD_TLV_HEADER_SIZE;

  TLV_ENCODE_PUTW (type);
  TLV_ENCODE_PUTW (length);

  return BFD_TLV_HEADER_SIZE;
}

/* BFD service encode.  */
int
bfd_encode_service (u_char **pnt, u_int16_t *size, struct bfd_msg_service *msg)
{
  u_char *sp = *pnt;

  if (*size < BFD_MSG_SERVICE_SIZE)
    return BFD_ERR_PKT_TOO_SMALL;

  TLV_ENCODE_PUTW (msg->version);
  TLV_ENCODE_PUTW (0);
  TLV_ENCODE_PUTL (msg->module_id);
  TLV_ENCODE_PUTL (msg->bits);

  if (MSG_CHECK_CTYPE (msg->cindex, BFD_SERVICE_CTYPE_RESTART))
    {
      bfd_encode_tlv (pnt, size, BFD_SERVICE_CTYPE_RESTART, 8);
      TLV_ENCODE_PUTW (msg->restart_state);
      TLV_ENCODE_PUTW (0);
      TLV_ENCODE_PUTL (msg->grace_period);
    }

  return *pnt - sp;
}

/* BFD service decode.  */
int
bfd_decode_service (u_char **pnt, u_int16_t *size, struct bfd_msg_service *msg)
{
  u_char *sp = *pnt;
  struct bfd_tlv_header tlv;

  if (*size < BFD_MSG_SERVICE_SIZE)
    return BFD_ERR_PKT_TOO_SMALL;

  TLV_DECODE_GETW (msg->version);
  TLV_DECODE_GETW (msg->reserved);
  TLV_DECODE_GETL (msg->module_id);
  TLV_DECODE_GETL (msg->bits);

  /* Optional TLV parser.  */
  while (*size)
    {
      if (*size < BFD_TLV_HEADER_SIZE)
        return BFD_ERR_PKT_TOO_SMALL;

      MSG_DECODE_TLV_HEADER (tlv);

      switch (tlv.type)
        {
        case BFD_SERVICE_CTYPE_RESTART:
	  TLV_DECODE_GETW (msg->restart_state);
	  TLV_DECODE_GETW (msg->restart_reserved);
          TLV_DECODE_GETL (msg->grace_period);
          MSG_SET_CTYPE (msg->cindex, tlv.type);
          break;
        default:
          TLV_DECODE_SKIP (tlv.length);
          break;
        }
    }
  return *pnt - sp;
}

#ifdef HAVE_MPLS_OAM
/**@brief - This function encodes MPLS data required for BFD.

 *  @param **pnt - Buffer Pointer.
 *  @param *size - Buffer size.
 *  @param *msg - BFD session message.

 *  @return Number of bytes encoded or Error (< 0)
 */
int
bfd_encode_session_mpls_data (u_char **pnt, u_int16_t *size,
                              struct bfd_msg_session *msg)
{
  u_char *sp = *pnt;

  /* Check size.  */
  if (*size < BFD_MPLS_MSG_SESSION_SIZE)
    return BFD_ERR_PKT_TOO_SMALL;

  TLV_ENCODE_PUTL (msg->addl_data.mpls_params.ftn_ix);
  TLV_ENCODE_PUTW (msg->addl_data.mpls_params.lsp_type);

  if (msg->addl_data.mpls_params.lsp_type == BFD_MPLS_LSP_TYPE_RSVP)
    {
      TLV_ENCODE_PUTL (msg->addl_data.mpls_params.tun_length);
      TLV_ENCODE_PUT (msg->addl_data.mpls_params.tun_name, 
                       msg->addl_data.mpls_params.tun_length);
    }
  else
    {
      TLV_ENCODE_PUTC (msg->addl_data.mpls_params.fec.family);
      TLV_ENCODE_PUTC (msg->addl_data.mpls_params.fec.prefixlen);
      TLV_ENCODE_PUTC (msg->addl_data.mpls_params.fec.pad1);
      TLV_ENCODE_PUTC (msg->addl_data.mpls_params.fec.pad2);
      TLV_ENCODE_PUT_IN4_ADDR (&msg->addl_data.mpls_params.fec.prefix);
    }

  TLV_ENCODE_PUTC (msg->addl_data.mpls_params.is_egress);
  TLV_ENCODE_PUTL (msg->addl_data.mpls_params.lsp_ping_intvl);
  TLV_ENCODE_PUTL (msg->addl_data.mpls_params.min_tx);
  TLV_ENCODE_PUTL (msg->addl_data.mpls_params.min_rx);
  TLV_ENCODE_PUTC (msg->addl_data.mpls_params.mult);
  TLV_ENCODE_PUTC (msg->addl_data.mpls_params.fwd_flags);

  TLV_ENCODE_PUTL (msg->addl_data.mpls_params.tunnel_label); 

  return *pnt - sp;
}
#endif /* HAVE_MPLS_OAM */

#ifdef HAVE_VCCV
/**@brief - This function encodes VCCV data required for BFD.

 *  @param **pnt - Buffer Pointer.
 *  @param *size - Buffer size.
 *  @param *msg - BFD session message.

 *  @return Number of bytes encoded or Error (< 0)
 */
int
bfd_encode_session_vccv_data (u_char **pnt, u_int16_t *size,
                              struct bfd_msg_session *msg)
{
  u_char *sp = *pnt;

  /* Check size.  */
  if (*size < BFD_VCCV_MSG_SESSION_SIZE)
    return BFD_ERR_PKT_TOO_SMALL;

  TLV_ENCODE_PUTL (msg->addl_data.vccv_params.vc_id);
  TLV_ENCODE_PUTL (msg->addl_data.vccv_params.ac_ix);
  TLV_ENCODE_PUTL (msg->addl_data.vccv_params.in_vc_label);
  TLV_ENCODE_PUTL (msg->addl_data.vccv_params.out_vc_label);
  TLV_ENCODE_PUTC (msg->addl_data.vccv_params.cv_type);
  TLV_ENCODE_PUTC (msg->addl_data.vccv_params.cc_type);

  TLV_ENCODE_PUTL (msg->addl_data.vccv_params.tunnel_label);

  return *pnt - sp;
}
#endif /* HAVE_VCCV */

int
bfd_encode_session (u_char **pnt, u_int16_t *size,
                    struct bfd_msg_session *msg)
{
  u_char *sp = *pnt;
#ifdef HAVE_MPLS_OAM
  int ret;
#endif /* HAVE_MPLS_OAM */

  /* Check size.  */
  if (*size < BFD_MSG_SESSION_SIZE)
    return BFD_ERR_PKT_TOO_SMALL;

  TLV_ENCODE_PUTC (msg->flags);
  TLV_ENCODE_PUTC (msg->ll_type);
  TLV_ENCODE_PUTW (msg->afi);
  TLV_ENCODE_PUTL (msg->ifindex);
  if (msg->afi == AFI_IP)
    {
      TLV_ENCODE_PUT_IN4_ADDR (&msg->addr.ipv4.src);
      TLV_ENCODE_PUT_IN4_ADDR (&msg->addr.ipv4.dst);
    }
#ifdef HAVE_IPV6
  else if (msg->afi == AFI_IP6)
    {
      TLV_ENCODE_PUT_IN6_ADDR (&msg->addr.ipv6.src);
      TLV_ENCODE_PUT_IN6_ADDR (&msg->addr.ipv6.dst);
    }
#endif /* HAVE_IPV6 */
#ifdef HAVE_MPLS_OAM
  /* Encode Additional data required for BFD MPLS & BFD VCCV. */
  if (msg->ll_type == BFD_MSG_LL_TYPE_MPLS_LSP)
    {
     ret = bfd_encode_session_mpls_data (pnt, size, msg);
      if (ret < 0)
        return ret;
    }
#ifdef HAVE_VCCV
  else if (msg->ll_type == BFD_MSG_LL_TYPE_MPLS_VCCV)
    {
      ret = bfd_encode_session_vccv_data (pnt, size, msg);
      if (ret < 0)
        return ret;
    }
#endif /* HAVE_VCCV */
#endif /* HAVE_MPLS_OAM */

  return *pnt - sp;
}

#ifdef HAVE_MPLS_OAM
/**@brief - This function decodes BFD MPLS data.

 *  @param **pnt - Data Pointer.
 *  @param *size - Data size.
 *  @param *msg - BFD session message.

 *  @return Number of bytes decoded or Error (< 0)
 */
int
bfd_decode_session_mpls_data (u_char **pnt, u_int16_t *size,
                              struct bfd_msg_session *msg)
{
  u_char *sp = *pnt;

  TLV_DECODE_GETL (msg->addl_data.mpls_params.ftn_ix);
  TLV_DECODE_GETW (msg->addl_data.mpls_params.lsp_type);

  if (msg->addl_data.mpls_params.lsp_type == BFD_MPLS_LSP_TYPE_RSVP)
    {
      TLV_DECODE_GETL (msg->addl_data.mpls_params.tun_length);

      msg->addl_data.mpls_params.tun_name = 
        XCALLOC (MTYPE_TMP, (msg->addl_data.mpls_params.tun_length + 1));

      if (!msg->addl_data.mpls_params.tun_name)
        return BFD_ERR_SYSTEM_FAILURE;

      TLV_DECODE_GET (msg->addl_data.mpls_params.tun_name,
                       msg->addl_data.mpls_params.tun_length);
    }
  else
    {
      TLV_DECODE_GETC (msg->addl_data.mpls_params.fec.family);
      TLV_DECODE_GETC (msg->addl_data.mpls_params.fec.prefixlen);
      TLV_DECODE_GETC (msg->addl_data.mpls_params.fec.pad1);
      TLV_DECODE_GETC (msg->addl_data.mpls_params.fec.pad2);
      TLV_DECODE_GET_IN4_ADDR (&msg->addl_data.mpls_params.fec.prefix);
    }

  TLV_DECODE_GETC (msg->addl_data.mpls_params.is_egress);
  TLV_DECODE_GETL (msg->addl_data.mpls_params.lsp_ping_intvl);
  TLV_DECODE_GETL (msg->addl_data.mpls_params.min_tx);
  TLV_DECODE_GETL (msg->addl_data.mpls_params.min_rx);
  TLV_DECODE_GETC (msg->addl_data.mpls_params.mult);
  TLV_DECODE_GETC (msg->addl_data.mpls_params.fwd_flags);

  TLV_DECODE_GETL (msg->addl_data.mpls_params.tunnel_label);

  return *pnt - sp;
}
#endif /* HAVE_MPLS_OAM */

#ifdef HAVE_VCCV
/**@brief - This function decodes BFD VCCV data.

 *  @param **pnt - Data Pointer.
 *  @param *size - Data size.
 *  @param *msg - BFD session message.

 *  @return Number of bytes decoded or Error (< 0)
 */
int
bfd_decode_session_vccv_data (u_char **pnt, u_int16_t *size,
                              struct bfd_msg_session *msg)
{
  u_char *sp = *pnt;

  TLV_DECODE_GETL (msg->addl_data.vccv_params.vc_id);
  TLV_DECODE_GETL (msg->addl_data.vccv_params.ac_ix);
  TLV_DECODE_GETL (msg->addl_data.vccv_params.in_vc_label);
  TLV_DECODE_GETL (msg->addl_data.vccv_params.out_vc_label);
  TLV_DECODE_GETC (msg->addl_data.vccv_params.cv_type);
  TLV_DECODE_GETC (msg->addl_data.vccv_params.cc_type);

  TLV_DECODE_GETL (msg->addl_data.vccv_params.tunnel_label);

  return *pnt - sp;
}
#endif /* HAVE_VCCV */

int
bfd_decode_session (u_char **pnt, u_int16_t *size,
                    struct bfd_msg_session *msg)
{
  u_char *sp = *pnt;
#ifdef HAVE_MPLS_OAM
  int ret;
#endif /* HAVE_MPLS_OAM */

  /* Check size.  */
  if (*size < BFD_MSG_SESSION_SIZE)
    return BFD_ERR_PKT_TOO_SMALL;

  /* Interface index.  */
  TLV_DECODE_GETC (msg->flags);
  TLV_DECODE_GETC (msg->ll_type);
  TLV_DECODE_GETW (msg->afi);
  TLV_DECODE_GETL (msg->ifindex);
  if (msg->afi == AFI_IP)
    {
      TLV_DECODE_GET_IN4_ADDR (&msg->addr.ipv4.src);
      TLV_DECODE_GET_IN4_ADDR (&msg->addr.ipv4.dst);
    }
#ifdef HAVE_IPV6
  else if (msg->afi == AFI_IP6)
    {
      TLV_DECODE_GET_IN6_ADDR (&msg->addr.ipv6.src);
      TLV_DECODE_GET_IN6_ADDR (&msg->addr.ipv6.dst);
    }
#endif /* HAVE_IPV6 */
  else
    return BFD_ERR_INVALID_AFI;
#ifdef HAVE_MPLS_OAM
  /* Decode Additional data required for BFD MPLS & BFD VCCV. */
  if (msg->ll_type == BFD_MSG_LL_TYPE_MPLS_LSP)
    {
      ret = bfd_decode_session_mpls_data (pnt, size, msg);
      if (ret < 0)
        return ret;
    }
#ifdef HAVE_VCCV
  else if (msg->ll_type == BFD_MSG_LL_TYPE_MPLS_VCCV)
    {
      ret = bfd_decode_session_vccv_data (pnt, size, msg);
      if (ret < 0)
        return ret;
    }
#endif /* HAVE_VCCV */
#endif /* HAVE_MPLS_OAM */

  return *pnt - sp;
}

int
bfd_encode_session_manage (u_char **pnt, u_int16_t *size,
                           struct bfd_msg_session_manage *msg)
{
  int i;
  u_char *sp = *pnt;

  /* Interface TLV parse.  */
  for (i = 0; i < BFD_CINDEX_SIZE; i++)
    if (MSG_CHECK_CTYPE (msg->cindex, i))
      {
        if (*size < BFD_TLV_HEADER_SIZE)
          return BFD_ERR_PKT_TOO_SMALL;
        switch (i)
          {
          case BFD_SESSION_MANAGE_CTYPE_RESTART_TIME:
            bfd_encode_tlv (pnt, size, i, 4);
            TLV_ENCODE_PUTL (msg->restart_time);
            break;
          default:
            break;
          }
      }
  return *pnt - sp;
}

int
bfd_decode_session_manage (u_char **pnt, u_int16_t *size,
                           struct bfd_msg_session_manage *msg)
{
  u_char *sp = *pnt;
  struct bfd_tlv_header tlv;

  /* Interface TLV parse.  */
  while (*size)
    {
      if (*size < BFD_TLV_HEADER_SIZE)
        return BFD_ERR_PKT_TOO_SMALL;

      MSG_DECODE_TLV_HEADER (tlv);

      switch (tlv.type)
        {
        case BFD_SESSION_MANAGE_CTYPE_RESTART_TIME:
          TLV_DECODE_GETL (msg->restart_time);
          MSG_SET_CTYPE (msg->cindex, tlv.type);
          break;
        default:
          TLV_DECODE_SKIP (tlv.length);
          break;
        }
    }
  return *pnt - sp;
}

/* All message parser.  These parser are used by both server and
   client side. */

/* Parse BFD service message.  */
int
bfd_parse_service (u_char **pnt, u_int16_t *size,
                   struct bfd_msg_header *header, void *arg,
                   BFD_CALLBACK callback)
{
  int ret;
  struct bfd_msg_service msg;

  pal_mem_set (&msg, 0, sizeof (struct bfd_msg_service));

  /* Parse service.  */
  ret = bfd_decode_service (pnt, size, &msg);
  if (ret < 0)
    return ret;

  /* Call callback with arg. */
  if (callback)
    {
      ret = (*callback) (header, arg, &msg);
      if (ret < 0)
        return ret;
    }
  return 0;
}

/* Parse BFD interface session message.  */
int
bfd_parse_session (u_char **pnt, u_int16_t *size,
                   struct bfd_msg_header *header, void *arg,
                   BFD_CALLBACK callback)
{
  struct bfd_msg_session msg;
  int ret = 0;

  while (*size != 0)
    {
      pal_mem_set (&msg, 0, sizeof (struct bfd_msg_session));

      ret = bfd_decode_session (pnt, size, &msg);

      if (ret < 0)
        return ret;

      if (callback)
        ret = (*callback) (header, arg, &msg);

#ifdef HAVE_MPLS_OAM
      /* Free the memory allocated during decode */
      if ((msg.ll_type == BFD_MSG_LL_TYPE_MPLS_LSP) && 
          (msg.addl_data.mpls_params.lsp_type == BFD_MPLS_LSP_TYPE_RSVP))
        {
          if (msg.addl_data.mpls_params.tun_name)
            XFREE (MTYPE_TMP, msg.addl_data.mpls_params.tun_name);
        }
#endif /* HAVE_MPLS_OAM */

      if (ret < 0)
        return ret;
    }

  return 0;
}

int
bfd_parse_session_manage (u_char **pnt, u_int16_t *size,
                          struct bfd_msg_header *header, void *arg,
                          BFD_CALLBACK callback)
{
  struct bfd_msg_session_manage msg;
  int ret = 0;

  while (*size != 0)
    {
      pal_mem_set (&msg, 0, sizeof (struct bfd_msg_session_manage));

      ret = bfd_decode_session_manage (pnt, size, &msg);
      if (ret < 0)
        return ret;

      if (callback)
        {
          ret = (*callback) (header, arg, &msg);
          if (ret < 0)
            return ret;
        }
    }
  return 0;
}

#ifdef HAVE_MPLS_OAM
int
oam_parse_mpls_oam_request (u_char **pnt, u_int16_t *size,
                          struct bfd_msg_header *header, void *arg,
                          BFD_CALLBACK callback)
{
  int ret;
  struct oam_msg_mpls_ping_req msg;

  pal_mem_set (&msg, 0, sizeof (struct oam_msg_mpls_ping_req));

  ret = oam_decode_mpls_onm_req (pnt, size, &msg);
  if (ret < 0)
    return ret;

  if (callback)
    {
      ret = (*callback) (header, arg, &msg);
      if (ret < 0)
        return ret;
    }
  return 0;
}

int
oam_encode_mpls_onm_req (u_char **pnt, u_int16_t *size,
                         struct oam_msg_mpls_ping_req *msg)
{
  u_char *sp = *pnt;
  int i;

  /* Check size.  */
  if (*size < OAM_MSG_MPLS_ONM_SIZE)
    return BFD_ERR_PKT_TOO_SMALL;

  /* MPLS ONM TLV parse. */
  for (i = 0; i < BFD_CINDEX_SIZE; i++)
    {
      if (MSG_CHECK_CTYPE (msg->cindex, i))
        {
          if (*size < BFD_TLV_HEADER_SIZE)
            return BFD_ERR_PKT_TOO_SMALL;

          switch (i)
            {    
              case OAM_CTYPE_MSG_MPLSONM_OPTION_ONM_TYPE :
                bfd_encode_tlv (pnt, size, i, 1);
                TLV_ENCODE_PUTC (msg->req_type);
              break;
              case OAM_CTYPE_MSG_MPLSONM_OPTION_LSP_TYPE:
                bfd_encode_tlv (pnt, size, i, 1);
                TLV_ENCODE_PUTC (msg->type);
                break;
              case OAM_CTYPE_MSG_MPLSONM_OPTION_LSP_ID:
                if (msg->type == MPLSONM_OPTION_L2CIRCUIT || 
                    msg->type == MPLSONM_OPTION_VPLS)
                  {
#ifndef HAVE_VCCV
                    bfd_encode_tlv (pnt, size, i, 4);
                    TLV_ENCODE_PUTL (msg->u.l2_data.vc_id);
#else
                    bfd_encode_tlv (pnt, size, i, 5);
                    TLV_ENCODE_PUTL (msg->u.l2_data.vc_id);
                    TLV_ENCODE_PUTC (msg->u.l2_data.is_vccv);
#endif /* HAVE_VCCV */
                  }
                else if (msg->type == MPLSONM_OPTION_L3VPN)
                  {
                    bfd_encode_tlv (pnt, size, i, msg->u.fec.vrf_name_len+2);
                    TLV_ENCODE_PUTW (msg->u.fec.vrf_name_len);
                   TLV_ENCODE_PUT (msg->u.fec.vrf_name,msg->u.fec.vrf_name_len);
                  }              
                break;
              case OAM_CTYPE_MSG_MPLSONM_OPTION_DESTINATION:
                bfd_encode_tlv (pnt, size, i, 4);
                TLV_ENCODE_PUT_IN4_ADDR (&msg->dst);
                break;
              case OAM_CTYPE_MSG_MPLSONM_OPTION_SOURCE:
                bfd_encode_tlv (pnt, size, i, 4);
                TLV_ENCODE_PUT_IN4_ADDR (&msg->src);
              break;
              case OAM_CTYPE_MSG_MPLSONM_OPTION_TUNNELNAME:
                bfd_encode_tlv (pnt, size, i, msg->u.rsvp.tunnel_len+2);
                TLV_ENCODE_PUTW (msg->u.rsvp.tunnel_len);
                TLV_ENCODE_PUT (msg->u.rsvp.tunnel_name, msg->u.rsvp.tunnel_len);
                break;
              case OAM_CTYPE_MSG_MPLSONM_OPTION_EGRESS:        
                bfd_encode_tlv (pnt, size, i, 4);
                TLV_ENCODE_PUT_IN4_ADDR (&msg->u.rsvp.addr);
                break;
              case OAM_CTYPE_MSG_MPLSONM_OPTION_PREFIX: 
                bfd_encode_tlv (pnt, size, i, 8);
                TLV_ENCODE_PUT_IN4_ADDR (&msg->u.fec.vpn_prefix);
                TLV_ENCODE_PUTL (msg->u.fec.prefixlen);
                break;
              case OAM_CTYPE_MSG_MPLSONM_OPTION_FEC:
                bfd_encode_tlv (pnt, size, i, 8);
                TLV_ENCODE_PUT_IN4_ADDR (&msg->u.fec.ip_addr);
                TLV_ENCODE_PUTL (msg->u.fec.prefixlen);
                break;  
              case OAM_CTYPE_MSG_MPLSONM_OPTION_PEER:
                bfd_encode_tlv (pnt, size, i, 4);
                TLV_ENCODE_PUT_IN4_ADDR (&msg->u.l2_data.vc_peer);
                break;
              case OAM_CTYPE_MSG_MPLSONM_OPTION_EXP:
                bfd_encode_tlv (pnt, size, i, 1);
                TLV_ENCODE_PUTC (msg->exp_bits);
                break;
              case OAM_CTYPE_MSG_MPLSONM_OPTION_TTL:
                bfd_encode_tlv (pnt, size, i, 1);
                TLV_ENCODE_PUTC (msg->ttl);
                break;
              case OAM_CTYPE_MSG_MPLSONM_OPTION_TIMEOUT:
                bfd_encode_tlv (pnt, size,i, 4);
                TLV_ENCODE_PUTL (msg->timeout);
                break;
              case OAM_CTYPE_MSG_MPLSONM_OPTION_REPEAT:
                bfd_encode_tlv (pnt, size,i, 4);
                TLV_ENCODE_PUTL (msg->repeat);
                break;
              case OAM_CTYPE_MSG_MPLSONM_OPTION_VERBOSE:
                bfd_encode_tlv (pnt, size,i, 1);
                TLV_ENCODE_PUTC (msg->timeout);
                break;
              case OAM_CTYPE_MSG_MPLSONM_OPTION_REPLYMODE:
                bfd_encode_tlv (pnt, size, i,1);
                TLV_ENCODE_PUTC (msg->reply_mode);
                break;
              case OAM_CTYPE_MSG_MPLSONM_OPTION_INTERVAL:
                bfd_encode_tlv (pnt, size, i,4);
                TLV_ENCODE_PUTL (msg->interval);
                break;
              case OAM_CTYPE_MSG_MPLSONM_OPTION_FLAGS:
                bfd_encode_tlv (pnt, size,i, 1);
                TLV_ENCODE_PUTC (msg->flags);
                break;
              case OAM_CTYPE_MSG_MPLSONM_OPTION_NOPHP:
                bfd_encode_tlv (pnt, size,i, 1);
                TLV_ENCODE_PUTC (msg->nophp);
                break;
            }
        }
    }

  return *pnt - sp;
}

int
oam_decode_mpls_onm_req (u_char **pnt, u_int16_t *size,
                         struct oam_msg_mpls_ping_req *msg)
{
  u_char *sp = *pnt;
  struct bfd_tlv_header tlv;

  /* Check size.  */
  if (*size < OAM_MSG_MPLS_ONM_SIZE)
    return BFD_ERR_PKT_TOO_SMALL;

  /* TLV parse.  */
  while (*size)
    {
      if (*size < BFD_TLV_HEADER_SIZE)
        return BFD_ERR_PKT_TOO_SMALL;

      MSG_DECODE_TLV_HEADER (tlv);

      switch (tlv.type)
        { 
           case OAM_CTYPE_MSG_MPLSONM_OPTION_ONM_TYPE :
             TLV_DECODE_GETC (msg->req_type);
             MSG_SET_CTYPE (msg->cindex, tlv.type);
             break;
            case OAM_CTYPE_MSG_MPLSONM_OPTION_LSP_TYPE:
              TLV_DECODE_GETC (msg->type);
             MSG_SET_CTYPE (msg->cindex, tlv.type);
              break;
            case OAM_CTYPE_MSG_MPLSONM_OPTION_LSP_ID:
              if (msg->type == MPLSONM_OPTION_L2CIRCUIT || 
                msg->type == MPLSONM_OPTION_VPLS)
                {
                TLV_DECODE_GETL (msg->u.l2_data.vc_id);
#ifdef HAVE_VCCV
                TLV_DECODE_GETC (msg->u.l2_data.is_vccv);
#endif /* HAVE_VCCV */
                }
              else if (msg->type == MPLSONM_OPTION_L3VPN)
                {
                  TLV_DECODE_GETW (msg->u.fec.vrf_name_len);
                  msg->u.fec.vrf_name = XCALLOC (MTYPE_VRF_NAME,
                                                 msg->u.fec.vrf_name_len+1);                             
                  TLV_DECODE_GET (msg->u.fec.vrf_name, msg->u.fec.vrf_name_len);
                }
             MSG_SET_CTYPE (msg->cindex, tlv.type);
              break;
           case OAM_CTYPE_MSG_MPLSONM_OPTION_DESTINATION:
             TLV_DECODE_GET_IN4_ADDR (&msg->dst);
             MSG_SET_CTYPE (msg->cindex, tlv.type);
             break;
           case OAM_CTYPE_MSG_MPLSONM_OPTION_SOURCE:
             TLV_DECODE_GET_IN4_ADDR (&msg->src);
             MSG_SET_CTYPE (msg->cindex, tlv.type);
             break;
           case OAM_CTYPE_MSG_MPLSONM_OPTION_TUNNELNAME:
             TLV_DECODE_GETW (msg->u.rsvp.tunnel_len);
             msg->u.rsvp.tunnel_name = XMALLOC (MTYPE_TMP,
                                                msg->u.rsvp.tunnel_len);
             TLV_DECODE_GET (msg->u.rsvp.tunnel_name, msg->u.rsvp.tunnel_len);
             MSG_SET_CTYPE (msg->cindex, tlv.type);
             break;
           case OAM_CTYPE_MSG_MPLSONM_OPTION_EGRESS:
             TLV_DECODE_GET_IN4_ADDR (&msg->u.rsvp.addr);
             MSG_SET_CTYPE (msg->cindex, tlv.type);
             break;
           case OAM_CTYPE_MSG_MPLSONM_OPTION_PREFIX:
             TLV_DECODE_GET_IN4_ADDR (&msg->u.fec.vpn_prefix);
             TLV_DECODE_GETL (msg->u.fec.prefixlen);
             MSG_SET_CTYPE (msg->cindex, tlv.type);
             break;
           case OAM_CTYPE_MSG_MPLSONM_OPTION_PEER:
             TLV_DECODE_GET_IN4_ADDR (&msg->u.l2_data.vc_peer);
             MSG_SET_CTYPE (msg->cindex, tlv.type);
             break;
           case OAM_CTYPE_MSG_MPLSONM_OPTION_FEC:
             TLV_DECODE_GET_IN4_ADDR (&msg->u.fec.ip_addr);
             TLV_DECODE_GETL (msg->u.fec.prefixlen);
             MSG_SET_CTYPE (msg->cindex, tlv.type);
                break;
           case OAM_CTYPE_MSG_MPLSONM_OPTION_EXP:
             TLV_DECODE_GETC (msg->exp_bits);
             MSG_SET_CTYPE (msg->cindex, tlv.type);
             break;
           case OAM_CTYPE_MSG_MPLSONM_OPTION_TTL:
             TLV_DECODE_GETC (msg->ttl);
             MSG_SET_CTYPE (msg->cindex, tlv.type);
             break;
           case OAM_CTYPE_MSG_MPLSONM_OPTION_TIMEOUT:
             TLV_DECODE_GETL (msg->timeout);
             MSG_SET_CTYPE (msg->cindex, tlv.type);
             break;
           case OAM_CTYPE_MSG_MPLSONM_OPTION_REPEAT:
             TLV_DECODE_GETL (msg->repeat);
             MSG_SET_CTYPE (msg->cindex, tlv.type);
             break;
           case OAM_CTYPE_MSG_MPLSONM_OPTION_VERBOSE:
             TLV_DECODE_GETC (msg->timeout);
             MSG_SET_CTYPE (msg->cindex, tlv.type);
             break;
           case OAM_CTYPE_MSG_MPLSONM_OPTION_REPLYMODE:
             TLV_DECODE_GETC (msg->reply_mode);
             MSG_SET_CTYPE (msg->cindex, tlv.type);
             break;
           case OAM_CTYPE_MSG_MPLSONM_OPTION_INTERVAL:
             TLV_DECODE_GETL (msg->interval);
             MSG_SET_CTYPE (msg->cindex, tlv.type);
             break;
           case OAM_CTYPE_MSG_MPLSONM_OPTION_FLAGS:
             TLV_DECODE_GETC (msg->flags);
             MSG_SET_CTYPE (msg->cindex, tlv.type);
             break;
           case OAM_CTYPE_MSG_MPLSONM_OPTION_NOPHP:
             TLV_DECODE_GETC (msg->nophp);
             MSG_SET_CTYPE (msg->cindex, tlv.type);
             break;
           default:
             TLV_DECODE_SKIP (tlv.length);
             break;
        }
    }
  return *pnt - sp;
}

int
oam_encode_mpls_onm_ping_reply (u_char **pnt, u_int16_t *size,
                                struct oam_msg_mpls_ping_reply *msg)
{
  u_int32_t i;
  u_char *sp = *pnt;

  /* Check size.  */
  if (*size < OAM_MSG_MPLS_ONM_SIZE)
    return BFD_ERR_PKT_TOO_SMALL;

  /* MPLS ONM TLV parse. */
  for (i = 0; i < BFD_CINDEX_SIZE; i++)
    {
      if (MSG_CHECK_CTYPE (msg->cindex, i))
        {
          if (*size < BFD_TLV_HEADER_SIZE)
            return BFD_ERR_PKT_TOO_SMALL;

          switch (i)
            {    
              case OAM_CTYPE_MSG_MPLSONM_PING_SENTTIME : 
                bfd_encode_tlv (pnt, size, i,8);
                TLV_ENCODE_PUTL (msg->sent_time_sec);
                TLV_ENCODE_PUTL (msg->sent_time_usec);
                break;
              case OAM_CTYPE_MSG_MPLSONM_PING_RECVTIME :
                bfd_encode_tlv (pnt, size, i,8);
                TLV_ENCODE_PUTL (msg->recv_time_sec);
                TLV_ENCODE_PUTL (msg->recv_time_usec);
                break;
              case OAM_CTYPE_MSG_MPLSONM_PING_REPLYADDR :
                bfd_encode_tlv (pnt, size,i, 4);
                TLV_ENCODE_PUT_IN4_ADDR (&msg->reply);
                break;
              case OAM_CTYPE_MSG_MPLSONM_PING_VPNID: 
                bfd_encode_tlv (pnt, size, i,8);
                TLV_ENCODE_PUTL (msg->recv_time_sec);
                TLV_ENCODE_PUTL (msg->recv_time_usec);
                break;
              case OAM_CTYPE_MSG_MPLSONM_PING_COUNT: 
                bfd_encode_tlv (pnt, size, i,4);
                TLV_ENCODE_PUTL (msg->ping_count);
                break;
              case OAM_CTYPE_MSG_MPLSONM_PING_RECVCOUNT: 
                bfd_encode_tlv (pnt, size,i, 4);
                TLV_ENCODE_PUTL (msg->recv_count);
                break;  
              case OAM_CTYPE_MSG_MPLSONM_PING_RETCODE: 
                bfd_encode_tlv (pnt, size, i,4);
                TLV_ENCODE_PUTL (msg->ret_code);
                break;

            }
        }
    }

  return *pnt - sp;
}

int
oam_decode_mpls_onm_ping_reply (u_char **pnt, u_int16_t *size,
                                struct oam_msg_mpls_ping_reply *msg)
{  
  u_char *sp = *pnt;
  struct bfd_tlv_header tlv;

  /* Check size.  */
  if (*size < OAM_MSG_MPLS_ONM_SIZE)
    return BFD_ERR_PKT_TOO_SMALL;

  /* TLV parse.  */
  while (*size)
    {
      if (*size < BFD_TLV_HEADER_SIZE)
        return BFD_ERR_PKT_TOO_SMALL;

      MSG_DECODE_TLV_HEADER (tlv);

      switch (tlv.type)
        {  
          case OAM_CTYPE_MSG_MPLSONM_PING_SENTTIME : 
            TLV_DECODE_GETL (msg->sent_time_sec);
            TLV_DECODE_GETL (msg->sent_time_usec);
            MSG_SET_CTYPE (msg->cindex, tlv.type);
            break;
          case OAM_CTYPE_MSG_MPLSONM_PING_RECVTIME :
            TLV_DECODE_GETL (msg->recv_time_sec);
            TLV_DECODE_GETL (msg->recv_time_usec);
            MSG_SET_CTYPE (msg->cindex, tlv.type);
            break;
          case OAM_CTYPE_MSG_MPLSONM_PING_REPLYADDR :
            TLV_DECODE_GET_IN4_ADDR (&msg->reply);      
            MSG_SET_CTYPE (msg->cindex, tlv.type); 
            break;
          case OAM_CTYPE_MSG_MPLSONM_PING_COUNT: 
            TLV_DECODE_GETL (msg->ping_count);
            MSG_SET_CTYPE (msg->cindex, tlv.type);
            break;
          case OAM_CTYPE_MSG_MPLSONM_PING_RECVCOUNT: 
            TLV_DECODE_GETL (msg->recv_count);
            MSG_SET_CTYPE (msg->cindex, tlv.type);
            break;
          case OAM_CTYPE_MSG_MPLSONM_PING_RETCODE: 
            TLV_DECODE_GETL (msg->ret_code);
            MSG_SET_CTYPE (msg->cindex, tlv.type);
            break;
          default:
            TLV_DECODE_SKIP (tlv.length);
            break;
        }
    }
    
  return *pnt - sp;
}

int
oam_encode_mpls_onm_trace_reply (u_char **pnt, u_int16_t *size,
                                 struct oam_msg_mpls_tracert_reply *msg)
{
  u_char *sp = *pnt;
  int i;
  struct shimhdr *shim;
  struct listnode *node;

  /* Check size.  */
  if (*size < OAM_MSG_MPLS_ONM_SIZE)
    return BFD_ERR_PKT_TOO_SMALL;

  /* MPLS ONM TLV parse. */
  for (i = 0; i < BFD_CINDEX_SIZE; i++)
    {
      if (MSG_CHECK_CTYPE (msg->cindex, i))
        {
          if (*size < BFD_TLV_HEADER_SIZE)
            return BFD_ERR_PKT_TOO_SMALL;

          switch (i)
            {    
              case OAM_CTYPE_MSG_MPLSONM_TRACE_REPLYADDR :
                bfd_encode_tlv (pnt, size,i, 4);
                TLV_ENCODE_PUT_IN4_ADDR (&msg->reply);
                break;
              case OAM_CTYPE_MSG_MPLSONM_TRACE_SENTTIME :
                bfd_encode_tlv (pnt, size, i,8);
                TLV_ENCODE_PUTL (msg->sent_time_sec);
                TLV_ENCODE_PUTL (msg->sent_time_usec);
                break;
              case OAM_CTYPE_MSG_MPLSONM_TRACE_RECVTIME :
                bfd_encode_tlv (pnt, size, i,8);
                TLV_ENCODE_PUTL (msg->recv_time_sec);
                TLV_ENCODE_PUTL (msg->recv_time_usec);
                break;
              case OAM_CTYPE_MSG_MPLSONM_TRACE_TTL: 
                bfd_encode_tlv (pnt, size, i,1);
                TLV_ENCODE_PUTC (msg->ttl);
                break; 
              case OAM_CTYPE_MSG_MPLSONM_TRACE_RETCODE: 
                bfd_encode_tlv (pnt, size, i,1);
                TLV_ENCODE_PUTC (msg->ret_code);
                break;
              case OAM_CTYPE_MSG_MPLSONM_TRACE_DSMAP: 
                bfd_encode_tlv (pnt, size, i,msg->ds_len*4);
                LIST_LOOP (msg->ds_label, shim, node)
                  {
                    TLV_ENCODE_PUT (shim, sizeof (struct shimhdr));
                  }
                break;
              case OAM_CTYPE_MSG_MPLSONM_TRACE_LAST_MSG:
                bfd_encode_tlv (pnt, size, i , 1);
                TLV_ENCODE_PUTC (msg->last);
                break;
              case OAM_CTYPE_MSG_MPLSONM_TRACE_RECVCOUNT :
                bfd_encode_tlv (pnt, size, i,4);
                TLV_ENCODE_PUTL (msg->recv_count);
                break; 
              case OAM_CTYPE_MSG_MPLSONM_TRACE_OUTLABEL:
                bfd_encode_tlv (pnt, size, i,4);
                TLV_ENCODE_PUTL (msg->out_label);
                break;


            }
        }
    }

  return *pnt - sp;
  
}

int
oam_decode_mpls_onm_trace_reply (u_char **pnt, u_int16_t *size,
                                struct oam_msg_mpls_tracert_reply *msg)
{  
  u_char *sp = *pnt;
  struct bfd_tlv_header tlv;
  struct shimhdr *shim;
  int cnt;

  /* Check size.  */
  if (*size < OAM_MSG_MPLS_ONM_SIZE)
    return BFD_ERR_PKT_TOO_SMALL;

  /* TLV parse.  */
  while (*size)
    {
      if (*size < BFD_TLV_HEADER_SIZE)
        return BFD_ERR_PKT_TOO_SMALL;

      MSG_DECODE_TLV_HEADER (tlv);

      switch (tlv.type)
        {  
           case OAM_CTYPE_MSG_MPLSONM_TRACE_REPLYADDR :
              TLV_DECODE_GET_IN4_ADDR (&msg->reply);
              MSG_SET_CTYPE (msg->cindex, tlv.type);
              break;
           case OAM_CTYPE_MSG_MPLSONM_TRACE_SENTTIME:
              TLV_DECODE_GETL (msg->sent_time_sec);
              TLV_DECODE_GETL (msg->sent_time_usec);
              MSG_SET_CTYPE (msg->cindex, tlv.type);
             break;
           case OAM_CTYPE_MSG_MPLSONM_TRACE_RECVTIME: 
              TLV_DECODE_GETL (msg->recv_time_sec);
              TLV_DECODE_GETL (msg->recv_time_usec);
              MSG_SET_CTYPE (msg->cindex, tlv.type);
             break;
           case OAM_CTYPE_MSG_MPLSONM_TRACE_TTL: 
             TLV_DECODE_GETC (msg->ttl);
             MSG_SET_CTYPE (msg->cindex, tlv.type);
             break;
           case OAM_CTYPE_MSG_MPLSONM_TRACE_RETCODE: 
             TLV_DECODE_GETC (msg->ret_code);
             MSG_SET_CTYPE (msg->cindex, tlv.type);
             break;
           case OAM_CTYPE_MSG_MPLSONM_TRACE_DSMAP :
             msg->ds_label = list_new();
             for (cnt = 0; cnt < tlv.length/4; cnt++)
               {
                 shim = XMALLOC (MTYPE_TMP, sizeof (struct shimhdr));
                 DECODE_GET (shim, sizeof (struct shimhdr));
                 listnode_add (msg->ds_label, shim);
               }
             MSG_SET_CTYPE (msg->cindex, tlv.type);
             msg->ds_len = tlv.length/4;
             break;
           case OAM_CTYPE_MSG_MPLSONM_TRACE_LAST_MSG:
             TLV_DECODE_GETC (msg->last);
             MSG_SET_CTYPE (msg->cindex, tlv.type);
             break;
           case OAM_CTYPE_MSG_MPLSONM_TRACE_RECVCOUNT: 
              TLV_DECODE_GETL (msg->recv_count);
              MSG_SET_CTYPE (msg->cindex, tlv.type);
             break;
           case OAM_CTYPE_MSG_MPLSONM_TRACE_OUTLABEL: 
              TLV_DECODE_GETL (msg->out_label);
              MSG_SET_CTYPE (msg->cindex, tlv.type);
             break;
           default:
             TLV_DECODE_SKIP (tlv.length);
             break;
        }
    }
    
  return *pnt - sp;
}

int
oam_encode_mpls_onm_error (u_char **pnt, u_int16_t *size,
                           struct oam_msg_mpls_ping_error *msg)
{
  u_char *sp = *pnt;
  int i;
  /* Check size.  */
  if (*size < OAM_MSG_MPLS_ONM_ERR_SIZE)
    return BFD_ERR_PKT_TOO_SMALL;

  /* MPLS ONM TLV parse. */
  for (i = 0; i < BFD_CINDEX_SIZE; i++)
    {
      if (MSG_CHECK_CTYPE (msg->cindex, i))
        {
          if (*size < BFD_TLV_HEADER_SIZE)
            return BFD_ERR_PKT_TOO_SMALL;

          switch (i)
            {    
              case OAM_CTYPE_MSG_MPLSONM_ERROR_MSGTYPE :
                bfd_encode_tlv (pnt, size, i,1);
                TLV_ENCODE_PUTC (msg->msg_type);
                break;
              case OAM_CTYPE_MSG_MPLSONM_ERROR_TYPE:
                bfd_encode_tlv (pnt, size, i,1);
                TLV_ENCODE_PUTC (msg->type);
                break;
              case OAM_CTYPE_MSG_MPLSONM_ERROR_VPNID: 
                bfd_encode_tlv (pnt, size, i,4);
                TLV_ENCODE_PUTL (msg->vpn_id);
                break;
              case OAM_CTYPE_MSG_MPLSONM_ERROR_FEC:
                bfd_encode_tlv (pnt, size, i,4);
                TLV_ENCODE_PUT_IN4_ADDR (&msg->fec);
                break;  
              case OAM_CTYPE_MSG_MPLSONM_ERROR_VPN_PEER: 
                bfd_encode_tlv (pnt, size, i,4);
                TLV_ENCODE_PUT_IN4_ADDR (&msg->vpn_peer);
                break;
              case OAM_CTYPE_MSG_MPLSONM_ERROR_SENTTIME: 
                bfd_encode_tlv (pnt, size, i,4);
                TLV_ENCODE_PUTL (msg->sent_time);
                break;
              case OAM_CTYPE_MSG_MPLSONM_ERROR_RETCODE: 
                bfd_encode_tlv (pnt, size, i,4);
                TLV_ENCODE_PUTL (msg->ret_code);
               break;
              case OAM_CTYPE_MSG_MPLSONM_ERROR_RECVCOUNT: 
                bfd_encode_tlv (pnt, size, i,4);
                TLV_ENCODE_PUTL (msg->recv_count);
               break;

            }
        }
    }

  return *pnt - sp;
}

int
oam_decode_mpls_onm_error (u_char **pnt, u_int16_t *size,
                           struct oam_msg_mpls_ping_error *msg)
{  
  u_char *sp = *pnt;
  struct bfd_tlv_header tlv;

  /* Check size.  */
  if (*size < OAM_MSG_MPLS_ONM_ERR_SIZE)
    return BFD_ERR_PKT_TOO_SMALL;

  /* TLV parse.  */
  while (*size)
    {
      if (*size < BFD_TLV_HEADER_SIZE)
        return BFD_ERR_PKT_TOO_SMALL;

      MSG_DECODE_TLV_HEADER (tlv);

      switch (tlv.type)
        {   
           case OAM_CTYPE_MSG_MPLSONM_ERROR_MSGTYPE :
              TLV_DECODE_GETC (msg->msg_type);
              MSG_SET_CTYPE (msg->cindex, tlv.type);
              break;
           case OAM_CTYPE_MSG_MPLSONM_ERROR_TYPE:
              TLV_DECODE_GETC (msg->type);
              MSG_SET_CTYPE (msg->cindex, tlv.type);
              break;
           case OAM_CTYPE_MSG_MPLSONM_ERROR_VPNID: 
              TLV_DECODE_GETC (msg->vpn_id);
              MSG_SET_CTYPE (msg->cindex, tlv.type);
              break;
           case OAM_CTYPE_MSG_MPLSONM_ERROR_FEC:
              TLV_DECODE_GET_IN4_ADDR (&msg->fec);
              MSG_SET_CTYPE (msg->cindex, tlv.type);
              break;    
           case OAM_CTYPE_MSG_MPLSONM_ERROR_VPN_PEER: 
              TLV_DECODE_GET_IN4_ADDR (&msg->vpn_peer);
              MSG_SET_CTYPE (msg->cindex, tlv.type);
              break;
           case OAM_CTYPE_MSG_MPLSONM_ERROR_SENTTIME: 
              TLV_DECODE_GETL (msg->sent_time);
              MSG_SET_CTYPE (msg->cindex, tlv.type);
              break;
           case OAM_CTYPE_MSG_MPLSONM_ERROR_RETCODE: 
              TLV_DECODE_GETL (msg->ret_code);
              MSG_SET_CTYPE (msg->cindex, tlv.type);
              break;
           case OAM_CTYPE_MSG_MPLSONM_ERROR_RECVCOUNT: 
              TLV_DECODE_GETL (msg->recv_count);
              MSG_SET_CTYPE (msg->cindex, tlv.type);
              break;
           default:
             TLV_DECODE_SKIP (tlv.length);
             break;
        }
    }
    
  return *pnt - sp;
}

/* Parse MPLS OAM PING Reply. */
int
oam_parse_ping_response (u_char **pnt, u_int16_t *size,
                         struct bfd_msg_header *header, void *arg,
                         BFD_CALLBACK callback)
{
  struct oam_msg_mpls_ping_reply msg;
  int ret;

  pal_mem_set (&msg, 0, sizeof (struct oam_msg_mpls_ping_reply));
  ret = oam_decode_mpls_onm_ping_reply (pnt, size, &msg);
  if (ret < 0)
    return ret;

  if (callback)
    {
      ret = (*callback) (header, arg, &msg);
      if (ret < 0)
        return ret;
    }

  return 0;
}

/* Parse MPLS OAM Tracert Reply. */
int
oam_parse_tracert_response (u_char **pnt, u_int16_t *size,
                            struct bfd_msg_header *header, void *arg,
                            BFD_CALLBACK callback)
{
  struct oam_msg_mpls_tracert_reply msg;
  int ret;

  pal_mem_set (&msg, 0, sizeof (struct oam_msg_mpls_tracert_reply));
  ret = oam_decode_mpls_onm_trace_reply (pnt, size, &msg);
  if (ret < 0)
    return ret;

  if (callback)
    {
      ret = (*callback) (header, arg, &msg);
      if (ret < 0)
        return ret;
    }

  return 0;
}

int
oam_parse_error_response (u_char **pnt, u_int16_t *size,
                          struct bfd_msg_header *header, void *arg,
                          BFD_CALLBACK callback)
{
  struct oam_msg_mpls_ping_error msg;
  int ret;

  pal_mem_set(&msg, 0, sizeof (struct oam_msg_mpls_ping_error));
  ret = oam_decode_mpls_onm_error (pnt, size, &msg);
  if (ret < 0)
    return ret;

  if (callback)
    {
      ret = (*callback) (header, arg, &msg);
      if (ret < 0)
        return ret;
    }

  return 0;
}
#endif /* HAVE_MPLS_OAM */
#endif /* HAVE_BFD  || HAVE_MPLS_OAM */
