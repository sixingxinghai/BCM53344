/* Copyright (C) 2008  All Rights Reserved. */

/* BFD client.  */
#include "pal.h"

#if defined (HAVE_BFD) || defined (HAVE_MPLS_OAM)

#include "log.h"
#include "thread.h"
#include "network.h"
#include "message.h"
#include "bfd_message.h"
#include "bfd_client.h"
#include "bfd_client_api.h"

void
bfd_client_pending_message (struct bfd_client_handler *bch,
                            struct bfd_msg_header *header)
{
  struct bfd_client_pend_msg *pmsg;
         
  /* Queue the message for later processing. */
  pmsg = XMALLOC (MTYPE_BFD_PENDING_MSG, sizeof (struct bfd_client_pend_msg));
  if (pmsg == NULL)
    return;
  
  pmsg->header = *header;

  pal_mem_cpy (pmsg->buf, bch->pnt_in, bch->size_in);

  /* Add to pending list. */
  listnode_add (&bch->pend_msg_list, pmsg);
}

/* Generic function to send message to BFD server.  */
static int
bfd_client_send_message (struct bfd_client_handler *bch,
                         u_int32_t vr_id, vrf_id_t vrf_id,
                         int type, u_int16_t len, u_int32_t *msg_id)
{
  struct bfd_msg_header header;
  u_int16_t size;
  u_char *pnt;
  int ret;

#ifdef HAVE_HA
  /* If HA_DROP_CFG_ISSET drop the messages 
   * other than service request and reply 
   * This flag takes care of when it is standby and 
   * for active, before reading the configuration
   * ie, (HA_startup_state is CONFIG_DONE) */
  if ((HA_DROP_CFG_ISSET (bch->bc->zg))
      && (! HA_ALLOW_BFD_MSG_TYPE (type)))
    return 0;
#endif /* HAVE_HA */

  pnt = bch->buf;
  size = BFD_MESSAGE_MAX_LEN;

  /* If message ID warparounds, start from 1. */
  ++bch->message_id;
  if (bch->message_id == 0)
    bch->message_id = 1;

  /* Prepare BFD message header.  */
  header.vr_id = vr_id;
  header.vrf_id = vrf_id;
  header.type = type;
  header.length = len + BFD_MSG_HEADER_SIZE;
  header.message_id = bch->message_id;

  /* Encode header.  */
  bfd_encode_header (&pnt, &size, &header);

  /* Write message to the socket.  */
  ret = writen (bch->mc->sock, bch->buf, len + BFD_MSG_HEADER_SIZE);
  if (ret != len + BFD_MSG_HEADER_SIZE)
    return -1;

  if (msg_id)
    *msg_id = header.message_id;

  return 0;
}

/* Send bundle message to BFD server.  */
static int
bfd_client_send_message_bundle (struct bfd_client_handler *bch,
                                u_int32_t vr_id, vrf_id_t vrf_id,
                                int type, u_int16_t len)
{
  struct bfd_msg_header header;
  u_int16_t size;
  u_char *pnt;
  u_char *buf;
  int ret;

  if (type == BFD_MSG_SESSION_ADD)
    buf = bch->buf_session_add;
  else if (type == BFD_MSG_SESSION_DELETE)
    buf = bch->buf_session_delete;
  else
    buf = bch->buf;

  pnt = buf;
  size = BFD_MESSAGE_MAX_LEN;

  /* Prepare BFD message header.  */
  header.vr_id = vr_id;
  header.vrf_id = vrf_id;
  header.type = type;
  header.length = len + BFD_MSG_HEADER_SIZE;
  header.message_id = ++bch->message_id;

  /* Encode header.  */
  bfd_encode_header (&pnt, &size, &header);

  /* Write message to the socket.  */
  ret = writen (bch->mc->sock, buf, len + BFD_MSG_HEADER_SIZE);
  if (ret != len + BFD_MSG_HEADER_SIZE)
    return -1;

  return 0;
}

/* Process pending message requests. */
static int
bfd_client_process_pending_msg (struct thread *t)
{
  struct bfd_client_handler *bch;
  struct bfd_client *bc;
  struct lib_globals *zg;
  struct listnode *node;
  u_int16_t size;
  u_char *pnt;
  int ret;

  bch = THREAD_ARG (t);
  bc = bch->bc;

  /* Reset thread. */
  zg = bch->bc->zg;
  zg->pend_read_thread = NULL;

  node = LISTHEAD (&bch->pend_msg_list);
  if (node)
    {
      struct bfd_client_pend_msg *pmsg = GETDATA (node);
      int type;

      if (!pmsg)
        return 0;
        
      pnt = pmsg->buf;
      size = pmsg->header.length - BFD_MSG_HEADER_SIZE;
      type = pmsg->header.type;

      if (bc->debug)
        bfd_header_dump (zg, &pmsg->header);

      if (bc->debug)
        zlog_info (bc->zg, "Processing pending message %s",
                   bfd_msg_to_str (type));

      ret = (*bc->parser[type]) (&pnt, &size, &pmsg->header, bch,
                                 bc->callback[type]);
      if (ret < 0)
        zlog_err (bc->zg, "Parse error for message %s", bfd_msg_to_str (type));

      /* Free processed message and node. */
      XFREE (MTYPE_BFD_PENDING_MSG, pmsg);
      list_delete_node (&bch->pend_msg_list, node);
    }
  else
    return 0;
 
  node = LISTHEAD (&bch->pend_msg_list);
  if (node)
    {
      /* Process pending requests. */
      if (!zg->pend_read_thread)
        zg->pend_read_thread
          = thread_add_read_pend (zg, bfd_client_process_pending_msg, bch, 0);
    }

  return 0;
}


