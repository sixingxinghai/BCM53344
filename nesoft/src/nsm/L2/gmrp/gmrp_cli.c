/* Copyright (C) 2003  All Rights Reserved.

   GMRP: GARP Multicast Registration Protocol
  
   This module defines the CLI interface to the GMRP Protocol 

   The following commands are implemented:

   EXEC_PRIV_MODE:
     * show gmrp configuration bridge BRIDGE_NAME
     * show gmrp timer IF_NAME 
     * show gmrp statistics vlanid VLANID bridge BRIDGE_NAME
      
   CONFIG_MODE:
     * set gmrp {ENABLE | DISABLE} bridge BRIDGE_NAME
     * set port gmrp { ENABLE | DISABLE } IF_NAME
     * set gmrp registration  { NORMAL | FIXED | FORBIDDEN | RESTRICTEDGROUP} IF_NAME 
     * set gmrp timer { JOIN | LEAVE | LEAVEALL } TIMER_VALUE IF_NAME
     * set gmrp fwdall { ENABLE | DISABLE } IF_NAME
     * clear gmrp statistics all bridge BRIDGE_NAME
     * clear gmrp statistics vlanid VLANID bridge BRTIDGE_NAME

   EXEC_PRIV_MODE:
     * show gmrp configuration bridge BRIDGE_NAME
     * show gmrp timer IF_NAME 
     * show gmrp statistics vlanid VLANID bridge BRIDGE_NAME
*/

#include "pal.h"
#include "lib.h"
#include "log.h"
#include "cli.h"
#include "avl_tree.h"

#include "nsm_interface.h"
#include "nsm_bridge.h"
#include "nsm_vlan.h"
#include "nsm_bridge_cli.h"
#include "nsm_vlan_cli.h"

#include "gmrp.h"
#include "gmrp_api.h"
#include "gmrp_cli.h"
#include "gmrp_debug.h"
#include "garp_gid.h"
#include "garp_gid_fsm.h"
#include "garp_pdu.h"

CLI (gmrp_debug_event,
     gmrp_debug_event_cmd,
     "debug gmrp event",
     "debug - enable debug output",
     "gmrp - GMRP commands",
     "event - echo events to console")
{
  if (GMRP_DEBUG (CLI))
    {
      zlog_info (gmrpm, "debug gmrp event");
    }

  DEBUG_ON(EVENT);
  return CLI_SUCCESS;
}


CLI (gmrp_debug_cli,
     gmrp_debug_cli_cmd,
     "debug gmrp cli",
     "debug - enable debug output",
     "gmrp - GMRP commands",
     "cli - echo commands to console")
{
  if (GMRP_DEBUG (CLI))
    {
      zlog_info (gmrpm, "debug gmrp cli");
    }

  DEBUG_ON(CLI);
  return CLI_SUCCESS;
}


CLI (gmrp_debug_timer,
     gmrp_debug_timer_cmd,
     "debug gmrp timer",
     "gmrp - GMRP commands",
     "debug - enable debug output",
     "timer - echo timer start to console")
{
  if (GMRP_DEBUG (CLI))
    {
      cli_out (cli, "debug gmrp timer");
    }

  DEBUG_ON(TIMER);
  return CLI_SUCCESS;
}


CLI (gmrp_debug_packet,
     gmrp_debug_packet_cmd,
     "debug gmrp packet",
     "debug - enable debug output",
     "gmrp - GMRP commands",
     "packet - echo packet contents to console")
{
  if (GMRP_DEBUG (CLI))
    {
      zlog_info (gmrpm, "debug gmrp packet");
    }

  DEBUG_ON(PACKET);
  return CLI_SUCCESS;
}


CLI (no_gmrp_debug_event,
     no_gmrp_debug_event_cmd,
     "no debug gmrp event",
     CLI_NO_STR,
     "debug - enable debug output",
     "gmrp - GMRP commands",
     "event - echo events to console")
{
  if (GMRP_DEBUG (CLI))
    {
      zlog_info (gmrpm, "no debug gmrp event");
    }

  DEBUG_ON(EVENT);
  return CLI_SUCCESS;
}


CLI (no_gmrp_debug_cli,
     no_gmrp_debug_cli_cmd,
     "no debug gmrp cli",
     CLI_NO_STR,
     "debug - disable debug output",
     "gmrp - GMRP commands",
     "cli - do not echo commands to console")
{
  if (GMRP_DEBUG (CLI))
    {
      cli_out (cli, "no debug gmrp cli");
    }

  DEBUG_OFF(CLI);
  return CLI_SUCCESS;
}


CLI (no_gmrp_debug_timer,
     no_gmrp_debug_timer_cmd,
     "no debug gmrp timer",
     CLI_NO_STR,
     "gmrp - GMRP commands",
     "debug - disable debug output",
     "timer - do not echo timer start to console")
{
  if (GMRP_DEBUG (CLI))
    {
      zlog_info (gmrpm, "no debug gmrp timer");
    }

  DEBUG_OFF(TIMER);
  return CLI_SUCCESS;
}


CLI (no_gmrp_debug_packet,
     no_gmrp_debug_packet_cmd,
     "no debug gmrp packet",
     CLI_NO_STR,
     "debug - disable debug output",
     "gmrp - GMRP commands",
     "packet - do not echo packet contents to console")
{
  if (GMRP_DEBUG (CLI))
    {
      zlog_info (gmrpm, "no debug gmrp packet");
    }

  DEBUG_OFF(PACKET);
  return CLI_SUCCESS;
}


CLI (gmrp_debug_all,
     gmrp_debug_all_cmd,
     "debug gmrp all",
     "debug - enable debug output",
     "gmrp - GMRP commands",
     "all - turn on all debugging")
{
  if (GMRP_DEBUG (CLI))
    {
      zlog_info (gmrpm, "debug gmrp all");
    }

  DEBUG_ON(EVENT);
  DEBUG_ON(CLI);
  DEBUG_ON(TIMER);
  DEBUG_ON(PACKET);
  return CLI_SUCCESS;
}


CLI (no_gmrp_debug_all,
     no_gmrp_debug_all_cmd,
     "no debug gmrp all",
     CLI_NO_STR,
     "debug - enable debug output",
     "gmrp - GMRP commands",
     "all - turn off all debugging")
{
  if (GMRP_DEBUG (CLI))
    {
      zlog_info (gmrpm, "no debug gmrp all");
    }

  DEBUG_OFF(EVENT);
  DEBUG_OFF(CLI);
  DEBUG_OFF(TIMER);
  DEBUG_OFF(PACKET);
  return CLI_SUCCESS;
}

CLI (show_debugging_gmrp,
     show_debugging_gmrp_cmd,
     "show debugging gmrp",
     CLI_SHOW_STR,
     CLI_DEBUG_STR,
     CLI_GMRP_STR)
{
  cli_out (cli, "GMRP debugging status:\n");

  if (GMRP_DEBUG (EVENT))
    cli_out (cli, "  GMRP Event debugging is on\n");
  if (GMRP_DEBUG (CLI))
    cli_out (cli, "  GMRP CLI debugging is on\n");
  if (GMRP_DEBUG (TIMER))
    cli_out (cli, "  GMRP Timer debugging is on\n");
  if (GMRP_DEBUG (PACKET))
    cli_out (cli, "  GMRP Packet debugging is on\n");
  cli_out (cli, "\n");

  return CLI_SUCCESS;
}

static void
gmrp_cli_process_result (struct cli *cli, int result,
                         struct interface  *ifp,
                         char *bridge_name,
                         char *proto)
{
  switch (result)
  {
    case NSM_BRIDGE_ERR_NOT_BOUND:
      ifp ? (cli_out (cli, "%% Interface %d not bound to bridge\n",
            ifp->ifindex)):
        (cli_out (cli, "%% Interface not bound to bridge\n"));
      break;
    case NSM_BRIDGE_ERR_MEM:
      bridge_name ? (cli_out (cli, "%% Bridge %s Out of Memory\n",
            bridge_name)):
        (cli_out (cli, "%% Bridge Out of memory\n"));
      break;
    case NSM_BRIDGE_ERR_NOTFOUND:
      bridge_name ? cli_out (cli, "%% Bridge %s Not Found\n",
          bridge_name):
        cli_out (cli, "%% Bridge Not Found\n");
      break;
    case NSM_VLAN_ERR_BRIDGE_NOT_VLAN_AWARE:
      bridge_name ? (cli_out (cli, "%% Bridge %s is not vlan aware\n",
            bridge_name)) :
        (cli_out (cli, "%% Bridge is not vlan aware\n"));
      break;
    case NSM_BRIDGE_ERR_GMRP_NOCONFIG:
      bridge_name ? (cli_out (cli,
            "%% No %s configured on bridge %s\n", proto, bridge_name)):
        (cli_out (cli, "%% No %s configured on bridge\n", proto));
      break;
    case NSM_ERR_GMRP_NOCONFIG_ONPORT:
      ifp ? (cli_out (cli, "%% No %s configured on the port %s\n",
             proto, ifp->name)):
          (cli_out (cli, "%% No %s configured on the port", proto));
      break; 
    case NSM_GMRP_ERR_VLAN_NOT_FOUND:
     bridge_name ? (cli_out (cli, "%% Vlan not configureed for Bridge %s\n",
            bridge_name)) :
        (cli_out (cli, "%% Vlan not configureed for Bridge \n"));
      break;
    case NSM_GMRP_ERR_GMRP_GLOBAL_CFG_BR:
     bridge_name ? (cli_out (cli, "%% %s Globally configured for Bridge %s\n",
             proto, bridge_name)):
          (cli_out (cli, "%% %s Globally configured for Bridge \n", proto));
     break;
    case NSM_GMRP_ERR_GMRP_NOT_CFG_ON_VLAN:
     bridge_name ? (cli_out (cli,
            "%% No %s configured on bridge %s for the vlan \n",
             proto, bridge_name)):
        (cli_out (cli, "%% No %s configured on bridge\n for the vlan ", proto));
     break;
    case NSM_GMRP_ERR_VLAN_NOT_CFG_ON_PORT:
     ifp ? (cli_out (cli, "%% VLAN not configured on Port %s \n",
             ifp->name)):
          (cli_out (cli, "%% VLAN not configured on Port \n"));
     break;
    case NSM_GMRP_ERR_GMRP_GLOBAL_CFG_PORT:
     ifp ? (cli_out (cli, "%% %s already configured on Port %s Globally \n",
            proto, ifp->name)):
          (cli_out (cli, "%% %s already configured on Port Globally \n", proto));
     break;
    case NSM_GMRP_ERR_GMRP_NOT_CFG_ON_PORT_VLAN:
     ifp ? (cli_out (cli, "%% %s not configured on Port %s for the vlan \n",
            proto, ifp->name)):
          (cli_out (cli, "%% %s not configured on Port for the vlan\n", proto));
     break;
    case NSM_BRIDGE_NOT_CFG:
      cli_out (cli, "%% No bridge configured\n");
      break;
    case NSM_BRIDGE_NO_PORT_CFG:
      cli_out (cli, "%% No ports added to Bridge\n");
      break;
    case NSM_BRIDGE_ERR_GENERAL:
      cli_out (cli, "%% Bridge error\n");
      break;
    default:
      break;
  }
}

