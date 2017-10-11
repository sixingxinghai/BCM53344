/* Copyright (C) 2001-2004  All Rights Reserved.  */

#include "pal.h"
#include "pal_mpls_stats.h"

#ifdef HAVE_MPLS_FWD

#ifdef HAVE_DIFFSERV
#include "diffserv.h"
#endif /* HAVE_DIFFSERV */

#include "nsmd.h"
#include "nsm_mpls.h"
#ifdef HAVE_MPLS_VC
#include "nsm_mpls_vc.h"
#endif /* HAVE_MPLS_VC */
#include "nsm_mpls_rib.h"

#ifdef HAVE_PROC_NET_DEV
#ifndef _PATH_PROC_NET_MPLS_STATS
#define _PATH_PROC_NET_MPLS_STATS     "/proc/net/mpls_stats"
#endif
#ifndef _PATH_PROC_NET_MPLS_IF_STATS
#define _PATH_PROC_NET_MPLS_IF_STATS  "/proc/net/mpls_if_stats"
#endif
#ifndef _PATH_PROC_NET_MPLS_FTN_STATS
#define _PATH_PROC_NET_MPLS_FTN_STATS "/proc/net/mpls_ftn_stats"
#endif
#ifndef _PATH_PROC_NET_MPLS_ILM_STATS
#define _PATH_PROC_NET_MPLS_ILM_STATS "/proc/net/mpls_ilm_stats"
#endif
#ifndef _PATH_PROC_NET_MPLS_NHLFE_STATS
#define _PATH_PROC_NET_MPLS_NHLFE_STATS "/proc/net/mpls_nhlfe_stats"
#endif
#ifndef PROCBUFSIZ
#define PROCBUFSIZ                    1024
#endif
#endif /* HAVE_PROC_NET_DEV */

/* GET top level mpls stats. */
void
pal_mpls_fib_stats (pal_fib_stats_t *stats, char * buf, int len)
{
  buf[0] = '\0';

  snprintf(buf, len,
           " Packets dropped IP:%u, dropped MPLS:%u sent to IP:%u, labeled:%u, switched:%u",
           stats->dropped_pkts,
           stats->dropped_labeled_pkts,
           stats->ip_tx_pkts,
           stats->labeled_tx_pkts,
           stats->switched_pkts);
}

/* GET Interface level mpls stats. */
void
pal_mpls_if_entry_stats (pal_mpls_if_entry_stats_t *stats, char * buf, int len)
{
  buf[0] = '\0';

  snprintf(buf, len,
           "  RX pkts:%u, dropped IP:%u, dropped MPLS:%u, failed label lookup:%u\n"
           "  Sent to IP:%u, labeled:%u, switched:%u, in-labels used:%u\n"
           "  TX out fragments:%u, out-labels used:%u\n",
           stats->rx_pkts,
           stats->dropped_pkts,
           stats->dropped_labeled_pkts,
           stats->label_lookup_failures,
           stats->ip_tx_pkts,
           stats->labeled_tx_pkts,
           stats->switched_pkts,
           stats->in_labels_used,
           stats->out_fragments,
           stats->out_labels_used);
}

/* Get FTN entry stats. */
void
pal_mpls_ftn_entry_stats (pal_ftn_entry_stats_t *stats, char *buf, int len)
{
  buf[0] = '\0';

  snprintf(buf, len,
           "    Matched bytes:%u, pkts:%u, TX bytes:%u, Pushed pkts:%u",
           stats->matched_bytes,
           stats->matched_pkts,
           stats->tx_bytes,
           stats->pushed_pkts);
}

/* Get ILM entry stats. */
void
pal_mpls_ilm_entry_stats (pal_ilm_entry_stats_t *stats, char * buf, int len)
{
  buf[0] = '\0';

  snprintf(buf, len,
           "    RX bytes:%u, pkts:%u, TX bytes:%u, Swapped pkts:%u, Popped pkts:%u",
           stats->rx_bytes,
           stats->rx_pkts,
           stats->tx_bytes,
           stats->swapped_pkts,
           stats->popped_pkts);
}

/* Get NHLFE entry stats. */
void
pal_mpls_nhlfe_entry_stats (pal_nhlfe_entry_stats_t *stats, char * buf, int len)
{
  buf[0] = '\0';

  snprintf(buf, len,
           "    TX bytes:%u, pkts:%u, error pkts:%u, discard pkts:%u",
           stats->tx_bytes,
           stats->tx_pkts,
           stats->error_pkts,
           stats->discard_pkts);
}

