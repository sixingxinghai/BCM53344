/* Copyright (C) 2004  All Rights Reserved. */

#include "config.h"
#include "hsl_os.h"

#include "bcm_incl.h"

#include "hsl_types.h"
#include "hsl_oss.h"
#include "hsl_avl.h"
#include "hsl_logger.h"
#include "hsl_error.h"
#include "hsl_table.h"
#include "hsl_ether.h"
#include "hsl_ifmgr.h"
#include "hsl_if_hw.h"
#include "hsl_bcm_if.h"
#include "hsl_bcm_ifmap.h"
#include "hsl_fib.h"
#include "hsl_fib_hw.h"
#ifdef HAVE_L2
#include "hal_types.h"
#include "hal_l2.h"
#include "hal_msg.h"
#include "hsl_vlan.h"
#include "hsl_bridge.h"
#endif /* HAVE_L2 */
#ifdef HAVE_MPLS
#include "hsl_mpls.h"
#include "hsl_bcm_mpls.h"
#endif /* HAVE_MPLS */

static struct hsl_fib_hw_callbacks hsl_bcm_fib_callbacks;
static int _hsl_bcm_nh_get (hsl_fib_id_t fib_id, struct hsl_nh_entry *nh, bcmx_l3_host_t *ip);
#ifdef HAVE_IPV6
static int _hsl_bcm_get_host_route (hsl_fib_id_t fib_id, hsl_prefix_t *p, 
                                   struct hsl_nh_entry *nh, bcmx_l3_host_t *ip, 
                                   struct hsl_prefix_entry *pe);
#endif /* HAVE_IPV6 */

/* 
   Initialization. 
*/
int 
hsl_bcm_fib_init (hsl_fib_id_t fib_id)
{
  return 0;
}

/* 
   Deinitialization. 
*/
int 
hsl_bcm_fib_deinit (hsl_fib_id_t fib_id)
{
  return 0;
}

/* 
   Dump. 
*/
void 
hsl_bcm_fib_dump (hsl_fib_id_t fib_id)
{
  return;
}

#ifdef HAVE_IPV6
/* 
   Fill up bcmx host add/delete request structure from rnp, nh.
*/
static int
_hsl_bcm_get_host (hsl_fib_id_t fib_id, hsl_prefix_t *p, struct hsl_nh_entry *nh,
                   bcmx_l3_host_t *ip, int connected)
{
  int ret;

  if (((p->family == AF_INET) && (nh && (nh->rn->p.family != AF_INET)))
      || ((p->family == AF_INET6) && (nh && (nh->rn->p.family != AF_INET6)))
      )
    return HSL_FIB_ERR_INVALID_PARAM;

  if (nh)
    {
      ret = _hsl_bcm_nh_get (fib_id, nh, ip);
      return ret;
    }

  /* No nexthop. */
  /* Initialize route. */
  bcmx_l3_host_t_init (ip);

  ip->l3a_vrf = (bcm_vrf_t)fib_id;

  if (p->family == AF_INET)
    {
      ip->l3a_ip_addr = p->u.prefix4;
    }
  else if (p->family == AF_INET6)
    { 
     sal_memcpy (ip->l3a_ip6_addr, &p->u.prefix6, BCM_IP6_ADDRLEN);
     ip->l3a_flags |= BCM_L3_IP6;
    }

  ip->l3a_lport = BCMX_LPORT_LOCAL_CPU;
  ip->l3a_flags |= BCM_L3_L2TOCPU; /* To get original headers. */
  if (connected)
    {
      ip->l3a_flags |= BCM_L3_DEFIP_LOCAL;
    }
  
  return 0;
}
#endif /* HAVE_IPV6 */

