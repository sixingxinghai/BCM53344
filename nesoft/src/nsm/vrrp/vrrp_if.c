
/* Copyright (C) 2001-2009  All Rights Reserved. */
#include "pal.h"

#include "lib.h"
#include "vty.h"
#include "if.h"
#include "prefix.h"
#include "stream.h"
#include "thread.h"
#include "sockunion.h"
#include "log.h"

#include "nsm/nsmd.h"
#include "nsm/nsm_fea.h"
#include "nsm/nsm_interface.h"
#include "nsm/nsm_debug.h"

#include "nsm/vrrp/vrrp_debug.h"
#include "nsm/vrrp/vrrp_api.h"
#include "nsm/vrrp/vrrp.h"

/* Forward definitions */
#ifdef HAVE_VRRP_LINK_ADDR
static int _del_virtual_ipv4_addr (struct interface *ifp, struct connected *ifc);
#ifdef HAVE_IPV6
static int _del_virtual_ipv6_addr (struct interface *ifp, struct connected *ifc);
#endif /* HAVE_IPV6 */
#endif
/*---------------------------------------------------------------------------
 * _sess_cmp - Compare Delete session at the time the inetrface is deleted
 *---------------------------------------------------------------------------
 */
static s_int32_t
_vrrp_if_sess_cmp(VRRP_SESSION *sess1, VRRP_SESSION *sess2)
{
  if (sess1->af_type != sess2->af_type) {
    return ((int)sess1->af_type - (int)sess2->af_type);
  }
  if (sess1->vrid != sess2->vrid) {
    return (sess1->vrid - sess2->vrid);
  }
  if (sess1->ifindex != sess2->ifindex) {
    return ((int)sess1->ifindex -(int) sess2->ifindex);
  }
  return 0;
}

/*---------------------------------------------------------------------------
 * _vrrp_if_sess_del - Delete session at the time the interface is deleted
 *                     We must shutdown the session and remove it completely.
 *---------------------------------------------------------------------------
 */
static s_int32_t
_vrrp_if_sess_del (VRRP_SESSION *sess)
{
  if (IS_DEBUG_VRRP_EVENT) {
    zlog_warn (NSM_ZG,
               "VRRP Event: Shutdown of VR %d/%d/%s\n",
               sess->af_type, sess->vrid, sess->ifindex);
  }
  vrrp_shutdown_sess (sess);

  /* Deletion of interface is a very violant act. But once it happens
     We have no other choice but to delete the session completely.
   */
  vrrp_sess_tbl_delete_sess(sess);

  return 0;
}
/*---------------------------------------------------------------------------
 * vrrp_if_add_sess - Add session to the interface
 *---------------------------------------------------------------------------
 */
int
vrrp_if_add_sess(VRRP_SESSION *sess)
{
  int rc=1;
  struct nsm_if *nif = sess->ifp->info;
  VRRP_IF       *vif = &nif->vrrp_if;

  if (IS_DEBUG_VRRP_EVENT) {
    zlog_warn (NSM_ZG, "VRRP Event: Addition of VRRP session to interface %s\n",
               sess->ifp->name);
  }
  /* Check the session is not there already.
     If not, attach it at the end of the list
   */
  if (sess->af_type == AF_INET) {
    rc = listnode_add_sort_nodup (vif->lh_sess_ipv4, sess);
  }
#ifdef HAVE_IPV6
  else { /* AF_INET6 */
    rc = listnode_add_sort_nodup (vif->lh_sess_ipv6, sess);
  }
#endif /* HAVE_IPV6 */
  return (rc ? VRRP_FAILURE : VRRP_OK);
}

/*---------------------------------------------------------------------------
 * vrrp_if_del_sess - Delete session from the interface (CLI/SNMP)
 *                    Do not care about the session state.
 *---------------------------------------------------------------------------
 */
void vrrp_if_del_sess(VRRP_SESSION *sess)
{
  struct nsm_if *nif = sess->ifp->info;
  VRRP_IF       *vif = &nif->vrrp_if;

  if (IS_DEBUG_VRRP_EVENT) {
    zlog_warn (NSM_ZG, "VRRP Event: Deletion VRRP session from interface %s\n",
               sess->ifp->name);
  }

  if (sess->af_type == AF_INET) {
    listnode_delete (vif->lh_sess_ipv4, sess);
  }
#ifdef HAVE_IPV6
  else {
    listnode_delete (vif->lh_sess_ipv6, sess);
  }
#endif /* HAVE_IPV6 */
}

