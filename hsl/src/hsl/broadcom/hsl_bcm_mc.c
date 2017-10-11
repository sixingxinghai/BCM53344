/* Copyright (C) 2004  All Rights Reserved. */

#include "config.h"
#include "hsl_os.h"

#if defined (HAVE_MCAST_IPV4) || defined (HAVE_MCAST_IPV6) || defined (HAVE_IGMP_SNOOP)

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
#include "hsl_mcast_fib.h"
#include "hsl_fib_mc_hw.h"
#include "hsl_bcm_mc.h"
#include "hsl_bcm.h"

#ifdef HAVE_L2
#include "hal_types.h"
#include "hal_l2.h"
#include "hal_msg.h"

#include "hsl_vlan.h"
#include "hsl_bridge.h"
#endif /* HAVE_L2 */

static struct hsl_fib_mc_hw_callbacks hsl_bcm_ipv4_mc_callbacks;

#ifdef HAVE_IPV6
static struct hsl_fib_ipv6_mc_hw_callbacks hsl_bcm_ipv6_mc_callbacks;
#endif /* HAVE_IPV6 */

#ifdef  HAVE_MCAST_IPV4
int hsl_mc_ipv4_get_vif_ifindex(u_int32_t vifindex,hsl_ifIndex_t *ifindex);
int hsl_ipv4_get_registered_vif();
static u_int8_t _hsl_bcm_ipv4_mc_enable_flag = HSL_FALSE;
#endif /*  HAVE_MCAST_IPV4 */

#ifdef  HAVE_MCAST_IPV6
static u_int8_t _hsl_bcm_ipv6_mc_enable_flag = HSL_FALSE;
#endif /*  HAVE_MCAST_IPV6 */

static int 
_hsl_bcm_mc_route_init(bcmx_ipmc_addr_t *p_mc_route, 
                       struct hsl_mc_fib_entry *p_grp_src_entry);

#if defined (HAVE_MCAST_IPV4) || defined (HAVE_MCAST_IPV6)

int hsl_bcm_mc_vif_add (struct hsl_if *ifp, u_int16_t flags)
{
  HSL_FN_ENTER();
  HSL_FN_EXIT(STATUS_OK);
}

int hsl_bcm_mc_vif_del(struct hsl_if *ifp)
{
  HSL_FN_ENTER(); 

  if(!ifp)
    HSL_FN_EXIT(STATUS_ERROR);

  HSL_FN_EXIT(STATUS_OK);
}

/* Common function for getting statistics. */
static int
_hsl_bcm_mc_sg_stat (bcmx_ipmc_addr_t *p_mc_route, u_int32_t *pktcnt,
                     u_int32_t *bytecnt, u_int32_t *wrong_if)
{
  int ret;

  /* Clear the hit bit after we check it */
  p_mc_route->flags |= BCM_IPMC_HIT_CLEAR;

  /* Find entry. */
  ret = bcmx_ipmc_find (p_mc_route);
  if (ret < 0)
    HSL_FN_EXIT(STATUS_OK);

  if ((ret == BCM_E_NONE) && (p_mc_route->flags & BCM_IPMC_HIT))
    {
      if (pktcnt) ((*pktcnt)++);
      if (bytecnt) ((*bytecnt)++);
    }

  HSL_FN_EXIT (0);
}

/* Get multicast route usage  statistics */ 
int hsl_bcm_mc_sg_stat (struct hsl_mc_fib_entry *p_grp_src_entry,
                             u_int32_t *pktcnt, u_int32_t *bytecnt, u_int32_t *wrong_if)
{
  bcmx_ipmc_addr_t mc_route;
  int ret;

  HSL_FN_ENTER(); 
  
  if(!p_grp_src_entry)
    HSL_FN_EXIT(HSL_FIB_ERR_INVALID_PARAM);
 
  /* Init mroute info. */
  ret = _hsl_bcm_mc_route_init(&mc_route,p_grp_src_entry);
  if(ret < 0)
     HSL_FN_EXIT (-1);

  /* Find entry and update stat. */
  ret = _hsl_bcm_mc_sg_stat (&mc_route, pktcnt, bytecnt, wrong_if);

  /* Free entry. */
  bcmx_ipmc_addr_t_free (&mc_route);

  HSL_FN_EXIT(ret);
}

#ifdef HAVE_MCAST_IPV4
/*
  Enable/Disable ipv4 multicast.
*/
int
_hsl_bcm_ipv4_mc_enable (HSL_BOOL enable)
{
  int ret;
  
  HSL_FN_ENTER();
  if ( enable == HSL_TRUE )
    {
      /* If IPv4 or IPv6 multicast has been enabled then do nothing */
      if (_hsl_bcm_ipv4_mc_enable_flag == HSL_TRUE)
        HSL_FN_EXIT(STATUS_OK);

#ifdef HAVE_MCAST_IPV6
      if (_hsl_bcm_ipv6_mc_enable_flag == HSL_TRUE)
        {
          _hsl_bcm_ipv4_mc_enable_flag = HSL_TRUE;
        }
#endif /* HAVE_MCAST_IPV6 */

      ret = bcmx_ipmc_init();
      if ( ret < 0 )
        {
          HSL_LOG (HSL_LOG_FIB, HSL_LEVEL_ERROR, 
                   "Error IPMC Initialisation failed\n");
          HSL_FN_EXIT(ret);
        }
    }
  else
    {
      /* If IPv4 MC is already disabled, do nothing */
      if (_hsl_bcm_ipv4_mc_enable_flag == HSL_FALSE)
        HSL_FN_EXIT(STATUS_OK);

#ifdef HAVE_MCAST_IPV6
      /* If IPv6 multicast is enabled then just flag IPv4 MC 
       * as disabled*/
      if (_hsl_bcm_ipv6_mc_enable_flag == HSL_TRUE)
        {
          _hsl_bcm_ipv4_mc_enable_flag = HSL_FALSE; 
        }
#endif /* HAVE_MCAST_IPV6 */
    }
  
  ret = bcmx_ipmc_enable(enable);
  if ( ret < 0 )
    {
      HSL_LOG (HSL_LOG_FIB, HSL_LEVEL_ERROR, 
               "Error enable/disable (%d) IPMC failed\n", enable);
      HSL_FN_EXIT(ret);
    }

  if (enable == HSL_TRUE)
    {
      _hsl_bcm_ipv4_mc_enable_flag = HSL_TRUE;
    }
  else
    {
      ret = bcmx_ipmc_detach();
      if ( ret < 0 )
        {
          HSL_LOG (HSL_LOG_FIB, HSL_LEVEL_ERROR, 
                   "Error IPMC Detach failed\n");
          HSL_FN_EXIT(ret);
        }
      _hsl_bcm_ipv4_mc_enable_flag = HSL_FALSE;
    }

  HSL_FN_EXIT(STATUS_OK);
}

