/* Copyright (C) 2002-2003  All Rights Reserved. */

#include "pal.h"
#include "pal_socket.h"
#include "pal_sock_raw.h"

static size_t
pal_sock_in4_cmsg_len_get (u_int32_t flags)
{
  size_t size = 0;

#ifdef IP_PKTINFO
  if (CHECK_FLAG (flags, PAL_CMSG_IPV4_PKTINFO))
    size += PAL_CMSG_SPACE (sizeof (struct in_pktinfo));
#endif /* IP_PKTINFO */
#ifdef IP_RECVIF
  if (CHECK_FLAG (flags, PAL_CMSG_IPV4_RECVIF))
    size += PAL_CMSG_SPACE (sizeof (struct sockaddr_dl));
#endif /* IP_RECVIF */

  return size;
}

char *
pal_sock_in4_cmsg_init (struct pal_msghdr *msg, u_int32_t flags)
{
  char *adata;
  size_t alen;

  memset (msg, 0, sizeof (struct pal_msghdr));

  alen = pal_sock_in4_cmsg_len_get (flags);
  if (alen == 0)
    return NULL;

  adata = XCALLOC (MTYPE_TMP, alen);
  msg->msg_control = adata;
  msg->msg_controllen = alen;

  return adata;
}

void
pal_sock_in4_cmsg_finish (struct pal_msghdr *msg)
{
  if (msg->msg_control != NULL)
    XFREE (MTYPE_TMP, msg->msg_control);
}

struct pal_cmsghdr *
pal_sock_in4_cmsg_pktinfo_set (struct pal_msghdr *msg,
                               struct pal_cmsghdr *cmsg,
                               struct pal_in4_addr *src,
                               u_int32_t ifindex)
{
#ifdef IP_PKTINFO
  struct pal_in_pktinfo *pkt;

  if (cmsg == NULL)
    cmsg = PAL_CMSG_FIRSTHDR (msg);
  else
    cmsg = PAL_CMSG_NXTHDR (msg, cmsg);

  if (!cmsg)
    return NULL;

  if(cmsg)
    {
     cmsg->cmsg_len = PAL_CMSG_LEN (sizeof (struct pal_in_pktinfo));
     cmsg->cmsg_level = PAL_IPPROTO_IP;
     cmsg->cmsg_type = PAL_IP_PKTINFO;
    }

  pkt = (struct pal_in_pktinfo *)PAL_CMSG_DATA (cmsg);
  if (src)
#ifdef HAVE_IPNET
    pkt->ipi_spec_dest = *src;
#else
  pkt->ipi_spec_dst = *src;
#endif /* HAVE_IPNET */
  else
#ifdef HAVE_IPNET
    pkt->ipi_spec_dest.s_addr = 0;
#else
  pkt->ipi_spec_dst.s_addr = 0;
#endif /* HAVE_IPNET */
  pkt->ipi_ifindex = ifindex;
#endif /* IP_PKTINFO */

  return cmsg;
}

static void *
pal_sock_in4_cmsg_lookup (struct pal_msghdr *msg, int level, int type)
{
  struct pal_cmsghdr *cmsg;

  for (cmsg = PAL_CMSG_FIRSTHDR (msg); cmsg; cmsg = PAL_CMSG_NXTHDR (msg, cmsg))
    if (cmsg->cmsg_level == level && cmsg->cmsg_type == type)
      return PAL_CMSG_DATA (cmsg);

  return NULL;
}

struct pal_in4_pktinfo *
pal_sock_in4_cmsg_pktinfo_get (struct pal_msghdr *msg)
{
#ifdef IP_PKTINFO
  return (struct pal_in4_pktinfo *)pal_sock_in4_cmsg_lookup (msg,
                                                             PAL_IPPROTO_IP,
                                                             PAL_IP_PKTINFO);
#else
  return NULL;
#endif /* IP_PKTINFO */
}

struct pal_sockaddr_dl *
pal_sock_in4_cmsg_recvif_get (struct pal_msghdr *msg)
{
#ifdef IP_RECVIF
  return (struct pal_sockaddr_dl *)pal_sock_in4_cmsg_lookup (msg,
                                                             PAL_IPPROTO_IP,
                                                             PAL_IP_RECVIF);
#else
  return NULL;
#endif /* IP_RECVIF */
}

int
pal_sock_in4_cmsg_ifindex_get (struct pal_msghdr *msg)
{
#ifdef IP_PKTINFO
  struct pal_in_pktinfo *pkt;
#endif /* IP_PKTINFO */
#ifdef IP_RECVIF
  struct pal_sockaddr_dl *sdl;
#endif /* IP_RECVIF */
  int proto = PAL_IPPROTO_IP;

#ifdef IP_PKTINFO
  pkt = (struct pal_in_pktinfo *)pal_sock_in4_cmsg_lookup (msg, proto, PAL_IP_PKTINFO);
  if (pkt != NULL)
    return pkt->ipi_ifindex;
#endif /* IP_PKTINFO */
#ifdef IP_RECVIF
  sdl = (struct pal_sockaddr_dl *)pal_sock_in4_cmsg_lookup (msg, proto, PAL_IP_RECVIF);
  if (sdl != NULL)
    return sdl->sdl_index;
#endif /* IP_RECVIF */

  return -1;
}

