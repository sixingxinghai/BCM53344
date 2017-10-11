/* Copyright (C) 2002-2003  All Rights Reserved. */

#include "pal.h"
#include "lib.h"

#ifdef HAVE_MPLS_VC

#include "vty.h"
#include "if.h"
#include "thread.h"
#include "prefix.h"
#include "table.h"
#include "linklist.h"
#include "label_pool.h"
#include "log.h"
#include "network.h"
#include "stream.h"
#include "mpls_common.h"
#include "mpls_client.h"
#include "mpls.h"
#include "nsmd.h"
#ifdef HAVE_DSTE
#include "dste.h"
#endif /* HAVE_DSTE */
#include "nsm_message.h"
#include "ptree.h"
#ifdef HAVE_DIFFSERV
#include "diffserv.h"
#endif /* HAVE_DIFFSERV */
#include "nsmd.h"
#ifdef HAVE_DSTE
#include "nsm_dste.h"
#endif /* HAVE_DSTE */
#include "nsm_debug.h"
#include "nsm_mpls.h"
#include "nsm_mpls_vc.h"
#include "nsm_mpls_rib.h"
#include "nsm_router.h"
#include "nsm_server.h"
#include "nsm_interface.h"
#include "nsm_mpls_vc_api.h"
#include "nsm_mpls_dep.h"
#include "nsm_mpls_fwd.h"

#ifdef HAVE_SNMP
#include "nsm_mpls_vc_snmp.h"
#endif /* HAVE_SNMP */
#include "nsm_mpls_vc_api.c"
#ifdef HAVE_VPLS
#include "nsm_vpls.h"
#endif /* HAVE_VPLS */

#ifdef HAVE_VPLS
#include "nsm_vpls.h"
#endif /* HAVE_VPLS */

#ifdef HAVE_VLAN
#include "nsm_vlan.h"
#endif /* HAVE_VLAN */

#include "bitmap.h"

#ifdef HAVE_MS_PW
#include "nsm_mpls_ms_pw.h"
#endif /* HAVE_MS_PW */

void nsm_mpls_l2_circuit_add_send (struct nsm_mpls_if *, 
                                   struct nsm_mpls_circuit *);
void nsm_mpls_l2_circuit_del_send (struct nsm_mpls_if *,
                                   struct nsm_mpls_circuit *);

#ifdef HAVE_VPLS
int nsm_vpls_add_send (struct nsm_vpls *, cindex_t);
void nsm_vpls_spoke_vc_add_send (struct nsm_mpls_circuit *, u_int16_t);
void nsm_vpls_spoke_vc_delete_send (struct nsm_mpls_circuit *);
#endif /* HAVE_VPLS */
extern pal_time_t nsm_get_sys_up_time ();
#ifdef HAVE_SNMP
struct nsm_mpls_circuit_temp *
nsm_mpls_l2_circuit_temp_lookup (u_int32_t ix, u_int32_t lookup_type)
{
  struct nsm_mpls_circuit_temp *vc_temp = NULL;
  struct listnode *node = NULL;

  if (lookup_type == IDX_LOOKUP)  /* Lookup to be done based on Index */
    {
      LIST_LOOP (vc_entry_temp_list, vc_temp, node)
        if (vc_temp->vc_id == ix)
          return vc_temp;
    }
  return NULL;
}

struct nsm_mpls_circuit_temp *
nsm_mpls_l2_circuit_temp_lookup_by_name (char *name, u_int32_t lookup_type)
{
  struct nsm_mpls_circuit_temp *vc_temp = NULL;
  struct listnode *node = NULL;

  if (lookup_type == IDX_LOOKUP)  /* Lookup to be done based on Index */
    {
      LIST_LOOP (vc_entry_temp_list, vc_temp, node)
        if ((vc_temp->vc_name) && (pal_strcmp (name, vc_temp->vc_name) == 0))
          return vc_temp;
      
    }
  return NULL;
}

struct nsm_mpls_circuit_temp *
nsm_mpls_l2_circuit_temp_lookup_next (u_int32_t ix)
{
  struct nsm_mpls_circuit_temp *vc_temp_curr = NULL;
  struct nsm_mpls_circuit_temp *vc_temp_best = NULL;
  struct listnode *node = NULL;

  LIST_LOOP (vc_entry_temp_list, vc_temp_curr, node)
    {
      if (ix < vc_temp_curr->vc_snmp_index)
        {
          if (vc_temp_best == NULL ||
              (vc_temp_curr->vc_snmp_index < vc_temp_best->vc_snmp_index))
            vc_temp_best = vc_temp_curr;
        }
    }
  return vc_temp_best;
}
#endif /* HAVE_SNMP */

/* Free VC entry */
void
mpls_vc_free (struct nsm_mpls_circuit *vc)
{
  NSM_ASSERT (vc != NULL);
  if (!vc)
    return;
  /* remove it from table */
  if (vc->rn)
    {
      vc->rn->info = NULL;
      route_unlock_node (vc->rn);
    }

  if (vc->name)
    XFREE (MTYPE_TMP, vc->name);

  if (vc->tunnel_name)
    XFREE (MTYPE_TMP, vc->tunnel_name);

  XFREE (MTYPE_NSM_VIRTUAL_CIRCUIT, vc);
}

/* Lookup "next" MPLS Layer-2 Virtual Circuit by id. */
struct nsm_mpls_circuit *
nsm_mpls_l2_circuit_lookup_next_by_id (struct nsm_master *nm, u_int32_t vc_id)
{
  struct route_node *rn;
  struct prefix p;

  if ((! NSM_MPLS) || (! NSM_MPLS->vc_table))
    return NULL;

  /* if no current vc_id provided */
  if (vc_id == 0)
    {
      for (rn = route_top (NSM_MPLS->vc_table); rn; rn = route_next (rn))
        {
          if (rn->info != NULL)
            {
              route_unlock_node (rn);
              return (struct nsm_mpls_circuit *)rn->info;
            }
        }
      return NULL;      
    }

  /* Fill prefix. */
  pal_mem_set (&p, 0, sizeof (struct prefix));
  p.family = AF_INET;
  p.prefixlen = NSM_MPLS_CIRCUIT_ID_LEN;
  PREP_FOR_NETWORK (p.u.prefix4.s_addr, vc_id, NSM_MPLS_CIRCUIT_ID_LEN);

  rn = route_node_get (NSM_MPLS->vc_table, &p);
 
  if (!rn)
    return NULL;

  /* then do a getnext from the current vc*/
  for (rn = route_next (rn); rn; rn = route_next (rn))
    {
      if (rn->info != NULL) 
        {
          route_unlock_node (rn);
          return (struct nsm_mpls_circuit *)rn->info;
        }
    }

  return NULL;
}

/* Lookup an MPLS Layer-2 Virtual Circuit by id. */
struct nsm_mpls_circuit *
nsm_mpls_l2_circuit_lookup_by_id (struct nsm_master *nm, u_int32_t vc_id)
{
  struct route_node *rn;
  struct prefix p;

  if ((! NSM_MPLS) || (! NSM_MPLS->vc_table) ||
      (vc_id == NSM_MPLS_L2_VC_UNDEFINED))
    return NULL;

  /* Fill prefix. */
  pal_mem_set (&p, 0, sizeof (struct prefix));
  p.family = AF_INET;
  p.prefixlen = NSM_MPLS_CIRCUIT_ID_LEN;
  PREP_FOR_NETWORK (p.u.prefix4.s_addr, vc_id, NSM_MPLS_CIRCUIT_ID_LEN);

  rn = route_node_lookup (NSM_MPLS->vc_table, &p);
  if (rn)
    {
      route_unlock_node (rn);
      return (struct nsm_mpls_circuit *)rn->info;
    }

  return NULL;
}

/* Lookup an MPLS Layer-2 Circuit by name. */
struct nsm_mpls_circuit *
nsm_mpls_l2_circuit_lookup_by_name (struct nsm_master *nm, char *name)
{
  struct nsm_mpls_circuit *vc;
  struct route_node *rn;

  if ((! NSM_MPLS) || (! NSM_MPLS->vc_table) || (name == NULL))
    return NULL;

  for (rn = route_top (NSM_MPLS->vc_table); rn; rn = route_next (rn))
    {
      /* If no info, skip. */
      vc = rn->info;
      if (! vc)
        continue;

      if (pal_strcmp (name, vc->name) == 0)
        {
          route_unlock_node (rn);
          return vc;
        }
    }
  return NULL;
}

#ifdef HAVE_SNMP
/** @brief Function to  loop through the L2VC's 
    for snmp get request based on the vc_snmp_index

    @param struct nsm_master *nm            - NSM master
    @param vc_index                         - vc_snmp_index

    @return struct nsm_mpls_circuit * - VC structure
*/
struct nsm_mpls_circuit *
nsm_mpls_l2_circuit_snmp_lookup_by_index (struct nsm_master *nm,
                                          u_int32_t vc_index)
{
   struct nsm_mpls_circuit *vc;
   struct route_node *rn;

   if ((! NSM_MPLS) || (! NSM_MPLS->vc_table) ||
           (vc_index == NSM_MPLS_L2_VC_UNDEFINED))
       return NULL;

  for (rn = route_top (NSM_MPLS->vc_table); rn; rn = route_next (rn))
    {
      /* If no info, skip. */
      vc = rn->info;
      if (! vc)
        continue;

      if (vc->vc_snmp_index == vc_index)
        {
          route_unlock_node (rn);
          return vc;
        }
    }
   return NULL;
}

/** @brief Function to  loop through the L2 temp VC's 
    for snmp get request based on the vc_snmp_index

    @param vc_index     - vc_snmp_index

    @return  struct nsm_mpls_circuit_temp * - VC temp structure
*/
struct nsm_mpls_circuit_temp *
nsm_mpls_l2_circuit_snmp_temp_lookup_by_index (u_int32_t ix)
{
  struct nsm_mpls_circuit_temp *vc_temp = NULL;
  struct listnode *node = NULL;

  LIST_LOOP (vc_entry_temp_list, vc_temp, node)
    if (vc_temp->vc_snmp_index == ix)
       return vc_temp;
  
  return NULL;
}

/** @brief Function to  loop through the L2 temp VC's
    for snmp getnext request based on the vc_snmp_index

    @param vc_index     - vc_snmp_index

    @return  struct nsm_mpls_circuit_temp * - VC temp structure
*/
struct nsm_mpls_circuit_temp *
nsm_mpls_l2_circuit_temp_snmp_lookup_next (u_int32_t ix)
{
  struct nsm_mpls_circuit_temp *vc_temp_curr = NULL;
  struct nsm_mpls_circuit_temp *vc_temp_best = NULL;
  struct listnode *node = NULL;

  LIST_LOOP (vc_entry_temp_list, vc_temp_curr, node)
    {
      if (ix < vc_temp_curr->vc_snmp_index)
        {
          if (vc_temp_best == NULL ||
              (vc_temp_curr->vc_snmp_index < vc_temp_best->vc_snmp_index))
            vc_temp_best = vc_temp_curr;
        }
    }
  return vc_temp_best;
}
#endif /*HAVE_SNMP*/

/* Initiate a new layer-2 circuit object. */
struct nsm_mpls_circuit *
nsm_mpls_l2_circuit_new (struct nsm_master *nm,
                         char *name,
                         u_int8_t is_passive)
{
  struct nsm_mpls_circuit *vc = NULL;

  if ((! NSM_MPLS) || (! NSM_MPLS->vc_table) || (name == NULL))
    return NULL;

  /* Create new Virtual Circuit and add it to top list. */
  vc = XCALLOC (MTYPE_NSM_VIRTUAL_CIRCUIT, sizeof (struct nsm_mpls_circuit));
  if (! vc)
    return NULL;

  vc->name = XSTRDUP (MTYPE_TMP, name);

#ifdef HAVE_MS_PW
  if (is_passive)
    vc->ms_pw_role = NSM_MSPW_ROLE_PASSIVE;
  else
    vc->ms_pw_role = NSM_MSPW_ROLE_ACTIVE;
#endif /* HAVE_MS_PW */

  return vc;
}

/* Lookup an MPLS Layer-2 Virtual Circuit Group. */
struct nsm_mpls_vc_group *
nsm_mpls_l2_vc_group_lookup_by_id (struct nsm_master *nm,
                                   u_int32_t group_id)
{
  struct nsm_mpls_vc_group *group;
  struct listnode *ln;

  if ((! NSM_MPLS) || (! NSM_MPLS->vc_group_list) ||
      (group_id == NSM_MPLS_L2_VC_UNDEFINED))
    return NULL;
  
  LIST_LOOP (NSM_MPLS->vc_group_list, group, ln)
    {
      if (group->id == group_id)
        return group;
    }
  return NULL;
}

struct nsm_mpls_vc_group *
nsm_mpls_l2_vc_group_lookup_by_name (struct nsm_master *nm, char *name)
{
  struct nsm_mpls_vc_group *group;
  struct listnode *ln;

  if ((! NSM_MPLS) || (! NSM_MPLS->vc_group_list) || (name == NULL))
    return NULL;

  LIST_LOOP (NSM_MPLS->vc_group_list, group, ln)
    {
      if (pal_strcmp (name, group->name) == 0)
        return group;
    }
  return NULL;
}

struct nsm_mpls_vc_group *
nsm_mpls_l2_vc_group_new (struct nsm_master *nm,
                          char *name)
{
  struct nsm_mpls_vc_group *group = NULL;
  struct listnode *ln, *pp = NULL;
  u_int32_t id;

  if ((! NSM_MPLS) || (! NSM_MPLS->vc_group_list) || (name == NULL))
    return NULL;

  /* Fetch new id. */
  id = 1;
  pp = NULL;
  LIST_LOOP (NSM_MPLS->vc_group_list, group, ln)
    {
      if (group->id != id)
        break;
      id++;
      pp = ln;
    }

  /* Create new Virtual Circuit and add it to top list. */
  group = XCALLOC (MTYPE_NSM_VC_GROUP, sizeof (struct nsm_mpls_vc_group));
  if (! group)
    return NULL;
  group->id = id;
  group->name = XSTRDUP (MTYPE_TMP, name);
  ln = listnode_add_after (NSM_MPLS->vc_group_list, pp, group);
  group->ln = ln;

  /* Initialize list for vc's. */
  group->vc_list = list_new ();
#ifdef HAVE_VPLS
  /* Initialize list for vpls. */
  group->vpls_list = list_new ();
#endif /* HAVE_VPLS */

  return group;
}

