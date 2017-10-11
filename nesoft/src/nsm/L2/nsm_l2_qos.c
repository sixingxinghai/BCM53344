/* Copyright (C) 2004  All Rights Reserved. */

#include "pal.h"
#include "lib.h"
#include "log.h"

#ifdef HAVE_VLAN
#include "avl_tree.h"
#include "l2lib.h"
#include "pal_types.h"
#include "hal_incl.h"

#include "nsmd.h"
#include "nsm_server.h"

#include "nsm_bridge.h"
#include "nsm_vlan.h"
#include "nsm_vlan_cli.h"
#include "nsm_interface.h"
#include "nsm_l2_qos.h"

int 
nsm_vlan_port_set_traffic_class_table (struct interface *ifp,
                                       unsigned char user_priority,
                                       unsigned char num_traffic_classes,
                                       unsigned char traffic_class_value)
{
#ifdef HAVE_HAL
  int ret;
#endif /* HAVE_HAL */
  struct nsm_if *zif = NULL;
  struct nsm_vlan_port *vlan_port = NULL;
  struct nsm_bridge_port *br_port = NULL;
  struct nsm_bridge *bridge = NULL;

#ifdef HAVE_LACPD
  if (NSM_INTF_TYPE_AGGREGATOR(ifp))
    return NSM_VLAN_ERR_IFP_INVALID;
#endif  

  zif = (struct nsm_if *)ifp->info;

  if (zif == NULL)
    return NSM_VLAN_ERR_BRIDGE_NOT_FOUND;

  if (zif->bridge == NULL|| zif->switchport == NULL)
    return NSM_VLAN_ERR_IFP_NOT_BOUND;

  br_port = zif->switchport;
  bridge    = zif->bridge;
  vlan_port = &br_port->vlan_port;
  if ( nsm_bridge_is_vlan_aware(bridge->master, bridge->name) != PAL_TRUE )
    return NSM_VLAN_ERR_BRIDGE_NOT_VLAN_AWARE;

#ifdef HAVE_HAL
  ret = hal_l2_qos_traffic_class_set (ifp->ifindex,
                                      user_priority,num_traffic_classes,
                                      traffic_class_value);

  if (ret < 0 )
    return ret;
#endif /* HAVE_HAL */
  vlan_port->traffic_class_table[user_priority][num_traffic_classes - 1] =
       traffic_class_value;
  SET_FLAG (vlan_port->port_config, NSM_PORT_CFG_TRF_CLASS);

  return 0;

}

int
nsm_vlan_port_set_default_user_priority (struct interface *ifp, unsigned char user_priority )
{
  int ret;
  struct nsm_if *zif = NULL;
  struct nsm_vlan_port *vlan_port = NULL;
  struct nsm_bridge_port *br_port = NULL;
  struct nsm_bridge *bridge = NULL;

#ifdef HAVE_LACPD
  if (NSM_INTF_TYPE_AGGREGATOR(ifp))
    return NSM_VLAN_ERR_IFP_INVALID;
#endif

  zif = (struct nsm_if *)ifp->info;

  if (zif == NULL)
    return NSM_VLAN_ERR_BRIDGE_NOT_FOUND;

  if (zif->bridge == NULL|| zif->switchport == NULL)
    return NSM_VLAN_ERR_IFP_NOT_BOUND;

  bridge    = zif->bridge;
  br_port = zif->switchport;
  vlan_port = &br_port->vlan_port;

  if ( nsm_bridge_is_vlan_aware(bridge->master, bridge->name) != PAL_TRUE )
    return NSM_VLAN_ERR_BRIDGE_NOT_VLAN_AWARE;

  ret = hal_l2_qos_default_user_priority_set (ifp->ifindex,
                                              user_priority);
  if (ret < 0 )
    return ret;

  SET_FLAG (vlan_port->port_config, NSM_PORT_CFG_DEF_USR_PRI);
  vlan_port->default_user_priority = user_priority;

  return 0;

}

int
nsm_vlan_port_set_regen_user_priority (struct interface *ifp, 
                                       unsigned char user_priority,
                                       unsigned char regen_user_priority)
{
#ifdef HAVE_HAL
  int ret;
#endif /* HAVE_HAL */
  struct nsm_if *zif = NULL;
  struct nsm_vlan_port *vlan_port = NULL;
  struct nsm_bridge_port *br_port = NULL;
  struct nsm_bridge *bridge = NULL;

  zif = (struct nsm_if *)ifp->info;

  if (zif == NULL)
    return NSM_VLAN_ERR_BRIDGE_NOT_FOUND;

  if (zif->bridge == NULL|| zif->switchport == NULL)
    return NSM_VLAN_ERR_IFP_NOT_BOUND;

  br_port   = zif->switchport;
  bridge    = zif->bridge;
  vlan_port = &br_port->vlan_port;

  if ( nsm_bridge_is_vlan_aware(bridge->master, bridge->name) != PAL_TRUE )
    return NSM_VLAN_ERR_BRIDGE_NOT_VLAN_AWARE;

  SET_FLAG (vlan_port->port_config, NSM_PORT_CFG_REGEN_USR_PRI);

#ifdef HAVE_HAL
  vlan_port->user_prio_regn_table [user_priority] = regen_user_priority;

  ret = hal_l2_qos_regen_user_priority_set (ifp->ifindex,
                                            user_priority,
                                            regen_user_priority);
  if (ret < 0 )
    return ret;
#endif /* HAVE_HAL */

  return 0;

}

