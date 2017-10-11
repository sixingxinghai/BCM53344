/* Copyright (C) 2002-2003  All Rights Reserved. */

#include "pal_sock_udp.h"

/* Recv function for udp sockets with options
**
** Parameters:
**    IN     sock        socket handle
**    OUT    *buf        Pointer to buffer where data will be copied
**    IN/OUt *cdata      The control message structure. This will contain
**                       the control data options required on input and
**                       output.
** Returns:
**    No. of bytes of data received on success, -1 on failure
*/
s_int32_t
pal_in4_udp_recv_packet (pal_sock_handle_t sock,
                         u_char *buf,
                         struct pal_udp_ctrl_info *c_info)
{
  u_char c_buf[UDP_CTRL_BUFSIZ];
  int len = 0;
  struct pal_msghdr mhdr;
  struct pal_iovec iov;
  struct pal_cmsghdr *cmsg;

  /* setup input message structures */
  iov.iov_base = buf;
  iov.iov_len = UDP_BUFSIZ;

  pal_mem_set (&mhdr, 0, sizeof (struct pal_msghdr));
  if (c_info)
    {
      mhdr.msg_name = &c_info->src;
      mhdr.msg_namelen = sizeof (struct pal_sockaddr_in4);
    }
  else
    {
      mhdr.msg_name = NULL;
      mhdr.msg_namelen = 0;
    }
  mhdr.msg_iov = &iov;
  mhdr.msg_iovlen = 1;
  mhdr.msg_control = c_buf;
  mhdr.msg_controllen = UDP_CTRL_BUFSIZ;

  /* recv udp packet */
  len = pal_sock_recvmsg (sock, &mhdr, 0);
  if (len <= 0)
    {
      return -1;
    }

  if (! c_info)
    return len; 

  c_info->c_flag = PAL_RECV_SRCADDR;

  /* get control information */
  for (cmsg = PAL_CMSG_FIRSTHDR (&mhdr); cmsg && cmsg->cmsg_level == PAL_IPPROTO_IP;
       cmsg = PAL_CMSG_NXTHDR (&mhdr, cmsg))
    {
#if defined (IP_PKTINFO)
      if (cmsg->cmsg_type == PAL_IP_PKTINFO)
        {
          c_info->in_ifindex = ((struct in_pktinfo *)PAL_CMSG_DATA (cmsg))->ipi_ifindex;
          pal_mem_cpy (&c_info->dst_addr, 
                       &((struct in_pktinfo *)PAL_CMSG_DATA (cmsg))->ipi_addr, 
                       sizeof (struct pal_in4_addr));
          c_info->c_flag |= (PAL_RECV_IFINDEX | PAL_RECV_DSTADDR); 
          
          return len;
        }
#else 
#if defined (IP_RECVDSTADDR)
      if (cmsg->cmsg_type == PAL_IP_RECVDSTADDR)
        {
          pal_mem_cpy (&c_info->dst_addr, PAL_CMSG_DATA (cmsg), sizeof (struct pal_in4_addr));
          c_info->c_flag |= PAL_RECV_DSTADDR; 
          continue;
        }
#endif /* IP_RECVDSTADDR */
#if defined (IP_RECVIF)
      if (cmsg->cmsg_type == PAL_IP_RECVIF)
        {
          c_info->in_ifindex = ((struct pal_sockaddr_dl *) PAL_CMSG_DATA (cmsg))->sdl_index;
          c_info->c_flag |= PAL_RECV_IFINDEX; 
        }
#endif /* IP_RECVIF */
#endif /* IP_PKTINFO */
    }

  return len;
}
