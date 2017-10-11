/* Copyright (C) 2003,  All Rights Reserved. */

#include "pal.h"

#include "lacp_types.h"
#include "lacp_main.h"
#include "lacp_debug.h"
#include "lacp_selection_logic.h"
#include "lacp_timer.h"
#include "lacp_link.h"
#include "lacp_tx.h"
#include "lacpd.h"
#include "lacp_nsm.h"
#include "lacp_timer.h"

#ifdef HAVE_HA
#include "lacp_cal.h"
#endif /* HAVE_HA */

int 
begin ()
{
  return lacp_instance->lacp_begin;
}


/* Link management routines. */

static struct lacp_link *lacp_link_hash[LACP_PORT_HASH_SIZE];
#define lacp_link_hashfn(IDX) ((IDX) & LACP_PORT_HASH_MASK)

/* Insert a link entry in our local list. */
static void 
link_link (struct lacp_link *link)
{
  int hash_index = lacp_link_hashfn (link->actor_port_number);
  struct lacp_link ** head = & lacp_link_hash[hash_index];

  if (LACP_DEBUG(EVENT))
    zlog_info(LACPM, "link_link: index = %d ifindex = %d", hash_index, 
              link->actor_port_number);
  link->next = *head;
  *head = link;
}

/* Remove a link entry from our local list. */
void 
unlink_link (struct lacp_link *link)
{
  struct lacp_link *next, **pprev;

  pprev = &lacp_link_hash[lacp_link_hashfn(link->actor_port_number)];
  next = *pprev;
  while (next != link) 
    {
      pprev = &next->next;
      next = *pprev;
    }

  *pprev = link->next;
}

/* Find a link based on ifindex of the device */

struct lacp_link *
lacp_find_link (u_int32_t port_num)
{
  struct lacp_link *link;

  for (link = lacp_link_hash[lacp_link_hashfn(port_num)]; 
       link; link = link->next) 
    {
      if (link->actor_port_number == port_num)
        return link;
    }

  return NULL;
}

/* Find a link based on name of the device */

struct lacp_link *
lacp_find_link_by_name (const char *const name)
{
  register struct lacp_link *link;
  int hash;

  for (hash = 0; hash < LACP_PORT_HASH_SIZE; hash++)
    {
      for (link = lacp_link_hash[hash]; link; link = link->next) 
        {
          if (pal_strncmp(link->name, name, LACP_IFNAMSIZ) == 0)
            return link;
        }
    }

  return NULL;
}

/* Set the link data structure to the default values. 
   This function presumes that the name parameter has
   been verified. */

static int
link_set_defaults (struct lacp_link *link,
                   char *name,
                   struct interface *ifp)
{
  if (name)
    pal_strcpy (link->name, name);

  link->lacp_enabled = PAL_TRUE;
  link->actor_port_priority = LACP_DEFAULT_PORT_PRIORITY;

  /* 
     Set up partner defaults - lacp_record_default 
     will actually copy these to oper 
  */
  CLR_LACP_ACTIVITY (link->partner_admin_port_state);
  SET_DEFAULTED (link->partner_admin_port_state);
  SET_AGGREGATION (link->partner_admin_port_state);

  SET_LACP_ACTIVITY (link->actor_admin_port_state);
  SET_DEFAULTED (link->actor_admin_port_state);
  SET_AGGREGATION (link->actor_admin_port_state);
  
  link->actor_oper_port_state = link->actor_admin_port_state;
  link->partner_oper_port_state = link->partner_admin_port_state;
  link->partner_oper_system_priority  = link->partner_admin_system_priority = 
    LACP_DEFAULT_SYSTEM_PRIORITY;
  
  if (ifp)
    {
      pal_mem_cpy (link->mac_addr, ifp->hw_addr, LACP_GRP_ADDR_LEN);
      link->actor_port_number = ifp->ifindex;
      link->actor_oper_port_key = 
        link->actor_admin_port_key = 0;
      link->ifp = ifp;
      link->port_enabled = if_is_up (ifp);
    }
  
  return RESULT_OK;
}

