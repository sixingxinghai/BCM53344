/* Copyright 2003  All Rights Reserved. */

#include "pal.h"
#include "lib.h"
#include "avl_tree.h"
#include "memory.h"

#include "nsm_interface.h"
#include "nsm_vlan.h"

#include "hal_incl.h"
#include "table.h"

#include "nsm_bridge_cli.h"
#include "nsm_vlan_cli.h"

#include "garp_gid.h"
#include "garp_gip.h"
#include "garp_debug.h"
#include "garp_pdu.h"
#include "gmrp_api.h"
#include "gmrp.h"
#include "gmrp_cli.h"
#include "gmrp_pdu.h"
#include "gmrp_debug.h"
#include "gmrp_database.h"

#include "hal_gmrp.h"
#include "garp_sock.h"

struct lib_globals *gmrpm;

/* Forward Declarations of functions */
static bool_t
gmrp_reg_vlan_listener(struct nsm_bridge *bridge, struct gmrp_bridge *new_gmrp_bridge );

static void
gmrp_dereg_vlan_listener(struct nsm_bridge *bridge, struct gmrp_bridge *gmrp_bridge);

static bool_t
gmrp_reg_bridge_listener(struct nsm_bridge *bridge, struct gmrp_bridge *gmrp_bridge);
                                                                          
static void
gmrp_dereg_bridge_listener(struct nsm_bridge *bridge, struct gmrp_bridge *gmrp_bridge);
/* End forward declarations of functions */

void
gmrp_init (struct lib_globals *zg)
{
  gmrpm = zg;

  /* CLI intialization. */
  gmrp_cli_init (zg);

  /* Debugging initialization. */
  gmrp_debug_init ();
}

s_int32_t 
gmrp_add_vlan_cb (struct nsm_bridge_master *master,
                  char *bridge_name, u_int16_t vlanid)
{
  struct gmrp *gmrp;
  struct nsm_bridge *bridge;
  struct gmrp_bridge *gmrp_bridge = NULL;
  struct avl_tree *gmrp_list = NULL;

  /* Bridge does not exist, Global GMRP exist */
  if ( (!(bridge = nsm_lookup_bridge_by_name(master, bridge_name))) ||
       nsm_bridge_is_igmp_snoop_enabled(master, bridge_name) || 
       (!(gmrp_bridge = bridge->gmrp_bridge)) ||
       (!(gmrp_list = bridge->gmrp_list)) )
  {
    return NSM_BRIDGE_ERR_GENERAL;
  }

  if ( (!gmrp_bridge->globally_enabled) &&
       (vlanid != NSM_VLAN_DEFAULT_VID) )
    return RESULT_OK;

#ifdef HAVE_PROVIDER_BRIDGE
  if (bridge->provider_edge == PAL_TRUE)
    return RESULT_OK;
#endif
  
  if ( !gmrp_create_gmrp_instance (gmrp_list, gmrp_bridge, vlanid,
                                   vlanid, &gmrp))
    {
       return NSM_BRIDGE_ERR_GENERAL;
    }

  return 0;
}

s_int32_t
gmrp_delete_vlan_cb (struct nsm_bridge_master *master,
                     char *bridge_name, u_int16_t vlanid)
{
  struct gmrp_bridge *gmrp_bridge;
  struct gmrp        *old_gmrp;
  struct avl_tree    *gmrp_list;
  struct avl_node  *node;
  struct nsm_bridge  *bridge;

  /* Bridge does not exist, Global GMRP exist */
  if ( (!(bridge = nsm_lookup_bridge_by_name(master, bridge_name))) ||
       nsm_bridge_is_igmp_snoop_enabled(master, bridge_name) || 
      (!(gmrp_bridge = bridge->gmrp_bridge)) ||
      (!(gmrp_list   = bridge->gmrp_list)) )

  {
    return NSM_BRIDGE_ERR_GENERAL;
  }

#ifdef HAVE_PROVIDER_BRIDGE
  if (bridge->provider_edge == PAL_TRUE)
    return RESULT_OK;
#endif

  old_gmrp = gmrp_get_by_vid (gmrp_list, vlanid, vlanid, &node);

  if (old_gmrp)
    {
      /* gmrp_destroy_ports_by_vlan (bridge, vlanid); */
      gmrp_destroy_gmrp_instance (old_gmrp);
    }

  return 0;
}

s_int32_t
gmrp_add_vlan_to_port_cb (struct nsm_bridge_master *master, char *bridge_name, 
                          struct interface *ifp, u_int16_t vid)
{
  s_int32_t ret;
  struct nsm_if *zif;
  struct nsm_bridge *bridge;
  struct avl_node  *node = NULL;
  struct nsm_bridge_port *br_port;
  struct gmrp_bridge *gmrp_bridge = NULL;
  struct gmrp_port   *gmrp_port = NULL;
  struct avl_tree    *gmrp_list = NULL;

  bridge = nsm_lookup_bridge_by_name (master, bridge_name);
  if (! bridge)
    return RESULT_ERROR;

#ifdef HAVE_PROVIDER_BRIDGE
  if (bridge->provider_edge == PAL_TRUE)
    return RESULT_OK;
#endif

  zif = (struct nsm_if *)ifp->info;

  if (! zif)
    return RESULT_ERROR;

  br_port = zif->switchport;

  if (! br_port)
    return RESULT_ERROR;

  gmrp_port = br_port->gmrp_port;
  gmrp_bridge = bridge->gmrp_bridge;
  gmrp_list  = bridge->gmrp_list;

  if (!gmrp_port || !gmrp_bridge )
    return RESULT_ERROR;

  /* If GMRP is enabled for a particular vlan for the
   * port and the current VID is not DEFAULT vlan id
   * return.
   */

  if ( (!gmrp_port->globally_enabled)
      && (vid != NSM_VLAN_DEFAULT_VID) )
    return RESULT_OK;

  /* If the port has GMRP globally enabled and Bridge does
   * not have GMRP globally enabled then check whether vlan
   * context for this is enabled and if it is not enabled
   * then  return.
   */

  if ( (!gmrp_bridge->globally_enabled) &&
       (!gmrp_get_by_vid (gmrp_list, vid, vid, &node)))
    return RESULT_OK;

  ret = gmrp_add_vlan_to_port (bridge, ifp, vid, vid);

  if (ret < 0)
    return RESULT_ERROR;

  return 0;
}

s_int32_t
gmrp_delete_vlan_to_port_cb (struct nsm_bridge_master *master, char *bridge_name,
                             struct interface *ifp, u_int16_t vid)
{
  struct nsm_bridge *bridge;
  struct nsm_if *zif;
  s_int32_t ret;

  bridge = nsm_lookup_bridge_by_name (master, bridge_name);
  if (! bridge)
    return RESULT_ERROR;

  zif = (struct nsm_if *)ifp->info;
  if (! zif)
    return RESULT_ERROR;

#ifdef HAVE_PROVIDER_BRIDGE
  if (bridge->provider_edge == PAL_TRUE)
    return RESULT_OK;
#endif

