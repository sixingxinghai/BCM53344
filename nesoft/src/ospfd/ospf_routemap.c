/* Copyright (C) 2001-2003  All Rights Reserved. */

#include "pal.h"
#include "lib.h"
#include "if.h"
#include "prefix.h"
#include "filter.h"
#include "plist.h"
#include "linklist.h"

#include "ospfd/ospfd.h"
#include "ospfd/ospf_interface.h"
#include "ospfd/ospf_lsa.h"
#include "ospfd/ospf_asbr.h"
#include "ospfd/ospf_nsm.h"


/* Hook function for updating route_map assignment. */
void
ospf_route_map_update (struct apn_vr *vr, char *name)
{
  struct ospf_master *om = vr->proto;
  struct listnode *node;
  struct ospf *top;
  struct ospf_redist_conf *rc;
  int type;

  /* Update route-map.  */
  LIST_LOOP (om->ospf, top, node)
    for (type = 0; type < APN_ROUTE_MAX; type++)
     for (rc = &top->redist[type]; rc; rc = rc->next)
      if (RMAP_NAME (rc)
          && pal_strcmp (RMAP_NAME (rc), name) == 0)
        {
          /* Keep old route-map.  */
          struct route_map *old = RMAP_MAP (rc);

          /* Update route-map.  */
          RMAP_MAP (rc) =
            route_map_lookup_by_name (vr, RMAP_NAME (rc));

          /* No update for this distribute type.  */
          if (old == NULL && RMAP_MAP (rc) == NULL)
            continue;

          ospf_redistribute_timer_add (top, type);
        }
}

void
ospf_route_map_event (struct apn_vr *vr, route_map_event_t event, char *name)
{
  struct ospf_master *om = vr->proto;
  struct listnode *node;
  struct ospf *top;
  struct ospf_redist_conf *rc;
  int type;

  /* Update route-map.  */
  LIST_LOOP (om->ospf, top, node)
    for (type = 0; type < APN_ROUTE_MAX; type++)
     for (rc = &top->redist[type]; rc; rc = rc->next)
      if (RMAP_NAME (rc)
          && RMAP_MAP (rc)
          && !pal_strcmp (RMAP_NAME (rc), name))
        ospf_redistribute_timer_add (top, type);
}

/* `match ip netxthop' */
/* Match function return 1 if match is success else return zero. */
route_map_result_t
ospf_route_match_ip_nexthop (void *rule, struct prefix *prefix,
                             struct route_map_rule *type, void *object)
{
  enum filter_type acode = FILTER_NO_MATCH;
  struct ospf_route_map_arg *rma = object;
  struct ospf_master *om = rma->top->om;
  struct access_list *alist;
  struct prefix_ipv4 p;

  p.family = AF_INET;
  p.prefix = rma->nsm_nexthop;
  p.prefixlen = IPV4_MAX_BITLEN;

  alist = access_list_lookup (om->vr, AFI_IP, (char *) rule);
  if (alist != NULL)
    acode = access_list_apply (alist, &p);

  return route_map_alist2rmap_rcode (acode);
}

/* Route map `ip next-hop' match statement. `arg' should be
   access-list name. */
void *
ospf_route_match_ip_nexthop_compile (char *arg)
{
  if (! access_list_reference_validate (arg))
    return NULL;

  return XSTRDUP (MTYPE_ROUTE_MAP_COMPILED, arg);
}

/* Free route map's compiled `ip address' value. */
void
ospf_route_match_ip_nexthop_free (void *rule)
{
  XFREE (MTYPE_ROUTE_MAP_COMPILED, rule);
}

/* Route map commands for metric matching. */
struct route_map_rule_cmd ospf_route_match_ip_nexthop_cmd =
{
  "ip next-hop",
  ospf_route_match_ip_nexthop,
  ospf_route_match_ip_nexthop_compile,
  ospf_route_match_ip_nexthop_free
};