static int
bfd_util_service_reply (struct bfd_msg_header *header,
			void *arg, void *message)
{
  struct bfd_client_handler *bch = arg;
  struct bfd_client *bc = bch->bc;
  struct bfd_msg_service *msg = message;
  struct bfd_client_api_server s;

  if (bc->debug)
    bfd_service_dump (bc->zg, msg);

  pal_mem_set (&s, 0, sizeof (s));
  s.zg = bc->zg;

  if (bc->state == BFD_CLIENT_STATE_INIT)
    {
      if (bc->pm_callbacks.connected)
	(bc->pm_callbacks.connected) (&s);
    }
  else if (bc->state == BFD_CLIENT_STATE_DISCONNECTED)
    {
      if (bc->pm_callbacks.reconnected)
	(bc->pm_callbacks.reconnected) (&s);
    }

  /* Set the client state.  */
  bc->state = BFD_CLIENT_STATE_CONNECTED;

  /* Provide the preserve time if it's configured.  */
  if (bc->preserve_time)
    bfd_client_send_preserve (bc, bc->preserve_time);

  return 0;
}

static int
bfd_util_session_up (struct bfd_msg_header *header,
                     void *arg, void *message)
{
  struct bfd_client_handler *bch = arg;
  struct bfd_client *bc = bch->bc;
  struct bfd_msg_session *msg = message;
  struct bfd_client_api_session s;

  if (bc->debug)
    bfd_session_dump (bc->zg, msg);

  pal_mem_set (&s, 0, sizeof (struct bfd_client_api_session));
  s.zg = bc->zg;
  s.vr_id = header->vr_id;
  s.vrf_id = header->vrf_id;
  s.ifindex = msg->ifindex;
  s.flags = msg->flags;
  s.ll_type = msg->ll_type;
  s.afi = msg->afi;
  if (s.afi == AFI_IP)
    {
      IPV4_ADDR_COPY (&s.addr.ipv4.src, &msg->addr.ipv4.src);
      IPV4_ADDR_COPY (&s.addr.ipv4.dst, &msg->addr.ipv4.dst);
    }
#ifdef HAVE_IPV6
  else if (s.afi == AFI_IP6)
    {
      IPV6_ADDR_COPY (&s.addr.ipv6.src, &msg->addr.ipv6.src);
      IPV6_ADDR_COPY (&s.addr.ipv6.dst, &msg->addr.ipv6.dst);
    }
#endif /* HAVE_IPV6 */

#ifdef HAVE_MPLS_OAM
  if (s.ll_type == BFD_MSG_LL_TYPE_MPLS_LSP)
    {
      pal_mem_cpy (&s.addl_data.mpls_params, &msg->addl_data.mpls_params,
                   sizeof (struct bfd_mpls_params));
      if (bc->pm_callbacks.mpls_session_up)
        (*bc->pm_callbacks.mpls_session_up) (&s);
    }
#ifdef HAVE_VCCV
  else if (s.ll_type == BFD_MSG_LL_TYPE_MPLS_VCCV)
    {
      pal_mem_cpy (&s.addl_data.vccv_params, &msg->addl_data.vccv_params,
                   sizeof (struct bfd_vccv_params));
      if (bc->pm_callbacks.vccv_session_up)
        (*bc->pm_callbacks.vccv_session_up) (&s);
    }
#endif /* HAVE_VCCV */
  else 
#endif /* HAVE_MPLS_OAM */
    {
      if (bc->pm_callbacks.session_up)
        (*bc->pm_callbacks.session_up) (&s);
    }

  return 0;
}

#ifdef HAVE_MPLS_OAM
int
bfd_util_oam_ping_response (struct bfd_msg_header *header,
                            void *arg, void *message)
{
  struct bfd_client_handler *bch = arg;
  struct bfd_client *bc = bch->bc;

  if (bc->pm_callbacks.oam_ping_response)
    (bc->pm_callbacks.oam_ping_response) (header, arg, message);

  return 0;
}

int
bfd_util_oam_trace_response (struct bfd_msg_header *header,
                             void *arg, void *message)
{
  struct bfd_client_handler *bch = arg;
  struct bfd_client *bc = bch->bc;

  if (bc->pm_callbacks.oam_trace_response)
    (bc->pm_callbacks.oam_trace_response) (header, arg, message);

  return 0;
}
 
int
bfd_util_oam_error (struct bfd_msg_header *header,
                    void *arg, void *message)
{
  struct bfd_client_handler *bch = arg;
  struct bfd_client *bc = bch->bc;

  if (bc->pm_callbacks.oam_error)
    (bc->pm_callbacks.oam_error) (header, arg, message);

  return 0;
}
#endif /* HAVE_MPLS_OAM */