/*---------------------------------------------------------------------------
 * vrrp_if_init - Creation of new interface. Init the VRRP content.
 *---------------------------------------------------------------------------
 */
int
vrrp_if_init (struct interface *ifp)
{
  struct nsm_if *nif = ifp->info;
  VRRP_IF       *vif = &nif->vrrp_if;

  if (IS_DEBUG_VRRP_EVENT) {
    zlog_warn (NSM_ZG, "VRRP Event: Init VRRP on interface %s\n", ifp->name);
  }

  /* Initialize session lists. */
  vif->lh_sess_ipv4 = list_create((list_cmp_cb_t)_vrrp_if_sess_cmp,
                                  (list_del_cb_t)_vrrp_if_sess_del);
#ifdef HAVE_IPV6
  vif->lh_sess_ipv6 = list_create((list_cmp_cb_t)_vrrp_if_sess_cmp,
                                  (list_del_cb_t)_vrrp_if_sess_del);
#endif /*HAVE_IPV6 */

  return VRRP_OK;
}

/*---------------------------------------------------------------------------
 * vrrp_if_delete - Interface deletion. We also need to shutdown and delete
 *                  all the sessions.
 *---------------------------------------------------------------------------
 */
void
vrrp_if_delete (struct interface *ifp)
{
  struct nsm_if *nif = ifp->info;
  VRRP_IF       *vif = &nif->vrrp_if;

  if (IS_DEBUG_VRRP_EVENT) {
    zlog_warn (NSM_ZG, "VRRP Event: Closing VRRP on interface %s\n", ifp->name);
  }

  if (vif->lh_sess_ipv4 == NULL)
    return;

#ifdef HAVE_IPV6
  if (vif->lh_sess_ipv6 == NULL)
    return;
#endif /* HAVE_IPV6 */

  /* Shutdown and delete all the sessions on this interface */
  list_delete_all_node (vif->lh_sess_ipv4);
  list_free(vif->lh_sess_ipv4);
  vif->lh_sess_ipv4 = NULL;

#ifdef HAVE_IPV6
  list_delete_all_node (vif->lh_sess_ipv6);
  list_free(vif->lh_sess_ipv6);
  vif->lh_sess_ipv6 = NULL;
#endif /* HAVE_IPV6 */
}

/*---------------------------------------------------------------------------
 * vrrp_if_is_vipv4_addr - When interface address is changing
 *                         check whether this is the VRRP session virtual
 *                         IP address
 *---------------------------------------------------------------------------
 */
bool_t
vrrp_if_is_vipv4_addr (struct interface *ifp, struct pal_in4_addr *addr)
{

  struct nsm_if *nif = ifp->info;
  VRRP_IF       *vif = &nif->vrrp_if;
  VRRP_SESSION *sess = NULL;

  struct listnode *node;

  LIST_LOOP(vif->lh_sess_ipv4,sess,node) {

    if (sess->vip_v4.s_addr == addr->s_addr && ! sess->owner)
      return PAL_TRUE;
  }
  return PAL_FALSE;
}

#ifdef HAVE_IPV6

/*---------------------------------------------------------------------------
 * vrrp_if_is_vipv6_addr - When interface address is changing
 *                         check whether this is the VRRP session virtual
 *                         IP address
 *---------------------------------------------------------------------------
 */
bool_t
vrrp_if_is_vipv6_addr (struct interface *ifp, struct pal_in6_addr *addr)
{

  struct nsm_if *nif = ifp->info;
  VRRP_IF       *vif = &nif->vrrp_if;
  VRRP_SESSION *sess = NULL;

  struct listnode *node;

  LIST_LOOP(vif->lh_sess_ipv6, sess, node) {

    if (IPV6_ADDR_SAME (&sess->vip_v6, addr) && ! sess->owner)
      return PAL_TRUE;
  }
  return PAL_FALSE;
}
#endif  /* HAVE_IPV6 */

/*---------------------------------------------------------------------------
 * vrrp_if_join_mcast_grp_for_first - Join multicast group when first
 *                                    session starts on interface.
 *---------------------------------------------------------------------------
 */
