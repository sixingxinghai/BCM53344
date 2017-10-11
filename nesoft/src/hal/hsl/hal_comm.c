/* Copyright (C) 2004  All Rights Reserved. */

#include "pal.h"
#include "lib.h"
#include "thread.h"

#include "hal_incl.h"
#include "hal_netlink.h"
#include "hal_comm.h"
#include "hal_msg.h"
#include "hal_debug.h"
#include "hal_if.h"

#ifdef HAVE_USER_HSL
#include "network.h"
#include "message.h"
#endif /* HAVE_USER_HSL */

#ifdef HAVE_ONMD

int
hal_efm_dying_gasp(struct hal_nlmsghdr *h, void *data);

int
hal_efm_frame_period_window_expiry (struct hal_nlmsghdr *h, void *data);

#endif /* HAVE_ONMD */

#ifndef HAL_TCP_COMM

extern struct lib_globals *hal_zg;
extern int hal_if_newlink (struct hal_nlmsghdr *h, void *data);
extern int hal_if_dellink (struct hal_nlmsghdr *h, void *data);
#ifdef HAVE_L2
extern int hal_if_stp_refresh(struct hal_nlmsghdr *h, void *data);
#endif /* HAVE_L2 */
#ifdef HAVE_L3
extern int hal_rx_max_multipath(struct hal_nlmsghdr *h, void *data);
extern int hal_if_ipv4_addr(struct hal_nlmsghdr *h, void *data);
#ifdef HAVE_IPV6
extern int hal_if_ipv6_addr(struct hal_nlmsghdr *h, void *data);
#endif /* HAVE_IPV6 */
#endif 

#ifdef HAVE_USER_HSL
/*
   Asynchronous messages.
*/
struct hal_client hallink_async      = { NULL, 0, "hallink-listen" };

/*
   Command channel.
*/
struct hal_client hallink_cmd  = { NULL, 0, "hallink-cmd" };

#else

/* 
   Asynchronous messages. 
*/
struct halsock hallink      = { -1, 0, {0}, "hallink-listen" };

/* 
   Command channel. 
*/
struct halsock hallink_cmd  = { -1, 0, {0}, "hallink-cmd" };

/* 
   if-arbiter command channel.
*/
struct halsock hallink_poll = { -1, 0, {0}, "hallink-poll" };

#endif /* HAVE_USER_HSL */

int hal_comm_initialized = 0;

static int
hal_recv_cb (struct hal_nlmsghdr *h, void *data)
{
  struct hal_nlmsgerr *err = (struct hal_nlmsgerr *) HAL_NLMSG_DATA (h);

  if (IS_HAL_DEBUG_EVENT)
    zlog_warn (hal_zg, "hal_recv_cb: ignoring message type 0x%04x",
               h->nlmsg_type);

  if (err)
    return err->error;
  else
    return 0;
}

/* 
   Read thread callback. 
*/
static int
_hal_callbacks (struct hal_nlmsghdr *h, void *data)
{
  switch (h->nlmsg_type)
    {
    case HAL_MSG_IF_NEWLINK:
    case HAL_MSG_IF_UPDATE:
      return hal_if_newlink (h, NULL);
    case HAL_MSG_IF_DELLINK:
      return hal_if_dellink (h, NULL);
#ifdef HAVE_L2
    case HAL_MSG_IF_STP_REFRESH:
      return hal_if_stp_refresh(h, NULL);
#endif /* HAVE_L2 */
#ifdef HAVE_L3
    case HAL_MSG_IF_IPV4_NEWADDR:
    case HAL_MSG_IF_IPV4_DELADDR:
       return hal_if_ipv4_addr(h, NULL);
    case HAL_MSG_GET_MAX_MULTIPATH:
       return hal_rx_max_multipath(h, NULL);
#ifdef HAVE_IPV6
    case HAL_MSG_IF_IPV6_NEWADDR:
    case HAL_MSG_IF_IPV6_DELADDR:
       return hal_if_ipv6_addr(h, NULL);
#endif /* HAVE_IPV6 */
#endif /* HAVE_L3   */
#ifdef HAVE_EFM
    case HAL_MSG_EFM_DYING_GASP:
      return hal_efm_dying_gasp(h, NULL);
    case HAL_MSG_EFM_FRAME_PERIOD_WINDOW_EXPIRY:
      return hal_efm_frame_period_window_expiry(h, NULL);
#endif/* HAVE_EFM */
    }
  return 0;
}

