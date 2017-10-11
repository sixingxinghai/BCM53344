/* Copyright (C) 2001-2003  All Rights Reserved. */

#include "pal.h"
#include "plat_vrrp.h"

#ifdef HAVE_VRRP

#include "lib.h"

#include "vty.h"
#include "prefix.h"
#include "thread.h"
#include "if.h"
#include "stream.h"
#include "log.h"
#include "stream.h"

#include "linux/if_ether.h"

#include "net/if_arp.h"
#include "netinet/if_ether.h"

#include "nsm/nsm_interface.h"
#include "nsm/nsm_connected.h"
#include "nsm/rib/rib.h"
#include "nsm/nsm_rt.h"

#include "nsm/vrrp/vrrp.h"

#ifndef HAVE_VRRP_LINK_ADDR
static result_t plat_vrrp_promisc_recv (struct thread *);
#ifdef HAVE_IPV6
static result_t plat_vrrp_neigh_disc_recv (struct thread *);

static struct vip6_addr_struc *vrrp_linux_exists_vip6 (pal_vrrp_handle_t,
                                                       struct pal_in6_addr *);

#endif /* HAVE_IPV6 */

static struct vip_addr_struc *vrrp_linux_exists_vip (pal_vrrp_handle_t,
                                                     struct pal_in4_addr *);
/* Initialize promiscuous receive processing. */
void
plat_vrrp_promisc_start (struct lib_globals *lib_node)
{
  pal_vrrp_handle_t vh = lib_node->pal_vrrp;
  struct packet_mreq mreq;
  result_t ret;

  /* Open socket for promiscuous mode. */
  if (vh->sock_promisc <= 0)
    {
      vh->sock_promisc = socket (PF_PACKET, SOCK_RAW, htons(ETH_P_ARP));
      if (vh->sock_promisc < 0)
        {
          zlog_err (lib_node, "VRRP Error: Open RAW socket failed.\n");
          return;
        }

      pal_mem_set (&mreq, 0, sizeof (struct packet_mreq));

      /* Setup the socket. */
      mreq.mr_ifindex = 1;
      mreq.mr_type = PACKET_MR_PROMISC;
      ret = setsockopt (vh->sock_promisc,
                        SOL_PACKET,
                        PACKET_ADD_MEMBERSHIP,
                        &mreq,
                        sizeof(struct packet_mreq));
    }

  /* Setup callback function for promiscuous processing. */
  if (! vh->t_vrrp_prom)
    vh->t_vrrp_prom = thread_add_read (lib_node, plat_vrrp_promisc_recv, NULL, vh->sock_promisc);
}

/* Stop promiscuous receive processing (no VRRP sessions are active). */
void
plat_vrrp_promisc_stop (struct lib_globals *lib_node)
{
  pal_vrrp_handle_t vh = lib_node->pal_vrrp;

  /* Cancel thread. */
  if (vh->t_vrrp_prom)
    thread_cancel (vh->t_vrrp_prom);

  /* Close socket. */
  if (vh->sock_promisc > 0)
    pal_sock_close (lib_node, vh->sock_promisc);

  /* Reset handle. */
  vh->sock_promisc = -1;
  vh->t_vrrp_prom = NULL;
}

/* ------------------------------------------------------------------------
 * plat_vrrp_promisc_recv - This function is used by Ver2 to watch for ARP
 *      requests for any of the Virtual IP addresses that exist on this
 *      VRRP router.  When it sees one, this function builds & sends
 *      an ARP reply containing the Virtual Mac Addresss.
 *      This function was used by Ver1 to watch for advertisements from
 *      higher priority VRRP routers when preempt mode was enabled.
 * ------------------------------------------------------------------------ */
