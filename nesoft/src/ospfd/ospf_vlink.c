/* Copyright (C) 2001-2003  All Rights Reserved. */

#include "pal.h"
#include "lib.h"
#include "if.h"
#include "prefix.h"
#include "linklist.h"
#include "log.h"

#include "ospfd/ospfd.h"
#include "ospfd/ospf_interface.h"
#include "ospfd/ospf_vlink.h"
#ifdef HAVE_OSPF_MULTI_AREA
#include "ospfd/ospf_multi_area_link.h"
#endif /* HAVE_OSPF_MULTI_AREA */
#include "ospfd/ospf_lsa.h"
#include "ospfd/ospf_lsdb.h"
#include "ospfd/ospf_abr.h"
#include "ospfd/ospf_spf.h"
#include "ospfd/ospf_route.h"
#include "ospfd/ospf_api.h"
#include "ospfd/ospf_debug.h"


#define OSPF_VLINK_INDEX_MAX   0x7fffffff


int
ospf_vlink_entry_insert (struct ospf *top, struct ospf_vlink *vlink)
{
  struct ls_node *rn;
  struct ls_prefix8 p;
  int ret = OSPF_API_ENTRY_INSERT_ERROR;

  pal_mem_set (&p, 0, sizeof (struct ls_prefix8));
  p.prefixsize = OSPF_VIRT_IF_TABLE_DEPTH;
  p.prefixlen = OSPF_VIRT_IF_TABLE_DEPTH * 8;

  /* Set Virtual Interface Area ID and Virtual Neighbor ID as index. */
  ls_prefix_set_args ((struct ls_prefix *)&p,
                      ospf_api_table_def[OSPF_VIRT_IF_TABLE].vars,
                      &vlink->area_id, &vlink->peer_id);

  rn = ls_node_get (top->vlink_table, (struct ls_prefix *)&p);
  if ((RN_INFO (rn, RNI_DEFAULT)) == NULL)
    {
      ospf_if_lock (vlink->oi);
      RN_INFO_SET (rn, RNI_DEFAULT, vlink);
      ret = OSPF_API_ENTRY_INSERT_SUCCESS;
    }
  ls_unlock_node (rn);

  return ret;
}

int
ospf_vlink_entry_delete (struct ospf *top, struct ospf_vlink *vlink)
{
  struct ls_node *rn;
  struct ls_prefix8 p;

  pal_mem_set (&p, 0, sizeof (struct ls_prefix8));
  p.prefixsize = OSPF_VIRT_IF_TABLE_DEPTH;
  p.prefixlen = OSPF_VIRT_IF_TABLE_DEPTH * 8;

  /* Set Virtual Link Area ID and Virtual Neighbor ID as index. */
  ls_prefix_set_args ((struct ls_prefix *)&p,
                      ospf_api_table_def[OSPF_VIRT_IF_TABLE].vars,
                      &vlink->area_id, &vlink->peer_id);

  rn = ls_node_lookup (top->vlink_table, (struct ls_prefix *)&p);
  if (rn)
    {
      ospf_if_unlock (vlink->oi);
      RN_INFO_UNSET (rn, RNI_DEFAULT);
      ls_unlock_node (rn);
      return OSPF_API_ENTRY_DELETE_SUCCESS;
    }
  return OSPF_API_ENTRY_DELETE_ERROR;
}

struct ospf_vlink *
ospf_vlink_entry_lookup (struct ospf *top, struct pal_in4_addr area_id,
                         struct pal_in4_addr nbr_id)
{
  struct ls_node *rn;

  rn = ospf_api_lookup (top->vlink_table, OSPF_VIRT_IF_TABLE,
                        &area_id, &nbr_id);
  if (rn)
    {
      ls_unlock_node (rn);
      return RN_INFO (rn, RNI_DEFAULT);
    }
  return NULL;
}

struct ospf_vlink *
ospf_vlink_entry_lookup_functional (struct ospf *top,
                                    struct pal_in4_addr area_id)
{
  struct ospf_vlink *vlink;
  struct ls_node *rn, *rn_tmp;
  struct ls_prefix8 lp;

  pal_mem_set (&lp, 0, sizeof (struct ls_prefix8));
  lp.prefixsize = OSPF_VIRT_IF_TABLE_DEPTH;
  lp.prefixlen = 32;
  ls_prefix_set_args ((struct ls_prefix *)&lp, "a", &area_id);

