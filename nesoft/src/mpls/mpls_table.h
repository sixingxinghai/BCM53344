/* Copyright (C) 2001-2003  All Rights Reserved.  */

/*********************************************************************
 * The following routines are being stolen from lib/table.c,
 * since we dont want to bind the whole lib to the mpls_module,
 * cos there'll be too many issues with free and assert and all that.
 *********************************************************************/

#ifndef _PACOS_MPLS_TABLE_H
#define _PACOS_MPLS_TABLE_H

struct mpls_table;

/* Each routing entry. */
struct mpls_table_node
{
  /* Actual prefix of this radix. */
  struct mpls_prefix p;
  
  /* Tree link. */
  struct mpls_table *table;
  struct mpls_table_node *parent;
  struct mpls_table_node *link[2];
#define l_left   link[0]
#define l_right  link[1]

  /* Lock of this radix */
  unsigned int lock;

  /* Each node contains this. */
  void *info;
  
  /* Aggregation. */
  void *aggregate;
};

/* Routing table top structure. */
struct mpls_table
{
  struct mpls_table_node *top;

  /*
   * The following are only being used by VRF/FTN tables.
   */

  /* Identifier for this mpls table */
  int ident;

  /* Refcount of entities using this mpls table */
  int refcount;

  /* Linklist specific variables */
  struct mpls_table *next;
  struct mpls_table *prev;
};

/* Prototypes. */
struct mpls_table *mpls_table_init (int ident);
void mpls_table_finish (struct mpls_table *);
void mpls_table_unlock_node (struct mpls_table_node *node);
void mpls_table_node_delete (struct mpls_table_node *node);
struct mpls_table_node *mpls_table_top (struct mpls_table *);
struct mpls_table_node *mpls_table_next (struct mpls_table_node *);
struct mpls_table_node *mpls_table_node_get (struct mpls_table *,
                                             struct mpls_prefix *);
struct mpls_table_node *mpls_table_node_lookup (struct mpls_table *,
                                                struct mpls_prefix *);
struct mpls_table_node *mpls_table_node_match_ipv4 (struct mpls_table *,
                                                    uint32 daddr);
struct mpls_table_node *mpls_table_lock_node (struct mpls_table_node *node);
const int mpls_prefix_match(struct mpls_prefix *p, struct mpls_prefix *n);

#define mpls_prefix_copy(dest,src) \
do { \
  (dest)->family = (src)->family; \
  (dest)->prefixlen = (src)->prefixlen; \
  if ((src)->family == AF_INET) \
    (dest)->u.prefix4 = (src)->u.prefix4; \
  else \
  (dest)->u.prefix4.s_addr = 0; \
} while (0)

#ifndef PNBBY
#define PNBBY 8
#endif /* PNBBY */

#define net32_prefix(v, l) (htonl (((v) << (32 - (l)))))

#endif /* _PACOS_MPLS_TABLE_H */
