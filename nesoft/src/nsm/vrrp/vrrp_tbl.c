
/* Copyright (C) 2001-2003  All Rights Reserved.
 *
 * This module contains handler for VRRP session and VRRP IP address tables.
 *
 *
 */

#include "pal.h"

#include "lib.h"
#include "log.h"
#include "vty.h"
#include "thread.h"

#include "nsm/nsmd.h"
#include "nsm/nsm_interface.h"
#include "nsm/nsm_fea.h"
#include "nsm/vrrp/vrrp.h"
#include "nsm/vrrp/vrrp_cli.h"
#include "nsm/vrrp/vrrp_init.h"
#include "nsm/vrrp/vrrp_debug.h"
#include "nsm/vrrp/vrrp_snmp.h"

static VRRP_SESSION **_sess_tbl_lkup_ref(VRRP_GLOBAL_DATA *vrrp, VRRP_SESSION *sess);


typedef int (*vrrp_bs_cmp_fun_t) (__const void *, __const void *);

static int
_sess_cmp (const VRRP_SESSION **pp_sess1, const VRRP_SESSION **pp_sess2)
{
  int res=0;

  if ((res = (int)(*pp_sess1)->af_type - (int)(*pp_sess2)->af_type) != 0) {
    return res;
  }
  if ((res = (*pp_sess1)->vrid - (*pp_sess2)->vrid) != 0) {
    return res;
  }
  if ((res = (int)(*pp_sess1)->ifindex - (int)(*pp_sess2)->ifindex) != 0) {
    return res;
  }
  return 0;
}

/*---------------------------------------------------------------------------
 * Create session table
 *---------------------------------------------------------------------------
 */
int
vrrp_sess_tbl_create(VRRP_GLOBAL_DATA *vrrp)
{
  if (vrrp->sess_tbl) {
    return VRRP_OK;
  }
  vrrp->sess_tbl_cnt = 0;
  vrrp->sess_tbl_max = VRRP_SESS_TBL_INI_SZ;
  vrrp->sess_tbl     = (VRRP_SESSION  **)XCALLOC(MTYPE_VRRP_GLOBAL_INFO,
                                                 VRRP_SESS_TBL_INI_SZ * sizeof (VRRP_SESSION *));
  if (vrrp->sess_tbl==NULL) {
    return VRRP_FAILURE;
  }
  return VRRP_OK;
}
/*---------------------------------------------------------------------------
 * Delete session table
 *  Assume all sessions are disabled
 *  Free session memeory as well
 *---------------------------------------------------------------------------
 */
int
vrrp_sess_tbl_delete(VRRP_GLOBAL_DATA *vrrp)
{
  int sess_ix;
  VRRP_SESSION *sess;

  if (vrrp->sess_tbl == NULL) {
    return VRRP_OK;
  }
  for (sess_ix=0; sess_ix<vrrp->sess_tbl_cnt; sess_ix++)
  {
    sess = vrrp->sess_tbl[sess_ix];
    if (sess == NULL) {
      zlog_err (nzg, "VRRP Error: Blank entry %d in session table\n",sess_ix);
    }
    else {
      XFREE(MTYPE_VRRP_SESSION, sess);
    }
  }
  /* Be nice and leave it tidy.. */
  vrrp->sess_tbl_cnt = 0;
  vrrp->sess_tbl_max = 0;
  XFREE(MTYPE_VRRP_GLOBAL_INFO, vrrp->sess_tbl);

  vrrp->sess_tbl     = NULL;
  return VRRP_OK;
}

/*---------------------------------------------------------------------------
 * Find and return session reference (by key)
 *---------------------------------------------------------------------------
 */
VRRP_SESSION *
vrrp_sess_tbl_lkup(int       apn_vrid,
                   u_int8_t  af_type,
                   int       vrid,
                   u_int32_t ifindex)
{
  VRRP_SESSION sess_key, *sess, **pp_sess;
  VRRP_GLOBAL_DATA *vrrp;

  sess_key.af_type = af_type;
  sess_key.vrid    = vrid;
  sess_key.ifindex = ifindex;

  sess = &sess_key;

  vrrp = VRRP_GET_GLOBAL(apn_vrid);

  pp_sess = (VRRP_SESSION **)bsearch(&sess,vrrp->sess_tbl,vrrp->sess_tbl_cnt,
                                     sizeof(VRRP_SESSION *),
                                     (vrrp_bs_cmp_fun_t)_sess_cmp);
  if (pp_sess == NULL) {
    return NULL;
  }
  else {
    return *pp_sess;
  }
}

