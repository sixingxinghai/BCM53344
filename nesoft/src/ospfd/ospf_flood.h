/* Copyright (C) 2001-2003  All Rights Reserved. */

#ifndef _PACOS_OSPF_FLOOD_H
#define _PACOS_OSPF_FLOOD_H

#define OSPF_LS_REQUEST_TABLE_DEPTH   12

/* Link State Request. */
struct ospf_ls_request
{
  struct lsa_header data;
};

struct ls_prefix_lsr
{
  u_char family;
  u_char prefixlen;
  u_char prefixsize;
  u_char pad2;

  /* LS type. */
  u_int32_t type;

  /* Link State ID. */
  struct pal_in4_addr id;

  /* Advertising Router. */
  struct pal_in4_addr adv_router;
};

/* Prototypes. */
unsigned long ospf_ls_request_count (struct ospf_neighbor *);
int ospf_ls_request_isempty (struct ospf_neighbor *);
struct ospf_ls_request *ospf_ls_request_new (struct lsa_header *);
void ospf_ls_request_add (struct ospf_neighbor *, struct ospf_ls_request *);
void ospf_ls_request_delete (struct ospf_neighbor *, struct ospf_ls_request *);
void ospf_ls_request_delete_all (struct ospf_neighbor *);
struct ospf_ls_request *ospf_ls_request_lookup (struct ospf_neighbor *,
                                                struct lsa_header *);

struct ospf_lsa *ospf_ls_retransmit_lookup (struct ospf_neighbor *, struct ospf_lsa *);
struct ospf_lsa *
ospf_ls_retransmit_lookup_by_id (struct ospf_neighbor *, u_char,
                               struct pal_in4_addr, struct pal_in4_addr);
  
struct ospf_lsdb *ospf_ls_retransmit_list_find (struct ospf_neighbor *);

unsigned long ospf_ls_retransmit_count (struct ospf_neighbor *);
int ospf_ls_retransmit_isempty (struct ospf_neighbor *);
void ospf_ls_retransmit_add (struct ospf_neighbor *, struct ospf_lsa *);
void ospf_ls_retransmit_delete (struct ospf_neighbor *, struct ospf_lsa *);
void ospf_ls_retransmit_clear (struct ospf_neighbor *);
void ospf_ls_retransmit_delete_nbr_by_scope (struct ospf_lsa *, int);

int ospf_flood_through (struct ospf_neighbor *, struct ospf_lsa *);
int ospf_flood (struct ospf *, struct ospf_neighbor *,
                struct ospf_lsa *, struct ospf_lsa *);
void ospf_flood_lsa_flush (struct ospf *, struct ospf_lsa *);

#endif /* _PACOS_OSPF_FLOOD_H */
