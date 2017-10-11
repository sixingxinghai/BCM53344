/* Copyright (C) 2001-2003  All Rights Reserved. */

#include "pal.h"

#ifdef HAVE_RTADV

#include "lib.h"
#include "thread.h"
#include "if.h"
#include "log.h"
#include "linklist.h"
#include "table.h"
#include "stream.h"

#include "nsm/nsmd.h"
#include "nsm/nsm_interface.h"
#include "nsm/nsm_connected.h"
#include "rib/rib.h"
#include "nsm/nsm_debug.h"
#include "nsm/nsm_rtadv.h"
#include "nsm/nsm_api.h"
#include "nsm/nsm_router.h"


void nsm_rtadv_send_ra (struct rtadv_if *, struct pal_in6_addr *);
void nsm_rtadv_send_ra_lifetime_zero (struct rtadv_if *, struct pal_in6_addr *);
int nsm_rtadv_read (struct thread *);


#define ALLNODE   "ff02::1"
#define ALLROUTER "ff02::2"

pal_sock_handle_t
nsm_rtadv_open_socket (void)
{
  pal_sock_handle_t sock;
  struct pal_icmp6_filter filter;

  if ((sock = pal_sock (NSM_ZG, AF_INET6, SOCK_RAW, IPPROTO_ICMPV6)) < 0)
    {
      if (IS_NSM_DEBUG_PACKET)
        zlog_warn (NSM_ZG, "RTADV: Can't create socket: %s",
                   pal_strerror (errno));
      return -1;
    }

  if (pal_sock_set_icmp6_checksum (sock, 2) < 0)
    {
      if (IS_NSM_DEBUG_PACKET)
        zlog_warn (NSM_ZG, "RTADV: Can't set ICMPv6 checksum: %s",
                   pal_strerror (errno));
      return -1;
    }
  if (pal_sock_set_ipv6_multicast_loop (sock, 0) < 0)
    {
      if (IS_NSM_DEBUG_PACKET)
        zlog_warn (NSM_ZG, "RTADV: Can't set multicast loop: %s",
                   pal_strerror (errno));
      return -1;
    }
  if (pal_sock_set_ipv6_pktinfo (sock, 1) < 0)
    {
      if (IS_NSM_DEBUG_PACKET)
        zlog_warn (NSM_ZG, "RTADV: Can't set pktinfo: %s",
                   pal_strerror (errno));
      return -1;
    }
  if (pal_sock_set_ipv6_hoplimit (sock, 1) < 0)
    {
      if (IS_NSM_DEBUG_PACKET)
        zlog_warn (NSM_ZG, "RTADV: Can't set hoplimit: %s",
                   pal_strerror (errno));
      return -1;
    }

  PAL_ICMP6_FILTER_SETBLOCKALL (&filter);
  PAL_ICMP6_FILTER_SETPASS (PAL_ND_ROUTER_SOLICIT, &filter);
  PAL_ICMP6_FILTER_SETPASS (PAL_ND_ROUTER_ADVERT, &filter);
#ifdef HAVE_MIP6
  PAL_ICMP6_FILTER_SETPASS (PAL_ICMP6_HADISCOV_REQUEST, &filter);
  PAL_ICMP6_FILTER_SETPASS (PAL_ICMP6_MOBILEPREFIX_SOLICIT, &filter);
#endif /* HAVE_MIP6 */

  if (pal_sock_set_ipv6_icmp_filter (sock, &filter) < 0)
    {
      if (IS_NSM_DEBUG_PACKET)
        zlog_warn (NSM_ZG, "RTADV: Can't set ICMPv6 filter: %s",
                   pal_strerror (errno));
      return -1;
    }

  return sock;
}

pal_sock_handle_t
nsm_rtadv_close_socket (pal_sock_handle_t sock)
{
  pal_sock_close (NSM_ZG, sock);

  return -1;
}

int
nsm_if_join_all_router (pal_sock_handle_t sock, struct interface *ifp)
{
  struct pal_in6_addr addr;

  if (pal_inet_pton (AF_INET6, ALLROUTER, &addr) <= 0)
    {
      if (IS_NSM_DEBUG_PACKET)
        zlog_warn (NSM_ZG, "RTADV[%s]: Join all routers failed: Malformed add", 
                   ifp->name);

      return -1;
    }

  if (pal_sock_set_ipv6_multicast_join (sock, addr, ifp->ifindex) < 0)
    {
      if (IS_NSM_DEBUG_PACKET)
        zlog_warn (NSM_ZG, "RTADV[%s]: Can't join %R multicast group: %s",
                   ifp->name, &addr, pal_strerror (errno));
      return -1;
    }
  return 0;
}

int
nsm_if_leave_all_router (pal_sock_handle_t sock, struct interface *ifp)
{
  struct pal_in6_addr addr;

  if (pal_inet_pton (AF_INET6, ALLROUTER, &addr) <= 0)
    {
      if (IS_NSM_DEBUG_PACKET)
        zlog_warn (NSM_ZG, "RTADV[%s]: Leave all routers failed: Malformed add", 
                   ifp->name);

      return -1;
    }

  if (pal_sock_set_ipv6_multicast_leave (sock, addr, ifp->ifindex) < 0)
    {
      if (IS_NSM_DEBUG_PACKET)
        zlog_warn (NSM_ZG, "RTADV[%s]: Can't leave %R multicast group: %s",
                   ifp->name, &addr, pal_strerror (errno));
      return -1;
    }
  return 0;
}


#ifdef HAVE_MIP6
/* XXX copyed from lib/prefix.c. */
static u_char maskbit[] = {0x00, 0x80, 0xc0, 0xe0, 0xf0,
                                 0xf8, 0xfc, 0xfe, 0xff};

#define HAANYCASTEUI64 "::fdff:ffff:ffff:fffe"
#define HAANYCASTALLONE "ffff:ffff:ffff:ffff:ffff:ffff:ffff:fffe"

struct prefix_ipv6
nsm_rtadv_ha_anycast (struct pal_in6_addr *addr, u_char prefixlen)
{
  int index;
  int offset;
  u_char *ptr0, *ptr1;
  struct prefix_ipv6 anycast;

  anycast.family = AF_INET6;
  anycast.prefixlen = prefixlen;

  if (prefixlen == 64)
    {
      /* Use EUI-64 anycast address format. */
      pal_inet_pton (AF_INET6, HAANYCASTEUI64, &anycast.prefix);
      pal_mem_cpy (&anycast.prefix, addr, 8);
    }
  else
    {
      index = prefixlen / 8;
      offset = prefixlen % 8;
      pal_inet_pton (AF_INET6, HAANYCASTALLONE, &anycast.prefix);
      pal_mem_cpy (&anycast.prefix, addr, index);
      if (offset)
        {
          ptr0 = (u_char *) &anycast.prefix;
          ptr1 = (u_char *) addr;
          ptr0[index] &= ~maskbit[offset];
          ptr0[index] |= ptr1[index] & maskbit[offset];
        }
    }

  return anycast;
}

void
nsm_rtadv_delete_ha_anycast_all (struct interface *ifp,
                                 struct route_table *table)
{
  struct route_node *rn;
  struct rtadv_prefix *rp;
  struct prefix_ipv6 anycast;

  for (rn = route_top (table); rn; rn = route_next (rn))
    if ((rp = rn->info))
      {
        anycast = nsm_rtadv_ha_anycast (&rn->p.u.prefix6, rp->prefixlen);
        if (nsm_connected_check_ipv6 (ifp, (struct prefix *) &anycast))
          nsm_ipv6_address_uninstall (ifp->vr->id, ifp->name,
                                      &anycast.prefix, anycast.prefixlen);
      }
}
#endif /* HAVE_MIP6 */


struct rtadv *
nsm_rtadv_new (struct nsm_master *nm)
{
  struct rtadv *new;

  new = XMALLOC (MTYPE_RTADV, sizeof (struct rtadv));
  pal_mem_set (new, 0, sizeof (struct rtadv));
  new->nm = nm;

  return new;
}

void
nsm_rtadv_free (struct rtadv *top)
{
  if (top == NULL)
    return;

  XFREE (MTYPE_RTADV, top);
}


struct rtadv_prefix *
nsm_rtadv_prefix_new (void)
{
  struct rtadv_prefix *new;

  new = XMALLOC (MTYPE_RTADV_PREFIX, sizeof (struct rtadv_prefix));
  pal_mem_set (new, 0, sizeof (struct rtadv_prefix));

  return new;
}

void
nsm_rtadv_prefix_free (struct rtadv_prefix *rp)
{
  XFREE (MTYPE_RTADV_PREFIX, rp);
}

struct rtadv_prefix *
nsm_rtadv_prefix_lookup (struct route_table *table, struct prefix *p)
{
  struct route_node *rn;

  if ((rn = route_node_lookup (table, p)))
    return rn->info;
  else
    return NULL;
}

struct rtadv_prefix *
nsm_rtadv_prefix_get (struct route_table *table, struct prefix *p)
{
  struct route_node *rn;
  struct rtadv_prefix *rp;

  if ((rp = nsm_rtadv_prefix_lookup (table, p)))
    return rp;
  else
    {
      rn = route_lock_node (route_node_get (table, p));
      rp = nsm_rtadv_prefix_new ();

      rn->info = rp;
      rp->rn = rn;

      return rp;
    }
}

void
nsm_rtadv_prefix_unset (struct rtadv_prefix *rp)
{
  struct route_node *rn = rp->rn;

  rn->info = NULL;
  route_unlock_node (rn);

  /* Cancel threads. */
  NSM_RTADV_TIMER_OFF (rp->t_vlifetime);
  NSM_RTADV_TIMER_OFF (rp->t_plifetime);

  nsm_rtadv_prefix_free (rp);
}

int
nsm_rtadv_prefix_count (struct route_table *table)
{
  struct route_node *rn;
  int count = 0;

  for (rn = route_top (table); rn; rn = route_next (rn))
    if (rn->info)
      count++;

  return count;
}

void
nsm_rtadv_prefix_unset_all (struct route_table *table)
{
  struct route_node *rn;
  struct rtadv_prefix *rp;

  for (rn = route_top (table); rn; rn = route_next (rn))
    if ((rp = rn->info))
      nsm_rtadv_prefix_unset (rp);
}


#ifdef HAVE_MIP6
struct rtadv_home_agent *
nsm_rtadv_ha_new (void)
{
  struct rtadv_home_agent *new;

  new = XMALLOC (MTYPE_RTADV_HOME_AGENT, sizeof (struct rtadv_home_agent));
  pal_mem_set (new, 0, sizeof (struct rtadv_home_agent));

  return new;
}

void
nsm_rtadv_ha_free (struct rtadv_home_agent *ha)
{
  XFREE (MTYPE_RTADV_HOME_AGENT, ha);
}

int
nsm_rtadv_ha_cmp (struct rtadv_home_agent *ha0, struct rtadv_home_agent *ha1)
{
  if (ha0->preference > ha1->preference)
    return -1;
  else if (ha0->preference == ha1->preference)
    return IPV6_ADDR_CMP (&ha0->lladdr, &ha1->lladdr);
  else
    return 1;
}

struct rtadv_home_agent *
nsm_rtadv_ha_lookup (struct rtadv_if *rif, struct pal_in6_addr *lladdr)
{
  struct listnode *n;
  struct rtadv_home_agent *ha;

  LIST_LOOP (rif->li_homeagent, ha, n)
    if (IPV6_ADDR_SAME (&ha->lladdr, lladdr))
      return ha;

  return NULL;
}