/*---------------------------------------------------------------------------
 * Find and return next session (by key)
 * Key values might be out of range (vide: find first)
 *---------------------------------------------------------------------------
 */
VRRP_SESSION *
vrrp_sess_tbl_lkup_next (int       apn_vrid,
                         u_int8_t  af_type,
                         u_int8_t  vrid,
                         u_int32_t ifindex)
{
  VRRP_SESSION *sess;
  VRRP_GLOBAL_DATA *vrrp;
  int sess_ix;

  vrrp = VRRP_GET_GLOBAL(apn_vrid);

  /* Perfrom the linear search. */

  for (sess_ix=0; sess_ix<vrrp->sess_tbl_cnt; sess_ix++) {
    sess = vrrp->sess_tbl[sess_ix];

    if (sess->af_type > af_type) {
      return sess;
    }
    else if (sess->af_type == af_type) {
      if (sess->vrid > vrid) {
        return sess;
      }
      else if (sess->vrid == vrid) {
        if (sess->ifindex > ifindex) {
          return sess;
        }
      }
    }
  }
  return NULL;
}
/*---------------------------------------------------------------------------
 * Validate session reference
 *---------------------------------------------------------------------------
 */
int
vrrp_sess_tbl_check_ref(VRRP_SESSION *sess)
{
  VRRP_SESSION **sess_ref;
  VRRP_GLOBAL_DATA *vrrp = sess->vrrp;

  if (vrrp == NULL) {
    return (VRRP_FAILURE);
  }
  sess_ref = bsearch(&sess,
                     vrrp->sess_tbl,
                     vrrp->sess_tbl_cnt,
                     sizeof(VRRP_SESSION *),
                     (vrrp_bs_cmp_fun_t)_sess_cmp);
  return ((sess_ref!=NULL) ? (VRRP_OK) : (VRRP_FAILURE));
}

/*---------------------------------------------------------------------------
 * Find and return session instance; create if do not exist
 *---------------------------------------------------------------------------
 */
VRRP_SESSION *
vrrp_sess_tbl_add_ref (VRRP_SESSION *sess)
{
  VRRP_GLOBAL_DATA *vrrp = sess->vrrp;

  if (vrrp->sess_tbl_cnt >= vrrp->sess_tbl_max) {
    VRRP_SESSION **new_tbl = XREALLOC(MTYPE_VRRP_GLOBAL_INFO,
                                      vrrp->sess_tbl,
                                      vrrp->sess_tbl_max*2*sizeof(VRRP_SESSION *));
    if (new_tbl==NULL) {
      return (NULL);
    }
    vrrp->sess_tbl      = new_tbl;
    vrrp->sess_tbl_max *= 2;
  }
  vrrp->sess_tbl[vrrp->sess_tbl_cnt++] = sess;
  pal_qsort (vrrp->sess_tbl,
             vrrp->sess_tbl_cnt,
             sizeof(VRRP_SESSION *),
             (vrrp_bs_cmp_fun_t)_sess_cmp);
  return (sess);
}
/*---------------------------------------------------------------------------
 * Walk the VRRP session table and execute the given callback.
 * The behavior of the walk function depends on the value returned
 * from teh callback:
 * VRRP_OK      - Increment the counter and return to the caller
 * VRRP_FOUND   - We were searched for a single instance occurence
 * VRRP_FAILURE - The callback encountered a failure - terminates.
 * Otherwise continue the walk.
 *---------------------------------------------------------------------------
 */
