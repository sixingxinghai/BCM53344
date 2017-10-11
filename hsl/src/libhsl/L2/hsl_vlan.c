/* Copyright (C) 2004  All Rights Reserved. */

#include "config.h"
#include "hsl_os.h"
#include "hsl_types.h"

#ifdef HAVE_VLAN
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

#if defined HAVE_PROVIDER_BRIDGE || defined HAVE_VLAN_STACK

struct hsl_vlan_regis_key *
hsl_vlan_regis_key_init (void)
{
  return oss_malloc (sizeof (struct hsl_vlan_regis_key), OSS_MEM_HEAP);
}
                                                                                                                                                             
/*
  Deinit registration table port.
*/
void
hsl_vlan_regis_key_deinit (void *key)
{
  if (key)
    oss_free (key, OSS_MEM_HEAP);
                                                                                                                                                             
  return;
}
#endif /* HAVE_PROVIDER_BRIDGE || HAVE_VLAN_STACK */

/*
  hsl_vlan_agg_port_bind_update
  Update vlan <-> agg <-> port hierarchy 
  Bind operation:
  If port is part of any vlans, we need to build the 
  following heirarchy:
  ++++++++++
  +  Vlan  +
  ++++++++++
  +
  ++++++++++
  + L2 Agg +
  ++++++++++
  +
  +++++++++++
  + L2 Port +
  +++++++++++
  Unbind operation  
  If aggregator is part of any vlans, we need to change the heirarchy to:
  ++++++++++
  +  Vlan  +
  ++++++++++
  +
  +++++++++++
  + L2 Port +
  +++++++++++
*/

hsl_vid_t
hsl_get_pvid (struct hsl_if *ifp)
{
  struct hsl_bridge_port *port;
  struct hsl_port_vlan *vlan;
  hsl_vid_t pvid = 0;

  HSL_FN_ENTER();

  if (!ifp)
    HSL_FN_EXIT (pvid);

  if (ifp->type == HSL_IF_TYPE_L2_ETHERNET)
    {
      port = ifp->u.l2_ethernet.port;

      if (!port)
        HSL_FN_EXIT (pvid);

      vlan = port->vlan;

      if (!vlan)
        HSL_FN_EXIT (pvid);

      pvid = vlan->pvid;
    }
  else if (ifp->type == HSL_IF_TYPE_IP)
    {
      pvid = ifp->u.ip.vid;
    }

  HSL_FN_EXIT (pvid);
}

int 
hsl_vlan_agg_port_bind_update(struct hsl_if *port_ifp, 
                              struct hsl_if *agg_ifp,
                              HSL_BOOL do_bind)
{
  struct hsl_if *iterator;   
  struct hsl_if *vlan_ifp;   
  struct hsl_if_list *node = NULL;   
  struct hsl_if_list *node_next = NULL;   

  HSL_FN_ENTER();

  if((!port_ifp) || (!agg_ifp)) 
    HSL_FN_EXIT(HSL_IFMGR_ERR_INVALID_PARAM);
   
  iterator = (do_bind)?port_ifp:agg_ifp;  

  for(node = iterator->parent_list; node; node = node_next)
    {
      /* Vlan interface candidate. */
      vlan_ifp = node->ifp;
      node_next = node->next;
      /* Compare name to check if interface is svi. */ 
      if ( (vlan_ifp) && (!(memcmp(vlan_ifp->name, "vlan", 4))) )
        { 
          if(do_bind)
            {
              /* Unbind vlan ifp from ifp,and bind it to agg_ifp */
              hsl_ifmgr_unbind2(vlan_ifp, port_ifp);
              /* Bind agg to a vlan */ 
              if ((agg_ifp->children_list == NULL) &&
                  (hsl_ifmgr_isbound(vlan_ifp, agg_ifp) != HSL_TRUE)) 
                hsl_ifmgr_bind2(vlan_ifp, agg_ifp);
            }
          else
            {
              /* Bind vlan ifp to port */
              hsl_ifmgr_bind2(vlan_ifp, port_ifp);
            }
        } /* Vlan interface found. */
    }  /* for parents node list. */

  HSL_FN_EXIT(0);
}

/*
  hsl_vlan_agg_unbind
  Unbind agg from all vlans. 
*/
int 
hsl_vlan_agg_unbind( struct hsl_if *agg_ifp)
{
  struct hsl_if *vlan_ifp;   
  struct hsl_if_list *node = NULL;
  struct hsl_if_list *node_next = NULL;

  HSL_FN_ENTER();

  if(!agg_ifp) 
    HSL_FN_EXIT(HSL_IFMGR_ERR_INVALID_PARAM);

  for(node = agg_ifp->parent_list; node; node = node_next)
    {
      /* Vlan interface candidate. */
      vlan_ifp = node->ifp;
      node_next = node->next;
      /* Compare name to check if interface is svi. */ 
      if ( (vlan_ifp) && (!(memcmp(vlan_ifp->name, "vlan", 4))) )
        { 
          /* Unbind it from agg_ifp */
          hsl_ifmgr_unbind2(vlan_ifp, agg_ifp);
  
        } /* Vlan interface found. */
    }  /* for parents node list. */

  HSL_FN_EXIT(0);
}


/*
  Port compare routine.
*/
int
_hsl_vlan_port_cmp (void *param1, void *param2)
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
  Initialize vlan->port map.
*/
struct hsl_vlan_port *
hsl_vlan_port_map_init (void)
{
  struct hsl_vlan_port *v = NULL;
  int ret;

  v = oss_malloc (sizeof (struct hsl_vlan_port), OSS_MEM_HEAP);
  if (! v)
    return v;

  /* Create port tree. */
  ret = hsl_avl_create (&v->port_tree, 0, _hsl_vlan_port_cmp);
  if (ret < 0)
    {
      oss_free (v, OSS_MEM_HEAP);

      return NULL;
    }

#ifdef HAVE_PVLAN
  v->vlan_type = HAL_PVLAN_NONE;
#endif /* HAVE_PVLAN */
  return v;
}

/*
  deinitialize vlan->port map.
*/
int
hsl_vlan_port_map_deinit (struct hsl_vlan_port *v)
{
  if (p_hsl_bridge_master && p_hsl_bridge_master->bridge && v)
    {
      if (p_hsl_bridge_master && p_hsl_bridge_master->hw_cb->delete_vlan)
        (*p_hsl_bridge_master->hw_cb->delete_vlan) (p_hsl_bridge_master->bridge, v);

      hsl_avl_tree_free (&v->port_tree, NULL);

#ifdef HAVE_PVLAN
      if (v->vlan_tree)    
        hsl_avl_tree_free (&v->vlan_tree, NULL);    
#endif /* HAVE_PVLAN */

      oss_free (v, OSS_MEM_HEAP);
    }

  return 0;
}