void
nsm_rtadv_ha_sort (struct rtadv_if *rif,
                   struct rtadv_home_agent *ha, int preference)
{
  listnode_delete (rif->li_homeagent, ha);
  ha->preference = preference;
  listnode_add_sort (rif->li_homeagent, ha);
}

struct rtadv_home_agent *
nsm_rtadv_ha_get (struct rtadv_if *rif,
                  struct pal_in6_addr *lladdr, int preference)
{
  struct rtadv_home_agent *ha;

  ha = nsm_rtadv_ha_lookup (rif, lladdr);
  if (ha)
    {
      if (ha->preference != preference)
        nsm_rtadv_ha_sort (rif, ha, preference);
      return ha;
    }

  ha = nsm_rtadv_ha_new ();
  IPV6_ADDR_COPY (&ha->lladdr, lladdr);
  ha->rt_address = route_table_init ();
  ha->preference = preference;
  listnode_add_sort (rif->li_homeagent, ha);

  ha->rif = rif;

  return ha;
}

void
nsm_rtadv_ha_reset (struct rtadv_if *rif, struct rtadv_home_agent *ha)
{
  NSM_RTADV_TIMER_OFF (ha->t_lifetime);

  nsm_rtadv_prefix_unset_all (ha->rt_address);
  route_table_finish (ha->rt_address);

  listnode_delete (rif->li_homeagent, ha);
  nsm_rtadv_ha_free (ha);
}

void
nsm_rtadv_ha_reset_all (struct rtadv_if *rif)
{
  struct listnode *n, *next;
  struct rtadv_home_agent *ha;

  for (n = LISTHEAD (rif->li_homeagent); n; n = next)
    {
      next = n->next;
      if ((ha = GETDATA (n)))
        nsm_rtadv_ha_reset (rif, ha);
    }
}

/* Home Agent Prefix timer. */
int
nsm_rtadv_ha_prefix_timer (struct thread *thread)
{
  struct rtadv_prefix *rp = THREAD_ARG (thread);

  if (rp->t_vlifetime == thread)
    rp->t_vlifetime = NULL;
  else if (rp->t_plifetime == thread)
    rp->t_plifetime = NULL;

  nsm_rtadv_prefix_unset (rp);

  return 0;
}

void
nsm_rtadv_ha_prefix_timer_reset (struct rtadv_prefix *rp)
{
  NSM_RTADV_TIMER_OFF (rp->t_vlifetime);
  NSM_RTADV_PREFIX_TIMER_ON (rp->t_vlifetime, nsm_rtadv_ha_prefix_timer,
                             rp->vlifetime);

  NSM_RTADV_TIMER_OFF (rp->t_plifetime);
  NSM_RTADV_PREFIX_TIMER_ON (rp->t_plifetime, nsm_rtadv_ha_prefix_timer,
                             rp->plifetime);
}

/* Home Agent timer. */
int
nsm_rtadv_ha_timer (struct thread *thread)
{
  struct rtadv_home_agent *ha = THREAD_ARG (thread);

  ha->t_lifetime = NULL;

  nsm_rtadv_ha_reset (ha->rif, ha);

  return 0;
}

void
nsm_rtadv_ha_timer_reset (struct rtadv_home_agent *ha)
{
  NSM_RTADV_TIMER_OFF (ha->t_lifetime);
  NSM_RTADV_HA_TIMER_ON (ha->t_lifetime, nsm_rtadv_ha_timer, ha->lifetime);
}
#endif /* HAVE_MIP6 */


int
nsm_rtadv_if_count (struct nsm_master *nm)
{
  struct interface *ifp;
  struct nsm_if *nif;
  struct rtadv_if *rif;
  struct route_node *rn;
  int count = 0;

  for (rn = route_top (nm->vr->ifm.if_table); rn; rn = route_next (rn))
    if ((ifp = rn->info))
      if ((nif = ifp->info))
        if ((rif = nif->rtadv_if))
          if (!if_is_loopback (rif->ifp))
            if (NSM_RTADV_CONFIG_CHECK (rif, RA_ADVERTISE))
              count++;

  return count;
}

/* Solicited Router Advertisement thread. */
int
nsm_rtadv_if_ra_solicited (struct thread *thread)
{
  struct rtadv_if *rif = THREAD_ARG (thread);

  rif->t_ra_solicited = NULL;
  
  if (! rif->block_rs_on_delay)
    {
      rif->block_rs_on_delay = 1;
      nsm_rtadv_send_ra (rif, &rif->dst);
    }
  /* Send solicited Router Advertisement. */
  nsm_rtadv_send_ra (rif, &rif->dst);

  return 0;
}

/* Solicited router solicitation delay thread. */
int
nsm_rtadv_if_rs_solicited_delay (struct thread *thread)
{
  struct rtadv_if *rif;
  pal_assert (thread != NULL);
  rif = THREAD_ARG (thread);
  if (rif)
    {
      rif->t_rs_solicited = NULL;
      rif->block_rs_on_delay = 0;
    }
  return 0;
}

/* Router Advertisement timer. */
int
nsm_rtadv_if_timer (struct thread *thread)
{
  struct pal_in6_addr allnode;
  int random_ra_interval;
  struct rtadv_if *rif;
  int range;

  pal_assert (thread != NULL);

  /* Init local variables. */
  rif = THREAD_ARG (thread);
  rif->t_ra_unsolicited = NULL;
  random_ra_interval = 0;
  range = 0;

/*  struct rtadv_if *rif = THREAD_ARG (thread);

  rif->t_ra_unsolicited = NULL;
*/

  /* Send to the all node multicast address. */
  (void) pal_inet_pton (AF_INET6, ALLNODE, &allnode);

  /* Send Unsolicited Router Advertisement. */
  if (rif->max_initial_rtr_advert)
    nsm_rtadv_send_ra (rif, &allnode);

  rif->max_initial_rtr_advert++;
  if ((rif->max_initial_rtr_advert <= RTADV_MAX_INITIAL_RTR_ADVERTISEMENTS)
      && (rif->ra_interval > RTADV_MAX_INITIAL_RTR_ADVERT_INTERVAL))
    {
      if (IS_NSM_DEBUG_PACKET)
      zlog_warn (NSM_ZG, "RTADV[%s]: Start SEND RA Pkt at FIXED interval [%d]",
                 rif->ifp->name, RTADV_MAX_INITIAL_RTR_ADVERT_INTERVAL);

      NSM_RTADV_IF_TIMER_ON (rif->t_ra_unsolicited,
                             nsm_rtadv_if_timer, RTADV_MAX_INITIAL_RTR_ADVERT_INTERVAL);
    }
  else
    {
      range = rif->ra_interval - rif->min_ra_interval;
      random_ra_interval = pal_rand ();
      random_ra_interval = (random_ra_interval % range) + rif->min_ra_interval;
      if ((random_ra_interval == rif->min_ra_interval) || (random_ra_interval == rif->ra_interval))
        {
          random_ra_interval = pal_rand ();
          random_ra_interval = (random_ra_interval % range) + rif->min_ra_interval;
          if (IS_NSM_DEBUG_PACKET)
            zlog_warn (NSM_ZG, "RTADV[%s]: Start SEND RA packet at random interval %d between %d-%d",
                       rif->ifp->name, random_ra_interval, rif->min_ra_interval, rif->ra_interval);
        }
       random_ra_interval -=1;
       NSM_RTADV_IF_TIMER_ON (rif->t_ra_unsolicited,
                           nsm_rtadv_if_timer, random_ra_interval);
    }

  return 0;
}

void
nsm_rtadv_if_timer_reset (struct rtadv_if *rif, u_int32_t interval)
{
  if (rif->t_ra_unsolicited)
    {
      NSM_RTADV_TIMER_OFF (rif->t_ra_unsolicited);
      NSM_RTADV_IF_TIMER_ON (rif->t_ra_unsolicited,
                             nsm_rtadv_if_timer, interval);
    }
}

int
nsm_rtadv_if_start (struct rtadv_if *rif)
{
  struct interface *ifp = rif->ifp;
  struct rtadv *top = rif->top;

  if (!if_is_up (ifp) && !if_is_running (ifp))
    return 0;

  if (top->sock < 0)
    if ((top->sock = nsm_rtadv_open_socket ()) < 0)
      return -1;

  if (top->t_read == NULL)
    top->t_read = thread_add_read (NSM_ZG, nsm_rtadv_read, top, top->sock);

  nsm_if_join_all_router (top->sock, rif->ifp);
  
  if (rif->t_ra_unsolicited)
    {
       NSM_RTADV_TIMER_OFF (rif->t_ra_unsolicited);
    }
  rif->max_initial_rtr_advert = 0;
        
  NSM_RTADV_EVENT_EXECUTE (nsm_rtadv_if_timer, rif);

  return 0;
}

void
nsm_rtadv_if_stop (struct rtadv_if *rif)
{
  struct rtadv *top = rif->top;
  struct nsm_master *nm = rif->ifp->vr->proto;

  nsm_if_leave_all_router (top->sock, rif->ifp);

  NSM_RTADV_TIMER_OFF (rif->t_ra_solicited);
  NSM_RTADV_TIMER_OFF (rif->t_ra_unsolicited);

  if (nsm_rtadv_if_count (nm) == 0)
    {
      top->sock = nsm_rtadv_close_socket (top->sock);

      THREAD_OFF (top->t_read);
    }
}

struct rtadv_if *
nsm_rtadv_if_new (struct interface *ifp)
{
  struct nsm_master *nm = ifp->vr->proto;
  struct nsm_if *nif;
  struct rtadv_if *rif;
  int ret = 0;

  if (nm->rtadv == NULL)
    return NULL;

  if ((nif = ifp->info) == NULL)
    return NULL;

  if (nif->rtadv_if)
    nsm_rtadv_if_free (ifp);

  rif = XMALLOC (MTYPE_RTADV_IF, sizeof (struct rtadv_if));
  pal_mem_set (rif, 0, sizeof (struct rtadv_if));

  nif->rtadv_if = rif;

  rif->ifp = ifp;
  rif->top = nm->rtadv;

  rif->ra_interval = RTADV_MAX_RA_INTERVAL;
  rif->min_ra_interval = RTADV_DEFAULT_MIN_RA_INTERVAL;
  rif->router_lifetime = RTADV_DEFAULT_ROUTER_LIFETIME;
  rif->reachable_time = RTADV_DEFAULT_REACHABLE_TIME;
  rif->retrans_timer = RTADV_DEFAULT_RETRANS_TIMER;
  rif->curr_hoplimit = RTADV_DEFAULT_CUR_HOPLIMIT;
  rif->prefix_valid_lifetime = RTADV_DEFAULT_PREFIX_VALID_LIFETIME;
  rif->prefix_preferred_lifetime = RTADV_DEFAULT_PREFIX_PREFERRED_LIFETIME;
  rif->max_initial_rtr_advert = 0;

  rif->rt_prefix = route_table_init ();

  /* Set destination address to the all node multicast address as default. */
  ret = pal_inet_pton (AF_INET6, ALLNODE, &rif->dst);

  if (ret <= 0 && IS_NSM_DEBUG_EVENT)
    {
      zlog_warn (NSM_ZG, "RTADV: Set dest add to all node mcast add failed: %d", 
                 ret);
    }

#ifdef HAVE_MIP6
  rif->ha_preference = RTADV_DEFAULT_HA_PREFERENCE;
  rif->ha_lifetime = RTADV_DEFAULT_HA_LIFETIME;

  rif->li_homeagent = list_new ();
  rif->li_homeagent->cmp = (int (*) (void *, void *)) nsm_rtadv_ha_cmp;
#endif /* HAVE_MIP6 */

  return rif;
}