int 
nsm_vlan_port_get_default_user_priority (struct interface *ifp)
                                         
{
  int ret = 0;
  struct nsm_bridge *bridge = NULL;
  struct nsm_vlan_port *vlan_port = NULL;
  struct nsm_bridge_port *br_port = NULL;
  struct nsm_if *zif = NULL;
  int user_priority = 0;

  zif = (struct nsm_if *)ifp->info;

  if (zif)
    if (zif->bridge && zif->switchport)
    {
      br_port   = zif->switchport;
      bridge    = zif->bridge;
      vlan_port = &br_port->vlan_port;
      user_priority = vlan_port->default_user_priority;
      if ( nsm_bridge_is_vlan_aware(bridge->master,bridge->name) != PAL_TRUE )
         ret = NSM_VLAN_ERR_BRIDGE_NOT_VLAN_AWARE;

    }
    else
      ret = NSM_VLAN_ERR_BRIDGE_NOT_FOUND;
  else
    ret = NSM_VLAN_ERR_IFP_NOT_BOUND;
 
  ret = user_priority;

  return ret;
}

int
nsm_vlan_port_get_regen_user_priority (struct interface *ifp,
                                         unsigned char user_priority )
{
  struct nsm_bridge *bridge = NULL;
  struct nsm_vlan_port *vlan_port = NULL;
  struct nsm_bridge_port *br_port = NULL;
  struct nsm_if *zif = NULL;
  int regen_user_priority = 0;
  zif = (struct nsm_if *)ifp->info;
  if (zif)
    if (zif->bridge && zif->switchport)
    {
      br_port   = zif->switchport;
      bridge    = zif->bridge;
      vlan_port = &br_port->vlan_port;
#ifdef HAVE_HAL
      regen_user_priority = vlan_port->user_prio_regn_table[user_priority];
#endif /* HAVE_HAL */
    }
  return regen_user_priority;
}

int
nsm_vlan_port_set_traffic_class_status (int ifindex,
                                       unsigned char traffic_class_enabled)
{
 int ret;
#ifdef HAVE_HAL
  ret = hal_l2_traffic_class_status_set (ifindex,traffic_class_enabled);
#endif
  return 0;

}
int
nsm_vlan_port_get_traffic_class_table (int ifindex,unsigned char user_priority,
                                       unsigned char num_traffic_classes,
                                       unsigned char traffic_class_value)
{
int ret =0;
#ifdef HAVE_HAL
  ret = hal_l2_qos_traffic_class_get (ifindex,user_priority, num_traffic_classes,
                                      traffic_class_value);
#endif
  return 0;

}

s_int32_t nsm_bridge_set_traffic_class (struct nsm_bridge *br,
                                        u_char traffic_class_enabled)
{
  struct avl_node *avl_port;
  struct nsm_bridge_port *br_port = NULL;
  s_int32_t ret = 0;

  for (avl_port = avl_top (br->port_tree); avl_port;
       avl_port = avl_next (avl_port))
    {
      br_port = (struct nsm_bridge_port *) AVL_NODE_INFO (avl_port);

      if (traffic_class_enabled == DOT1DTRAFFICCLASS_ENABLED)
        {
          ret = nsm_vlan_port_set_traffic_class_status
                    (br_port->ifindex, br->traffic_class_enabled);
        }
      else
        {
          ret = nsm_vlan_port_set_traffic_class_status
                    (br_port->ifindex,br->traffic_class_enabled);
        }
     }
   return ret;
}

#ifdef HAVE_SMI
int
nsm_set_preserve_ce_cos (struct interface *ifp)
{
 int ret = 0;

  if (ifp == NULL)
    return SMI_ERROR;

#ifdef HAVE_HAL
  ret = hal_if_set_preserve_ce_cos (ifp->ifindex);
#endif

  if (ret < 0)
    return SMI_ERROR;

  return ret;
}
#endif /* HAVE_SMI */

#if defined HAVE_QOS && defined HAVE_HAL
void
nsm_qos_get_num_cosq(s_int32_t *num_cosq)
{
  int ret =0;
  s_int32_t num = 0 ;
  ret = hal_l2_qos_get_num_coq (&num);

  if (ret < 0)
    *num_cosq = 0;
  else
    *num_cosq = num;

  return;

}

int
nsm_qos_set_num_cosq(s_int32_t num_cosq)
{
  int ret =0;
  s_int32_t num = 0;

  ret = hal_l2_qos_set_num_coq (num);

  return ret;

}

#endif /* HAVE_QOS && HAVE_HAL */
#endif /* HAVE_VLAN */