/*
  Send HAL generic  message to HSL.
*/
int
hal_msg_generic_request (int msg, int (*filter) (struct hal_nlmsghdr *, void *),
                         void *data)
{
  int ret;
  struct hal_nlmsghdr *nlh;
  struct 
  {
    struct hal_nlmsghdr nlh;
  } req;

  memset (&req.nlh, 0, sizeof (struct hal_nlmsghdr));
  
  /* Set header. */
  nlh = &req.nlh;
  nlh->nlmsg_len = HAL_NLMSG_LENGTH (0);
  nlh->nlmsg_flags = HAL_NLM_F_CREATE | HAL_NLM_F_REQUEST;
  nlh->nlmsg_type = msg;
  nlh->nlmsg_seq = ++hallink_cmd.seq;

  /* Request list of interfaces. */
  ret = hal_talk (&hallink_cmd, nlh, filter, data);
  if (ret < 0)
    return ret;

  return HAL_SUCCESS;
}

#ifdef HAVE_USER_HSL
/*
   Receive message from netlink(RFC 3549) interface and pass those information
   to the given function.
*/
int
hal_parse_info (struct hal_client *nl,
                int (*filter) (struct hal_nlmsghdr *, void *),
                void *data)
{
  int status;
  int ret = 0;
  u_char *pnt;
  u_int32_t len;
  int error;

  while (1)
    {
      u_char buf[4096];
      struct hal_nlmsghdr *h;

      /* Read header */
      status = readn (nl->mc->sock, buf, HAL_NLMSGHDR_SIZE);

      if (status < 0)
        {
          if (IS_HAL_DEBUG_EVENT)
            zlog_err (hal_zg, "%s recvmsg overrun: %s", nl->name,
                pal_strerror (errno));
          continue;
        }
  
      if (status == 0)
        {
          if (IS_HAL_DEBUG_EVENT)
            zlog_err (hal_zg, "%s EOF", nl->name);
          return -1;
        }

      /* Check header length.  If length is smaller than HAL_NLMSGHDR_SIZE
       * message header size close the connection.
       */
      if (status != HAL_NLMSGHDR_SIZE)
        return -1;

      h = (struct hal_nlmsghdr *) buf;
      pnt = buf + HAL_NLMSGHDR_SIZE;
      len = HAL_NLMSG_ALIGN (h->nlmsg_len) - HAL_NLMSGHDR_SIZE;

      if (len != 0)
        {
          /* Read message body.  */
          status = readn (nl->mc->sock, pnt, len);

          if (status < 0)
            {
              if (IS_HAL_DEBUG_EVENT)
              if (IS_HAL_DEBUG_EVENT)
                zlog_err (hal_zg, "%s recvmsg overrun: %s", nl->name,
                    pal_strerror (errno));
              continue;
            }

          if (status == 0)
            {
              if (IS_HAL_DEBUG_EVENT)
                zlog_err (hal_zg, "%s EOF", nl->name);
              return -1;
            }
        } 

      /* Finish of reading. */
      if (h->nlmsg_type == HAL_NLMSG_DONE)
        {
          return ret;
        }

      /* Error handling. */
      if (h->nlmsg_type == HAL_NLMSG_ERROR)
        {
          struct hal_nlmsgerr *err = (struct hal_nlmsgerr *) HAL_NLMSG_DATA (h);
          if (h->nlmsg_len < HAL_NLMSG_LENGTH (sizeof (struct hal_nlmsgerr)))
            {
              if (IS_HAL_DEBUG_EVENT)
                zlog_err (hal_zg, "%s error: message truncated", nl->name);
              return -1;
            }

          if (IS_HAL_DEBUG_EVENT)
            zlog_err (hal_zg, "%s error: %s, type=%u, seq=%u, pid=%d",
                nl->name, pal_strerror (-err->error),
                err->msg.nlmsg_type, err->msg.nlmsg_seq,
                err->msg.nlmsg_pid);
          return err->error;
        }

      /* OK we got netlink(RFC 3549) message. */
      if (IS_HAL_DEBUG_EVENT)
        zlog_info (hal_zg,
            "hal_parse_info: %s type %u, seq=%u, pid=%d",
            nl->name, h->nlmsg_type,
            h->nlmsg_seq, h->nlmsg_pid);

      error = (*filter) (h, data);
      if (error < 0)
        {
          if (IS_HAL_DEBUG_EVENT)
            zlog_err (hal_zg, "%s filter function error", nl->name);
          ret = error;
        }

    }
  return ret;
}