int
vrrp_sess_tbl_walk(int apn_vrid,
                   void *ref,
                   vrrp_api_sess_walk_fun_t efun,
                   int *num_sess)
{
  VRRP_GLOBAL_DATA *vrrp;
  int cnt=0, sess_ix;
  int rc;

  if (efun == NULL) {
    return VRRP_FAILURE;
  }
  vrrp = VRRP_GET_GLOBAL(apn_vrid);

  for (sess_ix=0; sess_ix<vrrp->sess_tbl_cnt; sess_ix++) {
    rc = efun(vrrp->sess_tbl[sess_ix], ref);
    switch (rc) {
    case VRRP_OK:
      cnt++;
      break;
    case VRRP_FOUND:
      return VRRP_FOUND;
    case VRRP_FAILURE:
      return VRRP_FAILURE;
    case VRRP_IGNORE:
    default:
      break;
    }
  }
  if (num_sess) {
    *num_sess = cnt;
  }
  return VRRP_OK;
}

/*---------------------------------------------------------------------------
 * Create a new session
 *---------------------------------------------------------------------------
 */
VRRP_SESSION *
vrrp_sess_tbl_create_sess (VRRP_GLOBAL_DATA *vrrp,
                           u_int8_t  af_type,
                           int       vrid,
                           u_int32_t ifindex)
{
  VRRP_SESSION *sess;

  sess = XCALLOC (MTYPE_VRRP_SESSION, (sizeof (VRRP_SESSION)));
  if (sess == NULL) {
    return (NULL);
  }
  sess->af_type = af_type;
  sess->vrid    = vrid;
  sess->ifindex = ifindex;
  sess->vrrp    = vrrp;
  if (sess != vrrp_sess_tbl_add_ref(sess))
  {
    XFREE(MTYPE_VRRP_SESSION,sess);
    return (NULL);
  }
  return (sess);
}

/*----------------------------------------------------------------------------
 * Delete session reference from the table.
 * Do not free its memory.
 *----------------------------------------------------------------------------
 */
int
vrrp_sess_tbl_del_ref(VRRP_SESSION *sess)
{
  VRRP_SESSION **sess_ref;
  VRRP_GLOBAL_DATA *vrrp = sess->vrrp;

  if (vrrp == NULL) {
    return (VRRP_FAILURE);
  }
  if ((sess_ref = _sess_tbl_lkup_ref(vrrp, sess)) == NULL) {
    return (VRRP_FAILURE);
  }
  if (sess_ref != pal_mem_move(sess_ref,
                               sess_ref+1,
                               (char *)(&vrrp->sess_tbl[vrrp->sess_tbl_cnt]) -
                               (char *)(sess_ref+1))) {
    return (VRRP_FAILURE);
  }
  vrrp->sess_tbl_cnt--;
  return (VRRP_OK);
}

/*----------------------------------------------------------------------------
 * Delete session from the table.
 * Free its memory.
 *----------------------------------------------------------------------------
 */
int
vrrp_sess_tbl_delete_sess(VRRP_SESSION *sess)
{
  /* The session ref may not be there ... */
  vrrp_sess_tbl_del_ref(sess);
  XFREE (MTYPE_VRRP_SESSION, sess);
  return VRRP_OK;
}


/*---------------------------------------------------------------------------
 * Find and return a pointer to a session reference in session reference table
 *---------------------------------------------------------------------------
 */
static VRRP_SESSION **
_sess_tbl_lkup_ref(VRRP_GLOBAL_DATA *vrrp, VRRP_SESSION *sess)
{
  if (vrrp == NULL) {
    return (NULL);
  }
  return bsearch(&sess,
                 vrrp->sess_tbl,
                 vrrp->sess_tbl_cnt,
                 sizeof(VRRP_SESSION *),
                 (vrrp_bs_cmp_fun_t)_sess_cmp);
}




#ifdef HAVE_SNMP
/**************************************************************************
 **************************************************************************
 *                   A S S O C I A T E D   T A B L E
 **************************************************************************
 **************************************************************************
 */

static VRRP_ASSO **_asso_tbl_lkup_ref(VRRP_GLOBAL_DATA *vrrp, VRRP_ASSO *asso);


typedef int (*vrrp_as_cmp_fun_t) (__const void *, __const void *);