int
pal_sock_in4_cmsg_dst_get (struct pal_msghdr *msg)
{
#if defined (IP_PKTINFO)
  struct pal_in_pktinfo *pkt;
#elif defined (IP_RECVIF)
  struct pal_in_addr *dst;
#endif
  int proto = PAL_IPPROTO_IP;

#if defined (IP_PKTINFO)
  pkt = (struct pal_in_pktinfo *) pal_sock_in4_cmsg_lookup (msg, proto, 
                                                            PAL_IP_PKTINFO);
  if (pkt != NULL)
    return pkt->ipi_addr.s_addr;
#elif defined (IP_RECVIF)
  dst = (struct pal_in_addr *) pal_sock_in4_cmsg_lookup (msg, proto, 
                                                         PAL_IP_RECVDSTADDR);
  if (dst != NULL)
    return dst->s_addr;
#endif

  return -1;
}


/* Return length of IP packet
**
** Parameters:
**    IN    pal_in4_header   *iph
**    OUT   u_int16_t        *length
** Results:
**    RESULT_OK for success, < 0 for error
*/
result_t
pal_in4_ip_packet_len_get (struct pal_in4_header *iph,
                           u_int32_t *length)
{
  *length = pal_ntoh16 (iph->ip_len);
  return RESULT_OK; 
}

/* Set length of IP packet
**
** Parameters:
**   IN/OUT    pal_in4_header   *iph
**   IN         u_int16_T        length of IP packet
** Results:
**   RESULT_OK for success, < 0 for error
*/
result_t
pal_in4_ip_packet_len_set (struct pal_in4_header *iph,
                           u_int32_t length)
{
  iph->ip_len = iph->ip_hl * 4 + length;
  return RESULT_OK;
}

/* Return IP header length 
**
** Parameters:
**   IN   pal_in4_header   *iph
**   OUT  u_int16_t        *length
** Results:
**   RESULT_OK for success, < 0 for error
*/
result_t
pal_in4_ip_hdr_len_get (struct pal_in4_header *iph,
                        u_int16_t *length)
{
  *length = iph->ip_hl;
  return RESULT_OK;
}

/* Set length of IP header given IP options length
**
** Parameters:
**   IN/OUT      pal_in4_header      *iph
**   IN          u_int16_t           length of IP options field.
** Results:
**   RESULT_OK for success, < 0 for error
*/
result_t
pal_in4_ip_hdr_len_set (struct pal_in4_header *iph,
                        u_int16_t length)
{
  iph->ip_hl = (sizeof(struct ip) + length) >> 2;
  return RESULT_OK;
}

/* Set TOS bits in IP header
** 
** Parameters:
**   IN/OUT      pal_in4_header     *iph
**   IN          s_int32_t          TOS bitmask
*/
result_t
pal_in4_ip_hdr_tos_set (struct pal_in4_header *iph,
                        s_int32_t    tos)
{
  if(tos & PAL_IPTOS_PREC_INTERNETCONTROL)
    iph->ip_tos = IPTOS_PREC_INTERNETCONTROL;

  /* Other TOS bits are currently not used by protocols */
  /* Can add others as required */

  return RESULT_OK;
}

/* Peek packet. This will return the length of the IP packet.
** Callers should allocate buffer of this size and pass it to
** pal_ip_recv_packet()
**
** Parameters:
**   IN          sock        socket handle
**   OUT         *iph        IP header
**   OUT         *iphdr_len  IP header length
**   OUt         *ippkt_len  IP packet length
*/
result_t
pal_in4_packet_peek (pal_sock_handle_t sock,
                     struct pal_in4_header *iph,
                     u_int16_t  *iphdr_len,
                     u_int32_t  *ippkt_len
                     )
{
  int hsize = (int)sizeof (struct pal_in4_header);
  socklen_t len = 0;
  int ret;

  ret = pal_sock_recvfrom (sock, (void *)iph, hsize, PAL_MSG_PEEK, NULL, &len);
  if (ret < hsize)
    return -1;

  pal_in4_ip_hdr_len_get (iph, iphdr_len);
  pal_in4_ip_packet_len_get (iph, ippkt_len);

  return RESULT_OK;
}

/* Recv function for raw sockets with options
** 
** Parameters:
**    IN     sock        socket handle
**    IN     iph         IP header
**    OUT    *buf        Pointer to buffer where data will be copied
**    IN    pkt_len      IP packet length
**    IN/OUt *cdata      The control message structure. This will contain
**                       the control data options required on input and
**                       output.
** Returns:
**    RESULT_OK for success, -1 for error.
*/
result_t
pal_in4_recv_packet (pal_sock_handle_t sock,
                     struct pal_in4_header *iph,
                     u_char *buf,
                     s_int32_t pkt_len,
                     struct pal_cdata_msg *msg)
{
  struct msghdr msgh;
  struct cmsghdr *cmsg;
  struct iovec iov;
#if defined (IP_PKTINFO)
  struct in_pktinfo *pktinfo;
#elif defined (IP_RECVIF)
  struct sockaddr_dl *pktinfo;
#endif
#if defined (IP_PKTINFO) || defined(IP_RECVIF)
  char pktinfo_buf[sizeof (*cmsg) + sizeof (*pktinfo)];
#endif
  int ret;

