/* Copyright (C) 2004  All Rights Reserved.  */
/* Private VLAN specifc apis */

#include "pal.h"
#include "lib.h"
#include "hal_incl.h"

#ifdef HAVE_PVLAN

#include "if.h"
#include "cli.h"
#include "table.h"
#include "avl_tree.h"
#include "linklist.h"
#include "snprintf.h"
#include "nsm_message.h"

#ifdef HAVE_L3
#include "rib.h"
#endif /* HAVE_L3 */

#include "nsmd.h"
#include "nsm_api.h"
#include "nsm_server.h"
#include "nsm_bridge.h"
#include "ptree.h"
#include "nsm_vlan.h"
#include "nsm_vlan_cli.h"

#include "nsm_interface.h"
#include "nsm_debug.h"
#include "nsm_vlan.h"
#include "nsm_pvlan.h"
#ifdef HAVE_HAL
#include "hal_pvlan.h"
#endif /* HAVE_HAL */

/* Forward declarations */

int
nsm_pvlan_clear_host_port (struct interface *ifp);

int
nsm_pvlan_clear_promiscuous_port (struct interface *ifp);

int
nsm_pvlan_clear_common_port (struct interface *ifp, 
                             enum nsm_pvlan_port_mode mode);

static int
nsm_pvlan_set_configure (struct nsm_bridge *bridge,
                         u_int16_t vid, u_int16_t type);

static int
nsm_pvlan_set_association (struct nsm_bridge *bridge,
                           u_int16_t primary_vid, u_int16_t secondary_vid,
                           u_int16_t flags);
int
nsm_pvlan_configure (struct nsm_bridge_master *master, char *brname, 
                     u_int16_t vid,  enum nsm_pvlan_type type)
{
  int ret = 0;
  struct nsm_bridge *bridge = NULL;
  struct nsm_vlan *vlan = NULL;
  struct nsm_vlan nvlan;
  struct avl_node *rn = NULL;
  enum hal_pvlan_type hal_type = 0;

  bridge = nsm_lookup_bridge_by_name(master, brname);
  if (! bridge)
    return NSM_VLAN_ERR_BRIDGE_NOT_FOUND;

  if ( nsm_bridge_is_vlan_aware(master, brname) != PAL_TRUE  )
    return NSM_VLAN_ERR_BRIDGE_NOT_VLAN_AWARE;
  
   if (! bridge->vlan_table)
     return NSM_NO_VLAN_CONFIGURED;

#ifdef HAVE_PROVIDER_BRIDGE
   if (NSM_BRIDGE_TYPE_PROVIDER (bridge))
     return NSM_PVLAN_ERR_PROVIDER_BRIDGE;
#endif

  NSM_VLAN_SET (&nvlan, vid);
  rn = avl_search (bridge->vlan_table, (void *)&nvlan);
  if (rn)
    {
      vlan = (struct nsm_vlan *)rn->info;

      if (!vlan)
        return NSM_VLAN_ERR_VLAN_NOT_FOUND;
    
      vlan->pvlan_type =  type;
      vlan->pvlan_configured = NSM_PVLAN_CONFIGURED;

      if (type == NSM_PVLAN_PRIMARY)
         NSM_VLAN_BMP_INIT (vlan->pvlan_info.secondary_vlan_bmp);

      if (type == NSM_PVLAN_ISOLATED)
        hal_type = HAL_PVLAN_ISOLATED;
      else if (type == NSM_PVLAN_COMMUNITY)
        hal_type = HAL_PVLAN_COMMUNITY;
      else if (type == NSM_PVLAN_PRIMARY)
        hal_type = HAL_PVLAN_PRIMARY;
      else
        hal_type = HAL_PVLAN_NONE;

      /* Pl change the type to HAL type before passing it to hal */
      ret = hal_set_pvlan_type (bridge->name, vid, hal_type);
      
      nsm_pvlan_set_configure (bridge, vid, type);
    }
  else
    {
      return NSM_VLAN_ERR_VLAN_NOT_FOUND;
    }  

   if (ret < 0)
     return ret;

   return RESULT_OK;

}

int
nsm_pvlan_configure_clear (struct nsm_bridge_master *master, char *brname,
                           u_int16_t vid, u_char type)
{
  int ret = 0;
  struct nsm_bridge *bridge = NULL;
  struct nsm_vlan *vlan = NULL;
  struct nsm_vlan nvlan;
  struct avl_node *rn = NULL;

  bridge = nsm_lookup_bridge_by_name(master, brname);
  if (! bridge)
    return NSM_VLAN_ERR_BRIDGE_NOT_FOUND;

  if ( nsm_bridge_is_vlan_aware(master, brname) != PAL_TRUE  )
    return NSM_VLAN_ERR_BRIDGE_NOT_VLAN_AWARE;

  if (! bridge->vlan_table)
     return NSM_NO_VLAN_CONFIGURED;

#ifdef HAVE_PROVIDER_BRIDGE
   if (NSM_BRIDGE_TYPE_PROVIDER (bridge))
     return NSM_PVLAN_ERR_PROVIDER_BRIDGE;
#endif

  NSM_VLAN_SET (&nvlan, vid);
  rn = avl_search (bridge->vlan_table, (void *)&nvlan);
  if (rn)
    {
      vlan = (struct nsm_vlan*)rn->info;
      if (!vlan)
        return NSM_VLAN_ERR_VLAN_NOT_FOUND;

      if (vlan->pvlan_configured == NSM_PVLAN_NOT_CONFIGURED)
        return NSM_PVLAN_ERR_NOT_CONFIGURED;

      /* remove the ports associated to this private vlan and remove associations too */
      nsm_pvlan_delete_all (bridge, vid);

      vlan->pvlan_type = NSM_PVLAN_NONE ;
      vlan->pvlan_configured = NSM_PVLAN_NOT_CONFIGURED;

      ret = hal_set_pvlan_type (bridge->name, vid, HAL_PVLAN_NONE);
      nsm_pvlan_set_configure (bridge, vid, NSM_PVLAN_NONE);
    }
  else
    {
      return NSM_VLAN_ERR_VLAN_NOT_FOUND;
    }
   if (ret < 0)
     return ret;

 return RESULT_OK;
}

