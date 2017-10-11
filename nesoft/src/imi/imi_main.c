/* Copyright (C) 2001-2003  All Rights Reserved. */

#include "pal.h"
#ifdef HAVE_GETOPT_H
#include <getopt.h>
#endif /* HAVE_GETOPT_H */

#include "lib.h"
#include "log.h"
#include "thread.h"
#include "cli.h"
#include "line.h"
#include "snprintf.h"
#include "keychain.h"

#include "imi/imi.h"

#include "imi/imi_cli.h"
#include "imi/imi_config.h"
#include "imi/imi_line.h"
#include "imi/imi_mode.h"
#include "imi/imi_memory.h"
#include "imi/imi_server.h"
#include "imi/imi_api.h"
#ifdef HAVE_ACL
#include "imi/util/imi_filter.h"
#endif /* HAVE_ACL */

#ifdef HAVE_NAT
#include "imi/util/imi_fw.h"
#endif /* HAVE_NAT */

#ifdef HAVE_APS_TOOLKIT
#include "imi/imi_api.h"
#endif /* HAVE_APS_TOOLKIT */

#ifdef HAVE_NTP
#include "imi/util/imi_ntp.h"
#endif /* HAVE_NTP */

#ifdef HAVE_CRX
#include "crx.h"
#endif /* HAVE_CRX */

#ifdef HAVE_CUSTOM1
#include "imi/custom1/imi_snmp.h"
#endif /* HAVE_CUSTOM1 */

#ifdef HAVE_HA
#include "fm_api.h"
#endif /* HAVE_HA */

#include "fm/lib_fm_api.h"

/* Integrated configuration file. */
int boot_flag = 0;

/* Master of threads. */
struct lib_globals *imim;
struct imi_globals *imig;

void imi_igmp_cli_show_init (struct lib_globals *);
void imi_mld_cli_show_init (struct lib_globals *);

/* Access list add callback for IMI. */
void
imi_access_list_add_hook (struct apn_vr *vr,
                          struct access_list *access,
                          struct filter_list *filter)
{
#ifdef HAVE_NTP
  imi_ntp_acl_add_hook (access, filter);
#endif /* HAVE_NTP */

}

/* Access list delete callback for IMI. */
void
imi_access_list_delete_hook (struct apn_vr *vr,
                             struct access_list *access,
                             struct filter_list *filter)
{
#ifdef HAVE_NTP
  imi_ntp_acl_delete_hook (access, filter);
#endif /* HAVE_NTP */
}

/* Access list notification callback. */
result_t
imi_access_list_ntf_cb (struct apn_vr *vr,
                        struct access_list *access,
                        struct filter_list *filter,
                        filter_opcode_t     opcode)
{
#if defined HAVE_ACL || defined HAVE_NAT
   result_t res;
#endif /* defined HAVE_ACL || HAVE_NAT */

#ifdef HAVE_ACL
  if (IS_APN_VR_PRIVILEGED(vr))
     {
       res = imi_filter_acl_ntf_cb (access, filter, opcode);
       if (res != LIB_API_SET_SUCCESS)
         return res;
     }
#endif /* HAVE_ACL */

#ifdef HAVE_NAT
  if (IS_APN_VR_PRIVILEGED(vr))
    {
      res = imi_nat_acl_ntf_cb (access, filter, opcode);
      if (res != LIB_API_SET_SUCCESS)
        return res;
    }
#endif /* HAVE_NAT */
return LIB_API_SET_SUCCESS;
}

/* Initialize access-list add and delete callback hooks. */
result_t
imi_access_list_callback_init (struct lib_globals *zg)
{
  struct apn_vr *vr;

  vr = apn_vr_get_privileged (zg);

  access_list_add_hook (vr, imi_access_list_add_hook);
  access_list_delete_hook (vr, imi_access_list_delete_hook);

  return 0;
}

void
imi_add_callback (struct lib_globals *zg, enum imi_callback_type type,
                  int (*func) (struct apn_vr *))
{
  struct imi_globals *imig = zg->proto;

  if (type < 0 || type >= IMI_CALLBACK_MAX)
    return;

  imig->imi_callback[type] = func;
}

