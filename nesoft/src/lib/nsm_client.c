/* Copyright (C) 2002-2003  All Rights Reserved. */

/* NSM client.  */
#include "pal.h"

#include "thread.h"
#include "network.h"
#include "message.h"
#ifdef HAVE_DSTE
#include "dste.h"
#endif /* HAVE_DSTE */
#include "nsm_message.h"
#include "nsm_client.h"
#include "imi_client.h"
#include "log.h"

#ifdef HAVE_MPLS
#include "mpls.h"
#endif /* HAVE_MPLS */

#ifdef HAVE_GMPLS
#include "gmpls/gmpls.h"
#endif /* HAVE_GMPLS */
#ifdef HAVE_DIFFSERV
#include "diffserv.h"
#endif /* HAVE_DIFFSERV */
#ifdef HAVE_HA
#include "lib_cal.h"
#endif /* HAVE_HA */

#ifdef HAVE_HA
#include "lib_ha.h"
#endif /* HAVE_HA */

#ifdef HAVE_MPLS_OAM
#include "oam_mpls_msg.h"
#endif /* HAVE_MPLS_OAM */

/* Prototypes. */
s_int32_t nsm_util_link_add (struct nsm_msg_header *, void *, void *);
s_int32_t nsm_util_link_delete (struct nsm_msg_header *, void *, void *);
s_int32_t nsm_util_link_up (struct nsm_msg_header *, void *, void *);
s_int32_t nsm_util_link_down (struct nsm_msg_header *, void *, void *);
s_int32_t nsm_util_link_update (struct nsm_msg_header *, void *, void *);
s_int32_t nsm_util_link_priority_bw_update (struct nsm_msg_header *, void *, void *);

/* TBD old code s_int32_t nsm_util_link_gmpls_update (struct nsm_msg_header *, void *, void *);*/
s_int32_t nsm_util_addr_add (struct nsm_msg_header *, void *, void *);
s_int32_t nsm_util_addr_delete (struct nsm_msg_header *, void *, void *);
s_int32_t nsm_util_vr_add (struct nsm_msg_header *, void *, void *);
s_int32_t nsm_util_vr_delete (struct nsm_msg_header *, void *, void *);
s_int32_t nsm_util_vr_bind (struct nsm_msg_header *, void *, void *);
s_int32_t nsm_util_vr_unbind (struct nsm_msg_header *, void *, void *);
s_int32_t nsm_util_vrf_add (struct nsm_msg_header *, void *, void *);
s_int32_t nsm_util_vrf_delete (struct nsm_msg_header *, void *, void *);
s_int32_t nsm_util_vrf_bind (struct nsm_msg_header *, void *, void *);
s_int32_t nsm_util_vrf_unbind (struct nsm_msg_header *, void *, void *);
s_int32_t nsm_util_router_id (struct nsm_msg_header *, void *, void *);
s_int32_t nsm_util_server_status_notify (struct nsm_msg_header *, void *, void *);
s_int32_t nsm_util_user_update (struct nsm_msg_header *, void *, void *);
s_int32_t nsm_util_user_delete (struct nsm_msg_header *, void *, void *);
#ifdef HAVE_GMPLS
int gmpls_control_adj_update_by_msg (struct control_adj *,
                                     struct nsm_msg_control_adj *);
#endif /* HAVE_GMPLS */

/* Specify each NSM service is sync or async.  */
struct
{
  int message;
  int type;
} nsm_service_type[] =
{
  {NSM_SERVICE_INTERFACE,         MESSAGE_TYPE_ASYNC},
  {NSM_SERVICE_ROUTE,             MESSAGE_TYPE_ASYNC},
  {NSM_SERVICE_ROUTER_ID,         MESSAGE_TYPE_ASYNC},
  {NSM_SERVICE_VRF,               MESSAGE_TYPE_ASYNC},
  {NSM_SERVICE_ROUTE_LOOKUP,      MESSAGE_TYPE_ASYNC},
  {NSM_SERVICE_LABEL,             MESSAGE_TYPE_ASYNC},
  {NSM_SERVICE_TE,                MESSAGE_TYPE_ASYNC},
  {NSM_SERVICE_QOS,               MESSAGE_TYPE_ASYNC},
  {NSM_SERVICE_QOS_PREEMPT,       MESSAGE_TYPE_ASYNC},
  {NSM_SERVICE_USER,              MESSAGE_TYPE_ASYNC},
  {NSM_SERVICE_HOSTNAME,          MESSAGE_TYPE_ASYNC},
  {NSM_SERVICE_MPLS_VC,           MESSAGE_TYPE_ASYNC},
  {NSM_SERVICE_MPLS,              MESSAGE_TYPE_ASYNC},
  {NSM_SERVICE_GMPLS,             MESSAGE_TYPE_ASYNC},
  {NSM_SERVICE_DIFFSERV,          MESSAGE_TYPE_ASYNC},
  {NSM_SERVICE_VPLS,              MESSAGE_TYPE_ASYNC},
  {NSM_SERVICE_DSTE,              MESSAGE_TYPE_ASYNC},
  {NSM_SERVICE_IPV4_MRIB,         MESSAGE_TYPE_ASYNC},
  {NSM_SERVICE_IPV4_PIM,          MESSAGE_TYPE_ASYNC},
  {NSM_SERVICE_IPV4_MCAST_TUNNEL, MESSAGE_TYPE_ASYNC},
  {NSM_SERVICE_IPV4_MRIB,         MESSAGE_TYPE_ASYNC},
  {NSM_SERVICE_IPV4_PIM,          MESSAGE_TYPE_ASYNC},
  {NSM_SERVICE_BRIDGE,            MESSAGE_TYPE_ASYNC},
  {NSM_SERVICE_VLAN,              MESSAGE_TYPE_ASYNC},
  {NSM_SERVICE_IGP_SHORTCUT,      MESSAGE_TYPE_ASYNC},
  {NSM_SERVICE_CONTROL_ADJ,       MESSAGE_TYPE_ASYNC},
  {NSM_SERVICE_CONTROL_CHANNEL,   MESSAGE_TYPE_ASYNC},
  {NSM_SERVICE_TE_LINK,           MESSAGE_TYPE_ASYNC},
  {NSM_SERVICE_DATA_LINK  ,       MESSAGE_TYPE_ASYNC},
  {NSM_SERVICE_DATA_LINK_SUB,     MESSAGE_TYPE_ASYNC}
};


/* Set packet parser.  */
void
nsm_client_set_parser (struct nsm_client *nc, int message_type,
                       NSM_PARSER parser)
{

  nc->parser[message_type] = parser;
}

void
nsm_client_id_set (struct nsm_client *nc, struct nsm_msg_service *service)
{
  if (nc->async && nc->async->service.client_id != service->client_id)
    nc->async->service.client_id = service->client_id;
}

void
nsm_client_id_unset (struct nsm_client *nc)
{
  if (nc->async)
    nc->async->service.client_id = 0;
}

s_int32_t
nsm_client_read (struct nsm_client_handler *nch, pal_sock_handle_t sock)
{
  struct nsm_msg_header *header;
  u_int16_t length;
  int nbytes = 0;

  nch->size_in = 0;
  nch->pnt_in = nch->buf_in;

  nbytes = readn (sock, nch->buf_in, NSM_MSG_HEADER_SIZE);
  if (nbytes < NSM_MSG_HEADER_SIZE)
    return -1;

  header = (struct nsm_msg_header *) nch->buf_in;
  length = pal_ntoh16 (header->length);

  nbytes = readn (sock, nch->buf_in + NSM_MSG_HEADER_SIZE,
                  length - NSM_MSG_HEADER_SIZE);
  if (nbytes <= 0)
    return nbytes;

  nch->size_in = length;

  return length;
}

/* Read NSM message body.  */
s_int32_t
nsm_client_read_msg (struct message_handler *mc,
                     struct message_entry *me,
                     pal_sock_handle_t sock)
{
  struct nsm_client_handler *nch;
  struct nsm_client *nc;
  struct nsm_msg_header header;
  struct apn_vr *vr;
  int nbytes;
  int type;
  int ret;

  /* Get NSM client handler from message entry. */
  nch = mc->info;
  nc = nch->nc;

  /* Read data. */
  nbytes = nsm_client_read (nch, sock);
  if (nbytes <= 0)
    return nbytes;

  /* Parse NSM message header. */
  ret = nsm_decode_header (&nch->pnt_in, &nch->size_in, &header);
  if (ret < 0)
    return -1;

#ifdef HAVE_HA
  /* During startup config, do not process messages from NSM
   * till the config_done state, as for simplex-active,
   * configuration is downloaded from checkpoint server */
  if ((HA_DROP_CFG_ISSET (nc->zg))
      && (! HA_ALLOW_NSM_MSG_TYPE (header.type)))
    return 0;
#endif /* HAVE_HA */

  /* Dump NSM header. */
  if (nc->debug)
    nsm_header_dump (mc->zg, &header);

  /* Set the VR Context before invoking the Client Callback */
  vr = apn_vr_lookup_by_id (nc->zg, header.vr_id);
  if (vr)
    LIB_GLOB_SET_VR_CONTEXT (nc->zg, vr);
  else
    {
      /* If a message is received for a non existing VR, just ignore the
         message. Send the +ve number of bytes to not cause a disconnection. */
      return nbytes;
    }

  type = header.type;

  /* Invoke parser with call back function pointer.  There is no callback
     set by protocols for MPLS replies. */
  if (type < NSM_MSG_MAX && nc->parser[type] && nc->callback[type])
    {
      ret = (*nc->parser[type]) (&nch->pnt_in, &nch->size_in, &header, nch,
                                 nc->callback[type]);
      if (ret < 0)
        zlog_err (nc->zg, "Parse error for message %s", nsm_msg_to_str (type));
    }

  return nbytes;
}

/* Read NSM message body. Don't parse and call callback  */
s_int32_t
nsm_client_read_sync (struct message_handler *mc, struct message_entry *me,
                      pal_sock_handle_t sock,
                      struct nsm_msg_header *header, int *type)
{
  struct nsm_client_handler *nch;
  struct nsm_client *nc;
  int nbytes;
  int ret;

  /* Get NSM server entry from message entry.  */
  nch = mc->info;
  nc = nch->nc;

  /* Read msg */
  nbytes = nsm_client_read (nch, sock);
  if (nbytes <= 0)
    {
      message_client_disconnect (mc, sock);
      return -1;
    }

  /* Parse NSM message header.  */
  ret = nsm_decode_header (&nch->pnt_in, &nch->size_in, header);
  if (ret < 0)
    return -1;

  /* Dump NSM header.  */
  if (nc->debug)
    nsm_header_dump (mc->zg, header);

  *type = header->type;

  return nbytes;
}

void
nsm_client_pending_message (struct nsm_client_handler *nch,
                            struct nsm_msg_header *header)
{
  struct nsm_client_pend_msg *pmsg;

  /* Queue the message for later processing. */
  pmsg = XMALLOC (MTYPE_NSM_PENDING_MSG, sizeof (struct nsm_client_pend_msg));
  if (pmsg == NULL)
    return;

  pmsg->header = *header;

  pal_mem_cpy (pmsg->buf, nch->pnt_in, nch->size_in);

  /* Add to pending list. */
  listnode_add (&nch->pend_msg_list, pmsg);
}

/* Generic function to send message to NSM server.  */
s_int32_t
nsm_client_send_message (struct nsm_client_handler *nch,
                         u_int32_t vr_id, vrf_id_t vrf_id,
                         int type, u_int16_t len, u_int32_t *msg_id)
{
  struct nsm_msg_header header;
  u_int16_t size;
  u_char *pnt;
  int ret;

  if (nch->nc == NULL)
    return -1;
  
   /* check if nsm is shutdown*/
  if (CHECK_FLAG (nch->nc->nsm_server_flags, NSM_SERVER_SHUTDOWN))
    return 0;

#ifdef HAVE_HA
  /* If HA_DROP_CFG_ISSET drop the messages
   * other than service request and reply
   * This flag takes care of when it is standby and
   * for active, before reading the configuration
   * ie, (HA_startup_state is CONFIG_DONE) */
  if ((HA_DROP_CFG_ISSET (nch->nc->zg))
      && (! HA_ALLOW_NSM_MSG_TYPE (type)))
    return 0;
#endif /* HAVE_HA */

  pnt = nch->buf;
  size = NSM_MESSAGE_MAX_LEN;

  /* If message ID warparounds, start from 1. */
  ++nch->message_id;
  if (nch->message_id == 0)
    nch->message_id = 1;

  /* Prepare NSM message header.  */
  header.vr_id = vr_id;
  header.vrf_id = vrf_id;
  header.type = type;
  header.length = len + NSM_MSG_HEADER_SIZE;
  header.message_id = nch->message_id;

  /* Encode header.  */
  nsm_encode_header (&pnt, &size, &header);

  /* Write message to the socket.  */
  ret = writen (nch->mc->sock, nch->buf, len + NSM_MSG_HEADER_SIZE);
  if (ret != len + NSM_MSG_HEADER_SIZE)
    return -1;

  if (msg_id)
    *msg_id = header.message_id;

  return 0;
}

/* Send bundle message to NSM server.  */
s_int32_t
nsm_client_send_message_bundle (struct nsm_client_handler *nch,
                                u_int32_t vr_id, vrf_id_t vrf_id,
                                int type, u_int16_t len)
{
  struct nsm_msg_header header;
  u_int16_t size;
  u_char *pnt;
  u_char *buf;
  int ret;

  if (type == NSM_MSG_ROUTE_IPV4)
    buf = nch->buf_ipv4;
#ifdef HAVE_IPV6
  else if (type == NSM_MSG_ROUTE_IPV6)
    buf = nch->buf_ipv6;
#endif /* HAVE_IPV6 */
  else
    buf = nch->buf;

  pnt = buf;
  size = NSM_MESSAGE_MAX_LEN;

  /* Prepare NSM message header.  */
  header.vr_id = vr_id;
  header.vrf_id = vrf_id;
  header.type = type;
  header.length = len + NSM_MSG_HEADER_SIZE;
  header.message_id = ++nch->message_id;

  /* Encode header.  */
  nsm_encode_header (&pnt, &size, &header);

  /* Write message to the socket.  */
  ret = writen (nch->mc->sock, buf, len + NSM_MSG_HEADER_SIZE);
  if (ret != len + NSM_MSG_HEADER_SIZE)
    return -1;

  return 0;
}

/* Process pending message requests. */
s_int32_t
nsm_client_process_pending_msg (struct thread *t)
{
  struct nsm_client_handler *nch;
  struct nsm_client *nc;
  struct lib_globals *zg;
  struct listnode *node;
  u_int16_t size;
  u_char *pnt;
  int ret;

  nch = THREAD_ARG (t);
  nc = nch->nc;

  /* Reset thread. */
  zg = nch->nc->zg;
  zg->pend_read_thread = NULL;

  node = LISTHEAD (&nch->pend_msg_list);
  if (node)
    {
      struct nsm_client_pend_msg *pmsg = GETDATA (node);
      int type;

      if (!pmsg)
        return 0;

      pnt = pmsg->buf;
      size = pmsg->header.length - NSM_MSG_HEADER_SIZE;
      type = pmsg->header.type;

      if (nc->debug)
        nsm_header_dump (zg, &pmsg->header);

      if (nc->debug)
        zlog_info (nc->zg, "Processing pending message %s",
                   nsm_msg_to_str (type));

      ret = (*nc->parser[type]) (&pnt, &size, &pmsg->header, nch,
                                 nc->callback[type]);
      if (ret < 0)
        zlog_err (nc->zg, "Parse error for message %s", nsm_msg_to_str (type));

      /* Free processed message and node. */
      XFREE (MTYPE_NSM_PENDING_MSG, pmsg);
      list_delete_node (&nch->pend_msg_list, node);
    }
  else
    return 0;

  node = LISTHEAD (&nch->pend_msg_list);
  if (node)
    {
      /* Process pending requests. */
      if (!zg->pend_read_thread)
        zg->pend_read_thread
          = thread_add_read_pend (zg, nsm_client_process_pending_msg, nch, 0);
    }

  return 0;
}

/* Send service message.  */
s_int32_t
nsm_client_send_service (struct nsm_client_handler *nch)
{
  u_int32_t msg_id;
  u_int16_t size;
  u_char *pnt;
  int nbytes;
  u_int32_t vr_id = 0;
  vrf_id_t vrf_id = 0;

  if (! nch || ! nch->up)
    return -1;

  pnt = nch->buf + NSM_MSG_HEADER_SIZE;
  size = nch->len - NSM_MSG_HEADER_SIZE;

  nbytes = nsm_encode_service (&pnt, &size, &nch->service);
  if (nbytes < 0)
    return nbytes;

  return nsm_client_send_message (nch, vr_id, vrf_id,
                                  NSM_MSG_SERVICE_REQUEST, nbytes, &msg_id);
}

/* Send redistribute message.  */
s_int32_t
nsm_client_send_redistribute_set (u_int32_t vr_id, vrf_id_t vrf_id,
                                  struct nsm_client *nc,
                                  struct nsm_msg_redistribute *msg)
{
  struct nsm_client_handler *nch = nc->async;
  u_int32_t msg_id;
  u_int16_t size;
  u_char *pnt;
  int nbytes;

  if (! nch || ! nch->up)
    return -1;

  pnt = nch->buf + NSM_MSG_HEADER_SIZE;
  size = nch->len - NSM_MSG_HEADER_SIZE;

  nbytes = nsm_encode_redistribute (&pnt, &size, msg);
  if (nbytes < 0)
    return nbytes;

  return nsm_client_send_message (nch, vr_id, vrf_id,
                                  NSM_MSG_REDISTRIBUTE_SET, nbytes, &msg_id);
}

/* Send redistribute message.  */
s_int32_t
nsm_client_send_redistribute_unset (u_int32_t vr_id, vrf_id_t vrf_id,
                                    struct nsm_client *nc,
                                    struct nsm_msg_redistribute *msg)
{
  struct nsm_client_handler *nch = nc->async;
  u_int32_t msg_id;
  u_int16_t size;
  u_char *pnt;
  int nbytes;

  /* We use async connection for IPv4 route updates.  */
  if (! nch || ! nch->up )
    return -1;

  /* Set pnt and size.  */
  pnt = nch->buf + NSM_MSG_HEADER_SIZE;
  size = nch->len - NSM_MSG_HEADER_SIZE;

  /* Encode service.  */
  nbytes = nsm_encode_redistribute (&pnt, &size, msg);
  if (nbytes < 0)
    return nbytes;

  return nsm_client_send_message (nch, vr_id, vrf_id,
                                  NSM_MSG_REDISTRIBUTE_UNSET, nbytes, &msg_id);
}

/* Send wait for bgp set message.  */
s_int32_t
nsm_client_send_wait_for_bgp_set (u_int32_t vr_id, vrf_id_t vrf_id,
                                  struct nsm_client *nc,
                                  struct nsm_msg_wait_for_bgp *msg)
{
  struct nsm_client_handler *nch = nc->async;
  u_int32_t msg_id;
  u_int16_t size;
  u_char *pnt;
  int nbytes;

  if (! nch || ! nch->up)
    return -1;

  pnt = nch->buf + NSM_MSG_HEADER_SIZE;
  size = nch->len - NSM_MSG_HEADER_SIZE;

  nbytes = nsm_encode_bgp_conv (&pnt, &size, msg);
  if (nbytes < 0)
    return nbytes;

  return nsm_client_send_message (nch, vr_id, vrf_id,
                                  NSM_MSG_ISIS_WAIT_BGP_SET, nbytes, &msg_id);
}

/* Send BGP converged message */
s_int32_t nsm_client_send_bgp_conv_done (u_int32_t vr_id, vrf_id_t vrf_id,
                                  struct nsm_client *nc,
                                  struct nsm_msg_wait_for_bgp *msg)
{
  struct nsm_client_handler *nch = nc->async;
  u_int32_t msg_id;
  u_int16_t size;
  u_char *pnt;
  int nbytes;

  if (! nch || ! nch->up)
    return -1;

  pnt = nch->buf + NSM_MSG_HEADER_SIZE;
  size = nch->len - NSM_MSG_HEADER_SIZE;

  /* Encode BGP converged message */
  nbytes = nsm_encode_bgp_conv (&pnt, &size, msg);
  if (nbytes < 0)
    return nbytes;

  return nsm_client_send_message (nch, vr_id, vrf_id,
                                  NSM_MSG_BGP_CONV_DONE, nbytes, &msg_id);
}

s_int32_t
nsm_client_send_ldp_session_state (u_int32_t vr_id, vrf_id_t vrf_id,
                                   struct nsm_client *nc,
                                   struct nsm_msg_ldp_session_state *msg)
{
  struct nsm_client_handler *nch = nc->async;
  u_int32_t msg_id;
  u_int16_t size;
  u_char *pnt;
  int nbytes;

  if (! nch || ! nch->up)
    return -1;

  pnt = nch->buf + NSM_MSG_HEADER_SIZE;
  size = nch->len - NSM_MSG_HEADER_SIZE;

  nbytes = nsm_encode_ldp_session_state (&pnt, &size, msg);
  if (nbytes < 0)
    return nbytes;

  return nsm_client_send_message (nch, vr_id, vrf_id,
                                  NSM_MSG_LDP_SESSION_STATE, nbytes, &msg_id);
}

int
nsm_client_send_ldp_session_query (u_int32_t vr_id, vrf_id_t vrf_id,
                                   struct nsm_client *nc,
                                   struct nsm_msg_ldp_session_state *msg)
{
  struct nsm_client_handler *nch = nc->async;
  u_int32_t msg_id;
  u_int16_t size;
  u_char *pnt;
  int nbytes;

  if (! nch || ! nch->up)
    return -1;

  pnt = nch->buf + NSM_MSG_HEADER_SIZE;
  size = nch->len - NSM_MSG_HEADER_SIZE;

  nbytes = nsm_encode_ldp_session_state (&pnt, &size, msg);
  if (nbytes < 0)
    return nbytes;

  return nsm_client_send_message (nch, vr_id, vrf_id,
                                  NSM_MSG_LDP_SESSION_QUERY, nbytes, &msg_id);
}

#ifdef HAVE_CRX
/* Send interface address(typically for virtual IP). */
s_int32_t
nsm_client_send_addr_add (struct nsm_client *nc,
                          struct nsm_msg_address *msg)
{
  struct nsm_client_handler *nch = nc->async;
  u_int32_t msg_id;
  u_int16_t size;
  u_char *pnt;
  int nbytes;
  u_int32_t vr_id = 0;
  vrf_id_t vrf_id = 0;

  if (! nch || ! nch->up)
    return -1;

  pnt = nch->buf + NSM_MSG_HEADER_SIZE;
  size = nch->len - NSM_MSG_HEADER_SIZE;

  nbytes = nsm_encode_address (&pnt, &size, msg);
  if (nbytes < 0)
    return nbytes;

  return nsm_client_send_message (nch, vr_id, vrf_id,
                                  NSM_MSG_ADDR_ADD, nbytes, &msg_id);
}

s_int32_t
nsm_client_send_addr_delete (struct nsm_client *nc,
                             struct nsm_msg_address *msg)
{
  struct nsm_client_handler *nch = nc->async;
  u_int32_t msg_id;
  u_int16_t size;
  u_char *pnt;
  int nbytes;
  u_int32_t vr_id = 0;
  vrf_id_t vrf_id = 0;

  if (! nch || ! nch->up)
    return -1;

  pnt = nch->buf + NSM_MSG_HEADER_SIZE;
  size = nch->len - NSM_MSG_HEADER_SIZE;

  nbytes = nsm_encode_address (&pnt, &size, msg);
  if (nbytes < 0)
    return nbytes;

  return nsm_client_send_message (nch, vr_id, vrf_id,
                                  NSM_MSG_ADDR_DELETE, nbytes, &msg_id);
}
#endif /* HAVE_CRX */

/* Send IPv4 message.  */
s_int32_t
nsm_client_send_route_ipv4 (u_int32_t vr_id, vrf_id_t vrf_id,
                            struct nsm_client *nc,
                            struct nsm_msg_route_ipv4 *msg)
{
  struct nsm_client_handler *nch = nc->async;
  u_int32_t msg_id;
  u_int16_t size;
  u_char *pnt;
  int nbytes;

  if (! nch || ! nch->up)
    return -1;

  pnt = nch->buf + NSM_MSG_HEADER_SIZE;
  size = nch->len - NSM_MSG_HEADER_SIZE;

  nbytes = nsm_encode_route_ipv4 (&pnt, &size, msg);
  if (nbytes < 0)
    return nbytes;

  return nsm_client_send_message (nch, vr_id, vrf_id,
                                  NSM_MSG_ROUTE_IPV4, nbytes, &msg_id);
}

/* Flush the bundle message.  */
s_int32_t
nsm_client_send_route_ipv4_flush (struct thread *t)
{
  struct nsm_client_handler *nch;
  struct nsm_client *nc;
  int nbytes;

  nc = THREAD_ARG (t);
  nch = nc->async;
  nch->t_ipv4 = NULL;

  nbytes = nch->pnt_ipv4 - nch->buf_ipv4 - NSM_MSG_HEADER_SIZE;

  nsm_client_send_message_bundle (nch, nch->vr_id_ipv4, nch->vrf_id_ipv4,
                                  NSM_MSG_ROUTE_IPV4, nbytes);

  nch->pnt_ipv4 = NULL;
  nch->len_ipv4 = NSM_MESSAGE_MAX_LEN;

  return 0;
}

/* API for flush bundle message.  */
s_int32_t
nsm_client_flush_route_ipv4 (struct nsm_client *nc)
{
  struct nsm_client_handler *nch = nc->async;

  if (nch->pnt_ipv4)
    {
      THREAD_TIMER_OFF (nch->t_ipv4);
      thread_execute (nc->zg, nsm_client_send_route_ipv4_flush, nc, 0);
      return 1;
    }
  return 0;
}

s_int32_t
nsm_client_send_route_ipv4_bundle (u_int32_t vr_id, vrf_id_t vrf_id,
                                   struct nsm_client *nc,
                                   struct nsm_msg_route_ipv4 *msg)
{
  struct nsm_client_handler *nch = nc->async;
  u_int16_t size;
  u_char *pnt;
  int nbytes;

  /* We use async connection for IPv4 route updates.  */
  if (! nch || ! nch->up)
    return -1;

  /* If stored data is different context, flush it first. */
  if (nch->pnt_ipv4 != NULL
      && (nch->vr_id_ipv4 != vr_id || nch->vrf_id_ipv4 != vrf_id))
    {
      THREAD_TIMER_OFF (nch->t_ipv4);
      thread_execute (nc->zg, nsm_client_send_route_ipv4_flush, nc, 0);
    }

  /* Set temporary pnt and size.  */
  pnt = nch->buf;
  size = nch->len;

  /* Encode service.  */
  nbytes = nsm_encode_route_ipv4 (&pnt, &size, msg);
  if (nbytes < 0)
    return nbytes;

  /* When bundle buffer is filled in flush it.  */
  if (nbytes >= nch->len_ipv4)
    {
      THREAD_TIMER_OFF (nch->t_ipv4);
      thread_execute (nc->zg, nsm_client_send_route_ipv4_flush, nc, 0);
    }

  /* Set IPv4 pnt and size.  */
  if (nch->pnt_ipv4)
    {
      pnt = nch->pnt_ipv4;
      size = nch->len_ipv4;
    }
  else
    {
      pnt = nch->buf_ipv4 + NSM_MSG_HEADER_SIZE;
      size = nch->len_ipv4 - NSM_MSG_HEADER_SIZE;
    }

  /* Copy the information.  */
  pal_mem_cpy (pnt, nch->buf, nbytes);

  /* Update pointer and length.  */
  nch->pnt_ipv4 = pnt + nbytes;
  nch->len_ipv4 = size -= nbytes;

  /* Update context. */
  nch->vr_id_ipv4 = vr_id;
  nch->vrf_id_ipv4 = vrf_id;

  /* Start timer.  */
  THREAD_TIMER_ON (nc->zg, nch->t_ipv4,
                   nsm_client_send_route_ipv4_flush, nc,
                   NSM_CLIENT_BUNDLE_TIME);

  return 0;
}

#ifdef HAVE_IPV6
/* Send IPv6 message.  */
s_int32_t
nsm_client_send_route_ipv6 (u_int32_t vr_id, vrf_id_t vrf_id,
                            struct nsm_client *nc,
                            struct nsm_msg_route_ipv6 *msg)
{
  struct nsm_client_handler *nch = nc->async;
  u_int32_t msg_id;
  u_int16_t size;
  u_char *pnt;
  int nbytes;

  /* We use async connection for IPv6 route updates.  */
  if (! nch || ! nch->up)
    return -1;

  /* Set pnt and size.  */
  pnt = nch->buf + NSM_MSG_HEADER_SIZE;
  size = nch->len - NSM_MSG_HEADER_SIZE;

  /* Encode service.  */
  nbytes = nsm_encode_route_ipv6 (&pnt, &size, msg);
  if (nbytes < 0)
    return nbytes;

  return nsm_client_send_message (nch, vr_id, vrf_id,
                                  NSM_MSG_ROUTE_IPV6, nbytes, &msg_id);
}

s_int32_t
nsm_client_send_route_ipv6_flush (struct thread *t)
{
  struct nsm_client_handler *nch;
  struct nsm_client *nc;
  int nbytes;

  nc = THREAD_ARG (t);
  nch = nc->async;
  nch->t_ipv6 = NULL;

  nbytes = nch->pnt_ipv6 - nch->buf_ipv6 - NSM_MSG_HEADER_SIZE;

  nsm_client_send_message_bundle (nch, nch->vr_id_ipv6, nch->vrf_id_ipv6,
                                  NSM_MSG_ROUTE_IPV6, nbytes);

  nch->pnt_ipv6 = NULL;
  nch->len_ipv6 = NSM_MESSAGE_MAX_LEN;

  return 0;
}

/* API for flush bundle message.  */
s_int32_t
nsm_client_flush_route_ipv6 (struct nsm_client *nc)
{
  struct nsm_client_handler *nch = nc->async;

  if (nch->pnt_ipv6)
    {
      THREAD_TIMER_OFF (nch->t_ipv6);
      thread_execute (nc->zg, nsm_client_send_route_ipv6_flush, nc, 0);
      return 1;
    }
  return 0;
}

s_int32_t
nsm_client_send_route_ipv6_bundle (u_int32_t vr_id, vrf_id_t vrf_id,
                                   struct nsm_client *nc,
                                   struct nsm_msg_route_ipv6 *msg)
{
  struct nsm_client_handler *nch = nc->async;
  u_int16_t size;
  u_char *pnt;
  int nbytes;

  /* We use async connection for IPv6 route updates. */
  if (! nch || ! nch->up)
    return -1;

  /* If stored data is different context, flush it first. */
  if (nch->pnt_ipv6 != NULL
      && (nch->vr_id_ipv6 != vr_id || nch->vrf_id_ipv6 != vrf_id))
    {
      THREAD_TIMER_OFF (nch->t_ipv6);
      thread_execute (nc->zg, nsm_client_send_route_ipv6_flush, nc, 0);
    }

  /* Set temporary pnt and size. */
  pnt = nch->buf;
  size = nch->len;

  /* Encode service. */
  nbytes = nsm_encode_route_ipv6 (&pnt, &size, msg);
  if (nbytes < 0)
    return nbytes;

  /* When bundle buffer is filled in flush it. */
  if (nbytes >= nch->len_ipv6)
    {
      THREAD_TIMER_OFF (nch->t_ipv6);
      thread_execute (nc->zg, nsm_client_send_route_ipv6_flush, nc, 0);
    }

  /* Set IPv6 pnt and size. */
  if (nch->pnt_ipv6)
    {
      pnt = nch->pnt_ipv6;
      size = nch->len_ipv6;
    }
  else
    {
      pnt = nch->buf_ipv6 + NSM_MSG_HEADER_SIZE;
      size = nch->len_ipv6 - NSM_MSG_HEADER_SIZE;
    }

  /* Copy the information. */
  pal_mem_cpy (pnt, nch->buf, nbytes);

  /* Update pointer and length. */
  nch->pnt_ipv6 = pnt + nbytes;
  nch->len_ipv6 = size -= nbytes;

  /* Update context. */
  nch->vr_id_ipv6 = vr_id;
  nch->vrf_id_ipv6 = vrf_id;

  /* Start timer. */
  THREAD_TIMER_ON (nc->zg, nch->t_ipv6,
                   nsm_client_send_route_ipv6_flush, nc,
                   NSM_CLIENT_BUNDLE_TIME);

  return 0;
}

#endif /* HAVE_IPV6 */

#ifdef HAVE_MPLS
s_int32_t
nsm_client_send_label_request (struct nsm_client *nc,
                               struct nsm_msg_label_pool *msg,
                               struct nsm_msg_label_pool *reply)
{
  struct nsm_client_handler *nch;
  struct nsm_msg_header header;
  struct lib_globals *zg;
  u_int32_t msg_id;
  u_int16_t size;
  u_char *pnt;
  int nbytes;
  int ret_type = -1;
  int ret;
  u_int32_t vr_id = 0;
  vrf_id_t vrf_id = 0; /* XXX-VR */

  /* Label pool service is provided as sync on the async socket. Other
   * messages from NSM are queued. */
  nch = nc->async;

  if (! nch || ! nch->up)
    return -1;

  /* Do not allow sync messages on HA standby */
#ifdef HAVE_HA
  if (HA_IS_STANDBY (nc->zg))
    return 0;
#endif /* HAVE_HA */

  /* Set pnt and size.  */
  pnt = nch->buf + NSM_MSG_HEADER_SIZE;
  size = nch->len - NSM_MSG_HEADER_SIZE;

  /* Encode service.  */
  nbytes = nsm_encode_label_pool (&pnt, &size, msg);
  if (nbytes < 0)
    return nbytes;

  /* Send message. */
  nbytes = nsm_client_send_message (nch, vr_id, vrf_id,
                                    NSM_MSG_LABEL_POOL_REQUEST,
                                    nbytes, &msg_id);
  if (nbytes < 0)
    return nbytes;

  do
    {
      /* Sync wait for reply. */
      nbytes = nsm_client_read_sync (nch->mc, NULL, nch->mc->sock, &header,
                                     &ret_type);
      if (nbytes <= 0)
        return nbytes;

      if ((ret_type != NSM_MSG_LABEL_POOL_REQUEST)
          || (header.message_id != msg_id))
        nsm_client_pending_message (nch, &header);
    }
  while ((ret_type != NSM_MSG_LABEL_POOL_REQUEST)
         || (header.message_id != msg_id));

  /* At here read thread may already registered as readable.
     Re-register read thread avoid hang.  */
  message_client_read_reregister (nch->mc);

  pal_mem_set (reply, 0, sizeof (struct nsm_msg_label_pool));

  /* Set pnt and size. */
  pnt = nch->buf_in + NSM_MSG_HEADER_SIZE;
  size = nch->size_in;

  /* Decode route */
  ret = nsm_decode_label_pool (&pnt, &size, reply);
  if (ret < 0)
    return ret;

  if (nc->debug)
    nsm_label_pool_dump (nc->zg, reply);

  /* Launch event to process pending requests. */
  zg = nch->nc->zg;
  if (!zg->pend_read_thread)
    zg->pend_read_thread
      = thread_add_read_pend (zg, nsm_client_process_pending_msg, nch, 0);

  return 0;
}

s_int32_t
nsm_client_send_label_release (struct nsm_client *nc,
                               struct nsm_msg_label_pool *msg)
{
  struct nsm_client_handler *nch;
  struct nsm_msg_label_pool reply;
  struct nsm_msg_header header;
  struct lib_globals *zg;
  u_int32_t msg_id;
  u_int16_t size;
  u_char *pnt;
  int nbytes;
  int ret_type = -1;
  int ret;
  u_int32_t vr_id = 0;
  vrf_id_t vrf_id = 0; /* XXX-VR */

  /* Label pool service is provided as sync on the async socket. Other
   * messages from NSM are queued. */
  nch = nc->async;

  if (! nch || ! nch->up)
    return -1;

  /* Do not allow sync messages on HA standby */
#ifdef HAVE_HA
  if (HA_IS_STANDBY (nc->zg))
    return 0;
#endif /* HAVE_HA */

  /* Set pnt and size.  */
  pnt = nch->buf + NSM_MSG_HEADER_SIZE;
  size = nch->len - NSM_MSG_HEADER_SIZE;

  /* Encode service.  */
  nbytes = nsm_encode_label_pool (&pnt, &size, msg);
  if (nbytes < 0)
    return nbytes;

  /* Send message. */
  nbytes = nsm_client_send_message (nch, vr_id, vrf_id,
                                    NSM_MSG_LABEL_POOL_RELEASE,
                                    nbytes, &msg_id);
  if (nbytes < 0)
    return nbytes;

  do
    {
      /* Sync wait for reply. */
      nbytes = nsm_client_read_sync (nch->mc, NULL, nch->mc->sock, &header,
                                     &ret_type);
      if (nbytes < 0)
        return nbytes;

      if ((ret_type != NSM_MSG_LABEL_POOL_RELEASE)
          || (header.message_id != msg_id))
        nsm_client_pending_message (nch, &header);
    }
  while ((ret_type != NSM_MSG_LABEL_POOL_RELEASE)
         || (header.message_id != msg_id));

  /* At here read thread may already registered as readable.
     Re-register read thread avoid hang.  */
  message_client_read_reregister (nch->mc);

  pal_mem_set (&reply, 0, sizeof (struct nsm_msg_label_pool));