/* Init/Enable ipv4 multicast. */ 
int
hsl_bcm_ipv4_mc_init()
{
  HSL_FN_ENTER();
  HSL_FN_EXIT(_hsl_bcm_ipv4_mc_enable(HSL_TRUE));
}

/* Deinit ipv4 multicast. */ 
int 
hsl_bcm_ipv4_mc_deinit()
{
  HSL_FN_ENTER();
  HSL_FN_EXIT(_hsl_bcm_ipv4_mc_enable(HSL_FALSE));
}

#endif /* HAVE_MCAST_IPV4 */

#ifdef HAVE_MCAST_IPV6
/*
  Enable/Disable ipv6 multicast.
*/
int
_hsl_bcm_ipv6_mc_enable (HSL_BOOL enable)
{
  int ret;
  
  HSL_FN_ENTER();
  if ( enable == HSL_TRUE )
    {
      /* If IPv4 or IPv6 multicast has been enabled then do nothing */
      if (_hsl_bcm_ipv6_mc_enable_flag == HSL_TRUE)
        HSL_FN_EXIT(STATUS_OK);

#ifdef HAVE_MCAST_IPV4
      if (_hsl_bcm_ipv4_mc_enable_flag == HSL_TRUE)
        {
          _hsl_bcm_ipv6_mc_enable_flag = HSL_TRUE;
        }
#endif /* HAVE_MCAST_IPV4 */


      ret = bcmx_ipmc_init();
      if ( ret < 0 )
        {
          HSL_LOG (HSL_LOG_FIB, HSL_LEVEL_ERROR, 
                   "Error IPv6 IPMC Initialisation failed\n");
          HSL_FN_EXIT(ret);
        }
    }
  else
    {
      /* If IPv6 MC is already disabled, do nothing */
      if (_hsl_bcm_ipv6_mc_enable_flag == HSL_FALSE)
        HSL_FN_EXIT(STATUS_OK);

#ifdef HAVE_MCAST_IPV4
      /* If IPv4 multicast is enabled then just flag IPv6 MC 
       * as disabled*/
      if (_hsl_bcm_ipv4_mc_enable_flag == HSL_TRUE)
        {
          _hsl_bcm_ipv6_mc_enable_flag = HSL_FALSE; 
        }
#endif /* HAVE_MCAST_IPV4 */
    }
  
  ret = bcmx_ipmc_enable(enable);
  if ( ret < 0 )
    {
      HSL_LOG (HSL_LOG_FIB, HSL_LEVEL_ERROR, 
               "Error enable/disable (%d) IPv6 IPMC failed\n", enable);
      HSL_FN_EXIT(ret);
    }

  if (enable == HSL_TRUE)
    {
      _hsl_bcm_ipv6_mc_enable_flag = HSL_TRUE;
    }
  else
    {
      ret = bcmx_ipmc_detach();
      if ( ret < 0 )
        {
          HSL_LOG (HSL_LOG_FIB, HSL_LEVEL_ERROR, 
                   "Error IPv6 IPMC Detach failed\n");
          HSL_FN_EXIT(ret);
        }
      _hsl_bcm_ipv6_mc_enable_flag = HSL_FALSE;
    }
  
  HSL_FN_EXIT(STATUS_OK);
}

/* Init/Enable ipv6 multicast. */ 
int
hsl_bcm_ipv6_mc_init()
{
  HSL_FN_ENTER();
  HSL_FN_EXIT(_hsl_bcm_ipv6_mc_enable (HSL_TRUE));
}

/* Deinit ipv6 multicast. */ 
int 
hsl_bcm_ipv6_mc_deinit()
{
  HSL_FN_ENTER();
  HSL_FN_EXIT(_hsl_bcm_ipv6_mc_enable (HSL_FALSE));
}
#endif /* HAVE_MCAST_IPV6 */


#endif /* (HAVE_MCAST_IPV4) || defined (HAVE_MCAST_IPV6) */                    

static int 
_hsl_bcm_mc_route_init(bcmx_ipmc_addr_t *p_mc_route, 
                                struct hsl_mc_fib_entry *p_grp_src_entry)
{
    struct hsl_if *ifp;                 /* Incoming interface pointer. */
    struct hsl_if *in_l2_ifp;           /* Layer 2 interface pointer.  */ 
    struct hsl_bcm_if *in_bcmifp_l2;    /* Layer 2 interface bcm info. */
    struct hsl_bcm_if *in_bcmifp_l3;    /* Incoming interface bcm info.*/   


    HSL_FN_ENTER();

    if((!p_mc_route) || (!p_grp_src_entry))
        HSL_FN_EXIT(STATUS_WRONG_PARAMS);
    if(AF_INET == HSL_MCAST_FIB_GROUP_FAMILY)
    {
#if 0 
        bcmx_ipmc_addr_t_init (p_mc_route, HSL_MCAST_FIB_SRC4, 
                               HSL_MCAST_FIB_GROUP4, 0);
#else
        
        bcmx_ipmc_addr_t_init (p_mc_route);
        /* Copy IPv6 addresses */
        IPV4_ADDR_COPY (&p_mc_route->s_ip_addr, &HSL_MCAST_FIB_SRC4);
        IPV4_ADDR_COPY (&p_mc_route->mc_ip_addr, &HSL_MCAST_FIB_GROUP4);        
#endif
    }
#ifdef HAVE_IPV6
    else 
    {
#if 0 
        /* Initialize entry. */  
        bcmx_ipmc_addr_t_init (p_mc_route, 0, 0, 0);
#else
        bcmx_ipmc_addr_t_init (p_mc_route);
#endif
        /* Copy IPv6 addresses */
        IPV6_ADDR_COPY (&p_mc_route->s_ip6_addr, &HSL_MCAST_FIB_SRC6);
        IPV6_ADDR_COPY (&p_mc_route->mc_ip6_addr, &HSL_MCAST_FIB_GROUP6);
        /* Set as IPv6. */
        p_mc_route->flags |= BCM_IPMC_IP6;
    }
#endif /* HAVE_IPV6 */

