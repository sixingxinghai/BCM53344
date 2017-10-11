/* Copyright (C) 2008-2009  All Rights Reserved. */
#include "pal.h"
#include "lib.h"
#include "if.h"
#include "prefix.h"
#include "thread.h"
#include "table.h"
#include "linklist.h"
#include "log.h"

#include "ospfd/ospfd.h"
#include "ospfd/ospf_interface.h"
#include "ospfd/ospf_vlink.h"
#include "ospfd/ospf_multi_area_link.h"
#include "ospfd/ospf_network.h"
#include "ospfd/ospf_packet.h"
#include "ospfd/ospf_neighbor.h"
#include "ospfd/ospf_ifsm.h"
#include "ospfd/ospf_nfsm.h"
#include "ospfd/ospf_lsa.h"
#include "ospfd/ospf_lsdb.h"
#include "ospfd/ospf_abr.h"
#include "ospfd/ospf_asbr.h"
#include "ospfd/ospf_route.h"
#include "ospfd/ospf_nsm.h"
#include "ospfd/ospf_api.h"
#include "ospfd/ospf_debug.h"
#include "ospfd/ospf_vrf.h"
#ifdef HAVE_RESTART
#include "ospfd/ospf_restart.h"
#endif /* HAVE_RESTART */
#ifdef HAVE_OSPF_TE
#include "ospfd/ospf_te.h"
#endif /* HAVE_OSPF_TE */

#ifdef HAVE_OSPF_MULTI_AREA

/* Free the multi area link */
void
ospf_multi_area_link_free (struct ospf_multi_area_link *mlink)
{
  if (mlink->oi)
    ospf_if_unlock (mlink->oi);

  XFREE (MTYPE_OSPF_DESC, mlink->ifname);
  XFREE (MTYPE_OSPF_MULTI_AREA_LINK, mlink);
}

/* Create interface for multi area link */
struct ospf_interface *
ospf_multi_area_if_new (struct ospf *top, struct connected *ifc,
                        struct pal_in4_addr area_id, struct ls_prefix nbr_prefix)
{
  struct ospf_interface *oi_primary = NULL;
  struct ospf_interface *oi = NULL;
  struct ospf_area *area = NULL;

  oi_primary = ospf_global_if_entry_lookup (top->om, ifc->address->u.prefix4,
                                          ifc->ifp->ifindex);

  /* Check whether primary adjacency exists or not */
  if (oi_primary == NULL)
    {
      zlog_err (ZG, "Primary adjacency not exists");
      return NULL;
    }

  /* Multi area adjacency area id is same as primary adjacency no need to create
     the multi area adjacency link */
  if (IPV4_ADDR_SAME (&oi_primary->area->area_id, &area_id))
    {
      zlog_err (ZG, "Area id is same as primary adjacency link");
      return NULL; 
    }

  area = ospf_area_get (top, area_id);
  if (area == NULL)
    return NULL;

  /* Create ospf interface for multi area link */ 
  oi = XMALLOC (MTYPE_OSPF_IF, sizeof (struct ospf_interface));
  if (oi == NULL)
    return NULL;

  pal_mem_set (oi, 0, sizeof (struct ospf_interface));

  /* Init reference count. */
  ospf_if_lock (oi);
  oi->clone = 1;

  /* Set references. */
  oi->top = top;
  oi->u.ifp = ifc->ifp;

  /* Set the multi area adjacency flag */
  SET_FLAG (oi->flags, OSPF_MULTI_AREA_IF);

  /* Set the interface type.  */
  oi->type = OSPF_IFTYPE_POINTOPOINT;

  /* Lock on primary interface */
  ospf_if_lock (oi_primary);
  oi->oi_primary = oi_primary;

  /* Initialize the interfaces.  */
  ospf_if_init (oi, ifc, area);

  /* Initialize the neighbor */
  prefix_copy (oi->destination, (struct prefix *) &nbr_prefix);

  /* Call notifiers. */
  ospf_call_notifiers (top->om, OSPF_NOTIFY_NETWORK_NEW, oi, NULL);

  if (if_is_up (ifc->ifp))
    ospf_multi_area_if_up (oi); 


  return oi;
}

/* Get interface for multi area link */
struct ospf_interface *
ospf_multi_area_link_if_new (struct ospf *top,
                             struct ospf_multi_area_link *mlink)
{
  struct apn_vrf *iv = top->ov->iv;
  struct interface *ifp = NULL;
  struct connected *ifc = NULL;
  struct route_node *rn = NULL;
  struct ospf_interface *oi = NULL;
  struct ls_prefix  nbr_prefix;

  if (!IS_PROC_ACTIVE (top))
    return NULL;
  
  ls_prefix_ipv4_set (&nbr_prefix, IPV4_MAX_BITLEN, mlink->nbr_addr);