int
lacp_initialize_link (struct lacp_link *link)
{
  if (IS_LACP_LINK_ENABLED (link))
    {
      lacp_instance->lacp_begin = PAL_TRUE;
      link->ntt = PAL_TRUE;
      lacp_run_sm (link);
      lacp_instance->lacp_begin = PAL_FALSE;
#ifdef HAVE_HA
       lacp_cal_modify_instance ();
       lacp_cal_modify_lacp_link (link);
#endif /* HAVE_HA */
    }

  return RESULT_OK;
}



/* Manage a link under LACP.  Name must exist and be unique. */
int
lacp_add_link (char *name, struct lacp_link **pplink)
{
  struct lacp_link  * link;
  struct interface *ifp;

  if (pplink)
    *pplink = NULL;

  if (name == NULL)
    {
      zlog_err (LACPM, "lacp_add_link: invalid link name parameter");
      return RESULT_ERROR;
    }

  link = XCALLOC (MTYPE_LACP_LINK, sizeof (struct lacp_link));
  if (link == NULL)
    {
      zlog_err (LACPM, "lacp_add_link: unable to allocate link %s", name);
      return RESULT_ERROR;
    }

  ifp = ifg_lookup_by_name (&LACPM->ifg, name);

  link_set_defaults (link, name, ifp);
  
  /* add link to hash table */
  link_link (link);

  if (pplink)
    *pplink = link;

#ifdef HAVE_HA
  lacp_cal_create_lacp_link (link);
#endif /* HAVE_HA */

  return RESULT_OK;
}

void
link_cleanup (struct lacp_link *link, bool_t notify_nsm)
{
  struct lacp_aggregator *agg = NULL;

  if (! link)
    return;

  if (link->aggregator)
    {
      agg = link->aggregator;
      lacp_disaggregate_link (link, notify_nsm);       
    }
  else if ((agg = lacp_aggregator_lookup_by_key (link->actor_admin_port_key)))
    if (agg->refCnt == 0)
      if (FLAG_ISSET (agg->flag, LACP_AGG_FLAG_INSTALLED)) 
        {  
          lacp_nsm_aggregator_delete (agg);
          UNSET_FLAG (agg->flag, LACP_AGG_FLAG_INSTALLED);
        }

  if (agg && agg->refCnt == 0)
    {
#ifdef HAVE_HA
      lacp_cal_delete_lacp_agg (agg);
#endif /* HAVE_HA */

      unlink_aggregator (agg);
      XFREE (MTYPE_LACP_AGGREGATOR, agg);
    }
  
  if (link->current_while_timer)
    {
#ifdef HAVE_HA
      lacp_cal_delete_current_while_timer (link);
#endif /* HAVE_HA */
      LACP_TIMER_OFF (link->current_while_timer);
    }

  if (link->actor_churn_timer)
    {
#ifdef HAVE_HA
      lacp_cal_delete_actor_churn_timer (link);
#endif /* HAVE_HA */

      LACP_TIMER_OFF (link->actor_churn_timer);
    }

  if (link->periodic_timer)
    {
#ifdef HAVE_HA
      lacp_cal_delete_periodic_timer (link);
#endif /* HAVE_HA */

      LACP_TIMER_OFF (link->periodic_timer);
    }

  if (link->partner_churn_timer)
    {
#ifdef HAVE_HA
      lacp_cal_delete_partner_churn_timer (link);
#endif /* HAVE_HA */

      LACP_TIMER_OFF (link->partner_churn_timer);
    }

  if (link->wait_while_timer)
    {
#ifdef HAVE_HA
      lacp_cal_delete_wait_while_timer (link);
#endif /* HAVE_HA */

      LACP_TIMER_OFF (link->wait_while_timer);
    }
}


/* Release a link (device) from lacp control */
int
lacp_delete_link (struct lacp_link *link, bool_t notify_nsm)
{
  struct lacp_aggregator *temp = NULL; 

  if (link)
    {
      if (link->aggregator)
        {
          link->aggregator->refCnt--;
          UNSET_FLAG (link->flags, LINK_FLAG_AGG_MATCH);
        }
      else if ((temp = lacp_aggregator_lookup_by_key (link->actor_admin_port_key)))
        {
          temp->refCnt--;
          UNSET_FLAG (link->flags, LINK_FLAG_AGG_MATCH);
        }

      link_cleanup (link, notify_nsm);
      unlink_link (link);
      
      if (IS_LACP_LINK_ENABLED (link))
        lacp_update_selection_logic ();
      
#ifdef HAVE_HA
      /* Delete link from CAL*/
      lacp_cal_delete_lacp_link (link);
#endif /* HAVE_HA */

      XFREE (MTYPE_LACP_LINK, link);
    }

  return RESULT_OK;
}


