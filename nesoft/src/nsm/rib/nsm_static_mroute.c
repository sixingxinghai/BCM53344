/* Copyright (C) 2005  All Rights Reserved. */

#include "pal.h"
#include "lib.h"
#include "message.h"
#include "network.h"
#include "nsmd.h"
#include "nexthop.h"
#include "nsm_message.h"
#include "log.h"
#include "snprintf.h"

#include "rib.h"
#include "nsm_server.h"
#include "nsm_server_rib.h"
#include "nsm_api.h"
#include "nsm/nsm_cli.h"
#include "nsm_debug.h"
#include "nsm_table.h"
#include "nsm_static_mroute.h"
#include "ptree.h"
extern int nsm_cli_return (struct cli *cli, int ret);
extern u_char nsm_static_nexthop_type_get (afi_t afi, 
    union nsm_nexthop *gate, char *ifname);
extern int nsm_nexthop_active_update (struct nsm_ptree_node *rn, 
    struct rib *rib, int set);
extern void nsm_nexthop_free (struct nexthop *nexthop);
extern void nsm_nexthop_delete (struct rib *rib, struct nexthop *nexthop);
extern const char * route_type_str (u_char type);

#define NSM_MROUTE_GATEWAY_INVALID              0
#define NSM_MROUTE_GATEWAY_IFNAME               1
#define NSM_MROUTE_GATEWAY_ADDRESS              2
#define NSM_MROUTE_GATEWAY_BCAST                3

#define NSM_MROUTE_CONFIG_NEXTHOP_CHANGE        (1 << 0)
#define NSM_MROUTE_CONFIG_DISTANCE_CHANGE       (1 << 1)

/* Core function for choosing RPF route from multicast route and 
 * unicast routes */
struct rib *
nsm_mrib_compare (struct rib *uc_rib, struct rib *mc_rib)
{
  struct nsm_mrib *mrib;
  struct rib *select;

  for (select = mc_rib; select; select = select->next)
    {
      /* Ignore multicast routes with non-active nexthops */
      if (! CHECK_FLAG (select->nexthop->flags, NEXTHOP_FLAG_ACTIVE))
        continue;

      mrib = RIB2MRIB (select);
      /* If a multicast dependent route type matches the route type of 
       * unicast rib, or if the multicast route is not dependent 
       * then we have found a candidate.
       */
      if ((mrib->uc_rtype == APN_ROUTE_DEFAULT) ||
          ((uc_rib != NULL) && (uc_rib->type == mrib->uc_rtype)))
        break;
    }

  /* Finally, compare the distances. The unicast rib is chosen
   * only if it's distance is better (strictly less) than that of
   * the multicast rib of if the multicast rib does not exist. 
   */
  if (select != NULL && uc_rib != NULL)
    {
      if (uc_rib->distance < select->distance)
        select = uc_rib;
    }
  else if (select == NULL)
    select = uc_rib;

  return select;
}

struct rib *
nsm_mrib_lookup (struct nsm_vrf *nv,
                afi_t afi,
                struct prefix *p,
                struct nsm_ptree_node **rn_ret,
                u_int32_t exclude_proto)
{
  struct nsm_ptree_node *rn = NULL;
  struct rib *uc_match = NULL;
  struct rib *select = NULL;

  /* Sanity check. */
  if (afi != AFI_IP
#ifdef HAVE_IPV6
      && afi != AFI_IP6
#endif /* HAVE_IPV6 */
      )
    return NULL;

  uc_match = nsm_rib_lookup (nv, afi, p, rn_ret, exclude_proto, 0);

  if (afi == AFI_IP)
    rn = nsm_ptree_node_lookup (nv->afi[afi].rib[SAFI_MULTICAST], 
                                (u_char *) &p->u.prefix4,
                                p->prefixlen);
#ifdef HAVE_IPV6
  else if (afi == AFI_IP6)
    rn = nsm_ptree_node_lookup (nv->afi[afi].rib[SAFI_MULTICAST],
                                (u_char *) &p->u.prefix6,
                                p->prefixlen);
#endif
  /* If no multicast rib, then select the unicast rib */
  if (rn == NULL || rn->info == NULL)
    return uc_match;

  /* Unlock for lookup */
  nsm_ptree_unlock_node (rn);

  select =  nsm_mrib_compare (uc_match, rn->info);
  if (select != NULL && select != uc_match)
    if (rn_ret != NULL)
      *rn_ret = (RIB2MRIB (select))->rn;

  return select;
}

struct rib *
nsm_mrib_match (struct nsm_vrf *nv,
                afi_t afi,
                struct prefix *p,
                struct nsm_ptree_node **rn_ret,
                u_int32_t exclude_proto)
{
  struct nsm_ptree_node *rn = NULL;
  struct rib *uc_match = NULL;
  struct rib *select = NULL;

  /* Sanity check. */
  if (afi != AFI_IP
#ifdef HAVE_IPV6
      && afi != AFI_IP6
#endif /* HAVE_IPV6 */
      )
    return NULL;

  uc_match = nsm_rib_match (nv, afi, p, rn_ret, exclude_proto, 0);

  if (afi == AFI_IP)
    rn = nsm_ptree_node_match (nv->afi[afi].rib[SAFI_MULTICAST],
                               (u_char *) &p->u.prefix4,
                                p->prefixlen);
#ifdef HAVE_IPV6

  else if (afi == AFI_IP6)
    rn = nsm_ptree_node_match (nv->afi[afi].rib[SAFI_MULTICAST],
                               (u_char *)&p->u.prefix6,
                                p->prefixlen);
#endif

  /* If no multicast rib, then select the unicast rib */
  if (rn == NULL || rn->info == NULL)
    return uc_match;

  /* Unlock for lookup */
  nsm_ptree_unlock_node (rn);

  select = nsm_mrib_compare (uc_match, rn->info);

  if (select != NULL && select != uc_match)
    *rn_ret = (RIB2MRIB (select))->rn;

  return select;
}

struct nexthop *
nsm_mrib_static_nexthop_add (struct rib *rib, struct nsm_mroute_config *mc)
{
  struct nexthop *nexthop = NULL;

  if (mc->type == NEXTHOP_TYPE_IFNAME)
    nexthop = nsm_nexthop_ifname_add (rib, mc->ifname, ROUTE_TYPE_OTHER);
  else if (mc->type == NEXTHOP_TYPE_IPV4)
    nexthop = nsm_nexthop_ipv4_add (rib, &mc->gate.ipv4, ROUTE_TYPE_OTHER);
  else if (mc->type == NEXTHOP_TYPE_IPV4_IFNAME)
    nexthop = nsm_nexthop_ipv4_ifname_add (rib, &mc->gate.ipv4, mc->ifname,
                                           ROUTE_TYPE_OTHER);

#ifdef HAVE_IPV6
  else if (mc->type == NEXTHOP_TYPE_IPV6)
    nexthop = nsm_nexthop_ipv6_add (rib, &mc->gate.ipv6);
  else if (mc->type == NEXTHOP_TYPE_IPV6_IFNAME)
    nexthop = nsm_nexthop_ipv6_ifname_add (rib, &mc->gate.ipv6, mc->ifname);
#endif /* HAVE_IPV6 */

  if (nexthop)
    SET_FLAG (nexthop->flags, NEXTHOP_FLAG_MROUTE);

  return nexthop;
}

/* Add RIB to tail of the route node. */
void
nsm_mrib_addnode (struct nsm_ptree_node *rn, struct rib *rib)
{
  struct rib *last;

  for (last = rn->info; last && last->next; last = last->next)
    ;

  if (last)
    last->next = rib;
  else
    rn->info = rib;

  rib->prev = last;

  return;
}

void
nsm_mrib_delnode (struct nsm_ptree_node *rn, struct rib *rib)
{
  if (rib->next)
    rib->next->prev = rib->prev;
  if (rib->prev)
    rib->prev->next = rib->next;
  else
    rn->info = rib->next;

  rib->next = rib->prev = NULL;
  return;
}

void
nsm_mrib_free (struct nsm_mrib *mrib)
{
  struct nexthop *nexthop;
  struct nexthop *next;

  for (nexthop = mrib->rib.nexthop; nexthop; nexthop = next)
    {
      next = nexthop->next;
      nsm_nexthop_free (nexthop);
    }

  XFREE (MTYPE_NSM_MRIB, mrib);

  return;
}

/* Allocate a new RIB structure.  */
struct nsm_mrib *
nsm_mrib_new (u_char type, u_char distance, u_char flags, u_char ext_flags,
             u_int32_t metric, u_char uc_rtype, struct nsm_vrf *vrf)
{
  struct nsm_mrib *mrib;

  mrib = XCALLOC (MTYPE_NSM_RIB, sizeof (struct nsm_mrib));
  if (! mrib)
    return NULL;

  mrib->rib.type = type;
  mrib->rib.flags = flags;
  mrib->rib.ext_flags = ext_flags;
  mrib->rib.metric = metric;
  mrib->rib.distance = distance;
  mrib->rib.vrf = vrf;
  mrib->rib.uptime = pal_time_current (NULL);

  mrib->uc_rtype = uc_rtype;

  return mrib;
}

struct nsm_mrib *
nsm_mrib_lookup_by_type (struct nsm_vrf *nv, afi_t afi,
                        struct prefix *p, int type, u_char uc_rtype)
{
  struct nsm_ptree_node *rn = NULL;
  struct rib *match;
  struct nsm_mrib *mrib;

  /* Sanity check. */
  if (afi != AFI_IP
#ifdef HAVE_IPV6
      && afi != AFI_IP6
#endif /* HAVE_IPV6 */
      )
    return NULL;
  if (afi == AFI_IP)
  rn = nsm_ptree_node_lookup (nv->afi[afi].rib[SAFI_MULTICAST],
                               (u_char *) &p->u.prefix4,
                                p->prefixlen);
#ifdef HAVE_IPV6
  else if (afi == AFI_IP6)
    rn = nsm_ptree_node_lookup (nv->afi[afi].rib[SAFI_MULTICAST],
                                (u_char *) &p->u.prefix6,
                                p->prefixlen);
#endif

  if (rn == NULL)
    return NULL;

  /* Unlock node. */
  nsm_ptree_unlock_node (rn);

  /* Pick the same type of route. */
  for (match = rn->info; match; match = match->next)
    {
      if (match->type != type)
        continue;
      mrib = RIB2MRIB (match);
      if (mrib->uc_rtype == uc_rtype)
        return mrib;
    }

  return NULL;
}

/* Install static multicast route into RIB. */
int
nsm_static_mroute_rib_add (struct nsm_vrf *nv, struct prefix *p,
                    struct nsm_mroute_config *mc)
{
  struct nsm_vrf_afi *nva = &nv->afi[mc->afi];
  struct nsm_ptree_node *rn = NULL;
  struct nsm_mrib *mrib = NULL;


  if (p->family == AF_INET)
    rn = nsm_ptree_node_get (nva->rib[SAFI_MULTICAST], 
                             (u_char *) &p->u.prefix4,p->prefixlen);
#ifdef HAVE_IPV6
  else if (p->family == AF_INET6)
    rn = nsm_ptree_node_get (nva->rib[SAFI_MULTICAST], 
                             (u_char *) &p->u.prefix6,p->prefixlen);
#endif
  if (!rn)
    return NSM_API_SET_ERR_MROUTE_NO_MEM;

  /* Always set internal so that recursive nexthops are resolved */
  mrib = nsm_mrib_new (APN_ROUTE_STATIC, mc->distance, RIB_FLAG_INTERNAL, 
      RIB_EXT_FLAG_MROUTE, 0, mc->uc_rtype, nv);
  if (!mrib)
    {
      nsm_ptree_unlock_node (rn);
      return NSM_API_SET_ERR_MROUTE_NO_MEM;
    }

  /* Add the new nexthop to the RIB */
  if ((nsm_mrib_static_nexthop_add (&mrib->rib, mc)) == NULL)
    {
      nsm_mrib_free (mrib);
      nsm_ptree_unlock_node (rn);
      return NSM_API_SET_ERR_MROUTE_NO_MEM;
    }

  if (IS_NULL_INTERFACE_STR (mc->ifname))
    SET_FLAG (mrib->rib.flags, RIB_FLAG_BLACKHOLE);

  /* Add to the route node list */
  nsm_mrib_addnode (rn, &mrib->rib);
  mrib->rn = rn;

  /* Check whether nexthop is active or not */
  nsm_nexthop_active_update (rn, &mrib->rib, 1);

  /* Process addition of MRIB only if the nexthop is active */
  if (CHECK_FLAG (mrib->rib.flags, RIB_FLAG_CHANGED))
    {
      /* Re-process the multicast nexthop registrations */
      nsm_mnh_reg_reprocess (nv, mc->afi, p, p); 
    }

  return NSM_SUCCESS;
}

