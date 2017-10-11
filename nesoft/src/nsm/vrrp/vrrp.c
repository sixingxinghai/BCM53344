/* Copyright (C) 2001-2003  All Rights Reserved. */

#include "pal.h"

#include "lib.h"
#include "vty.h"
#include "stream.h"
#include "if.h"
#include "prefix.h"
#include "log.h"
#include "checksum.h"
#include "vector.h"

#include "nsm/nsmd.h"
#include "nsm/nsm_interface.h"
#include "nsm/nsm_fea.h"
#include "nsm/nsm_debug.h"
#include "nsm/vrrp/vrrp.h"
#include "nsm/vrrp/vrrp_snmp.h"
#include "nsm/vrrp/vrrp_debug.h"

typedef enum vrrp_circuit_status {
  VRRP_CIRCUIT_NOT_USED = 0,
  VRRP_CIRCUIT_UP = 1,
  VRRP_CIRCUIT_DOWN = 2,
  VRRP_CIRCUIT_UNCHG = 4,
  VRRP_CIRCUIT_MAX
} vrrp_circuit_status_t;


/* Static function declarations */
static  int vrrp_state_to_master (VRRP_SESSION *sess, int reason);
static int vrrp_state_to_backup (VRRP_SESSION *sess);

/*
 * ---------------------------------------------------------
 * PUBLIC VRRP FUNCTIONS - ACCESSIBLE WITH #include "vrrp.h"
 * ---------------------------------------------------------
 */

/*------------------------------------------------------------------------
 * vrrp_adjust_priority () - Adjust priority using priority Delta.
 When prio is -1 return -1.
 *------------------------------------------------------------------------*/
u_int8_t
vrrp_adjust_priority (VRRP_SESSION *sess)
{
  /* Prio is not yet set.  */
  if (sess->prio < 0)
    return sess->prio;

  /* Monitored interface is defined and status is down.  */
  if (sess->monitor_if && ! sess->mc_status)
    {
      if (sess->conf_prio == VRRP_UNSET)
        {
          if (sess->owner == PAL_TRUE)
            return (VRRP_DEFAULT_IP_OWNER_PRIORITY - sess->priority_delta);
          else
            return (VRRP_DEFAULT_NON_IP_OWNER_PRIORITY >= sess->priority_delta
                    ? VRRP_DEFAULT_NON_IP_OWNER_PRIORITY - sess->priority_delta
                    : 0);
        }
      else
        return (sess->conf_prio >= sess->priority_delta
                ? sess->conf_prio - sess->priority_delta : 0);
    }
  else
    return sess->prio;
}

/*------------------------------------------------------------------------
 * vrrp_enable_sess () - Enable a session. The call may come from the
 *                       vrrp_if. c
 *                          - addition of the IP address;
 *                          - setting the interface UP;
 *                       or vrrp_api.c
 *                          - enable from the CLI.
 *                       The session must be in the ADMIN_STATE_UP,
 *                       otherwise it takes no effect.
 *------------------------------------------------------------------------*/
