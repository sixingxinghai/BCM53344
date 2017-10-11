/* Copyright (C) 2001-2009  All Rights Reserved. */

#include "pal.h"

#include "lib.h"
#include "vty.h"
#include "if.h"
#include "prefix.h"
#include "stream.h"
#include "thread.h"
#include "sockunion.h"
#include "log.h"
#include "checksum.h"

#include "nsm/nsmd.h"
#include "nsm/nsm_interface.h"
#include "nsm/nsm_router.h"
#include "nsm/nsm_debug.h"

#include "nsm/vrrp/vrrp.h"
#include "nsm/vrrp/vrrp_debug.h"
#include "nsm/vrrp/vrrp_api.h"

static  int _make_socket();
static VRRP_SESSION *_validate_advert (VRRP_GLOBAL_DATA *vrrp,
                                       struct pal_in4_header *iph,
                                       struct interface *ifp,
                                       struct pal_sockaddr_in4 *from,
                                       struct vrrp_advt_info   *advt);
static int _send_advert (struct interface *, int, struct stream *, int, u_int8_t);

static void _prepare_stream_buffer (struct stream *s, size_t size);
static s_int16_t _get_vrrp_packet_length (struct pal_in4_header *iph);
static struct interface *_recv_advert (VRRP_GLOBAL_DATA *vrrp, struct pal_in4_header *iph);

#ifdef HAVE_IPV6
static int  _make_ipv6_socket();
static int
_validate_check_sum (VRRP_GLOBAL_DATA *vrrp,
                     u_int32_t vrrp_len,
                     struct pal_in6_addr *vrrpsrc,
                     struct pal_in6_addr *vrrpdst);
static VRRP_SESSION *
_validate_ipv6_advert (VRRP_GLOBAL_DATA      *vrrp,
                       struct interface      *ifp,
                       struct pal_sockaddr_in6 *from,
                       struct vrrp_advt_info *advt);

#endif /* HAVE_IPV6 */
/*------------------------------------------------------------------------
 * vrrp_ip_init() - called by VRRP to perform any initialization or
 *              registration with the IP application.  This API call
 *              will call all appropriate IP APIs.
 *------------------------------------------------------------------------*/
int
vrrp_ip_init (struct vrrp_global *vrrp)
{
  /* Create the VRRP socket */
  vrrp->sock = _make_socket ();
  if (vrrp->sock < 0) {
    zlog_err (NSM_ZG, "Error opening VRRP socket\n");
    return VRRP_FAILURE;
  }

  vrrp->ibuf = stream_new (VRRP_BUFFER_SIZE_DEFAULT);
  /* Setup the VRRP function to receive VRRP packets */
  vrrp->t_vrrp_read =
    thread_add_read (nzg, vrrp_ip_recv_advert, vrrp, vrrp->sock);

  return VRRP_OK;
}

#ifdef HAVE_IPV6
int
vrrp_ipv6_init (struct vrrp_global *vrrp)
{
  /* Create the VRRP socket */
  vrrp->ipv6_sock = _make_ipv6_socket ();
  if (vrrp->ipv6_sock < 0) {
    zlog_err (NSM_ZG, "Error opening VRRP socket\n");
    return VRRP_FAILURE;
  }

  vrrp->i6buf = stream_new (VRRP_BUFFER_SIZE_DEFAULT);

  /* Setup the VRRP function to receive VRRP packets */
  vrrp->t_vrrp_read =
  thread_add_read (nzg, vrrp_ipv6_recv_advert, vrrp, vrrp->ipv6_sock);

  return VRRP_OK;
}
#endif /* HAVE_IPV6 */

/*-------------------------------------------------------------------
 *                   M U L T I C A S T
 *-------------------------------------------------------------------
 */
#ifdef HAVE_IPV6
int
vrrp_ipv6_multicast_join (pal_sock_handle_t sock,
                           struct pal_in6_addr addr,
                           struct interface *ifp)
{
  result_t ret;

  ret = pal_sock_set_ipv6_multicast_join (sock, addr, ifp->ifindex);
  if (ret < 0)
    {
      if (IS_DEBUG_VRRP_EVENT)
        zlog_err (NSM_ZG, "VRRP error[%s]: Can't join %R multicast group: %s",
                  ifp->name, &addr, pal_strerror (errno));
      return VRRP_FAILURE;
    }

  if (IS_DEBUG_VRRP_EVENT)
    zlog_info (NSM_ZG, "VRRP info: %s ADD_MEMBERSHIP to %R SUCCESS",
               ifp->name, &addr);

  return VRRP_OK;
}

int
vrrp_ipv6_multicast_leave (pal_sock_handle_t sock, struct pal_in6_addr addr,
                           struct interface *ifp)
{
  result_t ret;

  ret = pal_sock_set_ipv6_multicast_leave (sock, addr, ifp->ifindex);
  if (ret < 0)
    {
      if (IS_DEBUG_VRRP_EVENT)
        zlog_err (NSM_ZG, "VRRP error[%s]: Can't leave %R multicast group: %s",
                  ifp->name, &addr, pal_strerror (errno));
      return -1;
    }

  if (IS_DEBUG_VRRP_EVENT)
    zlog_info (NSM_ZG, "VRRP info: %s LEAVE_MEMBERSHIP to %R SUCCESS",
               ifp->name, &addr);

  return 0;
}

/* ------------------------------------------------------------
 *  VRRP Backup Router activity:
 *  Compute and join the Solicited-Node multicast address for the
 *  IPv6 address addresses associated with the Virtual Router.
   --------------------------------------------------------------*/
void
vrrp_ipv6_join_solicit_node_multicast (VRRP_SESSION *sess)
{
  struct pal_in6_addr addr;
  int ret;

  if (sess)
    {
     addr.s6_addr[0]  = 0xff;
     addr.s6_addr[1]  = 0x02;
     addr.s6_addr[2]  = 0x0;
     addr.s6_addr[3]  = 0x0;
     addr.s6_addr[4]  = 0x0;
     addr.s6_addr[5]  = 0x0;
     addr.s6_addr[6]  = 0x0;
     addr.s6_addr[7]  = 0x0;
     addr.s6_addr[8]  = 0x0;
     addr.s6_addr[9]  = 0x0;
     addr.s6_addr[10] = 0x0;
     addr.s6_addr[11] =0x01;
     addr.s6_addr[12] =0xff;
     addr.s6_addr[13] =((char *) &sess->vip_v6.s6_addr)[13];
     addr.s6_addr[14] =((char *) &sess->vip_v6.s6_addr)[14];
     addr.s6_addr[15] =((char *) &sess->vip_v6.s6_addr)[15];

     ret = vrrp_ipv6_multicast_join (sess->vrrp->ipv6_sock, addr, sess->ifp);
     if (ret < 0)
       zlog_warn (NSM_ZG, "VRRP warn: Error joining solicit node multicast");
     else
       zlog_info (NSM_ZG, "VRRP Info: Joining solicit node multicast");
   }
}

/* ------------------------------------------------------------
 *  Compute and join the Solicited-Node multicast address for the
 *  IPv6 address addresses associated with the Virtual Router.
   --------------------------------------------------------------*/
