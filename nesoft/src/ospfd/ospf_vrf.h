/* Copyright (C) 2002-2003  All Rights Reserved. */

#ifndef _PACOS_OSPF_VRF_H
#define _PACOS_OSPF_VRF_H

/* OSPF VRF. */
struct ospf_vrf
{
  /* OSPF instance with vrf enabled */
  u_char flags;
#define OSPF_VRF_ENABLED          (1<<0)

  /* Pointer to APN VRF. */
  struct apn_vrf *iv;

  /* OSPF instance list. */
  struct list *ospf;

  /* Redistribute info. */
  struct ls_table *redist_table;

  /* Redistribute count. */
  u_int32_t redist_count[APN_ROUTE_MAX];

  /* igp-shortcut-lsp info. */
  struct route_table *igp_lsp_table;
};

struct ospf_vrf_domain_id
{
  u_int16_t type;
  u_int8_t value[6];
};

/* Prototypes. */
struct ospf_vrf *ospf_vrf_get (struct ospf_master *, char *);
int ospf_vrf_delete (struct apn_vrf *);
struct ospf_vrf *ospf_vrf_lookup_by_id (struct ospf_master *, vrf_id_t);
int ospf_vrf_update_router_id (struct apn_vrf *iv);
struct ospf_vrf *ospf_vrf_lookup_by_name (struct ospf_master *, char *);

void ospf_vrf_init (struct lib_globals *);

#endif /* _PACOS_OSPF_VRF_H */