  /* Set pnt and size. */
  pnt = nch->buf_in + NSM_MSG_HEADER_SIZE;
  size = nch->size_in;

  /* Decode route */
  ret = nsm_decode_label_pool (&pnt, &size, &reply);
  if (ret < 0)
    return ret;

  if (nc->debug)
    nsm_label_pool_dump (nc->zg, &reply);

  /* Launch event to process pending requests. */
  zg = nch->nc->zg;
  if (!zg->pend_read_thread)
    zg->pend_read_thread
      = thread_add_read_pend (zg, nsm_client_process_pending_msg, nch, 0);

  return 0;
}

int nsm_client_ilm_gen_lookup (struct nsm_client *nc,
                               struct nsm_msg_ilm_gen_lookup *msg,
                               struct nsm_msg_ilm_gen_lookup *reply)
{
  struct nsm_client_handler *nch;
  struct nsm_msg_header header;
  struct lib_globals *zg;
  u_int32_t msg_id;
  u_int16_t size;
  u_char *pnt;
  int nbytes;
  int ret_type = -1;
  int ret;
  u_int32_t vr_id = 0;
  vrf_id_t vrf_id = 0;

  /* ILM entry lookup is provided as sync on the async socket. */
  nch = nc->async;

  if (! nch || ! nch->up)
    return -1;

  /* Do not allow sync messages on HA standby */
#ifdef HAVE_HA
  if (HA_IS_STANDBY (nc->zg))
    return 0;
#endif /* HAVE_HA */

  pnt = nch->buf + NSM_MSG_HEADER_SIZE;
  size = nch->len - NSM_MSG_HEADER_SIZE;

  /* Encode the message. */
  nbytes = nsm_encode_ilm_gen_lookup (&pnt, &size, msg);
  if (nbytes < 0)
    return nbytes;

  /* Send the message. */
  nbytes = nsm_client_send_message (nch, vr_id, vrf_id,
                                    NSM_MSG_ILM_LOOKUP,
                                    nbytes, &msg_id);
  if (nbytes < 0)
    return nbytes;

  do
    {
      /* Sync wait for reply. */
      nbytes = nsm_client_read_sync (nch->mc, NULL, nch->mc->sock,
                                     &header, &ret_type);
      if (nbytes < 0)
        return nbytes;

      if ((ret_type != NSM_MSG_ILM_LOOKUP) ||
          (header.message_id != msg_id))
        nsm_client_pending_message (nch, &header);
    }
  while ((ret_type != NSM_MSG_ILM_LOOKUP) ||
         (header.message_id != msg_id));

  /* At here read thread may already registered as readable.
     Re-register read thread avoid hang.  */
  message_client_read_reregister (nch->mc);

  pal_mem_set (reply, 0, sizeof (struct nsm_msg_ilm_gen_lookup));

  /* Set pnt and size. */
  pnt = nch->buf_in + NSM_MSG_HEADER_SIZE;
  size = nch->size_in;

  /* Decode route */
  ret = nsm_decode_ilm_gen_lookup (&pnt, &size, reply);
  if (ret < 0)
    return ret;

  if (nc->debug)
    nsm_ilm_lookup_gen_dump (nc->zg, reply);

  /* Launch event to process pending requests. */
  zg = nch->nc->zg;
  if (!zg->pend_read_thread)
    zg->pend_read_thread
      = thread_add_read_pend (zg, nsm_client_process_pending_msg, nch, 0);

  return 0;
}

#ifdef HAVE_PACKET
s_int32_t
nsm_client_ilm_lookup (struct nsm_client *nc,
                       struct nsm_msg_ilm_lookup *msg,
                       struct nsm_msg_ilm_lookup *reply)
{
  struct nsm_client_handler *nch;
  struct nsm_msg_header header;
  struct lib_globals *zg;
  u_int32_t msg_id;
  u_int16_t size;
  u_char *pnt;
  int nbytes;
  int ret_type = -1;
  int ret;
  u_int32_t vr_id = 0;
  vrf_id_t vrf_id = 0;

  /* ILM entry lookup is provided as sync on the async socket. */
  nch = nc->async;

  if (! nch || ! nch->up)
    return -1;

  /* Do not allow sync messages on HA standby */
#ifdef HAVE_HA
  if (HA_IS_STANDBY (nc->zg))
    return 0;
#endif /* HAVE_HA */

  pnt = nch->buf + NSM_MSG_HEADER_SIZE;
  size = nch->len - NSM_MSG_HEADER_SIZE;

  /* Encode the message. */
  nbytes = nsm_encode_ilm_lookup (&pnt, &size, msg);
  if (nbytes < 0)
    return nbytes;

  /* Send the message. */
  nbytes = nsm_client_send_message (nch, vr_id, vrf_id,
                                    NSM_MSG_ILM_LOOKUP,
                                    nbytes, &msg_id);
  if (nbytes < 0)
    return nbytes;

  do
    {
      /* Sync wait for reply. */
      nbytes = nsm_client_read_sync (nch->mc, NULL, nch->mc->sock,
                                     &header, &ret_type);
      if (nbytes < 0)
        return nbytes;

      if ((ret_type != NSM_MSG_ILM_LOOKUP) ||
          (header.message_id != msg_id))
        nsm_client_pending_message (nch, &header);
    }
  while ((ret_type != NSM_MSG_ILM_LOOKUP) ||
         (header.message_id != msg_id));

  /* At here read thread may already registered as readable.
     Re-register read thread avoid hang.  */
  message_client_read_reregister (nch->mc);

  pal_mem_set (reply, 0, sizeof (struct nsm_msg_ilm_lookup));

  /* Set pnt and size. */
  pnt = nch->buf_in + NSM_MSG_HEADER_SIZE;
  size = nch->size_in;

  /* Decode route */
  ret = nsm_decode_ilm_lookup (&pnt, &size, reply);
  if (ret < 0)
    return ret;

  if (nc->debug)
    nsm_ilm_lookup_dump (nc->zg, reply);

  /* Launch event to process pending requests. */
  zg = nch->nc->zg;
  if (!zg->pend_read_thread)
    zg->pend_read_thread
      = thread_add_read_pend (zg, nsm_client_process_pending_msg, nch, 0);

  return 0;
}
#endif /* HAVE_PACKET */

s_int32_t
nsm_client_send_igp_shortcut_route (u_int32_t vr_id, vrf_id_t vrf_id,
                                    struct nsm_client *nc,
                                    struct nsm_msg_igp_shortcut_route *msg)
{
  struct nsm_client_handler *nch = nc->async;
  u_int16_t size;
  u_char *pnt;
  int nbytes;
  u_int32_t msg_id;

  pnt = nch->buf + NSM_MSG_HEADER_SIZE;
  size = nch->len - NSM_MSG_HEADER_SIZE;

  nbytes = nsm_encode_igp_shortcut_route (&pnt, &size, msg);
  nsm_client_send_message (nch, vr_id, vrf_id, NSM_MSG_MPLS_IGP_SHORTCUT_ROUTE,
                           nbytes, &msg_id);

  return 0;
}
#endif /* HAVE_MPLS */

/* Send IPv4 nexthop lookup message(sync). */
s_int32_t
nsm_client_route_lookup_ipv4 (u_int32_t vr_id, vrf_id_t vrf_id,
                              struct nsm_client *nc,
                              struct nsm_msg_route_lookup_ipv4 *msg,
                              int lookup_type,
                              struct nsm_msg_route_ipv4 *route)
{
  struct nsm_client_handler *nch;
  struct nsm_msg_header header;
  struct lib_globals *zg;
  u_int32_t msg_id;
  u_int16_t size;
  u_char *pnt;
  int nbytes;
  int lookup_msg_type;
  int ret_type = -1;
  int ret;

  /* Do not allow sync messages on HA standby */
#ifdef HAVE_HA
  if (HA_IS_STANDBY (nc->zg))
    return 0;
#endif /* HAVE_HA */

  /* Get message type. */
  switch (lookup_type)
    {
    case BEST_MATCH_LOOKUP:
      lookup_msg_type = NSM_MSG_IPV4_NEXTHOP_BEST_LOOKUP;
      break;
    case EXACT_MATCH_LOOKUP:
      lookup_msg_type = NSM_MSG_IPV4_NEXTHOP_EXACT_LOOKUP;
      break;
    default:
      return -1;
    }

  nch = nc->async;

  if (! nch || ! nch->up)
    return -1;

  /* Set pnt and size.  */
  pnt = nch->buf + NSM_MSG_HEADER_SIZE;
  size = nch->len - NSM_MSG_HEADER_SIZE;

  /* Encode service.  */
  nbytes = nsm_encode_ipv4_route_lookup (&pnt, &size, msg);
  if (nbytes < 0)
    return nbytes;

  /* Send message. */
  nbytes = nsm_client_send_message (nch, vr_id, vrf_id,
                                    lookup_msg_type, nbytes, &msg_id);
  if (nbytes < 0)
    return nbytes;

  do
    {
      /* Sync wait for reply. */
      nbytes = nsm_client_read_sync (nch->mc, NULL, nch->mc->sock, &header,
                                     &ret_type);
      if (nbytes < 0)
        return nbytes;

      if ((ret_type != NSM_MSG_ROUTE_IPV4) || (header.message_id != msg_id))
        nsm_client_pending_message (nch, &header);
    }
  while ((ret_type != NSM_MSG_ROUTE_IPV4) || (header.message_id != msg_id));

  /* At here read thread may already registered as readable.
     Re-register read thread avoid hang.  */
  message_client_read_reregister (nch->mc);

  pal_mem_set (route, 0, sizeof (struct nsm_msg_route_ipv4));

  /* Set pnt and size. */
  pnt = nch->buf_in + NSM_MSG_HEADER_SIZE;
  size = nch->size_in;

  /* Decode route */
  ret = nsm_decode_route_ipv4 (&pnt, &size, route);
  if (ret < 0)
    {
      return -1;
    }

  if (nc->debug)
    nsm_route_ipv4_dump (nc->zg, route);

  /* Launch event to process pending requests. */
  zg = nch->nc->zg;
  if (!zg->pend_read_thread)
    zg->pend_read_thread
      = thread_add_read_pend (zg, nsm_client_process_pending_msg, nch, 0);

  return 0;
}

s_int32_t
nsm_client_register_route_lookup_ipv4 (u_int32_t vr_id,
                                       u_int32_t vrf_id,
                                       struct nsm_client *nc,
                                       struct nsm_msg_route_lookup_ipv4 *msg,
                                       u_int32_t lookup_type)
{
  struct nsm_client_handler *nch;
  u_int32_t msg_id;
  u_int16_t size;
  u_char *pnt;
  int nbytes;
  int lookup_msg_type;

  /* Get message type. */
  switch (lookup_type)
    {
    case BEST_MATCH_LOOKUP:
      lookup_msg_type = NSM_MSG_IPV4_NEXTHOP_BEST_LOOKUP_REG;
      break;
    case EXACT_MATCH_LOOKUP:
      lookup_msg_type = NSM_MSG_IPV4_NEXTHOP_EXACT_LOOKUP_REG;
      break;
    default:
      return -1;
    }

  nch = nc->async;

  if (! nch || ! nch->up)
    return -1;

  /* Set pnt and size.  */
  pnt = nch->buf + NSM_MSG_HEADER_SIZE;
  size = nch->len - NSM_MSG_HEADER_SIZE;

  /* Encode service.  */
  nbytes = nsm_encode_ipv4_route_lookup (&pnt, &size, msg);
  if (nbytes < 0)
    return nbytes;

  /* Send message. */
  return nsm_client_send_message (nch, vr_id, vrf_id,
                                  lookup_msg_type, nbytes, &msg_id);
}

s_int32_t
nsm_client_deregister_route_lookup_ipv4 (u_int32_t vr_id,
                                         u_int32_t vrf_id,
                                         struct nsm_client *nc,
                                         struct nsm_msg_route_lookup_ipv4 *msg,
                                         u_int32_t lookup_type)
{
  struct nsm_client_handler *nch;
  u_int32_t msg_id;
  u_int16_t size;
  u_char *pnt;
  int lookup_msg_type;
  int nbytes;

  /* Get message type. */
  switch (lookup_type)
    {
    case BEST_MATCH_LOOKUP:
      lookup_msg_type = NSM_MSG_IPV4_NEXTHOP_BEST_LOOKUP_DEREG;
      break;
    case EXACT_MATCH_LOOKUP:
      lookup_msg_type = NSM_MSG_IPV4_NEXTHOP_EXACT_LOOKUP_DEREG;
      break;
    default:
      return -1;
    }

  nch = nc->async;

  if (! nch || ! nch->up)
    return -1;

  /* Set pnt and size.  */
  pnt = nch->buf + NSM_MSG_HEADER_SIZE;
  size = nch->len - NSM_MSG_HEADER_SIZE;

  /* Encode service.  */
  nbytes = nsm_encode_ipv4_route_lookup (&pnt, &size, msg);
  if (nbytes < 0)
    return nbytes;

  /* Send message. */
  return nsm_client_send_message (nch, vr_id, vrf_id,
                                  lookup_msg_type, nbytes, &msg_id);
}

#ifdef HAVE_IPV6
/* Send IPv6 nexthop lookup message(sync). */
s_int32_t
nsm_client_route_lookup_ipv6 (u_int32_t vr_id, vrf_id_t vrf_id,
                              struct nsm_client *nc,
                              struct nsm_msg_route_lookup_ipv6 *msg,
                              int lookup_type,
                              struct nsm_msg_route_ipv6 *route)
{
  struct nsm_client_handler *nch;
  struct nsm_msg_header header;
  struct lib_globals *zg;
  u_int32_t msg_id;
  u_int16_t size;
  u_char *pnt;
  int nbytes;
  int lookup_msg_type = 0;
  int ret_type = -1;
  int ret;

  /* Do not allow sync messages on HA standby */
#ifdef HAVE_HA
  if (HA_IS_STANDBY (nc->zg))
    return 0;
#endif /* HAVE_HA */

  /* Get message type. */
  switch (lookup_type)
    {
    case BEST_MATCH_LOOKUP:
      lookup_msg_type = NSM_MSG_IPV6_NEXTHOP_BEST_LOOKUP;
      break;
    case EXACT_MATCH_LOOKUP:
      lookup_msg_type = NSM_MSG_IPV6_NEXTHOP_EXACT_LOOKUP;
      break;
    }

  nch = nc->async;

  if (! nch || ! nch->up)
    return -1;

  /* Set pnt and size.  */
  pnt = nch->buf + NSM_MSG_HEADER_SIZE;
  size = nch->len - NSM_MSG_HEADER_SIZE;

  /* Encode service.  */
  nbytes = nsm_encode_ipv6_route_lookup (&pnt, &size, msg);
  if (nbytes < 0)
    return nbytes;

  /* Send message. */
  nbytes = nsm_client_send_message (nch, vr_id, vrf_id,
                                    lookup_msg_type, nbytes, &msg_id);
  if (nbytes < 0)
    return nbytes;

  do
    {
      /* Sync wait for reply. */
      nbytes = nsm_client_read_sync (nch->mc, NULL, nch->mc->sock,
                                     &header, &ret_type);
      if (nbytes < 0)
        return nbytes;

      if ((ret_type != NSM_MSG_ROUTE_IPV6) || (header.message_id != msg_id))
        nsm_client_pending_message (nch, &header);
    }
  while ((ret_type != NSM_MSG_ROUTE_IPV6) || (header.message_id != msg_id));

  /* At here read thread may already registered as readable.
     Re-register read thread avoid hang.  */
  message_client_read_reregister (nch->mc);

  pal_mem_set (route, 0, sizeof (struct nsm_msg_route_ipv6));

  /* Set pnt and size. */
  pnt = nch->buf_in + NSM_MSG_HEADER_SIZE;
  size = nch->size_in;

  /* Decode route */
  ret = nsm_decode_route_ipv6 (&pnt, &size, route);
  if (ret < 0)
    {
      return -1;
    }

  if (nc->debug)
    nsm_route_ipv6_dump ( nc->zg, route);

  /* Launch event to process pending requests. */
  zg = nch->nc->zg;
  if (!zg->pend_read_thread)
    zg->pend_read_thread
      = thread_add_read_pend (zg, nsm_client_process_pending_msg, nch, 0);

  return 0;
}

s_int32_t
nsm_client_register_route_lookup_ipv6 (u_int32_t vr_id,
                                       u_int32_t vrf_id,
                                       struct nsm_client *nc,
                                       struct nsm_msg_route_lookup_ipv6 *msg,
                                       u_int32_t lookup_type)
{
  struct nsm_client_handler *nch;
  u_int32_t msg_id;
  u_int16_t size;
  u_char *pnt;
  int lookup_msg_type = 0;
  int nbytes;

  /* Get message type. */
  switch (lookup_type)
    {
    case BEST_MATCH_LOOKUP:
      lookup_msg_type = NSM_MSG_IPV6_NEXTHOP_BEST_LOOKUP_REG;
      break;
    case EXACT_MATCH_LOOKUP:
      lookup_msg_type = NSM_MSG_IPV6_NEXTHOP_EXACT_LOOKUP_REG;
      break;
    }

  nch = nc->async;

  if (! nch || ! nch->up)
    return -1;

  /* Set pnt and size.  */
  pnt = nch->buf + NSM_MSG_HEADER_SIZE;
  size = nch->len - NSM_MSG_HEADER_SIZE;

  /* Encode service. */
  nbytes = nsm_encode_ipv6_route_lookup (&pnt, &size, msg);
  if (nbytes < 0)
    return nbytes;

  /* Send message. */
  return nsm_client_send_message (nch, vr_id, vrf_id,
                                  lookup_msg_type, nbytes, &msg_id);
}

s_int32_t
nsm_client_deregister_route_lookup_ipv6 (u_int32_t vr_id,
                                         u_int32_t vrf_id,
                                         struct nsm_client *nc,
                                         struct nsm_msg_route_lookup_ipv6 *msg,
                                         u_int32_t lookup_type)
{
  struct nsm_client_handler *nch;
  u_int32_t msg_id;
  u_int16_t size;
  u_char *pnt;
  int lookup_msg_type = 0;
  int nbytes;

  /* Get message type. */
  switch (lookup_type)
    {
    case BEST_MATCH_LOOKUP:
      lookup_msg_type = NSM_MSG_IPV6_NEXTHOP_BEST_LOOKUP_DEREG;
      break;
    case EXACT_MATCH_LOOKUP:
      lookup_msg_type = NSM_MSG_IPV6_NEXTHOP_EXACT_LOOKUP_DEREG;
      break;
    }

  nch = nc->async;

  if (! nch || ! nch->up)
    return -1;

  /* Set pnt and size.  */
  pnt = nch->buf + NSM_MSG_HEADER_SIZE;
  size = nch->len - NSM_MSG_HEADER_SIZE;

  /* Encode service.  */
  nbytes = nsm_encode_ipv6_route_lookup (&pnt, &size, msg);
  if (nbytes < 0)
    return nbytes;

  /* Send message. */
  return nsm_client_send_message (nch, vr_id, vrf_id,
                                  lookup_msg_type, nbytes, &msg_id);
}

#endif /* HAVE_IPV6 */

#ifdef HAVE_CRX
/* Send route clean message. */
s_int32_t
nsm_client_send_route_clean (struct nsm_client *nc,
                             struct nsm_msg_route_clean *msg)
{
  u_char *pnt;
  u_int16_t size;
  int nbytes;
  struct nsm_client_handler *nch = nc->async;
  u_int32_t msg_id;
  u_int32_t vr_id = 0;
  vrf_id_t vrf_id = 0;

  /* We use async connection for IPv6 route updates.  */
  if (! nch || ! nch->up)
    return -1;

  /* Set pnt and size.  */
  pnt = nch->buf + NSM_MSG_HEADER_SIZE;
  size = nch->len - NSM_MSG_HEADER_SIZE;

  /* Encode route clean message. */
  nbytes = nsm_encode_route_clean (&pnt, &size, msg);
  if (nbytes < 0)
    return nbytes;

  return nsm_client_send_message (nch, vr_id, vrf_id,
                                  NSM_MSG_ROUTE_CLEAN, nbytes, &msg_id);
}

/* Send protocol start message. */
s_int32_t
nsm_client_send_protocol_restart (struct nsm_client *nc,
                                  struct nsm_msg_protocol_restart *msg)
{
  u_char *pnt;
  u_int16_t size;
  int nbytes;
  struct nsm_client_handler *nch = nc->async;
  u_int32_t msg_id;
  u_int32_t vr_id = 0;
  vrf_id_t vrf_id = 0;

  /* We use async connection.  */
  if (! nch || ! nch->up)
    return -1;

  /* Set pnt and size.  */
  pnt = nch->buf + NSM_MSG_HEADER_SIZE;
  size = nch->len - NSM_MSG_HEADER_SIZE;

  /* Encode route clean message. */
  nbytes = nsm_encode_protocol_restart (&pnt, &size, msg);
  if (nbytes < 0)
    return nbytes;

  return nsm_client_send_message (nch, vr_id, vrf_id,
                                  NSM_MSG_PROTOCOL_RESTART, nbytes, &msg_id);
}
#endif /* HAVE_CRX */

s_int32_t
nsm_client_reconnect (struct thread *t)
{
  struct nsm_client *nc;

  nc = THREAD_ARG (t);
  nc->t_connect = NULL;
  nsm_client_start (nc);
  return 0;
}

/* Start to connect NSM services.  This function always return success. */
s_int32_t
nsm_client_start (struct nsm_client *nc)
{
  int ret;

  if (nc->async)
    {
      ret = message_client_start (nc->async->mc);
      if (ret < 0)
        {
          /* Start reconnect timer.  */
          if (nc->t_connect == NULL)
            {
              nc->t_connect
                = thread_add_timer (nc->zg, nsm_client_reconnect,
                                    nc, nc->reconnect_interval);
              if (nc->t_connect == NULL)
                return -1;
            }
        }
    }
  return 0;
}

/* Stop NSM client. */
void
nsm_client_stop (struct nsm_client *nc)
{
  if (nc->async)
    message_client_stop (nc->async->mc);
}

/* Client connection is established.  Client send service description
   message to the server.  */
s_int32_t
nsm_client_connect (struct message_handler *mc, struct message_entry *me,
                    pal_sock_handle_t sock)
{
  struct nsm_client_handler *nch = mc->info;
  struct thread t;

  nch->up = 1;

  /* Send service message to NSM server.  */
  nsm_client_send_service (nch);

  /* Always read service message synchronously */
  THREAD_ARG (&t) = mc;
  THREAD_FD (&t) = mc->sock;
  message_client_read (&t);

  return 0;
}

/* Reconnect to NSM. */
s_int32_t
nsm_client_reconnect_start (struct nsm_client *nc)
{
  /* Start reconnect timer.  */
  nc->t_connect = thread_add_timer (nc->zg, nsm_client_reconnect,
                                    nc, nc->reconnect_interval);
  if (! nc->t_connect)
    return -1;

  return 0;
}

s_int32_t
nsm_client_disconnect (struct message_handler *mc, struct message_entry *me,
                       pal_sock_handle_t sock)
{
  struct nsm_client_handler *nch;
  struct nsm_client *nc;
  struct listnode *node, *next;

  nch = mc->info;
  nc = nch->nc;

  /* Set status to down.  */
  nch->up = 0;

  /* Clean up client ID.  */
  nsm_client_id_unset (nc);

  /* Cancel pending read thread. */
  if (nc->zg->pend_read_thread)
    THREAD_OFF (nc->zg->pend_read_thread);

  /* Free all pending reads. */
  for (node = LISTHEAD (&nch->pend_msg_list); node; node = next)
    {
      struct nsm_client_pend_msg *pmsg = GETDATA (node);

      next = node->next;

      XFREE (MTYPE_NSM_PENDING_MSG, pmsg);
      list_delete_node (&nch->pend_msg_list, node);
    }

  /* Stop async connection.  */
  if (nc->async)
    message_client_stop (nc->async->mc);

  /* Unset the shutdown flag */
  UNSET_FLAG (nc->nsm_server_flags, NSM_SERVER_SHUTDOWN);

  /* Call client specific disconnect handler. */
  if (nc->disconnect_callback)
    nc->disconnect_callback ();
  else
    nsm_client_reconnect_start (nc);

  return 0;
}

struct nsm_client_handler *
nsm_client_handler_create (struct nsm_client *nc, int type)
{
  struct nsm_client_handler *nch;
  struct message_handler *mc;

  /* Allocate NSM client handler.  */
  nch = XCALLOC (MTYPE_NSM_CLIENT_HANDLER, sizeof (struct nsm_client_handler));
  nch->type = type;
  nch->nc = nc;

  /* Set max message length.  */
  nch->len = NSM_MESSAGE_MAX_LEN;
  nch->len_in = NSM_MESSAGE_MAX_LEN;
  nch->len_ipv4 = NSM_MESSAGE_MAX_LEN;
#ifdef HAVE_IPV6
  nch->len_ipv6 = NSM_MESSAGE_MAX_LEN;
#endif /* HAVE_IPV6 */

  /* Create async message client. */
  mc = message_client_create (nc->zg, type);

#ifndef HAVE_TCP_MESSAGE
  /* Use UNIX domain socket connection.  */
  message_client_set_style_domain (mc, NSM_SERV_PATH);
#else /* HAVE_TCP_MESSAGE */
  message_client_set_style_tcp (mc, NSM_PORT);
#endif /* !HAVE_TCP_MESSAGE */

  /* Initiate connection using NSM connection manager.  */
  message_client_set_callback (mc, MESSAGE_EVENT_CONNECT,
                               nsm_client_connect);
  message_client_set_callback (mc, MESSAGE_EVENT_DISCONNECT,
                               nsm_client_disconnect);
  message_client_set_callback (mc, MESSAGE_EVENT_READ_MESSAGE,
                               nsm_client_read_msg);

#ifdef HAVE_MPLS
  /* Register MPLS reply callbacks. */
  nsm_client_set_callback (nc, NSM_MSG_MPLS_FTN_REPLY,
                           nsm_ftn_reply_callback);
  nsm_client_set_callback (nc, NSM_MSG_MPLS_ILM_REPLY,
                           nsm_ilm_reply_callback);

  nsm_client_set_callback (nc, NSM_MSG_MPLS_ILM_GEN_REPLY,
                           nsm_ilm_reply_gen_callback);
#endif /* HAVE_MPLS */
#ifdef HAVE_GMPLS
  nsm_client_set_callback (nc, NSM_MSG_MPLS_BIDIR_FTN_REPLY,
                           nsm_ftn_bidir_reply_callback);
  nsm_client_set_callback (nc, NSM_MSG_MPLS_BIDIR_ILM_REPLY,
                           nsm_ilm_bidir_reply_callback);
#endif /* HAVE_GMPLS */
#ifdef HAVE_MPLS_VC
#if 0
  nsm_client_set_callback (nc, NSM_MSG_MPLS_VC_FTN_REPLY,
                           nsm_vc_ftn_reply_callback);
  nsm_client_set_callback (nc, NSM_MSG_MPLS_VC_ILM_REPLY,
                           nsm_vc_ilm_reply_callback);
#endif /* Remove code */
#endif /* HAVE_MPLS_VC */

  /* Link each other.  */
  nch->mc = mc;
  mc->info = nch;

  nch->pnt = nch->buf;
  nch->pnt_in = nch->buf_in;

  return nch;
}

s_int32_t
nsm_client_handler_free (struct nsm_client_handler *nch)
{
  THREAD_TIMER_OFF (nch->t_ipv4);
#ifdef HAVE_IPV6
  THREAD_TIMER_OFF (nch->t_ipv6);
#endif /* HAVE_IPV6 */

  if (nch->mc)
    message_client_delete (nch->mc);

  XFREE (MTYPE_NSM_CLIENT_HANDLER, nch);

  return 0;
}

/* Set service type flag.  */
s_int32_t
nsm_client_set_service (struct nsm_client *nc, int service)
{
  int type;

  if (service >= NSM_SERVICE_MAX)
    return NSM_ERR_INVALID_SERVICE;

  /* Set service bit to NSM client.  */
  NSM_SET_CTYPE (nc->service.bits, service);

  /* Check the service is sync or async.  */
  type = nsm_service_type[service].type;

  /* Create client hander corresponding to message type.  */
  if (type == MESSAGE_TYPE_ASYNC)
    {
      if (! nc->async)
        {
          nc->async = nsm_client_handler_create (nc, MESSAGE_TYPE_ASYNC);
          nc->async->service.version = nc->service.version;
          nc->async->service.protocol_id = nc->service.protocol_id;
        }
      NSM_SET_CTYPE (nc->async->service.bits, service);
    }

  return 0;
}

void
nsm_client_set_version (struct nsm_client *nc, u_int16_t version)
{
  nc->service.version = version;
}

void
nsm_client_set_protocol (struct nsm_client *nc, u_int32_t protocol_id)
{
  nc->service.protocol_id = protocol_id;
}

/* Register callback.  */
void
nsm_client_set_callback (struct nsm_client *nc, int message_type,
                         NSM_CALLBACK callback)
{
  nc->callback[message_type] = callback;
}

/* Register disconnect callback. */
void
nsm_client_set_disconnect_callback (struct nsm_client *nc,
                                    NSM_DISCONNECT_CALLBACK callback)
{
  nc->disconnect_callback = callback;
}

/* Initialize NSM client.  This function allocate NSM client
   memory.  */