/* 
 * GARP Timer object 
 */

/* Read GARP Timers */
CLI (show_gmrp_timer,
     show_gmrp_timer_cmd,
     "show (gmrp|mmrp) timer IF_NAME",
     "CLI_SHOW_STR",
     "GARP Multicast Registration Protocol",
     "MRP Multicast Registration Protocol",
     "GARP timer",
     "Ifname")
{

  /* 802.1D 14.9.1.1 */
  struct nsm_if *zif;
  register struct interface *ifp;
  struct nsm_master *nm = cli->vr->proto;
  pal_time_t timer_details[GARP_MAX_TIMERS];
  struct nsm_bridge_master *master = nsm_bridge_get_master (nm);

  if (GMRP_DEBUG (CLI))
    {
      zlog_info (gmrpm, "show %s timer %s", argv[0], argv[1]);
    }

  if ( (ifp = if_lookup_by_name(&cli->vr->ifm, argv[1])) )
    {

      zif = (struct nsm_if *)ifp->info;

      if (zif == NULL)
        {
          cli_out (cli, "interface not found \n");
          return CLI_ERROR;
        }

      if ((zif->bridge == NULL) || (zif->bridge->gmrp_bridge == NULL))
        {
          cli_out (cli, "%%Interface not bound to the bridge\n");
          return CLI_ERROR;
        }

      CHECK_XMRP_IS_RUNNING(zif->bridge,argv[0]);    

      if (gmrp_get_timer_details (master, ifp, timer_details) != RESULT_OK)
        {
          cli_out (cli, "%% Unable to display %s timer info for %s \n", 
                   argv[0], argv[1]);
          return CLI_ERROR;
        }
      cli_out (cli, "Timer           Timer Value (centiseconds)\n");
      cli_out (cli, "------------------------------------------\n");
      cli_out (cli, "Join                 %u\n", 
               timer_details[GARP_JOIN_TIMER]);
      cli_out (cli, "Leave                %u\n", 
               timer_details[GARP_LEAVE_CONF_TIMER]);
      cli_out (cli, "Leave All            %u\n", 
               timer_details[GARP_LEAVEALL_CONF_TIMER]); 
  }

  return CLI_SUCCESS;
}

/* Set GARP Timers (Join) */
CLI (set_gmrp_timer1,
     set_gmrp_timer1_cmd,
     "set (gmrp|mmrp) timer join TIMER_VALUE IF_NAME",
     "Set GMRP join timer for the specified interface",
     "GARP Multicast Registration Protocol",
     "MRP Multicast Registration Protocol",
     "GARP timer",
     "Join Timer",
     "Timervalue in centiseconds",
     "Ifname")
{ /* 802.1D 14.9.1.2 */
  register struct interface *ifp;
  pal_time_t timer_value;
  struct nsm_master *nm = cli->vr->proto;
  struct nsm_bridge_master *master = nsm_bridge_get_master (nm);
  struct nsm_if *zif = NULL;

  if (GMRP_DEBUG (CLI))
    {
      zlog_info (gmrpm, "set %s timer join %u %s", argv [0], argv[1], argv[2]);
    }

  CLI_GET_INTEGER_RANGE("garp_join_timer", timer_value, argv[1], 1, UINT32_MAX); 

  if ( !(ifp = if_lookup_by_name (&cli->vr->ifm, argv[2])) )
    {
      cli_out (cli, "%% Interface not found\n");
      return CLI_ERROR;
    }

  zif = (struct nsm_if *)ifp->info;
  if (!zif)
    {
      cli_out (cli, "%% Interface not found\n");
      return CLI_ERROR;
    }

  /* The interface is not bound to bridge*/
  if ((zif->bridge == NULL) || (zif->bridge->gmrp_bridge == NULL))
    {
      GMRP_ALLOC_PORT_CONFIG (zif->gmrp_port_cfg,
                              zif->gmrp_port_cfg->join_timeout,
                              timer_value, argv[2]);
    }

  CHECK_XMRP_IS_RUNNING(zif->bridge, argv[0]);

  if (gmrp_check_timer_rules(master, ifp, GARP_JOIN_TIMER,
            timer_value) != RESULT_OK)
    {
      cli_out(cli, "%% Timer does not meet timer relationship rules\n");
      return CLI_ERROR;
    }

  if (gmrp_set_timer (master, ifp, GARP_JOIN_TIMER, 
                            timer_value) != RESULT_OK)
    {
      cli_out (cli, "%% Unable to set %s join timer info for %s \n", 
               argv[0], argv[1]);
      return CLI_ERROR;
    }
  
  return CLI_SUCCESS;
}


/* Set GARP Timers (Leave) */
CLI (set_gmrp_timer2,
     set_gmrp_timer2_cmd,
     "set (gmrp|mmrp) timer leave TIMER_VALUE IF_NAME",
     "Set GMRP leave timer for the specified interface",
     "GARP Multicast Registration Protocol",
     "MRP Multicast Registration Protocol",
     "GARP timer",
     "Leave Timer",
     "Timervalue in centiseconds",
     "Ifname")
{ /* 802.1D 14.9.1.2 */
  register struct interface *ifp;
  pal_time_t timer_value;
  struct nsm_master *nm = cli->vr->proto;
  struct nsm_bridge_master *master = nsm_bridge_get_master (nm);
  struct nsm_if *zif = NULL;

  if (GMRP_DEBUG (CLI))
    {
      zlog_info (gmrpm, "set %s timer leave %u %s", argv [0], argv[1], argv[2]);
    }

  CLI_GET_INTEGER_RANGE("garp_leave_timer", timer_value, argv[1], 1, UINT32_MAX); 

  if ( !(ifp = if_lookup_by_name (&cli->vr->ifm, argv[2])) )
    {
      cli_out (cli, "%% Interface not found\n");
      return CLI_ERROR;
    }

  zif = (struct nsm_if *)ifp->info;
  if (!zif)
    {
      cli_out (cli, "%% Interface not found\n");
      return CLI_ERROR;
    }

  /* The interface is not bound to bridge*/
  if ((zif->bridge == NULL) || (zif->bridge->gmrp_bridge == NULL))
    {
      GMRP_ALLOC_PORT_CONFIG (zif->gmrp_port_cfg,
                              zif->gmrp_port_cfg->leave_timeout,
                              timer_value, argv[2]);
    }

  CHECK_XMRP_IS_RUNNING(zif->bridge, argv[0]);

  if (gmrp_check_timer_rules(master, ifp, GARP_LEAVE_TIMER,
            timer_value) != RESULT_OK)
    {
      cli_out(cli, "%% Timer does not meet timer relationship rules\n");
      return CLI_ERROR;
    }

  if (gmrp_set_timer (master, ifp, GARP_LEAVE_TIMER, 
       timer_value) != RESULT_OK)
  {
    cli_out (cli, "%% Unable to set %s leave timer info for %s \n", 
             argv[0], argv[2]);
    return CLI_ERROR;
  }

  return CLI_SUCCESS;
}


/* Set GARP Timers (Leaveall) */
CLI (set_gmrp_timer3,
     set_gmrp_timer3_cmd,
     "set (gmrp|mmrp) timer leaveall TIMER_VALUE IF_NAME",
     "Set GMRP leaveall timer for the specified interface",
     "GARP Multicast Registration Protocol",
     "MRP Multicast Registration Protocol",
     "GARP timer",
     "Leaveall Timer",
     "Timervalue in centiseconds",
     "Ifname")
{ /* 802.1D 14.9.1.2 */
  register struct interface *ifp;
  pal_time_t timer_value;
  struct nsm_master *nm = cli->vr->proto;
  struct nsm_bridge_master *master = nsm_bridge_get_master (nm);
  struct nsm_if *zif = NULL;

  if (GMRP_DEBUG (CLI))
  {
    zlog_info (gmrpm, "set %s timer leaveall %u %s", argv [0], argv[1], argv[2]);
  }

  CLI_GET_INTEGER_RANGE("garp_leaveall_timer", timer_value, argv[1], 1, UINT32_MAX); 

  if ( !(ifp = if_lookup_by_name (&cli->vr->ifm, argv[2])) )
    {
      cli_out (cli, "%% Interface not found\n");
      return CLI_ERROR;
    }

  zif = (struct nsm_if *)ifp->info;
  if (!zif)
  {
    cli_out (cli, "%% Interface not found\n");
    return CLI_ERROR;
  }

  /* The interface is not bound to bridge*/
  if ((zif->bridge == NULL) || (zif->bridge->gmrp_bridge == NULL))
    {
      GMRP_ALLOC_PORT_CONFIG (zif->gmrp_port_cfg,
                              zif->gmrp_port_cfg->leave_all_timeout,
                              timer_value, argv[2]);
    }

  CHECK_XMRP_IS_RUNNING(zif->bridge, argv[0]);

  if (gmrp_check_timer_rules(master, ifp, GARP_LEAVE_ALL_TIMER,
          timer_value) != RESULT_OK)
  {
    cli_out(cli, "%% Timer does not meet timer relationship rules\n");
    return CLI_ERROR;
  }

  if (gmrp_set_timer (master, ifp, GARP_LEAVE_ALL_TIMER, 
       timer_value) != RESULT_OK)
  {
    cli_out (cli, "%% Unable to set %s leaveall timer info for "
          "%s \n", argv[0], argv[2]);
    return CLI_ERROR;
  }  

  return CLI_SUCCESS;
}


/*
 * GARP Attribute type object
 */

/* TODO 14.9.2.1 Read GARP Applicant controls
   Input: ifname, garp application address, attribute type
   Output: Applicant administrative control values, Failed registrations 
           due to lack of space in the fdb

   14.9.2.2 Set GARP Applicant controls
   Input: ifname, garp application address, attribute type
   Output: None

 * GARP State Machine object

   14.9.3.1 Read GARP State 
 * Input: ifname, garp application address, gip context, attribute type, 
          attribute value
 * Output: Current value of the registrar and applicant state machine for the
           value
           (Optional) Originator address: The MAC of the originator that
           caused the state change in teh state machine 
*/