  ret = gmrp_remove_vlan_from_port (bridge, zif, vid, vid);

  if (ret < 0)
    return RESULT_ERROR;

  return 0;
}

#ifdef HAVE_PROVIDER_BRIDGE

s_int32_t 
gmrp_add_swtcx_cb (struct nsm_bridge_master *master,
                   char *bridge_name, u_int16_t vlanid,
                   u_int16_t svlanid)
{
  struct gmrp *gmrp;
  struct nsm_bridge *bridge;
  struct gmrp_bridge *gmrp_bridge = NULL;
  struct avl_tree *gmrp_list = NULL;

  /* Bridge does not exist, Global GMRP exist */
  if ( (!(bridge = nsm_lookup_bridge_by_name(master, bridge_name))) ||
       nsm_bridge_is_igmp_snoop_enabled(master, bridge_name) || 
       (!(gmrp_bridge = bridge->gmrp_bridge)) ||
       (!(gmrp_list = bridge->gmrp_list)) )
  {
    return NSM_BRIDGE_ERR_GENERAL;
  }

  if ( (!gmrp_bridge->globally_enabled) &&
       (vlanid != NSM_VLAN_DEFAULT_VID) )
    return RESULT_OK;
  
  if ( !gmrp_create_gmrp_instance (gmrp_list, gmrp_bridge, vlanid,
                                   svlanid, &gmrp))
    {
       return NSM_BRIDGE_ERR_GENERAL;
    }

  return 0;
}

s_int32_t
gmrp_del_swtcx_cb (struct nsm_bridge_master *master,
                   char *bridge_name, u_int16_t vlanid,
                   u_int16_t svlanid)
{
  struct gmrp_bridge *gmrp_bridge;
  struct gmrp        *old_gmrp;
  struct avl_tree    *gmrp_list;
  struct avl_node  *node;
  struct nsm_bridge  *bridge;

  /* Bridge does not exist, Global GMRP exist */
  if ( (!(bridge = nsm_lookup_bridge_by_name(master, bridge_name))) ||
       nsm_bridge_is_igmp_snoop_enabled(master, bridge_name) || 
      (!(gmrp_bridge = bridge->gmrp_bridge)) ||
      (!(gmrp_list   = bridge->gmrp_list)) )

  {
    return NSM_BRIDGE_ERR_GENERAL;
  }

  old_gmrp = gmrp_get_by_vid (gmrp_list, vlanid, svlanid, &node);

  if (old_gmrp)
    {
      /* gmrp_destroy_ports_by_vlan (bridge, vlanid); */
      gmrp_destroy_gmrp_instance (old_gmrp);
    }

  return 0;
}

s_int32_t
gmrp_add_swtcx_to_port_cb (struct nsm_bridge_master *master, char *bridge_name, 
                           struct interface *ifp, u_int16_t vid, u_int16_t svid)
{
  s_int32_t ret;
  struct nsm_if *zif;
  struct nsm_bridge *bridge;
  struct avl_node  *node = NULL;
  struct nsm_bridge_port *br_port;
  struct gmrp_bridge *gmrp_bridge = NULL;
  struct gmrp_port   *gmrp_port = NULL;
  struct avl_tree    *gmrp_list = NULL;

  bridge = nsm_lookup_bridge_by_name (master, bridge_name);
  if (! bridge)
    return RESULT_ERROR;

  zif = (struct nsm_if *)ifp->info;

  if (! zif)
    return RESULT_ERROR;

  br_port = zif->switchport;

  if (! br_port)
    return RESULT_ERROR;

  gmrp_port = br_port->gmrp_port;
  gmrp_bridge = bridge->gmrp_bridge;
  gmrp_list  = bridge->gmrp_list;

  if (!gmrp_port || !gmrp_bridge )
    return RESULT_ERROR;

  /* If GMRP is enabled for a particular vlan for the
   * port and the current VID is not DEFAULT vlan id
   * return.
   */

  if ( (!gmrp_port->globally_enabled)
      && (vid != NSM_VLAN_DEFAULT_VID) )
    return RESULT_OK;

  /* If the port has GMRP globally enabled and Bridge does
   * not have GMRP globally enabled then check whether vlan
   * context for this is enabled and if it is not enabled
   * then  return.
   */

  if ( (!gmrp_bridge->globally_enabled) &&
       (!gmrp_get_by_vid (gmrp_list, vid, svid, &node)))
    return RESULT_OK;

  ret = gmrp_add_vlan_to_port (bridge, ifp, vid, svid);

  if (ret < 0)
    return RESULT_ERROR;

  return 0;
}

s_int32_t
gmrp_del_swtcx_from_port_cb (struct nsm_bridge_master *master, char *bridge_name,
                             struct interface *ifp, u_int16_t vid, u_int16_t svid)
{
  struct nsm_bridge *bridge;
  struct nsm_if *zif;
  s_int32_t ret;

  bridge = nsm_lookup_bridge_by_name (master, bridge_name);
  if (! bridge)
    return RESULT_ERROR;

  zif = (struct nsm_if *)ifp->info;
  if (! zif)
    return RESULT_ERROR;

  ret = gmrp_remove_vlan_from_port (bridge, zif, vid, svid);

  if (ret < 0)
    return RESULT_ERROR;

  return 0;
}

#endif /* HAVE_PROVIDER_BRIDGE */

bool_t
gmrp_create_port_instance (struct avl_tree *tree, struct gid_port *gid_port,
                           struct gmrp *gmrp,
                           struct gmrp_port_instance **gmrp_port_instance)
{
  s_int32_t ret;
  struct avl_node *an = NULL;
  struct gmrp_port_instance *new_port_instance = NULL;


  new_port_instance = gmrp_port_instance_get_by_vid (tree, gmrp->vlanid,
                                                     gmrp->svlanid,
                                                     &an);

  if (new_port_instance != NULL)
    {
      *gmrp_port_instance = new_port_instance;
       return PAL_TRUE;
    }

  new_port_instance =  XCALLOC (MTYPE_GMRP_PORT_INSTANCE,
                                sizeof (struct gmrp_port_instance));

  if (!new_port_instance)
    {
      return PAL_FALSE;
    }

  new_port_instance->vlanid = gmrp->vlanid;
  new_port_instance->svlanid = gmrp->svlanid;

  if (!gid_create_gid (gid_port, &gmrp->garp_instance, &new_port_instance->gid,
                       GMRP_NUMBER_OF_LEGACY_CONTROL))
    {
      XFREE (MTYPE_GMRP_PORT_INSTANCE, new_port_instance);
      return PAL_FALSE;
    }

  ret = avl_insert (tree, (void *) new_port_instance);

  if ( ret < 0 )
    {
      gid_destroy_gid (new_port_instance->gid);
      XFREE (MTYPE_GMRP_PORT_INSTANCE, new_port_instance);
      return PAL_FALSE;
    }

  new_port_instance->tree = tree;

  *gmrp_port_instance = new_port_instance;

  return PAL_TRUE;
}

bool_t
gmrp_create_port (struct interface *ifp, char globally_enabled,
                  struct gmrp_port **gmrp_port)
{