int
vrrp_if_join_mcast_grp_first (VRRP_SESSION *sess)
{
  struct interface *ifp = sess->ifp;
  struct nsm_if *nif = ifp->info;
  VRRP_IF       *vif = &nif->vrrp_if;
  VRRP_SESSION *t_sess = NULL;
  struct listnode *node;
  struct list  *lh;
  int       iret = VRRP_FAILURE;
#ifdef HAVE_IPV6
  struct pal_in6_addr addr;
#endif /* HAVE_IPV6 */

  lh = NULL;

  if (sess->af_type == AF_INET) {
    lh = vif->lh_sess_ipv4;
  }
#ifdef HAVE_IPV6
  else { /* AF_INET6 */
    lh = vif->lh_sess_ipv6;
  }
#endif /* HAVE_IPV6 */
  LIST_LOOP(lh, t_sess, node)
  {
    if (VRRP_SESS_IS_RUNNING(t_sess))
    {
      /* A session is running => we have already joined. */
      return (VRRP_OK);
    }
  }
  /* The first session is starting on this interface. Join the group. */
  if (sess->af_type == AF_INET)
    {
      if ((iret = vrrp_multicast_join (sess->vrrp->sock,
                                       sess->ifp)) == VRRP_OK)
        {
          SET_FLAG (nif->vrrp_if.vrrp_flags, NSM_VRRP_IPV4_MCAST_JOINED);
        }
    }
#ifdef HAVE_IPV6
  else
    {
      iret = pal_inet_pton (AF_INET6, VRRP_MCAST_ADDR6, &addr);
      if (iret <= 0)
        return (VRRP_FAILURE);

      if ((iret = vrrp_ipv6_multicast_join (sess->vrrp->ipv6_sock,
                                           addr,
                                           sess->ifp)) == VRRP_OK)
        {
          SET_FLAG (nif->vrrp_if.vrrp_flags, NSM_VRRP_IPV6_MCAST_JOINED);
        }
    }
#endif /* HAVE_IPV6 */
  if (iret < 0)
    return (VRRP_FAILURE);
  else
    return (VRRP_OK);
    }

/*---------------------------------------------------------------------------
 * vrrp_if_leave_mcast_grp_last - Leave the multicast group on interface
 *                                if last session is shutdown.
 *---------------------------------------------------------------------------
 */
void
vrrp_if_leave_mcast_grp_last (VRRP_SESSION *sess)
{
  struct interface *ifp = sess->ifp;
  struct nsm_if *nif = ifp->info;
  VRRP_IF       *vif = &nif->vrrp_if;
  VRRP_SESSION *t_sess = NULL;
  struct listnode *node;
  struct list  *lh = NULL;
  int iret = -1;
#ifdef HAVE_IPV6
  s_int32_t ret;
#endif /* HAVE_IPV6 */

  if (sess->af_type == AF_INET) {
    lh = vif->lh_sess_ipv4;
  }
#ifdef HAVE_IPV6
  else { /* AF_INET6 */
    lh = vif->lh_sess_ipv6;
  }
#endif /* HAVE_IPV6 */

  LIST_LOOP(lh, t_sess, node)
  {
    if (VRRP_SESS_IS_RUNNING(t_sess) && (t_sess != sess))
    {
      /* Another session is still running. */
      return;
    }
  }
  if (sess->af_type == AF_INET)
    {
      iret = vrrp_multicast_leave (sess->vrrp->sock, sess->ifp);
      UNSET_FLAG (vif->vrrp_flags, NSM_VRRP_IPV4_MCAST_JOINED);
    }
#ifdef HAVE_IPV6
  else
    {
      struct pal_in6_addr addr;
      ret = pal_inet_pton (AF_INET6, VRRP_MCAST_ADDR6, &addr);
      if (ret <= 0)
        return;

      iret = vrrp_ipv6_multicast_leave (sess->vrrp->ipv6_sock,
                                        addr,
                                        sess->ifp);
      UNSET_FLAG (vif->vrrp_flags, NSM_VRRP_IPV4_MCAST_JOINED);
    }
#endif /* HAVE_IPV6 */
  if (iret < 0)
    {
      if (IS_DEBUG_VRRP_EVENT)
        zlog_warn (NSM_ZG, "VRRP Warining: Error leaving mcast group\n");
    }
  }

/*--------------------------------------------------------------------------
 * vrrp_if_match_vip4_if - Check newly configured VIPv4 matches the right
 *                        interface.
 *                        Called from the vrrp_api.c
 *--------------------------------------------------------------------------
 */
bool_t
vrrp_if_match_vip4_if (VRRP_SESSION *sess, struct pal_in4_addr *addr, struct interface *ifp)
{
  struct nsm_master *nm = sess->vrrp->nm;
  if (sess->af_type != AF_INET) {
    return (PAL_FALSE);
  }
  return (if_lookup_by_ipv4_address (&nm->vr->ifm, addr) == ifp) ? (PAL_TRUE) : (PAL_FALSE);
}