CLI (set_gmrp1,
     set_gmrp1_br_cmd,
     "set (gmrp|mmrp) enable bridge BRIDGE_NAME",
     "Enable GMRP globally for the bridge instance",
     "GARP Multicast Registration Protocol",
     "MRP Multicast Registration Protocol",
     "Enable",
     "Bridge instance ",
     "Bridge instance name")
{
  s_int32_t ret;
  struct nsm_bridge *bridge;
  struct nsm_master *nm = cli->vr->proto;
  struct nsm_bridge_master *master = nsm_bridge_get_master (nm);

  if (GMRP_DEBUG (CLI))
  {
    if (argc == 2)
      zlog_info (gmrpm, "set %s enable bridge %s", argv [0], argv[1]);
    else
      zlog_info (gmrpm, "set %s enable", argv [0]);
  }

  if (argc == 2)
    bridge = nsm_lookup_bridge_by_name (master, argv [1]);
  else
    bridge = nsm_get_default_bridge (master);

  if (! bridge)
  {
    cli_out(cli,"Bridge not found \n");
    return CLI_ERROR;
  }

  if (bridge->gmrp_bridge != NULL)
    CHECK_XMRP_IS_RUNNING(bridge, argv[0]);

  if ( nsm_bridge_is_igmp_snoop_enabled(master, bridge->name) )
  {
    cli_out(cli,"%s cannot be enabled on an IGMP Snoop enabled bridge %s\n",
            argv[0], bridge->name);
    zlog_info (gmrpm, "IGMP Snoop is already enabled on bridge %s", bridge->name);
    return CLI_ERROR;
  }

  ret = gmrp_enable (master, argv[0], bridge->name);

  if ( ret < 0 )
    {
      gmrp_cli_process_result(cli, ret, 0, bridge->name, argv[0]);
      return CLI_ERROR;
    }

  return CLI_SUCCESS;
}

ALI (set_gmrp1,
     set_gmrp1_cmd,
     "set (gmrp|mmrp) enable",
     "Enable GMRP globally for the bridge instance",
     "GARP Multicast Registration Protocol",
     "MRP Multicast Registration Protocol",
     "Enable");

CLI (set_gmrp2,
     set_gmrp2_br_cmd,
     "set (gmrp|mmrp) disable bridge BRIDGE_NAME",
     "Disable GMRP globally for the bridge instance",
     "GARP Multicast Registration Protocol",
     "MRP Multicast Registration Protocol",
     "Disable",
     "Bridge instance ",
     "Bridge instance name")
{
  s_int32_t ret;
  struct nsm_bridge *bridge;
  struct nsm_master *nm = cli->vr->proto;
  struct nsm_bridge_master *master = nsm_bridge_get_master (nm);

  if (GMRP_DEBUG (CLI))
  {
    if (argc == 2)
      zlog_info (gmrpm, "set %s enable bridge %s", argv [0], argv[1]);
    else
      zlog_info (gmrpm, "set %s enable", argv [0]);
  }

  if (argc == 2)
    bridge = nsm_lookup_bridge_by_name (master, argv [1]);
  else
    bridge = nsm_get_default_bridge (master);

  if (! bridge)
  {
    cli_out(cli,"Bridge not found \n");
    return CLI_ERROR;
  }

  if (bridge->gmrp_bridge != NULL)
    CHECK_XMRP_IS_RUNNING(bridge, argv[0]);

  ret = gmrp_disable (master, bridge->name);
  if ( ret < 0 )
    {
      gmrp_cli_process_result(cli, ret, 0, bridge->name, argv[0]);
      return CLI_ERROR;
    }

  return CLI_SUCCESS;
}

ALI (set_gmrp2,
     set_gmrp2_cmd,
     "set (gmrp|mmrp) disable",
     "Disable GMRP globally for the bridge instance",
     "GARP Multicast Registration Protocol",
     "MRP Multicast Registration Protocol",
     "Disable");

CLI (set_gmrp_instance_enable,
     set_gmrp_br_instance_enable_cmd,
     "set (gmrp|mmrp) enable bridge BRIDGE_NAME vlan VLANID",
     "Enable GMRP globally for the bridge",
     "GARP Multicast Registration Protocol",
     "MRP Multicast Registration Protocol",
     "Enable",
     "Identify the name of the bridge to use",
     "The text string to use for the name of the bridge",
     "The Vlan of the instance",
     "VLAN ID of the instance")

{
  s_int32_t ret;
  struct nsm_master *nm = cli->vr->proto;
  struct nsm_bridge_master *master = nsm_bridge_get_master (nm);
  int vid;
  struct nsm_bridge *bridge;

  if (GMRP_DEBUG (CLI))
  {
    if (argc > 2)
      zlog_info (gmrpm, "set %s enable bridge %s vlan %s", argv[0], argv[1], argv[2]);
    else
      zlog_info (gmrpm, "set %s enable vlan %s", argv[0], argv[1]);
  }

  if (argc > 2)
    bridge = nsm_lookup_bridge_by_name (master, argv [1]);
  else
    bridge = nsm_get_default_bridge (master);

  if (! bridge)
  {
    cli_out(cli,"Bridge not found \n");
    return CLI_ERROR;
  }

  if ( nsm_bridge_is_igmp_snoop_enabled(master, bridge->name) )
  {
    cli_out(cli,"%s cannot be enabled on an IGMP Snoop enabled bridge %s\n",
            argv[0], argv[1]);
    zlog_info (gmrpm, "IGMP Snoop is already enabled on bridge %s",
               bridge->name);
    return CLI_ERROR;
  }

  if (argc > 2)
    CLI_GET_INTEGER_RANGE ( "vlanid", vid, argv[2], NSM_VLAN_DEFAULT_VID,
                           NSM_VLAN_MAX );
  else
    CLI_GET_INTEGER_RANGE ( "vlanid", vid, argv[1], NSM_VLAN_DEFAULT_VID,
                            NSM_VLAN_MAX );

  ret = gmrp_enable_instance (master, argv [0], bridge->name, vid);

  if ( ret < 0 )
    {
      gmrp_cli_process_result(cli, ret, 0, bridge->name, argv[0]);
      return CLI_ERROR;
    }

  return CLI_SUCCESS;
}

ALI (set_gmrp_instance_enable,
     set_gmrp_instance_enable_cmd,
     "set (gmrp|mmrp) enable vlan VLANID",
     "Enable GMRP globally for the bridge",
     "GARP Multicast Registration Protocol",
     "MRP Multicast Registration Protocol",
     "Enable",
     "The Vlan of the instance",
     "VLAN ID of the instance");

CLI (set_gmrp_instance_disable,
     set_gmrp_br_instance_disable_cmd,
     "set (gmrp|mmrp) disable bridge BRIDGE_NAME vlan VLANID",
     "Disable GMRP globally for the bridge",
     "GARP Multicast Registration Protocol",
     "MRP Multicast Registration Protocol",
     "Disable",
     "Identify the name of the bridge to use",
     "The text string to use for the name of the bridge",
     "The Vlan of the instance",
     "VLAN ID of the instance")
{
  s_int32_t ret;
  struct nsm_master *nm = cli->vr->proto;
  struct nsm_bridge_master *master = nsm_bridge_get_master (nm);
  struct nsm_bridge *bridge;
  u_int16_t vid;

  if (GMRP_DEBUG (CLI))
  {
    if (argc > 2)
      zlog_info (gmrpm, "set %s disable bridge %s vlan %s", argv[0], argv[1], argv[2]);
    else
      zlog_info (gmrpm, "set %s disable spanning-tree vlan %s", argv[0], argv[1]);
  }

  if (argc > 2)
    bridge = nsm_lookup_bridge_by_name (master, argv [1]);
  else
    bridge = nsm_get_default_bridge (master);

  if (! bridge)
  {
    cli_out(cli,"Bridge not found \n");
    return CLI_ERROR;
  }

  if (argc > 2)
    CLI_GET_INTEGER_RANGE ( "vlanid", vid, argv[2], NSM_VLAN_DEFAULT_VID,
                            NSM_VLAN_MAX );
  else
    CLI_GET_INTEGER_RANGE ( "vlanid", vid, argv[1], NSM_VLAN_DEFAULT_VID,
                            NSM_VLAN_MAX );

  ret = gmrp_disable_instance (master, bridge->name, vid);

  if ( ret < 0 )
    {
      gmrp_cli_process_result(cli, ret, 0, bridge->name, argv[0]);
      return CLI_ERROR;
    }

  return CLI_SUCCESS;
}

ALI (set_gmrp_instance_disable,
     set_gmrp_instance_disable_cmd,
     "set (gmrp|mmrp) disable vlan VLANID",
     "Disable GMRP globally for the bridge",
     "GARP Multicast Registration Protocol",
     "MRP Multicast Registration Protocol",
     "Disable",
     "The Vlan of the instance",
     "VLAN ID of the instance");

#ifdef HAVE_MMRP

CLI (set_mmrp_point_to_point,
     set_mmrp_point_to_point_cmd,
     "set mmrp pointtopoint (enable|disable) interface IF_NAME",
     "set MMRP globally for the bridge",
     "MRP Multicast Registration Protocol",
     "point to point mode of interface",
     "enable",
     "disale",
     "Identify the name of the bridge to use",
     "The text string to use for the name of the bridge")
{
  u_char ctrl;
  struct nsm_if *zif;
  struct nsm_master *nm = cli->vr->proto;
  struct nsm_bridge_master *master = nsm_bridge_get_master (nm);
  struct interface *ifp;
  
  if (pal_strncmp (argv[1], "e", 1) == 0)
    ctrl = MMRP_POINT_TO_POINT_ENABLE;
  else if (pal_strncmp (argv[1], "d", 1) == 0)
    ctrl = MMRP_POINT_TO_POINT_DISABLE;
  else
    return CLI_ERROR;

  if ( !(ifp = if_lookup_by_name (&cli->vr->ifm, argv[2])) )
    {
      cli_out (cli, "%% Interface not found\n");
      return CLI_ERROR;
    }

  zif = (struct nsm_if *)ifp->info;

  if (zif == NULL)
    {
      cli_out (cli, "%% Interface not found\n");
      return CLI_ERROR;
    }

  /* The interface is not bound to bridge*/
  if ((zif->bridge == NULL) || (zif->bridge->gmrp_bridge == NULL))
    {
      GMRP_ALLOC_PORT_CONFIG (zif->gmrp_port_cfg,
                              zif->gmrp_port_cfg->p2p,
                              ctrl, argv[2]);
    }

  mmrp_set_pointtopoint(master, ctrl, ifp);

  return CLI_SUCCESS;
}

#endif /* HAVE_MMRP */