int
nsm_pvlan_associate (struct nsm_bridge_master *master, char *brname, 
                     u_int16_t vid, u_int16_t pvid)
{
  int ret = 0;
  struct nsm_bridge *bridge = NULL;
  struct nsm_vlan *vlan = NULL, *secondary_vlan = NULL;
  struct nsm_vlan nvlanv, nvlanp;
  struct avl_node *rnv = NULL, *rnp = NULL;

  bridge = nsm_lookup_bridge_by_name(master, brname);
  if (! bridge)
    return NSM_VLAN_ERR_BRIDGE_NOT_FOUND;

  if ( nsm_bridge_is_vlan_aware(master, brname) != PAL_TRUE  )
    return NSM_VLAN_ERR_BRIDGE_NOT_VLAN_AWARE;

  if (! bridge->vlan_table)
        return NSM_NO_VLAN_CONFIGURED;

#ifdef HAVE_PROVIDER_BRIDGE
   if (NSM_BRIDGE_TYPE_PROVIDER (bridge))
     return NSM_PVLAN_ERR_PROVIDER_BRIDGE;
#endif

  if (vid == pvid)
    return NSM_PVLAN_ERR_PRIMARY_SECOND_SAME; 

  NSM_VLAN_SET (&nvlanv, vid);
  NSM_VLAN_SET (&nvlanp, pvid);
  rnv = avl_search (bridge->vlan_table, (void *)&nvlanv);
  rnp = avl_search (bridge->vlan_table, (void *)&nvlanp);

  if (rnv && rnp)
    {
      
      vlan = (struct nsm_vlan *)rnv->info;

      secondary_vlan = (struct nsm_vlan *)rnp->info;

      if (!vlan || !secondary_vlan)
        return NSM_VLAN_ERR_VLAN_NOT_FOUND;

      if ((!(vlan->pvlan_configured)) || (!(secondary_vlan->pvlan_configured)))
        return NSM_PVLAN_ERR_NOT_CONFIGURED;

      if (vlan->pvlan_type != NSM_PVLAN_PRIMARY)
        return NSM_PVLAN_ERR_NOT_PRIMARY_VLAN;
      if ((secondary_vlan->pvlan_type != NSM_PVLAN_ISOLATED)
           && (secondary_vlan->pvlan_type != NSM_PVLAN_COMMUNITY))
        return NSM_PVLAN_ERR_NOT_SECONDARY_VLAN;

      if ((vlan->pvlan_info.vid.isolated_vid !=0)
           && (secondary_vlan->pvlan_type == NSM_PVLAN_ISOLATED))
        return NSM_PVLAN_ERR_ISOLATED_VLAN_EXISTS; 

      if (secondary_vlan->pvlan_info.vid.primary_vid !=0)
        return NSM_PVLAN_ERR_ASSOCIATED_TO_PRIMARY;
      /* update the secondary_vlan info in primary_vlan */
      NSM_VLAN_BMP_SET(vlan->pvlan_info.secondary_vlan_bmp, pvid);

      /* Set the primary vid of seconary_vlan */
      secondary_vlan->pvlan_info.vid.primary_vid = vid;

      /* Update the isolated vid for primary vlan */
      if (secondary_vlan->pvlan_type == NSM_PVLAN_ISOLATED)
        vlan->pvlan_info.vid.isolated_vid = pvid;           

      ret = hal_vlan_associate (bridge->name, vid, pvid, PAL_TRUE);

      nsm_pvlan_set_association (bridge, vid, pvid, PAL_TRUE);
    }
  else
    {
      return NSM_VLAN_ERR_VLAN_NOT_FOUND;
    }

  if (ret < 0)
     return ret;

  return RESULT_OK;

}
      
int
nsm_pvlan_associate_clear (struct nsm_bridge_master *master, char *brname,
                           u_int16_t vid, u_int16_t pvid)
{
  int ret = 0;
  struct nsm_bridge *bridge = NULL;
  struct nsm_vlan *vlan = NULL, *secondary_vlan = NULL;
  struct nsm_vlan nvlanv, nvlanp;
  struct avl_node *rnv = NULL, *rnp = NULL;

  bridge = nsm_lookup_bridge_by_name(master, brname);
  if (! bridge)
    return NSM_VLAN_ERR_BRIDGE_NOT_FOUND;

  if ( nsm_bridge_is_vlan_aware(master, brname) != PAL_TRUE  )
    return NSM_VLAN_ERR_BRIDGE_NOT_VLAN_AWARE;

   if (! bridge->vlan_table)
     return NSM_NO_VLAN_CONFIGURED;

#ifdef HAVE_PROVIDER_BRIDGE
   if (NSM_BRIDGE_TYPE_PROVIDER (bridge))
     return NSM_PVLAN_ERR_PROVIDER_BRIDGE;
#endif

  NSM_VLAN_SET (&nvlanv, vid);
  NSM_VLAN_SET (&nvlanp, pvid);
  rnv = avl_search (bridge->vlan_table, (void *)&nvlanv);
  rnp = avl_search (bridge->vlan_table, (void *)&nvlanp);

  if (rnv && rnp)
    {
      vlan = (struct nsm_vlan *)rnv->info;

      secondary_vlan = (struct nsm_vlan *)rnp->info;
      if (!vlan || !secondary_vlan)
        return NSM_VLAN_ERR_VLAN_NOT_FOUND; 

      if ((!(vlan->pvlan_configured)) || (!(secondary_vlan->pvlan_configured)))
        return NSM_PVLAN_ERR_NOT_CONFIGURED;

      if (vlan->pvlan_type != NSM_PVLAN_PRIMARY)
        return NSM_PVLAN_ERR_NOT_PRIMARY_VLAN;
      if ((secondary_vlan->pvlan_type != NSM_PVLAN_ISOLATED)
           && (secondary_vlan->pvlan_type != NSM_PVLAN_COMMUNITY))
        return NSM_PVLAN_ERR_NOT_SECONDARY_VLAN;

      nsm_pvlan_remove_port_association (bridge, vid, pvid);

      ret = hal_vlan_associate (bridge->name, vid, pvid, PAL_FALSE);

      /* update the secondary_vlan info in primary_vlan */
      NSM_VLAN_BMP_UNSET(vlan->pvlan_info.secondary_vlan_bmp, pvid);

      /* Set the primary vid of seconary_vlan */
      secondary_vlan->pvlan_info.vid.primary_vid = 0;

      /* Update the isolated vid for primary vlan */
      if (secondary_vlan->pvlan_type == NSM_PVLAN_ISOLATED)
        vlan->pvlan_info.vid.isolated_vid = 0;

      nsm_pvlan_set_association (bridge, vid, pvid, PAL_FALSE);
    }
  else
    {
      return NSM_VLAN_ERR_VLAN_NOT_FOUND;
    }
  if (ret < 0)
      return ret;

  return RESULT_OK;

}

