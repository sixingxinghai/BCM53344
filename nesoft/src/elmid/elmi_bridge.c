/**@file elmi_bridge.c
   @brief  This file contains functions related to Layer 2 bridge.
*/
/* Copyright (C) 2010  All Rights Reserved. */

#include "pal.h"
#include "lib.h"
#include "nsm_message.h"
#include "nsm_client.h"
#include "table.h"
#include "l2_timer.h"
#include "l2lib.h"
#include "avl_tree.h"
#include "linklist.h"
#include "elmid.h"
#include "elmi_bridge.h"
#include "elmi_types.h"
#include "elmi_api.h"
#include "elmi_port.h"

static int
elmi_bridge_port_idx_cmp (void *data1, void *data2)
{
  struct elmi_ifp *port1 = NULL;
  struct elmi_ifp *port2 = NULL;

  if (data1 == NULL || data2 == NULL)
    return ELMI_FAILURE;

  port1 = (struct elmi_ifp *) data1;
  port2 = (struct elmi_ifp *) data2;

  if (port1->ifindex > port2->ifindex)
    return ELMI_OK;

  if (port2->ifindex > port1->ifindex)
    return ELMI_FAILURE;

  return ELMI_SUCCESS;

}

s_int16_t
elmi_bridge_add (u_char *br_name, u_int8_t bridge_type, 
                 u_int8_t is_edge)
{
  struct elmi_bridge *bridge = NULL;
  struct elmi_master *elmi_master = NULL;
  s_int16_t ret = 0;

  if (br_name == NULL)
    return RESULT_ERROR;

  elmi_master = elmi_master_get ();
  if (!elmi_master)
    return RESULT_ERROR;

  bridge = XCALLOC (MTYPE_ELMI_BRIDGE, sizeof (struct elmi_bridge));

  if (bridge == NULL)
    return RESULT_ERROR;

  pal_strcpy (bridge->name, br_name);

  /* Update the bridge type*/
  if ((bridge_type == ELMI_BRIDGE_TYPE_PROVIDER_RSTP)||
      (bridge_type == ELMI_BRIDGE_TYPE_PROVIDER_MSTP)) 
    {
      if (is_edge == PAL_TRUE)
        bridge->bridge_type = ELMI_BRIDGE_TYPE_MEF_PE;
    }
  else if ((bridge_type == ELMI_BRIDGE_TYPE_STP_VLANAWARE) 
           || (bridge_type == ELMI_BRIDGE_TYPE_RSTP_VLANAWARE) ||
             bridge_type == ELMI_BRIDGE_TYPE_MSTP)
    bridge->bridge_type = ELMI_BRIDGE_TYPE_MEF_CE;
  else 
    bridge->bridge_type = bridge_type;

  ret = avl_create (&bridge->port_tree, 0, elmi_bridge_port_idx_cmp);

  if(ret < 0)
    {
      XFREE (MTYPE_ELMI_BRIDGE, bridge);
      return (ELMI_FAILURE);
    }

  bridge->vlan_table = route_table_init ();

  listnode_add (elmi_master->bridge_list, bridge);

  return ELMI_SUCCESS;
}

s_int16_t
elmi_bridge_delete (u_char *br_name)
{
  struct elmi_bridge *bridge = NULL;
  struct elmi_ifp *elmi_ifp = NULL;
  struct avl_node *avl_port = NULL;
  struct elmi_master *elmi_master = NULL;

  if (br_name == NULL)
    {
      return RESULT_ERROR;
    }

  bridge = elmi_find_bridge (br_name);

  if (bridge == NULL)
    {
      return RESULT_ERROR;
    }

  AVL_TREE_LOOP (bridge->port_tree, elmi_ifp, avl_port) 
    {
      elmi_delete_port (elmi_ifp);

      /* Delete interface from bridge port list */
      avl_remove (bridge->port_tree, elmi_ifp);
    }

  if (bridge->port_tree)
    avl_tree_free (&bridge->port_tree, NULL);

  /* clean vlan_bmp */
  if (bridge->vlan_table)
    {
      XFREE (MTYPE_ROUTE_TABLE, bridge->vlan_table);
      bridge->vlan_table = NULL; 
    }

  elmi_master = elmi_master_get ();

  listnode_delete (elmi_master->bridge_list, bridge);
  XFREE (MTYPE_ELMI_BRIDGE, bridge);

  bridge = NULL;

  return RESULT_OK;

}    

