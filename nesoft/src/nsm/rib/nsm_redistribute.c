/* Copyright (C) 2001-2003  All Rights Reserved. */

#include "pal.h"
#include "lib.h"

#ifdef HAVE_MPLS
#include "mpls.h"
#endif /* HAVE_MPLS */

#ifdef HAVE_TE
#include "admin_grp.h"
#endif /* HAVE_TE */

#include "nsmd.h"
#include "nsm_vrf.h"
#include "rib/nsm_table.h"
#include "nsm_server.h"
#include "rib/nsm_server_rib.h"
#include "rib/nsm_redistribute.h"
#include "rib/rib.h"
#ifdef HAVE_VRF
#include "nsm_rib_vrf.h"
#endif /* HAVE_VRF */

#include "ptree.h"
/* Check loopback address, invalid route and multicast address.  */
int
nsm_check_addr (struct prefix *p)
{
#ifdef HAVE_IPV6
  struct pal_in6_addr *prefix6;
#endif /* HAVE_IPV6 */

  if (p->family == AF_INET)
    {
      u_int32_t addr;

      addr = p->u.prefix4.s_addr;
      addr = pal_ntoh32 (addr);

      if ((! addr && (p->prefixlen > 0))
          || IPV4_NET127 (addr)
          || IN_CLASSD (addr))
        return 0;
      
    }
#ifdef HAVE_IPV6
  else if (NSM_CAP_HAVE_IPV6 && p->family == AF_INET6)
    {
      prefix6 = &p->u.prefix6; 

      if ((! prefix6->s6_addr32[0]
           && ! prefix6->s6_addr32[1]
           && ! prefix6->s6_addr32[2]
           && ! prefix6->s6_addr32[3])
          && p->prefixlen > 0)
        return 0;
          
      if (IN6_IS_ADDR_LOOPBACK (prefix6))
        return 0;
      if (IN6_IS_ADDR_LINKLOCAL (prefix6))
        return 0;
    }
#endif /* HAVE_IPV6 */
  return 1;
}

#ifdef HAVE_INTERFACE_MANAGE
int
nsm_check_rib (struct rib *rib)
{
  struct nexthop *nexthop;
  struct interface *ifp;
  struct apn_vrf *vrf = rib->vrf->vrf;

  if (rib->type != APN_ROUTE_CONNECT)
    return 1;

  for (nexthop = rib->nexthop; nexthop; nexthop = nexthop->next)
    {
      ifp = ifv_lookup_by_index (&vrf->ifv, nexthop->ifindex);
      if (ifp && CHECK_FLAG (ifp->status, NSM_INTERFACE_MANAGE))
        return 0;
    }

  return 1;
}
#endif /* HAVE_INTERFACE_MANAGE */

/* If the route is default route, return 1.  */
int
is_default (struct prefix *p)
{
  if (p->family == AF_INET)
    if (p->u.prefix4.s_addr == 0 && p->prefixlen == 0)
      return 1;
  return 0;
}

/* Redistribute routes when NSM client is connected. */
void
nsm_redistribute (struct nsm_vrf *vrf, u_char type, afi_t afi,
                  struct nsm_server_entry *nse) /* XXX */
{
  struct rib *rib;
  struct nsm_ptree_node *rn;
  struct prefix p;