int
nsm_pvlan_remove_port_association (struct nsm_bridge *bridge, 
                                   u_int16_t vid, u_int16_t pvid)
{
  int ret = 0;
  struct nsm_vlan v;
  struct nsm_if *zif;
  struct avl_node *avl_port;
  struct interface *ifp = NULL;
  struct nsm_vlan *vlan = NULL;
  struct avl_node *rnv = NULL;
  struct nsm_bridge_port *br_port;
  struct nsm_vlan_port *vlan_port = NULL;

  NSM_VLAN_SET (&v, vid);

  rnv = avl_search (bridge->vlan_table, (void *)&v);

  if (!rnv)
     return NSM_VLAN_ERR_VLAN_NOT_FOUND;
  vlan = (struct nsm_vlan *)rnv->info;

  if (!vlan)
    return NSM_VLAN_ERR_VLAN_NOT_FOUND;

  if (!(vlan->pvlan_configured))
    return NSM_PVLAN_ERR_NOT_CONFIGURED;

  for (avl_port = avl_top (bridge->port_tree); avl_port;
       avl_port = avl_next (avl_port))
    {

      if ((br_port = (struct nsm_bridge_port *)
                             AVL_NODE_INFO (avl_port)) == NULL)
        continue;

      if ((zif = br_port->nsmif) == NULL)
        continue;

      if ((ifp = zif->ifp) == NULL)
        continue;
      
      vlan_port = &br_port->vlan_port;

      if ( !vlan_port )
        continue;

      if (!(vlan_port->pvlan_configured))
        continue;

      /* Check for Port is the member of this vlan
       * is below
       */

      if (vlan_port->pvlan_mode == NSM_PVLAN_PORT_MODE_HOST)
        {
          if ((vlan_port->pvid != pvid)
              && (vlan_port->pvlan_port_info.primary_vid != vid))
            continue;

          if (ifp)
            ret = nsm_pvlan_api_host_association_clear (ifp, vid, pvid);
        }
      if (vlan_port->pvlan_mode == NSM_PVLAN_PORT_MODE_PROMISCUOUS)
        {
          if (vid == vlan_port->pvid)
            {
              if (NSM_VLAN_BMP_IS_MEMBER
                       (vlan->pvlan_info.secondary_vlan_bmp, pvid));
                {
                  if (ifp)
                  nsm_pvlan_api_switchport_mapping_clear
                          ( ifp, vid, pvid);
                }
            }

        }
    }

  return 0;
}

int
nsm_pvlan_associate_clear_all (struct nsm_bridge_master *master,
                               char *brname, u_int16_t vid)
{
  int ret = 0;
  struct nsm_bridge *bridge = NULL;
  struct nsm_vlan *vlan = NULL;
  struct nsm_vlan v;
  struct avl_node *rnv = NULL;
  u_int16_t secondary_vid = 0;

  bridge = nsm_lookup_bridge_by_name(master, brname);
  if (! bridge)
    return NSM_VLAN_ERR_BRIDGE_NOT_FOUND;

  if ( nsm_bridge_is_vlan_aware(master, brname) != PAL_TRUE  )
    return NSM_VLAN_ERR_BRIDGE_NOT_VLAN_AWARE;

  if (! bridge->vlan_table)
    return NSM_NO_VLAN_CONFIGURED;

#ifdef HAVE_PROVIDER_BRIDGE
   if (NSM_BRIDGE_TYPE_PROVIDER (bridge))
     return NSM_PVLAN_ERR_PROVIDER_BRIDGE;
#endif

  NSM_VLAN_SET (&v, vid);
  rnv = avl_search (bridge->vlan_table, (void *)&v);

  if (!rnv || !rnv->info)
    return NSM_VLAN_ERR_VLAN_NOT_FOUND;

  vlan = (struct nsm_vlan *) rnv->info;

  if (!vlan->pvlan_configured)
    return NSM_PVLAN_NOT_CONFIGURED;

  if (vlan->pvlan_type != NSM_PVLAN_PRIMARY)
    return NSM_PVLAN_ERR_NOT_PRIMARY_VLAN;

  for (secondary_vid = 1; secondary_vid < NSM_VLAN_MAX; secondary_vid++)
    {
      if (NSM_VLAN_BMP_IS_MEMBER (vlan->pvlan_info.secondary_vlan_bmp,
                                  secondary_vid))
        {
          ret = nsm_pvlan_associate_clear (master, brname, vid, secondary_vid);
          if (ret < 0)
            return ret;
        }
    }

  return RESULT_OK;

}
        
int
nsm_pvlan_api_clear_port_mode (struct interface *ifp, 
                               enum nsm_pvlan_port_mode mode)
{
  struct nsm_if *zif = NULL;
  struct nsm_vlan_port *vlan_port = NULL;
  struct nsm_bridge_port *br_port = NULL;
  struct nsm_bridge *bridge = NULL;
  int ret = 0;

  zif = (struct nsm_if *)ifp->info;

  if (! zif || ! zif->switchport)
    return NSM_VLAN_ERR_IFP_NOT_BOUND;

  br_port = zif->switchport;

  vlan_port = &br_port->vlan_port;
  
  bridge = zif->bridge;
  if (! bridge)
    return NSM_VLAN_ERR_BRIDGE_NOT_FOUND;

#ifdef HAVE_PROVIDER_BRIDGE
   if (NSM_BRIDGE_TYPE_PROVIDER (bridge))
     return NSM_PVLAN_ERR_PROVIDER_BRIDGE;
#endif

  if (vlan_port->mode != NSM_VLAN_PORT_MODE_ACCESS)
    return NSM_VLAN_ERR_INVALID_MODE;

  if (vlan_port->pvlan_mode != mode)
    return NSM_PVLAN_ERR_INVALID_MODE;
    
  ret = hal_pvlan_set_port_mode (bridge->name, ifp->ifindex, 
                                       HAL_PVLAN_PORT_MODE_INVALID);
  if (ret < 0)
    return ret;

  vlan_port->pvlan_mode = NSM_PVLAN_PORT_MODE_INVALID;
  vlan_port->pvlan_configured = NSM_PVLAN_PORT_NOT_CONFIGURED;

  nsm_vlan_set_ingress_filter (ifp, NSM_VLAN_PORT_MODE_ACCESS,
                               NSM_VLAN_PORT_MODE_ACCESS, PAL_FALSE);

  /* send message to PM for enabling port fast and bpdu guard */
  if (mode == NSM_PVLAN_PORT_MODE_HOST)
     ret = nsm_pvlan_set_port_fast_bpdu_guard (bridge, ifp, PAL_FALSE);

  nsm_vlan_set_access_port (ifp, NSM_VLAN_DEFAULT_VID, PAL_TRUE, PAL_TRUE);
  if (ret < 0)
    return ret;

  return RESULT_OK;
}

int
nsm_pvlan_api_set_port_mode (struct interface *ifp, 
                             enum nsm_pvlan_port_mode mode)
{
  struct nsm_if *zif = NULL;
  struct nsm_vlan_port *vlan_port = NULL;
  struct nsm_bridge_port *br_port = NULL;
  int ret = 0;
  struct nsm_bridge *bridge = NULL;

  zif = (struct nsm_if *)ifp->info;

