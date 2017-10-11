/* Copyright (C) 2001-2003  All Rights Reserved. */

#include "pal.h"
#include "lib.h"
#include "if.h"
#include "prefix.h"
#include "thread.h"
#include "linklist.h"
#include "log.h"

#include "ospfd/ospfd.h"
#include "ospfd/ospf_interface.h"
#include "ospfd/ospf_vlink.h"
#ifdef HAVE_OSPF_MULTI_AREA
#include "ospfd/ospf_multi_area_link.h"
#endif /* HAVE_OSPF_MULTI_AREA */
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
#ifdef HAVE_RESTART
#include "ospfd/ospf_restart.h"
#endif /* HAVE_RESTART */
#ifdef HAVE_OSPF_TE
#include "ospfd/ospf_te.h"
#endif /* HAVE_OSPF_TE */

#ifdef HAVE_BFD
#include "ospfd/ospf_bfd.h"
#endif /* HAVE_BFD */

char ospf_null_password[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };

struct ospf_crypt_key ospf_crypt_key_default =
{
  0, 0, "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0", NULL
};

/* Prototype. */
void ospf_if_params_free (struct ospf_if_params *);


unsigned int
ospf_address_less_ifindex (struct ospf_interface *oi)
{
  if (oi != NULL)
    if (IS_OSPF_IPV4_UNNUMBERED (oi))
      return oi->u.ifp->ifindex;

  return 0;
}

/* if_table manipulation. */
int
ospf_if_entry_insert (struct ospf *top, struct ospf_interface *oi,
                      struct connected *ifc)
{
  unsigned int ifindex = ospf_address_less_ifindex (oi);
  int ret = OSPF_API_ENTRY_INSERT_ERROR;
  struct pal_in4_addr *addr;
  struct ls_prefix8 p;
  struct ls_node *rn;

  pal_mem_set (&p, 0, sizeof (struct ls_prefix8));
  p.prefixsize = OSPF_IF_TABLE_DEPTH;
  p.prefixlen = OSPF_IF_TABLE_DEPTH * 8;

  /* Get ifindex only if interface address is not set. */
  if (ifindex)
    addr = &IPv4AddressUnspec;
  else
    addr = &oi->ident.address->u.prefix4;

  /* Set IP Address and ifIndex as index. */
  ls_prefix_set_args ((struct ls_prefix *)&p,
                      ospf_api_table_def[OSPF_IF_TABLE].vars, addr, &ifindex);

  rn = ls_node_get (top->if_table, (struct ls_prefix *)&p);
  if ((RN_INFO (rn, RNI_OSPF_IF)) == NULL)
    {
      RN_INFO_SET (rn, RNI_OSPF_IF, ospf_if_lock (oi));
      RN_INFO_SET (rn, RNI_CONNECTED, ifc);
      ret = OSPF_API_ENTRY_INSERT_SUCCESS;
    }
  ls_unlock_node (rn);

  return ret;
}

int
ospf_if_entry_delete (struct ospf *top, struct ospf_interface *oi)
{
  unsigned int ifindex = ospf_address_less_ifindex (oi);
  int ret = OSPF_API_ENTRY_DELETE_ERROR;
  struct pal_in4_addr *addr;
  struct ls_prefix8 p;
  struct ls_node *rn;

  pal_mem_set (&p, 0, sizeof (struct ls_prefix8));
  p.prefixsize = OSPF_IF_TABLE_DEPTH;
  p.prefixlen = OSPF_IF_TABLE_DEPTH * 8;

  /* Get ifindex only if interface address is not set. */
  if (ifindex)
    addr = &IPv4AddressUnspec;
  else
    addr = &oi->ident.address->u.prefix4;

  /* Set IP Address and ifIndex as index. */
  ls_prefix_set_args ((struct ls_prefix *)&p,
                      ospf_api_table_def[OSPF_IF_TABLE].vars, addr, &ifindex);

  rn = ls_node_lookup (top->if_table, (struct ls_prefix *)&p);
  if (rn)
    {
      oi = RN_INFO_UNSET (rn, RNI_OSPF_IF);
      if (oi != NULL)
        ospf_if_unlock (oi);
      RN_INFO_UNSET (rn, RNI_OSPF_IF);
      RN_INFO_UNSET (rn, RNI_CONNECTED);
      ls_unlock_node (rn);
      ret = OSPF_API_ENTRY_DELETE_SUCCESS;
    }
  return ret;
}

struct ospf_interface *
ospf_if_entry_lookup (struct ospf *top, struct pal_in4_addr addr,
                      u_int32_t ifindex)
{
  struct ls_node *rn;

  rn = ospf_api_lookup (top->if_table, OSPF_IF_TABLE, &addr, &ifindex);
  if (rn)
    {
      ls_unlock_node (rn);
      return RN_INFO (rn, RNI_OSPF_IF);
    }
  return NULL;
}

#ifdef HAVE_OSPF_MULTI_INST
struct ospf_interface *
ospf_if_entry_match (struct ospf *top, struct ospf_interface *oi)
{
  struct ls_prefix8 p;
  struct ls_node *rn = NULL, *rn_tmp = NULL;
  struct pal_in4_addr *addr = NULL;
  unsigned int ifindex = ospf_address_less_ifindex (oi);

  pal_mem_set (&p, 0, sizeof (struct ls_prefix8));
  p.prefixsize = OSPF_IF_TABLE_DEPTH;
  p.prefixlen = OSPF_IF_TABLE_DEPTH * 8;

  /* Get ifindex only if interface address is not set. */
  if (ifindex)
    addr = &IPv4AddressUnspec;
  else
    addr = &oi->ident.address->u.prefix4;

  /* Set IP Address and ifIndex as index. */
  ls_prefix_set_args ((struct ls_prefix *)&p,
                      ospf_api_table_def[OSPF_IF_TABLE].vars, addr, &ifindex);
 
  rn_tmp = ls_node_get (top->if_table, (struct ls_prefix *)&p);
  for (rn = ls_lock_node (rn_tmp); rn; rn = ls_route_next_until (rn, rn_tmp))
    if ((oi = RN_INFO (rn, RNI_DEFAULT)))
      return oi;
  
  return NULL;
}

void
ospf_global_if_entry_add_next (struct ospf_master *om, 
                               struct ospf_interface *oi_other)
{
  struct ospf_interface *oi_alt = NULL, *oi_glob = NULL;
  struct ospf *top_alt = NULL;
  struct listnode *node = NULL;
  struct pal_in4_addr prefix = oi_other->ident.address->u.prefix4;
  u_int32_t ifindex = oi_other->u.ifp->ifindex;
  u_int8_t count = 0;

  /* Check if multiple instances are running on this interface */
  LIST_LOOP (om->ospf, top_alt, node)
    {
      if ((oi_alt = ospf_if_entry_match (top_alt, oi_other)))
        {
          /* Add the next first oi entry to global table */
          if (count == 0)
            {
              ospf_global_if_entry_add (om, prefix, ifindex, oi_alt);
              oi_glob = oi_alt;
            }
          count++;
          /* If more than one process are running on the subnet 
             then set the flag */
          if (count > 1)
            {
              SET_FLAG (oi_glob->flags, OSPF_IF_EXT_MULTI_INST);
              break;
            }
        }
    }
}
#endif /* HAVE_OSPF_MULTI_INST */

/* OSPF global interface table related functions. */
int
ospf_global_if_entry_add (struct ospf_master *om, struct pal_in4_addr addr,
                          u_int32_t ifindex, struct ospf_interface *oi)
{
  struct ls_prefix8 p;
  struct ls_node *rn;
  int ret = 0;
  struct ospf_interface *oi_glob = NULL;

  p.prefixlen = OSPF_GLOBAL_IF_TABLE_DEPTH * 8;
  p.prefixsize = OSPF_GLOBAL_IF_TABLE_DEPTH;
  ls_prefix_set_args ((struct ls_prefix *)&p, "ia", &ifindex, &addr);

  rn = ls_node_get (om->if_table, (struct ls_prefix *)&p);
  if ((oi_glob = RN_INFO (rn, RNI_DEFAULT)) == NULL)
    {
      RN_INFO_SET (rn, RNI_DEFAULT, ospf_if_lock (oi));
      ret = 1;
    }
#ifdef HAVE_OSPF_MULTI_INST
  else
    SET_FLAG (oi_glob->flags, OSPF_IF_EXT_MULTI_INST);
#endif /* HAVE_OSPF_MULTI_INST */

  ls_unlock_node (rn);
  return ret;
}

int
ospf_global_if_entry_delete (struct ospf_master *om, struct pal_in4_addr addr,
                             u_int32_t ifindex)
{
  struct ospf_interface *oi;
  struct ls_prefix8 p;
  struct ls_node *rn;

  p.prefixlen = OSPF_GLOBAL_IF_TABLE_DEPTH * 8;
  p.prefixsize = OSPF_GLOBAL_IF_TABLE_DEPTH;
  ls_prefix_set_args ((struct ls_prefix *)&p, "ia", &ifindex, &addr);

  rn = ls_node_lookup (om->if_table, (struct ls_prefix *)&p);
  if (rn)
    {
      oi = RN_INFO (rn, RNI_DEFAULT);
      if (oi != NULL)
        ospf_if_unlock (oi);
      RN_INFO_UNSET (rn, RNI_DEFAULT);
      ls_unlock_node (rn);
      return 1;
    }
  return 0;
}

struct ospf_interface *
ospf_global_if_entry_lookup (struct ospf_master *om, struct pal_in4_addr addr,
                             u_int32_t ifindex)
{
  struct ls_prefix8 p;
  struct ls_node *rn;

  p.prefixlen = OSPF_GLOBAL_IF_TABLE_DEPTH * 8;
  p.prefixsize = OSPF_GLOBAL_IF_TABLE_DEPTH;
  ls_prefix_set_args ((struct ls_prefix *)&p, "ia", &ifindex, &addr);

  rn = ls_node_lookup (om->if_table, (struct ls_prefix *)&p);
  if (rn)
    {
      ls_unlock_node (rn);
      return RN_INFO (rn, RNI_DEFAULT);
    }

  return NULL;
}

struct ospf_interface *
ospf_area_if_lookup_by_prefix (struct ospf_area *area, struct prefix *p)
{
  struct ospf_interface *oi;
  struct prefix q;
  struct listnode *node;

  LIST_LOOP (area->iflist, oi, node)
    {
      prefix_copy (&q, oi->ident.address);
      apply_mask (&q);

      if (prefix_same (p, &q))
        return oi;
    }

  return NULL;
}


/* OSPF crypt key related functions. */
struct ospf_crypt_key *
ospf_crypt_key_new ()
{
  struct ospf_crypt_key *ck;

  ck = XMALLOC (MTYPE_OSPF_CRYPT_KEY, sizeof (struct ospf_crypt_key));
  pal_mem_set (ck, 0, sizeof (struct ospf_crypt_key));

  return ck;
}

void
ospf_crypt_key_free (struct ospf_crypt_key *ck)
{
  OSPF_TIMER_OFF (ck->t_passive);
  XFREE (MTYPE_OSPF_CRYPT_KEY, ck);
}

struct ospf_crypt_key *
ospf_crypt_key_lookup (struct list *crypt, u_char key_id)
{
  struct listnode *node;
  struct ospf_crypt_key *ck;

  for (node = LISTHEAD (crypt); node; NEXTNODE (node))
    {
      ck = GETDATA (node);
      if (!ck)
        continue;

        if (ck->key_id == key_id)
        return ck;
    }

  return NULL;
}

int
ospf_crypt_key_cmp (void *v1, void *v2)
{
  struct ospf_crypt_key *ck1 = v1, *ck2 = v2;
  return ck1->key_id - ck2->key_id;
}

void
ospf_crypt_key_add (struct list *crypt, struct ospf_crypt_key *ck)
{
  listnode_add (crypt, ck);
}

void
ospf_crypt_key_delete (struct list *crypt, u_char key_id)
{
  struct ospf_crypt_key *ck;

  ck = ospf_crypt_key_lookup (crypt, key_id);

  if (ck)
    {
      listnode_delete (crypt, ck);
      ospf_crypt_key_free (ck);
    }
}

void
ospf_crypt_key_delete_all (struct list *crypt)
{
  struct listnode *node, *next;

  for (node = LISTHEAD (crypt); node; node = next)
    {
      next = node->next;
      ospf_crypt_key_free (node->data);
      list_delete_node (crypt, node);
    }
}


/* General get functions. */
int
ospf_if_mtu_get (struct ospf_interface *oi)
{
  if (oi->type == OSPF_IFTYPE_VIRTUALLINK)
    return oi->u.vlink->mtu;

  if (OSPF_IF_PARAM_CHECK (oi->params_default, MTU))
    return oi->params_default->mtu;

  return oi->u.ifp->mtu;
}

int
ospf_if_mtu_ignore_get (struct ospf_interface *oi)
{
  if (OSPF_IF_PARAM_CHECK (oi->params_default, MTU_IGNORE))
    return 1;

  return 0;
}

