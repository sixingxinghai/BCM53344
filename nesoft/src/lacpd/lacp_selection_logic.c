/* Copyright (C) 2003,  All Rights Reserved. */

/* This module implements the selection logic - 43.4.14.1 */

#include "pal.h"

#include "lacp_config.h"
#include "lacp_types.h"
#include "lacpd.h"
#include "lacp_debug.h"
#include "lacp_main.h"
#include "lacp_nsm.h"
#include "lacp_selection_logic.h"
#include "lacp_link.h"
#include "lacp_tx.h"

#ifdef HAVE_HA
#include "lacp_cal.h"
#endif /* HAVE_HA */

/* This routine updates the list of links used by the selection logic. 
   It must be called on every link addition/deletion/priority change. 
   It then runs the selection logic to let the changed links find an 
   aggregator. */

void
lacp_update_selection_logic ()
{
  lacp_instance->link_count = 
    lacp_generate_sorted_links (lacp_instance->links, PAL_TRUE);

#ifdef HAVE_HA
  lacp_cal_modify_instance ();
#endif /* HAVE_HA */
}


int
lacp_selection_logic_add_link (struct lacp_link *link)
{
  int i, j;
  struct lacp_link **links = lacp_instance->links;


  if (lacp_instance->link_count == LACP_MAX_LINKS || 
      ! IS_LACP_LINK_ENABLED (link))
    return RESULT_ERROR;

  if (lacp_instance->link_count == 0)
    {
      links[0] = link;
      lacp_instance->link_count = 1;
      
      return RESULT_OK;
    }

  for (i = 0; i < lacp_instance->link_count; i++)
    {
      if (compare_link_priorities (link, links[i]) <= 0)
        {
          for (j = lacp_instance->link_count; j > i; j--)
            links[j] = links[j-1];

          links[i] = link;
          lacp_instance->link_count++;
          return RESULT_OK;
        }
    }

  links[i] = link;
  lacp_instance->link_count++;

  return RESULT_OK;
}


/* Insert a aggregator entry in our local list. */

static void 
link_aggregator (struct lacp_aggregator *agg)
{
  register unsigned int agg_ndx;
  for (agg_ndx = 0; agg_ndx < LACP_AGG_COUNT_MAX; agg_ndx++)
    {
      if (LACP_AGG_LIST[agg_ndx] == NULL)
        {
          if (LACP_DEBUG(EVENT))
            zlog_info(LACPM, "link_aggregator: index = %d ifindex = %d", 
                      agg_ndx, 
                      agg->agg_ix);

          LACP_AGG_LIST[agg_ndx] = agg;
          agg->instance_agg_index = agg_ndx;
          lacp_instance->agg_cnt++;
          return;
        }
    }
}

/* Remove an aggregator entry from our local list of all aggregators. */

void 
unlink_aggregator (struct lacp_aggregator * agg)
{
  register unsigned int agg_ndx;
  for (agg_ndx = 0; agg_ndx < LACP_AGG_COUNT_MAX; agg_ndx++)
    {
      if (LACP_AGG_LIST[agg_ndx] == agg)
        {
          if (LACP_DEBUG(EVENT))
            zlog_info(LACPM, "unlink_aggregator: index = %d ifindex = %d", 
                      agg_ndx, 
                      agg->agg_ix);

          LACP_AGG_LIST[agg_ndx] = NULL;
          agg->instance_agg_index = -1;
          if (lacp_instance->agg_cnt > 0)
            lacp_instance->agg_cnt--;

          return;
        }
    }
}

/* Find an aggregator based on ifindex of the device */

struct lacp_aggregator *
lacp_find_aggregator (const int id)
{
  register unsigned int agg_ndx;
  for (agg_ndx = 0; agg_ndx < LACP_AGG_COUNT_MAX; agg_ndx++)
    {
      if ((LACP_AGG_LIST[agg_ndx] != NULL)
          && (LACP_AGG_LIST[agg_ndx]->agg_ix == id))
        {
          return LACP_AGG_LIST[agg_ndx];
        }
    }
  return NULL;
}

/* Find an aggregator based on name of the device */

struct lacp_aggregator *
lacp_find_aggregator_by_name (const char * const name)
{
  register unsigned int agg_ndx;
  for (agg_ndx = 0; agg_ndx < LACP_AGG_COUNT_MAX; agg_ndx++)
    {
      if ((LACP_AGG_LIST[agg_ndx] != NULL)
          && (pal_strncmp(LACP_AGG_LIST[agg_ndx]->name, name, 
                          LACP_IFNAMSIZ) == 0))
        return LACP_AGG_LIST[agg_ndx];
    }
  return NULL;
}

