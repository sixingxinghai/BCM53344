/**@file onm_client.c
 ** @brief  This file contains the ONM client implementation.
 **/
/* Copyright (C) 2010  All Rights Reserved. */

#include "pal.h"

#if defined(HAVE_ONMD) && defined (HAVE_ELMID)

#include "thread.h"
#include "prefix.h"
#include "network.h"
#include "message.h"
#include "log.h"

#include "onm_message.h"
#include "onm_client.h"

/* Specify each ONM service is sync or async.  */
int
onm_client_read_msg (struct message_handler *mc,
                     struct message_entry *me,
                     pal_sock_handle_t sock);

struct
{
    int message;
    int type;
} onm_service_type[] =
{
  {ONM_SERVICE_INTERFACE,  MESSAGE_TYPE_ASYNC},
  {ONM_SERVICE_EVC,        MESSAGE_TYPE_ASYNC}
};

/* Send service message.  */
int
onm_client_send_service (struct onm_client_handler *och)
{
  u_int32_t msg_id = 0;
  u_int16_t size = 0;
  u_char *pnt = NULL;
  int nbytes = 0;
  u_int32_t vr_id = 0;

  if (! och || ! och->up)
    return RESULT_ERROR;

  pnt = och->buf + ONM_MSG_HEADER_SIZE;
  size = och->len - ONM_MSG_HEADER_SIZE;

  nbytes = onm_encode_service (&pnt, &size, &och->service);
  if (nbytes < 0)
    return nbytes;

  return onm_client_send_message (och, vr_id, ONM_MSG_SERVICE_REQUEST, 
                                  nbytes, &msg_id);

}

void
onm_client_set_parser (struct onm_client *oc, int message_type,
                       ONM_PARSER parser)
{
  if (!oc)
    return;

  oc->parser[message_type] = parser;
}

void
onm_client_id_set (struct onm_client *oc, struct onm_msg_service *service)
{
  if (!oc || !service)
    return;

  if (oc->async && oc->async->service.client_id != service->client_id)
    oc->async->service.client_id = service->client_id;
}

void
onm_client_id_unset (struct onm_client *oc)
{
  if (oc && oc->async)
    oc->async->service.client_id = 0;
}

int
onm_client_read (struct onm_client_handler *och, pal_sock_handle_t sock)
{
  int nbytes = 0;
  u_int16_t length = 0;
  struct onm_api_msg_header *header = NULL;

  if (!och)
    return RESULT_ERROR;

  och->size_in = 0;
  och->pnt_in = och->buf_in;
  
  nbytes = readn (sock, och->buf_in, ONM_MSG_HEADER_SIZE);
  if (nbytes < ONM_MSG_HEADER_SIZE)
    return RESULT_ERROR;

  header = (struct onm_api_msg_header *)och->buf_in;
  length = pal_ntoh16 (header->length);

  nbytes = readn (sock, och->buf_in + ONM_MSG_HEADER_SIZE,
                  length - ONM_MSG_HEADER_SIZE);

  if (nbytes <= 0)
    return nbytes;

  och->size_in = length;

  return length;
}