/* `match ip address IP_ACCESS_LIST' */
/* Match function should return 1 if match is success else return
   zero. */
route_map_result_t
ospf_route_match_ip_address (void *rule, struct prefix *prefix,
                             struct route_map_rule *type, void *object)
{
  enum filter_type acode = FILTER_NO_MATCH;
  struct ospf_route_map_arg *rma = object;
  struct ospf_master *om = rma->top->om;
  struct access_list *alist;

  alist = access_list_lookup (om->vr, AFI_IP, (char *) rule);
  if (alist != NULL)
    acode = access_list_apply (alist, prefix);

  return route_map_alist2rmap_rcode (acode);
}

/* Route map `ip address' match statement.  `arg' should be
   access-list name. */
void *
ospf_route_match_ip_address_compile (char *arg)
{
  if (! access_list_reference_validate (arg))
    return NULL;

  return XSTRDUP (MTYPE_ROUTE_MAP_COMPILED, arg);
}

/* Free route map's compiled `ip address' value. */
void
ospf_route_match_ip_address_free (void *rule)
{
  XFREE (MTYPE_ROUTE_MAP_COMPILED, rule);
}

/* Route map commands for ip address matching. */
struct route_map_rule_cmd ospf_route_match_ip_address_cmd =
{
  "ip address",
  ospf_route_match_ip_address,
  ospf_route_match_ip_address_compile,
  ospf_route_match_ip_address_free
};

/* `match ip address prefix-list PREFIX_LIST' */
/* Match function should return 1 if match is success else return
   zero. */
route_map_result_t
ospf_route_match_ip_address_plist (void *rule, struct prefix *prefix,
                                   struct route_map_rule *type, void *object)
{
  enum prefix_list_type pcode = PREFIX_NO_MATCH;
  struct ospf_route_map_arg *rma = object;
  struct ospf_master *om = rma->top->om;
  struct prefix_list *plist;

  plist = prefix_list_lookup (om->vr, AFI_IP, (char *) rule);
  if (plist != NULL)
    pcode = prefix_list_apply (plist, prefix);

  return route_map_plist2rmap_rcode (pcode);
}

/* Route map `ip address prefix-list' match statement.  `arg' should be
   prefix-list name. */
void *
ospf_route_match_ip_address_plist_compile (char *arg)
{
  return XSTRDUP (MTYPE_ROUTE_MAP_COMPILED, arg);
}

/* Free route map's compiled `ip address prefix-list' value. */
void
ospf_route_match_ip_address_plist_free (void *rule)
{
  XFREE (MTYPE_ROUTE_MAP_COMPILED, rule);
}

/* Route map commands for ip address matching. */
struct route_map_rule_cmd ospf_route_match_ip_address_plist_cmd =
{
  "ip address prefix-list",
  ospf_route_match_ip_address_plist,
  ospf_route_match_ip_address_plist_compile,
  ospf_route_match_ip_address_plist_free
};

/* `match interface IFNAME' */
/* Match function should return 1 if match is success else return
   zero. */
route_map_result_t
ospf_route_match_interface (void *rule, struct prefix *prefix,
                            struct route_map_rule *type, void *object)
{
  struct ospf_route_map_arg *rma = object;
  struct ospf_master *om = rma->top->om;
  struct interface *ifp;

  ifp = if_lookup_by_name (&om->vr->ifm, (char *)rule);

  if (ifp == NULL || ifp->ifindex != rma->arg.ifindex)
    return RMAP_NOMATCH;

  return RMAP_MATCH;
}

/* Route map `interface' match statement.  `arg' should be interface name. */
void *
ospf_route_match_interface_compile (char *arg)
{
  return XSTRDUP (MTYPE_ROUTE_MAP_COMPILED, arg);
}

/* Free route map's compiled `interface' value. */
void
ospf_route_match_interface_free (void *rule)
{
  XFREE (MTYPE_ROUTE_MAP_COMPILED, rule);
}