/* Unegister a virtual circuit from a group. */
void
nsm_mpls_l2_vc_group_unregister (struct nsm_master *nm,
                                 struct nsm_mpls_vc_group *group,
                                 struct nsm_mpls_circuit *vc)
{
  struct nsm_mpls_circuit *tmp_vc;
  struct listnode *ln;

  NSM_ASSERT (group != NULL);

  /* Find vc. */
  LIST_LOOP (group->vc_list, tmp_vc, ln)
    {
      if (tmp_vc == vc)
        {
          vc->group = NULL;
          list_delete_node (group->vc_list, ln);
          break;
        }
    }
}

struct nsm_mpls_vc_group *
nsm_mpls_l2_vc_group_create (struct nsm_master *nm,
                             char *name, u_int32_t group_id)
{
  struct nsm_mpls_vc_group *group = NULL;
  struct listnode *ln, *pp = NULL;
      
  if ((NSM_MPLS) && (NSM_MPLS->vc_group_list) && name)
    {
      LIST_LOOP (NSM_MPLS->vc_group_list, group, ln)
        {
          if (group->id > group_id)
            break;
          pp = ln;
        }

      /* Create new Virtual Circuit and add it to top list. */
      group = XCALLOC (MTYPE_NSM_VC_GROUP, sizeof (struct nsm_mpls_vc_group));
      if (group)
        {
          group->id = group_id;
          group->name = XSTRDUP (MTYPE_TMP, name);
          ln = listnode_add_after (NSM_MPLS->vc_group_list, pp, group);
          group->ln = ln;

          /* Initialize list for vc's. */
          group->vc_list = list_new ();
#ifdef HAVE_VPLS
          /* Initialize list for vpls. */
          group->vpls_list = list_new ();
#endif /* HAVE_VPLS */
        }
    }
  return group;
}

int
nsm_mpls_l2_vc_group_delete (struct nsm_master *nm, 
                             struct nsm_mpls_vc_group *group )
{
  /* If vc_list is empty, remove group. */
  if (listcount (group->vc_list) <= 0)
    {
      struct listnode *ln;

      /* Remove vc list. */
      list_delete (group->vc_list);

      /* Remove vpls list. */
#ifdef HAVE_VPLS
      list_delete (group->vpls_list);
#endif /*HAVE_VPLS*/

      /* Free name. */
      XFREE (MTYPE_TMP, group->name);

      /* Remove list node. */
      ln = group->ln;
      list_delete_node (NSM_MPLS->vc_group_list, ln);

      /* Delete group. */
      XFREE (MTYPE_NSM_VC_GROUP, group);

      return NSM_SUCCESS;
    }
    
  return NSM_FAILURE;
}

/* Register a virtual circuit in a group. */
void
nsm_mpls_l2_vc_group_register (struct nsm_master *nm, char *name, 
                               u_int32_t group_id, struct nsm_mpls_circuit *vc)
{
  struct nsm_mpls_vc_group *group = NULL;

  /* If group-id is passed as an argument and not configured. */
  if (group_id)
    {
      if (! nsm_mpls_ac_group_create (nm , name, group_id))
        return;
    }

  /* Lookup group. */
  group = nsm_mpls_l2_vc_group_lookup_by_name (nm, name);

  if (! group)
    group = nsm_mpls_l2_vc_group_new (nm, name);

  if (! group)
    return;

  /* Add listnode and set group. */
  listnode_add (group->vc_list, (void *)vc);
  vc->group = group;
}

/* Unet Virtual Circuit ID from interface in MPLS Forwarder. */
void
nsm_mpls_l2_circuit_del_from_fwd (u_int32_t vc_id, struct interface *ifp,
                                  u_int16_t vlan_id)
                                  
{
  struct if_ident ident;
  int ret;

  /* Fill ident. */
  pal_mem_set (&ident, 0, sizeof (struct if_ident));
  ident.if_index = ifp->ifindex;
  pal_mem_cpy (&ident.if_name, ifp->name, INTERFACE_NAMSIZ);
  
  ret = apn_mpls_vc_end (vc_id, &ident, vlan_id);
  if (ret < 0)
    zlog_warn (NSM_ZG, "Could not unbind Virtual Circuit id from "
               "interface %s", ifp->name);
  else if (IS_NSM_DEBUG_EVENT)
    zlog_info (NSM_ZG, "Successfully unbound Virtual Circuit id from "
               "interface %s", ifp->name);
}

/* Set Virtual Circuit ID for interface in MPLS Forwarder. */
int
nsm_mpls_l2_circuit_add_to_fwd (u_int32_t vc_id, struct interface *ifp,
                                u_int16_t vlan_id)
{
  struct if_ident ident;
  int ret;

  /* Fill ident. */
  pal_mem_set (&ident, 0, sizeof (struct if_ident));
  ident.if_index = ifp->ifindex;
  pal_mem_cpy (&ident.if_name, ifp->name, INTERFACE_NAMSIZ);

  ret = apn_mpls_vc_init (vc_id, &ident, vlan_id);
  if (ret < 0)
    zlog_warn (NSM_ZG, "Could not bind Virtual Circuit id %d to "
               "interface %s", (int)vc_id, ifp->name);
  else if (IS_NSM_DEBUG_EVENT)
    zlog_info (NSM_ZG, "Virtual Circuit id %d successfully bound to "
               "interface %s", (int)vc_id, ifp->name);

  return ret;
}

/* Install VC configure above and below. */
int
nsm_mpls_if_install_vc_data (struct nsm_mpls_if *mif,
                             struct nsm_mpls_circuit *vc, u_int16_t vlan_id)
{
  struct if_ident ident;
  int ret;

  /* If there is a real virtual circuit. */
  if (vc && mif->ifp)
    {
      if (!mif->nls)
        {
          pal_mem_set (&ident, 0, sizeof (struct if_ident));
          ident.if_index = mif->ifp->ifindex;
          pal_mem_cpy (ident.if_name, mif->ifp->name, INTERFACE_NAMSIZ);
          /* Loopback interface check, return err if loopback */
          if (if_is_loopback (mif->ifp))
            {
              zlog_warn (NSM_ZG, "ERROR: Invalid Interface for MPLS: %s\n",
                         mif->ifp->name);
              return -1;
            }
          ret = apn_mpls_if_init (&ident, PLATFORM_WIDE_LABEL_SPACE);
          if (ret < 0)
            {
              zlog_warn (NSM_ZG, "Could not enable interface %s with index %d "
                         "for MPLS", mif->ifp->name, mif->ifp->ifindex);
              return ret;
            }
          if (IS_NSM_DEBUG_EVENT)
            zlog_info (NSM_ZG, "Enabled interface %s with index %d for MPLS",
                       mif->ifp->name, mif->ifp->ifindex);
        }

      SET_FLAG (vc->pw_status, NSM_CODE_PW_NOT_FORWARDING);
      
      /* Send add to overlying protocols. */
      if (! if_is_running (mif->ifp))
        {
          SET_FLAG (vc->pw_status, NSM_CODE_PW_AC_INGRESS_RX_FAULT);
          SET_FLAG (vc->pw_status, NSM_CODE_PW_AC_EGRESS_TX_FAULT);
        }

      nsm_mpls_l2_circuit_add_send (mif, vc);
    }
  return 0;
}

struct nsm_mpls_circuit *
nsm_mpls_l2_circuit_create (struct nsm_master *nm, char *vc_name,
                            u_int32_t vc_id,
                            u_int8_t is_passive,
                            int *ret)
{
  struct nsm_mpls_circuit *vc = NULL;

#ifdef HAVE_VPLS
    {
      struct nsm_vpls *vpls;

      /* should not have a vpls instance with the same ID */
      vpls = nsm_vpls_lookup (nm, vc_id, NSM_FALSE);
      if (vpls)
        {
          zlog_err (NSM_ZG, "%s - %d: A VPLS instance with same ID exists\n",
                    __FUNCTION__, __LINE__);
          *ret = NSM_ERR_INVALID_VC_ID;
          return NULL; 
        }
    }
#endif /* HAVE_VPLS */

  /* Get new Virtual Circuit object. */
  vc = nsm_mpls_l2_circuit_new (nm, vc_name, is_passive);

  if (!vc)
    *ret = NSM_FAILURE;

  return vc;
}

/**@brief: Function to fill data into the dummy vc that was created as a part
           of bind/ms_pw stitch before the vc is created.
    @param *nm         - pointer to the NSM master global.
    @param *vc         - pointer to the MPLS vc.
    @param *egress     - pointer to the address of the vc's endpoint.
    @param *group_name - pointer to the group name of the vc.
    @param group_id    - Group id of the VC group.
    @param cc_types    - cc_types.
    @param cv_types    - cv_types.
    @return            - NSM_SUCCESS/NSM_FAILURE.
*/
int
nsm_mpls_vc_fill_data (struct nsm_master *nm,
                       u_int32_t vc_id,
                       struct pal_in4_addr *egress,
                       char *group_name, u_int32_t group_id,
                       u_char c_word,
                       u_char mapping_type,
                       char *tunnel_info,
                       u_char tunnel_dir,
                       u_int8_t fec_type_vc,
#ifdef HAVE_VCCV
                       u_int8_t cc_types, u_int8_t cv_types,
#endif /* HAVE_VCCV */
                       struct nsm_mpls_circuit *vc)
{
  struct nsm_mpls_if *mif = NULL;
  struct listnode *ln = NULL, *node = NULL;
  int found = 0;
  struct nsm_mpls_vc_info *vc_info = NULL;
  struct route_node *rn = NULL;
  struct prefix p;
  int ret = NSM_FAILURE;

  if (!vc)
    return NSM_ERR_VC_ID_NOT_FOUND;

  vc->id = vc_id;

  vc->cw = c_word;
  vc->mapping_type = mapping_type;
  if ( mapping_type == MPLS_VC_MAPPING_TUNNEL_NAME)
    {
      vc->tunnel_name = XSTRDUP (MTYPE_TMP, tunnel_info);
      vc->tunnel_dir = tunnel_dir;
    }
  else if (mapping_type == MPLS_VC_MAPPING_TUNNEL_ID)
    {
      vc->tunnel_id = pal_strtou32 (tunnel_info, NULL, 10);
      vc->tunnel_dir = tunnel_dir;
    }
  else
    {
      vc->tunnel_name = NULL;
      vc->tunnel_id = 0;
      vc->tunnel_dir = 0;
    }

  if (fec_type_vc == 0)
    {
#ifdef HAVE_FEC129
      vc->fec_type_vc = PW_OWNER_GEN_FEC_SIGNALING;
#else
      vc->fec_type_vc = PW_OWNER_PWID_FEC_SIGNALING;
#endif

    }
  else
    vc->fec_type_vc = fec_type_vc;

  vc->row_status = RS_ACTIVE;
  SET_FLAG (vc->pw_status, NSM_CODE_PW_NOT_FORWARDING);

  /* Set in table. */
  pal_mem_set (&p, 0, sizeof (struct prefix));
  p.family = AF_INET;
  p.prefixlen = NSM_MPLS_CIRCUIT_ID_LEN;
  PREP_FOR_NETWORK (p.u.prefix4.s_addr, vc_id, NSM_MPLS_CIRCUIT_ID_LEN);
  rn = route_node_get (NSM_MPLS->vc_table, &p);
  vc->rn = rn;
  rn->info = vc;
  /* for snmp */
  vc->create_time = nsm_get_sys_up_time();
  NSM_MPLS->notify_time_init = pal_time_current(NULL);

#ifdef HAVE_VCCV
  vc->cc_types = cc_types;
  vc->cv_types = cv_types;
#endif /* HAVE_VCCV */
#ifdef HAVE_SNMP
   vc->t_time_elpsd = NULL;
   VC_TIMER_ON(nm->zg, vc->t_time_elpsd, vc, vc_timer, 0);
  vc->admin_status = PW_SNMP_ADMIN_UP;
#endif /* HAVE_SNMP */
  vc->last_change = vc->create_time;

  /* Register in group. */
  if (group_name)
    nsm_mpls_l2_vc_group_register (nm, group_name, group_id, vc);

  /* Set end-point. */
  NSM_ASSERT (egress != NULL);
  vc->address.family = AF_INET;
  vc->address.prefixlen = IPV4_MAX_PREFIXLEN;
  IPV4_ADDR_COPY (&vc->address.u.prefix4, egress);

  /* Set the flag indicating that vc has been configured physically */
  SET_FLAG (vc->flags, NSM_MPLS_VC_FLAG_CONFIGURED);

#ifdef HAVE_MS_PW
  /* check if there are any MSPW instance that need to be activated
   * as a result of this VC addition.
   */
  if (vc->ms_pw)
    {
      ret = nsm_mpls_ms_pw_stitch_send (nm, vc->ms_pw, NSM_TRUE);
      if (ret < 0)
        zlog_err (nzg, "%s-%d: Unable to send pw stich command\n",
                  __FUNCTION__, __LINE__);

      return ret;
    }
#endif /* HAVE_MS_PW */