  if (! zif || ! zif->switchport)
    return NSM_VLAN_ERR_IFP_NOT_BOUND;

  br_port = zif->switchport;
  vlan_port = &br_port->vlan_port;

  bridge = zif->bridge;
  if (! bridge)
    return NSM_VLAN_ERR_BRIDGE_NOT_FOUND;

  if ( nsm_bridge_is_vlan_aware(bridge->master, bridge->name) != PAL_TRUE  )
    return NSM_VLAN_ERR_BRIDGE_NOT_VLAN_AWARE;

#ifdef HAVE_PROVIDER_BRIDGE
   if (NSM_BRIDGE_TYPE_PROVIDER (bridge))
     return NSM_PVLAN_ERR_PROVIDER_BRIDGE;
#endif

  if (vlan_port->mode != NSM_VLAN_PORT_MODE_ACCESS)
    return NSM_VLAN_ERR_INVALID_MODE;

  if (vlan_port->pvlan_mode == mode)
    {
      /* Mode already set, nothing to do */
      return 0;
    }

  if (mode == NSM_PVLAN_PORT_MODE_HOST
            && (vlan_port->mode != NSM_VLAN_PORT_MODE_ACCESS))
    return RESULT_ERROR;

  /* Clear previous mode. */
  if (vlan_port->pvlan_mode != NSM_PVLAN_PORT_MODE_INVALID)
    {
      switch(vlan_port->pvlan_mode)
        {
          case NSM_PVLAN_PORT_MODE_HOST:
              ret = nsm_pvlan_clear_host_port (ifp);
              break;
          case NSM_PVLAN_PORT_MODE_PROMISCUOUS:
              ret = nsm_pvlan_clear_promiscuous_port (ifp);
              break;
          default:
            return NSM_PVLAN_ERR_INVALID_MODE;
        }

       if (ret < 0)
        {
          return -1;  /* Error code */
        }
    }


  vlan_port->pvlan_configured = NSM_PVLAN_PORT_CONFIGURED;  

  /* Set port mode. */
  vlan_port->pvlan_mode = mode;

  /* Set new mode. */
  switch (mode)
    {
      case NSM_PVLAN_PORT_MODE_HOST:
        ret = hal_pvlan_set_port_mode (bridge->name, ifp->ifindex, HAL_PVLAN_PORT_MODE_HOST);
        break;
      case  NSM_PVLAN_PORT_MODE_PROMISCUOUS:
        ret = hal_pvlan_set_port_mode (bridge->name, ifp->ifindex, HAL_PVLAN_PORT_MODE_PROMISCUOUS);
        break;
      default:
        break;
    }

  nsm_vlan_set_ingress_filter (ifp, NSM_VLAN_PORT_MODE_ACCESS,
                               NSM_VLAN_PORT_MODE_ACCESS, PAL_TRUE);

  if (ret < 0)
    return ret;

  /* send message to PM for enabling port fast and bpdu guard */
  if (mode == NSM_PVLAN_PORT_MODE_HOST)
     ret = nsm_pvlan_set_port_fast_bpdu_guard (bridge, ifp, PAL_TRUE);

  if (ret < 0)
    return ret;

  return RESULT_OK;

}

int
nsm_pvlan_clear_host_port (struct interface *ifp)
{
  return nsm_pvlan_clear_common_port (ifp, NSM_PVLAN_PORT_MODE_HOST);
}

int 
nsm_pvlan_clear_promiscuous_port (struct interface *ifp)
{
   return nsm_pvlan_clear_common_port (ifp, NSM_PVLAN_PORT_MODE_PROMISCUOUS);
}

int
nsm_pvlan_clear_common_port (struct interface *ifp, 
                             enum nsm_pvlan_port_mode mode)
{
  struct nsm_if *zif = NULL;
  struct nsm_vlan_port *vlan_port = NULL;
  struct nsm_bridge_port *br_port = NULL;
  struct nsm_bridge *bridge = NULL;
  int ret = 0;
  
  zif = (struct nsm_if *)ifp->info;

  if (! zif || ! zif->switchport)
    return NSM_VLAN_ERR_IFP_NOT_BOUND;

  br_port = zif->switchport;
  vlan_port = &br_port->vlan_port;

  bridge = zif->bridge;
  if (! bridge)
    return NSM_VLAN_ERR_BRIDGE_NOT_FOUND;

  if (vlan_port->pvlan_mode != mode)
    return NSM_VLAN_ERR_INVALID_MODE;

  ret =  hal_pvlan_set_port_mode (bridge->name, ifp->ifindex,
                                    HAL_PVLAN_PORT_MODE_INVALID);

  vlan_port->pvlan_mode = NSM_PVLAN_PORT_MODE_INVALID;
  vlan_port->pvlan_configured = NSM_PVLAN_PORT_NOT_CONFIGURED;

  /* send message to PM for enabling port fast and bpdu guard */
  if (mode == NSM_PVLAN_PORT_MODE_HOST)
     ret = nsm_pvlan_set_port_fast_bpdu_guard (bridge, ifp, PAL_FALSE);

  if (ret < 0)
    return ret;

  return RESULT_OK;
}

int
nsm_pvlan_set_port_fast_bpdu_guard (struct nsm_bridge *bridge, 
                                    struct interface *ifp, 
                                    bool_t enable)
{
  struct nsm_msg_pvlan_if msg;
  int nbytes = 0, i;
  struct nsm_server_client *nsc;
  struct nsm_server_entry *nse;

  if (!bridge || !ifp)
    return -1;
  
  /* Send message to PM. */
  NSM_SERVER_CLIENT_LOOP (ng, i, nsc, nse)
    if (nsm_service_check (nse, NSM_SERVICE_INTERFACE))
      {
         pal_mem_set (&msg, 0, sizeof (struct nsm_msg_pvlan_if));
         pal_strncpy (msg.bridge_name, bridge->name, NSM_BRIDGE_NAMSIZ);
         msg.ifindex = ifp->ifindex;

         if (enable)
           msg.flags = NSM_PVLAN_ENABLE_PORTFAST_BPDUGUARD;
         else
           msg.flags = NSM_PVLAN_DISABLE_PORTFAST_BPDUGUARD;

         /* Set nse pointer and size. */
         nse->send.pnt = nse->send.buf + NSM_MSG_HEADER_SIZE;
         nse->send.size = nse->send.len - NSM_MSG_HEADER_SIZE;

         /* Encode NSM bridge interface message. */
         nbytes = nsm_encode_pvlan_if_msg (&nse->send.pnt,
                                              &nse->send.size, &msg);
         if (nbytes < 0)
           return nbytes;

         /* Send bridge interface message. */
         nsm_server_send_message (nse, 0, 0,
                                  NSM_MSG_PVLAN_PORT_HOST_ASSOCIATE, 
                                  0, nbytes);
      }
  return nbytes;
}
 