/* Aggregator management routines. */

/* This function initializes the aggregator variables. Mac address is
   not required. */
int
lacp_initialize_aggregator (struct lacp_aggregator * agg, 
                            const unsigned char * const name, 
                            const unsigned char * const mac_addr,
                            const int ifindex, 
                            const unsigned short key,
                            const unsigned int individual)
{
  if (agg && name)
    {
      pal_mem_cpy (agg->name, name, LACP_IFNAMSIZ);
      if (mac_addr)
        pal_mem_cpy (agg->aggregator_mac_address, mac_addr, LACP_GRP_ADDR_LEN);
      agg->agg_ix = ifindex;
      agg->individual_aggregator = (individual == PAL_FALSE) 
        ? PAL_FALSE : PAL_TRUE;
      agg->actor_admin_aggregator_key = key;
      agg->actor_oper_aggregator_key = key;
      pal_mem_set (agg->partner_system, 0, LACP_GRP_ADDR_LEN);
      agg->partner_system_priority = 0;
      agg->partner_oper_aggregator_key = 0;
      agg->receive_state = PAL_FALSE;
      agg->transmit_state = PAL_FALSE;

      /*Setting the default value for collector_max_delay */
      agg->collector_max_delay = LACP_COLLECTOR_MAX_DELAY;

      agg->ready = PAL_FALSE;
      return RESULT_OK;
    }
  return RESULT_ERROR;
}

/* Manage an aggregator under LACP. 
   Some restrictions: 
   The name is required and must be unique.
   The mac address must exist. It need not be unique (even tho' the 
   spec demands that it be unique).
   The aggregator identifier (ifindex argument) must be unique.

*/


struct lacp_aggregator *
lacp_add_aggregator (char *name, 
                     u_char *mac_addr,
                     int ifindex, 
                     u_int16_t key,
                     u_int32_t individual)
{
  struct lacp_aggregator *agg;

  if (lacp_instance->agg_cnt >= LACP_AGG_COUNT_MAX)
    {
      zlog_err (LACPM, "lacp_add_aggregator: maximum aggregators already allocated: %s",
                name);
      return NULL;
    }

  agg = XCALLOC (MTYPE_LACP_AGGREGATOR, sizeof (struct lacp_aggregator));
  if (agg == NULL)
    {
      zlog_err (LACPM, "lacp_add_aggregator: unable to allocate aggregator %s",
                name);
      return NULL;
    }

  if (lacp_initialize_aggregator (agg, name, mac_addr, ifindex, key, 
                                  individual) != RESULT_OK)
    {
      XFREE (MTYPE_LACP_AGGREGATOR, agg);
      zlog_err (LACPM, "lacp_add_aggregator: "
                "add aggregator %s failed in init", name);
      return NULL;
    }
  
  link_aggregator (agg);

#ifdef HAVE_HA
  lacp_cal_create_lacp_agg (agg);
#endif /* HAVE_HA */

  return agg;
}


void
lacp_aggregator_delete_process (struct lacp_aggregator *agg, bool_t notify_nsm)
{

#ifdef HAVE_HA
  lacp_cal_delete_lacp_agg (agg);
#endif /* HAVE_HA */

  lacp_aggregator_detach_links (agg, notify_nsm);
  unlink_aggregator (agg);
  
  XFREE (MTYPE_LACP_AGGREGATOR, agg);

#ifdef HAVE_HA
  lacp_cal_modify_instance ();
#endif /* HAVE_HA */
}

/* Release an aggregator from lacp management */

int
lacp_delete_aggregator (char *name)
{
  struct lacp_aggregator  *agg = lacp_find_aggregator_by_name (name);

  if (! agg)
    return RESULT_ERROR;

  lacp_aggregator_delete_process (agg, PAL_TRUE);

  return RESULT_OK;
}

void
lacp_aggregator_delete_all (bool_t notify_nsm)
{
  u_int32_t agg_ndx;
  struct lacp_aggregator *agg;

  for (agg_ndx = 0; agg_ndx < LACP_AGG_COUNT_MAX; agg_ndx++)
    {
      if ((agg = LACP_AGG_LIST[agg_ndx]) == NULL)
        continue;

      if (agg->linkCnt > 0) 
        lacp_aggregator_detach_links (agg, notify_nsm);

      if (FLAG_ISSET (agg->flag, LACP_AGG_FLAG_INSTALLED))
        {
          lacp_nsm_aggregator_delete (agg);
          UNSET_FLAG (agg->flag, LACP_AGG_FLAG_INSTALLED);
        }

      LACP_AGG_LIST[agg_ndx] = NULL;
      
      if (lacp_instance->agg_cnt > 0)
        lacp_instance->agg_cnt--;
      
#ifdef HAVE_HA
      lacp_cal_delete_lacp_agg (agg);
#endif /* HAVE_HA */

      XFREE (MTYPE_LACP_AGGREGATOR, agg);
    }
}