/* Route map commands for ip address matching. */
struct route_map_rule_cmd ospf_route_match_interface_cmd =
{
  "interface",
  ospf_route_match_interface,
  ospf_route_match_interface_compile,
  ospf_route_match_interface_free
};

/* `match tag NUMBER' */
/* Match function should return 1 if match is success else return
   zero. */
route_map_result_t
ospf_route_match_tag (void *rule, struct prefix *prefix,
                      struct route_map_rule *type, void *object)
{
  struct ospf_route_map_arg *rma = object;
  u_int32_t tag = *(u_int32_t*)rule;

  if (rma->arg.tag != tag)
    return RMAP_NOMATCH;

  return RMAP_MATCH;
}

/* Route map `tag' match statement. */
void *
ospf_route_match_tag_compile (char *arg)
{
  int ret;
  u_int32_t val;
  u_int32_t *tag;

  /* +/- is not allowed. */
  if (! pal_char_isdigit ((int) arg[0]))
    return NULL;

  val = cmd_str2int (arg, &ret);
  if (ret < 0)
    return NULL;

  tag = XMALLOC (MTYPE_ROUTE_MAP_COMPILED, sizeof (u_int32_t));
  *tag = val;

  return tag;
}

/* Free route map's compiled `tag' value. */
void
ospf_route_match_tag_free (void *rule)
{
  XFREE (MTYPE_ROUTE_MAP_COMPILED, rule);
}

/* Route map commands for tag matching. */
struct route_map_rule_cmd ospf_route_match_tag_cmd =
{
  "tag",
  ospf_route_match_tag,
  ospf_route_match_tag_compile,
  ospf_route_match_tag_free
};

/* `match metric NUMBER' */
/* Match function should return 1 if match is success else return
   zero. */
route_map_result_t
ospf_route_match_metric (void *rule, struct prefix *prefix,
                         struct route_map_rule *type, void *object)
{
  struct ospf_route_map_arg *rma = object;
  u_int32_t metric = *(u_int32_t*)rule;

  if (rma->metric != metric)
    return RMAP_NOMATCH;

  return RMAP_MATCH;
}

/* Route map `metric' match statement. */
void *
ospf_route_match_metric_compile (char *arg)
{
  int ret;
  u_int32_t val;
  u_int32_t *metric;

  /* +/- is not allowed. */
  if (! pal_char_isdigit ((int) arg[0]))
    return NULL;

  val = cmd_str2int (arg, &ret);
  if (ret < 0)
    return NULL;

  metric = XMALLOC (MTYPE_ROUTE_MAP_COMPILED, sizeof (u_int32_t));
  *metric = val;

  return metric;
}

/* Free route map's compiled `metric' value. */
void
ospf_route_match_metric_free (void *rule)
{
  XFREE (MTYPE_ROUTE_MAP_COMPILED, rule);
}

/* Route map commands for tag matching. */
struct route_map_rule_cmd ospf_route_match_metric_cmd =
{
  "metric",
  ospf_route_match_metric,
  ospf_route_match_metric_compile,
  ospf_route_match_metric_free
};

/* `match metric route-type external [type-1 | type-2]' */
/* Match function should return 1 if match is success else return
   zero. */
route_map_result_t
ospf_route_match_route_type (void *rule, struct prefix *prefix,
                             struct route_map_rule *type, void *object)
{
  struct ospf_route_map_arg *rma = object;
  u_int32_t route_type = *(u_int32_t *)rule;

  if (rma->arg.metric_type != route_type)
    return RMAP_NOMATCH;

  return RMAP_MATCH;
}

/* Route map `route-type' match statement. */
void *
ospf_route_match_route_type_compile (char *arg)
{
  u_int32_t *type;

  type = XMALLOC (MTYPE_ROUTE_MAP_COMPILED, sizeof (u_int32_t));

  if (pal_strcmp (arg, "type-1") == 0)
    *type = EXTERNAL_METRIC_TYPE_1;
  else
    *type = EXTERNAL_METRIC_TYPE_2;

  return type;
}

