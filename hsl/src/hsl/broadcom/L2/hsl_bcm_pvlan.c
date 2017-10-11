/* Copyright (C) 2005  All Rights Reserved. */
#include "config.h"
#include "hsl_os.h"
#include "hsl_types.h"
                                                                                
#include "bcm_incl.h"
                                                                                
#ifdef HAVE_PVLAN

#include "hal_types.h"
#include "hal_l2.h"
#include "hal_msg.h"
                                                                                
#include "hsl_oss.h"
#include "hsl_ifmgr.h"
#include "hsl_logger.h"
                                                                                
#include "hsl_avl.h"
#include "hsl_vlan.h"
#include "hsl_bridge.h"

#include "hsl_bcm.h"
#include "hsl_bcm_if.h"
#include "hsl_bcm_l2.h"
#include "hsl_bcm_pvlan.h"

static HSL_BOOL bcm_pvlan_init = HSL_FALSE;
static int bcm_pvlan_filter_type = 0;

struct hsl_bcm_pvlan *
hsl_bcm_pvlan_alloc(void)
{
  struct hsl_bcm_pvlan *bcm_vlan;
  
  bcm_vlan = (struct hsl_bcm_pvlan *)(oss_malloc(sizeof (struct hsl_bcm_pvlan), 
                                               OSS_MEM_HEAP));
  if (bcm_vlan)
    {
      memset (bcm_vlan, 0, sizeof (struct hsl_bcm_pvlan));
    }
  
  return bcm_vlan;
}

int
hsl_bcm_pvlan_filter_type_get(void)
{
  HSL_FN_ENTER();
  HSL_FN_EXIT(bcm_pvlan_filter_type);
}

static int
_hsl_bcm_pvlan_init(void)
{
  int ret;
  
  HSL_FN_ENTER();
  
  /* Attempt to initialise field filtering */
  ret = bcmx_field_init();
  if (ret == BCM_E_NONE)
    {
      bcm_pvlan_filter_type = HSL_BCM_FEATURE_FIELD;
      bcm_pvlan_init = HSL_TRUE;
      HSL_FN_EXIT(0);
    }
  
  ret = bcmx_filter_init();
  if (ret == BCM_E_NONE)
    {
      bcm_pvlan_filter_type = HSL_BCM_FEATURE_FILTER;
      bcm_pvlan_init = HSL_TRUE;
      HSL_FN_EXIT(0);
    }
  
  HSL_FN_EXIT (ret);
}

int _hsl_bcm_vlan_set(struct hsl_bcm_pvlan *bcm_vlan,
                      hsl_vid_t vid,
                      enum hal_pvlan_type vlan_type)
{
  int ret;
  bcm_field_group_t group;
  bcm_field_entry_t entry;
  int group_created = 0;
  int entry_created = 0;
  
  if (bcm_pvlan_init == HSL_FALSE)
    {
      ret = _hsl_bcm_pvlan_init();
      if (ret < 0)
        HSL_FN_EXIT (ret);
    }

  if (bcm_vlan->filter_type == HSL_BCM_FEATURE_FILTER)
    {
      bcm_filterid_t filter;
      int filter_created = 0;

      ret = bcmx_filter_create (&filter);
      if (ret != BCM_E_NONE)
        HSL_FN_EXIT (ret);

      filter_created = 1;

      ret = bcmx_filter_qualify_data16 (filter, HSL_PVLAN_OFFSET, vid,
                                                HSL_PVLAN_MASK);
      if (ret != BCM_E_NONE)
        {
          bcmx_filter_remove (filter);
          bcmx_filter_destroy (filter);
          HSL_FN_EXIT (ret);
        }

      bcm_vlan->u.filter = filter;
    }
  else if (bcm_vlan->filter_type == HSL_BCM_FEATURE_FIELD)
    {
      bcm_field_qset_t qset;

      BCM_FIELD_QSET_INIT (qset);

      ret = bcmx_field_group_create (qset, BCM_FIELD_GROUP_PRIO_ANY, &group);
      if (ret != BCM_E_NONE)
        HSL_FN_EXIT (ret);

      group_created = 1;

      BCM_FIELD_QSET_ADD (qset, bcmFieldQualifyOuterVlan);

      ret = bcmx_field_group_set (group, qset);
      if (ret < 0)
        goto err_ret_field;

      ret = bcmx_field_entry_create (group, &entry);
      if (ret < 0)
        goto err_ret_field;

      entry_created = 1;

      bcm_vlan->u.field.entry = entry;
      bcm_vlan->u.field.group = group;

    }

