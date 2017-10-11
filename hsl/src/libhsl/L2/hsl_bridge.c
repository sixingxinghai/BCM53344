/* Copyright (C) 2007  All Rights Reserved. */

#include "config.h"
#include "hsl_os.h"
#include "hsl_types.h"

#ifdef HAVE_L2
#include "hal_types.h"
#include "hal_l2.h"
#include "hal_msg.h"

#include "hsl_avl.h"
#include "hsl_oss.h"
#include "hsl_error.h"

#include "hsl_logger.h"
#include "hsl_ifmgr.h"
#include "hsl_vlan.h"
#include "hsl_bridge.h"
#include "hsl_mac_tbl.h"

struct hsl_bridge_master *p_hsl_bridge_master = NULL;

/*
  Forward declarations. 
*/
int hsl_bridge_hw_register(void);

/*
  VLAN compare.
*/
static int
_hsl_bridge_vlan_cmp (void *param1, void *param2)
{
  struct hsl_vlan_port *p1 = (struct hsl_vlan_port *) param1;
  struct hsl_vlan_port *p2 = (struct hsl_vlan_port *) param2;

  /* Less than. */
  if (p1->vid < p2->vid)
    return -1;

  /* Greater than. */
  if (p1->vid > p2->vid)
    return 1;

  /* Equals to. */
  return 0;
}

/*
  Port compare routine.
*/
int
_hsl_bridge_port_cmp (void *param1, void *param2)
{
  /* The port is of type hsl_if. Just compare the pointers as integers. */
  unsigned int p1 = (unsigned int) param1;
  unsigned int p2 = (unsigned int) param2;

  /* Less than. */
  if (p1 < p2)
    return -1;

  /* Greater than. */
  if (p1 > p2)
    return 1;

  /* Equals to. */
  return 0;
}

/*
  Init bridge.
*/
struct hsl_bridge *
hsl_bridge_init ()
{
  struct hsl_bridge *b = NULL;
  int ret;

  HSL_FN_ENTER ();

  b = oss_malloc (sizeof (struct hsl_bridge), OSS_MEM_HEAP);
  if (! b)
    HSL_FN_EXIT (b);

#ifdef HAVE_VLAN
  /* Create vlan tree. */
  ret = hsl_avl_create (&b->vlan_tree, 0, _hsl_bridge_vlan_cmp);
  if (ret < 0)
    goto ERR;
#endif /* HAVE_VLAN. */

  /* Create tree of ports. */
  ret = hsl_avl_create (&b->port_tree, 0, _hsl_bridge_port_cmp);
  if (ret < 0)
    goto ERR;

  HSL_FN_EXIT (b);

 ERR:
  if (b && b->port_tree)
    hsl_avl_tree_free (&b->port_tree, NULL);

#ifdef HAVE_VLAN
  if (b && b->vlan_tree)
    hsl_avl_tree_free (&b->vlan_tree, hsl_free);
#endif /* HAVE_VLAN */

  if (b)
    oss_free (b, OSS_MEM_HEAP);

  HSL_FN_EXIT (NULL);
}

/* 
   Deinit bridge.
*/
int
hsl_bridge_deinit (struct hsl_bridge *b)
{
#ifdef HAVE_VLAN
  struct hsl_vlan_port *vlan;
  struct hsl_avl_node *node;
#endif /* HAVE_VLAN. */

  HSL_FN_ENTER ();

  if (! b)
    HSL_FN_EXIT (0);
   
#ifdef HAVE_VLAN
  for (node = hsl_avl_top (b->vlan_tree); node; node = hsl_avl_next (node))
    {
      vlan = HSL_AVL_NODE_INFO (node);
      
      /* Deinit vlan. */
      hsl_vlan_port_map_deinit (vlan);    
    }
  /* Free vlans tree. */
  hsl_avl_tree_free (&b->vlan_tree, NULL);

  /* Free ports tree. */
  hsl_avl_tree_free (&b->port_tree, NULL);
#endif /* HAVE_VLAN. */  

  HSL_FN_EXIT (0);
}

/*
  Initialize bridge port.
*/
struct hsl_bridge_port *
hsl_bridge_port_init (void)
{
  return oss_malloc (sizeof (struct hsl_bridge_port), OSS_MEM_HEAP);
}

/*
  Deinit bridge port.
*/
int
hsl_bridge_port_deinit (struct hsl_bridge_port *port)
{
  if (port)
    {
#ifdef HAVE_VLAN
      if (port->vlan)
        {
          hsl_port_vlan_map_deinit (port->vlan);
          port->vlan = NULL;
        }
     
#endif /* HAVE_VLAN. */
      oss_free (port, OSS_MEM_HEAP);
    }
  return 0;
}

/*
  Init master. 
*/
int
hsl_bridge_master_init (void)
{
  int ret;

  hsl_mac_address_t bpdu_addr  = {0x1, 0x80, 0xc2, 0x00, 0x00, 0x00};
  hsl_mac_address_t gmrp_addr  = {0x1, 0x80, 0xc2, 0x00, 0x00, 0x20};
  hsl_mac_address_t gvrp_addr  = {0x1, 0x80, 0xc2, 0x00, 0x00, 0x21};
  hsl_mac_address_t lacp_addr  = {0x1, 0x80, 0xc2, 0x00, 0x00, 0x02};
  hsl_mac_address_t eapol_addr = {0x1, 0x80, 0xc2, 0x00, 0x00, 0x03};
  hsl_mac_address_t cfm_addr   = {0x1, 0x80, 0xC2, 0x11, 0x11, 0x10};
  hsl_mac_address_t lldp_addr  = {0x1, 0x80, 0xc2, 0x00, 0x00, 0x0e};

  HSL_FN_ENTER ();

  if (! p_hsl_bridge_master)
    {
      p_hsl_bridge_master = oss_malloc (sizeof (struct hsl_bridge_master), OSS_MEM_HEAP);
      if (! p_hsl_bridge_master)
        return HSL_ERR_BRIDGE_NOMEM;

      /* Create mutex. */
      ret = oss_sem_new ("Bridge mutex", OSS_SEM_MUTEX, 0, NULL, &p_hsl_bridge_master->mutex);
      if (ret < 0)
        {
          oss_free (p_hsl_bridge_master, OSS_MEM_HEAP);

          HSL_FN_EXIT (HSL_ERR_BRIDGE_NOMEM);
        }

      /* Call HW to set callbacks.  */
      hsl_bridge_hw_register();

      memcpy (p_hsl_bridge_master->dest_addr [HAL_PROTO_STP],
                   bpdu_addr, HSL_ETHER_ALEN);
      memcpy (p_hsl_bridge_master->dest_addr [HAL_PROTO_RSTP],
                   bpdu_addr, HSL_ETHER_ALEN);
      memcpy (p_hsl_bridge_master->dest_addr [HAL_PROTO_MSTP],
                   bpdu_addr, HSL_ETHER_ALEN);
      memcpy (p_hsl_bridge_master->dest_addr [HAL_PROTO_GMRP],
                   gmrp_addr, HSL_ETHER_ALEN);
      memcpy (p_hsl_bridge_master->dest_addr [HAL_PROTO_MMRP],
                   gmrp_addr, HSL_ETHER_ALEN);
      memcpy (p_hsl_bridge_master->dest_addr [HAL_PROTO_GVRP],
                   gvrp_addr, HSL_ETHER_ALEN);
      memcpy (p_hsl_bridge_master->dest_addr [HAL_PROTO_MVRP],
                   gvrp_addr, HSL_ETHER_ALEN);
      memcpy (p_hsl_bridge_master->dest_addr [HAL_PROTO_LACP],
                   lacp_addr, HSL_ETHER_ALEN);
      memcpy (p_hsl_bridge_master->dest_addr [HAL_PROTO_DOT1X],
                   eapol_addr, HSL_ETHER_ALEN);
      memcpy (p_hsl_bridge_master->dest_addr [HAL_PROTO_LLDP],
                   lldp_addr, HSL_ETHER_ALEN);
      memcpy (p_hsl_bridge_master->dest_addr [HAL_PROTO_CFM],
                   cfm_addr, HSL_ETHER_ALEN);

#ifdef HAVE_ONMD
      p_hsl_bridge_master->cfm_ether_type = HSL_L2_ETH_P_CFM;
#endif /* HAVE_ONMD */

#if defined HAVE_IGMP_SNOOP || defined HAVE_MLD_SNOOP      
      p_hsl_bridge_master->hsl_l2_unknown_mcast = HSL_BRIDGE_L2_UNKNOWN_MCAST_FLOOD;      
#endif /* HAVE_IGMP_SNOOP || HAVE_MLD_SNOOP */
    }

  HSL_FN_EXIT (0);
}