  /* Check all interfaces and all connected addresses. */
  for (rn = route_top (iv->ifv.if_table); rn; rn = route_next (rn))
    if ((ifp = rn->info))
      if (pal_strcmp (ifp->name, mlink->ifname) == 0) 
        {
          if (ospf_if_disable_all_get (ifp))
            break; 
          else
            {
              /* Check for the subnet corresponds to the neighbor address */ 
              for (ifc = ifp->ifc_ipv4; ifc; ifc = ifc->next)
                {
                  /* Check for matching interface address */
                  if (prefix_match (ifc->address, (struct prefix *)&nbr_prefix))
                    {
                      oi = ospf_multi_area_if_new (top, ifc, mlink->area_id, 
                                                   nbr_prefix);
                      return oi; 
                    }
                }
            } 
          break;
        }
  return NULL; 
} 
            
 
/* Lookup for multi area link */
struct ospf_multi_area_link *
ospf_multi_area_link_entry_lookup (struct ospf *top,  
                              struct pal_in4_addr area_id,
                              struct pal_in4_addr nbr_addr)
{
  struct ls_prefix8 p;
  struct ls_node *rn;

  p.prefixlen = OSPF_MULTI_AREA_IF_TABLE_DEPTH * 8;
  p.prefixsize = OSPF_MULTI_AREA_IF_TABLE_DEPTH;
  ls_prefix_set_args ((struct ls_prefix *)&p, "aa", &area_id, &nbr_addr);

  rn = ls_node_lookup (top->multi_area_link_table, (struct ls_prefix *)&p);
  if (rn)
    {
      ls_unlock_node (rn);
      return RN_INFO (rn, RNI_DEFAULT);
    }

  return NULL;
}

/* Insert multi area link */
void
ospf_multi_area_link_entry_insert (struct ospf *top, 
                                   struct ospf_multi_area_link *mlink)
{
  struct ls_node *rn;
  struct ls_prefix8 p;

  pal_mem_set (&p, 0, sizeof (struct ls_prefix8));
  p.prefixsize = OSPF_MULTI_AREA_IF_TABLE_DEPTH;
  p.prefixlen = OSPF_MULTI_AREA_IF_TABLE_DEPTH * 8;

  ls_prefix_set_args ((struct ls_prefix *)&p, "aa", 
                       &mlink->area_id, &mlink->nbr_addr);

  rn = ls_node_get (top->multi_area_link_table, (struct ls_prefix *)&p);
  if ((RN_INFO (rn, RNI_DEFAULT)) == NULL)
    {
      RN_INFO_SET (rn, RNI_DEFAULT, mlink);
    }

  ls_unlock_node (rn);
  return ;
}

  
    
/* Get multi area link */
struct ospf_multi_area_link *
ospf_multi_area_link_get (struct ospf *top, struct pal_in4_addr area_id,
                               u_char *ifname, struct pal_in4_addr nbr_addr, 
                               u_char format)
{
  struct ospf_multi_area_link *mlink = NULL;
  struct ospf_area *area = NULL;
   
  mlink = ospf_multi_area_link_entry_lookup (top, area_id, nbr_addr);
  if (mlink == NULL)
    {
      area = ospf_area_get (top, area_id);
      if (area == NULL)
        return NULL;
 
      mlink = XMALLOC (MTYPE_OSPF_MULTI_AREA_LINK, 
                       sizeof (struct ospf_multi_area_link)); 
      if (mlink == NULL)
        return NULL;

      mlink->ifname = XSTRDUP (MTYPE_OSPF_DESC, ifname);
      mlink->area_id = area_id;
      mlink->nbr_addr = nbr_addr;
      mlink->format = format;
      mlink->oi = ospf_multi_area_link_if_new (top, mlink);
      if (mlink->oi)
        ospf_if_lock (mlink->oi); 

      ospf_area_lock (area);

      /* Insert to the MLINK entry.  */
      ospf_multi_area_link_entry_insert (top, mlink);

    }
  return mlink;
} 

/* Delete interface from multi area link */
s_int32_t
ospf_multi_area_if_entry_delete (struct ospf *top, struct pal_in4_addr area_id,
                                 struct pal_in4_addr nbr_addr)
{
  struct ls_prefix8 p;
  struct ls_node *rn = NULL;
  struct ospf_multi_area_link *mlink = NULL;

  p.prefixlen = OSPF_MULTI_AREA_IF_TABLE_DEPTH * 8;
  p.prefixsize = OSPF_MULTI_AREA_IF_TABLE_DEPTH;
  ls_prefix_set_args ((struct ls_prefix *)&p, "aa", &area_id, &nbr_addr);

  rn = ls_node_lookup (top->multi_area_link_table, (struct ls_prefix *)&p);
  if (rn)
    {
      mlink = RN_INFO (rn, RNI_DEFAULT);
      if (mlink)
        { 
          ospf_if_unlock (mlink->oi);
          mlink->oi = NULL;
        }
      ls_unlock_node (rn);
   
      return 1;
    }
  return 0;
}

/* Delete multi area link from table */
s_int32_t
ospf_multi_area_link_entry_delete (struct ospf *top, 
                             struct ospf_multi_area_link *mlink)
{
  struct ls_prefix8 p;
  struct ls_node *rn = NULL;

