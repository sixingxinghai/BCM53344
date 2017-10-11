/* Copyright (C) 2001-2003  All Rights Reserved. */

#include "pal.h"
#include "lib.h"
#include "if.h"
#include "prefix.h"
#include "sockunion.h"
#include "thread.h"
#include "log.h"

#include "ospfd/ospfd.h"
#include "ospfd/ospf_interface.h"
#include "ospfd/ospf_network.h"
#include "ospfd/ospf_packet.h"
#include "ospfd/ospf_lsa.h"
#include "ospfd/ospf_vrf.h"
#include "ospfd/ospf_debug.h"
#ifdef HAVE_OSPF_MULTI_AREA
#include "ospfd/ospf_api.h"
#include "ospfd/ospf_multi_area_link.h"
#endif /* HAVE_OSPF_MULTI_AREA */


int
ospf_if_count_allspfrouters (struct ospf *top, struct interface *ifp)
{
  struct ospf_interface *oi;
  struct ls_node *rn, *rn_tmp;
  struct ls_prefix8 p;
  int count = 0;
#ifdef HAVE_OSPF_MULTI_INST
  struct ospf_interface *oi_other = NULL;
#endif /* HAVE_OSPF_MULTI_INST */

  pal_mem_set (&p, 0, sizeof (struct ls_prefix8));
  p.prefixlen = 32;
  p.prefixsize = OSPF_GLOBAL_IF_TABLE_DEPTH;
  ls_prefix_set_args ((struct ls_prefix *)&p, "i", &ifp->ifindex);

  rn_tmp = ls_node_get (top->om->if_table, (struct ls_prefix *)&p);
  
  for (rn = ls_lock_node (rn_tmp); rn; rn = ls_route_next_until (rn, rn_tmp))
    if ((oi = RN_INFO (rn, RNI_DEFAULT)))
      {
        if (oi->top == top)
          {
            if (CHECK_FLAG (oi->flags, OSPF_IF_MC_ALLSPFROUTERS))
              count++;
          }
#ifdef HAVE_OSPF_MULTI_INST
        else if (CHECK_FLAG (oi->flags, OSPF_IF_EXT_MULTI_INST))
          {
            /* Extension to multi instance is enabled */
            oi_other = ospf_if_entry_match (top, oi);
            if (oi_other && CHECK_FLAG (oi_other->flags, OSPF_IF_MC_ALLSPFROUTERS))
              count++;
          }
#endif /* HAVE_OSPF_MULTI_INST */
      }

  ls_unlock_node (rn_tmp);

  return count;
}
#ifdef HAVE_OSPF_MULTI_AREA
int
ospf_multi_area_if_count_allspfrouters (struct ospf *top, struct interface *ifp)
{
  struct ls_node *rn = NULL;
  struct ospf_multi_area_link *mlink = NULL;
  int count = 0;

  for (rn = ls_table_top (top->multi_area_link_table); rn;
               rn = ls_route_next (rn))
    if ((mlink = RN_INFO (rn, RNI_DEFAULT)))
      if ((pal_strcmp(mlink->ifname, ifp->name) == 0) && 
          (mlink->oi && mlink->oi->top == top))
        if (CHECK_FLAG (mlink->oi->flags, OSPF_IF_MC_ALLSPFROUTERS))
          count++;

  return count;
}
#endif /* HAVE_OSPF_MULTI_AREA */