void
lacp_link_delete_all (bool_t notify_nsm)
{
  register struct lacp_link *link, *link_next;
  int hash;

  for (hash = 0; hash < LACP_PORT_HASH_SIZE; hash++)
    {
      for (link = lacp_link_hash[hash]; link != NULL; link = link_next)
        {
          link_next = link->next;

#ifdef HAVE_HA
          /* Delete link from CAL*/
          lacp_cal_delete_lacp_link (link);
#endif /* HAVE_HA */

          link_cleanup (link, notify_nsm);

          if (IS_LACP_LINK_ENABLED (link))
            lacp_update_selection_logic ();

          XFREE (MTYPE_LACP_LINK, link);
        }

      lacp_link_hash[hash] = NULL;
    }
}


int
lacp_link_port_detach (struct lacp_link *link, bool_t notify_nsm)
{
  struct lacp_aggregator *temp = NULL;

  if (! link)
    return RESULT_ERROR;

  if (link->aggregator)
    {
      link->aggregator->refCnt--;
      UNSET_FLAG (link->flags, LINK_FLAG_AGG_MATCH);
    }
  else if ((temp = lacp_aggregator_lookup_by_key (link->actor_admin_port_key)))
    {
      temp->refCnt--;
      UNSET_FLAG (link->flags, LINK_FLAG_AGG_MATCH);
    }

  link_cleanup (link, notify_nsm);
  unlink_link (link);

  if (IS_LACP_LINK_ENABLED (link))
    lacp_update_selection_logic ();

  pal_mem_set (link->mac_addr, 0, LACP_GRP_ADDR_LEN);
  link->actor_port_number = 0;
  link->ifp = NULL;
  link->port_enabled = PAL_FALSE;

  link_link (link);
#ifdef HAVE_HA
  lacp_cal_modify_lacp_link (link);
#endif /* HAVE_HA */

  return RESULT_OK;
}


void
lacp_link_port_detach_all (bool_t notify_nsm)
{
  register struct lacp_link *link, *link_next;
  int hash;

  for (hash = 0; hash < LACP_PORT_HASH_SIZE; hash++)
    {
      for (link = lacp_link_hash[hash]; link; link = link_next)
                        {
                                link_next = link->next;

                                lacp_link_port_detach (link, notify_nsm);
                        }
    }
}



int
lacp_link_port_attach (struct lacp_link *link, struct interface *ifp)
{
  if (! link || ! ifp)
    return RESULT_ERROR;

  unlink_link (link);

  pal_mem_cpy (link->mac_addr, ifp->hw_addr, LACP_GRP_ADDR_LEN);
  link->actor_port_number = ifp->ifindex;
  link->ifp = ifp;
  link->port_enabled = if_is_up (ifp);
  
  /* add link to hash table */
  link_link (link);

  if (IS_LACP_LINK_ENABLED (link))
    {
      lacp_selection_logic_add_link (link); 
      lacp_initialize_link (link); 
      /* lacp_attempt_to_aggregate_link (link); */
    }

  return RESULT_OK;
}

/* Set the admin key for a link */