static int
bfd_util_session_down (struct bfd_msg_header *header,
                       void *arg, void *message)
{
  struct bfd_client_handler *bch = arg;
  struct bfd_client *bc = bch->bc;
  struct bfd_msg_session *msg = message;
  struct bfd_client_api_session s;

  if (bc->debug)
    bfd_session_dump (bc->zg, msg);

  pal_mem_set (&s, 0, sizeof (struct bfd_client_api_session));
  s.zg = bc->zg;
  s.vr_id = header->vr_id;
  s.vrf_id = header->vrf_id;
  s.ifindex = msg->ifindex;
  s.flags = msg->flags;
  s.ll_type = msg->ll_type;
  s.afi = msg->afi;
  if (s.afi == AFI_IP)
    {
      IPV4_ADDR_COPY (&s.addr.ipv4.src, &msg->addr.ipv4.src);
      IPV4_ADDR_COPY (&s.addr.ipv4.dst, &msg->addr.ipv4.dst);
    }
#ifdef HAVE_IPV6
  else if (s.afi == AFI_IP6)
    {
      IPV6_ADDR_COPY (&s.addr.ipv6.src, &msg->addr.ipv6.src);
      IPV6_ADDR_COPY (&s.addr.ipv6.dst, &msg->addr.ipv6.dst);
    }
#endif /* HAVE_IPV6 */

  if (CHECK_FLAG (s.flags, BFD_CLIENT_API_FLAG_AD))
    {
      if (bc->pm_callbacks.session_admin_down)
	(*bc->pm_callbacks.session_admin_down) (&s);
    }
  else /* session down */
    {
#ifdef HAVE_MPLS_OAM
      if (s.ll_type == BFD_MSG_LL_TYPE_MPLS_LSP)
        {
          pal_mem_cpy (&s.addl_data.mpls_params, &msg->addl_data.mpls_params,
                       sizeof (struct bfd_mpls_params));
          if (bc->pm_callbacks.mpls_session_down)
            (*bc->pm_callbacks.mpls_session_down) (&s);
        }
#ifdef HAVE_VCCV
      else if (s.ll_type == BFD_MSG_LL_TYPE_MPLS_VCCV)
        {
          pal_mem_cpy (&s.addl_data.vccv_params, &msg->addl_data.vccv_params,
                       sizeof (struct bfd_vccv_params));
          if (bc->pm_callbacks.vccv_session_down)
            (*bc->pm_callbacks.vccv_session_down) (&s);
        }
#endif /* HAVE_VCCV */
      else 
#endif /* HAVE_MPLS_OAM */
        {
          if (bc->pm_callbacks.session_down)
            (*bc->pm_callbacks.session_down) (&s);
        }
    }

  return 0;
}

static int
bfd_util_session_error (struct bfd_msg_header *header,
			void *arg, void *message)
{
  struct bfd_client_handler *bch = arg;
  struct bfd_client *bc = bch->bc;
  struct bfd_msg_session *msg = message;
  struct bfd_client_api_session s;

  if (bc->debug)
    bfd_session_dump (bc->zg, msg);

  if (! bc->pm_callbacks.session_error)
    return 0;

  pal_mem_set (&s, 0, sizeof (struct bfd_client_api_session));
  s.zg = bc->zg;
  s.vr_id = header->vr_id;
  s.vrf_id = header->vrf_id;
  s.ifindex = msg->ifindex;
  s.flags = msg->flags;
  s.ll_type = msg->ll_type;
  s.afi = msg->afi;
  if (s.afi == AFI_IP)
    {
      IPV4_ADDR_COPY (&s.addr.ipv4.src, &msg->addr.ipv4.src);
      IPV4_ADDR_COPY (&s.addr.ipv4.dst, &msg->addr.ipv4.dst);
    }
#ifdef HAVE_IPV6
  else if (s.afi == AFI_IP6)
    {
      IPV6_ADDR_COPY (&s.addr.ipv6.src, &msg->addr.ipv6.src);
      IPV6_ADDR_COPY (&s.addr.ipv6.dst, &msg->addr.ipv6.dst);
    }
#endif /* HAVE_IPV6 */
#ifdef HAVE_MPLS_OAM
  if (s.ll_type == BFD_MSG_LL_TYPE_MPLS_LSP)
    pal_mem_cpy (&s.addl_data.mpls_params, &msg->addl_data.mpls_params,
                  sizeof (struct bfd_mpls_params));
#ifdef HAVE_VCCV
  else if (s.ll_type == BFD_MSG_LL_TYPE_MPLS_VCCV)
    pal_mem_cpy (&s.addl_data.vccv_params, &msg->addl_data.vccv_params,
                  sizeof (struct bfd_vccv_params));
#endif /* HAVE_VCCV */
#endif /* HAVE_MPLS_OAM */

  (*bc->pm_callbacks.session_error) (&s);

  return 0;
}


/* Send service message.  */
static int
bfd_client_send_service (struct bfd_client_handler *bch)
{
  u_int32_t msg_id;
  u_int16_t size;
  u_char *pnt;
  int nbytes;
  u_int32_t vr_id = 0;
  vrf_id_t vrf_id = 0;

  if (! bch || ! bch->up)
    return -1;

  pnt = bch->buf + NSM_MSG_HEADER_SIZE;
  size = bch->len - NSM_MSG_HEADER_SIZE;

  nbytes = bfd_encode_service (&pnt, &size, &bch->service);
  if (nbytes < 0)
    return nbytes;

  return bfd_client_send_message (bch, vr_id, vrf_id,
                                  BFD_MSG_SERVICE_REQUEST, nbytes, &msg_id);
}