/* Read top level stats and store them in MPLS object. */
void
pal_mpls_fib_stats_update (struct lib_globals *zg, pal_fib_stats_t *stats)
{
  FILE *fp;
  char buf[PROCBUFSIZ];
  char *ptr, *delim;

  /* Open /proc/net/dev. */
  fp = fopen (_PATH_PROC_NET_MPLS_STATS, "r");
  if (fp == NULL)
    {
      zlog_warn (zg, "Unable to open proc file %s: %s",
                 _PATH_PROC_NET_MPLS_STATS, strerror (errno));
      return;
    }

  /* Incoming IP packets labeled. */
  ptr = fgets (buf, PROCBUFSIZ, fp);
  if (ptr)
    {
      ptr = strtok_r (buf, ":", &delim);
      ptr = strtok_r (NULL, ":", &delim);
      sscanf (ptr, "%u", &stats->labeled_tx_pkts);
    }

  /* Packets sent to IP. */
  ptr = fgets (buf, PROCBUFSIZ, fp);
  if (ptr)
    {
      ptr = strtok_r (buf, ":", &delim);
      ptr = strtok_r (NULL, ":", &delim);
      sscanf (ptr, "%u", &stats->ip_tx_pkts);
    }

  /* Packets switched. */
  ptr = fgets (buf, PROCBUFSIZ, fp);
  if (ptr)
    {
      ptr = strtok_r (buf, ":", &delim);
      ptr = strtok_r (NULL, ":", &delim);
      sscanf (ptr, "%u", &stats->switched_pkts);
    }

  /* Dropped packets. */
  ptr = fgets (buf, PROCBUFSIZ, fp);
  if (ptr)
    {
      ptr = strtok_r (buf, ":", &delim);
      ptr = strtok_r (NULL, ":", &delim);
      sscanf (ptr, "%u", &stats->dropped_pkts);
    }

  /* Dropped labeled packets. */
  ptr = fgets (buf, PROCBUFSIZ, fp);
  if (ptr)
    {
      ptr = strtok_r (buf, ":", &delim);
      ptr = strtok_r (NULL, ":", &delim);
      sscanf (ptr, "%u", &stats->dropped_labeled_pkts);
    }
  fclose (fp);
}

/* Read all MPLS Interface stats. */
void pal_mpls_if_stats_update (struct nsm_master *nm)
{
  FILE *fp;
  char buf[PROCBUFSIZ];
  u_int32_t ifindex;
  u_int32_t rx_pkts;
  u_int32_t ip_tx_pkts;
  u_int32_t labeled_tx_pkts;
  u_int32_t switched_pkts;
  u_int32_t dropped_pkts;
  u_int32_t dropped_labeled_pkts;
  u_int32_t label_lookup_failures;
  u_int32_t out_fragments;
  struct interface *ifp;
  struct nsm_mpls_if *mif;

  /* Open /proc/net/dev. */
  fp = fopen (_PATH_PROC_NET_MPLS_IF_STATS, "r");
  if (fp == NULL)
    {
      zlog_warn (nm->zg, "Unable to open proc file %s: %s",
                 _PATH_PROC_NET_MPLS_IF_STATS, strerror (errno));
      return;
    }

  /* Ignore header. */
  fgets (buf, PROCBUFSIZ, fp);

  /* Read all data. */
  while (fgets (buf, PROCBUFSIZ, fp) != NULL)
    {
      sscanf (buf, "%u %u %u %u %u %u %u %u %u",
              &ifindex,
              &rx_pkts,
              &dropped_pkts,
              &ip_tx_pkts,
              &labeled_tx_pkts,
              &dropped_labeled_pkts,
              &switched_pkts,
              &label_lookup_failures,
              &out_fragments);

      /* Lookup correct MPLS Interface entry. */
      if (ifindex == 0)
        {
          nm->nmpls->if_stats.rx_pkts = rx_pkts;

          nm->nmpls->if_stats.dropped_pkts = dropped_pkts;
          nm->nmpls->if_stats.ip_tx_pkts = ip_tx_pkts;
          nm->nmpls->if_stats.labeled_tx_pkts = labeled_tx_pkts;

          nm->nmpls->if_stats.dropped_labeled_pkts = dropped_labeled_pkts;
          nm->nmpls->if_stats.switched_pkts = switched_pkts;
          nm->nmpls->if_stats.label_lookup_failures = label_lookup_failures;

          nm->nmpls->if_stats.out_fragments = out_fragments;
        }
      else
        {
          ifp = if_lookup_by_index (&nm->vr->ifm, ifindex);
          mif = nsm_mpls_if_lookup (ifp);
          if (mif)
            {
              mif->stats.rx_pkts = rx_pkts;

              mif->stats.dropped_pkts = dropped_pkts;
              mif->stats.ip_tx_pkts = ip_tx_pkts;
              mif->stats.labeled_tx_pkts = labeled_tx_pkts;

              mif->stats.dropped_labeled_pkts = dropped_labeled_pkts;
              mif->stats.switched_pkts = switched_pkts;
              mif->stats.label_lookup_failures = label_lookup_failures;

              mif->stats.out_fragments = out_fragments;
            }
        }
    }
  fclose (fp);
}