  struct gmrp_port *new_port = NULL;

  new_port =  XCALLOC (MTYPE_GMRP_PORT, sizeof (struct gmrp_port));

  if (!new_port)
  {
    return PAL_FALSE;
  }

  pal_mem_set (new_port, 0, sizeof (struct gmrp_port));
  new_port->port = ifp;
  new_port->registration_cfg = GID_REG_MGMT_NORMAL;
  new_port->globally_enabled = globally_enabled;

  if (GMRP_DEBUG(EVENT))
  {
    zlog_debug (gmrpm, "gmrp_create_port: [port %d] Creating gmrp port", 
        ifp->ifindex);
  }

  if (!gid_create_gid_port (ifp->ifindex, &new_port->gid_port))
  {
    XFREE (MTYPE_GMRP_PORT, new_port);
    return PAL_FALSE;
  }

  *gmrp_port = new_port; 

  return PAL_TRUE;
}

bool_t
gmrp_create_vlan (struct gmrp_bridge *gmrp_bridge)
{
  struct gmrp_vlan_table *vlan_table;

  vlan_table =  XCALLOC (MTYPE_GMRP_VLAN, sizeof (struct gmrp_vlan_table));

  if (!vlan_table)
  {
    return PAL_FALSE;
  }

  pal_mem_set (vlan_table, 0, sizeof (struct gmrp_vlan_table));

  gmrp_bridge->gm_vlan_table = vlan_table;

  return PAL_TRUE;
}

void
gmrp_free_port_instance (struct gmrp_port_instance *old_gmrp_port_instance)
{
  if (!old_gmrp_port_instance)
    return;

  gid_destroy_gid (old_gmrp_port_instance->gid);

  XFREE (MTYPE_GMRP_PORT_INSTANCE, old_gmrp_port_instance);

}

void
gmrp_destroy_port_instance (struct gmrp_port_instance *old_gmrp_port_instance)
{
  if (!old_gmrp_port_instance)
    return;

  gid_destroy_gid (old_gmrp_port_instance->gid);

  avl_remove (old_gmrp_port_instance->tree, old_gmrp_port_instance);

  XFREE (MTYPE_GMRP_PORT_INSTANCE, old_gmrp_port_instance);

}

void
gmrp_destroy_port (struct gmrp_port *old_gmrp_port)
{
  if (!old_gmrp_port)
    return;
  XFREE (MTYPE_GARP_GID_PORT, old_gmrp_port->gid_port);
  XFREE (MTYPE_GMRP_PORT, old_gmrp_port);
}

static inline void
gmrp_destroy_ports (struct nsm_bridge *bridge, struct gmrp *gmrp)
{
  struct avl_node *avl_port;
  struct avl_node *node = NULL;
  struct nsm_bridge_port *br_port;
  struct avl_tree *gmrp_port_list = NULL;
  struct gmrp_port_instance *old_gmrp_port_instance = NULL;

  if (bridge == NULL)
    return;

  /* Loop through the port list and create a gid per port */
  for (avl_port = avl_top (bridge->port_tree); avl_port;
       avl_port = avl_next (avl_port))
    {
      if ((br_port = (struct nsm_bridge_port *)
                             AVL_NODE_INFO (avl_port)) == NULL)
        continue;

      gmrp_port_list = br_port->gmrp_port_list;

      if (gmrp_port_list)
        {
          old_gmrp_port_instance  = gmrp_port_instance_get_by_vid 
                                                       (gmrp_port_list, 
                                                        gmrp->vlanid,
                                                        gmrp->svlanid,
                                                        &node);
         if (old_gmrp_port_instance)
           gmrp_destroy_port_instance (old_gmrp_port_instance);
        }
    }
}

bool_t
gmrp_create_gmrp_instance (struct avl_tree *tree,
                           struct gmrp_bridge *gmrp_bridge, 
                           u_int16_t vlanid, 
                           u_int16_t svlanid, struct gmrp **gmrp)
{
  s_int32_t ret;
  struct gmrp *new_gmrp = 0;
  struct avl_node *an = NULL;

  if (!gmrp_bridge)
    return PAL_FALSE;


  new_gmrp = gmrp_get_by_vid (tree, vlanid, svlanid, &an);

  if (new_gmrp)
    {
      *gmrp = new_gmrp;
      return PAL_TRUE;
    }

  new_gmrp = XCALLOC (MTYPE_GMRP, sizeof (struct gmrp));

  if (!new_gmrp)
    {
      if (GMRP_DEBUG(EVENT))
        zlog_debug (gmrpm, "gmrp_create_gmr:Unable to create "
                    "new gmrp for vlan %d", vlanid);
      return PAL_FALSE;
    }

  new_gmrp->gmrp_bridge = gmrp_bridge;
  new_gmrp->garp_instance.application = (struct garp *)new_gmrp; 
  new_gmrp->garp_instance.garp = &gmrp_bridge->garp;
  new_gmrp->vlanid = vlanid;
  new_gmrp->svlanid = svlanid;

  ret = avl_insert (tree, new_gmrp);

  if (ret < 0)
    {
      if (GMRP_DEBUG(EVENT))
        zlog_debug (gmrpm, "gmrp_create_gmr: Unable to create new gmrp "
                    "for vlan %d", vlanid);

      XFREE (MTYPE_GMRP, new_gmrp);
      return PAL_FALSE;
    }

  if (!gip_create_gip (&new_gmrp->garp_instance))
    {
      if (GMRP_DEBUG(EVENT))
        zlog_debug (gmrpm, "gmrp_create_gmr: Unable to create new gmrp "
                    "for vlan %d", vlanid);
      avl_remove (tree, new_gmrp);
      XFREE (MTYPE_GMRP, new_gmrp);
      return PAL_FALSE;
    }

  if (!gmd_create_gmd (&new_gmrp->gmd))
  {
    if (GMRP_DEBUG(EVENT))
    {
      zlog_debug (gmrpm, "gmrp_create_gmr: Unable to create new gmd "
          "for vlan %d", vlanid);
    }

    gip_destroy_gip (&new_gmrp->garp_instance);
    XFREE (MTYPE_GMRP, new_gmrp);
    return PAL_FALSE;
  }

  pal_mem_set (new_gmrp->garp_instance.receive_counters, 0,
      (GARP_ATTR_EVENT_MAX + 1) * sizeof (u_int32_t));


  pal_mem_set (new_gmrp->garp_instance.transmit_counters, 0,
      (GARP_ATTR_EVENT_MAX + 1) * sizeof (u_int32_t));


  new_gmrp->tree = tree;

  return PAL_TRUE;
}

void
gmrp_free_gmrp_instance (struct gmrp *gmrp)
{

  struct gmrp_bridge *gmrp_bridge = NULL;

  if (gmrp == NULL)
    return;

  gmrp_bridge = gmrp->gmrp_bridge;

  if (gmrp_bridge == NULL)
    return;

  gmrp_destroy_ports (gmrp_bridge->bridge, gmrp);
  gmd_destroy_gmd (gmrp->gmd);
  gip_destroy_gip (&gmrp->garp_instance);

  XFREE(MTYPE_GMRP, gmrp);
}


