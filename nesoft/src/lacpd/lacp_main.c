/* Copyright (C) 2003  All Rights Reserved. */

/* This modules defines the main interface to LACP.
   Creation of the aggregators on system startup occurs here. */

#include "pal.h"
#include "pal_socket.h"
#include "lib.h"
#include "thread.h"
#include "log.h"

#include "lacp_types.h"
#include "lacpd.h"
#include "lacp_debug.h"
#include "lacp_config.h"
#include "lacp_cli.h"
#include "lacp_link.h"
#include "lacp_timer.h"
#include "lacpdu.h"
#include "lacp_selection_logic.h"
#include "lacp_nsm.h"
#include "lacp_timer.h"

#ifdef HAVE_SNMP
#include "lacp_snmp.h"
#endif /* HAVE_SNMP */

#ifdef HAVE_HA
#include "cal_api.h"
#include "commsg.h"
#include "lib_cal.h"
#include "lacp_cal.h"
#include "nsm_client.h"
#endif /* HAVE_HA */

#ifdef HAVE_SMI
#include "smi_server.h"
#include "smi_lacp_server.h"
struct smi_server *lacp_smiserver;
#endif

#ifdef HAVE_SMI
#include "lib_fm_api.h"
#include "smi_lacp_fm.h"
#endif

struct lacp *lacp_instance = NULL;

/* Local socket connection to forwarder */

#ifdef HAVE_USER_HSL
static int sockfd = -1;
#endif /* HAVE_USER_HSL */


int
lacp_init (struct lib_globals *zg)
{
  if (lacp_instance)
    return RESULT_OK;

  /* create lacp instance */
  lacp_instance = XCALLOC (MTYPE_LACP_INSTANCE, sizeof (struct lacp));
  if (! lacp_instance)
    {
      pal_system_err("Out of memory");
      return LACP_ERR_MEM_ALLOC_FAILURE;
    }

  lacp_instance->lacpm = zg;

  lacp_debug_init ();

  lacp_instance->fwd_socket = -1;
  lacp_instance->agg_ix = LACP_AGG_IX_MIN_LIMIT;

#ifndef HAVE_USER_HSL
  if (open_lacp () < 0)
    {
      pal_system_err("Open Lacp error");
      return RESULT_ERROR;
    }
#else
  sockfd = lacp_sock_init (zg);
  if (sockfd < 0)
    {
      zlog_err (lacp_instance->lacpm, "Error opening socket (%d)", sockfd);
      lib_stop (lacp_instance->lacpm);
      return RESULT_ERROR;
    }
#endif /* HAVE_USER_HSL */

  if (lacp_nsm_init () < 0)
    {
      pal_system_err("NSM init failure");
      return RESULT_ERROR;
    }

  LACP_SYSTEM_PRIORITY = LACP_DEFAULT_SYSTEM_PRIORITY;

#ifdef HAVE_HA
  /* Committing lacp global instance */
  HA_CHKPT_CRE (LACPM, HA_CRT_LACP_INSTANCE, lacp_instance,
                 &lacp_instance->lacp_instance_cdr_ref);
#endif /* HAVE_HA */

  LACP_TIMER_ON (lacp_instance->tx_timer, lacp_tx_timer_expiry,
                 0, FAST_PERIODIC_TIME);
#ifdef HAVE_HA
  lacp_cal_create_tx_timer ();
#endif /* HAVE_HA */

  return RESULT_OK;
}

