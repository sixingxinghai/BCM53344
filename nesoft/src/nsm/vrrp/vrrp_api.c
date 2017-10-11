/* Copyright (C) 2001-2009  All Rights Reserved. */

#include "pal.h"

#include "lib.h"
#include "if.h"
#include "prefix.h"
#include "vty.h"
#ifdef HAVE_SNMP
#include "snmp.h"
#endif /* HAVE_SNMP. */

#include "nsm/nsmd.h"
#include "nsm/nsm_interface.h"
#include "nsm/nsm_connected.h"
#include "nsm/vrrp/vrrp.h"
#include "nsm/vrrp/vrrp_api.h"
#include "nsm/vrrp/vrrp_snmp.h"



void
vrrp_session_stats_init (VRRP_SESSION *sess)
{
  /* statistics from vrrp header */
  vrrp_stats_checksum_err = 0;
  vrrp_stats_version_err = 0;
  vrrp_stats_invalid_vrid = 0;
  sess->stats_become_master = 0;
  sess->stats_advt_int_errors = 0;
  sess->stats_pri_zero_pkts_rcvd = 0;
  sess->stats_pri_zero_pkts_sent = 0;
  sess->stats_invalid_type_pkts_rcvd = 0;
  sess->stats_addr_list_errors = 0;
  sess->stats_invalid_auth_type = 0;
  sess->stats_advt_rcvd = 0;
  sess->stats_ip_ttl_errors = 0;
  sess->stats_pkt_len_errors = 0;
}

/*------------------------------------------------------------------------
 * vrrp_api_lookup_session() - Finds the VRRP VR session for the
 *                             vrid provided or returns NULL.
 *-----------------------------------------------------------------------
 */
VRRP_SESSION *
vrrp_api_lookup_session (int apn_vrid, u_int8_t af_type, int vrid, u_int32_t ifindex)
{
  VRRP_SESSION *sess;
  sess = vrrp_sess_tbl_lkup (apn_vrid, af_type, vrid, ifindex);
  return(sess);
}

  /*------------------------------------------------------------------------
  * vrrp_api_get_session_by_ifname() - Retrieves existing VRRP VR session
  *                                    of creates a new one.
  *-----------------------------------------------------------------------
  */
 int
 vrrp_api_get_session_by_ifname (int apn_vrid,
                                 u_int8_t af_type,
                                 int vrid,
                                 char *ifname,
                                 VRRP_SESSION **pp_sess)
 {
   VRRP_GLOBAL_DATA *vrrp=NULL;
   struct interface *ifp=NULL;

   vrrp = VRRP_GET_GLOBAL(apn_vrid);

   ifp  = if_lookup_by_name (&vrrp->nm->vr->ifm, ifname);
   if (! ifp) {
     return VRRP_API_SET_ERR_NO_SUCH_INTERFACE;
   }
   if(NSM_INTF_TYPE_L2(ifp))
     return VRRP_API_SET_ERR_L2_INTERFACE;
   *pp_sess = vrrp_api_get_session(apn_vrid,
                                   af_type,
                                   vrid,
                                   ifp->ifindex);
   return (*pp_sess != NULL) ? VRRP_OK : VRRP_API_SET_ERR_SEESION_GET_OR_CRE;
 }

/*------------------------------------------------------------------------
 * vrrp_api_get_session() - Retrieves existing VRRP VR session of creates
 *                          a new one.
 *-----------------------------------------------------------------------*/
VRRP_SESSION *
vrrp_api_get_session (int apn_vrid, u_int8_t af_type, int vrid, u_int32_t ifindex)
{
  struct interface *ifp=NULL;
  VRRP_GLOBAL_DATA *vrrp;
  VRRP_SESSION  *sess;

  sess = vrrp_sess_tbl_lkup (apn_vrid, af_type, vrid, ifindex);
  if (sess != NULL) {
    return(sess);
  }
  vrrp = VRRP_GET_GLOBAL(apn_vrid);

    ifp = if_lookup_by_index (&vrrp->nm->vr->ifm, ifindex);
    if (ifp == NULL || if_is_loopback(ifp))
      return(NULL);

  sess = vrrp_sess_tbl_create_sess(vrrp, af_type, vrid, ifindex);
  if (! sess) {
    return(NULL);
  }
  /* Set the VRID and initialize values */
  sess->state           = VRRP_STATE_INIT;
  sess->admin_state     = VRRP_ADMIN_STATE_DOWN;
  sess->shutdown_flag   = PAL_TRUE;
  sess->vip_status      = VRRP_UNSET;
  sess->owner           = PAL_FALSE;
  sess->prio            = VRRP_DEFAULT_NON_IP_OWNER_PRIORITY;  /* Per MIB */
  sess->conf_prio       = VRRP_UNSET;
  sess->adv_int         = 1;           /* 1 second - Per MIB - 100 csec */
  sess->preempt         = PAL_TRUE;
  sess->num_ip_addrs    = 1;
  sess->vrrp_uptime     = 0;
  sess->ifindex         = ifindex;

  sess->ifp           = ifp;

  if (vrrp_if_add_sess(sess) != VRRP_OK) {
    vrrp_sess_tbl_delete_sess(sess);
    return NULL;
  }

#ifdef HAVE_VRRP_LINK_ADDR
  sess->link_addr_deleted       = PAL_FALSE;
#endif /* HAVE_VRRP_LINK_ADDR */
#ifdef HAVE_SNMP
  sess->rowstatus               = SNMP_ROW_NOTREADY;
#endif /* HAVE_SNMP. */

  /* Set the Virtual MAC Address */
  sess->vmac.v.mbyte[0] = 0x00;
  sess->vmac.v.mbyte[1] = 0x00;
  sess->vmac.v.mbyte[2] = 0x5e;
  sess->vmac.v.mbyte[3] = 0x00;
  sess->vmac.v.mbyte[4] = 0x01;
#ifdef HAVE_IPV6
 if (sess->af_type == AF_INET6)
   sess->vmac.v.mbyte[4] = 0x02;
#endif /* HAVE_IPV6 */
  sess->vmac.v.mbyte[5] = vrid;

  sess->accept_mode = PAL_FALSE;
  sess->storage_type= STORAGE_TYPE_NONVOLATILE;

  vrrp_session_stats_init(sess);

  return(sess);
}

int
vrrp_api_del_session_by_ifname (int       apn_vrid,
                                u_int8_t  af_type,
                                int       vrid,
                                char     *ifname)
{
  VRRP_GLOBAL_DATA *vrrp=NULL;
  struct interface *ifp=NULL;

  vrrp = VRRP_GET_GLOBAL(apn_vrid);

  ifp  = if_lookup_by_name (&vrrp->nm->vr->ifm, ifname);
  if (! ifp) {
    return VRRP_API_SET_ERR_NO_SUCH_INTERFACE;
  }
  return vrrp_api_delete_session(apn_vrid,
                                 af_type,
                                 vrid,
                                 ifp->ifindex);
}

/*-----------------------------------------------------------------------
 * vrrp_api_delete_session() - Deletes the VRRP session.
 *                             Only disabled sessions can be deleted.
 *----------------------------------------------------------------------*/
int
vrrp_api_delete_session (int       apn_vrid,
                         u_int8_t  af_type,
                         int       vrid,
                         u_int32_t ifindex)
{
  VRRP_SESSION  *sess;

  /* Make sure vrid exists */
  if ((sess = vrrp_sess_tbl_lkup (apn_vrid, af_type, vrid, ifindex)) == NULL)
    return VRRP_API_SET_ERR_NO_EXIST;

  /* Make sure it isn't currently running */
  if (sess->state != VRRP_STATE_INIT)
    return VRRP_API_SET_ERR_ENABLED;

  if (sess->monitor_if) {
    XFREE (MTYPE_TMP, sess->monitor_if);
    sess->monitor_if = NULL;
  }
  /* Delete from the interface list */
  vrrp_if_del_sess(sess);

  vrrp_sess_tbl_delete_sess(sess);

/*
#ifdef HAVE_SNMP
   XFREE (MTYPE_VRRP_SESSION, sess);
   asso_tbl[vrid] = NULL;
#endif

*/
  return VRRP_OK;

}

/*-----------------------------------------------------------------------
 * vrrp_api_virtual_ip() - Sets/unsets the virtual IP address.
 *                         Serves both: IPv4 and IPv6
 *----------------------------------------------------------------------
 */