static result_t
plat_vrrp_promisc_recv (struct thread *t)
{
  pal_vrrp_handle_t vrrp_handle = t->zg->pal_vrrp;
  int nbytes;
  unsigned char packet[BUFSIZ];
  struct sockaddr_in from;
  int fromlen;
  struct prom_rx *rx;
  u_int16_t type_in;
  u_int16_t arp_type_in;

  /* Reset the receive function */
  vrrp_handle->t_vrrp_prom = NULL;

  /* Add the next event */
  vrrp_handle->t_vrrp_prom = thread_add_read (t->zg, plat_vrrp_promisc_recv, NULL, vrrp_handle->sock_promisc);

  /* Get the packet from the interface */
  memset (&from, 0, sizeof(struct sockaddr_in));
  fromlen = sizeof(struct sockaddr_in);
  nbytes = recvfrom (vrrp_handle->sock_promisc, packet, BUFSIZ, 0,
                (struct sockaddr *)&from, &fromlen);

  /* eth_hdr = 14 bytes, ip_hdr = 20 bytes, vrrp_advt = 20 bytes */
  /*   - check the length; we don't want to segfault */
  if (nbytes < 54) {
    return RESULT_OK;
  }

  /* Cast the front of the packet */
  rx = (struct prom_rx *)packet;

  type_in = ntohs (rx->eth_type);

  /* See if it's an ARP packet (0x0806). */
  if (type_in == 0x0806) {
    struct ether_arp *ea;
    struct sockaddr_in dst;
    struct vip_addr_struc *vs;

    /* Forward to the ARP packet */
    ea = (struct ether_arp *)&packet[14];

    /* If not ARP request (type=1), return. */
    arp_type_in = ntohs (ea->ea_hdr.ar_op);
    if (arp_type_in != 1)
      return RESULT_OK;

    /* Get the destination address from the packet */
    dst.sin_addr.s_addr = ea->arp_tpa[3] << 24;
    dst.sin_addr.s_addr |= ea->arp_tpa[2] << 16;
    dst.sin_addr.s_addr |= ea->arp_tpa[1] << 8;
    dst.sin_addr.s_addr |= ea->arp_tpa[0];

    /* See if we are currently forwarding for an address we don't own */
    /* (VRRP Default Backup as Master case) */
    if ((vs = vrrp_linux_exists_vip (vrrp_handle,
                                     (struct pal_in4_addr *)&dst.sin_addr))
        != NULL) {
      struct stream *ap;
      struct sockaddr_in src;
      u_int16_t eth_hw_type;
      u_int16_t arp_type;
      u_int16_t ip_type;
      u_int8_t hw_len;
      u_int8_t ip_len;
      u_int16_t op;
      int i;

      /* Get the source address from the request */
      src.sin_addr.s_addr = ea->arp_spa[3] << 24;
      src.sin_addr.s_addr |= ea->arp_spa[2] << 16;
      src.sin_addr.s_addr |= ea->arp_spa[1] << 8;
      src.sin_addr.s_addr |= ea->arp_spa[0];

      /* See if it's a gratuitous ARP request. */
      if (src.sin_addr.s_addr == dst.sin_addr.s_addr)
        return RESULT_OK;

      eth_hw_type = 1;  /* H/W type is Ethernet */
      arp_type = 0x806; /* ARP Reply */
      ip_type = 0x0800;
      hw_len = 6;
      ip_len = 4;
      op = 2;

      /* Build packet */
      ap = stream_new (SIZE_ARP_PACKET);

      /* Put the Ethernet header first */
      /* Put destination address - get from received packet */
      for (i=0; i<hw_len; i++) {
        stream_putc (ap, packet[6+i]);
      }

      if (vrrp_handle->vmac_enabled)
        {
          /* Put source address */
          vrrp_handle->vrrp_mac.v.mbyte[5] = vs->vrid;
          for (i=0; i<hw_len; i++) {
            stream_putc (ap, vrrp_handle->vrrp_mac.v.mbyte[i]);
          }
        }
      else
        {
          /* Put interface MAC in source. */
          for (i = 0; i < hw_len; i++)
            {
              stream_putc (ap, vs->ifp->hw_addr[i]);
            }
        }

      /* Put type */
      stream_putw (ap, arp_type);

      /* Fill in the ARP packet */
      stream_putw (ap, eth_hw_type);
      stream_putw (ap, ip_type);
      stream_putc (ap, hw_len);
      stream_putc (ap, ip_len);
      stream_putw (ap, op);

      if (vrrp_handle->vmac_enabled)
        {
          for (i=0; i<hw_len; i++) {
            stream_putc (ap, vrrp_handle->vrrp_mac.v.mbyte[i]);
          }
           /* Reset the global variable */
          vrrp_handle->vrrp_mac.v.mbyte[5] = 0x00;
        }
      else
        {
          /* Put interface MAC in source. */
          for (i = 0; i < hw_len; i++)
            {
              stream_putc (ap, vs->ifp->hw_addr[i]);
            }
        }

      /* Put the Virtual IP address */
      stream_put_in_addr (ap, &dst.sin_addr);

      /* Put the destination hardware address */
      for (i=0; i<hw_len; i++) {
        stream_putc (ap, packet[6+i]);
      }

      /* Put the destination IP address */
      stream_put_in_addr (ap, &src.sin_addr);

      /* padding */
      for (i=0; i<18; i++)
        stream_putc (ap, 0);

      plat_vrrp_send_garp (t->zg, ap, vs->ifp);

      /* Free stream. */
      stream_free (ap);
      }
   }

  return RESULT_OK;
}
#endif /* ! HAVE_VRRP_LINK_ADDR */