/*--------------------------------------------------------------------------
 * IP address addition. Called from the nsm_interface.c
 *--------------------------------------------------------------------------
 */
int
vrrp_if_add_ipv4_addr (struct interface *ifp, struct connected *ifc)
{
  struct nsm_if *nif = NULL;
  VRRP_IF       *vif = NULL;
  struct prefix_ipv4 *p;
  struct listnode *node;
  VRRP_SESSION *sess;

  if (! ifp || !ifc)
    return 0;

  if (ifp->vr == NULL)
    return 1;

  nif = ifp->info;
  vif = &nif->vrrp_if;

  p = (struct prefix_ipv4 *)ifc->address;
  if (p->family != AF_INET)
    return 0;

  if (IS_DEBUG_VRRP_EVENT)
    zlog_warn (NSM_ZG, "VRRP Event: Interface %s address added\n", ifp->name);

  /* Run through all the sessions on this interface and enable those waiting
   * for the address to be installed.
   */
  LIST_LOOP(vif->lh_sess_ipv4, sess, node)
  {
      if (VRRP_SESS_IS_SHUTDOWN (sess))
        {
          /* Try to enable the session. */
      vrrp_enable_sess (sess);
    }
      /* NOTE For reviewer:
          We need to join the multicast group only once.
          We will do it from the the vrrp_enable_sess().
      */
    }
  return 0;
}

#ifdef HAVE_VRRP_LINK_ADDR
bool_t
vrrp_if_can_vip_addr_be_used (VRRP_SESSION *sess)
{
/* This will need to be implemented for platforms that cannot
 * respond to ARP using promiscuous mode.
 * 1. The VIP must be installed at the time of handling address delete event
 *    or may not be installed at the time the non-address owner is going
 *    Active (it might be installed after calling this function).
 * 2. Besides the VIP address the interface must also have a real
 *    IP address installed in the subnet of the VIP address.
 * 3. The VIP address must not be the primary IP address.
 */
  return PAL_FALSE;
}

#else /* Not having the HAVE_VRRP_LINK_ADDR set. */


/*-----------------------------------------------------------------------
 * vrrp_if_can_vip_addr_be_used - Check whether Virtual IP address
 *                                can be used on the session interface.
 * Purpose: To validate the use of Virtual IP address on interface at the
 *          time session is enabled, or any address is deleted from interface.
 *
 * Virtual IP address can be used on interface if:
 * 1. Session is VIP address owner and interface has the IP addresss istalled.
 * 2. Virtual IP address belongs to a subnet of another installed address.
 *-----------------------------------------------------------------------
 */
bool_t
vrrp_if_can_vip_addr_be_used (VRRP_SESSION *sess)
{
  struct nsm_master *nm = NULL ;
  struct apn_vrf *vrf = NULL;
  struct if_vr_master *ifm;

  nm  = sess->vrrp->nm;
  ifm = &nm->vr->ifm;

  /* For the address owner, same as VIP - real address must be installed.
     If same as VIP installed and not address owner -> bad, very bad.
   */
  if (sess->af_type == AF_INET)
    {
      if (if_lookup_by_ipv4_address (ifm, &sess->vip_v4) == sess->ifp)
        /* Address found: good if VIP owner; otherwise bad, very bad.
        */
        return (sess->owner);
      else
        { /* If not found and the VIP owner, also bad. */
          if (sess->owner)
            return PAL_FALSE;

          /* If not owner, find Real address with matching subnet of the VIP.
          */
          if ((vrf = apn_vrf_lookup_default (nm->vr)) == NULL)
            return (PAL_FALSE);

          return (if_match_by_ipv4_address (ifm,
                                            &sess->vip_v4,
                                            LIB_VRF_GET_VRF_ID (vrf)) == sess->ifp);
        }
    }
#ifdef HAVE_IPV6
  else if (sess->af_type == AF_INET6)
    {
      if (if_lookup_by_ipv6_address (ifm, &sess->vip_v6) == sess->ifp)
        return sess->owner;
      else
        { /* If not found and the VIP owner, also bad. */
          if (sess->owner)
            return PAL_FALSE;

          if ((vrf = apn_vrf_lookup_default (nm->vr)) == NULL)
            return PAL_FALSE;

          return (if_match_by_ipv6_address (ifm,
                                            &sess->vip_v6,
                                            LIB_VRF_GET_VRF_ID (vrf)) == sess->ifp);
        }
    }
#endif /*HAVE_IPV6*/
  else
    return PAL_FALSE;
}

