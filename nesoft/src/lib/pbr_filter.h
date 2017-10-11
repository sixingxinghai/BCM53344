/* Copyright (C) 2008  All Rights Reserved. */

#ifndef _PACOS_PBR_FILTER_H
#define _PACOS_PBR_FILTER_H

#include "pal.h"
#include "routemap.h"

#define PBR_SUCCESS                0
#define PBR_RMAP_NOT_FOUND         -1

/* route map interface structure */
/* Keeps track of the number of interfaces on which a particular routemap
   is configured */
struct route_map_if
{
  char *route_map_name;
  struct list *ifnames;
};

/* nexthop declaration */
/* keeps information about the nexthop value, ifname and type configured
   in set clause */
struct pbr_nexthop
{
  char *nexthop_str;
  bool_t  nh_type;
  char *ifname;
};

enum pbr_cmd
{
   RMAP_POLICY_ADD,
   RMAP_POLICY_DELETE
};

struct pbr_rmap_set
{
  enum pbr_cmd cmd;
  struct route_map_index *index;
  struct pbr_nexthop *pbr_nexthop;
};

struct pbr_rmap_match
{
  enum pbr_cmd cmd;
  char *map_name;
  struct route_map_index *index;
  char *rule_str;
};

struct pbr_acl
{
  enum pbr_cmd cmd;
  char *acl_name;
  struct filter_list *filter;
};

typedef enum
{
  PBR_RMAP_EVENT_SET_IP_NEXTHOP_ADD_DEL,
  PBR_RMAP_EVENT_MATCH_IP_ADDR_ADD_DEL,
  PBR_RMAP_EVENT_INDEX_DELETE
}pbr_rmap_event_t;

struct pbr_rmap_event 
{
  void (*event_hook)(pbr_rmap_event_t, struct apn_vr *, void *);
};

struct pbr_rmap_event pbr_event;


struct pbr_nexthop *pbr_nexthop_copy (struct pbr_nexthop *,
                                      struct pbr_nexthop *, s_int32_t );

void pbr_nexthop_free (struct pbr_nexthop *);
void pbr_filter_copy_common (struct filter_pacos_ext *,
                        struct filter_common *);
#endif /* _PACOS_PBR_FILTER_H */