s_int32_t 
show_gmrp_machine_details(struct cli *cli, char *bridgename)
{
  struct nsm_if *zif;
  struct ptree_node *rn;
  struct gid   *gid = NULL;
  struct avl_node *avl_port;
  struct nsm_bridge  *bridge;
  register struct interface *ifp;
  struct avl_node  *node = NULL;
  struct gmrp_bridge *gmrp_bridge;
  struct gmrp        *gmrp = NULL;
  struct nsm_bridge_port *br_port;
  struct gid_machine *machine = NULL;
  struct gmrp_port *gmrp_port = NULL;
  struct avl_tree  *gmrp_port_list = NULL;
  struct avl_tree  *gmrp_list = NULL; 
  struct avl_node  *gmrp_node = NULL;
  struct nsm_master *nm = cli->vr->proto;
  struct gmrp_port_instance *gmrp_port_instance = NULL;
  struct nsm_bridge_master *master = nsm_bridge_get_master (nm);

  if (! master)
    return NSM_BRIDGE_ERR_NOTFOUND;

  if ( !(bridge = nsm_lookup_bridge_by_name(master, bridgename)) )
    return NSM_BRIDGE_ERR_NOTFOUND;

  /* If gmrp is not enabled on bridge return */
  if ( !(gmrp_bridge = bridge->gmrp_bridge) ||
       !(gmrp_list = bridge->gmrp_list) )
    return NSM_BRIDGE_ERR_GMRP_NOCONFIG;

  for (avl_port = avl_top (bridge->port_tree); avl_port;
       avl_port = avl_next (avl_port))
    {
      if ((br_port = (struct nsm_bridge_port *)
                             AVL_NODE_INFO (avl_port)) == NULL)
        continue;

      if ( (zif = br_port->nsmif) == NULL
          || (ifp = zif->ifp) == NULL)
        continue;

      gmrp_port_list = br_port->gmrp_port_list;
      gmrp_port = br_port->gmrp_port;

      if (!gmrp_port_list || !gmrp_port)
        continue;

      for (node = avl_top (gmrp_port_list); node; node = avl_next (node))
         {
            gmrp_port_instance = AVL_NODE_INFO (node);

            if ( !gmrp_port_instance || ! (gid = gmrp_port_instance->gid ))
              continue;

            cli_out(cli, "  port = %-6s", ifp->name);
            cli_out(cli, "  VLAN = %-4d", gmrp_port_instance->vlanid);

            gmrp = gmrp_get_by_vid (gmrp_list, gmrp_port_instance->vlanid,
                                    gmrp_port_instance->svlanid,
                                    &gmrp_node);

            if (!gmrp)
              continue;

            cli_out (cli, " \n");
            for (rn = ptree_top (gid->gid_machine_table); rn;
                 rn = ptree_next (rn))
              {
                 if (!rn->info)
                   continue;

                 machine = rn->info;

                 cli_out(cli, "%-28s", " ");
                 cli_out(cli, "  applicant state[%d] = %s",
                         gid_get_attr_index (rn),
                         applicant_states_string[machine->applicant]);
                 cli_out(cli, "  registrar state[%d] = %s\n",
                         gid_get_attr_index (rn),
                         registrar_states_string[machine->registrar]);
              }
         }
    }

 return CLI_SUCCESS;

}

s_int32_t
show_gmrp_bridge_configuration(struct cli *cli, char *reg_type,
                               char *bridgename)
{
  u_char flags;
  struct nsm_if *zif;
  u_char is_gmrp_enabled;
  struct avl_node *avl_port;
  struct nsm_bridge  *bridge;
  struct gmrp_port *gmrp_port;
  register struct interface *ifp;
  struct gmrp_bridge *gmrp_bridge;
  struct nsm_bridge_port  *br_port;
  struct avl_tree *gmrp_port_list = NULL;
  struct nsm_master *nm = cli->vr->proto;
  pal_time_t timer_details[GARP_MAX_TIMERS];
  struct nsm_bridge_master *master = nsm_bridge_get_master (nm);

  if (! master)
    return NSM_BRIDGE_ERR_NOTFOUND;
  
  if ( !(bridge = nsm_lookup_bridge_by_name(master, bridgename)) )
    return NSM_BRIDGE_ERR_NOTFOUND;

  /* If gmrp is not enabled on bridge return */
  if ( !(gmrp_bridge = bridge->gmrp_bridge) )
    return NSM_BRIDGE_ERR_GMRP_NOCONFIG;

  CHECK_XMRP_IS_RUNNING (bridge, reg_type);

#ifdef HAVE_MMRP
  if (gmrp_bridge->reg_type == REG_TYPE_MMRP)
    {
      cli_out (cli, "Global MMRP Configuration for bridge :%s\n", bridgename);
      cli_out (cli, "MMRP Feature: %s\n\n",  bridge->gmrp_bridge ? "Enabled" : "Disabled");

      cli_out (cli, "Port based MMRP Configuration:\n"); 
      cli_out (cli, "                                                    "
                    "   Timers(centiseconds)\n");
      cli_out (cli, "Port      MMRP Status    Registration    Forward All"
                    "   Join   Leave   LeaveAll\n");
      cli_out (cli, "----------------------------------------------------"
                    "--------------------------\n");
    }
  else
#endif /* HAVE_MMRP */
    {
      cli_out (cli, "Global GMRP Configuration for bridge :%s\n", bridgename);
      cli_out (cli, "GMRP Feature: %s\n\n",  bridge->gmrp_bridge ? "Enabled" : "Disabled");

      cli_out (cli, "Port based GMRP Configuration:\n");
      cli_out (cli, "                                                    "
                    "   Timers(centiseconds)\n");
      cli_out (cli, "Port      GMRP Status    Registration    Forward All"
                    "   Join   Leave   LeaveAll\n");
      cli_out (cli, "----------------------------------------------------"
                    "--------------------------\n");
    }

  for (avl_port = avl_top (bridge->port_tree); avl_port;
       avl_port = avl_next (avl_port))
    {
      if ((br_port = (struct nsm_bridge_port *)
                             AVL_NODE_INFO (avl_port)) == NULL)
        continue;

      if ((zif = br_port->nsmif) == NULL)
        continue;

      if ((ifp = zif->ifp) == NULL)
        continue;

      if (gmrp_get_port_details (master, ifp, &flags) != RESULT_OK) 
        continue;

      gmrp_get_timer_details (master, ifp, timer_details);

      gmrp_port_list = br_port->gmrp_port_list;

      if (gmrp_port_list == NULL)
        continue; 

      gmrp_port = br_port->gmrp_port;

      if (!gmrp_port)
        continue;

      is_gmrp_enabled = flags & GMRP_MGMT_PORT_CONFIGURED;

      cli_out 
         (cli, "%-10s%-15s%-16s%-14s%-7d%-8d%-8d\n", ifp->name,
          (is_gmrp_enabled ? "Enabled" : "Disabled"),
          (is_gmrp_enabled ?  
           (gmrp_port->registration_cfg == GID_REG_MGMT_FORBIDDEN ? "Forbidden" :
           (gmrp_port->registration_cfg == GID_REG_MGMT_RESTRICTED_GROUP ? "Restricted Group" : 
           (gmrp_port->registration_cfg == GID_REG_MGMT_FIXED ? "Fixed" : "Normal"))) :
           "Unknown"),
          (is_gmrp_enabled ? ((gmrp_port->forward_all_cfg & GMRP_MGMT_FORWARD_ALL_CONFIGURED) ?
                             "Enabled" : "Disabled") : "Unknown"),
          timer_details[GARP_JOIN_TIMER],
          timer_details[GARP_LEAVE_CONF_TIMER], 
          timer_details[GARP_LEAVEALL_CONF_TIMER]);
    }

  return CLI_SUCCESS;
}

CLI (show_gmrp_configuration,
     show_gmrp_configuration_br_cmd,
     "show (gmrp|mmrp) configuration bridge BRIDGE_NAME",
     "CLI_SHOW_STR",
     "GARP Multicast Registration Protocol",
     "MRP Multicast Registration Protocol",
     "Display GMRP configuration",
     "Bridge instance ",
     "Bridge instance name")
{
  struct nsm_bridge *bridge;
  struct nsm_master *nm = cli->vr->proto;
  struct nsm_bridge_master *master = nsm_bridge_get_master (nm);

  if (GMRP_DEBUG (CLI))
    {
      if (argc == 2)
        zlog_info (gmrpm, "show %s configuration bridge %s", argv[0], argv[1]);
      else
        zlog_info (gmrpm, "show %s configuration", argv[0]);
    }

  if (argc == 2)
    bridge = nsm_lookup_bridge_by_name (master, argv [1]);
  else
    bridge = nsm_get_default_bridge (master);

  if (!bridge)
  {
    cli_out(cli,"Bridge not found \n");
    return CLI_ERROR;
  }

  show_gmrp_bridge_configuration (cli, argv[0], bridge->name);

  return CLI_SUCCESS;
}

ALI (show_gmrp_configuration,
     show_gmrp_configuration_cmd,
     "show (gmrp|mmrp) configuration",
     CLI_SHOW_STR,
     "GARP Multicast Registration Protocol",
     "MRP Multicast Registration Protocol",
     "Display GMRP configuration");

CLI (show_gmrp_machine,
     show_gmrp_machine_br_cmd,
     "show (gmrp|mmrp) machine bridge BRIDGE_NAME",
     "CLI_SHOW_STR",
     "GARP Multicast Registration Protocol",
     "MRP Multicast Registration Protocol",
     "Finite State Machine",
     "Bridge",
     "Bridge name")
{
  struct nsm_bridge *bridge;
  struct nsm_master *nm = cli->vr->proto;
  struct nsm_bridge_master *master = nsm_bridge_get_master (nm);

  if (GMRP_DEBUG (CLI))
    {
      if (argc == 2)
        zlog_info (gmrpm, "show %s machine bridge %s", argv[0], argv[1]);
      else
        zlog_info (gmrpm, "show %s machine", argv[0]);
    }

  if (argc == 2)
    bridge = nsm_lookup_bridge_by_name (master, argv [1]);
  else
    bridge = nsm_get_default_bridge (master);

  if (!bridge)
  {
    cli_out(cli,"Bridge not found \n");
    return CLI_ERROR;
  }

  if (bridge->gmrp_bridge == NULL)
    {
      cli_out (cli, " %s is not configured on bridge \n", argv[0]);
      return CLI_ERROR;
    }

  CHECK_XMRP_IS_RUNNING(bridge, argv[0]);

  show_gmrp_machine_details (cli, bridge->name);

  return CLI_SUCCESS;
}

ALI (show_gmrp_machine,
     show_gmrp_machine_cmd,
     "show (gmrp|mmrp) machine",
     "CLI_SHOW_STR",
     "Generic Attribute Registration Protocol",
     "MRP Multicast Registration Protocol",
     "Finite State Machine");

CLI (set_port_gmrp1,
    set_port_gmrp1_cmd,
    "set port (gmrp|mmrp) enable (IF_NAME|all)",
    "Enable GMRP on port",
    "Layer2 Interface",
    "GARP Multicast Registration Protocol",
    "MRP Multicast Registration Protocol",
    "Enable",
    "Ifname",
    "All the ports")
{
  s_int32_t ret = 0;
  register struct interface *ifp = NULL;
  struct nsm_master *nm = cli->vr->proto;
  struct nsm_bridge_master *master = nsm_bridge_get_master (nm);

  if (GMRP_DEBUG (CLI))
  {
    zlog_info (gmrpm, "set port %s enable %s", argv[0], argv[1]);
  }

  if (pal_strcmp (argv[1], "all") == 0)
    {
      ret = gmrp_enable_port_all (master);
    }
  else
    {
      if ( !(ifp = if_lookup_by_name (&cli->vr->ifm, argv[1])) )
        {
          cli_out (cli, "%% Interface not found\n");
          return CLI_ERROR;
        }
      ret = gmrp_enable_port (master, ifp);;
    }

  if ( ret <  0 )
    {
      gmrp_cli_process_result (cli, ret, ifp, 0, argv[0]);
      return CLI_ERROR;
    }

  return CLI_SUCCESS;
}