/* Install static multicast route into RIB. */
int
nsm_static_mroute_rib_update (struct nsm_vrf *nv, struct prefix *p,
                    struct nsm_mroute_config *mc, u_char change_flags)
{
  struct nsm_mrib *mrib = NULL;
  struct nexthop *nexthop = NULL, *old_nexthop = NULL;
  int ret = NSM_SUCCESS;
  int del_mrib = 0;

  mrib = nsm_mrib_lookup_by_type (nv, mc->afi, p, APN_ROUTE_STATIC,
      mc->uc_rtype);
  if (mrib == NULL)
    return NSM_API_ERR_MROUTE_NO_MRIB;

  /* Process nexthop update */
  if (CHECK_FLAG (change_flags, NSM_MROUTE_CONFIG_NEXTHOP_CHANGE))
    {
      /* Since static multicast routes have only one nexthop, delete
       * and replace
       */
      old_nexthop = mrib->rib.nexthop;
      nsm_nexthop_delete (&mrib->rib, old_nexthop);

      /* Add the nexthop to the RIB */
      if ((nexthop = nsm_mrib_static_nexthop_add (&mrib->rib, mc)) == NULL)
        {
          nsm_mrib_delnode (mrib->rn, &mrib->rib);
          del_mrib = 1;

          ret = NSM_API_SET_ERR_MROUTE_NO_MEM;
          goto process;
        }

      if (IS_NULL_INTERFACE_STR (mc->ifname))
        SET_FLAG (mrib->rib.flags, RIB_FLAG_BLACKHOLE);

      /* Copy the NEXTHOP_FLAG_ACTIVE from old_nexthop to new nexthop
       * so that if the the old nexthop was active and new nexthop is
       * inactive, the rib will be marked as changed.
       */
      nexthop->flags |= (old_nexthop->flags & NEXTHOP_FLAG_ACTIVE);

      /* Set the nexthop active number to 0 so that 
       * nsm_nexthop_active_update() will mark the rib as changed
       * when the nexthop gateway is changed on the same interface.
       */
      mrib->rib.nexthop_active_num = 0;

      /* Check whether new nexthop is active or not */
      nsm_nexthop_active_update (mrib->rn, &mrib->rib, 1);
    }

  /* Process distance update */
  if (CHECK_FLAG (change_flags, NSM_MROUTE_CONFIG_DISTANCE_CHANGE))
    mrib->rib.distance = mc->distance;

process:
  /* Process the update of MRIB  */
  /* Process addition of MRIB only if the rib has changed or
   * the distance has changed */
  if (del_mrib || CHECK_FLAG (mrib->rib.flags, RIB_FLAG_CHANGED) ||
      CHECK_FLAG (change_flags, NSM_MROUTE_CONFIG_DISTANCE_CHANGE))
    {
      /* Re-process the multicast nexthop registrations */
      nsm_mnh_reg_reprocess (nv, mc->afi, p, p); 
    }

  if (del_mrib)
    {
      nsm_ptree_unlock_node (mrib->rn);
      nsm_mrib_free (mrib);
    }

  if (old_nexthop)
    nsm_nexthop_free (old_nexthop);

  return ret;
}

/* Delete static multicast route from RIB. */
int
nsm_static_mroute_rib_delete (struct nsm_vrf *nv, struct prefix *p,
                    struct nsm_mroute_config *mc)
{
  struct nsm_mrib *mrib = NULL;
  int ret = NSM_SUCCESS;


  mrib = nsm_mrib_lookup_by_type (nv, mc->afi, p, APN_ROUTE_STATIC,
      mc->uc_rtype);
  if (mrib == NULL)
    return NSM_API_ERR_MROUTE_NO_MRIB;

  /* Delete the MRIB from the route node list */
  nsm_mrib_delnode (mrib->rn, &mrib->rib);

  /* Process the delete of MRIB  */
  /* Re-process the multicast nexthop registrations */
  nsm_mnh_reg_reprocess (nv, mc->afi, p, p); 

  nsm_ptree_unlock_node (mrib->rn);
  nsm_mrib_free (mrib);

  return ret;
}

int
nsm_mroute_update_config (struct nsm_mroute_config *mc,
    u_char type, union nsm_nexthop *gate, char *ifname, 
    u_char distance, u_char change_flags)
{
  if (CHECK_FLAG (change_flags, NSM_MROUTE_CONFIG_NEXTHOP_CHANGE))
    {
      /* Update nexthop ifname if present */
      if (ifname)
        {
          if (mc->ifname)
            XFREE (MTYPE_NSM_IFNAME, mc->ifname);
          mc->ifname = XSTRDUP (MTYPE_NSM_IFNAME, ifname);

          if (mc->ifname == NULL)
            return NSM_API_SET_ERR_MROUTE_NO_MEM;
        }
      else if (mc->ifname)
        {
          XFREE (MTYPE_NSM_IFNAME, mc->ifname);
          mc->ifname = NULL;
        }

      /* Update nexthop gateway if present */
      if (gate)
        mc->gate = *gate;
      else 
        pal_mem_set (&mc->gate, 0, sizeof (union nsm_nexthop));

      mc->type = type;
    }

  if (CHECK_FLAG (change_flags, NSM_MROUTE_CONFIG_DISTANCE_CHANGE))
    mc->distance = distance;

  return NSM_API_SET_SUCCESS;
}

void
nsm_mroute_config_free (struct nsm_mroute_config *mc)
{
  if (mc->ifname)
    XFREE (MTYPE_NSM_IFNAME, mc->ifname);

  XFREE (MTYPE_NSM_MROUTE_CONFIG, mc);
  return;
}

struct nsm_mroute_config *
nsm_mroute_config_new (afi_t afi, u_char type,
    u_char uc_rtype, union nsm_nexthop *gate, char *ifname, 
    u_char distance)
{
  struct nsm_mroute_config *mc;

  mc = XCALLOC (MTYPE_NSM_MROUTE_CONFIG, sizeof (struct nsm_mroute_config));
  if (mc == NULL)
    return NULL;

  if (ifname)
    {
      mc->ifname = XSTRDUP (MTYPE_NSM_IFNAME, ifname);
      if (mc->ifname == NULL)
        {
          nsm_mroute_config_free (mc);
          return NULL;
        }
    }

  mc->afi = afi;
  mc->type = type;
  mc->uc_rtype = uc_rtype;
  if (gate)
    mc->gate = *gate;
  mc->distance = distance;

  return mc;
}

int
nsm_mroute_config_nexthop_same (struct nsm_mroute_config *mc, 
    u_char type, union nsm_nexthop *gate, char *ifname)
{
  if (type != mc->type)
    return 0;

  if (gate != NULL)
    {
      if (mc->afi == AFI_IP)
        {
          if (!IPV4_ADDR_SAME (&gate->ipv4, &mc->gate.ipv4))
            return 0;
        }
#ifdef HAVE_IPV6
      else if (mc->afi == AFI_IP6)
        {
          if (!IPV6_ADDR_SAME (&gate->ipv6, &mc->gate.ipv6))
            return 0;
        }
#endif /* HAVE_IPV6 */
    }

  if (ifname != NULL)
    if (pal_strcmp (ifname, mc->ifname) != 0)
      return 0;

  return 1;
}

/* Add multicast static route to config and RIB. */
int
nsm_mroute_config_set (struct nsm_vrf *nv, afi_t afi, 
    struct prefix *p, union nsm_nexthop *gate, char *ifname, 
    u_char uc_rtype, u_char distance)
{
  struct ptree_node *rn = NULL;
  struct nsm_mroute_config *mc, *pp;
  u_char type;
  u_char change_flags;
  int ret;

  /* For IPv6 link-local gw address should be accompanied by
   * interface.
   */
#ifdef HAVE_IPV6
  if (afi == AFI_IP6)
    {
      if (gate != NULL && IN6_IS_ADDR_LINKLOCAL (&gate->ipv6) && 
          ifname == NULL)
        return NSM_API_SET_ERR_INVALID_IPV6_NEXTHOP_LINKLOCAL;
    }
#endif /* HAVE_IPV6 */

  type = nsm_static_nexthop_type_get (afi, gate, ifname);
  if (type == 0)
    return 0;

  if (afi == AFI_IP)
    rn = ptree_node_get (nv->afi[afi].ip_mroute, 
                         (u_char *)&p->u.prefix4,
                         p->prefixlen);

  if (!rn)
    return NSM_API_SET_ERROR;

  /* Check for duplicate configuration.
   * Unicast rtype same && nexthop same && distance same 
   */
  change_flags = 0;
  for (pp = NULL, mc = rn->info; mc; pp = mc, mc = mc->next)
    {
      if (mc->uc_rtype != uc_rtype)
        continue;

      /* Check whether nexthop or distance has changed */
      if (!nsm_mroute_config_nexthop_same (mc, type, gate, ifname))
        SET_FLAG (change_flags, NSM_MROUTE_CONFIG_NEXTHOP_CHANGE);

      if (mc->distance != distance)
        SET_FLAG (change_flags, NSM_MROUTE_CONFIG_DISTANCE_CHANGE);

      /* If nothing has changed, return success */
      if (change_flags == 0)
        {
          ptree_unlock_node (rn);
          return NSM_API_SET_SUCCESS;
        }

      break;
    }

  /* If configuration exists, update it and the RIB */
  if (mc)
    {
      ptree_unlock_node (rn);

      ret = nsm_mroute_update_config (mc, type, gate, ifname, distance, change_flags);
      if (ret < 0)
        return ret;

      /* Update the RIB */
      ret = nsm_static_mroute_rib_update (nv, p, mc, change_flags); 
    }
  /* Else add a new configuration and update RIB */
  else
    {
      mc = nsm_mroute_config_new (afi, type, uc_rtype, gate, ifname, distance);
      if (mc == NULL)
        {
          ptree_unlock_node (rn);
          return NSM_API_SET_ERR_MROUTE_NO_MEM; 
        }

      /* Add it to the list */
      if (pp)
        {
          mc->next = pp->next;
          pp->next = mc;
        }
      else
        rn->info = mc;

      /* Add to the RIB */
      ret = nsm_static_mroute_rib_add (nv, p, mc); 
    }

  return ret;
}

/* Delete multicast static route from config and RIB. */
int
nsm_mroute_config_unset (struct nsm_vrf *nv, afi_t afi, 
    struct prefix *p, u_char uc_rtype)
{
  struct ptree_node *rn = NULL;
  struct nsm_mroute_config *mc, *pp;
  int ret;

  if (afi == AFI_IP)
    rn = ptree_node_lookup (nv->afi[afi].ip_mroute, 
                            (u_char *) &p->u.prefix4, p->prefixlen);
#ifdef HAVE_IPV6
  else if (afi == AFI_IP6)
    rn = ptree_node_lookup (nv->afi[afi].ip_mroute, 
                            (u_char *) &p->u.prefix6, p->prefixlen);
#endif

  if (rn == NULL)
    return NSM_API_SET_ERR_MROUTE_NO_CONFIG; 

  /* Unlock for lookup */
  ptree_unlock_node (rn);

  /* Check if the configuration exists */
  for (pp = NULL, mc = rn->info; mc; pp = mc, mc = mc->next)
    {
      if (mc->uc_rtype == uc_rtype)
        break;
    }

  /* No configuration found */
  if (mc == NULL)
    return NSM_API_SET_ERR_MROUTE_NO_CONFIG; 

  /* Delete from RIB */
  ret = nsm_static_mroute_rib_delete (nv, p, mc); 

  /* Remove from the list */
  if (pp)
    pp->next = mc->next;
  else
    rn->info = mc->next;

  nsm_mroute_config_free (mc);

  /* Unlock for delete */
  ptree_unlock_node (rn);

  return NSM_API_SET_SUCCESS;
}

static int
nsm_mroute_cli_return (struct cli *cli, int ret)
{
  char *str;

  switch (ret)
    {
      /* Static mroute errors */
    case NSM_API_SET_ERR_UNKNOWN_UCAST_PROTO:
      str = "Unknown unicast protocol type";
      break;

    case NSM_API_SET_ERR_MROUTE_GENERAL_ERR:
      str = "General error";
      break;

    case NSM_API_SET_ERR_GW_BCAST:
      str = "Nexthop IP address required for multi-access media";
      break;

    case NSM_API_SET_ERR_MROUTE_NO_MEM:
      str = "Out of memory";
      break;

    case NSM_API_SET_ERR_MROUTE_NO_CONFIG:
      str = "No such static multicast route configured";
      break;

    case NSM_API_ERR_MROUTE_NO_MRIB:
      str = "No such static multicast route present";
      break;

    default:
      return nsm_cli_return (cli, ret);
    }

  cli_out (cli, "%% %s\n", str);

  return CLI_ERROR;
}

/* When gateway is A.B.C.D format, gate is treated as nexthop
   address other case gate is treated as interface name. */