int
vrrp_api_virtual_ip (int       apn_vrid,
                     u_int8_t  af_type,
                     int       vrid,
                     u_int32_t ifindex,
                     u_int8_t *vip_addr,
                     bool_t    is_owner)
{
  VRRP_SESSION *sess=NULL;

#ifdef HAVE_IPV6
  if (af_type == AF_INET6) {
      if (vip_addr)
        if (!IN6_IS_ADDR_LINKLOCAL ((struct pal_in6_addr *)vip_addr))
          return VRRP_API_SET_ERR_INVALID_LINKLOCAL_ADDRESS;
    }
#endif /* HAVE_IPV6 */

  if ((sess = vrrp_sess_tbl_lkup (apn_vrid, af_type, vrid, ifindex)) == NULL) {
    return VRRP_API_SET_ERR_NO_SUCH_SESSION;
  }
  /* Make sure it isn't currently running */
  if (sess->state != VRRP_STATE_INIT)
    return VRRP_API_SET_ERR_ENABLED;

  /* Must be unset? */
  if (! vip_addr) {
    if (af_type == AF_INET) {
      pal_mem_set (&sess->vip_v4, 0, sizeof (struct pal_in4_addr));
    }
#ifdef HAVE_IPV6
    else { /* AF_INET6 */
      pal_mem_set (&sess->vip_v6, 0, sizeof (struct pal_in6_addr));
    }
#endif /* HAVE_IPV6 */
    sess->owner = PAL_FALSE;
    sess->vip_status = VRRP_UNSET;
    sess->init_msg_code = VRRP_INIT_MSG_VIP_UNSET;
    return VRRP_OK;
  }
  /* Setting a new Virtual IP address. */
  if (is_owner) {
    /* Owner priority MUST be 255. */
      sess->prio = VRRP_DEFAULT_IP_OWNER_PRIORITY;
  }
  else /* not owner */
  {
    /* Non-owner priority CANT be 255 (reset default of 100).
 Imaginary Buffer Line
    */
    if (sess->prio == VRRP_DEFAULT_IP_OWNER_PRIORITY)
      sess->conf_prio = sess->prio = VRRP_DEFAULT_NON_IP_OWNER_PRIORITY;
  }
  /* Now we can set the address */
  if (af_type == AF_INET) {
    pal_mem_cpy(&sess->vip_v4, vip_addr, sizeof (struct pal_in4_addr));
  }
#ifdef HAVE_IPV6
  else { /* AF_INET6 */
    pal_mem_cpy(&sess->vip_v6, vip_addr, sizeof (struct pal_in6_addr));
  }
#endif /* HAVE_IPV6 */

  sess->vip_status = VRRP_SET;
  sess->owner      = is_owner;
  sess->init_msg_code = VRRP_INIT_MSG_ADMIN_DOWN;

#ifdef HAVE_SNMP
  vrrp_api_enable_asso_entry (apn_vrid,
                              af_type,
                              vrid,
                              ifindex,
                              vip_addr,
                              VRRP_SNMP_ROW_STATUS_ACTIVE);
#endif /* HAVE_SNMP */

  return VRRP_OK;
}

/*-----------------------------------------------------------------------
 * vrrp_api_monitored_circuit() -  Sets the monitored circuit for a vrrp
 *                                 session.
 *----------------------------------------------------------------------*/
int
vrrp_api_monitored_circuit (int       apn_vrid,
                            u_int8_t  af_type,
                            int       vrid,
                            u_int32_t ifindex,
                            char      *if_str,
                            int       priority_delta)
{
  VRRP_SESSION  *sess;

  if ((sess = vrrp_sess_tbl_lkup (apn_vrid, af_type, vrid, ifindex)) == NULL) {
    return VRRP_API_SET_ERR_NO_SUCH_SESSION;
  }

  /* Make sure it isn't currently running */
  if (sess->state != VRRP_STATE_INIT)
    return VRRP_API_SET_ERR_ENABLED;

  if (! if_str) {
    /* NULL interface string means delete of the configuration. */
    if (sess->monitor_if)
        XFREE (MTYPE_TMP, sess->monitor_if);
    sess->monitor_if = NULL;
    sess->priority_delta = 0;
    sess->mc_status = PAL_FALSE;
    sess->mc_down_cnt = 2;
  }
  else {
    /* Update monitored circuit configuration.  */
    if (sess->monitor_if)
      XFREE (MTYPE_TMP, sess->monitor_if);
    sess->monitor_if = XSTRDUP (MTYPE_TMP, if_str);
    sess->priority_delta = priority_delta;
    sess->mc_status = PAL_FALSE;
  }
  return VRRP_OK;
}

/*-----------------------------------------------------------------------
 * vrrp_api_priority() - Sets the priority for a vrrp session.
 *----------------------------------------------------------------------*/
int
vrrp_api_priority (int       apn_vrid,
                   u_int8_t  af_type,
                   int       vrid,
                   u_int32_t ifindex,
                   int       prio)
{
  VRRP_SESSION     *sess;

  if ((sess = vrrp_sess_tbl_lkup (apn_vrid, af_type, vrid, ifindex)) == NULL) {
    return VRRP_API_SET_ERR_NO_SUCH_SESSION;
  }
  /* Make sure it isn't currently running */
  if (sess->state != VRRP_STATE_INIT)
    return VRRP_API_SET_ERR_ENABLED;
  else {
    /* Make sure virtual IP address is set already */
    if (sess->vip_status == VRRP_UNSET)
      return VRRP_API_SET_ERR_VIP_UNSET;
    else
      if (prio == VRRP_DEFAULT_IP_OWNER_PRIORITY
          && sess->owner != PAL_TRUE)
        return VRRP_API_SET_ERR_PRIO_CANT_255;
      else
        if (prio != VRRP_DEFAULT_IP_OWNER_PRIORITY
            && sess->owner == PAL_TRUE)
          return VRRP_API_SET_ERR_PRIO_MUST_255;
        else
          sess->conf_prio = sess->prio = prio;
  }
  return VRRP_OK;
}

/*-----------------------------------------------------------------------
 * vrrp_api_unset_priority() - Sets the priority to default.
 *----------------------------------------------------------------------*/
int
vrrp_api_unset_priority (int       apn_vrid,
                         u_int8_t  af_type,
                         int       vrid,
                         u_int32_t ifindex)
{
  VRRP_SESSION  *sess;

  if ((sess = vrrp_sess_tbl_lkup  (apn_vrid, af_type, vrid, ifindex)) == NULL) {
    return(VRRP_API_SET_ERR_NO_SUCH_SESSION);
  }
  /* Make sure it isn't currently running */
  if (sess->state != VRRP_STATE_INIT)
    return VRRP_API_SET_ERR_ENABLED;
  else {
    /* Make sure virtual IP address is set already */
    if (sess->vip_status == VRRP_UNSET)
      return VRRP_API_SET_ERR_VIP_UNSET;
    else
      VRRP_SET_DEFAULT_PRIORITY (sess);
  }
  return VRRP_OK;
}


/*-----------------------------------------------------------------------
 * vrrp_api_advt_interval() - Sets the advt interval for a vrrp session.
 *----------------------------------------------------------------------*/
int
vrrp_api_advt_interval (int       apn_vrid,
                        u_int8_t  af_type,
                        int       vrid,
                        u_int32_t ifindex,
                        int interval)
{
  VRRP_SESSION     *sess;

  if ((sess = vrrp_sess_tbl_lkup  (apn_vrid, af_type, vrid, ifindex)) == NULL) {
    return(VRRP_API_SET_ERR_NO_SUCH_SESSION);
  }
  /* Make sure it isn't currently running */
  if (sess->state != VRRP_STATE_INIT)
    return VRRP_API_SET_ERR_ENABLED;
  else
    sess->adv_int = interval;

  return VRRP_OK;
}

/*-----------------------------------------------------------------------
 * vrrp_api_unset_advt_interval() - Sets the advt interval to default.
 *----------------------------------------------------------------------*/
int
vrrp_api_unset_advt_interval (int       apn_vrid,
                              u_int8_t  af_type,
                              int       vrid,
                              u_int32_t ifindex)
{
  VRRP_SESSION     *sess;

  if ((sess = vrrp_sess_tbl_lkup (apn_vrid, af_type, vrid, ifindex)) == NULL) {
    return(VRRP_API_SET_ERR_NO_SUCH_SESSION);
  }
  /* Make sure it isn't currently running */
  if (sess->state != VRRP_STATE_INIT)
    return VRRP_API_SET_ERR_ENABLED;
  else
    sess->adv_int = VRRP_ADVT_INT_DFLT;

  return VRRP_OK;
}


/*-----------------------------------------------------------------------
 * vrrp_api_preempt_mode() - Sets the preempt mode for a vrrp session.
 *----------------------------------------------------------------------*/
int
vrrp_api_preempt_mode (int       apn_vrid,
                       u_int8_t  af_type,
                       int       vrid,
                       u_int32_t ifindex,
                       int mode)
{
  VRRP_SESSION     *sess;

  if ((sess = vrrp_sess_tbl_lkup (apn_vrid, af_type, vrid, ifindex)) == NULL) {
    return(VRRP_API_SET_ERR_NO_SUCH_SESSION);
  }
  /* Make sure it isn't currently running */
  if (sess->state != VRRP_STATE_INIT)
    return VRRP_API_SET_ERR_ENABLED;
  else {
    if (mode == 1)
      sess->preempt = PAL_TRUE;
    else
      sess->preempt = PAL_FALSE;
  }
  return VRRP_OK;
}


/*-----------------------------------------------------------------------
 * vrrp_api_enable_session() - Enables a vrrp session.
 *----------------------------------------------------------------------*/