void
gmrp_destroy_gmrp_instance (struct gmrp *gmrp)
{
  struct gmrp_bridge *gmrp_bridge = NULL;

  if (gmrp == NULL)
    return;

  gmrp_bridge = gmrp->gmrp_bridge;

  if (gmrp_bridge == NULL)
    return;

  gmrp_destroy_ports (gmrp_bridge->bridge, gmrp);
  gmd_destroy_gmd (gmrp->gmd);
  gip_destroy_gip (&gmrp->garp_instance);

  avl_remove (gmrp->tree, gmrp); 
  XFREE(MTYPE_GMRP, gmrp);
}

bool_t
gmrp_create_gmrp_bridge (struct nsm_bridge *bridge, char globally_enabled,
                         char *protocol, struct gmrp_bridge **gmrp_bridge )
{
  struct gmrp_bridge *new_gmrp_bridge = 0;

  /* Assign the global variable (lib_globals for gmrp) to be sanme as that of
     the bridge (rstp, stp, mstp) */
  new_gmrp_bridge = XCALLOC (MTYPE_GMRP_BRIDGE, sizeof (struct gmrp_bridge));

  if (!new_gmrp_bridge)
    {
      if (GMRP_DEBUG(EVENT))
        {
          zlog_debug (gmrpm, "gmrp_create_gmrp_bridge :Unable to create new gmrp");
        }

      return PAL_FALSE;
    }

  /* Create and Initialise the gmrp listener for l2 vlan info
     with proper values */
  if ( !gmrp_reg_vlan_listener(bridge, new_gmrp_bridge) ) {
    if (GMRP_DEBUG(EVENT)) {
      zlog_debug (gmrpm, "gmrp_create_gmrp: Unable to create new gmrp ");
    }
    XFREE (MTYPE_GMRP_BRIDGE, new_gmrp_bridge);
    return PAL_FALSE;
  }

  /* Create and Initialise the gmrp listener for nsm bridge info
     with proper values */
  if ( !gmrp_reg_bridge_listener(bridge, new_gmrp_bridge) ) {
    if (GMRP_DEBUG(EVENT)) {
      zlog_debug (gmrpm, "gmrp_create_gmrp: Unable to create new gmrp "
          "bridge listener");
    }  
    gmrp_dereg_vlan_listener(bridge, new_gmrp_bridge);
    XFREE (MTYPE_GMRP_BRIDGE, new_gmrp_bridge);
    return PAL_FALSE;
  }

  XMRP_UPDATE_PROTOCOL (new_gmrp_bridge, protocol);

  new_gmrp_bridge->garp.join_indication_func = gmrp_join_indication;
  new_gmrp_bridge->garp.leave_indication_func = gmrp_leave_indication;
  new_gmrp_bridge->garp.join_propagated_func = gmrp_join_propagated;
  new_gmrp_bridge->garp.leave_propagated_func = gmrp_leave_propagated;
  new_gmrp_bridge->garp.get_bridge_instance_func  = (void *)gmrp_get_bridge_instance;
  new_gmrp_bridge->garp.transmit_func = gmrp_transmit_pdu;
  new_gmrp_bridge->garp.get_gid_func = (struct gid* (*)(const void *, uint16_t, uint16_t))
                                                        gmrp_get_gid;
  new_gmrp_bridge->garp.get_vid_func = gmrp_get_vid;
  new_gmrp_bridge->garp.get_svid_func = gmrp_get_svid;
  new_gmrp_bridge->garp.garpm = gmrpm;
  new_gmrp_bridge->bridge = bridge;
  new_gmrp_bridge->globally_enabled = globally_enabled;

  *gmrp_bridge = new_gmrp_bridge;

  return PAL_TRUE;
}

void
gmrp_destroy_gmrp_bridge (struct nsm_bridge *bridge, struct gmrp_bridge *gmrp_bridge)
{

  if ( (gmrp_bridge == NULL) || (bridge == NULL) )
    return;

  gmrp_dereg_vlan_listener(bridge, gmrp_bridge);
  gmrp_dereg_bridge_listener(bridge, gmrp_bridge);

  XFREE(MTYPE_GMRP_BRIDGE, gmrp_bridge);
}