int
nsm_mroute_gateway_get (struct nsm_master *nm, afi_t afi, 
    char *str, char **ifname, union nsm_nexthop *gate, 
    vrf_id_t vrf_id)
{
  struct interface *ifp;
  int ret;

  ret = pal_inet_pton (afi2family (afi), str, gate);
  if (!ret)
    {
      if (IS_NULL_INTERFACE_STR (str))
        *ifname = NULL_INTERFACE;
      else
        {
          /* Check if it is valid ifname. */    /* XXX */
          ifp = if_lookup_by_name (&nm->vr->ifm, str);
          if (ifp == NULL || ifp->vrf->id != vrf_id)
            return NSM_MROUTE_GATEWAY_INVALID;

          if (if_is_broadcast (ifp))
            return NSM_MROUTE_GATEWAY_BCAST;

          *ifname = str;
        }
      return NSM_MROUTE_GATEWAY_IFNAME;
    }
  else
    {
      if (afi == AFI_IP)
        {
          /* Check IP address */
          if (IN_MULTICAST (pal_ntoh32 (gate->ipv4.s_addr)) || 
              IN_EXPERIMENTAL (pal_ntoh32 (gate->ipv4.s_addr)))
            return NSM_MROUTE_GATEWAY_INVALID;
        }
#ifdef HAVE_IPV6
      else if (afi == AFI_IP6)
        {
          if (IN6_IS_ADDR_MULTICAST (&gate->ipv6)
              || IN6_IS_ADDR_UNSPECIFIED (&gate->ipv6))
            return NSM_MROUTE_GATEWAY_INVALID;
        }
#endif /* HAVE_IPV6 */
    }

  return NSM_MROUTE_GATEWAY_ADDRESS;
}

/* CLI APIs */
int
nsm_ip_mroute_set (struct nsm_master *nm, afi_t afi, 
    vrf_id_t vrf_id, struct prefix *p, char *gate_str, 
    u_char ucast_rtype, u_char distance)
{
  union nsm_nexthop gate;
  char *ifname = NULL;
  struct nsm_vrf *nv;
  int ret = 0;

  if (afi == AFI_IP)
    {
      /* Check IP address */
      if (IN_MULTICAST (pal_ntoh32 (p->u.prefix4.s_addr)) || 
          IN_EXPERIMENTAL (pal_ntoh32 (p->u.prefix4.s_addr)))
        return NSM_API_SET_ERR_INVALID_IPV4_ADDRESS;
    }
#ifdef HAVE_IPV6
  else if (afi == AFI_IP6)
    {
      if (IN6_IS_ADDR_MULTICAST (&p->u.prefix6)
           || IN6_IS_ADDR_UNSPECIFIED (&p->u.prefix6))
        return NSM_API_SET_ERR_INVALID_IPV6_ADDRESS;
    }
#endif /* HAVE_IPV6 */
  else
    {
      pal_assert(0);
      return NSM_API_SET_ERR_MROUTE_GENERAL_ERR;
    }

  nv = nsm_vrf_lookup_by_id (nm, vrf_id);
  if (nv == NULL)
    return NSM_API_SET_ERR_VRF_NOT_EXIST;

  apply_mask (p);

  ret = nsm_mroute_gateway_get (nm, afi, gate_str, &ifname, &gate, vrf_id);
  if (ret == NSM_MROUTE_GATEWAY_INVALID)
    return NSM_API_SET_ERR_MALFORMED_GATEWAY;
  else if (ret == NSM_MROUTE_GATEWAY_BCAST)
    return NSM_API_SET_ERR_GW_BCAST;

  if (ret == NSM_MROUTE_GATEWAY_ADDRESS)
    ret = nsm_mroute_config_set (nv, afi, p, &gate, ifname,
                              ucast_rtype, distance);
  else
    ret = nsm_mroute_config_set (nv, afi, p, NULL, ifname,
                              ucast_rtype, distance);

  return ret;
}

int
nsm_ip_mroute_ifname_set (struct nsm_master *nm, afi_t afi, 
    vrf_id_t vrf_id, struct prefix *p, struct prefix *gw, 
    char *ifname, u_char ucast_rtype, u_char distance)
{
  struct interface *ifp;
  union nsm_nexthop gate;
  struct nsm_vrf *nv;
  int ret = 0;

  if (afi == AFI_IP)
    {
      /* Check IP address */
      if (IN_MULTICAST (pal_ntoh32 (p->u.prefix4.s_addr)) || 
          IN_EXPERIMENTAL (pal_ntoh32 (p->u.prefix4.s_addr)))
        return NSM_API_SET_ERR_INVALID_IPV4_ADDRESS;

      /* Check IP address */
      if (IN_MULTICAST (pal_ntoh32 (gw->u.prefix4.s_addr)) || 
          IN_EXPERIMENTAL (pal_ntoh32 (gw->u.prefix4.s_addr)))
        return NSM_MROUTE_GATEWAY_INVALID;

      IPV4_ADDR_COPY (&gate.ipv4, &gw->u.prefix4);
    }
#ifdef HAVE_IPV6
  else if (afi == AFI_IP6)
    {
      if (IN6_IS_ADDR_MULTICAST (&p->u.prefix6)
           || IN6_IS_ADDR_UNSPECIFIED (&p->u.prefix6))
        return NSM_API_SET_ERR_INVALID_IPV6_ADDRESS;

      if (IN6_IS_ADDR_MULTICAST (&gw->u.prefix6)
           || IN6_IS_ADDR_UNSPECIFIED (&gw->u.prefix6))
        return NSM_MROUTE_GATEWAY_INVALID;

      IPV6_ADDR_COPY (&gate.ipv6, &gw->u.prefix6);
    }
#endif /* HAVE_IPV6 */
  else
    {
      pal_assert(0);
      return NSM_API_SET_ERR_MROUTE_GENERAL_ERR;
    }

  ifp = if_lookup_by_name (&nm->vr->ifm, ifname);
  if (ifp == NULL || ifp->vrf->id != vrf_id)
    return NSM_API_SET_ERR_NO_SUCH_INTERFACE;

  nv = nsm_vrf_lookup_by_id (nm, vrf_id);
  if (nv == NULL)
    return NSM_API_SET_ERR_VRF_NOT_EXIST;

  apply_mask (p);

  ret = nsm_mroute_config_set (nv, afi, p, &gate, ifname,
                            ucast_rtype, distance);

  return ret;
}

int
nsm_ip_mroute_unset (struct nsm_master *nm, afi_t afi, 
    vrf_id_t vrf_id, struct prefix *p, u_char ucast_rtype)
{
  struct nsm_vrf *nv;
  int ret = 0;

  if (afi == AFI_IP)
    {
      /* Check IP address */
      if (IN_MULTICAST (pal_ntoh32 (p->u.prefix4.s_addr)) || 
          IN_EXPERIMENTAL (pal_ntoh32 (p->u.prefix4.s_addr)))
        return NSM_API_SET_ERR_INVALID_IPV4_ADDRESS;
    }
#ifdef HAVE_IPV6
  else if (afi == AFI_IP6)
    {
      if (IN6_IS_ADDR_MULTICAST (&p->u.prefix6)
           || IN6_IS_ADDR_UNSPECIFIED (&p->u.prefix6))
        return NSM_API_SET_ERR_INVALID_IPV6_ADDRESS;
    }
#endif /* HAVE_IPV6 */
  else
    {
      pal_assert(0);
      return NSM_API_SET_ERR_MROUTE_GENERAL_ERR;
    }

  nv = nsm_vrf_lookup_by_id (nm, vrf_id);
  if (nv == NULL)
    return NSM_API_SET_ERR_VRF_NOT_EXIST;

  apply_mask (p);

  ret = nsm_mroute_config_unset (nv, afi, p, ucast_rtype);

  return ret;
}

/* Cleanup mroute table configuration */
void
nsm_ip_mroute_table_clean_all (struct ptree *table)
{
  struct ptree_node *rn;
  struct nsm_mroute_config *mc, *next;

  for (rn = ptree_top (table); rn; rn = ptree_next (rn))
    {
      if (rn->info == NULL)
        continue;

      for (mc = rn->info; mc; mc = next)
        {
          next = mc->next;

          nsm_mroute_config_free (mc);

          ptree_unlock_node (rn);
        }
      rn->info = NULL;
    }

  ptree_finish (table);
  return;
}

int
nsm_mrib_table_free (struct nsm_ptree_table *rt)
{
  struct nsm_ptree_node *tmp_node;
  struct nsm_ptree_node *node;
  struct rib *rib;
  struct rib *tmp_rib;
  struct nsm_mrib *mrib;

  if (rt == NULL)
    return 0;

  node = rt->top;

  while (node)
    {
      if (node->p_left)
        {
          node = node->p_left;
          continue;
        }

      if (node->p_right)
        {
          node = node->p_right;
          continue;
        }

      tmp_node = node;
      node = node->parent;

      if (node != NULL)
        {
          if (node->p_left == tmp_node)
            node->p_left = NULL;
          else
            node->p_right = NULL;

          rib = tmp_node->info;
          while (rib)
            {
              tmp_rib = rib;
              rib = tmp_rib->next;

              mrib = RIB2MRIB (tmp_rib);
              nsm_mrib_free (mrib);
            }

          nsm_ptree_node_free (tmp_node);
        }
      else
        {
          rib = tmp_node->info;
          while (rib)
            {
              tmp_rib = rib;
              rib = tmp_rib->next;

              mrib = RIB2MRIB (tmp_rib);
              nsm_mrib_free (mrib);
            }

          nsm_ptree_node_free (tmp_node);
          break;
        }
    }

  XFREE (MTYPE_NSM_PTREE_TABLE, rt);
  return 0;
}

/* Handle change in unicast rib */
void
nsm_mrib_handle_unicast_rib_change (struct nsm_vrf *nv, 
    struct prefix *unicast_p)
{
  struct nsm_ptree_node *rn;
  struct nsm_ptree_table *table;
  struct rib *rib;
  struct prefix nh_p, change_p;
  int rn_p_used = 0;
  struct prefix p;

  afi_t afi = family2afi (unicast_p->family);

  if (afi == AF_UNSPEC)
    return;

  /* Prepare the nexthop prefix */
  nh_p.family = unicast_p->family;
  nh_p.prefixlen = PREFIX_MAX_BITLEN (nh_p.family);

  /* Copy the unicast prefix into the change prefix */
  prefix_copy (&change_p, unicast_p);

  table = nv->afi[afi].rib[SAFI_MULTICAST];

  /* Check whether any nexthops of multicast routes are affected */
  for (rn = nsm_ptree_top (table); rn; rn = nsm_ptree_next (rn))
    { 
      pal_mem_set(&p, 0, sizeof (struct prefix));
      p.prefixlen = rn->key_len;
      p.family = rn->tree->ptype;
      if (p.family == AF_INET)
        NSM_PTREE_KEY_COPY (&p.u.prefix4, rn);
#ifdef HAVE_IPV6
      else if (p.family == AF_INET6)
        NSM_PTREE_KEY_COPY (&p.u.prefix6, rn);
#endif

      rn_p_used = 0;
      for (rib = rn->info; rib; rib = rib->next)
        {
          /* Process only nexthops with gateway addresses. The
           * nexthops with interface will be processed in interface
           * event handler.
           */
          switch (rib->nexthop->type)
            {
              case NEXTHOP_TYPE_IPV4:
              case NEXTHOP_TYPE_IPV4_IFINDEX:
              case NEXTHOP_TYPE_IPV4_IFNAME:
#ifdef HAVE_IPV6
              case NEXTHOP_TYPE_IPV6:
              case NEXTHOP_TYPE_IPV6_IFINDEX:
              case NEXTHOP_TYPE_IPV6_IFNAME:
#endif /* HAVE_IPV6 */
                
                /* Copy nexthop address */
                pal_mem_cpy (&nh_p.u.prefix, &rib->nexthop->gate, 
                    PREFIX_MAX_BYTELEN (nh_p.family));

                /* See if the nexthop address falls under changed unicast
                 * prefix
                 */
                if (!prefix_match (unicast_p, &nh_p))
                  break;

                /* Check if nexthop state (active) is changed */
                nsm_nexthop_active_update (rn, rib, 1);
                
                /* Update the change prefix, if the current route node
                 * prefix has not already been considered in the change 
                 */
                if (rn_p_used)
                  break;

                /* Note that the order of arguments is important !!!
                 * If change_p cannot is passed as 2nd argument, 
                 * prefix_common would not work
                 */
                prefix_common (&change_p, &p, &change_p);

                rn_p_used = 1;

                break;

              default:
                break;
            }
        }
    }

  /* Now process multicast nexthop registration */
  nsm_mnh_reg_reprocess (nv, afi, &change_p, unicast_p);

  return;
}

/* Handle change in interface state for interface nexthops */
void
nsm_mrib_handle_if_state_change (struct nsm_vrf *nv, afi_t afi,
    struct interface *ifp)
{
  struct nsm_ptree_node *rn;
  struct nsm_ptree_table *table;
  struct rib *rib;
  struct prefix change_p;
  int rn_p_used = 0;
  u_int8_t family = (u_int8_t)afi2family (afi);
  struct prefix p;

  if (family == 0)
    return;