/* Flush the bundle message.  */
static int
bfd_client_send_session_add_flush (struct thread *t)
{
  struct bfd_client_handler *bch;
  struct bfd_client *bc;
  int nbytes;

  bc = THREAD_ARG (t);
  bch = bc->async;  
  bch->t_session_add = NULL;

  nbytes = bch->pnt_session_add - bch->buf_session_add - BFD_MSG_HEADER_SIZE;

  bfd_client_send_message_bundle (bch, bch->vr_id_session_add,
                                  bch->vrf_id_session_add,
                                  BFD_MSG_SESSION_ADD, nbytes);

  bch->pnt_session_add = NULL;
  bch->len_session_add = BFD_MESSAGE_MAX_LEN;

  return 0;
}

/* Get the interface index by the session.  */
bool_t
bfd_client_get_ifindex (struct bfd_client *bc, u_int32_t vr_id,
			struct bfd_msg_session *s)
{
  struct apn_vr *vr = apn_vr_lookup_by_id (bc->zg, vr_id);
  if (vr)
    {
      struct interface *ifp = NULL;
      if (s->afi == AFI_IP)
	ifp = if_lookup_by_ipv4_address (&vr->ifm,
					 &s->addr.ipv4.src);
#ifdef HAVE_IPV6
      else if (s->afi == AFI_IP6)
	ifp = if_lookup_by_ipv6_address (&vr->ifm,
					 &s->addr.ipv6.src);
#endif /* HAVE_IPV6 */
      if (ifp)
	{
	  s->ifindex = ifp->ifindex;
	  return PAL_TRUE;
	}
    }
  return PAL_FALSE;
}

int
bfd_client_send_session_add (struct bfd_client *bc,
                             u_int32_t vr_id, vrf_id_t vrf_id,
                             struct bfd_msg_session *msg)
{
  struct bfd_client_handler *bch = bc->async;
  u_int16_t size;
  u_char *pnt;
  int nbytes;

  /* We use async connection for session add updates.  */
  if (! bch || ! bch->up)
    return -1;

  /* We make sure the ifindex is set in the client side.  */
  if (! msg->ifindex && !(msg->flags & BFD_MSG_SESSION_FLAG_MH))
    bfd_client_get_ifindex (bc, vr_id, msg);

  /* Bulk functionality is not correct as there seem to be different
     bulking messages for Add and delete - which if reordered can result in 
     wrong behavior. BFD anyway needs the messages being sent immediately */
#ifdef ALLOW_BULK
  /* If stored data is different context, flush it first. */
  if (bch->pnt_session_add != NULL
      && (bch->vr_id_session_add != vr_id || bch->vrf_id_session_add != vrf_id))
    {
      THREAD_TIMER_OFF (bch->t_session_add);
      thread_execute (bc->zg, bfd_client_send_session_add_flush, bc, 0);
    }

  /* Set temporary pnt and size.  */
  pnt = bch->buf;
  size = bch->len;

  /* Encode service.  */
  nbytes = bfd_encode_session (&pnt, &size, msg);
  if (nbytes < 0)
    return nbytes;

  /* When bundle buffer is filled in flush it.  */
  if (nbytes >= bch->len_session_add)
    {
      THREAD_TIMER_OFF (bch->t_session_add);
      thread_execute (bc->zg, bfd_client_send_session_add_flush, bc, 0);
    }

  /* Set session add pnt and size.  */
  if (bch->pnt_session_add)
    {
      pnt = bch->pnt_session_add;
      size = bch->len_session_add;
    }
  else
    {
      pnt = bch->buf_session_add + BFD_MSG_HEADER_SIZE;
      size = bch->len_session_add - BFD_MSG_HEADER_SIZE;
    }

  /* Copy the information.  */
  pal_mem_cpy (pnt, bch->buf, nbytes);

  /* Update pointer and length.  */
  bch->pnt_session_add = pnt + nbytes;
  bch->len_session_add = size -= nbytes;

  /* Update context. */
  bch->vr_id_session_add = vr_id;
  bch->vrf_id_session_add = vrf_id;

  /* Start timer.  */
  THREAD_TIMER_ON (bc->zg, bch->t_session_add,
                   bfd_client_send_session_add_flush, bc,
                   BFD_CLIENT_BUNDLE_TIME);

#else /* Bulking not required - general BFD behavior */
  /* Set temporary pnt and size.  */
  pnt = bch->buf;
  size = bch->len;

  /* Encode service.  */
  nbytes = bfd_encode_session (&pnt, &size, msg);
  if (nbytes < 0)
    return nbytes;

    /* Set session add pnt and size.  */
  if (bch->pnt_session_add)
    {
      pnt = bch->pnt_session_add;
      size = bch->len_session_add;
    }
  else
    {
      pnt = bch->buf_session_add + BFD_MSG_HEADER_SIZE;
      size = bch->len_session_add - BFD_MSG_HEADER_SIZE;
    }

  /* Copy the information.  */
  pal_mem_cpy (pnt, bch->buf, nbytes);

  /* Update pointer and length.  */
  bch->pnt_session_add = pnt + nbytes;
  bch->len_session_add = size -= nbytes;