  bcm_vlan->vid = vid;
  bcm_vlan->vlan_type = vlan_type;

  HSL_FN_EXIT (0);

  err_ret_field:
    if (entry_created)
      {
        bcmx_field_entry_remove (entry);
        bcmx_field_entry_destroy (entry);
      }
    if (group_created)
      {
        bcmx_field_group_destroy (group);
      }

   HSL_FN_EXIT (-1);
}

int _hsl_bcm_vlan_unset(struct hsl_bcm_pvlan *bcm_vlan)
{
  if (bcm_vlan->filter_type == HSL_BCM_FEATURE_FILTER)
    {
      bcmx_filter_remove (bcm_vlan->u.filter);
      bcmx_filter_destroy (bcm_vlan->u.filter);
      HSL_FN_EXIT (0);
    }
  else if (bcm_vlan->filter_type == HSL_BCM_FEATURE_FIELD)
    {
      bcmx_field_entry_remove (bcm_vlan->u.field.entry);
      bcmx_field_entry_destroy (bcm_vlan->u.field.entry);
      bcmx_field_group_destroy (bcm_vlan->u.field.group);
    }
      
  HSL_FN_EXIT (-1);
}

int hsl_bcm_pvlan_set_vlan_type (struct hsl_bridge *bridge,
                                 struct hsl_vlan_port *vlan,
                                 enum hal_pvlan_type vlan_type)
{
  struct hsl_bcm_pvlan *bcm_vlan;
  int ret;
  
  HSL_FN_ENTER();
  
  bcm_vlan = (struct hsl_bcm_pvlan *)vlan->system_info;
  if (!bcm_vlan)
    {
      bcm_vlan = hsl_bcm_pvlan_alloc();
      if (!bcm_vlan)
        HSL_FN_EXIT(-1);
       
      bcm_vlan->filter_type = hsl_bcm_pvlan_filter_type_get();
      
      ret = _hsl_bcm_vlan_set(bcm_vlan, vlan->vid, vlan_type);
      if (ret < 0)
        HSL_FN_EXIT(ret);
      
      vlan->system_info = bcm_vlan;
    }
  else
    {
      /* Changing the vlan type */
      _hsl_bcm_vlan_unset(bcm_vlan);
      
      if (vlan_type != HAL_PVLAN_NONE)
        {
          ret = _hsl_bcm_vlan_set(bcm_vlan, vlan->vid, vlan_type);
          if (ret < 0)
            HSL_FN_EXIT(ret);
        }
    }
  
  HSL_FN_EXIT (0);
}

int hsl_bcm_pvlan_add_vlan_association (struct hsl_bridge *bridge,
                                        struct hsl_vlan_port *prim_vlan,
                                        struct hsl_vlan_port *second_vlan)
{
  return 0;
}

int hsl_bcm_pvlan_del_vlan_association (struct hsl_bridge *bridge,
                                        struct hsl_vlan_port *prim_vlan,
                                        struct hsl_vlan_port *second_vlan)
{
  return 0;
}

