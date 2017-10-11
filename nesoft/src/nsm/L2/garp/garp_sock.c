/* Copyright (C) 2003  All Rights Reserved. */

 /* This module acts as the top half of the GARP layer 
   It will make pal calls to initialise, send & receive frames, 
   and other GARP related functions. */
#include "pal.h"
#include "lib.h"
#include "thread.h"
#include "l2lib.h"

#include "nsmd.h"
#ifdef HAVE_USER_HSL
#include "hal_socket.h"
#include "message.h"
#include "network.h"
#endif /* HAVE_USER_HSL */

#include "nsmd.h"
#include "nsm_bridge.h"
#include "l2_llc.h"
#include "l2_debug.h"
#include "garp_gid.h"
#include "garp_pdu.h"
#include "garp.h"
#include "garp_sock.h"

#include "hal_incl.h"

#ifdef HAVE_GVRP
#include "nsm_vlan.h"
#include "gvrp.h"
#include "gvrp_pdu.h"
#endif /* HAVE_GVRP */

#ifdef HAVE_GMRP
#include "nsm_vlan.h"
#include "gmrp.h"
#include "gmrp_pdu.h"
#endif /* HAVE_GMRP */

#define RCV_BUFSIZ  1500

/* Local socket connection to forwarder */
static pal_sock_handle_t sockfd = -1;

#ifndef AF_GARP
#define AF_GARP        44
#endif 

#ifdef HAVE_USER_HSL
struct garp_client garplink_async = { NULL };
#else
static struct thread *garp_thread = NULL;
#endif /* HAVE_USER_HSL */

const unsigned char gmrp_group_addr[6] = { 0x01, 0x80, 0xc2, 0x00, 0x00, 0x20 };
const unsigned char gvrp_group_addr[6] = { 0x01, 0x80, 0xc2, 0x00, 0x00, 0x21 };
const unsigned char pro_gvrp_addr[6]   = { 0x01, 0x80, 0xc2, 0x00, 0x00, 0x0d };

static void 
garp_handle_frame (struct lib_globals *zg, struct sockaddr_vlan *l2_skaddr, 
                   unsigned char *buf, s_int32_t len);

#ifndef HAVE_USER_HSL
/* Initialize GARP sub system.  */
s_int32_t
garp_sock_init (struct lib_globals *zg)
{
  if ( sockfd < 0)
    sockfd = pal_sock(zg, AF_GARP, SOCK_RAW,0);

  if (sockfd < 0)
    {
      zlog_debug (zg, "Can't initialize bridge feature.");
      return RESULT_ERROR;
    }

  /* Immediately add read thread. */
  if (garp_thread == NULL)
    garp_thread = thread_add_read_high (zg, garp_recv, NULL, sockfd);

  return sockfd;
}

/* Deinitialize socket. */
s_int32_t
garp_sock_deinit (struct lib_globals *zg)
{
  /* Cancel GARP thread. */
  if (garp_thread)
    {
      thread_cancel (garp_thread);
      garp_thread = NULL;
    }

  /* Close socket. */
  pal_sock_close (zg, sockfd);

  return 0;
}

/* Receive a frame via the pal. */
s_int32_t
garp_recv (struct thread *thread)
{
  struct sockaddr_vlan l2_skaddr;
  struct lib_globals *zg = thread->zg;
  u_int8_t data[RCV_BUFSIZ];
  s_int32_t ret = 0;
  static unsigned int recv_err = 0;
  u_int32_t fromlen = sizeof(struct sockaddr_vlan);

  if (sockfd < 0)
    {
      zlog_err (zg, "PDU[RECV]: socket is not open (%d)", sockfd);
      return -1;
    }

  pal_mem_set (data, 0, RCV_BUFSIZ);

  /* Receive GARP BPDU frame. */
  ret = pal_sock_recvfrom(sockfd, (char *)data, RCV_BUFSIZ,MSG_TRUNC,
                          (struct pal_sockaddr *)&l2_skaddr,
                          (pal_socklen_t *) &fromlen);

  if (ret < 0)
    {
      recv_err++;
      if (recv_err > 10)
        {
          zlog_err (zg, 
            "PDU[RECV]: receive failed (%d) - aborting recv thread", ret);
          return ret;
        }
      zlog_warn (zg, "PDU[RECV]: receive failed (%d)", ret);
      garp_thread = thread_add_read_high (zg, garp_recv, NULL, sockfd);
      return ret;
    }
  else
    {
      recv_err = 0;
    }
  if (LAYER2_DEBUG (proto, PACKET_RX)) 
    {
      zlog_debug (zg, "PDU[RECV]: len (%d) "
          "src addr (%.02x%.02x.%.02x%.02x.%.02x%.02x) "
          "dst addr (%.02x%.02x.%.02x%.02x.%.02x%.02x) ",
          ret,
          l2_skaddr.src_mac[0],
          l2_skaddr.src_mac[1],
          l2_skaddr.src_mac[2],
          l2_skaddr.src_mac[3],
          l2_skaddr.src_mac[4],
          l2_skaddr.src_mac[5],
          l2_skaddr.dest_mac[0],
          l2_skaddr.dest_mac[1],
          l2_skaddr.dest_mac[2],
          l2_skaddr.dest_mac[3],
          l2_skaddr.dest_mac[4],
          l2_skaddr.dest_mac[5]);

    }


  garp_handle_frame (zg, &l2_skaddr, data, ret);

  /* Immediately add read thread. */
  garp_thread = thread_add_read_high (zg, garp_recv, NULL, sockfd);
  return ret;
}