  /* Prepare the change prefix */
  pal_mem_set (&change_p, 0, sizeof (struct prefix));
  change_p.family = AF_UNSPEC;

  /* Use the fact that initially the family of change prefix is 
   * AF_UNSPEC (i.e. 0) to keep track whether the prefix has changed
   * or not.
   */

  table = nv->afi[afi].rib[SAFI_MULTICAST];

  /* Check whether any nexthops of multicast routes are affected */
  for (rn = nsm_ptree_top (table); rn; rn = nsm_ptree_next (rn))
    {
      pal_mem_set(&p, 0, sizeof (struct prefix));
      p.prefixlen = rn->key_len;
      p.family = rn->tree->ptype;
      if (p.family == AF_INET)
        NSM_PTREE_KEY_COPY (&p.u.prefix4, rn);
#ifdef HAVE_IPV6
      else if (p.family == AF_INET6)
        NSM_PTREE_KEY_COPY (&p.u.prefix6, rn);
#endif
      rn_p_used = 0;
      for (rib = rn->info; rib; rib = rib->next)
        {
          /* Process only nexthops with gateway addresses. The
           * nexthops with interface will be processed in interface
           * event handler.
           */
          switch (rib->nexthop->type)
            {
              case NEXTHOP_TYPE_IFINDEX:
              case NEXTHOP_TYPE_IPV4_IFINDEX:
#ifdef HAVE_IPV6
              case NEXTHOP_TYPE_IPV6_IFINDEX:
#endif /* HAVE_IPV6 */
                if (rib->nexthop->ifindex != ifp->ifindex)
                  break;

                /* Check if nexthop state (active) is changed */
                nsm_nexthop_active_update (rn, rib, 1);
                
                /* Update the change prefix, if the current route node
                 * prefix has not already been considered in the change 
                 */
                if (rn_p_used)
                  break;

                /* Note that the order of arguments is important !!!
                 * If change_p cannot is passed as 2nd argument, 
                 * prefix_common would not work
                 */ 

                if (change_p.family == AF_UNSPEC)
                  prefix_copy (&change_p, &p);
                else
                  prefix_common (&change_p, &p, &change_p);

                rn_p_used = 1;

                break;

              case NEXTHOP_TYPE_IFNAME:
              case NEXTHOP_TYPE_IPV4_IFNAME:
#ifdef HAVE_IPV6
              case NEXTHOP_TYPE_IPV6_IFNAME:
#endif /* HAVE_IPV6 */
                if (pal_strcmp (rib->nexthop->ifname, ifp->name))
                  break;

                /* Check if nexthop state (active) is changed */
                nsm_nexthop_active_update (rn, rib, 1);
                
                /* Update the change prefix, if the current route node
                 * prefix has not already been considered in the change 
                 */
                if (rn_p_used)
                  break;

                /* Note that the order of arguments is important !!!
                 * If change_p cannot is passed as 2nd argument, 
                 * prefix_common would not work
                 */
                if (change_p.family == AF_UNSPEC)
                  prefix_copy (&change_p, &p);
                else
                  prefix_common (&change_p, &p, &change_p);

                rn_p_used = 1;

                break;

              default:
                break;
            }
        }
    }

  /* If nothing changed, just return */
  if (change_p.family == AF_UNSPEC)
    return;

  /* Now process multicast nexthop registration */
  nsm_mnh_reg_reprocess (nv, afi, &change_p, &change_p);

  return;
}

void
nsm_static_mroute_delete_by_if (struct nsm_vrf *vrf, afi_t afi,
                         struct interface *ifp)
{
  struct ptree_node *rn;
  struct nsm_mroute_config *mc, *next;
  struct prefix p;

  for (rn = ptree_top (vrf->afi[afi].ip_mroute);
       rn; rn = ptree_next (rn))
    for (mc = rn->info; mc; mc = next)
      {
        next = mc->next;
        if (mc->type == NEXTHOP_TYPE_IFNAME
            || mc->type == NEXTHOP_TYPE_IPV4_IFNAME
#ifdef HAVE_IPV6
            || mc->type == NEXTHOP_TYPE_IPV6_IFNAME
#endif /* HAVE_IPV6 */
            )
          if (pal_strcmp (mc->ifname, ifp->name) == 0)
            {
              pal_mem_set(&p, 0, sizeof (struct prefix));
              p.prefixlen = rn->key_len;
              if (afi == AFI_IP)
                {
                  p.family = AF_INET;
                  NSM_PTREE_KEY_COPY (&p.u.prefix4, rn);
                }
#ifdef HAVE_IPV6
              else if (afi == AFI_IP6)
                {
                  p.family = AF_INET6;
                  NSM_PTREE_KEY_COPY (&p.u.prefix6, rn);
                }
#endif
              nsm_mroute_config_unset (vrf, afi, &p, mc->uc_rtype);
            }
      }
  return ;
}

/* Multicast nexthop registration functions */
struct nsm_mnh_reg_client *
nsm_mnh_reg_client_new (struct nsm_server_client *nsc)
{
  struct nsm_mnh_reg_client *cl;

  cl = XCALLOC (MTYPE_NSM_MNH_REG_CL, sizeof (struct nsm_mnh_reg_client));
  if (cl == NULL)
    return NULL;

  cl->nsc = nsc;

  return cl;
}

void
nsm_mnh_reg_client_free (struct nsm_mnh_reg_client *cl)
{
  XFREE (MTYPE_NSM_MNH_REG_CL, cl);
  return;
}

struct nsm_mnh_reg_client *
nsm_mnh_reg_client_remove (struct nsm_mnh_reg *reg, 
    struct nsm_server_client *nsc)
{
  struct nsm_mnh_reg_client **pp, *ret = NULL;

  for (pp = &reg->clients; (*pp) && ((*pp)->nsc != nsc); 
      pp = &((*pp)->next)) 
    ;

  if (*pp != NULL)
    {
      ret = *pp;
      *pp = (*pp)->next;
    }

  return ret;
}

struct nsm_mnh_reg *
nsm_mnh_reg_new (u_char lookup_type)
{
  struct nsm_mnh_reg *reg;

  reg = XCALLOC (MTYPE_NSM_MNH_REG, sizeof (struct nsm_mnh_reg));
  if (reg == NULL)
    return NULL;

  reg->lookup_type = lookup_type;

  return reg;
}

void
nsm_mnh_reg_free (struct nsm_mnh_reg *reg)
{
  struct nsm_mnh_reg_client *cl, *next;

  for (cl = reg->clients; cl; cl = next)
    {
      next = cl->next;
      nsm_mnh_reg_client_free (cl);
    }

  XFREE (MTYPE_NSM_MNH_REG, reg);

  return;
}

void
nsm_mnh_reg_remove (struct nsm_ptree_node *rn, struct nsm_mnh_reg *reg)
{
  struct nsm_mnh_reg **pp;

  for (pp = (struct nsm_mnh_reg **)&(rn->info); (*pp) && (*pp != reg); 
      pp = &((*pp)->next)) 
    ;

  if (*pp == NULL)
    return;
  else
    *pp = reg->next;

  reg->next = NULL;
  return;
}

void
nsm_mnh_reg_client_send_add (struct prefix *p,
    struct nsm_ptree_node *rn, struct rib *rib, 
    struct nsm_server_client *nsc)
{
  struct nsm_server_entry *nse;

  for (nse = nsc->head; nse; nse = nse->next)
    if (NSM_CHECK_CTYPE (nse->service.bits, NSM_SERVICE_ROUTE)
        || NSM_CHECK_CTYPE (nse->service.bits, NSM_SERVICE_ROUTE_LOOKUP))
      {
        if (p->family == AF_INET)
          nsm_send_ipv4_add (nse, 0, p, rn->key_len, rib, rib->vrf, 0, NULL);
#ifdef HAVE_IPV6
        else if (NSM_CAP_HAVE_IPV6 && p->family == AF_INET6)
          nsm_send_ipv6_add (nse, 0, p, rn->key_len, rib, rib->vrf, 0, NULL);
#endif /* HAVE_IPV6 */
      }

  return;
}

void
nsm_mnh_reg_client_send_delete (struct prefix *p,
    struct nsm_ptree_node *rn, struct rib *rib, 
    struct nsm_server_client *nsc)
{
  struct nsm_server_entry *nse;

  for (nse = nsc->head; nse; nse = nse->next)
    if (NSM_CHECK_CTYPE (nse->service.bits, NSM_SERVICE_ROUTE)
        || NSM_CHECK_CTYPE (nse->service.bits, NSM_SERVICE_ROUTE_LOOKUP))
      {
        if (p->family == AF_INET)
          nsm_send_ipv4_delete (nse, p, rn->key_len, rib, rib->vrf, 0);
#ifdef HAVE_IPV6
        else if (NSM_CAP_HAVE_IPV6 && p->family == AF_INET6)
          nsm_send_ipv6_delete (nse, p, rn->key_len, rib, rib->vrf, 0);
#endif /* HAVE_IPV6 */
      }

  return;
}
/* Core function to update the nexthop registration. 
 * If the rib changes, then an update is sent to the client.
 */
void
nsm_mnh_reg_update_rib (struct nsm_vrf *nv, afi_t afi,
    struct nsm_ptree_node *rn, struct nsm_mnh_reg *reg)
{
  struct rib *old;
  struct nsm_ptree_node *rn_rib = NULL;
  struct nsm_mnh_reg_client *cl;
  struct prefix p;

  
  pal_mem_set(&p, 0, sizeof (struct prefix));
  p.prefixlen = rn->key_len;
  p.family = rn->tree->ptype;
  if (rn->tree->ptype == AF_INET)
    NSM_PTREE_KEY_COPY (&p.u.prefix4, rn);
#ifdef HAVE_IPV6
  else if (rn->tree->ptype == AF_INET6)
    NSM_PTREE_KEY_COPY (&p.u.prefix6, rn);
#endif
  /* Store current rib */
  old = reg->rib;

  if (reg->lookup_type == EXACT_MATCH_LOOKUP)
    reg->rib = nsm_mrib_lookup (nv, afi, &p, &rn_rib, 0);
  else
    reg->rib = nsm_mrib_match (nv, afi, &p, &rn_rib, 0);

  /* If the old rib is NULL and if there is no change, nothing to do */
  if ((old == NULL) && (old == reg->rib)) 
    return;

  if ((old != NULL) && (reg->rib == NULL))
    {
      for (cl = reg->clients; cl; cl = cl->next)
        nsm_mnh_reg_client_send_delete (&p, rn, old, cl->nsc);
    }
  else
    {
      for (cl = reg->clients; cl; cl = cl->next)
        nsm_mnh_reg_client_send_add (&p, rn_rib, reg->rib, 
            cl->nsc);
    }

  return;
}

/* Function to re-evaluate the multicast nexthop registrations */
void
nsm_mnh_reg_reprocess (struct nsm_vrf *nv, afi_t afi, struct prefix *p,
    struct prefix *exact_lookup_p)
{
  struct nsm_ptree_node *rn =  NULL;
  struct nsm_ptree_node *rn_temp = NULL;
  struct nsm_mnh_reg *reg;
  
  if (afi == AFI_IP)
    rn_temp = nsm_ptree_node_get (nv->afi[afi].nh_reg_m, 
                                  (u_char *)&p->u.prefix4,
                                  p->prefixlen); 
#ifdef HAVE_IPV6
  else  if (afi == AFI_IP6)
    rn_temp = nsm_ptree_node_get (nv->afi[afi].nh_reg_m, 
                                  (u_char *)&p->u.prefix6,
                                  p->prefixlen);
#endif
 
  if (rn_temp == NULL)
    return;

  nsm_ptree_lock_node (rn_temp);

  /* Do a limited traversal of nexthop registrations under prefix p
   * and evaluate the registration for change
   */
  for (rn = rn_temp; rn; rn = nsm_ptree_next_until (rn, rn_temp))
    {
      for (reg = rn->info; reg; reg = reg->next)
        {
          /* Skip exact lookup registrations if they do not match
           * the exact lookup prefix.
           */
          if ((reg->lookup_type == EXACT_MATCH_LOOKUP) &&
              (exact_lookup_p != NULL) && 
              (rn->key_len != exact_lookup_p->prefixlen))
            continue;

          nsm_mnh_reg_update_rib (nv, afi, rn, reg);
        }
    }

  nsm_ptree_unlock_node (rn_temp);

  return;
}

void 
nsm_mnh_reg_client_add (struct nsm_vrf *nv, afi_t afi,
    struct nsm_ptree_node *rn, struct nsm_mnh_reg *reg, 
    struct nsm_server_client *nsc)
{
  struct nsm_mnh_reg_client *cl;
  int first_cl = 0;
  struct prefix addr;

  if (reg->clients == NULL)
    first_cl = 1;