int
onm_client_read_msg (struct message_handler *mc,
                      struct message_entry *me,
                      pal_sock_handle_t sock)
{
  struct onm_client *oc = NULL;
  struct onm_client_handler *och = NULL;
  struct onm_api_msg_header header;
  int nbytes = 0;
  int type = 0;
  int ret = 0;
  struct apn_vr *vr = NULL;
  
  if (!mc || !mc->info)
    return RESULT_ERROR;

   /* Get ONM client handler from message entry. */
   och = mc->info;
   oc = och->oc; 
   if (!oc)
    return RESULT_ERROR;

  /* Read data. */
  nbytes = onm_client_read (och, sock);
  if (nbytes <= 0)
    return nbytes;

  /* Parse ONM header. */
  ret = onm_decode_header (&och->pnt_in, &och->size_in, &header);
  if (ret < 0)
    {
      zlog_err (oc->zg, "onm_client_read: Message header decode error, "
                "ret = %d; terminating connection to server", ret);

      /* Show hexdump. */
      if (oc->debug)
        onm_header_dump (mc->zg, &header);

      return RESULT_ERROR;
    }

  /* Set the VR Context before invoking the Client Callback */
  vr = apn_vr_lookup_by_id (oc->zg, header.vr_id);
  if (vr)
    LIB_GLOB_SET_VR_CONTEXT (oc->zg, vr);
  else
    {
      /* If a message is received for a non existing VR, just ignore the
       *  message. Send the +ve number of bytes to not cause a disconnection. */
      return nbytes;
    }

  type = header.type;

  /* Invoke parser with call back function pointer.  There is no callback
   * set by protocols */
  if (type < ONM_MSG_MAX && oc->parser[type] && oc->callback[type])
    {
      ret = (*oc->parser[type]) (&och->pnt_in, &och->size_in, &header, och,
          oc->callback[type]);
      if (ret < 0)
        zlog_err (oc->zg, "Parse error for message %s", onm_msg_to_str (type));
    }

  return nbytes;
}

int
onm_client_send_message (struct onm_client_handler *och, u_int32_t vr_id,
                         int type, u_int16_t len, u_int32_t *msg_id)
{
  struct onm_api_msg_header header;
  int ret;
  u_char *pnt;
  u_int16_t size;

  if (!och || !och->mc || !och->mc->sock)
    return RESULT_ERROR;

  pnt = och->buf;
  size = ONM_MSG_MAX_LEN;

   /* If message ID warparounds, start from 1. */
  ++och->message_id;
  if (och->message_id == 0)
    och->message_id = 1;

  /* Prepare ONM message header. */
  header.vr_id = vr_id;
  header.type = type;
  header.length = len;
  header.message_id = och->message_id;

  /* Encode header. */
  onm_encode_header (&pnt, &size, &header);

  /* Write message to socket. */
  ret = writen (och->mc->sock, och->buf, len + ONM_MSG_HEADER_SIZE);
  if (ret != len + ONM_MSG_HEADER_SIZE)
    return RESULT_ERROR;

   if (msg_id)
     *msg_id = header.message_id;

  return RESULT_OK;
}

int
onm_client_start (struct onm_client *oc)
{
  int ret = 0;;

  if (oc->async)
    { 

      ret = message_client_start (oc->async->mc);
      if (ret < 0)
        {
          /* Start reconnect timer. */
          if (! oc->t_connect)
            {
              oc->t_connect = thread_add_timer (oc->zg, onm_client_reconnect, 
                                                oc,
                                                ONM_CLIENT_RECONNECT_INTERVAL);
              if (!oc->t_connect)
                return RESULT_ERROR;
            }
        }
    }

  /*  if (oc && oc->async && oc->async->mc->sock >= 0)
      pal_sock_set_nonblocking (oc->async->mc->sock, 1);
  */

  return RESULT_OK;
}

/* Stop ONM client. */
void
onm_client_stop (struct onm_client *oc)
{
  if (!oc || !oc->async || !oc->async->mc)
    return;

  message_client_stop (oc->async->mc);
}


int
onm_client_connect (struct message_handler *mc, struct message_entry *me,
                    pal_sock_handle_t sock)
{
  struct onm_client_handler *och = NULL;

  if (!mc || !mc->info)
    return RESULT_ERROR;

  och = mc->info;

  och->up = PAL_TRUE;

 /* Register read thread. */
  message_client_read_register (mc);

  return RESULT_OK;
}

int
onm_client_reconnect (struct thread *t)
{
  struct onm_client *oc = NULL;

  oc = THREAD_ARG (t);
  if (!oc)
    return RESULT_ERROR;

  oc->t_connect = NULL;
  onm_client_start (oc);

  return RESULT_OK;
}

