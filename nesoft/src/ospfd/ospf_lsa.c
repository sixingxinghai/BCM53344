/* Copyright (C) 2001-2003  All Rights Reserved. */

#include "pal.h"
#include "lib.h"
#include "if.h"
#include "prefix.h"
#include "thread.h"
#include "linklist.h"
#include "hash.h"
#include "snprintf.h"
#include "log.h"
#include "nsm_client.h"

#include "ospfd.h"
#include "ospf_interface.h"
#include "ospf_vlink.h"
#include "ospf_packet.h"
#include "ospf_neighbor.h"
#include "ospf_ifsm.h"
#include "ospf_lsa.h"
#include "ospf_lsdb.h"
#include "ospf_flood.h"
#include "ospf_abr.h"
#include "ospf_asbr.h"
#include "ospf_spf.h"
#include "ospf_ia.h"
#include "ospf_ase.h"
#include "ospf_route.h"
#include "ospf_nsm.h"
#include "ospf_api.h"
#include "ospf_debug.h"
#include "ospf_vrf.h"
#include "ospf_te.h"
#ifdef HAVE_RESTART
#include "ospf_restart.h"
#endif /* HAVE_RESTART */
#ifdef HAVE_SNMP
#include "ospf_snmp.h"
#endif /* HAVE_SNMP */


/* Macros. */
#define OSPF_LSA_INSTALL_BY_TYPE(O,F,OL,NL)                                   \
  do {                                                                        \
    if (IS_LSA_TYPE_KNOWN ((NL)->data->type))                                 \
      (NL) = ospf_lsa_func[((NL)->data->type)].install ((O), (F), (OL), (NL));\
  } while (0)

#define OSPF_LSA_ORIGINATE_BY_TYPE(T,O,P,NL)                                  \
  do {                                                                        \
    if (IS_LSA_TYPE_KNOWN ((T)))                                              \
      (NL) = ospf_lsa_func[(T)].originate ((O), (P));                         \
  } while (0)

#define OSPF_LSA_REFRESH_BY_TYPE(T,O,OL,NL)                                   \
  do {                                                                        \
    if (IS_LSA_TYPE_KNOWN ((T)))                                              \
      (NL) = ospf_lsa_func[(T)].refresh ((O), (OL));                          \
  } while (0)

/* Prototypes. */
void ospf_lsa_refresh_queue_set (struct ospf_lsa *);


u_char ospf_flood_scope[OSPF_MAX_LSA] =
{
  LSA_FLOOD_SCOPE_UNKNOWN, /* OSPF_UNKNOWN_LSA. */
  LSA_FLOOD_SCOPE_AREA,    /* OSPF_ROUTER_LSA. */
  LSA_FLOOD_SCOPE_AREA,    /* OSPF_NETWORK_LSA. */
  LSA_FLOOD_SCOPE_AREA,    /* OSPF_SUMMARY_LSA. */
  LSA_FLOOD_SCOPE_AREA,    /* OSPF_SUMMARY_LSA_ASBR. */
  LSA_FLOOD_SCOPE_AS,      /* OSPF_AS_EXTERNAL_LSA. */
#ifdef HAVE_NSSA
  LSA_FLOOD_SCOPE_UNKNOWN, /* OSPF_GROUP_MEMBER_LSA. */
  LSA_FLOOD_SCOPE_AREA,    /* OSPF_AS_NSSA_LSA. */
#endif /* HAVE_NSSA */
#ifdef HAVE_OPAQUE_LSA
#ifndef HAVE_NSSA
  LSA_FLOOD_SCOPE_UNKNOWN, /* OSPF_GROUP_MEMBER_LSA. */
  LSA_FLOOD_SCOPE_UNKNOWN, /* OSPF_AS_NSSA_LSA. */
#endif /* HAVE_NSSA */
  LSA_FLOOD_SCOPE_UNKNOWN, /* Type-8 LSA. */
  LSA_FLOOD_SCOPE_LINK,    /* OSPF_LINK_OPAQUE_LSA. */
  LSA_FLOOD_SCOPE_AREA,    /* OSPF_AREA_OPAQUE_LSA. */
  LSA_FLOOD_SCOPE_AS,      /* OSPF_AS_OPAQUE_LSA. */
#endif /* HAVE_OPAQUE_LSA */
};


/* Create LSA data.  */
struct lsa_header *
ospf_lsa_data_new (size_t size)
{
  struct lsa_header *new;

  new = (struct lsa_header *) XMALLOC (MTYPE_OSPF_LSA_DATA, size);
  pal_assert (new != NULL);

  return new;
}

/* Free LSA data. */
void
ospf_lsa_data_free (struct lsa_header *lsah)
{
  XFREE (MTYPE_OSPF_LSA_DATA, lsah);
}

/* Create OSPF LSA. */
struct ospf_lsa *
ospf_lsa_new (void)
{
  struct ospf_lsa *new;

  new = XMALLOC (MTYPE_OSPF_LSA, sizeof (struct ospf_lsa));
  pal_mem_set (new, 0, sizeof (struct ospf_lsa));
  new->lock = 1;
  new->status = notEXPLORED;

  return new;
}

/* Free OSPF LSA.  */
void
ospf_lsa_free (struct ospf_lsa *lsa)
{
  /* Delete LSA data. */
  if (lsa->data != NULL)
    ospf_lsa_data_free (lsa->data);

  XFREE (MTYPE_OSPF_LSA, lsa);
}


/* Unused LSA handling.  */
int
ospf_lsa_delete_unuse_tail (struct ospf *top)
{
  struct ospf_lsa *lsa;

  lsa = top->lsa_unuse_tail;
  if (lsa == NULL)
    return 0;

  if (lsa->prev != NULL)
    top->lsa_unuse_tail = lsa->prev;
  else
    top->lsa_unuse_tail = NULL;

  if (top->lsa_unuse_tail != NULL)
    top->lsa_unuse_tail->next = NULL;
  else
    top->lsa_unuse_head = NULL;

  lsa->prev = NULL;
  lsa->next = NULL;

  /* Free the LSA.  */
  ospf_lsa_free (lsa);

  top->lsa_unuse_count--;

  return 1;
}

struct ospf_lsa *
ospf_lsa_add_unuse (struct ospf *top, struct ospf_lsa *lsa)
{
  /* Unused LSA is off.  */
  if (top->lsa_unuse_max == 0)
    return lsa;

  /* Reset flags and pointers.  */
  lsa->flags = 0;
  lsa->lsdb = NULL;
  lsa->param = NULL;
  lsa->lock = 1;

  lsa->prev = NULL;
  lsa->next = top->lsa_unuse_head;

  if (top->lsa_unuse_head != NULL)
    top->lsa_unuse_head->prev = lsa;
  else
    top->lsa_unuse_tail = lsa;
  top->lsa_unuse_head = lsa;

  top->lsa_unuse_count++;

  /* Delete the last LSA when it exceeds maximum number.  */
  if (top->lsa_unuse_count > top->lsa_unuse_max)
    ospf_lsa_delete_unuse_tail (top);

  return NULL;
}

struct ospf_lsa *
ospf_lsa_delete_unuse (struct ospf *top, struct ospf_lsa *lsa)
{
  if (lsa->prev != NULL)
    lsa->prev->next = lsa->next;
  else
    top->lsa_unuse_head = lsa->next;

  if (lsa->next != NULL)
    lsa->next->prev = lsa->prev;
  else
    top->lsa_unuse_tail = lsa->prev;

  lsa->prev = NULL;
  lsa->next = NULL;

  top->lsa_unuse_count--;

  return lsa;
}

struct ospf_lsa *
ospf_lsa_lookup_unuse (struct ospf *top, size_t size)
{
  struct ospf_lsa *lsa;

  for (lsa = top->lsa_unuse_head; lsa; lsa = lsa->next)
    if (lsa->alloced >= size)
      return ospf_lsa_delete_unuse (top, lsa);

  return NULL;
}

void
ospf_lsa_unuse_clear (struct ospf *top)
{
  while (ospf_lsa_delete_unuse_tail (top))
    ;
}

void
ospf_lsa_unuse_clear_half (struct ospf *top)
{
  u_int32_t half;

  /* Free half of them.  */
  half = top->lsa_unuse_count / 2;

  while (ospf_lsa_delete_unuse_tail (top))
    if (top->lsa_unuse_count <= half)
      return;
}

void
ospf_lsa_unuse_max_update (struct ospf *top, u_int32_t max)
{
  if (top->lsa_unuse_max != max)
    {
      while (top->lsa_unuse_count > max)
        if (!ospf_lsa_delete_unuse_tail (top))
          break;

      /* Update the maximum number.  */
      top->lsa_unuse_max = max;
    }
}


/* Lock LSA. */
struct ospf_lsa *
ospf_lsa_lock (struct ospf_lsa *lsa)
{
  if (lsa != NULL)
    lsa->lock++;
  return lsa;
}

/* Unlock LSA. */
void
ospf_lsa_unlock (struct ospf_lsa *lsa)
{
  struct ospf *top;

  pal_assert (lsa->lock > 0);
  lsa->lock--;

  if (lsa->lock == 0)
    {
      if (lsa->lsdb)
        if ((top = ospf_proc_lookup_by_lsdb (lsa->lsdb)))
          if (IS_PROC_ACTIVE (top))
            lsa = ospf_lsa_add_unuse (top, lsa);

      if (lsa != NULL)
        ospf_lsa_free (lsa);
    }
}

struct ospf_lsa *
ospf_lsa_get (struct ospf *top, void *data, size_t size)
{
  struct ospf_lsa *lsa;

  lsa = ospf_lsa_lookup_unuse (top, size);
  if (lsa == NULL)
    {
      lsa = ospf_lsa_new ();
      lsa->data = ospf_lsa_data_new (size);
      lsa->alloced = size;
    }
  pal_mem_cpy (lsa->data, data, size);
  pal_time_tzcurrent (&lsa->tv_update, NULL);

  return lsa;
}

/* Do not set current time in the function.  */
struct ospf_lsa *
ospf_lsa_get_without_time (struct ospf *top, void *data, size_t size)
{
  struct ospf_lsa *lsa;

  lsa = ospf_lsa_lookup_unuse (top, size);
  if (lsa == NULL)
    {
      lsa = ospf_lsa_new ();
      lsa->data = ospf_lsa_data_new (size);
      lsa->alloced = size;
    }
  pal_mem_cpy (lsa->data, data, size);

  return lsa;
}

/* Check discard flag. */
void
ospf_lsa_discard (struct ospf_lsa *lsa)
{
  if (!CHECK_FLAG (lsa->flags, OSPF_LSA_DISCARD))
    {
      SET_FLAG (lsa->flags, OSPF_LSA_DISCARD);
      ospf_lsa_refresh_event_unset (lsa);
      ospf_lsa_unlock (lsa);
    }
}


int
ospf_lsa_age_get (struct ospf_lsa *lsa)
{
  struct pal_timeval now;
  int age;

  pal_time_tzcurrent (&now, NULL);
  age =
    pal_ntoh16 (lsa->data->ls_age) + TV_FLOOR (TV_SUB (now, lsa->tv_update));

  if (age >= OSPF_LSA_MAXAGE)
    return OSPF_LSA_MAXAGE;

  return age;
}

/* return +n, l1 is more recent.
   return -n, l2 is more recent.
   return 0, l1 and l2 is identical. */
int
ospf_lsa_more_recent (struct ospf_lsa *l1, struct ospf_lsa *l2)
{
  int r;
  int x, y;

  if (l1 == NULL && l2 == NULL)
    return 0;
  if (l1 == NULL)
    return -1;
  if (l2 == NULL)
    return 1;

  /* compare LS sequence number. */
  x = (int) pal_ntoh32 (l1->data->ls_seqnum);
  y = (int) pal_ntoh32 (l2->data->ls_seqnum);
  if (x > y)
    return 1;
  if (x < y)
    return -1;

  /* compare LS checksum. */
  r = pal_ntoh16 (l1->data->checksum) - pal_ntoh16 (l2->data->checksum);
  if (r)
    return r;

  /* compare LS age. */
  if (IS_LSA_MAXAGE (l1) && !IS_LSA_MAXAGE (l2))
    return 1;
  else if (!IS_LSA_MAXAGE (l1) && IS_LSA_MAXAGE (l2))
    return -1;

  /* compare LS age with MaxAgeDiff. */
  if (LS_AGE (l1) - LS_AGE (l2) > OSPF_LSA_MAXAGE_DIFF)
    return -1;
  else if (LS_AGE (l2) - LS_AGE (l1) > OSPF_LSA_MAXAGE_DIFF)
    return 1;

  /* LSAs are identical. */
  return 0;
}

int
ospf_lsa_header_more_recent (struct lsa_header *lsah, struct ospf_lsa *lsa)
{
  int r;
  int x, y;
  int lsr_age;

  if (lsah == NULL && lsa == NULL)
    return 0;
  if (lsah == NULL)
    return -1;
  if (lsa == NULL)
    return 1;

  /* compare LS sequence number. */
  x = (int) pal_ntoh32 (lsah->ls_seqnum);
  y = (int) pal_ntoh32 (lsa->data->ls_seqnum);
  if (x > y)
    return 1;
  if (x < y)
    return -1;

  /* compare LS checksum. */
  r = pal_ntoh16 (lsah->checksum) - pal_ntoh16 (lsa->data->checksum);
  if (r)
    return r;

  lsr_age = pal_ntoh16 (lsah->ls_age);
  /* compare LS age. */
  if (lsr_age >= OSPF_LSA_MAXAGE && !IS_LSA_MAXAGE (lsa))
    return 1;
  else if (lsr_age < OSPF_LSA_MAXAGE && IS_LSA_MAXAGE (lsa))
    return -1;

  /* compare LS age with MaxAgeDiff. */
  if (lsr_age - LS_AGE (lsa) > OSPF_LSA_MAXAGE_DIFF)
    return -1;
  else if (LS_AGE (lsa) - lsr_age > OSPF_LSA_MAXAGE_DIFF)
    return 1;

  /* LSAs are identical. */
  return 0;
}

/* If two LSAs are different, return 1, otherwise return 0. */
int
ospf_lsa_different (struct ospf_lsa *l1, struct ospf_lsa *l2)
{
  char *p1, *p2;

  /* Special handling for router-LSA. */
  if (l1->data->type == OSPF_ROUTER_LSA)
    if (CHECK_FLAG (l1->flags, OSPF_LSA_RECEIVED))
      if (!CHECK_FLAG (l2->flags, OSPF_LSA_RECEIVED))
        return 1;

  if (l1->data->options != l2->data->options)
    return 1;

  if (IS_LSA_MAXAGE (l1) && !IS_LSA_MAXAGE (l2))
    return 1;

  if (IS_LSA_MAXAGE (l2) && !IS_LSA_MAXAGE (l1))
    return 1;

  if (l1->data->length != l2->data->length)
    return 1;

  /* Juniper sends TE-LSA with zero-payload if misconfigured. */
  if (pal_ntoh16 (l1->data->length) <= OSPF_LSA_HEADER_SIZE)
    return 1;

  p1 = (char *) l1->data;
  p2 = (char *) l2->data;

  if (pal_mem_cmp (p1 + OSPF_LSA_HEADER_SIZE, p2 + OSPF_LSA_HEADER_SIZE,
                   pal_ntoh16 (l1->data->length) - OSPF_LSA_HEADER_SIZE) != 0)
    return 1;

  return 0;
}

/* Fletcher Checksum -- Refer to RFC1008. */
#define MODX                 4102
#define LSA_CHECKSUM_OFFSET    15