  rn_tmp = ls_node_get (top->vlink_table, (struct ls_prefix *)&lp);

  for (rn = ls_lock_node (rn_tmp); rn; rn = ls_route_next_until (rn, rn_tmp))
    if ((vlink = RN_INFO (rn, RNI_DEFAULT)))
      if (vlink->oi->full_nbr_count > 0)
        {
          ls_unlock_node (rn_tmp);
          ls_unlock_node (rn);
          return vlink;
        }

  ls_unlock_node (rn_tmp);

  return NULL;
}

struct ospf_vlink *
ospf_vlink_lookup_by_name (struct ospf_master *om, char *name)
{
  struct listnode *node;
  struct ospf *top;
  struct ls_node *rn;
  struct ospf_vlink *vlink;

  LIST_LOOP (om->ospf, top, node)
    for (rn = ls_table_top (top->vlink_table); rn; rn = ls_route_next (rn))
      if ((vlink = RN_INFO (rn, RNI_DEFAULT)))
        if (pal_strcmp (vlink->name, name) == 0)
          {
            ls_unlock_node (rn);
            return vlink;
          }

  return NULL;
}

struct ospf_vlink *
ospf_vlink_new (struct ospf_master *om, struct ospf *top)
{
  struct ospf_vlink *vlink;
  char vlink_str[OSPF_VLINK_STRING_MAXLEN];

  if (om->vlink_index == OSPF_VLINK_INDEX_MAX)
    return NULL;

  vlink = XMALLOC (MTYPE_OSPF_VLINK, sizeof (struct ospf_vlink));
  pal_mem_set (vlink, 0, sizeof (struct ospf_vlink));

  /* Set virtual link default values. */
  vlink->index = om->vlink_index++;
  vlink->status = ROW_STATUS_ACTIVE;
  vlink->mtu = OSPF_VLINK_MTU;

  /* Set virtual link name. */
  vlink->name = XSTRDUP (MTYPE_OSPF_DESC, VLINK_NAME (vlink));

  return vlink;
}

void
ospf_vlink_free (struct ospf_vlink *vlink)
{
  if (vlink->oi)
    ospf_if_unlock (vlink->oi);
  if (vlink->out_oi)
    ospf_if_unlock (vlink->out_oi);

  XFREE (MTYPE_OSPF_DESC, vlink->name);
  XFREE (MTYPE_OSPF_VLINK, vlink);
}

struct ospf_interface *
ospf_vlink_if_new (struct ospf *top, struct ospf_vlink *vlink)
{
  struct ospf_interface *oi;
  struct ospf_area *area;

  /* Virtual-Link always link to Backbone. */
  area = ospf_area_get (top, OSPFBackboneAreaID);
  if (area == NULL)
    return NULL;

  oi = XMALLOC (MTYPE_OSPF_IF, sizeof (struct ospf_interface));
  if (oi == NULL)
    return NULL;

  pal_mem_set (oi, 0, sizeof (struct ospf_interface));

  /* Init reference count. */
  ospf_if_lock (oi);

  /* Set the interface type.  */
  oi->type = OSPF_IFTYPE_VIRTUALLINK;

  /* Set references. */
  oi->top = top;
  oi->u.vlink = vlink;

  ospf_area_lock (area);

  /* Initialize interfaces.  */
  ospf_if_init (oi, NULL, area);

  /* Call notifiers. */
  ospf_call_notifiers (top->om, OSPF_NOTIFY_NETWORK_NEW, oi, NULL);

  return oi;
}

struct ospf_vlink *
ospf_vlink_get (struct ospf *top,
                struct pal_in4_addr area_id, struct pal_in4_addr peer_id)
{
  struct ospf_master *om = top->om;
  struct ospf_vlink *vlink;
  struct ospf_area *area;

  if (IS_AREA_ID_BACKBONE (area_id))
    return NULL;