CLI (set_port_gmrp2,
     set_port_gmrp2_cmd,
     "set port (gmrp|mmrp) disable (IF_NAME|all)",
     "Disable GMRP on interface",
     "Layer2 Interface",
     "GARP Multicast Registration Protocol",
     "MRP Multicast Registration Protocol",
     "Disable",
     "Ifname",
     "All the ports")
{
  s_int32_t ret = 0;
  register struct interface *ifp = NULL;
  struct nsm_master *nm = cli->vr->proto;
  struct nsm_bridge_master *master = nsm_bridge_get_master (nm);

  if (GMRP_DEBUG (CLI))
  {
    zlog_info (gmrpm, "set port %s disable %s", argv[0], argv[1]);
  }

  if (pal_strcmp (argv[1], "all") == 0)
    {
      ret = gmrp_disable_port_all (master);
    }
  else
    {
      if ( !(ifp = if_lookup_by_name (&cli->vr->ifm, argv[1])) )
        {
          cli_out (cli, "%% Interface not found\n");
          return CLI_ERROR;
        }

      ret = gmrp_disable_port (master, ifp);
    }

  if ( ret <  0 )
    {
      gmrp_cli_process_result (cli, ret, ifp, 0, argv [0]);
      return CLI_ERROR;
    }

  return CLI_SUCCESS;
}

CLI (set_gmrp_registration1,
     set_gmrp_registration1_cmd,
     "set (gmrp|mmrp) registration normal IF_NAME",
     "Set GMRP Registration to Normal Registration",
     "GARP Multicast Registration Protocol",
     "MRP Multicast Registration Protocol",
     "GMRP Registration",
     "Normal Registration",
     "Ifname")
{
  s_int32_t ret;
  struct nsm_if *zif;
  struct interface *ifp;
  struct nsm_master *nm = cli->vr->proto;
  struct nsm_bridge_master *master = nsm_bridge_get_master (nm);

  if (GMRP_DEBUG (CLI))
  {
    zlog_info (gmrpm, "set %s registration normal %s", argv[0], argv[1]);
  }

  if ( !(ifp = if_lookup_by_name (&cli->vr->ifm, argv[1])) )
  {
    cli_out (cli, "%% Interface not found\n");
    return CLI_ERROR;
  }

  zif = (struct nsm_if *)ifp->info;

  if ((zif ->bridge == NULL) || (zif->bridge->gmrp_bridge == NULL))
    {
      GMRP_ALLOC_PORT_CONFIG (zif->gmrp_port_cfg,
                              zif->gmrp_port_cfg->registration,
                              GID_EVENT_NORMAL_REGISTRATION, argv[1]);
    }

  CHECK_XMRP_IS_RUNNING(zif->bridge, argv[0]);

  ret = gmrp_set_registration (master, ifp, GID_EVENT_NORMAL_REGISTRATION);
                               
  if ( ret <  0 )
    {
      gmrp_cli_process_result (cli, ret, ifp, 0, argv[0]);
      return CLI_ERROR;
    }

  return CLI_SUCCESS;
}

CLI (set_port_gmrp_enable_vlan,
     set_port_gmrp_enable_vlan_cmd,
     "set port (gmrp|mmrp) enable IF_NAME vlan VLANID",
     "Enable GMRP on a port",
     "Layer2 Interface",
     "GARP Multicast Registration Protocol",
     "MRP Multicast Registration Protocol",
     "Enable",
     "Ifname",
     "Identify the VLAN to use",
     "The VLAN ID to use")
{
  s_int32_t ret;
  register struct interface *ifp;
  struct nsm_master *nm = cli->vr->proto;
  struct nsm_bridge_master *master = nsm_bridge_get_master (nm);
  struct nsm_if *zif = NULL;
  u_int16_t vid;

  CLI_GET_INTEGER_RANGE ( "vlanid", vid, argv[2], NSM_VLAN_DEFAULT_VID,
                          NSM_VLAN_MAX );

  if (GMRP_DEBUG (CLI))
  {
    zlog_info (gmrpm, "set port %s enable %s vlan %d", argv[0], argv[1], vid);
  }

  if ( !(ifp = if_lookup_by_name (&cli->vr->ifm, argv[1])) )
  {
    cli_out (cli, "%% Interface not found\n");
    return CLI_ERROR;
  }

  zif = (struct nsm_if *)ifp->info;
  if (!zif)
  {
    cli_out (cli, "%% Interface not found\n");
    return CLI_ERROR;
  }

  if ((zif ->bridge == NULL) || (zif->bridge->gmrp_bridge == NULL))
    {
       cli_out (cli, " Interface not bound to bridge \n");
       return CLI_ERROR;
    }

 
  CHECK_XMRP_IS_RUNNING(zif->bridge, argv[0]);
 
  ret = gmrp_enable_port_instance (master, ifp, vid);

  if ( ret <  0 )
    {
      gmrp_cli_process_result (cli, ret, ifp, 0, argv[0]);
      return CLI_ERROR;
    }

  return CLI_SUCCESS;
}

CLI (set_port_gmrp_disable_vlan,
     set_port_gmrp_disable_vlan_cmd,
     "set port (gmrp|mmrp) disable IF_NAME vlan VLANID",
     "Enable GMRP on a port",
     "Layer2 Interface",
     "GARP Multicast Registration Protocol",
     "MRP Multicast Registration Protocol",
     "Disable",
     "Ifname",
     "Identify the VLAN to use",
     "The VLAN ID to use")
{
  s_int32_t ret;
  register struct interface *ifp;
  struct nsm_master *nm = cli->vr->proto;
  struct nsm_bridge_master *master = nsm_bridge_get_master (nm);
  struct nsm_if *zif = NULL;
  u_int16_t vid;

  CLI_GET_INTEGER_RANGE ( "vlanid", vid, argv[2], NSM_VLAN_DEFAULT_VID,
                          NSM_VLAN_MAX );

  if (GMRP_DEBUG (CLI))
  {
    zlog_info (gmrpm, "set port %s disable %s vlan %d", argv[0], argv[1], vid);
  }

  if ( !(ifp = if_lookup_by_name (&cli->vr->ifm, argv[1])) )
  {
    cli_out (cli, "%% Interface not found\n");
    return CLI_ERROR;
  }

  zif = (struct nsm_if *)ifp->info;
  if (!zif)
  {
    cli_out (cli, "%% Interface not found\n");
    return CLI_ERROR;
  }

  if ((zif ->bridge == NULL) || (zif->bridge->gmrp_bridge == NULL))
    {
       cli_out (cli, " Interface not bound to bridge \n");
       return CLI_ERROR;
    }


  CHECK_XMRP_IS_RUNNING(zif->bridge, argv[0]);

  ret = gmrp_disable_port_instance (master, ifp, vid);

  if ( ret <  0 )
    {
      gmrp_cli_process_result (cli, ret, ifp, 0, argv[0]);
      return CLI_ERROR;
    }

  return CLI_SUCCESS;
}

CLI (set_gmrp_registration2,
     set_gmrp_registration2_cmd,
     "set (gmrp|mmrp) registration fixed IF_NAME",
     "Set GMRP Registration to Fixed Registration",
     "GARP Multicast Registration Protocol",
     "MRP Multicast Registration Protocol",
     "GMRP Registration",
     "Fixed Registration",
     "Ifname")
{
  s_int32_t ret;
  struct interface *ifp;
  struct nsm_master *nm = cli->vr->proto;
  struct nsm_bridge_master *master = nsm_bridge_get_master (nm);
  struct nsm_if *zif = NULL;

  if (GMRP_DEBUG (CLI))
  {
    zlog_info (gmrpm, "set %s registration fixed %s", argv[0], argv[1]);
  }

  if ( !(ifp = if_lookup_by_name (&cli->vr->ifm, argv[1])) )
  {
    cli_out (cli, "%% Interface not found\n");
    return CLI_ERROR;
  }

  zif = (struct nsm_if *)ifp->info;
  if (!zif)
  {
    cli_out (cli, "%% Interface not found\n");
    return CLI_ERROR;
  }

  /* The interface is not bound to bridge*/
  if ((zif->bridge == NULL) || (zif->bridge->gmrp_bridge == NULL))
    {
      GMRP_ALLOC_PORT_CONFIG (zif->gmrp_port_cfg,
                              zif->gmrp_port_cfg->registration,
                              GID_EVENT_FIXED_REGISTRATION, argv[1]);
    }

  CHECK_XMRP_IS_RUNNING(zif->bridge, argv[0]);

  ret = gmrp_set_registration (master, ifp, GID_EVENT_FIXED_REGISTRATION);
                               
  if ( ret <  0 )
    {
      gmrp_cli_process_result (cli, ret, ifp, 0, argv[0]);
      return CLI_ERROR;
    }

  return CLI_SUCCESS;
}

CLI (set_gmrp_registration3,
     set_gmrp_registration3_cmd,
     "set (gmrp|mmrp) registration forbidden IF_NAME",
     "Set GMRP Registration to Forbidden Registration",
     "GARP Multicast Registration Protocol",
     "MRP Multicast Registration Protocol",
     "GMRP Registration",
     "Forbidden Registration",
     "Interface name")
{
  s_int32_t ret;
  struct interface *ifp;
  struct nsm_master *nm = cli->vr->proto;
  struct nsm_bridge_master *master = nsm_bridge_get_master (nm);
  struct nsm_if *zif = NULL;

  if (GMRP_DEBUG (CLI))
  {
    zlog_info (gmrpm, "set %s registration forbidden %s", argv[0], argv[1]);
  }

  if ( !(ifp = if_lookup_by_name (&cli->vr->ifm, argv[1])) )
  {
    cli_out (cli, "%% Interface not found\n");
    return CLI_ERROR;
  }

  zif = (struct nsm_if *)ifp->info;
  if (!zif)
  {
    cli_out (cli, "%% Interface not found\n");
    return CLI_ERROR;
  }

  /* The interface is not bound to bridge*/
  if ((zif->bridge == NULL) || (zif->bridge->gmrp_bridge == NULL))
    {
      GMRP_ALLOC_PORT_CONFIG (zif->gmrp_port_cfg,
                              zif->gmrp_port_cfg->registration,
                              GID_EVENT_FORBID_REGISTRATION, argv[1]);
    }

  CHECK_XMRP_IS_RUNNING(zif->bridge, argv[0]);

  ret = gmrp_set_registration (master, ifp, GID_EVENT_FORBID_REGISTRATION);
                              
  if ( ret <  0 )
    {
      gmrp_cli_process_result (cli, ret, ifp, 0, argv[0]);
      return CLI_ERROR;
    }

  return CLI_SUCCESS;
}