static int
_asso_cmp (const VRRP_ASSO **pp_asso1, const VRRP_ASSO **pp_asso2)
{
  int res=0;

  if ((res = (int)(*pp_asso1)->af_type - (int)(*pp_asso2)->af_type) != 0) {
    return res;
  }
  if ((res = (*pp_asso1)->vrid - (*pp_asso2)->vrid) != 0) {
    return res;
  }
  if ((res = (int)(*pp_asso1)->ifindex - (int)(*pp_asso2)->ifindex) != 0) {
    return res;
  }
  /* Compare IP addresses */
  if ((*pp_asso1)->af_type == AF_INET) {
    res = pal_mem_cmp(&(*pp_asso1)->ipad_v4, &(*pp_asso1)->ipad_v4, 4);
  }
#ifdef HAVE_IPV6
  else {
    res = pal_mem_cmp(&(*pp_asso1)->ipad_v6, &(*pp_asso1)->ipad_v6, 16);
  }
#endif
  return res;
}

/*---------------------------------------------------------------------------
 * Create associated table
 *---------------------------------------------------------------------------
 */
int
vrrp_asso_tbl_create(VRRP_GLOBAL_DATA *vrrp)
{
  if (vrrp->asso_tbl) {
    return VRRP_OK;
  }
  vrrp->asso_tbl_cnt = 0;
  vrrp->asso_tbl_max = VRRP_ASSO_TBL_INI_SZ;
  vrrp->asso_tbl     = (VRRP_ASSO  **)XCALLOC(MTYPE_VRRP_GLOBAL_INFO,
                                                 VRRP_ASSO_TBL_INI_SZ * sizeof (VRRP_ASSO *));
  if (vrrp->asso_tbl==NULL) {
    return VRRP_FAILURE;
  }
  return VRRP_OK;
}
/*---------------------------------------------------------------------------
 * Delete associated table
 *  Assume all associations are disabled
 *  Free associations memory as well
 *---------------------------------------------------------------------------
 */
int
vrrp_asso_tbl_delete(VRRP_GLOBAL_DATA *vrrp)
{
  int asso_ix;
  VRRP_ASSO *asso;

  if (vrrp->asso_tbl == NULL) {
    return VRRP_OK;
  }
  for (asso_ix=0; asso_ix<vrrp->asso_tbl_cnt; asso_ix++)
  {
    asso = vrrp->asso_tbl[asso_ix];
    if (asso == NULL) {
      zlog_err (nzg, "VRRP Error: Blank entry %d in assoion table\n",asso_ix);
    }
    else {
      XFREE(MTYPE_VRRP_ASSO, asso);
    }
  }
  /* Be nice and leave it tidy.. */
  vrrp->asso_tbl_cnt = 0;
  vrrp->asso_tbl_max = 0;
  XFREE(MTYPE_VRRP_GLOBAL_INFO, vrrp->asso_tbl);

  vrrp->asso_tbl     = NULL;
  return VRRP_OK;
}

/*---------------------------------------------------------------------------
 * Find and return association reference (by key)
 *---------------------------------------------------------------------------
 */
VRRP_ASSO *
vrrp_asso_tbl_lkup(int       apn_vrid,
                   u_int8_t  af_type,
                   int       vrid,
                   u_int32_t ifindex,
                   u_int8_t  *ipaddr)
{
  VRRP_ASSO asso_key, *asso, **pp_asso;
  VRRP_GLOBAL_DATA *vrrp;

  asso_key.af_type = af_type;
  asso_key.vrid    = vrid;
  asso_key.ifindex = ifindex;
  if (af_type == AF_INET) {
    pal_mem_cpy(&asso_key.ipad_v4, ipaddr, 4);
  }
#ifdef HAVE_IPV6
  else {
    pal_mem_cpy(&asso_key.ipad_v6, ipaddr, 4);
  }
#endif
  asso = &asso_key;

  vrrp = VRRP_GET_GLOBAL(apn_vrid);

  pp_asso = (VRRP_ASSO **)bsearch(&asso,
                                  vrrp->asso_tbl,
                                  vrrp->asso_tbl_cnt,
                                  sizeof(VRRP_ASSO *),
                                  (vrrp_as_cmp_fun_t)_asso_cmp);
  if (pp_asso == NULL) {
    return NULL;
  }
  else {
    return *pp_asso;
}
}

/*---------------------------------------------------------------------------
 * Find and return next association (by key)
 *
 *---------------------------------------------------------------------------
 */