/* Send an GARP frame through the selected port via the pal.
   This function returns the number of bytes transmitted or
   -1 if an error.
*/
s_int32_t
garp_send (const char * const brname,u_char * src_addr, const u_char *dest_addr,
           const u_int32_t ifindex , unsigned char *data, s_int32_t length, u_int16_t vid)
{
  struct sockaddr_vlan  l2_skaddr;
  int ret;
  int tolen;
   
  if ( (sockfd < 0) || (data == NULL) )
    {
      return RESULT_ERROR;
    }
  
  tolen = sizeof(struct sockaddr_vlan);
  pal_mem_set (&l2_skaddr, 0, tolen);
  
  /* Fill out the l2_skaddr structure here */
  l2_skaddr.port = ifindex;
  l2_skaddr.vlanid = vid;
  pal_mem_cpy (l2_skaddr.dest_mac, dest_addr, HAL_HW_LENGTH);
  pal_mem_cpy (l2_skaddr.src_mac, src_addr, HAL_HW_LENGTH);
 
  ret = pal_sock_sendto (sockfd, (char *)data, length, 0, 
                         (struct pal_sockaddr *)&l2_skaddr, tolen);
  if (ret < 0)
    return RESULT_ERROR;
  
  return ret;
}

#else

pal_sock_handle_t
garp_sock_init (struct lib_globals *zg)
{
  /* Open sockets to HSL. */
  sockfd = garp_client_create (zg, &garplink_async, GARP_ASYNC);
  if (sockfd < 0)
    {
      if (LAYER2_DEBUG(proto, PROTO))
        zlog_debug (zg, "Can't initialize bridge feature.");

      return RESULT_ERROR;
    }

  return sockfd;
}

int
garp_client_create (struct lib_globals *zg, struct garp_client *nl,
                    u_char async)
{
  struct message_handler *mc;
  int ret;

  /* Create async message client.  */
  mc = message_client_create (zg, MESSAGE_TYPE_ASYNC);
  if (mc == NULL)
    return -1;

#ifdef HAVE_TCP_MESSAGE
  message_client_set_style_tcp (mc, HSL_ASYNC_PORT);
#else
  message_client_set_style_domain (mc, HSL_ASYNC_PATH);
#endif /* HAVE_TCP_MESSAGE */

  /* Initiate connection using mstp client connection manager.  */
  message_client_set_callback (mc, MESSAGE_EVENT_CONNECT,
                               garp_client_connect);
  message_client_set_callback (mc, MESSAGE_EVENT_DISCONNECT,
                               garp_client_disconnect);

  if (async)
    message_client_set_callback (mc, MESSAGE_EVENT_READ_MESSAGE,
                                 garp_recv);

  /* Link each other.  */
  mc->info = nl;
  nl->mc = mc;

  /* Start the mstp client. */
  ret = message_client_start (nl->mc);

  return ret;
}

int
garp_client_connect (struct message_handler *mc, struct message_entry *me,
                     pal_sock_handle_t sock)
{
  int ret;
  struct preg_msg preg;

  /* Make the client socket blocking. */
  pal_sock_set_nonblocking (sock, PAL_FALSE);

  /* Register read thread.  */
  if ((struct garp_client *)mc->info == &garplink_async)
    message_client_read_register (mc);

  preg.len  = MESSAGE_REGMSG_SIZE;
  preg.value = HAL_SOCK_PROTO_GARP;

  /* Encode protocol identifier and send to HSL server */
  ret = pal_sock_write (sock, &preg, MESSAGE_REGMSG_SIZE);
  if (ret <= 0)
    {
      return RESULT_ERROR;
    }

  return RESULT_OK;

}

int
garp_client_disconnect (struct message_handler *mc, struct message_entry *me,
                        pal_sock_handle_t sock)
{
  /* Stop message client.  */
  message_client_stop (mc);

  return RESULT_OK;
}

s_int32_t
garp_send (const char * const brname,u_char * src_addr, const u_char *dest_addr,
           const u_int32_t ifindex , unsigned char *data, s_int32_t length, u_int16_t vid)
{
  struct sockaddr_vlan vlan_skaddr;
  u_int8_t send_buf [RCV_BUFSIZ];
  int ret;
  int tolen;

  if ( (sockfd < 0) || (data == NULL) )
    {
      return RESULT_ERROR;
    }

  tolen = length + sizeof(struct sockaddr_vlan);

  /* Fill out the vlan_skaddr structure here */
  vlan_skaddr.port = ifindex;
  vlan_skaddr.vlanid = vid;
  vlan_skaddr.length = tolen;

  pal_mem_cpy (vlan_skaddr.dest_mac, dest_addr, HAL_HW_LENGTH);
  pal_mem_cpy (vlan_skaddr.src_mac, src_addr, HAL_HW_LENGTH);

  pal_mem_cpy (send_buf, &vlan_skaddr, sizeof (struct sockaddr_vlan));
  pal_mem_cpy ((send_buf + sizeof (struct sockaddr_vlan)), data, length);

  ret = writen (garplink_async.mc->sock, send_buf, tolen);

  if (ret < 0)
    return RESULT_ERROR;

  return ret;
}