/*------------------------------------------------------------------------
 * plat_vrrp_init - called by VRRP to perform any initialization or
 *                  registration with the forwarding engine application.
 *                  It will call all appropriate FE APIs.
 *------------------------------------------------------------------------*/
result_t
plat_vrrp_init (struct lib_globals *lib_node)
{
  pal_vrrp_handle_t vrrp_handle;
#ifdef HAVE_IPV6
  s_int32_t state;
  struct pal_icmp6_filter filter;
#endif /* HAVE_IPV6 */
  /* Allocate Linux global data */
  lib_node->pal_vrrp = XCALLOC (MTYPE_VRRP_LINUX_DATA, sizeof (struct pal_vrrp_data));
  vrrp_handle = lib_node->pal_vrrp;

  /* VMAC feature is enabled by default*/
  vrrp_handle->vmac_enabled = VRRP_VMAC_ENABLE;

  /* Fill in the VRRP virtaul mac information - used for promiscuous mode */
  vrrp_handle->vrrp_mac.v.mbyte[0] = 0x00;
  vrrp_handle->vrrp_mac.v.mbyte[1] = 0x00;
  vrrp_handle->vrrp_mac.v.mbyte[2] = 0x5e;
  vrrp_handle->vrrp_mac.v.mbyte[3] = 0x00;
  vrrp_handle->vrrp_mac.v.mbyte[4] = 0x01;
  vrrp_handle->vrrp_mac.v.mbyte[5] = 0xFF;

  /*
   * Open socket for communicating with the network layer
   */
  vrrp_handle->sock_net = socket (AF_INET, SOCK_DGRAM, 0);
  if (vrrp_handle->sock_net < 0) {
    zlog_err (lib_node, "VRRP Error: Open DGRAM socket failed.\n");
    return EVRRP_SOCK_OPEN;
  }

  /* Open socket for sending Gratuitous ARP messages */
  vrrp_handle->sock_garp = socket (PF_PACKET, SOCK_PACKET, 0);
  if (vrrp_handle->sock_garp < 0)
  {
    zlog_err (lib_node, "VRRP Error: Open PACKET socket failed.\n");
    return EVRRP_SOCK_OPEN;
  }

#ifdef HAVE_IPV6
  vrrp_handle->sock_ndadvt = socket (AF_INET6, SOCK_RAW, IPPROTO_ICMPV6);
  if (vrrp_handle->sock_ndadvt < 0)
    {
      zlog_err (lib_node, "VRRP Error: Open DGRAM socket failed.\n");
      return EVRRP_SOCK_OPEN;
    }

  state = 2;
  if (setsockopt (vrrp_handle->sock_ndadvt, PAL_IPPROTO_RAW, PAL_IPV6_CHECKSUM,
                  &state, sizeof(state) )< 0)
    {
      zlog_err (lib_node, "VRRP Error:  Can't set ICMPv6 checksum: \n");
      return EVRRP_SOCK_OPEN;
   }
 state = 0;
  if (setsockopt (vrrp_handle->sock_ndadvt, PAL_IPPROTO_IPV6,
                  PAL_IPV6_MULTICAST_LOOP, &state, sizeof(state)) < 0)
    {
      zlog_err (lib_node, "VRRP Error: Can't set multicast loop:\n");
      return EVRRP_SOCK_OPEN;
   }

 state = 1;

  if (setsockopt (vrrp_handle->sock_ndadvt, PAL_IPPROTO_IPV6,
                  PAL_IPV6_PKTINFO, &state, sizeof (state)) < 0)
    {
      zlog_err (lib_node, "VRRP Error: Can't set pktinfo:\n");
      return EVRRP_SOCK_OPEN;
   }

  if (setsockopt (vrrp_handle->sock_ndadvt, PAL_IPPROTO_IPV6,
                  PAL_IPV6_HOPLIMIT, &state, sizeof (state)) < 0)
    {
      zlog_err (lib_node, "VRRP Error: Can't set Hoplimit:\n");
      return EVRRP_SOCK_OPEN;
   }

  PAL_ICMP6_FILTER_SETBLOCKALL (&filter);
  PAL_ICMP6_FILTER_SETPASS (PAL_ND_ROUTER_SOLICIT, &filter);
  PAL_ICMP6_FILTER_SETPASS (PAL_ND_ROUTER_ADVERT, &filter);
  PAL_ICMP6_FILTER_SETPASS (PAL_ND_NEIGHBOR_SOLICIT, &filter);
  PAL_ICMP6_FILTER_SETPASS (PAL_ND_NEIGHBOR_ADVERT, &filter);
  PAL_ICMP6_FILTER_SETPASS (PAL_ND_REDIRECT, &filter);

  if (pal_sock_set_ipv6_icmp_filter (vrrp_handle->sock_ndadvt, &filter) < 0)
    {
      zlog_err (lib_node, "VRRP Error: Can't set ICMPv6 filter: \n");
      return EVRRP_SOCK_OPEN;
    }

#endif /* HAVE_IPV6 */

#ifndef HAVE_VRRP_LINK_ADDR
  /* Initialize layer2 vip. */
  vrrp_handle->vip_head = NULL;
  vrrp_handle->vip_tail = NULL;
#ifdef HAVE_IPV6
  vrrp_handle->vip6_head = NULL;
  vrrp_handle->vip6_tail = NULL;
#endif /* HAVE_IPV6 */
#endif /* ! HAVE_VRRP_LINK_ADDR */

  return RESULT_OK;
}

