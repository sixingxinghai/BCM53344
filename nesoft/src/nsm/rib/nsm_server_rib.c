/* Copyright (C) 2002-2003  All Rights Reserved. */

/* NSM protocol server implementation.  */

#include "pal.h"
#include "lib.h"
#include "thread.h"
#include "vector.h"
#include "message.h"
#include "network.h"
#include "nsmd.h"
#include "nexthop.h"
#include "cli.h"
#ifdef HAVE_DSTE
#include "dste.h"
#endif /* HAVE_DSTE */
#include "nsm_message.h"
#include "log.h"
#include "table.h"
#include "ptree.h"

#include "rib/rib.h"

#include "nsm_server.h"
#include "rib/nsm_server_rib.h"
#include "nsm_router.h"
#include "nsm_debug.h"
#include "rib/nsm_redistribute.h"
#include "nsm_interface.h"
#include "rib/nsm_nexthop.h"
#ifdef HAVE_LACPD
#include "nsm_lacp.h"
#endif /* HAVE_LACPD */
#include "rib/nsm_static_mroute.h"
#ifdef HAVE_MPLS
#include "mpls.h"
#include "nsm_mpls.h"
#include "nsm_lp_serv.h"
#ifdef HAVE_MPLS_VC
#include "nsm_mpls_vc.h"
#endif /* HAVE_MPLS_VC */
#include "nsm_mpls_rib.h"
#endif /* HAVE_MPLS */

#ifdef HAVE_DSTE
#include "nsm_dste.h"
#endif /* HAVE_DSTE */

#ifdef HAVE_VPLS
#include "nsm_vpls.h"
#endif /* HAVE_VPLS */

#ifdef HAVE_MPLS
#include "nsm_mpls.h"
#endif /* HAVE_MPLS */

#ifdef HAVE_MPLS_FRR
#include "mpls_common.h"
#include "mpls_client.h" 
#endif /* HAVE_MPLS_FRR */


#ifdef HAVE_MCAST_IPV4
#include "lib_mtrace.h"
#endif /* HAVE_MCAST_IPV4 */

#ifdef HAVE_MCAST_IPV6
#include "nsm_mcast6.h"
#endif /* HAVE_MCAST_IPV6 */

#ifdef HAVE_CRX
#include "crx_vip.h"
#include "crx_rib.h"
#endif /* HAVE_CRX */

#ifdef HAVE_RMM
#include "nsm_rd.h"
void nsm_rib_free (struct rib *);
#endif /* HAVE_RMM */

#ifdef HAVE_RMOND
#include "nsm_rmon.h"
#endif

#ifdef HAVE_HA
#include "nsm_cal.h"
#endif /* HAVE_HA */

/* Null RIB. */
struct rib RibNull;

struct
{
  int (*send_route) (struct nsm_server_entry *, u_int32_t, struct prefix *,
                     u_int16_t prefixlen, struct rib *, struct nsm_vrf *,
                     int, struct ftn_entry *);

} nsm_afi[AFI_MAX] =
  {
    { NULL },
    { nsm_send_ipv4_add },
#ifdef HAVE_IPV6
    { nsm_send_ipv6_add }
#else /* HAVE_IPV6 */
    { NULL }
#endif /* HAVE_IPV6 */
  };

struct
{
  int (*send_route) (struct nsm_server_entry *, u_int32_t, struct prefix *,
                     u_int16_t prefixlen, struct rib *, struct nsm_vrf *,
                     int, struct ftn_entry *);

} nsm_afi_override[AFI_MAX] =
  {
    { NULL },
    { nsm_send_ipv4_add_override },
#ifdef HAVE_IPV6
    { nsm_send_ipv6_add_override }
#else /* HAVE_IPV6 */
    { NULL }
#endif /* HAVE_IPV6 */
  };


/* Send IPv4 add to client.  */
int
nsm_send_ipv4_add_final (struct nsm_server_entry *nse, u_int32_t msg_id,
                         struct prefix *p, u_int16_t prefixlen,
                         struct rib *rib, struct nsm_vrf *vrf, int bundle,
                         struct ftn_entry *ftn, int override_adv)
{
  struct apn_vr *vr = vrf->vrf->vr;
  struct nsm_msg_route_ipv4 msg;
  struct nexthop *nexthop;
  struct interface *ifp;
  u_int32_t ifindex;
  bool_t advertise = NSM_FALSE;
  struct ptree_node *rn = NULL;
  struct nsm_static *ns = NULL;
#ifdef HAVE_MPLS
  struct nhlfe_key *nkey = NULL;
#endif /*HAVE_MPLS*/

  /* Flag is 1 for normal IPv4 route addition.  */
  msg.flags = 0;
  SET_FLAG (msg.flags, NSM_MSG_ROUTE_FLAG_ADD);

  if (nse->nsc->nhtenable == PAL_TRUE
      && CHECK_FLAG(rib->pflags, NSM_ROUTE_CHG_INFORM_BGP))
    {
      if (IS_NSM_DEBUG_EVENT) 
        zlog_info (nse->ns->zg, 
                   "nsm_send_ipv4_add_final:: NSM_ROUTE_CHG_INFORM_BGP\n");
      SET_FLAG (msg.flags, NSM_MSG_ROUTE_FLAG_NH_CHANGE);
      UNSET_FLAG(rib->pflags, NSM_ROUTE_CHG_INFORM_BGP);
    }

  msg.type = rib->type;
  msg.sub_type = rib->sub_type;
  msg.distance = rib->distance;
  msg.metric = rib->metric;
  msg.prefixlen = prefixlen ? prefixlen : p->prefixlen;
  msg.prefix = p->u.prefix4;
  msg.cindex = 0;
  msg.nexthop_num = 0;

  /* Tag */
  if (rib->tag)
    {
      NSM_SET_CTYPE (msg.cindex, NSM_ROUTE_CTYPE_IPV4_TAG);
      msg.tag = rib->tag;
    }

#ifdef HAVE_MPLS
  IF_NSM_CAP_HAVE_MPLS
    {
      if (ftn)
        nkey = (struct nhlfe_key *) &FTN_NHLFE (ftn)->nkey;

      if (ftn
          && FTN_XC (ftn)
          && FTN_NHLFE (ftn)
          && nkey->afi == AFI_IP
          && nkey->u.ipv4.out_label != LABEL_IPV4_EXP_NULL)
        {
          NSM_SET_CTYPE (msg.cindex, NSM_ROUTE_CTYPE_MPLS_IPV4_NHOP);

          msg.mpls_flags = ftn->flags;
          msg.mpls_out_protocol = ftn->protocol;
          msg.mpls_oif_ix = nkey->u.ipv4.oif_ix;
          msg.mpls_out_label = nkey->u.ipv4.out_label;
          IPV4_ADDR_COPY (&msg.mpls_nh_addr,
                          &nkey->u.ipv4.nh_addr);
          msg.mpls_ftn_ix = ftn->ftn_ix;
        }
    }
#endif /* HAVE_MPLS */

  /* Nexthop */
  for (nexthop = rib->nexthop; nexthop; nexthop = nexthop->next)
    {
      if (vrf->vrf->id || CHECK_FLAG (nexthop->flags, NEXTHOP_FLAG_FIB) ||
          (VALID_MROUTE_NEXTHOP (nexthop)))
        {
          ifindex = 0;

          if (CHECK_FLAG (nexthop->flags, NEXTHOP_FLAG_RECURSIVE))
            {
              if (nexthop->rtype == NEXTHOP_TYPE_IFINDEX
                  || nexthop->rtype == NEXTHOP_TYPE_IFNAME
#ifdef HAVE_VRF
                  || nexthop->rtype == NEXTHOP_TYPE_MPLS
#endif /* HAVE_VRF */
                  || nexthop->rtype == NEXTHOP_TYPE_IPV4_IFINDEX
                  || nexthop->rtype == NEXTHOP_TYPE_IPV4_IFNAME)
                {
                  ifindex = nexthop->rifindex;

                  ifp = if_lookup_by_index (&vr->ifm, ifindex);

                  /* Check interface binding.  */
                  if (ifp || ! IS_NULL_INTERFACE_STR (nexthop->ifname))
                    if (! nsm_server_if_bind_check (nse, ifp))
                      continue;
                }

              /* Set nexthop address.  */
              NSM_SET_CTYPE (msg.cindex, NSM_ROUTE_CTYPE_IPV4_NEXTHOP);
              msg.nexthop_num++;

              if (nexthop->rtype == NEXTHOP_TYPE_IPV4
                  || nexthop->rtype == NEXTHOP_TYPE_IPV4_IFINDEX
                  || nexthop->rtype == NEXTHOP_TYPE_IPV4_IFNAME)
                msg.nexthop[0].addr = nexthop->rgate.ipv4;
              else
                pal_mem_set (&msg.nexthop[0].addr, 0, 4);

              msg.nexthop[0].ifindex = ifindex;
              advertise = NSM_TRUE;
              break;
            }
          /* Non-recursive nexthop */
          else
            {
              if (nexthop->type == NEXTHOP_TYPE_IFINDEX
                  || nexthop->type == NEXTHOP_TYPE_IFNAME
#ifdef HAVE_VRF
                  || nexthop->type == NEXTHOP_TYPE_MPLS
#endif /* HAVE_VRF */
                  || nexthop->type == NEXTHOP_TYPE_IPV4_IFINDEX
                  || nexthop->type == NEXTHOP_TYPE_IPV4_IFNAME)
                {
                  ifindex = nexthop->ifindex;

                  ifp = if_lookup_by_index (&vr->ifm, ifindex);

                  /* Check interface binding.  */
                  if (ifp || ! IS_NULL_INTERFACE_STR (nexthop->ifname))
                    if (! nsm_server_if_bind_check (nse, ifp))
                      continue;
                }

              /* Set nexthop address.  */
              NSM_SET_CTYPE (msg.cindex, NSM_ROUTE_CTYPE_IPV4_NEXTHOP);
              msg.nexthop_num++;

              if (nexthop->type == NEXTHOP_TYPE_IPV4
                  || nexthop->type == NEXTHOP_TYPE_IPV4_IFINDEX
                  || nexthop->type == NEXTHOP_TYPE_IPV4_IFNAME)
                msg.nexthop[0].addr = nexthop->gate.ipv4;
              else
                pal_mem_set (&msg.nexthop[0].addr, 0, 4);

              msg.nexthop[0].ifindex = ifindex;
              advertise = NSM_TRUE;
              break;
            }
        }
    }
  /* Send the static route tag */
  if (rib->type == APN_ROUTE_STATIC)
    {
      /* Get the node based on the prefix */
      rn = ptree_node_get (vrf->afi[AFI_IP].ip_static, (u_char *) &p->u.prefix4,
			   p->prefixlen);
      if (rn)
        /* Get the respective "nsm_static" structure */
        for (ns = rn->info; ns; ns = ns->next)
          /* Compare the nexthop */
          if (IPV4_ADDR_SAME (&msg.nexthop[0].addr, &ns->gate.ipv4))
            {
              msg.tag = ns->tag;
              NSM_SET_CTYPE (msg.cindex, NSM_ROUTE_CTYPE_IPV4_TAG);
            }
    }