void
elmi_bridge_free (struct elmi_bridge *bridge)
{
  struct elmi_master *elmi_master = NULL;

  elmi_master = elmi_master_get ();

  listnode_delete (elmi_master->bridge_list, bridge);
  XFREE (MTYPE_ELMI_BRIDGE, bridge);

  bridge = NULL;
 
  return;

}

s_int16_t
elmi_bridge_add_port (char *bridge_name, struct interface *ifp)
{
  struct elmi_bridge *bridge = NULL;
  struct elmi_ifp *elmi_if = NULL;

  if (bridge_name == NULL || !ifp)
    return RESULT_ERROR;
  
  bridge = elmi_find_bridge (bridge_name);

  if (bridge == NULL)
    return RESULT_ERROR;

  elmi_if = elmi_find_bridge_port (bridge, ifp);

  if (elmi_if != NULL)
    return RESULT_ERROR;
  
  elmi_if = ifp->info;

  if (elmi_if == NULL)
    return RESULT_ERROR;
  
  elmi_if->br = bridge;

  elmi_if->vlan_bmp = XCALLOC (MTYPE_ELMI_VLAN_BMP,
                               sizeof (struct elmi_vlan_bmp));
  if (elmi_if->vlan_bmp == NULL)
    return RESULT_ERROR;

  pal_strcpy(ifp->bridge_name, bridge->name);

  if (elmi_if->evc_status_list == NULL)
    elmi_if->evc_status_list = list_new();

  if (elmi_if->evc_status_list == NULL) 
    {
      XFREE (MTYPE_ELMI_VLAN_BMP,  elmi_if->vlan_bmp);
      return RESULT_ERROR;
    }
  elmi_if->evc_status_list->cmp = (list_cmp_cb_t)elmi_evc_ref_id_cmp;

  if (elmi_if->cevlan_evc_map_list == NULL)
    {
      elmi_if->cevlan_evc_map_list = list_new();
    }

  if (elmi_if->cevlan_evc_map_list == NULL)
    {
      XFREE (MTYPE_ELMI_VLAN_BMP,  elmi_if->vlan_bmp);
      /* fress elmi_if->evc_status_list */
      list_free (elmi_if->evc_status_list);
      return RESULT_ERROR;
    }

  elmi_if->cevlan_evc_map_list->cmp = 
    (list_cmp_cb_t)elmi_cevlan_evc_ref_id_cmp;

  avl_insert (bridge->port_tree, elmi_if);

  /* If Elmi is enabled on bridge level, 
   * then it should be enabled on this port also.
   */

  if (bridge->elmi_enabled)
    {
      elmi_api_enable_port (ifp->ifindex);
    }

  return RESULT_OK;

}

s_int16_t
elmi_bridge_delete_port (u_char *bridge_name, struct interface *ifp)
{
  struct elmi_bridge *bridge = NULL;
  struct elmi_ifp *elmi_if = NULL;

  if (bridge_name == NULL || ifp == NULL)
    return RESULT_ERROR;

  bridge = elmi_find_bridge (bridge_name);

  if (bridge == NULL)
    return RESULT_ERROR;

  elmi_if = elmi_find_bridge_port (bridge, ifp);

  if (elmi_if == NULL)
    return RESULT_ERROR;

  elmi_delete_port (elmi_if);

  if (bridge->port_tree)
   avl_remove (bridge->port_tree, elmi_if);

  return RESULT_OK;
}

s_int16_t
elmi_bridge_vlan_add (u_char *bridge_name, u_int16_t vid,
                      u_char *vlan_name, u_int32_t flags)
{
  struct elmi_bridge *bridge = NULL;
  struct elmi_prefix_vlan p;
  struct route_node *rn = NULL;
  struct elmi_vlan *v = NULL;

  if (!bridge_name)
    {
      return RESULT_ERROR;
    }

  bridge = elmi_find_bridge (bridge_name);

  if (bridge == NULL || bridge->vlan_table == NULL)
    {
      return RESULT_ERROR;
    }

  ELMI_PREFIX_VLAN_SET (&p, vid);

  rn = route_node_lookup (bridge->vlan_table, (struct prefix *)&p);
  if (rn)
    {
      v = rn->info;

      if (vlan_name)
        pal_strcpy (v->vlan_name, vlan_name);

      v->flags = flags;
    }
  else
    {
      v = XCALLOC (MTYPE_ELMI_VLAN, sizeof (struct elmi_vlan));
      v->vlan_id = vid;
      
      if (vlan_name)
        pal_strcpy (v->vlan_name, vlan_name);

      v->flags = flags;

      rn = route_node_get (bridge->vlan_table, (struct prefix *)&p);
      if (!rn)
        {
          XFREE (MTYPE_ELMI_VLAN, v);
          return RESULT_ERROR;
        }

      ELMI_VLAN_INFO_SET (rn, v);
    }

  return RESULT_OK;
}