int
vrrp_api_enable_session (int       apn_vrid,
                         u_int8_t  af_type,
                         int       vrid,
                         u_int32_t ifindex)
{
  int           ret;
  VRRP_SESSION *sess;

  if ((sess = vrrp_sess_tbl_lkup (apn_vrid, af_type, vrid, ifindex)) == NULL) {
    return(VRRP_API_SET_ERR_NO_SUCH_SESSION);
  }
  /* Make sure it isn't currently running */
  if (sess->admin_state == VRRP_ADMIN_STATE_UP)
    return VRRP_API_SET_ERR_ENABLED;

  /* Make sure IP addresses are set */
  if (sess->vip_status == VRRP_UNSET)
    {
      sess->init_msg_code = VRRP_INIT_MSG_VIP_UNSET;
    return VRRP_API_SET_ERR_CONFIG_UNSET;
    }

  /* Set any defaults that haven't been configured */
  if (sess->prio == VRRP_UNSET)
    VRRP_SET_DEFAULT_PRIORITY (sess);

  /* Check the priority / owner status. */
  if ((sess->prio == VRRP_DEFAULT_IP_OWNER_PRIORITY
       && sess->owner == PAL_FALSE)
      || (sess->prio < VRRP_DEFAULT_IP_OWNER_PRIORITY
          && sess->owner == PAL_TRUE))
    return VRRP_API_SET_ERR_PRIO_MISMATCH;

  if (sess->adv_int == VRRP_UNSET)
    sess->adv_int = VRRP_ADVT_INT_DFLT;

  /* Calculate the Skew_Time and Master_Down_Interval */
  /*    sess->skew_time = ( (256 - sess->prio) / 256 ); */
  if (sess->prio > 128)
    sess->skew_time = 0;
  else
    sess->skew_time = 1;
  sess->master_down_int = (3 * sess->adv_int) + sess->skew_time;

  sess->admin_state = VRRP_ADMIN_STATE_UP;

  /* Initialize the session */
  ret = vrrp_enable_sess (sess);
  if (ret != VRRP_OK) {
    sess->state       = VRRP_STATE_INIT;
    sess->admin_state = VRRP_ADMIN_STATE_DOWN;
    return VRRP_API_SET_ERR_ENABLE;
  }
  sess->vrrp_uptime = pal_time_current(NULL);

#ifdef HAVE_SNMP
  sess->rowstatus = SNMP_ROW_ACTIVE;
#endif /* HAVE_SNMP. */
  return VRRP_OK;
}

/*-----------------------------------------------------------------------
 * vrrp_api_disable_session() - Disables a vrrp session.
 *----------------------------------------------------------------------*/
int
vrrp_api_disable_session (int       apn_vrid,
                          u_int8_t  af_type,
                          int       vrid,
                          u_int32_t ifindex)
{
  VRRP_SESSION *sess;

  if ((sess = vrrp_sess_tbl_lkup (apn_vrid, af_type, vrid, ifindex)) == NULL) {
    return(VRRP_API_SET_ERR_NO_SUCH_SESSION);
  }
  /* Make sure it is currently running
  */
  if (sess->admin_state == VRRP_ADMIN_STATE_DOWN)
    return VRRP_API_SET_ERR_DISABLED;

  /* Shutdown cannot fail. Bring the session to Init state.
  */
  if (sess->state != VRRP_STATE_INIT)
    {
      vrrp_shutdown_sess (sess);
    }
  sess->admin_state = VRRP_ADMIN_STATE_DOWN;
  sess->init_msg_code = VRRP_INIT_MSG_ADMIN_DOWN;
  sess->vrrp_uptime = 0;
  return VRRP_OK;
}


int
vrrp_api_walk_sessions (int     apn_vrid,
                        void   *ref,
                        vrrp_api_sess_walk_fun_t efun,
                        int    *num_sess)
{
  return vrrp_sess_tbl_walk(apn_vrid,ref,efun,num_sess);
}

/*----------------------------------------------------------------------------
 * vrrp_api_check_any_sess_master - Finds if there is a VRRP session
 *                                  in Master state
 *---------------------------------------------------------------------------*/
static int
_vrrp_check_sess_master(VRRP_SESSION *sess, void *ref)
{
  PAL_UNREFERENCED_PARAMETER(ref);

  if (sess->state == VRRP_STATE_MASTER)
    return VRRP_FOUND;
  return VRRP_IGNORE;
}

int
vrrp_api_check_any_sess_master (int apn_vrid)
{
  int rc;

  rc = vrrp_sess_tbl_walk(apn_vrid, NULL, _vrrp_check_sess_master, NULL);
  if (rc == VRRP_FOUND)
    return VRRP_OK;
  return VRRP_FAILURE;
}


/*----------------------------------------------------------------------------
 * vrrp_api_set_vmac_status - Setting VMAC status to enabled or disabled.
 *---------------------------------------------------------------------------*/
int
vrrp_api_set_vmac_status(int new_vmac_status)
{
  int cur_vmac_status;
  int ret;

  cur_vmac_status = vrrp_fe_get_vmac_status();

  /* No offence, if the status is already set. */
  if (new_vmac_status == cur_vmac_status)
    return VRRP_OK;

  /* Fail, if any session in master state.
     Alternatively, we shall shutdown all the sessions, set the new VMAC
     state, and enable the sessions back.
  */
  ret = vrrp_api_check_any_sess_master(0);
  if (ret == VRRP_OK)
    return VRRP_API_MASTER_FOUND;

  vrrp_fe_set_vmac_status (new_vmac_status);

  return VRRP_OK;
}

/**************************************************************
 *                      S N M P
 **************************************************************
 */

/* SNMP TRAP Callback API. */
#ifdef HAVE_SNMP


/*

bool_t
vrrp_check_address (struct interface *ifp, struct pal_in4_addr *addr)
{
  struct nsm_master *nm = ifp->vr->proto;
  struct vrrp_global *vrrp = nm->vrrp;
  int i;
  VRRP_SESSION *sess = NULL;

  for (i = 1; i <= MAX_VRIDS; i++)
    {
      if ((sess = vrrp->vs_tbl[i]) == NULL)
        continue;

      if (sess->ifp && sess->ifp == ifp)
        if (sess->vip.s_addr == addr->s_addr)
          return PAL_TRUE;
    }
  return PAL_FALSE;
}
*/
/*
bool_t
vrrp_match_interface_vip (VRRP_SESSION *sess,
                          struct pal_in4_addr *addr, struct interface *ifp)
{
  struct interface *t_ifp;
  struct nsm_master *nm = sess->vrrp->nm;

  t_ifp = if_lookup_by_ipv4_address (&nm->vr->ifm, addr);
  if (t_ifp != ifp)
    return PAL_FALSE;

  return PAL_TRUE;
}
*/
/*
bool_t
vrrp_api_check_asso_ipaddr (struct vrrp_global *vrrp, int vrid)
{
  VRRP_SESSION *sess = NULL;
  struct interface *ifp = NULL;
  sess = vrrp_sess_tbl_lkup(apn_vrid, af_type, vrid, ifindex);

  if (sess == NULL || asso_tbl[vrid] == NULL)
    return PAL_FALSE;

  ifp = if_lookup_by_index (&vrrp->nm->vr->ifm, asso_tbl[vrid]->ifindex);
  if (sess->ifp && sess->ifp == ifp)
    if (sess->vip.s_addr == asso_tbl[vrid]->assoipaddr.s_addr)
      return PAL_TRUE;

  return PAL_FALSE;
}
*/
int
vrrp_api_enable_asso_entry(int apn_vrid,
                           u_int8_t  af_type,
                           u_int8_t  vrid,
                           u_int32_t ifindex,
                           u_int8_t  *ipaddr,
                           int row_status)
{
  VRRP_ASSO *asso = vrrp_asso_tbl_lkup(apn_vrid,
                                       af_type,
                                       vrid,
                                       ifindex,
                                       ipaddr);
  if(asso == NULL) {
    asso = vrrp_asso_tbl_create_asso (VRRP_GET_GLOBAL(apn_vrid),
                                      af_type,
                                      vrid,
                                      ifindex,
                                      ipaddr);
  }
  if (asso != NULL) {
    if (asso->asso_rowstatus != row_status) {
      asso->asso_rowstatus = row_status;
    }
    return VRRP_OK;
  }
  return VRRP_FAILURE;
}

int
vrrp_get_version (int apn_vrid, int *val)
{
  *val = VRRP_IPV6_VERSION;
  return VRRP_API_GET_SUCCESS;
}

int
vrrp_get_notify (int apn_vrid, int *val)
{
  VRRP_GLOBAL_DATA *vrrp = VRRP_GET_GLOBAL(apn_vrid);
  if (vrrp != NULL) {
    *val = vrrp->notificationcntl;
    return VRRP_API_GET_SUCCESS;
  }
  else
    return VRRP_API_GET_ERROR;
}

int
vrrp_get_oper_virtual_macaddr (int       apn_vrid,
                               u_int8_t  aftype,
                               u_int8_t  vrid,
                               u_int32_t ifindex,
                               struct vmac_addr *vmac)
{
  int i = 0;
  VRRP_SESSION *sess = vrrp_sess_tbl_lkup(apn_vrid, aftype, vrid, ifindex);
  if (sess != NULL) {
    for (i = 0; i< SIZE_VMAC_ADDR; i++) {
      vmac->v.mbyte[i] = sess->vmac.v.mbyte[i];
    }
    return VRRP_API_GET_SUCCESS;
  }
  else
    return VRRP_API_GET_ERROR;
}
int
vrrp_get_next_oper_virtual_macaddr (int        apn_vrid,
                                    u_int8_t  *aftype,
                                    u_int8_t  *vrid,
                                    u_int32_t *ifindex,
                                    int       len,
                                    struct vmac_addr *vmac)
{
  int i = 0;
  VRRP_SESSION *sess = vrrp_sess_tbl_lkup_next (apn_vrid, *aftype, *vrid, *ifindex);
  if (sess != NULL) {
    *aftype = sess->af_type;
    *ifindex = sess->ifindex;
    *vrid    = sess->vrid;
    for (i = 0; i< SIZE_VMAC_ADDR; i++) {
      vmac->v.mbyte[i] = sess->vmac.v.mbyte[i];
    }
    return VRRP_API_GET_SUCCESS;
  }
  else
    return VRRP_API_GET_ERROR;
}