/* Send a message. */
int
hal_client_send (struct hal_client *nl, u_char *buf, int len)
{
  int ret;

  ret = writen (nl->mc->sock, buf, len);


  if (ret != len)
    return -1;

  return ret;
}

/*
   sendmsg() to netlink(RFC 3549) socket then recvmsg().
*/
int
hal_talk (struct hal_client *nl, struct hal_nlmsghdr *n,
          int (*filter) (struct hal_nlmsghdr *, void *),
          void *data)
{
  int status;

  /* Request an acknowledgement by setting HAL_NLM_F_ACK */
  n->nlmsg_flags |= HAL_NLM_F_ACK;
  
  if (IS_HAL_DEBUG_EVENT)
    zlog_info (hal_zg, "hallink_talk: %s type %u, seq=%u",
               hallink_cmd.name, n->nlmsg_type, n->nlmsg_seq);
    
  /* Send message to netlink(RFC 3549) interface. */
  status = hal_client_send (nl, (u_char *) n, n->nlmsg_len);
  if (status < 0)
    {
      if (IS_HAL_DEBUG_EVENT)
        zlog_err (hal_zg, "hallink_talk send() error: %s",
                  strerror (errno));
      return -1;
    }

  if (filter)
    status = hal_parse_info (nl, filter, data);
  else
    status = hal_parse_info (nl, hal_recv_cb, data);

  return status;
}

/* Async client Read. */
int
hal_async_client_read (struct message_handler *mc,
                   struct message_entry *me, pal_sock_handle_t sock)
{ 
  return hal_parse_info ((struct hal_client *)mc->info, _hal_callbacks, NULL);
}

/*
  Send HAL generic poll message to HSL.
*/
int
hal_msg_generic_poll_request (int msg,
                              int (*filter) (struct hal_nlmsghdr *, void *),
                              void *data)
{ 
  return hal_msg_generic_request (msg, filter, data);
} 

/* Client connection is established.  Client send service description
   message to the server.  */
int 
hal_client_connect (struct message_handler *mc,
                      struct message_entry *me, pal_sock_handle_t sock)
{
  int ret;
  struct preg_msg preg;

  /* Make the client socket blocking. */
  pal_sock_set_nonblocking (sock, PAL_FALSE); 

  /* Register read thread.  */
  if ((struct hal_client *)mc->info == &hallink_async)
    message_client_read_register (mc);

  preg.len  = MESSAGE_REGMSG_SIZE;
  preg.value = HAL_SOCK_PROTO_NSM;

  /* Encode HAL identifier and send to HSL server */
  ret = pal_sock_write (sock, &preg, MESSAGE_REGMSG_SIZE);
  if (ret <= 0)
    {
      return RESULT_ERROR;
    }

