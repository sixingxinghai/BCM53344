/* Copyright (C) 2004  All Rights Reserved */

#include "pal.h"
#include "lib.h"
#ifdef HAVE_HAL
#include "hal_incl.h"
#endif /* HAVE_HAL */

#ifdef HAVE_RMOND
#include "nsmd.h"
#include "nsm_message.h"
#include "nsm_server.h"
#include "nsm_router.h"
#include "nsm_rmon.h"

#ifdef HAVE_HAL
static int
nsm_rmon_copy_stats (struct nsm_msg_rmon_stats *msg,
                     struct hal_if_counters *if_stats)
{
  msg->good_octets_rcv      = if_stats->good_octets_rcv;
  msg->bad_octets_rcv       = if_stats->bad_octets_rcv;
  msg->mac_transmit_err     = if_stats->mac_transmit_err;
  msg->good_pkts_rcv        = if_stats->good_pkts_rcv;
  msg->bad_pkts_rcv         = if_stats->bad_pkts_rcv;
  msg->brdc_pkts_rcv        = if_stats->brdc_pkts_rcv;
  msg->mc_pkts_rcv          = if_stats->mc_pkts_rcv;
  msg->pkts_64_octets       = if_stats->pkts_64_octets;
  msg->pkts_65_127_octets   = if_stats->pkts_65_127_octets;
  msg->pkts_128_255_octets  = if_stats->pkts_128_255_octets;
  msg->pkts_256_511_octets  = if_stats->pkts_256_511_octets;
  msg->pkts_512_1023_octets = if_stats->pkts_512_1023_octets;
  msg->pkts_1024_max_octets = if_stats->pkts_1024_max_octets;
  msg->good_pkts_sent       = if_stats->good_pkts_sent;
  msg->excessive_collisions = if_stats->excessive_collisions;
  msg->mc_pkts_sent         = if_stats->mc_pkts_sent;
  msg->brdc_pkts_sent       = if_stats->brdc_pkts_sent;
  msg->unrecog_mac_cntr_rcv = if_stats->unrecog_mac_cntr_rcv;
  msg->fc_sent              = if_stats->fc_sent;
  msg->good_fc_rcv          = if_stats->good_fc_rcv;
  msg->drop_events          = if_stats->drop_events;
  msg->undersize_pkts       = if_stats->undersize_pkts;
  msg->fragments_pkts       = if_stats->fragments_pkts;
  msg->oversize_pkts        = if_stats->oversize_pkts;
  msg->jabber_pkts          = if_stats->jabber_pkts;
  msg->mac_rcv_error        = if_stats->mac_rcv_error;
  msg->bad_crc              = if_stats->bad_crc;
  msg->collisions           = if_stats->collisions;
  msg->late_collisions      = if_stats->late_collisions;
  msg->bad_fc_rcv           = if_stats->bad_fc_rcv;

  return 0;
}
#endif /* HAVE_HAL */

int
nsm_server_recv_rmon_req_statistics (struct nsm_msg_header *header,
                                     void *arg, void *message)
{
  struct nsm_master *nm;
  struct nsm_server_entry *nse;
  struct nsm_server *ns;
  struct nsm_msg_rmon *msg;
#ifdef HAVE_HAL
  struct hal_if_counters if_stats;
#endif /* HAVE_HAL */
  struct nsm_msg_rmon_stats msg_stats; 

  /* Set connection and message.  */
  nse = arg;
  ns = nse->ns;
  msg = message;

  if (!msg)
    return 0;

  nm = nsm_master_lookup_by_id (nzg, header->vr_id);
  if (nm == NULL)
    return 0;

#ifdef HAVE_HAL
  hal_if_get_counters (msg->ifindex, &if_stats);
#endif /* HAVE_HAL */

  msg_stats.ifindex = msg->ifindex;

#ifdef HAVE_HAL
  nsm_rmon_copy_stats (&msg_stats, &if_stats);
#endif /* HAVE_HAL */

  nsm_rmon_send_statistics (nse, &msg_stats, header->message_id);

  return 0;
}

int
nsm_rmon_send_statistics (struct nsm_server_entry *nse,
                          struct nsm_msg_rmon_stats *msg, u_int32_t msg_id)
{
  int nbytes;

  /* Set encode pointer and size.  */
  nse->send.pnt = nse->send.buf + NSM_MSG_HEADER_SIZE;
  nse->send.size = nse->send.len - NSM_MSG_HEADER_SIZE;

  nbytes = nsm_encode_rmon_stats (&nse->send.pnt, &nse->send.size, msg );

  nsm_server_send_message (nse, 0, 0, NSM_MSG_RMON_SERVICE_STATS, msg_id, nbytes);
  return 0;
}

#endif /* HAVE_RMOND */