int
ospf_if_count_alldrouters (struct ospf *top, struct interface *ifp)
{
  struct ospf_interface *oi;
  struct ls_node *rn, *rn_tmp;
  struct ls_prefix8 p;
  int count = 0;
#ifdef HAVE_OSPF_MULTI_INST
  struct ospf_interface *oi_other = NULL;
#endif /* HAVE_OSPF_MULTI_INST */

  pal_mem_set (&p, 0, sizeof (struct ls_prefix8));
  p.prefixlen = 32;
  p.prefixsize = OSPF_GLOBAL_IF_TABLE_DEPTH;
  ls_prefix_set_args ((struct ls_prefix *)&p, "i", &ifp->ifindex);

  rn_tmp = ls_node_get (top->om->if_table, (struct ls_prefix *)&p);

  for (rn = ls_lock_node (rn_tmp); rn; rn = ls_route_next_until (rn, rn_tmp))
    if ((oi = RN_INFO (rn, RNI_DEFAULT)))
      {
        if (oi->top == top)
          {
            if (CHECK_FLAG (oi->flags, OSPF_IF_MC_ALLDROUTERS))
              count++;
          }
#ifdef HAVE_OSPF_MULTI_INST
        else if (CHECK_FLAG (oi->flags, OSPF_IF_EXT_MULTI_INST))
          {
            oi_other = ospf_if_entry_match (top, oi);
            if (oi_other && CHECK_FLAG (oi_other->flags, OSPF_IF_MC_ALLDROUTERS))
              count++;
          }            
#endif /* HAVE_OSPF_MULTI_INST */
      }

  ls_unlock_node (rn_tmp);

  return count;
}

/* Join to the OSPF ALL SPF ROUTERS multicast group. */
int
ospf_if_join_allspfrouters (struct ospf_interface *oi)
{
  char if_str[OSPF_IF_STRING_MAXLEN];
  struct ospf_master *om = oi->top->om;
  struct interface *ifp = oi->u.ifp;
  struct prefix *p = oi->ident.address;
  int ret;

  if (CHECK_FLAG (oi->flags, OSPF_IF_MC_ALLSPFROUTERS))
    return 1;

  if (IS_DEBUG_OSPF (event, EVENT_OS))
    zlog_info (ZG, "OS[%s]: Join to AllSPFRouters Multicast group",
               IF_NAME (oi));

  if (ospf_if_count_allspfrouters (oi->top, ifp) == 0)
    {
      ret = pal_sock_set_ipv4_multicast_join (oi->top->fd, IPv4AllSPFRouters,
                                              p->u.prefix4, ifp->ifindex);
      if (ret < 0)
        {
          if (IS_DEBUG_OSPF (event, EVENT_OS))
            zlog_warn (ZG, "OS[%s]: Can't setsockotp IP_ADD_MEMBERSHIP: %s",
                       IF_NAME (oi), pal_strerror (errno));
          return 0;
        }
    }
  SET_FLAG (oi->flags, OSPF_IF_MC_ALLSPFROUTERS);

  return 1;
}

int
ospf_if_leave_allspfrouters (struct ospf_interface *oi)
{
  char if_str[OSPF_IF_STRING_MAXLEN];
  struct ospf_master *om = oi->top->om;
  struct interface *ifp = oi->u.ifp;
  struct prefix *p = oi->ident.address;
  int ret = 1;

  if (!CHECK_FLAG (oi->flags, OSPF_IF_MC_ALLSPFROUTERS))
    return 1;

  if (IS_DEBUG_OSPF (event, EVENT_OS))
    zlog_info (ZG, "OS[%s]: Leave from AllSPFRouters Multicast group",
               IF_NAME (oi));

  if (ospf_if_count_allspfrouters (oi->top, ifp)
#ifdef HAVE_OSPF_MULTI_AREA
      +
      ospf_multi_area_if_count_allspfrouters (oi->top, ifp)
#endif /* HAVE_OSPF_MULTI_AREA */ 
      == 1)
    {
      ret = pal_sock_set_ipv4_multicast_leave (oi->top->fd, IPv4AllSPFRouters,
                                               p->u.prefix4, ifp->ifindex);
      if (ret < 0)
        {
          if (IS_DEBUG_OSPF (event, EVENT_OS))
            zlog_warn (ZG, "OS[%s]: Can't setsockopt IP_DROP_MEMBERSHIP: %s",
                       IF_NAME (oi), pal_strerror (errno));
          ret = 0;
        }
    }
  UNSET_FLAG (oi->flags, OSPF_IF_MC_ALLSPFROUTERS);

  return ret;
}