  vlink = ospf_vlink_entry_lookup (top, area_id, peer_id);
  if (vlink == NULL)
    {
      vlink = ospf_vlink_new (om, top);
      if (vlink == NULL)
        return NULL;

      vlink->area_id = area_id;
      vlink->peer_id = peer_id;
      vlink->oi = ospf_vlink_if_new (top, vlink);

      /* Perform NULL check */  
      if (vlink->oi)
        ospf_if_lock (vlink->oi);

      area = ospf_area_get (top, area_id);
      if (area == NULL)
        return NULL;

      ospf_area_lock (area);

      /* Count up VLINK counter.  */
      area->vlink_count++;

      /* Insert to the VLINK entry.  */
      ospf_vlink_entry_insert (top, vlink);

      /* Schedule the SPF calculation.  */
      ospf_spf_calculate_timer_add (area);
    }
  return vlink;
}

int
ospf_vlink_if_up (struct ospf_vlink *vlink)
{
  char vlink_str[OSPF_VLINK_STRING_MAXLEN];
  struct ospf_interface *oi = vlink->oi;
  struct ospf_master *om = oi->top->om;
#ifdef HAVE_OSPF_MULTI_AREA
  struct ospf_multi_area_link *mlink = NULL;
  struct ls_node *rn = NULL;
#endif /* HAVE_OSPF_MULTI_AREA */

  if (CHECK_FLAG (vlink->flags, OSPF_VLINK_UP))
    return 0;

  /* Local and remote address should be selected.  */
  if (oi->destination->u.prefix4.s_addr == 0
      || oi->ident.address->u.prefix4.s_addr == 0)
    return 0;
 
  if (IS_DEBUG_OSPF (event, EVENT_VLINK))
    zlog_info (ZG, "VLINK[%s]: peer %r link up",
               VLINK_NAME (vlink), &vlink->peer_id);

#ifdef HAVE_OSPF_MULTI_AREA
  /* Check whether multi area link enabled on back bone area If enabled no need 
     to activate virtual link on interface */
  for (rn = ls_table_top (oi->top->multi_area_link_table); rn;
                          rn = ls_route_next (rn))
    if ((mlink = RN_INFO (rn, RNI_DEFAULT)))
      if (mlink->oi && IS_AREA_BACKBONE (mlink->oi->area) && 
          CHECK_FLAG (mlink->oi->flags, OSPF_IF_UP) && 
          IPV4_ADDR_SAME (&mlink->oi->destination->u.prefix4, 
                         &oi->destination->u.prefix4) &&
          IPV4_ADDR_SAME (&vlink->area_id,
                          &mlink->oi->oi_primary->area->area_id))
        return 0; 
        
#endif /* HAVE_OSPF_MULTI_AREA */

  SET_FLAG (vlink->flags, OSPF_VLINK_UP);

  /* Up the interface.  */
  ospf_if_up (oi);

  return 1;
}

int
ospf_vlink_if_down (struct ospf_vlink *vlink)
{
  char vlink_str[OSPF_VLINK_STRING_MAXLEN];
  struct ospf_interface *oi = vlink->oi;
  struct ospf_master *om = oi->top->om;
#ifdef HAVE_OSPF_MULTI_AREA
  struct ospf_multi_area_link *mlink = NULL;
  struct ls_node *rn = NULL;
#endif

  if (!CHECK_FLAG (vlink->flags, OSPF_VLINK_UP))
    return 0;

  if (IS_DEBUG_OSPF (event, EVENT_VLINK))
    zlog_info (ZG, "VLINK[%s]: peer %r link down",
               VLINK_NAME (vlink), &vlink->peer_id);

  UNSET_FLAG (vlink->flags, OSPF_VLINK_UP);

  /* Down the interface.  */
  ospf_if_down (oi);

#ifdef HAVE_OSPF_MULTI_AREA
  /* Enable multi area link on back bone area after vlink deletion */
  for (rn = ls_table_top (oi->top->multi_area_link_table); rn;
       rn = ls_route_next (rn))
    if ((mlink = RN_INFO (rn, RNI_DEFAULT)))
      if (mlink->oi && IS_AREA_BACKBONE (mlink->oi->area) && 
          (IPV4_ADDR_SAME (&vlink->area_id, 
           &mlink->oi->oi_primary->area->area_id)) &&
          (!CHECK_FLAG (mlink->oi->flags, OSPF_IF_UP)))
        ospf_multi_area_if_up (mlink->oi);
#endif /* HAVE_OSPF_MULTI_AREA */

  /* Reset the outgoing interface.  */
  if (oi->ident.address->u.prefix4.s_addr == 0)
    if (vlink->out_oi != NULL)
      {
        ospf_if_unlock (vlink->out_oi);
        vlink->out_oi = NULL;
      }
  return 1;
}

