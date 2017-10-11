/* Copyright (C) 2002-2003  All Rights Reserved. */

#ifndef _PACOS_LS_PREFIX_H
#define _PACOS_LS_PREFIX_H

#include "pal_string.h"

#define LS_INDEX_NONE    '\0'
#define LS_INDEX_INT8    'c'
#define LS_INDEX_INT16   's'
#define LS_INDEX_INT32   'i'
#define LS_INDEX_INADDR  'a'
#define LS_INDEX_OSIADDR 'o'
#define LS_INDEX_LSPID   'l'
#define LS_INDEX_INADDR6 'p'

#define LS_IPV4_ROUTE_TABLE_DEPTH  4
#define LS_IPV6_ROUTE_TABLE_DEPTH  16

/* Link state prefix structure. */
#define LS_PREFIX_MIN_SIZE      4
struct ls_prefix
{
  u_char family;
  u_char prefixlen;
  u_char prefixsize;
  u_char pad;
  u_char prefix[LS_PREFIX_MIN_SIZE];
};

#define LS_PREFIX_SIZE(P)                                                     \
    (sizeof (struct ls_prefix) + (P)->prefixsize - LS_PREFIX_MIN_SIZE)

/* Some useful structure. */
struct ls_prefix6
{
  u_char family;
  u_char prefixlen;
  u_char prefixsize;
  u_char pad1;
  u_char prefix[6];
};

struct ls_prefix8
{
  u_char family;
  u_char prefixlen;
  u_char prefixsize;
  u_char pad;
  u_char prefix[8];
};

struct ls_prefix12
{
  u_char family;
  u_char prefixlen;
  u_char prefixsize;
  u_char pad;
  u_char prefix[12];
};

struct ls_prefix16
{
  u_char family;
  u_char prefixlen;
  u_char prefixsize;
  u_char pad;
  u_char prefix[16];
};

struct ls_prefix_nh
{
  u_char family;
  u_char prefixlen;
  u_char prefixsize;
  u_char pad;
  struct pal_in4_addr if_id;
  struct pal_in4_addr nbr_id;
};

/* Prototypes. */
struct ls_prefix *ls_prefix_new (size_t);
void ls_prefix_free (struct ls_prefix *);
void ls_prefix_copy (struct ls_prefix *, struct ls_prefix *);
int ls_prefix_match (struct ls_prefix *, struct ls_prefix *);
void apply_mask_ls (struct ls_prefix *);
void ls_prefix_ipv4_set (struct ls_prefix *, int, struct pal_in4_addr);
#ifdef HAVE_IPV6
void ls_prefix_ipv6_set (struct ls_prefix16 *, int, struct pal_in6_addr);
#endif /* HAVE_IPV6 */
void ls_prefix_set_args (struct ls_prefix *, char *, ...);
void ls_prefix_set_va_list (struct ls_prefix *, char *, va_list);

#endif /* _PACOS_LS_PREFIX_H */