struct lacp_aggregator *
lacp_aggregator_lookup_by_key (u_int16_t key)
{
  u_int32_t agg_ndx;
  struct lacp_aggregator *agg;
  
  for (agg_ndx = 0; agg_ndx < LACP_AGG_COUNT_MAX; agg_ndx++)
    {
      if (LACP_AGG_LIST[agg_ndx] == NULL)
        continue;
      
      agg = LACP_AGG_LIST[agg_ndx];
      
      if (agg->actor_admin_aggregator_key == key)
        return agg;
    }

  return NULL;
}
      

void
lacp_aggregator_detach_links (struct lacp_aggregator *agg, bool_t notify_nsm)
{
  register int linkNdx;

  for (linkNdx = 0; linkNdx < LACP_MAX_AGGREGATOR_LINKS; linkNdx++)
    {
      if (agg->link[linkNdx])
        {
          agg->refCnt--;
          UNSET_FLAG (agg->link[linkNdx]->flags, LINK_FLAG_AGG_MATCH);
          lacp_disaggregate_link (agg->link[linkNdx], notify_nsm);
        }
    }
}



/* Port identification - 43.3.4 */

unsigned int
lacp_port_identifier (struct lacp_link * link)
{
  return (link->actor_port_priority & 0xffff0000) 
    | (link->actor_port_number & 0x0000ffff);
}

/* 43.3.4 */

unsigned int
lacp_partner_port_identifier (struct lacp_link * link)
{
  return (link->partner_oper_port_priority & 0xffff0000) 
    | (link->partner_oper_port_number & 0x0000ffff);
}

/* This function return '0' if the partner system id (oper system or 
    admin system) is NULL. i.e partner is a non lacp partner         */ 
int lacp_is_partner_system_NULL(u_char *partner_system)
{
  int i;
  int ret = -1;
 
  for (i = 0; i < LACP_GRP_ADDR_LEN; i++)
     {
       if (partner_system[i] == 0)
           ret = 0;
       else
           ret = -1;
     }
  return ret;
}


/* 43.4.14.1 - If this function returns an aggregator, then the link
   should be attached to it. If this function does not return an aggregator,
   then there is no suitable aggregator for the link.
   Note that this logic mimics 43.6.1 although it always 
   actively participates in the selection logic. A 
   strict implementation of 43.6.1 
   requires the selection logic to act on the partner_oper
   information for systems with a higher system aggregation 
   priority. */