u_char
ospf_if_network_get (struct ospf_interface *oi)
{
  if (OSPF_IF_PARAM_CHECK (oi->params_default, NETWORK_TYPE))
    return oi->params_default->type;

  return OSPF_IFTYPE_NONE;
}

u_char
ospf_if_network_get_any (struct ospf_interface *oi)
{
  u_char type;

  if (oi->type == OSPF_IFTYPE_VIRTUALLINK)
    return OSPF_IFTYPE_VIRTUALLINK;
  else if (if_is_loopback (oi->u.ifp))
    return OSPF_IFTYPE_LOOPBACK;
#ifdef HAVE_OSPF_MULTI_AREA
  else if (CHECK_FLAG (oi->flags, OSPF_MULTI_AREA_IF))
    return OSPF_IFTYPE_POINTOPOINT;
#endif /* HAVE_OSPF_MULTI_AREA */
  else if (oi->type == OSPF_IFTYPE_HOST)
    return OSPF_IFTYPE_HOST;
  else
    {
      type = ospf_if_network_get (oi);
      if (type != OSPF_IFTYPE_NONE)
        return type;
      else if (if_is_pointopoint (oi->u.ifp))
        return OSPF_IFTYPE_POINTOPOINT;
    }
  return OSPF_IFTYPE_BROADCAST;
}

u_char
ospf_if_priority_get (struct ospf_interface *oi)
{
  if (OSPF_IF_PARAM_CHECK (oi->params, PRIORITY))
    return oi->params->priority;
  if (OSPF_IF_PARAM_CHECK (oi->params_default, PRIORITY))
    return oi->params_default->priority;

  return OSPF_ROUTER_PRIORITY_DEFAULT;
}

u_int32_t
ospf_if_transmit_delay_get (struct ospf_interface *oi)
{
  if (OSPF_IF_PARAM_CHECK (oi->params, TRANSMIT_DELAY))
    return oi->params->transmit_delay;
  else if (OSPF_IF_PARAM_CHECK (oi->params_default, TRANSMIT_DELAY))
    return oi->params_default->transmit_delay;

  return OSPF_TRANSMIT_DELAY_DEFAULT;
}

u_int32_t
ospf_if_retransmit_interval_get (struct ospf_interface *oi)
{
  if (OSPF_IF_PARAM_CHECK (oi->params, RETRANSMIT_INTERVAL))
    return oi->params->retransmit_interval;
  if (OSPF_IF_PARAM_CHECK (oi->params_default, RETRANSMIT_INTERVAL))
    return oi->params_default->retransmit_interval;

  return OSPF_RETRANSMIT_INTERVAL_DEFAULT;
}

u_int32_t
ospf_if_hello_interval_get (struct ospf_interface *oi)
{
  if (OSPF_IF_PARAM_CHECK (oi->params, HELLO_INTERVAL))
    return oi->params->hello_interval;
  if (OSPF_IF_PARAM_CHECK (oi->params_default, HELLO_INTERVAL))
    return oi->params_default->hello_interval;

  return OSPF_IF_PARAM_HELLO_INTERVAL_DEFAULT (oi->params_default);
}

u_int32_t
ospf_if_dead_interval_get (struct ospf_interface *oi)
{
  if (OSPF_IF_PARAM_CHECK (oi->params, DEAD_INTERVAL))
    return oi->params->dead_interval;
  if (OSPF_IF_PARAM_CHECK (oi->params_default, DEAD_INTERVAL))
    return oi->params_default->dead_interval;

  if (OSPF_IF_PARAM_CHECK (oi->params, HELLO_INTERVAL))
    return OSPF_IF_PARAM_DEAD_INTERVAL_DEFAULT (oi->params);
  else
    return OSPF_IF_PARAM_DEAD_INTERVAL_DEFAULT (oi->params_default);
}

u_int32_t
ospf_if_output_cost_get (struct ospf_interface *oi)
{
  float bw = oi->u.ifp->bandwidth
    ? oi->u.ifp->bandwidth / 1000 * 8 : OSPF_DEFAULT_BANDWIDTH;
  float refbw = oi->top->ref_bandwidth;
  u_int32_t cost;

  /* A specifed configured cost overrides a calculated one.  */
  if (OSPF_IF_PARAM_CHECK (oi->params, OUTPUT_COST))
    cost = oi->params->output_cost;
  else if (OSPF_IF_PARAM_CHECK (oi->params_default, OUTPUT_COST))
    cost = oi->params_default->output_cost;
  /* See if a cost can be calculated from the NSM processes
     interface bandwidth field. */
  else
    {
      cost = (u_int32_t) (refbw / bw);
      if (cost < OSPF_OUTPUT_COST_MIN)
        cost = OSPF_OUTPUT_COST_MIN;
      else if (cost > OSPF_OUTPUT_COST_MAX)
        cost = OSPF_OUTPUT_COST_MAX;
    }
  return cost;
}

#ifdef HAVE_OSPF_TE
u_int32_t
ospf_if_te_metric_get (struct ospf_interface *oi)
{
  if (OSPF_IF_PARAM_CHECK (oi->params, TE_METRIC))
    return oi->params->te_metric;
  else if (OSPF_IF_PARAM_CHECK (oi->params_default, TE_METRIC))
    return oi->params_default->te_metric;
  else
    return oi->output_cost;
}
#endif /* HAVE_OSPF_TE */

int
ospf_if_authentication_type_get (struct ospf_interface *oi)
{
  if (OSPF_IF_PARAM_CHECK (oi->params, AUTH_TYPE))
    return oi->params->auth_type;
  if (OSPF_IF_PARAM_CHECK (oi->params_default, AUTH_TYPE))
    return oi->params_default->auth_type;

  return OSPF_AUTH_NULL;
}

char *
ospf_if_authentication_key_get (struct ospf_interface *oi)
{
  if (OSPF_IF_PARAM_CHECK (oi->params, AUTH_SIMPLE))
    return oi->params->auth_simple;
  if (OSPF_IF_PARAM_CHECK (oi->params_default, AUTH_SIMPLE))
    return oi->params_default->auth_simple;

  return ospf_null_password;
}

struct ospf_crypt_key *
ospf_if_message_digest_key_get (struct ospf_interface *oi, u_char key_id)
{
  if (OSPF_IF_PARAM_CHECK (oi->params, AUTH_CRYPT))
    return ospf_crypt_key_lookup (oi->params->auth_crypt, key_id);
  if (OSPF_IF_PARAM_CHECK (oi->params_default, AUTH_CRYPT))
    return ospf_crypt_key_lookup (oi->params_default->auth_crypt, key_id);

  if (key_id == 0)
    return &ospf_crypt_key_default;

  return NULL;
}

struct ospf_crypt_key *
ospf_if_message_digest_key_get_last (struct ospf_interface *oi)
{
  if (OSPF_IF_PARAM_CHECK (oi->params, AUTH_CRYPT))
    return oi->params->auth_crypt->tail->data;
  if (OSPF_IF_PARAM_CHECK (oi->params_default, AUTH_CRYPT))
    return oi->params_default->auth_crypt->tail->data;

  return &ospf_crypt_key_default;
}

int
ospf_if_database_filter_get (struct ospf_interface *oi)
{
  if (OSPF_IF_PARAM_CHECK (oi->params, DATABASE_FILTER))
    return 1;
  if (OSPF_IF_PARAM_CHECK (oi->params_default, DATABASE_FILTER))
    return 1;

  return 0;
}

int
ospf_if_disable_all_get (struct interface *ifp)
{
  struct ospf_master *om = ifp->vr->proto;
  struct ospf_if_params *oip;

  oip = ospf_if_params_lookup_default (om, ifp->name);
  if (oip)
    return OSPF_IF_PARAM_CHECK (oip, DISABLE_ALL);

  return 0;
}

#ifdef HAVE_RESTART
u_int32_t
ospf_if_resync_timeout_get (struct ospf_interface *oi)
{
  u_int32_t dead_interval;
  u_int32_t resync_timeout;

  dead_interval = ospf_if_dead_interval_get (oi);

  if (OSPF_IF_PARAM_CHECK (oi->params, RESYNC_TIMEOUT))
    resync_timeout = oi->params->resync_timeout;
  else if (OSPF_IF_PARAM_CHECK (oi->params_default, RESYNC_TIMEOUT))
    resync_timeout = oi->params_default->resync_timeout;
  else
    resync_timeout = OSPF_RESYNC_TIMEOUT_DEFAULT;

  return resync_timeout > dead_interval ? resync_timeout : dead_interval;
}
#endif /* HAVE_RESTART */


void
ospf_if_priority_apply (struct ospf_master *om, struct pal_in4_addr addr,
                        u_int32_t ifindex, int priority)
{
  struct ospf_interface *oi;
#ifdef HAVE_OSPF_MULTI_INST
  struct listnode *node_alt = NULL;
  struct ospf *top_alt = NULL;
  struct ospf_interface *oi_other = NULL;
#endif /* HAVE_OSPF_MULTI_INST */

#ifdef HAVE_OSPF_MULTI_AREA
  struct ospf_multi_area_link *mlink = NULL;
  struct listnode *node = NULL;
  struct ospf *top = NULL;
  struct ls_node *rn = NULL;
#endif /* HAVE_OSPF_MULTI_AREA */

  oi = ospf_global_if_entry_lookup (om, addr, ifindex);
  if (oi)
    if (oi->ident.priority != priority)
      {
#ifdef HAVE_OSPF_MULTI_INST
        /* If multiple instances are running on this interface */
        if (CHECK_FLAG (oi->flags, OSPF_IF_EXT_MULTI_INST))
          LIST_LOOP(om->ospf, top_alt, node_alt)
            {
              if (top_alt != oi->top)
                {
                  /* Apply priority for each instance */
                  oi_other = ospf_if_entry_match (top_alt, oi);
                  if (oi_other)
                    {
                      oi_other->ident.priority = priority;
                      OSPF_IFSM_EVENT_SCHEDULE (oi_other, IFSM_NeighborChange);
                    }
                }
            }
#endif /* HAVE_OSPF_MULTI_INST */
        oi->ident.priority = priority;
        OSPF_IFSM_EVENT_SCHEDULE (oi, IFSM_NeighborChange);
#ifdef HAVE_OSPF_MULTI_AREA
        LIST_LOOP(om->ospf, top, node)
          {
            for (rn = ls_table_top (top->multi_area_link_table); rn;
                 rn = ls_route_next (rn))
              if ((mlink = RN_INFO (rn, RNI_DEFAULT)))
                if (mlink->oi && mlink->oi->oi_primary == oi && 
                    mlink->oi->ident.priority != priority)
                  {
                    mlink->oi->ident.priority = priority;
                    OSPF_IFSM_EVENT_SCHEDULE (mlink->oi, IFSM_NeighborChange); 
                  }
          }
#endif /* HAVE_OSPF_MULTI_AREA */
      }
}

#ifdef HAVE_OSPF_TE
void
ospf_if_te_metric_update (struct interface *ifp)
{
  struct ospf_master *om = ifp->vr->proto;
  struct ospf_interface *oi;
  struct connected *ifc;
  u_int32_t metric;
#ifdef HAVE_OSPF_MULTI_INST
  struct listnode *node_alt = NULL;
  struct ospf *top_alt = NULL;
  struct ospf_interface *oi_other = NULL;
#endif /* HAVE_OSPF_MULTI_INST */

#ifdef HAVE_OSPF_MULTI_AREA
  struct ospf_multi_area_link *mlink = NULL;
  struct ls_node *rn = NULL;
  struct listnode *node = NULL;
  struct ospf *top = NULL;
#endif /* HAVE_OSPF_MULTI_AREA */

  for (ifc = ifp->ifc_ipv4; ifc; ifc = ifc->next)
    {
      oi = ospf_global_if_entry_lookup (om, ifc->address->u.prefix4,
                                        ifp->ifindex);
      if (oi != NULL)
        {
          metric = ospf_if_te_metric_get (oi);
          if (oi->te_metric != metric)
            {
#ifdef HAVE_OSPF_MULTI_INST
              /* If multiple instances are running on this interface */
              if (CHECK_FLAG (oi->flags, OSPF_IF_EXT_MULTI_INST))
                LIST_LOOP(om->ospf, top_alt, node_alt)
                  {
                    if (top_alt != oi->top)
                      {
                        /* Get oi from interface table of each instance */
                        oi_other = ospf_if_entry_match (top_alt, oi);
                        if (oi_other)
                          {
                            oi_other->te_metric = metric;
                            ospf_te_if_attr_update (ifp, oi_other->top);                                  
                          }
                      }
                  }
#endif /* HAVE_OSPF_MULTI_INST */
              oi->te_metric = metric;
              ospf_te_if_attr_update (ifp, oi->top);
            }
#ifdef HAVE_OSPF_MULTI_AREA
          LIST_LOOP(om->ospf, top, node)
            {
              for (rn = ls_table_top (top->multi_area_link_table); rn;
                   rn = ls_route_next (rn))
                if ((mlink = RN_INFO (rn, RNI_DEFAULT)))
                  if (mlink->oi && mlink->oi->oi_primary == oi &&
                      mlink->oi->te_metric != metric)
                  {
                    mlink->oi->te_metric = metric;
                    ospf_te_if_attr_update (ifp, mlink->oi->top);
                  }
            }
        
#endif /* HAVE_OSPF_MULTI_AREA */
        }
    }
}
#endif /* HAVE_OSPF_TE */

