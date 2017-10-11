/* Copyright (C) 2003  All Rights Reserved.

MSTP Daemon main (contains initialization and dispatch routines).

*/

#include "pal.h"
#include "lib.h"
#include "thread.h"
#include "log.h"
#include "l2_debug.h"
#include "l2lib.h"

#include "mstp_config.h"
#include "mstp_types.h"
#include "mstpd.h"
#include "mstp_sock.h"
#include "mstp_cli.h"
#include "mstp_bridge.h"
#include "mstp_bpdu.h"
#include "mstp_port.h"
#include "mstp_api.h"
#include "mstp_snmp.h"
#ifdef HAVE_WMI
#include "mstp_wmi.h"
#endif
#ifdef HAVE_HA
#include "cal_api.h"
#include "commsg.h"
#include "lib_cal.h"
#include "mstp_cal.h"
#include "nsm_client.h"
#endif /* HAVE_HA */

#ifdef HAVE_SMI
#include "lib_fm_api.h"
#include "smi_server.h"
#include "smi_mstp_server.h"
struct smi_server *mstp_smiserver;
#endif /* HAVE_SMI */

#ifdef HAVE_SMI
#include "smi_mstp_fm.h"
#include "smi_message.h"
#endif /* HAVE_SMI */

#define MAX_ERROR 10
/* Global variable container. */
struct lib_globals * mstpm;

/* Local socket connection to forwarder */
static int sockfd = RESULT_ERROR;

/* Forward declarations. */
void mstp_nsm_init (struct lib_globals * zg);