int
vrrp_get_oper_state (int       apn_vrid,
                     u_int8_t  aftype,
                     u_int8_t  vrid,
                     u_int32_t ifindex,
                     int *val)
{
  VRRP_SESSION *sess =vrrp_sess_tbl_lkup(apn_vrid, aftype, vrid, ifindex);
  if (sess != NULL) {
    *val = sess->state;
    return VRRP_API_GET_SUCCESS;
  }
  else
    return VRRP_API_GET_ERROR;
}
int
vrrp_get_next_oper_state (int        apn_vrid,
                          u_int8_t  *aftype,
                          u_int8_t  *vrid,
                          u_int32_t *ifindex,
                          int len,
                          int *val)
{
  VRRP_SESSION *sess = vrrp_sess_tbl_lkup_next (apn_vrid, *aftype, *vrid, *ifindex);
  if (sess != NULL) {
    *aftype = sess->af_type;
    *ifindex = sess->ifindex;
    *vrid    = sess->vrid;
    *val = sess->state;
    return VRRP_API_GET_SUCCESS;
  }
  else
    return VRRP_API_GET_ERROR;
}

int
vrrp_get_oper_priority (int       apn_vrid,
                        u_int8_t  aftype,
                        u_int8_t  vrid,
                        u_int32_t ifindex,
                        int *val)
{
  VRRP_SESSION *sess = vrrp_sess_tbl_lkup(apn_vrid, aftype, vrid, ifindex);
  if (sess != NULL) {
    *val = (sess->prio);
    return VRRP_API_GET_SUCCESS;
  }
  else
    return VRRP_API_GET_ERROR;
}

int
vrrp_get_next_oper_priority (int        apn_vrid,
                             u_int8_t  *aftype,
                             u_int8_t  *vrid,
                             u_int32_t *ifindex,
                             int len,
                             int *val)
{
  VRRP_SESSION *sess = vrrp_sess_tbl_lkup_next (apn_vrid, *aftype, *vrid, *ifindex);
  if (sess != NULL) {
    *aftype = sess->af_type;
    *ifindex = sess->ifindex;
    *vrid    = sess->vrid;
    *val     = sess->prio;
    return VRRP_API_GET_SUCCESS;
  }
  else
    return VRRP_API_GET_ERROR;
  return 0;
}

int
vrrp_get_oper_addr_count (int       apn_vrid,
                          u_int8_t  aftype,
                          u_int8_t  vrid,
                          u_int32_t ifindex,
                          int *val)
{
  VRRP_SESSION *sess = vrrp_sess_tbl_lkup(apn_vrid, aftype, vrid, ifindex);
  if (sess != NULL) {
    *val = sess->num_ip_addrs;
    return VRRP_API_GET_SUCCESS;
  }
  else
    return VRRP_API_GET_ERROR;
}
int
vrrp_get_next_oper_addr_count (int        apn_vrid,
                               u_int8_t  *aftype,
                               u_int8_t  *vrid,
                               u_int32_t *ifindex,
                               int len,
                               int *val)
{
  VRRP_SESSION *sess = vrrp_sess_tbl_lkup_next (apn_vrid, *aftype, *vrid, *ifindex);
  if (sess != NULL) {
    *aftype = sess->af_type;
    *ifindex = sess->ifindex;
    *vrid    = sess->vrid;
    *val     = sess->num_ip_addrs;
    return VRRP_API_GET_SUCCESS;
  }
  else
    return VRRP_API_GET_ERROR;
}

int
vrrp_get_oper_master_ipaddr (int       apn_vrid,
                             u_int8_t  aftype,
                             u_int8_t  vrid,
                             u_int32_t ifindex,
                             u_int8_t *ipaddr)
{
  VRRP_SESSION *sess = vrrp_sess_tbl_lkup(apn_vrid, aftype, vrid, ifindex);
  if (sess != NULL) {
    if (aftype == AF_INET) {
      pal_mem_cpy(ipaddr, &sess->vip_v4, IN_ADDR_SIZE);
    }
#ifdef HAVE_IPV6
    else {
      pal_mem_cpy(ipaddr, &sess->vip_v6, IN6_ADDR_SIZE);
    }
#endif
    return VRRP_API_GET_SUCCESS;
  }
  else
    return  VRRP_API_GET_ERROR;
}
int
vrrp_get_next_oper_master_ipaddr (int        apn_vrid,
                                  u_int8_t  *aftype,
                                  u_int8_t  *vrid,
                                  u_int32_t *ifindex,
                                  int len,
                                  u_int8_t *ipaddr)
{
  VRRP_SESSION *sess= vrrp_sess_tbl_lkup_next (apn_vrid, *aftype, *vrid, *ifindex);
  if (sess != NULL) {
    *aftype = sess->af_type;
    *ifindex = sess->ifindex;
    *vrid    = sess->vrid;
    if (sess->af_type == AF_INET) {
      pal_mem_cpy(ipaddr, &sess->vip_v4, IN_ADDR_SIZE);
    }
#ifdef HAVE_IPV6
    else {
      pal_mem_cpy(ipaddr, &sess->vip_v6, IN6_ADDR_SIZE);
    }
#endif
    return VRRP_API_GET_SUCCESS;
  }
  else
    return  VRRP_API_GET_ERROR;
}

int
vrrp_get_oper_primary_ipaddr (int       apn_vrid,
                              u_int8_t  aftype,
                              u_int8_t  vrid,
                              u_int32_t ifindex,
                              u_int8_t *ipaddr)
{
  VRRP_SESSION *sess = vrrp_sess_tbl_lkup(apn_vrid, aftype, vrid, ifindex);
  if (sess != NULL) {
    if (sess->af_type == AF_INET) {
      pal_mem_cpy(ipaddr, &sess->vip_v4, IN_ADDR_SIZE);
    }
#ifdef HAVE_IPV6
    else {
      pal_mem_cpy(ipaddr, &sess->vip_v6, IN6_ADDR_SIZE);
    }
#endif
    return VRRP_API_GET_SUCCESS;
  }
  else
    return VRRP_API_GET_ERROR;
}

int
vrrp_get_next_oper_primary_ipaddr (int        apn_vrid,
                                   u_int8_t  *aftype,
                                   u_int8_t  *vrid,
                                   u_int32_t *ifindex,
                                   int len,
                                   u_int8_t  *ipaddr)
{
  VRRP_SESSION *sess = vrrp_sess_tbl_lkup_next (apn_vrid, *aftype, *vrid, *ifindex);
  if (sess != NULL) {
    *aftype = sess->af_type;
    *ifindex = sess->ifindex;
    *vrid    = sess->vrid;
    if (sess->af_type == AF_INET) {
      pal_mem_cpy(ipaddr, &sess->vip_v4, IN_ADDR_SIZE);
    }
#ifdef HAVE_IPV6
    else {
      pal_mem_cpy(ipaddr, &sess->vip_v6, IN6_ADDR_SIZE);
    }
#endif
    return VRRP_API_GET_SUCCESS;
  }
  else
    return VRRP_API_GET_ERROR;
}

int
vrrp_get_oper_adv_interval (int       apn_vrid,
                            u_int8_t  aftype,
                            u_int8_t  vrid,
                            u_int32_t ifindex,
                            int *val)
{
  VRRP_SESSION *sess = vrrp_sess_tbl_lkup(apn_vrid, aftype, vrid, ifindex);
  if (sess != NULL) {
    *val = sess->adv_int*100;  /* Convert to centiseconds. */
    return VRRP_API_GET_SUCCESS;
  }
  else
    return VRRP_API_GET_ERROR;
}

int
vrrp_get_next_oper_adv_interval (int        apn_vrid,
                                 u_int8_t  *aftype,
                                 u_int8_t  *vrid,
                                 u_int32_t *ifindex,
                                 int len,
                                 int *val)
{
  VRRP_SESSION *sess = vrrp_sess_tbl_lkup_next (apn_vrid, *aftype, *vrid, *ifindex);
  if (sess != NULL) {
    *aftype = sess->af_type;
    *ifindex = sess->ifindex;
    *vrid    = sess->vrid;
    *val = sess->adv_int*100;
    return VRRP_API_GET_SUCCESS;
  }
  else
    return VRRP_API_GET_ERROR;
}

int
vrrp_get_oper_preempt_mode (int       apn_vrid,
                            u_int8_t  aftype,
                            u_int8_t  vrid,
                            u_int32_t ifindex,
                            int *val)
{
  VRRP_SESSION *sess = vrrp_sess_tbl_lkup(apn_vrid, aftype, vrid, ifindex);
  if (sess != NULL) {
    *val = sess->preempt;
    return VRRP_API_GET_SUCCESS;
  }
  else
    return VRRP_API_GET_ERROR;
}