struct nsm_client *
nsm_client_create (struct lib_globals *zg, u_int32_t debug)
{
  struct nsm_client *nc;

  nc = XCALLOC (MTYPE_NSM_CLIENT, sizeof (struct nsm_client));
  if (! nc)
    return NULL;
  nc->zg = zg;
  zg->nc = nc;

  /* Set parsers. */
  nsm_client_set_parser (nc, NSM_MSG_SERVICE_REPLY, nsm_parse_service);
  nsm_client_set_parser (nc, NSM_MSG_LINK_ADD, nsm_parse_link);
  nsm_client_set_parser (nc, NSM_MSG_LINK_DELETE, nsm_parse_link);
  nsm_client_set_parser (nc, NSM_MSG_VR_SYNC_DONE, nsm_parse_vr_done);
  nsm_client_set_parser (nc, NSM_MSG_LINK_ATTR_UPDATE, nsm_parse_link);
  nsm_client_set_parser (nc, NSM_MSG_LINK_UP, nsm_parse_link);
  nsm_client_set_parser (nc, NSM_MSG_LINK_DOWN, nsm_parse_link);
  nsm_client_set_parser (nc, NSM_MSG_ADDR_ADD, nsm_parse_address);
  nsm_client_set_parser (nc, NSM_MSG_ADDR_DELETE, nsm_parse_address);
  nsm_client_set_parser (nc, NSM_MSG_VR_ADD, nsm_parse_vr);
  nsm_client_set_parser (nc, NSM_MSG_VR_DELETE, nsm_parse_vr);
  nsm_client_set_parser (nc, NSM_MSG_VR_BIND, nsm_parse_vr_bind);
  nsm_client_set_parser (nc, NSM_MSG_VR_UNBIND, nsm_parse_vr_bind);
  nsm_client_set_parser (nc, NSM_MSG_VRF_ADD, nsm_parse_vrf);
  nsm_client_set_parser (nc, NSM_MSG_VRF_DELETE, nsm_parse_vrf);
  nsm_client_set_parser (nc, NSM_MSG_VRF_BIND, nsm_parse_vrf_bind);
  nsm_client_set_parser (nc, NSM_MSG_VRF_UNBIND, nsm_parse_vrf_bind);
  nsm_client_set_parser (nc, NSM_MSG_NSM_SERVER_STATUS, 
                         nsm_parse_nsm_server_status_notify);

  nsm_client_set_parser (nc, NSM_MSG_ROUTE_IPV4, nsm_parse_route_ipv4);
#ifdef HAVE_IPV6
  nsm_client_set_parser (nc, NSM_MSG_ROUTE_IPV6, nsm_parse_route_ipv6);
#endif /* HAVE_IPV6 */

  nsm_client_set_parser (nc, NSM_MSG_ROUTE_PRESERVE, nsm_parse_route_manage);
  nsm_client_set_parser (nc, NSM_MSG_ROUTE_STALE_REMOVE,
                         nsm_parse_route_manage);
  nsm_client_set_parser (nc, NSM_MSG_ROUTE_STALE_MARK,
                         nsm_parse_route_manage);
  nsm_client_set_parser (nc, NSM_MSG_ISIS_BGP_CONV_DONE,
                         nsm_parse_bgp_conv_done);
  nsm_client_set_parser (nc, NSM_MSG_ISIS_BGP_UP, nsm_parse_bgp_up_down);
  nsm_client_set_parser (nc, NSM_MSG_ISIS_BGP_DOWN, nsm_parse_bgp_up_down);
  nsm_client_set_parser (nc, NSM_MSG_LDP_UP, nsm_parse_ldp_up_down);
  nsm_client_set_parser (nc, NSM_MSG_LDP_DOWN, nsm_parse_ldp_up_down);
  nsm_client_set_parser (nc, NSM_MSG_LDP_SESSION_STATE, nsm_parse_ldp_session_state);
  nsm_client_set_parser (nc, NSM_MSG_LDP_SESSION_QUERY, nsm_parse_ldp_session_state);

#ifdef HAVE_MPLS
  nsm_client_set_parser (nc, NSM_MSG_MPLS_FTN_REPLY, nsm_parse_ftn_reply);
  nsm_client_set_parser (nc, NSM_MSG_MPLS_ILM_REPLY, nsm_parse_ilm_reply);
  nsm_client_set_parser (nc, NSM_MSG_MPLS_ILM_GEN_REPLY,
                         nsm_parse_ilm_gen_reply);
  nsm_client_set_parser (nc, NSM_MSG_MPLS_NOTIFICATION,
                         nsm_parse_mpls_notification);
  nsm_client_set_parser (nc, NSM_MSG_LABEL_POOL_REQUEST, nsm_parse_label_pool);
  nsm_client_set_parser (nc, NSM_MSG_LABEL_POOL_RELEASE, nsm_parse_label_pool);
  nsm_client_set_parser (nc, NSM_MSG_MPLS_IGP_SHORTCUT_LSP,
                         nsm_parse_igp_shortcut_lsp);
#endif /* HAVE_MPLS */

#ifdef HAVE_GMPLS
  nsm_client_set_parser (nc, NSM_MSG_MPLS_BIDIR_FTN_REPLY, 
                         nsm_parse_ftn_bidir_reply);
  nsm_client_set_parser (nc, NSM_MSG_MPLS_BIDIR_ILM_REPLY,
                         nsm_parse_ilm_bidir_reply);
#ifdef HAVE_PBB_TE
  nsm_client_set_parser (nc, NSM_MSG_PBB_TESID_INFO,
                         nsm_parse_te_tesi_msg);
#endif /* HAVE_PBB_TE */
#endif /* HAVE_GMPLS */

#ifndef HAVE_IMI
  nsm_client_set_parser (nc, NSM_MSG_USER_ADD, nsm_parse_user);
  nsm_client_set_parser (nc, NSM_MSG_USER_DELETE, nsm_parse_user);
  nsm_client_set_parser (nc, NSM_MSG_USER_UPDATE, nsm_parse_user);
#endif /* HAVE_IMI */

#ifdef HAVE_TE
  nsm_client_set_parser (nc, NSM_MSG_INTF_PRIORITY_BW_UPDATE,
                         nsm_parse_link);
  nsm_client_set_parser (nc, NSM_MSG_ADMIN_GROUP_UPDATE,
                         nsm_parse_admin_group);
  nsm_client_set_parser (nc, NSM_MSG_QOS_CLIENT_PREEMPT,
                         nsm_parse_qos_preempt);
  nsm_client_set_parser (nc, NSM_MSG_QOS_CLIENT_PROBE, nsm_parse_qos);
  nsm_client_set_parser (nc, NSM_MSG_QOS_CLIENT_RESERVE, nsm_parse_qos);
  nsm_client_set_parser (nc, NSM_MSG_QOS_CLIENT_MODIFY, nsm_parse_qos);
#ifdef HAVE_GMPLS
  nsm_client_set_parser (nc, NSM_MSG_GMPLS_QOS_CLIENT_PREEMPT,
                         nsm_parse_gmpls_qos_preempt);
  nsm_client_set_parser (nc, NSM_MSG_GMPLS_QOS_CLIENT_PROBE, nsm_parse_gmpls_qos);
  nsm_client_set_parser (nc, NSM_MSG_GMPLS_QOS_CLIENT_RESERVE, nsm_parse_gmpls_qos);
  nsm_client_set_parser (nc, NSM_MSG_GMPLS_QOS_CLIENT_MODIFY, nsm_parse_gmpls_qos);
#endif /*HAVE_GMPLS*/
#endif /* HAVE_TE */

#ifdef HAVE_MPLS_VC
#if 0
  nsm_client_set_parser (nc, NSM_MSG_MPLS_VC_FTN_REPLY,
                         nsm_parse_vc_ftn_reply);
  nsm_client_set_parser (nc, NSM_MSG_MPLS_VC_ILM_REPLY,
                         nsm_parse_vc_ilm_reply);
#endif
  nsm_client_set_parser (nc, NSM_MSG_MPLS_VC_ADD, nsm_parse_mpls_vc);
  nsm_client_set_parser (nc, NSM_MSG_MPLS_VC_DELETE, nsm_parse_mpls_vc);
  nsm_client_set_parser (nc, NSM_MSG_MPLS_PW_STATUS, nsm_parse_pw_status);
#ifdef HAVE_MS_PW
  nsm_client_set_parser (nc, NSM_MSG_MPLS_MS_PW, nsm_parse_mpls_ms_pw_msg);
#endif /* HAVE_MS_PW */
#endif /* HAVE_MPLS_VC */

#ifdef HAVE_GMPLS
  /* TBD old code nsm_client_set_parser (nc, NSM_MSG_GMPLS_IF, nsm_parse_gmpls_if); */
  nsm_client_set_parser (nc, NSM_MSG_CONTROL_CHANNEL,
                         nsm_parse_control_channel);
  nsm_client_set_parser (nc, NSM_MSG_CONTROL_ADJ, nsm_parse_control_adj);
  nsm_client_set_parser (nc, NSM_MSG_DATA_LINK, nsm_parse_data_link);
  nsm_client_set_parser (nc, NSM_MSG_DATA_LINK_SUB, nsm_parse_data_link_sub);
  nsm_client_set_parser (nc, NSM_MSG_TE_LINK, nsm_parse_te_link);
#endif /* HAVE_GMPLS */

  nsm_client_set_parser (nc, NSM_MSG_ROUTER_ID_UPDATE, nsm_parse_router_id);
#ifdef HAVE_DIFFSERV
  nsm_client_set_parser (nc, NSM_MSG_SUPPORTED_DSCP_UPDATE,
                         nsm_parse_supported_dscp_update);
  nsm_client_set_parser (nc, NSM_MSG_DSCP_EXP_MAP_UPDATE,
                         nsm_parse_dscp_exp_map_update);
#endif /* HAVE_DIFFSERV */

#ifdef HAVE_VPLS
  nsm_client_set_parser (nc, NSM_MSG_VPLS_ADD, nsm_parse_vpls_add);
  nsm_client_set_parser (nc, NSM_MSG_VPLS_DELETE, nsm_parse_vpls_delete);
  nsm_client_set_parser (nc, NSM_MSG_VPLS_MAC_WITHDRAW,
                         nsm_parse_vpls_mac_withdraw);
#endif /* HAVE_VPLS */

#ifdef HAVE_DSTE
  nsm_client_set_parser (nc, NSM_MSG_DSTE_TE_CLASS_UPDATE,
                         nsm_parse_dste_te_class_update);
#endif /* HAVE_DSTE */

#ifdef HAVE_MCAST_IPV4
  nsm_client_set_parser (nc, NSM_MSG_IPV4_MRT_NOCACHE,
                         nsm_parse_ipv4_mrt_nocache);
  nsm_client_set_parser (nc, NSM_MSG_IPV4_MRT_WRONGVIF,
                         nsm_parse_ipv4_mrt_wrongvif);
  nsm_client_set_parser (nc, NSM_MSG_IPV4_MRT_WHOLEPKT_REQ,
                         nsm_parse_ipv4_mrt_wholepkt_req);
  nsm_client_set_parser (nc, NSM_MSG_IPV4_MRT_STAT_UPDATE,
                         nsm_parse_ipv4_mrt_stat_update);
  nsm_client_set_parser (nc, NSM_MSG_IPV4_MRIB_NOTIFICATION,
                         nsm_parse_ipv4_mrib_notification);
  nsm_client_set_parser (nc, NSM_MSG_MTRACE_QUERY,
                         nsm_parse_mtrace_query_msg);
  nsm_client_set_parser (nc, NSM_MSG_MTRACE_REQUEST,
                         nsm_parse_mtrace_request_msg);
  nsm_client_set_parser (nc, NSM_MSG_IGMP_LMEM, nsm_parse_igmp);
#endif /* HAVE_MCAST_IPV4 */

#ifdef HAVE_MCAST_IPV6
  nsm_client_set_parser (nc, NSM_MSG_IPV6_MRT_NOCACHE,
                         nsm_parse_ipv6_mrt_nocache);
  nsm_client_set_parser (nc, NSM_MSG_IPV6_MRT_WRONGMIF,
                         nsm_parse_ipv6_mrt_wrongmif);
  nsm_client_set_parser (nc, NSM_MSG_IPV6_MRT_WHOLEPKT_REQ,
                         nsm_parse_ipv6_mrt_wholepkt_req);
  nsm_client_set_parser (nc, NSM_MSG_IPV6_MRT_STAT_UPDATE,
                         nsm_parse_ipv6_mrt_stat_update);
  nsm_client_set_parser (nc, NSM_MSG_IPV6_MRIB_NOTIFICATION,
                         nsm_parse_ipv6_mrib_notification);
  nsm_client_set_parser (nc, NSM_MSG_MLD_LMEM, nsm_parse_mld);
#endif /* HAVE_MCAST_IPV6 */

#ifdef HAVE_L2
  nsm_client_set_parser (nc, NSM_MSG_BRIDGE_ADD,
                         nsm_parse_bridge_msg);
  nsm_client_set_parser (nc, NSM_MSG_BRIDGE_DELETE,
                         nsm_parse_bridge_msg);
  nsm_client_set_parser (nc, NSM_MSG_BRIDGE_ADD_PORT,
                         nsm_parse_bridge_if_msg);
  nsm_client_set_parser (nc, NSM_MSG_BRIDGE_DELETE_PORT,
                         nsm_parse_bridge_if_msg);
  nsm_client_set_parser (nc, NSM_MSG_BRIDGE_PORT_SPANNING_TREE_ENABLE,
                         nsm_parse_bridge_if_msg);
  nsm_client_set_parser (nc, NSM_MSG_BRIDGE_PORT_STATE_SYNC_REQ,
                         nsm_parse_bridge_if_msg);
  nsm_client_set_parser (nc, NSM_MSG_BRIDGE_PORT_STATE,
                         nsm_parse_bridge_port_msg);
  nsm_client_set_parser (nc, NSM_MSG_BRIDGE_SET_AGEING_TIME,
                         nsm_parse_bridge_msg);
  nsm_client_set_parser (nc, NSM_MSG_SET_BPDU_PROCESS,
                         nsm_parse_bridge_if_msg);
#ifdef HAVE_PVLAN
  nsm_client_set_parser (nc, NSM_MSG_PVLAN_PORT_HOST_ASSOCIATE,
                         nsm_parse_pvlan_if_msg);
  nsm_client_set_parser (nc, NSM_MSG_PVLAN_CONFIGURE,
                         nsm_parse_pvlan_configure);
  nsm_client_set_parser (nc, NSM_MSG_PVLAN_SECONDARY_VLAN_ASSOCIATE,
                         nsm_parse_pvlan_associate);

#endif /* HAVE_PVLAN */
#ifdef HAVE_LACPD
  nsm_client_set_parser (nc, NSM_MSG_STATIC_AGG_CNT_UPDATE,
                         nsm_parse_static_agg_cnt_update);
  nsm_client_set_parser (nc, NSM_MSG_LACP_AGGREGATOR_CONFIG,
                         nsm_parse_lacp_send_agg_config);
#endif /*HAVE_LACPD*/
#ifndef HAVE_CUSTOM1
#ifdef HAVE_VLAN
  nsm_client_set_parser (nc, NSM_MSG_VLAN_ADD,
                         nsm_parse_vlan_msg);
  nsm_client_set_parser (nc, NSM_MSG_VLAN_DELETE,
                         nsm_parse_vlan_msg);
  nsm_client_set_parser (nc, NSM_MSG_VLAN_ADD_PORT,
                         nsm_parse_vlan_port_msg);
  nsm_client_set_parser (nc, NSM_MSG_VLAN_DELETE_PORT,
                         nsm_parse_vlan_port_msg);
  nsm_client_set_parser (nc, NSM_MSG_VLAN_PORT_TYPE,
                         nsm_parse_vlan_port_type_msg);
  nsm_client_set_parser (nc, NSM_MSG_SVLAN_ADD_CE_PORT,
                         nsm_parse_vlan_port_msg);
  nsm_client_set_parser (nc, NSM_MSG_SVLAN_DELETE_CE_PORT,
                         nsm_parse_vlan_port_msg);
  nsm_client_set_parser (nc, NSM_MSG_VLAN_SET_PVID,
                         nsm_parse_vlan_port_msg);
  nsm_client_set_parser (nc, NSM_MSG_UNTAGGED_VID_PE_PORT,
                         nsm_parse_vlan_port_msg);
  nsm_client_set_parser (nc, NSM_MSG_DEFAULT_VID_PE_PORT,
                         nsm_parse_vlan_port_msg);

#if defined (HAVE_I_BEB) || defined (HAVE_B_BEB)
  nsm_client_set_parser (nc,NSM_MSG_PBB_ISID_TO_PIP_ADD,
      nsm_parse_isid2svid_msg);
  nsm_client_set_parser (nc,NSM_MSG_PBB_SVID_TO_ISID_DEL,
      nsm_parse_isid2svid_msg);
  nsm_client_set_parser (nc,NSM_MSG_PBB_ISID_DEL,
      nsm_parse_isid2svid_msg);
  nsm_client_set_parser (nc,NSM_MSG_PBB_ISID_TO_BVID_ADD,
      nsm_parse_isid2bvid_msg);
  nsm_client_set_parser (nc,NSM_MSG_PBB_ISID_TO_BVID_DEL,
      nsm_parse_isid2bvid_msg);
#endif /* HAVE_I_BEB || HAVE_B_BEB  */
#ifdef HAVE_G8031
  /* Message received from NSM */
  nsm_client_set_parser (nc,NSM_MSG_G8031_CREATE_PROTECTION_GROUP,
                         nsm_parse_g8031_create_pg_msg);
  nsm_client_set_parser (nc,NSM_MSG_G8031_DEL_PROTECTION_GROUP,
                         nsm_parse_g8031_create_pg_msg);

  nsm_client_set_parser (nc,NSM_MSG_G8031_CREATE_VLAN_GROUP,
                         nsm_parse_g8031_create_vlan_msg);

  /* Message sending to NSM */
  nsm_client_set_parser (nc,NSM_MSG_G8031_PG_INITIALIZED,
                         nsm_parse_oam_pg_init_msg);

  nsm_client_set_parser (nc,NSM_MSG_G8031_PG_PORTSTATE,
                         nsm_parse_oam_g8031_portstate_msg);

  nsm_client_set_parser (nc, NSM_MSG_VLAN_ADD_TO_PROTECTION,
                         nsm_parse_vlan_msg);

  nsm_client_set_parser (nc, NSM_MSG_VLAN_DEL_FROM_PROTECTION,
                         nsm_parse_vlan_msg);
#endif /* HAVE_G8031 */
#if defined HAVE_G8031 || defined HAVE_G8032
  nsm_client_set_parser (nc, NSM_MSG_BRIDGE_ADD_PG,
                         nsm_parse_bridge_pg_msg);

  nsm_client_set_parser (nc, NSM_MSG_BRIDGE_DEL_PG,
                         nsm_parse_bridge_pg_msg);

#endif /* HAVE_G8031 */

#ifdef HAVE_G8032
   nsm_client_set_parser (nc,NSM_MSG_G8032_CREATE_VLAN_GROUP,
                          nsm_parse_g8032_create_vlan_msg);

   nsm_client_set_parser (nc,NSM_MSG_BRIDGE_G8032_PORT_STATE,
                          nsm_parse_bridge_g8032_port_msg);

#endif /*HAVE_G8032*/

#ifdef HAVE_PBB_TE
  nsm_client_set_parser (nc, NSM_MSG_PBB_TE_VID_ADD,
                         nsm_parse_te_vlan_msg);
  nsm_client_set_parser (nc, NSM_MSG_PBB_TE_VID_DELETE,
                         nsm_parse_te_vlan_msg);
  nsm_client_set_parser (nc, NSM_MSG_PBB_TE_ESP_ADD,
                         nsm_parse_te_esp_msg);
  nsm_client_set_parser (nc, NSM_MSG_PBB_TE_ESP_DELETE,
                         nsm_parse_te_esp_msg);
  nsm_client_set_parser (nc, NSM_MSG_PBB_TE_ESP_PNP_ADD,
                         nsm_parse_esp_pnp_msg);
  nsm_client_set_parser (nc, NSM_MSG_PBB_TE_ESP_PNP_DELETE,
                         nsm_parse_esp_pnp_msg);
/*  nsm_client_set_parser (nc, NSM_MSG_BRIDGE_PBB_TE_PORT_STATE,
                         nsm_parse_bridge_pbb_te_port_state_msg);*/
  nsm_client_set_parser (nc, NSM_MSG_PBB_TE_APS_GRP_ADD,
                         nsm_parse_te_aps_grp);
  nsm_client_set_parser (nc, NSM_MSG_PBB_TE_APS_GRP_DELETE,
                         nsm_parse_te_aps_grp);
#endif /* HAVE_PBB_TE */

#ifdef HAVE_VLAN_CLASS
  nsm_client_set_parser (nc, NSM_MSG_VLAN_CLASSIFIER_ADD,
                         nsm_parse_vlan_classifier_msg);
  nsm_client_set_parser (nc, NSM_MSG_VLAN_CLASSIFIER_ADD,
                         nsm_parse_vlan_classifier_msg);
  nsm_client_set_parser (nc, NSM_MSG_VLAN_PORT_CLASS_ADD,
                         nsm_parse_vlan_port_class_msg);
  nsm_client_set_parser (nc, NSM_MSG_VLAN_PORT_CLASS_DEL,
                         nsm_parse_vlan_port_class_msg);
#endif /* HAVE_VLAN_CLASS */
#endif /* HAVE_VLAN */
#endif /* HAVE_CUSTOM1 */
#ifdef HAVE_ELMID
  nsm_client_set_parser (nc, NSM_MSG_ELMI_EVC_NEW, nsm_parse_elmi_evc_msg);
  nsm_client_set_parser (nc, NSM_MSG_ELMI_EVC_DELETE, nsm_parse_elmi_evc_msg);
  nsm_client_set_parser (nc, NSM_MSG_ELMI_EVC_UPDATE, nsm_parse_elmi_evc_msg);
  nsm_client_set_parser (nc, NSM_MSG_ELMI_UNI_ADD, nsm_parse_uni_msg);
  nsm_client_set_parser (nc, NSM_MSG_ELMI_UNI_DELETE, nsm_parse_uni_msg);
  nsm_client_set_parser (nc, NSM_MSG_ELMI_UNI_UPDATE, nsm_parse_uni_msg);

  nsm_client_set_parser (nc, NSM_MSG_ELMI_OPERATIONAL_STATE,
                         nsm_parse_elmi_status_msg);

  nsm_client_set_parser (nc, NSM_MSG_ELMI_UNI_BW_ADD,
                         nsm_parse_elmi_bw_msg);

  nsm_client_set_parser (nc, NSM_MSG_ELMI_UNI_BW_DEL,
                         nsm_parse_elmi_bw_msg);

  nsm_client_set_parser (nc, NSM_MSG_ELMI_EVC_BW_ADD,
                         nsm_parse_elmi_bw_msg);

  nsm_client_set_parser (nc, NSM_MSG_ELMI_EVC_BW_DEL,
                         nsm_parse_elmi_bw_msg);

  nsm_client_set_parser (nc, NSM_MSG_ELMI_EVC_COS_BW_ADD,
                         nsm_parse_elmi_bw_msg);

  nsm_client_set_parser (nc, NSM_MSG_ELMI_EVC_COS_BW_DEL,
                         nsm_parse_elmi_bw_msg);
#endif /* HAVE_ELMID */
#endif /* HAVE_L2 */

#ifdef HAVE_ONMD
  nsm_client_set_parser (nc, NSM_MSG_EFM_OAM_IF,
                         nsm_parse_oam_efm_msg);
  nsm_client_set_parser (nc, NSM_MSG_OAM_LLDP,
             nsm_parse_oam_lldp_msg);
  nsm_client_set_parser (nc, NSM_MSG_OAM_CFM,
                         nsm_parse_oam_cfm_msg);

  nsm_client_set_parser (nc, NSM_MSG_CFM_OPERATIONAL,
                         nsm_parse_oam_cfm_status_msg);
#ifdef HAVE_G8032
  nsm_client_set_parser (nc, NSM_MSG_BRIDGE_ADD_G8032_RING,
                         nsm_parse_g8032_create_msg);
  nsm_client_set_parser (nc, NSM_MSG_BRIDGE_DEL_G8032_RING,
                         nsm_parse_g8032_delete_msg);
  nsm_client_set_parser (nc, NSM_MSG_BRIDGE_G8032_PORT_STATE,
                         nsm_parse_oam_g8032_portstate_msg);
                        // nsm_parse_bridge_g8032_port_msg);
#endif /*HAVE_G8032*/

#ifdef HAVE_CFM
#ifdef HAVE_LACPD
  nsm_client_set_parser (nc,NSM_MSG_LACP_ADD_AGGREGATOR_MEMBER,
                         nsm_parse_bridge_if_msg);
  nsm_client_set_parser (nc,NSM_MSG_LACP_DEL_AGGREGATOR_MEMBER,
                         nsm_parse_bridge_if_msg);
#endif /* HAVE_LACPD */
#endif /* HAVE_CFM */

#endif /* HAVE_ONMD */

#ifdef HAVE_MPLS
#ifdef HAVE_NSM_MPLS_OAM
  nsm_client_set_parser (nc,NSM_MSG_MPLS_PING_REPLY,
                         nsm_parse_oam_ping_response);
  nsm_client_set_parser (nc,NSM_MSG_MPLS_TRACERT_REPLY,
                         nsm_parse_oam_tracert_reply);
  nsm_client_set_parser (nc, NSM_MSG_MPLS_OAM_ERROR,
                         nsm_parse_oam_error);
#endif /* HAVE_NSM_MPLS_OAM */

#ifdef HAVE_MPLS_OAM
/* Set parser functions for echo request/reply processing
 * related response messages from NSM to OAMD.
 */
  nsm_client_set_parser (nc, NSM_MSG_OAM_LSP_PING_REQ_RESP_LDP,
                         nsm_parse_oam_ping_req_resp_ldp);

  nsm_client_set_parser (nc, NSM_MSG_OAM_LSP_PING_REQ_RESP_RSVP,
                         nsm_parse_oam_ping_req_resp_rsvp);

  nsm_client_set_parser (nc, NSM_MSG_OAM_LSP_PING_REQ_RESP_GEN,
                         nsm_parse_oam_ping_req_resp_gen);

#ifdef HAVE_MPLS_VC
  nsm_client_set_parser (nc, NSM_MSG_OAM_LSP_PING_REQ_RESP_L2VC,
                         nsm_parse_oam_ping_req_resp_l2vc);
#endif /* HAVE_MPLS_VC */

  nsm_client_set_parser (nc, NSM_MSG_OAM_LSP_PING_REQ_RESP_L3VPN,
                         nsm_parse_oam_ping_req_resp_l3vpn);

  nsm_client_set_parser (nc, NSM_MSG_OAM_LSP_PING_REQ_RESP_ERR,
                         nsm_parse_oam_ping_req_resp_err);

  nsm_client_set_parser (nc, NSM_MSG_OAM_LSP_PING_REP_RESP,
                         nsm_parse_oam_ping_rep_resp);

  nsm_client_set_parser (nc, NSM_MSG_OAM_UPDATE,
                         nsm_parse_oam_update);
#endif /* HAVE_MPLS_OAM */

#ifdef HAVE_BFD
  nsm_client_set_parser (nc, NSM_MSG_MPLS_FTN_DOWN,
                         nsm_parse_ftn_down);
#endif /* HAVE_BFD */

#ifdef HAVE_RESTART
  nsm_client_set_parser (nc, NSM_MSG_STALE_INFO,
                         nsm_parse_stale_bundle_msg);
  nsm_client_set_parser (nc, NSM_MSG_GEN_STALE_INFO,
                         nsm_parse_gen_stale_bundle_msg);
#endif /* HAVE_RESTART */

#ifdef HAVE_TE
  nsm_client_set_callback (nc, NSM_MSG_INTF_PRIORITY_BW_UPDATE,
                           nsm_util_link_priority_bw_update);
#endif /* HAVE_TE */
#ifdef HAVE_GMPLS
  /* TBD old code nsm_client_set_callback (nc, NSM_MSG_GMPLS_IF, nsm_util_link_gmpls_update); */
  nsm_client_set_callback (nc, NSM_MSG_CONTROL_CHANNEL,
                           nsm_util_control_channel_callback);
  nsm_client_set_callback (nc, NSM_MSG_CONTROL_ADJ,
                           nsm_util_control_adj_callback);
  nsm_client_set_callback (nc, NSM_MSG_DATA_LINK,
                           nsm_util_data_link_callback);
  nsm_client_set_callback (nc, NSM_MSG_DATA_LINK_SUB,
                           nsm_util_data_link_sub_callback);
  nsm_client_set_callback (nc, NSM_MSG_TE_LINK,
                           nsm_util_te_link_callback);
#endif /* HAVE_GMPLS */
#endif /* HAVE_MPLS */

#ifdef HAVE_GMPLS
#ifdef HAVE_LMP
#ifdef HAVE_RESTART
  nsm_client_set_parser (nc, NSM_MSG_LMP_GRACEFUL_RESTART,
                         nsm_parse_lmp_protocol_restart);
#endif /* HAVE_RESTART */
#endif /* HAVE_LMP */
#endif /* HAVE_GMPLS */

  /* Set callbacks. */
  nsm_client_set_callback (nc, NSM_MSG_LINK_ADD, nsm_util_link_add);
  nsm_client_set_callback (nc, NSM_MSG_LINK_DELETE, nsm_util_link_delete);
  nsm_client_set_callback (nc, NSM_MSG_LINK_UP, nsm_util_link_up);
  nsm_client_set_callback (nc, NSM_MSG_LINK_DOWN, nsm_util_link_down);
  nsm_client_set_callback (nc, NSM_MSG_LINK_ATTR_UPDATE, nsm_util_link_update);

#ifdef HAVE_L3
  nsm_client_set_callback (nc, NSM_MSG_ADDR_ADD, nsm_util_addr_add);
  nsm_client_set_callback (nc, NSM_MSG_ADDR_DELETE, nsm_util_addr_delete);
#endif /* HAVE_L3 */

  nsm_client_set_callback (nc, NSM_MSG_VR_ADD, nsm_util_vr_add);
  nsm_client_set_callback (nc, NSM_MSG_VR_DELETE, nsm_util_vr_delete);
  nsm_client_set_callback (nc, NSM_MSG_VR_BIND, nsm_util_vr_bind);
  nsm_client_set_callback (nc, NSM_MSG_VR_UNBIND, nsm_util_vr_unbind);
  nsm_client_set_callback (nc, NSM_MSG_VRF_ADD, nsm_util_vrf_add);
  nsm_client_set_callback (nc, NSM_MSG_VRF_DELETE, nsm_util_vrf_delete);
  nsm_client_set_callback (nc, NSM_MSG_VRF_BIND, nsm_util_vrf_bind);
  nsm_client_set_callback (nc, NSM_MSG_VRF_UNBIND, nsm_util_vrf_unbind);
  nsm_client_set_callback (nc, NSM_MSG_ROUTER_ID_UPDATE, nsm_util_router_id);
  nsm_client_set_callback (nc, NSM_MSG_NSM_SERVER_STATUS, 
                           nsm_util_server_status_notify);
#ifndef HAVE_IMI
  nsm_client_set_callback (nc, NSM_MSG_USER_UPDATE, nsm_util_user_update);
  nsm_client_set_callback (nc, NSM_MSG_USER_DELETE, nsm_util_user_delete);
#endif /* HAVE_IMI */

  nc->reconnect_interval = 5;

  if (debug)
    nc->debug = 1;

  return nc;
}

s_int32_t
nsm_client_delete (struct nsm_client *nc)
{
  struct listnode *node, *node_next;
  struct nsm_client_handler *nch;

  if (nc)
    {
      if (nc->t_connect)
        THREAD_OFF (nc->t_connect);

      /* Cancel pending read thread. */
      if (nc->zg->pend_read_thread)
        THREAD_OFF (nc->zg->pend_read_thread);

      nch = nc->async;
      /* Free all pending reads. */
      if (nch)
        {
          for (node = LISTHEAD (&nch->pend_msg_list); node; node = node_next)
            {
              struct nsm_client_pend_msg *pmsg = GETDATA (node);

              node_next = node->next;

              XFREE (MTYPE_NSM_PENDING_MSG, pmsg);
              list_delete_node (&nch->pend_msg_list, node);
            }

          nsm_client_handler_free (nch);
        }

      XFREE (MTYPE_NSM_CLIENT, nc);
    }
  return 0;
}


/* Utility functions for NSM client.  */
cindex_t
nsm_util_link_val_set (struct lib_globals *zg, struct nsm_msg_link *msg,
                       struct interface *ifp)
{
  ifp->ifindex = msg->ifindex;

  if (NSM_CHECK_CTYPE (msg->cindex, NSM_LINK_CTYPE_NAME))
    pal_strncpy (ifp->name, msg->ifname, IFNAMSIZ);

  if (NSM_CHECK_CTYPE (msg->cindex, NSM_LINK_CTYPE_FLAGS))
    ifp->flags = msg->flags;

  if (NSM_CHECK_CTYPE (msg->cindex, NSM_LINK_CTYPE_STATUS))
    ifp->status = msg->status;

  if (NSM_CHECK_CTYPE (msg->cindex, NSM_LINK_CTYPE_METRIC))
    ifp->metric = msg->metric;

  if (NSM_CHECK_CTYPE (msg->cindex, NSM_LINK_CTYPE_MTU))
    ifp->mtu = msg->mtu;

  if (NSM_CHECK_CTYPE (msg->cindex, NSM_LINK_CTYPE_BANDWIDTH))
    ifp->bandwidth = msg->bandwidth;

  if (NSM_CHECK_CTYPE (msg->cindex, NSM_LINK_CTYPE_DUPLEX))
    ifp->duplex = msg->duplex;

#ifdef HAVE_TE
  if (NSM_CHECK_CTYPE (msg->cindex, NSM_LINK_CTYPE_ADMIN_GROUP))
    ifp->admin_group = msg->admin_group;

  if (NSM_CHECK_CTYPE (msg->cindex, NSM_LINK_CTYPE_MAX_RESV_BANDWIDTH))
    pal_mem_cpy (&ifp->max_resv_bw, &msg->max_resv_bw, sizeof (float32_t));
#ifdef HAVE_DSTE

  if (NSM_CHECK_CTYPE (msg->cindex, NSM_LINK_CTYPE_BC_MODE))
    ifp->bc_mode = msg->bc_mode;

  if (NSM_CHECK_CTYPE (msg->cindex, NSM_LINK_CTYPE_BW_CONSTRAINT))
    pal_mem_cpy (ifp->bw_constraint, msg->bw_constraint,
                 MAX_BW_CONST * sizeof (float32_t));
#endif /* HAVE_DSTE */
#endif /* HAVE_TE */

  if (NSM_CHECK_CTYPE (msg->cindex, NSM_LINK_CTYPE_HW_ADDRESS))
    {
      ifp->hw_type = msg->hw_type;
      ifp->hw_addr_len = msg->hw_addr_len;
      if (ifp->hw_addr_len)
        pal_mem_cpy (ifp->hw_addr, msg->hw_addr, ifp->hw_addr_len);
    }

#ifdef HAVE_MPLS
  if (NSM_CHECK_CTYPE (msg->cindex, NSM_LINK_CTYPE_LABEL_SPACE))
    ifp->ls_data = msg->lp;
#endif /* HAVE_MPLS */

#ifdef HAVE_VRX
  if (NSM_CHECK_CTYPE (msg->cindex, NSM_LINK_CTYPE_VRX_FLAG))
    ifp->vrx_flag = msg->vrx_flag;

  if (NSM_CHECK_CTYPE (msg->cindex, NSM_LINK_CTYPE_VRX_FLAG_LOCALSRC))
    ifp->local_flag = msg->local_flag;
#endif /* HAVE_VRX */

#ifdef HAVE_LACPD
  if (NSM_CHECK_CTYPE (msg->cindex, NSM_LINK_CTYPE_LACP_KEY))
    {
      ifp->lacp_admin_key = msg->lacp_admin_key;
      ifp->agg_param_update = msg->agg_param_update;
      ifp->lacp_agg_key = msg->lacp_agg_key;
    }
#endif /* HAVE_LACPD */

#ifdef HAVE_GMPLS
  ifp->gifindex = msg->gifindex;

  if (NSM_CHECK_CTYPE (msg->cindex, NSM_LINK_CTYPE_GMPLS_TYPE))
    ifp->gmpls_type = msg->gtype;
#endif /* HAVE_GMPLS */

  /* Copy the cindex to the interface to notify PMs.  */
  ifp->cindex = msg->cindex;

  /* Unset the interface name cindex.  */
  NSM_UNSET_CTYPE (ifp->cindex, NSM_LINK_CTYPE_NAME);

#ifdef HAVE_HA
  lib_cal_modify_interface (zg, ifp);
#endif /* HAVE_HA */

  return ifp->cindex;
}

#ifdef HAVE_TE
s_int32_t
nsm_util_link_priority_bw_set (struct nsm_msg_link *msg, struct interface *ifp)
{
  int i;

  for (i = 0; i < MAX_PRIORITIES; i++)
    ifp->tecl_priority_bw[i] = msg->tecl_priority_bw[i];

  return 0;
}
#endif /* HAVE_TE */

s_int32_t
nsm_util_link_add (struct nsm_msg_header *header, void *arg, void *message)
{
  struct nsm_client_handler *nch = arg;
  struct nsm_client *nc = nch->nc;
  struct nsm_msg_link *msg = message;
  struct interface *ifp;

  if (nc->debug)
    nsm_interface_dump (nc->zg, msg);

  /* For protocol backward compability check interface name first.  */
  if (NSM_CHECK_CTYPE (msg->cindex, NSM_LINK_CTYPE_NAME))
    ifp = ifg_lookup_by_name (&nc->zg->ifg, msg->ifname);
  else
    ifp = ifg_lookup_by_index (&nc->zg->ifg, msg->ifindex);

  /* If this is really new one, create the structure. */
  if (ifp == NULL)
    ifp = ifg_get_by_name (&nc->zg->ifg, msg->ifname);

#ifdef HAVE_HA
  /* Unset the stale flag - ifp is ready for use*/
  UNSET_FLAG (ifp->status, HA_IF_STALE_FLAG);
#endif /* HAVE_HA */

  /* Copy NSM message value to interface.  */
  nsm_util_link_val_set (nc->zg, msg, ifp);

  /* Add to tables only if ifindex is valid */
  if (msg->ifindex > 0)
    {
      ifp->ifindex = msg->ifindex;
      ifg_table_add (&nc->zg->ifg, msg->ifindex, ifp);
    }

  return 0;
}

s_int32_t
nsm_util_link_delete (struct nsm_msg_header *header,
                      void *arg, void *message)
{
  struct nsm_client_handler *nch = arg;
  struct nsm_client *nc = nch->nc;
  struct nsm_msg_link *msg = message;
  struct interface *ifp;

  if (nc->debug)
    nsm_interface_dump (nc->zg, msg);

  /* For protocol backward compability check interface name first.  */
  if (NSM_CHECK_CTYPE (msg->cindex, NSM_LINK_CTYPE_NAME))
    ifp = ifg_lookup_by_name (&nc->zg->ifg, msg->ifname);
  else
    ifp = ifg_lookup_by_index (&nc->zg->ifg, msg->ifindex);

  if (ifp != NULL)
    {
      if_delete (&nc->zg->ifg, ifp);

      /* Notify nsm that deletion processing complete. */
      nsm_client_send_nsm_if_del_done(nc, msg, 0);
    }

  return 0;
}

s_int32_t
nsm_util_link_up (struct nsm_msg_header *header,
                  void *arg, void *message)
{
  struct nsm_client_handler *nch = arg;
  struct nsm_client *nc = nch->nc;
  struct nsm_msg_link *msg = message;
  struct interface *ifp;
  int old_up;

  if (nc->debug)
    nsm_interface_dump (nc->zg, msg);

  ifp = ifg_lookup_by_index (&nc->zg->ifg, msg->ifindex);
  if (ifp != NULL)
    {
      /* Check the previous state.  */
      old_up = if_is_up (ifp);

      /* Update the interface values.  */
      nsm_util_link_val_set (nc->zg, msg, ifp);

      /* Call the PM up callback function.  */
      if (ifp->vr != NULL)
        {
          if (nc->zg->ifg.if_callback[IF_CALLBACK_UP])
            (*nc->zg->ifg.if_callback[IF_CALLBACK_UP]) (ifp);

          if (old_up)
            if (nc->zg->ifg.if_callback[IF_CALLBACK_UPDATE])
              (*nc->zg->ifg.if_callback[IF_CALLBACK_UPDATE]) (ifp);
        }
    }

  return 0;
}

s_int32_t
nsm_util_link_down (struct nsm_msg_header *header,
                    void *arg, void *message)
{
  struct nsm_client_handler *nch = arg;
  struct nsm_client *nc = nch->nc;
  struct nsm_msg_link *msg = message;
  struct interface *ifp;

  if (nc->debug)
    nsm_interface_dump (nc->zg, msg);

  ifp = ifg_lookup_by_index (&nc->zg->ifg, msg->ifindex);
  if (ifp != NULL)
    {
      /* Update the interface values.  */
      nsm_util_link_val_set (nc->zg, msg, ifp);

      /* Call the PM down callback function.  */
      if (ifp->vr != NULL)
        if (nc->zg->ifg.if_callback[IF_CALLBACK_DOWN])
          (*nc->zg->ifg.if_callback[IF_CALLBACK_DOWN]) (ifp);
    }

  return 0;
}

s_int32_t
nsm_util_link_update (struct nsm_msg_header *header,
                      void *arg, void *message)
{
  struct nsm_client_handler *nch = arg;
  struct nsm_client *nc = nch->nc;
  struct nsm_msg_link *msg = message;
  struct interface *ifp;
  cindex_t cindex;
  int ret;

  if (nc->debug)
    nsm_interface_dump (nc->zg, msg);

  ifp = ifg_lookup_by_name (&nc->zg->ifg, msg->ifname);
  if (ifp != NULL)
    {
      /* Update the interface index.  */
      ret = if_ifindex_update (&nc->zg->ifg, ifp, msg->ifindex);

      /* Update the interface values.  */
      cindex = nsm_util_link_val_set (nc->zg, msg, ifp);

      /* Call the PM update callback function.  */
      if (ret || cindex)
        if (ifp->vr != NULL)
          if (nc->zg->ifg.if_callback[IF_CALLBACK_UPDATE])
            (*nc->zg->ifg.if_callback[IF_CALLBACK_UPDATE]) (ifp);
    }

  return 0;
}

#ifdef HAVE_TE
s_int32_t
nsm_util_link_priority_bw_update (struct nsm_msg_header *header,
                                  void *arg, void *message)
{
  struct nsm_client_handler *nch = arg;
  struct nsm_client *nc = nch->nc;
  struct nsm_msg_link *msg = message;
  struct interface *ifp;

  if (nc->debug)
    nsm_interface_dump (nc->zg, msg);

  ifp = ifg_lookup_by_index (&nc->zg->ifg, msg->ifindex);
  if (ifp != NULL)
    {
      /* Update the priority values.  */
      nsm_util_link_priority_bw_set (msg, ifp);

      /* Call the PM update callback function.  */
      if (ifp->vr != NULL)
        if (nc->zg->ifg.if_callback[IF_CALLBACK_PRIORITY_BW])
          (*nc->zg->ifg.if_callback[IF_CALLBACK_PRIORITY_BW]) (ifp);
    }
#ifdef HAVE_HA
      lib_cal_modify_interface (nc->zg, ifp);
#endif /* HAVE_HA */

  return 0;
}
#endif /* HAVE_TE */
#if 0 /*TBD old code*/
#ifdef HAVE_GMPLS
s_int32_t
nsm_util_link_gmpls_update (struct nsm_msg_header *header,
                            void *arg, void *message)
{
  struct nsm_client_handler *nch = arg;
  struct nsm_client *nc = nch->nc;
  struct nsm_msg_gmpls_if *msg = message;
  struct interface *ifp;

  if (nc->debug)
    nsm_gmpls_if_dump (nc->zg, msg);

  ifp = ifg_lookup_by_index (&nc->zg->ifg, msg->ifindex);
  if (ifp != NULL)
    {
      if (nsm_util_interface_gmpls_update (msg, ifp))
        if (ifp->vr != NULL)
          if (nc->zg->ifg.if_callback[IF_CALLBACK_GMPLS])
            (*nc->zg->ifg.if_callback[IF_CALLBACK_GMPLS]) (ifp);

      if (GMPLS_IF_EMPTY (ifp))
        GMPLS_IF_FREE (ifp);
    }

  return 0;
}
#endif /* HAVE_GMPLS */
#endif

s_int32_t
nsm_util_vr_add (struct nsm_msg_header *header, void *arg, void *message)
{
  struct nsm_client_handler *nch = arg;
  struct nsm_client *nc = nch->nc;
  struct nsm_msg_vr *msg = message;
  struct apn_vr *vr;

  if (msg->name)
    msg->name [msg->len] = '\0';

  if (nc->debug)
    nsm_vr_dump (nc->zg, msg);

  vr = apn_vr_update_by_name (nc->zg, msg->name, msg->vr_id);
  if (vr != NULL)
    {
      if (nc->zg->vr_callback[VR_CALLBACK_ADD_UNCHG])
        (*nc->zg->vr_callback[VR_CALLBACK_ADD_UNCHG]) (vr);
      return 0;
    }

  vr = apn_vr_get_by_id (nc->zg, msg->vr_id);
  if (msg->len)
    vr->name = XSTRDUP (MTYPE_TMP, msg->name); /* XXX */

  /* Protocol callback. */
  if (nc->zg->vr_callback[VR_CALLBACK_ADD])
    (*nc->zg->vr_callback[VR_CALLBACK_ADD]) (vr);

  /* Read the configuration.  */
  HOST_CONFIG_VR_START (vr);

  return 0;
}