    /* In case L2 route added -> we are done. */
    if(!p_grp_src_entry->iif)
    {
        p_mc_route->flags |= BCM_IPMC_SOURCE_PORT_NOCHECK;
        HSL_FN_EXIT(STATUS_OK);  
    } 

    /* Find incoming interface. */
    ifp = hsl_ifmgr_lookup_by_index(p_grp_src_entry->iif);
    if ( !ifp )
    {
        HSL_LOG (HSL_LOG_FIB, HSL_LEVEL_ERROR, "Error finding incoming interface %d\n",
                 p_grp_src_entry->iif);
        HSL_FN_EXIT(STATUS_WRONG_PARAMS); 
    }

    /* Get Broadcom specific info. */
    in_bcmifp_l3 = ifp->system_info;
    if (! in_bcmifp_l3)
    {
        HSL_LOG (HSL_LOG_FIB, HSL_LEVEL_ERROR, 
                 "Error finding hardware interface for interface %s\n", ifp->name);
        HSL_IFMGR_IF_REF_DEC (ifp);

        HSL_FN_EXIT (STATUS_WRONG_PARAMS);
    }

    /* Set vid of iif */
    p_mc_route->vid = ifp->u.ip.vid;

    /* Populate trunk incoming interface information. */
    if (in_bcmifp_l3->type == HSL_BCM_IF_TYPE_TRUNK)
    {
        p_mc_route->ts = 1;  /* Trunk Port */
        p_mc_route->port_tgid = in_bcmifp_l3->trunk_id;
        HSL_IFMGR_IF_REF_DEC (ifp);
        HSL_FN_EXIT(STATUS_OK); /* We are done. */
    }

    /* If iif is SVI, get the actual iif L2 port */
    if (memcmp (ifp->name, "vlan", 4) == 0)
        in_l2_ifp = hsl_ifmgr_lookup_by_index (p_grp_src_entry->l2_iif);
    else
        /* Get first layer 2 port. */
        in_l2_ifp = hsl_ifmgr_get_first_L2_port (ifp);

    if (!in_l2_ifp)
    {
        HSL_LOG (HSL_LOG_FIB, HSL_LEVEL_ERROR, 
                 "Error finding iif L2 interface info %d\n", ifp->ifindex);
        HSL_IFMGR_IF_REF_DEC (ifp);
        HSL_FN_EXIT (STATUS_WRONG_PARAMS);
    }

    /* Get layer 2 port broadcom info. */
    in_bcmifp_l2 = in_l2_ifp->system_info;
    if ( !in_bcmifp_l2)
    {
        HSL_LOG (HSL_LOG_FIB, HSL_LEVEL_ERROR, 
                 "Error finding hw interface info %d\n", ifp->ifindex);
        HSL_IFMGR_IF_REF_DEC (ifp);
        HSL_IFMGR_IF_REF_DEC (in_l2_ifp);
        HSL_FN_EXIT (STATUS_WRONG_PARAMS);
    }

    p_mc_route->ts = 0;  /* Non-Trunk Port */
    p_mc_route->port_tgid = in_bcmifp_l2->u.l2.lport;

    HSL_IFMGR_IF_REF_DEC (ifp);
    HSL_IFMGR_IF_REF_DEC (in_l2_ifp);
    HSL_FN_EXIT(STATUS_OK);

}

/* Common function for deleting IPMC route. */
static int
_hsl_bcm_mc_route_del (bcmx_ipmc_addr_t *p_mc_route)
{
  int ret;
  
  ret = bcmx_ipmc_find (p_mc_route);
  if(ret < 0)
     HSL_FN_EXIT (-1);

  /* Delete any previous replications. */
  bcmx_ipmc_repl_delete_all (p_mc_route->ipmc_index, p_mc_route->l3_ports);

  /* Remove the entry completely */
  ret = bcmx_ipmc_remove (p_mc_route);
  if (ret < 0)
    {
      HSL_LOG (HSL_LOG_FIB, HSL_LEVEL_ERROR, 
               "Error deleting ip multicast entry\n");
    }

  return ret;
}


#ifdef HAVE_L2
/*  Routine adds L2 interface to an outgoing interfaces list. 

    Name: _hsl_bcm_mc_route_add_l2_if_to_list 
    IN -> p_mc_route - mc route info. 
    IN -> p_grp_src_entry - G,S entry information. 
    IN -> p_if_entry - Added interface information. 
   
    Return:
     STATUS_OK  Port added successfully  
     -STATUS  Otherwise. 
*/ 
static int 
_hsl_bcm_mc_route_add_l2_if_to_list(bcmx_ipmc_addr_t *p_mc_route,
    struct hsl_mc_fib_entry *p_grp_src_entry)
{
  struct hsl_if *ifp;
  struct hsl_bcm_if *bcmifp_l2;
  struct hsl_avl_node *node;
  struct hsl_bridge *bridge;
  struct hsl_vlan_port *p_vlan_info;
  struct hsl_vlan_port lookup_vlan; 
  struct hsl_mc_eggress_if_entry *p_egress_if;  
  struct hsl_mc_vlan_entry *p_vlan_entry;

  HSL_FN_ENTER();

  if(!p_mc_route || !p_grp_src_entry)
     HSL_FN_EXIT(STATUS_WRONG_PARAMS);

  p_vlan_entry = hsl_mc_fib_vlan_entry_get(p_grp_src_entry, p_mc_route->vid);