void
nsm_rtadv_if_free (struct interface *ifp)
{
  struct nsm_if *nif;
  struct rtadv_if *rif;

  if ((nif = ifp->info) == NULL || (rif = nif->rtadv_if) == NULL)
    return;

  /* Stop the service.  */
  if (if_is_up (ifp))
    nsm_rtadv_if_stop (rif);
  
  nsm_rtadv_prefix_unset_all (rif->rt_prefix);
  
  route_table_finish (rif->rt_prefix);
  
#ifdef HAVE_MIP6
  nsm_rtadv_ha_reset_all (rif);
  list_free (rif->li_homeagent);
#endif /* HAVE_MIP6 */
  
  XFREE (MTYPE_RTADV_IF, rif);
  
  nif->rtadv_if = NULL;
}

struct rtadv_if *
nsm_rtadv_if_lookup_by_index (struct nsm_master *nm, u_int32_t ifindex)
{
  struct interface *ifp;
  struct nsm_if *nif;
  struct rtadv_if *rif;

  if ((ifp = if_lookup_by_index (&nm->vr->ifm, ifindex)))
    if ((nif = ifp->info))
      if ((rif = nif->rtadv_if))
        return rif;

  return NULL;
}

struct rtadv_if *
nsm_rtadv_if_lookup_by_name (struct nsm_master *nm, char *ifname)
{
  struct interface *ifp;
  struct nsm_if *nif;
  struct rtadv_if *rif;

  if ((ifp = if_lookup_by_name (&nm->vr->ifm, ifname)))
    if ((nif = ifp->info))
      {
        if ((rif = nif->rtadv_if))
          return rif;
        else
          return nsm_rtadv_if_new (ifp);
      }

  return NULL;
}

void
nsm_rtadv_if_up (struct interface *ifp)
{
  struct nsm_if *nif;
  struct rtadv_if *rif;

  if ((nif = ifp->info) != NULL
      && (rif = nif->rtadv_if) != NULL
      && NSM_RTADV_CONFIG_CHECK (rif, RA_ADVERTISE))
    {
      if (IS_NSM_DEBUG_PACKET)
        zlog_warn (NSM_ZG, "RTADV[%s]: Start UP interface", rif->ifp->name);

      nsm_rtadv_if_start (rif);
   }
}

void
nsm_rtadv_if_down (struct interface *ifp)
{
  struct nsm_if *nif;
  struct rtadv_if *rif;

  if ((nif = ifp->info) != NULL
      && (rif = nif->rtadv_if) != NULL
      && NSM_RTADV_CONFIG_CHECK (rif, RA_ADVERTISE))
   {
       rif->max_initial_rtr_advert = 0;

       nsm_rtadv_if_stop (rif);
   }
}


/* Make Source Link-layer address option. */
void
nsm_rtadv_make_opt_slla (struct stream *s, struct interface *ifp)
{
  /* Only allow the address which length + 2 is the multiply of eight. */
  if ((ifp->hw_addr_len + 2) % 8)
    return;

  /* Type. */
  stream_putc (s, PAL_ND_OPT_SOURCE_LINKADDR);

  /* Length. */
  stream_putc (s, (ifp->hw_addr_len + 2) >> 3);

  /* Link-Layer Address. */
  stream_put (s, ifp->hw_addr, ifp->hw_addr_len);
}

void
nsm_rtadv_opt_pi (struct stream *s, struct prefix *p, struct rtadv_prefix *rp, struct rtadv_if *rif)
{
  /* Type. */
  stream_putc (s, PAL_ND_OPT_PREFIX_INFORMATION);

  /* Length. */
  stream_putc (s, 4);

  if (rp)
    {
      /* Prefix Length. */
      stream_putc (s, rp->prefixlen);

      /* Flags. */
      if ((!(NSM_RTADV_CONFIG_CHECK (rif, PREFIX_OFFLINK)))
          &&(!(NSM_RTADV_CONFIG_CHECK (rif, PREFIX_NO_AUTOCONFIG))))
        stream_putc (s, rp->flags);
      else if ((!(NSM_RTADV_CONFIG_CHECK (rif, PREFIX_OFFLINK)))
               &&(NSM_RTADV_CONFIG_CHECK (rif, PREFIX_NO_AUTOCONFIG)))
        {
          if ((CHECK_FLAG(rp->flags, PAL_ND_OPT_PI_FLAG_AUTO)) && (!(CHECK_FLAG(rp->flags, PAL_ND_OPT_PI_FLAG_ONLINK))))
            stream_putc (s, rp->flags); 
          else
            stream_putc (s, PAL_ND_OPT_PI_FLAG_ONLINK);
        }
      else if ((NSM_RTADV_CONFIG_CHECK (rif, PREFIX_OFFLINK))
               &&(!(NSM_RTADV_CONFIG_CHECK (rif, PREFIX_NO_AUTOCONFIG))))
        {
          if ((! CHECK_FLAG(rp->flags, PAL_ND_OPT_PI_FLAG_AUTO)) && (CHECK_FLAG(rp->flags, PAL_ND_OPT_PI_FLAG_ONLINK)))
            stream_putc (s, rp->flags);
          else
            stream_putc (s, PAL_ND_OPT_PI_FLAG_AUTO);          
        }
      else if ((NSM_RTADV_CONFIG_CHECK (rif, PREFIX_OFFLINK))
               &&(NSM_RTADV_CONFIG_CHECK (rif, PREFIX_NO_AUTOCONFIG)))
        {
          if (!(CHECK_FLAG(rp->flags, PAL_ND_OPT_PI_FLAG_AUTO)) && (!(CHECK_FLAG(rp->flags, PAL_ND_OPT_PI_FLAG_ONLINK))))
            stream_putc (s, rp->flags);
          else
             stream_putc (s, 0);          
        }

      /* Valid Lifetime. */
      if (NSM_RTADV_CONFIG_CHECK (rif, PREFIX_VALID_LIFETIME) &&
          (! rp->vlifetime))
        stream_putl (s, rif->prefix_valid_lifetime);
      else if (! rp->vlifetime)
        stream_putl (s, RTADV_DEFAULT_VALID_LIFETIME);
      else
        stream_putl (s, rp->vlifetime);

      /* Preferred Lifetime. */
      if (NSM_RTADV_CONFIG_CHECK (rif, PREFIX_PREFERRED_LIFETIME) &&
          (! rp->plifetime))
        stream_putl (s, rif->prefix_preferred_lifetime);
      else if (! rp->plifetime)
        stream_putl (s, RTADV_DEFAULT_PREFERRED_LIFETIME);
      else
        stream_putl (s, rp->plifetime);
    }
  else
    {
      /* Prefix Length. */
      stream_putc (s, p->prefixlen);

      /* Flags. */
      stream_putc (s, PAL_ND_OPT_PI_FLAG_ONLINK|PAL_ND_OPT_PI_FLAG_AUTO);

      /* Valid Lifetime. */
      stream_putl (s, RTADV_DEFAULT_VALID_LIFETIME);

      /* Preferred Lifetime. */
      stream_putl (s, RTADV_DEFAULT_PREFERRED_LIFETIME);
    }

  /* Reserved2. */
  stream_putl (s, 0);

  /* Prefix. */
  stream_put_in6_addr (s, &p->u.prefix6);
}

/* Make Link MTU option. */
void
nsm_rtadv_opt_linkmtu (struct stream *s, struct interface *ifp)
{
  /* Type. */
  stream_putc (s, PAL_ND_OPT_MTU);
  /* Length. */
  stream_putc (s, 1);
  /* MTU Link*/
  stream_putc (s, 0);
  stream_putc (s, 0);
  stream_putl(s, ifp->mtu);
}

/* CHECK INS Link MTU option. */
void
nsm_rtadv_make_opt_linkmtu (struct stream *s, struct interface *ifp)
{
  if (STREAM_REMAIN (s) >= sizeof (struct pal_nd_opt_mtu))
       nsm_rtadv_opt_linkmtu (s, ifp);
}

/* Make Prefix Information option. */
void
nsm_rtadv_make_opt_pi (struct stream *s, struct route_table *table, struct rtadv_if *rif)
{
  struct route_node *rn;
  struct rtadv_prefix *rp;

  for (rn = route_top (table); rn; rn = route_next (rn))
    if ((rp = rn->info))
      if (STREAM_REMAIN (s) >= sizeof (struct pal_nd_opt_prefix_info))
        nsm_rtadv_opt_pi (s, &rn->p, rp, rif);
}

/* Make Prefix Information option for connected routes. */
void
nsm_rtadv_make_opt_pi_connected (struct stream *s, struct interface *ifp, struct rtadv_if *rif)
{
  struct connected *ifc;
  struct prefix *p;
  struct prefix pp;

  for (ifc = ifp->ifc_ipv6; ifc; ifc = ifc->next)
    if ((p = ifc->address))
      if (! IN6_IS_ADDR_UNSPECIFIED (&p->u.prefix6)
          && ! IN6_IS_ADDR_LOOPBACK (&p->u.prefix6)
          && ! IN6_IS_ADDR_MULTICAST (&p->u.prefix6)
          && ! IN6_IS_ADDR_LINKLOCAL (&p->u.prefix6))
        {
          prefix_copy (&pp, p);
          apply_mask (&pp);
          nsm_rtadv_opt_pi (s, &pp, NULL, rif);
        }
}


#ifdef HAVE_MIP6
/* Make Advertisemet Interval option. */
void
nsm_rtadv_make_opt_ai (struct stream *s, struct rtadv_if *rif)
{
  /* Type. */
  stream_putc (s, PAL_ND_OPT_ADVINTERVAL);

  /* Length. */
  stream_putc (s, 1);

  /* Reserved. */
  stream_putw (s, 0);

  /* Advertisement Interval. */
  stream_putl (s, rif->ra_interval * 1000);
}

/* Make Home Agent Lifetime option. */
void
nsm_rtadv_make_opt_ha_lifetime (struct stream *s, struct rtadv_if *rif)
{
  /* Type. */
  stream_putc (s, PAL_ND_OPT_HOMEAGENT_INFO);

  /* Length. */
  stream_putc (s, 1);

  /* Reserved. */
  stream_putw (s, 0);

  /* Home Agent Preference. */
  stream_putw (s, rif->ha_preference);

  /* Mome Agent Lifetime. */
  stream_putw (s, rif->ha_lifetime);
}
#endif /* HAVE_MIP6 */