void
imi_config_file_set (struct apn_vr *vr, char *config_file)
{
  struct imi_master *im = vr->proto;
  struct host *host = vr->host;
  char buf[MAXPATHLEN];
  char file[MAXPATHLEN];
  char *path;

  /* Update configuration for config filename... */
  if (im->filename)
    {
      XFREE (MTYPE_CONFIG, im->filename);
      im->filename = NULL;
    }
  if (config_file)
    im->filename = XSTRDUP (MTYPE_CONFIG, config_file);

  if (host->config_file)
    XFREE (MTYPE_CONFIG, host->config_file);

  /* Get the configuration file name.  */
  if (config_file)
    zsnprintf (file, sizeof file, "%s", config_file);
  else
    zsnprintf (file, sizeof file, "PacOS.%s", PAL_FILE_SUFFIX);

  /* Get the path name.  */
  if (vr->name)
    {
      zsnprintf (buf, sizeof buf, "%s%c%s",
                 vr->zg->cwd, PAL_FILE_SEPARATOR, vr->name);
      path = buf;
    }
  else
    path = vr->zg->cwd;

  /* Make sure the path existence.  */
  pal_mkdir (path, PAL_DIR_MODE);

  /* Create a configuration file name.  */
  zsnprintf (buf, sizeof buf, "%s%c%s", path, PAL_FILE_SEPARATOR, file);

  host->config_file = XSTRDUP (MTYPE_CONFIG, buf);
}

struct imi_master *
imi_master_init (struct apn_vr *vr)
{
  struct imi_master *im;
#ifdef HAVE_VLOGD
  modbmap_t def_mbm = modbmap_or(PM_NSM, PM_VLOG);
#else
  modbmap_t def_mbm = PM_NSM;
#endif

  im = XCALLOC (MTYPE_IMI_MASTER, sizeof (struct imi_master));
  im->vr = vr;
  vr->proto = im;

  /* PMs will be loaded by "load" CLI.  */
  if (vr->name != NULL)
    im->module = im->module_config = def_mbm;
  else
    im->module_config = PM_ALL;

  imi_config_file_set (vr, NULL);

  /* Prepare IMI line vector.  */
  imi_line_init (im, LINE_TYPE_CONSOLE, LINE_CONSOLE_DEFAULT);
  imi_line_init (im, LINE_TYPE_AUX, LINE_AUX_DEFAULT);
  imi_line_init (im, LINE_TYPE_VTY, LINE_VTY_DEFAULT);

  return im;
}

void
imi_master_finish (struct apn_vr *vr)
{
  struct imi_master *im = vr->proto;

  /* Free the IMI line vector.  */
  imi_line_shutdown (im, LINE_TYPE_CONSOLE);
  imi_line_shutdown (im, LINE_TYPE_AUX);
  imi_line_shutdown (im, LINE_TYPE_VTY);

  /* Free Description */
  if (im->desc != NULL)
    XFREE (MTYPE_IMI_STRING, im->desc);

  /* Free the configuration file.  */
  if (im->filename != NULL)
    XFREE (MTYPE_CONFIG, im->filename);

  XFREE (MTYPE_IMI_MASTER, im);

  vr->proto = NULL;
}

/* IMI service initialization. */
void
imi_init (struct lib_globals *zg, u_int16_t vty_port)
{
  /* Initialize IMI top structure. */
  imig = XCALLOC (MTYPE_IMI, sizeof (struct imi_globals));
  pal_mem_set (imig, 0, sizeof (struct imi_globals));

  /* Link together.  */
  imig->zg = zg;
  zg->proto = imig;

  /* Init CLI tree. */
  imi_cli_init (vty_port);

  /* Initialize IMI server.  */
  imi_server_init (zg);

  /* Initialize IMI interfaces. */
  imi_if_init (zg);

#ifdef HAVE_DNS
  /* Initialize DNS Client. */
  imi_dns_init (imig);
#endif /* HAVE_DNS */

#ifdef HAVE_DHCP_CLIENT
  /* Initialize DHCP Client. */
  imi_dhcpc_init ();
#endif /* HAVE_DHCP_CLIENT */

#ifdef HAVE_DHCP_SERVER
  /* Initialize DHCP Server. */
  imi_dhcps_init (imig);
#endif /* HAVE_DHCP_SERVER */

#ifdef HAVE_ACL
  /* Initialize ACL service.  */
  imi_acl_init (zg);
#endif /* HAVE_ACL */

#ifdef HAVE_NAT
  /* Initialize NAT/Firewall. */
  imi_nat_init (zg);
#endif /* HAVE_NAT */

#ifdef HAVE_PPPOE
  /* Initialize PPPOE. */
  imi_pppoe_init ();
#endif /* HAVE_PPPOE */

#ifdef HAVE_NTP
  /* Initialize NTP. */
  imi_ntp_init ();
#endif /* HAVE_NTP */

  /* Access-list. */
  imi_access_list_init (zg);

  /* Access list callback init. */
  imi_access_list_callback_init (zg);
 
  /* Register new access-list notification callback. */
   filter_set_ntf_cb (zg, imi_access_list_ntf_cb);

  /* Routemap. */
  route_map_cli_init (zg);

  /* snmp community */
  snmp_community_cli_init (zg);

  /* Prefix-list. */
  imi_prefix_list_init (zg);

  /* Keychain. */
  keychain_cli_init (zg);

#ifdef HAVE_SNMP
  /* Initialize snmp debugging. */
  imi_snmp_debug_cli_init (imim);
#endif /* HAVE_SNMP */

  /* Logging. */
  zlog_cli_init_imi (zg->ctree);
}