void
ospf_if_output_cost_update (struct ospf_interface *oi)
{
  u_int32_t newcost = ospf_if_output_cost_get (oi);

  if (oi->output_cost == newcost)
    return;

  /* Update the cost.  */
  oi->output_cost = newcost;

  /* Refresh the router LSA.  */
  ospf_router_lsa_refresh_by_interface (oi);

#ifdef HAVE_OSPF_TE
  /* Refresh the TE LSA.  */
  if (oi->type != OSPF_IFTYPE_VIRTUALLINK)
    ospf_if_te_metric_update (oi->u.ifp);
#endif /* HAVE_OSPF_TE */
}

void
ospf_if_output_cost_apply (struct ospf_master *om,
                           struct pal_in4_addr addr, u_int32_t ifindex)
{
  struct ospf_interface *oi;
#ifdef HAVE_OSPF_MULTI_INST
  struct listnode *node_alt = NULL;
  struct ospf *top_alt = NULL;
  struct ospf_interface *oi_other = NULL;
#endif /* HAVE_OSPF_MULTI_INST */
#ifdef HAVE_OSPF_MULTI_AREA
  struct ospf_multi_area_link *mlink = NULL;
  struct ls_node *rn = NULL;
  struct listnode *node = NULL;
  struct ospf *top = NULL;
#endif /* HAVE_OSPF_MULTI_AREA */

  /* Update the cost.  */
  oi = ospf_global_if_entry_lookup (om, addr, ifindex);
  if (oi != NULL)
    {
#ifdef HAVE_OSPF_MULTI_INST    
      /* If multiple instances are running on this interface */
      if (CHECK_FLAG (oi->flags, OSPF_IF_EXT_MULTI_INST))
        LIST_LOOP(om->ospf, top_alt, node_alt)
          {
            if (top_alt != oi->top)
              {
                /* Get oi from interface table of each instance */
                oi_other = ospf_if_entry_match (top_alt, oi);
                if (oi_other)
                  ospf_if_output_cost_update (oi_other);
              }
          }
#endif /* HAVE_OSPF_MULTI_INST */
      ospf_if_output_cost_update (oi);

#ifdef HAVE_OSPF_MULTI_AREA
      LIST_LOOP(om->ospf, top, node)
        {
          for (rn = ls_table_top (top->multi_area_link_table); rn;
               rn = ls_route_next (rn))
            if ((mlink = RN_INFO (rn, RNI_DEFAULT)))
              if (mlink->oi && mlink->oi->oi_primary == oi)
                ospf_if_output_cost_update (mlink->oi);
        }
#endif /* HAVE_OSPF_MULTI_AREA */
    }
}

void
ospf_if_output_cost_apply_all (struct ospf *top)
{
  struct ospf_interface *oi;
  struct ls_node *rn;
#ifdef HAVE_OSPF_MULTI_AREA
  struct ospf_multi_area_link *mlink = NULL;
#endif /* HAVE_OSPF_MULTI_AREA */

  for (rn = ls_table_top (top->if_table); rn; rn = ls_route_next (rn))
    if ((oi = RN_INFO (rn, RNI_OSPF_IF)))
      ospf_if_output_cost_update (oi);

#ifdef HAVE_OSPF_MULTI_AREA
  for (rn = ls_table_top (top->multi_area_link_table); rn; 
       rn = ls_route_next (rn))
    if ((mlink = RN_INFO (rn, RNI_DEFAULT)))
      if (mlink->oi)
        ospf_if_output_cost_update (mlink->oi);
#endif /* HAVE_OSPF_MULTI_AREA */
}

void
ospf_if_database_filter_apply (struct ospf_master *om,
                               struct pal_in4_addr addr, u_int32_t ifindex)
{
  struct ospf_interface *oi;
#ifdef HAVE_OSPF_MULTI_INST
  struct listnode *node_alt = NULL;
  struct ospf *top_alt = NULL;
  struct ospf_interface *oi_other = NULL;
#endif /* HAVE_OSPF_MULTI_INST */
#ifdef HAVE_OSPF_MULTI_AREA
  struct ospf_multi_area_link *mlink = NULL;
  struct ls_node *rn = NULL;
  struct listnode *node = NULL;
  struct ospf *top = NULL;
#endif /* HAVE_OSPF_MULTI_AREA */
  
  oi = ospf_global_if_entry_lookup (om, addr, ifindex);
  if (oi != NULL)
    {
#ifdef HAVE_OSPF_MULTI_INST
      /* If multiple instances are running on this interface */
      if (CHECK_FLAG (oi->flags, OSPF_IF_EXT_MULTI_INST))
        LIST_LOOP(om->ospf, top_alt, node_alt)
          {
            if (top_alt != oi->top)
              {
                /* Get oi from interface table of each instance */
                oi_other = ospf_if_entry_match (top_alt, oi);
                if (oi_other)
                  {
                    /* Force to reset neighbor. */
                    ospf_hello_send (oi_other, NULL, OSPF_HELLO_RESET_NBR);

                    /* Restart Hello timer. */
                    OSPF_IFSM_TIMER_OFF (oi_other->t_hello);
                    OSPF_IFSM_TIMER_ON (oi_other->t_hello, ospf_hello_timer,
                                        ospf_if_hello_interval_get (oi_other));
                  }
              }
          }
#endif /* HAVE_OSPF_MULTI_INST */
      /* Force to reset neighbor. */
      if (CHECK_FLAG (oi->flags, OSPF_IF_UP))
        ospf_hello_send (oi, NULL, OSPF_HELLO_RESET_NBR);

      OSPF_IFSM_TIMER_OFF (oi->t_hello);

      /* Restart Hello timer. */
      if (CHECK_FLAG (oi->flags, OSPF_IF_UP))
        OSPF_IFSM_TIMER_ON (oi->t_hello, ospf_hello_timer,
                            ospf_if_hello_interval_get (oi));

#ifdef HAVE_OSPF_MULTI_AREA
      LIST_LOOP(om->ospf, top, node)
        {
          for (rn = ls_table_top (top->multi_area_link_table); rn;
               rn = ls_route_next (rn))
            if ((mlink = RN_INFO (rn, RNI_DEFAULT)))
              if (mlink->oi && mlink->oi->oi_primary == oi)
                {
                  ospf_hello_send (mlink->oi, NULL, OSPF_HELLO_RESET_NBR);

                  /* Restart Hello timer. */
                  OSPF_IFSM_TIMER_OFF (mlink->oi->t_hello);
                  OSPF_IFSM_TIMER_ON (mlink->oi->t_hello, ospf_hello_timer,
                                    ospf_if_hello_interval_get (mlink->oi));
                }
        }
#endif /* HAVE_OSPF_MULTI_AREA */
    }
}

void
ospf_if_nbr_timers_update_by_addr (struct interface *ifp,
                                   struct pal_in4_addr addr)
{
  struct ospf_interface *oi;
  struct ospf_neighbor *nbr;
  struct ls_node *rn;
  struct apn_vr *vr = ifp->vr;
  struct ospf_master *om = vr->proto;

#ifdef HAVE_OSPF_MULTI_AREA
  struct ospf_multi_area_link *mlink = NULL;
  struct listnode *node = NULL;
  struct ls_node *rn2 = NULL;
  struct ospf *top = NULL;
#endif /* HAVE_OSPF_MULTI_AREA */

  oi = ospf_global_if_entry_lookup (om, addr, ifp->ifindex);
  if (oi != NULL)
#ifdef HAVE_OSPF_MULTI_AREA
    {
#endif /* HAVE_OSPF_MULTI_AREA */

      for (rn = ls_table_top (oi->nbrs); rn; rn = ls_route_next (rn))
        if ((nbr = RN_INFO (rn, RNI_DEFAULT)))
          {
            nbr->v_inactivity = ospf_if_dead_interval_get (oi);
            nbr->v_retransmit = ospf_if_retransmit_interval_get (oi);
          }
#ifdef HAVE_OSPF_MULTI_AREA
      LIST_LOOP(om->ospf, top, node)
        {
          for (rn2 = ls_table_top (top->multi_area_link_table); rn2;
               rn2 = ls_route_next (rn2))
            if ((mlink = RN_INFO (rn2, RNI_DEFAULT)))
              if (mlink->oi && mlink->oi->oi_primary == oi)
                {
                  for (rn = ls_table_top (mlink->oi->nbrs); rn; 
                       rn = ls_route_next (rn))
                  if ((nbr = RN_INFO (rn, RNI_DEFAULT)))
                    {
                      nbr->v_inactivity = ospf_if_dead_interval_get (mlink->oi);
                      nbr->v_retransmit = 
                         ospf_if_retransmit_interval_get (mlink->oi);
                    } 
                }
        }
    }
#endif /* HAVE_OSPF_MULTI_AREA */

}

/* XXX */
void
ospf_if_nbr_timers_update (struct interface *ifp)
{
  struct connected *ifc;

  for (ifc = ifp->ifc_ipv4; ifc; ifc = ifc->next)
    ospf_if_nbr_timers_update_by_addr (ifp, ifc->address->u.prefix4);
}

void
ospf_if_nbr_timers_update_by_vlink (struct ospf_vlink *vlink)
{
  struct ospf_interface *oi = vlink->oi;
  struct ospf_neighbor *nbr;
  struct ls_node *rn;

  if (oi != NULL)
    for (rn = ls_table_top (oi->nbrs); rn; rn = ls_route_next (rn))
      if ((nbr = RN_INFO (rn, RNI_DEFAULT)))
        {
          nbr->v_inactivity = ospf_if_dead_interval_get (oi);
          nbr->v_retransmit = ospf_if_retransmit_interval_get (oi);
        }
}


void
ospf_if_area_link (struct ospf_interface *oi, struct ospf_area *area)
{
  struct ospf_interface *oi_alt;
  struct prefix p, q;
  struct listnode *node;

  /* Check if there is interface which has the same network address. */
  prefix_copy (&p, oi->ident.address);
  apply_mask (&p);
  LIST_LOOP (area->iflist, oi_alt, node)
    {
      prefix_copy (&q, oi_alt->ident.address);
      apply_mask (&q);
      if (prefix_same (&p, &q))
        oi->clone = ++oi_alt->clone;
    }

  oi->area = ospf_area_lock (area);
  listnode_add (area->iflist, ospf_if_lock (oi));
}

void
ospf_if_area_unlink (struct ospf_interface *oi, struct ospf_area *area)
{
  struct ospf_interface *oi_alt;
  struct prefix p, q;
  struct listnode *node;

  oi->area = NULL;
  listnode_delete (area->iflist, oi);

  /* Check if there is interface which has the same network address. */
  prefix_copy (&p, oi->ident.address);
  apply_mask (&p);
  LIST_LOOP (area->iflist, oi_alt, node)
    {
      prefix_copy (&q, oi_alt->ident.address);
      apply_mask (&q);
      if (prefix_same (&p, &q))
        oi_alt->clone--;
    }
  ospf_area_unlock (area);
  ospf_if_unlock (oi);
}

void
ospf_if_init (struct ospf_interface *oi,
              struct connected *ifc, struct ospf_area *area)
{
  struct ospf_master *om = oi->top->om;
  struct ls_prefix p;

  /* Set the local IP address.  */
  oi->ident.address = prefix_new ();
  if (oi->type == OSPF_IFTYPE_VIRTUALLINK)
    oi->ident.address->family = AF_INET;
  else
    prefix_copy (oi->ident.address, ifc->address);

  /* Set the remote IP address.  */
  oi->destination = prefix_new ();
  if (oi->type == OSPF_IFTYPE_VIRTUALLINK)
    oi->destination->family = AF_INET;
  else if (ifc->destination != NULL)
    prefix_copy (oi->destination, ifc->destination);

  /* Initialize params. */
  if (oi->type != OSPF_IFTYPE_VIRTUALLINK && 
      oi->type != OSPF_IFTYPE_HOST)
    {
      ls_prefix_ipv4_set (&p, IPV4_MAX_BITLEN, oi->ident.address->u.prefix4);
      oi->params = ospf_if_params_lookup (om, oi->u.ifp->name, &p);
      oi->params_default = ospf_if_params_lookup_default (om, oi->u.ifp->name);
      if (oi->params == NULL)
        oi->params = oi->params_default;
    }

  /* Initialize the neighbor table.  */
  oi->nbrs = ls_table_init (LS_IPV4_ROUTE_TABLE_DEPTH, 1);

  /* Initialize the Link State Acknowledgment list.  */
  oi->ls_ack = vector_init (OSPF_VECTOR_SIZE_MIN);
  oi->direct.ls_ack = vector_init (OSPF_VECTOR_SIZE_MIN);

  /* Initialize the LS update queue.  */
  oi->ls_upd_queue = ls_table_init (LS_IPV4_ROUTE_TABLE_DEPTH, 1);
#ifdef HAVE_OPAQUE_LSA
  oi->lsdb = ospf_link_lsdb_init (oi);
#endif /* HAVE_OPAQUE_LSA */

