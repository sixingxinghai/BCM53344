/* Copyright (C) 2002-2003  All Rights Reserved. */

#include "pal.h"

#include "ls_prefix.h"

/* Utility mask array. */
static u_char maskbit[] = 
{
  0x00, 0x80, 0xc0, 0xe0, 0xf0, 0xf8, 0xfc, 0xfe, 0xff
};

/* Number of bits in prefix type. */
#ifndef PNBBY
#define PNBBY 8
#endif /* PNBBY */

struct ls_prefix *
ls_prefix_new (size_t size)
{
  struct ls_prefix *p;

  p = (struct ls_prefix *) XCALLOC (MTYPE_LS_PREFIX,
                                    sizeof (struct ls_prefix) + size -
                                    LS_PREFIX_MIN_SIZE);
  p->prefixsize = size;

  return p;
}

void
ls_prefix_free (struct ls_prefix *p)
{
  XFREE (MTYPE_LS_PREFIX, p);
}

void
ls_prefix_copy (struct ls_prefix *dest, struct ls_prefix *src)
{
  pal_mem_cpy (dest, src, LS_PREFIX_SIZE (src));
}

/* If n includes p prefix then return else return 0. */
int
ls_prefix_match (struct ls_prefix *n, struct ls_prefix *p)
{
  int offset;
  int shift;

  /* Set both prefix's head pointer. */
  u_char *np = n->prefix;
  u_char *pp = p->prefix;

  pal_assert (n->prefixsize == p->prefixsize);

  /* If n's prefix is longer than p's one return 0. */
  if (n->prefixlen > p->prefixlen)
    return 0;

  offset = n->prefixlen / PNBBY;
  shift =  n->prefixlen % PNBBY;

  if (shift)
    if (maskbit[shift] & (np[offset] ^ pp[offset]))
      return 0;
  
  while (offset--)
    if (np[offset] != pp[offset])
      return 0;

  return 1;
}

void
apply_mask_ls (struct ls_prefix *p)
{
  u_char *pnt;
  int index;
  int offset;

  index = p->prefixlen / 8;

  if (index < p->prefixsize)
    {
      pnt = (u_char *) &p->prefix;
      offset = p->prefixlen % 8;

      pnt[index] &= maskbit[offset];
      index++;

      while (index < p->prefixsize)
        pnt[index++] = 0;
    }
}

void
ls_prefix_ipv4_set (struct ls_prefix *p, int prefixlen,
                    struct pal_in4_addr addr)
{
  p->family = AF_INET;
  p->prefixlen = prefixlen;
  p->prefixsize = LS_IPV4_ROUTE_TABLE_DEPTH;
  pal_mem_cpy (p->prefix, &addr, sizeof (struct pal_in4_addr));
  apply_mask_ls (p);
}

#ifdef HAVE_IPV6
void
ls_prefix_ipv6_set (struct ls_prefix16 *p, int prefixlen,
                    struct pal_in6_addr addr)
{
  p->family = AF_INET6;
  p->prefixlen = prefixlen;
  p->prefixsize = LS_IPV6_ROUTE_TABLE_DEPTH;
  pal_mem_cpy (p->prefix, &addr, sizeof (struct pal_in6_addr));
  apply_mask_ipv6 ((struct prefix_ipv6 *)p);
}
#endif /* HAVE_IPV6 */

/* This function just pass the arguments to ls_prefix_set_va_list. */
void
ls_prefix_set_args (struct ls_prefix *p, char *types, ...)
{
  va_list args;

  va_start (args, types);
  ls_prefix_set_va_list (p, types, args);
  va_end (args);
}

/* Suppose all arguments are passed as a pointer. */
void
ls_prefix_set_va_list (struct ls_prefix *p, char *types, va_list args)
{
  char type;
  unsigned int index = 0;

  while ((type = *types++) != LS_INDEX_NONE)
    {
      unsigned int *valp;
      unsigned int val = 0;
      unsigned int addrval = 0;
      int i, size = 0;

      valp = (unsigned int *)va_arg (args, unsigned int *);
      switch (type)
        {
        case LS_INDEX_INT8:
          val = (*valp & 0xFF);
          size = 1;
          break;
        case LS_INDEX_INT16:
          val = (*valp & 0xFFFF);
          size = 2;
          break;
        case LS_INDEX_INT32:
          val = (*valp & 0xFFFFFFFF);
          size = 4;
          break;
        case LS_INDEX_INADDR:
          addrval = (*valp & 0xFFFFFFFF);
          val = pal_ntoh32 (addrval);
          size = 4;
          break;
        }

      for (i = 1; i <= size; i++)
        p->prefix[index++] = (u_char)((val >> ((size - i) * 8)) & 0xff);
    }
}