u_int16_t
ospf_lsa_checksum (struct lsa_header *lsa)
{
  u_char *sp, *ep, *p, *q;
  int c0 = 0, c1 = 0;
  int x, y;
  u_int16_t length;

  lsa->checksum = 0;
  length = pal_ntoh16 (lsa->length) - 2;
  sp = (u_char *) &lsa->options;

  for (ep = sp + length; sp < ep; sp = q)
    {
      q = sp + MODX;
      if (q > ep)
        q = ep;
      for (p = sp; p < q; p++)
        {
          c0 += *p;
          c1 += c0;
        }
      c0 %= 255;
      c1 %= 255;
    }

  /* r = (c1 << 8) + c0; */
  x = ((length - LSA_CHECKSUM_OFFSET) * c0 - c1) % 255;
  if (x <= 0)
    x += 255;
  y = 510 - c0 - x;
  if (y > 255)
    y -= 255;

  /* take care endian issue. */
  lsa->checksum = pal_hton16 ((x << 8) + y);

  return (lsa->checksum);
}

int
ospf_lsa_id_increment (struct pal_in4_addr *addr, u_char masklen)
{
  static const u_char wildmask[] =
    { 0x00, 0x01, 0x03, 0x07, 0x0f, 0x1f, 0x3f, 0x7f };
  u_char *pnt = (u_char *)addr;
  u_char wildmasklen;
  int offset;
  u_char max;
  u_char bit;
  u_char id;

  if (masklen == 0 || masklen >= IPV4_MAX_BITLEN)
    return 0;

  wildmasklen = IPV4_MAX_BITLEN - masklen;
  offset = wildmasklen / 8;
  bit = wildmasklen % 8;

  for (pnt += 3; offset >= 0; pnt--, offset--)
    {
      /* Get the host bit maximum value.  */
      if (offset == 0)
        max = wildmask[bit];
      else
        max = 0xff;

      /* Get the host part.  */
      id = *pnt & max;

      /* This byte was already reaches to maximum,
         try next byte.  */
      if (id == max)
        continue;

      /* Increment the host part.  */
      *pnt = (*pnt & ~max) | ++id;

      return 1;
    }

  return 0;
}

int
ospf_lsa_update_unique (struct ospf *top,
                        struct ospf_lsa *other, struct ls_prefix *lp)
{
  struct pal_in4_addr *mask;
  u_char masklen;

  switch (other->data->type)
    {
    case OSPF_SUMMARY_LSA:
      mask = OSPF_PNT_IN_ADDR_GET (other->data,
                                   OSPF_SUMMARY_LSA_NETMASK_OFFSET);
      break;
    case OSPF_AS_EXTERNAL_LSA:
    case OSPF_AS_NSSA_LSA:
      mask = OSPF_PNT_IN_ADDR_GET (other->data,
                                   OSPF_EXTERNAL_LSA_NETMASK_OFFSET);
      break;
    default:
      return 0;
    }

  masklen = ip_masklen (*mask);

  /* Preempt the other LSA to update the LS ID because it has
     less specific prefix length.  */
  if (masklen < lp->prefixlen)
    ospf_lsa_originate_event_add (top, other);

  /* otherwise, update the self LS ID.  This is the modified version
     of RFC2328 Apendix F.  */
  else if (masklen > lp->prefixlen)
    return -1;

  return 0;
}

struct ospf_lsa *
ospf_lsa_map_lsa_id_update (struct ospf *top,
                            struct ospf_lsa *old, struct pal_in4_addr id)
{
  struct ospf_lsa *lsa;

  if (old != NULL)
    ospf_lsa_unlock (old);

  lsa = ospf_lsa_lookup_unuse (top, OSPF_LSA_HEADER_SIZE);
  if (lsa == NULL)
    {
      lsa = ospf_lsa_new ();
      lsa->data = ospf_lsa_data_new (OSPF_LSA_HEADER_SIZE);
      lsa->alloced = OSPF_LSA_HEADER_SIZE;
    }

  /* Update the ID.  */
  lsa->data->id = id;

  return lsa;
}

u_int32_t
ospf_lsa_seqnum_increment (struct ospf_lsa *lsa)
{
  u_int32_t seqnum = pal_ntoh32 (lsa->data->ls_seqnum);
  return pal_hton32 (++seqnum);
}

u_char
ospf_lsa_options_get (struct ospf_area *area, u_char type, int propagate)
{
  u_char options = 0;

  if (LSA_FLOOD_SCOPE (type) == LSA_FLOOD_SCOPE_AS
      || IS_AREA_DEFAULT (area))
    options |= OSPF_OPTION_E;

  if (propagate)
    options |= OSPF_OPTION_NP;

  return options;
}

static void
ospf_lsa_header_set (struct stream *s, u_char options, u_char type,
                     struct pal_in4_addr id, struct pal_in4_addr router_id)
{
  stream_putw (s, 0);                                   /* LS age. */
  stream_putc (s, options);                             /* Options. */
  stream_putc (s, type);                                /* LS type. */
  stream_put_in_addr (s, &id);                          /* Link State ID. */
  stream_put_in_addr (s, &router_id);                   /* Adv Router. */
  stream_putl (s, OSPF_INITIAL_SEQUENCE_NUMBER);        /* LS seq number. */
  stream_putw (s, 0);                                   /* LS checksum. */
  stream_putw (s, 0);                                   /* Length. */
}


/* Router-LSA related functions. */

/* RFC2328 A.4.2 Router-LSAs, RFC3101 B Router-LSAs.  */
u_char
ospf_router_lsa_flags (struct ospf_area *area)
{
  u_char flags = 0;

  /* bit B

     When set, the router is an area border router (B is for border).  */
  if (CHECK_FLAG (area->top->flags, OSPF_ROUTER_ABR))
    SET_FLAG (flags, ROUTER_LSA_BIT_B);

  /* bit E

     When set, the router is an AS boundary router (E is for external).  */
  if (IS_OSPF_ASBR (area->top) && !IS_AREA_STUB (area))
    SET_FLAG (flags, ROUTER_LSA_BIT_E);

  /* bit V

     When set, the router is an endpoint of one or more fully
     adjacent virtual links having the described area as Transit area
     (V is for virtual link endpoint).  */
  if (area->full_virt_nbr_count)
    SET_FLAG (flags, ROUTER_LSA_BIT_V);

#ifdef HAVE_NSSA
  /* bit Nt

     When set, the router is an NSSA border router that is
     unconditionally translating Type-7 LSAs into Type-5 LSAs (Nt
     stands for NSSA translation).  Note that such routers have
     their NSSATranslatorRole area configuration parameter set to
     Always.  (See Appendix D and Section 3.1.)  */
  if (IS_AREA_NSSA (area)
      && ospf_nssa_translator_role_get (area) == OSPF_NSSA_TRANSLATE_ALWAYS 
      && area->translator_state != OSPF_NSSA_TRANSLATOR_DISABLED)
    SET_FLAG (flags, ROUTER_LSA_BIT_NT);
#endif /* HAVE_NSSA */

  /* Set Shortcut ABR behabiour flag. */
  if (area->top->abr_type == OSPF_ABR_TYPE_SHORTCUT)
    if (!IS_AREA_BACKBONE (area))
      {
        u_char mode;

        mode = ospf_area_shortcut_get (area);
        if ((mode == OSPF_SHORTCUT_DEFAULT && area->top->backbone == NULL)
            || mode == OSPF_SHORTCUT_ENABLE)
          SET_FLAG (flags, ROUTER_LSA_BIT_S);
      }

  return flags;
}

struct ospf_link_map *
ospf_link_map_new ()
{
  struct ospf_link_map *new;

  new = XMALLOC (MTYPE_OSPF_ROUTER_LSA_MAP, sizeof (struct ospf_link_map));
  pal_mem_set (new, 0, sizeof (struct ospf_link_map));

  return new;
}

void
ospf_link_map_free (struct ospf_link_map *map)
{
  XFREE (MTYPE_OSPF_ROUTER_LSA_MAP, map);
}

struct ospf_link_map *
ospf_area_link_map_get (struct ospf_area *area, int index)
{
  struct ospf_link_map *map;

  map = vector_lookup_index (area->link_vec, index);
  if (map == NULL)
    {
      map = ospf_link_map_new ();
      vector_set_index (area->link_vec, index, map);
    }

  return map;
}

struct ospf_link_map *
ospf_area_link_info_set (struct ospf_area *area, int index, u_char type,
                         u_int16_t metric, struct pal_in4_addr id,
                         struct pal_in4_addr data, u_char source, void *ref)
{
  struct ospf_link_map *map;

  map = ospf_area_link_map_get (area, index);
  map->type = type;
  map->metric = metric;
  map->id = id;
  map->data = data;
  map->source = source;
  map->nh.ref = ref;

  return map;
}

static int
ospf_router_lsa_ptop_set (struct ospf_interface *oi, int index)
{
  struct ospf_neighbor *nbr;
  struct pal_in4_addr ifindex;
  struct pal_in4_addr mask;
  struct prefix_ipv4 p;

  if ((nbr = ospf_nbr_lookup_ptop (oi)))
    if (IS_NBR_FULL (nbr))
      {
        /* For unnumbered point-to-point networks, the Link Data field
           should specify the interface's MIB-II ifIndex value. */
        if (IS_OSPF_IPV4_UNNUMBERED (oi))
          {
            ifindex.s_addr = pal_hton32 (oi->u.ifp->ifindex);
            ospf_area_link_info_set (oi->area, index,
                                     LSA_LINK_TYPE_POINTOPOINT,
                                     oi->output_cost, nbr->ident.router_id,
                                     ifindex, OSPF_LINK_MAP_NBR, nbr);
          }
        else
#ifdef HAVE_OSPF_MULTI_AREA
          /* RFC 5185 Section 2.7 */
          {
            if (!CHECK_FLAG (oi->flags, OSPF_MULTI_AREA_IF))
#endif /* HAVE_OSPF_MULTI_AREA */
              ospf_area_link_info_set (oi->area, index, 
                                       LSA_LINK_TYPE_POINTOPOINT,
                                       oi->output_cost, nbr->ident.router_id,
                                       oi->ident.address->u.prefix4,
                                       OSPF_LINK_MAP_NBR, nbr);
#ifdef HAVE_OSPF_MULTI_AREA
            else
              ospf_area_link_info_set (oi->area, index,
                                       LSA_LINK_TYPE_POINTOPOINT,
                                       oi->output_cost, nbr->ident.router_id,
                                       oi->destination->u.prefix4,
                                       OSPF_LINK_MAP_NBR, nbr);
          }
#endif  /* HAVE_OSPF_MULTI_AREA */
        index++;
      }

  if (oi->ident.address
      && !IS_OSPF_IPV4_UNNUMBERED (oi))
#ifdef HAVE_OSPF_MULTI_AREA
    if (!CHECK_FLAG (oi->flags, OSPF_MULTI_AREA_IF))
#endif /* HAVE_OSPF_MULTI_AREA */
      {
        masklen2ip (oi->ident.address->prefixlen, &mask);
        prefix_copy ((struct prefix *)&p, oi->ident.address);
        apply_mask ((struct prefix *)&p);

        ospf_area_link_info_set (oi->area, index, LSA_LINK_TYPE_STUB,
                                 oi->output_cost, p.prefix, mask,
                                 OSPF_LINK_MAP_IF, oi);
        index++;
      }

  return index;
}

static int
ospf_router_lsa_transit_set (struct ospf_interface *oi, int index)
{
  ospf_area_link_info_set (oi->area, index, LSA_LINK_TYPE_TRANSIT,
                           oi->output_cost, oi->ident.d_router,
                           oi->ident.address->u.prefix4,
                           OSPF_LINK_MAP_IF, oi);
  return index + 1;
}

static int
ospf_router_lsa_stub_set (struct ospf_interface *oi, int index)
{
  struct pal_in4_addr id, mask;

  masklen2ip (oi->ident.address->prefixlen, &mask);
  id.s_addr = oi->ident.address->u.prefix4.s_addr & mask.s_addr;

  ospf_area_link_info_set (oi->area, index, LSA_LINK_TYPE_STUB,
                           oi->output_cost, id, mask,
                           OSPF_LINK_MAP_IF, oi);

  return index + 1;
}

static int
ospf_lsa_link_is_transit (struct ospf_interface *oi)
{
  struct ospf_neighbor *dr;
  int count;

  count = ospf_nbr_count_full (oi);
#ifdef HAVE_RESTART
  count += ospf_nbr_count_restart (oi);
#endif /* HAVE_RESTART */
  if (count > 0)
    {
      if (IS_DECLARED_DR (&oi->ident))
        return 1;

      dr = ospf_nbr_lookup_by_addr (oi->nbrs, &oi->ident.d_router);
      if (dr != NULL)
        {
          if (IS_NBR_FULL (dr))
            return 1;
#ifdef HAVE_RESTART
          if (CHECK_FLAG (dr->flags, OSPF_NEIGHBOR_RESTART))
            return 1;
#endif /* HAVE_RESTART */
        }
      else
        {
          if (oi->clone > 1)
            {
              struct ospf_interface *oi_alt;
              struct listnode *node;

              LIST_LOOP (oi->area->iflist, oi_alt, node)
                if (oi_alt->clone > 1)
                  if (oi != oi_alt)
                    if (IPV4_ADDR_SAME (&oi->ident.d_router,
                                        &oi_alt->ident.address->u.prefix4))
                      return 1;
            }
        }
    }
  return 0;
}

static int
ospf_router_lsa_broadcast_set (struct ospf_interface *oi, int index)
{
  /* Describe Type 3 Link. */
  if (oi->state == IFSM_Waiting
      || !ospf_lsa_link_is_transit (oi))
    return ospf_router_lsa_stub_set (oi, index);

  /* Describe Type 2 link. */
  return ospf_router_lsa_transit_set (oi, index);
}

static int
ospf_router_lsa_loopback_set (struct ospf_interface *oi, int index)
{
  if (oi->state != IFSM_Loopback)
    return index;

  ospf_area_link_info_set (oi->area, index, LSA_LINK_TYPE_STUB,
                           oi->output_cost, oi->ident.address->u.prefix4,
                           IPv4HostAddressMask, OSPF_LINK_MAP_IF, oi);
  return index + 1;
}

static int
ospf_router_lsa_p2mp_set (struct ospf_interface *oi, int index)
{
  struct ospf_neighbor *nbr;
  struct ospf_nbr_static *nbr_static;
  struct ls_node *rn;
  u_int16_t cost;

  for (rn = ls_table_top (oi->nbrs); rn; rn = ls_route_next (rn))
    if ((nbr = RN_INFO (rn, RNI_DEFAULT)))
      if (IS_NBR_FULL (nbr))
        {
          cost = oi->output_cost;
          nbr_static = ospf_nbr_static_lookup (oi->top,
                                               nbr->ident.address->u.prefix4);
          if (nbr_static)
            if (CHECK_FLAG (nbr_static->flags, OSPF_NBR_STATIC_COST))
              cost = nbr_static->cost;

          ospf_area_link_info_set (oi->area, index, LSA_LINK_TYPE_POINTOPOINT,
                                   cost, nbr->ident.router_id,
                                   oi->ident.address->u.prefix4,
                                   OSPF_LINK_MAP_NBR, nbr);
          index++;
        }

  ospf_area_link_info_set (oi->area, index, LSA_LINK_TYPE_STUB, 0,
                           oi->ident.address->u.prefix4, IPv4HostAddressMask,
                           OSPF_LINK_MAP_IF, oi);

  return index + 1;
}

static int
ospf_router_lsa_vlink_set (struct ospf_interface *oi, int index)
{
  struct ospf_neighbor *nbr;

  if (oi->state == IFSM_PointToPoint)
    if ((nbr = ospf_nbr_lookup_ptop (oi)))
      if (IS_NBR_FULL (nbr))
        {
          ospf_area_link_info_set (oi->area, index,
                                   LSA_LINK_TYPE_VIRTUALLINK,
                                   oi->output_cost, nbr->ident.router_id,
                                   oi->ident.address->u.prefix4,
                                   OSPF_LINK_MAP_NBR, nbr);
          return index + 1;
        }

  return index;
}