static int
_hsl_bcm_pvlan_port_filter_add (struct hsl_vlan_port *second_vlan)
{
  struct hsl_bridge_port *bridge_port_ingress, *bridge_port_egress;
  struct hsl_bcm_if *bcmifp_ingress, *bcmifp_egress;
  struct hsl_if *ifp_ingress, *ifp_egress;
  bcmx_lport_t lport_ingress, lport_egress;
  bcmx_lplist_t plist;
  struct hsl_avl_node *node_ingress, *node_egress;
  int ingress_filter_present;
 
  HSL_FN_ENTER ();
  
  if (second_vlan->vlan_type != HAL_PVLAN_ISOLATED)
    HSL_FN_EXIT (0);

  /* Block from each isolated port to all other isolated ports */ 
  /* Isolated to trunk      - Permit
   * Isolated to Isolated   - Deny
   * Trunk to Isolated      - Deny
   * Trunk to Trunk         - Permit 
   */
  for (node_ingress = hsl_avl_top (second_vlan->port_tree); node_ingress;
       node_ingress = hsl_avl_next (node_ingress))
     {
       ingress_filter_present = 0;
       ifp_ingress = HSL_AVL_NODE_INFO (node_ingress);
       if (!ifp_ingress)
         continue;
          
       bcmifp_ingress = (struct hsl_bcm_if *)ifp_ingress->system_info;
       if (!bcmifp_ingress)
         continue;
       
       lport_ingress = bcmifp_ingress->u.l2.lport;

       bridge_port_ingress = ifp_ingress->u.l2_ethernet.port;

       HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO, " lport_ingress = %d\n", lport_ingress);
       if (!bridge_port_ingress)
         continue;

       /* If ingress port is promiscuous, no need to block any port
        * But for trunk ingress ports, block traffic to isolated egress ports */
       if (bridge_port_ingress->pvlan_port_mode 
                   == HAL_PVLAN_PORT_MODE_PROMISCUOUS)
         continue;

       bcmx_lplist_init (&plist, 0, 0); 

       bcmx_port_egress_get (lport_ingress, -1, &plist);
       /* Loop through all other ports, and set egressing on the ingress port */
       for (node_egress = hsl_avl_top (second_vlan->port_tree); node_egress;
            node_egress = hsl_avl_next (node_egress))
         {
           ifp_egress = HSL_AVL_NODE_INFO (node_egress);
           if (!ifp_egress)
             continue;

           bcmifp_egress = (struct hsl_bcm_if *)ifp_egress->system_info;
           if (!bcmifp_egress)
             continue;

           lport_egress = bcmifp_egress->u.l2.lport;
           bridge_port_egress = ifp_egress->u.l2_ethernet.port;

           HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO, " lport_egress = %d\n", lport_egress);
           if (!bridge_port_egress)
             continue;

           if (lport_ingress == lport_egress)
             continue;

           /* If it is a host isolated port, for both trunk and isolated ports
            * block the traffic */
           /* if (bridge_port_egress->pvlan_port_mode == HAL_PVLAN_PORT_MODE_HOST)
             continue; */

           if (bridge_port_egress->pvlan_port_mode != HAL_PVLAN_PORT_MODE_HOST)
             continue;

           /* bcmx_lplist_add (&plist, lport_egress); */
           bcmx_lplist_port_remove (&plist, lport_egress, 1);
           ingress_filter_present = 1;
          }
         /* This is the list of ports on which traffic is allowed from lport_ingress */
         if (ingress_filter_present)
           {
             bcmx_port_egress_set (lport_ingress, -1, plist);
           }
         bcmx_lplist_free (&plist);
     }

  HSL_FN_EXIT (0);
}

static int
_hsl_bcm_pvlan_port_filter_remove (bcmx_lport_t lport, 
                                   struct hsl_vlan_port *second_vlan)
{
  struct hsl_bridge_port *bridge_port_ingress;
  struct hsl_bcm_if *bcmifp_ingress;
  struct hsl_if *ifp_ingress;
  bcmx_lport_t lport_ingress, lport_egress;
  bcmx_lplist_t plist;
  struct hsl_avl_node *node_ingress;

  HSL_FN_ENTER ();

  bcmx_lplist_init (&plist, 0, 0);
  if (second_vlan->vlan_type != HAL_PVLAN_ISOLATED)
    HSL_FN_EXIT (-1);

  /* As the configuration is removed, allow all the ports */
  if (second_vlan->vlan_type == HAL_PVLAN_ISOLATED)
    {
      BCMX_FOREACH_LPORT (lport_egress)
        {
          HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO, "lport_egress = %d \n", lport_egress);
          bcmx_lplist_add (&plist, lport_egress);
        }

      bcmx_port_egress_set (lport, -1, plist);
      bcmx_lplist_free (&plist);
    } 

  /* For rest of the ports, add this port to egress ports list */
  for (node_ingress = hsl_avl_top (second_vlan->port_tree); node_ingress;
       node_ingress = hsl_avl_next (node_ingress))
    {
       bcmx_lplist_init (&plist, 0, 0);
       ifp_ingress = HSL_AVL_NODE_INFO (node_ingress);
       if (!ifp_ingress)
         continue;

       bcmifp_ingress = (struct hsl_bcm_if *)ifp_ingress->system_info;
       if (!bcmifp_ingress)
         continue;

       lport_ingress = bcmifp_ingress->u.l2.lport;

       bridge_port_ingress = ifp_ingress->u.l2_ethernet.port;

       HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO, " lport_ingress = %d\n", lport_ingress);
       if (!bridge_port_ingress)
         continue;

       /* Allow egressing on promiscuous and trunk ports  */
       if ((bridge_port_ingress->pvlan_port_mode == HAL_PVLAN_PORT_MODE_INVALID) ||
          (bridge_port_ingress->pvlan_port_mode == HAL_PVLAN_PORT_MODE_PROMISCUOUS))
         continue;

       if (lport_ingress == lport)
         continue;

       bcmx_port_egress_get (lport_ingress, -1, &plist);
       bcmx_lplist_add (&plist, lport);

       bcmx_port_egress_set (lport_ingress, -1, plist);
       bcmx_lplist_free (&plist);
    }

  HSL_FN_EXIT (0);
}