  if(!p_vlan_entry)
    {
      /* If no joins received add  all vlan members to outgoing interface. */
      HSL_BRIDGE_LOCK;
      bridge = p_hsl_bridge_master->bridge;

      if (!bridge)
      {
          HSL_BRIDGE_UNLOCK;
          HSL_FN_EXIT(STATUS_OK);
      }

      lookup_vlan.vid = p_mc_route->vid;

      node = hsl_avl_lookup (bridge->vlan_tree, &lookup_vlan);
      if(!node)
        {
          HSL_BRIDGE_UNLOCK;
          return HSL_ERR_BRIDGE_INVALID_PARAM;
        }

      p_vlan_info = (struct hsl_vlan_port *) HSL_AVL_NODE_INFO (node);
      if (! p_vlan_info)
        {
          HSL_BRIDGE_UNLOCK;
          return HSL_ERR_BRIDGE_INVALID_PARAM;
        }

      /* Add ports. */
      for (node = hsl_avl_top (p_vlan_info->port_tree); node; node = hsl_avl_next (node))
        {
          ifp = HSL_AVL_NODE_INFO (node);
          if (! ifp)
            continue;

          /* Do not add the L2 iif */
          if (ifp->ifindex == p_grp_src_entry->l2_iif)
            continue;

          HSL_IFMGR_IF_REF_INC (ifp);       

          bcmifp_l2 = ifp->system_info;
          if (! bcmifp_l2)
            {
              HSL_IFMGR_IF_REF_DEC (ifp);       
              HSL_LOG (HSL_LOG_FIB, HSL_LEVEL_ERROR, 
                  "Error finding hardware interface for interface %s\n", ifp->name);
              continue; 
            }

          /* Add ports to a proper list . */
          bcmx_lplist_add (&p_mc_route->l2_ports, bcmifp_l2->u.l2.lport);

          /* Mark port as untagged if necessary.*/  
          if (!(hsl_vlan_get_egress_type (ifp, p_vlan_info->vid))) 
            bcmx_lplist_add (&p_mc_route->l2_untag_ports, bcmifp_l2->u.l2.lport);
          HSL_IFMGR_IF_REF_DEC (ifp);       
        }
      HSL_BRIDGE_UNLOCK;
    }
  else
    {
      /* If no L2 filtering for VLAN, add all ports except the iif */
      /* Add ports. */
      for (node = hsl_avl_top (HSL_MCAST_FIB_L2_IF_TREE); node; node = hsl_avl_next (node))
        {
          p_egress_if = HSL_AVL_NODE_INFO (node);
          if (! p_egress_if)
            continue;

          /* Do not add the L2 iif */
          if (p_egress_if->ifindex == p_grp_src_entry->l2_iif)
            continue;

          /* Check for Filter mode */
          if (p_egress_if->is_exclude == HSL_TRUE)
            continue;

          ifp = hsl_ifmgr_lookup_by_index (p_egress_if->ifindex); 
          if(!ifp)            
            continue;

          bcmifp_l2 = ifp->system_info;
          if (! bcmifp_l2)
            {
              HSL_IFMGR_IF_REF_DEC (ifp);
              HSL_LOG (HSL_LOG_FIB, HSL_LEVEL_ERROR, 
                  "Error finding hardware interface for interface %s\n", ifp->name);
              continue; 
            }

          /* Add ports to a proper list . */
          bcmx_lplist_add (&p_mc_route->l2_ports, bcmifp_l2->u.l2.lport);

          /* Mark port as untagged if necessary.*/  
          if (!(hsl_vlan_get_egress_type (ifp, p_vlan_entry->vid))) 
            bcmx_lplist_add (&p_mc_route->l2_untag_ports, bcmifp_l2->u.l2.lport);

          HSL_IFMGR_IF_REF_DEC (ifp);
        }
    }

  HSL_FN_EXIT(STATUS_OK);
}
#endif /* HAVE_L2 */  

#if defined (HAVE_MCAST_IPV4) || defined (HAVE_MCAST_IPV6)

#ifdef HAVE_L2
/*  Routine adds vlan interfaces to an outgoing l3 interfaces list. 

    Name: _hsl_bcm_mc_route_add_vlan_if_to_list 
    IN -> p_port_list  - Port list. 
    IN -> p_grp_src_entry - G,S entry information. 
    IN -> vid - Vlan id. 
   
    Return:
     STATUS_OK  Port added successfully  
     -STATUS  Otherwise. 
*/ 

static int 
_hsl_bcm_mc_route_add_vlan_if_to_list(bcmx_lplist_t *p_port_list,
                               struct hsl_mc_fib_entry *p_grp_src_entry,
                               hsl_vid_t vid)
{
  struct hsl_if *ifp;
  struct hsl_bcm_if *bcmifp_l2;
  struct hsl_bridge *bridge;
  struct hsl_mc_vlan_entry *p_vlan_entry;
  struct hsl_mc_eggress_if_entry *p_egress_if;
  struct hsl_avl_node *node;
  struct hsl_vlan_port *p_vlan_info;
  struct hsl_vlan_port lookup_vlan; 

  HSL_FN_ENTER();

  if((!p_port_list) || (!p_grp_src_entry))
     HSL_FN_EXIT(STATUS_WRONG_PARAMS);

  p_vlan_entry = hsl_mc_fib_vlan_entry_get(p_grp_src_entry, vid);

  if(!p_vlan_entry)
  {
     /* If no joins received add  all vlan members to outgoing interface. */
     HSL_BRIDGE_LOCK;
     bridge = p_hsl_bridge_master->bridge;

      if (!bridge)
      {
          HSL_BRIDGE_UNLOCK;
          HSL_FN_EXIT(STATUS_OK);
      }

     lookup_vlan.vid = vid;

     node = hsl_avl_lookup (bridge->vlan_tree, &lookup_vlan);
     if(!node)
     {
        HSL_BRIDGE_UNLOCK;
        return HSL_ERR_BRIDGE_INVALID_PARAM;
     }

     p_vlan_info = (struct hsl_vlan_port *) HSL_AVL_NODE_INFO (node);
     if (! p_vlan_info)
     {
        HSL_BRIDGE_UNLOCK;
        return HSL_ERR_BRIDGE_INVALID_PARAM;
     }

     /* Add ports. */
     for (node = hsl_avl_top (p_vlan_info->port_tree); node; node = hsl_avl_next (node))
     {
        ifp = HSL_AVL_NODE_INFO (node);
        if (! ifp)
          continue;

        HSL_IFMGR_IF_REF_INC (ifp);       

        bcmifp_l2 = ifp->system_info;
        if (! bcmifp_l2)
        {
           HSL_IFMGR_IF_REF_DEC (ifp);       
           HSL_LOG (HSL_LOG_FIB, HSL_LEVEL_ERROR, 
               "Error finding hardware interface for interface %s\n", ifp->name);
           continue; 
        }

        /* !!!!! TRUNK IS NOT HANDLED RIGHT !!!! */   
        /* Add L3 ports. */
        bcmx_lplist_add (p_port_list, bcmifp_l2->u.l2.lport);
        HSL_IFMGR_IF_REF_DEC (ifp);       
     }
     HSL_BRIDGE_UNLOCK;
   }
   /* Else add only joined members. */
   else
   {
     /* Add ports. */
     for (node = hsl_avl_top (HSL_MCAST_FIB_L2_IF_TREE); node; node = hsl_avl_next (node))
     {
        p_egress_if = HSL_AVL_NODE_INFO (node);
        if (! p_egress_if)
          continue;

        ifp = hsl_ifmgr_lookup_by_index (p_egress_if->ifindex); 
        if(!ifp)            
          continue;
 
        bcmifp_l2 = ifp->system_info;
        if (! bcmifp_l2)
        {
           HSL_IFMGR_IF_REF_DEC (ifp);
           HSL_LOG (HSL_LOG_FIB, HSL_LEVEL_ERROR, 
               "Error finding hardware interface for interface %s\n", ifp->name);
           continue; 
        }

        /* Add L3 ports. */
        bcmx_lplist_add (p_port_list, bcmifp_l2->u.l2.lport);
        HSL_IFMGR_IF_REF_DEC (ifp);
     }
   }
  HSL_FN_EXIT(STATUS_OK);
}
#endif /* HAVE_L2 */