/* Free route map's compiled `route-type' value. */
void
ospf_route_match_route_type_free (void *rule)
{
  XFREE (MTYPE_ROUTE_MAP_COMPILED, rule);
}

/* Route map commands for tag matching. */
struct route_map_rule_cmd ospf_route_match_route_type_cmd =
{
  "route-type external",
  ospf_route_match_route_type,
  ospf_route_match_route_type_compile,
  ospf_route_match_route_type_free
};


/* `set metric METRIC' */
/* Set metric to attribute. */
route_map_result_t
ospf_route_set_metric (void *rule, struct prefix *prefix,
                       struct route_map_rule *type, void *object)
{
  struct ospf_route_map_arg *rma;
  u_int32_t *metric;

  /* Fetch routemap's rule information. */
  metric = rule;
  rma = object;

  /* Set metric out value. */
  rma->arg.metric = *metric;

  return RMAP_OKAY;
}

/* set metric compilation. */
void *
ospf_route_set_metric_compile (char *arg)
{
  int ret;
  u_int32_t val;
  u_int32_t *metric;

  /* +/- is just dropped off. */
  if ((arg[0] == '+' || arg[0] == '-') && pal_char_isdigit ((int) arg[1]))
    val = cmd_str2int (arg + 1, &ret);
  else
    val = cmd_str2int (arg, &ret);

  if (ret < 0)
    return NULL;

  metric = XMALLOC (MTYPE_ROUTE_MAP_COMPILED, sizeof (u_int32_t));
  *metric = val;

  return metric;
}

/* Free route map's compiled `set metric' value. */
void
ospf_route_set_metric_free (void *rule)
{
  XFREE (MTYPE_ROUTE_MAP_COMPILED, rule);
}

/* Set metric rule structure. */
struct route_map_rule_cmd ospf_route_set_metric_cmd =
{
  "metric",
  ospf_route_set_metric,
  ospf_route_set_metric_compile,
  ospf_route_set_metric_free,
};

/* `set tag TAG' */
/* Set tag to attribute. */
route_map_result_t
ospf_route_set_tag (void *rule, struct prefix *prefix,
                    struct route_map_rule *type, void *object)
{
  struct ospf_route_map_arg *rma;
  u_int32_t *tag;

  /* Fetch routemap's rule information. */
  tag = rule;
  rma = object;

  /* Set metric out value. */
  rma->arg.tag = *tag;

  return RMAP_OKAY;
}

/* set tag compilation. */
void *
ospf_route_set_tag_compile (char *arg)
{
  int ret;
  u_int32_t val;
  u_int32_t *tag;

  /* +/- is not allowed. */
  if (! pal_char_isdigit ((int) arg[0]))
    return NULL;

  val = cmd_str2int (arg, &ret);
  if (ret < 0)
    return NULL;

  tag = XMALLOC (MTYPE_ROUTE_MAP_COMPILED, sizeof (u_int32_t));
  *tag = val;

  return tag;
}

/* Free route map's compiled `set tag' value. */
void
ospf_route_set_tag_free (void *rule)
{
  XFREE (MTYPE_ROUTE_MAP_COMPILED, rule);
}

/* Set tag rule structure. */
struct route_map_rule_cmd ospf_route_set_tag_cmd =
{
  "tag",
  ospf_route_set_tag,
  ospf_route_set_tag_compile,
  ospf_route_set_tag_free,
};

/* `set metric-type TYPE' */
/* Set metric-type to attribute. */
route_map_result_t
ospf_route_set_metric_type (void *rule, struct prefix *prefix,
                            struct route_map_rule *type, void *object)
{
  struct ospf_route_map_arg *rma;
  u_int32_t *metric_type;

