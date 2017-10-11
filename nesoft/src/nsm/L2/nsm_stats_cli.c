/* Copyright (C) 2001-2003  All Rights Reserved. */

#include "pal.h"
#include <pal.h>
#include "lib.h"
#include "if.h"
#include "table.h"
#include "prefix.h"
#include "thread.h"
#include "nexthop.h"
#include "cli.h"
#include "host.h"
#include "snprintf.h"
#include "ptree.h"
#include "filter.h"
#include "lib_mtrace.h"
#include "hash.h"
#include "message.h"
#include "nsm_message.h"

#ifdef HAVE_HAL
#include "hal_incl.h"
#endif /* HAVE_HAL */
#include "nsm/nsmd.h"
#include "nsm/nsm_interface.h"
#ifdef HAVE_L3
#include "nsm/rib/nsm_table.h"
#include "nsm/rib/rib.h"
#endif /* HAVE_L3 */
#include "nsm/nsm_debug.h"
#include "nsm/nsm_api.h"
#ifdef HAVE_USR_HSL
#include "hal/hsl/hal_msg.h"
#endif /* HAVE_USR_HSL */
#include "nsm_stats_api.h"
#ifdef HAVE_QOS
#include "nsm/nsm_qos.h"
#include "nsm/nsm_qos_cli.h"
#endif /* HAVE_QOS */


#ifdef HAVE_USER_HSL

/* Function to Display CLI Error */
int
nsm_stats_cli_return (struct cli *cli, int ret)
{
  char *str;

  switch (ret)
    {
    case NSM_API_GET_SUCCESS:
      return CLI_SUCCESS;

    case NSM_API_ERR_STATS_VLAN_ID_INVALID:
      str = " Statistics VLAN-ID is Invalid";
      break;
    case NSM_API_ERR_STATS_VLAN_ID_FAILED:
      str = " Statistics for Vlan operation Failed";
      break;
    case NSM_API_ERR_STATS_PORT_ID_INVALID:
      str = " Statistics Port_Id is Invalid";
      break;
    case NSM_API_ERR_STATS_PORT_ID_FAILED:
      str = " Statistics for Port operation Failed";
      break;
    case NSM_API_ERR_STATS_HOST_ID_FAILED:
      str = " Statistics for Host operation Failed";
      break;
    case NSM_API_ERR_STATS_CLEAR_FAILED:
      str = " Statistics Clear Operation Failed";
      break;
    default:
      return CLI_SUCCESS;
    }
  cli_out (cli, "%% %s\n", str);
  return CLI_ERROR;
}

CLI (show_statistics_vlan,
     show_statistics_vlan_cmd,
     "show statistics vlan <1-4094>",
     CLI_SHOW_STR,
     CLI_STATS_STR,
     "Display VLAN information",
     "VLAN id")
{
  u_int32_t ret;
  u_int32_t vlan_id;
  struct hal_stats_vlan *vlan;

  vlan = XCALLOC(MTYPE_HSL_SERVER_ENTRY,sizeof(struct hal_stats_vlan));

  CLI_GET_UINT32 ("Vlan Id", vlan_id, argv[0]);

   /* Function to call the nsm to talk with hal */
   ret = nsm_statistics_vlan_get (vlan_id, vlan);

   if (ret != HAL_SUCCESS)
     {
       XFREE (MTYPE_HSL_SERVER_ENTRY, vlan);
       ret = NSM_API_ERR_STATS_VLAN_ID_FAILED;
     }
   else
     {
       cli_out (cli, "Vlan-Id      : %d  \n", vlan_id );
       cli_out (cli, "UcastPkts    : %d  \n", vlan->ucastpkts  );
       cli_out (cli, "UcastBytes   : %d  \n", vlan->ucastbytes );
       cli_out (cli, "McastPkts    : %d  \n", vlan->mcastpkts  );
       cli_out (cli, "McastBytes   : %d  \n", vlan->mcastbytes );
     }

   XFREE (MTYPE_HSL_SERVER_ENTRY, vlan);
   return nsm_stats_cli_return (cli, ret);
}


CLI (clear_statistics,
     clear_statistics_cmd,
     "clear statistics (IFNAME)",
     CLI_CLEAR_STR,
     CLI_STATS_STR,
     "Port Number")
{
  u_int32_t ret;
  u_int32_t port_id = 0;
  struct apn_vr *vr = cli->vr;
  struct interface *ifp;
  struct hal_stats_port *port;

 /* Get interface statistics if available */
#if !(defined HAVE_HAL) || defined HAVE_SWFWDR
  NSM_IF_STAT_UPDATE (vr);
#endif /* !HAVE_HAL || HAVE_SWFWDR  */

  port = XCALLOC(MTYPE_HSL_SERVER_ENTRY,sizeof(struct hal_stats_port));

  if (argc != 0)
    {
      ifp = if_lookup_by_name (&vr->ifm, argv[0]);
      if (ifp == NULL
#ifdef HAVE_SUPPRESS_UNMAPPED_IF
          || ! CHECK_FLAG (ifp->status, NSM_INTERFACE_MAPPED)
#endif /* HAVE_SUPPRESS_UNMAPPED_IF */ 
          )
        {
          cli_out (cli, "%% Can't find interface %s\n", argv[0]);
          XFREE (MTYPE_HSL_SERVER_ENTRY, port);
          return CLI_ERROR;
        }

        port_id = ifp->ifindex;
    }

  /* Function to call the nsm to talk with hal  */
  ret = nsm_statistics_clear (port_id);

  if (ret != HAL_SUCCESS)
    {
      XFREE (MTYPE_HSL_SERVER_ENTRY, port);
      ret = NSM_API_ERR_STATS_CLEAR_FAILED;
    }
  else 
    {
      XFREE (MTYPE_HSL_SERVER_ENTRY, port);
    }
  return nsm_stats_cli_return (cli, ret);
}


CLI (show_statistics_host,
     show_statistics_host_cmd,
     "show statistics host",
     CLI_SHOW_STR,
     CLI_STATS_STR,
     "IP domain-name, lookup style and nameservers")
{
  u_int32_t ret;
  struct hal_stats_host *host;

  host = XCALLOC(MTYPE_HSL_SERVER_ENTRY,sizeof(struct hal_stats_host));

  /* Function to call the nsm to talk with hal */
  ret = nsm_statistics_host_get(host);

  if (ret != HAL_SUCCESS)
    {
      XFREE (MTYPE_HSL_SERVER_ENTRY, host);
      ret = NSM_API_ERR_STATS_HOST_ID_FAILED;
    }
  else
    {
      cli_out (cli, "HostInPkts     : %ld \n", host->hostInpkts);
      cli_out (cli, "HostOutPkts    : %ld \n", host->hostOutpkts);
      cli_out (cli, "HostOutErrs    : %ld \n", host->hostOuterrs);
      cli_out (cli, "HostLearnDrops : %ld \n", host->learnDrops);
    }

  XFREE (MTYPE_HSL_SERVER_ENTRY, host);
  return nsm_stats_cli_return (cli, ret);
}



/* NSM show functions.  */
void
nsm_stats_show_init (struct cli_tree *ctree)
{
  /* "show ip route" */
  cli_install (ctree, EXEC_MODE, &show_statistics_vlan_cmd);
  cli_install (ctree, EXEC_MODE, &show_statistics_host_cmd);
  cli_install (ctree, EXEC_MODE, &clear_statistics_cmd);
}
#endif /* HAVE_USER_HSL */