s_int32_t
garp_recv (struct message_handler *mc, struct message_entry *me,
           pal_sock_handle_t sock)
{
  int nbytes = 0;
  u_char buf[RCV_BUFSIZ];
  struct sockaddr_vlan vlan_skaddr;
  static unsigned int recv_err = 0;

  /* Peek at least Control Information */
  nbytes = pal_sock_recvfrom (sock, (void *)&vlan_skaddr,
                              sizeof (struct sockaddr_vlan),
                              MSG_PEEK, NULL, NULL);

  if (nbytes <= 0)
    {
      zlog_warn (NSM_ZG, "PDU[RECV]: receive failed (%d)", nbytes);
      return nbytes;
    }

  nbytes = pal_sock_read (sock, buf, vlan_skaddr.length);

  if (nbytes <= 0)
    {
      zlog_warn (NSM_ZG, "PDU[RECV]: receive failed (%d)", nbytes);
      return nbytes;
    }

  if (nbytes < vlan_skaddr.length)
    zlog_warn (NSM_ZG, "PDU[RECV]: Packet Truncated (%d)", nbytes);

  if (nbytes < 0)
    {
      recv_err++;
      if (recv_err > 10)
        {
          zlog_err (NSM_ZG,
            "PDU[RECV]: receive failed (%d) - aborting recv thread", nbytes);
          return nbytes;
        }

      zlog_warn (NSM_ZG, "PDU[RECV]: receive failed (%d)", nbytes);

      return nbytes;
    }
  else
    {
      recv_err = 0;
    }

  pal_mem_cpy (&vlan_skaddr, buf, sizeof (struct sockaddr_vlan));  

  if (LAYER2_DEBUG (proto, PACKET_RX))
    {
      zlog_debug (NSM_ZG, "PDU[RECV]: len (%d) "
          "src addr (%.02x%.02x.%.02x%.02x.%.02x%.02x) "
          "dst addr (%.02x%.02x.%.02x%.02x.%.02x%.02x) ",
          nbytes,
          vlan_skaddr.src_mac[0],
          vlan_skaddr.src_mac[1],
          vlan_skaddr.src_mac[2],
          vlan_skaddr.src_mac[3],
          vlan_skaddr.src_mac[4],
          vlan_skaddr.src_mac[5],
          vlan_skaddr.dest_mac[0],
          vlan_skaddr.dest_mac[1],
          vlan_skaddr.dest_mac[2],
          vlan_skaddr.dest_mac[3],
          vlan_skaddr.dest_mac[4],
          vlan_skaddr.dest_mac[5]);
    }
  
  garp_handle_frame (NSM_ZG, &vlan_skaddr, (buf + sizeof(struct sockaddr_vlan)),
                     (nbytes - sizeof(struct sockaddr_vlan)) );

  return nbytes; 
}

#endif /* HAVE_USER_HSL */

static void 
garp_handle_frame (struct lib_globals *zg, struct sockaddr_vlan *l2_skaddr,
                   u_char *buf, s_int32_t len)
{

  /* Validate the l2_skaddr data. Probably need to add length check. */
  if (l2_skaddr == NULL)
      return;


  if (LAYER2_DEBUG(proto, PROTO))
    {
      zlog_info (zg, 
       "garp_handle_frame: received frame on port %d",
        l2_skaddr->port);
     }

#if defined HAVE_GVRP || defined HAVE_MVRP
  if ( (!memcmp (l2_skaddr->dest_mac, gvrp_group_addr, 6)) ||
       (!memcmp (l2_skaddr->dest_mac, pro_gvrp_addr, 6)) )
    {
      gvrp_receive_pdu (l2_skaddr->port, l2_skaddr->src_mac, buf, len);
      return;
    }
#endif /* HAVE_GVRP || HAVE_MVRP */

#if defined HAVE_GMRP || defined HAVE_MMRP
  if ( !memcmp (l2_skaddr->dest_mac, gmrp_group_addr, 6) )
    {
      gmrp_receive_pdu (l2_skaddr->port, l2_skaddr->src_mac,
                        buf, len, l2_skaddr->vlanid);
      return;
    }
#endif /* HAVE_GMRP || HAVE_MMRP */

}

bool_t
garp_ethertype_parse (unsigned char * frame)
{
  if ((*frame++ == 0x08) && ((*frame == 0x11) || (*frame == 0x12)))
    return PAL_TRUE;
  else
    return PAL_FALSE;
}