/* Join to the OSPF ALL Designated ROUTERS multicast group. */
int
ospf_if_join_alldrouters (struct ospf_interface *oi)
{
  char if_str[OSPF_IF_STRING_MAXLEN];
  struct ospf_master *om = oi->top->om;
  struct interface *ifp = oi->u.ifp;
  struct prefix *p = oi->ident.address;
  int ret;

  if (CHECK_FLAG (oi->flags, OSPF_IF_MC_ALLDROUTERS))
    return 1;

  if (IS_DEBUG_OSPF (event, EVENT_OS))
    zlog_info (ZG, "OS[%s]: Join to AllDRouters Multicast group",
               IF_NAME (oi));

  if (ospf_if_count_alldrouters (oi->top, ifp) == 0)
    {
      ret = pal_sock_set_ipv4_multicast_join (oi->top->fd, IPv4AllDRouters,
                                              p->u.prefix4, ifp->ifindex);
      if (ret < 0)
        {
          if (IS_DEBUG_OSPF (event, EVENT_OS))
            zlog_warn (ZG, "OS[%s]: Can't setsockopt IP_ADD_MEMBERSHIP: %s",
                       IF_NAME (oi), pal_strerror (errno));
          return 0;
        }
    }
  SET_FLAG (oi->flags, OSPF_IF_MC_ALLDROUTERS);

  return 1;
}

int
ospf_if_leave_alldrouters (struct ospf_interface *oi)
{
  char if_str[OSPF_IF_STRING_MAXLEN];
  struct ospf_master *om = oi->top->om;
  struct interface *ifp = oi->u.ifp;
  struct prefix *p = oi->ident.address;
  int ret = 1;

  if (!CHECK_FLAG (oi->flags, OSPF_IF_MC_ALLDROUTERS))
    return 1;

  if (IS_DEBUG_OSPF (event, EVENT_OS))
    zlog_info (ZG, "OS[%s]: Leave from AllDRouters Multicast group",
               IF_NAME (oi));

  if (ospf_if_count_alldrouters (oi->top, ifp) == 1)
    {
      ret = pal_sock_set_ipv4_multicast_leave (oi->top->fd, IPv4AllDRouters,
                                               p->u.prefix4, ifp->ifindex);
      if (ret < 0)
        {
          if (IS_DEBUG_OSPF (event, EVENT_OS))
            zlog_warn (ZG, "OS[%s]: Can't setsockopt IP_DROP_MEMBERSHIP: %s",
                       IF_NAME (oi), pal_strerror (errno));
          ret = 0;
        }
    }
  UNSET_FLAG (oi->flags, OSPF_IF_MC_ALLDROUTERS);

  return ret;
}

int
ospf_if_ipmulticast (struct ospf_interface *oi)
{
  char if_str[OSPF_IF_STRING_MAXLEN];
  struct ospf_master *om = oi->top->om;
  u_char val = 0;
  int ret, len;

  len = sizeof (val);

  /* Prevent receiving self-origined multicast packets. */
  ret = pal_sock_set_ipv4_multicast_loop (oi->top->fd, val);
  if (ret < 0)
    if (IS_DEBUG_OSPF (event, EVENT_OS))
      zlog_warn (ZG, "OS[%s]: Can't setsockopt IP_MULTICAST_LOOP(0): %s",
                 IF_NAME (oi), pal_strerror (errno));

  ret = pal_sock_set_ipv4_multicast_if (oi->top->fd,
                                        oi->ident.address->u.prefix4,
                                        oi->u.ifp->ifindex);
  if (ret < 0)
    {
      if (IS_DEBUG_OSPF (event, EVENT_OS))
        zlog_warn (ZG, "OS[%s]: Can't setsockopt IP_MULTICAST_IF: %s",
                   IF_NAME (oi), pal_strerror (errno));
      return 0;
    }

  return 1;
}