/* 
   Fill up bcmx route add/delete request structure. 
*/
static int
_hsl_bcm_get_route (hsl_fib_id_t fib_id, struct hsl_route_node *rnp, struct hsl_nh_entry *nh, bcmx_l3_route_t *r, int oper)
{
  hsl_ipv4Address_t mask;
  struct hsl_bcm_if *bcmifp;
  struct hsl_if *ifp;
  struct hsl_prefix_entry *pe;
#ifdef HAVE_L2
  int tagged = 0;
  struct hsl_if_list *node = NULL;
#endif /* HAVE_L2 */

  pe = rnp->info;

  if (! pe || ! nh)
    return HSL_FIB_ERR_INVALID_PARAM;

  if (((rnp->p.family == AF_INET) && (nh->rn->p.family != AF_INET))
#ifdef HAVE_IPV6
      || ((rnp->p.family == AF_INET6) && (nh->rn->p.family != AF_INET6))
#endif /* HAVE_IPV6 */
      )
    return HSL_FIB_ERR_INVALID_PARAM;

  /* Initialize route. */
  bcmx_l3_route_t_init (r);

  r->l3a_vrf = (bcm_vrf_t)fib_id;

  if (pe && CHECK_FLAG (pe->flags, HSL_PREFIX_ENTRY_IN_HW) && (oper == HSL_OPER_ADD))
    r->l3a_flags |= BCM_L3_REPLACE;

  /* Set hit bit for updation. */
  if (oper == HSL_OPER_ADD)
    r->l3a_flags |= BCM_L3_HIT;

  /* Set VID. */
#ifdef HAVE_MPLS
  if (nh->ifp && nh->ifp->type == HSL_IF_TYPE_MPLS)
    r->l3a_vid = nh->ifp->u.mpls.vid;
  else
#endif /* HAVE_MPLS */
    {
      if (CHECK_FLAG (nh->flags, HSL_NH_ENTRY_BLACKHOLE))
        r->l3a_vid = 0;
      else
        r->l3a_vid = nh->ifp->u.ip.vid;
    }

  /* Check for ECMP. */
  if (rnp->is_ecmp)
    r->l3a_flags |= BCM_L3_MULTIPATH;

  /* Set prefix. */
  if (rnp->p.family == AF_INET)
    {
      r->l3a_subnet = rnp->p.u.prefix4;
      hsl_masklen2ip (rnp->p.prefixlen, &mask);
      r->l3a_ip_mask = mask;
    }
#ifdef HAVE_IPV6
  else if (rnp->p.family == AF_INET6)
    {
      sal_memcpy (r->l3a_ip6_net, &rnp->p.u.prefix6, BCM_IP6_ADDRLEN);

      /* Get mask. */
      bcm_ip6_mask_create (r->l3a_ip6_mask, rnp->p.prefixlen);

      /* Set flags for IPv6. */
      r->l3a_flags |= BCM_L3_IP6;
    }
#endif /* HAVE_IPV6 */

  /* Set nexthop. */
  if (rnp->p.family == AF_INET)
    {
      r->l3a_nexthop_ip = nh->rn->p.u.prefix4;
    }

  /* Set nexthop MAC. */
  memcpy (r->l3a_nexthop_mac, nh->mac, HSL_ETHER_ALEN);

  /* Add the blackhole route it hw.*/
  /* Note: Currently only EZR supports the DST Discard flag*/

  if (CHECK_FLAG (nh->flags, HSL_NH_ENTRY_BLACKHOLE))
     {
       r->l3a_lport = BCMX_LPORT_LOCAL_CPU;
       r->l3a_intf = 4095;
       r->l3a_flags |= BCM_L3_DST_DISCARD;
       return 0;
     }

  /* Interface valid? */
  if (nh->ifp->type != HSL_IF_TYPE_IP
#ifdef HAVE_MPLS
      && nh->ifp->type != HSL_IF_TYPE_MPLS
#endif /* HAVE_MPLS */
      )
     return -1;

  bcmifp = nh->ifp->system_info;
  if (! bcmifp)
    return -1;

  /* Set L3 interface. */
#ifdef HAVE_MPLS
  if (nh->ifp->type == HSL_IF_TYPE_MPLS)
    r->l3a_intf = bcmifp->u.mpls.ifindex;
  else
#endif /* HAVE_MPLS */
  r->l3a_intf = bcmifp->u.l3.ifindex;

  if (bcmifp->type == HSL_BCM_IF_TYPE_TRUNK)
    {
      /* Egress is a trunk. Set the trunk id. */
      r->l3a_flags |= BCM_L3_TGID;
      r->l3a_trunk = bcmifp->trunk_id;
    }
  else
    {
      /* Egress is a non-trunk port. */
      r->l3a_flags &= ~BCM_L3_TGID;
    }

#ifdef HAVE_L2
  if (! memcmp (nh->ifp->name, "vlan", 4))
    {
      r->l3a_lport = (u_int32_t)nh->system_info;

      /* Get information whether the port is egress tagged. */
      if (bcmifp->type == HSL_BCM_IF_TYPE_TRUNK)
        {
          /* 
             The hierarchy for this case is as follows:

             +-----------+
             |  vlanX.X  |
             +-----------+
             |
             +-----------+
             |   L2 agg  |
             +-----------+
             |
             +-----------+
             |    L2     |
             +-----------+

             For this case, get the L2 port from the lport.
             From lport get L2 agg, 
             From L2 agg, get the hsl_if
             For the hsl_if, find if the VLAN is egress tagged.
          */

          ifp = hsl_bcm_ifmap_if_get (r->l3a_lport);
          if (! ifp)
            {
              HSL_LOG (HSL_LOG_FIB, HSL_LEVEL_ERROR, "Error finding L2 port information for interface %s\n", nh->ifp->name);
              return HSL_FIB_ERR_HW_NH_NOT_FOUND;
            }
          
          /* Its a aggregate, should have only one parent. */
          node = ifp->parent_list;        
          if (! node)
            {
              HSL_LOG (HSL_LOG_FIB, HSL_LEVEL_ERROR, "Error finding L2 port information for interface %s\n", nh->ifp ? nh->ifp->name : "NULL");
              return HSL_FIB_ERR_HW_NH_NOT_FOUND;
            }

          ifp = node->ifp;
          if (! ifp)
            {
              HSL_LOG (HSL_LOG_FIB, HSL_LEVEL_ERROR, "Error finding L2 port information for interface %s\n", nh->ifp ? nh->ifp->name : "NULL");
              return HSL_FIB_ERR_HW_NH_NOT_FOUND;
            }

          tagged = hsl_vlan_get_egress_type (ifp, nh->ifp->u.ip.vid);
          if (tagged < 0)
            {
              HSL_LOG (HSL_LOG_FIB, HSL_LEVEL_ERROR, "Error finding L2 port information for interface %s\n", nh->ifp ? nh->ifp->name : "NULL");
              return HSL_FIB_ERR_HW_NH_NOT_FOUND;    
            }  
        }
      else
        {
          /* 
             This is for the following hierarchy

             +-----------+
             |  vlanX.X  |
             +-----------+
             |
             +-----------+
             |     L2    |
             +-----------+
                 
             For this case, get the L2 port.
             From the L2 port if the VID is tagged/untagged
          */

          ifp = hsl_bcm_ifmap_if_get (r->l3a_lport);
          if (! ifp)
            {
              HSL_LOG (HSL_LOG_FIB, HSL_LEVEL_ERROR, "Error finding L2 port information for interface %s\n", nh->ifp ? nh->ifp->name : "NULL");
              return HSL_FIB_ERR_HW_NH_NOT_FOUND;
            }

          tagged = hsl_vlan_get_egress_type (ifp, nh->ifp->u.ip.vid);
          if (tagged < 0)
            {
              HSL_LOG (HSL_LOG_FIB, HSL_LEVEL_ERROR, "Error finding L2 port information for interface %s\n", nh->ifp ? nh->ifp->name : "NULL");
              return HSL_FIB_ERR_HW_NH_NOT_FOUND;    
            } 
        }

      /* Set tagged/untagged. */
      if (! tagged)
        r->l3a_flags |= BCM_L3_UNTAG;
      else
        r->l3a_flags &= ~BCM_L3_UNTAG;
    }
  else
#endif /* HAVE_L2 */
    {
      ifp = hsl_ifmgr_get_first_L2_port (nh->ifp);
      if (! ifp)
        {
          HSL_LOG (HSL_LOG_FIB, HSL_LEVEL_ERROR, "Error finding L2 port information for interface %s\n", nh->ifp ? nh->ifp->name : "NULL");
          return HSL_FIB_ERR_HW_NH_NOT_FOUND;
        }
      HSL_IFMGR_IF_REF_DEC (ifp);
          
      /* Get the lport. */
      bcmifp = ifp->system_info;
      if (! bcmifp)
        {
          HSL_LOG (HSL_LOG_FIB, HSL_LEVEL_ERROR, "Error finding L2 port information for interface %s\n", nh->ifp ? nh->ifp->name : "NULL");
          return HSL_FIB_ERR_HW_NH_NOT_FOUND;
        }

      r->l3a_lport = bcmifp->u.l2.lport;
    
      /* Pure router port. Egress always untagged. */
      r->l3a_flags |= BCM_L3_UNTAG;
    }  

  return 0;  
}

