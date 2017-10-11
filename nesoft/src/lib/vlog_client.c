/*=============================================================================
**
** Copyright (C) 2008-2009   All Rights Reserved.
**
** vlog_vr.c - Implementation of the VLOG_CLIENT object.
*/

#include "pal.h"
#include "lib.h"
#include "log.h"
#include "network.h"
#include "pal_log.h"

#ifdef HAVE_VLOGD

#include "thread.h"
#include "message.h"
#include "vlog_client.h"

struct vlog_client * vlog_client_create (struct lib_globals *);
int vlog_client_send_message (struct vlog_client *vc,
                              struct vlog_msg    *msg);
int vlog_client_start (struct vlog_client *vc);
void vlog_client_stop (struct vlog_client *);


/*-------------------------------------------------------------------
 * vlog_client_send_ctrl_msg - Send a control message to VLOG server
 */
ZRESULT
vlog_client_send_ctrl_msg (struct lib_globals *zg)
{
  struct vlog_msg_header header;

  header.vmh_msg_type = VLOG_MSG_TYPE_CTRL;
  header.vmh_vr_id    = 0;
  header.vmh_mod_id   = zg->protocol;
  header.vmh_proc_id  = pal_get_process_id();
  header.vmh_priority = 0;
  header.vmh_data_len = 0;
  vlog_client_send_message (zg->vlog_clt, (struct vlog_msg *)&header);

  return ZRES_OK;
}

/*-------------------------------------------------------------------
 * vlog_client_send_data_msg - Send a data (i.e. log) message to VLOG server
 */
ZRESULT
vlog_client_send_data_msg (struct lib_globals *zg,
                           s_int8_t priority,
                           char     *buf,
                           u_int16_t len)
{
  struct vlog_msg_header *header;
  struct vlog_msg *msg;

  if (zg->protocol == APN_PROTO_VLOG)
    return ZRES_OK;

  if (zg->vr_in_cxt == NULL)
    return ZRES_ERR;

  msg = XCALLOC (MTYPE_TMP, VLOG_MSG_MAX_SIZE);
  if (msg == NULL)
    return ZRES_ERR;

  header = &msg->vms_msg_hdr;

  header->vmh_msg_type = VLOG_MSG_TYPE_DATA;
  header->vmh_vr_id    = zg->vr_in_cxt->id;
  header->vmh_mod_id   = zg->protocol;
  header->vmh_proc_id   = pal_get_process_id();
  header->vmh_priority = priority;

  if (len > VLOG_MSG_MAX_DATA_SIZE-1)
    len = VLOG_MSG_MAX_DATA_SIZE - 1;

  header->vmh_data_len = len;

  pal_strncpy (msg->vms_msg_data, buf, VLOG_MSG_MAX_DATA_SIZE-1);
  vlog_client_send_message (zg->vlog_clt, msg);

  XFREE (MTYPE_TMP, msg);

  return ZRES_OK;
}

/*-------------------------------------------------------------------
 * vlog_client_send_message - Coomon function to send msg to VLOG server
 */
int
vlog_client_send_message (struct vlog_client *vc,
                          struct vlog_msg    *msg)
{
  int ret;
  u_int16_t msg_len = VLOG_MSG_HDR_SIZE + msg->vms_msg_hdr.vmh_data_len;
  struct message_handler *mc = vc->mc;

  if (! mc || mc->sock < 0)
    return ZRES_ERR;

  ret = writen (mc->sock, (void *)msg, msg_len);

  if (ret < 0)
    /* printf ("socket send error: %s\n", pal_strerror (errno)); */
    return ZRES_ERR;
  else
    return ZRES_OK;
}

/*-------------------------------------------------------------------
 * vlog_client_read_cb - Socket read function callback
 *                       Called by the message layer at the time
 *                       data indication arrived.
 *                       We do not expect anything from VLOG server,
 *                       except an error when the connection is broken.
 */
int
vlog_client_read_cb (struct message_handler *mc,
                     struct message_entry   *me,
                     pal_sock_handle_t       sock)
{
  int nbytes;
  char buf[256];

  nbytes = pal_sock_read (sock, buf, sizeof(buf));

  if (nbytes <= 0)
    return ZRES_ERR;
  else
    return nbytes;
}

/*-------------------------------------------------------------------
 * vlog_client_reconnect_cb - Timer callback to attempt another connection to
 *                            client.
 */
int
vlog_client_reconnect_cb (struct thread *t)
{
  struct vlog_client *vc;

  vc = THREAD_ARG (t);
  vc->t_reconnect = NULL;

  vlog_client_start (vc);

  return ZRES_OK;
}

