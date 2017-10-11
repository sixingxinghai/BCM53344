/* Copyright (C) 2004  All Rights Reserved */

#include "pal.h"
#include "lib.h"
#include "hal_incl.h"
#ifndef HAVE_CUSTOM1
#include "memory.h"
#include "thread.h"
#include "table.h"
#include "avl_tree.h"
#include "hash.h"
#include "bitmap.h"
#include "nsm_message.h"

#include "nsmd.h"
#include "nsm_server.h"
#ifdef HAVE_L2
#include "nsm_bridge.h"
#include "ptree.h"
#include "avl_tree.h"
#ifdef HAVE_VLAN
#include "nsm_vlan.h"
#include "nsm_vlan_cli.h"
#ifdef HAVE_PROVIDER_BRIDGE
#include "nsm_pro_vlan.h"
#endif /* HAVE_PROVIDER_BRIDGE */
#endif /* HAVE_VLAN */
#ifdef HAVE_VLAN_CLASS
#include "nsm_vlanclassifier.h"
#endif /* HAVE_VLAN_CLASS */
#ifdef HAVE_IGMP_SNOOP
#include "igmp.h"
#endif /* HAVE_IGMP_SNOOP */
#ifdef HAVE_MLD_SNOOP
#include "mld.h"
#endif /* HAVE_MLD_SNOOP */
#if defined (HAVE_IGMP_SNOOP) || defined (HAVE_MLD_SNOOP)
#include "nsm_l2_mcast.h"
#endif /* HAVE_IGMP_SNOOP || HAVE_MLD_SNOOP */
#include "nsm_pmirror.h"
#ifdef HAVE_RATE_LIMIT
#include "nsm_ratelimit.h"
#endif /* HAVE_RATE_LIMIT */
#ifdef HAVE_HAL 
#ifdef HAVE_QOS
#include "nsm_l2_qos.h"
#endif /* HAVE_QOS */
#endif /* HAVE_HAL */
#endif /* HAVE_L2 */
#include "nsm_interface.h"
#include "nsm_router.h"
#include "nsm_api.h"
#include "nsm_debug.h"
#include "nsm_flowctrl.h"
#include "nsm_bridge_cli.h"

#ifdef HAVE_SNMP
#include "nsm_snmp_common.h"
#endif /* HAVE_SNMP */


#ifdef HAVE_LACPD
#include "nsm_lacp.h"
#endif /* HAVE_LACPD */

#ifdef HAVE_ONMD
#include "nsm_oam.h"
#endif /* HAVE_ONMD */

#ifdef HAVE_GMRP
#include "gmrp_api.h"
#endif /* HAVE_GMRP */

#ifdef HAVE_GVRP
#include "gvrp_api.h"
#endif

#ifdef HAVE_PVLAN
#include "nsm_pvlan.h"
#endif

#ifdef HAVE_HA
#include "nsm_bridge_cal.h"
#endif /* HAVE_HA */

#ifdef HAVE_SMI
#include "lib_fm_api.h"
#include "smi_nsm_fm.h"
#endif /* HAVE_SMI */

#if defined (HAVE_I_BEB) || defined (HAVE_B_BEB)
#include "nsm_bridge_pbb.h"
#include "nsm_pbb_cli.h"
#endif

#ifdef HAVE_PBB_TE
#include "nsm_pbb_te.h"
#endif /* HAVE_PBB_TE */

#ifdef HAVE_DCB
#include "nsm_dcb_api.h"
#endif /* HAVE_DCB */

static int nsm_bridge_sync_bridge (struct nsm_master *nm, struct nsm_server_entry *nse, struct nsm_bridge *bridge, int msgid);
static int nsm_bridge_sync_bridge_if (struct nsm_master *nm, struct nsm_server_entry *nse, struct nsm_bridge *bridge, int msgid);
static int _nsm_bridge_port_delete (struct interface *ifp, u_int8_t remove_avl_node);
static int nsm_map_port_msg_state_to_hal (u_int32_t state);
extern u_char nsm_default_traffic_class_table [] [HAL_BRIDGE_MAX_TRAFFIC_CLASS];

u_char nsm_default_traffic_class_table[HAL_BRIDGE_MAX_USER_PRIO + 1]
[HAL_BRIDGE_MAX_TRAFFIC_CLASS] =   { {0, 0, 0, 1, 1, 1, 1, 2},
                                     {0, 0, 0, 0, 0, 0, 0, 0},
                                     {0, 0, 0, 0, 0, 0, 0, 1},
                                     {0, 0, 0, 1, 1, 2, 2, 3},
                                     {0, 1, 1, 2, 2, 3, 3, 4},
                                     {0, 1, 1, 2, 3, 4, 4, 5},
                                     {0, 1, 2, 3, 4, 5, 5, 6},
                                     {0, 1, 2, 3, 4, 5, 6, 7}
};


static int
nsm_bridge_port_idx_cmp (void *data1, void *data2)
{
  struct nsm_bridge_port *port1 = (struct nsm_bridge_port *) data1;
  struct nsm_bridge_port *port2 = (struct nsm_bridge_port *) data2;

  if (data1 == NULL || data2 == NULL)
    return -1;

  port1 = (struct nsm_bridge_port *) data1;
  port2 = (struct nsm_bridge_port *) data2;

  if (port1->ifindex > port2->ifindex)
    return 1;

  if (port2->ifindex > port1->ifindex)
    return -1;

#ifdef HAVE_VLAN

  if (port1->svid > port2->svid)
    return 1;

  if (port2->svid > port1->svid)
    return -1;

#endif /* HAVE_VLAN */

  return 0;

}

/* Get bridge master. */
struct nsm_bridge_master *
nsm_bridge_get_master (struct nsm_master *nm)
{
  return nm->bridge;
}

u_int8_t *
nsm_get_bridge_str (struct nsm_bridge *br)
{
  static u_int8_t bridge_str [NSM_BRIDGE_STR_LEN + NSM_BRIDGE_NAMSIZ + 1];

  pal_mem_set (bridge_str, 0, sizeof (bridge_str));

  if (! br->is_default)
    pal_snprintf (bridge_str, sizeof (bridge_str), "bridge %s", br->name);

  return bridge_str;

}

enum nsm_bridge_pri_ovr_mac_type
nsm_bridge_str_to_ovr_mac_type (char *str)
{
  if (pal_strcmp (str, "static") == 0)
    return NSM_BRIDGE_MAC_STATIC;
  else if (pal_strcmp (str, "static-priority-override") == 0)
    return NSM_BRIDGE_MAC_STATIC_PRI_OVR;
  else if (pal_strcmp (str, "static-mgmt") == 0)
    return NSM_BRIDGE_MAC_STATIC_MGMT;
  else if (pal_strcmp (str, "static-mgmt-priority-overide") == 0)
    return NSM_BRIDGE_MAC_STATIC_MGMT_PRI_OVR;

  return NSM_BRIDGE_MAC_PRI_OVR_MAX;
}

u_int8_t
nsm_bridge_str_to_type (u_int8_t *str)
{
  if (pal_strcmp (str, "stp") == 0)
    return NSM_BRIDGE_TYPE_STP;
  else if (pal_strcmp (str, "stp-vlan-bridge") == 0)
    return NSM_BRIDGE_TYPE_STP_VLANAWARE;
  else if (pal_strcmp (str, "rstp") == 0)
    return NSM_BRIDGE_TYPE_RSTP;
  else if (pal_strcmp (str, "rstp-vlan-bridge") == 0)
    return NSM_BRIDGE_TYPE_RSTP_VLANAWARE;
  else if (pal_strcmp (str, "mstp") == 0)
    return NSM_BRIDGE_TYPE_MSTP;
  else if (pal_strcmp (str, "provider-rstp") == 0)
    return NSM_BRIDGE_TYPE_PROVIDER_RSTP;
  else if (pal_strcmp (str, "provider-mstp") == 0)
    return NSM_BRIDGE_TYPE_PROVIDER_MSTP;
  else if (pal_strcmp (str, "rpvst+") == 0)
    return NSM_BRIDGE_TYPE_RPVST_PLUS;
  return NSM_BRIDGE_TYPE_STP;
}

u_int8_t *
nsm_bridge_type_to_str (u_int8_t type)
{

  switch (type)
    {
      case NSM_BRIDGE_TYPE_STP:
        return "stp";
        break;
      case NSM_BRIDGE_TYPE_STP_VLANAWARE:
        return "stp-vlan-bridge";
        break;
      case NSM_BRIDGE_TYPE_RSTP:
        return "rstp";
        break;
      case NSM_BRIDGE_TYPE_RSTP_VLANAWARE:
        return "rstp-vlan-bridge";
        break;
      case NSM_BRIDGE_TYPE_MSTP:
        return "mstp";
        break;
      case NSM_BRIDGE_TYPE_PROVIDER_RSTP:
        return "provider-rstp";
        break;
      case NSM_BRIDGE_TYPE_PROVIDER_MSTP:
        return "provider-mstp";
        break;
      case NSM_BRIDGE_TYPE_RPVST_PLUS:
        return "rpvst+";
        break;
      default:
        return "invalid";
        break;
    }

  return "invalid";
}


/* Allocate new bridge master. */
static struct nsm_bridge_master *
nsm_bridge_master_new (struct nsm_master *nm)
{
  struct nsm_bridge_master *master;
#ifdef HAVE_VLAN_CLASS
  int ret;
#endif /* HAVE_VLAN_CLASS */

  master = XCALLOC (MTYPE_NSM_BRIDGE_MASTER, sizeof (struct nsm_bridge_master));
  if (! master)
    return NULL;

#ifdef HAVE_VLAN_CLASS
  /* Create tree of rules. */
  ret = avl_create (&master->group_tree,0,nsm_vlan_classifier_group_id_cmp);

  if(ret < 0)
    {
      nsm_bridge_master_deinit (&master);
      return NULL;
    }

  /* Create tree of groups. */
  ret = avl_create (&master->rule_tree,0, nsm_vlan_classifier_rule_id_cmp);

  if(ret < 0)
    {
      nsm_bridge_master_deinit (&master);
      return NULL;
    }
#endif /* HAVE_VLAN_CLASS */

  nm->bridge = master;
  master->nm = nm;

#if defined  HAVE_G8031 || defined HAVE_G8032
  NSM_INSTANCE_BMP_CLEAR(master->instanceBmp);
#endif /* HAVE_G8031 || HAVE_G8032*/
    
  return master;
}

/* NSM Bridge master deinit. */
int
nsm_bridge_master_deinit (struct nsm_bridge_master **master)
{
  if (*master)
    {
#ifdef HAVE_VLAN_CLASS
      if((*master)->rule_tree)
        {
          nsm_del_all_classifier_rules(*master);
          nsm_vlan_classifier_del_rules_tree(&(*master)->rule_tree);

        }
      if((*master)->group_tree)
        {
          nsm_del_all_classifier_groups(*master);
          nsm_vlan_classifier_del_group_tree(&(*master)->group_tree);
        }
#endif /* HAVE_VLAN_CLASS */

      XFREE (MTYPE_NSM_BRIDGE_MASTER, *master);
      *master = NULL;
    }

  return 0;
}

/* Initialize Layer 2 protocols. */
int
nsm_l2_init (struct lib_globals *zg)
{
  flow_control_init (zg);

#if defined (HAVE_GVRP) || defined (HAVE_GMRP)
  garp_init (zg);

#ifdef HAVE_GVRP
  gvrp_init (zg);
#endif /* HAVE_GVRP */

#ifdef HAVE_GMRP
  gmrp_init (zg);
#endif /* HAVE_GMRP */
#endif /* defined (HAVE_GVRP) || defined (HAVE_GMRP) */

#ifdef HAVE_RATE_LIMIT
  nsm_ratelimit_init(zg);
#endif /* HAVE_RATE_LIMIT */

  port_mirroring_init(zg);

  return 0;
}

/* Deinitialize Layer 2 protocols. */
int
nsm_l2_deinit (struct lib_globals *zg)
{
#if defined (HAVE_GVRP) || defined (HAVE_GMRP)
  garp_deinit (zg);
#endif /* defined (HAVE_GVRP) || defined (HAVE_GMRP) */

  port_mirroring_deinit ();
  return 0;
}

#ifdef HAVE_DEFAULT_BRIDGE
static int
nsm_default_bridge_add_event (struct thread *thread)
{
  char name[NSM_BRIDGE_NAMSIZ + 1];
  struct nsm_bridge_master *bridge = THREAD_ARG (thread);
  
  pal_strncpy (name, "0", NSM_BRIDGE_NAMSIZ);

  if (! bridge)
    return -1;
#ifdef HAVE_B_BEB
  nsm_bridge_add (bridge, NSM_BRIDGE_TYPE_BACKBONE_RSTP,
                  name, PAL_TRUE, TOPOLOGY_NONE, PAL_FALSE, PAL_FALSE, NULL);
#endif
#if defined (HAVE_PROVIDER_BRIDGE) || defined (HAVE_I_BEB)
  nsm_bridge_add (bridge, NSM_BRIDGE_TYPE_PROVIDER_RSTP,
                  name, PAL_TRUE, TOPOLOGY_NONE, PAL_FALSE, PAL_FALSE, NULL);
#else
  nsm_bridge_add (bridge, NSM_BRIDGE_TYPE_RSTP_VLANAWARE,
                  name, PAL_TRUE, TOPOLOGY_NONE, PAL_FALSE, PAL_FALSE, NULL);
#endif /*(HAVE_PROVIDER_BRIDGE) || (HAVE_I_BEB)*/
  
  return 0;
}
#endif /* HAVE_DEFAULT_BRIDGE */

/* Initialize bridge master. */
int
nsm_bridge_init (struct nsm_master *nm)
{

  char name[NSM_BRIDGE_NAMSIZ + 1];

  nm->bridge = nsm_bridge_master_new (nm);

  pal_strncpy (name, "0", NSM_BRIDGE_NAMSIZ);
#if defined HAVE_IGMP_SNOOP || defined HAVE_MLD_SNOOP
  nsm_l2_mcast_init (nm);
#endif/* HAVE_IGMP_SNOOP || HAVE_MLD_SNOOP */

#if defined HAVE_I_BEB || defined HAVE_B_BEB
  nsm_pbb_init(nm);
/* check for the return value*/
#endif /* HAVE_I_BEB || HAVE_B_BEB */

#ifdef HAVE_DEFAULT_BRIDGE

  if (! nm->bridge->event)
    {
      nm->bridge->event = thread_add_event (NSM_ZG, nsm_default_bridge_add_event,
                                            nm->bridge, 0);
    }

#endif /* HAVE_DEFAULT_BRIDGE */

  return 0;
}

/* Deinitialize bridge master. */
int
nsm_bridge_deinit (struct nsm_master *nm)
{
#if defined HAVE_IGMP_SNOOP || defined HAVE_MLD_SNOOP
  nsm_l2_mcast_deinit (nm);
#endif /* HAVE_IGMP_SNOOP || HAVE_MLD_SNOOP */

#if defined HAVE_I_BEB || defined HAVE_B_BEB
  nsm_pbb_deinit(nm);
#endif /* HAVE_I_BEB || HAVE_B_BEB */

#ifdef HAVE_DCB
  nsm_dcb_deinit (nm->bridge);
#endif /* HAVE_DCB */

  if (nm->bridge->event)
    {
      /* Cancel all previous events. */
      thread_cancel_event (NSM_ZG, nm->bridge);
      nm->bridge->event = NULL;
    }

  /* Bridge master deinitialization. */
  nsm_bridge_master_deinit (&nm->bridge);

  return 0;
}

/* Allocate new bridge. */
struct nsm_bridge *
nsm_bridge_new (u_char type, char *name, char is_default,
                u_char topology_type, u_int8_t edge, u_int8_t beb,
                u_char *macaddr)
{
  s_int32_t ret;
  struct nsm_bridge *br = NULL;

  br = XCALLOC (MTYPE_NSM_BRIDGE, sizeof (struct nsm_bridge));
  if (! br)
    return NULL;

  br->type = type;
  br->is_default = is_default;
  br->topology_type = topology_type;
  pal_strncpy (br->name, name, NSM_BRIDGE_NAMSIZ);
  br->ageing_time = NSM_BRIDGE_AGEING_DEFAULT;
  br->learning = NSM_LEARNING_BRIDGE_SET;
  br->static_fdb_list = ptree_init (ETHER_MAX_BIT_LEN);
  br->enable = PAL_TRUE;

#ifdef HAVE_PROVIDER_BRIDGE
  br->provider_edge = edge;
#endif
#if defined (HAVE_I_BEB)||defined (HAVE_B_BEB)
  br->backbone_edge = beb;
  br->bridge_id = pal_atoi(br->name);
  if (macaddr)
    pal_mem_cpy (br->bridge_mac,(u_int8_t *) macaddr, ETHER_ADDR_LEN);
#ifdef HAVE_I_BEB
  pal_mem_set (br->vip_port_map, 0, 512);
#endif /*HAVE_I_BEB*/
#endif /* (HAVE_I_BEB)|| (HAVE_B_BEB)  */ 
  if (! br->static_fdb_list )
    {
      XFREE(MTYPE_NSM_BRIDGE, br);
      return NULL;
    }

  ret = avl_create (&br->port_tree, 0, nsm_bridge_port_idx_cmp);

  if(ret < 0)
    {
      ptree_finish (br->static_fdb_list);
      XFREE(MTYPE_NSM_BRIDGE, br);
      return NULL;
    }

  br->bridge_listener_list = list_new();

  if ( !br->bridge_listener_list )
    {
      ptree_finish(br->static_fdb_list);
      /* Delete port tree. */
      avl_tree_free (&br->port_tree, NULL);

      XFREE (MTYPE_NSM_BRIDGE, br);
      return NULL;
    }


  br->cvlan_reg_tab_list = list_new();

  if ( !br->cvlan_reg_tab_list )
    {
      ptree_finish(br->static_fdb_list);
      avl_tree_free (&br->port_tree, NULL);
      list_delete (br->bridge_listener_list);
      XFREE (MTYPE_NSM_BRIDGE, br);
      return NULL;
    }

#ifdef HAVE_SNMP
  br->port_id_mgr = bitmap_new (32, 1, NSM_L2_SNMP_MAX_PORTS);

  if (br->port_id_mgr == NULL)
    {
      ptree_finish(br->static_fdb_list);
      avl_tree_free (&br->port_tree, NULL);
      list_delete (br->bridge_listener_list);
      list_delete (br->cvlan_reg_tab_list);
      XFREE (MTYPE_NSM_BRIDGE, br);
      return NULL;
    }
#endif /* HAVE_SNMP */

#ifdef HAVE_DCB
   nsm_dcb_init (br);
#endif /* HAVE_DCB */

#ifdef HAVE_HA
  nsm_bridge_cal_create_nsm_bridge (br);
#endif /* HAVE_HA */
  return br;
}

/* Free bridge. */
static void
nsm_bridge_free (struct nsm_bridge **bridge)
{
  if (*bridge)
    {
#ifdef HAVE_HA
      nsm_bridge_cal_delete_nsm_bridge (*bridge);
#endif /* HAVE_HA */

      /* Delete port tree. */
      avl_tree_free (&((*bridge)->port_tree), NULL);

      /* Delete bridge_listner list. */
      list_delete ((*bridge)->bridge_listener_list);
      if ((*bridge)->static_fdb_list)
        {
          nsm_bridge_free_static_list ((*bridge)->static_fdb_list);
          ptree_finish((*bridge)->static_fdb_list);
        }

      /* Delete cvlan_reg_tab_list list. */
      list_delete ((*bridge)->cvlan_reg_tab_list);

#ifdef HAVE_SNMP
      bitmap_free ((*bridge)->port_id_mgr);
#endif /* HAVE_SNMP */

      XFREE (MTYPE_NSM_BRIDGE, *bridge);
      *bridge = NULL;
    }

  return;
}

/* Link new bridge. */
void
nsm_link_bridge (struct nsm_bridge_master *master, struct nsm_bridge *new)
{
  if (! master)
    return;
#ifdef HAVE_B_BEB
  if (pal_strncmp(new->name,"backbone",NSM_BRIDGE_NAMSIZ))
    {
#endif /* HAVE_B_BEB */
      new->next = master->bridge_list;
      if (new->next != NULL)
        new->next->pprev = &new->next;
      master->bridge_list = new;
      new->pprev = &master->bridge_list;
#if !defined HAVE_I_BEB && !defined HAVE_B_BEB && defined HAVE_PBB_TE
      avl_create (&new->pbb_te_group_tree, 0, nsm_pbb_te_cmp);
#endif /* !HAVE_I_BEB && !HAVE_B_BEB */

#ifdef HAVE_I_BEB
    if (new->backbone_edge)
      master->beb->beb_tot_icomp += 1;
#endif /* HAVE_I_BEB */
#ifdef HAVE_B_BEB
    }
  else
    {
      master->b_bridge = new; 
      master->beb->beb_tot_bcomp = 1;
#if defined HAVE_I_BEB && defined HAVE_B_BEB && defined (HAVE_PBB_TE)
      /* creating the pbb te global list to hold all the te-instances */
      avl_create (&new->pbb_te_group_tree, 0, nsm_pbb_te_cmp);
#endif /* HAVE_I_BEB && HAVE_PBB_TE */
    }
#endif /* HAVE_B_BEB */
}

#ifdef HAVE_G8032
#include "nsm_message.h"
#include "onmd.h"
#include "g8032_api.h"
#endif  /*HAVE_G8032*/

/* Unlink new bridge. */
static void
nsm_unlink_bridge (struct nsm_bridge_master *master, struct nsm_bridge *curr)
{
  if (! master)
    return;
#ifdef HAVE_B_BEB 
  if (pal_strncmp(curr->name,"backbone",NSM_BRIDGE_NAMSIZ))
    {
#endif /* HAVE_B_BEB */
      *(curr->pprev) = curr->next;
      if (curr->next != NULL)
        curr->next->pprev = curr->pprev;
      curr->next = NULL;
      curr->pprev = NULL;
#ifdef HAVE_I_BEB
      master->beb->beb_tot_icomp--;
#endif /* HAVE_I_BEB */
#ifdef HAVE_B_BEB
    }
  else
    {
      master->b_bridge = NULL;
      master->beb->beb_tot_bcomp = 0;
    }
#endif /* HAVE_B_BEB */
}

/* Lookup bridge by name. */
struct nsm_bridge *
nsm_lookup_bridge_by_name (struct nsm_bridge_master *master,
                           const char *const name)
{
  struct nsm_bridge *curr;

  if (! master)
    return NULL;
#ifdef HAVE_B_BEB
  if (! pal_strncmp ("backbone", name, NSM_BRIDGE_NAMSIZ))
    {
      if (master->b_bridge)
        return master->b_bridge;
    }
  else
    {
#endif /* HAVE_B_BEB */
      curr = master->bridge_list;
      if (! curr)
        return NULL;

      while (curr)
        {
          if (! strncmp (curr->name, name, NSM_BRIDGE_NAMSIZ))
            return curr;

          curr = curr->next;
        }
#ifdef HAVE_B_BEB
    }
#endif /* HAVE_B_BEB */
  return NULL;
}

#ifdef HAVE_VLAN
struct nsm_bridge *
nsm_lookup_bridge_by_vlan_id (struct nsm_master *nm, int vid, u_char type)
{
  struct avl_tree *vlan_table;
  struct nsm_bridge_master *bm;
  struct nsm_bridge *br;
  struct avl_node *rn;
  struct nsm_vlan *vlan;
  struct nsm_vlan nvlan;

  bm = nsm_bridge_get_master (nm);
  br = nsm_get_first_bridge (bm);

  while (br)
    {
      if (nsm_bridge_is_vlan_aware (br->master, br->name) != PAL_TRUE)
        {
          br = br->next;
          continue;
        }

      vlan_table = type == NSM_VLAN_SVLAN ? br->svlan_table :
                                            br->vlan_table;

      if (! vlan_table)
       continue;

      NSM_VLAN_SET (&nvlan, vid);
      rn = avl_search (vlan_table, (void *)&nvlan);
      if ( rn)
        {
          vlan = rn->info;
          if (vlan->vid == vid)
            return br;
        }
      br = br->next;
    }

  return NULL;
}
#endif /* HAVE_VLAN */

struct nsm_bridge *
nsm_get_first_bridge (struct nsm_bridge_master *master)
{
  if (! master)
    return NULL;

  return (master->bridge_list);
}

struct nsm_bridge *
nsm_get_default_bridge (struct nsm_bridge_master *master)
{
  struct nsm_bridge *curr;

  if (! master)
    return NULL;

  curr = master->bridge_list;
  if (! curr)
    return NULL;

  while (curr)
    {
      if (curr->is_default)
        return curr;

      curr = curr->next;
    }

  return NULL;
}

struct nsm_bridge *
nsm_get_next_bridge (struct nsm_bridge_master *master,
                     struct nsm_bridge *bridge)
{
  struct nsm_bridge *curr;

  if (! master)
    return NULL;

  curr = master->bridge_list;
  if (! curr)
    return NULL;

  while (curr)
    {
      if (! strncmp (curr->name, bridge->name, NSM_BRIDGE_NAMSIZ))
        return curr->next;

      curr = curr->next;
    }

  return NULL;
}

/* Add bridge background event. */
static int
nsm_bridge_add_event (struct thread *thread)
{
  int i;
  struct listnode *ln;
  struct nsm_server_client *nsc;
  struct nsm_server_entry *nse;
  struct nsm_bridge_listener *brlistnr;
  struct nsm_bridge *bridge = THREAD_ARG (thread);

  if (! bridge)
    return -1;

  /* Send message to PM for bridge add. */
  NSM_SERVER_CLIENT_LOOP (ng, i, nsc, nse)
    if (NSM_CHECK_CTYPE (nse->service.bits, NSM_SERVICE_BRIDGE))
      {
        nsm_bridge_sync_bridge (bridge->master->nm, nse, bridge,
                                NSM_MSG_BRIDGE_ADD);
      }

  LIST_LOOP (bridge->bridge_listener_list, brlistnr, ln)
    {
      if(brlistnr->add_bridge_func)
        {
          (brlistnr->add_bridge_func)(bridge->master, bridge->name);
        }
    }

#ifdef HAVE_IGMP_SNOOP
  nsm_l2_mcast_add_bridge (bridge, bridge->master);
#endif /* HAVE_IGMP_SNOOP */

#ifdef HAVE_VLAN
  if ( nsm_bridge_is_vlan_aware(bridge->master, bridge->name) == PAL_TRUE )
    {
      nsm_vlan_init (bridge);
    }
#endif /* HAVE_VLAN */

#if defined HAVE_I_BEB && defined HAVE_B_BEB
  nsm_pbb_ib_bridge_init (bridge);
#endif /* HAVE_I_BEB && HAVE_B_BEB */

  return 0;
}


int
nsm_map_bridge_type_to_hal_protocol (u_char type)
{

  switch(type)
    {
    case NSM_BRIDGE_TYPE_STP:
    case NSM_BRIDGE_TYPE_STP_VLANAWARE:
      return HAL_BRIDGE_STP;
      break;
    case NSM_BRIDGE_TYPE_RSTP:
    case NSM_BRIDGE_TYPE_RSTP_VLANAWARE:
      return HAL_BRIDGE_RSTP;
      break;
    case NSM_BRIDGE_TYPE_MSTP:
      return HAL_BRIDGE_MSTP;
      break;
    case NSM_BRIDGE_TYPE_PROVIDER_RSTP:
      return HAL_BRIDGE_PROVIDER_RSTP;
      break;
    case NSM_BRIDGE_TYPE_PROVIDER_MSTP:
      return HAL_BRIDGE_PROVIDER_MSTP;
      break;
    case NSM_BRIDGE_TYPE_BACKBONE_RSTP:
      return HAL_BRIDGE_BACKBONE_RSTP;
      break;
    case NSM_BRIDGE_TYPE_BACKBONE_MSTP:
      return HAL_BRIDGE_BACKBONE_MSTP;
      break;
    case NSM_BRIDGE_TYPE_RPVST_PLUS:
      return HAL_BRIDGE_RPVST_PLUS;
      break;
    default:
      return HAL_PROTO_MAX;
      break;
    }
}

enum hal_l2_proto
nsm_map_str_to_hal_protocol (u_int8_t *proto_str)
{

  if (! pal_strncmp (proto_str, "stp", 3))
    return HAL_PROTO_STP;
  else if (! pal_strncmp (proto_str, "rstp", 4))
    return HAL_PROTO_RSTP;
  else if (! pal_strncmp (proto_str, "mstp", 4))
    return HAL_PROTO_MSTP;
  else if (! pal_strncmp (proto_str, "gmrp", 4))
    return HAL_PROTO_GMRP;
  else if (! pal_strncmp (proto_str, "gvrp", 4))
    return HAL_PROTO_GVRP;
  else if (! pal_strncmp (proto_str, "mmrp", 4))
    return HAL_PROTO_MMRP;
  else if (! pal_strncmp (proto_str, "mvrp", 4))
    return HAL_PROTO_MVRP;
  else if (! pal_strncmp (proto_str, "lacp", 4))
    return HAL_PROTO_LACP;
  else if (! pal_strncmp (proto_str, "dot1x", 5))
    return HAL_PROTO_DOT1X;

  return HAL_PROTO_MAX;
}

enum hal_l2_proto_process
nsm_map_str_to_hal_protocol_process (u_int8_t *process_str)
{

  if (! pal_strncmp (process_str, "peer", 4))
    return HAL_L2_PROTO_PEER;
  else if (! pal_strncmp (process_str, "tunnel", 6))
    return HAL_L2_PROTO_TUNNEL;
  else if (! pal_strncmp (process_str, "discard", 7))
    return HAL_L2_PROTO_DISCARD;

  return HAL_L2_PROTO_PROCESS_MAX;
}

u_int8_t *
nsm_map_hal_proto_process_to_str (enum hal_l2_proto_process process)
{
  switch (process)
    {
      case HAL_L2_PROTO_PEER:
        return "Peer";
        break;
      case HAL_L2_PROTO_TUNNEL:
        return "Tunnel";
        break;
      case HAL_L2_PROTO_DISCARD:
        return "Discard";
        break;
      default:
        return "Invalid";
        break;
    }

  return "Invalid";
}

u_int8_t *
nsm_map_hal_proto_to_str (enum hal_l2_proto proto)
{
  switch (proto)
    {
      case HAL_PROTO_STP:
        return "stp";
        break;
      case HAL_PROTO_RSTP:
        return "rstp";
        break;
      case HAL_PROTO_MSTP:
        return "mstp";
        break;
      case HAL_PROTO_LACP:
        return "lacp";
        break;
      case HAL_PROTO_GMRP:
        return "gmrp";
        break;
      case HAL_PROTO_MMRP:
        return "mmrp";
        break;
      case HAL_PROTO_GVRP:
        return "gvrp";
        break;
      case HAL_PROTO_MVRP:
        return "mvrp";
        break;
      case HAL_PROTO_DOT1X:
        return "dot1x";
        break;
      default:
        return "Invalid";
        break;
    }

  return "Invalid";
}

#ifdef HAVE_PROVIDER_BRIDGE

u_int8_t *
nsm_bridge_get_process_str (struct interface *ifp,
                            enum hal_l2_proto proto)
{
  struct nsm_if *zif;
  struct nsm_bridge_port *br_port;

  zif = (struct nsm_if *)ifp->info;

  if (! zif)
    return "Invalid";

  if (! (br_port = zif->switchport))
    return "Invalid";

  if (! zif->bridge)
    return "Invalid";

  switch (proto)
    {

#if defined HAVE_STPD || defined HAVE_RSTPD || defined HAVE_MSTPD
      case HAL_PROTO_STP:
      case HAL_PROTO_RSTP:
      case HAL_PROTO_MSTP:
        return nsm_map_hal_proto_process_to_str
                       (br_port->proto_process.stp_process);
        break;
#endif /* HAVE_STPD || HAVE_RSTPD || HAVE_MSTPD */

#ifdef HAVE_GMRP
      case HAL_PROTO_GMRP:
        return nsm_map_hal_proto_process_to_str
                       (br_port->proto_process.gmrp_process);
        break;
#endif /* HAVE_GMRP */

#ifdef HAVE_MMRP
      case HAL_PROTO_MMRP:
        return nsm_map_hal_proto_process_to_str
                       (br_port->proto_process.mmrp_process);
        break;
#endif /* HAVE_MMRP */

#ifdef HAVE_GVRP
      case HAL_PROTO_GVRP:
        return nsm_map_hal_proto_process_to_str
                       (br_port->proto_process.gvrp_process);
        break;
#endif /* HAVE_GVRP */

#ifdef HAVE_MVRP
      case HAL_PROTO_MVRP:
        return nsm_map_hal_proto_process_to_str
                       (br_port->proto_process.mvrp_process);
        break;
#endif /* HAVE_MVRP */

#ifdef HAVE_LACPD
      case HAL_PROTO_LACP:
        return nsm_map_hal_proto_process_to_str
                       (br_port->proto_process.lacp_process);
        break;
#endif /* HAVE_LACPD */

#ifdef HAVE_AUTHD
      case HAL_PROTO_DOT1X:
        return nsm_map_hal_proto_process_to_str
                       (br_port->proto_process.dot1x_process);
        break;
#endif /* HAVE_AUTHD */

      default:
        return "Invalid";
        break;
    }

  return "Invalid";
}

u_int16_t
nsm_bridge_get_process_vid (struct interface *ifp,
                            enum hal_l2_proto proto)
{
  u_int16_t vid;
  struct nsm_if *zif;
  struct nsm_bridge_port *br_port;

  zif = (struct nsm_if *)ifp->info;

  vid = NSM_VLAN_NULL_VID;

  if (! zif)
    return vid;

  if (! (br_port = zif->switchport))
    return vid;

  if (! zif->bridge)
    return vid;

  vid = NSM_VLAN_NULL_VID;

  switch (proto)
    {

#if defined HAVE_STPD || defined HAVE_RSTPD || defined HAVE_MSTPD
      case HAL_PROTO_STP:
      case HAL_PROTO_RSTP:
      case HAL_PROTO_MSTP:
        if (br_port->proto_process.stp_process == HAL_L2_PROTO_TUNNEL)
          vid = br_port->proto_process.stp_tun_vid;
        break;
#endif /* HAVE_STPD || HAVE_RSTPD || HAVE_MSTPD */

#ifdef HAVE_GVRP
      case HAL_PROTO_GVRP:
        if (br_port->proto_process.gvrp_process == HAL_L2_PROTO_TUNNEL)
          vid = br_port->proto_process.gvrp_tun_vid;
        break;
#endif /* HAVE_GVRP */

#ifdef HAVE_MVRP
      case HAL_PROTO_MVRP:
        if (br_port->proto_process.mvrp_process == HAL_L2_PROTO_TUNNEL)
          vid = br_port->proto_process.mvrp_tun_vid;
#endif /* HAVE_MVRP */

#ifdef HAVE_LACPD
      case HAL_PROTO_LACP:
        if (br_port->proto_process.lacp_process == HAL_L2_PROTO_TUNNEL)
          vid = br_port->proto_process.lacp_tun_vid;
        break;
#endif /* HAVE_LACPD */

#ifdef HAVE_AUTHD
      case HAL_PROTO_DOT1X:
        if (br_port->proto_process.dot1x_process == HAL_L2_PROTO_TUNNEL)
          vid = br_port->proto_process.dot1x_tun_vid;
        break;
#endif /* HAVE_AUTHD */
      default:
        break;
    }

  return vid;
}

#endif /* HAVE_PROVIDER_BRIDGE */

/* Change the bridge type for ex, from stp->rstp */

int
nsm_bridge_change_type (struct nsm_bridge_master *master,
                        char *name, u_char type, u_char topo_type)
{
  int i;
  int nbytes;
  int ret = 0;
  struct listnode *ln;
  struct listnode *ln_next;
  struct avl_node *avlnext;
  int is_old_type_provider;
  struct avl_node *avl_port;
  struct nsm_if *zif = NULL;
  struct nsm_bridge *bridge;
  int is_new_type_vlan_aware;
  int is_old_type_vlan_aware;
  struct nsm_msg_bridge_if msg;
  struct interface *ifp = NULL;
  struct nsm_server_entry *nse;
  struct nsm_server_client *nsc;
  struct nsm_bridge_port *br_port;
  struct nsm_bridge_listener *brlistnr;
#ifdef HAVE_VLAN
  struct avl_node *rn;
  struct nsm_vlan *vlan;
#endif /* HAVE_VLAN */

  if (!master)
    return NSM_BRIDGE_ERR_NOTFOUND;

  bridge = nsm_lookup_bridge_by_name (master, name);

  is_old_type_provider = PAL_FALSE;

  if (!bridge)
    return NSM_BRIDGE_ERR_NOTFOUND;

#ifdef HAVE_HA
  nsm_bridge_cal_modify_nsm_bridge (bridge);
#endif /* HAVE_HA */

  if ((type == NSM_BRIDGE_TYPE_STP_VLANAWARE) ||
      (type == NSM_BRIDGE_TYPE_RSTP_VLANAWARE) ||
      (type == NSM_BRIDGE_TYPE_RPVST_PLUS) ||
      (type == NSM_BRIDGE_TYPE_MSTP) ||
      (type == NSM_BRIDGE_TYPE_PROVIDER_RSTP) ||
      (type == NSM_BRIDGE_TYPE_PROVIDER_MSTP) ||
      (type == NSM_BRIDGE_TYPE_BACKBONE_RSTP) ||
      (type == NSM_BRIDGE_TYPE_BACKBONE_MSTP))
    {
      is_new_type_vlan_aware = PAL_TRUE;
    }
  else
    {
      is_new_type_vlan_aware = PAL_FALSE;
    }

   is_old_type_vlan_aware = nsm_bridge_is_vlan_aware (bridge->master,
                                                      bridge->name);

#ifdef HAVE_PROVIDER_BRIDGE
   is_old_type_provider = NSM_BRIDGE_TYPE_PROVIDER (bridge);
#endif /* HAVE_PROVIDER_BRIDGE */

 /* Disable the ports for bridge listeners */

 for (avl_port = avl_top (bridge->port_tree); avl_port; avl_port = avlnext)
    {
      avlnext = avl_next (avl_port);

      if ((br_port = (struct nsm_bridge_port *)
                             AVL_NODE_INFO (avl_port)) == NULL)
        continue;

      if ((zif = br_port->nsmif) == NULL)
        continue;

      if ((ifp = zif->ifp) == NULL)
        continue;

      LIST_LOOP (bridge->bridge_listener_list, brlistnr, ln)
        {
          if (brlistnr->disable_port_func)
            {
              (brlistnr->disable_port_func)(bridge->master, ifp);
            }
        }
   }

#ifdef HAVE_VLAN
  /* Previously if the bridge is vlan aware and new type is not
     vlan aware delete all vlans */
  if (((is_old_type_vlan_aware) && (!is_new_type_vlan_aware))
       || (is_old_type_provider) || (type == NSM_BRIDGE_TYPE_PROVIDER_RSTP)
       || (type == NSM_BRIDGE_TYPE_PROVIDER_MSTP))
    {
      /* Delete all the vlans */
      if (bridge->vlan_table != NULL)
        for (rn = avl_top (bridge->vlan_table); rn; rn = avl_next (rn))
             if ((vlan = rn->info) != NULL)
               nsm_vlan_delete (master, bridge->name, vlan->vid, vlan->type);

      if (bridge->svlan_table != NULL)
        for (rn = avl_top (bridge->svlan_table); rn; rn = avl_next (rn))
             if ((vlan = rn->info) != NULL)
               nsm_vlan_delete (master, bridge->name, vlan->vid, vlan->type);

      for (avl_port = avl_top (bridge->port_tree); avl_port; avl_port = avlnext)
        {
          avlnext = avl_next (avl_port);

          if ((br_port = (struct nsm_bridge_port *)
                                 AVL_NODE_INFO (avl_port)) == NULL)
            continue;

          if ((zif = br_port->nsmif) == NULL)
            continue;

          if ((ifp = zif->ifp) == NULL)
            continue;

          nsm_vlan_clear_port (ifp);
        }

      nsm_vlan_deinit (bridge);

      /*Change vlan type in the hardware */
      hal_bridge_change_vlan_type (bridge->name, is_new_type_vlan_aware,
                                   nsm_map_bridge_type_to_hal_protocol (type));

      /* Delete the brige listeners */
      for (ln = LISTHEAD (bridge->bridge_listener_list); ln; ln = ln_next)
        {
          ln_next = ln->next;

          if (((brlistnr = (struct nsm_bridge_listener *)GETDATA (ln)) == NULL)
                 || (!brlistnr->delete_bridge_func))
            continue;

          (brlistnr->delete_bridge_func)(bridge->master, bridge->name);
        }
    }
#endif /* HAVE_VLAN */

  /* Send bridge delete message to all protocols */
  NSM_SERVER_CLIENT_LOOP (ng, i, nsc, nse)
    if (NSM_CHECK_CTYPE (nse->service.bits, NSM_SERVICE_BRIDGE))
      nsm_bridge_sync_bridge (bridge->master->nm, nse, bridge,
                              NSM_MSG_BRIDGE_DELETE);

  /* Change type */
  bridge->type = type;

  /* Send message to PM for bridge add. */
  NSM_SERVER_CLIENT_LOOP (ng, i, nsc, nse)
    if (NSM_CHECK_CTYPE (nse->service.bits, NSM_SERVICE_BRIDGE))
      {
        nsm_bridge_sync_bridge (bridge->master->nm, nse, bridge,
                                NSM_MSG_BRIDGE_ADD);
      }

#ifdef HAVE_VLAN
  if (((!is_old_type_vlan_aware) && (is_new_type_vlan_aware))
       || (is_old_type_provider) || (type == NSM_BRIDGE_TYPE_PROVIDER_RSTP)
       || (type == NSM_BRIDGE_TYPE_PROVIDER_MSTP))
    {
      /*Change vlan type in the hardware */
      hal_bridge_change_vlan_type (bridge->name, is_new_type_vlan_aware,
                                   nsm_map_bridge_type_to_hal_protocol (type));

      ret = nsm_vlan_init (bridge);

      if (ret < 0)
        return ret;
    }
#endif /* HAVE_VLAN */

  for (avl_port = avl_top (bridge->port_tree); avl_port; avl_port = avlnext)
    {
      avlnext = avl_next (avl_port);

      if ((br_port = (struct nsm_bridge_port *)
                             AVL_NODE_INFO (avl_port)) == NULL)
        continue;

      if ((zif = br_port->nsmif) == NULL)
        continue;

      if ((ifp = zif->ifp) == NULL)
        continue;

      /* Send interface add message to PM. */
      NSM_SERVER_CLIENT_LOOP (ng, i, nsc, nse)
        if (NSM_CHECK_CTYPE (nse->service.bits, NSM_SERVICE_BRIDGE))
          {
            pal_strncpy (msg.bridge_name, bridge->name, NSM_BRIDGE_NAMSIZ);
            msg.num = 1;

            /* Allocate ifindex array. */
            msg.ifindex = XCALLOC (MTYPE_TMP, msg.num * sizeof (u_int32_t));
            msg.ifindex[0] = ifp->ifindex;
            msg.spanning_tree_disable = br_port->spanning_tree_disable;

            /* Set nse pointer and size. */
            nse->send.pnt = nse->send.buf + NSM_MSG_HEADER_SIZE;
            nse->send.size = nse->send.len - NSM_MSG_HEADER_SIZE;

            /* Encode NSM bridge interface message. */
            nbytes = nsm_encode_bridge_if_msg (&nse->send.pnt,
                                               &nse->send.size, &msg);
            if (nbytes < 0)
              return nbytes;

            /* Send bridge interface message. */
            nsm_server_send_message (nse, 0, 0, NSM_MSG_BRIDGE_ADD_PORT,
                                     0, nbytes);
            /* Free ifindex array. */
            XFREE (MTYPE_TMP, msg.ifindex);
          }
#ifdef HAVE_VLAN
       /* If previously it is not vlan aware and now vlan aware,
          add the default vlan */

       if (is_new_type_vlan_aware)
         NSM_VLAN_SET_PORT_DEFAULT_MODE (ifp, PAL_TRUE, PAL_TRUE);
#endif /* HAVE_VLAN */
    }

#ifdef HAVE_HAL
  if ( hal_bridge_set_state (bridge->name, PAL_TRUE) != HAL_SUCCESS )
    return NSM_BRIDGE_ERR_HAL;
#endif /* HAVE_HAL */

  bridge->enable = PAL_TRUE;

  bridge->topology_type = topo_type;

#ifdef HAVE_HA
  nsm_bridge_cal_modify_nsm_bridge (bridge);
#endif /* HAVE_HA */

#ifdef HAVE_SMI
  smi_record_bridge_proto_change_alarm (bridge->name, type, topo_type);
#endif /* HAVE_SMI */

  return 0;
}

/* Add bridge. */
int
nsm_bridge_add (struct nsm_bridge_master *master, u_char type, char *name,
                bool_t is_default, u_char topology_type, u_int8_t edge,
                u_int8_t beb, u_char *macaddr)

{
  struct nsm_bridge *bridge;
  int ret = 0;
  u_int32_t is_vlan_aware;

  if (! master)
    return NSM_BRIDGE_ERR_NOTFOUND;
#ifdef HAVE_B_BEB
  if (!pal_strncmp(name,"backbone",NSM_BRIDGE_NAMSIZ)&& 
      master->beb->beb_tot_bcomp)
    return NSM_BRIDGE_ERR_BACKBONE_EXISTS;
#endif
  bridge = nsm_lookup_bridge_by_name (master, name);

  if (bridge)
    {
#ifdef HAVE_I_BEB
      if (beb)
        return NSM_BRIDGE_ERR_EXISTS; 
#endif
      if (bridge->type == type)
        return 0;
      else
        {
          /* Change the bridge protocol type */
          ret = nsm_bridge_change_type (master, name, type, topology_type);

          return ret;
        }
    }

  if ( (type == NSM_BRIDGE_TYPE_STP_VLANAWARE) ||
       (type == NSM_BRIDGE_TYPE_RSTP_VLANAWARE) ||
       (type == NSM_BRIDGE_TYPE_RPVST_PLUS) ||
       (type == NSM_BRIDGE_TYPE_MSTP) ||
       (type == NSM_BRIDGE_TYPE_PROVIDER_RSTP) ||
       (type == NSM_BRIDGE_TYPE_PROVIDER_MSTP) || 
       (type == NSM_BRIDGE_TYPE_BACKBONE_RSTP) || 
       (type == NSM_BRIDGE_TYPE_BACKBONE_MSTP))
    {
      is_vlan_aware = 1;
    }
  else
    {
      is_vlan_aware = 0;
    }
#ifdef HAVE_HAL
  if ((ret = hal_bridge_add (name, is_vlan_aware, 
                             nsm_map_bridge_type_to_hal_protocol (type), edge, 
                             beb, macaddr)) != HAL_SUCCESS)
    return ret;
#endif /* HAVE_HAL */

  bridge = nsm_bridge_new (type, name, is_default, topology_type, edge, beb, 
                           macaddr);

  if (! bridge)
    {
#ifdef HAVE_HAL
      hal_bridge_delete(name);
#endif /* HAVE_HAL */
      return NSM_BRIDGE_ERR_MEM;
    }

  if (bridge->type == NSM_BRIDGE_TYPE_MSTP)
     bridge->max_mst_instances = NSM_BRIDGE_INSTANCE_MAX;
#ifdef HAVE_RPVST_PLUS
  else if (bridge->type == NSM_BRIDGE_TYPE_RPVST_PLUS) 
     bridge->max_mst_instances = NSM_RPVST_BRIDGE_INSTANCE_MAX;
#endif /* HAVE_RPVST_PLUS */

  /* Set master. */
  bridge->master = master;

  /* Link bridge. */
  nsm_link_bridge (master, bridge);

  /* Add Event to advertise this bridge to protocols. */
  if (! bridge->event)
    {
      bridge->event = thread_add_event (NSM_ZG, nsm_bridge_add_event,
                                        bridge, 0);
    }

#ifdef HAVE_ONMD
  nsm_oam_lldp_set_sys_cap (master->nm->vr->id,
                            NSM_LLDP_L2_SWICHING, PAL_TRUE);
#endif

#ifdef HAVE_HAL 
#ifdef HAVE_QOS
  nsm_qos_get_num_cosq(&bridge->num_cosq);
#endif /* HAVE_QOS */
#endif /* HAVE_HAL */

  return 0;
}

/* Delete bridge background event. */
static int
nsm_bridge_delete_event (struct thread *thread)
{
  struct nsm_bridge_master *master;
  struct nsm_bridge *bridge = THREAD_ARG (thread);
  int ret = 0;

  if (! bridge)
    return -1;

  master = bridge->master;
  if (! master)
    return -1;

  ret = nsm_bridge_cleanup (master, bridge);

  return ret;
}

int nsm_bridge_cleanup (struct nsm_bridge_master *master,
                        struct nsm_bridge *bridge)
{
  int i;
  struct nsm_if *zif;
  struct listnode *ln;
  struct interface *ifp;
  struct listnode *ln_next;
  struct avl_node *avl_port;
  struct avl_node *avlnext;
  struct nsm_server_entry *nse;
  struct nsm_server_client *nsc;
  struct nsm_bridge_port *br_port;
  struct nsm_bridge_listener *brlistnr;

  if ((master == NULL)
      || (bridge == NULL))
    return -1;

  /* Delete bridge interfaces from PM. */
  NSM_SERVER_CLIENT_LOOP (ng, i, nsc, nse)
    if (NSM_CHECK_CTYPE (nse->service.bits, NSM_SERVICE_BRIDGE))
      {
        nsm_bridge_sync_bridge_if (bridge->master->nm, nse, bridge,
                                   NSM_MSG_BRIDGE_DELETE_PORT);
      }

  /* Delete bridge from PM. */
  NSM_SERVER_CLIENT_LOOP (ng, i, nsc, nse)
    if (NSM_CHECK_CTYPE (nse->service.bits, NSM_SERVICE_BRIDGE))
      nsm_bridge_sync_bridge (bridge->master->nm, nse, bridge,
                              NSM_MSG_BRIDGE_DELETE);

  for (avl_port = avl_top (bridge->port_tree); avl_port; avl_port = avlnext)
    {
      avlnext = avl_next (avl_port);

      if ((br_port = (struct nsm_bridge_port *)
                             AVL_NODE_INFO (avl_port)) == NULL)
        continue;

      if ((zif = br_port->nsmif) == NULL)
        continue;

      if ((ifp = zif->ifp) == NULL)
        continue;

      nsm_bridge_if_delete(bridge, ifp, PAL_FALSE);

      AVL_NODE_INFO (avl_port) = NULL;
    }
#ifdef HAVE_I_BEB
  
  /*Deleting isids associated to i-component */
  if (bridge->backbone_edge && bridge->name[0]!='b')
    nsm_pbb_i_comp_deinit (bridge);
  
#ifdef HAVE_B_BEB
  /*Deleting TESIDs, ESPs and logical interafaces*/
  nsm_pbb_ib_bridge_deinit (bridge);
#endif /* HAVE_B_BEB */
  
#endif /* HAVE_I_BEB */

  if (bridge->port_tree)
    avl_tree_free (&bridge->port_tree, NULL);

  for (ln = LISTHEAD (bridge->bridge_listener_list); ln; ln = ln_next)
    {
      ln_next = ln->next;

      if (((brlistnr = (struct nsm_bridge_listener *)GETDATA (ln)) == NULL) ||
          (!brlistnr->delete_bridge_func))
        continue;

      (brlistnr->delete_bridge_func)(bridge->master, bridge->name);
    }

#ifdef HAVE_VLAN
  if ( nsm_bridge_is_vlan_aware(master, bridge->name) == PAL_TRUE )
    {
      nsm_vlan_deinit (bridge);
    }
#endif /* HAVE_VLAN */

  /* Unlink bridge. */
  nsm_unlink_bridge (master, bridge);

#ifdef HAVE_HAL
  /* Delete the bridge in the hardware */
  hal_bridge_delete( bridge->name );
#endif /* HAVE_HAL */

  /* Free bridge. */
  nsm_bridge_free (&bridge);

#ifdef HAVE_ONMD
  if (master->bridge_list == NULL)
    nsm_oam_lldp_set_sys_cap (master->nm->vr->id,
                              NSM_LLDP_L2_SWICHING, PAL_FALSE);
#endif

  return 0;
}

/* Delete bridge. */
int
nsm_bridge_delete (struct nsm_bridge_master *master, char *name)
{
  struct nsm_bridge *bridge;

  if (! master)
    return NSM_BRIDGE_ERR_NOTFOUND;

  bridge = nsm_lookup_bridge_by_name (master, name);
  if (! bridge)
    return NSM_BRIDGE_ERR_NOTFOUND;

  /* Launch event to delete this bridge. */
  if (bridge->event)
    {
      /* Cancel all previous events. */
      thread_cancel_event (NSM_ZG, bridge);
      bridge->event = NULL;
    }

  bridge->event = thread_add_event (NSM_ZG, nsm_bridge_delete_event,
                                    bridge, 0);

  return 0;
}
struct nsm_bridge_fdb_ifinfo *
nsm_bridge_fdb_ifinfo_lookup (struct ptree *static_fdb_list,
                              u_char *mac_addr,
                              u_int32_t ifindex)
{
  struct nsm_bridge_static_fdb *static_fdb = NULL;
  struct nsm_bridge_fdb_ifinfo *pInfo = NULL;
  struct nsm_bridge_fdb_ifinfo *pNext = NULL;
  struct ptree_node *node = NULL;

  if (static_fdb_list == NULL)
    return NULL;

  node = ptree_node_lookup (static_fdb_list, mac_addr, ETHER_MAX_BIT_LEN);

  if (NULL == node)
    return NULL;

  if (!node->info)
    return NULL;

  static_fdb = node->info;

  ptree_unlock_node (node);

  if (NULL == static_fdb->ifinfo_list)
    return NULL;

  pInfo = static_fdb->ifinfo_list;

  while (pInfo)
    {
      if (pInfo->ifindex == ifindex)
        return pInfo;
      else
        pNext = pInfo->next_if;

      pInfo = pNext;
    }

  return pInfo;
}

static struct nsm_bridge_fdb_ifinfo *
nsm_bridge_add_port_to_maclist(struct nsm_bridge *bridge,
                               struct nsm_bridge_static_fdb *static_fdb,
                               u_int32_t ifindex, int type, bool_t is_forward,
                               u_int16_t vid)
{
  struct nsm_bridge_fdb_ifinfo *pInfo;
  struct nsm_bridge_fdb_ifinfo *list_head;

  if (!static_fdb)
    return NULL;

  list_head = static_fdb->ifinfo_list;

  if (!list_head)
    {
      list_head = XCALLOC(MTYPE_NSM_BRIDGE_FDB_IFINFO,
                          sizeof(struct nsm_bridge_fdb_ifinfo));
      if (list_head)
        {
          list_head->ifindex = ifindex;
          list_head->type = type;
          list_head->is_forward = is_forward;
#ifdef HAVE_HA
          nsm_bridge_cal_create_nsm_bridge_mac (list_head, bridge,
                                                static_fdb, vid);
#endif /* HAVE_HA */
          return list_head;
        }
    }

  if (!(pInfo = list_head))
    return NULL;

  while (pInfo)
    {
      if (pInfo->ifindex == ifindex)
        {
          /* Maybe we're just changing forwarding info and type */
          pInfo->is_forward = is_forward;
          pInfo->type = type;
          return list_head;
        }
      pInfo = pInfo->next_if;
    }

  /* Port info not found add new */
  pInfo = XCALLOC(MTYPE_NSM_BRIDGE_FDB_IFINFO,
                  sizeof(struct nsm_bridge_fdb_ifinfo));
  if ( !pInfo )
    return NULL;

  /* Always add at head */
  pInfo->ifindex = ifindex;
  pInfo->type = type;
  pInfo->is_forward = is_forward;
  pInfo->next_if = list_head;
  pInfo->next_if->prev_if = pInfo;
  list_head = pInfo;

#ifdef HAVE_HA
  nsm_bridge_cal_create_nsm_bridge_mac (pInfo, bridge, static_fdb, vid);
#endif /* HAVE_HA */

  return list_head;
}

static struct nsm_bridge_fdb_ifinfo *
nsm_bridge_del_port_from_maclist(struct nsm_bridge *bridge,
                                 struct nsm_bridge_static_fdb *static_fdb,
                                 u_int32_t ifindex)
{
  struct nsm_bridge_fdb_ifinfo *pInfo;
  struct nsm_bridge_fdb_ifinfo *pNext;
  struct nsm_bridge_fdb_ifinfo *list_head;

  if (!static_fdb)
    return NULL;

  list_head = static_fdb->ifinfo_list;

  if (!(pInfo = list_head))
    return NULL;

  while (pInfo)
    {
      if (pInfo->ifindex == ifindex)
        {
          if (pInfo->next_if)
            pInfo->next_if->prev_if = pInfo->prev_if;
          if (pInfo->prev_if)
            pInfo->prev_if->next_if = pInfo->next_if;

          pNext = pInfo->next_if;
          if (pInfo == list_head)
            list_head = pNext;

#ifdef HAVE_HA
          nsm_bridge_cal_delete_nsm_bridge_mac (pInfo, bridge, static_fdb);
#endif /* HAVE_HA */
          XFREE (MTYPE_NSM_BRIDGE_FDB_IFINFO, pInfo);
        }
      else
        pNext = pInfo->next_if;

      pInfo = pNext;
    }
  return list_head;
}

static struct nsm_bridge_static_fdb *
nsm_bridge_static_fdb_new (struct nsm_bridge *bridge,  u_int32_t ifindex,
                           u_int8_t *mac, bool_t is_forward,
                           int type, u_int16_t vid)
{

  struct nsm_bridge_static_fdb *static_fdb = NULL;
  static_fdb = XCALLOC (MTYPE_NSM_BRIDGE_STATIC_FDB,
                        sizeof (struct nsm_bridge_static_fdb) );

  if (!static_fdb)
    return NULL;

  pal_mem_cpy (static_fdb->mac_addr, mac, ETHER_ADDR_LEN);
  if ( !(static_fdb->ifinfo_list =
         nsm_bridge_add_port_to_maclist(bridge, static_fdb,
                                        ifindex, type, is_forward, vid)) )
    {
      XFREE (MTYPE_NSM_BRIDGE_STATIC_FDB, static_fdb);
      return NULL;
    }

  return static_fdb;
}

int
nsm_check_duplicate_unicast_mac (struct ptree *list, u_int32_t ifindex,
                                 u_int8_t *mac)
{
  struct ptree_node *node = NULL;
  struct nsm_bridge_static_fdb *static_fdb = NULL;

  if ( !list )
    {
      return RESULT_ERROR;
    }

  node = ptree_node_lookup (list, mac, ETHER_MAX_BIT_LEN);

  if (NULL == node)
    return RESULT_OK;

  if (!node->info)
    return RESULT_OK;

  static_fdb = node->info;

  ptree_unlock_node (node);

  if (NULL == static_fdb->ifinfo_list)
    return RESULT_OK;

  if ( !(mac[0] & 0x01) && (static_fdb->ifinfo_list->ifindex != ifindex) )
    return NSM_ERR_DUPLICATE_MAC;

  return RESULT_OK;
}

int
nsm_add_mac_to_static_list (struct nsm_bridge *bridge, struct ptree *list,
                            u_int32_t ifindex, u_int8_t *mac,
                            bool_t is_forward, int type, u_int16_t vid)
{
  struct ptree_node *node = NULL;
  struct nsm_bridge_static_fdb *static_fdb = NULL;

  if (bridge == NULL)
    return RESULT_ERROR;

  if (list == NULL)
    return RESULT_ERROR;

  node = ptree_node_lookup (list, mac, ETHER_MAX_BIT_LEN);

  if (node == NULL)
    {
      node = ptree_node_get (list, mac, ETHER_MAX_BIT_LEN);

      if (node == NULL)
        return RESULT_ERROR;

      static_fdb = nsm_bridge_static_fdb_new (bridge, ifindex, mac,
                                              is_forward, type, vid);
      if (!static_fdb)
        return RESULT_ERROR;

      /* Default Status should be Permanent for Static Entries */
      static_fdb->snmp_status = DOT1DPORTSTATICSTATUS_PERMANENT;
      node->info = static_fdb;
    }
  else
    {
      if (!node->info)
        {
          static_fdb = nsm_bridge_static_fdb_new (bridge, ifindex, mac,
                                                  is_forward, type, vid);
          if (!static_fdb)
            return RESULT_ERROR;

	  /* Default Status should be Permanent for Static Entries */
	  static_fdb->snmp_status = DOT1DPORTSTATICSTATUS_PERMANENT;
          node->info = static_fdb;
        }
      else
        {
          static_fdb = node->info;
          /* Add port */
          if (!(static_fdb->ifinfo_list =
                nsm_bridge_add_port_to_maclist(bridge, static_fdb,
                                               ifindex, type, is_forward, vid)))
            return RESULT_ERROR;
        }
    }

  return RESULT_OK;
}

static void
nsm_del_mac_from_static_list (struct nsm_bridge *bridge,
                              struct ptree *list,
                              u_int32_t ifindex,
                              u_int8_t *mac)
{
  struct ptree_node *node = NULL;
  struct nsm_bridge_static_fdb *static_fdb = NULL;

  if ((bridge == NULL)
      || (list == NULL))
    return;

  node = ptree_node_lookup (list, mac, ETHER_MAX_BIT_LEN);

  if ( !node )
    return;

  static_fdb = node->info;

  if ( !static_fdb )
    return;

  /* Delete from port */
  static_fdb->ifinfo_list = nsm_bridge_del_port_from_maclist (bridge,
                                                              static_fdb, ifindex);

  if (!static_fdb->ifinfo_list)
    {
      XFREE (MTYPE_NSM_BRIDGE_STATIC_FDB, static_fdb);
      node->info = NULL;
    }
  return;
}

/* Internal function for syncing multicast fdb entries for a port. */
static int
_nsm_bridge_sync_multicast_entries (struct nsm_bridge *bridge, struct interface *ifp,
                                    struct ptree *tree,
                                    u_int16_t vid,
                                    u_int16_t svid,
                                    u_int8_t add)
{
  struct nsm_bridge_static_fdb *static_fdb = NULL;
  struct nsm_bridge_fdb_ifinfo *pInfo = NULL;
  struct ptree_node *pn;

  if (! bridge || ! tree)
    return -1;

  /* Scan for all multicast entries. */
  for (pn = ptree_top (tree); pn; pn = ptree_next (pn))
    {
      static_fdb = pn->info;
      if (! static_fdb)
        continue;

      /* If not multicast entry, ignore. */
      if (! static_fdb->mac_addr[0] & 0x1)
        continue;

      pInfo = static_fdb->ifinfo_list;
      while (pInfo)
        {
#ifdef HAVE_HAL
          /* If entry exists for this interface, resync it. */
          if (pInfo->ifindex == ifp->ifindex)
            {
              if (add)
                hal_l2_add_fdb (bridge->name, pInfo->ifindex, static_fdb->mac_addr, HAL_HW_LENGTH,
                                vid, svid, HAL_L2_FDB_STATIC, pInfo->is_forward);
              else
                hal_l2_del_fdb (bridge->name, pInfo->ifindex, static_fdb->mac_addr,
                                HAL_HW_LENGTH, vid, svid, HAL_L2_FDB_STATIC);
            }
#endif /* HAVE_HAL */

          /* Goto next interface. */
          pInfo = pInfo->next_if;
        }
    }

  return 0;
}

#ifdef HAVE_VLAN
/* Internal function for syncing VLANs. */
static int
_nsm_bridge_sync_multicast_entries_by_type (struct nsm_bridge *bridge,
                                            struct interface *ifp,
                                            struct nsm_vlan_bmp *bmp,
                                            u_int8_t add)
{
  struct nsm_bridge_port *br_port = NULL;
  struct nsm_vlan_port *vlan_port;
  struct avl_tree *vlan_table;
  struct avl_node *rn;
  struct nsm_vlan *vlan;
  struct nsm_vlan nvlan;
  struct nsm_if *zif;
  u_int16_t vid;

  zif = (struct nsm_if *) ifp->info;

  if (! zif)
    return 0;

  if (zif == NULL)
    return 0;

  br_port = zif->switchport;

  if (br_port == NULL)
    return 0;

  vlan_port = &br_port->vlan_port;

  vlan_table = vlan_port->mode == NSM_VLAN_PORT_MODE_CN
               || vlan_port->mode == NSM_VLAN_PORT_MODE_PN ?
               bridge->svlan_table : bridge->vlan_table;

  if (! vlan_table)
    return 0;

  /* Process static VLANs. */
  NSM_VLAN_SET_BMP_ITER_BEGIN (((struct nsm_vlan_bmp) (*bmp)), vid)
    {
      NSM_VLAN_SET (&nvlan, vid);
      rn = avl_search (vlan_table, (void *) &nvlan);
      if (rn)
        {
          vlan = rn->info;
          if (!vlan)
            continue;

          /* Resync for this vlan. */
          _nsm_bridge_sync_multicast_entries (bridge, ifp,
                                              vlan->static_fdb_list,
                                              vid, vid, add);

        }
    }
  NSM_VLAN_SET_BMP_ITER_END (((struct nsm_vlan_bmp) (*bmp)), vid);

  return 0;
}
#endif /* HAVE_VLAN */

/* Sync the multicast entries for a port. */
int
nsm_bridge_sync_multicast_entries (struct nsm_bridge *bridge, struct interface *ifp, u_int8_t add)
{
  u_int8_t is_vlan_aware;
#ifdef HAVE_VLAN
  struct nsm_bridge_port *br_port = NULL;
  struct nsm_vlan_port *vlan_port;
  struct avl_tree *vlan_table;
  struct nsm_if *zif;
#ifdef HAVE_PROVIDER_BRIDGE
  struct avl_node *an;
  struct nsm_pro_edge_sw_ctx *ctx = NULL;
#endif /* HAVE_PROVIDER_BRIDGE */
#endif /* HAVE_VLAN */

  /* NULL parameter check. */
  if (! bridge || !ifp)
    return -1;

  /* Is bridge vlan-aware? */
  is_vlan_aware = NSM_BRIDGE_VLAN_AWARE (bridge);

#ifdef HAVE_VLAN

  zif = (struct nsm_if *) ifp->info;

  if (zif == NULL)
    return -1;

  br_port = zif->switchport;

  if (br_port == NULL)
    return -1;

#ifdef HAVE_PROVIDER_BRIDGE
  if (bridge->provider_edge == PAL_TRUE)
    return 0;
#endif /* HAVE_PROVIDER_EDGE */

  vlan_port = &br_port->vlan_port;

  vlan_table = vlan_port->mode == NSM_VLAN_PORT_MODE_CN
               || vlan_port->mode == NSM_VLAN_PORT_MODE_PN ?
               bridge->svlan_table : bridge->vlan_table;
#endif

#ifdef HAVE_PROVIDER_BRIDGE

  if (bridge->provider_edge == PAL_TRUE)
    {
      for (an = avl_top (bridge->pro_edge_swctx_table); an; an = avl_next (an))
        {
          if ((ctx = an->info))
            _nsm_bridge_sync_multicast_entries (bridge, ifp, ctx->static_fdb_list,
                                                ctx->cvid, ctx->svid, add);
        }

      return 0;
    }

#endif /* HAVE_PROVIDER_BRIDGE */

  switch (is_vlan_aware)
    {
    case 0: /* Non-VLAN aware. */
      /* Resync for this vlan. */
      _nsm_bridge_sync_multicast_entries (bridge, ifp, bridge->static_fdb_list,
                                          NSM_VLAN_NULL_VID, NSM_VLAN_NULL_VID,
                                          add);

      break;
#ifdef HAVE_VLAN
    case 1: /* VLAN aware. */
      {
        if (! vlan_table)
          return -1;

        /* Process entries added for static VLANs. */
        _nsm_bridge_sync_multicast_entries_by_type (bridge, ifp, &vlan_port->staticMemberBmp, add);

        /* Process entries added for dynamic VLANs. */
        _nsm_bridge_sync_multicast_entries_by_type (bridge, ifp, &vlan_port->dynamicMemberBmp, add);
      }
      break;
#endif /* HAVE_VLAN */
    default:
      return -1;
    }

  return 0;
}

void
nsm_bridge_free_static_list ( struct ptree *list )
{
  struct ptree_node *pn = NULL;
  struct nsm_bridge_static_fdb *static_fdb = NULL;
  struct nsm_bridge_fdb_ifinfo *pInfo = NULL;
  struct nsm_bridge_fdb_ifinfo *pNext = NULL;

  if (!list)
    return;

  for (pn = ptree_top (list); pn; pn = ptree_next (pn))
    {
      if (! (static_fdb = pn->info) )
        continue;
      pInfo = static_fdb->ifinfo_list;
      while (pInfo)
        {
          pNext = pInfo->next_if;
          XFREE (MTYPE_NSM_BRIDGE_FDB_IFINFO, pInfo);
          pInfo = pNext;
        }
      XFREE (MTYPE_NSM_BRIDGE_STATIC_FDB, static_fdb);
      pn->info = NULL;
    }
}

static int
nsm_bridge_delete_fdb (struct nsm_bridge *bridge, struct nsm_bridge_static_fdb *static_fdb,
                       u_int16_t vid, u_int16_t svid,
                       nsm_bridge_delete_mac_from_port_func_t should_delete_mac_from_port,
                       struct nsm_bridge_fdb_ifinfo *cmp)
{
  struct nsm_bridge_fdb_ifinfo *pInfo = NULL;
  struct nsm_bridge_fdb_ifinfo *pNext = NULL;

  if (!static_fdb || !should_delete_mac_from_port)
    return 0;

  pInfo = static_fdb->ifinfo_list;

  while (pInfo)
    {
      pNext = pInfo->next_if;
      if (should_delete_mac_from_port (pInfo, cmp) != PAL_TRUE)
        {
          pInfo = pNext;
          continue;
        }
#ifdef HAVE_HAL
      if (hal_l2_del_fdb (bridge->name, pInfo->ifindex, static_fdb->mac_addr,
                          HAL_HW_LENGTH, vid, svid, HAL_L2_FDB_STATIC)
                          != HAL_SUCCESS)
        return NSM_HAL_FDB_ERR;
#endif /* HAVE_HAL */

      if (pInfo->next_if)
        pInfo->next_if->prev_if = pInfo->prev_if;
      if (pInfo->prev_if)
        pInfo->prev_if->next_if = pInfo->next_if;
      if (pInfo ==  static_fdb->ifinfo_list)
        static_fdb->ifinfo_list = pNext;

#ifdef HAVE_HA
      nsm_bridge_cal_delete_nsm_bridge_mac (pInfo, bridge, static_fdb);
#endif /* HAVE_HA */

      XFREE (MTYPE_NSM_BRIDGE_FDB_IFINFO, pInfo);
      pInfo = pNext;
    }

  return 0;
}

#ifdef HAVE_PROVIDER_BRIDGE

void
nsm_bridge_delete_port_fdb_by_vid_svid (struct nsm_bridge *bridge, struct nsm_if *zif,
                                        u_int16_t cvid, u_int16_t svid)
{
  struct ptree_node *pn = NULL;
  struct nsm_pro_edge_sw_ctx *ctx;
  struct nsm_vlan_port *vlan_port = NULL;
  struct nsm_bridge_port *br_port = NULL;
  struct ptree      *list = NULL;
  struct nsm_bridge_static_fdb *static_fdb = NULL;

  if (!bridge)
    return;

  if (zif == NULL || zif->ifp == NULL || zif->switchport == NULL)
    return;

  br_port = zif->switchport;

  vlan_port = &br_port->vlan_port;

  ctx = nsm_pro_edge_swctx_lookup (bridge, cvid, svid);

  if (! ctx)
    return;

  list = ctx->static_fdb_list;

  for (pn = ptree_top (list); pn; pn = ptree_next (pn))
    {
      if (! (static_fdb = pn->info) )
        continue;

      static_fdb->ifinfo_list = nsm_bridge_del_port_from_maclist
                                           (bridge, static_fdb, zif->ifp->ifindex);

#ifdef HAVE_HAL
      hal_l2_del_fdb(bridge->name, zif->ifp->ifindex, static_fdb->mac_addr,
                     HAL_HW_LENGTH, cvid, svid, HAL_L2_FDB_STATIC);
#endif /* HAVE_HAL */


      if (!static_fdb->ifinfo_list)
        {
          XFREE (MTYPE_NSM_BRIDGE_STATIC_FDB, static_fdb);
          pn->info = NULL;
        }
    }

  return;

}

#endif /* HAVE_PROVIDER_BRIDGE */

void
nsm_bridge_delete_port_fdb_by_vid (struct nsm_bridge *bridge, struct nsm_if *zif,
                                   u_int16_t vid)
{
  struct ptree_node *pn = NULL;
#ifdef HAVE_VLAN
  struct nsm_vlan nvlan;
  struct nsm_vlan *vlan = NULL;
  struct avl_node *rn   = NULL;
  struct avl_tree *vlan_table = NULL;
  struct nsm_vlan_port *vlan_port = NULL;
  struct nsm_bridge_port *br_port = NULL;
#endif /* HAVE_VLAN */
  struct ptree      *list = NULL;
  struct nsm_bridge_static_fdb *static_fdb = NULL;

  if (!bridge)
    return;

#ifdef HAVE_VLAN

  if (zif == NULL || zif->ifp == NULL || zif->switchport == NULL)
    return;

  br_port = zif->switchport;

  vlan_port = &br_port->vlan_port;

  vlan_table = vlan_port->mode == NSM_VLAN_PORT_MODE_CN
               || vlan_port->mode == NSM_VLAN_PORT_MODE_PN ?
               bridge->svlan_table : bridge->vlan_table;

  if (! vlan_table)
    return;

  NSM_VLAN_SET(&nvlan, vid);

  rn = avl_search (vlan_table, (void *)&nvlan);

  if ( (!rn) || !(vlan = rn->info) || !(list = vlan->static_fdb_list) )
    return;

#endif /* HAVE_VLAN */

  for (pn = ptree_top (list); pn; pn = ptree_next (pn))
    {
      if (! (static_fdb = pn->info) )
        continue;

      static_fdb->ifinfo_list = nsm_bridge_del_port_from_maclist
                                           (bridge, static_fdb, zif->ifp->ifindex);

#ifdef HAVE_HAL
      hal_l2_del_fdb(bridge->name, zif->ifp->ifindex, static_fdb->mac_addr, HAL_HW_LENGTH,
                     vid, vid, HAL_L2_FDB_STATIC);
#endif /* HAVE_HAL */


      if (!static_fdb->ifinfo_list)
        {
          XFREE (MTYPE_NSM_BRIDGE_STATIC_FDB, static_fdb);
          pn->info = NULL;
        }
    }

  return;

}

void
nsm_bridge_delete_all_fdb_by_port ( struct nsm_bridge *bridge, struct nsm_if *zif,
                                    int hw_del_only)
{
  int is_vlan_aware = PAL_FALSE;
  struct ptree      *list = NULL;
  struct ptree_node *pn   =   NULL;
  struct nsm_bridge_static_fdb *static_fdb = NULL;
#ifdef HAVE_VLAN
#ifdef HAVE_PROVIDER_BRIDGE
  struct nsm_pro_edge_sw_ctx *ctx;
#endif /* HAVE_PROVIDER_BRIDGE */
  struct avl_node *rn   = NULL;
  struct avl_tree *vlan_table   = NULL;
  struct nsm_vlan *vlan = NULL;
  struct nsm_vlan_port *vlan_port = NULL;
  struct nsm_bridge_port *br_port = NULL;
#endif /* HAVE_VLAN */

  if (bridge == NULL
      || zif == NULL
      || zif->ifp == NULL
      || zif->switchport == NULL)
    return;

  is_vlan_aware =  NSM_BRIDGE_VLAN_AWARE (bridge);

#ifdef HAVE_VLAN

  br_port = zif->switchport;

  vlan_port = &br_port->vlan_port;

  vlan_table = vlan_port->mode == NSM_VLAN_PORT_MODE_CN
               || vlan_port->mode == NSM_VLAN_PORT_MODE_PN ?
               bridge->svlan_table : bridge->vlan_table;

#endif

  if ( !is_vlan_aware )
    {
      if (! (list = bridge->static_fdb_list))
        return;

      for (pn = ptree_top (list); pn; pn = ptree_next (pn))
        {
          if (! (static_fdb = pn->info) )
            continue;

          if (hw_del_only == PAL_FALSE)
            static_fdb->ifinfo_list = nsm_bridge_del_port_from_maclist
                                      (bridge, static_fdb,
                                       zif->ifp->ifindex);
#ifdef HAVE_HAL
          hal_l2_del_fdb(bridge->name, zif->ifp->ifindex, static_fdb->mac_addr, HAL_HW_LENGTH,
                         NSM_VLAN_NULL_VID, NSM_VLAN_NULL_VID, HAL_L2_FDB_STATIC);
#endif /* HAVE_HAL */
          if (hw_del_only == PAL_FALSE)
            if (!static_fdb->ifinfo_list)
              {
                XFREE (MTYPE_NSM_BRIDGE_STATIC_FDB, static_fdb);
                pn->info = NULL;
              }
        }
    }
  else
    {
#ifdef HAVE_VLAN
#ifdef HAVE_PROVIDER_BRIDGE
      if (bridge->provider_edge == PAL_TRUE)
        {
          if (! bridge->pro_edge_swctx_table)
            return;

          for (rn = avl_top (bridge->pro_edge_swctx_table); rn;
               rn = avl_next (rn))
            {
              if (!(ctx = rn->info) || !(list = ctx->static_fdb_list) )
                continue;

              for (pn = ptree_top (list); pn; pn = ptree_next (pn))
                {
                  if (! (static_fdb = pn->info) )
                    continue;

                  if (hw_del_only == PAL_FALSE)
                    static_fdb->ifinfo_list =
                      nsm_bridge_del_port_from_maclist (bridge, static_fdb,
                                                        zif->ifp->ifindex);
#ifdef HAVE_HAL
                  hal_l2_del_fdb(bridge->name, zif->ifp->ifindex,
                                 static_fdb->mac_addr, HAL_HW_LENGTH,
                                 ctx->cvid, ctx->svid, HAL_L2_FDB_STATIC);
#endif /* HAVE_HAL */

                  if (hw_del_only == PAL_FALSE
                      && !static_fdb->ifinfo_list)
                    {
                      XFREE (MTYPE_NSM_BRIDGE_STATIC_FDB, static_fdb);
                      pn->info = NULL;
                    }
                }
            }
        }
      else
#endif /* HAVE_PROVIDER_BRIDGE */
        {
          if (! vlan_table)
            return;

          for (rn = avl_top (vlan_table); rn; rn = avl_next (rn))
            {
              if (!(vlan = rn->info) || !(list = vlan->static_fdb_list) )
                continue;

              for (pn = ptree_top (list); pn; pn = ptree_next (pn))
                {
                  if (! (static_fdb = pn->info) )
                    continue;

                  static_fdb->ifinfo_list =
                    nsm_bridge_del_port_from_maclist (bridge, static_fdb,
                                                      zif->ifp->ifindex);
#ifdef HAVE_HAL
                  hal_l2_del_fdb(bridge->name, zif->ifp->ifindex,
                                 static_fdb->mac_addr, HAL_HW_LENGTH,
                                 vlan->vid, vlan->vid, HAL_L2_FDB_STATIC);
#endif /* HAVE_HAL */

                  if (hw_del_only == PAL_FALSE
                      && !static_fdb->ifinfo_list)
                    {
                      XFREE (MTYPE_NSM_BRIDGE_STATIC_FDB, static_fdb);
                      pn->info = NULL;
                    }
                }
            }
        }
#endif /* HAVE_VLAN */
    }
}

/* Add the received MAC from nsm configuration to the hardware fdb*/
void
nsm_bridge_add_static_fdb (struct nsm_bridge *bridge, u_int32_t ifindex)
{
  int is_vlan_aware = PAL_FALSE;
  struct ptree_node *pn = NULL;
  struct ptree *list = NULL;
  struct nsm_bridge_static_fdb *static_fdb = NULL;
#ifdef HAVE_VLAN
  struct avl_node *rn = NULL;
  struct nsm_vlan *vlan = NULL;
#endif /* HAVE_VLAN */

  if (!bridge)
    return;

  is_vlan_aware = NSM_BRIDGE_VLAN_AWARE (bridge);

  if (! is_vlan_aware)
    {
      if (! (list = bridge->static_fdb_list))
        return;

      for (pn = ptree_top (list); pn; pn = ptree_next (pn))
        {
          if (! (static_fdb = pn->info))
            continue;

#ifdef HAVE_HAL
          hal_l2_add_fdb (bridge->name, ifindex, static_fdb->mac_addr,
                          HAL_HW_LENGTH, NSM_VLAN_NULL_VID,
                          NSM_VLAN_NULL_VID, HAL_L2_FDB_STATIC,
                          static_fdb->ifinfo_list->is_forward);
#endif /* HAVE_HAL */
        }
    }
  else
    {
#ifdef HAVE_VLAN
      if (! bridge->vlan_table)
        return;
      for (rn = avl_top (bridge->vlan_table); rn; rn = avl_next (rn))
        {
          if (!(vlan = rn->info) || !(list = vlan->static_fdb_list) )
            continue;

          for (pn = ptree_top (list); pn; pn = ptree_next (pn))
            {
              if (! (static_fdb = pn->info) )
                continue;

#ifdef HAVE_HAL
              hal_l2_add_fdb (bridge->name, ifindex, static_fdb->mac_addr,
                              HAL_HW_LENGTH, vlan->vid, vlan->vid,
                              HAL_L2_FDB_STATIC,
                              static_fdb->ifinfo_list->is_forward);
#endif /* HAVE_HAL */
            }
        }
#endif /* HAVE_VLAN */
    }
}

static int
nsm_bridge_delete_static (struct nsm_bridge_static_fdb *curr,
                          struct nsm_bridge_static_fdb *cmp)
{
  return PAL_TRUE;
}

static int
nsm_bridge_delete_all_static_from_port (struct nsm_bridge_fdb_ifinfo *curr,
                                        struct nsm_bridge_fdb_ifinfo *cmp)
{
  if (!curr)
    return PAL_FALSE;

  return (curr->type == CLI_MAC );
}

static int
nsm_bridge_delete_static_by_ifindex (struct nsm_bridge_fdb_ifinfo *curr,
                                     struct nsm_bridge_fdb_ifinfo *cmp)
{
  if (!curr || !cmp)
    return PAL_FALSE;

  return ( (curr->type == CLI_MAC ) && (curr->ifindex == cmp->ifindex) );
}

static int
nsm_bridge_delete_multicast (struct nsm_bridge_static_fdb *curr,
                             struct nsm_bridge_static_fdb *cmp)
{

  if (!curr)
    return PAL_FALSE;

  return ( (curr->mac_addr[0]) & 1);
}

static int
nsm_bridge_should_delete_static_mac (struct nsm_bridge_static_fdb *curr,
                                     struct nsm_bridge_static_fdb *cmp)
{

  if (!curr || !cmp)
    return PAL_FALSE;

  return (!pal_mem_cmp(curr->mac_addr, cmp->mac_addr, ETHER_ADDR_LEN));

}

static int
nsm_bridge_should_delete_multicast_mac (struct nsm_bridge_static_fdb *curr,
                                        struct nsm_bridge_static_fdb *cmp)
{

  if (!curr || !cmp)
    return PAL_FALSE;

  return ( ((curr->mac_addr[0]) & 1) &&
           (!pal_mem_cmp(curr->mac_addr, cmp->mac_addr, ETHER_ADDR_LEN)) );

}


static int
nsm_bridge_delete_all_multicast_from_port (struct nsm_bridge_fdb_ifinfo *curr,
                                           struct nsm_bridge_fdb_ifinfo *cmp)
{
  return PAL_TRUE;
}

static int
nsm_bridge_delete_multicast_by_ifindex (struct nsm_bridge_fdb_ifinfo *curr,
                                        struct nsm_bridge_fdb_ifinfo *cmp)
{
  if (!cmp || !curr)
    return PAL_FALSE;

  return (curr->ifindex == cmp->ifindex);
}


static int
nsm_bridge_delete_all_fdb ( struct nsm_bridge *bridge,
                            nsm_bridge_delete_mac_func_t should_delete_mac_func,
                            nsm_bridge_delete_mac_from_port_func_t should_delete_mac_from_port,
                            struct nsm_bridge_fdb_ifinfo *cmp,
                            struct nsm_bridge_static_fdb *fdbcmp)
{
  int is_vlan_aware = PAL_FALSE;
  int ret = 0;
  struct ptree_node *pn   =   NULL;
  struct ptree      *list = NULL;
  struct nsm_bridge_static_fdb *static_fdb = NULL;
#ifdef HAVE_VLAN
#ifdef HAVE_PROVIDER_BRIDGE
  struct nsm_pro_edge_sw_ctx *ctx;
#endif /* HAVE_PROVIDER_BRIDGE */
  struct avl_tree *vlan_table = NULL;
  struct avl_node *rn   = NULL;
  struct nsm_vlan *vlan = NULL;
#endif /* HAVE_VLAN */

  if (!bridge || !should_delete_mac_func)
    return NSM_BRIDGE_ERR_NOTFOUND;

  is_vlan_aware =  NSM_BRIDGE_VLAN_AWARE (bridge);

#ifdef HAVE_VLAN
  if (NSM_BRIDGE_TYPE_PROVIDER (bridge))
     vlan_table = bridge->svlan_table;
  else
     vlan_table = bridge->vlan_table;
#endif

  if ( !is_vlan_aware )
    {
      if (! (list = bridge->static_fdb_list))
        return 0;

      for (pn = ptree_top (list); pn; pn = ptree_next (pn))
        {
          if (  (!(static_fdb = pn->info)) ||
                (should_delete_mac_func(static_fdb, fdbcmp) == PAL_FALSE) )
            continue;

          if ( (ret = nsm_bridge_delete_fdb (bridge, static_fdb,
                                             NSM_VLAN_NULL_VID,
                                             NSM_VLAN_NULL_VID,
                                             should_delete_mac_from_port,
                                             cmp))
               < 0)
            return ret;

          if (!static_fdb->ifinfo_list)
            {
              XFREE (MTYPE_NSM_BRIDGE_STATIC_FDB, static_fdb);
              pn->info = NULL;
            }
        }
    }
  else
    {
#ifdef HAVE_VLAN
#ifdef HAVE_PROVIDER_BRIDGE
      if (bridge->provider_edge == PAL_TRUE)
        {
          if (! bridge->pro_edge_swctx_table)
            return NSM_VLAN_ERR_VLAN_NOT_FOUND;

          for (rn = avl_top (bridge->pro_edge_swctx_table); rn;
               rn = avl_next (rn))
            {
              if (!(ctx = rn->info) || !(list = ctx->static_fdb_list) )
                continue;

              for (pn = ptree_top (list); pn; pn = ptree_next (pn))
                {
                  if (! (static_fdb = pn->info) ||
                      (should_delete_mac_func(static_fdb, fdbcmp) == PAL_FALSE))
                    continue;

                  if ( (ret = nsm_bridge_delete_fdb (bridge,
                                                     static_fdb,
                                                     ctx->cvid, ctx->svid,
                                                     should_delete_mac_from_port,
                                                     cmp))
                       < 0)
                    return ret;

                  if (!static_fdb->ifinfo_list)
                    {
                      XFREE (MTYPE_NSM_BRIDGE_STATIC_FDB, static_fdb);
                      pn->info = NULL;
                    }
                }
            }
          }
      else
#endif /* HAVE_PROVIDER_BRIDGE */
        {
          if (! vlan_table)
            return NSM_VLAN_ERR_VLAN_NOT_FOUND;

          for (rn = avl_top (vlan_table); rn; rn = avl_next (rn))
            {
              if (!(vlan = rn->info) || !(list = vlan->static_fdb_list) )
                continue;

              for (pn = ptree_top (list); pn; pn = ptree_next (pn))
                {
                  if (! (static_fdb = pn->info) ||
                      (should_delete_mac_func(static_fdb, fdbcmp) == PAL_FALSE))
                    continue;

                  if ( (ret = nsm_bridge_delete_fdb (bridge,
                                                     static_fdb, vlan->vid,
                                                     vlan->vid,
                                                     should_delete_mac_from_port,
                                                     cmp))
                       < 0)
                    return ret;

                  if (!static_fdb->ifinfo_list)
                    {
                      XFREE (MTYPE_NSM_BRIDGE_STATIC_FDB, static_fdb);
                      pn->info = NULL;
                    }
                }
            }
          }
#endif /* HAVE_VLAN */
    }

  return 0;
}

int nsm_is_mac_present ( struct nsm_bridge *bridge, u_int8_t *mac_addr,
                         u_int16_t vid, u_int16_t svid, int type,
                         int ifindex)
{
  struct ptree_node *node = NULL;
#ifdef HAVE_VLAN
#ifdef HAVE_PROVIDER_BRIDGE
  struct nsm_pro_edge_sw_ctx *ctx;
#endif /* HAVE_PROVIDER_BRIDGE */
  struct nsm_vlan *vlan = NULL;
  struct avl_node *rn = NULL;
  struct nsm_vlan nvlan;
#endif /* HAVE_VLAN */
  struct nsm_bridge_static_fdb *static_fdb = NULL;
  struct nsm_bridge_fdb_ifinfo *pInfo = NULL;

#ifdef HAVE_VLAN

  if ( NSM_BRIDGE_VLAN_AWARE (bridge) )
    {

#ifdef HAVE_PROVIDER_BRIDGE
      if (bridge->provider_edge == PAL_TRUE)
        {
          ctx = nsm_pro_edge_swctx_lookup (bridge, vid, svid);

          if (!ctx || !ctx->static_fdb_list )
            return PAL_FALSE;

          node = ptree_node_lookup (ctx->static_fdb_list, mac_addr,
                                    ETHER_MAX_BIT_LEN);
        }
      else
#endif
        {
          NSM_VLAN_SET(&nvlan, vid);
          rn = avl_search (bridge->vlan_table, (void *)&nvlan);

          if (!rn)
            return PAL_FALSE;
          vlan = rn->info;

          if (!vlan || !vlan->static_fdb_list )
            return PAL_FALSE;

          node = ptree_node_lookup (vlan->static_fdb_list, mac_addr,
                                    ETHER_MAX_BIT_LEN);
        }

      if ( (!node) || (!node->info) )
        {
          return PAL_FALSE;
        }
      else
        {
          static_fdb = node->info;
          pInfo = static_fdb->ifinfo_list;
          while (pInfo)
            {
              if (ifindex < 0)
                {
                  if (pInfo->type == type)
                    return PAL_TRUE;
                }
              else
                {
                  if ((pInfo->type == type) && (pInfo->ifindex == ifindex))
                    return PAL_TRUE;
                }

              pInfo = pInfo->next_if;
            }

          return PAL_FALSE;
        }
    }
  else
#endif /* HAVE_VLAN */
    {
      if ( !bridge->static_fdb_list )
        return PAL_FALSE;

      node = ptree_node_lookup (bridge->static_fdb_list, mac_addr, ETHER_MAX_BIT_LEN);

      if ( (!node) || (!node->info) )
        {
          return PAL_FALSE;
        }
      else
        {
          static_fdb = node->info;
          pInfo = static_fdb->ifinfo_list;
          while (pInfo)
            {
              if (ifindex < 0)
                {
                  if (pInfo->type == type)
                    return PAL_TRUE;
                }
              else
                {
                  if ((pInfo->type == type) && (pInfo->ifindex == ifindex))
                    return PAL_TRUE;
                }

              pInfo = pInfo->next_if;
            }

          return PAL_FALSE;
        }
    }

  return PAL_FALSE;
}

#ifdef HAVE_PVLAN
static int
nsm_pvlan_add_mac (struct nsm_bridge *bridge, struct interface *ifp,
                   u_int8_t *mac, u_int16_t vid, unsigned char flags,
                   bool_t is_forward, int type)
{
  struct nsm_vlan_port *vlan_port;
  struct nsm_bridge_port *br_port = NULL;
  struct nsm_vlan *secondary_vlan, *primary_vlan;
  struct nsm_vlan *vlan;
  u_int16_t secondary_vid, primary_vid;
  struct nsm_vlan nvlan;
  struct avl_tree *vlan_table;
  struct avl_node *rn;
  struct nsm_if *zif;
  int ret;

  if (!bridge || !ifp)
    return NSM_BRIDGE_ERR_NOT_BOUND;

  zif = (struct nsm_if *)ifp->info;

  if (zif == NULL || zif->switchport == NULL)
    return NSM_BRIDGE_ERR_NOT_BOUND;

  br_port = zif->switchport;

  vlan_port = &br_port->vlan_port;

  if (!vlan_port)
    return NSM_BRIDGE_ERR_NOT_BOUND;

  NSM_VLAN_SET(&nvlan, vid);

  vlan_table = vlan_port->mode == NSM_VLAN_PORT_MODE_CN
               || vlan_port->mode == NSM_VLAN_PORT_MODE_PN ?
               bridge->svlan_table : bridge->vlan_table;


  if (!vlan_table)
    return NSM_VLAN_ERR_VLAN_NOT_FOUND;

  rn = avl_search (vlan_table, (void *)&nvlan);
  if (!rn)
    return NSM_VLAN_ERR_VLAN_NOT_FOUND;

  vlan = rn->info;

  if (!vlan )
    return NSM_VLAN_ERR_VLAN_NOT_FOUND;

  if (!vlan->pvlan_configured)
    return 0;

  switch (vlan->pvlan_type)
    {
      case NSM_PVLAN_PRIMARY:
        for (secondary_vid = 2; secondary_vid < NSM_VLAN_MAX; secondary_vid++)
          {
            if (NSM_VLAN_BMP_IS_MEMBER (vlan_port->pvlan_port_info.secondary_vlan_bmp,
                                        secondary_vid))
              {
#ifdef HAVE_HAL
                 ret =  hal_l2_add_fdb (bridge->name, ifp->ifindex, mac,
                                        HAL_HW_LENGTH, secondary_vid,
                                        secondary_vid, flags, is_forward);

                 if (HAL_SUCCESS != ret)
                   return NSM_HAL_FDB_ERR;
#endif /* HAVE_HAL */

                 NSM_VLAN_SET(&nvlan, secondary_vid);

                 if (!bridge->vlan_table)
                   return NSM_VLAN_ERR_VLAN_NOT_FOUND;

                 rn = avl_search (bridge->vlan_table, (void *)&nvlan);

                 if (!rn)
                   return NSM_VLAN_ERR_VLAN_NOT_FOUND;

                 secondary_vlan = rn->info;

                 if (!secondary_vlan)
                   return NSM_VLAN_ERR_VLAN_NOT_FOUND;

                 if (nsm_add_mac_to_static_list
                        (bridge, secondary_vlan->static_fdb_list,
                         ifp->ifindex, mac,
                         is_forward, type, secondary_vid) != RESULT_OK)
                    return NSM_BRIDGE_ERR_MEM;
              }
          }

        break;
      case NSM_PVLAN_COMMUNITY:
      case NSM_PVLAN_ISOLATED:
        primary_vid = vlan_port->pvlan_port_info.primary_vid;
        NSM_VLAN_SET (&nvlan, primary_vid);

        rn = avl_search (bridge->vlan_table, (void *)&nvlan);

        if (!rn)
          return NSM_VLAN_ERR_VLAN_NOT_FOUND;

        primary_vlan = rn->info;

        if (!primary_vlan)
          return NSM_VLAN_ERR_VLAN_NOT_FOUND;

#ifdef HAVE_HAL
        ret = hal_l2_add_fdb (bridge->name, ifp->ifindex, mac,
                              HAL_HW_LENGTH, primary_vid, primary_vid,
                              flags, is_forward);
#endif /* HAVE_HAL */

        if (nsm_add_mac_to_static_list
                (bridge, primary_vlan->static_fdb_list,
                 ifp->ifindex, mac,
                 is_forward, type, primary_vid) != RESULT_OK)
           return NSM_BRIDGE_ERR_MEM;

      default:
        return 0;
    }
  return 0;
}
#endif /* HAVE_PVLAN */

#ifdef HAVE_PVLAN
static int
nsm_pvlan_delete_mac (struct nsm_bridge *bridge, struct interface *ifp,
                      u_int8_t *mac, u_int16_t vid, unsigned char flags,
                      bool_t is_forward, int type)
{
  struct nsm_vlan_port *vlan_port;
  struct nsm_bridge_port *br_port = NULL;
  struct nsm_vlan *secondary_vlan, *primary_vlan;
  u_int16_t secondary_vid, primary_vid;
  struct nsm_vlan *vlan;
  struct nsm_vlan nvlan;
  struct avl_node *rn;
  struct nsm_if *zif;
  struct avl_tree *vlan_table = NULL;
  int ret;

  if (!bridge || !ifp)
    return NSM_BRIDGE_ERR_NOT_BOUND;

  zif = (struct nsm_if *)ifp->info;

  if (zif == NULL || zif->switchport == NULL)
    return NSM_BRIDGE_ERR_NOT_BOUND;

  br_port = zif->switchport;

  vlan_port = &br_port->vlan_port;

  if (!vlan_port)
    return NSM_BRIDGE_ERR_NOT_BOUND;

  if ((bridge->type == NSM_BRIDGE_TYPE_PROVIDER_RSTP) ||
     (bridge->type == NSM_BRIDGE_TYPE_PROVIDER_MSTP))
    vlan_table = bridge->svlan_table;
#ifdef HAVE_B_BEB
  else if ((bridge->type == NSM_BRIDGE_TYPE_BACKBONE_RSTP) ||
           (bridge->type == NSM_BRIDGE_TYPE_BACKBONE_MSTP))
    vlan_table = bridge->bvlan_table;
#endif /*HAVE_B_BEB*/
  else
    vlan_table =  bridge->vlan_table;

  if (!vlan_table)
    return NSM_VLAN_ERR_VLAN_NOT_FOUND;

  NSM_VLAN_SET(&nvlan, vid);

  rn = avl_search (vlan_table, (void *)&nvlan);

  if (!rn)
    return NSM_VLAN_ERR_VLAN_NOT_FOUND;

  vlan = rn->info;

  if (!vlan)
    return NSM_VLAN_ERR_VLAN_NOT_FOUND;

  if (!vlan->pvlan_configured)
    return 0;

  switch (vlan->pvlan_type)
    {
      case NSM_PVLAN_PRIMARY:
        for (secondary_vid = 2; secondary_vid < NSM_VLAN_MAX; secondary_vid++)
          {
            if (NSM_VLAN_BMP_IS_MEMBER (vlan_port->pvlan_port_info.secondary_vlan_bmp,
                                        secondary_vid))
              {
#ifdef HAVE_HAL
                 ret =  hal_l2_del_fdb (bridge->name, ifp->ifindex, mac,
                                        HAL_HW_LENGTH, secondary_vid,
                                        secondary_vid, flags);

                 if (HAL_SUCCESS != ret)
                   return NSM_HAL_FDB_ERR;
#endif /* HAVE_HAL */

                 NSM_VLAN_SET(&nvlan, secondary_vid);

                 if (!bridge->vlan_table)
                   return NSM_VLAN_ERR_VLAN_NOT_FOUND;

                 rn = avl_search (bridge->vlan_table, (void *)&nvlan);

                 if (!rn)
                   return NSM_VLAN_ERR_VLAN_NOT_FOUND;

                 secondary_vlan = rn->info;

                 if (!secondary_vlan)
                   return NSM_VLAN_ERR_VLAN_NOT_FOUND;

                 nsm_del_mac_from_static_list
                     (bridge, secondary_vlan->static_fdb_list, ifp->ifindex, mac);
              }
          }

        break;
      case NSM_PVLAN_COMMUNITY:
      case NSM_PVLAN_ISOLATED:
        primary_vid = vlan_port->pvlan_port_info.primary_vid;
        NSM_VLAN_SET (&nvlan, primary_vid);

        rn = avl_search (bridge->vlan_table, (void *)&nvlan);
        if (!rn)
          return NSM_VLAN_ERR_VLAN_NOT_FOUND;

        primary_vlan = rn->info;

        if (!primary_vlan)
          return NSM_VLAN_ERR_VLAN_NOT_FOUND;

#ifdef HAVE_HAL
        ret = hal_l2_del_fdb (bridge->name, ifp->ifindex, mac,
                              HAL_HW_LENGTH, primary_vid, primary_vid,
                              flags);
#endif /* HAVE_HAL */

        nsm_del_mac_from_static_list
              (bridge, primary_vlan->static_fdb_list, ifp->ifindex, mac);

      default:
        return 0;
    }
  return 0;
}
#endif /* HAVE_PVLAN */

int
nsm_bridge_add_mac(struct nsm_bridge_master *master, char *name,
                   struct interface *ifp, u_int8_t *mac, u_int16_t vid,
                   u_int16_t svid, unsigned char flags,
                   bool_t is_forward, int type)
{
  int ret = 0;
  struct nsm_bridge *bridge;
  int    is_vlan_aware = PAL_FALSE;
  struct ptree *static_fdb_list = NULL;
#ifdef HAVE_GMRP
  struct nsm_bridge_listener *brlistnr = NULL;
  struct listnode *ln = NULL;
#endif /* HAVE_GMRP */

#ifdef HAVE_VLAN
  struct nsm_if *zif;
  struct nsm_vlan nvlan;
  struct avl_node *rn;
  struct nsm_vlan *vlan = NULL;
  struct avl_tree *vlan_table;
  struct nsm_vlan_port *vlan_port;
  struct nsm_bridge_port *br_port = NULL;
#ifdef HAVE_PROVIDER_BRIDGE
  struct nsm_cvlan_reg_key *key = NULL;
  struct nsm_pro_edge_sw_ctx *ctx = NULL;
#endif /* HAVE_PROVIDER_BRIDGE */
#endif /* HAVE_VLAN */

  if (!master)
    return NSM_BRIDGE_ERR_NOTFOUND;

  bridge = nsm_lookup_bridge_by_name (master, name);

  if ( !bridge )
    return NSM_BRIDGE_ERR_NOTFOUND;

  is_vlan_aware = nsm_bridge_is_vlan_aware(master, name);

#ifdef HAVE_VLAN
  zif = (struct nsm_if *)ifp->info;

  /* If the vid passed from cli is 0 and the bridge is vlan aware associate
   * the static entry with the default vlan
   */
  if ( !vid && is_vlan_aware )
    {
      vid = NSM_VLAN_DEFAULT_VID;
      svid = NSM_VLAN_DEFAULT_VID;
    }

  /* If the interface is not bound to the bridge store the configuration
   * and return Error. This is to make sure that during start up since
   * the command for static mac address is before interface configuration,
   * the static mac configuration needs to be stored so that it can be
   * restored when interface is added to the bridge.
   */

  if ( (zif) && (!zif->bridge) && (type == CLI_MAC) )
    {
       nsm_bridge_static_fdb_config_add (bridge->name, ifp, mac, vid,
                                        is_forward,
                                        NSM_BRIDGE_MAC_PRI_OVR_NONE,
                                        0);

      return NSM_BRIDGE_ERR_NOT_BOUND;
    }


  if (zif == NULL || zif->switchport == NULL)
    return NSM_BRIDGE_ERR_NOT_BOUND;

  br_port = zif->switchport;

  vlan_port = &br_port->vlan_port;

  if ( !vlan_port )
    {
      return NSM_BRIDGE_ERR_NOT_BOUND;
    }


  vlan_table = vlan_port->mode == NSM_VLAN_PORT_MODE_CN
               || vlan_port->mode == NSM_VLAN_PORT_MODE_PN ?
               bridge->svlan_table : bridge->vlan_table;

  /* If the bridge is vlan aware verify that vlan is configured for the bridge
   * and the vlan is configured for the port.
   */

#ifdef HAVE_PROVIDER_BRIDGE
  if (bridge->provider_edge == PAL_TRUE)
    {
      if (svid == NSM_VLAN_NULL_VID)
        return NSM_BRIDGE_ERR_INVALID_ARG;

      ctx = nsm_pro_edge_swctx_lookup (bridge, vid, svid);

      if (!ctx)
        return NSM_VLAN_ERR_VLAN_NOT_FOUND;

      if (vlan_port->mode == NSM_VLAN_PORT_MODE_CE)
        {
           if ((key = nsm_vlan_port_cvlan_regis_exist (ifp, vid)) == NULL
               || key->svid != svid)
             return NSM_BRIDGE_ERR_NOT_BOUND;
        }
      else
        {
          if ((!(NSM_VLAN_BMP_IS_MEMBER (vlan_port->staticMemberBmp, svid))) &&
              (!(NSM_VLAN_BMP_IS_MEMBER (vlan_port->dynamicMemberBmp, svid))) )
            return NSM_BRIDGE_ERR_NOT_BOUND;
        }

      static_fdb_list = ctx->static_fdb_list;

      if (!static_fdb_list)
        return NSM_BRIDGE_ERR_NOT_BOUND;

    }
  else if ( is_vlan_aware )
#else
  if ( is_vlan_aware )
#endif /* HAVE_PROVIDER_BRIDGE */
    {
      NSM_VLAN_SET(&nvlan, vid);

      if (!vlan_table)
        return NSM_VLAN_ERR_VLAN_NOT_FOUND;

      rn = avl_search (vlan_table, (void *)&nvlan);

      if (!rn)
        return NSM_VLAN_ERR_VLAN_NOT_FOUND;

      vlan = rn->info;

      if (!vlan)
        return NSM_VLAN_ERR_VLAN_NOT_FOUND;

      static_fdb_list = vlan->static_fdb_list;

      if (!static_fdb_list)
        return NSM_BRIDGE_ERR_NOT_BOUND;

      /* Verify that the vlan is part of the port membership */
      if ( (vid != NSM_VLAN_DEFAULT_VID) &&
           (!(NSM_VLAN_BMP_IS_MEMBER (vlan_port->staticMemberBmp, vid))) &&
           (!(NSM_VLAN_BMP_IS_MEMBER (vlan_port->dynamicMemberBmp, vid))) )
        return NSM_BRIDGE_ERR_NOT_BOUND;
    }

  if (PAL_TRUE == is_vlan_aware)
    {
      if ((CLI_MAC == type)
#if defined HAVE_GMPLS && defined HAVE_GELS
         || (GELS_MAC == type)
#endif /*HAVE_GMPLS && HAVE_GELS */
         )
        ret = nsm_check_duplicate_unicast_mac (static_fdb_list, ifp->ifindex,
                                               mac);
      if (NSM_ERR_DUPLICATE_MAC == ret)
        return ret;
    }
  else
#endif /* HAVE_VLAN */
    {
      if ((CLI_MAC == type)
#if defined HAVE_GMPLS && defined HAVE_GELS
         || (GELS_MAC == type)
#endif /*HAVE_GMPLS && HAVE_GELS */
         )
        ret = nsm_check_duplicate_unicast_mac (bridge->static_fdb_list, ifp->ifindex,
                                               mac);
      if (NSM_ERR_DUPLICATE_MAC == ret)
        return ret;
    }

#ifdef HAVE_HAL
  if( IGMP_SNOOPING_MAC != type && AUTH_MAC != type )
    {
       ret =  hal_l2_add_fdb(bridge->name, ifp->ifindex, mac, HAL_HW_LENGTH,
                             vid, svid, flags, is_forward);
       if( HAL_SUCCESS != ret )
         return NSM_HAL_FDB_ERR;
    }
#endif /* HAVE_HAL */
#ifdef HAVE_VLAN
      if ( is_vlan_aware == PAL_TRUE )
        {
#ifdef HAVE_MAC_AUTH
          if ( AUTH_MAC == type )
            {
            if ( HAL_L2_FDB_STATIC == flags )
              if ( nsm_add_mac_to_static_list (bridge, static_fdb_list,
                                               ifp->ifindex, mac, is_forward,
                                               type, vid) != RESULT_OK )
                return NSM_BRIDGE_ERR_MEM;
            }
          else
#endif
           /* Add only static entires to static_fdb_list */
           if ( HAL_L2_FDB_STATIC == flags )
             if (nsm_add_mac_to_static_list (bridge, static_fdb_list, ifp->ifindex,
                                            mac, is_forward, type, vid)!= RESULT_OK )
              return NSM_BRIDGE_ERR_MEM;
        }
      else
#endif
        {
          /* Add only static entires to static_fdb_list */
          if ( HAL_L2_FDB_STATIC == flags )
            if ( nsm_add_mac_to_static_list (bridge, bridge->static_fdb_list,
                                           ifp->ifindex, mac, is_forward,
                                           type, vid) != RESULT_OK )
            return NSM_BRIDGE_ERR_MEM;
        }
#ifdef HAVE_GMRP
#ifdef HAVE_MAC_AUTH
  if (AUTH_MAC != type)
#endif
    {
      LIST_LOOP (bridge->bridge_listener_list, brlistnr, ln)
      {
        if(brlistnr->add_static_mac_addr_func && (type == CLI_MAC))
          {
            (brlistnr->add_static_mac_addr_func)(bridge->master, bridge->name,
                                                 ifp, mac, vid, svid,
                                                 is_forward);
          }
      }
    }
#endif /* HAVE_GMRP */

#ifdef HAVE_PVLAN
#ifdef HAVE_MAC_AUTH
  if ( AUTH_MAC != type && is_vlan_aware)
#endif
  /* If private vlan is configured, mac to be configured on all the associated
   * vlans */
    {
      ret = nsm_pvlan_add_mac (bridge, ifp, mac, vid, flags, is_forward, type);

      if (ret < 0)
        return ret;
    }
#endif /* HAVE_PVLAN */
#ifdef HAVE_MAC_AUTH
  if ( AUTH_MAC == type && is_vlan_aware)
    {
       ret = hal_l2_add_fdb (bridge->name, ifp->ifindex, mac, HAL_HW_LENGTH,
                             vid, svid, flags, is_forward);
       if( HAL_SUCCESS != ret )
         return NSM_HAL_FDB_ERR;
    }
#endif

  return 0;
}

int
nsm_bridge_delete_mac(struct nsm_bridge_master *master, char *name,
                      struct interface *ifp, u_int8_t *mac, u_int16_t vid,
                      u_int16_t svid, unsigned char flags, bool_t is_forward,
                      int type)
{
  struct nsm_bridge *bridge;
  int    is_vlan_aware = PAL_FALSE;
#ifdef HAVE_GMRP
  struct nsm_bridge_listener *brlistnr = NULL;
  struct listnode *ln = NULL;
#endif /* HAVE_GMRP */
#ifdef HAVE_VLAN
  struct nsm_if *zif;
  struct nsm_vlan nvlan;
  struct avl_node *rn;
  struct nsm_vlan *vlan = NULL;
  struct avl_tree *vlan_table = NULL;
  struct nsm_vlan_port *vlan_port = NULL;
  struct ptree *static_fdb_list = NULL;
  struct nsm_bridge_port *br_port = NULL;
#ifdef HAVE_PROVIDER_BRIDGE
  struct nsm_cvlan_reg_key *key = NULL;
  struct nsm_pro_edge_sw_ctx *ctx = NULL;
#endif /* HAVE_PROVIDER_BRIDGE */
#endif /* HAVE_VLAN */
  struct ptree_node *pn = NULL;
  struct nsm_bridge_static_fdb *static_fdb = NULL;
  struct nsm_bridge_fdb_ifinfo *pInfo = NULL;
  int ret = RESULT_ERROR;

  if (!master)
    return NSM_BRIDGE_ERR_NOTFOUND;

  bridge = nsm_lookup_bridge_by_name (master, name);
  if ( !bridge )
    return NSM_BRIDGE_ERR_NOTFOUND;

  bridge = nsm_lookup_bridge_by_name (master, name);
  if ( !bridge )
    return NSM_BRIDGE_ERR_NOTFOUND;

  is_vlan_aware = nsm_bridge_is_vlan_aware(master, name);

#ifdef HAVE_VLAN
  zif = (struct nsm_if *)ifp->info;

  if (zif == NULL || zif->switchport == NULL)
    return NSM_BRIDGE_ERR_NOT_BOUND;

  br_port = zif->switchport;

  if (is_vlan_aware)
    {
      vlan_port = &br_port->vlan_port;

      if (vlan_port == NULL)
        return NSM_BRIDGE_ERR_NOT_BOUND;
    }
  else
      return NSM_BRIDGE_ERR_INVALID_MODE;

  /* If the vid passed from cli is 0 and the bridge is vlan aware associate
   * the static entry with the default vlan
   */

  if ( !vid && is_vlan_aware )
    {
      vid = NSM_VLAN_DEFAULT_VID;
      svid = NSM_VLAN_DEFAULT_VID;
    }

  if (vlan_port)
    vlan_table = vlan_port->mode == NSM_VLAN_PORT_MODE_CN
                 || vlan_port->mode == NSM_VLAN_PORT_MODE_PN ?
                 bridge->svlan_table : bridge->vlan_table;

  /* If the bridge is vlan aware verify that vlan is configured for the bridge
   * and the vlan is configured for the port.
   */

  if (! vlan_table && is_vlan_aware)
    return NSM_VLAN_ERR_GENERAL;

#ifdef HAVE_PROVIDER_BRIDGE
  if (bridge->provider_edge == PAL_TRUE)
    {
      ctx = nsm_pro_edge_swctx_lookup (bridge, vid, svid);

      if (!ctx)
        return NSM_VLAN_ERR_VLAN_NOT_FOUND;

      if (!vlan_port)
        return NSM_BRIDGE_ERR_NOT_BOUND;

      if (vlan_port->mode == NSM_VLAN_PORT_MODE_CE)
        {
           if ((key = nsm_vlan_port_cvlan_regis_exist (ifp, vid)) == NULL
               || key->svid != svid)
             return NSM_BRIDGE_ERR_NOT_BOUND;
        }
      else
        {
           if ((!(NSM_VLAN_BMP_IS_MEMBER (vlan_port->staticMemberBmp, svid))) &&
               (!(NSM_VLAN_BMP_IS_MEMBER (vlan_port->dynamicMemberBmp, svid))) )
             return NSM_BRIDGE_ERR_NOT_BOUND;
        }

      static_fdb_list = ctx->static_fdb_list;

      if (!static_fdb_list)
        return NSM_BRIDGE_ERR_NOT_BOUND;

      for (pn = ptree_top (static_fdb_list); pn; pn = ptree_next (pn))
        {
          if (! (static_fdb = pn->info) )
            continue;
          pInfo = static_fdb->ifinfo_list;
          while (pInfo)
            {
              if ((!pal_mem_cmp (static_fdb->mac_addr, mac, ETHER_ADDR_LEN))
                  && (pInfo->is_forward == is_forward)
                  && (pInfo->ifindex == ifp->ifindex))
                {
                  ret = RESULT_OK;
                  break;
                }

              pInfo = pInfo->next_if;
            }
          if (ret == RESULT_OK)
            break;
        }

      if( RESULT_OK != ret )
        return NSM_BRIDGE_ERR_NOT_BOUND;

    }
  else if ( is_vlan_aware )
#else
  if ( is_vlan_aware )
#endif /* HAVE_PROVIDER_BRIDGE */
    {
      NSM_VLAN_SET(&nvlan, vid);
      rn = avl_search (vlan_table, (void *)&nvlan);
      if (!rn)
        return NSM_VLAN_ERR_VLAN_NOT_FOUND;
      vlan = rn->info;
      if (!vlan)
        return NSM_VLAN_ERR_VLAN_NOT_FOUND;

      /* Verify that the vlan is part of the port membership */
      if ( (vid != NSM_VLAN_DEFAULT_VID) &&
           !(NSM_VLAN_BMP_IS_MEMBER (vlan_port->staticMemberBmp, vid)) &&
           !(NSM_VLAN_BMP_IS_MEMBER (vlan_port->dynamicMemberBmp, vid)))
        return NSM_BRIDGE_ERR_NOT_BOUND;

      static_fdb_list = vlan->static_fdb_list;

      if (!static_fdb_list)
        return NSM_BRIDGE_ERR_NOT_BOUND;

      for (pn = ptree_top (static_fdb_list); pn; pn = ptree_next (pn))
        {
          if (! (static_fdb = pn->info) )
            continue;
          pInfo = static_fdb->ifinfo_list;
          while (pInfo)
            {
              if ((!pal_mem_cmp (static_fdb->mac_addr, mac, ETHER_ADDR_LEN))
                  && (pInfo->is_forward == is_forward)
                  && (pInfo->ifindex == ifp->ifindex))
                {
                  ret = RESULT_OK;
                  break;
                }
              pInfo = pInfo->next_if;
            }

          if (ret == RESULT_OK)
            break;
        }

      if( RESULT_OK != ret )
        return NSM_BRIDGE_ERR_NOT_BOUND;

    }

#endif /* HAVE_VLAN */

#ifdef HAVE_HAL
  if( IGMP_SNOOPING_MAC != type )
    {
       ret = hal_l2_del_fdb(bridge->name, ifp->ifindex, mac, HAL_HW_LENGTH,
                            vid, svid, flags);
       if( HAL_SUCCESS != ret )
          return NSM_HAL_FDB_ERR;
    }
#endif /* HAVE_HAL */

#ifdef HAVE_VLAN
  if ( nsm_bridge_is_vlan_aware(master, name) == PAL_TRUE )
    nsm_del_mac_from_static_list (bridge, static_fdb_list, ifp->ifindex, mac);
  else
#endif /* HAVE_VLAN */
    nsm_del_mac_from_static_list (bridge, bridge->static_fdb_list, ifp->ifindex, mac);

#ifdef HAVE_GMRP
  LIST_LOOP (bridge->bridge_listener_list, brlistnr, ln)
    {
      if(brlistnr->delete_static_mac_addr_func && (type == CLI_MAC))
        {
          (brlistnr->delete_static_mac_addr_func)(bridge->master, bridge->name,
                                                  ifp, mac, vid, svid,
                                                  is_forward);
        }
    }
#endif /* HAVE_GMRP */

#ifdef HAVE_PVLAN
  ret = nsm_pvlan_delete_mac (bridge, ifp, mac, vid, flags, is_forward, type);

  if (ret < 0)
    return ret;
#endif /* HAVE_PVLAN */

  return 0;
}

int nsm_bridge_clear_dynamic_mac_port (struct nsm_bridge_master *master,
                                       struct interface *ifp, char *bridge_name, 
                                       int instance)
{
  struct nsm_bridge *bridge = NULL;

  if (!master)
    return NSM_BRIDGE_ERR_NOTFOUND;

  bridge = nsm_lookup_bridge_by_name (master, bridge_name);

  if ( !bridge )
    return NSM_BRIDGE_ERR_NOTFOUND;

#ifdef HAVE_HAL
  if (hal_bridge_flush_fdb_by_port (bridge->name, ifp->ifindex, instance,0,0)
                                    != HAL_SUCCESS)
    return NSM_HAL_FDB_ERR;
#endif /* HAVE_HAL */

  return 0;

}

int nsm_bridge_clear_dynamic_mac_vlan (struct nsm_bridge_master *master,
                                       u_int16_t vid, char *bridge_name)
{
  struct nsm_bridge *bridge = NULL;

  if (!master)
    return NSM_BRIDGE_ERR_NOTFOUND;

  bridge = nsm_lookup_bridge_by_name (master, bridge_name);

  if ( !bridge )
    return NSM_BRIDGE_ERR_NOTFOUND;

#ifdef HAVE_PROVIDER_BRIDGE
  if (bridge->provider_edge == PAL_TRUE)
    return NSM_BRIDGE_ERR_INVALID_ARG;
#endif /* HAVE_PROVIDER_BRIDGE */

#ifdef HAVE_HAL
  if (hal_bridge_flush_fdb_by_port (bridge->name,0, 0, vid, vid) != HAL_SUCCESS)
    return NSM_HAL_FDB_ERR;
#endif /* HAVE_HAL */

  return 0;

}

int nsm_bridge_clear_dynamic_mac_addr (struct nsm_bridge_master *master,
                                       u_int8_t *mac_addr, char *bridge_name)
{
  struct nsm_bridge *bridge = NULL;

  if (!master)
    return NSM_BRIDGE_ERR_NOTFOUND;

  bridge = nsm_lookup_bridge_by_name (master, bridge_name);

  if ( !bridge )
    return NSM_BRIDGE_ERR_NOTFOUND;

#ifdef HAVE_HAL
  if (hal_bridge_flush_dynamic_fdb_by_mac (bridge->name, mac_addr,
                                           HAL_HW_LENGTH) != HAL_SUCCESS)
    return NSM_HAL_FDB_ERR;
#endif /* HAVE_HAL */

  return 0;
}

int nsm_bridge_clear_dynamic_mac_bridge (struct nsm_bridge_master *master,
                                         char *bridge_name)
{
  struct nsm_if *zif = NULL;
  struct interface *ifp = NULL;
  struct avl_node *avl_port = NULL;
  struct nsm_bridge *bridge = NULL;
  struct nsm_bridge_port *br_port = NULL;

  if (!master)
    return NSM_BRIDGE_ERR_NOTFOUND;

  bridge =  nsm_lookup_bridge_by_name (master, bridge_name);

  if ( !bridge )
    return NSM_BRIDGE_ERR_NOTFOUND;

  if (!bridge->port_tree)
    return 0;

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

#ifdef HAVE_HAL
      if (hal_bridge_flush_fdb_by_port (bridge->name, ifp->ifindex, 0, 0, 0)
          != HAL_SUCCESS)
        return NSM_HAL_FDB_ERR;
#endif /* HAVE_HAL */
    }

  return 0;
}

int
nsm_bridge_clear_static_mac (struct nsm_bridge_master *master, char *bridge_name)
{
  struct nsm_bridge *bridge;
  struct nsm_bridge_fdb_ifinfo cmp;
  struct nsm_bridge_static_fdb fdbcmp;

  if (!master)
    return NSM_BRIDGE_ERR_NOTFOUND;

  bridge = nsm_lookup_bridge_by_name (master, bridge_name);
  if ( !bridge )
    return NSM_BRIDGE_ERR_NOTFOUND;

  pal_mem_set (&cmp, 0, sizeof (struct nsm_bridge_fdb_ifinfo));
  pal_mem_set (&fdbcmp, 0, sizeof (struct nsm_bridge_static_fdb));

  return nsm_bridge_delete_all_fdb (bridge, nsm_bridge_delete_static,
                                    nsm_bridge_delete_all_static_from_port,
                                    &cmp, &fdbcmp);

}

int
nsm_bridge_clear_multicast_mac (struct nsm_bridge_master *master, char *bridge_name)
{
  struct nsm_bridge *bridge;
  struct nsm_bridge_fdb_ifinfo cmp;
  struct nsm_bridge_static_fdb fdbcmp;

  if (!master)
    return NSM_BRIDGE_ERR_NOTFOUND;

  bridge = nsm_lookup_bridge_by_name (master, bridge_name);
  if ( !bridge )
    return NSM_BRIDGE_ERR_NOTFOUND;

  pal_mem_set (&cmp, 0, sizeof (struct nsm_bridge_fdb_ifinfo));
  pal_mem_set (&fdbcmp, 0, sizeof (struct nsm_bridge_static_fdb));

  return nsm_bridge_delete_all_fdb (bridge, nsm_bridge_delete_multicast,
                                    nsm_bridge_delete_all_multicast_from_port,
                                    &cmp, &fdbcmp);

}

int
nsm_bridge_clear_static_mac_port (struct nsm_bridge_master *master, char *bridge_name,
                                  struct interface *ifp)
{

  struct nsm_bridge *bridge;
  struct nsm_bridge_fdb_ifinfo cmp;
  struct nsm_bridge_static_fdb fdbcmp;

  if (!master)
    return NSM_BRIDGE_ERR_NOTFOUND;

  bridge = nsm_lookup_bridge_by_name (master, bridge_name);
  if ( !bridge )
    return NSM_BRIDGE_ERR_NOTFOUND;

  pal_mem_set (&cmp, 0, sizeof (struct nsm_bridge_fdb_ifinfo));
  pal_mem_set (&fdbcmp, 0, sizeof (struct nsm_bridge_static_fdb));

  cmp.ifindex = ifp->ifindex;

  return nsm_bridge_delete_all_fdb (bridge, nsm_bridge_delete_static,
                                    nsm_bridge_delete_static_by_ifindex,
                                    &cmp, &fdbcmp);
}

int nsm_bridge_clear_multicast_mac_port (struct nsm_bridge_master *master, char *bridge_name,
                                         struct interface *ifp)
{

  struct nsm_bridge *bridge;
  struct nsm_bridge_fdb_ifinfo cmp;
  struct nsm_bridge_static_fdb fdbcmp;

  if (!master)
    return NSM_BRIDGE_ERR_NOTFOUND;

  bridge = nsm_lookup_bridge_by_name (master, bridge_name);
  if ( !bridge )
    return NSM_BRIDGE_ERR_NOTFOUND;

  pal_mem_set (&cmp, 0, sizeof (struct nsm_bridge_fdb_ifinfo));
  pal_mem_set (&fdbcmp, 0, sizeof (struct nsm_bridge_static_fdb));

  cmp.ifindex = ifp->ifindex;

  return nsm_bridge_delete_all_fdb (bridge, nsm_bridge_delete_multicast,
                                    nsm_bridge_delete_multicast_by_ifindex,
                                    &cmp, &fdbcmp);

}

int nsm_bridge_clear_static_mac_addr (struct nsm_bridge_master *master, char *bridge_name,
                                      u_int8_t *mac_addr)
{
  struct nsm_bridge *bridge;
  struct nsm_bridge_fdb_ifinfo cmp;
  struct nsm_bridge_static_fdb fdbcmp;

  if (!master)
    return NSM_BRIDGE_ERR_NOTFOUND;

  bridge = nsm_lookup_bridge_by_name (master, bridge_name);
  if ( !bridge )
    return NSM_BRIDGE_ERR_NOTFOUND;

  pal_mem_set (&cmp, 0, sizeof (struct nsm_bridge_fdb_ifinfo));
  pal_mem_set (&fdbcmp, 0, sizeof (struct nsm_bridge_static_fdb));
  pal_mem_cpy (fdbcmp.mac_addr, mac_addr, ETHER_ADDR_LEN);

  return nsm_bridge_delete_all_fdb (bridge,
                                    nsm_bridge_should_delete_static_mac,
                                    nsm_bridge_delete_all_static_from_port,
                                    &cmp, &fdbcmp);

}

int nsm_bridge_clear_multicast_mac_addr (struct nsm_bridge_master *master, char *bridge_name,
                                         u_int8_t *mac_addr)
{
  struct nsm_bridge *bridge;
  struct nsm_bridge_fdb_ifinfo cmp;
  struct nsm_bridge_static_fdb fdbcmp;

  if (!master)
    return NSM_BRIDGE_ERR_NOTFOUND;

  bridge = nsm_lookup_bridge_by_name (master, bridge_name);
  if ( !bridge )
    return NSM_BRIDGE_ERR_NOTFOUND;

  pal_mem_set (&cmp, 0, sizeof (struct nsm_bridge_fdb_ifinfo));
  pal_mem_set (&fdbcmp, 0, sizeof (struct nsm_bridge_static_fdb));
  pal_mem_cpy (fdbcmp.mac_addr, mac_addr, ETHER_ADDR_LEN);

  return nsm_bridge_delete_all_fdb (bridge,
                                    nsm_bridge_should_delete_multicast_mac,
                                    nsm_bridge_delete_all_multicast_from_port,
                                    &cmp, &fdbcmp);

}


#ifdef HAVE_VLAN
static int
nsm_bridge_clear_mac_vlan (struct nsm_bridge_master *master,
                           char *bridge_name, u_int16_t vid,
                           nsm_bridge_delete_mac_func_t should_delete_mac_func,
                           nsm_bridge_delete_mac_from_port_func_t should_delete_mac_from_port,
                           struct nsm_bridge_fdb_ifinfo *cmp,
                           struct nsm_bridge_static_fdb *fdbcmp)
{
  struct nsm_bridge *bridge = NULL;
  int    is_vlan_aware = PAL_FALSE;
  struct ptree_node *pn   =   NULL;
  struct nsm_vlan nvlan;
  struct nsm_bridge_static_fdb *static_fdb = NULL;
  struct avl_node *rn   = NULL;
  struct avl_tree *vlan_table = NULL;
  struct nsm_vlan *vlan = NULL;
  int ret = 0;

  if (!master)
    return NSM_BRIDGE_ERR_NOTFOUND;

  bridge = nsm_lookup_bridge_by_name (master, bridge_name);

  if ( !bridge || !should_delete_mac_func)
    return NSM_BRIDGE_ERR_NOTFOUND;

  is_vlan_aware = nsm_bridge_is_vlan_aware(master, bridge_name);

#ifdef HAVE_PROVIDER_BRIDGE
  if (bridge->provider_edge == PAL_TRUE)
    return NSM_BRIDGE_ERR_INVALID_ARG;
#endif /* HAVE_PROVIDER_BRIDGE */

  if (NSM_BRIDGE_TYPE_PROVIDER (bridge))
    vlan_table = bridge->svlan_table;
  else
    vlan_table = bridge->svlan_table;

  if (!is_vlan_aware || !vlan_table)
    return NSM_VLAN_ERR_VLAN_NOT_FOUND;

  NSM_VLAN_SET(&nvlan, vid);

  rn = avl_search (vlan_table, (void *)&nvlan);

  if (!rn)
    return NSM_VLAN_ERR_VLAN_NOT_FOUND;
  vlan = rn->info;
  if (!vlan)
    return NSM_VLAN_ERR_VLAN_NOT_FOUND;

  if (!vlan->static_fdb_list)
    return 0;

  for (pn = ptree_top (vlan->static_fdb_list); pn; pn = ptree_next (pn))
    {
      if (! (static_fdb = pn->info)  ||
          (should_delete_mac_func(static_fdb, fdbcmp) == PAL_FALSE) )
        continue;

      if ( (ret = nsm_bridge_delete_fdb (bridge, static_fdb, vlan->vid,
                                         vlan->vid, should_delete_mac_from_port,
                                         cmp))
           < 0)
        return ret;

      if (!static_fdb->ifinfo_list)
        {
          XFREE (MTYPE_NSM_BRIDGE_STATIC_FDB, static_fdb);
          pn->info = NULL;
        }
    }

  return 0;
}

int
nsm_bridge_clear_static_mac_vlan (struct nsm_bridge_master *master, char *bridge_name,
                                  u_int16_t vid)
{
  struct nsm_bridge_fdb_ifinfo cmp;
  struct nsm_bridge_static_fdb fdbcmp;

  pal_mem_set (&cmp, 0, sizeof (struct nsm_bridge_fdb_ifinfo));
  pal_mem_set (&fdbcmp, 0, sizeof (struct nsm_bridge_static_fdb));

  return nsm_bridge_clear_mac_vlan (master, bridge_name, vid,
                                    nsm_bridge_delete_static,
                                    nsm_bridge_delete_all_static_from_port,
                                    &cmp, &fdbcmp);

}

int
nsm_bridge_clear_multicast_mac_vlan (struct nsm_bridge_master *master, char *bridge_name,
                                     u_int16_t vid)
{

  struct nsm_bridge_fdb_ifinfo cmp;
  struct nsm_bridge_static_fdb fdbcmp;

  pal_mem_set (&cmp, 0, sizeof (struct nsm_bridge_fdb_ifinfo));
  pal_mem_set (&fdbcmp, 0, sizeof (struct nsm_bridge_static_fdb));

  return nsm_bridge_clear_mac_vlan (master, bridge_name, vid,
                                    nsm_bridge_delete_multicast,
                                    nsm_bridge_delete_all_multicast_from_port,
                                    &cmp, &fdbcmp);

}

#ifdef HAVE_PROVIDER_BRIDGE

static int
nsm_bridge_clear_mac_pro_edge_vlan
                           (struct nsm_bridge_master *master,
                            char *bridge_name, u_int16_t cvid, u_int16_t svid,
                            nsm_bridge_delete_mac_func_t should_delete_mac_func,
                            nsm_bridge_delete_mac_from_port_func_t should_delete_mac_from_port,
                            struct nsm_bridge_fdb_ifinfo *cmp,
                            struct nsm_bridge_static_fdb *fdbcmp)
{
  int ret = 0;
  struct nsm_bridge *bridge = NULL;
  struct ptree_node *pn   =   NULL;
  struct nsm_pro_edge_sw_ctx *ctx = NULL;
  struct nsm_bridge_static_fdb *static_fdb = NULL;

  if (!master)
    return NSM_BRIDGE_ERR_NOTFOUND;

  bridge = nsm_lookup_bridge_by_name (master, bridge_name);

  if ( !bridge || !should_delete_mac_func)
    return NSM_BRIDGE_ERR_NOTFOUND;

#ifdef HAVE_PROVIDER_BRIDGE
  if (bridge->provider_edge == PAL_FALSE)
    return NSM_BRIDGE_ERR_INVALID_ARG;
#endif /* HAVE_PROVIDER_BRIDGE */

  ctx = nsm_pro_edge_swctx_lookup (bridge, cvid, svid);

  if (! ctx)
    return NSM_VLAN_ERR_VLAN_NOT_FOUND;

  if (!ctx->static_fdb_list)
    return 0;

  for (pn = ptree_top (ctx->static_fdb_list); pn; pn = ptree_next (pn))
    {
      if (! (static_fdb = pn->info)  ||
          (should_delete_mac_func(static_fdb, fdbcmp) == PAL_FALSE) )
        continue;

      if ( (ret = nsm_bridge_delete_fdb (bridge, static_fdb, ctx->cvid,
                                         ctx->svid, should_delete_mac_from_port,
                                         cmp))
           < 0)
        return ret;

      if (!static_fdb->ifinfo_list)
        {
          XFREE (MTYPE_NSM_BRIDGE_STATIC_FDB, static_fdb);
          pn->info = NULL;
        }
    }

  return 0;
}

int
nsm_bridge_clear_static_mac_pro_edge_vlan
                             (struct nsm_bridge_master *master, char *bridge_name,
                              u_int16_t vid, u_int16_t svid)
{
  struct nsm_bridge_fdb_ifinfo cmp;
  struct nsm_bridge_static_fdb fdbcmp;

  pal_mem_set (&cmp, 0, sizeof (struct nsm_bridge_fdb_ifinfo));
  pal_mem_set (&fdbcmp, 0, sizeof (struct nsm_bridge_static_fdb));

  return nsm_bridge_clear_mac_pro_edge_vlan
                              (master, bridge_name, vid, svid,
                               nsm_bridge_delete_static,
                               nsm_bridge_delete_all_static_from_port,
                               &cmp, &fdbcmp);

}

int
nsm_bridge_clear_multicast_mac_pro_edge_vlan
                               (struct nsm_bridge_master *master,
                                char *bridge_name,
                                u_int16_t vid, u_int16_t svid)
{

  struct nsm_bridge_fdb_ifinfo cmp;
  struct nsm_bridge_static_fdb fdbcmp;

  pal_mem_set (&cmp, 0, sizeof (struct nsm_bridge_fdb_ifinfo));
  pal_mem_set (&fdbcmp, 0, sizeof (struct nsm_bridge_static_fdb));

  return nsm_bridge_clear_mac_pro_edge_vlan
                              (master, bridge_name, vid, svid,
                               nsm_bridge_delete_multicast,
                               nsm_bridge_delete_all_multicast_from_port,
                               &cmp, &fdbcmp);

}

int nsm_bridge_clear_dynamic_mac_pro_edge_vlan
                                 (struct nsm_bridge_master *master,
                                  char *bridge_name,
                                  u_int16_t vid, u_int16_t svid)
{
  struct nsm_bridge *bridge = NULL;

  if (!master)
    return NSM_BRIDGE_ERR_NOTFOUND;

  bridge = nsm_lookup_bridge_by_name (master, bridge_name);

  if ( !bridge )
    return NSM_BRIDGE_ERR_NOTFOUND;

#ifdef HAVE_HAL
  if (hal_bridge_flush_fdb_by_port (bridge->name, 0,0, vid, svid)
                                    != HAL_SUCCESS)
    return NSM_HAL_FDB_ERR;
#endif /* HAVE_HAL */

  return 0;

}

#endif /* HAVE_PROVIDER_BRIDGE */

#endif /* HAVE_VLAN */

/* API to flush dynamic entries on a port */
s_int32_t
nsm_bridge_flush_fdb_by_port (struct nsm_master *master, char *ifname)
{
  struct interface *ifp;
  struct nsm_bridge *bridge;
  struct nsm_if *zif;

  if (!master)
    return RESULT_ERROR;

  ifp = if_lookup_by_name (&master->vr->ifm, ifname);

  if (!ifp)
    return RESULT_ERROR;

  if (!INTF_TYPE_L2 (ifp))
    return RESULT_OK;

  zif = (struct nsm_if *) ifp->info;

  if (!zif)
    return RESULT_ERROR;

  bridge = zif->bridge;

  if (!bridge)
    return RESULT_OK;

#ifdef HAVE_HAL
  if (hal_bridge_flush_fdb_by_port (bridge->name, ifp->ifindex, 0, 0, 0)
                                   != HAL_SUCCESS)
    return RESULT_ERROR;
#endif /* HAVE_HAL */

  return RESULT_OK;
}

s_int32_t
nsm_bridge_flush_fdb_by_instance (struct nsm_master *master, char *ifname, int instance)
{
  struct interface *ifp;
  struct nsm_bridge *bridge;
  struct nsm_if *zif;

  if (!master)
    return RESULT_ERROR;

  ifp = if_lookup_by_name (&master->vr->ifm, ifname);

  if (!ifp)
    return RESULT_ERROR;

  if (!INTF_TYPE_L2 (ifp))
    return RESULT_OK;

  zif = (struct nsm_if *) ifp->info;

  if (!zif)
    return RESULT_ERROR;

  bridge = zif->bridge;

  if (!bridge)
    return RESULT_OK;

#ifdef HAVE_HAL
  if (hal_bridge_flush_fdb_by_port (bridge->name, ifp->ifindex, instance, 0, 0)
                                   != HAL_SUCCESS)
    return RESULT_ERROR;
#endif /* HAVE_HAL */

  return RESULT_OK;
}

/* Process Bridge Ageing Time message. */
int
nsm_server_recv_bridge_set_ageing_time(struct nsm_msg_header *header,
                                       void *arg, void *message)
{
  struct nsm_master *nm;
  struct nsm_bridge_master *master;
  struct nsm_server_entry *nse;
  struct nsm_server *ns;
  struct nsm_msg_bridge *msg;

  /* Set connection and message.  */
  nse = arg;
  ns = nse->ns;
  msg = message;

  /* Debug.  */
  if (IS_NSM_DEBUG_RECV)
    nsm_bridge_dump (ns->zg, msg);

  nm = nsm_master_lookup_by_id (nzg, header->vr_id);
  if (nm == NULL)
    return 0;

  master = nsm_bridge_get_master (nm);
  if (master == NULL) 
    return 0;

  nsm_bridge_ageing_time_set(master, msg->bridge_name,
                             msg->ageing_time, PAL_FALSE);

  return 0;
}

/* Process Bridge set state message. */
int
nsm_server_recv_bridge_set_state (struct nsm_msg_header *header,
                                  void *arg, void *message)
{
  struct nsm_master *nm = NULL;
  struct nsm_server *ns;
  struct nsm_server_entry *nse;
  struct nsm_bridge_master *master = NULL;
  struct nsm_msg_bridge_enable *msg;

  /* Set connection and message.  */
  nse = arg;
  ns = nse->ns;
  msg = message;

  /* Debug.  */
  if (IS_NSM_DEBUG_RECV)
    nsm_bridge_enable_dump (ns->zg, msg);

  nm = nsm_master_lookup_by_id (nzg, header->vr_id);

  if (nm)
    master = nsm_bridge_get_master (nm);

  if (master == NULL)
    return 0;

  nsm_bridge_set_state (master, msg->bridge_name,
                        msg->enable);

  return 0;
}

int
nsm_bridge_set_state (struct nsm_bridge_master *master, char *name,
                      u_int16_t enable)
{
  struct nsm_bridge *bridge;

  if (!master)
    return NSM_BRIDGE_ERR_NOTFOUND;

  bridge = nsm_lookup_bridge_by_name (master, name);
  if ( !bridge )
    return NSM_BRIDGE_ERR_NOTFOUND;

#ifdef HAVE_HAL
  if ( hal_bridge_set_state (bridge->name, enable) != HAL_SUCCESS )
    return NSM_BRIDGE_ERR_HAL;
#endif /* HAVE_HAL */

  bridge->enable = enable;

#ifdef HAVE_HA
  nsm_bridge_cal_modify_nsm_bridge (bridge);
#endif /* HAVE_HA */

  return 0;
}

/* Send notification to the protocol regarding update of Ageing time.*/
void
nsm_bridge_ageing_time_set_notification (struct nsm_bridge *bridge)
{
  struct nsm_msg_bridge msg;
  int nbytes, i;
  struct nsm_server_client *nsc;
  struct nsm_server_entry *nse;

  pal_mem_set (&msg, 0, sizeof (msg));

  /* Send message to PM. */
  NSM_SERVER_CLIENT_LOOP (ng, i, nsc, nse)
    if (NSM_CHECK_CTYPE (nse->service.bits, NSM_SERVICE_BRIDGE))
      {
        pal_mem_set (&msg, 0, sizeof (struct nsm_msg_bridge));
        pal_strncpy (msg.bridge_name, bridge->name, NSM_BRIDGE_NAMSIZ);
#if defined HAVE_CFM && (defined HAVE_I_BEB || defined HAVE_B_BEB)
        if (bridge->backbone_edge == NSM_PBB_BACKBONE_EDGE_SET)
          {
            /* PBB bridge will be identified as I-COMP or B-COMP bridge  */
            if ((bridge->type == NSM_BRIDGE_TYPE_PROVIDER_RSTP) ||
                (bridge->type == NSM_BRIDGE_TYPE_PROVIDER_MSTP))
              bridge->type = NSM_BRIDGE_TYPE_PBB_I_COMPONENT;
          }
        msg.type = bridge->type;
#else
        msg.type = bridge->type;
#endif /* HAVE_CFM && (HAVE_I_BEB || HAVE_B_BEB)  */

        msg.ageing_time = bridge->ageing_time;

        /* Set nse pointer and size. */
        nse->send.pnt = nse->send.buf + NSM_MSG_HEADER_SIZE;
        nse->send.size = nse->send.len - NSM_MSG_HEADER_SIZE;

        /* Encode NSM bridge interface message. */
        nbytes = nsm_encode_bridge_msg (&nse->send.pnt, &nse->send.size, &msg);
        if (nbytes < 0)
          return;

        /* Send bridge interface message. */
        nsm_server_send_message (nse, 0, 0, NSM_MSG_BRIDGE_SET_AGEING_TIME, 0, nbytes);
      }
}

int
nsm_bridge_ageing_time_set(struct nsm_bridge_master *master, char *name,
                           u_int32_t ageing_time, bool_t notify_pm )
{
  struct nsm_bridge *bridge;

  if (!master)
    return NSM_BRIDGE_ERR_NOTFOUND;

  bridge = nsm_lookup_bridge_by_name (master, name);
  if ( !bridge )
    return NSM_BRIDGE_ERR_NOTFOUND;

#ifdef HAVE_HAL
  if ( hal_bridge_set_ageing_time(bridge->name, ageing_time) != HAL_SUCCESS )
    return NSM_BRIDGE_ERR_SET_AGE_TIME;
#endif /* HAVE_HAL */

  bridge->ageing_time = ageing_time;

#ifdef HAVE_HA
  nsm_bridge_cal_modify_nsm_bridge (bridge);
#endif /* HAVE_HA */

  if ( notify_pm )
    nsm_bridge_ageing_time_set_notification (bridge);

  return 0;
}

int
nsm_bridge_set_learning(struct nsm_bridge_master *master, char *name)
{
  struct nsm_bridge *bridge;

  if (!master)
    return NSM_BRIDGE_ERR_NOTFOUND;

  bridge = nsm_lookup_bridge_by_name (master, name);
  if ( !bridge )
    return NSM_BRIDGE_ERR_NOTFOUND;

#ifdef HAVE_HAL
  if ( hal_bridge_set_learning (bridge->name, PAL_TRUE) != HAL_SUCCESS )
    return NSM_BRIDGE_ERR_NOTFOUND;
#endif /* HAVE_HAL */

#ifdef HAVE_HA
  nsm_bridge_cal_modify_nsm_bridge (bridge);
#endif /* HAVE_HA */

  bridge->learning = NSM_LEARNING_BRIDGE_SET;
  return 0;
}

int
nsm_bridge_unset_learning(struct nsm_bridge_master *master, char *name)
{
  struct nsm_bridge *bridge;

  if (!master)
    return NSM_BRIDGE_ERR_NOTFOUND;

  bridge = nsm_lookup_bridge_by_name (master, name);
  if ( !bridge )
    return NSM_BRIDGE_ERR_NOTFOUND;

#ifdef HAVE_HAL
  if ( hal_bridge_set_learning (bridge->name, PAL_FALSE) != HAL_SUCCESS )
    return NSM_BRIDGE_ERR_NOTFOUND;
#endif /* HAVE_HAL */

#ifdef HAVE_HA
  nsm_bridge_cal_modify_nsm_bridge (bridge);
#endif /* HAVE_HA */

  bridge->learning = NSM_LEARNING_BRIDGE_UNSET;
  return 0;
}

bool_t
nsm_bridge_is_vlan_aware (struct nsm_bridge_master *master, char *name)
{
  struct nsm_bridge *bridge;

  if (! master)
    return PAL_FALSE;

  bridge = nsm_lookup_bridge_by_name (master, name);

  if (! bridge)
    return PAL_FALSE;

  return ( ((bridge->type == NSM_BRIDGE_TYPE_STP_VLANAWARE) ||
            (bridge->type == NSM_BRIDGE_TYPE_RSTP_VLANAWARE) ||
            (bridge->type == NSM_BRIDGE_TYPE_RPVST_PLUS) ||
            (bridge->type == NSM_BRIDGE_TYPE_PROVIDER_MSTP) ||
            (bridge->type == NSM_BRIDGE_TYPE_PROVIDER_RSTP) ||
            (bridge->type == NSM_BRIDGE_TYPE_MSTP) || 
            (bridge->type == NSM_BRIDGE_TYPE_BACKBONE_RSTP) || 
            (bridge->type == NSM_BRIDGE_TYPE_BACKBONE_MSTP))? 
           PAL_TRUE : PAL_FALSE );  
}

bool_t
nsm_bridge_is_gmrp_enabled (struct nsm_bridge_master *master, char *name)
{
  struct nsm_bridge *bridge;

  if (! master)
    return PAL_FALSE;

  bridge = nsm_lookup_bridge_by_name (master, name);
  if (! bridge)
    return PAL_FALSE;
#ifdef HAVE_GMRP
  return ( bridge->gmrp_bridge ? PAL_TRUE : PAL_FALSE );
#else
  return PAL_FALSE;
#endif /* HAVE_GMRP */
}

bool_t
nsm_bridge_is_igmp_snoop_enabled (struct nsm_bridge_master *master, char *name)
{
  struct nsm_bridge *bridge;

  if (! master)
    return PAL_FALSE;

  bridge = nsm_lookup_bridge_by_name (master, name);
  if (! bridge)
    return PAL_FALSE;
#ifdef HAVE_IGMP_SNOOP
  return PAL_FALSE;
#else
  return PAL_FALSE;
#endif /* HAVE_IGMP_SNOOP */
}

/* Bridge port add event. */
static int
_nsm_bridge_port_add (struct interface *ifp)
{
  struct nsm_if *zif;
  struct nsm_bridge *bridge;
  struct nsm_msg_bridge_if msg;
  int nbytes, i;
  struct nsm_server_client *nsc;
  struct nsm_server_entry *nse;
  struct nsm_bridge_listener *brlistnr;
  struct nsm_bridge_port *br_port = NULL;
  struct listnode *ln;
#ifdef HAVE_VLAN
  unsigned char  priority;
#endif

  if (ifp == NULL)
    return -1;

  zif = (struct nsm_if *) ifp->info;

  if (zif == NULL || zif->switchport == NULL)
    return -1;

  bridge = zif->bridge;

  if (bridge == NULL)
    return -1;

  br_port = zif->switchport;

  LIST_LOOP (bridge->bridge_listener_list, brlistnr, ln)
    {
      if(brlistnr->add_port_to_bridge_func)
        {
          (brlistnr->add_port_to_bridge_func) (bridge->master,
                                               bridge->name, ifp);
        }

    }
  
  /* Populate the bridge_name on the interface */
  pal_strncpy (ifp->bridge_name, bridge->name, NSM_BRIDGE_NAMSIZ + 1);

  /* Send message to PM. */
  NSM_SERVER_CLIENT_LOOP (ng, i, nsc, nse)
    if (NSM_CHECK_CTYPE (nse->service.bits, NSM_SERVICE_BRIDGE))
      {
        pal_mem_set (&msg, 0, sizeof (struct nsm_msg_bridge_if));
        pal_strncpy (msg.bridge_name, bridge->name, NSM_BRIDGE_NAMSIZ);
        msg.num = 1;

        /* Allocate ifindex array. */
        msg.ifindex = XCALLOC (MTYPE_TMP, msg.num * sizeof (u_int32_t));
        msg.ifindex[0] = ifp->ifindex;

        /* Set nse pointer and size. */
        nse->send.pnt = nse->send.buf + NSM_MSG_HEADER_SIZE;
        nse->send.size = nse->send.len - NSM_MSG_HEADER_SIZE;
        msg.spanning_tree_disable = br_port->spanning_tree_disable;

        /* Encode NSM bridge interface message. */
        nbytes = nsm_encode_bridge_if_msg (&nse->send.pnt, &nse->send.size,
                                           &msg);
        if (nbytes < 0)
          return nbytes;

        /* Send bridge interface message. */
        nsm_server_send_message (nse, 0, 0, NSM_MSG_BRIDGE_ADD_PORT, 0, nbytes);
        /* Free ifindex array. */
        XFREE (MTYPE_TMP, msg.ifindex);
      }

#if defined HAVE_VLAN && defined HAVE_HAL

  br_port->vlan_port.default_user_priority = 0;
  pal_mem_cpy (br_port->vlan_port.traffic_class_table,
               nsm_default_traffic_class_table,
               HAL_BRIDGE_MAX_TRAFFIC_CLASS *
               HAL_BRIDGE_MAX_USER_PRIO * sizeof (u_char));

  for (priority = 0; priority <= HAL_BRIDGE_MAX_USER_PRIO; priority++)
    {
      br_port->vlan_port.user_prio_regn_table[priority] = priority;
    }

  /* If the bridge is Vlan-aware, set the default switchport-mode to access */
  if (NSM_BRIDGE_VLAN_AWARE (bridge))
    {
      /* bridge_port_add will be called for lacp aggregate members if they
         exist, hence we don't need to iterate here */

#if defined HAVE_I_BEB && defined HAVE_B_BEB
      /* Skipping for pbb-logical-interface as the mode will directly be set to
       * PIP and CBP */

      if (ifp->hw_type != IF_TYPE_PBB_LOGICAL)
#endif /* HAVE_I_BEB && HAVE_B_BEB */
        NSM_VLAN_SET_PORT_DEFAULT_MODE (ifp, PAL_FALSE, PAL_TRUE);
    }
  else
    {
      nsm_bridge_static_fdb_config_activate (bridge->master,  bridge->name,
                                             NSM_VLAN_NULL_VID, ifp);
    }

#else
  nsm_bridge_static_fdb_config_activate (bridge->master,  bridge->name,
                                         NSM_VLAN_NULL_VID, ifp);
#endif /* HAVE_VLAN && HAVE_HAL */

#ifdef HAVE_LACPD
  {
    u_int16_t key = ifp->lacp_admin_key;

    nsm_lacp_if_admin_key_set (ifp);
    if (key != ifp->lacp_admin_key)
      nsm_server_if_lacp_admin_key_update (ifp);
  }
#endif /* HAVE_LACPD */

  LIST_LOOP (bridge->bridge_listener_list, brlistnr, ln)
    {
      if (brlistnr->activate_port_config_func)
        {
          (brlistnr->activate_port_config_func)(bridge->master, ifp);
        }
    }

#ifdef HAVE_ONMD
  if (bridge->type == NSM_BRIDGE_TYPE_STP
      || bridge->type == NSM_BRIDGE_TYPE_STP_VLANAWARE)
    nsm_oam_lldp_if_set_protocol_list (ifp, NSM_LLDP_PROTO_STP,
                                       PAL_TRUE);
  else if (bridge->type == NSM_BRIDGE_TYPE_RSTP
           || bridge->type == NSM_BRIDGE_TYPE_PROVIDER_RSTP
           || bridge->type == NSM_BRIDGE_TYPE_RSTP_VLANAWARE
           || bridge->type == NSM_BRIDGE_TYPE_RPVST_PLUS)
    nsm_oam_lldp_if_set_protocol_list (ifp, NSM_LLDP_PROTO_RSTP,
                                       PAL_TRUE);
  else
    nsm_oam_lldp_if_set_protocol_list (ifp, NSM_LLDP_PROTO_MSTP,
                                       PAL_TRUE);
#endif /* HAVE_ONMD */
   
#ifdef HAVE_DCB
   if (bridge->dcbg && 
       CHECK_FLAG (bridge->dcbg->dcb_global_flags, NSM_DCB_ENABLE))
     {
       if (nsm_dcb_init_interface (ifp->vr->id, ifp->name) == NULL)
          return NSM_DCB_API_SET_ERR_DCB_IF_INIT;
     }
#endif /* HAVE_DCB */

  return 0;
}

/* Bridge port add. */
/* Private api should not be called from outside */
int
nsm_bridge_port_add (struct nsm_bridge_master *master, char *name,
                     struct interface *ifp, bool_t notify_kernel,
                     u_int8_t spanning_tree_disable)
{
  int ret;
#ifdef HAVE_HAL
#if defined HAVE_LACPD && defined HAVE_VLAN
  size_t size;
#endif /* HAVE_LACPD && HAVE_VLAN */
#endif /* HAVE_HAL */
  struct nsm_if *zif;
  struct nsm_if *tmpzif;
  struct interface *tmpifp;
  struct avl_node *avl_port;
  struct nsm_bridge *bridge;
  struct nsm_bridge_port *br_port;
#ifdef HAVE_LACPD
  struct nsm_bridge *default_bridge;
  struct nsm_bridge *port_bridge;
#endif /* HAVE_LACPD */

  zif = (struct nsm_if *)ifp->info;

  if (! zif)
    return NSM_BRIDGE_ERR_NOTFOUND;

  if (zif->bridge)
    return NSM_BRIDGE_ERR_ALREADY_BOUND;

  bridge = nsm_lookup_bridge_by_name (master, name);

  if (! bridge)
    return NSM_BRIDGE_ERR_NOTFOUND;

  /* Check if already exists. */
  for (avl_port = avl_top (bridge->port_tree); avl_port;
       avl_port = avl_next (avl_port))
    {
      if ((br_port = (struct nsm_bridge_port *)
                             AVL_NODE_INFO (avl_port)) == NULL)
        continue;

      if ((tmpzif = br_port->nsmif) == NULL)
        continue;

      if ((tmpifp = tmpzif->ifp) == NULL)
        continue;

      if (tmpifp == ifp)
        return  NSM_BRIDGE_ERR_ALREADY_BOUND;
    }

#ifdef HAVE_HAL
  if ((notify_kernel == PAL_TRUE) && (ifp->ifindex > 0))
    {
      if ((ret = hal_bridge_add_port (bridge->name, ifp->ifindex)) != HAL_SUCCESS)
        {
#if defined HAVE_LACPD && defined HAVE_VLAN
          size = sizeof (struct nsm_bridge_port_conf);
          if NSM_INTF_TYPE_AGGREGATOR(ifp)
            zif->nsm_bridge_port_conf = XCALLOC (MTYPE_TMP, size);
          if (zif->nsm_bridge_port_conf)
            zif->nsm_bridge_port_conf->bridge = bridge;
#endif /* HAVE_LACPD && HAVE_VLAN */
            return ret;
        }
    }
#endif /* HAVE_HAL */

  if (ifp->ifindex <= 0)
    return NSM_BRIDGE_ERR_NOTFOUND;

  br_port = nsm_bridge_if_init_switchport (bridge, ifp, NSM_VLAN_NULL_VID);

#ifdef HAVE_LACPD
  if (br_port == NULL)
    {
      /* Strange as we already looped through the bridge and checked for the 
       * Port Membership in the bridge. Intention of the function is to add
       * the interface to the Bridge. If zif->switchport->bridge is Not null
       * deinitialize and reinitializze with proper values.
       */
      if (zif->switchport)
        {
        if (zif->switchport->bridge != NULL)
          {
            /* Get the Default Bridge */  
            default_bridge = nsm_get_default_bridge (master);

            if (!default_bridge)
              return NSM_BRIDGE_ERR_NOTFOUND;
 
            /* If the default bridge is not the same as the bridge on which
             * the port is supposed to be added.
             */ 
            if (default_bridge != bridge)
              {
                /* Check if the port's  bridge is default bridge.
                 * Most Likely it's the Aggregator Port.
                 */
                port_bridge = 
                       nsm_lookup_bridge_by_name (master, 
                                               zif->switchport->bridge->name);

                if ((port_bridge == default_bridge) && 
                     NSM_INTF_TYPE_AGGREGATOR(ifp))
                  {
                     /* Delete switchport data structure holding all
                        switchport config */
                    if (zif->switchport)
                      {
#ifdef HAVE_SNMP
                        bitmap_release_index (default_bridge->port_id_mgr, 
                                              zif->switchport->port_id);
#endif /* HAVE_SNMP */
                        XFREE (MTYPE_NSM_BRIDGE_PORT, zif->switchport);

                        zif->switchport = NULL;
                      }
           
                    /* Below we are going to Initialize the Zif Switchport
                       and Bridge as well. */
                    br_port = nsm_bridge_if_init_switchport (bridge, ifp, 
                                                           NSM_VLAN_NULL_VID);
                    if (br_port == NULL)
                       return NSM_BRIDGE_ERR_NOTFOUND; 
                   }
              } 
        }
      }
      else
        return PAL_FALSE;
    }

#endif /* HAVE_LACPD */

  /* Bind it. */
  zif->bridge = bridge;
  
  if (br_port == NULL)
    return NSM_BRIDGE_ERR_NOTFOUND;

#ifdef HAVE_SNMP
  if (ifp->hw_type != IF_TYPE_PBB_LOGICAL)
    ret = bitmap_request_index (bridge->port_id_mgr, &br_port->port_id);
#endif /* HAVE_SNMP */

  if (br_port)
    br_port->state  = NSM_BRIDGE_PORT_STATE_DISABLED;
#if defined HAVE_I_BEB && defined HAVE_B_BEB
  /* this is required since nsm_bridge_if_init_switchport is already
     called while adding a logical interface */
  else if (zif->switchport)
    br_port = zif->switchport;
#endif /* HAVE_I_BEB && HAVE_B_BEB */
  else
    return  NSM_BRIDGE_ERR_ALREADY_BOUND;

  br_port->spanning_tree_disable = spanning_tree_disable;
 
#ifdef HAVE_HAL 
#ifdef HAVE_QOS
  br_port->num_cosq = bridge->num_cosq;
#endif /* HAVE_QOS */
#endif /* HAVE_HAL */

  /* Add port to the list. */
  avl_insert (bridge->port_tree, br_port);

  /* Add port to bridge. */
  _nsm_bridge_port_add (ifp);

  return 0;
}


/*
** The flags are interpreted in the foll manner:
** iterate_members -- This flag is only for aggregator ports and doesn't
**                    have any meaning otherwise.
** notify_kernel   -- In some situations we may want to add properties to
**                    nsm but not notify the kernel (hal/hsl). This flag
**                    is used to control that.
*/
int
nsm_bridge_api_port_add (struct nsm_bridge_master *master, char *name,
                         struct interface *ifp, bool_t iterate_members,
                         bool_t notify_kernel, bool_t exp_bridge_grpd,
                         u_int8_t spanning_tree_disable)
{
  int ret;
  struct nsm_if *nif;
  struct nsm_if *tempnif;
  struct interface *tmpifp;
  struct avl_node *avl_port;
  struct nsm_bridge *bridge;
  struct nsm_bridge_port *br_port;
#ifdef HAVE_LACPD
  struct interface *memifp;
  struct listnode *lsn;
#endif /* HAVE LACP */

  nif = (struct nsm_if *)ifp->info;
  if (! nif)
    return NSM_BRIDGE_ERR_NOTFOUND;

  if (nif->bridge)
    {
      if (!(nif->bridge->is_default))
        return NSM_BRIDGE_ERR_ALREADY_BOUND;

      if (! strncmp (nif->bridge->name, name, NSM_BRIDGE_NAMSIZ))
        return NSM_BRIDGE_ERR_ALREADY_BOUND;

      if (nif->bridge->is_default)
        {
          ret = nsm_bridge_port_delete (master, nif->bridge->name, ifp, PAL_TRUE, PAL_TRUE);
          if (ret < 0)
            {
              return NSM_BRIDGE_ERR_ALREADY_BOUND;
            }
        }
    }

  bridge = nsm_lookup_bridge_by_name (master, name);
  if (! bridge)
    return NSM_BRIDGE_ERR_NOTFOUND;

  /* Check if already exists. */
  for (avl_port = avl_top (bridge->port_tree); avl_port;
       avl_port = avl_next (avl_port))
    {
      if ((br_port = (struct nsm_bridge_port *)
                             AVL_NODE_INFO (avl_port)) == NULL)
        continue;

      if ((tempnif = br_port->nsmif) == NULL)
        continue;

      if ((tmpifp = tempnif->ifp) == NULL)
        continue;

      if (tmpifp == ifp)
        return  NSM_BRIDGE_ERR_ALREADY_BOUND;
    }

#ifdef HAVE_LACPD
  if ((NSM_INTF_TYPE_AGGREGATOR(ifp)) && (iterate_members == PAL_TRUE))
    {
      pal_strncpy (ifp->bridge_name, name, NSM_BRIDGE_NAMSIZ);
      AGGREGATOR_MEMBER_LOOP (nif, memifp, lsn)
        {
          NSM_BRIDGE_CHECK_AGG_MEM_L2_PROPERTY(memifp);
          memifp->agg_param_update = 1;
          ret = nsm_bridge_port_add (master, ifp->bridge_name, memifp,
                                     notify_kernel, spanning_tree_disable);
          if (ret < 0)
            {
              zlog_err(NSM_ZG,
                       "Bridge Port add failed(%d) on aggregator member %s\n",
                       ret, memifp->name);
            }
          memifp->agg_param_update = 0;
        }
    }
#endif /* HAVE_LACPD */

  ret = nsm_bridge_port_add (master, name, ifp, notify_kernel,
                             spanning_tree_disable);
  if (ret < 0)
    return ret;

#ifdef HAVE_LACPD
  nif->exp_bridge_grpd = exp_bridge_grpd;
#endif

  return 0;
}

/* Bridge port event enable/disable spanning-tree. */
static int
nsm_bridge_port_update_spanning_tree_enable (struct interface *ifp, 
                                             char *bridge_name,
                                             u_int8_t spanning_tree_disable)
{
  struct nsm_msg_bridge_if msg;
  struct nsm_server_client *nsc;
  struct nsm_server_entry *nse; 
  int nbytes, i;

  if (ifp == NULL)
    return RESULT_ERROR;

  if (bridge_name == NULL)
    return RESULT_ERROR;

 
  /* Send message to PM. */
  NSM_SERVER_CLIENT_LOOP (ng, i, nsc, nse)
  if (NSM_CHECK_CTYPE (nse->service.bits, NSM_SERVICE_BRIDGE))
    {
      pal_mem_set (&msg, 0, sizeof (struct nsm_msg_bridge_if));
      pal_strncpy (msg.bridge_name, bridge_name, NSM_BRIDGE_NAMSIZ);
      msg.num = 1;

      /* Allocate ifindex array. */
      msg.ifindex = XCALLOC (MTYPE_TMP, msg.num * sizeof (u_int32_t));
      msg.ifindex[0] = ifp->ifindex;

      /* Set nse pointer and size. */
      nse->send.pnt = nse->send.buf + NSM_MSG_HEADER_SIZE;
      nse->send.size = nse->send.len - NSM_MSG_HEADER_SIZE;
      msg.spanning_tree_disable = spanning_tree_disable;

      /* Encode NSM bridge interface message. */
      nbytes = nsm_encode_bridge_if_msg (&nse->send.pnt, &nse->send.size,
                                         &msg);
      if (nbytes < 0)
        return nbytes;

      /* Send bridge interface message. */
      nsm_server_send_message (nse, 0, 0, 
                               NSM_MSG_BRIDGE_PORT_SPANNING_TREE_ENABLE,
                               0, nbytes);

      /* Free ifindex array. */
      XFREE (MTYPE_TMP, msg.ifindex);
    }
  return RESULT_OK;
}

/* This function is used  for enable/disable spanning-tree */
int
nsm_bridge_api_port_spanning_tree_enable (struct nsm_bridge_master *master, 
                                          char *bridge_name, 
                                          struct interface *ifp, 
                                          u_int8_t spanning_tree_disable)
{
  struct nsm_if *nif = NULL;
  struct nsm_bridge_port *br_port = NULL;
  int ret = RESULT_OK;
 
  if (!ifp)
   return NSM_BRIDGE_ERR_NOTFOUND;
 
  nif = (struct nsm_if *)ifp->info;
  if (! nif)
    return NSM_BRIDGE_ERR_NOTFOUND;

  if (nif->bridge)
    {
      if (nif->switchport)
        br_port = nif->switchport;

      /* check to see wheather the given bridge exist*/
      if (bridge_name)
        {
          if (pal_strncmp (nif->bridge->name, bridge_name, 
                           NSM_BRIDGE_NAMSIZ) != 0)
            {
              if (nif->bridge->is_default)
                {
                  /* First time add bridge to interface, iterate if interface
                     is aggregator notify kernel, pass exp_bridge_grpd as true 
                     since this interface is explicitly added by giving a 
                     bridge-group command.
                  */
                  ret = nsm_bridge_api_port_add (master, bridge_name, ifp, 
                                                 PAL_TRUE, PAL_TRUE, PAL_TRUE,
                                                 spanning_tree_disable);
                }
              else
                return NSM_BRIDGE_ERR_NOTFOUND;
            }
          else
            {
              if (br_port)
                {
                  /* Return error if the old spanning-tree mode is equal to 
                     new spanning-tree mode */
                  if (br_port->spanning_tree_disable == spanning_tree_disable)
                    return  NSM_BRIDGE_ERR_EXISTS;
                }
          
              /* Update the port spanning-tree mode */
              br_port->spanning_tree_disable = spanning_tree_disable;

              /* Send the message spanning-tree enable/disable */
              nsm_bridge_port_update_spanning_tree_enable (ifp, bridge_name,
                                                         spanning_tree_disable); 
            }
        }
      else
        return NSM_BRIDGE_ERR_NOTFOUND; 
    }
  else 
    {
      /* First time add bridge group to interface, iterate if interface 
         is aggregator notify kernel, pass exp_bridge_grpd as true since 
         this interface is explicitly added by giving a bridge-group command 
       */
      ret = nsm_bridge_api_port_add (master, bridge_name, ifp, PAL_TRUE,
                                     PAL_TRUE, PAL_TRUE,
                                     spanning_tree_disable);
    }

  return ret;
}

#ifdef HAVE_SMI

/* Get bridge type */
int
nsm_api_bridge_get_type (struct nsm_bridge_master *master,
                         char * name, enum smi_bridge_type *type,
                         enum smi_bridge_topo_type *topo_type)
{
  struct nsm_bridge *bridge = NULL;

  bridge = nsm_lookup_bridge_by_name (master, name);
  if (! bridge)
    return NSM_BRIDGE_ERR_NOTFOUND;

  *type = bridge->type;
  *topo_type = bridge->topology_type;

  return RESULT_OK;
}

int
nsm_set_sw_reset ()
{
#ifdef HAVE_HAL
  hal_if_set_sw_reset ();
#endif
  return RESULT_OK;
}

#endif /* HAVE_SMI */

int
nsm_add_if_to_def_bridge (struct interface *ifp)
{
  struct apn_vr *vr;
  struct nsm_master *nm;
  struct nsm_bridge *bridge;
  struct nsm_bridge_master *master;

  vr = apn_vr_get_privileged (nzg);

  nm = vr->proto;

  master = nsm_bridge_get_master (nm);

  if (! master)
    return NSM_BRIDGE_ERR_NOTFOUND;

  bridge = nsm_get_default_bridge (master);

  if (! bridge)
    return NSM_BRIDGE_ERR_NOTFOUND;

  return nsm_bridge_api_port_add (master, bridge->name, ifp, PAL_TRUE,
                                  PAL_TRUE, PAL_TRUE, PAL_FALSE);

}

void
nsm_bridge_if_send_state_sync_req (struct nsm_bridge *bridge,
                                   struct interface *ifp)
{
  struct nsm_msg_bridge_if msg;
  int nbytes, i;
  struct nsm_server_client *nsc;
  struct nsm_server_entry *nse;

  /* Send message to PM. */
  NSM_SERVER_CLIENT_LOOP (ng, i, nsc, nse)
    if (NSM_CHECK_CTYPE (nse->service.bits, NSM_SERVICE_BRIDGE))
      {
        pal_mem_set (&msg, 0, sizeof (struct nsm_msg_bridge_if));
        pal_strncpy (msg.bridge_name, bridge->name, NSM_BRIDGE_NAMSIZ);
        msg.num = 1;

        /* Allocate ifindex array. */
        msg.ifindex = XCALLOC (MTYPE_TMP, msg.num * sizeof (u_int32_t));
        if (! msg.ifindex)
          return;
        msg.ifindex[0] = ifp->ifindex;

        /* Set nse pointer and size. */
        nse->send.pnt = nse->send.buf + NSM_MSG_HEADER_SIZE;
        nse->send.size = nse->send.len - NSM_MSG_HEADER_SIZE;

        /* Encode NSM bridge interface message. */
        nbytes = nsm_encode_bridge_if_msg (&nse->send.pnt, &nse->send.size, &msg);
        if (nbytes < 0)
          {
            /* Free ifindex array. */
            XFREE (MTYPE_TMP, msg.ifindex);

            return;
          }

            /* Send bridge interface message. */
        nsm_server_send_message (nse, 0, 0,
                                 NSM_MSG_BRIDGE_PORT_STATE_SYNC_REQ,
                                 0, nbytes);
        /* Free ifindex array. */
        XFREE (MTYPE_TMP, msg.ifindex);
      }
}

void
nsm_bridge_if_add_send_notification (struct nsm_bridge *bridge,
                                     struct interface *ifp)
{

  struct nsm_msg_bridge_if msg;
  int nbytes, i;
  struct nsm_server_client *nsc;
  struct nsm_server_entry *nse;

  /* Send message to PM. */
  NSM_SERVER_CLIENT_LOOP (ng, i, nsc, nse)
    if (NSM_CHECK_CTYPE (nse->service.bits, NSM_SERVICE_BRIDGE))
      {
         pal_mem_set (&msg, 0, sizeof (struct nsm_msg_bridge_if));
         pal_strncpy (msg.bridge_name, bridge->name, NSM_BRIDGE_NAMSIZ);
         msg.num = 1;

         /* Allocate ifindex array. */
         msg.ifindex = XCALLOC (MTYPE_TMP, msg.num * sizeof (u_int32_t));
         if (! msg.ifindex)
           return;
         msg.ifindex[0] = ifp->ifindex;

         /* Set nse pointer and size. */
         nse->send.pnt = nse->send.buf + NSM_MSG_HEADER_SIZE;
         nse->send.size = nse->send.len - NSM_MSG_HEADER_SIZE;

         /* Encode NSM bridge interface message. */
         nbytes = nsm_encode_bridge_if_msg (&nse->send.pnt, &nse->send.size, &msg);
         if (nbytes < 0)
           {
             /* Free ifindex array. */
             XFREE (MTYPE_TMP, msg.ifindex);

             return;
           }

         /* Send bridge interface message. */
         nsm_server_send_message (nse, 0, 0, NSM_MSG_BRIDGE_ADD_PORT, 0, nbytes);
         /* Free ifindex array. */
         XFREE (MTYPE_TMP, msg.ifindex);
      }
}


void
nsm_bridge_if_delete_send_notification (struct nsm_bridge *bridge,
                                        struct interface *ifp)
{
  struct nsm_msg_bridge_if msg;
  int nbytes, i;
  struct nsm_server_client *nsc;
  struct nsm_server_entry *nse;

  /* Send message to PM. */
  NSM_SERVER_CLIENT_LOOP (ng, i, nsc, nse)
    if (NSM_CHECK_CTYPE (nse->service.bits, NSM_SERVICE_BRIDGE))
      {
        pal_mem_set (&msg, 0, sizeof (struct nsm_msg_bridge_if)); 
        pal_strncpy (msg.bridge_name, bridge->name, NSM_BRIDGE_NAMSIZ);
        msg.num = 1;

        /* Allocate ifindex array. */
        msg.ifindex = XCALLOC (MTYPE_TMP, msg.num * sizeof (u_int32_t));
        if (! msg.ifindex)
          return;
        msg.ifindex[0] = ifp->ifindex;

        /* Set nse pointer and size. */
        nse->send.pnt = nse->send.buf + NSM_MSG_HEADER_SIZE;
        nse->send.size = nse->send.len - NSM_MSG_HEADER_SIZE;

        /* Encode NSM bridge interface message. */
        nbytes = nsm_encode_bridge_if_msg (&nse->send.pnt, &nse->send.size, &msg);
        if (nbytes < 0)
          {
            /* Free ifindex array. */
            XFREE (MTYPE_TMP, msg.ifindex);

            return;
          }

        /* Send bridge interface message. */
        nsm_server_send_message (nse, 0, 0, NSM_MSG_BRIDGE_DELETE_PORT, 0, nbytes);
        /* Free ifindex array. */
        XFREE (MTYPE_TMP, msg.ifindex);
      }
}



/* Bridge port delete event. */
static int
_nsm_bridge_port_delete (struct interface *ifp, u_int8_t remove_avl_node)
{
  struct nsm_if *zif;
  struct nsm_bridge *bridge;
  struct nsm_bridge_port *br_port;
#ifdef HAVE_GELS
  struct nsm_vlan_port *vlan_port;
#endif
#ifdef HAVE_DCB  
  int ret;
#endif /* HAVE_DCB */  

  if (! ifp)
    return -1;

  zif = (struct nsm_if *) ifp->info;

  if (! zif)
    return -1;

  if (! zif->bridge)
    return -1;

  bridge = zif->bridge;

#ifdef HAVE_DCB
   if (bridge->dcbg &&
       CHECK_FLAG (bridge->dcbg->dcb_global_flags, NSM_DCB_ENABLE))
     {
       if ((ret = nsm_dcb_deinit_interface (ifp->vr->id, ifp->name)) < 0)
          return ret;
     }
#endif /* HAVE_DCB */

  zif->bridge = NULL;

  nsm_bridge_if_delete_send_notification (bridge, ifp);

  br_port = zif->switchport;

#if defined HAVE_PBB_TE
#if defined (HAVE_I_BEB) && defined (HAVE_B_BEB)
#if defined HAVE_GMPLS && defined HAVE_GELS

  if(br_port)
  {
    vlan_port = &br_port->vlan_port;

    if(vlan_port->mode == NSM_VLAN_PORT_MODE_CBP)
    {
      XFREE (MTYPE_NSM_BRIDGE_PORT, vlan_port->cbp_vlan_alloc_map1);
      XFREE (MTYPE_NSM_BRIDGE_PORT, vlan_port->cbp_vlan_alloc_map2);
    }
  }
#endif
#endif
#endif

  if (bridge->port_tree && remove_avl_node == PAL_TRUE)
    avl_remove (bridge->port_tree, br_port);

  br_port->bridge = NULL;
  br_port->state  = NSM_BRIDGE_PORT_STATE_DISABLED;

#ifdef HAVE_LACPD
  {
    u_int16_t key = ifp->lacp_admin_key;
    nsm_lacp_if_admin_key_set (ifp);
    if (key != ifp->lacp_admin_key)
      nsm_server_if_lacp_admin_key_update (ifp);
    zif->exp_bridge_grpd = PAL_FALSE;
  }
#endif /* HAVE_LACPD */

  nsm_bridge_delete_all_fdb_by_port( bridge, ifp->info, PAL_FALSE);

#ifdef HAVE_HAL
  hal_bridge_delete_port(bridge->name, ifp->ifindex);
#endif /* HAVE_HAL */
  nsm_bridge_static_fdb_config_free (ifp);

#ifdef HAVE_VLAN
  /* Add to Vlan */
  /* see old l2_vlan_br_del_if. TBD */
#endif /* HAVE_VLAN */

  /* Delete switchport data structure holding all switchport config */
  if (zif->switchport)
    {
#ifdef HAVE_RATE_LIMIT
      nsm_ratelimit_if_deinit(ifp);
#endif /* HAVE_RATE_LIMIT */

#ifdef HAVE_HA
      nsm_bridge_cal_delete_nsm_bridge_port (zif->switchport);
#endif /* HAVE_HA */
#ifdef HAVE_SNMP
      bitmap_release_index (bridge->port_id_mgr, zif->switchport->port_id);
#endif /* HAVE_SNMP */
      XFREE (MTYPE_NSM_BRIDGE_PORT, zif->switchport);
      zif->switchport = NULL;
    }

  return 0;
}

int
nsm_bridge_if_delete (struct nsm_bridge *bridge,
                      struct interface *ifp,
                      u_int8_t remove_avl_node)
{
  struct nsm_if *zif;
  struct nsm_bridge_listener *brlistnr;
  struct listnode *ln;

  if (! ifp || ! bridge)
    return NSM_ERR_INTERNAL;

  zif = (struct nsm_if *)ifp->info;
  if (! zif)
    return NSM_BRIDGE_ERR_NOTFOUND;

  if (! zif->bridge)
    return NSM_BRIDGE_ERR_NOT_BOUND;

  if (zif->bridge != bridge)
    return NSM_BRIDGE_ERR_MISMATCH;

  LIST_LOOP (bridge->bridge_listener_list, brlistnr, ln)
    {
      if(brlistnr->delete_port_from_bridge_func)
        {
          (brlistnr->delete_port_from_bridge_func)(bridge->master, bridge->name, ifp);
        }
    }

#ifdef HAVE_VLAN
  /* Clear Vlan memberships */
  nsm_vlan_clear_port (ifp);
#endif /* HAVE_VLAN */

#ifdef HAVE_ONMD
  if (bridge->type == NSM_BRIDGE_TYPE_STP
      || bridge->type == NSM_BRIDGE_TYPE_STP_VLANAWARE)
    nsm_oam_lldp_if_set_protocol_list (ifp, NSM_LLDP_PROTO_STP,
                                       PAL_FALSE);
  else if (bridge->type == NSM_BRIDGE_TYPE_RSTP
           || bridge->type == NSM_BRIDGE_TYPE_PROVIDER_RSTP
           || bridge->type == NSM_BRIDGE_TYPE_RSTP_VLANAWARE
           || bridge->type == NSM_BRIDGE_TYPE_RPVST_PLUS)
    nsm_oam_lldp_if_set_protocol_list (ifp, NSM_LLDP_PROTO_RSTP,
                                       PAL_FALSE);
  else
    nsm_oam_lldp_if_set_protocol_list (ifp, NSM_LLDP_PROTO_MSTP,
                                       PAL_FALSE);
#endif /* HAVE_ONMD */

  /* Delete interface from bridge. */
  _nsm_bridge_port_delete (ifp, remove_avl_node);

  return 0;
}

/* Bridge port delete. */
int
nsm_bridge_port_delete (struct nsm_bridge_master *master, char *name,
                        struct interface *ifp, bool_t iterate_members,
                        bool_t notify_kernel)
{
  struct nsm_bridge *bridge;
  struct nsm_if *nif;
  int ret;
#ifdef HAVE_LACPD
  struct interface *memifp;
  struct listnode *lsn;
#endif /* HAVE LACP */

  nif = (struct nsm_if *)ifp->info;
  if (! nif)
    return NSM_BRIDGE_ERR_NOTFOUND;

  bridge = nsm_lookup_bridge_by_name (master, name);

  if (! bridge)
    return NSM_BRIDGE_ERR_NOTFOUND;

#ifdef HAVE_LACPD
  if (iterate_members == PAL_TRUE)
    {
      pal_mem_set(ifp->bridge_name, 0, NSM_BRIDGE_NAMSIZ);
      AGGREGATOR_MEMBER_LOOP (nif, memifp, lsn)
        {
          NSM_BRIDGE_CHECK_AGG_MEM_L2_PROPERTY(memifp);

          memifp->agg_param_update = 1;

          ret = nsm_bridge_if_delete (bridge, memifp, PAL_TRUE);

          if (ret < 0)
            {
              zlog_err(NSM_ZG,
                       "Bridge Port delete failed(%d) on aggregator member %s\n",
                       ret, memifp->name);
            }
          memifp->agg_param_update = 0;
        }
    }
#endif /* HAVE_LACPD */

  ret = nsm_bridge_if_delete (bridge, ifp, PAL_TRUE);

  return ret;
}

struct interface * nsm_lookup_port_by_index (struct nsm_bridge *bridge,
                                             u_int32_t ifindex)
{
  struct nsm_bridge_port *tmp_br_port;
  struct nsm_bridge_port br_port;
  struct avl_node *avl_node;

  if ( !bridge )
    return NULL;

  br_port.ifindex = ifindex;
  br_port.svid    = NSM_VLAN_NULL_VID;

  avl_node = avl_search (bridge->port_tree, &br_port);

  if (avl_node == NULL)
    return NULL;

  tmp_br_port = AVL_NODE_INFO (avl_node);

  if (tmp_br_port == NULL)
    return NULL;

  if (tmp_br_port->nsmif == NULL)
    return NULL;

  return tmp_br_port->nsmif->ifp;
}

struct nsm_bridge_port *
nsm_bridge_port_lookup_by_index (struct nsm_bridge *bridge,
                                 u_int32_t ifindex)
{
  struct nsm_bridge_port tmp_br_port;
  struct nsm_bridge_port *br_port = NULL;
  struct avl_node *avl_node;

  if ( !bridge )
    return NULL;

  tmp_br_port.ifindex = ifindex;
  tmp_br_port.svid    = NSM_VLAN_NULL_VID;

  avl_node = avl_search (bridge->port_tree, &tmp_br_port);

  if (avl_node == NULL)
    return NULL;

  br_port = AVL_NODE_INFO (avl_node);

  return br_port;
}

int
nsm_bridge_api_set_port_state_all (struct nsm_master *nm, char *name,
                                   struct interface *ifp, u_int32_t state)
{
  struct nsm_bridge_master *master;
  struct nsm_bridge *bridge;
  u_int32_t instance;

  master = nsm_bridge_get_master(nm);
  bridge = nsm_lookup_bridge_by_name (master, name);
  if ( (! bridge) || (!ifp) )
    return NSM_BRIDGE_ERR_NOTFOUND;

  /* If bridge is mstp iterate over instances, else
     only set for instance 0 */
  if (bridge->type == NSM_BRIDGE_TYPE_MSTP ||
      bridge->type == NSM_BRIDGE_TYPE_RPVST_PLUS)
    {
      for (instance = NSM_BRIDGE_INSTANCE_MIN;
           instance < bridge->max_mst_instances; instance++)
        {
          nsm_bridge_set_port_state(master, name, ifp, state, instance);
        }
    }
  else
    nsm_bridge_set_port_state(master, name, ifp, state,
                              NSM_BRIDGE_INSTANCE_MIN);

  return RESULT_OK;

}

void
nsm_bridge_port_state_set_notification (struct nsm_bridge *bridge,
                                        struct interface *ifp,
                                        u_int32_t state, u_int32_t instance)
{
  struct nsm_msg_bridge_port msg;
  int nbytes, i;
  struct nsm_server_client *nsc;
  struct nsm_server_entry *nse;

  /* Send message to PM. */
  NSM_SERVER_CLIENT_LOOP (ng, i, nsc, nse)
    if (NSM_CHECK_CTYPE (nse->service.bits, NSM_SERVICE_BRIDGE))
      {
        pal_strncpy (msg.bridge_name, bridge->name, NSM_BRIDGE_NAMSIZ);
        msg.ifindex = ifp->ifindex;
        msg.num = 1;
        msg.instance = XCALLOC(MTYPE_TMP, msg.num * sizeof(u_int32_t));
        msg.state = XCALLOC(MTYPE_TMP, msg.num * sizeof(u_int32_t));

        msg.state[0] = state;
        msg.instance[0] = instance;

        /* Set nse pointer and size. */
        nse->send.pnt = nse->send.buf + NSM_MSG_HEADER_SIZE;
        nse->send.size = nse->send.len - NSM_MSG_HEADER_SIZE;

        /* Encode NSM bridge interface message. */
        nbytes = nsm_encode_bridge_port_msg (&nse->send.pnt, &nse->send.size, &msg);
        if (nbytes < 0)
          return;

        /* Send bridge interface message. */
        nsm_server_send_message (nse, 0, 0, NSM_MSG_BRIDGE_PORT_STATE, 0, nbytes);

        XFREE(MTYPE_TMP, msg.instance);
        XFREE(MTYPE_TMP, msg.state);
      }

}

int
nsm_bridge_set_port_state (struct nsm_bridge_master *master, char *name,
                           struct interface *ifp, u_int32_t state,
                           u_int32_t instance)
{
  int ret, ret1;
  int port_state;
  int learn, forward;
  struct nsm_if *zif;
  struct listnode *ln;
  struct nsm_bridge *bridge;
  struct nsm_bridge_port *br_port;
  struct nsm_bridge_listener *brlistnr;

  bridge = nsm_lookup_bridge_by_name (master, name);

  if ( (! bridge) || (!ifp) )
    return NSM_BRIDGE_ERR_NOTFOUND;

  zif = (struct nsm_if *)ifp->info;

  if (! zif)
    return NSM_BRIDGE_ERR_NOTFOUND;

  if (! zif->bridge
      || ! zif->switchport)
    return NSM_BRIDGE_ERR_NOT_BOUND;

  if (zif->bridge != bridge)
    return NSM_BRIDGE_ERR_MISMATCH;

  br_port = zif->switchport;

  if (!br_port)
    return NSM_BRIDGE_ERR_NOTFOUND;
      
  if ((state == NSM_BRIDGE_PORT_STATE_DISABLED) ||
      (state == NSM_BRIDGE_PORT_STATE_DISCARDING) ||
      (state == NSM_BRIDGE_PORT_STATE_BLOCKING))
    {
#ifdef HAVE_MPLS_VC
      if (ifp->bind != 0)
        return NSM_SUCCESS;
#endif /* HAVE_MPLS_VC */

      /* flush fdb */
      nsm_bridge_flush_fdb_by_port (master->nm, ifp->name);

    }

    if (state == NSM_BRIDGE_PORT_STATE_DISABLED)
      {
        if (CHECK_FLAG (ifp->flags, IFF_UP))
          {
            ret = nsm_if_flag_up_unset (master->nm->vr->id, ifp->name, PAL_TRUE);
          }
      }
    
    if (state == NSM_BRIDGE_PORT_STATE_ENABLED)
      {
        if (!CHECK_FLAG (ifp->flags, IFF_UP))
          {
            ret = nsm_if_flag_up_set (master->nm->vr->id, ifp->name, PAL_TRUE);
          }
      }    
  /* For edge ports, dont flush fdb, but state has to be discarding */
  if (state == NSM_BRIDGE_PORT_STATE_DISCARDING_EDGE)
    {
#ifdef HAVE_MPLS_VC
      if (ifp->bind != 0)
        return NSM_SUCCESS;
#endif /* HAVE_MPLS_VC */

      state = NSM_BRIDGE_PORT_STATE_DISCARDING;
    }

  port_state = nsm_map_port_msg_state_to_hal (state);

  learn = (port_state == HAL_BR_PORT_STATE_LEARNING ) ||
    (port_state == HAL_BR_PORT_STATE_FORWARDING);

  forward = (port_state == HAL_BR_PORT_STATE_FORWARDING);
  
#ifdef HAVE_HAL
  ret  = hal_bridge_set_port_state (bridge->name, ifp->ifindex,
                                    instance, port_state);
  ret1 = hal_bridge_set_learn_fwd (bridge->name, ifp->ifindex,
                                   instance, learn, forward);

  br_port->state = state;

  if ( ( ret == HAL_SUCCESS ) && ( ret1 == HAL_SUCCESS ) )
    {
      if ( (state == NSM_BRIDGE_PORT_STATE_DISABLED) ||
           (state == NSM_BRIDGE_PORT_STATE_BLOCKING) ||
           (state == NSM_BRIDGE_PORT_STATE_DISCARDING) )
        {
          LIST_LOOP (bridge->bridge_listener_list, brlistnr, ln)
            {
              if(brlistnr->disable_port_func)
                {
                  (brlistnr->disable_port_func)(bridge->master, ifp);
                }
            }
        }
      else if (state == NSM_BRIDGE_PORT_STATE_FORWARDING)
        {
          LIST_LOOP (bridge->bridge_listener_list, brlistnr, ln)
            {
              if(brlistnr->enable_port_func)
                {
                  (brlistnr->enable_port_func)(bridge->master, ifp);
                }
            }
        }
      LIST_LOOP (bridge->bridge_listener_list, brlistnr, ln)
        {
          if(brlistnr->change_port_state_func)
            {
              (brlistnr->change_port_state_func)(bridge->master, ifp);
            }
        }
    }
#endif /* HAVE_HAL */

#ifdef HAVE_MSTPD
  br_port->instance_state[instance] = (u_char) state;
#endif /* HAVE_MSTPD */

#ifdef HAVE_HA
  nsm_bridge_cal_modify_nsm_bridge_port (br_port);
#endif /* HAVE_HA */

  nsm_bridge_port_state_set_notification (bridge, ifp, state, instance);

  return ret;
}

#ifdef HAVE_PBB_TE
int
nsm_bridge_set_pbb_te_port_state (struct nsm_bridge_master *master, u_int32_t eps_grp_id,
                                  u_int32_t tesid_a);

#endif /* HAVE_PBB_TE */

int
nsm_map_port_msg_state_to_hal (u_int32_t state)
{
  switch(state)
    {
    case NSM_BRIDGE_PORT_STATE_DISABLED:
      return HAL_BR_PORT_STATE_DISABLED;
      break;
    case NSM_BRIDGE_PORT_STATE_LISTENING:
      return HAL_BR_PORT_STATE_LISTENING;
      break;
    case NSM_BRIDGE_PORT_STATE_LEARNING:
      return HAL_BR_PORT_STATE_LEARNING;
      break;
    case NSM_BRIDGE_PORT_STATE_FORWARDING:
      return HAL_BR_PORT_STATE_FORWARDING;
      break;
    case NSM_BRIDGE_PORT_STATE_DISCARDING:
    case NSM_BRIDGE_PORT_STATE_BLOCKING:
      return HAL_BR_PORT_STATE_BLOCKING;
      break;
    default:
      return HAL_BR_PORT_STATE_MAX;
      break;
    }
}

struct nsm_bridge_port *
nsm_bridge_if_init_switchport (struct nsm_bridge *bridge,
                               struct interface *ifp, u_int16_t svid)
{
  struct nsm_if *zif;
  struct nsm_bridge_port *switch_port = NULL;

  if (!(zif = ifp->info) || ((zif->switchport)&&(zif->switchport->bridge != NULL)))
    return NULL;

  /* Add switchport data structure to hold all switchport config */
  switch_port = XCALLOC (MTYPE_NSM_BRIDGE_PORT,
                         sizeof (struct nsm_bridge_port));
  if (!switch_port)
    {
      zlog_info(NSM_ZG, "NSM out of memory");
      return NULL;
    }

  switch_port->svid = svid;
  switch_port->nsmif = zif;
  switch_port->bridge = bridge;
  switch_port->ifindex = ifp->ifindex;
  zif->switchport = switch_port;
  
  pal_mem_set (&switch_port->uni_type_status, 0,
      sizeof (struct nsm_uni_type_status));

#ifdef HAVE_RATE_LIMIT
  nsm_ratelimit_if_init(ifp);
#endif /* HAVE_RATE_LIMIT */

#ifdef HAVE_HA
  nsm_bridge_cal_create_nsm_bridge_port (switch_port);
#endif /* HAVE_HA */

  return switch_port;
}

/* Remove Mirror configuration */
static int
port_mirroring_config_remove (struct interface *ifp)
{
  struct listnode *current,*next;
  struct pm_node *node;
  struct interface *ifp_to, *ifp_from;
  int ret;
  int direction = HAL_PORT_MIRROR_DIRECTION_BOTH;

  LIST_LOOP_DEL (port_mirror_list, node, current, next)
    {
      ret = 0;
      ifp_to = ifp_from = NULL;

      if (ifp->ifindex == node->to->ifindex)
        {
         ifp_to = ifp;
         ifp_from = if_lookup_by_index (&ifp->vr->ifm, node->from->ifindex);
        }
      else if (ifp->ifindex == node->from->ifindex)
        {
          ifp_to = if_lookup_by_index (&ifp->vr->ifm, node->to->ifindex);
          ifp_from = ifp;
        }
      else
        continue;

      if (ifp_from == NULL || ifp_to == NULL)
        {
           zlog_info (NSM_ZG,
                      "Error in finding the interface\n");
           return -1;
        }

      ret = port_mirroring_list_del (ifp_to, ifp_from,
                                     &direction,
                                     NSM_ZG);

      if (ret < 0)
        {
           zlog_info (NSM_ZG,
                      "Error in removing the mirror configuration %s\n",
                       ifp->name);
           return ret;
        }
#ifdef HAVE_HAL
      /* Delete mirroring for port from hardware layer. */
      ret = hal_port_mirror_unset(ifp_to->ifindex, ifp_from->ifindex,
                                  HAL_PORT_MIRROR_DIRECTION_BOTH);

      if (ret < 0)
        {
          if (ret == RESULT_NO_SUPPORT)
            zlog_info (NSM_ZG,
                      "%%Port Mirroring is not supported\n");
          else
            zlog_info (NSM_ZG,
                      "%%Mirrored-to port doesn't exist. \n");
          /* Add mirror port back to list. */
          port_mirroring_list_add (NSM_ZG, port_mirror_list, ifp_to, 
                                   ifp_from, direction);
          return ret;
        }
#endif /* HAVE_HAL */
    }
  return 0;
}

/* Set a port as layer2. */
int
nsm_bridge_if_set_switchport (struct interface *ifp)
{
  struct nsm_if *zif = NULL;
  int toggle_interface = 0;
#ifdef HAVE_HAL
  unsigned int retifindex;
  int ret;
#endif /* HAVE_HAL */

  zif = (struct nsm_if *)(ifp)->info;
  if ( !zif )
    return NSM_INTERFACE_NOT_FOUND;

  if (zif->type == NSM_IF_TYPE_L2)
    {
      zlog_info (NSM_ZG, "interface %s index %d is already a switchport",
                 ifp->name, ifp->ifindex);
      return NSM_INTERFACE_L2_MODE;
    }

#if (defined HAVE_MPLS_VC || defined HAVE_VPLS)
  if (ifp->bind != 0)
    return NSM_INTERFACE_IN_USE;
#endif /* HAVE_MPLS_VC || HAVE_VPLS */

  /* Remove the port mirror configuration if there */
  ret = port_mirroring_config_remove (ifp);
  if (ret < 0)
    {
      zlog_info (NSM_ZG,
                 "Error in removing mirror configuration from interface %s",                      ifp->name);
    }

  if ( if_is_up(ifp) )
    {
      nsm_if_flag_up_unset(ifp->vr->id, ifp->name, PAL_TRUE);
      toggle_interface = 1;
    }

#ifdef HAVE_L3
  /* Unset L3 mode on the interface. */
  nsm_interface_unset_mode_l3(ifp);
#endif /* HAVE_L3 */

#ifdef HAVE_HAL
  ret = hal_if_set_port_type (ifp->name, ifp->ifindex,
                              HAL_IF_SWITCH_PORT, &retifindex);
  if (ret < 0)
    return ret;

  /* Update the interface index.  */
  nsm_if_ifindex_update (ifp, retifindex);
#endif /* HAVE_HAL */

  /* Set L2 mode on the interface. */
  nsm_interface_set_mode_l2 (ifp);

#ifdef HAVE_DEFAULT_BRIDGE
  nsm_add_if_to_def_bridge (ifp);
#endif /* HAVE_DEfAULT_BRIDGE */

#ifdef HAVE_VLAN
  nsm_vlan_add_default (ifp);
#endif /* HAVE_VLAN */

  if (toggle_interface)
    nsm_if_flag_up_set (ifp->vr->id, ifp->name, PAL_TRUE);

  return 0;
}

/* Set a port as layer3. */
int
nsm_bridge_if_unset_switchport (struct interface *ifp)
{
  struct nsm_if *zif;
  int toggle_interface = 0;
#ifdef HAVE_HAL
  unsigned int retifindex;
  int ret;
#endif /* HAVE_HAL */

  if (! ifp)
    return NSM_ERR_INVALID_ARGS;

  /* delete interface from bridge */
  zif = ifp->info;
  if (! zif)
    return NSM_INTERFACE_NOT_FOUND;

  if (zif->type != NSM_IF_TYPE_L2)
    {
      zlog_info (NSM_ZG, "interface %s index %d is not a switchport",
                 ifp->name, ifp->ifindex);
      return -1;
    }

#if (defined HAVE_MPLS_VC || defined HAVE_VPLS)
  if (ifp->bind != 0)
    return NSM_INTERFACE_IN_USE;
#endif /* HAVE_MPLS_VC || HAVE_VPLS */

  if ( if_is_up(ifp) )
    {
      nsm_if_flag_up_unset(ifp->vr->id, ifp->name, PAL_TRUE);
      toggle_interface = 1;
    }

  if (zif->bridge)
    nsm_bridge_if_delete (zif->bridge, ifp, PAL_TRUE);

  /* Unset L2 mode on the interface. */
  nsm_interface_unset_mode_l2(ifp);

  /* Delete switchport data structure holding all switchport config */
  if (zif->switchport)
    {
#ifdef HAVE_RATE_LIMIT
      nsm_ratelimit_if_deinit(ifp);
#endif /* HAVE_RATE_LIMIT */
      XFREE (MTYPE_NSM_BRIDGE_PORT, zif->switchport);
      zif->switchport = NULL;
    }

#ifdef HAVE_HAL
  ret = hal_if_set_port_type (ifp->name, ifp->ifindex,
                              HAL_IF_ROUTER_PORT, &retifindex);
  if (ret < 0)
    return ret;

  /* Update the interface index.  */
  nsm_if_ifindex_update (ifp, retifindex);
#endif /* HAVE_HAL */

#ifdef HAVE_L3
  /* Set L3 mode on the interface. */
  nsm_interface_set_mode_l3 (ifp);
#endif /* HAVE_L3 */

  if (toggle_interface)
    nsm_if_flag_up_set (ifp->vr->id, ifp->name, PAL_TRUE);

  return 0;
}

/* Process Bridge TCN message. */
int
nsm_server_recv_bridge_tcn(struct nsm_msg_header *header,
                           void *arg, void *message)
{
  struct listnode *ln = NULL;
  struct nsm_master *nm = NULL;
  struct nsm_bridge *br = NULL;
  struct nsm_server *ns = NULL;
  struct nsm_msg_bridge *msg = NULL;
  struct nsm_server_entry *nse = NULL;
  struct nsm_bridge_master *master = NULL;
  struct nsm_bridge_listener *br_appln = NULL;

  /* Set connection and message.  */
  nse = arg;
  ns = nse->ns;
  msg = message;

  /* Debug.  */
  if (IS_NSM_DEBUG_RECV)
    nsm_bridge_dump (ns->zg, msg);

  nm = nsm_master_lookup_by_id (nzg, header->vr_id);
  
  if (nm)
    master = nsm_bridge_get_master (nm);

  if (master == NULL)
    return 0;

  br = nsm_lookup_bridge_by_name(master, msg->bridge_name);
  if (br == NULL)
    return 0;

  /* Notify all listeners about TCN message received */
  LIST_LOOP(br->bridge_listener_list, br_appln, ln)
    {
      if( (br_appln) &&
          (br_appln->topology_change_notify_func) )
        {
          (br_appln->topology_change_notify_func)(master, msg->bridge_name);
        }
    }
  return 0;
}

/* Process Port flush fdb message. */
int
nsm_server_recv_bridge_port_flush_fdb(struct nsm_msg_header *header,
                                      void *arg, void *message)
{
  int index;
  int nbytes;
  struct apn_vr *vr;
  struct interface *ifp;
  struct nsm_server *ns;
  struct nsm_master *nm = NULL;
  struct nsm_server_entry *nse;
  struct nsm_bridge_master *master = NULL; 
  struct nsm_msg_bridge_if *msg;
  struct nsm_msg_bridge_if reply;

  /* Set connection and message.  */
  nse = arg;
  ns = nse->ns;
  msg = message;

  nm = nsm_master_lookup_by_id (nzg, header->vr_id);

  if (nm)
    master = nsm_bridge_get_master (nm);

  if (master == NULL)
    return 0;

  vr = nm->vr;

  for(index = 0; index < msg->num; index++)
    {
      ifp = if_lookup_by_index (&vr->ifm, msg->ifindex[index]);
      if (! ifp)
        continue;

#ifdef HAVE_HAL
      /* Flush FDB by port. */
      hal_bridge_flush_fdb_by_port(msg->bridge_name, ifp->ifindex, 0, 0, 0);
#endif /* HAVE_HAL */
    }

  pal_mem_set (&reply, 0, sizeof (struct nsm_msg_bridge_if));
  pal_strncpy (reply.bridge_name, msg->bridge_name, NSM_BRIDGE_NAMSIZ);
  reply.num = msg->num;

  if (reply.num > 0)
   {
     reply.ifindex = XCALLOC(MTYPE_TMP, reply.num * sizeof(u_int32_t));
     pal_mem_cpy (reply.ifindex, msg->ifindex, reply.num * sizeof(u_int32_t));
   }

  /* Set encode pointer and size.  */
  nse->send.pnt = nse->send.buf + NSM_MSG_HEADER_SIZE;
  nse->send.size = nse->send.len - NSM_MSG_HEADER_SIZE;

  nbytes = nsm_encode_bridge_if_msg (&nse->send.pnt, &nse->send.size, &reply);

  nsm_server_send_message (nse, 0, 0, NSM_MSG_BRIDGE_PORT_FLUSH_FDB,
                           header->message_id, nbytes);

  if (msg->ifindex)
    XFREE (MTYPE_TMP, msg->ifindex);

  if (reply.ifindex)
    XFREE (MTYPE_TMP, reply.ifindex);

  return 0;
}

/* Process Port state update message. */
int
nsm_server_recv_bridge_port_state_update(struct nsm_msg_header *header,
                                         void *arg, void *message)
{
  int i;
  int nbytes;
  struct apn_vr *vr;
  struct interface *ifp;
  struct nsm_server *ns;
  struct nsm_master *nm = NULL;
  struct nsm_server_entry *nse;
  struct nsm_msg_bridge_port *msg;
  struct nsm_bridge_master *master = NULL;
  struct nsm_msg_bridge_port reply;

  /* Set connection and message.  */
  nse = arg;
  ns = nse->ns;
  msg = message;

  nm = nsm_master_lookup_by_id (nzg, header->vr_id);

  if (nm)
    master = nsm_bridge_get_master (nm);

  if (master == NULL)
    goto reply;

  vr = nm->vr;

  ifp = if_lookup_by_index (&vr->ifm, msg->ifindex);
  if (! ifp)
    goto reply;
  for (i = 0; i < msg->num; i++)
    {
      nsm_bridge_set_port_state (master, msg->bridge_name, ifp,
                                 msg->state[i], msg->instance[i]);
    }
 reply :
  pal_mem_set(&reply, 0, sizeof(struct nsm_msg_bridge_port));

  reply.cindex = msg->cindex;
  reply.ifindex = msg->ifindex;
  reply.num = msg->num;

  reply.instance = XCALLOC(MTYPE_TMP, msg->num * sizeof(u_int32_t));
  if (reply.instance && msg->instance)
    pal_mem_cpy (reply.instance, msg->instance,  msg->num * sizeof(u_int32_t));

  reply.state = XCALLOC(MTYPE_TMP, msg->num * sizeof(u_int32_t));
  if (reply.state && msg->state)
    pal_mem_cpy (reply.state , msg->state,  msg->num * sizeof(u_int32_t));

  pal_strncpy(reply.bridge_name, msg->bridge_name, NSM_BRIDGE_NAMSIZ);

  /* Set encode pointer and size.  */
  nse->send.pnt = nse->send.buf + NSM_MSG_HEADER_SIZE;
  nse->send.size = nse->send.len - NSM_MSG_HEADER_SIZE;

  nbytes = nsm_encode_bridge_port_msg (&nse->send.pnt, &nse->send.size, &reply);

  nsm_server_send_message (nse, 0, 0, NSM_MSG_BRIDGE_PORT_STATE,
                           header->message_id, nbytes);

  if (msg->instance)
    XFREE (MTYPE_TMP, msg->instance);
  if (msg->state)
    XFREE (MTYPE_TMP, msg->state);

  if (reply.instance)
    XFREE (MTYPE_TMP, reply.instance);
  if (reply.state)
    XFREE (MTYPE_TMP, reply.state);

  return 0;
}

#if defined HAVE_PBB_TE && defined HAVE_I_BEB && defined HAVE_B_BEB
int
nsm_server_recv_bridge_pbb_te_port_state_update (struct nsm_msg_header *header,
                                                 void *arg, void *message)
{
  struct nsm_server *ns;
  struct nsm_master *nm;
  struct nsm_server_entry *nse;
  struct nsm_msg_bridge_pbb_te_port *msg;
  struct nsm_bridge_master *master;

  /* Set connection and message.  */
  nse = arg;
  ns = nse->ns;
  msg = message;

  nm = nsm_master_lookup_by_id (nzg, header->vr_id);

  if (!nm)
    return -1;

  master = nsm_bridge_get_master (nm);

  if (!master)
    return -1;

  return nsm_bridge_set_pbb_te_port_state (master, msg->eps_grp_id, msg->te_sid_a);
}
#endif /* HAVE_PBB_TE && HAVE_I_BEB && HAVE_B_BEB */

#ifdef HAVE_ONMD
int
nsm_server_recv_req_ifindex (struct nsm_msg_header *header,
                             void *arg, void *message)
{
  struct nsm_master *nm;
  struct nsm_server_entry *nse;
  struct nsm_server *ns;
  struct nsm_msg_cfm_mac *msg;
  struct nsm_msg_cfm_ifindex index;

  /* Set connection and message.  */
  nse = arg;
  ns = nse->ns;
  msg = message;

  nm = nsm_master_lookup_by_id (nzg, header->vr_id);
  if (nm == NULL)
    return 0;

#ifdef HAVE_HAL
  hal_l2_get_index_by_mac_vid_svid (msg->name, &index.index, msg->mac, msg->vid, msg->svid);
#endif /* HAVE_HAL */

  nsm_cfm_send_ifindex (nse, &index, header->message_id);

  return 0;
}

int
nsm_cfm_send_ifindex (struct nsm_server_entry *nse,
                      struct nsm_msg_cfm_ifindex *msg, u_int32_t msg_id)
{
  int nbytes;

  /* Set encode pointer and size.  */
  nse->send.pnt = nse->send.buf + NSM_MSG_HEADER_SIZE;
  nse->send.size = nse->send.len - NSM_MSG_HEADER_SIZE;

  nbytes = nsm_encode_cfm_index (&nse->send.pnt, &nse->send.size, msg);

  nsm_server_send_message (nse, 0, 0, NSM_MSG_CFM_GET_IFINDEX, msg_id, 4);

  return 0;
}
#endif /* HAVE_ONMD */

/* Set NSM server callbacks for Bridge. */
int
nsm_bridge_set_server_callback (struct nsm_server *ns)
{
  nsm_server_set_callback (ns, NSM_MSG_BRIDGE_TCN,
                           nsm_parse_bridge_msg,
                           nsm_server_recv_bridge_tcn);
  nsm_server_set_callback (ns, NSM_MSG_BRIDGE_PORT_FLUSH_FDB,
                           nsm_parse_bridge_if_msg,
                           nsm_server_recv_bridge_port_flush_fdb);
  nsm_server_set_callback (ns, NSM_MSG_BRIDGE_PORT_STATE,
                           nsm_parse_bridge_port_msg,
                           nsm_server_recv_bridge_port_state_update);
  nsm_server_set_callback (ns, NSM_MSG_BRIDGE_SET_AGEING_TIME,
                           nsm_parse_bridge_msg,
                           nsm_server_recv_bridge_set_ageing_time);
  nsm_server_set_callback (ns, NSM_MSG_BRIDGE_SET_STATE,
                           nsm_parse_bridge_enable_msg,
                           nsm_server_recv_bridge_set_state);
#ifdef HAVE_ONMD
  nsm_server_set_callback (ns, NSM_MSG_CFM_REQ_IFINDEX,
                           nsm_parse_cfm_mac_msg,
                           nsm_server_recv_req_ifindex);
#endif /* HAVE_ONMD */

#if defined  HAVE_G8031 || defined HAVE_G8032
  nsm_server_set_callback (ns, NSM_MSG_BLOCK_INSTANCE ,
                           nsm_parse_vlan_msg,
                           nsm_server_recv_block_protection_instance); 
  nsm_server_set_callback (ns, NSM_MSG_UNBLOCK_INSTANCE ,
                           nsm_parse_vlan_msg,
                           nsm_server_recv_unblock_protection_instance); 
#endif /* HAVE_G8031 || defined HAVE_G8032 */

#if defined HAVE_PBB_TE && defined HAVE_I_BEB && defined HAVE_B_BEB
  nsm_server_set_callback (ns, NSM_MSG_BRIDGE_PBB_TE_PORT_STATE,
                           nsm_parse_bridge_pbb_te_port_state_msg,
                           nsm_server_recv_bridge_pbb_te_port_state_update);
#endif /* HAVE_PBB_TE */
  return 0;
}
/* NSM bridge sync. */
static int
nsm_bridge_sync_bridge (struct nsm_master *nm, struct nsm_server_entry *nse, struct nsm_bridge *bridge, int msgid)
{
  struct nsm_msg_bridge msg;
  int nbytes;

  pal_mem_set (&msg, 0 , sizeof (struct nsm_msg_bridge));
  pal_strncpy (msg.bridge_name, bridge->name, NSM_BRIDGE_NAMSIZ);
#if defined HAVE_CFM && (defined HAVE_I_BEB || defined HAVE_B_BEB)
  if (bridge->backbone_edge == NSM_PBB_BACKBONE_EDGE_SET)
    {
      /* PBB bridge will be identified as I-COMP or B-COMP bridge  */
      if ((bridge->type == NSM_BRIDGE_TYPE_PROVIDER_RSTP) || 
          (bridge->type == NSM_BRIDGE_TYPE_PROVIDER_MSTP))
        msg.type = NSM_BRIDGE_TYPE_PBB_I_COMPONENT;
      else
        {
          msg.type = bridge->type;
          /* return -1; */
        }
    }
  else
    msg.type = bridge->type;
    
#else
  msg.type = bridge->type;
#endif /* HAVE_CFM && (HAVE_I_BEB || HAVE_B_BEB)  */
  msg.is_default = bridge->is_default;
  msg.topology_type = bridge->topology_type;

#ifdef HAVE_PROVIDER_BRIDGE
  /* Indicating this is an edge bridge */
  if (((bridge->type == NSM_BRIDGE_TYPE_PROVIDER_RSTP)||
        (bridge->type == NSM_BRIDGE_TYPE_PROVIDER_MSTP)) && 
      (bridge->provider_edge == PAL_TRUE))
    {
      msg.is_edge = PAL_TRUE;
    }
  else
#endif /* HAVE_PROVIDER_BRIDGE */
    msg.is_edge = PAL_FALSE;

  /* Set nse pointer and size. */
  nse->send.pnt = nse->send.buf + NSM_MSG_HEADER_SIZE;
  nse->send.size = nse->send.len - NSM_MSG_HEADER_SIZE;

  /* Encode NSM bridge message. */
  nbytes = nsm_encode_bridge_msg (&nse->send.pnt, &nse->send.size, &msg);
  if (nbytes < 0)
    return nbytes;

  /* Send bridge message. */
  nsm_server_send_message (nse, 0, 0, msgid, 0, nbytes);

  return 0;
}

/* Send the Bridge Interface Messages to Protocol. */
static int
nsm_bridge_interface_sync_message (struct nsm_server_entry *nse, 
                                   struct nsm_msg_bridge_if *msg, 
                                   int msgid, int num)
{
  int nbytes;
     
  msg->num = num;

  /* Set nse pointer and size. */
  nse->send.pnt = nse->send.buf + NSM_MSG_HEADER_SIZE;
  nse->send.size = nse->send.len - NSM_MSG_HEADER_SIZE;  

  /* Encode the bridge ports with stp enabled. */
  nbytes = nsm_encode_bridge_if_msg (&nse->send.pnt, &nse->send.size, msg);
      
  if (nbytes < 0)
    return nbytes;

  /* Send bridge interface message. */
  nsm_server_send_message (nse, 0, 0, msgid, 0, nbytes);

  return nbytes;  
}  

/* NSM bridge interface sync. */
static int
nsm_bridge_sync_bridge_if (struct nsm_master *nm, struct nsm_server_entry *nse,
                           struct nsm_bridge *bridge, int msgid)
{
  int nbytes;
  int num, i;
  int enable_stp_count;
  int disable_stp_count;
  struct avl_node *avl_port;
  struct nsm_msg_bridge_if msg_stp_enable;
  struct nsm_msg_bridge_if msg_stp_disable;
  struct nsm_bridge_port *br_port;
  struct interface *ifp;

  pal_mem_set (&msg_stp_enable, 0, sizeof (struct nsm_msg_bridge_if));
  pal_mem_set (&msg_stp_disable, 0, sizeof (struct nsm_msg_bridge_if));

  pal_strncpy (msg_stp_enable.bridge_name, bridge->name, NSM_BRIDGE_NAMSIZ);
  pal_strncpy (msg_stp_disable.bridge_name, bridge->name, NSM_BRIDGE_NAMSIZ);

  num = AVL_COUNT (bridge->port_tree);

  if (num > 0)
    {
      /* Allocate ifindex array. This represents the ports that are in 
       * STP enabled state
       */
      msg_stp_enable.ifindex = XCALLOC (MTYPE_TMP, num * sizeof (u_int32_t));

      /* Allocate ifindex array. This represents the ports that are in 
       * STP disabled state
       */
      msg_stp_disable.ifindex = XCALLOC (MTYPE_TMP, num * sizeof (u_int32_t));

      i = 0;
      nbytes = 0;
      enable_stp_count = 0;
      disable_stp_count = 0;

      for (avl_port = avl_top (bridge->port_tree); avl_port;
           avl_port = avl_next (avl_port))
        {
          if ((br_port = (struct nsm_bridge_port *)
                                 AVL_NODE_INFO (avl_port)) == NULL)
            continue;

#ifdef HAVE_LACPD
          ifp = ifg_lookup_by_index (&nm->zg->ifg, br_port->ifindex);

          if ((ifp == NULL)
              || (ifp->info == NULL))
            continue;

          /* If the interface is aggregated,
           * dont send bridge_if_add to protocol */
          if (NSM_INTF_TYPE_AGGREGATED (ifp))
            continue;
#endif /* HAVE_LACPD */

          if (br_port->spanning_tree_disable == PAL_FALSE) 
            msg_stp_enable.ifindex[enable_stp_count++] = br_port->ifindex;

          if (br_port->spanning_tree_disable == PAL_TRUE) 
            msg_stp_disable.ifindex[disable_stp_count++] = br_port->ifindex;

        }

      if (enable_stp_count > 0)
        {
          msg_stp_enable.spanning_tree_disable = PAL_FALSE; 

          nbytes = nsm_bridge_interface_sync_message (nse, &msg_stp_enable,
                                                      msgid, enable_stp_count);      

          if (nbytes < 0)
            {
              XFREE (MTYPE_TMP, msg_stp_enable.ifindex);
              XFREE (MTYPE_TMP, msg_stp_disable.ifindex);        
              return nbytes;
            }
        } 

      nbytes = 0;

      if (disable_stp_count > 0)
        {
          msg_stp_disable.spanning_tree_disable = PAL_TRUE;

          nbytes = nsm_bridge_interface_sync_message (nse, &msg_stp_disable, 
                                                      msgid, disable_stp_count);      

          if (nbytes < 0)
            {
              XFREE (MTYPE_TMP, msg_stp_enable.ifindex);
              XFREE (MTYPE_TMP, msg_stp_disable.ifindex);
              return nbytes;
           }
        }

      /* Free ifindex array. */
       XFREE (MTYPE_TMP, msg_stp_enable.ifindex);

      /* Free ifindex array. */
       XFREE (MTYPE_TMP, msg_stp_disable.ifindex);

      for (avl_port = avl_top (bridge->port_tree); avl_port;
           avl_port = avl_next (avl_port))
        {
          if ((br_port = (struct nsm_bridge_port *)
                                 AVL_NODE_INFO (avl_port)) == NULL)
            continue;

          ifp = ifg_lookup_by_index (&nm->zg->ifg, br_port->ifindex);

          if ((ifp == NULL)
              || (ifp->info == NULL))
            continue;

#ifdef HAVE_LACPD
          /* If the interface is aggregated,
           * dont send bridge_if_add to protocol */
          if (NSM_INTF_TYPE_AGGREGATED (ifp))
            continue;
#endif /* HAVE_LACPD */

          nsm_bridge_port_state_set_notification (bridge, ifp,
                                                  br_port->state, 0);
        }
    }

  return 0;
}

/* NSM sync bridge information to protocols. */
int
nsm_bridge_sync (struct nsm_master *nm, struct nsm_server_entry *nse)
{
  struct nsm_bridge_master *master;
  struct nsm_bridge *bridge = NULL;

  if (! nm || ! nse || ! nm->bridge)
    return -1;

  master = nsm_bridge_get_master (nm);
  if (! master)
    return -1;

  bridge = master->bridge_list;
  while (bridge)
    {
      /* 1. Send bridge. */
      nsm_bridge_sync_bridge (nm, nse, bridge, NSM_MSG_BRIDGE_ADD);

      /* 2. Send bridge interfaces. */
      nsm_bridge_sync_bridge_if (nm, nse, bridge, NSM_MSG_BRIDGE_ADD_PORT);

      /* Next bridge. */
      bridge = bridge->next;
    }
#ifdef HAVE_B_BEB
  bridge = master->b_bridge;
  if (bridge)
    {
      /* 1. Send bridge. */
      nsm_bridge_sync_bridge (nm, nse, bridge, NSM_MSG_BRIDGE_ADD);

      /* 2. Send bridge interfaces. */
      nsm_bridge_sync_bridge_if (nm, nse, bridge, NSM_MSG_BRIDGE_ADD_PORT);

      /* Next bridge. */
//      bridge = bridge->next;
      
    }
#endif /* HAVE_B_BEB */
  return 0;
}

int
nsm_server_recv_stp_interface (struct nsm_msg_header *header,
                               void *arg, void *message)
{
  struct nsm_master *nm;
  struct nsm_server_entry *nse;
  struct nsm_server *ns;
  struct nsm_msg_stp *msg;
  struct apn_vr *vr;

  /* Set connection and message.  */
  nse = arg;
  ns = nse->ns;
  msg = message;

  /* Debug.  */
  if (IS_NSM_DEBUG_RECV)
    nsm_stp_interface_dump (ns->zg, msg);

  nm = nsm_master_lookup_by_id (nzg, header->vr_id);
  if (nm == NULL)
    return 0;

  vr = nm->vr;

  if (msg->flags & NSM_MSG_INTERFACE_DOWN)
    nsm_if_flag_up_unset (header->vr_id, if_index2name (&vr->ifm,
                                                        msg->ifindex), PAL_TRUE);

  if (msg->flags & NSM_MSG_INTERFACE_UP)
    nsm_if_flag_up_set (header->vr_id, if_index2name (&vr->ifm,
                                                      msg->ifindex), PAL_TRUE);
  return 0;
}

int nsm_stp_set_server_callback (struct nsm_server *ns)
{
  nsm_server_set_callback (ns, NSM_MSG_STP_INTERFACE,
                           nsm_parse_stp_msg,
                           nsm_server_recv_stp_interface);
  return 0;
}

static void
nsm_show_dynamic_fdb_entries ( struct cli *cli, struct nsm_bridge *bridge,
                               hal_get_fdb_func_t fdb_get_func, u_int8_t edge)
{
  struct hal_fdb_entry hfdbInfo[HAL_MAX_L2_FDB_ENTRIES];
  char *bridge_name = NULL;
  char mac_addr [ETHER_ADDR_LEN];
  int ret = 0;
  int i = 0;
  u_int16_t vid = 0;
  bool_t first_call = PAL_TRUE;
  
  bridge_name = bridge->name;
 
  do
    {
      pal_mem_set(&hfdbInfo, 0,
                  (sizeof(struct hal_fdb_entry)*HAL_MAX_L2_FDB_ENTRIES));
      if (first_call)
        {
          pal_mem_set (mac_addr, 0, ETHER_ADDR_LEN);
          ret = fdb_get_func (bridge_name, mac_addr, vid,
                              HAL_MAX_L2_FDB_ENTRIES, hfdbInfo);
          first_call = PAL_FALSE;
        }
      else
        {
          ret = fdb_get_func (bridge_name, mac_addr, vid,
                              HAL_MAX_L2_FDB_ENTRIES, hfdbInfo);
        }

      if ( ret <= 0 )
        return;

      for ( i = 0 ; i < ret ; i++ )
        {
          if (edge == PAL_TRUE)
            cli_out (cli, 
                     "%-12s %-5d %-5d  %10s  %7.04hx.%.04hx.%.04hx %1d    %d\n",
                     bridge_name,
                     hfdbInfo[i].vid,
                     hfdbInfo[i].svid,
                     if_index2name (&cli->vr->ifm, hfdbInfo[i].port),
                     pal_ntoh16(((unsigned short *)hfdbInfo[i].mac_addr)[0]),
                     pal_ntoh16(((unsigned short *)hfdbInfo[i].mac_addr)[1]),
                     pal_ntoh16(((unsigned short *)hfdbInfo[i].mac_addr)[2]),
                     hfdbInfo[i].is_forward,
                     hfdbInfo[i].ageing_timer_value);
#ifdef HAVE_B_BEB
          else if (bridge->backbone_edge == PAL_TRUE)
            cli_out (cli, "%-12s %13d  %8s  %7.04hx.%.04hx.%.04hx %1d    %d\n",
                     bridge_name,
                     hfdbInfo[i].vid,
                     if_index2name (&cli->vr->ifm, hfdbInfo[i].port),
                     pal_ntoh16(((unsigned short *)hfdbInfo[i].mac_addr)[0]),
                     pal_ntoh16(((unsigned short *)hfdbInfo[i].mac_addr)[1]),
                     pal_ntoh16(((unsigned short *)hfdbInfo[i].mac_addr)[2]),
                     hfdbInfo[i].is_forward,
                     hfdbInfo[i].ageing_timer_value);
#endif /* HAVE_B_BEB */
          else            
            cli_out (cli, "%-12s %-4d %18s %8.04hx.%.04hx.%.04hx %1d    %d\n",
                     bridge_name,
                     hfdbInfo[i].vid,
                     if_index2name (&cli->vr->ifm, hfdbInfo[i].port),
                     pal_ntoh16(((unsigned short *)hfdbInfo[i].mac_addr)[0]),
                     pal_ntoh16(((unsigned short *)hfdbInfo[i].mac_addr)[1]),
                     pal_ntoh16(((unsigned short *)hfdbInfo[i].mac_addr)[2]),
                     hfdbInfo[i].is_forward,
                     hfdbInfo[i].ageing_timer_value);
        }

      pal_mem_cpy (mac_addr, hfdbInfo[ret - 1].mac_addr, ETHER_ADDR_LEN);
      vid = hfdbInfo[ret - 1].vid;
    } while ( (ret == HAL_MAX_L2_FDB_ENTRIES ) );

}

#ifdef HAVE_VLAN

char *
nsm_bridge_ovr_mac_type_to_str (enum nsm_bridge_pri_ovr_mac_type ovr_mac_type)
{

  switch (ovr_mac_type)
    {
      case NSM_BRIDGE_MAC_STATIC:
        return "static";
        break;
      case NSM_BRIDGE_MAC_STATIC_PRI_OVR:
       return "static-priority-override";
       break;
      case NSM_BRIDGE_MAC_STATIC_MGMT:
       return "static-mgmt";
       break;
      case NSM_BRIDGE_MAC_STATIC_MGMT_PRI_OVR:
       return "static-mgmt-priority-overide";
       break;
      default:
       return "unknown";
       break;

    }

  return "unknown";

}

void
nsm_pri_ovr_show_running_display (struct cli *cli, char *bridge_name,
                                  u_int16_t vid, u_int16_t svid,
                                  struct nsm_bridge_static_fdb *static_fdb,
                                  struct nsm_bridge_fdb_ifinfo *pInfo)

{
  struct nsm_master *nm = NULL;
  struct interface *ifp = NULL;
  struct nsm_bridge *bridge = NULL;
  struct nsm_bridge_master *master = NULL;

  if (!cli || !bridge_name || !static_fdb ||
      !static_fdb || !pInfo)
    return;

  if (pInfo->type != CLI_MAC)
    return;

  if (pInfo->ovr_mac_type == NSM_BRIDGE_MAC_PRI_OVR_NONE)
    return;

  nm = cli->vr->proto;

  ifp = if_lookup_by_index(&cli->vr->ifm, pInfo->ifindex);

  if (!ifp)
    return;

  if (nm)
    master = nsm_bridge_get_master (nm);

  if (master == NULL)
    return;
  
  bridge = nsm_lookup_bridge_by_name (master, bridge_name);

  if (!bridge)
    return;

  cli_out (cli, "bridge %s mac-priority-override mac-address "
                "%.04hx.%.04hx.%.04hx interface %s vlan %d "
                "%s priority %d\n",
           bridge_name,
           pal_ntoh16(((unsigned short *)static_fdb->mac_addr)[0]),
           pal_ntoh16(((unsigned short *)static_fdb->mac_addr)[1]),
           pal_ntoh16(((unsigned short *)static_fdb->mac_addr)[2]),
           ifp->name,
           vid,
           nsm_bridge_ovr_mac_type_to_str (pInfo->ovr_mac_type),
           pInfo->priority);

  return;
}

#endif /* HAVE_VLAN */

void
nsm_show_running_display (struct cli *cli, char *bridge_name,
                          u_int16_t vid, u_int16_t svid,
                          struct nsm_bridge_static_fdb *static_fdb,
                          struct nsm_bridge_fdb_ifinfo *pInfo)

{
  struct nsm_master *nm = NULL;
  struct interface *ifp = NULL;
  struct nsm_bridge *bridge;
  struct nsm_bridge_master *master = NULL;

  if (!cli || !bridge_name || !static_fdb ||
      !static_fdb || !pInfo)
    return;

  if (pInfo->type != CLI_MAC)
    return;

  if (pInfo->ovr_mac_type != NSM_BRIDGE_MAC_PRI_OVR_NONE)
    return;

  nm = cli->vr->proto;

  ifp = if_lookup_by_index(&cli->vr->ifm, pInfo->ifindex);

  if (!ifp)
    return;

  if (nm)
    master = nsm_bridge_get_master (nm);

  if (master == NULL)
    return;

  bridge = nsm_lookup_bridge_by_name (master, bridge_name);

  if (!bridge)
    return;

#ifdef HAVE_VLAN
#ifdef HAVE_PROVIDER_BRIDGE
  if (bridge->provider_edge == PAL_TRUE)
    {
      if (bridge->is_default)
        cli_out (cli, "mac-address-table static %.04hx.%.04hx.%.04hx %s %s "
                 "cvlan %d svlan %d\n",
                 pal_ntoh16(((unsigned short *)static_fdb->mac_addr)[0]),
                 pal_ntoh16(((unsigned short *)static_fdb->mac_addr)[1]),
                 pal_ntoh16(((unsigned short *)static_fdb->mac_addr)[2]),
                 pInfo->is_forward ? "forward" : "discard",
                 ifp->name,
                 vid, svid);
      else
         cli_out (cli, "bridge %s address %.04hx.%.04hx.%.04hx %s %s "
                  "cvlan %d svlan %d\n",
                  bridge_name,
                  pal_ntoh16(((unsigned short *)static_fdb->mac_addr)[0]),
                  pal_ntoh16(((unsigned short *)static_fdb->mac_addr)[1]),
                  pal_ntoh16(((unsigned short *)static_fdb->mac_addr)[2]),
                  pInfo->is_forward ? "forward" : "discard",
                  ifp->name,
                  vid, svid);
    }
  else if ( vid >  NSM_VLAN_DEFAULT_VID )
#else
  if ( vid >  NSM_VLAN_DEFAULT_VID )
#endif /* HAVE_PROVIDER_BRIDGE */
    {
      if (bridge->is_default)
        cli_out (cli, "mac-address-table static %.04hx.%.04hx.%.04hx %s %s vlan %d\n",
                 pal_ntoh16(((unsigned short *)static_fdb->mac_addr)[0]),
                 pal_ntoh16(((unsigned short *)static_fdb->mac_addr)[1]),
                 pal_ntoh16(((unsigned short *)static_fdb->mac_addr)[2]),
                 pInfo->is_forward ? "forward" : "discard",
                 ifp->name,
                 vid);
      else
         cli_out (cli, "bridge %s address %.04hx.%.04hx.%.04hx %s %s vlan %d\n",
                  bridge_name,
                  pal_ntoh16(((unsigned short *)static_fdb->mac_addr)[0]),
                  pal_ntoh16(((unsigned short *)static_fdb->mac_addr)[1]),
                  pal_ntoh16(((unsigned short *)static_fdb->mac_addr)[2]),
                  pInfo->is_forward ? "forward" : "discard",
                  ifp->name,
                  vid);
    }
  else
    {
      if (bridge->is_default)
        cli_out (cli, "mac-address-table static %.04hx.%.04hx.%.04hx %s %s\n",
                 pal_ntoh16(((unsigned short *)static_fdb->mac_addr)[0]),
                 pal_ntoh16(((unsigned short *)static_fdb->mac_addr)[1]),
                 pal_ntoh16(((unsigned short *)static_fdb->mac_addr)[2]),
                 pInfo->is_forward ? "forward" : "discard",
                 ifp->name);
      else
        cli_out (cli, "bridge %s address %.04hx.%.04hx.%.04hx %s %s\n",
                 bridge_name,
                 pal_ntoh16(((unsigned short *)static_fdb->mac_addr)[0]),
                 pal_ntoh16(((unsigned short *)static_fdb->mac_addr)[1]),
                 pal_ntoh16(((unsigned short *)static_fdb->mac_addr)[2]),
                 pInfo->is_forward ? "forward" : "discard",
                 ifp->name);
    }
#else

  if (bridge->is_default)
    cli_out (cli, "mac-address-table static %.04hx.%.04hx.%.04hx %s %s\n",
             pal_ntoh16(((unsigned short *)static_fdb->mac_addr)[0]),
             pal_ntoh16(((unsigned short *)static_fdb->mac_addr)[1]),
             pal_ntoh16(((unsigned short *)static_fdb->mac_addr)[2]),
             pInfo->is_forward ? "forward" : "discard",
             ifp->name);
  else
    cli_out (cli, "bridge %s address %.04hx.%.04hx.%.04hx %s %s\n",
             bridge_name,
             pal_ntoh16(((unsigned short *)static_fdb->mac_addr)[0]),
             pal_ntoh16(((unsigned short *)static_fdb->mac_addr)[1]),
             pal_ntoh16(((unsigned short *)static_fdb->mac_addr)[2]),
             pInfo->is_forward ? "forward" : "discard",
             ifp->name);

#endif


}

static void
nsm_show_bridge_display (struct cli *cli, char *bridge_name,
                         u_int16_t vid, u_int16_t svid,
                         struct nsm_bridge_static_fdb *static_fdb,
                         struct nsm_bridge_fdb_ifinfo *pInfo)

{

  struct interface *ifp = NULL;

  if (!cli || !bridge_name || !static_fdb ||
      !static_fdb || !pInfo)
    return;

  ifp = if_lookup_by_index(&cli->vr->ifm, pInfo->ifindex);

  if (!ifp)
    {
      cli_out(cli, "No interface found\n"); 
      return;
    }
    cli_out (cli, "%-12s %-4d %18s  %8.04hx.%.04hx.%.04hx %1d    %d\n",
             bridge_name,
             vid,
             ifp->name,
             pal_ntoh16(((unsigned short *)static_fdb->mac_addr)[0]),
             pal_ntoh16(((unsigned short *)static_fdb->mac_addr)[1]),
             pal_ntoh16(((unsigned short *)static_fdb->mac_addr)[2]),
             pInfo->is_forward,
             0);
}

#ifdef HAVE_PROVIDER_BRIDGE

static void
nsm_show_pro_edge_bridge_display (struct cli *cli, char *bridge_name,
                                  u_int16_t cvid, u_int16_t svid,
                                  struct nsm_bridge_static_fdb *static_fdb,
                                  struct nsm_bridge_fdb_ifinfo *pInfo)

{

  struct interface *ifp = NULL;

  if (!cli || !bridge_name || !static_fdb ||
      !static_fdb || !pInfo)
    return;

  ifp = if_lookup_by_index(&cli->vr->ifm, pInfo->ifindex);

  if (!ifp)
    {
      cli_out(cli, "No interface found\n");
      return;
    }
  cli_out (cli, "%-12s %-4d %-4d %10s  %.04hx.%.04hx.%.04hx %1d    %d\n",
           bridge_name,
           cvid,
           svid,
           ifp->name,
           pal_ntoh16(((unsigned short *)static_fdb->mac_addr)[0]),
           pal_ntoh16(((unsigned short *)static_fdb->mac_addr)[1]),
           pal_ntoh16(((unsigned short *)static_fdb->mac_addr)[2]),
           pInfo->is_forward,
           0);

}
#endif /* HAVE_PROVIDER_BRIDGE */

#ifdef HAVE_B_BEB
static void
nsm_show_beb_bridge_display (struct cli *cli, char *bridge_name,
                             u_int16_t vid, u_int16_t svid,
                             struct nsm_bridge_static_fdb *static_fdb,
                             struct nsm_bridge_fdb_ifinfo *pInfo)

{
  struct interface *ifp = NULL;

  if (!cli || !bridge_name || !static_fdb || !pInfo)
    return;

  ifp = if_lookup_by_index(&cli->vr->ifm, pInfo->ifindex);

  if (!ifp)
    {
      cli_out(cli, "No interface found\n");
      return;
    }

    cli_out (cli, "%-12s %13d %8s  %7.04hx.%.04hx.%.04hx %1d    %d\n",
             bridge_name,
             vid,
             ifp->name,
             pal_ntoh16(((unsigned short *)static_fdb->mac_addr)[0]),
             pal_ntoh16(((unsigned short *)static_fdb->mac_addr)[1]),
             pal_ntoh16(((unsigned short *)static_fdb->mac_addr)[2]),
             pInfo->is_forward,
             0);
}
#endif /* HAVE_B_BEB */

static void
nsm_show_fdb_list ( struct cli *cli, char *bridge_name, struct ptree *list,
                    u_int16_t vid, u_int16_t svid,
                    nsm_display_static_mac_t display_static_mac )
{

  struct ptree_node *pn = NULL;
  struct nsm_bridge_static_fdb *static_fdb = NULL;
  struct nsm_bridge_fdb_ifinfo *pInfo = NULL;

  if (!cli || !bridge_name || !list || !display_static_mac )
    return;

  for (pn = ptree_top (list); pn; pn = ptree_next (pn))
    {
      if (! (static_fdb = pn->info) )
        continue;
      pInfo = static_fdb->ifinfo_list;
      while (pInfo)
        {
          display_static_mac ( cli, bridge_name, vid, svid, static_fdb, pInfo);

          pInfo = pInfo->next_if;
        }
    }
}

void
nsm_show_cp_fdb_entries ( struct cli *cli, struct nsm_bridge *bridge,
                          nsm_display_static_mac_t display_static_mac)
{
#ifdef HAVE_VLAN
#ifdef HAVE_PROVIDER_BRIDGE
  struct nsm_pro_edge_sw_ctx *ctx = NULL;
#endif /* HAVE_PROVIDER_BRIDGE */
  struct avl_tree *vlan_table;
  struct avl_node *rn;
  struct nsm_vlan *vlan;
#endif /* HAVE_VLAN */
  int is_vlan_aware = PAL_FALSE;

  is_vlan_aware = NSM_BRIDGE_VLAN_AWARE (bridge);

#ifdef HAVE_VLAN
  if (NSM_BRIDGE_TYPE_PROVIDER (bridge))
     vlan_table = bridge->svlan_table;
#ifdef HAVE_B_BEB  
  else if (NSM_BRIDGE_TYPE_BACKBONE (bridge))
     vlan_table = bridge->bvlan_table;
#endif  
  else
     vlan_table = bridge->vlan_table;
#endif

  if ( !is_vlan_aware )
    {
      nsm_show_fdb_list (cli, bridge->name, bridge->static_fdb_list,
                         NSM_VLAN_NULL_VID, NSM_VLAN_NULL_VID,
                         display_static_mac);
    }
#ifdef HAVE_VLAN
#ifdef HAVE_PROVIDER_BRIDGE
  else if (bridge->provider_edge == PAL_TRUE)
    {
      for (rn = avl_top (bridge->pro_edge_swctx_table); rn; rn = avl_next (rn))
        {
          if ((ctx = rn->info))
            nsm_show_fdb_list (cli, bridge->name, ctx->static_fdb_list,
                               ctx->cvid, ctx->svid, display_static_mac);
        }
    }
#endif /* HAVE_PROVIDER_BRIDGE */
  else
    {
      if (vlan_table != NULL)
        for (rn = avl_top (vlan_table); rn; rn = avl_next (rn))
          {
            if ((vlan = rn->info))
              nsm_show_fdb_list (cli, bridge->name, vlan->static_fdb_list,
                                 vlan->vid, vlan->vid, display_static_mac);
          }
    }
#endif /* HAVE_VLAN */
}

static inline int
nsm_bridge_map_bridge_type_to_type ( int type)
{

  if ( (type == NSM_BRIDGE_TYPE_STP) ||
       (type == NSM_BRIDGE_TYPE_STP_VLANAWARE) )
    return NSM_BRIDGE_TYPE_STP;
  else if ( (type == NSM_BRIDGE_TYPE_RSTP) ||
            (type == NSM_BRIDGE_TYPE_RSTP_VLANAWARE) )
    return NSM_BRIDGE_TYPE_RSTP;
  else if ( type == NSM_BRIDGE_TYPE_RPVST_PLUS)
        return NSM_BRIDGE_TYPE_RPVST_PLUS;
  else
    return type;

}


void
nsm_bridge_show(struct cli *cli,
                int type)
{

  struct nsm_master *nm = cli->vr->proto;
  struct nsm_bridge_master *master = nsm_bridge_get_master (nm);
  struct nsm_bridge *bridge;

  if ( !master )
    {
      cli_out(cli, "No bridge master found\n");
      return;
    }

  bridge = master->bridge_list;
  if ( !bridge )
    {
      cli_out(cli, "No Bridge configured\n");
      return;
    }

  cli_out (cli,
           "bridge       CVLAN SVLAN BVLAN  port     mac            "
           "fwd timeout\n");

  while (bridge)
    {

      if ( (type != 0) &&
           (type != nsm_bridge_map_bridge_type_to_type (bridge->type) ))
        {
          bridge=bridge->next;
          continue;
        }

#ifdef HAVE_PROVIDER_BRIDGE
      if (bridge->provider_edge == PAL_TRUE)
        {
           nsm_show_cp_fdb_entries (cli, bridge,
                                    nsm_show_pro_edge_bridge_display);

#ifdef HAVE_HAL
           nsm_show_dynamic_fdb_entries (cli, bridge,
                                         hal_l2_fdb_unicast_get, PAL_TRUE);
           nsm_show_dynamic_fdb_entries (cli, bridge,
                                         hal_l2_fdb_multicast_get, PAL_TRUE);
#endif /* HAVE_HAL */
        }
      else
#endif /* HAVE_PROVIDER_BRIDGE */
        {
          nsm_show_cp_fdb_entries ( cli, bridge, nsm_show_bridge_display);

#ifdef HAVE_HAL
          nsm_show_dynamic_fdb_entries (cli, bridge,
                                        hal_l2_fdb_unicast_get, PAL_FALSE);
          nsm_show_dynamic_fdb_entries (cli, bridge,
                                        hal_l2_fdb_multicast_get, PAL_FALSE);
#endif /* HAVE_HAL */
        }

      bridge = bridge->next;
    }
#ifdef HAVE_B_BEB
    bridge = master->b_bridge;
    if (bridge)
    {
        nsm_show_cp_fdb_entries ( cli, bridge, nsm_show_beb_bridge_display);
#ifdef HAVE_HAL
        nsm_show_dynamic_fdb_entries (cli, bridge,
                                      hal_l2_fdb_unicast_get, PAL_FALSE);
        nsm_show_dynamic_fdb_entries (cli, bridge,
                                      hal_l2_fdb_multicast_get, PAL_FALSE);
#endif /* HAVE_HAL */
    }
#endif /*HAVE_B_BEB*/
  return;
}

struct nsm_bridge_static_fdb_config *
nsm_bridge_static_fdb_config_add (char *bridge_name, struct interface *ifp,
                                  u_int8_t *mac_addr,
                                  u_int16_t vid, bool_t is_forward,
                                  enum nsm_bridge_pri_ovr_mac_type ovr_mac_type,
                                  u_int8_t priority)
{
  struct nsm_if *zif = NULL;
  struct nsm_bridge_static_fdb_config *static_fdb_config = NULL;

  if ( (!ifp) || (!(zif = ifp->info)) || (!zif->bridge_static_mac_config))
    return NULL;

  static_fdb_config = XCALLOC (MTYPE_NSM_BRIDGE_STAT_FDB_CFG,
                               sizeof (struct nsm_bridge_static_fdb_config) );

  if (!static_fdb_config)
    return NULL;

  pal_strncpy (static_fdb_config->bridge_name, bridge_name,
               NSM_BRIDGE_NAMSIZ + 1);

  pal_mem_cpy( static_fdb_config->mac_addr, mac_addr, ETHER_ADDR_LEN);

  static_fdb_config->vid = vid;
  static_fdb_config->ovr_mac_type = ovr_mac_type;
  static_fdb_config->priority = priority;

  static_fdb_config->is_forward = is_forward;

  if (!listnode_add (zif->bridge_static_mac_config, static_fdb_config))
    return NULL;

  return static_fdb_config;

}

void
nsm_bridge_static_fdb_config_activate (struct nsm_bridge_master *master,
                                       char *bridge_name,
                                       u_int16_t vid,
                                       struct interface *ifp)
{
  u_int8_t l_mac[ETHER_ADDR_LEN];
  struct nsm_if *zif = NULL;
  struct listnode *curr = NULL;
  struct listnode *next = NULL;
  struct nsm_bridge_static_fdb_config *static_fdb_config = NULL;
  int ret = 0;

  pal_mem_set (l_mac, 0 ,ETHER_ADDR_LEN);

  if (!ifp || !master || !bridge_name)
    return;

  if (! (zif = ifp->info) || !(zif->bridge_static_mac_config))
    return;

  /* We cannot free the entries corresponding to DEFAULT_VLAN since
   * initially when port is added to the bridge the mode is set to
   * ACCESS. When subsequently when the switchport mode command is
   * executed, the static fdb is flushed, hence we need the configuration
   * for default vlan at this point so that we are able to restore the
   * configuration.
   */


  for (curr = zif->bridge_static_mac_config->head; curr;
       curr = next)
    {
      next = curr->next;
      if (!(static_fdb_config = curr->data))
        continue;
      if ((!pal_strncmp (static_fdb_config->bridge_name, bridge_name,
                         NSM_BRIDGE_NAMSIZ + 1)) &&
          (vid == static_fdb_config->vid))
        {
          /* Convert to network order */
          *(unsigned short *)&l_mac[0] =
            pal_hton16(*(unsigned short *)&static_fdb_config->mac_addr[0]);
          *(unsigned short *)&l_mac[2] =
            pal_hton16(*(unsigned short *)&static_fdb_config->mac_addr[2]);
          *(unsigned short *)&l_mac[4] =
            pal_hton16(*(unsigned short *)&static_fdb_config->mac_addr[4]);

          if (static_fdb_config->ovr_mac_type == NSM_BRIDGE_MAC_PRI_OVR_NONE)
             ret = nsm_bridge_add_mac (master, bridge_name, ifp,
                                 l_mac,
                                 static_fdb_config->vid,
                                 static_fdb_config->svid,
                                 HAL_L2_FDB_STATIC,
                                 static_fdb_config->is_forward, CLI_MAC);
         else
           nsm_bridge_add_mac_prio_ovr (master, bridge_name, ifp,
                                        l_mac, static_fdb_config->vid,
                                        static_fdb_config->ovr_mac_type,
                                        static_fdb_config->priority);

#ifdef HAVE_VLAN
          if (vid == NSM_VLAN_DEFAULT_VID)
            continue;
#endif
          listnode_delete (zif->bridge_static_mac_config, static_fdb_config);
          XFREE (MTYPE_NSM_BRIDGE_STAT_FDB_CFG, static_fdb_config);
        }
    }

#ifdef HAVE_VLAN
  /* Free the nodes corresponding to DEFAULT_VID when the configuration corresponding
   * to vlan other than DEFAULT_VID is activated. This means that the mode of the port
   * is set and hence the entries corresponding to default vlan can be freed.
   */

  if (vid == NSM_VLAN_DEFAULT_VID)
    return;
  for (curr = zif->bridge_static_mac_config->head; curr;
       curr = next)
    {
      next = curr->next;
      if (!(static_fdb_config = curr->data))
        continue;
      if ((!pal_strncmp (static_fdb_config->bridge_name, bridge_name,
                         NSM_BRIDGE_NAMSIZ + 1)) &&
          (static_fdb_config->vid == NSM_VLAN_DEFAULT_VID))
        {
          listnode_delete (zif->bridge_static_mac_config, static_fdb_config);
          XFREE (MTYPE_NSM_BRIDGE_STAT_FDB_CFG, static_fdb_config);
        }
    }
#endif

}

void
nsm_bridge_static_fdb_config_free (struct interface *ifp)
{

  struct listnode *ln = NULL;
  struct nsm_if   *zif  = NULL;
  struct nsm_bridge_static_fdb_config *static_fdb_config = NULL;
  zif = ifp->info;
  if ( !zif || !zif->bridge_static_mac_config)
    return;
  LIST_LOOP (zif->bridge_static_mac_config, static_fdb_config, ln)
    {
      XFREE (MTYPE_NSM_BRIDGE_STAT_FDB_CFG, static_fdb_config);
    }
  list_delete (zif->bridge_static_mac_config);
  zif->bridge_static_mac_config = NULL;
}


struct
nsm_bridge_config *nsm_bridge_config_new (struct nsm_bridge_master *master,
                                          char *bridge_name)
{
  struct nsm_bridge_config *new_br_conf = NULL;
  struct nsm_bridge *bridge = NULL;
  bridge = nsm_lookup_bridge_by_name(master, bridge_name);

  if ( bridge )
    {
      new_br_conf = XCALLOC (MTYPE_BRIDGE_VLAN_CONFIG,
                             sizeof (struct nsm_bridge_config));
      if (new_br_conf)
        pal_strcpy (new_br_conf->br_name, bridge_name);

      bridge->br_conf = new_br_conf;

      return new_br_conf;
    }
  return NULL;
}

struct
nsm_bridge_config *nsm_bridge_config_find (struct nsm_bridge_master *master,
                                           char *bridge_name)
{
  struct nsm_bridge *bridge = NULL;
  bridge = nsm_lookup_bridge_by_name(master, bridge_name);

  return (bridge ? bridge->br_conf : NULL);
}

void
nsm_bridge_config_free (struct nsm_bridge_master *master,
                        char *br_name)
{
  struct nsm_bridge *bridge = NULL;
  bridge = nsm_lookup_bridge_by_name(master, br_name);

  if(bridge)
    {
      if ( bridge->br_conf )
        XFREE (MTYPE_BRIDGE_VLAN_CONFIG, bridge->br_conf);
      bridge->br_conf = NULL;
    }

  return;
}

void
nsm_bridge_config_activate (struct nsm_bridge_master *master,
                            char* br_name, int vid)
{
  struct nsm_bridge_config *br_conf = NULL;
#ifdef HAVE_VLAN
  struct nsm_vlan_config_entry * vlan_conf_entry = NULL;
#endif /* HAVE_VLAN */

  br_conf = nsm_bridge_config_find (master, br_name);
  if (!br_conf)
    return;

#ifdef HAVE_VLAN
  vlan_conf_entry = nsm_vlan_config_entry_find(master, br_name, vid);
  if (!vlan_conf_entry)
    return;

#ifdef HAVE_IGMP_SNOOP
#if 0 /* TBD */
  igmp_snoop_activate_vlan_config (master, &(vlan_conf_entry->igs_config),
                                   br_name, vid);
#endif /* TBD */
#endif /* HAVE_IGMP_SNOOP */
  listnode_delete(br_conf->vlan_config_list, vlan_conf_entry);
  nsm_vlan_config_entry_free (vlan_conf_entry);
#endif /* HAVE_VLAN */

  return;
}

/* Bridge Listener interface */
int nsm_add_listener_to_bridgelist(struct list *listener_list,
                                   struct nsm_bridge_listener *appln)
{
  if ( !(appln) || !(listener_list) )
    {
      return RESULT_ERROR;
    }

  if ( ((appln->listener_id) < NSM_LISTENER_ID_MIN) ||
       ((appln->listener_id) >= NSM_LISTENER_ID_MAX) )
    {
      return RESULT_ERROR;
    }

  if ( !(listnode_lookup(listener_list, appln)) )
    {
      listnode_add(listener_list, appln);
    }
  return RESULT_OK;
}

void nsm_remove_listener_from_bridgelist(struct list *listener_list,
                                         nsm_listener_id_t appln_id)
{
  struct listnode *ln = NULL;
  struct listnode *lnext = NULL;
  struct nsm_bridge_listener *appln = NULL;

  if ( !(listener_list) ||
       (appln_id < NSM_LISTENER_ID_MIN) ||
       (appln_id >= NSM_LISTENER_ID_MAX) )
    {
      return;
    }

  for (ln = LISTHEAD (listener_list); ln; ln = lnext)
    {
      lnext = ln->next;

      appln = (struct nsm_bridge_listener *)GETDATA (ln);

      if(appln->listener_id == appln_id)
        {
          listnode_delete(listener_list, appln);
          break;
        }
    }
}

int nsm_create_bridge_listener(struct nsm_bridge_listener **appln)
{
  if (((*appln) =
       XCALLOC (MTYPE_TMP, sizeof (struct nsm_bridge_listener))) == NULL)
    return RESULT_ERROR;

  (*appln)->listener_id = NSM_LISTENER_ID_MAX;
  (*appln)->add_bridge_func = NULL;
  (*appln)->delete_bridge_func = NULL;
  (*appln)->add_port_to_bridge_func = NULL;
  (*appln)->delete_port_from_bridge_func = NULL;
  (*appln)->enable_port_func = NULL;
  (*appln)->disable_port_func = NULL;
  (*appln)->topology_change_notify_func = NULL;
  (*appln)->activate_port_config_func  = NULL;
  (*appln)->change_port_state_func = NULL;
  (*appln)->add_static_mac_addr_func = NULL;
  (*appln)->delete_static_mac_addr_func = NULL;
  return RESULT_OK;
}

void nsm_destroy_bridge_listener(struct nsm_bridge_listener *appln)
{
  if ( appln != NULL )
    {
      appln->add_bridge_func = NULL;
      appln->delete_bridge_func = NULL;
      appln->add_port_to_bridge_func = NULL;
      appln->delete_port_from_bridge_func = NULL;
      appln->enable_port_func = NULL;
      appln->disable_port_func = NULL;
      appln->topology_change_notify_func = NULL;
      appln->activate_port_config_func = NULL;
      appln->change_port_state_func = NULL;
      appln->delete_static_mac_addr_func = NULL;
      appln->add_static_mac_addr_func = NULL;

      XFREE (MTYPE_TMP, appln);
    }
}

int
nsm_bridge_del_mac_prio_ovr (struct nsm_bridge_master *master, char *name,
                             struct interface *ifp, u_int8_t *mac,
                             u_int16_t vid)
{
  int ret = RESULT_OK;

  ret = nsm_bridge_delete_mac (master, name, ifp, mac, vid, vid,
                               HAL_L2_FDB_STATIC,
                               PAL_TRUE, CLI_MAC);
  if (ret != 0)
    return ret;

  return RESULT_OK;

}

int
nsm_bridge_add_mac_prio_ovr (struct nsm_bridge_master *master, char *name,
                             struct interface *ifp, u_int8_t *mac,
                             u_int16_t vid,
                             enum nsm_bridge_pri_ovr_mac_type ovr_mac_type,
                             u_int8_t priority)
{
  int ret = RESULT_OK;
  struct nsm_bridge_fdb_ifinfo *pInfo = NULL;
  struct ptree *static_fdb_list = NULL;
  struct nsm_vlan *vlan = NULL;
  struct nsm_bridge *bridge;
  struct avl_node *rn;
  struct nsm_vlan p;
  struct nsm_if *zif;
#ifdef HAVE_HA
  struct nsm_bridge_static_fdb *static_fdb = NULL;
  struct ptree_node *node = NULL;
#endif /*HAVE_HA*/

  if (ovr_mac_type == NSM_BRIDGE_MAC_PRI_OVR_NONE
      || ovr_mac_type == NSM_BRIDGE_MAC_PRI_OVR_MAX)
    return NSM_BRIDGE_ERR_INVALID_ARG;

  ret = nsm_bridge_add_mac (master, name, ifp, mac, vid, vid,
                            HAL_L2_FDB_STATIC,
                            PAL_TRUE, CLI_MAC);
  if (ret != RESULT_OK)
    {
      zif = ifp->info;

      if ( (zif) && (zif->bridge == NULL))
         nsm_bridge_static_fdb_config_add (name, ifp, mac, vid,
                                           PAL_TRUE,
                                           ovr_mac_type, priority);

      return ret;
    }

  ret =  hal_l2_add_priority_ovr (name, ifp->ifindex, mac,
                                  HAL_HW_LENGTH, vid, ovr_mac_type, priority);
  if ( HAL_SUCCESS != ret )
    return NSM_HAL_FDB_ERR;

  bridge = nsm_lookup_bridge_by_name (master, name);

#ifdef HAVE_VLAN
  if (nsm_bridge_is_vlan_aware (master, name) == PAL_TRUE)
    {
      NSM_VLAN_SET (&p, vid);

      rn = avl_search (bridge->vlan_table, (void *)&p);

      if (rn && rn->info)
        vlan = rn->info;

      if (vlan == NULL)
        {
          zlog_err (nzg, "vlan %d not found \n", vid);

          return RESULT_ERROR;
        }

      static_fdb_list = vlan->static_fdb_list;
    }
  else
#endif /* HAVE_VLAN */
    {
      static_fdb_list = bridge->static_fdb_list;
    }

  if (static_fdb_list)
    pInfo = nsm_bridge_fdb_ifinfo_lookup (static_fdb_list, mac, ifp->ifindex);
  else
    {
      zlog_err (nzg, "unable to find the bridge fdb list\n");
      return RESULT_ERROR;
    }
  if (pInfo)
    {
      pInfo->ovr_mac_type = ovr_mac_type;
      pInfo->priority   = priority;
    }
  else
    {
      zlog_err (nzg, "unable to find the bridge pInfo \n");
      return RESULT_ERROR;
    }
#ifdef HAVE_HA
  node = ptree_node_lookup (static_fdb_list, mac, ETHER_MAX_BIT_LEN);

  if (NULL == node)
    return RESULT_ERROR;

  if (!node->info)
    return RESULT_ERROR;

  static_fdb = node->info;
  nsm_bridge_cal_modify_nsm_bridge_mac (pInfo, bridge, static_fdb, vid);

#endif /*HAVE_HA*/

  return ret;

}

#ifdef HAVE_PROVIDER_BRIDGE

int
nsm_bridge_api_port_proto_process (struct interface *ifp,
                                   enum hal_l2_proto proto,
                                   enum hal_l2_proto_process process,
                                   bool_t iterate_members,
                                   u_int16_t svid)
{
  struct nsm_vlan_port *vlan_port;
  struct nsm_bridge_port *br_port;
  struct nsm_bridge *bridge;
#ifdef HAVE_LACPD
  struct interface *memifp;
  struct listnode *lsn;
#endif /* HAVE LACP */
  struct nsm_if *zif;
  int ret;
  struct nsm_vlan p;
  struct avl_node *rn = NULL;

  zif = (struct nsm_if *)ifp->info;

  if (! zif)
    return NSM_BRIDGE_ERR_NOTFOUND;

  if (! (br_port = zif->switchport))
    return NSM_BRIDGE_ERR_NOTFOUND;

  if (! (bridge = zif->bridge))
    return NSM_BRIDGE_ERR_NOTFOUND;

  NSM_VLAN_SET (&p, svid); 
  rn = avl_search (bridge->svlan_table, (void *)&p);

  if (rn == NULL  || rn->info == NULL)
    return NSM_VLAN_ERR_VLAN_NOT_FOUND;

  vlan_port = &br_port->vlan_port;

  if (! vlan_port)
    return NSM_BRIDGE_ERR_NOTFOUND;

  if (vlan_port->mode != NSM_VLAN_PORT_MODE_CN
      && vlan_port->mode != NSM_VLAN_PORT_MODE_CE
      && vlan_port->mode != NSM_VLAN_PORT_MODE_CNP)
    return NSM_BRIDGE_ERR_INVALID_MODE;

  switch (proto)
    {
#if defined HAVE_STPD || defined HAVE_RSTPD || defined HAVE_MSTPD
    case HAL_PROTO_STP:
    case HAL_PROTO_RSTP:
    case HAL_PROTO_MSTP:
      if (vlan_port->mode == NSM_VLAN_PORT_MODE_CN
          && process == HAL_L2_PROTO_PEER)
         return NSM_BRIDGE_ERR_INVALID_MODE;

      br_port->proto_process.stp_process = process;

      nsm_pro_vlan_send_proto_process (bridge->name, ifp, process);

      if (process == HAL_L2_PROTO_TUNNEL)
        br_port->proto_process.stp_tun_vid = svid;
      break;
#endif /* HAVE_STPD || HAVE_RSTPD || HAVE_MSTPD */

#ifdef HAVE_GMRP
    case HAL_PROTO_GMRP:
      br_port->proto_process.gmrp_process = process;
      break;
#endif /* HAVE_GMRP */

#ifdef HAVE_MMRP
    case HAL_PROTO_MMRP:
      br_port->proto_process.mmrp_process = process;
      break;
#endif /* HAVE_MMRP */

#ifdef HAVE_GVRP
    case HAL_PROTO_GVRP:
      if (vlan_port->mode == NSM_VLAN_PORT_MODE_CN
          && process == HAL_L2_PROTO_PEER)
         return NSM_BRIDGE_ERR_INVALID_MODE;

      br_port->proto_process.gvrp_process = process;
      if (process == HAL_L2_PROTO_TUNNEL)
        br_port->proto_process.gvrp_tun_vid = svid;
      break;
#endif /* HAVE_GVRP */

#ifdef HAVE_MVRP
    case HAL_PROTO_MVRP:
      if (vlan_port->mode == NSM_VLAN_PORT_MODE_CN
          && process == HAL_L2_PROTO_PEER)
         return NSM_BRIDGE_ERR_INVALID_MODE;

      br_port->proto_process.mvrp_process = process;
      if (process == HAL_L2_PROTO_TUNNEL)
        br_port->proto_process.mvrp_tun_vid = svid;
      break;
#endif /* HAVE_MVRP */

#ifdef HAVE_LACPD
    case HAL_PROTO_LACP:
      br_port->proto_process.lacp_process = process;
      if (process == HAL_L2_PROTO_TUNNEL)
        br_port->proto_process.lacp_tun_vid = svid;
      break;
#endif /* HAVE_LACPD */

#ifdef HAVE_AUTHD
    case HAL_PROTO_DOT1X:
      br_port->proto_process.dot1x_process = process;
      if (process == HAL_L2_PROTO_TUNNEL)
        br_port->proto_process.dot1x_tun_vid = svid;
      break;
#endif /* HAVE_AUTHD */

    default:
      return NSM_BRIDGE_ERR_INVALID_PROTO;
    }

  ret = hal_bridge_set_proto_process_port (bridge->name, ifp->ifindex,
                                           proto, process, svid);

#ifdef HAVE_LACPD
  {
    u_int16_t key = ifp->lacp_admin_key;
    nsm_lacp_if_admin_key_set (ifp);
    if (key != ifp->lacp_admin_key)
      nsm_server_if_lacp_admin_key_update (ifp);
  }
#endif /* HAVE_LACPD */

#ifdef HAVE_LACPD
  if (iterate_members == PAL_TRUE)
    {
      AGGREGATOR_MEMBER_LOOP (zif, memifp, lsn)
        {
          NSM_BRIDGE_CHECK_AGG_MEM_L2_PROPERTY(memifp);

          memifp->agg_param_update = 1;
          ret = nsm_bridge_api_port_proto_process (memifp, proto,
                                                   process, PAL_FALSE, svid);
          if (ret < 0)
            {
              zlog_err(NSM_ZG,
                       "nsm_bridge_api_port_proto_process Failed (%d) on"
                       "aggregator member %s\n",
                       ret, memifp->name);
            }
          memifp->agg_param_update = 0;

        }
    }
#endif /* HAVE_LACPD */

  return RESULT_OK;
}

#ifdef HAVE_SMI
int
nsm_bridge_api_get_port_proto_process (struct interface *ifp,
                                       enum smi_bridge_proto proto,
                                       enum smi_bridge_proto_process *process)
{
  struct nsm_bridge_port *br_port;
  struct nsm_bridge *bridge;
  struct nsm_if *zif;

  zif = (struct nsm_if *)ifp->info;

  if (! zif)
    return NSM_BRIDGE_ERR_NOTFOUND;

  if (! (br_port = zif->switchport))
    return NSM_BRIDGE_ERR_NOTFOUND;

  if (! (bridge = zif->bridge))
    return NSM_BRIDGE_ERR_NOTFOUND;

  switch (proto)
    {
#if defined HAVE_STPD || defined HAVE_RSTPD || defined HAVE_MSTPD
    case HAL_PROTO_STP:
    case HAL_PROTO_RSTP:
    case HAL_PROTO_MSTP:
      *process = br_port->proto_process.stp_process;
      break;
#endif /* HAVE_STPD || HAVE_RSTPD || HAVE_MSTPD */

#ifdef HAVE_GMRP
    case HAL_PROTO_GMRP:
      *process = br_port->proto_process.gmrp_process;
      break;
#endif /* HAVE_GMRP */

#ifdef HAVE_MMRP
    case HAL_PROTO_MMRP:
      *process = br_port->proto_process.mmrp_process;
      break;
#endif /* HAVE_MMRP */

#ifdef HAVE_GVRP
    case HAL_PROTO_GVRP:
      *process = br_port->proto_process.gvrp_process;
      break;
#endif /* HAVE_GVRP */

#ifdef HAVE_MVRP
    case HAL_PROTO_MVRP:
      *process = br_port->proto_process.mvrp_process;
      break;
#endif /* HAVE_MVRP */

#ifdef HAVE_LACPD
    case HAL_PROTO_LACP:
      *process = br_port->proto_process.lacp_process;
      break;
#endif /* HAVE_LACPD */

#ifdef HAVE_AUTHD
    case HAL_PROTO_DOT1X:
      *process = br_port->proto_process.dot1x_process;
      break;
#endif /* HAVE_AUTHD */

    default:
      return NSM_BRIDGE_ERR_INVALID_PROTO;
    }
  return RESULT_OK;
}
#endif /* HAVE_SMI */

#endif /* HAVE_PROVIDER_BRIDGE */

#ifdef  HAVE_L2
void
nsm_bridge_if_send_state_sync_req_wrap ( struct interface *ifp)
{
  struct nsm_if *zif = NULL;

  if ((ifp == NULL) ||
     (! CHECK_FLAG (ifp->status, NSM_INTERFACE_ACTIVE)) ||
     (ifp->type != NSM_IF_TYPE_L2 ))
    {
      return RESULT_ERROR;
    }

  zif = ifp->info;
  if ((zif) && (zif->bridge))
    nsm_bridge_if_send_state_sync_req (zif->bridge, ifp);
}
#endif /* HAVE_L2 */


#ifdef HAVE_G8031
static int eps_id_cmp ( void * data1, void * data2 )
{
  struct g8031_protection_group *pg1 = (struct g8031_protection_group *)data1;
  struct g8031_protection_group *pg2 = (struct g8031_protection_group *)data2;

  pal_assert(pg1);
  pal_assert(pg2);

  if ( pg1->eps_id > pg2->eps_id )
    return 1;

  if ( pg2->eps_id > pg1->eps_id )
    return -1;

  return (RESULT_ERROR);
}

static int eps_vid_cmp ( void * data1, void * data2 )
{
  struct nsm_vlan * vid1 = (struct nsm_vlan *)data1;
  struct nsm_vlan * vid2 = (struct nsm_vlan *)data2;

  pal_assert(vid1);
  pal_assert(vid2);

  if ( vid1->vid > vid2->vid )
    return 1;

  if ( vid2->vid > vid1->vid )
    return -1;

  return (RESULT_ERROR);
}

s_int16_t 
nsm_create_g8031_protection_group ( u_int8_t *br_name,
                                    u_int8_t * ifname_w,
                                    u_int8_t * ifname_p,
                                    u_int16_t instance,
                                    u_int16_t eps_id,
                                    struct nsm_master *nm
                                  )
{

  struct g8031_protection_group    * pg = NULL;
  struct interface                 * ifp_w = NULL, *ifp_p = NULL;
  struct nsm_bridge                * bridge = NULL;
  struct nsm_msg_g8031_pg          * msg = NULL;
  struct nsm_bridge_master         * master;
  struct nsm_if                    *nif = NULL;
#ifdef HAVE_LACPD
  struct interface                 *memifp;
  struct listnode                  *lsn;
#endif /* HAVE LACP */
      
  pal_assert(br_name);
  pal_assert(ifname_w);
  pal_assert(ifname_p);
  pal_assert(nm);

  master = nsm_bridge_get_master(nm);

  pal_assert(master);

  bridge = nsm_lookup_bridge_by_name (master,br_name);

  if (bridge == NULL)
    {
      return (NSM_BRIDGE_ERR_NOTFOUND);
    }

  /* find if protection group with eps_id already existing */
  if ((nsm_g8031_find_protection_group(bridge,eps_id)) != NULL )
    {
      return (NSM_BRIDGE_PG_ALREADY_EXISTS);
    }

  if ((ifp_w = if_lookup_by_name(&nm->vr->ifm,ifname_w)) == NULL )
    {
      return (NSM_INTERFACE_NOT_FOUND);
    }

  if ((ifp_p = if_lookup_by_name(&nm->vr->ifm,ifname_p)) == NULL )
    {
      return (NSM_INTERFACE_NOT_FOUND);
    }

#ifdef HAVE_LACPD
  if (NSM_INTF_TYPE_AGGREGATOR(ifp_w))
    {
      AGGREGATOR_MEMBER_LOOP (nif, memifp, lsn)
        {
          if (memifp->name == ifp_p->name);
          return NSM_G8031_LACP_IF_CONFIG_ERR;
        }
    }

  if (NSM_INTF_TYPE_AGGREGATOR(ifp_p))
    {
      AGGREGATOR_MEMBER_LOOP (nif, memifp, lsn)
        {
          if (memifp->name == ifp_w->name);
          return NSM_G8031_LACP_IF_CONFIG_ERR;

        }
    }
#endif /* HAVE LACP */
  
  if (!bridge->eps_tree)
    {
      if (avl_create(&bridge->eps_tree,0,eps_id_cmp) < 0)
        {
          return (RESULT_ERROR);
        }
    }

  /* Check whether the Instance is Free or Not */
  if (NSM_INSTANCE_BMP_IS_MEMBER(master->instanceBmp, instance))
   return NSM_G8031_INSTANCE_IN_USE;

  /* Memory allocation for a protection group */
  pg = XCALLOC(MTYPE_G8031_PROTECTION_GROUP \
      ,sizeof(struct g8031_protection_group));

  if (!pg)
    {
      return (RESULT_ERROR);
    }

  pg->eps_id = eps_id;
  pg->ifindex_w = ifp_w->ifindex;
  pg->ifindex_p = ifp_p->ifindex;
  pg->instance = instance;

   /* Mark the instance as Used */
  NSM_INSTANCE_BMP_SET(master->instanceBmp, instance);
 
  if (!pg->eps_vids)
    {
      if (avl_create(&pg->eps_vids,0,eps_vid_cmp) < 0)
        {
          XFREE(MTYPE_G8031_PROTECTION_GROUP,pg);
          return (RESULT_ERROR);
        }
    }

  if (avl_insert(bridge->eps_tree, pg) < 0)
    {
      return (RESULT_ERROR);
    }

  /* Disable STP on the Working/Protected Ports. */
  protection_group_update_stp (instance, ifp_w, ifp_p, PAL_TRUE, NSM_MSG_BRIDGE_ADD_PG);

  pg->pg_state = G8031_PG_NOT_INITALIZED; 
  msg = XCALLOC (MTYPE_G8031_PROTECTION_GROUP, \
      sizeof (struct nsm_msg_g8031_pg));

  if (msg)
    {
      msg->eps_id = eps_id;
      msg->working_ifindex   = ifp_w->ifindex; 
      msg->protected_ifindex = ifp_p->ifindex;
      pal_mem_cpy(&msg->bridge_name,bridge->name,strlen(bridge->name));

      /* send the same details to ONMD */
      nsm_g8031_send_protection_group(bridge, msg, 
          NSM_MSG_G8031_CREATE_PROTECTION_GROUP);
    }
  else
    {
      return (RESULT_ERROR);
    }

  XFREE(MTYPE_G8031_PROTECTION_GROUP,msg);

  return RESULT_OK;
}

void pg_free_vlan_tree(void * ptr)
{
	
   XFREE(MTYPE_NSM_G8031_VLAN,ptr);
}

s_int16_t
nsm_delete_g8031_protection_group ( u_int8_t * br_name,
                                    u_int16_t eps_id,
                                    struct nsm_master *nm
                                  )
{

  struct g8031_protection_group    * pg = NULL;
  struct g8031_protection_group    * pg1 = NULL;
  struct avl_node                  * node = NULL;
  struct nsm_bridge                * bridge = NULL;
  struct nsm_msg_g8031_pg          * msg = NULL;
  struct nsm_bridge_master         * master;
  struct nsm_vlan                  * vlan = NULL;
  struct avl_node                  *eps_vid_node = NULL;
  struct nsm_msg_vlan              vlan_decouple_msg;
  struct interface                 *ifp_w;
  struct interface                 *ifp_p;

  pal_assert(br_name);

  master = nsm_bridge_get_master(nm);
  pal_assert(master);

  bridge = nsm_lookup_bridge_by_name (master,br_name);

  if (bridge == NULL)
    {
      return (RESULT_ERROR);
    }

  /* find if protection group with eps_id already existing */
  pg = nsm_g8031_find_protection_group(bridge,eps_id);

  if (!pg)
    {
      return (RESULT_ERROR);
    }

  /* remove decoupling for this vlan */
  /* Send VLAN Add message to MSTP. */
  AVL_TREE_LOOP (pg->eps_vids, vlan, eps_vid_node )
    {
      pal_mem_set (&vlan_decouple_msg, 0, sizeof (struct nsm_msg_vlan));
      vlan_decouple_msg.vid = vlan->vid;
      pal_strncpy (vlan_decouple_msg.vlan_name, vlan->vlan_name, NSM_VLAN_NAMSIZ);
      NSM_SET_CTYPE(vlan_decouple_msg.cindex, NSM_VLAN_CTYPE_NAME);
      pal_strncpy (vlan_decouple_msg.bridge_name, bridge->name, NSM_BRIDGE_NAMSIZ);
      NSM_SET_CTYPE(vlan_decouple_msg.cindex, NSM_VLAN_CTYPE_BRIDGENAME);

      nsm_send_vlan_update_msg (bridge, &vlan_decouple_msg, NSM_MSG_VLAN_DEL_FROM_PROTECTION);
    }

  ifp_w = ifg_lookup_by_index (&nm->zg->ifg, pg->ifindex_w);
  ifp_p = ifg_lookup_by_index (&nm->zg->ifg, pg->ifindex_p);
  if ((!ifp_w) || (!ifp_p))
    return RESULT_ERROR;  

  protection_group_update_stp (pg->instance, ifp_w, ifp_p, PAL_FALSE, NSM_MSG_BRIDGE_DEL_PG);
  msg = XCALLOC (MTYPE_G8031_PROTECTION_GROUP, \
      sizeof (struct nsm_msg_g8031_pg));

  if (msg)
    {
      /* delete & Free protection group at NSM */
      for (node = avl_top(bridge->eps_tree);node;node=avl_next(node))
        {
          if ((pg1 = (struct g8031_protection_group *) \
                AVL_NODE_INFO(node))== NULL)
            continue;

          if (pg1->eps_id == eps_id)
            break;
        }
      
     NSM_INSTANCE_BMP_UNSET(master->instanceBmp, pg->instance);
 
      if (node)
        avl_delete_node(bridge->eps_tree,node);

      XFREE(MTYPE_G8031_PROTECTION_GROUP,(void *)pg);

      /* Post delete pg msg to ONMD */
      msg->eps_id = eps_id;
      pal_mem_cpy(&msg->bridge_name,bridge->name,strlen(bridge->name));

      nsm_g8031_send_protection_group(bridge, msg,
                  NSM_MSG_G8031_DEL_PROTECTION_GROUP);
    }
  else
    {
      return (RESULT_ERROR);
    }

  XFREE(MTYPE_G8031_PROTECTION_GROUP,msg);

  return RESULT_OK;
}

struct g8031_protection_group *
nsm_g8031_find_protection_group ( struct nsm_bridge * bridge,
                                  u_int16_t eps_id
                                )
{
  struct avl_node                  * node = NULL;
  struct g8031_protection_group    * pg = NULL;

  for (node = avl_top(bridge->eps_tree);node;node=avl_next(node))
    {
      if ((pg = (struct g8031_protection_group *) \
            AVL_NODE_INFO(node))== NULL)
        continue;

      if (pg->eps_id == eps_id)
        {
          return (pg);
        }
    }
  return (NULL);
}

s_int16_t
nsm_map_vlans_to_g8031_protection_group ( u_int16_t eps_vid,
                                          u_int8_t * br_name,
                                          u_int16_t eps_id,
                                          u_int8_t  primary,
                                          struct nsm_master *nm
                                        )
{
  struct nsm_vlan                  * vlan = NULL;
  struct g8031_protection_group    * pg = NULL;
  struct nsm_bridge                * bridge = NULL;
  struct nsm_bridge_master         * master;
  struct nsm_msg_g8031_vlanpg      * msg = NULL;
  struct interface                 * ifp_w = NULL, *ifp_p = NULL;
  struct nsm_if                    * zif = NULL;
  struct nsm_bridge_port           * br_port = NULL;
  struct nsm_vlan_port             * port = NULL;
  struct nsm_msg_vlan              vlan_decouple_msg;

  pal_assert(br_name);
  pal_assert(nm);

  master = nsm_bridge_get_master(nm);
  pal_assert(master);

  bridge = nsm_lookup_bridge_by_name (master,br_name);

  if (bridge == NULL)
    {
      return (NSM_BRIDGE_ERR_NOTFOUND);
    }

  /* find if protection group with eps_id already existing */
  pg = nsm_g8031_find_protection_group(bridge,eps_id);

  if (!pg) 
    {
      return (NSM_BRIDGE_ERR_PG_NOTFOUND);
    }

  if (pg->pg_state == G8031_PG_INITALIZED) 
    return NSM_PG_VLAN_OPERATION_NOT_ALLOWED;

  if ((ifp_w = if_lookup_by_index(&nm->vr->ifm, pg->ifindex_w)) == NULL )
    {
      return (NSM_INTERFACE_NOT_FOUND);
    }

  if ((ifp_p = if_lookup_by_index(&nm->vr->ifm, pg->ifindex_p))== NULL )
    {
      return (NSM_INTERFACE_NOT_FOUND);
    }

  /* make sure that eps_vid is not part of any other protection group
     on the same interface.  */ 
  if (nsm_find_vlan_common_if(bridge,pg->ifindex_w,pg->ifindex_p,eps_vid) == RESULT_OK)
    {
      return (NSM_BRIDGE_ERR_PG_VLAN_EXISTS);
    }
  
  vlan = nsm_vlan_find (bridge,eps_vid);

  if (!vlan)
    {
      return (NSM_VLAN_ERR_VLAN_NOT_FOUND);
    }

  /* check if vlan already part of protection group */
  if (nsm_g8031_eps_vlan_find(pg->eps_vids,eps_vid) == RESULT_OK)
    {
      return (NSM_BRIDGE_ERR_PG_VLAN_EXISTS);
    }

  zif = (struct nsm_if *)ifp_w->info;

  if (! zif)
    {
      return -1;
    }

  br_port = zif->switchport;

  if (! br_port)
    {
      return -1;
    }

  port = &(br_port->vlan_port);

  /* verify given vlan is part of interface */
  if (!NSM_VLAN_BMP_IS_MEMBER(port->staticMemberBmp,vlan->vid))
    {
      return (NSM_VLAN_ERR_IFP_NOT_BOUND);
    }

  zif = (struct nsm_if *)ifp_p->info;

  if (! zif)
    {
      return -1;
    }

  br_port = zif->switchport;

  if (! br_port)
    {
      return -1;
    }

  if (!NSM_VLAN_BMP_IS_MEMBER(port->staticMemberBmp,vlan->vid))
    {
      return (NSM_VLAN_ERR_IFP_NOT_BOUND);
    }

  if (primary)
    pg->primary_vid = eps_vid; 

  if (avl_insert(pg->eps_vids, vlan) < 0)
    {
      return (RESULT_ERROR);
    }

  msg = XCALLOC (MTYPE_NSM_G8031_VLAN, \
      sizeof (struct nsm_msg_g8031_vlanpg));

  if (msg)
    {
      /* Sending delete vlan msg to MSTP to decouple them from mstp */
      pal_mem_set (&vlan_decouple_msg, 0, sizeof (struct nsm_msg_vlan));
      vlan_decouple_msg.vid = eps_vid;
      pal_strncpy (vlan_decouple_msg.vlan_name, vlan->vlan_name, NSM_VLAN_NAMSIZ);
      NSM_SET_CTYPE(vlan_decouple_msg.cindex, NSM_VLAN_CTYPE_NAME);
      pal_strncpy (vlan_decouple_msg.bridge_name, bridge->name, NSM_BRIDGE_NAMSIZ);
      NSM_SET_CTYPE(vlan_decouple_msg.cindex, NSM_VLAN_CTYPE_BRIDGENAME);

      nsm_send_vlan_update_msg (bridge, &vlan_decouple_msg, NSM_MSG_VLAN_ADD_TO_PROTECTION);

      /* Posting msg to ONMD */
      if (primary)
        msg->pvid = eps_vid;

      msg->eps_id = eps_id;
      pal_mem_cpy(msg->bridge_name,br_name,sizeof(br_name));
      msg->vid = eps_vid;

      nsm_g8031_send_vlan_group( msg, NSM_MSG_G8031_CREATE_VLAN_GROUP);
    }

  return (RESULT_ERROR);
}

s_int16_t 
nsm_g8031_eps_vlan_find ( struct avl_tree * eps_tree,
                          u_int16_t eps_vid
                        )
{
  struct avl_node  * node = NULL;
  struct nsm_vlan  * vlan = NULL;

  for (node = avl_top(eps_tree);node;node=avl_next(node))
    {
      if ((vlan = (struct nsm_vlan *) AVL_NODE_INFO(node))== NULL)
        continue;

      if (vlan->vid == eps_vid)
        {
          return (RESULT_OK);
        }
    }

  return (RESULT_ERROR);
}

s_int16_t
nsm_g8031_vlan_config_exists ( struct nsm_bridge *bridge,
                               u_int8_t vid
                             )
{
  struct g8031_protection_group *pg = NULL;
  struct avl_node *pg_node = NULL;
  struct nsm_vlan  * vlan = NULL;
  struct avl_node *eps_vid_node = NULL;

  pal_assert (bridge);
 
  if (bridge->eps_tree != NULL)
    {
      AVL_TREE_LOOP (bridge->eps_tree, pg, pg_node)
        {
          AVL_TREE_LOOP (pg->eps_vids, vlan, eps_vid_node )
            {
              if (vlan->vid == vid)
                return (RESULT_OK);
            } 
        }      
    } 
  return (RESULT_ERROR);     
}

int
nsm_find_vlan_common_if( struct nsm_bridge *bridge,
                         u_int32_t if_w,
                         u_int32_t if_p,
                         u_int8_t vid
                        )
{
  struct nsm_vlan                 * vlan = NULL;
  struct g8031_protection_group   *pg = NULL;
  struct avl_node                 *pg_node = NULL;
  struct avl_node                 *eps_vid_node = NULL;
  
  pal_assert (bridge);

  if (bridge->eps_tree != NULL)
    {
      AVL_TREE_LOOP (bridge->eps_tree, pg, pg_node)
        {
          AVL_TREE_LOOP (pg->eps_vids, vlan, eps_vid_node )
            {
              if (if_w == pg->ifindex_w ||
                  if_w == pg->ifindex_p ||
                  if_p == pg->ifindex_p ||
                  if_p == pg->ifindex_w)
                {
                  /* go thru eps_vid tree, check against given vid */
                  /* if given vid matches, any of vid in pg->vid_list, 
                   * should not allowed */
                  if (vid == vlan->vid)
                    return (RESULT_OK);
                }
            }
        }
    }
  return (RESULT_ERROR);
}

int
nsm_g8031_send_protection_group(struct nsm_bridge * bridge,
    struct nsm_msg_g8031_pg *msg,
    int msgid )
{
    int nbytes;
    struct nsm_server_client *nsc;
    struct nsm_server_entry *nse;
    int i;

    NSM_SERVER_CLIENT_LOOP (ng, i, nsc, nse)
      if (NSM_CHECK_CTYPE(nse->service.bits, NSM_SERVICE_VLAN))
        {
          /* Set nse pointer and size. */
          nse->send.pnt = nse->send.buf + NSM_MSG_HEADER_SIZE;
          nse->send.size = nse->send.len - NSM_MSG_HEADER_SIZE;

          /* Encode NSM bridge message. */
          nbytes = nsm_encode_g8031_create_pg_msg(&nse->send.pnt, &nse->send.size, msg);
          if (nbytes < 0)
            return nbytes;

          /* Send G8031 protection group add message. */
          nsm_server_send_message (nse, 0, 0, msgid, 0, nbytes);
        }

    return 0;
}

int
nsm_g8031_send_vlan_group(struct nsm_msg_g8031_vlanpg *msg,
    int msgid )
{
    int nbytes;
    struct nsm_server_client *nsc;
    struct nsm_server_entry *nse;
    int i;

    NSM_SERVER_CLIENT_LOOP (ng, i, nsc, nse)
      if (NSM_CHECK_CTYPE(nse->service.bits, NSM_SERVICE_VLAN))
        {
          /* Set nse pointer and size. */
          nse->send.pnt = nse->send.buf + NSM_MSG_HEADER_SIZE;
          nse->send.size = nse->send.len - NSM_MSG_HEADER_SIZE;

          /* Encode NSM bridge message. */
          nbytes = nsm_encode_g8031_create_vlan_msg(&nse->send.pnt, &nse->send.size, msg);

          if (nbytes < 0)
            return nbytes;

          /* Send G8031  vlan group add message. */
          nsm_server_send_message (nse, 0, 0, msgid, 0, nbytes);
        }

    return 0;
}

int
nsm_g8031_send_vlan_del (struct nsm_bridge *bridge,
    struct nsm_msg_vlan *msg,
    int msgid)
{
    int nbytes;
    struct nsm_server_client *nsc;
    struct nsm_server_entry *nse;
    int i;

    NSM_SERVER_CLIENT_LOOP (ng, i, nsc, nse)
      if (NSM_CHECK_CTYPE(nse->service.bits, NSM_SERVICE_VLAN)
          && nse->service.protocol_id == APN_PROTO_MSTP)
        {
          /* Set nse pointer and size. */
          nse->send.pnt = nse->send.buf + NSM_MSG_HEADER_SIZE;
          nse->send.size = nse->send.len - NSM_MSG_HEADER_SIZE;

          /* Encode NSM bridge message. */
          nbytes = nsm_encode_vlan_msg (&nse->send.pnt, &nse->send.size, msg);
          if (nbytes < 0)
            return nbytes;

          /* Send VLAN add/delete message. */
          nsm_server_send_message (nse, 0, 0, msgid, 0, nbytes);
        }
    return 0;
}

int
nsm_bridge_set_g8031_port_state ( struct nsm_bridge *bridge,
                                  u_int16_t eps_id, 
                                  u_int32_t ifindex_fwd,
                                  u_int32_t state_f,
                                  u_int32_t ifindex_blck,
                                  u_int32_t state_b)
{
  struct g8031_protection_group *pg = NULL;
  struct avl_node *eps_vid_node = NULL;
  struct nsm_vlan  * vlan = NULL;
  
  pal_assert (bridge);

  pg = nsm_g8031_find_protection_group(bridge,eps_id);

  AVL_TREE_LOOP (pg->eps_vids, vlan, eps_vid_node )
    {
      if (state_b == NSM_BRIDGE_G8031_PORTSTATE_BLOCKING)
        {
          vlan->instance = pg->instance;
#ifdef HAVE_HAL
          hal_bridge_flush_fdb_by_port (bridge->name, ifindex_blck,
              0, vlan->vid,0);
          hal_bridge_set_port_state (bridge->name,ifindex_blck,
              vlan->instance,NSM_BRIDGE_PORT_STATE_BLOCKING);
          hal_bridge_set_learn_fwd (bridge->name, ifindex_blck,
              vlan->instance, PAL_FALSE, PAL_FALSE);
        }
#endif /* HAVE_HAL */ 
    }

  AVL_TREE_LOOP (pg->eps_vids, vlan, eps_vid_node )
    {
      if (state_f == NSM_BRIDGE_G8031_PORTSTATE_FORWARDING)
        {
          vlan->instance = pg->instance;
#ifdef HAVE_HAL
          hal_bridge_flush_fdb_by_port (bridge->name, ifindex_fwd,
              0, vlan->vid,0);
          hal_bridge_set_port_state (bridge->name,ifindex_fwd,
              vlan->instance,NSM_BRIDGE_PORT_STATE_FORWARDING );
          hal_bridge_set_learn_fwd (bridge->name, ifindex_fwd,
              vlan->instance, PAL_FALSE, PAL_FALSE);
        }
#endif /* HAVE_HAL */
    }

  return RESULT_OK;
}

int
nsm_g8031_sync (struct nsm_master *nm, struct nsm_server_entry *nse)
{
  struct nsm_bridge_master *master;
  struct nsm_bridge *bridge = NULL;
  struct g8031_protection_group *pg = NULL;
  struct avl_node * pg_node = NULL;
  struct interface * ifp_w = NULL;
  struct interface *ifp_p = NULL;
  struct nsm_msg_g8031_pg *msg = NULL;
  struct nsm_vlan *vlan;
  struct nsm_msg_g8031_vlanpg *msg_vlan = NULL;
  struct avl_node *eps_vid_node = NULL;

  if (! nm || ! nse || ! nm->bridge)
    return -1;

  master = nsm_bridge_get_master (nm);
  if (! master)
    return -1;
 
  bridge = master->bridge_list;
  if (bridge && bridge->eps_tree)
    {
      /* Protection Group Creation Sync Up Message. */
      AVL_TREE_LOOP (bridge->eps_tree, pg, pg_node)
        {
          ifp_w = ifg_lookup_by_index (&nm->zg->ifg, pg->ifindex_w);
          ifp_p = ifg_lookup_by_index (&nm->zg->ifg, pg->ifindex_p);

          if ((!ifp_w) || (!ifp_p))
            continue;

          protection_group_update_stp (pg->instance, ifp_w, ifp_p, PAL_TRUE, 
                                       NSM_MSG_BRIDGE_ADD_PG);
          
          /* Memory allocation for a protection group message */
          msg = XCALLOC(MTYPE_G8031_PROTECTION_GROUP \
              ,sizeof(struct nsm_msg_g8031_pg));

          if (!msg)
              return (RESULT_ERROR);

          msg->eps_id = pg->eps_id; 
          msg->working_ifindex = ifp_w->ifindex;
          msg->protected_ifindex = ifp_p->ifindex;
          pal_mem_cpy (&msg->bridge_name, bridge->name, strlen(bridge->name));            
          /* send the same details to ONMD */
          nsm_g8031_send_protection_group (bridge, msg,
              NSM_MSG_G8031_CREATE_PROTECTION_GROUP);

          XFREE(MTYPE_G8031_PROTECTION_GROUP,msg);
        }

      /* Protection Group Vlan Sync Messages. */ 
      AVL_TREE_LOOP (bridge->eps_tree, pg, pg_node)
        {
          AVL_TREE_LOOP (pg->eps_vids, vlan, eps_vid_node )
            {
              msg_vlan = XCALLOC(MTYPE_NSM_G8031_VLAN                   \
                  ,sizeof(struct nsm_msg_g8031_vlanpg));

              if (!msg_vlan)
                return (RESULT_ERROR);

              /* Posting msg to ONMD */
              if (pg->primary_vid)
                msg_vlan->pvid = pg->primary_vid;

              msg_vlan->eps_id = pg->eps_id;
              pal_mem_cpy(&msg_vlan->bridge_name, bridge->name  \
                  , strlen(bridge->name));
              msg_vlan->vid = vlan->vid;
              nsm_g8031_send_vlan_group(msg_vlan, \
                  NSM_MSG_G8031_CREATE_VLAN_GROUP);

              XFREE(MTYPE_NSM_G8031_VLAN,msg_vlan);
            }
        }
    }

  return RESULT_OK;
}
#endif /* HAVE_G8031 */

#if defined HAVE_PBB_TE || defined HAVE_G8031 || defined HAVE_G8032 
/* NSM server send message. */
int
nsm_send_vlan_update_msg (struct nsm_bridge *bridge,
                          struct nsm_msg_vlan *msg,
                          int msgid)
{
  int nbytes;
  struct nsm_server_client *nsc;
  struct nsm_server_entry *nse;
  int i;

  NSM_SERVER_CLIENT_LOOP (ng, i, nsc, nse)
    if (NSM_CHECK_CTYPE(nse->service.bits, NSM_SERVICE_VLAN)
        && nse->service.protocol_id == APN_PROTO_MSTP)
      {
        /* Set nse pointer and size. */
        nse->send.pnt = nse->send.buf + NSM_MSG_HEADER_SIZE;
        nse->send.size = nse->send.len - NSM_MSG_HEADER_SIZE;

        /* Encode NSM bridge message. */
        nbytes = nsm_encode_vlan_msg (&nse->send.pnt, &nse->send.size, msg);
        if (nbytes < 0)
          return nbytes;

        /* Send bridge message. */
        nsm_server_send_message (nse, 0, 0, msgid, 0, nbytes);
      }
  return 0;
}

#endif /*defined HAVE_PBB_TE || defined HAVE_G8031 || defined HAVE_G8032  */

#ifdef HAVE_PBB_TE
int
nsm_bridge_decouple_vlan (struct nsm_bridge_master *master,char *brname, 
                          u_int16_t start_vid, u_int16_t end_vid, 
                          u_int8_t protocol)
{
  struct nsm_vlan p;
  struct avl_node *rn = NULL;
  struct nsm_vlan *vlan = NULL;
  struct nsm_msg_vlan msg;
  struct avl_tree *vlan_table;
  struct nsm_bridge *bridge = NULL;
  u_int8_t vid = 0;

  if (!master)
    return -1;

  bridge = nsm_lookup_bridge_by_name(master, brname);

  if (! bridge)
    return NSM_VLAN_ERR_BRIDGE_NOT_FOUND;

  if (nsm_bridge_is_vlan_aware(master, brname) != PAL_TRUE  )
    return NSM_VLAN_ERR_BRIDGE_NOT_VLAN_AWARE; 

#if defined (HAVE_PROVIDER_BRIDGE) 
  if (NSM_BRIDGE_TYPE_PROVIDER (bridge))
    vlan_table = bridge->svlan_table;
  else
#endif /* HAVE_PROVIDER_BRIDGE */
  vlan_table = bridge->vlan_table;

  if (! vlan_table)
    return NSM_VLAN_ERR_GENERAL;

  for (vid = start_vid; vid <= end_vid; vid++)
    {
      NSM_VLAN_SET (&p, vid);
      rn = avl_search (vlan_table, (void *)&p);
      if (rn == NULL)
         continue;

      if (rn)
        vlan = (struct nsm_vlan *)rn->info;
      /* Before inserting set the protocol specific flag */
      avl_insert (vlan_table, (void *)vlan);
  
      /* Sending delete vlan msg to MSTP to decouple them from mstp */
      pal_mem_set (&msg, 0, sizeof (struct nsm_msg_vlan));
      msg.cindex = 0;
      msg.flags = vlan->type;
      msg.vid = vid;
      pal_strncpy (msg.bridge_name, bridge->name, NSM_BRIDGE_NAMSIZ);
      NSM_SET_CTYPE(msg.cindex, NSM_VLAN_CTYPE_BRIDGENAME);

      nsm_send_vlan_update_msg (bridge, &msg, NSM_MSG_VLAN_DELETE);
    }
  return 0;
}

int
nsm_bridge_remove_decoupling (struct nsm_bridge_master *master, char *brname, 
                              u_int16_t start_vid, u_int16_t end_vid, 
                              u_int8_t protocol)
{
  int ret = 0;
  u_int8_t vid = 0;
  struct avl_node *rn = NULL;
  struct nsm_vlan *vlan = NULL;
  struct nsm_bridge *bridge = NULL;
  struct avl_tree *xstp_vlan_table = NULL;
  struct nsm_vlan p;
  struct nsm_msg_vlan msg;

  if (!master)
    return -1;

  bridge = nsm_lookup_bridge_by_name(master, brname);

  if (! bridge)
    return NSM_VLAN_ERR_BRIDGE_NOT_FOUND;

  if (nsm_bridge_is_vlan_aware(master, brname) != PAL_TRUE  )
    return NSM_VLAN_ERR_BRIDGE_NOT_VLAN_AWARE;

#if defined (HAVE_PROVIDER_BRIDGE)
  if (NSM_BRIDGE_TYPE_PROVIDER (bridge))
    xstp_vlan_table = bridge->svlan_table;
  else
#endif /* HAVE_PROVIDER_BRIDGE */
   xstp_vlan_table = bridge->vlan_table;

  for (vid = start_vid; vid <= end_vid; vid++)
    {
      NSM_VLAN_SET (&p, vid);
      rn = avl_search (xstp_vlan_table, (void *)&p);

      if (rn) 
        vlan = (struct nsm_vlan *)rn->info;
      else
        continue;
      
      if(!vlan)
        continue;

      /* if entry not in table then do nothing */
      ret = avl_remove (xstp_vlan_table, (void *)vlan);
  
      /* Sending delete vlan msg to MSTP to decouple them from mstp */
      pal_mem_set (&msg, 0, sizeof (struct nsm_msg_vlan));

      msg.cindex = 0;
        
      if (vlan->vlan_state == NSM_VLAN_ACTIVE)
            msg.flags |= NSM_MSG_VLAN_STATE_ACTIVE;

      msg.flags |= vlan->type;
      msg.vid = vlan->vid;
      pal_strncpy (msg.vlan_name, vlan->vlan_name, NSM_VLAN_NAMSIZ);
      NSM_SET_CTYPE(msg.cindex, NSM_VLAN_CTYPE_NAME);

      msg.flags = vlan->type;
      msg.vid = vid;
      pal_strncpy (msg.bridge_name, bridge->name, NSM_BRIDGE_NAMSIZ);
      NSM_SET_CTYPE(msg.cindex, NSM_VLAN_CTYPE_BRIDGENAME);

      /* post message to mstp to add this vlan */
      nsm_send_vlan_update_msg (bridge, &msg, NSM_MSG_VLAN_ADD);
    }
  return 0;
}
#endif

#if defined HAVE_PBB_TE || defined HAVE_G8031 || defined HAVE_G8032
void
nsm_put_instance (u_int8_t inst_map[], u_int16_t instance)
{
  u_int8_t byte = (instance-1)/8 ;
  u_int8_t temp = 1;
  temp = (temp << ((instance-1) % 8));
  inst_map[byte] = inst_map[byte] &(~temp);
}

u_int16_t
nsm_get_instance (struct nsm_bridge *bridge, u_int8_t inst_map[])
{
 unsigned int i,k,j= 0;
 
 if (!bridge)
   return RESULT_ERROR;
 
 for (i = 0; i < bridge->max_mst_instances/8; i++)
   {
     if (inst_map[i] < 255)
       {
         for (j = 0, k = 0;j <= 255 && (inst_map[i] & j);j = j<<1, k++);
           inst_map[i] |= j;
         return ((i*8)+k);
       }
   }
 return bridge->max_mst_instances;
}
#endif /* #if defined HAVE_PBB_TE || defined HAVE_G8031 || defined HAVE_G8032*/

#ifdef HAVE_G8032
/* Utility routines for G8032 */
int g8032_ring_cmp (void *data1,void *data2)
{
  struct nsm_g8032_ring *node1 = (struct nsm_g8032_ring *) data1;
  struct nsm_g8032_ring *node2 = (struct nsm_g8032_ring *) data2;

  if(node1 == NULL || node2 == NULL)

    return -1;

  if (node1->g8032_ring_id > node2->g8032_ring_id)
   return 1 ;

  if (node2->g8032_ring_id > node1->g8032_ring_id)
   return -1 ;

  return 0;
}

int g8032_vid_cmp ( void * data1, void * data2 )
{
  struct nsm_vlan * vid1 = (struct nsm_vlan *)data1;
  struct nsm_vlan * vid2 = (struct nsm_vlan *)data2;

  if ( vid1->vid > vid2->vid )
    return 1;

  if ( vid2->vid > vid1->vid )
    return -1;

  return 0; 
}

struct nsm_g8032_ring *
nsm_g8032_find_ring_by_id (struct nsm_bridge * bridge,
                           u_int16_t ring_id)
{
  struct avl_node  * node = NULL;
  struct nsm_g8032_ring * ring = NULL;

  for (node = avl_top(bridge->raps_tree);node;node=avl_next(node))
     {
        if ((ring = (struct nsm_g8032_ring *) AVL_NODE_INFO(node))== NULL)
           continue;

        if (ring->g8032_ring_id == ring_id)
          return (ring);
     }
   return (NULL);

}
/* Check if the VLAN is already associated any other rings*/
s_int16_t 
nsm_g8032_find_vlan_ring (struct avl_tree * raps_tree, u_int16_t ring_vid)
{
  struct avl_node  * node = NULL;
  struct nsm_vlan  tmp_vlan;

  pal_mem_set (&tmp_vlan, 0, sizeof(struct nsm_vlan ));

  tmp_vlan.vid = ring_vid;

  node = avl_search(raps_tree, &tmp_vlan);

  if (node)
    return RESULT_OK;
  else
    return RESULT_ERROR;
}

int
nsm_bridge_g8032_create_ring (u_int8_t * br_name,
                              u_int8_t * ifname_e,
                              u_int8_t * ifname_w,
                              u_int16_t instance,
                              u_int16_t ring_id,
                              struct nsm_master *nm)
{
  struct nsm_bridge_master * master = NULL;
  struct interface         * ifp_e  = NULL; 
  struct interface         *ifp_w   = NULL;
  struct nsm_bridge        * bridge = NULL;
  struct nsm_msg_g8032      msg ; 
  struct nsm_g8032_ring    *ring    = NULL;
  struct nsm_if            *nif     = NULL;
#ifdef HAVE_LACPD
  struct interface         *memifp = NULL;
  struct listnode          *lsn    = NULL;
#endif /* HAVE LACP */
          
  master = nsm_bridge_get_master(nm);

  if (master == NULL)
    return RESULT_ERROR;

  bridge = nsm_lookup_bridge_by_name (master,br_name);

  if (bridge == NULL)
      return NSM_BRIDGE_ERR_NOTFOUND;

  if (ifname_e == NULL)
    return NSM_INTERFACE_NOT_FOUND;

  if (ifname_w == NULL)
    return NSM_INTERFACE_NOT_FOUND;

  /* find if the ring exists with ring_id  */
  if ((nsm_g8032_find_ring_by_id (bridge,ring_id)) != NULL )
      return NSM_BRIDGE_RING_ALREADY_EXISTS;

  if ((ifp_e = if_lookup_by_name (&nm->vr->ifm,ifname_e)) == NULL )
      return NSM_INTERFACE_NOT_FOUND;

  if ((ifp_w = if_lookup_by_name (&nm->vr->ifm,ifname_w)) == NULL )
      return NSM_INTERFACE_NOT_FOUND;

#ifdef HAVE_LACPD
  if (NSM_INTF_TYPE_AGGREGATOR(ifp_e))
    {
      AGGREGATOR_MEMBER_LOOP (nif, memifp, lsn)
        {
          if (pal_strcmp (memifp->name,ifp_e->name)== 0)
          return NSM_G8032_LACP_IF_CONFIG_ERR;
        }
    }

  if (NSM_INTF_TYPE_AGGREGATOR(ifp_w))
    {
      AGGREGATOR_MEMBER_LOOP (nif, memifp, lsn)
        {
          if (pal_strcmp (memifp->name,ifp_w->name)== 0)
          return NSM_G8032_LACP_IF_CONFIG_ERR;

        }
    }
#endif /* HAVE LACP */
  /* check if the user had given the same IFNAME for east and west*/
  if (ifp_e->ifindex == ifp_w->ifindex)
    return NSM_G8032_INTERFACE_NAME_SAME_ERROR;
      
  if (!bridge->raps_tree)
    {
      if (avl_create (&bridge->raps_tree, 0, g8032_ring_cmp) < 0)
          return RESULT_ERROR;
    }

  /* Check whether the Instance is Free or Not */
   if (NSM_INSTANCE_BMP_IS_MEMBER(master->instanceBmp, instance))
    return NSM_G8031_INSTANCE_IN_USE;

  /* Memory allocation for a protection group */
  ring = XCALLOC(MTYPE_NSM_G8032_NODE,sizeof(struct nsm_g8032_ring));

  if (!ring)
      return RESULT_ERROR;

  ring->g8032_ring_id = ring_id;
  ring->ifindex_e = ifp_e->ifindex;
  ring->ifindex_w = ifp_w->ifindex;
  ring->instance = instance;

   /* Mark the instance as Used */
  NSM_INSTANCE_BMP_SET(master->instanceBmp, instance);


  if (!ring->g8032_vids)
    {
      if (avl_create(&ring->g8032_vids,0,g8032_vid_cmp) < 0)
        {
          XFREE(MTYPE_NSM_G8032_NODE, ring);
          return RESULT_ERROR;
        }
    }

  if (avl_insert(bridge->raps_tree, ring) < 0)
    {
      XFREE (MTYPE_NSM_G8032_VLAN,ring->g8032_vids);
      XFREE (MTYPE_NSM_G8032_NODE,ring);
      return (RESULT_ERROR);
    }

  /* Disable STP on the Working/Protected Ports. */
  protection_group_update_stp (instance, ifp_e, ifp_w, PAL_TRUE, 
                               NSM_MSG_BRIDGE_ADD_PG);

      pal_mem_set (&msg, 0 , sizeof(struct nsm_msg_g8032));
      msg.ring_id = ring_id;
      msg.east_ifindex   = ifp_e->ifindex; 
      msg.west_ifindex = ifp_w->ifindex;
      pal_strcpy (msg.bridge_name,bridge->name);

      /* send the same details to ONMD */
      nsm_g8032_send_ring_msg (bridge, &msg, NSM_MSG_BRIDGE_ADD_G8032_RING);


  return RESULT_OK;
}

int
nsm_g8032_send_delete_vlan (struct nsm_bridge *bridge,
                            struct nsm_msg_vlan *msg,
    			    int msgid)
{
  int nbytes;
  struct nsm_server_client *nsc = NULL;
  struct nsm_server_entry *nse  = NULL;
  int i;

  NSM_SERVER_CLIENT_LOOP (ng, i, nsc, nse)
    if (NSM_CHECK_CTYPE(nse->service.bits, NSM_SERVICE_VLAN)
        && nse->service.protocol_id == APN_PROTO_MSTP)
      {
        /* Set nse pointer and size. */
        nse->send.pnt = nse->send.buf + NSM_MSG_HEADER_SIZE;
        nse->send.size = nse->send.len - NSM_MSG_HEADER_SIZE;

        /* Encode NSM bridge message. */
        nbytes = nsm_encode_vlan_msg (&nse->send.pnt, &nse->send.size, msg);
        if (nbytes < 0)
          return nbytes;

        /* Send VLAN add/delete message. */
        nsm_server_send_message (nse, 0, 0, msgid, 0, nbytes);
      }
  return 0;
}

int
nsm_g8032_send_vlan_group (struct nsm_msg_g8032_vlan *msg, int msgid )
{
  int nbytes;
  struct nsm_server_client *nsc = NULL;
  struct nsm_server_entry *nse  = NULL;
  int i;

  NSM_SERVER_CLIENT_LOOP (ng, i, nsc, nse)

    if (NSM_CHECK_CTYPE(nse->service.bits, NSM_SERVICE_VLAN))
      {
        /* Set nse pointer and size. */
        nse->send.pnt = nse->send.buf + NSM_MSG_HEADER_SIZE;
        nse->send.size = nse->send.len - NSM_MSG_HEADER_SIZE;

        /* Encode NSM bridge message. */
        nbytes = nsm_encode_g8032_create_vlan_msg (&nse->send.pnt, \
            &nse->send.size, msg);

        if (nbytes < 0)
          return nbytes;

        /* Send G80322  vlan group add message. */
        nsm_server_send_message (nse, 0, 0, msgid, 0, nbytes);
      }

  return 0;
}

int
nsm_g8032_find_vlan_common_if (struct nsm_bridge *bridge,
                               u_int32_t if_e,
                               u_int32_t if_w,
                               u_int16_t vid)
{
  struct nsm_vlan         * vlan = NULL;
  struct nsm_g8032_ring   *ring = NULL;
  struct avl_node         *node = NULL;
  struct avl_node         *ring_vid_node = NULL;

  if (bridge->raps_tree != NULL)
    {
      AVL_TREE_LOOP (bridge->raps_tree, ring, node)
        {
          
          AVL_TREE_LOOP (ring->g8032_vids, vlan, ring_vid_node )
            {
              if (if_e == ring->ifindex_e ||
                  if_w == ring->ifindex_w ||
                  if_e == ring->ifindex_w ||
                  if_w == ring->ifindex_e )
                {
                  /* go thru vid tree, check against given vid */
                  /* if given vid matches, any of vid in pg->vid_list,
                     should not allowed */
                  if (vid == vlan->vid)
                    return (RESULT_OK);
                }
            }
        }
    }
  return (RESULT_ERROR);
}
/* This function creates a primary VLAN for the ring */
s_int16_t
nsm_bridge_g8032_create_vlan (u_int16_t ring_vid, 
                              struct nsm_msg_g8032_vlan *pg_instance,
                              u_int8_t  primary,
                              struct nsm_master *nm )
{
  struct nsm_vlan           * vlan     = NULL;
  struct nsm_g8032_ring     * ring     = NULL;
  struct nsm_bridge         * bridge   = NULL;
  struct nsm_msg_vlan       vlan_decouple_msg;
  struct nsm_bridge_master  * master   = NULL;
  struct nsm_msg_g8032_vlan  msg ;

  if (nm == NULL)
     return RESULT_ERROR;

  master = nsm_bridge_get_master(nm);
  
  if (master == NULL)
    return RESULT_ERROR;

  bridge = nsm_lookup_bridge_by_name (master,pg_instance->bridge_name);

  if (bridge == NULL)
      return NSM_BRIDGE_ERR_NOTFOUND;

  /* find the ring ID already existing */
  ring = nsm_g8032_find_ring_by_id(bridge,pg_instance->ring_id);
  if (!ring)
      return NSM_BRIDGE_ERR_RING_NOTFOUND;

  if (ring->primary_vid && primary == PAL_TRUE)
     return NSM_RING_PRIMARY_VID_ALREADY_EXISTS;

  /* check for the vlan ID
     on the same interface.  */
  if ((vlan = nsm_vlan_find (bridge,ring_vid)) == NULL)
      return NSM_VLAN_ERR_VLAN_NOT_FOUND;

  if (vlan->primary_vid == PAL_TRUE)
      return NSM_RING_PRIMARY_VID_ALREADY_EXISTS;

    if (primary == PAL_TRUE)
     {
        ring->primary_vid = ring_vid;
        vlan->primary_vid = PAL_TRUE;
     }

  if (avl_insert(ring->g8032_vids, vlan) < 0)
    {
      ring->primary_vid = 0;
      vlan->primary_vid = PAL_FALSE;

      return RESULT_ERROR;
    }


  /* Sending delete vlan msg to MSTP to decouple them from mstp */
  pal_mem_set (&vlan_decouple_msg, 0, sizeof (struct nsm_msg_vlan));
  
  vlan_decouple_msg.vid = ring_vid;
  
  pal_strncpy (vlan_decouple_msg.vlan_name, vlan->vlan_name, NSM_VLAN_NAMSIZ);
  
  NSM_SET_CTYPE (vlan_decouple_msg.cindex, NSM_VLAN_CTYPE_NAME);
  
  pal_strncpy (vlan_decouple_msg.bridge_name, bridge->name, 
               NSM_BRIDGE_NAMSIZ);
  
  NSM_SET_CTYPE(vlan_decouple_msg.cindex, NSM_VLAN_CTYPE_BRIDGENAME);
  
  /* Send a messsage to delete VLANs from MSTP*/
  nsm_send_vlan_update_msg (bridge, &vlan_decouple_msg, 
                            NSM_MSG_VLAN_ADD_TO_PROTECTION);

 /* Posting msg to ONMD */
   if (primary == PAL_TRUE)
     msg.is_primary = ring->primary_vid;

    msg.ring_id = ring->g8032_ring_id;
    pal_mem_cpy (&msg.bridge_name,pg_instance->bridge_name,
                  NSM_BRIDGE_NAMSIZ);
    msg.vid = ring_vid;
    nsm_g8032_send_vlan_group(&msg, \
                 NSM_MSG_G8032_CREATE_VLAN_GROUP);

  return RESULT_OK;
}


int 
nsm_g8032_send_ring_msg (struct nsm_bridge * bridge, 
                         struct nsm_msg_g8032 *msg,
                         int msgid )
{
  int nbytes;
  struct nsm_server_client *nsc = NULL;
  struct nsm_server_entry *nse  = NULL;
  int i;

  NSM_SERVER_CLIENT_LOOP (ng, i, nsc, nse)
    if (NSM_CHECK_CTYPE(nse->service.bits, NSM_SERVICE_BRIDGE))
      {
        pal_strncpy (msg->bridge_name, bridge->name, NSM_BRIDGE_NAMSIZ);
        /* Set nse pointer and size. */
        nse->send.pnt = nse->send.buf + NSM_MSG_HEADER_SIZE;
        nse->send.size = nse->send.len - NSM_MSG_HEADER_SIZE;

        /* Encode NSM bridge message. */
        nbytes = nsm_encode_g8032_create_msg (&nse->send.pnt, 
                                              &nse->send.size, msg);
        if (nbytes < 0)
          return nbytes;

        /* Send G8032 add message. */
        nsm_server_send_message (nse, 0, 0, msgid, 0, nbytes);
      }
  return 0;
}

int 
nsm_bridge_g8032_delete_ring (struct nsm_bridge * bridge, 
                              struct nsm_msg_g8032 *msg, int msgid )
{
  int nbytes;
  struct nsm_server_client *nsc = NULL;
  struct nsm_server_entry *nse  = NULL;
  int i;

  NSM_SERVER_CLIENT_LOOP (ng, i, nsc, nse)
    if (NSM_CHECK_CTYPE(nse->service.bits, NSM_SERVICE_BRIDGE))
      {
        pal_strncpy (msg->bridge_name, bridge->name, NSM_BRIDGE_NAMSIZ);
        /* Set nse pointer and size. */
        nse->send.pnt = nse->send.buf + NSM_MSG_HEADER_SIZE;
        nse->send.size = nse->send.len - NSM_MSG_HEADER_SIZE;

        /* Encode NSM bridge message. */
        nbytes = nsm_encode_g8032_create_msg(&nse->send.pnt, 
            &nse->send.size, msg);
        if (nbytes < 0)
          return nbytes;

        /* Send G8032 protection group delete message. */
        nsm_server_send_message (nse, 0, 0, msgid, 0, nbytes);
      }
  return 0;
}
/* Function to Delete a Ring */
s_int16_t
nsm_g8032_delete_ring (u_int8_t * br_name, 
                       u_int16_t ring_id,
                       struct nsm_master *nm)
{
  struct nsm_g8032_ring * ring       = NULL;
  struct nsm_bridge     * bridge     = NULL;
  struct nsm_vlan       * vlan       = NULL;
  struct avl_node       *ring_vid_node = NULL;
  struct nsm_msg_vlan    vlan_decouple_msg;
  struct nsm_bridge_master  * master = NULL;
  struct nsm_msg_g8032    msg;
  struct interface      *ifp_e = NULL;
  struct interface      *ifp_w = NULL;

  if (br_name == NULL || nm == NULL)
    return RESULT_ERROR;

  master = nsm_bridge_get_master(nm);
  if (master == NULL)
    return RESULT_ERROR;

  bridge = nsm_lookup_bridge_by_name (master,br_name);
  if (bridge == NULL)
      return NSM_BRIDGE_ERR_NOTFOUND;

  ring = nsm_g8032_find_ring_by_id (bridge,ring_id);
  if (!ring)
      return G8032_RING_NOT_FOUND;

  /* remove decoupling for this vlan */
  /* Send VLAN Add message to MSTP. */
  AVL_TREE_LOOP (ring->g8032_vids, vlan, ring_vid_node )
    {
      pal_mem_set (&vlan_decouple_msg, 0, sizeof (struct nsm_msg_vlan));
      vlan_decouple_msg.vid = vlan->vid;
      pal_strncpy (vlan_decouple_msg.vlan_name, vlan->vlan_name, 
                   NSM_VLAN_NAMSIZ);
      NSM_SET_CTYPE(vlan_decouple_msg.cindex, NSM_VLAN_CTYPE_NAME);
      pal_strncpy (vlan_decouple_msg.bridge_name, bridge->name, 
                   NSM_BRIDGE_NAMSIZ);
      NSM_SET_CTYPE(vlan_decouple_msg.cindex, NSM_VLAN_CTYPE_BRIDGENAME);
    
       if (vlan->primary_vid == PAL_TRUE)
          vlan->primary_vid = PAL_FALSE;

      /* Add the VLAN back to MSTP */
      nsm_send_vlan_update_msg (bridge, &vlan_decouple_msg,
                                NSM_MSG_VLAN_DEL_FROM_PROTECTION);
    }

 ifp_e = ifg_lookup_by_index (&nm->zg->ifg, ring->ifindex_e);
 ifp_w = ifg_lookup_by_index (&nm->zg->ifg, ring->ifindex_w);

 if ( (ifp_e == NULL) || (ifp_w == NULL) )
     return RESULT_ERROR;

 protection_group_update_stp (ring->instance, ifp_e, ifp_w, PAL_FALSE, 
                              NSM_MSG_BRIDGE_DEL_PG);

  NSM_INSTANCE_BMP_UNSET(master->instanceBmp, ring->instance);
  if (ring)
    {
      if (ring->g8032_vids)
        avl_remove (bridge->raps_tree,ring->g8032_vids);
      
      avl_remove (bridge->raps_tree,ring);
      XFREE (MTYPE_NSM_G8032_NODE,(void *)ring);
    }

  /* Post delete ring  msg to ONMD */
  msg.ring_id = ring_id;
  pal_strcpy (msg.bridge_name, bridge->name);
  nsm_bridge_g8032_delete_ring (bridge, &msg,
                                NSM_MSG_BRIDGE_DEL_G8032_RING);

  return RESULT_OK;
}
/* Send a message to set the port state from Ring Protection */
s_int16_t
nsm_bridge_set_g8032_port_state (struct nsm_bridge *bridge ,
                                 struct nsm_msg_bridge_g8032_port *msg)
{
  struct nsm_g8032_ring *ring = NULL;
  int state;
  s_int32_t ret = 0;

  if (msg == NULL || bridge == NULL)
    return RESULT_ERROR;

  ring  = nsm_g8032_find_ring_by_id (bridge,msg->ring_id);
  if (ring == NULL)
    return RESULT_ERROR;

  if (msg->state == NSM_BRIDGE_G8032_PORTSTATE_BLOCKING)
    state = NSM_BRIDGE_PORT_STATE_BLOCKING;
  else
    state = NSM_BRIDGE_PORT_STATE_FORWARDING;

#ifdef HAVE_HAL

/* Set the port state to blocking/forwarding state based on message type. */
  ret = hal_bridge_set_port_state (bridge->name, msg->ifindex,
                                   ring->instance, state);

  if (ret < 0)
    return NSM_OAM_G8032_FAIL_SET_PORT_STATE;

  /* Flush/No Flush the FDB based on message type. */
  if (msg->fdb_flush)
    {
      ret = hal_bridge_flush_fdb_by_port (bridge->name, msg->ifindex,
                                           ring->instance, 0, 0);
      if (ret < 0)
         return NSM_OAM_G8032_FAIL_TO_FLUSH;
    }
#endif /* HAVE_HAL */

  return RESULT_OK;
}

s_int16_t
nsm_g8032_sync (struct nsm_master *nm, struct nsm_server_entry *nse)
{
  struct nsm_bridge_master *master;
  struct nsm_bridge *bridge = NULL;
  struct nsm_msg_g8032 *msg = NULL;
  struct interface * ifp_w = NULL;
  struct interface * ifp_e = NULL;
  struct nsm_g8032_ring *ring = NULL;
  struct avl_node *node = NULL;
  struct avl_node *ring_vid_node = NULL;
  struct nsm_vlan  * vlan = NULL;
  struct nsm_msg_g8032_vlan  *msg_vlan;

  if (! nm || ! nse || ! nm->bridge)
    return -1;

  master = nsm_bridge_get_master (nm);
  if (! master)
    return -1;

  bridge = master->bridge_list;
  if (bridge!= NULL && bridge->raps_tree != NULL )
    {
      AVL_TREE_LOOP (bridge->raps_tree, ring, node)
        {
          ifp_e = ifg_lookup_by_index (&nm->zg->ifg, ring->ifindex_e);
          ifp_w = ifg_lookup_by_index (&nm->zg->ifg, ring->ifindex_w);

          if ((!ifp_e) || (!ifp_w))
            continue;

          protection_group_update_stp (ring->instance, ifp_e, ifp_w, PAL_TRUE,
              NSM_MSG_BRIDGE_ADD_PG);

          /* Memory allocation for a protection group */
          msg = XCALLOC(MTYPE_NSM_G8032_NODE, sizeof(struct nsm_msg_g8032));

          if (!msg)
            return RESULT_ERROR;

          msg->ring_id = ring->g8032_ring_id;
          msg->east_ifindex = ifp_e->ifindex;
          msg->west_ifindex = ifp_w->ifindex;
          pal_mem_cpy (&msg->bridge_name, bridge->name, strlen(bridge->name));
          /* send the same details to ONMD */
          nsm_g8032_send_ring_msg (bridge, msg,
              NSM_MSG_BRIDGE_ADD_G8032_RING);

          XFREE(MTYPE_NSM_G8032_NODE, msg);
        }

      AVL_TREE_LOOP (bridge->raps_tree, ring, node)
        {
          AVL_TREE_LOOP (ring->g8032_vids, vlan, ring_vid_node  )
            {
              msg_vlan = XCALLOC(MTYPE_NSM_G8032_VLAN,
                  sizeof(struct nsm_msg_g8032_vlan));

              if (!msg_vlan)
                return (RESULT_ERROR);

              /* Posting msg to ONMD */
              if (ring->primary_vid)
                msg_vlan->is_primary = ring->primary_vid;

              msg_vlan->ring_id = ring->g8032_ring_id;
              pal_mem_cpy (&msg_vlan->bridge_name, bridge->name,
                  NSM_BRIDGE_NAMSIZ);
              msg_vlan->vid = vlan->vid;
              nsm_g8032_send_vlan_group(msg_vlan, \
                  NSM_MSG_G8032_CREATE_VLAN_GROUP);

              XFREE(MTYPE_NSM_G8032_VLAN,msg_vlan);
            }
        }
    }
    return RESULT_OK;
}

/* This function checks if the VLAN is bound to
 * G8032 before deleting if from the VLAN database */
s_int16_t
nsm_g8032_vlan_config_exists (struct nsm_bridge *bridge,
                               u_int16_t vid)
{
  struct nsm_g8032_ring *ring = NULL;
  struct avl_node   *ring_node = NULL;
  struct avl_node  *ring_vid_node = NULL;
  struct nsm_vlan  vlan;

  if (bridge == NULL)
    return RESULT_ERROR;

  if (bridge->raps_tree != NULL)
    {
      AVL_TREE_LOOP (bridge->raps_tree, ring, ring_node)
        {
          pal_mem_set (&vlan,0 , sizeof(struct nsm_vlan));

          vlan.vid = vid;

          ring_vid_node = avl_search (ring->g8032_vids, &vlan);

          if (ring_vid_node)
            return RESULT_OK;
          else
           return RESULT_ERROR;
        }
    }
  return RESULT_ERROR;
}

#endif /* HAVE_G8032 */

#if defined HAVE_G8031 || defined HAVE_G8032
 /* Update the MSTP Module for Disabling STP. */
int
protection_group_update_stp (u_int16_t instance,
                             struct interface *ifp_w,
                             struct interface *ifp_p,
                             u_int8_t spanning_tree_disable,
                             u_int8_t msg_id)
{
  struct nsm_if *nif_w;
  struct nsm_if *nif_p;
  struct nsm_msg_bridge_pg msg;
  struct nsm_bridge_port *br_port_w = NULL;
  struct nsm_bridge_port *br_port_p = NULL;

  pal_mem_set (&msg, 0, sizeof (struct nsm_msg_bridge_pg));

  if (!ifp_w || !ifp_p)
    return RESULT_ERROR;

  nif_w = (struct nsm_if *)ifp_w->info;
  if (! nif_w)
    return NSM_BRIDGE_ERR_NOTFOUND;

  nif_p = (struct nsm_if *)ifp_p->info;
  if (! nif_w)
    return NSM_BRIDGE_ERR_NOTFOUND;

  if (!nif_w->bridge || !nif_p->bridge)
    return NSM_BRIDGE_ERR_NOT_BOUND;

  br_port_w = nif_w->switchport;
  br_port_p = nif_p->switchport;

  if (!br_port_w || !br_port_p)
    return RESULT_ERROR;

  msg.instance = instance;

  msg.ifindex = XCALLOC (MTYPE_TMP, 2 * sizeof (u_int32_t));

  if (msg_id == NSM_MSG_BRIDGE_ADD_PG)
    {
      /* Allocate ifindex array. */
      msg.ifindex[msg.num++] = ifp_w->ifindex;
      msg.ifindex[msg.num++] = ifp_p->ifindex;

      msg.spanning_tree_disable = spanning_tree_disable;
    }
  else if (msg_id == NSM_MSG_BRIDGE_DEL_PG)
    {
      if (br_port_w->spanning_tree_disable == PAL_FALSE)
        msg.ifindex[msg.num++] = ifp_w->ifindex;

      if (br_port_p->spanning_tree_disable == PAL_FALSE)
        msg.ifindex[msg.num++] = ifp_p->ifindex;
    }
  /* If the bridge is STP/RSTP enable/disable Spanning Tree. */
  if ((nif_w->bridge->type != NSM_BRIDGE_TYPE_STP) ||
      (nif_w->bridge->type != NSM_BRIDGE_TYPE_RSTP))
    {
      pal_strncpy (msg.bridge_name, nif_w->bridge->name,
                      NSM_BRIDGE_NAMSIZ);
      nsm_pg_send_instance_msg_to_mstp (nif_w->bridge,&msg,msg_id );
    }
  /* Free ifindex array. */
  XFREE (MTYPE_TMP, msg.ifindex);

  return RESULT_OK;
}

int
nsm_pg_send_instance_msg_to_mstp (struct nsm_bridge * bridge,
                                  struct nsm_msg_bridge_pg * msg,
                                  int msgid )
{
  int nbytes;
  struct nsm_server_client *nsc = NULL;
  struct nsm_server_entry *nse  = NULL;
  int i;

  /* Send interface add message to PM. */
  NSM_SERVER_CLIENT_LOOP (ng, i, nsc, nse)
    if (NSM_CHECK_CTYPE (nse->service.bits, NSM_SERVICE_BRIDGE)
        && nse->service.protocol_id == APN_PROTO_MSTP)
      {
        /* Set nse pointer and size. */
        nse->send.pnt = nse->send.buf + NSM_MSG_HEADER_SIZE;
        nse->send.size = nse->send.len - NSM_MSG_HEADER_SIZE;

        /* Encode NSM bridge interface message. */
        nbytes = nsm_encode_bridge_pg_msg (&nse->send.pnt,
            &nse->send.size, msg);
        if (nbytes < 0)
          {
            return nbytes;
          }

        /* Send bridge interface message. */
        nsm_server_send_message (nse, 0, 0, msgid,
            0, nbytes);

      }
  return 0;
}

/* New Instance has been added in MSTP. Mark the instance as blocked
 * for the protection groups.
 */  
int 
nsm_server_recv_block_protection_instance (struct nsm_msg_header *header,
                                           void *arg, void *message)
{
  struct nsm_bridge_master *master;
  struct nsm_server_entry *nse;
  struct nsm_msg_vlan *msg;
  struct nsm_master *nm;
  struct nsm_server *ns;
  struct nsm_bridge *br;

  /* Set connection and message.  */
  nse = arg;
  ns = nse->ns;
  msg = message;

  nm = nsm_master_lookup_by_id (nzg, header->vr_id);
  if (nm == NULL)
    return RESULT_ERROR;

  master = nsm_bridge_get_master(nm);
  br = nsm_lookup_bridge_by_name(master, msg->bridge_name);
  if ( !br )
    return RESULT_ERROR;

  NSM_INSTANCE_BMP_SET(master->instanceBmp, msg->br_instance);
 
  return RESULT_OK;
}

int
nsm_server_recv_unblock_protection_instance (struct nsm_msg_header *header,
                                             void *arg, void *message)
{
  struct nsm_bridge_master *master;
  struct nsm_server_entry *nse;
  struct nsm_msg_vlan *msg;
  struct nsm_master *nm;
  struct nsm_server *ns;
  struct nsm_bridge *br;

  /* Set connection and message.  */
  nse = arg;
  ns = nse->ns;
  msg = message;

  nm = nsm_master_lookup_by_id (nzg, header->vr_id);
  if (nm == NULL)
    return RESULT_ERROR;

  master = nsm_bridge_get_master(nm);
  br = nsm_lookup_bridge_by_name(master, msg->bridge_name);
  if ( !br )
    return RESULT_ERROR;

  NSM_INSTANCE_BMP_UNSET(master->instanceBmp, msg->br_instance);

  return RESULT_OK;
}

int 
nsm_api_bridge_get_hw_instance (u_int16_t *instance)
{
  return RESULT_ERROR;
}
#endif /* HAVE_G8031 || HAVE_G8032*/

#endif /* HAVE_CUSTOM1 */