s_int32_t
nsm_util_vr_delete (struct nsm_msg_header *header,
                    void *arg, void *message)
{
  struct nsm_client_handler *nch = arg;
  struct nsm_client *nc = nch->nc;
  struct nsm_msg_vr *msg = message;
  struct apn_vr *vr;

  if (nc->debug)
    nsm_vr_dump (nc->zg, msg);

  vr = apn_vr_lookup_by_id (nc->zg, msg->vr_id);
  if (vr == NULL)
    return 0;

  /* Connection close callback.  */
  if (vr->zg->vr_callback[VR_CALLBACK_CLOSE])
    (*vr->zg->vr_callback[VR_CALLBACK_CLOSE]) (vr);

  /* Protocol callback. */
  if (nc->zg->vr_callback[VR_CALLBACK_DELETE])
    (*nc->zg->vr_callback[VR_CALLBACK_DELETE]) (vr);

  /* Delete of VR-config file should NOT be done by
   * PMs. Only NSM should control it.
   */
#if 0
  /* Set VR FLAG, when Shutdown is not in Progress */
  if (!CHECK_FLAG (nc->zg->flags, LIB_FLAG_SHUTDOWN_IN_PROGRESS))
    SET_FLAG (vr->flags, LIB_FLAG_DELETE_VR_CONFIG_FILE);
#endif

  apn_vr_delete (vr);

  return 0;
}

s_int32_t
nsm_util_vr_bind (struct nsm_msg_header *header, void *arg, void *message)
{
  struct nsm_client_handler *nch = arg;
  struct nsm_client *nc = nch->nc;
  struct nsm_msg_vr_bind *msg = message;
  struct apn_vr *vr;

  if (nc->debug)
    nsm_vr_bind_dump (nc->zg, msg);

  vr = apn_vr_lookup_by_id (nc->zg, msg->vr_id);
  if (vr != NULL)
    if_vr_bind (&vr->ifm, msg->ifindex);

  return 0;
}

s_int32_t
nsm_util_vr_unbind (struct nsm_msg_header *header, void *arg, void *message)
{
  struct nsm_client_handler *nch = arg;
  struct nsm_client *nc = nch->nc;
  struct nsm_msg_vr_bind *msg = message;
  struct apn_vr *vr;

  if (nc->debug)
    nsm_vr_bind_dump (nc->zg, msg);

  vr = apn_vr_lookup_by_id (nc->zg, msg->vr_id);
  if (vr != NULL)
    {
      /* Make sure nobody is on this unbound interface mode.  */
      if (vr->name != NULL)
        if (vr->zg->vr_callback[VR_CALLBACK_UNBIND])
          (*vr->zg->vr_callback[VR_CALLBACK_UNBIND]) (vr);

      if_vr_unbind (&vr->ifm, msg->ifindex);
    }

  return 0;
}

s_int32_t
nsm_util_vrf_bind (struct nsm_msg_header *header, void *arg, void *message)
{
  struct nsm_client_handler *nch = arg;
  struct nsm_client *nc = nch->nc;
  struct nsm_msg_vrf_bind *msg = message;
  struct apn_vr *vr;
  struct apn_vrf *vrf;

  if (nc->debug)
    nsm_vrf_bind_dump (nc->zg, msg);

  vr = apn_vr_lookup_by_id (nc->zg, header->vr_id);
  if (vr == NULL)
    return 0;

  vrf = apn_vrf_lookup_by_id (vr, msg->vrf_id);
  if (vrf != NULL)
    if_vrf_bind (&vrf->ifv, msg->ifindex);

  return 0;
}

s_int32_t
nsm_util_vrf_unbind (struct nsm_msg_header *header,
                     void *arg, void *message)
{
  struct nsm_client_handler *nch = arg;
  struct nsm_client *nc = nch->nc;
  struct nsm_msg_vrf_bind *msg = message;
  struct apn_vr *vr;
  struct apn_vrf *vrf;

  if (nc->debug)
    nsm_vrf_bind_dump (nc->zg, msg);

  vr = apn_vr_lookup_by_id (nc->zg, header->vr_id);
  if (vr == NULL)
    return 0;

  vrf = apn_vrf_lookup_by_id (vr, msg->vrf_id);
  if (vrf != NULL)
    if_vrf_unbind (&vrf->ifv, msg->ifindex);

  return 0;
}

s_int32_t
nsm_util_vrf_add (struct nsm_msg_header *header, void *arg, void *message)
{
  struct nsm_client_handler *nch = arg;
  struct nsm_client *nc = nch->nc;
  struct nsm_msg_vrf *msg = message;
  struct apn_vr *vr;
  struct apn_vrf *vrf, *vrf_old;
  vrf_id_t old_id;

  if (msg->name)
    msg->name[msg->len] = '\0';

  if (nc->debug)
    nsm_vrf_dump (nc->zg, msg);

  vr = apn_vr_lookup_by_id (nc->zg, header->vr_id);
  if (vr == NULL)
    return 0;

  /* Get VRF.  */
  vrf = apn_vrf_get_by_name (vr, msg->name);

  /* Check if VRF already exists.  */
  vrf_old = apn_vrf_lookup_by_id (vr, msg->vrf_id);
  if (vrf_old == vrf)
    return 0;
  else if (vrf_old != NULL)
    {
      /* Delete VRF from vector if ID was changed.  */
      vector_slot (vr->vrf_vec, vrf->id) = NULL;
      vector_slot (vr->zg->fib2vrf, vrf->fib_id) = NULL;
    }

  /* Set VRF ID.  */
  old_id = vrf->id;
  vrf->id = vector_set_index (vr->vrf_vec, msg->vrf_id, vrf);

  /* Set FIB ID.  */
  vrf->fib_id = vector_set_index (nc->zg->fib2vrf, msg->fib_id, vrf);

#ifdef HAVE_HA
  lib_cal_modify_vrf (nc->zg, vrf);
#endif /* HAVE_HA */

  /* Protocol callback. */
  if (vrf->proto == NULL)
    {
      if (nc->zg->vrf_callback[VRF_CALLBACK_ADD])
        (*nc->zg->vrf_callback[VRF_CALLBACK_ADD]) (vrf);
    }
  else if (vrf->id != old_id)
    {
      if (nc->zg->vrf_callback[VRF_CALLBACK_UPDATE])
        (*nc->zg->vrf_callback[VRF_CALLBACK_UPDATE]) (vrf);
    }

  return 0;
}

s_int32_t
nsm_util_vrf_delete (struct nsm_msg_header *header,
                     void *arg, void *message)
{
  struct nsm_client_handler *nch = arg;
  struct nsm_client *nc = nch->nc;
  struct nsm_msg_vrf *msg = message;
  struct apn_vr *vr;
  struct apn_vrf *vrf;

  if (msg->name)
    msg->name[msg->len] = '\0';

  if (nc->debug)
    nsm_vrf_dump (nc->zg, msg);

  vr = apn_vr_lookup_by_id (nc->zg, header->vr_id);
  if (vr == NULL)
    return 0;

  vrf = apn_vrf_lookup_by_name (vr, msg->name);
  if (vrf == NULL)
    return 0;

  /* Protocol callback. */
  if (vrf->proto != NULL)
    if (nc->zg->vrf_callback[VRF_CALLBACK_DELETE])
      (*nc->zg->vrf_callback[VRF_CALLBACK_DELETE]) (vrf);

  if (! IS_APN_VRF_DEFAULT (vrf))
    apn_vrf_delete (vrf);

  return 0;
}

#ifdef HAVE_L3
s_int32_t
nsm_util_addr_add (struct nsm_msg_header *header, void *arg, void *message)
{
  struct nsm_client_handler *nch = arg;
  struct nsm_client *nc = nch->nc;
  struct nsm_msg_address *msg = message;
  struct interface *ifp;
  struct connected *ifc = NULL;
  struct prefix *d;
  struct prefix p;

  if (nc->debug)
    nsm_address_dump (nc->zg, msg);

  /* Lookup index. */
  ifp = ifg_lookup_by_index (&nc->zg->ifg, msg->ifindex);
  if (! ifp)
    return 0;

  if (msg->afi == AFI_IP)
    {
      p.family = AF_INET;
      p.u.prefix4 = msg->u.ipv4.src;
      p.prefixlen = msg->prefixlen;

      /* Check duplicate. */
      ifc = if_lookup_ifc_prefix (ifp, &p);
      if (ifc == NULL)
        {
          ifc = ifc_get_ipv4 (&msg->u.ipv4.src, msg->prefixlen, ifp);
          ifc->flags = msg->flags;

          ifc->destination = d = (struct prefix *)prefix_ipv4_new ();
          d->family = AF_INET;
          d->prefixlen = msg->prefixlen;
          d->u.prefix4 = msg->u.ipv4.dst;

          /* Add connected address to the interface. */
          if_add_ifc_ipv4 (ifp, ifc);
        }
    }
#ifdef HAVE_IPV6
  else if (msg->afi == AFI_IP6)
    {
      p.family = AF_INET6;
      p.u.prefix6 = msg->u.ipv6.src;
      p.prefixlen = msg->prefixlen;

      /* Check duplicate. */
      ifc = if_lookup_ifc_prefix (ifp, &p);
      if (ifc == NULL)
        {
          ifc = ifc_get_ipv6 (&msg->u.ipv6.src, msg->prefixlen, ifp);
          ifc->flags = msg->flags;

          ifc->destination = d = (struct prefix *)prefix_ipv6_new ();
          d->family = AF_INET6;
          d->prefixlen = msg->prefixlen;
          d->u.prefix6 = msg->u.ipv6.dst;

          /* Add connected address to the interface. */
          if_add_ifc_ipv6 (ifp, ifc);
        }
    }
#endif /* HAVE_IPV6 */

  /* Call callback. */
  if (ifc != NULL)
    if (ifc->ifp->vr != NULL)
      if (nc->zg->ifg.ifc_callback[IFC_CALLBACK_ADDR_ADD])
        (*nc->zg->ifg.ifc_callback[IFC_CALLBACK_ADDR_ADD]) (ifc);

  return 0;
}

s_int32_t
nsm_util_addr_delete (struct nsm_msg_header *header,
                      void *arg, void *message)
{
  struct nsm_client_handler *nch = arg;
  struct nsm_client *nc = nch->nc;
  struct nsm_msg_address *msg = message;
  struct interface *ifp;
  struct connected *ifc = NULL;
  struct prefix p;

  if (nc->debug)
    nsm_address_dump (nc->zg, msg);

  /* Lookup index. */
  ifp = ifg_lookup_by_index (&nc->zg->ifg, msg->ifindex);
  if (! ifp)
    return 0;

  p.prefixlen = msg->prefixlen;

  if (msg->afi == AFI_IP)
    {
      p.family = AF_INET;
      p.u.prefix4 = msg->u.ipv4.src;

      ifc = if_lookup_ifc_prefix (ifp, &p);
      if (ifc)
        if_delete_ifc_ipv4 (ifp, ifc);
    }
#ifdef HAVE_IPV6
  else if (msg->afi == AFI_IP6)
    {
      p.family = AF_INET6;
      p.u.prefix6 = msg->u.ipv6.src;

      ifc = if_lookup_ifc_prefix (ifp, &p);
      if (ifc)
        if_delete_ifc_ipv6 (ifp, ifc);
    }
#endif /* HAVE_IPV6 */

  if (ifc != NULL)
    {
      /* Call callback.  */
      if (ifc->ifp->vr != NULL)
        {
          /* Finish the VTY session.  */
          if (nc->zg->ifg.ifc_callback[IFC_CALLBACK_SESSION_CLOSE])
            (*nc->zg->ifg.ifc_callback[IFC_CALLBACK_SESSION_CLOSE]) (ifc);

          /* And remove the IP address.  */
          if (nc->zg->ifg.ifc_callback[IFC_CALLBACK_ADDR_DELETE])
            (*nc->zg->ifg.ifc_callback[IFC_CALLBACK_ADDR_DELETE]) (ifc);
        }

      ifc_free (nc->zg, ifc);
    }

  return 0;
}
#endif /* HAVE_L3 */

s_int32_t
nsm_util_server_status_notify (struct nsm_msg_header *header, void *arg, 
                               void *message)
{
  struct nsm_client_handler *nch = arg;
  struct nsm_client *nc = nch->nc;
  struct nsm_msg_server_status *msg = message;
  
  /* Nsm Client is already disconnected. */
  if (nc == NULL)
    return 0;

  /* Set the Flag to notify that NSM (hence nsm_client)
   * went down.
   */
  SET_FLAG (nc->nsm_server_flags, msg->nsm_status);
  
  if (msg->nsm_status == NSM_MSG_NSM_SERVER_SHUTDOWN) 
    zlog_info (nc->zg, "NSM Server Status Shutdown received");  

  return 0;
}

s_int32_t
nsm_util_router_id (struct nsm_msg_header *header, void *arg, void *message)
{
  struct apn_vr *vr;
  struct apn_vrf *iv;
  struct nsm_client_handler *nch = arg;
  struct nsm_client *nc = nch->nc;
  struct nsm_msg_router_id *msg = message;

  if (nc->debug)
    nsm_router_id_dump (nc->zg, msg);

  vr = apn_vr_lookup_by_id (nc->zg, header->vr_id);
  if (vr == NULL)
    return 0;

  iv = apn_vrf_lookup_by_id (vr, header->vrf_id);
  if (iv == NULL)
    return 0;

  /* Update Router ID.  */
  if (iv->router_id.s_addr != msg->router_id)
    {
      iv->router_id.s_addr = msg->router_id;
      vr->router_id = iv->router_id;

      if (nc->zg->vrf_callback[VRF_CALLBACK_ROUTER_ID])
        (*nc->zg->vrf_callback[VRF_CALLBACK_ROUTER_ID]) (iv);

#ifdef HAVE_HA
      lib_cal_modify_vr (nc->zg, vr);
#endif /* HAVE_HA */
    }

  return 0;
}

#ifndef HAVE_IMI
s_int32_t
nsm_util_user_update (struct nsm_msg_header *header, void *arg, void *message)
{
  struct nsm_client_handler *nch = arg;
  struct nsm_client *nc = nch->nc;
  struct nsm_msg_user *msg = message;
  struct apn_vr *vr;
  struct host_user *user = NULL;
  char *password = NULL;
  char *password_encrypt = NULL;

  if (msg->username)
    msg->username [msg->username_len] = '\0';

  if (nc->debug)
    nsm_user_dump (nc->zg, msg, "Update");

  vr = apn_vr_lookup_by_id (nc->zg, header->vr_id);
  if (vr == NULL)
    return 0;

  if (msg->username)
    user = host_user_get (vr->host, msg->username);
  if (!user)
    return 0;

  SET_FLAG (user->flags, HOST_USER_FLAG_PRIVILEGED);

  if (NSM_CHECK_CTYPE (msg->cindex, NSM_USER_CTYPE_PASSWORD))
    password = msg->password;
  if (NSM_CHECK_CTYPE (msg->cindex, NSM_USER_CTYPE_PASSWORD_ENCRYPT))
    password_encrypt = msg->password_encrypt;

  host_user_update (vr->host, msg->username,
                    msg->privilege, password, password_encrypt);

  return 0;
}

s_int32_t
nsm_util_user_delete (struct nsm_msg_header *header, void *arg, void *message)
{
  struct nsm_client_handler *nch = arg;
  struct nsm_client *nc = nch->nc;
  struct nsm_msg_user *msg = message;
  struct apn_vr *vr;
  struct host_user *user = NULL;

  if (msg->username)
    msg->username [msg->username_len] = '\0';

  if (nc->debug)
    nsm_user_dump (nc->zg, msg, "Delete");

  vr = apn_vr_lookup_by_id (nc->zg, header->vr_id);
  if (vr == NULL)
    return 0;

  if (msg->username)
    user = host_user_lookup (vr->host, msg->username);
  if (user != NULL)
    host_user_delete (vr->host, msg->username);

  return 0;
}
#endif /* HAVE_IMI */

struct interface *
nsm_util_interface_state (struct nsm_msg_link *msg, struct if_master *ifg)
{
  struct interface *ifp = NULL;

  ifp = ifg_lookup_by_index (ifg, msg->ifindex);
  if (ifp == NULL)
    return NULL;

  /* Copy NSM message value to interface.  */
  nsm_util_link_val_set (ifg->zg, msg, ifp);

  return ifp;
}

#ifdef HAVE_TE
struct interface *
nsm_util_interface_priority_bw_update (struct nsm_msg_link *msg,
                                       struct if_master *ifg)
{
  struct interface *ifp;

  ifp = ifg_lookup_by_index (ifg, msg->ifindex);
  if (! ifp)
    return NULL;

  nsm_util_link_priority_bw_set (msg, ifp);

  return ifp;
}

/* Admin group update from zebra daemon. */
void
nsm_util_admin_group_update (struct admin_group array[],
                             struct nsm_msg_admin_group *msg)
{
  int i;
  int val;

  for (i = 0; i < ADMIN_GROUP_MAX; i++)
    {
      /* Reset val */
      val = -1;

      /* If val is '-1', check the next one */
      if (msg->array[i].val == -1)
        continue;

      /* Copy over the name, and the value */
      array [i].val = msg->array[i].val;
      pal_mem_cpy (array[i].name, msg->array[i].name, ADMIN_GROUP_NAMSIZ);
    }
}
#endif /* HAVE_TE */
#if 0 /*TBD old code*/
#ifdef HAVE_GMPLS
/* GMPLS interface update utility. */
s_int32_t
nsm_util_interface_gmpls_update (struct nsm_msg_gmpls_if *msg,
                                 struct interface *ifp)
{
  int change = 0;
  struct gmpls_if *gmif;

  if ((gmif = ifp->gmpls_if) == NULL)
    gmif = ifp->gmpls_if = gmpls_if_new ();

  if (gmif->link_ids.local != msg->link_ids.local
      || gmif->link_ids.remote != msg->link_ids.remote)
    {
      gmif->link_ids.local = msg->link_ids.local;
      gmif->link_ids.remote = msg->link_ids.remote;
      change++;
    }
  if (gmif->protection != msg->protection)
    {
      gmif->protection = msg->protection;
      change++;
    }
  if (gmpls_srlg_cmp (gmif->srlg, msg->srlg))
    {
      gmif->srlg = gmpls_srlg_copy (gmif->srlg, msg->srlg);
      change++;
    }
  if (gmif->capability != msg->capability)
    {
      gmif->capability = msg->capability;
      change++;
    }
  if (gmif->encoding != msg->encoding)
    {
      gmif->encoding = msg->encoding;
      change++;
    }
  if (gmif->min_lsp_bw != msg->min_lsp_bw)
    {
      gmif->min_lsp_bw = msg->min_lsp_bw;
      change++;
    }
  if (gmif->indication != msg->indication)
    {
      gmif->indication = msg->indication;
      change++;
    }
  return change;
}
#endif /* HAVE_GMPLS */
#endif

#ifdef HAVE_DIFFSERV
/* Update the support dscp configurationi for the Diffserv client. */
u_char
nsm_util_supported_dscp_update (struct nsm_msg_supported_dscp_update *msg,
                                u_char *supported_dscp)
{

  /* Update the value in client. */
  supported_dscp[(int)(msg->dscp)] = msg->support;

  return msg->dscp;
}


/* Update the DSCP value to EXP-bit mapping for the Diffserv client. */
void
nsm_util_dscp_exp_map_update (struct nsm_msg_dscp_exp_map_update *msg,
                              u_char *dscp_exp_map)
{

  /* If EXP bit value is greater than DIFFSERV_MAX_DSCP_EXP_MAPPINGS,
     there's a problem. */
  if (msg->exp > DIFFSERV_MAX_DSCP_EXP_MAPPINGS)
    return;

  /* Update the value in client. */
  dscp_exp_map[(int)(msg->exp)] = msg->dscp;
}
#endif /* HAVE_DIFFSREV */

/* XXX-VR */
s_int32_t
nsm_util_send_redistribute_set (struct nsm_client *nc, int type,
                                vrf_id_t vrf_id)
{
  struct nsm_msg_redistribute msg;
  int ret;
  struct apn_vr *vr = apn_vr_get_privileged (nc->zg);

  pal_mem_set (&msg, 0, sizeof (struct nsm_msg_redistribute));

  msg.type = type;
  msg.cindex = 0;

  ret = nsm_client_send_redistribute_set (vr->id, vrf_id, nc, &msg);

  return ret;
}

s_int32_t
nsm_util_send_redistribute_unset (struct nsm_client *nc, int type,
                                  vrf_id_t vrf_id)
{
  struct nsm_msg_redistribute msg;
  int ret;
  struct apn_vr *vr = apn_vr_get_privileged (nc->zg);

  pal_mem_set (&msg, 0, sizeof (struct nsm_msg_redistribute));

  msg.type = type;
  msg.cindex = 0;

  ret = nsm_client_send_redistribute_unset (vr->id, vrf_id, nc, &msg);

  return ret;
}

s_int32_t
nsm_client_send_route_manage (struct nsm_client *nc, int msg_type,
                              struct nsm_msg_route_manage *msg)
{
  u_char *pnt;
  u_int16_t size;
  int nbytes;
  struct nsm_client_handler *nch = nc->async;
  u_int32_t msg_id;
  u_int32_t vr_id = 0;
  vrf_id_t vrf_id = 0;

  /* We use async connection for IPv4 route updates.  */
  if (! nch || ! nch->up)
    return -1;

  /* Set pnt and size.  */
  pnt = nch->buf + NSM_MSG_HEADER_SIZE;
  size = nch->len - NSM_MSG_HEADER_SIZE;

  /* Encode service.  */
  nbytes = nsm_encode_route_manage (&pnt, &size, msg);
  if (nbytes < 0)
    return nbytes;

  return nsm_client_send_message (nch, vr_id, vrf_id,
                                  msg_type, nbytes, &msg_id);
}


s_int32_t
nsm_client_send_route_manage_vr (struct nsm_client *nc, int msg_type,
                                 struct nsm_msg_route_manage *msg,
                                 u_int32_t vr_id)
{
  u_char *pnt;
  u_int16_t size;
  int nbytes;
  struct nsm_client_handler *nch = nc->async;
  u_int32_t msg_id;
  vrf_id_t vrf_id = 0;

  /* We use async connection for IPv4 route updates.  */
  if (! nch || ! nch->up)
    return -1;

  /* Set pnt and size.  */
  pnt = nch->buf + NSM_MSG_HEADER_SIZE;
  size = nch->len - NSM_MSG_HEADER_SIZE;

  /* Encode service.  */
  nbytes = nsm_encode_route_manage (&pnt, &size, msg);
  if (nbytes < 0)
    return nbytes;

  return nsm_client_send_message (nch, vr_id, vrf_id,
                                  msg_type, nbytes, &msg_id);
}

s_int32_t
nsm_client_send_preserve (struct nsm_client *nc, int restart_time)
{
  struct nsm_msg_route_manage msg;

  pal_mem_set (&msg, 0, sizeof (struct nsm_msg_route_manage));
  NSM_SET_CTYPE (msg.cindex, NSM_ROUTE_MANAGE_CTYPE_RESTART_TIME);
  msg.restart_time = restart_time;

  return nsm_client_send_route_manage (nc, NSM_MSG_ROUTE_PRESERVE, &msg);
}

s_int32_t
nsm_client_send_preserve_with_val (struct nsm_client *nc, int restart_time,
                                   u_char *restart_val,
                                   u_int16_t restart_length)
{
  struct nsm_msg_route_manage msg;

  pal_mem_set (&msg, 0, sizeof (struct nsm_msg_route_manage));
  NSM_SET_CTYPE (msg.cindex, NSM_ROUTE_MANAGE_CTYPE_RESTART_TIME);
  msg.restart_time = restart_time;

  if (restart_val)
    {
      NSM_SET_CTYPE (msg.cindex, NSM_ROUTE_MANAGE_CTYPE_RESTART_OPTION);
      msg.restart_val = restart_val;
      msg.restart_length = restart_length;
    }

  return nsm_client_send_route_manage (nc, NSM_MSG_ROUTE_PRESERVE, &msg);
}

s_int32_t
nsm_client_send_stale_remove (struct nsm_client *nc, afi_t afi, safi_t safi)
{
  struct nsm_msg_route_manage msg;

  pal_mem_set (&msg, 0, sizeof (struct nsm_msg_route_manage));
  NSM_SET_CTYPE (msg.cindex, NSM_ROUTE_MANAGE_CTYPE_AFI_SAFI);
  msg.afi = afi;
  msg.safi = safi;

  return nsm_client_send_route_manage (nc, NSM_MSG_ROUTE_STALE_REMOVE, &msg);
}

#if defined HAVE_RESTART && defined HAVE_MPLS
int
nsm_client_send_force_stale_remove (struct nsm_client *nc, afi_t afi,
                                    safi_t safi, u_int32_t vrf_id,
                                    bool_t force_del)
{

  u_char *pnt;
  u_int16_t size;
  int nbytes;
  u_int32_t msg_id;
  u_int32_t vr_id = 0;
  struct nsm_client_handler *nch = nc->async;
  struct nsm_msg_route_manage msg;

  pal_mem_set (&msg, 0, sizeof (struct nsm_msg_route_manage));

  /* We use async connection for IPv4 route updates.  */
  if (! nch || ! nch->up)
    return -1;

  NSM_SET_CTYPE (msg.cindex, NSM_ROUTE_MANAGE_CTYPE_AFI_SAFI);
  msg.afi = afi;
  msg.safi = safi;

  if (force_del)
    {
      NSM_SET_CTYPE (msg.cindex, NSM_ROUTE_MANAGE_CTYPE_MPLS_FLAGS);
      SET_FLAG (msg.flags, NSM_MSG_ILM_FORCE_STALE_DEL);
    }

  /* Set pnt and size.  */
  pnt = nch->buf + NSM_MSG_HEADER_SIZE;
  size = nch->len - NSM_MSG_HEADER_SIZE;

  /* Encode service.  */
  nbytes = nsm_encode_route_manage (&pnt, &size, &msg);
  if (nbytes < 0)
    return nbytes;

  return nsm_client_send_message (nch, vr_id, vrf_id,
                                  NSM_MSG_ROUTE_STALE_REMOVE,
                                  nbytes, &msg_id);
}
#endif /* HAVE_RESTART  && HAVE_MPLS*/

s_int32_t
nsm_client_send_stale_mark (struct nsm_client *nc, afi_t afi,
                            safi_t safi, u_int32_t vr_id)
{
  struct nsm_msg_route_manage msg;

  pal_mem_set (&msg, 0, sizeof (struct nsm_msg_route_manage));
  NSM_SET_CTYPE (msg.cindex, NSM_ROUTE_MANAGE_CTYPE_AFI_SAFI);
  msg.afi = afi;
  msg.safi = safi;

  return nsm_client_send_route_manage_vr (nc, NSM_MSG_ROUTE_STALE_MARK, &msg, vr_id);
}


#ifdef HAVE_MPLS
s_int32_t
nsm_client_send_mpls_notification (struct nsm_client *nc,
                                   struct mpls_notification *msg,
                                   u_int32_t *mpls_msg_id)
{
  struct nsm_client_handler *nch = nc->async;
  u_int32_t msg_id;
  u_int16_t size;
  u_char *pnt;
  int nbytes;
  u_int32_t vr_id = 0;
  vrf_id_t vrf_id = 0;

  /* We use async connection. */
  if (! nch || ! nch->up)
    return -1;

  /* Set pnt and size.  */
  pnt = nch->buf + NSM_MSG_HEADER_SIZE;
  size = nch->len - NSM_MSG_HEADER_SIZE;

  if (++nch->mpls_msg_id == 0)
    ++nch->mpls_msg_id;
  msg->id = nch->mpls_msg_id;

  /* Encode service.  */
  nbytes = nsm_encode_mpls_notification (&pnt, &size, msg);
  if (nbytes < 0)
    return nbytes;

  /* Set MPLS notification message id. */
  if (mpls_msg_id)
    *mpls_msg_id = nch->mpls_msg_id;

  return nsm_client_send_message (nch, vr_id, vrf_id,
                                  NSM_MSG_MPLS_NOTIFICATION, nbytes, &msg_id);
}

#ifdef HAVE_IPV6
s_int32_t
nsm_client_send_ftn_add_ipv6 (struct nsm_client *nc, struct ftn_add_data *msg,
                              u_int32_t *mpls_msg_id)
{
  struct nsm_client_handler *nch = nc->async;
  u_int32_t msg_id;
  u_int16_t size;
  u_char *pnt;
  int nbytes;
  u_int32_t vr_id = 0;
  vrf_id_t vrf_id = 0;

  /* We use async connection. */
  if (! nch || ! nch->up)
    return -1;

  /* Set pnt and size.  */
  pnt = nch->buf + NSM_MSG_HEADER_SIZE;
  size = nch->len - NSM_MSG_HEADER_SIZE;

  /* This is add message.  */
  msg->flags = 0;
  SET_FLAG (msg->flags, NSM_MSG_FTN_ADD);

  if (++nch->mpls_msg_id == 0)
    ++nch->mpls_msg_id;
  msg->id = nch->mpls_msg_id;

  /* Encode service.  */
  nbytes = nsm_encode_ftn_data_ipv6 (&pnt, &size, msg);
  if (nbytes < 0)
    return nbytes;

  /* Set FTN message id.  */
  if (mpls_msg_id)
    *mpls_msg_id = nch->mpls_msg_id;

  return nsm_client_send_message (nch, vr_id, vrf_id,
                                  NSM_MSG_MPLS_FTN_IPV6, nbytes, &msg_id);
}

s_int32_t
nsm_client_send_ftn_del_slow_ipv6 (struct nsm_client *nc,
                                   struct ftn_add_data *msg)
{
  struct nsm_client_handler *nch = nc->async;
  u_int32_t msg_id;
  u_int16_t size;
  u_char *pnt;
  int nbytes;
  u_int32_t vr_id = 0;
  vrf_id_t vrf_id = 0;

  /* We use async connection. */
  if (! nch || ! nch->up)
    return -1;

  /* Set pnt and size.  */
  pnt = nch->buf + NSM_MSG_HEADER_SIZE;
  size = nch->len - NSM_MSG_HEADER_SIZE;

  /* This is delete message.  */
  msg->flags = 0;
  msg->id = 0;

  /* Encode service.  */
  nbytes = nsm_encode_ftn_data_ipv6 (&pnt, &size, msg);
  if (nbytes < 0)
    return nbytes;

  return nsm_client_send_message (nch, vr_id, vrf_id,
                                  NSM_MSG_MPLS_FTN_IPV6, nbytes, &msg_id);
}

s_int32_t
nsm_client_send_ftn_del_ipv6 (struct nsm_client *nc,
                              struct ftn_del_data *fdd)
{
  struct nsm_client_handler *nch = nc->async;
  struct ftn_add_data msg;
  u_int32_t msg_id;
  u_int16_t size;
  u_char *pnt;
  int nbytes;
  u_int32_t vr_id = 0;
  vrf_id_t vrf_id = 0;

  /* We use async connection. */
  if (! nch || ! nch->up)
    return -1;

  /* Set pnt and size.  */
  pnt = nch->buf + NSM_MSG_HEADER_SIZE;
  size = nch->len - NSM_MSG_HEADER_SIZE;

  /* This is fast delete message.  */
  pal_mem_set (&msg, 0, sizeof (struct ftn_add_data));
  SET_FLAG (msg.flags, NSM_MSG_FTN_FAST_DELETE);

  /* Assign data.  */
  msg.vrf_id = fdd->vrf_id;
  prefix_copy (&msg.fec_prefix, &fdd->fec);
  msg.ftn_ix = fdd->ftn_ix;

  /* Encode service.  */
  nbytes = nsm_encode_ftn_data_ipv6 (&pnt, &size, &msg);
  if (nbytes < 0)
    return nbytes;

  return nsm_client_send_message (nch, vr_id, vrf_id,
                                  NSM_MSG_MPLS_FTN_IPV6, nbytes, &msg_id);
}

s_int32_t
nsm_client_send_ilm_add_ipv6 (struct nsm_client *nc, struct ilm_add_data *msg,
                              u_int32_t *mpls_msg_id)
{
  struct nsm_client_handler *nch = nc->async;
  u_int32_t msg_id;
  u_int16_t size;
  u_char *pnt;
  int nbytes;
  u_int32_t vr_id = 0;
  vrf_id_t vrf_id = 0;

  /* We use async connection. */
  if (! nch || ! nch->up)
    return -1;

  /* Set pnt and size.  */
  pnt = nch->buf + NSM_MSG_HEADER_SIZE;
  size = nch->len - NSM_MSG_HEADER_SIZE;

  /* This is add message.  */
  SET_FLAG (msg->flags, NSM_MSG_ILM_ADD);

  if (++nch->mpls_msg_id == 0)
    ++nch->mpls_msg_id;
  msg->id = nch->mpls_msg_id;

  /* Encode service.  */
  nbytes = nsm_encode_ilm_data_ipv6 (&pnt, &size, msg);
  if (nbytes < 0)
    return nbytes;

  /* Set FTN message id.  */
  if (mpls_msg_id)
    *mpls_msg_id = nch->mpls_msg_id;

  return nsm_client_send_message (nch, vr_id, vrf_id,
                                  NSM_MSG_MPLS_ILM_IPV6, nbytes, &msg_id);
}

s_int32_t
nsm_client_send_ilm_fast_del_ipv6 (struct nsm_client *nc,
                                   struct ilm_del_data *msg)
{
  struct nsm_client_handler *nch = nc->async;
  struct ilm_add_data iad;
  u_int32_t msg_id;
  u_int16_t size;
  u_char *pnt;
  int nbytes;
  u_int32_t vr_id = 0;
  vrf_id_t vrf_id = 0;

  /* We use async connection. */
  if (! nch || ! nch->up)
    return -1;

  /* Set pnt and size.  */
  pnt = nch->buf + NSM_MSG_HEADER_SIZE;
  size = nch->len - NSM_MSG_HEADER_SIZE;

  /* Fill data. */
  pal_mem_set (&iad, 0, sizeof (struct ilm_add_data));
  iad.iif_ix = msg->iif_ix;
  iad.in_label = msg->in_label;
  iad.ilm_ix = msg->ilm_ix;
  iad.ilm_ix = msg->ilm_ix;

  SET_FLAG (iad.flags, NSM_MSG_ILM_FAST_DELETE);

  /* Encode service.  */
  nbytes = nsm_encode_ilm_data_ipv6 (&pnt, &size, &iad);
  if (nbytes < 0)
    return nbytes;

  return nsm_client_send_message (nch, vr_id, vrf_id,
                                  NSM_MSG_MPLS_ILM_IPV6, nbytes, &msg_id);
}

s_int32_t
nsm_client_send_ilm_del_ipv6 (struct nsm_client *nc,
                              struct ilm_add_data *msg)
{
  struct nsm_client_handler *nch = nc->async;
  u_int32_t msg_id;
  u_int16_t size;
  u_char *pnt;
  int nbytes;
  u_int32_t vr_id = 0;
  vrf_id_t vrf_id = 0;

  /* We use async connection. */
  if (! nch || ! nch->up)
    return -1;

  /* Set pnt and size.  */
  pnt = nch->buf + NSM_MSG_HEADER_SIZE;
  size = nch->len - NSM_MSG_HEADER_SIZE;

  /* This is a delete message.  */
  SET_FLAG (msg->flags, NSM_MSG_ILM_DELETE);
  msg->id = 0;

  /* Encode service.  */
  nbytes = nsm_encode_ilm_data_ipv6 (&pnt, &size, msg);
  if (nbytes < 0)
    return nbytes;

  return nsm_client_send_message (nch, vr_id, vrf_id,
                                  NSM_MSG_MPLS_ILM_IPV6, nbytes, &msg_id);
}
#endif

s_int32_t
nsm_client_send_ftn_add_ipv4 (struct nsm_client *nc, struct ftn_add_data *msg,
                              u_int32_t *mpls_msg_id)
{
  struct nsm_client_handler *nch = nc->async;
  u_int32_t msg_id;
  u_int16_t size;
  u_char *pnt;
  int nbytes;
  u_int32_t vr_id = 0;
  vrf_id_t vrf_id = 0;

  /* We use async connection. */
  if (! nch || ! nch->up)
    return -1;

  /* Set pnt and size.  */
  pnt = nch->buf + NSM_MSG_HEADER_SIZE;
  size = nch->len - NSM_MSG_HEADER_SIZE;

  /* This is add message.  */
  msg->flags = 0;
  SET_FLAG (msg->flags, NSM_MSG_FTN_ADD);

  if (++nch->mpls_msg_id == 0)
    ++nch->mpls_msg_id;
  msg->id = nch->mpls_msg_id;

  /* Encode service.  */
  nbytes = nsm_encode_ftn_data_ipv4 (&pnt, &size, msg);
  if (nbytes < 0)
    return nbytes;

  /* Set FTN message id.  */
  if (mpls_msg_id)
    *mpls_msg_id = nch->mpls_msg_id;

  return nsm_client_send_message (nch, vr_id, vrf_id,
                                  NSM_MSG_MPLS_FTN_IPV4, nbytes, &msg_id);
}