/* De-init VRRP PAL. */
result_t
plat_vrrp_deinit (struct lib_globals *lib_node)
{
  pal_vrrp_handle_t vh = lib_node->pal_vrrp;

  /* Close sockets. */
  if (vh->sock_promisc)
    pal_sock_close (lib_node, vh->sock_promisc);
  if (vh->sock_garp)
    pal_sock_close (lib_node, vh->sock_garp);
  if (vh->sock_net)
    pal_sock_close (lib_node, vh->sock_net);

#ifdef HAVE_IPV6
  if (vh->sock_ndadvt)
    pal_sock_close (lib_node, vh->sock_ndadvt);

#endif /* HAVE_IPV6 */

  /* Cancel threads. */
  if (vh->t_vrrp_prom)
    thread_cancel (vh->t_vrrp_prom);

  XFREE (MTYPE_VRRP_LINUX_DATA, vh);
  lib_node->pal_vrrp = NULL;
  return RESULT_OK;
}

/*------------------------------------------------------------------------
 * plat_vrrp_send_garp- called by VRRP to send a gratuitous ARP message
 *              via the forwarding layer.  This API call will create
 *              the message if necessary, and call the appropriate
 *              FE APIs.
 *------------------------------------------------------------------------*/
result_t
plat_vrrp_send_garp (struct lib_globals *lib_node,
                     struct stream *ap,
                     struct interface *ifp)
{
  pal_vrrp_handle_t vrrp_handle = lib_node->pal_vrrp;
  int sock;
  struct sockaddr from;
  int fromlen;
  int ret;

  /* setup structures */
  sock = vrrp_handle->sock_garp;
  memset (&from, '\0', sizeof(from));
  strcpy (from.sa_data, if_kernel_name (ifp));
  from.sa_family = AF_INET;
  fromlen = sizeof(struct sockaddr);

  /* Bind the socket */
  ret = bind (sock, (struct sockaddr *)&from, sizeof (struct sockaddr));
  if (ret < 0)
  {
    zlog_err (lib_node, "VRRP Error: error binding to socket (grat arp)\n");
    return EVRRP_SOCK_BIND;
  }

  /* eend the ARP packet */
  ret = sendto (sock, ap->data, SIZE_ARP_PACKET, 0, &from, fromlen);
  if (ret != SIZE_ARP_PACKET)
  {
      zlog_err (lib_node, "VRRP Error: sendto sent short Gratuitous ARP\n");
    return EVRRP_SOCK_SEND;
  }

  return RESULT_OK;
}