/* 
   Add prefix. 
*/
int 
hsl_bcm_prefix_add (hsl_fib_id_t fib_id, struct hsl_route_node *rnp, struct hsl_nh_entry *nh)
{
  struct hsl_prefix_entry *pe;
  bcmx_l3_route_t r;
#ifdef HAVE_IPV6
  bcmx_l3_host_t ip;
  char buf[256];
#endif /* HAVE_IPV6 */
  int ret;
  hsl_ipv4Address_t mask;
  bcmx_l3_route_t route;

  HSL_FN_ENTER();

  pe = rnp->info;

#ifdef HAVE_IPV6
  if (rnp->p.family == AF_INET6 && rnp->p.prefixlen == IPV6_MAX_PREFIXLEN)
    {
      /* Looks like broadcom chip requires route with 128 bit prefix len
       * to be added in ipv6 host table instead of ipv6 route table
       */
      ret = _hsl_bcm_get_host_route (fib_id, &rnp->p, nh, &ip, pe);
      if (ret < 0)
        HSL_FN_EXIT (HSL_FIB_ERR_HW_OPERATION_FAILED); 

      ret = bcmx_l3_host_add (&ip);

      if (ret < 0)
        {
          hsl_prefix2str (&rnp->p, buf, sizeof(buf));
          HSL_LOG (HSL_LOG_FIB, HSL_LEVEL_ERROR, "Error adding nexthop %s to hardware\n", buf);
          HSL_FN_EXIT (HSL_FIB_ERR_HW_OPERATION_FAILED); 
        }

      if (pe)
        {
          pe->flags = 0;
          SET_FLAG (pe->flags, HSL_PREFIX_ENTRY_IN_HW);
        }
      return 0;
    }
#endif /* HAVE_IPV6 */

  if (HSL_PREFIX_ENTRY_POPULATED_IN_HW (rnp)
      && CHECK_FLAG (pe->flags, HSL_PREFIX_ENTRY_EXCEPTION))
    {
      if (rnp->p.family == AF_INET)
        {
          /* Initialize route. */
          bcmx_l3_route_t_init (&route);

          hsl_masklen2ip (rnp->p.prefixlen, (hsl_ipv4Address_t *) &mask);
          route.l3a_subnet = rnp->p.u.prefix4;
          route.l3a_ip_mask = mask;
          route.l3a_intf = 0;
          route.l3a_lport = BCMX_LPORT_LOCAL_CPU;
          route.l3a_flags |= BCM_L3_DEFIP_LOCAL;
          route.l3a_flags |= BCM_L3_L2TOCPU; 
       
          /* Delete any previous route. */
          ret = bcmx_l3_route_delete (&route);
          HSL_LOG (HSL_LOG_FIB, HSL_LEVEL_ERROR, 
             "removed  DEFIP exception entry (%x/%x) from  hardware ret = %d\n",
              route.l3a_subnet, route.l3a_ip_mask, ret);
        }
    }

  /* Map route. */
  ret = _hsl_bcm_get_route (fib_id, rnp, nh, &r, HSL_OPER_ADD);
  if (ret < 0)
    return ret;

  /* Add the route. */
  ret = bcmx_l3_route_add (&r);
  if (ret < 0)
    {
      HSL_LOG (HSL_LOG_FIB, HSL_LEVEL_ERROR, "Error adding DEFIP entry (%x/%x) to  hardware ret = %d\n",
               r.l3a_subnet, r.l3a_ip_mask, ret);
      return HSL_FIB_ERR_HW_OPERATION_FAILED;
    }

  if (pe)
    {
      pe->flags = 0;
      SET_FLAG (pe->flags, HSL_PREFIX_ENTRY_IN_HW);
    }

  return 0;
}

/* 
   Delete prefix. 
*/
int
hsl_bcm_prefix_delete (hsl_fib_id_t fib_id, struct hsl_route_node *rnp, struct hsl_nh_entry *nh)
{
  bcmx_l3_route_t r;
#ifdef HAVE_IPV6
  bcmx_l3_host_t ip;
  char buf[256];
#endif /* HAVE_IPV6 */

  int ret;
  hsl_ipv4Address_t mask;
  struct hsl_prefix_entry *pe;

  pe = rnp->info;

#ifdef HAVE_IPV6
  if (rnp->p.family == AF_INET6 && rnp->p.prefixlen == IPV6_MAX_PREFIXLEN)
    {
      ret = _hsl_bcm_get_host (fib_id, &rnp->p, nh, &ip, 0);
      if (ret < 0)
        HSL_FN_EXIT (HSL_FIB_ERR_HW_OPERATION_FAILED); 

      ret = bcmx_l3_host_delete (&ip);

      if ((ret < 0) && (ret != BCM_E_NOT_FOUND)) 
        {
          hsl_prefix2str (&rnp->p, buf, sizeof(buf));
          HSL_LOG (HSL_LOG_FIB, HSL_LEVEL_ERROR, "Error deleting nexthop %s to hardware\n", buf);
          HSL_FN_EXIT (HSL_FIB_ERR_HW_OPERATION_FAILED); 
        }
  
      if (pe)
        {
          pe->flags = 0;
        }
      HSL_FN_EXIT (0); 
    }
#endif /* HAVE_IPV6 */


  if (nh)
    {
      ret = _hsl_bcm_get_route (fib_id, rnp, nh, &r, HSL_OPER_DELETE);
      if (ret < 0)
        HSL_FN_EXIT (ret); 
    }
  else
    {
      bcmx_l3_route_t_init (&r);

      r.l3a_vrf = (bcm_vrf_t)fib_id;

      if (rnp->p.family == AF_INET)
        {
          /* Set prefix. */
          r.l3a_subnet = rnp->p.u.prefix4;
          hsl_masklen2ip (rnp->p.prefixlen, &mask);
          r.l3a_ip_mask = mask;
        }
#ifdef HAVE_IPV6
      else if (rnp->p.family == AF_INET6)
        {
          sal_memcpy (r.l3a_ip6_net, &rnp->p.u.prefix6, BCM_IP6_ADDRLEN);
          
          /* Get mask. */
          bcm_ip6_mask_create (r.l3a_ip6_mask, rnp->p.prefixlen);
          
          /* Set flags for IPv6. */
          r.l3a_flags |= BCM_L3_IP6;
        }
#endif /* HAVE_IPV6 */
      if (CHECK_FLAG (pe->flags, HSL_PREFIX_ENTRY_EXCEPTION))
        {
           r.l3a_intf = 0; /* As we are setting the exception to CPU with 
                              BCM_L3_L2TOCPU, this can be ignored, I guess.*/
           r.l3a_lport = BCMX_LPORT_LOCAL_CPU;
           r.l3a_flags |= BCM_L3_DEFIP_LOCAL;
           r.l3a_flags |= BCM_L3_L2TOCPU; /* To get original headers. */
        }

      r.l3a_lport = BCMX_LPORT_LOCAL_CPU;
    }

  /* Delete the route. */
  ret = bcmx_l3_route_delete (&r);
  if ((ret != BCM_E_NONE) && (ret != BCM_E_NOT_FOUND))
    {
      HSL_LOG (HSL_LOG_FIB, HSL_LEVEL_ERROR, "Error deleting DEFIP entry (%x/%x) from  hardware ret = %d\n",
               r.l3a_subnet, r.l3a_ip_mask, ret);
      return HSL_FIB_ERR_HW_OPERATION_FAILED;
    }

  if (! nh)
    {
      if (pe)
        {
          pe->flags = 0;
        }
    }

  HSL_FN_EXIT (0); 
}