s_int32_t
nsm_client_send_ftn_del_slow_ipv4 (struct nsm_client *nc,
                                   struct ftn_add_data *msg)
{
  struct nsm_client_handler *nch = nc->async;
  u_int32_t msg_id;
  u_int16_t size;
  u_char *pnt;
  int nbytes;
  u_int32_t vr_id = 0;
  vrf_id_t vrf_id = 0;

  /* We use async connection. */
  if (! nch || ! nch->up)
    return -1;

  /* Set pnt and size.  */
  pnt = nch->buf + NSM_MSG_HEADER_SIZE;
  size = nch->len - NSM_MSG_HEADER_SIZE;

  /* This is delete message.  */
  msg->flags = 0;
  msg->id = 0;

  /* Encode service.  */
  nbytes = nsm_encode_ftn_data_ipv4 (&pnt, &size, msg);
  if (nbytes < 0)
    return nbytes;

  return nsm_client_send_message (nch, vr_id, vrf_id,
                                  NSM_MSG_MPLS_FTN_IPV4, nbytes, &msg_id);
}

s_int32_t
nsm_client_send_ftn_del_ipv4 (struct nsm_client *nc,
                              struct ftn_del_data *fdd)
{
  struct nsm_client_handler *nch = nc->async;
  struct ftn_add_data msg;
  u_int32_t msg_id;
  u_int16_t size;
  u_char *pnt;
  int nbytes;
  u_int32_t vr_id = 0;
  vrf_id_t vrf_id = 0;

  /* We use async connection. */
  if (! nch || ! nch->up)
    return -1;

  /* Set pnt and size.  */
  pnt = nch->buf + NSM_MSG_HEADER_SIZE;
  size = nch->len - NSM_MSG_HEADER_SIZE;

  /* This is fast delete message.  */
  pal_mem_set (&msg, 0, sizeof (struct ftn_add_data));
  SET_FLAG (msg.flags, NSM_MSG_FTN_FAST_DELETE);

  /* Assign data.  */
  msg.vrf_id = fdd->vrf_id;
  prefix_copy (&msg.fec_prefix, &fdd->fec);
  msg.ftn_ix = fdd->ftn_ix;

  /* Encode service.  */
  nbytes = nsm_encode_ftn_data_ipv4 (&pnt, &size, &msg);
  if (nbytes < 0)
    return nbytes;

  return nsm_client_send_message (nch, vr_id, vrf_id,
                                  NSM_MSG_MPLS_FTN_IPV4, nbytes, &msg_id);
}

s_int32_t
nsm_client_send_ilm_add_ipv4 (struct nsm_client *nc, struct ilm_add_data *msg,
                              u_int32_t *mpls_msg_id)
{
  struct nsm_client_handler *nch = nc->async;
  u_int32_t msg_id;
  u_int16_t size;
  u_char *pnt;
  int nbytes;
  u_int32_t vr_id = 0;
  vrf_id_t vrf_id = 0;

  /* We use async connection. */
  if (! nch || ! nch->up)
    return -1;

  /* Set pnt and size.  */
  pnt = nch->buf + NSM_MSG_HEADER_SIZE;
  size = nch->len - NSM_MSG_HEADER_SIZE;

  /* This is add message.  */
  SET_FLAG (msg->flags, NSM_MSG_ILM_ADD);

  if (++nch->mpls_msg_id == 0)
    ++nch->mpls_msg_id;
  msg->id = nch->mpls_msg_id;

  /* Encode service.  */
  nbytes = nsm_encode_ilm_data_ipv4 (&pnt, &size, msg);
  if (nbytes < 0)
    return nbytes;

  /* Set FTN message id.  */
  if (mpls_msg_id)
    *mpls_msg_id = nch->mpls_msg_id;

  return nsm_client_send_message (nch, vr_id, vrf_id,
                                  NSM_MSG_MPLS_ILM_IPV4, nbytes, &msg_id);
}

s_int32_t
nsm_client_send_ilm_fast_del_ipv4 (struct nsm_client *nc,
                                   struct ilm_del_data *msg)
{
  struct nsm_client_handler *nch = nc->async;
  struct ilm_add_data iad;
  u_int32_t msg_id;
  u_int16_t size;
  u_char *pnt;
  int nbytes;
  u_int32_t vr_id = 0;
  vrf_id_t vrf_id = 0;

  /* We use async connection. */
  if (! nch || ! nch->up)
    return -1;

  /* Set pnt and size.  */
  pnt = nch->buf + NSM_MSG_HEADER_SIZE;
  size = nch->len - NSM_MSG_HEADER_SIZE;

  /* Fill data. */
  pal_mem_set (&iad, 0, sizeof (struct ilm_add_data));
  iad.iif_ix = msg->iif_ix;
  iad.in_label = msg->in_label;
  iad.ilm_ix = msg->ilm_ix;

  iad.flags = NSM_MSG_ILM_FAST_DELETE;

  /* Encode service.  */
  nbytes = nsm_encode_ilm_data_ipv4 (&pnt, &size, &iad);
  if (nbytes < 0)
    return nbytes;

  return nsm_client_send_message (nch, vr_id, vrf_id,
                                  NSM_MSG_MPLS_ILM_IPV4, nbytes, &msg_id);
}

s_int32_t
nsm_client_send_ilm_del_ipv4 (struct nsm_client *nc,
                              struct ilm_add_data *msg)
{
  struct nsm_client_handler *nch = nc->async;
  u_int32_t msg_id;
  u_int16_t size;
  u_char *pnt;
  int nbytes;
  u_int32_t vr_id = 0;
  vrf_id_t vrf_id = 0;

  /* We use async connection. */
  if (! nch || ! nch->up)
    return -1;

  /* Set pnt and size.  */
  pnt = nch->buf + NSM_MSG_HEADER_SIZE;
  size = nch->len - NSM_MSG_HEADER_SIZE;

  /* This is a delete message.  */
  msg->flags = 0;
  SET_FLAG (msg->flags, NSM_MSG_ILM_DELETE);
  msg->id = 0;

  /* Encode service.  */
  nbytes = nsm_encode_ilm_data_ipv4 (&pnt, &size, msg);
  if (nbytes < 0)
    return nbytes;

  return nsm_client_send_message (nch, vr_id, vrf_id,
                                  NSM_MSG_MPLS_ILM_IPV4, nbytes, &msg_id);
}

#if defined HAVE_MPLS_OAM || defined HAVE_NSM_MPLS_OAM
s_int32_t
nsm_client_send_mpls_vpn_rd (u_int32_t vr_id, vrf_id_t vrf_id,
                             struct nsm_msg_vpn_rd *msg, struct nsm_client *nc)
{
  struct nsm_client_handler *nch = nc->async;
  u_int32_t msg_id;
  u_int16_t size;
  u_char *pnt;
  int nbytes;

  /* We'll use async connection */
  if (!nch || !nch->up)
    return -1;

  /* Sent pnt and size */
  pnt = nch->buf + NSM_MSG_HEADER_SIZE;
  size = nch->len - NSM_MSG_HEADER_SIZE;

  /* Encode details */
  nbytes = nsm_encode_mpls_vpn_rd (&pnt, &size, msg);
  if (nbytes < 0)
    return nbytes;

  return nsm_client_send_message (nch, vr_id, vrf_id,
                                  NSM_MSG_MPLS_OAM_L3VPN, nbytes, &msg_id);
}
#endif /* HAVE_MPLS_OAM || HAVE_NSM_MPLS_OAM */
#endif /* HAVE_MPLS */

#ifdef HAVE_MPLS_VC

#if 0
s_int32_t
nsm_client_send_vc_fib_add ( struct nsm_client *nc,
                             struct vc_fib_add_data *data,
                             u_int32_t *mpls_msg_id)
{
  struct nsm_client_handler *nch = nc->async;
  u_int32_t vr_id = 0;
  vrf_id_t vrf_id = 0;
  u_int32_t msg_id;
  u_int16_t size;
  u_char *pnt;
  int nbytes;

  /* We can use an async connection */
  if (!nch || !nch->up)
    return -1;

  /* set pnt and size. */
  pnt = nch->buf + NSM_MSG_HEADER_SIZE;
  size = nch->len - NSM_MSG_HEADER_SIZE;

  /* Set flag for Add message */
  data->flags = 0;
  SET_FLAG (data->flags, NSM_MSG_VC_FIB_ADD);

  if (++nch->mpls_msg_id == 0)
    ++nch->mpls_msg_id;
  data->id = nch->mpls_msg_id;

  /* Encode the data required */
  nbytes = nsm_encode_vc_fib_add_data (&pnt, &size, data);
  if (nbytes < 0)
    return nbytes;

  return nsm_client_send_message (nch, vr_id, vrf_id,
                                  NSM_MSG_MPLS_VC_FIB_ADD, nbytes, &msg_id);
}

s_int32_t
nsm_client_send_vc_fib_del ( struct nsm_client *nc,
                            struct vc_fib_del_data *data)
{
  struct nsm_client_handler *nch = nc->async;
  u_int32_t vr_id = 0;
  vrf_id_t vrf_id = 0;
  u_int32_t msg_id;
  u_int16_t size;
  u_char *pnt;
  int nbytes;

  /* Async Connection is used */
  if (!nch || !nch->up)
    return -1;

  /* Set pnt and size */
  pnt = nch->buf + NSM_MSG_HEADER_SIZE;
  size = nch->len - NSM_MSG_HEADER_SIZE;

  /* This is a fib del message. */
  data->flags = 0;
  SET_FLAG (data->flags, NSM_MSG_VC_FIB_DEL);

  /* Encode service.  */
  nbytes = nsm_encode_vc_fib_delete_data (&pnt, &size, data);
  if (nbytes < 0)
    return nbytes;

  return nsm_client_send_message (nch, vr_id, vrf_id,
                                  NSM_MSG_MPLS_VC_FIB_DELETE, nbytes, &msg_id);
}

s_int32_t
nsm_client_send_vc_ftn_add_ipv4 (struct nsm_client *nc,
                                 VC_FTN_ADD_DATA *data,
                                 u_int32_t *mpls_msg_id)
{
  struct nsm_client_handler *nch = nc->async;
  u_int32_t msg_id;
  u_int16_t size;
  u_char *pnt;
  int nbytes;
  u_int32_t vr_id = 0;
  vrf_id_t vrf_id = 0;

  /* We use async connection. */
  if (! nch || ! nch->up)
    return -1;

  /* Set pnt and size.  */
  pnt = nch->buf + NSM_MSG_HEADER_SIZE;
  size = nch->len - NSM_MSG_HEADER_SIZE;

  /* This is add message.  */
  data->flags = 0;
  SET_FLAG (data->flags, NSM_MSG_VC_FTN_ADD);

  if (++nch->mpls_msg_id == 0)
    ++nch->mpls_msg_id;
  data->id = nch->mpls_msg_id;

  /* Encode service.  */
  nbytes = nsm_encode_vc_ftn_data_ipv4 (&pnt, &size, data);
  if (nbytes < 0)
    return nbytes;

  /* Set FTN message id.  */
  if (mpls_msg_id)
    *mpls_msg_id = nch->mpls_msg_id;

  return nsm_client_send_message (nch, vr_id, vrf_id,
                                  NSM_MSG_MPLS_VC_FTN_IPV4, nbytes, &msg_id);
}

s_int32_t
nsm_client_send_vc_ftn_del_ipv4 (struct nsm_client *nc,
                                 struct vc_ftn_del_data *data)
{
  struct nsm_client_handler *nch = nc->async;
  struct vc_ftn_entry vfe;
  u_int32_t msg_id;
  u_int16_t size;
  u_char *pnt;
  int nbytes;
  u_int32_t vr_id = 0;
  vrf_id_t vrf_id = 0;

  /* We use async connection. */
  if (! nch || ! nch->up)
    return -1;

  /* Set pnt and size.  */
  pnt = nch->buf + NSM_MSG_HEADER_SIZE;
  size = nch->len - NSM_MSG_HEADER_SIZE;

  /* Fill data. */
  pal_mem_set (&vfe, 0, sizeof (struct vc_ftn_entry));
  pal_mem_cpy (&vfe.owner, &data->owner, sizeof (struct mpls_owner));
  vfe.iif_ix = data->iif_ix;

  /* Encode service.  */
  nbytes = nsm_encode_vc_ftn_data_ipv4 (&pnt, &size, &vfe);
  if (nbytes < 0)
    return nbytes;

  return nsm_client_send_message (nch, vr_id, vrf_id,
                                  NSM_MSG_MPLS_VC_FTN_IPV4, nbytes, &msg_id);
}

s_int32_t
nsm_client_send_vc_ilm_add_ipv4 (struct nsm_client *nc,
                                 struct vc_ilm_add_data *data,
                                 u_int32_t *mpls_msg_id)
{
  struct nsm_client_handler *nch = nc->async;
  u_int32_t msg_id;
  u_int16_t size;
  u_char *pnt;
  int nbytes;
  u_int32_t vr_id = 0;
  vrf_id_t vrf_id = 0;

  /* We use async connection. */
  if (! nch || ! nch->up)
    return -1;

  /* Set pnt and size.  */
  pnt = nch->buf + NSM_MSG_HEADER_SIZE;
  size = nch->len - NSM_MSG_HEADER_SIZE;

  /* This is add message.  */
  data->flags = 0;
  SET_FLAG (data->flags, NSM_MSG_VC_ILM_ADD);

  if (++nch->mpls_msg_id == 0)
    ++nch->mpls_msg_id;
  data->id = nch->mpls_msg_id;

  /* Encode service.  */
  nbytes = nsm_encode_vc_ilm_data_ipv4 (&pnt, &size, data);
  if (nbytes < 0)
    return nbytes;

  /* Set FTN message id.  */
  if (mpls_msg_id)
    *mpls_msg_id = nch->mpls_msg_id;

  return nsm_client_send_message (nch, vr_id, vrf_id,
                                  NSM_MSG_MPLS_VC_ILM_IPV4, nbytes, &msg_id);
}

s_int32_t
nsm_client_send_vc_ilm_del_ipv4 (struct nsm_client *nc,
                                 struct vc_ilm_del_data *data)
{
  struct nsm_client_handler *nch = nc->async;
  struct vc_ilm_add_data viad;
  u_int32_t msg_id;
  u_int16_t size;
  u_char *pnt;
  int nbytes;
  u_int32_t vr_id = 0;
  vrf_id_t vrf_id = 0;

  /* We use async connection. */
  if (! nch || ! nch->up)
    return -1;

  /* Fill add data. */
  pal_mem_set (&viad, 0, sizeof (struct vc_ilm_add_data));
  viad.owner = data->owner;
  viad.iif_ix = data->iif_ix;
  viad.in_label = data->in_label;

  /* Set pnt and size.  */
  pnt = nch->buf + NSM_MSG_HEADER_SIZE;
  size = nch->len - NSM_MSG_HEADER_SIZE;

  /* Encode service.  */
  nbytes = nsm_encode_vc_ilm_data_ipv4 (&pnt, &size, &viad);
  if (nbytes < 0)
    return nbytes;

  return nsm_client_send_message (nch, vr_id, vrf_id,
                                  NSM_MSG_MPLS_VC_ILM_IPV4, nbytes, &msg_id);
}
#endif /* Old Forwarding additon module */

s_int32_t
nsm_client_send_vc_fib_add (struct nsm_client *nc,
                            struct nsm_msg_vc_fib_add *data)
{
  struct nsm_client_handler *nch = nc->async;
  u_int32_t msg_id;
  u_int16_t size;
  u_char *pnt;
  int nbytes;
  u_int32_t vr_id = 0;
  vrf_id_t vrf_id = 0;

  /* We use async connection. */
  if (! nch || ! nch->up)
    return -1;

  /* Set pnt and size.  */
  pnt = nch->buf + NSM_MSG_HEADER_SIZE;
  size = nch->len - NSM_MSG_HEADER_SIZE;

  /* Encode service.  */
  nbytes = nsm_encode_vc_fib_add (&pnt, &size, data);
  if (nbytes < 0)
    return nbytes;

  return nsm_client_send_message (nch, vr_id, vrf_id,
                                  NSM_MSG_MPLS_VC_FIB_ADD, nbytes, &msg_id);
}

s_int32_t
nsm_client_send_pw_status (struct nsm_client *nc,
                           struct nsm_msg_pw_status *data)
{
  struct nsm_client_handler *nch = nc->async;
  u_int32_t msg_id;
  u_int16_t size;
  u_char *pnt;
  int nbytes;
  u_int32_t vr_id = 0;
  vrf_id_t vrf_id = 0;

  /* We use async connection. */
  if (! nch || ! nch->up)
    return -1;

  /* Set pnt and size.  */
  pnt = nch->buf + NSM_MSG_HEADER_SIZE;
  size = nch->len - NSM_MSG_HEADER_SIZE;

  /* Encode service.  */
  nbytes = nsm_encode_pw_status (&pnt, &size, data);
  if (nbytes < 0)
    return nbytes;

  return nsm_client_send_message (nch, vr_id, vrf_id,
                                  NSM_MSG_MPLS_PW_STATUS, nbytes, &msg_id);
}

s_int32_t
nsm_client_send_tunnel_info (struct nsm_client *nc,
                             struct nsm_msg_vc_tunnel_info *data)
{
  struct nsm_client_handler *nch = nc->async;
  u_int32_t msg_id;
  u_int16_t size;
  u_char *pnt;
  int nbytes;
  u_int32_t vr_id = 0;
  vrf_id_t vrf_id = 0;

  /* We use async connection. */
  if (! nch || ! nch->up)
    return -1;

  /* Set pnt and size.  */
  pnt = nch->buf + NSM_MSG_HEADER_SIZE;
  size = nch->len - NSM_MSG_HEADER_SIZE;

  /* Encode service.  */
  nbytes = nsm_encode_vc_tunnel_info (&pnt, &size, data);
  if (nbytes < 0)
    return nbytes;

  return nsm_client_send_message (nch, vr_id, vrf_id,
                                  NSM_MSG_MPLS_VC_TUNNEL_INFO, nbytes,
                                  &msg_id);
}

s_int32_t
nsm_client_send_vc_fib_delete (struct nsm_client *nc,
                               struct nsm_msg_vc_fib_delete *data)
{
  struct nsm_client_handler *nch = nc->async;
  u_int32_t msg_id;
  u_int16_t size;
  u_char *pnt;
  int nbytes;
  u_int32_t vr_id = 0;
  vrf_id_t vrf_id = 0;

  /* We use async connection. */
  if (! nch || ! nch->up)
    return -1;

  /* Set pnt and size.  */
  pnt = nch->buf + NSM_MSG_HEADER_SIZE;
  size = nch->len - NSM_MSG_HEADER_SIZE;

  /* Encode service.  */
  nbytes = nsm_encode_vc_fib_delete (&pnt, &size, data);
  if (nbytes < 0)
    return nbytes;

  return nsm_client_send_message (nch, vr_id, vrf_id,
                                  NSM_MSG_MPLS_VC_FIB_DELETE, nbytes, &msg_id);
}
#endif /* HAVE_MPLS_VC */

#ifdef HAVE_VPLS
s_int32_t
nsm_client_send_mac_address_withdraw (struct nsm_client *nc,
                                      struct nsm_msg_vpls_mac_withdraw *data)
{
  struct nsm_client_handler *nch = nc->async;
  u_int32_t msg_id;
  u_int16_t size;
  u_char *pnt;
  int nbytes;
  u_int32_t vr_id = 0;
  vrf_id_t vrf_id = 0;

  /* We use async connection. */
  if (! nch || ! nch->up)
    return -1;

  /* Set pnt and size.  */
  pnt = nch->buf + NSM_MSG_HEADER_SIZE;
  size = nch->len - NSM_MSG_HEADER_SIZE;

  /* Encode service.  */
  nbytes = nsm_encode_vpls_mac_withdraw (&pnt, &size, data);
  if (nbytes < 0)
    return nbytes;

  return nsm_client_send_message (nch, vr_id, vrf_id,
                                  NSM_MSG_VPLS_MAC_WITHDRAW, nbytes, &msg_id);
}
#endif /* HAVE_VPLS */

#ifdef HAVE_MCAST_IPV4
/* VIF add */
s_int32_t
nsm_client_send_ipv4_vif_add (u_int32_t vr_id,
                              u_int32_t vrf_id,
                              struct nsm_client *nc,
                              struct nsm_msg_ipv4_vif_add *msg,
                              struct nsm_msg_ipv4_mrib_notification *notif,
                              u_int32_t *msg_id)
{
  struct nsm_client_handler *nch = nc->async;
  struct nsm_msg_header header;
  struct lib_globals *zg;
  s_int32_t nbytes;
  u_int16_t size;
  u_char *pnt;
  int ret_type = -1;
  int ret;
  bool_t add_pend;

  /* We use async connection for IPv6 route updates.  */
  if (! nch || ! nch->up)
    return -1;

  /* Do not allow sync messages on HA standby */
#ifdef HAVE_HA
  if (HA_IS_STANDBY (nc->zg))
    return 0;
#endif /* HAVE_HA */

  pal_mem_set (&header, 0, sizeof (header));

  /* Set pnt and size.  */
  pnt = nch->buf + NSM_MSG_HEADER_SIZE;
  size = nch->len - NSM_MSG_HEADER_SIZE;

  /* Encode service.  */
  nbytes = nsm_encode_ipv4_vif_add (&pnt, &size, msg);
  if (nbytes < 0)
    return nbytes;

  nbytes = nsm_client_send_message (nch, vr_id, vrf_id,
                                  NSM_MSG_IPV4_VIF_ADD, nbytes, msg_id);
  if (nbytes < 0)
    return nbytes;

  do
    {
      add_pend = PAL_FALSE;

      /* Sync wait for reply. */
      nbytes = nsm_client_read_sync (nch->mc, NULL, nch->mc->sock, &header,
                                     &ret_type);
      if (nbytes < 0)
        return nbytes;

      /* If return is MRIB notification, ensure that it is VIF notification */
      if ((ret_type == NSM_MSG_IPV4_MRIB_NOTIFICATION) &&
          (header.message_id == *msg_id) &&
          (header.vr_id == vr_id) &&
          (header.vrf_id == vrf_id)
         )
        {
          pal_mem_set (notif, 0, sizeof (struct nsm_msg_ipv4_mrib_notification));

          /* Set pnt and size. */
          pnt = nch->buf_in + NSM_MSG_HEADER_SIZE;
          size = nch->size_in;

          /* Decode route */
          ret = nsm_decode_ipv4_mrib_notification (&pnt, &size, notif);
          if (ret < 0)
            {
              return -1;
            }

          /* If notification is for VIF add we are done, else add to pending list */
          if (notif->type == NSM_MSG_IPV4_MRIB_NOTIFICATION_VIF_ADD)
            break;
          else
            add_pend = PAL_TRUE;
        }

      if ((ret_type != NSM_MSG_IPV4_MRIB_NOTIFICATION) ||
          (header.message_id != *msg_id) ||
          (header.vr_id != vr_id) ||
          (header.vrf_id != vrf_id) ||
          (add_pend == PAL_TRUE)
          )
        nsm_client_pending_message (nch, &header);
    }
  while ((ret_type != NSM_MSG_IPV4_MRIB_NOTIFICATION) ||
         (header.message_id != *msg_id) ||
         (header.vr_id != vr_id) ||
         (header.vrf_id != vrf_id) ||
         (add_pend == PAL_TRUE)
         );

  /* At here read thread may already registered as readable.
     Re-register read thread avoid hang.  */
  message_client_read_reregister (nch->mc);

  if (nc->debug)
    nsm_ipv4_mrib_notification_dump (nc->zg, notif);

  /* Launch event to process pending requests. */
  zg = nch->nc->zg;
  if (!zg->pend_read_thread)
    zg->pend_read_thread
      = thread_add_read_pend (zg, nsm_client_process_pending_msg, nch, 0);

  return 0;
}

/* VIF del */
s_int32_t
nsm_client_send_ipv4_vif_del (u_int32_t vr_id,
                              u_int32_t vrf_id,
                              struct nsm_client *nc,
                              struct nsm_msg_ipv4_vif_del *msg,
                              struct nsm_msg_ipv4_mrib_notification *notif,
                              u_int32_t *msg_id)
{
  struct nsm_client_handler *nch = nc->async;
  struct nsm_msg_header header;
  struct lib_globals *zg;
  s_int32_t nbytes;
  u_int16_t size;
  u_char *pnt;
  int ret_type = -1;
  int ret;
  bool_t add_pend;

  /* We use async connection for IPv6 route updates.  */
  if (! nch || ! nch->up)
    return -1;

  /* Do not allow sync messages on HA standby */
#ifdef HAVE_HA
  if (HA_IS_STANDBY (nc->zg))
    return 0;
#endif /* HAVE_HA */

  pal_mem_set (&header, 0, sizeof (header));

  /* Set pnt and size.  */
  pnt = nch->buf + NSM_MSG_HEADER_SIZE;
  size = nch->len - NSM_MSG_HEADER_SIZE;

  /* Encode service.  */
  nbytes = nsm_encode_ipv4_vif_del (&pnt, &size, msg);
  if (nbytes < 0)
    return nbytes;

  nbytes = nsm_client_send_message (nch, vr_id, vrf_id,
                                    NSM_MSG_IPV4_VIF_DEL, nbytes, msg_id);
  if (nbytes < 0)
    return nbytes;

  do
    {
      add_pend = PAL_FALSE;

      /* Sync wait for reply. */
      nbytes = nsm_client_read_sync (nch->mc, NULL, nch->mc->sock, &header,
                                     &ret_type);
      if (nbytes < 0)
        return nbytes;

      /* If return is MRIB notification, ensure that it is VIF notification */
      if ((ret_type == NSM_MSG_IPV4_MRIB_NOTIFICATION) &&
          (header.message_id == *msg_id) &&
          (header.vr_id == vr_id) &&
          (header.vrf_id == vrf_id)
         )
        {
          pal_mem_set (notif, 0, sizeof (struct nsm_msg_ipv4_mrib_notification));

          /* Set pnt and size. */
          pnt = nch->buf_in + NSM_MSG_HEADER_SIZE;
          size = nch->size_in;

          /* Decode route */
          ret = nsm_decode_ipv4_mrib_notification (&pnt, &size, notif);
          if (ret < 0)
            {
              return -1;
            }

          /* If notification is for VIF del we are done, else add to pending list */
          if (notif->type == NSM_MSG_IPV4_MRIB_NOTIFICATION_VIF_DEL)
            break;
          else
            add_pend = PAL_TRUE;
        }

      if ((ret_type != NSM_MSG_IPV4_MRIB_NOTIFICATION) ||
          (header.message_id != *msg_id) ||
          (header.vr_id != vr_id) ||
          (header.vrf_id != vrf_id) ||
          (add_pend == PAL_TRUE)
          )
        nsm_client_pending_message (nch, &header);
    }
  while ((ret_type != NSM_MSG_IPV4_MRIB_NOTIFICATION) ||
         (header.message_id != *msg_id) ||
         (header.vr_id != vr_id) ||
         (header.vrf_id != vrf_id) ||
         (add_pend == PAL_TRUE)
         );

  /* At here read thread may already registered as readable.
     Re-register read thread avoid hang.  */
  message_client_read_reregister (nch->mc);

  if (nc->debug)
    nsm_ipv4_mrib_notification_dump (nc->zg, notif);

  /* Launch event to process pending requests. */
  zg = nch->nc->zg;
  if (!zg->pend_read_thread)
    zg->pend_read_thread
      = thread_add_read_pend (zg, nsm_client_process_pending_msg, nch, 0);

  return 0;
}

/* MRT add */
s_int32_t
nsm_client_send_ipv4_mrt_add (u_int32_t vr_id,
                              u_int32_t vrf_id,
                              struct nsm_client *nc,
                              struct nsm_msg_ipv4_mrt_add *msg,
                              u_int32_t *msg_id)
{
  struct nsm_client_handler *nch = nc->async;
  u_int16_t size;
  u_char *pnt;
  s_int32_t nbytes;

  if (! nch || ! nch->up)
    return -1;

  /* Set pnt and size.  */
  pnt = nch->buf + NSM_MSG_HEADER_SIZE;
  size = nch->len - NSM_MSG_HEADER_SIZE;

  /* Encode service.  */
  nbytes = nsm_encode_ipv4_mrt_add (&pnt, &size, msg);
  if (nbytes < 0)
    return nbytes;

  return nsm_client_send_message (nch, vr_id, vrf_id,
                                    NSM_MSG_IPV4_MRT_ADD, nbytes, msg_id);
}

/* MRT del */
s_int32_t
nsm_client_send_ipv4_mrt_del (u_int32_t vr_id,
                              u_int32_t vrf_id,
                              struct nsm_client *nc,
                              struct nsm_msg_ipv4_mrt_del *msg,
                              u_int32_t *msg_id)
{
  struct nsm_client_handler *nch = nc->async;
  u_int16_t size;
  u_char *pnt;
  s_int32_t nbytes;

  if (! nch || ! nch->up)
    return -1;

  /* Set pnt and size.  */
  pnt = nch->buf + NSM_MSG_HEADER_SIZE;
  size = nch->len - NSM_MSG_HEADER_SIZE;

  /* Encode service.  */
  nbytes = nsm_encode_ipv4_mrt_del (&pnt, &size, msg);
  if (nbytes < 0)
    return nbytes;

  return nsm_client_send_message (nch, vr_id, vrf_id,
                                    NSM_MSG_IPV4_MRT_DEL, nbytes, msg_id);
}

/* MRT Stat flags update */
s_int32_t
nsm_client_send_ipv4_mrt_stat_flags_update (u_int32_t vr_id,
                                            u_int32_t vrf_id,
                                            struct nsm_client *nc,
                                            struct nsm_msg_ipv4_mrt_stat_flags_update *msg,
                                            u_int32_t *msg_id)
{
  struct nsm_client_handler *nch = nc->async;
  u_int16_t size;
  u_char *pnt;
  s_int32_t nbytes;

  if (! nch || ! nch->up)
    return -1;

  /* Set pnt and size.  */
  pnt = nch->buf + NSM_MSG_HEADER_SIZE;
  size = nch->len - NSM_MSG_HEADER_SIZE;

  /* Encode service.  */
  nbytes = nsm_encode_ipv4_mrt_stat_flags_update (&pnt, &size, msg);
  if (nbytes < 0)
    return nbytes;

  return nsm_client_send_message (nch, vr_id, vrf_id,
                                  NSM_MSG_IPV4_MRT_STAT_FLAGS_UPDATE,
                                  nbytes, msg_id);
}

/* MRT Stat Refresh flags update */
s_int32_t
nsm_client_send_ipv4_mrt_state_refresh_flag_update (u_int32_t vr_id,
                                                    u_int32_t vrf_id,
                                                    struct nsm_client *nc,
                                                    struct nsm_msg_ipv4_mrt_state_refresh_flag_update *msg,
                                                    u_int32_t *msg_id)
{
  struct nsm_client_handler *nch = nc->async;
  u_int16_t size;
  u_char *pnt;
  s_int32_t nbytes;

  if (! nch || ! nch->up)
    return -1;

  /* Set pnt and size.  */
  pnt = nch->buf + NSM_MSG_HEADER_SIZE;
  size = nch->len - NSM_MSG_HEADER_SIZE;

  /* Encode service.  */
  nbytes = nsm_encode_ipv4_mrt_state_refresh_flag_update (&pnt, &size, msg);
  if (nbytes < 0)
    return nbytes;

  return nsm_client_send_message (nch, vr_id, vrf_id,
                                  NSM_MSG_IPV4_MRT_ST_REFRESH_FLAG_UPDATE,
                                  nbytes, msg_id);
}

/* MRT WHOLEPKT REPLY */
s_int32_t
nsm_client_send_ipv4_mrt_wholepkt_reply (u_int32_t vr_id,
                                         u_int32_t vrf_id,
                                         struct nsm_client *nc,
                                         struct nsm_msg_ipv4_mrt_wholepkt_reply *msg,
                                         u_int32_t *msg_id)
{
  struct nsm_client_handler *nch = nc->async;
  u_int16_t size;
  u_char *pnt;
  s_int32_t nbytes;

  if (! nch || ! nch->up)
    return -1;

  /* Set pnt and size.  */
  pnt = nch->buf + NSM_MSG_HEADER_SIZE;
  size = nch->len - NSM_MSG_HEADER_SIZE;

  /* Encode service.  */
  nbytes = nsm_encode_ipv4_mrt_wholepkt_reply (&pnt, &size, msg);
  if (nbytes < 0)
    return nbytes;

  return nsm_client_send_message (nch, vr_id, vrf_id,
                                    NSM_MSG_IPV4_MRT_WHOLEPKT_REPLY, nbytes,
                                    msg_id);
}

/* Mtrace Query */
s_int32_t
nsm_client_send_mtrace_query_msg (u_int32_t vr_id,
                                  u_int32_t vrf_id,
                                  struct nsm_client *nc,
                                  struct nsm_msg_mtrace_query *msg,
                                  u_int32_t *msg_id)
{
  struct nsm_client_handler *nch = nc->async;
  u_int16_t size;
  u_char *pnt;
  s_int32_t nbytes;

  /* We use async connection for IPv6 route updates.  */
  if (! nch || ! nch->up)
    return -1;

  /* Set pnt and size.  */
  pnt = nch->buf + NSM_MSG_HEADER_SIZE;
  size = nch->len - NSM_MSG_HEADER_SIZE;

  /* Encode service.  */
  nbytes = nsm_encode_mtrace_query_msg (&pnt, &size, msg);
  if (nbytes < 0)
    return nbytes;

  return nsm_client_send_message (nch, vr_id, vrf_id,
                                    NSM_MSG_MTRACE_QUERY, nbytes, msg_id);
}

/* Mtrace Query */
s_int32_t
nsm_client_send_mtrace_request_msg (u_int32_t vr_id,
                                    u_int32_t vrf_id,
                                    struct nsm_client *nc,
                                    struct nsm_msg_mtrace_request *msg,
                                    u_int32_t *msg_id)
{
  struct nsm_client_handler *nch = nc->async;
  u_int16_t size;
  u_char *pnt;
  s_int32_t nbytes;

  /* We use async connection for IPv6 route updates.  */
  if (! nch || ! nch->up)
    return -1;

  /* Set pnt and size.  */
  pnt = nch->buf + NSM_MSG_HEADER_SIZE;
  size = nch->len - NSM_MSG_HEADER_SIZE;

  /* Encode service.  */
  nbytes = nsm_encode_mtrace_request_msg (&pnt, &size, msg);
  if (nbytes < 0)
    return nbytes;

  return nsm_client_send_message (nch, vr_id, vrf_id,
                                    NSM_MSG_MTRACE_REQUEST, nbytes, msg_id);
}
#endif /* HAVE_MCAST_IPV4 */

#ifdef HAVE_MCAST_IPV6
/* MIF add */
s_int32_t
nsm_client_send_ipv6_mif_add (u_int32_t vr_id,
                              u_int32_t vrf_id,
                              struct nsm_client *nc,
                              struct nsm_msg_ipv6_mif_add *msg,
                              struct nsm_msg_ipv6_mrib_notification *notif,
                              u_int32_t *msg_id)
{
  struct nsm_client_handler *nch = nc->async;
  struct nsm_msg_header header;
  struct lib_globals *zg;
  s_int32_t nbytes;
  u_int16_t size;
  u_char *pnt;
  int ret_type = -1;
  int ret;
  bool_t add_pend;

  /* We use async connection for IPv6 route updates.  */
  if (! nch || ! nch->up)
    return -1;

  /* Do not allow sync messages on HA standby */
#ifdef HAVE_HA
  if (HA_IS_STANDBY (nc->zg))
    return 0;
#endif /* HAVE_HA */

  pal_mem_set (&header, 0, sizeof (header));

  /* Set pnt and size.  */
  pnt = nch->buf + NSM_MSG_HEADER_SIZE;
  size = nch->len - NSM_MSG_HEADER_SIZE;

  /* Encode msg.  */
  nbytes = nsm_encode_ipv6_mif_add (&pnt, &size, msg);
  if (nbytes < 0)
    return nbytes;

  nbytes = nsm_client_send_message (nch, vr_id, vrf_id,
                                  NSM_MSG_IPV6_MIF_ADD, nbytes, msg_id);
  if (nbytes < 0)
    return nbytes;

  do
    {
      add_pend = PAL_FALSE;

      /* Sync wait for reply. */
      nbytes = nsm_client_read_sync (nch->mc, NULL, nch->mc->sock, &header,
                                     &ret_type);
      if (nbytes < 0)
        return nbytes;

      /* If return is MRIB notification, ensure that it is VIF notification */
      if ((ret_type == NSM_MSG_IPV6_MRIB_NOTIFICATION) &&
          (header.message_id == *msg_id) &&
          (header.vr_id == vr_id) &&
          (header.vrf_id == vrf_id)
         )
        {
          pal_mem_set (notif, 0, sizeof (struct nsm_msg_ipv6_mrib_notification));

          /* Set pnt and size. */
          pnt = nch->buf_in + NSM_MSG_HEADER_SIZE;
          size = nch->size_in;

          /* Decode route */
          ret = nsm_decode_ipv6_mrib_notification (&pnt, &size, notif);
          if (ret < 0)
            {
              return -1;
            }

          /* If notification is for VIF add we are done, else add to pending list */
          if (notif->type == NSM_MSG_IPV6_MRIB_NOTIFICATION_MIF_ADD)
            break;
          else
            add_pend = PAL_TRUE;
        }

      if ((ret_type != NSM_MSG_IPV6_MRIB_NOTIFICATION) ||
          (header.message_id != *msg_id) ||
          (header.vr_id != vr_id) ||
          (header.vrf_id != vrf_id) ||
          (add_pend == PAL_TRUE)
          )
        nsm_client_pending_message (nch, &header);
    }
  while ((ret_type != NSM_MSG_IPV6_MRIB_NOTIFICATION) ||
         (header.message_id != *msg_id) ||
         (header.vr_id != vr_id) ||
         (header.vrf_id != vrf_id) ||
         (add_pend == PAL_TRUE)
         );

  /* At here read thread may already registered as readable.
     Re-register read thread avoid hang.  */
  message_client_read_reregister (nch->mc);

  if (nc->debug)
    nsm_ipv6_mrib_notification_dump (nc->zg, notif);

  /* Launch event to process pending requests. */
  zg = nch->nc->zg;
  if (!zg->pend_read_thread)
    zg->pend_read_thread
      = thread_add_read_pend (zg, nsm_client_process_pending_msg, nch, 0);

  return 0;
}