void
vrrp_ipv6_leave_solicit_node_multicast (VRRP_SESSION *sess)
{
  struct pal_in6_addr addr;
  int ret;

  if (sess)
    {
     addr.s6_addr[0]  = 0xff;
     addr.s6_addr[1]  = 0x02;
     addr.s6_addr[2]  = 0x0;
     addr.s6_addr[3]  = 0x0;
     addr.s6_addr[4]  = 0x0;
     addr.s6_addr[5]  = 0x0;
     addr.s6_addr[6]  = 0x0;
     addr.s6_addr[7]  = 0x0;
     addr.s6_addr[8]  = 0x0;
     addr.s6_addr[9]  = 0x0;
     addr.s6_addr[10] = 0x0;
     addr.s6_addr[11] =0x01;
     addr.s6_addr[12] =0xff;
     addr.s6_addr[13] =((char *) &sess->vip_v6.s6_addr)[13];
     addr.s6_addr[14] =((char *) &sess->vip_v6.s6_addr)[14];
     addr.s6_addr[15] =((char *) &sess->vip_v6.s6_addr)[15];

     ret = vrrp_ipv6_multicast_leave (sess->vrrp->ipv6_sock, addr, sess->ifp);
     if (ret < 0)
       zlog_warn (NSM_ZG, "VRRP warn: Error leaving solicit node multicast");
     else
       zlog_info (NSM_ZG, "VRRP Info: Leaving solicit node multicast");
   }
}

#endif /* HAVE_IPV6 */

/*------------------------------------------------------------------------
 * vrrp_ipv4_multicast_join - called to join the VRRP multicast group
 *------------------------------------------------------------------------*/
int
vrrp_ipv4_multicast_join (int sock,
                          struct pal_in4_addr group,
                          struct pal_in4_addr ifa,
                          u_int32_t ifindex)
{
  result_t ret;

  ret = pal_sock_set_ipv4_multicast_join (sock, group, ifa, 0);
  if (ret < 0)
    {
      if (IS_DEBUG_VRRP_EVENT)
        {
          if (errno == EADDRINUSE)
            zlog_err (NSM_ZG, "VRRP Warning: IP_ADD_MEMBERSHIP to %r failed."
                      " Membership already exists", &ifa);
          else
            zlog_err (NSM_ZG, "VRRP Error: can't setsockopt IP_ADD_MEMBERSHIP");
        }
      return VRRP_FAILURE;
    }

  if (IS_DEBUG_VRRP_EVENT)
  zlog_info (NSM_ZG, "VRRP info: IP_ADD_MEMBERSHIP to %r SUCCESS", &ifa);

  return VRRP_OK;
}

/*------------------------------------------------------------------------
 * vrrp_ipv4_multicast_leave - called to leave the VRRP multicast group
 *------------------------------------------------------------------------*/
int
vrrp_ipv4_multicast_leave (int sock,
                           struct pal_in4_addr group,
                           struct pal_in4_addr ifa,
                           u_int32_t ifindex)
{
  result_t ret;

  ret = pal_sock_set_ipv4_multicast_leave (sock, group, ifa, 0);
  if (ret < 0) {
    if (IS_DEBUG_VRRP_EVENT) {
      if (errno == EADDRNOTAVAIL)
        zlog_err (NSM_ZG, "VRRP Warning: IP_DROP_MEMBERSHIP to %r membership doesn't exist", &ifa);
      else
        zlog_err (NSM_ZG, "VRRP Error: can't setsockopt IP_DROP_MEMBERSHIP");
    }
    return 0;
  }
  if (IS_DEBUG_VRRP_EVENT)
    zlog_err (NSM_ZG, "VRRP info: IP_DROP_MEMBERSHIP to %r SUCCESS", &ifa);

  return ret;
}

/*
** Join VRRP multicast group for given interface.
*/
int
vrrp_multicast_join (int sock,
                     struct interface *ifp)
{
  struct connected *ifc = NULL;;

  for (ifc = ifp->ifc_ipv4; ifc; ifc = ifc->next)
  {
    struct prefix_ipv4 *p;
    struct pal_in4_addr group;

    p = (struct prefix_ipv4 *)ifc->address;

    group.s_addr = pal_hton32(VRRP_MCAST_ADDR);
    return vrrp_ipv4_multicast_join (sock, group, p->prefix, ifp->ifindex);
  }
  return -1;
}

/*
** Leave VRRP multicast group for given interface.
*/
int
vrrp_multicast_leave (int sock,
                      struct interface *ifp)
{
  struct connected *ifc;

  for (ifc = ifp->ifc_ipv4; ifc; ifc = ifc->next)
    {
      struct prefix_ipv4 *p;
      struct pal_in4_addr group;

      p = (struct prefix_ipv4 *)ifc->address;

      group.s_addr = pal_hton32(VRRP_MCAST_ADDR);
      vrrp_ipv4_multicast_leave (sock, group, p->prefix, ifp->ifindex);
    }

  return VRRP_OK;
}


/*--------------------------------------------------------------------------
 * vrrp_ip_recv_packet - Fired by thread infrastructure to carry out the
 *                       reception of VRRP advertisement
 *--------------------------------------------------------------------------
 */
int
vrrp_ip_recv_advert (struct thread *thread)
{
  VRRP_GLOBAL_DATA *vrrp = THREAD_ARG (thread);
  struct pal_sockaddr_in4 sin;
  struct vrrp_advt_info advt;
  struct pal_in4_header iph;
  struct interface *ifp;
  s_int16_t vrrp_len;
  VRRP_SESSION *sess;
  int sin_len;
  int ret;

  vrrp->t_vrrp_read = NULL;

  sin_len = sizeof (struct pal_sockaddr_in4);
  /* Peek the packet to get the VRRP packet length from the IP header.  */

  ret = pal_sock_recvfrom (vrrp->sock,
                           (void *)&iph,
                           sizeof (struct pal_in4_header),
                           MSG_PEEK,
                           (struct pal_sockaddr *)&sin,
                           (socklen_t *)&sin_len);

  vrrp->t_vrrp_read = thread_add_read (nzg, vrrp_ip_recv_advert, vrrp, vrrp->sock);

  if (ret < 0)
    zlog_warn (NSM_ZG, "VRRP RECV: recvfrom error: %s", pal_strerror (errno));
  else
  {
    vrrp_len = _get_vrrp_packet_length (&iph);
    if (vrrp_len < VRRP_ADVT_SIZE_V4) {
      zlog_warn (NSM_ZG, "VRRP RECV: Bad VRRP packet length %d - expected %d",
                 vrrp_len, VRRP_ADVT_SIZE);
      return -1;
    }
    _prepare_stream_buffer (vrrp->ibuf, vrrp_len);

    ifp = _recv_advert (vrrp, &iph);
    if (ifp == NULL) {
      zlog_warn (NSM_ZG, "VRRP RECV: Cannot determine the incoming interface");
      return -1;
    }
    sess = _validate_advert (vrrp, &iph, ifp, &sin, &advt);
    if (sess == NULL) {
      return -1;
    }
    /* Call VRRP state machine to handle this event */
    vrrp_handle_advert (sess, &advt, &sin, NULL);
  }
  return 0;
}

#define VRRP_RCV_ADV_CMSG_BUF_LEN  1024
/* VRRP ipv6 advertisement Message Decoder */
#ifdef HAVE_IPV6
static int
_validate_check_sum (VRRP_GLOBAL_DATA *vrrp,
                     u_int32_t vrrp_len,
                     struct pal_in6_addr *vrrpsrc,
                     struct pal_in6_addr *vrrpdst)
{
  struct prefix p_src;
  struct prefix p_dst;
  u_int16_t calc_csum;
  u_int16_t rcvd_csum;
  struct stream *s;
  u_int32_t len;