/* Make Router Advertisement header. */ 
void
nsm_rtadv_make_ra_header (struct stream *s, struct rtadv_if *rif, u_char zero)
{
  u_char flags = 0;

  /* Type. */
  stream_putc (s, PAL_ND_ROUTER_ADVERT);

  /* Code. */
  stream_putc (s, 0);

  /* Checksum. */
  stream_putw (s, 0);

  /* Cur Hop Limit. */
  stream_putc (s, rif->curr_hoplimit);


  /* Flags. */
  if (NSM_RTADV_CONFIG_CHECK (rif, RA_FLAG_MANAGED))
    SET_FLAG (flags, PAL_ND_RA_FLAG_MANAGED);
  if (NSM_RTADV_CONFIG_CHECK (rif, RA_FLAG_OTHER))
    SET_FLAG (flags, PAL_ND_RA_FLAG_OTHER);
  if (NSM_RTADV_CONFIG_CHECK (rif, MIP6_HOME_AGENT))
    SET_FLAG (flags, PAL_ND_RA_FLAG_HOME_AGENT);
  stream_putc (s, flags);

  /* Router Lifetime. */
  if (zero)
    {
      stream_putw (s, 0);
    }
  else
    {
      if ((rif->router_lifetime == RTADV_DEFAULT_ROUTER_LIFETIME)
        && (rif->ra_interval != RTADV_MAX_RA_INTERVAL))
      stream_putw (s, rif->ra_interval * 3);
      else
        stream_putw (s, rif->router_lifetime);
    }

  /* Reachable Time. */
  if (zero)
    stream_putl (s, 0);
  else
    stream_putl (s, rif->reachable_time);

  /* Retrans Timer. */
  if (zero)
    stream_putl (s, 0);
  else
    stream_putl (s, rif->retrans_timer);
}

/* Make Router Advertisement optinos. */
void
nsm_rtadv_make_ra_opt (struct stream *s, struct rtadv_if *rif)
{
#ifdef HAVE_MIP6
  struct pal_in6_addr allzero;
  struct rtadv_home_agent *ha;
#endif /* HAVE_MIP6 */
  struct interface *ifp = rif->ifp;

  /* Source Link-layer address option. */
  nsm_rtadv_make_opt_slla (s, ifp);

  /* Advanced link MTU option */
  if (NSM_RTADV_CONFIG_CHECK (rif, LINKMTU))
    nsm_rtadv_make_opt_linkmtu (s, ifp);

#ifdef HAVE_MIP6
  /* Advertisement interval option. */
  if (NSM_RTADV_CONFIG_CHECK (rif, MIP6_ADV_INTERVAL))
    nsm_rtadv_make_opt_ai (s, rif);

  /* Home agent inoformation option. */
  if (NSM_RTADV_CONFIG_CHECK (rif, MIP6_HA_INFO))
    nsm_rtadv_make_opt_ha_lifetime (s, rif);
#endif /* HAVE_MIP6 */

  /* Prefix Information option for connected routers. */
  if (NSM_RTADV_CONFIG_CHECK (rif, RA_PREFIX))
    nsm_rtadv_make_opt_pi (s, rif->rt_prefix, rif);
  else
    nsm_rtadv_make_opt_pi_connected (s, ifp, rif);

#ifdef HAVE_MIP6
  /* Prefix Information option with Router flag set. */
  if (NSM_RTADV_CONFIG_CHECK (rif, MIP6_HOME_AGENT))
    {
      pal_mem_set (&allzero, 0, sizeof (struct pal_in6_addr));
      if ((ha = nsm_rtadv_ha_lookup (rif, &allzero)))
        nsm_rtadv_make_opt_pi (s, ha->rt_address, rif);
    }
#endif /* HAVE_MIP6 */
}

/* Make Router Advertisement packet. */
void
nsm_rtadv_make_ra (struct rtadv_if *rif)
{
  /* Router Advertisement header. */
  nsm_rtadv_make_ra_header (rif->top->obuf, rif, 0);

  /* Router Advertisement options. */
  nsm_rtadv_make_ra_opt (rif->top->obuf, rif);
}

/* Make router advertisement packet */
void
nsm_rtadv_make_ra_lifetime_zero (struct rtadv_if *rif)
{
  /* Router advertisement header */
  nsm_rtadv_make_ra_header (rif->top->obuf, rif, 1);
}

#ifdef HAVE_MIP6
/* Make Home Agent Address Reply header. */
void
nsm_rtadv_make_ha_rep_header (struct stream *s, u_int16_t id)
{
  /* Type. */
  stream_putc (s, PAL_ICMP6_HADISCOV_REPLY);

  /* Code. */
  stream_putc (s, 0);

  /* Checksum. */
  stream_putw (s, 0);

  /* Identifier. */
  stream_put (s, &id, sizeof (id));

  /* Reserved. */
  stream_put (s, 0, 10);
}

/* Make Home Agent Address Reply packet. */
void
nsm_rtadv_make_ha_rep (struct rtadv_if *rif, u_int16_t id)
{
  int i;
  struct listnode *n;
  struct route_node *rn;
  struct rtadv_home_agent *ha;
  struct rtadv_prefix *rp;

  nsm_rtadv_make_ha_rep_header (rif->top->obuf, id);

  for (i = 0, n = LISTHEAD (rif->li_homeagent); n; i++, NEXTNODE (n))
    if ((ha = GETDATA (n)))
      {
        if (STREAM_REMAIN (rif->top->obuf) < sizeof (struct pal_in6_addr))
          break;

        if (i == 0 && IN6_IS_ADDR_UNSPECIFIED (&ha->lladdr))
          continue;

        for (rn = route_top (ha->rt_address); rn; rn = route_next (rn))
          if ((rp = rn->info))
            {
              stream_put_in6_addr (rif->top->obuf, &rn->p.u.prefix6);
              break;
            }
      }
}

void
nsm_rtadv_make_pa_header (struct stream *s)
{
  /* Type. */
  stream_putc (s, PAL_ICMP6_MOBILEPREFIX_ADVERT);

  /* Code. */
  stream_putc (s, 0);

  /* Checksum. */
  stream_putw (s, 0);
}
  
/* Make Prefix Advertisement packet. */
void
nsm_rtadv_make_pa (struct rtadv_if *rif)
{
  /* Prefix Advertisement header. */
  nsm_rtadv_make_pa_header (rif->top->obuf);

  /* Prefix Information option. */
  nsm_rtadv_make_opt_pi (rif->top->obuf, rif->rt_prefix, rif);
}
#endif /* HAVE_MIP6 */

int
nsm_rtadv_send_packet (struct rtadv_if *rif,
                       struct pal_in6_addr *dst, int hoplimit)
{
  int ret;
  struct pal_msghdr msg;
  struct pal_cmsghdr *cmsg;
  struct pal_sockaddr_in6 sin6;
  struct rtadv *top = rif->top;
  struct interface *ifp = rif->ifp;
  u_char *buf = STREAM_DATA (rif->top->obuf);
  u_int32_t len = stream_get_putp (rif->top->obuf);

  /* For some CMSG_NXTHDR() implementation, at least glibc 2.1.x,
     applications should clear the ancillary data buffer before
     transmittion.  */
  pal_sock_in6_cmsg_init (&msg, PAL_CMSG_IPV6_PKTINFO|PAL_CMSG_IPV6_HOPLIMIT);

  /* Specify the sending interface index. */
  cmsg = pal_sock_in6_cmsg_pktinfo_set (&msg, NULL, NULL, ifp->ifindex);
  pal_sock_in6_cmsg_hoplimit_set (&msg, cmsg, hoplimit);

  /* Fill in destination address. */
  pal_mem_set (&sin6, 0, sizeof (struct pal_sockaddr_in6));
  sin6.sin6_family = AF_INET6;
#ifdef SIN6_LEN
  sin6.sin6_len = sizeof (struct pal_sockaddr_in6);
#endif /* SIN6_LEN */
  sin6.sin6_addr = *dst;
  sin6.sin6_port = pal_hton16 (IPPROTO_ICMPV6);

  ret = pal_in6_send_packet (top->sock, &msg, buf, len, &sin6);

  if (ret < 0)
    if (IS_NSM_DEBUG_SEND)
      zlog_warn (NSM_ZG, "RTADV[%R%%%s]: sendmsg failed: %s",
                 dst, ifp->name, pal_strerror (errno));

  pal_sock_in6_cmsg_finish (&msg);

  return ret;
}

/* Send Router Advertisement packet. */
void
nsm_rtadv_send_ra (struct rtadv_if *rif, struct pal_in6_addr *dst)
{
  if (IS_NSM_DEBUG_SEND)
    zlog_info (NSM_ZG, "RTADV[%R%%%s]: Send Router Advertisement",
               dst, rif->ifp->name);

  stream_reset (rif->top->obuf);

  nsm_rtadv_make_ra (rif);

  /* Send to the specified destination. */
  nsm_rtadv_send_packet (rif, dst, 255);
}

/* Send Router Advertisement packet. */
void
nsm_rtadv_send_ra_lifetime_zero (struct rtadv_if *rif, struct pal_in6_addr *dst)
{
  if (IS_NSM_DEBUG_SEND)
    zlog_info (NSM_ZG, "RTADV[%R%%%s]: Send Router Advertisement [lifetime=0]",
               dst, rif->ifp->name);
  stream_reset (rif->top->obuf);
  nsm_rtadv_make_ra_lifetime_zero(rif);
  /* Send to the specified destination. */
  nsm_rtadv_send_packet (rif, dst, 255);
}

#ifdef HAVE_MIP6
/* Send Home Agent Address Reply packet. */
void
nsm_rtadv_send_ha_rep (struct rtadv_if *rif,
                       u_int16_t id, struct pal_in6_addr *dst)
{
  if (IS_NSM_DEBUG_SEND)
    zlog_info (NSM_ZG, "RTADV[%R%%%s]: Send Home Agent Address Reply",
               dst, rif->ifp->name);

  stream_reset (rif->top->obuf);

  nsm_rtadv_make_ha_rep (rif, id);

  /* Send back to where the packet comes from. */
  nsm_rtadv_send_packet (rif, dst, -1);
}

/* Send Prefix Advertisement packet. */
void
nsm_rtadv_send_pa (struct rtadv_if *rif, struct pal_in6_addr *dst)
{
  if (IS_NSM_DEBUG_SEND)
    zlog_info (NSM_ZG, "RTADV[%R%%%s]: Send Prefix Advertisement",
               dst, rif->ifp->name);

  stream_reset (rif->top->obuf);

  nsm_rtadv_make_pa (rif);

  /* Send back to where the packet comes from. */
  nsm_rtadv_send_packet (rif, dst, -1);
}
#endif /* HAVE_MIP6 */


void
nsm_rtadv_process_rs (struct rtadv_if *rif, struct pal_in6_addr *src)
{
  if (IS_NSM_DEBUG_RECV)
    zlog_info (NSM_ZG, "RTADV[%R%%%s]: Recv Router Solicitation",
               src, rif->ifp->name);

  /* Store source address on non multicast capable interface. */
  if (! if_is_multicast (rif->ifp))
    IPV6_ADDR_COPY (&rif->dst, src);

  /* Send the solicited router addvertisement. */
   NSM_RTADV_RS_TIMER_ON (rif->t_rs_solicited, nsm_rtadv_if_rs_solicited_delay, RTADV_MIN_DELAY_BETWEEN_RAS);

  /* Send the solicited router advertisement. */
  NSM_RTADV_IF_TIMER_ON (rif->t_ra_solicited, nsm_rtadv_if_ra_solicited, 0);

  /* The interface interval timer is reset to a new random value,
     as if an unsolicited advertisement had just been sent.
     (RFC2461 6.2.6) */
  nsm_rtadv_if_timer_reset (rif, rif->ra_interval);
}