#ifdef HAVE_IPV6
/*------------------------------------------------------------------------
 * plat_vrrp_send_nd_nbadvt- called by VRRP to send a Unsolicited ND
 *              Neighbor advertisement message
 *              via the forwarding layer.  This API call will create
 *              the message if necessary, and call the appropriate
 *              FE APIs.
 *------------------------------------------------------------------------*/
result_t
plat_vrrp_send_nd_nbadvt (struct lib_globals *lib_node,
                          struct stream *s,
                          struct interface *ifp)
{
  pal_vrrp_handle_t vrrp_handle = lib_node->pal_vrrp;
  struct pal_sockaddr_in6       sin6;
  struct pal_in6_addr           addr6;
  struct pal_msghdr             msg;
  struct pal_cmsghdr            *cmsg;
  struct connected              *ifc;
  struct prefix                 *p;
  int sock;
  int ret;

  /* setup structures */
  sock = vrrp_handle->sock_ndadvt;

  pal_mem_set (&addr6, 0, sizeof (struct pal_in6_addr));
  for (ifc = ifp->ifc_ipv6; ifc; ifc = ifc->next)
    {
     /* Check for link local address. */
     if (!IN6_IS_ADDR_LOOPBACK (&ifc->address->u.prefix6)
         && IN6_IS_ADDR_LINKLOCAL (&ifc->address->u.prefix6))
       {
         p = ifc->address;

         if (! CHECK_FLAG (ifc->flags, NSM_IFA_VIRTUAL))
           addr6 = p->u.prefix6;
       }

    }

  ret = pal_sock_set_ipv6_multicast_loop (sock, 0);
  if (ret != RESULT_OK)
    zlog_err (lib_node, "VRRP Error: Can't setsockopt IPV6_MULTICAST_LOOP:\n");

  /* Set destination ipv6 address. */
  pal_mem_set (&sin6, 0, sizeof (struct pal_sockaddr_in6));
  sin6.sin6_family = AF_INET6;
  sin6.sin6_port = pal_hton16 (IPPROTO_ICMPV6);
  ret = pal_inet_pton (AF_INET6, ALLNODES_GROUP, &sin6.sin6_addr);
  if (ret <= 0)
    return RESULT_ERROR;

  ret = pal_sock_set_ipv6_multicast_if (sock, ifp->ifindex);
  if (ret < 0)
    {
      zlog_err (lib_node, "VRRP Error: Can't set interface \
                as multicast when sending packet\n");
    }

  pal_sock_in6_cmsg_init (&msg, PAL_CMSG_IPV6_PKTINFO|PAL_CMSG_IPV6_HOPLIMIT);

  /* Specify the sending interface index. */
  cmsg = pal_sock_in6_cmsg_pktinfo_set (&msg, NULL, NULL, ifp->ifindex);

  pal_sock_in6_cmsg_hoplimit_set (&msg, cmsg, 255);

  ret = pal_in6_send_packet (sock, &msg, STREAM_DATA (s),
                                 stream_get_putp (s), &sin6);
  if (ret < 0 )
    {
      zlog_err (lib_node, "VRRP Error: 'ipv6 ND ADV sendto' failed\n");
      return EVRRP_SOCK_SEND;
    }

  pal_sock_in6_cmsg_finish (&msg);

  return RESULT_OK;
}

void
plat_vrrp_nd_neigh_adv_send (struct lib_globals *lib_node,
                             struct interface *ifp, struct pal_in6_addr *vip6)
{

  struct stream *s;
  u_char flags = 0xA0;
  int i, ret;

  /* Only allow the address which length + 2 is the multiply of eight. */
  if ((ifp->hw_addr_len + 2) % 8)
    return;

  /* Build the Neighbor advertisement packet */
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
  stream_put_in6_addr (s, vip6);

  /* Type. */
  stream_putc (s, PAL_ND_OPT_TARGET_LINKADDR);

  /* Length. */
  stream_putc (s, (ifp->hw_addr_len + 2) >> 3);

  for (i = 0; i < 6; i++)
     stream_putc (s, ifp->hw_addr[i]);

  ret = plat_vrrp_send_nd_nbadvt (lib_node, s, ifp);

  if (ret < 0)
    zlog_err (lib_node, "VRRP Error: 'ipv6 ND ADV sendto' failed\n");

 /* Now, free the packet. */
  stream_free (s);
}