/*  Routine adds L3 interface to an outgoing interfaces list. 

    Name: _hsl_bcm_mc_route_add_l3_if_to_list 
    IN -> p_port_list  - Port list. 
    IN -> p_grp_src_entry - G,S entry information. 
    IN -> p_if_entry - Added interface information. 
   
    Return:
     STATUS_OK  Port added successfully  
     -STATUS  Otherwise. 
*/ 

static int 
_hsl_bcm_mc_route_add_l3_if_to_list(bcmx_lplist_t *p_port_list,
                                    struct hsl_mc_fib_entry *p_grp_src_entry,
                                    struct hsl_mc_eggress_if_entry *p_if_entry)
{
  struct hsl_if *ifp;
  struct hsl_if *l2_ifp;
  struct hsl_bcm_if *bcmifp_l2;
  struct hsl_bcm_if *bcmifp_l3;

  HSL_FN_ENTER();
 
  if((!p_port_list) || (!p_if_entry) || (!p_grp_src_entry))
     HSL_FN_EXIT(STATUS_WRONG_PARAMS);

  /* Find outgoing interface. */
  ifp = hsl_ifmgr_lookup_by_index(p_if_entry->ifindex);
  if ( !ifp )
   {
       HSL_LOG (HSL_LOG_FIB, HSL_LEVEL_ERROR, "Error finding outgoing interface %d\n",
                   p_if_entry->ifindex);
       HSL_FN_EXIT(STATUS_WRONG_PARAMS); 
   }
  
   /* Get Broadcom specific info. */
   bcmifp_l3 = ifp->system_info;
   if (! bcmifp_l3)
    {
      HSL_LOG (HSL_LOG_FIB, HSL_LEVEL_ERROR, 
               "Error finding hardware interface for interface %s\n", ifp->name);
      HSL_IFMGR_IF_REF_DEC (ifp);
      HSL_FN_EXIT (STATUS_WRONG_PARAMS);
    }
#ifdef HAVE_L2
    /* Check for SVI. */
   if (!memcmp (ifp->name, "vl", 2)) 
   {
      /* populate olist based on igmp joins. */ 
      _hsl_bcm_mc_route_add_vlan_if_to_list(p_port_list, p_grp_src_entry,
                                                 ifp->u.ip.vid);
       
      HSL_IFMGR_IF_REF_DEC (ifp);
      HSL_FN_EXIT(STATUS_OK);
   }
#endif /* HAVE_L2 */

   /* For trunks & pure L3 ports get first underlying layer 2 interface. */ 
   l2_ifp = hsl_ifmgr_get_first_L2_port (ifp);
   if (!l2_ifp)
    {
        HSL_LOG (HSL_LOG_FIB, HSL_LEVEL_ERROR, 
           "Error finding iif L2 interface info %d\n", ifp->ifindex);
        HSL_IFMGR_IF_REF_DEC (ifp);
        HSL_FN_EXIT (STATUS_WRONG_PARAMS);
    }
      
   /* Get layer 2 port broadcom info. */
   bcmifp_l2 = l2_ifp->system_info;
   if ( !bcmifp_l2)
    {
       HSL_LOG (HSL_LOG_FIB, HSL_LEVEL_ERROR, 
         "Error finding hw interface info %d\n", l2_ifp->ifindex);

        HSL_IFMGR_IF_REF_DEC (ifp);
        HSL_IFMGR_IF_REF_DEC (l2_ifp);

        HSL_FN_EXIT (STATUS_WRONG_PARAMS);
    }
  
   /* Add L3 ports. */
   bcmx_lplist_add (p_port_list, bcmifp_l2->u.l2.lport);

   HSL_IFMGR_IF_REF_DEC (ifp);
   HSL_IFMGR_IF_REF_DEC (l2_ifp);
   HSL_FN_EXIT(STATUS_OK);

}