/* MIF del */
s_int32_t
nsm_client_send_ipv6_mif_del (u_int32_t vr_id,
                              u_int32_t vrf_id,
                              struct nsm_client *nc,
                              struct nsm_msg_ipv6_mif_del *msg,
                              struct nsm_msg_ipv6_mrib_notification *notif,
                              u_int32_t *msg_id)
{
  struct nsm_client_handler *nch = nc->async;
  struct nsm_msg_header header;
  struct lib_globals *zg;
  s_int32_t nbytes;
  u_int16_t size;
  u_char *pnt;
  int ret_type = -1;
  int ret;
  bool_t add_pend;

  /* We use async connection for IPv6 route updates.  */
  if (! nch || ! nch->up)
    return -1;

  /* Do not allow sync messages on HA standby */
#ifdef HAVE_HA
  if (HA_IS_STANDBY (nc->zg))
    return 0;
#endif /* HAVE_HA */

  pal_mem_set (&header, 0, sizeof (header));

  /* Set pnt and size.  */
  pnt = nch->buf + NSM_MSG_HEADER_SIZE;
  size = nch->len - NSM_MSG_HEADER_SIZE;

  /* Encode msg.  */
  nbytes = nsm_encode_ipv6_mif_del (&pnt, &size, msg);
  if (nbytes < 0)
    return nbytes;

  nbytes = nsm_client_send_message (nch, vr_id, vrf_id,
                                  NSM_MSG_IPV6_MIF_DEL, nbytes, msg_id);
  if (nbytes < 0)
    return nbytes;

  do
    {
      add_pend = PAL_FALSE;

      /* Sync wait for reply. */
      nbytes = nsm_client_read_sync (nch->mc, NULL, nch->mc->sock, &header,
                                     &ret_type);
      if (nbytes < 0)
        return nbytes;

      /* If return is MRIB notification, ensure that it is VIF notification */
      if ((ret_type == NSM_MSG_IPV6_MRIB_NOTIFICATION) &&
          (header.message_id == *msg_id) &&
          (header.vr_id == vr_id) &&
          (header.vrf_id == vrf_id)
         )
        {
          pal_mem_set (notif, 0, sizeof (struct nsm_msg_ipv6_mrib_notification));

          /* Set pnt and size. */
          pnt = nch->buf_in + NSM_MSG_HEADER_SIZE;
          size = nch->size_in;

          /* Decode route */
          ret = nsm_decode_ipv6_mrib_notification (&pnt, &size, notif);
          if (ret < 0)
            {
              return -1;
            }

          /* If notification is for VIF del we are done, else add to pending list */
          if (notif->type == NSM_MSG_IPV6_MRIB_NOTIFICATION_MIF_DEL)
            break;
          else
            add_pend = PAL_TRUE;
        }

      if ((ret_type != NSM_MSG_IPV6_MRIB_NOTIFICATION) ||
          (header.message_id != *msg_id) ||
          (header.vr_id != vr_id) ||
          (header.vrf_id != vrf_id) ||
          (add_pend == PAL_TRUE)
          )
        nsm_client_pending_message (nch, &header);
    }
  while ((ret_type != NSM_MSG_IPV6_MRIB_NOTIFICATION) ||
         (header.message_id != *msg_id) ||
         (header.vr_id != vr_id) ||
         (header.vrf_id != vrf_id) ||
         (add_pend == PAL_TRUE)
         );

  /* At here read thread may already registered as readable.
     Re-register read thread avoid hang.  */
  message_client_read_reregister (nch->mc);

  if (nc->debug)
    nsm_ipv6_mrib_notification_dump (nc->zg, notif);

  /* Launch event to process pending requests. */
  zg = nch->nc->zg;
  if (!zg->pend_read_thread)
    zg->pend_read_thread
      = thread_add_read_pend (zg, nsm_client_process_pending_msg, nch, 0);

  return 0;
}

/* MRT add */
s_int32_t
nsm_client_send_ipv6_mrt_add (u_int32_t vr_id,
                              u_int32_t vrf_id,
                              struct nsm_client *nc,
                              struct nsm_msg_ipv6_mrt_add *msg,
                              u_int32_t *msg_id)
{
  struct nsm_client_handler *nch = nc->async;
  u_int16_t size;
  u_char *pnt;
  s_int32_t nbytes;

  if (! nch || ! nch->up)
    return -1;

  /* Set pnt and size.  */
  pnt = nch->buf + NSM_MSG_HEADER_SIZE;
  size = nch->len - NSM_MSG_HEADER_SIZE;

  /* Encode service.  */
  nbytes = nsm_encode_ipv6_mrt_add (&pnt, &size, msg);
  if (nbytes < 0)
    return nbytes;

  return nsm_client_send_message (nch, vr_id, vrf_id,
                                    NSM_MSG_IPV6_MRT_ADD, nbytes, msg_id);
}

/* MRT del */
s_int32_t
nsm_client_send_ipv6_mrt_del (u_int32_t vr_id,
                              u_int32_t vrf_id,
                              struct nsm_client *nc,
                              struct nsm_msg_ipv6_mrt_del *msg,
                              u_int32_t *msg_id)
{
  struct nsm_client_handler *nch = nc->async;
  u_int16_t size;
  u_char *pnt;
  s_int32_t nbytes;

  if (! nch || ! nch->up)
    return -1;

  /* Set pnt and size.  */
  pnt = nch->buf + NSM_MSG_HEADER_SIZE;
  size = nch->len - NSM_MSG_HEADER_SIZE;

  /* Encode service.  */
  nbytes = nsm_encode_ipv6_mrt_del (&pnt, &size, msg);
  if (nbytes < 0)
    return nbytes;

  return nsm_client_send_message (nch, vr_id, vrf_id,
                                    NSM_MSG_IPV6_MRT_DEL, nbytes, msg_id);
}

/* MRT Stat flags update */
s_int32_t
nsm_client_send_ipv6_mrt_stat_flags_update (u_int32_t vr_id,
                                            u_int32_t vrf_id,
                                            struct nsm_client *nc,
                                            struct nsm_msg_ipv6_mrt_stat_flags_update *msg,
                                            u_int32_t *msg_id)
{
  struct nsm_client_handler *nch = nc->async;
  u_int16_t size;
  u_char *pnt;
  s_int32_t nbytes;

  if (! nch || ! nch->up)
    return -1;

  /* Set pnt and size.  */
  pnt = nch->buf + NSM_MSG_HEADER_SIZE;
  size = nch->len - NSM_MSG_HEADER_SIZE;

  /* Encode service.  */
  nbytes = nsm_encode_ipv6_mrt_stat_flags_update (&pnt, &size, msg);
  if (nbytes < 0)
    return nbytes;

  return nsm_client_send_message (nch, vr_id, vrf_id,
                                  NSM_MSG_IPV6_MRT_STAT_FLAGS_UPDATE,
                                  nbytes, msg_id);
}


/* MRT Stat Refresh flags update */
s_int32_t
nsm_client_send_ipv6_mrt_state_refresh_flag_update (u_int32_t vr_id,
                                                    u_int32_t vrf_id,
                                                    struct nsm_client *nc,
                                                    struct nsm_msg_ipv6_mrt_state_refresh_flag_update *msg,
                                                    u_int32_t *msg_id)
{
  struct nsm_client_handler *nch = nc->async;
  u_int16_t size;
  u_char *pnt;
  s_int32_t nbytes;

  if (! nch || ! nch->up)
    return -1;

  /* Set pnt and size.  */
  pnt = nch->buf + NSM_MSG_HEADER_SIZE;
  size = nch->len - NSM_MSG_HEADER_SIZE;

  /* Encode service.  */
  nbytes = nsm_encode_ipv6_mrt_state_refresh_flag_update (&pnt, &size, msg);
  if (nbytes < 0)
    return nbytes;

  return nsm_client_send_message (nch, vr_id, vrf_id,
                                  NSM_MSG_IPV6_MRT_ST_REFRESH_FLAG_UPDATE,
                                  nbytes, msg_id);
}

/* MRT WHOLEPKT REPLY */
s_int32_t
nsm_client_send_ipv6_mrt_wholepkt_reply (u_int32_t vr_id,
                                         u_int32_t vrf_id,
                                         struct nsm_client *nc,
                                         struct nsm_msg_ipv6_mrt_wholepkt_reply *msg,
                                         u_int32_t *msg_id)
{
  struct nsm_client_handler *nch = nc->async;
  u_int16_t size;
  u_char *pnt;
  s_int32_t nbytes;

  if (! nch || ! nch->up)
    return -1;

  /* Set pnt and size.  */
  pnt = nch->buf + NSM_MSG_HEADER_SIZE;
  size = nch->len - NSM_MSG_HEADER_SIZE;

  /* Encode service.  */
  nbytes = nsm_encode_ipv6_mrt_wholepkt_reply (&pnt, &size, msg);
  if (nbytes < 0)
    return nbytes;

  return nsm_client_send_message (nch, vr_id, vrf_id,
                                    NSM_MSG_IPV6_MRT_WHOLEPKT_REPLY, nbytes,
                                    msg_id);
}
#endif /* HAVE_MCAST_IPV6 */

/* NSM_MSG_IF_DEL_DONE*/
s_int32_t
nsm_client_send_nsm_if_del_done(struct nsm_client *nc,
    struct nsm_msg_link *msg, u_int32_t *msg_id)
{
  struct nsm_client_handler *nch = nc->async;
  u_int16_t size;
  u_char *pnt;
  int nbytes;
  u_int32_t vr_id = 0;
  vrf_id_t vrf_id = 0;

  if (! nch || ! nch->up)
    return -1;

  /* Set pnt and size.  */
  pnt = nch->buf + NSM_MSG_HEADER_SIZE;
  size = nch->len - NSM_MSG_HEADER_SIZE;

  /* Encode service.  */
  nbytes = nsm_encode_link(&pnt, &size, msg);
  if (nbytes < 0)
    return nbytes;

  return nsm_client_send_message (nch, vr_id, vrf_id,
                                    NSM_MSG_IF_DEL_DONE, nbytes,
                                    msg_id);
}

#ifndef HAVE_CUSTOM1
#ifdef HAVE_L2
/* Send Bridge Message. */
s_int32_t
nsm_client_send_bridge_msg (struct nsm_client *nc,
                            struct nsm_msg_bridge *msg,
                            int msgtype)
{
  u_char *pnt;
  u_int16_t size;
  int nbytes;
  struct nsm_client_handler *nch;
  u_int32_t msg_id;
  u_int32_t vr_id = 0;
  vrf_id_t vrf_id = 0;

  if ( !nc || !msg )
    return -1;

  nch = nc->async;

  /* We use async connection.  */
  if (! nch || ! nch->up)
    return -1;

  /* Set pnt and size.  */
  pnt = nch->buf + NSM_MSG_HEADER_SIZE;
  size = nch->len - NSM_MSG_HEADER_SIZE;

  /* Encode route clean message. */
  nbytes = nsm_encode_bridge_msg (&pnt, &size, msg);
  if (nbytes < 0)
    return nbytes;

  return nsm_client_send_message (nch, vr_id, vrf_id,
                                  msgtype, nbytes, &msg_id);
}

/* Send Bridge port Message. */
s_int32_t
nsm_client_send_bridge_if_msg (struct nsm_client *nc,
                                 struct nsm_msg_bridge_if *msg,
                                 int msgtype)
{
  int nbytes;
  u_char *pnt;
  int ret = 0;
  u_int16_t size;
  u_int32_t msg_id;
  int ret_type = -1;
  vrf_id_t vrf_id = 0;
  u_int32_t vr_id = 0;
  struct lib_globals *zg;
  struct nsm_msg_header header;
  struct nsm_client_handler *nch;
  struct nsm_msg_bridge_if reply;

  if ( !nc || !msg )
    return -1;

  nch = nc->async;

  /* We use async connection.  */
  if (! nch || ! nch->up)
    return -1;

  /* Do not allow sync messages on HA standby */
#ifdef HAVE_HA
  if (HA_IS_STANDBY (nc->zg))
    return 0;
#endif /* HAVE_HA */

  /* Set pnt and size.  */
  pnt = nch->buf + NSM_MSG_HEADER_SIZE;
  size = nch->len - NSM_MSG_HEADER_SIZE;

  /* Encode route clean message. */
  nbytes = nsm_encode_bridge_if_msg (&pnt, &size, msg);
  if (nbytes < 0)
    return nbytes;

  nbytes = nsm_client_send_message (nch, vr_id, vrf_id,
                                    msgtype, nbytes, &msg_id);

  if (nbytes < 0)
    return nbytes;

  do
    {
      /* Sync wait for reply. */
      nbytes = nsm_client_read_sync (nch->mc, NULL, nch->mc->sock, &header,
                                     &ret_type);
      if (nbytes <= 0)
        return nbytes;

      if ((ret_type != msgtype)
          || (header.message_id != msg_id))
        nsm_client_pending_message (nch, &header);
    }
  while ((ret_type != msgtype)
         || (header.message_id != msg_id));

 /* At here read thread may already registered as readable.
     Re-register read thread avoid hang.  */
  message_client_read_reregister (nch->mc);

  pal_mem_set (&reply, 0, sizeof (struct nsm_msg_bridge_if));

  /* Set pnt and size. */
  pnt = nch->buf_in + NSM_MSG_HEADER_SIZE;
  size = nch->size_in;

  ret = nsm_decode_bridge_if_msg (&pnt, &size, &reply);

  if (ret < 0)
    return ret;

  if (reply.ifindex)
    XFREE (MTYPE_TMP, reply.ifindex);

  /* Launch event to process pending requests. */
  zg = nch->nc->zg;

  if (!zg->pend_read_thread)
    zg->pend_read_thread
      = thread_add_read_pend (zg, nsm_client_process_pending_msg, nch, 0);

  return 0;

}
#ifdef HAVE_G8032
s_int32_t
nsm_client_send_bridge_g8032_port_state_msg (struct nsm_client *nc,
                             struct nsm_msg_bridge_g8032_port *msg,
                             int msgtype)
{
  int nbytes;
  u_char *pnt;
  int ret = 0;
  u_int16_t size;
  u_int32_t msg_id;
  int ret_type = -1;
  vrf_id_t vrf_id = 0;
  u_int32_t vr_id = 0;
  struct nsm_msg_header header;
  struct nsm_client_handler *nch;
  struct nsm_msg_bridge_g8032_port reply;

  if ( !nc || !msg )
    return -1;

  nch = nc->async;

  /* We use async connection.  */
  if (! nch || ! nch->up)
    return -1;

  /* Expecting a reply from NSM.
   * Initializing the reply to be 0.
   */
  pal_mem_set (&reply, 0, sizeof (struct nsm_msg_bridge_g8032_port));

  /* Set pnt and size.  */
  pnt = nch->buf + NSM_MSG_HEADER_SIZE;
  size = nch->len - NSM_MSG_HEADER_SIZE;

  /* Encode G8032 Port State Message. */
  nbytes = nsm_encode_g8032_port_msg (&pnt, &size, msg);
  if (nbytes < 0)
    return nbytes;

  nbytes = nsm_client_send_message (nch, vr_id, vrf_id,
                                    msgtype, nbytes, &msg_id);

  if (nbytes < 0)
    return nbytes;

  do
    {
      /* Sync wait for reply. */
      nbytes = nsm_client_read_sync (nch->mc, NULL, nch->mc->sock, &header,
                                     &ret_type);
      if (nbytes <= 0)
        return nbytes;

      if ((ret_type != msgtype)
          || (header.message_id != msg_id))
        nsm_client_pending_message (nch, &header);
    }
  while ((ret_type != msgtype)
         || (header.message_id != msg_id));

 /* At here read thread may already registered as readable.
     Re-register read thread avoid hang.  */
  message_client_read_reregister (nch->mc);

  /* Set pnt and size. */
  pnt = nch->buf_in + NSM_MSG_HEADER_SIZE;
  size = nch->size_in;

  ret = nsm_decode_g8032_portstate_msg (&pnt, &size, &reply);

  if ((reply.state == NSM_BRIDGE_G8032_PORTSTATE_ERROR) ||
      (reply.fdb_flush == G8032_FLUSH_FAIL))
    return RESULT_ERROR;

  return ret;
}

#endif /*HAVE_G8032*/

/* Send Bridge port Message. */
s_int32_t
nsm_client_send_bridge_port_msg (struct nsm_client *nc,
                                 struct nsm_msg_bridge_port *msg,
                                 int msgtype)
{
  int nbytes;
  u_char *pnt;
  int ret = 0;
  u_int16_t size;
  u_int32_t msg_id;
  int ret_type = -1;
  vrf_id_t vrf_id = 0;
  u_int32_t vr_id = 0;
  struct lib_globals *zg;
  struct nsm_msg_header header;
  struct nsm_client_handler *nch;
  struct nsm_msg_bridge_port reply;

  if ( !nc || !msg )
    return -1;

  nch = nc->async;

  /* We use async connection.  */
  if (! nch || ! nch->up)
    return -1;

  /* Set pnt and size.  */
  pnt = nch->buf + NSM_MSG_HEADER_SIZE;
  size = nch->len - NSM_MSG_HEADER_SIZE;

  /* Encode route clean message. */
  nbytes = nsm_encode_bridge_port_msg (&pnt, &size, msg);
  if (nbytes < 0)
    return nbytes;

  nbytes = nsm_client_send_message (nch, vr_id, vrf_id,
                                    msgtype, nbytes, &msg_id);

  if (nbytes < 0)
    return nbytes;

  do
    {
      /* Sync wait for reply. */
      nbytes = nsm_client_read_sync (nch->mc, NULL, nch->mc->sock, &header,
                                     &ret_type);
      if (nbytes <= 0)
        return nbytes;

      if ((ret_type != msgtype)
          || (header.message_id != msg_id))
        nsm_client_pending_message (nch, &header);
    }
  while ((ret_type != msgtype)
         || (header.message_id != msg_id));

 /* At here read thread may already registered as readable.
     Re-register read thread avoid hang.  */
  message_client_read_reregister (nch->mc);

  pal_mem_set (&reply, 0, sizeof (struct nsm_msg_bridge_port));

  /* Set pnt and size. */
  pnt = nch->buf_in + NSM_MSG_HEADER_SIZE;
  size = nch->size_in;

  ret = nsm_decode_bridge_port_msg (&pnt, &size, &reply);

  if (ret < 0)
   return ret;

  if (reply.instance)
    XFREE (MTYPE_TMP, reply.instance);

  if (reply.state)
    XFREE (MTYPE_TMP, reply.state);

  /* Launch event to process pending requests. */
  zg = nch->nc->zg;

  if (!zg->pend_read_thread)
    zg->pend_read_thread
      = thread_add_read_pend (zg, nsm_client_process_pending_msg, nch, 0);

  return 0;
}

/* Send Bridge Enable Message. */
s_int32_t
nsm_client_send_bridge_enable_msg (struct nsm_client *nc,
                                   struct nsm_msg_bridge_enable *msg)
{
  int nbytes;
  u_char *pnt;
  u_int16_t size;
  u_int32_t msg_id;
  u_int32_t vr_id = 0;
  vrf_id_t vrf_id = 0;
  struct nsm_client_handler *nch;

  if ( !nc || !msg )
    return -1;

  nch = nc->async;

  /* We use async connection.  */
  if (! nch || ! nch->up)
    return -1;

  /* Set pnt and size.  */
  pnt = nch->buf + NSM_MSG_HEADER_SIZE;
  size = nch->len - NSM_MSG_HEADER_SIZE;

  /* Encode Bridge Enable message. */
  nbytes = nsm_encode_bridge_enable_msg (&pnt, &size, msg);

  if (nbytes < 0)
    return nbytes;

  return nsm_client_send_message (nch, vr_id, vrf_id,
                                  NSM_MSG_BRIDGE_SET_STATE, nbytes, &msg_id);
}

#if defined HAVE_PBB_TE && defined HAVE_B_BEB && defined HAVE_I_BEB && defined HAVE_CFM
/* Send port state Message to nsm from aps-onmd. */
s_int32_t
nsm_client_send_pbb_te_port_state_msg (struct nsm_client *nc,
                                       struct nsm_msg_bridge_pbb_te_port *msg)
{
  int nbytes;
  u_char *pnt;
  u_int16_t size;
  u_int32_t msg_id;
  u_int32_t vr_id = 0;
  vrf_id_t vrf_id = 0;
  struct nsm_client_handler *nch;

  if ( !nc || !msg )
    return -1;

  nch = nc->async;

  /* We use async connection.  */
  if (! nch || ! nch->up)
    return -1;

  /* Set pnt and size.  */
  pnt = nch->buf + NSM_MSG_HEADER_SIZE;
  size = nch->len - NSM_MSG_HEADER_SIZE;

  /* Encode Bridge Enable message. */
  nbytes = nsm_encode_bridge_pbb_te_port_msg (&pnt, &size, msg);

  if (nbytes < 0)
    return nbytes;

  return nsm_client_send_message (nch, vr_id, vrf_id,
                                  NSM_MSG_BRIDGE_PBB_TE_PORT_STATE, nbytes, &msg_id);
}

#endif /* HAVE_PBB_TE HAVE_B_BEB HAVE_I_BEB */

#endif /* HAVE_L2 */

#ifdef HAVE_VLAN
/* Send Vlan Message. */
s_int32_t
nsm_client_send_vlan_msg (struct nsm_client *nc,
                          struct nsm_msg_vlan *msg,
                          int msgtype)
{
  u_char *pnt;
  u_int16_t size;
  int nbytes;
  struct nsm_client_handler *nch;
  u_int32_t msg_id;
  u_int32_t vr_id = 0;
  vrf_id_t vrf_id = 0;

  if ( !nc || !msg )
    return -1;

  nch = nc->async;

  /* We use async connection.  */
  if (! nch || ! nch->up)
    return -1;

  /* Set pnt and size.  */
  pnt = nch->buf + NSM_MSG_HEADER_SIZE;
  size = nch->len - NSM_MSG_HEADER_SIZE;

  /* Encode route clean message. */
  nbytes = nsm_encode_vlan_msg (&pnt, &size, msg);
  if (nbytes < 0)
    return nbytes;

  return nsm_client_send_message (nch, vr_id, vrf_id,
                                  msgtype, nbytes, &msg_id);
}

/* Send Vlan Port message. Vlan added/deleted from interface */
s_int32_t
nsm_client_send_vlan_port_msg (struct nsm_client *nc,
                               struct nsm_msg_vlan_port *msg,
                               int msgtype)
{
  u_char *pnt;
  u_int16_t size;
  int nbytes;
  struct nsm_client_handler *nch;
  u_int32_t msg_id;
  u_int32_t vr_id = 0;
  vrf_id_t vrf_id = 0;

  if ( !nc || !msg )
    return -1;

  nch = nc->async;

  /* We use async connection.  */
  if (! nch || ! nch->up)
    return -1;

  /* Set pnt and size.  */
  pnt = nch->buf + NSM_MSG_HEADER_SIZE;
  size = nch->len - NSM_MSG_HEADER_SIZE;

  /* Encode route clean message. */
  nbytes = nsm_encode_vlan_port_msg (&pnt, &size, msg);
  if (nbytes < 0)
    return nbytes;

  return nsm_client_send_message (nch, vr_id, vrf_id,
                                  msgtype, nbytes, &msg_id);
}
#endif /* HAVE_VLAN */
#endif /* HAVE_CUSTOM1 */

#ifdef HAVE_AUTHD
s_int32_t
nsm_client_send_auth_port_state (struct nsm_client *nc, u_int32_t vr_id,
                                 struct nsm_msg_auth_port_state *msg)
{
  struct nsm_client_handler *nch = nc->async;
  u_int32_t vrf_id = 0;
  u_int32_t msg_id = 0;
  u_int16_t size;
  u_char *pnt;
  int nbytes;

  /* We use async connection. */
  if (! nch || ! nch->up)
    return -1;

  /* Set pnt and size.  */
  pnt = nch->buf + NSM_MSG_HEADER_SIZE;
  size = nch->len - NSM_MSG_HEADER_SIZE;

  /* Encode msg.  */
  nbytes = nsm_encode_auth_port_state_msg (&pnt, &size, msg);
  if (nbytes < 0)
    return nbytes;

  return nsm_client_send_message (nch, vr_id, vrf_id,
                                  NSM_MSG_AUTH_PORT_STATE, nbytes, &msg_id);
}

#ifdef HAVE_MAC_AUTH
/* Send auth-mac port state message */
s_int32_t
nsm_client_send_auth_mac_port_state (struct nsm_client *nc, u_int32_t vr_id,
                                     struct nsm_msg_auth_mac_port_state *msg)
{
  struct nsm_client_handler *nch = nc->async;
  u_int32_t vrf_id = 0;
  u_int32_t msg_id = 0;
  u_int16_t size;
  u_char *pnt;
  s_int32_t nbytes;

  /* We use async connection. */
  if (! nch || ! nch->up)
    return -1;

  /* Set pnt and size.  */
  pnt = nch->buf + NSM_MSG_HEADER_SIZE;
  size = nch->len - NSM_MSG_HEADER_SIZE;

  /* Encode msg.  */
  nbytes = nsm_encode_auth_mac_port_state_msg (&pnt, &size, msg);
  if (nbytes < 0)
    return nbytes;

  return nsm_client_send_message (nch, vr_id, vrf_id,
                                  NSM_MSG_MACAUTH_PORT_STATE, nbytes,
                                  &msg_id);
}

/* Send mac authentication status message */
s_int32_t
nsm_client_send_auth_mac_auth_status (struct nsm_client *nc, u_int32_t vr_id,
                                      struct nsm_msg_auth_mac_status *msg)
{
  struct nsm_client_handler *nch = nc->async;
  u_int32_t vrf_id = 0;
  u_int32_t msg_id = 0;
  u_int16_t size;
  u_char *pnt;
  s_int32_t nbytes;

  /* We use async connection. */
  if (! nch || ! nch->up)
    return -1;

  /* Set pnt and size.  */
  pnt = nch->buf + NSM_MSG_HEADER_SIZE;
  size = nch->len - NSM_MSG_HEADER_SIZE;

  /* Encode msg.  */
  nbytes = nsm_encode_auth_mac_status_msg (&pnt, &size, msg);
  if (nbytes < 0)
    return nbytes;

  return nsm_client_send_message (nch, vr_id, vrf_id,
                                  NSM_MSG_AUTH_MAC_AUTH_STATUS, nbytes,
                                  &msg_id);
}
#endif /* HAVE_MAC_AUTH */
#endif /* HAVE_AUTHD */

#ifdef HAVE_ONMD

s_int32_t
nsm_client_send_efm_if_msg (struct nsm_client *nc, u_int32_t vr_id,
                            struct nsm_msg_efm_if *msg,
                            enum nsm_efm_opcode opcode)
{
  struct nsm_client_handler *nch = nc->async;
  u_int32_t vrf_id = 0;
  u_int32_t msg_id = 0;
  u_int16_t size;
  u_char *pnt;
  int nbytes;

  /* We use async connection. */
  if (! nch || ! nch->up)
    return -1;

  /* Set pnt and size.  */
  pnt = nch->buf + NSM_MSG_HEADER_SIZE;
  size = nch->len - NSM_MSG_HEADER_SIZE;

  msg->opcode = opcode;

  /* Encode msg.  */
  nbytes = nsm_encode_efm_if_msg (&pnt, &size, msg);

  if (nbytes < 0)
    return nbytes;

  return nsm_client_send_message (nch, vr_id, vrf_id,
                                  NSM_MSG_EFM_OAM_IF,
                                  nbytes, &msg_id);
}

s_int32_t
nsm_client_send_lldp_msg (struct nsm_client *nc, u_int32_t vr_id,
                          struct nsm_msg_lldp *msg,
                          enum nsm_lldp_opcode opcode)
{
  struct nsm_client_handler *nch = nc->async;
  u_int32_t vrf_id = 0;
  u_int32_t msg_id = 0;
  u_int16_t size;
  u_char *pnt;
  int nbytes;

  /* We use async connection. */
  if (! nch || ! nch->up)
    return -1;

  /* Set pnt and size.  */
  pnt = nch->buf + NSM_MSG_HEADER_SIZE;
  size = nch->len - NSM_MSG_HEADER_SIZE;

  msg->opcode = opcode;

  /* Encode msg.  */
  nbytes = nsm_encode_lldp_msg (&pnt, &size, msg);

  if (nbytes < 0)
    return nbytes;

  return nsm_client_send_message (nch, vr_id, vrf_id,
                                  NSM_MSG_OAM_LLDP,
                                  nbytes, &msg_id);
}

s_int32_t
nsm_client_send_cfm_msg (struct nsm_client *nc, u_int32_t vr_id,
                         struct nsm_msg_cfm *msg,
                         enum nsm_cfm_opcode opcode)
{
  struct nsm_client_handler *nch = nc->async;
  u_int32_t vrf_id = 0;
  u_int32_t msg_id = 0;
  u_int16_t size;
  u_char *pnt;
  int nbytes;


  /* We use async connection. */
  if (! nch || ! nch->up)
    return -1;


  /* Set pnt and size.  */
  pnt = nch->buf + NSM_MSG_HEADER_SIZE;
  size = nch->len - NSM_MSG_HEADER_SIZE;

  msg->opcode = opcode;


  /* Encode msg.  */
  nbytes = nsm_encode_cfm_msg (&pnt, &size, msg);


  if (nbytes < 0)
    return nbytes;


  return nsm_client_send_message (nch, vr_id, vrf_id,
                                  NSM_MSG_OAM_CFM,
                                  nbytes, &msg_id);
}

/* Sending the UNI-MEG CFM Status to NSM*/
int
nsm_client_send_cfm_status_msg (struct nsm_client *nc, u_int32_t vr_id,
                         struct nsm_msg_cfm_status *msg)
{
  struct nsm_client_handler *nch = nc->async;
  u_int32_t vrf_id = 0;
  u_int32_t msg_id = 0;
  u_int16_t size;
  u_char *pnt;
  int nbytes;


  /* We use async connection. */
  if (! nch || ! nch->up)
    return -1;


  /* Set pnt and size.  */
  pnt = nch->buf + NSM_MSG_HEADER_SIZE;
  size = nch->len - NSM_MSG_HEADER_SIZE;

  /* Encode msg.  */
  nbytes = nsm_encode_cfm_status_msg (&pnt, &size, msg);


  if (nbytes < 0)
    return nbytes;


  return nsm_client_send_message (nch, vr_id, vrf_id,
                                  NSM_MSG_CFM_OPERATIONAL,
                                  nbytes, &msg_id);
}

#ifdef HAVE_G8031
s_int32_t
nsm_client_send_eps_msg ( struct nsm_client *nc,
                          u_int32_t vr_id,
                          struct nsm_msg_pg_initialized *msg,
                          enum nsm_eps_opcode opcode)
{
  struct nsm_client_handler *nch = nc->async;
  u_int32_t vrf_id = 0;
  u_int32_t msg_id = 0;
  u_int16_t size;
  u_char *pnt;
  int nbytes;

  /* We use async connection. */
  if (! nch || ! nch->up)
    return -1;


  /* Set pnt and size.  */
  pnt = nch->buf + NSM_MSG_HEADER_SIZE;
  size = nch->len - NSM_MSG_HEADER_SIZE;

  /* Encode msg.  */
  nbytes = nsm_encode_pg_init_msg (&pnt, &size, msg);

  if (nbytes < 0)
    return nbytes;

  return nsm_client_send_message (nch, vr_id, vrf_id,
      NSM_MSG_G8031_PG_INITIALIZED,nbytes, &msg_id);
}

s_int32_t
nsm_client_send_g8031_port_state_msg ( struct nsm_client *nc,
                                       u_int32_t vr_id,
                                       struct nsm_msg_g8031_portstate *msg)
{
    struct nsm_client_handler *nch = nc->async;
    u_int32_t vrf_id = 0;
    u_int32_t msg_id = 0;
    u_int16_t size;
    u_char *pnt;
    int nbytes;

    /* We use async connection. */
    if (! nch || ! nch->up)
      return -1;


    /* Set pnt and size.  */
    pnt = nch->buf + NSM_MSG_HEADER_SIZE;
    size = nch->len - NSM_MSG_HEADER_SIZE;

    /* Encode msg.  */
    nbytes = nsm_encode_g8031_portstate_msg (&pnt, &size, msg);

    if (nbytes < 0)
      return nbytes;

    return nsm_client_send_message (nch, vr_id, vrf_id,
        NSM_MSG_G8031_PG_PORTSTATE,nbytes, &msg_id);
}

#endif /* HAVE_G8031 */
#endif /* HAVE_ONMD */

#ifdef HAVE_LACPD
s_int32_t
nsm_client_send_lacp_aggregator_add (struct nsm_client *nc,
                                     struct nsm_msg_lacp_agg_add *agg)
{
  struct nsm_client_handler *nch = nc->async;
  u_int16_t size;
  u_char *pnt;
  int nbytes;
  u_int32_t vr_id = 0;
  vrf_id_t vrf_id = 0;
  u_int32_t msg_id = 0;


  /* We use async connection. */
  if (! nch || ! nch->up)
    return -1;

  /* Set pnt and size.  */
  pnt = nch->buf + NSM_MSG_HEADER_SIZE;
  size = nch->len - NSM_MSG_HEADER_SIZE;

  /* Encode msg.  */
  nbytes = nsm_encode_lacp_aggregator_add (&pnt, &size, agg);
  if (nbytes < 0)
    return nbytes;

  return nsm_client_send_message (nch, vr_id, vrf_id,
          NSM_MSG_LACP_AGGREGATE_ADD, nbytes, &msg_id);
}

s_int32_t
nsm_client_send_lacp_aggregator_del (struct nsm_client *nc,
                                     char *agg_name)
{
  struct nsm_client_handler *nch = nc->async;
  u_int16_t size;
  u_char *pnt;
  int nbytes;
  u_int32_t vr_id = 0;
  vrf_id_t vrf_id = 0;
  u_int32_t msg_id = 0;


  /* We use async connection. */
  if (! nch || ! nch->up)
    return -1;


  /* Set pnt and size.  */
  pnt = nch->buf + NSM_MSG_HEADER_SIZE;
  size = nch->len - NSM_MSG_HEADER_SIZE;


  /* Encode msg.  */
  nbytes = nsm_encode_lacp_aggregator_del (&pnt, &size, agg_name);
  if (nbytes < 0)
    return nbytes;

  return nsm_client_send_message (nch, vr_id, vrf_id,
          NSM_MSG_LACP_AGGREGATE_DEL, nbytes, &msg_id);
}

#endif /* HAVE_LACP */

#ifdef HAVE_L2
s_int32_t nsm_client_send_stp_message (struct nsm_client *nc,
                                 struct nsm_msg_stp *msg,
                                 int msgtype)
{
  u_char *pnt;
  u_int16_t size;
  int nbytes;
  struct nsm_client_handler *nch;
  u_int32_t msg_id;
  u_int32_t vr_id = 0;
  vrf_id_t vrf_id = 0;

  if ( !nc || !msg )
    return -1;

  nch = nc->async;

  /* We use async connection.  */
  if (! nch || ! nch->up)
    return -1;

  /* Set pnt and size.  */
  pnt = nch->buf + NSM_MSG_HEADER_SIZE;
  size = nch->len - NSM_MSG_HEADER_SIZE;

  /* Encode route clean message. */
  nbytes = nsm_encode_stp_msg (&pnt, &size, msg);
  if (nbytes < 0)
    return nbytes;

  return nsm_client_send_message (nch, vr_id, vrf_id,
                                  msgtype, nbytes, &msg_id);
}
#endif /* HAVE_L2 */

#ifdef HAVE_ONMD

s_int32_t nsm_client_send_cfm_mac_message (struct nsm_client *nc,
                                     struct nsm_msg_cfm_mac *msg,
                                     struct nsm_msg_cfm_ifindex *reply)
{
  u_char *pnt;
  u_int16_t size;
  int nbytes;
  struct nsm_client_handler *nch;
  struct nsm_msg_header header;
  struct lib_globals *zg;
  u_int32_t msg_id;
  u_int32_t vr_id = 0;
  vrf_id_t vrf_id = 0;
  int ret = 0;
  int ret_type = -1;

  if ( !nc || !msg )
    return -1;

  nch = nc->async;

  /* We use async connection.  */
  if (! nch || ! nch->up)
    return -1;

  /* Set pnt and size.  */
  pnt = nch->buf + NSM_MSG_HEADER_SIZE;
  size = nch->len - NSM_MSG_HEADER_SIZE;