#ifdef HAVE_MIP6
void
nsm_rtadv_ha_update_opt (struct rtadv_if *rif,
                         struct rtadv_home_agent *ha,
                         struct pal_nd_router_advert *ra, int len)
{
  u_char *ptr;
  int optlen;
  struct prefix p;
  struct rtadv_prefix *rp;
  struct pal_nd_opt_hdr *rtopt;
  struct pal_nd_opt_prefix_info *pi;
  struct pal_nd_opt_home_agent_info *hi;
  int hinfo_update;

  hinfo_update = 0;

  ptr = (u_char *) (ra + 1);
  len -= sizeof (struct pal_nd_router_advert);
  while (len > 0)
    {
      rtopt = (struct pal_nd_opt_hdr *) ptr;
      switch (rtopt->nd_opt_type)
        {
        case PAL_ND_OPT_PREFIX_INFORMATION:
          pi = (struct pal_nd_opt_prefix_info *) ptr;
          if (pi->nd_opt_pi_flags_reserved & PAL_ND_OPT_PI_FLAG_ROUTER)
            {
              p.family = AF_INET6;
              p.prefixlen = IPV6_MAX_PREFIXLEN;
              p.u.prefix6 = pi->nd_opt_pi_prefix;

              if ((rp = nsm_rtadv_prefix_get (ha->rt_address, &p)))
                {
                  rp->prefixlen = pi->nd_opt_pi_prefix_len;
                  rp->vlifetime = pal_ntoh32 (pi->nd_opt_pi_valid_time);
                  rp->plifetime = pal_ntoh32 (pi->nd_opt_pi_preferred_time);
                  rp->flags = 0;
                  if (pi->nd_opt_pi_flags_reserved & PAL_ND_OPT_PI_FLAG_ONLINK)
                    SET_FLAG (rp->flags, PAL_ND_OPT_PI_FLAG_ONLINK);
                  if (pi->nd_opt_pi_flags_reserved & PAL_ND_OPT_PI_FLAG_AUTO)
                    SET_FLAG (rp->flags, PAL_ND_OPT_PI_FLAG_AUTO);
                  SET_FLAG (rp->flags, PAL_ND_OPT_PI_FLAG_ROUTER);

                  nsm_rtadv_ha_prefix_timer_reset (rp);
                }
            }
          break;
        case PAL_ND_OPT_HOMEAGENT_INFO:
          hi = (struct pal_nd_opt_home_agent_info *) ptr;
          ha->preference = pal_ntoh16 (hi->nd_opt_home_agent_info_preference);
          ha->lifetime = pal_ntoh16 (hi->nd_opt_home_agent_info_lifetime);
          hinfo_update = 1;
          break;
        default:
          break;
        }
      optlen = rtopt->nd_opt_len << 3;
      ptr += optlen;
      len -= optlen;
    }

  if (hinfo_update)
    nsm_rtadv_ha_sort (rif, ha, ha->preference);
  else
    ha->lifetime = pal_ntoh16 (ra->nd_ra_router_lifetime);
}

void
nsm_rtadv_ha_update (struct rtadv_if *rif,
                     int len, struct pal_in6_addr *src)
{
  struct pal_nd_router_advert *ra;
  struct rtadv_home_agent *ha;
  struct rtadv *top = rif->top;

  ra = (struct pal_nd_router_advert *) STREAM_DATA (top->ibuf);

  if (! CHECK_FLAG (ra->nd_ra_flags_reserved, PAL_ND_RA_FLAG_HOME_AGENT))
    {
      ha = nsm_rtadv_ha_lookup (rif, src);
      if (ha)
        nsm_rtadv_ha_reset (rif, ha);
    }
  else
    {
      ha = nsm_rtadv_ha_get (rif, src, 0);

      if (len > sizeof (struct pal_nd_router_advert))
        nsm_rtadv_ha_update_opt (rif, ha, ra, len);
      else
        ha->lifetime = pal_ntoh16 (ra->nd_ra_router_lifetime);

      nsm_rtadv_ha_timer_reset (ha);
    }
}
#endif /* HAVE_MIP6 */

void
nsm_rtadv_process_ra (struct rtadv_if *rif,
                      int len, struct pal_in6_addr *src)
{
  if (IS_NSM_DEBUG_RECV)
    zlog_info (NSM_ZG, "RTADV[%R%%%s]: Recv Router Advertisement",
               src, rif->ifp->name);

  if (len < sizeof (struct pal_nd_router_advert))
    {
      if (IS_NSM_DEBUG_RECV)
        zlog_warn (NSM_ZG, "RTADV[%R%%%s]: Invalid RA packet length: %d",
                   src, rif->ifp->name, len);
      return;
    }

#ifdef HAVE_MIP6
  /* Update home agent list. */
  if (NSM_RTADV_CONFIG_CHECK (rif, MIP6_HOME_AGENT))
    if (IN6_IS_ADDR_LINKLOCAL (src))
      nsm_rtadv_ha_update (rif, len, src);
#endif /* HAVE_MIP6 */
}

#ifdef HAVE_MIP6
void
nsm_rtadv_process_ha_discov (struct rtadv_if *rif,
                             int len, struct pal_in6_addr *src)
{
  struct pal_ha_discov_req *hareq;
  struct rtadv *top = rif->top;

  if (IS_NSM_DEBUG_RECV)
    zlog_info (NSM_ZG, "RTADV[%R%%%s]: Recv Home Agent Discovery Request",
               src, rif->ifp->name);

  if (len < sizeof (struct pal_ha_discov_req))
    {
      if (IS_NSM_DEBUG_RECV)
        zlog_warn (NSM_ZG, "RTADV[%R%%%s]: Invalid DHAAD packet length: %d",
                   src, rif->ifp->name, len);
      return;
    }

  hareq = (struct pal_ha_discov_req *) STREAM_DATA (top->ibuf);

  /* Send Home Agent Reply message. */
  if (NSM_RTADV_CONFIG_CHECK (rif, MIP6_HOME_AGENT))
    nsm_rtadv_send_ha_rep (rif, hareq->discov_req_id, src);
}

void
nsm_rtadv_process_ps (struct rtadv_if *rif, struct pal_in6_addr *src)
{
  if (IS_NSM_DEBUG_RECV)
    zlog_info (NSM_ZG, "RTADV[%R%%%s]: Recv Prefix Solicitation",
               src, rif->ifp->name);

  /* XXX Check if this source address is one of the MN's care-of address. */

  /* Send Prefix Advertisement. */
  nsm_rtadv_send_pa (rif, src);
}
#endif /* HAVE_MIP6 */

void
nsm_rtadv_process_packet (struct rtadv_if *rif,
                          int len, struct pal_in6_addr *src, int hoplimit)
{
  struct pal_icmp6_hdr *icmp;
  struct rtadv *top = rif->top;
  struct interface *ifp = rif->ifp;

  if (! NSM_RTADV_CONFIG_CHECK (rif, RA_ADVERTISE))
    return;

  /* ICMP message length check. */
  if (len < sizeof (struct pal_icmp6_hdr))
    {
      if (IS_NSM_DEBUG_RECV)
        zlog_warn (NSM_ZG, "RTADV[%R%%%s]: Invalid ICMP length: %d",
                   src, ifp->name, len);
      return;
    }

  icmp = (struct pal_icmp6_hdr *) STREAM_DATA (top->ibuf);

  /* ICMP message type check. */
  switch (icmp->icmp6_type)
    {
    case PAL_ND_ROUTER_SOLICIT:
      if (hoplimit >= 0 && hoplimit != 255)
        {
          if (IS_NSM_DEBUG_RECV)
            zlog_warn (NSM_ZG, "RTADV[%R%%%s]: Invalid RS packet hoplimit: %d",
                       src, ifp->name, hoplimit);
        }
      else
        nsm_rtadv_process_rs (rif, src);
      break;
    case PAL_ND_ROUTER_ADVERT:
      if (hoplimit >= 0 && hoplimit != 255)
        {
          if (IS_NSM_DEBUG_RECV)
            zlog_warn (NSM_ZG, "RTADV[%R%%%s]: Invalid RA packet hoplimit: %d",
                       src, ifp->name, hoplimit);
        }
      else
        nsm_rtadv_process_ra (rif, len, src);
      break;
#ifdef HAVE_MIP6
    case PAL_ICMP6_HADISCOV_REQUEST:
      nsm_rtadv_process_ha_discov (rif, len, src);
      break;
    case PAL_ICMP6_MOBILEPREFIX_SOLICIT:
      nsm_rtadv_process_ps (rif, src);
      break;
#endif /* HAVE_MIP6 */
    default:
      zlog_warn (NSM_ZG, "RTADV[%R%%%s]: Unknown ICMP packet type: %u",
                 src, ifp->name, icmp->icmp6_type);
      break;
    }

  return;
}

#define RTADV_MSG_SIZE 4096

#define NSM_RTADV_CMSG_BUF_LEN  1024

int
nsm_rtadv_read (struct thread *thread)
{
  struct rtadv *top = THREAD_ARG (thread);
  struct nsm_master *nm = top->nm;
  struct rtadv_if *rif;
  struct pal_msghdr msg;
  struct pal_sockaddr_in6 sin6;
  struct pal_in6_pktinfo *pkt;
  char adata[NSM_RTADV_CMSG_BUF_LEN];
  s_int32_t ifindex = 0;
  int *hoplimit;
  int len;

  top->t_read = NULL;

  stream_reset (top->ibuf);

  len = pal_in6_recv_packet (top->sock, &msg, adata, sizeof (adata),
                             STREAM_DATA (top->ibuf), STREAM_SIZE (top->ibuf),
                             &sin6);

  /* Register the next read thread. */
  top->t_read = thread_add_read (nzg, nsm_rtadv_read, top, top->sock);

  /* Get receiving ifindex. */
  pkt = pal_sock_in6_cmsg_pktinfo_get (&msg);
  if (pkt == NULL)
    {
      if (IS_NSM_DEBUG_RECV)
        zlog_warn (NSM_ZG, "RTADV: Unknown interface index");
      return -1;
    }

  ifindex = pkt->ipi6_ifindex;
  if ((rif = nsm_rtadv_if_lookup_by_index (nm, ifindex)) == NULL)
    {
      if (IS_NSM_DEBUG_RECV)
        zlog_warn (NSM_ZG, "RTADV[%s]: Recv on the wrong interface",
                   if_index2name (&nm->vr->ifm, ifindex));
      return -1;
    }

  /* Drop the packet which comes from myself. */
  if (if_lookup_by_ipv6_address (&nm->vr->ifm, &sin6.sin6_addr))
    {
      if (IS_NSM_DEBUG_RECV)
        zlog_warn (NSM_ZG, "RTADV[%s]: Recv from myself", rif->ifp->name);
      return -1;
    }

  hoplimit = pal_sock_in6_cmsg_hoplimit_get (&msg);

  if (len > 0)
    nsm_rtadv_process_packet (rif, len, &sin6.sin6_addr,
                              hoplimit ? *hoplimit : -1);

  return 0;
}