/*-----------------------------------------------------------------------
 * vrrp_if_addr_del_down_sess - Check whether deletion of address shutdowns
 *                              session.
 * Deletion of IP address on the session's interface.
 * Return:
 *  PAL_TRUE  - Deletion of Owner address or non-owner subnet address
 *  PAL_FALSE - Session is not affected by address deletion
 *-----------------------------------------------------------------------
 */
static bool_t
_vrrp_if_addr_del_down_sess (VRRP_SESSION *sess, struct connected *ifc)
{
  /* For the address owner, same as VIP - real address must be installed.
     If same as VIP installed and not address owner -> bad, very bad.
   */
  if (sess->af_type == AF_INET)
    {
      struct prefix_ipv4 vp4;

      /* If address owner and VIP is deleted delete the session. */
      if (sess->owner)
        {
          if (IPV4_ADDR_SAME (&ifc->address->u.prefix4, &sess->vip_v4))
            return PAL_TRUE;
        }
      else
        {
          /* If subnet of VIP is removed for non-owner session,
             delete the session as well.
           */
          vp4.family    = AF_INET;
          vp4.prefixlen = IPV4_MAX_PREFIXLEN;
          vp4.prefix    = sess->vip_v4;
          if (prefix_match (ifc->address, (struct prefix *)&vp4))
              return PAL_TRUE;
        }
    }
#ifdef HAVE_IPV6
  else if (sess->af_type == AF_INET6)
    {
      struct prefix_ipv6 vp6;

      /* If address owner and VIP is deleted return FALSE */
      if (sess->owner)
        {
          if (IPV6_ADDR_SAME (&ifc->address->u.prefix6, &sess->vip_v6))
            return PAL_TRUE;
        }
      else
        {
          /* If subnet of VIP is removed for non-owner session,
              delete the session as well.
           */
          vp6.family    = AF_INET;
          vp6.prefixlen = IPV6_MAX_PREFIXLEN;
          vp6.prefix    = sess->vip_v6;
          if (prefix_match (ifc->address, (struct prefix *)&vp6))
            return PAL_TRUE;
        }
    }
#endif /*HAVE_IPV6*/

  return PAL_FALSE;
}

#endif /* for not having the HAVE_VRRP_LINK_ADDR set */


#ifdef HAVE_IPV6
/*--------------------------------------------------------------------------
 * vrrp_if_match_vip6_if - Check newly configured VIPv6 matches the right
 *                        interface.
 *                        Called from the vrrp_api.c
 *--------------------------------------------------------------------------
 */
bool_t
vrrp_if_match_vip6_if (VRRP_SESSION *sess, struct pal_in6_addr *addr, struct interface *ifp)
{
  struct nsm_master *nm = sess->vrrp->nm;
  if (sess->af_type != AF_INET6) {
    return (PAL_FALSE);
  }
  return (if_lookup_by_ipv6_address (&nm->vr->ifm, addr) == ifp) ? (PAL_TRUE) : (PAL_FALSE);
}

int
vrrp_if_add_ipv6_addr (struct interface *ifp, struct connected *ifc)
{
  struct nsm_if *nif = NULL;
  VRRP_IF       *vif = NULL;
  struct listnode *node;
  VRRP_SESSION *sess;
  struct prefix *p;

  if (! ifp || !ifc)
    return 0;

  if (ifp->vr == NULL)
    return 0;

  nif = ifp->info;
  vif = &nif->vrrp_if;

  p = (struct prefix *)ifc->address;
  if (p->family != AF_INET6)
    return 0;

  if (IS_DEBUG_VRRP_EVENT)
    zlog_warn (NSM_ZG, "VRRP Event: Interface %s address added\n", ifp->name);

  /* Run through all the sessions on this interface and enable those waiting
   * for the address to be installed.
   */
  LIST_LOOP(vif->lh_sess_ipv6, sess, node)
  {
    if (IN6_IS_ADDR_LINKLOCAL (&ifc->address->u.prefix6))
      {
        if (VRRP_SESS_IS_SHUTDOWN (sess))
          {
            /* Try to enable the session. */
          vrrp_enable_sess (sess);
          }
        /* NOTE For reviewer:
           We need to join the multicast group only once.
           We will do it from the the vrrp_enable_sess().
        */
      }
    return 1;
  }
  return 0;
}


