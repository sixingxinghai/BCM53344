/* Copyright (C) 2001-2004  All Rights Reserved.  */

#include "pal.h"
#include "pal_mpls_stats.h"

#ifdef HAVE_MPLS_FWD
#include "ptree.h"
#ifdef HAVE_DIFFSERV
#include "diffserv.h"
#endif /* HAVE_DIFFSERV */
#include "qos_common.h"

#include "rsvpd/rsvp_interface.h"
#include "rsvpd/rsvp_intsrv.h"
#include "rsvpd/rsvp_path.h"
#include "rsvpd/rsvp_objects.h"
#ifdef HAVE_DSTE
#include "rsvpd/dste/rsvp_dste.h"
#endif /* HAVE_DSTE */
#include "rsvpd/rsvpd.h"
#include "rsvpd/rsvp_session.h"

#ifdef HAVE_PROC_NET_DEV
#ifndef _PATH_PROC_NET_MPLS_TUNNEL_STATS
#define _PATH_PROC_NET_MPLS_TUNNEL_STATS "/proc/net/mpls_tunnel_stats"
#endif
#ifndef PROCBUFSIZ
#define PROCBUFSIZ                    1024
#endif
#endif /* HAVE_PROC_NET_DEV */

/* Get TUNNEL entry stats. */
void
pal_mpls_tunnel_entry_stats (pal_tunnel_entry_stats_t *stats, char * buf, int len)
{
  buf[0] = '\0';

  snprintf(buf, len,
           "    TX bytes:%u, pkts:%u, error pkts:%u",
           stats->tx_bytes,
           stats->tx_pkts,
           stats->error_pkts);
}

/* Read all TUNNEL stats. */
void pal_mpls_tunnel_stats_update (void *m)
{
  FILE *fp;
  char buf[PROCBUFSIZ];
  u_int32_t trunk_id;
  u_int32_t lsp_id;
  struct pal_in4_addr ingr;
  struct pal_in4_addr egr;
  u_int32_t tx_bytes;
  u_int32_t tx_pkts;
  u_int32_t error_pkts;
  struct rsvp_master *rm = m;
  struct rsvp_session *session;

  /* Open /proc/net/dev. */
  fp = fopen (_PATH_PROC_NET_MPLS_TUNNEL_STATS, "r");
  if (fp == NULL)
    {
      zlog_warn (rm->zg, "Unable to open proc file %s: %s",
                 _PATH_PROC_NET_MPLS_TUNNEL_STATS, strerror (errno));
      return;
    }

  /* Ignore header. */
  fgets (buf, PROCBUFSIZ, fp);

  /* Read all data. */
  while (fgets (buf, PROCBUFSIZ, fp) != NULL)
    {
      sscanf (buf, "%u %u %x %x %u %u %u",
              &trunk_id,
              &lsp_id,
              &ingr.s_addr,
              &egr.s_addr,
              &tx_bytes,
              &tx_pkts,
              &error_pkts);

      /* Lookup correct ILM entry. */
      session = rsvp_session_lookup_snmp (trunk_id, lsp_id, &ingr, &egr);
      if (session)
        {
          session->stats.tx_bytes = tx_bytes;
          session->stats.tx_pkts = tx_pkts;
          session->stats.error_pkts = error_pkts;
        }
    }
  fclose (fp);
}
#endif /* HAVE_MPLS_FWD */