  /* Update context. */
  bch->vr_id_session_add = vr_id;
  bch->vrf_id_session_add = vrf_id;

  /* Send message immediately */
  thread_execute (bc->zg, bfd_client_send_session_add_flush, bc, 0);
#endif /* ALLOW_BULK */
  
  return 0;
}

/* API for flush bundle message.  */
int
bfd_client_flush_session_add (struct bfd_client *bc)
{
  struct bfd_client_handler *bch = bc->async;
  if (bch->pnt_session_add)
    {
      THREAD_TIMER_OFF (bch->t_session_add);
      thread_execute (bc->zg, bfd_client_send_session_add_flush, bc, 0);
      return 1;
    }
  return 0;
}

/* Flush the bundle message.  */
static int
bfd_client_send_session_delete_flush (struct thread *t)
{
  struct bfd_client_handler *bch;
  struct bfd_client *bc;
  int nbytes;

  bc = THREAD_ARG (t);
  bch = bc->async;  
  bch->t_session_delete = NULL;

  nbytes = bch->pnt_session_delete - bch->buf_session_delete
    - BFD_MSG_HEADER_SIZE;

  bfd_client_send_message_bundle (bch, bch->vr_id_session_delete,
                                  bch->vrf_id_session_delete,
                                  BFD_MSG_SESSION_DELETE, nbytes);

  bch->pnt_session_delete = NULL;
  bch->len_session_delete = BFD_MESSAGE_MAX_LEN;

  return 0;
}

int
bfd_client_send_session_delete (struct bfd_client *bc,
                                u_int32_t vr_id, vrf_id_t vrf_id,
                                struct bfd_msg_session *msg)
{
  struct bfd_client_handler *bch = bc->async;
  u_int16_t size;
  u_char *pnt;
  int nbytes;

  /* We use async connection for session delete updates.  */
  if (! bch || ! bch->up)
    return -1;

  /* We make sure the ifindex is set in the client side.  */
  if (! msg->ifindex && !(msg->flags & BFD_MSG_SESSION_FLAG_MH))
    bfd_client_get_ifindex (bc, vr_id, msg);

#ifdef ALLOW_BULK
  /* If stored data is different context, flush it first. */
  if (bch->pnt_session_delete != NULL
      && (bch->vr_id_session_delete != vr_id
          || bch->vrf_id_session_delete != vrf_id))
    {
      THREAD_TIMER_OFF (bch->t_session_delete);
      thread_execute (bc->zg, bfd_client_send_session_delete_flush, bc, 0);
    }
#endif /* ALLOW_BULK */

  /* Set temporary pnt and size.  */
  pnt = bch->buf;
  size = bch->len;

  /* Encode service.  */
  nbytes = bfd_encode_session (&pnt, &size, msg);
  if (nbytes < 0)
    return nbytes;

#ifdef ALLOW_BULK
  /* When bundle buffer is filled in flush it.  */
  if (nbytes >= bch->len_session_delete)
    {
      THREAD_TIMER_OFF (bch->t_session_delete);
      thread_execute (bc->zg, bfd_client_send_session_delete_flush, bc, 0);
    }
#endif /* ALLOW_BULK */

  /* Set session delete pnt and size.  */
  if (bch->pnt_session_delete)
    {
      pnt = bch->pnt_session_delete;
      size = bch->len_session_delete;
    }
  else
    {
      pnt = bch->buf_session_delete + BFD_MSG_HEADER_SIZE;
      size = bch->len_session_delete - BFD_MSG_HEADER_SIZE;
    }

  /* Copy the information.  */
  pal_mem_cpy (pnt, bch->buf, nbytes);

  /* Update pointer and length.  */
  bch->pnt_session_delete = pnt + nbytes;
  bch->len_session_delete = size -= nbytes;

  /* Update context. */
  bch->vr_id_session_delete = vr_id;
  bch->vrf_id_session_delete = vrf_id;

#ifdef ALLOW_BULK
  /* Start timer.  */
  THREAD_TIMER_ON (bc->zg, bch->t_session_delete,
                   bfd_client_send_session_delete_flush, bc,
                   BFD_CLIENT_BUNDLE_TIME);
#else /* ALLOW_BULK */
  thread_execute (bc->zg, bfd_client_send_session_delete_flush, bc, 0);
#endif /* ALLOW_BULK */

  return 0;
}

/* API for flush bundle message.  */
int
bfd_client_flush_session_delete (struct bfd_client *bc)
{
  struct bfd_client_handler *bch = bc->async;
  if (bch->pnt_session_delete)
    {
      THREAD_TIMER_OFF (bch->t_session_delete);
      thread_execute (bc->zg, bfd_client_send_session_delete_flush, bc, 0);
      return 1;
    }
  return 0;
}

static int
bfd_client_send_session_manage (struct bfd_client *bc, int msg_type,
                                struct bfd_msg_session_manage *msg)
{
  u_char *pnt;
  u_int16_t size;
  int nbytes;
  struct bfd_client_handler *bch = bc->async;
  u_int32_t msg_id;
  u_int32_t vr_id = 0;
  vrf_id_t vrf_id = 0;

  /* We use async connection for session manage updates.  */
  if (! bch || ! bch->up)
    return -1;

  /* Set pnt and size.  */
  pnt = bch->buf + BFD_MSG_HEADER_SIZE;
  size = bch->len - BFD_MSG_HEADER_SIZE;