  /* Assumption is that clients do not register duplicate prefixes.
   * So we allocate a new memory for every add.
   */
  cl = nsm_mnh_reg_client_new (nsc);
  if (cl == NULL)
    {
      /* If it is the first client, remove the registration */
      if (first_cl)
        {
          nsm_mnh_reg_remove (rn, reg);
          nsm_mnh_reg_free (reg);
          nsm_ptree_unlock_node (rn);
        }
      return;
    }

  /* Insert the client into the list */
  cl->next = reg->clients;
  reg->clients = cl;

  /* If this is the first client, update the registration
   * Else, update the new client if the rib is non null 
   */
  if (first_cl)
    nsm_mnh_reg_update_rib (nv, afi, rn, reg);
  else if (reg->rib != NULL)
    {
      if (rn != NULL)
        {
          pal_mem_set(&addr, 0, sizeof (struct prefix));
          addr.prefixlen = rn->key_len;
          addr.family = rn->tree->ptype;
          if (rn->tree->ptype == AF_INET)
            NSM_PTREE_KEY_COPY (&addr.u.prefix4, rn);
#ifdef HAVE_IPV6
          else if (rn->tree->ptype == AF_INET6)
            NSM_PTREE_KEY_COPY (&addr.u.prefix6, rn);
#endif
          nsm_mnh_reg_client_send_add (&addr, rn, reg->rib, nsc);
        }
    }

  return;
}

void 
nsm_mnh_reg_client_delete (struct nsm_ptree_node *rn, 
    struct nsm_mnh_reg *reg, struct nsm_server_client *nsc)
{
  struct nsm_mnh_reg_client *cl;

  cl = nsm_mnh_reg_client_remove (reg, nsc);
  if (cl == NULL)
    return;

  nsm_mnh_reg_client_free (cl);

  /* If this was the last client, remove registration */
  if (reg->clients == NULL)
    {
      nsm_mnh_reg_remove (rn, reg);
      nsm_mnh_reg_free (reg);
      nsm_ptree_unlock_node (rn);
    }

  return;
}

void
nsm_multicast_nh_reg (struct nsm_vrf *nv, afi_t afi,
    struct nsm_server_client *nsc, struct prefix *p, u_char lookup_type)
{
  struct nsm_ptree_node *rn = NULL;
  struct nsm_mnh_reg *reg;
 
  if (afi == AFI_IP)
    rn = nsm_ptree_node_get (nv->afi[afi].nh_reg_m, 
                             (u_char *)&p->u.prefix4,
                              p->prefixlen); 
#ifdef HAVE_IPV6
  else  if (afi == AFI_IP6)
    rn = nsm_ptree_node_get (nv->afi[afi].nh_reg_m, 
                             (u_char *)&p->u.prefix6,
                              p->prefixlen);
#endif
 
  if (rn == NULL)
    return;

  /* Find the registration by type */
  for (reg = rn->info; reg; reg = reg->next)
    if (reg->lookup_type == lookup_type)
      break;

  /* If not found create a new registration */
  if (reg == NULL)
    {
      reg = nsm_mnh_reg_new (lookup_type);
      if (reg == NULL)
        {
          nsm_ptree_unlock_node (rn);
          return;
        }
      reg->next = rn->info;
      rn->info = reg;
    }
  else
    nsm_ptree_unlock_node (rn);

  nsm_mnh_reg_client_add (nv, afi, rn, reg, nsc);

  return;
}

void
nsm_multicast_nh_dereg (struct nsm_vrf *nv, afi_t afi,
    struct nsm_server_client *nsc, struct prefix *p, u_char lookup_type)
{
  struct nsm_ptree_node *rn = NULL;
  struct nsm_mnh_reg *reg;
 
  if (afi == AFI_IP)
    rn = nsm_ptree_node_lookup (nv->afi[afi].nh_reg_m,
                                (u_char *)&p->u.prefix4,
                                p->prefixlen); 
#ifdef HAVE_IPV6
  else  if (afi == AFI_IP6)
    rn = nsm_ptree_node_lookup (nv->afi[afi].nh_reg_m,
                                (u_char *)&p->u.prefix6,
                                p->prefixlen);
#endif

  if (rn == NULL || rn->info == NULL)
    return;

  /* Unlock for lookup */
  nsm_ptree_unlock_node (rn);

  /* Find the registration by type */
  for (reg = rn->info; reg; reg = reg->next)
    if (reg->lookup_type == lookup_type)
      break;

  /* If not found create a new registration */
  if (reg == NULL)
    return;

  nsm_mnh_reg_client_delete (rn, reg, nsc);

  return;
}

void 
nsm_multicast_nh_reg_table_clean_all (struct nsm_ptree_table *table)
{
  struct nsm_ptree_node *rn;
  struct nsm_mnh_reg *reg, *next;

  for (rn = nsm_ptree_top (table); rn; rn = nsm_ptree_next (rn))
    {
      for (reg = rn->info; reg; reg = next)
        {
          next = reg->next;

          nsm_mnh_reg_free (reg);

          nsm_ptree_unlock_node (rn);
        }
      rn->info = NULL;
    }

  nsm_ptree_table_finish (table);
  return;
}

void 
nsm_multicast_nh_reg_table_clean_client (struct nsm_ptree_table *table,
    struct nsm_server_client *nsc)
{
  struct nsm_ptree_node *rn;
  struct nsm_mnh_reg *reg, *next;

  for (rn = nsm_ptree_top (table); rn; rn = nsm_ptree_next (rn))
    {
      for (reg = rn->info; reg; reg = next)
        {
          next = reg->next;

          nsm_mnh_reg_client_delete (rn, reg, nsc); 
        }
    }

  return;
}

void
nsm_multicast_nh_reg_client_disconnect (struct nsm_master *nm, 
    struct nsm_server_client *nsc)
{
  struct apn_vrf *iv;
  struct nsm_vrf *nv;
  afi_t afi;
  int i;

  for (i = 0; i < vector_max (nm->vr->vrf_vec); i++)
    if ((iv = vector_slot (nm->vr->vrf_vec, i)) != NULL)
      {
        nv = iv->proto;
        NSM_AFI_LOOP (afi)
          nsm_multicast_nh_reg_table_clean_client (nv->afi[afi].nh_reg_m, nsc);
      }
}

static int
nsm_mroute_cli_ipv4_proto2route_type (char *str, u_char *route_type)
{
  /* Sanity check. */
  if (str == NULL)
    return 0;

  if (pal_strncmp (str, "s", 1) == 0)
    *route_type = APN_ROUTE_STATIC;
  else if (pal_strncmp (str, "r", 1) == 0)
    *route_type = APN_ROUTE_RIP;
  else if (pal_strncmp (str, "o", 1) == 0)
    *route_type = APN_ROUTE_OSPF;
  else if (pal_strncmp (str, "b", 1) == 0)
    *route_type = APN_ROUTE_BGP;
  else if (pal_strncmp (str, "i", 1) == 0)
    *route_type = APN_ROUTE_ISIS;
  else
    return 0;

  return 1;
}

static char *
nsm_mroute_cli_route2proto (u_char route_type)
{
  char *str;

  switch (route_type)
    {
    case APN_ROUTE_STATIC:
      str = "static";
      break;
    case APN_ROUTE_RIP:
      str = "rip";
      break;
    case APN_ROUTE_OSPF:
      str = "ospf";
      break;
    case APN_ROUTE_BGP:
      str = "bgp";
      break;
    case APN_ROUTE_ISIS:
      str = "isis";
      break;
#ifdef HAVE_IPV6
    case APN_ROUTE_RIPNG:
      str = "rip";
      break;
    case APN_ROUTE_OSPF6:
      str = "ospf";
      break;
#endif /* HAVE_IPV6 */
    default:
      str ="";
      break;
    }
  return str;
}

/* Static mroute CLIs */

#define MROUTE_CLI_IPV4_PROTOS_CMD                                            \
    "static|rip|ospf|bgp|isis|"

#define MROUTE_CLI_IPV4_PROTOS_STR                                            \
    "Static routes",                                                          \
    "Routing Information Protocol (RIP)",                                     \
    "Open Shortest Patch First (OSPF)",                                       \
    "Border Gateway Protocol (BGP)",                                          \
    "ISO IS-IS"

#ifdef HAVE_VRF
CLI (ip_mroute_prefix,
     ip_mroute_prefix_cmd,
     "ip mroute (vrf NAME|) A.B.C.D/M (" MROUTE_CLI_IPV4_PROTOS_CMD ") (A.B.C.D|INTERFACE)",
     CLI_IP_STR,
     CLI_MROUTE_STR,
     CLI_VRF_STR,
     CLI_VRF_NAME_STR,
     "Source prefix",
     MROUTE_CLI_IPV4_PROTOS_STR,
     "RPF neighbor address or route",
     "RPF interface or pseudo interface Null")
#else
CLI (ip_mroute_prefix,
     ip_mroute_prefix_cmd,
     "ip mroute A.B.C.D/M (" MROUTE_CLI_IPV4_PROTOS_CMD ") (A.B.C.D|INTERFACE)",
     CLI_IP_STR,
     CLI_MROUTE_STR,
     "Source prefix",
     MROUTE_CLI_IPV4_PROTOS_STR,
     "RPF neighbor address or route",
     "RPF interface or pseudo interface Null")
#endif /* HAVE_VRF */
{
  struct apn_vrf *ivrf;
  struct nsm_master *nm = cli->vr->proto;
  struct prefix_ipv4 p;
  int distance = APN_DISTANCE_MSTATIC;
  u_char route_type = APN_ROUTE_DEFAULT;
  char *gw = NULL;
  int ret;

  /* Get the VRF Instance  */
  if (argc > 1
      && ! pal_strcmp (argv [0], "vrf"))
    {
      ivrf = apn_vrf_lookup_by_name (LIB_GLOB_GET_VR_CONTEXT (cli->zg), argv [1]);

      ret = str2prefix_ipv4 (argv[2], &p);
      if (ret <= 0)
        return nsm_mroute_cli_return (cli, NSM_API_SET_ERR_MALFORMED_ADDRESS);

      gw = argv[3];

      if (argc > 4)
        {
          if (!nsm_mroute_cli_ipv4_proto2route_type (argv[3], &route_type))
            return nsm_mroute_cli_return (cli, NSM_API_SET_ERR_UNKNOWN_UCAST_PROTO);
          gw = argv[4];
        }
    }
  else
    {
      ivrf = apn_vrf_lookup_by_name (LIB_GLOB_GET_VR_CONTEXT (cli->zg), NULL);

      ret = str2prefix_ipv4 (argv[0], &p);
      if (ret <= 0)
        return nsm_mroute_cli_return (cli, NSM_API_SET_ERR_MALFORMED_ADDRESS);

      gw = argv[1];

      if (argc > 2)
        {
          if (!nsm_mroute_cli_ipv4_proto2route_type (argv[1], &route_type))
            return nsm_mroute_cli_return (cli, NSM_API_SET_ERR_UNKNOWN_UCAST_PROTO);

          gw = argv[2];
        }
    }

  if (! ivrf)
    return nsm_mroute_cli_return (cli, NSM_API_SET_ERR_VRF_NOT_EXIST);

  ret = nsm_ip_mroute_set (nm, AFI_IP, ivrf->id, (struct prefix *)&p,
       gw, route_type, (u_char)distance);


  return nsm_mroute_cli_return (cli, ret);
}

#ifdef HAVE_VRF
CLI (ip_mroute_prefix_distance,
     ip_mroute_prefix_distance_cmd,
     "ip mroute (vrf NAME|) A.B.C.D/M (" MROUTE_CLI_IPV4_PROTOS_CMD ") (A.B.C.D|INTERFACE) <1-255>",
     CLI_IP_STR,
     CLI_MROUTE_STR,
     CLI_VRF_STR,
     CLI_VRF_NAME_STR,
     "Source prefix",
     MROUTE_CLI_IPV4_PROTOS_STR,
     "RPF neighbor address or route",
     "RPF interface or pseudo interface Null",
     "Administrative distance for mroute")
#else
CLI (ip_mroute_prefix_distance,
     ip_mroute_prefix_distance_cmd,
     "ip mroute A.B.C.D/M (" MROUTE_CLI_IPV4_PROTOS_CMD ") (A.B.C.D|INTERFACE) <1-255>",
     CLI_IP_STR,
     CLI_MROUTE_STR,
     "Source prefix",
     MROUTE_CLI_IPV4_PROTOS_STR,
     "RPF neighbor address or route",
     "RPF interface or pseudo interface Null",
     "Administrative distance for mroute")
