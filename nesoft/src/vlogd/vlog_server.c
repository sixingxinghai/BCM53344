/*=============================================================================
**
** Copyright (C) 2008-2009   All Rights Reserved.
**
** vlog_api.c - Implementation of teh VLOG server.
*/


#include "vlogd.h"
#include "network.h"
#include "vlog_server.h"
#include "vlog_client.h"

extern VLOG_GLOBALS *vlog_vgb;


/*------------------------------------------------------------------------
 * VLOG_SRV_WALK_CTRL - VLOG server walk control parameters
 *
 * Purpose: To store the vlog server user parameters and pass them as a single
 *          reference to the vector walk function.
 *          Used by vlog_server_walk_clients.
 */
typedef struct vlog_server_walk_ctrl
{
  VLOG_SERVER_WALK_CB vsw_user_cb;
  intptr_t            vsw_user_ref;
} VLOG_SRV_WALK_CTRL;

/* Here we create client unaware about its identification.
 */

VLOG_SRV_ENTRY *
_vlog_server_new_client(VLOG_SERVER *vsv,
                        struct message_entry *me)
{
  VLOG_SRV_ENTRY *vse;

  vse = XCALLOC (MTYPE_VLOG_SRV_ENTRY, sizeof (VLOG_SRV_ENTRY));
  if (vse == NULL)
    return (NULL);
  vse->vse_me     = me;
  vse->vse_server = vsv;
  vse->vse_recv_msg_count = 0;
  vse->vse_connect_time = pal_time_sys_current (NULL);
  return (vse);
}

void
_vlog_server_delete_client(VLOG_SERVER *vsv, VLOG_SRV_ENTRY *vse)
{
  if (vse->vse_mod_id != 0)
    vector_unset (vsv->vsv_clt_vect, vse->vse_mod_id);

  vse->vse_me->info = NULL;
  XFREE (MTYPE_VLOG_SRV_ENTRY, vse);
}

/* Here we create client once we know its identification.
 */

static VLOG_SRV_ENTRY *
_vlog_server_get_client(VLOG_SERVER *vsv,
                        struct message_entry *me,
                        u_int16_t mod_id)
{
  VLOG_SRV_ENTRY *vse = NULL;

  vse = vector_lookup_index(vsv->vsv_clt_vect, mod_id);
  if (vse == NULL)
  {
    vse = _vlog_server_new_client (vsv, me);
    if (vse == NULL)
      return (NULL);
    vse->vse_mod_id = mod_id;
    vector_set_index (vsv->vsv_clt_vect, mod_id, vse);
  }
  return (vse);
}

/* Client connect to VLOG server.
   Here we create the VLOG_SRV_ENTRY but we do not install it
   as we do not know the module id yet.
   Once we receive teh first message, we will add the entry to the
   vector of clients.
 */
int
vlog_server_connect (struct message_handler *ms,
                     struct message_entry   *me,
                     pal_sock_handle_t       sock)
{
  struct vlog_server *vsv = ms->info;
  struct vlog_server_entry *vse = NULL;

  if (me->info == NULL)
  {
    /* Create new client entry. */
    vse = _vlog_server_new_client(vsv, me);
    if (vse == NULL)
      return ZRES_ERR;
  }
  /* Attach the new entry to the message entry. */
  me->info        = vse;

  /* Once the first msg is received we will do more. */
  return (ZRES_OK);
}

/* Client disconnect from VLOG server. */
int
vlog_server_disconnect (struct message_handler *ms,
                        struct message_entry   *me,
                        pal_sock_handle_t       sock)
{
  struct vlog_server *vsv = ms->info;
  struct vlog_server_entry *vse = me->info;

  if (vse != NULL)
    _vlog_server_delete_client(vsv, vse);
  return (ZRES_OK);
}