int
vrrp_get_next_oper_preempt_mode (int        apn_vrid,
                                 u_int8_t  *aftype,
                                 u_int8_t  *vrid,
                                 u_int32_t *ifindex,
                                 int len,
                                 int *val)
{
  VRRP_SESSION *sess= vrrp_sess_tbl_lkup_next (apn_vrid, *aftype, *vrid, *ifindex);
  if (sess != NULL) {
    *aftype = sess->af_type;
    *ifindex = sess->ifindex;
    *vrid    = sess->vrid;
    *val = sess->preempt;
    return VRRP_API_GET_SUCCESS;
  }
  else
    return VRRP_API_GET_ERROR;
}

int
vrrp_get_oper_uptime (int       apn_vrid,
                      u_int8_t  aftype,
                      u_int8_t  vrid,
                      u_int32_t ifindex,
                      u_int32_t *val)
{
  VRRP_SESSION *sess = vrrp_sess_tbl_lkup(apn_vrid, aftype, vrid, ifindex);
  if (sess != NULL) {
    /*RFC2787 sysUpTime in hundredth of seconds when vrrp
     * transition out of init state */
    if (sess->state > VRRP_STATE_INIT)
      *val = 100 *(pal_time_current (NULL) - sess->vrrp_uptime);
    else
      *val = 0;
    return VRRP_API_GET_SUCCESS;
  }
  else
    return VRRP_API_GET_ERROR;
}

int
vrrp_get_next_oper_uptime (int        apn_vrid,
                           u_int8_t  *aftype,
                           u_int8_t  *vrid,
                           u_int32_t *ifindex,
                           int len,
                           u_int32_t *val)
{
  VRRP_SESSION *sess = vrrp_sess_tbl_lkup_next (apn_vrid, *aftype, *vrid, *ifindex);
  if (sess != NULL) {
    *aftype = sess->af_type;
    *ifindex = sess->ifindex;
    *vrid    = sess->vrid;
    if (sess->state > VRRP_STATE_INIT)
      *val = 100 *(pal_time_current (NULL) - sess->vrrp_uptime);
    else
      *val = 0;
    return VRRP_API_GET_SUCCESS;
  }
  else
    return VRRP_API_GET_ERROR;
}

int
vrrp_get_oper_accept_mode (int       apn_vrid,
                           u_int8_t  aftype,
                           u_int8_t  vrid,
                           u_int32_t ifindex,
                           u_int32_t *val)
{
  VRRP_SESSION *sess = vrrp_sess_tbl_lkup(apn_vrid, aftype, vrid, ifindex);
  if (sess != NULL) {
    *val = sess->accept_mode;
    return VRRP_API_GET_SUCCESS;
  }
  else
    return VRRP_API_GET_ERROR;
}

int
vrrp_get_next_oper_accept_mode (int        apn_vrid,
                                u_int8_t  *aftype,
                                u_int8_t  *vrid,
                                u_int32_t *ifindex,
                                int len,
                                u_int32_t *val)
{
  VRRP_SESSION *sess = vrrp_sess_tbl_lkup_next (apn_vrid, *aftype, *vrid, *ifindex);

  if (sess != NULL) {
    *aftype = sess->af_type;
    *ifindex = sess->ifindex;
    *vrid    = sess->vrid;
    *val     = sess->accept_mode;
    return VRRP_API_GET_SUCCESS;
  }
  else
    return VRRP_API_GET_ERROR;
}

int
vrrp_get_oper_storage_type (int       apn_vrid,
                            u_int8_t  aftype,
                            u_int8_t  vrid,
                            u_int32_t ifindex,
                            u_int32_t *val)
{
  VRRP_SESSION *sess = vrrp_sess_tbl_lkup(apn_vrid, aftype, vrid, ifindex);
  if (sess != NULL) {
    *val = sess->storage_type;
    return VRRP_API_GET_SUCCESS;
  }
  else
    return VRRP_API_GET_ERROR;
}

int
vrrp_get_next_oper_storage_type (int       apn_vrid,
                                 u_int8_t  *aftype,
                                 u_int8_t  *vrid,
                                 u_int32_t *ifindex,
                                 int        len,
                                 u_int32_t *val)
{
  VRRP_SESSION *sess = vrrp_sess_tbl_lkup_next (apn_vrid, *aftype, *vrid, *ifindex);

  if (sess != NULL) {
    *aftype = sess->af_type;
    *ifindex = sess->ifindex;
    *vrid    = sess->vrid;
    *val     = sess->storage_type;
    return VRRP_API_GET_SUCCESS;
  }
  else
    return VRRP_API_GET_ERROR;
}

int
vrrp_get_oper_rowstatus (int       apn_vrid,
                         u_int8_t  aftype,
                         u_int8_t  vrid,
                         u_int32_t ifindex,
                         int *val)
{
  VRRP_SESSION *sess = vrrp_sess_tbl_lkup(apn_vrid, aftype, vrid, ifindex);
  if (sess != NULL) {
    *val = sess->rowstatus;
    return VRRP_API_GET_SUCCESS;
  }
  return VRRP_API_GET_ERROR;
}

int
vrrp_get_next_oper_rowstatus (int        apn_vrid,
                              u_int8_t  *aftype,
                              u_int8_t  *vrid,
                              u_int32_t *ifindex,
                              int len,
                              int *val)
{
  VRRP_SESSION *sess= vrrp_sess_tbl_lkup_next (apn_vrid, *aftype, *vrid, *ifindex);
  if (sess != NULL) {
    *aftype = sess->af_type;
    *ifindex = sess->ifindex;
    *vrid    = sess->vrid;
    *val = sess->rowstatus;
    return VRRP_API_GET_SUCCESS;
  }

  return VRRP_API_GET_ERROR;
}

/******************************************************************/

int
vrrp_get_asso_storage_type (int       apn_vrid,
                            u_int8_t  aftype,
                            u_int8_t  vrid,
                            u_int32_t ifindex,
                            u_int8_t *ipaddr,
                            u_int32_t *val)
{
  VRRP_ASSO *asso = vrrp_asso_tbl_lkup(apn_vrid, aftype, vrid, ifindex, ipaddr);
  if (asso != NULL) {
    *val = asso->asso_storage_type;
    return VRRP_API_GET_SUCCESS;
  }
  else
    return VRRP_API_GET_ERROR;
}

int
vrrp_get_next_asso_storage_type (int        apn_vrid,
                                 u_int8_t  *aftype,
                                 u_int8_t  *vrid,
                                 u_int32_t *ifindex,
                                 u_int8_t *ipaddr,
                                 int len,
                                 u_int32_t *val)
{
  VRRP_ASSO *asso = vrrp_asso_tbl_lkup_next (apn_vrid, *aftype, *vrid, *ifindex, ipaddr);

  if (asso != NULL) {
    *aftype = asso->af_type;
    *ifindex = asso->ifindex;
    *vrid    = asso->vrid;
    if (asso->af_type == AF_INET) {
      pal_mem_cpy(ipaddr, &asso->ipad_v4, IN_ADDR_SIZE);
    }
#ifdef HAVE_IPV6
    else {
      pal_mem_cpy(ipaddr, &asso->ipad_v6, IN6_ADDR_SIZE);
    }
#endif
    *val = asso->asso_storage_type;
    return VRRP_API_GET_SUCCESS;
  }
  else
    return VRRP_API_GET_ERROR;
}

int
vrrp_get_asso_ipaddr_rowstatus (int       apn_vrid,
                                u_int8_t  aftype,
                                u_int8_t  vrid,
                                u_int32_t ifindex,
                                u_int8_t *ipaddr,
                                int      *val)
{
  VRRP_ASSO *asso= vrrp_asso_tbl_lkup (apn_vrid, aftype, ifindex, vrid, ipaddr);
  if (asso != NULL) {
    *val = asso->asso_rowstatus;
    return VRRP_API_GET_SUCCESS;
  }
  return VRRP_API_GET_ERROR;
}

int
vrrp_get_next_asso_ipaddr_rowstatus (int        apn_vrid,
                                     u_int8_t  *aftype,
                                     u_int8_t  *vrid,
                                     u_int32_t *ifindex,
                                     u_int8_t  *ipaddr,
                                     int len,
                                     int *val)
{
  VRRP_ASSO *asso = vrrp_asso_tbl_lkup_next (apn_vrid, *aftype, *vrid, *ifindex, ipaddr);
  if (asso != NULL) {
    *aftype = asso->af_type;
    *vrid    = asso->vrid;
    *ifindex = asso->ifindex;
    if (asso->af_type == AF_INET) {
      pal_mem_cpy(ipaddr, &asso->ipad_v4, IN_ADDR_SIZE);
    }
 #ifdef HAVE_IPV6
    else {
      pal_mem_cpy(ipaddr, &asso->ipad_v6, IN6_ADDR_SIZE);
    }
 #endif
    *val = asso->asso_rowstatus;
    return VRRP_API_GET_SUCCESS;
  }
  return VRRP_API_GET_ERROR;
}

/***********************************************************************/

int
vrrp_get_checksum_errors (int apn_vrid, int *val)
{
  *val = vrrp_stats_checksum_err;
  return VRRP_API_GET_SUCCESS;
}

int
vrrp_get_version_errors (int apn_vrid, int *val)
{
  *val = vrrp_stats_version_err;
  return VRRP_API_GET_SUCCESS;
}

int
vrrp_get_vrid_errors (int apn_vrid, int *val)
{
  *val = vrrp_stats_invalid_vrid;
  return VRRP_API_GET_SUCCESS;
}