int
lacp_set_link_admin_key (struct lacp_link *link, const unsigned int key)
{
  struct lacp_aggregator *agg;
  bool_t link_enabled = PAL_FALSE;

  if (link == NULL)
    return RESULT_ERROR;

  if (key > LACP_LINK_ADMIN_KEY_MAX)
    return RESULT_ERROR;

  if (key != link->actor_admin_port_key)
    {
      /* If the new key value is different and the port is aggregated,
         we need to pull it out of the aggregation group. */
      if (link->aggregator)
        {
          agg = link->aggregator;
          link->aggregator->refCnt--;
          UNSET_FLAG (link->flags, LINK_FLAG_AGG_MATCH);
          lacp_disaggregate_link (link, PAL_TRUE);
        }
      else if ((agg = lacp_aggregator_lookup_by_key (link->actor_admin_port_key)))
        {
          agg->refCnt--;
          UNSET_FLAG (link->flags, LINK_FLAG_AGG_MATCH);
          if (agg->refCnt == 0)
            if (FLAG_ISSET (agg->flag, LACP_AGG_FLAG_INSTALLED))
              {
                /* send aggregator delete msg to nsm */
                lacp_nsm_aggregator_delete (agg);
                UNSET_FLAG (agg->flag, LACP_AGG_FLAG_INSTALLED);
              }
        }

      if (agg && agg->refCnt == 0)
        {
#ifdef HAVE_HA
          lacp_cal_delete_lacp_agg (agg);
#endif /* HAVE_HA */

          unlink_aggregator (agg);
          XFREE (MTYPE_LACP_AGGREGATOR, agg);
        }
  
      link_enabled = IS_LACP_LINK_ENABLED (link);

      link->actor_oper_port_key = link->actor_admin_port_key = key;
      link->ntt = PAL_TRUE;

      if ((agg = lacp_aggregator_lookup_by_key (link->actor_oper_port_key)) != NULL)
        {
          SET_FLAG (link->flags, LINK_FLAG_AGG_MATCH);
          agg->refCnt++;
        }

      if (IS_LACP_LINK_ENABLED (link))
        {
          if (! link_enabled)
            {
              lacp_selection_logic_add_link (link); 
              lacp_initialize_link (link);
            }

          /* find a new aggregator */
          /* lacp_attempt_to_aggregate_link (link); */
        }
    }
#ifdef HAVE_HA
  lacp_cal_modify_lacp_link (link);
#endif /* HAVE_HA */

  return RESULT_OK;
}

/* Set the priority for a link. This will not cause a link to 
   disaggregate immediately. However, if an aggregator has the maximum
   nuumber of links attached and the new priority for this link is higher
   (and all other selection criteria is matched), then this link will
   replace the lowest priority link on the aggregator. */

int
lacp_set_link_priority (struct lacp_link *link, const unsigned int priority)
{
  if (link == NULL)
    return RESULT_ERROR;

  if (priority > LACP_LINK_PRIORITY_MAX)
    return RESULT_ERROR;

  link->actor_port_priority = priority;

  /* Let the selection logic know. */
  if (IS_LACP_LINK_ENABLED (link))
    lacp_update_selection_logic ();

  /* Tell the other side as soon as is possible. */
  link->ntt = PAL_TRUE;

  return RESULT_OK;
}

int
lacp_set_link_timeout (struct lacp_link *link, int timeout)
{
  if (! link)
    return RESULT_ERROR;

  if (timeout)
    {
      SET_LACP_TIMEOUT (link->actor_admin_port_state); /* Short Timeout */
      SET_LACP_TIMEOUT (link->actor_oper_port_state);
    }
  else
    {
      CLR_LACP_TIMEOUT (link->actor_admin_port_state); /* Long Timeout */
      CLR_LACP_TIMEOUT (link->actor_oper_port_state);
    }

  lacp_run_sm (link);

  return RESULT_OK;
}

int
attach_mux_to_aggregator (struct lacp_link *link)
{
  int ret;

  if (! link->aggregator)
    return RESULT_ERROR;

  /* send aggregator add/update message to nsm */
  ret = lacp_nsm_aggregator_link_add (link->aggregator, &link, 1);
  if (ret != RESULT_OK)
    return ret;

  if (! FLAG_ISSET (link->aggregator->flag, LACP_AGG_FLAG_INSTALLED))
    SET_FLAG (link->aggregator->flag, LACP_AGG_FLAG_INSTALLED);

  return RESULT_OK;
}


int
detach_mux_from_aggregator (struct lacp_link *link, bool_t notify_nsm)
                            
{
   if (! link->aggregator)
    return RESULT_ERROR;

  if (link->aggregator->refCnt == 0)
    {
      if (FLAG_ISSET (link->aggregator->flag, LACP_AGG_FLAG_INSTALLED))
        UNSET_FLAG (link->aggregator->flag, LACP_AGG_FLAG_INSTALLED);
    }
  if (notify_nsm)
    {
      /* send aggregaor update msg to nsm */
      lacp_nsm_aggregator_link_del (link->aggregator, &link, 1);
    }
        
  return RESULT_OK;
}