  /* Encode service.  */
  nbytes = bfd_encode_session_manage (&pnt, &size, msg);
  if (nbytes < 0)
    return nbytes;

  return bfd_client_send_message (bch, vr_id, vrf_id,
                                  msg_type, nbytes, &msg_id);
}

int
bfd_client_send_preserve (struct bfd_client *bc, u_int32_t restart_time)
{
  struct bfd_msg_session_manage msg;

  pal_mem_set (&msg, 0, sizeof (struct bfd_msg_session_manage));
  MSG_SET_CTYPE (msg.cindex, BFD_SESSION_MANAGE_CTYPE_RESTART_TIME);
  msg.restart_time = bc->preserve_time = restart_time;

  return bfd_client_send_session_manage (bc, BFD_MSG_SESSION_PRESERVE, &msg);
}

int
bfd_client_send_stale_remove (struct bfd_client *bc)
{
  struct bfd_msg_session_manage msg;

  pal_mem_set (&msg, 0, sizeof (struct bfd_msg_session_manage));
  return bfd_client_send_session_manage (bc, BFD_MSG_SESSION_STALE_REMOVE,
                                         &msg);
}


/* Client connection is established.  Client send service description
   message to the server.  */
static int
bfd_client_connect (struct message_handler *mc, struct message_entry *me,
                    pal_sock_handle_t sock)
{
  struct bfd_client_handler *bch = mc->info;
  struct thread t;

  bch->up = 1;

  /* Send service message to BFD server.  */
  bfd_client_send_service (bch);

  /* Always read service message synchronously */
  THREAD_ARG (&t) = mc;
  THREAD_FD (&t) = mc->sock;
  message_client_read (&t);

  return 0;
}

static int
bfd_client_reconnect (struct thread *t)
{
  struct bfd_client *bc;

  bc = THREAD_ARG (t);
  bc->t_connect = NULL;
  bfd_client_start (bc);
  return 0;
}

/* Reconnect to BFD. */
static int
bfd_client_reconnect_start (struct bfd_client *bc)
{
  /* Start reconnect timer.  */
  bc->t_connect = thread_add_timer (bc->zg, bfd_client_reconnect,
                                    bc, bc->reconnect_interval);
  if (! bc->t_connect)
    return -1;

  return 0;
}

static int
bfd_client_disconnect (struct message_handler *mc, struct message_entry *me,
                       pal_sock_handle_t sock)
{
  struct bfd_client_handler *bch;
  struct bfd_client *bc;
  struct listnode *node, *next;

  bch = mc->info;
  bc = bch->bc;

  /* Set status to down.  */
  bch->up = 0;

  /* Cancel pending read thread. */
  if (bc->zg->pend_read_thread)
    THREAD_OFF (bc->zg->pend_read_thread);

  /* Free all pending reads. */
  for (node = LISTHEAD (&bch->pend_msg_list); node; node = next)
    {
      struct bfd_client_pend_msg *pmsg = GETDATA (node);

      next = node->next;

      XFREE (MTYPE_BFD_PENDING_MSG, pmsg);
      list_delete_node (&bch->pend_msg_list, node);
    }

  /* Stop async connection.  */
  if (bc->async)
    message_client_stop (bc->async->mc);

  /* Call client specific disconnect handler. */
  if (bc->pm_callbacks.disconnected)
    {
      struct bfd_client_api_server s;
      pal_mem_set (&s, 0, sizeof (s));
      s.zg = bc->zg;
      (bc->pm_callbacks.disconnected) (&s);
    }

  /* Set the client state.  */
  bc->state = BFD_CLIENT_STATE_DISCONNECTED;

  bfd_client_reconnect_start (bc);

  return 0;
}

static int
bfd_client_read (struct bfd_client_handler *bch, pal_sock_handle_t sock)
{
  struct bfd_msg_header *header;
  u_int16_t length;
  int nbytes = 0;

  bch->size_in = 0;
  bch->pnt_in = bch->buf_in;

  nbytes = readn (sock, bch->buf_in, BFD_MSG_HEADER_SIZE);
  if (nbytes < BFD_MSG_HEADER_SIZE)
    return -1;

  header = (struct bfd_msg_header *) bch->buf_in;
  length = pal_ntoh16 (header->length);

  nbytes = readn (sock, bch->buf_in + BFD_MSG_HEADER_SIZE,
                  length - BFD_MSG_HEADER_SIZE);
  if (nbytes <= 0)
    return nbytes;

  bch->size_in = length;

  return length;
}