  msgh.msg_name = NULL;
  msgh.msg_namelen = 0;
  msgh.msg_iov = &iov;
  msgh.msg_iovlen = 1;
#if defined (IP_PKTINFO) || defined(IP_RECVIF)
  msgh.msg_control = pktinfo_buf;
  msgh.msg_controllen = sizeof (*cmsg) + sizeof (*pktinfo);
#else
  msgh.msg_control = NULL;
  msgh.msg_controllen = 0;
#endif /* defined (IP_PKTINFO) || defined(IP_RECVIF) */
  msgh.msg_flags = 0;

  iov.iov_base = (char *)buf;
  iov.iov_len = pkt_len;

  ret = pal_sock_recvmsg (sock, &msgh, 0);
  if (ret < 0)
    return ret;
  if (ret != pkt_len)
    return -1;

#if !defined(IP_PKTINFO) && !defined(IP_RECVIF)
  msg->cinfo = PAL_RECVIF_ADDR;
  msg->data.addr = iph->ip_src;
  return RESULT_OK;
#else
  cmsg = PAL_CMSG_FIRSTHDR (&msgh);
  if (cmsg != NULL
      && cmsg->cmsg_level == PAL_IPPROTO_IP
#if defined (IP_PKTINFO)
      && cmsg->cmsg_type == PAL_IP_PKTINFO
#elif defined (IP_RECVIF)
      && cmsg->cmsg_type == PAL_IP_RECVIF
#endif
      )
    {
      msg->cinfo = PAL_RECVIF_INDEX;
#if defined (IP_PKTINFO)
      pktinfo = (struct in_pktinfo *)PAL_CMSG_DATA (cmsg);
      msg->data.ifindex = pktinfo->ipi_ifindex;
#elif defined (IP_RECVIF)
      pktinfo = (struct sockaddr_dl *)PAL_CMSG_DATA (cmsg);
      msg->data.ifindex = pktinfo->sdl_index;
#endif
    }
#endif /* !defined(IP_PKTINFO) && !defined(IP_RECVIF) */

  return RESULT_OK;
}

/* Recv function for raw sockets with options which returns number of bytes
** received if not all pkt_len bytes are received
** 
** Parameters:
**    IN     sock        socket handle
**    IN     iph         IP header
**    OUT    *buf        Pointer to buffer where data will be copied
**    IN    pkt_len      IP packet length
**    IN/OUt *cdata      The control message structure. This will contain
**                       the control data options required on input and
**                       output.
** Returns:
**    RESULT_OK for success, -1 for error, > 0 if number of bytes received
**    != pkt_len.
*/
result_t
pal_in4_recv_packet_len (pal_sock_handle_t sock,
                         struct pal_in4_header *iph,
                         u_char *buf,
                         s_int32_t pkt_len,
                         struct pal_cdata_msg *msg)
{
  struct msghdr msgh;
  struct cmsghdr *cmsg;
  struct iovec iov;
#if defined (IP_PKTINFO)
  struct in_pktinfo *pktinfo;
#elif defined (IP_RECVIF)
  struct sockaddr_dl *pktinfo;
#endif
#if defined (IP_PKTINFO) || defined(IP_RECVIF)
  char pktinfo_buf[sizeof (*cmsg) + sizeof (*pktinfo)];
#endif
  int ret;
  int retv = RESULT_OK;

  msgh.msg_name = NULL;
  msgh.msg_namelen = 0;
  msgh.msg_iov = &iov;
  msgh.msg_iovlen = 1;
#if defined (IP_PKTINFO) || defined(IP_RECVIF)
  msgh.msg_control = pktinfo_buf;
  msgh.msg_controllen = sizeof (*cmsg) + sizeof (*pktinfo);
#else
  msgh.msg_control = NULL;
  msgh.msg_controllen = 0;
#endif /* defined (IP_PKTINFO) || defined(IP_RECVIF) */
  msgh.msg_flags = 0;

  iov.iov_base = (char *)buf;
  iov.iov_len = pkt_len;

  ret = pal_sock_recvmsg (sock, &msgh, 0);
  if (ret < 0)
    return ret;
  if (ret != pkt_len)
    retv = pkt_len;

#if !defined(IP_PKTINFO) && !defined(IP_RECVIF)
  msg->cinfo = PAL_RECVIF_ADDR;
  msg->data.addr = iph->ip_src;
  return retv;
#else
  cmsg = PAL_CMSG_FIRSTHDR (&msgh);
  if (cmsg != NULL
      && cmsg->cmsg_level == PAL_IPPROTO_IP
#if defined (IP_PKTINFO)
      && cmsg->cmsg_type == PAL_IP_PKTINFO
#elif defined (IP_RECVIF)
      && cmsg->cmsg_type == PAL_IP_RECVIF
#endif
      )
    {
      msg->cinfo = PAL_RECVIF_INDEX;
#if defined (IP_PKTINFO)
      pktinfo = (struct in_pktinfo *)PAL_CMSG_DATA (cmsg);
      msg->data.ifindex = pktinfo->ipi_ifindex;
#elif defined (IP_RECVIF)
      pktinfo = (struct sockaddr_dl *)PAL_CMSG_DATA (cmsg);
      msg->data.ifindex = pktinfo->sdl_index;
#endif
    }
#endif /* !defined(IP_PKTINFO) && !defined(IP_RECVIF) */