static int
nsm_pvlan_set_configure (struct nsm_bridge *bridge,
                         u_int16_t vid, u_int16_t type)
{
  struct nsm_msg_pvlan_configure msg;
  int nbytes = 0, i;
  struct nsm_server_client *nsc;
  struct nsm_server_entry *nse;

  if (!bridge)
    return -1;

  /* Send message to PM. */
  NSM_SERVER_CLIENT_LOOP (ng, i, nsc, nse)
    if (nsm_service_check (nse, NSM_SERVICE_INTERFACE))
      {
        pal_mem_set (&msg, 0, sizeof (struct nsm_msg_pvlan_configure));
        pal_strncpy (msg.bridge_name, bridge->name, NSM_BRIDGE_NAMSIZ);
        msg.vid = vid;

        if (type == NSM_PVLAN_PRIMARY)
          msg.type = NSM_PVLAN_CONFIGURE_PRIMARY;
        else if (type == NSM_PVLAN_NONE)
          msg.type = NSM_PVLAN_CONFIGURE_NONE;
        else
          msg.type = NSM_PVLAN_CONFIGURE_SECONDARY;

        /* Set nse pointer and size. */
        nse->send.pnt = nse->send.buf + NSM_MSG_HEADER_SIZE;
        nse->send.size = nse->send.len - NSM_MSG_HEADER_SIZE;

        /* Encode NSM bridge interface message. */
        nbytes = nsm_encode_pvlan_configure (&nse->send.pnt,
                                             &nse->send.size, &msg);
        if (nbytes < 0)
          return nbytes;

        /* Send bridge interface message. */
        nsm_server_send_message (nse, 0, 0,
                                 NSM_MSG_PVLAN_CONFIGURE,
                                 0, nbytes);
      }
  return nbytes;
}

static int
nsm_pvlan_set_association (struct nsm_bridge *bridge,
                           u_int16_t primary_vid, u_int16_t secondary_vid,
                           u_int16_t flags)
{
  struct nsm_msg_pvlan_association msg;
  int nbytes = 0, i;
  struct nsm_server_client *nsc;
  struct nsm_server_entry *nse;

  if (!bridge)
    return -1;

  /* Send message to PM. */
  NSM_SERVER_CLIENT_LOOP (ng, i, nsc, nse)
    if (nsm_service_check (nse, NSM_SERVICE_INTERFACE))
      {
         pal_mem_set (&msg, 0, sizeof (struct nsm_msg_pvlan_configure));
         pal_strncpy (msg.bridge_name, bridge->name, NSM_BRIDGE_NAMSIZ);
         msg.primary_vid = primary_vid;
         msg.secondary_vid = secondary_vid;

         msg.flags = flags;

         /* Set nse pointer and size. */
         nse->send.pnt = nse->send.buf + NSM_MSG_HEADER_SIZE;
         nse->send.size = nse->send.len - NSM_MSG_HEADER_SIZE;

         /* Encode NSM bridge interface message. */
         nbytes = nsm_encode_pvlan_associate (&nse->send.pnt,
                                              &nse->send.size, &msg);
         if (nbytes < 0)
           return nbytes;

         /* Send bridge interface message. */
         nsm_server_send_message (nse, 0, 0,
                                  NSM_MSG_PVLAN_SECONDARY_VLAN_ASSOCIATE,
                                  0, nbytes);

      }
  return nbytes;
}