int
vrrp_get_stats_master_transitions (int       apn_vrid,
                                   u_int8_t  aftype,
                                   u_int8_t  vrid,
                                   u_int32_t ifindex,
                                   int *val)
{
  VRRP_SESSION *sess = vrrp_sess_tbl_lkup(apn_vrid, aftype, vrid, ifindex);
  if (sess != NULL) {
    *val = sess->stats_become_master;
    return VRRP_API_GET_SUCCESS;
  }

  return VRRP_API_GET_ERROR;
}

int
vrrp_get_next_stats_master_transitions (int        apn_vrid,
                                        u_int8_t  *aftype,
                                        u_int8_t  *vrid,
                                        u_int32_t *ifindex,
                                        int len,
                                        int *val)
{
  VRRP_SESSION *sess = vrrp_sess_tbl_lkup_next (apn_vrid, *aftype, *vrid, *ifindex);

  if (sess != NULL) {
    *aftype = sess->af_type;
    *ifindex = sess->ifindex;
    *vrid    = sess->vrid;
    *val = sess->stats_become_master;
    return VRRP_API_GET_SUCCESS;
  }
  return VRRP_API_GET_ERROR;
}

int
vrrp_get_stats_rcvd_advertisements (int       apn_vrid,
                                    u_int8_t  aftype,
                                    u_int8_t  vrid,
                                    u_int32_t ifindex,
                                    int *val)
{
  VRRP_SESSION *sess = vrrp_sess_tbl_lkup(apn_vrid, aftype, vrid, ifindex);
  if (sess != NULL) {
    *val = sess->stats_advt_rcvd;
    return VRRP_API_GET_SUCCESS;
  }

  return VRRP_API_GET_ERROR;
}

int
vrrp_get_next_stats_rcvd_advertisements (int        apn_vrid,
                                         u_int8_t  *aftype,
                                         u_int8_t  *vrid,
                                         u_int32_t *ifindex,
                                         int len,
                                         int *val)
{
  VRRP_SESSION *sess = vrrp_sess_tbl_lkup_next (apn_vrid, *aftype, *vrid, *ifindex);

  if (sess != NULL) {
    *aftype = sess->af_type;
    *ifindex = sess->ifindex;
    *vrid    = sess->vrid;
    *val = sess->stats_advt_rcvd;
    return VRRP_API_GET_SUCCESS;
  }

  return VRRP_API_GET_ERROR;
}

int
vrrp_get_stats_adv_interval_errors (int       apn_vrid,
                                    u_int8_t  aftype,
                                    u_int8_t  vrid,
                                    u_int32_t ifindex,
                                    int *val)
{
  VRRP_SESSION *sess = vrrp_sess_tbl_lkup(apn_vrid, aftype, vrid, ifindex);
  if (sess != NULL) {
    *val = sess->stats_advt_int_errors;
    return VRRP_API_GET_SUCCESS;
  }

  return VRRP_API_GET_ERROR;
}

int
vrrp_get_next_stats_adv_interval_errors (int        apn_vrid,
                                         u_int8_t  *aftype,
                                         u_int8_t  *vrid,
                                         u_int32_t *ifindex,
                                         int len,
                                         int *val)
{
  VRRP_SESSION *sess = vrrp_sess_tbl_lkup_next (apn_vrid, *aftype, *vrid, *ifindex);
  if (sess != NULL) {
    *aftype = sess->af_type;
    *ifindex = sess->ifindex;
    *vrid    = sess->vrid;
    *val = sess->stats_advt_int_errors;
    return VRRP_API_GET_SUCCESS;
  }

  return VRRP_API_GET_ERROR;
}

int
vrrp_get_stats_ip_ttl_errors (int       apn_vrid,
                              u_int8_t  aftype,
                              u_int8_t  vrid,
                              u_int32_t ifindex,
                              int *val)
{
  VRRP_SESSION *sess = vrrp_sess_tbl_lkup(apn_vrid, aftype, vrid, ifindex);
  if (sess != NULL) {
    *val = sess->stats_ip_ttl_errors;
    return VRRP_API_GET_SUCCESS;
  }

  return VRRP_API_GET_ERROR;
}

int
vrrp_get_next_stats_ip_ttl_errors (int        apn_vrid,
                                   u_int8_t  *aftype,
                                   u_int8_t  *vrid,
                                   u_int32_t *ifindex,
                                   int len,
                                   int *val)
{
  VRRP_SESSION *sess;
  sess = vrrp_sess_tbl_lkup_next (apn_vrid, *aftype, *vrid, *ifindex);

  if (sess != NULL) {
    *aftype = sess->af_type;
    *ifindex = sess->ifindex;
    *vrid    = sess->vrid;
    *val = sess->stats_ip_ttl_errors;
    return VRRP_API_GET_SUCCESS;
  }
  return VRRP_API_GET_ERROR;
}

int
vrrp_get_stats_rcvd_pri_zero_packets (int       apn_vrid,
                                      u_int8_t  aftype,
                                      u_int8_t  vrid,
                                      u_int32_t ifindex,
                                      int *val)
{
  VRRP_SESSION *sess = vrrp_sess_tbl_lkup(apn_vrid, aftype, vrid, ifindex);
  if (sess != NULL) {
    *val = sess->stats_pri_zero_pkts_rcvd;
    return VRRP_API_GET_SUCCESS;
  }

  return VRRP_API_GET_ERROR;
}
int
vrrp_get_next_stats_rcvd_pri_zero_packets (int        apn_vrid,
                                           u_int8_t  *aftype,
                                           u_int8_t  *vrid,
                                           u_int32_t *ifindex,
                                           int len,
                                           int *val)
{
  VRRP_SESSION *sess;
  sess = vrrp_sess_tbl_lkup_next (apn_vrid, *aftype, *vrid, *ifindex);
  if (sess != NULL) {
    *aftype = sess->af_type;
    *ifindex = sess->ifindex;
    *vrid    = sess->vrid;
    *val = sess->stats_pri_zero_pkts_rcvd;
    return VRRP_API_GET_SUCCESS;
  }

  return VRRP_API_GET_ERROR;
}

int
vrrp_get_stats_sent_pri_zero_packets (int       apn_vrid,
                                      u_int8_t  aftype,
                                      u_int8_t  vrid,
                                      u_int32_t ifindex,
                                      int *val)
{
  VRRP_SESSION *sess = vrrp_sess_tbl_lkup(apn_vrid, aftype, vrid, ifindex);
  if (sess != NULL) {
    *val = sess->stats_pri_zero_pkts_sent;
    return VRRP_API_GET_SUCCESS;
  }

  return VRRP_API_GET_ERROR;
}

int
vrrp_get_next_stats_sent_pri_zero_packets (int        apn_vrid,
                                           u_int8_t  *aftype,
                                           u_int8_t  *vrid,
                                           u_int32_t *ifindex,
                                           int len,
                                           int *val)
{
  VRRP_SESSION *sess = vrrp_sess_tbl_lkup_next (apn_vrid, *aftype, *vrid, *ifindex);
  if (sess != NULL) {
    *aftype = sess->af_type;
    *ifindex = sess->ifindex;
    *vrid    = sess->vrid;
    *val = sess->stats_pri_zero_pkts_sent;
    return VRRP_API_GET_SUCCESS;
  }
  return VRRP_API_GET_ERROR;
}

int
vrrp_get_stats_rcvd_invalid_type_pkts (int       apn_vrid,
                                       u_int8_t  aftype,
                                       u_int8_t  vrid,
                                       u_int32_t ifindex,
                                       int *val)
{
  VRRP_SESSION *sess = vrrp_sess_tbl_lkup(apn_vrid, aftype, vrid, ifindex);
  if (sess != NULL) {
    *val = sess->stats_invalid_type_pkts_rcvd;
    return VRRP_API_GET_SUCCESS;
  }
  return VRRP_API_GET_ERROR;
}

int
vrrp_get_next_stats_rcvd_invalid_type_pkts (int        apn_vrid,
                                            u_int8_t  *aftype,
                                            u_int8_t  *vrid,
                                            u_int32_t *ifindex,
                                            int len,
                                            int *val)
{
  VRRP_SESSION *sess = vrrp_sess_tbl_lkup_next (apn_vrid, *aftype, *vrid, *ifindex);
  if (sess != NULL) {
    *aftype = sess->af_type;
    *ifindex = sess->ifindex;
    *vrid    = sess->vrid;
    *val = sess->stats_invalid_type_pkts_rcvd;
    return VRRP_API_GET_SUCCESS;
  }
  return VRRP_API_GET_ERROR;
}

int
vrrp_get_stats_address_list_errors (int       apn_vrid,
                                    u_int8_t  aftype,
                                    u_int8_t  vrid,
                                    u_int32_t ifindex,
                                    int *val)
{
  VRRP_SESSION *sess = vrrp_sess_tbl_lkup(apn_vrid, aftype, vrid, ifindex);
  if (sess != NULL) {
    *val = sess->stats_addr_list_errors;
    return VRRP_API_GET_SUCCESS;
  }
  return VRRP_API_GET_ERROR;
}