  /* Get stream buffer */
  s = vrrp->i6buf;

  /* Decode and Reset Checksum field */
  rcvd_csum = pal_ntoh16 (stream_getw_from (s, VRRP_ADVT_CHECKSUM_POS));
  stream_putw_at (s, VRRP_ADVT_CHECKSUM_POS, 0);

  p_src.family = AF_INET6;
  p_src.prefixlen = IPV6_MAX_PREFIXLEN;
  p_src.u.prefix6 = *vrrpsrc;

  p_dst.family = AF_INET6;
  p_dst.prefixlen = IPV6_MAX_PREFIXLEN;
  p_dst.u.prefix6 = *vrrpdst;

  /* Calculate Checksum */
  calc_csum = in6_checksum (&p_src, &p_dst, IPPROTO_VRRP,
                            (u_int16_t *) STREAM_DATA (s), vrrp_len);

  /* Validate Checksum */
  if (calc_csum != rcvd_csum)
    {
      if (IS_DEBUG_VRRP_EVENT)
       zlog_warn (NSM_ZG, "Checksum error. Rcvd=%u Calc=%u,"
                    " Ignoring ipv6 advertisement", rcvd_csum,
                    calc_csum);
     return -1;
    }

  /* Put the Checksum back */
  stream_putw_at (s, VRRP_ADVT_CHECKSUM_POS, pal_hton16 (rcvd_csum));

  return 0;
}

int
vrrp_ipv6_recv_advert (struct thread *thread)
{
  VRRP_GLOBAL_DATA *vrrp = THREAD_ARG (thread);
  char adata[VRRP_RCV_ADV_CMSG_BUF_LEN];
  struct pal_sockaddr_in6 sin6;
  struct pal_in6_pktinfo *pkt;
  struct vrrp_advt_info advt;
  struct pal_msghdr msg;
  struct interface *ifp;
  int vrrp_len, ifindex = 0;
  int ret;
  VRRP_SESSION *sess;
  int *hoplimit;

  vrrp->t_vrrp_read = NULL;

  stream_reset (vrrp->i6buf);
  vrrp_len = pal_in6_recv_packet (vrrp->ipv6_sock, &msg, adata, sizeof (adata),
                             STREAM_DATA (vrrp->i6buf), STREAM_SIZE (vrrp->i6buf),
                             &sin6);

  /* Register the next read thread. */
  vrrp->t_vrrp_read = thread_add_read (nzg, vrrp_ipv6_recv_advert, vrrp,
                                       vrrp->ipv6_sock);
  if (vrrp_len < 0)
    {
     if (IS_DEBUG_VRRP_EVENT)
       zlog_err (NSM_ZG, "OS: Receive error on socket %d: %s",
                 vrrp->sock, pal_strerror (errno));
      return vrrp_len;
    }

  /* Get receiving ifindex. */
  pkt = pal_sock_in6_cmsg_pktinfo_get (&msg);
  if (pkt == NULL)
    {
      if (IS_DEBUG_VRRP_EVENT)
        zlog_warn (NSM_ZG, "VRRP RECV: Unknown interface index\n");
      return -1;
    }

  ifindex = pkt->ipi6_ifindex;

  ifp = if_lookup_by_index (&vrrp->nm->vr->ifm, ifindex);

  if (ifp == NULL)
    {
      if (IS_DEBUG_VRRP_EVENT)
        zlog_warn (NSM_ZG, "VRRP RECV: can n't determine Recv interface\n");
      return -1;
    }

  /* Hoplimit check shold be done when destination address is
         multicast address. */
  if (! IN6_IS_ADDR_MULTICAST (&pkt->ipi6_addr))
    {
      if (IS_DEBUG_VRRP_EVENT)
        zlog_warn (NSM_ZG, "VRRP RECV: Unknown destination address\n");
      return -1;
    }

  hoplimit = pal_sock_in6_cmsg_hoplimit_get (&msg);
  if (hoplimit != NULL)
    {
      if (*hoplimit != VRRP_IP_TTL)
        {
          if (IS_DEBUG_VRRP_EVENT)
            zlog_err (NSM_ZG, "VRRP RECV: packet comes with hop count [%d]"
                      "  but expected value 255 - Discarded\n", *hoplimit);
          return -1;
        }
    }
  else
    {
      if (IS_DEBUG_VRRP_EVENT)
        zlog_err (NSM_ZG, "VRRP RECV: packet received without hoplimit\n");
      return -1;
    }

  if (vrrp_len > 0)
    {
      if (IS_DEBUG_VRRP_EVENT)
        zlog_info (NSM_ZG, "VRRP RECV: on interface %s, from [%R]",
                   ifp->name, &sin6.sin6_addr);

      _prepare_stream_buffer (vrrp->i6buf, vrrp_len);

      /* Validate the check sum for entire IPv6 packet */
      ret  = _validate_check_sum (vrrp, vrrp_len, &sin6.sin6_addr,
                                  &pkt->ipi6_addr);

      if (ret < 0)
        {
          if (IS_DEBUG_VRRP_EVENT)
            zlog_warn (NSM_ZG, "VRRP RECV: Invalid checksum\n");
          return -1;
        }

      sess = _validate_ipv6_advert (vrrp, ifp, &sin6, &advt);
      if (sess == NULL)
        {
          /* Free VIP vector if advt is invalid. */       
          /* if (advt.vip != NULL)                        
              {                                             
                vrrp_vipaddr_vector_free_ipv6 (advt.vip);   
                advt.vip = NULL;                            
              } */                                          

          return -1;
       }

      /* Call VRRP state machine to handle this event */
      vrrp_handle_advert (sess, &advt, NULL, &sin6);
      
      /* Free VIP vector if advt is valid */                           
      /* if (advt.vip != NULL)                                         
        {                                                              
           vrrp_vipaddr_vector_free_ipv6 (advt.vip);                    
           advt.vip = NULL;                                             
         }*/                                                            
                                                                 
      zlog_info (NSM_ZG, "VRRP RECV: ipv6 Advertisement success \n");  

    }
  return 0;
}

#endif /* HAVE_IPV6 */

#ifndef HAVE_CUSTOM1
/*------------------------------------------------------------------------
 * vrrp_send_advert - Send an advertisement message.  This function
 *              creates and sends a VRRP advertisement message for a
 *              particular session.  It utilizes the API calls
 *              provided in the vrrp_ip module.
 *------------------------------------------------------------------------*/