int
mstp_start (s_int32_t deamon_mode, char *config_file,
            s_int32_t vty_port, char * progname, u_int16_t ha_cold)
{
  /* read config info ?? */
  struct thread thread;
  int ret;
#ifdef HAVE_LICENSE_MGR
  char* buf[2];
#endif /* HAVE_LICENSE_MGR */
#ifdef HAVE_HA
  HA_RC cal_rc;
  CAL_STARTUP_TYPE ha_startup_type;
  COMMSG_HAN cm_han;
#endif /* HAVE_HA */

  /* Initialize memory */
  memory_init (APN_PROTO_MSTP);

  /* Allocate a global variable container */
  mstpm = lib_create (progname);
  if (mstpm == NULL)
    return RESULT_ERROR;

  mstpm->protocol = APN_PROTO_MSTP;
  mstpm->vr_instance = 0;
  mstpm->log = openzlog (mstpm, mstpm->vr_instance, APN_PROTO_MSTP,
                         LOGDEST_MAIN);
#ifdef TEST_FM
  pal_signal_set(SIGUSR1, siguser_handler);
#endif

#ifdef HAVE_SMI
  /* Initialize Fault management */
  mstp_smi_fm_init(mstpm);

#endif /* HAVE_SMI */

  /* deamon mode */
  if (deamon_mode)
    pal_daemonize (0, 0);

#ifdef HAVE_PID
  PID_REGISTER (PATH_MSTPD_PID);
#endif /* HAVE_PID */

#ifdef HAVE_HA
  ha_startup_type = (ha_cold) ? CAL_STARTUP_TYPE_COLD
                              : CAL_STARTUP_TYPE_AUTO;
  cal_rc = cal_init (mstpm, ha_startup_type, NULL);
  if (cal_rc != HA_RC_OK)
    {
      return RESULT_ERROR;
    }

  lib_cal_reg_crtypes (mstpm);

  /* Register MSTP data types */
  mstp_cal_reg_all_types (mstpm);

  /* Register be_active_cb */
  cal_reg_new_active_cb (HA_CAL_PTR_GET(mstpm), mstp_be_active);

#endif /* HAVE_HA */

  ret = lib_start (mstpm);
  if (ret < 0)
    {
      lib_stop (mstpm);
      return ret;
    }

#ifdef HAVE_LICENSE_MGR
  buf[0] = pal_strdup (MTYPE_TMP, LIC_MGR_FEATURE_IPV4);
  buf[1] = pal_strdup (MTYPE_TMP, LIC_MGR_FEATURE_MSTP);
  ret = lic_mgr_get (buf, 2, LIC_MGR_VERSION_IPV4, mstpm);
  XFREE (MTYPE_TMP, buf[0]);
  XFREE (MTYPE_TMP, buf[1]);
  if (ret != LS_SUCCESS)
    {
      pal_system_err ("%s:%s", modname_strs(mstpm->protocol), ERR_NOLIC);
      exit(0);
    }
#endif /* HAVE_LICENSE_MGR */

  host_vty_init (mstpm);
  mstp_nsm_init (mstpm);

  /* open a STP socket to recieve BPDU frames */
  sockfd = mstp_sock_init (mstpm);
  if (sockfd < 0)
    {
      zlog_err (mstpm, "Error opening socket (%d)", sockfd);
      lib_stop (mstpm);
      return RESULT_ERROR;
    }

  mstp_bridge_cli_init (mstpm);

#ifdef HAVE_WMI
  mstp_wmi_init (mstpm);
#endif /* HAVE_WMI */

#ifdef HAVE_SNMP
  mstp_snmp_init (mstpm);
#endif /* HAVE_SNMP */

#ifdef HAVE_SMI
  mstp_smiserver = mstp_smi_server_init (mstpm);
#endif /* HAVE_SMI */

#ifdef HAVE_HA
  /* Hookup the COMMSG to protocol and vice versa. */
  cm_han = commsg_reg_tp(mstpm, mstpm->nc,
                         nsm_client_commsg_getbuf,
                         nsm_client_commsg_send);
  ret = nsm_client_commsg_init (mstpm->nc, commsg_recv, cm_han);
  if (ret) {
    return -1;
  }
  /* Once we have the message transport handle, let the CAL to know. */
  cal_audit_attach_commsg(HA_CAL_PTR_GET(mstpm), cm_han);

  /* Register for audits */
  mstp_cal_audit_init ();
#endif /* HAVE_HA */

#ifdef HAVE_HA
  zlog_info (mstpm, "Calling cal_start(CAL_STARTUP_DONE)");
  HA_RC rc = cal_start(HA_CAL_PTR_GET(mstpm), CAL_STARTUP_DONE);
  if (rc == HA_RC_STARTUP_DONE)
    zlog_info (mstpm, "CAL_STARTUP_DONE OK - ");
  else if (rc == HA_RC_CONFIG_DONE)
    zlog_info (mstpm, "CAL_STARTUP_DONE: ACTIVE startup configuration");
  else
    {
      zlog_err (mstpm, "mstp_start: cal_start failure");
      return (-1);
    }
#endif /* HAVE_HA */

  /* Start the configuration management.  */
  host_config_start (mstpm, config_file, vty_port);

#ifndef HAVE_USER_HSL
  /* Add the recv thread to thread list  */
  thread_add_read_high (mstpm, mstp_recv, NULL, sockfd);
#endif /* HAVE_USER_HSL */

  /* Execute each thread. */
  while (thread_fetch (mstpm, &thread))
    {
      thread_call (&thread);
#ifdef HAVE_HA
      cal_txn_commit (HA_CAL_PTR_GET(mstpm));
#endif /* HAVE_HA */
    }

  return RESULT_OK;
}

void
mstp_reset (void)
{
  struct mstp_bridge *br;
  struct mstp_bridge *brnext;

  br = mstp_get_first_bridge();
  while ( br )
    {
      brnext = br->next;
      mstp_bridge_delete (br->name, NULL);
      br = br->next;
    }
}

void
mstp_stop (void)
{
  /* Mark lib in shutdown for HA */
  SET_LIB_IN_SHUTDOWN (mstpm);

#ifdef HAVE_SMI
  /* Finish API server. */
  smi_server_finish (mstp_smiserver);
#endif /* HAVE_SMI */

  /* Terminate MSTP.  */
  mstp_bridge_terminate ();

#ifdef HAVE_PID
  PID_REMOVE (PATH_MSTPD_PID);
#endif /* HAVE_PID */

  /* Stop the system. */
  lib_stop (mstpm);
  mstpm = NULL;
}