int
vrrp_enable_sess (VRRP_SESSION  *sess)
{
  int       iret;
  struct interface * monitor_ifp;
  struct nsm_master *nm;

#ifdef HAVE_IPV6
  struct pal_in6_addr addr;
  if (pal_inet_pton (AF_INET6, VRRP_MCAST_ADDR6, &addr) <= 0)
    return VRRP_FAILURE;
#endif /* HAVE_IPV6 */

  if (! VRRP_SESS_IS_SHUTDOWN(sess))
    return VRRP_OK;

  iret = VRRP_OK;
  monitor_ifp = NULL;
  nm = sess->vrrp->nm;

  /* We will allow the session to be ADMIN_UP even if the
     interface is down.
   */
  if (!if_is_up (sess->ifp) || !if_is_running (sess->ifp))
  {
      sess->init_msg_code = VRRP_INIT_MSG_IF_NOT_RUNNING;
    return VRRP_OK;
  }

  /* Check the onwer's address is installed or
     the VIP address has its subnet's real IP address installed.
     Once it is installed we will enter Running state.
   */
  if (! vrrp_if_can_vip_addr_be_used(sess))
    {
      sess->init_msg_code = VRRP_INIT_MSG_NO_SUBNET;
      return VRRP_OK;
    }
  /* If the circuit state is down stay in Shutdown state.
   * Once it goes up, we will automatically start the session.
   */
  if (sess->monitor_if)
    {
      if (! vrrp_if_monitored_circuit_state(sess))
        {
        if (IS_DEBUG_VRRP_EVENT)
            zlog_warn (NSM_ZG, "VRRP Warning: Session stays shutdown "
                       "- monitored circuit down\n", sess->vrid);
          sess->init_msg_code = VRRP_INIT_MSG_CIRCUIT_DOWN;
          return VRRP_OK;
        }
    }
  /* Join multicast group for first starting session on interface.
     Here we must return a failure and not toadvance to Admin UP state.
  */
  if (vrrp_if_join_mcast_grp_first (sess) != VRRP_OK)
        return VRRP_FAILURE;

  /* Change to proper state */
  if (sess->admin_state == VRRP_ADMIN_STATE_UP)
  {
    if (sess->prio == VRRP_DEFAULT_IP_OWNER_PRIORITY)
      vrrp_state_to_master (sess, VRRP_NEW_MASTER_REASON_PRIORITY);
    else
      vrrp_state_to_backup (sess);
  }
  return VRRP_OK;
}


/*------------------------------------------------------------------------
 * vrrp_shutdown_sess - Disable a session.
 *
 *------------------------------------------------------------------------*/
int
vrrp_shutdown_sess (VRRP_SESSION *sess)
{
  if (IS_DEBUG_VRRP_EVENT)
    zlog_warn (NSM_ZG, "VRRP: Shutdown session %d/%d/%s transition to"
               " INIT state\n", sess->af_type, sess->vrid, sess->ifp->name);

  /* If state is Master, must send Advt with priority 0.
     Sending makes only sense when the interface is running.
  */
  if (sess->state == VRRP_STATE_MASTER && if_is_running (sess->ifp))
    {
      vrrp_ip_send_advert (sess, 0);
    }
  /* Change state & remove IP addrs before leaving multicast group
  */
  if (vrrp_fe_state_change (sess, sess->state,
                            VRRP_STATE_INIT,
                            sess->owner) < 0)
    {
    if (IS_DEBUG_VRRP_EVENT) {
      zlog_warn (NSM_ZG, "VRRP Event: FE State change failed for %d/%d/%s\n",
                 sess->af_type, sess->vrid, sess->ifp->name);
    }
    /* Let's finish the business here anyway. */
  }
  /* Leave the mcast group if this is the last session on interface.
  */
  vrrp_if_leave_mcast_grp_last (sess);

    sess->state = VRRP_STATE_INIT;
  /* Session enters operational SHUTDOWN state. */

  return VRRP_OK;
}

/*------------------------------------------------------------------------
 * vrrp_monitor_circuit () - Check monitored interface.
 *
 * Purpose:
 *   Periodic monitoring of the monitored circuit.
 *   Called from the timer thread.
 *------------------------------------------------------------------------*/
static vrrp_circuit_status_t
_vrrp_monitor_circuit (struct nsm_master *nm, VRRP_SESSION *sess)
{
  bool_t old_stts, new_stts;

  /* Monitored circuit is not configured.  */
  if (! sess->monitor_if)
    return VRRP_CIRCUIT_NOT_USED;

  old_stts = sess->mc_status;
  new_stts = vrrp_if_monitored_circuit_state(sess);

  if (old_stts != new_stts)
    {
      if (new_stts == PAL_TRUE)
        {
          sess->mc_up_events++;
          sess->mc_status = PAL_TRUE;

          /* If another VRRP session protects monitored circuit/interface,
             the i/f down state must be caused by setting VMAC address...
             Wait 1..2 to make sure the circuit is really down.
          */
          sess->mc_down_cnt = 2;

          /* Revert back to configured priority.
          */
          if (sess->conf_prio == VRRP_UNSET)
            VRRP_SET_DEFAULT_PRIORITY (sess);
          else
            sess->prio = sess->conf_prio;

          zlog_info (NSM_ZG,
                     "VRRP Monitored circuit for sess: %d/%s went UP \n",
                     sess->vrid, sess->ifp->name);
          return VRRP_CIRCUIT_UP;
        }
      else
        {
          sess->mc_down_cnt--;
          if (sess->mc_down_cnt > 0)
            /* Let it be - maybe this guy is setting the VMAC address. */
            return VRRP_CIRCUIT_UNCHG;
          else
            {
              /* It looks like the circuit shut down for good. */
              sess->mc_down_events++;
              sess->mc_status = PAL_FALSE;
              sess->prio = vrrp_adjust_priority (sess);
              zlog_info (NSM_ZG,
                         "VRRP Monitored circuit for sess. %d/%s went DOWN\n",
                         sess->vrid, sess->ifp->name);
              return VRRP_CIRCUIT_DOWN;
            }
        }
    }
  return VRRP_CIRCUIT_UNCHG;
}

