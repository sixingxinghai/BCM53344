/* Copyright (C) 2001-2003  All Rights Reserved. */

#ifndef _PACOS_OSPF_ASBR_H
#define _PACOS_OSPF_ASBR_H

#define OSPF_SUMMARY_TIMER_DELAY        1

struct ospf_redist_arg
{
  /* Metric type. */
  char metric_type;

#ifdef HAVE_NSSA
  /* Propagate bit. */
  char propagate;
#endif /* HAVE_NSSA */

  /* Metric. */
  u_int32_t metric;

  /* Nexthop. */
  struct pal_in4_addr nexthop;

  /* Tag. */
  u_int32_t tag;

  /* Interface index. */
  int ifindex;
};

struct ospf_redist_map
{
  /* LSA type. */
  u_char type;

  /* Source. */
  u_char source;
#define OSPF_REDIST_MAP_NSM             (1 << 0)
#define OSPF_REDIST_MAP_TRANSLATE       (1 << 1)
#define OSPF_REDIST_MAP_SUMMARY         (1 << 2)
#define OSPF_REDIST_MAP_CONFIG_DEFAULT  (1 << 3)

  /* Flags. */
  u_char flags;
#define OSPF_REDIST_MAP_MATCH_SUMMARY   (1 << 0)
#define OSPF_REDIST_MAP_NOT_ADVERTISE   (1 << 1)
#define OSPF_REDIST_EXTERNAL_SET_DN_BIT (1 << 2)

  /* Prefix. */
  struct ls_prefix *lp;

  /* Parent structure. */
  union
  {
    /* OSPF process for AS-external-LSA. */
    struct ospf *top;
#ifdef HAVE_NSSA
    /* OSPF area for NSSA-LSA. */
    struct ospf_area *area;
#endif /* HAVE_NSSA */

    /* Dummy member. */
    void *parent;
  } u;

  /* Redistribute arguments. */
  struct ospf_redist_arg arg;

  /* Pointer to redist info. */
  struct ospf_redist_info *ri;

  /* Pointer to summary. */
  struct ospf_summary *summary;
#ifdef HAVE_NSSA
  /* Poitner to NSSA-LSA. */
  struct ospf_lsa *nssa_lsa;
#endif /* HAVE_NSSA */

  /* Pointer to LSA. */
  struct ospf_lsa *lsa;
};

struct ospf_route_map_arg
{
  /* OSPF process. */
  struct ospf *top;

  /* Redistribute arguments. */
  struct ospf_redist_arg arg;

  /* Metric value from NSM. */
  u_int32_t metric;

  /* Nexthop received from NSM.  */
  struct pal_in4_addr nsm_nexthop;
};

struct ospf_summary
{
  /* OSPF instance. */
  struct ospf *top;

  /* Summary address prefix. */
  struct ls_prefix *lp;

  /* Flag for advertise or not. */
  u_char flags;
#define OSPF_SUMMARY_ADDRESS_ADVERTISE          (1 << 0)

  /* Mathced count. */
  u_int16_t matched;

  /* External route tag. */
  u_int32_t tag;

  /* Forwarding address. */
  /* struct pal_in4_addr nexthop; */

  /* Update time. */
  struct thread *t_update;
};

void ospf_asbr_status_update (struct ospf *);
int ospf_metric_type (struct ospf *, u_char);
u_int32_t ospf_metric_value (struct ospf *, u_char, u_char, int);

struct pal_in4_addr *ospf_asbr_nexthop_check_internal (struct ospf *,
                                                       struct pal_in4_addr *);
int ospf_redist_map_arg_set (struct ospf *, struct ospf_redist_map *,
                             struct ospf_redist_arg *);
void ospf_redist_map_nexthop_update_by_if_up (struct ospf *,
                                              struct ospf_interface *);
void ospf_redist_map_nexthop_update_by_if_down (struct ospf *,
                                                struct ospf_interface *);
struct ospf_summary *ospf_summary_new ();
struct ospf_summary *ospf_summary_lookup (struct ls_table *,
                                          struct ls_prefix *);
struct ospf_summary *ospf_summary_lookup_by_addr (struct ls_table *,
                                                  struct pal_in4_addr, u_char);
struct ospf_summary *ospf_summary_match (struct ls_table *,
                                         struct ls_prefix *);
void ospf_summary_address_add (struct ospf *, struct ospf_summary *,
                               struct ls_prefix *);
void ospf_summary_address_delete (struct ospf *, struct ospf_summary *);
int ospf_summary_address_timer (struct thread *);

#endif /* _PACOS_OSPF_ASBR_H */