/*
  Set prefix as exception to CPU.
*/
int
hsl_bcm_prefix_exception (hsl_fib_id_t fib_id, struct hsl_route_node *rnp)
{
  bcmx_l3_route_t route;
#ifdef HAVE_IPV6
  bcmx_l3_host_t ip;
#endif /* HAVE_IPV6 */
  hsl_prefix_t *p;
  hsl_ipv4Address_t addr, mask;
  int ret;
  struct hsl_prefix_entry *pe;

  HSL_FN_ENTER ();

  pe = rnp->info;

  p = &rnp->p;

#ifdef HAVE_IPV6
  if (rnp->p.family == AF_INET6 && rnp->p.prefixlen == IPV6_MAX_PREFIXLEN)
    {
      ret = _hsl_bcm_get_host (fib_id, &rnp->p, NULL, &ip, 0);
      if (ret < 0)
        HSL_FN_EXIT (HSL_FIB_ERR_HW_OPERATION_FAILED); 

      ret = bcmx_l3_host_add (&ip);

      if (ret < 0)
        {
          HSL_LOG (HSL_LOG_IFMGR, HSL_LEVEL_ERROR, " Error adding connected route to hardware\n");
          HSL_FN_EXIT (-1);
        }
  
      if (pe)
        {
          pe->flags = 0;
          SET_FLAG (pe->flags, HSL_PREFIX_ENTRY_IN_HW);
          SET_FLAG (pe->flags, HSL_PREFIX_ENTRY_EXCEPTION);
        }
      HSL_FN_EXIT (0);
    }
#endif /* HAVE_IPV6 */

  /* Initialize route. */
  bcmx_l3_route_t_init (&route);

  route.l3a_vrf = (bcm_vrf_t)fib_id;

  if (rnp->p.family == AF_INET)
    {
      hsl_masklen2ip (p->prefixlen, (hsl_ipv4Address_t *) &mask);
      addr = p->u.prefix4;
      addr &= mask;
      route.l3a_subnet = addr;
      route.l3a_ip_mask = mask;
    }
#ifdef HAVE_IPV6
  else
    {

      sal_memcpy (route.l3a_ip6_net, &rnp->p.u.prefix6, BCM_IP6_ADDRLEN);
    
      /* Get mask. */
      bcm_ip6_mask_create (route.l3a_ip6_mask, rnp->p.prefixlen);
      
      /* Set flags for IPv6. */
      route.l3a_flags |= BCM_L3_IP6;
    }
#endif /* HAVE_IPV6 */

  /* Delete any previous route. */
  bcmx_l3_route_delete (&route);

  route.l3a_intf = 0; /* As we are setting the exception to CPU with BCM_L3_L2TOCPU, this can
                         be ignored, I guess. */
  route.l3a_lport = BCMX_LPORT_LOCAL_CPU;
  route.l3a_flags |= BCM_L3_DEFIP_LOCAL;
  route.l3a_flags |= BCM_L3_L2TOCPU; /* To get original headers. */
  
  /* Add connected route to prefix table. */
  ret = bcmx_l3_route_add (&route);
  if (ret < 0)
    {
      HSL_LOG (HSL_LOG_IFMGR, HSL_LEVEL_ERROR, " Error adding exception route to hardware ret = %d\n", ret);
      HSL_FN_EXIT (-1);
    }
#ifdef DEBUG
  else
      HSL_LOG (HSL_LOG_FIB, HSL_LEVEL_INFO, "Successfully added exception route (0x%x/0x%x) to hardware ret = %d\n",
               route.l3a_subnet, route.l3a_ip_mask, ret);
#endif /* DEBUG */


  if (pe)
    {
      SET_FLAG (pe->flags, HSL_PREFIX_ENTRY_IN_HW);
      SET_FLAG (pe->flags, HSL_PREFIX_ENTRY_EXCEPTION);
    }

  HSL_FN_EXIT (0);
}

#ifdef HAVE_IPV6
/*
  Fill up ip host structure.
*/
static int
_hsl_bcm_get_host_route (hsl_fib_id_t fib_id, hsl_prefix_t *p, 
                         struct hsl_nh_entry *nh, bcmx_l3_host_t *ip,
                         struct hsl_prefix_entry *pe)
{
  struct hsl_bcm_if *bcmifp;
  struct hsl_if *ifp;
#ifdef HAVE_L2
  struct hsl_if_list *node = NULL;
  int tagged;
  bcmx_l2_addr_t l2addr;
#endif /* HAVE_L2 */

  if ((!nh) || (!p))
    return HSL_FIB_ERR_INVALID_PARAM;

  /* Initialize host route. */
  bcmx_l3_host_t_init (ip);

  ip->l3a_vrf = (bcm_vrf_t)fib_id;
  
  if (pe && CHECK_FLAG (pe->flags, HSL_PREFIX_ENTRY_IN_HW))
    ip->l3a_flags |= BCM_L3_REPLACE;

  /* Set address. */
  if (p->family == AF_INET)
    {
      ip->l3a_ip_addr = p->u.prefix4; 
    }
#ifdef HAVE_IPV6
  else if (p->family == AF_INET6)
    {

      sal_memcpy (ip->l3a_ip6_addr, &p->u.prefix6, BCM_IP6_ADDRLEN);

     /* Set flags for IPv6. */
      ip->l3a_flags |= BCM_L3_IP6;
    }
#endif /* HAVE_IPV6 */

  /* Set hit flags. */
  ip->l3a_flags |= BCM_L3_HIT;
 
  /* Set nexthop address. */
  memcpy (ip->l3a_nexthop_mac, nh->mac, HSL_ETHER_ALEN);

  /* Add the blackhole route it hw.*/
  /* Note: Currently only EZR supports the DST Discard flag*/

  if (CHECK_FLAG (nh->flags, HSL_NH_ENTRY_BLACKHOLE))
     {
       ip->l3a_lport = BCMX_LPORT_LOCAL_CPU;
       ip->l3a_intf = 4095;
       ip->l3a_flags |= BCM_L3_DST_DISCARD;
       return 0;
     }

  /* Set l3_intf. */
  bcmifp = nh->ifp->system_info;
  if (! bcmifp)
    return -1;