/*------------------------------------------------------------------------
 * _sess_timer_handler - Timer thread handler.
 *
 * Purpose:
 *   To scroll through the list of configured VRRP sessions, and take any
 *   appropriate action for any session.  This will include decrementing
 *   and/or resetting timers, sending messages, changing state, etc.
 *------------------------------------------------------------------------*/
static int
_sess_timer_handler(VRRP_SESSION *sess, struct nsm_master *nm)
{
  vrrp_circuit_status_t circuit_status;

  /* Update monitored circuit status.  */
  circuit_status = _vrrp_monitor_circuit (nm, sess);

  switch (sess->state)
    {
  case VRRP_STATE_INIT:
      /* Enable the session - if possible - when the circuit is up.
      */
      if (circuit_status == VRRP_CIRCUIT_UP)
        {
          if (sess->admin_state == VRRP_ADMIN_STATE_UP)
            {
              if (IS_DEBUG_VRRP_EVENT)
                zlog_warn (NSM_ZG,
                           "VRRP Event: Circuit up - start session %d \n",
                           sess->vrid);
              vrrp_enable_sess (sess);
            }
          return VRRP_OK;
        }
    break;

  case VRRP_STATE_BACKUP:
      /* If monitored circuit is broken then bring down session
       * because if master goes down, this might become
       * master and drop the traffic
       */
      if (circuit_status == VRRP_CIRCUIT_DOWN)
        {
          if (IS_DEBUG_VRRP_EVENT)
            zlog_warn (NSM_ZG,
                       "VRRP Event: Circuit down - shutdown Backup session %d\n",
                       sess->vrid);
          vrrp_shutdown_sess (sess);
          sess->init_msg_code = VRRP_INIT_MSG_CIRCUIT_DOWN;
          return VRRP_OK;
        }
    /* If timer expired --> Master */
      if (--sess->timer <= 0)
        {
          vrrp_state_to_master (sess,
                                VRRP_NEW_MASTER_REASON_MASTER_NO_RESPONSE);
#ifdef HAVE_IPV6
      if (sess->af_type == AF_INET6)
        vrrp_ipv6_join_solicit_node_multicast (sess);
#endif /* HAVE_IPV6 */
    }
    break;

  case VRRP_STATE_MASTER:
      /* Circuit is broken bring the session down to avoid traffic loss.
       */
      if (circuit_status == VRRP_CIRCUIT_DOWN)
        {
          if (IS_DEBUG_VRRP_EVENT)
            zlog_warn (NSM_ZG,
                       "VRRP Event: Circuit down - shutdown Master session %d\n",
                       sess->vrid);
          vrrp_shutdown_sess (sess);
          sess->init_msg_code = VRRP_INIT_MSG_CIRCUIT_DOWN;
          return VRRP_OK;
        }

    /* If timer expired --> Tx advt */
      if (--sess->timer <= 0)
        {
          vrrp_ip_send_advert (sess, sess->prio);
      sess->timer = sess->adv_int;
        }
    break;
  }
  return VRRP_OK;
}

void
vrrp_timer (struct vrrp_global *vrrp)
{
  struct nsm_master *nm = vrrp->nm;

  vrrp_api_walk_sessions(vrrp->nm->vr->id, nm,
                         (vrrp_api_sess_walk_fun_t) _sess_timer_handler,
                         NULL);
}


/*------------------------------------------------------------------------
 * vrrp_handle_advert() - This function handles validated advertisment message
 *------------------------------------------------------------------------
 */
