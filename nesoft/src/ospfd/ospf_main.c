/* Copyright (C) 2001-2003  All Rights Reserved. */

#include "pal.h"
#include "lib.h"
#include "version.h"
#include "if.h"
#include "thread.h"
#include "prefix.h"
#include "filter.h"
#include "log.h"
#ifdef HAVE_LICENSE_MGR
#include "licmgr.h"
#endif /* HAVE_LICENSE_MGR */

#include "ospfd/ospfd.h"
#include "ospfd/ospf_cli.h"
#include "ospfd/ospf_show.h"

#ifdef HAVE_HA
#include "fm_api.h"
#endif /* HAVE_HA */

#include "lib_fm_api.h"


void ospf_snmp_init (struct lib_globals *);

/* OSPF Globals. */
struct lib_globals *og;

/* OSPF Feature Capability variables. */
u_char ospf_cap_have_vrf_ospf;
u_char ospf_cap_have_vr;

/* OSPFd Feature Capability check. */
static void
ospf_feature_capability_check (struct lib_globals *zg)
{
#ifdef HAVE_LICENSE_MGR
  char *buf[2];
  int ret1, ret2;

  buf[0] = LIC_MGR_FEATURE_MPLS;
  ret1 = lic_mgr_get (buf, 1, LIC_MGR_VERSION_IPV4, zg);
  if (ret1 == LS_SUCCESS)
    ospf_cap_have_vrf_ospf = 1;
  else
    ospf_cap_have_vrf_ospf = 0;

  buf[0] = LIC_MGR_FEATURE_IPV4;
  buf[1] = LIC_MGR_FEATURE_OSPF;
  ret2 = lic_mgr_get (buf, 2, LIC_MGR_VERSION_IPV4, zg);
  if (ret1 != LS_SUCCESS && ret2 != LS_SUCCESS)
    {
      pal_system_err ("%s:%s", modname_strs (zg->protocol), ERR_NOLIC);
      exit (0);
    }

  return;
#else

  /* Set default feature capabilities. */
  ospf_cap_have_vrf_ospf = 1;

  return;
#endif /* HAVE_LICENSE_MGR */
}

void
ospf_lib_cli_init (struct lib_globals *zg)
{
  /* Initialize signal and lib stuff. */
  host_vty_init (zg);

  /* Initialize Access List. */
  access_list_init (zg);

  /* Initialize Prefix List. */
  prefix_list_init (zg);

#ifdef MEMMGR
  /* Init Memory Manager CLIs. */
  memmgr_cli_init (zg);
#endif /* MEMMGR */

  /* Install Route-map CLI. */
  route_map_cli_init (zg);

  /* Install SNMP CLI. */
  snmp_community_cli_init (zg);
}

/* OSPFd main routine. */
int
ospf_start (int daemon_mode, char *config_file, int vty_port, char *progname)
{
  struct thread thread;
  char buf[50];
  result_t ret;

  /* Initialize memory. */
  memory_init (APN_PROTO_OSPF);

  /* Initialize zglobals. */
  ZG = lib_create (progname);
  if (ZG == NULL)
    return -1;

  ret = lib_start (ZG);
  if (ret < 0)
    {
      lib_stop (ZG);
      return ret;
    }

  /* OSPF global init. */
  ospf_global_init ();

  /* Make default entity. */
  ZG->protocol = APN_PROTO_OSPF;
  ZG->log = openzlog (ZG, ZG->vr_instance, APN_PROTO_OSPF, LOGDEST_MAIN);

#ifdef HAVE_HA
  fm_init(ZG,NULL);
#endif /* HAVE_HA */

  /* Check OSPF Feature Capability. */
  ospf_feature_capability_check (ZG);

  /* Initialize Library CLI commands. */
  ospf_lib_cli_init (ZG);

  /* Initialize OSPF CLI commands. */
  ospf_cli_init (ZG);

  /* Initialize OSPF_CLI show commands. */
  ospf_show_init (ZG);

  /* OSPF master init. */
  ospf_master_init (apn_vr_get_privileged (ZG));

  /* Initialize OSPF protocol global data. */
  ospf_init (ZG);

#ifdef HAVE_SNMP
  /* Initialize SMUX related stuff. */
  ospf_snmp_init (ZG);
#endif /* HAVE_SNMP */

  /* Daemonize program before any log message goes out. */
  if (daemon_mode)
    pal_daemonize (0, 0);

#ifdef HAVE_PID
  PID_REGISTER (PATH_OSPFD_PID);
#endif /* HAVE_PID */

  /* Start the configuration management.  */
  host_config_start (ZG, config_file, vty_port);

  /* Print banner. */
  zlog_info (ZG, "OSPFd (%s) starts", pacos_version (buf, 50));

  /* Fetch next active thread. */
  while (thread_fetch (ZG, &thread))
    thread_call (&thread);

  return 0;
}

void
ospf_stop (mod_stop_cause_t cause)
{
  /* Mark lib in shutdown for HA */
  SET_LIB_IN_SHUTDOWN (ZG);
  SET_LIB_STOP_CAUSE(ZG, cause);

  /* Terminate OSPF.  */
  ospf_terminate (ZG);

#ifdef HAVE_PID
  PID_REMOVE (PATH_OSPFD_PID);
#endif /* HAVE_PID */

  /* Stop the system.  */
  lib_stop (ZG);
}