/* 
   Deinit master.
*/
int
hsl_bridge_master_deinit (void)
{
  HSL_FN_ENTER ();

  if (p_hsl_bridge_master)
    {
      /* Deinit bridge. */
      if (p_hsl_bridge_master->bridge)
        {
          hsl_bridge_deinit (p_hsl_bridge_master->bridge);
          oss_free (p_hsl_bridge_master->bridge, OSS_MEM_HEAP);
          p_hsl_bridge_master->bridge = NULL;
        }

      /* Delete mutex. */
      oss_sem_delete (OSS_SEM_MUTEX, p_hsl_bridge_master->mutex);
      
      oss_free (p_hsl_bridge_master, OSS_MEM_HEAP);

      p_hsl_bridge_master = NULL;
    }

  HSL_FN_EXIT (0);
}

/*
  Bridge add.
*/
int
hsl_bridge_add (char *name, int is_vlan_aware, enum hal_bridge_type type,
                char edge)
{
  struct hsl_bridge *b;

  HSL_FN_ENTER ();
  HSL_BRIDGE_LOCK;

  if (p_hsl_bridge_master->bridge)
    {
      HSL_BRIDGE_UNLOCK;
      HSL_FN_EXIT (HSL_ERR_BRIDGE_EEXISTS);
    }

  b = hsl_bridge_init ();
  if (! b)
    {
      HSL_BRIDGE_UNLOCK;
      HSL_FN_EXIT (HSL_ERR_BRIDGE_NOMEM);
    }
  
  if (is_vlan_aware)
    SET_FLAG (b->flags, HSL_BRIDGE_VLAN_AWARE);
  SET_FLAG (b->flags, HSL_BRIDGE_LEARNING);

  memcpy (b->name, name, HAL_BRIDGE_NAME_LEN + 1);
  b->type = type;

#ifdef HAVE_PROVIDER_BRIDGE
  b->edge = edge;
#endif /* HAVE_PROVIDER_BRIDGE */

  p_hsl_bridge_master->bridge = b;

  if (p_hsl_bridge_master->hw_cb && p_hsl_bridge_master->hw_cb->bridge_init)
    (*p_hsl_bridge_master->hw_cb->bridge_init) (b);
  
  HSL_BRIDGE_UNLOCK;

  HSL_FN_EXIT (0);
}

/*
  Bridge delete.
*/
int
hsl_bridge_delete (char *name)
{
  HSL_FN_ENTER ();

  HSL_BRIDGE_LOCK;
  
  if (p_hsl_bridge_master->bridge)
    {
      if (memcmp (p_hsl_bridge_master->bridge->name, name, HAL_BRIDGE_NAME_LEN))
        {
          HSL_BRIDGE_UNLOCK;
          HSL_FN_EXIT (HSL_ERR_BRIDGE_NOTFOUND);
        }

      hsl_bridge_deinit (p_hsl_bridge_master->bridge);

      if (p_hsl_bridge_master->hw_cb && p_hsl_bridge_master->hw_cb->bridge_deinit)
        (*p_hsl_bridge_master->hw_cb->bridge_deinit) (p_hsl_bridge_master->bridge);
           
      oss_free (p_hsl_bridge_master->bridge, OSS_MEM_HEAP);
      p_hsl_bridge_master->bridge = NULL;
    }

  HSL_BRIDGE_UNLOCK;

  HSL_FN_EXIT (0);
}

/* Bridge Change Type */
int
hsl_bridge_change_vlan_type (char *name, int is_vlan_aware,
                             enum hal_bridge_type type)
{
  struct hsl_bridge *b;

  HSL_FN_ENTER ();
  HSL_BRIDGE_LOCK;

  if (! p_hsl_bridge_master->bridge)
    {
      HSL_BRIDGE_UNLOCK;
      HSL_FN_EXIT (HSL_ERR_BRIDGE_NOMEM);
    }

  if ((memcmp (p_hsl_bridge_master->bridge->name, name,
              HAL_BRIDGE_NAME_LEN)) != 0)
    {
      HSL_BRIDGE_UNLOCK;
      HSL_FN_EXIT (HSL_ERR_BRIDGE_INVALID_PARAM);
    }

  b = p_hsl_bridge_master->bridge;

  if (is_vlan_aware)
    SET_FLAG (b->flags, HSL_BRIDGE_VLAN_AWARE);
  else
    UNSET_FLAG (b->flags, HSL_BRIDGE_VLAN_AWARE);

  b->type = type;

  HSL_BRIDGE_UNLOCK;

  HSL_FN_EXIT (0);
}

/*
  Add port to a bridge.
*/
int
hsl_bridge_add_port (char *name, hsl_ifIndex_t ifindex)
{
  struct hsl_if *ifp;
  struct hsl_bridge_port *port;

  HSL_FN_ENTER ();
  HSL_BRIDGE_LOCK;
  if (! p_hsl_bridge_master->bridge && memcmp (p_hsl_bridge_master->bridge->name, name, HAL_BRIDGE_NAME_LEN))
    {
      HSL_BRIDGE_UNLOCK;
      HSL_FN_EXIT (HSL_ERR_BRIDGE_INVALID_PARAM);
    }

  ifp = hsl_ifmgr_lookup_by_index (ifindex);
  if (! ifp)
    {
      HSL_BRIDGE_UNLOCK;
      HSL_FN_EXIT (HSL_ERR_BRIDGE_INVALID_PARAM);
    }

  if (ifp->type != HSL_IF_TYPE_L2_ETHERNET)
    {
      HSL_IFMGR_IF_REF_DEC (ifp);
      HSL_BRIDGE_UNLOCK;
      HSL_FN_EXIT (HSL_ERR_BRIDGE_INVALID_PARAM);
    }

  if (ifp->u.l2_ethernet.port)
    {
      HSL_IFMGR_IF_REF_DEC (ifp);
      HSL_BRIDGE_UNLOCK;
      HSL_FN_EXIT (HSL_ERR_BRIDGE_PORT_EXISTS);
    }

  port = hsl_bridge_port_init ();
  if (! port)
    {
      HSL_IFMGR_IF_REF_DEC (ifp);
      HSL_BRIDGE_UNLOCK;
      HSL_FN_EXIT (HSL_ERR_BRIDGE_NOMEM);
    }

  ifp->u.l2_ethernet.port = port;
  port->ifp = ifp;

  port->bridge = p_hsl_bridge_master->bridge;

  /* Add port to the bridge. */
  hsl_avl_insert (p_hsl_bridge_master->bridge->port_tree, ifp);

  /* Set acceptable packet types for this interface. */
  hsl_ifmgr_set_acceptable_packet_types (ifp, HSL_IF_PKT_L2); 

#ifdef HAVE_IGMP_SNOOP
  /* Enable IGMP snooping on port */
  if (p_hsl_bridge_master->hw_cb
      && p_hsl_bridge_master->hw_cb->enable_igmp_snooping_port)
      (*p_hsl_bridge_master->hw_cb->enable_igmp_snooping_port)
         (p_hsl_bridge_master->bridge, ifp);
#endif /* HAVE_IGMP_SNOOP */

  /* Call the port add hardware callback */
  if (p_hsl_bridge_master->hw_cb
      && p_hsl_bridge_master->hw_cb->add_port_to_bridge)
      (*p_hsl_bridge_master->hw_cb->add_port_to_bridge)
         (p_hsl_bridge_master->bridge, ifp->ifindex);


  HSL_IFMGR_IF_REF_DEC (ifp);

  HSL_BRIDGE_UNLOCK;

  HSL_FN_EXIT (0);
}
/* 
   Flush port fdb 
 */ 