int 
nsm_pvlan_api_host_association (struct interface *ifp, u_int16_t vid,
                                u_int16_t pvid)
{
  int ret = 0;
  struct nsm_if *zif = NULL;
  struct nsm_vlan_port *vlan_port = NULL;
  struct nsm_bridge_port *br_port = NULL;
  struct nsm_bridge *bridge = NULL;
  struct nsm_vlan *vlan = NULL, *secondary_vlan = NULL;
  struct avl_node *rnp = NULL, *rnv = NULL;
  struct nsm_vlan p,v;

  zif = (struct nsm_if *)ifp->info;

  if (! zif || ! zif->switchport)
    return NSM_VLAN_ERR_IFP_NOT_BOUND;

  br_port = zif->switchport;
  vlan_port = &br_port->vlan_port;

  bridge = zif->bridge;
  if (! bridge)
    return NSM_VLAN_ERR_IFP_NOT_BOUND;

#ifdef HAVE_PROVIDER_BRIDGE
   if (NSM_BRIDGE_TYPE_PROVIDER (bridge))
     return NSM_PVLAN_ERR_PROVIDER_BRIDGE;
#endif

  if ( nsm_bridge_is_vlan_aware(bridge->master, bridge->name) != PAL_TRUE  )
    return NSM_VLAN_ERR_BRIDGE_NOT_VLAN_AWARE;

  if (vlan_port->mode != NSM_VLAN_PORT_MODE_ACCESS)
    return NSM_VLAN_ERR_INVALID_MODE;

  if (vlan_port->pvlan_mode != NSM_PVLAN_PORT_MODE_HOST)
    return NSM_PVLAN_ERR_NOT_HOST_PORT;

  /* check whether port is access port or not. PVLAN host association is for
   * access ports only */

  /* Check whether the secondary vlan is part of primary vlan
   * else error out. to be implemented
   */
   
  if (! bridge->vlan_table)
     return NSM_NO_VLAN_CONFIGURED;

  NSM_VLAN_SET (&v, vid);
  NSM_VLAN_SET (&p, pvid);


  rnv = avl_search (bridge->vlan_table, (void *)&v);
  rnp = avl_search (bridge->vlan_table, (void *)&p);


  if (rnv && rnp)
    {
      vlan = (struct nsm_vlan *)rnv->info;

      secondary_vlan = (struct nsm_vlan *)rnp->info;

      if (!vlan || !secondary_vlan)
        return NSM_VLAN_ERR_VLAN_NOT_FOUND;

     if ((!(vlan->pvlan_configured)) || (!(secondary_vlan->pvlan_configured)))
        return NSM_PVLAN_ERR_NOT_CONFIGURED;

     if (vlan->pvlan_type != NSM_PVLAN_PRIMARY)
       return NSM_PVLAN_ERR_NOT_PRIMARY_VLAN;

     if ((secondary_vlan->pvlan_type == NSM_PVLAN_ISOLATED)
          && (secondary_vlan->pvlan_type == NSM_PVLAN_COMMUNITY))
       return NSM_PVLAN_ERR_INVALID_MODE;

     if (!(NSM_VLAN_BMP_IS_MEMBER (vlan->pvlan_info.secondary_vlan_bmp, pvid)))
       return NSM_PVLAN_ERR_SECOND_NOT_ASSOCIATED;

     /* If previously associated to a secondary vlan, remove the configuration
      * Required for hardware clearance */
     if ((vlan_port->pvid != NSM_VLAN_DEFAULT_VID)
        && (vlan_port->pvlan_port_info.primary_vid != 0))
       nsm_pvlan_api_host_association_clear (ifp, vlan_port->pvlan_port_info.primary_vid,
                                                  vlan_port->pvid);     
 
     /* The default vlan will be set to secondary vlan id for 
      * isolated and community hosts
      */
     vlan_port->pvlan_port_info.primary_vid = vid;
     nsm_vlan_set_access_port (ifp, pvid, PAL_TRUE, PAL_TRUE);

     ret = hal_pvlan_host_association (bridge->name, ifp->ifindex, 
                                       vid, pvid, PAL_TRUE);
    }
  else
    {
       return NSM_VLAN_ERR_VLAN_NOT_FOUND;
    }

  if (ret < 0)
    return ret;

  return RESULT_OK;

}
int
nsm_pvlan_api_host_association_clear (struct interface *ifp, u_int16_t vid,
                                      u_int16_t pvid)
{
  int ret = 0;
  struct nsm_if *zif = NULL;
  struct nsm_vlan_port *vlan_port = NULL;
  struct nsm_bridge_port *br_port = NULL;
  struct nsm_bridge *bridge = NULL;
  struct nsm_vlan *vlan = NULL, *secondary_vlan = NULL;
  struct avl_node *rnp = NULL, *rnv = NULL;
  struct nsm_vlan p, v;

  zif = (struct nsm_if *)ifp->info;

  if (! zif || ! zif->switchport)
    return NSM_VLAN_ERR_IFP_NOT_BOUND;

  br_port = zif->switchport;
  vlan_port = &br_port->vlan_port;

  bridge = zif->bridge;
  if (! bridge)
    return NSM_VLAN_ERR_IFP_NOT_BOUND;

  /* check whether port is access port or not. PVLAN host association is for
   * access ports only 
   */
  if (vlan_port->mode != NSM_VLAN_PORT_MODE_ACCESS)
    return NSM_VLAN_ERR_INVALID_MODE;

  if (vlan_port->pvlan_mode != NSM_PVLAN_PORT_MODE_HOST)
    return NSM_PVLAN_ERR_INVALID_MODE;

  NSM_VLAN_SET (&v, vid);
  NSM_VLAN_SET (&p, pvid);

  rnv = avl_search (bridge->vlan_table, (void *)&v);
  rnp = avl_search (bridge->vlan_table, (void *)&p);

  if (rnp && rnv)
    {
      vlan = (struct nsm_vlan *)rnv->info;

      secondary_vlan = (struct nsm_vlan *)rnp->info;
      
      if (!vlan || !secondary_vlan)
        return NSM_VLAN_ERR_VLAN_NOT_FOUND;
      
      if ((!(vlan->pvlan_configured)) || (!(secondary_vlan->pvlan_configured)))
        return NSM_PVLAN_ERR_NOT_CONFIGURED;
      if (vlan->pvlan_type != NSM_PVLAN_PRIMARY)
        return NSM_PVLAN_ERR_NOT_PRIMARY_VLAN;
      if ((secondary_vlan->pvlan_type != NSM_PVLAN_ISOLATED)
          && (secondary_vlan->pvlan_type != NSM_PVLAN_COMMUNITY))
        return NSM_PVLAN_ERR_INVALID_MODE;

      if (!NSM_VLAN_BMP_IS_MEMBER (vlan->pvlan_info.secondary_vlan_bmp, pvid))
        return NSM_PVLAN_ERR_SECOND_NOT_ASSOCIATED;

      /* No secondary vid */
      vlan_port->pvlan_port_info.primary_vid = 0;

      ret = hal_pvlan_host_association (bridge->name, ifp->ifindex,
                                        vid, pvid, PAL_FALSE);
      nsm_vlan_set_access_port (ifp, NSM_VLAN_DEFAULT_VID, PAL_TRUE, PAL_TRUE);
    }
  else
    {
      return NSM_VLAN_ERR_VLAN_NOT_FOUND;
    }

  if (ret < 0)
    return ret;

  return RESULT_OK;

}

int
nsm_pvlan_api_host_association_clear_all (struct interface *ifp)
{
  int ret = 0;
  struct nsm_if *zif = NULL;
  struct nsm_vlan_port *vlan_port = NULL;
  struct nsm_bridge_port *br_port = NULL;
  struct nsm_bridge *bridge = NULL;
  struct nsm_vlan *vlan = NULL;
  struct avl_node *rnv = NULL;
  struct nsm_vlan v;
  u_int16_t vid, primary_vid;

  zif = (struct nsm_if *)ifp->info;

  if (! zif || ! zif->switchport)
    return NSM_VLAN_ERR_IFP_NOT_BOUND;

  br_port = zif->switchport;
  vlan_port = &br_port->vlan_port;
  bridge = zif->bridge;
  if (! bridge)
    return NSM_VLAN_ERR_IFP_NOT_BOUND;

#ifdef HAVE_PROVIDER_BRIDGE
   if (NSM_BRIDGE_TYPE_PROVIDER (bridge))
     return NSM_PVLAN_ERR_PROVIDER_BRIDGE;
#endif

  if (vlan_port->pvlan_mode != NSM_PVLAN_PORT_MODE_HOST)
    return NSM_PVLAN_ERR_INVALID_MODE;

  vid = vlan_port->pvid;

  NSM_VLAN_SET (&v, vid);

  rnv = avl_search (bridge->vlan_table, (void *)&v);
  
  if (rnv)
    {
      vlan = (struct nsm_vlan *)rnv->info;

      if (!vlan)
        return NSM_VLAN_ERR_VLAN_NOT_FOUND;       

      if ((vlan->pvlan_type != NSM_PVLAN_ISOLATED) 
           && (vlan->pvlan_type != NSM_PVLAN_COMMUNITY))
        return NSM_PVLAN_ERR_NOT_SECONDARY_VLAN;
 
      primary_vid = vlan->pvlan_info.vid.primary_vid;
      
      ret = nsm_pvlan_api_host_association_clear (ifp, primary_vid, vid);

      if (ret < 0)
        return ret;
    }
  else
    {
      return NSM_VLAN_ERR_VLAN_NOT_FOUND;
    }

  return RESULT_OK;
}

