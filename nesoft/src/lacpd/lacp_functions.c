/* Copyright (C) 2003  All Rights Reserved. */

#include "lacp_types.h"
#include "lacpd.h"
#include "lacp_main.h"
#include "lacp_debug.h"
#include "lacp_link.h"


/* record the pdu values - 43.4.9 */

void
lacp_record_pdu (struct lacp_link * link)
{
  struct lacp_pdu * pdu = link->pdu;
  unsigned int is_sync = PAL_FALSE;
  int is_maintaining; 

  if (pdu == NULL)
    return;

  /* 43.4.9 recordPDU paragraph 5 */
  is_maintaining = ((GET_LACP_ACTIVITY(pdu->actor_state) == LACP_ACTIVITY_ACTIVE) 
    || ((GET_LACP_ACTIVITY(link->actor_oper_port_state) == LACP_ACTIVITY_ACTIVE) 
    && (GET_LACP_ACTIVITY(pdu->partner_state) == LACP_ACTIVITY_ACTIVE)));

  /* 43.4.9 recordPDU paragraph 1 */
  link->partner_oper_port_number = pdu->actor_port_number;
  link->partner_oper_port_priority = pdu->actor_port_priority;
  pal_mem_cpy (link->partner_oper_system, pdu->actor_system, 
      LACP_GRP_ADDR_LEN);
    
  /*When there is a change in system priority, the LAG ID changes */
  if (link->partner_oper_system_priority != pdu->actor_system_priority)
    link->partner_change_count++;
       
  link->partner_oper_system_priority = pdu->actor_system_priority;
  link->partner_oper_key = pdu->actor_key;
  /* Record partner state (except SYNCHRONIZATION which is set or cleared below) */
  link->partner_oper_port_state = pdu->actor_state;

  CLR_DEFAULTED (link->actor_oper_port_state);


  if (LACP_DEBUG(SYNC))
    {
      zlog_debug (LACPM, "lacp_record_pdu: actor link id:%d", link->actor_port_number);
      zlog_debug (LACPM, "lacp_record_pdu: port - actor:%d partner:%d", 
          link->actor_port_number, pdu->partner_port);
      zlog_debug (LACPM, "lacp_record_pdu: port priority - actor:%d partner:%d", 
          link->actor_port_priority, pdu->partner_port_priority);
      zlog_debug (LACPM, "lacp_record_pdu: key - actor:%d partner:%d", 
          link->actor_port_priority, pdu->partner_port_priority);
      zlog_debug (LACPM, "lacp_record_pdu: system prio - actor:%d partner:%d", 
          LACP_SYSTEM_PRIORITY, pdu->partner_system_priority);
      zlog_debug (LACPM, "lacp_record_pdu: system actor: %.04x-%.02x:%.02x:%.02x:%.02x:%.02x:%02x",
                  LACP_SYSTEM_PRIORITY,
                  LACP_SYSTEM_ID[0], 
                  LACP_SYSTEM_ID[1], 
                  LACP_SYSTEM_ID[2], 
                  LACP_SYSTEM_ID[3], 
                  LACP_SYSTEM_ID[4], 
                  LACP_SYSTEM_ID[5]);
      zlog_debug (LACPM, "lacp_record_pdu: system partner: %.04x-%.02x:%.02x:%.02x:%.02x:%.02x:%02x",
                  pdu->partner_system_priority,
                  pdu->partner_system[0],
                  pdu->partner_system[1],
                  pdu->partner_system[2],
                  pdu->partner_system[3],
                  pdu->partner_system[4],
                  pdu->partner_system[5]);

      zlog_debug (LACPM, "lacp_record_pdu: Actor flags - %d%d%d%d%d%d%d%d",
                  GET_LACP_ACTIVITY(link->actor_oper_port_state),
                  GET_LACP_TIMEOUT(link->actor_oper_port_state),
                  GET_AGGREGATION(link->actor_oper_port_state),
                  GET_SYNCHRONIZATION(link->actor_oper_port_state),
                  GET_COLLECTING(link->actor_oper_port_state),
                  GET_DISTRIBUTING(link->actor_oper_port_state),
                  GET_DEFAULTED(link->actor_oper_port_state),
                  GET_EXPIRED(link->actor_oper_port_state));

      zlog_debug (LACPM, "lacp_record_pdu: PDU partner flags - %d%d%d%d%d%d%d%d",
        GET_LACP_ACTIVITY(pdu->partner_state),
        GET_LACP_TIMEOUT(pdu->partner_state),
        GET_AGGREGATION(pdu->partner_state),
        GET_SYNCHRONIZATION(pdu->partner_state),
        GET_COLLECTING(pdu->partner_state),
        GET_DISTRIBUTING(pdu->partner_state),
        GET_DEFAULTED(pdu->partner_state),
        GET_EXPIRED(pdu->partner_state));

      zlog_debug (LACPM, "lacp_record_pdu: PDU actor flags - %d%d%d%d%d%d%d%d",
        GET_LACP_ACTIVITY(pdu->actor_state),
        GET_LACP_TIMEOUT(pdu->actor_state),
        GET_AGGREGATION(pdu->actor_state),
        GET_SYNCHRONIZATION(pdu->actor_state),
        GET_COLLECTING(pdu->actor_state),
        GET_DISTRIBUTING(pdu->actor_state),
        GET_DEFAULTED(pdu->actor_state),
        GET_EXPIRED(pdu->actor_state));

      zlog_debug (LACPM, "lacp_record_pdu: comparison -> pt:%d pr:%d si:%d sp:%d ky:%d ag:%d sn:%d mn:%d",
        (link->actor_port_number == pdu->partner_port),
        (link->actor_port_priority == pdu->partner_port_priority),
        (pal_mem_cmp (LACP_SYSTEM_ID, pdu->partner_system, LACP_GRP_ADDR_LEN) == 0),
        (LACP_SYSTEM_PRIORITY == pdu->partner_system_priority),
        (link->actor_oper_port_key == pdu->partner_key),
        (GET_AGGREGATION (link->actor_oper_port_state) == GET_AGGREGATION (pdu->partner_state)),
        (GET_SYNCHRONIZATION (pdu->actor_state) == LACP_SYNCHRONIZATION_IN_SYNC),
        is_maintaining);
    }

  /* 43.4.9 recordPDU paragraph 2 */
  is_sync = ((link->actor_port_number == pdu->partner_port)
   && (link->actor_port_priority == pdu->partner_port_priority)
   && (pal_mem_cmp (LACP_SYSTEM_ID, pdu->partner_system, LACP_GRP_ADDR_LEN) == 0)
   && (LACP_SYSTEM_PRIORITY == pdu->partner_system_priority)
   && (link->actor_oper_port_key == pdu->partner_key)
   && (GET_AGGREGATION (link->actor_oper_port_state) == GET_AGGREGATION (pdu->partner_state))
   && (GET_SYNCHRONIZATION (pdu->actor_state) == LACP_SYNCHRONIZATION_IN_SYNC)
   && is_maintaining)
  /* 43.4.9 recordPDU paragraph 3 */
      || ((GET_AGGREGATION (pdu->actor_state) == LACP_AGGREGATION_INDIVIDUAL)
   && (GET_SYNCHRONIZATION (pdu->actor_state) == LACP_SYNCHRONIZATION_IN_SYNC) 
   && is_maintaining);

  if (is_sync)
    {
      if (LACP_DEBUG(SYNC))
        {
          zlog_debug (LACPM, "lacp_record_pdu: link %d synchronized", link->actor_port_number);
        }
      SET_SYNCHRONIZATION (link->partner_oper_port_state);
      link->partner_sync_transition_count++;
    } 
  else
    {
    if (LACP_DEBUG(SYNC))
      {
        zlog_debug (LACPM, "lacp_record_pdu: link %d not synchronized", link->actor_port_number);
      }
      /* 43.4.9 recordPDU paragraph 4 */
      CLR_SYNCHRONIZATION (link->partner_oper_port_state);
    } 
}

