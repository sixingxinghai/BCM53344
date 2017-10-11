
/**@file elmi_sock.c
 * @brief This file contains functions like pal calls to initialise, 
 * send & receive frames and other ELMI related functions.
 **/

/* Copyright (C) 2010  Rights Reserved. */
#include "pal.h"
#include "lib.h"
#include "thread.h"
#include "l2lib.h"
#include "elmi_types.h"
#include "elmi_bridge.h"
#include "l2_llc.h"
#include "l2_debug.h"
#include "elmi_debug.h"
#include "elmid.h"
#include "elmi_sock.h"
#include "message.h"
#include "network.h"
#include "elmi_uni_c.h"

#ifndef RCV_BUFSIZ
#define RCV_BUFSIZ 1500 
#endif /* RCV_BUFSIZ */

/* Local socket connection to forwarder */
static pal_sock_handle_t sockfd = ELMI_FAILURE;

#ifndef AF_ELMI
#define AF_ELMI   51 
#endif

static void
elmi_handle_frame (struct lib_globals *zg, struct sockaddr_l2 *l2_skaddr,
                   u_char *pnt, s_int32_t len);

s_int32_t
elmi_frame_recv (struct thread *thread)
{
  struct sockaddr_l2 l2_skaddr;
  struct lib_globals *zg = thread->zg;
  u_char  data[RCV_BUFSIZ];
  s_int32_t ret = 0;
  pal_socklen_t fromlen = sizeof(struct sockaddr_l2);

  if (sockfd < 0)
    {
      zlog_err (zg, "PDU[RECV]: socket is not open (%d)", sockfd);
      return RESULT_ERROR;
    }

  /* Receive ELMI frame. */
  pal_mem_set (data, 0, RCV_BUFSIZ);

  ret = pal_sock_recvfrom (sockfd, (char *)data, RCV_BUFSIZ, MSG_TRUNC,
                           (struct pal_sockaddr *)&l2_skaddr, &fromlen);

  if (ret < 0)
    {
      zlog_warn (zg, "PDU[RECV]: receive failed (%d)", ret);

      /* Add read thread for next frame. */
      thread_add_read_high (zg, elmi_frame_recv, NULL, sockfd);

      return ret; 
    }

  if (ret < ELMI_MIN_PACKET_SIZE)
   {
      zlog_warn (zg, "PDU[RECV]: Len invalid (%d)", ret);
      return ret; 
   }

 /* Handle elmi frame */ 
  elmi_handle_frame (zg, &l2_skaddr, data, ret);

  /* Continue for next frames */
  thread_add_read_high (ZG, elmi_frame_recv, NULL, sockfd);
  
  return ret;
}

/* Transmission */
s_int32_t
elmi_frame_send (u_char *src_addr, const u_char *dest_addr, 
                 u_int32_t ifindex , u_char *data, u_int32_t length)
{
  s_int32_t ret = 0;
  struct sockaddr_l2 l2_skaddr;  

  if ( (sockfd < 0) || (data == NULL) )
    {
      return RESULT_ERROR;
    }

  /* Fill out the l2_skaddr structure here */
  l2_skaddr.port = ifindex;
  pal_mem_cpy (l2_skaddr.dest_mac, dest_addr, ETHER_ADDR_LEN);
  pal_mem_cpy (l2_skaddr.src_mac, src_addr, ETHER_ADDR_LEN);

  ret = pal_sock_sendto (sockfd, (u_char *)data, length, 0,
                         (struct pal_sockaddr *)&l2_skaddr, 
                         sizeof (struct sockaddr_l2));

  if (ret < 0)
    return RESULT_ERROR;

  return ret;

}

pal_sock_handle_t
elmi_sock_init (struct lib_globals *zg)
{

  sockfd = pal_sock(zg, AF_ELMI, SOCK_RAW, 0);

  if ( sockfd < 0)
    {
      perror ("socket");

      zlog_debug (ZG, "socket creation failure");

      return RESULT_ERROR;
    }

  return sockfd;
}


static void
elmi_handle_frame (struct lib_globals *zg, struct sockaddr_l2 *l2_skaddr,
                   u_char *pnt, s_int32_t len)
{
  struct interface *ifp = NULL;
  struct elmi_ifp *elmi_if = NULL;
  struct apn_vr *vr = apn_vr_get_privileged (zg);
  struct elmi_master *em = NULL;
  s_int32_t ret = 0;

  /* Validate the l2_skaddr data */
  if (l2_skaddr == NULL)
    return;

  ifp = if_lookup_by_index (&vr->ifm, l2_skaddr->port);

  if (!ifp)
    return;

  elmi_if = ifp->info;

  if (!elmi_if || !elmi_if->elmi_enabled)
    return;
 
  em = elmi_if->elmim;

  if (ELMI_DEBUG(protocol, PROTOCOL))
    elmi_hexdump (pnt, len);
 
  /* validate the frame for correct MAC address */
  if (!ELMI_IS_VALID_DEST_MAC (l2_skaddr->dest_mac))
    {
      if (ELMI_DEBUG(protocol, PROTOCOL))
        zlog_debug (ZG, "ELMI [PDU-RCV]: Frame Recognition -"
                    " Destination address mismatch %s", l2_skaddr->dest_mac);
      return;
    }

  /* Check for port type */
  if (elmi_if->uni_mode == ELMI_UNI_MODE_UNIC)
    ret = elmi_unic_handle_frame (zg, l2_skaddr, &pnt, &len);
  else if (elmi_if->uni_mode == ELMI_UNI_MODE_UNIN)
    ret = elmi_unin_handle_frame (zg, l2_skaddr, &pnt, &len);  
  else 
    {
      if (ELMI_DEBUG(protocol, PROTOCOL))
        zlog_debug (ZG, "ELMI [PDU-RCV]: Frame Recognition -"
            "Discarding the Frame as port type is not valid\n");
      return;
    }

  return;
}