/* Set host link entry info to link map. */
int
ospf_router_lsa_host_entry_set (struct ospf_interface *oi, int index)
{
  struct ospf *top = oi->top;
  struct ospf_host_entry *host = NULL;
  struct ls_node *rn = NULL;

  for (rn = ls_table_top (top->host_table); rn; rn = ls_route_next (rn))
    if ((host = RN_INFO (rn, RNI_DEFAULT)))
      if (IPV4_ADDR_SAME (&host->area_id, &oi->area->area_id) &&
          IPV4_ADDR_SAME (&host->addr, &oi->ident.address->u.prefix4))
        {
          ospf_area_link_info_set (oi->area, index, LSA_LINK_TYPE_STUB,
                                   host->metric, host->addr,
                                   IPv4HostAddressMask,
                                   OSPF_LINK_MAP_HOST, host);
          index++;
        }

  return index;
}

int
ospf_router_lsa_link_vec_set (struct ospf_area *area)
{
  struct ospf_interface *oi;
  struct ospf_link_map *map;
  struct listnode *node;
  int index = 0;
  int max = vector_max (area->link_vec);
  int i;

  LIST_LOOP (area->iflist, oi, node)
    if (ospf_if_is_up (oi))
      if (!CHECK_FLAG (oi->flags, OSPF_IF_DESTROY))
        if (oi->state != IFSM_Down)
          switch (oi->type)
            {
            case OSPF_IFTYPE_POINTOPOINT:
              index = ospf_router_lsa_ptop_set (oi, index);
              break;
            case OSPF_IFTYPE_BROADCAST:
            case OSPF_IFTYPE_NBMA:
              index = ospf_router_lsa_broadcast_set (oi, index);
              break;
            case OSPF_IFTYPE_POINTOMULTIPOINT:
            case OSPF_IFTYPE_POINTOMULTIPOINT_NBMA:
              index = ospf_router_lsa_p2mp_set (oi, index);
              break;
            case OSPF_IFTYPE_VIRTUALLINK:
              index = ospf_router_lsa_vlink_set (oi, index);
              break;
            case OSPF_IFTYPE_LOOPBACK:
              index = ospf_router_lsa_loopback_set (oi, index);
              break;
            case OSPF_IFTYPE_HOST:
              index = ospf_router_lsa_host_entry_set (oi, index);
              break;
            }

  /* Garbage collection. */
  for (i = index; i < max; i++)
    if ((map = vector_slot (area->link_vec, i)))
      {
        ospf_link_map_free (map);
        vector_unset (area->link_vec, i);
      }

  return index;
}

void
ospf_link_vec_unset_by_if (struct ospf_area *area,
                           struct ospf_interface *oi)
{
  struct ospf_link_map *map;
  int i;

  for (i = 0; i < vector_max (area->link_vec); i++)
    if ((map = vector_slot (area->link_vec, i)))
      if (map->source == OSPF_LINK_MAP_IF)
        if (map->nh.oi == oi)
          {
            ospf_link_map_free (map);
            vector_unset (area->link_vec, i);
          }
}

void
ospf_link_vec_unset_by_nbr (struct ospf_area *area,
                            struct ospf_neighbor *nbr)
{
  struct ospf_link_map *map;
  int i;

  for (i = 0; i < vector_max (area->link_vec); i++)
    if ((map = vector_slot (area->link_vec, i)))
      if (map->source == OSPF_LINK_MAP_NBR)
        if (map->nh.nbr == nbr)
          {
            ospf_link_map_free (map);
            vector_unset (area->link_vec, i);
          }
}

void
ospf_link_vec_unset_by_host (struct ospf *top,
                             struct ospf_host_entry *host)
{
  struct ospf_area *area;
  struct ospf_link_map *map;
  int i;

  area = ospf_area_entry_lookup (top, host->area_id);
  if (area != NULL)
    for (i = 0; i < vector_max (area->link_vec); i++)
      if ((map = vector_slot (area->link_vec, i)))
        if (map->source == OSPF_LINK_MAP_HOST)
          if (map->nh.host == host)
            {
              ospf_link_map_free (map);
              vector_unset (area->link_vec, i);
            }
}

static u_int16_t
ospf_router_lsa_link_set (struct stream *s, struct ospf_area *area)
{
  struct ospf_link_map *map;
  u_int16_t links = 0;
  int i;

  ospf_buffer_stream_ensure (s, vector_count (area->link_vec) * 12);

  for (i = 0; i < vector_max (area->link_vec); i++)
    if ((map = vector_slot (area->link_vec, i)))
      {
        stream_put_in_addr (s, &map->id);       /* Link ID. */
        stream_put_in_addr (s, &map->data);     /* Link Data. */
        stream_putc (s, map->type);             /* Link Type. */
        stream_putc (s, 0);                     /* TOS = 0. */
        stream_putw (s, map->metric);           /* Link Cost. */
        links++;
      }
  return links;
}

#define OSPF_ROUTER_LSA_LINK_NUM_OFFSET         22

/* Set router-LSA body. */
void
ospf_router_lsa_body_set (struct stream *s, struct ospf_area *area)
{
  u_int16_t count;

  /* Set flags. */
  stream_putc (s, ospf_router_lsa_flags (area));

  /* Set Zero fields. */
  stream_putc (s, 0);

  /* Set zero for #links. */
  stream_putw (s, 0);

  /* Set all link information. */
  count = ospf_router_lsa_link_set (s, area);

  /* Set # of links here. */
  stream_putw_at (s, OSPF_ROUTER_LSA_LINK_NUM_OFFSET, count);
}

/* Create new router-LSA. */
struct ospf_lsa *
ospf_router_lsa_new (struct ospf_area *area)
{
  struct ospf *top = area->top;
  struct stream *s = top->lbuf;
  struct ospf_lsa *new;
  int links;
  u_int32_t size;
  u_char options;

  /* First, list all possible link into vector. */
  links = ospf_router_lsa_link_vec_set (area);
  size = OSPF_LSA_HEADER_SIZE + 4 + OSPF_LINK_DESC_SIZE * links;
  if (size > OSPF_MAX_ROUTER_LSA_SIZE)
    size = OSPF_MAX_ROUTER_LSA_SIZE;

  /* Reset stream. */
  stream_reset (s);

  /* Set LSA common header fields. */
  options = ospf_lsa_options_get (area, OSPF_ROUTER_LSA, 0);
  ospf_lsa_header_set (s, options, OSPF_ROUTER_LSA,
                       top->router_id, top->router_id);

  /* Set router-LSA body fields. */
  ospf_router_lsa_body_set (s, area);

  /* Set length and create OSPF LSA instance. */
  stream_putw_at (s, OSPF_LSA_LENGTH_OFFSET, stream_get_endp (s));
  new = ospf_lsa_get (top, STREAM_DATA (s), stream_get_endp (s));
  SET_FLAG (new->flags, OSPF_LSA_SELF);

  new->lsdb = area->lsdb;
  new->param = area;

  return new;
}

struct ospf_lsa *
ospf_router_lsa_originate (struct ospf *top, void *param)
{
  struct ospf_area *area = (struct ospf_area *)param;
  struct ospf_lsa *new;

  new = ospf_router_lsa_new (area);

  return new;
}

struct ospf_lsa *
ospf_router_lsa_refresh (struct ospf *top, struct ospf_lsa *lsa)
{
  struct ospf_area *area = lsa->lsdb->u.area;
  struct ospf_lsa *new;

  new = ospf_router_lsa_new (area);

  return new;
}

struct ospf_lsa *
ospf_router_lsa_self (struct ospf_area *area)
{
  return ospf_lsdb_lookup_by_id (area->lsdb, OSPF_ROUTER_LSA,
                                 area->top->router_id, area->top->router_id);
}

void
ospf_router_lsa_refresh_all (struct ospf *top)
{
  struct ospf_area *area;
  struct ls_node *rn;

  for (rn = ls_table_top (top->area_table); rn; rn = ls_route_next (rn))
    if ((area = RN_INFO (rn, RNI_DEFAULT)))
      if (IS_AREA_ACTIVE (area))
        LSA_REFRESH_TIMER_ADD (top, OSPF_ROUTER_LSA,
                               ospf_router_lsa_self (area), area);
}

void
ospf_router_lsa_refresh_by_area_id (struct ospf *top,
                                    struct pal_in4_addr area_id)
{
  struct ospf_area *area;

  area = ospf_area_entry_lookup (top, area_id);
  if (area != NULL)
    if (IS_AREA_ACTIVE (area))
      LSA_REFRESH_TIMER_ADD (top, OSPF_ROUTER_LSA,
                             ospf_router_lsa_self (area), area);
}

void
ospf_router_lsa_refresh_by_interface (struct ospf_interface *oi)
{
  /* Update the connected route.  */
  if (!IS_OSPF_IPV4_UNNUMBERED (oi)
      && oi->type != OSPF_IFTYPE_VIRTUALLINK)
    {
      if (CHECK_FLAG (oi->flags, OSPF_IF_UP))
        ospf_route_add_connected (oi, oi->ident.address);
      else
        ospf_route_delete_connected (oi, oi->ident.address);
    }

  /* Refresh the route LSA.  */
  if (IS_AREA_ACTIVE (oi->area))
    LSA_REFRESH_TIMER_ADD (oi->top, OSPF_ROUTER_LSA,
                           ospf_router_lsa_self (oi->area), oi->area);
}


/* network-LSA related functions. */
/* Originate Network-LSA. */
void
ospf_network_lsa_body_set (struct stream *s, struct ospf_interface *oi)
{
  struct pal_in4_addr mask;
  struct ls_node *rn;
  struct ospf_neighbor *nbr;

  masklen2ip (oi->ident.address->prefixlen, &mask);
  stream_put_ipv4 (s, mask.s_addr);

  /* The network-LSA lists those routers that are fully adjacent to
    the Designated Router; each fully adjacent router is identified by
    its OSPF Router ID.  The Designated Router includes itself in this
    list. RFC2328, Section 12.4.2 */
  stream_put_ipv4 (s, oi->ident.router_id.s_addr);
  for (rn = ls_table_top (oi->nbrs); rn; rn = ls_route_next (rn))
    if ((nbr = RN_INFO (rn, RNI_DEFAULT)))
      if (IS_NBR_FULL (nbr))
        stream_put_ipv4 (s, nbr->ident.router_id.s_addr);
}

struct ospf_lsa *
ospf_network_lsa_new (struct ospf_interface *oi)
{
  struct ospf *top = oi->top;
  struct stream *s = top->lbuf;
  struct ospf_lsa *new;
  u_char options;

  /* If there are no neighbours on this network (the net is stub),
     the router does not originate network-LSA (see RFC 12.4.2) */
  if (oi->full_nbr_count == 0)
    return NULL;

  /* Reset stream. */
  stream_reset (s);

  options = ospf_lsa_options_get (oi->area, OSPF_NETWORK_LSA, 0);
  ospf_lsa_header_set (s, options, OSPF_NETWORK_LSA,
                       oi->ident.d_router, top->router_id);

  /* Set network-LSA body fields. */
  ospf_network_lsa_body_set (s, oi);

  /* Set length and create OSPF LSA instance. */
  stream_putw_at (s, OSPF_LSA_LENGTH_OFFSET, stream_get_endp (s));
  new = ospf_lsa_get (top, STREAM_DATA (s), stream_get_endp (s));
  SET_FLAG (new->flags, OSPF_LSA_SELF);

  new->lsdb = oi->area->lsdb;
  new->param = oi;

  return new;
}

/* Originate network-LSA. */
struct ospf_lsa *
ospf_network_lsa_originate (struct ospf *top, void *param)
{
  struct ospf_interface *oi = (struct ospf_interface *)param;

  return ospf_network_lsa_new (oi);
}

struct ospf_lsa *
ospf_network_lsa_refresh (struct ospf *top, struct ospf_lsa *lsa)
{
  struct ospf_interface *oi = (struct ospf_interface *)lsa->param;

  if (oi != NULL)
    return ospf_network_lsa_new (oi);

  zlog_info (ZG, "Network-LSA can not refresh!!!");

  return NULL;
}

struct ospf_lsa *
ospf_network_lsa_self (struct ospf_interface *oi)
{
  return ospf_lsdb_lookup_by_id (oi->area->lsdb, OSPF_NETWORK_LSA,
                                 oi->ident.address->u.prefix4,
                                 oi->ident.router_id);
}


struct ospf_ia_lsa_map *
ospf_ia_lsa_map_new (u_char type)
{
  struct ospf_ia_lsa_map *map;

  map = XMALLOC (MTYPE_OSPF_SUMMARY_LSA_MAP, sizeof (struct ospf_ia_lsa_map));
  pal_mem_set (map, 0, sizeof (struct ospf_ia_lsa_map));
  map->type = type;

  return map;
}

void
ospf_ia_lsa_map_free (struct ospf_ia_lsa_map *map)
{
  LSA_MAP_FLUSH (map->lsa);
  XFREE (MTYPE_OSPF_SUMMARY_LSA_MAP, map);
}

void
ospf_ia_lsa_map_add (struct ls_table *table, struct ls_prefix *lp,
                      struct ospf_ia_lsa_map *map)
{
  struct ls_node *rn;

  rn = ls_node_get (table, lp);
  if (RN_INFO (rn, RNI_DEFAULT) == NULL)
    {
      map->lp = rn->p;
      RN_INFO_SET (rn, RNI_DEFAULT, map);
    }
  ls_unlock_node (rn);
}

void
ospf_ia_lsa_map_delete (struct ls_table *table, struct ls_prefix *lp)
{
  struct ls_node *rn;

  rn = ls_node_lookup (table, lp);
  if (rn != NULL)
    {
      RN_INFO_UNSET (rn, RNI_DEFAULT);
      ls_unlock_node (rn);
    }
}

struct ospf_ia_lsa_map *
ospf_ia_lsa_map_lookup (struct ls_table *table, struct ls_prefix *lp)
{
  struct ls_node *rn;

  rn = ls_node_lookup (table, lp);
  if (rn)
    {
      ls_unlock_node (rn);
      return RN_INFO (rn, RNI_DEFAULT);
    }
  return NULL;
}

void
ospf_ia_lsa_map_clear (struct ls_table *table)
{
  struct ls_node *rn;
  struct ospf_ia_lsa_map *map;

  for (rn = ls_table_top (table); rn; rn = ls_route_next (rn))
    if ((map = RN_INFO (rn, RNI_DEFAULT)))
      {
        ospf_ia_lsa_map_free (map);
        RN_INFO_UNSET (rn, RNI_DEFAULT);
      }
}


/* summary-LSA related functions. */
void
ospf_summary_lsa_body_set (struct stream *s, struct prefix *p,
                           u_int32_t metric)
{
  struct pal_in4_addr mask;

  masklen2ip (p->prefixlen, &mask);

  /* Put Network Mask. */
  stream_put_ipv4 (s, mask.s_addr);

  /* Set # TOS. */
  stream_putc (s, (u_char) 0);

  /* Set metric. */
  STREAM_PUT_METRIC (s, metric);
}