  return retv;
}

/* Send IP packet. The IP header needs to be properly populated before
** calling of this function.
**
** Parameters:
**   IN           sock            socket handle
**   IN           *hdr            IP header
**   IN           *nexthop        nexthop IP address
**   IN           *buf            Packet buffer
**   IN           length          Length of packet buffer
**   IN           dontroute       Packet send option.
**
** Returns:
**   Number of bytes written or -1 for error.
*/
s_int32_t
pal_in4_send_packet_to_nexthop (pal_sock_handle_t sock,
                                struct pal_in4_header *hdr,
                                s_int32_t      hdr_len,
                                struct pal_in4_addr *nexthop,
                                u_char       *buf,
                                s_int32_t      length,
                                s_int32_t      dontroute)
{
  struct sockaddr_in sa_dst;
  struct msghdr msg;
  struct iovec iov[2];
  int flags = 0;
  int ret;

  pal_mem_set (&sa_dst, 0, sizeof sa_dst);
  sa_dst.sin_family = AF_INET;
#ifdef HAVE_SIN_LEN
  sa_dst.sin_len = sizeof sa_dst;
#endif /* HAVE_SIN_LEN */
  if (nexthop)
    sa_dst.sin_addr = *(struct in_addr *)nexthop;
  else
    sa_dst.sin_addr.s_addr = hdr->ip_dst.s_addr;

  sa_dst.sin_port = pal_hton16 (0);

  /* Set DONTROUTE flag if dst is unicast. */
  if (dontroute == PAL_PACKET_DONTROUTE)
    if (!IN_MULTICAST (pal_ntoh32 (hdr->ip_dst.s_addr)))
      flags = PAL_MSG_DONTROUTE;

  pal_mem_set (&msg, 0, sizeof (msg));
  msg.msg_name = &sa_dst;
  msg.msg_namelen = sizeof (sa_dst); 
  msg.msg_iov = iov;
  msg.msg_iovlen = 2;

  iov[0].iov_base = (char *)hdr;
  iov[0].iov_len = hdr->ip_hl * 4;
  iov[1].iov_base = (char *)buf;
  iov[1].iov_len = length;

  ret = pal_sock_sendmsg (sock, &msg, flags);

  return ret;
}

/* Send IP packet. The IP header needs to be properly populated before
** calling of this function.
** 
** Parameters:
**   IN           sock            socket handle
**   IN           *hdr            IP header
**   IN           *buf            Packet buffer
**   IN           length          Length of packet buffer
**   IN           dontroute       Packet send option.
**
** Returns:
**   Number of bytes written or -1 for error.
*/
s_int32_t
pal_in4_send_packet (pal_sock_handle_t sock,
                     struct pal_in4_header *hdr,
                     s_int32_t      hdr_len,
                     u_char       *buf,
                     s_int32_t      length,
                     s_int32_t      dontroute)
{
  return pal_in4_send_packet_to_nexthop (sock, hdr, hdr_len,
                                         &hdr->ip_dst, buf, length, dontroute);
}

#ifdef HAVE_IPV6
/* XXX
   RFC2292 says "the kernel will calculate and insert the ICMPv6 checksum
   for ICMPv6 raw sockets, since this checksum is mandatory." but linux
   kernel doesn't calculate it.

   And draft-ietf-ipngwg-rfc2292bis-07.txt says "An attempt to set
   IPV6_CHECKSUM for an ICMPv6 socket will fail. Also, an attempt to set
   or get IPV6_CHECKSUM for a non-raw IPv6 socket will fail." So this
   socketopt() is only executed on the linux kernel.
*/
result_t
pal_sock_set_icmp6_checksum (pal_sock_handle_t sock, s_int32_t state)
{
  return setsockopt (sock, PAL_IPPROTO_RAW, PAL_IPV6_CHECKSUM,
                     &state, sizeof state);
}

static size_t
pal_sock_in6_cmsg_hbh_len_get (u_int32_t flags)
{
  size_t hbhlen = 0;

  if (CHECK_FLAG (flags, PAL_CMSG_IPV6_RTRALRT))
    {
      /* Prepare the buffer for Router Alert Hop-By-Hop option and
       * pktinfo ancillary data */
      if ((hbhlen = pal_inet6_opt_init (NULL, 0)) == -1)
        return 0;
      if ((hbhlen = pal_inet6_opt_append (NULL, 0, hbhlen, PAL_IP6OPT_RTALERT, 
                                          2, 2, NULL)) == -1)
        return 0;
      if ((hbhlen = pal_inet6_opt_finish (NULL, 0, hbhlen)) == -1)
        return 0;
    }

  return hbhlen;
}

