/**@file onm_message.h
 ** @brief  This file contains data structures and constants for
 * ONM message implementation. This implementation is used by 
 * both server and client side.
 **/
/* Copyright (C) 2010  All Rights Reserved. */

#ifndef _ONM_MESSAGE_H
#define _ONM_MESSAGE_H

/* ONM protocol definition.  */
#define APN_PROTO_ONMSERVER                  1
#define APN_PROTO_ONMCLIENT                  2

#define ONM_ERR_PKT_TOO_SMALL               -1
#define ONM_ERR_INVALID_SERVICE             -2
#define ONM_ERR_INVALID_PKT                 -3
#define ONM_ERR_SYSTEM_FAILURE              -4
#define ONM_ERR_INVALID_AFI                 -5

#define LIB_ERR_INVALID_TLV                 -100

/* ONM protocol version is 1.  */
#define ONM_PROTOCOL_VERSION_1               1
#define ONM_PROTOCOL_VERSION_2               2 

/* ONM message max length.  */
#define ONM_MSG_MAX_LEN                     4096

/* CFM events.  */
#define ONM_EVENT_CONNECT                    0
#define ONM_EVENT_DISCONNECT                 1

#define ONM_SERVICE_INTERFACE                0
#define ONM_SERVICE_EVC                      1
#define ONM_SERVICE_MAX                      2

#define ONM_ERR_MSG_TYPE_UNKNOWN             -1
#define ONM_ERR_INVALID_TLV        -33
#define ONM_ERR_NO_ROUTER_ID       -34
#define ONM_ERR_PKT_DECODE         -35
#define ONM_ERR_INVALID_MSG_LENGTH -36
#define ONM_ERR_CONNECT_FAILURE    -37
#define ONM_ERR_MEM_ALLOC_FAILURE  -38

#define ONM_MSG_SERVICE_REQUEST              0
#define ONM_MSG_SERVICE_REPLY                1
#define ONM_MSG_EVC_STATUS                   2
#define ONM_MSG_MAX                          3

#define ONM_CHECK_CTYPE(F,C)        (CHECK_FLAG (F, (1 << C)))
#define ONM_SET_CTYPE(F,C)          (SET_FLAG (F, (1 << C)))
#define ONM_UNSET_CTYPE(F,C)        (UNSET_FLAG (F, (1 << C)))

#define ONM_CINDEX_SIZE              MSG_CINDEX_SIZE

#define ONM_DECODE_TLV_HEADER(TH)                                             \
      do {                                                                    \
                TLV_DECODE_GETW ((TH).type);                                  \
                TLV_DECODE_GETW ((TH).length);                                \
                (TH).length -= ONM_TLV_HEADER_SIZE;                           \
              } while (0)

struct onm_api_msg_header 
{

  /* VR-ID. */
  u_int32_t vr_id;

  /* Message Type. */
  u_int16_t type;

  /* Message Len. */
  u_int16_t length;

  /* Message ID. */
  u_int32_t message_id;
};

struct onm_api_tlv_header
{
      u_int16_t type;
      u_int16_t length;
};

struct onm_api_tlv_status
{
  struct onm_api_tlv_header tlv_h;
  u_int32_t status;
  u_char opt;
};

/* ONM services structure.  */
struct onm_msg_service
{
    /* TLV flags. */
    cindex_t cindex;

    /* ONM Protocol Version. */
    u_int16_t version;

    /* Reserved. */
    u_int16_t reserved;

    /* Protocol ID. */
    u_int32_t protocol_id;

    /* Client Id. */
    u_int32_t client_id;

    /* Service Bits. */
    u_int32_t bits;

};
#define ONM_MSG_SERVICE_SIZE     16

#define ONM_MSG_EVC_SIZE          4

/* Maximum bridge name length. */
#define ONM_BRIDGE_NAMSIZ                    16

struct onm_msg_evc_status
{
  u_int8_t br_name[ONM_BRIDGE_NAMSIZ+1];
  u_int16_t evc_id;
  u_int8_t evc_status; 
};


/* ONM message send queue.  */
struct onm_message_queue
{
  struct onm_message_queue *next;
  struct onm_message_queue *prev;

  u_char *buf;
  u_int16_t length;
  u_int16_t written;
};

struct onm_api_message_connect
{
  struct onm_api_msg_header mes_h;
  u_char client_id;
};

/* Send reply for successful ping */
struct onm_elmi_msg_evc_status
{
  /* These values are used for detailed and standard output */
  u_int32_t evc_id;
  u_int32_t status;
  
};

struct onm_tlv_header
{
  u_int16_t type;
  u_int16_t length;
};

#define ONM_TLV_HEADER_SIZE   4
#define ONM_MSG_HEADER_SIZE   12

typedef void (* onm_msg_commsg_recv_cb_t)(void   *user_ref,
    u_int16_t  src_mod_id, 
    u_char    *buf, 
    u_int16_t  len);
/* end of ONM COMMSG specific stuff ... */


/* ONM callback function typedef.  */
typedef int (*ONM_CALLBACK) (struct onm_api_msg_header *, void *, void *);
typedef int (*ONM_DISCONNECT_CALLBACK) ();

typedef int (*ONM_PARSER) (u_char **, u_int16_t *, struct onm_api_msg_header *,
             void *, ONM_CALLBACK);

const char *onm_service_to_str (int);
const char *onm_msg_to_str (int);

int onm_encode_service (u_char **, u_int16_t *, struct onm_msg_service *);
int onm_encode_link (u_char **, u_int16_t *, struct onm_msg_evc_status *);

int onm_parse_service (u_char **, u_int16_t *, struct onm_api_msg_header *,
                           void *, ONM_CALLBACK);

int
onm_encode_evc_status_msg (u_char **pnt, u_int16_t *size,
                           struct onm_msg_evc_status *msg);
int
onm_decode_evc_status_msg (u_char **pnt, u_int16_t *size,
                           struct onm_msg_evc_status *msg);
int
onm_parse_evc_status_msg (u_char **pnt, u_int16_t *size,
                          struct onm_api_msg_header *header,
                          void *arg, ONM_CALLBACK callback);

void 
onm_header_dump (struct lib_globals *, struct onm_api_msg_header *);
void 
onm_interface_dump (struct lib_globals *, struct onm_msg_evc_status *);
void 
onm_evc_dump (struct lib_globals *, struct onm_msg_evc_status *);
void 
onm_service_dump (struct lib_globals *, struct onm_msg_service *);
int 
onm_decode_header (u_char **, u_int16_t *, struct onm_api_msg_header *);
int 
onm_encode_header (u_char **, u_int16_t *, struct onm_api_msg_header *);
#endif /* _ONM_MESSAGE_H */