int
ospf_vlink_if_update (struct ospf_vlink *vlink)
{
  int changed = 0;

  /* Down and up the interface.  */
  changed += ospf_vlink_if_down (vlink);
  changed += ospf_vlink_if_up (vlink);

  return changed;
}

void
ospf_vlink_params_reset (struct ospf_vlink *vlink)
{
  vlink->vertex = NULL;
  vlink->oi->output_cost = 0;
  vlink->oi->ident.address->prefixlen = 0;
  vlink->oi->ident.address->u.prefix4.s_addr = 0;
  vlink->oi->destination->prefixlen = 0;
  vlink->oi->destination->u.prefix4.s_addr = 0;
}

void
ospf_vlink_if_delete (struct ospf_area *transit, struct ospf_vlink *vlink)
{
  /* Count down VLINK counter.  */
  transit->vlink_count--;

  ospf_vlink_params_reset (vlink);
  ospf_vlink_if_down (vlink);
  ospf_if_delete (vlink->oi);

  /* Delete info from virtual interface table. */
  ospf_if_params_delete_default (transit->top->om, vlink->name);
}

void
ospf_vlink_delete (struct ospf *top, struct ospf_vlink *vlink)
{
  struct ospf_area *transit = ospf_area_entry_lookup (top, vlink->area_id);

  if (transit == NULL)
    return;

  /* Remove the virtual link interface.  */
  ospf_vlink_if_delete (transit, vlink);

  /* Unlock areas. */
  ospf_area_unlock (top->backbone);
  ospf_area_unlock (transit);

  /* Free Virtual Link. */
  ospf_vlink_free (vlink);
}

int
ospf_vlink_set_metric (struct ospf_vlink *vlink, int cost)
{
  if (vlink->oi->output_cost != cost)
    {
      vlink->oi->output_cost = cost;
      return 1;
    }
  return 0;
}

int
ospf_vlink_outgoing_if_update (struct ospf_vlink *vlink,
                               struct ospf_interface *oi)
{
  char vlink_str[OSPF_VLINK_STRING_MAXLEN];
  char if_str[OSPF_IF_STRING_MAXLEN];
  struct ospf_master *om = vlink->oi->top->om;

  if (vlink->out_oi == oi)
    return 0;

  if (IS_DEBUG_OSPF (event, EVENT_VLINK))
    zlog_info (ZG, "VLINK[%s]: outgoing interface is changed from %s to %s",
               VLINK_NAME (vlink),
               vlink->out_oi ? IF_NAME (vlink->out_oi) : "(null)",
               IF_NAME (oi));

  /* Shutdown the VLINK interface first.  */
  ospf_vlink_if_down (vlink);

  /* Unset the old outgoing interface.  */
  if (vlink->out_oi != NULL)
    ospf_if_unlock (vlink->out_oi);

  /* Set the new outgoing interface.  */
  vlink->out_oi = ospf_if_lock (oi);

  /* Up the VLINK interface.  */
  ospf_vlink_if_up (vlink);

  return 1;
}

struct prefix *
ospf_vlink_local_address_get (struct ospf_area *area,
                              struct ospf_vlink *vlink)
{
  struct ospf_interface *oi;
  struct listnode *node;

  if ((oi = vlink->out_oi) != NULL)
    if (CHECK_FLAG (oi->flags, OSPF_IF_UP))
      {
        /* Get from the specified outgoing interface.  */
        if (!IS_OSPF_IPV4_UNNUMBERED (oi))
          {
            if (oi->ident.address != NULL)
              return oi->ident.address;
          }
        else
          LIST_LOOP (area->iflist, oi, node)
            if (CHECK_FLAG (oi->flags, OSPF_IF_UP))
              if (!IS_OSPF_IPV4_UNNUMBERED (oi))
                if (oi->ident.address != NULL)
                  return oi->ident.address;
      }

  return NULL;
}

