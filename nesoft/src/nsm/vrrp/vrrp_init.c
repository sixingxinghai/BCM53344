/* Copyright (C) 2001-2003  All Rights Reserved. */

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

/*----------------------------------------------------------------------------
 * Initialization of the VRRP component
 *----------------------------------------------------------------------------
 */
void
vrrp_init (struct nsm_master *nm)
{
  struct vrrp_global *vrrp;

  int ret;

  /* Allocate space for the VRRP Global Data. */
  vrrp = XCALLOC (MTYPE_VRRP_GLOBAL_INFO, (sizeof (VRRP_GLOBAL_DATA)));

  /* Set pointer. */
  vrrp->nm = nm;
  nm->vrrp = vrrp;

  /* Fill in the VRRP virtaul mac information - used for promiscuous mode */
  vrrp->vrrp_mac.v.mbyte[0] = 0x00;
  vrrp->vrrp_mac.v.mbyte[1] = 0x00;
  vrrp->vrrp_mac.v.mbyte[2] = 0x5e;
  vrrp->vrrp_mac.v.mbyte[3] = 0x00;
  vrrp->vrrp_mac.v.mbyte[4] = 0x01;
  vrrp->vrrp_mac.v.mbyte[5] = 0xFF;

  /* Create the session table   */
  ret = vrrp_sess_tbl_create (vrrp);
  if (ret != VRRP_OK)
    zlog_err (nzg,
              "VRRP Error: Cannot create session table during initialization of VR %d\n",
              nm->vr->id);
#ifdef HAVE_SNMP
  /* Create the association table   */
  ret = vrrp_asso_tbl_create (vrrp);
  if (ret != VRRP_OK)
    zlog_err (nzg,
              "VRRP Error: Cannot create associotion table during initialization of VR %d\n",
              nm->vr->id);
#endif

  /* Call any System layer initialization */
  vrrp_sys_init (vrrp);

  /* Call any IP layer initialization */
  vrrp_ip_init (vrrp);
#ifdef HAVE_IPV6
  vrrp_ipv6_init (vrrp);
#endif /* HAVE_IPV6 */
  /* Call any Forwarding Layer initialization */
  vrrp_fe_init (vrrp);

  /* Init VRRP debug. */
  vrrp_debug_init ();

#ifdef HAVE_SNMP
  vrrp_snmp_init (nzg);

  /* Set default value for notificationcntl */
    vrrp->notificationcntl = VRRP_NOTIFICATIONCNTL_ENA;
#endif
}

/*
** vrrp_close - Shutdown VRRP gracefully; restore original Mac addresses.
*/
void
vrrp_close (struct nsm_master *nm)
{
  struct vrrp_global *vrrp = nm->vrrp;
  VRRP_SESSION *sess;
  int ret;
  int sess_ix;

  /* Already shutdown? */
  if (!vrrp)
    return;

  for (sess_ix=0; sess_ix<vrrp->sess_tbl_cnt; sess_ix++)
  {
    sess = vrrp->sess_tbl[sess_ix];

    /* If session id invalid, continue. */
    if (sess == NULL)
      continue;

    if (sess->state != VRRP_STATE_INIT) {
      /* Disable the session. */
      ret = vrrp_shutdown_sess (sess);
      if (ret != VRRP_OK)
        zlog_err (nzg, "VRRP Error: Unexpected error disabling VRRP session\n");
    }
    vrrp_if_del_sess(sess);
  }
  vrrp_sess_tbl_delete(vrrp);

  /* Free VRRP data and cancel threads. */
  if (vrrp->t_vrrp_read)
    thread_cancel (vrrp->t_vrrp_read);
  if (vrrp->t_vrrp_timer)
    thread_cancel (vrrp->t_vrrp_timer);

  /* Stop PAL -- currently we only support for PVR. */
  if (vrrp->nm->vr->id == 0)
    nsm_fea_vrrp_stop (nzg);

  /* Free global data. */
  XFREE (MTYPE_VRRP_GLOBAL_INFO, vrrp);
  nm->vrrp = NULL;
}

void
vrrp_close_all (struct lib_globals *zg)
{
  struct apn_vr *vr;
  struct nsm_master *nm;
  int i;

  for (i = 0; i < vector_max (zg->vr_vec); i++)
    if ((vr = vector_slot (zg->vr_vec, i)))
      if ((nm = vr->proto))
        vrrp_close (nm);
}