int
vrrp_ip_send_advert (VRRP_SESSION *sess, u_int8_t prio)
{
  struct stream         *s = NULL;
  struct nsm_if         *nsm_if;
  u_int16_t             cksum;

  nsm_if = sess->ifp->info; 
  cksum  = 0;

  /* allocate space for advt */
  if (sess->af_type == AF_INET)
    s = stream_new (VRRP_ADVT_SIZE_V4);
#ifdef HAVE_IPV6
  else
    s = stream_new (VRRP_ADVT_SIZE_V6);
#endif /* HAVE_IPV6 */

  /* Fill in the advertisement message prior to filling in the stream */
  /* This is necessary to perform the checksum calculation */
  if (sess->af_type == AF_INET)
    stream_putc (s, ((VRRP_VERSION << 4) | VRRP_ADVT_TYPE));
  else
    stream_putc (s, ((VRRP_IPV6_VERSION << 4) | VRRP_ADVT_TYPE));

  stream_putc (s, sess->vrid);
  stream_putc (s, prio);

  stream_putc (s, sess->num_ip_addrs);
  /* Authentication-type is only for ipv4 */
  if (sess->af_type == AF_INET)
    {
      stream_putc (s, VRRP_AUTH_NONE);
      stream_putc (s, sess->adv_int);
    }
  else
    {
     stream_putc (s,  0);
     stream_putc (s, sess->adv_int);
    }

  stream_putw (s, 0);

  if (sess->af_type == AF_INET)
    {
      stream_putl (s, pal_hton32 (sess->vip_v4.s_addr));

      /* Clear authentication data field. */
      stream_putl (s, 0);
      stream_putl (s, 0);
                                                                               
      /* Calculate checksum */                                             
      cksum = in_checksum ((u_int16_t *)STREAM_DATA(s), VRRP_ADVT_SIZE_V4);
    }
#ifdef HAVE_IPV6
  else
    {
      stream_put_in6_addr (s, &sess->vip_v6);
                                                                                
      /* Calculate checksum                                                    
      cksum = in6_checksum ((u_int16_t *)STREAM_DATA(s), VRRP_ADVT_SIZE_V6); */
    }
#endif /* HAVE_IPV6 */

  /* Update checksum for IPv4 */
  if (sess->af_type == AF_INET)
    stream_putw_at (s, VRRP_ADVT_CHECKSUM_POS, pal_hton16 (cksum)); 

  /* Call the appropriate IP interaction routine to transmit the message */
    /* Call the function to send the advertisement message */

  if (sess->af_type == AF_INET)
    {
      if (_send_advert (sess->ifp, sess->vrrp->sock, s, VRRP_ADVT_SIZE_V4,
                      sess->af_type) != VRRP_OK)
        {
          if (IS_DEBUG_VRRP_SEND)
            zlog_warn (NSM_ZG, "VRRP SEND[ADV]: Error Sending VRRP ipv4 Advertisement\n");

          stream_free (s);
          return VRRP_FAILURE;
        }

      if (IS_DEBUG_VRRP_SEND)
        zlog_info (NSM_ZG, "VRRP IPv4: Advertisement sent for vrid=[%d], [%r]\n", sess->vrid, &sess->vip_v4.s_addr);
     }
#ifdef HAVE_IPV6
   else
     {
       if (_send_advert (sess->ifp, sess->vrrp->ipv6_sock, s, VRRP_ADVT_SIZE_V6,
                         sess->af_type) != VRRP_OK)
         {
           if (IS_DEBUG_VRRP_SEND)
             zlog_warn (NSM_ZG, "VRRP SEND[ADV]: Error Sending VRRP ipv6 Advertisement\n");

           stream_free (s);
           return VRRP_FAILURE;
         }

      if (IS_DEBUG_VRRP_SEND)
        zlog_info (NSM_ZG, "VRRP IPv6: Advertisement sent for %s - vrid=[%d]"
                   " %R SUCCESS", sess->ifp->name, sess->vrid, &sess->vip_v6);
     }
#endif /* HAVE_IPV6 */
  /* Now, free the packet. */
  stream_free (s);

  if (prio == 0)
    sess->stats_pri_zero_pkts_sent++;

  return VRRP_OK;
}
#endif /* HAVE_CUSTOM1 */



/*------------------------------------------------------------------------
 * vrrp_ip_send_arp - Send an ARP message (via the FE).  This function
 *               creates (if necessary) and sends a VRRP advertisement
 *               message for a particular session.  Depending on the
 *               implementation, this may be performed inside the FE
 *               itself, in which case, this function will effectively
 *               do nothing.
 *------------------------------------------------------------------------*/
int
vrrp_ip_send_arp (VRRP_SESSION *sess)
{
  struct stream *ap;
  u_int16_t eth_hw_type;
  u_int16_t arp_type;
  u_int16_t ip_type;
  u_int8_t hw_len;
  u_int8_t ip_len;
  u_int16_t op;
  int i;
  int ret;

  /* Assign values */
  eth_hw_type = 1;      /* Hartdare type = Ethernet */
  arp_type = 0x0806;    /* Protocol type = ARP */
  ip_type = 0x0800;     /* IP type */
  hw_len = 6;           /* hardware address length */
  ip_len = 4;           /* protocol address length */
  op = 1;               /* Request */

  /* Build the Gratuitous ARP packet */
  ap = stream_new (SIZE_ARP_PACKET);

  /* Put the Ethernet header first */
  for (i=0; i<6; i++)
    stream_putc (ap, 0xFF);

  if (vrrp_fe_get_vmac_status (NSM_ZG))
    for (i=0; i<6; i++)
      stream_putc (ap, sess->vmac.v.mbyte[i]);
  else
    for (i = 0; i < 6; i++)
      stream_putc (ap, sess->ifp->hw_addr[i]);

  stream_putw (ap, arp_type);

  /* Fill in the ARP packet */
  stream_putw (ap, eth_hw_type);
  stream_putw (ap, ip_type);
  stream_putc (ap, hw_len);
  stream_putc (ap, ip_len);
  stream_putw (ap, op);

  if (vrrp_fe_get_vmac_status (NSM_ZG))
    for (i=0; i<6; i++)
      stream_putc (ap, sess->vmac.v.mbyte[i]);
  else
    for (i = 0; i < 6; i++)
      stream_putc (ap, sess->ifp->hw_addr[i]);

  stream_putl (ap, pal_hton32 (sess->vip_v4.s_addr));
  for (i=0; i<6; i++)
    {
      stream_putc (ap, 0);
    }
  stream_putl (ap, pal_hton32 (sess->vip_v4.s_addr));

  /* put the padding */
  for (i=0; i<18; i++)
    stream_putc (ap, 0);

  if (IS_DEBUG_VRRP_SEND)
    zlog_warn (NSM_ZG, "VRRP SEND[GARP]: Gratuitous ARP sent for vrid=%d\n",
               sess->vrid);

  /* Send the packet */
  ret = vrrp_fe_tx_grat_arp (ap, sess->ifp);

  /* Now, free the packet. */
  stream_free (ap);

  return ret;
}

int
vrrp_ip_start_master(VRRP_SESSION *sess)
{
  /* Need to transmit an advertisement, since you're now the Master.  */
  if (vrrp_ip_send_advert (sess, sess->prio) != VRRP_OK) {
    if (IS_DEBUG_VRRP_SEND)
      zlog_warn (NSM_ZG, "VRRP SEND[Hello]: Error transmitting VRRP advertisement\n");
    return VRRP_FAILURE;
  }

  /* Need to transmit a gratuitous ARP message so that other hosts    */
  /* can update their tables.                                         */
  if (sess->af_type == AF_INET)
    if (vrrp_ip_send_arp (sess) != VRRP_OK)
      {
        if (IS_DEBUG_VRRP_SEND)
          zlog_warn (NSM_ZG, "VRRP SEND[ARP]: Error transmitting Gratuitous ARP\n");
        return VRRP_FAILURE;
      }
#ifdef HAVE_IPV6
  if (sess->af_type == AF_INET6)
    if (vrrp_ipv6_send_nd_nbadvt (sess) != VRRP_OK)
      {
        if (IS_DEBUG_VRRP_SEND)
          zlog_warn (NSM_ZG, "VRRP SEND[ARP]: Error Tx-ing ND-Neighbor Advert \n");
        return VRRP_FAILURE;
      }
#endif /* HAVE_IPV6 */
  /* When in Master state, the countdown timer associated with the      */
  /* session represents the Advertisement interval timer.               */
  sess->timer = sess->adv_int;
  return VRRP_OK;
}