size_t
pal_sock_in6_cmsg_len_get (u_int32_t flags)
{
  size_t size = 0;
  size_t hbhlen = 0;

  if (CHECK_FLAG (flags, PAL_CMSG_IPV6_PKTINFO))
    size += PAL_CMSG_SPACE (sizeof (struct in6_pktinfo));
  if (CHECK_FLAG (flags, PAL_CMSG_IPV6_HOPLIMIT))
    size += PAL_CMSG_SPACE (sizeof (int));
  if (CHECK_FLAG (flags, PAL_CMSG_IPV6_RTRALRT))
    {
      hbhlen = pal_sock_in6_cmsg_hbh_len_get (flags);
      if (hbhlen == 0)
        return 0;
      else
        size += PAL_CMSG_SPACE (hbhlen);
    }
  if (CHECK_FLAG (flags, PAL_CMSG_IPV6_NEXTHOP))
    size += CMSG_SPACE (sizeof (struct pal_sockaddr_in6));

  return size;
}

char *
pal_sock_in6_cmsg_init (struct pal_msghdr *msg, u_int32_t flags)
{
  char *adata;
  size_t alen;

  alen = pal_sock_in6_cmsg_len_get (flags);
  if (alen == 0)
    return NULL;

  adata = XCALLOC (MTYPE_TMP, alen);
  msg->msg_control = adata;
  msg->msg_controllen = alen;

  return adata;
}

void
pal_sock_in6_cmsg_finish (struct pal_msghdr *msg)
{
  XFREE (MTYPE_TMP, msg->msg_control);
}

struct pal_cmsghdr *
pal_sock_in6_cmsg_rtralrt_set (struct pal_msghdr *msg,
                               struct pal_cmsghdr *cmsg,
                               u_int16_t rtralrt)
{
  int hbhlen = 0, curlen = 0;
  u_char *buf;
  u_char *val = NULL;
  u_int16_t rtalert_code = pal_hton16 (rtralrt);
  u_int32_t flags = 0;

  if (cmsg == NULL)
    cmsg = PAL_CMSG_FIRSTHDR (msg);
  else
    cmsg = PAL_CMSG_NXTHDR (msg, cmsg);

  SET_FLAG (flags, PAL_CMSG_IPV6_RTRALRT);
  hbhlen = pal_sock_in6_cmsg_hbh_len_get (flags);
  if (hbhlen == 0)
    return NULL;

  
  cmsg->cmsg_len = PAL_CMSG_LEN (hbhlen);
  cmsg->cmsg_level = PAL_IPPROTO_IPV6;
  cmsg->cmsg_type = PAL_IPV6_HOPOPTS;

  buf = PAL_CMSG_DATA (cmsg);

  if ((curlen = pal_inet6_opt_init (buf, hbhlen)) == -1)
    return NULL;

  if ((curlen = pal_inet6_opt_append (buf, hbhlen, curlen, 
                                      PAL_IP6OPT_RTALERT, 2, 2, (void **)&val)) == -1)
    return NULL;
  pal_inet6_opt_set_val (val, 0, &rtalert_code, sizeof (rtalert_code));
  if (pal_inet6_opt_finish (buf, hbhlen, curlen) == -1)
    return NULL;

  return cmsg;
}

struct pal_cmsghdr *
pal_sock_in6_cmsg_pktinfo_set (struct pal_msghdr *msg,
                               struct pal_cmsghdr *cmsg,
                               struct pal_in6_addr *addr,
                               u_int32_t ifindex)
{
  struct pal_in6_pktinfo *pkt = NULL;

  if (cmsg == NULL)
    cmsg = PAL_CMSG_FIRSTHDR (msg);
  else
    cmsg = PAL_CMSG_NXTHDR (msg, cmsg);
  
  if (!cmsg)
    return NULL;

  if (cmsg)
    {
      cmsg->cmsg_len = PAL_CMSG_LEN (sizeof (struct in6_pktinfo));
      cmsg->cmsg_level = PAL_IPPROTO_IPV6;
      cmsg->cmsg_type = PAL_IPV6_PKTINFO;

      pkt = (struct pal_in6_pktinfo *) PAL_CMSG_DATA (cmsg);
    }
  
  if (addr)
    pkt->ipi6_addr = *addr;
  else
    memset (&pkt->ipi6_addr, 0, sizeof (struct in6_addr));
  pkt->ipi6_ifindex = ifindex;

  return cmsg;
}

struct pal_cmsghdr *
pal_sock_in6_cmsg_hoplimit_set (struct pal_msghdr *msg,
                                struct pal_cmsghdr *cmsg,
                                s_int32_t hoplimit)
{
  s_int32_t *ptr = NULL;

  if (cmsg == NULL)
    cmsg = PAL_CMSG_FIRSTHDR (msg);
  else
    cmsg = PAL_CMSG_NXTHDR (msg, cmsg);

  if (!cmsg)
    return NULL;

  if (cmsg)
    {
     cmsg->cmsg_len = PAL_CMSG_LEN (sizeof (s_int32_t));
     cmsg->cmsg_level = PAL_IPPROTO_IPV6;
     cmsg->cmsg_type = PAL_IPV6_HOPLIMIT;

     ptr = (s_int32_t *)PAL_CMSG_DATA (cmsg);
    }
  *ptr = hoplimit;

  return cmsg;
}