int
ospf_vlink_set_local_address (struct ospf_vlink *vlink)
{
  char vlink_str[OSPF_VLINK_STRING_MAXLEN];
  struct ospf_interface *oi = vlink->oi;
  struct ospf_master *om = oi->top->om;
  struct prefix *q = oi->ident.address;
  struct ospf_area *area = NULL;
  struct prefix *p = NULL;

  area = ospf_area_entry_lookup (oi->top, vlink->area_id);

  if (area == NULL)
    return 0;

  /* Get the local address from the area.  */
  p = ospf_vlink_local_address_get (area, vlink);
  if (p == NULL)
    /* Reset the local address.  */
    ospf_vlink_params_reset (vlink);
  else
    {
      if (IPV4_ADDR_SAME (&q->u.prefix4, &p->u.prefix4))
        {
          /* Return 1 when the interface is not up.  */
          if (!CHECK_FLAG (vlink->flags, OSPF_IF_UP))
            return 1;
          else
            return 0;
        }

      /* Set the new address. */
      q->prefixlen = IPV4_MAX_PREFIXLEN;
      q->u.prefix4 = p->u.prefix4;
    }

  if (IS_DEBUG_OSPF (event, EVENT_VLINK))
    zlog_info (ZG, "VLINK[%s]: local address is %r",
               VLINK_NAME (vlink), &q->u.prefix4);

  return 1;
}

/* Return 1 if destination address is still valid.  */
int
ospf_vlink_remote_address_check (struct ospf_vlink *vlink,
                                 struct ospf_vertex *v)
{
  struct prefix *p = vlink->oi->destination;
  struct ospf_link_desc link;
  struct ospf_vertex *pv;
  u_char *lim;
  u_char *ptr;
  int i;

  if (p->u.prefix4.s_addr == 0)
    return 0;

  for (i = 0; i < vector_max (v->parent); i++)
    if ((pv = vector_slot (v->parent, i)))
      {
        ptr = (u_char *)v->lsa->data + OSPF_ROUTER_LSA_LINK_DESC_OFFSET;
        lim = (u_char *)v->lsa->data + pal_ntoh16 (v->lsa->data->length);

        while (ptr + OSPF_LINK_DESC_SIZE <= lim)
          {
            OSPF_LINK_DESC_GET (ptr, link);

            ptr += OSPF_LINK_DESC_SIZE + OSPF_LINK_TOS_SIZE * link.num_tos;

            if (link.type == LSA_LINK_TYPE_TRANSIT
                || link.type == LSA_LINK_TYPE_POINTOPOINT)
              if (IPV4_ADDR_SAME (&pv->lsa->data->id, link.id))
                if (IPV4_ADDR_SAME (&p->u.prefix4, link.data))
                  return 1;
          }
      }

  return 0;
}

int
ospf_vlink_remote_address_get (struct ospf_vlink *vlink, struct ospf_vertex *v)
{
  struct prefix *p = vlink->oi->destination;
  struct pal_in4_addr *addr = NULL;
  struct ospf_link_desc link;
  struct ospf_vertex *pv;
  u_char *lim;
  u_char *ptr;
  int i;

  for (i = 0; i < vector_max (v->parent); i++)
    if ((pv = vector_slot (v->parent, i)))
      {
        ptr = (u_char *)v->lsa->data + OSPF_ROUTER_LSA_LINK_DESC_OFFSET;
        lim = (u_char *)v->lsa->data + pal_ntoh16 (v->lsa->data->length);

        while (ptr + OSPF_LINK_DESC_SIZE <= lim)
          {
            OSPF_LINK_DESC_GET (ptr, link);

            ptr += OSPF_LINK_DESC_SIZE + OSPF_LINK_TOS_SIZE * link.num_tos;

            if (link.type == LSA_LINK_TYPE_TRANSIT
                || link.type == LSA_LINK_TYPE_POINTOPOINT)
              {
                if (IPV4_ADDR_SAME (&pv->lsa->data->id, link.id))
                  {
                    p->prefixlen = IPV4_MAX_PREFIXLEN;
                    p->u.prefix4 = *link.data;
                    return 1;
                  }
                else if (addr == NULL)
                  addr = link.data;
              }
          }
      }

  if (addr == NULL)
    ospf_vlink_params_reset (vlink);
  else
    {
      p->prefixlen = IPV4_MAX_PREFIXLEN;
      p->u.prefix4 = *addr;
    }
  return 1;
}