VRRP_ASSO *
vrrp_asso_tbl_lkup_next (int       apn_vrid,
                         u_int8_t  af_type,
                         u_int8_t  vrid,
                         u_int32_t ifindex,
                         u_int8_t *ipaddr)
{
  VRRP_GLOBAL_DATA *vrrp;
  VRRP_ASSO        *asso;
  int asso_ix;

  vrrp = VRRP_GET_GLOBAL(apn_vrid);

  /* Perfrom th elinear search. */

  for (asso_ix=0; asso_ix<vrrp->asso_tbl_cnt; asso_ix++) {
    asso = vrrp->asso_tbl[asso_ix];

    if (asso->af_type > af_type) {
      return asso;
    }
    else if (asso->af_type == af_type) {
      if (asso->vrid > vrid) {
        return asso;
      }
      else if (asso->vrid == vrid) {
        if (asso->ifindex > ifindex) {
          return asso;
        }
        else if (asso->ifindex > ifindex) {
          /* Compare IP addresses */
          if (af_type == AF_INET) {
             if (pal_mem_cmp(&asso->ipad_v4, ipaddr, IN_ADDR_SIZE) > 0) {
               return asso;
             }
          }
#ifdef HAVE_IPV6
          else {
            if (pal_mem_cmp(&asso->ipad_v6, ipaddr, IN6_ADDR_SIZE) > 0) {
              return asso;
            }
          }
#endif
        }
      }
    }
  }
  return NULL;
}

/*---------------------------------------------------------------------------
 * Validate association reference
 *---------------------------------------------------------------------------
 */
int
vrrp_asso_tbl_check_ref(VRRP_ASSO *asso)
{
  VRRP_ASSO **asso_ref;
  VRRP_GLOBAL_DATA *vrrp = asso->vrrp;

  if (vrrp == NULL) {
    return (VRRP_FAILURE);
  }
  asso_ref = bsearch(&asso,
                     vrrp->asso_tbl,
                     vrrp->asso_tbl_cnt,
                     sizeof(VRRP_ASSO *),
                     (vrrp_as_cmp_fun_t)_asso_cmp);
  return ((asso_ref!=NULL) ? (VRRP_OK) : (VRRP_FAILURE));
}

/*---------------------------------------------------------------------------
 * Find and return assoion instance; create if do not exist
 *---------------------------------------------------------------------------
 */
VRRP_ASSO *
vrrp_asso_tbl_add_ref (VRRP_ASSO *asso)
{
  VRRP_GLOBAL_DATA *vrrp = asso->vrrp;

  if (vrrp->asso_tbl_cnt >= vrrp->asso_tbl_max) {
    VRRP_ASSO **new_tbl = XREALLOC(MTYPE_VRRP_GLOBAL_INFO,
                                      vrrp->asso_tbl,
                                      vrrp->asso_tbl_max*2*sizeof(VRRP_ASSO *));
    if (new_tbl==NULL) {
      return (NULL);
    }
    vrrp->asso_tbl      = new_tbl;
    vrrp->asso_tbl_max *= 2;
  }
  vrrp->asso_tbl[vrrp->asso_tbl_cnt++] = asso;
  pal_qsort (vrrp->asso_tbl,
             vrrp->asso_tbl_cnt,
             sizeof(VRRP_ASSO *),
             (vrrp_as_cmp_fun_t)_asso_cmp);
  return (asso);
}
/*---------------------------------------------------------------------------
 * Walk the VRRP assoion table and execute the given callback.
 *---------------------------------------------------------------------------
 */
int
vrrp_asso_tbl_walk(int apn_vrid,
                   void *ref,
                   vrrp_api_asso_walk_fun_t efun,
                   int *num_asso)
{
  VRRP_GLOBAL_DATA *vrrp;
  int cnt=0, asso_ix;
  int rc;

  if (efun == NULL) {
    return VRRP_FAILURE;
  }
  vrrp = VRRP_GET_GLOBAL(apn_vrid);

  for (asso_ix=0; asso_ix<vrrp->asso_tbl_cnt; asso_ix++) {
    rc = efun(vrrp->asso_tbl[asso_ix], ref);
    switch (rc) {
    case VRRP_OK:
      cnt++;
      break;
    case VRRP_FAILURE:
      return VRRP_FAILURE;
    case VRRP_IGNORE:
    default:
      break;
    }
  }
  if (num_asso) {
    *num_asso = cnt;
  }
  return VRRP_OK;
}