int 
hsl_bridge_delete_port_vlan_fdb(struct hsl_if *ifp, hsl_vid_t vid)
{
  int ret = 0;
  HSL_FN_ENTER();

/* TBD: s/w learning to implement the following features properly */
#if 0
#ifdef HAVE_L3
  struct hsl_if *ifp2;
  struct hsl_if_list *node;
  /* 
     We don't flush fdb becase arps might be hanging 
     on top of some fdb entries 
  */
  if ((ifp) && (ifp->parent_list))
  {
    node = ifp->parent_list;
    while (node)
    {
      ifp2 = node->ifp;
      if (ifp2->type == HSL_IF_TYPE_IP)
      {
        if(ifp2->u.ip.ipv4.ucAddr) 
          HSL_FN_EXIT(0);
#ifdef HAVE_IPV6                   
        if(ifp2->u.ip.ipv6.ucAddr)
          HSL_FN_EXIT(0);
#endif /* HAVE_IPV6 */
      }
      node = node->next;
    }
  }
#endif /* HAVE_L3 */
#endif /* 0 */

  if (p_hsl_bridge_master->hw_cb && p_hsl_bridge_master->hw_cb->flush_port_fdb)
    ret = (*p_hsl_bridge_master->hw_cb->flush_port_fdb) (p_hsl_bridge_master->bridge, ifp, vid);
  
  HSL_FN_EXIT(ret);
}

/*
  Delete port to a bridge.
*/
int
hsl_bridge_delete_port (char *name, hsl_ifIndex_t ifindex)
{
  struct hsl_if *ifp;
  struct hsl_bridge_port *port;

  HSL_FN_ENTER ();

  HSL_BRIDGE_LOCK;
  if (! p_hsl_bridge_master->bridge && memcmp (p_hsl_bridge_master->bridge->name, name, HAL_BRIDGE_NAME_LEN))
    {
      HSL_BRIDGE_UNLOCK;
      HSL_FN_EXIT (HSL_ERR_BRIDGE_INVALID_PARAM);
    }

  ifp = hsl_ifmgr_lookup_by_index (ifindex);
  if (! ifp)
    {
      HSL_BRIDGE_UNLOCK;
      HSL_FN_EXIT (HSL_ERR_BRIDGE_INVALID_PARAM);
    }

  if (ifp->type != HSL_IF_TYPE_L2_ETHERNET)
    {
      HSL_IFMGR_IF_REF_DEC (ifp);
      HSL_BRIDGE_UNLOCK;
      return HSL_ERR_BRIDGE_INVALID_PARAM;
    }

  if (! ifp->u.l2_ethernet.port)
    {
      HSL_IFMGR_IF_REF_DEC (ifp);
      HSL_BRIDGE_UNLOCK;
      HSL_FN_EXIT (HSL_ERR_BRIDGE_PORT_NOT_EXISTS);
    }

#ifdef HAVE_IGMP_SNOOP
  /* Disable IGMP snooping on port */
  if (p_hsl_bridge_master->hw_cb
      && p_hsl_bridge_master->hw_cb->disable_igmp_snooping_port)
      (*p_hsl_bridge_master->hw_cb->disable_igmp_snooping_port)
         (p_hsl_bridge_master->bridge, ifp);
#endif /* HAVE_IGMP_SNOOP */

  /* Call the port add hardware callback */
  if (p_hsl_bridge_master->hw_cb
      && p_hsl_bridge_master->hw_cb->delete_port_from_bridge)
      (*p_hsl_bridge_master->hw_cb->delete_port_from_bridge)
         (p_hsl_bridge_master->bridge, ifp->ifindex);

  /* Don't accept any L2 packets from this port. */
  hsl_ifmgr_unset_acceptable_packet_types (ifp, HSL_IF_PKT_L2); 

  /* Port could still want authd and lacpd packets. */
  hsl_ifmgr_set_acceptable_packet_types (ifp, 
                                         HSL_IF_PKT_EAPOL | HSL_IF_PKT_LACP); 
  /* Flush mac addresses learned from deleted port. */
  hsl_bridge_delete_port_vlan_fdb(ifp, 0);

  /* Delete interface from bridge port tree. */
  hsl_avl_delete (p_hsl_bridge_master->bridge->port_tree, ifp);

  /* Deinitialize port vlan map. */
  port = ifp->u.l2_ethernet.port;

#ifdef HAVE_VLAN
  hsl_port_vlan_map_deinit (port->vlan);
  port->vlan = NULL;
#endif /* HAVE_VLAN */

#ifdef HAVE_PROVIDER_BRIDGE

  if (port->reg_tab != NULL)
    hsl_avl_tree_free (&port->reg_tab, hsl_vlan_regis_key_deinit);

#endif /* HAVE_PROVIDER_BRIDGE */

  /* Deinitialize bridge port. */
  hsl_bridge_port_deinit (port);
  ifp->u.l2_ethernet.port = NULL;

  HSL_IFMGR_IF_REF_DEC (ifp);
  HSL_BRIDGE_UNLOCK;

  HSL_FN_EXIT (0);
}

/*
  Set L2 age timer in seconds.
*/
int
hsl_bridge_age_timer_set (char *name, int age_seconds)
{
  int ret = 0;

  HSL_FN_ENTER ();

  HSL_BRIDGE_LOCK;
  if (! p_hsl_bridge_master->bridge && memcmp (p_hsl_bridge_master->bridge->name, name, HAL_BRIDGE_NAME_LEN))
    {
      HSL_BRIDGE_UNLOCK;
      return HSL_ERR_BRIDGE_INVALID_PARAM;
    }

  if (p_hsl_bridge_master->hw_cb && p_hsl_bridge_master->hw_cb->set_age_timer)
    ret = (*p_hsl_bridge_master->hw_cb->set_age_timer) (p_hsl_bridge_master->bridge, age_seconds);

  HSL_BRIDGE_UNLOCK;

  HSL_FN_EXIT (ret);
}