void
gmrp_join_indication (void *application, struct gid *gid, 
                      u_int32_t joining_attribute_index)
{
  s_int32_t ret;
  struct gmrp *gmrp;
  struct nsm_if *zif;
  struct gmrp_gmd *gmd;
  struct interface *ifp;
  struct ptree_node *rn;
  u_int32_t attribute_index;
  u_char mac_addr[HAL_HW_LENGTH];
  struct gid_port *gid_port = NULL;
  struct gmrp_bridge *gmrp_bridge = NULL;
  struct gmrp_attr_entry_tbd *attr_entry_tbd;
  struct gmrp_port *gmrp_port = NULL;
  
  gmrp = (struct gmrp*)application;
 
  if ((!gid) || (!(gid_port = gid->gid_port)) ||
      (!gmrp) || (!(gmrp_bridge = gmrp->gmrp_bridge)))
    return;

  ifp = nsm_lookup_port_by_index(gmrp_bridge->bridge, gid_port->ifindex);

  if (!ifp)
    return;

  zif = (struct nsm_if *)ifp->info;

  gmrp_port = zif->switchport->gmrp_port; 

  if (GMRP_DEBUG(EVENT))
  {
    zlog_debug (gmrpm, "gmrp_join_indication: [port %d] Recieved a join "
        "indication for attribute_index %u", gid_port->ifindex, 
        joining_attribute_index);
  }

  if ((joining_attribute_index == GMRP_LEGACY_FORWARD_ALL) ||
      (joining_attribute_index == GMRP_LEGACY_FORWARD_UNREGISTERED))
    {

       if (joining_attribute_index == GMRP_LEGACY_FORWARD_ALL)
         ret =  hal_l2_gmrp_set_service_requirement
                       (gmrp_bridge->bridge->name, gmrp->vlanid, ifp->ifindex,
                        HAL_GMRP_LEGACY_FORWARD_ALL, PAL_TRUE);
       else
         ret = hal_l2_gmrp_set_service_requirement
                      (gmrp_bridge->bridge->name, gmrp->vlanid, ifp->ifindex, 
                       HAL_GMRP_LEGACY_FORWARD_UNREGISTERED, PAL_TRUE);

       gmd = gmrp->gmd;

      if (!gmd)
        return;

      if (ret == NOT_IMPLEMENTED)
        {
          for (rn = ptree_top (gmd->gmd_entry_table); rn;
               rn = ptree_next (rn))
            {
              attribute_index = gid_get_attr_index (rn);

              if (!gid_registered_here (gid, attribute_index))
                {
                  if (joining_attribute_index == GMRP_LEGACY_FORWARD_ALL)
                    {
                      gmd_get_mac_addr (gmrp->gmd, attribute_index, mac_addr);

                      if (GMRP_DEBUG(EVENT))
                        zlog_debug (gmrpm, "gmrp_join_indication: [port %d] "
                                    "Inserting dynamic entry into the "
                                    "forwarding plane FDB mac_addr "
                                    "%2hx:%2hx:%2hx:%2hx:%2hx:%2hx vlan %u",
                                    gid_port->ifindex, mac_addr[0], mac_addr[1],
                                    mac_addr[2], mac_addr[3], mac_addr[4], 
                                    mac_addr[5], gmrp->vlanid);

                      nsm_bridge_add_mac (gmrp_bridge->bridge->master,
                                          gmrp_bridge->bridge->name,
                                          ifp, mac_addr, gmrp->vlanid,
                                          gmrp->svlanid, HAL_L2_FDB_STATIC,
                                          PAL_TRUE, GMRP_MAC);
                    }
                } /* end of gid_registered_here */

            } /* end of while */
    
        } /* End of NOT IMPLEMENTED */

    if (GMRP_DEBUG(EVENT))
      zlog_debug (gmrpm, "gmrp_join_indication: [port %d] Setting the "
                  "port's fwd_all flag in the forwarding plane FDB",
                  gid_port->ifindex);

  } /* end of if (FORWARD_ALL || FORWARD_UNREGISTERED */
  else /* Multicast attibute */
    {
      gmd_get_mac_addr (gmrp->gmd, joining_attribute_index, mac_addr);

      if (GMRP_DEBUG(EVENT))
        {
          zlog_debug (gmrpm, "gmrp_join_indication: [port %d] Inserting a "
                      "dynamic entry into the forwarding plane FDB mac_addr "
                      "%2hx:%2hx:%2hx:%2hx:%2hx:%2hx vlan %u",
                      gid_port->ifindex,
                      mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3],
                      mac_addr[4], mac_addr[5], gmrp->vlanid);
        }

      if ( gid_registered_here(gid, GMRP_LEGACY_FORWARD_ALL) )
        {
          nsm_bridge_add_mac (gmrp_bridge->bridge->master,
                              gmrp_bridge->bridge->name,
                              ifp, mac_addr, gmrp->vlanid, gmrp->svlanid,
                              HAL_L2_FDB_STATIC, PAL_TRUE, GMRP_MAC);
        }
      else
        {

          if (gmrp_port->registration_cfg == GID_REG_MGMT_RESTRICTED_GROUP)
            {
              /* If port is in Restricted Group registration mode.
               * add the MAC only if there is a static MAC configured
               * through CLI for the MAC/ VLAN combination.
               */ 
              if (nsm_is_mac_present (gmrp_bridge->bridge, mac_addr,
                                       gmrp->vlanid, gmrp->svlanid,
                                       CLI_MAC, NSM_INVALID_INTERFACE_INDEX))
                {
                  nsm_bridge_add_mac (gmrp_bridge->bridge->master,
                                      gmrp_bridge->bridge->name,
                                      ifp, mac_addr, gmrp->vlanid, gmrp->svlanid,
                                      HAL_L2_FDB_STATIC,PAL_TRUE, GMRP_MAC);
                }
              else
                {
                  gmrp_port->gmrp_failed_registrations++;
                }
            }
          else
            {
              /* If a static mac has been through CLI configured for the
               * MAC/ VLAN combination. Do not add the filtering entry.
               */ 

              if (!nsm_is_mac_present (gmrp_bridge->bridge, mac_addr,
                                       gmrp->vlanid, gmrp->svlanid,
                                       CLI_MAC, NSM_INVALID_INTERFACE_INDEX))
                {
                  nsm_bridge_add_mac (gmrp_bridge->bridge->master,
                                      gmrp_bridge->bridge->name,
                                      ifp, mac_addr, gmrp->vlanid, gmrp->svlanid,
                                      HAL_L2_FDB_STATIC,PAL_TRUE, GMRP_MAC);
                }
              else
                {
                  gmrp_port->gmrp_failed_registrations++;
                }
            }
        }
    } /* end of else Multicast attibute */

  attr_entry_tbd = gmd_entry_tbd_lookup (gmrp->gmd, joining_attribute_index);

  if (attr_entry_tbd)
    {
      gmrp_gmd_delete_tbd_entry (gmrp->gmd, joining_attribute_index);
    }
  return;
}

void
gmrp_join_propagated (void *application, struct gid *gid, 
                      u_int32_t joining_gid_index)
{
  struct gmrp *gmrp;
  struct interface *ifp;
  u_char mac_addr[HAL_HW_LENGTH];
  struct gid_port *gid_port = NULL;
  struct gmrp_bridge *gmrp_bridge = NULL;
  
  gmrp = (struct gmrp*)application;

  if ((!gid) || (!(gid_port = gid->gid_port)) ||
      (!gmrp) || (!(gmrp_bridge = gmrp->gmrp_bridge)))
    return;

  ifp = nsm_lookup_port_by_index(gmrp_bridge->bridge, gid_port->ifindex);

  if (!ifp)
    return;

  if (GMRP_DEBUG(EVENT))
  {
    zlog_debug (gmrpm, "gmrp_join_propagated: [port %d] Recieved a join for "
        "attribute_index %u", gid_port->ifindex, joining_gid_index);
  }

  if (joining_gid_index >= GMRP_NUMBER_OF_LEGACY_CONTROL)
  { /* Multicast Address */
    if ((gid_registered_here (gid, GMRP_LEGACY_FORWARD_ALL)) &&
        (!gid_registered_here (gid, joining_gid_index)))
    {
      gmd_get_mac_addr (gmrp->gmd, joining_gid_index, mac_addr);

      if (GMRP_DEBUG(EVENT))
      {
        zlog_debug (gmrpm, "gmrp_join_propagated: [port %d] Inserting "
            "a dynamic entry into the forwarding plane FDB "
            "mac_addr %2hx:%2hx:%2hx:%2hx:%2hx:%2hx vlan %u", 
            gid_port->ifindex, mac_addr[0], mac_addr[1], mac_addr[2], 
            mac_addr[3], mac_addr[4], mac_addr[5], gmrp->vlanid);
      }

      nsm_bridge_add_mac (gmrp_bridge->bridge->master,
                          gmrp_bridge->bridge->name,
                          ifp, mac_addr, gmrp->vlanid, gmrp->svlanid,
                           HAL_L2_FDB_STATIC,PAL_TRUE, GMRP_MAC);
    }
  }
}