s_int16_t
elmi_bridge_vlan_delete (char *bridge_name, u_int16_t vid)
{
  struct elmi_bridge *bridge = NULL;
  struct elmi_prefix_vlan p;
  struct route_node *rn = NULL;
  struct elmi_vlan *v = NULL;
  s_int16_t ret = RESULT_ERROR;

  if (!bridge_name)
    return ret;

  bridge = elmi_find_bridge (bridge_name);

  if (bridge == NULL || bridge->vlan_table == NULL)
    return ret;

  ELMI_PREFIX_VLAN_SET (&p, vid);

  rn = route_node_lookup (bridge->vlan_table, (struct prefix *) &p);
  if (rn)
    {
      route_unlock_node (rn);
      if (! rn->info)
        return ret;

      v = rn->info;

      ELMI_VLAN_INFO_UNSET (rn);

      XFREE (MTYPE_ELMI_VLAN, v);
    }

  return RESULT_OK;
}

struct elmi_vlan *
elmi_bridge_vlan_lookup (struct elmi_bridge *bridge, u_int16_t vid)
{
  struct elmi_prefix_vlan p;
  struct route_node *rn = NULL;
  struct elmi_vlan *v = NULL;

  if (bridge == NULL || bridge->vlan_table == NULL)
    return NULL;

  ELMI_PREFIX_VLAN_SET (&p, vid);

  rn = route_node_lookup (bridge->vlan_table, (struct prefix *) &p);

  if (rn)
    {
      route_unlock_node (rn);

      if (! rn->info)
        return NULL;

      v = rn->info;

      return v;
    }

  return NULL;

}

struct elmi_vlan_bmp *
elmi_vlan_port_map (u_int32_t ifindex)
{
  struct elmi_bridge *bridge = NULL;
  struct elmi_vlan_bmp *bmp = NULL;
  struct listnode *node = NULL;
  struct elmi_master *elmi_master = NULL;
  struct elmi_ifp *port = NULL;
  struct interface *ifp = NULL;

  elmi_master = elmi_master_get ();
  ifp = ifg_lookup_by_index (&ZG->ifg, ifindex);
  
  if (!ifp)
    return NULL;

  LIST_LOOP (elmi_master->bridge_list, bridge, node)
    {
      if ((port = elmi_find_bridge_port (bridge, ifp)) != NULL)
        break;
    }

  if (!port)
    return NULL;

  bmp = port->vlan_bmp;

  if (!bmp)
    {  
      bmp = XCALLOC (MTYPE_ELMI_VLAN_BMP,
          sizeof (struct elmi_vlan_bmp));

      if(bmp)
        port->vlan_bmp = bmp;
    }

  return bmp;
}

s_int16_t
elmi_bridge_list_delete (void)
{
  struct listnode *node_next = NULL;
  struct elmi_bridge *bridge = NULL;
  struct listnode *node = NULL;
  struct elmi_master *elmim = NULL;

  elmim = elmi_master_get ();

  for (node = LISTHEAD (elmim->bridge_list); node; node = node_next)
    {
      node_next = node->next;
      bridge = GETDATA (node);
      elmi_bridge_free (bridge);
    }
                                                                                
  return RESULT_OK;
}

struct elmi_bridge *
elmi_find_bridge (char *name)
{
  struct elmi_bridge *bridge = NULL;
  struct listnode *node = NULL;
  struct elmi_master *elmi_master = NULL;

  elmi_master = elmi_master_get ();

  if (!elmi_master)
    return NULL;

  LIST_LOOP (elmi_master->bridge_list, bridge, node)
    {
      if (pal_strcmp (bridge->name, name) == 0)
        {
          return bridge;
        }
    }

  return NULL;
}

struct elmi_ifp *
elmi_find_bridge_port (struct elmi_bridge *bridge, struct interface *ifp)
{
  struct elmi_ifp *br_port = NULL;
  struct elmi_ifp tmp_br_port;
  struct avl_node *avl_node = NULL; 

  if (!ifp)
    return NULL;

  if (!bridge)
    return NULL;

  tmp_br_port.ifindex = ifp->ifindex;
  avl_node = avl_search (bridge->port_tree, &tmp_br_port);

  if (avl_node == NULL)
    return NULL;

  br_port = AVL_NODE_INFO (avl_node);

  return br_port;
}