#ifndef HAVE_VRRP_LINK_ADDR
#define NSM_VRRP_CMSG_BUF_LEN 1024
result_t
plat_vrrp_neigh_disc_recv (struct thread *t)
{
  pal_vrrp_handle_t vrrp_handle = t->zg->pal_vrrp;
  pal_sock_handle_t sock;
  struct vip6_addr_struc *vs;
  struct pal_in6_addr vip6;
  struct pal_msghdr msg;
  struct pal_sockaddr_in6 sin6;
  struct pal_in6_pktinfo *pkt;
  struct interface *ifp;
  struct pal_icmp6_hdr *icmp;
  char adata[NSM_VRRP_CMSG_BUF_LEN];
  u_int32_t ifindex;
  int *hoplimit;
  int len;

  /* Reset the receive function */
  vrrp_handle->t_vrrp_nd = NULL;

  sock = vrrp_handle->sock_ndadvt;
  stream_reset (vrrp_handle->ibuf);

  len = pal_in6_recv_packet (sock, &msg, adata, sizeof (adata),
                             STREAM_DATA (vrrp_handle->ibuf),
                             STREAM_SIZE (vrrp_handle->ibuf),
                             &sin6);

  /* Add the next event */
  vrrp_handle->t_vrrp_nd = thread_add_read (t->zg, plat_vrrp_neigh_disc_recv, NULL,
                                            sock);

  pkt = pal_sock_in6_cmsg_pktinfo_get (&msg);
  if (pkt == NULL)
    {
        zlog_warn (t->zg, "VRRP ND: Unknown interface index\n");
      return -1;
    }

  ifindex = pkt->ipi6_ifindex;
  ifp = ifg_lookup_by_index (&t->zg->ifg, ifindex);

  if (ifp == NULL)
    {
      zlog_warn (t->zg, "VRRP ND: Unknown interface\n");
      return -1;

    }
  hoplimit = pal_sock_in6_cmsg_hoplimit_get (&msg);

  if (len > 0 && (hoplimit != NULL))
    {

     /* ICMP message length check. */
     if (len < sizeof (struct pal_icmp6_hdr))
       {
         zlog_warn (t->zg, "VRRP ND[%R%%%s]: Invalid ICMP length: %d",
                    &sin6.sin6_addr, ifp->name, len);
         return -1;
       }
     icmp = (struct pal_icmp6_hdr *) STREAM_DATA (vrrp_handle->ibuf);

     /* ICMP message type check. */
     switch (icmp->icmp6_type)
       {
       case PAL_ND_NEIGHBOR_SOLICIT:
         stream_forward (vrrp_handle->ibuf, sizeof (u_int32_t));
         vip6 = stream_get_ipv6 (vrrp_handle->ibuf);
         if ((vs = vrrp_linux_exists_vip6 (vrrp_handle, &vip6)) != NULL)
         plat_vrrp_nd_neigh_adv_send (t->zg, ifp, &vip6);
       break;

       default:
        zlog_warn (t->zg, "VRRP ND[%R%%%s]: Unknown ICMP packet type: %u",
                   &sin6.sin6_addr, ifp->name, icmp->icmp6_type);
       break;
       }
    }

  return RESULT_OK;
}

static struct vip6_addr_struc *
vrrp_linux_exists_vip6 (pal_vrrp_handle_t vrrp_handle,
                        struct pal_in6_addr *vip6)
{
  struct vip6_addr_struc *curr;

  curr = vrrp_handle->vip6_head;
  while (curr != NULL) {
    if (IPV6_ADDR_SAME (&curr->vip6, vip6))
      return curr;
    else
      curr = curr->next;
 }
  return NULL;
}

#endif /* ! HAVE_VRRP_LINK_ADDR */
#endif /* HAVE_IPV6 */

#ifndef HAVE_VRRP_LINK_ADDR
/*-----------------------------------------------------------------------
 * vrrp_linux_exists_vip - search list of virtual IPs for a match
 *------------------------------------------------------------------------ */
static struct vip_addr_struc *
vrrp_linux_exists_vip (pal_vrrp_handle_t vrrp_handle,
                       struct pal_in4_addr *vip)
{
  struct vip_addr_struc *curr;

  curr = vrrp_handle->vip_head;
  while (curr != NULL) {
    if (curr->vip.s_addr == vip->s_addr)
      return curr;
    else
      curr = curr->next;
  }
  return NULL;
}
#endif /* ! HAVE_VRRP_LINK_ADDR */

/*------------------------------------------------------------------------
 * plat_vrrp_set_vip - add the virtual IP addres to the interface
 *------------------------------------------------------------------------*/
