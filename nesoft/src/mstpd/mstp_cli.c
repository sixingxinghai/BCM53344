/* Copyright (C) 2003  All Rights Reserved. 
  
L2 BRIDGE CLI
  
This module defines the CLI interface to the Layer 2
bridging function.
  
The following commands are implemented:
(Config mode)
bridge <1-32> multiple-spanning-tree enable
no bridge <1-32> multiple-spanning-tree
bridge <1-32> rapid-spanning-tree enable
no bridge <1-32> rapid-spanning-tree
bridge <1-32> spanning-tree enable
no bridge <1-32> spanning-tree
bridge <1-32> priority <0-61440>
bridge <1-32> instance <1-63> priority <0-61440>
no bridge <1-32> priority
no bridge <1-32> instance <1-63> priority
bridge <1-32> forward-time <4-30>
no bridge <1-32> forward-time
bridge <1-32> hello-time <1-10>
no bridge <1-32> hello-time
bridge <1-32> max-age <6-40>
no bridge <1-32> max-age
bridge <1-32> max-hops <1-40>
no bridge <1-32> max-hops
bridge <1-32> spanning-tree portfast bpdu-filter
no bridge <1-32> spanning-tree portfast bpdu-filter
bridge <1-32> spanning-tree portfast bpdu-guard
no bridge <1-32> spanning-tree portfast bpduuard
bridge 1 spanning-tree errdisable-timeout enable
no bridge 1 spanning-tree errdisable-timeout enable
bridge 1 spanning-tree errdisable-timeout interval
no bridge 1 spanning-tree errdisable-timeout interval
bridge <1-32> spanning-tree pathcost method (short|long)
no bridge <1-32> spanning-tree pathcost method
show bridge <1-32> spanning-tree pathcost method


clear spanning-tree detected protocols
clear spanning-tree detected protocols interface INTERFACE

(EXEC PREV Mode)
show spanning-tree mst
show spanning-tree mst detail
show spanning-tree mst instance <1-63>
show spanning-tree instance-vlan
  
(Interface mode)
show spanning-tree
spanning-tree link-type point-to-point
no spanning-tree link-type
clear spanning-tree detected protocols bridge <1-32>
bridge-group <1-32> priority <0-240>
no bridge-group <1-32> priority
bridge-group <1-32> path-cost <1-200000000>
no bridge-group <1-32> path-cost
bridge-group <1-32> instance <1-63> priority <0-240>
no bridge-group <1-32> instance <1-63> priority 
bridge-group <1-32> instance <1-63> path-cost <1-200000000>
no bridge-group <1-32> instance <1-63> path-cost
spanning-tree portfast
no spanning-tree portfast
spanning-tree portfast bpdu-guard [enable|disable|default]
no spanning-tree portfast bpdu-guard
spanning-tree portfast bpdu-filter [enable|disable|default]
no spanning-tree portfast bpdu-filter
*/

#include "pal.h"
#include "cli.h"
#include "lib.h"
#include "if.h"
#include "log.h"

#include "l2_vlan_port_range_parser.h"
#include "l2_debug.h"
#include "l2_timer.h"
#include "l2lib.h"
#include "l2_llc.h"

#include "mstp_config.h"
#include "mstp_types.h"
#include "mstpd.h"
#include "mstp_api.h"
#include "mstp_bridge.h"
#include "mstp_port.h"
#include "mstp_cli.h"
#include "mstp_rlist.h"

#ifdef HAVE_RPVST_PLUS
#include "mstp_rpvst.h"
#endif /* HAVE_RPVST_PLUS */

#ifdef HAVE_WMI
#include "wmi_server.h"
#include "mstp_wmi.h"

#define WMI_XSTP_DEBUG_ON(a) (mstp_wmi_server->debug |= WMI_DEBUG_FLAG_ ## a)
#define WMI_XSTP_DEBUG_OFF(a) \
        (mstp_wmi_server->debug &= ~(WMI_DEBUG_FLAG_ ## a))
#define IS_DEBUG_WMI_XSTP(a) (mstp_wmi_server->debug & WMI_DEBUG_FLAG_ ## a)
#endif /* HAVE_WMI */

static u_int8_t *def_bridge_name = "0";

/* Bridge CLI definitions.  */

/* Common CLI help word.  */
#define BRIDGE_STR           "Bridge group commands"
#define BRIDGE_NAME_STR      "Bridge Group name for bridging"
#define BACKBONE_BRIDGE_NAME "Specifies the backbone bridge group name"
#define INSTANCE_STR         "Instance"
#define INSTANCE_ID_STR      "ID"
#define SPANNING_STR         "Spanning Tree Commands"
#define CUS_SPANNING_STR     "Customer Spanning Tree commands"
#define CE_PORT_STR          "Configure Customer Edge Port"
#define PE_PORT_STR          "Configure Provider Edge Port"
#define PE_SVLAN_STR         "SVLAN Of Provider Edge Port"
#define PE_SVLAN_ID_STR      "SVLAN ID Of Provider Edge Port"
#define INSTANCE_TE_MSTI     "MSTI to be the traffic engineering MSTI instance"

#define CIST(B)                                                               \
        ((IS_BRIDGE_MSTP (B)) ? " CIST" : "")

/* forward declarations */
u_int32_t mstp_nsm_get_port_path_cost (struct lib_globals *zg, const u_int32_t ifindex);

static void mstp_display_cist_info (struct mstp_bridge * br,
                                    struct interface *ifp, struct cli * cli, 
                                    int detail_flag);
static int  mstp_stats_display_info (struct mstp_bridge *br, 
                                     struct interface *ifp, struct cli *cli);
static int  mstp_stats_display_inst_info (struct mstp_bridge *br,
                                         struct interface *ifp,
                                         struct cli *cli, int instance);
static void mstp_stats_clear_info (struct mstp_bridge *br,
                                   struct interface *ifp, struct cli *cli);

static int mstp_stats_clear_mstp(struct mstp_bridge *br,
                                 struct interface *ifp, struct cli *cli,
                                 u_int32_t instance);

static s_int32_t
mstp_cist_get_topology_change_time (struct mstp_port *port);

#ifdef HAVE_MSTPD
static void mstp_display_msti_port (struct mstp_instance_port *inst_port, struct cli *cli);
static void mstp_display_instance_info ( struct mstp_bridge * br,
                                         struct interface *ifp, struct cli * cli);
static void mstp_display_instance_info_det ( struct mstp_bridge * br, 
                                             struct interface *ifp,
                                             struct cli * cli);
static void mstp_display_single_instance_info_det (
                   struct mstp_bridge_instance * br_inst,
                   struct interface *ifp,
                   struct cli * cli);
static void mstp_display_msti_vlan_list (struct cli *cli,
                                         struct rlist_info *vlan_list);

static  int
mstp_stats_display_single_inst_info (struct mstp_bridge *br,
                                     struct interface *ifp,
                                     struct cli *cli,
                                     u_int32_t instance);

static  int
mstp_stats_display_inst_info_det (struct mstp_bridge *br, 
                                  struct mstp_bridge_instance *br_inst,
                                  struct interface *ifp, struct cli *cli,
                                  u_int32_t instance);

static int
mstp_stats_clear_single_inst_info (struct mstp_bridge *br,
                                   struct interface *ifp,
                                   struct cli *cli,
                                   u_int32_t instance, int vid);
static s_int32_t
mstp_msti_get_topology_change_time (struct mstp_instance_port *port);

static void
mstp_display_msti_vlan_range_idx_list (struct cli *cli,
                                       struct rlist_info *vlan_list);
#endif /* HAVE_MSTPD */

static void 
mstp_display_stp_info (struct mstp_bridge * br,
                       struct interface *ifp, struct cli * cli,
                       int detail_flag); 

int
mstp_interface_new_hook (struct interface *ifp)
{
  ifp->info = mstp_interface_new (ifp);

  return 0;
}

int
mstp_interface_delete_hook (struct interface *ifp)
{
  /* Free memory */
  XFREE(MTYPE_XSTP_INTERFACE, ifp->info);

  /* Set pointer to NULL */
  ifp->info = NULL;

  return 0;

}


#ifdef HAVE_RPVST_PLUS
CLI (bridge_multiple_spanning_tree,
     bridge_multiple_spanning_tree_cmd,
     "bridge <1-32> "
     "(multiple-spanning-tree|rapid-pervlan-spanning-tree|rapid-spanning-tree|spanning-tree) enable",
     BRIDGE_STR,
     BRIDGE_NAME_STR,
     "multiple-spanning-tree - MSTP commands",
     "rapid-pervlan-spanning-tree - RPVST+ commands",
     "rapid-spanning-tree - RSTP commands",
     "spanning-tree - STP commands",
     "enable spanning tree protocol")
{
  int ret = RESULT_ERROR;
  int brno;
  u_int8_t *bridge_name;
  struct mstp_bridge *br;
  struct mstp_bridge_info *br_config_info = NULL;
  struct mstp_bridge_config_list *br_conf_list = NULL;
  u_int8_t br_type = BRIDGE_TYPE_XSTP;

  if (LAYER2_DEBUG(proto, CLI_ECHO))
    {
      if (argc >= 1)
        zlog_debug (mstpm, "bridge %s spanning-tree enable", argv[0]);
      else
        zlog_debug (mstpm, "spanning-tree enable");
    }

  if (argc > 1)
    {
      MSTP_GET_BR_NAME (2, 0);
    }
  else
    {
      MSTP_GET_BR_NAME (1, 0);
    }
  if (argc == 2)
    {
      if ((pal_strncmp (argv[1], "m", 1) == 0))
        br_type = NSM_BRIDGE_TYPE_MSTP;
      else if ((pal_strncmp (argv[1], "rapid-p", 7) == 0))
        br_type = NSM_BRIDGE_TYPE_RPVST_PLUS;
      else if ((pal_strncmp (argv[1], "rapid-s", 7) == 0))
        br_type = NSM_BRIDGE_TYPE_RSTP;
      else if ((pal_strncmp (argv[1], "s", 1) == 0))
        br_type = NSM_BRIDGE_TYPE_STP;
    }

  br = mstp_find_bridge (bridge_name);
  if (br == NULL)
    {
      br_config_info = mstp_bridge_config_info_get (bridge_name);
      if (!br_config_info)
        {
          br_conf_list = mstp_bridge_config_list_new (bridge_name);
          if (br_conf_list == NULL)
            {
              cli_out (cli, "%% Could not allocate memory for bridge config\n");
              return CLI_ERROR;
            }
          mstp_bridge_config_list_link (br_conf_list);
          br_config_info = &(br_conf_list->br_info);
        }

      br_config_info->mstp_enable_br_type = br_type;
      SET_FLAG (br_config_info->config, MSTP_BRIDGE_CONFIG_MSTP_ENABLE);
      return CLI_SUCCESS;
    }

  ret = mstp_api_enable_bridge (bridge_name, br_type);

  if (ret != RESULT_OK)
    {
      cli_out (cli, "%% Cannot enable spanning-tree for bridge : %s\n", 
               bridge_name);
      return CLI_ERROR;
    }

  return CLI_SUCCESS;
}

#else
CLI (bridge_multiple_spanning_tree,
     bridge_multiple_spanning_tree_cmd,
     "bridge <1-32> "
     "(multiple-spanning-tree|rapid-spanning-tree|spanning-tree) enable",
     BRIDGE_STR,
     BRIDGE_NAME_STR,
     "multiple-spanning-tree - MSTP commands",
     "rapid-spanning-tree - RSTP commands",
     "spanning-tree - STP commands",
     "enable spanning tree protocol")
{
  int ret = RESULT_ERROR;
  int brno;
  u_int8_t *bridge_name;
  struct mstp_bridge *br;
  struct mstp_bridge_info *br_config_info = NULL;
  struct mstp_bridge_config_list *br_conf_list = NULL;
  u_int8_t br_type = BRIDGE_TYPE_XSTP;

  if (LAYER2_DEBUG(proto, CLI_ECHO))
    {
      if (argc >= 1)
        zlog_debug (mstpm, "bridge %s spanning-tree enable", argv[0]);
      else
        zlog_debug (mstpm, "spanning-tree enable");
    }

  if (argc > 1)
    {
      MSTP_GET_BR_NAME (2, 0);
    }
  else
    {
      MSTP_GET_BR_NAME (1, 0);
    }

  if (argc == 2)
    {
      if ((pal_strncmp (argv[1], "m", 1) == 0))
        br_type = NSM_BRIDGE_TYPE_MSTP;
      else if ((pal_strncmp (argv[1], "rapid-s", 7) == 0))
        br_type = NSM_BRIDGE_TYPE_RSTP;
      else if ((pal_strncmp (argv[1], "s", 1) == 0))
        br_type = NSM_BRIDGE_TYPE_STP;
    }

  br = mstp_find_bridge (bridge_name);
  if (br == NULL)
    {
      br_config_info = mstp_bridge_config_info_get (bridge_name);
      if (!br_config_info)
        {
          br_conf_list = mstp_bridge_config_list_new (bridge_name);
          if (br_conf_list == NULL)
            {
              cli_out (cli, "%% Could not allocate memory for bridge config\n");
              return CLI_ERROR;
            }
          mstp_bridge_config_list_link (br_conf_list);
          br_config_info = &(br_conf_list->br_info);
        }

      br_config_info->mstp_enable_br_type = br_type;
      SET_FLAG (br_config_info->config, MSTP_BRIDGE_CONFIG_MSTP_ENABLE);
      return CLI_SUCCESS;
    }

  ret = mstp_api_enable_bridge (bridge_name, br_type);

  if (ret != RESULT_OK)
    {
      cli_out (cli, "%% Cannot enable spanning-tree for bridge : %s\n", 
               bridge_name);
      return CLI_ERROR;
    }

  return CLI_SUCCESS;
}
#endif /* HAVE_RPVST_PLUS */

ALI (bridge_multiple_spanning_tree,
     no_bridge_shutdown_multiple_spanning_tree_cmd,
     "no bridge shutdown <1-32>",
     CLI_NO_STR,
     BRIDGE_STR,
     "reset",
     BRIDGE_NAME_STR);

ALI (bridge_multiple_spanning_tree,
     no_spanning_shutdown_multiple_spanning_tree_cmd,
     "no spanning-tree shutdown",
     CLI_NO_STR,
     SPANNING_STR,
     "reset");

ALI (bridge_multiple_spanning_tree,
     spanning_multiple_spanning_tree_cmd,
     "spanning-tree enable",
     SPANNING_STR,
     "enable multiple spanning tree protocol");

#ifdef HAVE_RPVST_PLUS
CLI (no_bridge_multiple_spanning_tree,
     no_bridge_multiple_spanning_tree_cmd,
     "no bridge <1-32> "
     "(multiple-spanning-tree|rapid-pervlan-spanning-tree|rapid-spanning-tree|spanning-tree) enable "
     "(|bridge-forward)",
     CLI_NO_STR,
     BRIDGE_STR,
     BRIDGE_NAME_STR,
     "multiple-spanning-tree - MSTP commands",
     "rapid pervlan-spanning-tree - RPVST+ commands",
     "rapid-spanning-tree - RSTP commands",
     "spanning-tree - STP commands",
     "enable(disable) spanning tree protocol",
     "put all ports of the bridge into forwarding state")
{
  int ret = RESULT_ERROR;
  u_int8_t *bridge_name;
  int brno;
  struct mstp_bridge *br;
  bool_t bridge_forward = PAL_FALSE;
  u_int8_t br_type = BRIDGE_TYPE_XSTP;

  if (LAYER2_DEBUG(proto, CLI_ECHO))
  {
    if (argc >= 1)
      zlog_debug (mstpm, "no bridge %s spanning-tree enable", argv[0]);
    else
      zlog_debug (mstpm, "no spanning-tree enable");
  }
  
  if (argc >= 1)
    {
      CLI_GET_INTEGER_RANGE ("Max bridge groups", brno, argv[0], 1, 32);
      bridge_name = argv [0];
    }
  else
    bridge_name = def_bridge_name;

  if (argc >= 2)
    {
      if ((pal_strncmp (argv[1], "m", 1) == 0))
        br_type = NSM_BRIDGE_TYPE_MSTP;
      else if ((pal_strncmp (argv[1], "rapid-p", 7) == 0))
        br_type = NSM_BRIDGE_TYPE_RPVST_PLUS;
      else if ((pal_strncmp (argv[1], "rapid-s", 7) == 0))
        br_type = NSM_BRIDGE_TYPE_RSTP;
      else if ((pal_strncmp (argv[1], "s", 1) == 0))
        br_type = NSM_BRIDGE_TYPE_STP;
    }

  br = mstp_find_bridge (bridge_name);

  if ( br == NULL)
    {
      cli_out (cli, "%% Bridge %s does not exist\n", bridge_name);
      return CLI_ERROR;
    }
  
  if (argc == 3)
    if (pal_strncmp (argv[2], "b", 1) == 0)
      bridge_forward = PAL_TRUE;
   
  ret = mstp_api_disable_bridge (bridge_name, br_type, bridge_forward);
  if (ret != RESULT_OK)
    {
      cli_out (cli, "%% Cannot disable spanning-tree for bridge %s\n",
               argv[0]);
      return CLI_ERROR;
    }
  
  return CLI_SUCCESS;
}
#else
CLI (no_bridge_multiple_spanning_tree,
     no_bridge_multiple_spanning_tree_cmd,
     "no bridge <1-32> "
     "(multiple-spanning-tree|rapid-spanning-tree|spanning-tree) enable "
     "(|bridge-forward)",
     CLI_NO_STR,
     BRIDGE_STR,
     BRIDGE_NAME_STR,
     "multiple-spanning-tree - MSTP commands",
     "rapid-spanning-tree - RSTP commands",
     "spanning-tree - STP commands",
     "enable(disable) spanning tree protocol",
     "put all ports of the bridge into forwarding state")
{
  int ret = RESULT_ERROR;
  u_int8_t *bridge_name;
  int brno;
  struct mstp_bridge *br;
  bool_t bridge_forward = PAL_FALSE;
  u_int8_t br_type = BRIDGE_TYPE_XSTP;

  if (LAYER2_DEBUG(proto, CLI_ECHO))
    {
      if (argc >= 1)
        zlog_debug (mstpm, "no bridge %s spanning-tree enable", argv[0]);
      else
        zlog_debug (mstpm, "no spanning-tree enable");
    }
  
  if (argc >= 1)
    {
      CLI_GET_INTEGER_RANGE ("Max bridge groups", brno, argv[0], 1, 32);
      bridge_name = argv [0];
    }
  else
    bridge_name = def_bridge_name;

  if (argc >= 2)
    {
      if ((pal_strncmp (argv[1], "m", 1) == 0))
        br_type = NSM_BRIDGE_TYPE_MSTP;
      else if ((pal_strncmp (argv[1], "rapid-s", 7) == 0))
        br_type = NSM_BRIDGE_TYPE_RSTP;
      else if ((pal_strncmp (argv[1], "s", 1) == 0))
        br_type = NSM_BRIDGE_TYPE_STP;
    }

  br = mstp_find_bridge (bridge_name);

  if (br == NULL)
    {
      cli_out (cli, "%% Bridge %s does not exist\n", bridge_name);
      return CLI_ERROR;
    }
  
  if (argc == 3)
    if (pal_strncmp (argv[2], "b", 1) == 0)
      bridge_forward = PAL_TRUE;
   
  ret = mstp_api_disable_bridge (bridge_name, br_type, bridge_forward);
  if (ret != RESULT_OK)
    {
      cli_out (cli, "%% Cannot disable spanning-tree for bridge %s\n",
               argv[0]);
      return CLI_ERROR;
    }
  
  return CLI_SUCCESS;
}
#endif /* HAVE_RPVST_PLUS */

ALI (no_bridge_multiple_spanning_tree,
     bridge_shutdown_multiple_spanning_tree_cmd,
     "bridge shutdown <1-32>",
     BRIDGE_STR,
     "reset",
     BRIDGE_NAME_STR);

#ifdef HAVE_B_BEB
CLI (no_bridge_multiple_spanning_tree_for,
     bridge_shutdown_multiple_spanning_tree_for_cmd,
     "bridge shutdown (<1-32> | backbone) bridge-forward",
     BRIDGE_STR,
     "reset",
     BRIDGE_NAME_STR,
     "backbone bridge",
     "put all ports of the bridge into forwarding state")
{
  int brno;
  int ret = RESULT_ERROR;
  struct mstp_bridge *br;
  struct mstp_bridge_info *br_config_info = NULL;
  struct mstp_bridge_config_list *br_conf_list = NULL;

  if (LAYER2_DEBUG(proto, CLI_ECHO))
    zlog_debug (mstpm, "bridge shutdown %s bridge-forward", argv[0]);
  if (argv[0][0] != 'b')
    CLI_GET_INTEGER_RANGE ("Max bridge groups", brno, argv[0], 1, 32);

  br = mstp_find_bridge (argv[0]);

  if ( br == NULL)
    {
      if (!br_config_info)
        {
          br_conf_list = mstp_bridge_config_list_new (argv [0]);
          if (br_conf_list == NULL)
            {
              cli_out (cli, "%% Could not allocate memory for bridge config\n");
              return CLI_ERROR;
            }
          mstp_bridge_config_list_link (br_conf_list);
          br_config_info = &(br_conf_list->br_info);

          SET_FLAG (br_config_info->config, MSTP_BRIDGE_CONFIG_SHUT_FORWARD);
        }
      cli_out (cli, "%% Bridge %s does not exist\n", argv[0]);
      return CLI_ERROR;
    }
  /* There are no internal changes in the following API as mstpd only knows
     the bridge as MSTP or RSTP not Backbone MSTP or Backbone RSTP */
  ret = mstp_api_disable_bridge (argv[0], BRIDGE_TYPE_XSTP, PAL_TRUE);

  if (ret != RESULT_OK)
    {
      cli_out (cli, "%% Cannot disable spanning-tree for bridge %s\n",
               argv[0]);
      return CLI_ERROR;
    }

  return CLI_SUCCESS;
}

#else
CLI (no_bridge_multiple_spanning_tree_for,
     bridge_shutdown_multiple_spanning_tree_for_cmd,
     "bridge shutdown <1-32> bridge-forward",
     BRIDGE_STR,
     "reset",
     BRIDGE_NAME_STR,
     "put all ports of the bridge into forwarding state")
{
  int brno;
  int ret = RESULT_ERROR;
  struct mstp_bridge *br;

  if (LAYER2_DEBUG(proto, CLI_ECHO))
    zlog_debug (mstpm, "bridge shutdown %s bridge-forward", argv[0]);
  
  CLI_GET_INTEGER_RANGE ("Max bridge groups", brno, argv[0], 1, 32);

  br = mstp_find_bridge (argv [0]);

  if ( br == NULL)
    {
      cli_out (cli, "%% Bridge %s does not exist\n", argv[0]);
      return CLI_ERROR;
    }
  
  ret = mstp_api_disable_bridge (argv[0], BRIDGE_TYPE_XSTP, PAL_TRUE);

  if (ret != RESULT_OK)
    {
      cli_out (cli, "%% Cannot disable spanning-tree for bridge %s\n",
               argv[0]);
      return CLI_ERROR;
    }
  
  return CLI_SUCCESS;
}
#endif /* HAVE_B_BEB */

ALI (no_bridge_multiple_spanning_tree,
     spanning_shutdown_multiple_spanning_tree_cmd,
     "spanning-tree shutdown",
     SPANNING_STR,
     "reset");
 
CLI (mstp_bridge_priority,
     mstp_bridge_priority_cmd,
     "bridge <1-32> priority <0-61440>",
     BRIDGE_STR,
     BRIDGE_NAME_STR,
     "priority - bridge priority for the common instance",
     "bridge priority in increments of 4096 (Lower priority indicates greater likelihood of becoming root)")
{
  char *str = NULL;
  int ret;
  u_int32_t priority;
  int brno;
  u_int8_t *bridge_name;
  struct mstp_bridge *br = NULL;
  struct mstp_bridge_info *br_config_info = NULL;
  struct mstp_bridge_config_list *br_conf_list = NULL;

  if (LAYER2_DEBUG(proto, CLI_ECHO))
       {
      if (argc == 2)
        zlog_debug (mstpm, "bridge %s priority %s", argv[0], argv[1]);
      else
        zlog_debug (mstpm, "spanning-tree priority %s", argv[1]);
    }

  MSTP_GET_BR_NAME (2, 0);

  if (argc == 2)
    CLI_GET_UINT32_RANGE ("priority", priority, argv[1], 0, 61440);
  else
    CLI_GET_UINT32_RANGE ("priority", priority, argv[0], 0, 61440);
 
  br =  (struct mstp_bridge *)mstp_find_bridge (bridge_name);
  if (br == NULL)
    {
      br_config_info = mstp_bridge_config_info_get (bridge_name);
      if (!br_config_info)
        {
          br_conf_list = mstp_bridge_config_list_new (bridge_name);
          if (br_conf_list == NULL)
            {
              cli_out (cli, "%% Could not allocate memory for bridge config\n");
              return CLI_ERROR;
            }
          mstp_bridge_config_list_link (br_conf_list);
          br_config_info = &(br_conf_list->br_info);
        }

      br_config_info->priority = priority;
      SET_FLAG (br_config_info->config, MSTP_BRIDGE_CONFIG_PRIORITY);
      return CLI_SUCCESS;
    }

  ret = mstp_api_set_bridge_priority (bridge_name, priority);
  if (ret < 0)
    switch (ret)
      {
        case MSTP_ERR_BRIDGE_NOT_FOUND :
          str = "Bridge not found";
          break;
        case MSTP_ERR_PRIORITY_VALUE_WRONG :
          str = "The priority value must be in multiples of 4096";
          break;
        case MSTP_ERR_PRIORITY_OUTOFBOUNDS :
          str = "The priority value is out of bounds";
          break;
        default :
          str = "Can't set priority";
          break;
      }

  if (str)
    {
      cli_out (cli, "%% %s\n", str);
      return CLI_ERROR;
    }

  return CLI_SUCCESS;
}

ALI (mstp_bridge_priority,
     mstp_spanning_priority_cmd,
     "spanning-tree priority <0-61440>",
     SPANNING_STR,
     "priority - bridge priority for the common instance",
     "bridge priority in increments of 4096 (Lower priority indicates greater likelihood of becoming root)");

CLI (no_mstp_bridge_priority,
     no_mstp_bridge_priority_cmd,
     "no bridge <1-32> priority",
     CLI_NO_STR,
     BRIDGE_STR,
     BRIDGE_NAME_STR,
     "priority - Reset bridge priority to default value for the common instance")
{
  char *str = NULL;
  int ret;
  int brno;
  u_int8_t *bridge_name;

  if (LAYER2_DEBUG(proto, CLI_ECHO))
    {
      if (argc == 1)
        zlog_debug (mstpm, "no bridge %s priority", argv[0]);
      else
        zlog_debug (mstpm, "no spanning-tree priority");
    }

  MSTP_GET_BR_NAME (1, 0);

  ret = mstp_api_set_bridge_priority (bridge_name, BRIDGE_DEFAULT_PRIORITY);

  if (ret < 0)
    switch (ret)
      {
        case MSTP_ERR_BRIDGE_NOT_FOUND :
          str = "Bridge not found";
          break;
        case MSTP_ERR_PRIORITY_VALUE_WRONG :
          str = "The priority value must be in multiples of 4096";
          break;
        case MSTP_ERR_PRIORITY_OUTOFBOUNDS :
          str = "The priority value is out of bounds";
          break;
        default :
          str = "Can't set priority";
          break;
      }

  if (str)
    {
      cli_out (cli, "%% %s\n", str);
      return CLI_ERROR;
    }

  return CLI_SUCCESS;
}

ALI (no_mstp_bridge_priority,
     no_mstp_spanning_priority_cmd,
     "no spanning-tree priority",
     CLI_NO_STR,
     SPANNING_STR,
     "priority - Reset bridge priority to default value for the common instance");

CLI  (mstp_spanning_tree_pathcost_method,
      mstp_spanning_tree_pathcost_method_cmd,
      "bridge <1-32> spanning-tree pathcost method (short|long)",
      BRIDGE_STR,
      BRIDGE_NAME_STR,
      "spanning-tree",
      "Spanning tree pathcost options",
      "Method to calculate default port path cost",
      "short - Use 16 bit based values for default port path costs",
      "long  - Use 32 bit based values for default port path costs")
{
  int ret;
  u_int8_t *bridge_name;
  u_int8_t path_cost_method;
  int brno;
  struct mstp_bridge_info *br_config_info = NULL;
  struct mstp_bridge_config_list *br_conf_list = NULL;
  struct mstp_bridge *br = NULL;

  MSTP_GET_BR_NAME (3, 0);

  if (pal_strncmp(argv[argc - 1], "s", 1) == 0)
    path_cost_method = MSTP_PATHCOST_SHORT;
  else if (pal_strncmp(argv[argc - 1], "l", 1) == 0)
    path_cost_method = MSTP_PATHCOST_LONG;
  else
    {
      cli_out (cli, "%% Invalid input detected.\n");
      return CLI_ERROR;
    }

  br = mstp_find_bridge (bridge_name);
  if (br == NULL)
    {
      br_config_info = mstp_bridge_config_info_get (bridge_name);

      if (!br_config_info)
        {
          br_conf_list = mstp_bridge_config_list_new (bridge_name);

          if (br_conf_list == NULL)
            {
              cli_out (cli, "%% Could not allocate memory for bridge config\n");
              return CLI_ERROR;
            }

          mstp_bridge_config_list_link (br_conf_list);
          br_config_info = &(br_conf_list->br_info);
        }

      br_config_info->path_cost_method = path_cost_method;
      SET_FLAG (br_config_info->config, MSTP_BRIDGE_CONFIG_PATH_COST_METHOD);

      return CLI_SUCCESS;
    }

  ret = mstp_api_set_pathcost_method (bridge_name, path_cost_method);

  if (ret < 0)
    {
      cli_out (cli, "%% Can't update the pathcost method\n");
      return CLI_ERROR;
    }
  else if (ret == MSTP_ERR)
    {
      cli_out (cli, "%% Configured Method already exist\n");
    }

  return CLI_SUCCESS;
}

CLI  (no_mstp_spanning_tree_pathcost_method,
      no_mstp_spanning_tree_pathcost_method_cmd,
      "no bridge <1-32> spanning-tree pathcost method",
      BRIDGE_STR,
      BRIDGE_NAME_STR,
      "Spanning Tree Subsystem",
      "Spanning tree pathcost options",
      "Method to calculate default port path cost")
{
  int ret;
  int brno;
  struct mstp_bridge *br = NULL;
  u_int8_t *bridge_name;

  MSTP_GET_BR_NAME (2, 0);

  br = mstp_find_bridge (bridge_name);
  if (! br)
    {
      cli_out (cli, "%% Bridge %s does not exist\n", bridge_name);
    }

  ret = mstp_api_set_pathcost_method (bridge_name, MSTP_PATHCOST_DEFAULT);
  if (ret < 0)
    {
      cli_out (cli, "%% Can't update the pathcost method\n");
      return CLI_ERROR;
    }

  return CLI_SUCCESS;
}
 
CLI (show_spanning_tree_pathcost_method,
     show_spanning_tree_pathcost_method_cmd,
     "show bridge <1-32> spanning-tree pathcost method",
     CLI_SHOW_STR,
     BRIDGE_STR,
     BRIDGE_NAME_STR,
     "Spanning tree topology",
     "Show Spanning pathcost options",
     "Default pathcost calculation method")
{
  int ret;
  int brno;
  u_int8_t *bridge_name;
  struct mstp_bridge *br = NULL;

  MSTP_GET_BR_NAME (1, 0);
  br = mstp_find_bridge (bridge_name);

  if (!br)
    {
      cli_out (cli, "%% Bridge %s does not exist\n", bridge_name);
      return CLI_ERROR;
    }

  ret = mstp_api_get_pathcost_method (bridge_name);

  if (ret == MSTP_PATHCOST_SHORT)
    cli_out (cli, "%% Spanning tree default pathcost method used is short\n");
  else if (ret == MSTP_PATHCOST_LONG)
    cli_out (cli, "%% Spanning tree default pathcost method used is long\n");
  else
    {
      cli_out (cli, "%% Path cost method not configured\n");
      return CLI_ERROR;
    }

  return CLI_SUCCESS;
}

CLI (mstp_spanning_tree_restricted_tcn,
     mstp_spanning_tree_restricted_tcn_cmd,
     "spanning-tree restricted-tcn",
     "spanning tree commands",
     "restrict propagation of topology change and received topology change notifications")
{
  int ret;
  struct interface *ifp = cli->index;
  struct mstp_interface *mstpif = NULL;

  if (LAYER2_DEBUG(proto, CLI_ECHO))
    zlog_debug (mstpm, "spanning-tree port restricted tcn");

  if ((ifp == NULL) || (ifp->port == NULL))
    {
      if (ifp)
        cli_out (cli, "%% Can't configure on interface %s\n", ifp->name);
      return CLI_ERROR;
    }

  mstpif = ifp->info;
  if (mstpif == NULL)
    return CLI_ERROR;

  if (if_lookup_by_name (&cli->vr->ifm, ifp->name) == NULL)
    {
      mstpif->restricted_tcn = PAL_TRUE;
      SET_FLAG (mstpif->config, MSTP_IF_CONFIG_RESTRICTED_TCN);
      return CLI_ERROR;
    }

  ret = mstp_api_set_port_restricted_tcn (ifp->bridge_name, ifp->name, PAL_TRUE);
  if (ret < 0)
    {
      cli_out (cli, "%% Can't configure port restricted tcn \n");
      return CLI_ERROR;
    }

  return CLI_SUCCESS;

}

CLI (no_mstp_spanning_tree_restricted_tcn,
     no_mstp_spanning_tree_restricted_tcn_cmd,
     "no spanning-tree restricted-tcn",
     CLI_NO_STR,
     "spanning tree commands",
     "remove restriction on propagation of topology change notifications")
{
  int ret;
  struct interface *ifp = cli->index;

  if (LAYER2_DEBUG(proto, CLI_ECHO))
    zlog_debug (mstpm, "no spanning-tree port restricted tcn");

  ret = mstp_api_set_port_restricted_tcn (ifp->bridge_name, ifp->name, PAL_FALSE);
  if (ret < 0)
    {
      cli_out (cli, "%% Can't configure port restricted tcn \n");
      return CLI_ERROR;
    }

  return CLI_SUCCESS;

}

CLI (mstp_spanning_tree_restricted_role,
     mstp_spanning_tree_restricted_role_cmd,
     "spanning-tree restricted-role",
     "spanning tree commands",
     "restrict the role of the port")
{
  int ret;
  struct interface *ifp = cli->index;
  struct mstp_interface *mstpif = NULL;

  if (LAYER2_DEBUG(proto, CLI_ECHO))
    zlog_debug (mstpm, "spanning-tree port restricted role");

  if ((ifp == NULL) || (ifp->port == NULL))
    {
      if (ifp)
        cli_out (cli, "%% Can't configure on interface %s\n", ifp->name);
      return CLI_ERROR;
    }

  mstpif = ifp->info;
  if (mstpif == NULL)
    return CLI_ERROR;

  if (if_lookup_by_name (&cli->vr->ifm, ifp->name) == NULL)
    {
      mstpif->restricted_role = PAL_TRUE;
      SET_FLAG (mstpif->config, MSTP_IF_CONFIG_RESTRICTED_ROLE);
      return CLI_ERROR;
    }

  ret = mstp_api_set_port_restricted_role (ifp->bridge_name, ifp->name, PAL_TRUE);
  if (ret < 0)
    {
      cli_out (cli, "%% Can't configure port restricted role \n");
      return CLI_ERROR;
    }

  return CLI_SUCCESS;

}

CLI (no_mstp_spanning_tree_restricted_role,
     no_mstp_spanning_tree_restricted_role_cmd,
     "no spanning-tree restricted-role",
     CLI_NO_STR,
     "spanning tree commands",
     "remove restriction on the role of the port")
{
  int ret;
  struct interface *ifp = cli->index;

  if (LAYER2_DEBUG(proto, CLI_ECHO))
    zlog_debug (mstpm, "no spanning-tree port restricted role");

  ret = mstp_api_set_port_restricted_role (ifp->bridge_name, ifp->name, PAL_FALSE);
  if (ret < 0)
    {
      cli_out (cli, "%% Can't configure port restricted role \n");
      return CLI_ERROR;
    }

  return CLI_SUCCESS;

}

#ifdef HAVE_MSTPD

#if defined (HAVE_PROVIDER_BRIDGE) || defined (HAVE_B_BEB)
CLI (spanning_tree_te_msti_configuration,
     spanning_tree_te_msti_configuration_cmd,
     "spanning-tree te-msti configuration",
     "spanning tree commands",
     "te-msti configuration mode",
     "Configuration")
{
   if (LAYER2_DEBUG(proto, CLI_ECHO))
    zlog_debug (mstpm, "spanning-tree te-msti configuration");

   cli->mode = TE_MSTI_CONFIG_MODE;
   return CLI_SUCCESS;
}

CLI (spanning_tree_bridge_te_msti,
     spanning_tree_bridge_te_msti_cmd,
     "bridge (<1-32> | backbone) te-msti",
     BRIDGE_STR,
     BRIDGE_NAME_STR,
     BACKBONE_BRIDGE_NAME,
     INSTANCE_TE_MSTI)
{
   struct mstp_bridge *mstp_br =NULL;
   struct mstp_bridge_instance *mstp_br_inst = NULL;
   char   br_name[L2_BRIDGE_NAME_LEN+1];
   int    ret;

   pal_strcpy (br_name, argv[0]);

   mstp_br = (struct mstp_bridge *) mstp_find_bridge (br_name);
   if (! mstp_br)
     {
       cli_out (cli, " Bridge %s is not configured as MSTP\n", br_name);
       return CLI_ERROR;
     }

   mstp_br_inst = mstp_br->instance_list[MSTP_TE_MSTID];  
   if (! mstp_br_inst)
     {
       cli_out (cli, "% Bridge %s has no MSTP bridge instance\n", br_name);
       return CLI_ERROR;
     }

   ret = mstp_api_enable_bridge_msti (mstp_br_inst);
   if (ret == RESULT_ERROR)
     {
        cli_out (cli, "% Can't enable MSTI on bridge %s \n", br_name);
        return CLI_ERROR;
     }

   return CLI_SUCCESS;
}
 
CLI (no_spanning_tree_bridge_te_msti,
     no_spanning_tree_bridge_te_msti_cmd,
     "no bridge (<1-32> | backbone) te-msti",
     CLI_NO_STR,
     BRIDGE_STR,
     BRIDGE_NAME_STR,
     BACKBONE_BRIDGE_NAME,
     INSTANCE_TE_MSTI)
{
   struct mstp_bridge *mstp_br =NULL;
   struct mstp_bridge_instance *mstp_br_inst = NULL;
   char   br_name[L2_BRIDGE_NAME_LEN+1];
   int    ret;

   pal_strcpy (br_name, argv[0]);

   mstp_br = (struct mstp_bridge *) mstp_find_bridge (br_name);
   if (! mstp_br)
     {
        cli_out (cli, "Bridge %s is not configured as MSTP\n", br_name);
        return CLI_ERROR;
     }

   mstp_br_inst = mstp_br->instance_list[MSTP_TE_MSTID];
   if (! mstp_br_inst)
     {
        cli_out (cli, "% Bridge %s has no MSTP bridge instance\n", br_name);
        return CLI_ERROR;
     }

   ret = mstp_api_disable_bridge_msti (mstp_br_inst);
   if (ret == RESULT_ERROR)
     {
        cli_out (cli, "% Can't disabled MSTI on bridge %s\n", br_name);
        return CLI_ERROR;
     }

   return CLI_SUCCESS;
}

CLI (bridge_te_msti_vlan,
     bridge_te_msti_vlan_cmd,
     "bridge (<1-32> | backbone) te-msti vlan <1-4094>",
     BRIDGE_STR,
     BRIDGE_NAME_STR,
     BACKBONE_BRIDGE_NAME,
     INSTANCE_TE_MSTI,
     "vlanid (1-4094)",
     "vlan id to be associated")
{
   int ret;
   char *curr;
   u_int16_t vid = 0;
   s_int32_t instance;
   struct mstp_vlan *v;
   char bridge_name[L2_BRIDGE_NAME_LEN+1];
   l2_range_parser_t par;
   struct mstp_bridge *br;
   u_int8_t *vlan_arg = NULL;
   l2_range_parse_error_t r_err;

   if (LAYER2_DEBUG(proto, CLI_ECHO))
     {
        if (argc == 2)
          zlog_debug (mstpm, "bridge %s  vlan %s",
                      argv[0], argv[1]);
        else if (argc == 1)
          zlog_debug (mstpm, "instance %s vlan %s",
                      argv[0], argv[1]);;
     }

   pal_strcpy (bridge_name, argv[0]);
   curr = NULL;

   if (argc == 2)
     {
        instance = MSTP_TE_MSTID;
        vlan_arg = argv [1];
        CLI_GET_INTEGER_RANGE ("vlan", vid, argv[1], 1, 4094);
     }
   else
     {
        instance = MSTP_TE_MSTID;
        vlan_arg = argv [0];
        CLI_GET_INTEGER_RANGE ("vlan", vid, argv[0], 1, 4094);
     }

   br = mstp_find_bridge (bridge_name);

   if (! br)
     {
        cli_out (cli, "%% Bridge %s is not configured as MSTP\n", bridge_name);
        return CLI_ERROR;
     }
    
   /* Store the configuration if the vlan is not found */

   l2_range_parser_init (vlan_arg, &par);

   if ((ret = l2_range_parser_validate (&par, cli,
                                        mstp_bridge_vlan_validate_vid))
        != CLI_SUCCESS)
     return ret;

   while ((curr = l2_range_parser_get_next (&par, &r_err)))
     {
        if (r_err != RANGE_PARSER_SUCCESS)
          {
             cli_out (cli, "%% Invalid Input %s \n",
                      l2_range_parse_err_to_str (r_err) );
             return CLI_ERROR;
          }

        CLI_GET_INTEGER_RANGE ("vlan", vid, curr, 1, 4094);

        v = mstp_bridge_vlan_lookup (bridge_name, vid);

        if (v == NULL)
          {
             ret = mstp_bridge_vlan_add2 (bridge_name, vid, instance,
                                          MSTP_VLAN_INSTANCE_CONFIGURED);
             if (ret < 0)
               mstp_bridge_instance_config_add (bridge_name, instance, vid);

             continue;
          }

        ret = mstp_api_add_instance (bridge_name,instance, vid, 0); 
        if (ret != RESULT_OK)
          {
             if (ret == MSTP_INSTANCE_IN_USE_ERR)
               {
                 cli_out (cli, "%% Instance is in use\n");
                 return CLI_ERROR;
               }   

             mstp_bridge_instance_config_add (bridge_name, instance, vid);

             cli_out (cli, "Cannot create instance \n");
             return CLI_ERROR;
          }
     }

   l2_range_parser_deinit (&par);
   return CLI_SUCCESS;
}

CLI (no_bridge_te_msti_vlan,
     no_bridge_te_msti_vlan_cmd,
     "no bridge (<1-32> | backbone) te-msti vlan <1-4094>",
     CLI_NO_STR,
     BRIDGE_STR,
     BRIDGE_NAME_STR,
     BACKBONE_BRIDGE_NAME,
     INSTANCE_TE_MSTI,
     "vlanid (1-4094)",
     "vlan id to be associated")
{
   int ret;
   char *curr;
   int instance;
   u_int16_t vid;
   struct mstp_vlan *v;
   char bridge_name[L2_BRIDGE_NAME_LEN+1];
   l2_range_parser_t par;
   u_int8_t *vlan_arg = NULL;
   l2_range_parse_error_t r_err;


   if (LAYER2_DEBUG(proto, CLI_ECHO))
     {
        if (argc == 2)
          zlog_debug (mstpm, "no bridge %s vlan %s",
                      argv[0], argv[1]);
        else
          zlog_debug (mstpm, "no instance %s vlan %s",
                      argv[0], argv[1]);
     }

   pal_strcpy (bridge_name, argv[0]);
   curr = NULL;

   if (argc == 2)
     {
        instance = MSTP_TE_MSTID;
        vlan_arg = argv [1];
        CLI_GET_INTEGER_RANGE ("vlan", vid, argv[1], 1, 4094);
     }
   else
     {
        instance = MSTP_TE_MSTID;
        vlan_arg = argv [0];
        CLI_GET_INTEGER_RANGE ("vlan", vid, argv[0], 1, 4094);
     }

   l2_range_parser_init (vlan_arg, &par);

   if ((ret = l2_range_parser_validate (&par, cli,
                                        mstp_bridge_vlan_validate_vid))
       != CLI_SUCCESS)
     return ret;

   while ((curr = l2_range_parser_get_next (&par, &r_err)))
     {
        if (r_err != RANGE_PARSER_SUCCESS)
          {
            cli_out (cli, "%% Invalid Input %s \n",
                     l2_range_parse_err_to_str (r_err) );
            return CLI_ERROR;
          }

        CLI_GET_INTEGER_RANGE ("vlan", vid, curr, 1, 4094);

        v = mstp_bridge_vlan_lookup (bridge_name, vid);

        if (! v)
          {
             cli_out (cli, "VLAN not found\n");
             return CLI_ERROR;
          }

#ifdef HAVE_PVLAN
        if (v->pvlan_type == MSTP_PVLAN_SECONDARY)
          {
             cli_out (cli, "cannot delete instance \n");
             return CLI_ERROR;
          }
#endif /* HAVE_PVLAN */

        if (CHECK_FLAG (v->flags, MSTP_VLAN_INSTANCE_CONFIGURED))
          {
             /* Instance not alive. Delete VLAN. */
             mstp_bridge_vlan_delete (bridge_name, vid);

             mstp_bridge_instance_config_delete (bridge_name, instance,
                                                 vid);
 
             return CLI_SUCCESS;
          }

        if (mstp_api_delete_instance_vlan (bridge_name, instance, vid) != RESULT_OK)
          {
             cli_out (cli, "Cannot delete instance\n");
             return CLI_ERROR;
          }
     }

   l2_range_parser_deinit (&par); 
   return CLI_SUCCESS;
}
#endif /*(HAVE_PROVIDER_BRIDGE) || (HAVE_B_BEB)*/

CLI (mstp_port_hello_time,
     mstp_port_hello_time_cmd,
     "spanning-tree hello-time <1-10>",
     "spanning tree commands",
     "hello-time - hello BDPU interval",
     "seconds <1-10> - Hello BPDU interval")
{
  int ret;
  struct interface *ifp = cli->index;
  struct mstp_interface *mstpif = NULL;
  s_int32_t hello_time;

  CLI_GET_INTEGER_RANGE ("hello-time", hello_time, argv[0], 1, 10);

  if (LAYER2_DEBUG(proto, CLI_ECHO))
    zlog_debug (mstpm, "spanning-tree port hello-time");

  if ((ifp == NULL) || (ifp->port == NULL))
    {
      if (ifp)
        cli_out (cli, "%% Can't configure on interface %s\n", ifp->name);
      return CLI_ERROR;
    }

  mstpif = ifp->info;

  if (if_lookup_by_name (&cli->vr->ifm, ifp->name) == NULL)
    {
      mstpif->hello_time = hello_time;
      SET_FLAG (mstpif->config, MSTP_IF_CONFIG_PORT_HELLO_TIME);
      return CLI_ERROR;
    }

  ret = mstp_api_set_port_hello_time (ifp->bridge_name, ifp->name, hello_time);

  if (ret < 0)
    {
      cli_out (cli, "%% Can't configure port hello time \n");
      return CLI_ERROR;
    }

  return CLI_SUCCESS;
}

CLI (no_mstp_port_hello_time,
     no_mstp_port_hello_time_cmd,
     "no spanning-tree hello-time",
     CLI_NO_STR,
     "spanning tree commands",
     "hello-time - hello BDPU interval")
{
  int ret;
  struct interface *ifp = cli->index;
                                                                                                                                                             
  if (LAYER2_DEBUG(proto, CLI_ECHO))
    zlog_debug (mstpm, "no spanning-tree hello-time");

  ret = mstp_api_set_port_hello_time (ifp->bridge_name, ifp->name,
                                      BRIDGE_TIMER_DEFAULT_HELLO_TIME);
  if (ret < 0)
    {
      cli_out (cli, "%% Can't configure port hello time \n");
      return CLI_ERROR;
    }
                                                                                                                                                             
  return CLI_SUCCESS;
                                                                                                                                                             
}

CLI (mstp_spanning_tree_inst_restricted_tcn,
     mstp_spanning_tree_inst_restricted_tcn_cmd,
     "spanning-tree instance <1-63> restricted-tcn",
     "spanning tree commands",
     "Set restrictions for the port of particular instance",
     "instance id",
     "restrict propagation of topology change notifications from port")
{
  int ret;
  struct mstp_bridge *br;
  struct interface *ifp = cli->index;
  struct mstp_interface *mstpif = ifp->info;
  struct port_instance_info *pinst_info = NULL;
  int instance;

  CLI_GET_INTEGER_RANGE ("instance", instance, argv[0], 1, 63);

  br = mstp_find_bridge (ifp->bridge_name);

  if ((br != NULL) && (! IS_BRIDGE_MSTP (br)))
    {
      cli_out (cli, "%% Bridge %s is not configured as MSTP\n", ifp->bridge_name);
      return CLI_ERROR;
    }

  if (if_lookup_by_name (&cli->vr->ifm, ifp->name) == NULL)
    {
      pinst_info = mstpif_get_instance_info (mstpif, instance);
      pinst_info->restricted_tcn = PAL_TRUE;
      SET_FLAG (pinst_info->config, MSTP_IF_CONFIG_INSTANCE_RESTRICTED_TCN);
      return CLI_ERROR;
    }

  ret = mstp_api_set_msti_instance_restricted_tcn (ifp->bridge_name, ifp->name,
                                         instance, PAL_TRUE);

  if (ret < 0)
    {
      cli_out (cli, "%% Can't configure instance restricted-tcn\n");
      return CLI_ERROR;
    }

  return CLI_SUCCESS;
}

CLI (no_mstp_spanning_tree_inst_restricted_tcn,
     no_mstp_spanning_tree_inst_restricted_tcn_cmd,
     "no spanning-tree instance <1-63> restricted_tcn",
     CLI_NO_STR,
     "spanning tree commands",
     "remove restrictions for the port of particular instance",
     "instance id",
     "remove restriction on propagation of topology change notifications from port")
{
  int ret;
  struct mstp_bridge *br;
  struct interface *ifp = cli->index;
  int instance;

  CLI_GET_INTEGER_RANGE ("instance", instance, argv[0], 1, 63);

  br = mstp_find_bridge (ifp->bridge_name);
  if ((br != NULL) && (! IS_BRIDGE_MSTP (br)))
    {
      cli_out (cli, "%% Bridge %s is not configured as MSTP\n", ifp->bridge_name);
      return CLI_ERROR;
    }

  ret = mstp_api_set_msti_instance_restricted_tcn (ifp->bridge_name, ifp->name,
                                                   instance, PAL_FALSE);

  if (ret < 0)
    {
      cli_out (cli, "%% Can't configure instance restricted-tcn\n");
      return CLI_ERROR;
    }

  return CLI_SUCCESS;
}

CLI (mstp_spanning_tree_inst_restricted_role,
     mstp_spanning_tree_inst_restricted_role_cmd,
     "spanning-tree instance <1-63> restricted-role",
     "spanning tree commands",
     "Set restrictions for the port of particular instance",
     "instance id",
     "restrict the role of the port")
{
  int ret;
  struct mstp_bridge *br;
  struct interface *ifp = cli->index;
  struct mstp_interface *mstpif = ifp->info;
  struct port_instance_info *pinst_info = NULL;
  int instance;

  CLI_GET_INTEGER_RANGE ("instance", instance, argv[0], 1, 63);

  br = mstp_find_bridge (ifp->bridge_name);

  if ((br != NULL) && (! IS_BRIDGE_MSTP (br)))
    {
      cli_out (cli, "%% Bridge %s is not configured as MSTP\n", ifp->bridge_name);
      return CLI_ERROR;
    }

  if (if_lookup_by_name (&cli->vr->ifm, ifp->name) == NULL)
    {
      pinst_info = mstpif_get_instance_info (mstpif, instance);
      pinst_info->restricted_role = PAL_TRUE;
      SET_FLAG (pinst_info->config, MSTP_IF_CONFIG_INSTANCE_RESTRICTED_ROLE);
      return CLI_ERROR;
    }

  ret = mstp_api_set_msti_instance_restricted_role (ifp->bridge_name, ifp->name,
                                         instance, PAL_TRUE);

  if (ret < 0)
    {
      cli_out (cli, "%% Can't configure instance restricted-role\n");
      return CLI_ERROR;
    }

  return CLI_SUCCESS;
}

CLI (no_mstp_spanning_tree_inst_restricted_role,
     no_mstp_spanning_tree_inst_restricted_role_cmd,
     "no spanning-tree instance <1-63> restricted-role",
     CLI_NO_STR,
     "spanning tree commands",
     "remove restrictions for the port of particular instance",
     "instance id",
     "remove restriction on the role of the port")
{
  int ret;
  struct mstp_bridge *br;
  struct interface *ifp = cli->index;
  int instance;

  CLI_GET_INTEGER_RANGE ("instance", instance, argv[0], 1, 63);

  br = mstp_find_bridge (ifp->bridge_name);

  if ((br != NULL) && (! IS_BRIDGE_MSTP (br)))
    {
      cli_out (cli, "%% Bridge %s is not configured as MSTP\n", ifp->bridge_name);
      return CLI_ERROR;
    }

  ret = mstp_api_set_msti_instance_restricted_role (ifp->bridge_name, ifp->name,
                                                    instance, PAL_FALSE);

  if (ret < 0)
    {
      cli_out (cli, "%% Can't configure instance restricted-role\n");
      return CLI_ERROR;
    }

  return CLI_SUCCESS;
}

CLI (bridge_inst_priority,
     bridge_inst_priority_cmd,
     "bridge (<1-32>|backbone) instance <1-63> priority <0-61440>",
     BRIDGE_STR,
     BRIDGE_NAME_STR,
     BACKBONE_BRIDGE_NAME,       
     "Change priority for a particular instance",
     "instance id",
     "priority - bridge priority for the common instance",
     "bridge priority in increments of 4096 (Lower priority indicates greater likelihood of becoming root)")
{
  char *str = NULL;
  int ret;
  u_int32_t priority;
  int instance;
  int brno;
  u_int8_t *bridge_name;
  struct mstp_bridge *br = NULL;
  struct mstp_bridge_info *br_config_info = NULL;
  struct mstp_bridge_config_list *br_conf_list = NULL;
  struct mstp_bridge_instance_info *br_inst_info = NULL; 
  struct mstp_bridge_instance_list *br_inst_list = NULL;

  if (LAYER2_DEBUG(proto, CLI_ECHO))
    {
      if (argc == 3)
        zlog_debug (mstpm, "bridge %s instance %s priority %s",
                    argv[0], argv[1],argv[2]);
      else
        zlog_debug (mstpm, "spanning-tree instance %s priority %s",
                    argv[0], argv[1]);
    }

  MSTP_GET_BR_NAME (3, 0);

  if (argc == 3)
    {
      CLI_GET_UINT32_RANGE ("priority", priority, argv[2], 0, 61440);
      CLI_GET_INTEGER_RANGE ("instance", instance, argv[1], 1, 63);
    }
  else
    {
      CLI_GET_UINT32_RANGE ("priority", priority, argv[1], 0, 61440);
      CLI_GET_INTEGER_RANGE ("instance", instance, argv[0], 1, 63);
    }
 
  br = (struct mstp_bridge *)mstp_find_bridge (bridge_name);

  if ((br != NULL) && (! IS_BRIDGE_MSTP (br)))
    {
      cli_out (cli, "%% Bridge %s is not configured as MSTP\n", bridge_name);
      return CLI_ERROR;
    }

  if ((br == NULL) || (br->instance_list[instance] == NULL))
    {
      br_config_info = mstp_bridge_config_info_get (bridge_name);
      if (!br_config_info)
        {
          br_conf_list = mstp_bridge_config_list_new (bridge_name);
          if (br_conf_list == NULL)
            {
              cli_out (cli, "%% Could not allocate memory for bridge config\n");
              return CLI_ERROR;
            }
          mstp_bridge_config_list_link (br_conf_list);
          br_config_info = &(br_conf_list->br_info);
        }

      br_inst_info = mstp_br_config_find_instance (br_config_info, instance);
      if (!br_inst_info)
        {
          br_inst_list = mstp_br_config_instance_new (instance);
          if (br_inst_list == NULL)
            {
              cli_out (cli, "%% Could not allocate memory for bridge instance "
                            "configuration\n");
              return CLI_ERROR;
            }
          mstp_br_config_link_instance_new (br_config_info, br_inst_list);
          br_inst_info = & (br_inst_list->instance_info);
        }
      mstp_bridge_instance_config_add (bridge_name , instance, 0);
      br_inst_info->priority = priority;
      SET_FLAG (br_inst_info->config, MSTP_BRIDGE_CONFIG_INSTANCE_PRIORITY);
      return CLI_SUCCESS;
    }

  ret = mstp_api_set_msti_bridge_priority (bridge_name, instance, priority);

  if (ret < 0)
    switch (ret)
      {
        case MSTP_ERR_BRIDGE_NOT_FOUND :
          str = "Bridge not found";
          break;
        case MSTP_ERR_PRIORITY_VALUE_WRONG :
          str = "The priority value must be in multiples of 4096";
          break;
        case MSTP_ERR_PRIORITY_OUTOFBOUNDS :
          str = "The priority value is out of bounds";
          break;
        case MSTP_ERR_INSTANCE_OUTOFBOUNDS :
          str = "The instance value is out of bounds";
          break;
        case MSTP_ERR_INSTANCE_NOT_FOUND :
          str = "Bridge-instance not found";
          break;
        case MSTP_ERR_NOT_MSTP_BRIDGE :
          str = "Bridge is not configured as MSTP";
          break;
        default :
          str = "Can't set priority";
          break;
      }

  if (str)
    {
      cli_out (cli, "%% %s\n", str);
      return CLI_ERROR;
    }
 
  return CLI_SUCCESS;
}

ALI (bridge_inst_priority,
     spanning_inst_priority_cmd,
     "spanning-tree instance <1-63> priority <0-61440>",
     SPANNING_STR,
     "Change priority for a particular instance",
     "instance id",
     "priority - bridge priority for the common instance",
     "bridge priority in increments of 4096 (Lower priority indicates greater likelihood of becoming root)");

CLI (no_bridge_inst_priority,
     no_bridge_inst_priority_cmd,
     "no bridge (<1-32> | backbone) instance <1-63> priority",
     CLI_NO_STR,
     BRIDGE_STR,
     BRIDGE_NAME_STR,
     BACKBONE_BRIDGE_NAME,       
     "Change priority for a particular instance",
     "instance id",
     "priority - Reset bridge priority to default for the instance")
{
  char *str = NULL;
  int ret;
  int instance;
  int brno;
  u_int8_t *bridge_name;

  if (LAYER2_DEBUG(proto, CLI_ECHO))
    {
      if (argc == 2)
        zlog_debug (mstpm, "no bridge %s instance %s priority",
                    argv[0], argv[1]);
      else
        zlog_debug (mstpm, "no spanning-tree instance %s priority",
                    argv[0]);
    }

  MSTP_GET_BR_NAME (2, 0);

  if (argc == 2)
    CLI_GET_INTEGER_RANGE ("instance", instance, argv[1], 1, 63);
  else
    CLI_GET_INTEGER_RANGE ("instance", instance, argv[0], 1, 63);

  ret = mstp_api_set_msti_bridge_priority (bridge_name, instance, 
                                           BRIDGE_DEFAULT_PRIORITY);
  if (ret < 0)
    switch (ret)
      {
        case MSTP_ERR_BRIDGE_NOT_FOUND :
          str = "Bridge not found";
          break;
        case MSTP_ERR_PRIORITY_VALUE_WRONG :
          str = "The priority value must be in multiples of 4096";
          break;
        case MSTP_ERR_PRIORITY_OUTOFBOUNDS :
          str = "The priority value is out of bounds";
          break;
        case MSTP_ERR_INSTANCE_OUTOFBOUNDS :
          str = "The instance value is out of bounds";
          break;
        case MSTP_ERR_INSTANCE_NOT_FOUND :
          str = "Bridge-instance not found";
          break;
        default :
          str = "Can't set priority";
          break;
      }

  if (str)
    {
      cli_out (cli, "%% %s\n", str);
      return CLI_ERROR;
    }
 
  return CLI_SUCCESS;

}

ALI (no_bridge_inst_priority,
     no_spanning_inst_priority_cmd,
     "no spanning-tree instance <1-63> priority",
     CLI_NO_STR,
     SPANNING_STR,
     "Change priority for a particular instance",
     "instance id",
     "priority - Reset bridge priority to default for the instance");

#endif /* HAVE_MSTPD */

CLI (mstp_bridge_forward_time,
     mstp_bridge_forward_time_cmd,
     "bridge <1-32> forward-time <4-30>",
     BRIDGE_STR,
     BRIDGE_NAME_STR,
     "forward-time - forwarding delay time",
     "forward delay time in seconds")
{
  int ret;
  s_int32_t forward_delay;
  u_int8_t *bridge_name;
  int brno;
  struct mstp_bridge *br = NULL;
  struct mstp_bridge_info *br_config_info = NULL;
  struct mstp_bridge_config_list *br_conf_list = NULL;

  if (LAYER2_DEBUG(proto, CLI_ECHO))
    {
      if (argc == 2)
        zlog_debug (mstpm, "bridge %s forward-time %s", argv[0], argv[1]);
      else
        zlog_debug (mstpm, "spanning-tree forward-time %s", argv[0]);
    }

  MSTP_GET_BR_NAME (2, 0);

  if (argc == 2)
    CLI_GET_INTEGER_RANGE ("forward-time", forward_delay, argv[1], 4, 30);
  else
    CLI_GET_INTEGER_RANGE ("forward-time", forward_delay, argv[0], 4, 30);

  br =  (struct mstp_bridge *)mstp_find_bridge (bridge_name);
  if (br == NULL)
    {
      br_config_info = mstp_bridge_config_info_get (bridge_name);
      if (!br_config_info)
        {
          br_conf_list = mstp_bridge_config_list_new (bridge_name);
          if (br_conf_list == NULL)
            {
              cli_out (cli, "%% Could not allocate memory for bridge config\n");
              return CLI_ERROR;
            }
          mstp_bridge_config_list_link (br_conf_list);
          br_config_info = &(br_conf_list->br_info);
        }

      br_config_info->forward_time = forward_delay;
      SET_FLAG (br_config_info->config, MSTP_BRIDGE_CONFIG_FORWARD_TIME);
      return CLI_SUCCESS;
    }

  ret = mstp_api_set_forward_delay (bridge_name, forward_delay);

  if (ret < 0)
    {
      cli_out (cli, "%% Can't set forward-time\n");
      return CLI_ERROR;
    }

  return CLI_SUCCESS;
}

ALI (mstp_bridge_forward_time,
     mstp_spanning_forward_time_cmd,
     "spanning-tree forward-time <4-30>",
     SPANNING_STR,
     "forward-time - forwarding delay time",
     "forward delay time in seconds");

CLI (no_mstp_bridge_forward_time,
     no_mstp_bridge_forward_time_cmd,
     "no bridge <1-32> forward-time",
     CLI_NO_STR,
     BRIDGE_STR,
     BRIDGE_NAME_STR,
     "forward-time - set forward delay time to default value")
{
  /* Set to default forward delay to the default.  */
  int ret;
  int brno;
  u_int8_t *bridge_name;

  if (LAYER2_DEBUG(proto, CLI_ECHO))
    {
      if (argc == 1)
        zlog_debug (mstpm, "no bridge %s forward-time", argv[0]);
      else
        zlog_debug (mstpm, "no spanning-tree forward-time");
    }

  MSTP_GET_BR_NAME (1, 0);

  ret = mstp_api_set_forward_delay (bridge_name, 
                                    BRIDGE_TIMER_DEFAULT_FWD_DELAY);
  if (ret < 0)
    {
      cli_out (cli, "%% Can't unset forward-time\n");
      return CLI_ERROR;
    }

  return CLI_SUCCESS;
}

ALI (no_mstp_bridge_forward_time,
     no_mstp_spanning_forward_time_cmd,
     "no spanning-tree forward-time ",
     CLI_NO_STR,
     SPANNING_STR,
     "forward-time - forwarding delay time");

CLI (mstp_bridge_hello_time,
     mstp_bridge_hello_time_cmd,
     "bridge <1-32> hello-time <1-10>",
     BRIDGE_STR,
     BRIDGE_NAME_STR,
     "hello-time - hello BDPU interval",
     "seconds <1-10> - Hello BPDU interval")
{
  int ret;
  s_int32_t hello_time;
  int brno;
  u_int8_t *bridge_name;
  struct mstp_bridge *br = NULL;
  struct mstp_bridge_info *br_config_info = NULL;
  struct mstp_bridge_config_list *br_conf_list = NULL;

  if (LAYER2_DEBUG(proto, CLI_ECHO))
    {
      if (argc == 2)
        zlog_debug (mstpm, "bridge %s hello-time %s", argv[0], argv[1]);
      else
        zlog_debug (mstpm, "spanning-tree hello-time %s", argv[0]);
    }

  MSTP_GET_BR_NAME (2, 0);

  if (argc == 2)
    CLI_GET_INTEGER_RANGE ("hello-time", hello_time, argv[1], 1, 10);
  else
    CLI_GET_INTEGER_RANGE ("hello-time", hello_time, argv[0], 1, 10);

  br =  (struct mstp_bridge *)mstp_find_bridge (bridge_name);
  if (br == NULL)
    {
      br_config_info = mstp_bridge_config_info_get (bridge_name);

      if (!br_config_info)
        {
          br_conf_list = mstp_bridge_config_list_new (bridge_name);
          if (br_conf_list == NULL)
            {
              cli_out (cli, "%% Could not allocate memory for bridge config\n");
              return CLI_ERROR;
            }
          mstp_bridge_config_list_link (br_conf_list);
          br_config_info = &(br_conf_list->br_info);
        }

      br_config_info->hello_time = hello_time;
      SET_FLAG (br_config_info->config, MSTP_BRIDGE_CONFIG_HELLO_TIME);
      cli_out (cli, "%% Can't set hello-time\n");
      return CLI_ERROR;
    }

  ret = mstp_api_set_hello_time (bridge_name, hello_time);

  if (ret < 0)
    {
      switch (ret)
        {
          case MSTP_ERR_HELLO_NOT_CONFIGURABLE:
            cli_out(cli, "%% Can't configure hello-time for RSTP/MSTP "
                    "bridge\n");
            break;
          default:
            cli_out (cli, "%% Can't set hello-time\n");
            break;
       }
      return CLI_ERROR;
    }

  return CLI_SUCCESS;
}

ALI (mstp_bridge_hello_time,
      mstp_spanning_hello_time_cmd,
      "spanning-tree hello-time <1-10>",
      SPANNING_STR,
      "hello-time - hello BDPU interval",
      "seconds <1-10> - Hello BPDU interval");

CLI (no_mstp_bridge_hello_time,
     no_mstp_bridge_hello_time_cmd,
     "no bridge <1-32> hello-time",
     CLI_NO_STR,
     BRIDGE_STR,
     BRIDGE_NAME_STR,
     "hello-time - set Hello BPDU interval to default")
{
  /* Set to default hello time 2 sec.  */
  int ret;
  int brno;
  u_int8_t *bridge_name;

  if (LAYER2_DEBUG(proto, CLI_ECHO))
    {
      if (argc == 1)
        zlog_debug (mstpm, "no bridge %s hello-time", argv[0]);
      else
        zlog_debug (mstpm, "no spanning-tree hello-time");
    }

  MSTP_GET_BR_NAME (1, 0);

  ret = mstp_api_set_hello_time (bridge_name, BRIDGE_TIMER_DEFAULT_HELLO_TIME);
  if (ret < 0)
    {
      cli_out (cli, "%% Can't unset hello-time\n");
      return CLI_ERROR;
    }
  return CLI_SUCCESS;
}

ALI (no_mstp_bridge_hello_time,
      no_mstp_spanning_hello_time_cmd,
      "no spanning-tree hello-time",
      CLI_NO_STR,
      SPANNING_STR,
      "hello-time - hello BDPU interval");

CLI (mstp_bridge_ageing_time,
     mstp_bridge_ageing_time_cmd,
     "bridge <1-32> ageing-time <10-1000000>",
     BRIDGE_STR,
     BRIDGE_NAME_STR,
     "time a learned mac address will persist after last update",
     "ageing time in seconds")
{
  int ret;
  int brno;
  s_int32_t ageing_time;
  u_int8_t *bridge_name;
  struct mstp_bridge *br = NULL;
  struct mstp_bridge_info *br_config_info = NULL;
  struct mstp_bridge_config_list *br_conf_list = NULL;

  if (LAYER2_DEBUG(proto, CLI_ECHO))
    {
      if (argc == 2)
        zlog_debug (mstpm, "bridge %s ageing-time %s", argv[0], argv[1]);
      else
        zlog_debug (mstpm, "spanning-tree ageing-time %s", argv[0]);
    }

  MSTP_GET_BR_NAME (2, 0);
  if (argc == 2)
    CLI_GET_INTEGER_RANGE ("ageing-time", ageing_time, argv[1], 10, 1000000);
  else
    CLI_GET_INTEGER_RANGE ("ageing-time", ageing_time, argv[0], 10, 1000000);

  br =  (struct mstp_bridge *)mstp_find_bridge (bridge_name);
  if (br == NULL)
    {
      br_config_info = mstp_bridge_config_info_get (bridge_name);
      if (!br_config_info)
        {
          br_conf_list = mstp_bridge_config_list_new (bridge_name);
          if (br_conf_list == NULL)
            {
              cli_out (cli, "%% Could not allocate memory for bridge config\n");
              return CLI_ERROR;
            }
          mstp_bridge_config_list_link (br_conf_list);
          br_config_info = &(br_conf_list->br_info);
        }

      br_config_info->ageing_time = ageing_time;
      SET_FLAG (br_config_info->config, MSTP_BRIDGE_CONFIG_AGEING_TIME);
      cli_out (cli, "%% Can't set ageing-time\n");
      return CLI_ERROR;
    }

  ret = mstp_api_set_ageing_time (bridge_name, ageing_time);
  if (ret < 0)
    {
      cli_out (cli, "%% Can't set ageing-time\n");
      return CLI_ERROR;
    }
  return CLI_SUCCESS;
}

ALI (mstp_bridge_ageing_time,
     spanning_tree_ageing_time_cmd,
     "spanning-tree ageing-time <10-1000000>",
     SPANNING_STR,
     "time a learned mac address will persist after last update",
     "ageing time in seconds");

CLI (no_mstp_bridge_ageing_time,
     no_mstp_bridge_ageing_time_cmd,
     "no bridge <1-32> ageing-time",
     CLI_NO_STR,
     BRIDGE_STR,
     BRIDGE_NAME_STR,
     "ageing-time")
{
  int ret;
  int brno;
  u_int8_t *bridge_name;

  if (LAYER2_DEBUG(proto, CLI_ECHO))
    {
      if (argc == 1)
        zlog_debug (mstpm, "no bridge %s ageing-time", argv[0]);
      else
        zlog_debug (mstpm, "no spanning-tree ageing-time");
    }

  MSTP_GET_BR_NAME (1, 0);

  ret = mstp_api_set_ageing_time (bridge_name,
                                  BRIDGE_TIMER_DEFAULT_AGEING_TIME);
  if (ret < 0)
    {
      cli_out (cli, "%% Can't unset ageing-time\n");
      return CLI_ERROR;
    }
  return CLI_SUCCESS;
}

ALI (no_mstp_bridge_ageing_time,
     no_spanning_tree_ageing_time_cmd,
     "no spanning-tree ageing-time",
     CLI_NO_STR,
     SPANNING_STR,
     "ageing-time");

CLI (mstp_bridge_max_age,
     mstp_bridge_max_age_cmd,
     "bridge <1-32> max-age <6-40>",
     BRIDGE_STR,
     BRIDGE_NAME_STR,
     "max-age",
     "seconds <6-40> - Maximum time to listen for root bridge in seconds")
{
  int ret;
  s_int32_t max_age;
  int brno;
  u_int8_t *bridge_name;
  struct mstp_bridge *br = NULL;
  struct mstp_bridge_info *br_config_info = NULL;
  struct mstp_bridge_config_list *br_conf_list = NULL;

  MSTP_GET_BR_NAME (2, 0);

  if (argc == 2)
    CLI_GET_INTEGER_RANGE ("max-age", max_age, argv[1], 6, 40);
  else
    CLI_GET_INTEGER_RANGE ("max-age", max_age, argv[0], 6, 40);

  if (LAYER2_DEBUG(proto, CLI_ECHO))
    {
      if (argc == 2)
        zlog_debug (mstpm, "bridge %s max-age %s", argv[0], argv[1]);
      else
        zlog_debug (mstpm, "spanning-tree max-age %s", argv[0]);
    }

  br =  (struct mstp_bridge *)mstp_find_bridge (bridge_name);
  if (br == NULL)
    {
      br_config_info = mstp_bridge_config_info_get (bridge_name);
      if (!br_config_info)
        {
          br_conf_list = mstp_bridge_config_list_new (bridge_name);
          if (br_conf_list == NULL)
            {
              cli_out (cli, "%% Could not allocate memory for bridge config\n");
              return CLI_ERROR;
            }
          mstp_bridge_config_list_link (br_conf_list);
          br_config_info = &(br_conf_list->br_info);
        }

      br_config_info->max_age = max_age;
      SET_FLAG (br_config_info->config, MSTP_BRIDGE_CONFIG_MAX_AGE);
      cli_out (cli, "%% Can't set max-age\n");
      return CLI_ERROR;
    }

  ret = mstp_api_set_max_age (bridge_name, max_age);
  if (ret < 0)
    {
      cli_out (cli, "%% Can't set max-age\n");
      return CLI_ERROR;
    }
  return CLI_SUCCESS;
}

ALI (mstp_bridge_max_age,
     mstp_spanning_max_age_cmd,
     "spanning-tree max-age <6-40>",
     SPANNING_STR,
     "max-age",
     "seconds <6-40> - Maximum time to listen for root bridge in seconds");

CLI (no_mstp_bridge_max_age,
     no_mstp_bridge_max_age_cmd,
     "no bridge <1-32> max-age",
     CLI_NO_STR,
     BRIDGE_STR,
     BRIDGE_NAME_STR,
     "max-age - set time to listen for root bridge to default")
{

  /* Set to default max age 20 sec.  */
  int ret;
  int brno;
  u_int8_t *bridge_name;

  if (LAYER2_DEBUG(proto, CLI_ECHO))
    {
      if (argc == 1)
        zlog_debug (mstpm, "no bridge %s max-age", argv[0]);
      else
        zlog_debug (mstpm, "no spanning-tree max-age");
    }

  MSTP_GET_BR_NAME (1, 0);

  ret = mstp_api_set_max_age (bridge_name, BRIDGE_TIMER_DEFAULT_MAX_AGE);
  if (ret < 0)
    {
      cli_out (cli, "%% Can't unset max-age\n");
      return CLI_ERROR;
    }
  return CLI_SUCCESS;
}

ALI (no_mstp_bridge_max_age,
     no_mstp_spanning_max_age_cmd,
     "no spanning-tree max-age ",
     CLI_NO_STR,
     SPANNING_STR,
     "max-age");

CLI (bridge_max_hops,
     bridge_max_hops_cmd,
     "bridge <1-32> max-hops <1-40>",
     BRIDGE_STR,
     BRIDGE_NAME_STR,
     "max-hops",
     "hops <1-40> - Maximum hops the BPDU will be valid")
{
  int ret;
  int brno;
  s_int32_t max_hops;
  u_int8_t *bridge_name;
  struct mstp_bridge *br = NULL;
  struct mstp_bridge_info *br_config_info = NULL;
  struct mstp_bridge_config_list *br_conf_list = NULL;

  MSTP_GET_BR_NAME (2, 0);

  if (argc == 2)
    CLI_GET_INTEGER_RANGE ("max-hops", max_hops, argv[1],
                           MSTP_MIN_BRIDGE_MAX_HOPS,
                           MSTP_MAX_BRIDGE_MAX_HOPS);
  else
    CLI_GET_INTEGER_RANGE ("max-hops", max_hops, argv[0],
                           MSTP_MIN_BRIDGE_MAX_HOPS,
                           MSTP_MAX_BRIDGE_MAX_HOPS);

  if (LAYER2_DEBUG(proto, CLI_ECHO))
    {
      if (argc == 2)
        zlog_debug (mstpm, "bridge %s max-hops %s", argv[0], argv[1]);
      else
        zlog_debug (mstpm, "spanning-tree max-hops %s", argv[0]);
    }


  br =  (struct mstp_bridge *)mstp_find_bridge (bridge_name);
  if (br == NULL)
    {
      br_config_info = mstp_bridge_config_info_get (bridge_name);
      if (!br_config_info)
        {
          br_conf_list = mstp_bridge_config_list_new (bridge_name);
          if (br_conf_list == NULL)
            {
              cli_out (cli, "%% Could not allocate memory for bridge config\n");
              return CLI_ERROR;
            }
          mstp_bridge_config_list_link (br_conf_list);
          br_config_info = &(br_conf_list->br_info);
        }

      br_config_info->max_hops = max_hops;
      SET_FLAG (br_config_info->config, MSTP_BRIDGE_CONFIG_MAX_HOPS);
      cli_out (cli, "%% Bridge does not Exist\n");
      return CLI_ERROR;
    }

  if(!(IS_BRIDGE_MSTP(br)))
    {
      cli_out (cli,"%%Bridge is not MSTP\n");
      return CLI_ERROR;
    }

  ret = mstp_api_set_max_hops (bridge_name, max_hops);
  if (ret < 0)
    {
      cli_out (cli, "%% Can't set max-hops: \n");
      return CLI_ERROR;
    }
  return CLI_SUCCESS;
}

ALI (bridge_max_hops,
     spanning_max_hops_cmd,
     "spanning-tree max-hops <1-40>",
     SPANNING_STR,
     "max-hops",
     "hops <1-40> - Maximum hops the BPDU will be valid");

CLI (no_bridge_max_hops,
     no_bridge_max_hops_cmd,
     "no bridge <1-32> max-hops",
     CLI_NO_STR,
     BRIDGE_STR,
     BRIDGE_NAME_STR,
     "max-hops - set time to listen for root bridge to default")
{
  /* Set to default max age 20 sec.  */
  struct mstp_bridge *br = NULL;
  int ret;
  int brno;
  u_int8_t *bridge_name;

  if (LAYER2_DEBUG(proto, CLI_ECHO))
    {
      if (argc == 1)
        zlog_debug (mstpm, "no bridge %s max-hops", argv[0]);
      else
        zlog_debug (mstpm, "no spanning-tree max-hops");
    }

  MSTP_GET_BR_NAME (1, 0);

  br =  (struct mstp_bridge *)mstp_find_bridge (argv[0]);
  if (br == NULL)
    {
      cli_out (cli,"%%Bridge does not Exist\n");
      return CLI_ERROR;
    }

  if (!IS_BRIDGE_MSTP(br))
    {
      cli_out (cli,"%%Bridge is not MSTP\n");
      return CLI_ERROR;
    }
  ret = mstp_api_set_max_hops (bridge_name, MST_DEFAULT_MAXHOPS);

  if (ret < 0)
    {
      cli_out (cli, "%% Can't unset max-hops:\n");
      return CLI_ERROR;
    }

  return CLI_SUCCESS;

}

ALI (no_bridge_max_hops,
     no_spanning_max_hops_cmd,
     "no spanning-tree max-hops <1-40>",
     CLI_NO_STR,
     SPANNING_STR,
     "max-hops",
     "hops <1-40> - Maximum hops the BPDU will be valid");

CLI (mstp_bridge_transmit_hold_count,
     mstp_bridge_transmit_hold_count_cmd,
     "bridge <1-32> transmit-holdcount <1-10>",
     BRIDGE_STR,
     BRIDGE_NAME_STR,
     "Transmit hold count of the bridge",
     "range of the transmitholdcount")
{
  int ret;
  int brno;
  u_int8_t *bridge_name;
  unsigned char txholdcount;
  struct mstp_bridge *br = NULL;
  struct mstp_bridge_info *br_config_info = NULL;
  struct mstp_bridge_config_list *br_conf_list = NULL;

  MSTP_GET_BR_NAME (2, 0);

  if (argc == 2)
    CLI_GET_INTEGER_RANGE ("Transmit hold count", txholdcount, argv[1], 1, 10);
  else
    CLI_GET_INTEGER_RANGE ("Transmit hold count", txholdcount, argv[0], 1, 10);

  if (LAYER2_DEBUG(proto, CLI_ECHO))
    {
      if (argc == 2)
        zlog_debug (mstpm, "bridge %s Transmit hold count %s", argv[0], argv[1]);
      else
        zlog_debug (mstpm, "spanning-tree Transmit hold count %s", argv[0]);
    }

  br =  (struct mstp_bridge *)mstp_find_bridge (bridge_name);

  if (br == NULL)
    {
      br_config_info = mstp_bridge_config_info_get (bridge_name);

      if (!br_config_info)
        {
          br_conf_list = mstp_bridge_config_list_new (bridge_name);

          if (br_conf_list == NULL)
            {
              cli_out (cli, "%% Could not allocate memory for bridge config\n");
              return CLI_ERROR;
            }

          mstp_bridge_config_list_link (br_conf_list);
          br_config_info = &(br_conf_list->br_info);
        }

      br_config_info-> transmit_hold_count= txholdcount;
      SET_FLAG (br_config_info->config, MSTP_BRIDGE_CONFIG_TX_HOLD_COUNT);
      cli_out (cli, "%% Can't set Transmit hold count\n");
      return CLI_ERROR;
    }

  ret = mstp_api_set_transmit_hold_count (bridge_name, txholdcount);

  if (ret < 0)
    {
      cli_out (cli, "%% Can't set transmit hold count\n");
      return CLI_ERROR;
    }

  return CLI_SUCCESS;
}

ALI (mstp_bridge_transmit_hold_count,
     mstp_spanning_transmit_hold_count_cmd,
     "spanning-tree transmit-holdcount <1-10>",
     SPANNING_STR,
     "Transmit hold count of the bridge",
     "range of the transmitholdcount");

CLI (no_mstp_bridge_transmit_hold_count,
     no_mstp_bridge_transmit_hold_count_cmd,
     "no bridge <1-32> transmit-holdcount",
     CLI_NO_STR,
     BRIDGE_STR,
     BRIDGE_NAME_STR,
     "transmit hold count - set count to default")
{

  /* Set to default max age 20 sec.  */
  int ret;
  int brno;
  u_int8_t *bridge_name;
  struct mstp_bridge *br = NULL;

  if (LAYER2_DEBUG(proto, CLI_ECHO))
    {
      if (argc == 1)
        zlog_debug (mstpm, "no bridge %s transmit hold count", argv[0]);
      else
        zlog_debug (mstpm, "no spanning-tree transmit hold count");
    }
  
  br = (struct mstp_bridge *)mstp_find_bridge (argv[0]);
  
  if(br==NULL)
    {
      cli_out (cli, "%% bridge does not exist\n");
      return CLI_ERROR;
    }
  
  MSTP_GET_BR_NAME (1, 0);

  ret = mstp_api_set_transmit_hold_count (bridge_name, MSTP_TX_HOLD_COUNT);

  if (ret < 0)
    {
      cli_out (cli, "%% Can't unset transmit hold count\n");
      return CLI_ERROR;
    }

  return CLI_SUCCESS;
}

ALI (no_mstp_bridge_transmit_hold_count,
     no_mstp_spanning_transmit_hold_count_cmd,
     "no spanning-tree transmit-holdcount <1-10>",
     CLI_NO_STR,
     SPANNING_STR,
     "Transmit hold count of the bridge",
     "range of the transmitholdcount");

CLI (mstp_clear_spanning_tree_detected_protocols,
     mstp_clear_spanning_tree_detected_protocols_cmd,
     "clear spanning-tree detected protocols bridge <1-32>",
     "clear", 
     "spanning-tree",
     "detected",
     "protocols",
     "bridge",
     "NAME - bridge name")
{
  int ret;
  int brno;

  if (LAYER2_DEBUG(proto, CLI_ECHO))
    zlog_debug (mstpm, "clear spanning-tree detected protocols bridge %s", argv[0]);

  CLI_GET_INTEGER_RANGE ("Max bridge groups", brno, argv[0], 1, 32);

  ret = mstp_api_mcheck (argv[0], 0);
  if (ret < 0)
    {
      cli_out (cli, "%% Could not clear protocols for bridge %s\n", argv[0]);
      return CLI_ERROR;
    }
  return CLI_SUCCESS;
}



CLI (mstp_clear_spanning_tree_detected_protocols_interface,
     mstp_clear_spanning_tree_detected_protocols_interface_cmd,
     "clear spanning-tree detected protocols interface INTERFACE",
     "clear", 
     "spanning-tree",
     "detected",
     "protocols",
     "interface",
     "INTERFACE - interface name")
{
  int ret;
  struct interface *ifp = if_lookup_by_name (&cli->vr->ifm, argv[0]);

  if (LAYER2_DEBUG(proto, CLI_ECHO))
    zlog_debug (mstpm, "clear spanning-tree detected protocols interface %s", argv[0]);

  if ((ifp == NULL) || (ifp->port == NULL))
    {
      cli_out (cli, "%% Can't find interface %s\n", argv[0]);
      return CLI_ERROR;
    }

  ret = mstp_api_mcheck (ifp->bridge_name, argv[0]);
  if (ret < 0)
    {
      cli_out (cli, "%% Could not clear protocols on port %s\n", argv[0]);
      return CLI_ERROR;
    }

  return CLI_SUCCESS;
}

CLI  (mstp_bridge_portfast_bpduguard,
      mstp_bridge_portfast_bpduguard_cmd,
      "bridge <1-32> spanning-tree portfast bpdu-guard",
      BRIDGE_STR,
      BRIDGE_NAME_STR,
      "spanning-tree",
      "portfast",
      "guard the portfast ports against bpdu receive")
{
  int ret;
  int brno;
  u_int8_t *bridge_name;
  struct mstp_bridge *br = NULL;
  struct mstp_bridge_info *br_config_info = NULL;
  struct mstp_bridge_config_list *br_conf_list = NULL;

  if (LAYER2_DEBUG(proto, CLI_ECHO))
    {
      if (argc == 2)
        zlog_debug (mstpm, "bridge %s spanning-tree portfast bpdu-guard",
                    argv[0]);
      else
        zlog_debug (mstpm, "spanning-tree portfast bpdu-guard");
    }

  MSTP_GET_BR_NAME (2, 0);

  br = (struct mstp_bridge *)mstp_find_bridge (bridge_name);

  if (! br)
    {
      br_config_info = mstp_bridge_config_info_get (bridge_name);
      if (!br_config_info)
        {
          br_conf_list = mstp_bridge_config_list_new (bridge_name);
          if (br_conf_list == NULL)
            {
              cli_out (cli, "%% Could not allocate memory for bridge config\n");
              return CLI_ERROR;
            }
          mstp_bridge_config_list_link (br_conf_list);
          br_config_info = &(br_conf_list->br_info);
        }
      br_config_info->bpdugaurd = PAL_TRUE;
      SET_FLAG (br_config_info->config, MSTP_BRIDGE_CONFIG_PORTFAST_BPDUGUARD);
      return CLI_SUCCESS;
    }

  ret = mstp_api_set_bridge_portfast_bpduguard (bridge_name, PAL_TRUE);
  if (ret < 0)
    {
      cli_out (cli, "%% Can't update bridge\n");
      return CLI_ERROR;
    }

  return CLI_SUCCESS;
}

ALI  (mstp_bridge_portfast_bpduguard,
      mstp_span_portfast_bpduguard_cmd,
      "spanning-tree portfast bpdu-guard",
      "spanning-tree",
      "portfast",
      "guard the portfast ports against bpdu receive");

CLI  (no_mstp_bridge_portfast_bpduguard,
      no_mstp_bridge_portfast_bpduguard_cmd,
      "no bridge <1-32> spanning-tree portfast bpdu-guard",
      CLI_NO_STR,
      BRIDGE_STR,
      BRIDGE_NAME_STR,
      "spanning-tree",
      "portfast",
      "Disable Guard of portfast port against bpdu receive")
{
  int ret;
  int brno;
  u_int8_t *bridge_name;
  struct mstp_bridge *br = NULL;

  if (LAYER2_DEBUG(proto, CLI_ECHO))
    {
      if (argc == 2)
        zlog_debug (mstpm, "no bridge %s spanning-tree portfast bpdu-guard",
                    argv[0]);
      else
        zlog_debug (mstpm, "spanning-tree portfast bpdu-guard");
    }

  MSTP_GET_BR_NAME (2, 0);

  br = (struct mstp_bridge *)mstp_find_bridge (bridge_name);

  if (! br)
    {
      cli_out (cli, "%% Bridge %s does not exist\n", bridge_name);
      return CLI_ERROR;
    }

  ret = mstp_api_set_bridge_portfast_bpduguard (bridge_name, PAL_FALSE);

  if (ret < 0)
    {
      cli_out (cli, "%% Can't update bridge\n");
      return CLI_ERROR;
    }

  return CLI_SUCCESS;

}

ALI  (no_mstp_bridge_portfast_bpduguard,
      no_mstp_span_portfast_bpduguard_cmd,
      "no spanning-tree portfast bpdu-guard",
      CLI_NO_STR,
      "spanning-tree",
      "portfast",
      "guard the portfast ports against bpdu receive");

CLI  (mstp_spanning_tree_errdisable_timeout_enable,
      mstp_spanning_tree_errdisable_timeout_enable_cmd,
      "bridge <1-32> spanning-tree errdisable-timeout enable",
      BRIDGE_STR,
      BRIDGE_NAME_STR,
      "spanning-tree",
      "errdisable-timeout",
      "enable the timeout mechanism for the port to be enabled back")
{
  int ret;
  u_int8_t *bridge_name;
  struct mstp_bridge *br = NULL;
  struct mstp_bridge_info *br_config_info = NULL;
  struct mstp_bridge_config_list *br_conf_list = NULL;
  int brno;

  if (LAYER2_DEBUG(proto, CLI_ECHO))
    {
      if (argc == 2)
        zlog_debug (mstpm, "bridge %s spanning-tree errdisable-timeout enable",
                    argv[0]);
      else
        zlog_debug (mstpm, "spanning-tree errdisable-timeout enable");
    }

  MSTP_GET_BR_NAME (2, 0);

  br = (struct mstp_bridge *)mstp_find_bridge (bridge_name);

  if (! br)
    {
      br_config_info = mstp_bridge_config_info_get (bridge_name);

      if (!br_config_info)
        {
          br_conf_list = mstp_bridge_config_list_new (bridge_name);

          if (br_conf_list == NULL)
            {
              cli_out (cli, "%% Could not allocate memory for bridge config\n");
              return CLI_ERROR;
            }

          mstp_bridge_config_list_link (br_conf_list);
          br_config_info = &(br_conf_list->br_info);
        }

      br_config_info->err_disable = PAL_TRUE;
      SET_FLAG (br_config_info->config, MSTP_BRIDGE_CONFIG_ERRDISABLE);
      return CLI_SUCCESS;
    }

  ret = mstp_api_set_bridge_errdisable_timeout_enable (bridge_name, PAL_TRUE);

  if (ret < 0)
    {
      cli_out (cli, "%% Can't update bridge\n");
      return CLI_ERROR;
    }

  return CLI_SUCCESS;

}

ALI  (mstp_spanning_tree_errdisable_timeout_enable,
      mstp_spanning_tree_errdisable_timeout_enable_cmd1,
      "spanning-tree errdisable-timeout enable",
      "spanning-tree",
      "errdisable-timeout",
      "enable the timeout mechanism for the port to be enabled back");

CLI  (no_mstp_spanning_tree_errdisable_timeout_enable,
      no_mstp_spanning_tree_errdisable_timeout_enable_cmd,
      "no bridge <1-32> spanning-tree errdisable-timeout enable",
      CLI_NO_STR,
      BRIDGE_STR,
      BRIDGE_NAME_STR,
      "spanning-tree",
      "errdisable-timeout",
      "disable the timeout mechanism for the port to be enbaled back")
{
  int ret;
  struct mstp_bridge *br = NULL;
  u_int8_t *bridge_name;
  int brno;

  if (LAYER2_DEBUG(proto, CLI_ECHO))
    {
      if (argc == 2)
        zlog_debug (mstpm, "no bridge %s spanning-tree errdisable-timeout"
                    " enable", argv[0]);
      else
        zlog_debug (mstpm, "no spanning-tree errdisable-timeout enable");
    }

  MSTP_GET_BR_NAME (2, 0);

  br = (struct mstp_bridge *)mstp_find_bridge (bridge_name);

  if (! br)
    {
      cli_out (cli, "%% Bridge %s does not exist\n", bridge_name);
      return CLI_ERROR;
    }

  ret = mstp_api_set_bridge_errdisable_timeout_enable (bridge_name, PAL_FALSE);

  if (ret < 0)
    {
      cli_out (cli, "%% Can't update bridge\n");
      return CLI_ERROR;
    }

  return CLI_SUCCESS;

}

ALI  (no_mstp_spanning_tree_errdisable_timeout_enable,
      no_mstp_spanning_tree_errdisable_timeout_enable_cmd1,
      "no spanning-tree errdisable-timeout enable",
      CLI_NO_STR,
      "spanning-tree",
      "errdisable-timeout",
      "enable the timeout mechanism for the port to be enabled back");

CLI  (mstp_spanning_tree_errdisable_timeout_interval,
      mstp_spanning_tree_errdisable_timeout_interval_cmd,
      "bridge <1-32> spanning-tree errdisable-timeout interval <10-1000000>",
      BRIDGE_STR,
      BRIDGE_NAME_STR,
      "spanning-tree",
      "errdisable-timeout",
      "interval after which port shall be enabled",
      "errdisable-timeout interval in seconds")
{
  int ret;
  struct mstp_bridge *br = NULL;
  struct mstp_bridge_info *br_config_info = NULL;
  struct mstp_bridge_config_list *br_conf_list = NULL;
  u_int8_t *bridge_name;
  s_int32_t time;
  int brno;

  if (LAYER2_DEBUG(proto, CLI_ECHO))
    {
      if (argc == 3)
        zlog_debug (mstpm, "bridge %s spanning-tree errdiable-timeout "
                    "interval %s", argv[0], argv[2]);
      else
        zlog_debug (mstpm, "spanning-tree errdiable-timeout interval %s",
                    argv[0]);
    }

  MSTP_GET_BR_NAME (3, 0);

  if (argc == 3)
    CLI_GET_INTEGER_RANGE ("errdisable-timeout", time, argv[2], 10, 1000000);
  else
    CLI_GET_INTEGER_RANGE ("errdisable-timeout", time, argv[0], 10, 1000000);

  br = (struct mstp_bridge *)mstp_find_bridge (bridge_name);
  if (! br)
    {
      br_config_info = mstp_bridge_config_info_get (bridge_name);
      if (!br_config_info)
        {
          br_conf_list = mstp_bridge_config_list_new (bridge_name);
          if (br_conf_list == NULL)
            {
              cli_out (cli, "%% Could not allocate memory for bridge config\n");
              return CLI_ERROR;
            }
          mstp_bridge_config_list_link (br_conf_list);
          br_config_info = &(br_conf_list->br_info);
        }
      br_config_info->errdisable_timeout_interval = time;
      SET_FLAG (br_config_info->config, MSTP_BRIDGE_CONFIG_ERRDISABLE_TIMEOUT);
      return CLI_SUCCESS;
    }

  ret = mstp_api_set_bridge_errdisable_timeout_interval (bridge_name, time);

  if (ret < 0)
    {
      cli_out (cli, "%% Can't update bridge\n");
      return CLI_ERROR;
    }

  return CLI_SUCCESS;

}

ALI  (mstp_spanning_tree_errdisable_timeout_interval,
      mstp_spanning_tree_errdisable_timeout_interval_cmd1,
      "spanning-tree errdisable-timeout interval <10-1000000>",
      "spanning-tree",
      "errdisable-timeout",
      "interval after which port shall be enabled",
      "errdisable-timeout interval in seconds");

CLI  (no_mstp_spanning_tree_errdisable_timeout_interval,
      no_mstp_spanning_tree_errdisable_timeout_interval_cmd,
      "no bridge <1-32> spanning-tree errdisable-timeout interval ",
      CLI_NO_STR,
      BRIDGE_STR,
      BRIDGE_NAME_STR,
      "spanning-tree",
      "errdisable-timeout",
      "disable the errdisable-timeout interval setting")
{
  int ret;
  struct mstp_bridge *br = NULL;
  u_int8_t *bridge_name;
  int brno;

  if (LAYER2_DEBUG(proto, CLI_ECHO))
    {
      if (argc == 2)
        zlog_debug (mstpm, "no bridge %s spanning-tree errdiable-timeout "
                    "interval", argv[0]);
      else
        zlog_debug (mstpm, "no spanning-tree errdiable-timeout interval");
    }

  MSTP_GET_BR_NAME (2, 0);

  br = (struct mstp_bridge *)mstp_find_bridge (bridge_name);
  if (! br)
    {
      cli_out (cli, "%% Bridge %s does not exist\n", bridge_name);
      return CLI_ERROR;
    }

  ret = mstp_api_set_bridge_errdisable_timeout_interval (bridge_name,
             MSTP_BRIDGE_ERRDISABLE_DEFAULT_TIMEOUT_INTERVAL / L2_TIMER_SCALE_FACT);

  if (ret < 0)
    {
      cli_out (cli, "%% Can't update bridge\n");
      return CLI_ERROR;
    }

  return CLI_SUCCESS;

}

ALI  (no_mstp_spanning_tree_errdisable_timeout_interval,
      no_mstp_spanning_tree_errdisable_timeout_interval_cmd1,
      "no spanning-tree errdisable-timeout interval ",
      CLI_NO_STR,
      "spanning-tree",
      "errdisable-timeout",
      "interval after which port shall be enabled");

CLI  (mstp_bridge_portfast_bpdufilter,
      mstp_bridge_portfast_bpdufilter_cmd,
      "bridge <1-32> spanning-tree portfast bpdu-filter",
      BRIDGE_STR,
      BRIDGE_NAME_STR,
      "spanning-tree",
      "portfast",
      "Filter the BPDUs on portfast enabled ports")
{
  int ret;
  int brno;
  u_int8_t *bridge_name;
  struct mstp_bridge *br = NULL;
  struct mstp_bridge_info *br_config_info = NULL;
  struct mstp_bridge_config_list *br_conf_list = NULL;

  if (LAYER2_DEBUG(proto, CLI_ECHO))
    {
      if (argc == 2)
        zlog_debug (mstpm, "bridge %s spanning-tree portfast bpdu-filter",
                    argv[0]);
      else
        zlog_debug (mstpm, "spanning-tree portfast bpdu-filter");
    }

  MSTP_GET_BR_NAME (2, 0);

  br = (struct mstp_bridge *)mstp_find_bridge (bridge_name);

  if (! br)
    {
      br_config_info = mstp_bridge_config_info_get (bridge_name);
      if (!br_config_info)
        {
          br_conf_list = mstp_bridge_config_list_new (bridge_name);
          if (br_conf_list == NULL)
            {
              cli_out (cli, "%% Could not allocate memory for bridge config\n");
              return CLI_ERROR;
            }
          mstp_bridge_config_list_link (br_conf_list);
          br_config_info = &(br_conf_list->br_info);
        }
      br_config_info->bpdugaurd = PAL_TRUE;
      SET_FLAG (br_config_info->config, MSTP_BRIDGE_CONFIG_BPDUFILTER);
      return CLI_SUCCESS;
    }

  ret = mstp_api_set_bridge_portfast_bpdufilter (bridge_name, PAL_TRUE);

  if (ret < 0)
    {
      cli_out (cli, "%% Can't update bridge\n");
      return CLI_ERROR;
    }

  return CLI_SUCCESS;
}

ALI  (mstp_bridge_portfast_bpdufilter,
      mstp_span_portfast_bpdufilter_cmd,
      "spanning-tree portfast bpdu-filter",
      "spanning-tree",
      "portfast",
      "Filter the BPDUs on portfast enabled ports");

CLI  (no_mstp_bridge_portfast_bpdufilter,
      no_mstp_bridge_portfast_bpdufilter_cmd,
      "no bridge <1-32> spanning-tree portfast bpdu-filter",
      CLI_NO_STR,
      BRIDGE_STR,
      BRIDGE_NAME_STR,
      "spanning-tree",
      "portfast",
      "disable bpdu filter ")
{
  int ret;
  int brno;
  u_int8_t *bridge_name;
  struct mstp_bridge *br = NULL;

  if (LAYER2_DEBUG(proto, CLI_ECHO))
    {
      if (argc == 2)
        zlog_debug (mstpm, "no bridge %s spanning-tree portfast bpdu-filter",
                    argv[0]);
      else
        zlog_debug (mstpm, "no spanning-tree portfast bpdu-filter");
    }

  MSTP_GET_BR_NAME (2, 0);

  br = (struct mstp_bridge *)mstp_find_bridge (bridge_name);

  if (! br)
    {
      cli_out (cli, "%% Bridge %s does not exist\n", bridge_name);
      return CLI_ERROR;
    }

  ret = mstp_api_set_bridge_portfast_bpdufilter (bridge_name, PAL_FALSE);

  if (ret < 0)
    {
      cli_out (cli, "%% Can't update bridge\n");
      return CLI_ERROR;
    }

  return CLI_SUCCESS;
}

ALI  (no_mstp_bridge_portfast_bpdufilter,
      no_mstp_span_portfast_bpdufilter_cmd,
      "no spanning-tree portfast bpdu-filter",
      CLI_NO_STR,
      "spanning-tree",
      "portfast",
      "Filter the BPDUs on portfast enabled ports");


CLI (mstp_bridge_cisco_interop,
     mstp_bridge_cisco_interop_cmd,
     "bridge <1-32> cisco-interoperability ( enable | disable)",
     BRIDGE_STR,
     BRIDGE_NAME_STR,
     "Configure CISCO Interoperability",
     "Enable CISCO Interoperability",
     "Disable CISCO Interoperability")
{
  int brno;
  u_int8_t *bridge_name;
  u_int8_t enable;
  struct mstp_bridge *br = NULL;
  struct mstp_bridge_info *br_config_info = NULL;
  struct mstp_bridge_config_list *br_conf_list = NULL;

  if (LAYER2_DEBUG(proto, CLI_ECHO))
    {
      if (argc == 2)
        zlog_debug (mstpm, "bridge %s cisco-interoperability %s",
                    argv[0], argv[1]);
      else
        zlog_debug (mstpm, "spanning-tree cisco-interoperability %s",
                    argv[0]);
    }

  enable = PAL_FALSE;
  if (argc == 2)
    {
      if (*argv[1] == 'e')
        enable = PAL_TRUE;
      else
        enable = PAL_FALSE;
    }
  else
    {
      if (*argv[0] == 'e')
        enable = PAL_TRUE;
      else
        enable = PAL_FALSE;
    }

  MSTP_GET_BR_NAME (2, 0);

  br =  (struct mstp_bridge *)mstp_find_bridge (bridge_name);

  if (br == NULL)
    {
      br_config_info = mstp_bridge_config_info_get (bridge_name);
      if (!br_config_info)
        {
          br_conf_list = mstp_bridge_config_list_new (bridge_name);
          if (br_conf_list == NULL)
            {
              cli_out (cli, "%% Could not allocate memory for bridge config\n");
              return CLI_ERROR;
            }
          mstp_bridge_config_list_link (br_conf_list);
          br_config_info = &(br_conf_list->br_info);
        }

      if (argc == 2)
        {
          if (*argv[1] == 'e')
            br_config_info->cisco_interop = PAL_TRUE;
          else
            br_config_info->cisco_interop = PAL_FALSE;
        }
      else
        {
          if (*argv[0] == 'e')
            br_config_info->cisco_interop = PAL_TRUE;
          else
            br_config_info->cisco_interop = PAL_FALSE;
        }

      return CLI_SUCCESS;
    }

  mstp_bridge_set_cisco_interoperability (br, enable);

  return CLI_SUCCESS;
}

ALI (mstp_bridge_cisco_interop,
     mstp_spanning_cisco_interop_cmd,
     "spanning-tree cisco-interoperability ( enable | disable)",
     SPANNING_STR,
     "Configure CISCO Interoperability",
     "Enable CISCO Interoperability",
     "Disable CISCO Interoperability");

#ifdef HAVE_RPVST_PLUS
CLI (rpvst_plus_config,
    rpvst_plus_config_cmd,
    "spanning-tree rpvst+ configuration",
    "Spanning tree commands",
    "Rapid per vlan spanning tree",
    "Configuration")
{
  if (LAYER2_DEBUG(proto, CLI_ECHO))
    zlog_debug (mstpm, "spanning-tree rpvst+ configuration");

  cli->mode = RPVST_PLUS_CONFIG_MODE;

  return CLI_SUCCESS;

}
CLI (bridge_vlan_priority,
     bridge_vlan_priority_cmd,
     "bridge <1-32> vlan <2-4094> priority <0-61440>",
     "Bridge group commands",
     "Bridge Group name for bridging",
     "Change priority for a particular instance",
     "instance id",
     "priority - bridge priority for the common instance",
     "bridge priority in increments of 4096 (Lower priority indicates greater likelihood of becoming root)")
{
  int ret;
  u_int32_t priority = 0;
  int instance = 0;
  u_int16_t vid = 0;
  int brno;
  u_int8_t *bridge_name;
  struct mstp_bridge *br = NULL;
  struct mstp_bridge_info *br_config_info = NULL;
  struct mstp_bridge_config_list *br_conf_list = NULL;
  struct mstp_bridge_instance_info *br_inst_info = NULL; 
  struct mstp_bridge_instance_list *br_inst_list = NULL;

  if (LAYER2_DEBUG(proto, CLI_ECHO))
    {
      if (argc == 3)
        zlog_debug(mstpm, "bridge %s vlan %s priority %s",
                   argv[0], argv[1],argv[2]);
      else
        zlog_debug(mstpm, "spanning-tree vlan %s priority %s",
                   argv[0], argv[1]);
    }

  MSTP_GET_BR_NAME(3, 0);

  if (argc == 3)
    {
      CLI_GET_UINT32_RANGE("priority", priority, argv[2], 0, 
                            MSTP_MAX_BRIDGE_PRIORITY);
      CLI_GET_INTEGER_RANGE("vlan", vid, argv[1], 2, 4094);
    }
  else
    {
      CLI_GET_UINT32_RANGE("priority", priority, argv[1], 0, 
                            MSTP_MAX_BRIDGE_PRIORITY);

      CLI_GET_INTEGER_RANGE("vlan", vid, argv[0], MSTP_VLAN_MIN, 
                            MSTP_VLAN_MAX);
    }

  br = (struct mstp_bridge *)mstp_find_bridge (bridge_name);

  if (br != NULL && ! IS_BRIDGE_RPVST_PLUS (br))
    {
      cli_out (cli, "%% Bridge %s is not configured as RPVST+\n", bridge_name);
      return CLI_ERROR;
    }

  if (br == NULL)
    {
      br_config_info = mstp_bridge_config_info_get(bridge_name);
      if (!br_config_info)
        {
          br_conf_list = mstp_bridge_config_list_new(bridge_name);
          if (br_conf_list == NULL)
            {
              cli_out(cli, "%% Could not allocate memory for bridge config\n");
              return CLI_ERROR;
            }
          mstp_bridge_config_list_link(br_conf_list);
          br_config_info = &(br_conf_list->br_info);
        }

      MSTP_GET_VLAN_INSTANCE_MAP(br_config_info->vlan_instance_map, vid, 
                                 instance);

      if (instance < 0)
        {
          cli_out(cli, "%% Bridge %s vlan is not configured\n",
                  bridge_name);
          return CLI_ERROR;
        }

      br_inst_info = mstp_br_config_find_instance(br_config_info, instance);
      if (!br_inst_info)
        {
          br_inst_list = mstp_br_config_instance_new(instance);
          if (br_inst_list == NULL)
            {
              cli_out(cli, "%% Could not allocate memory for bridge " 
                      "instance configuration\n");
              return CLI_ERROR;
            }

          mstp_br_config_link_instance_new(br_config_info, br_inst_list); 
          br_inst_info = &(br_inst_list->instance_info);
        }
      br_inst_info->priority = priority;
      SET_FLAG(br_inst_info->config, MSTP_BRIDGE_CONFIG_INSTANCE_PRIORITY);
      return CLI_SUCCESS;
    }

  if (! IS_BRIDGE_RPVST_PLUS(br))
    {
      cli_out(cli, "%% Bridge %s is not configured as RPVST+\n", 
          bridge_name);
      return CLI_ERROR;
    }

  ret = rpvst_plus_api_set_msti_bridge_priority (bridge_name, vid, priority);

  if (ret < 0)
  {
    switch (ret)
      {
        case MSTP_ERR_BRIDGE_NOT_FOUND :
          cli_out(cli, "%% Bridge not found\n");
          break;
        case MSTP_ERR_PRIORITY_VALUE_WRONG :
          cli_out(cli, "%% The priority value must be in multiples of 4096\n");
          break;
        case MSTP_ERR_PRIORITY_OUTOFBOUNDS :
          cli_out(cli, "%% The priority value is out of bounds\n");
          break;
        case MSTP_ERR_INSTANCE_OUTOFBOUNDS :
          cli_out(cli, "%% The instance value is out of bounds\n");
          break;
        case MSTP_ERR_INSTANCE_NOT_FOUND :
          cli_out(cli, "%% Bridge-Vlan not found\n");
          break;
        case MSTP_ERR_NOT_RPVST_BRIDGE :
          cli_out(cli, "%% Bridge is not configured as RPVST+\n");
          break;
        case MSTP_ERR_RPVST_BRIDGE_NO_VLAN:
          cli_out(cli, "%% Bridge %s vlan %d is not configured\n", bridge_name,
                  vid);
          break;
        case MSTP_ERR_RPVST_BRIDGE_VLAN_EXISTS:
          cli_out(cli, "%% Spanning-tree already enabled on this vlan\n");
          break;
        case MSTP_ERR_RPVST_BRIDGE_MAX_VLAN:
          cli_out(cli, "%% Spanning-tree enabled on 64 vlans\n");
          break;
        default :
          cli_out(cli, "%% Can't set priority\n");
          break;
      }

    return CLI_ERROR;
  }

  return CLI_SUCCESS;
}

ALI (bridge_vlan_priority,
     spanning_vlan_priority_cmd,
     "spanning-tree vlan <2-4094> priority <0-61440>",
     "Spanning Tree Commands",
     "Change priority for a particular vlan",
     "vlan id",
     "priority - bridge priority for the common instance",
     "bridge priority in increments of 4096 (Lower priority indicates greater likelihood of becoming root)");

CLI (no_bridge_vlan_priority,
     no_bridge_vlan_priority_cmd,
     "no bridge <1-32> vlan <2-4094> priority",
     CLI_NO_STR,
     "Bridge group commands",
     "Bridge Group name for bridging",
     "Change priority for a particular vlan",
     "instance id",
     "priority - Reset bridge priority to default for the vlan")
{
  int ret;
  int brno;
  u_int8_t *bridge_name = NULL;
  u_int16_t vid = 0;
  struct mstp_bridge *br = NULL;

  if (LAYER2_DEBUG(proto, CLI_ECHO))
    {
      if (argc == 2)
        zlog_debug (mstpm, "no bridge %s vlan %s priority",
                    argv[0], argv[1]);
      else
        zlog_debug (mstpm, "no spanning-tree vlan %s priority",
                    argv[0]);
    }

  MSTP_GET_BR_NAME(2, 0);

  if (argc == 2)
    CLI_GET_INTEGER_RANGE("vid", vid, argv[1], MSTP_VLAN_MIN, MSTP_VLAN_MAX);
  else
    CLI_GET_INTEGER_RANGE("vid", vid, argv[0], MSTP_VLAN_MIN, MSTP_VLAN_MAX);

  br = (struct mstp_bridge *)mstp_find_bridge(bridge_name);

  if ((br != NULL) && (! IS_BRIDGE_RPVST_PLUS (br)))
    {
      cli_out(cli, "%% Bridge %s is not configured as RPVST+\n", 
              bridge_name);
      return CLI_ERROR;
    }

  ret = rpvst_plus_api_set_msti_bridge_priority (bridge_name, vid, 
                                                 BRIDGE_DEFAULT_PRIORITY);

  if (ret < 0)
    switch (ret)
      {
      case MSTP_ERR_BRIDGE_NOT_FOUND :
        cli_out(cli, "%% Bridge not found\n");
        break;
      case MSTP_ERR_PRIORITY_VALUE_WRONG :
        cli_out(cli, "%% The priority value must be in multiples of 4096\n");
        break;
      case MSTP_ERR_PRIORITY_OUTOFBOUNDS :
        cli_out(cli, "%% The priority value is out of bounds\n");
        break;
      case MSTP_ERR_INSTANCE_OUTOFBOUNDS :
        cli_out(cli, "%% The instance value is out of bounds\n");
        break;
      case MSTP_ERR_INSTANCE_NOT_FOUND :
        cli_out(cli, "%%Bridge-instance not found\n");
        break;
      default :
        cli_out(cli, "%% Can't set priority\n");
        break;
      }

  return CLI_SUCCESS;

}

ALI (no_bridge_vlan_priority,
     no_spanning_vlan_priority_cmd,
     "no spanning-tree vlan <2-4094> priority",
     CLI_NO_STR,
     "Spanning Tree Commands" ,
     "Change priority for a particular vlan",
     "vlan id",
     "priority - Reset bridge priority to default for the vlan");

CLI (mstp_spanning_tree_vlan_restricted_tcn,
     mstp_spanning_tree_vlan_restricted_tcn_cmd,
     "spanning-tree vlan <2-4094> restricted-tcn",
     "spanning tree commands",
     "Set restrictions for the port of particular vlan",
     "vlan id",
     "restrict propagation of topology change notifications from port")
{
  struct mstp_bridge *br = NULL;
  struct interface *ifp = NULL;
  struct mstp_interface *mstpif = NULL;
  struct port_instance_info *pinst_info = NULL;
  struct mstp_bridge_info *br_config_info = NULL; 
  int instance = 0;
  int vid = 0;
  int ret = 0;

  ifp = cli->index; 
  if (!ifp)
    return CLI_ERROR;
  
  mstpif = ifp->info;
  if (!mstpif)
    return CLI_ERROR;
  
  CLI_GET_INTEGER_RANGE("vlan", vid, argv[0], MSTP_VLAN_MIN, MSTP_VLAN_MAX);

  br = mstp_find_bridge(ifp->bridge_name);

  if ((br != NULL) && (! IS_BRIDGE_RPVST_PLUS (br)))
    {
      cli_out(cli, "%% Bridge %s is not configured as RPVST+\n", 
              ifp->bridge_name);
      return CLI_ERROR;
    }

  if (if_lookup_by_name (&cli->vr->ifm, ifp->name) == NULL)
    {
      br_config_info = mstp_bridge_config_info_get (ifp->bridge_name);
      if (!br_config_info)
        {
           cli_out (cli, "%% Could not allocate memory for bridge config\n");
           return CLI_ERROR;
        }
          
      MSTP_GET_VLAN_INSTANCE_MAP(br_config_info->vlan_instance_map, vid, 
                                 instance);

      if (instance <= 0)
        {
          cli_out(cli, "%% Bridge %s vlan is not configured\n",
                  ifp->bridge_name);
          return CLI_ERROR;
        }

      pinst_info = mstpif_get_instance_info(mstpif, instance);
      if (!pinst_info)
        {
          cli_out(cli, "%% Bridge %s vlan %d is not configured\n",
                  ifp->bridge_name, vid);
          return CLI_ERROR;
        }
      pinst_info->restricted_tcn = PAL_TRUE;
      SET_FLAG(pinst_info->config, MSTP_IF_CONFIG_INSTANCE_RESTRICTED_TCN);
      return CLI_ERROR;
    }

  ret = rpvst_plus_api_set_msti_vlan_restricted_tcn(ifp->bridge_name, ifp->name,
                                                    vid, PAL_TRUE);

  if (ret < 0)
    {
      cli_out(cli, "%% Can't configure vlan restricted-tcn\n");
      return CLI_ERROR;
    }

  return CLI_SUCCESS;
}

CLI (no_mstp_spanning_tree_vlan_restricted_tcn,
     no_mstp_spanning_tree_vlan_restricted_tcn_cmd,
     "no spanning-tree vlan <2-4094> restricted_tcn",
     CLI_NO_STR,
     "spanning tree commands",
     "remove restrictions for the port of particular vlan",
     "vlan id",
     "remove restriction on propagation of topology change notifications from port")
{
  int ret = 0;
  struct mstp_bridge *br = NULL;
  struct interface *ifp = NULL;
  int vid = 0;

  ifp = cli->index;
  if (!ifp) 
    return CLI_ERROR;

  CLI_GET_INTEGER_RANGE("vlan", vid, argv[0], MSTP_VLAN_MIN, MSTP_VLAN_MAX);

  br = mstp_find_bridge(ifp->bridge_name);
  if ((br != NULL) && (! IS_BRIDGE_RPVST_PLUS (br)))
    {
      cli_out(cli, "%% Bridge %s is not configured as RPVST+\n", 
              ifp->bridge_name);
      return CLI_ERROR;
    }

  ret = rpvst_plus_api_set_msti_vlan_restricted_tcn(ifp->bridge_name, 
                                            ifp->name, vid, PAL_FALSE);
  if (ret < 0)
    {
      cli_out(cli, "%% Can't configure vlan restricted-tcn\n");
      return CLI_ERROR;
    }

  return CLI_SUCCESS;
}

CLI (mstp_spanning_tree_vlan_restricted_role,
     mstp_spanning_tree_vlan_restricted_role_cmd,
     "spanning-tree vlan <2-4094> restricted-role",
     "spanning tree commands",
     "Set restrictions for the port of particular vlan",
     "vlan id",
     "restrict the role of the port")
{
  int ret;
  struct mstp_bridge *br = NULL;
  struct interface *ifp = NULL;
  struct mstp_interface *mstpif = NULL;
  struct port_instance_info *pinst_info = NULL;
  struct mstp_bridge_info *br_config_info = NULL;
  int instance = 0;
  int vid = 0;

  ifp = cli->index;
  if (!ifp)
    return CLI_ERROR;

  mstpif = ifp->info;
  if (!mstpif)
    return CLI_ERROR;

  CLI_GET_INTEGER_RANGE("vlan", vid, argv[0], MSTP_VLAN_MIN, MSTP_VLAN_MAX);

  br = mstp_find_bridge(ifp->bridge_name);

  if ((br != NULL) && (! IS_BRIDGE_RPVST_PLUS (br)))
    {
      cli_out (cli, "%% Bridge %s is not configured as RPVST+\n", 
       ifp->bridge_name);
      return CLI_ERROR;
    }

  if (if_lookup_by_name (&cli->vr->ifm, ifp->name) == NULL || br == NULL)
    {
      br_config_info = mstp_bridge_config_info_get (ifp->bridge_name);
      if (!br_config_info)
        {
          cli_out (cli, "%% Could not allocate memory for bridge config\n");
          return CLI_ERROR;
        }

      MSTP_GET_VLAN_INSTANCE_MAP(br_config_info->vlan_instance_map, vid, 
                                 instance);

      if (instance <= 0)
        {
          cli_out(cli, "%% Bridge %s vlan is not configured\n",
                  ifp->bridge_name);
          return CLI_ERROR;
        }

      pinst_info = mstpif_get_instance_info(mstpif, instance);
      if (!pinst_info)
        {
          cli_out (cli, "%% Bridge %s vlan %d is not configured\n", 
                   ifp->bridge_name, vid);
          return CLI_ERROR;
        }

      pinst_info->restricted_role = PAL_TRUE;
      SET_FLAG(pinst_info->config, MSTP_IF_CONFIG_INSTANCE_RESTRICTED_ROLE);
      return CLI_ERROR;
    }

  ret = rpvst_plus_api_set_msti_vlan_restricted_role(ifp->bridge_name, 
                                             ifp->name, vid, PAL_TRUE);

  if (ret < 0)
    {
      cli_out(cli, "%% Can't configure vlan restricted-role\n");
      return CLI_ERROR;
    }

  return CLI_SUCCESS;
}

CLI (no_mstp_spanning_tree_vlan_restricted_role,
     no_mstp_spanning_tree_vlan_restricted_role_cmd,
     "no spanning-tree vlan <2-4094> restricted-role",
     CLI_NO_STR,
     "spanning tree commands",
     "remove restrictions for the port of particular vlan",
     "vlan id",
     "remove restriction on the role of the port")
{
  int ret = 0;
  struct mstp_bridge *br = NULL;
  struct interface *ifp = NULL;
  int vid = 0;

  ifp = cli->index;
  if (!ifp)
    return CLI_ERROR;

  CLI_GET_INTEGER_RANGE("vlan", vid, argv[0], MSTP_VLAN_MIN, MSTP_VLAN_MAX);

  br = mstp_find_bridge(ifp->bridge_name);

  if ((br != NULL) && (! IS_BRIDGE_RPVST_PLUS (br)))
    {
      cli_out(cli, "%% Bridge %s is not configured as RPVST+\n", 
               ifp->bridge_name);
      return CLI_ERROR;
    }

  ret = rpvst_plus_api_set_msti_vlan_restricted_role(ifp->bridge_name, 
                                                     ifp->name,
                                                     vid, PAL_FALSE);

  if (ret < 0)
    {
      cli_out(cli, "%% Can't configure vlan restricted-role\n");
      return CLI_ERROR;
    }

  return CLI_SUCCESS;
}

CLI (show_spanning_tree_rpvst,
     show_spanning_tree_rpvst_cmd,
     "show spanning-tree rpvst+",
     CLI_SHOW_STR,
     "spanning-tree Display spanning tree information",
     "Display RPVST+ information" )
{
  struct mstp_bridge *br = NULL;

  if (LAYER2_DEBUG(proto, CLI_ECHO))
    zlog_debug(mstpm, "show spanning-tree rpvst+");

  br = mstp_get_first_bridge();
  
  while(br)
    {
      if (IS_BRIDGE_RPVST_PLUS (br))
        {
          /* Print bridge info */
          mstp_display_cist_info(br, NULL, cli, PAL_FALSE);
          cli_out(cli, "%% \n");
          mstp_display_instance_info(br, NULL, cli);
        }

      br = br->next;
    }
  return CLI_SUCCESS;
}

CLI (show_spanning_tree_rpvst_interface,
     show_spanning_tree_rpvst_interface_cmd,
     "show spanning-tree rpvst+ interface IFNAME",
     CLI_SHOW_STR,
     "spanning-tree Display spanning tree information",
     "Display RPVST information",
     "Interface information",
     "Interface name")
{
  struct mstp_bridge *br = NULL;
  struct interface *ifp = NULL;

  if (LAYER2_DEBUG(proto, CLI_ECHO))
    zlog_debug(mstpm, "show spanning-tree rpvst+");

  ifp = if_lookup_by_name(&cli->vr->ifm, argv[0]);

  if ((ifp == NULL) || (ifp->port == NULL))
    {
      cli_out (cli, "%% Can't find interface %s\n", argv[0]);
      return CLI_ERROR;
    }

  br = mstp_find_bridge(ifp->bridge_name);
  if (br)
    {
      if (IS_BRIDGE_RPVST_PLUS (br))
        {
          /* Print bridge info */
          mstp_display_cist_info(br, ifp, cli, PAL_FALSE);
          cli_out (cli, "%% \n");
          mstp_display_instance_info(br, ifp, cli);
        }

      br = br->next;
    }
  return CLI_SUCCESS;
}

CLI (show_spanning_tree_rpvst_detail,
     show_spanning_tree_rpvst_detail_cmd,
     "show spanning-tree rpvst+ detail",
     CLI_SHOW_STR,
     "spanning-tree Display spanning tree information",
     "Display RPVST+ information",
     "Display detailed information" )
{
  struct mstp_bridge *br = NULL;

  if (LAYER2_DEBUG(proto, CLI_ECHO))
    zlog_debug(mstpm, "show spanning-tree rpvst+ detail");

  br = mstp_get_first_bridge();

  while(br)
    {
      if (IS_BRIDGE_RPVST_PLUS (br))
        mstp_display_instance_info_det(br, NULL, cli);
      br = br->next;
    }
  return CLI_SUCCESS;
}

CLI (show_spanning_tree_rpvst_detail_interface,
     show_spanning_tree_rpvst_detail_interface_cmd,
     "show spanning-tree rpvst+ detail interface IFNAME",
     CLI_SHOW_STR,
     "spanning-tree Display spanning tree information",
     "Display RPVST+ information",
     "Display detailed information",
     "Interface information",
     "Interface name")
{
  struct mstp_bridge *br = NULL;
  struct interface *ifp = NULL;

  if (LAYER2_DEBUG(proto, CLI_ECHO))
    zlog_debug(mstpm, "show spanning-tree rpvst+ detail interface IFNAME");

  ifp = if_lookup_by_name(&cli->vr->ifm, argv[0]);

  if ((ifp == NULL) || (ifp->port == NULL))
    {
      cli_out(cli, "%% Can't find interface %s\n", argv[0]);
      return CLI_ERROR;
    }

  br = mstp_find_bridge(ifp->bridge_name);
  if (br && (IS_BRIDGE_RPVST_PLUS (br)))
    mstp_display_instance_info_det(br, ifp, cli); 

  return CLI_SUCCESS;
} 


CLI (show_spanning_tree_rpvst_vlan,
     show_spanning_tree_rpvst_vlan_cmd,
     "show spanning-tree rpvst+ vlan <1-4094>",
     CLI_SHOW_STR,
     "spanning-tree Display spanning tree information",
     "Display RPVST+ information",
     "Display vlan information",
     "vlan-id")
{
  struct mstp_bridge *br = NULL;
  int instance = 0;
  u_int16_t vid = 0;

  if (LAYER2_DEBUG(proto, CLI_ECHO))
    zlog_debug(mstpm, "show spanning-tree rpvst+ vlan");

  CLI_GET_INTEGER_RANGE("vlan", vid, argv[0], MSTP_VLAN_DEFAULT_VID, 
                        MSTP_VLAN_MAX);

  br = mstp_get_first_bridge();

  while(br)
    {
      if (! IS_BRIDGE_RPVST_PLUS (br))
        {
          cli_out(cli, "%% bridge %s is not configured as RPVST+ bridge\n",
                  br->name);
          br = br->next;
          continue;
        }

     if (vid == MSTP_VLAN_DEFAULT_VID)
       instance = MST_INSTANCE_IST;
     else 
       MSTP_GET_VLAN_INSTANCE_MAP(br->vlan_instance_map, vid, instance);

     cli_out(cli, "%% vlan %d Instance %d configured \n", vid, instance); 
     if(instance < 0)
       {
          cli_out(cli, "%% bridge %s vlan %d not configured \n \n", 
                  br->name, vid);
          br = br->next;
          continue;
       }

      if (!br->instance_list[instance])
        {
          cli_out(cli, "%% bridge %s vlan %d not configured \n \n", 
                  br->name, vid);
          br = br->next;
          continue;
        }

      if (instance == MST_INSTANCE_IST)
        mstp_display_cist_info(br, NULL, cli, PAL_TRUE); 
      else
        mstp_display_single_instance_info_det(br->instance_list[instance], 
                                              NULL, cli);
      cli_out(cli, "%% \n");
      br = br->next;
    }
  return CLI_SUCCESS;
}

CLI (show_spanning_tree_rpvst_vlan_interface,
     show_spanning_tree_rpvst_vlan_interface_cmd,
     "show spanning-tree rpvst+ vlan <1-4094> interface IFNAME",
     CLI_SHOW_STR,
     "spanning-tree Display spanning tree information",
     "Display RPVST+ information",
     "Display vlan information",
     "instance_id",
     "Interface information",
     "Interface name")
{
  struct mstp_bridge *br = NULL;
  struct interface *ifp = NULL;
  int instance = 0;
  u_int16_t vid = 0;

  if (LAYER2_DEBUG(proto, CLI_ECHO))
    zlog_debug(mstpm, "show spanning-tree rpvst+ vlan"
               "INTERFACE IFNAME");

  CLI_GET_INTEGER_RANGE("vlan", vid, argv[0],
                         MSTP_VLAN_DEFAULT_VID, MSTP_VLAN_MAX);

  ifp = if_lookup_by_name (&cli->vr->ifm, argv[1]);

  if ((ifp == NULL) || (ifp->port == NULL))
    {
      cli_out(cli, "%% Can't find interface %s\n", argv[0]);
      return CLI_ERROR; 
    }

  br = mstp_find_bridge(ifp->bridge_name);
  if (br)
    {
      if (! IS_BRIDGE_RPVST_PLUS (br))
        {
          cli_out(cli, "%% bridge %s is not configured as RPVST bridge\n",
                  br->name);
          return CLI_SUCCESS;
        }

     if (vid == MSTP_VLAN_DEFAULT_VID)
       instance = MST_INSTANCE_IST;
     else
       MSTP_GET_VLAN_INSTANCE_MAP(br->vlan_instance_map, vid, instance);

     if(instance < 0)
       {
          cli_out(cli, "%% bridge %s vlan %d not configured \n \n", 
                         br->name, vid);
          return CLI_SUCCESS;
       }

      if (!br->instance_list[instance])
        {
          cli_out(cli, "%% bridge %s instance %d not configured \n \n",
                       br->name, instance);
          return CLI_SUCCESS;
        }

      if (instance == MST_INSTANCE_IST)
        mstp_display_cist_info(br, ifp, cli, PAL_TRUE);
      else
        mstp_display_single_instance_info_det(br->instance_list[instance],
                                              ifp, cli);
      cli_out (cli, "%% \n");
    }

  return CLI_SUCCESS;
}

CLI (show_spanning_tree_rpvst_config,
     show_spanning_tree_rpvst_config_cmd,
     "show spanning-tree rpvst+ config",
     CLI_SHOW_STR,
     "spanning-tree Display spanning tree information",
     "Display RPVST information",
     "Display Configuration information")
{
  struct mstp_bridge *br = NULL;
  
  if (LAYER2_DEBUG(proto, CLI_ECHO))
    zlog_debug (mstpm, "show spanning-tree rpvst+");

  br = mstp_get_first_bridge();

  while(br)
    {
      if (IS_BRIDGE_RPVST_PLUS (br))
        {
          int index;
          cli_out (cli, "%% \n");
          cli_out (cli, "%%  RPVST Configuration Information for bridge %s :\n",
                   br->name);   
          cli_out (cli, "%%------------------------------------------------------\n"); 
          /* Print bridge info */
          cli_out (cli, "%%  Format Id      : %d \n",
                   br->config.cfg_format_id);
          cli_out (cli, "%%  Name           : %s \n",br->config.cfg_name);
          cli_out (cli, "%%  Revision Level : %d \n",
                   br->config.cfg_revision_lvl);
          cli_out (cli, "%%  Digest         : 0x");
          for (index = 0; index < MSTP_CONFIG_DIGEST_LEN ; index++)
             {
               cli_out (cli, "%.2X",br->config.cfg_digest[index]);
             }
          cli_out (cli, "\n");
          cli_out (cli, "%%------------------------------------------------------\n"); 
        }
      br = br->next;
    }
      
  return CLI_SUCCESS;
}

/* Interface node's bridge configuration.  */
CLI (bridge_group_vlan,
     bridge_group_vlan_cmd,
     "bridge-group <1-32> vlan <2-4094>",
     BRIDGE_STR,
     BRIDGE_NAME_STR,
     "Rapid Pervlan Spanning tree instance",
     INSTANCE_ID_STR)
{
  int ret = 0;
  int brno;
  int instance = 0;
  u_int16_t vid = 0;
  u_int8_t *bridge_name;
  struct mstp_bridge *br = NULL;
  struct interface *ifp = NULL;
  struct mstp_interface *mstpif = NULL;
  struct port_instance_list *pinst_list = NULL;
  struct mstp_bridge_info *br_config_info = NULL;
  struct mstp_bridge_config_list *br_conf_list = NULL;

  if (LAYER2_DEBUG(proto, CLI_ECHO))
    {
      if (argc == 2)
        zlog_debug(mstpm, "bridge-group %s vlan %s", argv[0], argv[1]);
      else
        zlog_debug(mstpm, "spanning-tree vlan %s", argv[0]);
    }

  ifp = cli->index;
  if (!ifp)
    return CLI_ERROR;

  mstpif = ifp->info;
  if (!mstpif)
    return CLI_ERROR;

  MSTP_GET_BR_NAME(2, 0);

  if (argc == 2)
    CLI_GET_INTEGER_RANGE("vlan", vid, argv[1], MSTP_VLAN_MIN, MSTP_VLAN_MAX);
  else
    CLI_GET_INTEGER_RANGE("vlan", vid, argv[0], MSTP_VLAN_MIN, MSTP_VLAN_MAX);

  br = mstp_find_bridge(bridge_name);
  if (br != NULL && ! IS_BRIDGE_RPVST_PLUS (br))
    {
      cli_out (cli, "%% Bridge %s is not configured as RPVST+\n", bridge_name);
      return CLI_ERROR;
    }

  if (if_lookup_by_name (&cli->vr->ifm, ifp->name) == NULL || br == NULL)
    {
      br_config_info = mstp_bridge_config_info_get (bridge_name);
      if (!br_config_info)
        {
          br_conf_list = mstp_bridge_config_list_new (bridge_name);
          if (br_conf_list == NULL)
            {
              cli_out (cli, "%% Could not allocate memory for bridge config\n");
              return CLI_ERROR;
            }
          mstp_bridge_config_list_link (br_conf_list);
          br_config_info = &(br_conf_list->br_info);
        }

      MSTP_GET_VLAN_INSTANCE_MAP(br_config_info->vlan_instance_map, vid, 
                                 instance);

      if (instance <= 0)
        return CLI_ERROR;
      

      pinst_list = XCALLOC (MTYPE_PORT_INSTANCE_LIST,
                    sizeof (struct port_instance_list));
      if (pinst_list == NULL)
        {
          cli_out(cli, "Could not allocate memory to store vlan information\n");
          return CLI_ERROR;
        }

      pinst_list->instance_info.instance = instance;
      pinst_list->instance_info.vlan_id = vid;
      mstpif_config_add_instance(mstpif, pinst_list);
      mstp_br_config_vlan_add_port (bridge_name, ifp, vid); 
      pal_strcpy (mstpif->bridge_group, bridge_name);
      SET_FLAG (mstpif->config, MSTP_IF_CONFIG_BRIDGE_GROUP_INSTANCE);
      return CLI_ERROR;
    }

  ret =  rpvst_plus_api_add_port(bridge_name, ifp->name, vid, PAL_FALSE); 

  if (ret < 0)
    {
    switch (ret)
      {
        case MSTP_ERR_BRIDGE_NOT_FOUND :
          cli_out(cli, "%% Bridge not found\n");
          break;
        case MSTP_ERR_INSTANCE_OUTOFBOUNDS :
          cli_out(cli, "%% The instance value is out of bounds\n");
          break;
        case MSTP_ERR_INSTANCE_NOT_FOUND :
          cli_out(cli, "%% Bridge-instance not found\n");
          break;
        case MSTP_ERR_NOT_RPVST_BRIDGE:
          cli_out(cli, "%% Bridge is not configured as RPVST+\n");
          break;
        case MSTP_ERR_RPVST_VLAN_MEM_ERR:
          cli_out(cli, "%% Could not allocate memory for vlan\n");
          break;
        case MSTP_ERR_RPVST_VLAN_CONFIG_ERR:
          cli_out(cli, "%% Can't add vlan %d for bridge %s\n", vid, br->name);
          break;
        case MSTP_ERR_RPVST_VLAN_BR_GR_ASSOCIATE:
          cli_out(cli, "%% Associate the interface to the bridge\n");
          break;
        default :
          cli_out(cli, "%% Can't add port to bridge\n");
          break;
      }
     return CLI_ERROR;
    }
  /* store the bridge name for convineince */
  pal_strncpy(ifp->bridge_name, bridge_name, INTERFACE_NAMSIZ + 1);

  return CLI_SUCCESS;
}

ALI (bridge_group_vlan,
     spanning_vlanif_cmd,
     "spanning-tree vlan <2-4094>",
     SPANNING_STR,
     "Rapid Pervlan Spanning Tree Vlan Instance",
     "Vlan identifier");

CLI (no_bridge_group_vlan,
     no_bridge_group_vlan_cmd,
     "no bridge-group <1-32> vlan <2-4094>",
     CLI_NO_STR,
     "Bridge group commands",
     "Bridge Group name for bridging",
     "Rapid pervlan spanning tree vlan instance",
     "Vlan identifier")
{
  int ret = 0;
  int brno;
  u_int16_t vid = 0;
  u_int8_t *bridge_name = NULL;
  struct interface * ifp = NULL;
  struct mstp_bridge *br = NULL;

  if (LAYER2_DEBUG(proto, CLI_ECHO))
    {
      if (argc == 2)
        zlog_debug(mstpm, "no bridge-group %s vlan %s", argv[0], argv[1]);
      else
        zlog_debug(mstpm, "no spanning-tree vlan %s", argv[0]);
    }

  ifp = cli->index;
  if (!ifp)
    return CLI_ERROR;

  MSTP_GET_BR_NAME(2, 0);

  if (argc == 2)
    CLI_GET_INTEGER_RANGE("vlan", vid, argv[1], MSTP_VLAN_MIN, MSTP_VLAN_MAX);
  else
    CLI_GET_INTEGER_RANGE("vlan", vid, argv[0], MSTP_VLAN_MIN, MSTP_VLAN_MAX);

  br = mstp_find_bridge(bridge_name);

  if ((br != NULL) && (! IS_BRIDGE_RPVST_PLUS (br)))
    {
      cli_out(cli, "%% Bridge %s is not configured as RPVST+\n", 
              bridge_name);
      return CLI_ERROR;
    }

  ret = rpvst_plus_delete_port(bridge_name, ifp->name, vid,
                               PAL_FALSE, PAL_TRUE);
  if (ret < 0)
    {
      cli_out(cli, "%% Can't delete interface from bridge group\n");
      return CLI_ERROR;
    }

  return CLI_SUCCESS;
}

ALI (no_bridge_group_vlan,
     no_spanning_vlanif_cmd,
     "no spanning-tree vlan <2-4094>",
     CLI_NO_STR,
     "Spanning Tree Commands",
     "Rapid Pervlan Spanning Tree Instance",
     "Vlan identifier");

CLI (bridge_group_vlan_priority,
     bridge_group_vlan_priority_cmd,
     "bridge-group <1-32> vlan <2-4094> priority <0-240>",
     "Bridge group commands",
     "Bridge Group name for bridging",
     "Rapid pervlan spanning tree vlan instance",
     "Vlan identifier",
     "port priority for a bridge",
     "port priority in increments of 16 (lower priority indicates greater likelihood of becoming root)")
{
  struct interface *ifp = NULL;
  s_int16_t priority = 0;
  int instance = 0;
  u_int16_t vid = 0;
  int brno;
  u_int8_t *bridge_name = NULL;
  struct mstp_interface *mstpif = NULL;
  struct port_instance_info *pinst_info = NULL;
  struct mstp_bridge *br = NULL;
  int ret = 0;
  struct mstp_bridge_info *br_config_info = NULL;

  if (LAYER2_DEBUG(proto, CLI_ECHO))
    {
      if (argc == 3)
        zlog_debug(mstpm, "bridge-group %s vlan %s priority %s",
                   argv[0], argv[1], argv[2]);
      else
        zlog_debug(mstpm, "spanning-tree vlan %s priority %s",
                   argv[0], argv[1]);
    }

  ifp = cli->index;
  if (!ifp)
    return CLI_ERROR;

  mstpif = ifp->info;
  if (!mstpif)
    return CLI_ERROR;


  MSTP_GET_BR_NAME(3, 0);

  if (argc == 3)
    {
      CLI_GET_INTEGER_RANGE("vlan", vid, argv[1], MSTP_VLAN_MIN, MSTP_VLAN_MAX);
      CLI_GET_INTEGER_RANGE("priority", priority, argv[2], 0, 
                             MSTP_MAX_PORT_PRIORITY);
    }
  else
    {
      CLI_GET_INTEGER_RANGE("vlan", vid, argv[0], MSTP_VLAN_MIN, MSTP_VLAN_MAX);
      CLI_GET_INTEGER_RANGE("priority", priority, argv[1], 0, 
                            MSTP_MAX_PORT_PRIORITY);
    }

  br = mstp_find_bridge(bridge_name);

  if ((br != NULL) && ((! IS_BRIDGE_RPVST_PLUS (br))))
    {
      cli_out(cli, "%% Bridge %s is not configured as RPVST+\n", bridge_name);
      return CLI_ERROR;
    }

  if (if_lookup_by_name(&cli->vr->ifm, ifp->name) == NULL || br == NULL)
    {
      br_config_info = mstp_bridge_config_info_get (bridge_name);
      if (!br_config_info)
        {
          cli_out (cli, "%% Could not allocate memory for bridge config\n");
          return CLI_ERROR;
        }

      MSTP_GET_VLAN_INSTANCE_MAP(br_config_info->vlan_instance_map, vid, instance);

      if (instance <= 0)
        {
          return CLI_ERROR;
        }

      pal_strcpy(mstpif->bridge_group, bridge_name);
      pinst_info = mstpif_get_instance_info(mstpif, instance);
      if (!pinst_info)
        {
          cli_out(cli, "%% Bridge %s vlan %d is not configured\n",
                  br->name, vid);
          return CLI_ERROR;
        }
      pinst_info->port_instance_priority = priority; 
      SET_FLAG(pinst_info->config, MSTP_IF_CONFIG_BRIDGE_INSTANCE_PRIORITY);
      return CLI_ERROR;
    }

  ret = rpvst_plus_api_set_msti_port_priority(bridge_name, ifp->name,
                                              vid, priority);
  if (ret < 0)
    {
     switch (ret)
       {
        case MSTP_ERR_BRIDGE_NOT_FOUND :
          cli_out(cli, "%% Bridge not found\n");
          break;
        case MSTP_ERR_PRIORITY_VALUE_WRONG :
          cli_out(cli, "%% The priority value must be in multiples of 16\n");
          break;
        case MSTP_ERR_PRIORITY_OUTOFBOUNDS :
          cli_out(cli, "%% The priority value is out of bounds\n");
          break;
        case MSTP_ERR_PORT_NOT_FOUND :
          cli_out(cli, "%%Port not found\n");
          break;
        case MSTP_ERR_INSTANCE_NOT_FOUND :
          cli_out(cli, "%%Bridge-vlan instance not found\n");
          break;
        case MSTP_ERR_NOT_RPVST_BRIDGE:
          cli_out(cli, "%% Bridge is not configured as RPVST+\n");
          break;
        case MSTP_ERR_RPVST_VLAN_MEM_ERR:
          cli_out(cli, "%% Could not allocate memory for vlan\n");
          break;
        case MSTP_ERR_RPVST_VLAN_CONFIG_ERR:
          cli_out(cli, "%% Can't add vlan %d for bridge %s\n", vid, br->name);
          break;
        case MSTP_ERR_RPVST_VLAN_BR_GR_ASSOCIATE:
          cli_out(cli, "%% Associate the interface to the bridge\n");
          break;
        default:
          cli_out(cli, "%%Can't configure priority for port\n");
          break;
       }

     return CLI_ERROR;
     
    }

  return CLI_SUCCESS;
}

ALI (bridge_group_vlan_priority,
     spanning_port_vlan_priority_cmd,
     "spanning-tree vlan <2-4094> priority <0-240>",
     "Spanning Tree Commands",
     "rapid Pervlan spanning tree instance",
     "identifier",
     "port priority for a bridge",
     "port priority in increments of 16 (lower priority indicates greater likelihood of becoming root)");

CLI (no_bridge_group_vlan_priority,
     no_bridge_group_vlan_priority_cmd,
     "no bridge-group <1-32> vlan <2-4094> priority",
     CLI_NO_STR,
     "Bridge group commands",
     "Bridge Group name for bridging",
     "Rapid Pervlan Spanning Tree Instance",
     "identifier",
     "port priority for a bridge")
{
  int ret;
  struct interface *ifp = NULL;
  u_int8_t *bridge_name = NULL;
  u_int16_t vid = 0;
  int brno;
  struct mstp_bridge *br = NULL;

  if (LAYER2_DEBUG(proto, CLI_ECHO))
     {
      if (argc == 2)
        {
          zlog_debug(mstpm, "no bridge-group %s vlan %s priority",
                     argv[0], argv[1]);
        }
      else
        {
          zlog_debug(mstpm, "no spanning-tree vlan %s priority", argv[0]);
        }
    }

  ifp = cli->index;
  if (!ifp)
    return CLI_ERROR;

  MSTP_GET_BR_NAME(2, 0);

  if (argc == 2)
    CLI_GET_INTEGER_RANGE("vlan", vid, argv[1], MSTP_VLAN_MIN, MSTP_VLAN_MAX);
  else
    CLI_GET_INTEGER_RANGE("vlan", vid, argv[0], MSTP_VLAN_MIN, MSTP_VLAN_MAX);
 
   br = mstp_find_bridge(bridge_name);

  if ((br != NULL) && (! IS_BRIDGE_RPVST_PLUS (br)))
    {
      cli_out(cli, "%% Bridge %s is not configured as RPVST+\n", bridge_name);
      return CLI_ERROR;
    }

  ret = rpvst_plus_api_set_msti_port_priority(bridge_name, ifp->name, vid,
                                              BRIDGE_DEFAULT_PORT_PRIORITY);
  if (ret < 0)
    {
      cli_out(cli, "%% Can't configure priority for port %s\n", ifp->name);
      return CLI_ERROR;
    }
  return CLI_SUCCESS;
}

ALI (no_bridge_group_vlan_priority,
     no_spanning_port_vlan_priority_cmd,
     "no spanning-tree vlan <2-4094> priority",
     CLI_NO_STR,
     "Spanning Tree Commands",
     "Rapid Pervlan Spanning Tree vlan",
     "identifier",
     "Port priority for bridge");

CLI (bridge_group_vlan_path_cost,
     bridge_group_vlan_path_cost_cmd,
     "bridge-group <1-32> vlan <2-4094> path-cost <1-200000000>",
     "Bridge group commands",
     "Bridge Group name for bridging",
     "rapid Pervlan Spanning Tree Instance",
     "vlan identifier",
     "path cost for a port",
     "path cost in range <1-200000000> (lower path cost indicates greater likelihood of becoming root)")
{
  int ret;
  u_int8_t *bridge_name;
  struct interface *ifp = NULL;
  u_int32_t path_cost = 0;
  u_int16_t vid = 0;
  struct mstp_interface *mstpif = NULL;
  struct port_instance_info *pinst_info = NULL;
  int brno;
  struct mstp_bridge *br = NULL;
  int instance = 0;
  struct mstp_bridge_info *br_config_info = NULL;
  struct mstp_bridge_config_list *br_conf_list = NULL;

  if (LAYER2_DEBUG(proto, CLI_ECHO))
    {
      if (argc == 3)
        zlog_debug(mstpm, "bridge-group %s vlan %s path-cost %s",
                   argv[0], argv[1], argv[2]);
      else
        zlog_debug(mstpm, "spanning-tree vlan %s path-cost %s",
                   argv[0], argv[1]);
    }

  ifp = cli->index;
  if (!ifp)
    return CLI_ERROR;

  mstpif = ifp->info;
  if (!mstpif)
    return CLI_ERROR;

  MSTP_GET_BR_NAME (3, 0);

  if (argc == 3 )
    {
      CLI_GET_INTEGER_RANGE("vlan", vid, argv[1], MSTP_VLAN_MIN, MSTP_VLAN_MAX);
      CLI_GET_INTEGER_RANGE("path-cost", path_cost, argv[2], 1, 
                            MSTP_MAX_PATH_COST);
    }
  else
    {
      CLI_GET_INTEGER_RANGE("vlan", vid, argv[0], MSTP_VLAN_MIN, MSTP_VLAN_MAX);
      CLI_GET_INTEGER_RANGE("path-cost", path_cost, argv[1], 1, 
                            MSTP_MAX_PATH_COST);
    }

  br = mstp_find_bridge(bridge_name);

  if ((br != NULL) && (! IS_BRIDGE_RPVST_PLUS (br)))
    {
      cli_out(cli, "%% Bridge %s is not configured as RPVST+\n", bridge_name);
      return CLI_ERROR;
    }

  if (if_lookup_by_name(&cli->vr->ifm, ifp->name) == NULL || br == NULL)
    {
      br_config_info = mstp_bridge_config_info_get(bridge_name);
      if (!br_config_info)
        {
          br_conf_list = mstp_bridge_config_list_new(bridge_name);
          if (br_conf_list == NULL)
            {
              cli_out(cli, "%% Could not allocate memory for bridge config\n");
              return CLI_ERROR;
            }
          mstp_bridge_config_list_link(br_conf_list);
          br_config_info = &(br_conf_list->br_info);
        }

      MSTP_GET_VLAN_INSTANCE_MAP(br_config_info->vlan_instance_map, vid,
                                 instance);

      if (instance <= 0)
        {
          cli_out(cli, "%% bridge %s vlan %d not configured \n \n", 
                  br->name, vid);
          return CLI_ERROR;
        }
      pal_strcpy(mstpif->bridge_group, bridge_name);
      pinst_info = mstpif_get_instance_info(mstpif, instance);
      if (!pinst_info)
        {
          cli_out(cli, "%% Bridge %s vlan %d is not configured\n",
                  br->name, vid);
          return CLI_ERROR;
        }
      pinst_info->port_instance_pathcost = path_cost;
      SET_FLAG (pinst_info->config, MSTP_IF_CONFIG_BRIDGE_INSTANCE_PATHCOST);
      return CLI_ERROR;
    }

  ret = rpvst_plus_api_set_msti_port_path_cost(bridge_name, ifp->name, 
                                               vid, path_cost);

  if (ret < 0)
    {
      switch (ret)
        {
          case MSTP_ERR_BRIDGE_NOT_FOUND :
            cli_out(cli, "%% Bridge not found\n");
            break;
          case MSTP_ERR_PRIORITY_VALUE_WRONG :
            cli_out(cli, "%% The priority value must be in "
                    "multiples of 16\n");
            break;
          case MSTP_ERR_PRIORITY_OUTOFBOUNDS :
            cli_out(cli, "%% The priority value is out of bounds\n");
            break;
          case MSTP_ERR_PORT_NOT_FOUND :
            cli_out(cli, "%%Port not found\n");
            break;
          case MSTP_ERR_INSTANCE_NOT_FOUND :
            cli_out(cli, "%%Bridge-vlan instance not found\n");
            break;
          case MSTP_ERR_NOT_RPVST_BRIDGE:
            cli_out(cli, "%% Bridge is not configured as RPVST+\n");
            break;
          case MSTP_ERR_RPVST_VLAN_MEM_ERR:
            cli_out(cli, "%% Could not allocate memory for vlan\n");
            break;
          case MSTP_ERR_RPVST_VLAN_CONFIG_ERR:
            cli_out(cli, "%% Can't configure path-cost for vlan %d\n", vid);
            break;
          case MSTP_ERR_RPVST_VLAN_BR_GR_ASSOCIATE:
            cli_out(cli, "%% Associate the interface to the bridge\n");
            break;
          default:
            cli_out(cli, "%%Can't configure path-cost on bridge %s vlan %d\n",
                    bridge_name, vid);
            break;
        }
      return CLI_ERROR;

    }

  return CLI_SUCCESS;
}

ALI (bridge_group_vlan_path_cost,
     spanning_vlan_path_cost_cmd,
     "spanning-tree vlan <2-4094> path-cost <1-200000000>",
     "Spanning Tree Commands",
     "rapid Pervlan Spanning Tree Instance",
     "identifier",
     "path cost for a port",
     "path cost in range <1-200000000>"
     "(lower path cost indicates greater likelihood of becoming root)");

CLI (no_bridge_group_vlan_path_cost,
     no_bridge_group_vlan_path_cost_cmd,
     "no bridge-group <1-32> vlan <2-4094> path-cost",
     CLI_NO_STR,
     "Bridge group commands",
     "Bridge Group name for bridging",
     "Rapid Pervlan Spanning Tree Instance",
     "identifier",
     "path-cost - path cost for a port")
{
  int ret;
  struct interface *ifp = NULL;
  u_int8_t *bridge_name = NULL;
  u_int16_t vid = 0;
  int brno;
  struct mstp_bridge *br = NULL;

  if (LAYER2_DEBUG(proto, CLI_ECHO))
     {
      if (argc == 2)
        {
          zlog_debug(mstpm, "no bridge-group %s vlan %s path-cost",
                     argv[0], argv[1]);
        }
      else
        {
          zlog_debug(mstpm, "no spanning-tree vlan %s path-cost", argv[0]);
        }
    }

  ifp = cli->index;
  if (!ifp)
    return CLI_ERROR;

  MSTP_GET_BR_NAME(2, 0);

  if (argc == 2)
    CLI_GET_INTEGER_RANGE("vlan", vid, argv[1], MSTP_VLAN_MIN, MSTP_VLAN_MAX);
  else
    CLI_GET_INTEGER_RANGE("vlan", vid, argv[0], MSTP_VLAN_MIN, MSTP_VLAN_MAX);
 
   br = mstp_find_bridge(bridge_name);

  if ((br != NULL) && (! IS_BRIDGE_RPVST_PLUS (br)))
    {
      cli_out(cli, "%% Bridge %s is not configured as RPVST+\n", bridge_name);
      return CLI_ERROR;
    }

  ret = rpvst_plus_api_set_msti_port_path_cost(bridge_name, ifp->name, vid,
                                               mstp_nsm_get_port_path_cost(mstpm,
                                                                 ifp->ifindex));
  if (ret < 0)
    {
      cli_out(cli, "%% Can't configure path cost for port %s\n", ifp->name);
      return CLI_ERROR;
    }
  return CLI_SUCCESS;
}

ALI (no_bridge_group_vlan_path_cost,
     no_spanning_vlan_path_cost_cmd,
     "no spanning-tree vlan <2-4094> path-cost",
     CLI_NO_STR,
     "Spanning Tree Commands",
     "Rapid Pervlan Spanning Tree Instance",
     "identifier",
     "path-cost - path cost for a port");

CLI (bridge_vlan,
     bridge_vlan_cmd,
     "bridge <1-32> vlan <2-4094>",
     "Bridge group commands",
     "Bridge Group name for bridging",
     "Vlan",
     "Vlan Id")
{
  int brno;
  s_int32_t vid;
  u_int8_t *bridge_name = NULL;
  struct mstp_bridge *br = NULL;
  int ret = 0;

  if (LAYER2_DEBUG(proto, CLI_ECHO))
    {
      if (argc == 2)
        zlog_debug(mstpm, "bridge %s vlan %s", argv[0],argv[1]);
      else
        zlog_debug(mstpm, "vlan %s", argv[0]);
    }

  MSTP_GET_BR_NAME(2, 0);

  if (argc == 2)
    CLI_GET_INTEGER_RANGE("vlan", vid, argv[1], MSTP_VLAN_MIN, MSTP_VLAN_MAX);
  else
    CLI_GET_INTEGER_RANGE("vlan", vid, argv[0], MSTP_VLAN_MIN, MSTP_VLAN_MAX);

  br = mstp_find_bridge (bridge_name);

  if ((br != NULL) && (! IS_BRIDGE_RPVST_PLUS(br)))
    {
      cli_out (cli, "%% Bridge %s is not configured as RPVST+\n", bridge_name);
      return CLI_ERROR;
    } 

  if (br == NULL)
    {
      rpvst_plus_bridge_vlan_config_add(bridge_name, vid);
      cli_out (cli, "Can not put vlan in spanning-tree\n");
      return CLI_ERROR;
    }

  ret = rpvst_plus_api_add_vlan(bridge_name, vid);

  if (ret < 0)
    {
      switch (ret)
        {
	  case MSTP_ERR_BRIDGE_NOT_FOUND :
	      cli_out(cli, "%% Bridge not found\n");
	      break;
	  case MSTP_ERR_PRIORITY_VALUE_WRONG :
	      cli_out(cli, "%% The priority value must be in multiples of 4096\n");
	      break;
	  case MSTP_ERR_PRIORITY_OUTOFBOUNDS :
	      cli_out(cli, "%% The priority value is out of bounds\n");
	      break;
	  case MSTP_ERR_INSTANCE_OUTOFBOUNDS :
	      cli_out(cli, "%% The instance value is out of bounds\n");
	      break;
	  case MSTP_ERR_INSTANCE_NOT_FOUND :
	      cli_out(cli, "%% Bridge-Vlan not found\n");
	      break;
	  case MSTP_ERR_NOT_RPVST_BRIDGE :
	      cli_out(cli, "%% Bridge is not configured as RPVST+\n");
	      break;
	  case MSTP_ERR_RPVST_BRIDGE_NO_VLAN:
	      cli_out(cli, "%% Bridge %s vlan %d is not configured\n", 
		      bridge_name,vid);
	      break;
	  case MSTP_ERR_RPVST_BRIDGE_VLAN_EXISTS:
	      cli_out(cli, "%% Spanning-tree already enabled on this vlan\n");
	      break;
	  case MSTP_ERR_RPVST_BRIDGE_MAX_VLAN:
	      cli_out(cli, "%% Spanning-tree enabled on 64 vlans\n");
	      break;
	  default :
	      cli_out(cli, "%% Can't configure vlan %d\n", vid);
	      break;
        }
      return CLI_ERROR;
    }

  return CLI_SUCCESS;
}

ALI (bridge_vlan,
     spanning_vlan_cmd,
     "vlan <2-4094>",
     "Vlan",
     "Vlan ID");

CLI (no_bridge_vlan,
     no_bridge_vlan_cmd,
     "no bridge <1-32> vlan <2-4094>",
     CLI_NO_STR,
     "Bridge group commands",
     "Bridge Group name for bridging",
     "Vlan",
     "Vlan Id")
{
  int brno;
  u_int16_t vid = 0; 
  u_int8_t *bridge_name = NULL;
  struct mstp_bridge *br = NULL;
  int ret = 0;

  if (LAYER2_DEBUG(proto, CLI_ECHO))
    {
      if (argc == 2)
        zlog_debug(mstpm, "no bridge %s vlan %s", argv[0],argv[1]);
      else
        zlog_debug(mstpm, "no vlan %s", argv[0]);
    }

  MSTP_GET_BR_NAME(2, 0);

  if (argc == 2)
    CLI_GET_INTEGER_RANGE("vlan", vid, argv[1], 1, 4094);
  else
    CLI_GET_INTEGER_RANGE("vlan", vid, argv[0], 1, 4094);

  br = mstp_find_bridge (bridge_name);
  
   if (! IS_BRIDGE_RPVST_PLUS (br))
    {
      cli_out (cli, "%% Bridge %s is not configured as RPVST+\n", bridge_name);
      return CLI_ERROR;
    }

   ret = rpvst_plus_api_vlan_delete (bridge_name, vid);
   if (ret < 0)
    {
      cli_out (cli, "Cannot delete vlan %d from bridge %s \n", 
               vid, bridge_name);
      return CLI_ERROR;
    }

  /* Delete configuration stored */
  mstp_bridge_vlan_config_delete(bridge_name, vid);

  return CLI_SUCCESS;

}

ALI (no_bridge_vlan,
     no_spanning_vlan_cmd,
     "no vlan <2-4094>",
     CLI_NO_STR,
     "Vlan",
     "Vlan Id");

#endif /* HAVE_RPVST_PLUS */

/* These data structures are shared among all of the "show/write routines. */
/* Therefore, none of those routines are re-entrant nor are they thread-safe. */

static char * portStateStr [] = {
  "Discarding",
  "Listening",
  "Learning",
  "Forwarding",
  "Blocked",
  "Error"
};

static char * stpPortStateStr [] = {
  "Blocked",
  "Listening",
  "Learning",
  "Forwarding",
  "Blocked",
  "Discard/Blocking",
  "Error"
};

static char * portRoleStr [] = {
  "Masterport",
  "Alternate",
  "Rootport",
  "Designated",
  "Disabled",
  "Backup"
};


static char *addtype_str [] = {
  "Explicit",
  "Implicit"
};


static char *boolean_str [] = {
  "Disabled",
  "Enabled"
};

static char *version_str [] = {
  "Spanning Tree Protocol ",
  "Unsupported",
  "Rapid Spanning Tree Protocol",
  "Multiple Spanning Tree Protocol"
};

static char * portfastStr [] = {
  "enabled",
  "disabled",
  "default"
};

static char * mstp_port_get_cist_port_state (struct mstp_port *port)
{
  /* If it a port in listening state with root guard enabled then port state
   * should be shown as root-inconsistent.
   */
  if ((port->cist_state == STATE_DISCARDING) && (port->oper_rootguard))
    return "root-inconsistent";

  if (port->br->type == NSM_BRIDGE_TYPE_STP
      || port->br->type == NSM_BRIDGE_TYPE_STP_VLANAWARE)
    {
      if (port->cist_role == ROLE_DISABLED)
        return "Disabled";
      else if (port->cist_role == ROLE_BACKUP
               || port->cist_role == ROLE_ALTERNATE)
        return "Blocked";
      else if ((port->cist_state == STATE_DISCARDING)
               && (port->cist_role == ROLE_DESIGNATED
                   || port->cist_role == ROLE_ROOTPORT))
          return "Listening";
      else
        return stpPortStateStr[port->cist_state];
    }
  else
    return portStateStr[port->cist_state];
}

/* CLI Show routines */

CLI (show_spanning_tree,
     show_spanning_tree_cmd,
     "show spanning-tree",
     CLI_SHOW_STR,
     "Display spanning-tree information")
{
  struct mstp_bridge * br;

  if (LAYER2_DEBUG(proto, CLI_ECHO))
    zlog_debug (mstpm, "show spanning-tree mst");
  br = mstp_get_first_bridge();

  while(br)
    {
      if (IS_BRIDGE_STP (br))
        {
          /* Print bridge info */
          mstp_display_stp_info (br, NULL, cli, PAL_TRUE);
          cli_out (cli, "%% \n");
        }
      else
        {
          /* Print bridge info */
          mstp_display_cist_info (br, NULL, cli, PAL_TRUE);
          cli_out (cli, "%% \n");

        }
      br = br->next;
    }
  return CLI_SUCCESS;
}

/*show spanning-tree statistics */
CLI (show_spanning_tree_stats,
     show_spanning_tree_stats_cmd,
     "show spanning-tree statistics (interface IFNAME| "
                    "(instance <1-63>| vlan <1-4094>)) bridge <1-32> ",
     CLI_SHOW_STR,
     "Display Spanning-tree Information",
     "Statistics of the BPDUs",
     "Interface",
     CLI_IFNAME_STR,
     "Display Instance Information",
     "Instance ID",
     "Vlan",
     "VLAN ID Associated with the Instance",
     "Bridge",  
     "Bridge ID")
{
  u_int8_t *bridge_name = NULL;
  u_int32_t brno = 0;
  u_int16_t vid = 0;
  u_int32_t instance = 0;
  int ret = 0;
  struct mstp_bridge *br = NULL;
  struct interface *ifp = NULL;

  /* bridge <no> */
  if (argc == 1)
    {
      CLI_GET_INTEGER_RANGE ("Max Bridge groups", brno, argv[0], 1, 32);
      bridge_name = argv[0];
    }
  else if (argc > 1)
    {

      if (pal_strcmp (argv[0], "interface") == 0)
        {
          ifp = if_lookup_by_name(&cli->vr->ifm, argv[1]);
          if ((ifp == NULL) || (ifp->port == NULL))
            {
              cli_out (cli, "%% Can't find interface %s\n", argv[1]);
              return CLI_ERROR;
            }

          /* interface <name> instance <no> bridge <no>*/
          else if (pal_strcmp (argv[2], "instance") == 0)
            {
              CLI_GET_INTEGER_RANGE ("instance", instance, argv[3], 1, 63);            
              CLI_GET_INTEGER_RANGE ("Max Bridge groups", brno, argv[4], 1, 32);
              bridge_name = argv[4];            
            }   
          /* interface <name> vlan <no> bridge <no>*/
          else if (pal_strcmp (argv[2], "vlan") == 0)         
            {
              CLI_GET_INTEGER_RANGE ("vid range", vid, argv[3], 1, 4094);            
              CLI_GET_INTEGER_RANGE ("Max Bridge groups", brno, argv[4], 1, 32);
              bridge_name = argv[4];
            }
          /* interface <name> bridge <no> */
          else
            {
              CLI_GET_INTEGER_RANGE ("Max Bridge groups", brno, argv[2], 1, 32);               
              bridge_name = argv[2];
            }         
        }
      /* instance <no> bridge <no> */
      else if (pal_strcmp (argv[0], "instance") == 0)      
        { 
          CLI_GET_INTEGER_RANGE ("instance", instance, argv[1], 1, 63);            
          CLI_GET_INTEGER_RANGE ("Max Bridge groups", brno, argv[2], 1, 32);
          bridge_name = argv[2];
        }
      /* vlan <no> bridge <no> */
      else if (pal_strcmp (argv[0], "vlan") == 0)
        {
          CLI_GET_INTEGER_RANGE ("vid range", vid, argv[1], 1, 4094);            
          CLI_GET_INTEGER_RANGE ("Max Bridge groups", brno, argv[2], 1, 32);          
          bridge_name = argv[2];
        }  
    }

  if (bridge_name != NULL)
    {
      br = mstp_find_bridge (bridge_name);
      if (br == NULL)
        {
          cli_out(cli,"%% Bridge %s is does not exists",bridge_name);
          return CLI_ERROR;
        }
    }
  else
    return CLI_ERROR;

  if (! IS_BRIDGE_MSTP (br) && instance > 0) 
    {
      cli_out (cli, "%% Bridge %s is not configured as MSTP\n", 
          bridge_name);
      return CLI_ERROR;
    }
  
  if (! IS_BRIDGE_RPVST_PLUS (br) && vid > 0)
    {
      cli_out (cli, "%% Bridge %s is not configured as RPVST+\n", 
          bridge_name);
      return CLI_ERROR;
    }

  switch (br->type)
    {
    case NSM_BRIDGE_TYPE_STP :
      case NSM_BRIDGE_TYPE_STP_VLANAWARE:
        if (!IS_BRIDGE_STP(br))
          {
            cli_out (cli, "%% bridge %s is not a STP bridge\n", br->name);
            return CLI_ERROR;
          }
        /* Print bridge info */
        ret = mstp_stats_display_info (br, ifp, cli);
        if (ret < 0 )
          {
            cli_out (cli, "%% No Ports are associated with  bridge");
            return CLI_ERROR;
          }
        break;

      case NSM_BRIDGE_TYPE_RSTP :
      case NSM_BRIDGE_TYPE_RSTP_VLANAWARE:
          {
            if (!IS_BRIDGE_RSTP(br))
              {
                cli_out (cli, "%% bridge %s is not a RSTP/STP bridge\n",
                         br->name);
                return CLI_ERROR;
              }

            /* Print bridge info */
            ret = mstp_stats_display_info (br, ifp, cli);
            if (ret < 0 )
              {
                cli_out (cli, "%% No Ports are associated with  bridge");
                return CLI_ERROR;
              }
            break;
          }
#ifdef HAVE_RPVST_PLUS
      case NSM_BRIDGE_TYPE_RPVST_PLUS :
          {
            if (!IS_BRIDGE_RPVST_PLUS(br))
              cli_out (cli, "%% bridge %s is not configured as RPVST bridge \n",
                       br->name);
            
            if (vid <= 0)
              {
                ret = mstp_stats_display_info (br, ifp, cli);
                if (ret < 0)
                  return CLI_ERROR;
                
                ret = mstp_stats_display_inst_info (br, ifp, cli, instance);
                if (ret < 0)
                  return CLI_ERROR;

                cli_out (cli, "%% \n");
             }       
            else if (vid > 0)
              {
                MSTP_GET_VLAN_INSTANCE_MAP (br->vlan_instance_map, vid,
                                            instance);

                cli_out (cli, "\n");

                if (instance <= 0)
                  {
                    ret = MSTP_ERR_RPVST_BRIDGE_NO_VLAN;
                    cli_out (cli, "%% VLAN does not exist for "
                             "RPVST+ bridge %s\n", br->name);
                    return CLI_ERROR;
                  }

                if (!br->instance_list[instance])
                  {
                    cli_out(cli, "%% bridge %s instance %d not configured \n\n",
                            br->name, instance);
                    return CLI_ERROR;
                  }
             
                if (instance > 0 && vid > 0)
                  {
                    ret = mstp_stats_display_single_inst_info(br, ifp, cli, instance);
                    if (ret < 0)
                     return CLI_ERROR;
                  }

              }
           break;
          }
#endif /* HAVE_RPVST_PLUS */
      case NSM_BRIDGE_TYPE_MSTP:
          {
            if (! IS_BRIDGE_MSTP (br))
              {
                cli_out (cli, "%% bridge %s is not a MSTP bridge\n", br->name);
                return CLI_ERROR;
              }
            if (!br->instance_list[instance] && instance != 0)
              {
                cli_out (cli, "%% bridge %s instance %d not configured\n \n", 
                         br->name, instance);
                return CLI_ERROR;
              }

            if (instance > 0)
              {
                ret = mstp_stats_display_single_inst_info(br, ifp, cli, 
                                                          instance);
              }
            else
              {
                ret = mstp_stats_display_info (br, ifp, cli);
                if (ret < 0)
                  return CLI_ERROR;
                ret = mstp_stats_display_inst_info (br, ifp, cli, instance);
                if (ret < 0)
                  return CLI_ERROR;
              }
            break;

          }
      }

    cli_out (cli, "%% \n");
    return CLI_SUCCESS;
}

ALI (show_spanning_tree_stats,
     show_spanning_tree_stats_bridge_cmd,
     "show spanning-tree statistics bridge <1-32>",
     CLI_SHOW_STR,
     "Display Spanning-tree Information",
     "Statistics of the BPDUs",
     "Bridge",
     "Bridge ID");

ALI (show_spanning_tree_stats,
     show_spanning_tree_stats_interface_bridge_cmd,
     "show spanning-tree statistics interface IFNAME (instance <1-63>| vlan <2-4094>) bridge <1-32>",
     CLI_SHOW_STR,
     "Display Spanning-tree Information",
     "Statistics of the BPDUs",
     "Interface",
     CLI_IFNAME_STR,
     "Display Instance Information",
     "Instance ID",
     "Vlan",
     "VLAN ID Associated with the Instance",
     "Bridge",
     "Bridge ID");

CLI (mstp_clear_spanning_tree_interface,
    mstp_clear_spanning_tree_interface_cmd,
    "clear spanning-tree statistics (interface IFNAME| (instance <1-63>| vlan <2-4094>)) bridge <1-32>",
    CLI_CLEAR_STR,
    "Spanning-tree Information",
    "Statistics of the BPDUs",
    "Interface",
    CLI_IFNAME_STR,
    "Instance Information",
    "Instance ID",
    "Vlan",
    "VLAN ID Associated with the Instance",
    "Bridge",
    "Bridge ID")
{
    u_int32_t brno = 0;
    u_int32_t instance = 0;
    u_int16_t vid = 0;
    int ret = 0;
    u_int8_t *bridge_name = NULL;
    struct mstp_bridge *br = NULL;
    struct interface *ifp  = NULL;
    /* bridge <no> */
    if (argc == 1)
      {
        CLI_GET_INTEGER_RANGE ("Max Bridge groups", brno, argv[0], 1, 32);
        bridge_name = argv[0];
      }
    else if (argc > 1)
      {
        if (pal_strcmp (argv[0], "interface") == 0)
          {
            ifp = if_lookup_by_name(&cli->vr->ifm, argv[1]);
            if ((ifp == NULL) || (ifp->port == NULL))
              {
                cli_out (cli, "%% Can't find interface %s\n", argv[1]);
                return CLI_ERROR;
              }
            /* interface <name> instance <no> bridge <no>*/
            else if (pal_strcmp (argv[2], "instance") == 0)
              {
                CLI_GET_INTEGER_RANGE ("instance", instance, argv[3], 1, 63);
                CLI_GET_INTEGER_RANGE ("Max Bridge groups", brno, argv[4], 1, 32);
                bridge_name = argv[4];
              }
            /* interface <name> vlan <no> bridge <no>*/
            else if (pal_strcmp (argv[2], "vlan") == 0)
              {
                CLI_GET_INTEGER_RANGE ("vid range", vid, argv[3], 1, 4094);
                CLI_GET_INTEGER_RANGE ("Max Bridge groups", brno, argv[4], 1, 32);
                bridge_name = argv[4];
              }
            /* interface <name> bridge <no> */
            else
              {
                CLI_GET_INTEGER_RANGE ("Max Bridge groups", brno, argv[2], 1, 32);
                bridge_name = argv[2];
              }
          }
        /* instance <no> bridge <no> */
        else if (pal_strcmp (argv[0], "instance") == 0)
          {
            CLI_GET_INTEGER_RANGE ("instance", instance, argv[1], 1, 63);
            CLI_GET_INTEGER_RANGE ("Max Bridge groups", brno, argv[2], 1, 32);
            bridge_name = argv[2];
          }
        /* vlan <no> bridge <no> */
        else if (pal_strcmp (argv[0], "vlan") == 0)
          {
            CLI_GET_INTEGER_RANGE ("vid range", vid, argv[1], 1, 4094);
            CLI_GET_INTEGER_RANGE ("Max Bridge groups", brno, argv[2], 1, 32);
            bridge_name = argv[2];
          }
      }
    
    if (bridge_name != NULL)
      {
        br = (struct mstp_bridge *)mstp_find_bridge (bridge_name);
        if (br == NULL)
          {
            cli_out (cli, "%% bridge does not exist\n");
            return CLI_ERROR;
          }
      }
    else
      return CLI_ERROR;

    if (! IS_BRIDGE_MSTP (br) && instance > 0) 
      {
        cli_out (cli, "%% Bridge %s is not configured as MSTP\n", 
            bridge_name);
        return CLI_ERROR;
      }

    if (! IS_BRIDGE_RPVST_PLUS (br) && vid > 0)
      {
        cli_out (cli, "%% Bridge %s is not configured as RPVST+\n", 
            bridge_name);
        return CLI_ERROR;
      }
    
    switch (br->type)
      {
      case NSM_BRIDGE_TYPE_STP:
      case NSM_BRIDGE_TYPE_STP_VLANAWARE:

        mstp_stats_clear_info (br, ifp, cli);

        break;
      case NSM_BRIDGE_TYPE_RSTP:
      case NSM_BRIDGE_TYPE_RSTP_VLANAWARE:
          {
            mstp_stats_clear_info (br, ifp, cli);
            break;
          }
#ifdef HAVE_RPVST_PLUS
      case NSM_BRIDGE_TYPE_RPVST_PLUS:
          {
            if (!IS_BRIDGE_RPVST_PLUS(br))
              {
                cli_out (cli, "%% bridge %s is not configured as RPVST bridge \n", br->name);
                return CLI_ERROR;
              }
            MSTP_GET_VLAN_INSTANCE_MAP(br->vlan_instance_map, vid,
                                       instance);

            if (instance <= 0)
              {
                cli_out (cli, "%% VLAN does not exist for RPVST+ bridge %s\n", br->name);
                return CLI_ERROR;
              }
            if (vid > 0)
              { 
                ret = mstp_stats_clear_single_inst_info (br, ifp, cli, 
                                                         instance, vid);
                if (ret < 0)
                  return CLI_ERROR;
              }
            else
              {
               mstp_stats_clear_info(br, ifp, cli);
               ret = mstp_stats_clear_mstp (br, ifp, cli, instance);
               if (ret < 0)
                 return CLI_ERROR;
              }
            break;
          }
#endif /* HAVE_RPVST_PLUS */ 
      case NSM_BRIDGE_TYPE_MSTP :
          {
            if (! IS_BRIDGE_MSTP (br))
              {
                cli_out (cli, "%% bridge %s is not a MSTP bridge\n", br->name);
                return CLI_ERROR;
              }
            if (!br->instance_list[instance] && instance != 0)
              {
                cli_out (cli, "%% bridge %s instance %d not configured\n \n", br->name, instance);
                return CLI_ERROR;
              }

            if (instance > 0)
              {
                ret = mstp_stats_clear_single_inst_info (br, ifp, cli, 
                                                         instance, vid);
                if (ret < 0)
                  return CLI_ERROR;
              }
            else
              {
                mstp_stats_clear_info(br, ifp, cli);
                ret = mstp_stats_clear_mstp (br, ifp, cli, instance);
                if (ret < 0)
                  return CLI_ERROR;
              }

            break;
          }
      }
    return CLI_SUCCESS;
}
ALI (mstp_clear_spanning_tree_interface,
     mstp_clear_spanning_tree_bridge_cmd,
     "clear spanning-tree statistics bridge <1-32>",
     CLI_CLEAR_STR,
     "Spanning-tree Information",
     "Statistics of the BPDUs",
     "Bridge",
     "Bridge ID");

ALI (mstp_clear_spanning_tree_interface,
     mstp_clear_spanning_tree_interface_bridge_cmd,
     "clear spanning-tree statistics interface IFNAME (instance <1-63>| vlan <1-4094>) bridge <1-32>",
     CLI_CLEAR_STR,
     "Display spanning-tree information",
     "statistics of the BPDUs",
     "interface",
     CLI_IFNAME_STR,
     "Display instance information",
     "instance_id",
     "vlan",
     "vlan id to be associated to instance",
     "bridge",
     "bridge id");


#ifdef HAVE_PROVIDER_BRIDGE

CLI (show_cus_spanning_tree,
     show_cus_spanning_tree_cmd,
     "show customer spanning-tree",
     CLI_SHOW_STR,
     "Display Customer spanning-tree",
     "Display spanning-tree information")
{
  struct mstp_port *port;
  struct mstp_bridge * br;

  if (LAYER2_DEBUG(proto, CLI_ECHO))
    zlog_debug (mstpm, "show spanning-tree mst");

  br = mstp_get_first_bridge ();

  for (port = br->port_list; port;port = port->next)
    {
      /* Print bridge info */
      if (port->ce_br)
        mstp_display_cist_info (port->ce_br, NULL, cli, PAL_TRUE);
    }

  return CLI_SUCCESS;
}

#endif /* HAVE_PROVIDER_BRIDGE */

#ifdef HAVE_MSTPD
CLI (show_spanning_tree_mst,
     show_spanning_tree_mst_cmd,
     "show spanning-tree mst",
     CLI_SHOW_STR,
     "spanning-tree Display spanning tree information",
     "Display MST information" )
{
  struct mstp_bridge * br;

  if (LAYER2_DEBUG(proto, CLI_ECHO))
    zlog_debug (mstpm, "show spanning-tree mst");

  br = mstp_get_first_bridge();
  
  while(br)
    {
      if (IS_BRIDGE_MSTP (br))
        {
          /* Print bridge info */
          mstp_display_cist_info (br, NULL, cli, PAL_FALSE);
          cli_out (cli, "%% \n");
          mstp_display_instance_info (br, NULL, cli);
        }

      br = br->next;
    }
  return CLI_SUCCESS;
}
#endif /* HAVE_MSTPD */

CLI (show_spanning_tree_interface,
     show_spanning_tree_interface_cmd,
     "show spanning-tree interface IFNAME",
     CLI_SHOW_STR,
     "Display spanning-tree information",
     CLI_INTERFACE_STR,
     CLI_IFNAME_STR)
{
  struct mstp_bridge * br;
  struct interface *ifp;

  if (LAYER2_DEBUG(proto, CLI_ECHO))
    zlog_debug (mstpm, "show spanning-tree mst");

  ifp = if_lookup_by_name (&cli->vr->ifm, argv[0]);

  if ((ifp == NULL) || (ifp->port == NULL))
    {
      cli_out (cli, "%% Can't find interface %s\n", argv[0]);
      return CLI_ERROR;
    }

  br = mstp_find_bridge (ifp->bridge_name);
  if (br)
    {
      if (! (IS_BRIDGE_MSTP (br)))
        {
          /* Print bridge info */
          mstp_display_cist_info (br, ifp, cli, PAL_TRUE);
          cli_out (cli, "%% \n");
        }

      br = br->next;
    }
  return CLI_SUCCESS;
}

#ifdef HAVE_MSTPD
CLI (show_spanning_tree_mst_interface,
     show_spanning_tree_mst_interface_cmd,
     "show spanning-tree mst interface IFNAME",
     CLI_SHOW_STR,
     "spanning-tree Display spanning tree information",
     "Display MST information",
     CLI_INTERFACE_STR,
     CLI_IFNAME_STR)
{
  struct mstp_bridge * br;
  struct interface *ifp;

  if (LAYER2_DEBUG(proto, CLI_ECHO))
    zlog_debug (mstpm, "show spanning-tree mst");

  ifp = if_lookup_by_name (&cli->vr->ifm, argv[0]);

  if ((ifp == NULL) || (ifp->port == NULL))
    {
      cli_out (cli, "%% Can't find interface %s\n", argv[0]);
      return CLI_ERROR;
    }

  br = mstp_find_bridge (ifp->bridge_name);
  if (br)
    {
      if (IS_BRIDGE_MSTP (br))
        {
          /* Print bridge info */
          mstp_display_cist_info (br, ifp, cli, PAL_FALSE);
          cli_out (cli, "%% \n");
          mstp_display_instance_info (br, ifp, cli);
        }

      br = br->next;
    }
  return CLI_SUCCESS;
}

CLI (show_spanning_tree_mst_detail,
     show_spanning_tree_mst_detail_cmd,
     "show spanning-tree mst detail",
     CLI_SHOW_STR,
     "spanning-tree Display spanning tree information",
     "Display MST information",
     "Display detailed information" )
{
  struct mstp_bridge * br;

  if (LAYER2_DEBUG(proto, CLI_ECHO))
    zlog_debug (mstpm, "show spanning-tree mst detail");
  br = mstp_get_first_bridge();

  while(br)
    {
      if (IS_BRIDGE_MSTP (br))
        mstp_display_instance_info_det (br, NULL, cli);
      br = br->next;
    }
  return CLI_SUCCESS;
}

CLI (show_spanning_tree_mst_detail_interface,
     show_spanning_tree_mst_detail_interface_cmd,
     "show spanning-tree mst detail interface IFNAME",
     CLI_SHOW_STR,
     "spanning-tree Display spanning tree information",
     "Display MST information",
     "Display detailed information",
     CLI_INTERFACE_STR,
     CLI_IFNAME_STR)
{
  struct mstp_bridge * br;
  struct interface *ifp;

  if (LAYER2_DEBUG(proto, CLI_ECHO))
    zlog_debug (mstpm, "show spanning-tree mst detail interface IFNAME");

  ifp = if_lookup_by_name (&cli->vr->ifm, argv[0]);

  if ((ifp == NULL) || (ifp->port == NULL))
    {
      cli_out (cli, "%% Can't find interface %s\n", argv[0]);
      return CLI_ERROR;
    }

  br = mstp_find_bridge (ifp->bridge_name);
  if (br && (IS_BRIDGE_MSTP (br)))
    mstp_display_instance_info_det (br, ifp, cli); 

  return CLI_SUCCESS;
} 


CLI (show_spanning_tree_mst_instance,
     show_spanning_tree_mst_instance_cmd,
     "show spanning-tree mst instance (<1-63> | te-msti)",
     CLI_SHOW_STR,
     "spanning-tree Display spanning tree information",
     "Display MST information",
     "Display instance information",
     "instance_id",
     INSTANCE_TE_MSTI)
{
  struct mstp_bridge *br = NULL;
  int instance;

  if (LAYER2_DEBUG(proto, CLI_ECHO))
    zlog_debug (mstpm, "show spanning-tree mst");

  if (pal_strcmp(argv[0],"te-msti") == 0)
    instance = MSTP_TE_MSTID;
  else
    CLI_GET_INTEGER_RANGE ("instance", instance, argv[0], 
                         MST_INSTANCE_IST,MST_MAX_INSTANCES);

  br = mstp_get_first_bridge();

  while(br)
    {
      if (! mstp_find_bridge(br->name))
        {
          cli_out (cli, "%% bridge %s is not configured as MSTP bridge\n",
                   br->name);
          br = br->next;
          continue;
        }
      if (!br->instance_list[instance])
        {
          cli_out (cli, "%% bridge %s instance %d not configured \n \n", 
                                                      br->name, instance);
          br = br->next;
          continue;
        }

      if (instance == MST_INSTANCE_IST)
        mstp_display_cist_info (br, NULL, cli, PAL_TRUE); 
      else
        mstp_display_single_instance_info_det (br->instance_list[instance], 
                                               NULL, cli);
      cli_out (cli, "%% \n");
      br = br->next;
    }
  return CLI_SUCCESS;
}

/* For Range-Index */
CLI(show_spanning_tree_instance_vlan,
    show_spanning_tree_instance_vlan_cmd,
    "show spanning-tree  vlan range-index",
    CLI_SHOW_STR,
    "spanning-tree Display spanning tree information",
    "vlan information",
    "range-index value")
{
  int instance_index ;
  struct mstp_bridge_instance *br_inst = NULL;
  char *margin = "  ";
  char *delim = "      ";
  struct mstp_bridge *br = NULL;
  struct interface *ifp = NULL;
  struct rlist_info *vlan_list = NULL;
  
  br = mstp_get_first_bridge();

  if (br == NULL)
    {
      cli_out (cli, "%% No Bridge configured \n");
      return CLI_ERROR; 
    }
  
  cli_out (cli,"%%%s Bridge  MST Instance %s VLAN \t RangeIdx\n", margin,
      delim);

  while(br)
    {
      if (! mstp_find_bridge(br->name))
        {
          cli_out (cli, "%% bridge %s is not configured as MSTP bridge\n",
              br->name);
          br = br->next;
          continue;
        }
  
      if (IS_BRIDGE_MSTP (br))
        {
          for (instance_index = 1;instance_index < MST_MAX_INSTANCES;instance_index++)
            {
              br_inst = br->instance_list[instance_index];

              if (br_inst == NULL || br_inst->vlan_list == NULL)
                continue;

              for (vlan_list = br_inst->vlan_list; vlan_list;
                  vlan_list = vlan_list->next)
                {
                  cli_out (cli,"%%%s %s \t    %d            %s ",margin, 
                           br->name, instance_index, delim);
                  mstp_display_msti_vlan_range_idx_list (cli, vlan_list);
                  if (ifp)
                    break;
                }
            }
          cli_out (cli, "%% \n");
        }
     br = br->next;
    }
  return CLI_SUCCESS;
}

  
    
CLI (show_spanning_tree_mst_instance_interface,
     show_spanning_tree_mst_instance_interface_cmd,
     "show spanning-tree mst instance <1-63> interface IFNAME",
     CLI_SHOW_STR,
     "spanning-tree Display spanning tree information",
     "Display MST information",
     "Display instance information",
     "instance_id",
     CLI_INTERFACE_STR,
     CLI_IFNAME_STR)
{
  struct mstp_bridge * br;
  struct interface *ifp;
  int instance;

  if (LAYER2_DEBUG(proto, CLI_ECHO))
    zlog_debug (mstpm, "show spanning-tree mst instance <1-63>"
               "INTERFACE IFNAME");

  CLI_GET_INTEGER_RANGE ("instance", instance, argv[0],
                         MST_INSTANCE_IST,MST_MAX_INSTANCES);

  ifp = if_lookup_by_name (&cli->vr->ifm, argv[1]);
  if ((ifp == NULL) || (ifp->port == NULL))
    {
      cli_out (cli, "%% Can't find interface %s\n", argv[0]);
      return CLI_ERROR; 
    }

  br = mstp_find_bridge (ifp->bridge_name);
  if (br)
    {
      if (! IS_BRIDGE_MSTP (br))
        {
          cli_out (cli, "%% bridge %s is not configured as MSTP bridge\n",
                   br->name);
          return CLI_SUCCESS;
        }

      if (!br->instance_list[instance])
        {
          cli_out (cli, "%% bridge %s instance %d not configured \n \n",
                                                      br->name, instance);
          return CLI_SUCCESS;
        }

      if (instance == MST_INSTANCE_IST)
        mstp_display_cist_info (br, ifp, cli, PAL_TRUE);
      else
        mstp_display_single_instance_info_det (br->instance_list[instance],
                                               ifp, cli);
      cli_out (cli, "%% \n");
    }

  return CLI_SUCCESS;
}

CLI (show_spanning_tree_mst_config,
     show_spanning_tree_mst_config_cmd,
     "show spanning-tree mst config",
     CLI_SHOW_STR,
     "spanning-tree Display spanning tree information",
     "Display MST information",
     "Display Configuration information")
{
  struct mstp_bridge * br;
  
  if (LAYER2_DEBUG(proto, CLI_ECHO))
    zlog_debug (mstpm, "show spanning-tree mst");

  br = mstp_get_first_bridge();

  while(br)
    {
      if (IS_BRIDGE_MSTP (br))
        {
          int index;
          cli_out (cli, "%% \n");
          cli_out (cli, "%%  MSTP Configuration Information for bridge %s :\n", 
                   br->name);   
          cli_out (cli, "%%------------------------------------------------------\n"); 
          /* Print bridge info */
          cli_out (cli, "%%  Format Id      : %d \n",
                   br->config.cfg_format_id);
          cli_out (cli, "%%  Name           : %s \n",br->config.cfg_name);
          cli_out (cli, "%%  Revision Level : %d \n",
                   br->config.cfg_revision_lvl);
          cli_out (cli, "%%  Digest         : 0x");
          for (index = 0; index < MSTP_CONFIG_DIGEST_LEN ; index++)
             {
               cli_out (cli, "%.2X",br->config.cfg_digest[index]);
             }
          cli_out (cli, "\n");
          cli_out (cli, "%%------------------------------------------------------\n"); 
        }
      br = br->next;
    }
      
  return CLI_SUCCESS;
}
#endif /* HAVE_MSTPD */

/* interface commands */
CLI (interface_mstp,
     interface_mstp_cmd,
     "interface IFNAME",
     "Select an interface to configure",
     "Interface's name")
{
  struct interface *ifp;

  if (!argv[0])
    return CLI_ERROR;

  if (LAYER2_DEBUG(proto, CLI_ECHO))
    zlog_debug (mstpm, "interface %s", argv[0]);

  ifp = ifg_get_by_name (&mstpm->ifg, argv[0]);

  cli->index = ifp;
  cli->mode = INTERFACE_MODE;

  return CLI_SUCCESS;
}

#ifdef HAVE_MSTPD 
/* Interface node's bridge configuration.  */
CLI (bridge_group_inst,
     bridge_group_inst_cmd,
     "bridge-group (<1-32> | backbone) instance (<1-63> | te-msti)",
     BRIDGE_STR,
     BRIDGE_NAME_STR,
     BACKBONE_BRIDGE_NAME,
     "Multiple Spanning tree instance",
     INSTANCE_ID_STR,
     INSTANCE_TE_MSTI)
{
  int ret;
  int instance_id;
  char bridge_name[9];
  struct mstp_bridge *br;
  char null_str[L2_BRIDGE_NAME_LEN];
  struct interface *ifp = NULL;
  struct mstp_interface *mstpif = NULL;
  struct port_instance_list *pinst_list = NULL;

  ifp = (struct interface *)cli->index;
  if (!ifp)
    return CLI_ERROR;

  /* Verify the interface existence in interface globalist */
  ifp = ifg_lookup_by_name(&mstpm->ifg, ifp->name);
  if (!ifp)
    return CLI_ERROR;

  mstpif = (struct mstp_interface *)ifp->info;
  if (!mstpif)
    return CLI_ERROR;

  if (LAYER2_DEBUG(proto, CLI_ECHO))
    {
      if (argc == 2)
        zlog_debug (mstpm, "bridge-group %s instance %s", argv[0], argv[1]);
      else
        zlog_debug (mstpm, "spanning-tree instance %s", argv[0]);
    }

  pal_strcpy (bridge_name, argv[0]);

  if (argc == 2 && pal_strcmp(argv[1],"te-msti")==0)
    instance_id = MSTP_TE_MSTID; 
  else if (argc == 2)  
    CLI_GET_INTEGER_RANGE ("instance", instance_id, argv[1], 1, 63);
  else
    CLI_GET_INTEGER_RANGE ("instance", instance_id, argv[0], 1, 63);

  br = mstp_find_bridge (bridge_name);

  if (br != NULL && ! IS_BRIDGE_MSTP (br)) 
    {
      cli_out (cli, "%% Bridge %s is not configured as MSTP\n", bridge_name);
      return CLI_ERROR;
    }

  if (if_lookup_by_name (&cli->vr->ifm, ifp->name) == NULL || br == NULL)
    {
      pinst_list = XCALLOC (MTYPE_PORT_INSTANCE_LIST,
                            sizeof (struct port_instance_list));
      if (pinst_list == NULL)
        {
          cli_out (cli, "Could not allocate memory to store instance "
                         "information\n");
          return CLI_ERROR;
        }
      pinst_list->instance_info.instance = instance_id;
      mstpif_config_add_instance (mstpif, pinst_list);
      mstp_br_config_instance_add_port(bridge_name, ifp, instance_id);
      pal_strcpy (mstpif->bridge_group, bridge_name);
      SET_FLAG (mstpif->config, MSTP_IF_CONFIG_BRIDGE_GROUP_INSTANCE);
      return CLI_ERROR;
    }

  if ( ! mstp_find_cist_port (mstp_find_bridge (bridge_name), ifp->ifindex) )
    {
      pinst_list = XCALLOC (MTYPE_PORT_INSTANCE_LIST,
                            sizeof (struct port_instance_list));
      if (pinst_list == NULL)
        {
          cli_out (cli, "%% Could not allocate memory to store instance "
                        "information\n");
          return CLI_ERROR;
        }
      pinst_list->instance_info.instance = instance_id;
      mstpif_config_add_instance (mstpif, pinst_list);

      pal_mem_set (null_str, '\0', L2_BRIDGE_NAME_LEN);

      if ((pal_strncmp (mstpif->bridge_group, null_str, L2_BRIDGE_NAME_LEN) !=0)
          && (pal_strncmp (mstpif->bridge_group, bridge_name,
              L2_BRIDGE_NAME_LEN) != 0))
        {
          cli_out (cli, " Can't add instance \n");
        }

      pal_strcpy (mstpif->bridge_group, bridge_name);
      SET_FLAG (mstpif->config, MSTP_IF_CONFIG_BRIDGE_GROUP_INSTANCE);
      cli_out (cli, " %% Associate the interface to the bridge \n");
      return CLI_ERROR;
    }

  ret = mstp_api_add_port (bridge_name, ifp->name, MSTP_VLAN_NULL_VID,
                           instance_id, PAL_FALSE);

  if (ret < 0)
    {
      if (ret == MSTP_ERR_INSTANCE_ALREADY_BOUND)
        {
          cli_out (cli, "%% Instance already added\n");
          return CLI_ERROR;
        } 
      else
       { 
         mstp_br_config_instance_add_port (bridge_name, ifp, instance_id);
         cli_out (cli, "%% Can't add port to bridge\n");
         return CLI_ERROR;
       }
    }

  /* store the bridge name for convineince */
  pal_strncpy (ifp->bridge_name, bridge_name, INTERFACE_NAMSIZ + 1);

  return CLI_SUCCESS;
}

ALI (bridge_group_inst,
     spanning_inst_cmd,
     "spanning-tree instance <1-63>",
     SPANNING_STR,
     "Multiple Spanning Tree Instance",
     "identifier");

CLI (no_bridge_group_inst,
     no_bridge_group_inst_cmd,
     "no bridge-group (<1-32> | backbone) instance (<1-63> | te-msti)",
     CLI_NO_STR,
     BRIDGE_STR,
     BRIDGE_NAME_STR,
     BACKBONE_BRIDGE_NAME,
     "multiple spanning tree instance",
     INSTANCE_ID_STR,
     INSTANCE_TE_MSTI)
{
  int ret;
  int instance_id;
  char bridge_name[9];
  struct interface * ifp = cli->index;

  if (LAYER2_DEBUG(proto, CLI_ECHO))
    {
      if (argc == 2)
        zlog_debug (mstpm, "no bridge-group %s instance %s", argv[0], argv[1]);
      else
        zlog_debug (mstpm, "no spanning-tree instance %s", argv[0]);
    }

  pal_strcpy (bridge_name, argv[0]);
  
  if (argc == 2 && pal_strcmp(argv[1],"te-msti")==0)
    instance_id = MSTP_TE_MSTID;
  else if (argc == 2)
    CLI_GET_INTEGER_RANGE ("instance", instance_id, argv[1], 1, 63);
  else
    CLI_GET_INTEGER_RANGE ("instance", instance_id, argv[0], 1, 63);

  ret = mstp_delete_port (bridge_name, ifp->name, MSTP_VLAN_NULL_VID,
                          instance_id, PAL_FALSE, PAL_TRUE);

  mstp_bridge_port_instance_config_delete (ifp, bridge_name, instance_id);

  if (ret < 0)
    {
      cli_out (cli, "%% Can't delete interface from bridge group\n");
      return CLI_ERROR;
    }

  return CLI_SUCCESS;
}

ALI (no_bridge_group_inst,
     no_spanning_inst_cmd,
     "no spanning-tree instance <1-63>",
     CLI_NO_STR,
     SPANNING_STR,
     "Multiple Spanning Tree Instance",
     "identifier");

CLI (bridge_group_inst_priority,
     bridge_group_inst_priority_cmd,
     "bridge-group (<1-32> | backbone) instance <1-63> priority <0-240>",
     BRIDGE_STR,
     BRIDGE_NAME_STR,
     BACKBONE_BRIDGE_NAME,
     "multiple spanning tree instance",
     "identifier",
     "port priority for a bridge",
     "port priority in increments of 16 (lower priority indicates greater likelihood of becoming root)")
{
  int ret;
  struct interface *ifp = cli->index;
  s_int16_t priority = 0;
  int instance;
  int brno;
  u_int8_t *bridge_name;
  struct mstp_interface *mstpif = ifp->info;
  struct port_instance_info *pinst_info = NULL;
  char *str = NULL;
  struct mstp_bridge *br;

  if (LAYER2_DEBUG(proto, CLI_ECHO))
    {
      if (argc == 3)
        zlog_debug (mstpm, "bridge-group %s instance %s priority %s",
                    argv[0], argv[1], argv[2]);
      else
        zlog_debug (mstpm, "spanning-tree instance %s priority %s",
                    argv[0], argv[1]);
    }

  MSTP_GET_BR_NAME (3, 0);

  if (argc == 3)
    {
      CLI_GET_INTEGER_RANGE ("instance", instance, argv[1], 1, 63);
      CLI_GET_INTEGER_RANGE ("priority", priority, argv[2], 0, 240);
    }
  else
    {
      CLI_GET_INTEGER_RANGE ("instance", instance, argv[0], 1, 63);
      CLI_GET_INTEGER_RANGE ("priority", priority, argv[1], 0, 240);
    }

  br = mstp_find_bridge (bridge_name);

  if ((br != NULL) && (! IS_BRIDGE_MSTP (br)))
    {
      cli_out (cli, "%% Bridge %s is not configured as MSTP\n", bridge_name);
      return CLI_ERROR;
    }

  if (if_lookup_by_name (&cli->vr->ifm, ifp->name) == NULL || br == NULL)
    {
      pal_strcpy (mstpif->bridge_group, bridge_name);
      pinst_info = mstpif_get_instance_info (mstpif, instance);
      if (!pinst_info)  
        return CLI_ERROR;
      pinst_info->port_instance_priority = priority; 
      SET_FLAG (pinst_info->config, MSTP_IF_CONFIG_BRIDGE_INSTANCE_PRIORITY);
      return CLI_ERROR;
    }

  ret = mstp_api_set_msti_port_priority (bridge_name, ifp->name,
                                         instance,  priority);
  if (ret < 0)
    switch (ret)
      {
        case MSTP_ERR_BRIDGE_NOT_FOUND :
          str = "Bridge not found";
          break;
        case MSTP_ERR_PRIORITY_VALUE_WRONG :
          str = "The priority value must be in multiples of 16";
          break;
        case MSTP_ERR_PRIORITY_OUTOFBOUNDS :
          str = "The priority value is out of bounds";
          break;
        case MSTP_ERR_PORT_NOT_FOUND :
          str = "Port not found";
          break;
        case MSTP_ERR_INSTANCE_NOT_FOUND :
          str = "Bridge-instance not found";
          break;
        default:
          str = "Can't configure priority for port";
          break;
      }

  if (ret < 0)
    {
      cli_out (cli, "%% %s\n", str);
      return CLI_ERROR;
    }

  return CLI_SUCCESS;
}

ALI (bridge_group_inst_priority,
     spanning_port_inst_priority_cmd,
     "spanning-tree instance <1-63> priority <0-240>",
     SPANNING_STR,
     "multiple spanning tree instance",
     "identifier",
     "port priority for a bridge",
     "port priority in increments of 16 (lower priority indicates greater likelihood of becoming root)");

#endif /* HAVE_MSTPD */

CLI (mstp_bridge_group_priority,
     mstp_bridge_group_priority_cmd,
     "bridge-group <1-32> priority <0-240>",
     BRIDGE_STR,
     BRIDGE_NAME_STR,
     "port priority for a bridge",
     "port priority in increments of 16 (lower priority indicates greater "
     "likelihood of becoming root)")
{
  int ret;
  struct interface *ifp = cli->index;
  s_int16_t priority = 0;
  struct mstp_interface *mstpif = ifp->info;
  int brno;
  char *str = NULL;

  if (LAYER2_DEBUG(proto, CLI_ECHO))
    zlog_debug (mstpm, "bridge-group %s priority %s", argv[0], argv[1]);

  CLI_GET_INTEGER_RANGE ("Max bridge groups", brno, argv[0], 1, 32);
  CLI_GET_INTEGER_RANGE ("priority", priority, argv[1], 0, 240);

  if (if_lookup_by_name (&cli->vr->ifm, ifp->name) == NULL)
    {
      pal_strcpy (mstpif->bridge_group, argv[0]);
      mstpif->bridge_priority = priority;
      SET_FLAG (mstpif->config, MSTP_IF_CONFIG_BRIDGE_PRIORITY);
      return CLI_ERROR;
    }

  ret = mstp_api_set_port_priority (argv[0], ifp->name,
                                    MSTP_VLAN_NULL_VID, priority);

  if (ret < 0)
    switch (ret)
      {
        case MSTP_ERR_BRIDGE_NOT_FOUND :
          str = "Bridge not found";
          break;
        case MSTP_ERR_PRIORITY_VALUE_WRONG :
          str = "The priority value must be in multiples of 16";
          break;
        case MSTP_ERR_PRIORITY_OUTOFBOUNDS :
          str = "The priority value is out of bounds";
          break;
        case MSTP_ERR_PORT_NOT_FOUND :
          str = "Port not found";
          break;
        default:
          str = "Can't configure priority for port";
          break; 
      }

  if (ret < 0)
    {
      cli_out (cli, "%% %s\n", str);
      return CLI_ERROR;
    }

  return CLI_SUCCESS;
}

CLI (mstp_bridge_group_path_cost,
     mstp_bridge_group_path_cost_cmd,
     "bridge-group <1-32> path-cost <1-200000000>",
     BRIDGE_STR,
     BRIDGE_NAME_STR,
     "path cost for a port",
     "path cost in range <1-200000000> (lower path cost indicates greater "
     "likelihood of becoming root)")
{
  int ret;
  u_int8_t *bridge_name;
  struct interface *ifp = cli->index;
  u_int32_t path_cost = 0;
  struct mstp_interface *mstpif = ifp->info;
  int brno;

  if (LAYER2_DEBUG(proto, CLI_ECHO))
    {
      if (argc == 2)
        zlog_debug (mstpm, "bridge-group %s path-cost %s", argv[0], argv[1]);
      else
        zlog_debug (mstpm, "spanning-tree path-cost %s", argv[0]);
    }

  MSTP_GET_BR_NAME (2, 0);

  if (argc == 2)
    CLI_GET_INTEGER_RANGE ("path-cost", path_cost, argv[1], 1, 200000000);
  else
    CLI_GET_INTEGER_RANGE ("path-cost", path_cost, argv[0], 1, 200000000);

  if (if_lookup_by_name (&cli->vr->ifm, ifp->name) == NULL) 
    {
      pal_strcpy (mstpif->bridge_group, bridge_name);
      mstpif->bridge_pathcost = path_cost;
      SET_FLAG (mstpif->config, MSTP_IF_CONFIG_BRIDGE_PATHCOST);
      return CLI_SUCCESS; 
    }

  ret = mstp_api_set_port_path_cost (bridge_name, ifp->name,
                                     MSTP_VLAN_NULL_VID, path_cost);

  if (ret < 0)
    {
      cli_out (cli, "%% Can't configure path cost for port %s\n", ifp->name);
      return CLI_ERROR;
    }
  else
    mstp_set_port_pathcost_configured (ifp, PAL_TRUE);
  return CLI_SUCCESS;
}

ALI (mstp_bridge_group_path_cost,
     mstp_spanning_path_cost_cmd,
     "spanning-tree path-cost <1-200000000>",
     SPANNING_STR,
     "path cost for a port",
     "path cost in range <1-200000000> (lower path cost indicates greater "
     "likelihood of becoming root)");

CLI (no_mstp_bridge_group_path_cost,
     no_mstp_bridge_group_path_cost_cmd,
     "no bridge-group <1-32> path-cost",
     CLI_NO_STR,
     BRIDGE_STR,
     BRIDGE_NAME_STR,
     "path-cost - path cost for a port")
{
  int ret;
  struct interface *ifp = cli->index;
  int brno;
  u_int8_t *bridge_name;

  if (LAYER2_DEBUG(proto, CLI_ECHO))
    {
      if (argc == 1)
        zlog_debug (mstpm, "no bridge-group %s path-cost", argv[0]);
      else
        zlog_debug (mstpm, "no spanning-tree path-cost");
    }

  MSTP_GET_BR_NAME (1, 0);

  ret = mstp_api_set_port_path_cost (bridge_name, ifp->name, MSTP_VLAN_NULL_VID,
                                     mstp_nsm_get_port_path_cost(mstpm, ifp->ifindex));
  if (ret < 0)
    {
      cli_out (cli, "%% Can't configure path cost for port %s\n", ifp->name);
      return CLI_ERROR;
    }
  else
    mstp_set_port_pathcost_configured (ifp, PAL_FALSE);

  return CLI_SUCCESS;
}

ALI (no_mstp_bridge_group_path_cost,
     no_mstp_spanning_path_cost_cmd,
     "no spanning-tree path-cost",
     CLI_NO_STR,
     SPANNING_STR,
     "path-cost - path cost for a port");

#ifdef HAVE_MSTPD
CLI (bridge_group_inst_path_cost,
     bridge_group_inst_path_cost_cmd,
     "bridge-group (<1-32> | backbone) instance <1-63> path-cost <1-200000000>",
     BRIDGE_STR,
     BRIDGE_NAME_STR,
     BACKBONE_BRIDGE_NAME,
     "Multiple Spanning Tree Instance",
     "identifier",
     "path cost for a port",
     "path cost in range <1-200000000> (lower path cost indicates greater likelihood of becoming root)")
{
  int ret;
  u_int8_t *bridge_name;
  struct interface *ifp = cli->index;
  u_int32_t path_cost = 0;
  int instance;
  struct mstp_interface *mstpif = ifp->info;
  int brno;
  struct port_instance_info *pinst_info = NULL;
  struct mstp_bridge *br;

  if (LAYER2_DEBUG(proto, CLI_ECHO))
    {
      if (argc == 3)
        zlog_debug (mstpm, "bridge-group %s instance %s path-cost %s",
                    argv[0], argv[1], argv[2]);
      else
        zlog_debug (mstpm, "spanning-tree instance %s path-cost %s",
                    argv[0], argv[1]);
    }

  MSTP_GET_BR_NAME (3, 0);

  if (argc == 3 )
    {
      CLI_GET_INTEGER_RANGE ("instance", instance, argv[1], 1, 63);
      CLI_GET_INTEGER_RANGE ("path-cost", path_cost, argv[2], 1, 200000000);
    }
  else
    {
      CLI_GET_INTEGER_RANGE ("instance", instance, argv[0], 1, 63);
      CLI_GET_INTEGER_RANGE ("path-cost", path_cost, argv[1], 1, 200000000);
    }

  br = mstp_find_bridge (bridge_name);

  if ((br != NULL) && (! IS_BRIDGE_MSTP (br)))
    {
      cli_out (cli, "%% Bridge %s is not configured as MSTP\n", bridge_name);
      return CLI_ERROR;
    }

  if (if_lookup_by_name (&cli->vr->ifm, ifp->name) == NULL || br == NULL)
    {
      pal_strcpy (mstpif->bridge_group, bridge_name);
      pinst_info = mstpif_get_instance_info (mstpif, instance);
      if (!pinst_info)
        return CLI_ERROR;
      pinst_info->port_instance_pathcost = path_cost;
      SET_FLAG (pinst_info->config, MSTP_IF_CONFIG_BRIDGE_INSTANCE_PATHCOST);
      return CLI_ERROR;
    }

  ret = mstp_api_set_msti_port_path_cost (bridge_name, ifp->name, 
                                          instance, path_cost);
  if (ret < 0)
    {
      cli_out (cli, "%% Can't configure path cost for port %s\n",
               ifp->name);
      return CLI_ERROR;
    }

  return CLI_SUCCESS;
}

ALI (bridge_group_inst_path_cost,
     spanning_inst_path_cost_cmd,
     "spanning-tree instance <1-63> path-cost <1-200000000>",
     SPANNING_STR,
     "Multiple Spanning Tree Instance",
     "identifier",
     "path cost for a port",
     "path cost in range <1-200000000>"
     " (lower path cost indicates greater likelihood of becoming root)");

CLI (no_bridge_group_inst_path_cost,
     no_bridge_group_inst_path_cost_cmd,
     "no bridge-group ( <1-32> | backbone) instance <1-63> path-cost",
     CLI_NO_STR,
     BRIDGE_STR,
     BRIDGE_NAME_STR,
     BACKBONE_BRIDGE_NAME,
     "Multiple Spanning Tree Instance",
     "identifier",
     "path-cost - path cost for a port")
{
  int ret;
  struct interface *ifp = cli->index;
  u_int8_t *bridge_name;
  int instance;
  int brno;

  if (LAYER2_DEBUG(proto, CLI_ECHO))
     {
      if (argc == 2)
        {
          zlog_debug (mstpm, "no bridge-group %s instance %s path-cost",
                      argv[0], argv[1]);
        }
      else
        {
          zlog_debug (mstpm, "no spanning-tree instance %s path-cost",
                      argv[0]);
        }
    }

  MSTP_GET_BR_NAME (2, 0);

  if (argc == 2)
    CLI_GET_INTEGER_RANGE ("instance", instance, argv[1], 1, 63);
  else
    CLI_GET_INTEGER_RANGE ("instance", instance, argv[0], 1, 63);
 
  ret = mstp_api_set_msti_port_path_cost (bridge_name, ifp->name, instance,
                                          mstp_nsm_get_port_path_cost(mstpm, ifp->ifindex));
  if (ret < 0)
    {
      cli_out (cli, "%% Can't configure path cost for port %s\n", ifp->name);
      return CLI_ERROR;
    }
  return CLI_SUCCESS;
}

ALI (no_bridge_group_inst_path_cost,
     no_spanning_inst_path_cost_cmd,
     "no spanning-tree instance <1-63> path-cost",
     CLI_NO_STR,
     SPANNING_STR,
     "Multiple Spanning Tree Instance",
     "identifier",
     "path-cost - path cost for a port");

#endif /* HAVE_MSTPD */

/* Rapid Spanning tree specific CLI commands - begin */
CLI (mstp_spanning_tree_linktype,
     mstp_spanning_tree_linktype_cmd,
     "spanning-tree link-type point-to-point",
     "spanning tree commands",
     "link-type - point-to-point or shared",
     "point-to-point - enable rapid transition")
{
  int ret;
  struct interface *ifp = cli->index;
  struct mstp_interface *mstpif = NULL;

  if (LAYER2_DEBUG(proto, CLI_ECHO))
    zlog_debug (mstpm, "spanning-tree link-type point-to-point");

  if ((ifp == NULL) || (ifp->port == NULL))
    {
      if (ifp)
        cli_out (cli, "%% Can't configure on interface %s\n", ifp->name);
      return CLI_ERROR;
    }

  mstpif = ifp->info;

  if (if_lookup_by_name (&cli->vr->ifm, ifp->name) == NULL)
    {
      mstpif->port_p2p = PAL_TRUE;
      SET_FLAG (mstpif->config, MSTP_IF_CONFIG_LINKTYPE);
      return CLI_ERROR;
    }

  if ((ifp == NULL) || (ifp->port == NULL))
    {
      if (ifp)
        cli_out (cli, "%% Can't configure on interface %s\n", ifp->name);
      return CLI_ERROR;
    }

  ret = mstp_api_set_port_p2p (ifp->bridge_name, ifp->name, PAL_TRUE);
  if (ret < 0)
    {
      cli_out (cli, "%% Can't configure port to be point to point\n");
      return CLI_ERROR;
    }

  return CLI_SUCCESS;
}

CLI (mstp_spanning_tree_linktype_shared,
     mstp_spanning_tree_linktype_shared_cmd,
     "spanning-tree link-type shared",
     "spanning tree commands",
     "link-type - point-to-point or shared",
     "shared - disable rapid transition")
{
  int ret;
  struct interface *ifp = cli->index;
  struct mstp_interface *mstpif = NULL;

  if (LAYER2_DEBUG(proto, CLI_ECHO))
    zlog_debug (mstpm, "spanning-tree link-type shared");

  if (ifp)
   mstpif = ifp->info;
   
  if ((ifp == NULL) || (ifp->port == NULL))
    {
      if (mstpif)
        {
          mstpif->port_p2p = PAL_TRUE;
          SET_FLAG (mstpif->config, MSTP_IF_CONFIG_LINKTYPE);
        }

      cli_out (cli, "%% Can't configure on interface %s\n", ifp->name);
      return CLI_ERROR;
    }

  if (if_lookup_by_name (&cli->vr->ifm, ifp->name) == NULL)
    {
      mstpif->port_p2p = PAL_FALSE;
      SET_FLAG (mstpif->config, MSTP_IF_CONFIG_LINKTYPE);
      return CLI_ERROR;
    }

  ret = mstp_api_set_port_p2p (ifp->bridge_name, ifp->name, PAL_FALSE);
  if (ret < 0)
    {
      mstpif->port_p2p = PAL_TRUE;
      SET_FLAG (mstpif->config, MSTP_IF_CONFIG_LINKTYPE);
      cli_out (cli, "%% Can't configure port to be shared\n");
      return CLI_ERROR;
    }

  return CLI_SUCCESS;
}

CLI (mstp_spanning_tree_linktype_auto,
     mstp_spanning_tree_linktype_auto_cmd,
     "spanning-tree link-type auto",
     "spanning tree commands",
     "link-type - point-to-point shared or auto",
     "auto - will be set to either p2p or shared based on duplex state")
{
  int ret;
  struct interface *ifp = cli->index;
  struct mstp_interface *mstpif = NULL;

  if (ifp)
    mstpif = ifp->info;
  else
    {
      cli_out (cli, "%% Interface not found.\n");
      return CLI_ERROR;
    }

  if (LAYER2_DEBUG(proto, CLI_ECHO))
    zlog_debug (mstpm, "spanning-tree link-type auto");

  if (if_lookup_by_name (&cli->vr->ifm, ifp->name) == NULL)
    {
      mstpif->port_p2p = PAL_FALSE;
      SET_FLAG (mstpif->config, MSTP_IF_CONFIG_LINKTYPE);
      return CLI_ERROR;
    }

  if ((ifp == NULL) || (ifp->port == NULL))
    {
      mstpif->port_p2p = PAL_TRUE;
      SET_FLAG (mstpif->config, MSTP_IF_CONFIG_LINKTYPE);
      cli_out (cli, "%% Can't configure on interface %s\n", ifp->name);
      return CLI_ERROR;
    }

  ret = mstp_api_set_port_p2p (ifp->bridge_name, ifp->name, 
                               MSTP_ADMIN_LINK_TYPE_AUTO);
  if (ret < 0)
    {
      mstpif->port_p2p = PAL_TRUE;
      SET_FLAG (mstpif->config, MSTP_IF_CONFIG_LINKTYPE);
      cli_out (cli, "%% Can't configure port to be shared\n");
      return CLI_ERROR;
    }

  return CLI_SUCCESS;
}


CLI (no_mstp_spanning_tree_linktype,
     no_mstp_spanning_tree_linktype_cmd,
     "no spanning-tree link-type",
     CLI_NO_STR,
     "spanning-tree", 
     "default (point-to-point) link type enables rapid transitions")
{
  int ret;
  struct interface *ifp = cli->index;

  if (LAYER2_DEBUG(proto, CLI_ECHO))
    zlog_debug (mstpm, "no spanning-tree link-type");

  ret = mstp_api_set_port_p2p (ifp->bridge_name, ifp->name, PAL_TRUE);
  if (ret < 0)
    {
      cli_out (cli, "%% Can't configure port to be default link type "
               "for bridge\n");
      return CLI_ERROR;
    }

  return CLI_SUCCESS;
}

/* spanning-tree portfast */

CLI (mstp_spanning_tree_portfast,
     mstp_spanning_tree_portfast_cmd,
     "spanning-tree (portfast | edgeport)",
     "spanning-tree",
     "portfast - enable fast transitions",
     "edgeport - enable it as edgeport")
{
  int ret;
  struct interface *ifp = cli->index;
  struct mstp_interface *mstpif = NULL;
  bool_t portfast_conf = 0;
  
  if (ifp == NULL)
    {
      cli_out (cli, "%% Interface not found.\n");
      return CLI_ERROR;
    }
  
  mstpif = ifp->info;

  if (mstpif == NULL)
    {
      cli_out (cli, "%% mstp bridge port not found.\n");
      return CLI_ERROR;
    }
  
  if (LAYER2_DEBUG(proto, CLI_ECHO))
    zlog_debug (mstpm, "spanning-tree portfast");

  if (pal_strncmp (argv[0], "portfast", 1) == 0)
    portfast_conf = MSTP_CONFIG_PORTFAST;
  else if (pal_strncmp (argv[0], "edgeport", 1) == 0)
    portfast_conf = MSTP_CONFIG_EDGEPORT;

  if (if_lookup_by_name (&cli->vr->ifm, ifp->name) == NULL)
    {
      mstpif->port_edge = PAL_TRUE;
      mstpif->portfast_conf = portfast_conf;
      SET_FLAG (mstpif->config, MSTP_IF_CONFIG_PORTFAST);
      return CLI_ERROR;
    }

  if ((ifp == NULL) || (ifp->port == NULL))
    {
      mstpif->port_edge = PAL_TRUE;
      mstpif->portfast_conf = portfast_conf;
      SET_FLAG (mstpif->config, MSTP_IF_CONFIG_PORTFAST);
      cli_out (cli, "%% Can't find interface %s\n", ifp->name);
       return CLI_ERROR;
     }
  
  ret = mstp_api_set_port_edge (ifp->bridge_name, ifp->name, PAL_TRUE,
                                portfast_conf);
  if (ret < 0)
    {
     mstpif->port_edge = PAL_TRUE;
     mstpif->portfast_conf = portfast_conf;
     SET_FLAG (mstpif->config, MSTP_IF_CONFIG_PORTFAST);
     
     cli_out (cli, "%% Can't configure port %s to portfast\n", ifp->name);
      return CLI_ERROR;
    }

  return CLI_SUCCESS;
}

/* no spanning-tree portfast */

CLI (no_mstp_spanning_tree_portfast,
     no_mstp_spanning_tree_portfast_cmd,
     "no spanning-tree (portfast | edgeport)",
     CLI_NO_STR,
     "spanning-tree",
     "portfast - disable fast transitions",
     "edgeport - disable it as edgeport")
{
  int ret;
  struct interface *ifp = cli->index;
  bool_t portfast_conf = 0;

  if (LAYER2_DEBUG(proto, CLI_ECHO))
    zlog_debug (mstpm, "no spanning-tree portfast");

  if (pal_strncmp (argv[0], "portfast", 1))
    portfast_conf = MSTP_CONFIG_PORTFAST;
  else if (pal_strncmp (argv[0], "edgeport", 1))
    portfast_conf = MSTP_CONFIG_EDGEPORT;

  if ((ifp == NULL) || (ifp->port == NULL))
    {
     cli_out (cli, "%% Can't find interface %s\n", ifp->name);
     return CLI_ERROR;
    }

  ret = mstp_api_set_port_edge (ifp->bridge_name, ifp->name, PAL_FALSE, 
                                portfast_conf);
  if (ret < 0)
    {
      cli_out (cli, "%% Can't disable portfast for bridge\n");
      return CLI_ERROR;
    }

  return CLI_SUCCESS;
}

CLI (mstp_spanning_tree_portfast_bpduguard,
     mstp_spanning_tree_portfast_bpduguard_cmd,
     "spanning-tree portfast bpdu-guard (enable|disable|default)",
     "spanning-tree",
     "portfast",
     "guard the port against reception of BPDUs",
     "enable",
     "disable",
     "default")
{
  int ret;
  struct interface *ifp = cli->index;
  struct mstp_interface *mstpif = NULL;
  u_char bpduguard;

  if (LAYER2_DEBUG(proto, CLI_ECHO))
    zlog_debug (mstpm, "spanning-tree portfast bpdu-guard");

  /* Since there is a command spanning-tree (portfast|edgeport)
   * portfast is passed as argv[0] and (enable|disable|default)
   * is passed as argv[1]
   */

  if (! pal_strncmp (argv[1], "e", 1))
    bpduguard = MSTP_PORT_PORTFAST_BPDUGUARD_ENABLED;
  else if (! pal_strncmp (argv[1], "di", 2))
    bpduguard = MSTP_PORT_PORTFAST_BPDUGUARD_DISABLED;
  else
    bpduguard = MSTP_PORT_PORTFAST_BPDUGUARD_DEFAULT;

  if ((ifp == NULL) || (ifp->port == NULL))
    {
      cli_out (cli, "%% Can't find interface %s\n", ifp->name);
      return CLI_ERROR;
    }

  mstpif = ifp->info;
  
  if (if_lookup_by_name (&cli->vr->ifm, ifp->name) == NULL)
    {
      SET_FLAG (mstpif->config, MSTP_IF_CONFIG_BPDUGUARD);
      mstpif->bpduguard = bpduguard;
      return CLI_ERROR;
    }

  ret = mstp_api_set_port_bpduguard (ifp->bridge_name, ifp->name,
                                     bpduguard);
  if (ret < 0)
    {
      cli_out (cli, "%% Can't set portfast bpdu-guard for port\n");
      return CLI_ERROR;
    }

  return CLI_SUCCESS;

}

CLI (no_mstp_spanning_tree_portfast_bpduguard,
     no_mstp_spanning_tree_portfast_bpduguard_cmd,
     "no spanning-tree portfast bpdu-guard ",
     CLI_NO_STR,
     "spanning-tree",
     "portfast",
     "disable guarding the port against reception of BPDUs")
{
  int ret;
  struct interface *ifp = cli->index;

  if (LAYER2_DEBUG(proto, CLI_ECHO))
    zlog_debug (mstpm, "no spanning-tree portfast bpdu-guard");

  if ((ifp == NULL) || (ifp->port == NULL))
    {
      cli_out (cli, "%% Can't find interface %s\n", ifp->name);
      return CLI_ERROR;
    }

  ret = mstp_api_set_port_bpduguard (ifp->bridge_name, ifp->name,
                                     MSTP_PORT_PORTFAST_BPDUGUARD_DEFAULT);

  if (ret < 0)
    {
      cli_out (cli, "%% Can't set portfast bpdu-guard for port\n");
      return CLI_ERROR;
    }

  return CLI_SUCCESS;

}

/* spanning-tree autoedge*/

CLI (mstp_spanning_tree_autoedge,
     mstp_spanning_tree_autoedge_cmd,
     "spanning-tree autoedge",
     "spanning-tree",
     "autoedge - enable automatic edge detection")
{
  int ret;
  struct interface *ifp = cli->index;
  struct mstp_interface *mstpif = NULL;

  if (LAYER2_DEBUG(proto, CLI_ECHO))
    zlog_debug (mstpm, "spanning-tree autoedge");

  if (ifp)
    mstpif = ifp->info;

  if ((ifp == NULL) || (ifp->port == NULL))
    {
      if (mstpif)
        SET_FLAG (mstpif->config, MSTP_IF_CONFIG_AUTOEDGE);
      if (ifp)
        cli_out (cli, "%% Can't find interface %s\n", ifp->name);
      return CLI_ERROR;
    }
  

  if (if_lookup_by_name (&cli->vr->ifm, ifp->name) == NULL)
    {
      SET_FLAG (mstpif->config, MSTP_IF_CONFIG_AUTOEDGE);
      return CLI_ERROR;
    }

  ret = mstp_api_set_auto_edge (ifp->bridge_name, ifp->name, PAL_TRUE);

  if (ret < 0)
    {
      SET_FLAG (mstpif->config, MSTP_IF_CONFIG_AUTOEDGE);
      cli_out (cli, "%% Can't configure port %s to autoedge\n", ifp->name);
      return CLI_ERROR;
    }

  return CLI_SUCCESS;
}

/* no spanning-tree portfast */

CLI (no_mstp_spanning_tree_autoedge,
     no_mstp_spanning_tree_autoedge_cmd,
     "no spanning-tree autoedge",
     CLI_NO_STR,
     "spanning-tree",
     "autoedge - enable automatic edge detection")
{
  int ret;
  struct interface *ifp = cli->index;

  if (LAYER2_DEBUG(proto, CLI_ECHO))
    zlog_debug (mstpm, "no spanning-tree autoedge");

  if ((ifp == NULL) || (ifp->port == NULL))
    {
      cli_out (cli, "%% Can't find interface %s\n", ifp->name);
      return CLI_ERROR;
    }

  ret = mstp_api_set_auto_edge (ifp->bridge_name, ifp->name, PAL_FALSE);

  if (ret < 0)
    {
      cli_out (cli, "%% Can't autoedge portfast for bridge\n");
      return CLI_ERROR;
    }

  return CLI_SUCCESS;

}

CLI (mstp_spanning_tree_guard_root,
     mstp_spanning_tree_guard_root_cmd,
     "spanning-tree guard root",
     "spanning-tree",
     "guard",
     "disable reception of superior BPDUs")
{
  int ret;
  struct interface *ifp = cli->index;
  struct mstp_interface *mstpif = NULL;

  if (LAYER2_DEBUG(proto, CLI_ECHO))
    zlog_debug (mstpm, "spanning-tree guard root");

  if ((ifp == NULL) || (ifp->port == NULL))
    {
      if (ifp)
        cli_out (cli, "%% Can't find interface %s\n", ifp->name);
      return CLI_ERROR;
    }

  mstpif = ifp->info;

  if (if_lookup_by_name (&cli->vr->ifm, ifp->name) == NULL)
    {
      SET_FLAG (mstpif->config, MSTP_IF_CONFIG_ROOTGUARD);
      return CLI_ERROR;
    }

  ret = mstp_api_set_port_rootguard (ifp->bridge_name, ifp->name, PAL_TRUE);
  if (ret < 0)
    {
      cli_out (cli, "%% Can't set root guard for port\n");
      return CLI_ERROR;
    }

  return CLI_SUCCESS;

}

CLI (no_mstp_spanning_tree_guard_root,
     no_mstp_spanning_tree_guard_root_cmd,
     "no spanning-tree guard root",
     CLI_NO_STR,
     "spanning-tree",
     "guard",
     "disable root guard feature for this port")
{
  int ret;
  struct interface *ifp = cli->index;

  if (LAYER2_DEBUG(proto, CLI_ECHO))
    zlog_debug (mstpm, "spanning-tree guard root");

  if ((ifp == NULL) || (ifp->port == NULL))
    {
      cli_out (cli, "%% Can't find interface %s\n", ifp->name);
      return CLI_ERROR;
    }

  ret = mstp_api_set_port_rootguard (ifp->bridge_name, ifp->name, PAL_FALSE);
  if (ret < 0)
    {
      cli_out (cli, "%% Can't disable root guard for port\n");
      return CLI_ERROR;
    }

  return CLI_SUCCESS;

}

CLI (mstp_spanning_tree_portfast_bpdufilter,
     mstp_spanning_tree_portfast_bpdufilter_cmd,
     "spanning-tree portfast bpdu-filter (enable|disable|default)",
     "spanning-tree",
     "portfast",
     "set the portfast bpdu-filter for the port",
     "enable",
     "disable",
     "default")
{
  int ret;
  struct interface *ifp = cli->index;
  struct mstp_interface *mstpif = NULL;
  u_char bpdu_filter;

  if (ifp)
    mstpif = ifp->info;
  else
    {
       cli_out (cli, "%% Can't find interface \n");
       return CLI_ERROR;
     }
      
  
  if (LAYER2_DEBUG(proto, CLI_ECHO))
    zlog_debug (mstpm, "spanning-tree portfast bpdu-filter %s", argv[1]);

  /* Since there is a command spanning-tree (portfast|edgeport)
   * portfast is passed as argv[0] and (enable|disable|default)
   * is passed as argv[1]
   */

  if (! pal_strncmp (argv[1], "e", 1))
    bpdu_filter = MSTP_PORT_PORTFAST_BPDUFILTER_ENABLED;
  else if (! pal_strncmp (argv[1], "di", 2))
    bpdu_filter = MSTP_PORT_PORTFAST_BPDUFILTER_DISABLED;
  else
    bpdu_filter = MSTP_PORT_PORTFAST_BPDUFILTER_DEFAULT;

  if ((if_lookup_by_name (&cli->vr->ifm, ifp->name) == NULL)||
      (ifp->port == NULL))
    {
      if (mstpif)
      {
        SET_FLAG (mstpif->config, MSTP_IF_CONFIG_BPDUFILTER);
        mstpif->bpdu_filter = bpdu_filter;
        return CLI_ERROR;
      }
    }

  if ((ifp == NULL) || (ifp->port == NULL))
     {
       cli_out (cli, "%% Can't find interface \n");
       return CLI_ERROR;
     }

  ret = mstp_api_set_port_bpdufilter (ifp->bridge_name, ifp->name, bpdu_filter);
  if (ret < 0)
    {
      cli_out (cli, "%% Can't set portfast bpdu-filter for port\n");
      return CLI_ERROR;
    }

  cli_out (cli, "%% Warning:Ports enabled with bpdu filter will not send BPDUs and drop all received BPDUs.\n You may cause loops in the bridged network if you misuse this feature\n");

  return CLI_SUCCESS;

}

CLI (no_mstp_spanning_tree_portfast_bpdufilter,
     no_mstp_spanning_tree_portfast_bpdufilter_cmd,
     "no spanning-tree portfast bpdu-filter ",
     CLI_NO_STR,
     "spanning-tree",
     "portfast",
     "disable the bpdu-filter feature for the port")
{
  int ret;
  struct interface *ifp = cli->index;

  if (LAYER2_DEBUG(proto, CLI_ECHO))
    zlog_debug (mstpm, "no spanning-tree portfast bpdu-filter");

  if ((ifp == NULL) || (ifp->port == NULL))
    {
      cli_out (cli, "%% Can't find interface %s\n", ifp->name);
      return CLI_ERROR;
    }

  ret = mstp_api_set_port_bpdufilter (ifp->bridge_name, ifp->name,
                                      MSTP_PORT_PORTFAST_BPDUFILTER_DEFAULT);

  if (ret < 0)
    {
      cli_out (cli, "%% Can't set portfast bpdufilter for port\n");
      return CLI_ERROR;
    }

  return CLI_SUCCESS;

}

CLI (mstp_bridge_spanning_tree_forceversion,
     mstp_bridge_spanning_tree_forceversion_cmd,
     "bridge <1-32> spanning-tree force-version <0-3>",
     BRIDGE_STR,
     BRIDGE_NAME_STR,
     "spanning-tree" ,
     "force-version - Version of the protocol",
     "Version identifier - 0-  STP ,1- Not supported ,2 -RSTP, 3- MSTP")
  {
    int ret;
    u_int8_t *bridge_name;
    s_int32_t version = 0;
    struct mstp_bridge *br = NULL;
    struct mstp_bridge_info *br_config_info = NULL;
    struct mstp_bridge_config_list *br_conf_list = NULL;

    if (LAYER2_DEBUG(proto, CLI_ECHO))
      {
        if (argc == 2)
          zlog_debug (mstpm, "bridge %s spanning-tree force-version %s",
                      argv[0], argv[2]);
        else
          zlog_debug (mstpm, "spanning-tree force-version %s", argv[0]);
      }

    if (argc > 1)
      {
        bridge_name = argv[0];
        CLI_GET_INTEGER_RANGE ("force-version", version, argv[2], 0, 3);
      }
    else
      {
        bridge_name = def_bridge_name;
        CLI_GET_INTEGER_RANGE ("force-version", version, argv[0], 0, 3);
      }


    if (version == BR_VERSION_INVALID)
      {
        cli_out (cli, "%% Version %u not supported\n", version);
        return CLI_ERROR;
      }

    br =  (struct mstp_bridge *)mstp_find_bridge (bridge_name);
    if (br == NULL)
      {
        br_config_info = mstp_bridge_config_info_get (bridge_name);

        if (!br_config_info)
          {
            br_conf_list = mstp_bridge_config_list_new (bridge_name);
              if (br_conf_list == NULL)
                {
                  cli_out (cli, "%% Could not allocate memory for bridge config\n");
                  return CLI_ERROR;
                }
              mstp_bridge_config_list_link (br_conf_list);
              br_config_info = &(br_conf_list->br_info);
          }

        br_config_info->force_version = version;
        SET_FLAG (br_config_info->config, MSTP_BRIDGE_CONFIG_FORCE_VERSION);
        return CLI_SUCCESS;
      }

    ret = mstp_api_set_bridge_forceversion (br->name, version);
    if (ret < 0)
      {
        cli_out (cli, "%% Can't configure version for bridge %s\n",
                 br->name);
        return CLI_ERROR;
      }

    return CLI_SUCCESS;
  }

ALI (mstp_bridge_spanning_tree_forceversion,
     mstp_spanning_tree_forceversion_cmd,
     "spanning-tree force-version <0-3>",
     SPANNING_STR,
     "force-version - Version of the protocol",
     "Version identifier - 0-  STP ,1- Not supported ,2 -RSTP, 3- MSTP");

CLI (no_mstp_bridge_spanning_tree_forceversion,
     no_mstp_bridge_spanning_tree_forceversion_cmd,
     "no bridge <1-32> spanning-tree force-version",
     CLI_NO_STR,
     BRIDGE_STR,
     BRIDGE_NAME_STR,
     "spanning-tree" ,
     "force-version - Version of the protocol")
  {
    int ret;
    struct mstp_bridge *br;
    u_int8_t *bridge_name;
    u_int8_t force_version;

    if (LAYER2_DEBUG(proto, CLI_ECHO))
      {
        if (argc == 2)
          zlog_debug (mstpm, "no bridge %s spanning-tree force-version",
                      argv[0]);
        else
          zlog_debug (mstpm, "no spanning-tree force-version");
      }

    if (argc > 1)
      {
        bridge_name = argv [0];
      }
    else
      {
        bridge_name = def_bridge_name;
      }

    br =  (struct mstp_bridge *)mstp_find_bridge (bridge_name);

    if (br == NULL)
      {
        cli_out (cli, "%% Bridge not found\n");
        return CLI_ERROR;
      }

    /* Set force version based on bridge type */
    if (br->type == MSTP_BRIDGE_TYPE_STP
        || br->type == MSTP_BRIDGE_TYPE_STP_VLANAWARE)
       force_version = BR_VERSION_STP;
    else if (br->type == MSTP_BRIDGE_TYPE_RSTP
             || br->type == MSTP_BRIDGE_TYPE_RSTP_VLANAWARE
             || br->type == MSTP_BRIDGE_TYPE_RPVST_PLUS
             || br->type == MSTP_BRIDGE_TYPE_PROVIDER_RSTP)
       force_version = BR_VERSION_RSTP;
    else
       force_version = BR_VERSION_MSTP;


    ret = mstp_api_set_bridge_forceversion (br->name,
                                            force_version);
    if (ret < 0)
      {
        cli_out (cli, "%% Can't configure version for bridge %s\n",
                 br->name);
        return CLI_ERROR;
      }
  return CLI_SUCCESS;

  }

ALI (no_mstp_bridge_spanning_tree_forceversion,
     no_mstp_spanning_tree_forceversion_cmd,
     "no spanning-tree force-version",
     CLI_NO_STR,
     "spanning-tree" ,
     "force-version - Version of the protocol");

#ifdef HAVE_MSTPD
/* mst configuration commands */
CLI (mst_config,
     mst_config_cmd,
     "spanning-tree mst configuration",
     "Spanning tree commands",
     "Multiple spanning tree",
     "Configuration")
{
  if (LAYER2_DEBUG(proto, CLI_ECHO))
    zlog_debug (mstpm, "spanning-tree mst configuration");

  cli->mode = MST_CONFIG_MODE;

  return CLI_SUCCESS;
}


CLI (bridge_region,
     bridge_region_cmd,
     "bridge <1-32> region REGION_NAME",
     BRIDGE_STR,
     BRIDGE_NAME_STR,
     "MST Region",
     "Name of the region")
{
  int brno;
  u_int8_t *bridge_name;
  struct mstp_bridge *br = NULL;
  struct mstp_bridge_info *br_config_info = NULL;
  struct mstp_bridge_config_list *br_conf_list = NULL;


  if (LAYER2_DEBUG(proto, CLI_ECHO))
    {
      if (argc == 2)
         zlog_debug (mstpm, "bridge %s region%s", argv[0],argv[1]);
      else
         zlog_debug (mstpm, "region %s", argv[0]);
    }

  if (pal_strlen(argv[1]) > MSTP_CONFIG_NAME_LEN)
    {
      cli_out (cli, "%% region cannot be greater than 32 characters\n");
      return CLI_ERROR;
    }

  MSTP_GET_BR_NAME (2, 0);

  br = (struct mstp_bridge *)mstp_find_bridge (bridge_name);

  if (br != NULL && ! IS_BRIDGE_MSTP (br))
    {
      cli_out (cli, "%% Bridge %s is not configured as MSTP\n", bridge_name);
      return CLI_ERROR;
    } 

  if (br == NULL)
    {
      br_config_info = mstp_bridge_config_info_get (bridge_name);

      if (!br_config_info)
        {
          br_conf_list = mstp_bridge_config_list_new (bridge_name);

          if (br_conf_list == NULL)
            {
              cli_out (cli, "%% Could not allocate memory for bridge config\n");
              return CLI_ERROR;
            }
          mstp_bridge_config_list_link (br_conf_list);
          br_config_info = &(br_conf_list->br_info);
        }

      pal_strncpy (br_config_info->cfg_name, argc == 2 ? argv[1]:argv[0],
                   MSTP_CONFIG_NAME_LEN);
      SET_FLAG (br_config_info->config, MSTP_BRIDGE_CONFIG_REGION_NAME);
      return CLI_SUCCESS;
    }

  if (mstp_api_region_name (bridge_name, argc == 2 ? argv[1] : argv [0])
                             != RESULT_OK)
    cli_out (cli, "Cannot create region \n"); 

  return CLI_SUCCESS;
} 

ALI (bridge_region,
     spanning_region_cmd,
     "region REGION_NAME",
     "MST Region",
     "Name of the region");

CLI (no_bridge_region,
     no_bridge_region_cmd,
     "no bridge <1-32> region",
     CLI_NO_STR,
     BRIDGE_STR,
     BRIDGE_NAME_STR,
     "MST Region")
{
  int brno;
  struct mstp_bridge *br = NULL;
  u_int8_t *bridge_name;

  MSTP_GET_BR_NAME (1, 0);
 
  br =  (struct mstp_bridge *)mstp_find_bridge (bridge_name);

  if (br == NULL)
    {
      cli_out (cli, "%% bridge not configured\n");
      return CLI_ERROR;
    }

  if (! IS_BRIDGE_MSTP (br))
    {
      cli_out (cli, "%% Bridge %s is not configured as MSTP\n", bridge_name);
      return CLI_ERROR;
    }

  if (mstp_api_region_name (bridge_name, "Default") != RESULT_OK)
    cli_out (cli, "Cannot delete region\n");

  return CLI_SUCCESS;
}

ALI (no_bridge_region,
     no_spanning_region_cmd,
     "no region REGION_NAME",
     CLI_NO_STR,
     "MST Region",
     "Name of the region");

CLI (bridge_revision,
     bridge_revision_cmd,
     "bridge <1-32> revision REVISION_NUM",
     BRIDGE_STR,
     BRIDGE_NAME_STR,
     "Revision  Number for configuration information",
     "Number 0-255")
{
  short rev_num;
  int brno;
  u_int8_t *bridge_name;
  struct mstp_bridge *br = NULL;
  struct mstp_bridge_info *br_config_info = NULL;
  struct mstp_bridge_config_list *br_conf_list = NULL;

  
  if (LAYER2_DEBUG(proto, CLI_ECHO))
    {
      if (argc == 2)
        zlog_debug (mstpm, "bridge %s revision %s", argv[0],argv[1]);
      else
        zlog_debug (mstpm, "revision %s", argv[0]);
    }

  MSTP_GET_BR_NAME (2, 0);

  if (argc == 2)
    CLI_GET_INTEGER_RANGE ("revision", rev_num, argv[1],
                           0,255);
  else
    CLI_GET_INTEGER_RANGE ("revision", rev_num, argv[0],
                           0,255);

  br = (struct mstp_bridge *)mstp_find_bridge (bridge_name);

  if (br != NULL && ! IS_BRIDGE_MSTP (br))
    {
      cli_out (cli, "%% Bridge %s is not configured as MSTP\n", argv [0]);
      return CLI_ERROR;
    }
  
  if (br == NULL)
    {
      br_config_info = mstp_bridge_config_info_get (bridge_name);

      if (!br_config_info)
        {
          br_conf_list = mstp_bridge_config_list_new (bridge_name);

          if (br_conf_list == NULL)
            {
              cli_out (cli, "%% Could not allocate memory for bridge config\n");
              return CLI_ERROR;
            }
          mstp_bridge_config_list_link (br_conf_list);
          br_config_info = &(br_conf_list->br_info);
        }

      br_config_info->cfg_revision_lvl = rev_num;
      SET_FLAG (br_config_info->config, MSTP_BRIDGE_CONFIG_REVISION_LEVEL);
      return CLI_SUCCESS;
    }

  if (mstp_api_revision_number (bridge_name, rev_num) != RESULT_OK)
    {
      cli_out (cli, "Cannot change revision \n"); 
      return CLI_ERROR;
    }
  return CLI_SUCCESS;
} 

ALI (bridge_revision,
     spanning_revision_cmd,
     "revision REVISION_NUM",
     "Revision  Number for configuration information",
     "Number 0-255");

CLI (bridge_instance,
     bridge_instance_cmd,
     "bridge (<1-32> | backbone) instance <1-63>",
     BRIDGE_STR,
     BRIDGE_NAME_STR,
     BACKBONE_BRIDGE_NAME,       
     INSTANCE_STR,
     INSTANCE_ID_STR)
{
  int brno;
  u_int16_t vid = 0;
  s_int32_t instance;
  u_int8_t *bridge_name;
  struct mstp_bridge *br;
  int ret;

  if (LAYER2_DEBUG(proto, CLI_ECHO))
    {
      if (argc == 2)
        zlog_debug (mstpm, "bridge %s instance %s", argv[0],argv[1]);
      else
        zlog_debug (mstpm, "instance %s", argv[0]);
    }

  MSTP_GET_BR_NAME (2, 0);


  if (argc == 2)
    CLI_GET_INTEGER_RANGE ("instance", instance, argv[1], 1, 63);
  else
    CLI_GET_INTEGER_RANGE ("instance", instance, argv[0], 1, 63);

  br = mstp_find_bridge (bridge_name);

  if ((br != NULL) && (! IS_BRIDGE_MSTP (br)))
    {
      cli_out (cli, "%% Bridge %s is not configured as MSTP\n", bridge_name);
      return CLI_ERROR;
    }

  ret = mstp_api_add_instance (bridge_name,instance, vid, 0);
  if (ret != RESULT_OK)
    {
      if (ret == MSTP_INSTANCE_IN_USE_ERR)
        {
          cli_out (cli, "%% Instance is in use\n");
          return CLI_ERROR;
        }

      mstp_bridge_instance_config_add (bridge_name, instance, vid);
      cli_out (cli, "Cannot create instance \n");
      return CLI_ERROR;
    }

  return CLI_SUCCESS;
}

ALI (bridge_instance,
     spanning_instance_cmd,
     "instance <1-63>",
     INSTANCE_STR,
     INSTANCE_ID_STR);

CLI (bridge_instance_vlan,
     bridge_instance_vlan_cmd,
     "bridge (<1-32> | backbone) instance <1-63> vlan VLANID",
     BRIDGE_STR,
     BRIDGE_NAME_STR,
     BACKBONE_BRIDGE_NAME,
     INSTANCE_STR,
     INSTANCE_ID_STR,
     "vlan",
     "vlan id to be associated to instance")
{

  int ret;
  int brno;
  char *curr;
  u_int16_t vid = 0;
  s_int32_t instance = 0;
  struct mstp_vlan *v;
  u_int8_t *bridge_name;
  l2_range_parser_t par;
  struct mstp_bridge *br;
  u_int8_t *vlan_arg = NULL;
  l2_range_parse_error_t r_err;

  if (LAYER2_DEBUG(proto, CLI_ECHO))
    {
      if (argc == 3)
        zlog_debug (mstpm, "bridge %s instance %s vlan %s",
                    argv[0], argv[1], argv[2]);
      else if (argc == 2)
        zlog_debug (mstpm, "instance %s vlan %s",
                    argv[0], argv[1]);;
    }

  MSTP_GET_BR_NAME (3, 0);
  curr = NULL;

  if (argc == 3)
    {
      CLI_GET_INTEGER_RANGE ("instance", instance, argv[1], 1, 63);
      vlan_arg = argv [2];
    }
  else
    {
      CLI_GET_INTEGER_RANGE ("instance", instance, argv[0], 1, 63);
      vlan_arg = argv [1];
      CLI_GET_INTEGER_RANGE ("vlan", vid, argv[1], 1, 4094);
    }

  br = mstp_find_bridge (bridge_name);

  if (br != NULL && ! IS_BRIDGE_MSTP (br))
    {
      cli_out (cli, "%% Bridge %s is not configured as MSTP\n", bridge_name);
      return CLI_ERROR;
    }

  l2_range_parser_init (vlan_arg, &par);

  if ((ret = l2_range_parser_validate (&par, cli,
                                       mstp_bridge_vlan_validate_vid))
       != CLI_SUCCESS)
    return ret;

  while ((curr = l2_range_parser_get_next (&par, &r_err)))
    {
      if (r_err != RANGE_PARSER_SUCCESS)
        {
          cli_out (cli, "%% Invalid Input %s \n",
                   l2_range_parse_err_to_str (r_err) );
          return CLI_ERROR;
        }

      CLI_GET_INTEGER_RANGE ("vlan", vid, curr, 1, 4094);

      v = mstp_bridge_vlan_lookup (bridge_name, vid);

      if (v == NULL)
        {
          ret = mstp_bridge_vlan_add2 (bridge_name, vid, instance,
                                       MSTP_VLAN_INSTANCE_CONFIGURED);
          if (ret < 0)
            {
              mstp_bridge_instance_config_add (bridge_name, instance, vid);
              cli_out (cli, "%% Cannot create instance \n");
              return CLI_ERROR;
            }

          continue;
        }
#if defined  HAVE_G8031 || defined HAVE_G8032
      else if (CHECK_FLAG(v->flags, MSTP_VLAN_PG_CONFIGURED)) 
        {
          cli_out (cli, " Vlan %d is being used for protection \n", vid);
          return CLI_ERROR;
        }
#endif /* HAVE_G8031 */
  
      ret = mstp_api_add_instance (bridge_name,instance, vid, 0);

      if (ret != RESULT_OK)
        {
          if (ret == MSTP_INSTANCE_IN_USE_ERR)
            {
              cli_out (cli, "%% Instance is in use\n");
              return CLI_ERROR;
            }

          mstp_bridge_instance_config_add (bridge_name, instance, vid);
          cli_out (cli, "Cannot create instance \n");
          return CLI_ERROR;
        }
    }

  l2_range_parser_deinit (&par);
  return CLI_SUCCESS;
}

ALI (bridge_instance_vlan,
     spanning_instance_vlan_cmd,
     "instance <1-63> vlan VLANID",
     INSTANCE_STR,
     INSTANCE_ID_STR,
     "vlan",
     "vlan id to be associated to instance");

CLI (no_bridge_instance,
     no_bridge_instance_cmd,
     "no bridge (<1-32> | backbone) instance <1-63>",
     CLI_NO_STR,
     BRIDGE_STR,
     BRIDGE_NAME_STR,
     BACKBONE_BRIDGE_NAME,
     INSTANCE_STR,
     INSTANCE_ID_STR)
{
  int brno;
  int instance;
  u_int8_t *bridge_name;

  if (LAYER2_DEBUG(proto, CLI_ECHO))
    {
      if (argc == 2)
        zlog_debug (mstpm, "no bridge %s instance %s", argv[0],argv[1]);
      else
        zlog_debug (mstpm, "no instance %s", argv[0]);
    }

  MSTP_GET_BR_NAME (2, 0);


  if (argc == 2)
    CLI_GET_INTEGER_RANGE ("instance", instance, argv[1], 1, 63);
  else
    CLI_GET_INTEGER_RANGE ("instance", instance, argv[0], 1, 63);

  /* Delete this instance from VLAN config if present. */
  mstp_bridge_vlan_instance_delete (bridge_name, instance);

  if (mstp_api_delete_instance (bridge_name, instance) != RESULT_OK)
    {
      cli_out (cli, "Cannot delete instance \n");
    }

  mstp_bridge_instance_config_delete (bridge_name, instance,
                                      MSTP_VLAN_NULL_VID);

  return CLI_SUCCESS;

}

ALI (no_bridge_instance,
     no_spanning_instance_cmd,
     "no instance <1-63>",
     CLI_NO_STR,
     INSTANCE_STR,
     INSTANCE_ID_STR);

CLI (no_bridge_instance_vlan,
     no_bridge_instance_vlan_cmd,
     "no bridge (<1-32> | backbone) instance <1-63> vlan VLANID",
     CLI_NO_STR,
     BRIDGE_STR,
     BRIDGE_NAME_STR,
     BACKBONE_BRIDGE_NAME,
     INSTANCE_STR,
     INSTANCE_ID_STR,
     "Delete the association of vlan with this instance",
     "vlanid associated with instance")
{
  int ret;
  int brno;
  char *curr;
  int instance;
  u_int16_t vid;
  struct mstp_vlan *v;
  u_int8_t *bridge_name;
  l2_range_parser_t par;
  u_int8_t *vlan_arg = NULL;
  l2_range_parse_error_t r_err;


  if (LAYER2_DEBUG(proto, CLI_ECHO))
    {
      if (argc == 3)
        zlog_debug (mstpm, "no bridge %s instance %s vlan %s",
                    argv[0], argv[1], argv[2]);
      else
        zlog_debug (mstpm, "no instance %s vlan %s",
                    argv[0], argv[1]);
   }

  MSTP_GET_BR_NAME (3, 0);
  curr = NULL;

  if (argc == 3)
    {
      CLI_GET_INTEGER_RANGE ("instance", instance, argv[1], 1, 63);
      vlan_arg = argv [2];
    }
  else
    {
      CLI_GET_INTEGER_RANGE ("instance", instance, argv[0], 1, 63);
      vlan_arg = argv [1];
    }

  l2_range_parser_init (vlan_arg, &par);

  if ((ret = l2_range_parser_validate (&par, cli,
                                       mstp_bridge_vlan_validate_vid))
      != CLI_SUCCESS)
    return ret;

  while ((curr = l2_range_parser_get_next (&par, &r_err)))
    {
      if (r_err != RANGE_PARSER_SUCCESS)
        {
          cli_out (cli, "%% Invalid Input %s \n",
                   l2_range_parse_err_to_str (r_err) );
          return CLI_ERROR;
        }

      CLI_GET_INTEGER_RANGE ("vlan", vid, curr, 1, 4094);

      v = mstp_bridge_vlan_lookup (bridge_name, vid);

      if (! v)
        {
          cli_out (cli, "VLAN not found\n");
          return CLI_ERROR;
        }

#ifdef HAVE_PVLAN
      if (v->pvlan_type == MSTP_PVLAN_SECONDARY)
        {
          cli_out (cli, "cannot delete instance \n");
          return CLI_ERROR;
        }
#endif /* HAVE_PVLAN */

      if (CHECK_FLAG (v->flags, MSTP_VLAN_INSTANCE_CONFIGURED))
        {
          /* Instance not alive. Delete VLAN. */
          mstp_bridge_vlan_delete (bridge_name, vid);

          mstp_bridge_instance_config_delete (bridge_name, instance,
                                              vid);

          return CLI_SUCCESS;
        }

      if (mstp_api_delete_instance_vlan (bridge_name, instance, vid) != RESULT_OK)
        {
          cli_out (cli, "Cannot delete instance\n");
          return CLI_ERROR;
        }

    }

  l2_range_parser_deinit (&par);

  return CLI_SUCCESS;

}

ALI (no_bridge_instance_vlan,
     no_spanning_instance_vlan_cmd,
     "no instance <1-63> vlan VLANID",
     CLI_NO_STR,
     INSTANCE_STR,
     INSTANCE_ID_STR,
     "Delete the association of vlan with this instance",
     "vlanid associated with instance");

#endif /* HAVE_MSTPD */

#ifdef HAVE_PROVIDER_BRIDGE

CLI (mstp_cus_bridge_priority,
     mstp_cus_bridge_priority_cmd,
     "customer-spanning-tree priority <0-61440>",
     CUS_SPANNING_STR,
     "priority - bridge priority for the common instance",
     "bridge priority in increments of 4096 (Lower priority ",
     "indicates greater likelihood of becoming root)")
{
  int ret;
  u_int32_t priority;
  u_int8_t *str = NULL;
  struct mstp_bridge *br = NULL;
  struct interface *ifp = cli->index;
  u_int8_t bridge_name [L2_BRIDGE_NAME_LEN+1];
  struct mstp_bridge_info *br_config_info = NULL;
  struct mstp_bridge_config_list *br_conf_list = NULL;

  if (LAYER2_DEBUG(proto, CLI_ECHO))
    zlog_debug (mstpm, "customer-spanning-tree priority %s", argv[0]);

  BRIDGE_GET_CE_BR_NAME (ifp->ifindex);

  CLI_GET_UINT32_RANGE ("priority", priority, argv[0], 0, 61440);

  br =  (struct mstp_bridge *) mstp_find_bridge (bridge_name);

  if (br == NULL)
    {
      br_config_info = mstp_bridge_config_info_get (bridge_name);

      if (!br_config_info)
        {
          br_conf_list = mstp_bridge_config_list_new (bridge_name);

          if (br_conf_list == NULL)
            {
              cli_out (cli, "%% Could not allocate memory for bridge config\n");
              return CLI_ERROR;
            }

          mstp_bridge_config_list_link (br_conf_list);
          br_config_info = &(br_conf_list->br_info);
        }

      br_config_info->priority = priority;
      SET_FLAG (br_config_info->config, MSTP_BRIDGE_CONFIG_PRIORITY);
      return CLI_SUCCESS;
    }

  ret = mstp_api_set_bridge_priority (bridge_name, priority);

  if (ret < 0)
    switch (ret)
      {
        case MSTP_ERR_BRIDGE_NOT_FOUND :
          str = "Bridge not found";
          break;
        case MSTP_ERR_PRIORITY_VALUE_WRONG :
          str = "The priority value must be in multiples of 4096";
          break;
        case MSTP_ERR_PRIORITY_OUTOFBOUNDS :
          str = "The priority value is out of bounds";
          break;
        default :
          str = "Can't set priority";
          break;
      }

  if (str)
    {
      cli_out (cli, "%% %s\n", str);
      return CLI_ERROR;
    }

  return CLI_SUCCESS;
}

CLI (no_mstp_cus_bridge_priority,
     no_mstp_cus_bridge_priority_cmd,
     "no customer-spanning-tree priority",
     CLI_NO_STR,
     CUS_SPANNING_STR,
     "priority - bridge priority for the common instance")
{
  int ret;
  u_int8_t *str = NULL;
  struct interface *ifp = cli->index;
  u_int8_t bridge_name [L2_BRIDGE_NAME_LEN+1];

  if (LAYER2_DEBUG(proto, CLI_ECHO))
    zlog_debug (mstpm, "no customer-spanning-tree priority");

  BRIDGE_GET_CE_BR_NAME (ifp->ifindex);

  ret = mstp_api_set_bridge_priority (bridge_name, CE_BRIDGE_DEFAULT_PRIORITY);

  if (ret < 0)
    switch (ret)
      {
        case MSTP_ERR_BRIDGE_NOT_FOUND :
          str = "Bridge not found";
          break;
        case MSTP_ERR_PRIORITY_VALUE_WRONG :
          str = "The priority value must be in multiples of 4096";
          break;
        case MSTP_ERR_PRIORITY_OUTOFBOUNDS :
          str = "The priority value is out of bounds";
          break;
        default :
          str = "Can't set priority";
          break;
      }

  if (str)
    {
      cli_out (cli, "%% %s\n", str);
      return CLI_ERROR;
    }

  return CLI_SUCCESS;
}

CLI (mstp_cus_bridge_forward_time,
     mstp_cus_bridge_forward_time_cmd,
     "customer-spanning-tree forward-time <4-30>",
     CUS_SPANNING_STR,
     "forward-time - forwarding delay time",
     "forward delay time in seconds")
{
  int ret;
  s_int32_t forward_delay;
  struct mstp_bridge *br = NULL;
  struct interface *ifp = cli->index;
  u_int8_t bridge_name [L2_BRIDGE_NAME_LEN+1];
  struct mstp_bridge_info *br_config_info = NULL;
  struct mstp_bridge_config_list *br_conf_list = NULL;

  if (LAYER2_DEBUG(proto, CLI_ECHO))
    zlog_debug (mstpm, "customer-spanning-tree forward-time %s", argv[0]);

  BRIDGE_GET_CE_BR_NAME (ifp->ifindex);
  CLI_GET_INTEGER_RANGE ("forward-time", forward_delay, argv[0], 4, 30);

  br =  (struct mstp_bridge *)mstp_find_bridge (bridge_name);

  if (br == NULL)
    {

      br_config_info = mstp_bridge_config_info_get (bridge_name);

      if (!br_config_info)
        {
          br_conf_list = mstp_bridge_config_list_new (bridge_name);

          if (br_conf_list == NULL)
            {
              cli_out (cli, "%% Could not allocate memory for bridge config\n");
              return CLI_ERROR;
            }

          mstp_bridge_config_list_link (br_conf_list);
          br_config_info = &(br_conf_list->br_info);
        }

      br_config_info->forward_time = forward_delay;
      SET_FLAG (br_config_info->config, MSTP_BRIDGE_CONFIG_FORWARD_TIME);
      return CLI_SUCCESS;
    }

  ret = mstp_api_set_forward_delay (bridge_name, forward_delay);

  if (ret < 0)
    {
      cli_out (cli, "%% Can't set forward-time\n");
      return CLI_ERROR;
    }

  return CLI_SUCCESS;
}

CLI (no_mstp_cus_bridge_forward_time,
     no_mstp_cus_bridge_forward_time_cmd,
     "no customer-spanning-tree forward-time",
     CLI_NO_STR,
     CUS_SPANNING_STR,
     "forward-time - forwarding delay time")
{
  int ret;
  struct interface *ifp = cli->index;
  u_int8_t bridge_name [L2_BRIDGE_NAME_LEN+1];

  if (LAYER2_DEBUG(proto, CLI_ECHO))
    zlog_debug (mstpm, "no customer-spanning-tree forward-time");

  BRIDGE_GET_CE_BR_NAME (ifp->ifindex);

  ret = mstp_api_set_forward_delay (bridge_name,
                                    BRIDGE_TIMER_DEFAULT_FWD_DELAY);

  if (ret < 0)
    {
      cli_out (cli, "%% Can't set forward-time\n");
      return CLI_ERROR;
    }

  return CLI_SUCCESS;
}

CLI (mstp_cus_bridge_hello_time,
     mstp_cus_bridge_hello_time_cmd,
     "customer-spanning-tree hello-time <1-10>",
     CUS_SPANNING_STR,
     "hello-time - hello BDPU interval",
     "seconds <1-10> - Hello BPDU interval")
{
  int ret;
  s_int32_t hello_time;
  struct mstp_bridge *br = NULL;
  struct interface *ifp = cli->index;
  u_int8_t bridge_name [L2_BRIDGE_NAME_LEN+1];
  struct mstp_bridge_info *br_config_info = NULL;
  struct mstp_bridge_config_list *br_conf_list = NULL;

  if (LAYER2_DEBUG(proto, CLI_ECHO))
    zlog_debug (mstpm, "spanning-tree hello-time %s", argv[0]);

  BRIDGE_GET_CE_BR_NAME (ifp->ifindex);

  CLI_GET_INTEGER_RANGE ("hello-time", hello_time, argv[0], 1, 10);

  br =  (struct mstp_bridge *)mstp_find_bridge (bridge_name);

  if (br == NULL)
    {
      br_config_info = mstp_bridge_config_info_get (bridge_name);

      if (!br_config_info)
        {

          br_conf_list = mstp_bridge_config_list_new (bridge_name);

          if (br_conf_list == NULL)
            {
              cli_out (cli, "%% Could not allocate memory for bridge config\n");
              return CLI_ERROR;
            }

          mstp_bridge_config_list_link (br_conf_list);
          br_config_info = &(br_conf_list->br_info);
        }

      br_config_info->hello_time = hello_time;
      SET_FLAG (br_config_info->config, MSTP_BRIDGE_CONFIG_HELLO_TIME);
      cli_out (cli, "%% Can't set hello-time\n");
      return CLI_ERROR;
    }

  ret = mstp_api_set_hello_time (bridge_name, hello_time);

  if (ret < 0)
    {
      cli_out (cli, "%% Can't set hello-time\n");
      return CLI_ERROR;
    }

  return CLI_SUCCESS;
}

CLI (no_mstp_cus_bridge_hello_time,
     no_mstp_cus_bridge_hello_time_cmd,
     "no customer-spanning-tree hello-time",
     CLI_NO_STR,
     CUS_SPANNING_STR,
     "hello-time - hello BDPU interval")
{
  int ret;
  struct interface *ifp = cli->index;
  u_int8_t bridge_name [L2_BRIDGE_NAME_LEN+1];

  if (LAYER2_DEBUG(proto, CLI_ECHO))
    zlog_debug (mstpm, "no spanning-tree hello-time");

  BRIDGE_GET_CE_BR_NAME (ifp->ifindex);

  ret = mstp_api_set_hello_time (bridge_name, BRIDGE_TIMER_DEFAULT_HELLO_TIME);

  if (ret < 0)
    {
      cli_out (cli, "%% Can't set hello-time\n");
      return CLI_ERROR;
    }

  return CLI_SUCCESS;
}

CLI (mstp_cus_bridge_max_age,
     mstp_cus_bridge_max_age_cmd,
     "customer-spanning-tree max-age <6-40>",
     CUS_SPANNING_STR,
     "max-age",
     "seconds <6-40> - Maximum time to listen for root bridge in seconds")
{
  int ret;
  s_int32_t max_age;
  struct mstp_bridge *br = NULL;
  struct interface *ifp = cli->index;
  u_int8_t bridge_name [L2_BRIDGE_NAME_LEN+1];
  struct mstp_bridge_info *br_config_info = NULL;
  struct mstp_bridge_config_list *br_conf_list = NULL;

  BRIDGE_GET_CE_BR_NAME (ifp->ifindex);

  CLI_GET_INTEGER_RANGE ("max-age", max_age, argv[0], 6, 40);

  if (LAYER2_DEBUG(proto, CLI_ECHO))
    zlog_debug (mstpm, "spanning-tree max-age %s", argv[0]);

  br =  (struct mstp_bridge *)mstp_find_bridge (bridge_name);

  if (br == NULL)
    {
      br_config_info = mstp_bridge_config_info_get (bridge_name);

      if (!br_config_info)
        {
          br_conf_list = mstp_bridge_config_list_new (bridge_name);
          if (br_conf_list == NULL)
            {
              cli_out (cli, "%% Could not allocate memory for bridge config\n");
              return CLI_ERROR;
            }
          mstp_bridge_config_list_link (br_conf_list);
          br_config_info = &(br_conf_list->br_info);
        }

      br_config_info->max_age = max_age;
      SET_FLAG (br_config_info->config, MSTP_BRIDGE_CONFIG_MAX_AGE);
      cli_out (cli, "%% Can't set max-age\n");
      return CLI_ERROR;
    }

  ret = mstp_api_set_max_age (bridge_name, max_age);

  if (ret < 0)
    {
      cli_out (cli, "%% Can't set max-age\n");
      return CLI_ERROR;
    }
  return CLI_SUCCESS;
}

CLI (no_mstp_cus_bridge_max_age,
     no_mstp_cus_bridge_max_age_cmd,
     "no customer-spanning-tree max-age",
     CLI_NO_STR,
     CUS_SPANNING_STR,
     "max-age")
{
  int ret;
  struct interface *ifp = cli->index;
  u_int8_t bridge_name [L2_BRIDGE_NAME_LEN+1];

  BRIDGE_GET_CE_BR_NAME (ifp->ifindex);

  if (LAYER2_DEBUG(proto, CLI_ECHO))
    zlog_debug (mstpm, "no spanning-tree max-age");

  ret = mstp_api_set_max_age (bridge_name, BRIDGE_TIMER_DEFAULT_MAX_AGE);

  if (ret < 0)
    {
      cli_out (cli, "%% Can't set max-age\n");
      return CLI_ERROR;
    }

  return CLI_SUCCESS;
}

CLI (mstp_cus_bridge_transmit_hold_count,
     mstp_cus_bridge_transmit_hold_count_cmd,
     "customer-spanning-tree transmit-holdcount <1-10>",
     CUS_SPANNING_STR,
     "Transmit hold count of the bridge",
     "range of the transmitholdcount")
{
  int ret;
  unsigned char txholdcount;
  struct mstp_bridge *br = NULL;
  struct interface *ifp = cli->index;
  u_int8_t bridge_name [L2_BRIDGE_NAME_LEN+1];
  struct mstp_bridge_info *br_config_info = NULL;
  struct mstp_bridge_config_list *br_conf_list = NULL;

  BRIDGE_GET_CE_BR_NAME (ifp->ifindex);

  CLI_GET_INTEGER_RANGE ("Transmit hold count", txholdcount, argv[0], 1, 10);

  if (LAYER2_DEBUG(proto, CLI_ECHO))
    zlog_debug (mstpm, "customer-spanning-tree transmit-holdcount %s", argv[0]);

  br =  (struct mstp_bridge *)mstp_find_bridge (bridge_name);

  if (br == NULL)
    {
      br_config_info = mstp_bridge_config_info_get (bridge_name);

      if (!br_config_info)
        {
          br_conf_list = mstp_bridge_config_list_new (bridge_name);
          if (br_conf_list == NULL)
            {
              cli_out (cli, "%% Could not allocate memory for bridge config\n");
              return CLI_ERROR;
            }
           mstp_bridge_config_list_link (br_conf_list);
           br_config_info = &(br_conf_list->br_info);
         }

      br_config_info-> transmit_hold_count= txholdcount;
      SET_FLAG (br_config_info->config, MSTP_BRIDGE_CONFIG_TX_HOLD_COUNT);
      cli_out (cli, "%% Can't set Transmit hold count\n");
      return CLI_ERROR;
    }

  ret = mstp_api_set_transmit_hold_count (bridge_name, txholdcount);

  if (ret < 0)
    {
      cli_out (cli, "%% Can't set transmit hold count\n");
      return CLI_ERROR;
    }

  return CLI_SUCCESS;
}

CLI (no_mstp_cus_bridge_transmit_hold_count,
     no_mstp_cus_bridge_transmit_hold_count_cmd,
     "no customer-spanning-tree transmit-holdcount",
     CLI_NO_STR,
     CUS_SPANNING_STR,
     "Transmit hold count of the bridge")
{
  int ret;
  struct interface *ifp = cli->index;
  u_int8_t bridge_name [L2_BRIDGE_NAME_LEN+1];

  BRIDGE_GET_CE_BR_NAME (ifp->ifindex);

  if (LAYER2_DEBUG(proto, CLI_ECHO))
    zlog_debug (mstpm, "no customer-spanning-tree transmit-holdcount");

  ret = mstp_api_set_transmit_hold_count (bridge_name, MSTP_TX_HOLD_COUNT);

  if (ret < 0)
    {
      cli_out (cli, "%% Can't set transmit hold count\n");
      return CLI_ERROR;
    }

  return CLI_SUCCESS;
}

CLI (mstp_cus_bridge_group_priority,
     mstp_cus_bridge_group_priority_cmd,
     "customer-spanning-tree customer-edge priority <0-240>",
     CUS_SPANNING_STR,
     CE_PORT_STR,
     "port priority for a bridge",
     "port priority in increments of 16"
     " (lower priority indicates greater likelihood of becoming root)")
{
  int ret;
  u_int16_t svid;
  s_int16_t priority = 0;
  struct interface *ifp = cli->index;
  u_int8_t bridge_name [L2_BRIDGE_NAME_LEN + 1];

  if (LAYER2_DEBUG(proto, CLI_ECHO))
    {
      if (argc == 2)
        zlog_debug (mstpm, "customer-spanning-tree provider-edge "
                           "svlan %s priority %s",
                           argv[0], argv[1]);
      else
        zlog_debug (mstpm, "customer-spanning-tree customer-edge "
                            "priority %s",
                            argv[0]);
    }
  BRIDGE_GET_CE_BR_NAME (ifp->ifindex);

  if (argc == 2)
    {
      CLI_GET_INTEGER_RANGE ("SVLAN ID", svid, argv[0], 1, 4094);
      CLI_GET_INTEGER_RANGE ("priority", priority, argv[1], 0, 240);
    }
  else
    {
      svid = MSTP_VLAN_NULL_VID;
      CLI_GET_INTEGER_RANGE ("priority", priority, argv[0], 0, 240);
    }

  ret = mstp_api_set_port_priority (bridge_name, ifp->name, svid,
                                    priority);

  if (ret < 0)
    {
      cli_out (cli, "Unable to set priority of intance on the port \n");
      return CLI_ERROR;
    }

  return CLI_SUCCESS;
}

ALI (mstp_cus_bridge_group_priority,
     mstp_cus_pe_bridge_group_priority_cmd,
     "customer-spanning-tree provider-edge svlan <1-4094> priority <0-240",
     CUS_SPANNING_STR,
     PE_PORT_STR,
     PE_SVLAN_STR,
     PE_SVLAN_ID_STR,
     "port priority for a bridge",
     "port priority in increments of 16"
     " (lower priority indicates greater likelihood of becoming root)");

CLI (mstp_cus_bridge_group_path_cost,
     mstp_cus_bridge_group_path_cost_cmd,
     "customer-spanning-tree customer-edge path-cost <1-200000000>",
     CUS_SPANNING_STR,
     CE_PORT_STR,
     "path cost for a port",
     "path cost in range <1-200000000> (lower path cost indicates greater "
     "likelihood of becoming root)")
{
  int ret;
  u_int16_t svid;
  u_int32_t path_cost = 0;
  struct interface *ifp = cli->index;
  u_int8_t bridge_name [L2_BRIDGE_NAME_LEN + 1];

  if (LAYER2_DEBUG(proto, CLI_ECHO))
    {
      if (argc == 2)
        zlog_debug (mstpm, "customer-spanning-tree provider-edge "
                           "svlan %s path-cost %s",
                           argv[0], argv[1]);
      else
        zlog_debug (mstpm, "customer-spanning-tree customer-edge "
                            "path-cost %s", argv[0]);
    }

  BRIDGE_GET_CE_BR_NAME (ifp->ifindex);

  if (argc == 2)
    {
      CLI_GET_INTEGER_RANGE ("SVLAN ID", svid, argv[0], 1, 4094);
      CLI_GET_INTEGER_RANGE ("path-cost", path_cost, argv[1], 1, 200000000);
    }
  else
    {
      svid = MSTP_VLAN_NULL_VID;
      CLI_GET_INTEGER_RANGE ("path-cost", path_cost, argv[0], 1, 200000000);
    }

  ret = mstp_api_set_port_path_cost (bridge_name, ifp->name, svid, path_cost);

  if (ret < 0)
    {
      cli_out (cli, "Unable to set path cost on the port \n");
      return CLI_ERROR;
    }

  return CLI_SUCCESS;
}

ALI (mstp_cus_bridge_group_path_cost,
     mstp_cus_pe_bridge_group_path_cost_cmd,
     "customer-spanning-tree provider-edge svlan <1-4094> "
     "path-cost <1-200000000>",
     CUS_SPANNING_STR,
     PE_PORT_STR,
     PE_SVLAN_STR,
     PE_SVLAN_ID_STR,
     "path cost for a port",
     "path cost in range <1-200000000> (lower path cost indicates greater "
     "likelihood of becoming root)");

CLI (no_mstp_cus_bridge_group_path_cost,
     no_mstp_cus_bridge_group_path_cost_cmd,
     "no customer-spanning-tree customer-edge path-cost",
     CLI_NO_STR,
     CUS_SPANNING_STR,
     CE_PORT_STR,
     "path cost for a port",
     "path cost in range <1-200000000> (lower path cost indicates greater "
     "likelihood of becoming root)")
{
  int ret;
  u_int16_t svid;
  struct interface *ifp = cli->index;
  u_int8_t bridge_name [L2_BRIDGE_NAME_LEN + 1];

  if (LAYER2_DEBUG(proto, CLI_ECHO))
    {
      if (argc == 1)
        zlog_debug (mstpm, "no customer-spanning-tree provider-edge "
                           "svlan %s path-cost", argv[0]);
      else
        zlog_debug (mstpm, "no customer-spanning-tree customer-edge "
                            "path-cost");
    }

  BRIDGE_GET_CE_BR_NAME (ifp->ifindex);

  if (argc == 1)
    CLI_GET_INTEGER_RANGE ("SVLAN ID", svid, argv[0], 1, 4094);
  else
    svid = MSTP_VLAN_NULL_VID;

  ret = mstp_api_set_port_path_cost (bridge_name, ifp->name, svid,
                                     mstp_nsm_get_port_path_cost (mstpm,
                                                                  ifp->ifindex));
  if (ret < 0)
    {
      cli_out (cli, "Unable to reset path cost on the port \n");
      return CLI_ERROR;
    }

  return CLI_SUCCESS;
}

ALI (no_mstp_cus_bridge_group_path_cost,
     no_mstp_cus_pe_bridge_group_path_cost_cmd,
     "no customer-spanning-tree provider-edge svlan <1-4094> path-cost",
     CUS_SPANNING_STR,
     PE_PORT_STR,
     PE_SVLAN_STR,
     PE_SVLAN_ID_STR,
     "path cost for a port",
     "path cost in range <1-200000000> (lower path cost indicates greater "
     "likelihood of becoming root)");



#endif /* HAVE_PROVIDER_BRIDGE */

#ifdef HAVE_L2GP

CLI (mstp_add_l2gp,
     mstp_add_l2gp_cmd,
     "switchport l2gp  psuedoRootId ROOTID:MAC",
     "Set the l2gp characteristics of the Layer2 interface",
     "l2gp",
     "puedoInfo priority/bridge mac address",
     "priority/mac-address 12345/0000.0000.0000")
{
   int ret=0, i=0;
   struct interface *ifp = cli->index;
   struct mstp_interface *mstpif = ifp->info;
   struct bridge_id psuedoRootId;
   int enableBPDUrx = PAL_TRUE;
   uint32_t psuedoBridgeId;
   uint8_t buf[20];
   struct mstp_bridge *br;
   struct mstp_port *port;

   pal_assert(ifp);
   pal_assert(mstpif);

   if(LAYER2_DEBUG(proto,CLI_ECHO))
        zlog_debug(mstpm, "spanning-tree l2gp");

   if ( argv[0] == NULL )
     {
       cli_out(cli,"%%no input for psuedoRootId \n");
       return CLI_ERROR; 
     }

   pal_mem_cpy((void *)buf,(void *)argv[0],pal_strlen(argv[0]));
   pal_mem_set(&psuedoRootId,0,sizeof(psuedoRootId));


    /* extracting macaddress */
   while(buf[i++] != '/'); 

    /* extracting psuedo bridge id */
   psuedoBridgeId = pal_atoi(pal_strtok(buf,"/"));

   if ((psuedoBridgeId % MSTP_BRIDGE_PRIORITY_MULTIPLIER) != 0 )
     {
       if ( psuedoBridgeId > MSTP_MAX_BRIDGE_PRIORITY )
         {
            cli_out (cli, "%% priority should be lower than %d \n",
                              MSTP_MAX_BRIDGE_PRIORITY );
            return CLI_ERROR;
         }
       cli_out (cli, "%% priority should be in multiples of 4096 \n");
       return CLI_ERROR;
     }

   if (ifp == NULL)
     {
        cli_out(cli,"%% Can't find interface %s\n",ifp->name);
        return CLI_ERROR;
     }

   if (pal_sscanf (&buf[i],"%4hx.%4hx.%4hx", (unsigned short *)&psuedoRootId.addr[0],
                (unsigned short *)&psuedoRootId.addr[2],
                (unsigned short *)&psuedoRootId.addr[4]) != 3)
     {
         cli_out (cli, "%% Unable to translate mac address %s\n", &buf[i]);
         return CLI_ERROR;
     }

   /* Convert to network order */
   *(unsigned short *)&psuedoRootId.addr[0] = pal_hton16(*(unsigned short *) \
                                               &psuedoRootId.addr[0]);
   *(unsigned short *)&psuedoRootId.addr[2] = pal_hton16(*(unsigned short *) \
                                               &psuedoRootId.addr[2]);
   *(unsigned short *)&psuedoRootId.addr[4] = pal_hton16(*(unsigned short *) \
                                               &psuedoRootId.addr[4]);

    /*
     * setting enableBPDUrx by default
     */
   enableBPDUrx = PAL_TRUE;

   br = mstp_find_bridge (ifp->bridge_name);
   if (!br)
     {
       if (if_lookup_by_name (&cli->vr->ifm, ifp->name) == NULL)
         {
	   pal_strcpy (mstpif->bridge_group, ifp->bridge_name);
	   mstpif->isL2gp = PAL_TRUE;
	   mstpif->enableBPDUrx = enableBPDUrx;
	   mstp_put_prio_in_bridgeid(&psuedoRootId,(u_int16_t)psuedoBridgeId);
	   pal_mem_cpy (&mstpif->psuedoRootId, &psuedoRootId,
		   sizeof(struct bridge_id));
	   SET_FLAG (mstpif->config, MSTP_IF_CONFIG_L2GP_PORT);
	   cli_out (cli, " %% Associate the interface to the bridge %s\n", 
		   ifp->bridge_name);
	   return CLI_SUCCESS;
        }
    }

   port = mstp_find_cist_port (br, ifp->ifindex);
   if (!port)
     {
       cli_out(cli,"%%can not find port %s \n", ifp->name);
       return CLI_ERROR;
     }

   if (port->isL2gp)
     {
       unsigned short prio=0;
       bcopy((void *)port->psuedoRootId.prio,(void *)&prio, 2);

       if ((!pal_mem_cmp(&psuedoRootId.addr,&port->psuedoRootId.addr,
		       ETHER_ADDR_LEN) && 
		   (pal_hton16(prio) == (u_int16_t)psuedoBridgeId )))
           {
              cli_out(cli,"%%%s already an l2gp port \n", ifp->name);
              return CLI_ERROR;
           }
           /*
            * else case is for, given input differs from the existing
            * l2gp configuration. such cases, update the given
            * configuration
            */ 
     }

   mstp_put_prio_in_bridgeid(&psuedoRootId,(u_int16_t)psuedoBridgeId);

   ret = mstp_api_set_l2gp_port(ifp->bridge_name,ifp->name,
                                 PAL_TRUE,enableBPDUrx,&psuedoRootId);
   if (ret<0)
     {
       cli_out(cli,"%%Can't configure port %s to l2gp\n", ifp->name);
       return CLI_ERROR;
     }

   return CLI_SUCCESS;
}

CLI (mstp_add_l2gp_rx_status,
     mstp_add_l2gp_rx_statuscmd,
     "switchport l2gp  psuedoRootId ROOTID:MAC (enableBPDUrx|disableBPDUrx)",
     "Set the l2gp characteristics of the Layer2 interface",
     "l2gp",
     "puedoInfo bridge/bridge mac address",
     "priority/mac-address 12345/0000.0000.0000",
     "enableBPDUrx",
     "disableBPDUrx")
{
   int ret=0, i=0;
   struct interface *ifp = cli->index;
   struct mstp_interface *mstpif = ifp->info;
   struct bridge_id psuedoRootId;
   int enableBPDUrx = PAL_TRUE;
   uint32_t psuedoBridgeId;
   uint8_t buf[20];
   struct mstp_bridge *br;
   struct mstp_port *port;

   pal_assert(ifp);
   pal_assert(mstpif);

   if(LAYER2_DEBUG(proto,CLI_ECHO))
       zlog_debug(mstpm, "spanning-tree l2gp");

   if ( argv[0] == NULL || argv[1] == NULL )
     {
        cli_out(cli,"%%no input for psuedoRootId or enableBPDUrx \n");
        return CLI_ERROR; 
     }

   pal_mem_cpy((void *)buf,(void *)argv[0],pal_strlen(argv[0]));
   pal_mem_set(&psuedoRootId,0, sizeof(psuedoRootId));

   /* extracting macaddress */
   while(buf[i++] != '/'); 

   /* extracting psuedo bridge id */
   psuedoBridgeId = pal_atoi(pal_strtok(buf,"/"));

   if ((psuedoBridgeId % MSTP_BRIDGE_PRIORITY_MULTIPLIER) != 0 )
     {
       if ( psuedoBridgeId > MSTP_MAX_BRIDGE_PRIORITY )
         {
            cli_out (cli, "%% priority should be lower than %d \n",
                              MSTP_MAX_BRIDGE_PRIORITY );
            return CLI_ERROR;
         }
       cli_out (cli, "%% priority should be in multiples of 4096 \n");
       return CLI_ERROR;
     }

   if (ifp == NULL)
     {
        cli_out(cli,"%% Can't configure on interface %s\n",ifp->name);
        return CLI_ERROR;
     }

   if (pal_sscanf (&buf[i],"%4hx.%4hx.%4hx", (unsigned short *)&psuedoRootId.addr[0],
                (unsigned short *)&psuedoRootId.addr[2],
                (unsigned short *)&psuedoRootId.addr[4]) != 3)
     {
          cli_out (cli, "%% Unable to translate mac address %s\n", &buf[i]);
          return CLI_ERROR;
     }

   /* Convert to network order */
   *(unsigned short *)&psuedoRootId.addr[0] = pal_hton16(*(unsigned short *) \
                                              &psuedoRootId.addr[0]);
   *(unsigned short *)&psuedoRootId.addr[2] = pal_hton16(*(unsigned short *) \
                                              &psuedoRootId.addr[2]);
   *(unsigned short *)&psuedoRootId.addr[4] = pal_hton16(*(unsigned short *) \
                                              &psuedoRootId.addr[4]);

   if ( pal_strcmp("enableBPDUrx",argv[1]) == 0 )
   {
      enableBPDUrx = PAL_TRUE;
   }
   else if ( pal_strcmp("disableBPDUrx",argv[1]) == 0 )
    {
      enableBPDUrx = PAL_FALSE;
    }
   else
    {
       return CLI_ERROR;
    }

   br = mstp_find_bridge (ifp->bridge_name);
   if (!br)
   {
       if (if_lookup_by_name (&cli->vr->ifm, ifp->name) != NULL)
       {
	   pal_strcpy (mstpif->bridge_group, ifp->bridge_name);
	   mstpif->isL2gp = PAL_TRUE;
	   mstpif->enableBPDUrx = enableBPDUrx;
	   mstp_put_prio_in_bridgeid(&psuedoRootId,(u_int16_t)psuedoBridgeId);
	   pal_mem_cpy (&mstpif->psuedoRootId, &psuedoRootId,
		   sizeof(struct bridge_id));
	   SET_FLAG (mstpif->config, MSTP_IF_CONFIG_L2GP_PORT);
	   cli_out (cli, " %% Associate the interface to the bridge %s\n", 
		   ifp->bridge_name);
	   return CLI_SUCCESS;
       }
   }

   port = mstp_find_cist_port (br, ifp->ifindex);
   if (!port)
     {
	   pal_strcpy (mstpif->bridge_group, ifp->bridge_name);
	   mstpif->isL2gp = PAL_TRUE;
	   mstpif->enableBPDUrx = enableBPDUrx;
	   mstp_put_prio_in_bridgeid(&psuedoRootId,(u_int16_t)psuedoBridgeId);
	   pal_mem_cpy (&mstpif->psuedoRootId, &psuedoRootId,
		   sizeof(struct bridge_id));
	   SET_FLAG (mstpif->config, MSTP_IF_CONFIG_L2GP_PORT);

         cli_out(cli,"%%can not find port %s \n", ifp->name);
         return CLI_ERROR;
     }

   if (port->isL2gp)
     {
         unsigned short prio=0;
         bcopy((void *)port->psuedoRootId.prio,(void *)&prio, 2);

         
         if ((!pal_mem_cmp(&psuedoRootId.addr,&port->psuedoRootId.addr,
                            ETHER_ADDR_LEN) &&
                            (pal_hton16(prio) == (u_int16_t)psuedoBridgeId ) &&
                            (enableBPDUrx == port->enableBPDUrx )))
           {
              cli_out(cli,"%%%s already an l2gp port \n", ifp->name);
              return CLI_ERROR;
           }
           /*
            * else case is for, given input differs from the existing
            * l2gp configuration. such cases, update the given
            * configuration
            */

     }   

   mstp_put_prio_in_bridgeid(&psuedoRootId,(u_int16_t)psuedoBridgeId);

   ret = mstp_api_set_l2gp_port(ifp->bridge_name,ifp->name,
                                PAL_TRUE,enableBPDUrx,&psuedoRootId);
   if (ret<0)
     {
         cli_out(cli,"%%Can't configure port %s to l2gp\n", ifp->name);
         return CLI_ERROR;
     }

   return CLI_SUCCESS;
}


CLI (no_mstp_add_l2gp,
     no_mstp_add_l2gp_cmd,
     "no switchport l2gp",
     "clear the l2gp characteristics of the Layer2 interface",
     "l2gp",
     "set mode as switch port")
{

   int ret = 0;
   struct interface * ifp = cli->index;
   struct mstp_port *port = NULL;
   struct mstp_bridge *br;
    
   pal_assert(ifp);
     
   if (LAYER2_DEBUG(proto,CLI_ECHO)) 
       zlog_debug(mstpm, "spanning-tree l2gp");

   br = mstp_find_bridge (ifp->bridge_name);
   if (!br)
     {
         cli_out(cli,"%%port not associated to any bridge %s \n", ifp->name);
         return CLI_ERROR;
     }


   port = mstp_find_cist_port (br, ifp->ifindex);
   if (!port)
     {
         cli_out(cli,"%%can not find port %s \n", ifp->name);
         return CLI_ERROR;
     }

   if (port->isL2gp == PAL_FALSE)
     {
       cli_out(cli,"%%port %s is not an l2gp\n", ifp->name);
       return CLI_ERROR;
     }
       
   ret = mstp_api_set_l2gp_port(ifp->bridge_name,ifp->name,PAL_FALSE,PAL_FALSE,NULL);
   if(ret<0)
   {
       cli_out(cli,"%%Can't configure port %s to no switchport l2gp\n", ifp->name);
       return CLI_ERROR;
   }

   return CLI_SUCCESS;
}

#endif /* HAVE_L2GP */

CLI (debug_mstp_proto,
     debug_mstp_proto_cmd,
     "debug mstp protocol",
     CLI_DEBUG_STR,
     CLI_MSTP_STR,
     "protocol")
{
  DEBUG_ON (cli, proto, PROTO);
  return CLI_SUCCESS;
}

CLI (debug_mstp_proto_detail,
     debug_mstp_proto_detail_cmd,
     "debug mstp protocol detail",
     CLI_DEBUG_STR,
     CLI_MSTP_STR,
     "Protocol",
     "detail")
{
  DEBUG_ON (cli, proto, PROTO_DETAIL);
  return CLI_SUCCESS;
}

CLI (debug_mstp_timer,
     debug_mstp_timer_cmd,
     "debug mstp timer",
     CLI_DEBUG_STR,
     CLI_MSTP_STR,
     "MSTP Timers")
{
  DEBUG_ON (cli, proto, TIMER);
  return CLI_SUCCESS;
}

CLI (debug_mstp_timer_detail,
     debug_mstp_timer_detail_cmd,
     "debug mstp timer detail",
     CLI_DEBUG_STR,
     CLI_MSTP_STR,
     "MSTP Timers",
     "Detailed output")
{
  DEBUG_ON (cli, proto, TIMER_DETAIL);
  return CLI_SUCCESS;
}

CLI (debug_mstp_packet_tx,
     debug_mstp_packet_tx_cmd,
     "debug mstp packet tx",
     CLI_DEBUG_STR,
     CLI_MSTP_STR,
     "MSTP Packets",
     "Transmit")
{
  DEBUG_ON (cli, proto, PACKET_TX);
  return CLI_SUCCESS;
}

CLI (debug_mstp_packet_rx,
     debug_mstp_packet_rx_cmd,
     "debug mstp packet rx",
     CLI_DEBUG_STR,
     CLI_MSTP_STR,
     "MSTP Packets",
     "Receive")
{
  DEBUG_ON (cli, proto, PACKET_RX);
  return CLI_SUCCESS;
}

CLI (debug_mstp_cli,
     debug_mstp_cli_cmd,
     "debug mstp cli",
     CLI_DEBUG_STR,
     CLI_MSTP_STR,
     "CLI Commands")
{
  DEBUG_ON (cli, proto, CLI_ECHO);
  return CLI_SUCCESS;
}

CLI (no_debug_mstp_packet_tx,
     no_debug_mstp_packet_tx_cmd,
     "no debug mstp packet tx",
     CLI_NO_STR,
     CLI_DEBUG_STR,
     CLI_MSTP_STR,
     "MSTP Packets",
     "Transmit")
{
  DEBUG_OFF (proto, PACKET_TX);
  return CLI_SUCCESS;
}

CLI (no_debug_mstp_packet_rx,
     no_debug_mstp_packet_rx_cmd,
     "no debug mstp packet rx",
     CLI_NO_STR,
     CLI_DEBUG_STR,
     CLI_MSTP_STR,
     "MSTP Packets",
     "Receive")
{
  DEBUG_OFF (proto, PACKET_RX);
  return CLI_SUCCESS;
}


CLI (no_debug_mstp_proto,
     no_debug_mstp_proto_cmd,
     "no debug mstp protocol",
     CLI_NO_STR,
     CLI_DEBUG_STR,
     CLI_MSTP_STR,
     "MSTP Protocol")
{
  DEBUG_OFF (proto, PROTO);
  return CLI_SUCCESS;
}

CLI (no_debug_mstp_proto_detail,
     no_debug_mstp_proto_detail_cmd,
     "no debug mstp protocol detail",
     CLI_NO_STR,
     CLI_DEBUG_STR,
     CLI_MSTP_STR,
     "MSTP Protocol",
     "Detailed output")
{
  DEBUG_OFF (proto, PROTO_DETAIL);
  return CLI_SUCCESS;
}


CLI (no_debug_mstp_cli,
     no_debug_mstp_cli_cmd,
     "no debug mstp cli",
     CLI_NO_STR,
     CLI_DEBUG_STR,
     CLI_MSTP_STR,
     "CLI Commands")
{
  DEBUG_OFF (proto, CLI_ECHO);
  return CLI_SUCCESS;
}


CLI (no_debug_mstp_timer,
     no_debug_mstp_timer_cmd,
     "no debug mstp timer",
     CLI_NO_STR,
     CLI_DEBUG_STR,
     CLI_MSTP_STR,
     "MSTP Timers")
{
  DEBUG_OFF (proto, TIMER);
  return CLI_SUCCESS;
}

CLI (no_debug_mstp_timer_detail,
     no_debug_mstp_timer_detail_cmd,
     "no debug mstp timer detail",
     CLI_NO_STR,
     CLI_DEBUG_STR,
     CLI_MSTP_STR,
     "MSTP Timers",
     "Detailed output")
{
  DEBUG_OFF (proto, TIMER_DETAIL);
  return CLI_SUCCESS;
}

CLI (debug_mstp_all,
     debug_mstp_all_cmd,
     "debug mstp all",
     CLI_DEBUG_STR,
     CLI_MSTP_STR,
     "all")
{
  DEBUG_ON (cli, proto, TIMER);
  DEBUG_ON (cli, proto, TIMER_DETAIL);
  DEBUG_ON (cli, proto, PROTO);
  DEBUG_ON (cli, proto, PROTO_DETAIL);
  DEBUG_ON (cli, proto, CLI_ECHO);
  DEBUG_ON (cli, proto, PACKET_TX);
  DEBUG_ON (cli, proto, PACKET_RX);
  return CLI_SUCCESS;
}

CLI (no_debug_mstp_all,
     no_debug_mstp_all_cmd,
     "no debug mstp all",
     CLI_NO_STR,
     CLI_DEBUG_STR,
     CLI_MSTP_STR,
     "all")
{
  DEBUG_OFF (proto, TIMER);
  DEBUG_OFF (proto, TIMER_DETAIL);
  DEBUG_OFF (proto, PROTO);
  DEBUG_OFF (proto, PROTO_DETAIL);
  DEBUG_OFF (proto, CLI_ECHO);
  DEBUG_OFF (proto, PACKET_TX);
  DEBUG_OFF (proto, PACKET_RX);
  return CLI_SUCCESS;
}

CLI (show_debugging_mstp,
     show_debugging_mstp_cmd,
     "show debugging mstp",
     CLI_SHOW_STR,
     "Debugging information outputs",
     "Multiple Spanning Tree Protocol (MSTP)")
{
  cli_out (cli, "MSTP debugging status:\n");

  if (LAYER2_DEBUG(proto, TIMER) &&
      LAYER2_DEBUG(proto, TIMER_DETAIL) &&
      LAYER2_DEBUG(proto, PROTO) &&
      LAYER2_DEBUG(proto, PROTO_DETAIL) &&
      LAYER2_DEBUG(proto, CLI_ECHO) &&
      LAYER2_DEBUG(proto, PACKET_TX) &&
      LAYER2_DEBUG(proto, PACKET_RX))
    cli_out (cli, "  MSTP all debugging is on\n");
  else
    {
      if (LAYER2_DEBUG(proto, TIMER))
        cli_out (cli, "  MSTP timer debugging is on\n");
      if (LAYER2_DEBUG(proto, TIMER_DETAIL))
        cli_out (cli, "  MSTP detailed timer debugging is on\n");
      if (LAYER2_DEBUG(proto, PROTO))
        cli_out (cli, "  MSTP protocol debugging is on\n");
      if (LAYER2_DEBUG(proto, PROTO_DETAIL))
        cli_out (cli, "  MSTP detailed protocol debugging is on\n");
      if (LAYER2_DEBUG(proto, CLI_ECHO))
        cli_out (cli, "  MSTP cli echo debugging is on\n");
      if (LAYER2_DEBUG(proto, PACKET_TX))
        cli_out (cli, "  MSTP transmiting packet debugging is on\n");
      if (LAYER2_DEBUG(proto, PACKET_RX))
        cli_out (cli, "  MSTP receiving packet debugging is on\n");
    }
  if (LAYER2_DEBUG(proto, EVENT))
    cli_out (cli, "  MSTP event debugging is on\n");

#ifdef HAVE_WMI
  if (IS_DEBUG_WMI_XSTP(EVENT) &&
      IS_DEBUG_WMI_XSTP(RECV) &&
      IS_DEBUG_WMI_XSTP(SEND))
    cli_out (cli, "  MSTP wmi (event,receive,send) debugging is on\n");
  else
    {
      if (IS_DEBUG_WMI_XSTP(EVENT))
        cli_out (cli, "  MSTP wmi event debugging is on\n");
      if (IS_DEBUG_WMI_XSTP(RECV))
        cli_out (cli, "  MSTP wmi receiving message debugging is on\n");
      if (IS_DEBUG_WMI_XSTP(SEND))
        cli_out (cli, "  MSTP wmi sending message debugging is on\n");
    }
  if (IS_DEBUG_WMI_XSTP(DETAIL))
    cli_out (cli, "  MSTP wmi detailed debugging is on\n");
#endif /* HAVE_WMI */
  return CLI_SUCCESS;
}


int
mstp_customer_bridge_config_write (struct cli *cli, struct mstp_bridge *br,
                                   struct interface *ifp)
{
  struct mstp_port *port;
  int write = 0;

  write++;

  if (br->cist_bridge_priority != CE_BRIDGE_DEFAULT_PRIORITY)
    {
      write++;
      cli_out (cli, " customer-spanning-tree priority %u\n",
               br->cist_bridge_priority);
    }

  if (br->bridge_forward_delay / L2_TIMER_SCALE_FACT !=
      BRIDGE_TIMER_DEFAULT_FWD_DELAY)
    {
      write++;
      cli_out (cli, " customer-spanning-tree forward-time %d\n",
               br->bridge_forward_delay / L2_TIMER_SCALE_FACT);
    }

  if (br->bridge_hello_time / L2_TIMER_SCALE_FACT !=
      BRIDGE_TIMER_DEFAULT_HELLO_TIME)
    {
      write++;
      cli_out (cli, " customer-spanning-tree hello-time %d\n",
               br->bridge_hello_time / L2_TIMER_SCALE_FACT);
    }

  if (br->bridge_max_age / L2_TIMER_SCALE_FACT !=
      BRIDGE_TIMER_DEFAULT_MAX_AGE)
    {
      write++;
      cli_out (cli, " customer-spanning-tree max-age %d\n",
               br->bridge_max_age / L2_TIMER_SCALE_FACT);
    }

  /* Max Hops */
  if (br->bridge_max_hops != MST_DEFAULT_MAXHOPS)
    {
      write++;
      cli_out (cli, " customer-spanning-tree max-hops %d\n", br->bridge_max_hops);
    }

  if (br->transmit_hold_count != MSTP_TX_HOLD_COUNT)
    {
      write++;
      cli_out (cli, " customer-spanning-tree transmit-holdcount %d\n",
               br->transmit_hold_count);
    }

  BRIDGE_CIST_PORTLIST_LOOP (port, br)
    {
      if (port->cist_priority != CE_BRIDGE_DEFAULT_PORT_PRIORITY)
        {
           write++;
           cli_out (cli, " %s priority %d\n", mstp_get_ce_br_port_str (port),
                    port->cist_priority);
        }

      if (port->cist_path_cost != mstp_nsm_get_port_path_cost (mstpm,
                                                               port->ifindex))
        {
           write++;
           cli_out (cli, " %s path-cost %u\n", mstp_get_ce_br_port_str (port),
                    port->cist_path_cost);
        }
    }

  return write;

}

#ifdef HAVE_RPVST_PLUS
void
mstp_if_rpvst_write(struct cli * cli, struct mstp_bridge *br,
                    struct interface *ifp,
                    struct mstp_instance_port *inst_port)
{

  /* bridge-group NAME instance INSTANCE */
  cli_out(cli," %s vlan %d\n",
      mstp_get_bridge_group_str_span (br),
      inst_port->vlan_id );

  if (inst_port->ifindex == ifp->ifindex)
    {
      /* bridge-group NAME instance INSTANCE priority <0-240> */
      if (inst_port->msti_priority != BRIDGE_DEFAULT_PORT_PRIORITY)
        {
          cli_out(cli," %s vlan %d priority %d\n",
              mstp_get_bridge_group_str_span (br),
              inst_port->vlan_id,inst_port->msti_priority);
        }

      /* bridge-group NAME instance INSTANCE path-cost <1-200000000> */
      if (inst_port->msti_path_cost !=  mstp_nsm_get_port_path_cost (mstpm, 
                                                ifp->ifindex))
        {
          cli_out(cli," %s vlan %d path-cost %u\n",
              mstp_get_bridge_group_str_span (br),
              inst_port->vlan_id,inst_port->msti_path_cost);
        }

      if (inst_port->restricted_tcn)
        {
          cli_out(cli," spanning-tree vlan %d restricted-tcn\n",
              inst_port->vlan_id);
        }

      if (inst_port->restricted_role)
        {
          cli_out(cli," spanning-tree vlan %d restricted-role\n",
              inst_port->vlan_id);
        }
    }
}
#endif /* HAVE_RPVST_PLUS */
int 
mstp_if_write (struct cli * cli)
{
  struct mstp_instance_port *inst_port = NULL;
  struct mstp_bridge *br;
  struct mstp_port *port;
  struct interface *ifp;
  struct listnode *n;
  int write = 0;

  LIST_LOOP (cli->vr->ifm.if_list, ifp, n)
    {

      if (CHECK_FLAG (ifp->status, IF_HIDDEN_FLAG))
        continue;

      if (write)
        cli_out (cli, "!\n");

      write++;

      cli_out (cli, "interface %s\n", ifp->name);

      if (ifp->port)
        {
          port = ifp->port;

          br = mstp_find_bridge (ifp->bridge_name);
          if (br == NULL)
            return RESULT_ERROR;

          /* bridge-group NAME priority P */
          if (port->cist_priority != BRIDGE_DEFAULT_PORT_PRIORITY)
            cli_out (cli, " %s priority %d\n", 
                mstp_get_bridge_group_str_span (br),
                port->cist_priority);

           if (port->pathcost_configured)
              cli_out (cli, " %s path-cost %u\n", 
                 mstp_get_bridge_group_str_span (br), 
                 port->cist_path_cost);


          for (inst_port = port->instance_list; inst_port ; 
               inst_port = inst_port->next_inst)
            if (inst_port)
              {
#ifdef HAVE_RPVST_PLUS
                if (br->type == NSM_BRIDGE_TYPE_RPVST_PLUS) 
                    mstp_if_rpvst_write(cli, br, ifp, inst_port);
                else
#endif /* HAVE_RPVST_PLUS */
                  {
                    /* bridge-group NAME instance INSTANCE */
                    cli_out(cli," %s instance %d\n",
                        mstp_get_bridge_group_str_span (br),
                        inst_port->instance_id );

                    if (inst_port->ifindex == ifp->ifindex)
                      {
                        /* bridge-group NAME instance INSTANCE priority <0-240> */
                        if (inst_port->msti_priority != BRIDGE_DEFAULT_PORT_PRIORITY)
                          {
                            cli_out(cli," %s instance %d priority %d\n",
                                mstp_get_bridge_group_str_span (br),
                                inst_port->instance_id,inst_port->msti_priority);
                          }

                        /* bridge-group NAME instance INSTANCE path-cost <1-200000000> */
                        if (inst_port->pathcost_configured)
                          {
                            cli_out(cli," %s instance %d path-cost %u\n",
                                mstp_get_bridge_group_str_span (br),
                                inst_port->instance_id,inst_port->msti_path_cost);
                          }

                        if (inst_port->restricted_tcn)
                          {
                            cli_out(cli," spanning-tree instance %d restricted-tcn\n",
                                inst_port->instance_id);
                          }

                        if (inst_port->restricted_role)
                          {
                            cli_out(cli," spanning-tree instance %d restricted-role\n",
                                inst_port->instance_id);
                          }

                      }
                  }
              }

          /* Default link type is p2p */
          if (port->admin_p2p_mac == PAL_FALSE)
            cli_out (cli, " spanning-tree link-type shared\n");
          if (port->admin_p2p_mac == MSTP_ADMIN_LINK_TYPE_AUTO)
            cli_out (cli, " spanning-tree link-type auto\n");

          /* default admin edge is false */
          if (port->admin_edge)
            {
              if (port->portfast_conf)
                cli_out (cli, " spanning-tree portfast\n");
              else
                cli_out (cli, " spanning-tree edgeport\n");
            }

          /* default admin bpduguard for port is default making the bridge
           * bpdu-guard value take the effect over this port.
           */
          if (port->admin_bpduguard != MSTP_PORT_PORTFAST_BPDUGUARD_DEFAULT)
            {
              if (port->admin_bpduguard == MSTP_PORT_PORTFAST_BPDUGUARD_ENABLED)
                cli_out (cli, " spanning-tree portfast bpdu-guard %s\n",
                    "enable");
              else
                cli_out (cli, " spanning-tree portfast bpdu-guard %s\n",
                    "disable");
            }

          if (port->restricted_tcn)
            cli_out(cli," spanning-tree restricted-tcn\n");

          if (port->restricted_role)
            cli_out(cli," spanning-tree restricted-role\n");


          /* default admin bpdufilter for port is default making the bridge
           * bpdu-filter value take the effect over this port.
           */
          if (port->admin_bpdufilter != MSTP_PORT_PORTFAST_BPDUFILTER_DEFAULT)
            {
              if (port->admin_bpdufilter ==
                  MSTP_PORT_PORTFAST_BPDUFILTER_ENABLED)
                cli_out (cli, " spanning-tree portfast bpdu-filter %s\n",
                    "enable");
              else
                cli_out (cli, " spanning-tree portfast bpdu-filter %s\n",
                    "disable");
            }

          /* default root guard value if FALSE */
          if (port->admin_rootguard != PAL_FALSE)
            cli_out (cli, " spanning-tree guard root\n");

          if (port->auto_edge)
            cli_out (cli, " spanning-tree autoedge\n");

          if ((IS_BRIDGE_MSTP (br)))
            {
              if (port->hello_time / L2_TIMER_SCALE_FACT !=
                  BRIDGE_TIMER_DEFAULT_HELLO_TIME)
                cli_out (cli, " spanning-tree hello-time %d\n",
                    port->hello_time / L2_TIMER_SCALE_FACT);
            }

#ifdef HAVE_L2GP             
          if ( port->isL2gp && (IS_BRIDGE_MSTP (br)))
            {

              unsigned short prio=0;
              bcopy((void *)port->psuedoRootId.prio,(void *)&prio, 2);

              cli_out(cli, " switchport l2gp psuedoRootId %d%s%.02hx%.02hx.%.02hx%.02hx.%.02hx%.02hx %s\n",
                  pal_hton16(prio), "/", 
                  port->psuedoRootId.addr[0],
                  port->psuedoRootId.addr[1],
                  port->psuedoRootId.addr[2],
                  port->psuedoRootId.addr[3],
                  port->psuedoRootId.addr[4],
                  port->psuedoRootId.addr[5],
                  ((port->enableBPDUrx == PAL_TRUE) ?
                   "enableBPDUrx" : "disableBPDUrx"));
            }
#endif /* HAVE_L2GP */

#ifdef HAVE_VLAN
          if (port->port_type == MSTP_VLAN_PORT_MODE_CE)
            mstp_customer_bridge_config_write (cli, port->ce_br, ifp);
#endif /* HAVE_VLAN */
        }
    }

  return write;
}

int
te_msti_config_write (struct cli *cli)
{
  int write = 0;
  int vid;
  struct mstp_bridge *br;
  struct rlist_info *vlan_list = NULL;
  struct mstp_bridge_instance *mstp_br_inst = NULL;
  
  br = mstp_get_first_bridge();

  while (br)
    {
      mstp_br_inst = br->instance_list[MSTP_TE_MSTID];
      if (!mstp_br_inst)
        return -1;
      
      if (mstp_br_inst->is_te_instance)
        {
          vlan_list = mstp_br_inst->vlan_list;

          cli_out (cli, "!\n");
          cli_out (cli, "spanning-tree te-msti configuration\n"); 
          while (vlan_list)
            {
              for (vid = vlan_list->lo ; vid <= vlan_list->hi ; vid++)
                cli_out (cli," %s te-msti vlan %d\n",
                    mstp_get_bridge_str (br), vid);
              vlan_list = vlan_list->next;
            }
        }

      write++;
      cli_out (cli, "\n");
      br = br->next;
    }
  return RESULT_OK;
}

int 
mst_config_write (struct cli * cli)
{
  int index;
  int write = 0;
  struct mstp_bridge *br;
  u_int8_t first = PAL_TRUE;

 
  br = mstp_get_first_bridge();

  while ( br )
    {

      if (! IS_BRIDGE_MSTP (br))
        {
          br = br->next;
          continue;
        }

      if (first == PAL_TRUE)
        {
          cli_out (cli, "!\n");
          cli_out (cli, "spanning-tree mst configuration\n");
          first = PAL_FALSE;
        }

      /* Add each instance */
      for (index = 1; index < MST_MAX_INSTANCES; index++)
        {
          struct mstp_bridge_instance *br_inst = br->instance_list[index];
          struct rlist_info *vlan_list;

          if (!br_inst)
            continue;

          vlan_list = br_inst->vlan_list;

          if (!vlan_list)
            cli_out (cli," %s instance %d\n", mstp_get_bridge_str (br), index);

          while (vlan_list)
            {
              int vid;
              /* Till we implement range addition in cli we have go thru 
               * each vid in the list */
              for (vid = vlan_list->lo ; vid <= vlan_list->hi ; vid++)
                cli_out (cli," %s instance %d vlan %d\n", 
                          mstp_get_bridge_str (br), index, vid);
                vlan_list = vlan_list->next;
            }
          write++;
        }
      
      if ((br->config.cfg_name[0] != '\0') && (br->type == NSM_BRIDGE_TYPE_MSTP))
        cli_out (cli, " %s region %s\n", mstp_get_bridge_str (br),
                 br->config.cfg_name);

      if (br->config.cfg_revision_lvl != 0)
        cli_out (cli, " %s revision %d\n", 
                 mstp_get_bridge_str (br), br->config.cfg_revision_lvl);
      
      br = br->next;
    } 

  cli_out (cli, "!\n"); 

  return RESULT_OK;
}

/* rpvst+ config write*/
int
rpvst_plus_config_write(struct cli * cli)
{
  int index;
  int write = 0;
  struct mstp_bridge *br = NULL;
  u_int8_t first = PAL_TRUE;

  br = mstp_get_first_bridge();

  while ( br )
    {
      if (! IS_BRIDGE_RPVST_PLUS (br))
        {
          br = br->next;
          continue;
        }

      if (first == PAL_TRUE)
        {
          cli_out (cli, "!\n");
          cli_out (cli, "spanning-tree rpvst+ configuration\n");
          first = PAL_FALSE;
        }

      /* Add each instance */
      for (index = 1; index < br->max_instances; index++)
        {
          struct mstp_bridge_instance *br_inst = br->instance_list[index];
          struct rlist_info *vlan_list;

          if (!br_inst)
            continue;

          vlan_list = br_inst->vlan_list;

          while (vlan_list)
            {
              int vid;
              /* Till we implement range addition in cli we have go thru
               *                * each vid in the list */
              for (vid = vlan_list->lo ; vid <= vlan_list->hi ; vid++)
                cli_out (cli," %s vlan %d\n",
                    mstp_get_bridge_str (br), vid);
              vlan_list = vlan_list->next;
            }
          write++;

        }

      br = br->next;
    }

  return RESULT_OK;
}


int
mstp_debug_write (struct cli *cli)
{
  return RESULT_OK;
}

void
mstp_cli_init_debug (struct cli_tree *ctree)
{
  cli_install_config (ctree, DEBUG_MODE, mstp_debug_write);
 
  cli_install (ctree, EXEC_MODE, &show_debugging_mstp_cmd);
  cli_install (ctree, EXEC_MODE, &debug_mstp_proto_cmd);
  cli_install (ctree, EXEC_MODE, &no_debug_mstp_proto_cmd);
  cli_install (ctree, EXEC_MODE, &debug_mstp_proto_detail_cmd);
  cli_install (ctree, EXEC_MODE, &no_debug_mstp_proto_detail_cmd);
  cli_install (ctree, EXEC_MODE, &debug_mstp_timer_cmd);
  cli_install (ctree, EXEC_MODE, &no_debug_mstp_timer_cmd);
  cli_install (ctree, EXEC_MODE, &debug_mstp_timer_detail_cmd);
  cli_install (ctree, EXEC_MODE, &no_debug_mstp_timer_detail_cmd);
  cli_install (ctree, EXEC_MODE, &debug_mstp_packet_rx_cmd);
  cli_install (ctree, EXEC_MODE, &no_debug_mstp_packet_rx_cmd);
  cli_install (ctree, EXEC_MODE, &debug_mstp_packet_tx_cmd);
  cli_install (ctree, EXEC_MODE, &no_debug_mstp_packet_tx_cmd);
  cli_install (ctree, EXEC_MODE, &debug_mstp_cli_cmd);
  cli_install (ctree, EXEC_MODE, &no_debug_mstp_cli_cmd);
  cli_install (ctree, EXEC_MODE, &debug_mstp_all_cmd);
  cli_install (ctree, EXEC_MODE, &no_debug_mstp_all_cmd);

  cli_install (ctree, CONFIG_MODE, &debug_mstp_proto_cmd);
  cli_install (ctree, CONFIG_MODE, &no_debug_mstp_proto_cmd);
  cli_install (ctree, CONFIG_MODE, &debug_mstp_proto_detail_cmd);
  cli_install (ctree, CONFIG_MODE, &no_debug_mstp_proto_detail_cmd);
  cli_install (ctree, CONFIG_MODE, &debug_mstp_timer_cmd);
  cli_install (ctree, CONFIG_MODE, &no_debug_mstp_timer_cmd);
  cli_install (ctree, CONFIG_MODE, &debug_mstp_timer_detail_cmd);
  cli_install (ctree, CONFIG_MODE, &no_debug_mstp_timer_detail_cmd);
  cli_install (ctree, CONFIG_MODE, &debug_mstp_packet_rx_cmd);
  cli_install (ctree, CONFIG_MODE, &no_debug_mstp_packet_rx_cmd);
  cli_install (ctree, CONFIG_MODE, &debug_mstp_packet_tx_cmd);
  cli_install (ctree, CONFIG_MODE, &no_debug_mstp_packet_tx_cmd);
  cli_install (ctree, CONFIG_MODE, &debug_mstp_cli_cmd);
  cli_install (ctree, CONFIG_MODE, &no_debug_mstp_cli_cmd);
  cli_install (ctree, CONFIG_MODE, &debug_mstp_all_cmd);
  cli_install (ctree, CONFIG_MODE, &no_debug_mstp_all_cmd);
}

/* Install bridge related CLI.  */
void
mstp_bridge_cli_init (struct lib_globals * mstpm)
{
  struct cli_tree *ctree = mstpm->ctree;

  cli_install_config (ctree, MSTP_MODE, mstp_bridge_config_write);
  cli_install_default (ctree, MSTP_MODE);
  
  /* Install interface node. */
  cli_install_config (ctree, INTERFACE_MODE, mstp_if_write);
  cli_install_default (ctree, INTERFACE_MODE);

#ifdef HAVE_MSTPD
  /* Install mst-config node. */
  cli_install_config (ctree, MST_CONFIG_MODE, mst_config_write);
  cli_install_default (ctree, MST_CONFIG_MODE);

#if defined (HAVE_PROVIDER_BRIDGE) || defined (HAVE_B_BEB)
  /* Install te-msti-config node. */
  cli_install_config (ctree, TE_MSTI_CONFIG_MODE, te_msti_config_write);
  cli_install_default (ctree, TE_MSTI_CONFIG_MODE);
#endif
#endif /* HAVE_MSTPD */

  /* Install commands. */
  cli_install (ctree, CONFIG_MODE, &interface_mstp_cmd);
#ifdef HAVE_MSTPD
  cli_install (ctree, CONFIG_MODE, &mst_config_cmd);
  cli_install (ctree, MST_CONFIG_MODE, &bridge_instance_cmd);
  cli_install (ctree, MST_CONFIG_MODE, &bridge_instance_vlan_cmd);
  cli_install (ctree, MST_CONFIG_MODE, &bridge_region_cmd);
  cli_install (ctree, MST_CONFIG_MODE, &no_bridge_region_cmd);
  cli_install (ctree, MST_CONFIG_MODE, &bridge_revision_cmd);
  cli_install (ctree, MST_CONFIG_MODE, &no_bridge_instance_cmd);
  cli_install (ctree, MST_CONFIG_MODE, &no_bridge_instance_vlan_cmd);
#endif /* HAVE_MSTPD */
  cli_install (ctree, EXEC_MODE, &show_spanning_tree_cmd);
  cli_install (ctree, EXEC_MODE, &show_spanning_tree_stats_cmd);
  cli_install (ctree, EXEC_MODE, &show_spanning_tree_stats_bridge_cmd);
  cli_install (ctree, EXEC_MODE, &show_spanning_tree_stats_interface_bridge_cmd);

  cli_install (ctree, EXEC_MODE, &mstp_clear_spanning_tree_interface_cmd);
  cli_install (ctree, EXEC_MODE, &mstp_clear_spanning_tree_bridge_cmd);
  cli_install (ctree, EXEC_MODE, &mstp_clear_spanning_tree_interface_bridge_cmd);

#ifdef HAVE_PROVIDER_BRIDGE
  cli_install (ctree, EXEC_MODE, &show_cus_spanning_tree_cmd);
#endif /* HAVE_PROVIDER_BRIDGE */
#ifdef HAVE_MSTPD
  cli_install (ctree, EXEC_MODE, &show_spanning_tree_mst_cmd);
#endif /* HAVE_MSTPD */
  cli_install (ctree, EXEC_MODE, &show_spanning_tree_interface_cmd);

#ifdef HAVE_MSTPD 
  cli_install (ctree, EXEC_MODE, &show_spanning_tree_mst_interface_cmd);
  cli_install (ctree, EXEC_MODE, &show_spanning_tree_mst_detail_cmd);
  cli_install (ctree, EXEC_MODE,
               &show_spanning_tree_mst_detail_interface_cmd);
  cli_install (ctree, EXEC_MODE,
               &show_spanning_tree_mst_instance_interface_cmd);

  cli_install (ctree, EXEC_MODE, &show_spanning_tree_mst_config_cmd);
  cli_install (ctree, EXEC_MODE, &show_spanning_tree_mst_instance_cmd);
#endif /* HAVE_MSTPD */

  cli_install (ctree, EXEC_MODE, &show_spanning_tree_instance_vlan_cmd);
  cli_install (ctree, EXEC_MODE, &mstp_clear_spanning_tree_detected_protocols_cmd);
  cli_install (ctree, EXEC_MODE, &mstp_clear_spanning_tree_detected_protocols_interface_cmd); 
  cli_install (ctree, EXEC_MODE, &show_spanning_tree_pathcost_method_cmd);

#ifdef HAVE_MSTPD
  cli_install (ctree, INTERFACE_MODE, &show_spanning_tree_mst_cmd);
  cli_install (ctree, INTERFACE_MODE, &show_spanning_tree_mst_detail_cmd);
  cli_install (ctree, INTERFACE_MODE, &show_spanning_tree_mst_config_cmd);
  cli_install (ctree, INTERFACE_MODE, &show_spanning_tree_mst_instance_cmd);
  
  cli_install (ctree, CONFIG_MODE, &mstp_bridge_priority_cmd);
  cli_install (ctree, CONFIG_MODE, &no_mstp_bridge_priority_cmd);
  cli_install (ctree, CONFIG_MODE, &bridge_inst_priority_cmd);
  cli_install (ctree, CONFIG_MODE, &no_bridge_inst_priority_cmd);
#endif /* HAVE_MSTPD */
  cli_install (ctree, CONFIG_MODE, &bridge_multiple_spanning_tree_cmd);
  cli_install (ctree, CONFIG_MODE, &no_bridge_multiple_spanning_tree_cmd);
  cli_install (ctree, CONFIG_MODE, &no_bridge_shutdown_multiple_spanning_tree_cmd);
  cli_install (ctree, CONFIG_MODE, &bridge_shutdown_multiple_spanning_tree_cmd);
  cli_install (ctree, CONFIG_MODE,
               &bridge_shutdown_multiple_spanning_tree_for_cmd);
  cli_install (ctree, CONFIG_MODE, &mstp_bridge_forward_time_cmd);
  cli_install (ctree, CONFIG_MODE, &no_mstp_bridge_forward_time_cmd);
  cli_install (ctree, CONFIG_MODE, &mstp_bridge_hello_time_cmd);
  cli_install (ctree, CONFIG_MODE, &no_mstp_bridge_hello_time_cmd);
  cli_install (ctree, CONFIG_MODE, &mstp_bridge_transmit_hold_count_cmd);
  cli_install (ctree, CONFIG_MODE, &no_mstp_bridge_transmit_hold_count_cmd);
  cli_install (ctree, CONFIG_MODE, &mstp_bridge_ageing_time_cmd);
  cli_install (ctree, CONFIG_MODE, &no_mstp_bridge_ageing_time_cmd);
  cli_install (ctree, CONFIG_MODE, &mstp_bridge_max_age_cmd);
  cli_install (ctree, CONFIG_MODE, &no_mstp_bridge_max_age_cmd);
  cli_install (ctree, CONFIG_MODE, &bridge_max_hops_cmd);
  cli_install (ctree, CONFIG_MODE, &no_bridge_max_hops_cmd);

  cli_install (ctree, CONFIG_MODE, &mstp_bridge_portfast_bpduguard_cmd);
  cli_install (ctree, CONFIG_MODE, &no_mstp_bridge_portfast_bpduguard_cmd);
  cli_install (ctree, CONFIG_MODE, &mstp_span_portfast_bpduguard_cmd);
  cli_install (ctree, CONFIG_MODE, &no_mstp_span_portfast_bpduguard_cmd);
  cli_install (ctree, CONFIG_MODE, &mstp_spanning_tree_errdisable_timeout_enable_cmd);
  cli_install (ctree, CONFIG_MODE, &no_mstp_spanning_tree_errdisable_timeout_enable_cmd);
  cli_install (ctree, CONFIG_MODE, &mstp_spanning_tree_errdisable_timeout_interval_cmd);
  cli_install (ctree, CONFIG_MODE, &no_mstp_spanning_tree_errdisable_timeout_interval_cmd);
  cli_install (ctree, CONFIG_MODE, &mstp_bridge_portfast_bpdufilter_cmd);
  cli_install (ctree, CONFIG_MODE, &no_mstp_bridge_portfast_bpdufilter_cmd);
  cli_install (ctree, CONFIG_MODE, &mstp_span_portfast_bpdufilter_cmd);
  cli_install (ctree, CONFIG_MODE, &no_mstp_span_portfast_bpdufilter_cmd);
  cli_install (ctree, CONFIG_MODE, &mstp_bridge_cisco_interop_cmd);
  cli_install (ctree, CONFIG_MODE, &spanning_max_hops_cmd);
  cli_install (ctree, CONFIG_MODE, &no_spanning_max_hops_cmd);
  cli_install (ctree, CONFIG_MODE, &mstp_spanning_tree_pathcost_method_cmd);
  cli_install (ctree, CONFIG_MODE, &no_mstp_spanning_tree_pathcost_method_cmd);
  cli_install (ctree, CONFIG_MODE,
               &mstp_bridge_spanning_tree_forceversion_cmd);
  cli_install (ctree, CONFIG_MODE,
               &no_mstp_bridge_spanning_tree_forceversion_cmd);
  cli_install (ctree, CONFIG_MODE, &mstp_spanning_tree_forceversion_cmd);
  cli_install (ctree, CONFIG_MODE, &no_mstp_spanning_tree_forceversion_cmd);


  /* "description" commands. */
  cli_install (ctree, INTERFACE_MODE, &interface_desc_cli);
  cli_install (ctree, INTERFACE_MODE, &no_interface_desc_cli);

  cli_install (ctree, INTERFACE_MODE, &mstp_bridge_group_priority_cmd);
  cli_install (ctree, INTERFACE_MODE, &mstp_bridge_group_path_cost_cmd);
  cli_install (ctree, INTERFACE_MODE, &no_mstp_bridge_group_path_cost_cmd);
#ifdef HAVE_MSTPD 
  cli_install (ctree, INTERFACE_MODE, &bridge_group_inst_cmd);
  cli_install (ctree, INTERFACE_MODE, &no_bridge_group_inst_cmd);
  cli_install (ctree, INTERFACE_MODE, &spanning_inst_cmd);
  cli_install (ctree, INTERFACE_MODE, &no_spanning_inst_cmd);
  cli_install (ctree, INTERFACE_MODE, &bridge_group_inst_priority_cmd);
  cli_install (ctree, INTERFACE_MODE, &spanning_port_inst_priority_cmd);
  cli_install (ctree, INTERFACE_MODE, &bridge_group_inst_path_cost_cmd);
  cli_install (ctree, INTERFACE_MODE, &no_bridge_group_inst_path_cost_cmd);

  cli_install (ctree, INTERFACE_MODE, &mstp_spanning_tree_inst_restricted_tcn_cmd);
  cli_install (ctree, INTERFACE_MODE, &mstp_spanning_tree_inst_restricted_role_cmd);
  cli_install (ctree, INTERFACE_MODE, &no_mstp_spanning_tree_inst_restricted_tcn_cmd);
  cli_install (ctree, INTERFACE_MODE, &no_mstp_spanning_tree_inst_restricted_role_cmd);
  cli_install (ctree, INTERFACE_MODE, &mstp_port_hello_time_cmd);
  cli_install (ctree, INTERFACE_MODE, &no_mstp_port_hello_time_cmd);

#endif /* HAVE_MSTPD */

  cli_install (ctree, INTERFACE_MODE, &mstp_spanning_tree_restricted_tcn_cmd);
  cli_install (ctree, INTERFACE_MODE, &mstp_spanning_tree_restricted_role_cmd);
  cli_install (ctree, INTERFACE_MODE, &no_mstp_spanning_tree_restricted_tcn_cmd);
  cli_install (ctree, INTERFACE_MODE, &no_mstp_spanning_tree_restricted_role_cmd);

  cli_install (ctree, INTERFACE_MODE, &mstp_spanning_tree_linktype_cmd);
  cli_install (ctree, INTERFACE_MODE, &mstp_spanning_tree_linktype_shared_cmd);
  cli_install (ctree, INTERFACE_MODE, &mstp_spanning_tree_linktype_auto_cmd);
  cli_install (ctree, INTERFACE_MODE, &no_mstp_spanning_tree_linktype_cmd);
  cli_install (ctree, INTERFACE_MODE, &mstp_spanning_tree_portfast_cmd);
  cli_install (ctree, INTERFACE_MODE, &no_mstp_spanning_tree_portfast_cmd);
  cli_install (ctree, INTERFACE_MODE, &mstp_spanning_tree_portfast_bpduguard_cmd);
  cli_install (ctree, INTERFACE_MODE, &no_mstp_spanning_tree_portfast_bpduguard_cmd);
  cli_install (ctree, INTERFACE_MODE, &mstp_spanning_tree_guard_root_cmd);
  cli_install (ctree, INTERFACE_MODE, &no_mstp_spanning_tree_guard_root_cmd);
  cli_install (ctree, INTERFACE_MODE, &mstp_spanning_tree_portfast_bpdufilter_cmd);
  cli_install (ctree, INTERFACE_MODE, &no_mstp_spanning_tree_portfast_bpdufilter_cmd);
  cli_install (ctree, INTERFACE_MODE, &mstp_spanning_tree_autoedge_cmd);
  cli_install (ctree, INTERFACE_MODE, &no_mstp_spanning_tree_autoedge_cmd);
  cli_install (ctree, INTERFACE_MODE, &mstp_bridge_transmit_hold_count_cmd);
  cli_install (ctree, INTERFACE_MODE, &no_mstp_bridge_transmit_hold_count_cmd);
  cli_install (ctree, INTERFACE_MODE, &mstp_spanning_transmit_hold_count_cmd);
  cli_install (ctree, INTERFACE_MODE, 
               &no_mstp_spanning_transmit_hold_count_cmd);

  /* Spanning Tree Commands for Default bridge */
  cli_install (ctree, CONFIG_MODE, &spanning_multiple_spanning_tree_cmd);
  cli_install (ctree, CONFIG_MODE, &spanning_shutdown_multiple_spanning_tree_cmd);
  cli_install (ctree, CONFIG_MODE, &no_spanning_shutdown_multiple_spanning_tree_cmd);
  cli_install (ctree, CONFIG_MODE, &spanning_tree_ageing_time_cmd);
  cli_install (ctree, CONFIG_MODE, &no_spanning_tree_ageing_time_cmd);
  cli_install (ctree, CONFIG_MODE, &mstp_spanning_hello_time_cmd);
  cli_install (ctree, CONFIG_MODE, &no_mstp_spanning_hello_time_cmd);
  cli_install (ctree, CONFIG_MODE, &mstp_spanning_cisco_interop_cmd);
  cli_install (ctree, CONFIG_MODE, &mstp_spanning_forward_time_cmd);
  cli_install (ctree, CONFIG_MODE, &no_mstp_spanning_forward_time_cmd);
  cli_install (ctree, CONFIG_MODE, &mstp_spanning_max_age_cmd);
  cli_install (ctree, CONFIG_MODE, &no_mstp_spanning_max_age_cmd);
  cli_install (ctree, CONFIG_MODE, &mstp_spanning_transmit_hold_count_cmd);
  cli_install (ctree, CONFIG_MODE, &no_mstp_spanning_transmit_hold_count_cmd);
  cli_install (ctree, CONFIG_MODE, &mstp_spanning_priority_cmd);
  cli_install (ctree, CONFIG_MODE, &no_mstp_spanning_priority_cmd);
#ifdef HAVE_MSTPD
  cli_install (ctree, CONFIG_MODE, &spanning_inst_priority_cmd);
  cli_install (ctree, CONFIG_MODE, &no_spanning_inst_priority_cmd);
#endif /* HAVE_MSTPD */
  cli_install (ctree, CONFIG_MODE,
               &mstp_spanning_tree_errdisable_timeout_enable_cmd1);
  cli_install (ctree, CONFIG_MODE,
               &no_mstp_spanning_tree_errdisable_timeout_enable_cmd1);
  cli_install (ctree, CONFIG_MODE,
               &mstp_spanning_tree_errdisable_timeout_interval_cmd1);
  cli_install (ctree, CONFIG_MODE,
               &no_mstp_spanning_tree_errdisable_timeout_interval_cmd1);
 
  cli_install (ctree, INTERFACE_MODE, &mstp_spanning_path_cost_cmd);
  cli_install (ctree, INTERFACE_MODE, &no_mstp_spanning_path_cost_cmd);

#ifdef HAVE_MSTPD
  cli_install (ctree, INTERFACE_MODE, &spanning_inst_path_cost_cmd);
  cli_install (ctree, INTERFACE_MODE, &no_spanning_inst_path_cost_cmd);

  cli_install (ctree, MST_CONFIG_MODE, &spanning_region_cmd);
  cli_install (ctree, MST_CONFIG_MODE, &no_spanning_region_cmd);
  cli_install (ctree, MST_CONFIG_MODE, &spanning_revision_cmd);
  cli_install (ctree, MST_CONFIG_MODE, &spanning_instance_cmd);
  cli_install (ctree, MST_CONFIG_MODE, &spanning_instance_vlan_cmd);
  cli_install (ctree, MST_CONFIG_MODE, &no_spanning_instance_cmd);
  cli_install (ctree, MST_CONFIG_MODE, &no_spanning_instance_vlan_cmd);
#endif /* HAVE_MSTPD */

#if defined (HAVE_PROVIDER_BRIDGE) || defined (HAVE_B_BEB)
cli_install (ctree, CONFIG_MODE,
             &spanning_tree_te_msti_configuration_cmd);
cli_install (ctree, CONFIG_MODE,
             &spanning_tree_bridge_te_msti_cmd);
cli_install (ctree, CONFIG_MODE,
             &no_spanning_tree_bridge_te_msti_cmd);

cli_install (ctree, TE_MSTI_CONFIG_MODE,
             &bridge_te_msti_vlan_cmd);
cli_install (ctree, TE_MSTI_CONFIG_MODE,
             &no_bridge_te_msti_vlan_cmd);

#endif /*(HAVE_PROVIDER_BRIDGE) || (HAVE_B_BEB)*/

#ifdef HAVE_PROVIDER_BRIDGE
  cli_install (ctree, INTERFACE_MODE, &mstp_cus_bridge_priority_cmd);
  cli_install (ctree, INTERFACE_MODE, &no_mstp_cus_bridge_priority_cmd);
  cli_install (ctree, INTERFACE_MODE, &mstp_cus_bridge_forward_time_cmd);
  cli_install (ctree, INTERFACE_MODE, &no_mstp_cus_bridge_forward_time_cmd);
  cli_install (ctree, INTERFACE_MODE, &mstp_cus_bridge_hello_time_cmd);
  cli_install (ctree, INTERFACE_MODE, &no_mstp_cus_bridge_hello_time_cmd);
  cli_install (ctree, INTERFACE_MODE, &mstp_cus_bridge_max_age_cmd);
  cli_install (ctree, INTERFACE_MODE, &no_mstp_cus_bridge_max_age_cmd);
  cli_install (ctree, INTERFACE_MODE, &mstp_cus_bridge_transmit_hold_count_cmd);
  cli_install (ctree, INTERFACE_MODE, &no_mstp_cus_bridge_transmit_hold_count_cmd);
  cli_install (ctree, INTERFACE_MODE, &mstp_cus_bridge_group_priority_cmd);
  cli_install (ctree, INTERFACE_MODE, &mstp_cus_pe_bridge_group_priority_cmd);
  cli_install (ctree, INTERFACE_MODE, &mstp_cus_bridge_group_path_cost_cmd);
  cli_install (ctree, INTERFACE_MODE, &mstp_cus_pe_bridge_group_path_cost_cmd);
  cli_install (ctree, INTERFACE_MODE, &no_mstp_cus_bridge_group_path_cost_cmd);
  cli_install (ctree, INTERFACE_MODE, &no_mstp_cus_pe_bridge_group_path_cost_cmd);
#endif /* HAVE_PROVIDER_BRIDGE */

#ifdef HAVE_L2GP
  cli_install (ctree, INTERFACE_MODE, &mstp_add_l2gp_cmd);
  cli_install (ctree, INTERFACE_MODE, &mstp_add_l2gp_rx_statuscmd);
  cli_install (ctree, INTERFACE_MODE, &no_mstp_add_l2gp_cmd);
#endif /* HAVE_L2GP */

#ifdef HAVE_RPVST_PLUS
  cli_install_config (ctree, RPVST_PLUS_CONFIG_MODE, rpvst_plus_config_write);
  cli_install_default (ctree, RPVST_PLUS_CONFIG_MODE);
  cli_install (ctree, CONFIG_MODE, &rpvst_plus_config_cmd);
  cli_install (ctree, RPVST_PLUS_CONFIG_MODE, &bridge_vlan_cmd);
  cli_install (ctree, RPVST_PLUS_CONFIG_MODE, &no_bridge_vlan_cmd);
  cli_install (ctree, EXEC_MODE, &show_spanning_tree_rpvst_cmd);

  cli_install (ctree, CONFIG_MODE, &spanning_vlan_priority_cmd);
  cli_install (ctree, CONFIG_MODE, &no_spanning_vlan_priority_cmd);

  cli_install (ctree, EXEC_MODE, &show_spanning_tree_rpvst_interface_cmd);
  cli_install (ctree, EXEC_MODE, &show_spanning_tree_rpvst_detail_cmd);
  cli_install (ctree, EXEC_MODE,
      &show_spanning_tree_rpvst_detail_interface_cmd);
  cli_install (ctree, EXEC_MODE,
      &show_spanning_tree_rpvst_vlan_interface_cmd);

  cli_install (ctree, EXEC_MODE, &show_spanning_tree_rpvst_config_cmd);
  cli_install (ctree, EXEC_MODE, &show_spanning_tree_rpvst_vlan_cmd);

  cli_install (ctree, INTERFACE_MODE, &show_spanning_tree_rpvst_cmd);
  cli_install (ctree, INTERFACE_MODE, &show_spanning_tree_rpvst_detail_cmd);
  cli_install (ctree, INTERFACE_MODE, &show_spanning_tree_rpvst_config_cmd);
  cli_install (ctree, INTERFACE_MODE, &show_spanning_tree_rpvst_vlan_cmd);

  cli_install (ctree, CONFIG_MODE, &bridge_vlan_priority_cmd);
  cli_install (ctree, CONFIG_MODE, &no_bridge_vlan_priority_cmd);
  cli_install (ctree, INTERFACE_MODE, &bridge_group_vlan_cmd);
  cli_install (ctree, INTERFACE_MODE, &no_bridge_group_vlan_cmd);
  cli_install (ctree, INTERFACE_MODE, &spanning_vlanif_cmd);
  cli_install (ctree, INTERFACE_MODE, &no_spanning_vlanif_cmd);
  cli_install (ctree, INTERFACE_MODE, &bridge_group_vlan_priority_cmd);
  cli_install (ctree, INTERFACE_MODE, &no_bridge_group_vlan_priority_cmd);
  cli_install (ctree, INTERFACE_MODE, &spanning_port_vlan_priority_cmd);
  cli_install (ctree, INTERFACE_MODE, &no_spanning_port_vlan_priority_cmd);
  cli_install (ctree, INTERFACE_MODE, &bridge_group_vlan_path_cost_cmd);
  cli_install (ctree, INTERFACE_MODE, &no_bridge_group_vlan_path_cost_cmd);

  cli_install (ctree, INTERFACE_MODE, &spanning_vlan_path_cost_cmd);
  cli_install (ctree, INTERFACE_MODE, &no_spanning_vlan_path_cost_cmd);

  cli_install (ctree, RPVST_PLUS_CONFIG_MODE, &spanning_vlan_cmd);
  cli_install (ctree, RPVST_PLUS_CONFIG_MODE, &no_spanning_vlan_cmd);
  cli_install (ctree, INTERFACE_MODE, &mstp_spanning_tree_vlan_restricted_tcn_cmd);
  cli_install (ctree, INTERFACE_MODE, &mstp_spanning_tree_vlan_restricted_role_cmd);
  cli_install (ctree, INTERFACE_MODE, &no_mstp_spanning_tree_vlan_restricted_tcn_cmd);
  cli_install (ctree, INTERFACE_MODE, &no_mstp_spanning_tree_vlan_restricted_role_cmd);

#endif /* HAVE_RPVST_PLUS */

  mstp_cli_init_debug (ctree);
  
  if_add_hook (&mstpm->ifg, IF_CALLBACK_NEW, mstp_interface_new_hook);
  if_add_hook (&mstpm->ifg, IF_CALLBACK_DELETE, mstp_interface_delete_hook);

#ifdef MEMMGR
  /* Init Memory Manager CLIs. */
  memmgr_cli_init (mstpm);
#endif /* MEMMGR */

}

/* Show bridge configuration.  */
int
mstp_bridge_config_write (struct cli *cli)
{
  struct mstp_bridge * br;
  int write = 0;
  int index = 0;

  br = mstp_get_first_bridge ();

  while ( br )
    {
      write++;

      if (BRIDGE_TYPE_CVLAN_COMPONENT (br))
        {
          br = br->next;
          continue;
        }
         
      if (br->ageing_time != BRIDGE_TIMER_DEFAULT_AGEING_TIME)
        cli_out (cli, "%s ageing-time %d\n",
                 mstp_get_bridge_str_span (br), br->ageing_time);
    
      if (br->type == MSTP_BRIDGE_TYPE_STP
          || br->type == MSTP_BRIDGE_TYPE_STP_VLANAWARE)
        {
          if (br->path_cost_method != MSTP_PATHCOST_SHORT)
            cli_out (cli, "%s spanning-tree pathcost method long\n",
                     mstp_get_bridge_str_span (br));
        }
      else
        {
          if (br->path_cost_method != MSTP_PATHCOST_LONG)
            cli_out (cli, "%s spanning-tree pathcost method short\n",
                     mstp_get_bridge_str_span (br));
        }
 
      if (br->cist_bridge_priority != BRIDGE_DEFAULT_PRIORITY)
        cli_out (cli, "%s priority %u\n",
                 mstp_get_bridge_str_span (br), br->cist_bridge_priority);
      
      if (br->bridge_forward_delay / L2_TIMER_SCALE_FACT != 
          BRIDGE_TIMER_DEFAULT_FWD_DELAY)
        cli_out (cli, "%s forward-time %d\n",
                 mstp_get_bridge_str_span (br),
                 br->bridge_forward_delay / L2_TIMER_SCALE_FACT);
      
      if ((! IS_BRIDGE_MSTP (br))
          && (br->bridge_hello_time / L2_TIMER_SCALE_FACT != 
              BRIDGE_TIMER_DEFAULT_HELLO_TIME))
        cli_out (cli, "%s hello-time %d\n",
                 mstp_get_bridge_str_span (br),
                 br->bridge_hello_time / L2_TIMER_SCALE_FACT);
      
      if (br->bridge_max_age / L2_TIMER_SCALE_FACT != 
          BRIDGE_TIMER_DEFAULT_MAX_AGE)
        cli_out (cli, "%s max-age %d\n",
                  mstp_get_bridge_str_span (br),
                  br->bridge_max_age / L2_TIMER_SCALE_FACT);

      /* Max Hops */
      if (br->bridge_max_hops != MST_DEFAULT_MAXHOPS)
        cli_out (cli, "%s max-hops %d\n",
                  mstp_get_bridge_str_span (br), br->bridge_max_hops);


      if (br->mstp_enabled == PAL_FALSE)
        {
          if (br->is_default)
            cli_out (cli, "spanning-tree shutdown\n");
          else
            {
              if (br->mstp_brforward == PAL_TRUE)
                cli_out (cli, "bridge shutdown %s bridge-forward\n", br->name);
              else
                cli_out (cli, "bridge shutdown %s\n", br->name);
            }
        }

      /* force-version */
      if (((br->force_version != BR_VERSION_MSTP)
          &&( br->type == NSM_BRIDGE_TYPE_MSTP))
          || ((br->force_version != BR_VERSION_RSTP)
          && ((br->type == NSM_BRIDGE_TYPE_RSTP)
          || (br->type == NSM_BRIDGE_TYPE_RSTP_VLANAWARE))))
        cli_out (cli, "%sspanning-tree force-version %d\n",
                 mstp_get_bridge_str (br), br->force_version);

      if (br->bpduguard == PAL_TRUE)
        cli_out (cli, "%sspanning-tree portfast bpdu-guard\n",
                 mstp_get_bridge_str (br));

      if (br->errdisable_timeout_enable)
        cli_out (cli, "%sspanning-tree errdisable-timeout enable\n",
                 mstp_get_bridge_str (br));

      if (br->errdisable_timeout_interval !=
          MSTP_BRIDGE_ERRDISABLE_DEFAULT_TIMEOUT_INTERVAL)
        cli_out (cli, "%sspanning-tree errdisable-timeout interval %d\n",
                  mstp_get_bridge_str (br),
                  br->errdisable_timeout_interval / L2_TIMER_SCALE_FACT);

      if (br->bpdu_filter != PAL_FALSE)
        cli_out (cli, "%sspanning-tree portfast bpdu-filter\n",
                 mstp_get_bridge_str (br));

      if (br->admin_cisco)
        cli_out (cli, "%s cisco-interoperability enable\n",
                 mstp_get_bridge_str_span (br));

      if (br->transmit_hold_count != MSTP_TX_HOLD_COUNT)
        cli_out (cli, "%s transmit-holdcount %d\n",
                 mstp_get_bridge_str_span (br),
                 br->transmit_hold_count);

      /* Add each instance */
      for (index = 1; index < br->max_instances; index++)
        {
          struct mstp_bridge_instance *br_inst = br->instance_list[index];

          if (!br_inst)
            continue;

          if (br_inst->msti_bridge_priority != BRIDGE_DEFAULT_PRIORITY)
            {
#ifdef HAVE_RPVST_PLUS
              if (br->type == NSM_BRIDGE_TYPE_RPVST_PLUS)
                rpvst_plus_bridge_config_write(cli, br_inst, br);
              else if(br->type != NSM_BRIDGE_TYPE_RPVST_PLUS)
#endif /* HAVE_RPVST_PLUS */
                cli_out (cli,"%s instance %d priority %u\n",
                    mstp_get_bridge_str_span (br),
                    index, br_inst->msti_bridge_priority);
            }
        }

      br = br->next;
    }

  if (write)
    cli_out (cli, "!\n");
  return write;
}

#ifdef HAVE_RPVST_PLUS
void
rpvst_plus_bridge_config_write (struct cli * cli, 
                                struct mstp_bridge_instance *br_inst,
                                struct mstp_bridge *br)
{
    struct rlist_info *vlan_list = NULL;
    int vid = 0;

    vlan_list = br_inst->vlan_list;

    if (vlan_list)
      vid = vlan_list->lo;

    cli_out(cli,"%s vlan %d priority %u\n",
        mstp_get_bridge_str_span (br),
        vid, br_inst->msti_bridge_priority);
    return;
}
#endif /* HAVE_RPVST_PLUS */

static inline void
mstp_display_bridge_id (struct cli * cli, char *prefix,
                        char *id_type, u_char *idPtr, int tab)
{
  char *margin = "  ";
  

  cli_out (cli, "%% %s%s: %s %.2x%.2x%.2x%.2x%.2x%.2x%.2x%.2x\n",
           (tab) ? margin : "",
           prefix,
           id_type,
           idPtr[0],
           idPtr[1],
           idPtr[2],
           idPtr[3],
           idPtr[4],
           idPtr[5],
           idPtr[6],
           idPtr[7]);

}

#ifdef HAVE_MSTPD
static void 
mstp_display_msti_port (struct mstp_instance_port *inst_port, 
                        struct cli * cli)
{
  u_char * rootPtr;
  struct mstp_port *port = inst_port ? inst_port->cst_port : NULL;
  u_char * idPtr;
  u_char version; 

  if (!port)
    return;

  rootPtr = (u_char *)&(inst_port->msti_root);
  idPtr = (u_char *)&(inst_port->designated_bridge);

  version = (port->force_version > 3) ? 1 : port->force_version;
  
  cli_out (cli, "%%   %s: Port Number %u - Ifindex %u - Port Id %hx "
           "- Role %s - State %s\n",
           port->name,
           (port->ifindex & 0x0fff),
           port->ifindex,
           port->cist_port_id,
           portRoleStr[inst_port->role],
           portStateStr[inst_port->state]);

  cli_out (cli, "%%   %s: Designated Internal Path Cost %u  "
           "- Designated Port Id %hx \n",
           port->name,
           inst_port->internal_root_path_cost,
           inst_port->designated_port_id);

  cli_out (cli, "%%   %s: Configured Internal Path Cost %u \n",
           port->name,
           inst_port->msti_path_cost);

  cli_out (cli, "%%   %s: Configured CST External Path cost %u \n",
           port->name,
           port->cist_path_cost);

  cli_out (cli, "%%   %s: CST Priority %d  - "
           "MSTI Priority %d \n",
           port->name,
           port->cist_priority,
           inst_port->msti_priority);
  
  mstp_display_bridge_id (cli, port->name, "Designated Root", rootPtr, PAL_TRUE);
  mstp_display_bridge_id (cli, port->name, "Designated Bridge", idPtr, PAL_TRUE);

  cli_out (cli, "%%   %s: Message Age %d - Max Age %d\n",
           port->name,
           inst_port->message_age / MSTP_TIMER_SCALE_FACT,
           inst_port->max_age / MSTP_TIMER_SCALE_FACT);

  cli_out (cli, "%%   %s: Hello Time %d - Forward Delay %d\n",
           port->name,
           port->cist_hello_time / MSTP_TIMER_SCALE_FACT,
           port->cist_fwd_delay / MSTP_TIMER_SCALE_FACT);
  
  cli_out (cli, "%%   %s: Forward Timer %d - Msg Age Timer %d"
           " - Hello Timer %d\n",
           port->name,
           l2_timer_get_remaining_secs (inst_port->forward_delay_timer),
           l2_timer_get_remaining_secs (inst_port->message_age_timer),
           l2_timer_get_remaining_secs (port->hello_timer));
}
#endif /* HAVE_MSTPD */


static void 
mstp_display_cist_port (struct mstp_port *port, struct cli * cli)
{
  u_char * rootPtr;
  u_char * reg_rootPtr;
  u_char * idPtr;
  u_char version;
  char ifname [MSTP_CLI_IFNAME_LEN];
  struct mstp_bridge *br = NULL;

  if (!port)
    return;

  br = port->br;
  rootPtr = (u_char *)&(port->cist_root);

#ifdef HAVE_RPVST_PLUS
  reg_rootPtr = (IS_BRIDGE_MSTP (br) || IS_BRIDGE_RPVST_PLUS(br)) ? 
                 (u_char *)&(port->cist_reg_root) : NULL;
#else
  reg_rootPtr = (IS_BRIDGE_MSTP (br) )? (u_char *)&(port->cist_reg_root) : NULL;
#endif /* HAVE_RPVST_PLUS */
  idPtr = (u_char *)&(port->cist_designated_bridge);

  pal_mem_set (ifname, 0, MSTP_CLI_IFNAME_LEN);
  version = (port->force_version > 3) ? 1 : port->force_version;

#ifdef HAVE_PROVIDER_BRIDGE
  if (port->port_type == MSTP_VLAN_PORT_MODE_PE)
    pal_snprintf (ifname, MSTP_CLI_IFNAME_LEN, "%s.%u", port->name, port->svid);
  else
#endif /* HAVE_PROVIDER_BRIDGE */
    pal_snprintf (ifname, L2_IF_NAME_LEN, "%s", port->name);

#ifdef HAVE_PROVIDER_BRIDGE
  if (port->port_type == MSTP_VLAN_PORT_MODE_PE)
    cli_out (cli, "%%   %s: Port Number %u - Ifindex %u SVLAN %u - "
             "Port Id %hx - Role %s - State %s\n",
             ifname,
             (port->ifindex & 0x0fff),
             port->ifindex,
             port->svid,
             port->cist_port_id,
             portRoleStr[port->cist_role],
             mstp_port_get_cist_port_state (port));
  else
#endif /* HAVE_PROVIDER_BRIDGE */
    cli_out (cli, "%%   %s: Port Number %u - Ifindex %u - Port Id %hx "
             "- Role %s - State %s\n",
             ifname,
             (port->ifindex & 0x0fff),
             port->ifindex,
             port->cist_port_id,
             portRoleStr[port->cist_role],
             mstp_port_get_cist_port_state (port));

#ifdef HAVE_RPVST_PLUS
  if (IS_BRIDGE_MSTP (br) || IS_BRIDGE_RPVST_PLUS (br))
#else
  if (IS_BRIDGE_MSTP (br)) 
#endif /* HAVE_RPVST_PLUS */
    cli_out (cli, "%%   %s: Designated External Path Cost %u -"
             "Internal Path Cost %u  \n",
             ifname,
             port->cist_external_rpc,
             port->cist_internal_rpc);
   else
     cli_out (cli, "%%   %s: Designated Path Cost %u\n",
              ifname,
              port->cist_external_rpc);

  cli_out (cli, "%%   %s: Configured Path Cost %u  "
           "- Add type %s"
           " ref count %d\n",
           ifname,
           port->cist_path_cost,
           addtype_str[port->type],
           port->ref_count);

  cli_out (cli, "%%   %s: Designated Port Id %hx -%s Priority %d  - \n",
           ifname,
           port->cist_designated_port_id,
           CIST (br),
           port->cist_priority);

#ifdef HAVE_RPVST_PLUS
  if (IS_BRIDGE_MSTP (br) || IS_BRIDGE_RPVST_PLUS(br))
#else
  if (IS_BRIDGE_MSTP (br))
#endif /* HAVE_RPVST_PLUS */
    {
      if ((!port->spanning_tree_disable) && (port->port_enabled))
      {
        mstp_display_bridge_id (cli, ifname, "CIST Root", rootPtr, PAL_TRUE);
        mstp_display_bridge_id (cli, ifname, "Regional Root", reg_rootPtr,
                                PAL_TRUE);
        mstp_display_bridge_id (cli, ifname, "Designated Bridge", idPtr,
                                PAL_TRUE);
      }
    }
  else
   {
    if ((!port->spanning_tree_disable) && (port->port_enabled)) 
     {
      mstp_display_bridge_id (cli, ifname, "Root", rootPtr, PAL_TRUE);
      mstp_display_bridge_id (cli, ifname, "Designated Bridge", idPtr,
                            PAL_TRUE);
     }
   } 
  cli_out (cli, "%%   %s: Message Age %d - Max Age %d\n",
           ifname,
           port->cist_message_age/L2_TIMER_SCALE_FACT,
           port->cist_max_age/L2_TIMER_SCALE_FACT);

  cli_out (cli, "%%   %s:%s Hello Time %d - Forward Delay %d\n",
           ifname,
           CIST (br),
           port->cist_hello_time/L2_TIMER_SCALE_FACT,
           port->cist_fwd_delay/L2_TIMER_SCALE_FACT);

  cli_out (cli, "%%   %s:%s Forward Timer %d - Msg Age Timer %d"
           " - Hello Timer %d - topo change timer %d\n",
           ifname,
           CIST (br),
           l2_timer_get_remaining_secs (port->forward_delay_timer),
           l2_timer_get_remaining_secs (port->message_age_timer),
           l2_timer_get_remaining_secs (port->hello_timer),
           l2_timer_get_remaining_secs (port->tc_timer));

  cli_out (cli, "%%   %s: forward-transitions %u\n",
           ifname,
           port->cist_forward_transitions);


  cli_out (cli, "%%   %s: Version %s - Received %s - Send %s\n",
           ifname,
           version_str[version],
           port->rcvd_mstp ? ( port->rcvd_rstp ? "RSTP": "MSTP"):  port->rcvd_stp ? "STP" : "None",
           port->send_mstp ? ((port->force_version == 3) ? "MSTP" : "RSTP") : "STP");

  cli_out (cli, "%%   %s: %s - Current %s\n",
           ifname,
           port->admin_edge ?
           (port->portfast_conf ? "Portfast configured" : "Edgeport configured")
           : "No portfast configured",
           port->oper_edge ?
           (port->portfast_conf ? "portfast on" : "edgeport on")
           : " portfast off");

  cli_out (cli, "%%   %s: %s %s  - Current %s\n",
           ifname, "portfast bpdu-guard ",
           portfastStr[port->admin_bpduguard],
           port->oper_bpduguard ? "portfast bpdu-guard on" :
           "portfast bpdu-guard off");

  cli_out (cli, "%%   %s: %s %s  - Current %s\n",
           ifname, "portfast bpdu-filter",
           portfastStr[port->admin_bpdufilter],
           port->oper_bpdufilter ? "portfast bpdu-filter on" :
           "portfast bpdu-filter off");

  cli_out (cli, "%%   %s: %s     - Current %s\n",
           ifname, port->admin_rootguard ?
           "root guard configured" : "no root guard configured",
           port->oper_rootguard ? "root guard on" :
           "root guard off");

  cli_out (cli, "%%   %s: Configured Link Type %s - Current %s\n",
           ifname,
           port->admin_p2p_mac == MSTP_ADMIN_LINK_TYPE_AUTO ? "auto" :
           port->admin_p2p_mac ? "point-to-point" : "shared",
           port->oper_p2p_mac ? "point-to-point" : "shared");

  cli_out (cli, "%%   %s: %s configured - Current port Auto Edge %s\n",
           ifname,
           port->auto_edge ? "Auto-edge" : "No auto-edge",
           port->auto_edge ? "on" : "off");

#ifdef HAVE_L2GP
       if ((port->isL2gp) && IS_BRIDGE_MSTP (br))
       {
          cli_out (cli, "%%   %s: enabled L2GP with %s %s\n",
               ifname,
               ((port->enableBPDUrx == PAL_TRUE) ?
               "enableBPDUrx" : "disableBPDUrx"), 
               ((port->enableBPDUtx == PAL_TRUE) ?
               "enableBPDUtx" : "disableBPDUtx")); 

       }
#endif /* HAVE_L2GP */

}


#ifdef HAVE_PROVIDER_BRIDGE

static void
mstp_display_customer_edge_info ( struct mstp_port *port, struct cli *cli )
{
  u_char * idPtr;
  u_char * rootPtr;
  struct mstp_bridge *br;
  char buf[MSTP_TIME_LENGTH];

  br = port->ce_br;

  if (br == NULL)
    return;

  cli_out (cli, "%% Customer Edge Spanning Tree Detail For %s: \n", port->name);

  cli_out (cli, "%% Bridge %s - Spanning Tree %s %s %s\n",
           br->bridge_enabled ? "up" : "down",
           boolean_str[br->mstp_enabled] ,
           br->topology_change ? "- topology change" : "",
           br->topology_change_detected ? "- topology change detected" : "");

  cli_out (cli, "%% %s:%s Root Path Cost %u -%s Root Port %d"
           " - %s Bridge Priority %u\n",
           br->name,
           CIST (br),
           br->external_root_path_cost,
           CIST (br),
           br->cist_root_port_ifindex,
           CIST (br),
           br->cist_bridge_priority);

  cli_out (cli, "%% %s: Forward Delay %d - Hello Time %d - Max Age %d",
           br->name,
           br->bridge_forward_delay / L2_TIMER_SCALE_FACT,
           br->bridge_hello_time / L2_TIMER_SCALE_FACT,
           br->bridge_max_age / L2_TIMER_SCALE_FACT);

  cli_out (cli, "\n");

  rootPtr = (u_char *)&(br->cist_designated_root);
  idPtr = (u_char *)&(br->cist_bridge_id);

  mstp_display_bridge_id (cli, br->name, "Root Id",
                          rootPtr, PAL_FALSE);

  mstp_display_bridge_id (cli, br->name, "Bridge Id",
                          idPtr, PAL_FALSE);

  pal_time_calendar (&br->time_last_topo_change, buf);

  cli_out (cli, "%% %s: last topology change %s",
           br->name,
           buf);

  for (port = br->port_list; port;port = port->next)
    {
      mstp_display_cist_port (port, cli);
      cli_out (cli, "%% \n");
    }

}

#endif /* HAVE_PROVIDER_BRIDGE */

static void 
mstp_display_cist_info (struct mstp_bridge * br, struct interface *ifp, 
                        struct cli *cli, int detail_flag)
{

  u_char * rootPtr;
  u_char * reg_rootPtr;
  u_char * idPtr;
  char buf[MSTP_TIME_LENGTH];

#ifdef HAVE_PROVIDER_BRIDGE
  if (br != NULL
      && BRIDGE_TYPE_CVLAN_COMPONENT (br))
    return;
#endif /* HAVE_PROVIDER_BRIDGE */
  

  cli_out (cli, "%% %s: Bridge %s - Spanning Tree %s %s %s\n",
           br->is_default ? "Default" : br->name,
           br->bridge_enabled ? "up" : "down",
           boolean_str[br->mstp_enabled] ,
           br->topology_change ? "- topology change" : "",
           br->topology_change_detected ? "- topology change detected" : "");

  cli_out (cli, "%% %s:%s Root Path Cost %u -%s Root Port %d"
           " - %s Bridge Priority %u\n",
           br->is_default ? "Default" : br->name,
           CIST (br),
           br->external_root_path_cost,
           CIST (br),
           br->cist_root_port_ifindex,
           CIST (br),
           br->cist_bridge_priority);

  cli_out (cli, "%% %s: Forward Delay %d - Hello Time %d - Max Age %d"
           " - Transmit Hold Count %d",
           br->is_default ? "Default" : br->name,
           br->bridge_forward_delay / L2_TIMER_SCALE_FACT,
           br->bridge_hello_time / L2_TIMER_SCALE_FACT, 
           br->bridge_max_age / L2_TIMER_SCALE_FACT,
           br->transmit_hold_count);

  if (IS_BRIDGE_MSTP (br))
    cli_out (cli, " - Max-hops %d\n", br->bridge_max_hops);
  else
    cli_out (cli, "\n");

  rootPtr = (u_char *)&(br->cist_designated_root);
  reg_rootPtr = IS_BRIDGE_MSTP (br) ? (u_char *)&(br->cist_reg_root) : NULL;
  idPtr = (u_char *)&(br->cist_bridge_id);

  if (IS_BRIDGE_MSTP (br))
    {
      mstp_display_bridge_id (cli, br->is_default ? "Default" : br->name,
                              "CIST Root Id",
                              rootPtr, PAL_FALSE);
      mstp_display_bridge_id (cli, br->is_default ? "Default" : br->name,
                              "CIST Reg Root Id", 
                              reg_rootPtr, PAL_FALSE);
      mstp_display_bridge_id (cli, br->is_default ? "Default" : br->name,
                              "CIST Bridge Id",
                              idPtr, PAL_FALSE);
    }
#ifdef HAVE_RPVST_PLUS
  else if (IS_BRIDGE_RPVST_PLUS (br))
    {
      mstp_display_bridge_id (cli, br->is_default ? "Default" : br->name,
                              "CIST Root Id",
                              rootPtr, PAL_FALSE);
      mstp_display_bridge_id (cli, br->is_default ? "Default" : br->name,
                              "CIST Bridge Id",
                              idPtr, PAL_FALSE);
    }
#endif /* HAVE_RPVST_PLUS */
  else
    {
      mstp_display_bridge_id (cli, br->is_default ? "Default" : br->name,
                              "Root Id", rootPtr, PAL_FALSE);
      mstp_display_bridge_id (cli, br->is_default ? "Default" : br->name,
                              "Bridge Id", idPtr, PAL_FALSE);

      pal_time_calendar (&br->time_last_topo_change, buf);
      cli_out (cli, "%% %s: last topology change %s",
               br->is_default ? "Default" : br->name,
               buf);

    }
  cli_out(cli,"%% %s: %d topology change(s) ",br->name,br->num_topo_changes);
  pal_time_calendar (&br->time_last_topo_change, buf);
  cli_out (cli, " - last topology change %s\n", buf);

  cli_out (cli, "%% %s: portfast bpdu-filter %s\n",
           br->is_default ? "Default" : br->name,
           br->bpdu_filter ? "enabled" : "disabled");
  cli_out (cli, "%% %s: portfast bpdu-guard %s\n",
           br->is_default ? "Default" : br->name,
           br->bpduguard ? "enabled" : "disabled");
  cli_out (cli, "%% %s: portfast errdisable timeout %s\n",
           br->is_default ? "Default" : br->name, 
           br->errdisable_timeout_enable ?  "enabled" : "disabled");
  cli_out (cli, "%% %s: portfast errdisable timeout interval %d sec\n",
           br->is_default ? "Default" : br->name,
           br->errdisable_timeout_interval / L2_TIMER_SCALE_FACT);

  if (detail_flag)
    {
      struct mstp_port *port;

      if (! ifp)
        {
          for (port = br->port_list; port;port = port->next)
           {
 
#ifdef HAVE_PROVIDER_BRIDGE
             if (BRIDGE_TYPE_PROVIDER (br)
                 && port->port_type == MSTP_VLAN_PORT_MODE_CE)
               continue;
#endif /* HAVE_PROVIDER_BRIDGE */
             mstp_display_cist_port (port, cli);
             cli_out (cli, "%% \n");
           }

#ifdef HAVE_PROVIDER_BRIDGE
         if (BRIDGE_TYPE_PROVIDER (br))
           {
             for (port = br->port_list; port;port = port->next)
              {
                if (port->port_type == MSTP_VLAN_PORT_MODE_CE)
                  mstp_display_customer_edge_info (port, cli);
              }
           }
#endif /* HAVE_PROVIDER_BRIDGE */
        }
      else
        {
          for (port = br->port_list; port;port = port->next)
           {
             if (port->ifindex ==  ifp->ifindex)
               {
#ifdef HAVE_PROVIDER_BRIDGE
                 if (BRIDGE_TYPE_PROVIDER (br)
                     && port->port_type == MSTP_VLAN_PORT_MODE_CE)
                    mstp_display_customer_edge_info (port, cli);
                 else
#endif /* HAVE_PROVIDER_BRIDGE */
                   mstp_display_cist_port (port, cli);
                  cli_out (cli, "%% \n");
                  break;
               }
           }
        }
    }
}

#ifdef HAVE_MSTPD
static void
mstp_display_instance_info ( struct mstp_bridge * br,
                             struct interface *ifp, struct cli * cli)
{
  int instance_index ;
  struct mstp_bridge_instance *br_inst;
  struct mstp_instance_port *inst_port;
  char *margin = "  ";
  char *delim = "      ";
 
  if (!br)
     return;

  cli_out (cli,"%%%s Instance %s VLAN\n",margin,delim); 
  

  cli_out (cli,"%%%s 0:       %s ",margin,delim);
  mstp_display_msti_vlan_list (cli, br->vlan_list);

  for (instance_index = 1;instance_index < br->max_instances; instance_index++)
    {
      if (!br->instance_list[instance_index])
        continue;
      
      br_inst = br->instance_list[instance_index];
      for (inst_port = br_inst->port_list; inst_port;
           inst_port = inst_port->next)
      {
        if (ifp && inst_port->ifindex != ifp->ifindex)
          continue;
        cli_out (cli,"%%%s %d:       %s ",margin,instance_index,delim);
        mstp_display_msti_vlan_list (cli, br_inst->vlan_list);
        if (ifp)
          break;
      }
    }
}


static void
mstp_display_instance_info_det (struct mstp_bridge * br,
                                struct interface *ifp, struct cli * cli)
{
  int instance_index ;
  struct mstp_bridge_instance *br_inst;
  struct mstp_instance_port *inst_port;
  struct rlist_info *rlist;
  u_char * rootPtr;
  bool_t display;
  u_char * idPtr;

  display = PAL_FALSE;
  inst_port = NULL;
  br_inst = NULL;
 
  if (!br)
    return;

  mstp_display_cist_info (br, ifp, cli, PAL_TRUE); 

  for (instance_index = 1;instance_index < br->max_instances;
       instance_index++)
    {
      if (!br->instance_list[instance_index])
        continue;

      br_inst = br->instance_list[instance_index];

      display = PAL_FALSE;

      for (inst_port = br_inst->port_list; inst_port;
           inst_port = inst_port->next)
        {
          if (ifp && inst_port->ifindex != ifp->ifindex)
            continue;

          if (! display)
            {
              cli_out (cli,"%% Instance  %d:  Vlans: ", instance_index);
              for (rlist = br_inst->vlan_list; rlist != NULL;
                   rlist = rlist->next)
                {
                  if (rlist->lo != rlist->hi)
                    {
                      if (rlist->next)
                        cli_out (cli, "%d-%d, ", rlist->lo, rlist->hi);
                      else
                        cli_out (cli, "%d-%d", rlist->lo, rlist->hi);
                    }
                  else
                    {
                      if (rlist->next)
                        cli_out (cli, "%d, ", rlist->lo);
                      else
                        cli_out (cli, "%d", rlist->lo);
                    }
                }

                cli_out (cli, "\n");
                cli_out (cli, "%% %s: MSTI Root Path Cost %u -"
                         "MSTI Root Port %d"
                         " - MSTI Bridge Priority %u\n",
                         br->is_default ? "Default" : br->name,
                         br_inst->internal_root_path_cost,
                         br_inst->msti_root_port_ifindex,
                         br_inst->msti_bridge_priority);
                rootPtr = (u_char *)&(br_inst->msti_designated_root);
                idPtr = (u_char *)&(br_inst->msti_bridge_id);
                mstp_display_bridge_id (cli, br->is_default ? "Default" : br->name,
                                        "MSTI Root Id",
                                        rootPtr, PAL_FALSE);
                mstp_display_bridge_id (cli, br->is_default ? "Default" : br->name,
                                        "MSTI Bridge Id",
                                        idPtr, PAL_FALSE);
                display = PAL_TRUE;  
            }
            mstp_display_msti_port (inst_port, cli);

            if (ifp)
              break;
          }
        
      cli_out (cli,"\n");
    } 
}

static void
mstp_display_single_instance_info_det (struct mstp_bridge_instance * br_inst,
                                       struct interface *ifp, struct cli * cli)
{
  struct mstp_instance_port *inst_port;
  u_char * rootPtr;
  u_char * idPtr;
 
  if (br_inst == NULL)
    return;  
      
  cli_out (cli, "%% %s: MSTI Root Path Cost %u - MSTI Root Port %d"
           " - MSTI Bridge Priority %u\n",
           br_inst->bridge->name,
           br_inst->internal_root_path_cost,
           br_inst->msti_root_port_ifindex,
           br_inst->msti_bridge_priority);
   
  rootPtr = (u_char *)&(br_inst->msti_designated_root);
  idPtr = (u_char *)&(br_inst->msti_bridge_id);
  mstp_display_bridge_id (cli, br_inst->bridge->name, "MSTI Root Id", 
                          rootPtr, PAL_FALSE);
  mstp_display_bridge_id (cli, br_inst->bridge->name, "MSTI Bridge Id", 
                          idPtr, PAL_FALSE);
  if (! ifp)
    {
      for (inst_port = br_inst->port_list; inst_port;
           inst_port = inst_port->next )
        {
          mstp_display_msti_port (inst_port, cli);
          cli_out (cli, "%% \n");
        }
    }
  else
    {
      for (inst_port = br_inst->port_list; inst_port;
           inst_port = inst_port->next )
        {
          if (inst_port->ifindex == ifp->ifindex)
            {
              mstp_display_msti_port (inst_port, cli);
              cli_out (cli, "%% \n");
              break;
            }
        }
    }

  cli_out (cli,"\n");
  
}

static void 
mstp_display_msti_vlan_list (struct cli *cli, struct rlist_info *vlan_list)
{
  while (vlan_list)
    {
      cli_out(cli, "%d", vlan_list->lo);
      if (vlan_list->lo != vlan_list->hi)
        {
          cli_out (cli, "-%d",vlan_list->hi);
        }
    
      if (vlan_list->next)
        cli_out (cli, ", ");

      vlan_list =vlan_list->next;
    }
  cli_out (cli, "\n");
    
}

static void 
mstp_display_msti_vlan_range_idx_list (struct cli *cli, 
                                       struct rlist_info *vlan_list)
{
  cli_out(cli, "%d", vlan_list->lo);
  if (vlan_list->lo != vlan_list->hi)
    {
      cli_out (cli, "-%d",vlan_list->hi);
    }

  cli_out (cli, "\t");
  cli_out (cli, " %d ", vlan_list->vlan_range_indx);
  cli_out (cli, "\n");
  
}

#endif /* HAVE_MSTPD */

static void
mstp_display_stp_port (struct mstp_port *port, struct cli * cli)
{
  u_char * rootPtr;
  u_char * idPtr;
  struct mstp_bridge *br = NULL;

  if (!port)
    return;

  br = port->br;

  rootPtr = (u_char *)&(port->cist_root);
  idPtr = (u_char *)&(port->cist_designated_bridge);

  cli_out (cli, "%%   %s: Port Number %u - Ifindex %u - Port Id %hx - "
           "path cost %u - designated cost %u\n",
           port->name,
           (port->ifindex & 0x0fff),
           port->ifindex,
           port->cist_port_id,
           port->cist_path_cost,
           port->cist_external_rpc);

  cli_out (cli, "%%   %s: Designated Port Id %hx -%s state %s -"
           "Priority %d   \n",
           port->name,
           port->cist_designated_port_id,
           CIST (br),
           mstp_port_get_cist_port_state (port),
           port->cist_priority);

 if ((!port->spanning_tree_disable) && (port->port_enabled))
   {

     mstp_display_bridge_id (cli, port->name, "Designated root",
                          rootPtr, PAL_TRUE);

     mstp_display_bridge_id (cli, port->name, "Designated Bridge", idPtr,
                          PAL_TRUE);
   }

  cli_out (cli, "%%   %s: Message Age %d - Max Age %d\n",
           port->name,
           port->cist_message_age/L2_TIMER_SCALE_FACT,
           port->cist_max_age/L2_TIMER_SCALE_FACT);

  cli_out (cli, "%%   %s: Hello Time %d - Forward Delay %d\n",
           port->name,
           port->cist_hello_time/L2_TIMER_SCALE_FACT,
           port->cist_fwd_delay/L2_TIMER_SCALE_FACT);

  cli_out (cli, "%%   %s:%s Forward Timer %d - Msg Age Timer %d"
           " - Hello Timer %d - topo change timer %d\n",
           port->name,
           CIST (br),
           l2_timer_get_remaining_secs (port->forward_delay_timer),
           l2_timer_get_remaining_secs (port->message_age_timer),
           l2_timer_get_remaining_secs (port->hello_timer),
           l2_timer_get_remaining_secs (port->tc_timer));

  cli_out (cli, "%%   %s: forward-transitions %u\n",
           port->name,
           port->cist_forward_transitions);

  cli_out (cli, "%%   %s: portfast %s\n", port->name,
           port->portfast_conf ? "enabled": "disabled");

  cli_out (cli, "%%   %s: %s %s  - Current %s\n",
           port->name, "portfast bpdu-guard ",
           portfastStr[port->admin_bpduguard],
           port->oper_bpduguard ? "portfast bpdu-guard on" :
           "portfast bpdu-guard off");

  cli_out (cli, "%%   %s: %s %s  - Current %s\n",
           port->name, "portfast bpdu-filter",
           portfastStr[port->admin_bpdufilter],
           port->oper_bpdufilter ? "portfast bpdu-filter on" :
           "portfast bpdu-filter off");

  cli_out (cli, "%%   %s: %s     - Current %s\n",
           port->name, port->admin_rootguard ?
           "root guard configured" : "no root guard configured",
           port->oper_rootguard ? "root guard on" :
           "root guard off");

}
static int 
mstp_stats_display_info (struct mstp_bridge *br,
		         struct interface *ifp,
		         struct cli *cli)
{
  u_char * rootPtr;
  u_char * idPtr = NULL;
  u_char version;
  struct mstp_port *port = NULL;
  char buf[MSTP_TIME_LENGTH];

  if (br->port_list == NULL)
    {
      cli_out (cli, "%% No ports are associated with the bridge\n");
      return CLI_ERROR;
    }

  cli_out (cli, "\n");

  for (port = br->port_list; port; port = port->next)
  {
    if (ifp != NULL)
        if (ifp->ifindex != port->ifindex)
          continue;

    rootPtr = (u_char *)&(port->cist_root.addr);
    idPtr = (u_char *)&(port->cist_designated_bridge.addr);

    cli_out (cli, " \t\tPort number = %d Interface = %s \n", 
             (port->ifindex & 0x0fff), port->name);

    cli_out (cli, " \t\t================================ \n");

    cli_out (cli, "%% BPDU Related Parameters\n");
    cli_out (cli, "%% -----------------------\n");

    cli_out (cli, "%% Port Spanning Tree                 : %s\n",
	     port->port_enabled ? "Enable" :"Disable");

    version = (port->force_version > 3) ? 1 : port->force_version;
    cli_out (cli,"%% Spanning Tree Type                 : %s\n",
             version_str[version]);

    cli_out (cli, "%% Current Port State                 : %s\n",
             mstp_port_get_cist_port_state (port));

    cli_out (cli, "%% Port ID                            : %hx \n",
             port->cist_port_id);

    cli_out (cli, "%% Port Number                        : %hx \n",
             (port->ifindex & 0x0fff));
    
    cli_out (cli, "%% Path Cost                          : %u \n",
             port->cist_path_cost);

    cli_out (cli, "%% Message Age                        : %d\n",
             port->cist_message_age/L2_TIMER_SCALE_FACT);

    cli_out (cli, "%% Designated Root                    :"
             " %.2x:%.2x:%.2x:%.2x:%.2x:%.2x\n",
             rootPtr[0], rootPtr[1],rootPtr[2],
             rootPtr[3],rootPtr[4],rootPtr[5]);

    cli_out (cli,"%% Designated Cost                    : %u\n",
             port->cist_external_rpc);

    cli_out (cli, "%% Designated Bridge                  :"
             " %.2x:%.2x:%.2x:%.2x:%.2x:%.2x\n", 
             idPtr[0], idPtr[1], idPtr[2], 
             idPtr[3], idPtr[4], idPtr[5]);
    
    cli_out (cli, "%% Designated Port Id                 : %hx\n",
             port->cist_designated_port_id);

    cli_out (cli, "%% Top Change Ack                     : %s \n", \
             port->tc_ack ? "TRUE" :"FALSE");

    cli_out (cli, "%% Config Pending                     : %s \n",
             port->config_bpdu_pending ?"TRUE" :"FALSE");

    cli_out (cli, "\n");

    cli_out (cli, "%% PORT Based Information & Statistics \n");
    cli_out (cli, "%% ----------------------------------- \n");

    cli_out (cli, "%% Config Bpdu's xmitted              : %u\n",
             port->conf_bpdu_sent);

    cli_out (cli, "%% Config Bpdu's received             : %u\n",
             port->conf_bpdu_rcvd);

    cli_out (cli, "%% TCN Bpdu's xmitted                 : %u\n",
             port->tcn_bpdu_sent);

    cli_out (cli, "%% TCN Bpdu's received                : %u\n",
             port->tcn_bpdu_rcvd);

    cli_out (cli, "%% Forward Trans Count                : %u\n",
             port->cist_forward_transitions);

    cli_out (cli, "\n");

    cli_out (cli, "%% STATUS of Port Timers \n");
    cli_out (cli, "%% --------------------- \n");

    cli_out (cli, "%% Hello Time Configured              : %d\n",
             port->hello_time/L2_TIMER_SCALE_FACT);

    cli_out (cli, "%% Hello timer                        : %s\n",
             (l2_is_timer_running (port->hello_timer)) ? 
             "ACTIVE":"INACTIVE");

    cli_out (cli, "%% Hello Time Value                   : %d\n",
             l2_timer_get_remaining_secs (port->hello_timer));

    cli_out (cli, "%% Forward Delay Timer                : %s\n",
             (l2_is_timer_running (port->forward_delay_timer)) ? 
             "ACTIVE":"INACTIVE");

    cli_out (cli, "%% Forward Delay Timer Value          : %d\n",
             l2_timer_get_remaining_secs (port->forward_delay_timer));

    cli_out (cli, "%% Message Age Timer                  : %s\n",
             (l2_is_timer_running (port->message_age_timer))? 
             "ACTIVE":"INACTIVE");

    cli_out (cli, "%% Message Age Timer Value            : %d\n",
             l2_timer_get_remaining_secs (port->message_age_timer));

    cli_out (cli, "%% Topology Change Timer              : %s\n",
             (l2_is_timer_running (port->tc_timer))? 
             "ACTIVE":"INACTIVE");

    cli_out (cli, "%% Topology Change Timer Value        : %d\n",
             l2_timer_get_remaining_secs (port->tc_timer));

    cli_out (cli, "%% Hold Timer                         : %s\n",
             (l2_is_timer_running (port->hold_timer))? "ACTIVE" : "INACTIVE");

    cli_out (cli, "%% Hold Timer Value                   : %d\n",
             l2_timer_get_remaining_secs (port->hold_timer));

    cli_out (cli, "\n");

    cli_out (cli, "%% Other Port-Specific Info \n");
    cli_out (cli, "  ------------------------ \n");
   
    cli_out (cli, "%% Max Age Transitions                : %u\n",
             br->max_age_count);

    cli_out (cli, "%% Msg Age Expiry                     : %u\n",
             port->msg_age_timer_cnt);

    cli_out (cli, "%% Similar BPDUS Rcvd                 : %u\n",
             port->similar_bpdu_cnt);

    cli_out (cli, "%% Src Mac Count                      : %d\n",
             port->src_mac_count);

    cli_out (cli, "%% Total Src Mac Rcvd                 : %d\n",
             port->total_src_mac_count);

    port->cist_next_state = mstp_map_port2msg_next_state(port->cist_state);
    
    cli_out (cli, "%% Next State                         : %s\n",
             stpPortStateStr[port->cist_next_state]);

    if (l2_is_timer_running (port->tc_timer))
      cli_out (cli, "%% Topology Change Time               : %u\n",
               (mstp_cist_get_topology_change_time(port))/L2_TIMER_SCALE_FACT); 
    else
      cli_out (cli, "%% Topology Change Time               : 0\n");

    cli_out (cli, "\n");
    cli_out (cli, "\n");

  }

   idPtr = (u_char *)&(br->cist_bridge_id.addr);

   cli_out (cli,"%% Other Bridge information & Statistics\n");
   cli_out (cli,"  --------------------------------------\n");

   cli_out (cli, "%% STP Multicast Address              :"
            " %.2x:%.2x:%.2x:%.2x:%.2x:%.2x \n",
            bridge_grp_add [0], bridge_grp_add [1],
            bridge_grp_add [2], bridge_grp_add [3],
            bridge_grp_add [4], bridge_grp_add [5]);

   cli_out (cli,"%% Bridge Priority                    : %u \n",
            br->cist_bridge_priority);

   cli_out (cli, "%% Bridge Mac Address                 :"
            " %.2x:%.2x:%.2x:%.2x:%.2x:%.2x\n",
            idPtr[0], idPtr[1], idPtr[2],
            idPtr[3], idPtr[4], idPtr[5]);

   cli_out (cli, "%% Bridge Hello Time                  : %d \n",
            br->bridge_hello_time / L2_TIMER_SCALE_FACT);

   cli_out (cli, "%% Bridge Forward Delay               : %d \n",
            br->bridge_forward_delay / L2_TIMER_SCALE_FACT);

   cli_out (cli, "%% Topology Change Initiator          : %d\n",
            br->tc_initiator);

   pal_time_calendar (&br->time_last_topo_change, buf);
   cli_out (cli, "%% Last Topology Change Occured       : %s", buf);

   cli_out (cli, "%% Topology Change                    : %s\n",
            br->tc_flag ? "TRUE" : "FALSE");

   cli_out (cli, "%% Topology Change Detected           : %s \n",
            br->topology_change_detected ? "TRUE" : "FALSE");

   cli_out (cli, "%% Topology Change Count              : %u\n",
            br->total_num_topo_changes);

   cli_out (cli, "%% Topology Change Last Recvd from    :"
            " %.2x:%.2x:%.2x:%.2x:%.2x:%.2x \n",
            br->tc_last_rcvd_from[0], br->tc_last_rcvd_from[1],
            br->tc_last_rcvd_from[2], br->tc_last_rcvd_from[3],
            br->tc_last_rcvd_from[4], br->tc_last_rcvd_from[5]);

   cli_out (cli, "\n");

  return CLI_SUCCESS;
}

/* Display statistics for multiple instances */
static  int
mstp_stats_display_inst_info (struct mstp_bridge *br,
                              struct interface *ifp,
                              struct cli *cli,
                              int instance)
{
  struct mstp_bridge_instance *br_inst  = NULL;
  u_int32_t instance_index = 0;

  for (instance_index = MST_MIN_INSTANCES; instance_index < br->max_instances; 
       instance_index++)
     {
       br_inst = br->instance_list[instance_index];
       if (!br_inst)
         continue;

       if (br_inst->port_list == NULL)
         continue;

       mstp_stats_display_inst_info_det (br, br_inst, ifp, cli, instance_index);
     }

  return CLI_SUCCESS;
}

/* Internal function to display statistics for an instance */
static  int
mstp_stats_display_inst_info_det (struct mstp_bridge *br, 
                                  struct mstp_bridge_instance *br_inst,
                                  struct interface *ifp, struct cli *cli,
                                  u_int32_t instance_index)
{
  struct mstp_port *port  = NULL;
  struct mstp_instance_port *inst_port  = NULL;
  u_char *idPtr = NULL;
  char buf[MSTP_TIME_LENGTH];
  bool_t display = PAL_FALSE; 

  if (!br || !br_inst)
    return CLI_ERROR;

  for (inst_port = br_inst->port_list; inst_port;
       inst_port = inst_port->next)
    {
      if (ifp != NULL && (ifp->ifindex != inst_port->ifindex))
        continue;

      if (!display)
        { 
          if (IS_BRIDGE_MSTP (br))
            {
              if (br_inst->port_list)
                {
                  cli_out (cli, "\n");
                  cli_out (cli, "\tSpanning Tree Enabled for Instance : "
                           "%u \n", instance_index);
                  cli_out (cli, "\t============================"
                           "====== \n");
                }
            }
          else
            {
              if (br_inst->port_list && br_inst->vlan_list)
                {
                  cli_out (cli, "\n");
                  cli_out (cli, "\tSpanning Tree Enabled for Vlan  : "
                           "%u \n", br_inst->vlan_list->lo);
                  cli_out (cli, "\t============================== \n");
                }
            } 

        }

      for (port = br->port_list; port; port = port->next)
        {
          if (port->ifindex != inst_port->ifindex)
            continue;

          if (IS_BRIDGE_MSTP (br))
            {
              cli_out (cli, "\n");
              cli_out (cli, "%% INST_PORT %s Information & " 
                       "Statistics \n", port->name);
              cli_out (cli, "%% ---------------------------------------- \n");

              cli_out (cli, "%% Config Bpdu's xmitted (port/inst)    :" 
                       " (%u/%u)\n",
                       port->conf_bpdu_sent, inst_port->conf_bpdu_sent);

              cli_out (cli,"%% Config Bpdu's received (port/inst)   :"
                       " (%u/%u)\n",
                       port->conf_bpdu_rcvd, inst_port->conf_bpdu_rcvd);

              cli_out (cli,"%% TCN Bpdu's xmitted (port/inst)       :" 
                       " (%u/%u)\n",
                       port->tcn_bpdu_sent, inst_port->tcn_bpdu_sent);

              cli_out (cli,"%% TCN Bpdu's received (port/inst)      :"
                       " (%u/%u)\n",
                       port->tcn_bpdu_rcvd, inst_port->tcn_bpdu_rcvd);

              cli_out (cli, "%% Message Age(port/Inst)               : "
                       "(%d/%d)\n", 
                       port->cist_message_age/L2_TIMER_SCALE_FACT,
                       inst_port->message_age/L2_TIMER_SCALE_FACT); 
            }
          else
            {
              cli_out (cli, "\n");
              cli_out (cli, "%% VLAN_PORT %s Information &" 
                       " Statistics \n", port->name);
              cli_out (cli, "%% --------------------------------------"
                       "--\n");

              cli_out (cli, "%% Config Bpdu's xmitted (port/vlan)    :" 
                       " (%u/%u)\n",
                       port->conf_bpdu_sent, inst_port->conf_bpdu_sent);

              cli_out (cli,"%% Config Bpdu's received (port/vlan)    :"
                       " (%u/%u)\n",
                       port->conf_bpdu_rcvd, inst_port->conf_bpdu_rcvd);

              cli_out (cli,"%% TCN Bpdu's xmitted (port/vlan)        :" 
                       " (%u/%u)\n",
                       port->tcn_bpdu_sent, inst_port->tcn_bpdu_sent);

              cli_out (cli,"%% TCN Bpdu's received (port/vlan)       :"
                       " (%u/%u)\n",
                       port->tcn_bpdu_rcvd, inst_port->tcn_bpdu_rcvd);

              cli_out (cli, "%% Message Age(port/vlan)               : "
                       "(%d/%d)\n",
                       port->cist_message_age/L2_TIMER_SCALE_FACT,
                       inst_port->message_age/L2_TIMER_SCALE_FACT); 
            }

          cli_out (cli, "%% %s: Forward Transitions                  : %u\n",
                   port->name, port->cist_forward_transitions);

          cli_out (cli, "%% Next State                           : %s\n",
                   stpPortStateStr[inst_port->next_state]);

          if (l2_is_timer_running (inst_port->tc_timer))
            cli_out (cli, "%% Topology Change Time                 : %d\n",
                     (mstp_msti_get_topology_change_time
                     ((inst_port))/L2_TIMER_SCALE_FACT));
          else
            cli_out (cli, "%% Topology Change Time                 : 0\n");


            }

      display = PAL_TRUE;

    }

  display = PAL_FALSE;

  for (inst_port = br_inst->port_list; inst_port;
      inst_port = inst_port->next)
    {
      if (ifp != NULL && (ifp->ifindex != inst_port->ifindex))
        continue;

      if (!display)
        {

          cli_out (cli, "\n");
          cli_out (cli, "%% Other Inst/Vlan Information & Statistics \n");
          cli_out (cli, "%% ---------------------------------------- \n");

          cli_out (cli,"%% Bridge Priority                     : %u \n",
                   br_inst->msti_bridge_priority);

          idPtr = (u_char *)&(br_inst->msti_bridge_id.addr);
          cli_out (cli, "%% Bridge Mac Address                  :"
                   " %.2x:%.2x:%.2x:%.2x:%.2x:%.2x\n",
                   idPtr[0], idPtr[1], idPtr[2],
                   idPtr[3], idPtr[4], idPtr[5]);

          cli_out (cli, "%% Topology Change Initiator           : %d\n",
                   br_inst->tc_initiator);

          pal_time_calendar (&br_inst->time_last_topo_change, buf);
          cli_out (cli, "%% Last Topology Change Occured        : %s", buf);

          cli_out (cli, "%% Topology Change                     : %s\n",
                   br_inst->tc_flag ? "TRUE" : "FALSE");

          cli_out (cli, "%% Topology Change Detected            : %s \n",
                   br_inst->topology_change_detected ? "TRUE" : "FALSE");

          cli_out (cli, "%% Topology Change Count               : %u\n",
                   br_inst->total_num_topo_changes);

          cli_out (cli, "%% Topology Change Last Recvd from     : "
                   "%.2x:%.2x:%.2x:%.2x:%.2x:%.2x \n",
                   br_inst->tc_last_rcvd_from[0], 
                   br_inst->tc_last_rcvd_from[1],
                   br_inst->tc_last_rcvd_from[2], 
                   br_inst->tc_last_rcvd_from[3],
                   br_inst->tc_last_rcvd_from[4], 
                   br_inst->tc_last_rcvd_from[5]);

          cli_out (cli,"\n");

        }

      display = PAL_TRUE;

    }        
       
  return CLI_SUCCESS;
}

/* Display statistics for a single instance or vlan */
static  int
mstp_stats_display_single_inst_info (struct mstp_bridge *br,
                                     struct interface *ifp,
                                     struct cli *cli,
                                     u_int32_t instance)
{
  struct mstp_bridge_instance *br_inst  = NULL;

  br_inst = br->instance_list[instance];
  if (br_inst == NULL)
    {
      cli_out (cli, "%% No Instances are configured\n ");
      return CLI_ERROR;
    }

  if (br_inst->port_list == NULL)
    {
      cli_out (cli, "%% No Interfaces are configured\n ");
      return CLI_ERROR;
    }

  mstp_stats_display_inst_info_det (br, br_inst, ifp, cli, instance);

  return CLI_SUCCESS;

}

static void
mstp_stats_clear_info (struct mstp_bridge *br,
                       struct interface *ifp,
                       struct cli *cli)
{ 
  struct mstp_port *port    = NULL;

  if (br == NULL)
    return ;

  for (port = br->port_list; port;port = port->next)
     {
       if (ifp && ifp->ifindex != port->ifindex)
         continue; 

       port->conf_bpdu_sent = 0 ;
       port->conf_bpdu_rcvd = 0;
       port->tcn_bpdu_sent = 0;
       port->tcn_bpdu_rcvd = 0;
       br->max_age_count = 0;
       port->similar_bpdu_cnt = 0;
       port->src_mac_count = 0;
       port->total_src_mac_count = 0;
       br->total_num_topo_changes = 0;

       cli_out (cli,"%% cleared statistics for port %u bridge %s\n",
	        port->ifindex,br->name);
     }
}


static int 
mstp_stats_clear_mstp (struct mstp_bridge *br,
                       struct interface *ifp,
                       struct cli *cli,
                       u_int32_t instance)
{
  u_int32_t instance_index = 0;
  struct mstp_instance_port *inst_port = NULL;
  struct mstp_bridge_instance * br_inst = NULL; 
  
  if (br == NULL)
    return CLI_ERROR;
  
  for (instance_index = MST_MIN_INSTANCES; instance_index < br->max_instances; 
       instance_index++)
     {

       br_inst = br->instance_list[instance_index];
       if (!br_inst)
         continue;

       if (br_inst->port_list == NULL)
         {
           cli_out (cli, "%% No Instances are configured\n ");
           continue;
         }

       for (inst_port = br_inst->port_list; inst_port;
            inst_port = inst_port->next)
          {

            if (ifp && inst_port->ifindex != ifp->ifindex)
              continue;

            inst_port->tcn_bpdu_rcvd   = 0;
            inst_port->tcn_bpdu_sent   = 0;
            inst_port->conf_bpdu_rcvd  = 0;
            inst_port->conf_bpdu_sent  = 0;

            cli_out (cli,"%% cleared statistics for instance %u bridge %s\n",
                     instance_index, br->name);
          }

     }

  return CLI_SUCCESS;
}

/* Clear statistics of a instance */
static int
mstp_stats_clear_single_inst_info (struct mstp_bridge *br,
                                   struct interface *ifp,
                                   struct cli *cli,
                                   u_int32_t instance, int vid)
{
  struct mstp_instance_port *inst_port = NULL;
  struct mstp_bridge_instance * br_inst = NULL;

  if (! br)
    {
      cli_out (cli, "%% Bridge not found\n");
      return CLI_ERROR;
    }

  br_inst = br->instance_list[instance];
  if (! br_inst)
    {
      cli_out (cli, "%% No Instances are configured\n");
      return CLI_ERROR;
    }

  if (br_inst->port_list == NULL)
    {
      cli_out (cli, "%%  No ports are associated with the instance\n");
      return CLI_ERROR;
    }

  for (inst_port = br_inst->port_list; inst_port;
       inst_port = inst_port->next)
     {
       if (ifp && inst_port->ifindex != ifp->ifindex)
         continue;

       inst_port->tcn_bpdu_rcvd   = 0;
       inst_port->tcn_bpdu_sent   = 0;
       inst_port->conf_bpdu_rcvd  = 0;
       inst_port->conf_bpdu_sent  = 0;

     }

  return CLI_SUCCESS;

}

static void
mstp_display_stp_info (struct mstp_bridge * br, struct interface *ifp,
                        struct cli *cli, int detail_flag)
{
  u_char * rootPtr;
  u_char * idPtr;
  char buf[MSTP_TIME_LENGTH];

  cli_out (cli, "%% %s: Bridge %s - Spanning Tree %s %s %s\n",
           br->is_default ? "Default" : br->name,
           br->bridge_enabled ? "up" : "down",
           boolean_str[br->mstp_enabled] ,
           br->topology_change ? "- topology change" : "",
           br->topology_change_detected ? "- topology change detected" : "");

  cli_out (cli, "%% %s:%s Root Path Cost %u -%s Priority %u\n",
           br->is_default ? "Default" : br->name,
           CIST (br),
           br->external_root_path_cost,
           CIST (br),
           br->cist_bridge_priority);

  cli_out (cli, "%% %s:%s Forward Delay %d -%s Hello Time %d -%s Max Age %d"
           "-%s Root port %d\n",
           br->is_default ? "Default" : br->name,
           CIST (br),
           br->bridge_forward_delay / L2_TIMER_SCALE_FACT,
           CIST (br),
           br->bridge_hello_time / L2_TIMER_SCALE_FACT,
           CIST (br),
           br->bridge_max_age / L2_TIMER_SCALE_FACT,
           CIST (br),
           br->cist_root_port_ifindex);



  rootPtr = (u_char *)&(br->cist_designated_root);
  idPtr = (u_char *)&(br->cist_bridge_id);

  mstp_display_bridge_id (cli, br->is_default ? "Default" : br->name,
                          "Root Id", rootPtr, PAL_FALSE);
  mstp_display_bridge_id (cli, br->is_default ? "Default" : br->name,
                          "Bridge Id", idPtr, PAL_FALSE);

  pal_time_calendar (&br->time_last_topo_change, buf);
  cli_out (cli, "%% %s: %d topology changes - last topology change %s",
           br->is_default ? "Default" : br->name,
           br->total_num_topo_changes, buf);


  cli_out (cli, "%% %s: portfast bpdu-filter %s\n",
           br->is_default ? "Default" : br->name,
           br->bpdu_filter ? "enabled" : "disabled");
  cli_out (cli, "%% %s: portfast bpdu-guard %s\n",
           br->is_default ? "Default" : br->name,
           br->bpduguard ? "enabled" : "disabled");
  cli_out (cli, "%% %s: portfast errdisable timeout %s\n",
           br->is_default ? "Default" : br->name,
           br->errdisable_timeout_enable ?  "enabled" : "disabled");
  cli_out (cli, "%% %s: portfast errdisable timeout interval %d sec\n",
           br->is_default ? "Default" : br->name,
          br->errdisable_timeout_interval / L2_TIMER_SCALE_FACT);

  if (detail_flag)
    {
      struct mstp_port *port;
      if (! ifp)
        {
          for (port = br->port_list; port;port = port->next)
           {
             mstp_display_stp_port (port, cli);
             cli_out (cli, "%% \n");
           }
        }
      else
        {
          for (port = br->port_list; port;port = port->next)
           {
             if (port->ifindex ==  ifp->ifindex)
               {
                  mstp_display_stp_port (port, cli);
                  cli_out (cli, "%% \n");
                  break;
               }
           }
        }
    }
}

static s_int32_t
mstp_cist_get_topology_change_time (struct mstp_port *port)
{

  if (IS_BRIDGE_MSTP (port->br) || IS_BRIDGE_RPVST_PLUS (port->br))
    return (port->send_mstp ? (port->cist_hello_time + L2_TIMER_SCALE_FACT):
                             (port->br->cist_max_age +
                              port->br->cist_forward_delay));
  else
    return (port->send_mstp ? (port->br->cist_hello_time + L2_TIMER_SCALE_FACT):
                             (port->br->cist_max_age +
                              port->br->cist_forward_delay));
}

static s_int32_t
mstp_msti_get_topology_change_time (struct mstp_instance_port *inst_port)
{

  struct mstp_port *port = inst_port->cst_port;;

  if (IS_BRIDGE_MSTP (port->br) || IS_BRIDGE_RPVST_PLUS (port->br))
    return (port->send_mstp ? (port->cist_hello_time + L2_TIMER_SCALE_FACT):
                              (port->br->cist_max_age +
                               port->br->cist_forward_delay));
  else
    return (port->send_mstp ? (port->br->cist_hello_time + L2_TIMER_SCALE_FACT):
                              (port->br->cist_max_age +
                               port->br->cist_forward_delay));
}


#ifdef HAVE_WMI
/* WMI Debug commands. */

#define DEBUG_MSTP_STR    "debug mstp wmi"
#define NO_DEBUG_MSTP_STR "no debug mstp wmi"
#define WMI_STR           "Web Management Interface (WMI)"

CLI (debug_mstp_wmi,
     debug_mstp_wmi_cmd,
     DEBUG_MSTP_STR,
     CLI_DEBUG_STR,
     CLI_MSTP_STR,
     WMI_STR)
{
  if (LAYER2_DEBUG(proto, CLI_ECHO))
    zlog_debug (cli->zg, "%% %s", DEBUG_MSTP_STR);
  WMI_XSTP_DEBUG_ON (EVENT);
  WMI_XSTP_DEBUG_ON (RECV);
  WMI_XSTP_DEBUG_ON (SEND);
  return CLI_SUCCESS;
}

CLI (debug_mstp_wmi_event,
     debug_mstp_wmi_event_cmd,
     DEBUG_MSTP_STR" event",
     CLI_DEBUG_STR,
     CLI_MSTP_STR,
     WMI_STR,
     "Event outputs")
{
  if (LAYER2_DEBUG(proto, CLI_ECHO))
    zlog_debug (cli->zg, "%% %s event", DEBUG_MSTP_STR);
  WMI_XSTP_DEBUG_ON (EVENT);
  return CLI_SUCCESS;
}

CLI (debug_mstp_wmi_recv,
     debug_mstp_wmi_recv_cmd,
     DEBUG_MSTP_STR" receive",
     CLI_DEBUG_STR,
     CLI_MSTP_STR,
     WMI_STR,
     "Received outputs")
{
  if (LAYER2_DEBUG(proto, CLI_ECHO))
    zlog_debug (cli->zg, "%% %s receive", DEBUG_MSTP_STR);
  WMI_XSTP_DEBUG_ON (RECV);
  return CLI_SUCCESS;
}

CLI (debug_mstp_wmi_send,
     debug_mstp_wmi_send_cmd,
     DEBUG_MSTP_STR" send",
     CLI_DEBUG_STR,
     CLI_MSTP_STR,
     WMI_STR,
     "Sent outputs")
{
  if (LAYER2_DEBUG(proto, CLI_ECHO))
    zlog_debug (cli->zg, "%% %s send", DEBUG_MSTP_STR);
  WMI_XSTP_DEBUG_ON (SEND);
  return CLI_SUCCESS;
}

CLI (debug_mstp_wmi_msg,
     debug_mstp_wmi_msg_cmd,
     DEBUG_MSTP_STR" message",
     CLI_DEBUG_STR,
     CLI_MSTP_STR,
     WMI_STR,
     "Received & sent message outputs")
{
  if (LAYER2_DEBUG(proto, CLI_ECHO))
    zlog_debug (cli->zg, "%% %s message", DEBUG_MSTP_STR);
  WMI_XSTP_DEBUG_ON (RECV);
  WMI_XSTP_DEBUG_ON (SEND);
  return CLI_SUCCESS;
}

CLI (debug_mstp_wmi_detail,
     debug_mstp_wmi_detail_cmd,
     DEBUG_MSTP_STR" detail",
     CLI_DEBUG_STR,
     CLI_MSTP_STR,
     WMI_STR,
     "Detailed outputs for debugging")
{
  if (LAYER2_DEBUG(proto, CLI_ECHO))
    zlog_debug (cli->zg, "%% %s detail", DEBUG_MSTP_STR);
  WMI_XSTP_DEBUG_ON (DETAIL);
  return CLI_SUCCESS;
}

CLI (no_debug_mstp_wmi,
     no_debug_mstp_wmi_cmd,
     NO_DEBUG_MSTP_STR,
     CLI_NO_STR,
     CLI_DEBUG_STR,
     CLI_MSTP_STR,
     WMI_STR)
{
  if (LAYER2_DEBUG(proto, CLI_ECHO))
    zlog_debug (cli->zg, "%% %s", NO_DEBUG_MSTP_STR);
  WMI_XSTP_DEBUG_OFF (EVENT);
  WMI_XSTP_DEBUG_OFF (RECV);
  WMI_XSTP_DEBUG_OFF (SEND);
  WMI_XSTP_DEBUG_OFF (DETAIL);
  return CLI_SUCCESS;
}

CLI (no_debug_mstp_wmi_event,
     no_debug_mstp_wmi_event_cmd,
     NO_DEBUG_MSTP_STR" event",
     CLI_NO_STR,
     CLI_DEBUG_STR,
     CLI_MSTP_STR,
     WMI_STR,
     "Event outputs")
{
  if (LAYER2_DEBUG(proto, CLI_ECHO))
    zlog_debug (cli->zg, "%% %s event", NO_DEBUG_MSTP_STR);
  WMI_XSTP_DEBUG_OFF (EVENT);
  return CLI_SUCCESS;
}

CLI (no_debug_mstp_wmi_recv,
     no_debug_mstp_wmi_recv_cmd,
     NO_DEBUG_MSTP_STR" receive",
     CLI_NO_STR,
     CLI_DEBUG_STR,
     CLI_MSTP_STR,
     WMI_STR,
     "Received outputs")
{
  if (LAYER2_DEBUG(proto, CLI_ECHO))
    zlog_debug (cli->zg, "%% %s receive", NO_DEBUG_MSTP_STR);
  WMI_XSTP_DEBUG_OFF (RECV);
  return CLI_SUCCESS;
}

CLI (no_debug_mstp_wmi_send,
     no_debug_mstp_wmi_send_cmd,
     NO_DEBUG_MSTP_STR" send",
     CLI_NO_STR,
     CLI_DEBUG_STR,
     CLI_MSTP_STR,
     WMI_STR,
     "Sent outputs")
{
  if (LAYER2_DEBUG(proto, CLI_ECHO))
    zlog_debug (cli->zg, "%% %s send", NO_DEBUG_MSTP_STR);
  WMI_XSTP_DEBUG_OFF (SEND);
  return CLI_SUCCESS;
}

CLI (no_debug_mstp_wmi_msg,
     no_debug_mstp_wmi_msg_cmd,
     NO_DEBUG_MSTP_STR" message",
     CLI_NO_STR,
     CLI_DEBUG_STR,
     CLI_MSTP_STR,
     WMI_STR,
     "Received & sent message outputs")
{
  if (LAYER2_DEBUG(proto, CLI_ECHO))
    zlog_debug (cli->zg, "%% %s message", NO_DEBUG_MSTP_STR);
  WMI_XSTP_DEBUG_OFF (RECV);
  WMI_XSTP_DEBUG_OFF (SEND);
  return CLI_SUCCESS;
}

CLI (no_debug_mstp_wmi_detail,
     no_debug_mstp_wmi_detail_cmd,
     NO_DEBUG_MSTP_STR" detail",
     CLI_NO_STR,
     CLI_DEBUG_STR,
     CLI_MSTP_STR,
     WMI_STR,
     "Detailed outputs for debugging")
{
  if (LAYER2_DEBUG(proto, CLI_ECHO))
    zlog_debug (cli->zg, "%% %s detail", NO_DEBUG_MSTP_STR);
  WMI_XSTP_DEBUG_OFF (DETAIL);
  return CLI_SUCCESS;
}

/* Installs WMI debug commands.  */
void
mstp_wmi_debug_init (struct lib_globals *mstpm)
{
  struct cli_tree *ctree = mstpm->ctree;

  /* Install debug commands. */
  cli_install_gen (ctree, EXEC_MODE, PRIVILEGE_MAX, 0,
                   &debug_mstp_wmi_cmd);
  cli_install_gen (ctree, EXEC_MODE, PRIVILEGE_MAX, 0,
                   &debug_mstp_wmi_event_cmd);
  cli_install_gen (ctree, EXEC_MODE, PRIVILEGE_MAX, 0,
                   &debug_mstp_wmi_recv_cmd);
  cli_install_gen (ctree, EXEC_MODE, PRIVILEGE_MAX, 0,
                   &debug_mstp_wmi_send_cmd);
  cli_install_gen (ctree, EXEC_MODE, PRIVILEGE_MAX, 0,
                   &debug_mstp_wmi_msg_cmd);
  cli_install_gen (ctree, EXEC_MODE, PRIVILEGE_MAX, 0,
                   &debug_mstp_wmi_detail_cmd);
  cli_install_gen (ctree, EXEC_MODE, PRIVILEGE_MAX, 0,
                   &no_debug_mstp_wmi_cmd);
  cli_install_gen (ctree, EXEC_MODE, PRIVILEGE_MAX, 0,
                   &no_debug_mstp_wmi_event_cmd);
  cli_install_gen (ctree, EXEC_MODE, PRIVILEGE_MAX, 0,
                   &no_debug_mstp_wmi_recv_cmd);
  cli_install_gen (ctree, EXEC_MODE, PRIVILEGE_MAX, 0,
                   &no_debug_mstp_wmi_send_cmd);
  cli_install_gen (ctree, EXEC_MODE, PRIVILEGE_MAX, 0,
                   &no_debug_mstp_wmi_msg_cmd);
  cli_install_gen (ctree, EXEC_MODE, PRIVILEGE_MAX, 0,
                   &no_debug_mstp_wmi_detail_cmd);
}

#endif /* HAVE_WMI */
