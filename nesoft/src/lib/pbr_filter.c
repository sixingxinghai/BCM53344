/* Copyright (C) 2009-2010  All Rights Reserved. */

#include "pal.h"

#include "lib.h"
#include "cli.h"

#include "snprintf.h"
#include "vty.h"
#include "ls_prefix.h"

#include "pbr_filter.h"

#ifdef HAVE_PBR


/* Copy the old nexthop value */
struct pbr_nexthop *
pbr_nexthop_copy (struct pbr_nexthop *pbr_nexthop,
                  struct pbr_nexthop *pbr_nexthop_old, s_int32_t nh_type)
{
  if (pbr_nexthop_old)
    {
      pbr_nexthop = XCALLOC (MTYPE_PBR_NEXTHOP,
                             sizeof (struct pbr_nexthop));
      pbr_nexthop->nexthop_str = XSTRDUP (MTYPE_PBR_STRING,
                                     pbr_nexthop_old->nexthop_str);
      pbr_nexthop->nh_type = nh_type;
      if (pbr_nexthop_old->ifname)
        pbr_nexthop->ifname = XSTRDUP (MTYPE_PBR_STRING,
                                  pbr_nexthop_old->ifname);
      return pbr_nexthop;
    }
  return NULL;
}

/* Free the nexthop address and interface name */
void
pbr_nexthop_free (struct pbr_nexthop *pbr_nexthop)
{
  if (pbr_nexthop)
    {
      XFREE (MTYPE_PBR_STRING, pbr_nexthop->nexthop_str);
      if (pbr_nexthop->ifname)
        XFREE (MTYPE_PBR_STRING, pbr_nexthop->ifname);
      XFREE (MTYPE_PBR_NEXTHOP, pbr_nexthop);
    }
}

/* Copy the common filter values into pacos extended filter */
void 
pbr_filter_copy_common (struct filter_pacos_ext *acl_filter, 
                        struct filter_common *cfilter)
{
  struct pal_in4_addr mask;
  struct pal_in4_addr addr;

  mask.s_addr = ~(cfilter->addr_mask.s_addr);
  addr.s_addr = (cfilter->addr.s_addr &  (~cfilter->addr_mask.s_addr));
  acl_filter->sprefix.prefixlen =  ip_masklen (mask);
  IPV4_ADDR_COPY (&acl_filter->sprefix.u.prefix4, &addr);
  if (cfilter->extended)
    {
       mask.s_addr = ~(cfilter->mask_mask.s_addr);
       acl_filter->dprefix.prefixlen = ip_masklen (mask);
       addr.s_addr = (cfilter->mask.s_addr &  (~cfilter->mask_mask.s_addr));
       IPV4_ADDR_COPY (&acl_filter->dprefix.u.prefix4, &addr);
    }
}

#endif /* HAVE_PBR */