  NSM_SET_CTYPE(msg.cindex,NSM_ROUTE_CTYPE_IPV4_PROCESS_ID);
  msg.pid = rib->pid;

#ifdef HAVE_VRF 
  /* Domain ID */
  if (rib->domain_conf)
    {
      NSM_SET_CTYPE (msg.cindex, NSM_ROUTE_CTYPE_DOMAIN_INFO);
      pal_mem_cpy (msg.domain_info.domain_id,rib->domain_conf->domain_id,
                         sizeof (msg.domain_info.domain_id));
      msg.domain_info.r_type = rib->domain_conf->r_type;
      pal_mem_cpy (&msg.domain_info.area_id, &rib->domain_conf->area_id,
                                         sizeof (struct pal_in4_addr));
      msg.domain_info.metric_type_E2 = rib->domain_conf->metric_type_E2;

      if (rib->domain_conf->router_id.s_addr)
        {
          NSM_SET_CTYPE (msg.cindex, NSM_ROUTE_CTYPE_OSPF_ROUTER_ID);
          pal_mem_cpy (&msg.domain_info.router_id, &rib->domain_conf->router_id,
                                              sizeof (struct pal_in4_addr));
        }
    }
#endif /* HAVE_VRF */

  /* Send message.  */
  if ((advertise == NSM_TRUE) || (override_adv == 1))
    return nsm_server_send_route_ipv4 (vr->id, vrf->vrf->id, nse, msg_id, &msg);
  return 0;
}

/* Send IPv4 add to client.  */
int
nsm_send_ipv4_add (struct nsm_server_entry *nse, u_int32_t msg_id,
                   struct prefix *p, u_int16_t prefixlen,
                   struct rib *rib, struct nsm_vrf *vrf,
                   int bundle, struct ftn_entry *ftn)
{
  int override_adv = 0;

  return nsm_send_ipv4_add_final (nse, msg_id, p, prefixlen, rib, vrf,
                                  bundle, ftn, override_adv);
}

/* Send IPv4 add to client. Override advertisement control.  */
int
nsm_send_ipv4_add_override (struct nsm_server_entry *nse, u_int32_t msg_id,
                            struct prefix *p, u_int16_t prefixlen,
                            struct rib *rib, struct nsm_vrf *vrf,
                            int bundle, struct ftn_entry *ftn)
{
  int override_adv = 1;

  return nsm_send_ipv4_add_final (nse, msg_id, p, prefixlen, rib, vrf,
                                  bundle, ftn, override_adv);
}

/* Send IPv4 delete to client.  */
int
nsm_send_ipv4_delete (struct nsm_server_entry *nse, struct prefix *p,
                      u_int16_t prefixlen, struct rib *rib,
                      struct nsm_vrf *vrf, int bundle)
{
  struct apn_vr *vr = vrf->vrf->vr;
  struct nsm_msg_route_ipv4 msg;
  struct nexthop *nexthop;
  struct interface *ifp;
  u_int32_t ifindex;

  /* Flag is zero for normal IPv4 route deletion.  */
  msg.flags = 0;
  if (nse->nsc->nhtenable == PAL_TRUE
      && CHECK_FLAG (rib->pflags, NSM_ROUTE_CHG_INFORM_BGP))
    {
      if (IS_NSM_DEBUG_EVENT) 
        zlog_info (nse->ns->zg, 
                   "nsm_send_ipv4_delete:: NSM_ROUTE_CHG_INFORM_BGP\n");
      SET_FLAG (msg.flags, NSM_MSG_ROUTE_FLAG_NH_CHANGE);
      UNSET_FLAG (rib->pflags, NSM_ROUTE_CHG_INFORM_BGP);
    }

  msg.type = rib->type;
  msg.sub_type = rib->sub_type;
  msg.distance = rib->distance;
  msg.metric = rib->metric;
  msg.prefixlen = prefixlen ? prefixlen : p->prefixlen;
  msg.prefix = p->u.prefix4;
  msg.cindex = 0;
  msg.nexthop_num = 0;

  /* Nexthop */
  for (nexthop = rib->nexthop; nexthop; nexthop = nexthop->next)
    {
      if (vrf->vrf->id || CHECK_FLAG (nexthop->flags, NEXTHOP_FLAG_FIB) ||
          (VALID_MROUTE_NEXTHOP (nexthop)))
        {
          if (CHECK_FLAG (nexthop->flags, NEXTHOP_FLAG_RECURSIVE))
            ifindex = nexthop->rifindex;
          else
            ifindex = nexthop->ifindex;

          ifp = if_lookup_by_index (&vr->ifm, ifindex);

          /* Check interface binding.  */
          if (! nsm_server_if_bind_check (nse, ifp))
            continue;

          /* Set nexthop address.  */
          NSM_SET_CTYPE (msg.cindex, NSM_ROUTE_CTYPE_IPV4_NEXTHOP);
          msg.nexthop_num = 1;

          if (nexthop->type == NEXTHOP_TYPE_IPV4
              || nexthop->type == NEXTHOP_TYPE_IPV4_IFINDEX
              || nexthop->type == NEXTHOP_TYPE_IPV4_IFNAME)
            msg.nexthop[0].addr = nexthop->gate.ipv4;
          else
            pal_mem_set (&msg.nexthop[0].addr, 0, 4);

          msg.nexthop[0].ifindex = ifindex;
          break;
        }
    }

#ifdef HAVE_VRF 
  if (rib->domain_conf)
    {
      NSM_SET_CTYPE (msg.cindex, NSM_ROUTE_CTYPE_DOMAIN_INFO);
      pal_mem_cpy (msg.domain_info.domain_id, rib->domain_conf->domain_id,
                                 sizeof (msg.domain_info.domain_id));
      msg.domain_info.r_type = rib->domain_conf->r_type;
      pal_mem_cpy (&msg.domain_info.area_id, &rib->domain_conf->area_id,
                                 sizeof (struct pal_in4_addr));
      msg.domain_info.metric_type_E2 = rib->domain_conf->metric_type_E2;

      if (rib->domain_conf->router_id.s_addr)
       {
         NSM_SET_CTYPE (msg.cindex, NSM_ROUTE_CTYPE_OSPF_ROUTER_ID);
         pal_mem_cpy (&msg.domain_info.router_id, &rib->domain_conf->router_id,
                                    sizeof (struct pal_in4_addr));
       }
    }
#endif /* HAVE_VRF */

  /* Send message.  */
  return nsm_server_send_route_ipv4 (vr->id, vrf->vrf->id, nse, 0, &msg);
}

#ifdef HAVE_IPV6
int
nsm_send_ipv6_add_final (struct nsm_server_entry *nse, u_int32_t msg_id,
                         struct prefix *p, u_int16_t prefixlen,
                         struct rib *rib, struct nsm_vrf *vrf, int bundle,
                         struct ftn_entry *ftn, int override_adv)
{
  struct apn_vr *vr = vrf->vrf->vr;
  struct nsm_msg_route_ipv6 msg;
  struct nexthop *nexthop;
  struct interface *ifp;
  u_int32_t ifindex;
  bool_t advertise = NSM_FALSE;

  msg.flags = 0;
  SET_FLAG (msg.flags, NSM_MSG_ROUTE_FLAG_ADD);
  if (nse->nsc->nhtenable == PAL_TRUE
      && CHECK_FLAG(rib->pflags, NSM_ROUTE_CHG_INFORM_BGP))
    {
      if (IS_NSM_DEBUG_EVENT) 
        zlog_info (nse->ns->zg, 
                   "nsm_send_ipv6_add_final:: NSM_ROUTE_CHG_INFORM_BGP\n");
      SET_FLAG (msg.flags, NSM_MSG_ROUTE_FLAG_NH_CHANGE);
      UNSET_FLAG (rib->pflags, NSM_ROUTE_CHG_INFORM_BGP);
    }

  msg.type = rib->type;
  msg.sub_type = rib->sub_type;
  msg.distance = rib->distance;
  msg.metric = rib->metric;
  msg.prefixlen =  prefixlen ? prefixlen : p->prefixlen;
  msg.prefix = p->u.prefix6;
  msg.cindex = 0;
  msg.nexthop_num = 0;

  /* Nexthop */
  for (nexthop = rib->nexthop; nexthop; nexthop = nexthop->next)
    {
      if (vrf->vrf->id || CHECK_FLAG (nexthop->flags, NEXTHOP_FLAG_FIB) ||
          (VALID_MROUTE_NEXTHOP (nexthop)))
        {
          ifindex = 0;

          if (CHECK_FLAG (nexthop->flags, NEXTHOP_FLAG_RECURSIVE))
            {
              if (nexthop->rtype == NEXTHOP_TYPE_IFINDEX
                  || nexthop->rtype == NEXTHOP_TYPE_IFNAME
#ifdef HAVE_VRF
                  || nexthop->rtype == NEXTHOP_TYPE_MPLS
#endif /* HAVE_VRF */
                  || nexthop->rtype == NEXTHOP_TYPE_IPV6_IFINDEX
                  || nexthop->rtype == NEXTHOP_TYPE_IPV6_IFNAME)
                {
                  ifindex = nexthop->rifindex;

                  ifp = if_lookup_by_index (&vr->ifm, ifindex);

                  if (ifp 
                      || (!IS_NULL_INTERFACE_STR (nexthop->ifname)))
                    if (! nsm_server_if_bind_check (nse, ifp))
                      continue;
                }

              /* Set nexthop address.  */
              NSM_SET_CTYPE (msg.cindex, NSM_ROUTE_CTYPE_IPV6_NEXTHOP);
              msg.nexthop_num = 1;

              if (nexthop->rtype == NEXTHOP_TYPE_IPV6
                  || nexthop->rtype == NEXTHOP_TYPE_IPV6_IFINDEX
                  || nexthop->rtype == NEXTHOP_TYPE_IPV6_IFNAME)
                msg.nexthop[0].addr = nexthop->rgate.ipv6;
              else
                pal_mem_set (&msg.nexthop[0].addr, 0, 16);

              msg.nexthop[0].ifindex = ifindex;
              advertise = NSM_TRUE;
              break;
            }
          /* Non-recursive nexthop */
          else
            {
              if (nexthop->type == NEXTHOP_TYPE_IFINDEX
                  || nexthop->type == NEXTHOP_TYPE_IFNAME
#ifdef HAVE_VRF
                  || nexthop->type == NEXTHOP_TYPE_MPLS
#endif /* HAVE_VRF */
                  || nexthop->type == NEXTHOP_TYPE_IPV6_IFINDEX
                  || nexthop->type == NEXTHOP_TYPE_IPV6_IFNAME)
                {
                  ifindex = nexthop->ifindex;

                  ifp = if_lookup_by_index (&vr->ifm, ifindex);

                  if (ifp 
                      || (!IS_NULL_INTERFACE_STR (nexthop->ifname)))
                    if (! nsm_server_if_bind_check (nse, ifp))
                      continue;
                }

              /* Set nexthop address.  */
              NSM_SET_CTYPE (msg.cindex, NSM_ROUTE_CTYPE_IPV6_NEXTHOP);
              msg.nexthop_num = 1;

              if (nexthop->type == NEXTHOP_TYPE_IPV6
                  || nexthop->type == NEXTHOP_TYPE_IPV6_IFINDEX
                  || nexthop->type == NEXTHOP_TYPE_IPV6_IFNAME)
                msg.nexthop[0].addr = nexthop->gate.ipv6;
              else
                pal_mem_set (&msg.nexthop[0].addr, 0, 16);

              msg.nexthop[0].ifindex = ifindex;
              advertise = NSM_TRUE;
              break;

            }
        }
    }