#ifdef HAVE_IPV6
int
vrrp_handle_advert (VRRP_SESSION *sess,
                    struct vrrp_advt_info *advt,
                    struct pal_sockaddr_in4 *from4,
                    struct pal_sockaddr_in6 *from6)
#else
int
vrrp_handle_advert (VRRP_SESSION *sess,
                    struct vrrp_advt_info *advt,
                    struct pal_sockaddr_in4 *from4,
                    struct pal_sockaddr_in4 *from6)
#endif /* HAVE_IPV6 */
{
  /* Connected address list. */
  struct connected *ifc_ipv4;
#ifdef HAVE_IPV6
  struct connected *ifc_ipv6;
#endif /* HAVE_IPV6 */
  struct interface *ifp;
  ifp      = sess->ifp;

  /* What state is the session in? */
  switch (sess->state) {

  case VRRP_STATE_INIT:         /* Drop packet */
    break;

  case VRRP_STATE_BACKUP:
    if (advt->priority == 0) {          /* Check msg priority */
      sess->timer = sess->skew_time;
      break;
    }
    /* Check Preempt */
    if (sess->preempt == PAL_FALSE)
      {
        sess->timer = sess->master_down_int;
        break;
      }
    /* Preempt mode - check priority. */
    if (advt->priority >= sess->prio)
      {
        sess->timer = sess->master_down_int;
        break;
      }
    /* Let the master_down_int epxire and make the coup.
     */
    break;

  case VRRP_STATE_MASTER:
    if (advt->priority == 0) {          /* Check msg priority */
      break;
    }
    /* Give up Master based on priority.
    */
    if ((advt->priority > sess->prio && sess->preempt == PAL_TRUE)
        || (advt->priority == VRRP_MAX_OPER_PRIORITY))
      {
	/* Transition to backup only if I am not the owner
            for the virtual ip address configured. */
        if (sess->owner != PAL_TRUE) 
          vrrp_state_to_backup (sess);
        break;
      }
    /* Give up Master based on lower IP address.
    */
    if (advt->priority == sess->prio)
      {
        if (sess->af_type == AF_INET)
          {
            for (ifc_ipv4 = ifp->ifc_ipv4; ifc_ipv4; ifc_ipv4 = ifc_ipv4->next)
               if (!CHECK_FLAG (ifc_ipv4->flags, NSM_IFA_SECONDARY))
                 {
                   if (IPV4_ADDR_CMP (&from4->sin_addr,
                                      &ifc_ipv4->address->u.prefix4) > 0)
                     {
                       vrrp_state_to_backup (sess);
                       break;
                     }
                 }
            break;
          }
#ifdef HAVE_IPV6
        if (sess->af_type == AF_INET6)
          {
            for (ifc_ipv6 = ifp->ifc_ipv6; ifc_ipv6;
                 ifc_ipv6 = ifp->ifc_ipv6->next)
               if (IN6_IS_ADDR_LINKLOCAL (&ifc_ipv6->address->u.prefix6))
                 {
                   if (IPV6_ADDR_CMP (&from6->sin6_addr,
                                      &ifc_ipv6->address->u.prefix6) > 0)
                     {
                       vrrp_state_to_backup (sess);
                       break;
                     }
                 }
            break;
          }
#endif /* HAVE_IPV6 */
     break;
      }
  }
  return VRRP_OK;
}

/*------------------------------------------------------------------------
 * vrrp_state_to_master - State change to Master for a session.  This function
 *              takes all action necessary to transition a VRRP router
 *              to the Master state for a particular session (vrid).  This
 *              includes all data manipulation and any calling of message
 *              sending functoins.
 *------------------------------------------------------------------------*/