  /* Add pseudo neighbor.  */
  oi->ident.priority = ospf_if_priority_get (oi);
  oi->ident.router_id = oi->top->router_id;

  /* Init IFSM state. */
  oi->state = IFSM_Down;
  oi->ostate = IFSM_Down;

  /* Get the initial network type.  */
  oi->type = ospf_if_network_get_any (oi);

  /* Reset crypt sequence number to current time. */
  oi->crypt_seqnum = pal_time_current (NULL);

  if (oi->type == OSPF_IFTYPE_HOST)
    return;

  /* Link the interface and area. */
  ospf_if_area_link (oi, area);
}

void
ospf_if_clean (struct ospf_interface *oi)
{
  struct ospf_lsa *lsa;

  /* Clean-up network-LSA. */
  lsa = ospf_network_lsa_self (oi);
  if (lsa != NULL)
    {
      ospf_lsa_refresh_event_unset (lsa);
      ospf_lsa_refresh_queue_unset (lsa);
      lsa->param = NULL;
    }

  /* Unlink interface and area. */
  ospf_if_area_unlink (oi, oi->area);

#ifdef HAVE_OSPF_MULTI_AREA
  /* Unlink the primary interface */
  if (CHECK_FLAG (oi->flags, OSPF_MULTI_AREA_IF))
    if (oi->oi_primary)
      {
        ospf_if_unlock (oi->oi_primary);
        oi->oi_primary = NULL;
      }
#endif /* HAVE_OSPF_MULTI_AREA */

  /* Clean neighbor tables. */
  TABLE_FINISH (oi->nbrs);

  /* Clean vector for LS ack. */
  vector_free (oi->ls_ack);
  vector_free (oi->direct.ls_ack);

  /* Clean ls update queue table. */
  TABLE_FINISH (oi->ls_upd_queue);

#ifdef HAVE_OPAQUE_LSA
  /* Clean Opaeue LSDB. */
  ospf_link_lsdb_finish (oi->lsdb);
  oi->lsdb = NULL;
#endif /* HAVE_OPAQUE_LSA */

  if (oi->type == OSPF_IFTYPE_HOST)
    ospf_if_params_free (oi->params_default);
}

struct ospf_interface *
ospf_if_new (struct ospf *top, struct connected *ifc, struct ospf_area *area)
{
  struct ospf_interface *oi;

  oi = XMALLOC (MTYPE_OSPF_IF, sizeof (struct ospf_interface));
  pal_mem_set (oi, 0, sizeof (struct ospf_interface));

  /* Init reference count. */
  ospf_if_lock (oi);
  oi->clone = 1;

  /* Set references. */
  oi->top = top;
  oi->u.ifp = ifc->ifp;

#ifdef HAVE_GMPLS
  if ((ifc->ifp->gmpls_type == PHY_PORT_TYPE_GMPLS_CONTROL)
      ||(ifc->ifp->gmpls_type == PHY_PORT_TYPE_GMPLS_DATA_CONTROL))
    SET_FLAG (oi->flags, OSPF_IF_GMPLS_TYPE_CONTROL);
  else if (ifc->ifp->gmpls_type == PHY_PORT_TYPE_GMPLS_DATA)
    SET_FLAG (oi->flags, OSPF_IF_GMPLS_TYPE_DATA);
#endif /* HAVE_GMPLS */

  /* Initialize the interface.  */
  ospf_if_init (oi, ifc, area);

  /* Register to tables. */
  ospf_if_entry_insert (top, oi, ifc);
  ospf_global_if_entry_add (top->om, ifc->address->u.prefix4,
                            ifc->ifp->ifindex, oi);
#ifdef HAVE_BFD
  ospf_bfd_if_update (oi, PAL_TRUE);
#endif /* HAVE_BFD */

  /* Call notifiers. */
  ospf_call_notifiers (top->om, OSPF_NOTIFY_NETWORK_NEW, oi, NULL);

  return oi;
}

/* Create a new host entry specific ospf interface. */
struct ospf_interface *
ospf_host_entry_if_new (struct ospf *top, struct connected *ifc,
                  struct ospf_area *area, struct ospf_host_entry *host)
{
  struct ospf_interface *oi = NULL;

  oi = XMALLOC (MTYPE_OSPF_IF, sizeof (struct ospf_interface));
  pal_mem_set (oi, 0, sizeof (struct ospf_interface));

  /* Init reference count. */
  ospf_if_lock (oi);
  oi->clone = 1;

  /* Set references. */
  oi->top = top;
  oi->u.ifp = ifc->ifp;
  oi->type = OSPF_IFTYPE_HOST;
  oi->output_cost = host->metric;
  oi->host = host;

  /* Initialize the interface.  */
  ospf_if_init (oi, ifc, area);

  oi->ident.address->u.prefix4 = host->addr;
  oi->ident.address->prefixlen = IPV4_MAX_BITLEN;

  oi->params_default = ospf_if_params_new (top->om);
  oi->params = oi->params_default;

  if (oi->params != NULL)
    ospf_if_params_cost_set (oi->params, host->metric);

  /* Link the interface and area. */
  ospf_if_area_link (oi, area);

  /* Register to tables. */
  ospf_if_entry_insert (top, oi, ifc);
  ospf_global_if_entry_add (top->om, oi->ident.address->u.prefix4,
                            ifc->ifp->ifindex, oi);

  /* Call notifiers. */
  ospf_call_notifiers (top->om, OSPF_NOTIFY_NETWORK_NEW, oi, NULL);

  return oi;
}

/* Delete all host specific ospf interfaces that match to this connected. */
void
ospf_global_if_host_entry_delete (struct ospf_master *om, struct connected *ifc)
{
  struct ls_node *rn = NULL;
  struct ospf_interface *oi = NULL;
  struct prefix p;
  struct prefix host_p;

  for (rn = ls_table_top (om->if_table); rn; rn = ls_route_next (rn))
    if ((oi = RN_INFO (rn, RNI_DEFAULT)))
      {
        if (oi->u.ifp->ifindex == ifc->ifp->ifindex &&
            oi->type == OSPF_IFTYPE_HOST)
          {
            prefix_copy (&p, ifc->address);
            apply_mask (&p);

            prefix_copy (&host_p, &p);
            host_p.u.prefix4 = oi->host->addr;
            apply_mask (&host_p);

            if (IPV4_ADDR_SAME (&p.u.prefix4, &host_p.u.prefix4))
              ospf_if_delete (oi);
          }
      }
}

void
ospf_if_free (struct ospf_interface *oi)
{
  if (oi->ident.address)
    prefix_free (oi->ident.address);
  if (oi->destination)
    prefix_free (oi->destination);

  XFREE (MTYPE_OSPF_IF, oi);
}

struct ospf_interface *
ospf_if_lock (struct ospf_interface *oi)
{
  oi->lock++;
  return oi;
}

void
ospf_if_unlock (struct ospf_interface *oi)
{
  if (!oi)
    return;

  pal_assert (oi->lock > 0);
  oi->lock--;

  if (oi->lock == 0)
    {
      pal_assert (CHECK_FLAG (oi->flags, OSPF_IF_DESTROY));
      ospf_if_free (oi);
    }
}

int
ospf_if_delete (struct ospf_interface *oi)
{
  struct ospf_master *om = oi->top->om;
#ifdef HAVE_OSPF_MULTI_INST
  struct ospf *top_alt = NULL;
  struct ospf_interface *oi_other = NULL;
  struct listnode *node = NULL; 
  u_int32_t proc_count = 0;
#endif /* HAVE_OSPF_MULTI_INST */
#ifdef HAVE_OSPF_MULTI_AREA
  struct pal_in4_addr area_id;
  struct pal_in4_addr nbr_addr;
#endif /* HAVE_OSPF_MULTI_AREA */

  /* Check destroy flag. */
  if (CHECK_FLAG (oi->flags, OSPF_IF_DESTROY))
    return 0;
#ifdef HAVE_OSPF_MULTI_AREA
  /* Remove multi area adjacencies before removing primary adjacency */
  if ((!(CHECK_FLAG (oi->flags, OSPF_MULTI_AREA_IF))) && 
        (oi->type != OSPF_IFTYPE_VIRTUALLINK))
    ospf_multi_area_if_entries_del_corr_to_primary_adj (oi);
#endif /* HAVE_OSPF_MULTI_AREA */

  /* Set destroy flag.  The interface is never reused. */
  SET_FLAG (oi->flags, OSPF_IF_DESTROY);

  if (CHECK_FLAG (oi->flags, OSPF_IF_UP))
    ospf_if_down (oi);

#ifdef HAVE_OSPF_MULTI_AREA
  if (CHECK_FLAG (oi->flags, OSPF_MULTI_AREA_IF))
    {
      pal_mem_cpy (&area_id, &oi->area->area_id, sizeof (struct pal_in4_addr));
      pal_mem_cpy (&nbr_addr, &oi->destination->u.prefix4, 
                   sizeof (struct pal_in4_addr));
    }
#endif /* HAVE_OSPF_MULTI_AREA */

  /* Clean the interfaces.  */
  ospf_if_clean (oi);

  /* Remove entry from tables. */
  if (oi->type == OSPF_IFTYPE_VIRTUALLINK)
    ospf_vlink_entry_delete (oi->top, oi->u.vlink);
  else
    {
#ifdef HAVE_OSPF_MULTI_AREA
      if (CHECK_FLAG (oi->flags, OSPF_MULTI_AREA_IF))
        ospf_multi_area_if_entry_delete (oi->top, area_id, nbr_addr);
      else
        {
#endif /* HAVE_OSPF_MULTI_AREA */
        ospf_if_entry_delete (oi->top, oi);

#ifdef HAVE_OSPF_MULTI_INST
        /* Check if it is the entry in global if-table */
        oi_other = ospf_global_if_entry_lookup (om, oi->ident.address->u.prefix4,
                                              oi->u.ifp->ifindex); 
        if (oi_other)
          {
            if (oi_other->top == oi->top)
              {
                /* Delete the entry */
                ospf_global_if_entry_delete (om, oi->ident.address->u.prefix4,
                                             oi->u.ifp->ifindex);
                if (CHECK_FLAG (oi->flags, OSPF_IF_EXT_MULTI_INST))
                  /* Add the next oi entry to global interface table */
                  ospf_global_if_entry_add_next (om, oi_other);
              }
            else if (CHECK_FLAG (oi_other->flags, OSPF_IF_EXT_MULTI_INST))
              {
                 /* Check if no more ospf processes are using 
                    "extenstion to multi instance" feature */
                 LIST_LOOP (om->ospf, top_alt, node)
                   {
                     if (ospf_if_entry_match (top_alt, oi_other))
                       proc_count++;
                   }
                 if (proc_count == 1) 
                   UNSET_FLAG (oi_other->flags, OSPF_IF_EXT_MULTI_INST);
              }
          }
#else
      ospf_global_if_entry_delete (om, oi->ident.address->u.prefix4,
                                   oi->u.ifp->ifindex);
#endif /* HAVE_OSPF_MULTI_INST */
#ifdef HAVE_OSPF_MULTI_AREA
        }
#endif /* HAVE_OSPF_MULTI_INST */
    }

  /* Call notifiers. */
  ospf_call_notifiers (om, OSPF_NOTIFY_NETWORK_DEL, oi, NULL);

  /* Cancel out any pending events.  */
  thread_cancel_event (ZG, oi);

  /* Unlock reference. */
  ospf_if_unlock (oi);

  return 1;
}


void
ospf_if_stream_set (struct ospf_interface *oi)
{
  if (oi->type != OSPF_IFTYPE_VIRTUALLINK && if_is_multicast (oi->u.ifp))
    ospf_if_join_allspfrouters (oi);
}

/* Suppose this function is called via IFSM. */
void
ospf_if_stream_unset (struct ospf_interface *oi)
{
  ospf_packet_clear_all (oi);

  if (CHECK_FLAG (oi->flags, OSPF_IF_WRITE_Q))
    {
      listnode_delete (oi->top->oi_write_q, oi);

      UNSET_FLAG (oi->flags, OSPF_IF_WRITE_Q);

      if (list_isempty (oi->top->oi_write_q))
        OSPF_IFSM_TIMER_OFF (oi->top->t_write);
    }

  if (CHECK_FLAG (oi->flags, OSPF_IF_MC_ALLDROUTERS))
    ospf_if_leave_alldrouters (oi);

  if (CHECK_FLAG (oi->flags, OSPF_IF_MC_ALLSPFROUTERS))
    ospf_if_leave_allspfrouters (oi);
}

int
ospf_auth_type (struct ospf_interface *oi)
{
  int auth_type;

  if (!OSPF_IF_PARAM_CHECK (oi->params, AUTH_TYPE)
      && !OSPF_IF_PARAM_CHECK (oi->params_default, AUTH_TYPE))
    auth_type = ospf_area_auth_type_get (oi->area);
  else
    auth_type = ospf_if_authentication_type_get (oi);

  return auth_type;
}