struct pal_cmsghdr *
pal_sock_in6_cmsg_nexthop_set (struct pal_msghdr *msg,
                               struct pal_cmsghdr *cmsg,
                               struct pal_in6_addr *addr)
{
  s_int32_t *ptr;
  struct pal_sockaddr_in6 sock6_addr;

  if (cmsg == NULL)
    cmsg = PAL_CMSG_FIRSTHDR (msg);
  else
    cmsg = PAL_CMSG_NXTHDR (msg, cmsg);

  if (!cmsg)
    return NULL;

  cmsg->cmsg_len = PAL_CMSG_LEN (sizeof (struct pal_sockaddr_in6));
  cmsg->cmsg_level = PAL_IPPROTO_IPV6;
  cmsg->cmsg_type = PAL_IPV6_NEXTHOP;

  ptr = (s_int32_t *)PAL_CMSG_DATA (cmsg);
  pal_mem_set (&sock6_addr, 0, sizeof (struct pal_sockaddr_in6));
  sock6_addr.sin6_family = AF_INET6;
  sock6_addr.sin6_scope_id = 4;
  IPV6_ADDR_COPY (&sock6_addr.sin6_addr, &addr);

  pal_mem_cpy (ptr, &sock6_addr, sizeof (struct pal_sockaddr_in6));

  return cmsg;
}

static void *
pal_sock_in6_cmsg_lookup (struct pal_msghdr *msg, int level, int type)
{
  struct pal_cmsghdr *cmsg;

  for (cmsg = PAL_CMSG_FIRSTHDR (msg); cmsg; cmsg = PAL_CMSG_NXTHDR (msg, cmsg))
    if (cmsg->cmsg_level == level && cmsg->cmsg_type == type)
      return PAL_CMSG_DATA (cmsg);

  return NULL;
}

struct pal_in6_pktinfo *
pal_sock_in6_cmsg_pktinfo_get (struct pal_msghdr *msg)
{
#ifdef HAVE_IPNET
  return (struct pal_in6_pktinfo *)pal_sock_in6_cmsg_lookup (msg,
                                                             PAL_IPPROTO_IPV6,
                                                             PAL_IPV6_RECVPKTINFO);
#else
  return (struct pal_in6_pktinfo *)pal_sock_in6_cmsg_lookup (msg,
                                                             PAL_IPPROTO_IPV6,
                                                             IPV6_PKTINFO);
#endif /* HAVE_IPNET */
}

int *
pal_sock_in6_cmsg_hoplimit_get (struct pal_msghdr *msg)
{
  return (int *)pal_sock_in6_cmsg_lookup (msg, PAL_IPPROTO_IPV6, PAL_IPV6_HOPLIMIT);
}

int *
pal_sock_in6_cmsg_nexthop_get (struct pal_msghdr *msg)
{
  return (int *) pal_sock_in6_cmsg_lookup (msg, PAL_IPPROTO_IPV6, 
                                          PAL_IPV6_NEXTHOP);
}

result_t
pal_in6_send_packet (pal_sock_handle_t sock, struct pal_msghdr *msg,
                     u_char *buf, int length, struct pal_sockaddr_in6 *sin6)
{
  struct pal_iovec iov;

  iov.iov_base = buf;
  iov.iov_len = length;
  msg->msg_name = (void *) sin6;
  msg->msg_namelen = sizeof (struct pal_sockaddr_in6);
  msg->msg_iov = &iov;
  msg->msg_iovlen = 1;
  /* Ancillary data must be set already. */

  return pal_sock_sendmsg (sock, (struct msghdr *)msg, 0);
}

result_t
pal_in6_recv_packet (pal_sock_handle_t sock,
                     struct pal_msghdr *msg, char *adata, size_t alen,
                     u_char *buf, int length, struct pal_sockaddr_in6 *sin6)
{
  struct pal_iovec iov;
  int ret;

  /* Fill in iovec and message. */
  iov.iov_base = buf;
  iov.iov_len = length;
  msg->msg_name = (void *) sin6;
  msg->msg_namelen = sizeof (struct pal_sockaddr_in6);
  msg->msg_iov = &iov;
  msg->msg_iovlen = 1;
  msg->msg_control = (void *) adata;
  msg->msg_controllen = alen;

  /* If recvmsg fail return minus value. */
  ret = pal_sock_recvmsg (sock, (struct msghdr *)msg, 0);

  /* Restore sin6_len and sin6_family since IPNET converts 
   * these to Linux style (i.e. no sin6_len) values for LKM
   */
#ifdef HAVE_IPNET
#ifdef APN_USE_SA_LEN
  sin6->sin6_len = sizeof (struct pal_sockaddr_in6);
#endif /* APN_USE_SA_LEN */
  sin6->sin6_family = AF_INET6;
#endif /* HAVE_IPNET */

  return ret;
}