  if (afi == 0 || afi == AFI_IP)
    for (rn = nsm_ptree_top (vrf->RIB_IPV4_UNICAST);
         rn; rn = nsm_ptree_next (rn))
      for (rib = rn->info; rib; rib = rib->next)
         {
          pal_mem_set(&p, 0, sizeof(struct prefix));
          p.prefixlen = rn->key_len;
          p.family = rn->tree->ptype;
          NSM_PTREE_KEY_COPY(&p.u.prefix4,  rn);
           if (CHECK_FLAG (rib->flags, RIB_FLAG_SELECTED) 
               && rib->type == type 
               && rib->distance != DISTANCE_INFINITY
               && nsm_check_addr (&p)
#ifdef HAVE_INTERFACE_MANAGE
               && nsm_check_rib (rib)
#endif /* HAVE_INTERFACE_MANAGE */
               )
             {
               if (!CHECK_FLAG (nse->redist->flag, ENABLE_INSTANCE_ID)) 
                 nsm_send_ipv4_add (nse, 0, &p, rn->key_len, rib, vrf, 1, NULL);
               else if (rib->pid == nse->redist->pid)
                 nsm_send_ipv4_add (nse, 0, &p, rn->key_len, rib, vrf, 1, NULL);
             }
          }
#ifdef HAVE_IPV6
  IF_NSM_CAP_HAVE_IPV6
    {
      if (afi == 0 || afi == AFI_IP6)
        for (rn = nsm_ptree_top (vrf->RIB_IPV6_UNICAST);
             rn; rn = nsm_ptree_next (rn))
          for (rib = rn->info; rib; rib = rib->next)
             {
                pal_mem_set(&p, 0, sizeof(struct prefix));
                p.prefixlen = rn->key_len;
                p.family = rn->tree->ptype;
                NSM_PTREE_KEY_COPY (&p.u.prefix6,  rn);
               if (CHECK_FLAG (rib->flags, RIB_FLAG_SELECTED)
                  && rib->type == type 
                  && rib->distance != DISTANCE_INFINITY
                  && nsm_check_addr (&p)
#ifdef HAVE_INTERFACE_MANAGE
                  && nsm_check_rib (rib)
#endif /* HAVE_INTERFACE_MANAGE */
                  )
                {
                  /* If PM sends the PID to be redistributed 
                     then consider that PID for selecting the rib */
                  if (!CHECK_FLAG (nse->redist->flag, ENABLE_INSTANCE_ID)
                       || rib->pid == nse->redist->pid)  
                     nsm_send_ipv6_add (nse, 0, &p, rn->key_len, rib, vrf, 1, NULL);
                }
              }
    }
#endif /* HAVE_IPV6 */
}

/* Redistribute default route when client is connected.  */
void
nsm_redistribute_default (struct nsm_vrf *vrf, afi_t afi,
                          struct nsm_server_entry *nse)
{
  struct nsm_ptree_node *rn;
  struct rib *rib;
  struct prefix p;

  /* Set default to prefix. */
  pal_mem_set (&p, 0, sizeof (struct prefix));

  /* IPv4 default route.  */
  if (afi == 0 || afi == AFI_IP)
    {
      p.family = AF_INET;
      rn = nsm_ptree_node_lookup (vrf->RIB_IPV4_UNICAST, 
                                  (u_char *) &p.u.prefix4,
                                  p.prefixlen);
      if (rn)
        {
          for (rib = rn->info; rib; rib = rib->next)
            if (CHECK_FLAG (rib->flags, RIB_FLAG_SELECTED))
              if (rib->distance != DISTANCE_INFINITY)
                if (rib->client_id != nse->nsc->client_id)
                  {
                    pal_mem_set(&p, 0, sizeof(struct prefix));
                    p.prefixlen = rn->key_len;
                    NSM_PTREE_KEY_COPY(&p.u.prefix4, rn);
                    nsm_send_ipv4_add (nse, 0, &p, rn->key_len, rib, vrf, 1, NULL);
                  }

          nsm_ptree_unlock_node (rn);
        }
    }

#ifdef HAVE_IPV6
  /* IPv6 default route. */
  IF_NSM_CAP_HAVE_IPV6
    if (afi == 0 || afi == AFI_IP6)
      {
        pal_mem_set(&p, 0, sizeof(struct prefix));
        p.family = AF_INET6;
        rn = nsm_ptree_node_lookup (vrf->RIB_IPV6_UNICAST, 
                                    (u_char *) &p.u.prefix6,
                                    p.prefixlen);
        if (rn)
          {
            for (rib = rn->info; rib; rib = rib->next)
              if (CHECK_FLAG (rib->flags, RIB_FLAG_SELECTED))
                if (rib->distance != DISTANCE_INFINITY)
                  if (rib->client_id != nse->nsc->client_id)
                    {
                      pal_mem_set(&p, 0, sizeof(struct prefix));
                      p.prefixlen = rn->key_len;
                      NSM_PTREE_KEY_COPY(&p.u.prefix6, rn);
                      nsm_send_ipv6_add (nse, 0, &p, rn->key_len, rib, vrf, 1, NULL);
                    }

            nsm_ptree_unlock_node (rn);
          }
      }
#endif /* HAVE_IPV6 */
}