  /* Sending the instance ID from which instance the 
     redistribution is going on */
  NSM_SET_CTYPE(msg.cindex,NSM_ROUTE_CTYPE_IPV6_PROCESS_ID);
  msg.pid = rib->pid;

  /* Send message.  */
  if ((advertise == NSM_TRUE) || (override_adv == 1))
    return nsm_server_send_route_ipv6 (vr->id, vrf->vrf->id, nse, msg_id, &msg);
  return 0;
}


/* Send IPv6 add to client.  */
int
nsm_send_ipv6_add (struct nsm_server_entry *nse, u_int32_t msg_id,
                   struct prefix *p, u_int16_t prefixlen,
                   struct rib *rib, struct nsm_vrf *vrf,
                   int bundle, struct ftn_entry *ftn)
{
  int override_adv = 0;

  return nsm_send_ipv6_add_final (nse, msg_id, p, prefixlen, rib, vrf,
                                  bundle, ftn, override_adv);
}

/* Send IPv6 add to client. Override advertisement control.  */
int
nsm_send_ipv6_add_override (struct nsm_server_entry *nse, u_int32_t msg_id,
                            struct prefix *p, u_int16_t prefixlen,
                            struct rib *rib, struct nsm_vrf *vrf,
                            int bundle, struct ftn_entry *ftn)
{
  int override_adv = 1;

  return nsm_send_ipv6_add_final (nse, msg_id, p, prefixlen, rib, vrf,
                                  bundle, ftn, override_adv);
}

int
nsm_send_ipv6_delete (struct nsm_server_entry *nse, struct prefix *p,
                      u_int16_t prefixlen, struct rib *rib,
                      struct nsm_vrf *vrf, int bundle)
{
  struct apn_vr *vr = vrf->vrf->vr;
  struct nsm_msg_route_ipv6 msg;
  struct nexthop *nexthop;
  struct interface *ifp;
  u_int32_t ifindex;

  msg.flags = 0;
  if (nse->nsc->nhtenable == PAL_TRUE
      && CHECK_FLAG(rib->pflags, NSM_ROUTE_CHG_INFORM_BGP))
    {
      if (IS_NSM_DEBUG_EVENT) 
        zlog_info (nse->ns->zg, 
                   "nsm_send_ipv6_delete:: NSM_ROUTE_CHG_INFORM_BGP\n");
      SET_FLAG (msg.flags, NSM_MSG_ROUTE_FLAG_NH_CHANGE);
      UNSET_FLAG (rib->pflags, NSM_ROUTE_CHG_INFORM_BGP);
    }

  msg.type = rib->type;
  msg.sub_type = rib->sub_type;
  msg.distance = rib->distance;
  msg.metric = rib->metric;
  msg.prefixlen = prefixlen ? prefixlen : p->prefixlen;
  msg.prefix = p->u.prefix6;
  msg.cindex = 0;
  msg.nexthop_num = 0;

  /* Nexthop */
  for (nexthop = rib->nexthop; nexthop; nexthop = nexthop->next)
    {
      if (vrf->vrf->id || CHECK_FLAG (nexthop->flags, NEXTHOP_FLAG_FIB) ||
          (VALID_MROUTE_NEXTHOP (nexthop)))
        {
          if (CHECK_FLAG (nexthop->flags, NEXTHOP_FLAG_RECURSIVE))
            ifindex = nexthop->rifindex;
          else
            ifindex = nexthop->ifindex;

          ifp = if_lookup_by_index (&vr->ifm, ifindex);

          /* Check interface binding.  */
          if (! nsm_server_if_bind_check (nse, ifp))
            continue;

          /* Set nexthop address.  */
          NSM_SET_CTYPE (msg.cindex, NSM_ROUTE_CTYPE_IPV6_NEXTHOP);
          msg.nexthop_num = 1;

          if (nexthop->type == NEXTHOP_TYPE_IPV6
              || nexthop->type == NEXTHOP_TYPE_IPV6_IFINDEX
              || nexthop->type == NEXTHOP_TYPE_IPV6_IFNAME)
            msg.nexthop[0].addr = nexthop->gate.ipv6;
          else
            pal_mem_set (&msg.nexthop[0].addr, 0, 16);

          msg.nexthop[0].ifindex = ifindex;
          break;
        }
    }

  /* Send message.  */
  return nsm_server_send_route_ipv6 (vr->id, vrf->vrf->id, nse, 0, &msg);
}
#endif /* HAVE_IPV6 */
/* Receive IPv4 routing updates.  */
int
nsm_server_recv_route_ipv4 (struct nsm_msg_header *header,
                            void *arg, void *message)
{
  struct nsm_master *nm;
  struct nsm_server_entry *nse;
  struct nsm_server *ns;
  struct nsm_msg_route_ipv4 *msg;
  struct prefix p;
  struct rib *rib;
  struct nsm_vrf *vrf;
  struct nsm_tlv_ipv4_nexthop *nexthop;
  int i;

  /* Set connection and message.  */
  nse = arg;
  ns = nse->ns;
  msg = message;

  /* Debug.  */
  if (IS_NSM_DEBUG_RECV)
    {
      zlog_info (nse->ns->zg, "NSM RECV IPv4 route");
      nsm_route_ipv4_dump (ns->zg, msg);
    }

  /* Fill in prefix.  */
  pal_mem_set (&p, 0, sizeof (struct prefix));
  p.family = AF_INET;
  p.prefixlen = msg->prefixlen;
  p.u.prefix4 = msg->prefix;

  /* Get VR context. */
  nm = nsm_master_lookup_by_id (nzg, header->vr_id);
  if (nm == NULL)
    return 0;

  /* Lookup table.  */
  vrf = nsm_vrf_lookup_by_id (nm, header->vrf_id);
  if (vrf == NULL)
    return 0;

  /* If this is delete.  */
  if (! CHECK_FLAG (msg->flags, NSM_MSG_ROUTE_FLAG_ADD))
    {
      nsm_rib_delete_by_type (&p, msg->type, msg->pid, AFI_IP, SAFI_UNICAST, vrf);

#ifdef HAVE_VRX /* XXX */
      /* Export route to other virtual-routers. */
      nsm_vr_export_route_ipv4 (0, msg->type, (struct prefix_ipv4 *) &p,
                                msg->nexthop[0].ifindex,  NULL, 0, 0,
                                header->vrf_id);
#endif /* HAVE_VRX */

#ifdef HAVE_RMM
      /* Send message to backup before returning.                           *
       * If RMM is doing primary -> backup transition, don't delete routes. *
       * If state is not primary, don't send the message.                   */
      if (ng->rmm->primary2backup_flag != PAL_TRUE &&
          ng->rmm->nsm_state == RMM_STATE_PRIMARY)
        {
          /* Build rib structure to send. */
          rib = XCALLOC (MTYPE_NSM_RIB, sizeof (struct rib));
          if (rib == NULL)
            {
              if (IS_NSM_DEBUG_EVENT)
                zlog_info (NSM_ZG, "RIB create failed");
              return NSM_FAILURE;
            }
          rib->type = msg->type;
          if (msg->nexthop[0].ifindex)
            nsm_nexthop_ifindex_add (rib, msg->nexthop[0].ifindex, 
                                     ROUTE_TYPE_OTHER);

          nsm_rmm_rpu_sync_ipv4 (&p, rib, header->vrf_id,
                                 NSM_RMM_RIB_CMD_IPV4_DELETE);
          nsm_rib_free (rib);
        }
#endif /* HAVE_RMM */

      return 0;
    }

  /* XXX: IMPORTANT
   * Do not call create or modify CAL checkpoint actions
   * here as nsm_rib_add() takes care of this
   */
  /* Fill in RIB structure.  */
  rib = XCALLOC (MTYPE_NSM_RIB, sizeof (struct rib)); 
  if (rib == NULL)
    {
       if (IS_NSM_DEBUG_EVENT)
         zlog_info (NSM_ZG, "RIB create failed");
       return NSM_FAILURE;
    }

  rib->type = msg->type;
  rib->pid = msg->pid;

#ifdef HAVE_VRF
  if (NSM_CHECK_CTYPE (msg->cindex, NSM_ROUTE_CTYPE_DOMAIN_INFO))
    {
      rib->domain_conf = XCALLOC (MTYPE_NSM_RIB,
                           sizeof(struct nsm_ospf_domain_conf));
      if (rib->domain_conf == NULL)
        {
          XFREE (MTYPE_NSM_RIB, rib);
          if (IS_NSM_DEBUG_EVENT)
            zlog_info (NSM_ZG, "NSM RIB domain info create failed");
          return NSM_FAILURE;
        }
      pal_mem_cpy (rib->domain_conf->domain_id, msg->domain_info.domain_id,
                                  sizeof(rib->domain_conf->domain_id));
      pal_mem_cpy (&rib->domain_conf->area_id, &msg->domain_info.area_id,
                                 sizeof (struct pal_in4_addr));
      rib->domain_conf->r_type = msg->domain_info.r_type;
      rib->domain_conf->metric_type_E2 = msg->domain_info.metric_type_E2;

      if (NSM_CHECK_CTYPE (msg->cindex, NSM_ROUTE_CTYPE_OSPF_ROUTER_ID))
        pal_mem_cpy (&rib->domain_conf->router_id, &msg->domain_info.router_id,
                                    sizeof (struct pal_in4_addr));
    }
  else
    rib->domain_conf = NULL;
#endif /* HAVE_VRF */

  rib->sub_type = msg->sub_type;
  rib->flags = 0;
  if (CHECK_FLAG (msg->flags, NSM_MSG_ROUTE_FLAG_BLACKHOLE))
    SET_FLAG (rib->flags, RIB_FLAG_BLACKHOLE);
  if (CHECK_FLAG (msg->flags, NSM_MSG_ROUTE_FLAG_RESOLVE))
    SET_FLAG (rib->flags, RIB_FLAG_INTERNAL);
#ifdef HAVE_CRX
  else
    {
      SET_FLAG (rib->flags, RIB_FLAG_INTERNAL);
    }
#endif /* HAVE_CRX */
  rib->distance = msg->distance;
  rib->metric = msg->metric;
  rib->uptime = pal_time_current (NULL);
  rib->client_id = nse->service.client_id;
  rib->vrf = vrf;

  if (NSM_CHECK_CTYPE (msg->cindex, NSM_ROUTE_CTYPE_IPV4_TAG))
    rib->tag = msg->tag;
  else
    rib->tag = 0;

  /* NSM master multipath number over-rides received value */
  if (msg->nexthop_num > nm->multipath_num)
    msg->nexthop_num = nm->multipath_num;

  /* If next hop count is larger than default number, refer to
     optional memory pointer.  */
  if (msg->nexthop_num > NSM_TLV_IPV4_NEXTHOP_NUM)
    nexthop = msg->nexthop_opt;
  else
    nexthop = msg->nexthop;

  /* Fetch and convert nexthop information to RIB format.  */
  for (i = 0; i < msg->nexthop_num; i++)
    {
      if (CHECK_FLAG (rib->flags, RIB_FLAG_BLACKHOLE))
        nsm_nexthop_ifname_add (rib, NULL_INTERFACE, ROUTE_TYPE_OTHER);
      else if (nexthop[i].ifindex)
        {
          if (nexthop[i].addr.s_addr != INADDR_ANY)
            nsm_nexthop_ipv4_ifindex_add (rib, &nexthop[i].addr,
                                          nexthop[i].ifindex,
                                          ROUTE_TYPE_OTHER);
          else
            nsm_nexthop_ifindex_add (rib, nexthop[i].ifindex,
                                     ROUTE_TYPE_OTHER);
        }
      else
        nsm_nexthop_ipv4_add (rib, &nexthop[i].addr, ROUTE_TYPE_OTHER);
    }

  /* Add RIB to tree.  */
  nsm_rib_add (&p, rib, AFI_IP, SAFI_UNICAST);

#ifdef HAVE_VRX
  /* Export route add to other virtual-routers. */
  nsm_vr_export_route_ipv4 (1, rib->type, (struct prefix_ipv4 *) &p, 
                            0, NULL, rib->flags, rib->metric, header->vrf_id);
#endif /* HAVE_VRX */

#ifdef HAVE_RMM
  /* If RMM is doing primary -> backup transition, don't delete routes. *
   * If state is not primary, don't send the route.                     */
  if (ng->rmm->primary2backup_flag != PAL_TRUE &&
      ng->rmm->nsm_state == RMM_STATE_PRIMARY)
    nsm_rmm_rpu_sync_ipv4 (&p, rib, header->vrf_id, NSM_RMM_RIB_CMD_IPV4_ADD);
#endif /* HAVE_RMM */

  /* Free nexthop information in message when it is needed.  */
  nsm_finish_route_ipv4 (msg);

  return 0;
}