#endif /* HAVE_VRF */
{
  struct apn_vrf *ivrf;
  struct nsm_master *nm = cli->vr->proto;
  struct prefix_ipv4 p;
  int distance = APN_DISTANCE_MSTATIC;
  u_char route_type = APN_ROUTE_DEFAULT;
  char *gw = NULL;
  int ret;

  /* Get the VRF Instance  */
  if (argc > 1
      && ! pal_strcmp (argv [0], "vrf"))
    {
      ivrf = apn_vrf_lookup_by_name (LIB_GLOB_GET_VR_CONTEXT (cli->zg), argv [1]);

      ret = str2prefix_ipv4 (argv[2], &p);
      if (ret <= 0)
        return nsm_mroute_cli_return (cli, NSM_API_SET_ERR_MALFORMED_ADDRESS);

      gw = argv[3];

      if (argc > 5)
        {
          if (!nsm_mroute_cli_ipv4_proto2route_type (argv[3], &route_type))
            return nsm_mroute_cli_return (cli, NSM_API_SET_ERR_UNKNOWN_UCAST_PROTO);

          gw = argv[4];

          CLI_GET_INTEGER_RANGE ("Distance", distance, argv[5], 1, 255);
        }
      else
        CLI_GET_INTEGER_RANGE ("Distance", distance, argv[4], 1, 255);
    }
  else
    {
      ivrf = apn_vrf_lookup_by_name (LIB_GLOB_GET_VR_CONTEXT (cli->zg), NULL);

      ret = str2prefix_ipv4 (argv[0], &p);
      if (ret <= 0)
        return nsm_mroute_cli_return (cli, NSM_API_SET_ERR_MALFORMED_ADDRESS);

      gw = argv[1];

      if (argc > 3)
        {
          if (!nsm_mroute_cli_ipv4_proto2route_type (argv[1], &route_type))
            return nsm_mroute_cli_return (cli, NSM_API_SET_ERR_UNKNOWN_UCAST_PROTO);

          gw = argv[2];

          CLI_GET_INTEGER_RANGE ("Distance", distance, argv[3], 1, 255);
        }
      else
        CLI_GET_INTEGER_RANGE ("Distance", distance, argv[2], 1, 255);
    }

  if (! ivrf)
    return nsm_mroute_cli_return (cli, NSM_API_SET_ERR_VRF_NOT_EXIST);

  ret = nsm_ip_mroute_set (nm, AFI_IP, ivrf->id, (struct prefix *)&p,
      gw, route_type, (u_char)distance);

  return nsm_mroute_cli_return (cli, ret);
}

#ifdef HAVE_VRF
CLI (no_ip_mroute_prefix,
     no_ip_mroute_prefix_cmd,
     "no ip mroute (vrf NAME|) A.B.C.D/M (" MROUTE_CLI_IPV4_PROTOS_CMD ")",
     CLI_NO_STR,
     CLI_IP_STR,
     CLI_MROUTE_STR,
     CLI_VRF_STR,
     CLI_VRF_NAME_STR,
     "Source prefix",
     MROUTE_CLI_IPV4_PROTOS_STR)
#else
CLI (no_ip_mroute_prefix,
     no_ip_mroute_prefix_cmd,
     "no ip mroute A.B.C.D/M (" MROUTE_CLI_IPV4_PROTOS_CMD ")",
     CLI_NO_STR,
     CLI_IP_STR,
     CLI_MROUTE_STR,
     "Source prefix",
     MROUTE_CLI_IPV4_PROTOS_STR)
#endif /* HAVE_VRF */
{
  struct apn_vrf *ivrf;
  struct nsm_master *nm = cli->vr->proto;
  struct prefix_ipv4 p;
  u_char route_type = APN_ROUTE_DEFAULT;
  int ret;

  /* Get the VRF Instance  */
  if (argc > 1
      && ! pal_strcmp (argv [0], "vrf"))
    {
      ivrf = apn_vrf_lookup_by_name (LIB_GLOB_GET_VR_CONTEXT (cli->zg), argv [1]);

      ret = str2prefix_ipv4 (argv[2], &p);
      if (ret <= 0)
        return nsm_mroute_cli_return (cli, NSM_API_SET_ERR_MALFORMED_ADDRESS);

      if (argc > 3)
        {
          if (!nsm_mroute_cli_ipv4_proto2route_type (argv[3], &route_type))
            return nsm_mroute_cli_return (cli, NSM_API_SET_ERR_UNKNOWN_UCAST_PROTO);
        }
    }
  else
    {
      ivrf = apn_vrf_lookup_by_name (LIB_GLOB_GET_VR_CONTEXT (cli->zg), NULL);

      ret = str2prefix_ipv4 (argv[0], &p);
      if (ret <= 0)
        return nsm_mroute_cli_return (cli, NSM_API_SET_ERR_MALFORMED_ADDRESS);

      if (argc > 1)
        {
          if (!nsm_mroute_cli_ipv4_proto2route_type (argv[1], &route_type))
            return nsm_mroute_cli_return (cli, NSM_API_SET_ERR_UNKNOWN_UCAST_PROTO);
        }
    }

  if (! ivrf)
    return nsm_mroute_cli_return (cli, NSM_API_SET_ERR_VRF_NOT_EXIST);

  ret = nsm_ip_mroute_unset (nm, AFI_IP, ivrf->id, (struct prefix *)&p,
      route_type);

  return nsm_mroute_cli_return (cli, ret);
}

void
nsm_mrib_show_rpf (struct cli *cli, afi_t afi, 
    struct nsm_ptree_node *rn, struct rib *rib)
{
  struct apn_vr *vr = cli->vr;
  struct nexthop *nexthop;
  struct pal_in4_addr *gw = NULL;
#ifdef HAVE_IPV6
  struct pal_in6_addr *gw6 = NULL;
#endif /* HAVE_IPV6 */
  int recursive;
  int ifindex;
  struct prefix p;

  for (nexthop = rib->nexthop; nexthop; nexthop = nexthop->next)
    if (CHECK_FLAG (nexthop->flags, NEXTHOP_FLAG_FIB) ||
        VALID_MROUTE_NEXTHOP (nexthop))
      break;
  
  if (! nexthop)
    {
      cli_out (cli, "No valid mroute nexthop\n");
      return;
    }

  recursive = CHECK_FLAG (nexthop->flags, NEXTHOP_FLAG_RECURSIVE);

  ifindex = (recursive ? nexthop->rifindex : nexthop->ifindex);
  if (afi == AFI_IP)
    gw = (recursive ?  &nexthop->rgate.ipv4 : &nexthop->gate.ipv4);
#ifdef HAVE_IPV6
  else if (afi == AFI_IP6)
    gw6 = (recursive ?  &nexthop->rgate.ipv6 : &nexthop->gate.ipv6);
#endif /* HAVE_IPV6 */

  /* RPF Interface */
  if (CHECK_FLAG (rib->flags, RIB_FLAG_BLACKHOLE))
    cli_out (cli, "  RPF interface: %s\n", "Null0");
  else if (ifindex)
    cli_out (cli, "  RPF interface: %s\n", 
        if_index2name (&vr->ifm, ifindex));
  else if (nexthop->ifname)
    cli_out (cli, "  RPF interface: %s\n", nexthop->ifname);

  /* RPF Neighbor */
  switch ((recursive ? nexthop->rtype : nexthop->type))
    {
    case NEXTHOP_TYPE_IPV4:
    case NEXTHOP_TYPE_IPV4_IFINDEX:
    case NEXTHOP_TYPE_IPV4_IFNAME:
      cli_out (cli, "  RPF neighbor: %r\n", gw);
      break;
#ifdef HAVE_IPV6
    case NEXTHOP_TYPE_IPV6:
    case NEXTHOP_TYPE_IPV6_IFINDEX:
    case NEXTHOP_TYPE_IPV6_IFNAME:
      cli_out (cli, "  RPF neighbor: %R\n", gw6);
      break;
#endif /* HAVE_IPV6 */
    case NEXTHOP_TYPE_IFNAME:
    case NEXTHOP_TYPE_IFINDEX:
        {
          struct pal_in4_addr dummy = {0};

          if (afi == AFI_IP)
            cli_out (cli, "  RPF neighbor: %r\n", &dummy);
#ifdef HAVE_IPV6
          else if (afi == AFI_IP6)
            cli_out (cli, "  RPF neighbor: %R\n", &pal_in6addr_any);
#endif /* HAVE_IPV6 */
          break;
        }
    }

  pal_mem_set(&p, 0, sizeof (struct prefix));
  p.family = rn->tree->ptype;
  p.prefixlen = rn->key_len;
  if (p.family == AF_INET)
    NSM_PTREE_KEY_COPY (&p.u.prefix4, rn);
#ifdef HAVE_IPV6
  else if (p.family == AF_INET6)
    NSM_PTREE_KEY_COPY (&p.u.prefix6, rn);
#endif
  /* RPF route */
  if (afi == AFI_IP)
    cli_out (cli, "  RPF route: %P\n", &p);
#ifdef HAVE_IPV6
  else if (afi == AFI_IP6)
    cli_out (cli, "  RPF route: %Q\n", &p);
#endif /* HAVE_IPV6 */

  if (CHECK_FLAG (rib->ext_flags, RIB_EXT_FLAG_MROUTE))
    cli_out (cli, "  RPF type: %s\n", route_type_str (rib->type));
  else
    cli_out (cli, "  RPF type: unicast (%s)\n", 
        route_type_str (rib->type));

  cli_out (cli, "  RPF recursion count: %d\n", recursive ? 1 : 0);
  cli_out (cli, "  Doing distance-preferred lookups across tables\n");

  cli_out (cli, "  Distance: %d\n", rib->distance);
  cli_out (cli, "  Metric: %d\n", rib->metric);

  return;
}

#ifdef HAVE_VRF
CLI (show_ip_rpf,
     show_ip_rpf_cmd,
     "show ip rpf (vrf NAME|) A.B.C.D",
     CLI_SHOW_STR,
     CLI_IP_STR,
     "Display RPF information for multicast source",
     CLI_VRF_STR,
     CLI_VRF_NAME_STR,
     "IP address of multicast source")
#else
CLI (show_ip_rpf,
     show_ip_rpf_cmd,
     "show ip rpf A.B.C.D",
     CLI_SHOW_STR,
     CLI_IP_STR,
     "Display RPF information for multicast source",
     "IP address of multicast source")
#endif /* HAVE_VRF */
{
  struct apn_vrf *ivrf;
  struct nsm_master *nm = cli->vr->proto;
  struct prefix p;
  struct nsm_ptree_node *rn;
  struct nsm_vrf *nv;
  struct rib *rib;
  char *pfx = NULL;

  /* Get the VRF Instance  */
  if (argc > 1
      && ! pal_strcmp (argv [0], "vrf"))
    {
      ivrf = apn_vrf_lookup_by_name (LIB_GLOB_GET_VR_CONTEXT (cli->zg), argv [1]);

      pfx = argv[2];
    }
  else
    {
      ivrf = apn_vrf_lookup_by_name (LIB_GLOB_GET_VR_CONTEXT (cli->zg), NULL);

      pfx = argv[0];
    }

  if (! ivrf)
    return nsm_mroute_cli_return (cli, NSM_API_SET_ERR_VRF_NOT_EXIST);

  nv = nsm_vrf_lookup_by_id (nm, ivrf->id);
  if (! nv)
    return nsm_mroute_cli_return (cli, NSM_API_SET_ERR_VRF_NOT_EXIST);

  p.family = AF_INET;
  p.prefixlen = IPV4_MAX_BITLEN;

  CLI_GET_IPV4_ADDRESS ("Source address", p.u.prefix4, pfx);

  rib = nsm_mrib_match (nv, AFI_IP, &p, &rn, 0);
  if (rib == NULL)
    cli_out (cli, "RPF information for %r failed, no route exists\n"
        , &p.u.prefix4);
  else
    {
      cli_out (cli, "RPF information for %r\n", &p.u.prefix4);
      nsm_mrib_show_rpf (cli, AFI_IP, rn, rib); 
    }

  return CLI_SUCCESS;
}

#ifdef HAVE_IPV6
static int
nsm_mroute_cli_ipv6_proto2route_type (char *str, u_char *route_type)
{
  /* Sanity check. */
  if (str == NULL)
    return 0;

  if (pal_strncmp (str, "s", 1) == 0)
    *route_type = APN_ROUTE_STATIC;
  else if (pal_strncmp (str, "r", 1) == 0)
    *route_type = APN_ROUTE_RIPNG;
  else if (pal_strncmp (str, "o", 1) == 0)
    *route_type = APN_ROUTE_OSPF6;
  else if (pal_strncmp (str, "b", 1) == 0)
    *route_type = APN_ROUTE_BGP;
  else if (pal_strncmp (str, "i", 1) == 0)
    *route_type = APN_ROUTE_ISIS;
  else
    return 0;

  return 1;
}

/* Static mroute CLIs */

#define MROUTE_CLI_IPV6_PROTOS_CMD                                            \
    "static|rip|ospf|bgp|isis|"

#define MROUTE_CLI_IPV6_PROTOS_STR                                            \
    "Static routes",                                                          \
    "Routing Information Protocol (RIPng)",                                   \
    "Open Shortest Patch First (OSPF)",                                       \
    "Border Gateway Protocol (BGP)",                                          \
    "ISO IS-IS"

