/* Copyright (C) 2001-2003  All Rights Reserved. */

#ifndef _PACOS_OSPF_NSM_H
#define _PACOS_OSPF_NSM_H

#include "ospfd/ospf_route.h"

#define EXTERNAL_METRIC_VALUE_UNSPEC            0xFFFFFF
#define OSPF_REDISTRIBUTE_TIMER_DELAY           1
#define OSPF_REDIST_INSTANCE_VALUE_UNSPEC       0x10000
#define OSPF_MAXIMUM_NUM_OF_NEXTHOPS_PER_ROUTE  255

#ifdef HAVE_VRF_OSPF
struct ospf_domain_info
{
  u_char domain_id[8];
  struct pal_in4_addr area_id;
  struct pal_in4_addr router_id;
  u_char r_type;
  u_char metric_type_E2;
};
#endif /* HAVE_VRF_OSPF */

struct ospf_redist_info
{
  /* Flags. */
  u_char flags;
#define OSPF_REDIST_INFO_DELETED        (1 << 0)
#ifdef HAVE_VRF_OSPF
#define OSPF_REDIST_VRF_INFO            (1 << 1)
#endif /* HAVE_VRF_OSPF  */
#define OSPF_STATIC_TAG_VALUE         (1<<2)

  /* Route type. */
  u_char type;

  /* Lock. */
  u_int16_t lock;

  /* Route node. */
  struct ls_node *rn;

  /* Timestamp. */
  struct pal_timeval tv_update;

  /* Nexthop. */
  struct
  {
    /* Interface index. */
    int ifindex;

    /* Address. */
    struct pal_in4_addr addr;

  } nexthop[1];

  /* Distance. */

  /* Metric.  */
  u_int32_t metric;

  /* Count for same redistribution */
  u_int32_t count;

#ifdef HAVE_VRF_OSPF
  /* OSPF Domain info */
  struct ospf_domain_info domain_info;
#endif /* HAVE_VRF_OSPF */

  /* Process id */
  u_int32_t pid;

  /* Static route tag */
  u_int32_t tag;
};

struct ospf_distance
{
  /* Distance value for the IP source prefix. */
  u_char distance;

  /* Name of the access-list to be matched. */
  char *access_list;
};

#define OSPF_NSM_ADD(T, P, R, S)                                              \
    do {                                                                      \
      if ((S)->type == OSPF_PATH_DISCARD)                                     \
        ospf_nsm_add_discard (T, P);                                          \
      else                                                                    \
        {                                                                     \
          ospf_nsm_add (T, P, R);                                             \
          ospf_nsm_igp_shortcut_route_add (T, P, R);                          \
        }                                                                     \
    } while (0)

#define OSPF_NSM_DELETE(T, P, R, S)                                           \
    do {                                                                      \
      if ((S)->type == OSPF_PATH_DISCARD)                                     \
        ospf_nsm_delete_discard (T, P);                                       \
      else                                                                    \
        {                                                                     \
          ospf_nsm_delete (T, P);                                             \
          ospf_nsm_igp_shortcut_route_del (T, P, R, S);                       \
        }                                                                     \
    } while (0)


/* Prototypes */
struct ospf_distance *ospf_distance_new ();
void ospf_distance_free (struct ospf_distance *);
struct ospf_distance *ospf_distance_get (struct ospf *, struct pal_in4_addr,
                                         u_char);
struct ospf_distance *ospf_distance_lookup (struct ospf *, struct pal_in4_addr,
                                            u_char);
void ospf_distance_delete (struct ospf *, struct pal_in4_addr, u_char);
void ospf_distance_access_list_set (struct ospf_distance *, char *);
void ospf_distance_table_clear (struct ospf *);
int ospf_distance_update (struct ospf *, struct ospf_route *);
void ospf_distance_update_all (struct ospf *);
void ospf_distance_update_by_type (struct ospf *, u_char);