int
ospf_sock_new (struct ospf *top)
{
  struct ospf_master *om = top->om;
#ifdef IPTOS_PREC_INTERNETCONTROL
  int tos = IPTOS_PREC_INTERNETCONTROL;
#endif /* IPTOS_PREC_INTERNETCONTROL */
#ifdef HAVE_IN_PKTINFO
  int ttl;
#endif /* HAVE_IN_PKTINFO */
  int sock;
  int state;
  int ret;

  sock = pal_sock (ZG, AF_INET, SOCK_RAW, IPPROTO_OSPFIGP);
  if (sock < 0)
    {
      if (IS_DEBUG_OSPF (event, EVENT_OS))
        zlog_warn (ZG, "OS: Can't create OSPF socket: %s",
                   pal_strerror (errno));
      return -1;
    }

  /* Set precedence field. */
#ifdef IPTOS_PREC_INTERNETCONTROL
  ret = pal_sock_set_ipv4_tos_prec (sock, tos);
  if (ret < 0)
    {
      pal_sock_close (ZG, sock);
      if (IS_DEBUG_OSPF (event, EVENT_OS))
        zlog_warn (ZG, "OS: Can't setsockopt IP_TOS: %s",
                   pal_strerror (errno));
      return -1;
    }
#endif /* IPTOS_PREC_INTERNETCONTROL */

#ifdef HAVE_IN_PKTINFO
  /* Set the multicast TTL value.  */
  ttl = OSPF_IP_TTL;
  ret = pal_sock_set_ipv4_multicast_hops (sock, ttl);
  if (ret < 0)
    {
      pal_sock_close (ZG, sock);
      if (IS_DEBUG_OSPF (event, EVENT_OS))
        zlog_warn (ZG, "OS: Can't setsockopt IP_MULTICAST_TTL: %s",
                   pal_strerror (errno));
      return -1;
    }

  /* Set the unicast TTL value.  */
  ttl = OSPF_VLINK_IP_TTL;
  ret = pal_sock_set_ipv4_unicast_hops (sock, ttl);
  if (ret < 0)
    {
      pal_sock_close (ZG, sock);
      if (IS_DEBUG_OSPF (event, EVENT_OS))
        zlog_warn (ZG, "OS: Can't setsockopt IP_TTL: %s",
                   pal_strerror (errno));
      return -1;
    }
#else
  /* Include the IP header if the terget doesn't support PKTINFO.  */
  state = 1;
  ret = pal_sock_set_ip_hdr_incl (sock, state);
  if (ret < 0)
    {
      pal_sock_close (ZG, sock);
      if (IS_DEBUG_OSPF (event, EVENT_OS))
        zlog_warn (ZG, "OS: Can't setsockopt IP_HDRINCL: %s",
                   pal_strerror (errno));
      return -1;
    }
#endif /* HAVE_IN_PKTINFO */

  state = 1;
  ret = pal_sock_set_ip_recvif (sock, state);
  if (ret < 0)
    if (IS_DEBUG_OSPF (event, EVENT_OS))
      zlog_warn (ZG, "OS: Can't setsockopt IP_RECVIF: %s",
                 pal_strerror (errno));

  ret = pal_sock_set_bindtofib (sock, top->ov->iv->fib_id);
  if (ret != RESULT_OK)
    if (IS_DEBUG_OSPF (event, EVENT_OS))
      zlog_warn (ZG, "OS: Can't bind socket to FIB(%d): %s",
                 top->ov->iv->fib_id, pal_strerror (errno));

  /* Increase the receiving buffer.  */
  ret = pal_sock_set_recvbuf (sock, (1024*512));
  if (ret < 0)
    if (IS_DEBUG_OSPF (event, EVENT_OS))
      zlog_warn (ZG, "OS: Can't set socket receive buffer size to %d: %s",
                 (1024 * 512), pal_strerror (errno));

  /* Increase the sending buffer.  */
  ret = pal_sock_set_sendbuf (sock, (1024*512));
  if (ret < 0)
    if (IS_DEBUG_OSPF (event, EVENT_OS))
      zlog_warn (ZG, "OS: Can't set socket send buffer size to %d: %s",
                 (1024 * 512), pal_strerror (errno));

  return sock;
}

void
ospf_sock_init (struct ospf *top)
{
  if (top->fd == -1)
    {
      /* Create a socket.  */
      top->fd = ospf_sock_new (top);
      if (top->fd != -1)
        OSPF_READ_ON (top->t_read, ospf_read, top);
    }
}

void
ospf_sock_reset (struct ospf *top)
{
  if (top->fd != -1)
    {
      /* Cancel threads.  */
      OSPF_TIMER_OFF (top->t_write);
      OSPF_TIMER_OFF (top->t_read);

      /* Close the socket.  */
      pal_sock_close (ZG, top->fd);

      /* Reset the discriptor.  */
      top->fd = -1;
    }
}