  /* Fetch routemap's rule information. */
  metric_type = rule;
  rma = object;

  /* Set metric out value. */
  rma->arg.metric_type = *metric_type;

  return RMAP_OKAY;
}

/* set metric-type compilation. */
void *
ospf_route_set_metric_type_compile (char *arg)
{
  u_int32_t *type;

  type = XMALLOC (MTYPE_ROUTE_MAP_COMPILED, sizeof (u_int32_t));
  if (pal_strcmp (arg, "type-1") == 0)
    {
      *type = EXTERNAL_METRIC_TYPE_1;
      return type;
    }
  else
    {
      *type = EXTERNAL_METRIC_TYPE_2;
      return type;
    }

  XFREE (MTYPE_ROUTE_MAP_COMPILED, type);
  return NULL;
}

/* Free route map's compiled `set metric-type' value. */
void
ospf_route_set_metric_type_free (void *rule)
{
  XFREE (MTYPE_ROUTE_MAP_COMPILED, rule);
}

/* Set metric rule structure. */
struct route_map_rule_cmd ospf_route_set_metric_type_cmd =
{
  "metric-type",
  ospf_route_set_metric_type,
  ospf_route_set_metric_type_compile,
  ospf_route_set_metric_type_free,
};

/* `set ip next-hop ADDR' */
route_map_result_t
ospf_route_set_ip_next_hop (void *rule, struct prefix *prefix,
                            struct route_map_rule *type, void *object)
{
  struct ospf_route_map_arg *rma;
  struct pal_in4_addr *nh;

  /* Fetch routemap's rule information. */
  nh = rule;
  rma = object;

  /* Set metric out value. */
  rma->arg.nexthop = *nh;

  return RMAP_OKAY;
}

/* set ip next-hop compilation. */
void *
ospf_route_set_ip_next_hop_compile (char *arg)
{
  struct pal_in4_addr *nh;

  nh = XMALLOC (MTYPE_ROUTE_MAP_COMPILED, sizeof (struct pal_in4_addr));
  if (pal_inet_pton (AF_INET, arg, nh))
    return nh;

  XFREE (MTYPE_ROUTE_MAP_COMPILED, nh);
  return NULL;
}

/* Free route map's compiled `set next-hop' value. */
void
ospf_route_set_ip_next_hop_free (void *rule)
{
  XFREE (MTYPE_ROUTE_MAP_COMPILED, rule);
}

/* Set tag rule structure. */
struct route_map_rule_cmd ospf_route_set_ip_next_hop_cmd =
{
  "ip next-hop",
  ospf_route_set_ip_next_hop,
  ospf_route_set_ip_next_hop_compile,
  ospf_route_set_ip_next_hop_free,
};

/* Route-map init. */
void
ospf_route_map_init (struct ospf_master *om)
{
  route_map_add_hook (om->vr, ospf_route_map_update);
  route_map_delete_hook (om->vr, ospf_route_map_update);
  route_map_event_hook (om->vr, ospf_route_map_event);
 
  route_map_install_match (om->vr, &ospf_route_match_ip_nexthop_cmd);
  route_map_install_match (om->vr, &ospf_route_match_ip_address_cmd);
  route_map_install_match (om->vr, &ospf_route_match_ip_address_plist_cmd);
  route_map_install_match (om->vr, &ospf_route_match_interface_cmd);
  route_map_install_match (om->vr, &ospf_route_match_tag_cmd);
  route_map_install_match (om->vr, &ospf_route_match_metric_cmd);
  route_map_install_match (om->vr, &ospf_route_match_route_type_cmd);
 
  route_map_install_set (om->vr, &ospf_route_set_metric_cmd);
  route_map_install_set (om->vr, &ospf_route_set_metric_type_cmd);
  route_map_install_set (om->vr, &ospf_route_set_tag_cmd);
  route_map_install_set (om->vr, &ospf_route_set_ip_next_hop_cmd);
}