/* 43.4.9 recordDefault */

void
lacp_record_default (struct lacp_link * link)
{
  link->partner_oper_port_number = link->partner_admin_port_number;
  link->partner_oper_port_priority = link->partner_admin_port_priority;
  pal_mem_cpy (link->partner_oper_system, link->partner_admin_system, 
              LACP_GRP_ADDR_LEN);
  link->partner_oper_system_priority = link->partner_admin_system_priority;
  link->partner_oper_key = link->partner_admin_key;
  link->partner_oper_port_state = link->partner_admin_port_state;
  if (LACP_DEBUG(EVENT))
    {
      zlog_debug (LACPM, "lacp_record_default: link %d defaulted", link->actor_port_number);
    }

  SET_DEFAULTED (link->actor_oper_port_state);
}

/* 43.4.9 - update_Selected */

void
lacp_update_selected (struct lacp_link *link)
{
  struct lacp_pdu * pdu = link->pdu;
  if (pdu == NULL)
    return;
  if (LACP_DEBUG(SYNC))
    {
      zlog_debug (LACPM, "lacp_update_selected: actor link %d", link->actor_port_number);
      zlog_debug (LACPM, "lacp_update_selected: port - partner:%d actor:%d", link->partner_oper_port_number, pdu->actor_port_number);
      zlog_debug (LACPM, "lacp_update_selected: port prio - partner:%d actor:%d", link->partner_oper_port_priority, pdu->actor_port_priority);
      zlog_debug (LACPM, "lacp_update_selected: key - partner:%d actor:%d", link->partner_oper_port_priority, pdu->actor_port_priority);
      zlog_debug (LACPM, "lacp_update_selected: system prio - partner:%d actor:%d", link->partner_oper_system_priority, pdu->actor_system_priority);
      zlog_debug (LACPM, "lacp_update_selected: aggregation flag - partner:%d actor:%d", GET_AGGREGATION(link->partner_oper_port_state), GET_AGGREGATION(pdu->actor_state));
      zlog_debug (LACPM, "lacp_update_selected: partner system %.04x-%.02x:%.02x:%.02x:%.02x:%.02x:%02x",
          link->partner_oper_system_priority,
          link->partner_oper_system[0], 
          link->partner_oper_system[1], 
          link->partner_oper_system[2], 
          link->partner_oper_system[3], 
          link->partner_oper_system[4], 
          link->partner_oper_system[5]);

      zlog_debug (LACPM, "lacp_update_selected: actor system %.04x-%.02x:%.02x:%.02x:%.02x:%.02x:%02x",
          pdu->actor_system_priority,
          pdu->actor_system[0],
          pdu->actor_system[1],
          pdu->actor_system[2],
          pdu->actor_system[3],
          pdu->actor_system[4],
          pdu->actor_system[5]);
    }

  if ((link->partner_oper_port_number != pdu->actor_port_number)
   || (link->partner_oper_port_priority != pdu->actor_port_priority)
   || (pal_mem_cmp (link->partner_oper_system, pdu->actor_system, LACP_GRP_ADDR_LEN) != 0)
   || (link->partner_oper_system_priority != pdu->actor_system_priority)
   || (link->partner_oper_key != pdu->actor_key)
   || (GET_AGGREGATION (link->partner_oper_port_state) != GET_AGGREGATION (pdu->actor_state)))
    {
      if (link->aggregator)
        lacp_disaggregate_link (link, PAL_FALSE); /* link->selected = UNSELECTED; */

      if (LACP_DEBUG(SYNC))
        {
          zlog_debug (LACPM, "lacp_update_selected: link %d unselected", link->actor_port_number);
        }
    }
}

