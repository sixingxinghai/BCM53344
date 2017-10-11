/* Copyright (C) 2003,  All Rights Reserved. */

/* This module implements the transmit machine - 43.4.16. */
/* Also included here (because it was convenient) is the marker responder. */

#include "pal.h"
#include "lacp_config.h"
#include "lacp_types.h"
#include "lacpd.h"
#include "lacp_tx.h"
#include "lacpdu.h"
#include "lacp_main.h"
#include "lacp_debug.h"

#ifdef HAVE_HA
#include "lacp_cal.h"
#include "cal_api.h"
#endif /* HAVE_HA */

static unsigned char buf[LACPDU_FRAME_SIZE];

/* lacp_tx formats an LACPDU and transmits it if ntt is true for the link
   and the link tx count has not exceeeded the transmission limit.
   This function returns the number of octets successfully transmitted.
   It returns < 0 on failure. lacp_tx shares it's transmit buffer with
   marker_tx. */

int
lacp_tx (struct lacp_link *link)
{
  int ret;
  struct sockaddr_l2 addr;

#ifdef HAVE_HA
  if (HA_IS_STANDBY(LACPM))
    return RESULT_OK;
#endif /* HAVE_HA */

  if (link->ntt == PAL_FALSE)
    return -1;

  if (link->periodic_tx_state == PERIODIC_TX_NO_PERIODIC ||
      ! IS_LACP_LINK_ENABLED (link))
    {
      link->ntt = PAL_FALSE;
#ifdef HAVE_HA
      lacp_cal_modify_lacp_link (link);
#endif /* HAVE_HA */
      return -1;
    }

  if ((GET_LACP_ACTIVITY(link->actor_oper_port_state)
           == LACP_ACTIVITY_PASSIVE)
       && (GET_LACP_ACTIVITY(link->partner_oper_port_state)
           == LACP_ACTIVITY_PASSIVE))
    return -1;

  if (link->tx_count >= LACP_TX_LIMIT)
    return 0;

  if (format_lacpdu (link, buf) != RESULT_OK)
    return -1;

  if (LACP_DEBUG(EVENT) || LACP_DEBUG(PACKET))
    {
      zlog_debug (LACPM, "lacp_tx: Sending lacpdu on link %d", 
                  link->actor_port_number);

      if (LACP_DEBUG(PACKET))
        lacp_dump_packet (buf);
    }

  /* Build the destination address */
  pal_mem_cpy (addr.dest_mac, lacp_grp_addr, LACP_GRP_ADDR_LEN);
  pal_mem_cpy (addr.src_mac, link->mac_addr, LACP_GRP_ADDR_LEN);
  addr.port = link->actor_port_number;

  /* Send it. */
  ret = send_lacp (&addr, buf, LACPDU_FRAME_SIZE - 14 - 4);
  
  if (ret >= 0)
    {
      link->lacpdu_sent_count++;
      link->tx_count++;
    }
  else
    {
      link->pckt_sent_err_count++;
    }
  link->ntt = PAL_FALSE;

#ifdef HAVE_HA
  lacp_cal_modify_lacp_link (link);
#endif /* HAVE_HA */
  return ret;
}

/* marker_tx formats a MARKER pdu and transmits it.
   This function returns the number of octets successfully transmitted.
   It returns < 0 on failure. 
   marker_tx shares the transmit buffer with lacp_tx. */

int
marker_tx (struct lacp_link * link)
{
  int ret;
  struct sockaddr_l2  addr;

#ifdef HAVE_HA
  if (HA_IS_STANDBY(LACPM))
    return RESULT_OK;
#endif /* HAVE_HA */

  if (link->lacp_enabled == PAL_FALSE)
    return -1;
  if (link->aggregator == NULL)
    return -1;

  /* We always send a response for right now. */
  format_marker (link, buf, PAL_TRUE);

  /* Build the destination address */
  pal_mem_cpy (addr.dest_mac, lacp_grp_addr, LACP_GRP_ADDR_LEN);
  pal_mem_cpy (addr.src_mac, link->mac_addr, LACP_GRP_ADDR_LEN);
  addr.port = link->actor_port_number;

  /* Send it. */
  ret = send_lacp (&addr, buf, LACPDU_FRAME_SIZE - 14 - 4);
  
  /* We don't always increment tx_count here because marker protocol responses
     do not count against the rate limitation. See 43.5.4.1 last paragraph. */
  if ((ret > 0) && (link->received_marker_info.response == PAL_FALSE))
    {
      link->tx_count++;
      link->lacpdu_sent_count++;
    }
  else
    {
      if (ret < 0)
        {
          link->pckt_sent_err_count++;
        }
      else
        {
          link->mpdu_sent_count++;
        }
    }

#ifdef HAVE_HA
  lacp_cal_modify_lacp_link (link);
#endif /* HAVE_HA */
  return ret;
}