  /* Check if some interface wants to use this virtual circuit. */
  LIST_LOOP (NSM_MPLS->iflist, mif, ln)
    {
      LIST_LOOP (mif->vc_info_list, vc_info, node)
        {
          if ((! CHECK_FLAG (vc_info->if_bind_type, NSM_IF_BIND_MPLS_VC)) &&
              (! CHECK_FLAG (vc_info->if_bind_type, NSM_IF_BIND_MPLS_VC_VLAN)))
            continue;

          if ((vc_info->vc_name)
              && (pal_strcmp (vc_info->vc_name, vc->name) == 0))
            {
              /* Install vc data. */
              NSM_ASSERT (vc_info->u.vc == NULL);
              vc->vc_info = vc_info;
              vc_info->u.vc = vc;
              if (vc_info->run_mode == NSM_MPLS_VC_STANDBY)
                SET_FLAG (vc->pw_status, NSM_CODE_PW_STANDBY);

              vc->state = NSM_MPLS_L2_CIRCUIT_ACTIVE;

              nsm_mpls_l2_circuit_if_bind (mif->ifp, vc_info->vlan_id,
                  vc->fec_type_vc);
              nsm_mpls_if_install_vc_data (mif, vc, vc_info->vlan_id);
              found = 1;
              break;
            }
          if (found)
            break;
        }
    }

#ifdef HAVE_VPLS
    /* Check if needs to be added to any vpls instance */
    if (! vc->vc_info)
      {
        struct nsm_vpls *vpls;
        struct ptree_node *pn;
        struct nsm_vpls_spoke_vc *svc;

        for (pn = ptree_top (NSM_MPLS->vpls_table); pn; pn = ptree_next (pn))
          {
            if (! pn->info)
              continue;

            vpls = pn->info;
            svc = nsm_vpls_spoke_vc_lookup_by_name (vpls, vc->name,
                NSM_FALSE);

            if (svc)
              {
                NSM_ASSERT (svc->vc == NULL);
                svc->vc = vc;
                vc->vpls = vpls;
                vc->state = NSM_MPLS_L2_CIRCUIT_ACTIVE;
                ptree_unlock_node (pn);
                nsm_vpls_activate (vpls);

                if (vpls->state == NSM_VPLS_STATE_ACTIVE)
                  {
                    nsm_vpls_add_send (vpls, NSM_VPLS_ATTR_FLAG_ALL);
                    nsm_vpls_spoke_vc_add_send (svc->vc, svc->vc_type);
                  }
                break;
              }
          }
      }
#endif /* HAVE_VPLS */

  return NSM_SUCCESS;
}

static int
nsm_mpls_l2_circuit_update (struct nsm_master *nm, char *vc_name,
                            u_int32_t vc_id, struct pal_in4_addr *egress,
                            char *group_name, u_int32_t group_id, u_char c_word,
                            u_char mapping_type, char *tunnel_info,
                            u_char tunnel_dir, u_int8_t fec_type_vc,
#ifdef HAVE_VCCV
                            u_int8_t cc_types, u_int8_t cv_types,
#endif /* HAVE_VCCV */
                            struct nsm_mpls_circuit *vc)
{
    bool_t send_update = NSM_FALSE;
    bool_t mapping_update = NSM_FALSE;
    struct fec_gen_entry gen_entry;

    NSM_MPLS_PREFIX_TO_FEC_GEN_ENTRY(&gen_entry, &vc->address);

    if (vc->cw != c_word)
      {
        send_update = NSM_TRUE;
        vc->cw = c_word;
      }

    if (vc->mapping_type != mapping_type ||
        vc->tunnel_dir != tunnel_dir)
      {
        if (vc->state >= NSM_MPLS_L2_CIRCUIT_ACTIVE &&
            FLAG_ISSET (vc->flags, NSM_MPLS_VC_FLAG_DEP))
          {
            gmpls_lsp_dep_del (nm, &gen_entry, CONFIRM_VC, vc);
            UNSET_FLAG (vc->flags, NSM_MPLS_VC_FLAG_DEP);
            vc->state = NSM_MPLS_L2_CIRCUIT_ACTIVE;
          } 

        vc->mapping_type = mapping_type;
        vc->tunnel_dir = tunnel_dir;
        mapping_update = NSM_TRUE;
      }

    if (mapping_type == MPLS_VC_MAPPING_NONE)
      {
        if (vc->tunnel_name)
          {
            XFREE (MTYPE_TMP, vc->tunnel_name);
            vc->tunnel_name = NULL;
          }
        vc->tunnel_id = 0;
      }
    else if (mapping_type == MPLS_VC_MAPPING_TUNNEL_NAME)
      {
        if (vc->tunnel_name == NULL)
          {
            vc->tunnel_name = XSTRDUP (MTYPE_TMP, tunnel_info);
            mapping_update = NSM_TRUE;
          }
        else if (pal_strcmp (vc->tunnel_name, tunnel_info) != 0)
          {
            if (mapping_update == NSM_FALSE)
              {
                if (vc->state >= NSM_MPLS_L2_CIRCUIT_ACTIVE &&
                    FLAG_ISSET (vc->flags, NSM_MPLS_VC_FLAG_DEP))
                  {
                    gmpls_lsp_dep_del (nm, &gen_entry, CONFIRM_VC, vc);
                                   UNSET_FLAG (vc->flags, NSM_MPLS_VC_FLAG_DEP);
                    vc->state = NSM_MPLS_L2_CIRCUIT_ACTIVE;
                  } 
              }

            XFREE (MTYPE_TMP, vc->tunnel_name);
            vc->tunnel_name = XSTRDUP (MTYPE_TMP, tunnel_info);
            mapping_update = NSM_TRUE;
          }
        vc->tunnel_id = 0;
      }
    else 
      {
        if (vc->tunnel_name != NULL)
          {
            XFREE (MTYPE_TMP, vc->tunnel_name);
            vc->tunnel_name = NULL;
          }

        if (vc->tunnel_id != pal_strtou32(tunnel_info, NULL,10))
          {
            if (mapping_update == NSM_FALSE)
              {
                if (vc->state >= NSM_MPLS_L2_CIRCUIT_ACTIVE &&
                    FLAG_ISSET (vc->flags, NSM_MPLS_VC_FLAG_DEP))
                  {
                    gmpls_lsp_dep_del (nm, &gen_entry, CONFIRM_VC, vc);
                    UNSET_FLAG (vc->flags, NSM_MPLS_VC_FLAG_DEP);
                    vc->state = NSM_MPLS_L2_CIRCUIT_ACTIVE;
                  }
              } 

            mapping_update = NSM_TRUE;
            vc->tunnel_id = pal_strtou32 (tunnel_info, NULL, 10);
          }
      }

    if (mapping_update == NSM_TRUE)
      nsm_mpls_vc_fib_check_and_add (nm, vc); 

    /* Compare and update end-point. */
    NSM_ASSERT (egress != NULL);
    if (! IPV4_ADDR_SAME (&vc->address.u.prefix4, egress))
      {
        send_update = NSM_TRUE;
        IPV4_ADDR_COPY (&vc->address.u.prefix4, egress);
      }

    /* Compare and update group. */
    if (vc->group)
      {
        if (group_name == NULL)
          {
            send_update = NSM_TRUE;
            nsm_mpls_l2_vc_group_unregister (nm, vc->group, vc);
          }
        else if (pal_strcmp (vc->group->name, group_name) != 0) 
          {
            send_update = NSM_TRUE;
            nsm_mpls_l2_vc_group_unregister (nm, vc->group, vc);
            nsm_mpls_l2_vc_group_register (nm, group_name, group_id, vc);
          }
      }
    else
      {
        if (group_name)
          {
            send_update = NSM_TRUE;
            nsm_mpls_l2_vc_group_register (nm, group_name, group_id, vc);
          }
      }

    /* Compare and update Owner - Cannot be changed if the VC is already
       in use */
    if (vc->fec_type_vc != fec_type_vc)
      {
        /* VC should not belong to either Interface or VPLS Instance */
        if (! vc->vc_info
#ifdef HAVES_VPLS
            && ! vc->vpls
#endif /* HAVE_VPLS */
           )
          {
            vc->fec_type_vc = fec_type_vc;
            send_update = NSM_FALSE;
          }
        else

          return NSM_ERR_OWNER_MISMATCH;
      }

#ifdef HAVE_VCCV
    /* Compare and update VCCV parameters */
    if (vc->cc_types != cc_types)
      {
        vc->cc_types = cc_types;
        send_update = NSM_TRUE;
      }

    if (vc->cv_types != cv_types)
      {
        vc->cv_types = cv_types;
        send_update = NSM_TRUE;
      }
#endif /* HAVE_VCCV */

    if (send_update == NSM_TRUE)
      {
#ifdef HAVE_VPLS
        if (vc->vpls && vc->vpls->state == NSM_VPLS_STATE_ACTIVE)
          {
            struct nsm_vpls_spoke_vc *svc;

            svc = nsm_vpls_spoke_vc_lookup_by_name (vc->vpls, vc->name,
                NSM_FALSE);
            if (svc)
              nsm_vpls_spoke_vc_add_send (vc, svc->vc_type);

            return NSM_SUCCESS;
          }
#endif /* HAVE_VPLS */

        /* Send update to clients and to Forwarder. */
        if (vc->vc_info && vc->vc_info->mif && vc->vc_info->mif->ifp)
          {
            SET_FLAG (vc->pw_status, NSM_CODE_PW_NOT_FORWARDING);
            if (! if_is_running (vc->vc_info->mif->ifp))
              {
                SET_FLAG (vc->pw_status, NSM_CODE_PW_AC_INGRESS_RX_FAULT);
                SET_FLAG (vc->pw_status, NSM_CODE_PW_AC_EGRESS_TX_FAULT);
              }
            nsm_mpls_l2_circuit_add_send (vc->vc_info->mif, vc);
          }
      }

    return NSM_SUCCESS;
}

/* Add a Layer-2 circuit. May also be used for updates. */
int
nsm_mpls_l2_circuit_add (struct nsm_master *nm, char *vc_name, u_int32_t vc_id,
                         struct pal_in4_addr *egress, char *group_name,
                         u_int32_t group_id, u_char c_word, u_char mapping_type,
                         char *tunnel_info, u_char tunnel_dir,
#ifdef HAVE_VCCV
                         u_int8_t cc_types, u_int8_t cv_types,
#endif /* HAVE_VCCV */
                         u_int8_t fec_type_vc, u_int8_t is_passive)
{
  struct nsm_mpls_circuit_container *vc_container = NULL;
  struct nsm_mpls_vc_group *group = NULL;
  int ret = NSM_SUCCESS;

  if (mapping_type != MPLS_VC_MAPPING_NONE && tunnel_info == NULL)
    return NSM_ERR_INVALID_ARGS;

  if (c_word && apn_mpls_cw_capability () < 0)
    return NSM_ERR_NO_CW_CAPABILITY;

  if (group_name && group_id)
    {
      /* If there is already another group with the same group-id, 
       * return error. 
       */
      group = nsm_mpls_l2_vc_group_lookup_by_id (nm, group_id);
      if (group && (pal_strcmp (group->name, group_name) != 0))
        return NSM_ERR_GROUP_ID_EXISTS;

      /* group-id of the given group cannot be changed. */
      group = nsm_mpls_l2_vc_group_lookup_by_name (nm, group_name);
      if (group && (group_id != group->id))
        return NSM_ERR_GROUP_EXISTS; 
    }

  /* Lookup using name and id. */
  vc_container = nsm_mpls_vc_get_by_name (nm, vc_name);

  if (!vc_container)
    return NSM_ERR_MEM_ALLOC_FAILURE;

  /* If this is a new one. */
  if (! vc_container->vc)
    {
      vc_container->vc = nsm_mpls_l2_circuit_create (nm, vc_name,
                                                     vc_id, is_passive, &ret);
    }

  if (ret < 0)
    return ret;

  /* check the vc->flags if it was already configured.
   * If yes : update the vc
   * If no : fill the data
   */
  if (FLAG_ISSET (vc_container->vc->flags, NSM_MPLS_VC_FLAG_CONFIGURED))
    {
#ifdef HAVE_MS_PW
      /* If there is a mismatch in the active/passive roles, return error */
      if (vc_container->vc->ms_pw_role != is_passive)
        return NSM_ERR_MS_PW_ROLE_MISMATCH;
#endif /* HAVE_MS_PW */

      /*
       * Update.
       */
      ret = nsm_mpls_l2_circuit_update (nm, vc_name, vc_id, egress,
                                        group_name, group_id, c_word, 
                                        mapping_type, tunnel_info, 
                                        tunnel_dir, fec_type_vc,
#ifdef HAVE_VCCV
                                        cc_types, cv_types,
#endif /* HAVE_VCCV */
                                        vc_container->vc);
    }
  else
    {
#ifdef HAVE_SNMP
      ret = bitmap_request_index (NSM_MPLS->vc_indx_mgr, 
                                  &vc_container->vc->vc_snmp_index);
      if (ret < 0)
        return NSM_FAILURE;
#endif /*HAVE_SNMP*/

      ret = nsm_mpls_vc_fill_data (nm, vc_id, egress,
                                   group_name, group_id, c_word, mapping_type,
                                   tunnel_info, tunnel_dir,
                                   fec_type_vc,
#ifdef HAVE_VCCV
                                   cc_types, cv_types,
#endif /* HAVE_VCCV */
                                   vc_container->vc);
      if (ret < 0)
        {
          zlog_err (NSM_ZG, "%s-%d: vc data not filled\n",
              __FUNCTION__, __LINE__);
#ifdef HAVE_SNMP
          ret = bitmap_release_index (NSM_MPLS->vc_indx_mgr, 
                                      vc_container->vc->vc_snmp_index);
          if (ret < 0)
            return NSM_FAILURE;
#endif /*HAVE_SNMP*/
        }
    }

  return ret;
}

/**@brief Function to create a key into the MPLS circuit hash table with the
          name as the key.
    @param  *name - pointer to the name of the vc.
    @return       - the Key to the MSPW hash table.
*/
u_int32_t
nsm_mpls_vc_hash_key_make (char *name)
{
  int i, len;
  u_int32_t key;

  if (name)
    {
      key = 0;
      len = pal_strlen (name);
      for (i = 1; i <= len; i++)
        key += (name[i] * i);

      return key;
    }
  return 0;
}

