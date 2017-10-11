/* Copyright (C) 2002-2003  All Rights Reserved. */

#ifndef _PACOS_LS_TABLE_H
#define _PACOS_LS_TABLE_H

/* Routing table top structure. */
struct ls_table
{
  /* Description for this table. */
  char *desc;

  /* Top node of route table. */
  struct ls_node *top;

  /* Prefix size of depth. */
  u_char prefixsize;

  /* Number of info slot. */
  u_char slotsize;

  /* Counter for vinfo. */
  u_int32_t count[1];
};

#define LS_TABLE_SIZE(S)                                                      \
  (sizeof (struct ls_table) + ((S) - 1) * sizeof (u_int32_t))

#define LS_NODE_SIZE(T)                                                       \
    (sizeof (struct ls_node) + ((T)->slotsize - 1) * sizeof (void *))

/* Each routing entry. */
struct ls_node
{
  /* DO NOT MOVE the first 2 pointers are used by the memory manager as well */
  struct ls_node *link[2];
#define l_left   link[0]
#define l_right  link[1]

  /* Pointer to variable length prefix of this radix. */
  struct ls_prefix *p;

  /* Tree link. */
  struct ls_table *table;
  struct ls_node *parent;

  /* Lock of this radix. */
  unsigned int lock;

  /* Routing information. */
  void *vinfo[1];
#define vinfo0   vinfo[0]
#define vinfo1   vinfo[1]
#define vinfo2   vinfo[2]

  /* Convenient macro. */
#define ri_cur   vinfo[0]
#define ri_new   vinfo[1]
#define ri_lsas  vinfo[2]
};

#define RN_INFO_SET(N,I,D)     ls_node_info_set ((N), (I), (D))
#define RN_INFO_UNSET(N,I)     ls_node_info_unset ((N), (I))
#define RN_INFO(N,I)                                                          \
    (pal_assert ((N)->table->slotsize > (I)), (N)->vinfo[(I)])

/* Route node info index. */
#define RNI_DEFAULT        0
#define RNI_CURRENT        0
#define RNI_NEW            1
#define RNI_LSAS           2

/* Prototypes. */
struct ls_table *ls_table_init (u_char, u_char);
void ls_table_finish (struct ls_table *);
void ls_unlock_node (struct ls_node *);
void ls_node_delete (struct ls_node *);
struct ls_node *ls_table_top (struct ls_table *);
struct ls_node *ls_route_next (struct ls_node *);
struct ls_node *ls_route_next_until (struct ls_node *, struct ls_node *);
struct ls_node *ls_node_get (struct ls_table *, struct ls_prefix *);
struct ls_node *ls_node_lookup (struct ls_table *, struct ls_prefix *);
struct ls_node *ls_node_lookup_first (struct ls_table *);
struct ls_node *ls_lock_node (struct ls_node *);
struct ls_node *ls_node_match (struct ls_table *, struct ls_prefix *);
struct ls_node *ls_node_match_ipv4 (struct ls_table *, struct pal_in4_addr *);
struct ls_node *ls_node_info_lookup (struct ls_table *, struct ls_prefix *,
                                     int);
void *ls_node_info_set (struct ls_node *, int, void *);
void *ls_node_info_unset (struct ls_node *, int);

#endif /* _PACOS_LS_TABLE_H */
