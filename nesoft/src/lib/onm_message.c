/**@file onm_message.c
 ** @brief  This file contains ONM message implementation.  
 *  This implementation is used by both server and client side.
 */
/* Copyright (C) 2010  All Rights Reserved. */

#include "pal.h"
#include "network.h"
#include "nexthop.h"
#include "message.h"
#include "onm_message.h"
#include "log.h"
#include "tlv.h"
#include "onm_client.h"

int
onm_decode_service (u_char **pnt, u_int16_t *size, 
                    struct onm_msg_service *msg);

int
onm_encode_tlv (u_char **pnt, u_int16_t *size, u_int16_t type,
                u_int16_t length);

/* ONM message strings.  */
static const char *onm_msg_str[] =
{
  "Service Request",                             /* 0 */
  "Service Reply",                               /* 1 */
  "EVC Status"                                   /* 2 */
};


/* ONM service strings.  */
static const char *onm_service_str[] =
{
  "Interface Service",                    /* 0 */
  "EVC Service",                          /* 1 */
};

/* ONM message to string.  */
const char *
onm_msg_to_str (int type)
{
  if (type < ONM_MSG_MAX)
    return onm_msg_str [type];

  return "Unknown";
}

/* ONM service to string.  */
const char *
onm_service_to_str (int service)
{
  if (service < ONM_SERVICE_MAX)
    return onm_service_str [service];

  return "Unknown Service";
}

void
onm_service_dump (struct lib_globals *zg, struct onm_msg_service *service)
{
  int i;

  zlog_info (zg, "ONM Service");
  zlog_info (zg, " Version: %d", service->version);
  zlog_info (zg, " Protocol ID: %s (%d)", modname_strl (service->protocol_id),
             service->protocol_id);
  zlog_info (zg, " Client ID: %d", service->client_id);
  zlog_info (zg, " Bits: %d", service->bits);

  for (i = 0; i < ONM_CINDEX_SIZE; i++)
    {
      if (ONM_CHECK_CTYPE (service->bits, i))
        zlog_info (zg, "  %s", onm_service_to_str (i));
    }

}

void
onm_evc_dump (struct lib_globals *zg, struct onm_msg_evc_status *msg)
{
  zlog_info (zg, "ONM EVC");
  zlog_info (zg, " EVC bridge Id: %s", msg->br_name);
  zlog_info (zg, " EVC Id: %d", msg->evc_id);
  zlog_info (zg, " Status: 0x%08x", msg->evc_status);
}

void
onm_header_dump (struct lib_globals *zg, struct onm_api_msg_header *msg)
{
  zlog_info (zg, "ONM EVC");
}

int
onm_encode_evc_status_msg (u_char **pnt, u_int16_t *size, 
                           struct onm_msg_evc_status *msg)
{
  u_char *sp = *pnt;

  /* Check size.  */
  if (*size < ONM_MSG_EVC_SIZE)
    return ONM_ERR_PKT_TOO_SMALL;

  /* Interface index.  */
  TLV_ENCODE_PUT (msg->br_name, ONM_BRIDGE_NAMSIZ);

  /* Put the EVC id */
  TLV_ENCODE_PUTW (msg->evc_id);
    
  /* Put the EVC Operational Status */
  TLV_ENCODE_PUTC (msg->evc_status);
  
  return *pnt - sp;

}

int
onm_decode_evc_status_msg (u_char **pnt, u_int16_t *size,
                           struct onm_msg_evc_status *msg)
{
  u_char *sp = *pnt;

  /* Check size.  */
  if (*size < ONM_MSG_EVC_SIZE)
    return ONM_ERR_PKT_TOO_SMALL;

  /* Bridge name. */
   TLV_DECODE_GET (msg->br_name, ONM_BRIDGE_NAMSIZ);

  /* Get the EVC ID */
  TLV_DECODE_GETW (msg->evc_id);

  /* Get the EVC Operational Status */
  TLV_DECODE_GETC (msg->evc_status);

  return *pnt - sp;

}

int
onm_parse_evc_status_msg (u_char **pnt, u_int16_t *size,
                          struct onm_api_msg_header *header,
                          void *arg, ONM_CALLBACK callback)