struct ospf_lsa *
ospf_summary_lsa_new (struct ospf_ia_lsa_map *map, struct pal_in4_addr id)
{
  struct ospf *top = map->area->top;
  struct stream *s = top->lbuf;
  struct ospf_lsa *new;
  u_int32_t cost;
  u_char options;

  /* Reset stream. */
  stream_reset (s);

  /* Set header. */
  options = ospf_lsa_options_get (map->area, OSPF_SUMMARY_LSA, 0);

#ifdef HAVE_VRF_OSPF
  /* Set the DN-bit for the Summary LSA */
 if (CHECK_FLAG (map->flags, OSPF_SET_SUMMARY_DN_BIT))
    options |= OSPF_OPTION_DN;
#endif /* HAVE_VRF_OSPF */

  ospf_lsa_header_set (s, options, OSPF_SUMMARY_LSA, id, top->router_id);

  if (map->type == OSPF_IA_MAP_PATH)
    {
#ifdef HAVE_VRF_OSPF
    /* If the OSPF_SET_SUMMARY_DN_BIT flag is set then the route
       is a VPN redistributed route recieived from a remote PE
       so the cost need to be updated from map->cost*/
     if (CHECK_FLAG (map->flags, OSPF_SET_SUMMARY_DN_BIT))
       cost = map->cost;
     else
#endif /* HAVE_VRF_OSPF */
        cost = OSPF_PATH_COST (map->u.path);
    }
  else if (map->type == OSPF_IA_MAP_RANGE)
    cost = map->u.range->cost;
  else /* STUB. */
    cost = OSPF_AREA_DEFAULT_COST (map->area);

  /* Set summary-LSA body fields. */
  ospf_summary_lsa_body_set (s, (struct prefix *)map->lp, cost);

  /* Set length and create OSPF LSA instance. */
  stream_putw_at (s, OSPF_LSA_LENGTH_OFFSET, stream_get_endp (s));
  new = ospf_lsa_get (top, STREAM_DATA (s), stream_get_endp (s));
  SET_FLAG (new->flags, OSPF_LSA_SELF);

  if (map->lsa != NULL)
    ospf_lsa_unlock (map->lsa);
  map->lsa = ospf_lsa_lock (new);

  new->lsdb = map->area->lsdb;
  new->param = map;

  return new;
}

struct ospf_lsa *
ospf_summary_lsa_originate (struct ospf *top, void *param)
{
  struct ospf_ia_lsa_map *map = (struct ospf_ia_lsa_map *)param;
  struct ls_prefix *lp = map->lp;
  struct pal_in4_addr id;
  struct ospf_lsa *other;
  
  if (map->lsa != NULL)
    id = map->lsa->data->id;
  else
    pal_mem_cpy (&id, lp->prefix, sizeof (struct pal_in4_addr));

  other = ospf_lsdb_lookup_by_id (map->area->lsdb, OSPF_SUMMARY_LSA,
                                  id, top->router_id);
  if (other != NULL
      && !IS_LSA_MAXAGE (other)
      && !CHECK_FLAG (other->flags, OSPF_LSA_DISCARD))
    if (ospf_lsa_update_unique (top, other, lp) < 0)
      if (ospf_lsa_id_increment (&id, lp->prefixlen))
        if ((map->lsa = ospf_lsa_map_lsa_id_update (top, map->lsa, id)))
          {
            ospf_lsa_lock (map->lsa);
            return ospf_summary_lsa_originate (top, param);
          }

  return ospf_summary_lsa_new (map, id);
}

struct ospf_lsa *
ospf_summary_lsa_refresh (struct ospf *top, struct ospf_lsa *lsa)
{
  struct ospf_ia_lsa_map *map = (struct ospf_ia_lsa_map *)lsa->param;
  struct ospf_ia_lsa_map *tmp_map;
  struct ls_prefix *lp = map->lp, new_lp;
  struct pal_in4_addr id;
  struct ospf_lsa *other;
  struct pal_in4_addr *mask;
  struct prefix_ipv4 prefix;
  u_char masklen;
  struct ls_node *rn;

  tmp_map = NULL;
  mask = NULL;
  masklen = 0;
  rn = NULL;
 
  if (map->lsa != NULL)
    id = map->lsa->data->id;
  else
    pal_mem_cpy (&id, lp->prefix, sizeof (struct pal_in4_addr));

  other = ospf_lsdb_lookup_by_id (map->area->lsdb, OSPF_SUMMARY_LSA,
                                  id, top->router_id);
  if (other != NULL
      && !IS_LSA_MAXAGE (other)
      && !CHECK_FLAG (other->flags, OSPF_LSA_DISCARD))
    if (ospf_lsa_update_unique (top, other, lp) < 0)
      if (ospf_lsa_id_increment (&id, lp->prefixlen))
        {
          /* Find the correct map for the lsa */
          /* First set the prefix and use that to find the map */
          mask = OSPF_PNT_IN_ADDR_GET (lsa->data,
                                       OSPF_SUMMARY_LSA_NETMASK_OFFSET);
          masklen = ip_masklen (*mask);
          prefix.prefixlen = masklen;
          pal_mem_cpy (&prefix.prefix, &lsa->data->id,
                       sizeof (struct pal_in4_addr));
          ls_prefix_ipv4_set (&new_lp, masklen, prefix.prefix);
          rn = ls_node_lookup (map->area->ia_prefix, &new_lp);
          if (rn)
            {
              tmp_map = RN_INFO (rn, RNI_DEFAULT);
              if (tmp_map && (tmp_map->lsa = ospf_lsa_map_lsa_id_update (
                                  top, tmp_map->lsa, id)))
                {
                  /* Set the map */
                  ospf_lsa_lock (tmp_map->lsa);
                  tmp_map->lsa->param = tmp_map;
                  return ospf_summary_lsa_refresh (top, tmp_map->lsa);
                }
            }
          }

  return ospf_summary_lsa_new (map, id);
}


void
ospf_asbr_summary_lsa_body_set (struct stream *s, u_int32_t metric)
{
  /* Put Network Mask. */
  stream_put_ipv4 (s, 0);

  /* Set # TOS. */
  stream_putc (s, (u_char) 0);

  /* Set metric. */
  STREAM_PUT_METRIC (s, metric);
}

struct ospf_lsa *
ospf_asbr_summary_lsa_new (struct ospf_ia_lsa_map *map)
{
  struct ospf *top = map->area->top;
  struct stream *s = top->lbuf;
  struct ospf_lsa *new;
  struct pal_in4_addr id;
  u_char options;

  /* Reset stream. */
  stream_reset (s);

  pal_mem_cpy (&id, map->lp->prefix, sizeof (struct pal_in4_addr));

  options = ospf_lsa_options_get (map->area, OSPF_SUMMARY_LSA_ASBR, 0);
  ospf_lsa_header_set (s, options, OSPF_SUMMARY_LSA_ASBR, id, top->router_id);

  /* Set summary-LSA body fields. */
  ospf_asbr_summary_lsa_body_set (s, OSPF_PATH_COST (map->u.path));

  /* Set length and create OSPF LSA instance. */
  stream_putw_at (s, OSPF_LSA_LENGTH_OFFSET, stream_get_endp (s));
  new = ospf_lsa_get (top, STREAM_DATA (s), stream_get_endp (s));
  SET_FLAG (new->flags, OSPF_LSA_SELF);

  if (map->lsa != NULL)
    ospf_lsa_unlock (map->lsa);
  map->lsa = ospf_lsa_lock (new);

  new->lsdb = map->area->lsdb;
  new->param = map;

  return new;
}

struct ospf_lsa *
ospf_asbr_summary_lsa_originate (struct ospf *top, void *param)
{
  struct ospf_ia_lsa_map *map = (struct ospf_ia_lsa_map *)param;

  return ospf_asbr_summary_lsa_new (map);
}

struct ospf_lsa *
ospf_asbr_summary_lsa_refresh (struct ospf *top, struct ospf_lsa *lsa)
{
  struct ospf_ia_lsa_map *map = (struct ospf_ia_lsa_map *)lsa->param;

  return ospf_asbr_summary_lsa_new (map);
}


#define STREAM_PUT_MASK_IN_ADDR(S,L)                                          \
    do {                                                                      \
      struct pal_in4_addr _mask;                                              \
      masklen2ip ((L), &_mask);                                               \
      stream_put_in_addr ((S), &_mask);                                       \
    } while (0)

void
ospf_external_lsa_body_set (struct stream *s, struct ospf_redist_map *map,
                            struct ls_prefix *lp)
{
  /* Put Network Mask. */
  STREAM_PUT_MASK_IN_ADDR (s, lp->prefixlen);

  /* Put external metric type. */
  stream_putc (s, (map->arg.metric_type == EXTERNAL_METRIC_TYPE_2 ? 0x80 : 0));

  /* Put 0 metric. TOS metric is not supported. */
  STREAM_PUT_METRIC (s, map->arg.metric);

  /* Put forwarding address. */
  stream_put_in_addr (s, &map->arg.nexthop);

  /* Put route tag */
  stream_putl (s, map->arg.tag);
}

/* RFC2328 section 12.4.4.1 and RFC3101 section 2.4.  */
int
ospf_external_lsa_prefer_other (struct ospf_lsa *lsa, struct ospf_lsa *self)
{
  struct ospf_route *route;
  struct ospf_area *area = NULL;
  struct ospf_path *path;
  struct ospf *top = NULL;
  u_int32_t metric, metric_self;
  struct pal_in4_addr *dest, *dest_self;

  if (self->lsdb != NULL)
    {
      area = self->lsdb->u.area;
      top = ospf_proc_lookup_by_lsdb (self->lsdb);

      if (area == NULL || top == NULL)
        return 0;

      if (lsa->data->length != self->data->length)
        return 0;

      route = ospf_route_lookup (top, &lsa->data->adv_router, IPV4_MAX_BITLEN);
      if (route != NULL)
        {
          path = ospf_route_path_lookup_by_area (route, area);
          if (path == NULL)
            return 0;
        }
    }

  /*  RFC2328 12.4.4.1

      The following rule is thereby established: if two routers,
      both reachable from one another, originate functionally equivalent
      AS-external-LSAs (i.e., same destination, cost and non-zero
      forwarding address), then the LSA originated by the router having
      the highest OSPF Router ID is used.  The router having the lower
      OSPF Router ID can then flush its LSA.  Flushing an LSA
      is discussed in Section 14.1.  */

  dest_self = OSPF_PNT_IN_ADDR_GET (self->data, OSPF_EXTERNAL_LSA_DEST_OFFSET);
  dest = OSPF_PNT_IN_ADDR_GET (lsa->data, OSPF_EXTERNAL_LSA_DEST_OFFSET);
  metric = OSPF_PNT_UINT24_GET (lsa->data, OSPF_EXTERNAL_LSA_METRIC_OFFSET);
  metric_self = OSPF_PNT_UINT24_GET (self->data,
                                     OSPF_EXTERNAL_LSA_METRIC_OFFSET);

  /* checking for same destination address and metric */
  if ((dest_self->s_addr == dest->s_addr) && (metric == metric_self))
    {
#ifdef HAVE_NSSA
      /*  RFC3101 2.4

          Preference between two Type-7 LSAs is determined by the following
          tie breaker rules:

          1. An LSA with the P-bit set is preferred over one with the P-bit
             clear.

          2. If the P-bit settings are the same, the LSA with the higher
             router ID is preferred.  */

      if (lsa->data->type == OSPF_AS_NSSA_LSA)
        if (CHECK_FLAG (lsa->data->options, OSPF_OPTION_NP)
            != CHECK_FLAG (self->data->options, OSPF_OPTION_NP))
          return CHECK_FLAG (lsa->data->options, OSPF_OPTION_NP) ? 1 : 0;
#endif /* HAVE_NSSA */

      if (IPV4_ADDR_CMP (&lsa->data->adv_router, &self->data->adv_router) > 0)
        return 1;
    }

  return 0;
}

int
ospf_translated_lsa_conditional_check (struct ospf *top)
{
  struct ospf_route *route = NULL;
  struct ls_node *rn, *rn1;
  struct ospf_path *path;
  struct ospf_area *area;
  struct ospf_lsa *lsa;
  u_char bits;
   
  if (top == NULL)
    return 1;

  for (rn = ls_table_top (top->area_table); rn; rn = ls_route_next (rn))
    if ((area = RN_INFO (rn, RNI_DEFAULT)))
      if (IS_AREA_ACTIVE (area)
          && IS_AREA_NSSA (area))
#ifdef HAVE_NSSA
        if (CHECK_FLAG (area->translator_flag, OSPF_NSSA_TRANSLATOR_UNCONDL))      
          {
            LSDB_LOOP (ROUTER_LSDB (area), rn1, lsa)
            if (!IS_LSA_SELF (lsa))
              {
                bits = OSPF_PNT_UCHAR_GET (lsa->data, 
                                           OSPF_ROUTER_LSA_BITS_OFFSET);
                if (!(IS_ROUTER_LSA_SET (bits, BIT_B)
                      && IS_ROUTER_LSA_SET (bits, BIT_NT)))
                  continue;   
                
                route = ospf_route_asbr_lookup (area->top, 
                                                lsa->data->adv_router);
                if (route == NULL)
                  {
                    route = ospf_route_new ();
                    ospf_route_asbr_add (area->top, 
                                         lsa->data->adv_router, route);
                  }
                else
                  {
                    path = ospf_route_path_lookup (route, 
                                                   OSPF_PATH_INTRA_AREA, area);
                    if (path == NULL)
                       return 0;
                  }
                  
                if (IPV4_ADDR_CMP (&top->router_id, 
                                   &lsa->data->adv_router) < 0)  
                  {
                    ls_unlock_node (rn1);  
                    return 1;  
                  } 
                }
            return 0;   
           }
#endif /* HAVE_NSSA */          

   return 1;
}


int
ospf_external_lsa_equivalent (struct ls_table *table, struct ospf_lsa *self)
{
  struct ls_node *rn, *rn_tmp;
  struct pal_in4_addr *nh, *nh_self;
  struct ospf *top = NULL;
  struct ls_prefix_ls lp;
  struct ospf_lsa *lsa;
  int ret = 0;

  if (self->lsdb)
    top = ospf_proc_lookup_by_lsdb (self->lsdb);

  /* Forwarding address should be set to check the functional
     equivalence.  */
  nh_self = OSPF_PNT_IN_ADDR_GET (self->data, OSPF_EXTERNAL_LSA_NEXTHOP_OFFSET);
  if (nh_self->s_addr == 0)
    return 0;

  pal_mem_set (&lp, 0, sizeof (struct ls_prefix_ls));
  lp.prefixsize = OSPF_LSDB_LOWER_TABLE_DEPTH;
  lp.prefixlen = 32;
  lp.id = self->data->id;

  rn_tmp = ls_node_get (table, (struct ls_prefix *)&lp);

  for (rn = ls_lock_node (rn_tmp); rn; rn = ls_route_next_until (rn, rn_tmp))
    if ((lsa = RN_INFO (rn, RNI_LSDB_LSA)))
      {
        nh = OSPF_PNT_IN_ADDR_GET (lsa->data, OSPF_EXTERNAL_LSA_NEXTHOP_OFFSET);

        /*Checking for same forwarding address for functionally equivalent LSA*/
        if (!IS_LSA_SELF (lsa)
            && !IS_LSA_MAXAGE (lsa) && (nh_self->s_addr == nh->s_addr))
          if (ospf_external_lsa_prefer_other (lsa, self)
              && ospf_translated_lsa_conditional_check (top))
            {
              ret = 1;
              break;
            }
      }

  ls_unlock_node (rn_tmp);

  return ret;
}

int
ospf_external_lsa_equivalent_update_self (struct ls_table *table,
                                          struct ospf_lsa *lsa)
{
  struct ls_table *lsdb_table = lsa->lsdb->type[lsa->data->type].db;
  struct ospf_redist_map *map;
  struct pal_in4_addr *mask;
  struct pal_in4_addr *nh;
  struct ls_prefix lp;
  struct ls_node *rn;
  int ret = 0;

  nh = OSPF_PNT_IN_ADDR_GET (lsa->data, OSPF_EXTERNAL_LSA_NEXTHOP_OFFSET);
  if (nh->s_addr == 0)
    return 0;

  mask = OSPF_PNT_IN_ADDR_GET (lsa->data, OSPF_EXTERNAL_LSA_NETMASK_OFFSET);

  ls_prefix_ipv4_set (&lp, ip_masklen (*mask), lsa->data->id);

  rn = ls_node_lookup (table, &lp);
  if (rn != NULL)
    {
      map = RN_INFO (rn, RNI_DEFAULT);
      if (map != NULL)
        {
          if (map->lsa == NULL)
            {
              if (!ospf_external_lsa_equivalent (lsdb_table, lsa))
                ospf_redist_map_lsa_refresh (map);
            }
          else if (ospf_external_lsa_equivalent (lsdb_table, map->lsa))
            {
              ospf_lsdb_lsa_discard (map->lsa->lsdb, map->lsa, 1, 1, 1);
              ospf_lsa_unlock (map->lsa);
              map->lsa = NULL;
            }
          ret = 1;
        }
      ls_unlock_node (rn);
    }
  return ret;
}