result_t
plat_vrrp_set_vip (struct lib_globals *lib_node,
                   struct pal_in4_addr *vip,
                   struct interface *ifp,
                   int vrid)
{
#ifdef HAVE_VRRP_LINK_ADDR
  struct nsm_if *nif = ifp->info;
  int ret;

  /* Set flag to permit VIP addition. */
  SET_FLAG (nif->vrrp_if.vrrp_flags, NSM_VRRP_IF_SET_VIP);

  /* Add the IP to the interface. */
  return nsm_ip_address_install (0, if_kernel_name (ifp), vip, 32, NULL, 1);

  /* Reset flag. */
  UNSET_FLAG (nif->vrrp_if.vrrp_flags, NSM_VRRP_IF_SET_VIP);
  return ret;
#else
  pal_vrrp_handle_t vrrp_handle = lib_node->pal_vrrp;
  struct vip_addr_struc *vs;

  /* Fill in the new structure */
  vs = XMALLOC (MTYPE_VRRP_VIP_ADDR, sizeof (struct vip_addr_struc));
  memcpy (&vs->vip, vip, sizeof(struct pal_in4_addr));
  vs->ifp = ifp;
  vs->vrid = vrid;
  vs->next = NULL;
  vs->prev = vrrp_handle->vip_tail;

  /* If head pointer is still null, set it */
  if (vrrp_handle->vip_head == NULL)
    {
      /* Add to the head. */
      vrrp_handle->vip_head = vs;

      /* Start promiscuous receive. */
      plat_vrrp_promisc_start (lib_node);
    }

  /* Link it into the list (at the end) */
  if (vrrp_handle->vip_tail != NULL)
    vrrp_handle->vip_tail->next = vs;

  /* Set the tail pointer */
  vrrp_handle->vip_tail = vs;
#endif /* HAVE_VRRP_LINK_ADDR */

  return RESULT_OK;
}

#ifdef HAVE_IPV6
/*------------------------------------------------------------------------
+  * plat_vrrp_set_vip6 - add the virtual IPV6 addres to the interface
+  *------------------------------------------------------------------------*/
result_t
plat_vrrp_set_vip6 (struct lib_globals *lib_node,
                    struct pal_in6_addr *vip6,
                    struct interface *ifp,
                    int vrid)
{
#ifdef HAVE_VRRP_LINK_ADDR
  struct nsm_if *nif = ifp->info;

  /* Set flag to permit VIP addition. */
  SET_FLAG (nif->vrrp_if.vrrp_flags, NSM_VRRP_IF_SET_VIP6);

  /* Add the IP to the interface. */
  /*  return nsm_ipv6_address_install (ifp->vr->id, ifp->name, vip6, 128,
                                       NULL, NULL, 0); */
  /* Reset flag. */
  UNSET_FLAG (nif->vrrp_if.vrrp_flags, NSM_VRRP_IF_SET_VIP6);
  return RESULT_OK;
#else
  pal_vrrp_handle_t vrrp_handle = lib_node->pal_vrrp;
  struct vip6_addr_struc *vs;

  /* Fill in the new structure */
  vs = XMALLOC (MTYPE_VRRP_VIP_ADDR, sizeof (struct vip6_addr_struc));
  memcpy (&vs->vip6, vip6, sizeof (struct pal_in6_addr));
  vs->ifp = ifp;
  vs->vrid = vrid;
  vs->next = NULL;
  vs->prev = vrrp_handle->vip6_tail;
  /* If head pointer is still null, set it */
  if (vrrp_handle->vip6_head == NULL)
    {
      /* Add to the head. */
      vrrp_handle->vip6_head = vs;

      /* Start receive neighbor solicitaion for VIP6. */
     if (vrrp_handle->t_vrrp_nd == NULL)
       vrrp_handle->t_vrrp_nd = thread_add_read (lib_node,
                                                 plat_vrrp_neigh_disc_recv,
                                                 NULL,
                                                 vrrp_handle->sock_ndadvt);
    }

  /* Link it into the list (at the end) */
  if (vrrp_handle->vip6_tail != NULL)
    vrrp_handle->vip6_tail->next = vs;

  /* Set the tail pointer */
  vrrp_handle->vip6_tail = vs;
#endif /* HAVE_VRRP_LINK_ADDR */

 return RESULT_OK;
}