#ifdef HAVE_VRF
CLI (ipv6_mroute_prefix,
     ipv6_mroute_prefix_cmd,
     "ipv6 mroute (vrf NAME|) X:X::X:X/M (" MROUTE_CLI_IPV6_PROTOS_CMD ") (X:X::X:X|INTERFACE)",
     CLI_IPV6_STR,
     CLI_MROUTE_STR,
     CLI_VRF_STR,
     CLI_VRF_NAME_STR,
     "Source prefix",
     MROUTE_CLI_IPV6_PROTOS_STR,
     "RPF neighbor address or route",
     "RPF interface or pseudo interface Null")
#else
CLI (ipv6_mroute_prefix,
     ipv6_mroute_prefix_cmd,
     "ipv6 mroute X:X::X:X/M (" MROUTE_CLI_IPV6_PROTOS_CMD ") (X:X::X:X|INTERFACE)",
     CLI_IPV6_STR,
     CLI_MROUTE_STR,
     "Source prefix",
     MROUTE_CLI_IPV6_PROTOS_STR,
     "RPF neighbor address or route",
     "RPF interface or pseudo interface Null")
#endif /* HAVE_VRF */
{
  struct apn_vrf *ivrf;
  struct nsm_master *nm = cli->vr->proto;
  struct prefix_ipv6 p;
  int distance = APN_DISTANCE_MSTATIC;
  u_char route_type = APN_ROUTE_DEFAULT;
  char *gw = NULL;
  int ret;

  /* Get the VRF Instance  */
  if (argc > 1
      && ! pal_strcmp (argv [0], "vrf"))
    {
      ivrf = apn_vrf_lookup_by_name (LIB_GLOB_GET_VR_CONTEXT (cli->zg), argv [1]);

      ret = str2prefix_ipv6 (argv[2], &p);
      if (ret <= 0)
        return nsm_mroute_cli_return (cli, NSM_API_SET_ERR_MALFORMED_ADDRESS);

      gw = argv[3];

      if (argc > 4)
        {
          if (!nsm_mroute_cli_ipv6_proto2route_type (argv[3], &route_type))
            return nsm_mroute_cli_return (cli, NSM_API_SET_ERR_UNKNOWN_UCAST_PROTO);
          else
            gw = argv[4];
        }
    }
  else
    {
      ivrf = apn_vrf_lookup_by_name (LIB_GLOB_GET_VR_CONTEXT (cli->zg), NULL);

      ret = str2prefix_ipv6 (argv[0], &p);
      if (ret <= 0)
        return nsm_mroute_cli_return (cli, NSM_API_SET_ERR_MALFORMED_ADDRESS);

      gw = argv[1];

      if (argc > 2)
        {
          if (!nsm_mroute_cli_ipv6_proto2route_type (argv[1], &route_type))
            return nsm_mroute_cli_return (cli, NSM_API_SET_ERR_UNKNOWN_UCAST_PROTO);
          else
            gw = argv[2];
        }
    }

  if (! ivrf)
    return nsm_mroute_cli_return (cli, NSM_API_SET_ERR_VRF_NOT_EXIST);

  ret = nsm_ip_mroute_set (nm, AFI_IP6, ivrf->id, (struct prefix *)&p,
      gw, route_type, (u_char)distance);

  return nsm_mroute_cli_return (cli, ret);
}

#ifdef HAVE_VRF
CLI (ipv6_mroute_prefix_ifname,
     ipv6_mroute_prefix_ifname_cmd,
     "ipv6 mroute (vrf NAME|) X:X::X:X/M (" MROUTE_CLI_IPV6_PROTOS_CMD ") X:X::X:X INTERFACE",
     CLI_IPV6_STR,
     CLI_MROUTE_STR,
     CLI_VRF_STR,
     CLI_VRF_NAME_STR,
     "Source prefix",
     MROUTE_CLI_IPV6_PROTOS_STR,
     "RPF neighbor address or route",
     "RPF interface or pseudo interface Null")
#else
CLI (ipv6_mroute_prefix_ifname,
     ipv6_mroute_prefix_ifname_cmd,
     "ipv6 mroute X:X::X:X/M (" MROUTE_CLI_IPV6_PROTOS_CMD ") X:X::X:X INTERFACE",
     CLI_IPV6_STR,
     CLI_MROUTE_STR,
     "Source prefix",
     MROUTE_CLI_IPV6_PROTOS_STR,
     "RPF neighbor address or route",
     "RPF interface or pseudo interface Null")
#endif /* HAVE_VRF */
{
  struct apn_vrf *ivrf;
  struct nsm_master *nm = cli->vr->proto;
  struct prefix_ipv6 p;
  int distance = APN_DISTANCE_MSTATIC;
  u_char route_type = APN_ROUTE_DEFAULT;
  char *ifname = NULL;
  struct prefix gw;
  int ret;

  /* Get the VRF Instance  */
  if (argc > 1
      && ! pal_strcmp (argv [0], "vrf"))
    {
      ivrf = apn_vrf_lookup_by_name (LIB_GLOB_GET_VR_CONTEXT (cli->zg), argv [1]);

      gw.family = AF_INET6;
      gw.prefixlen = IPV6_MAX_BITLEN;

      ret = str2prefix_ipv6 (argv[2], &p);
      if (ret <= 0)
        return nsm_mroute_cli_return (cli, NSM_API_SET_ERR_MALFORMED_ADDRESS);

      if (argc > 5)
        {
          if (!nsm_mroute_cli_ipv6_proto2route_type (argv[3], &route_type))
            return nsm_mroute_cli_return (cli, NSM_API_SET_ERR_UNKNOWN_UCAST_PROTO);

          CLI_GET_IPV6_ADDRESS ("Gateway address", gw.u.prefix6, argv[4]);
          ifname = argv[5];
        }
      else
        {
          CLI_GET_IPV6_ADDRESS ("Gateway address", gw.u.prefix6, argv[3]);
          ifname = argv[4];
        }
    }
  else
    {
      ivrf = apn_vrf_lookup_by_name (LIB_GLOB_GET_VR_CONTEXT (cli->zg), NULL);

      gw.family = AF_INET6;
      gw.prefixlen = IPV6_MAX_BITLEN;

      ret = str2prefix_ipv6 (argv[0], &p);
      if (ret <= 0)
        return nsm_mroute_cli_return (cli, NSM_API_SET_ERR_MALFORMED_ADDRESS);

      if (argc > 3)
        {
          if (!nsm_mroute_cli_ipv6_proto2route_type (argv[1], &route_type))
            return nsm_mroute_cli_return (cli, NSM_API_SET_ERR_UNKNOWN_UCAST_PROTO);

          CLI_GET_IPV6_ADDRESS ("Gateway address", gw.u.prefix6, argv[2]);
          ifname = argv[3];
        }
      else
        {
          CLI_GET_IPV6_ADDRESS ("Gateway address", gw.u.prefix6, argv[1]);
          ifname = argv[2];
        }
    }

  if (! ivrf)
    return nsm_mroute_cli_return (cli, NSM_API_SET_ERR_VRF_NOT_EXIST);

  ret = nsm_ip_mroute_ifname_set (nm, AFI_IP6, ivrf->id, 
      (struct prefix *)&p, &gw, ifname, route_type,
      (u_char)distance);

  return nsm_mroute_cli_return (cli, ret);
}

#ifdef HAVE_VRF
CLI (ipv6_mroute_prefix_distance,
     ipv6_mroute_prefix_distance_cmd,
     "ipv6 mroute (vrf NAME|) X:X::X:X/M (" MROUTE_CLI_IPV6_PROTOS_CMD ") (X:X::X:X|INTERFACE) <1-255>",
     CLI_IPV6_STR,
     CLI_MROUTE_STR,
     CLI_VRF_STR,
     CLI_VRF_NAME_STR,
     "Source prefix",
     MROUTE_CLI_IPV6_PROTOS_STR,
     "RPF neighbor address or route",
     "RPF interface or pseudo interface Null",
     "Administrative distance for mroute")
#else
CLI (ipv6_mroute_prefix_distance,
     ipv6_mroute_prefix_distance_cmd,
     "ipv6 mroute X:X::X:X/M (" MROUTE_CLI_IPV6_PROTOS_CMD ") (X:X::X:X|INTERFACE) <1-255>",
     CLI_IPV6_STR,
     CLI_MROUTE_STR,
     "Source prefix",
     MROUTE_CLI_IPV6_PROTOS_STR,
     "RPF neighbor address or route",
     "RPF interface or pseudo interface Null",
     "Administrative distance for mroute")
#endif /* HAVE_VRF */
{
  struct apn_vrf *ivrf;
  struct nsm_master *nm = cli->vr->proto;
  struct prefix_ipv6 p;
  int distance = APN_DISTANCE_MSTATIC;
  u_char route_type = APN_ROUTE_DEFAULT;
  char *gw = NULL;
  int ret;

  /* Get the VRF Instance  */
  if (argc > 1
      && ! pal_strcmp (argv [0], "vrf"))
    {
      ivrf = apn_vrf_lookup_by_name (LIB_GLOB_GET_VR_CONTEXT (cli->zg), argv [1]);

      ret = str2prefix_ipv6 (argv[2], &p);
      if (ret <= 0)
        return nsm_mroute_cli_return (cli, NSM_API_SET_ERR_MALFORMED_ADDRESS);

      gw = argv[3];

      if (argc > 5)
        {
          if (!nsm_mroute_cli_ipv6_proto2route_type (argv[3], &route_type))
            return nsm_mroute_cli_return (cli, NSM_API_SET_ERR_UNKNOWN_UCAST_PROTO);

          gw = argv[4];

          CLI_GET_INTEGER_RANGE ("Distance", distance, argv[5], 1, 255);
        }
      else
        CLI_GET_INTEGER_RANGE ("Distance", distance, argv[4], 1, 255);
    }
  else
    {
      ivrf = apn_vrf_lookup_by_name (LIB_GLOB_GET_VR_CONTEXT (cli->zg), NULL);

      ret = str2prefix_ipv6 (argv[0], &p);
      if (ret <= 0)
        return nsm_mroute_cli_return (cli, NSM_API_SET_ERR_MALFORMED_ADDRESS);

      gw = argv[1];

      if (argc > 3)
        {
          if (!nsm_mroute_cli_ipv6_proto2route_type (argv[1], &route_type))
            return nsm_mroute_cli_return (cli, NSM_API_SET_ERR_UNKNOWN_UCAST_PROTO);

          gw = argv[2];

          CLI_GET_INTEGER_RANGE ("Distance", distance, argv[3], 1, 255);
        }
      else
        CLI_GET_INTEGER_RANGE ("Distance", distance, argv[2], 1, 255);
    }

  if (! ivrf)
    return nsm_mroute_cli_return (cli, NSM_API_SET_ERR_VRF_NOT_EXIST);

  ret = nsm_ip_mroute_set (nm, AFI_IP6, ivrf->id, (struct prefix *)&p,
      gw, route_type, (u_char)distance);

  return nsm_mroute_cli_return (cli, ret);
}

#ifdef HAVE_VRF
CLI (ipv6_mroute_prefix_ifname_distance,
     ipv6_mroute_prefix_ifname_distance_cmd,
     "ipv6 mroute (vrf NAME|) X:X::X:X/M (" MROUTE_CLI_IPV6_PROTOS_CMD ") X:X::X:X INTERFACE <1-255>",
     CLI_IPV6_STR,
     CLI_MROUTE_STR,
     CLI_VRF_STR,
     CLI_VRF_NAME_STR,
     "Source prefix",
     MROUTE_CLI_IPV6_PROTOS_STR,
     "RPF neighbor address or route",
     "RPF interface or pseudo interface Null",
     "Administrative distance for mroute")
#else
CLI (ipv6_mroute_prefix_ifname_distance,
     ipv6_mroute_prefix_ifname_distance_cmd,
     "ipv6 mroute X:X::X:X/M (" MROUTE_CLI_IPV6_PROTOS_CMD ") X:X::X:X INTERFACE <1-255>",
     CLI_IPV6_STR,
     CLI_MROUTE_STR,
     "Source prefix",
     MROUTE_CLI_IPV6_PROTOS_STR,
     "RPF neighbor address or route",
     "RPF interface or pseudo interface Null",
     "Administrative distance for mroute")