void
ospf_if_set_variables (struct ospf_interface *oi)
{
  /* Calculate output cost accordingly.  */
  if (oi->type != OSPF_IFTYPE_VIRTUALLINK)
    oi->output_cost = ospf_if_output_cost_get (oi);

#ifdef HAVE_OSPF_TE
  /* Set Te Metric.  */
  if (OSPF_IF_PARAM_CHECK (oi->params, TE_METRIC))
    oi->te_metric = oi->params->te_metric;
  else if (OSPF_IF_PARAM_CHECK (oi->params_default, TE_METRIC))
    oi->te_metric = oi->params_default->te_metric;
#endif /* HAVE_OSPF_TE */

  /* Update the network type.  */
  oi->type = ospf_if_network_get_any (oi);

  /* Set the passive-interface flag.  */
  ospf_passive_if_apply (oi);

  /* Set crypt_seqnum to current time. */
  oi->crypt_seqnum = pal_time_current (NULL);

  /* Set outgoing stream. */
  if (oi->type != OSPF_IFTYPE_LOOPBACK)
    ospf_if_stream_set (oi);

#ifdef HAVE_RESTART
  if ((ospf_auth_type (oi)) == OSPF_AUTH_CRYPTOGRAPHIC)
    if (IS_GRACEFUL_RESTART (oi->top))
      {
        u_int32_t crypt_seqnum = 0;
        struct pal_in4_addr *p;

        if (oi->type != OSPF_IFTYPE_VIRTUALLINK
#ifdef HAVE_OSPF_MULTI_AREA
            &&
            !CHECK_FLAG (oi->flags, OSPF_MULTI_AREA_IF)
#endif /* HAVE_OSPF_MULTI_AREA */
           )
          {
            p = &oi->ident.address->u.prefix4;
            crypt_seqnum =
              ospf_restart_crypt_seqnum_lookup (oi->top, p,
                                                oi->u.ifp->ifindex);
          }
#ifdef HAVE_OSPF_MULTI_AREA
        else if (CHECK_FLAG (oi->flags, OSPF_MULTI_AREA_IF))
          {
            p = &oi->ident.address->u.prefix4;

            crypt_seqnum =
              ospf_restart_multi_area_link_crypt_seqnum_lookup (oi->top,
                                                oi->top->ospf_id,
                                                &oi->area->area_id,
                                                &oi->destination->u.prefix4);
          }
#endif /* HAVE_OSPF_MULTI_AREA */
        else
          crypt_seqnum =
            ospf_restart_vlink_crypt_seqnum_lookup (oi->top, oi->top->ospf_id,
                                                    &oi->area->area_id,
                                                    &oi->u.vlink->peer_id);

        if (crypt_seqnum != 0)
          oi->crypt_seqnum = crypt_seqnum;
      }
#endif /* HAVE_RESTART */
}

/* Restore an interface to its pre UP state. */
void
ospf_if_reset_variables (struct ospf_interface *oi)
{
  struct ospf_neighbor *nbr;
  struct ospf_lsa *lsa;
  struct ls_node *rn;
  vector queue;
  int i;

  if (oi->state == IFSM_Down)
    return;

  UNSET_FLAG (oi->flags, OSPF_IF_DR_ELECTION_READY);

  /* Send Neighbor event KillNbr to all associated neighbors. */
  for (rn = ls_table_top (oi->nbrs); rn; rn = ls_route_next (rn))
    if ((nbr = RN_INFO (rn, RNI_DEFAULT)))
      {
        /* Clear NBMA neighbor. */
        if (CHECK_FLAG (nbr->flags, OSPF_NEIGHBOR_STATIC))
          {
            UNSET_FLAG (nbr->flags, OSPF_NEIGHBOR_STATIC);
            ospf_nbr_static_reset (oi, nbr->ident.address->u.prefix4);
          }

        OSPF_NFSM_EVENT_EXECUTE (nbr, NFSM_KillNbr);
      }

  /* Cleanup Link State Acknowlegdment list. */
  for (i = 0; i < vector_max (oi->ls_ack); i++)
    if ((lsa = vector_slot (oi->ls_ack, i)))
      {
        ospf_lsa_unlock (lsa);
        vector_slot (oi->ls_ack, i) = NULL;
      }

  /* Cleanup Link State Update queue. */
  for (rn = ls_table_top (oi->ls_upd_queue); rn; rn = ls_route_next (rn))
    if ((queue = RN_INFO (rn, RNI_DEFAULT)) != NULL)
      {
        for (i = 0; i < vector_max (queue); i++)
          if ((lsa = vector_slot (queue, i)))
            ospf_lsa_unlock (lsa);

        vector_free (queue);
        RN_INFO_UNSET (rn, RNI_DEFAULT);
      }

#ifdef HAVE_OPAQUE_LSA
  /* Cleanup Opaque-LSA related data. */
  ospf_lsdb_lsa_discard_all (oi->lsdb, 1, 1, 1);
#endif /* HAVE_OPAQUE_LSA */

  /* Free outgoing stream and drop multicast. */
  ospf_if_stream_unset (oi);

  /* Reset DR and BDR. */
  oi->ident.d_router = OSPFRouterIDUnspec;
  oi->ident.bd_router = OSPFRouterIDUnspec;

  /* Self-originated network-LSA will be cleaned in ifsm_change_status (). */
  /* Timer will be canceled in ospf_ifsm_event() -> ifsm_timer_set(). */
}

int
ospf_if_is_up (struct ospf_interface *oi)
{
  if (oi->type == OSPF_IFTYPE_VIRTUALLINK)
    return CHECK_FLAG (oi->flags, OSPF_IF_UP);

  return if_is_up (oi->u.ifp);
}

int
ospf_if_up (struct ospf_interface *oi)
{
  /* Sanity check.  Destroyed interface never comes up again. */
  pal_assert (oi != NULL);
  pal_assert (!CHECK_FLAG (oi->flags, OSPF_IF_DESTROY));

  /* Process should be active.  */
  if (!IS_PROC_ACTIVE (oi->top))
    return 0;

  /* Interface is already up. */
  if (CHECK_FLAG (oi->flags, OSPF_IF_UP))
    return 0;

  /* Turn interface flag on. */
  SET_FLAG (oi->flags, OSPF_IF_UP);

  /* Init interface related variables and tables. */
  ospf_if_set_variables (oi);

  /* IFSM event start. */
  if (oi->type == OSPF_IFTYPE_LOOPBACK)
    OSPF_IFSM_EVENT_SCHEDULE (oi, IFSM_LoopInd);
  else
    {
      OSPF_IFSM_EVENT_SCHEDULE (oi, IFSM_InterfaceUp);
#ifdef HAVE_RESTART
      if (IS_GRACEFUL_RESTART (oi->top))
        if ((ospf_restart_reason_lookup (oi->top))
            == OSPF_RESTART_REASON_UNKNOWN)
          ospf_grace_lsa_flood_force (oi);

      if (IS_RESTART_SIGNALING(oi->top)) 
        SET_FLAG (oi->flags, OSPF_IF_RS_BIT_SET); 
#endif /* HAVE_RESTART */
    }

  return 1;
}

int
ospf_if_down (struct ospf_interface *oi)
{
#ifdef HAVE_OSPF_MULTI_AREA
  struct ospf_vlink *vlink = NULL;
  struct ls_node *rn = NULL;
#endif /* HAVE_OSPF_MULTI_AREA */

  if (!CHECK_FLAG (oi->flags, OSPF_IF_UP))
    return 0;

  /* If administrativly down, send remaining packets in the queue. */
  if (oi->type == OSPF_IFTYPE_VIRTUALLINK
      || (!if_is_loopback (oi->u.ifp) && if_is_up (oi->u.ifp)))
    {
      OSPF_EVENT_EXECUTE (ospf_ls_upd_send_event, oi, 0);
      ospf_packet_send_all (oi);
    }

  UNSET_FLAG (oi->flags, OSPF_IF_UP);

  ospf_link_vec_unset_by_if (oi->area, oi);

  /* Change IFSM state to IFSM_Down. */
  OSPF_IFSM_EVENT_EXECUTE (oi, IFSM_InterfaceDown);

#ifdef HAVE_OSPF_MULTI_AREA
  if (CHECK_FLAG (oi->flags, OSPF_MULTI_AREA_IF))
    if (IS_AREA_BACKBONE (oi->area))
      /* Enable vlink after multi area link deletion */
      for (rn = ls_table_top (oi->top->vlink_table); rn;
           rn = ls_route_next (rn))
        if ((vlink = RN_INFO (rn, RNI_DEFAULT)))
          if (!CHECK_FLAG (vlink->flags, OSPF_VLINK_UP) && vlink->oi && 
              IPV4_ADDR_SAME (&vlink->oi->destination->u.prefix4, 
                              &oi->destination->u.prefix4) &&
              IPV4_ADDR_SAME (&vlink->area_id, &oi->oi_primary->area->area_id))
            ospf_vlink_if_up (vlink); 
#endif /* HAVE_OSPF_MULTI_AREA */

  return 1;
}

int
ospf_if_update (struct ospf_interface *oi)
{
  int ret = 0;

  if (CHECK_FLAG (oi->flags, OSPF_IF_UP))
    {
      ospf_if_down (oi);
      ret = ospf_if_up (oi);
    }
  return ret;
}

int
ospf_if_update_by_params (struct ospf_if_params *oip)
{
  struct ospf_interface *oi;
  struct ospf_vlink *vlink;
  struct interface *ifp;
  struct connected *ifc;
#ifdef HAVE_OSPF_MULTI_INST
  struct listnode *node_alt = NULL;
  struct ospf *top_alt = NULL;
  struct ospf_interface *oi_other = NULL;
#endif /* HAVE_OSPF_MULTI_INST */
#ifdef HAVE_OSPF_MULTI_AREA
  struct ospf_multi_area_link *mlink = NULL;
  struct listnode *node = NULL;
  struct ls_node *rn = NULL;
  struct ospf *top = NULL;
#endif /* HAVE_OSPF_MULTI_AREA */

  ifp = if_lookup_by_name (&oip->om->vr->ifm, oip->desc);
  if (ifp != NULL)
    {
      for (ifc = ifp->ifc_ipv4; ifc; ifc = ifc->next)
        {
          oi = ospf_global_if_entry_lookup (oip->om,
                                            ifc->address->u.prefix4,
                                            ifp->ifindex);
          if (oi != NULL)
            {
#ifdef HAVE_OSPF_MULTI_INST
              if (CHECK_FLAG (oi->flags, OSPF_IF_EXT_MULTI_INST))
                {
                  LIST_LOOP(oip->om->ospf, top_alt, node_alt)
                    {
                      if (top_alt != oi->top)
                        {
                          /* Get oi from interface table of each instance */
                          oi_other = ospf_if_entry_match (top_alt, oi);
                          if (oi_other 
                              && (oi_other->params == oip 
                              || oi_other->params_default == oip))
                            ospf_if_update (oi_other);
                        }
                    }
                }
#endif /* HAVE_OSPF_MULTI_INST */

              if (oi->params == oip || oi->params_default == oip)
                ospf_if_update (oi);
#ifdef HAVE_OSPF_MULTI_AREA
              LIST_LOOP(oip->om->ospf, top, node)
                {
                  for (rn = ls_table_top (top->multi_area_link_table); rn;
                       rn = ls_route_next (rn))
                    if ((mlink = RN_INFO (rn, RNI_DEFAULT)))
                      if ((mlink->oi && mlink->oi->oi_primary == oi) && 
                          (mlink->oi->params == oip || 
                           mlink->oi->params_default == oip))
                         ospf_if_update (mlink->oi);  
                }
#endif /* HAVE_OSPF_MULTI_AREA */            
            }
        }
    }
  else
    {
      vlink = ospf_vlink_lookup_by_name (oip->om, oip->desc);
      if (vlink != NULL)
        if (vlink->oi != NULL)
          if (vlink->oi->params_default == oip)
            ospf_if_update (vlink->oi);
    }
  return 1;
}

struct ospf_interface *
ospf_if_lookup_by_params (struct ospf_if_params *oip)
{
  struct ospf_interface *oi;
  struct ospf_vlink *vlink;
  struct interface *ifp;
  struct connected *ifc;

  ifp = if_lookup_by_name (&oip->om->vr->ifm, oip->desc);
  if (ifp != NULL)
    {
      for (ifc = ifp->ifc_ipv4; ifc; ifc = ifc->next)
        {
          oi = ospf_global_if_entry_lookup (oip->om,
                                            ifc->address->u.prefix4,
                                            ifp->ifindex);
          if (oi != NULL)
            if (oi->params == oip || oi->params_default == oip)
	      return oi;
        }
    }
  else
    {
      vlink = ospf_vlink_lookup_by_name (oip->om, oip->desc);
      if (vlink != NULL)
        if (vlink->oi != NULL)
          if (vlink->oi->params_default == oip)
	    return vlink->oi;
    }
  return NULL;
}


