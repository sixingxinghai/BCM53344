/* Copyright (C) 2002-2004  All Rights Reserved. */

#ifndef _PAL_MPLS_STATS_H
#define _PAL_MPLS_STATS_H

#ifdef HAVE_MPLS_FWD

struct ftn_entry_stats
{
  u_int32_t tx_bytes;
  u_int32_t pushed_pkts;
  u_int32_t matched_bytes;
  u_int32_t matched_pkts;
};

struct ilm_entry_stats
{
  u_int32_t rx_bytes;
  u_int32_t tx_bytes;
  u_int32_t rx_pkts;
  u_int32_t swapped_pkts;
  u_int32_t popped_pkts;
  u_int32_t error_pkts;
  u_int32_t discard_pkts;
};

struct nhlfe_entry_stats
{
  u_int32_t tx_bytes;
  u_int32_t tx_pkts;
  u_int32_t error_pkts;
  u_int32_t discard_pkts;
};

struct mpls_if_entry_stats
{
  u_int32_t rx_pkts;
  u_int32_t dropped_pkts;
  u_int32_t ip_tx_pkts;
  u_int32_t labeled_tx_pkts;
  u_int32_t dropped_labeled_pkts;
  u_int32_t switched_pkts;
  u_int32_t in_labels_used;
  u_int32_t label_lookup_failures;
  u_int32_t out_labels_used;
  u_int32_t out_fragments;
};

struct fib_stats
{
  u_int32_t dropped_pkts;
  u_int32_t ip_tx_pkts;
  u_int32_t labeled_tx_pkts;
  u_int32_t dropped_labeled_pkts;
  u_int32_t switched_pkts;
};

typedef struct ftn_entry_stats pal_ftn_entry_stats_t;
typedef struct ilm_entry_stats pal_ilm_entry_stats_t;
typedef struct nhlfe_entry_stats pal_nhlfe_entry_stats_t;
typedef struct mpls_if_entry_stats pal_mpls_if_entry_stats_t;
typedef struct fib_stats pal_fib_stats_t;

/* Include prototypes. */
#include "pal_mpls_stats.def"

#endif /* HAVE_MPLS_FWD */

#endif /* _PAL_MPLS_STATS_H */