/**************************************************************************
 *                          LOCAL FUNCTIONS
 **************************************************************************
 */

/*------------------------------------------------------------------------
 * _make_socket - called to create the IP socket for tx/rx of
 *                vrrp advertisement messages
 *------------------------------------------------------------------------*/
static int
_make_socket ()
{
  int sock, ttl;
  result_t ret;
  s_int32_t on  = 1;

  sock = pal_sock (NSM_ZG, AF_INET, SOCK_RAW, IPPROTO_VRRP);
  if (sock < 0) {
    zlog_err (NSM_ZG, "socket");
    return VRRP_FAILURE;
  }
  ttl = 255;
  ret = pal_sock_set_ipv4_multicast_hops (sock, ttl);
  if (ret != RESULT_OK)
  {
    zlog_err (NSM_ZG, "VRRP Error: setting ipv4 multicast hops failed: %s\n",
              pal_strerror (errno));
    return -1;
  }
  /* We need to know the interface on which the packet is coming. */
  ret = pal_sock_set_ip_recvif (sock, on);
  if (ret < 0) {
    zlog_err (NSM_ZG,  "VRRP Error: setting \"recvif\" socket option \"on\" failed: %s\n",
              pal_strerror (errno));
    return -1;
  }
  return sock;
}

#ifdef HAVE_IPV6
int vrrp_ipv6_send_nd_nbadvt (VRRP_SESSION *sess)
{
  struct interface *ifp = sess->ifp;
  u_char flags = 0xA0;
  struct stream *s;
  int i, ret;

  /* Only allow the address which length + 2 is the multiply of eight. */
  if ((ifp->hw_addr_len + 2) % 8)
    return -1;

  /* Build the Gratuitous ARP packet */
  s = stream_new (SIZE_ND_ADV_PACKET);

  /* Type. */
  stream_putc (s, PAL_ND_NEIGHBOR_ADVERT);

  /* Code. */
  stream_putc (s, 0);

  /* Checksum. */
  stream_putw (s, 0);

  /* Set R, Override flags and Unset solicited flag */
  stream_putc (s, flags);

  /* Reserved */
  stream_putc (s, 0);
  stream_putw (s, 0);

  /* Target address */
  stream_put_in6_addr (s, &sess->vip_v6);

  /* Type. */
  stream_putc (s, PAL_ND_OPT_TARGET_LINKADDR);

  /* Length. */
  stream_putc (s, (ifp->hw_addr_len + 2) >> 3);


  if (vrrp_fe_get_vmac_status (NSM_ZG) && sess->owner == PAL_FALSE)
    for (i=0; i<6; i++)
      stream_putc (s, sess->vmac.v.mbyte[i]);
  else
    for (i = 0; i < 6; i++)
      stream_putc (s, sess->ifp->hw_addr[i]);


  if (IS_DEBUG_VRRP_SEND)
    zlog_warn (NSM_ZG, "VRRP SEND: Unsolicited ND advertisement sent for vrid=%d\n",
               sess->vrid);

  /* Send the packet */
  ret = vrrp_fe_tx_nd_advt (s, sess->ifp);

  /* Now, free the packet. */
  stream_free (s);

  return ret;

}
/*------------------------------------------------------------------------
 * _make_ipv6_socket - called to create the IPv6 socket for tx/rx of
 *                vrrp advertisement messages
 *------------------------------------------------------------------------*/
static int
_make_ipv6_socket ()
{
  pal_sock_handle_t sock;
  struct nsm_master *nm;
  struct apn_vrf *vrf;
  result_t ret;
  s_int32_t on  = 1;

  sock = pal_sock (NSM_ZG, AF_INET6, SOCK_RAW, IPPROTO_VRRP);
  if (sock < 0) {
    zlog_err (NSM_ZG, "socket");
    return VRRP_FAILURE;
  }

  ret = pal_sock_set_reuseaddr (sock, on);
  if (ret != RESULT_OK)
    {
      zlog_err (NSM_ZG, "VRRP Error: setting SO_REUSEADDR failed: %s\n",
                pal_strerror (errno));
      return -1;
    }

  ret = pal_sock_set_ipv6_multicast_hops (sock, 255);
  if (ret != RESULT_OK)
  {
    zlog_err (NSM_ZG, "VRRP Error: setting ipv6 multicast hops failed: %s\n",
              pal_strerror (errno));
    return -1;
  }

  ret = pal_sock_set_ipv6_pktinfo (sock, on);
  if (ret < 0)
    return ret;

  ret = pal_sock_set_ipv6_hoplimit (sock, on);
  if (ret < 0)
    {
      zlog_err (NSM_ZG, "VRRP Error: Can't set hoplimit: %s\n",
                pal_strerror (errno));
      return ret;
    }

  ret = pal_sock_set_ipv6_checksum (sock, VRRP_IPV6_CHECKSUM_OFFSET);
  if (ret != RESULT_OK)
    {
      zlog_err (NSM_ZG, "VRRP Error: setting SO_CHECKSUM failed: %s\n",
                pal_strerror (errno));
      return -1;
    }

  nm = nsm_master_lookup_by_id (NSM_ZG, 0);
  if (nm == NULL)
    return -1;

  vrf = apn_vrf_lookup_default (nm->vr);
  if (vrf != NULL)
    if (pal_sock_set_bindtofib (sock, vrf->fib_id) < 0)
      zlog_warn (NSM_ZG, "OS: can't bind socket to FIB(%d): %s",
                 vrf->fib_id, pal_strerror (errno));

  return sock;

}
#endif /* HAVE_IPV6 */


/*------------------------------------------------------------------------
 * vrrp_ip_send_advt - called to communicate with the interface/socket to
 *                      send the vrrp advertisements
 *------------------------------------------------------------------------*/