#ifdef HAVE_IPV6
/* Receive service request.  */
int
nsm_server_recv_route_ipv6 (struct nsm_msg_header *header,
                            void *arg, void *message)
{
  struct nsm_master *nm;
  struct nsm_server_entry *nse;
  struct nsm_server *ns;
  struct nsm_msg_route_ipv6 *msg;
  struct prefix p;
  struct rib *rib;
  struct nsm_vrf *vrf;
  struct nsm_tlv_ipv6_nexthop *nexthop;
  int ii;
#if defined HAVE_6PE || defined HAVE_VRF
  u_int32_t ipaddr_nw;
#endif /* HAVE_6PE  || HAVE_VRF */

  /* Set connection and message.  */
  nse = arg;
  ns = nse->ns;
  msg = message;

  /* Debug.  */
  if (IS_NSM_DEBUG_RECV)
    nsm_route_ipv6_dump (ns->zg, msg);

  /* Fill in prefix.  */
  pal_mem_set (&p, 0, sizeof (struct prefix));
  p.family = AF_INET6;
  p.prefixlen = msg->prefixlen;
  p.u.prefix6 = msg->prefix;
  
  /* Get VR context. */
  nm = nsm_master_lookup_by_id (nzg, header->vr_id);
  if (nm == NULL)
    return 0;

  /* Lookup table.  */
  vrf = nsm_vrf_lookup_by_id (nm, header->vrf_id);
  if (vrf == NULL)
    return 0;
  
  /* If this is delete.  */
  if (! CHECK_FLAG (msg->flags, NSM_MSG_ROUTE_FLAG_ADD))
    {
      nsm_rib_delete_by_type (&p, msg->type, msg->pid, AFI_IP6, SAFI_UNICAST, vrf);
#ifdef HAVE_RMM
      /* Send message to backup before returning.                           *
       * If RMM is doing primary -> backup transition, don't delete routes. *
       * If state is not primary, don't send the message.                   */
      if (ng->rmm->primary2backup_flag != PAL_TRUE &&
          ng->rmm->nsm_state == RMM_STATE_PRIMARY)
        {
          /* Build rib structure to send. */
          rib = XCALLOC (MTYPE_NSM_RIB, sizeof (struct rib)); 
          if (rib == NULL)
            {
              if (IS_NSM_DEBUG_EVENT)
                zlog_info (NSM_ZG, "RIB create failed");
              return NSM_FAILURE;
            }

          rib->type = msg->type;
          if (msg->nexthop[0].ifindex)
            nsm_nexthop_ifindex_add (rib, msg->nexthop[0].ifindex, 
                                     ROUTE_TYPE_OTHER);

          nsm_rmm_rpu_sync_ipv6 (&p, rib, header->vrf_id,
                                 NSM_RMM_RIB_CMD_IPV6_DELETE);
          nsm_rib_free (rib);
        }
#endif /* HAVE_RMM */

      return 0;
    }

  /* XXX: IMPORTANT
   * Do not call create or modify CAL checkpoint actions
   * here as nsm_rib_add() takes care of this
   */
  /* Fill in RIB structure.  */
  rib = XCALLOC (MTYPE_NSM_RIB, sizeof (struct rib)); 
  if (rib == NULL)
    {
      if (IS_NSM_DEBUG_EVENT)
          zlog_info (NSM_ZG, "RIB create failed");
      return NSM_FAILURE;
    }

  rib->type = msg->type;
  
  if (NSM_CHECK_CTYPE (msg->cindex, NSM_ROUTE_CTYPE_IPV6_PROCESS_ID))
    rib->pid = msg->pid;
  else
    rib->pid = 0; 
 
  rib->sub_type = msg->sub_type;
  rib->flags = 0;
  if (CHECK_FLAG (msg->flags, NSM_MSG_ROUTE_FLAG_BLACKHOLE))
    SET_FLAG (rib->flags, RIB_FLAG_BLACKHOLE);
  if (CHECK_FLAG (msg->flags, NSM_MSG_ROUTE_FLAG_RESOLVE))
    SET_FLAG (rib->flags, RIB_FLAG_INTERNAL);
    
#ifdef HAVE_CRX
  else
    {
      SET_FLAG (rib->flags, RIB_FLAG_INTERNAL);
    }
#endif /* HAVE_CRX */
  rib->distance = msg->distance;
  rib->metric = msg->metric;
  rib->uptime = pal_time_current (NULL);
  rib->client_id = nse->service.client_id;
  rib->vrf = vrf;

  /* If next hop count is larger than default number, refer to
     optional memory pointer.  */
  if (msg->nexthop_num > NSM_TLV_IPV6_NEXTHOP_NUM)
    nexthop = msg->nexthop_opt;
  else
    nexthop = msg->nexthop;

  /* Fetch and convert nexthop information to RIB format.  */
  
  for (ii = 0; ii < msg->nexthop_num; ii++)
    {
      if (nexthop[ii].ifindex)
        {
#if defined(HAVE_6PE) || defined(HAVE_VRF)
          if (CHECK_FLAG (msg->flags, NSM_MSG_ROUTE_FLAG_IPV4NEXTHOP) &&
              nexthop[ii].addr.s6_addr[3] != INADDR_ANY)
            { 
              UNSET_FLAG (msg->flags, NSM_MSG_ROUTE_FLAG_IPV4NEXTHOP);
              ipaddr_nw = nexthop[ii].addr.s6_addr32[3];
              nsm_nexthop_ipv4_ifindex_add (rib, (struct pal_in4_addr *)
                                            &ipaddr_nw,
                                            nexthop[ii].ifindex,
                                            ROUTE_TYPE_OTHER);
            }
          else 
#endif /* HAVE_6PE || HAVE_VRF*/
            if (! IN6_IS_ADDR_UNSPECIFIED (&nexthop[ii].addr))
              nsm_nexthop_ipv6_ifindex_add (rib, &nexthop[ii].addr,
                                            nexthop[ii].ifindex);
            else
              nsm_nexthop_ifindex_add (rib, nexthop[ii].ifindex,
                                       ROUTE_TYPE_OTHER);
        }
      else
        {
#if defined(HAVE_6PE) || defined(HAVE_VRF)
         if (CHECK_FLAG (msg->flags, NSM_MSG_ROUTE_FLAG_IPV4NEXTHOP))
           {
             UNSET_FLAG (msg->flags, NSM_MSG_ROUTE_FLAG_IPV4NEXTHOP);
             ipaddr_nw = nexthop[ii].addr.s6_addr32 [3];
             nsm_nexthop_ipv4_add (rib, (struct pal_in4_addr *)
                                   &ipaddr_nw, ROUTE_TYPE_OTHER);
            }
          else
#endif /* HAVE_6PE || HAVE_VRF*/
            nsm_nexthop_ipv6_add (rib, &nexthop[ii].addr);
        }
    }

  /* Add RIB to tree.  */
  nsm_rib_add (&p, rib, AFI_IP6, SAFI_UNICAST);

#ifdef HAVE_RMM
  /* If RMM is doing primary -> backup transition, don't delete routes. *
   * If state is not primary, don't send the route.                     */
  if (ng->rmm->primary2backup_flag != PAL_TRUE &&
      ng->rmm->nsm_state == RMM_STATE_PRIMARY)
    nsm_rmm_rpu_sync_ipv6 (&p, rib, header->vrf_id, NSM_RMM_RIB_CMD_IPV6_ADD);
#endif /* HAVE_RMM */

  /* Free nexthop information in message when it is needed.  */
  nsm_finish_route_ipv6 (msg);
 
  return 0;
}
#endif /* HAVE_IPV6 */

static u_int32_t
nsm_set_nh_lookup_exclude_protocol (u_int32_t protocol, u_char sub_type, 
                                    u_int32_t *proto, u_char *sub_proto)
{
  /* Exclude EBGP routes for IBGP. */
  if (protocol == APN_PROTO_BGP && (sub_type == APN_ROUTE_BGP_IBGP))
    {
      *proto = (1 << APN_ROUTE_BGP);
      *sub_proto = APN_ROUTE_BGP_EBGP; 
    }
  else
    {
      *proto = 0;
      *sub_proto = 0;
    }

  /* For other protocols don't exclude IGP as well as EGP routes. */

  return *proto;
}