void
imi_terminate (struct lib_globals *zg)
{
#ifdef HAVE_DNS
  imi_dns_shutdown ();
#endif /* HAVE_DNS */

#ifdef HAVE_DHCP_SERVER
  imi_dhcps_shutdown ();
#endif /* HAVE_DHCP_SERVER */

#ifdef HAVE_DHCP_CLIENT
  imi_dhcpc_shutdown ();
#endif /* HAVE_DHCP_CLIENT */

#ifdef HAVE_NAT
  imi_nat_finish (zg);
#endif /* HAVE_NAT */

#ifdef HAVE_ACL
  /* Finish ACL service.  */
  imi_acl_finish (zg);
#endif /* HAVE_ACL */

#ifdef HAVE_NTP
  /* Finish NTP service.  */
  imi_ntp_deinit (zg);
#endif /* HAVE_NTP */

#ifdef HAVE_APS_TOOLKIT
  /* External module shutdown. */
  imi_extern_shutdown ();
#endif /* HAVE_APS_TOOLKIT */

  /* Shutdown IMI server.  */
  imi_server_shutdown (zg);
}

/* IMI main routine. */
int
imi_start (int daemon_mode, char *config_file, int vty_port,
           char *progname, int boot_flag_param)
{
  struct thread thread;
  result_t ret;

  /* Initialize memory. */
  memory_init (APN_PROTO_IMI);

  /* Allocate lib globals. */
  imim = lib_create (progname);
  if (! imim)
    return -1;

  ret = lib_start (imim);
  if (ret < 0)
    {
      lib_stop (imim);
      return ret;
    }

  /* Make default entity. */
  imim->protocol = APN_PROTO_IMI;
  imim->log = openzlog (imim, 0, APN_PROTO_IMI, LOGDEST_MAIN);
#ifdef HAVE_VLOGD
  imi_vlog_init(imim);
#endif
  /* Initialize IMI. */
  imi_init (imim, vty_port);

  /* Initialize IMI master for PVR. */
  imi_master_init (apn_vr_get_privileged (imim));

#ifdef HAVE_APS_TOOLKIT
  /* External module initialization  */
  imi_extern_init ();
#endif /* HAVE_APS_TOOLKIT */

  /* Config init (show-running CLIs). */
  imi_config_init (imim);

  /* Initialize NSM Client connection. */
  imi_nsm_init (imim);

#ifdef HAVE_CRX
  /* Initialize clustering. */
  crx_init ();
#endif /* HAVE_CRX. */

#ifdef MEMMGR
  /* Memory CLIs. */
  imi_memory_init (imim);
#endif /* MEMMGR. */

#ifdef HAVE_CUSTOM1
  /* Additional custom1 CLIs. */
  imi_snmp_mngt_init (imim);
#endif /* HAVE_CUSTOM1 */

#ifdef HAVE_IMI_WAN_STATUS
  /* Initialize default WAN status. */
  imi_wan_status_reset_default ();
#endif /* HAVE_IMI_WAN_STATUS */

  /* Turn into daemon if daemon_mode is set. */
  if (daemon_mode)
    pal_daemonize (0, 0);

#ifdef HAVE_PID
  PID_REGISTER (PATH_IMI_PID);
#endif /* HAVE_PID */

  /* Initialize modes. */
  imi_mode_init (imim);

#ifndef HAVE_IMISH
  /* Sort installed commands. */
  cli_sort (imim->ctree);
#endif /* !HAVE_IMISH */

#ifndef HAVE_CRX
  /* Load the startup configuration.  */
  IMI_STARTUP_CONFIG (imim, config_file);
#endif /* !HAVE_CRX */

  /* Configuration file is read.  */
  boot_flag = 1;

#ifdef HAVE_IMI_DEFAULT_GW
  /* Initialize IMI utils after configuration is finished. */
  imi_util_init ();
#endif /* HAVE_IMI_DEFAULT_GW */

  while (thread_fetch (imim, &thread))
    thread_call (&thread);

  return 0;
}

void
imi_stop (void)
{
  /* Mark lib in shutdown for HA */
  SET_LIB_IN_SHUTDOWN (imim);

  /* Terminate IMI.  */
  imi_terminate (imim);

#ifdef HAVE_PID
  /* Remove pid file. */
  PID_REMOVE (PATH_IMI_PID);
#endif /* HAVE_PID */

  /* Stop the system. */
  lib_stop (imim);
}