int hsl_bcm_pvlan_add_port_association (struct hsl_bridge *bridge,
                                        struct hsl_port_vlan *port_vlan,
                                        struct hsl_vlan_port *prim_vlan,
                                        struct hsl_vlan_port *second_vlan)
{
  struct hsl_bridge_port *port;
  struct hsl_bcm_if *bcmifp;
  struct hsl_if *ifp;
  bcmx_lport_t lport;
  int ret;
   
  HSL_FN_ENTER();
  
  if ((!bridge) || (!port_vlan) || (!prim_vlan) || (!second_vlan))
    HSL_FN_EXIT(-1);

  port = port_vlan->port;
  if (! port)
    {
      HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_ERROR, "Interface not set\n");
      HSL_FN_EXIT (-1);
    }
                                                                                
  ifp = port->ifp;
  if (! ifp)
    {
      HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_ERROR, "Interface not set\n");
      HSL_FN_EXIT (-1);
    }
                                                                                
  bcmifp = ifp->system_info;
  if (! bcmifp)
    {
      HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_ERROR, "Interface not set\n");
      HSL_FN_EXIT (-1);
    }
                                                                                
  lport = bcmifp->u.l2.lport;

  /* Add this port to the secondary vlan tree. */
  hsl_avl_insert (second_vlan->port_tree, ifp);

  /* Add this port to the primary vlan tree. */
  hsl_avl_insert (prim_vlan->port_tree, ifp);

  /* Add this port to secondary vlan as untagged */
  ret = hsl_bcm_add_vlan_to_port (bridge, port_vlan, second_vlan->vid, 0);

  /* Add this port to primary vlan as untagged */
  ret = hsl_bcm_add_vlan_to_port (bridge, port_vlan, prim_vlan->vid, 0);

  /* For isolated vlan ports, set filter */
  ret = _hsl_bcm_pvlan_port_filter_add (second_vlan);

  if (ret < 0)
    HSL_FN_EXIT (-1);
  
  HSL_FN_EXIT (0);
}

int hsl_bcm_pvlan_del_port_association (struct hsl_bridge *bridge,
                                        struct hsl_port_vlan *port_vlan,
                                        struct hsl_vlan_port *prim_vlan,
                                        struct hsl_vlan_port *second_vlan)
{
  struct hsl_bridge_port *port;
  struct hsl_bcm_if *bcmifp;
  struct hsl_if *ifp;
  bcmx_lport_t lport;
  int ret;
   
  HSL_FN_ENTER();
  
  if ((!bridge) || (!port_vlan) || (!prim_vlan) || (!second_vlan))
    HSL_FN_EXIT(-1);

  port = port_vlan->port;
  if (! port)
    {
      HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_ERROR, "Interface not set\n");
      HSL_FN_EXIT (-1);
    }
                                                                                
  ifp = port->ifp;
  if (! ifp)
    {
      HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_ERROR, "Interface not set\n");
      HSL_FN_EXIT (-1);
    }
                                                                                
  bcmifp = ifp->system_info;
  if (! bcmifp)
    {
      HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_ERROR, "Interface not set\n");
      HSL_FN_EXIT (-1);
    }
                                                                                
  lport = bcmifp->u.l2.lport;

  /* For isolated vlan ports, remvoe filter */
  ret = _hsl_bcm_pvlan_port_filter_remove (lport, second_vlan);

   /* Add this port to the secondary vlan tree. */
  hsl_avl_delete (second_vlan->port_tree, ifp);

  /* Add this port to the primary vlan tree. */
  hsl_avl_delete(prim_vlan->port_tree, ifp);

  ret = hsl_bcm_delete_vlan_from_port (bridge, port_vlan, second_vlan->vid);

  ret = hsl_bcm_delete_vlan_from_port (bridge, port_vlan, prim_vlan->vid);

  if (ret < 0)
    HSL_FN_EXIT (-1);
  
  HSL_FN_EXIT (0);
}

int hsl_bcm_pvlan_set_port_mode (struct hsl_bridge *bridge,
                                 struct hsl_bridge_port *port,
                                 enum hal_pvlan_port_mode port_mode)
{
  HSL_FN_ENTER ();

  if (!port)
    HSL_FN_EXIT (-1);

  if (port->pvlan_port_mode != port_mode)
    port->pvlan_port_mode = port_mode;

  HSL_FN_EXIT (0);
}
 
#endif /* HAVE_PVLAN */