/* Receive IPv4 exact match lookup.  */
int
nsm_server_recv_ipv4_route_exact_lookup (struct nsm_msg_header *header,
                                         void *arg, void *message)
{
  struct nsm_msg_route_lookup_ipv4 *msg = message;
  struct nsm_server_entry *nse = arg;
  u_int32_t exclude_proto;
  struct nsm_master *nm;
  struct nsm_server *ns = nse->ns;
  struct nsm_vrf *vrf;
  struct ftn_entry *ftn = NULL;
  struct rib *rib;
  struct prefix p;
  u_char sub_type;
#ifdef HAVE_MPLS
  struct fec_gen_entry gen_entry;
#endif /*HAVE_MPLS*/

  if (IS_NSM_DEBUG_RECV)
    nsm_route_lookup_ipv4_dump (ns->zg, msg);

  /* Set the protocol routes to exclude. */
  nsm_set_nh_lookup_exclude_protocol (nse->service.protocol_id, 0,
                                      &exclude_proto, &sub_type);

  /* Get VR context. */
  nm = nsm_master_lookup_by_id (nzg, header->vr_id);
  if (nm == NULL)
    return 0;

  vrf = nsm_vrf_lookup_by_id (nm, header->vrf_id);
  if (vrf == NULL)
    return 0;

  /* Set prefix. */
  p.family = AF_INET;
  if (NSM_CHECK_CTYPE (msg->cindex, NSM_ROUTE_LOOKUP_CTYPE_PREFIXLEN))
    p.prefixlen = msg->prefixlen;
  else
    p.prefixlen = IPV4_MAX_PREFIXLEN;
  p.u.prefix4 = msg->addr;
#ifdef HAVE_MPLS
  NSM_MPLS_PREFIX_TO_FEC_GEN_ENTRY(&gen_entry, &p);
#endif /*HAVE_MPLS*/

  /* Lookup RIB by the prefix. */
  if (APN_PROTO_MCAST (nse->service.protocol_id))
    rib = nsm_mrib_lookup (vrf, AFI_IP, &p, NULL, exclude_proto);
  else
    rib = nsm_rib_lookup (vrf, AFI_IP, &p, NULL, exclude_proto, sub_type);

#ifdef HAVE_MPLS
  IF_NSM_CAP_HAVE_MPLS
    {
      /* MPLS lookup. */
      if (NSM_CHECK_CTYPE (msg->cindex, NSM_ROUTE_LOOKUP_CTYPE_LSP))
        ftn = nsm_gmpls_get_mapped_ftn (nm, &gen_entry);

      if (NSM_CHECK_CTYPE (msg->cindex, NSM_ROUTE_LOOKUP_CTYPE_LSP_PROTO))
        ftn = nsm_mpls_get_ftn_by_proto (nm, &p, nse->service.protocol_id);
    }
#endif /* HAVE_MPLS */

  /* Send it to client. */
  if (rib == NULL)
    rib = &RibNull;

  nsm_send_ipv4_add_override (nse, header->message_id, &p,
                              0, rib, vrf, 0, ftn);

  return 0;
}

/* Receive IPv4 exact best lookup.  */
int
nsm_server_recv_ipv4_route_best_lookup (struct nsm_msg_header *header,
                                        void *arg, void *message)
{
  struct nsm_master *nm;
  struct nsm_server_entry *nse = arg;
  struct nsm_server *ns = nse->ns;
  struct nsm_msg_route_lookup_ipv4 *msg = message;
  struct nsm_ptree_node *rn = NULL;
  struct nsm_vrf *vrf;
  struct rib *rib;
  struct ftn_entry *ftn = NULL;
  struct prefix p;
  struct prefix addr;
  u_int32_t exclude_proto;
  u_char sub_type;
#ifdef HAVE_MPLS
  struct fec_gen_entry gen_entry;
#endif /*HAVE_MPLS*/

  if (IS_NSM_DEBUG_RECV)
    nsm_route_lookup_ipv4_dump (ns->zg, msg);

  /* Set the protocol routes to exclude. */
  nsm_set_nh_lookup_exclude_protocol (nse->service.protocol_id, 0,
                                      &exclude_proto, &sub_type);

  /* Get VR context. */
  nm = nsm_master_lookup_by_id (nzg, header->vr_id);
  if (nm == NULL)
    return 0;

  vrf = nsm_vrf_lookup_by_id (nm, header->vrf_id);
  if (vrf == NULL)
    return 0;

  pal_mem_set (&p, 0, sizeof (struct prefix));
  p.family = AF_INET;
  p.prefixlen = IPV4_MAX_PREFIXLEN;
  p.u.prefix4 = msg->addr;
#ifdef HAVE_MPLS
  NSM_MPLS_PREFIX_TO_FEC_GEN_ENTRY(&gen_entry, &p);
#endif /*HAVE_MPLS*/

  if (APN_PROTO_MCAST (nse->service.protocol_id))
    rib = nsm_mrib_match (vrf, AFI_IP, &p, &rn, exclude_proto);
  else
    rib = nsm_rib_match (vrf, AFI_IP, &p, &rn, exclude_proto, sub_type);

#ifdef HAVE_MPLS
  IF_NSM_CAP_HAVE_MPLS
    {
      /* MPLS Lookup. */
      if (NSM_CHECK_CTYPE (msg->cindex, NSM_ROUTE_LOOKUP_CTYPE_LSP))
        ftn = nsm_gmpls_get_mapped_ftn (nm, &gen_entry);
    }
#endif /* HAVE_MPLS */
 
  /* Send it to client. */
  if (rib == NULL)
    rib = &RibNull;
  
  if (rn)
    {
      pal_mem_set(&addr, 0, sizeof (struct prefix));
      addr.family = rn->tree->ptype;
      addr.prefixlen = rn->key_len;
      NSM_PTREE_KEY_COPY (&addr, rn);
      nsm_send_ipv4_add_override (nse, header->message_id, &addr,
                                  rn->key_len, rib, vrf, 0, ftn);
    }
  else
    nsm_send_ipv4_add_override (nse, header->message_id, &p,
                                0, rib, vrf, 0, ftn);

  return 0;
}

#define NSM_RIB_NH_LIST_SET(N)                                                \
    do {                                                                      \
      (N)->nh_list = list_new ();                                             \
    } while (0)

#define NSM_REG_NH_LIST_SET(N)                                                \
    do {                                                                      \
      (N)->info = (N);                                                        \
      if ((N)->nh_list == NULL)                                               \
        (N)->nh_list = list_new ();                                           \
    } while (0)

#define NSM_REG_NH_LIST_UNSET(N)                                              \
    do {                                                                      \
      list_free ((N)->nh_list);                                               \
      (N)->info = NULL;                                                       \
      nsm_ptree_unlock_node ((N));                                            \
    } while (0)

#define NSM_RIB_NH_LIST_UNSET(N)                                              \
    do {                                                                      \
      list_free ((N)->nh_list);                                               \
      (N)->nh_list = NULL;                                                    \
    } while (0)

int
nsm_server_nh_reg_client_add (struct nsm_ptree_node *rn,
                              struct nsm_server_client *nsc,
                              struct prefix *p, u_char type)
{
  struct nexthop_lookup_reg *reg;
  struct listnode *node;

  for (node = LISTHEAD (rn->nh_list); node; NEXTNODE (node))
    {
      reg = GETDATA (node);
      if (!reg)
        continue;

      if (prefix_same (&reg->p, (struct prefix *)&p))
        if (reg->lookup_type == type)
          {
            listnode_add (&reg->client_list, nsc);
            return 1;
          }
    }
  return 0;
}

void
nsm_server_nh_reg_client_init (struct nsm_ptree_node *rn,
                               struct nsm_server_client *nsc,
                               struct prefix *p, u_char type)
{
  struct nexthop_lookup_reg *reg;

  reg = XCALLOC (MTYPE_NSM_NEXTHOP_LOOKUP_REG, 
                 sizeof (struct nexthop_lookup_reg));

  pal_mem_cpy (&reg->p, p, sizeof (struct prefix));
  reg->lookup_type = type;
  listnode_add (&reg->client_list, nsc);
  listnode_add (rn->nh_list, reg);
}

void
nsm_server_nh_reg_client_unset (struct nexthop_lookup_reg *nh_reg,
                                struct nsm_server_client *nsc_target,
                                struct prefix *p, u_char type)
{
  struct nsm_server_client *nsc;
  struct listnode *node, *next;

  /* Check for nsc in client list. */
  for (node = LISTHEAD (&nh_reg->client_list); node; node = next)
    {
      nsc = GETDATA (node);

      next = node->next;

      if (!nsc)
        continue;
              
      if (nsc == nsc_target
          && nh_reg->lookup_type == type
          && prefix_same (&nh_reg->p, p))
        list_delete_node (&nh_reg->client_list, node);
    }
}

void
nsm_server_nh_reg_client_unset_all (struct nsm_ptree_node *rn,
                                    struct nsm_server_client *nsc,
                                    struct prefix *p, u_char type)
{
  struct nexthop_lookup_reg *reg;
  struct listnode *node, *next;

  if (rn->nh_list)
    {
      for (node = LISTHEAD (rn->nh_list); node; node = next)
        {
          reg = GETDATA (node);
          next = node->next;
          
          /* Check for nsc in client list. */
          nsm_server_nh_reg_client_unset (reg, nsc, p, type);
          
          /* If all clients are freed, free nh_reg structure. */
          if (LIST_ISEMPTY (&reg->client_list))
            {
              XFREE (MTYPE_NSM_NEXTHOP_LOOKUP_REG, reg);
              list_delete_node (rn->nh_list, node);
            }
        }
    }
}