  /* Set L3 interface. */
  ip->l3a_intf = bcmifp->u.l3.ifindex;

#ifdef HAVE_L2
  if (! memcmp (nh->ifp->name, "vlan", 4))
    {
      /* If the NH interface is a vlan IP(SVI) do a L2 lookup to find the lport. */
      if (bcmx_l2_addr_get ((void *) nh->mac, nh->ifp->u.ip.vid, &l2addr, NULL) == 0)
        {
          ip->l3a_lport = l2addr.lport;
        }
      else
        {
          HSL_LOG (HSL_LOG_FIB, HSL_LEVEL_ERROR, "Error finding the NH destination port from L2 table for mac(%02x%02x.%02x%02x.%02x%02x)\n", nh->mac[0], nh->mac[1], nh->mac[2], nh->mac[3], nh->mac[4], nh->mac[5]);
          return HSL_FIB_ERR_HW_NH_NOT_FOUND;
        }

      if (BCMX_LPORT_INVALID == l2addr.lport)
        ifp = hsl_bcm_ifmap_if_get (HSL_BCM_TRUNK_2_LPORT(l2addr.tgid));
      else
        ifp = hsl_bcm_ifmap_if_get (l2addr.lport);
      if (! ifp)
        {
          HSL_LOG (HSL_LOG_FIB, HSL_LEVEL_ERROR, "Error finding L2 port"
                   " information for interface %s\n", nh->ifp->name);
          return HSL_FIB_ERR_HW_NH_NOT_FOUND;
        }
       
       bcmifp = ifp->system_info;
       if (! bcmifp)
         {
           HSL_LOG (HSL_LOG_FIB, HSL_LEVEL_ERROR, "Error finding L2 port"
                    " information for interface %s\n", nh->ifp->name);
           return HSL_FIB_ERR_HW_NH_NOT_FOUND;
         } 
      /* Get information whether the port is egress tagged. */
      if (bcmifp->type == HSL_BCM_IF_TYPE_TRUNK)
        {
          /* 
             The hierarchy for this case is as follows:

             +-----------+
             |  vlanX.X  |
             +-----------+
             |
             +-----------+
             |   L2 agg  |
             +-----------+
             |
             +-----------+
             |    L2     |
             +-----------+

             For this case, get the L2 port from the lport.
             From lport get L2 agg, 
             From L2 agg, get the hsl_if
             For the hsl_if, find if the VLAN is egress tagged.
          */

          /* Its a aggregate, should retreive the children*/
          node = ifp->children_list;      
          if (! node)
            {
              HSL_LOG (HSL_LOG_FIB, HSL_LEVEL_ERROR, "Error finding L2 port information for interface %s\n", nh->ifp->name);
              return HSL_FIB_ERR_HW_NH_NOT_FOUND;
            }

          ifp = node->ifp;
          if (! ifp)
            {
              HSL_LOG (HSL_LOG_FIB, HSL_LEVEL_ERROR, "Error finding L2 port information for interface %s\n", nh->ifp->name);
              return HSL_FIB_ERR_HW_NH_NOT_FOUND;
            }

          tagged = hsl_vlan_get_egress_type (ifp, nh->ifp->u.ip.vid);
          if (tagged < 0)
            {
              HSL_LOG (HSL_LOG_FIB, HSL_LEVEL_ERROR, "Error finding L2 port information for interface %s\n", nh->ifp->name);
              return HSL_FIB_ERR_HW_NH_NOT_FOUND;    
            }  
        }
      else
        {
          /* 
             This is for the following hierarchy
             
             +-----------+
             |  vlanX.X  |
             +-----------+
             |
             +-----------+
             |     L2    |
             +-----------+
                 
             For this case, get the L2 port.
             From the L2 port if the VID is tagged/untagged
          */

          tagged = hsl_vlan_get_egress_type (ifp, nh->ifp->u.ip.vid);
          if (tagged < 0)
            {
              HSL_LOG (HSL_LOG_FIB, HSL_LEVEL_ERROR, "Error finding L2 port information for interface %s\n", nh->ifp->name);
              return HSL_FIB_ERR_HW_NH_NOT_FOUND;    
            } 
        }
      
      /* Set tagged/untagged. */
      if (! tagged)
        ip->l3a_flags |= BCM_L3_UNTAG;
      else
        ip->l3a_flags &= ~BCM_L3_UNTAG;
    }
  else
#endif /* HAVE_L2 */
    {
      ifp = hsl_ifmgr_get_first_L2_port (nh->ifp);
      if (! ifp)
        {
          HSL_LOG (HSL_LOG_FIB, HSL_LEVEL_ERROR, "Error finding L2 port information for interface %s\n", nh->ifp->name);
          return HSL_FIB_ERR_HW_NH_NOT_FOUND;
        }
      HSL_IFMGR_IF_REF_DEC (ifp);
          
      /* Get the lport. */
      bcmifp = ifp->system_info;
      if (! bcmifp)
        {
          HSL_LOG (HSL_LOG_FIB, HSL_LEVEL_ERROR, "Error finding L2 port information for interface %s\n", nh->ifp->name);
          return HSL_FIB_ERR_HW_NH_NOT_FOUND;
        }

      /* Set lport. */
      ip->l3a_lport = bcmifp->u.l2.lport;

      /* Set untagged. */
      ip->l3a_flags |= BCM_L3_UNTAG;
    }  

  if (bcmifp->type == HSL_BCM_IF_TYPE_TRUNK)
    {
      /* Egress is a trunk. Set the trunk id. */
      ip->l3a_flags |= BCM_L3_TGID;
      ip->l3a_trunk = bcmifp->trunk_id;
    }
  else
    {
      /* Egress is a non-trunk port. */
      ip->l3a_flags &= ~BCM_L3_TGID;
    }  