/* Router Advertisement CLI-APIs. */
int
nsm_rtadv_ra_set (u_int32_t vr_id, char *ifname)
{
  struct rtadv_if *rif;
  struct nsm_master *nm;

  nm = nsm_master_lookup_by_id (nzg, vr_id);
  if (nm == NULL)
    return NSM_API_SET_ERR_MASTER_NOT_EXIST;

  if ((rif = nsm_rtadv_if_lookup_by_name (nm, ifname)) == NULL) 
    return NSM_API_SET_ERR_IF_NOT_EXIST;

  if (if_is_loopback (rif->ifp))
    return NSM_API_SET_ERR_INVALID_IF;

  if (NSM_RTADV_CONFIG_CHECK (rif, RA_ADVERTISE))
    return NSM_API_SET_SUCCESS;

  if (rif->suppress_ra_run == 1)
    {
      rif->suppress_ra_run = 0;
      if (nsm_rtadv_if_start (rif) < 0)
        return NSM_API_SET_ERR_NO_IPV6_SUPPORT;
    }
  else
    {
      if (nsm_rtadv_if_start (rif) < 0)
        return NSM_API_SET_ERR_NO_IPV6_SUPPORT;
    }

  NSM_RTADV_CONFIG_SET (rif, RA_ADVERTISE);

  return NSM_API_SET_SUCCESS;
}

int
nsm_rtadv_ra_unset (u_int32_t vr_id, char *ifname)
{
  struct rtadv *top;
  struct rtadv_if *rif;
  struct nsm_master *nm;
  struct pal_in6_addr allnode;
  
  nm = nsm_master_lookup_by_id (nzg, vr_id);
  if (nm == NULL)
    return NSM_API_SET_ERR_MASTER_NOT_EXIST;

  if ((rif = nsm_rtadv_if_lookup_by_name (nm, ifname)) == NULL) 
    return NSM_API_SET_ERR_IF_NOT_EXIST;

  if (if_is_loopback (rif->ifp))
    return NSM_API_SET_ERR_INVALID_IF;

  top = rif->top;

  if (! NSM_RTADV_CONFIG_CHECK (rif, RA_ADVERTISE))
    return NSM_API_SET_SUCCESS;
  
  /* Instantaneous RA sending lifetime = 0. */
  rif->suppress_ra_run = 1;
  if ((pal_inet_pton (AF_INET6, ALLNODE, &allnode)) < 0)
    return  NSM_API_SET_ERR_INVALID_IPV6_ADDRESS;
  
  nsm_rtadv_send_ra_lifetime_zero (rif, &allnode);

  nsm_rtadv_if_stop (rif);

  NSM_RTADV_CONFIG_UNSET (rif, RA_ADVERTISE);

  return NSM_API_SET_SUCCESS;
}

int
nsm_rtadv_ra_interval_set (u_int32_t vr_id, char *ifname, u_int32_t interval)
{
  struct rtadv_if *rif;
  struct nsm_master *nm;

  nm = nsm_master_lookup_by_id (nzg, vr_id);
  if (nm == NULL)
    return NSM_API_SET_ERR_MASTER_NOT_EXIST;

  if ((rif = nsm_rtadv_if_lookup_by_name (nm, ifname)) == NULL)
    return NSM_API_SET_ERR_IF_NOT_EXIST;

  NSM_RTADV_CONFIG_SET (rif, RA_INTERVAL);
  rif->ra_interval = interval;

  if (NSM_RTADV_CONFIG_CHECK (rif, RA_ADVERTISE))
    {
      if (IS_NSM_DEBUG_DETAIL)
      zlog_warn (NSM_ZG, "RTADV[%s]: RESET RA timer at %d sec",
                 rif->ifp->name, rif->ra_interval);
      if (!((rif->min_ra_interval >= 3) && (rif->min_ra_interval <= (0.75 * rif->ra_interval))))
        {
          rif->min_ra_interval = (rif->ra_interval * RTADV_MIN_RA_INTERVAL_FACTOR);
          if (IS_NSM_DEBUG_DETAIL)
            zlog_warn (NSM_ZG, "RTADV[%s]: Forced MIN RA interval timer at %d sec",
                       rif->ifp->name, rif->min_ra_interval);
        }

    nsm_rtadv_if_timer_reset (rif, rif->ra_interval);
   }
  return NSM_API_SET_SUCCESS;
}

int
nsm_rtadv_ra_interval_unset (u_int32_t vr_id, char *ifname)
{
  struct rtadv_if *rif;
  struct nsm_master *nm;

  nm = nsm_master_lookup_by_id (nzg, vr_id);
  if (nm == NULL)
    return NSM_API_SET_ERR_MASTER_NOT_EXIST;

  if ((rif = nsm_rtadv_if_lookup_by_name (nm, ifname)) == NULL)
    return NSM_API_SET_ERR_IF_NOT_EXIST;

  rif->ra_interval = RTADV_MAX_RA_INTERVAL;

  if (NSM_RTADV_CONFIG_CHECK (rif, RA_ADVERTISE))
    {
      if (IS_NSM_DEBUG_DETAIL)
      zlog_warn (NSM_ZG, "RTADV[%s]: Reset RA timer at %d sec",
                 rif->ifp->name, rif->ra_interval);
      if (!((rif->min_ra_interval >= 3) && (rif->min_ra_interval <= (0.75 * rif->ra_interval))))
      {
        rif->min_ra_interval = (rif->ra_interval * RTADV_MIN_RA_INTERVAL_FACTOR);
        if (IS_NSM_DEBUG_DETAIL)
          zlog_warn (NSM_ZG, "RTADV[%s]: Forced MIN RA interval timer at %d sec",
                     rif->ifp->name, rif->min_ra_interval);
    }
      nsm_rtadv_if_timer_reset (rif, rif->ra_interval);
    }

 NSM_RTADV_CONFIG_UNSET (rif, RA_INTERVAL);

  return NSM_API_SET_SUCCESS;
}

int
nsm_rtadv_router_lifetime_set (u_int32_t vr_id, char *ifname,
                               u_int16_t lifetime)
{
  struct rtadv_if *rif;
  struct nsm_master *nm;

  nm = nsm_master_lookup_by_id (nzg, vr_id);
  if (nm == NULL)
    return NSM_API_SET_ERR_MASTER_NOT_EXIST;

  if ((rif = nsm_rtadv_if_lookup_by_name (nm, ifname)) == NULL)
    return NSM_API_SET_ERR_IF_NOT_EXIST;

  NSM_RTADV_CONFIG_SET (rif, ROUTER_LIFETIME);

  rif->router_lifetime = lifetime;

  return NSM_API_SET_SUCCESS;
}

int
nsm_rtadv_router_lifetime_unset (u_int32_t vr_id, char *ifname)
{
  struct rtadv_if *rif;
  struct nsm_master *nm;

  nm = nsm_master_lookup_by_id (nzg, vr_id);
  if (nm == NULL)
    return NSM_API_SET_ERR_MASTER_NOT_EXIST;

  if ((rif = nsm_rtadv_if_lookup_by_name (nm, ifname)) == NULL)
    return NSM_API_SET_ERR_IF_NOT_EXIST;

  rif->router_lifetime = RTADV_DEFAULT_ROUTER_LIFETIME;
  NSM_RTADV_CONFIG_UNSET (rif, ROUTER_LIFETIME);

  return NSM_API_SET_SUCCESS;
}

int
nsm_rtadv_router_reachable_time_set (u_int32_t vr_id, char *ifname,
                                     u_int32_t reachtime)
{
  int ret;
  struct rtadv_if *rif;
  struct nsm_master *nm;

  nm = nsm_master_lookup_by_id (nzg, vr_id);
  if (nm == NULL)
    return NSM_API_SET_ERR_MASTER_NOT_EXIST;

  if ((rif = nsm_rtadv_if_lookup_by_name (nm, ifname)) == NULL)
    return NSM_API_SET_ERR_IF_NOT_EXIST;
  
  /* Set the router reachable time in IP stack. */
  /* TODO VIV */
  ret = pal_ipv6_reachabletime_set (ifname, rif->ifp->ifindex, reachtime);
  if (ret == RESULT_OK)
    {
      NSM_RTADV_CONFIG_SET (rif, ROUTER_REACHABLE_TIME);
      rif->reachable_time = reachtime;
      return NSM_API_SET_SUCCESS;
    }
  return NSM_API_SET_ERR_NO_PERMISSION;
}

int
nsm_rtadv_router_reachable_time_unset (u_int32_t vr_id, char *ifname)
{
  int ret;
  struct rtadv_if *rif;
  struct nsm_master *nm;

  nm = nsm_master_lookup_by_id (nzg, vr_id);
  if (nm == NULL)
    return NSM_API_SET_ERR_MASTER_NOT_EXIST;

  if ((rif = nsm_rtadv_if_lookup_by_name (nm, ifname)) == NULL)
    return NSM_API_SET_ERR_IF_NOT_EXIST;

  /* Set the router reachable time in IP stack. */
  /* TODO VIV */
  ret = pal_ipv6_reachabletime_set (ifname, rif->ifp->ifindex, RTADV_DEFAULT_REACHABLE_TIME);
  if (ret == RESULT_OK)
    {
      rif->reachable_time = RTADV_DEFAULT_REACHABLE_TIME;
      NSM_RTADV_CONFIG_UNSET (rif, ROUTER_REACHABLE_TIME);
      return NSM_API_SET_SUCCESS;
    }
  return NSM_API_SET_ERR_NO_PERMISSION;
}

int
nsm_rtadv_router_retrans_timer_set (u_int32_t vr_id, char *ifname,
                                  u_int32_t retrans_time)
{
  struct rtadv_if *rif;
  struct nsm_master *nm;
  nm = nsm_master_lookup_by_id (nzg, vr_id);
  if (nm == NULL)
    return NSM_API_SET_ERR_MASTER_NOT_EXIST;
  if ((rif = nsm_rtadv_if_lookup_by_name (nm, ifname)) == NULL)
    return NSM_API_SET_ERR_IF_NOT_EXIST;
  NSM_RTADV_CONFIG_SET (rif, ROUTER_RETRANS_TIMER);
  rif->retrans_timer = retrans_time;
  return NSM_API_SET_SUCCESS;
}

int
nsm_rtadv_router_retrans_timer_unset (u_int32_t vr_id, char *ifname)
{
  struct rtadv_if *rif;
  struct nsm_master *nm;
  nm = nsm_master_lookup_by_id (nzg, vr_id);
  if (nm == NULL)
    return NSM_API_SET_ERR_MASTER_NOT_EXIST;
  if ((rif = nsm_rtadv_if_lookup_by_name (nm, ifname)) == NULL)
    return NSM_API_SET_ERR_IF_NOT_EXIST;
  rif->retrans_timer = RTADV_DEFAULT_RETRANS_TIMER;

  NSM_RTADV_CONFIG_UNSET (rif, ROUTER_RETRANS_TIMER);
  return NSM_API_SET_SUCCESS;
}

int
nsm_rtadv_curr_hoplimit_set (u_int32_t vr_id, char *ifname,
                             u_int8_t curr_hoplimit)
{
  struct rtadv_if *rif;
  struct nsm_master *nm;
  int err = RESULT_ERROR;
  nm = nsm_master_lookup_by_id (nzg, vr_id);
  if (nm == NULL)
    return NSM_API_SET_ERR_MASTER_NOT_EXIST;
  if ((rif = nsm_rtadv_if_lookup_by_name (nm, ifname)) == NULL)
    return NSM_API_SET_ERR_IF_NOT_EXIST;
  NSM_RTADV_CONFIG_SET (rif, CUR_HOPLIMIT);
  rif->curr_hoplimit = curr_hoplimit;
  err = pal_ipv6_curhoplimit_set (ifname, rif->ifp->ifindex, curr_hoplimit);
  if (err == RESULT_OK)
      return NSM_API_SET_SUCCESS;
  else
      return NSM_API_SET_ERROR;
}