int
nsm_server_route_exact_lookup_reg (struct nsm_vrf *vrf, afi_t afi,
                                   struct nsm_server_entry *nse,
                                   struct rib *rib, struct prefix *p,
                                   u_int32_t msg_id)
{
  struct nsm_server_client *nsc = nse->nsc;
  struct nsm_ptree_node *rn = NULL;

  /* Lookup route in RIB. */
  if (afi == AFI_IP)
    rn = nsm_ptree_node_lookup (vrf->afi[afi].rib[SAFI_UNICAST], 
                                (u_char *) &p->u.prefix4,
                                p->prefixlen);

#ifdef HAVE_IPV6
  if (afi == AFI_IP6)
    rn = nsm_ptree_node_lookup (vrf->afi[afi].rib[SAFI_UNICAST], 
                                (u_char *) &p->u.prefix6,
                                p->prefixlen);
#endif

  if (rn)
    {
      nsm_ptree_unlock_node (rn);

      if (rib == NULL)
        rib = &RibNull;

      if (nse->service.protocol_id == APN_PROTO_RSVP)
        nsm_afi_override[afi].send_route (nse, msg_id, p, rn->key_len, rib, vrf, 0, NULL);
      else
        nsm_afi[afi].send_route (nse, msg_id, p, rn->key_len, rib, vrf, 0, NULL);

      /* Add client to the client list. */
      if (rn->nh_list)
        {
          /* First check if similar lookup exists for some other client. */
          if (!nsm_server_nh_reg_client_add (rn, nsc, p, EXACT_MATCH_LOOKUP))
            nsm_server_nh_reg_client_init (rn, nsc, p, EXACT_MATCH_LOOKUP);
        }
      else
        {
          NSM_RIB_NH_LIST_SET (rn);
          nsm_server_nh_reg_client_init (rn, nsc, p, EXACT_MATCH_LOOKUP);
        }
    }
  /* No route found in rib. Store this registration in nexthop lookup tree. */
  else
    {

      if (afi == AFI_IP)
        rn = nsm_ptree_node_get (vrf->afi[afi].nh_reg, 
                                 (u_char *)&p->u.prefix4,
                                 p->prefixlen);

#ifdef HAVE_IPV6
      if (afi == AFI_IP6)
        rn = nsm_ptree_node_get (vrf->afi[afi].nh_reg, 
                                 (u_char *) &p->u.prefix6,
                                 p->prefixlen);
#endif
      if (rn == NULL)
        { 
          if (IS_NSM_DEBUG_EVENT)
            zlog_info (NSM_ZG, "NSM_PTREE create failed");
 
          return NSM_FAILURE;
         }
      if (rn->info)
        nsm_ptree_unlock_node (rn);

      NSM_REG_NH_LIST_SET (rn);
      nsm_server_nh_reg_client_init (rn, nsc, p, EXACT_MATCH_LOOKUP);
          
      if (nse->service.protocol_id == APN_PROTO_RSVP)      
        nsm_afi_override[afi].send_route (nse, msg_id, p, rn->key_len, &RibNull, vrf, 0, NULL);
      else
        nsm_afi[afi].send_route (nse, msg_id, p, rn->key_len, &RibNull,
                                 vrf, 0, NULL);
    }
  return 0;
}

int
nsm_server_route_exact_lookup_dereg (struct nsm_ptree_table *nh_reg_table,
                                     struct nsm_ptree_table *rib_table,
                                     struct nsm_server_client *nsc,
                                     struct prefix *p)
{
  struct nsm_ptree_node *rn = NULL;

  /* First check in nexthop lookup registration tree. 
     If present, just delete this information from the tree.
     If not present, the registration information is in the RIB. */ 
 if (p->family  == AF_INET)
   rn = nsm_ptree_node_lookup (nh_reg_table,
                               (u_char *) &p->u.prefix4,
                                p->prefixlen);

#ifdef HAVE_IPV6
  if (p->family  == AF_INET6) 
    rn = nsm_ptree_node_lookup (nh_reg_table,
                                (u_char *) &p->u.prefix6,
                                p->prefixlen);
#endif

  if (rn)
    { 
      nsm_ptree_unlock_node (rn);
      
      if (rn->nh_list)
        {
          nsm_server_nh_reg_client_unset_all (rn, nsc, p, EXACT_MATCH_LOOKUP);

          if (LIST_ISEMPTY (rn->nh_list))
            NSM_REG_NH_LIST_UNSET (rn);

          return 0;
        }
    }

  /* Registration not found in nh_reg table, clean up from RIB. */
 if (p->family == AF_INET)
   rn = nsm_ptree_node_lookup (rib_table,
                               (u_char *) &p->u.prefix4,
                                p->prefixlen);

#ifdef HAVE_IPV6
  if (p->family == AF_INET6) 
  rn = nsm_ptree_node_lookup (rib_table,
                              (u_char *) &p->u.prefix6,
                               p->prefixlen);
#endif


  if (rn)
    {
      nsm_ptree_unlock_node (rn);

      if (rn->nh_list)
        {
          nsm_server_nh_reg_client_unset_all (rn, nsc, p, EXACT_MATCH_LOOKUP);

          if (LIST_ISEMPTY (rn->nh_list))
            NSM_RIB_NH_LIST_UNSET (rn);
        }
    }

  return 0;
}

int
nsm_server_route_best_lookup_reg (struct nsm_vrf *vrf, afi_t afi,
                                  struct nsm_server_entry *nse,
                                  struct rib *rib, struct prefix *p,
                                  u_int32_t msg_id, struct nsm_ptree_node *rn)
{
  struct nsm_server_client *nsc = nse->nsc;
  
  if (rib != NULL)
    {
      if (nse->service.protocol_id == APN_PROTO_RSVP)      
        nsm_afi_override[afi].send_route (nse, msg_id, p, rn->key_len, 
                                          rib, vrf, 0, NULL);
      else if(nse->service.protocol_id != APN_PROTO_BGP)
        nsm_afi[afi].send_route (nse, msg_id, p, rn->key_len, 
                                 rib, vrf, 0, NULL);

      /* Add client to the client list. */
      if (rn->nh_list)
        {
          if (!nsm_server_nh_reg_client_add (rn, nsc, p, BEST_MATCH_LOOKUP))
            nsm_server_nh_reg_client_init (rn, nsc, p, BEST_MATCH_LOOKUP);
        }
      else
        {
          NSM_RIB_NH_LIST_SET (rn);
          nsm_server_nh_reg_client_init (rn, nsc, p, BEST_MATCH_LOOKUP);
        }
    }
  else
    {
      /* No route found in rib. Store this registration in 
         nexthop lookup tree. */
      if (afi == AFI_IP)
        rn = nsm_ptree_node_get (vrf->afi[afi].nh_reg,
                                 (u_char *) &p->u.prefix4,
                                 p->prefixlen);

#ifdef HAVE_IPV6
      if (afi == AFI_IP6) 
        rn = nsm_ptree_node_get (vrf->afi[afi].nh_reg,
                                 (u_char *) &p->u.prefix6,
                                 p->prefixlen);
#endif
      if (rn == NULL)
        { 
          if (IS_NSM_DEBUG_EVENT)
             zlog_info (NSM_ZG, "NSM_PTREE create failed");
 
          return NSM_FAILURE;
        }

      if (rn->info)
        nsm_ptree_unlock_node (rn);

      NSM_REG_NH_LIST_SET (rn);
      nsm_server_nh_reg_client_init (rn, nsc, p, BEST_MATCH_LOOKUP);
          
      if (nse->service.protocol_id == APN_PROTO_RSVP)      
        nsm_afi_override[afi].send_route (nse, msg_id, p, rn->key_len, 
                                          &RibNull, vrf, 0, NULL);
      /* For BGP donot send any route as BGP registers with NSM after
         knowing the nexthop validity */
      else if (nse->service.protocol_id != APN_PROTO_BGP)
        nsm_afi[afi].send_route (nse, msg_id, p, rn->key_len, 
                                 &RibNull, vrf, 0, NULL);
    }

  return 0;
}

int
nsm_server_route_best_lookup_dereg (struct nsm_ptree_table *nh_reg_table,
                                    struct nsm_ptree_table *rib_table,
                                    struct nsm_server_client *nsc,
                                    struct prefix *p)
{
  struct nsm_ptree_node *rn = NULL;

  /* First check in nexthop lookup registration tree. 
     If present, just delete this information from the tree.
     If not present, the registration information is in the RIB. */
  if (p->family == AF_INET)
   rn = nsm_ptree_node_lookup (nh_reg_table,
                               (u_char *) &p->u.prefix4,
                                p->prefixlen);

#ifdef HAVE_IPV6
  if (p->family == AF_INET6) 
    rn = nsm_ptree_node_lookup (nh_reg_table,
                                (u_char *) &p->u.prefix6,
                                p->prefixlen);
#endif

  if (rn)
    { 
      nsm_ptree_unlock_node (rn);
      
      if (rn->nh_list)
        {
          nsm_server_nh_reg_client_unset_all (rn, nsc, p, BEST_MATCH_LOOKUP);

          if (LIST_ISEMPTY (rn->nh_list))
            NSM_REG_NH_LIST_UNSET (rn);

          return 0;
        }
    }

  /* Registration not found in nh_reg table, clean up from RIB. */
   if (p->family == AF_INET)
     rn = nsm_ptree_node_match (rib_table,
                                (u_char *) &p->u.prefix4,
                                p->prefixlen);

#ifdef HAVE_IPV6
  if (p->family == AF_INET6) 
    rn = nsm_ptree_node_match (rib_table,
                               (u_char *) &p->u.prefix6,
                               p->prefixlen);
#endif

  if (rn)
    {
      nsm_ptree_unlock_node (rn);

      if (rn->nh_list)
        {
          nsm_server_nh_reg_client_unset_all (rn, nsc, p, BEST_MATCH_LOOKUP);
          
          if (LIST_ISEMPTY (rn->nh_list))
            NSM_RIB_NH_LIST_UNSET (rn);
        }
    }

  return 0;
}

/* Register for a IPv4 exact match lookup. */
int
nsm_server_recv_ipv4_route_exact_lookup_reg (struct nsm_msg_header *header,
                                             void *arg, void *message)
{
  struct nsm_master *nm;
  struct nsm_server_entry *nse = arg;
  struct nsm_server *ns = nse->ns;
  struct nsm_msg_route_lookup_ipv4 *msg = message;
  u_int32_t exclude_proto;
  struct prefix p;
  struct rib *rib;
  struct nsm_vrf *vrf;
  u_char sub_type;

  if (IS_NSM_DEBUG_RECV)
    nsm_route_lookup_ipv4_dump (ns->zg, msg);

  /* Set the protocol routes to exclude. */
  nsm_set_nh_lookup_exclude_protocol (nse->service.protocol_id, 0,
                                      &exclude_proto, &sub_type);

  /* Get VR context. */
  nm = nsm_master_lookup_by_id (nzg, header->vr_id);
  if (nm == NULL)
    return 0;

  vrf = nsm_vrf_lookup_by_id (nm, header->vrf_id);
  if (vrf == NULL)
    return 0;

  p.family = AF_INET;
  if (NSM_CHECK_CTYPE (msg->cindex, NSM_ROUTE_LOOKUP_CTYPE_PREFIXLEN))
    p.prefixlen = msg->prefixlen;
  else
    p.prefixlen = IPV4_MAX_PREFIXLEN;
  p.u.prefix4 = msg->addr;
  apply_mask (&p);

  /* Handle multicast registrations */
  if (APN_PROTO_MCAST (nse->service.protocol_id))
    {
      nsm_multicast_nh_reg (vrf, AFI_IP, nse->nsc, &p, EXACT_MATCH_LOOKUP);
      return 0;
    }

  /* Lookup RIB by the prefix. */
  rib = nsm_rib_lookup (vrf, AFI_IP, &p, NULL, exclude_proto, sub_type);

  nsm_server_route_exact_lookup_reg (vrf, AFI_IP, nse, rib, &p,
                                     header->message_id);

  return 0;
}