int
ospf_no_route_to_asbr (struct ospf *top, struct ls_table *table, 
                       struct ospf_lsa *self)
{
  struct ls_node *rn, *rn_tmp;
  struct pal_in4_addr *nh;
  struct ls_prefix_ls lp;
  struct ospf_lsa *lsa;
  u_char *data, *data_self;
  struct ospf_route *route_base;
  int len;
  int ret = 0;

  /* Forwarding address should be set to check the functional
     equivalence.  */
  nh = OSPF_PNT_IN_ADDR_GET (self->data, OSPF_EXTERNAL_LSA_NEXTHOP_OFFSET);
  if (nh->s_addr == 0)
    return 0;

  pal_mem_set (&lp, 0, sizeof (struct ls_prefix_ls));
  lp.prefixsize = OSPF_LSDB_LOWER_TABLE_DEPTH;
  lp.prefixlen = 32;
  lp.id = self->data->id;

  rn_tmp = ls_node_get (table, (struct ls_prefix *)&lp);

  for (rn = ls_lock_node (rn_tmp); rn; rn = ls_route_next_until (rn, rn_tmp))
    if ((lsa = RN_INFO (rn, RNI_LSDB_LSA)))
      if (!IS_LSA_SELF (lsa)
          && !IS_LSA_MAXAGE (lsa))
        {
          if (lsa->data->length != self->data->length)
            return 0;

          data = (u_char *)(lsa->data + 1);
          data_self = (u_char *)(self->data + 1);

          len = pal_ntoh16 (lsa->data->length) - OSPF_LSA_HEADER_SIZE;

          if (!pal_mem_cmp (data, data_self, len))
            {        
              route_base = ospf_route_asbr_lookup (top, lsa->data->adv_router);

              if (route_base == NULL)
                {
                  zlog_info (ZG, "no route to ASBR");
                  ret = 1;
                  break;
                }
            } 
        }

  ls_unlock_node (rn_tmp);

  return ret;
}

struct ospf_lsa *
ospf_external_lsa_new (struct ospf *top, struct ospf_redist_map *map,
                       struct pal_in4_addr id)
{
  struct ls_table *table = top->lsdb->type[OSPF_AS_EXTERNAL_LSA].db;
  struct stream *s = top->lbuf;
  struct ls_prefix *lp = map->lp;
  struct ospf_lsa *new;
  u_char options;

  /* Reset stream. */
  stream_reset (s);

  /* Set LSA common header fields. */
  options = ospf_lsa_options_get (NULL, OSPF_AS_EXTERNAL_LSA, 0);

  /* Set the DN-bit for external LSA */
  if (CHECK_FLAG (map->flags, OSPF_REDIST_EXTERNAL_SET_DN_BIT))
     options |= OSPF_OPTION_DN;

  ospf_lsa_header_set (s, options, OSPF_AS_EXTERNAL_LSA, id, top->router_id);

  /* Set AS-external-LSA body fields. */
  ospf_external_lsa_body_set (s, map, lp);

  /* Set length and create OSPF LSA instance. */
  stream_putw_at (s, OSPF_LSA_LENGTH_OFFSET, stream_get_endp (s));
  new = ospf_lsa_get (top, STREAM_DATA (s), stream_get_endp (s));
  SET_FLAG (new->flags, OSPF_LSA_SELF);
  new->lsdb = top->lsdb;

  if (ospf_no_route_to_asbr (top, table, new))
    goto NEW;

  /* Check if the equivalent LSA already exists in the LSDB.  */
  if (ospf_external_lsa_equivalent (table, new))
    {
      if (map->lsa != NULL)
        {
          ospf_lsa_unlock (map->lsa);
          map->lsa = NULL;
        }

      /* Discard the new LSA.  */
      new->lsdb = NULL;
      ospf_lsa_discard (new);

      return NULL;
    }

  NEW:
  if (map->lsa)
    ospf_lsa_unlock (map->lsa);
  map->lsa = ospf_lsa_lock (new);

  new->param = map;

  return new;
}

/* Originate an AS-external-LSA, install and flood. */
struct ospf_lsa *
ospf_external_lsa_originate (struct ospf *top, void *param)
{
  struct ospf_redist_map *map = (struct ospf_redist_map *)param;
  struct ls_prefix *lp = map->lp;
  struct pal_in4_addr id;
  struct ospf_lsa *other;

  if (map->lsa != NULL)
    id = map->lsa->data->id;
  else
    pal_mem_cpy (&id, lp->prefix, sizeof (struct pal_in4_addr));

  other = ospf_lsdb_lookup_by_id (top->lsdb, OSPF_AS_EXTERNAL_LSA,
                                  id, top->router_id);
  if (other != NULL
      && !IS_LSA_MAXAGE (other)
      && !CHECK_FLAG (other->flags, OSPF_LSA_DISCARD))
    if (ospf_lsa_update_unique (top, other, lp) < 0)
      if (ospf_lsa_id_increment (&id, lp->prefixlen))
        if ((map->lsa = ospf_lsa_map_lsa_id_update (top, map->lsa, id)))
          {
            ospf_lsa_lock (map->lsa);
            return ospf_external_lsa_originate (top, param);
          }

  return ospf_external_lsa_new (top, map, id);
}

/* Refresh AS-external-LSA. */
struct ospf_lsa *
ospf_external_lsa_refresh (struct ospf *top, struct ospf_lsa *lsa)
{
  struct ospf_redist_map *map = (struct ospf_redist_map *)lsa->param;
  struct ospf_redist_map *tmp_map;
  struct ls_prefix *lp = map->lp, new_lp;
  struct pal_in4_addr id;
  struct ospf_lsa *other;
  struct pal_in4_addr *mask;
  struct prefix_ipv4 prefix;
  u_char masklen;
  struct ls_node *rn;

  if (map->lsa != NULL)
    id = map->lsa->data->id;
  else
    pal_mem_cpy (&id, lp->prefix, sizeof (struct pal_in4_addr));

  other = ospf_lsdb_lookup_by_id (top->lsdb, OSPF_AS_EXTERNAL_LSA,
                                  id, top->router_id);
  if (other != NULL
      && !IS_LSA_MAXAGE (other)
      && !CHECK_FLAG (other->flags, OSPF_LSA_DISCARD))
    if (ospf_lsa_update_unique (top, other, lp) < 0)
      if (ospf_lsa_id_increment (&id, lp->prefixlen))
        {
          /* Find the correct map for the lsa */
          /* First set the prefix and use that to find the map */
          mask = OSPF_PNT_IN_ADDR_GET (lsa->data,
                                       OSPF_EXTERNAL_LSA_NETMASK_OFFSET);
          masklen = ip_masklen (*mask);
          prefix.prefixlen = masklen;
          pal_mem_cpy (&prefix.prefix, &lsa->data->id,
                       sizeof (struct pal_in4_addr));
          apply_mask_ipv4 (&prefix);
          ls_prefix_ipv4_set (&new_lp, masklen, prefix.prefix);
          rn = ls_node_lookup (top->redist_table, &new_lp);
          if (rn)
            {
              tmp_map = RN_INFO (rn, RNI_DEFAULT);
              if (tmp_map && (tmp_map->lsa = ospf_lsa_map_lsa_id_update (
                                  top, tmp_map->lsa, id)))
                {
                  /* Find the map and set the map */
                  ospf_lsa_lock (tmp_map->lsa);
                  tmp_map->lsa->param = tmp_map;
                  return ospf_external_lsa_refresh (top, tmp_map->lsa);
                }
            }
        }

  return ospf_external_lsa_new (top, map, id);
}


#ifdef HAVE_NSSA
struct ospf_lsa *
ospf_nssa_lsa_new (struct ospf_area *area, struct ospf_redist_map *map,
                   struct pal_in4_addr id)
{
  struct ls_table *table = area->lsdb->type[OSPF_AS_NSSA_LSA].db;
  struct stream *s = area->top->lbuf;
  struct ls_prefix *lp = map->lp;
  struct ospf_lsa *new;
  u_char options;

  /* Reset stream. */
  stream_reset (s);

  /* Set LSA common header fields. */
  options = ospf_lsa_options_get (map->u.area, OSPF_AS_NSSA_LSA,
                                  map->arg.propagate);
  ospf_lsa_header_set (s, options, OSPF_AS_NSSA_LSA, id, area->top->router_id);

  /* Set AS-external-LSA body fields. */
  ospf_external_lsa_body_set (s, map, lp);

  /* Set length and create OSPF LSA instance. */
  stream_putw_at (s, OSPF_LSA_LENGTH_OFFSET, stream_get_endp (s));
  new = ospf_lsa_get (area->top, STREAM_DATA (s), stream_get_endp (s));
  SET_FLAG (new->flags, OSPF_LSA_SELF);
  new->lsdb = map->u.area->lsdb;

  /* Check if the equivalent LSA already exists in the LSDB.  */
  if (ospf_external_lsa_equivalent (table, new))
    {
      if (map->lsa != NULL)
        {
          ospf_lsa_unlock (map->lsa);
          map->lsa = NULL;
        }

      /* Discard the new LSA.  */
      new->lsdb = NULL;
      ospf_lsa_discard (new);

      return NULL;
    }

  if (map->lsa != NULL)
    ospf_lsa_unlock (map->lsa);
  map->lsa = ospf_lsa_lock (new);

  new->param = map;

  return new;
}

/* Originate an AS-NSSA-LSA, install and flood. */
struct ospf_lsa *
ospf_nssa_lsa_originate (struct ospf *top, void *param)
{
  struct ospf_redist_map *map = (struct ospf_redist_map *)param;
  struct ospf_area *area = map->u.area;
  struct ls_prefix *lp = map->lp;
  struct pal_in4_addr id;
  struct ospf_lsa *other;

  if (map->lsa != NULL)
    id = map->lsa->data->id;
  else
    pal_mem_cpy (&id, lp->prefix, sizeof (struct pal_in4_addr));

  other = ospf_lsdb_lookup_by_id (area->lsdb, OSPF_AS_NSSA_LSA,
                                  id, top->router_id);
  if (other != NULL
      && !IS_LSA_MAXAGE (other)
      && !CHECK_FLAG (other->flags, OSPF_LSA_DISCARD))
    if (ospf_lsa_update_unique (top, other, lp) < 0)
      if (ospf_lsa_id_increment (&id, lp->prefixlen))
        if ((map->lsa = ospf_lsa_map_lsa_id_update (top, map->lsa, id)))
          {
            ospf_lsa_lock (map->lsa);
            return ospf_nssa_lsa_originate (top, param);
          }

  return ospf_nssa_lsa_new (area, map, id);
}

/* Refresh AS-NSSA-LSA. */
struct ospf_lsa *
ospf_nssa_lsa_refresh (struct ospf *top, struct ospf_lsa *lsa)
{
  struct ospf_redist_map *map = (struct ospf_redist_map *)lsa->param;
  struct ospf_redist_map *tmp_map;
  struct ls_prefix *lp = map->lp, new_lp;
  struct ospf_area *area = map->u.area;
  struct pal_in4_addr id;
  struct ospf_lsa *other;
  struct pal_in4_addr *mask;
  struct prefix_ipv4 prefix;
  u_char masklen;
  struct ls_node *rn;
  
  tmp_map = NULL;
  mask = NULL;
  masklen = 0;
  rn = NULL;

  if (map->lsa != NULL)
    id = map->lsa->data->id;
  else
    pal_mem_cpy (&id, lp->prefix, sizeof (struct pal_in4_addr));

  other = ospf_lsdb_lookup_by_id (area->lsdb, OSPF_AS_NSSA_LSA,
                                  id, top->router_id);
  if (other != NULL
      && !IS_LSA_MAXAGE (other)
      && !CHECK_FLAG (other->flags, OSPF_LSA_DISCARD))
    if (ospf_lsa_update_unique (top, other, lp) < 0)
      if (ospf_lsa_id_increment (&id, lp->prefixlen))
        {
          /* Find the correct map for the lsa */
          /* First set the prefix and use that to find the map */
          mask = OSPF_PNT_IN_ADDR_GET (lsa->data,
                                       OSPF_EXTERNAL_LSA_NETMASK_OFFSET);
          masklen = ip_masklen (*mask);
          prefix.prefixlen = masklen;
          pal_mem_cpy (&prefix.prefix, &lsa->data->id,
                       sizeof (struct pal_in4_addr));
          ls_prefix_ipv4_set (&new_lp, masklen, prefix.prefix);
          rn = ls_node_lookup (top->redist_table, &new_lp);
          if (rn)
            {
              tmp_map = RN_INFO (rn, RNI_DEFAULT);
              if (tmp_map && (tmp_map->lsa = ospf_lsa_map_lsa_id_update (
                                  top, tmp_map->lsa, id)))
                {
                  /* Find the map and set the map */
                  ospf_lsa_lock (tmp_map->lsa);
                  tmp_map->lsa->param = tmp_map;
                  return ospf_nssa_lsa_refresh (top, tmp_map->lsa);
                }
            }
        }

  return ospf_nssa_lsa_new (area, map, id);
}
#endif /* HAVE_NSSA */

#ifdef HAVE_OPAQUE_LSA
struct ospf_lsa *
ospf_opaque_lsa_new (struct ospf *top, struct ospf_opaque_map *map)
{
  u_char type = OPAQUE_LSA_TYPE_BY_SCOPE (map->lsdb->scope);
  struct stream *s = top->lbuf;
  struct ospf_area *area;
  struct ospf_lsa *new;
  u_char options;

  /* Reset stream. */
  stream_reset (s);

  if (map->lsdb->scope ==  LSA_FLOOD_SCOPE_LINK)
    area = map->lsdb->u.oi->area;
  else if (map->lsdb->scope == LSA_FLOOD_SCOPE_AREA)
    area = map->lsdb->u.area;
  else
    return NULL;

  options = ospf_lsa_options_get (area, type, 0);
  ospf_lsa_header_set (s, options, type, map->id, top->router_id);

  stream_put (s, map->data, map->length);

  /* Set length and create OSPF LSA instance. */
  stream_putw_at (s, OSPF_LSA_LENGTH_OFFSET, stream_get_endp (s));
  new = ospf_lsa_get (top, STREAM_DATA (s), stream_get_endp (s));
  SET_FLAG (new->flags, OSPF_LSA_SELF);

  if (map->lsa != NULL)
    ospf_lsa_unlock (map->lsa);
  map->lsa = ospf_lsa_lock (new);

  new->param = map;
  new->lsdb = map->lsdb;

  return new;
}

struct ospf_lsa *
ospf_opaque_lsa_originate (struct ospf *top, void *param)
{
  struct ospf_opaque_map *map = (struct ospf_opaque_map *)param;

  return ospf_opaque_lsa_new (top, map);
}

struct ospf_lsa *
ospf_opaque_lsa_refresh (struct ospf *top, struct ospf_lsa *lsa)
{
  struct ospf_opaque_map *map = (struct ospf_opaque_map *)lsa->param;

  return ospf_opaque_lsa_new (top, map);
}
#endif /* HAVE_OPAQUE_LSA */


/* LSA installation functions. */
struct ospf_lsa *
ospf_router_lsa_install (struct ospf *top, int rt_recalc,
                         struct ospf_lsa *old, struct ospf_lsa *new)
{
#ifdef HAVE_RESTART
  struct ospf_master *om = top->om;
#endif /* HAVE_RESTART */

  /* RFC 2328 Section 13.2 Router-LSAs and network-LSAs
     The entire routing table must be recalculated, starting with
     the shortest path calculations for each area (not just the
     area whose link-state database has changed).  */
  struct ospf_area *area = new->lsdb->u.area;