/* Add a link to an aggregator */

void 
lacp_aggregate_link (struct lacp_aggregator *agg, struct lacp_link *new)
{        
  register unsigned int linkNdx;

  /* Make sure we don't cross link things. */
  if ((new->aggregator != NULL) && (new->aggregator != agg))
    {
      lacp_disaggregate_link (new, PAL_TRUE);
    }
  if (new->aggregator == agg)
    {
      if (new->selected == STANDBY)
        new->ready_n = PAL_TRUE;

      /* 43.4.14.1.n */
      new->selected = SELECTED;
      new->ntt = PAL_TRUE;
      /* We've attached the link to the aggregator - no more to do. */
      if (LACP_DEBUG(EVENT))
        {
          zlog_debug (LACPM, "lacp_aggregate_link: link %d selected",
                      new->actor_port_number);
        }
    }
  else
    {
      for (linkNdx = 0; linkNdx < LACP_MAX_AGGREGATOR_LINKS; linkNdx++) 
        {
          if (agg->link[linkNdx] == NULL)
            {
              agg->link[linkNdx] = new;
              new->agg_link_index = linkNdx;
              new->aggregator = agg;
              if (agg->linkCnt == 0)
                {
                  pal_mem_cpy (agg->aggregator_mac_address, new->mac_addr, 
                               LACP_GRP_ADDR_LEN);
                  pal_mem_cpy (agg->partner_system, new->partner_oper_system,
                               LACP_GRP_ADDR_LEN);
                  agg->partner_system_priority 
                    = new->partner_oper_system_priority;
                  agg->partner_oper_aggregator_key = new->partner_oper_key;
                }
              agg->linkCnt++;

              if (new->selected == STANDBY)
                new->ready_n = PAL_TRUE;

              /* 43.4.14.1.n */
              new->selected = SELECTED;
              new->ntt = PAL_TRUE;
              /* 43.4.6 */
              agg->individual_aggregator = 
                (GET_AGGREGATION (new->actor_admin_port_state) == 
                 LACP_AGGREGATION_INDIVIDUAL);
              /* We've attached the link to the aggregator - no more to do. */
              if (LACP_DEBUG(EVENT))
                {
                  zlog_debug (LACPM, "lacp_aggregate_link: link %d selected",
                              new->actor_port_number);
                }
              return;
            }
        }
    }
}


/* Remove a link from an aggregator. */