  /* Encode route clean message. */
  nbytes = nsm_encode_cfm_mac_msg (&pnt, &size, msg);

  if (nbytes < 0)
    return nbytes;

  nbytes = nsm_client_send_message (nch,vr_id, vrf_id,
                                  NSM_MSG_CFM_REQ_IFINDEX, nbytes, &msg_id);

  if (nbytes < 0)
    return nbytes;
   do
    {
      /* Sync wait for reply. */
      nbytes = nsm_client_read_sync (nch->mc, NULL, nch->mc->sock, &header,
                                     &ret_type);
      if (nbytes <= 0)
        return nbytes;

      if ((ret_type != NSM_MSG_CFM_GET_IFINDEX)
          || (header.message_id != msg_id))
        nsm_client_pending_message (nch, &header);
    }
  while ((ret_type != NSM_MSG_CFM_GET_IFINDEX)
         || (header.message_id != msg_id));

 /* At here read thread may already registered as readable.
     Re-register read thread avoid hang.  */
  message_client_read_reregister (nch->mc);

  /* Set pnt and size. */
  pnt = nch->buf_in + NSM_MSG_HEADER_SIZE;
  size = nch->size_in;

  /* Decode route */
  ret = nsm_decode_cfm_index (&pnt, &size, reply);
  if (ret < 0)
    return ret;

  /* Launch event to process pending requests. */
  zg = nch->nc->zg;
  if (!zg->pend_read_thread)
    zg->pend_read_thread
      = thread_add_read_pend (zg, nsm_client_process_pending_msg, nch, 0);

  return 0;
}

#endif /* HAVE_ONMD */

#ifdef HAVE_ELMID
/* Send the ELMI operational Status to NSM */
int
nsm_client_send_elmi_status_msg (struct nsm_client *nc, u_int32_t vr_id,
                                 struct nsm_msg_elmi_status *msg)
{
  struct nsm_client_handler *nch = nc->async;
  u_int32_t vrf_id = 0;
  u_int32_t msg_id = 0;
  u_int16_t size;
  u_char *pnt;
  int nbytes;

  /* We use async connection. */
  if (! nch || ! nch->up)
    return -1;

  /* Set pnt and size.  */
  pnt = nch->buf + NSM_MSG_HEADER_SIZE;
  size = nch->len - NSM_MSG_HEADER_SIZE;

  /* Encode msg.  */
  nbytes = nsm_encode_elmi_status_msg (&pnt, &size, msg);

  if (nbytes < 0)
    return nbytes;

  return nsm_client_send_message (nch, vr_id, vrf_id,
      NSM_MSG_ELMI_OPERATIONAL_STATE,
      nbytes, &msg_id);
}

int
elmi_nsm_send_auto_vlan_port (struct nsm_client *nc,
                              struct nsm_msg_vlan_port *msg,
                              s_int32_t msg_type)
{
  s_int32_t nbytes = 0;
  u_char *pnt;
  u_int16_t size;
  u_int32_t vr_id = 0;
  vrf_id_t vrf_id = 0;
  u_int32_t msg_id = 0;
  struct nsm_client_handler *nch = NULL;

  if ( !nc || !msg )
    return -1;

  nch = nc->async;

  /* We use async connection.  */
  if (! nch || ! nch->up)
    return -1;

  /* Set pnt and size.  */
  pnt = nch->buf + NSM_MSG_HEADER_SIZE;
  size = nch->len - NSM_MSG_HEADER_SIZE;

  /* Encode Bridge Enable message. */
  nbytes = nsm_encode_elmi_vlan_msg (&pnt, &size, msg);

  if (nbytes < 0)
    return nbytes;

  return nsm_client_send_message (nch, vr_id, vrf_id,
                                  msg_type, nbytes, &msg_id);
}

#endif /* HAVE_ELMID */


#ifdef HAVE_RMOND
/* In nsm_client.c */
s_int32_t nsm_client_send_rmon_message (struct nsm_client *nc,
                                  struct nsm_msg_rmon *msg,
                                  struct nsm_msg_rmon_stats *reply)
{
  u_char *pnt;
  u_int16_t size;
  int nbytes;
  struct nsm_client_handler *nch;
  struct nsm_msg_header header;
  struct lib_globals *zg;
  u_int32_t msg_id;
  u_int32_t vr_id = 0;
  vrf_id_t vrf_id = 0;
  int ret = 0;
  int ret_type = -1;

  if ( !nc || !msg )
    return -1;

  nch = nc->async;

  /* We use async connection.  */
  if (! nch || ! nch->up)
    return -1;

  /* Do not allow sync messages on HA standby */
#ifdef HAVE_HA
  if (HA_IS_STANDBY (nc->zg))
    return 0;
#endif /* HAVE_HA */

  /* Set pnt and size.  */
  pnt = nch->buf + NSM_MSG_HEADER_SIZE;
  size = nch->len - NSM_MSG_HEADER_SIZE;

  /* Encode route clean message. */
  nbytes = nsm_encode_rmon_msg (&pnt, &size, msg);
  if (nbytes < 0)
    return nbytes;

  nbytes = nsm_client_send_message (nch,vr_id, vrf_id,
                                  NSM_MSG_RMON_REQ_STATS, nbytes, &msg_id);

  if (nbytes < 0)
    return nbytes;

   do
    {
      /* Sync wait for reply. */
      nbytes = nsm_client_read_sync (nch->mc, NULL, nch->mc->sock, &header,
                                     &ret_type);
      if (nbytes <= 0)
        return nbytes;

      if ((ret_type != NSM_MSG_RMON_SERVICE_STATS)
          || (header.message_id != msg_id))
        nsm_client_pending_message (nch, &header);
    }
  while ((ret_type != NSM_MSG_RMON_SERVICE_STATS)
         || (header.message_id != msg_id));

 /* At here read thread may already registered as readable.
     Re-register read thread avoid hang.  */
  message_client_read_reregister (nch->mc);

  pal_mem_set (reply, 0, sizeof (struct nsm_msg_rmon_stats ));

  /* Set pnt and size. */
  pnt = nch->buf_in + NSM_MSG_HEADER_SIZE;
  size = nch->size_in;

  /* Decode route */
  ret = nsm_decode_rmon_stats (&pnt, &size, reply);
  if (ret < 0)
    return ret;

  /* Launch event to process pending requests. */
  zg = nch->nc->zg;
  if (!zg->pend_read_thread)
    zg->pend_read_thread
      = thread_add_read_pend (zg, nsm_client_process_pending_msg, nch, 0);

  return 0;
}
#endif /* HAVE_RMON */

#ifdef HAVE_MPLS_FRR
s_int32_t
nsm_client_send_rsvp_control_packet (struct nsm_client *nc,
                                     struct nsm_msg_rsvp_ctrl_pkt *msg)
{
  struct nsm_client_handler *nch = nc->async;
  u_int32_t msg_id;
  u_int16_t size;
  u_char *pnt;
  int nbytes;
  u_int32_t vr_id = 0;
  vrf_id_t vrf_id = 0;

  /* We use async connection.*/
  if (! nch || ! nch->up)
    return -1;

  /* Set pnt and size.*/
  pnt = nch->buf + NSM_MSG_HEADER_SIZE;
  size = nch->len - NSM_MSG_HEADER_SIZE;

  /* Encode service.*/
  nbytes = nsm_encode_rsvp_control_packet (&pnt, &size, msg);
  if (nbytes < 0)
    return nbytes;

  /* Send it to client.*/
  return nsm_client_send_message (nch, vr_id, vrf_id,
                                  NSM_MSG_RSVP_CONTROL_PACKET,
                                  nbytes,
                                  &msg_id);
}


#endif /* HAVE_MPLS_FRR */

#ifdef HAVE_NSM_MPLS_OAM
int
nsm_client_send_mpls_onm_request (struct nsm_client *nc,
                                  struct nsm_msg_mpls_ping_req *mpls_oam)
{
  struct nsm_client_handler *nch;
  u_int32_t msg_id;
  u_int16_t size;
  u_char *pnt;
  int nbytes;
  nch = nc->async;

  if (! nch || ! nch->up)
    return -1;

  /* Set pnt and size.  */
  pnt = nch->buf + NSM_MSG_HEADER_SIZE;
  size = nch->len - NSM_MSG_HEADER_SIZE;

  /* Encode service.  */
  nbytes = nsm_encode_mpls_onm_req (&pnt, &size, mpls_oam);
  if (nbytes < 0)
    return nbytes;
  /* Send message. */
  return nsm_client_send_message (nch, 0, 0,
                                  NSM_MSG_MPLS_ECHO_REQUEST,
                                  nbytes, &msg_id);
}

s_int32_t
nsm_read_mplsonm_onm_request (struct nsm_msg_header *header,
                              void *arg, void *message)
{
  struct mpls_onm_options *mplsonm;

  mplsonm = message;
  return 0;
}
#endif /* HAVE_NSM_MPLS_OAM */

/* nsm client message to send nexthop tracking information to NSM */
s_int32_t
nsm_client_send_nexthop_tracking (u_int32_t vr_id, vrf_id_t vrf_id,
                                  struct nsm_client *nc,
                                  struct nsm_msg_nexthop_tracking *msg)
{
  struct nsm_client_handler *nch;
  u_int32_t msg_id;
  u_int16_t size;
  u_char *pnt;
  int nbytes;

  if (!nc || !msg)
    return -1;

  nch = nc->async;

  pnt = nch->buf + NSM_MSG_HEADER_SIZE;
  size = nch->len - NSM_MSG_HEADER_SIZE;

  nbytes = nsm_encode_nexthop_tracking (&pnt, &size, msg);
  if (nbytes < 0)
    return nbytes;

  return nsm_client_send_message (nch, vr_id, vrf_id,
                                  NSM_MSG_NEXTHOP_TRACKING, nbytes, &msg_id);
}

#ifdef HAVE_HA
/**********************************************************************
 *           Support for the COMMSG communication interface
 *----------------------------------------------------------------
 * nsm_parse_commsg - Parse NSM COMMSG message.
 *
 *---------------------------------------------------------------
 */
static int
_commsg_parse (u_char    **pnt,
               u_int16_t  *len,
               struct nsm_msg_header *header,
               void       *arg,
               NSM_CALLBACK callback)
{
  struct nsm_client *nc = ((struct nsm_client_handler *)arg)->nc;

  /*  Just call the COMMSG receive method - it will take care
   *  of decoding the COMMSG header and dispatch the message to the
   *  application.
   */
  nc->nc_commsg_recv_cb(nc->nc_commsg_user_ref,
                        APN_PROTO_NSM, /* The only possible source module. */
                        *pnt,
                        *len);
  return 0;
}

/* nsm_client_read_msg needs this callback to dispatch the parse message */
static int
_commsg_dummy_callback (struct nsm_msg_header *header, void *arg, void *message)
{
  return 0;
}

/*----------------------------------------------------------------
 * nsm_client_commsg_init - Register the COMMSG with the nsm_client
 *                          dispatcher.
 *                          Register the COMMSG utility receive callback.
 *  nc        - nsm_client structure pointer
 *  msg_type  - The COMMSG type
 *  recv_cb   - The COMMSG utility receive callback.
 *---------------------------------------------------------------
 */
s_int32_t
nsm_client_commsg_init(struct nsm_client *nc,
                       nsm_msg_commsg_recv_cb_t recv_cb,
                       void *user_ref)
{
  nsm_client_set_parser  (nc, NSM_MSG_COMMSG, _commsg_parse);
  nsm_client_set_callback(nc, NSM_MSG_COMMSG, _commsg_dummy_callback);

  nc->nc_commsg_recv_cb = recv_cb;
  nc->nc_commsg_user_ref= user_ref;
  return 0;
}

/*------------------------------------------------------------------
 * nsm_client_commsg_getbuf - Obtain the network buffer.
 *
 *  nsm_clt   - nsm_client pointer
 *  dst_mod_id- must be APN_PROTO_NSM
 *  size      - Requested buffer space in bytes.
 *------------------------------------------------------------------
 */
u_char *
nsm_client_commsg_getbuf(void       *nsm_clt,
                         module_id_t dst_mod_id,
                         u_int16_t   size)
{
  struct nsm_client *nc = (struct nsm_client *)nsm_clt;
  struct nsm_client_handler *nch = nc->async;

  if (! nch || ! nch->up)
    return (NULL);

  if (size > (nch->len - NSM_MSG_HEADER_SIZE)) {
    return (NULL);
  }
  return (nch->buf + NSM_MSG_HEADER_SIZE);
}

/*------------------------------------------------------------------
 * nsm_client_commsg_send - Send the COMMSG
 *
 *  nsm_clt   - nsm_client pointer
 *  dst_mod_id- must be APN_PROTO_NSM
 *  msg_buf   - Ptr to the message buffer
 *  msg_len   - Message length
 *------------------------------------------------------------------
 */
s_int32_t
nsm_client_commsg_send(void       *nsm_clt,
                       module_id_t dst_mod_id,
                       u_char     *msg_buf,
                       u_int16_t   msg_len)
{
  struct nsm_client *nc = (struct nsm_client *)nsm_clt;
  struct nsm_client_handler *nch = nc->async;

  if (! nch || ! nch->up)
    return -1;

  if (dst_mod_id == nc->zg->protocol) {
    return -1;
  }
  /* Validate the buffer pointer. */
  if (msg_buf != (nch->buf + NSM_MSG_HEADER_SIZE)) {
    return -1;
  }
  return nsm_client_send_message (nch, 0, 0, NSM_MSG_COMMSG, msg_len, 0);
}
#endif /* HAVE_HA */


#ifdef HAVE_MPLS_OAM
int
nsm_client_send_oam_lsp_ping_req_process (struct nsm_client *nc,
                                   struct nsm_msg_oam_lsp_ping_req_process *msg)
{
  struct nsm_client_handler *nch;
  u_int32_t msg_id;
  u_int16_t size;
  u_char *pnt;
  int nbytes;

  if (!nc || !msg)
    return -1;

  nch = nc->async;

  pnt = nch->buf + NSM_MSG_HEADER_SIZE;
  size = nch->len - NSM_MSG_HEADER_SIZE;

  nbytes = nsm_encode_oam_req_process (&pnt, &size, msg);
  if (nbytes < 0)
    return nbytes;

  return nsm_client_send_message (nch, 0, 0, NSM_MSG_OAM_LSP_PING_REQ_PROCESS,
                                  nbytes, &msg_id);
}

int
nsm_client_send_oam_lsp_ping_rep_process_ldp (struct nsm_client *nc,
                               struct nsm_msg_oam_lsp_ping_rep_process_ldp *msg)
{
  struct nsm_client_handler *nch;
  u_int32_t msg_id;
  u_int16_t size;
  u_char *pnt;
  int nbytes;

  if (!nc || !msg)
    return -1;

  nch = nc->async;

  pnt = nch->buf + NSM_MSG_HEADER_SIZE;
  size = nch->len - NSM_MSG_HEADER_SIZE;

  nbytes = nsm_encode_oam_rep_process_ldp (&pnt, &size, msg);
  if (nbytes < 0)
    return nbytes;

  return nsm_client_send_message (nch, 0, 0,
                                  NSM_MSG_OAM_LSP_PING_REP_PROCESS_LDP,
                                  nbytes, &msg_id);
}

int
nsm_client_send_oam_lsp_ping_rep_process_rsvp (struct nsm_client *nc,
                              struct nsm_msg_oam_lsp_ping_rep_process_rsvp *msg)
{
  struct nsm_client_handler *nch;
  u_int32_t msg_id;
  u_int16_t size;
  u_char *pnt;
  int nbytes;

  if (!nc || !msg)
    return -1;

  nch = nc->async;

  pnt = nch->buf + NSM_MSG_HEADER_SIZE;
  size = nch->len - NSM_MSG_HEADER_SIZE;

  nbytes = nsm_encode_oam_rep_process_rsvp (&pnt, &size, msg);
  if (nbytes < 0)
    return nbytes;

  return nsm_client_send_message (nch, 0, 0,
                                  NSM_MSG_OAM_LSP_PING_REP_PROCESS_RSVP,
                                  nbytes, &msg_id);
}

int
nsm_client_send_oam_lsp_ping_rep_process_gen (struct nsm_client *nc,
                               struct nsm_msg_oam_lsp_ping_rep_process_gen *msg)
{
  struct nsm_client_handler *nch;
  u_int32_t msg_id;
  u_int16_t size;
  u_char *pnt;
  int nbytes;

  if (!nc || !msg)
    return -1;

  nch = nc->async;

  pnt = nch->buf + NSM_MSG_HEADER_SIZE;
  size = nch->len - NSM_MSG_HEADER_SIZE;

  nbytes = nsm_encode_oam_rep_process_gen (&pnt, &size, msg);
  if (nbytes < 0)
    return nbytes;

  return nsm_client_send_message (nch, 0, 0,
                                  NSM_MSG_OAM_LSP_PING_REP_PROCESS_GEN,
                                  nbytes, &msg_id);
}

#ifdef HAVE_MPLS_VC
int
nsm_client_send_oam_lsp_ping_rep_process_l2vc (struct nsm_client *nc,
                              struct nsm_msg_oam_lsp_ping_rep_process_l2vc *msg)
{
  struct nsm_client_handler *nch;
  u_int32_t msg_id;
  u_int16_t size;
  u_char *pnt;
  int nbytes;

  if (!nc || !msg)
    return -1;

  nch = nc->async;

  pnt = nch->buf + NSM_MSG_HEADER_SIZE;
  size = nch->len - NSM_MSG_HEADER_SIZE;

  nbytes = nsm_encode_oam_rep_process_l2vc (&pnt, &size, msg);
  if (nbytes < 0)
    return nbytes;

  return nsm_client_send_message (nch, 0, 0,
                                  NSM_MSG_OAM_LSP_PING_REP_PROCESS_L2VC,
                                  nbytes, &msg_id);
}
#endif /* HAVE_MPLS_VC */

int
nsm_client_send_oam_lsp_ping_rep_process_l3vpn (struct nsm_client *nc,
                             struct nsm_msg_oam_lsp_ping_rep_process_l3vpn *msg)
{
  struct nsm_client_handler *nch;
  u_int32_t msg_id;
  u_int16_t size;
  u_char *pnt;
  int nbytes;

  if (!nc || !msg)
    return -1;

  nch = nc->async;

  pnt = nch->buf + NSM_MSG_HEADER_SIZE;
  size = nch->len - NSM_MSG_HEADER_SIZE;

  nbytes = nsm_encode_oam_rep_process_l3vpn (&pnt, &size, msg);
  if (nbytes < 0)
    return nbytes;

  return nsm_client_send_message (nch, 0, 0,
                                  NSM_MSG_OAM_LSP_PING_REP_PROCESS_L3VPN,
                                  nbytes, &msg_id);
}
#endif /* HAVE_MPLS_OAM */

#ifdef HAVE_GMPLS
s_int32_t
nsm_util_control_channel_callback (struct nsm_msg_header *header,
                                   void *arg, void *message)
{
  struct nsm_client_handler *nch = arg;
  struct nsm_client *nc = nch->nc;
  struct nsm_msg_control_channel *msg = message;
  struct gmpls_if *gmif;
  struct control_channel *cc = NULL;
  struct control_adj *ca = NULL;

  gmif = gmpls_if_get (nc->zg, header->vr_id);
  if (! gmif)
    return -1;

  if (nc->debug)
    nsm_control_channel_dump (nc->zg, msg);

  cc = gmpls_control_channel_lookup_by_name (gmif, msg->name);

  if (NSM_CHECK_CTYPE (msg->cindex, NSM_CONTROL_CHANNEL_CTYPE_ADD))
    {
      if (cc && cc->gifindex > 0)
        return -1;

      if (cc)
        {
          cc->gifindex = msg->gifindex;
          /* Add to cctree */
          gmpls_control_channel_add (gmif, cc);
        }
      else
        cc = gmpls_control_channel_create (gmif, msg->name, msg->gifindex);

      if (! cc)
        return -1;

      cc->bifindex = msg->bifindex;
      cc->ccid = msg->ccid;
      pal_mem_cpy (&cc->l_addr, &msg->l_addr, sizeof (struct pal_in4_addr));
      pal_mem_cpy (&cc->r_addr, &msg->r_addr, sizeof (struct pal_in4_addr));
      cc->status = msg->status;
      cc->primary = msg->primary;
      SET_FLAG (cc->flags, GMPLS_CONTROL_CHANNEL_CONFIG);
      SET_FLAG (cc->flags, GMPLS_CONTROL_CHANNEL_CONFIG_LADDR);

      if (msg->cagifindex > 0)
        {
          ca = gmpls_control_adj_lookup_by_index (gmif, msg->cagifindex);
          if (ca)
            {
              if (cc->ca != ca)
                {
                  gmpls_control_adj_add_control_channel (ca, cc, cc->primary);
                }
              cc->ca = ca;
            }
        }

      if (gmif->gif_callback[GIF_CALLBACK_GMPLS_CONTROL_CHANNEL_ADD])
        (*gmif->gif_callback[GIF_CALLBACK_GMPLS_CONTROL_CHANNEL_ADD])
          (cc, NULL);
    }
  else if (NSM_CHECK_CTYPE (msg->cindex, NSM_CONTROL_CHANNEL_CTYPE_DEL))
    {
      cc = gmpls_control_channel_lookup_by_index (gmif, msg->gifindex);
      if (! cc)
        return -1;

      if (gmif->gif_callback[GIF_CALLBACK_GMPLS_CONTROL_CHANNEL_DEL])
        (*gmif->gif_callback[GIF_CALLBACK_GMPLS_CONTROL_CHANNEL_DEL])
          (cc, NULL);

      gmpls_control_channel_delete (gmif, cc, PAL_TRUE);
    }
  else
    {
      cc = gmpls_control_channel_lookup_by_index (gmif, msg->gifindex);
      if (! cc)
        return -1;

      /* If protocol provide hook function, call it, other wise, update cc */
      if (gmif->gif_callback[GIF_CALLBACK_GMPLS_CONTROL_CHANNEL_UPDATE])
        (*gmif->gif_callback[GIF_CALLBACK_GMPLS_CONTROL_CHANNEL_UPDATE])
          (cc, msg);
      else
        gmpls_control_channel_update_by_msg (cc, msg);
    }

  return 0;
}

s_int32_t
nsm_client_send_control_channel (u_int32_t vr_id, vrf_id_t vrf_id,
                                 struct nsm_client *nc,
                                 struct nsm_msg_control_channel *msg)
{
  struct nsm_client_handler *nch = nc->async;
  u_int32_t msg_id;
  u_int16_t size;
  u_char *pnt;
  int nbytes;

  if (! nch || ! nch->up)
    return -1;

  pnt = nch->buf + NSM_MSG_HEADER_SIZE;
  size = nch->len - NSM_MSG_HEADER_SIZE;

  nbytes = nsm_encode_control_channel (&pnt, &size, msg);
  if (nbytes < 0)
    return nbytes;

  return nsm_client_send_message (nch, vr_id, vrf_id,
                                  NSM_MSG_CONTROL_CHANNEL, nbytes, &msg_id);
}

s_int32_t
nsm_util_control_adj_callback (struct nsm_msg_header *header,
                               void *arg, void *message)
{
  struct nsm_client_handler *nch = arg;
  struct nsm_client *nc = nch->nc;
  struct nsm_msg_control_adj *msg = message;
  struct gmpls_if *gmif;
  struct control_adj *ca = NULL;

  gmif = gmpls_if_get (nc->zg, header->vr_id);
  if (! gmif)
    return -1;

  if (nc->debug)
    nsm_control_adj_dump (nc->zg, msg);

  ca = gmpls_control_adj_lookup_by_name (gmif, msg->name);

  if (NSM_CHECK_CTYPE (msg->cindex, NSM_CONTROL_ADJ_CTYPE_ADD))
    {
      if (ca && ca->gifindex > 0)
        return -1;

      if (ca)
        {
          ca->gifindex = msg->gifindex;
          /* Add to catree */
          gmpls_control_adj_add (gmif, ca);
        }
      else
        ca = gmpls_control_adj_create (gmif, msg->name, msg->gifindex);

      if (! ca)
        return -1;


      pal_mem_cpy (&ca->r_nodeid, &msg->r_nodeid, sizeof (struct pal_in4_addr));
      ca->flags = msg->flags;
#ifdef HAVE_LMP
      ca->lmp_inuse = msg->lmp_inuse;
#endif
      ca->ccupcntrs = msg->ccupcntrs;
      ca->ccgifindex = msg->ccgifindex;

      /* Need msg as second argument as this carries info on CC in use for RSVP */
      if (gmif->gif_callback[GIF_CALLBACK_GMPLS_CONTROL_ADJ_ADD])
        (*gmif->gif_callback[GIF_CALLBACK_GMPLS_CONTROL_ADJ_ADD])
          (ca, msg);
    }
  else if (NSM_CHECK_CTYPE (msg->cindex, NSM_CONTROL_ADJ_CTYPE_DEL))
    {
      if (! ca)
        return -1;

      /* Need clean up control_adj's cclist and teltree */
      if (gmif->gif_callback[GIF_CALLBACK_GMPLS_CONTROL_ADJ_DEL])
        (*gmif->gif_callback[GIF_CALLBACK_GMPLS_CONTROL_ADJ_DEL])
          (ca, NULL);

      gmpls_control_adj_delete (gmif, ca);
    }
  else
    {
      if (! ca)
        return -1;

      /* If protocol provide hook function, call it, other wise, update ca */
      if (gmif->gif_callback[GIF_CALLBACK_GMPLS_CONTROL_ADJ_UPDATE])
        (*gmif->gif_callback[GIF_CALLBACK_GMPLS_CONTROL_ADJ_UPDATE])
          (ca, msg);
      else
        gmpls_control_adj_update_by_msg (ca, msg);
    }

  return 0;
}

s_int32_t
nsm_client_send_control_adj (u_int32_t vr_id, vrf_id_t vrf_id,
                             struct nsm_client *nc,
                             struct nsm_msg_control_adj *msg)
{
  struct nsm_client_handler *nch = nc->async;
  u_int32_t msg_id;
  u_int16_t size;
  u_char *pnt;
  int nbytes;

  if (! nch || ! nch->up)
    return -1;

  pnt = nch->buf + NSM_MSG_HEADER_SIZE;
  size = nch->len - NSM_MSG_HEADER_SIZE;

  nbytes = nsm_encode_control_adj (&pnt, &size, msg);
  if (nbytes < 0)
    return nbytes;

  return nsm_client_send_message (nch, vr_id, vrf_id,
                                  NSM_MSG_CONTROL_ADJ, nbytes, &msg_id);
}

s_int32_t
nsm_util_data_link_update (struct datalink *dl,
                           struct nsm_msg_data_link *msg)
{
  struct telink *tel;
  if (NSM_CHECK_CTYPE_FLAG (msg->cindex, NSM_DATALINK_CTYPE_LOCAL_LINKID))
    {
      GMPLS_LINK_ID_COPY (&dl->l_linkid, &msg->l_linkid);
    }

  if (NSM_CHECK_CTYPE_FLAG (msg->cindex, NSM_DATALINK_CTYPE_REMOTE_LINKID))
    {
      GMPLS_LINK_ID_COPY (&dl->r_linkid, &msg->r_linkid);
    }

  if (NSM_CHECK_CTYPE_FLAG (msg->cindex, NSM_DATALINK_CTYPE_TE_GIFINDEX))
    {
      /* Check if there is a change in the gifindex. */
      if ((dl->telink != NULL) && (dl->telink->gifindex != msg->te_gifindex))
        {
          /* Delete from the older TE link adjacency */
          te_link_delete_data_link (dl->telink, dl);
          dl->telink = NULL;
        }

      if (msg->te_gifindex)
        {
          tel = te_link_lookup_by_name (dl->gmif, msg->tlname);
          if (tel != NULL)
            {
              te_link_add_data_link (tel, dl);
            }
        }
    }

  if (NSM_CHECK_CTYPE_FLAG (msg->cindex, NSM_DATALINK_CTYPE_STATUS))
    {
      dl->status = msg->status;
      dl->flags = msg->flags;
    }

  if (NSM_CHECK_CTYPE_FLAG (msg->cindex, NSM_DATALINK_CTYPE_PROPERTY))
    {
      if (dl->prop != NULL)
        {
          pal_mem_cpy (dl->prop, &msg->prop, sizeof (struct link_properties));
        }
    }

  if (NSM_CHECK_CTYPE_FLAG (msg->cindex, NSM_DATALINK_CTYPE_PHY_PROPERTY))
    {
      dl->ifp->phy_prop = msg->p_prop;
    }

  return 0;
}

s_int32_t
nsm_util_data_link_callback (struct nsm_msg_header *header,
                             void *arg, void *message)
{
  struct nsm_client_handler *nch = arg;
  struct nsm_client *nc = nch->nc;
  struct nsm_msg_data_link *msg = message;
  struct gmpls_if *gmif;
  struct datalink *dl = NULL;
  struct telink *tl = NULL;
  struct interface *ifp = NULL;

  gmif = gmpls_if_get (nc->zg, header->vr_id);
  if (! gmif)
    return -1;

  /* Lookup by name will work better than lookup by index */
  dl = data_link_lookup_by_name (gmif, msg->name);

  if (NSM_CHECK_CTYPE_FLAG (msg->cindex, NSM_DATALINK_CTYPE_ADD))
    {
      ifp = if_lookup_by_index (&gmif->vr->ifm, msg->ifindex);
 
      ifp->num_dl = 1;
      dl = data_link_add (header->vr_id, nc->zg, msg->gifindex, msg->name,
                          ifp, PAL_FALSE);
      if (! dl || !dl->prop)
        return -1;

      GMPLS_LINK_ID_COPY (&dl->l_linkid, &msg->l_linkid);
      GMPLS_LINK_ID_COPY (&dl->r_linkid, &msg->r_linkid);
      dl->status = msg->status;
      dl->flags = msg->flags;

      /* Copy properties */
      pal_mem_cpy (dl->prop, &msg->prop, sizeof (struct link_properties));
      ifp->phy_prop = msg->p_prop;

      if (msg->te_gifindex)
        {
          /* Find the TE link and add to the TE link */
          tl = te_link_lookup_by_index (gmif, msg->te_gifindex);
          if (tl != NULL)
            te_link_add_data_link (tl, dl);
        }

      if (gmif->gif_callback[GIF_CALLBACK_GMPLS_DATALINK_ADD])
        (*gmif->gif_callback[GIF_CALLBACK_GMPLS_DATALINK_ADD])(dl, NULL);
    }
  else if (NSM_CHECK_CTYPE_FLAG (msg->cindex, NSM_DATALINK_CTYPE_DEL))
    {
      if (dl)
        {
          /* Need clean the pointer to physical link and TE link tree */
          /* First call the call back function hen cleanup */
          if (gmif->gif_callback[GIF_CALLBACK_GMPLS_DATALINK_DEL])
            (*gmif->gif_callback[GIF_CALLBACK_GMPLS_DATALINK_DEL]) (dl, NULL);

          /* Update library code. It will clear and delete the data link */
          data_link_delete (dl);
        }
    }
  else
    {
      if (dl)
        {
          if (gmif->gif_callback[GIF_CALLBACK_GMPLS_DATALINK_UPDATE])
            (*gmif->gif_callback[GIF_CALLBACK_GMPLS_DATALINK_UPDATE]) (dl, msg);
          else
            nsm_util_data_link_update (dl, msg);
        }
    }

  return 0;
}

s_int32_t
nsm_client_send_data_link (u_int32_t vr_id, vrf_id_t vrf_id,
                           struct nsm_client *nc,
                           struct nsm_msg_data_link *msg)
{
  struct nsm_client_handler *nch = nc->async;
  u_int32_t msg_id;
  u_int16_t size;
  u_char *pnt;
  int nbytes;

  if (! nch || ! nch->up)
    return -1;

  pnt = nch->buf + NSM_MSG_HEADER_SIZE;
  size = nch->len - NSM_MSG_HEADER_SIZE;

  nbytes = nsm_encode_data_link (&pnt, &size, msg);
  if (nbytes < 0)
    return nbytes;

  return nsm_client_send_message (nch, vr_id, vrf_id,
                                  NSM_MSG_DATA_LINK, nbytes, &msg_id);
}

s_int32_t
nsm_util_data_link_sub_update (struct datalink *dl,
                               struct nsm_msg_data_link_sub *msg)
{
  struct telink *tel;

  if (NSM_CHECK_CTYPE (msg->cindex, NSM_DATALINK_CTYPE_LOCAL_LINKID))
    {
      GMPLS_LINK_ID_COPY (&dl->l_linkid, &msg->l_linkid);
    }

  if (NSM_CHECK_CTYPE_FLAG (msg->cindex, NSM_DATALINK_CTYPE_REMOTE_LINKID))
    {
      GMPLS_LINK_ID_COPY (&dl->r_linkid, &msg->r_linkid);
    }

  if (NSM_CHECK_CTYPE_FLAG (msg->cindex, NSM_DATALINK_CTYPE_TE_GIFINDEX))
    {
      /* Check if there is a change in the gifindex. */
      if ((dl->telink != NULL) && (dl->telink->gifindex != msg->te_gifindex))
        {
          /* Delete from the older TE link adjacency */
          te_link_delete_data_link (dl->telink, dl);
          dl->telink = NULL;
        }

      if (msg->te_gifindex)
        {
          tel = te_link_lookup_by_name (dl->gmif, msg->tlname);
          if (tel != NULL)
            {
              te_link_add_data_link (tel, dl);
            }
        }
    }

  if (NSM_CHECK_CTYPE_FLAG (msg->cindex, NSM_DATALINK_CTYPE_STATUS))
    {
      dl->status = msg->status;
      dl->flags = msg->flags;
    }

  return 0;
}

s_int32_t
nsm_util_data_link_sub_callback (struct nsm_msg_header *header,
                                 void *arg, void *message)
{
  struct nsm_client_handler *nch = arg;
  struct nsm_client *nc = nch->nc;
  struct nsm_msg_data_link_sub *msg = message;
  struct gmpls_if *gmif;
  struct datalink *dl = NULL;
  struct telink *tl = NULL;
  struct interface *ifp = NULL;

  gmif = gmpls_if_get (nc->zg, header->vr_id);
  if (! gmif)
    return -1;

  /* Lookup by name will work better than lookup by index */
  dl = data_link_lookup_by_name (gmif, msg->name);

  if (NSM_CHECK_CTYPE_FLAG (msg->cindex, NSM_DATALINK_CTYPE_ADD))
    {
      ifp = if_lookup_by_index (&gmif->vr->ifm, msg->ifindex);
 
      ifp->num_dl = 1;
      dl = data_link_add (header->vr_id, nc->zg, msg->gifindex, msg->name,
                          ifp, PAL_TRUE);
      if (! dl)
        return -1;

      GMPLS_LINK_ID_COPY (&dl->l_linkid, &msg->l_linkid);
      GMPLS_LINK_ID_COPY (&dl->r_linkid, &msg->r_linkid);
      dl->status = msg->status;
      dl->flags = msg->flags;

      if (msg->te_gifindex)
        {
          /* Find the TE link and add to the TE link */
          tl = te_link_lookup_by_index (gmif, msg->te_gifindex);
          if (tl != NULL)
            te_link_add_data_link (tl, dl);
        }

      if (gmif->gif_callback[GIF_CALLBACK_GMPLS_DATALINK_ADD])
        (*gmif->gif_callback[GIF_CALLBACK_GMPLS_DATALINK_ADD])(dl, NULL);
    }
  else if (NSM_CHECK_CTYPE_FLAG (msg->cindex, NSM_DATALINK_CTYPE_DEL))
    {
      if (! dl)
        return -1;

      /* Need clean the pointer to physical link and TE link tree */
      /* First call the call back function hen cleanup */
      if (gmif->gif_callback[GIF_CALLBACK_GMPLS_DATALINK_DEL])
        (*gmif->gif_callback[GIF_CALLBACK_GMPLS_DATALINK_DEL]) (dl, NULL);

      ifp = dl->ifp;
      /* Update library code. It will clear and delete the data link */
      data_link_delete (dl);
      if (ifp)
        ifp->num_dl = 0;
    }
  else
    {
      if (! dl)
        return -1;

      /* If the update needs to be done to the datalink property in the
         library too, the function util needs to be called. */
      if (gmif->gif_callback[GIF_CALLBACK_GMPLS_DATALINK_UPDATE])
        (*gmif->gif_callback[GIF_CALLBACK_GMPLS_DATALINK_UPDATE]) (dl, msg);
      else
        nsm_util_data_link_sub_update (dl, msg);
    }

  return 0;
}

s_int32_t
nsm_client_send_data_link_sub (u_int32_t vr_id, vrf_id_t vrf_id,
                               struct nsm_client *nc,
                               struct nsm_msg_data_link_sub *msg)
{
  struct nsm_client_handler *nch = nc->async;
  u_int32_t msg_id;
  u_int16_t size;
  u_char *pnt;
  int nbytes;

  if (! nch || ! nch->up)
    return -1;

  pnt = nch->buf + NSM_MSG_HEADER_SIZE;
  size = nch->len - NSM_MSG_HEADER_SIZE;

  nbytes = nsm_encode_data_link_sub (&pnt, &size, msg);
  if (nbytes < 0)
    return nbytes;

  return nsm_client_send_message (nch, vr_id, vrf_id,
                                  NSM_MSG_DATA_LINK_SUB, nbytes, &msg_id);
}

s_int32_t
nsm_util_te_link_update (struct telink *tel,
                         struct nsm_msg_te_link *msg)
{
  struct control_adj *ca;
  s_int32_t cnt = 0;