#if 0
/* Reception of VLOG message over UDP socket.
   This is not in use yet.
*/
int
vlog_server_read_msg_udp (struct message_handler *ms,
                          struct message_entry   *me,
                          pal_sock_handle_t sock)
{
  VLOG_SERVER *vsv = vlog_vgb->vgb_server;
  VLOG_SRV_ENTRY *vse = me->info;
  static struct vlog_msg msg;
  struct vlog_msg_header *hdr = NULL;
  struct apn_vr *vr;
  int nbytes = 0;

  nbytes = pal_sock_recvfrom (ms->sock,
                              (void *)&msg, VLOG_MSG_MAX_SIZE-1,
                              MSG_TRUNC,
                              NULL, NULL);
  if (nbytes < VLOG_MSG_HDR_SIZE)
    return (-1);

  hdr = &msg.vms_msg_hdr;

  if (hdr->vmh_mod_id >= APN_PROTO_MAX)
    return (-1);

  /* Install fully the client if it is not done yet. */
  if (vse == NULL)
  {
    vse = _vlog_server_get_client(vsv, me, hdr->vmh_mod_id);
    me->info = vse;
  }
  else if (vse->vse_mod_id == 0)
  {
    vector_set_index(vsv->vsv_clt_vect, hdr->vmh_mod_id, vse);
    vse->vse_mod_id =  hdr->vmh_mod_id;
  }
  vse->vse_recv_msg_count++;
  vse->vse_read_time = pal_time_sys_current (NULL);

  /* Terminate the data string. */
  msg.vms_msg_data[nbytes - VLOG_MSG_HDR_SIZE] = 0;

  /* No VR - no forwarding. */
  vr = apn_vr_get_by_id (ms->zg, hdr->vmh_vr_id);
  if (vr == NULL)
    return nbytes;

  switch (hdr->vmh_msg_type)
  {
  case VLOG_MSG_TYPE_DATA:
    vlog_vr_forward_log_msg(vr->proto,
                            hdr->vmh_mod_id,
                            hdr->vmh_proc_id,
                            hdr->vmh_priority,
                            msg.vms_msg_data);
    break;

  case VLOG_MSG_TYPE_CTRL:
    /* Whatever we wanted from this message we have already taken. */
    break;

  default:
    return (-1);
  }
  return nbytes;
}
#endif

/* Reception of VLOG message over Unix socket. */
int
vlog_server_read_msg (struct message_handler *ms,
                      struct message_entry *me,
                      pal_sock_handle_t sock)
{
  int nbytes;

  static struct vlog_msg msg;

  struct vlog_server *vsv = NULL ;
  struct vlog_server_entry *vse = NULL;
  struct vlog_msg_header *hdr = NULL;
  struct apn_vr *vr;

  /* Get VLOG server entry from message entry. */
  vsv = ms->info;
  vse = me->info;

  hdr = &msg.vms_msg_hdr;

  nbytes = readn (sock, (u_char *)hdr, VLOG_MSG_HDR_SIZE);

  if (nbytes < VLOG_MSG_HDR_SIZE)
    return (-1);

  /* Install fully the client if it has not been done yet. */
  if (vse == NULL)
  {
    vse = _vlog_server_get_client(vsv, me, hdr->vmh_mod_id);
    me->info = vse;
  }
  else if (vse->vse_mod_id == 0)
  {
    vector_set_index(vsv->vsv_clt_vect, hdr->vmh_mod_id, vse);
    vse->vse_mod_id =  hdr->vmh_mod_id;
  }
  vse->vse_recv_msg_count++;
  vse->vse_read_time = pal_time_sys_current (NULL);
  switch (hdr->vmh_msg_type)
  {
  case VLOG_MSG_TYPE_DATA:

    /* Check the data length is in bounds. */
    if (hdr->vmh_data_len >= VLOG_MSG_MAX_DATA_SIZE-1)
      return (-1);

    nbytes = readn (sock, msg.vms_msg_data, hdr->vmh_data_len);
    if (nbytes != hdr->vmh_data_len)
      return (-1);

    /* Terminate the string with \0 */
    msg.vms_msg_data[hdr->vmh_data_len] = 0;

    vr = apn_vr_get_by_id (ms->zg, hdr->vmh_vr_id);
    if (vr != NULL)
    {
      vlog_vr_forward_log_msg(vr->proto,
                              hdr->vmh_mod_id,
                              hdr->vmh_proc_id,
                              hdr->vmh_priority,
                              msg.vms_msg_data);
    }
    return (nbytes);

    case VLOG_MSG_TYPE_CTRL:
      nbytes = 0;
    /* Whatever we wanted from this message we have already taken. */
    break;

  default:
    return -1;
    break;
  }
  return (VLOG_MSG_HDR_SIZE + nbytes);
}