/* New route is selected.  Redistribute the route to client.  */
void
nsm_redistribute_add (struct prefix *p, struct rib *rib)
{
  struct nsm_vrf *vrf = rib->vrf;
  struct nsm_server_entry *nse;
  afi_t afi = AFI_IP;
  int i;

  /* Add DISTANCE_INFINITY check. */
  if (rib->distance == DISTANCE_INFINITY)
    return;

  /* Invalid route for redistribute.  */
  if (! nsm_check_addr (p))
    return;

#ifdef HAVE_INTERFACE_MANAGE
  if (! nsm_check_rib (rib))
    return;
#endif /* HAVE_INTERFACE_MANAGE */

  if (p->family == AF_INET)
    afi = AFI_IP;
#ifdef HAVE_IPV6
  else if (NSM_CAP_HAVE_IPV6 && p->family == AF_INET6)
    afi = AFI_IP6;
#endif /* HAVE_IPV6 */

  /* Default route redistribution.  */
  if (is_default (p))
    for (i = 0; i < vector_max (vrf->afi[afi].redist[APN_ROUTE_DEFAULT]); i++)
      if ((nse = vector_slot (vrf->afi[afi].redist[APN_ROUTE_DEFAULT], i)))
        if (rib->client_id != nse->nsc->client_id)
          {
            if (afi == AFI_IP)
              nsm_send_ipv4_add (nse, 0, p, 0, rib, vrf, 1, NULL);
#ifdef HAVE_IPV6
            else if (NSM_CAP_HAVE_IPV6 && afi == AFI_IP6)
              nsm_send_ipv6_add (nse, 0, p, 0, rib, vrf, 1, NULL);
#endif /* HAVE_IPV6 */
          }

  /* Normal route process.  */
  for (i = 0; i < vector_max (vrf->afi[afi].redist[rib->type]); i++)
    if ((nse = vector_slot (vrf->afi[afi].redist[rib->type], i)) != NULL)
      {
        if (afi == AFI_IP)
          nsm_send_ipv4_add (nse, 0, p, 0, rib, vrf, 1, NULL);
#ifdef HAVE_IPV6
        else if (NSM_CAP_HAVE_IPV6 && afi == AFI_IP6)
          nsm_send_ipv6_add (nse, 0, p, 0, rib, vrf, 1, NULL);
#endif /* HAVE_IPV6 */    
      }
}

/* Selected route is deleted.  Redistribute the information to the
   client.  */
void
nsm_redistribute_delete (struct prefix *p, struct rib *rib)
{
  struct nsm_vrf *vrf = rib->vrf;
  struct nsm_server_entry *nse;
  afi_t afi = AFI_IP;
  int i;

  /* Invalid route for redistribute.  */
  if (! nsm_check_addr (p))
    return;

#ifdef HAVE_INTERFACE_MANAGE
  if (! nsm_check_rib (rib))
    return;
#endif /* HAVE_INTERFACE_MANAGE */

  if (p->family == AF_INET)
    afi = AFI_IP;
#ifdef HAVE_IPV6
  else if (NSM_CAP_HAVE_IPV6 && p->family == AF_INET6)
    afi = AFI_IP6;
#endif /* HAVE_IPV6 */

  /* Default route redistribution.  */
  if (is_default (p))
    for (i = 0; i < vector_max (vrf->afi[afi].redist[APN_ROUTE_DEFAULT]); i++)
      if ((nse = vector_slot (vrf->afi[afi].redist[APN_ROUTE_DEFAULT], i)))
        if (rib->client_id != nse->nsc->client_id)
          {
            if (afi == AFI_IP)
              nsm_send_ipv4_delete (nse, p, 0, rib, vrf, 1);
#ifdef HAVE_IPV6
            else if (NSM_CAP_HAVE_IPV6 && afi == AFI_IP6)
              nsm_send_ipv6_delete (nse, p, 0, rib, vrf, 1);
#endif /* HAVE_IPV6 */
          }
  
  for (i = 0; i < vector_max (vrf->afi[afi].redist[rib->type]); i++)
    if ((nse = vector_slot (vrf->afi[afi].redist[rib->type], i)) != NULL)
      {
        if (afi == AFI_IP)
          nsm_send_ipv4_delete (nse, p, 0, rib, vrf, 1);
#ifdef HAVE_IPV6
        else if (NSM_CAP_HAVE_IPV6 && afi == AFI_IP6)
          nsm_send_ipv6_delete (nse, p, 0, rib, vrf, 1);
#endif /* HAVE_IPV6 */
      }
}