/*  Routine adds L3 interface to an replication vlan list. 

    Name:_hsl_bcm_mc_route_vlan_repl 
    IN -> p_mc_route - Replicated route. 
    IN -> p_grp_src_entry - G,S entry information. 
   
    Return:
     STATUS_OK  Port replication added successfully  
     -STATUS  Otherwise. 
*/ 
static int
_hsl_bcm_mc_route_vlan_repl(bcmx_ipmc_addr_t *p_mc_route, struct hsl_mc_fib_entry *p_grp_src_entry)
{
  bcmx_lplist_t port_list;
  struct hsl_avl_node *avl_node; 
  struct hsl_mc_eggress_if_entry *p_if_entry;
  struct hsl_if *ifp;
  struct hsl_bcm_if *bcmifp_l3;
  int ret;
  
  if(!p_grp_src_entry)  
    HSL_FN_EXIT(HSL_FIB_ERR_INVALID_PARAM);
 
  /* Initialize lplist. */
  if (bcmx_lplist_init (&port_list, 1, 0) < 0)
    {
      HSL_LOG (HSL_LOG_FIB, HSL_LEVEL_ERROR, "Out of memory\n");
      HSL_FN_EXIT (-1);
    }

  for (avl_node = hsl_avl_top (HSL_MCAST_FIB_L3_IF_TREE); avl_node; avl_node = hsl_avl_next (avl_node))
    {
       p_if_entry = (struct hsl_mc_eggress_if_entry *)HSL_AVL_NODE_INFO (avl_node);
       if(!p_if_entry)
         continue;

       ret = _hsl_bcm_mc_route_add_l3_if_to_list(&port_list,p_grp_src_entry,p_if_entry); 
       if(ret < 0)
         continue;

       /* Find outgoing interface. */
       ifp = hsl_ifmgr_lookup_by_index(p_if_entry->ifindex);
       if ( !ifp ) /* shouldn't happen since add l3 if to a list successeeded */
          continue;

       /* Get Broadcom specific info. */
       bcmifp_l3 = ifp->system_info;
       if ( !bcmifp_l3) /* shouldn't happen since add l3 if to a list successeeded */
          continue;

       /* If XGS2 use VLAN replication, if XGS3 use L3 interface id for replication. */ 
       ret = bcmx_ipmc_repl_add (p_mc_route->ipmc_index, port_list, ifp->u.ip.vid);
       if (ret < 0)
         {
            HSL_LOG (HSL_LOG_FIB, HSL_LEVEL_ERROR, 
                     "Error adding replication for ifindex %d vid %u %d\n", ifp->ifindex, 
                     ifp->u.ip.vid, ret);
         }

        bcmx_lplist_clear (&port_list);
        HSL_IFMGR_IF_REF_DEC (ifp);
    } /* End for loop */

  bcmx_lplist_free (&port_list);
  HSL_FN_EXIT (STATUS_OK);
}
#endif /* HAVE_MCAST_IPV4 || HAVE_MCAST_IPV6 */

/*  Routine Get outgoing interfaces list. 

    Name: _hsl_bcm_mc_route_get_olif
    IN -> p_port_list  - Port list. 
    IN -> p_grp_src_entry - G,S entry information. 
   
    Return:
     STATUS_OK  Port added successfully  
     -STATUS  Otherwise. 
*/ 

static int 
_hsl_bcm_mc_route_get_olif(bcmx_ipmc_addr_t *p_mc_route,
                           struct hsl_mc_fib_entry *p_grp_src_entry)
{
#if defined (HAVE_MCAST_IPV4) || defined (HAVE_MCAST_IPV6)
  struct hsl_avl_node *node;
  struct hsl_mc_eggress_if_entry *p_if_entry;
#endif /* HAVE_MCAST_IPV4 || HAVE_MCAST_IPV6 */
  int ret;

  HSL_FN_ENTER();  

  /* Input parameters check.*/ 
  if((!p_mc_route) || (!p_grp_src_entry))
     HSL_FN_EXIT(STATUS_WRONG_PARAMS);

  /* Iterate over the outgoing interface list and add each port as tagged
     or untagged */
#ifdef HAVE_L2
    /* Search for vid in vlan tree. */ 
  ret = _hsl_bcm_mc_route_add_l2_if_to_list(p_mc_route, p_grp_src_entry);

#endif /* HAVE_L2 */

#if defined (HAVE_MCAST_IPV4) || defined (HAVE_MCAST_IPV6)
   for (node = hsl_avl_top (HSL_MCAST_FIB_L3_IF_TREE); node; node = hsl_avl_next (node))
     {
           p_if_entry = (struct hsl_mc_eggress_if_entry *)HSL_AVL_NODE_INFO (node);
           if(!p_if_entry)
             continue;
           ret = _hsl_bcm_mc_route_add_l3_if_to_list(&p_mc_route->l3_ports,p_grp_src_entry,p_if_entry); 
           if(ret < 0)  
             continue;
      }
#endif /* HAVE_MCAST_IPV4 || HAVE_MCAST_IPV6 */

  HSL_FN_EXIT(STATUS_OK);
} 

static int
_hsl_bcm_mc_route_add_common (bcmx_ipmc_addr_t *p_mc_route, struct hsl_mc_fib_entry *p_grp_src_entry)
{
  HSL_BOOL pim_running;
  int ret;
#if defined (HAVE_MCAST_IPV4) || defined (HAVE_MCAST_IPV6)
  int ret1;
#endif /* HAVE_MCAST_IPV4 || HAVE_MCAST_IPV6 */

  HSL_FN_ENTER();
  /* Input parameters check */
  if(!p_mc_route || ! p_grp_src_entry) 
    HSL_FN_EXIT(STATUS_WRONG_PARAMS);

  /* Add registered vif mirroring if required. */
  if (p_grp_src_entry->mirror_to_cpu)
  { 
    /* Set CPU port in the L2 port list and continue */
    bcmx_lplist_add (&p_mc_route->l2_ports, BCMX_LPORT_LOCAL_CPU);
  }

  ret  = _hsl_bcm_mc_route_get_olif(p_mc_route, p_grp_src_entry);
  if(ret < 0)
  {
      hsl_mc_fib_log_msg(HSL_LEVEL_ERROR, p_grp_src_entry,"Error reading outgoing interfaces list\n");
      HSL_FN_EXIT(STATUS_ERROR);
  }

  /* If PIM is not enabled drop source port mismatch packets */
  pim_running = hsl_mc_is_pim_running (HSL_MCAST_FIB_GROUP_FAMILY);

#if 0 
  if ((pim_running == HSL_FALSE) && (p_grp_src_entry->iif))
    p_mc_route->cd = 1;
#endif
  
  /* Make the entry valid */
  p_mc_route->v = 1;

#if 0 
  /* Set the TTL threshold to minimum for now */
  p_mc_route->ttl = 1;
#endif

  /* Find IPMC route. */
  ret = bcmx_ipmc_find (p_mc_route);

  /* Remove the existing entry */
  if (ret == BCM_E_NONE)
    bcmx_ipmc_remove (p_mc_route);

  /* Add IPMC route. */
  ret = bcmx_ipmc_add (p_mc_route);
  if (ret < 0)
    {
      HSL_LOG (HSL_LOG_FIB, HSL_LEVEL_ERROR, 
               "Error Adding ip multicast entry %d %s \n", ret, bcm_errmsg (ret));
      goto exit;
    }

  /* The BCM SDK seems to add a new route with the BCM_L3_HIT bit set
   * for some reason :? This results in the first statistics query
   * to return a HIT even when only one pkt is forwarded via the
   * slow patch (i.e. TCP/IP stack).
   * To avoid this, set the BCM_IPMC_HIT_CLEAR flag
   */
  p_mc_route->flags |= BCM_IPMC_HIT_CLEAR; 

  /* XXX: SDK 5.2.2 does not support BCM_IPMC_REPLACE. So existing
   * route entries have to be deleted and added. After add, lookup
   * the entry to get the index which is then used to add replications
   *
   * UGLY!!!
   */
  /* Find IPMC route. */
  ret = bcmx_ipmc_find (p_mc_route);
  if (ret < 0)
    {
      HSL_LOG (HSL_LOG_FIB, HSL_LEVEL_ERROR, 
               "Error find newly added ip multicast entry %d\n", ret);
      goto exit;
    }
#if defined (HAVE_MCAST_IPV4) || defined (HAVE_MCAST_IPV6)
   ret1 = _hsl_bcm_mc_route_vlan_repl(p_mc_route,p_grp_src_entry);
   if(ret1 < 0)
     {  
        HSL_LOG (HSL_LOG_FIB, HSL_LEVEL_ERROR, 
               "Error %d adding replication entries\n", ret1);
     }
#endif /* HAVE_MCAST_IPV4 || HAVE_MCAST_IPV6 */
 exit:
  HSL_FN_EXIT (ret);
}