  return RESULT_OK;
}

int
hal_client_disconnect (struct message_handler *mc,
                         struct message_entry *me, pal_sock_handle_t sock)
{
  /* Stop message client.  */
  message_client_stop (mc);

  return 0;
}

#ifdef HAVE_TCP_MESSAGE
/* Get server address from client type */
int
hal_client_get_tcp_server_port (struct hal_client *nl, u_int16_t *port)
{ 
  int ret = 0;

  if (nl == &hallink_cmd)
    *port = HSL_CMD_PORT;
  else if (nl == &hallink_async)
    *port = HSL_ASYNC_PORT;
  else
    ret = -1;

  return ret;
}
  
#else /* TCP Message */
int
hal_client_get_domain_server_path (struct hal_client *nl, char **path)
{
  int ret = 0;

  if (nl == &hallink_cmd)
    *path = HSL_CMD_PATH;
  else if (nl == &hallink_async)
    *path = HSL_ASYNC_PATH;
  else
    ret = -1;

  return ret;
}
#endif /* HAVE_TCP_MESSAGE */

/*
   Make socket for netlink(RFC 3549) interface.
*/
int
hal_client_create (struct lib_globals *zg, struct hal_client *nl,
                   u_char async)
{
  struct message_handler *mc;
  int type = async ? MESSAGE_TYPE_ASYNC: MESSAGE_TYPE_SYNC;
  int ret;

  /* Get server path or port */
#ifdef HAVE_TCP_MESSAGE
  u_int16_t port;
    
  ret = hal_client_get_tcp_server_port (nl, &port);
#else
  char *path;
  
  ret = hal_client_get_domain_server_path (nl, &path);
#endif /* HAVE_TCP_MESSAGE */

  if (ret < 0)
    return ret;

  /* Create async message client.  */
  mc = message_client_create (zg, type);
  if (mc == NULL)
    return -1;
#ifdef HAVE_TCP_MESSAGE
  message_client_set_style_tcp (mc, port);
#else
  message_client_set_style_domain (mc, path);
#endif /* HAVE_TCP_MESSAGE */

  /* Initiate connection using sbipi connection manager.  */
  message_client_set_callback (mc, MESSAGE_EVENT_CONNECT,
                               hal_client_connect);
  message_client_set_callback (mc, MESSAGE_EVENT_DISCONNECT,
                               hal_client_disconnect);
  if (async)
    message_client_set_callback (mc, MESSAGE_EVENT_READ_MESSAGE,
        hal_async_client_read);
  
  /* Link each other.  */
  mc->info = nl;
  nl->mc = mc;
  
  /* Start the hal client. */
  ret = message_client_start (nl->mc);

  return ret;
}

/*
  Close HAL socket.
*/
int
hal_client_destroy (struct hal_client *nl)
{
  message_client_delete (nl->mc);
  nl->mc = NULL;

  return 0;
}

int
hal_comm_init (struct lib_globals *zg)
{ 
  /* Open sockets to HSL. */
  hal_client_create (zg, &hallink_async, 1);
  hal_client_create (zg, &hallink_cmd, 0);
  /*  hal_client_create (zg, &hallink_poll, 0, 0); */
  
  return 0;
}

/*
   Deinitialize HAL-HSL transport.
*/
int
hal_comm_deinit (struct lib_globals *zg)
{
  /* Close sockets to HSL. */
  hal_client_destroy (&hallink_async);
  hal_client_destroy (&hallink_cmd);

  return 0;
}

#else