/*--------------------------------------------------------------------------
 * vrrp_if_del_ipv6_addr - Notification from NSM about address deletion
 *
 *   IP address deletion. Called from the nsm_interface.c
 *   We must shutdown a VRRP session on interface if:
 *   1. Deleted same IP address as the Virtual IP address of Master, either
 *      the address owner or non-owner (HAVE_LINK_ADDR)
 *   2. Deleted Real IP address that belonged to the same subnet as
 *      the Virtual IP address of Running session.
 *--------------------------------------------------------------------------
 */
int
vrrp_if_del_ipv6_addr (struct interface *ifp, struct connected *ifc)
{
  struct nsm_if *nif = NULL;
  VRRP_IF       *vif = NULL;
  struct listnode *node;
  VRRP_SESSION *sess = NULL;
  struct prefix *p;

  if (! ifp || !ifc)
    return 0;

  if (ifp->vr == NULL)
    return 0;

  nif = ifp->info;
  vif = &nif->vrrp_if;

  p = (struct prefix *)ifc->address;
  if (p->family != AF_INET6)
    return 0;

  if (IS_DEBUG_VRRP_EVENT)
    zlog_warn (NSM_ZG, "VRRP Event: Interface %s IPv6 address deleted",
               ifp->name);

#ifdef HAVE_VRRP_LINK_ADDR
  /* In case the address is installed as a virtual, shutdown the Master session
     for which this is a VIP address.
   */
  if (vrrp_if_is_vipv6_addr (ifp, &ifc->address->u.prefix6))
    return _del_virtual_ipv6_addr (ifp, ifc);

  if (vif->lh_sess_ipv6 == NULL)
    return 0;
#endif

  LIST_LOOP(vif->lh_sess_ipv6, sess, node)
    {
      if (sess->state != VRRP_STATE_INIT)
        {
          if (! vrrp_if_can_vip_addr_be_used(sess))
            {
              if (IS_DEBUG_VRRP_EVENT)
                zlog_warn (NSM_ZG, "VRRP Event: Deletion of IPv6 address "
                           "forces session shutdown %d/%d/%s\n",
                           AF_INET, sess->vrid, ifp->name);
              vrrp_shutdown_sess (sess);
              sess->init_msg_code = VRRP_INIT_MSG_NO_SUBNET;
            }
        }
    }

#ifdef HAVE_VRRP_LINK_ADDR
    LIST_LOOP(vif->lh_sess_ipv6, sess, node)
      {
      if (sess->state == VRRP_STATE_MASTER)
      /* Refresh VIP. */
      vrrp_fe_vip_refresh (ifp, sess);
     }

#endif /* HAVE_VRRP_LINK_ADDR */
  return 0;
}
#endif /* HAVE_IPV6 */

/*--------------------------------------------------------------------------
 * vrrp_if_del_ipv4_addr - Notification from NSM about address deletion
 *
 * IP address deletion. Called from the nsm_interface.c
 * We must shutdown a VRRP session on interface if:
 * 1. Deleted same IP address as the Virtual IP address of Master, either
 *    the address owner or non-owner (HAVE_LINK_ADDR)
 * 2. Deleted Real IP address that belonged to the same subnet as
 *    the Virtual IP address of Running session.
 *--------------------------------------------------------------------------
 */
