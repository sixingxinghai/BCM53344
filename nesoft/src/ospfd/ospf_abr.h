/* Copyright (C) 2001-2003  All Rights Reserved. */

#ifndef _PACOS_OSPF_ABR_H
#define _PACOS_OSPF_ABR_H

struct ospf_area_range
{
  /* OSPF area. */
  struct ospf_area *area;

  /* Area range prefix. */
  struct ls_prefix *lp;

  /* Substitute prefix. */
  struct ls_prefix *subst;

  /* Flag for advertise or not. */
  u_char flags;
#define OSPF_AREA_RANGE_ADVERTISE   (1 << 0)
#define OSPF_AREA_RANGE_SUBSTITUTE  (1 << 1)

  /* Row Status. */
  u_char status;

  /* Number of mathced prefix. */
  u_int32_t matched;

  /* Cost for summary route. */
  u_int32_t cost;

  /* Update timer. */
  struct thread *t_update;
};

/* Macro. */
#define IS_AREA_RANGE_ACTIVE(R)         ((R) != NULL && (R)->matched)

/* Prototypes. */
void ospf_abr_nssa_translator_state_update (struct ospf_area *);
void ospf_abr_status_update (struct ospf *);

void ospf_abr_ia_prefix_update_all (struct ospf_area *, struct ls_prefix *,
                                    struct ospf_path *);
void ospf_abr_ia_prefix_delete_all (struct ospf_area *, struct ls_prefix *,
                                    struct ospf_path *);
void ospf_abr_ia_router_update_all (struct ospf_area *, struct ls_prefix *,
                                    struct ospf_path *);
void ospf_abr_ia_router_delete (struct ospf_area *, struct ls_prefix *);
void ospf_abr_ia_router_delete_all (struct ospf_area *, struct ls_prefix *,
                                    struct ospf_path *);

void ospf_abr_ia_update_default_to_area (struct ospf_area *);
void ospf_abr_ia_update_all_to_area (struct ospf_area *);
void ospf_abr_ia_update_area_to_all (struct ospf_area *);
void ospf_abr_ia_update_from_backbone_to_area (struct ospf_area *);

struct ospf_area_range *ospf_area_range_new (struct ospf_area *);
struct ospf_area_range *ospf_area_range_lookup (struct ospf_area *,
                                                struct ls_prefix *);
struct ospf_area_range *ospf_area_range_match (struct ospf_area *,
                                               struct ls_prefix *);
int ospf_area_range_active_match (struct ospf *, struct ls_prefix *);
void ospf_area_range_check_update (struct ospf_area *,
                                   struct ls_prefix *, struct ospf_path *);
void ospf_area_range_add (struct ospf_area *, struct ospf_area_range *,
                          struct ls_prefix *);
void ospf_area_range_delete (struct ospf_area *, struct ospf_area_range *);
void ospf_area_range_delete_all (struct ospf_area *);
int ospf_area_range_timer (struct thread *);

#endif /* _PACOS_OSPF_ABR_H */