/* 
   Make socket for netlink(RFC 3549) interface. 
*/
int
hal_socket (struct halsock *nl, unsigned long groups, u_char non_block)
{
  int ret;
  struct hal_sockaddr_nl snl;
  int sock;
  pal_socklen_t namelen;

  sock = pal_sock (hal_zg, AF_HSL, SOCK_RAW, 0);
  if (sock < 0)
    {
      if (IS_HAL_DEBUG_EVENT)
        zlog_err (hal_zg, "Can't open %s socket: %s", nl->name,
                  strerror (errno));
      return -1;
    }

  if (nl == &hallink)
    {
      /* Large socket buffer is required when 4032 VLAN interfaces are
       * present. Without this change, all VLAN interfaces do not
       * come up with RUNNING state
       */
      /* TODO: This requires also a change in Linux sysctl.conf file to allow
       * maximum socket receive buffer to be set to 384K. The net.core.rmem_max        * needs to be set to 384K (393216).
       */
      ret = pal_sock_set_recvbuf (sock, 384 * 1024);
      if (ret < 0)
        {
          if (IS_HAL_DEBUG_EVENT)
            zlog_warn (hal_zg, "Can't set %s socket buffer", nl->name);
        }
    }
              
                
  if (non_block)
    {
      ret = pal_sock_set_nonblocking (sock, PAL_TRUE);
      if (ret < 0)
        {
          if (IS_HAL_DEBUG_EVENT)
            zlog_err (hal_zg, "Can't set %s socket flags: %s", nl->name,
                      strerror (errno));
          close (sock);
          return -1;
        }
    }
  
  memset (&snl, 0, sizeof snl);
  snl.nl_family = AF_HSL;
  snl.nl_groups = groups;

  /* Bind the socket to the netlink(RFC 3549) structure for anything. */
  ret = pal_sock_bind (sock, (struct pal_sockaddr *) &snl, sizeof snl);
  if (ret < 0)
    {
      if (IS_HAL_DEBUG_EVENT)
        zlog_err (hal_zg, "Can't bind %s socket to group 0x%x: %s", nl->name,
                  snl.nl_groups, strerror (errno));
      close (sock);
      return -1;
    }

  /* multiple netlink(RFC 3549) sockets will have different nl_pid */
  namelen = sizeof snl;
  ret = pal_sock_getname (sock, (struct pal_sockaddr *) &snl, &namelen);
  if (ret < 0 || namelen != sizeof snl)
    {
      if (IS_HAL_DEBUG_EVENT)
        zlog_err (hal_zg, "Can't get %s socket name: %s", nl->name,
                  strerror (errno));
      close (sock);
      return -1;
    }

  nl->snl = snl;
  nl->sock = sock;
  return ret;
}

/*
  Close HAL socket. 
*/
int
hal_close (struct halsock *s)
{
  close (s->sock);

  return 0;
}

#if 0
/*
  Get type specified information from netlink(RFC 3549). 
*/
int
hal_request (void *req, int size, int type, struct halsock *nl, int ack_required)
{
  int ret;
  struct hal_sockaddr_nl snl;
  struct hal_nlmsghdr *nlh = (struct hal_nlmsghdr *) req->nlh;

  /* Check netlink(RFC 3549) socket. */
  if (nl->sock < 0)
    {
      if (IS_HAL_DEBUG_EVENT)
        zlog_err (hal_zg, "%s socket isn't active.", nl->name);
      return -1;
    }

  memset (&snl, 0, sizeof snl);
  snl.nl_family = AF_HSL;

  nlh->nlmsg_len = size;
  nlh->nlmsg_type = type;
  nlh->nlmsg_flags = HAL_NLM_F_ROOT | HAL_NLM_F_MATCH | HAL_NLM_F_REQUEST;
  if (ack_required)
    nlh->nlmsg_flags |= HAL_NLM_F_ACK;
  nlh->nlmsg_pid = 0;
  nlh->nlmsg_seq = ++nl->seq;
 
  ret = pal_sock_sendto (nl->sock, (void*) &req, size, 0, 
                         (struct sockaddr*) &snl, sizeof snl);
  if (ret < 0)
    {
      if (IS_HAL_DEBUG_EVENT)
        zlog_err (hal_zg, "%s sendto failed: %s", nl->name, strerror (errno));
      return -1;
    }
  return 0;
}
#endif