/*
  Set L2 learning for a bridge.
*/
int
hsl_bridge_learning_set (char *name, int learn)
{
  HSL_FN_ENTER ();

  HSL_BRIDGE_LOCK;
  if (! p_hsl_bridge_master->bridge && memcmp (p_hsl_bridge_master->bridge->name, name, HAL_BRIDGE_NAME_LEN))
    {
      HSL_BRIDGE_UNLOCK;
      return HSL_ERR_BRIDGE_INVALID_PARAM;
    }

  if (p_hsl_bridge_master->hw_cb && p_hsl_bridge_master->hw_cb->set_learning)
    (*p_hsl_bridge_master->hw_cb->set_learning) (p_hsl_bridge_master->bridge, learn);

  HSL_BRIDGE_UNLOCK;

  HSL_FN_EXIT (0);
}

/*
  Set port state.
*/
int
hsl_bridge_set_stp_port_state (char *name, hsl_ifIndex_t ifindex, int instance, int state)
{
  struct hsl_if *ifp;

#if defined (HAVE_L3) && defined (HAVE_L2LERN)
  uint32_t old_state;
#endif /* HAVE_L3 && HAVE_L2LERN*/

  HSL_FN_ENTER ();

  HSL_BRIDGE_LOCK;
  if (! p_hsl_bridge_master->bridge && memcmp (p_hsl_bridge_master->bridge->name, name, HAL_BRIDGE_NAME_LEN))
    {
      HSL_BRIDGE_UNLOCK;
      HSL_FN_EXIT (HSL_ERR_BRIDGE_INVALID_PARAM);
    }

  ifp = hsl_ifmgr_lookup_by_index (ifindex);
  if (! ifp)
    {
      HSL_BRIDGE_UNLOCK;
      HSL_FN_EXIT (HSL_ERR_BRIDGE_INVALID_PARAM);
    }

  if (ifp->type != HSL_IF_TYPE_L2_ETHERNET)
    {
      HSL_IFMGR_IF_REF_DEC (ifp);
      HSL_BRIDGE_UNLOCK;
      HSL_FN_EXIT (HSL_ERR_BRIDGE_INVALID_PARAM);
    }

  if (! ifp->u.l2_ethernet.port)
    {
      HSL_IFMGR_IF_REF_DEC (ifp);
      HSL_BRIDGE_UNLOCK;
      HSL_FN_EXIT (HSL_ERR_BRIDGE_PORT_NOT_EXISTS);
    }

#if defined (HAVE_L3) && defined (HAVE_L2LERN)
  old_state = ifp->u.l2_ethernet.port->stp_port_state;
#endif /* HAVE_L3 && HAVE_L2LERN */

  if (p_hsl_bridge_master->hw_cb && p_hsl_bridge_master->hw_cb->set_stp_port_state)
    (*p_hsl_bridge_master->hw_cb->set_stp_port_state) (p_hsl_bridge_master->bridge, ifp->u.l2_ethernet.port, instance, state);

#if defined (HAVE_L3) && defined (HAVE_L2LERN)
  if (old_state == HAL_BR_PORT_STATE_FORWARDING &&
      (state == HAL_BR_PORT_STATE_BLOCKING
       || state == HAL_BR_PORT_STATE_DISABLED))
    hsl_fib_handle_l2_port_state_change (ifp);
#endif /* HAVE_L3 && HAVE_L2LERN */

  HSL_IFMGR_IF_REF_DEC (ifp);
  HSL_BRIDGE_UNLOCK;

  HSL_FN_EXIT (0);
}

/*
  Add instance to a bridge.
*/
int
hsl_bridge_add_instance (char *name, int instance)
{
  HSL_FN_ENTER ();

  HSL_BRIDGE_LOCK;
  if (! p_hsl_bridge_master->bridge && memcmp (p_hsl_bridge_master->bridge->name, name, HAL_BRIDGE_NAME_LEN))
    {
      HSL_BRIDGE_UNLOCK;
      return HSL_ERR_BRIDGE_INVALID_PARAM;
    }

  if (p_hsl_bridge_master->hw_cb && p_hsl_bridge_master->hw_cb->add_instance)
    (*p_hsl_bridge_master->hw_cb->add_instance) (p_hsl_bridge_master->bridge, instance);

  HSL_BRIDGE_UNLOCK;

  HSL_FN_EXIT (0);
}

/*
  Delete instance.
*/
int
hsl_bridge_delete_instance (char *name, int instance)
{
  HSL_FN_ENTER ();

  HSL_BRIDGE_LOCK;
  if (! p_hsl_bridge_master->bridge && memcmp (p_hsl_bridge_master->bridge->name, name, HAL_BRIDGE_NAME_LEN))
    {
      HSL_BRIDGE_UNLOCK;
      return HSL_ERR_BRIDGE_INVALID_PARAM;
    }

  if (p_hsl_bridge_master->hw_cb && p_hsl_bridge_master->hw_cb->delete_instance)
    (*p_hsl_bridge_master->hw_cb->delete_instance) (p_hsl_bridge_master->bridge, instance);

  HSL_BRIDGE_UNLOCK;

  HSL_FN_EXIT (0);
}

/*
  Add VLAN to instance.
*/

static s_int32_t
hsl_insert_vid_into_inst_table (struct hsl_bridge *br, hsl_vid_t vid,
                                u_int32_t vid_state, u_int32_t inst)
{

  struct hsl_vid_entry *vid_entry = NULL;

  HSL_FN_ENTER ();
  vid_entry = oss_malloc (sizeof (struct hsl_vid_entry), OSS_MEM_HEAP);
  if (!vid_entry)
    {
      HSL_FN_EXIT (0);
    }

  vid_entry->prev = NULL;
  vid_entry->next = NULL;
  vid_entry->vid  = vid;
  vid_entry->vid_state = vid_state;

  if (br->inst_table[inst])
    {
      br->inst_table[inst]->prev = vid_entry;
      vid_entry->next = br->inst_table[inst];
    }
  br->inst_table[inst] = vid_entry;

  HSL_FN_EXIT (0);
}

/*
  Delete VLAN from instance.
*/
static s_int32_t
hsl_delete_vid_from_inst_table (struct hsl_bridge *br, hsl_vid_t vid,
                                u_int32_t inst)
{
  struct hsl_vid_entry *vid_entry;

  HSL_FN_ENTER ();

  vid_entry = br->inst_table[inst];


  if (!vid_entry)
    {
      HSL_FN_EXIT (0);
    }

  while (vid_entry)
    {
      if (vid_entry->vid == vid)
        {
          if (vid_entry->next)
            {
              vid_entry->next->prev = vid_entry->prev;
            }

          if (vid_entry->prev)
            {
              vid_entry->prev->next = vid_entry->next;
            }
          else
            {
              br->inst_table[inst] = vid_entry->next;
            }
         
          break;
        }

      vid_entry = vid_entry->next;
    }

  if (!vid_entry)
    {
      HSL_FN_EXIT (0);
    }

  oss_free (vid_entry, OSS_MEM_HEAP);

  HSL_FN_EXIT (0);
}