int
onm_client_reconnect_start (struct onm_client *oc)
{
  if (!oc)
    return RESULT_ERROR;

  /* Start reconnect timer. */
  oc->t_connect = thread_add_timer (oc->zg, onm_client_reconnect,
                                    oc, ONM_CLIENT_RECONNECT_INTERVAL);
  if (! oc->t_connect)
    return LIB_API_ERROR;

  return RESULT_OK;
}

int
onm_client_disconnect (struct message_handler *mc, struct message_entry *me,
                       pal_sock_handle_t sock)
{

  struct onm_client_handler *och = NULL;
  struct onm_client *oc = NULL;
  struct listnode *node = NULL;
  struct listnode *next = NULL;

  if (!mc || !mc->info)
    return RESULT_ERROR;

  och = mc->info;
  if (!och->oc)
    return RESULT_ERROR;

  oc = och->oc;
  if (!oc || !oc->zg)
    return RESULT_ERROR;

  /* Set status to down i.e is 0. */
  och->up = RESULT_OK;

  /* Clean up client ID.  */
  onm_client_id_unset (oc);

  /* Cancel pending read thread. */
  if (oc->zg->pend_read_thread)
    THREAD_OFF (oc->zg->pend_read_thread);

  /* Free all pending reads. */
  for (node = LISTHEAD (&och->pend_msg_list); node; node = next)
    {
      struct onm_client_pend_msg *pmsg = GETDATA (node);

      next = node->next;

      XFREE (MTYPE_ONM_PENDING_MSG, pmsg);  
      list_delete_node (&och->pend_msg_list, node);
    }

  /* Stop async connection.  */
  if (oc->async)
    message_client_stop (oc->async->mc);

  /* Call client specific disconnect handler. */
  if (oc->disconnect_callback)
    oc->disconnect_callback ();
  else
    onm_client_reconnect_start (oc);

  return RESULT_OK;
}


struct onm_client_handler *
onm_client_handler_create (struct onm_client *oc, int type)
{
  struct onm_client_handler *och = NULL;
  struct message_handler *mc = NULL;

  /* Allocate onm client handler.  */
  och = XCALLOC (MTYPE_ONM_CLIENT_HANDLER, sizeof (struct onm_client_handler));
  if (!och)
    return NULL;

  if (!oc || !oc->zg)
    {
      XFREE (MTYPE_ONM_CLIENT_HANDLER, och);
      return NULL;
    }

  och->type = type;
  och->oc = oc;

  /* Set max message length.  */
  och->len = ONM_MSG_MAX_LEN;
  och->len_in = ONM_MSG_MAX_LEN;
  och->len_ipv4 = ONM_MSG_MAX_LEN; 

  /* Create asyoc message client. */
  mc = message_client_create (oc->zg, type); //type

  if (!mc)
    return NULL;

#ifndef HAVE_TCP_MESSAGE
  /* Use UNIX domain socket connection.  */
  message_client_set_style_domain (mc, ONM_SERVER_PATH);
#else /* HAVE_TCP_MESSAGE */
  message_client_set_style_tcp (mc, ONM_PORT);
#endif /* !HAVE_TCP_MESSAGE */

  /* Initiate connection using onm connection manager.  */
  message_client_set_callback (mc, MESSAGE_EVENT_CONNECT,
                               onm_client_connect);
  message_client_set_callback (mc, MESSAGE_EVENT_DISCONNECT,
                               onm_client_disconnect);
  message_client_set_callback (mc, MESSAGE_EVENT_READ_MESSAGE,
                               onm_client_read_msg);

  /* Link each other.  */
  och->mc = mc;
  mc->info = och;
  och->pnt = och->buf;
  och->pnt_in = och->buf_in;

  return och;
}


int
onm_client_handler_free (struct onm_client_handler *och)
{
  if (och && och->mc)
    message_client_delete (och->mc);

  if (och)
    XFREE (MTYPE_ONM_CLIENT_HANDLER, och);

  return RESULT_OK;
}