#endif /* HAVE_VRF */
{
  struct apn_vrf *ivrf;
  struct nsm_master *nm = cli->vr->proto;
  struct prefix_ipv6 p;
  int distance = APN_DISTANCE_MSTATIC;
  u_char route_type = APN_ROUTE_DEFAULT;
  char *ifname = NULL;
  struct prefix gw;
  int ret;

  /* Get the VRF Instance  */
  if (argc > 1
      && ! pal_strcmp (argv [0], "vrf"))
    {
      ivrf = apn_vrf_lookup_by_name (LIB_GLOB_GET_VR_CONTEXT (cli->zg), argv [1]);

      gw.family = AF_INET6;
      gw.prefixlen = IPV6_MAX_BITLEN;

      ret = str2prefix_ipv6 (argv[2], &p);
      if (ret <= 0)
        return nsm_mroute_cli_return (cli, NSM_API_SET_ERR_MALFORMED_ADDRESS);

      if (argc > 6)
        {
          if (!nsm_mroute_cli_ipv6_proto2route_type (argv[3], &route_type))
            return nsm_mroute_cli_return (cli, NSM_API_SET_ERR_UNKNOWN_UCAST_PROTO);

          CLI_GET_IPV6_ADDRESS ("Gateway address", gw.u.prefix6, argv[4]);

          ifname = argv[5];

          CLI_GET_INTEGER_RANGE ("Distance", distance, argv[6], 1, 255);
        }
      else
        {
          CLI_GET_IPV6_ADDRESS ("Gateway address", gw.u.prefix6, argv[3]);

          ifname = argv[4];

          CLI_GET_INTEGER_RANGE ("Distance", distance, argv[5], 1, 255);
        }
    }
  else
    {
      ivrf = apn_vrf_lookup_by_name (LIB_GLOB_GET_VR_CONTEXT (cli->zg), NULL);

      gw.family = AF_INET6;
      gw.prefixlen = IPV6_MAX_BITLEN;

      ret = str2prefix_ipv6 (argv[0], &p);
      if (ret <= 0)
        return nsm_mroute_cli_return (cli, NSM_API_SET_ERR_MALFORMED_ADDRESS);

      if (argc > 4)
        {
          if (!nsm_mroute_cli_ipv6_proto2route_type (argv[1], &route_type))
            return nsm_mroute_cli_return (cli, NSM_API_SET_ERR_UNKNOWN_UCAST_PROTO);

          CLI_GET_IPV6_ADDRESS ("Gateway address", gw.u.prefix6, argv[2]);

          ifname = argv[3];

          CLI_GET_INTEGER_RANGE ("Distance", distance, argv[4], 1, 255);
        }
      else
        {
          CLI_GET_IPV6_ADDRESS ("Gateway address", gw.u.prefix6, argv[1]);

          ifname = argv[2];

          CLI_GET_INTEGER_RANGE ("Distance", distance, argv[3], 1, 255);
        }
    }

  if (! ivrf)
    return nsm_mroute_cli_return (cli, NSM_API_SET_ERR_VRF_NOT_EXIST);

  ret = nsm_ip_mroute_ifname_set (nm, AFI_IP6, ivrf->id, 
      (struct prefix *)&p, &gw, ifname, route_type,
      (u_char)distance);

  return nsm_mroute_cli_return (cli, ret);
}

#ifdef HAVE_VRF
CLI (no_ipv6_mroute_prefix,
     no_ipv6_mroute_prefix_cmd,
     "no ipv6 mroute (vrf NAME|) X:X::X:X/M (" MROUTE_CLI_IPV6_PROTOS_CMD ")",
     CLI_NO_STR,
     CLI_IPV6_STR,
     CLI_MROUTE_STR,
     CLI_VRF_STR,
     CLI_VRF_NAME_STR,
     "Source prefix",
     MROUTE_CLI_IPV6_PROTOS_STR)
#else
CLI (no_ipv6_mroute_prefix,
     no_ipv6_mroute_prefix_cmd,
     "no ipv6 mroute X:X::X:X/M (" MROUTE_CLI_IPV6_PROTOS_CMD ")",
     CLI_NO_STR,
     CLI_IPV6_STR,
     CLI_MROUTE_STR,
     "Source prefix",
     MROUTE_CLI_IPV6_PROTOS_STR)
#endif /* HAVE_VRF */
{
  struct apn_vrf *ivrf;
  struct nsm_master *nm = cli->vr->proto;
  struct prefix_ipv6 p;
  u_char route_type = APN_ROUTE_DEFAULT;
  int ret;

  /* Get the VRF Instance  */
  if (argc > 1
      && ! pal_strcmp (argv [0], "vrf"))
    {
      ivrf = apn_vrf_lookup_by_name (LIB_GLOB_GET_VR_CONTEXT (cli->zg), argv [1]);

      ret = str2prefix_ipv6 (argv[2], &p);
      if (ret <= 0)
        return nsm_mroute_cli_return (cli, NSM_API_SET_ERR_MALFORMED_ADDRESS);

      if (argc > 3)
        {
          if (!nsm_mroute_cli_ipv6_proto2route_type (argv[3], &route_type))
            return nsm_mroute_cli_return (cli, NSM_API_SET_ERR_UNKNOWN_UCAST_PROTO);
        }
    }
  else
    {
      ivrf = apn_vrf_lookup_by_name (LIB_GLOB_GET_VR_CONTEXT (cli->zg), NULL);

      ret = str2prefix_ipv6 (argv[0], &p);
      if (ret <= 0)
        return nsm_mroute_cli_return (cli, NSM_API_SET_ERR_MALFORMED_ADDRESS);

      if (argc > 1)
        {
          if (!nsm_mroute_cli_ipv6_proto2route_type (argv[1], &route_type))
            return nsm_mroute_cli_return (cli, NSM_API_SET_ERR_UNKNOWN_UCAST_PROTO);
        }
    }

  if (! ivrf)
    return nsm_mroute_cli_return (cli, NSM_API_SET_ERR_VRF_NOT_EXIST);

  ret = nsm_ip_mroute_unset (nm, AFI_IP6, ivrf->id, (struct prefix *)&p,
      route_type);

  return nsm_mroute_cli_return (cli, ret);
}

#ifdef HAVE_VRF
CLI (show_ipv6_rpf,
     show_ipv6_rpf_cmd,
     "show ipv6 rpf (vrf NAME|) X:X::X:X",
     CLI_SHOW_STR,
     CLI_IPV6_STR,
     "Display RPF information for multicast source",
     CLI_VRF_STR,
     CLI_VRF_NAME_STR,
     "IPv6 address of multicast source")
#else
CLI (show_ipv6_rpf,
     show_ipv6_rpf_cmd,
     "show ipv6 rpf X:X::X:X",
     CLI_SHOW_STR,
     CLI_IPV6_STR,
     "Display RPF information for multicast source",
     "IPv6 address of multicast source")
#endif /* HAVE_VRF */
{
  struct apn_vrf *ivrf;
  struct nsm_master *nm = cli->vr->proto;
  struct prefix p;
  struct nsm_ptree_node *rn;
  struct nsm_vrf *nv;
  struct rib *rib;
  char *pfx = NULL;

  /* Get the VRF Instance  */
  if (argc > 1
      && ! pal_strcmp (argv [0], "vrf"))
    {
      ivrf = apn_vrf_lookup_by_name (LIB_GLOB_GET_VR_CONTEXT (cli->zg), argv [1]);
      pfx = argv[2];
    }
  else
    {
      ivrf = apn_vrf_lookup_by_name (LIB_GLOB_GET_VR_CONTEXT (cli->zg), NULL);

      pfx = argv[0];
    }

  if (! ivrf)
    return nsm_mroute_cli_return (cli, NSM_API_SET_ERR_VRF_NOT_EXIST);

  nv = nsm_vrf_lookup_by_id (nm, ivrf->id);
  if (! nv)
    return nsm_mroute_cli_return (cli, NSM_API_SET_ERR_VRF_NOT_EXIST);

  p.family = AF_INET6;
  p.prefixlen = IPV6_MAX_BITLEN;

  CLI_GET_IPV6_ADDRESS ("Source address", p.u.prefix6, pfx);

  rib = nsm_mrib_match (nv, AFI_IP6, &p, &rn, 0);
  if (rib == NULL)
    cli_out (cli, "RPF information for %R failed, no route exists\n"
        , &p.u.prefix6);
  else
    {
      cli_out (cli, "RPF information for %R\n", &p.u.prefix6);
      nsm_mrib_show_rpf (cli, AFI_IP6, rn, rib); 
    }

  return CLI_SUCCESS;
}
#endif /* HAVE_IPV6 */

/* Write static mroute configuration. */
int
nsm_config_write_ip_mroute (struct cli *cli, struct nsm_vrf *nv, 
    afi_t afi)
{
  struct ptree_node *rn;
  struct nsm_mroute_config *mc;
  char buf1[5];
  char buf2[45];
  char vrf[NSM_VRF_NAMELEN_MAX];
  int write = 0;
  struct prefix p;

  if (!NSM_CAP_HAVE_VRF && nv->vrf->id != 0)
    return 0;

  vrf[0] = '\0';

#ifdef HAVE_VRF
  if(LIB_VRF_GET_VRF_NAME (nv->vrf))
    pal_snprintf (vrf, NSM_VRF_NAMELEN_MAX, " vrf %s",
        LIB_VRF_GET_VRF_NAME (nv->vrf));
#endif /* HAVE_VRF */

  for (rn = ptree_top (nv->afi[afi].ip_mroute); rn; rn = ptree_next (rn))
    {
      if (rn->info == NULL)
        continue;
      pal_mem_set(&p, 0, sizeof (struct prefix));
      p.prefixlen = rn->key_len;
      if (afi == AFI_IP)
        {
          p.family = AF_INET;
          NSM_PTREE_KEY_COPY (&p.u.prefix4, rn);
        }
#ifdef HAVE_IPV6
      else if (afi == AFI_IP6)
        {
          p.family = AF_INET6;
          NSM_PTREE_KEY_COPY (&p.u.prefix6, rn);
        }
#endif
      if (afi == AFI_IP)
        {
          zsnprintf (buf1, 5, "%s", "ip");
          zsnprintf (buf2, 45, "%P", &p);
        }
#ifdef HAVE_IPV6
      else if (afi == AFI_IP6)
        {
          zsnprintf (buf1, 5, "%s", "ipv6");
          zsnprintf (buf2, 45, "%Q", &p);
        }
#endif /* HAVE_IPV6 */

      for (mc = rn->info; mc; mc = mc->next)
        {
          cli_out (cli, "%s mroute%s %s", buf1, vrf, buf2);

          if (mc->uc_rtype != APN_ROUTE_DEFAULT)
            cli_out (cli, " %s", nsm_mroute_cli_route2proto (mc->uc_rtype));

          switch (mc->type)
            {
            case NEXTHOP_TYPE_IPV4:
              cli_out (cli, " %r", &mc->gate.ipv4);
              break;
            case NEXTHOP_TYPE_IFNAME:
              cli_out (cli, " %s", mc->ifname);
              break;
            case NEXTHOP_TYPE_IPV4_IFNAME:
              cli_out (cli, " %r %s", &mc->gate.ipv4, mc->ifname);
              break;
#ifdef HAVE_IPV6
            case NEXTHOP_TYPE_IPV6:
              cli_out (cli, " %R", &mc->gate.ipv6);
              break;
            case NEXTHOP_TYPE_IPV6_IFNAME:
              cli_out (cli, " %R %s", &mc->gate.ipv6, mc->ifname);
              break;
#endif /* HAVE_IPV6 */
            }

          if (mc->distance != APN_DISTANCE_MSTATIC)
            cli_out (cli, " %d", mc->distance);

          cli_out (cli, "\n");

          write++;
        }
    }

  return write;
}

#ifdef HAVE_L3
void
nsm_cli_init_static_mroute (struct cli_tree *ctree)
{
  /* "ip mroute" configuration CLIs */
  cli_install_gen (ctree, CONFIG_MODE, PRIVILEGE_NORMAL, 0,
                   &ip_mroute_prefix_cmd);
  cli_install_gen (ctree, CONFIG_MODE, PRIVILEGE_NORMAL, 0,
                   &ip_mroute_prefix_distance_cmd);
  cli_install_gen (ctree, CONFIG_MODE, PRIVILEGE_NORMAL, 0,
                   &no_ip_mroute_prefix_cmd);

  /* "show ip rpf" CLI */
  cli_install_gen (ctree, EXEC_MODE, PRIVILEGE_NORMAL, 0,
                   &show_ip_rpf_cmd);

#ifdef HAVE_IPV6
  if (NSM_CAP_HAVE_IPV6)
    {
      /* "ipv6 mroute" configuration CLIs */
      cli_install_gen (ctree, CONFIG_MODE, PRIVILEGE_NORMAL, 0,
          &ipv6_mroute_prefix_cmd);
      cli_install_gen (ctree, CONFIG_MODE, PRIVILEGE_NORMAL, 0,
          &ipv6_mroute_prefix_ifname_cmd);
      cli_install_gen (ctree, CONFIG_MODE, PRIVILEGE_NORMAL, 0,
          &ipv6_mroute_prefix_distance_cmd);
      cli_install_gen (ctree, CONFIG_MODE, PRIVILEGE_NORMAL, 0,
          &ipv6_mroute_prefix_ifname_distance_cmd);
      cli_install_gen (ctree, CONFIG_MODE, PRIVILEGE_NORMAL, 0,
          &no_ipv6_mroute_prefix_cmd);

      /* "show ip rpf" CLI */
      cli_install_gen (ctree, EXEC_MODE, PRIVILEGE_NORMAL, 0,
          &show_ipv6_rpf_cmd);
    }
#endif /* HAVE_IPV6 */

  return;
}
#endif /* HAVE_L3 */