struct lacp_aggregator *
lacp_search_for_aggregator (struct lacp_link *candidate_link)
{
  struct lacp_aggregator *agg;
  unsigned int linkNdx;
  struct lacp_link *link;

  if (! IS_LACP_LINK_ENABLED (candidate_link))
    return NULL;

  /* lookup existing aggregators */
  agg = lacp_aggregator_lookup_by_key (candidate_link->actor_oper_port_key);
  if (! agg)
    {
      /* create a new aggregator for this port */
      if (lacp_instance->agg_ix < LACP_AGG_IX_MAX_LIMIT &&
          lacp_instance->agg_cnt < LACP_AGG_COUNT_MAX)
        {
          char agg_name[LACP_IFNAMSIZ];
          struct lacp_aggregator *new_agg;
          
          pal_snprintf (agg_name, LACP_IFNAMSIZ, "po%d",
                        candidate_link->actor_oper_port_key);
          
          new_agg = lacp_add_aggregator (agg_name, candidate_link->mac_addr,
                                         lacp_instance->agg_ix,
                                         candidate_link->actor_oper_port_key, 
                                         PAL_FALSE);
          
          /* increment global aggregator index */
          if (new_agg)
            lacp_instance->agg_ix++;
          
          return new_agg;
        }

      return NULL;
    }

  /* If there's no other link, then choose this aggregator */
  if (agg->linkCnt == 0)
    {
      /* Here we overwrite the aggregator's key */
      agg->actor_oper_aggregator_key = 
        candidate_link->actor_oper_port_key;
      
      pal_mem_cpy (agg->aggregator_mac_address, candidate_link->mac_addr,
                   LACP_GRP_ADDR_LEN);
      return agg;
    }

  /* individual ports cannot be part of an aggregator having other ports */
  if ((GET_AGGREGATION (candidate_link->actor_oper_port_state) == 
       LACP_AGGREGATION_INDIVIDUAL)
      || ((GET_AGGREGATION (candidate_link->partner_oper_port_state)
           == LACP_AGGREGATION_INDIVIDUAL)
          && (candidate_link->partner_oper_key)))
    {
      if (candidate_link->aggregator == agg)
        return agg;
      
      return NULL;
    }

  for (linkNdx = 0; linkNdx < LACP_MAX_AGGREGATOR_LINKS; linkNdx++)
    {
      if ((link = agg->link[linkNdx]) == NULL) 
        continue;
          
      /* cannot add port to an aggregator having an individual port */
      if (GET_AGGREGATION (link->actor_oper_port_state) 
          == LACP_AGGREGATION_INDIVIDUAL)
        {
          return NULL;
        }

      /* ensure that the ports being aggregated have the same properties */
      if (! link->ifp ||
          link->ifp->lacp_admin_key != candidate_link->ifp->lacp_admin_key)
        {
          return NULL;
        }
         
      /* 43.4.14.1.g - Avoid loopback
       * Following are the conditions for Loopback test :-
       * Actor System Id and Partner System Id should be equal.
       * Port Identifier of Candidate link and First link of the
       * aggregated system are equal.
       */
      if (pal_mem_cmp (LACP_SYSTEM_ID, candidate_link->partner_oper_system,
                       LACP_GRP_ADDR_LEN) == 0)
        {
          if ((lacp_port_identifier (candidate_link) == 
              lacp_partner_port_identifier (link))
              && (lacp_port_identifier (link) == 
              lacp_partner_port_identifier (candidate_link)))
            {
              return NULL;
            }
        }

      /* 43.4.14.1.f & 43.4.14.1.l */
      if ((link->actor_oper_port_key == candidate_link->actor_oper_port_key)
          && (pal_mem_cmp (link->partner_oper_system,
                           candidate_link->partner_oper_system, 
                           LACP_GRP_ADDR_LEN) == 0)
          && (link->partner_oper_key == candidate_link->partner_oper_key))
        {
          return agg;
        }

      if ((link->actor_oper_port_key == candidate_link->actor_oper_port_key)
          && (lacp_is_partner_system_NULL(link->partner_oper_system) == 0)
          && (lacp_is_partner_system_NULL(link->partner_admin_system) == 0)
          && (link->partner_oper_key == 0))
        {
          return agg;
        }
    }

  return NULL;
}


/* Calculate the "ready" value for an aggregator. */

int
lacp_calculate_ready (struct lacp_aggregator * agg)
{
  register unsigned int linkNdx;
  if ((agg == NULL) || (agg->linkCnt == 0))
    return PAL_FALSE;

  for (linkNdx = 0; linkNdx < LACP_MAX_AGGREGATOR_LINKS; linkNdx++)
    {
      if (agg->link[linkNdx] && (agg->link[linkNdx]->ready_n == PAL_FALSE))
        {
          if (LACP_DEBUG(EVENT))
            {
              zlog_debug (LACPM, "lacp_calculate_ready: aggregator %d not ready",
                          agg->agg_ix);
            }
          return PAL_FALSE;
        }
    }
  if (LACP_DEBUG(EVENT))
    {
      zlog_debug (LACPM, "lacp_calculate_ready: aggregator %d ready",
                  agg->agg_ix);
    }
  return PAL_TRUE;
}

/* This routine calculates "ready" for all system aggregators. See 43.4.8. 
   It is called from lacp_selection_logic. */

void
lacp_ready_logic ()
{
  register struct lacp_aggregator * agg;
  register unsigned int agg_ndx;

  for (agg_ndx = 0; agg_ndx < LACP_AGG_COUNT_MAX; agg_ndx++)
    {
      if (LACP_AGG_LIST[agg_ndx] != NULL)
        {
          agg = LACP_AGG_LIST[agg_ndx];
          if (agg->linkCnt)
            {
              agg->ready = lacp_calculate_ready (agg);
#ifdef HAVE_HA
              lacp_cal_modify_lacp_agg (agg);
#endif /* HAVE_HA */
            }
        }
    }
}


/* Attempt to aggregate a link. Attempt to find a suitable
   aggregator, apply system policy, and aggregate 
   the link if all criteria are met. Link state must be STANDBY
   or UNSELECTED.  */