/*-------------------------------------------------------------------
 * vlog_client_reconnect_start -
 */
int
vlog_client_reconnect_start (struct vlog_client *vc)
{
  if (vc->t_reconnect)
    THREAD_OFF (vc->t_reconnect);

  vc->t_reconnect = thread_add_timer (vc->zg,
                                      vlog_client_reconnect_cb,
                                      vc,
                                      vc->reconnect_interval);
  if (! vc->t_reconnect)
    return ZRES_ERR;
  else
    return ZRES_OK;
}

/*-------------------------------------------------------------------
 * vlog_client_disconnect_cb - Called by the message layer at the
 *                             time server disconnected.
 */
int
vlog_client_disconnect_cb (struct message_handler *mc,
                           struct message_entry *me,
                           pal_sock_handle_t sock)
{
  struct vlog_client *vc = NULL;

  if (mc)
    {
      vc = mc->info;
      if (vc)
        {
          if (vc->t_reconnect)
            THREAD_OFF (vc->t_reconnect);
          message_client_stop (mc);
          vlog_client_reconnect_start (vc);
          return ZRES_OK;
        }
    }
  return ZRES_ERR;
}

/*-------------------------------------------------------------------
 * vlog_client_connect_cb - Called by the message layer at the
 *                          connection to the server has been established.
 *                          Here we register with the server.
 */
int
vlog_client_connect_cb (struct message_handler *mc,
                        struct message_entry *me,
                        pal_sock_handle_t sock)
{
  /* Register read thread.  */
  message_client_read_register (mc);

  /* Send service message to VLOG server.  */
  vlog_client_send_ctrl_msg (mc->zg);

  return ZRES_OK;
}

/*-------------------------------------------------------------------
 * vlog_client_create - Initialize VLOG client.
 *                      This function allocate VLOG client memory.
 */
struct vlog_client *
vlog_client_create (struct lib_globals *zg)
{
  struct vlog_client *vc;

  if (zg->protocol == APN_PROTO_VLOG)
    return ZRES_OK;

  vc = XCALLOC (MTYPE_VLOG_CLIENT, sizeof (struct vlog_client));
  if (! vc)
    return NULL;

  /* Create ac message client. */
  vc->mc = message_client_create (zg, MESSAGE_TYPE_ASYNC);

#ifdef HAVE_TCP_MESSAGE
  message_client_set_style_tcp (vc->mc, VLOG_PORT);
#else /* ! HAVE_TCP_MESSAGE */
  /* Use UNIX domain socket connection.  */
  message_client_set_style_domain (vc->mc, VLOG_SERV_PATH);
#endif /* !HAVE_TCP_MESSAGE */

  /* Initiate connection using LOG connection manager.  */
  message_client_set_callback (vc->mc, MESSAGE_EVENT_CONNECT,
                               vlog_client_connect_cb);
  message_client_set_callback (vc->mc, MESSAGE_EVENT_DISCONNECT,
                               vlog_client_disconnect_cb);
  message_client_set_callback (vc->mc, MESSAGE_EVENT_READ_MESSAGE,
                               vlog_client_read_cb);

  vc->zg = zg;
  zg->vlog_clt = vc;
  vc->reconnect_interval = VLOG_CLT_RECONNECT_INTERVAL;
  vc->mc->info = vc;

  vlog_client_start (vc);

  return vc;
}

int
vlog_client_delete (struct lib_globals *zg)
{
  struct message_handler *mc = zg->imh;
  struct vlog_client *vc;

  if (zg->protocol == APN_PROTO_VLOG)
    return ZRES_OK;

  if (mc == NULL || (vc = mc->info) == NULL)
    return -1;

  /* Cancel reconnect thread.  */
  THREAD_OFF (vc->t_reconnect);

  /* Free the message handler.  */
  message_client_delete (mc);

  XFREE (MTYPE_VLOG_CLIENT, vc);

  zg->vlog_clt = NULL;

  return 0;
}

/*-------------------------------------------------------------------
 * vlog_client_start - Start to connect VLOG services.
 *                     This function always return success.
 */
int
vlog_client_start (struct vlog_client *vc)
{
  int ret;

  ret = message_client_start (vc->mc);

  if (ret < 0)
    {
      /* Start reconnect timer. */
      if (vc->t_reconnect == NULL)
        {
          vc->t_reconnect = thread_add_timer (vc->zg, vlog_client_reconnect_cb,
                                       vc, vc->reconnect_interval);
          if (vc->t_reconnect == NULL)
            return -1;
        }
    }
  return 0;
}

#endif /* HAVE_VLOGD */