/* 
   Receive message from netlink(RFC 3549) interface and pass those information
   to the given function. 
*/
int
hal_parse_info (struct halsock *nl, 
                int (*filter) (struct hal_nlmsghdr *, void *),
                void *data)
{
  int status;
  int ret = 0;
  int error;

  while (1)
    {
      char buf[4096];
      struct iovec iov = { buf, sizeof buf };
      struct hal_sockaddr_nl snl;
      struct pal_msghdr msg = {  (void*)&snl, sizeof snl, (void *)&iov, 1, NULL, 0, 0};
      struct hal_nlmsghdr *h;

      status = pal_sock_recvmsg (nl->sock, &msg, 0);

      if (status < 0)
        {
          if (errno == EINTR)
            continue;
          if (errno == EWOULDBLOCK)
            break;
          if (IS_HAL_DEBUG_EVENT)
            zlog_err (hal_zg, "%s recvmsg overrun: %s", nl->name,
                      strerror (errno));
          continue;
        }

      if (status == 0)
        {
          if (IS_HAL_DEBUG_EVENT)
            zlog_err (hal_zg, "%s EOF", nl->name);
          return -1;
        }

      for (h = (struct hal_nlmsghdr *) buf; HAL_NLMSG_OK (h, status); 
           h = HAL_NLMSG_NEXT (h, status))
        {
          /* Finish of reading. */
          if (h->nlmsg_type == HAL_NLMSG_DONE)
          {
            return ret;
          }

          /* Error handling. */
          if (h->nlmsg_type == HAL_NLMSG_ERROR)
            {
              struct hal_nlmsgerr *err = (struct hal_nlmsgerr *) HAL_NLMSG_DATA (h);

              if (h->nlmsg_len < HAL_NLMSG_LENGTH (sizeof (struct hal_nlmsgerr)))
                {
                  if (IS_HAL_DEBUG_EVENT)
                    zlog_err (hal_zg, "%s error: message truncated", nl->name);
                  return -1;
                }

              if (IS_HAL_DEBUG_EVENT)
                zlog_err (hal_zg, "%s error: %s, type=%u, seq=%u, pid=%d",
                          nl->name, strerror (-err->error),
                          err->msg.nlmsg_type,
                          err->msg.nlmsg_type, err->msg.nlmsg_seq,
                          err->msg.nlmsg_pid);
              return err->error;
            }

          /* OK we got netlink(RFC 3549) message. */
          if (IS_HAL_DEBUG_EVENT)
            zlog_info (hal_zg,
                       "hal_parse_info: %s type %u, seq=%u, pid=%d",
                       nl->name, h->nlmsg_type,
                       h->nlmsg_type, h->nlmsg_seq, h->nlmsg_pid);

          /* Skip unsolicited messages originating from command socket. */
          if (nl != &hallink_cmd && h->nlmsg_pid == hallink_cmd.snl.nl_pid)
            {
              if (IS_HAL_DEBUG_EVENT)
                zlog_info (hal_zg,
                           "hallink_parse_info: %s packet comes from %s",
                           nl->name, hallink_cmd.name);
              continue;
            }

          error = (*filter) (h, data);
          if (error < 0)
            {
              if (IS_HAL_DEBUG_EVENT)
                zlog_err (hal_zg, "%s filter function error", nl->name);
              ret = error;
            }
        }

      /* After error care. */
      if (msg.msg_flags & MSG_TRUNC)
        {
          if (IS_HAL_DEBUG_EVENT)
            zlog_err (hal_zg, "%s error: message truncated", nl->name);
          continue;
        }
      if (status)
        {
          if (IS_HAL_DEBUG_EVENT)
            zlog_err (hal_zg, "%s error: data remnant size %d",
                      nl->name, status);
          return -1;
        }
    }
  return ret;
}