static int
_send_advert (struct interface *ifp, int sock, struct stream *s, int size,
              u_int8_t  af_type)
{
  struct pal_sockaddr_in4       sin;
  struct pal_in4_addr           addr;
#ifdef HAVE_IPV6
  struct pal_sockaddr_in6       sin6;
  struct pal_in6_addr           addr6;
  struct pal_msghdr             msg;
#endif /* HAVE_IPV6 */
  struct connected              *ifc;
  int                           nbytes;
  int                           found = 0;
  result_t                      ret = 0;
  struct prefix                 *p;

  if (af_type == AF_INET)
    {
      pal_mem_set (&addr, 0, sizeof (struct pal_in4_addr));

      /* Prevent receiving self-originated multicast packets  (FreeBSD only now) */
      ret = pal_sock_set_ipv4_multicast_loop (sock, PAL_FALSE);
      if (ret != RESULT_OK)
        zlog_err (NSM_ZG, "VRRP Error: can't setsockopt ipv4 multicast loop off: %s",
                  pal_strerror (errno));

      /* Make destination address. */
      pal_mem_set (&sin, 0, sizeof(struct pal_sockaddr_in4));

      sin.sin_addr.s_addr = pal_hton32 (VRRP_MCAST_ADDR);
      sin.sin_family = AF_INET;
    }
#ifdef HAVE_IPV6
  else  /* AF_INET6 */
    {
      pal_mem_set (&addr6, 0, sizeof (struct pal_in6_addr));

      ret = pal_sock_set_ipv6_multicast_loop (sock, 0);
      if (ret != RESULT_OK)
        zlog_err (NSM_ZG, "VRRP Error: Can't setsockopt IPV6_MULTICAST_LOOP: %s",
                  pal_strerror (errno));

      /* Set destination ipv6 address. */
      pal_mem_set (&sin6, 0, sizeof (struct pal_sockaddr_in6));
      sin6.sin6_family = AF_INET6;
      ret = pal_inet_pton (AF_INET6, VRRP_MCAST_ADDR6, &sin6.sin6_addr);
      if (ret <= 0)
        return VRRP_FAILURE;
    }
#endif /* HAVE_IPV6 */

  /* Get connected interface */
  if (af_type == AF_INET)
    {
      for (ifc = ifp->ifc_ipv4; ifc; ifc = ifc->next)
        {
           p = ifc->address;

           if (! CHECK_FLAG (ifc->flags, NSM_IFA_VIRTUAL))
             {
               /* Set the socket address */
               addr = p->u.prefix4;

               found = 1;
               break;
             }
        }
    }
#ifdef HAVE_IPV6
  else /* AF_INET6 */
    {
      for (ifc = ifp->ifc_ipv6; ifc; ifc = ifc->next)
        {
          /* Check for link local address. */
          if (!IN6_IS_ADDR_LOOPBACK (&ifc->address->u.prefix6)
              && IN6_IS_ADDR_LINKLOCAL (&ifc->address->u.prefix6))
            {
              p = ifc->address;

              if (! CHECK_FLAG (ifc->flags, NSM_IFA_VIRTUAL))
                  /* Set the socket address */
                {
                  addr6 = p->u.prefix6;

                  found = 1;
                  break;
                }
            }
        }
    }
#endif /* HAVE_IPV6 */

  if (!found)
    {
      zlog_warn (NSM_ZG, "VRRP Warn: No ip found; Can't send advertisement on %s\n",
                 ifp->name);
      return VRRP_FAILURE;
    }

  /* Set Interface as multicast. */
  if (af_type == AF_INET)
    ret = pal_sock_set_ipv4_multicast_if (sock, addr, ifp->ifindex);
#ifdef HAVE_IPV6
  else
    ret = pal_sock_set_ipv6_multicast_if (sock, ifp->ifindex);
#endif /* HAVE_IPV6 */

  if (ret < 0)
    {
      zlog_err (NSM_ZG, "VRRP Error: Can't set interface as multicast when sending packet\n");
    }

  /* Send the advertisement message */
  if (af_type == AF_INET)
    {
      nbytes = pal_sock_sendto (sock, s->data, size, 0,
                                (struct pal_sockaddr *) &sin,
                                sizeof (struct pal_sockaddr_in4));
      if (nbytes != size)
        {
          zlog_err (NSM_ZG, "VRRP Error: 'ipv4 sendto' failed");
          return (VRRP_FAILURE);
        }
    }
#ifdef HAVE_IPV6
  else
    {
      pal_sock_in6_cmsg_init (&msg, PAL_CMSG_IPV6_PKTINFO);
      pal_sock_in6_cmsg_pktinfo_set (&msg, NULL, &addr6, ifp->ifindex);

      ret = pal_in6_send_packet (sock, &msg, STREAM_DATA (s),
                                 size, &sin6);
      if (ret < 0 )
        {
          zlog_err (NSM_ZG, "VRRP Error: 'ipv6 sendto' %s failed ",
                    pal_strerror (errno));
          return (VRRP_FAILURE);
        }

      pal_sock_in6_cmsg_finish (&msg);

    }
#endif /* HAVE_IPV6 */

  return (VRRP_OK);
}

/*-------------------------------------------------------------------------
 * _prepare_stream_buffer - Prepare the stream buffer for the packet reception
 *-------------------------------------------------------------------------
 */
static void
_prepare_stream_buffer (struct stream *s, size_t size)
{
  if (s->size < size)
  {
    s->data = XREALLOC (MTYPE_STREAM_DATA, s->data, size+4);
    s->size = size+4;
  }
  else
    pal_mem_set (s->data, 0, s->size);

  stream_reset (s);
  stream_set_endp (s, size);
}

/*-------------------------------------------------------------------------
 * _get_vrrp_packet_lengtht - Find out the length of the VRRP packet
 *-------------------------------------------------------------------------
 */
static s_int16_t
_get_vrrp_packet_length (struct pal_in4_header *iph)
{
  s_int32_t length = 0;

  /* Get the IP packet length.  */
  pal_in4_ip_packet_len_get (iph, ( u_int32_t *)&length);

  /* And subtract the IP header length.  */
  return (length - (iph->ip_hl << 2));
}


/*-------------------------------------------------------------------------
 * _recv_advert - Receive the IPv4 header, VRRP advertisement and ifindex
 *                using the recvmsg() socket function.
 *-------------------------------------------------------------------------
 */
static struct interface *
_recv_advert (VRRP_GLOBAL_DATA *vrrp,
              struct pal_in4_header *iph)
{
  struct nsm_master *nm = vrrp->nm;
  struct stream *s = vrrp->ibuf;
  struct pal_iovec iov[2];
  struct interface *ifp = NULL;
  struct pal_msghdr msg;
  int ifindex;
  int ret;

  /* Allocate CMSG for PKTINFO/RECVIF.  */
  pal_sock_in4_cmsg_init (&msg, PAL_CMSG_IPV4_PKTINFO|PAL_CMSG_IPV4_RECVIF);

  /* Prepare the buffer.  */
  iov[0].iov_base = (void *)iph;
  iov[0].iov_len  = sizeof (struct pal_in4_header);
  iov[1].iov_base = (void *)STREAM_DATA (s);
  iov[1].iov_len  = stream_get_endp (s);

  /* Prepare the message header.  */
  msg.msg_iov = iov;
  msg.msg_iovlen = 2;

  /* Receive the packet.  */
  ret = pal_sock_recvmsg (vrrp->sock, &msg, 0);
  if (ret < 0)
  {
    pal_sock_in4_cmsg_finish (&msg);
    zlog_warn (NSM_ZG, "VRRP RECV: recvmsg error: %s", pal_strerror (errno));
    return NULL;
  }

  /* Lookup the receving interface.  */
  ifindex = pal_sock_in4_cmsg_ifindex_get (&msg);
  if (ifindex != -1)
    ifp = if_lookup_by_index (&nm->vr->ifm, ifindex);
/*
  else
    ifp = if_match_by_ipv4_address (&om->vr->ifm, &iph->ip_src,
                                    top->ov->iv->id);
*/
  /* Free CMSG.  */
  pal_sock_in4_cmsg_finish (&msg);

  return ifp;
}

/*------------------------------------------------------------------------
 * _validate_packet - This function preprocess the received advert packet,
 *                    validates its field and decodes it to the
 *                    host format.
 *                    It returns a pointer to the VRRP session.
 *------------------------------------------------------------------------
 */
static VRRP_SESSION *
_validate_advert (VRRP_GLOBAL_DATA      *vrrp,
                  struct pal_in4_header *iph,
                  struct interface      *ifp,
                  struct pal_sockaddr_in4 *from,
                  struct vrrp_advt_info *advt)
{
  struct nsm_master *nm = vrrp->nm;
  struct vrrp_advt_info *vrrph = NULL;
  u_int16_t cksum;
  VRRP_SESSION *sess = NULL;
  u_int32_t length = 0;
  struct nsm_if *nsm_if = NULL;
  struct stream *ibuf = vrrp->ibuf;
  int vrid;
  u_int32_t vrrp_len = 0;