int
vrrp_get_next_stats_address_list_errors (int        apn_vrid,
                                         u_int8_t  *aftype,
                                         u_int8_t  *vrid,
                                         u_int32_t *ifindex,
                                         int len,
                                         int *val)
{
  VRRP_SESSION *sess;
  sess = vrrp_sess_tbl_lkup_next (apn_vrid, *aftype, *vrid, *ifindex);

  if (sess != NULL) {
    *aftype = sess->af_type;
    *ifindex = sess->ifindex;
    *vrid    = sess->vrid;
    *val = sess->stats_addr_list_errors;
    return VRRP_API_GET_SUCCESS;
  }
  return VRRP_API_GET_ERROR;
}

int
vrrp_get_stats_packet_length_errors (int       apn_vrid,
                                     u_int8_t  aftype,
                                     u_int8_t  vrid,
                                     u_int32_t ifindex,
                                     int *val)
{
  VRRP_SESSION *sess = vrrp_sess_tbl_lkup(apn_vrid, aftype, vrid, ifindex);
  if (sess != NULL) {
    *val = sess->stats_pkt_len_errors;
    return VRRP_API_GET_SUCCESS;
  }
  return VRRP_API_GET_ERROR;
}

int
vrrp_get_next_stats_packet_length_errors (int        apn_vrid,
                                          u_int8_t  *aftype,
                                          u_int8_t  *vrid,
                                          u_int32_t *ifindex,
                                          int len,
                                          int *val)
{
  VRRP_SESSION *sess = vrrp_sess_tbl_lkup_next (apn_vrid, *aftype, *vrid, *ifindex);
  if (sess != NULL) {
    *aftype = sess->af_type;
    *ifindex = sess->ifindex;
    *vrid    = sess->vrid;
    *val = sess->stats_pkt_len_errors;
    return VRRP_API_GET_SUCCESS;
  }
  return VRRP_API_GET_ERROR;
}

int
vrrp_get_stats_rcvd_invalid_authentications (int       apn_vrid,
                                             u_int8_t  aftype,
                                             u_int8_t  vrid,
                                             u_int32_t ifindex,
                                             int *val)
{
  VRRP_SESSION *sess = vrrp_sess_tbl_lkup(apn_vrid, aftype, vrid, ifindex);

  if (sess != NULL) {
    *val = sess->stats_invalid_auth_type;
    return VRRP_API_GET_SUCCESS;
  }
  return VRRP_API_GET_ERROR;
}

int
vrrp_get_next_stats_rcvd_invalid_authentications (int        apn_vrid,
                                                  u_int8_t  *aftype,
                                                  u_int8_t  *vrid,
                                                  u_int32_t *ifindex,
                                                  int len,
                                                  int *val)
{
  VRRP_SESSION *sess;
  sess = vrrp_sess_tbl_lkup_next (apn_vrid, *aftype, *vrid, *ifindex);

  if (sess != NULL) {
    *aftype = sess->af_type;
    *ifindex = sess->ifindex;
    *vrid    = sess->vrid;
    *val = sess->stats_invalid_auth_type;
    return VRRP_API_GET_SUCCESS;
  }
  return VRRP_API_GET_ERROR;
}

int
vrrp_get_stats_discontinuity_time (int       apn_vrid,
                                   u_int8_t  aftype,
                                   u_int8_t  vrid,
                                   u_int32_t ifindex,
                                   int *val)
{
  VRRP_SESSION *sess = vrrp_sess_tbl_lkup(apn_vrid, aftype, vrid, ifindex);
  if (sess != NULL) {
    *val = 0;
    return VRRP_API_GET_SUCCESS;
  }
  return VRRP_API_GET_ERROR;
}

int
vrrp_get_next_stats_discontinuity_time (int        apn_vrid,
                                        u_int8_t  *aftype,
                                        u_int8_t  *vrid,
                                        u_int32_t *ifindex,
                                        int len,
                                        int *val)
{
  VRRP_SESSION *sess;
  sess = vrrp_sess_tbl_lkup_next (apn_vrid, *aftype, *vrid, *ifindex);

  if (sess != NULL) {
    *aftype = sess->af_type;
    *ifindex = sess->ifindex;
    *vrid    = sess->vrid;
    *val = 0;
    return VRRP_API_GET_SUCCESS;
  }
  return VRRP_API_GET_ERROR;
}

int
vrrp_get_stats_refresh_rate (int       apn_vrid,
                             u_int8_t  aftype,
                             u_int8_t  vrid,
                             u_int32_t ifindex,
                             int *val)
{
  VRRP_SESSION *sess= vrrp_sess_tbl_lkup(apn_vrid, aftype, vrid, ifindex);
  if (sess != NULL) {
    *val = 1000;
    return VRRP_API_GET_SUCCESS;
  }
  return VRRP_API_GET_ERROR;
}

int
vrrp_get_next_stats_refresh_rate (int        apn_vrid,
                                  u_int8_t  *aftype,
                                  u_int8_t  *vrid,
                                  u_int32_t *ifindex,
                                  int len,
                                  int *val)
{
  VRRP_SESSION *sess = vrrp_sess_tbl_lkup_next (apn_vrid, *aftype, *vrid, *ifindex);
  if (sess != NULL) {
    *aftype = sess->af_type;
    *ifindex = sess->ifindex;
    *vrid    = sess->vrid;
    *val     = 1000;
    return VRRP_API_GET_SUCCESS;
  }
  return VRRP_API_GET_ERROR;
}

/******************************************************************/

int
vrrp_set_notify(int apn_vrid, int val)
{
  VRRP_GLOBAL_DATA *vrrp = VRRP_GET_GLOBAL(apn_vrid);

  if (vrrp != NULL) {
    vrrp->notificationcntl = val;
    return VRRP_API_SET_SUCCESS;
  }
  return VRRP_API_SET_ERROR;
}

int
vrrp_set_oper_priority (int       apn_vrid,
                        u_int8_t  aftype,
                        u_int8_t  vrid,
                        u_int32_t ifindex,
                        int val)
{
  int ret;

  VRRP_SESSION *sess = vrrp_sess_tbl_lkup(apn_vrid, aftype, vrid, ifindex);
  if (sess==NULL || sess->ifp->ifindex != ifindex
      || sess->admin_state == VRRP_ADMIN_STATE_UP) {
    return VRRP_API_SET_ERROR;
  }
  ret = vrrp_api_priority (apn_vrid, aftype, vrid, ifindex, val);
  if (ret != VRRP_OK) {
    return VRRP_API_SET_ERROR;
  }
  return VRRP_API_SET_SUCCESS;
}

int
vrrp_set_oper_primary_ipaddr (int       apn_vrid,
                              u_int8_t  aftype,
                              u_int8_t  vrid,
                              u_int32_t ifindex,
                              u_int8_t *ipaddr)
{
  int ret = 0;

  VRRP_SESSION *sess = vrrp_sess_tbl_lkup(apn_vrid, aftype, vrid, ifindex);
  if (sess==NULL || sess->admin_state == VRRP_ADMIN_STATE_UP) {
    return VRRP_API_SET_ERROR;
  }
  ret = vrrp_api_virtual_ip (apn_vrid, sess->af_type, vrid, ifindex, ipaddr, PAL_TRUE);
  if (ret != VRRP_OK) {
    return VRRP_API_SET_ERROR;
  }
  return VRRP_API_SET_SUCCESS;
}

int
vrrp_set_oper_adv_interval (int       apn_vrid,
                            u_int8_t  aftype,
                            u_int8_t  vrid,
                            u_int32_t ifindex,
                            int val)
{
  int ret = 0;
  int adv_int;
  VRRP_SESSION *sess = vrrp_sess_tbl_lkup(apn_vrid, aftype, vrid, ifindex);
  if (sess == NULL || sess->admin_state == VRRP_ADMIN_STATE_UP) {
    return VRRP_API_SET_ERROR;
  }
  adv_int = val/100;
  if (adv_int<=0) {
    adv_int = 1;
  }
  ret = vrrp_api_advt_interval (apn_vrid,
                                sess->af_type,
                                vrid,
                                ifindex,
                                adv_int);
  if ( ret != VRRP_OK) {
    return VRRP_API_SET_ERROR;
  }
  return VRRP_API_SET_SUCCESS;
}

int
vrrp_set_oper_preempt_mode (int       apn_vrid,
                            u_int8_t  aftype,
                            u_int8_t  vrid,
                            u_int32_t ifindex,
                            int val)
{
  int ret = 0;

  VRRP_SESSION *sess = vrrp_sess_tbl_lkup(apn_vrid, aftype, vrid, ifindex);
  if (sess == NULL || sess->admin_state == VRRP_ADMIN_STATE_UP) {
    return VRRP_API_SET_ERROR;
  }
  ret = vrrp_api_preempt_mode (apn_vrid, sess->af_type, vrid, ifindex, val);
  if (ret != VRRP_OK)
    return VRRP_API_SET_ERROR;

  return VRRP_API_SET_SUCCESS;
}

int
vrrp_set_oper_accept_mode (int       apn_vrid,
                           u_int8_t  aftype,
                           u_int8_t  vrid,
                           u_int32_t ifindex,
                           int val)
{
  VRRP_SESSION *sess = vrrp_sess_tbl_lkup(apn_vrid, aftype, vrid, ifindex);
  if (sess == NULL) {
    return VRRP_API_SET_ERROR;
  }
  if (aftype != AF_INET6) {
    return VRRP_API_SET_ERROR;
  }
  sess->accept_mode = val;
  return VRRP_API_SET_SUCCESS;
}