/**@brief Support function for handling the MPLS vc hash table.
          This is the comparision function for the MPLS vc hash table.
    @param *vc      - pointer to the mpls vc structure.
    @param *vc_name - pointer to the name of the mpls vc name.
    @return boolean - TRUE or FALSE.
*/
bool_t
nsm_mpls_vc_hash_cmp (struct nsm_mpls_circuit_container *vc, char *vc_name)
{
  if (vc && vc->name && vc_name &&
      pal_strcmp (vc->name, vc_name) == 0)
    return NSM_TRUE;

  return NSM_FALSE;
}

/**@brief Function to initialise the NSM mpls vc hash table.
    @param *nm - pointer to the nsm master global.
    @return    - NSM_SUCCESS/NSM_FAILURE.
*/
int
nsm_mpls_vc_hash_init (struct nsm_master *nm)
{
  if (!NSM_MPLS)
    return NSM_FAILURE;

  NSM_MPLS_CIRCUIT_HASH = hash_create(nsm_mpls_vc_hash_key_make,
                                      nsm_mpls_vc_hash_cmp);
  if (! NSM_MPLS_CIRCUIT_HASH)
    return NSM_FAILURE;

  return NSM_SUCCESS;
}

/**@brief Function to allocate function for NSM mpls vc hash table.
    @param *name  - pointer to the name of the MPLS vc.
    @return *void - pointer to the mpls vc that has been allocated.
*/
void *
nsm_mpls_vc_hash_alloc (char *name)
{
  struct nsm_mpls_circuit_container *vc_container = NULL;

  vc_container = XCALLOC (MTYPE_NSM_VC_CONTAINER, 
                          sizeof(struct nsm_mpls_circuit_container));
  if (!vc_container)
    return NULL;

  return vc_container;
}

/**@brief Function to remove an entry from NSM mpls vc Hash table.
    @param *nm    - pointer to the NSM master global
    @param *name  - name of the vc_container that needs to be removed
                    from the NSM mpls vc global hash table.
    @return void.
*/
void
nsm_mpls_vc_remove_from_hash (struct nsm_master *nm,
                              char *vc_container_name)
{
  if ((!nm) || (!NSM_MPLS) || (!NSM_MPLS_CIRCUIT_HASH))
    return;

  hash_release (NSM_MPLS_CIRCUIT_HASH, vc_container_name);
}

/**@brief Function to look up vc hash table with vc name as key
    @param *nm    - pointer to the NSM master global
    @param *name  - pointer to the name of the vc.
    return *vc    - pointer to the nsm_mpls_vc struct, if it exists
                    OR NULL;
*/
struct nsm_mpls_circuit_container *
nsm_mpls_vc_lookup_by_name (struct nsm_master* nm, char *name)
{
  if ((!NSM_MPLS) || (!name) || (!NSM_MPLS_CIRCUIT_HASH))
    return NULL;

  return hash_lookup (NSM_MPLS_CIRCUIT_HASH, name);
}

/**@brief Function to get a mpls circuit instance. When none found, create
          a new one.
    @param *nm     - pointer to the NSM master global
    @param *name   - pointer to the name of the mpls vc instance.
    @param *status - pointer to the status of the get operation.
    return *vc     - pointer to the mpls vc struct, if it exists OR NULL;
*/
struct nsm_mpls_circuit_container *
nsm_mpls_vc_get_by_name (struct nsm_master* nm,
                         char *name)
{
  struct nsm_mpls_circuit_container *vc_container = NULL;

  if ((!NSM_MPLS) || (!name) || (!NSM_MPLS_CIRCUIT_HASH))
    return NULL;

  /* GET from hash. */
  vc_container = hash_get (NSM_MPLS_CIRCUIT_HASH, name, 
                           nsm_mpls_vc_hash_alloc);
  if (! vc_container)
    {
      zlog_warn (NSM_ZG, "GET for vc with name %s failed", name);
      return NULL;
    }

  /* Set VC name */
  if (!vc_container->name)
    vc_container->name = XSTRDUP (MTYPE_TMP, name);
  
  if (! vc_container->name)
    {
      nsm_mpls_vc_remove_from_hash (nm, name); 
      XFREE (MTYPE_NSM_VC_CONTAINER, vc_container);

      return NULL;
    }

  return vc_container;
}

int
nsm_mpls_ac_group_create (struct nsm_master *nm, char *name,
                          u_int32_t group_id)
{
  int ret = NSM_FALSE;
  struct nsm_mpls_vc_group *vc_group = NULL;

  vc_group = nsm_mpls_l2_vc_group_lookup_by_name (nm, name);
  if (vc_group)
    {
     if (vc_group->id == group_id)
       ret = NSM_TRUE;
    }
  else
    {
      if (! nsm_mpls_l2_vc_group_lookup_by_id (nm, group_id))
        {
          if (nsm_mpls_l2_vc_group_create (nm, name, group_id))
            {
              ret = NSM_TRUE;
            }
        }
    } 

  return ret;
}

#if 0
int
nsm_mpls_l2_circuit_add (struct nsm_master *nm, char *vc_name, u_int32_t vc_id,
                         struct pal_in4_addr *egress, char *group_name,
                         u_char c_word, char * mapping, char *tunnel_info,
                         u_int8_t fec_type_vc)
{
  struct nsm_mpls_circuit *vc, *vc2;
  struct nsm_mpls_vc_info *vc_info = NULL;
  bool_t send_update = NSM_FALSE;

  if (c_word)
    {
      if (apn_mpls_cw_capability () < 0)
        return NSM_ERR_NO_CW_CAPABILITY;
    }

  /* Lookup using name and id. */
  vc = nsm_mpls_l2_circuit_lookup_by_name (nm, vc_name);
  vc2 = nsm_mpls_l2_circuit_lookup_by_id (nm, vc_id);

  /* Make sure neither of these are in use or something. */
  if (vc != vc2)
    return NSM_ERR_VC_NAME_ID_MISMATCH;

  /* If this is a new one. */
  if (! vc)
    {
      struct nsm_mpls_if *mif;
      struct listnode *ln, *node;
      int found = 0;

#ifdef HAVE_VPLS
      {
        struct nsm_vpls *vpls;
        
        vpls = nsm_vpls_lookup (nm, vc_id, NSM_FALSE);
        if (vpls)
          return NSM_ERR_INVALID_VC_ID;
      }
#endif /* HAVE_VPLS */

      /* Get new Virtual Circuit object. */
      vc = nsm_mpls_l2_circuit_new (nm, vc_name, vc_id, c_word, mapping,
                                      tunnel_info, fec_type_vc);
      vc->t_time_elpsd = 0;
#ifdef HAVE_SNMP
      VC_TIMER_ON(nzg, vc->t_time_elpsd, vc, vc_timer, VC_TIMER_15MIN);
#endif
      vc->last_change = vc->create_time;

      if (! vc)
        return NSM_ERR_MEM_ALLOC_FAILURE;

      if (group_name)
        /* Register in group. */
        nsm_mpls_l2_vc_group_register (nm, group_name, vc);

      /* Set end-point. */
      NSM_ASSERT (egress != NULL);
      vc->address.family = AF_INET;
      vc->address.prefixlen = IPV4_MAX_PREFIXLEN;
      IPV4_ADDR_COPY (&vc->address.u.prefix4, egress);

      /* Check if some interface wants to use this virtual circuit. */
      mif = NULL;
      LIST_LOOP (NSM_MPLS->iflist, mif, ln)
        {
          LIST_LOOP (mif->vc_info_list, vc_info, node)
            {
              if ((! CHECK_FLAG (vc_info->if_bind_type, NSM_IF_BIND_MPLS_VC)) &&
                  (! CHECK_FLAG (vc_info->if_bind_type, 
                                 NSM_IF_BIND_MPLS_VC_VLAN)))
                continue;

              if ((vc_info->vc_name) 
                  && (pal_strcmp (vc_info->vc_name, vc_name) == 0))
                {
                  /* Install vc data. */
                  NSM_ASSERT (vc_info->u.vc == NULL);
                  vc->vc_info = vc_info;
                  vc_info->u.vc = vc;
                  nsm_mpls_l2_circuit_if_bind (mif->ifp, vc_info->vlan_id,
                                               vc->fec_type_vc);
                  nsm_mpls_if_install_vc_data (mif, vc, vc_info->vlan_id);
                  found = 1;
                  break;
                }
              if (found)
                break;
            }
        }     
      
#ifdef HAVE_VPLS
      /* Check if needs to be added to any vpls instance */
      if (! vc->vc_info)
        {
          struct nsm_vpls *vpls;
          struct ptree_node *pn;
          struct nsm_vpls_spoke_vc *svc;

          for (pn = ptree_top (NSM_MPLS->vpls_table); pn; pn = ptree_next (pn))
            {
              if (! pn->info)
                continue;
              vpls = pn->info;

              svc = nsm_vpls_spoke_vc_lookup_by_name (vpls, vc->name, 
                                                      NSM_FALSE);
              if (svc)
                {
                  NSM_ASSERT (svc->vc == NULL);
                  svc->vc = vc;
                  vc->vpls = vpls;
                  ptree_unlock_node (pn);
                  nsm_vpls_activate (vpls);

                  if (vpls->state == NSM_VPLS_STATE_ACTIVE)
                    {
                      nsm_vpls_add_send (vpls, NSM_VPLS_ATTR_FLAG_ALL);
                      nsm_vpls_spoke_vc_add_send (svc->vc, svc->vc_type);
                    }

                  break;
                }
            }
        }
#endif /* HAVE_VPLS */

      return NSM_SUCCESS;
    }

  /*
   * Update.
   */
 
  if (vc->cw != c_word)
    {
      send_update = NSM_TRUE;
      vc->cw = c_word;
    }
  
  if (mapping)
    {  
      if (mapping[7] == 'n') 
        { 
          if ((!vc->tunnel_name ) || (pal_strcmp (vc->tunnel_name, tunnel_info ) != 0))
            {
              send_update = NSM_TRUE;
              vc->tunnel_name = XSTRDUP (MTYPE_TMP, tunnel_info); 
            }
        }
      else if (!vc->tunnel_id || (vc->tunnel_id != 
              (pal_strtou32(tunnel_info, NULL,10))))
        {
          send_update = NSM_TRUE;
          vc->tunnel_id = pal_strtou32 (tunnel_info, NULL, 10);
        }
    } 
  else if (vc->tunnel_name || vc->tunnel_id > 0)
    {
      vc->tunnel_name = NULL;
      vc->tunnel_id = 0;
    }
   
  /* Compare and update end-point. */
  NSM_ASSERT (egress != NULL);
  if (! IPV4_ADDR_SAME (&vc->address.u.prefix4, egress))
    {
      send_update = NSM_TRUE;
      IPV4_ADDR_COPY (&vc->address.u.prefix4, egress);
    }

  /* Compare and update group. */
  if (vc->group)
    {
      if (group_name == NULL)
        {
          send_update = NSM_TRUE;
          nsm_mpls_l2_vc_group_unregister (nm, vc->group, vc);
        }
      else if (pal_strcmp (vc->group->name, group_name) != 0)
        {
          send_update = NSM_TRUE;
          nsm_mpls_l2_vc_group_unregister (nm, vc->group, vc);
          nsm_mpls_l2_vc_group_register (nm, group_name, vc);
        }
    }
  else
    {
      if (group_name)
        {
          send_update = NSM_TRUE;
          nsm_mpls_l2_vc_group_register (nm, group_name, vc);
        }
    }
 
    /* Compare and update Owner - Cannot be changed if the VC is already in use */
    if (vc->fec_type_vc != fec_type_vc)
      {
        /* VC should not belong to either Interface or VPLS Instance */
        if (! vc->vc_info 
#ifdef HAVES_VPLS
            && ! vc->vpls
#endif /* HAVE_VPLS */
            )
          {
            vc->fec_type_vc = fec_type_vc;
            send_update = NSM_FALSE;
          }
        else

         return NSM_ERR_OWNER_MISMATCH;
      }


  if (send_update == NSM_TRUE)
    {
#ifdef HAVE_VPLS
      if (vc->vpls && vc->vpls->state == NSM_VPLS_STATE_ACTIVE)
        {
          struct nsm_vpls_spoke_vc *svc;
 
          svc = nsm_vpls_spoke_vc_lookup_by_name (vc->vpls, vc->name,
                                                  NSM_FALSE);
          if (svc)
            nsm_vpls_spoke_vc_add_send (vc, svc->vc_type);
          return NSM_SUCCESS;
        }
#endif /* HAVE_VPLS */

      /* Send update to clients and to Forwarder. */
      if (vc->vc_info && vc->vc_info->mif && vc->vc_info->mif->ifp && 
          if_is_up (vc->vc_info->mif->ifp))
        /* Set update to overlying protocols. */
        nsm_mpls_l2_circuit_add_send (vc->vc_info->mif, vc);
    }

  return NSM_SUCCESS;
}
#endif /* if 0 */

/* Withdraw VC configuration from above and below. */
void
nsm_mpls_if_withdraw_vc_data (struct nsm_mpls_if *mif,
                              struct nsm_mpls_circuit *vc, bool_t send_msg)
{
  struct if_ident ident;
  u_int16_t vlan_id = 0;
  int ret;

  /* If there is a real virtual circuit. */
  if (vc && mif->ifp)
    {
      /* Clean up FTN entry for this VC before unbind from 
         MPLS Forwarder */
      if (vc->vc_info)
        vlan_id = vc->vc_info->vlan_id;

      if (send_msg == NSM_TRUE)
      /* Send delete to overlying protocols. */
      nsm_mpls_l2_circuit_del_send (mif, vc);
          
      if (!mif->nls)
        {
          pal_mem_set (&ident, 0, sizeof (struct if_ident));
          ident.if_index = mif->ifp->ifindex;
          pal_mem_cpy (ident.if_name, mif->ifp->name, INTERFACE_NAMSIZ);
          ret = apn_mpls_if_end (&ident);
          if (ret < 0)
            zlog_warn (NSM_ZG, "Could not enable interface %s with index %d "
                       "for MPLS", mif->ifp->name, mif->ifp->ifindex);
          else if (IS_NSM_DEBUG_EVENT)
            zlog_info (NSM_ZG, "Enabled interface %s with index %d for MPLS",
                       mif->ifp->name, mif->ifp->ifindex);
        }
    }
}