  /* Retrieve IP packet length based on header (varies on different OS). */
  pal_in4_ip_packet_len_get (iph, &length);

  /* validate packet length */
  vrrp_len = length - (iph->ip_hl << 2);
  if (vrrp_len < 2)
  {
    if (IS_DEBUG_VRRP_RECV)
      zlog_warn (NSM_ZG, "VRRP RECV[Hello]: Dropped - too short");
    return NULL;
  }

  vrrph = (struct vrrp_advt_info *) STREAM_PNT (vrrp->ibuf);

  /* Get VRRP information out of the message */
  advt->ver_type = stream_getc(ibuf);
  advt->vrid     = stream_getc(ibuf);

  sess = vrrp_api_lookup_session (nm->vr->id, AF_INET, advt->vrid, ifp->ifindex);
  /* See if I handle this VRID. */
  if (sess == NULL) {
    if (IS_DEBUG_VRRP_RECV)
      zlog_warn (NSM_ZG, "VRRP RECV[Hello]: Dropped - non existent session (%d/%d/%s)",
                 AF_INET, advt->vrid, ifp->name);
    vrrp_stats_invalid_vrid++;
    return NULL;
  }
  vrid = sess->vrid;

  /* if the session is in the initialize state, ignore it */
  if (sess->state == VRRP_STATE_INIT)
  {
    if (IS_DEBUG_VRRP_RECV)
      zlog_warn (NSM_ZG, "VRRP RECV[Hello]: Dropped - session in INIT state (%d/%d/%s)",
                 AF_INET, vrid, ifp->name);
    return NULL;
  }

  sess->stats_advt_rcvd++;

  /* validate the IP TTL - MUST be 255 */
  if (iph->ip_ttl != VRRP_IP_TTL)
    {
      if (IS_DEBUG_VRRP_RECV)
        zlog_warn (NSM_ZG, "VRRP RECV[Hello]: Dropped - invalid TTL value %d (%d/%d/%s)",
                   iph->ip_ttl, AF_INET, vrid, ifp->name);
      sess->stats_ip_ttl_errors++;
      return NULL;
    }

  /* validate VRRP version */
  if ((advt->ver_type >> 4) != VRRP_VERSION)
    {
      if (IS_DEBUG_VRRP_RECV)
        zlog_warn (NSM_ZG, "VRRP RECV[Hello]: Dropped - bad version %d (%d/%d/%s)",
                   (advt->ver_type >> 4), AF_INET, vrid, ifp->name);
      vrrp_stats_version_err++;
      return NULL;
    }

  /* validate VRRP type field. */
  if ((advt->ver_type & 0x0f) != VRRP_ADVT_TYPE)
    {
      if (IS_DEBUG_VRRP_RECV)
        zlog_warn (NSM_ZG, "VRRP RECV[Hello]: Dropped - bad type %d (%d/%d/%s)",
                   (advt->ver_type & 0x0F), AF_INET, vrid, ifp->name);
      sess->stats_invalid_type_pkts_rcvd++;
      return NULL;
    }
  advt->priority     = stream_getc(ibuf);                                                                
  advt->num_ip_addrs = stream_getc(ibuf);                                                                
                                                                                                        
  /*  MUST verify that the received packet contains the complete VRRP                                    
     packet (including fixed fields, IP Address(es), and Authentication                                 
     Data). */                                                                                          
  if (vrrp_len != VRRP_ADVT_V4_TOTAL_LEN(advt->num_ip_addrs))                                            
   {                                                                                                    
     if (IS_DEBUG_VRRP_RECV)                                                                            
       zlog_warn (NSM_ZG, "VRRP RECV[Advt]: Dropped - packet length incorrect for session %d/%d/%s",    
                  AF_INET, vrid, ifp->name);                                                            
                                                                                                        
     sess->stats_pkt_len_errors++;                                                                      
     return NULL;                                                                                       
   }                                                                                                    
                                                                                                        
  advt->auth_type = stream_getc(ibuf);                                                                   
  advt->advt_int  = stream_getc(ibuf);                                                                   
                                                                                                        
  /* Get checksum. */                                                                                    
  cksum = pal_ntoh16 (stream_getw(ibuf));                                                                
                                                                                                        
  /* validate VRRP checksum */
  advt->cksum = 0;
  stream_putw_at (ibuf, VRRP_ADVT_CHECKSUM_POS, 0);
  advt->cksum = in_checksum ((u_int16_t *)STREAM_DATA(ibuf), vrrp_len);

  if (advt->cksum  != cksum)
    {
      if (IS_DEBUG_VRRP_RECV)
        zlog_warn (NSM_ZG, "VRRP RECV[Hello]: Dropped - bad checksum %d (%d/%d/%s)",
                  (advt->ver_type & 0x0F), AF_INET, vrid, ifp->name);
      vrrp_stats_checksum_err++;
      return NULL;
    }

  /* Get rest of packet. */                                                    
                                                                              
  /* Primary Virtual IP received (1st VIP in the advertizement). */            
  advt->vip_v4.s_addr = pal_ntoh32 (stream_getl(ibuf));                        
                                                                              
  advt->vip = NULL;                                                            
                                                                              
  /*  MAY verify that "Count IP Addrs" and the list of IP Address              
     matches the IP_Addresses configured for the VRID  */                     
  if (advt->num_ip_addrs > 1)                                                  
   {                                                                          
     /* fetch the secondary VIPs from stream to vector */                     
     /* vrrp_vipaddr_vector_get_ipv4 (advt, ibuf);  */                        
                                                                              
     /* Forward the stream to auth data. */                                   
     vrrp_len = (sizeof (struct pal_in4_addr)) * ((advt->num_ip_addrs) - 1);  
     STREAM_FORWARD_GETP (ibuf, vrrp_len);                                    
   }                                                                          


  /* RFC 3768 obsoletes any VRRP authentication - check only limits */
  if (advt->auth_type > VRRP_AUTH_RES_2)
    {
      if (IS_DEBUG_VRRP_RECV)
        zlog_warn (NSM_ZG, "VRRP RECV[Advt]: Bad authentication type %d (%d/%d/%s)",
                   advt->auth_type & 0x0F, AF_INET, vrid, ifp->name);
      sess->stats_invalid_auth_type++;
      return NULL;
    }

  /* NOTE: We do not validate authentication data */
  advt->auth_data[0] = pal_ntoh32 (stream_getl(ibuf));  
  advt->auth_data[1] = pal_ntoh32 (stream_getl(ibuf));  
                                                      
  nsm_if = sess->ifp->info;                             

  /* Check the advert arrived on the expected interface. */
  ifp = if_match_by_ipv4_address (&nm->vr->ifm, &iph->ip_src, VRF_ID_UNSPEC);
  if (ifp)
    {
      if (ifp != sess->ifp)
        {
          if (IS_DEBUG_VRRP_RECV)
            zlog_warn (NSM_ZG, "VRRP RECV[Advt]: Received in wrong interface (vrid=%d)", vrid);
          return NULL;
        }
    }

  /* NOTE : We verify only the primary Virtual IP */  
                                                     