void
lacp_disaggregate_link (struct lacp_link *link, bool_t notify_nsm)
{
  register unsigned int linkNdx;
  register struct lacp_aggregator * agg;
  struct lacp_link *curr;

  if (link == NULL)
    return;

  if (link->aggregator == NULL)
    return;

  LACP_ASSERT (link->selected == SELECTED);

  agg = link->aggregator;

  for (linkNdx = 0; linkNdx < LACP_MAX_AGGREGATOR_LINKS; linkNdx++) 
    {
      if (agg->link[linkNdx] == link)
        {
          curr = agg->link[linkNdx];

#if defined (MUX_INDEPENDENT_CTRL)
          disable_collecting (link);
          disable_distributing (link);
#else
          disable_collecting_distributing (link);
#endif
          detach_mux_from_aggregator (link, notify_nsm);

          /* Detach the aggregator association */
          agg->link[linkNdx] = NULL;
          link->aggregator = NULL;
          /* Reset the link index in the agg->link array */
          link->agg_link_index = 0;
          if (agg->linkCnt > 0)
            agg->linkCnt--;

          if (agg->linkCnt == 0)
            {
              /* Clear aggregator variables */
              agg->transmit_state = 0;
              agg->receive_state = 0;
              pal_mem_set (agg->aggregator_mac_address, 0, LACP_GRP_ADDR_LEN);
              /* 43.4.6 */
              pal_mem_set (agg->partner_system, 0, LACP_GRP_ADDR_LEN);
              agg->partner_system_priority = 0;
              agg->partner_oper_aggregator_key = 0;
#if defined (LACP_DYNAMIC_AGG_ASSIGNMENT)
              agg->actor_oper_aggregator_key = 0;
#endif
            }
          else
            {
              /* If we remove the link having the mac address used by 
                 the aggregator, we need to change the mac address 
                 of the aggregator to another link in the aggregation. 
                 We are using the address of a local mac (See 43.2.10)
                 for the aggregator. */
              if (pal_mem_cmp (agg->aggregator_mac_address, 
                               curr->mac_addr, LACP_GRP_ADDR_LEN) == 0)
                {
                  int sublinkNdx;
                  for (sublinkNdx = 0; sublinkNdx < LACP_MAX_AGGREGATOR_LINKS;
                       sublinkNdx++)
                    {
                      if (agg->link[sublinkNdx])
                        pal_mem_cpy (agg->aggregator_mac_address, 
                                     agg->link[sublinkNdx]->mac_addr, 
                                     LACP_GRP_ADDR_LEN);

                      /* update the aggregator mac address in forwarder */
                                        lacp_aggregator_set_mac_address (agg, agg->aggregator_mac_address);
                    }
                }
            }
          
          /* 43.4.14.1.n */
          link->selected = UNSELECTED;
          link->ntt = PAL_TRUE;

          link->partner_oper_system_priority = 
            link->partner_admin_system_priority = LACP_DEFAULT_SYSTEM_PRIORITY;

#ifdef HAVE_HA
          lacp_cal_modify_lacp_link (link);
#endif /* HAVE_HA */
          /* We've found our link and disaggregated it. No need to do more. */
          if (LACP_DEBUG(EVENT))
            {
              zlog_debug (LACPM, "lacp_disaggregate_link: link %d unselected",
                          link->actor_port_number);
            }
          return;
        }
    }

#ifdef HAVE_HA
  lacp_cal_modify_lacp_agg (agg);
#endif /* HAVE_HA */

}


/* Give each aggregated link a transmission credit (so LACP can run).
   It also runs the state machine on every link. */

void
lacp_update_tx_credits ()
{
  register struct lacp_link * link;
  unsigned int hash;

  for (hash = 0; hash < LACP_PORT_HASH_SIZE; hash++)
    {
      for (link = lacp_link_hash[hash]; link; link = link->next) 
        {
          /* Credit is 3 per second - see 43.4.16 para. 3 */
          if (link->tx_count >= LACP_TX_LIMIT)
            link->tx_count -= LACP_TX_LIMIT;
          else
            link->tx_count = 0;
          if (link->ntt)
            lacp_run_sm (link);

#ifdef HAVE_HA
          lacp_cal_modify_lacp_link (link);
#endif /* HAVE_HA */
        }
    }
}

/* Return -1 if l1 < l2, 0 if l1 = l2, 1 if l1 > l2.
   The idea is that we order links strictly by priority
   where the priority is determined by the actor_priority
   or the partner_priority if our system id is numerically
   less than our system id. See 43.6.1.c. */

#define PARTNER_HAS_PRIORITY(ll) \
    GET_DEFAULTED ((ll)->partner_oper_port_state) \
    && (((ll)->partner_oper_system_priority < LACP_SYSTEM_PRIORITY) \
     || (((ll)->partner_oper_system_priority == LACP_SYSTEM_PRIORITY) \
      && (pal_mem_cmp ((ll)->partner_oper_system, LACP_SYSTEM_ID, LACP_GRP_ADDR_LEN) < 0)))

/* compare_link_priorities campares the priorities of two links
   based on the rules contained in 43.6.1.a, b, and c. Specifically,
   the comparison takes into account that the partner system
   priorities may be used. */

result_t 
compare_link_priorities (const void * l1, 
                         const void * l2)
{
  int l1_priority;
  int l2_priority;

  l1_priority = PARTNER_HAS_PRIORITY((struct lacp_link *)l1) 
    ? ((struct lacp_link *)l1)->partner_oper_port_priority 
    : ((struct lacp_link *)l1)->actor_port_priority;

  l2_priority = PARTNER_HAS_PRIORITY((struct lacp_link *)l2) 
    ? ((struct lacp_link *)l2)->partner_oper_port_priority 
    : ((struct lacp_link *)l2)->actor_port_priority;

  /* Lower number is greater priority */
  return l1_priority - l2_priority;
}