/* Interface parameter pool related functions. */
struct ls_table *
ospf_if_params_table_new (char *desc)
{
  struct ls_table *new;

  new = ls_table_init (LS_IPV4_ROUTE_TABLE_DEPTH, 1);
  new->desc = XMALLOC (MTYPE_OSPF_DESC, pal_strlen (desc) + 1);
  pal_strncpy (new->desc, desc, pal_strlen (desc) + 1);

  return new;
}

void
ospf_if_params_table_free (void *arg)
{
  struct ospf_if_params *oip;
  struct ls_table *rt;
  struct ls_node *rn;

  rt = arg;
  for (rn = ls_table_top (rt); rn; rn = ls_route_next (rn))
    if ((oip = RN_INFO (rn, RNI_DEFAULT)))
      {
        ospf_if_params_free (oip);
        RN_INFO_UNSET (rn, RNI_DEFAULT);
      }

  if (rt->desc)
    XFREE (MTYPE_OSPF_DESC, rt->desc);

  TABLE_FINISH (rt);
}

struct ls_table *
ospf_if_params_table_lookup_by_name (struct ospf_master *om, char *desc)
{
  struct listnode *node;

  for (node = LISTHEAD (om->if_params); node; NEXTNODE (node))
    {
      struct ls_table *rt = GETDATA (node);
      if (!rt)
        continue;

      if (rt->desc)
        if (pal_strcmp (rt->desc, desc) == 0)
          return rt;
    }
  return NULL;
}

struct ls_table *
ospf_if_params_table_get (struct ospf_master *om, char *desc)
{
  struct ls_table *rt;

  rt = ospf_if_params_table_lookup_by_name (om, desc);
  if (rt)
    return rt;

  rt = ospf_if_params_table_new (desc);

  listnode_add_sort (om->if_params, rt);

  return rt;
}

void
ospf_if_params_table_delete (struct ospf_master *om, char *desc)
{
  struct ls_table *rt;

  rt = ospf_if_params_table_lookup_by_name (om, desc);
  if (!rt)
    return;

  listnode_delete (om->if_params, rt);

  ospf_if_params_table_free (rt);
}

int
ospf_if_params_table_cmp (void *v1, void *v2)
{
  struct ls_table *rt1 = v1, *rt2 = v2;

  if (!rt1->desc)
    return 1;

  if (!rt2->desc)
    return 0;

  return pal_strcmp (rt1->desc, rt2->desc);
}


/* OSPF interface parameter related functions. */
struct ospf_if_params *
ospf_if_params_new (struct ospf_master *om)
{
  struct ospf_if_params *oip;

  oip = XMALLOC (MTYPE_OSPF_IF_PARAMS, sizeof (struct ospf_if_params));
  pal_mem_set (oip, 0, sizeof (struct ospf_if_params));

  /* Set the OSPF master pointer.  */
  oip->om = om;

  return oip;
}

void
ospf_if_params_clean (struct ospf_if_params *oip)
{
  if (oip->auth_simple)
    {
      XFREE (MTYPE_OSPF_AUTH_KEY, oip->auth_simple);
      oip->auth_simple = NULL;
    }

  if (oip->auth_crypt)
    {
      ospf_crypt_key_delete_all (oip->auth_crypt);
      list_delete (oip->auth_crypt);
    }
}

void
ospf_if_params_reset (struct ospf_if_params *oip)
{
  ospf_if_params_clean (oip);

  /* Initialize flags.  */
  oip->config = 0;

  /* Sanity cleanup.  */
  oip->type = OSPF_IFTYPE_NONE;
  oip->priority = OSPF_ROUTER_PRIORITY_DEFAULT;
  oip->output_cost = OSPF_OUTPUT_COST_DEFAULT;
  oip->hello_interval = OSPF_IF_PARAM_HELLO_INTERVAL_DEFAULT (oip);
  oip->dead_interval = OSPF_IF_PARAM_DEAD_INTERVAL_DEFAULT (oip);
  oip->transmit_delay = OSPF_TRANSMIT_DELAY_DEFAULT;
  oip->retransmit_interval = OSPF_RETRANSMIT_INTERVAL_DEFAULT;
  oip->auth_type = OSPF_AUTH_NULL;
  oip->mtu = 0;
  oip->auth_simple = NULL;
  oip->auth_crypt = NULL;
#ifdef HAVE_OSPF_TE
  oip->te_metric = 0;
#endif /* HAVE_OSPF_TE */
#ifdef HAVE_RESTART
  oip->resync_timeout = OSPF_RESYNC_TIMEOUT_DEFAULT;
#endif /* HAVE_RESTART */
}

void
ospf_if_params_free (struct ospf_if_params *oip)
{
  ospf_if_params_clean (oip);
  XFREE (MTYPE_OSPF_IF_PARAMS, oip);
}

void
ospf_if_params_link (struct ospf_master *om,
                     struct ospf_if_params *oip, struct ls_prefix *p)
{
  struct interface *ifp;
  struct ospf_interface *oi;
  struct pal_in4_addr addr;
#ifdef HAVE_OSPF_MULTI_INST
  struct listnode *node_alt = NULL;
  struct ospf *top_alt = NULL;
  struct ospf_interface *oi_other = NULL;
#endif /* HAVE_OSPF_MULTI_INST */
#ifdef HAVE_OSPF_MULTI_AREA
  struct ospf_multi_area_link *mlink = NULL;
  struct listnode *node = NULL;
  struct ls_node *rn = NULL;
  struct ospf *top = NULL;
#endif /* HAVE_OSPF_MULTI_AREA */

  ifp = if_lookup_by_name (&om->vr->ifm, oip->desc);
  if (ifp)
    {
      pal_mem_cpy (&addr, p->prefix, sizeof (struct pal_in4_addr));
      oi = ospf_global_if_entry_lookup (om, addr, ifp->ifindex);
      if (oi)
        {
#ifdef HAVE_OSPF_MULTI_INST
          /* If multiple instances are running on this interface */
          if (CHECK_FLAG (oi->flags, OSPF_IF_EXT_MULTI_INST))
            LIST_LOOP(om->ospf, top_alt, node_alt)
              {
                if (top_alt != oi->top)
                  {
                    /* Get oi from interface table of each instance */
                    oi_other = ospf_if_entry_match (top_alt, oi);
                    if (oi_other)
                      {
                        oi_other->params = oip;
                        if (oi_other->params_default != NULL)
                          oi_other->params->type = oi_other->params_default->type;
                      }
                  }
              }
#endif /* HAVE_OSPF_MULTI_INST */
          oi->params = oip;
          if (oi->params_default != NULL)
            oi->params->type = oi->params_default->type;
#ifdef HAVE_OSPF_MULTI_AREA
          LIST_LOOP(om->ospf, top, node)
            {
              for (rn = ls_table_top (top->multi_area_link_table); rn;
                   rn = ls_route_next (rn))
              if ((mlink = RN_INFO (rn, RNI_DEFAULT)))
                if (mlink->oi && mlink->oi->oi_primary == oi)
                  {
                    mlink->oi->params = oip;
                    if (mlink->oi->params_default != NULL)
                      mlink->oi->params->type = mlink->oi->params_default->type;
                  }
            }
#endif /* HAVE_OSPF_MULTI_AREA */
        }
    }
}

void
ospf_if_params_link_default (struct ospf_master *om,
                             struct ospf_if_params *oip)
{
  struct ospf_interface *oi;
  struct ospf_vlink *vlink;
  struct interface *ifp;
  struct connected *ifc;
#ifdef HAVE_OSPF_MULTI_INST
  struct listnode *node_alt = NULL;
  struct ospf *top_alt = NULL;
  struct ospf_interface *oi_other = NULL;
#endif /* HAVE_OSPF_MULTI_INST */

#ifdef HAVE_OSPF_MULTI_AREA
  struct ospf_multi_area_link *mlink = NULL;
  struct listnode *node = NULL;
  struct ls_node *rn = NULL;
  struct ospf *top = NULL;
#endif /* HAVE_OSPF_MULTI_AREA */

  ifp = if_lookup_by_name (&om->vr->ifm, oip->desc);
  if (ifp != NULL)
    {
      for (ifc = ifp->ifc_ipv4; ifc; ifc = ifc->next)
        {
          oi = ospf_global_if_entry_lookup (om, ifc->address->u.prefix4,
                                            ifp->ifindex);
          if (oi)
            {
#ifdef HAVE_OSPF_MULTI_INST
              /* If multiple instances are running on this interface */
              if (CHECK_FLAG (oi->flags, OSPF_IF_EXT_MULTI_INST))
                LIST_LOOP(om->ospf, top_alt, node_alt)
                  {
                    if (top_alt != oi->top)
                      {
                        /* Get oi from interface table of each instance */
                        oi_other = ospf_if_entry_match (top_alt, oi);
                        if (oi_other)
                          {
                            oi_other->params_default = oip;
                            if (oi_other->params == NULL)
                               oi_other->params = oip;
                          }
                      }
                  }
#endif /* HAVE_OSPF_MULTI_INST */
              oi->params_default = oip;
              if (oi->params == NULL)
                oi->params = oip;
#ifdef HAVE_OSPF_MULTI_AREA
              LIST_LOOP(om->ospf, top, node)
                {
                  for (rn = ls_table_top (top->multi_area_link_table); rn;
                       rn = ls_route_next (rn))
                  if ((mlink = RN_INFO (rn, RNI_DEFAULT)))
                    if (mlink->oi && mlink->oi->oi_primary == oi)
                      { 
                        mlink->oi->params_default = oip;
                        if (mlink->oi->params == NULL)
                          mlink->oi->params = oip;
                      }
                } 
#endif /* HAVE_OSPF_MULTI_AREA */
            }
        }
    }
  else
    {
      vlink = ospf_vlink_lookup_by_name (om, oip->desc);
      if (vlink != NULL)
        if (vlink->oi != NULL)
          {
            vlink->oi->params_default = oip;
            if (vlink->oi->params == NULL)
              vlink->oi->params = oip;
          }
    }
}

void
ospf_if_params_unlink (struct ospf_master *om, struct ospf_if_params *oip,
                       struct ls_prefix *p)
{
  struct ospf_interface *oi;
  struct interface *ifp;
  struct pal_in4_addr addr;
#ifdef HAVE_OSPF_MULTI_INST
  struct listnode *node_alt = NULL;
  struct ospf *top_alt = NULL;
  struct ospf_interface *oi_other = NULL;
#endif /* HAVE_OSPF_MULTI_INST */
#ifdef HAVE_OSPF_MULTI_AREA
  struct ospf_multi_area_link *mlink = NULL;
  struct listnode *node = NULL;
  struct ls_node *rn = NULL;
  struct ospf *top = NULL;
#endif /* HAVE_OSPF_MULTI_AREA */

  ifp = if_lookup_by_name (&om->vr->ifm, oip->desc);
  if (ifp)
    {
      pal_mem_cpy (&addr, p->prefix, sizeof (struct pal_in4_addr));
      oi = ospf_global_if_entry_lookup (om, addr, ifp->ifindex);
      if (oi)
        {
#ifdef HAVE_OSPF_MULTI_INST
          /* If multiple instances are running on this interface */
          if (CHECK_FLAG (oi->flags, OSPF_IF_EXT_MULTI_INST))
            LIST_LOOP(om->ospf, top_alt, node_alt)
              {
                if (top_alt != oi->top)
                  {
                    /* Get oi from interface table of each instance */
                    oi_other = ospf_if_entry_match (top_alt, oi);
                    if (oi_other)
                      oi_other->params = oi_other->params_default;
                  }
              }
#endif /* HAVE_OSPF_MULTI_INST */
          oi->params = oi->params_default;
#ifdef HAVE_OSPF_MULTI_AREA
          LIST_LOOP(om->ospf, top, node)
            {
              for (rn = ls_table_top (top->multi_area_link_table); rn;
                   rn = ls_route_next (rn))
                if ((mlink = RN_INFO (rn, RNI_DEFAULT)))
                  if (mlink->oi && mlink->oi->oi_primary == oi)
                     mlink->oi->params = mlink->oi->params_default;
            }
#endif /* HAVE_OSPF_MULTI_AREA */ 
        }
    }
}

void
ospf_if_params_unlink_default (struct ospf_master *om,
                               struct ospf_if_params *oip)
{
  struct ospf_interface *oi;
  struct ospf_vlink *vlink;
  struct interface *ifp;
  struct connected *ifc;
#ifdef HAVE_OSPF_MULTI_INST
  struct listnode *node_alt = NULL;
  struct ospf *top_alt = NULL;
  struct ospf_interface *oi_other = NULL;
#endif /* HAVE_OSPF_MULTI_INST */
#ifdef HAVE_OSPF_MULTI_AREA
  struct ospf_multi_area_link *mlink = NULL;
  struct listnode *node = NULL;
  struct ls_node *rn = NULL;
  struct ospf *top = NULL;
#endif /* HAVE_OSPF_MULTI_AREA */