int
hsl_bridge_add_vlan_to_inst (char *name, int instance, hsl_vid_t vid)
{
  struct hsl_vid_entry *vid_entry;
  int inst_index;

  HSL_FN_ENTER ();
	

  HSL_BRIDGE_LOCK;
  if (! p_hsl_bridge_master->bridge && memcmp (p_hsl_bridge_master->bridge->name, name, HAL_BRIDGE_NAME_LEN))
    {
      HSL_BRIDGE_UNLOCK;
      return HSL_ERR_BRIDGE_INVALID_PARAM;
    }

  /* check is done to find if the same vid is already configured on other instance */
  for (inst_index = 0; inst_index < HSL_MAX_MSTP_INSTANCES; inst_index++)
    {
      vid_entry = p_hsl_bridge_master->bridge->inst_table[inst_index]; 

      while (vid_entry)
        {
          if (vid == vid_entry->vid)
            {
              hsl_delete_vid_from_inst_table (p_hsl_bridge_master->bridge, vid, inst_index);
              
              break;
            }
          else
            vid_entry = vid_entry->next;
        }
     }

  hsl_insert_vid_into_inst_table (p_hsl_bridge_master->bridge, vid, 0, instance);

#ifdef HAVE_VLAN
  if (p_hsl_bridge_master->hw_cb && p_hsl_bridge_master->hw_cb->add_vlan_to_instance)
    (*p_hsl_bridge_master->hw_cb->add_vlan_to_instance) (p_hsl_bridge_master->bridge, instance, vid);
#endif /* HAVE_VLAN */

  HSL_BRIDGE_UNLOCK;

  HSL_FN_EXIT (0);
}

/*
  Delete VLAN to instance.
*/
int
hsl_bridge_delete_vlan_from_inst (char *name, int instance, hsl_vid_t vid)
{
  HSL_FN_ENTER ();

  HSL_BRIDGE_LOCK;
  if (! p_hsl_bridge_master->bridge && memcmp (p_hsl_bridge_master->bridge->name, name, HAL_BRIDGE_NAME_LEN))
    {
      HSL_BRIDGE_UNLOCK;
      return HSL_ERR_BRIDGE_INVALID_PARAM;
    }

#ifdef HAVE_VLAN
  if (p_hsl_bridge_master->hw_cb && p_hsl_bridge_master->hw_cb->delete_vlan_from_instance)
    (*p_hsl_bridge_master->hw_cb->delete_vlan_from_instance) (p_hsl_bridge_master->bridge, instance, vid);
#endif /* HAVE_VLAN */

  hsl_delete_vid_from_inst_table (p_hsl_bridge_master->bridge, vid, instance);

  HSL_BRIDGE_UNLOCK;

  HSL_FN_EXIT (0);
}

/*
  Initialize IGMP Snooping.
*/
int
hsl_bridge_enable_igmp_snooping (char *name)
{
  HSL_FN_ENTER ();

  HSL_BRIDGE_LOCK;
  if (! p_hsl_bridge_master->bridge && memcmp (p_hsl_bridge_master->bridge->name, name, HAL_BRIDGE_NAME_LEN))
    {
      HSL_BRIDGE_UNLOCK;
      return HSL_ERR_BRIDGE_INVALID_PARAM;
    }

  if (p_hsl_bridge_master->hw_cb && p_hsl_bridge_master->hw_cb->enable_igmp_snooping)
    (*p_hsl_bridge_master->hw_cb->enable_igmp_snooping) (p_hsl_bridge_master->bridge);

  HSL_BRIDGE_UNLOCK;

  HSL_FN_EXIT (0);
}

/*
  Disable IGMP Snooping.
*/
int
hsl_bridge_disable_igmp_snooping (char *name)
{
  HSL_FN_ENTER ();

  HSL_BRIDGE_LOCK;
  if (! p_hsl_bridge_master->bridge && memcmp (p_hsl_bridge_master->bridge->name, name, HAL_BRIDGE_NAME_LEN))
    {
      HSL_BRIDGE_UNLOCK;
      return HSL_ERR_BRIDGE_INVALID_PARAM;
    }

  if (p_hsl_bridge_master->hw_cb && p_hsl_bridge_master->hw_cb->disable_igmp_snooping)
    (*p_hsl_bridge_master->hw_cb->disable_igmp_snooping) (p_hsl_bridge_master->bridge);

  HSL_BRIDGE_UNLOCK;

  HSL_FN_EXIT (0);
}

#if defined HAVE_IGMP_SNOOP || defined HAVE_MLD_SNOOP
/*
   Sets IGMP Snooping L2 Unknown mcast mode.
*/
int
hsl_bridge_l2_unknown_mcast_mode (int mode)
{
  struct hsl_vlan_port *vlan;
  struct hsl_avl_node *node;
    
  HSL_FN_ENTER ();
 
  HSL_BRIDGE_LOCK;
  if (! p_hsl_bridge_master->bridge)
    {
      HSL_BRIDGE_UNLOCK;
      return HSL_ERR_BRIDGE_INVALID_PARAM;
    }
  
  p_hsl_bridge_master->hsl_l2_unknown_mcast  = mode;
  
  if (p_hsl_bridge_master->bridge->vlan_tree)
    {
       node = hsl_avl_top(p_hsl_bridge_master->bridge->vlan_tree);
       for (; node; node = hsl_avl_next(node))
          {
             vlan = HSL_AVL_NODE_INFO (node);
             if (vlan)
               {
                  if (p_hsl_bridge_master->hw_cb &&
                    p_hsl_bridge_master->hw_cb->l2_unknown_mcast_mode)
                    (*p_hsl_bridge_master->hw_cb->l2_unknown_mcast_mode)(vlan->vid, mode);
               }
          }
     }
   else
     {
        HSL_BRIDGE_UNLOCK;
        return HSL_ERR_BRIDGE_INVALID_PARAM;
     }
      
  HSL_BRIDGE_UNLOCK;

  HSL_FN_EXIT (0);
}

#endif /* HAVE_IGMP_SNOOP || HAVE_MLD_SNOOP */

/* Enable/Disable GVRP  */
int
hsl_bridge_gxrp_enable (char *bridge_name, unsigned long type, int enable)
{
  struct hsl_bridge *bridge;
  HSL_FN_ENTER();
  HSL_BRIDGE_LOCK;
  bridge = p_hsl_bridge_master->bridge;
  if(!bridge)
    {
      HSL_BRIDGE_UNLOCK;
      HSL_FN_EXIT(-1);
    }

  if (enable)
     bridge->gxrp_enable = (bridge->gxrp_enable | HSL_BRIDGE_GVRP_ENABLED) ;
  else
     bridge->gxrp_enable = (bridge->gxrp_enable & (~ HSL_BRIDGE_GVRP_ENABLED));

  if (p_hsl_bridge_master->hw_cb && p_hsl_bridge_master->hw_cb->gxrp_enable)
      (*p_hsl_bridge_master->hw_cb->gxrp_enable)
                         (bridge, type, bridge->gxrp_enable);

  HSL_BRIDGE_UNLOCK;
  HSL_FN_EXIT(0);
  return 0;
}

/*
  Initialize MLD Snooping.
*/
int
hsl_bridge_enable_mld_snooping (char *name)
{
  HSL_FN_ENTER ();

  HSL_BRIDGE_LOCK;
  if (! p_hsl_bridge_master->bridge && memcmp (p_hsl_bridge_master->bridge->name, name, HAL_BRIDGE_NAME_LEN))
    {
      HSL_BRIDGE_UNLOCK;
      return HSL_ERR_BRIDGE_INVALID_PARAM;
    }

  if (p_hsl_bridge_master->hw_cb && p_hsl_bridge_master->hw_cb->enable_mld_snooping)
    (*p_hsl_bridge_master->hw_cb->enable_mld_snooping) (p_hsl_bridge_master->bridge);

  HSL_BRIDGE_UNLOCK;

  HSL_FN_EXIT (0);
}