void
lacp_attempt_to_aggregate_link (struct lacp_link *link)
{
  register struct lacp_aggregator * agg;
  int linkNdx;
  int replaceNdx = -1;
  u_int32_t link_port_priority = 0;
  u_int32_t candidate_link_port_priority = 0;
  u_int32_t replace_link_port_priority = 0;

  if (! IS_LACP_LINK_ENABLED (link))
    return;

  link_port_priority = CALC_LINK_PRIORITY(link);

  /* Find an aggregator for this link. */
  agg = lacp_search_for_aggregator (link);
  if (agg == NULL)
    goto standby_link;

  if (! FLAG_ISSET (link->flags, LINK_FLAG_AGG_MATCH))
    {
      agg->refCnt++;
      SET_FLAG (link->flags, LINK_FLAG_AGG_MATCH);
    }
    
  /* If it's the same one as the link is already aggregated with, 
     we're done. */
  if (link->aggregator == agg)
    return;

  if (agg->linkCnt < LACP_MAX_AGGREGATOR_LINKS)
    {
      if (LACP_DEBUG(EVENT))
        {
          zlog_debug (LACPM, "lacp_attempt_to_aggregate_link: link %d "
                      "aggregates with %d",
                      link->actor_port_number, agg->agg_ix);
        }

      lacp_aggregate_link (agg, link);
    }
  else 
    {
      /* The link has chosen an aggregator, but the aggregator is full.
         If maximum number of links is aggregated on an aggregator, 
         examine the priority of each aggregated link vs. candidate link. 
         If candidate link has higher priority, disaggregate the lowest 
         priority link and aggregate this link. */

      /* This chunk of code finds the index (replaceNdx) of the 
         lowest priority link which is higher numerically than the 
         candidate link priority. */
      for (linkNdx = 0; linkNdx < LACP_MAX_AGGREGATOR_LINKS; linkNdx++)
        {
          if ((agg->link[linkNdx] != NULL) )
            {

              candidate_link_port_priority = CALC_LINK_PRIORITY(agg->link[linkNdx]);

              if ( link_port_priority < candidate_link_port_priority )
                {
                  if ((replaceNdx == -1))
                    {
                      replaceNdx = linkNdx;
                    }
                  else
                    {
                      replace_link_port_priority = CALC_LINK_PRIORITY(agg->link[replaceNdx]);

                      if ( replace_link_port_priority < 
                                  candidate_link_port_priority )
                        replaceNdx = linkNdx;
                    }
                }
            }
        }

      if (replaceNdx == -1)
        goto standby_link;
      
      /* 43.4.15.c */
      lacp_disaggregate_link (agg->link[replaceNdx], PAL_TRUE);
      lacp_aggregate_link (agg, link);
      if (LACP_DEBUG(EVENT))
        {
          zlog_debug (LACPM, "lacp_attempt_to_aggregate_link: link %d aggregates with %d",
                      link->actor_port_number, agg->agg_ix);
          zlog_debug (LACPM, "lacp_attempt_to_aggregate_link: link %d standby",
                      agg->link[replaceNdx]->actor_port_number);
        }
    }
  
#ifdef HAVE_HA
  lacp_cal_modify_lacp_link (link);
  lacp_cal_modify_lacp_agg (agg);
#endif /* HAVE_HA */

  return;
  
 standby_link:
  if (link->aggregator)
    lacp_disaggregate_link (link, PAL_TRUE);
  
  link->selected = STANDBY;

#ifdef HAVE_HA
  lacp_cal_modify_lacp_link (link);
  lacp_cal_modify_lacp_agg (agg);
#endif /* HAVE_HA */
}

/* This is the selection logic. Links are assigned to aggregators
   based strictly on link priority. The link priority (local or
   peer) utilized is based on comparison of the system priority
   vs. the peer system priority. The logic handles multiple 
   peer systems. N.B. This function is not thread-safe. */


void
lacp_selection_logic ()
{
  register struct lacp_link *link;
  register int link_ndx;

  for (link_ndx = 0; link_ndx < lacp_instance->link_count; link_ndx++)
    {
      link = lacp_instance->links[link_ndx];

      if (link == NULL)
        continue;

      if (! IS_LACP_LINK_ENABLED (link))
        continue;

      /* 43.4.15.b,c */
      if (((link->mux_machine_state == MUX_DETACHED) 
           && (link->selected == UNSELECTED))
          || (link->selected == STANDBY))
        {
          lacp_attempt_to_aggregate_link (link);
        }
      
      if (link->ntt)
        lacp_tx (link);
    }
}