int
lacp_start (s_int32_t deamon_mode, char * config_file,
            s_int32_t vty_port, char * progname, u_int16_t ha_cold)
{
#ifdef HAVE_LICENSE_MGR
  char* buf[2];
  int ret;
#endif /* HAVE_LICENSE_MGR */
  struct lib_globals *lacpm;
  struct thread thread;
  int ret;
#ifdef HAVE_HA
  HA_RC cal_rc;
  COMMSG_HAN cm_han;
  CAL_STARTUP_TYPE ha_startup_type;
#endif /* HAVE_HA */

  memory_init (APN_PROTO_LACP);

  /* Allocate a global variable container */
  lacpm = lib_create (progname);
  if (lacpm == NULL)
    return RESULT_ERROR;

  lacpm->protocol = APN_PROTO_LACP;
  lacpm->log = openzlog (lacpm, lacpm->vr_instance, APN_PROTO_LACP,
                         LOGDEST_MAIN);

#ifdef TEST_FM
  pal_signal_set(SIGUSR1, siguser_handler);
#endif

#ifdef HAVE_SMI
  /* Initialize Fault management for SMI */
  lacp_smi_fm_init(lacpm);
#endif /* HAVE_SMI */

  /* deamon mode */
  if (deamon_mode)
    pal_daemonize (0, 0);

#ifdef HAVE_PID
  PID_REGISTER (PATH_LACPD_PID);
#endif /* HAVE_PID */

#ifdef HAVE_HA

  ha_startup_type = (ha_cold) ? CAL_STARTUP_TYPE_COLD
                              : CAL_STARTUP_TYPE_AUTO;
  cal_rc = cal_init(lacpm, ha_startup_type, NULL);
  if (cal_rc != HA_RC_OK)
    {
      return RESULT_ERROR;
    }

  lib_cal_reg_crtypes (lacpm);
  /* Register LACP data types */
  lacp_cal_reg_all_types (lacpm);

  /* Register be_active_cb */
  cal_reg_new_active_cb (HA_CAL_PTR_GET(lacpm), lacp_be_active);
#endif /* HAVE_HA */

  ret = lib_start (lacpm);
  if (ret < 0)
    {
      lib_stop (lacpm);
      return ret;
    }

#ifdef HAVE_LICENSE_MGR
  buf[0] = pal_strdup (MTYPE_TMP, LIC_MGR_FEATURE_IPV4);
  buf[1] = pal_strdup (MTYPE_TMP, LIC_MGR_FEATURE_LACP);
  ret = lic_mgr_get (buf, 2, LIC_MGR_VERSION_IPV4, lacpm);
  XFREE (MTYPE_TMP, buf[0]);
  XFREE (MTYPE_TMP, buf[1]);
  if (ret != LS_SUCCESS)
    {
      pal_system_err ("%s:%s", modname_strs(lacpm->protocol), ERR_NOLIC);
      lib_stop (lacpm);
      pal_exit (0);
    }
#endif /* HAVE_LICENSE_MGR */

  host_vty_init (lacpm);
  lacp_cli_init (lacpm);

#ifdef HAVE_SNMP
  lacp_snmp_init (lacpm);
#endif /* HAVE_SNMP */

#ifdef HAVE_SMI
  lacp_smiserver = lacp_smi_server_init (lacpm);
#endif /* HAVE_SMI */

  ret = lacp_init (lacpm);
  if (ret < 0)
    {
      lib_stop (lacpm);
      return ret;
    }
#ifdef HAVE_HA
  /* Hookup the COMMSG to protocol and vice versa. */
  cm_han = commsg_reg_tp(lacpm, lacpm->nc,
                         nsm_client_commsg_getbuf,
                         nsm_client_commsg_send);
  ret = nsm_client_commsg_init (lacpm->nc, commsg_recv, cm_han);
  if (ret) {
    return -1;
  }
  /* Once we have the message transport handle, let the CAL to know. */
  cal_audit_attach_commsg(HA_CAL_PTR_GET(lacpm), cm_han);

  /* Register for audits */
  lacp_cal_audit_init ();
#endif /* HAVE_HA */

#ifdef HAVE_HA
  zlog_info (LACPM, "Calling cal_start(CAL_STARTUP_DONE)");
  HA_RC rc = cal_start(HA_CAL_PTR_GET(lacpm), CAL_STARTUP_DONE);
  if (rc == HA_RC_STARTUP_DONE)
    zlog_info (LACPM, "CAL_STARTUP_DONE OK - ");
  else if (rc == HA_RC_CONFIG_DONE)
    zlog_info (LACPM, "CAL_STARTUP_DONE: ACTIVE startup configuration");
  else
    {
      zlog_err (LACPM, "lacp_start: cal_start failure");
      return (-1);
    }
#endif /* HAVE_HA */

  /* Start the configuration management.  */
  host_config_start (lacpm, config_file, vty_port);

  /* Start the read on an LACP socket to receive lacp frames */
  if (! lacp_instance->t_read)
    lacp_instance->t_read = thread_add_read_high (LACPM, recv_lacp, NULL,
                                                  lacp_instance->fwd_socket);
  /* Execute each thread. */
  while (thread_fetch (lacpm, &thread))
    {
      thread_call (&thread);
#ifdef HAVE_HA
      cal_txn_commit (HA_CAL_PTR_GET(lacpm));
#endif /* HAVE_HA */
    }
  return RESULT_OK;
}

void
lacp_stop (void)
{
  struct lib_globals *zg = lacp_instance->lacpm;

  /* Mark lib in shutdown for HA */
  SET_LIB_IN_SHUTDOWN (zg);

#ifdef HAVE_SMI
  /* Finish SMI server. */
  smi_server_finish (lacp_smiserver);
#endif /* HAVE_SMI */

  if (lacp_instance->tx_timer)
    {
#ifdef HAVE_HA
      lacp_cal_delete_tx_timer ();
#endif /* HAVE_HA */

      LACP_TIMER_OFF (lacp_instance->tx_timer);
    }

  LACP_READ_OFF (lacp_instance->t_read);

  lacp_aggregator_delete_all (PAL_FALSE);
  lacp_link_delete_all (PAL_FALSE);
  close_lacp ();
  lacp_nsm_deinit ();

  XFREE (MTYPE_LACP_INSTANCE, lacp_instance);
  lacp_instance = NULL;

#ifdef HAVE_PID
  PID_REMOVE (PATH_LACPD_PID);
#endif /* HAVE_PID */

  /* Stop the system.  */
  lib_stop (zg);
}