int
nsm_pvlan_api_switchport_mapping (struct interface *ifp, u_int16_t vid,
                                  u_int16_t pvid)
{
  int ret = 0;
  struct nsm_if *zif = NULL;
  struct nsm_vlan_port *vlan_port = NULL;
  struct nsm_bridge_port *br_port = NULL;
  struct nsm_bridge *bridge = NULL;
  struct nsm_vlan *vlan = NULL, *secondary_vlan = NULL;
  struct avl_node *rnp = NULL, *rnv = NULL;
  struct nsm_vlan p, v;

  zif = (struct nsm_if *)ifp->info;

  if (! zif || ! zif->switchport)
    return NSM_VLAN_ERR_IFP_NOT_BOUND;

  br_port = zif->switchport;
  vlan_port = &br_port->vlan_port;
  bridge = zif->bridge;
  if (! bridge)
    return NSM_VLAN_ERR_IFP_NOT_BOUND;

#ifdef HAVE_PROVIDER_BRIDGE
   if (NSM_BRIDGE_TYPE_PROVIDER (bridge))
     return NSM_PVLAN_ERR_PROVIDER_BRIDGE;
#endif

  if ( nsm_bridge_is_vlan_aware(bridge->master, bridge->name) != PAL_TRUE  )
    return NSM_VLAN_ERR_BRIDGE_NOT_VLAN_AWARE;

  /* check whether port is access port or not. PVLAN host association is for
   * access ports only
   */
  if (vlan_port->mode != NSM_VLAN_PORT_MODE_ACCESS)
    return NSM_VLAN_ERR_INVALID_MODE;

  if (vlan_port->pvlan_mode != NSM_PVLAN_PORT_MODE_PROMISCUOUS)
    return NSM_PVLAN_ERR_INVALID_MODE;

  if (! bridge->vlan_table)
    return NSM_NO_VLAN_CONFIGURED;

  NSM_VLAN_SET (&v, vid);
  NSM_VLAN_SET (&p, pvid);

  rnv = avl_search (bridge->vlan_table, (void *)&v);
  rnp = avl_search (bridge->vlan_table, (void *)&p);

  if (rnv && rnp)
    {
      
      vlan = (struct nsm_vlan *)rnv->info;
      secondary_vlan = (struct nsm_vlan *)rnp->info;

      if (!vlan || !secondary_vlan)
        return NSM_VLAN_ERR_VLAN_NOT_FOUND;
      if ((!(vlan->pvlan_configured)) || (!(secondary_vlan->pvlan_configured)))
        return NSM_PVLAN_ERR_NOT_CONFIGURED;
      if (vlan->pvlan_type != NSM_PVLAN_PRIMARY)
        return NSM_PVLAN_ERR_NOT_PRIMARY_VLAN;
      if ((secondary_vlan->pvlan_type != NSM_PVLAN_ISOLATED)
          && (secondary_vlan->pvlan_type != NSM_PVLAN_COMMUNITY))
        return NSM_PVLAN_ERR_INVALID_MODE;

      if (!NSM_VLAN_BMP_IS_MEMBER (vlan->pvlan_info.secondary_vlan_bmp, pvid))
        return NSM_PVLAN_ERR_SECOND_NOT_ASSOCIATED;

      /* Set port's secondary vlan mapping. */
      NSM_VLAN_BMP_SET(vlan_port->pvlan_port_info.secondary_vlan_bmp, pvid);

     /* The default vlan will be set to primary vlan id for promiscuous ports*/
      nsm_vlan_set_access_port (ifp, vid, PAL_TRUE, PAL_TRUE);

      ret = hal_pvlan_host_association (bridge->name, ifp->ifindex, 
                                        vid, pvid, PAL_TRUE);
    }
  else
    {
      return NSM_VLAN_ERR_VLAN_NOT_FOUND;
    }

  if (ret < 0)
    return ret;

  return RESULT_OK;

}

int
nsm_pvlan_api_switchport_mapping_clear (struct interface *ifp, u_int16_t vid,
                                        u_int16_t pvid)
{
  int ret = 0;
  struct nsm_if *zif = NULL;
  struct nsm_vlan_port *vlan_port = NULL;
  struct nsm_bridge_port *br_port = NULL;
  struct nsm_bridge *bridge = NULL;
  struct nsm_vlan *vlan = NULL, *secondary_vlan = NULL;
  struct avl_node *rnp = NULL, *rnv = NULL;
  struct nsm_vlan p, v;

  zif = (struct nsm_if *)ifp->info;

  if (! zif || ! zif->switchport)
    return NSM_VLAN_ERR_IFP_NOT_BOUND;

  br_port = zif->switchport;
  vlan_port = &br_port->vlan_port;
  bridge = zif->bridge;
  if (! bridge)
    return NSM_VLAN_ERR_IFP_NOT_BOUND;

  if ( nsm_bridge_is_vlan_aware(bridge->master, bridge->name) != PAL_TRUE  )
    return NSM_VLAN_ERR_BRIDGE_NOT_VLAN_AWARE;

  if (vlan_port->mode != NSM_VLAN_PORT_MODE_ACCESS)
    return NSM_VLAN_ERR_INVALID_MODE;

  if (vlan_port->pvlan_mode != NSM_PVLAN_PORT_MODE_PROMISCUOUS)
    return NSM_PVLAN_ERR_INVALID_MODE;

  /* check whether port is access port or not. PVLAN host association is for
   * access ports only */

  /* Check whether the secondary vlan is part of primary vlan
   * else error out. to be implemented
   */

   if (! bridge->vlan_table)
     return NSM_NO_VLAN_CONFIGURED;

#ifdef HAVE_PROVIDER_BRIDGE
   if (NSM_BRIDGE_TYPE_PROVIDER (bridge))
     return NSM_PVLAN_ERR_PROVIDER_BRIDGE;
#endif

  NSM_VLAN_SET (&v, vid);
  NSM_VLAN_SET (&p, pvid);

  rnv = avl_search (bridge->vlan_table, (void *)&v);
  rnp = avl_search (bridge->vlan_table, (void *)&p);

  if (rnv && rnp)
    {

      vlan = (struct nsm_vlan *)rnv->info;
      secondary_vlan = (struct nsm_vlan *)rnp->info;
      
      if (!vlan || !secondary_vlan)
        return NSM_VLAN_ERR_VLAN_NOT_FOUND;

      if ((!(vlan->pvlan_configured)) || (!(secondary_vlan->pvlan_configured)))
        return NSM_PVLAN_ERR_NOT_CONFIGURED;
      if (vlan->pvlan_type != NSM_PVLAN_PRIMARY)
        return NSM_PVLAN_ERR_NOT_PRIMARY_VLAN;
      if ((secondary_vlan->pvlan_type != NSM_PVLAN_ISOLATED)
          && (secondary_vlan->pvlan_type != NSM_PVLAN_COMMUNITY))
        return NSM_PVLAN_ERR_INVALID_MODE;

      if (!NSM_VLAN_BMP_IS_MEMBER (vlan->pvlan_info.secondary_vlan_bmp, pvid))
        return NSM_PVLAN_ERR_SECOND_NOT_ASSOCIATED;

      /* Unset port's secondary vlan mapping. */
      NSM_VLAN_BMP_UNSET(vlan_port->pvlan_port_info.secondary_vlan_bmp, pvid);

      /* The default vlan will be set to primary vlan id for promiscuous ports
       * if no secondary vid exists. ie no member present 
       */
      /*  nsm_vlan_set_access_port (ifp, NSM_VLAN_DEFAULT_VID, PAL_TRUE, PAL_TRUE);*/
      ret = hal_pvlan_host_association (bridge->name, ifp->ifindex,
                                        vid, pvid, PAL_FALSE);
      nsm_bridge_delete_port_fdb_by_vid (bridge, zif, pvid);
    }
  else
    {
      return NSM_VLAN_ERR_VLAN_NOT_FOUND;
    }

  if (ret < 0)
    return ret;

return RESULT_OK;
}