int
vrrp_if_del_ipv4_addr (struct interface *ifp, struct connected *ifc)
{
  VRRP_SESSION *sess;
  struct prefix_ipv4 *p;
  struct listnode *node;
  struct nsm_if *nif = NULL;
  VRRP_IF       *vif = NULL;

  if (! ifp || !ifc)
    return 0;

  if (ifp->vr == NULL)
    return 1;

  nif = ifp->info;
  vif = &nif->vrrp_if;

  p = (struct prefix_ipv4 *)ifc->address;
  if (p->family != AF_INET)
    return 0;

  if (IS_DEBUG_VRRP_EVENT)
    zlog_warn (NSM_ZG, "VRRP Event: Interface %s IP address deleted", ifp->name);

#ifdef HAVE_VRRP_LINK_ADDR   /********* NOT FOR LINUX OR BROADCOM **********/

  if (vif->lh_sess_ipv4 == NULL)
    return 0;

  /* In case the address is installed as a virtual, shutdown the Master session
     for which this is a VIP address.
   */
  if (vrrp_if_is_vipv4_addr (ifp, &ifc->address->u.prefix4))
    return _del_virtual_ipv4_addr (ifp, ifc);

  /* Is the last real address being deleted ?
   * In case session is a MASTER at least 2 IP addresses
   * must exist.
   */

  LIST_LOOP(vif->lh_sess_ipv4, sess, node) {
    if (sess->state == VRRP_STATE_MASTER) {
      min_count = 2;
      break;
    }
  }
#endif /* HAVE_VRRP_LINK_ADDR*/

  /* Shutdown a session if there is no REAL address that matches the VIP
     (in case of owner) or a subnet of the VIP (non-owner).
   */
  LIST_LOOP(vif->lh_sess_ipv4, sess, node)
    {
      if (sess->state != VRRP_STATE_INIT)
        {
          if (_vrrp_if_addr_del_down_sess (sess, ifc))
            {
        if (IS_DEBUG_VRRP_EVENT)
                zlog_warn (NSM_ZG, "VRRP Event: Deletion of IP address "
                           "forces session shutdown %d/%d/%s\n",
                     AF_INET, sess->vrid, ifp->name);
        vrrp_shutdown_sess (sess);
              sess->init_msg_code = VRRP_INIT_MSG_NO_SUBNET;
            }
        }
    }
#ifdef HAVE_VRRP_LINK_ADDR  /********* NOT FOR LINUX OR BROADCOM **********/
  /* We may need to make sure that VIP is not the Primary IP address (???) */
  LIST_LOOP(vif->lh_sess_ipv4, sess, node)
    {
      if (sess->state == VRRP_STATE_MASTER)
      {
        /* Refresh VIP. */
        vrrp_fe_vip_refresh (ifp, sess);
      }
    }
#endif /* HAVE_VRRP_LINK_ADDR */
  return 0;
}

/*--------------------------------------------------------------------------
 * Interfac is going UP. Called from the nsm_interface.c
 *--------------------------------------------------------------------------
 */
int
vrrp_if_up (struct interface *ifp)
{
  VRRP_SESSION *sess;

#ifdef HAVE_VRRP_LINK_ADDR
  struct connected *ifc;
  struct prefix *p = NULL;
  int found = -1;
#endif /* HAVE_VRRP_LINK_ADDR */
  struct listnode *node;
  struct nsm_if *nif = ifp->info;
  VRRP_IF       *vif = &nif->vrrp_if;

  if (ifp->vr == NULL)
    return 1;

  if (IS_DEBUG_VRRP_EVENT)
    zlog_warn (NSM_ZG, "VRRP Event: Interface %s UP\n", ifp->name);

  /* NOTE:
   *   1. Session cannot be started while the interface is down.
   *   2. If the interface goes down while the session is Running, the
   *      session is Shutdown and the external states removed.
   */

  /* Try to enable all VRRP sessions on this interface
  */
  LIST_LOOP(vif->lh_sess_ipv4, sess, node)
  {
    vrrp_enable_sess (sess);
  }
#ifdef HAVE_IPV6
  LIST_LOOP(vif->lh_sess_ipv6, sess, node)
  {
    vrrp_enable_sess (sess);
  }
#endif /* HAVE_IPV6 */

  return 1;
}

/*--------------------------------------------------------------------------
 * Interface is going DOWN. Called from the nsm_interface.c
 *--------------------------------------------------------------------------
 */
void
vrrp_if_down (struct interface *ifp)
{
  struct nsm_if *nif = ifp->info;
  VRRP_IF       *vif = &nif->vrrp_if;
  struct listnode *node;
  VRRP_SESSION  *sess;

  if (ifp->vr == NULL)
    return;

  if (IS_DEBUG_VRRP_EVENT)
    zlog_warn (NSM_ZG,
               "VRRP Event: Interface %s DOWN (%d IPv4 sessions shutdown)\n",
               ifp->name, LISTCOUNT(vif->lh_sess_ipv4));

  LIST_LOOP(vif->lh_sess_ipv4, sess, node)
    {
    vrrp_shutdown_sess (sess);
      sess->init_msg_code = VRRP_INIT_MSG_IF_NOT_RUNNING;
    }

#ifdef HAVE_IPV6
  if (IS_DEBUG_VRRP_EVENT)
    zlog_warn (NSM_ZG,
               "VRRP Event: Interface %s DOWN (%d IPv6 sessions shutdown)\n",
               ifp->name, LISTCOUNT(vif->lh_sess_ipv6));

  LIST_LOOP(vif->lh_sess_ipv6, sess, node)
    {
    vrrp_shutdown_sess (sess);
      sess->init_msg_code = VRRP_INIT_MSG_IF_NOT_RUNNING;
    }
#endif /* HAVE_IPV6 */
}