/*------------------------------------------------------------------------
+  * plat_vrrp_unset_vip6 - remove the virtual IPV6 address from the interface
+  *------------------------------------------------------------------------*/
result_t
plat_vrrp_unset_vip6 (struct lib_globals *lib_node,
                      struct pal_in6_addr *vip6,
                      struct interface *ifp,
                      int vrid)
{
#ifdef HAVE_VRRP_LINK_ADDR

  /* Remove the IP from the interface. */
  /*  ret = nsm_ipv6_address_uninstall (ifp->vr->id, ifp->name, vip6, 128); */
  return RESULT_OK;;
#else
  pal_vrrp_handle_t vrrp_handle = lib_node->pal_vrrp;
  struct vip6_addr_struc *vs;

  /* Make sure vip exists */
  if ((vs = vrrp_linux_exists_vip6(lib_node->pal_vrrp, vip6)) == NULL)
    return EVRRP_MAC_UNSET;

  /* Reset list pointers */
  if (vs->prev != NULL)
     vs->prev->next = vs->next;

  if (vs->next != NULL)
    vs->next->prev = vs->prev;
  /* Reset head & tail pointers if necessary */
  if (vrrp_handle->vip6_head == vs)
    vrrp_handle->vip6_head = vs->next;

  if (vrrp_handle->vip6_tail == vs)
    vrrp_handle->vip6_tail = vs->prev;

  /* Last one deleted? */
  if (! vrrp_handle->vip6_head)
    if (vrrp_handle->t_vrrp_nd)
      thread_cancel (vrrp_handle->t_vrrp_nd);

  /* Free the data structure */
  XFREE (MTYPE_VRRP_VIP_ADDR, vs);
#endif  /* HAVE_VRRP_LINK_ADDR */

  return RESULT_OK;
}
#endif /* HAVE_IPV6 */

/*------------------------------------------------------------------------
 * plat_vrrp_unset_vip - remove the virtual IP address from the interface
 *------------------------------------------------------------------------*/
result_t
plat_vrrp_unset_vip (struct lib_globals *lib_node,
                     struct pal_in4_addr *vip,
                     struct interface *ifp,
                     int vrid)
{
#ifdef HAVE_VRRP_LINK_ADDR
  int ret;

  /* Remove the IP from the interface. */
  ret = nsm_ip_address_uninstall (0, if_kernel_name (ifp), vip, 32, NULL, 1);
  return ret;
#else
  pal_vrrp_handle_t vrrp_handle = lib_node->pal_vrrp;
  struct vip_addr_struc *vs;

  /* Make sure vip exists */
  if ((vs = vrrp_linux_exists_vip(lib_node->pal_vrrp, vip)) == NULL)
    return EVRRP_MAC_UNSET;

  /* Reset list pointers */
  if (vs->prev != NULL)
    vs->prev->next = vs->next;

  if (vs->next != NULL)
    vs->next->prev = vs->prev;

  /* Reset head & tail pointers if necessary */
  if (vrrp_handle->vip_head == vs)
    vrrp_handle->vip_head = vs->next;

  if (vrrp_handle->vip_tail == vs)
    vrrp_handle->vip_tail = vs->prev;

  /* Last one deleted? */
  if (! vrrp_handle->vip_head)
    plat_vrrp_promisc_stop (lib_node);

  /* Free the data structure */
  XFREE (MTYPE_VRRP_VIP_ADDR, vs);
#endif  /* HAVE_VRRP_LINK_ADDR */

  return RESULT_OK;
}


/*-------------------------------------------------------------------------
 * plat_vrrp_get_vmac_status - Returns the current status of VRRP VMAC flag
 *-------------------------------------------------------------------------*/
result_t
plat_vrrp_get_vmac_status (struct lib_globals *lib_node)
{
  pal_vrrp_handle_t vh = lib_node->pal_vrrp;
  return vh->vmac_enabled;
}

/*-------------------------------------------------------------------------------
 * plat_vrrp_set_vmac_status - Sets VRRP VMAC flag with the user specified status
 *-------------------------------------------------------------------------------*/
result_t
plat_vrrp_set_vmac_status (struct lib_globals *lib_node, int status)
{
  pal_vrrp_handle_t vh = lib_node->pal_vrrp;

  if (vh->vmac_enabled != status)
    vh->vmac_enabled = status;

  return vh->vmac_enabled;
}

#endif  /* HAVE_VRRP */