  if (rt_recalc ||
      (area->t_spf_calc != NULL &&
       IS_AREA_THREAD_SUSPENDED (area, area->t_spf_calc)))
    ospf_spf_calculate_timer_add (area);

#ifdef HAVE_RESTART
  /* Router receives an LSA that is inconsistent with its pre-restart
     router-LSA, exit graceful restart. */
  if (IS_GRACEFUL_RESTART (top))
    if (!ospf_restart_router_lsa_check (top, new))
      {
        if (IS_DEBUG_OSPF (event, EVENT_ROUTER))
          zlog_info (ZG, "ROUTER[Process:%d]: Exit Restarting by receiving "
                     "inconsistent pre-restart router-LSA", top->ospf_id);

        ospf_restart_exit (top);
      }
#endif /* HAVE_RESTART */

  return new;
}

struct ospf_lsa *
ospf_network_lsa_install (struct ospf *top, int rt_recalc,
                          struct ospf_lsa *old, struct ospf_lsa *new)
{
  /* RFC 2328 Section 13.2 Router-LSAs and network-LSAs
     The entire routing table must be recalculated, starting with
     the shortest path calculations for each area (not just the
     area whose link-state database has changed).  */

  if (rt_recalc)
    ospf_spf_calculate_timer_add (new->lsdb->u.area);

  return new;
}

struct ospf_lsa *
ospf_summary_lsa_install (struct ospf *top, int rt_recalc,
                          struct ospf_lsa *old, struct ospf_lsa *new)
{
  struct pal_in4_addr *omask;
  struct pal_in4_addr *nmask;
  u_char omasklen;
  u_char nmasklen;

  /* RFC 2328 Section 13.2 Summary-LSAs
     The best route to the destination described by the summary-
     LSA must be recalculated (see Section 16.5).  If this
     destination is an AS boundary router, it may also be
     necessary to re-examine all the AS-external-LSAs.  */
  if (!IS_LSA_SELF (new))
    {
      if (rt_recalc)
        {
          ospf_ia_calc_incremental_event_add (new);

          if (old)
            {
              omask = OSPF_PNT_IN_ADDR_GET (old->data,
                                            OSPF_SUMMARY_LSA_NETMASK_OFFSET);
              nmask = OSPF_PNT_IN_ADDR_GET (new->data,
                                            OSPF_SUMMARY_LSA_NETMASK_OFFSET);
              omasklen = ip_masklen (*omask);
              nmasklen = ip_masklen (*nmask);

              if (omasklen != nmasklen)
                ospf_ia_calc_incremental_event_add (old);
            }
        }
      else
        ospf_route_path_summary_lsa_set (top, new);
    }

  return new;
}

struct ospf_lsa *
ospf_asbr_summary_lsa_install (struct ospf *top, int rt_recalc,
                               struct ospf_lsa *old, struct ospf_lsa *new)
{
  /* RFC 2328 Section 13.2 Summary-LSAs
     The best route to the destination described by the summary-
     LSA must be recalculated (see Section 16.5).  If this
     destination is an AS boundary router, it may also be
     necessary to re-examine all the AS-external-LSAs.  */
  if (!IS_LSA_SELF (new))
    {
      if (rt_recalc)
        ospf_ia_calc_incremental_event_add (new);
      else
        ospf_route_path_asbr_summary_lsa_set (top, new);
    }

  return new;
}

struct ospf_lsa *
ospf_external_lsa_install (struct ospf *top, int rt_recalc,
                           struct ospf_lsa *old, struct ospf_lsa *new)
{
  struct ls_table *table = top->redist_table;
  struct pal_in4_addr *omask;
  struct pal_in4_addr *nmask;
  u_char omasklen;
  u_char nmasklen;
  struct ospf_area *area;
  struct ls_node *rn;

#ifdef HAVE_OSPF_DB_OVERFLOW
  if (new->data->id.s_addr)
    {
      unsigned long count;

      count = ospf_lsdb_count_external_non_default (top);

      if (top->ext_lsdb_limit != OSPF_DEFAULT_LSDB_LIMIT)
        {
          if (count >= top->ext_lsdb_limit)
            ospf_db_overflow_enter (top);
#ifdef HAVE_SNMP
          else if (top->ext_lsdb_limit != 0)
            {
              double f, g;

              f = ((double) (count - 1) / (double)top->ext_lsdb_limit) * 10.0;
              g = ((double)count / (double)top->ext_lsdb_limit) * 10.0;

              if (f <= 9.0 && g > 9.0)
                ospfLsdbOverflow (top, OSPFLSDBAPPROACHINGOVERFLOW);
            }
#endif /* HAVE_SNMP */
        }
    }
#endif /* HAVE_OSPF_DB_OVERFLOW */

  /* RFC 2328 Section 13.2 AS-external-LSAs
     The best route to the destination described by the AS-
     external-LSA must be recalculated (see Section 16.6).  */
  if (!IS_LSA_SELF (new))
    {
      /* Refresh self originated equivalent LSA.  */
      ospf_external_lsa_equivalent_update_self (table, new);

      if (rt_recalc)
        {
          if (top->incr_defer)
            return new;

          if (top->tot_incr > 20)
            {
              for (rn = ls_table_top (top->area_table); rn;
                   rn = ls_route_next (rn))
                if ((area = RN_INFO (rn, RNI_DEFAULT))
                     && area->t_spf_calc != NULL)
                  {
                    top->incr_defer = PAL_TRUE;
                    /* Schedule the ASE incremental defer timer. */
                    OSPF_TOP_TIMER_ON (top->t_ase_inc_calc,
                     ospf_ase_incr_counter_rst, 5);
                  }
             }

          top->tot_incr++;
          ospf_ase_calc_incremental (top, new);

          if (old)
            {
              omask = OSPF_PNT_IN_ADDR_GET (old->data,
                                            OSPF_EXTERNAL_LSA_NETMASK_OFFSET);
              nmask = OSPF_PNT_IN_ADDR_GET (new->data,
                                            OSPF_EXTERNAL_LSA_NETMASK_OFFSET);
              omasklen = ip_masklen (*omask);
              nmasklen = ip_masklen (*nmask);

              if (omasklen != nmasklen)
                ospf_ase_calc_incremental (top, old);
            }
        }
      else
        ospf_route_path_external_lsa_set (top, new);
    }

  return new;
}

int
ospf_ase_incr_counter_rst (struct thread *thread)
{
  struct ospf *top = THREAD_ARG (thread);
  struct ospf_master *om = top->om;

  top->t_ase_inc_calc = NULL;

  top->tot_incr =0;
  top->incr_defer = PAL_FALSE;

  /* Set VR Context */
  LIB_GLOB_SET_VR_CONTEXT (ZG, om->vr);

  /* Schedule the ASE timer periodically. */
  OSPF_TOP_TIMER_ON (top->t_ase_inc_calc, ospf_ase_incr_counter_rst, 5);

  return 0;
}
#ifdef HAVE_NSSA

int
ospf_nssa_incr_counter_rst (struct thread *thread)
{
  struct ospf *top = THREAD_ARG (thread);
  struct ospf_master *om = top->om;

  top->t_nssa_inc_calc = NULL;

  /* Set VR Context */
  LIB_GLOB_SET_VR_CONTEXT (ZG, om->vr);

  top->tot_n_incr =0;
  top->incr_n_defer = PAL_FALSE;
  /* Schedule the NSSA periodic timer. */
  OSPF_TOP_TIMER_ON (top->t_nssa_inc_calc, ospf_nssa_incr_counter_rst, 5);

  return 0;
}

/* Install AS-external-LSA. */
struct ospf_lsa *
ospf_nssa_lsa_install (struct ospf *top, int rt_recalc,
                       struct ospf_lsa *old, struct ospf_lsa *new)
{
  struct ospf_area *area = new->lsdb->u.area;
  struct ls_table *table = area->redist_table;
  struct pal_in4_addr *omask;
  struct pal_in4_addr *nmask;
  u_char omasklen;
  u_char nmasklen;

  if (!IS_LSA_SELF (new))
    {
      /* Refresh self originated equivalent LSA.  */
      ospf_external_lsa_equivalent_update_self (table, new);

      if (rt_recalc)
        {
          if (top->incr_n_defer)
            return new;

          if (top->tot_n_incr > 20 && area->t_spf_calc != NULL)
            {
              top->incr_defer = PAL_TRUE;
              /* Schedule the NSSA incremental defer timer. */
              OSPF_TOP_TIMER_ON (top->t_nssa_inc_calc,
                                   ospf_nssa_incr_counter_rst, 5);
             }

          top->tot_n_incr++;
          ospf_ase_calc_incremental (top, new);

          if (old)
            {
              omask = OSPF_PNT_IN_ADDR_GET (old->data,
                                            OSPF_EXTERNAL_LSA_NETMASK_OFFSET);
              nmask = OSPF_PNT_IN_ADDR_GET (new->data,
                                            OSPF_EXTERNAL_LSA_NETMASK_OFFSET);
              omasklen = ip_masklen (*omask);
              nmasklen = ip_masklen (*nmask);

              if (omasklen != nmasklen)
                {
                  ospf_redist_map_nssa_delete (old);
                  ospf_ase_calc_incremental (top, old);
                }
            }
        }
      else
        ospf_route_path_external_lsa_set (top, new);
    }

  if (area->translator_state != OSPF_NSSA_TRANSLATOR_DISABLED)
    if (!ospf_redist_map_nssa_update (new))
      ospf_redist_map_nssa_delete (new);

  return new;
}
#endif /* HAVE_NSSA */

#ifdef HAVE_OPAQUE_LSA

struct ospf_lsa *
ospf_link_opaque_lsa_install (struct ospf *top, int rt_recalc,
                              struct ospf_lsa *old, struct ospf_lsa *new)
{
  switch (OPAQUE_TYPE (new->data->id))
    {
    case OSPF_LSA_OPAQUE_TYPE_GRACE:
#ifdef HAVE_RESTART
      if (top->restart_method == OSPF_RESTART_METHOD_GRACEFUL)
        ospf_restart_helper_enter_by_lsa (new);
#endif /* HAVE_RESTART */
      break;
    case OSPF_LSA_OPAQUE_TYPE_TE:
#ifdef HAVE_GMPLS
      if (!IS_LSA_SELF (new))
        {
          struct opaque_lsa *opaque_lsa = NULL;
          struct ospf_telink *os_tel = NULL;
          struct pal_in4_addr *id;
          struct telink *telink = NULL;
          struct avl_node *tl_node = NULL;
          struct nsm_msg_te_link msg;
          u_int32_t vr_id = 0;
          vrf_id_t vrf_id = 0;
          int length;

          length = (new->data->length - OSPF_HEADER_SIZE);          
          opaque_lsa = (struct opaque_lsa *)(new->data + OSPF_HEADER_SIZE);
          id = (struct pal_in4_addr *) opaque_lsa->opaque_data;

          /* Get TE-link corresponding to router advertised in LSA */
          AVL_TREE_LOOP (&top->om->zg->gmif->teltree, telink, tl_node)
            {
              if (telink->info == NULL)
                continue;

              if (!IPV4_ADDR_CMP(&telink->prop.linkid, &new->data->adv_router))
                {
                  os_tel = telink->info;
                  break;
                }
            }

          if (!telink)
            break;

          if (os_tel && os_tel->params && 
              CHECK_FLAG (os_tel->params->config, OSPF_TLINK_PARAM_TE_LINK_LOCAL) &&
              (! telink->r_linkid.linkid.ipv4.s_addr))
            {
              /* Set the Remote interface id for te-link*/
              pal_mem_cpy (&telink->r_linkid.linkid.ipv4, &id,
                             sizeof (struct pal_in4_addr));

              /* Set the remote-link-id type for te-link*/
              telink->r_linkid.type = GMPLS_LINK_ID_TYPE_IPV4;

              /* Update te-link */
              ospf_update_te_link (os_tel);

              /* Set the message structure */
              pal_mem_set (&msg, 0, sizeof (struct nsm_msg_te_link));

              /* Set the remote_linkid for msg */
              pal_strcpy (msg.name, telink->name);
              pal_mem_cpy (&msg.r_linkid, &id, sizeof (struct pal_in4_addr)); 
              NSM_SET_CTYPE (msg.cindex, NSM_TELINK_CTYPE_REMOTE_LINKID);

              nsm_client_send_te_link (vr_id, vrf_id, top->om->zg->nc, &msg);
            } 
        }
#endif /* HAVE_GMPLS */
      break;

    default:
      break;
    }

  return new;
}

struct ospf_lsa *
ospf_area_opaque_lsa_install (struct ospf *top, int rt_recalc,
                              struct ospf_lsa *old, struct ospf_lsa *new)
{
  return new;
}

struct ospf_lsa *
ospf_as_opaque_lsa_install (struct ospf *top, int rt_recalc,
                            struct ospf_lsa *old, struct ospf_lsa *new)
{
  return new;
}
#endif /* HAVE_OPAQUE_LSA */

#ifndef HAVE_NSSA
#define ospf_nssa_lsa_install   NULL
#define ospf_nssa_lsa_originate NULL
#define ospf_nssa_lsa_refresh   NULL
#endif /* HAVE_NSSA */

/* LSA callback func struct. */
struct
{
  struct ospf_lsa *(*install) (struct ospf *, int,
                               struct ospf_lsa *, struct ospf_lsa *);
  struct ospf_lsa *(*originate) (struct ospf *, void *);
  struct ospf_lsa *(*refresh) (struct ospf *, struct ospf_lsa *);
} ospf_lsa_func[] =
{
  /* Dummy. */
  {
    NULL,
    NULL,
    NULL,
  },
  /* Router-LSA. */
  {
    ospf_router_lsa_install,
    ospf_router_lsa_originate,
    ospf_router_lsa_refresh,
  },
  /* Network-LSA. */
  {
    ospf_network_lsa_install,
    ospf_network_lsa_originate,
    ospf_network_lsa_refresh,
  },
  /* Summary-LSA. */
  {
    ospf_summary_lsa_install,
    ospf_summary_lsa_originate,
    ospf_summary_lsa_refresh,
  },
  /* ASBR-summary-LSA. */
  {
    ospf_asbr_summary_lsa_install,
    ospf_asbr_summary_lsa_originate,
    ospf_asbr_summary_lsa_refresh,
  },
  /* AS-external-LSA. */
  {
    ospf_external_lsa_install,
    ospf_external_lsa_originate,
    ospf_external_lsa_refresh,
  },
  /* Group-membership-LSA. */
  {
    NULL,
    NULL,
    NULL,
  },
  /* NSSA-LSA. */
  {
    ospf_nssa_lsa_install,
    ospf_nssa_lsa_originate,
    ospf_nssa_lsa_refresh,
  },
#ifdef HAVE_OPAQUE_LSA
  /* */
  {
    NULL,
    NULL,
    NULL,
  },
  /* Opaque-Link LSA. */
  {
    ospf_link_opaque_lsa_install,
    ospf_opaque_lsa_originate,
    ospf_opaque_lsa_refresh,
  },
  /* Opaque-Area LSA. */
  {
    ospf_area_opaque_lsa_install,
    ospf_opaque_lsa_originate,
    ospf_opaque_lsa_refresh,
  },
  /* Opaque-AS LSA. */
  {
    ospf_as_opaque_lsa_install,
    ospf_opaque_lsa_originate,
    ospf_opaque_lsa_refresh,
  },
#endif /* HAVE_OPAQUE_LSA */
};

int
ospf_lsa_overflow_event (struct thread *thread)
{
  struct ospf *top = THREAD_ARG (thread);
  struct ospf_master *om = top->om;

  /* Set VR context*/
  LIB_GLOB_SET_VR_CONTEXT (ZG, om->vr);

  top->t_lsdb_overflow_event = NULL;
  ospf_proc_down (top);

  return 0;
}

