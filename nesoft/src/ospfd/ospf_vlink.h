/* Copyright (C) 2001-2003  All Rights Reserved. */

#ifndef _PACOS_OSPF_VLINK_H
#define _PACOS_OSPF_VLINK_H

#define OSPF_VLINK_NAME_PREFIX  "VLINK"
#define OSPF_VLINK_MTU           1500

/* OSPF Virtual-Link. */
struct ospf_vlink
{
  /* Description. */
  char *name;

  /* Virtual-Link index. */
  u_int32_t index;

  /* Transit area for this virtual-link. */
  struct pal_in4_addr area_id;

  /* Router-ID of virtual-link peer. */
  struct pal_in4_addr peer_id;

  /* Interface MTU. */
  int mtu;

  /* Address used to reach the peer. */
#define peer_addr       oi->destination->u.prefix4

  /* Area ID format. */
  u_char format;

  /* Flags. */
  u_char flags;
#define OSPF_VLINK_UP        (1 << 0)

  /* Row Status. */
  u_char status;

  /* Interface data structure for the virtual-link. */
  struct ospf_interface *oi;

  /* The interface to go out. */
  struct ospf_interface *out_oi;

  /* Vertex of the remote peer.  */
  struct ospf_vertex *vertex;
};

/* Prototypes. */
int ospf_vlink_entry_delete (struct ospf *, struct ospf_vlink *);
struct ospf_vlink *ospf_vlink_entry_lookup (struct ospf *, struct pal_in4_addr,
                                            struct pal_in4_addr);
struct ospf_vlink *ospf_vlink_entry_lookup_functional (struct ospf *,
                                                       struct pal_in4_addr);
struct ospf_vlink *ospf_vlink_lookup_by_name (struct ospf_master *, char *);
struct ospf_vlink *ospf_vlink_get (struct ospf *, struct pal_in4_addr,
                                   struct pal_in4_addr);
void ospf_vlink_delete (struct ospf *, struct ospf_vlink *);
void ospf_vlink_local_address_update_by_interface (struct ospf_interface *);
void ospf_vlink_status_update (struct ospf_area *);
int ospf_vlink_if_up (struct ospf_vlink *);

#endif /* _PACOS_OSPF_VLINK_H */