/*
  VLAN compare.
*/
static int
_hsl_port_vlan_cmp (void *param1, void *param2)
{
  struct hsl_vlan_port_attr *p1 = (struct hsl_vlan_port_attr*) param1;
  struct hsl_vlan_port_attr *p2 = (struct hsl_vlan_port_attr*) param2;

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
  Initialize port->vlan map.
*/
struct hsl_port_vlan *
hsl_port_vlan_map_init (void)
{
  int ret;
  struct hsl_port_vlan *port = NULL;
  
  port = oss_malloc (sizeof (struct hsl_port_vlan), OSS_MEM_HEAP);
  if (! port)
    return NULL;

  /* Create vlan tree. */
  ret = hsl_avl_create (&port->vlan_tree, 0, _hsl_port_vlan_cmp);
  if (ret < 0)
    { 
      hsl_port_vlan_map_deinit (port);
      return NULL;
    } 
  return port;
}

/*
  Deinitialize port->vlan map.
*/
int
hsl_port_vlan_map_deinit (struct hsl_port_vlan *port)
{
  if (p_hsl_bridge_master && port)
    {
      if(port->vlan_tree)
        hsl_avl_tree_free (&port->vlan_tree, hsl_free);    
      oss_free (port, OSS_MEM_HEAP);
    }

  return 0;
}

/* 
   Add vlan to port. 
*/  
static int
_hsl_port_vlan_add_update(struct hsl_port_vlan *port, hsl_vid_t vid, HSL_BOOL etagged)
{
  struct hsl_vlan_port_attr vlan_attr,*p_vlan_attr;
  struct hsl_avl_node *node;

  if (! port)
    return -1;

  vlan_attr.vid = vid; 
  node = hsl_avl_lookup (port->vlan_tree, &vlan_attr);
  if (node)
    {
      p_vlan_attr = (struct hsl_vlan_port_attr *) HSL_AVL_NODE_INFO (node);
      if (! p_vlan_attr)
        return -1;
    } 
  else
    {  
      p_vlan_attr = oss_malloc (sizeof (struct hsl_vlan_port_attr), OSS_MEM_HEAP);
      if (! p_vlan_attr)
        return -1;

      p_vlan_attr->vid = vid;
      /* Insert into VLAN tree. */
      hsl_avl_insert (port->vlan_tree, p_vlan_attr);
    }
  p_vlan_attr->etagged = etagged;

  return 0;
}

/* 
   Remove vlan from port. 
*/  
static int
_hsl_port_vlan_delete(struct hsl_port_vlan *port, hsl_vid_t vid)
{
  struct hsl_vlan_port_attr vlan_attr, *p_vlan_attr;
  struct hsl_avl_node *node;

  if (! port)
    return -1;

  vlan_attr.vid = vid; 
  node = hsl_avl_lookup (port->vlan_tree, &vlan_attr);
  if (! node)
    return -1;

  p_vlan_attr = (struct hsl_vlan_port_attr *) HSL_AVL_NODE_INFO (node);
  if (! p_vlan_attr)
    return -1;

  hsl_avl_delete (port->vlan_tree, p_vlan_attr);
  oss_free(p_vlan_attr, OSS_MEM_HEAP);

  return 0;
}

/* 
   Get vlan attributes for port. 
*/  
struct hsl_vlan_port_attr *
hsl_port_vlan_get_attr(struct hsl_port_vlan *port, hsl_vid_t vid)
{
  struct hsl_vlan_port_attr vlan_attr, *p_vlan_attr; 
  struct hsl_avl_node *node;

  if (! port)
    return NULL;

  vlan_attr.vid = vid; 
  node = hsl_avl_lookup (port->vlan_tree, &vlan_attr);
  if (! node)
    return NULL;

  p_vlan_attr = (struct hsl_vlan_port_attr *) HSL_AVL_NODE_INFO (node);
  if (! p_vlan_attr)
    return NULL;

  return p_vlan_attr;
}

/*
  Add VLAN.
*/
int
hsl_vlan_add (char *name, enum hal_vlan_type type, hsl_vid_t vid)
{
  struct hsl_vlan_port *v = NULL;
  struct hsl_bridge *bridge;
  s_int32_t ret=0;

  HSL_BRIDGE_LOCK;
  bridge = p_hsl_bridge_master->bridge;
  if (! bridge && memcmp (bridge->name, name, HAL_BRIDGE_NAME_LEN))
    {
      HSL_BRIDGE_UNLOCK;
      return HSL_ERR_BRIDGE_INVALID_PARAM;
    }

  if ((bridge->type == HAL_BRIDGE_PROVIDER_RSTP
       || bridge->type == HAL_BRIDGE_PROVIDER_MSTP)
      && type == HAL_VLAN_TYPE_CVLAN)
    {
      HSL_BRIDGE_UNLOCK;
      return 0;
    }
  
  v = hsl_vlan_port_map_init ();
  if (!v)
    {
      HSL_BRIDGE_UNLOCK;
      return HSL_ERR_BRIDGE_NOMEM;
    }
  v->vid = vid;

  /* Insert into VLAN tree. */
  hsl_avl_insert (bridge->vlan_tree, v);

  /* Add VLAN to hardware. */
  if (p_hsl_bridge_master->hw_cb && p_hsl_bridge_master->hw_cb->add_vlan)
    ret = (*p_hsl_bridge_master->hw_cb->add_vlan) (bridge, v);

  HSL_BRIDGE_UNLOCK;

  if (ret < 0)
    return ret;

  return 0;
}

/*
  Delete VLAN.
*/
int
hsl_vlan_delete (char *name, hsl_vid_t vid)
{
  struct hsl_bridge_port *port;
  struct hsl_vlan_port *v, tv;
  struct hsl_avl_node *node = NULL;
  struct hsl_avl_node *node_next = NULL;
  struct hsl_bridge *bridge;
  struct hsl_if *ifp;
  struct hsl_port_vlan *port_vlan;

  HSL_BRIDGE_LOCK;
  bridge = p_hsl_bridge_master->bridge;
  if (! bridge && memcmp (bridge->name, name, HAL_BRIDGE_NAME_LEN))
    {
      HSL_BRIDGE_UNLOCK;
      return HSL_ERR_BRIDGE_INVALID_PARAM;
    }

  tv.vid = vid;
  node = hsl_avl_lookup (bridge->vlan_tree, &tv);
  if (! node)
    {
      HSL_BRIDGE_UNLOCK;
      return HSL_ERR_BRIDGE_INVALID_PARAM;
    }

  v = (struct hsl_vlan_port *) HSL_AVL_NODE_INFO (node);
  if (! v)
    {
      HSL_BRIDGE_UNLOCK;
      return HSL_ERR_BRIDGE_INVALID_PARAM;
    }

  /* Delete VLAN from hardware. */
  if (p_hsl_bridge_master->hw_cb && p_hsl_bridge_master->hw_cb->delete_vlan)
    (*p_hsl_bridge_master->hw_cb->delete_vlan) (bridge, v);

  /* Delete port->vlan map. */
  for (node = hsl_avl_top (v->port_tree); node; node = node_next)
    {
      node_next = hsl_avl_next (node);
      ifp = HSL_AVL_NODE_INFO (node);
      if (! ifp)
        continue;

      port = ifp->u.l2_ethernet.port;
      if (! port)
        continue;

      port_vlan = port->vlan;
      if (! port_vlan)
        continue;

      /* Unset port->vlan map. */
      _hsl_port_vlan_delete(port_vlan,vid);
    }

  /* Delete vlan->port map. */
  hsl_avl_delete (bridge->vlan_tree, v);
  hsl_vlan_port_map_deinit (v);

  HSL_BRIDGE_UNLOCK;

  return 0;
}

/*
  Set port type.
*/
int
hsl_vlan_set_port_type (char *name, 
                        hsl_ifIndex_t ifindex, 
                        enum hal_vlan_port_type port_type,
                        enum hal_vlan_port_type sub_port_type,
                        enum hal_vlan_acceptable_frame_type acceptable_frame_types,
                        u_int16_t enable_ingress_filter)
{
  struct hsl_if *ifp;
  struct hsl_bridge_port *port;
  struct hsl_bridge *bridge;

  HSL_BRIDGE_LOCK;
  bridge = p_hsl_bridge_master->bridge;
  if (! bridge && memcmp (bridge->name, name, HAL_BRIDGE_NAME_LEN))
    {
      HSL_BRIDGE_UNLOCK;
      return HSL_ERR_BRIDGE_INVALID_PARAM;
    }

  ifp = hsl_ifmgr_lookup_by_index (ifindex);
  if (! ifp)
    {
      HSL_BRIDGE_UNLOCK;
      return HSL_ERR_BRIDGE_INVALID_PARAM;
    }

  if (ifp->type != HSL_IF_TYPE_L2_ETHERNET)
    {
      HSL_IFMGR_IF_REF_DEC (ifp);
      HSL_BRIDGE_UNLOCK;
      return HSL_ERR_BRIDGE_INVALID_PARAM;
    }

  port = ifp->u.l2_ethernet.port;
  if (! port)
    {
      HSL_IFMGR_IF_REF_DEC (ifp);
      HSL_BRIDGE_UNLOCK;
      return HSL_ERR_BRIDGE_PORT_NOT_EXISTS;
    }


  /* Set acceptable frame type in hardware. */
  if (p_hsl_bridge_master->hw_cb && p_hsl_bridge_master->hw_cb->set_vlan_port_type)
    (*p_hsl_bridge_master->hw_cb->set_vlan_port_type) (p_hsl_bridge_master->bridge, port,
                                                       port_type, sub_port_type, acceptable_frame_types,
                                                       enable_ingress_filter);

  port->type = port_type;
  port->sub_type = sub_port_type;

  HSL_IFMGR_IF_REF_DEC (ifp);
  HSL_BRIDGE_UNLOCK;
  
  return 0;
}

/*
  Set dot1q State.
*/
int
hsl_vlan_set_dot1q_state (hsl_ifIndex_t ifindex,
                          u_int8_t enable,
                          u_int16_t enable_ingress_filter)
{
  struct hsl_if *ifp;

  HSL_BRIDGE_LOCK;

  ifp = hsl_ifmgr_lookup_by_index (ifindex);
  if (! ifp)
    {
      HSL_BRIDGE_UNLOCK;
      return HSL_ERR_BRIDGE_INVALID_PARAM;
    }

  if (ifp->type != HSL_IF_TYPE_L2_ETHERNET)
    {
      HSL_IFMGR_IF_REF_DEC (ifp);
      HSL_BRIDGE_UNLOCK;
      return HSL_ERR_BRIDGE_INVALID_PARAM;
    }

  /* Set acceptable frame type in hardware. */

  if (enable == HSL_TRUE)
    {
      if (p_hsl_bridge_master->hw_cb 
          && p_hsl_bridge_master->hw_cb->set_dot1q_state)
        (*p_hsl_bridge_master->hw_cb->set_dot1q_state) 
                                       (ifp, HSL_TRUE, enable_ingress_filter);
    }
  else
    {
      if (p_hsl_bridge_master->hw_cb 
          && p_hsl_bridge_master->hw_cb->set_dot1q_state)
        (*p_hsl_bridge_master->hw_cb->set_dot1q_state) 
                                       (ifp, HSL_FALSE, enable_ingress_filter);
    }

  HSL_IFMGR_IF_REF_DEC (ifp);
  HSL_BRIDGE_UNLOCK;
  
  return 0;
}
/*
  Set default PVID.
*/
int
hsl_vlan_set_default_pvid (char *name, hsl_ifIndex_t ifindex, hsl_vid_t pvid, enum hal_vlan_egress_type egress)
{
  struct hsl_if *ifp;
  struct hsl_bridge_port *port;
  struct hsl_bridge *bridge;
  struct hsl_port_vlan *port_vlan;
  struct hsl_if *ifpvlan = NULL;
  char ifname[HAL_IFNAME_LEN];
   
  HSL_BRIDGE_LOCK;
  bridge = p_hsl_bridge_master->bridge;
  if (! bridge && memcmp (bridge->name, name, HAL_BRIDGE_NAME_LEN))
    {
      HSL_BRIDGE_UNLOCK;
      return HSL_ERR_BRIDGE_INVALID_PARAM;
    }

  ifp = hsl_ifmgr_lookup_by_index (ifindex);
  if (! ifp)
    {
      HSL_BRIDGE_UNLOCK;
      return HSL_ERR_BRIDGE_INVALID_PARAM;
    }

  if (ifp->type != HSL_IF_TYPE_L2_ETHERNET)
    {
      HSL_IFMGR_IF_REF_DEC (ifp);
      HSL_BRIDGE_UNLOCK;
      return HSL_ERR_BRIDGE_INVALID_PARAM;
    }

  port = ifp->u.l2_ethernet.port;
  if (! port)
    {
      HSL_IFMGR_IF_REF_DEC (ifp);
      HSL_BRIDGE_UNLOCK;
      return HSL_ERR_BRIDGE_PORT_NOT_EXISTS;
    }

  port_vlan = port->vlan;
  if (! port_vlan)
    {
      HSL_IFMGR_IF_REF_DEC (ifp);
      HSL_BRIDGE_UNLOCK;
      return HSL_ERR_BRIDGE_PORT_NOT_EXISTS;
    }

  port_vlan->pvid = pvid;
  _hsl_port_vlan_add_update(port_vlan, pvid, 
                            (egress == HAL_VLAN_EGRESS_TAGGED)?HSL_TRUE:HSL_FALSE);

  /* Set default PVID in hardware. */
  if (p_hsl_bridge_master->hw_cb && p_hsl_bridge_master->hw_cb->set_default_pvid)
    (*p_hsl_bridge_master->hw_cb->set_default_pvid) (bridge, port_vlan, egress);

  /* Associate the port with vlan interface */
  memset(ifname, 0, HAL_IFNAME_LEN);
  snprintf(ifname, HAL_IFNAME_LEN, "%s%s.%d", "vlan", bridge->name, pvid);
  ifpvlan = hsl_ifmgr_lookup_by_name (ifname);
  if (ifpvlan)
    {
      hsl_ifmgr_set_acceptable_packet_types (ifp, HSL_IF_PKT_ALL);
      HSL_IFMGR_IF_REF_DEC (ifpvlan);
    }
   
  HSL_LOG(HSL_LOG_GENERAL, HSL_LEVEL_INFO, " Set PVID %d to interface %s\n", pvid, ifp->name);

  HSL_IFMGR_IF_REF_DEC (ifp);
  HSL_BRIDGE_UNLOCK;

  return 0;
}

/*
  Add VID to port.
*/
int
hsl_vlan_add_vid_to_port (char *name, hsl_ifIndex_t ifindex, hsl_vid_t vid, 
                          enum hal_vlan_egress_type egress)
{
  struct hsl_if *ifp;
  struct hsl_if *ifp_parent;
  struct hsl_bridge_port *port;
  struct hsl_bridge *bridge;
  struct hsl_port_vlan *port_vlan;
  struct hsl_vlan_port *v, tv;
  struct hsl_avl_node *node;
  struct hsl_if *ifpvlan = NULL;
  char ifname[HAL_IFNAME_LEN];
  int no_child = 0;
  
  HSL_BRIDGE_LOCK;
  bridge = p_hsl_bridge_master->bridge;
  if (! bridge && memcmp (bridge->name, name, HAL_BRIDGE_NAME_LEN))
    {
      HSL_BRIDGE_UNLOCK;
      return HSL_ERR_BRIDGE_INVALID_PARAM;
    }

  ifp = hsl_ifmgr_lookup_by_index (ifindex);
  if (! ifp)
    {
      HSL_BRIDGE_UNLOCK;
      return HSL_ERR_BRIDGE_INVALID_PARAM;
    }

  if (ifp->type != HSL_IF_TYPE_L2_ETHERNET)
    {
      HSL_IFMGR_IF_REF_DEC (ifp);
      HSL_BRIDGE_UNLOCK;
      return HSL_ERR_BRIDGE_INVALID_PARAM;
    }

  port = ifp->u.l2_ethernet.port;
  if (! port)
    {
      HSL_IFMGR_IF_REF_DEC (ifp);
      HSL_BRIDGE_UNLOCK;
      return HSL_ERR_BRIDGE_PORT_NOT_EXISTS;
    }

  port_vlan = port->vlan;
  if (! port_vlan)
    {
      port_vlan = hsl_port_vlan_map_init ();
      if (! port_vlan)
        {
          HSL_IFMGR_IF_REF_DEC (ifp);
          HSL_BRIDGE_UNLOCK;
          return HSL_ERR_BRIDGE_PORT_NOT_EXISTS;
        }
      port_vlan->port = port;
      port->vlan = port_vlan; 
    }

  /* Set port->vlan map. */
  _hsl_port_vlan_add_update(port_vlan, vid, 
                            (egress == HAL_VLAN_EGRESS_TAGGED)?HSL_TRUE:HSL_FALSE);

  /* Set vlan->port map. */
  tv.vid = vid;
  node = hsl_avl_lookup (bridge->vlan_tree, &tv);
  if (! node)
    {
      HSL_IFMGR_IF_REF_DEC (ifp);
      HSL_BRIDGE_UNLOCK;
      return HSL_ERR_BRIDGE_VID_NOT_EXISTS;
    }

  v = (struct hsl_vlan_port *) HSL_AVL_NODE_INFO (node);
  if (! v)
    {
      HSL_IFMGR_IF_REF_DEC (ifp);
      HSL_BRIDGE_UNLOCK;
      return HSL_ERR_BRIDGE_VID_NOT_EXISTS;
    }

  /* Add this port to the tree. */
  hsl_avl_insert (v->port_tree, ifp);

  /* Add VID to port in hardware in hardware. */
  if (p_hsl_bridge_master->hw_cb && p_hsl_bridge_master->hw_cb->add_vlan_to_port)
    (*p_hsl_bridge_master->hw_cb->add_vlan_to_port) (bridge, port_vlan, vid, egress);

  /* Associate the port with vlan interface */
  memset(ifname, 0, HAL_IFNAME_LEN);
  snprintf(ifname, HAL_IFNAME_LEN, "%s%s.%d", "vlan", bridge->name, vid);
  ifpvlan = hsl_ifmgr_lookup_by_name (ifname);
  if (ifpvlan)
    {
      if (! ifpvlan->children_list)
        no_child = 1;

      ifp_parent = hsl_ifmgr_get_L2_parent(ifp);
      if (ifp_parent && (ifp_parent != ifp))
        { 
          if (hsl_ifmgr_isbound(ifpvlan, ifp_parent) != HSL_TRUE)
            hsl_ifmgr_bind2(ifpvlan, ifp_parent);
        }
      else
        if (hsl_ifmgr_isbound(ifpvlan, ifp) != HSL_TRUE)
          hsl_ifmgr_bind2(ifpvlan, ifp);
           
      hsl_ifmgr_set_acceptable_packet_types (ifp, HSL_IF_PKT_ALL);
      HSL_IFMGR_IF_REF_DEC (ifpvlan);
    }

  if (no_child && ifpvlan)
    hsl_ifmgr_send_notification (HSL_IF_EVENT_IFNEW, ifpvlan, NULL);

  HSL_LOG(HSL_LOG_GENERAL, HSL_LEVEL_INFO, " Added VID %d to interface %s\n", vid, ifp->name);

  HSL_IFMGR_IF_REF_DEC (ifp);
  HSL_BRIDGE_UNLOCK;

  return 0;
}

/*
  Delete VID from port.
*/
int
hsl_vlan_delete_vid_from_port (char *name, hsl_ifIndex_t ifindex, hsl_vid_t vid)
{
  struct hsl_if *ifp;
  struct hsl_if *ifp_parent;
  struct hsl_bridge_port *port;
  struct hsl_bridge *bridge;
  struct hsl_port_vlan *port_vlan;
  struct hsl_vlan_port *v, tv;
  struct hsl_avl_node *node;
  struct hsl_if *ifpvlan = NULL;
  char ifname[HAL_IFNAME_LEN];
  
  HSL_BRIDGE_LOCK;
  bridge = p_hsl_bridge_master->bridge;
  if (! bridge && memcmp (bridge->name, name, HAL_BRIDGE_NAME_LEN))
    {
      HSL_BRIDGE_UNLOCK;
      return HSL_ERR_BRIDGE_INVALID_PARAM;
    }

  ifp = hsl_ifmgr_lookup_by_index (ifindex);
  if (! ifp)
    {
      HSL_BRIDGE_UNLOCK;
      return HSL_ERR_BRIDGE_INVALID_PARAM;
    }

  if (ifp->type != HSL_IF_TYPE_L2_ETHERNET)
    {
      HSL_IFMGR_IF_REF_DEC (ifp);
      HSL_BRIDGE_UNLOCK;
      return HSL_ERR_BRIDGE_INVALID_PARAM;
    }

  port = ifp->u.l2_ethernet.port;
  if (! port)
    {
      HSL_IFMGR_IF_REF_DEC (ifp);
      HSL_BRIDGE_UNLOCK;
      return HSL_ERR_BRIDGE_PORT_NOT_EXISTS;
    }

  port_vlan = port->vlan;
  if (! port_vlan)
    {
      HSL_IFMGR_IF_REF_DEC (ifp);
      HSL_BRIDGE_UNLOCK;
      return HSL_ERR_BRIDGE_PORT_NOT_EXISTS;
    }
   
  /* Flush mac addresses learned from deleted port. */
  hsl_bridge_delete_port_vlan_fdb(ifp, vid);
 

  /* Delete VID from port in hardware. */
  if (p_hsl_bridge_master->hw_cb && p_hsl_bridge_master->hw_cb->delete_vlan_from_port)
    (*p_hsl_bridge_master->hw_cb->delete_vlan_from_port) (bridge, port_vlan, vid);

  /* Unset port->vlan map. */
  _hsl_port_vlan_delete(port_vlan, vid);

  /* Unset vlan->port map. */
  tv.vid = vid;
  node = hsl_avl_lookup (bridge->vlan_tree, &tv);
  if (! node)
    {
      HSL_IFMGR_IF_REF_DEC (ifp);
      HSL_BRIDGE_UNLOCK;
      return HSL_ERR_BRIDGE_VID_NOT_EXISTS;
    }

  v = (struct hsl_vlan_port *) HSL_AVL_NODE_INFO (node);
  if (! v)
    {
      HSL_IFMGR_IF_REF_DEC (ifp);
      HSL_BRIDGE_UNLOCK;
      return HSL_ERR_BRIDGE_VID_NOT_EXISTS;
    }

  /* Delete this port from the tree. */
  hsl_avl_delete (v->port_tree, ifp);
  
  /* Associate the port with vlan interface */
  memset(ifname, 0, HAL_IFNAME_LEN);
  snprintf(ifname, HAL_IFNAME_LEN, "%s%s.%d", "vlan", bridge->name, vid);
  ifpvlan = hsl_ifmgr_lookup_by_name (ifname);
  if (ifpvlan)
    {
      ifp_parent = hsl_ifmgr_get_L2_parent(ifp);
      if (ifp_parent && (ifp_parent != ifp))
        {
          if (hsl_ifmgr_isbound(ifpvlan, ifp_parent) == HSL_TRUE)
            hsl_ifmgr_unbind2(ifpvlan, ifp_parent);
        }
      else
        hsl_ifmgr_unbind2(ifpvlan, ifp);
      
      /* If no vlans associated to this port, then receive only L2 pkts */
      if (HSL_AVL_COUNT (port_vlan->vlan_tree)== 0)
        {
          hsl_ifmgr_unset_acceptable_packet_types (ifp, HSL_IF_PKT_ALL);
          hsl_ifmgr_set_acceptable_packet_types (ifp, HSL_IF_PKT_L2);
        }
#ifdef HAVE_L3
      if (! ifpvlan->children_list)
        {
          /* Unset operational status for the SVI. */
          ifpvlan->flags &= ~IFF_RUNNING;

          hsl_ifmgr_send_notification (HSL_IF_EVENT_IFNEW, ifpvlan, NULL);
        }
#endif /* HAVE_L3 */
      HSL_IFMGR_IF_REF_DEC (ifpvlan);
    } /* if ifpvlan */

  HSL_LOG(HSL_LOG_GENERAL, HSL_LEVEL_INFO, " Deleted VID %d from interface %s\n", vid, ifp->name);

  HSL_IFMGR_IF_REF_DEC (ifp);
  HSL_BRIDGE_UNLOCK;

  return 0;
}

/* Get the egress type for a specific port and vid. */
int
hsl_vlan_get_egress_type (struct hsl_if *ifp, hsl_vid_t vid)
{
  int tagged = 0;
  struct hsl_bridge_port *port;
  struct hsl_port_vlan *port_vlan;
  struct hsl_vlan_port_attr *vlan_attr;

  if (! ifp || (ifp->type != HSL_IF_TYPE_L2_ETHERNET))
    return -1;

  HSL_BRIDGE_LOCK;
  
  port = ifp->u.l2_ethernet.port;
  if (! port)
    {
      HSL_BRIDGE_UNLOCK;
      return HSL_ERR_BRIDGE_PORT_NOT_EXISTS;
    }

  port_vlan = port->vlan;
  if (! port_vlan)
    {
      HSL_BRIDGE_UNLOCK;
      return HSL_ERR_BRIDGE_PORT_NOT_EXISTS;
    }

  vlan_attr = hsl_port_vlan_get_attr(port_vlan, vid);
  if(vlan_attr)
    {
      if(HSL_TRUE == vlan_attr->etagged)
        tagged = 1;
      else 
        tagged = 0;
    }
  HSL_BRIDGE_UNLOCK;

  return tagged;
}


/*  Check if the VLAN is associated to any port. */ 
int
hsl_vlan_check_if_empty(hsl_vid_t vid)
{
  struct hsl_if *ifp = NULL;
  struct hsl_bridge *bridge;
  struct hsl_vlan_port *v, tv;
  struct hsl_avl_node *node;
  
  HSL_FN_ENTER(); 
 
  HSL_BRIDGE_LOCK;

  bridge = p_hsl_bridge_master->bridge;
  if (!bridge)
    {
      HSL_BRIDGE_UNLOCK;
      HSL_FN_EXIT(-1);
    }
      
  tv.vid = vid;
  node = hsl_avl_lookup (bridge->vlan_tree, &tv);
  if (!node)
    {
      HSL_LOG(HSL_LOG_GENERAL, HSL_LEVEL_ERROR, " Vlan %d not found bridge\n", vid);
      HSL_BRIDGE_UNLOCK;
      HSL_FN_EXIT(-1);
    }
      
  v = (struct hsl_vlan_port *)HSL_AVL_NODE_INFO (node);
  if (! v)
    {
      HSL_BRIDGE_UNLOCK;
      HSL_FN_EXIT(-1);
    }
  HSL_BRIDGE_UNLOCK;
      
  node = NULL;
  node = hsl_avl_getnext(v->port_tree, node);
  if (!node)
    {
      HSL_LOG(HSL_LOG_GENERAL, HSL_LEVEL_ERROR, " No port associated to vlan %d\n", vid);
      HSL_FN_EXIT(-1);

      ifp = (struct hsl_if *)HSL_AVL_NODE_INFO (node);
      if (!ifp)
        HSL_FN_EXIT(-1);
    }
  HSL_FN_EXIT(0);
}


/*
  Delete all vlans from a port.
*/
int
hsl_port_flush_vlans(hsl_ifIndex_t ifindex)
{
  struct hsl_if *ifp;
  struct hsl_bridge_port *port;
  struct hsl_bridge *bridge;
  struct hsl_port_vlan *port_vlan;
  struct hsl_avl_node *node;
  struct hsl_avl_node *node_next;
  struct hsl_vlan_port_attr  *p_vlan_attr;

  HSL_BRIDGE_LOCK;
  bridge = p_hsl_bridge_master->bridge;

  ifp = hsl_ifmgr_lookup_by_index (ifindex);
  if (! ifp)
  {
    HSL_BRIDGE_UNLOCK;
    return HSL_ERR_BRIDGE_INVALID_PARAM;
  }

  if (ifp->type != HSL_IF_TYPE_L2_ETHERNET)
  {
    HSL_IFMGR_IF_REF_DEC (ifp);
    HSL_BRIDGE_UNLOCK;
    return HSL_ERR_BRIDGE_INVALID_PARAM;
  }

  port = ifp->u.l2_ethernet.port;
  if (! port)
  {
    HSL_IFMGR_IF_REF_DEC (ifp);
    HSL_BRIDGE_UNLOCK;
    return HSL_ERR_BRIDGE_PORT_NOT_EXISTS;
  }

  port_vlan = port->vlan;
  if (! port_vlan)
  {
    HSL_IFMGR_IF_REF_DEC (ifp);
    HSL_BRIDGE_UNLOCK;
    return HSL_ERR_BRIDGE_PORT_NOT_EXISTS;
  } 
  /* Flush member vlan tree. */
  for (node = hsl_avl_top (port_vlan->vlan_tree); node; node = node_next)
  {
    node_next = hsl_avl_next (node);

    p_vlan_attr = (struct hsl_vlan_port_attr *) HSL_AVL_NODE_INFO (node);
    if (! p_vlan_attr)
      /* Too bad but continue */
      continue;
    hsl_vlan_delete_vid_from_port (bridge->name, ifindex, p_vlan_attr->vid);
  }
  HSL_IFMGR_IF_REF_DEC (ifp);
  HSL_BRIDGE_UNLOCK;
  return 0;
}

#ifdef HAVE_L3
/*  Enable/Disable L3 traffic on vlan ports. */ 
int
hsl_vlan_enable_l3_pkt(hsl_vid_t vid, int enable)
{
  int ret;
  struct hsl_if *ifp = NULL;
  struct hsl_bridge *bridge;
  struct hsl_vlan_port *v, tv;
  struct hsl_avl_node *node;
  
  HSL_FN_ENTER(); 
 
  HSL_BRIDGE_LOCK;

  bridge = p_hsl_bridge_master->bridge;
  if (!bridge)
    {
      HSL_BRIDGE_UNLOCK;
      HSL_FN_EXIT(-1);
    }
      
  tv.vid = vid;
  node = hsl_avl_lookup (bridge->vlan_tree, &tv);
  if (!node)
    {
      HSL_LOG(HSL_LOG_GENERAL, HSL_LEVEL_ERROR, " Vlan %d not found bridge\n", vid);
      HSL_BRIDGE_UNLOCK;
      HSL_FN_EXIT(-1);
    }
      
  v = (struct hsl_vlan_port *)HSL_AVL_NODE_INFO (node);
  if (! v)
    {
      HSL_BRIDGE_UNLOCK;
      HSL_FN_EXIT(-1);
    }
  HSL_BRIDGE_UNLOCK;
      
  node = NULL;
  while ((node = hsl_avl_getnext(v->port_tree, node)))
    {
      ifp = (struct hsl_if *)HSL_AVL_NODE_INFO (node);
      if (!ifp)
        continue;
      if(enable)
        hsl_ifmgr_set_acceptable_packet_types (ifp, HSL_IF_PKT_ALL);
      else  
        {
          ret = hsl_ifmgr_get_additional_L3_port (ifp, vid);
          if(ret == 0)
            {
              hsl_ifmgr_unset_acceptable_packet_types (ifp, HSL_IF_PKT_ALL);                        
              hsl_ifmgr_unset_acceptable_packet_types (ifp, HSL_IF_PKT_L2);
            }
        }
    }
  HSL_FN_EXIT(0);
}

/* Bind a SVI to the ports which are members of the VLAN. */
int
hsl_vlan_svi_link_port_members (struct hsl_if *ifpp, hsl_vid_t vid)
{
  struct hsl_if *ifp = NULL;
  struct hsl_bridge *bridge;
  struct hsl_vlan_port *v, tv;
  struct hsl_avl_node *node;
  
  HSL_FN_ENTER(); 
 
  HSL_BRIDGE_LOCK;

  bridge = p_hsl_bridge_master->bridge;
  if (!bridge)
    {
      HSL_BRIDGE_UNLOCK;
      HSL_FN_EXIT(-1);
    }
      
  tv.vid = vid;
  node = hsl_avl_lookup (bridge->vlan_tree, &tv);
  if (!node)
    {
      HSL_LOG(HSL_LOG_GENERAL, HSL_LEVEL_ERROR, " Vlan %d not found bridge\n", vid);
      HSL_BRIDGE_UNLOCK;
      HSL_FN_EXIT(-1);
    }
      
  v = (struct hsl_vlan_port *)HSL_AVL_NODE_INFO (node);
  if (! v)
    {
      HSL_BRIDGE_UNLOCK;
      HSL_FN_EXIT(-1);
    }
  HSL_BRIDGE_UNLOCK;
      
  for (node = hsl_avl_top (v->port_tree); node; node = hsl_avl_getnext (v->port_tree, node))
    {
      ifp = (struct hsl_if *)HSL_AVL_NODE_INFO (node);
      if (! ifp)
        continue;

      hsl_ifmgr_bind2 (ifpp, ifp);
      hsl_ifmgr_set_acceptable_packet_types (ifp, HSL_IF_PKT_ALL);
    }

  if (ifpp->children_list)
    hsl_ifmgr_send_notification (HSL_IF_EVENT_IFNEW, ifpp, NULL);

  HSL_FN_EXIT (0);
}
#endif /* HAVE_L3 */

#ifdef HAVE_VLAN_CLASS
/*  HAL_MSG_VLAN_CLASSIFIER_ADD message. */
int
hsl_vlan_classifier_add (struct hal_msg_vlan_classifier_rule *msg)
{
  int ret = 0;

  HSL_FN_ENTER();  
 
  if(!msg)
    HSL_FN_EXIT(-1);

  if (msg->type == HAL_VLAN_CLASSIFIER_MAC)
    {
      if (p_hsl_bridge_master && p_hsl_bridge_master->hw_cb->vlan_mac_classifier_add)
        ret = (*p_hsl_bridge_master->hw_cb->vlan_mac_classifier_add)(msg->vlan_id, msg->u.hw_addr,
                                                                     msg->ifindex, msg->refcount);
    }
  else if (msg->type == HAL_VLAN_CLASSIFIER_IPV4)
    {
      if (p_hsl_bridge_master && p_hsl_bridge_master->hw_cb->vlan_ipv4_classifier_add)
        ret = (*p_hsl_bridge_master->hw_cb->vlan_ipv4_classifier_add) (msg->vlan_id, msg->u.ipv4.addr, 
                                                                       msg->u.ipv4.masklen,
                                                                       msg->ifindex, msg->refcount);
    }
  else if (msg->type == HAL_VLAN_CLASSIFIER_PROTOCOL)
    {
      if (p_hsl_bridge_master && p_hsl_bridge_master->hw_cb->vlan_proto_classifier_add)
        ret = (*p_hsl_bridge_master->hw_cb->vlan_proto_classifier_add) (msg->vlan_id, msg->u.protocol.ether_type, 
                                                                        msg->u.protocol.encaps,msg->ifindex,
                                                                        msg->refcount);
    }
  HSL_FN_EXIT(ret);
}

/*  HAL_MSG_VLAN_CLASSIFIER_DELETE message. */
int
hsl_vlan_classifier_delete (struct hal_msg_vlan_classifier_rule *msg)
{
  int ret = 0;

  HSL_FN_ENTER();  

  if (msg->type == HAL_VLAN_CLASSIFIER_MAC)
    {
      if (p_hsl_bridge_master && p_hsl_bridge_master->hw_cb->vlan_mac_classifier_delete)
        ret = (*p_hsl_bridge_master->hw_cb->vlan_mac_classifier_delete) (msg->vlan_id, msg->u.hw_addr,msg->ifindex,msg->refcount);
    }
  else if (msg->type == HAL_VLAN_CLASSIFIER_IPV4)
    {
      if (p_hsl_bridge_master && p_hsl_bridge_master->hw_cb->vlan_ipv4_classifier_delete)
        ret = (*p_hsl_bridge_master->hw_cb->vlan_ipv4_classifier_delete) (msg->vlan_id, msg->u.ipv4.addr,
                                                                          msg->u.ipv4.masklen,msg->ifindex,msg->refcount);
    }
  else if (msg->type == HAL_VLAN_CLASSIFIER_PROTOCOL)
    {
      if (p_hsl_bridge_master && p_hsl_bridge_master->hw_cb->vlan_proto_classifier_delete)
        ret = (*p_hsl_bridge_master->hw_cb->vlan_proto_classifier_delete) (msg->vlan_id, msg->u.protocol.ether_type,
                                                                           msg->u.protocol.encaps,msg->ifindex,msg->refcount);
    }
  HSL_FN_EXIT(ret);
}
#endif /* HAVE_VLAN_CLASS */

#ifdef HAVE_PVLAN
static int
_hsl_pvlan_vlan_cmp (void *param1, void *param2)
{
  struct hsl_vlan_port *p1 = (struct hsl_vlan_port *)param1;
  struct hsl_vlan_port *p2 = (struct hsl_vlan_port *)param2;
  
  if (p1->vid < p2->vid)
    return -1;
  
  if (p1->vid > p2->vid)
    return 1;
  
  return 0;
}

int hsl_pvlan_set_vlan_type(char *name, hsl_vid_t vid, 
                            enum hal_pvlan_type vlan_type)
{
  struct hsl_vlan_port *v, tv;
  struct hsl_bridge *bridge;
  struct hsl_avl_node *node;
  int ret;

  HSL_BRIDGE_LOCK;
  bridge = p_hsl_bridge_master->bridge;
  if (! bridge && memcmp (bridge->name, name, HAL_BRIDGE_NAME_LEN))
    {
      HSL_BRIDGE_UNLOCK;
      return HSL_ERR_BRIDGE_INVALID_PARAM;
    }

  tv.vid = vid;
  node = hsl_avl_lookup (bridge->vlan_tree, &tv);
  if (! node)
    {
      HSL_BRIDGE_UNLOCK;
      return HSL_ERR_BRIDGE_INVALID_PARAM;
    }

  v = (struct hsl_vlan_port *) HSL_AVL_NODE_INFO (node);
  if (! v)
    {
      HSL_BRIDGE_UNLOCK;
      return HSL_ERR_BRIDGE_INVALID_PARAM;
    }

  if (v->vlan_type == vlan_type)
    {
      HSL_BRIDGE_UNLOCK;
      return 0;
    } 

  if (vlan_type == HAL_PVLAN_PRIMARY)
    {
      ret = hsl_avl_create (&v->vlan_tree, 0, _hsl_pvlan_vlan_cmp);
      if (ret < 0)
        {
          HSL_BRIDGE_UNLOCK;
          return HSL_ERR_BRIDGE_INVALID_PARAM;
        }
     }
  else
    {
      if (v->vlan_tree)
        hsl_avl_tree_free (&v->vlan_tree, NULL);
      v->vlan_tree = NULL; 
    }

  v->vlan_type = vlan_type;
  
  /* Add VLAN to hardware. */
  if (p_hsl_bridge_master->hw_cb && p_hsl_bridge_master->hw_cb->set_pvlan_vlan_type)
    (*p_hsl_bridge_master->hw_cb->set_pvlan_vlan_type) (bridge, v, vlan_type);

  HSL_BRIDGE_UNLOCK;

  return 0;
}

int hsl_pvlan_add_vlan_association(char *name, hsl_vid_t primary_vid,
                                   hsl_vid_t secondary_vid)
{
  struct hsl_bridge *bridge;
  struct hsl_vlan_port *pv, tv_prim;
  struct hsl_vlan_port *sv, tv_second;
  struct hsl_avl_node *node;
  
  HSL_BRIDGE_LOCK;
  bridge = p_hsl_bridge_master->bridge;
  if (! bridge && memcmp (bridge->name, name, HAL_BRIDGE_NAME_LEN))
    {
      HSL_BRIDGE_UNLOCK;
      return HSL_ERR_BRIDGE_INVALID_PARAM;
    }

  /* Set vlan->port map. */
  tv_prim.vid = primary_vid;
  node = hsl_avl_lookup (bridge->vlan_tree, &tv_prim);
  if (! node)
    {
      HSL_BRIDGE_UNLOCK;
      return HSL_ERR_BRIDGE_VID_NOT_EXISTS;
    }

  pv = (struct hsl_vlan_port *) HSL_AVL_NODE_INFO (node);
  if (! pv)
    {
      HSL_BRIDGE_UNLOCK;
      return HSL_ERR_BRIDGE_VID_NOT_EXISTS;
    }

  tv_second.vid = secondary_vid;
  node = hsl_avl_lookup (bridge->vlan_tree, &tv_second);
  if (! node)
    {
      HSL_BRIDGE_UNLOCK;
      return HSL_ERR_BRIDGE_VID_NOT_EXISTS;
    }

  sv = (struct hsl_vlan_port *) HSL_AVL_NODE_INFO (node);
  if (! sv)
    {
      HSL_BRIDGE_UNLOCK;
      return HSL_ERR_BRIDGE_VID_NOT_EXISTS;
    }

  if ((pv->vlan_type != HAL_PVLAN_PRIMARY) ||
      ((sv->vlan_type == HAL_PVLAN_PRIMARY) ||
       (sv->vlan_type == HAL_PVLAN_NONE)))
    {
      HSL_BRIDGE_UNLOCK;
      return HSL_ERR_BRIDGE_INVALID_PARAM;
    }
     
  /* Add this port to the tree. */
  hsl_avl_insert (pv->vlan_tree, sv);

  /* Add VID to port in hardware in hardware. */
  if (p_hsl_bridge_master->hw_cb && p_hsl_bridge_master->hw_cb->add_pvlan_vlan_association)
    (*p_hsl_bridge_master->hw_cb->add_pvlan_vlan_association) (bridge, pv, sv);

  HSL_BRIDGE_UNLOCK;

  return 0;
}

int hsl_pvlan_remove_vlan_association(char *name, hsl_vid_t primary_vid,
                                      hsl_vid_t secondary_vid)
{
  struct hsl_bridge *bridge;
  struct hsl_vlan_port *pv, tv_prim;
  struct hsl_vlan_port *sv, tv_second;
  struct hsl_avl_node *node;
  
  HSL_BRIDGE_LOCK;
  bridge = p_hsl_bridge_master->bridge;
  if (! bridge && memcmp (bridge->name, name, HAL_BRIDGE_NAME_LEN))
    {
      HSL_BRIDGE_UNLOCK;
      return HSL_ERR_BRIDGE_INVALID_PARAM;
    }

  /* Set vlan->port map. */
  tv_prim.vid = primary_vid;
  node = hsl_avl_lookup (bridge->vlan_tree, &tv_prim);
  if (! node)
    {
      HSL_BRIDGE_UNLOCK;
      return HSL_ERR_BRIDGE_VID_NOT_EXISTS;
    }

  pv = (struct hsl_vlan_port *) HSL_AVL_NODE_INFO (node);
  if (! pv)
    {
      HSL_BRIDGE_UNLOCK;
      return HSL_ERR_BRIDGE_VID_NOT_EXISTS;
    }

  tv_second.vid = secondary_vid;
  node = hsl_avl_lookup (bridge->vlan_tree, &tv_second);
  if (! node)
    {
      HSL_BRIDGE_UNLOCK;
      return HSL_ERR_BRIDGE_VID_NOT_EXISTS;
    }

  sv = (struct hsl_vlan_port *) HSL_AVL_NODE_INFO (node);
  if (! sv)
    {
      HSL_BRIDGE_UNLOCK;
      return HSL_ERR_BRIDGE_VID_NOT_EXISTS;
    }

  if ((pv->vlan_type != HAL_PVLAN_PRIMARY) ||
      ((sv->vlan_type == HAL_PVLAN_PRIMARY) ||
       (sv->vlan_type == HAL_PVLAN_NONE)))
    {
      HSL_BRIDGE_UNLOCK;
      return HSL_ERR_BRIDGE_INVALID_PARAM;
    }
     
  /* Add VID to port in hardware in hardware. */
  if (p_hsl_bridge_master->hw_cb && p_hsl_bridge_master->hw_cb->del_pvlan_vlan_association)
    (*p_hsl_bridge_master->hw_cb->del_pvlan_vlan_association) (bridge, pv, sv);

  /* delete this secondary vlan from primary vlan vlan tree. */
  hsl_avl_delete (pv->vlan_tree, sv);

  HSL_BRIDGE_UNLOCK;
  return 0;
}

HSL_BOOL hsl_pvlan_port_vlan_check(struct hsl_port_vlan *port_vlan,
                                   struct hsl_vlan_port *svlan)
{
  struct hsl_vlan_port tv;
  struct hsl_avl_node *node;
  
  if (!port_vlan || !svlan)
    return HSL_FALSE;

  if ((port_vlan->mode == HAL_PVLAN_PORT_MODE_INVALID) ||
      (port_vlan->mode == HAL_PVLAN_PORT_MODE_PROMISCUOUS))
    return HSL_FALSE;
  
  tv.vid = svlan->vid;
  node = hsl_avl_lookup (port_vlan->vlan_tree, &tv);
  if (! node)
    return HSL_FALSE;
  
  return HSL_TRUE;
}

int hsl_pvlan_port_add_association(char *name, hsl_ifIndex_t ifindex,
                                   hsl_vid_t primary_vlan,
                                   hsl_vid_t secondary_vlan)
{
  struct hsl_if *ifp;
  struct hsl_bridge_port *port;
  struct hsl_bridge *bridge;
  struct hsl_port_vlan *port_vlan;
  struct hsl_vlan_port *pv, tv_prim;
  struct hsl_vlan_port *sv, tv_second;
  struct hsl_avl_node *node;
  
  HSL_BRIDGE_LOCK;
  bridge = p_hsl_bridge_master->bridge;
  if (! bridge && memcmp (bridge->name, name, HAL_BRIDGE_NAME_LEN))
    {
      HSL_BRIDGE_UNLOCK;
      return HSL_ERR_BRIDGE_INVALID_PARAM;
    }

  ifp = hsl_ifmgr_lookup_by_index (ifindex);
  if (! ifp)
    {
      HSL_BRIDGE_UNLOCK;
      return HSL_ERR_BRIDGE_INVALID_PARAM;
    }

  if (ifp->type != HSL_IF_TYPE_L2_ETHERNET)
    {
      HSL_IFMGR_IF_REF_DEC (ifp);
      HSL_BRIDGE_UNLOCK;
      return HSL_ERR_BRIDGE_INVALID_PARAM;
    }

  port = ifp->u.l2_ethernet.port;
  if (! port)
    {
      HSL_IFMGR_IF_REF_DEC (ifp);
      HSL_BRIDGE_UNLOCK;
      return HSL_ERR_BRIDGE_PORT_NOT_EXISTS;
    }

  port_vlan = port->vlan;
  if (! port_vlan)
    {
      HSL_IFMGR_IF_REF_DEC (ifp);
      HSL_BRIDGE_UNLOCK;
      return HSL_ERR_BRIDGE_PORT_NOT_EXISTS;
    }

  /* Set vlan->port map. */
  tv_prim.vid = primary_vlan;
  node = hsl_avl_lookup (bridge->vlan_tree, &tv_prim);
  if (! node)
    {
      HSL_IFMGR_IF_REF_DEC (ifp);
      HSL_BRIDGE_UNLOCK;
      return HSL_ERR_BRIDGE_VID_NOT_EXISTS;
    }

  pv = (struct hsl_vlan_port *) HSL_AVL_NODE_INFO (node);
  if (! pv)
    {
      HSL_IFMGR_IF_REF_DEC (ifp);
      HSL_BRIDGE_UNLOCK;
      return HSL_ERR_BRIDGE_VID_NOT_EXISTS;
    }

  tv_second.vid = secondary_vlan;
  node = hsl_avl_lookup (bridge->vlan_tree, &tv_second);
  if (! node)
    {
      HSL_BRIDGE_UNLOCK;
      return HSL_ERR_BRIDGE_VID_NOT_EXISTS;
    }

  sv = (struct hsl_vlan_port *) HSL_AVL_NODE_INFO (node);
  if (! sv)
    {
      HSL_BRIDGE_UNLOCK;
      return HSL_ERR_BRIDGE_VID_NOT_EXISTS;
    }

   /* Check secondary vlan associated to primary */
  node = hsl_avl_lookup (pv->vlan_tree, &tv_second);

  if (!node)
    {
      HSL_BRIDGE_UNLOCK;
      return HSL_ERR_BRIDGE_VID_NOT_EXISTS;
    }

  if ((pv->vlan_type != HAL_PVLAN_PRIMARY) ||
      ((sv->vlan_type == HAL_PVLAN_PRIMARY) ||
       (sv->vlan_type == HAL_PVLAN_NONE))) 
    {
      HSL_BRIDGE_UNLOCK;
      return HSL_ERR_BRIDGE_INVALID_PARAM;
    }

  /* Add VID to port in hardware in hardware. */
  if (p_hsl_bridge_master->hw_cb && p_hsl_bridge_master->hw_cb->add_pvlan_port_association)
    (*p_hsl_bridge_master->hw_cb->add_pvlan_port_association) (bridge, port_vlan, pv, sv);

  HSL_IFMGR_IF_REF_DEC (ifp);
  HSL_BRIDGE_UNLOCK;

  return 0;
}

int hsl_pvlan_port_delete_association(char *name, hsl_ifIndex_t ifindex,
                                      hsl_vid_t primary_vlan,
                                      hsl_vid_t secondary_vlan)
{
  struct hsl_if *ifp;
  struct hsl_bridge_port *port;
  struct hsl_bridge *bridge;
  struct hsl_port_vlan *port_vlan;
  struct hsl_vlan_port *pv, tv_prim;
  struct hsl_vlan_port *sv, tv_second;
  struct hsl_avl_node *node;
  
  HSL_BRIDGE_LOCK;
  bridge = p_hsl_bridge_master->bridge;
  if (! bridge && memcmp (bridge->name, name, HAL_BRIDGE_NAME_LEN))
    {
      HSL_BRIDGE_UNLOCK;
      return HSL_ERR_BRIDGE_INVALID_PARAM;
    }

  ifp = hsl_ifmgr_lookup_by_index (ifindex);
  if (! ifp)
    {
      HSL_BRIDGE_UNLOCK;
      return HSL_ERR_BRIDGE_INVALID_PARAM;
    }

  if (ifp->type != HSL_IF_TYPE_L2_ETHERNET)
    {
      HSL_IFMGR_IF_REF_DEC (ifp);
      HSL_BRIDGE_UNLOCK;
      return HSL_ERR_BRIDGE_INVALID_PARAM;
    }

  port = ifp->u.l2_ethernet.port;
  if (! port)
    {
      HSL_IFMGR_IF_REF_DEC (ifp);
      HSL_BRIDGE_UNLOCK;
      return HSL_ERR_BRIDGE_PORT_NOT_EXISTS;
    }

  port_vlan = port->vlan;
  if (! port_vlan)
    {
      HSL_IFMGR_IF_REF_DEC (ifp);
      HSL_BRIDGE_UNLOCK;
      return HSL_ERR_BRIDGE_PORT_NOT_EXISTS;
    }

  /* Set vlan->port map. */
  tv_prim.vid = primary_vlan;
  node = hsl_avl_lookup (bridge->vlan_tree, &tv_prim);
  if (! node)
    {
      HSL_IFMGR_IF_REF_DEC (ifp);
      HSL_BRIDGE_UNLOCK;
      return HSL_ERR_BRIDGE_VID_NOT_EXISTS;
    }

  pv = (struct hsl_vlan_port *) HSL_AVL_NODE_INFO (node);
  if (! pv)
    {
      HSL_IFMGR_IF_REF_DEC (ifp);
      HSL_BRIDGE_UNLOCK;
      return HSL_ERR_BRIDGE_VID_NOT_EXISTS;
    }

  tv_second.vid = secondary_vlan;
  node = hsl_avl_lookup (bridge->vlan_tree, &tv_second);
  if (! node)
    {
      HSL_BRIDGE_UNLOCK;
      return HSL_ERR_BRIDGE_VID_NOT_EXISTS;
    }

  sv = (struct hsl_vlan_port *) HSL_AVL_NODE_INFO (node);
  if (! sv)
    {
      HSL_BRIDGE_UNLOCK;
      return HSL_ERR_BRIDGE_VID_NOT_EXISTS;
    }

  if ((pv->vlan_type != HAL_PVLAN_PRIMARY) ||
      ((sv->vlan_type == HAL_PVLAN_PRIMARY) ||
       (sv->vlan_type == HAL_PVLAN_NONE)))
    {
      HSL_BRIDGE_UNLOCK;
      return HSL_ERR_BRIDGE_INVALID_PARAM;
    }

  /* Add VID to port in hardware in hardware. */
  if (p_hsl_bridge_master->hw_cb && p_hsl_bridge_master->hw_cb->del_pvlan_port_association)
    (*p_hsl_bridge_master->hw_cb->del_pvlan_port_association) (bridge, port_vlan, pv, sv);

  HSL_IFMGR_IF_REF_DEC (ifp);
  HSL_BRIDGE_UNLOCK;

  return 0;

}

int hsl_pvlan_set_port_mode(char *name, hsl_ifIndex_t ifindex,
                            enum hal_pvlan_port_mode port_mode)
{
  struct hsl_if *ifp;
  struct hsl_bridge_port *port;
  struct hsl_bridge *bridge;

  HSL_BRIDGE_LOCK;
  bridge = p_hsl_bridge_master->bridge;
  if (! bridge && memcmp (bridge->name, name, HAL_BRIDGE_NAME_LEN))
    {
      HSL_BRIDGE_UNLOCK;
      return HSL_ERR_BRIDGE_INVALID_PARAM;
    }

  ifp = hsl_ifmgr_lookup_by_index (ifindex);
  if (! ifp)
    {
      HSL_BRIDGE_UNLOCK;
      return HSL_ERR_BRIDGE_INVALID_PARAM;
    }

  if (ifp->type != HSL_IF_TYPE_L2_ETHERNET)
    {
      HSL_IFMGR_IF_REF_DEC (ifp);
      HSL_BRIDGE_UNLOCK;
      return HSL_ERR_BRIDGE_INVALID_PARAM;
    }

  port = ifp->u.l2_ethernet.port;
  if (! port)
    {
      HSL_IFMGR_IF_REF_DEC (ifp);
      HSL_BRIDGE_UNLOCK;
      return HSL_ERR_BRIDGE_PORT_NOT_EXISTS;
    }

  /* Set acceptable frame type in hardware. */
  if (p_hsl_bridge_master->hw_cb && p_hsl_bridge_master->hw_cb->set_pvlan_port_mode)
    (*p_hsl_bridge_master->hw_cb->set_pvlan_port_mode) (p_hsl_bridge_master->bridge, port, port_mode);

  HSL_IFMGR_IF_REF_DEC (ifp);
  HSL_BRIDGE_UNLOCK;
  
  return 0;
}
#endif /* HAVE_PVLAN */

#endif /*HAVE_VLAN */