result_t
pal_in6_mld_snp_recv_packet (pal_sock_handle_t sock,
                             struct pal_msghdr *msg, char *adata, size_t alen,
                             u_char *buf, int length, struct sockaddr_mld *mld)
{
  struct pal_iovec iov;

  /* Fill in iovec and message. */
  iov.iov_base = buf;
  iov.iov_len = length;
  msg->msg_name = (void *) mld;
  msg->msg_namelen = sizeof (struct sockaddr_mld);
  msg->msg_iov = &iov;
  msg->msg_iovlen = 1;
  msg->msg_control = (void *) adata;
  msg->msg_controllen = alen;

  /* If recvmsg fail return minus value. */
  return pal_sock_recvmsg (sock, (struct msghdr *)msg, 0);
}

result_t
pal_in6_packet_peek (pal_sock_handle_t sock,
                     struct pal_msghdr *msg,
                     char *adata, size_t alen,
                     u_char *buf, int length,
                     struct pal_sockaddr_in6 *sin6)
{
  struct pal_iovec iov;
  int ret;

  /* Fill in iovec and message. */
  iov.iov_base = buf;
  iov.iov_len = length;
  msg->msg_name = (void *) sin6;
  msg->msg_namelen = sizeof (struct pal_sockaddr_in6);
  msg->msg_iov = &iov;
  msg->msg_iovlen = 1;
  msg->msg_control = (void *) adata;
  msg->msg_controllen = alen;
  msg->msg_flags = 0;

  /* If recvmsg fail return minus value. */
  ret = pal_sock_recvmsg (sock, (struct msghdr *) msg, PAL_MSG_PEEK);

  /* Restore sin6_len and sin6_family since IPNET converts 
   * these to Linux style (i.e. no sin6_len) values for LKM
   */
#ifdef HAVE_IPNET
#ifdef APN_USE_SA_LEN
  sin6->sin6_len = sizeof (struct pal_sockaddr_in6);
#endif /* APN_USE_SA_LEN */
  sin6->sin6_family = AF_INET6;
#endif /* HAVE_IPNET */

  return ret;
}

/* Initialization buffer data for options header.  */
int
pal_inet6_opt_init (void *extbuf, size_t extlen)
{
  struct pal_ip6_opt *opt = (struct pal_ip6_opt *) extbuf;

  if (extlen < 0 || extlen % 8)
    return -1;

  if (opt)
    {
      if (! extlen)
        return -1;

      opt->ip6o_len = (extlen >> 3) - 1;
    }

  return 2;
}

/* Append one TLV option to the options header.  */
int
pal_inet6_opt_append (void *extbuf, socklen_t extlen, int offset, u_char type,
                      socklen_t len, u_char align, void **databufp)
{
  int clen;
  int padlen;

  /* align value check.  */
  if (align != 1 && align != 2 && align != 4 && align != 8)
    return -1;

  /* align must be smaller or equal to len.  */
  if (align > len)
    return -1;

  /* Calculate the final length. */
  clen = offset + 2 + len;

  /* If it does not match to alignment, calculate padding length.  */
  if (clen % align)
    padlen = align - (clen % align);
  else
    padlen = 0;

  /* The option must fit in the extension header buffer. */
  clen += padlen;
  if (extlen && clen > extlen)
    return -1;

  /* Put the value when extbuf is specified, otherwise just return
     length.  */
  if (extbuf)
    {
      u_char *optp = (u_char *) extbuf + offset;

      if (padlen == 1)
        *optp++ = PAL_IP6OPT_PAD1;
      else if (padlen > 0)
        {
          *optp++ = PAL_IP6OPT_PADN;
          *optp++ = padlen - 2;
          memset (optp, 0, padlen - 2);
          optp += (padlen - 2);
        }

      *optp++ = type;
      *optp++ = len;

      *databufp = optp;
    }

  return clen;
}

/* Add one component of the option content to the option.  Just copy
   the value.  */
int
pal_inet6_opt_set_val (void *databuf, int offset, void *val, socklen_t vallen)
{
  pal_mem_cpy ((u_char *) databuf + offset, val, vallen);
  return offset + vallen;
}

/* Finish adding TLV options to the options header.  */
int
pal_inet6_opt_finish (void *extbuf, socklen_t extlen, int offset)
{
  int clen;
  u_char *opt;
  int padlen;

  if (offset)
    clen = ((offset - 1) | 7) + 1;
  else
    clen = 0;

  if (extbuf)
    {
      padlen = clen - offset;

      if (clen > extlen)
        return -1;

      opt = (u_char *) extbuf + offset;

      if (padlen == 1)
        *opt = PAL_IP6OPT_PAD1;
      else if (padlen > 0)
        {
          *opt++ = PAL_IP6OPT_PADN;
          *opt++ = (padlen - 2);
          memset (opt, 0, padlen - 2);
        }
    }

  return clen;
}