/*  RFC 2328 13.2.  Installing LSAs in the database

    Installing a new LSA in the database, either as the result of
    flooding or a newly self-originated LSA, may cause the OSPF
    routing table structure to be recalculated.  The contents of the
    new LSA should be compared to the old instance, if present.  If
    there is no difference, there is no need to recalculate the
    routing table. When comparing an LSA to its previous instance,
    the following are all considered to be differences in contents:

        o   The LSA's Options field has changed.

        o   One of the LSA instances has LS age set to MaxAge, and
            the other does not.

        o   The length field in the LSA header has changed.

        o   The body of the LSA (i.e., anything outside the 20-byte
            LSA header) has changed. Note that this excludes changes
            in LS Sequence Number and LS Checksum. */
struct ospf_lsa *
ospf_lsa_install (struct ospf *top, struct ospf_lsa *lsa)
{
  struct ospf_master *om = top->om;
  char lsa_str[OSPF_LSA_STRING_MAXLEN];
  struct ospf_lsa *old;
  int rt_recalc = 0;
  u_int32_t ret = 0; 

  /* Calculate Checksum if self-originated? */
  if (IS_LSA_SELF (lsa))
    ospf_lsa_checksum (lsa->data);

  /* Look up old LSA and determine if any SPF calculation or incremental
     update is needed.  */
  old = ospf_lsdb_lookup (lsa->lsdb, lsa);

  /* If configured LSDB limit, check if it exceed limit. */
  if (old == NULL && CHECK_FLAG (top->config, OSPF_CONFIG_OVERFLOW_LSDB_LIMIT))
    {
      ret = ospf_proc_lsdb_count (top);
      if (ret >= top->lsdb_overflow_limit &&
          !CHECK_FLAG (top->flags, OSPF_LSDB_EXCEED_OVERFLOW_LIMIT)) 
        {
          SET_FLAG (top->flags, OSPF_LSDB_EXCEED_OVERFLOW_LIMIT);
          zlog_err (ZG, "Router ospf %d : has exceed lsdb limit",
                  top->ospf_id);        
          if (top->lsdb_overflow_limit_type == OSPF_LSDB_OVERFLOW_HARD_LIMIT)
            {
	      zlog_warn (ZG, "Manually restart the Ospf daemon");
              zlog_err (ZG, "Router ospf %d : will be shut down.\n", 
                        top->ospf_id);
              OSPF_EVENT_ON (top->t_lsdb_overflow_event, 
                             ospf_lsa_overflow_event, top);
            }
        }
    }

  /* Do comparision and record if recalc needed. */
  if (old == NULL || ospf_lsa_different (old, lsa))
    {
      rt_recalc = 1;
#ifdef HAVE_SNMP
      if (IS_LSA_SELF (lsa))
        ospfOriginateLsa (top, lsa, OSPFORIGINATELSA);
#endif /* HAVE_SNMP */
    }
  else
    SET_FLAG (lsa->flags, OSPF_LSA_REFRESHED);

  /* Discard old LSA from LSDB */
  if (old != NULL)
    {
      /* Lock for incremental update...  */
      ospf_lsa_lock (old);
      ospf_lsdb_lsa_discard (old->lsdb, old, 0, 1, 0);
    }

  /* Insert LSA to LSDB. */
  ospf_lsdb_add (lsa->lsdb, lsa);

  /* Do LSA specific installation process. */
  OSPF_LSA_INSTALL_BY_TYPE (top, rt_recalc, old, lsa);
  if (lsa != NULL)
    {
      if (IS_DEBUG_OSPF (lsa, LSA_INSTALL))
        zlog_info (ZG, "LSA[%s]: Install %s", LSA_NAME (lsa),
                   LOOKUP (ospf_lsa_type_msg, lsa->data->type));

      if (!IS_LSA_MAXAGE (lsa))
        if (IS_LSA_SELF (lsa))
          ospf_lsa_refresh_queue_set (lsa);
    }

  /* Unlock for incremental update...  */
  if (old != NULL)
    ospf_lsa_unlock (old);

  return lsa;
}


int
ospf_lsa_maxage_walker_ready (struct ospf *top)
{
  struct ospf_interface *oi;
  struct ospf_neighbor *nbr;
  struct ls_node *rn;

  for (rn = ls_table_top (top->nbr_table); rn; rn = ls_route_next (rn))
    if ((nbr = RN_INFO (rn, RNI_DEFAULT)))
      if ((oi = nbr->oi))
        if (CHECK_FLAG (oi->flags, OSPF_IF_UP))
          if (oi->type != OSPF_IFTYPE_LOOPBACK)
            if (nbr->state == NFSM_Exchange
                || nbr->state == NFSM_Loading)
              {
                ls_unlock_node (rn);
                return 0;
              }
  return 1;
}

void
ospf_lsa_maxage_apply (struct ospf *top, struct ospf_lsa *lsa)
{
  struct ospf_master *om = top->om;
  char lsa_str[OSPF_LSA_STRING_MAXLEN];

  if (IS_DEBUG_OSPF (lsa, LSA_MAXAGE))
    zlog_info (ZG, "LSA[%s]: Reached MaxAge, flush from LSDB", LSA_NAME (lsa));

  switch (lsa->data->type)
    {
    case OSPF_ROUTER_LSA:
    case OSPF_NETWORK_LSA:
      ospf_spf_calculate_timer_add (lsa->lsdb->u.area);
      break;
    case OSPF_SUMMARY_LSA:
      ospf_ia_calc_incremental_event_add (lsa);
      break;
    case OSPF_SUMMARY_LSA_ASBR:
      ospf_ia_calc_incremental_event_add (lsa);
      break;
    case OSPF_AS_EXTERNAL_LSA:
      ospf_ase_calc_incremental (top, lsa);
      break;
#ifdef HAVE_NSSA
    case OSPF_AS_NSSA_LSA:
      ospf_ase_calc_incremental (top, lsa);
      break;
#endif /* HAVE_NSSA */
#ifdef HAVE_OPAQUE_LSA
    case OSPF_LINK_OPAQUE_LSA:
#ifdef HAVE_RESTART
      if (top->restart_method == OSPF_RESTART_METHOD_GRACEFUL)
        if (OPAQUE_TYPE (lsa->data->id) == OSPF_LSA_OPAQUE_TYPE_GRACE)
          ospf_restart_helper_exit_by_lsa (lsa);
#endif /* HAVE_RESTART */
      break;
    case OSPF_AREA_OPAQUE_LSA:
      break;
    case OSPF_AS_OPAQUE_LSA:
      break;
#endif /* HAVE_OPAQUE_LSA */
    }
}

void
ospf_lsa_maxage_wrap_seqnum (struct ospf *top, struct ospf_lsa *lsa)
{
  SET_FLAG (lsa->flags, OSPF_LSA_SEQNUM_WRAPPED);

  switch (lsa->data->type)
    {
#ifdef HAVE_OPAQUE_LSA
    case OSPF_LINK_OPAQUE_LSA:
    case OSPF_AREA_OPAQUE_LSA:
    case OSPF_AS_OPAQUE_LSA:
      ospf_opaque_lsa_timer_add (lsa->lsdb);
      break;
#endif /* HAVE_OPAQUE_LSA  */
    default:
      LSA_REFRESH_TIMER_ADD (top, lsa->data->type, lsa, lsa->param);
      break;
    }
}

int
_ospf_lsa_maxage_walker (struct thread *thread)
{
  struct ospf_lsdb *lsdb = THREAD_ARG (thread);
  struct ospf_master *om;
  struct ospf_lsa *lsa;
  struct ospf *top;
  struct ls_node *rn;
  struct pal_timeval now;
  u_int16_t interval;
  int ready;
  int i, j;
  int count = 0;

  top = ospf_proc_lookup_by_lsdb (lsdb);
  if (top == NULL)
    return 0;

  om = top->om;

  /* Set VR context*/
  LIB_GLOB_SET_VR_CONTEXT (ZG, om->vr);
 
  /* Check none of neighbor is in Exchange nor Loading. */
  ready = ospf_lsa_maxage_walker_ready (top);
  if (ready)
    {
      pal_time_tzcurrent (&now, NULL);

      LSDB_LOOP_ALL_LSA (lsdb, i, rn, lsa)
        {
          if (count++ > OSPF_LSA_SCAN_COUNT_THRESHOLD)
            {
              if (IS_DEBUG_OSPF (event, EVENT))
                zlog_info (ZG, "LSDB MAXAGE WALKER suspended");

              OSPF_EVENT_SUSPEND_ON_LSDB (lsdb->t_lsa_walker, ospf_lsa_maxage_walker,
                                          lsdb, 0);
              lsdb->next_slot_type = i;
              lsdb->type[i].next_node = rn;	
              return 0;
            }
          if (IS_LSA_MAXAGE (lsa))
	    {
              if (TV_CMP (TV_SUB (now, lsa->tv_update), MinLSArrival) > 0)
                {
                  if (!IS_LSA_SELF (lsa))
                    ospf_lsa_maxage_apply (top, lsa);

                  if (lsa->rxmt_count == 0)
                    {
                      if (IS_LSA_SELF (lsa))
                        {
                          if (lsa->data->ls_seqnum == LSAMaxSeqNumber)
                            ospf_lsa_maxage_wrap_seqnum (top, lsa);
                        }
                      else
                        {
                          /* If it is naturally MaxAged. */
                          if (pal_ntoh16 (lsa->data->ls_age) < OSPF_LSA_MAXAGE)
                            ospf_flood_lsa_flush (top, lsa);
                        }

                      /* Remove from LSDB. */
                      if (!CHECK_FLAG (lsa->flags, OSPF_LSA_REFRESH_EVENT))
                        ospf_lsdb_lsa_discard (lsdb, lsa, 0, 1, 1);
                      else
                        {
                          lsdb->max_lsa_age = OSPF_LSA_MAXAGE - OSPF_LSA_MAXAGE_INTERVAL_DEFAULT; 
      	                  pal_time_tzcurrent (&lsdb->tv_maxage, NULL);
                        }
                    }
                  else
                    {
                      lsdb->max_lsa_age = OSPF_LSA_MAXAGE - OSPF_LSA_MAXAGE_INTERVAL_DEFAULT; 
      	              pal_time_tzcurrent (&lsdb->tv_maxage, NULL);
                    }
                }
              else
                {
                  lsdb->max_lsa_age = OSPF_LSA_MAXAGE - OSPF_LSA_MAXAGE_INTERVAL_DEFAULT; 
      	          pal_time_tzcurrent (&lsdb->tv_maxage, NULL);
                }
            }
	  else if (!lsdb->max_lsa_age || (LS_AGE (lsa) > (lsdb->max_lsa_age + TV_FLOOR (TV_SUB (now, lsdb->tv_maxage)))))
	    {
              lsdb->max_lsa_age = LS_AGE (lsa);
      	      pal_time_tzcurrent (&lsdb->tv_maxage, NULL);
	    }
        }
      /* Reset the suspend thread pointer.  */
      if (lsdb->t_suspend == thread)
        lsdb->t_suspend = NULL;

      /* Reset the next pointer.  */
      lsdb->next_slot_type = OSPF_MIN_LSA;
      for (j = OSPF_MIN_LSA; j < OSPF_MAX_LSA; j++)
        lsdb->type[j].next_node= NULL;
	
      /* Execute the pending event.  */
      OSPF_EVENT_PENDING_EXECUTE (lsdb->event_pend);

    }
  else
    {
      lsdb->max_lsa_age = OSPF_LSA_MAXAGE - OSPF_LSA_MAXAGE_INTERVAL_DEFAULT; 
      if (IS_DEBUG_OSPF (lsa, LSA_MAXAGE))
        zlog_info (ZG, "LSA[MaxAge]: Neighbor is in Exchange or Loading");
    }

  interval = OSPF_LSA_MAXAGE - lsdb->max_lsa_age;
  if (interval < OSPF_LSA_MAXAGE_INTERVAL_DEFAULT)
    interval = OSPF_LSA_MAXAGE_INTERVAL_DEFAULT;	

  if (ospf_lsdb_count_all (lsdb))
    if (!CHECK_FLAG (top->flags, OSPF_PROC_DESTROY))
      OSPF_TIMER_ON (lsdb->t_lsa_walker, ospf_lsa_maxage_walker,
                     lsdb, interval);

  return 0;
}

int
ospf_lsa_maxage_walker (struct thread *thread)
{
  struct ospf_lsdb *lsdb = THREAD_ARG (thread);
  struct thread *thread_self = lsdb->t_lsa_walker ? lsdb->t_lsa_walker : thread;
  struct ospf *top = ospf_proc_lookup_by_lsdb (lsdb);
  struct ospf_master *om = NULL;

  if (top == NULL)
    return 0;

  om = top->om;

  if (om == NULL)
    return 0;

  lsdb->t_lsa_walker = NULL;
  lsdb->max_lsa_age = 0;

  /* Set VR context*/
  LIB_GLOB_SET_VR_CONTEXT (ZG, om->vr);

  /* Wait for the other suspended thread.  */
  if (IS_LSDB_SUSPENDED (lsdb, thread_self))
    {
      OSPF_EVENT_PENDING_ON (lsdb->event_pend,
                             ospf_lsa_maxage_walker, lsdb, 0);
      return 0;
    }

  return ospf_thread_wrapper (thread_self, _ospf_lsa_maxage_walker,
                              "LSA[MaxAge]: Maxage walker",
                              IS_DEBUG_OSPF (lsa, LSA_MAXAGE));
}


struct ospf_lsa *
ospf_lsa_lookup (struct ospf_interface *oi, u_int32_t type,
                 struct pal_in4_addr id, struct pal_in4_addr adv_router)
{
  switch (LSA_FLOOD_SCOPE (type))
    {
#ifdef HAVE_OPAQUE_LSA
    case LSA_FLOOD_SCOPE_LINK:
      return ospf_lsdb_lookup_by_id (oi->lsdb, type, id, adv_router);
      break;
#endif /* HAVE_OPAQUE_LSA */
    case LSA_FLOOD_SCOPE_AREA:
      return ospf_lsdb_lookup_by_id (oi->area->lsdb, type, id, adv_router);
      break;
    case LSA_FLOOD_SCOPE_AS:
      return ospf_lsdb_lookup_by_id (oi->top->lsdb, type, id, adv_router);
      break;
    default:
      pal_assert (0);
      break;
    }

  return NULL;
}

struct ospf_lsa *
ospf_network_lsa_lookup_by_addr (struct ospf_area *area,
                                 struct pal_in4_addr addr)
{
  struct ospf_lsdb_slot *slot = NETWORK_LSDB (area);
  struct ls_node *rn, *rn_tmp;
  struct ls_prefix_ls lp;

  pal_mem_set (&lp, 0, sizeof (struct ls_prefix_ls));
  lp.prefixsize = OSPF_LSDB_LOWER_TABLE_DEPTH;
  lp.prefixlen = 32;
  lp.id = addr;

  rn_tmp = ls_node_get (slot->db, (struct ls_prefix *)&lp);
  for (rn = ls_lock_node (rn_tmp); rn; rn = ls_route_next_until (rn, rn_tmp))
    if (RN_INFO (rn, RNI_DEFAULT))
      {
        ls_unlock_node (rn);
        ls_unlock_node (rn_tmp);
        return RN_INFO (rn, RNI_DEFAULT);
      }
  ls_unlock_node (rn_tmp);

  return NULL;
}


