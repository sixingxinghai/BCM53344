/*=============================================================================
**
** Copyright (C) 2008-2009   All Rights Reserved.
**
** vlog_api.c - Implementation of the VLOG initialization/termination code.
*/

#include "pal.h"
#include "lib.h"
#include "thread.h"
#include "linklist.h"
#include "log.h"
#include "cli.h"
#include "snprintf.h"
#include "line.h"

#include "vlogd.h"
#include "vlog_server.h"
#include "imi_client.h"

/* The only global variable here. */
VLOG_GLOBALS *vlog_vgb = NULL;

/* Function used to Initialize VLOG globals. */

ZRESULT
vlog_globals_init (struct lib_globals *zg)
{
  /* Initialize IMI top structure. */
  vlog_vgb = XCALLOC (MTYPE_VLOG_GLOBAL, sizeof (struct vlog_globals));
  if (vlog_vgb == NULL)
    return ZRES_ERR;

  /* Create a container of all active terminals. */
  vlog_vgb->vgb_terms = vlog_terms_new();
  if (vlog_vgb->vgb_terms == NULL)
    {
      XFREE (MTYPE_VLOG_GLOBAL, vlog_vgb);
      return ZRES_ERR;
    }
  /* Link together.  */
  vlog_vgb->vgb_lgb = zg;
  zg->proto         = vlog_vgb;

  return ZRES_OK;
}

ZRESULT
vlog_globals_deinit (struct lib_globals *zg)
{
  struct apn_vr *vr = NULL;
  struct vlog_globals *vgb = zg->proto;

  vr = apn_vr_get_privileged (zg);

  if (!vr)
    return ZRES_ERR;

  vlog_terms_finish(vgb->vgb_terms);

  vlog_vrs_delete();

  THREAD_OFF(vlog_vgb->vgb_timer_thread);

  XFREE (MTYPE_VLOG_GLOBAL, vgb);
  zg->proto = NULL;

  return ZRES_OK;
}

/* Terminate VLOG related Information. */
void
vlog_terminate (struct lib_globals *zg)
{
  struct vlog_globals *vgb = zg->proto;

  /* Finish VLOG server. */
  vlog_server_finish (vgb->vgb_server);

  /* De-init VLOG Global data */
  vlog_globals_deinit (zg);
}

/* Called at the time of lost connection to IMI  server.
 */
static void
_vlog_imi_srv_disconnect_cb(struct lib_globals *zg)
{
  char msg[120];

  pal_sprintf(msg,  "IMI shutdown - Terminal will stop in %d sec.",
              VLOG_TERM_SHUTDOWN_GRACE_PERIOD);
  vlog_terms_broadcast (vlog_vgb->vgb_terms, msg);
  vlog_terms_init_shutdown(vlog_vgb->vgb_terms);
}

/* Called at the time of disrepancy between old and new VR
   name -> id mapping.
 */
static int
_vlog_delete_vr_proto(struct apn_vr *ivr)
{
  /* Delete VR upon detection of change in the VR id */
  vlog_vr_proto_delete(ivr->proto);
  return 0;
}


/* Main routine of VLOGd. */
int
vlog_start (int daemon_mode, char *config_file, int vty_port, char *progname)
{
  struct thread thread;
  result_t ret;
  struct lib_globals *zg;

  /* Initialize memory. */
  memory_init (APN_PROTO_VLOG);

  /* Allocate global variable container. */
  zg = lib_create (progname);
  if (zg == NULL)
    return -1;

  ret = lib_start (zg);
  if (ret < 0)
    {
      lib_stop (zg);
      return ret;
    }

  zg->protocol = APN_PROTO_VLOG;
  zg->log = openzlog (zg, zg->vr_instance, APN_PROTO_VLOG, LOGDEST_MAIN);

  vlog_lib_init(zg);

  /* VLOG globals init. */
  if (vlog_globals_init (zg) != ZRES_OK)
    return -1;

  if (vlog_vr_new(NULL, 0) == NULL)
    return -1;

  /* Vty related initialize. */
  host_vty_init (zg);

  /* Start the configuration management.  */
  host_config_start (zg, config_file, vty_port);

  /* Initialize VLOG CLIs.  */
  vlog_cli_init (zg);

#ifndef HAVE_IMISH
  /* Start VTY Server Configuration. */
  vlog_vty_init (zg, vty_port, config_file);
#endif /* HAVE_IMISH. */

  /* PacOS related initialize. */
  vlog_vgb->vgb_server = vlog_server_init (zg);

  /* IMI CLIENT Disconnect Handler. */
  imi_client_register_event_ntf_cb(zg,
                                   IMI_CLT_NTF_SRV_DISCON,
                                   _vlog_imi_srv_disconnect_cb);

  apn_vr_add_callback (zg, VR_CALLBACK_DELETE, _vlog_delete_vr_proto);

  /* Change to the daemon program. */
  if (daemon_mode)
    pal_daemonize (0, 0);

#ifdef HAVE_PID
  PID_REGISTER (PATH_VLOGD_PID);
#endif /*  HAVE_PID. */

  /* Execute each thread. */
  while (thread_fetch (zg, &thread)) {
    thread_call (&thread);
  }

  /* Not reached. */
  return (0);
}

/* Stop VLOG process. */
void
vlog_stop (void)
{
  struct lib_globals *zg = vlog_vgb->vgb_lgb;

  /* Mark lib in shutdown for HA */
  SET_LIB_IN_SHUTDOWN (zg);

  /* Terminate VLOG.  */
  vlog_terminate (zg);

#ifdef HAVE_PID
  PID_REMOVE (PATH_VLOGD_PID);
#endif /* HAVE_PID */

  /* Stop the system. */
  lib_stop (zg);
}


/*---------------------------------------------------------------
 * Callbacks to communicate with lib/log library.
 * The lib.log library hosts the "log file" CLI handler.
 */
static ZRESULT
_vlog_vr_file_set_cb(struct lib_globals *zg, u_int32_t vrid, char *fname)
{
  /* Call the API - if OK - save the file name in VLOG_VR.
   */
  return vlog_set_log_file (vrid, fname);
}

static ZRESULT
_vlog_vr_file_unset_cb(struct lib_globals *zg, u_int32_t vrid)
{
  return vlog_unset_log_file (vrid);
}


static ZRESULT
_vlog_vr_file_get_cb(struct lib_globals *zg, u_int32_t vrid, char **pfname)
{
  return vlog_get_log_file (vrid, pfname);
}

/* This will install all the callbacks in the local lib */
ZRESULT
vlog_lib_init(struct lib_globals *zg)
{
  lib_reg_vlog_cbs(zg,
                   _vlog_vr_file_set_cb,
                   _vlog_vr_file_unset_cb,
                   _vlog_vr_file_get_cb);
  return ZRES_OK;
}