void
gmrp_leave_indication (void *application, struct gid *gid, 
                       u_int32_t leaving_attribute_index)
{
  s_int32_t ret;
  struct gmrp *gmrp;
  struct gmrp_gmd *gmd;
  struct ptree_node *rn;
  struct interface *ifp;
  struct nsm_if *zif = NULL;
  u_int32_t attribute_index;
  u_char mac_addr[HAL_HW_LENGTH];
  struct nsm_bridge_port *br_port;
  struct gid_port *gid_port = NULL;
  struct gmrp_port *gmrp_port = NULL;
  struct avl_tree *gmrp_port_list = NULL;
  struct gmrp_bridge *gmrp_bridge = NULL;
  struct gmrp_attr_entry_tbd *attr_entry_tbd;

  gmrp = (struct gmrp*)application;

  if ((!gid) || (!(gid_port = gid->gid_port)) ||
      (!gmrp) || (!(gmrp_bridge = gmrp->gmrp_bridge)))
    return;

  attribute_index = 0;

  ifp = nsm_lookup_port_by_index(gmrp_bridge->bridge, gid_port->ifindex);

  if ( (!ifp) || (!ifp->info))
    return;

  zif=(struct nsm_if *)ifp->info;

  if (! zif)
    return;

  br_port = zif->switchport;

  if (! br_port)
    return;

  gmrp_port_list = br_port->gmrp_port_list;

  if (!gmrp_port_list)
    return;

  gmrp_port = br_port->gmrp_port;

  if (!gmrp_port)
    return;

  /* Do not process the leave indication for fixed registration.
   */

  if ( (gmrp_port->registration_cfg & GID_REG_MGMT_FIXED) )
    {
      return;
    }

  if (GMRP_DEBUG(EVENT))
  {
    zlog_debug (gmrpm, "gmrp_leave_indication: [port %d] Recieved a leave "
        "indication for attribute_index %u", gid_port->ifindex,
        leaving_attribute_index);
  }

  if( (leaving_attribute_index  == GMRP_LEGACY_FORWARD_ALL)||
      (leaving_attribute_index  == GMRP_LEGACY_FORWARD_UNREGISTERED))
  {
     if (leaving_attribute_index == GMRP_LEGACY_FORWARD_ALL)
       ret = hal_l2_gmrp_set_service_requirement
                    (gmrp_bridge->bridge->name, gmrp->vlanid, ifp->ifindex, 
                     HAL_GMRP_LEGACY_FORWARD_ALL, PAL_FALSE);
     else
       ret = hal_l2_gmrp_set_service_requirement
                    (gmrp_bridge->bridge->name, gmrp->vlanid, ifp->ifindex, 
                     HAL_GMRP_LEGACY_FORWARD_UNREGISTERED, PAL_FALSE);

   if ((ret == NOT_IMPLEMENTED)
       && (leaving_attribute_index == GMRP_LEGACY_FORWARD_ALL)) 
     {
       gmd = gmrp->gmd;

       if (gmd)
         {
           for (rn = ptree_top (gmd->gmd_entry_table); rn;
                rn = ptree_next (rn))
            {
              attribute_index = gid_get_attr_index (rn);

              if (!gid_registered_here (gid, attribute_index))
                {
                  gmd_get_mac_addr (gmrp->gmd, attribute_index, mac_addr);

                  if (GMRP_DEBUG(EVENT))
                    {
                      zlog_debug (gmrpm, "gmrp_leave_indication: [port %d] "
                      "Deleting a dynamic entry from the forwarding "
                      "plane FDB mac_addr "
                      "%2hx:%2hx:%2hx:%2hx:%2hx:%2hx vlan %u", 
                      gid_port->ifindex, mac_addr[0], mac_addr[1], 
                      mac_addr[2], mac_addr[3], mac_addr[4], 
                      mac_addr[5], gmrp->vlanid);
                    }

                  nsm_bridge_delete_mac (gmrp_bridge->bridge->master,
                                         gmrp_bridge->bridge->name,
                                         ifp, mac_addr, gmrp->vlanid,
                                         gmrp->svlanid, HAL_L2_FDB_DYNAMIC,
                                         PAL_TRUE, GMRP_MAC);
                }

            }
          }
      } /* End of NOT_IMPLEMENTED */

  }
  else if (!(gid_registered_here (gid, GMRP_LEGACY_FORWARD_ALL)))
  {
    /* Multicast Attribute */
    gmd_get_mac_addr (gmrp->gmd, leaving_attribute_index, mac_addr);

    if (GMRP_DEBUG(EVENT))
      zlog_debug (gmrpm, "gmrp_leave_indication: [port %d] Deleting a "
          "dynamic entry from the forwarding planes FDB "
          "mac_addr %2hx:%2hx:%2hx:%2hx:%2hx:%2hx vlan %u ",
          gid_port->ifindex, mac_addr[0], mac_addr[1], mac_addr[2],
          mac_addr[3], mac_addr[4], mac_addr[5], 
          gmrp->vlanid);

    nsm_bridge_delete_mac (gmrp_bridge->bridge->master,
                           gmrp_bridge->bridge->name, 
                           ifp, mac_addr, gmrp->vlanid, gmrp->svlanid,
                           HAL_L2_FDB_STATIC, PAL_TRUE, GMRP_MAC);
 
    if (gid_attr_entry_to_be_created (gid, leaving_attribute_index))
      {
        attr_entry_tbd = gmd_entry_tbd_create (gmrp->gmd, leaving_attribute_index);

        if (!attr_entry_tbd)
          return;

         /* Delete the attribute from this gid as leave indication sent */
         gid_delete_attribute (gid, leaving_attribute_index);
      }
  }
}

void
gmrp_leave_propagated (void *application, struct gid *gid, 
                       u_int32_t leaving_attribute_index)
{
  struct gmrp *gmrp;
  struct interface *ifp;
  u_char mac_addr[HAL_HW_LENGTH];
  struct gid_port *gid_port = NULL;
  struct gmrp_bridge *gmrp_bridge = NULL;
  struct gmrp_attr_entry_tbd *attr_entry_tbd;

  gmrp = (struct gmrp*)application;

  if ((!gid) || (!(gid_port = gid->gid_port)) ||
      (!gmrp) || (!(gmrp_bridge = gmrp->gmrp_bridge)))
    return;

  ifp = nsm_lookup_port_by_index(gmrp_bridge->bridge, gid_port->ifindex);

  if (!ifp)
    return;

  if (GMRP_DEBUG(EVENT))
    zlog_debug (gmrpm, "gmrp_leave_propagated: [port %d] Recieved a leave "
        "for attribute_index %u", gid_port->ifindex, leaving_attribute_index);

  if (leaving_attribute_index >= GMRP_NUMBER_OF_LEGACY_CONTROL)
  {
    if ((gid_registered_here (gid, GMRP_LEGACY_FORWARD_ALL)) &&
        (!gid_registered_here (gid, leaving_attribute_index)))
    {
      gmd_get_mac_addr (gmrp->gmd, leaving_attribute_index, mac_addr);

      if (GMRP_DEBUG(EVENT))
      {
        zlog_debug (gmrpm, "gmrp_leave_propagated: [port %d] Deleting a "
            "dynamic entry from the forwarding planes FDB "
            "mac_addr %2hx:%2hx:%2hx:%2hx:%2hx:%2hx vlan %u ",
            gid_port->ifindex, mac_addr[0], mac_addr[1], mac_addr[2],
            mac_addr[3], mac_addr[4], mac_addr[5], 
            gmrp->vlanid);
      }

      nsm_bridge_delete_mac (gmrp_bridge->bridge->master,
                             gmrp_bridge->bridge->name, 
                             ifp, mac_addr, gmrp->vlanid, gmrp->svlanid,
                             HAL_L2_FDB_STATIC, PAL_TRUE, GMRP_MAC);
    }
  }

  /* Dont delete it now after sending the leave delete it */

  attr_entry_tbd = gmd_entry_tbd_lookup (gmrp->gmd, leaving_attribute_index);

  if (!attr_entry_tbd)
    return;

  attr_entry_tbd->pending_gid++;
}