/* Deregister for a IPv4 exact match lookup. */
int
nsm_server_recv_ipv4_route_exact_lookup_dereg (struct nsm_msg_header *header,
                                               void *arg, void *message)
{
  struct nsm_master *nm;
  struct nsm_server_entry *nse = arg;
  struct nsm_server *ns = nse->ns;
  struct nsm_server_client *nsc = nse->nsc;
  struct nsm_msg_route_lookup_ipv4 *msg = message;
  struct nsm_vrf *vrf;
  struct prefix p;
 
  if (IS_NSM_DEBUG_RECV)
    nsm_route_lookup_ipv4_dump (ns->zg, msg);

  /* Get VR context. */
  nm = nsm_master_lookup_by_id (nzg, header->vr_id);
  if (nm == NULL)
    return 0;

  vrf = nsm_vrf_lookup_by_id (nm, header->vrf_id);
  if (vrf == NULL)
    return 0;

  p.family = AF_INET;
  if (NSM_CHECK_CTYPE (msg->cindex, NSM_ROUTE_LOOKUP_CTYPE_PREFIXLEN))
    p.prefixlen = msg->prefixlen;
  else
    p.prefixlen = IPV4_MAX_PREFIXLEN;
  p.u.prefix4 = msg->addr;
  apply_mask (&p);

  /* Handle multicast registrations */
  if (APN_PROTO_MCAST (nse->service.protocol_id))
    {
      nsm_multicast_nh_dereg (vrf, AFI_IP, nse->nsc, &p, EXACT_MATCH_LOOKUP);
      return 0;
    }

  nsm_server_route_exact_lookup_dereg (vrf->afi[AFI_IP].nh_reg,
                                       vrf->RIB_IPV4_UNICAST, nsc, &p);
  return 0;
}

/* Register for a IPv4 exact match lookup. */
int
nsm_server_recv_ipv4_route_best_lookup_reg (struct nsm_msg_header *header,
                                            void *arg, void *message)
{
  struct nsm_master *nm;
  struct nsm_server_entry *nse = arg;
  struct nsm_server *ns = nse->ns;
  struct nsm_msg_route_lookup_ipv4 *msg = message;
  struct nsm_vrf *vrf;
  struct nsm_ptree_node *rn = NULL;
  struct rib *rib;
  struct prefix p;
  u_int32_t exclude_proto;
  u_char sub_type;

  if (IS_NSM_DEBUG_RECV)
    nsm_route_lookup_ipv4_dump (ns->zg, msg);

  /* Set the protocol routes to exclude. */
  nsm_set_nh_lookup_exclude_protocol (nse->service.protocol_id, 0,
                                      &exclude_proto, &sub_type);

  /* Get VR context. */
  nm = nsm_master_lookup_by_id (nzg, header->vr_id);
  if (nm == NULL)
    return 0;

  vrf = nsm_vrf_lookup_by_id (nm, header->vrf_id);
  if (vrf == NULL)
    return 0;

  pal_mem_set (&p, 0, sizeof (struct prefix));
  p.family = AF_INET;
  p.prefixlen = IPV4_MAX_PREFIXLEN;
  p.u.prefix4 = msg->addr;

  rib = nsm_rib_match (vrf, AFI_IP, &p, &rn, exclude_proto, sub_type);

  if (NSM_CHECK_CTYPE (msg->cindex, NSM_ROUTE_LOOKUP_CTYPE_PREFIXLEN))
    {
      p.prefixlen = msg->prefixlen;
      apply_mask (&p);
    }

  /* Handle multicast registrations */
  if (APN_PROTO_MCAST (nse->service.protocol_id))
    {
      nsm_multicast_nh_reg (vrf, AFI_IP, nse->nsc, &p, BEST_MATCH_LOOKUP);
      return 0;
    }


  nsm_server_route_best_lookup_reg (vrf, AFI_IP, nse, rib, &p,
                                    header->message_id, rn);

  return 0;
}

/* Deregister for a IPv4 exact match lookup. */
int
nsm_server_recv_ipv4_route_best_lookup_dereg (struct nsm_msg_header *header,
                                              void *arg, void *message)
{
  struct nsm_master *nm;
  struct nsm_server_entry *nse = arg;
  struct nsm_server *ns = nse->ns;
  struct nsm_server_client *nsc = nse->nsc;
  struct nsm_msg_route_lookup_ipv4 *msg = message;
  struct prefix p;
  struct nsm_vrf *vrf;
 
  if (IS_NSM_DEBUG_RECV)
    nsm_route_lookup_ipv4_dump (ns->zg, msg);

  /* Get VR context. */
  nm = nsm_master_lookup_by_id (nzg, header->vr_id);
  if (nm == NULL)
    return 0;

  vrf = nsm_vrf_lookup_by_id (nm, header->vrf_id);
  if (vrf == NULL)
    return 0;

  p.family = AF_INET;
  if (NSM_CHECK_CTYPE (msg->cindex, NSM_ROUTE_LOOKUP_CTYPE_PREFIXLEN))
    p.prefixlen = msg->prefixlen;
  else
    p.prefixlen = IPV4_MAX_PREFIXLEN;
  p.u.prefix4 = msg->addr;
  apply_mask (&p);

  /* Handle multicast registrations */
  if (APN_PROTO_MCAST (nse->service.protocol_id))
    {
      nsm_multicast_nh_dereg (vrf, AFI_IP, nse->nsc, &p, BEST_MATCH_LOOKUP);
      return 0;
    }

  nsm_server_route_best_lookup_dereg (vrf->afi[AFI_IP].nh_reg,
                                      vrf->RIB_IPV4_UNICAST, nsc, &p);
  return 0;
}

int
nsm_server_nexthop_tracking (struct nsm_msg_header *header,
                             void *arg, void *message)
{
  struct nsm_msg_nexthop_tracking *msg = NULL;
  struct nsm_server_entry *nse = NULL;
  struct nsm_server_client *nc = NULL; 

  msg = message;
  nse = arg;
  
  if (!msg || !arg)
    return -1;

  nc  = nse->nsc;
  
  if (IS_NSM_DEBUG_RECV)
    nsm_nh_track_dump (nse->ns->zg, msg);

   nc->nhtenable = msg->flags; 
   return 0;
}

#ifdef HAVE_IPV6
/* Receive IPv6 exact match lookup.  */
int
nsm_server_recv_ipv6_route_exact_lookup (struct nsm_msg_header *header,
                                         void *arg, void *message)
{
  struct nsm_master *nm;
  struct nsm_server_entry *nse = arg;
  struct nsm_server *ns = nse->ns;
  struct nsm_msg_route_lookup_ipv6 *msg = message;
  u_int32_t exclude_proto;
  struct nsm_vrf *vrf;
  struct prefix p;
  struct rib *rib;
  u_char sub_type;

  if (IS_NSM_DEBUG_RECV)
    nsm_route_lookup_ipv6_dump (ns->zg, msg);

  /* Set the protocol routes to exclude. */
  nsm_set_nh_lookup_exclude_protocol (nse->service.protocol_id, 0,
                                      &exclude_proto, &sub_type);

  /* Get VR context. */
  nm = nsm_master_lookup_by_id (nzg, header->vr_id);
  if (nm == NULL)
    return 0;

  vrf = nsm_vrf_lookup_by_id (nm, header->vrf_id);
  if (vrf == NULL)
    return 0;

  /* Set prefix. */
  p.family = AF_INET6;
  if (NSM_CHECK_CTYPE (msg->cindex, NSM_ROUTE_LOOKUP_CTYPE_PREFIXLEN))
    p.prefixlen = msg->prefixlen;
  else
    p.prefixlen = IPV6_MAX_PREFIXLEN;
  p.u.prefix6 = msg->addr;

  /* Lookup RIB by the prefix. */
  if (APN_PROTO_MCAST (nse->service.protocol_id))
    rib = nsm_mrib_lookup (vrf, AFI_IP6, &p, NULL, exclude_proto);
  else
    rib = nsm_rib_lookup (vrf, AFI_IP6, &p, NULL, exclude_proto, sub_type);

  if (rib == NULL)
    rib = &RibNull;

  /* Send it to client. */
  nsm_send_ipv6_add_override (nse, header->message_id, &p,
                              0, rib, vrf, 0, NULL);

  return 0;
}

/* Receive IPv6 exact best lookup.  */
int
nsm_server_recv_ipv6_route_best_lookup (struct nsm_msg_header *header,
                                        void *arg, void *message)
{
  struct nsm_master *nm;
  struct nsm_server_entry *nse = arg;
  struct nsm_server *ns = nse->ns;
  struct nsm_msg_route_lookup_ipv6 *msg = message;
  struct nsm_vrf *vrf;
  struct rib *rib;
  struct nsm_ptree_node *rn = NULL;
  struct prefix p;
  u_int32_t exclude_proto;
  struct prefix addr;
  u_char sub_type;

  if (IS_NSM_DEBUG_RECV)
    nsm_route_lookup_ipv6_dump (ns->zg, msg);

  /* Set the protocol routes to exclude. */
  nsm_set_nh_lookup_exclude_protocol (nse->service.protocol_id, 0,
                                      &exclude_proto, &sub_type);

  /* Get VR context. */
  nm = nsm_master_lookup_by_id (nzg, header->vr_id);
  if (nm == NULL)
    return 0;

  vrf = nsm_vrf_lookup_by_id (nm, header->vrf_id);
  if (vrf == NULL)
    return 0;

  /* Set prefix. */
  pal_mem_set (&p, 0, sizeof (struct prefix));
  p.family = AF_INET6;
  p.prefixlen = IPV6_MAX_PREFIXLEN;
  p.u.prefix6 = msg->addr;

  if (APN_PROTO_MCAST (nse->service.protocol_id))
    rib = nsm_mrib_match (vrf, AFI_IP6, &p, &rn, exclude_proto);
  else
    rib = nsm_rib_match (vrf, AFI_IP6, &p, &rn, exclude_proto, sub_type);

  if (rib == NULL)
    rib = &RibNull;

  /* Send reply to client. */
  if (rn)
    {
      pal_mem_set (&addr, 0, sizeof (struct prefix));
      addr.family = AF_INET6;
      addr.prefixlen = rn->key_len;
      NSM_PTREE_KEY_COPY (&addr.u.prefix6, rn);

      nsm_send_ipv6_add_override (nse, header->message_id, &addr,
          rn->key_len, rib, vrf, 0, NULL);
    }
  else
    nsm_send_ipv6_add_override (nse, header->message_id, &p,
                                0, rib, vrf, 0, NULL);

  return 0;
}