  /* validate IP address associated with vrid */
  if (advt->vip_v4.s_addr != sess->vip_v4.s_addr)
    {
      /* If advertisement is generated by owner for the same VRID,
         the packet is considered to be valid. But this results in
         packet loss if the owner of the configured VIP (master)  
         becomes unavailable.*/                                   
      if (advt->priority == VRRP_MAX_OPER_PRIORITY)
        {
          if (IS_DEBUG_VRRP_RECV)
            zlog_warn (NSM_ZG, "VRRP RECV[Advt]: IPv4 Packet is advertised"
                       "by Owner for (vrid=%d) (vip=[%r]) different from ours [%r]",
                       vrid, &advt->vip_v4.s_addr, &sess->vip_v4.s_addr);
          return sess;
        }

      if (IS_DEBUG_VRRP_RECV)
        zlog_warn (NSM_ZG, "VRRP RECV[Advt]: Bad virtual IP address [%r] for" 
           "(vrid=%d)", &advt->vip_v4.s_addr, vrid);                  
      sess->stats_addr_list_errors++;
      return NULL;
    }

  /* validate advertisement interval */
  if (advt->advt_int != sess->adv_int)
  {
    if (IS_DEBUG_VRRP_RECV)
      zlog_warn (NSM_ZG, "VRRP RECV[Advt]: Advertisement Interval incorrect (vrid=%d)", vrid);
    sess->stats_advt_int_errors++;
    return NULL;
  }

  if (IS_DEBUG_VRRP_RECV)
    zlog_info (NSM_ZG, "VRRP RECV[Advt]: Advertisement OK - vrid=[%d], vip=[%r] from %r\n",
           vrid, &sess->vip_v4.s_addr, &iph->ip_src.s_addr);
                           
  if (advt->priority == 0)            
    sess->stats_pri_zero_pkts_rcvd++; 


  return sess;   /* We like this packet. */
}

/*------------------------------------------------------------------------
 * _validate_ipv6_advert - This function preprocess the received ipv6 advert
 *                    packet, validates its field and decodes it to the
 *                    host format.
 *                    It returns a pointer to the VRRP session.
 *------------------------------------------------------------------------
 */
#ifdef HAVE_IPV6
static VRRP_SESSION *
_validate_ipv6_advert (VRRP_GLOBAL_DATA      *vrrp,
                       struct interface      *ifp,
                       struct pal_sockaddr_in6 *from,
                       struct vrrp_advt_info *advt)
{

  struct stream *i6buf = vrrp->i6buf;
  struct nsm_master *nm = vrrp->nm;
  struct nsm_if *nsm_if = NULL;
  VRRP_SESSION *sess = NULL;
  int vrid;
  u_int32_t fwd_getp;

  /* Get VRRP information out of the message */
  advt->ver_type = stream_getc(i6buf);
  advt->vrid     = stream_getc(i6buf);

  sess = vrrp_api_lookup_session (nm->vr->id, AF_INET6, advt->vrid, ifp->ifindex);
  /* See if I handle this VRID. */
  if (sess == NULL) {
    if (IS_DEBUG_VRRP_RECV)
      zlog_warn (NSM_ZG, "VRRP RECV[Hello]: Dropped - non existent session (%d/%d/%s)",
                 AF_INET6, advt->vrid, ifp->name);
    vrrp_stats_invalid_vrid++;
    return NULL;
  }
  vrid = sess->vrid;

  /* if the session is in the initialize state, ignore it */
  if (sess->state == VRRP_STATE_INIT)
  {
    if (IS_DEBUG_VRRP_RECV)
      zlog_warn (NSM_ZG, "VRRP RECV[Hello]: Dropped - session in INIT state (%d/%d/%s)",
                 AF_INET6, vrid, ifp->name);
    return NULL;
  }

  /* validate VRRP version */
  if ((advt->ver_type >> 4) != VRRP_IPV6_VERSION)
    {
      if (IS_DEBUG_VRRP_RECV)
        zlog_warn (NSM_ZG, "VRRP RECV[Hello]: Dropped - bad version %d (%d/%d/%s)",
                   advt->ver_type, AF_INET6, vrid, ifp->name);
      vrrp_stats_version_err++;
      return NULL;
    }

  /* validate VRRP type field. */
  if ((advt->ver_type & 0x0f) != VRRP_ADVT_TYPE)
    {
      if (IS_DEBUG_VRRP_RECV)
        zlog_warn (NSM_ZG, "VRRP RECV[Hello]: Dropped - bad type %d (%d/%d/%s)",
                   advt->ver_type & 0x0F, AF_INET6, vrid, ifp->name);
      sess->stats_invalid_type_pkts_rcvd++;
      return NULL;
    }

  sess->stats_advt_rcvd++;

  advt->priority     = stream_getc(i6buf);
  advt->num_ip_addrs = stream_getc(i6buf);
  advt->auth_type    = stream_getc(i6buf);
  advt->advt_int     = stream_getc(i6buf);

  if (advt->priority == 0)           
    sess->stats_pri_zero_pkts_rcvd++;

  /* Forward stream as checksum is validated before. */
  stream_forward (i6buf, sizeof (u_int16_t));

  /* Get rest of packet. */
  advt->vip_v6      = stream_get_ipv6(i6buf);
                                                                                 
  advt->vip = NULL;                                                            
                                                                               
  /*  MAY verify that the IPv6 Address matches the IPv6_Address                
      configured for the VRID. */                                              
  if (advt->num_ip_addrs > 1)                                                  
    {                                                                          
      /* Get the remaining virtual IP addresses. */                            
     /* vrrp_vipaddr_vector_get_ipv6 (advt, i6buf); */                         
                                                                               
      /* Forward the stream to auth data. */                                   
      fwd_getp = (sizeof (struct pal_in6_addr)) * ((advt->num_ip_addrs) - 1);  
      STREAM_FORWARD_GETP (i6buf, fwd_getp);                                   
    }                                                                           

  nsm_if = sess->ifp->info;

  /* validate IP address associated with vrid */
  if (! IPV6_ADDR_SAME (&advt->vip_v6, &sess->vip_v6))
    {
      /* If advertisement is generated by owner for the same VRID,
         accept the packet */
      if (advt->priority == VRRP_MAX_OPER_PRIORITY)
        {
          if (IS_DEBUG_VRRP_RECV)
            zlog_warn (NSM_ZG, "VRRP RECV[Advt]: IPv6 Packet is advertised"
                       " by Owner for (vrid=%d)", vrid);
          return sess;
        }

      if (IS_DEBUG_VRRP_RECV)
        zlog_warn (NSM_ZG, "VRRP RECV[ADV]: Bad virtual IPV6 address (vrid=%d)", vrid);
      sess->stats_addr_list_errors++;
      return NULL;
    }

  /* validate advertisement interval */
  if (advt->advt_int != sess->adv_int)
  {
    if (IS_DEBUG_VRRP_RECV)
      zlog_warn (NSM_ZG, "VRRP RECV[ADV]: Advertisement Interval incorrect (vrid=%d)", vrid);
    sess->stats_advt_int_errors++;
    return NULL;
  }

  if (IS_DEBUG_VRRP_RECV)
    zlog_info (NSM_ZG, "VRRP RECV[ADV]: Advertisement OK - vrid=[%d], vip6=[%R]\n",
               vrid, &sess->vip_v6);

  return sess;   /* We like this packet. */
}
#endif /* HAVE_IPV6 */