  p.prefixlen = OSPF_MULTI_AREA_IF_TABLE_DEPTH * 8;
  p.prefixsize = OSPF_MULTI_AREA_IF_TABLE_DEPTH;
  ls_prefix_set_args ((struct ls_prefix *)&p, "aa", &mlink->area_id,
                      &mlink->nbr_addr);

  rn = ls_node_lookup (top->multi_area_link_table, (struct ls_prefix *)&p);
  if (rn)
    {
      RN_INFO_UNSET (rn, RNI_DEFAULT);
      ls_unlock_node (rn);
      return 1;
    }
  return 0;
}

/* Delete multi area adjacencies corresponding to primary adjacency */
void
ospf_multi_area_if_entries_del_corr_to_primary_adj (
                           struct ospf_interface *oi_primary)
{
  struct ospf_multi_area_link *mlink = NULL;
  struct ls_node *rn = NULL;
  struct ospf *top = NULL;
  top = oi_primary->top;

  for (rn = ls_table_top (top->multi_area_link_table); rn; 
       rn = ls_route_next (rn))
    {
      if ((mlink = RN_INFO (rn, RNI_DEFAULT)))
        if (mlink->oi != NULL && mlink->oi->oi_primary == oi_primary)
          {
            ospf_if_delete (mlink->oi);
          }
            
    }

}

/* Free Multi-Area link */ 
void 
ospf_multi_area_link_delete (struct ospf *top, 
                                  struct ospf_multi_area_link *mlink)
{
  struct ospf_area *area = NULL;

  area = ospf_area_entry_lookup (top, mlink->area_id);

  /* Delete the interface */
  if (mlink->oi)
      ospf_if_delete (mlink->oi);
  ospf_multi_area_link_entry_delete (top, mlink);

  /* unlock areas */
  if (area)
    ospf_area_unlock (area);

   /* Free Multi-Area Link. */
   ospf_multi_area_link_free (mlink); 
}

/* Delete  multi area adjacencies to all the neighbors on interface with 
   the given area_id */
void
ospf_multi_area_link_delete_all (struct ospf *top,
                                      struct pal_in4_addr area_id,
                                      u_char *ifname)
{
  struct ospf_multi_area_link *mlink = NULL;
  struct ls_node *rn = NULL;
  
  for (rn = ls_table_top (top->multi_area_link_table); rn;
       rn = ls_route_next (rn))
    if ((mlink = RN_INFO (rn, RNI_DEFAULT)))
      if (pal_strcmp (ifname, mlink->ifname) == 0 && 
          IPV4_ADDR_SAME (&area_id, &mlink->area_id))
        ospf_multi_area_link_delete (top, mlink);
} 

/* Multi area link up corresponding to primary link */
void
ospf_multi_area_link_up_corr_to_primary (struct ospf *top, 
                                         struct connected *ifc)
{
  struct ospf_multi_area_link *mlink = NULL;
  struct ls_node *rn = NULL;
  struct ls_prefix nbr_prefix;

  for (rn = ls_table_top (top->multi_area_link_table); rn;
            rn = ls_route_next (rn))
    {
      if ((mlink = RN_INFO (rn, RNI_DEFAULT)))
        if (pal_strcmp (mlink->ifname, ifc->ifp->name) == 0 )
          {
            pal_mem_set (&nbr_prefix, 0, sizeof (struct ls_prefix));  
            ls_prefix_ipv4_set (&nbr_prefix, IPV4_MAX_BITLEN, mlink->nbr_addr); 

            /* Check for matching interface address */
            if (prefix_match (ifc->address, (struct prefix *)&nbr_prefix))
              {
                /* Create interface for multi area link */
                mlink->oi = ospf_multi_area_if_new (top, ifc, mlink->area_id, 
                                                    nbr_prefix);
                if (mlink->oi)
                  ospf_if_lock (mlink->oi);
              }
          }
    } 
          
}

/* Enable ospf interface */
s_int32_t
ospf_multi_area_if_up (struct ospf_interface *oi)
{
  struct ospf_vlink *vlink = NULL;
  struct ls_node *rn = NULL;

  if (!(IS_OSPF_ABR (oi->top)))
    return 0;
 
  /* Check vlink is enabled on interface */ 
  if (IS_AREA_BACKBONE (oi->area))
    for (rn = ls_table_top (oi->top->vlink_table); rn;
         rn = ls_route_next (rn))
      if ((vlink = RN_INFO (rn, RNI_DEFAULT)))
        if (vlink->oi && CHECK_FLAG (vlink->flags, OSPF_VLINK_UP) && 
            IPV4_ADDR_SAME (&vlink->oi->destination->u.prefix4, 
                                &oi->destination->u.prefix4) &&
            IPV4_ADDR_SAME (&vlink->area_id, &oi->oi_primary->area->area_id))   
          return 0;   
   
  ospf_if_up (oi);
  return 1;
}
#endif /* HAVE_OSPF_MULTI_AREA */