/* Read BFD message body.  */
static int
bfd_client_read_msg (struct message_handler *mc,
                     struct message_entry *me,
                     pal_sock_handle_t sock)
{
  struct bfd_client_handler *bch;
  struct bfd_client *bc;
  struct bfd_msg_header header;
  struct apn_vr *vr;
  int nbytes;
  int type;
  int ret;

  /* Get BFD client handler from message entry. */
  bch = mc->info;
  bc = bch->bc;

  /* Read data. */
  nbytes = bfd_client_read (bch, sock);
  if (nbytes <= 0)
    return nbytes;

  /* Parse BFD message header. */
  ret = bfd_decode_header (&bch->pnt_in, &bch->size_in, &header);
  if (ret < 0)
    return -1;

#ifdef HAVE_HA
  /* During starup config, do not process messages from BFD
   * till the config_done state, as for simplex-active, 
   * configuration is downloaded from check point server */
  if ((HA_DROP_CFG_ISSET (bc->zg))
      && (! HA_ALLOW_BFD_MSG_TYPE (header.type)))
    return 0;
#endif /* HAVE_HA */

  /* Dump BFD header. */
  if (bc->debug)
    bfd_header_dump (mc->zg, &header);

  /* Set the VR Context before invoking the Client Callback */
  vr = apn_vr_lookup_by_id (bc->zg, header.vr_id);
  if (! vr)
    {
      /* If a message is received for a non existing VR, just ignore the
         message. Send the +ve number of bytes to not cause a disconnection. */
      return nbytes; 
    }

  type = header.type;

  /* Invoke parser with call back function pointer.  There is no callback
     set by protocols for MPLS replies. */
  if (type < BFD_MSG_MAX && bc->parser[type] && bc->callback[type])
    {
      ret = (*bc->parser[type]) (&bch->pnt_in, &bch->size_in, &header, bch,
                                 bc->callback[type]);
      if (ret < 0)
        zlog_err (bc->zg, "Parse error for message %s", bfd_msg_to_str (type));
    }

  return nbytes;
}

static struct bfd_client_handler *
bfd_client_handler_new (struct bfd_client *bc, int type)
{
  struct bfd_client_handler *bch;
  struct message_handler *mc;

  /* Allocate BFD client handler.  */
  bch = XCALLOC (MTYPE_BFD_CLIENT_HANDLER, sizeof (struct bfd_client_handler));
  bch->type = type;
  bch->bc = bc;

  /* Set max message length.  */
  bch->len = BFD_MESSAGE_MAX_LEN;
  bch->len_in = BFD_MESSAGE_MAX_LEN;
  bch->len_session_add = BFD_MESSAGE_MAX_LEN;
  bch->len_session_delete = BFD_MESSAGE_MAX_LEN;

  /* Create async message client. */
  mc = message_client_create (bc->zg, type);

#ifndef HAVE_TCP_MESSAGE
  /* Use UNIX domain socket connection.  */
  message_client_set_style_domain (mc, BFD_SERV_PATH);
#else /* HAVE_TCP_MESSAGE */
  message_client_set_style_tcp (mc, BFD_PORT);
#endif /* !HAVE_TCP_MESSAGE */

  /* Initiate connection using BFD connection manager.  */
  message_client_set_callback (mc, MESSAGE_EVENT_CONNECT,
                               bfd_client_connect);
  message_client_set_callback (mc, MESSAGE_EVENT_DISCONNECT,
                               bfd_client_disconnect);
  message_client_set_callback (mc, MESSAGE_EVENT_READ_MESSAGE,
                               bfd_client_read_msg);

  /* Link each other.  */
  bch->mc = mc;
  mc->info = bch;

  bch->pnt = bch->buf;
  bch->pnt_in = bch->buf_in;

  return bch;
}

/* Set packet parser.  */
static void
bfd_client_set_parser (struct bfd_client *bc, int message_type,
                       BFD_PARSER parser)
{
  bc->parser[message_type] = parser;
}

/* Initialize BFD client.  This function allocate BFD client
   memory.  */
struct bfd_client *
bfd_client_new (struct lib_globals *zg,
                struct bfd_client_api_callbacks *cb)
{
  struct bfd_client *bc;

  bc = XCALLOC (MTYPE_BFD_CLIENT, sizeof (struct bfd_client));
  if (! bc)
    return NULL;
  bc->zg = zg;
  zg->bc = bc;

  /* Set the module ID.  */
  bc->service.module_id = zg->protocol;

  /* Set parsers. */
  bfd_client_set_parser (bc, BFD_MSG_SERVICE_REPLY, bfd_parse_service);
  bfd_client_set_parser (bc, BFD_MSG_SESSION_UP, bfd_parse_session);
  bfd_client_set_parser (bc, BFD_MSG_SESSION_DOWN, bfd_parse_session);
  bfd_client_set_parser (bc, BFD_MSG_SESSION_ERROR, bfd_parse_session);

  /* Set callbacks. */
  bfd_client_set_callback (bc, BFD_MSG_SERVICE_REPLY, bfd_util_service_reply);
  bfd_client_set_callback (bc, BFD_MSG_SESSION_UP, bfd_util_session_up);
  bfd_client_set_callback (bc, BFD_MSG_SESSION_DOWN, bfd_util_session_down);
  bfd_client_set_callback (bc, BFD_MSG_SESSION_ERROR, bfd_util_session_error);

  bc->reconnect_interval = BFD_CLIENT_RECONNECT_INTERVAL;
  if (cb)
    bc->pm_callbacks = *cb;
  bc->debug = PAL_FALSE;

  return bc;
}

#ifdef HAVE_MPLS_OAM
/* Initialize BFD client.  This function allocate BFD client
   memory.  */
struct bfd_client *
bfd_oam_client_new (struct lib_globals *zg,
                    struct bfd_client_api_callbacks *cb)
{
  struct bfd_client *bc;

  bc = XCALLOC (MTYPE_BFD_CLIENT, sizeof (struct bfd_client));
  if (! bc)
    return NULL;
  bc->zg = zg;
  zg->bc = bc;