/*
  Disable MLD Snooping.
*/
int
hsl_bridge_disable_mld_snooping (char *name)
{
  HSL_FN_ENTER ();

  HSL_BRIDGE_LOCK;
  if (! p_hsl_bridge_master->bridge && memcmp (p_hsl_bridge_master->bridge->name, name, HAL_BRIDGE_NAME_LEN))
    {
      HSL_BRIDGE_UNLOCK;
      return HSL_ERR_BRIDGE_INVALID_PARAM;
    }

  if (p_hsl_bridge_master->hw_cb && p_hsl_bridge_master->hw_cb->disable_mld_snooping)
    (*p_hsl_bridge_master->hw_cb->disable_mld_snooping) (p_hsl_bridge_master->bridge);

  HSL_BRIDGE_UNLOCK;

  HSL_FN_EXIT (0);
}

/*
  Rate limiting for broadcast. 
*/
int
hsl_bridge_ratelimit_bcast (hsl_ifIndex_t ifindex, int level,
                            int fraction)
{
  struct hsl_if *ifp;

  HSL_FN_ENTER ();

  ifp = hsl_ifmgr_lookup_by_index (ifindex);
  if (! ifp)
    {
      HSL_FN_EXIT (HSL_ERR_BRIDGE_INVALID_PARAM);
    }

  if (p_hsl_bridge_master->hw_cb && p_hsl_bridge_master->hw_cb->ratelimit_bcast)
    (*p_hsl_bridge_master->hw_cb->ratelimit_bcast) (ifp, level, fraction);

  HSL_IFMGR_IF_REF_DEC (ifp);

  HSL_FN_EXIT (0);
}

/*
  Get broadcast discards.
*/
int
hsl_bridge_ratelimit_get_bcast_discards (hsl_ifIndex_t ifindex, int *discards)
{
  struct hsl_if *ifp;

  HSL_FN_ENTER ();

  ifp = hsl_ifmgr_lookup_by_index (ifindex);
  if (! ifp)
    {
      HSL_FN_EXIT (HSL_ERR_BRIDGE_INVALID_PARAM);
    }

  if (p_hsl_bridge_master->hw_cb && p_hsl_bridge_master->hw_cb->ratelimit_bcast_discards_get)
    (*p_hsl_bridge_master->hw_cb->ratelimit_bcast_discards_get) (ifp, discards);

  HSL_IFMGR_IF_REF_DEC (ifp);

  HSL_FN_EXIT (0);
}

/*
  Rate limiting for multicast. 
*/
int
hsl_bridge_ratelimit_mcast (hsl_ifIndex_t ifindex, int level,
                            int fraction)
{
  struct hsl_if *ifp;

  HSL_FN_ENTER ();

  ifp = hsl_ifmgr_lookup_by_index (ifindex);
  if (! ifp)
    {
      HSL_FN_EXIT (HSL_ERR_BRIDGE_INVALID_PARAM);
    }

  if (p_hsl_bridge_master->hw_cb && p_hsl_bridge_master->hw_cb->ratelimit_mcast)
    (*p_hsl_bridge_master->hw_cb->ratelimit_mcast) (ifp, level, fraction);

  HSL_IFMGR_IF_REF_DEC (ifp);

  HSL_FN_EXIT (0);
}

/*
  Get multicast discards.
*/
int
hsl_bridge_ratelimit_get_mcast_discards (hsl_ifIndex_t ifindex, int *discards)
{
  struct hsl_if *ifp;

  HSL_FN_ENTER ();

  ifp = hsl_ifmgr_lookup_by_index (ifindex);
  if (! ifp)
    {
      HSL_FN_EXIT (HSL_ERR_BRIDGE_INVALID_PARAM);
    }

  if (p_hsl_bridge_master->hw_cb && p_hsl_bridge_master->hw_cb->ratelimit_mcast_discards_get)
    (*p_hsl_bridge_master->hw_cb->ratelimit_mcast_discards_get) (ifp, discards);

  HSL_IFMGR_IF_REF_DEC (ifp);

  HSL_FN_EXIT (0);
}

/*
  Rate limiting for dlf broadcast. 
*/
int
hsl_bridge_ratelimit_dlf_bcast (hsl_ifIndex_t ifindex, int level,
                                int fraction)
{
  struct hsl_if *ifp;

  HSL_FN_ENTER ();

  ifp = hsl_ifmgr_lookup_by_index (ifindex);
  if (! ifp)
    {
      HSL_FN_EXIT (HSL_ERR_BRIDGE_INVALID_PARAM);
    }

  if (p_hsl_bridge_master->hw_cb && p_hsl_bridge_master->hw_cb->ratelimit_dlf_bcast)
    (*p_hsl_bridge_master->hw_cb->ratelimit_dlf_bcast) (ifp, level, fraction);

  HSL_IFMGR_IF_REF_DEC (ifp);

  HSL_FN_EXIT (0);
}

/*
  Get dlf broadcast discards.
*/
int
hsl_bridge_ratelimit_get_dlf_bcast_discards (hsl_ifIndex_t ifindex, int *discards)
{
  struct hsl_if *ifp;

  HSL_FN_ENTER ();

  ifp = hsl_ifmgr_lookup_by_index (ifindex);
  if (! ifp)
    {
      HSL_FN_EXIT (HSL_ERR_BRIDGE_INVALID_PARAM);
    }

  if (p_hsl_bridge_master->hw_cb && p_hsl_bridge_master->hw_cb->ratelimit_dlf_bcast_discards_get)
    (*p_hsl_bridge_master->hw_cb->ratelimit_dlf_bcast_discards_get) (ifp, discards);

  HSL_IFMGR_IF_REF_DEC (ifp);

  HSL_FN_EXIT (0);
}

/*
  Initialize flow control.
*/
int
hsl_bridge_init_flowcontrol (void)
{
  return 0;
}

/*
  Deinitialize flow control.
*/
int
hsl_bridge_deinit_flowcontrol (void)
{
  return 0;
}

/*
  Set flowcontrol direction.
*/
int
hsl_bridge_set_flowcontrol (hsl_ifIndex_t ifindex, u_char direction)
{
  struct hsl_if *ifp;

  HSL_FN_ENTER ();

  ifp = hsl_ifmgr_lookup_by_index (ifindex);
  if (! ifp)
    {
      HSL_FN_EXIT (HSL_ERR_BRIDGE_INVALID_PARAM);
    }

  if (p_hsl_bridge_master->hw_cb && p_hsl_bridge_master->hw_cb->set_flowcontrol)
    (*p_hsl_bridge_master->hw_cb->set_flowcontrol) (ifp, direction);

  HSL_IFMGR_IF_REF_DEC (ifp);

  HSL_FN_EXIT (0);
}

/*
  Get flowcontrol staticsics.
*/
int
hsl_bridge_flowcontrol_statistics (hsl_ifIndex_t ifindex, u_char *direction,
                                   int *rxpause, int *txpause)
{
  struct hsl_if *ifp;

  HSL_FN_ENTER ();

  ifp = hsl_ifmgr_lookup_by_index (ifindex);
  if (! ifp)
    {
      HSL_FN_EXIT (HSL_ERR_BRIDGE_INVALID_PARAM);
    }

  if (p_hsl_bridge_master->hw_cb && p_hsl_bridge_master->hw_cb->get_flowcontrol_statistics)
    (*p_hsl_bridge_master->hw_cb->get_flowcontrol_statistics) (ifp, direction, rxpause, txpause);

  HSL_IFMGR_IF_REF_DEC (ifp);

  HSL_FN_EXIT (0);
}