  ifp = if_lookup_by_name (&om->vr->ifm, oip->desc);
  if (ifp != NULL)
    {
      for (ifc = ifp->ifc_ipv4; ifc; ifc = ifc->next)
        {
          oi = ospf_global_if_entry_lookup (om, ifc->address->u.prefix4,
                                            ifp->ifindex);
          if (oi)
            {
#ifdef HAVE_OSPF_MULTI_INST
              /* If multiple instances are running on this interface */
              if (CHECK_FLAG (oi->flags, OSPF_IF_EXT_MULTI_INST))
                LIST_LOOP(om->ospf, top_alt, node_alt)
                  {
                    if (top_alt != oi->top)
                      {
                        /* Get oi from interface table of each instance */
                        oi_other = ospf_if_entry_match (top_alt, oi);
                        if (oi_other)
                          {
                            oi_other->params_default = NULL;
                            if (oi_other->params == oip)
                              oi_other->params = NULL;

                          }
                      }
                  }
#endif /* HAVE_OSPF_MULTI_INST */
              oi->params_default = NULL;
              if (oi->params == oip)
                oi->params = NULL;
#ifdef HAVE_OSPF_MULTI_AREA
              LIST_LOOP(om->ospf, top, node)
                {
                  for (rn = ls_table_top (top->multi_area_link_table); rn;
                       rn = ls_route_next (rn))
                  if ((mlink = RN_INFO (rn, RNI_DEFAULT)))
                    if (mlink->oi && mlink->oi->oi_primary == oi)
                      {
                        mlink->oi->params_default = NULL;
                        if (mlink->oi->params == oip)
                          mlink->oi->params = NULL;
                      }                    
                }  
#endif /* HAVE_OSPF_MULTI_AREA */
            }
        }
    }
  else
    {
      vlink = ospf_vlink_lookup_by_name (om, oip->desc);
      if (vlink != NULL)
        if (vlink->oi != NULL)
          {
            vlink->oi->params_default = NULL;
            if (vlink->oi->params == oip)
              vlink->oi->params = NULL;
          }
    }
}

struct ospf_if_params *
ospf_if_params_lookup (struct ospf_master *om, char *desc, struct ls_prefix *p)
{
  struct ls_table *rt;
  struct ls_node *rn;

  rt = ospf_if_params_table_lookup_by_name (om, desc);
  if (rt)
    {
      rn = ls_node_lookup (rt, p);
      if (rn)
        {
          ls_unlock_node (rn);
          return RN_INFO (rn, RNI_DEFAULT);
        }
    }
  return NULL;
}

struct ospf_if_params *
ospf_if_params_lookup_default (struct ospf_master *om, char *desc)
{
  return ospf_if_params_lookup (om, desc,
                                (struct ls_prefix *)&LsPrefixIPv4Default);
}

struct ospf_if_params *
ospf_if_params_get (struct ospf_master *om, char *desc, struct ls_prefix *p)
{
  struct ls_table *rt;
  struct ls_node *rn;
  struct ospf_if_params *oip;

  rt = ospf_if_params_table_get (om, desc);
  rn = ls_node_get (rt, p);
  if ((RN_INFO (rn, RNI_DEFAULT)) == NULL)
    {
      oip = ospf_if_params_new (om);
      RN_INFO_SET (rn, RNI_DEFAULT, oip);
      oip->desc = rt->desc;
      ospf_if_params_link (om, oip, p);
      ospf_if_params_reset (oip);
    }
  ls_unlock_node (rn);

  return RN_INFO (rn, RNI_DEFAULT);
}

struct ospf_if_params *
ospf_if_params_get_default (struct ospf_master *om, char *desc)
{
  struct ospf_if_params *oip;

  oip = ospf_if_params_get (om, desc,
                            (struct ls_prefix *)&LsPrefixIPv4Default);
  ospf_if_params_link_default (om, oip);

  return oip;
}

void
ospf_if_params_delete (struct ospf_master *om, char *desc, struct ls_prefix *p)
{
  struct ls_table *rt;
  struct ls_node *rn;

  rt = ospf_if_params_table_lookup_by_name (om, desc);
  if (rt)
    {
      rn = ls_node_lookup (rt, p);
      if (rn)
        {
          ospf_if_params_unlink (om, RN_INFO (rn, RNI_DEFAULT), p);
          ospf_if_params_free (RN_INFO (rn, RNI_DEFAULT));
          RN_INFO_UNSET (rn, RNI_DEFAULT);
          ls_unlock_node (rn);

          if (rt->top == NULL)
            ospf_if_params_table_delete (om, desc);
        }
    }
}

void
ospf_if_params_delete_default (struct ospf_master *om, char *desc)
{
  struct ospf_if_params *oip;

  oip = ospf_if_params_lookup_default (om, desc);
  if (oip)
    ospf_if_params_unlink_default (om, oip);

  ospf_if_params_delete (om, desc, (struct ls_prefix *)&LsPrefixIPv4Default);
}


int
ospf_if_params_priority_set (struct ospf_if_params *oip, u_char priority)
{
  if (oip->priority != priority)
    {
      oip->priority = priority;
      if (priority != OSPF_ROUTER_PRIORITY_DEFAULT)
        OSPF_IF_PARAM_SET (oip, PRIORITY);
      else
        OSPF_IF_PARAM_UNSET (oip, PRIORITY);
    }
  return OSPF_API_SET_SUCCESS;
}

int
ospf_if_params_priority_unset (struct ospf_if_params *oip)
{
  oip->priority = OSPF_ROUTER_PRIORITY_DEFAULT;
  OSPF_IF_PARAM_UNSET (oip, PRIORITY);

  return OSPF_API_SET_SUCCESS;
}

int
ospf_if_params_mtu_set (struct ospf_if_params *oip, u_int16_t mtu)
{
  if (oip->mtu != mtu)
    {
      oip->mtu = mtu;
      OSPF_IF_PARAM_SET (oip, MTU);
    }
  return OSPF_API_SET_SUCCESS;
}

int
ospf_if_params_mtu_unset (struct ospf_if_params *oip)
{
  oip->mtu = 0;
  OSPF_IF_PARAM_UNSET (oip, MTU);
  return OSPF_API_SET_SUCCESS;
}

int
ospf_if_params_mtu_ignore_set (struct ospf_if_params *oip)
{
  OSPF_IF_PARAM_SET (oip, MTU_IGNORE);
  return OSPF_API_SET_SUCCESS;
}

int
ospf_if_params_mtu_ignore_unset (struct ospf_if_params *oip)
{
  OSPF_IF_PARAM_UNSET (oip, MTU_IGNORE);
  return OSPF_API_SET_SUCCESS;
}

int
ospf_if_params_cost_set (struct ospf_if_params *oip, u_int32_t cost)
{
  if (cost < OSPF_OUTPUT_COST_MIN || cost > OSPF_OUTPUT_COST_MAX)
    return OSPF_API_SET_ERR_IF_COST_INVALID;

  oip->output_cost = cost;
  OSPF_IF_PARAM_SET (oip, OUTPUT_COST);

  return OSPF_API_SET_SUCCESS;
}

int
ospf_if_params_cost_unset (struct ospf_if_params *oip)
{
  oip->output_cost = OSPF_OUTPUT_COST_DEFAULT;
  OSPF_IF_PARAM_UNSET (oip, OUTPUT_COST);

  return OSPF_API_SET_SUCCESS;
}

#ifdef HAVE_OSPF_TE
int
ospf_if_params_te_metric_set (struct ospf_if_params *oip, u_int32_t metric)
{
  struct interface *ifp;

  if (metric < 1 || metric > 65535)
    return OSPF_API_SET_ERR_IF_COST_INVALID;

  oip->te_metric = metric;
  if (metric != 0)
    OSPF_IF_PARAM_SET (oip, TE_METRIC);
  else
    OSPF_IF_PARAM_UNSET (oip, TE_METRIC);

  /* Update the TE metric.  */
  ifp = if_lookup_by_name (&oip->om->vr->ifm, oip->desc);
  if (ifp != NULL)
    ospf_if_te_metric_update (ifp);

  return OSPF_API_SET_SUCCESS;
}

int
ospf_if_params_te_metric_unset (struct ospf_if_params *oip)
{
  struct interface *ifp;

  oip->te_metric = 0;
  OSPF_IF_PARAM_UNSET (oip, TE_METRIC);

  /* Update the TE metric.  */
  ifp = if_lookup_by_name (&oip->om->vr->ifm, oip->desc);
  if (ifp != NULL)
    ospf_if_te_metric_update (ifp);

  return OSPF_API_SET_SUCCESS;
}
#endif /* HAVE_OSPF_TE */

int
ospf_if_params_transmit_delay_set (struct ospf_if_params *oip,
                                   u_int32_t delay)
{
  if (delay < 1 || delay > 65535)
    return OSPF_API_SET_ERR_IF_TRANSMIT_DELAY_INVALID;

  if (oip->transmit_delay != delay)
    {
      oip->transmit_delay = delay;
      if (delay != OSPF_TRANSMIT_DELAY_DEFAULT)
        OSPF_IF_PARAM_SET (oip, TRANSMIT_DELAY);
      else
        OSPF_IF_PARAM_UNSET (oip, TRANSMIT_DELAY);
    }
  return OSPF_API_SET_SUCCESS;
}

int
ospf_if_params_transmit_delay_unset (struct ospf_if_params *oip)
{
  oip->transmit_delay = OSPF_TRANSMIT_DELAY_DEFAULT;
  OSPF_IF_PARAM_UNSET (oip, TRANSMIT_DELAY);

  return OSPF_API_SET_SUCCESS;
}

int
ospf_if_params_retransmit_interval_set (struct ospf_if_params *oip,
                                        u_int32_t interval)
{
  if (interval < OSPF_RETRANSMIT_INTERVAL_MIN
      || interval > OSPF_RETRANSMIT_INTERVAL_MAX)
    return OSPF_API_SET_ERR_IF_RETRANSMIT_INTERVAL_INVALID;

  if (oip->retransmit_interval != interval)
    {
      oip->retransmit_interval = interval;
      if (interval != OSPF_RETRANSMIT_INTERVAL_DEFAULT)
        OSPF_IF_PARAM_SET (oip, RETRANSMIT_INTERVAL);
      else
        OSPF_IF_PARAM_UNSET (oip, RETRANSMIT_INTERVAL);
    }
  return OSPF_API_SET_SUCCESS;
}

int
ospf_if_params_retransmit_interval_unset (struct ospf_if_params *oip)
{
  oip->retransmit_interval = OSPF_RETRANSMIT_INTERVAL_DEFAULT;
  OSPF_IF_PARAM_UNSET (oip, RETRANSMIT_INTERVAL);

  return OSPF_API_SET_SUCCESS;
}

int
ospf_if_params_hello_interval_set (struct ospf_if_params *oip,
                                   u_int32_t interval)
{
  if (oip->hello_interval != interval)
    {
      oip->hello_interval = interval;
      if (interval != OSPF_IF_PARAM_HELLO_INTERVAL_DEFAULT (oip))
        OSPF_IF_PARAM_SET (oip, HELLO_INTERVAL);
      else
        OSPF_IF_PARAM_UNSET (oip, HELLO_INTERVAL);
      OSPF_IF_PARAM_DEAD_INTERVAL_UPDATE (oip);

      /* Update the OSPF interface.  */
      ospf_if_update_by_params (oip);
    }
  return OSPF_API_SET_SUCCESS;
}

int
ospf_if_params_hello_interval_unset (struct ospf_if_params *oip)
{
  if (oip->hello_interval != OSPF_IF_PARAM_HELLO_INTERVAL_DEFAULT (oip))
    {
      oip->hello_interval = OSPF_IF_PARAM_HELLO_INTERVAL_DEFAULT (oip);
      OSPF_IF_PARAM_UNSET (oip, HELLO_INTERVAL);
      OSPF_IF_PARAM_DEAD_INTERVAL_UPDATE (oip);

      /* Update the OSPF interface.  */
      ospf_if_update_by_params (oip);
    }
  return OSPF_API_SET_SUCCESS;
}

int
ospf_if_params_dead_interval_set (struct ospf_if_params *oip,
                                  u_int32_t interval)
{
  if (oip->dead_interval != interval)
    {
      oip->dead_interval = interval;
      if (interval != OSPF_IF_PARAM_DEAD_INTERVAL_DEFAULT (oip))
        OSPF_IF_PARAM_SET (oip, DEAD_INTERVAL);
      else
        OSPF_IF_PARAM_UNSET (oip, DEAD_INTERVAL);

      /* Update the OSPF interface.  */
      ospf_if_update_by_params (oip);
    }
  return OSPF_API_SET_SUCCESS;
}

int
ospf_if_params_dead_interval_unset (struct ospf_if_params *oip)
{
  if (oip->dead_interval != OSPF_IF_PARAM_DEAD_INTERVAL_DEFAULT (oip))
    {
      oip->dead_interval = OSPF_IF_PARAM_DEAD_INTERVAL_DEFAULT (oip);
      OSPF_IF_PARAM_UNSET (oip, DEAD_INTERVAL);

      /* Update the OSPF interface.  */
      ospf_if_update_by_params (oip);
    }
  return OSPF_API_SET_SUCCESS;
}

