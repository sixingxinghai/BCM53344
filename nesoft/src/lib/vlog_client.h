/* Copyright (C) 2009  All Rights Reserved. */

#ifdef HAVE_VLOGD

#ifndef _PACOS_VLOG_CLIENT_H
#define _PACOS_VLOG_CLIENT_H

#include "lib.h"

/* Message Type. */
#define VLOG_MSG_TYPE_CTRL     1
#define VLOG_MSG_TYPE_DATA     2

/* Message/header size macros. */
#define VLOG_MSG_HDR_SIZE sizeof (struct vlog_msg_header)
#define VLOG_MSG_MAX_SIZE sizeof (struct vlog_msg)
#define VLOG_MSG_MAX_DATA_SIZE ZLOG_BUF_MAXLEN

#define VLOG_CLT_RECONNECT_INTERVAL (5)

/*----------------------------------------------------------------
 * VLOG_MSG_HDR - VLOG message header
 *
 * Members:
 *   vmh_vr_id    - VR id of the VR in context.
 *   vmh_msg_type - Message type (see above)
 *   vmh_mod_id   - Module/protocol id (APN_PROTO_****)
 *   vmh_proc_id  - Process id.
 *   vmh_priority - Message priority (not used)
 *   vmh_data_len - The lenght of data part of teh message.
 */
typedef struct vlog_msg_header
{
  u_int32_t   vmh_vr_id;
  u_int16_t   vmh_msg_type;
  module_id_t vmh_mod_id;
  u_int32_t   vmh_proc_id;
  s_int8_t    vmh_priority;
  u_int32_t   vmh_data_len;
} VLOG_MSG_HDR;

/*----------------------------------------------------------------
 * VLOG_MSG - VLOG message definition
 *
 * Members:
 *  vms_msg_hdr - VLOG message header
 *  vms_msg_data- The message payload - the log message.
 */
typedef struct vlog_msg
{
  VLOG_MSG_HDR vms_msg_hdr;
  char         vms_msg_data[VLOG_MSG_MAX_DATA_SIZE+1];
} VLOG_MSG;

/*----------------------------------------------------------------
 * VLOG_CLT - The VLOG client definition.
 *
 * Members
 *  zg
 *  mc
 *  appl_discon_cb
 *
 */
typedef struct vlog_client
{
  struct lib_globals      *zg;
  struct message_handler  *mc;
  struct thread           *t_reconnect;
  int                      reconnect_interval;
} VLOG_CLT;

struct vlog_client * vlog_client_create (struct lib_globals *);
int vlog_client_delete (struct lib_globals *);
ZRESULT vlog_client_send_data_msg (struct lib_globals *zg, s_int8_t priority,
                                   char *buf, u_int16_t len);
#endif /* _PACOS_VLOG_CLIENT_H */

#endif /* HAVE_VLOGD */