  /* Read the link and update */
  if (NSM_CHECK_CTYPE_FLAG (msg->cindex, NSM_TELINK_CTYPE_LOCAL_LINKID))
    {
      GMPLS_LINK_ID_COPY (&tel->l_linkid, &msg->l_linkid);
    }

  if (NSM_CHECK_CTYPE_FLAG (msg->cindex, NSM_TELINK_CTYPE_REMOTE_LINKID))
    {
      GMPLS_LINK_ID_COPY (&tel->r_linkid, &msg->r_linkid);
    }

  if (NSM_CHECK_CTYPE_FLAG (msg->cindex, NSM_TELINK_CTYPE_STATUS))
    {
      tel->status = msg->status;
    }

#ifdef HAVE_DSTE
  if (NSM_CHECK_CTYPE_FLAG (msg->cindex, NSM_TELINK_CTYPE_BCMODE))
    {
      tel->prop.bc_mode = msg->mode;
    }
#endif /* HAVE_DSTE */

  if (NSM_CHECK_CTYPE_FLAG (msg->cindex, NSM_TELINK_CTYPE_FLAG))
    {
      tel->flags = msg->flags;
    }

  if (NSM_CHECK_CTYPE_FLAG (msg->cindex, NSM_TELINK_CTYPE_LINK_ID))
    {
      tel->prop.linkid = msg->linkid;
    }

  if (NSM_CHECK_CTYPE_FLAG (msg->cindex, NSM_TELINK_CTYPE_ADDR))
    {
      tel->addr = msg->addr;
    }

  if (NSM_CHECK_CTYPE_FLAG (msg->cindex, NSM_TELINK_CTYPE_REMOTE_ADDR))
    {
      tel->remote_addr = msg->remote_addr;
    }

  if (NSM_CHECK_CTYPE_FLAG (msg->cindex, NSM_TELINK_CTYPE_LINK_PROPERTY) ||
      NSM_CHECK_CTYPE_FLAG (msg->cindex, NSM_TELINK_CTYPE_TE_PROPERTY))
    {
      tel->prop.lpro = msg->l_prop;
    }

  if (NSM_CHECK_CTYPE_FLAG (msg->cindex, NSM_TELINK_CTYPE_PHY_PROPERTY) ||
      NSM_CHECK_CTYPE_FLAG (msg->cindex, NSM_TELINK_CTYPE_TE_PROPERTY))
    {
      tel->prop.phy_prop = msg->p_prop;
    }

  if (NSM_CHECK_CTYPE_FLAG (msg->cindex, NSM_TELINK_CTYPE_ADMIN_GROUP))
    {
      tel->prop.admin_group = msg->admin_group;
    }

  if (NSM_CHECK_CTYPE_FLAG (msg->cindex, NSM_TELINK_CTYPE_MTU))
    {
      tel->prop.mtu = msg->mtu;
     }

  cnt = 0;
  if (NSM_CHECK_CTYPE_FLAG (msg->cindex, NSM_TELINK_CTYPE_SRLG_UPD))
    {
      vector_reset (tel->prop.srlg);
      while (cnt < msg->srlg_num)
        {
          gmpls_srlg_add (tel->prop.srlg, msg->srlg[cnt]);
          cnt++;
        }
    }
  else
    {
      if (NSM_CHECK_CTYPE_FLAG (msg->cindex, NSM_TELINK_CTYPE_SRLG_CLR))
        {
          gmpls_srlg_delete_all (tel->prop.srlg);
        }
    }

  if (NSM_CHECK_CTYPE_FLAG (msg->cindex, NSM_TELINK_CTYPE_CAGIFINDEX))
    {
      if ((tel->control_adj == NULL) ||
          (tel->control_adj->gifindex != msg->ca_gifindex))
        {
          /* if part of a control adjacency already */
          if (tel->control_adj != NULL)
            {
              /* Remove TE link from control adjacency */
              gmpls_control_adj_del_telink (tel->control_adj, tel);
              tel->control_adj = NULL;
            }

          /* Add the TE link to a diffferent control adjacency */
          if (msg->ca_gifindex)
            {
              ca = gmpls_control_adj_lookup_by_index (tel->gmif,
                                                     msg->ca_gifindex);
              if (ca != NULL)
                {
                  gmpls_control_adj_add_telink (ca, tel);
                }
            }
        }
    }

  return 0;
}

s_int32_t
nsm_util_te_link_callback (struct nsm_msg_header *header,
                           void *arg, void *message)
{
  struct nsm_client_handler *nch = arg;
  struct nsm_client *nc = nch->nc;
  struct nsm_msg_te_link *msg = message;
  struct gmpls_if *gmif;
  struct telink *tl = NULL;
  struct control_adj *ca = NULL;
  int cnt;
  bool_t new;

  gmif = gmpls_if_get (nc->zg, header->vr_id);
  if (! gmif)
    return -1;

  if (NSM_CHECK_CTYPE_FLAG (msg->cindex, NSM_TELINK_CTYPE_ADD))
    {
      tl = te_link_add (header->vr_id, nc->zg, msg->gifindex, msg->name,
                        &new);
      if (! tl)
        return -1;

      GMPLS_LINK_ID_COPY (&tl->l_linkid, &msg->l_linkid);
      GMPLS_LINK_ID_COPY (&tl->r_linkid, &msg->r_linkid);
      tl->status = msg->status;
      prefix_copy (&tl->addr, &msg->addr);
      prefix_copy (&tl->remote_addr, &msg->remote_addr);
      tl->prop.lpro = msg->l_prop;
      tl->prop.phy_prop = msg->p_prop;
      tl->prop.admin_group = msg->admin_group;
      tl->prop.linkid = msg->linkid;
      tl->prop.mtu = msg->mtu;
      tl->flags = msg->flags;

      /* Copy SRLG */
      cnt = 0;
      while (cnt < msg->srlg_num)
        {
          gmpls_srlg_add (tl->prop.srlg, msg->srlg[cnt]);
          cnt ++;
        }

      /* Add the TE link to a diffferent control adjacency */
      if (msg->ca_gifindex)
        {
          ca = gmpls_control_adj_lookup_by_index (tl->gmif,
                                                  msg->ca_gifindex);
          if (ca != NULL)
            {
              gmpls_control_adj_add_telink (ca, tl);
            }
        }

      if (gmif->gif_callback[GIF_CALLBACK_GMPLS_TELINK_ADD])
        (*gmif->gif_callback[GIF_CALLBACK_GMPLS_TELINK_ADD])(tl, NULL);
    }
  else
    {
      /* Lookup by name will work better than lookup by index */
      tl = te_link_lookup_by_name (gmif, msg->name);
      if (! tl)
        return -1;

      if (NSM_CHECK_CTYPE_FLAG (msg->cindex, NSM_TELINK_CTYPE_DEL))
        {
          /* Need clean the pointer to physical link and TE link tree */
          /* First call the call back function hen cleanup */
          if (gmif->gif_callback[GIF_CALLBACK_GMPLS_TELINK_DEL])
            (*gmif->gif_callback[GIF_CALLBACK_GMPLS_TELINK_DEL]) (tl, NULL);

          /* Update library code. It will clear and delete the data link */
          te_link_delete (tl, PAL_TRUE);
        }
      else
        {
          /* If the update needs to be done to the datalink property in the
             library too, the function util needs to be called. */
          if (gmif->gif_callback[GIF_CALLBACK_GMPLS_TELINK_UPDATE])
            (*gmif->gif_callback[GIF_CALLBACK_GMPLS_TELINK_UPDATE]) (tl, msg);
          else
            nsm_util_te_link_update (tl, msg);
        }
    }

  return 0;
}

s_int32_t
nsm_client_send_te_link (u_int32_t vr_id, vrf_id_t vrf_id,
                         struct nsm_client *nc,
                         struct nsm_msg_te_link *msg)
{
  struct nsm_client_handler *nch = nc->async;
  u_int32_t msg_id;
  u_int16_t size;
  u_char *pnt;
  int nbytes;

  if (! nch || ! nch->up)
    return -1;

  pnt = nch->buf + NSM_MSG_HEADER_SIZE;
  size = nch->len - NSM_MSG_HEADER_SIZE;

  nbytes = nsm_encode_te_link (&pnt, &size, msg);
  if (nbytes < 0)
    return nbytes;

  return nsm_client_send_message (nch, vr_id, vrf_id,
                                  NSM_MSG_TE_LINK, nbytes, &msg_id);
}

s_int32_t
nsm_client_send_gen_ftn_add (struct nsm_client *nc,
                             struct ftn_add_gen_data *msg,
                             u_int32_t *mpls_msg_id)
{
  struct nsm_client_handler *nch = nc->async;
  u_int32_t msg_id;
  u_int16_t size;
  u_char *pnt;
  int nbytes;
  u_int32_t vr_id = 0;
  vrf_id_t vrf_id = 0;

  /* We use async connection. */
  if (! nch || ! nch->up)
    return -1;

  /* Set pnt and size.  */
  pnt = nch->buf + NSM_MSG_HEADER_SIZE;
  size = nch->len - NSM_MSG_HEADER_SIZE;

  if (++nch->mpls_msg_id == 0)
    ++nch->mpls_msg_id;

  msg->id = nch->mpls_msg_id;

  SET_FLAG (msg->flags, NSM_MSG_GEN_FTN_ADD);

  /* Encode service.  */
  nbytes = nsm_encode_gmpls_ftn_data (&pnt, &size, msg);
  if (nbytes < 0)
    return nbytes;

  if (mpls_msg_id)
    *mpls_msg_id = nch->mpls_msg_id;

  return nsm_client_send_message (nch, vr_id, vrf_id,
                                  NSM_MSG_GMPLS_FTN, nbytes, &msg_id);
}

s_int32_t
nsm_client_send_gen_ftn_add_dummy (struct nsm_client *nc,
                                   struct ftn_add_gen_data *msg)
{
  struct nsm_client_handler *nch = nc->async;
  u_int32_t msg_id;
  u_int16_t size;
  u_char *pnt;
  int nbytes;
  u_int32_t vr_id = 0;
  vrf_id_t vrf_id = 0;

  /* We use async connection. */
  if (! nch || ! nch->up)
    return -1;

  /* Set pnt and size.  */
  pnt = nch->buf + NSM_MSG_HEADER_SIZE;
  size = nch->len - NSM_MSG_HEADER_SIZE;

  SET_FLAG (msg->flags, NSM_MSG_GEN_FTN_ADD);
  SET_FLAG (msg->flags, NSM_MSG_GEN_FTN_DUMMY);

  /* Encode service.  */
  nbytes = nsm_encode_gmpls_ftn_data (&pnt, &size, msg);
  if (nbytes < 0)
    return nbytes;

  return nsm_client_send_message (nch, vr_id, vrf_id,
                                  NSM_MSG_GMPLS_FTN, nbytes, &msg_id);
}

s_int32_t
nsm_client_send_gen_ftn_fast_add (struct nsm_client *nc,
                                  struct ftn_add_gen_data *msg)
{
  struct nsm_client_handler *nch = nc->async;
  u_int32_t msg_id;
  u_int16_t size;
  u_char *pnt;
  int nbytes;
  u_int32_t vr_id = 0;
  vrf_id_t vrf_id = 0;

  /* We use async connection. */
  if (! nch || ! nch->up)
    return -1;

  /* Set pnt and size.  */
  pnt = nch->buf + NSM_MSG_HEADER_SIZE;
  size = nch->len - NSM_MSG_HEADER_SIZE;

  SET_FLAG (msg->flags, NSM_MSG_GEN_FTN_FAST_ADD);

  /* Encode service.  */
  nbytes = nsm_encode_gmpls_ftn_data (&pnt, &size, msg);
  if (nbytes < 0)
    return nbytes;

  return nsm_client_send_message (nch, vr_id, vrf_id,
                                  NSM_MSG_GMPLS_FTN, nbytes, &msg_id);
}

s_int32_t
nsm_client_send_gen_ftn_del_slow (struct nsm_client *nc,
                                  struct ftn_add_gen_data *msg)
{
  struct nsm_client_handler *nch = nc->async;
  u_int32_t msg_id;
  u_int16_t size;
  u_char *pnt;
  int nbytes;
  u_int32_t vr_id = 0;
  vrf_id_t vrf_id = 0;

  /* We use async connection. */
  if (! nch || ! nch->up)
    return -1;

  /* Set pnt and size.  */
  pnt = nch->buf + NSM_MSG_HEADER_SIZE;
  size = nch->len - NSM_MSG_HEADER_SIZE;

  /* This is delete message.  */
  msg->flags = 0;
  SET_FLAG (msg->flags, NSM_MSG_GEN_FTN_DEL);
  msg->id = 0;

  /* Encode service.  */
  nbytes = nsm_encode_gmpls_ftn_data (&pnt, &size, msg);
  if (nbytes < 0)
    return nbytes;

  return nsm_client_send_message (nch, vr_id, vrf_id,
                                  NSM_MSG_GMPLS_FTN, nbytes, &msg_id);

}

s_int32_t
nsm_client_send_gen_ftn_del (struct nsm_client *nc,
                             struct ftn_del_gen_data *fdd)
{
  struct nsm_client_handler *nch = nc->async;
  struct ftn_add_gen_data msg;
  u_int32_t msg_id;
  u_int16_t size;
  u_char *pnt;
  int nbytes = 0;
  u_int32_t vr_id = 0;
  vrf_id_t vrf_id = 0;

  /* We use async connection. */
  if (! nch || ! nch->up)
    return -1;

  /* Set pnt and size.  */
  pnt = nch->buf + NSM_MSG_HEADER_SIZE;
  size = nch->len - NSM_MSG_HEADER_SIZE;

  pal_mem_set (&msg, '\0', sizeof (struct ftn_add_gen_data));
  /* This is delete message.  */
  msg.flags = fdd->flags;
  SET_FLAG (msg.flags, NSM_MSG_GEN_FTN_FAST_DEL);

  /* Copy data to msg from fdd */
  msg.vrf_id = fdd->vrf_id;
  msg.ftn_ix = fdd->ftn_ix;
  msg.ftn = fdd->fec;

  /* Encode service.  */
  nbytes = nsm_encode_gmpls_ftn_data (&pnt, &size, &msg);
  if (nbytes < 0)
    return nbytes;

  return nsm_client_send_message (nch, vr_id, vrf_id,
                                  NSM_MSG_GMPLS_FTN, nbytes, &msg_id);
}

s_int32_t
nsm_client_send_gen_ilm_add (struct nsm_client *nc,
                             struct ilm_add_gen_data *msg,
                             u_int32_t *mpls_msg_id)
{
  struct nsm_client_handler *nch = nc->async;
  u_int32_t msg_id;
  u_int16_t size;
  u_char *pnt;
  int nbytes;
  u_int32_t vr_id = 0;
  vrf_id_t vrf_id = 0;

  /* We use async connection. */
  if (! nch || ! nch->up)
    return -1;

  /* Set pnt and size.  */
  pnt = nch->buf + NSM_MSG_HEADER_SIZE;
  size = nch->len - NSM_MSG_HEADER_SIZE;

  if (++nch->mpls_msg_id == 0)
    ++nch->mpls_msg_id;
  msg->id = nch->mpls_msg_id;

  SET_FLAG (msg->flags, NSM_MSG_ILM_ADD);
  SET_FLAG (msg->flags, NSM_MSG_ILM_GEN_MPLS);

  /* Encode service.  */
  nbytes = nsm_encode_gmpls_ilm_data (&pnt, &size, msg);
  if (nbytes < 0)
    return nbytes;

  /* Set FTN message id.  */
  if (mpls_msg_id)
    *mpls_msg_id = nch->mpls_msg_id;

  return nsm_client_send_message (nch, vr_id, vrf_id,
                                  NSM_MSG_GMPLS_ILM, nbytes, &msg_id);
}

s_int32_t
nsm_client_send_gen_ilm_dummy_add (struct nsm_client *nc,
                                   struct ilm_add_gen_data *msg)
{
  struct nsm_client_handler *nch = nc->async;
  u_int32_t msg_id;
  u_int16_t size;
  u_char *pnt;
  int nbytes;
  u_int32_t vr_id = 0;
  vrf_id_t vrf_id = 0;

  /* We use async connection. */
  if (! nch || ! nch->up)
    return -1;

  /* Set pnt and size.  */
  pnt = nch->buf + NSM_MSG_HEADER_SIZE;
  size = nch->len - NSM_MSG_HEADER_SIZE;

  msg->flags = 0;
  msg->id = 0;
  SET_FLAG (msg->flags, NSM_MSG_ILM_ADD);

  /* Encode service.  */
  nbytes = nsm_encode_gmpls_ilm_data (&pnt, &size, msg);
  if (nbytes < 0)
    return nbytes;

  return nsm_client_send_message (nch, vr_id, vrf_id,
                                  NSM_MSG_GMPLS_ILM, nbytes, &msg_id);
}

s_int32_t
nsm_client_send_gen_ilm_fast_add (struct nsm_client *nc,
                                  struct ilm_add_gen_data *msg)
{
  struct nsm_client_handler *nch = nc->async;
  u_int32_t msg_id;
  u_int16_t size;
  u_char *pnt;
  int nbytes;
  u_int32_t vr_id = 0;
  vrf_id_t vrf_id = 0;

  /* We use async connection. */
  if (! nch || ! nch->up)
    return -1;

  /* Set pnt and size.  */
  pnt = nch->buf + NSM_MSG_HEADER_SIZE;
  size = nch->len - NSM_MSG_HEADER_SIZE;

  msg->id = 0;
  SET_FLAG (msg->flags, NSM_MSG_ILM_FAST_ADD);

  /* Encode service.  */
  nbytes = nsm_encode_gmpls_ilm_data (&pnt, &size, msg);
  if (nbytes < 0)
    return nbytes;

  return nsm_client_send_message (nch, vr_id, vrf_id,
                                  NSM_MSG_GMPLS_ILM, nbytes, &msg_id);
}

s_int32_t
nsm_client_send_gen_ilm_fast_del (struct nsm_client *nc,
                                  struct ilm_del_gen_data *msg)
{
  struct nsm_client_handler *nch = nc->async;
  struct ilm_add_gen_data iad;
  u_int32_t msg_id;
  u_int16_t size;
  u_char *pnt;
  int nbytes = 0;
  u_int32_t vr_id = 0;
  vrf_id_t vrf_id = 0;

  /* We use async connection. */
  if (! nch || ! nch->up)
    return -1;

  /* Set pnt and size.  */
  pnt = nch->buf + NSM_MSG_HEADER_SIZE;
  size = nch->len - NSM_MSG_HEADER_SIZE;

  /* This is delete message.  */
  pal_mem_set (&iad, 0, sizeof (struct ilm_add_gen_data));

  iad.iif_ix = msg->iif_ix;
  iad.ilm_ix = msg->ilm_ix;
  iad.flags = NSM_MSG_ILM_FAST_DELETE;
  iad.in_label = msg->in_label;

  /* Encode service.  */
  nbytes = nsm_encode_gmpls_ilm_data (&pnt, &size, &iad);
  if (nbytes < 0)
    return nbytes;

  return nsm_client_send_message (nch, vr_id, vrf_id,
                                  NSM_MSG_GMPLS_ILM, nbytes, &msg_id);
}

s_int32_t
nsm_client_send_gen_ilm_del (struct nsm_client *nc,
                             struct ilm_add_gen_data *msg)
{
  struct nsm_client_handler *nch = nc->async;
  u_int32_t msg_id;
  u_int16_t size;
  u_char *pnt;
  int nbytes;
  u_int32_t vr_id = 0;
  vrf_id_t vrf_id = 0;

  /* We use async connection. */
  if (! nch || ! nch->up)
    return -1;

  /* Set pnt and size.  */
  pnt = nch->buf + NSM_MSG_HEADER_SIZE;
  size = nch->len - NSM_MSG_HEADER_SIZE;

  /* This is delete message.  */
  msg->flags = 0;

  SET_FLAG (msg->flags, NSM_MSG_ILM_DELETE);
  msg->id = 0;

  /* Encode service.  */
  nbytes = nsm_encode_gmpls_ilm_data (&pnt, &size, msg);
  if (nbytes < 0)
    return nbytes;

  return nsm_client_send_message (nch, vr_id, vrf_id,
                                  NSM_MSG_GMPLS_ILM, nbytes, &msg_id);
}

s_int32_t
nsm_client_send_gen_bidir_ftn_add (struct nsm_client *nc,
                                   struct ftn_bidir_add_data *msg,
                                   u_int32_t *mpls_msg_id)
{
  struct nsm_client_handler *nch = nc->async;
  u_int32_t msg_id;
  u_int16_t size;
  u_char *pnt;
  int nbytes;
  u_int32_t vr_id = 0;
  vrf_id_t vrf_id = 0;

  /* We use async connection. */
  if (! nch || ! nch->up)
    return -1;

  /* Set pnt and size.  */
  pnt = nch->buf + NSM_MSG_HEADER_SIZE;
  size = nch->len - NSM_MSG_HEADER_SIZE;

  /* This is delete message.  */
  msg->ftn.flags = 0;
  msg->ftn.id = 0;
  msg->ilm.flags = 0;
  msg->ilm.id = 0;

  if (++nch->mpls_msg_id == 0)
    ++nch->mpls_msg_id;

  msg->id = nch->mpls_msg_id;

  SET_FLAG (msg->ftn.flags, NSM_MSG_GEN_FTN_ADD);
  SET_FLAG (msg->ftn.flags, NSM_MSG_GEN_FTN_BIDIR);

  SET_FLAG (msg->ilm.flags, NSM_MSG_ILM_ADD);
  SET_FLAG (msg->ilm.flags, NSM_MSG_ILM_GEN_MPLS);
  SET_FLAG (msg->ilm.flags, NSM_MSG_ILM_BIDIR);

  /* Encode service.  */
  nbytes = nsm_encode_gmpls_ftn_bidir_data (&pnt, &size, msg);
  if (nbytes < 0)
    return nbytes;

  /* Set FTN message id.  */
  if (mpls_msg_id)
    *mpls_msg_id = nch->mpls_msg_id;

  return nsm_client_send_message (nch, vr_id, vrf_id,
                                  NSM_MSG_GMPLS_BIDIR_FTN, nbytes, &msg_id);

}

s_int32_t
nsm_client_sent_gen_bidir_ilm_add (struct nsm_client *nc,
                                   struct ilm_bidir_add_data *msg,
                                   u_int32_t *mpls_msg_id)
{
  struct nsm_client_handler *nch = nc->async;
  u_int32_t msg_id;
  u_int16_t size;
  u_char *pnt;
  int nbytes;
  u_int32_t vr_id = 0;
  vrf_id_t vrf_id = 0;

  /* We use async connection. */
  if (! nch || ! nch->up)
    return -1;

  /* Set pnt and size.  */
  pnt = nch->buf + NSM_MSG_HEADER_SIZE;
  size = nch->len - NSM_MSG_HEADER_SIZE;

  /* This is delete message.  */
  msg->ilm_fwd.flags = 0;
  msg->ilm_fwd.id = 0;
  msg->ilm_bwd.flags = 0;
  msg->ilm_bwd.id = 0;
  SET_FLAG (msg->ilm_fwd.flags, NSM_MSG_ILM_BIDIR);
  SET_FLAG (msg->ilm_fwd.flags, NSM_MSG_ILM_ADD);
  SET_FLAG (msg->ilm_fwd.flags, NSM_MSG_ILM_GEN_MPLS);
  SET_FLAG (msg->ilm_bwd.flags, NSM_MSG_ILM_BIDIR);
  SET_FLAG (msg->ilm_bwd.flags, NSM_MSG_ILM_ADD);
  SET_FLAG (msg->ilm_bwd.flags, NSM_MSG_ILM_GEN_MPLS);

  if (++nch->mpls_msg_id == 0)
    ++nch->mpls_msg_id;

  msg->id = nch->mpls_msg_id;

  /* Encode service.  */
  nbytes = nsm_encode_gmpls_ilm_bidir_data (&pnt, &size, msg);
  if (nbytes < 0)
    return nbytes;

  if (mpls_msg_id)
    *mpls_msg_id = nch->mpls_msg_id;

  return nsm_client_send_message (nch, vr_id, vrf_id,
                                  NSM_MSG_GMPLS_BIDIR_ILM, nbytes, &msg_id);
}

s_int32_t
nsm_client_send_gen_bidir_ftn_del (struct nsm_client *nc,
                                   struct ftn_bidir_del_data *msg)
{
  struct nsm_client_handler *nch = nc->async;
  struct ftn_bidir_add_data fad;
  u_int32_t msg_id;
  u_int16_t size;
  u_char *pnt;
  int nbytes = 0;
  u_int32_t vr_id = 0;
  vrf_id_t vrf_id = 0;

  /* We use async connection. */
  if (! nch || ! nch->up)
    return -1;

  /* Set pnt and size.  */
  pnt = nch->buf + NSM_MSG_HEADER_SIZE;
  size = nch->len - NSM_MSG_HEADER_SIZE;

  pal_mem_set (&fad, 0, sizeof (struct ftn_bidir_add_data));
  /* This is delete message.  */
  fad.ftn.flags = 0;
  fad.ilm.flags = 0;

  SET_FLAG (fad.ftn.flags, NSM_MSG_GEN_FTN_FAST_DEL);
  SET_FLAG (fad.ftn.flags, NSM_MSG_GEN_FTN_BIDIR);

  SET_FLAG (fad.ilm.flags, NSM_MSG_ILM_FAST_DELETE);
  SET_FLAG (fad.ilm.flags, NSM_MSG_ILM_BIDIR);

  fad.ftn.vrf_id = msg->ftn.vrf_id;
  fad.ftn.ftn_ix = msg->ftn.ftn_ix;
  fad.ftn.ftn = msg->ftn.fec;

  fad.ilm.iif_ix = msg->ilm.iif_ix;
  fad.ilm.ilm_ix = msg->ilm.ilm_ix;
  fad.ilm.in_label = msg->ilm.in_label;

  /* Encode service.  */
  nbytes = nsm_encode_gmpls_ftn_bidir_data (&pnt, &size, &fad);
  if (nbytes < 0)
    return nbytes;

  return nsm_client_send_message (nch, vr_id, vrf_id,
                                  NSM_MSG_GMPLS_BIDIR_FTN, nbytes, &msg_id);
}

s_int32_t
nsm_client_send_gen_bidir_ilm_del (struct nsm_client *nc,
                                   struct ilm_bidir_del_data *msg)
{
  struct nsm_client_handler *nch = nc->async;
  struct ilm_bidir_add_data iad;
  u_int32_t msg_id;
  u_int16_t size;
  u_char *pnt;
  int nbytes = 0;
  u_int32_t vr_id = 0;
  vrf_id_t vrf_id = 0;

  /* We use async connection. */
  if (! nch || ! nch->up)
    return -1;

  /* Set pnt and size.  */
  pnt = nch->buf + NSM_MSG_HEADER_SIZE;
  size = nch->len - NSM_MSG_HEADER_SIZE;

  /* This is delete message.  */
  iad.ilm_fwd.flags = 0;
  iad.ilm_bwd.flags = 0;

  SET_FLAG (iad.ilm_fwd.flags, NSM_MSG_ILM_FAST_DELETE);
  SET_FLAG (iad.ilm_fwd.flags, NSM_MSG_ILM_BIDIR);

  iad.ilm_fwd.iif_ix = msg->ilm_fwd.iif_ix;
  iad.ilm_fwd.ilm_ix = msg->ilm_fwd.ilm_ix;
  iad.ilm_fwd.in_label = msg->ilm_fwd.in_label;

  SET_FLAG (iad.ilm_bwd.flags, NSM_MSG_ILM_FAST_DELETE);
  SET_FLAG (iad.ilm_bwd.flags, NSM_MSG_ILM_BIDIR);

  iad.ilm_bwd.iif_ix = msg->ilm_bwd.iif_ix;
  iad.ilm_bwd.ilm_ix = msg->ilm_bwd.ilm_ix;
  iad.ilm_bwd.in_label = msg->ilm_bwd.in_label;

  /* Encode service.  */
  nbytes = nsm_encode_gmpls_ilm_bidir_data (&pnt, &size, &iad);
  if (nbytes < 0)
    return nbytes;

  return nsm_client_send_message (nch, vr_id, vrf_id,
                                  NSM_MSG_GMPLS_BIDIR_ILM, nbytes, &msg_id);
}

s_int32_t
nsm_client_send_gen_bidir_ftn_del_slow (struct nsm_client *nc,
                                        struct ftn_bidir_add_data *msg)
{
  struct nsm_client_handler *nch = nc->async;
  u_int32_t msg_id;
  u_int16_t size;
  u_char *pnt;
  int nbytes;
  u_int32_t vr_id = 0;
  vrf_id_t vrf_id = 0;

  /* We use async connection. */
  if (! nch || ! nch->up)
    return -1;

  /* Set pnt and size.  */
  pnt = nch->buf + NSM_MSG_HEADER_SIZE;
  size = nch->len - NSM_MSG_HEADER_SIZE;

  /* This is delete message.  */
  msg->ftn.flags = 0;
  msg->ilm.flags = 0;

  SET_FLAG (msg->ftn.flags, NSM_MSG_GEN_FTN_DEL);
  SET_FLAG (msg->ftn.flags, NSM_MSG_GEN_FTN_BIDIR);

  SET_FLAG (msg->ilm.flags, NSM_MSG_ILM_DELETE);
  SET_FLAG (msg->ilm.flags, NSM_MSG_ILM_BIDIR);

  /* Encode service.  */
  nbytes = nsm_encode_gmpls_ftn_bidir_data (&pnt, &size, msg);
  if (nbytes < 0)
    return nbytes;

  return nsm_client_send_message (nch, vr_id, vrf_id,
                                  NSM_MSG_GMPLS_BIDIR_FTN, nbytes, &msg_id);
}

s_int32_t
nsm_client_send_gen_bidir_ilm_del_slow (struct nsm_client *nc,
                                        struct ilm_bidir_add_data *msg)
{
  struct nsm_client_handler *nch = nc->async;
  u_int32_t msg_id;
  u_int16_t size;
  u_char *pnt;
  int nbytes;
  u_int32_t vr_id = 0;
  vrf_id_t vrf_id = 0;

  /* We use async connection. */
  if (! nch || ! nch->up)
    return -1;

  /* Set pnt and size.  */
  pnt = nch->buf + NSM_MSG_HEADER_SIZE;
  size = nch->len - NSM_MSG_HEADER_SIZE;

  /* This is delete message.  */
  msg->ilm_fwd.flags = 0;
  msg->ilm_bwd.flags = 0;

  SET_FLAG (msg->ilm_fwd.flags, NSM_MSG_ILM_DELETE);
  SET_FLAG (msg->ilm_fwd.flags, NSM_MSG_ILM_BIDIR);

  SET_FLAG (msg->ilm_bwd.flags, NSM_MSG_ILM_DELETE);
  SET_FLAG (msg->ilm_bwd.flags, NSM_MSG_ILM_BIDIR);

  /* Encode service.  */
  nbytes = nsm_encode_gmpls_ilm_bidir_data (&pnt, &size, msg);
  if (nbytes < 0)
    return nbytes;

  return nsm_client_send_message (nch, vr_id, vrf_id,
                                  NSM_MSG_GMPLS_BIDIR_ILM, nbytes, &msg_id);
}

#ifdef HAVE_LMP
int
nsm_client_send_dlink_bundle_opaque (u_int32_t vr_id, vrf_id_t vrf_id,
                                     struct nsm_client *nc,
                                     struct nsm_msg_dlink_opaque *msg,
                                     int dlink_count,
                                     struct nsm_msg_dlink_opaque *reply)
{
 return 0;
}

int
nsm_client_send_dlink_opaque (u_int32_t vr_id, vrf_id_t vrf_id,
	struct nsm_client *nc,
	struct nsm_msg_dlink_opaque *msg,
	struct nsm_msg_dlink_opaque *reply)
{
  struct nsm_client_handler *nch;
  struct nsm_msg_header header;
  struct lib_globals *zg = NULL;
  u_int16_t size = 0;
  u_int32_t msg_id = 0;
  int ret_type = -1;
  int nbytes = 0;
  u_char *pnt;
  int ret = 0;

  if ((NULL == nc) || (NULL == nc->async) || (NULL == msg))
    return -1;

  nch = nc->async;

  pnt = nch->buf + NSM_MSG_HEADER_SIZE;
  size = nch->len - NSM_MSG_HEADER_SIZE;

  nbytes = nsm_encode_dlink_opaque (&pnt, &size, msg);
  if (nbytes < 0)
	  return nbytes;

  nbytes = nsm_client_send_message (nch, vr_id, vrf_id,
		  NSM_MSG_DLINK_OPAQUE,
		  nbytes,
		  &msg_id);
  if (nbytes < 0)
	  return nbytes;

  do
  {
     nbytes = nsm_client_read_sync (nch->mc, NULL, nch->mc->sock,
			  &header, &ret_type);
     if (nbytes < 0)
       return nbytes;

     if ((ret_type!= NSM_MSG_DLINK_OPAQUE)
	||(header.message_id != msg_id))
      nsm_client_pending_message (nch, &header);
  }

  while ((ret_type != NSM_MSG_DLINK_OPAQUE) || (header.message_id != msg_id));

  message_client_read_reregister (nch->mc);

  pal_mem_set (reply, 0, sizeof (struct nsm_msg_dlink_opaque));

  pnt = nch->buf_in + NSM_MSG_HEADER_SIZE;
  size = nch->size_in;

  ret = nsm_decode_dlink_opaque (&pnt, &size, reply);
  if (ret < 0)
	  return -1;

  /* nsm_dlink_opaque_dump (nc->zg, reply); */

  zg = nch->nc->zg;
  if (!zg->pend_read_thread)
	  zg->pend_read_thread = thread_add_read_pend (zg,
			  nsm_client_process_pending_msg,
			  nch,
			  0);

  return 0;
}

int
nsm_client_send_dlink_testmsg (u_int32_t vr_id, vrf_id_t vrf_id,
	                       struct nsm_client *nc,
	                       struct nsm_msg_dlink_testmsg *msg)
{
  struct nsm_client_handler *nch;
  u_int16_t size = 0;
  u_int32_t msg_id = 0;
  int nbytes = 0;
  u_char *pnt;

  if ((NULL == nc) || (NULL == nc->async) || (NULL == msg))
    return -1;

  nch = nc->async;

  pnt = nch->buf + NSM_MSG_HEADER_SIZE;
  size = nch->len - NSM_MSG_HEADER_SIZE;

  nbytes = nsm_encode_dlink_testmsg (&pnt, &size, msg);
  if (nbytes < 0)
    return nbytes;

  nbytes = nsm_client_send_message (nch, vr_id, vrf_id,
		                    NSM_MSG_DLINK_SEND_TEST_MESSAGE,
                                    nbytes, &msg_id);
  if (nbytes < 0)
    return nbytes;

  return 0;
}
#endif /* HAVE_LMP */

s_int32_t
nsm_client_send_generic_label_msg (struct nsm_client *nc,
                          int msg_type, struct nsm_msg_generic_label_pool *msg,
                          struct nsm_msg_generic_label_pool *reply)
{
  struct nsm_client_handler *nch;
  struct nsm_msg_header header;
  struct lib_globals *zg;
  u_int32_t msg_id;
  u_int16_t size;
  u_char *pnt;
  int nbytes;
  int ret_type = -1;
  int ret;
  u_int32_t vr_id = 0;
  vrf_id_t vrf_id = 0; /* XXX-VR */

  /* Label pool service is provided as sync on the async socket. Other
   * messages from NSM are queued. */
  nch = nc->async;

  if (! nch || ! nch->up)
    return -1;

  /* Do not allow sync messages on HA standby */
#ifdef HAVE_HA
  if (HA_IS_STANDBY (nc->zg))
    return 0;
#endif /* HAVE_HA */

  /* Set pnt and size.  */
  pnt = nch->buf + NSM_MSG_HEADER_SIZE;
  size = nch->len - NSM_MSG_HEADER_SIZE;

  /* Encode service.  */
  nbytes = nsm_encode_generic_label_pool (&pnt, &size, msg);
  if (nbytes < 0)
    return nbytes;

  /* Send message. */
  nbytes = nsm_client_send_message (nch, vr_id, vrf_id,
                                    msg_type, nbytes, &msg_id);
  if (nbytes < 0)
    return nbytes;

  do
    {
      /* Sync wait for reply. */
      nbytes = nsm_client_read_sync (nch->mc, NULL, nch->mc->sock, &header,
                                     &ret_type);
      if (nbytes <= 0)
        return nbytes;

      if ((ret_type != msg_type)
          || (header.message_id != msg_id))
        nsm_client_pending_message (nch, &header);
    }
  while ((ret_type != msg_type)
         || (header.message_id != msg_id));

  /* At here read thread may already registered as readable.
     Re-register read thread avoid hang.  */
  message_client_read_reregister (nch->mc);

  /* Set pnt and size. */
  pnt = nch->buf_in + NSM_MSG_HEADER_SIZE;
  size = nch->size_in;

  /* Decode route */
  ret = nsm_decode_generic_label_pool (&pnt, &size, reply);
  if (ret < 0)
    return ret;

  /* Launch event to process pending requests. */
  zg = nch->nc->zg;
  if (!zg->pend_read_thread)
    zg->pend_read_thread
      = thread_add_read_pend (zg, nsm_client_process_pending_msg, nch, 0);

  return 0;
}


#endif /* HAVE_GMPLS */