CLI (set_gmrp_registration4,
     set_gmrp_registration4_cmd,
     "set (gmrp|mmrp) registration restricted IF_NAME",
     "Set GMRP Registration to Restricted Registration",
     "GARP Multicast Registration Protocol",
     "MRP Multicast Registration Protocol",
     "GMRP Registration",
     "Restricted Registration",
     "Interface name")
{
  s_int32_t ret;
  struct interface *ifp;
  struct nsm_master *nm = cli->vr->proto;
  struct nsm_bridge_master *master = nsm_bridge_get_master (nm);
  struct nsm_if *zif = NULL;

  if (GMRP_DEBUG (CLI))
  {
    zlog_info (gmrpm, "set %s registration restricted %s", argv[0], argv[1]);
  }

  if ( !(ifp = if_lookup_by_name (&cli->vr->ifm, argv[1])) )
  {
    cli_out (cli, "%% Interface not found\n");
    return CLI_ERROR;
  }

  zif = (struct nsm_if *)ifp->info;
  if (!zif)
  {
    cli_out (cli, "%% Interface not found\n");
    return CLI_ERROR;
  }

 if ((zif->bridge == NULL) || (zif->bridge->gmrp_bridge == NULL))
   {
      GMRP_ALLOC_PORT_CONFIG (zif->gmrp_port_cfg,
                              zif->gmrp_port_cfg->registration,
                              GID_EVENT_RESTRICTED_GROUP_REGISTRATION, argv[1]);
   }

  CHECK_XMRP_IS_RUNNING(zif->bridge, argv[0]);

  ret = gmrp_set_registration (master, ifp, GID_EVENT_RESTRICTED_GROUP_REGISTRATION);

  if ( ret <  0 )
    {
      gmrp_cli_process_result (cli, ret, ifp, 0, argv[0]);
      return CLI_ERROR;
    }

  return CLI_SUCCESS;
}

CLI (show_gmrp_statistics,
     show_gmrp_br_statistics_cmd,
     "show (gmrp|mmrp) statistics vlanid <1-4094> (bridge <1-32>|)",
     "CLI_SHOW_STR",
     "gmrp",
     "MRP Multicast Registration Protocol",
     "GMRP Statistics for the given vlanid",
     "Vlanid",
     "Vlanid value <1-4094>",
     "Bridge instance",
     "Bridge instance name")
{
  u_int16_t vid;
  struct gmrp_bridge *gmrp_bridge;
#ifdef HAVE_MMRP
  u_int32_t receive_counters[MRP_ATTR_EVENT_MAX + 1];
  u_int32_t transmit_counters[MRP_ATTR_EVENT_MAX + 1];
#else
  u_int32_t receive_counters[GARP_ATTR_EVENT_MAX + 1];
  u_int32_t transmit_counters[GARP_ATTR_EVENT_MAX + 1];
#endif /* HAVE_MMRP */

  struct nsm_master *nm = cli->vr->proto;
  struct nsm_bridge_master *master = nsm_bridge_get_master (nm);
  struct nsm_bridge *bridge;

  gmrp_bridge = NULL;
  
  if (GMRP_DEBUG (CLI))
    {
      if (argc > 2)
        zlog_info (gmrpm, "show %s statistics vlanid %s bridge %s", 
                   argv[0], argv[1], argv[3]);
      else
        zlog_info (gmrpm, "show %s statistics vlanid %s spanning-tree",
                   argv[0], argv[1]);
    }

  CLI_GET_INTEGER_RANGE ("vlanid", vid, argv[1], NSM_VLAN_DEFAULT_VID,
                         NSM_VLAN_MAX);

  if (argc > 2)
    bridge = nsm_lookup_bridge_by_name (master, argv [3]);
  else
    bridge = nsm_get_default_bridge (master);

  if (!bridge)
    {
      cli_out(cli,"Bridge not found \n");
      return CLI_ERROR;
    }
 
 if ((gmrp_bridge = bridge->gmrp_bridge) == NULL)
   {
     cli_out (cli, " %s is not configured on bridge \n", argv[0]);
     return CLI_ERROR;
   }

  CHECK_XMRP_IS_RUNNING(bridge, argv[0]);

  if (gmrp_get_per_vlan_statistics_details (master, bridge->name, vid, 
                                            receive_counters, 
                                            transmit_counters) != RESULT_OK)
    {
      cli_out (cli, "%% Unable to show %s per vlan statistics for vlanid "
               "%u bridge %s \n", argv[0], vid, bridge->name);
      return CLI_ERROR;
    }

#ifdef HAVE_MMRP
  if (gmrp_bridge->reg_type == REG_TYPE_MMRP)
    {
      cli_out (cli, "MMRP Statistics for bridge %s vlan %u\n", bridge->name, vid);
      cli_out (cli, "---------------------------------------------------\n");
      cli_out (cli, "Total MMRP Events Received:       %d\n", 
               receive_counters[MRP_ATTR_EVENT_MAX]);

      cli_out (cli, "New Ins:                          %d\n",
               receive_counters[MRP_ATTR_EVENT_NEW_IN]);
      cli_out (cli, "Join Ins:                         %d\n",
               receive_counters[MRP_ATTR_EVENT_JOIN_IN]);
      cli_out (cli, "Leave:                            %d\n",
               receive_counters[MRP_ATTR_EVENT_LEAVE]);
      cli_out (cli, "Null:                             %d\n",
               receive_counters[MRP_ATTR_EVENT_NULL]);
      cli_out (cli, "New Empties:                      %d\n",
               receive_counters[MRP_ATTR_EVENT_NEW_EMPTY]);
      cli_out (cli, "Join Empties:                     %d\n",
               receive_counters[MRP_ATTR_EVENT_JOIN_EMPTY]);
      cli_out (cli, "Empties:                          %d\n",
               receive_counters[MRP_ATTR_EVENT_EMPTY]);
      cli_out (cli, "\n");

      cli_out (cli, "Total MMRP Events Transmitted:    %d\n", 
               transmit_counters[MRP_ATTR_EVENT_MAX]);

      cli_out (cli, "New Ins:                          %d\n",
               transmit_counters[MRP_ATTR_EVENT_NEW_IN]);
      cli_out (cli, "Join Ins:                         %d\n",
               transmit_counters[MRP_ATTR_EVENT_JOIN_IN]);
      cli_out (cli, "Leave:                            %d\n",
               transmit_counters[MRP_ATTR_EVENT_LEAVE]);
      cli_out (cli, "Null:                             %d\n",
               transmit_counters[MRP_ATTR_EVENT_NULL]);
      cli_out (cli, "New Empties:                      %d\n",
               transmit_counters[MRP_ATTR_EVENT_NEW_EMPTY]);
      cli_out (cli, "Join Empties:                     %d\n",
               transmit_counters[MRP_ATTR_EVENT_JOIN_EMPTY]);
      cli_out (cli, "Empties:                          %d\n",
               transmit_counters[MRP_ATTR_EVENT_EMPTY]);
      cli_out (cli, "\n");
    }
  else
#endif /* HAVE_MMRP */
    {
      cli_out (cli, "GMRP Statistics for bridge %s vlan %u\n", bridge->name, vid);
      cli_out (cli, "---------------------------------------------------\n");
      cli_out (cli, "Total GMRP packets Received:      %d\n", 
               receive_counters[GARP_ATTR_EVENT_MAX]);
  
      cli_out (cli, "Leave alls:                       %d\n",
               receive_counters[GARP_ATTR_EVENT_LEAVE_ALL]);
      cli_out (cli, "Join Empties:                     %d\n",
               receive_counters[GARP_ATTR_EVENT_JOIN_EMPTY]);
      cli_out (cli, "Join Ins:                         %d\n",
               receive_counters[GARP_ATTR_EVENT_JOIN_IN]);
      cli_out (cli, "Leave Empties:                    %d\n",
               receive_counters[GARP_ATTR_EVENT_LEAVE_EMPTY]);
      cli_out (cli, "Leave Ins:                        %d\n",
               receive_counters[GARP_ATTR_EVENT_LEAVE_IN]);
      cli_out (cli, "Empties:                          %d\n",
               receive_counters[GARP_ATTR_EVENT_EMPTY]);
      cli_out (cli, "\n");

      cli_out (cli, "Total GMRP packets Transmitted:   %d\n", 
               transmit_counters[GARP_ATTR_EVENT_MAX]);

      cli_out (cli, "Leave alls:                       %d\n",
               transmit_counters[GARP_ATTR_EVENT_LEAVE_ALL]);
      cli_out (cli, "Join Empties:                     %d\n",
               transmit_counters[GARP_ATTR_EVENT_JOIN_EMPTY]);
      cli_out (cli, "Join Ins:                         %d\n",
               transmit_counters[GARP_ATTR_EVENT_JOIN_IN]);
      cli_out (cli, "Leave Empties:                    %d\n",
               transmit_counters[GARP_ATTR_EVENT_LEAVE_EMPTY]);
      cli_out (cli, "Leave Ins:                        %d\n",
               transmit_counters[GARP_ATTR_EVENT_LEAVE_IN]);
      cli_out (cli, "Empties:                          %d\n",
               transmit_counters[GARP_ATTR_EVENT_EMPTY]);
      cli_out (cli, "\n");
    }

  return CLI_SUCCESS;
}
 
ALI (show_gmrp_statistics,
     show_gmrp_statistics_cmd,
     "show (gmrp|mmrp) statistics vlanid <1-4094>",
     "CLI_SHOW_STR",
     "gmrp",
     "MRP Multicast Registration Protocol",
     "GMRP Statistics for the given vlanid",
     "Vlanid",
     "Vlanid value <1-4094>");

CLI (clear_gmrp_statistics1,
     clear_gmrp_br_statistics1_cmd,
     "clear (gmrp|mmrp) statistics all bridge BRIDGE_NAME",
     "Clear GMRP statistics for all vlans",
     "GARP Multicast Registration Protocol",
     "MRP Multicast Registration Protocol",
     "GMRP Statistics",
     "All vlans",
     "Bridge instance",
     "Bridge instance name")
{
  s_int32_t ret;
  struct nsm_master *nm = cli->vr->proto;
  struct nsm_bridge_master *master = nsm_bridge_get_master (nm);
  struct nsm_bridge *bridge;
  
  if (GMRP_DEBUG (CLI))
    {
      if (argc > 1)
        zlog_info (gmrpm, "clear %s statistics all bridge %s", argv[0], argv[1]);
      else
        zlog_info (gmrpm, "clear %s statistics all spanning-tree", argv[0]);
    }
  
  if (argc > 1)
    bridge = nsm_lookup_bridge_by_name (master, argv [1]);
  else
    bridge = nsm_get_default_bridge (master);

 if (! bridge)
  {
    cli_out(cli,"Bridge not found \n");
    return CLI_ERROR;
  }

 if (bridge->gmrp_bridge == NULL)
   {
     cli_out (cli, "%s is not configured on bridge \n", argv[0]);
     return CLI_ERROR;
   }

  CHECK_XMRP_IS_RUNNING(bridge, argv[0]);

  ret = gmrp_clear_bridge_statistics (master, bridge->name );
  if ( ret < 0 )
    {
      cli_out (cli, "%% Unable to clear all vlans %s statistics for bridge"
               " %s \n", argv[0], bridge->name);
      return CLI_ERROR;
    }

  return CLI_SUCCESS;
}