int
nsm_mpls_vc_activate (struct nsm_mpls_circuit *vc)
{
  int ret = NSM_SUCCESS;

  if (! vc)
    return NSM_FAILURE;

  if (vc->vc_info && vc->vc_info->mif &&
      vc->vc_info->mif->ifp)
    {
#ifdef HAVE_VLAN
      if (vc->vc_info->vlan_id != 0)
        ret = nsm_is_vlan_exist (vc->vc_info->mif->ifp, vc->vc_info->vlan_id);
#endif /* HAVE_VLAN */
        if (ret == NSM_SUCCESS)
        {
          vc->state = NSM_MPLS_L2_CIRCUIT_ACTIVE;

          /* send l2-circuit interface binding msg to ldp */
          nsm_mpls_l2_circuit_if_bind (vc->vc_info->mif->ifp,
                                       vc->vc_info->vlan_id, vc->fec_type_vc);

          ret = nsm_mpls_if_install_vc_data (vc->vc_info->mif, vc,
                                             vc->vc_info->vlan_id);
          if (ret < 0)
            {
              return ret;
            }

          nsm_mpls_vc_fib_check_and_add (vc->vc_info->mif->nm, vc);

          return NSM_SUCCESS;
        }
    }

  vc->state = NSM_MPLS_L2_CIRCUIT_DOWN;

  return NSM_SUCCESS;
}

void
nsm_mpls_vc_deactivate (struct nsm_mpls_circuit *vc,
                        int send_msg, int del_vc_fib)
{
  if (vc)
    {
      vc->pw_status = 0;
      SET_FLAG (vc->pw_status, NSM_CODE_PW_NOT_FORWARDING);
    }

  if (vc && vc->vc_info && vc->vc_info->mif)
    {

      if (vc->vc_fib)
        nsm_mpls_vc_fib_cleanup (vc->vc_info->mif->nm, vc, del_vc_fib);

      if (vc->state >= NSM_MPLS_L2_CIRCUIT_ACTIVE)
        {
          nsm_mpls_if_withdraw_vc_data (vc->vc_info->mif, vc,
                                        send_msg);
          vc->state = NSM_MPLS_L2_CIRCUIT_DOWN;

          /* Unbind the interface from the L2 circuit.  */
          nsm_mpls_l2_circuit_if_unbind (vc->vc_info->mif,
                                         vc->vc_info->vlan_id,
                                         vc->fec_type_vc);
       }
    }
}

/* Always add to tail. */
void
nsm_mpls_vc_info_add (struct nsm_mpls_if *mif, struct list *vlist,
                      struct nsm_mpls_vc_info *v_info)
{
  listnode_add (vlist, v_info);
  v_info->mif = mif;
}

void
nsm_mpls_vc_info_del (struct nsm_mpls_if *mif, struct list *vlist,
                      struct nsm_mpls_vc_info *v_info)
{
  if (v_info == NULL)
    return;

  listnode_delete (vlist, v_info); 
  v_info->mif = NULL;
}

struct nsm_mpls_vc_info *
nsm_mpls_vc_info_create (char *vc_name, struct nsm_mpls_if *mif, 
                         u_int16_t vc_type, u_int16_t vlan_id, 
                         u_int16_t vc_mode, u_char if_bind_type)
{
  struct nsm_mpls_vc_info *vc_info = NULL;
  u_int16_t hw_type, vtype = VC_TYPE_UNKNOWN;
  struct interface *ifp;

  ifp = mif->ifp;
  if (!ifp)
    return NULL;
  
  vtype = vc_type;

  /* If vc_type is UNKNOWN, figure it out based on the interface. */
  if (vc_type != VC_TYPE_UNKNOWN)
    vtype = vc_type;
  else if (vlan_id == 0)
    {
      /* Guess interface type.  */
      hw_type = if_get_hw_type (ifp);
      if (hw_type == IF_TYPE_ETHERNET)
        vtype = VC_TYPE_ETHERNET;
      else if (if_is_pointopoint (ifp))
        vtype = VC_TYPE_PPP;
      else if (hw_type == IF_TYPE_HDLC)
        vtype = VC_TYPE_HDLC;
      else
        vtype = VC_TYPE_UNKNOWN;
    }
  else
    vtype = VC_TYPE_ETH_VLAN;

  vc_info = XCALLOC (MTYPE_TMP, sizeof (struct nsm_mpls_vc_info));
  if (! vc_info)
    return NULL;

  /* Set vc_name. */
  if (vc_name)
    {
      vc_info->vc_name = XSTRDUP (MTYPE_TMP, vc_name);
      if (! vc_info->vc_name)
        {
          XFREE (MTYPE_TMP, vc_info);
          return NULL;
        }
    }

#ifdef HAVE_SNMP
  /* By default set the value for this object to 1. */
  vc_info->pw_enet_pw_instance = 1;
#endif /* HAVE_SNMP */

  vc_info->mif = NULL;
  /* Set vc_type. */
  vc_info->vc_type = vtype;
  /* Set vlan_id in vc_info. */
  vc_info->vlan_id = vlan_id;
  /* Set vc_info bind_type */
  SET_FLAG (vc_info->if_bind_type, if_bind_type);
  /* Set the vc_mode */
  vc_info->vc_mode = vc_mode;
  /* Set flags for secondary and primary links */
  if (vc_mode == NSM_MPLS_VC_CONF_SECONDARY)
    {
      SET_FLAG (vc_info->conf_flag, NSM_MPLS_VC_CONF_SECONDARY);
      vc_info->run_mode = NSM_MPLS_VC_STANDBY;
    }

  /* Add vc_info to mif->vc_info_list. */
  if (if_bind_type == NSM_IF_BIND_MPLS_VC || 
      if_bind_type == NSM_IF_BIND_MPLS_VC_VLAN)
    nsm_mpls_vc_info_add (mif, mif->vc_info_list, vc_info);
#ifdef HAVE_VPLS
  else
    nsm_mpls_vc_info_add (mif, mif->vpls_info_list, vc_info);
#endif /* HAVE_VPLS */

  return vc_info;
}

void 
nsm_mpls_vc_info_free (struct nsm_mpls_vc_info *v_info) 
{
  /* unlink sibling if there is any */
  if (v_info->sibling_info)
    {
      v_info->sibling_info->sibling_info = NULL;
      v_info->sibling_info = NULL;
    }

  /* Clean up interface vc name. */
  if (v_info->vc_name)
    XFREE (MTYPE_TMP, v_info->vc_name);
  v_info->vc_name = NULL;

  if (CHECK_FLAG (v_info->if_bind_type, NSM_IF_BIND_MPLS_VC) ||
      CHECK_FLAG (v_info->if_bind_type, NSM_IF_BIND_MPLS_VC_VLAN))
    {
      if (v_info->u.vc)
        {
          v_info->u.vc->vc_info = NULL;
          v_info->u.vc = NULL;
        }
    }
#ifdef HAVE_VPLS
    else if (CHECK_FLAG (v_info->if_bind_type, NSM_IF_BIND_VPLS) ||
             CHECK_FLAG (v_info->if_bind_type, NSM_IF_BIND_VPLS_VLAN))
      {
        if (v_info->u.vpls)
          {
            v_info->u.vpls = NULL;
          }
      }
#endif /* HAVE_VPLS */

  v_info->if_bind_type = 0;
  XFREE (MTYPE_TMP, v_info);
  v_info = NULL;
}

/* Only VC_TYPE_ETH_VLAN type VCs are allowed bind to one interface with 
   different vlan_id. If found sibling, assign to vc_info_sibling. */
int
nsm_mpls_l2_circuit_bind_check (struct nsm_mpls_if *mif, u_int16_t vc_type, 
		                char *vc_name, u_int16_t vlan_id, 
				u_int16_t vc_mode, u_char bind_type, 
				struct nsm_mpls_vc_info **vc_info_sibling)
{
  struct nsm_mpls_vc_info *vc_info = NULL;
  struct listnode *ln;
  struct list *list;

  if (LISTHEAD (mif->vc_info_list))
    {
      vc_info = GETDATA (LISTHEAD (mif->vc_info_list));
      if (vc_info && vc_info->vc_type != vc_type)
        return NSM_ERR_VC_TYPE_MISMATCH;
    }

#ifdef HAVE_VPLS
  /* VPLS interface binding do not support secondary mode */
  if (vc_mode == NSM_MPLS_VC_CONF_SECONDARY && 
      (bind_type == NSM_IF_BIND_VPLS_VLAN || bind_type == NSM_IF_BIND_VPLS))
    return NSM_ERR_IF_BINDING_EXISTS;

  if (LISTHEAD (mif->vpls_info_list))
    {
      vc_info = GETDATA (LISTHEAD (mif->vpls_info_list));
      if (vc_info && vc_info->vc_type != vc_type)
        return NSM_ERR_VC_TYPE_MISMATCH;
    }
#endif /* HAVE_VPLS */

  /* Only VC_TYPE_ETH_VLAN type VCs are allowed bind to 
     one interface with different vlan_id. */
  list = mif->vc_info_list;
  while (list)
    {
      LIST_LOOP (list, vc_info, ln)
        {
          /* Different vlan_id */
          if (vc_info->vlan_id != vlan_id)
            {
              if (pal_strcmp (vc_info->vc_name, vc_name) == 0)
                return NSM_ERR_VC_IN_USE;

              continue;
            }
     
          /* Same vlan_id, same vc_name */ 
          if (pal_strcmp (vc_info->vc_name, vc_name) == 0)
            {
              if ((vc_mode == NSM_MPLS_VC_CONF_SECONDARY &&
                   ! CHECK_FLAG (vc_info->conf_flag, 
                                 NSM_MPLS_VC_CONF_SECONDARY)) ||
                  (vc_mode == 0 && 
                   CHECK_FLAG (vc_info->conf_flag, NSM_MPLS_VC_CONF_SECONDARY)))
                return NSM_ERR_VC_MODE_MISMATCH;

              if (! CHECK_FLAG (vc_info->if_bind_type, bind_type)) 
                return NSM_ERR_VC_BINDING_EXISTS;
 
              return NSM_SUCCESS;
            }
      
          /* Same vlan_id, different vc_name, for one port (port+vlan)
             only allow two sibling, also secondary/secondary is not allowed */
          if ((vc_mode == NSM_MPLS_VC_CONF_SECONDARY &&
               CHECK_FLAG (vc_info->conf_flag, NSM_MPLS_VC_CONF_SECONDARY))||
               vc_info->sibling_info)
            return NSM_ERR_VPN_EXISTS_ON_IF_VLAN;
       
          /* Only one vc_info_sibling could be found,
             also, VPLS binding will not have sibling. */ 
          if (bind_type == NSM_IF_BIND_MPLS_VC_VLAN || 
              bind_type == NSM_IF_BIND_MPLS_VC)
            *vc_info_sibling = vc_info;
        }
#ifdef HAVE_VPLS
      if (list == mif->vc_info_list)
        list = mif->vpls_info_list;
      else
#endif /*HAVE_VPLS*/
        list = NULL;

    }

  return NSM_SUCCESS;
}

/* Unbind interface from l2 circuit. */
int
nsm_mpls_l2_circuit_unbind (struct nsm_mpls_if *mif, char *vc_name,
                            u_int16_t vlan_id)
{
  struct nsm_mpls_vc_info *vc_info = NULL;
  struct listnode *ln;
  int found = 0;

  /* Find VC configured on this interface. */
  LIST_LOOP (mif->vc_info_list, vc_info, ln)
    {
      if ((! CHECK_FLAG (vc_info->if_bind_type,  NSM_IF_BIND_MPLS_VC)) &&
          (! CHECK_FLAG (vc_info->if_bind_type, NSM_IF_BIND_MPLS_VC_VLAN)))
        continue;

      if (vc_info->vc_name
          && pal_strcmp (vc_info->vc_name, vc_name) == 0)
        {
          if (vc_info->vlan_id != vlan_id)
            return NSM_ERR_VC_ID_NOT_FOUND;
          
          found = 1;
          break;
        }
    }

  if (found == 0)
    return NSM_ERR_MIF_VC_MISMATCH;

  if (LISTCOUNT (mif->vc_info_list) == 1)
    {
      if (vlan_id != 0)
        UNSET_FLAG (mif->ifp->bind, NSM_IF_BIND_MPLS_VC_VLAN); 
      else
        UNSET_FLAG (mif->ifp->bind, NSM_IF_BIND_MPLS_VC); 
    }

  /* If there is a real virtual circuit. */
  if (vc_info->u.vc)
    { 
      nsm_mpls_vc_deactivate (vc_info->u.vc, NSM_TRUE, NSM_TRUE);
    }
  
  /* Delete vc_info from vc_info_list. */
  nsm_mpls_vc_info_del (mif, mif->vc_info_list, vc_info);
  nsm_mpls_vc_info_free (vc_info);

  return NSM_SUCCESS;
}