/* 
   sendmsg() to netlink(RFC 3549) socket then recvmsg().
*/
int
hal_talk (struct halsock *nl, struct hal_nlmsghdr *n,
          int (*filter) (struct hal_nlmsghdr *, void *),
          void *data)
{
  int status;
  struct hal_sockaddr_nl snl;
  struct pal_iovec iov = { (void*) n, n->nlmsg_len };
  struct pal_msghdr msg = {(void*) &snl, sizeof snl, &iov, 1, NULL, 0, 0};

  memset (&snl, 0, sizeof (snl));
  snl.nl_family = AF_HSL;
  
  /* Request an acknowledgement by setting HAL_NLM_F_ACK */
  n->nlmsg_flags |= HAL_NLM_F_ACK;

  if (IS_HAL_DEBUG_EVENT)
    zlog_info (hal_zg, "hallink_talk: %s type %u, seq=%u",
               hallink_cmd.name, n->nlmsg_type,
               n->nlmsg_type, n->nlmsg_seq);

  /* Send message to netlink(RFC 3549) interface. */
  status = pal_sock_sendmsg (nl->sock, &msg, 0);
  if (status < 0)
    {
      if (IS_HAL_DEBUG_EVENT)
        zlog_err (hal_zg, "hallink_talk sendmsg() error: %s",
                  strerror (errno));
      return -1;
    }

  if (filter)
    status = hal_parse_info (nl, filter, data);
  else
    status = hal_parse_info (nl, hal_recv_cb, data);

  return status;
}

/*
  Send HAL generic poll message to HSL.
*/
int
hal_msg_generic_poll_request (int msg, int (*filter) (struct hal_nlmsghdr *, void *),
                              void *data)
{
  int ret;
  struct hal_nlmsghdr *nlh;
  struct 
  {
    struct hal_nlmsghdr nlh;
  } req;

  memset (&req.nlh, 0, sizeof (struct hal_nlmsghdr));

  /* Set header. */
  nlh = &req.nlh;
  nlh->nlmsg_len = HAL_NLMSG_LENGTH (0);
  nlh->nlmsg_flags = HAL_NLM_F_CREATE | HAL_NLM_F_REQUEST;
  nlh->nlmsg_type = msg;
  nlh->nlmsg_seq = ++hallink_poll.seq;

  /* Request list of interfaces. */
  ret = hal_talk (&hallink_poll, nlh, filter, data);
  if (ret < 0)
    return ret;

  return HAL_SUCCESS;
}

/*
  Read thread for async messages.
*/
int
_hal_read (struct thread *thread)
{
  int ret = 0;
  int sock;
  struct lib_globals *zg;

  sock = THREAD_FD (thread);
  zg = THREAD_ARG (thread);
  hallink.t_read = NULL;

  ret = hal_parse_info (&hallink, _hal_callbacks, NULL);

  hallink.t_read = thread_add_read (zg, _hal_read, zg, hallink.sock);

  return ret;
}

/* 
   Initialize HAL-HSL transport.
*/
int
hal_comm_init (struct lib_globals *zg)
{
  unsigned long groups;

  groups = HAL_GROUP_LINK;

  /* Open sockets to HSL. */
  hal_socket (&hallink, groups, 0);
  hal_socket (&hallink_cmd, 0, 0);
  hal_socket (&hallink_poll, 0, 0);

  /* Register HAL socket. */
  if (hallink.sock > 0)
    hallink.t_read = thread_add_read (zg, _hal_read, zg, hallink.sock);

  hal_comm_initialized = 1;
  return 0;
}

/* 
   Deinitialize HAL-HSL transport.
*/
int
hal_comm_deinit (struct lib_globals *zg)
{
  /* Close sockets to HSL. */
  hal_close (&hallink);
  hal_close (&hallink_cmd);
  hal_close (&hallink_poll);

  hal_comm_initialized = 0;
  return 0;
}

#endif /* HAVE_USER_HSL */
#endif /* HAL_TCP_COMM */