struct ospf_redist_map *ospf_redist_map_new ();
void ospf_redist_map_free (struct ospf_redist_map *);
void ospf_redist_map_lsa_refresh (struct ospf_redist_map *);
void ospf_redist_map_lsa_delete (struct ospf *, struct ospf_lsa *, struct ospf_lsdb *);
void ospf_redist_map_default_update (struct ls_table *, u_char, void *);
#ifdef HAVE_NSSA
int ospf_redist_map_nssa_update (struct ospf_lsa *);
int ospf_redist_map_nssa_delete (struct ospf_lsa *);
#endif /* HAVE_NSSA */
void ospf_redist_map_update_by_prefix (struct ospf *, struct prefix *);
void ospf_redist_map_delete_all (struct ls_table *);
void ospf_redist_map_lsa_update_all (struct ospf *);
#ifdef HAVE_VRF_OSPF
bool_t ospf_vrf_compare_domain_id (struct ospf *, struct ospf_redist_info *);
#endif
void ospf_redistribute_timer_add (struct ospf *, u_char);
#ifdef HAVE_NSSA
void ospf_nssa_redistribute_timer_add (struct ospf_area *, u_char);
void ospf_nssa_redistribute_timer_add_all_proto (struct ospf_area *);
void ospf_nssa_redistribute_timer_delete_all_proto (struct ospf_area *);
#endif /* HAVE_NSSA */
void ospf_redistribute_timer_add_all_proto (struct ospf *);
void ospf_redistribute_timer_delete_all_proto (struct ospf *);

void ospf_redist_info_free (struct ospf_redist_info *);
struct ospf_redist_info *ospf_redist_info_lock (struct ospf_redist_info *);
void ospf_redist_info_unlock (struct ospf_redist_info *);

int ospf_redist_conf_set (struct ospf *, int, int);
int ospf_redist_conf_unset (struct ospf *, int, int);
int ospf_redist_conf_default_set (struct ospf *, int);
int ospf_redist_conf_default_unset (struct ospf *);
int ospf_redist_conf_unset_all_proto (struct ospf *);
int ospf_redist_conf_metric_type_set (struct ospf *, int, int, int);
int ospf_redist_conf_metric_type_unset (struct ospf *, int, int);
int ospf_redist_conf_metric_set (struct ospf *, int, int, int);
int ospf_redist_conf_metric_unset (struct ospf *, int, int);
int ospf_redist_conf_default_metric_set (struct ospf *, int);
int ospf_redist_conf_default_metric_unset (struct ospf *);
int ospf_redist_conf_tag_set (struct ospf *, int, u_int32_t, int);
int ospf_redist_conf_tag_unset (struct ospf *, int, int);
int ospf_redist_conf_distribute_list_set (struct ospf *, int, int, char *);
int ospf_redist_conf_distribute_list_unset (struct ospf *, int, int, char *);
int ospf_redist_conf_route_map_set (struct ospf *, int, char *, int);
int ospf_redist_conf_route_map_unset (struct ospf *, int, int);
#ifdef HAVE_NSSA
int ospf_redist_conf_nssa_no_redistribute_set (struct ospf_area *);
int ospf_redist_conf_nssa_no_redistribute_unset (struct ospf_area *);
int ospf_redist_conf_nssa_default_set (struct ospf_area *);
int ospf_redist_conf_nssa_default_unset (struct ospf_area *);
int ospf_redist_conf_nssa_default_metric_set (struct ospf_area *, int);
int ospf_redist_conf_nssa_default_metric_type_set (struct ospf_area *, int);
#endif /* HAVE_NSSA */

void ospf_nsm_add (struct ospf *, struct ls_prefix *, struct ospf_route *);
void ospf_nsm_delete (struct ospf *, struct ls_prefix *);
void ospf_nsm_add_discard (struct ospf *, struct ls_prefix *);
void ospf_nsm_delete_discard (struct ospf *, struct ls_prefix *);
void ospf_nsm_igp_shortcut_route_add (struct ospf *, struct ls_prefix *,
                                      struct ospf_route *);
void ospf_nsm_igp_shortcut_route_del (struct ospf *, struct ls_prefix *,
                                      struct ospf_route *, struct ospf_path *);
void ospf_nsm_preserve_set (struct ospf_master *);
void ospf_nsm_preserve_unset (struct ospf_master *);
void ospf_nsm_flush (struct ospf_master *);
void ospf_nsm_stale_remove (struct ospf_master *);
void ospf_nsm_debug_set (struct ospf_master *);
void ospf_nsm_debug_unset (struct ospf_master *);

void ospf_nsm_init (struct lib_globals *);

#endif /* _PACOS_OSPF_NSM_H */
