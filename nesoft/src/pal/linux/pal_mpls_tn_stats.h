/* Copyright (C) 2002-2004  All Rights Reserved. */

#ifndef _PAL_MPLS_TN_STATS_H
#define _PAL_MPLS_TN_STATS_H

#ifdef HAVE_MPLS_FWD

struct tunnel_entry_stats
{
  u_int32_t tx_bytes;
  u_int32_t tx_pkts;
  u_int32_t error_pkts;
};

typedef struct tunnel_entry_stats pal_tunnel_entry_stats_t;

/* Include prototypes. */
#include "pal_mpls_tn_stats.def"

#endif /* HAVE_MPLS_FWD */

#endif /* _PAL_MPLS_TN_STATS_H */