/* Read all FTN/VRF stats. */
void pal_mpls_ftn_stats_update (struct nsm_master *nm)
{
  FILE *fp;
  char buf[PROCBUFSIZ];
  u_int32_t vrf_id;
  int o1, o2, o3, o4;
  int prefixlen;
  int tx_bytes, pusked_pkts;
  int matched_bytes, matched_pkts;
  struct prefix p;
  struct ftn_entry *ftn;

  /* Open /proc/net/dev. */
  fp = fopen (_PATH_PROC_NET_MPLS_FTN_STATS, "r");
  if (fp == NULL)
    {
      zlog_warn (nm->zg, "Unable to open proc file %s: %s",
                 _PATH_PROC_NET_MPLS_FTN_STATS, strerror (errno));
      return;
    }

  /* Ignore header. */
  fgets (buf, PROCBUFSIZ, fp);

  /* Read all data. */
  while (fgets (buf, PROCBUFSIZ, fp) != NULL)
    {
      sscanf (buf, "%u %d.%d.%d.%d/%d %u %u %u %u",
              &vrf_id,
              &o1, &o2, &o3, &o4,
              &prefixlen,
              &tx_bytes, &pusked_pkts,
              &matched_bytes, &matched_pkts);

      /* Create prefix. */
      memset (&p, 0, sizeof (struct prefix));
      p.family = AF_INET;
      p.prefixlen = prefixlen;
      p.u.prefix4.s_addr = pal_ntoh32 ((o1 << 24) | (o2 << 16) | (o3 << 8) | o4);
      
      /* Lookup correct FTN entry. */
      ftn = nsm_gmpls_ftn_lookup_installed (nm, vrf_id, &p);
      if (ftn)
        {
          ftn->stats.tx_bytes = tx_bytes;
          ftn->stats.pushed_pkts = pusked_pkts;
          ftn->stats.matched_bytes = matched_bytes;
          ftn->stats.matched_pkts = matched_pkts;
        }
    }
  fclose (fp);
}

/* Read all ILM stats. */
void pal_mpls_ilm_stats_update (struct nsm_master *nm)
{
  FILE *fp;
  char buf[PROCBUFSIZ];
  u_int32_t ifindex;
  u_int32_t label;
  u_int32_t rx_bytes;
  u_int32_t tx_bytes;
  u_int32_t rx_pkts;
  u_int32_t swapped_pkts;
  u_int32_t popped_pkts;
  struct ilm_entry *ilm;

  /* Open /proc/net/dev. */
  fp = fopen (_PATH_PROC_NET_MPLS_ILM_STATS, "r");
  if (fp == NULL)
    {
      zlog_warn (nm->zg, "Unable to open proc file %s: %s",
                 _PATH_PROC_NET_MPLS_ILM_STATS, strerror (errno));
      return;
    }

  /* Ignore header. */
  fgets (buf, PROCBUFSIZ, fp);

  /* Read all data. */
  while (fgets (buf, PROCBUFSIZ, fp) != NULL)
    {
      sscanf (buf, "%u:%u %u %u %u %u %u",
              &ifindex,
              &label,
              &rx_bytes,
              &tx_bytes,
              &rx_pkts,
              &swapped_pkts,
              &popped_pkts);

      /* Lookup correct ILM entry. */
      ilm = nsm_mpls_ilm_lookup_installed (nm, ifindex, label);
      if (ilm)
        {
          ilm->stats.rx_bytes = rx_bytes;
          ilm->stats.tx_bytes = tx_bytes;
          ilm->stats.rx_pkts = rx_pkts;
          ilm->stats.swapped_pkts = swapped_pkts;
          ilm->stats.popped_pkts = popped_pkts;
        }
    }
  fclose (fp);
}

/* Read all NHLFE stats. */
void pal_mpls_nhlfe_stats_update (struct nsm_master *nm)
{
  FILE *fp;
  char buf[PROCBUFSIZ];
  u_int32_t nhlfe_ix;
  u_int32_t tx_bytes;
  u_int32_t tx_pkts;
  u_int32_t error_pkts;
  u_int32_t discard_pkts;
  struct nhlfe_entry *nhlfe;

  /* Open /proc/net/dev. */
  fp = fopen (_PATH_PROC_NET_MPLS_NHLFE_STATS, "r");
  if (fp == NULL)
    {
      zlog_warn (nm->zg, "Unable to open proc file %s: %s",
                 _PATH_PROC_NET_MPLS_NHLFE_STATS, strerror (errno));
      return;
    }

  /* Ignore header. */
  fgets (buf, PROCBUFSIZ, fp);

  /* Read all data. */
  while (fgets (buf, PROCBUFSIZ, fp) != NULL)
    {
      sscanf (buf, "%u %u %u %u %u",
              &nhlfe_ix,
              &tx_bytes,
              &tx_pkts,
              &error_pkts,
              &discard_pkts);

      /* Lookup correct ILM entry. */
      nhlfe = nsm_mpls_nhlfe_ix_lookup (nm, nhlfe_ix);
      if (nhlfe)
        {
          nhlfe->stats.tx_bytes += tx_bytes;
          nhlfe->stats.tx_pkts += tx_pkts;
          nhlfe->stats.error_pkts += error_pkts;
          nhlfe->stats.discard_pkts += discard_pkts;
        }
    }
  fclose (fp);
}
#endif /* HAVE_MPLS_FWD */