/* Bind an interface to a virtual circuit. */
int
nsm_mpls_l2_circuit_bind (struct nsm_mpls_if *mif, char *vc_name,
                          u_int16_t vc_type, u_int16_t vlan_id,
                          u_int16_t vc_mode)
{
  struct nsm_master *nm = mif->nm;
  struct nsm_mpls_circuit *vc = NULL;
  struct nsm_mpls_circuit_container *vc_container = NULL;
  struct nsm_mpls_circuit *s_vc = NULL;
  struct nsm_mpls_vc_info *vc_info = NULL, *s_info = NULL;
  struct listnode *ln;
  struct nsm_mpls_if *mif_tmp;
  struct interface *ifp;
  int ret = NSM_SUCCESS;

  ifp = mif->ifp;
  if (! ifp)
    {
      zlog_err (NSM_ZG, "Internal Error: Interface object mismatch while "
                "binding a Layer-2 MPLS Virtual Circuit to an interface");
      return NSM_ERR_NOT_FOUND;
    }
  
  /* Only VC_TYPE_ETH_VLAN type VCs are allowed bind to 
     one interface with different vlan_id. */
  ret = nsm_mpls_l2_circuit_bind_check (mif, vc_type, vc_name, vlan_id, vc_mode,
                                        vlan_id ? NSM_IF_BIND_MPLS_VC_VLAN :
                                                  NSM_IF_BIND_MPLS_VC,
                                        &s_info);
  if (ret != NSM_SUCCESS)
    return ret;

  /* secondary/secondary configuration is not allowed */
  if (s_info && 
      CHECK_FLAG (s_info->conf_flag, NSM_MPLS_VC_CONF_STANDBY) &&
      vc_mode == NSM_MPLS_VC_CONF_SECONDARY)
    return NSM_ERR_VC_MODE_MISMATCH;

  /* secondary/secondary configuration is not allowed */
  if (s_info && 
      ! CHECK_FLAG (s_info->conf_flag, NSM_MPLS_VC_CONF_SECONDARY) &&
      vc_mode != NSM_MPLS_VC_CONF_SECONDARY &&
      CHECK_FLAG (s_info->conf_flag, NSM_MPLS_VC_CONF_REVERTIVE))
    return NSM_ERR_VC_MODE_MISMATCH_REV;

  /* Lookup vc. */
  vc_container = nsm_mpls_vc_lookup_by_name (nm, vc_name);
  if (vc_container && vc_container->vc)
    {
      vc = vc_container->vc;
      /* Is this vc in use by some other interface ?? */
      if (vc->vc_info 
#ifdef HAVE_SNMP
          || vc->vc_info_temp)
#endif /* HAVE_SNMP */
        return NSM_ERR_VC_IN_USE;

#ifdef HAVE_MS_PW
      if (vc->ms_pw)
        return NSM_ERR_VC_IN_USE;
#endif /* HAVE_MS_PW */

#ifdef HAVE_VPLS
      if (vc->vpls)
        return NSM_ERR_VC_IN_USE;
#endif /* HAVE_VPLS */
    }
  else
    {
#ifdef HAVE_VPLS
      struct nsm_vpls *vpls;
      struct nsm_vpls_spoke_vc *svc;
      struct ptree_node *pn;
#endif /* HAVE_VPLS */
      struct listnode *node;

      /* Check if some other interface has configured this vc. */
      LIST_LOOP (NSM_MPLS->iflist, mif_tmp, ln)
        {
          vc_info = NULL;
          if (mif_tmp != mif)
            LIST_LOOP (mif_tmp->vc_info_list, vc_info, node) 
              if (vc_info->vc_name 
                  && pal_strcmp (vc_info->vc_name, vc_name) == 0)
                return NSM_ERR_VC_IN_USE;
        }

#ifdef HAVE_VPLS
      /* Check if some vpls has configured this vc as spoke */
      for (pn = ptree_top (NSM_MPLS->vpls_table); pn; pn = ptree_next (pn))
        {
          if (! pn->info)
            continue;

          vpls = pn->info;
          svc = nsm_vpls_spoke_vc_lookup_by_name (vpls, vc_name, NSM_FALSE);
          if (svc)
            {
              ptree_unlock_node (pn);
              return NSM_ERR_VC_IN_USE;
            }
        }
#endif /* HAVE_VPLS */
    } 
  
  /* Create vc_info. */
  vc_info = nsm_mpls_vc_info_create (vc_name, mif, vc_type, vlan_id, vc_mode, 
                                     vlan_id ? 
                                     NSM_IF_BIND_MPLS_VC_VLAN : 
                                     NSM_IF_BIND_MPLS_VC);
  if (! vc_info)
    return NSM_ERR_MEM_ALLOC_FAILURE;

  if (vlan_id == 0)
    SET_FLAG (ifp->bind, NSM_IF_BIND_MPLS_VC);
  else
    SET_FLAG (ifp->bind, NSM_IF_BIND_MPLS_VC_VLAN);

  /* Link sibling */
  if (s_info)
    {
      vc_info->sibling_info = s_info;
      s_info->sibling_info = vc_info;
      s_vc = s_info->u.vc;

      if (CHECK_FLAG (s_info->conf_flag, NSM_MPLS_VC_CONF_STANDBY))
        {
          SET_FLAG (vc_info->conf_flag, NSM_MPLS_VC_CONF_STANDBY);
          vc_info->run_mode = NSM_MPLS_VC_STANDBY;
        }
      if (CHECK_FLAG (s_info->conf_flag, NSM_MPLS_VC_CONF_REVERTIVE))
        SET_FLAG (vc_info->conf_flag, NSM_MPLS_VC_CONF_REVERTIVE);
    }

  /* If secondary VC already installed, if non-revertive mode, 
     signal run_mode as standby */
  if (! CHECK_FLAG (vc_info->conf_flag, NSM_MPLS_VC_CONF_SECONDARY) &&
      s_vc && CHECK_FLAG (s_info->conf_flag, NSM_MPLS_VC_CONF_SECONDARY))
    {
      if (s_vc && s_vc->vc_fib && s_vc->vc_fib->install_flag == NSM_TRUE &&
          ! CHECK_FLAG (vc_info->conf_flag, NSM_MPLS_VC_CONF_REVERTIVE))
        vc_info->run_mode = NSM_MPLS_VC_STANDBY;
    }

  /* Activate VC. */
  if (vc)
    {
      vc->vc_info = vc_info;
      vc_info->u.vc = vc; 
      if (vc_info->run_mode == NSM_MPLS_VC_STANDBY)
        SET_FLAG (vc->pw_status, NSM_CODE_PW_STANDBY);

      ret = nsm_mpls_vc_activate (vc); 
      if (ret != NSM_SUCCESS &&
          vc->state != NSM_MPLS_L2_CIRCUIT_ACTIVE)
        {
          if (CHECK_FLAG (vc_info->if_bind_type, NSM_IF_BIND_MPLS_VC) ||
              CHECK_FLAG (vc_info->if_bind_type, NSM_IF_BIND_MPLS_VC_VLAN))
            nsm_mpls_vc_info_del (mif, mif->vc_info_list, vc_info);
#ifdef HAVE_VPLS
          else
            nsm_mpls_vc_info_del (mif, mif->vpls_info_list, vc_info);
#endif /* HAVE_VPLS */

          if (vlan_id == 0)
            UNSET_FLAG (ifp->bind, NSM_IF_BIND_MPLS_VC);
          else
            UNSET_FLAG (ifp->bind, NSM_IF_BIND_MPLS_VC_VLAN);

          nsm_mpls_vc_info_free (vc_info);
          vc_info = NULL;

          return ret;
        }
    }
  
  if (vc_info->u.vc)
    vc_info->u.vc->last_change = nsm_get_sys_up_time();
  return NSM_SUCCESS;
}

/* Delete a Layer-2 circuit. */
int
nsm_mpls_l2_circuit_del (struct nsm_master *nm,
                         struct nsm_mpls_circuit *vc, bool_t send_msg)
{
  struct nsm_mpls_if *mif = NULL;
  struct nsm_mpls_vc_info *vc_info = NULL;
  struct listnode *ln;
  struct route_node *rn;
  struct nsm_mpls_circuit_container *vc_container = NULL;
#if defined (HAVE_SNMP) || defined (HAVE_MS_PW)
  int ret = 0;
#endif /* HAVE_SNMP || HAVE_MS_PW */

  if (! vc)
    return NSM_FAILURE;

  /* Send update to clients. */
  if (vc->vc_info && vc->vc_info->mif)
    mif = vc->vc_info->mif;

  if (mif)
    {
      LIST_LOOP (mif->vc_info_list, vc_info, ln)
        {
          if ((! CHECK_FLAG (vc_info->if_bind_type, NSM_IF_BIND_MPLS_VC)) &&
              (! CHECK_FLAG (vc_info->if_bind_type, NSM_IF_BIND_MPLS_VC_VLAN)))
            continue;

          if ((vc_info->vc_name) 
              && (pal_strcmp (vc_info->vc_name, vc->name) == 0)
              && vc_info->u.vc == vc)
            {
#ifdef HAVE_MS_PW
              if (vc->ms_pw)
                {
                  /* Send vc delete info to LDP */
                  if (CHECK_FLAG (vc->flags, NSM_MPLS_VC_FLAG_CONFIGURED))
                    ret = nsm_mpls_ms_pw_stitch_send (nm, vc->ms_pw, NSM_FALSE);

                  if (ret < 0)
                    {
                      zlog_info (NSM_ZG, "%s - %d: send to PM failed\n",
                          __FUNCTION__, __LINE__);
                    }

                  ret = nsm_mpls_ms_pw_deactivate (nm, vc);
                  if (ret < 0)
                    {
                      zlog_err (nzg, "%s-%d: Failed to delete MSPW and vc %d\n",
                          __FUNCTION__, __LINE__, vc->id);
                      return ret;
                    }
                }
              else
#endif /* HAVE_MS_PW */
                {
                  /* Withdraw from above and below. */
                  nsm_mpls_vc_deactivate (vc_info->u.vc, send_msg, NSM_TRUE);
                  vc_info->u.vc->vc_info = NULL;
                  vc_info->u.vc = NULL;
                }
              break;
            }
        } 

    }
#ifdef HAVE_VPLS
  else if (vc->vpls)
    {
      struct nsm_vpls_spoke_vc *svc;

      svc = nsm_vpls_spoke_vc_lookup (vc->vpls, vc->id, NSM_FALSE);
      NSM_ASSERT (svc != NULL);
      if (svc)
        nsm_vpls_spoke_vc_cleanup (nm, svc, NSM_TRUE, NSM_TRUE);
    }
#endif /* HAVE_VPLS */

  /* Trap notifying VC deletion */
#ifdef HAVE_SNMP
  if (vc->vc_info)
    {
      nsm_mpls_vc_del_notify (vc->id, vc->vc_info->vc_type,
                              ((vc->address.family == AF_INET)
                                ? INET_AD_IPV4 :INET_AD_UNKNOWN), 
                              &vc->address.u.prefix4,
                              vc->vc_snmp_index);
    }
  else
    {
      nsm_mpls_vc_del_notify (vc->id, 0,
                              ((vc->address.family == AF_INET)
                                ? INET_AD_IPV4 :INET_AD_UNKNOWN), 
                              &vc->address.u.prefix4,
                              vc->vc_snmp_index);
    }
  ret = bitmap_release_index (NSM_MPLS->vc_indx_mgr,vc->vc_snmp_index);
  if (ret < 0)
    return NSM_FAILURE;
#endif /* HAVE_SNMP */
   VC_TIMER_OFF(vc->t_time_elpsd);
   if (vc->nsm_mpls_pw_snmp_perf_curr_list)
     list_delete (vc->nsm_mpls_pw_snmp_perf_curr_list);

  /* Unbind from group. */
  if (vc->group)
    nsm_mpls_l2_vc_group_unregister (nm, vc->group, vc);

  vc_container = nsm_mpls_vc_lookup_by_name (nm, vc->name);
  if (!vc_container)
    return NSM_FAILURE;

  /* Remove listnode and free vc object. */
  rn = vc->rn;
  rn->info = NULL;
  route_unlock_node (rn);

#ifdef HAVE_MS_PW
  if (vc->ms_pw)
    {
      /* There is a mspw configuration so do not delete the vc or vc_container
       * Unset the flag configured indicating the VC is not physically created
       */
      UNSET_FLAG (vc->flags, NSM_MPLS_VC_FLAG_CONFIGURED);
      vc->id = 0;
    }
  else
#endif /* HAVE_MS_PW */
    {
      /* Remove name. */
      XFREE (MTYPE_TMP, vc->name);
      XFREE (MTYPE_NSM_VIRTUAL_CIRCUIT, vc);

      nsm_mpls_vc_remove_from_hash (nm, vc_container->name);
      XFREE (MTYPE_NSM_VC_CONTAINER, vc_container);
    }

  return NSM_SUCCESS;
}

/* Delete a Layer-2 circuit using name and id. */
int
nsm_mpls_l2_circuit_del2 (struct nsm_master *nm,
                          char *vc_name, u_int32_t vc_id)
{
  struct nsm_mpls_circuit *vc1, *vc2;

  /* Lookup by id. */
  vc1 = nsm_mpls_l2_circuit_lookup_by_id (nm, vc_id);
  if (! vc1)
    return NSM_ERR_VC_ID_NOT_FOUND;

  /* Compare name. */
  if (pal_strcmp (vc_name, vc1->name) != 0)
    return NSM_ERR_VC_NAME_ID_MISMATCH;

  /* Lookup by name. */
  vc2 = nsm_mpls_l2_circuit_lookup_by_name (nm, vc_name);
  if (! vc2)
    return NSM_ERR_VC_NAME_NOT_FOUND;

  /* Confirm that these two match. */
  if (vc1 != vc2)
    return NSM_ERR_VC_NAME_ID_MISMATCH;

  return nsm_mpls_l2_circuit_del (nm, vc1, NSM_TRUE);
}

/* Delete all Layer-2 circuits configured. */
void
nsm_mpls_l2_circuit_delete_all (struct nsm_master *nm, bool_t send_msg)
{
  struct nsm_mpls_circuit *vc;
  struct route_node *rn;
  struct nsm_mpls_vc_group *group = NULL;
  struct listnode *ln = NULL;

  if ((! NSM_MPLS) || (! NSM_MPLS->vc_table))
    return;

  for (rn = route_top (NSM_MPLS->vc_table); rn; rn = route_next (rn))
    {
      /* If no info, skip. */
      vc = rn->info;
      if (! vc)
        continue;

      (void) nsm_mpls_l2_circuit_del (nm, vc, send_msg);
    }

  LIST_LOOP (NSM_MPLS->vc_group_list, group, ln)
    {
      if (listcount(group->vc_list) == 0 )
        nsm_mpls_l2_vc_group_delete ( nm, group );
    }

}