  return 0;
}
#endif /* HAVE_IPV6 */
/*
  Fill up ip host route.
*/
static int
_hsl_bcm_nh_get (hsl_fib_id_t fib_id, struct hsl_nh_entry *nh, bcmx_l3_host_t *ip)
{
  struct hsl_bcm_if *bcmifp;
  struct hsl_if *ifp;
#ifdef HAVE_L2
  struct hsl_if_list *node = NULL;
  int tagged;
  bcmx_l2_addr_t l2addr;
#endif /* HAVE_L2 */

  if (! nh)
    return HSL_FIB_ERR_INVALID_PARAM;

  /* Initialize host route. */
  bcmx_l3_host_t_init (ip);

  ip->l3a_vrf = (bcm_vrf_t)fib_id;

  if (CHECK_FLAG (nh->ext_flags, HSL_NH_ENTRY_EFLAG_IN_HW))
    ip->l3a_flags |= BCM_L3_REPLACE;

  /* Set address. */
  if (nh->rn->p.family == AF_INET)
    {
      ip->l3a_ip_addr = nh->rn->p.u.prefix4; 
    }
#ifdef HAVE_IPV6
  else if (nh->rn->p.family == AF_INET6)
    {
      sal_memcpy (ip->l3a_ip6_addr, &nh->rn->p.u.prefix6, BCM_IP6_ADDRLEN);

      /* Set flags for IPv6. */
      ip->l3a_flags |= BCM_L3_IP6;
    }
#endif /* HAVE_IPV6 */

  /* Set hit flags. */
  ip->l3a_flags |= BCM_L3_HIT;
 
  /* Set nexthop address. */
  memcpy (ip->l3a_nexthop_mac, nh->mac, HSL_ETHER_ALEN);

  /* Add the blackhole route it hw.*/
  /* Note: Currently only EZR supports the DST Discard flag*/

  if (CHECK_FLAG (nh->flags, HSL_NH_ENTRY_BLACKHOLE))
     {
       ip->l3a_lport = BCMX_LPORT_LOCAL_CPU;
       ip->l3a_intf = 4095;
       ip->l3a_flags |= BCM_L3_DST_DISCARD;
       return 0;
     }

  /* Set l3_intf. */
  bcmifp = nh->ifp->system_info;
  if (! bcmifp)
    return -1;

  /* Set L3 interface. */
  ip->l3a_intf = bcmifp->u.l3.ifindex;

#ifdef HAVE_L2
  if (! memcmp (nh->ifp->name, "vlan", 4))
    {
      /* If the NH interface is a vlan IP(SVI) do a L2 lookup to find the lport. */
      if (bcmx_l2_addr_get ((void *) nh->mac, nh->ifp->u.ip.vid, &l2addr, NULL) == 0)
        {
          ip->l3a_lport = l2addr.lport;
          nh->system_info = (void *)l2addr.lport;
        }
      else
        {
          HSL_LOG (HSL_LOG_FIB, HSL_LEVEL_ERROR, "Error finding the NH destination port from L2 table for mac(%02x%02x.%02x%02x.%02x%02x)\n", nh->mac[0], nh->mac[1], nh->mac[2], nh->mac[3], nh->mac[4], nh->mac[5]);

          nh->system_info = (void *)BCMX_LPORT_INVALID;

          return HSL_FIB_ERR_HW_NH_NOT_FOUND;
        }

      if (BCMX_LPORT_INVALID == l2addr.lport)
        ifp = hsl_bcm_ifmap_if_get (HSL_BCM_TRUNK_2_LPORT(l2addr.tgid));
      else
        ifp = hsl_bcm_ifmap_if_get (l2addr.lport);
      if (! ifp)
        {
          HSL_LOG (HSL_LOG_FIB, HSL_LEVEL_ERROR, "Error finding L2 port"
                   " information for interface %s\n", nh->ifp->name);
          return HSL_FIB_ERR_HW_NH_NOT_FOUND;
        }
       
       bcmifp = ifp->system_info;
       if (! bcmifp)
         {
           HSL_LOG (HSL_LOG_FIB, HSL_LEVEL_ERROR, "Error finding L2 port"
                    " information for interface %s\n", nh->ifp->name);
           return HSL_FIB_ERR_HW_NH_NOT_FOUND;
         } 
      /* Get information whether the port is egress tagged. */
      if (bcmifp->type == HSL_BCM_IF_TYPE_TRUNK)
        {
          /* 
             The hierarchy for this case is as follows:

             +-----------+
             |  vlanX.X  |
             +-----------+
             |
             +-----------+
             |   L2 agg  |
             +-----------+
             |
             +-----------+
             |    L2     |
             +-----------+

             For this case, get the L2 port from the lport.
             From lport get L2 agg, 
             From L2 agg, get the hsl_if
             For the hsl_if, find if the VLAN is egress tagged.
          */

          /* Its a aggregate, should retreive the children*/
          node = ifp->children_list;      
          if (! node)
            {
              HSL_LOG (HSL_LOG_FIB, HSL_LEVEL_ERROR, "Error finding L2 port information for interface %s\n", nh->ifp->name);
              return HSL_FIB_ERR_HW_NH_NOT_FOUND;
            }

          ifp = node->ifp;
          if (! ifp)
            {
              HSL_LOG (HSL_LOG_FIB, HSL_LEVEL_ERROR, "Error finding L2 port information for interface %s\n", nh->ifp->name);
              return HSL_FIB_ERR_HW_NH_NOT_FOUND;
            }

          tagged = hsl_vlan_get_egress_type (ifp, nh->ifp->u.ip.vid);
          if (tagged < 0)
            {
              HSL_LOG (HSL_LOG_FIB, HSL_LEVEL_ERROR, "Error finding L2 port information for interface %s\n", nh->ifp->name);
              return HSL_FIB_ERR_HW_NH_NOT_FOUND;    
            }  
        }
      else
        {
          /* 
             This is for the following hierarchy
             
             +-----------+
             |  vlanX.X  |
             +-----------+
             |
             +-----------+
             |     L2    |
             +-----------+
                 
             For this case, get the L2 port.
             From the L2 port if the VID is tagged/untagged
          */

          tagged = hsl_vlan_get_egress_type (ifp, nh->ifp->u.ip.vid);
          if (tagged < 0)
            {
              HSL_LOG (HSL_LOG_FIB, HSL_LEVEL_ERROR, "Error finding L2 port information for interface %s\n", nh->ifp->name);
              return HSL_FIB_ERR_HW_NH_NOT_FOUND;    
            } 
        }
      
      /* Set tagged/untagged. */
      if (! tagged)
        ip->l3a_flags |= BCM_L3_UNTAG;
      else
        ip->l3a_flags &= ~BCM_L3_UNTAG;
    }
  else
#endif /* HAVE_L2 */
    {
      ifp = hsl_ifmgr_get_first_L2_port (nh->ifp);
      if (! ifp)
        {
          HSL_LOG (HSL_LOG_FIB, HSL_LEVEL_ERROR, "Error finding L2 port information for interface %s\n", nh->ifp->name);
          return HSL_FIB_ERR_HW_NH_NOT_FOUND;
        }
      HSL_IFMGR_IF_REF_DEC (ifp);
          
      /* Get the lport. */
      bcmifp = ifp->system_info;
      if (! bcmifp)
        {
          HSL_LOG (HSL_LOG_FIB, HSL_LEVEL_ERROR, "Error finding L2 port information for interface %s\n", nh->ifp->name);
          return HSL_FIB_ERR_HW_NH_NOT_FOUND;
        }

      /* Set lport. */
      ip->l3a_lport = bcmifp->u.l2.lport;

      /* Set untagged. */
      ip->l3a_flags |= BCM_L3_UNTAG;
    }  

  if (bcmifp->type == HSL_BCM_IF_TYPE_TRUNK)
    {
      /* Egress is a trunk. Set the trunk id. */
      ip->l3a_flags |= BCM_L3_TGID;
      ip->l3a_trunk = bcmifp->trunk_id;
    }
  else
    {
      /* Egress is a non-trunk port. */
      ip->l3a_flags &= ~BCM_L3_TGID;
    }  

  return 0;
}

/* 
   Add nexthop. 
*/
int 
hsl_bcm_nh_add (hsl_fib_id_t fib_id, struct hsl_nh_entry *nh)
{
  bcmx_l3_host_t ip;
  int ret;
  char buf[256];
  struct hsl_avl_node *node;
  struct hsl_route_node *rnp;

  HSL_FN_ENTER ();

  if (! nh)
    HSL_FN_EXIT (HSL_FIB_ERR_INVALID_PARAM); 

  /* Map nexthop. */
  ret = _hsl_bcm_nh_get (fib_id, nh, &ip);
  if (ret < 0)
    HSL_FN_EXIT (ret);

  /* Add the host route. */
  ret = bcmx_l3_host_add (&ip);
  if (ret < 0)
    {
      hsl_prefix2str (&nh->rn->p, buf, sizeof(buf));     
      HSL_LOG (HSL_LOG_FIB, HSL_LEVEL_ERROR, "Error adding nexthop %s from hardware ret = %d\n", buf, ret);
      HSL_FN_EXIT (HSL_FIB_ERR_HW_OPERATION_FAILED); 
    }

  SET_FLAG (nh->ext_flags, HSL_NH_ENTRY_EFLAG_IN_HW);

  /* Activate all prefixes dependant on this NH. */
  for (node = hsl_avl_top (nh->prefix_tree); node; node = hsl_avl_next (node))
    {
      rnp = HSL_AVL_NODE_INFO (node);

      /* Add prefix. */
      hsl_bcm_prefix_add (fib_id, rnp, nh);
    }

  HSL_FN_EXIT (0);
}