void 
gmrp_do_actions (struct gid *gid)
{
  gip_do_actions (gid);
}

struct gid *
gmrp_get_gid (struct nsm_bridge_port *br_port, u_int16_t vlanid,
              u_int16_t svlanid)
{
  struct gmrp_port_instance *gmrp_port_instance = NULL;
  struct avl_tree *gmrp_port_list;
  struct avl_node *node;

  if ( br_port == NULL)
    return NULL;
  
  if ( (gmrp_port_list = br_port->gmrp_port_list ) == NULL)
    return NULL;

  gmrp_port_instance = gmrp_port_instance_get_by_vid (gmrp_port_list,
                                                      vlanid, svlanid, &node);

  return gmrp_port_instance ? gmrp_port_instance->gid : 0;
  
}

u_int16_t
gmrp_get_vid (void *application)
{
  struct gmrp *gmrp = (struct gmrp *)application;
  return gmrp->vlanid;
}

u_int16_t
gmrp_get_svid (void *application)
{
  struct gmrp *gmrp = (struct gmrp *)application;
  return gmrp->svlanid;
}

struct nsm_bridge *
gmrp_get_bridge_instance (void *application)
{
  struct gmrp *gmrp = (struct gmrp *)application;
  struct gmrp_bridge *gmrp_bridge = NULL;

  gmrp_bridge = gmrp->gmrp_bridge;

  if (!gmrp_bridge)
    return NULL;

  return gmrp_bridge->bridge;
}

static bool_t
gmrp_reg_vlan_listener(struct nsm_bridge *bridge, struct gmrp_bridge *new_gmrp)
{
  if ( (nsm_create_vlan_listener(&new_gmrp->gmrp_appln) != RESULT_OK) )
  {
    return PAL_FALSE;
  }

  (new_gmrp->gmrp_appln)->listener_id = NSM_LISTENER_ID_GMRP_PROTO;
  (new_gmrp->gmrp_appln)->flags |= (NSM_VLAN_DYNAMIC | NSM_VLAN_STATIC);
  (new_gmrp->gmrp_appln)->add_vlan_to_bridge_func = gmrp_add_vlan_cb;
  (new_gmrp->gmrp_appln)->delete_vlan_from_bridge_func = gmrp_delete_vlan_cb;
  (new_gmrp->gmrp_appln)->add_vlan_to_port_func = gmrp_add_vlan_to_port_cb;
  (new_gmrp->gmrp_appln)->delete_vlan_from_port_func = gmrp_delete_vlan_to_port_cb;
#ifdef HAVE_PROVIDER_BRIDGE
  (new_gmrp->gmrp_appln)->add_pro_edge_swctx_to_bridge = gmrp_add_swtcx_cb;
  (new_gmrp->gmrp_appln)->delete_pro_edge_swctx_from_bridge = gmrp_del_swtcx_cb;
  (new_gmrp->gmrp_appln)->add_port_to_swctx_func       = gmrp_add_swtcx_to_port_cb;
  (new_gmrp->gmrp_appln)->del_port_from_swctx_func     = gmrp_del_swtcx_from_port_cb;
#endif /* HAVE_PROVIDER_BRIDGE */

  if ( bridge->vlan_listener_list )
  {
    if ( nsm_add_listener_to_list(bridge->vlan_listener_list,
          new_gmrp->gmrp_appln) ==RESULT_OK ) 
    {
      return PAL_TRUE;
    }
  }

  nsm_destroy_vlan_listener(new_gmrp->gmrp_appln);
  return PAL_FALSE;
}

static void
gmrp_dereg_vlan_listener(struct nsm_bridge *bridge, struct gmrp_bridge *new_gmrp)
{
  if ( bridge->vlan_listener_list )
  {
    nsm_remove_listener_from_list(bridge->vlan_listener_list,
        (new_gmrp->gmrp_appln)->listener_id);
  }

  nsm_destroy_vlan_listener(new_gmrp->gmrp_appln);
}

static bool_t
gmrp_reg_bridge_listener(struct nsm_bridge *bridge, struct gmrp_bridge *new_gmrp)
{
  if ( (nsm_create_bridge_listener(&new_gmrp->gmrp_br_appln) != RESULT_OK) )
  {
    return PAL_FALSE;
  }
  (new_gmrp->gmrp_br_appln)->listener_id = NSM_LISTENER_ID_GMRP_PROTO;
  (new_gmrp->gmrp_br_appln)->delete_bridge_func = gmrp_disable;
  (new_gmrp->gmrp_br_appln)->disable_port_func = gmrp_disable_port_cb;
  (new_gmrp->gmrp_br_appln)->enable_port_func = gmrp_enable_port_cb;
  (new_gmrp->gmrp_br_appln)->change_port_state_func = gmrp_set_port_state;
  (new_gmrp->gmrp_br_appln)->delete_port_from_bridge_func = gmrp_delete_port_cb;
  /* Listener to activate the gmrp port when the port is attached to the bridge
   * during loading from the conf file*/
  (new_gmrp->gmrp_br_appln)->activate_port_config_func = gmrp_activate_port;
  (new_gmrp->gmrp_br_appln)->add_static_mac_addr_func = gmrp_add_static_mac_addr;
  (new_gmrp->gmrp_br_appln)->delete_static_mac_addr_func = gmrp_delete_static_mac_addr;
                                                                          
  if( bridge->bridge_listener_list )
  {
    if( nsm_add_listener_to_bridgelist(bridge->bridge_listener_list,
          new_gmrp->gmrp_br_appln) == RESULT_OK )
    {
      return PAL_TRUE;
    }
  }
                                                                          
  nsm_destroy_bridge_listener(new_gmrp->gmrp_br_appln);
  return PAL_FALSE;
}
                                                                          
static void
gmrp_dereg_bridge_listener(struct nsm_bridge *bridge, struct gmrp_bridge *new_gmrp)
{
  if( bridge->bridge_listener_list )
  {
    nsm_remove_listener_from_bridgelist(bridge->bridge_listener_list,
        (new_gmrp->gmrp_br_appln)->listener_id);
  }
                                                                          
  nsm_destroy_bridge_listener(new_gmrp->gmrp_br_appln);
}