ALI (clear_gmrp_statistics1,
     clear_gmrp_statistics1_cmd,
     "clear (gmrp|mmrp) statistics all",
     "Clear GMRP statistics for all vlans",
     "GARP Multicast Registration Protocol",
     "MRP Multicast Registration Protocol",
     "GMRP Statistics",
     "All vlans");

CLI (clear_gmrp_statistics2,
     clear_gmrp_br_statistics2_cmd,
     "clear (gmrp|mmrp) statistics vlanid <1-4094> bridge <1-32>",
     "Clear GMRP statistics for given vlan",
     "GARP Multicast Registration Protocol",
     "MRP Multicast Registration Protocol",
     "GMRP Statistics",
     "Vlanid",
     "Vlanid value <1-4094>",
     "Bridge instance",
     "Bridge instance name")
{
  u_int16_t vid;
  struct nsm_master *nm = cli->vr->proto;
  struct nsm_bridge_master *master = nsm_bridge_get_master (nm);
  struct nsm_bridge *bridge;

  if (GMRP_DEBUG (CLI))
    {
      if (argc > 2)
        zlog_info (gmrpm, "clear %s statistics vlanid %u bridge %s", 
                   argv[0], argv[1], argv[2]);
      else
        zlog_info (gmrpm, "clear %s statistics vlanid %u spanning-tree",
                   argv[0], argv[1]);
    }
  
  if (argc > 2)
    bridge = nsm_lookup_bridge_by_name (master, argv [2]);
  else
    bridge = nsm_get_default_bridge (master);

  if (! bridge)
    {
      cli_out(cli,"Bridge not found \n");
      return CLI_ERROR;
    }

  if (bridge->gmrp_bridge == NULL)
    {
      cli_out (cli, "%s is not configured on bridge \n", argv[0]);
      return CLI_ERROR;
    }

  CHECK_XMRP_IS_RUNNING(bridge, argv[0]);

  CLI_GET_INTEGER_RANGE ("vlanid", vid, argv[1], NSM_VLAN_DEFAULT_VID,
                         NSM_VLAN_MAX);

  if (gmrp_clear_per_vlan_statistics (master, bridge->name, vid) != RESULT_OK)
    {
      cli_out (cli, "%% Unable to clear %s statistics for vlan %u bridge"
               " %s \n", argv[0], argv[1], bridge->name);
      return CLI_ERROR;
    }

  return CLI_SUCCESS;
}

ALI (clear_gmrp_statistics2,
     clear_gmrp_statistics2_cmd,
     "clear (gmrp|mmrp) statistics vlanid <1-4094>",
     "Clear GMRP statistics for given vlan",
     "GARP Multicast Registration Protocol",
     "MRP Multicast Registration Protocol",
     "GMRP Statistics",
     "Vlanid",
     "Vlanid value <1-4094>");

CLI (set_gmrp_fwd_all1,
     set_gmrp_fwd_all1_cmd,
     "set (gmrp|mmrp) fwdall enable IF_NAME",
     "Set GMRP Forward All Option",
     "GARP Multicast Registration Protocol",
     "MRP Multicast Registration Protocol",
     "Forward All Option",
     "Enable",
     "Ifname")
{
  struct interface *ifp;
  s_int32_t ret;
  struct nsm_master *nm = cli->vr->proto;
  struct nsm_bridge_master *master = nsm_bridge_get_master (nm);
  struct nsm_if *zif = NULL;

  ifp = if_lookup_by_name (&cli->vr->ifm, argv[1]);
  if (ifp == NULL)
  {
    cli_out (cli, "%% Interface %s not found\n", argv[1]);
    return CLI_ERROR;
  }

  if (GMRP_DEBUG (CLI))
  {
    zlog_info (gmrpm, "set %s fwdall enable %s", argv[0], argv[1]);
  }

  zif = (struct nsm_if *)ifp->info;
  if (!zif)
  {
    cli_out (cli, "%% Interface not found\n");
    return CLI_ERROR;
  }

  /* The interface is not bound to bridge*/
  if ((zif->bridge == NULL) || (zif->bridge->gmrp_bridge == NULL))
    {
      GMRP_ALLOC_PORT_CONFIG (zif->gmrp_port_cfg,
                              zif->gmrp_port_cfg->fwd_all,
                              PAL_TRUE, argv[1]);
    }

  CHECK_XMRP_IS_RUNNING(zif->bridge, argv[0]);

  ret = gmrp_set_fwd_all (master, ifp, GID_EVENT_JOIN);
  if ( ret < 0 )
    {
      gmrp_cli_process_result(cli, ret, ifp, 0, argv[0]);
      return CLI_ERROR;
    }
  
  return CLI_SUCCESS;
}

CLI (set_gmrp_fwd_all2,
     set_gmrp_fwd_all2_cmd,
     "set (gmrp|mmrp) fwdall disable IF_NAME",
     "Reset GMRP Forward All Option",
     "GARP Multicast Registration Protocol",
     "MRP Multicast Registration Protocol",
     "Forward All Option",
     "Disable",
     "Ifname")
{
  struct interface *ifp;
  s_int32_t ret;
  struct nsm_master *nm = cli->vr->proto;
  struct nsm_bridge_master *master = nsm_bridge_get_master (nm);
  struct nsm_if *zif;

  ifp = if_lookup_by_name (&cli->vr->ifm, argv[1]);
  if (ifp == NULL)
  {
    cli_out (cli, "%% Interface %s not found\n", argv[1]);
    return CLI_ERROR;
  }

  zif = (struct nsm_if *) ifp->info;

  if ((zif->bridge == NULL) || (zif->bridge->gmrp_bridge == NULL))
    {
      cli_out (cli, "Bridge not found \n");
      return CLI_ERROR;
    }

  CHECK_XMRP_IS_RUNNING(zif->bridge, argv[0]);

  if (GMRP_DEBUG (CLI))
  {
    zlog_info (gmrpm, "set %s fwdall disable %s", argv[0], argv[1]);
  }

  ret = gmrp_set_fwd_all (master, ifp, GID_EVENT_LEAVE);
  if ( ret < 0 )
    {
      gmrp_cli_process_result(cli, ret, ifp, 0, argv[0]);
      return CLI_ERROR;
    }
  
  return CLI_SUCCESS;
}

static void
gmrp_show_gmrp_instance (struct cli *cli, struct avl_tree *gmrp_list,
                         struct nsm_bridge *bridge, char *proto)
{

  struct avl_node *node = NULL;
  struct gmrp     *gmrp = NULL;

  if (!cli || !gmrp_list || !bridge)
    return;

   for (node = avl_top (gmrp_list); node; node = avl_next (node))
      {
        gmrp = AVL_NODE_INFO (node);

        if (gmrp)
          {
            cli_out (cli, "set %s enable %s vlan %d \n", proto,
                     nsm_get_bridge_str (bridge), gmrp->vlanid);
          }
      }

  return;
}

static void
gmrp_show_gmrp_port_instance (struct cli *cli, struct avl_tree *gmrp_port_list,
                              struct interface *ifp, char *proto)
{

  struct avl_node *node = NULL;
  struct gmrp_port_instance  *gmrp_port_instance = NULL;

  if (!cli || !gmrp_port_list || !ifp)
    return;

   for (node = avl_top (gmrp_port_list); node; node = avl_next (node))
      {
        gmrp_port_instance = AVL_NODE_INFO (node);

        if (gmrp_port_instance)
          {
            cli_out (cli, "set port %s enable %s vlan %d \n",
                     proto, ifp->name, gmrp_port_instance->vlanid);
          }
      }
  return;
}


CLI (set_gmrp_ext_filtering,
     set_gmrp_ext_filtering_br_cmd,
     "set (gmrp|mmrp) extended-filtering enable bridge BRIDGE_NAME ",
     "Enable Extended filtering Option",
     "GARP Multicast Registration Protocol",
     "MRP Multicast Registration Protocol",
     "Extended filtering Option",
     "Enable",
     "Bridge",
     "BRIDGE_NAME")
{
  s_int32_t ret;
  struct nsm_bridge *bridge;
  struct nsm_master *nm = cli->vr->proto;
  struct nsm_bridge_master *master = nsm_bridge_get_master (nm);

  if (GMRP_DEBUG (CLI))
  {
    if (argc == 2)
      zlog_info (gmrpm, "set %s extended-filtering enable bridge %s", argv[0], argv[1]);
    else
      zlog_info (gmrpm, "set %s extended-filtering enable", argv[0]);
  }

  if (argc == 2)
    bridge = nsm_lookup_bridge_by_name (master, argv [1]);
  else
    bridge = nsm_get_default_bridge (master);

  if (! bridge)
  {
    cli_out(cli,"Bridge not found \n");
    return CLI_ERROR;
  }

  if ( nsm_bridge_is_igmp_snoop_enabled(master, bridge->name))
  {
    cli_out(cli,"%s extended filtering cannot be enabled on an IGMP Snoop "
            "enabled bridge %s\n", argv[0], argv[1]);
    zlog_info (gmrpm, "IGMP Snoop is already enabled on bridge %s", bridge->name);

    return CLI_ERROR;
  }

  ret = gmrp_extended_filtering (master, bridge->name, PAL_TRUE);

  if ( ret < 0 )
    {
      gmrp_cli_process_result(cli, ret, 0, bridge->name, argv[0]);
      return CLI_ERROR;
    }

  return CLI_SUCCESS;

}

ALI (set_gmrp_ext_filtering,
     set_gmrp_ext_filtering_cmd,
     "set (gmrp|mmrp) extended-filtering enable",
     "Enable Extended filtering Option",
     "GARP Multicast Registration Protocol",
     "MRP Multicast Registration Protocol",
     "Extended filtering Option",
     "Enable");

CLI (set_gmrp_ext_filtering2,
     set_gmrp_ext_filtering2_br_cmd,
     "set (gmrp|mmrp) extended-filtering disable bridge BRIDGE_NAME ",
     "Enable Extended filtering Option",
     "GARP Multicast Registration Protocol",
     "MRP Multicast Registration Protocol",
     "Extended filtering Option",
     "disable",
     "Bridge",
     "BRIDGE_NAME")
{
  s_int32_t ret;
  struct nsm_bridge *bridge;
  struct nsm_master *nm = cli->vr->proto;
  struct nsm_bridge_master *master = nsm_bridge_get_master (nm);

  if (GMRP_DEBUG (CLI))
  {
    if (argc == 2)
      zlog_info (gmrpm, "set %s extended-filtering enable bridge %s", argv[0], argv[1]);
    else
      zlog_info (gmrpm, "set %s extended-filtering enable", argv[0]);
  }

  if (argc == 2)
    bridge = nsm_lookup_bridge_by_name (master, argv [1]);
  else
    bridge = nsm_get_default_bridge (master);

  if (! bridge)
  {
    cli_out(cli,"Bridge not found \n");
    return CLI_ERROR;
  }

  if ( nsm_bridge_is_igmp_snoop_enabled(master, bridge->name) )
  {
    cli_out(cli,"%s extended filtering cannot be enabled on an IGMP Snoop "
            "enabled bridge %s\n",argv[0], argv[1]);
    zlog_info (gmrpm, "IGMP Snoop is already enabled on bridge %s", bridge->name);
    return CLI_ERROR;
  }

  ret = gmrp_extended_filtering (master, bridge->name, PAL_FALSE);

  if ( ret < 0 )
    {
      gmrp_cli_process_result(cli, ret, 0, bridge->name, argv[0]);
      return CLI_ERROR;
    }

  return CLI_SUCCESS;

}