/*
  Add user defined LLDP/CFM MAC address to FDB Table
*/
int
hsl_bridge_set_proto_dest_addr (u_int8_t *dest_addr,
                                enum hal_l2_proto proto)
{
  int ret = STATUS_ERROR;

  HSL_FN_ENTER ();

  ret = STATUS_OK;

  HSL_BRIDGE_LOCK;

  if (p_hsl_bridge_master->hw_cb && 
      p_hsl_bridge_master->hw_cb->set_proto_dest_mac)
    ret = (*p_hsl_bridge_master->hw_cb->set_proto_dest_mac)
                                           (dest_addr, proto);

  if (ret == STATUS_OK)
    memcpy (p_hsl_bridge_master->dest_addr [proto],
                 dest_addr, HSL_ETHER_ALEN);

  HSL_BRIDGE_UNLOCK;
 
  HSL_FN_EXIT (ret);
}

/*
  set the Ethernet Type of the protocol.
*/
int
hsl_bridge_set_proto_ether_type (u_int16_t ether_type,
                                 enum hal_l2_proto proto)
{
  HSL_FN_ENTER ();

  HSL_BRIDGE_LOCK;

#ifdef HAVE_ONMD
  if (proto == HAL_PROTO_CFM)
    p_hsl_bridge_master->cfm_ether_type = ether_type;
#endif /* HAVE_ONMD */

  HSL_BRIDGE_UNLOCK;

  HSL_FN_EXIT (STATUS_OK);
}

#ifdef HAVE_ONMD
/*
  Trap a CFM PDU of particular level.
*/
int
hsl_bridge_set_cfm_trap_level_pdu (u_int8_t level,
                                   enum hal_cfm_pdu_type pdu)
{
  int ret;

  HSL_FN_ENTER ();

  ret = STATUS_OK;

  HSL_BRIDGE_LOCK;

  if (p_hsl_bridge_master->hw_cb &&
      p_hsl_bridge_master->hw_cb->cfm_enable_level)
    ret = (*p_hsl_bridge_master->hw_cb->cfm_enable_level)
                                           (level, pdu);

  if (pdu == HSL_CFM_CCM_FRAME)
    SET_FLAG (p_hsl_bridge_master->cfm_cc_levels, (1 << level));
  else
    SET_FLAG (p_hsl_bridge_master->cfm_tr_levels, (1 << level));

  HSL_BRIDGE_UNLOCK;

  HSL_FN_EXIT (ret);
}

/*
  Trap a CFM PDU of particular level.
*/
int
hsl_bridge_unset_cfm_trap_level_pdu (u_int8_t level,
                                     enum hal_cfm_pdu_type pdu)
{
  int ret;

  HSL_FN_ENTER ();

  ret = STATUS_OK;

  HSL_BRIDGE_LOCK;

  if (p_hsl_bridge_master->hw_cb &&
      p_hsl_bridge_master->hw_cb->cfm_disable_level)
    ret = (*p_hsl_bridge_master->hw_cb->cfm_disable_level)
                                           (level, pdu);

  if (pdu == HSL_CFM_CCM_FRAME)
    UNSET_FLAG (p_hsl_bridge_master->cfm_cc_levels, (1 << level));
  else
    UNSET_FLAG (p_hsl_bridge_master->cfm_tr_levels, (1 << level));

  HSL_BRIDGE_UNLOCK;

  HSL_FN_EXIT (ret);
}
#endif /* HAVE_ONMD */

/*
  Add FDB entry.
*/
int
hsl_bridge_add_fdb (char *name, hsl_ifIndex_t ifindex, char *mac, int len, hsl_vid_t vid, 
                    u_char flags, int is_forward)
{
  struct hsl_if *ifp;
  int ret = 0;

  HSL_FN_ENTER ();

  HSL_BRIDGE_LOCK;
  if (! p_hsl_bridge_master->bridge && memcmp (p_hsl_bridge_master->bridge->name, name, HAL_BRIDGE_NAME_LEN))
    {
      HSL_BRIDGE_UNLOCK;
      HSL_FN_EXIT (HSL_ERR_BRIDGE_INVALID_PARAM);
    }

  ifp = hsl_ifmgr_lookup_by_index (ifindex);
  if (! ifp)
    {
      HSL_BRIDGE_UNLOCK;
      HSL_FN_EXIT (HSL_ERR_BRIDGE_INVALID_PARAM);
    }

  if (p_hsl_bridge_master->hw_cb && p_hsl_bridge_master->hw_cb->add_fdb)
    ret = (*p_hsl_bridge_master->hw_cb->add_fdb) (p_hsl_bridge_master->bridge, ifp, (u_char*)mac, len, vid, flags, is_forward);

  HSL_IFMGR_IF_REF_DEC (ifp);
  HSL_BRIDGE_UNLOCK;

  HSL_FN_EXIT (ret);
}

/* 
   Delete FDB entry.
*/
int
hsl_bridge_delete_fdb (char *name, hsl_ifIndex_t ifindex, char *mac, int len, hsl_vid_t vid,
                       u_char flags)
{
  struct hsl_if *ifp;
  int ret = 0;

  HSL_FN_ENTER ();

  HSL_BRIDGE_LOCK;
  if (! p_hsl_bridge_master->bridge && memcmp (p_hsl_bridge_master->bridge->name, name, HAL_BRIDGE_NAME_LEN))
    {
      HSL_BRIDGE_UNLOCK;
      HSL_FN_EXIT (HSL_ERR_BRIDGE_INVALID_PARAM);
    }

  ifp = hsl_ifmgr_lookup_by_index (ifindex);
  if (! ifp)
    {
      HSL_BRIDGE_UNLOCK;
      HSL_FN_EXIT (HSL_ERR_BRIDGE_INVALID_PARAM);
    }

  if (p_hsl_bridge_master->hw_cb && p_hsl_bridge_master->hw_cb->delete_fdb)
    ret = (*p_hsl_bridge_master->hw_cb->delete_fdb) (p_hsl_bridge_master->bridge, ifp, (u_char*)mac, len, vid, flags);

  HSL_IFMGR_IF_REF_DEC (ifp);
  HSL_BRIDGE_UNLOCK;

  HSL_FN_EXIT (ret);
}

/*
  Get Unicast FDB entry.
*/
int
hsl_bridge_unicast_get_fdb (struct hal_msg_l2_fdb_entry_req *req,
                            struct hal_msg_l2_fdb_entry_resp *resp)
                           
{
  int ret = 0;

  HSL_FN_ENTER ();

  if (p_hsl_bridge_master->hw_cb && p_hsl_bridge_master->hw_cb->get_uni_fdb)
    ret = (*p_hsl_bridge_master->hw_cb->get_uni_fdb)(req, resp);

  HSL_FN_EXIT (ret);
}


/* TBD */
/*
  Get Multicast FDB entry.
*/
int
hsl_bridge_multicast_get_fdb (struct hal_msg_l2_fdb_entry_req *req,
                              struct hal_msg_l2_fdb_entry_resp *resp)
{
  return 0;
}