/*--------------------------------------------------------------------------
 * IP address delete. Called from the nsm_interface.c
 *--------------------------------------------------------------------------
 */
/*
static int
_get_ipv4_address_count (struct interface *ifp)
{
  struct connected *ifc;
  int count = 0;

  if (! ifp)
    return 0;

  for (ifc = ifp->ifc_ipv4; ifc; ifc = ifc->next)
    count++;

  return count;
}
*/
/*--------------------------------------------------------------------------
 * Interface is going DOWN. Called from the nsm_interface.c
 *--------------------------------------------------------------------------
 */
#ifdef HAVE_VRRP_LINK_ADDR
/* Virtual IP address is deleted from the interface. */
static int
_del_virtual_ipv4_addr (struct interface *ifp, struct connected *ifc)
{
  struct nsm_if *nif = ifp->info;
  VRRP_IF       *vif = &nif->vrrp_if;
  VRRP_SESSION  *sess;
  struct listnode *node;

  if (ifp->vr == NULL)
    return 1;

  LIST_LOOP(vif->lh_sess_ipv4, sess, node)
  {
    if (sess->state == VRRP_STATE_MASTER && ! sess->link_addr_deleted)
      {
        if (sess->vip_v4.s_addr == ifc->address->u.prefix4.s_addr)
          {
            if (IS_DEBUG_VRRP_EVENT)
              {
          zlog_warn (NSM_ZG,
                           "VRRP: Virtual IP address deleted "
                           "- shutting down session %d/%d/%s\n",
                     sess->af_type, sess->vrid, sess->ifp->name);
        }
        sess->link_addr_deleted = PAL_TRUE;
        vrrp_shutdown_sess (sess);
            sess->init_msg_code = VRRP_INIT_MSG_VIP_DELETED;

            /* As long as the session is in Init state it may configured anything.
               We will verify it at the time of enabling it.
            */
        sess->link_addr_deleted = PAL_FALSE;
      }
    }
  }
  return 0;
}

#ifdef HAVE_IPV6

/* Virtual IPv6 address is deleted from the interface. */

static int
_del_virtual_ipv6_addr (struct interface *ifp, struct connected *ifc)
{
  struct nsm_if *nif = ifp->info;
  VRRP_IF       *vif = &nif->vrrp_if;
  struct listnode *node;
  VRRP_SESSION  *sess;

  if (ifp->vr == NULL)
    return 1;

  LIST_LOOP(vif->lh_sess_ipv6, sess, node)
  {
    if (sess->state == VRRP_STATE_MASTER && ! sess->link_addr_deleted)
      {
        if (IPV6_ADDR_SAME (&sess->vip_v6, &ifc->address->u.prefix6))
          {
            if (IS_DEBUG_VRRP_EVENT)
              {
          zlog_warn (NSM_ZG,
                     "VRRP Event: Virtual IPv6 address deleted; "
                     "  shutdown Virtual Router %d/%d/%s\n",
                     sess->af_type, sess->vrid, sess->ifp->name);
        }
        sess->link_addr_deleted = PAL_TRUE;
        vrrp_shutdown_sess (sess);
            sess->init_msg_code = VRRP_INIT_MSG_VIP_DELETED;

            /* As long as the session is in Init state it may have configured
               anything.
               We will verify it at the time of enabling it.
            */
        sess->link_addr_deleted = PAL_FALSE;
      }
    }
  }
  return 0;
}
#endif /*HAVE_IPV6 */
#endif /* HAVE_VRRP_LINK_ADDR */

bool_t
vrrp_if_monitored_circuit_state(VRRP_SESSION *sess)
{
  struct nsm_master *nm = NULL;
  struct interface  *monitor_ifp = NULL;

  /* If the circuit is enabled and it is broken
   * do not enable the session.
   */
  nm = sess->vrrp->nm;

  if (sess->monitor_if)
    {
      if (nm->vr)
        {
          monitor_ifp = if_lookup_by_name (&nm->vr->ifm,
                                           sess->monitor_if);
          if (monitor_ifp)
            {
              /* Update interface flags.  */
              nsm_fea_if_flags_get (monitor_ifp);

              return (monitor_ifp && if_is_running (monitor_ifp));
            }
        }
    }
  return PAL_FALSE;
}