ALI (set_gmrp_ext_filtering2,
     set_gmrp_ext_filtering2_cmd,
     "set (gmrp|mmrp) extended-filtering disable",
     "Enable Extended filtering Option",
     "GARP Multicast Registration Protocol",
     "MRP Multicast Registration Protocol",
     "Extended filtering Option",
     "disable");

/* GMRP configuration write function */
int 
gmrp_config_write (struct cli *cli)
{
  char *proto;
  u_int32_t write = 0;
  struct    nsm_if *zif;
  struct    interface * ifp;
  struct    nsm_bridge *bridge;
  struct    avl_node *avl_port;
  struct    nsm_bridge_port *br_port;
  struct    gmrp_port   *gmrp_port = NULL;
  pal_time_t timer_details[GARP_MAX_TIMERS];
  struct    gmrp_bridge *gmrp_bridge = NULL;
  struct    nsm_master *nm = cli->vr->proto;
  struct    avl_tree   *gmrp_port_list = NULL;
  struct    nsm_bridge_master *master = nsm_bridge_get_master (nm);

  if (GMRP_DEBUG (CLI))
    cli_out (cli, "debug gmrp cli\n");
  if (GMRP_DEBUG (PACKET))
    cli_out (cli, "debug gmrp packet\n");
  if (GMRP_DEBUG (TIMER))
    cli_out (cli, "debug gmrp timer\n");
  if (GMRP_DEBUG (EVENT))
    cli_out (cli, "debug gmrp event\n");

  for (bridge = nsm_get_first_bridge(master); bridge;
       bridge = nsm_get_next_bridge(master, bridge))
  {
    gmrp_bridge = bridge->gmrp_bridge;

    if (gmrp_bridge != NULL)
      {
        proto = GMRP_REG_TYPE_TO_STR (gmrp_bridge->reg_type);

        write++;

        if (gmrp_bridge->globally_enabled)
          {
            cli_out (cli, "set %s enable %s\n", proto, nsm_get_bridge_str (bridge));
          }
        else
          {
            gmrp_show_gmrp_instance (cli, bridge->gmrp_list, bridge, proto);
          } 

        cli_out (cli, "set %s extended-filtering %s %s \n", 
                 proto, bridge->gmrp_ext_filter ? "enable" : "disable",
                 nsm_get_bridge_str (bridge));

        for (avl_port = avl_top (bridge->port_tree); avl_port;
             avl_port = avl_next (avl_port))
           {
             if ((br_port = (struct nsm_bridge_port *)
                                    AVL_NODE_INFO (avl_port)) == NULL)
               continue;

             if ((zif = br_port->nsmif) == NULL)
               continue;

             if ((ifp = zif->ifp) == NULL)
               continue;

             gmrp_port_list = br_port->gmrp_port_list;
             gmrp_port      = br_port->gmrp_port;

             if ( !gmrp_port_list || !gmrp_port)
               continue;

             gmrp_get_timer_details (master, ifp, timer_details);
 
             if (gmrp_port->globally_enabled)
               {
                 cli_out (cli, "set port %s enable %s\n", proto, ifp->name);
               }
             else
               {
                 gmrp_show_gmrp_port_instance (cli, gmrp_port_list, ifp, proto);
               }

             if (timer_details[GARP_JOIN_TIMER] != GID_DEFAULT_JOIN_TIME)
               cli_out (cli, "set %s timer join %d %s\n",
                        proto, timer_details[GARP_JOIN_TIMER], ifp->name);

             if (timer_details[GARP_LEAVE_CONF_TIMER] != GID_DEFAULT_LEAVE_TIME)
               cli_out (cli, "set %s timer leave %d %s\n",
                        proto, timer_details[GARP_LEAVE_CONF_TIMER], ifp->name);

             if (timer_details[GARP_LEAVEALL_CONF_TIMER]
                  !=  GID_DEFAULT_LEAVE_ALL_TIME)
               cli_out (cli, "set %s timer leaveall %d %s\n",
                        proto, timer_details[GARP_LEAVEALL_CONF_TIMER], ifp->name);

             if (gmrp_port->registration_cfg == GID_REG_MGMT_FIXED)
               cli_out (cli, "set %s registration fixed %s\n",
                        proto, ifp->name);
             else if (gmrp_port->registration_cfg
                       == GID_REG_MGMT_FORBIDDEN)
               cli_out (cli, "set %s registration forbidden %s\n",
                        proto, ifp->name);
             else if (gmrp_port->registration_cfg
                       ==  GID_REG_MGMT_RESTRICTED_GROUP)
               cli_out (cli, "set %s registration restricted group %s\n",
                        proto, ifp->name);

             if (gmrp_port->forward_all_cfg
                 == GMRP_MGMT_FORWARD_ALL_CONFIGURED)
               cli_out (cli, "set %s fwdall enable %s\n",
                        proto, ifp->name);

#ifdef HAVE_MMRP
             if ((gmrp_port->gid_port != NULL)
                 && (gmrp_port->gid_port->p2p) == PAL_TRUE)
               cli_out (cli, "set mmrp pointtopoint enable interface %s\n",
                        ifp->name);
#endif /* HAVE_MMRP */

           }
      }
  }

  return write;
}

void
gmrp_cli_init (struct lib_globals *zg)
{
  struct cli_tree *ctree = zg->ctree;

  cli_install_config (ctree, GMRP_MODE, &gmrp_config_write);

  /* enable debug commands */
  cli_install (ctree, CONFIG_MODE, &gmrp_debug_event_cmd);
  cli_install (ctree, CONFIG_MODE, &gmrp_debug_cli_cmd);
  cli_install (ctree, CONFIG_MODE, &gmrp_debug_timer_cmd);
  cli_install (ctree, CONFIG_MODE, &gmrp_debug_packet_cmd);
  cli_install (ctree, CONFIG_MODE, &gmrp_debug_all_cmd);

  /* disable debug commands */
  cli_install (ctree, CONFIG_MODE, &no_gmrp_debug_event_cmd);
  cli_install (ctree, CONFIG_MODE, &no_gmrp_debug_cli_cmd);
  cli_install (ctree, CONFIG_MODE, &no_gmrp_debug_timer_cmd);
  cli_install (ctree, CONFIG_MODE, &no_gmrp_debug_packet_cmd);
  cli_install (ctree, CONFIG_MODE, &no_gmrp_debug_all_cmd);

   /* display commands */
   cli_install (ctree, EXEC_MODE, &show_gmrp_timer_cmd);
   cli_install (ctree, EXEC_MODE, &show_gmrp_statistics_cmd);
   cli_install (ctree, EXEC_MODE, &show_gmrp_br_statistics_cmd);
   cli_install (ctree, EXEC_MODE, &show_gmrp_configuration_cmd);
   cli_install (ctree, EXEC_MODE, &show_gmrp_machine_cmd);
   cli_install (ctree, EXEC_MODE, &show_gmrp_configuration_br_cmd);
   cli_install (ctree, EXEC_MODE, &show_gmrp_machine_br_cmd);
   cli_install (ctree, EXEC_MODE, &show_debugging_gmrp_cmd);

   /* statistics clear commands */
   cli_install (ctree, EXEC_MODE, &clear_gmrp_statistics1_cmd);
   cli_install (ctree, EXEC_MODE, &clear_gmrp_br_statistics1_cmd);
   cli_install (ctree, EXEC_MODE, &clear_gmrp_statistics2_cmd);
   cli_install (ctree, EXEC_MODE, &clear_gmrp_br_statistics2_cmd);

   /* "timers" commands. */
   cli_install (ctree, CONFIG_MODE, &set_gmrp_timer1_cmd);
   cli_install (ctree, CONFIG_MODE, &set_gmrp_timer2_cmd);
   cli_install (ctree, CONFIG_MODE, &set_gmrp_timer3_cmd);
   
   /* global GMRP enable/disable commands */
   cli_install (ctree, CONFIG_MODE, &set_gmrp1_cmd);
   cli_install (ctree, CONFIG_MODE, &set_gmrp2_cmd);
   cli_install (ctree, CONFIG_MODE, &set_gmrp1_br_cmd);
   cli_install (ctree, CONFIG_MODE, &set_gmrp2_br_cmd);

   /* poer port GMRP enable/disable commands */
   cli_install (ctree, CONFIG_MODE, &set_port_gmrp1_cmd);
   cli_install (ctree, CONFIG_MODE, &set_port_gmrp2_cmd);

   /* Per Vlan GMRP Enable/ Disable commands */
   cli_install (ctree, CONFIG_MODE, &set_gmrp_instance_enable_cmd);
   cli_install (ctree, CONFIG_MODE, &set_gmrp_br_instance_enable_cmd);
   cli_install (ctree, CONFIG_MODE, &set_gmrp_instance_disable_cmd);
   cli_install (ctree, CONFIG_MODE, &set_gmrp_br_instance_disable_cmd);
   cli_install (ctree, CONFIG_MODE, &set_port_gmrp_enable_vlan_cmd);
   cli_install (ctree, CONFIG_MODE, &set_port_gmrp_disable_vlan_cmd);


   /* per port GMRP registration (normal, fixed, forbidden) commands */
   cli_install (ctree, CONFIG_MODE, &set_gmrp_registration1_cmd);
   cli_install (ctree, CONFIG_MODE, &set_gmrp_registration2_cmd);
   cli_install (ctree, CONFIG_MODE, &set_gmrp_registration3_cmd);
   cli_install (ctree, CONFIG_MODE, &set_gmrp_registration4_cmd);
#if defined HAVE_MMRP
   /* point to point control per port */
   cli_install (ctree, CONFIG_MODE, &set_mmrp_point_to_point_cmd);
#endif

   /* per port GMRP registration (normal, fixed, forbidden) commands */
   cli_install (ctree, CONFIG_MODE, &set_gmrp_fwd_all1_cmd);
   cli_install (ctree, CONFIG_MODE, &set_gmrp_fwd_all2_cmd);

   cli_install (ctree, CONFIG_MODE, &set_gmrp_ext_filtering_cmd);
   cli_install (ctree, CONFIG_MODE, &set_gmrp_ext_filtering2_cmd);
   cli_install (ctree, CONFIG_MODE, &set_gmrp_ext_filtering_br_cmd);
   cli_install (ctree, CONFIG_MODE, &set_gmrp_ext_filtering2_br_cmd);
   
}