/* 
   Flush FDB for port.
*/
int
hsl_bridge_flush_fdb (char *name, hsl_ifIndex_t ifindex, int instance, hsl_vid_t vid)
{
  struct hsl_if *ifp;
  int ret = 0;
  struct hsl_vid_entry *vid_entry;

  HSL_FN_ENTER ();

  HSL_BRIDGE_LOCK;
  if (! p_hsl_bridge_master->bridge || memcmp (p_hsl_bridge_master->bridge->name, name, HAL_BRIDGE_NAME_LEN))
    {
      HSL_BRIDGE_UNLOCK;
      HSL_FN_EXIT (HSL_ERR_BRIDGE_INVALID_PARAM);
    }

  ifp = hsl_ifmgr_lookup_by_index (ifindex);
  
  if (instance)
    {  
      /* check for the vids in the passed instance if instance!=0
         and call them in a loop */
      vid_entry = p_hsl_bridge_master->bridge->inst_table[instance]; 

      while (vid_entry)
        { 
          if (vid_entry->vid)
            {
              ret += hsl_bridge_delete_port_vlan_fdb(ifp, vid_entry->vid);
              /*comment */
            }

          vid_entry = vid_entry->next;
        }
    }
  else
    ret = hsl_bridge_delete_port_vlan_fdb(ifp, vid);

  if(ifp)
    HSL_IFMGR_IF_REF_DEC (ifp);

  HSL_BRIDGE_UNLOCK;

  HSL_FN_EXIT (ret);
}

/* 
   Flush FDB for mac.
*/
int
hsl_bridge_flush_fdb_by_mac (char *name, char *mac, int len, int flags)
{
  int ret = 0;

  HSL_FN_ENTER ();

  HSL_BRIDGE_LOCK;
  if (! p_hsl_bridge_master->bridge && memcmp (p_hsl_bridge_master->bridge->name, name, HAL_BRIDGE_NAME_LEN))
    {
      HSL_BRIDGE_UNLOCK;
      HSL_FN_EXIT (HSL_ERR_BRIDGE_INVALID_PARAM);
    }

  if (p_hsl_bridge_master->hw_cb && p_hsl_bridge_master->hw_cb->flush_fdb_by_mac)
    ret = (*p_hsl_bridge_master->hw_cb->flush_fdb_by_mac) (p_hsl_bridge_master->bridge, (u_char*)mac, len, flags);

  HSL_BRIDGE_UNLOCK;

  HSL_FN_EXIT (ret);
}

/* 
 * FOR UT
 *  Set EFM symbol period window
 *  
int
hsl_bridge_symbol_period_window_set(u_int32_t index, u_int64_t* sym_window)
{
  int ret = 0;

  HSL_FN_ENTER ();

  if (p_hsl_bridge_master->hw_cb && 
      p_hsl_bridge_master->hw_cb->hsl_bcm_symbol_period_window_set)
  {
    ret = (*p_hsl_bridge_master->hw_cb->hsl_bcm_symbol_period_window_set)
                                       (index, sym_window);
  }

  HSL_FN_EXIT (ret);

}

 Set EFM frame period window
int
hsl_bridge_frame_period_window_set(u_int32_t index, u_int32_t fp_window)
{
  int ret = 0;

  HSL_FN_ENTER ();

  if (p_hsl_bridge_master->hw_cb &&
      p_hsl_bridge_master->hw_cb->hsl_bcm_frame_period_window_set)
  {
    ret = (*p_hsl_bridge_master->hw_cb->hsl_bcm_frame_period_window_set)
                                      (index, fp_window);
  }

  HSL_FN_EXIT (ret);

}

 Get EFM error frame count
int
hsl_bridge_efm_get_error_frames(u_int32_t index, 
                                struct hal_msg_efm_err_frames_resp *resp)
{
  int ret = 0;

  HSL_FN_ENTER ();

  if (p_hsl_bridge_master->hw_cb &&
      p_hsl_bridge_master->hw_cb->hsl_bcm_frame_error_get)
  {
    ret = (*p_hsl_bridge_master->hw_cb->hsl_bcm_frame_error_get)(index,
                                                                 resp);
  }

  HSL_FN_EXIT (ret);
}

 Get EFM error frame seconds count
int
hsl_bridge_efm_get_error_frame_secs(u_int32_t index, 
                                struct hal_msg_efm_err_frame_secs_resp *resp)
{
  int ret = 0;

  HSL_FN_ENTER ();

  if (p_hsl_bridge_master->hw_cb &&
      p_hsl_bridge_master->hw_cb->hsl_bcm_frame_error_seconds_get)
  {
    ret = (*p_hsl_bridge_master->hw_cb->hsl_bcm_frame_error_seconds_get)(index,
                                                                         resp);
  }

  HSL_FN_EXIT (ret);
}
*/

/*
  Set the port state for Remote Loopback operation
*/
#ifdef HAVE_ONMD

int
hsl_bridge_efm_set_port_state(unsigned int index,
                        enum hal_efm_par_action local_par_action,
                        enum hal_efm_mux_action local_mux_action)
{
  int ret = 0;

  HSL_FN_ENTER ();
  /*
  if (p_hsl_bridge_master->hw_cb &&
      p_hsl_bridge_master->hw_cb->efm_set_port_state)
  {
     ret = (*p_hsl_bridge_master->hw_cb->efm_set_port_state)(index, 
                                          local_par_action, local_mux_action); 
  }*/  
  HSL_FN_EXIT (ret);
} 

#endif /*HAVE_ONMD*/

/* 
   Register hardware callbacks.
*/
int
hsl_bridge_hw_cb_register (struct hsl_l2_hw_callbacks *cb)
{
  HSL_FN_ENTER ();

  HSL_BRIDGE_LOCK;
  if (p_hsl_bridge_master->hw_cb)
    {
      HSL_BRIDGE_UNLOCK;
      HSL_FN_EXIT (HSL_ERR_BRIDGE_HWCB_ALREADY_REGISTERED);
    }
  p_hsl_bridge_master->hw_cb = cb;
  HSL_BRIDGE_UNLOCK;
  HSL_FN_EXIT (0);
}


int
hsl_bridge_priority_ovr (char *name, hsl_ifIndex_t ifindex, char *mac,
                         int len, hsl_vid_t vid,
                         enum hal_bridge_pri_ovr_mac_type ovr_mac_type,
                         u_int8_t priority)
{
  struct hsl_if *ifp;
  int ret = 0;

  HSL_FN_ENTER ();

  HSL_BRIDGE_LOCK;
  if (! p_hsl_bridge_master->bridge && memcmp (p_hsl_bridge_master->bridge->name, name, HAL_BRIDGE_NAME_LEN))
    {
      HSL_BRIDGE_UNLOCK;
      HSL_FN_EXIT (HSL_ERR_BRIDGE_INVALID_PARAM);
    }

  ifp = hsl_ifmgr_lookup_by_index (ifindex);
  if (! ifp)
    {
      HSL_BRIDGE_UNLOCK;
      HSL_FN_EXIT (HSL_ERR_BRIDGE_INVALID_PARAM);
    }

  if (p_hsl_bridge_master->hw_cb && p_hsl_bridge_master->hw_cb->set_mac_prio_ovr)
    ret = (*p_hsl_bridge_master->hw_cb->set_mac_prio_ovr) 
             (p_hsl_bridge_master->bridge, ifp, (u_char*)mac, len, vid,
              ovr_mac_type, priority);

  HSL_IFMGR_IF_REF_DEC (ifp);
  HSL_BRIDGE_UNLOCK;

  HSL_FN_EXIT (ret);
}

#endif /* HAVE_L2 */