static int 
_hsl_bcm_mc_route_mark_for_update (bcmx_ipmc_addr_t *p_mc_route)
{
  int ret;

  HSL_FN_ENTER ();

  /* Input parameters check. */
  if(!p_mc_route)
    HSL_FN_EXIT(STATUS_WRONG_PARAMS);
 

  /* Find IPMC entry. */
  ret = bcmx_ipmc_find (p_mc_route);
  if (ret == BCM_E_NONE)
    {
      /* Set entry replacement flag. */
      p_mc_route->flags |= BCM_IPMC_REPLACE;

      /* Delete any previous replications L3 ports. */
      bcmx_ipmc_repl_delete_all (p_mc_route->ipmc_index, p_mc_route->l3_ports);

      /* Delete any previous replications. */
      bcmx_ipmc_repl_delete_all (p_mc_route->ipmc_index, p_mc_route->l2_ports);

      /* Clear L3 ports. */
      bcmx_lplist_clear (&p_mc_route->l3_ports); 

      /* Clear L2 ports. */
      bcmx_lplist_clear (&p_mc_route->l2_ports); 

      /* Clear L2 untagged ports. */
      bcmx_lplist_clear (&p_mc_route->l2_untag_ports); 
    }
  else
    {
      HSL_FN_EXIT (STATUS_KEY_NOT_FOUND);
    }
   ret= bcmx_ipmc_remove (p_mc_route);
   if (ret < 0)
    {
      HSL_LOG (HSL_LOG_FIB, HSL_LEVEL_INFO, "Error deleting existing entry in route update!!\n");

      ret = STATUS_KEY_NOT_FOUND;
    }

  HSL_FN_EXIT (ret);
}
static int 
_hsl_bcm_mc_route_add(struct hsl_mc_fib_entry *p_grp_src_entry)
{
  bcmx_ipmc_addr_t mc_route;
  struct hsl_avl_node *p_node;
  struct hsl_mc_vlan_entry *p_vlan_entry;
  int ret = 0;

  HSL_FN_ENTER ();

  /* Init mroute info. */
  ret = _hsl_bcm_mc_route_init(&mc_route,p_grp_src_entry);
  if(ret < 0)
     HSL_FN_EXIT (-1);
 
  /* Mark entry for update entry. */
  if(mc_route.vid)
     ret = _hsl_bcm_mc_route_add_common (&mc_route,p_grp_src_entry);
  else 
   { 
      for (p_node = hsl_avl_top (HSL_MCAST_FIB_VLAN_TREE); p_node;p_node = hsl_avl_next (p_node))
       {
           p_vlan_entry = (struct hsl_mc_vlan_entry *)HSL_AVL_NODE_INFO (p_node);
           if(!p_vlan_entry)
             continue;
           mc_route.vid = p_vlan_entry->vid;
           ret = _hsl_bcm_mc_route_add_common(&mc_route, p_grp_src_entry);
       }
   }

  /* Free entry. */
  bcmx_ipmc_addr_t_free (&mc_route);
  HSL_FN_EXIT (ret);
}


static int 
_hsl_bcm_mc_route_update (struct hsl_mc_fib_entry *p_grp_src_entry)
{
  bcmx_ipmc_addr_t mc_route;
  hsl_ifIndex_t newIfindex;
  struct hsl_avl_node *p_node;
  struct hsl_mc_vlan_entry *p_vlan_entry;
  int ret = 0;

  HSL_FN_ENTER ();


  /* Preserve new ifindex */   
  newIfindex = p_grp_src_entry->iif; 
  p_grp_src_entry->iif =  p_grp_src_entry->prev_iif; 

  /* Init mroute info. */
  ret = _hsl_bcm_mc_route_init(&mc_route,p_grp_src_entry);
  if(ret < 0)
     HSL_FN_EXIT (-1);
 
  /* Mark entry for update entry. */
  if(mc_route.vid)
     _hsl_bcm_mc_route_mark_for_update(&mc_route);
  else 
   { 
      for (p_node = hsl_avl_top (HSL_MCAST_FIB_VLAN_TREE); p_node;p_node = hsl_avl_next (p_node))
       {
           p_vlan_entry = (struct hsl_mc_vlan_entry *)HSL_AVL_NODE_INFO (p_node);
           if(!p_vlan_entry)
             continue;
           mc_route.vid = p_vlan_entry->vid;
           _hsl_bcm_mc_route_mark_for_update(&mc_route);
       }
   }                                   

  /* Restore new ifindex. */
  p_grp_src_entry->iif = newIfindex; 

  /* Add an entry. */
  ret =  _hsl_bcm_mc_route_add(p_grp_src_entry);

  /* Free entry. */
  bcmx_ipmc_addr_t_free (&mc_route);
  HSL_FN_EXIT (ret);
}