/* API to be used by interface code for unbinding from l2 circuit. */
int
nsm_mpls_l2_circuit_unbind_by_ifp (struct interface *ifp, char *vc_name,
                                   u_int16_t vlan_id)
{
  struct nsm_mpls_if *mif;

  /* Lookup interface. */
  mif = nsm_mpls_if_lookup (ifp);
  if (! mif)
    return NSM_ERR_NOT_FOUND;

  return nsm_mpls_l2_circuit_unbind (mif, vc_name, vlan_id);
}

/* API to be used by interface code for binding to l2 circuit. */
int
nsm_mpls_l2_circuit_bind_by_ifp (struct interface *ifp, char *vc_name,
                                 u_int16_t vc_type, u_int16_t vlan_id,
                                 u_int16_t vc_mode)
{
  struct nsm_mpls_if *mif;

  /* Check if this is already bound to a VRF. */
  if (ifp->vrf->id != 0)
    return NSM_ERR_VPN_EXISTS_ON_IF;

  /* Lookup interface. */
  mif = nsm_mpls_if_lookup (ifp);
  if (! mif)
    return NSM_ERR_NOT_FOUND;

  return nsm_mpls_l2_circuit_bind (mif, vc_name, vc_type, vlan_id, vc_mode);
}

#ifdef HAVE_VLAN
void
nsm_mpls_vc_vlan_bind (struct interface *ifp, u_int16_t vlan_id)
{
  struct nsm_mpls_if *mif;
  struct nsm_mpls_vc_info *vc_info;
  struct listnode *ln;

  /* Lookup interface. */
  mif = nsm_mpls_if_lookup (ifp);
  if (! mif)
    return;

  LIST_LOOP (mif->vc_info_list, vc_info, ln) 
    if ((vc_info->vlan_id != 0) &&
        (vc_info->vlan_id == vlan_id || vlan_id == NSM_VLAN_ALL) &&
        vc_info->u.vc)
      {
        nsm_mpls_vc_activate (vc_info->u.vc);
      }
  return;
}

void
nsm_mpls_vc_vlan_unbind (struct interface *ifp, u_int16_t vlan_id)
{
  struct nsm_mpls_if *mif;
  struct nsm_mpls_vc_info *vc_info;
  struct listnode *ln;

  /* Lookup interface. */
  mif = nsm_mpls_if_lookup (ifp);
  if (! mif)
    return;

  LIST_LOOP (mif->vc_info_list, vc_info, ln) 
    if ((vc_info->vlan_id != 0) &&
        (vc_info->vlan_id == vlan_id || vlan_id == NSM_VLAN_ALL) && 
        vc_info->u.vc)
       {
         nsm_mpls_vc_deactivate (vc_info->u.vc, NSM_TRUE,
                                 vc_info->u.vc->fec_type_vc == PW_OWNER_MANUAL
                                 ? NSM_FALSE : NSM_TRUE);
       }

  return;
}
#endif /* HAVE_VLAN */
void
nsm_mpls_vc_if_up_process (struct nsm_mpls_circuit *vc, struct nsm_mpls_if *mif)
{
    int ret;
    struct fec_gen_entry gen_entry;

    if (! vc)
      return;

    NSM_MPLS_PREFIX_TO_FEC_GEN_ENTRY(&gen_entry, &vc->address);

    UNSET_FLAG (vc->pw_status, NSM_CODE_PW_AC_INGRESS_RX_FAULT);
    UNSET_FLAG (vc->pw_status, NSM_CODE_PW_AC_EGRESS_TX_FAULT);

    if (vc->state == NSM_MPLS_L2_CIRCUIT_COMPLETE &&
        FLAG_ISSET (vc->flags, NSM_MPLS_VC_FLAG_SELECTED))
      {
        gmpls_lsp_dep_add (mif->nm, &gen_entry, CONFIRM_VC, vc);
        SET_FLAG (vc->flags, NSM_MPLS_VC_FLAG_DEP);
      }
    else if (vc->state == NSM_MPLS_L2_CIRCUIT_ACTIVE)
      {
        ret = nsm_mpls_if_install_vc_data (mif, vc, vc->vc_info->vlan_id);
        if (ret < 0)
          {
            if (vc->vc_info &&
                (CHECK_FLAG (vc->vc_info->if_bind_type, NSM_IF_BIND_MPLS_VC) ||
                 CHECK_FLAG (vc->vc_info->if_bind_type, NSM_IF_BIND_MPLS_VC_VLAN)))
              nsm_mpls_vc_info_del (mif, mif->vc_info_list, vc->vc_info);

            if (vc->vc_info)
              {
                nsm_mpls_vc_info_free (vc->vc_info);
                vc->vc_info = NULL;
              }
          }
      }
}

void
nsm_mpls_vc_if_down_process (struct nsm_mpls_circuit *vc,
                             struct nsm_mpls_if *mif)
{
    struct fec_gen_entry gen_entry;

    if (! vc)
      return;

    NSM_MPLS_PREFIX_TO_FEC_GEN_ENTRY(&gen_entry, &vc->address);

    SET_FLAG (vc->pw_status, NSM_CODE_PW_AC_INGRESS_RX_FAULT);
    SET_FLAG (vc->pw_status, NSM_CODE_PW_AC_EGRESS_TX_FAULT);

    if (vc->state >= NSM_MPLS_L2_CIRCUIT_COMPLETE &&
        FLAG_ISSET (vc->flags, NSM_MPLS_VC_FLAG_DEP))
      {
        gmpls_lsp_dep_del (mif->nm, &gen_entry, CONFIRM_VC, vc);
        UNSET_FLAG (vc->flags, NSM_MPLS_VC_FLAG_DEP);
      }
}

int
nsm_mpls_vc_runtime_mode_set (struct nsm_master *nm, 
                              struct nsm_mpls_vc_info *vc_info)
{
  u_char old_rmode = vc_info->run_mode;
  struct nsm_mpls_circuit *vc = vc_info->u.vc;
  int ret;

  if (CHECK_FLAG (vc_info->conf_flag, NSM_MPLS_VC_CONF_STANDBY))
    vc_info->run_mode = NSM_MPLS_VC_STANDBY;
  else
    vc_info->run_mode = NSM_MPLS_VC_ACTIVE;

  if (old_rmode == vc_info->run_mode)
    return 0;

  if (! vc)
    return 0;

  if (vc_info->run_mode == NSM_MPLS_VC_STANDBY)
    SET_FLAG (vc->pw_status, NSM_CODE_PW_STANDBY);
  else
    UNSET_FLAG (vc->pw_status, NSM_CODE_PW_STANDBY);

  /* runtime mode change from Active -> Standby, need remove vc from fib */
  if (vc->vc_fib && vc->vc_fib->install_flag == NSM_TRUE && 
      vc_info->run_mode == NSM_MPLS_VC_STANDBY)
    {
      ret = nsm_mpls_vc_fib_del (nm, vc, vc->id);
      if (ret != NSM_SUCCESS)
        {
          /* send pw-status to signaling protocol */
          nsm_mpls_l2_circuit_send_pw_status (vc->vc_info->mif, vc);
        }
    }
  else if ((vc->vc_fib && vc->vc_fib->install_flag != NSM_TRUE) &&
           vc->state == NSM_MPLS_L2_CIRCUIT_UP &&
           vc_info->run_mode == NSM_MPLS_VC_ACTIVE &&
           PW_STATUS_INSTALL_CHECK (vc->pw_status, vc->remote_pw_status))
    {
      ret = nsm_mpls_vc_fib_add (nm, vc, vc->id, vc->vc_fib->opcode, vc->ftn);
    }
  else
    {
      /* send pw-status to signaling protocol */
      nsm_mpls_l2_circuit_send_pw_status (vc->vc_info->mif, vc);
    }

  return 0;
}

/* API to be used to set VC runtime mode. */
int
nsm_mpls_l2_circuit_runtime_mode_conf (struct interface *ifp, u_char vc_mode,
                                       u_int16_t vlan_id)
{
  struct nsm_master *nm;
  struct nsm_mpls_if *mif;
  struct listnode *ln;
  struct nsm_mpls_vc_info *vc_info = NULL, *s_info = NULL;
  int found = 0;
  int ret = 0;

  if (! ifp)
    return NSM_ERR_NOT_FOUND;

  /* Check if this is already bound to a VRF. */
  if (ifp->vrf->id != 0)
    return NSM_ERR_VPN_EXISTS_ON_IF;

  /* Lookup interface. */
  mif = nsm_mpls_if_lookup (ifp);
  if (! mif)
    return NSM_ERR_NOT_FOUND;

  nm = mif->nm;

  LIST_LOOP (mif->vc_info_list, vc_info, ln)
    {
      if (vc_info->vlan_id != vlan_id)
        continue;

      s_info = vc_info->sibling_info;

      if (CHECK_FLAG (vc_info->conf_flag, NSM_MPLS_VC_CONF_REVERTIVE))
        return NSM_ERR_VC_MODE_CONFLICT; 

      /* standby could only configure on primary or primary/primary sibling */
      if (vc_mode == NSM_MPLS_VC_CONF_STANDBY)
        if (CHECK_FLAG (vc_info->conf_flag, NSM_MPLS_VC_CONF_SECONDARY) || 
            (s_info && 
             CHECK_FLAG (s_info->conf_flag, NSM_MPLS_VC_CONF_SECONDARY)))
        return NSM_ERR_VC_MODE_MISMATCH;

      if (vc_mode == NSM_MPLS_VC_CONF_STANDBY)
        SET_FLAG (vc_info->conf_flag, NSM_MPLS_VC_CONF_STANDBY);
      else
        UNSET_FLAG (vc_info->conf_flag, NSM_MPLS_VC_CONF_STANDBY);

      ret = nsm_mpls_vc_runtime_mode_set (nm, vc_info);

      if (s_info)
        {
          if (vc_mode == NSM_MPLS_VC_CONF_STANDBY)
            SET_FLAG (s_info->conf_flag, NSM_MPLS_VC_CONF_STANDBY);
          else
            UNSET_FLAG (s_info->conf_flag, NSM_MPLS_VC_CONF_STANDBY);

          ret = nsm_mpls_vc_runtime_mode_set (nm, s_info);
        }

      found = 1;
      break;
    }

  if (! found)
    return NSM_ERR_VC_NOT_CONFIGURED;

  return NSM_SUCCESS;
}

int
nsm_mpls_vc_secondary_qualified (struct nsm_mpls_circuit *vc)
{
  u_int32_t tmp_pw_status;

  if (! vc || ! vc->vc_info || ! vc->vc_fib)
    return NSM_FALSE;

  tmp_pw_status = vc->pw_status;
  UNSET_FLAG (tmp_pw_status, NSM_CODE_PW_STANDBY);

  if (! CHECK_FLAG (vc->vc_info->conf_flag, NSM_MPLS_VC_CONF_STANDBY) &&
       PW_STATUS_INSTALL_CHECK (tmp_pw_status, vc->remote_pw_status))
    return NSM_TRUE;

  return NSM_FALSE;
}

int
nsm_mpls_vc_revert (struct nsm_mpls_circuit *vc, int primary)
{
  int ret = NSM_FALSE;
  u_int32_t tmp_pw_status;
  struct nsm_mpls_vc_info *vc_info;

  if (! vc)
    return ret;

  vc_info = vc->vc_info;

  if (! vc_info || ! vc->vc_fib || 
      vc->state != NSM_MPLS_L2_CIRCUIT_UP ||
      vc->vc_fib->install_flag == NSM_TRUE)
    return ret;

  /* test if vc is qualified by only clear local standby bit */
  tmp_pw_status = vc->pw_status;
  UNSET_FLAG (tmp_pw_status, NSM_CODE_PW_STANDBY);

  /* for primary revertive case */
  if (primary)
    {
      if (! CHECK_FLAG (vc_info->conf_flag, NSM_MPLS_VC_CONF_STANDBY) &&
          CHECK_FLAG (vc_info->conf_flag, NSM_MPLS_VC_CONF_REVERTIVE) && 
          ! CHECK_FLAG (vc_info->conf_flag, NSM_MPLS_VC_CONF_SECONDARY) &&
          PW_STATUS_INSTALL_CHECK (tmp_pw_status, vc->remote_pw_status))
        return NSM_TRUE;
    }
  /* for any primary/secondary qualification test */
  else if (! CHECK_FLAG (vc_info->conf_flag, NSM_MPLS_VC_CONF_STANDBY) &&
           PW_STATUS_INSTALL_CHECK (tmp_pw_status, vc->remote_pw_status))
    return NSM_TRUE;

  return ret;
}

int
nsm_mpls_vc_primary_revert (struct nsm_master *nm, struct nsm_mpls_circuit *vc) 
{
  struct nsm_mpls_circuit *s_vc = NULL;
  struct nsm_mpls_vc_info *s_info = NULL;
  int ret = 0;

  if (! vc || ! vc->vc_info)
    return 0;

  if (CHECK_FLAG (vc->vc_info->conf_flag, NSM_MPLS_VC_CONF_SECONDARY))
    return 0;

  s_info = vc->vc_info->sibling_info;
  if (s_info)
    s_vc = s_info->u.vc;

  if (s_vc && s_vc->vc_fib && s_vc->vc_fib->install_flag == NSM_TRUE)
    {
      s_info->run_mode = NSM_MPLS_VC_STANDBY;
      SET_FLAG (s_vc->pw_status, NSM_CODE_PW_STANDBY);
      ret = nsm_mpls_vc_fib_del (nm, s_vc, s_vc->id);
      if (ret != NSM_SUCCESS)
        {
          /* send pw-status to signaling protocol */
          nsm_mpls_l2_circuit_send_pw_status (s_vc->vc_info->mif, s_vc);
        }
    }

  vc->vc_info->run_mode = NSM_MPLS_VC_ACTIVE;
  UNSET_FLAG (vc->pw_status, NSM_CODE_PW_STANDBY);
  ret = nsm_mpls_vc_fib_add (nm, vc, vc->id, vc->vc_fib->opcode, vc->ftn);

  return ret;
}