int
ospf_if_params_authentication_type_set (struct ospf_if_params *oip,
                                        u_char type)
{
  if (type != OSPF_AUTH_NULL
      && type != OSPF_AUTH_SIMPLE
      && type != OSPF_AUTH_CRYPTOGRAPHIC)
    return OSPF_API_SET_ERR_AUTH_TYPE_INVALID;

  oip->auth_type = type;
  OSPF_IF_PARAM_SET (oip, AUTH_TYPE);

  return OSPF_API_SET_SUCCESS;
}

int
ospf_if_params_authentication_type_unset (struct ospf_if_params *oip)
{
  oip->auth_type = OSPF_AUTH_NULL;
  OSPF_IF_PARAM_UNSET (oip, AUTH_TYPE);

  return OSPF_API_SET_SUCCESS;
}

int
ospf_if_params_authentication_key_set (struct ospf_if_params *oip,
                                       char *auth_key)
{
  if (oip->auth_simple)
    XFREE (MTYPE_OSPF_AUTH_KEY, oip->auth_simple);

  oip->auth_simple = XMALLOC (MTYPE_OSPF_AUTH_KEY, OSPF_AUTH_SIMPLE_SIZE + 1);
  pal_mem_set (oip->auth_simple, 0, OSPF_AUTH_SIMPLE_SIZE + 1);
  pal_strncpy (oip->auth_simple, auth_key, OSPF_AUTH_SIMPLE_SIZE);

  OSPF_IF_PARAM_SET (oip, AUTH_SIMPLE);

  return OSPF_API_SET_SUCCESS;
}

int
ospf_if_params_authentication_key_unset (struct ospf_if_params *oip)
{
  if (oip->auth_simple)
    {
      XFREE (MTYPE_OSPF_AUTH_KEY, oip->auth_simple);
      oip->auth_simple = NULL;
      OSPF_IF_PARAM_UNSET (oip, AUTH_SIMPLE);

      return OSPF_API_SET_SUCCESS;
    }
  return OSPF_API_SET_ERR_IF_PARAM_NOT_CONFIGURED;
}

int
ospf_if_message_digest_passive_timer (struct thread *thread)
{
  struct ospf_crypt_key *ck = THREAD_ARG (thread);
  struct ospf_master *om = ck->om;

  ck->t_passive = NULL;
  LIB_GLOB_SET_VR_CONTEXT (ZG, om->vr);

  if (!CHECK_FLAG (ck->flags, OSPF_AUTH_MD5_KEY_PASSIVE))
    {
      SET_FLAG (ck->flags, OSPF_AUTH_MD5_KEY_PASSIVE);
      zlog_info (ZG, "Message-digest-key key-id %d enter Passive mode",
                 ck->key_id);
    }
  return 0;
}

int
ospf_if_params_message_digest_passive_set_all (struct ospf_if_params *oip)
{
  struct listnode *node;
  struct ospf_crypt_key *ck;

  /* Younger key (tail) never set to PASSIVE. */
  if (!oip->auth_crypt)
    return 0;

  LIST_LOOP (oip->auth_crypt, ck, node)
    if (node != oip->auth_crypt->tail)
      {
        if (!CHECK_FLAG (ck->flags, OSPF_AUTH_MD5_KEY_PASSIVE))
          {
            SET_FLAG (ck->flags, OSPF_AUTH_MD5_KEY_PASSIVE);
            zlog_info (ZG, "Message-digest-key key-id %d enter Passive mode",
                        ck->key_id);
          }
      }
  return 1;
}

int
ospf_if_params_message_digest_key_set (struct ospf_if_params *oip,
                                       u_char key_id, char *auth_key,
                                       struct ospf_master *om)
{
  struct ospf_crypt_key *ck;
  struct ls_node *rn1 = NULL;
  struct ls_node *rn2 = NULL;
  struct ospf_interface *oi = NULL;
  struct ospf_neighbor *nbr = NULL;
  struct listnode *node;
#ifdef HAVE_OSPF_MULTI_INST
  struct ospf *top_alt = NULL;
  struct ospf_interface *oi_other = NULL;
#endif /* HAVE_OSPF_MULTI_INST */
#ifdef HAVE_OSPF_MULTI_AREA
  struct ospf_multi_area_link *mlink = NULL;
  struct ospf *top = NULL;
#endif /* HAVE_OSPF_MULTI_AREA */

  if (oip->auth_crypt)
    {
      if (ospf_crypt_key_lookup (oip->auth_crypt, key_id) != NULL)
        return OSPF_API_SET_ERR_MD5_KEY_EXIST;
    }
  else
    {
      oip->auth_crypt = list_new ();
      oip->auth_crypt->cmp = ospf_crypt_key_cmp;
    }

  ck = ospf_crypt_key_new ();
  ck->key_id = key_id;
  ck->flags = 0;
  /* Set the OSPF master pointer. */
  ck->om = om;
  pal_mem_set (ck->auth_key, 0, OSPF_AUTH_MD5_SIZE + 1);
  pal_strncpy ((char *) ck->auth_key, auth_key, OSPF_AUTH_MD5_SIZE);

  ospf_crypt_key_add (oip->auth_crypt, ck);
  OSPF_IF_PARAM_SET (oip, AUTH_CRYPT);

  /*if there are more than one key configured, mark all neighbor in rollover 
    state. Go through all key and start timer for all key except tail */
  if (listcount (oip->auth_crypt) > 1)
    {
      /* Mark all neighbors rollover state */
      for (rn1 = ls_table_top (om->if_table); rn1; rn1 = ls_route_next (rn1))
        if ((oi = RN_INFO (rn1, RNI_OSPF_IF)))
          {
#ifdef HAVE_OSPF_MULTI_INST
            /* If multiple instances are running on this interface */
            if (CHECK_FLAG (oi->flags, OSPF_IF_EXT_MULTI_INST))
              LIST_LOOP(om->ospf, top_alt, node)
                {
                  if (top_alt != oi->top)
                    {
                      /* Delete oi from interface table of each instance */
                      oi_other = ospf_if_entry_match (top_alt, oi);
                      if (oi_other)
                        {
                          if (oi_other->params == oip || oi_other->params_default == oip)
                            for (rn2 = ls_table_top (oi_other->nbrs); rn2; 
                                 rn2 = ls_route_next (rn2))
                              if ((nbr = RN_INFO (rn2, RNI_DEFAULT)))
                                SET_FLAG (nbr->flags, OSPF_NEIGHBOR_MD5_KEY_ROLLOVER);
                        }
                    }
                }
#endif /* HAVE_OSPF_MULTI_INST */
            if (oi->params == oip || oi->params_default == oip)
              for (rn2 = ls_table_top (oi->nbrs); rn2; rn2 = ls_route_next (rn2))
                if ((nbr = RN_INFO (rn2, RNI_DEFAULT)))
                  SET_FLAG (nbr->flags, OSPF_NEIGHBOR_MD5_KEY_ROLLOVER);
          }
#ifdef HAVE_OSPF_MULTI_AREA
      LIST_LOOP(om->ospf, top, node)
        {
          /* Mark all multi-adj neighbors rollover state */
          for (rn1 = ls_table_top (top->multi_area_link_table); rn1;
               rn1 = ls_route_next (rn1))
          if ((mlink = RN_INFO (rn1, RNI_DEFAULT)))
            if (mlink->oi)
              if (mlink->oi->params == oip || mlink->oi->params_default == oip)
                for (rn2 = ls_table_top (mlink->oi->nbrs); 
                     rn2; rn2 = ls_route_next (rn2))
                  if ((nbr = RN_INFO (rn2, RNI_DEFAULT)))
                    SET_FLAG (nbr->flags, OSPF_NEIGHBOR_MD5_KEY_ROLLOVER);
        }
#endif /* HAVE_OSPF_MULTI_AREA */

      /* Go through ck list, if not tail, start timer.
         When timer expired, ck will be marked PASSIVE. */
      LIST_LOOP (oip->auth_crypt, ck, node)
        if (node != oip->auth_crypt->tail)
          {
            OSPF_TIMER_ON (ck->t_passive, 
                           ospf_if_message_digest_passive_timer, ck,
                           OSPF_ROUTER_DEAD_INTERVAL_DEFAULT); 
          }
    }

  return OSPF_API_SET_SUCCESS;
}

int
ospf_if_params_message_digest_key_unset (struct ospf_if_params *oip,
                                         u_char key_id)
{
  struct ospf_crypt_key *ck;
  struct listnode *node;

  if (oip->auth_crypt)
    {
      ck = ospf_crypt_key_lookup (oip->auth_crypt, key_id);
      if (ck == NULL)
        return OSPF_API_SET_ERR_MD5_KEY_NOT_EXIST;

      ospf_crypt_key_delete (oip->auth_crypt, key_id);
      if (listcount (oip->auth_crypt) == 0)
        {
          list_delete (oip->auth_crypt);
          oip->auth_crypt = NULL;
          OSPF_IF_PARAM_UNSET (oip, AUTH_CRYPT);
        }
      /* Make sure tail key is not in PASSIVE mode. */
      else
        {
          node = oip->auth_crypt->tail;
          ck = GETDATA (node);
          if (ck != NULL)
            {
              if (CHECK_FLAG (ck->flags, OSPF_AUTH_MD5_KEY_PASSIVE))
              UNSET_FLAG (ck->flags, OSPF_AUTH_MD5_KEY_PASSIVE);
            }
        }

      return OSPF_API_SET_SUCCESS;
    }

  return OSPF_API_SET_ERR_IF_PARAM_NOT_CONFIGURED;
}

int
ospf_if_params_database_filter_set (struct ospf_if_params *oip)
{
  OSPF_IF_PARAM_SET (oip, DATABASE_FILTER);

  return OSPF_API_SET_SUCCESS;
}

int
ospf_if_params_database_filter_unset (struct ospf_if_params *oip)
{
  OSPF_IF_PARAM_UNSET (oip, DATABASE_FILTER);

  return OSPF_API_SET_SUCCESS;
}

int
ospf_if_params_disable_all_set (struct ospf_if_params *oip)
{
  if (!OSPF_IF_PARAM_CHECK (oip, DISABLE_ALL))
    OSPF_IF_PARAM_SET (oip, DISABLE_ALL);

  return OSPF_API_SET_SUCCESS;
}

int
ospf_if_params_disable_all_unset (struct ospf_if_params *oip)
{
  if (OSPF_IF_PARAM_CHECK (oip, DISABLE_ALL))
    OSPF_IF_PARAM_UNSET (oip, DISABLE_ALL);

  return OSPF_API_SET_SUCCESS;
}

#ifdef HAVE_RESTART
int
ospf_if_params_resync_timeout_set (struct ospf_if_params *oip,
                                   u_int32_t timeout)
{
  if (timeout < 1 || timeout > 65535)
    return OSPF_API_SET_ERR_IF_RESYNC_TIMEOUT_INVALID;

  oip->resync_timeout = timeout;
  OSPF_IF_PARAM_SET (oip, RESYNC_TIMEOUT);

  return OSPF_API_SET_SUCCESS;
}

int
ospf_if_params_resync_timeout_unset (struct ospf_if_params *oip)
{
  oip->resync_timeout = OSPF_RESYNC_TIMEOUT_DEFAULT;
  OSPF_IF_PARAM_UNSET (oip, RESYNC_TIMEOUT);

  return OSPF_API_SET_SUCCESS;
}
#endif /* HAVE_RESTART */

#ifdef HAVE_BFD
int
ospf_if_params_bfd_set (struct ospf_if_params *oip)
{
  OSPF_IF_PARAM_SET (oip, BFD);
  OSPF_IF_PARAM_UNSET (oip, BFD_DISABLE);
  return OSPF_API_SET_SUCCESS;
}

int
ospf_if_params_bfd_unset (struct ospf_if_params *oip)
{
  OSPF_IF_PARAM_UNSET (oip, BFD);
  OSPF_IF_PARAM_UNSET (oip, BFD_DISABLE);
  return OSPF_API_SET_SUCCESS;
}

int
ospf_if_params_bfd_disable_set (struct ospf_if_params *oip)
{
  OSPF_IF_PARAM_SET (oip, BFD);
  OSPF_IF_PARAM_SET (oip, BFD_DISABLE);
  return OSPF_API_SET_SUCCESS;
}

int
ospf_if_params_bfd_disable_unset (struct ospf_if_params *oip)
{
  OSPF_IF_PARAM_UNSET (oip, BFD);
  OSPF_IF_PARAM_UNSET (oip, BFD_DISABLE);
  return OSPF_API_SET_SUCCESS;
}
#endif /* HAVE_BFD */


/* Initialize OSPF interface master for system wide configurations. */
void
ospf_if_master_init (struct ospf_master *om)
{
  /* Initialize ospf interface instance table. */
  om->if_table = ls_table_init (OSPF_GLOBAL_IF_TABLE_DEPTH, 1);

  /* Initialize ospf interface parameter list. */
  om->if_params = list_new ();
  om->if_params->cmp = ospf_if_params_table_cmp;
  om->if_params->del = ospf_if_params_table_free;
}

void
ospf_if_master_finish (struct ospf_master *om)
{
  /* Finish global interface table.  */
  TABLE_FINISH (om->if_table);

  /* Finish global interface parameter list.  */
  LIST_DELETE (om->if_params);
}