/*---------------------------------------------------------------------------
 * Find first association for a given keys
 *---------------------------------------------------------------------------
*/
VRRP_ASSO *
vrrp_asso_tbl_find_first(int       apn_vrid,
                         u_int8_t  af_type,
                         u_int8_t  vrid,
                         u_int32_t ifindex)
{
  VRRP_GLOBAL_DATA *vrrp;
  VRRP_ASSO        *asso;
  int asso_ix;

  vrrp = VRRP_GET_GLOBAL(apn_vrid);

  for (asso_ix=0; asso_ix<vrrp->asso_tbl_cnt; asso_ix++) {
    asso = vrrp->asso_tbl[asso_ix];

    if (asso->af_type == af_type &&
        asso->vrid == vrid &&
        asso->ifindex == ifindex)
    {
      return asso;
    }
  }
  return NULL;
}

/*---------------------------------------------------------------------------
 * Create a new assoion
 *---------------------------------------------------------------------------
 */
VRRP_ASSO *
vrrp_asso_tbl_create_asso (VRRP_GLOBAL_DATA *vrrp,
                           u_int8_t  af_type,
                           int       vrid,
                           u_int32_t ifindex,
                           u_int8_t  *ipaddr)
{
  VRRP_ASSO *asso;

  asso = XCALLOC (MTYPE_VRRP_ASSO, (sizeof (VRRP_ASSO)));
  if (asso == NULL) {
    return (NULL);
  }
  asso->af_type = af_type;
  asso->vrid    = vrid;
  asso->ifindex = ifindex;
  asso->vrrp    = vrrp;
  if (af_type == AF_INET) {
    pal_mem_cpy(&asso->ipad_v4, ipaddr, IN_ADDR_SIZE);
  }
#ifdef HAVE_IPV6
  else {
    pal_mem_cpy(&asso->ipad_v6, ipaddr, IN6_ADDR_SIZE);
  }
#endif
  /* Default storage type */
  asso->asso_storage_type = STORAGE_TYPE_NONVOLATILE;

  if (asso != vrrp_asso_tbl_add_ref(asso))
  {
    XFREE(MTYPE_VRRP_ASSO,asso);
    return (NULL);
  }
  return (asso);
}

/*----------------------------------------------------------------------------
 * Delete assoion reference from the table.
 * Do not free its memory.
 *----------------------------------------------------------------------------
 */
int
vrrp_asso_tbl_del_ref(VRRP_ASSO *asso)
{
  VRRP_ASSO **asso_ref;
  VRRP_GLOBAL_DATA *vrrp = asso->vrrp;

  if (vrrp == NULL) {
    return (VRRP_FAILURE);
  }
  if ((asso_ref = _asso_tbl_lkup_ref(vrrp, asso)) == NULL) {
    return (VRRP_FAILURE);
  }
  if (asso_ref != pal_mem_move(asso_ref,
                               asso_ref+1,
                               (char *)(&vrrp->asso_tbl[vrrp->asso_tbl_cnt]) -
                               (char *)(asso_ref+1))) {
    return (VRRP_FAILURE);
  }
  vrrp->asso_tbl_cnt--;
  return (VRRP_OK);
}

/*----------------------------------------------------------------------------
 * Delete assoion - free memory;
 * Do not free its memory.
 *----------------------------------------------------------------------------
 */
int
vrrp_asso_tbl_delete_asso(VRRP_ASSO *asso)
{
  /* The assoion ref may not be there ... */
  vrrp_asso_tbl_del_ref(asso);
  XFREE (MTYPE_VRRP_ASSO, asso);
  return VRRP_OK;
}


/*---------------------------------------------------------------------------
 * Find and return a pointer to a assoion reference in assoion reference table
 *---------------------------------------------------------------------------
 */
static VRRP_ASSO **
_asso_tbl_lkup_ref(VRRP_GLOBAL_DATA *vrrp, VRRP_ASSO *asso)
{
  if (vrrp == NULL) {
    return (NULL);
  }
  return bsearch(&asso,
                 vrrp->asso_tbl,
                 vrrp->asso_tbl_cnt,
                 sizeof(VRRP_ASSO *),
                 (vrrp_as_cmp_fun_t)_asso_cmp);
}

#endif /* HAVE_SNMP */