ZRESULT
_vlog_server_entry_finish(void *vse, intptr_t vsv)
{
  _vlog_server_delete_client((VLOG_SERVER *)vsv, (VLOG_SRV_ENTRY *)vse);
  return (ZRES_OK);
}

ZRESULT
vlog_server_finish (VLOG_SERVER *vsv)
{
  /* Clean up the VLOG server clients.  */
  vector_walk(vsv->vsv_clt_vect, _vlog_server_entry_finish, (intptr_t)vsv);
  vector_free(vsv->vsv_clt_vect);
  message_server_delete (vsv->vsv_ms);
  XFREE (MTYPE_VLOG_SRV, vsv);
  return (ZRES_OK);
}

struct vlog_server *
vlog_server_init (struct lib_globals *zg)
{
  struct vlog_server *vsv = NULL;
  struct message_handler *ms = NULL;
  int ret = 0;

  /* Create message server.  */
  ms = message_server_create (zg);
  if (! ms)
    return (NULL);

#ifdef HAVE_TCP_MESSAGE
  message_server_set_style_tcp (ms, VLOG_PORT);
#else
  /* Set server type to UNIX domain socket.  */
  message_server_set_style_domain (ms, VLOG_SERV_PATH);
#endif /* HAVE_TCP_MESSAGE */

  /* Set call back functions.  */
  message_server_set_callback (ms, MESSAGE_EVENT_CONNECT,
                               vlog_server_connect);
  message_server_set_callback (ms, MESSAGE_EVENT_READ_MESSAGE,
                               vlog_server_read_msg);
  message_server_set_callback (ms, MESSAGE_EVENT_DISCONNECT,
                               vlog_server_disconnect);
  /* Start VLOG server.  */
  ret = message_server_start (ms);

  if (ret < 0)
  {
    message_server_delete(ms);
    return NULL;
  }

  /* When message server works fine, go forward to create VLOG server
     structure.  */
  vsv = XCALLOC (MTYPE_VLOG_SRV, sizeof (struct vlog_server));
  vsv->vsv_zg = zg;
  vsv->vsv_ms = ms;
  vsv->vsv_clt_vect = vector_init (APN_PROTO_MAX+1);
  ms->info = vsv;
  return (vsv);
}


/*-----------------------------------------------------------------
 * Support for "show vlog clients" command.
 *
 *-----------------------------------------------------------------
 */
ZRESULT
_vlog_server_walk_cb(void *val, intptr_t srv_ref)
{
  VLOG_SRV_ENTRY *vse = val;
  VLOG_SRV_WALK_CTRL *srv_ctrl = (VLOG_SRV_WALK_CTRL *)srv_ref;

  return srv_ctrl->vsw_user_cb(vse, srv_ctrl->vsw_user_ref);
}

ZRESULT
vlog_server_walk_clients(VLOG_SERVER *vsv,
                         VLOG_SERVER_WALK_CB user_cb,
                         intptr_t     user_ref)
{
  VLOG_SRV_WALK_CTRL walk_ctrl;

  walk_ctrl.vsw_user_cb = user_cb;
  walk_ctrl.vsw_user_ref= user_ref;

  return vector_walk(vsv->vsv_clt_vect,
                     _vlog_server_walk_cb,
                     (intptr_t)&walk_ctrl);
}