void
nsm_mpls_l2_circuit_revertive_set (struct nsm_mpls_circuit *vc)
{
  struct nsm_mpls_circuit *s_vc = NULL;
  struct nsm_mpls_vc_info *s_info = NULL;
  struct nsm_mpls_vc_info *vc_info = NULL; 
  int send = NSM_FALSE;

  if (! vc)
    return;

  vc_info = vc->vc_info;

  if (! vc_info)
    return;

  s_info = vc->vc_info->sibling_info;
  if (s_info)
    s_vc = s_info->u.vc;

  /* if vc_mode is revertive, vc is primary, set run_mode to active */
  if (CHECK_FLAG (vc_info->conf_flag, NSM_MPLS_VC_CONF_REVERTIVE) &&
      ! CHECK_FLAG (vc_info->conf_flag, NSM_MPLS_VC_CONF_SECONDARY) &&
      vc_info->run_mode != NSM_MPLS_VC_ACTIVE)
    {
      vc_info->run_mode = NSM_MPLS_VC_ACTIVE;
      UNSET_FLAG (vc->pw_status, NSM_CODE_PW_STANDBY);
      send = NSM_TRUE;
    }

  /* if vc_mode is non-revertive, vc is primary, sibling is installed, 
     set run_mode to standby */
  if (! CHECK_FLAG (vc_info->conf_flag, NSM_MPLS_VC_CONF_REVERTIVE) &&
      ! CHECK_FLAG (vc_info->conf_flag, NSM_MPLS_VC_CONF_SECONDARY) &&
      s_vc && s_vc->vc_fib && s_vc->vc_fib->install_flag == NSM_TRUE &&
      vc_info->run_mode != NSM_MPLS_VC_STANDBY)
    {
      vc_info->run_mode = NSM_MPLS_VC_STANDBY;
      SET_FLAG (vc->pw_status, NSM_CODE_PW_STANDBY);
      send = NSM_TRUE;
    }

  if (send)
    nsm_mpls_l2_circuit_send_pw_status (vc->vc_info->mif, vc);
}

/* API to be used to set VC revertive mode. */
int
nsm_mpls_l2_circuit_revertive_mode_conf (struct interface *ifp, u_char vc_mode,
                                          u_int16_t vlan_id)
{
  struct nsm_master *nm;
  struct nsm_mpls_if *mif;
  struct listnode *ln;
  struct nsm_mpls_vc_info *vc_info = NULL, *s_info = NULL;
  struct nsm_mpls_circuit *vc = NULL, *s_vc = NULL;
  int found = 0;
  int ret = 0;

  if (! ifp)
    return NSM_ERR_NOT_FOUND;

  /* Check if this is already bound to a VRF. */
  if (ifp->vrf->id != 0)
    return NSM_ERR_VPN_EXISTS_ON_IF;

  /* Lookup interface. */
  mif = nsm_mpls_if_lookup (ifp);
  if (! mif)
    return NSM_ERR_NOT_FOUND;

  nm = mif->nm;

  LIST_LOOP (mif->vc_info_list, vc_info, ln)
    {
      if (vc_info->vlan_id != vlan_id)
        continue;

      vc = vc_info->u.vc;
      s_info = vc_info->sibling_info;

      if (CHECK_FLAG (vc_info->conf_flag, NSM_MPLS_VC_CONF_STANDBY))
        return NSM_ERR_VC_MODE_CONFLICT;
 
      if (s_info &&
          ! CHECK_FLAG (s_info->conf_flag, NSM_MPLS_VC_CONF_SECONDARY) &&
          ! CHECK_FLAG (vc_info->conf_flag, NSM_MPLS_VC_CONF_SECONDARY) &&
          vc_mode == NSM_MPLS_VC_CONF_REVERTIVE)
        return NSM_ERR_VC_MODE_MISMATCH_REV; 

      if (vc_mode == NSM_MPLS_VC_CONF_REVERTIVE)
        SET_FLAG (vc_info->conf_flag, NSM_MPLS_VC_CONF_REVERTIVE);
      else
        UNSET_FLAG (vc_info->conf_flag, NSM_MPLS_VC_CONF_REVERTIVE);

      nsm_mpls_l2_circuit_revertive_set (vc);

      if (s_info)
        {
          if (vc_mode == NSM_MPLS_VC_CONF_REVERTIVE)
            SET_FLAG (s_info->conf_flag, NSM_MPLS_VC_CONF_REVERTIVE);
          else
            UNSET_FLAG (s_info->conf_flag, NSM_MPLS_VC_CONF_REVERTIVE);

          s_vc = s_info->u.vc;
          nsm_mpls_l2_circuit_revertive_set (s_vc);
        }

      if (vc && nsm_mpls_vc_revert (vc, NSM_TRUE))
        ret = nsm_mpls_vc_primary_revert (nm, vc);
      if (s_vc && nsm_mpls_vc_revert (s_vc, NSM_TRUE))
        ret = nsm_mpls_vc_primary_revert (nm, s_vc);

      found = 1;
      break;
    }

  if (! found)
    return NSM_ERR_VC_NOT_CONFIGURED;

  return NSM_SUCCESS;
}

#ifdef HAVE_VPLS
void
nsm_vpls_group_unregister (struct nsm_master *nm,
                           struct nsm_vpls *vpls)
{
  struct nsm_vpls *tmp_vpls;
  struct listnode *ln;
  struct nsm_mpls_vc_group *group;

  NSM_ASSERT( nm );
  NSM_ASSERT( vpls );

  /* Lookup group. */
  group = nsm_mpls_l2_vc_group_lookup_by_id (nm, vpls->grp_id);

  if (! group)
    return;

  NSM_ASSERT( group->vpls_list );
  /* Find vpls. */
  LIST_LOOP (group->vpls_list, tmp_vpls, ln)
    {
      if (tmp_vpls == vpls)
        {
          vpls->grp_id = 0;
          list_delete_node (group->vpls_list, ln);
          break;
        }
    }
}

void
nsm_vpls_group_register (struct nsm_master *nm,
                         struct nsm_mpls_vc_group* group,
                         struct nsm_vpls *vpls )
{
  NSM_ASSERT( nm );
  NSM_ASSERT( group );
  NSM_ASSERT( group->vpls_list );
  NSM_ASSERT( vpls );

  /* Add listnode and set group. */
  listnode_add (group->vpls_list, (void *)vpls);
  vpls->grp_id = group->id;
}
#endif /* HAVE_VPLS */

/** @brief API to manually switch between VC's.

    @param vr_id         - VR identifier
    @param primary_vc    - Primary VC name
    @param sec_vc        - Secondary VC name
 */
int
nsm_mpls_api_pw_switchover (u_int32_t vr_id, char *primary_vc, char *sec_vc)
{
  struct nsm_master *nm = NULL; 
  struct nsm_mpls_circuit *pvc = NULL, *svc = NULL, *sibling_vc = NULL;
  struct nsm_mpls_vc_info *s_info = NULL;

  nm = nsm_master_lookup_by_id (nzg, vr_id);
  if (!nm)
    return NSM_ERR_INVALID_ARGS;

  if (primary_vc == NULL || sec_vc == NULL)
    return NSM_ERR_INVALID_ARGS;

  pvc = nsm_mpls_l2_circuit_lookup_by_name (nm, primary_vc);
  svc = nsm_mpls_l2_circuit_lookup_by_name (nm, sec_vc);

  /* Validate VC. */
  if (pvc == NULL || svc == NULL)
    return NSM_ERR_INVALID_VC_NAME;

  /* Validate whether VC is installed - fib.*/
  if (!pvc->vc_fib || 
      pvc->vc_fib->install_flag != NSM_TRUE)
    return NSM_ERR_PRIMARY_VC_INACTIVE;
 
  /* Validate sibling info. */
  s_info = pvc->vc_info->sibling_info;
  if (s_info == NULL)
    return NSM_VC_ERR_NO_SIBLING;

  /* Compare Sibling and Secondary VC. */ 
  sibling_vc = s_info->u.vc;
  if (!sibling_vc)
    return NSM_VC_ERR_NO_SIBLING; 

  if (sibling_vc != svc) 
    return NSM_VC_ERR_SIBLING_MISMATCH;  

  /* Validate mode. */
  if (! CHECK_FLAG (s_info->conf_flag, NSM_MPLS_VC_CONF_SECONDARY) &&
      ! CHECK_FLAG (pvc->vc_info->conf_flag, NSM_MPLS_VC_CONF_SECONDARY))
    return NSM_ERR_VC_MODE_NO_SECONDARY;

  /* Validate VC mode. */
  if (CHECK_FLAG (s_info->conf_flag, NSM_MPLS_VC_CONF_REVERTIVE))
    return NSM_ERR_VC_SWITCHOVER_MODE_CONFLICT; 

  /* Validate whether VC is installed - fib.*/
  if (svc->vc_fib == NULL)
    return NSM_ERR_SECONDARY_VC_INACTIVE;

  /* Check if secondary VC already installed. */
  if (svc->vc_fib->install_flag == NSM_TRUE)
    return NSM_ERR_SEC_VC_ALREADY_INSTALLED;
  
  /* Switchover VC. */
  return nsm_mpls_pw_manual_switchover (nm, pvc, svc);
}


#ifdef HAVE_VCCV
void
nsm_mpls_set_cc_types (u_int8_t* cc_types, char * cc_type_str)
{
  if (!cc_types || !cc_type_str)
    return;

  if (pal_strncmp (cc_type_str, "1", 1) == 0)
    SET_FLAG (*cc_types, CC_TYPE_1_BIT);
  else if (pal_strncmp (cc_type_str, "2", 1) == 0)
    SET_FLAG (*cc_types, CC_TYPE_2_BIT);
  else
    SET_FLAG (*cc_types, CC_TYPE_3_BIT);
}

u_int8_t
nsm_mpls_get_default_cc_types (bool_t c_word)
{
  u_int8_t cc_types = 0;

  /* Set all the supported  cc_types */
  if (c_word)
    SET_FLAG (cc_types, CC_TYPE_1_BIT);
  SET_FLAG (cc_types, CC_TYPE_2_BIT);
  SET_FLAG (cc_types, CC_TYPE_3_BIT);

  return cc_types;
}

char *
nsm_mpls_get_cctype_from_bit (u_int8_t cc_types)
{
    if (CHECK_FLAG (cc_types, CC_TYPE_1_BIT))
      return "1";
    else if (CHECK_FLAG (cc_types, CC_TYPE_2_BIT))
      return "2";
    else if (CHECK_FLAG (cc_types, CC_TYPE_3_BIT))
      return "3";
    else
      return "0";
}

char *
nsm_mpls_get_bfd_cvtype_from_bit (u_int8_t cv_types)
{
  if (CHECK_FLAG (cv_types, CV_TYPE_BFD_IPUDP_DET_BIT))
    return "1";
  else if (CHECK_FLAG (cv_types, CV_TYPE_BFD_IPUDP_DET_SIG_BIT))
    return "2";
  else if (CHECK_FLAG (cv_types, CV_TYPE_BFD_ACH_DET_BIT))
    return "3";
  else if (CHECK_FLAG (cv_types, CV_TYPE_BFD_ACH_DET_SIG_BIT))
    return "4";
  else
    return "0";
}

u_int8_t
nsm_mpls_get_default_cv_types ()
{
  u_int8_t cv_types = 0;

  /* Set all the supported cv_types */
  SET_FLAG (cv_types, CV_TYPE_LSP_PING_BIT);

  return cv_types;
}

bool_t
nsm_mpls_is_bfd_set (u_int8_t cv_types)
{
  if (CHECK_FLAG(cv_types, CV_TYPE_BFD_IPUDP_DET_BIT) ||
      CHECK_FLAG(cv_types, CV_TYPE_BFD_IPUDP_DET_SIG_BIT) ||
      CHECK_FLAG(cv_types, CV_TYPE_BFD_ACH_DET_BIT) ||
      CHECK_FLAG(cv_types, CV_TYPE_BFD_ACH_DET_SIG_BIT))
    return PAL_TRUE;
  else
    return PAL_FALSE;
}

u_int8_t
nsm_mpls_get_default_bfd_cv_types (bool_t is_manual, bool_t c_word)
{
  u_int8_t cv_types = 0;
 
  /* Set all the supported BFD cv types */
  SET_FLAG (cv_types, CV_TYPE_BFD_IPUDP_DET_BIT);

  if (c_word)
    SET_FLAG (cv_types, CV_TYPE_BFD_ACH_DET_BIT);

  if (is_manual)
    {
      SET_FLAG (cv_types, CV_TYPE_BFD_IPUDP_DET_SIG_BIT);

      if (c_word)
        SET_FLAG (cv_types, CV_TYPE_BFD_ACH_DET_SIG_BIT);
    }

  return cv_types;
}

bool_t
nsm_mpls_is_specific_bfd_in_use (u_int8_t cv_types, 
                                 bool_t is_manual, 
                                 bool_t c_word)
{
  if (!CHECK_FLAG(cv_types, CV_TYPE_BFD_IPUDP_DET_BIT))
    return PAL_TRUE;

  if (c_word && (!CHECK_FLAG(cv_types, CV_TYPE_BFD_ACH_DET_BIT)))
    return PAL_TRUE;

  if (is_manual)
    {
      if (!CHECK_FLAG(cv_types, CV_TYPE_BFD_IPUDP_DET_SIG_BIT))
        return PAL_TRUE;

      if (c_word && (!CHECK_FLAG (cv_types, CV_TYPE_BFD_ACH_DET_SIG_BIT)))
        return PAL_TRUE;
    }

  return PAL_FALSE;
}
#endif /* HAVE_VCCV */
#endif /* HAVE_MPLS_VC */