result_t
pal_in6_recv_packet_len (pal_sock_handle_t sock,
                         struct pal_in6_header *iph,
                         u_char *buf,
                         s_int32_t pkt_len,
                         struct pal_cdata_msg *msg)
{
  struct msghdr msgh;
  struct cmsghdr *cmsg;
  struct iovec iov;
#if defined (IP_PKTINFO)
  struct in_pktinfo *pktinfo;
#elif defined (IP_RECVIF)
  struct sockaddr_dl *pktinfo;
#endif
#if defined (IP_PKTINFO) || defined(IP_RECVIF)
  char pktinfo_buf[sizeof (*cmsg) + sizeof (*pktinfo)];
#endif
  int ret;
  int retv = RESULT_OK;

  msgh.msg_name = NULL;
  msgh.msg_namelen = 0;
  msgh.msg_iov = &iov;
  msgh.msg_iovlen = 1;
#if defined (IP_PKTINFO) || defined(IP_RECVIF)
  msgh.msg_control = pktinfo_buf;
  msgh.msg_controllen = sizeof (*cmsg) + sizeof (*pktinfo);
#else
  msgh.msg_control = NULL;
  msgh.msg_controllen = 0;
#endif /* defined (IP_PKTINFO) || defined(IP_RECVIF) */
  msgh.msg_flags = 0;

  iov.iov_base = (char *)buf;
  iov.iov_len = pkt_len;

  ret = pal_sock_recvmsg (sock, &msgh, 0);
  if (ret < 0)
    return ret;

  if (ret != pkt_len)
    retv = pkt_len;

#if !defined(IP_PKTINFO) && !defined(IP_RECVIF)
  msg->cinfo = PAL_RECVIF_ADDR;
  IPV6_ADDR_COPY (&msg->data.addr, &iph->ip6_src);
  return retv;
#else
  cmsg = PAL_CMSG_FIRSTHDR (&msgh);
  if (cmsg != NULL
      && cmsg->cmsg_level == PAL_IPPROTO_IP
#if defined (IP_PKTINFO)
      && cmsg->cmsg_type == PAL_IP_PKTINFO
#elif defined (IP_RECVIF)
      && cmsg->cmsg_type == PAL_IP_RECVIF
#endif
      )
    {
      msg->cinfo = PAL_RECVIF_INDEX;
#if defined (IP_PKTINFO)
      pktinfo = (struct in_pktinfo *)PAL_CMSG_DATA (cmsg);
      msg->data.ifindex = pktinfo->apn_ifindex;
#elif defined (IP_RECVIF)
      pktinfo = (struct sockaddr_dl *)PAL_CMSG_DATA (cmsg);
      msg->data.ifindex = pktinfo->sdl_index;
#endif
    }
#endif /* !defined(IP_PKTINFO) && !defined(IP_RECVIF) */

  return retv;
}

/* Send IP packet. The IP header needs to be properly populated before
** calling of this function.
**
** Parameters:
**   IN           sock            socket handle
**   IN           *hdr            IPv6 header
**   IN           *nexthop        nexthop IPv6 address
**   IN           *buf            Packet buffer
**   IN           length          Length of packet buffer
**   IN           dontroute       Packet send option.
**
** Returns:
**   Number of bytes written or -1 for error.
*/
s_int32_t
pal_in6_send_packet_to_nexthop (pal_sock_handle_t sock,
                                struct pal_in6_header *hdr,
                                s_int32_t      hdr_len,
                                struct pal_in6_addr *nexthop,
                                u_char         *buf,
                                s_int32_t      length,
                                s_int32_t      dontroute)
{
  struct sockaddr_in6 sa_dst;
  struct msghdr msg;
  struct iovec iov[2];
  int flags = 0;
  int ret;

  pal_mem_set (&sa_dst, 0, sizeof sa_dst);
  sa_dst.sin6_family = AF_INET6;
#ifdef HAVE_SIN_LEN
  sa_dst.sin_len = sizeof sa_dst;
#endif /* HAVE_SIN_LEN */
  if (nexthop)
    pal_mem_cpy (&sa_dst.sin6_addr, nexthop, sizeof (struct pal_in6_addr));
  else
    pal_mem_cpy (&sa_dst.sin6_addr, &hdr->ip6_dst, sizeof (struct pal_in6_addr));
  sa_dst.sin6_port = pal_hton16 (0);

  /* Set DONTROUTE flag if dst is unicast. */
  if (dontroute == PAL_PACKET_DONTROUTE)
    if (!IN_MULTICAST (hdr->ip6_dst.s6_addr))
      flags = MSG_DONTROUTE;

  pal_mem_set (&msg, 0, sizeof (msg));
  msg.msg_name = &sa_dst;
  msg.msg_namelen = sizeof (sa_dst);
  msg.msg_iov = iov;
  msg.msg_iovlen = 2;

  iov[0].iov_base = (char *)hdr;
  iov[0].iov_len = hdr_len;
  iov[1].iov_base = (char *)buf;
  iov[1].iov_len = length;

  ret = pal_sock_sendmsg (sock, &msg, flags);

  return ret;
}


#endif /* HAVE_IPV6 */