/* Register for a IPv6 exact match lookup. */
int
nsm_server_recv_ipv6_route_exact_lookup_reg (struct nsm_msg_header *header,
                                             void *arg, void *message)
{
  struct nsm_master *nm;
  struct nsm_server_entry *nse = arg;
  struct nsm_server *ns = nse->ns;
  struct nsm_msg_route_lookup_ipv6 *msg = message;
  u_int32_t exclude_proto;
  struct prefix p;
  struct rib *rib;
  struct nsm_vrf *vrf;
  u_char sub_type;

  if (IS_NSM_DEBUG_RECV)
    nsm_route_lookup_ipv6_dump (ns->zg, msg);

  /* Set the protocol routes to exclude. */
  nsm_set_nh_lookup_exclude_protocol (nse->service.protocol_id, 0,
                                      &exclude_proto, &sub_type);

  /* Get VR context. */
  nm = nsm_master_lookup_by_id (nzg, header->vr_id);
  if (nm == NULL)
    return 0;

  vrf = nsm_vrf_lookup_by_id (nm, header->vrf_id);
  if (vrf == NULL)
    return 0;

  p.family = AF_INET6;
  if (NSM_CHECK_CTYPE (msg->cindex, NSM_ROUTE_LOOKUP_CTYPE_PREFIXLEN))
    p.prefixlen = msg->prefixlen;
  else
    p.prefixlen = IPV6_MAX_PREFIXLEN;
  p.u.prefix6 = msg->addr;
  apply_mask (&p);

  /* Handle multicast registrations */
  if (APN_PROTO_MCAST (nse->service.protocol_id))
    {
      nsm_multicast_nh_reg (vrf, AFI_IP6, nse->nsc, &p, EXACT_MATCH_LOOKUP);
      return 0;
    }

  /* Lookup RIB by the prefix. */
  rib = nsm_rib_lookup (vrf, AFI_IP6, &p, NULL, exclude_proto, sub_type);

  nsm_server_route_exact_lookup_reg (vrf, AFI_IP6, nse, rib, &p,
                                     header->message_id);

  return 0;
}

/* Deregister for a IPv6 exact match lookup. */
int
nsm_server_recv_ipv6_route_exact_lookup_dereg (struct nsm_msg_header *header,
                                               void *arg, void *message)
{
  struct nsm_master *nm;
  struct nsm_server_entry *nse = arg;
  struct nsm_server *ns = nse->ns;
  struct nsm_server_client *nsc = nse->nsc;
  struct nsm_msg_route_lookup_ipv6 *msg = message;
  struct prefix p;
  struct nsm_vrf *vrf;
 
  if (IS_NSM_DEBUG_RECV)
    nsm_route_lookup_ipv6_dump (ns->zg, msg);

  /* Get VR context. */
  nm = nsm_master_lookup_by_id (nzg, header->vr_id);
  if (nm == NULL)
    return 0;

  vrf = nsm_vrf_lookup_by_id (nm, header->vrf_id);
  if (vrf == NULL)
    return 0;

  p.family = AF_INET6;
  if (NSM_CHECK_CTYPE (msg->cindex, NSM_ROUTE_LOOKUP_CTYPE_PREFIXLEN))
    p.prefixlen = msg->prefixlen;
  else
    p.prefixlen = IPV6_MAX_PREFIXLEN;
  p.u.prefix6 = msg->addr;
  apply_mask (&p);

  if (APN_PROTO_MCAST (nse->service.protocol_id))
    {
      nsm_multicast_nh_dereg (vrf, AFI_IP6, nse->nsc, &p, EXACT_MATCH_LOOKUP);
      return 0;
    }

  nsm_server_route_exact_lookup_dereg (vrf->afi[AFI_IP6].nh_reg,
                                       vrf->RIB_IPV6_UNICAST, nsc, &p);
  return 0;
}

/* Register for a IPv6 exact match lookup. */
int
nsm_server_recv_ipv6_route_best_lookup_reg (struct nsm_msg_header *header,
                                            void *arg, void *message)
{
  struct nsm_master *nm;
  struct nsm_server_entry *nse = arg;
  struct nsm_server *ns = nse->ns;
  struct nsm_msg_route_lookup_ipv6 *msg = message;
  struct nsm_vrf *vrf;
  struct nsm_ptree_node *rn = NULL;
  struct rib *rib;
  struct prefix p;
  u_int32_t exclude_proto;
  u_char sub_type;

  /* Debug.  */
  if (IS_NSM_DEBUG_RECV)
    nsm_route_lookup_ipv6_dump (ns->zg, msg);

  /* Set the protocol routes to exclude. */
  nsm_set_nh_lookup_exclude_protocol (nse->service.protocol_id, 0,
                                      &exclude_proto, &sub_type);

  /* Get VR context. */
  nm = nsm_master_lookup_by_id (nzg, header->vr_id);
  if (nm == NULL)
    return 0;

  vrf = nsm_vrf_lookup_by_id (nm, header->vrf_id);
  if (vrf == NULL)
    return 0;

  pal_mem_set (&p, 0, sizeof (struct prefix));
  p.family = AF_INET6;
  p.prefixlen = IPV6_MAX_PREFIXLEN;
  p.u.prefix6 = msg->addr;
 
  rib = nsm_rib_match (vrf, AFI_IP6, &p, &rn, exclude_proto, sub_type);

  if (NSM_CHECK_CTYPE (msg->cindex, NSM_ROUTE_LOOKUP_CTYPE_PREFIXLEN))
    {
      p.prefixlen = msg->prefixlen;
      apply_mask (&p);
    }

  if (APN_PROTO_MCAST (nse->service.protocol_id))
    {
      nsm_multicast_nh_reg (vrf, AFI_IP6, nse->nsc, &p, BEST_MATCH_LOOKUP);
      return 0;
    }

  nsm_server_route_best_lookup_reg (vrf, AFI_IP6, nse, rib, &p,
                                    header->message_id, rn);

  return 0;
}

/* Deregister for a IPv6 exact match lookup. */
int
nsm_server_recv_ipv6_route_best_lookup_dereg (struct nsm_msg_header *header,
                                              void *arg, void *message)
{
  struct nsm_master *nm;
  struct nsm_server_entry *nse = arg;
  struct nsm_server *ns = nse->ns;
  struct nsm_server_client *nsc = nse->nsc;
  struct nsm_msg_route_lookup_ipv6 *msg = message;
  struct prefix p;
  struct nsm_vrf *vrf;

  /* Debug.  */
  if (IS_NSM_DEBUG_RECV)
    nsm_route_lookup_ipv6_dump (ns->zg, msg);

  /* Get VR context. */
  nm = nsm_master_lookup_by_id (nzg, header->vr_id);
  if (nm == NULL)
    return 0;

  vrf = nsm_vrf_lookup_by_id (nm, header->vrf_id);
  if (vrf == NULL)
    return 0;

  p.family = AF_INET6;
  if (NSM_CHECK_CTYPE (msg->cindex, NSM_ROUTE_LOOKUP_CTYPE_PREFIXLEN))
    p.prefixlen = msg->prefixlen;
  else
    p.prefixlen = IPV6_MAX_PREFIXLEN;
  p.u.prefix6 = msg->addr;

  if (APN_PROTO_MCAST (nse->service.protocol_id))
    {
      nsm_multicast_nh_dereg (vrf, AFI_IP6, nse->nsc, &p, BEST_MATCH_LOOKUP);
      return 0;
    }

  nsm_server_route_best_lookup_dereg (vrf->afi[AFI_IP6].nh_reg,
                                      vrf->RIB_IPV6_UNICAST, nsc, &p);
  return 0;
}
#endif /* HAVE_IPV6 */

void  
nsm_rib_set_server_callback (struct nsm_server *ns)
{
  nsm_server_set_callback (ns, NSM_MSG_ROUTE_IPV4,
                           nsm_parse_route_ipv4,
                           nsm_server_recv_route_ipv4);
  nsm_server_set_callback (ns, NSM_MSG_IPV4_NEXTHOP_EXACT_LOOKUP,
                           nsm_parse_ipv4_route_lookup,
                           nsm_server_recv_ipv4_route_exact_lookup);
  nsm_server_set_callback (ns, NSM_MSG_IPV4_NEXTHOP_BEST_LOOKUP,
                           nsm_parse_ipv4_route_lookup,
                           nsm_server_recv_ipv4_route_best_lookup);
  nsm_server_set_callback (ns, NSM_MSG_IPV4_NEXTHOP_EXACT_LOOKUP_REG,
                           nsm_parse_ipv4_route_lookup,
                           nsm_server_recv_ipv4_route_exact_lookup_reg);
  nsm_server_set_callback (ns, NSM_MSG_IPV4_NEXTHOP_EXACT_LOOKUP_DEREG,
                           nsm_parse_ipv4_route_lookup,
                           nsm_server_recv_ipv4_route_exact_lookup_dereg);
  nsm_server_set_callback (ns, NSM_MSG_IPV4_NEXTHOP_BEST_LOOKUP_REG,
                           nsm_parse_ipv4_route_lookup,
                           nsm_server_recv_ipv4_route_best_lookup_reg);
  nsm_server_set_callback (ns, NSM_MSG_IPV4_NEXTHOP_BEST_LOOKUP_DEREG,
                           nsm_parse_ipv4_route_lookup,
                           nsm_server_recv_ipv4_route_best_lookup_dereg);
#ifdef HAVE_IPV6
  IF_NSM_CAP_HAVE_IPV6
    {
      nsm_server_set_callback (ns, NSM_MSG_ROUTE_IPV6,
                               nsm_parse_route_ipv6,
                               nsm_server_recv_route_ipv6);
      nsm_server_set_callback (ns, NSM_MSG_IPV6_NEXTHOP_EXACT_LOOKUP,
                               nsm_parse_ipv6_route_lookup,
                               nsm_server_recv_ipv6_route_exact_lookup);
      nsm_server_set_callback (ns, NSM_MSG_IPV6_NEXTHOP_BEST_LOOKUP,
                               nsm_parse_ipv6_route_lookup,
                               nsm_server_recv_ipv6_route_best_lookup);
      nsm_server_set_callback (ns, NSM_MSG_IPV6_NEXTHOP_EXACT_LOOKUP_REG,
                               nsm_parse_ipv6_route_lookup,
                               nsm_server_recv_ipv6_route_exact_lookup_reg);
      nsm_server_set_callback (ns, NSM_MSG_IPV6_NEXTHOP_EXACT_LOOKUP_DEREG,
                               nsm_parse_ipv6_route_lookup,
                               nsm_server_recv_ipv6_route_exact_lookup_dereg);
      nsm_server_set_callback (ns, NSM_MSG_IPV6_NEXTHOP_BEST_LOOKUP_REG,
                               nsm_parse_ipv6_route_lookup,
                               nsm_server_recv_ipv6_route_best_lookup_reg);
      nsm_server_set_callback (ns, NSM_MSG_IPV6_NEXTHOP_BEST_LOOKUP_DEREG,
                               nsm_parse_ipv6_route_lookup,
                               nsm_server_recv_ipv6_route_best_lookup_dereg);
    }
#endif /* HAVE_IPV6 */
  nsm_server_set_callback (ns, NSM_MSG_NEXTHOP_TRACKING,
                           nsm_parse_nexthop_tracking,
                           nsm_server_nexthop_tracking);
}