/* This function generates a list of links known to the system sorted
   in priority order. The argument is expected to point to an array
   of lacp_link pointers. It is expected that there is sufficient
   room in the array to hold pointers for all possible links in the 
   system (LACP_MAX_LINKS). The function returns the number of links
   in the list. */

int
lacp_generate_sorted_links (struct lacp_link * links[],
                            bool_t ignore_disabled)
{
  register struct lacp_link *link;
  register int i;
  register int j;
  unsigned int hash;
  unsigned int  link_cnt = 0;

  if (links == NULL)
    return 0;

  /* Flush the contents of the links array before generating sorted links */
  pal_mem_set (links, 0, LACP_MAX_LINKS * 4);

  /* First we form a list of links sorted by priority */
  for (hash = 0; hash < LACP_PORT_HASH_SIZE; hash++)
    {
      if (link_cnt == LACP_MAX_LINKS)
        break;

      for (link = lacp_link_hash[hash]; link; link = link->next) 
        {
          if (ignore_disabled && ! IS_LACP_LINK_ENABLED (link))
            continue;
          
          if (link_cnt >= LACP_MAX_LINKS)
            break;
          
          links[link_cnt++] = link;
        }
    }

  if (link_cnt <= 1)
    return link_cnt;

  for (i = 0; i < (link_cnt - 1); i++)
    {
      for (j = (i+1); j < link_cnt; j++)
        {
          if (compare_link_priorities (links[i], links[j]) > 0)
            {
              link = links[i];
              links[i] = links[j];
              links[j] = link;
            }
        }
    }
  
  return link_cnt;
}



#if defined (MUX_INDEPENDENT_CTRL)
/* 43.3.11 */

/* 43.4.9 Enable_Collecting */

void
enable_collecting (struct lacp_link * link)
{
  /* 43.4.15.e */
  if (link->aggregator)
    {
      link->aggregator->receive_state++;
      lacp_nsm_enable_collecting (link->aggregator->name, link->name);
      link->ntt = PAL_TRUE;
    }
}

/* 43.4.9 Disable_Collecting */
void
disable_collecting (struct lacp_link * link)
{
  /* 43.4.15.e */
  if (link->aggregator && link->aggregator->receive_state)
    {
      if (link->aggregator->receive_state)
        link->aggregator->receive_state--;
      lacp_nsm_disable_collecting (link->aggregator->name, link->name);
      link->ntt = PAL_TRUE;
    }
}

/* 43.4.9 Enable_Distributing */

void
enable_distributing (struct lacp_link * link)
{
  /* 43.4.15.f */
  if (link->aggregator)
    {
      link->aggregator->transmit_state++;
      lacp_nsm_enable_distributing (link->aggregator->name, link->name);
    }
}

/* 43.4.9 Disable_Distributing */

void
disable_distributing (struct lacp_link * link)
{
  /* 43.4.15.f */
  if (link->aggregator && link->aggregator->transmit_state)
    {
      if (link->aggregator->transmit_state)
        link->aggregator->transmit_state--;
      lacp_nsm_disable_distributing (link->aggregator->name, link->name);
    }
}

#else
/* 43.3.11 */

/* 43.4.9 Enable_Collecting_Distributing */

void
enable_collecting_distributing (struct lacp_link * link)
{
  /* 43.4.15.e */
  if (link->aggregator)
    {
      link->aggregator->receive_state++;

      lacp_nsm_collecting_distributing (link->aggregator->name, 
                                        link->ifp->ifindex, 1);
      link->ntt = PAL_TRUE;
    }
}

/* 43.4.9 Disable_Collecting_Distributing */
void
disable_collecting_distributing (struct lacp_link * link)
{
  /* 43.4.15.e */
  if (link->aggregator && link->aggregator->receive_state)
    {
      if (link->aggregator->receive_state)
        link->aggregator->receive_state--;
      lacp_nsm_collecting_distributing (link->aggregator->name, 
                                        link->ifp->ifindex, 0);
      link->ntt = PAL_TRUE;
    }
}
#endif