/*
  Nexthop delete. 
*/
int 
hsl_bcm_nh_delete (hsl_fib_id_t fib_id, struct hsl_nh_entry *nh)
{
  int ret;
  char buf[256];
  bcmx_l3_host_t ip;
  struct hsl_bcm_if *bcmifp;

  HSL_FN_ENTER ();

  if (! nh)
    return HSL_FIB_ERR_INVALID_PARAM;

  /* Initialize host route. */
  bcmx_l3_host_t_init (&ip);

  if (! CHECK_FLAG (nh->flags, HSL_NH_ENTRY_BLACKHOLE))
    {
      bcmifp = (struct hsl_bcm_if *) nh->ifp->system_info;
      ip.l3a_vrf = (bcm_vrf_t)fib_id;
      ip.l3a_intf = bcmifp->u.l3.ifindex;
      ip.l3a_lport = BCMX_LPORT_LOCAL_CPU;
      ip.l3a_flags |= BCM_L3_L2TOCPU;   /* To get original headers. */
      ip.l3a_flags |= BCM_L3_REPLACE;   /* clear next hop info */
    }
  else
    {
      ip.l3a_lport = BCMX_LPORT_LOCAL_CPU;
      ip.l3a_intf =4095;
      ip.l3a_flags |= BCM_L3_DST_DISCARD;
    }

  if (nh->rn->p.family == AF_INET)
    {
      ip.l3a_ip_addr = nh->rn->p.u.prefix4; 
    }
#ifdef HAVE_IPV6
  else if (nh->rn->p.family == AF_INET6)
    {
      sal_memcpy (ip.l3a_ip6_addr, &nh->rn->p.u.prefix6, BCM_IP6_ADDRLEN);

      ip.l3a_flags |= BCM_L3_IP6;
    }
#endif /* HAVE_IPV6 */


  nh->system_info = (void *)BCMX_LPORT_INVALID;

  /* Delete the host route. */
  ret = bcmx_l3_host_delete (&ip);
  if ((ret < 0) && (ret != BCM_E_NOT_FOUND)) 
    {
      hsl_prefix2str (&nh->rn->p, buf, sizeof(buf));     
      HSL_LOG (HSL_LOG_FIB, HSL_LEVEL_ERROR, "Error delete nexthop %s from hardware ret = %d\n", buf, ret);
      return HSL_FIB_ERR_HW_OPERATION_FAILED;
    }

  UNSET_FLAG (nh->ext_flags, HSL_NH_ENTRY_EFLAG_IN_HW);

  HSL_FN_EXIT (0);
}

/*
  Get maximum number of multipaths. 
*/
int
hsl_bcm_get_max_multipath(u_int32_t *ecmp)
{
   HSL_FN_ENTER();

   if(!ecmp)
     HSL_FN_EXIT(HSL_FIB_ERR_INVALID_PARAM);
 
   *ecmp = 8;

   HSL_FN_EXIT(STATUS_OK); 
}


/*
  Check if nexthop entry has been hit.
*/
int
hsl_bcm_nh_hit (hsl_fib_id_t fib_id, struct hsl_nh_entry *nh)
{
  bcmx_l3_host_t ip;
  int ret;

  /* Initialize host route. */
  bcmx_l3_host_t_init (&ip);

  ip.l3a_vrf = (bcm_vrf_t)fib_id;

  /* Clearing l3 hit bit here might lead to hit bit being cleared when router
   * is a transit router for the traffic and thus the entry will be aged out.
   * ARP must be relearned for hardware to avoid traffic going to CPU
   * (in slow path). ARP (control) packets need to be prioritized to/from CPU
   * to handle the case of wire rate traffic blocking ARP learning and causing
   * all packets going to CPU slow path.
   *
   * Not clearing will keep stale l3 arp entries forever in l3 table and
   * may cause l3 table full with stale entries.
   * Hence clearing is necessary here (Defect Id PacOS00031567) though one can
   * use static arp to avoid ARP timeout and slow path issue.
   */
  ip.l3a_flags |= BCM_L3_HIT_CLEAR;

  if (nh->rn->p.family == AF_INET)
    {
      ip.l3a_ip_addr = nh->rn->p.u.prefix4;
    }
#ifdef HAVE_IPV6
  else
    {
      sal_memcpy (ip.l3a_ip6_addr, &nh->rn->p.u.prefix6, BCM_IP6_ADDRLEN);
   
      ip.l3a_flags |= BCM_L3_IP6;
    }
#endif /* HAVE_IPV6 */

  /* Find host route. */
  ret = bcmx_l3_host_find (&ip);
  if (ret < 0)
    return 0; /* Invalid. */

  if (ip.l3a_flags & BCM_L3_HIT)
    return 1; /* Valid. */
  else
    return 0; /* Invalid. */
}

/*
  Add connected route as exception to prefix table.
*/
int
hsl_bcm_add_connected_route (hsl_fib_id_t fib_id, struct hsl_route_node *rnp, struct hsl_if *ifp)
{
  bcmx_l3_route_t route;
  hsl_prefix_t *prefix;
  struct hsl_prefix_entry *pe;
  hsl_ipv4Address_t addr, mask;
  int ret;
#ifdef HAVE_IPV6
  bcmx_l3_host_t ip;
  hsl_prefix_t p;
#endif /* HAVE_IPV6 */

  HSL_FN_ENTER();

  prefix = &rnp->p;

  pe = rnp->info;

  /* Reject lookback address. */
  if (prefix->family == AF_INET && prefix->u.prefix4 == INADDR_LOOPBACK)
      return 0;

  /* For all other connected addresses add a prefix route going to the
     CPU. */
#ifdef HAVE_IPV6
  /* For IPV6_MAX_PREFIXLEN, add to host table since for easyrider box,
     prefix table do not allow IPV6_MAX_PREFIXLEN route to be added. */
  if (prefix->family == AF_INET6 &&
      prefix->prefixlen == IPV6_MAX_PREFIXLEN)
    {
      /* No need to add a linklocal route */
      if (IPV6_IS_ADDR_LINKLOCAL (&prefix->u.prefix6))
        {
          if (pe)
            {
              pe->flags = 0;
              SET_FLAG (pe->flags, HSL_PREFIX_ENTRY_IN_HW);
            }
          return 0;
        }

      ret = _hsl_bcm_get_host (fib_id, prefix, NULL, &ip, 1);
      if (ret < 0)
        return -1;                                                                          
      ret = bcmx_l3_host_add (&ip);
                                                                                
      if ((ret < 0) && (ret != BCM_E_EXISTS))
        {
          HSL_LOG (HSL_LOG_IFMGR, HSL_LEVEL_ERROR, " Error adding connected route to hardware: %s\n", bcm_errmsg(ret));
          return -1;
        }

      if (pe)
        {
          pe->flags = 0;
          SET_FLAG (pe->flags, HSL_PREFIX_ENTRY_IN_HW);
        }

      return 0;
    }
#endif /* HAVE_IPV6 */

  /* Initialize route. */
  bcmx_l3_route_t_init (&route);

  route.l3a_vrf = (bcm_vrf_t)fib_id;

  if (prefix->family == AF_INET)
    {
      hsl_masklen2ip (prefix->prefixlen, (hsl_ipv4Address_t *) &mask);
      addr = prefix->u.prefix4;
      addr &= mask;
      route.l3a_subnet = addr;
      route.l3a_ip_mask = mask;
    }
#ifdef HAVE_IPV6
  else if (prefix->family == AF_INET6)
    {
      /* No need to add a linklocal route */
      if (IPV6_IS_ADDR_LINKLOCAL (&prefix->u.prefix6))
        {
          if (pe)
            {
              pe->flags = 0;
              SET_FLAG (pe->flags, HSL_PREFIX_ENTRY_IN_HW);
            }
          return 0;
        }

      /* Apply mask to prefix. */
      memcpy (&p, prefix, sizeof (hsl_prefix_t));
      hsl_apply_mask_ipv6 (&p);

      sal_memcpy (route.l3a_ip6_net, &p.u.prefix6, BCM_IP6_ADDRLEN);

      bcm_ip6_mask_create (route.l3a_ip6_mask, p.prefixlen);
      
      /* Set flags for IPv6. */
      route.l3a_flags |= BCM_L3_IP6;
    }
#endif /* HAVE_IPV6 */

  route.l3a_lport = BCMX_LPORT_LOCAL_CPU;
  route.l3a_flags |= BCM_L3_DEFIP_LOCAL;
  route.l3a_flags |= BCM_L3_L2TOCPU; /* To get original headers. */

  /* Add connected route to prefix table. */
  ret = bcmx_l3_route_add (&route);
  if ((ret < 0) && (ret != BCM_E_EXISTS)) 
    {
      HSL_LOG (HSL_LOG_IFMGR, HSL_LEVEL_ERROR, " Error adding connected route to hardware: %s\n", bcm_errmsg(ret));
      return -1;
    }

  if (pe)
    {
       pe->flags = 0;
       SET_FLAG (pe->flags, HSL_PREFIX_ENTRY_IN_HW);
    }

  return 0;
}