/* 43.4.9 - update_Default_Selected */
void
lacp_update_default_selected (struct lacp_link * link)
{
  if ((link->partner_oper_port_number != link->partner_admin_port_number)
   || (link->partner_oper_port_priority != link->partner_admin_port_priority)
   || (pal_mem_cmp (link->partner_oper_system, link->partner_admin_system, LACP_GRP_ADDR_LEN) != 0)
   || (link->partner_oper_system_priority != link->partner_admin_system_priority)
   || (link->partner_oper_key != link->partner_admin_key)
   || (GET_AGGREGATION (link->partner_oper_port_state) != GET_AGGREGATION (link->partner_admin_port_state)))
    {
      if (link->aggregator)
        lacp_disaggregate_link (link, PAL_TRUE); /* link->selected = UNSELECTED; */

      if (LACP_DEBUG(SYNC))
        {
          zlog_debug (LACPM, "lacp_update_default_selected: link %d unselected", link->actor_port_number);
        }
    }
}

/* 43.4.9 - update_NTT */
void
lacp_update_ntt (struct lacp_link * link)
{
  struct lacp_pdu * pdu = link->pdu;
  if (pdu == NULL)
    return;
  if ((link->actor_port_number != pdu->partner_port)
   || (link->actor_port_priority != pdu->partner_port_priority)
   || (pal_mem_cmp (LACP_SYSTEM_ID, pdu->partner_system, LACP_GRP_ADDR_LEN) != 0)
   || (LACP_SYSTEM_PRIORITY != pdu->partner_system_priority)
   || (link->actor_oper_port_key != pdu->partner_key)
   || (GET_LACP_ACTIVITY (link->actor_oper_port_state) != GET_LACP_ACTIVITY (pdu->partner_state))
   || (GET_LACP_TIMEOUT (link->actor_oper_port_state) != GET_LACP_TIMEOUT (pdu->partner_state))
   || (GET_SYNCHRONIZATION (link->actor_oper_port_state) != GET_SYNCHRONIZATION (pdu->partner_state))
   || (GET_AGGREGATION (link->actor_oper_port_state) != GET_AGGREGATION (pdu->partner_state)))
    {
      link->ntt = PAL_TRUE;
      if (LACP_DEBUG(SYNC))
        {
          zlog_debug (LACPM, "lacp_update_ntt: link %d sets ntt", link->actor_port_number);
        }
    }
}