  /* Set the module ID.  */
  bc->service.module_id = zg->protocol;

  /* Set parsers. */
  bfd_client_set_parser (bc, OAM_MSG_MPLS_PING_REPLY, 
                         oam_parse_ping_response);
  bfd_client_set_parser (bc, OAM_MSG_MPLS_TRACERT_REPLY,
                         oam_parse_tracert_response);
  bfd_client_set_parser (bc, OAM_MSG_MPLS_OAM_ERROR,
                         oam_parse_error_response);

  /* Set callbacks. */
  bfd_client_set_callback (bc, OAM_MSG_MPLS_PING_REPLY , 
                           bfd_util_oam_ping_response);
  bfd_client_set_callback (bc, OAM_MSG_MPLS_TRACERT_REPLY, 
                           bfd_util_oam_trace_response);
  bfd_client_set_callback (bc, OAM_MSG_MPLS_OAM_ERROR, 
                           bfd_util_oam_error);

  bc->reconnect_interval = BFD_CLIENT_RECONNECT_INTERVAL;
  if (cb)
    bc->pm_callbacks = *cb;
  bc->debug = PAL_FALSE;

  return bc;
}
#endif /* HAVE_MPLS_OAM */

static void
bfd_client_handler_free (struct bfd_client_handler *bch)
{
  THREAD_TIMER_OFF (bch->t_session_add);
  THREAD_TIMER_OFF (bch->t_session_delete);
  if (bch->mc)
    message_client_delete (bch->mc);
  XFREE (MTYPE_BFD_CLIENT_HANDLER, bch);
}

int
bfd_client_delete (struct bfd_client *bc)
{
  struct listnode *node, *node_next;
  struct bfd_client_handler *bch;

  if (bc)
    {
      if (bc->t_connect)
        THREAD_OFF (bc->t_connect);

      /* Cancel pending read thread. */
      if (bc->zg->pend_read_thread)
        THREAD_OFF (bc->zg->pend_read_thread);

      bch = bc->async;
      /* Free all pending reads. */
      if (bch)
        {
          for (node = LISTHEAD (&bch->pend_msg_list); node; node = node_next)
            {
              struct bfd_client_pend_msg *pmsg = GETDATA (node);

              node_next = node->next;

              XFREE (MTYPE_BFD_PENDING_MSG, pmsg);
              list_delete_node (&bch->pend_msg_list, node);
            }

          bfd_client_handler_free (bch);
        }

      XFREE (MTYPE_BFD_CLIENT, bc);
    }
  return 0;
}

/* Start to connect BFD services.  This function always return success. */
int
bfd_client_start (struct bfd_client *bc)
{
  int ret;

  if (bc->async)
    {
      ret = message_client_start (bc->async->mc);
      if (ret < 0)
        {
          /* Start reconnect timer.  */
          if (bc->t_connect == NULL)
            {
              bc->t_connect
                = thread_add_timer (bc->zg, bfd_client_reconnect,
                                    bc, bc->reconnect_interval);
              if (bc->t_connect == NULL)
                return -1;
            }
        }
    }
  return 0;
}

/* Stop BFD client. */
void
bfd_client_stop (struct bfd_client *bc)
{
  if (bc->async)
    message_client_stop (bc->async->mc);
}

/* Set service type flag.  */
void
bfd_client_set_service (struct bfd_client *bc, int service)
{
  if (service >= BFD_SERVICE_MAX)
    return;

  /* Set service bit to BFD client.  */
  MSG_SET_CTYPE (bc->service.bits, service);

  /* Create client hander corresponding to message type.  */
  if (! bc->async)
    {
      bc->async = bfd_client_handler_new (bc, MESSAGE_TYPE_ASYNC);
      bc->async->service.version = bc->service.version;
      bc->async->service.module_id = bc->service.module_id;
    }
  MSG_SET_CTYPE (bc->async->service.bits, service);
}

void
bfd_client_set_version (struct bfd_client *bc, u_int16_t version)
{
  bc->service.version = version;
}

/* Register callback.  */
void
bfd_client_set_callback (struct bfd_client *bc, int message_type,
                         BFD_CALLBACK callback)
{
  bc->callback[message_type] = callback;
}

#ifdef HAVE_MPLS_OAM
int
bfd_client_send_mpls_onm_request (struct bfd_client *bc,
                                  struct oam_msg_mpls_ping_req *mpls_oam)
{
  struct bfd_client_handler *bch;
  u_int32_t msg_id;
  u_int16_t size;
  u_char *pnt;
  int nbytes;
  bch = bc->async;

  if (! bch || ! bch->up)
    return -1;

  /* Set pnt and size.  */
  pnt = bch->buf + BFD_MSG_HEADER_SIZE;
  size = bch->len - BFD_MSG_HEADER_SIZE;

  /* Encode service.  */
  nbytes = oam_encode_mpls_onm_req (&pnt, &size, mpls_oam);
  if (nbytes < 0)
    return nbytes;

  /* Send message. */
  return bfd_client_send_message (bch, 0, 0,
                                  OAM_MSG_MPLS_ECHO_REQUEST,
                                  nbytes, &msg_id);
}
#endif /* HAVE_MPLS_OAM */
#endif /* HAVE_BFD || HAVE_MPLS_OAM */