int
nsm_pvlan_api_switchport_mapping_clear_all (struct interface *ifp)
{
  int ret = 0;
  struct nsm_vlan v;
  struct nsm_if *zif = NULL;
  u_int16_t vid, secondary_vid;
  struct nsm_vlan *vlan = NULL;
  struct avl_node *rnv = NULL;
  struct nsm_bridge *bridge = NULL;
  struct nsm_vlan_port *vlan_port = NULL;
  struct nsm_bridge_port *br_port = NULL;
  
  zif = (struct nsm_if *)ifp->info;

  if (! zif || ! zif->switchport)
    return NSM_VLAN_ERR_IFP_NOT_BOUND;

  br_port = zif->switchport;
  vlan_port = &br_port->vlan_port;
  bridge = zif->bridge;
  if (! bridge)
    return NSM_VLAN_ERR_IFP_NOT_BOUND;

  if (vlan_port->pvlan_mode != NSM_PVLAN_PORT_MODE_PROMISCUOUS)
    return NSM_PVLAN_ERR_INVALID_MODE;

  vid = vlan_port->pvid;

  NSM_VLAN_SET (&v, vid);

  rnv = avl_search (bridge->vlan_table, (void *)&v);

  if (rnv)
    {

      vlan = (struct nsm_vlan *)rnv->info;

      if (!vlan)
        return NSM_VLAN_ERR_VLAN_NOT_FOUND;

      if (!vlan->pvlan_configured)
        return NSM_PVLAN_ERR_NOT_CONFIGURED;

      if (vlan->pvlan_type != NSM_PVLAN_PRIMARY)
        return NSM_PVLAN_ERR_NOT_PRIMARY_VLAN;

      
      for (secondary_vid = 2; secondary_vid < NSM_VLAN_MAX; secondary_vid++)
        {
          if (NSM_VLAN_BMP_IS_MEMBER (vlan->pvlan_info.secondary_vlan_bmp,
                                      secondary_vid))
            {
              ret = nsm_pvlan_api_switchport_mapping_clear (ifp, vid, 
                                                            secondary_vid);

              if (ret < 0)
                return ret;
              /* Set to default vid */
              nsm_vlan_set_access_port (ifp, NSM_VLAN_DEFAULT_VID, PAL_TRUE, PAL_TRUE);
            }
        }
    }
   else
    {
      return NSM_VLAN_ERR_VLAN_NOT_FOUND;
    }

  return RESULT_OK;
}

int
nsm_pvlan_delete_all (struct nsm_bridge *bridge, nsm_vid_t vid)
{
  int ret = 0;
  struct nsm_vlan v;
  struct nsm_if *zif;
  struct avl_node *avl_port;
  struct nsm_vlan *vlan = NULL;
  struct interface *ifp = NULL;
  struct avl_node *rnv = NULL;
  nsm_vid_t pvid, secondary_vid;
  struct nsm_bridge_port *br_port;
  struct nsm_vlan_port *vlan_port = NULL;

  NSM_VLAN_SET (&v, vid);
  rnv = avl_search (bridge->vlan_table, (void *)&v);
  if (!rnv || !rnv->info)
    return NSM_VLAN_ERR_VLAN_NOT_FOUND;

   vlan = (struct nsm_vlan *)rnv->info;

   if (!(vlan->pvlan_configured))
     return NSM_PVLAN_ERR_NOT_CONFIGURED;


  for (avl_port = avl_top (bridge->port_tree); avl_port;
       avl_port = avl_next (avl_port))
    {
      if ((br_port = (struct nsm_bridge_port *)
                             AVL_NODE_INFO (avl_port)) == NULL)
        continue;

      if ((zif = br_port->nsmif) == NULL)
        continue;

      if ((ifp = zif->ifp) == NULL)
        continue;

      vlan_port = &br_port->vlan_port;

      if ( !vlan_port )
        continue;

      if (!(vlan_port->pvlan_configured))
        continue;

      /* Check for Port is the member of this vlan 
       * is below
       */

      if (ifp)
        {
          if (vlan_port->pvlan_mode == NSM_PVLAN_PORT_MODE_HOST)
            {
              if (vid == vlan_port->pvid)
                {
                  pvid = vlan_port->pvlan_port_info.primary_vid; 
                  ret = nsm_pvlan_api_host_association_clear (ifp, pvid, vid);
                }
              else if (vid == vlan_port->pvlan_port_info.primary_vid)
                {
                  ret = nsm_pvlan_api_host_association_clear
                    (ifp, vid, vlan_port->pvid);
                }
            }
          if (vlan_port->pvlan_mode == NSM_PVLAN_PORT_MODE_PROMISCUOUS)
            {
              if (vid == vlan_port->pvid)
                {
                  nsm_pvlan_api_switchport_mapping_clear_all (ifp);
                }

              else if (NSM_VLAN_BMP_IS_MEMBER
                  (vlan_port->pvlan_port_info.secondary_vlan_bmp, vid))
                {
                  nsm_pvlan_api_switchport_mapping_clear (ifp, vlan_port->pvid, vid);
                }
            }
        }
    }

  if (vlan->pvlan_type == NSM_PVLAN_PRIMARY)
    {
      for (secondary_vid =2; secondary_vid < NSM_VLAN_MAX;
                                            secondary_vid++)
        {
          if (NSM_VLAN_BMP_IS_MEMBER
                 (vlan->pvlan_info.secondary_vlan_bmp,secondary_vid));
             nsm_pvlan_associate_clear (bridge->master, bridge->name,
                                                  vid, secondary_vid);
        }
    }
  else if ((vlan->pvlan_type == NSM_PVLAN_ISOLATED)
           || (vlan->pvlan_type == NSM_PVLAN_COMMUNITY))
    {
       if (vlan->pvlan_info.vid.primary_vid !=0)
         nsm_pvlan_associate_clear (bridge->master, bridge->name, 
                                    vlan->pvlan_info.vid.primary_vid, vid);
    }
             
  return 0;
} 
 
#endif /*end of HAVE_VLAN */