void
gmrp_add_static_mac_addr (struct nsm_bridge_master *master, 
                          char *bridge_name, struct interface *ifp, 
                          unsigned char *mac, u_int16_t vid, 
                          u_int16_t svid, bool_t is_forward)
{
  struct gid *gid;
  struct nsm_if *zif;
  struct gmrp *gmrp = NULL;
  u_int32_t attribute_index;
  struct nsm_bridge_port *br_port;
  struct nsm_bridge *bridge = NULL;
  struct avl_tree *gmrp_list = NULL;
  struct avl_node *gmrp_node = NULL;
  struct nsm_vlan_port *port = NULL;
  struct gmrp_port *gmrp_port = NULL;
  struct avl_node *gmrp_port_node = NULL;
  struct avl_tree *gmrp_port_list = NULL;
  struct gmrp_port_instance *gmrp_port_instance = NULL;

  /* if the bridge is not vlan aware set the vid to default vid */
  if (!vid)
    vid = NSM_VLAN_DEFAULT_VID;

  if (!ifp || !(ifp->info))
    return;
  else
    zif = (struct nsm_if *)ifp->info;

  if (!(bridge = nsm_lookup_bridge_by_name (master, bridge_name)) ||
      !(br_port = zif->switchport) ||
      !(port = &(br_port->vlan_port)) ||
      !(gmrp_list = bridge->gmrp_list) ||
      !(gmrp = gmrp_get_by_vid (gmrp_list, vid, svid, &gmrp_node)) ||
      !(gmrp_port_list = br_port->gmrp_port_list) ||
      !(gmrp_port = br_port->gmrp_port) ||
      !(gmrp_port_instance = gmrp_port_instance_get_by_vid (gmrp_port_list, 
                                          vid, svid, &gmrp_port_node)))
     return;

  /* Get the attribute_index associated with the mac_addr from the GMD */
  if (! gmd_find_entry (gmrp->gmd, mac, &attribute_index))
    {
      if (!gmd_create_entry (gmrp->gmd, mac, &attribute_index))
        {
          return;
        }
    }

  gid = gmrp_port_instance->gid;

  if (!gid)
    return;

  /* Put the received port in Fixed Registration */
  gid_register_gid (gid, attribute_index);

  gid_manage_attribute (gid, attribute_index, GID_EVENT_FIXED_REGISTRATION);

  if (is_forward)
    {
      gmrp_inject_event_all_ports (master, bridge_name, vid, svid, ifp,
                                   attribute_index, GID_EVENT_JOIN);
    }
  else
    {
      gid_set_attribute_states (gmrp_port_instance->gid, attribute_index,
                                GID_EVENT_FORBID_REGISTRATION);
    }
}

void
gmrp_delete_static_mac_addr (struct nsm_bridge_master *master, 
                             char *bridge_name, struct interface *ifp,
                             unsigned char *  mac, u_int16_t vid,
                             u_int16_t svid, bool_t is_forward)
{
  struct gid *gid;
  struct nsm_if *zif;
  struct gmrp *gmrp = NULL;
  u_int32_t attribute_index;
  struct nsm_bridge_port *br_port;
  u_int32_t registration_type = 0;
  struct nsm_bridge *bridge = NULL;
  struct avl_tree *gmrp_list = NULL;
  struct avl_node *gmrp_node = NULL;
  struct nsm_vlan_port *port = NULL;
  struct gmrp_port *gmrp_port = NULL;
  struct avl_node *gmrp_port_node = NULL;
  struct avl_tree *gmrp_port_list = NULL;
  struct gmrp_port_instance *gmrp_port_instance = NULL;

  /* if the bridge is not vlan aware set the vid to default vid */
  if (!vid)
    vid = NSM_VLAN_DEFAULT_VID;

  if (!ifp || !(ifp->info))
    return;
  else
    zif = (struct nsm_if *)ifp->info;

  if (!(bridge = nsm_lookup_bridge_by_name (master, bridge_name)) ||
      !(br_port = zif->switchport) ||
      !(port = &(br_port->vlan_port)) ||
      !(gmrp_list = bridge->gmrp_list) ||
      !(gmrp = gmrp_get_by_vid (gmrp_list, vid, svid, &gmrp_node)) ||
      !(gmrp_port_list = br_port->gmrp_port_list) ||
      !(gmrp_port = br_port->gmrp_port) ||
      !(gmrp_port_instance = gmrp_port_instance_get_by_vid (gmrp_port_list,
                                          vid, svid, &gmrp_port_node)))
     return;

  /* Get the attribute_index associated with the mac_addr from the GMD */
  if (!gmd_find_entry (gmrp->gmd, mac, &attribute_index))
    return;

  gid = gmrp_port_instance->gid;

  if (!gid)
    return;

  /* Put the received port in Fixed Registration */
  gid_unregister_gid (gid, attribute_index);

  gid_manage_attribute (gid, attribute_index, GID_EVENT_JOIN);

  if (is_forward)
    {                                                                                
      gmrp_inject_event_all_ports (master, bridge_name, vid, svid, ifp, 
                                   attribute_index, GID_EVENT_LEAVE);
    }
  else
    {
      /* Reset to current state */
      switch ( gmrp_port->registration_cfg )
        {
          case GID_REG_MGMT_NORMAL : 
            registration_type = GID_EVENT_NORMAL_REGISTRATION;
            break;
          case GID_REG_MGMT_FIXED :
            registration_type = GID_EVENT_FIXED_REGISTRATION;  
            break;
          case GID_REG_MGMT_FORBIDDEN :
            registration_type = GID_EVENT_FORBID_REGISTRATION;
            break;
          default:
            return;
            break;
        }
    
      gid_set_attribute_states (gmrp_port_instance->gid, attribute_index, 
                                registration_type);
    }
}

void
gmrp_inject_event_all_ports (struct nsm_bridge_master *master, 
                             char *bridge_name, u_int16_t vid, 
                             u_int16_t svid, struct interface *ifp, 
                             u_int32_t attribute_index, gid_event_t event)
{
  struct gid *gid = NULL;
  struct avl_node *avl_port;
  struct avl_tree *gmrp_list = NULL;
  struct nsm_bridge *bridge = NULL;
  struct avl_tree *gmrp_port_list = NULL;
  struct nsm_bridge_port *br_port = NULL;
  struct nsm_vlan_port *curr_port = NULL;
  struct avl_node *gmrp_port_node = NULL;
  struct gmrp_port_instance *gmrp_port_instance = NULL;

  if (!(bridge = nsm_lookup_bridge_by_name (master, bridge_name)) ||
      !(gmrp_list = bridge->gmrp_list))
    {
      return;
    }

  /* Loop through the port list and create a gid per port */
  for (avl_port = avl_top (bridge->port_tree); avl_port;
       avl_port = avl_next (avl_port))
  {
    if ((br_port = (struct nsm_bridge_port *)
                           AVL_NODE_INFO (avl_port)) == NULL)
      continue;

      if ((curr_port = &(br_port->vlan_port)) &&
         (gmrp_port_list = br_port->gmrp_port_list) &&
         (gmrp_port_instance = gmrp_port_instance_get_by_vid (gmrp_port_list,
                                            vid, svid, &gmrp_port_node)))
        {
          if ((gid = gmrp_port_instance->gid))
            {
              gid_manage_attribute (gid, attribute_index, event);
              gid_do_actions (gid);
            }
         }
      }
  return;
}