int
ospf_vlink_set_remote_address (struct ospf_vlink *vlink, struct ospf_vertex *v)
{
  char vlink_str[OSPF_VLINK_STRING_MAXLEN];
  struct ospf_interface *oi = vlink->oi;
  struct ospf_master *om = oi->top->om;

  if (!ospf_vlink_remote_address_check (vlink, v))
    {
      /* Get the remote IPv4 address.  */
      ospf_vlink_remote_address_get (vlink, v);

      if (IS_DEBUG_OSPF (event, EVENT_VLINK))
        zlog_info (ZG, "VLINK[%s]: remote address is %r",
                   VLINK_NAME (vlink), &oi->destination->u.prefix4);

      return 1;
    }

  /* Return 1 when the interface is not up.  */
  if (!CHECK_FLAG (vlink->flags, OSPF_VLINK_UP))
    return 1;
  else
    return 0;
}

int
ospf_vlink_params_set (struct ospf_vlink *vlink, struct ospf_vertex *v)
{
  int metric_changed = 0;
  int changed = 0;

  /* Update the vertex pointer.  */
  vlink->vertex = v;

  /* Note any change in IP address for regenerating backbone
     router LSA - Link data field changes */
  metric_changed += ospf_vlink_set_metric (vlink, v->distance);
  changed += ospf_vlink_set_local_address (vlink);
  changed += ospf_vlink_set_remote_address (vlink, v);

  /* Originate new router-LSA for Backbone if any cost or IP address change. */
  if (changed || metric_changed)
    LSA_REFRESH_TIMER_ADD (vlink->oi->top, OSPF_ROUTER_LSA,
                           ospf_router_lsa_self (vlink->oi->area),
                           vlink->oi->area);

  return changed;
}

void
ospf_vlink_local_address_update_by_interface (struct ospf_interface *oi)
{
  struct ospf_vlink *vlink;
  struct ls_node *rn, *rn_tmp;
  struct ls_prefix8 lp;

  if (IS_AREA_BACKBONE (oi->area))
    return;

  pal_mem_set (&lp, 0, sizeof (struct ls_prefix8));
  lp.prefixsize = OSPF_VIRT_IF_TABLE_DEPTH;
  lp.prefixlen = 32;
  ls_prefix_set_args ((struct ls_prefix *)&lp, "a", &oi->area->area_id);

  rn_tmp = ls_node_get (oi->area->top->vlink_table, (struct ls_prefix *)&lp);

  for (rn = ls_lock_node (rn_tmp); rn; rn = ls_route_next_until (rn, rn_tmp))
    if ((vlink = RN_INFO (rn, RNI_DEFAULT)) != NULL)
      if (vlink->out_oi == oi)
        if (ospf_vlink_set_local_address (vlink))
          ospf_vlink_if_update (vlink);

  ls_unlock_node (rn_tmp);
}

void
ospf_vlink_status_update (struct ospf_area *area)
{
  struct ospf *top = area->top;
  struct ospf_vlink *vlink;
  struct ospf_vertex *v;
  struct ospf_nexthop *nh;
  struct ls_node *rn, *rn_tmp;
  struct ls_prefix8 lp;
  int i;

  pal_mem_set (&lp, 0, sizeof (struct ls_prefix8));
  lp.prefixsize = OSPF_VIRT_IF_TABLE_DEPTH;
  lp.prefixlen = 32;
  ls_prefix_set_args ((struct ls_prefix *)&lp, "a", &area->area_id);

  rn_tmp = ls_node_get (top->vlink_table, (struct ls_prefix *)&lp);

  for (rn = ls_lock_node (rn_tmp); rn; rn = ls_route_next_until (rn, rn_tmp))
    if ((vlink = RN_INFO (rn, RNI_DEFAULT)))
      {
        nh = NULL;

        v = ospf_spf_vertex_lookup (area, vlink->peer_id);
        if (v != NULL)
          for (i = 0; i < vector_max (v->nexthop); i++)
            if ((nh = vector_slot (v->nexthop, i)))
              if (!CHECK_FLAG (nh->flags, OSPF_NEXTHOP_INVALID))
                break;

        if (v == NULL || nh == NULL)
          {
            /* Shutdown the VLINK.  */
            ospf_vlink_params_reset (vlink);
            ospf_vlink_if_down (vlink);
          }
        else
          {
            /* Update the outgoing interface.  */
            ospf_vlink_outgoing_if_update (vlink, nh->oi);

            if (ospf_vlink_params_set (vlink, v))
              ospf_vlink_if_update (vlink);
          }
      }

  ls_unlock_node (rn_tmp);
}