int
nsm_rtadv_curr_hoplimit_unset (u_int32_t vr_id, char *ifname)
{
  struct rtadv_if *rif;
  struct nsm_master *nm;
  int err = RESULT_ERROR;
  nm = nsm_master_lookup_by_id (nzg, vr_id);
  if (nm == NULL)
    return NSM_API_SET_ERR_MASTER_NOT_EXIST;
  if ((rif = nsm_rtadv_if_lookup_by_name (nm, ifname)) == NULL)
    return NSM_API_SET_ERR_IF_NOT_EXIST;
  rif->curr_hoplimit = RTADV_DEFAULT_CUR_HOPLIMIT;
  err = pal_ipv6_curhoplimit_set (ifname, rif->ifp->ifindex, RTADV_DEFAULT_CUR_HOPLIMIT);
 
 NSM_RTADV_CONFIG_UNSET (rif, CUR_HOPLIMIT); 
 
 if (err == RESULT_OK)
    return NSM_API_SET_SUCCESS;
  else
    return NSM_API_SET_ERROR;
}

int
nsm_rtadv_linkmtu_set (u_int32_t vr_id, char *ifname)
{
  struct rtadv_if *rif;
  struct nsm_master *nm;
  nm = nsm_master_lookup_by_id (nzg, vr_id);
  if (nm == NULL)
    return NSM_API_SET_ERR_MASTER_NOT_EXIST;
  if ((rif = nsm_rtadv_if_lookup_by_name (nm, ifname)) == NULL)
    return NSM_API_SET_ERR_IF_NOT_EXIST;
  NSM_RTADV_CONFIG_SET (rif, LINKMTU);
   return NSM_API_SET_SUCCESS;
}

int
nsm_rtadv_linkmtu_unset (u_int32_t vr_id, char *ifname)
{
  struct rtadv_if *rif;
  struct nsm_master *nm;
  nm = nsm_master_lookup_by_id (nzg, vr_id);
  if (nm == NULL)
    return NSM_API_SET_ERR_MASTER_NOT_EXIST;
  if ((rif = nsm_rtadv_if_lookup_by_name (nm, ifname)) == NULL)
    return NSM_API_SET_ERR_IF_NOT_EXIST;
  NSM_RTADV_CONFIG_UNSET (rif, LINKMTU);
  return NSM_API_SET_SUCCESS;
}

int
nsm_rtadv_min_ra_interval_set (u_int32_t vr_id, char *ifname, u_int32_t min_interval)
{
  struct rtadv_if *rif;
  struct nsm_master *nm;
  nm = nsm_master_lookup_by_id (nzg, vr_id);
  if (nm == NULL)
    return NSM_API_SET_ERR_MASTER_NOT_EXIST;
  if ((rif = nsm_rtadv_if_lookup_by_name (nm, ifname)) == NULL)
    return NSM_API_SET_ERR_IF_NOT_EXIST;
  if (!((min_interval >= 3)&&(min_interval <= (0.75 * rif->ra_interval))))
     return NSM_API_SET_ERR_IPV6_RA_MIN_INTERVAL_VALUE;
  rif->min_ra_interval = min_interval;
  if (NSM_RTADV_CONFIG_CHECK (rif, RA_ADVERTISE))
     nsm_rtadv_if_timer_reset (rif, rif->ra_interval);
  NSM_RTADV_CONFIG_SET (rif, MIN_RA_INTERVAL);
  return NSM_API_SET_SUCCESS;
}

int
nsm_rtadv_min_ra_interval_unset (u_int32_t vr_id, char *ifname)
{
  struct rtadv_if *rif;
  struct nsm_master *nm;
  nm = nsm_master_lookup_by_id (nzg, vr_id);
  if (nm == NULL)
    return NSM_API_SET_ERR_MASTER_NOT_EXIST;
  if ((rif = nsm_rtadv_if_lookup_by_name (nm, ifname)) == NULL)
    return NSM_API_SET_ERR_IF_NOT_EXIST;
  
  if (!(((RTADV_MAX_RA_INTERVAL * RTADV_MIN_RA_INTERVAL_FACTOR) >= 3)
      &&((RTADV_MAX_RA_INTERVAL * RTADV_MIN_RA_INTERVAL_FACTOR) <= (0.75 * rif->ra_interval))))
  {
        return NSM_API_SET_ERR_IPV6_RA_MIN_INTERVAL_CONGRUENT;
  }
  rif->min_ra_interval = (RTADV_MIN_RA_INTERVAL_FACTOR * rif->ra_interval);
  if (NSM_RTADV_CONFIG_CHECK (rif, RA_ADVERTISE))
      nsm_rtadv_if_timer_reset (rif, rif->ra_interval);

  NSM_RTADV_CONFIG_UNSET (rif, MIN_RA_INTERVAL);

  return NSM_API_SET_SUCCESS;
}

int
nsm_rtadv_prefix_valid_lifetime_set (u_int32_t vr_id, char *ifname, u_int32_t valid_lifetime)
{
  struct rtadv_if *rif;
  struct nsm_master *nm;
  nm = nsm_master_lookup_by_id (nzg, vr_id);

  if (nm == NULL)
    return NSM_API_SET_ERR_MASTER_NOT_EXIST;

  if ((rif = nsm_rtadv_if_lookup_by_name (nm, ifname)) == NULL)
    return NSM_API_SET_ERR_IF_NOT_EXIST;

  rif->prefix_valid_lifetime = valid_lifetime;

  if (rif->prefix_valid_lifetime <= rif->prefix_preferred_lifetime)
    {
      NSM_RTADV_CONFIG_SET (rif, PREFIX_PREFERRED_LIFETIME);
      rif->prefix_preferred_lifetime = rif->prefix_valid_lifetime;
    }   

  if (NSM_RTADV_CONFIG_CHECK (rif, RA_ADVERTISE))
      nsm_rtadv_if_timer_reset (rif, rif->ra_interval);

  NSM_RTADV_CONFIG_SET (rif, PREFIX_VALID_LIFETIME);

  return NSM_API_SET_SUCCESS;
}

int
nsm_rtadv_prefix_valid_lifetime_unset (u_int32_t vr_id, char *ifname)
{
  struct rtadv_if *rif;
  struct nsm_master *nm;

  nm = nsm_master_lookup_by_id (nzg, vr_id);
  if (nm == NULL)
    return NSM_API_SET_ERR_MASTER_NOT_EXIST;

  if ((rif = nsm_rtadv_if_lookup_by_name (nm, ifname)) == NULL)
    return NSM_API_SET_ERR_IF_NOT_EXIST;

  rif->prefix_valid_lifetime = RTADV_DEFAULT_PREFIX_VALID_LIFETIME;
  if (NSM_RTADV_CONFIG_CHECK (rif, RA_ADVERTISE))
     nsm_rtadv_if_timer_reset (rif, rif->ra_interval);

  if (! NSM_RTADV_CONFIG_CHECK (rif, PREFIX_PREFERRED_LIFETIME))
    {
      NSM_RTADV_CONFIG_UNSET (rif, PREFIX_PREFERRED_LIFETIME);
      rif->prefix_preferred_lifetime = RTADV_DEFAULT_PREFIX_PREFERRED_LIFETIME;     
    }

  NSM_RTADV_CONFIG_UNSET (rif, PREFIX_VALID_LIFETIME);
  return NSM_API_SET_SUCCESS;
}

int
nsm_rtadv_prefix_preferred_lifetime_set (u_int32_t vr_id, char *ifname, u_int32_t preferred_lifetime)
{
  struct rtadv_if *rif;
  struct nsm_master *nm;
  nm = nsm_master_lookup_by_id (nzg, vr_id);
  if (nm == NULL)
    return NSM_API_SET_ERR_MASTER_NOT_EXIST;
  if ((rif = nsm_rtadv_if_lookup_by_name (nm, ifname)) == NULL)
    return NSM_API_SET_ERR_IF_NOT_EXIST;

  if(preferred_lifetime > rif->prefix_valid_lifetime)
    return NSM_API_SET_ERR_IPV6_RA_PREFIX_PREFERRED_LIFETIME;

  rif->prefix_preferred_lifetime = preferred_lifetime;

  if (NSM_RTADV_CONFIG_CHECK (rif, RA_ADVERTISE))
      nsm_rtadv_if_timer_reset (rif, rif->ra_interval);

  NSM_RTADV_CONFIG_SET (rif, PREFIX_PREFERRED_LIFETIME);

  return NSM_API_SET_SUCCESS;
}

int
nsm_rtadv_prefix_preferred_lifetime_unset (u_int32_t vr_id, char *ifname)
{
  struct rtadv_if *rif;
  struct nsm_master *nm;

  nm = nsm_master_lookup_by_id (nzg, vr_id);
  if (nm == NULL)
    return NSM_API_SET_ERR_MASTER_NOT_EXIST;

  if ((rif = nsm_rtadv_if_lookup_by_name (nm, ifname)) == NULL)
    return NSM_API_SET_ERR_IF_NOT_EXIST;

  rif->prefix_preferred_lifetime = RTADV_DEFAULT_PREFIX_PREFERRED_LIFETIME;

  if (NSM_RTADV_CONFIG_CHECK (rif, RA_ADVERTISE))
    nsm_rtadv_if_timer_reset (rif, rif->ra_interval);

  NSM_RTADV_CONFIG_UNSET (rif, PREFIX_PREFERRED_LIFETIME);

  return NSM_API_SET_SUCCESS;
}

int
nsm_rtadv_prefix_offlink_set (u_int32_t vr_id, char *ifname)
{
  struct rtadv_if *rif;
  struct nsm_master *nm;
  nm = nsm_master_lookup_by_id (nzg, vr_id);
  if (nm == NULL)
    return NSM_API_SET_ERR_MASTER_NOT_EXIST;
  if ((rif = nsm_rtadv_if_lookup_by_name (nm, ifname)) == NULL)
    return NSM_API_SET_ERR_IF_NOT_EXIST;
  NSM_RTADV_CONFIG_SET (rif, PREFIX_OFFLINK);
  return NSM_API_SET_SUCCESS;
}

int
nsm_rtadv_prefix_offlink_unset (u_int32_t vr_id, char *ifname)
{
  struct rtadv_if *rif;
  struct nsm_master *nm;
  nm = nsm_master_lookup_by_id (nzg, vr_id);
  if (nm == NULL)
    return NSM_API_SET_ERR_MASTER_NOT_EXIST;
  if ((rif = nsm_rtadv_if_lookup_by_name (nm, ifname)) == NULL)
    return NSM_API_SET_ERR_IF_NOT_EXIST;
  NSM_RTADV_CONFIG_UNSET (rif, PREFIX_OFFLINK);
  return NSM_API_SET_SUCCESS;
}

int
nsm_rtadv_prefix_noautoconf_set (u_int32_t vr_id, char *ifname)
{
  struct rtadv_if *rif;
  struct nsm_master *nm;
  nm = nsm_master_lookup_by_id (nzg, vr_id);
  if (nm == NULL)
    return NSM_API_SET_ERR_MASTER_NOT_EXIST;
  if ((rif = nsm_rtadv_if_lookup_by_name (nm, ifname)) == NULL)
    return NSM_API_SET_ERR_IF_NOT_EXIST;
  NSM_RTADV_CONFIG_SET (rif, PREFIX_NO_AUTOCONFIG);
  return NSM_API_SET_SUCCESS;
}

