/* Copyright (C) 2002-2003  All Rights Reserved. */

#ifndef _PAL_IF_STATS_H
#define _PAL_IF_STATS_H

/*
  pal_if_stats.h -- PacOS PAL interface statistics
*/

#ifdef HAVE_PROC_NET_DEV

#undef  pal_if_stats

struct if_stats
{
  u_int32_t rx_packets;   /* total packets received       */
  u_int32_t tx_packets;   /* total packets transmitted    */
  u_int32_t rx_bytes;     /* total bytes received         */
  u_int32_t tx_bytes;     /* total bytes transmitted      */
  u_int32_t rx_errors;    /* bad packets received         */
  u_int32_t tx_errors;    /* packet transmit problems     */
  u_int32_t rx_dropped;   /* no space in linux buffers    */
  u_int32_t tx_dropped;   /* no space available in linux  */
  u_int32_t rx_multicast; /* multicast packets received   */
  u_int32_t rx_compressed;
  u_int32_t tx_compressed;
  u_int32_t collisions;

  /* detailed rx_errors: */
  u_int32_t rx_length_errors;
  u_int32_t rx_over_errors;       /* receiver ring buff overflow  */
  u_int32_t rx_crc_errors;        /* recved pkt with crc error    */
  u_int32_t rx_frame_errors;      /* recv'd frame alignment error */
  u_int32_t rx_fifo_errors;       /* recv'r fifo overrun          */
  u_int32_t rx_missed_errors;     /* receiver missed packet     */
  /* detailed tx_errors */
  u_int32_t tx_aborted_errors;
  u_int32_t tx_carrier_errors;
  u_int32_t tx_fifo_errors;
  u_int32_t tx_heartbeat_errors;
  u_int32_t tx_window_errors;
};

#define pal_if_stats  if_stats

#else
/* Dummy interface statistics structure */
struct pal_if_stats
{
  u_int32_t rx_packets;   /* total packets received       */
  u_int32_t tx_packets;   /* total packets transmitted    */
  u_int32_t rx_bytes;     /* total bytes received         */
  u_int32_t tx_bytes;     /* total bytes transmitted      */
  u_int32_t rx_errors;    /* bad packets received         */
  u_int32_t tx_errors;    /* packet transmit problems     */
  u_int32_t rx_dropped;   /* no space in linux buffers    */
  u_int32_t tx_dropped;   /* no space available in linux  */
  u_int32_t rx_multicast; /* multicast packets received   */
  u_int32_t rx_compressed;
  u_int32_t tx_compressed;
  u_int32_t collisions;

  /* detailed rx_errors: */
  u_int32_t rx_length_errors;
  u_int32_t rx_over_errors;       /* receiver ring buff overflow  */
  u_int32_t rx_crc_errors;        /* recved pkt with crc error    */
  u_int32_t rx_frame_errors;      /* recv'd frame alignment error */
  u_int32_t rx_fifo_errors;       /* recv'r fifo overrun          */
  u_int32_t rx_missed_errors;     /* receiver missed packet     */
  /* detailed tx_errors */
  u_int32_t tx_aborted_errors;
  u_int32_t tx_carrier_errors;
  u_int32_t tx_fifo_errors;
  u_int32_t tx_heartbeat_errors;
  u_int32_t tx_window_errors;
};
#endif /* HAVE_PROC_NET_DEV */

#include "pal_if_stats.def"

#endif /* PAL_IF_STATS_H */