struct ospf_lsa *
ospf_lsa_originate (struct ospf *top, u_char type, void *param)
{
  struct ospf_master *om = top->om;
  char lsa_str[OSPF_LSA_STRING_MAXLEN];
  struct ospf_lsa *new = NULL;
  struct ospf_lsa *old;

#ifdef HAVE_RESTART
  /* Process is in "Graceful Restart. */
  if (CHECK_FLAG (top->flags, OSPF_ROUTER_RESTART))
    if (type == OSPF_ROUTER_LSA
        || type == OSPF_NETWORK_LSA
        || type == OSPF_SUMMARY_LSA
        || type == OSPF_SUMMARY_LSA_ASBR
        || type == OSPF_AS_EXTERNAL_LSA
#ifdef HAVE_NSSA
        || type == OSPF_AS_NSSA_LSA
#endif /* HAVE_NSSA */
        )
      return NULL;
#endif /* HAVE_RESTART */

  if (!IS_PROC_ACTIVE (top))
    return NULL;

  OSPF_LSA_ORIGINATE_BY_TYPE (type, top, param, new);
  if (new == NULL)
    {
      if (IS_DEBUG_OSPF (lsa, LSA_GENERATE))
        zlog_warn (ZG, "LSA[Type%d]: could not originate", type);
      return NULL;
    }

  pal_time_tzcurrent (&new->tv_update, NULL);
  top->lsa_originate_count++;

  old = ospf_lsdb_lookup (new->lsdb, new);
  if (old != NULL)
    new->data->ls_seqnum = ospf_lsa_seqnum_increment (old);

  ospf_lsa_install (top, new);
  ospf_flood_through (NULL, new);

  if (IS_DEBUG_OSPF (lsa, LSA_GENERATE))
    zlog_info (ZG, "LSA[%s]: %s(0x%x) originated", LSA_NAME (new),
               LOOKUP (ospf_lsa_type_msg, new->data->type), new);

  return new;
}

int
ospf_lsa_originate_event (struct thread *thread)
{
  struct ospf *top = THREAD_ARG (thread);
  struct ospf_lsa *lsa;
  int i;
  struct ospf_master *om = top->om;

  top->t_lsa_originate = NULL;

  /* Set VR context*/
  LIB_GLOB_SET_VR_CONTEXT (ZG, om->vr);

  for (i = 0; i < vector_max (top->lsa_originate); i++)
    if ((lsa = vector_slot (top->lsa_originate, i)))
      {
        if (!IS_LSA_MAXAGE (lsa))
          ospf_lsa_originate (top, lsa->data->type, lsa->param);

        UNSET_FLAG (lsa->flags, OSPF_LSA_ORIGINATE_EVENT);
        vector_slot (top->lsa_originate, i) = NULL;
        ospf_lsa_unlock (lsa);
      }

  return 0;
}

void
ospf_lsa_originate_event_add (struct ospf *top, struct ospf_lsa *lsa)
{
  if (!IS_PROC_ACTIVE (top))
    return;

  if (CHECK_FLAG (lsa->flags, OSPF_LSA_ORIGINATE_EVENT))
    return;

  /* Set the LSA to the originate vector.  */
  SET_FLAG (lsa->flags, OSPF_LSA_ORIGINATE_EVENT);
  vector_set (top->lsa_originate, ospf_lsa_lock (lsa));

  /* Register the LSA originate event.  */
  OSPF_EVENT_ON (top->t_lsa_originate, ospf_lsa_originate_event, top);
}

void
ospf_lsa_originate_event_unset (struct ospf_lsa *lsa)
{
  struct ospf *top;
  int i;

  if (!CHECK_FLAG (lsa->flags, OSPF_LSA_ORIGINATE_EVENT))
    return;

  UNSET_FLAG (lsa->flags, OSPF_LSA_ORIGINATE_EVENT);
  if (lsa->lsdb)
    {
      top = ospf_proc_lookup_by_lsdb (lsa->lsdb);
      pal_assert (top != NULL);

      for (i = 0; i < vector_max (top->lsa_originate); i++)
        if ((vector_slot (top->lsa_originate, i)) == lsa)
          {
            vector_slot (top->lsa_originate, i) = NULL;
            ospf_lsa_unlock (lsa);
            return;
          }
    }
}


struct ospf_lsa *
ospf_lsa_refresh (struct ospf *top, struct ospf_lsa *lsa)
{
  struct ospf_master *om = top->om;
  char lsa_str[OSPF_LSA_STRING_MAXLEN];
  u_char type = lsa->data->type;
  struct ospf_lsa *new = NULL;

  if (!IS_PROC_ACTIVE (top))
    return NULL;

  ospf_ls_retransmit_delete_nbr_by_scope (lsa, PAL_FALSE);

  if (lsa->data->ls_seqnum == LSAMaxSeqNumber
      && !CHECK_FLAG (lsa->flags, OSPF_LSA_SEQNUM_WRAPPED))
    {
      ospf_flood_lsa_flush (top, lsa);
      return lsa;
    }

  OSPF_LSA_REFRESH_BY_TYPE (type, top, lsa, new);
  if (new == NULL)
    {
      if (IS_DEBUG_OSPF (lsa, LSA_REFRESH))
        zlog_info (ZG, "LSA[Type%d]: could not refresh", type);
      return NULL;
    }

  pal_time_tzcurrent (&new->tv_update, NULL);
  if (CHECK_FLAG (lsa->flags, OSPF_LSA_SEQNUM_WRAPPED))
    {
      UNSET_FLAG (lsa->flags, OSPF_LSA_SEQNUM_WRAPPED);
      new->data->ls_seqnum = pal_hton32 (OSPF_INITIAL_SEQUENCE_NUMBER);
    }
  else
    new->data->ls_seqnum = ospf_lsa_seqnum_increment (lsa);

  ospf_lsa_install (top, new);
  ospf_flood_through (NULL, new);

  if (IS_DEBUG_OSPF (lsa, LSA_REFRESH))
    zlog_info (ZG, "LSA[%s]: %s refreshed", LSA_NAME (new),
               LOOKUP (ospf_lsa_type_msg, new->data->type));

  if (IS_DEBUG_OSPF (lsa, LSA_GENERATE))
    ospf_lsa_header_dump (new->data);

  return new;
}

int
ospf_lsa_refresh_event_timer (struct thread *thread)
{
  struct ospf *top = THREAD_ARG (thread);
  struct ospf_lsa *lsa;
  struct pal_timeval delta, now;
  int i;
  struct ospf_master *om = top->om;

  top->t_lsa_event = NULL;

  /* Set VR context*/
  LIB_GLOB_SET_VR_CONTEXT (ZG, om->vr);

  pal_time_tzcurrent (&now, NULL);

  for (i = 0; i < vector_max (top->lsa_event); i++)
    if ((lsa = vector_slot (top->lsa_event, i)))
      {
        delta = TV_SUB (now, lsa->tv_update);
        if (TV_CMP (delta, MinLSInterval) > 0)
          {
            UNSET_FLAG (lsa->flags, OSPF_LSA_REFRESH_EVENT);
            vector_slot (top->lsa_event, i) = NULL;
            ospf_lsa_unlock (lsa);
            ospf_lsa_refresh (top, lsa);
          }
      }

  if (vector_count (top->lsa_event))
    OSPF_TV_TIMER_ON (top->t_lsa_event, ospf_lsa_refresh_event_timer,
                      top, OSPFLSARefreshEventInterval);

  return 0;
}

void
ospf_lsa_refresh_timer_add (struct ospf_lsa *lsa)
{
  struct ospf *top;
  struct pal_timeval delta, now;

  if (CHECK_FLAG (lsa->flags, OSPF_LSA_DISCARD))
    return;

  if (CHECK_FLAG (lsa->flags, OSPF_LSA_REFRESH_EVENT))
    return;

  top = ospf_proc_lookup_by_lsdb (lsa->lsdb);
  pal_assert (top != NULL);

  pal_time_tzcurrent (&now, NULL);
  delta = TV_SUB (now, lsa->tv_update);

  SET_FLAG (lsa->flags, OSPF_LSA_REFRESH_EVENT);
  vector_set (top->lsa_event, ospf_lsa_lock (lsa));

  if (TV_CMP (delta, MinLSInterval) < 0)
    {
      OSPF_TV_TIMER_ON (top->t_lsa_event, ospf_lsa_refresh_event_timer,
                        top, OSPFLSARefreshEventInterval);
    }
  else
    {
      if (top->t_lsa_event && top->t_lsa_event->type == THREAD_TIMER)
        OSPF_TIMER_OFF (top->t_lsa_event);

      OSPF_EVENT_ON (top->t_lsa_event, ospf_lsa_refresh_event_timer, top);
    }
}

void
ospf_lsa_refresh_event_unset (struct ospf_lsa *lsa)
{
  struct ospf *top;
  int i;

  if (!CHECK_FLAG (lsa->flags, OSPF_LSA_REFRESH_EVENT))
    return;

  UNSET_FLAG (lsa->flags, OSPF_LSA_REFRESH_EVENT);
  if (lsa->lsdb)
    {
      top = ospf_proc_lookup_by_lsdb (lsa->lsdb);
      pal_assert (top != NULL);

      for (i = 0; i < vector_max (top->lsa_event); i++)
        if ((vector_slot (top->lsa_event, i)) == lsa)
          {
            vector_slot (top->lsa_event, i) = NULL;
            ospf_lsa_unlock (lsa);
            return;
          }
    }
}


#define LSA_REFRESH_QUEUE_ADD(O,R,L)                                          \
    do {                                                                      \
      vector _vec;                                                            \
      int _column;                                                            \
      _vec = vector_slot ((O)->lsa_refresh.vec, (R));                         \
      if (_vec == NULL)                                                       \
        {                                                                     \
          _vec = vector_init (OSPF_VECTOR_SIZE_MIN);                          \
          vector_set_index ((O)->lsa_refresh.vec, (R), _vec);                 \
        }                                                                     \
      _column = vector_set (_vec, ospf_lsa_lock ((L)));                       \
      LSA_REFRESH_INDEX_SET ((L), (R), _column);                              \
    } while (0)

int
ospf_lsa_refresh_walker (struct thread *thread)
{
  struct ospf *top = THREAD_ARG (thread);
  struct ospf_master *om = top->om;
  struct ospf_lsa *lsa;
  vector vec;
  int i;

  top->t_lsa_walker = NULL;

  /* Set VR context*/
  LIB_GLOB_SET_VR_CONTEXT (ZG, om->vr);

  if (IS_DEBUG_OSPF (lsa, LSA))
    zlog_info (ZG, "LSA[Refresh]: timer expired");

  vec = vector_slot (top->lsa_refresh.vec, top->lsa_refresh.index);
  if (vec != NULL)
    {
      for (i = 0; i < vector_max (vec); i++)
        if ((lsa = vector_slot (vec, i)))
          {
            UNSET_FLAG (lsa->flags, OSPF_LSA_REFRESH_QUEUED);
            ospf_lsa_unlock (lsa);
            ospf_lsa_refresh (top, lsa);
          }
      vector_free (vec);
      vector_slot (top->lsa_refresh.vec, top->lsa_refresh.index) = NULL;
    }

  top->lsa_refresh.index =
    (top->lsa_refresh.index + 1) % top->lsa_refresh.rows;

  OSPF_TOP_TIMER_ON (top->t_lsa_walker, ospf_lsa_refresh_walker,
                     top->lsa_refresh.interval);

  return 0;
}

void
ospf_lsa_refresh_init (struct ospf *top)
{
  struct ospf_master *om = top->om;

  OSPF_TIMER_OFF (top->t_lsa_walker);

  /* Re-calculate rows. */
  top->lsa_refresh.rows =
    (OSPF_LS_REFRESH_TIME + OSPF_LS_REFRESH_INITIAL_JITTER)
    / top->lsa_refresh.interval + 1;

  if (IS_DEBUG_OSPF (lsa, LSA))
    zlog_info (ZG, "LSA[Refresh]: Init Rows %d, Interval %d sec",
               top->lsa_refresh.rows, top->lsa_refresh.interval);

  top->lsa_refresh.index = 0;
  if (top->lsa_refresh.vec == NULL)
    top->lsa_refresh.vec = vector_init (top->lsa_refresh.rows);

  OSPF_TOP_TIMER_ON (top->t_lsa_walker, ospf_lsa_refresh_walker,
                     top->lsa_refresh.interval);
}

void
ospf_lsa_refresh_clear (struct ospf *top)
{
  struct ospf_lsa *lsa;
  vector vec;
  int i, j;

  for (i = 0; i < vector_max (top->lsa_refresh.vec); i++)
    if ((vec = vector_slot (top->lsa_refresh.vec, i)))
      {
        for (j = 0; j < vector_max (vec); j++)
          if ((lsa = vector_slot (vec, j)))
            ospf_lsa_unlock (lsa);
        vector_free (vec);
        vector_slot (top->lsa_refresh.vec, i) = NULL;
      }
}

void
ospf_lsa_refresh_restart (struct ospf *top)
{
  struct ospf_lsa *lsa;
  int row, column;
  vector vec, old;

  if (!IS_PROC_ACTIVE (top))
    return;

  /* Keep old vector. */
  old = top->lsa_refresh.vec;
  top->lsa_refresh.vec = NULL;

  /* Init struct and timer. */
  ospf_lsa_refresh_init (top);

  /* Move all LSAs from old vector to new vector. */
  for (row = 0; row < vector_max (old); row++)
    if ((vec = vector_slot (old, row)))
      {
        for (column = 0; column < vector_max (vec); column++)
          if ((lsa = vector_slot (vec, column)))
            {
              int rw;
              rw = lsa->refresh_age / top->lsa_refresh.interval;
              LSA_REFRESH_QUEUE_ADD (top, rw, lsa);
              ospf_lsa_unlock (lsa);
            }
        vector_free (vec);
      }

  vector_free (old);
}

void
ospf_lsa_refresh_queue_set (struct ospf_lsa *lsa)
{
  struct ospf *top;
  struct ospf_master *om;
  int jitter, age;
  int row;
  char lsa_str[OSPF_LSA_STRING_MAXLEN];

  if (lsa->param == NULL)
    return;

  top = ospf_proc_lookup_by_lsdb (lsa->lsdb);
  if (top == NULL)
    return;

  om = top->om;

  if (CHECK_FLAG (lsa->flags, OSPF_LSA_DISCARD))
    pal_assert (0);

  if (CHECK_FLAG (lsa->flags, OSPF_LSA_REFRESH_QUEUED))
    return;

  /* Randomize next refresh timer with jitter. */
  if (pal_ntoh32 (lsa->data->ls_seqnum) == OSPF_INITIAL_SEQUENCE_NUMBER)
    jitter = OSPF_LS_REFRESH_INITIAL_JITTER;
  else
    jitter = OSPF_LS_REFRESH_JITTER;

  /* Calculate age to refresh. */
  age = OSPF_LS_REFRESH_TIME + (pal_rand () % (2 * jitter)) - jitter;

  if (IS_DEBUG_OSPF (lsa, LSA))
    zlog_info (ZG, "LSA[%s]: LSA refresh scheduled at LS age %d",
               LSA_NAME (lsa), age);

  row = (top->lsa_refresh.index + (age / top->lsa_refresh.interval))
    % top->lsa_refresh.rows;

  LSA_REFRESH_QUEUE_ADD (top, row, lsa);

  lsa->refresh_age = age;
  SET_FLAG (lsa->flags, OSPF_LSA_REFRESH_QUEUED);
}

void
ospf_lsa_refresh_queue_unset (struct ospf_lsa *lsa)
{
  struct ospf *top;
  vector vec;
  int row, column;

  top = ospf_proc_lookup_by_lsdb (lsa->lsdb);
  if (top == NULL)
    return;

  if (!CHECK_FLAG (lsa->flags, OSPF_LSA_REFRESH_QUEUED))
    return;

  row = LSA_REFRESH_INDEX_ROW (lsa);
  column = LSA_REFRESH_INDEX_COLUMN (lsa);

  vec = vector_slot (top->lsa_refresh.vec, row);
  if (vec != NULL)
    {
      if (vector_slot (vec, column))
        if ((vector_slot (vec, column)) == lsa)
          {
            ospf_lsa_unlock (lsa);
            vector_unset (vec, column);
          }

      if (vector_count (vec) == 0)
        {
          vector_free (vec);
          vector_slot (top->lsa_refresh.vec, row) = NULL;
        }
    }

  UNSET_FLAG (lsa->flags, OSPF_LSA_REFRESH_QUEUED);
}