/* Add multicast route */
int hsl_bcm_mc_route_add (struct hsl_mc_fib_entry *p_grp_src_entry)
{
   int ret;
   HSL_FN_ENTER(); 

  /* Input parameters check. */ 
  if (!p_grp_src_entry) 
    HSL_FN_EXIT(HSL_FIB_ERR_INVALID_PARAM);
   
  if (!p_grp_src_entry->do_hw_update)
    ret = _hsl_bcm_mc_route_add(p_grp_src_entry);
  else
    ret = _hsl_bcm_mc_route_update (p_grp_src_entry);

  HSL_FN_EXIT (ret);
}

/* Delete multicast route */
int hsl_bcm_mc_route_del (struct hsl_mc_fib_entry *p_grp_src_entry)
{
  bcmx_ipmc_addr_t mc_route;
  struct hsl_avl_node *p_node;
  struct hsl_mc_vlan_entry *p_vlan_entry;
  int ret;
  
  HSL_FN_ENTER(); 
  
  /* Init mroute info. */
  ret = _hsl_bcm_mc_route_init(&mc_route,p_grp_src_entry);
  if(ret < 0)
     HSL_FN_EXIT (-1);

  /* Delete entry. */
  if(mc_route.vid)
     ret = _hsl_bcm_mc_route_del (&mc_route);
  else 
   { 
      for (p_node = hsl_avl_top (HSL_MCAST_FIB_VLAN_TREE); p_node;p_node = hsl_avl_next (p_node))
       {
           p_vlan_entry = (struct hsl_mc_vlan_entry *)HSL_AVL_NODE_INFO (p_node);
           if(!p_vlan_entry)
             continue;
           mc_route.vid = p_vlan_entry->vid;
           ret = _hsl_bcm_mc_route_del (&mc_route);
       }
   }                                   

  /* Free entry. */
  bcmx_ipmc_addr_t_free (&mc_route);
  HSL_FN_EXIT(ret);
}


#ifdef HAVE_L2
/* Delete layer 2 multicast route */
int hsl_bcm_l2_mc_route_del (struct hsl_mc_fib_entry *p_grp_src_entry, hsl_vid_t vid)
{
  bcmx_ipmc_addr_t mc_route;
  int ret;
  
  HSL_FN_ENTER(); 

  /* Init mroute info. */
  ret = _hsl_bcm_mc_route_init(&mc_route,p_grp_src_entry);
  if(ret < 0)
     HSL_FN_EXIT (-1);

  mc_route.vid = vid;
  ret = _hsl_bcm_mc_route_del (&mc_route);

  /* Free entry. */
  bcmx_ipmc_addr_t_free (&mc_route);
  HSL_FN_EXIT(ret);
}
#endif /* HAVE_L2 */  


/*
  Register callbacks 
*/
void
hsl_ipv4_mc_hw_cb_register(void)
{
  HSL_FN_ENTER();
  hsl_bcm_ipv4_mc_callbacks.hw_ipv4_mc_route_add = hsl_bcm_mc_route_add;
  hsl_bcm_ipv4_mc_callbacks.hw_ipv4_mc_route_del = hsl_bcm_mc_route_del;
#ifdef HAVE_MCAST_IPV4
  hsl_bcm_ipv4_mc_callbacks.hw_ipv4_mc_init = hsl_bcm_ipv4_mc_init;
  hsl_bcm_ipv4_mc_callbacks.hw_ipv4_mc_deinit = hsl_bcm_ipv4_mc_deinit;
  hsl_bcm_ipv4_mc_callbacks.hw_ipv4_mc_sg_stat = hsl_bcm_mc_sg_stat;
  hsl_bcm_ipv4_mc_callbacks.hw_ipv4_mc_vif_add = hsl_bcm_mc_vif_add;
  hsl_bcm_ipv4_mc_callbacks.hw_ipv4_mc_vif_del = hsl_bcm_mc_vif_del;
#endif /* HAVE_MCAST_IPV4 */
#ifdef HAVE_L2
  hsl_bcm_ipv4_mc_callbacks.hw_l2_mc_route_del = hsl_bcm_l2_mc_route_del;
#endif /* HAVE_L2 */  
  
  hsl_ipv4_mc_hw_cb_reg(&hsl_bcm_ipv4_mc_callbacks);
  HSL_FN_EXIT(STATUS_OK);
}

#ifdef HAVE_IPV6
/*
  Register callbacks 
*/
void
hsl_ipv6_mc_hw_cb_register(void)
{
  HSL_FN_ENTER();
  hsl_bcm_ipv6_mc_callbacks.hw_ipv6_mc_route_add = hsl_bcm_mc_route_add;
  hsl_bcm_ipv6_mc_callbacks.hw_ipv6_mc_route_del = hsl_bcm_mc_route_del;
#ifdef HAVE_MCAST_IPV6
  hsl_bcm_ipv6_mc_callbacks.hw_ipv6_mc_init = hsl_bcm_ipv6_mc_init;
  hsl_bcm_ipv6_mc_callbacks.hw_ipv6_mc_deinit = hsl_bcm_ipv6_mc_deinit;
  hsl_bcm_ipv6_mc_callbacks.hw_ipv6_mc_sg_stat = hsl_bcm_mc_sg_stat;
  hsl_bcm_ipv6_mc_callbacks.hw_ipv6_mc_vif_add = hsl_bcm_mc_vif_add;
  hsl_bcm_ipv6_mc_callbacks.hw_ipv6_mc_vif_del = hsl_bcm_mc_vif_del;
#endif /* HAVE_MCAST_IPV6 */
#ifdef HAVE_L2
  hsl_bcm_ipv6_mc_callbacks.hw_l2_mc_route_del = hsl_bcm_l2_mc_route_del;
#endif /* HAVE_L2 */
  hsl_ipv6_mc_hw_cb_reg(&hsl_bcm_ipv6_mc_callbacks);
  HSL_FN_EXIT(STATUS_OK);
}
#endif /* HAVE_IPV6 */
#endif /* HAVE_MCAST_IPV4 || HAVE_MCAST_IPV6) || HAVE_IGMP_SNOOP */