/* Set service type flag.  */
int
onm_client_set_service (struct onm_client *oc, int service)
{
  int type = 0;

  if (service >= ONM_SERVICE_MAX)
    return ONM_ERR_INVALID_SERVICE;

  if (!oc)
    return RESULT_ERROR;

  /* Set service bit to ONM client.  */
  ONM_SET_CTYPE (oc->service.bits, service);

  /* Check the service is sync or async.  */
  type = onm_service_type[service].type;

  /* Create client hander corresponding to message type.  */
  if (type == MESSAGE_TYPE_ASYNC)
    {
      if (! oc->async)
        {
          oc->async = onm_client_handler_create (oc, MESSAGE_TYPE_ASYNC);
          oc->async->service.version = oc->service.version;
          oc->async->service.protocol_id = oc->service.protocol_id;
        }

      ONM_SET_CTYPE (oc->async->service.bits, service);
    }

  return RESULT_OK;
}

void
onm_client_set_version (struct onm_client *oc, u_int16_t version)
{
  if (!oc)
    return;

  oc->service.version = version;
}

void
onm_client_set_protocol (struct onm_client *oc, u_int32_t protocol_id)
{
  if (!oc)
    return;

  oc->service.protocol_id = protocol_id;
}

void
onm_client_set_client_id (struct onm_client *oc, u_int32_t client_id)
{
  if (!oc)
    return;

  oc->client_id = client_id;
}

/* Register callback.  */
void
onm_client_set_callback (struct onm_client *oc, int message_type,
                         ONM_CALLBACK callback)
{
  if (!oc)
    return;

  oc->callback[message_type] = callback;
}
/* Register disconnect callback. */
void
onm_client_set_disconnect_callback (struct onm_client *oc,
                                    ONM_DISCONNECT_CALLBACK callback)
{
  if (!oc)
    return;

  oc->disconnect_callback = callback;
}

/* Initialize ONM client.  This fuoction allocate ONM client
   memory.  */
struct onm_client *
onm_client_create (struct lib_globals *zg, u_int32_t debug)
{
  struct onm_client *oc = NULL;

  if (! zg)
    return NULL;

  oc = XCALLOC (MTYPE_ONM_CLIENT, sizeof (struct onm_client));
  if (! oc)
    return NULL;

  oc->zg = zg;
  zg->oc = oc;

  /* Set the module ID.  */
  oc->service.protocol_id = zg->protocol;

  /* Set parsers. */
  onm_client_set_parser (oc, ONM_MSG_SERVICE_REPLY, onm_parse_service);
  
  onm_client_set_parser (oc, ONM_MSG_EVC_STATUS, onm_parse_evc_status_msg);

  oc->reconnect_interval = ONM_CLIENT_RECONNECT_INTERVAL;

  if (debug)
    oc->debug = PAL_TRUE;

  return oc;
}

int
onm_client_delete (struct onm_client *oc)
{
  struct listnode *node = NULL;
  struct listnode *node_next = NULL;
  struct onm_client_handler *och = NULL;

  if (oc)
    {
      if (oc->t_connect)
        THREAD_OFF (oc->t_connect);

      /* Cancel pending read thread. */
      if (oc->zg->pend_read_thread)
        THREAD_OFF (oc->zg->pend_read_thread);

      och = oc->async;
      /* Free all pending reads. */
      if (och)
        {
          for (node = LISTHEAD (&och->pend_msg_list); node; node = node_next)
            {
              struct onm_client_pend_msg *pmsg = GETDATA (node);

              node_next = node->next;

              XFREE (MTYPE_ONM_PENDING_MSG, pmsg);
              list_delete_node (&och->pend_msg_list, node);
            }

          onm_client_handler_free (och);
        }

      XFREE (MTYPE_ONM_CLIENT, oc);
    }
  return RESULT_OK;
}

#endif /* defined(HAVE_ONMD) && defined (HAVE_ELMID) */


