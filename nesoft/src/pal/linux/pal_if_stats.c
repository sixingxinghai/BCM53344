/* Copyright (C) 2002-2003   All Rights Reserved. */

/*
** pal_if_stats.c -- Functions for formatting output of statistics
*/

/*
** Include files
*/
#include "pal.h"
#include "pal_string.h"
#include "pal_if_stats.h"
#include "nsm_snmp.h"

/*
** Functions
*/

#ifdef HAVE_PROC_NET_DEV

/* Procedure to fill up stats for snmp support */
void
pal_if_get_stats (struct pal_if_stats *stats,
                  struct nsm_if_stats *buf)
{
  buf->ifInOctets = stats->rx_bytes;
  buf->ifInUcastPkts = stats->rx_packets - stats->rx_multicast;
  buf->ifInNUcastPkts = stats->rx_multicast;
  buf->ifInDiscards = stats->rx_dropped;
  buf->ifInErrors = stats->rx_errors;
  buf->ifInUnknownProtos = 0; /* no support yet */
  buf->ifOutOctets = stats->tx_bytes;
  buf->ifOutUcastPkts = 0; /* no support */
  buf->ifOutNUcastPkts = 0; /* no support yet */
  buf->ifOutDiscards = stats->tx_dropped; 
  buf->ifOutErrors = stats->tx_errors;

  return;
}

/*
** pal_if_stats_get_input_stats ()
** This doesnot add the newline character. The caller needs to do that.
*/
void
pal_if_stats_get_input_stats (struct pal_if_stats *stats,
                              char * buf, 
                              s_int32_t len)
{
  pal_snprintf(buf, len,
               "    input packets %u, bytes %u, dropped %u, multicast packets %u",
               stats->rx_packets, stats->rx_bytes,
               stats->rx_dropped, stats->rx_multicast);
}

/*
** pal_if_stats_get_input_errors ()
** This doesnot add the newline character. The caller needs to do that.
*/
void
pal_if_stats_get_input_errors (struct pal_if_stats *stats,
                               char * buf, 
                               s_int32_t len)
{
  pal_snprintf(buf, len, 
               "    input errors %u, length %u, overrun %u, CRC %u, frame %u, fifo %u, missed %u",
               stats->rx_errors, stats->rx_length_errors,
               stats->rx_over_errors, stats->rx_crc_errors,
               stats->rx_frame_errors, stats->rx_fifo_errors,
               stats->rx_missed_errors);
}

/*
** pal_if_stats_get_output ()
** This doesnot add the newline character. The caller needs to do that.
*/
void
pal_if_stats_get_output (struct pal_if_stats *stats, 
                         char * buf, 
                         s_int32_t len)
{
  pal_snprintf(buf, len, "    output packets %u, bytes %u, dropped %u",
               stats->tx_packets, stats->tx_bytes, stats->tx_dropped);
}

/*
** pal_if_stats_get_output_errors ()
** This doesnot add the newline character. The caller needs to do that.
*/
void
pal_if_stats_get_output_errors (struct pal_if_stats *stats, 
                                char * buf, 
                                s_int32_t len)
{
  pal_snprintf(buf, len, "    output errors %u, aborted %u, carrier %u, fifo %u, heartbeat %u, window %u",
               stats->tx_errors, stats->tx_aborted_errors,
               stats->tx_carrier_errors, stats->tx_fifo_errors,
               stats->tx_heartbeat_errors, stats->tx_window_errors);
}

/*
** pal_if_stats_get_collisions ()
** This doesnot add the newline character. The caller needs to do that.
*/
void
pal_if_stats_get_collisions (struct pal_if_stats *stats,
                             char * buf, 
                             s_int32_t len)
{
  pal_snprintf(buf, len, "    collisions %u", stats->collisions);
}
#else

/*
** pal_if_stats_get_input_stats ()
** This doesnot add the newline character. The caller needs to do that.
*/
void
pal_if_stats_get_input_stats (struct pal_if_stats *stats,
                              char * buf, 
                              s_int32_t len)
{
  pal_snprintf(buf, len, "%s", "");
}

/*
** pal_if_stats_get_input_errors ()
** This doesnot add the newline character. The caller needs to do that.
*/
void
pal_if_stats_get_input_errors (struct pal_if_stats *stats, 
                               char * buf, 
                               s_int32_t len)
{
  pal_snprintf(buf, len, "%s", "");
}

/*
** pal_if_stats_get_output ()
** This doesnot add the newline character. The caller needs to do that.
*/
void
pal_if_stats_get_output (struct pal_if_stats *stats, 
                         char * buf, 
                         s_int32_t len)
{
  pal_snprintf(buf, len, "%s", "");
}

/*
** pal_if_stats_get_output_errors ()
** This doesnot add the newline character. The caller needs to do that.
*/
void
pal_if_stats_get_output_errors (struct pal_if_stats *stats, 
                                char * buf, 
                                s_int32_t len)
{
  pal_snprintf(buf, len, "%s", "");
}

/*
** pal_if_stats_get_collisions ()
** This doesnot add the newline character. The caller needs to do that.
*/
void
pal_if_stats_get_collisions (struct pal_if_stats *stats, 
                             char * buf, 
                             s_int32_t len)
{
  pal_snprintf(buf, len, "%s", "");
}

#endif /* HAVE_PROC_NET_DEV */