int
vrrp_set_oper_storage_type (int       apn_vrid,
                            u_int8_t  aftype,
                            u_int8_t  vrid,
                            u_int32_t ifindex,
                            int val)
{
  VRRP_SESSION *sess = vrrp_sess_tbl_lkup(apn_vrid, aftype, vrid, ifindex);
  if (sess == NULL) {
    return VRRP_API_SET_ERROR;
  }
  sess->storage_type = val;
  return VRRP_API_SET_SUCCESS;
}

int
vrrp_set_oper_rowstatus (int       apn_vrid,
                         u_int8_t  aftype,
                         u_int8_t  vrid,
                         u_int32_t ifindex,
                         int val)
{

  VRRP_SESSION *sess = vrrp_sess_tbl_lkup(apn_vrid, aftype, vrid, ifindex);
  VRRP_ASSO    *asso = vrrp_asso_tbl_find_first(apn_vrid, aftype, vrid, ifindex);
  u_int8_t *ipaddr=NULL;

  switch (val) {
  case VRRP_SNMP_ROW_STATUS_ACTIVE:
    if (sess == NULL) {
      return VRRP_API_SET_ERROR;
    }
    switch (sess->rowstatus) {
    case VRRP_SNMP_ROW_STATUS_NOTREADY:
    case VRRP_SNMP_ROW_STATUS_NOTINSERVICE:
      if (asso == NULL || asso->asso_rowstatus != VRRP_SNMP_ROW_STATUS_ACTIVE) {
        /* From the RFC the vrrpAssoIpAddr Table must have
           atleast one row active for the same vrid and if ifindex
           combination to make the row status of this table
          active*/
        return VRRP_API_SET_ERROR;
      }
      if (aftype == AF_INET) {
        ipaddr = (u_int8_t *)&asso->ipad_v4;
      }
#ifdef HAVE_IPV6
      else {
        ipaddr = (u_int8_t *)&asso->ipad_v6;
      }
#endif
      if (vrrp_api_virtual_ip (apn_vrid, aftype, vrid, ifindex, ipaddr,
                               sess->prio == VRRP_DEFAULT_IP_OWNER_PRIORITY)
          != VRRP_OK){
        return VRRP_API_SET_ERROR;
      }
      if (vrrp_api_enable_session(apn_vrid, aftype, vrid, ifindex) != VRRP_OK) {
        return VRRP_API_SET_ERROR;
      }
      break;

    default:
      return VRRP_API_SET_ERROR;
      break;
    }
    break;

  case VRRP_SNMP_ROW_STATUS_CREATEANDGO:
    if ((sess = vrrp_api_get_session(apn_vrid, aftype, vrid, ifindex))
                == NULL)
      return VRRP_API_SET_ERROR;

    /* From the RFC the vrrpAssoIpAddr Table must have atleast one
       row active for the same vrid and ifindex combination to
       make the row status of this table active.
     */
    if (asso == NULL || asso->asso_rowstatus != VRRP_SNMP_ROW_STATUS_ACTIVE) {
      return VRRP_API_SET_ERROR;
    }
    if (aftype == AF_INET) {
      ipaddr = (u_int8_t *)&asso->ipad_v4;
    }
#ifdef HAVE_IPV6
    else {
      ipaddr = (u_int8_t *)&asso->ipad_v6;
    }
#endif
    if (vrrp_api_virtual_ip (apn_vrid, aftype, vrid, ifindex, ipaddr,
                             sess->prio == VRRP_DEFAULT_IP_OWNER_PRIORITY)
        != VRRP_OK){
      return VRRP_API_SET_ERROR;
    }
    if (vrrp_api_enable_session(apn_vrid, aftype, vrid, ifindex) != VRRP_OK) {
      return VRRP_API_SET_ERROR;
    }
    break;

  case VRRP_SNMP_ROW_STATUS_CREATEANDWAIT:
    if (sess != NULL)
      return VRRP_API_SET_ERROR;
    if (vrrp_api_get_session(apn_vrid, aftype, vrid, ifindex) == NULL)
      return VRRP_API_SET_ERROR;
    break;

  case VRRP_SNMP_ROW_STATUS_NOTINSERVICE:
    if (sess == NULL) {
      return VRRP_API_SET_ERROR;
    }
    switch (sess->rowstatus) {
    case VRRP_SNMP_ROW_STATUS_NOTREADY:
    case VRRP_SNMP_ROW_STATUS_NOTINSERVICE:
      return VRRP_API_SET_ERROR;
      break;

    case VRRP_SNMP_ROW_STATUS_ACTIVE:
      if (vrrp_api_disable_session (apn_vrid, aftype, vrid, ifindex) != VRRP_OK) {
        return VRRP_API_SET_ERROR;
      }
      sess->admin_state = VRRP_ADMIN_STATE_DOWN;
      sess->rowstatus = VRRP_SNMP_ROW_STATUS_NOTINSERVICE;
    }
    break;

  case VRRP_SNMP_ROW_STATUS_DESTROY:
    if (sess == NULL || sess->ifp->ifindex == ifindex) {
      return VRRP_API_SET_ERROR;
    }
    vrrp_api_delete_session (apn_vrid, aftype, vrid, ifindex);
    break;

  case VRRP_SNMP_ROW_STATUS_NOTREADY:
    /* User is not allowed to set the Row Status to Not-Ready.*/
    return VRRP_API_SET_ERROR;
    break;

  default:
    return VRRP_API_SET_ERROR;
    break;
  }
  /* active(1),notInservice(2),createAndGo(4),
   * createAndWait(5), destroy(6) */
  return VRRP_API_SET_SUCCESS;
}

/*******************************************************************/

int
vrrp_set_asso_storage_type (int       apn_vrid,
                            u_int8_t  aftype,
                            u_int8_t  vrid,
                            u_int32_t ifindex,
                            u_int8_t *ipaddr,
                            int val)
{
  VRRP_ASSO *asso = vrrp_asso_tbl_lkup(apn_vrid, aftype, vrid, ifindex, ipaddr);
  if (asso == NULL) {
    return VRRP_API_SET_ERROR;
  }
  asso->asso_storage_type = val;
  return VRRP_API_SET_SUCCESS;
}

int
vrrp_set_asso_ipaddr_rowstatus (int       apn_vrid,
                                u_int8_t  aftype,
                                u_int8_t  vrid,
                                u_int32_t ifindex,
                                u_int8_t *ipaddr,
                                int val)
{

  VRRP_ASSO *asso = vrrp_asso_tbl_lkup(apn_vrid, aftype, vrid, ifindex, ipaddr);

  switch (val) {
  case VRRP_SNMP_ROW_STATUS_ACTIVE:
    if (asso == NULL) {
      /* If the session is not already existing then
       * return error.
       */
      return VRRP_API_SET_ERROR;
    }
    switch (asso->asso_rowstatus) {
    case VRRP_SNMP_ROW_STATUS_NOTREADY:
    case VRRP_SNMP_ROW_STATUS_NOTINSERVICE:
      asso->asso_rowstatus = VRRP_SNMP_ROW_STATUS_ACTIVE;
      break;
    default:
      return VRRP_API_SET_ERROR;
      break;
    }
    break;

  case VRRP_SNMP_ROW_STATUS_CREATEANDGO:
    if (asso != NULL) {
      return VRRP_API_SET_ERROR;
    }

    if (vrrp_api_enable_asso_entry (apn_vrid,
                                    aftype,
                                    vrid,
                                    ifindex,
                                    ipaddr,
                                    VRRP_SNMP_ROW_STATUS_ACTIVE) != VRRP_OK)
      return VRRP_API_SET_ERROR;
    break;

  case VRRP_SNMP_ROW_STATUS_CREATEANDWAIT:
    if (asso != NULL) {
      return VRRP_API_SET_ERROR;
    }
    if (vrrp_api_enable_asso_entry (apn_vrid,
                                    aftype,
                                    vrid,
                                    ifindex,
                                    ipaddr,
                                    VRRP_SNMP_ROW_STATUS_ACTIVE) != VRRP_OK)
      return VRRP_API_SET_ERROR;
    break;

  case VRRP_SNMP_ROW_STATUS_NOTINSERVICE:
    if (asso == NULL) {
      return VRRP_API_SET_ERROR;
    }
    switch (asso->asso_rowstatus) {
    case VRRP_SNMP_ROW_STATUS_NOTREADY:
    case VRRP_SNMP_ROW_STATUS_NOTINSERVICE:
      return VRRP_API_SET_ERROR;
      break;
    case  VRRP_SNMP_ROW_STATUS_ACTIVE:
      asso->asso_rowstatus = VRRP_SNMP_ROW_STATUS_NOTINSERVICE;
      break;
    }
    break;

  case VRRP_SNMP_ROW_STATUS_DESTROY:
    if (asso!= NULL) {
      vrrp_asso_tbl_delete_asso(asso);
    }
    else {
      return VRRP_API_SET_ERROR;
    }
    break;

  case VRRP_SNMP_ROW_STATUS_NOTREADY:
    /* User is not allowed to set the Row Status to Not-Ready. */
    return VRRP_API_SET_ERROR;
    break;

  default:
    return VRRP_API_SET_ERROR;
    break;
  }
  /* active(1),notInservice(2),createAndGo(4),
   * createAndWait(5), destroy(6)
   */
  return VRRP_API_SET_SUCCESS;
}

#endif /*HAVE_SNMP */