int
nsm_rtadv_prefix_noautoconf_unset (u_int32_t vr_id, char *ifname)
{
  struct rtadv_if *rif;
  struct nsm_master *nm;
  nm = nsm_master_lookup_by_id (nzg, vr_id);
  if (nm == NULL)
     return NSM_API_SET_ERR_MASTER_NOT_EXIST;
  if ((rif = nsm_rtadv_if_lookup_by_name (nm, ifname)) == NULL)
    return NSM_API_SET_ERR_IF_NOT_EXIST;
  NSM_RTADV_CONFIG_UNSET (rif, PREFIX_NO_AUTOCONFIG);
  return NSM_API_SET_SUCCESS;
}

int
nsm_rtadv_ra_managed_config_flag_set (u_int32_t vr_id, char *ifname)
{
  struct rtadv_if *rif;
  struct nsm_master *nm;

  nm = nsm_master_lookup_by_id (nzg, vr_id);
  if (nm == NULL)
    return NSM_API_SET_ERR_MASTER_NOT_EXIST;

  if ((rif = nsm_rtadv_if_lookup_by_name (nm, ifname)) == NULL)
    return NSM_API_SET_ERR_IF_NOT_EXIST;

  NSM_RTADV_CONFIG_SET (rif, RA_FLAG_MANAGED);

  return NSM_API_SET_SUCCESS;
}

int
nsm_rtadv_ra_managed_config_flag_unset (u_int32_t vr_id, char *ifname)
{
  struct rtadv_if *rif;
  struct nsm_master *nm;

  nm = nsm_master_lookup_by_id (nzg, vr_id);
  if (nm == NULL)
    return NSM_API_SET_ERR_MASTER_NOT_EXIST;

  if ((rif = nsm_rtadv_if_lookup_by_name (nm, ifname)) == NULL)
    return NSM_API_SET_ERR_IF_NOT_EXIST;

  NSM_RTADV_CONFIG_UNSET (rif, RA_FLAG_MANAGED);

  return NSM_API_SET_SUCCESS;
}

int
nsm_rtadv_ra_other_config_flag_set (u_int32_t vr_id, char *ifname)
{
  struct rtadv_if *rif;
  struct nsm_master *nm;

  nm = nsm_master_lookup_by_id (nzg, vr_id);
  if (nm == NULL)
    return NSM_API_SET_ERR_MASTER_NOT_EXIST;

  if ((rif = nsm_rtadv_if_lookup_by_name (nm, ifname)) == NULL)
    return NSM_API_SET_ERR_IF_NOT_EXIST;

  NSM_RTADV_CONFIG_SET (rif, RA_FLAG_OTHER);

  return NSM_API_SET_SUCCESS;
}

int
nsm_rtadv_ra_other_config_flag_unset (u_int32_t vr_id, char *ifname)
{
  struct rtadv_if *rif;
  struct nsm_master *nm;
  nm = nsm_master_lookup_by_id (nzg, vr_id);
  if (nm == NULL)
    return NSM_API_SET_ERR_MASTER_NOT_EXIST;

  if ((rif = nsm_rtadv_if_lookup_by_name (nm, ifname)) == NULL)
    return NSM_API_SET_ERR_IF_NOT_EXIST;

  NSM_RTADV_CONFIG_UNSET (rif, RA_FLAG_OTHER);

  return NSM_API_SET_SUCCESS;
}
 
int
nsm_rtadv_ra_prefix_set (u_int32_t vr_id, char *ifname,
                         struct pal_in6_addr *addr, u_char prefixlen,
                         u_int32_t vlifetime, u_int32_t plifetime,
                         u_char flags)
{
  struct prefix p;
  struct rtadv_if *rif;
  struct rtadv_prefix *rp;
  struct nsm_master *nm;

  nm = nsm_master_lookup_by_id (nzg, vr_id);
  if (nm == NULL)
    return NSM_API_SET_ERR_MASTER_NOT_EXIST;

  if ((rif = nsm_rtadv_if_lookup_by_name (nm, ifname)) == NULL)
    return NSM_API_SET_ERR_IF_NOT_EXIST;

  p.family = AF_INET6;
  p.prefixlen = prefixlen;
  p.u.prefix6 = *addr;

  if ((rp = nsm_rtadv_prefix_get (rif->rt_prefix, &p)))
    {
      NSM_RTADV_CONFIG_SET (rif, RA_PREFIX);

      rp->prefixlen = prefixlen;
      rp->vlifetime = vlifetime;
      rp->plifetime = plifetime;
      rp->flags = 0;
      if (CHECK_FLAG (flags, PAL_ND_OPT_PI_FLAG_ONLINK))
        SET_FLAG (rp->flags, PAL_ND_OPT_PI_FLAG_ONLINK);
      if (CHECK_FLAG (flags, PAL_ND_OPT_PI_FLAG_AUTO))
        SET_FLAG (rp->flags, PAL_ND_OPT_PI_FLAG_AUTO);
    }

  return NSM_API_SET_SUCCESS;
}

int
nsm_rtadv_ra_prefix_unset (u_int32_t vr_id, char *ifname,
                           struct pal_in6_addr *addr, u_char prefixlen)
{
  struct prefix p;
  struct rtadv_if *rif;
  struct rtadv_prefix *rp;
  struct nsm_master *nm;

  nm = nsm_master_lookup_by_id (nzg, vr_id);
  if (nm == NULL)
    return NSM_API_SET_ERR_MASTER_NOT_EXIST;

  if ((rif = nsm_rtadv_if_lookup_by_name (nm, ifname)) == NULL)
    return NSM_API_SET_ERR_IF_NOT_EXIST;

  p.family = AF_INET6;
  p.prefixlen = prefixlen;
  p.u.prefix6 = *addr;

  if ((rp = nsm_rtadv_prefix_lookup (rif->rt_prefix, &p)) == NULL)
    return NSM_API_SET_ERR_PREFIX_NOT_EXIST;

  nsm_rtadv_prefix_unset (rp);

  if (nsm_rtadv_prefix_count (rif->rt_prefix) <= 0)
    NSM_RTADV_CONFIG_UNSET (rif, RA_PREFIX);

  return NSM_API_SET_SUCCESS;
}

#ifdef HAVE_MIP6
int
nsm_rtadv_mip6_prefix_set (u_int32_t vr_id, char *ifname,
                           struct pal_in6_addr *addr, u_char prefixlen,
                           u_int32_t vlifetime, u_int32_t plifetime,
                           u_char flags)
{
  int ret;
  struct prefix p;
  struct rtadv_if *rif;
  struct rtadv_prefix *rp;
  struct pal_in6_addr allzero;
  struct prefix_ipv6 anycast;
  struct rtadv_home_agent *ha;
  struct nsm_master *nm;

  nm = nsm_master_lookup_by_id (nzg, vr_id);
  if (nm == NULL)
    return NSM_API_SET_ERR_MASTER_NOT_EXIST;

  if ((rif = nsm_rtadv_if_lookup_by_name (nm, ifname)) == NULL)
    return NSM_API_SET_ERR_IF_NOT_EXIST;


  if (prefixlen > 121)
    return NSM_API_SET_ERR_INVALID_PREFIX_LENGTH;

  p.family = AF_INET6;
  p.prefixlen = IPV6_MAX_PREFIXLEN;
  p.u.prefix6 = *addr;

  if (nsm_connected_check_ipv6 (rif->ifp, &p) == 0)
    return NSM_API_SET_ERR_INVALID_IPV6_ADDRESS;

  /* Install Home Agent subnet anycast address. */
  anycast = nsm_rtadv_ha_anycast (addr, prefixlen);
  if ((ret = nsm_ipv6_address_install (vr_id, rif->ifp->name,
                                       &anycast.prefix, anycast.prefixlen,
                                       NULL, NULL, 1)) < 0)
    return ret;

  pal_mem_set (&allzero, 0, sizeof (struct pal_in6_addr));
  ha = nsm_rtadv_ha_get (rif, &allzero, rif->ha_preference);
  ha->lifetime = rif->ha_lifetime;

  if ((rp = nsm_rtadv_prefix_get (ha->rt_address, &p)))
    {
      rp->prefixlen = prefixlen;
      rp->vlifetime = vlifetime;
      rp->plifetime = plifetime;
      rp->flags = 0;
      if (CHECK_FLAG (flags, PAL_ND_OPT_PI_FLAG_ROUTER))
        SET_FLAG (rp->flags, PAL_ND_OPT_PI_FLAG_ROUTER);
    }

  return NSM_API_SET_SUCCESS;
}

int
nsm_rtadv_mip6_prefix_unset (u_int32_t vr_id, char *ifname,
                             struct pal_in6_addr *addr, u_char prefixlen)
{
  struct prefix p;
  struct rtadv_if *rif;
  struct rtadv_prefix *rp;
  struct pal_in6_addr allzero;
  struct prefix_ipv6 anycast;
  struct rtadv_home_agent *ha;
  struct nsm_master *nm;

  nm = nsm_master_lookup_by_id (nzg, vr_id);
  if (nm == NULL)
    return NSM_API_SET_ERR_MASTER_NOT_EXIST;

  if ((rif = nsm_rtadv_if_lookup_by_name (nm, ifname)) == NULL)
    return NSM_API_SET_ERR_IF_NOT_EXIST;

  p.family = AF_INET6;
  p.prefixlen = IPV6_MAX_PREFIXLEN;
  p.u.prefix6 = *addr;

  if (! NSM_RTADV_CONFIG_CHECK (rif, MIP6_HOME_AGENT))
    return NSM_API_SET_SUCCESS;

  /* Delete Home Agent subnet anycast address. */
  pal_mem_set (&allzero, 0, sizeof (struct pal_in6_addr));
  if ((ha = nsm_rtadv_ha_lookup (rif, &allzero)))
    if ((rp = nsm_rtadv_prefix_lookup (ha->rt_address, &p)))
      {
        anycast = nsm_rtadv_ha_anycast (addr, prefixlen);
        if (nsm_connected_check_ipv6 (rif->ifp, (struct prefix *) &anycast))
          nsm_ipv6_address_uninstall (vr_id, rif->ifp->name,
                                      &anycast.prefix, anycast.prefixlen);
        nsm_rtadv_prefix_unset (rp);

        return NSM_API_SET_SUCCESS;
      }

  return NSM_API_SET_ERR_PREFIX_NOT_EXIST;
}
#endif /* HAVE_MIP6 */

void
nsm_rtadv_init (struct nsm_master *nm)
{
  nm->rtadv = nsm_rtadv_new (nm);
  nm->rtadv->sock = -1;
  nm->rtadv->obuf = stream_new (RTADV_MSG_SIZE);
  nm->rtadv->ibuf = stream_new (RTADV_MSG_SIZE);
}

void
nsm_rtadv_finish (struct nsm_master *nm)
{
  struct interface *ifp;
  struct listnode *node;
  struct rtadv *top;

  top = nm->rtadv;
  if (top == NULL)
    return;

  /* Free the router advertisement interfaces.  */
  LIST_LOOP (nm->vr->ifm.if_list, ifp, node)
    nsm_rtadv_if_free (ifp);

  if (top->obuf != NULL)
    stream_free (top->obuf);
  if (top->ibuf != NULL)
    stream_free (top->ibuf);

  /* Free the top structure.  */
  nsm_rtadv_free (top);

  nm->rtadv = NULL;
}

#endif /* HAVE_RTADV */