static int
vrrp_state_to_master (VRRP_SESSION *sess, int reason)
{
  if (IS_DEBUG_VRRP_EVENT)
    zlog_warn (NSM_ZG, "VRRP Event: Transition to MASTER state for %d/%d/%s\n",
               sess->af_type, sess->vrid, sess->ifp->name);

  if (! if_is_running (sess->ifp))
    {
      if (IS_DEBUG_VRRP_EVENT)
        zlog_warn (NSM_ZG, "VRRP Warning: Interface %s is down; "
                   " can't transition to MASTER state\n", sess->ifp->name);
      return VRRP_FAILURE;
    }

  /* Perform any layer 2 manipulation necessary to change to Master     */
  if ((vrrp_fe_state_change (sess, sess->state,
                             VRRP_STATE_MASTER, sess->owner)) != VRRP_OK)
    {
      if (IS_DEBUG_VRRP_EVENT)
        zlog_warn (NSM_ZG, "VRRP Error: State change failed for %d/%d/%s",
                   sess->af_type, sess->vrid, sess->ifp->name);

      return VRRP_FAILURE;
    }

  /* Need to transmit an advertisement, since you're now the Master.  */
  if (vrrp_ip_start_master (sess) != VRRP_OK) {
    if (IS_DEBUG_VRRP_SEND)
      zlog_warn (NSM_ZG, "VRRP: Error starting sendingt\n");
    return VRRP_FAILURE;
  }

  /* Set the State variable */
  sess->state = VRRP_STATE_MASTER;
  sess->stats_become_master++;
  sess->new_master_reason = reason;
#ifdef HAVE_SNMP
  /*Function to be called for trap generation */
  if (sess->vrrp->notificationcntl == VRRP_NOTIFICATIONCNTL_ENA) {
    vrrp_trap_new_master (sess, sess->new_master_reason);
  }
#endif
  return VRRP_OK;
}


/*------------------------------------------------------------------------
 * vrrp_state_to_backup- State change to Backup for a session.  This function
 *              takes all action necessary to transition a VRRP router
 *              to the Backup state for a particular session (vrid).  This
 *              includes all data manipulation and any calling of message
 *              sending functoins.
 *------------------------------------------------------------------------*/
static int
vrrp_state_to_backup (VRRP_SESSION *sess)
{
  if (IS_DEBUG_VRRP_EVENT)
    zlog_warn (NSM_ZG, "VRRP Event: Transition to BACKUP state for %d/%d/%s\n",
               sess->af_type, sess->vrid, sess->ifp->name);

  /* When in Backup state, the countdown timer represents the   */
  /* Master_Down_Timer, and must be set appropriately.          */
  sess->timer = sess->master_down_int;

  /* Perform any layer 2 manipulation necessary to change to Backup     */
  if ((vrrp_fe_state_change (sess, sess->state,
                             VRRP_STATE_BACKUP, sess->owner)) != VRRP_OK)
    {
      if (IS_DEBUG_VRRP_SEND)
        zlog_warn (NSM_ZG, "VRRP Error: State change failed for %d/%d/%s",
                   sess->af_type, sess->vrid, sess->ifp->name);

      return VRRP_FAILURE;
    }

  /* Set the state variable */
  sess->state = VRRP_STATE_BACKUP;

  return VRRP_OK;
}

/* Virtual IP addresses vector handling functions (added for future purposes). */         
                                                                                          
/* To fetch IPv4 Virtual Ip addresses from the advt packet received. */                   
int                                                                                       
vrrp_vipaddr_vector_get_ipv4 (struct vrrp_advt_info *advt, struct stream *s)              
{                                                                                         
  u_int8_t i;                                                                             
  u_int8_t num;                                                                           
  struct pal_in4_addr *ip_addr;                                                           
                                                                                          
  /* return error if vector already exists. */                                            
  if (advt->vip)                                                                          
    return VRRP_FAILURE;                                                                  
                                                                                          
  num = (advt->num_ip_addrs) - 1;                                                         
                                                                                          
  advt->vip = vector_init (num);                                                          
  if (!advt->vip)                                                                         
    return VRRP_FAILURE;                                                                  
                                                                                          
  /* Primary VIP is already fetched. */                                                   
  for (i = 0; i <= num; i++)                                                              
    {                                                                                     
      ip_addr = XMALLOC (MTYPE_VRRP_VIP_ADDR, sizeof (struct pal_in4_addr));              
      if (!ip_addr)                                                                       
        return VRRP_FAILURE;                                                              
                                                                                          
      ip_addr->s_addr =  pal_ntoh32 (stream_getl(s));                                     
      vector_set_index (advt->vip, i, ip_addr);                                           
    }                                                                                     
                                                                                          
  return VRRP_OK;                                                                         
}                                                                                         
                                                                                          