/* 
   Delete connected route as exception to prefix table.
*/
int
hsl_bcm_delete_connected_route (hsl_fib_id_t fib_id, struct hsl_route_node *rnp, struct hsl_if *ifp)
{
  hsl_prefix_t *prefix;
  struct hsl_prefix_entry *pe;
  bcmx_l3_route_t route;
  hsl_ipv4Address_t addr, mask;
  int ret;
#ifdef HAVE_IPV6
  bcmx_l3_host_t ip;
  hsl_prefix_t p;
#endif /* HAVE_IPV6 */

  prefix = &rnp->p;

  pe = rnp->info;

  /* For all other connected addresses delete the prefix route going to the
     CPU. */
#ifdef HAVE_IPV6
  /* For IPV6_MAX_PREFIXLEN, delete to host table since for easyrider box,
     prefix table do not allow IPV6_MAX_PREFIXLEN route to be added. */
  if (prefix->family == AF_INET6 &&
      prefix->prefixlen == IPV6_MAX_PREFIXLEN)
    {
      /* No need to add a linklocal route */
      if (IPV6_IS_ADDR_LINKLOCAL (&prefix->u.prefix6))
        {
          if (pe)
            pe->flags = 0;
          return 0;
        }

      ret = _hsl_bcm_get_host (fib_id, prefix, NULL, &ip, 1);
      if (ret < 0)
        return -1;

      ret = bcmx_l3_host_delete (&ip);
 
      if ((ret != BCM_E_NONE) && (ret != BCM_E_NOT_FOUND))
        {
          HSL_LOG (HSL_LOG_IFMGR, HSL_LEVEL_ERROR, " Error deleting connected route to hardware: %s\n", bcm_errmsg(ret));
          return -1;
        }
      if (pe)
        pe->flags = 0;

      return 0;
    }
#endif /* HAVE_IPV6 */

  /* Initialize route. */
  bcmx_l3_route_t_init (&route);

  route.l3a_vrf = (bcm_vrf_t)fib_id;

  if (prefix->family == AF_INET)
    {
      hsl_masklen2ip (prefix->prefixlen, (hsl_ipv4Address_t *) &mask);
      addr = prefix->u.prefix4;
      addr &= mask;
      route.l3a_subnet = addr;
      route.l3a_ip_mask = mask;
    }
#ifdef HAVE_IPV6
  else if (prefix->family == AF_INET6)
    {
      /* No need to add a linklocal route */
      if (IPV6_IS_ADDR_LINKLOCAL (&prefix->u.prefix6))
        {
          if (pe)
            pe->flags = 0;
          return 0;
        }

      /* Apply mask to prefix. */
      memcpy (&p, prefix, sizeof (hsl_prefix_t));
      hsl_apply_mask_ipv6 (&p);

      sal_memcpy (route.l3a_ip6_net, &p.u.prefix6, BCM_IP6_ADDRLEN);

      bcm_ip6_mask_create (route.l3a_ip6_mask, p.prefixlen);

      /* Set flags for IPv6. */
      route.l3a_flags |= BCM_L3_IP6;
    }
#endif /* HAVE_IPV6 */

  route.l3a_lport = BCMX_LPORT_LOCAL_CPU;
  route.l3a_flags |= BCM_L3_L2TOCPU; /* To get original headers. */
  route.l3a_flags |= BCM_L3_DEFIP_LOCAL;

  /* Delete connected route to prefix table. */
  ret = bcmx_l3_route_delete (&route);
  if ((ret != BCM_E_NONE) && (ret != BCM_E_NOT_FOUND)) 
    {
      HSL_LOG (HSL_LOG_IFMGR, HSL_LEVEL_ERROR, " Error deleting connected route from hardware\n");
      return -1;
    }

  if (pe)
    pe->flags = 0;

  return 0;
}

/*
  Register callbacks.
*/
void
hsl_fib_hw_cb_register (void)
{
  hsl_bcm_fib_callbacks.hw_fib_init = hsl_bcm_fib_init;
  hsl_bcm_fib_callbacks.hw_fib_deinit = hsl_bcm_fib_deinit;
  hsl_bcm_fib_callbacks.hw_fib_dump = hsl_bcm_fib_dump;
  hsl_bcm_fib_callbacks.hw_prefix_add = hsl_bcm_prefix_add;
  hsl_bcm_fib_callbacks.hw_prefix_delete = hsl_bcm_prefix_delete;
  hsl_bcm_fib_callbacks.hw_prefix_add_exception = hsl_bcm_prefix_exception;
  hsl_bcm_fib_callbacks.hw_nh_add = hsl_bcm_nh_add;
  hsl_bcm_fib_callbacks.hw_nh_delete = hsl_bcm_nh_delete;
  hsl_bcm_fib_callbacks.hw_nh_hit = hsl_bcm_nh_hit;
  hsl_bcm_fib_callbacks.hw_add_connected_route = hsl_bcm_add_connected_route;
  hsl_bcm_fib_callbacks.hw_delete_connected_route = hsl_bcm_delete_connected_route;
  hsl_bcm_fib_callbacks.hw_get_max_multipath = hsl_bcm_get_max_multipath;

  hsl_fibmgr_hw_cb_register (&hsl_bcm_fib_callbacks);
}