{
  struct onm_msg_evc_status msg;
  int ret = 0;

  pal_mem_set (&msg, 0, sizeof (struct onm_msg_evc_status));

  ret = onm_decode_evc_status_msg (pnt, size, &msg);
  if (ret < 0)
    return ret;

  if (callback)
    {
      ret = (*callback) (header, arg, &msg);
      if (ret < 0)
        return ret;
    }

  return RESULT_OK;

}

/* ONM message header encode.  */
int
onm_encode_header (u_char **pnt, u_int16_t *size,
                   struct onm_api_msg_header *header)
{
  u_char *sp = *pnt;

  if (*size < ONM_MSG_HEADER_SIZE)
    return ONM_ERR_PKT_TOO_SMALL;

  TLV_ENCODE_PUTL (header->vr_id);
  TLV_ENCODE_PUTW (header->type);
  TLV_ENCODE_PUTW (header->length);
  TLV_ENCODE_PUTL (header->message_id);

  return *pnt - sp;
}

/* ONM message header decode.  */
int
onm_decode_header (u_char **pnt, u_int16_t *size,
                   struct onm_api_msg_header *header)
{
  if (*size < ONM_MSG_HEADER_SIZE)
    return ONM_ERR_PKT_TOO_SMALL;

  TLV_DECODE_GETL (header->vr_id);
  TLV_DECODE_GETW (header->type);
  TLV_DECODE_GETW (header->length);
  TLV_DECODE_GETL (header->message_id);

  return ONM_MSG_HEADER_SIZE;
}

int
onm_encode_tlv (u_char **pnt, u_int16_t *size, u_int16_t type,
                u_int16_t length)
{
  length += ONM_TLV_HEADER_SIZE;

  TLV_ENCODE_PUTW (type);
  TLV_ENCODE_PUTW (length);

  return ONM_TLV_HEADER_SIZE;
}

/* All message parser.  These parser are used by both server and
*    client side. */

/* Parse ONM service message.  */
int
onm_parse_service (u_char **pnt, u_int16_t *size,
                   struct onm_api_msg_header *header, void *arg,
                   ONM_CALLBACK callback)
{
  int ret;
  struct onm_msg_service msg;

  pal_mem_set (&msg, 0, sizeof (struct onm_msg_service));
   
  /* Parse service.  */
  ret = onm_decode_service (pnt, size, &msg);
  if (ret < 0)
    return ret;

  /* Call callback with arg. */
  if (callback)
    {
      ret = (*callback) (header, arg, &msg);
      if (ret < 0)
        return ret;
    }
  return RESULT_OK;
}


/* ONM service encode.  */
int
onm_encode_service (u_char **pnt, u_int16_t *size, struct onm_msg_service *msg)
{
  u_char *sp = *pnt;

  if (*size < ONM_MSG_SERVICE_SIZE)
    return ONM_ERR_PKT_TOO_SMALL;

  TLV_ENCODE_PUTW (msg->version);
  TLV_ENCODE_PUTW (0);
  TLV_ENCODE_PUTL (msg->protocol_id);
  TLV_ENCODE_PUTL (msg->client_id);
  TLV_ENCODE_PUTL (msg->bits);

  return *pnt - sp;
}

/* ONM service decode.  */
int
onm_decode_service (u_char **pnt, u_int16_t *size, struct onm_msg_service *msg)
{
  u_char *sp = *pnt;
  struct onm_tlv_header tlv;

  pal_mem_set (&tlv, 0, sizeof(struct onm_tlv_header));

  if (*size < ONM_MSG_SERVICE_SIZE)
    return ONM_ERR_PKT_TOO_SMALL;

  TLV_DECODE_GETW (msg->version);
  TLV_DECODE_GETW (msg->reserved);
  TLV_DECODE_GETL (msg->protocol_id);
  TLV_DECODE_GETL (msg->client_id);
  TLV_DECODE_GETL (msg->bits);

  /* Optional TLV parser.  */
  while (*size)
    {
      if (*size < ONM_TLV_HEADER_SIZE)
        return ONM_ERR_PKT_TOO_SMALL;

      ONM_DECODE_TLV_HEADER (tlv);
      switch (tlv.type)
        {
        default:
          TLV_DECODE_SKIP (tlv.length);
          break;
        }
    }

  return *pnt - sp;
}