/* To place IPv4 Virtual Ip addresses while sending advt packet. */                       
int                                                                                       
vrrp_vipaddr_vector_put_ipv4 (vector vipvec, struct stream *s)                            
{                                                                                         
  int i;                                                                                  
  struct pal_in4_addr *ip_addr;                                                           
  u_int32_t max;                                                                          
                                                                                          
  if (!vipvec)                                                                            
    return VRRP_FAILURE;                                                                  
                                                                                          
  max = vector_max (vipvec);                                                              
                                                                                          
  for (i = 0; i < max; i++)                                                               
    if ((ip_addr = vector_slot (vipvec, i)))                                              
      stream_putl (s, pal_hton32(ip_addr->s_addr));                                       
                                                                                          
  return VRRP_OK;                                                                         
}                                                                                         
                                                                                          
void                                                                                      
vrrp_vipaddr_vector_free_ipv4 (vector vipvec)                                             
{                                                                                         
  int i;                                                                                  
  u_int32_t max;                                                                          
  struct pal_in4_addr *ip_addr;                                                           
                                                                                          
  if (vipvec == NULL)                                                                     
    return;                                                                               
                                                                                          
  max = vector_max (vipvec);                                                              
                                                                                          
  for (i = 0; i <= max; i++)                                                              
    if ((ip_addr = vector_slot (vipvec, i)))                                              
      XFREE (MTYPE_VRRP_VIP_ADDR, ip_addr);                                               
                                                                                          
  vector_free (vipvec);                                                                   
}                                                                                         
                                                                                          
                                                                                          
#ifdef HAVE_IPV6                                                                          
int                                                                                       
vrrp_vipaddr_vector_get_ipv6 (struct vrrp_advt_info *advt, struct stream *s)              
{                                                                                         
  u_int8_t i;                                                                             
  u_int8_t num;                                                                           
  struct pal_in6_addr *ip6_addr;                                                          
                                                                                          
  /* return error if vector already exists. */                                            
  if (advt->vip)                                                                          
    return VRRP_FAILURE;                                                                  
                                                                                          
  num = (advt->num_ip_addrs) - 1;                                                         
                                                                                          
  advt->vip = vector_init (num);                                                          
  if (!advt->vip)                                                                         
    return VRRP_FAILURE;                                                                  
                                                                                          
  /* Primary VIP is already fetched. */                                                   
  for (i = 0; i <= num; i++)                                                              
    {                                                                                     
      ip6_addr = XMALLOC (MTYPE_VRRP_VIP_ADDR, sizeof (struct pal_in6_addr));             
      if (!ip6_addr)                                                                      
        return VRRP_FAILURE;                                                              
                                                                                          
      *ip6_addr = stream_get_ipv6 (s);                                                    
      vector_set_index (advt->vip, i, ip6_addr);                                          
    }                                                                                     
                                                                                          
  return VRRP_OK;                                                                         
}                                                                                         
                                                                                          
int                                                                                       
vrrp_vipaddr_vector_put_ipv6 (vector vipvec, struct stream *s)                            
{                                                                                         
  int i;                                                                                  
  struct pal_in6_addr *ip6_addr;                                                          
                                                                                          
  if (!vipvec)                                                                            
    return VRRP_FAILURE;                                                                  
                                                                                          
  for (i = 0; i < vector_max (vipvec); i++)                                               
    if ((ip6_addr = vector_slot (vipvec, i)))                                             
      stream_put_in6_addr (s, ip6_addr);                                                  
                                                                                          
  return VRRP_OK;                                                                         
}                                                                                         
                                                                                          
void                                                                                      
vrrp_vipaddr_vector_free_ipv6 (vector vipvec)                                             
{                                                                                         
  int i;                                                                                  
  struct pal_in6_addr *ip6_addr;                                                          
                                                                                          
  if (vipvec == NULL)                                                                     
    return;                                                                               
                                                                                          
  for (i = 0; i < vector_max (vipvec); i++)                                               
    if ((ip6_addr = vector_slot (vipvec, i)))                                             
      XFREE (MTYPE_VRRP_VIP_ADDR, ip6_addr);                                              
                                                                                          
  vector_free (vipvec);                                                                   
}                                                                                         
                                                                                          
#endif /* HAVE_IPV6 */                                                                    

