/* Copyright (C) 2003  All Rights Reserved.

   GVRP: GARP Vlan Registration Protocol

   This module defines the CLI interface to the GVRP Protocol 

   The following commands are implemented:

   EXEC_PRIV_MODE:

     * show gvrp configuration bridge BRIDGE_NAME
     * show gvrp timer IF_NAME
     * show gvrp statistics
     * show gvrp statistics IF_NAME

   CONFIG_MODE:

     * set gvrp {ENABLE | DISABLE} bridge BRIDGE_NAME
     * set port gvrp { ENABLE | DISABLE } IF_NAME
     * set gvrp dynamic-vlan-creation { ENABLE | DISABLE } bridge BRIDGE_NAME 
     * set gvrp registration  { NORMAL | FIXED | FORBIDDEN | RESTRICTED GROUP} IF_NAME 
     * set gvrp applicant state {NORMAL | ACTIVE } IF_NAME
     * set gvrp timer { JOIN | LEAVE | LEAVEALL } TIMER_VALUE IF_NAME
     * clear gvrp statistics all bridge BRIDGE_NAME
     * clear gvrp statistics all
     * clear gvrp statistics bridge BRIDGE_NAME
     * clear gvrp statistics IF_NAME
*/

#include "pal.h"
#include "lib.h"
#include "log.h"
#include "cli.h"
#include "table.h"
#include "avl_tree.h"

#include "nsm_interface.h"
#include "nsm_bridge.h"
#include "nsm_vlan.h"
#include "nsm_bridge_cli.h"
#include "nsm_vlan_cli.h"

#include "gvrp.h"
#include "garp.h"
#include "garp_gid.h"
#include "garp_gid_fsm.h"
#include "garp_pdu.h"
#include "gvrp_api.h"

#include "gvrp_cli.h"
#include "gvrp_debug.h"
#include "gvrp_database.h"

void
gvrp_port_activate (struct nsm_bridge_master *master, struct interface *ifp)
{
  struct nsm_if *zif = NULL;

  if (!ifp)
    return;

  zif = (struct nsm_if *)ifp->info;

  if (zif->gvrp_port_config == NULL)
    return;
 
  if (!master)
    return;
  
  if (zif->gvrp_port_config->enable_port == PAL_TRUE)
    {
      gvrp_enable_port (master, ifp);
    }
  else
    {
      gvrp_disable_port (master, ifp);
      return;
    }
}

int 
show_gvrp_bridge_configuration(struct cli *cli, char *reg_type,
                               char *bridgename)
{
  int ret;
  u_char flags;
  struct gvrp *gvrp;
  struct nsm_if *zif;
  u_int32_t applicant_state;
  struct avl_node *avl_port;
  struct nsm_bridge  *bridge;
  u_char is_gvrp_enabled = 0;
  u_int32_t registration_type;
  register struct interface *ifp;
  struct nsm_bridge_port  *br_port;
  struct nsm_master *nm = cli->vr->proto;
  pal_time_t timer_details[GARP_MAX_TIMERS];
  struct nsm_bridge_master *master = nsm_bridge_get_master (nm);

  if (! master)
    return NSM_BRIDGE_ERR_NOTFOUND;

  /* Check for existence of bridge */
  if ( !(bridge = nsm_lookup_bridge_by_name(master, bridgename)) )
    {
      cli_out(cli,"%% Bridge %s not found",bridgename);
      return NSM_BRIDGE_ERR_NOTFOUND;
    }

  /* If gvrp is not enabled on bridge return */
  if ( !(gvrp = (struct gvrp*)bridge->gvrp))  
    {
      cli_out(cli,"%% gvrp is not enabled on bridge %s",bridgename);
      return NSM_BRIDGE_ERR_GVRP_NOCONFIG; 
    }

  CHECK_XVRP_IS_RUNNING(bridge, reg_type);

#ifdef HAVE_MVRP
  if (gvrp->reg_type == REG_TYPE_MVRP)
    { 
      cli_out (cli, "Global MVRP Configuration for bridge %s:\n", bridgename);
      cli_out (cli, "Dynamic Vlan Creation: %s\n", gvrp->dynamic_vlan_enabled ?
               "Enabled" : "Disabled");
           
      cli_out (cli, "Port based MVRP Configuration:\n"); 
      cli_out (cli, "                                                      "
                    "   Timers(centiseconds)\n");
      cli_out (cli, "Port         MVRP Status    Registration     Applicant"
                    "   Join   Leave   LeaveAll\n");
    }
  else
#endif /* HAVE_MVRP */
    {
      cli_out (cli, "Global GVRP Configuration for bridge %s:\n", bridgename);
      cli_out (cli, "Dynamic Vlan Creation: %s\n", gvrp->dynamic_vlan_enabled ?
               "Enabled" : "Disabled");
           
      cli_out (cli, "Port based GVRP Configuration:\n"); 
      cli_out (cli, "                                                      "
                    "   Timers(centiseconds)\n");
      cli_out (cli, "Port         GVRP Status    Registration     Applicant"
                    "   Join   Leave   LeaveAll\n");
    }

  cli_out (cli, "--------------------------------------------------------------------------------\n");

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

      if (gvrp_get_port_details (master, ifp, &flags) != RESULT_OK)
        {
          if (GVRP_DEBUG (CLI))
            {
               zlog_info (gvrpm, "cannot get port details\n");
            }
          continue;
        }

      if (!(is_gvrp_enabled = flags & GVRP_MGMT_PORT_CONFIGURED))
        continue;

      ret = gvrp_get_applicant_state(master, ifp, &applicant_state);

      if ((ret != RESULT_OK) && GVRP_DEBUG (CLI))
        {
          zlog_info (gvrpm, "cannot get applicant state\n");
        }

      ret = gvrp_get_registration(master, ifp, &registration_type);

      if ((ret != RESULT_OK) && GVRP_DEBUG (CLI))
        {
          zlog_info (gvrpm, "cannot get registration type\n");
        }

      gvrp_get_timer_details(master, ifp, timer_details);
                
      cli_out (cli, "%-13s%-15s%-17s%-12s%-7d%-8d%-7d\n",
               ifp->name,
               is_gvrp_enabled ? "Enabled":"Disabled",
               (registration_type == GVRP_VLAN_REGISTRATION_FORBIDDEN ? "Forbidden" : 
               (registration_type == GVRP_VLAN_REGISTRATION_FIXED ? "Fixed" : "Normal")),
               (applicant_state==GVRP_APPLICANT_NORMAL ?  "Normal" :
               "Active"), timer_details[GARP_JOIN_TIMER],
                timer_details[GARP_LEAVE_CONF_TIMER], 
                timer_details[GARP_LEAVEALL_CONF_TIMER]);
    } /* end of listloop */

  return CLI_SUCCESS;
}

CLI (gvrp_debug_event,
     gvrp_debug_event_cmd,
     "debug gvrp event",
     "debug - enable debug output",
     "gvrp - GVRP commands",
     "event - echo events to console")
{
  if (GVRP_DEBUG (CLI))
    {
      zlog_info (gvrpm, "debug gvrp event");
    }

  DEBUG_ON(EVENT);

  return CLI_SUCCESS;
}


CLI (gvrp_debug_cli,
     gvrp_debug_cli_cmd,
     "debug gvrp cli",
     "debug - enable debug output",
     "gvrp - GVRP commands",
     "cli - echo commands to console")
{
  if (GVRP_DEBUG (CLI))
    {
      zlog_info (gvrpm, "debug gvrp cli");
    }

  DEBUG_ON(CLI);

  return CLI_SUCCESS;
}


CLI (gvrp_debug_timer,
     gvrp_debug_timer_cmd,
     "debug gvrp timer",
     "gvrp - GVRP commands",
     "debug - enable debug output",
     "timer - echo timer start to console")
{
  if (GVRP_DEBUG (CLI))
    {
      cli_out (cli, "debug gvrp timer");
    }

  DEBUG_ON(TIMER);

  return CLI_SUCCESS;
}


CLI (gvrp_debug_packet,
     gvrp_debug_packet_cmd,
     "debug gvrp packet",
     "debug - enable debug output",
     "gvrp - GVRP commands",
     "packet - echo packet contents to console")
{
  if (GVRP_DEBUG (CLI))
    {
      zlog_info (gvrpm, "debug gvrp packet");
    }

  DEBUG_ON(PACKET);

  return CLI_SUCCESS;
}


CLI (no_gvrp_debug_event,
     no_gvrp_debug_event_cmd,
     "no debug gvrp event",
     CLI_NO_STR,
     "debug - enable debug output",
     "gvrp - GVRP commands",
     "event - echo events to console")
{
  if (GVRP_DEBUG (CLI))
    {
      zlog_info (gvrpm, "no debug gvrp event");
    }

  DEBUG_ON(EVENT);

  return CLI_SUCCESS;
}


CLI (no_gvrp_debug_cli,
     no_gvrp_debug_cli_cmd,
     "no debug gvrp cli",
     CLI_NO_STR,
     "debug - disable debug output",
     "gvrp - GVRP commands",
     "cli - do not echo commands to console")
{
  if (GVRP_DEBUG (CLI))
    {
      cli_out (cli, "no debug gvrp cli");
    }

  DEBUG_OFF(CLI);

  return CLI_SUCCESS;
}


CLI (no_gvrp_debug_timer,
     no_gvrp_debug_timer_cmd,
     "no debug gvrp timer",
     CLI_NO_STR,
     "gvrp - GVRP commands",
     "debug - disable debug output",
     "timer - do not echo timer start to console")
{
  if (GVRP_DEBUG (CLI))
    {
      zlog_info (gvrpm, "no debug gvrp timer");
    }

  DEBUG_OFF(TIMER);

  return CLI_SUCCESS;
}


CLI (no_gvrp_debug_packet,
     no_gvrp_debug_packet_cmd,
     "no debug gvrp packet",
     CLI_NO_STR,
     "debug - disable debug output",
     "gvrp - GVRP commands",
     "packet - do not echo packet contents to console")
{
  if (GVRP_DEBUG (CLI))
    {
      zlog_info (gvrpm, "no debug gvrp packet");
    }

  DEBUG_OFF(PACKET);

  return CLI_SUCCESS;
}


CLI (gvrp_debug_all,
     gvrp_debug_all_cmd,
     "debug gvrp all",
     "debug - enable debug output",
     "gvrp - GVRP commands",
     "all - turn on all debugging")
{
  if (GVRP_DEBUG (CLI))
    {
      zlog_info (gvrpm, "debug gvrp all");
    }

  DEBUG_ON(EVENT);
  DEBUG_ON(CLI);
  DEBUG_ON(TIMER);
  DEBUG_ON(PACKET);

  return CLI_SUCCESS;
}


CLI (no_gvrp_debug_all,
     no_gvrp_debug_all_cmd,
     "no debug gvrp all",
     CLI_NO_STR,
     "debug - enable debug output",
     "gvrp - GVRP commands",
     "all - turn off all debugging")
{
  if (GVRP_DEBUG (CLI))
    {
      zlog_info (gvrpm, "no debug gvrp all");
    }

  DEBUG_OFF(EVENT);
  DEBUG_OFF(CLI);
  DEBUG_OFF(TIMER);
  DEBUG_OFF(PACKET);

  return CLI_SUCCESS;
}

CLI (show_debugging_gvrp,
     show_debugging_gvrp_cmd,
     "show debugging gvrp",
     CLI_SHOW_STR,
     CLI_DEBUG_STR,
     CLI_GVRP_STR)
{
  cli_out (cli, "GVRP debugging status:\n");

  if (GVRP_DEBUG (EVENT))
    cli_out (cli, "  GVRP Event debugging is on\n");
  if (GVRP_DEBUG (CLI))
    cli_out (cli, "  GVRP CLI debugging is on\n");
  if (GVRP_DEBUG (TIMER))
    cli_out (cli, "  GVRP Timer debugging is on\n");
  if (GVRP_DEBUG (PACKET))
    cli_out (cli, "  GVRP Packet debugging is on\n");
  cli_out (cli, "\n");

  return CLI_SUCCESS;
}

/* 
 * GARP Timer object 
 */

/* Read GARP Timers */
CLI (show_gvrp_timer,
     show_gvrp_timer_cmd,
     "show (gvrp|mvrp) timer IF_NAME",
     "CLI_SHOW_STR",
     "GARP Vlan Registration Protocol",
     "MRP Vlan Registration Protocol",
     "GARP timer",
     "Ifname")
{ /* 802.1D 14.9.1.1 */
  register struct interface *ifp;
  pal_time_t timer_details[GARP_MAX_TIMERS];
  struct nsm_master *nm = cli->vr->proto;
  struct nsm_bridge_master *master = nsm_bridge_get_master (nm);
  struct nsm_if *zif;

  if (GVRP_DEBUG (CLI))
    {
      zlog_info (gvrpm, "show %s timer %s", argv[0], argv[1]);
    }
 
  ifp = if_lookup_by_name(&cli->vr->ifm, argv[1]);

  if ((ifp == NULL)
      || (ifp->info == NULL))
    {
      cli_out (cli, "%% Interface not found \n");
      return CLI_ERROR;
    }

   zif = (struct nsm_if *)ifp->info;

   if (!zif)
     {
       cli_out (cli, "%% Interface not found\n");
       return CLI_ERROR;
     }
 
   if ((zif->bridge == NULL) || (zif->bridge->gvrp == NULL))
     {
       cli_out (cli, "%%Interface not bound to the bridge\n");
       return CLI_ERROR;
     }

   CHECK_XVRP_IS_RUNNING(zif->bridge, argv[0]);

   if (gvrp_get_timer_details (master, ifp, timer_details) != RESULT_OK)
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

  return CLI_SUCCESS;
}

void
gvrp_cli_process_result (struct cli *cli, int result,
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
      case NSM_BRIDGE_ERR_GVRP_NOCONFIG:
        bridge_name ? (cli_out (cli, 
                      "%% No %s configured on bridge %s\n", proto, bridge_name)):
                      (cli_out (cli, "%% No %s configured on bridge\n", proto));
        break;
      case NSM_ERR_GVRP_NOCONFIG_ONPORT:
        ifp ? (cli_out (cli, "%% No %s configured on the port %s\n",
               proto, ifp->name)):
             (cli_out (cli, "%% No %s configured on the port", proto));
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
#ifdef HAVE_PVLAN
      case NSM_PVLAN_ERR_CONFIGURED:
        cli_out (cli, "%% Private vlan is configured on the port\n");
        cli_out (cli, "%% %s cannot be enabled on the port \n", proto);
        break;
#endif /* HAVE_PVLAN */
      default:
        break;
    }
}

CLI (set_gvrp_timer1,
     set_gvrp_timer1_cmd,
     "set (gvrp|mvrp) timer join TIMER_VALUE IF_NAME",
     "Set GVRP join timer for the specified interface",
     "GARP Vlan Registration Protocol",
     "MRP Vlan Registration Protocol",
     "GARP timer",
     "Join Timer",
     "Timervalue in centiseconds",
     "Ifname")
{ 
  /* 802.1D 14.9.1.2 */
  register struct interface *ifp;
  pal_time_t timer_value;
  struct nsm_master *nm = cli->vr->proto;
  struct nsm_bridge_master *master = nsm_bridge_get_master (nm);
  struct nsm_if *zif = NULL;
  
  if (GVRP_DEBUG (CLI))
    {
      zlog_info (gvrpm, "set %s timer join %s %s", argv[0], argv[1], argv[2]);
    }

  CLI_GET_INTEGER_RANGE("garp_join_timer", timer_value, argv[1], 1, UINT32_MAX); 
  
  if ( !(ifp = if_lookup_by_name(&cli->vr->ifm, argv[2])) )
    {
      cli_out (cli, "%% Interface not found\n");
      return CLI_ERROR;
    }
  zif = (struct nsm_if *)ifp->info;
  if (!zif)
    {
      cli_out (cli, "%% Interface not found \n");
      return CLI_ERROR;
    }
   
  if ((zif->bridge == NULL) || (zif->bridge->gvrp == NULL))
    {
      GVRP_ALLOC_PORT_CONFIG (zif->gvrp_port_config,
                              zif->gvrp_port_config->timer_details[
                              GARP_JOIN_TIMER], timer_value, argv[2]);

      cli_out (cli, "%%Interface %s not bound to the bridge\n", argv[2]);
      return CLI_ERROR;
    }
  CHECK_XVRP_IS_RUNNING(zif->bridge, argv[0]);
  if (gvrp_check_timer_rules(master, ifp, GARP_JOIN_TIMER, 
            timer_value) != RESULT_OK)
    {
      cli_out(cli, "%% Timer does not meet timer relationship rules\n");
      return CLI_ERROR;
    }

  if (gvrp_set_timer (master, ifp,
                      GARP_JOIN_TIMER, timer_value) != RESULT_OK)
    {
      cli_out (cli, "%% Unable to set %s join timer info for %s \n", 
               argv[0], argv[2]);
      return CLI_ERROR;
    }

  return CLI_SUCCESS;
}

/* Set GARP Timers (Leave) */
CLI (set_gvrp_timer2,
     set_gvrp_timer2_cmd,
     "set (gvrp|mvrp) timer leave TIMER_VALUE IF_NAME",
     "Set GVRP leave timer for the specified interface",
     "GARP Vlan Registration Protocol",
     "MRP Vlan Registration Protocol",
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

  if (GVRP_DEBUG (CLI))
    {
      zlog_info (gvrpm, "set %s timer leave %s %s", argv[0], argv[1], argv[2]);
    }


  CLI_GET_INTEGER_RANGE("garp_leave_timer", timer_value, argv[1], 1,UINT32_MAX); 

  if ( !(ifp = if_lookup_by_name(&cli->vr->ifm, argv[2])) )
    {
      cli_out (cli, "%% Interface not found\n");
      return CLI_ERROR;
    }
  zif = (struct nsm_if *)ifp->info;
  if (!zif)
    {
      cli_out (cli, "%% Interface not found \n");
      return CLI_ERROR;
    }

  if ((zif->bridge == NULL) || (zif->bridge->gvrp == NULL))
    {
      GVRP_ALLOC_PORT_CONFIG (zif->gvrp_port_config,
                              zif->gvrp_port_config->timer_details[
                              GARP_LEAVE_TIMER], timer_value, argv[2]);
    }

  CHECK_XVRP_IS_RUNNING(zif->bridge, argv[0]); 

  if (gvrp_check_timer_rules(master, ifp, GARP_LEAVE_TIMER, 
     timer_value) != RESULT_OK) 
    {
      cli_out(cli, "%% Timer does not meet timer relationship rules\n");
      return CLI_ERROR;
    }

  if (gvrp_set_timer (master, ifp, 
                      GARP_LEAVE_TIMER, timer_value) != RESULT_OK)
    {
      cli_out (cli, "%% Unable to set %s leave timer info for %s \n",
               argv[0], argv[2]);
      return CLI_ERROR;
    }
  
  return CLI_SUCCESS;
}


/* Set GARP Timers (Leaveall) */
CLI (set_gvrp_timer3,
     set_gvrp_timer3_cmd,
     "set (gvrp|mvrp) timer leaveall TIMER_VALUE IF_NAME",
     "Set GVRP leaveall timer for the specified interface",
     "GARP Vlan Registration Protocol",
     "MRP Vlan Registration Protocol",
     "GARP timer",
     "Leaveall Timer",
     "Timervalue in centiseconds",
     "Ifname")
{ 
  /* 802.1D 14.9.1.2 */
  register struct interface *ifp;
  pal_time_t timer_value;
  struct nsm_master *nm = cli->vr->proto;
  struct nsm_bridge_master *master = nsm_bridge_get_master (nm);
  struct nsm_if *zif = NULL;
  
  if (GVRP_DEBUG (CLI))
    {
      zlog_info (gvrpm, "set %s timer leaveall %s %s", argv[1], argv[2]);
    }
  
  CLI_GET_INTEGER_RANGE("garp_leaveall_timer", timer_value, argv[1], 1, UINT32_MAX); 
  
  if ( !(ifp = if_lookup_by_name(&cli->vr->ifm, argv[2])) )
    {
      cli_out (cli, "%% Interface not found\n");
      return CLI_ERROR;
    }

  zif = (struct nsm_if *)ifp->info;
  if (!zif)
    {
      cli_out (cli, "%% Interface not found \n");
      return CLI_ERROR;
    }

  if ((zif->bridge == NULL) || (zif->bridge->gvrp == NULL))
    {
      GVRP_ALLOC_PORT_CONFIG (zif->gvrp_port_config,
                              zif->gvrp_port_config->timer_details[
                              GARP_LEAVE_ALL_TIMER], timer_value, argv[2]);
    }
 
  CHECK_XVRP_IS_RUNNING(zif->bridge, argv[0]);

  if (gvrp_check_timer_rules(master, ifp, GARP_LEAVE_ALL_TIMER,
   timer_value) != RESULT_OK) 
    {
      cli_out(cli, "%% Timer does not meet timer relationship rules\n");
      return CLI_ERROR;
    }

  if (gvrp_set_timer (master, ifp,
                      GARP_LEAVE_ALL_TIMER, timer_value) != RESULT_OK)
    {
      cli_out (cli, "%% Unable to set %s leaveall timer info for "
               "%s \n", argv[0], argv[2]);
      return CLI_ERROR;
    }
  
  return CLI_SUCCESS;
}


CLI (set_gvrp1,
     set_gvrp1_br_cmd,
     "set (gvrp|mvrp) enable bridge BRIDGE_NAME",
     "Enable GVRP globally for the bridge instance",
     "GARP Vlan Registration Protocol",
     "MRP Vlan Registration Protocol",
     "Enable",
     "Bridge instance ",
     "Bridge instance name")
{
  int ret;
  struct nsm_bridge *bridge;
  struct nsm_master *nm = cli->vr->proto;
  struct nsm_bridge_master *master = nsm_bridge_get_master (nm);
  
  if (GVRP_DEBUG (CLI))
    {
      if (argc == 2)
        zlog_info (gvrpm, "set %s enable bridge %s", argv[0], argv[1]);
      else
        zlog_info (gvrpm, "set %s enable", argv[0]);
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

  if (bridge->gvrp != NULL)
    CHECK_XVRP_IS_RUNNING(bridge, argv[0]);
 
  ret = gvrp_enable (master, argv[0], bridge->name);

  if ( ret < 0 )
    {
      gvrp_cli_process_result (cli, ret,0, bridge->name, argv[0]);
      return CLI_ERROR;
    }
  
  return CLI_SUCCESS;
}

ALI (set_gvrp1,
     set_gvrp1_cmd,
     "set (gvrp|mvrp) enable",
     "Enable GVRP globally for the bridge instance",
     "GARP Vlan Registration Protocol",
     "MRP Vlan Registration Protocol",
     "Enable");

CLI (set_gvrp2,
     set_gvrp2_br_cmd,
     "set (gvrp|mvrp) disable bridge BRIDGE_NAME",
     "Disable GVRP globally for the bridge instance",
     "GARP Vlan Registration Protocol",
     "MRP Vlan Registration Protocol",
     "Disable",
     "Bridge instance ",
     "Bridge instance name")
{
  int ret;
  struct nsm_bridge *bridge;
  struct nsm_master *nm = cli->vr->proto;
  struct nsm_bridge_master *master = nsm_bridge_get_master (nm);
  
  if (GVRP_DEBUG (CLI))
    {
      if (argc == 2)
        zlog_info (gvrpm, "set %s disable bridge %s", argv[0], argv[1]);
      else
        zlog_info (gvrpm, "set %s disable", argv[0]);
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

  if (bridge->gvrp != NULL)
    CHECK_XVRP_IS_RUNNING(bridge, argv[0]);
 
  ret = gvrp_disable (master, bridge->name);

  if ( ret <  0 )
    {
      gvrp_cli_process_result (cli, ret, 0, bridge->name, argv[0]);
      return CLI_ERROR;
    }
  
  return CLI_SUCCESS;
}

ALI (set_gvrp2,
     set_gvrp2_cmd,
     "set (gvrp|mvrp) disable",
     "Disable GVRP globally for the bridge instance",
     "GARP Vlan Registration Protocol",
     "MRP Vlan Registration Protocol",
     "Disable");

CLI (set_gvrp_dyn_vlan_enable,
     set_gvrp_br_dyn_vlan_enable_cmd,
     "set (gvrp|mvrp) dynamic-vlan-creation enable bridge BRIDGE_NAME",
     CLI_SET_STR,
     "GARP Vlan Registration Protocol",
     "MRP Vlan Registration Protocol",
     "Dynamic Vlan creation",
     "Enable",
     "Bridge instance ",
     "Bridge instance name")
{
  int ret;
  struct nsm_master *nm = cli->vr->proto;
  struct nsm_bridge_master *master = nsm_bridge_get_master (nm);
  struct nsm_bridge *bridge;

  if (argc == 2)
    bridge = nsm_lookup_bridge_by_name (master, argv [1]);
  else
    bridge = nsm_get_default_bridge (master);

  if (! bridge)
    {
      cli_out(cli,"Bridge not found \n");
      return CLI_ERROR;
    }

  if (bridge->gvrp == NULL)
    {
      cli_out (cli, "vlan registration protocol not enabled on bridge \n");
      return CLI_ERROR;
    }

  CHECK_XVRP_IS_RUNNING(bridge, argv[0]);

  ret = gvrp_dynamic_vlan_learning_set (master, bridge->name, PAL_TRUE);

  if ( ret <  0 )
    {
      gvrp_cli_process_result (cli, ret, 0, bridge->name, argv[0]);
      return CLI_ERROR;
    }
  
  cli_out(cli, "Dynamic VLAN creation enabled\n");
  return CLI_SUCCESS;
}

ALI (set_gvrp_dyn_vlan_enable,
     set_gvrp_dyn_vlan_enable_cmd,
     "set (gvrp|mvrp) dynamic-vlan-creation enable",
     CLI_SET_STR,
     "GARP Vlan Registration Protocol",
     "MRP Vlan Registration Protocol",
     "Dynamic Vlan creation",
     "Enable");

CLI (set_gvrp_dyn_vlan_disable,
     set_gvrp_br_dyn_vlan_disable_cmd,
     "set (gvrp|mvrp) dynamic-vlan-creation disable bridge BRIDGE_NAME",
     CLI_SET_STR,
     "GARP Vlan Registration Protocol",
     "MRP Vlan Registration Protocol",
     "Dynamic Vlan creation",
     "Disable",
     "Bridge instance ",
     "Bridge instance name")
{
  int ret;
  struct nsm_master *nm = cli->vr->proto;
  struct nsm_bridge_master *master = nsm_bridge_get_master (nm);
  struct nsm_bridge *bridge;

  if (argc == 2)
    bridge = nsm_lookup_bridge_by_name (master, argv [1]);
  else
    bridge = nsm_get_default_bridge (master);

  if (! bridge)
    {
      cli_out(cli,"Bridge not found \n");
      return CLI_ERROR;
    }

  if (bridge->gvrp == NULL)
    {
      cli_out (cli, "vlan registration protocol not enabled on bridge \n");
      return CLI_ERROR;
    }

  CHECK_XVRP_IS_RUNNING(bridge, argv[0]);
  
  ret = gvrp_dynamic_vlan_learning_set (master, bridge->name, PAL_FALSE);

  if ( ret <  0 )
    {
      gvrp_cli_process_result (cli, ret, 0, bridge->name, argv[0]);
      return CLI_ERROR;
    }
  
  cli_out(cli, "Dynamic VLAN creation disabled\n");
  return CLI_SUCCESS;
}

ALI (set_gvrp_dyn_vlan_disable,
     set_gvrp_dyn_vlan_disable_cmd,
     "set (gvrp|mvrp) dynamic-vlan-creation disable",
     CLI_SET_STR,
     "GARP Vlan Registration Protocol",
     "MRP Vlan Registration Protocol",
     "Dynamic Vlan creation",
     "Disable");

CLI (show_gvrp_configuration,
     show_gvrp_configuration_cmd,
     "show (gvrp|mvrp) configuration bridge BRIDGE_NAME",
     "CLI_SHOW_STR",
     "GARP Vlan Registration Protocol",
     "MRP Vlan Registration Protocol",
     "Display GVRP configuration",
     "Bridge instance ",
     "Bridge instance name")
{
  if (GVRP_DEBUG (CLI))
    {
      zlog_info (gvrpm, "show %s configuration bridge %s", argv[0], argv[1]);
    }
  
  show_gvrp_bridge_configuration(cli, argv[0], argv[1]);
  
  return CLI_SUCCESS;
}

CLI (show_gvrp_configuration_all,
     show_gvrp_configuration_all_cmd,
     "show (gvrp|mvrp) configuration",
     "CLI_SHOW_STR",
     "GARP Vlan Registration Protocol",
     "MRP Vlan Registration Protocol",
     "Display GVRP configuration")
{
  struct nsm_master *nm = cli->vr->proto;
  struct nsm_bridge_master *master = nsm_bridge_get_master (nm);
  struct nsm_bridge *bridge;
  
  if (GVRP_DEBUG (CLI))
    {
      zlog_info (gvrpm, "show %s configuration all", argv[0]);
    }
   
  for (bridge=nsm_get_first_bridge(master);
       bridge; bridge = nsm_get_next_bridge (master, bridge))
    {
      show_gvrp_bridge_configuration(cli, argv[0], bridge->name);
    }
  
  return CLI_SUCCESS;
}


CLI (set_port_gvrp1,
     set_port_gvrp1_cmd,
     "set port (gvrp|mvrp) enable (IF_NAME|all)",
     "Enable GVRP on port",
     "Layer2 Interface",
     "GARP Vlan Registration Protocol",
     "MRP Vlan Registration Protocol",
     "Enable",
     "Ifname",
     "All the ports")
{
  int ret = 0;
  register struct interface *ifp = NULL;
  struct nsm_master *nm = cli->vr->proto;
  struct nsm_bridge_master *master = nsm_bridge_get_master (nm);
  
  if (GVRP_DEBUG (CLI))
    {
      zlog_info (gvrpm, "set port %s enable %s", argv[0], argv[1]);
    }

  if (pal_strcmp (argv[1], "all") == 0)
    {
      ret = gvrp_enable_port_all (master, ifp);
    }

  else
    {
      if ( !(ifp = if_lookup_by_name (&cli->vr->ifm, argv[1])) )
        {
          cli_out (cli, "%% Interface not found\n");
          return CLI_ERROR;
        }
      ret = gvrp_enable_port (master, ifp);   
    }

  if ( ret <  0 )
    {
      gvrp_cli_process_result (cli, ret, ifp, 0, argv[0]);
      return CLI_ERROR;
    }
  
  return CLI_SUCCESS;
}


CLI (set_port_gvrp2,
     set_port_gvrp2_cmd,
     "set port (gvrp|mvrp) disable (IF_NAME|all)",
     "Disable GVRP on interface",
     "Layer2 Interface",
     "GARP Vlan Registration Protocol",
     "MRP Vlan Registration Protocol",
     "Disable",
     "Ifname",
     "All the ports")
{
  register struct interface *ifp = NULL;
  int ret = 0;
  struct nsm_master *nm = cli->vr->proto;
  struct nsm_bridge_master *master = nsm_bridge_get_master (nm);
  
  if (GVRP_DEBUG (CLI))
    {
      zlog_info (gvrpm, "set port %s disable %s", argv[0], argv[1]);
    }

  if (pal_strcmp (argv[1], "all") == 0)
    {
      ret = gvrp_disable_port_all (master, ifp);
    }
  else
    {
      if ( !(ifp = if_lookup_by_name (&cli->vr->ifm, argv[1])) )
        {
          cli_out (cli, "%% Interface not found\n");
          return CLI_ERROR;
        }
   
      ret = gvrp_disable_port (master, ifp);
    }

  if ( ret <  0 )
    {
      gvrp_cli_process_result (cli, ret, ifp, 0, argv[0]);
      return CLI_ERROR;
    }
  
  return CLI_SUCCESS;
}

CLI (set_gvrp_registration_normal,
     set_gvrp_registration_normal_cmd,
     "set (gvrp|mvrp) registration normal IF_NAME",
     "Set GVRP Registration to Normal Registration",
     "GARP Vlan Registration Protocol",
     "MRP Vlan Registration Protocol",
     "GVRP Registration",
     "Normal Registration",
     "Ifname")
{
  register struct interface *ifp;
  int ret;
  struct nsm_master *nm = cli->vr->proto;
  struct nsm_bridge_master *master = nsm_bridge_get_master (nm);
  struct nsm_if *zif;
   
  if (GVRP_DEBUG (CLI))
    {
      zlog_info (gvrpm, "set %s registration normal %s", argv[0], argv[1]);
    }
  
  if ( !(ifp = if_lookup_by_name (&cli->vr->ifm, argv[1])) )
    {
      cli_out (cli, "%% Interface %s not found\n", argv[1]);
      return CLI_ERROR;
    }
  
  zif = (struct nsm_if *)ifp->info;

  if (zif == NULL)
    {
      cli_out (cli, "%% Interface %s not found\n", argv[1]);
      return CLI_ERROR;
    }

  if ((zif->bridge == NULL) || (zif->bridge->gvrp == NULL))
    {
      GVRP_ALLOC_PORT_CONFIG (zif->gvrp_port_config,
                              zif->gvrp_port_config->registration_mode,
                              GVRP_VLAN_REGISTRATION_NORMAL, argv[1]);
    }

  CHECK_XVRP_IS_RUNNING(zif->bridge, argv[0]);
  
  ret = gvrp_set_registration (master, ifp, GVRP_VLAN_REGISTRATION_NORMAL);
  if ( ret <  0 )
    {
      gvrp_cli_process_result (cli, ret, ifp, 0, argv[0]);
      return CLI_ERROR;
    }
  
  return CLI_SUCCESS;
}


CLI (set_gvrp_registration_fixed,
     set_gvrp_registration_fixed_cmd,
     "set (gvrp|mvrp) registration fixed IF_NAME",
     "Set GVRP Registration to Fixed Registration",
     "GARP Vlan Registration Protocol",
     "MRP Vlan Registration Protocol",
     "GVRP Registration",
     "Fixed Registration",
     "Ifname")
{
  register struct interface *ifp;
  int ret;
  struct nsm_master *nm = cli->vr->proto;
  struct nsm_bridge_master *master = nsm_bridge_get_master (nm);
  struct nsm_if *zif = NULL;
  
  if (GVRP_DEBUG (CLI))
    {
      zlog_info (gvrpm, "set %s registration fixed %s", argv[0], argv[1]);
    }
  
  if ( !(ifp = if_lookup_by_name (&cli->vr->ifm, argv[1])) )
    {
      cli_out (cli, "%% Interface %s not found\n", argv[1]);
      return CLI_ERROR;
    }

  zif = (struct nsm_if *)ifp->info;
  if (!zif)
    {
      cli_out (cli, "%% Interface not found \n");
      return CLI_ERROR;
    }

  if ((zif->bridge == NULL) || (zif->bridge->gvrp == NULL))
    {
      GVRP_ALLOC_PORT_CONFIG (zif->gvrp_port_config,
                              zif->gvrp_port_config->registration_mode,
                              GVRP_VLAN_REGISTRATION_FIXED, argv[1]);

    }

  CHECK_XVRP_IS_RUNNING(zif->bridge, argv[0]);

  ret = gvrp_set_registration (master, ifp, GVRP_VLAN_REGISTRATION_FIXED);

  if ( ret <  0 )
    {
      gvrp_cli_process_result (cli, ret, ifp, 0, argv[0]);
      return CLI_ERROR;
    }
  
  return CLI_SUCCESS;
}


CLI (set_gvrp_registration_forbidden,
     set_gvrp_registration_forbidden_cmd,
     "set (gvrp|mvrp) registration forbidden IF_NAME",
     "Set GVRP Registration to Forbidden Registration",
     "GARP Vlan Registration Protocol",
     "MRP Vlan Registration Protocol",
     "GVRP Registration",
     "Forbidden Registration",
     "Interface Name")
{
  register struct interface *ifp;
  int ret;
  struct nsm_master *nm = cli->vr->proto;
  struct nsm_bridge_master *master = nsm_bridge_get_master (nm);
  struct nsm_if *zif = NULL;
  
  if (GVRP_DEBUG (CLI))
    {
      zlog_info (gvrpm, "set %s registration forbidden %s", argv[0], argv[1]);
    }
  
  if ( !(ifp = if_lookup_by_name (&cli->vr->ifm, argv[1])) )
    {
      cli_out (cli, "%% Interface %s not found\n", argv[1]);
      return CLI_ERROR;
    }
   
  zif = (struct nsm_if *)ifp->info;
  if (!zif)
    {
      cli_out (cli, "%% Interface not found \n");
      return CLI_ERROR;
    }
  
  if ((zif->bridge == NULL) || (zif->bridge->gvrp == NULL))
    {
      GVRP_ALLOC_PORT_CONFIG (zif->gvrp_port_config,
                              zif->gvrp_port_config->registration_mode,
                              GVRP_VLAN_REGISTRATION_FORBIDDEN, argv[1]);

    }

  CHECK_XVRP_IS_RUNNING(zif->bridge, argv[0]);

  ret = gvrp_set_registration (master, ifp, GVRP_VLAN_REGISTRATION_FORBIDDEN);

  if ( ret <  0 )
    {
      gvrp_cli_process_result (cli, ret, ifp, 0, argv[0]);
      return CLI_ERROR;
    }
  
  return CLI_SUCCESS;
}

CLI (set_gvrp_app_state_normal,
     set_gvrp_app_state_normal_cmd,
     "set (gvrp|mvrp) applicant state normal IF_NAME",
     CLI_SET_STR,
     "GARP Vlan Registration Protocol",
     "MRP Vlan Registration Protocol",
     "GID Applicant",
     "State of the Applicant",
     "Normal State",
     "Interface name")
{
  register struct interface *ifp;
  int ret;
  struct nsm_master *nm = cli->vr->proto;
  struct nsm_bridge_master *master = nsm_bridge_get_master (nm);
  struct nsm_if *zif;
  
  if (GVRP_DEBUG (CLI))
    {
      zlog_info (gvrpm, "set %s applicant normal %s", argv[0], argv[1]);
    }
  
  if ( !(ifp = if_lookup_by_name (&cli->vr->ifm, argv[1])) )
    {
      cli_out (cli, "%% Interface %s not found\n", argv[1]);
      return CLI_ERROR;
    }
  
  zif = (struct nsm_if *)ifp->info;

  if (zif == NULL)
    {
      cli_out (cli, "%% Interface not found \n");
      return CLI_ERROR;
    }

  if ((zif->bridge == NULL) || (zif->bridge->gvrp == NULL))
    {
      GVRP_ALLOC_PORT_CONFIG (zif->gvrp_port_config,
                              zif->gvrp_port_config->applicant_state,
                              GVRP_APPLICANT_NORMAL, argv[1]);

    }

  CHECK_XVRP_IS_RUNNING(zif->bridge, argv[0]);

  ret = gvrp_set_applicant_state (master, ifp, GVRP_APPLICANT_NORMAL);
  if ( ret <  0 )
    {
      gvrp_cli_process_result (cli, ret, ifp, 0, argv[0]);
      return CLI_ERROR;
    }
  
  cli_out (cli, "Applicant was set to normal on port %s \n", argv[1]);
  return CLI_SUCCESS;
}


CLI (set_gvrp_app_state_active,
     set_gvrp_app_state_active_cmd,
     "set (gvrp|mvrp) applicant state active IF_NAME",
     CLI_SET_STR,
     "GARP Vlan Registration Protocol",
     "MRP Vlan Registration Protocol",
     "GID Applicant",
     "State of the Applicant",
     "Active State",
     "Interface name")
{
  register struct interface *ifp;
  int ret;
  struct nsm_master *nm = cli->vr->proto;
  struct nsm_bridge_master *master = nsm_bridge_get_master (nm);
  struct nsm_if *zif = NULL;
  
  if (GVRP_DEBUG (CLI))
    {
      zlog_info (gvrpm, "set %s applicant active %s", argv[0], argv[1]);
    }
  
  if ( !(ifp = if_lookup_by_name (&cli->vr->ifm, argv[1])) )
    {
      cli_out (cli, "%% Interface %s not found\n", argv[1]);
      return CLI_ERROR;
    }
  
  zif = (struct nsm_if *)ifp->info;
  if (!zif)
    {
      cli_out (cli, "%% Interface not found \n");
      return CLI_ERROR;
    }

  if ((zif->bridge == NULL) || (zif->bridge->gvrp == NULL))
    {
      GVRP_ALLOC_PORT_CONFIG (zif->gvrp_port_config,
                              zif->gvrp_port_config->applicant_state,
                              GVRP_APPLICANT_ACTIVE, argv[1]);
    }

  CHECK_XVRP_IS_RUNNING(zif->bridge, argv[0]);
 
  ret = gvrp_set_applicant_state (master, ifp, GVRP_APPLICANT_ACTIVE);
  if ( ret <  0 )
    {
      gvrp_cli_process_result (cli, ret, ifp, 0, argv[0]);
      return CLI_ERROR;
    }
  
  cli_out (cli, "Applicant was set to active on port %s \n", argv[1]);
  return CLI_SUCCESS;
}

#if defined HAVE_MVRP
CLI (set_mvrp_point_to_point,
     set_mvrp_point_to_point_cmd,
     "set mvrp pointtopoint (enable|disable) interface IF_NAME",
     "set MVRP globally for the bridge",
     "MRP Vlan Registration Protocol",
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
  s_int32_t ret =0;

  if (pal_strncmp (argv[1], "e", 1) == 0)
    ctrl = MVRP_POINT_TO_POINT_ENABLE;
  else if (pal_strncmp (argv[1], "d", 1) == 0)
    ctrl = MVRP_POINT_TO_POINT_DISABLE;
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
      cli_out (cli, "%% Interface not found \n");
      return CLI_ERROR;
    }

  if ((zif->bridge == NULL) || (zif->bridge->gvrp == NULL))
    {
      GVRP_ALLOC_PORT_CONFIG (zif->gvrp_port_config,
                              zif->gvrp_port_config->p2p,
                              ctrl, argv[2]);
    }

  ret = mvrp_set_pointtopoint(master, ctrl, ifp);

  if (ret < 0)
    {
      gvrp_cli_process_result (cli, ret, ifp, 0, "mvrp");
      return CLI_ERROR;
    }

  return CLI_SUCCESS;
}
#endif /* HAVE_MVRP */

int
gvrp_show_port_statistics (struct cli *cli, struct interface *ifp)
{
  struct nsm_bridge_port *br_port;
  struct nsm_bridge *bridge;
  struct gvrp_port *gport;
  struct nsm_if *zif;
  struct gvrp *gvrp;
  
  if (ifp == NULL)
    return RESULT_ERROR;
  
  zif = (struct nsm_if *)ifp->info;

  if (zif == NULL || zif->switchport == NULL)
    return RESULT_ERROR;

  br_port = zif->switchport;

  if ( !(gport=br_port->gvrp_port) )
    return RESULT_ERROR;

  if ( !(bridge = zif->bridge) 
      || !(gvrp = bridge->gvrp))
    return RESULT_ERROR;

#ifdef HAVE_MVRP
  if (gvrp->reg_type == REG_TYPE_MVRP)
    {
      cli_out (cli, "MVRP Statistics for bridge %s \n", bridge->name);
      cli_out (cli, "---------------------------------------------------\n");

      cli_out (cli, "MVRP Events Received:\n");

      cli_out (cli, "New Ins:                          %d\n",
               gport->receive_counters[MRP_ATTR_EVENT_NEW_IN]);
      cli_out (cli, "Join Ins:                         %d\n",
               gport->receive_counters[MRP_ATTR_EVENT_JOIN_IN]);
      cli_out (cli, "Leave:                            %d\n",
               gport->receive_counters[MRP_ATTR_EVENT_LEAVE]);
      cli_out (cli, "Null:                             %d\n",
               gport->receive_counters[MRP_ATTR_EVENT_NULL]);
      cli_out (cli, "New Empties:                      %d\n",
               gport->receive_counters[MRP_ATTR_EVENT_NEW_EMPTY]);
      cli_out (cli, "Join Empties:                     %d\n",
               gport->receive_counters[MRP_ATTR_EVENT_JOIN_EMPTY]);
      cli_out (cli, "Empties:                          %d\n",
               gport->receive_counters[MRP_ATTR_EVENT_EMPTY]);
      cli_out (cli, "\n");

      cli_out (cli, "MVRP Events Transmitted:    \n");

      cli_out (cli, "New Ins:                          %d\n",
               gport->transmit_counters[MRP_ATTR_EVENT_NEW_IN]);
      cli_out (cli, "Join Ins:                         %d\n",
               gport->transmit_counters[MRP_ATTR_EVENT_JOIN_IN]);
      cli_out (cli, "Leave:                            %d\n",
               gport->transmit_counters[MRP_ATTR_EVENT_LEAVE]);
      cli_out (cli, "Null:                             %d\n",
               gport->transmit_counters[MRP_ATTR_EVENT_NULL]);
      cli_out (cli, "New Empties:                      %d\n",
               gport->transmit_counters[MRP_ATTR_EVENT_NEW_EMPTY]);
      cli_out (cli, "Join Empties:                     %d\n",
               gport->transmit_counters[MRP_ATTR_EVENT_JOIN_EMPTY]);
      cli_out (cli, "Empties:                          %d\n",
               gport->transmit_counters[MRP_ATTR_EVENT_EMPTY]);
      cli_out (cli, "\n");
    }
  else
#endif /* HAVE_MVRP */
    {
      cli_out (cli, "%-19s  RX  %10lu %10lu %10lu %10lu %10lu\n",
               ifp->name, gport->receive_counters[GARP_ATTR_EVENT_JOIN_EMPTY],
               gport->receive_counters[GARP_ATTR_EVENT_JOIN_IN],
               gport->receive_counters[GARP_ATTR_EVENT_LEAVE_EMPTY],
               gport->receive_counters[GARP_ATTR_EVENT_LEAVE_IN],
               gport->receive_counters[GARP_ATTR_EVENT_EMPTY]);
      cli_out (cli, "%-19s  TX  %10lu %10lu %10lu %10lu %10lu\n",
               " ", gport->transmit_counters[GARP_ATTR_EVENT_JOIN_EMPTY],
               gport->transmit_counters[GARP_ATTR_EVENT_JOIN_IN],
               gport->transmit_counters[GARP_ATTR_EVENT_LEAVE_EMPTY],
               gport->transmit_counters[GARP_ATTR_EVENT_LEAVE_IN],
               gport->transmit_counters[GARP_ATTR_EVENT_EMPTY]);
    }

  return RESULT_OK;
}

CLI (show_gvrp_statistics,
     show_gvrp_statistics_cmd,
     "show (gvrp|mvrp) statistics",
     CLI_SHOW_STR,
     "GARP VLAN Registration Protocol",
     "MRP Vlan Registration Protocol",
     "Statistics")
{
  struct gvrp *gvrp;
  struct nsm_if *zif;
  struct interface *ifp;
  struct avl_node *avl_port;
  struct nsm_bridge *bridge;
  struct nsm_bridge_port *br_port;
  struct nsm_master *nm = cli->vr->proto;
  struct nsm_bridge_master *master = nsm_bridge_get_master (nm);
  
  for (bridge = nsm_get_first_bridge(master);
       bridge; bridge = nsm_get_next_bridge (master, bridge))
    {

      if ((gvrp = bridge->gvrp) == NULL)
        continue;

      if (gvrp->reg_type == REG_TYPE_GVRP)
        {
          cli_out (cli, "Bridge: %s\n\n", bridge->name);

          cli_out (cli, "Port                     JoinEmpty    JoinIn   "
                   "LeaveEmpty  LeaveIn     Empty\n");

          cli_out (cli, "-----------------------------------------------"
                   "--------------------------------\n");
        }

      for (avl_port = avl_top (bridge->port_tree); avl_port;
           avl_port = avl_next (avl_port))
         {
           br_port = AVL_NODE_INFO (avl_port);

           if (br_port == NULL)
             continue;

           zif = br_port->nsmif;

           if (zif == NULL)
             continue;

           ifp = zif->ifp;

           if (ifp == NULL)
             continue;

           gvrp_show_port_statistics (cli, ifp);
         }
    }
  
  return CLI_SUCCESS;
}

CLI (show_gvrp_statistics_port,
     show_gvrp_statistics_port_cmd,
     "show (gvrp|mvrp) statistics IFNAME",
     CLI_SHOW_STR,
     "GARP VLAN Registration Protocol",
     "MRP Vlan Registration Protocol",
     "Statistics",
     "Port Name")
{
  int ret;
  struct gvrp *gvrp;
  struct interface *ifp;
  struct nsm_if *zif = NULL;
  struct nsm_bridge *bridge = NULL;
  
  ifp = if_lookup_by_name (&cli->vr->ifm, argv[1]);
  if (ifp == NULL)
    {
      cli_out (cli, "%% No such Port name\n");
      return CLI_ERROR;
    }
  if ( !(zif = (struct nsm_if *)ifp->info) ||
       !(bridge = zif->bridge) )
    {
      cli_out (cli, "%% Interface not bound to bridge\n");
      return CLI_ERROR;
    }

  if ((gvrp = bridge->gvrp) == NULL)
    {
      cli_out (cli, "%% vlan registrtation protocol is not enabled \n");
      return CLI_ERROR;
    }

  CHECK_XVRP_IS_RUNNING(zif->bridge, argv[0]);
  
  if (gvrp->reg_type == REG_TYPE_GVRP)
    {
      cli_out (cli, "Bridge: %s\n\n", bridge->name);
      cli_out (cli, "Port                     JoinEmpty    JoinIn   LeaveEmpty"
               "  LeaveIn     Empty\n");
      cli_out (cli, "---------------------------------------------------------"
               "----------------------\n");
    }
  
  ret = gvrp_show_port_statistics (cli, ifp);

  if (ret != RESULT_OK)
    {
      cli_out (cli, "%% Unable to show %s per port statistics for %s\n",
               argv[0], argv[1]);
      return CLI_ERROR;
    }
  
  return CLI_SUCCESS;
}

CLI (clear_gvrp_statistics_all,
     clear_gvrp_statistics_all_cmd,
     "clear (gvrp|mvrp) statistics all",
     CLI_CLEAR_STR,
     "GARP Vlan Registration Protocol",
     "MRP Vlan Registration Protocol",
     "GVRP Statistics",
     "All bridges")
{
  int ret;
  
  ret = gvrp_clear_all_statistics (cli);
  if (ret != RESULT_OK)
    {
      cli_out (cli, "%% Unable to clear all %s statistics\n", argv[0]);
      return CLI_ERROR;
    }
  
  return CLI_SUCCESS;
}

CLI (clear_gvrp_statistics_bridge,
     clear_gvrp_statistics_bridge_cmd,
     "clear (gvrp|mvrp) statistics bridge BRIDGE_NAME",
     CLI_CLEAR_STR,
     "GARP Vlan Registration Protocol",
     "MRP Vlan Registration Protocol",
     "GVRP Statistics",
     "Bridge instance",
     "Bridge instance name")
{
  int ret;
  struct nsm_master *nm = cli->vr->proto;
  struct nsm_bridge_master *master = nsm_bridge_get_master (nm);
  struct nsm_bridge *bridge;

  if (argc == 2)
    bridge = nsm_lookup_bridge_by_name (master, argv [1]);
  else
    bridge = nsm_get_default_bridge (master);

  if (! bridge)
    {
      cli_out(cli,"Bridge not found \n");
      return CLI_ERROR;
    }

  if (bridge->gvrp == NULL)
    {
      cli_out (cli, "vlan registration protocol not enabled on bridge \n");
      return CLI_ERROR;
    }

  CHECK_XVRP_IS_RUNNING(bridge, argv[0]);  

  ret = gvrp_clear_per_bridge_statistics (master, bridge->name);
  if (ret != RESULT_OK)
    {
      cli_out (cli, "%% Unable to clear %s statistics for bridge %s\n",
               argv[0], bridge->name);
      return CLI_ERROR;
    }
  
  return CLI_SUCCESS;
}

ALI (clear_gvrp_statistics_bridge,
     clear_gvrp_statistics_cmd,
     "clear (gvrp|mvrp) statistics",
     CLI_CLEAR_STR,
     "GARP Vlan Registration Protocol",
     "MRP Vlan Registration Protocol",
     "GVRP Statistics");

CLI (clear_gvrp_statistics_port,
     clear_gvrp_statistics_port_cmd,
     "clear (gvrp|mvrp) statistics IFNAME",
     CLI_CLEAR_STR,
     "GARP Vlan Registration Protocol",
     "MRP Vlan Registration Protocol",
     "GVRP Statistics",
     "Port Name")
{
  struct interface *ifp;
  int ret;
  struct nsm_master *nm = cli->vr->proto;
  struct nsm_bridge_master *master = nsm_bridge_get_master (nm);
  struct nsm_if *zif;
  
  ifp = if_lookup_by_name (&cli->vr->ifm, argv[1]);
  if ((ifp == NULL) || (ifp->info == NULL))
    {
      cli_out (cli, "%% No such port name\n");
      return CLI_ERROR;
    }

  zif = (struct nsm_if *)ifp->info;

  if (! zif->bridge)
    {
      cli_out(cli,"interface not bound to bridge \n");
      return CLI_ERROR;
    }

  if (zif->bridge->gvrp == NULL)
    {
      cli_out (cli, "vlan registration protocol not enabled on bridge \n");
      return CLI_ERROR;
    }

  CHECK_XVRP_IS_RUNNING(zif->bridge, argv[0]);
  
  ret = gvrp_clear_per_port_statistics (master, ifp);

  if (ret != RESULT_OK)
    {
      cli_out (cli, "%% Unable to clear %s statistics for port %s\n",
               argv[0], argv[1]);
      return CLI_ERROR;
    }
  
  return CLI_SUCCESS;
}

/* GVRP configuration write function */
int 
gvrp_config_write (struct cli *cli)
{ 
  char      *proto;
  int       write = 0;
  struct    gvrp *gvrp;
  struct    nsm_if *zif;
  struct    interface * ifp;
  struct    nsm_bridge *bridge;
  struct    avl_node *avl_port;
  struct    gvrp_port *gvrp_port = 0;
  struct    nsm_bridge_port *br_port;
  pal_time_t timer_details[GARP_MAX_TIMERS];
  struct    nsm_master *nm = cli->vr->proto;
  struct    nsm_bridge_master *master = nsm_bridge_get_master (nm);
  
  /* Debug command */
  if (GVRP_DEBUG(CLI)) 
    cli_out (cli, "debug gvrp cli\n"); 
  if (GVRP_DEBUG(PACKET)) 
    cli_out (cli, "debug gvrp packet\n"); 
  if (GVRP_DEBUG(TIMER)) 
    cli_out (cli, "debug gvrp timer\n"); 
  if (GVRP_DEBUG(EVENT)) 
    cli_out (cli, "debug gvrp event\n"); 
  write += 4;
  
  for (bridge = nsm_get_first_bridge(master); bridge;
       bridge = nsm_get_next_bridge(master, bridge))
    { 
      if ( ((gvrp = bridge->gvrp) != NULL) )
        {
          proto = GVRP_REG_TYPE_TO_STR (gvrp->reg_type);

          /* gvrp is globally enabled on bridge */
          cli_out (cli, "set %s enable %s\n", proto,
                   nsm_get_bridge_str (bridge));
          
          /* dymanic vlan learning enabled */
          if (gvrp->dynamic_vlan_enabled == PAL_TRUE)
            cli_out (cli,
                     "set %s dynamic-vlan-creation enable %s\n",
                     proto, nsm_get_bridge_str (bridge));
          else
            cli_out (cli,
                     "set %s dynamic-vlan-creation disable %s\n",
                     proto, nsm_get_bridge_str (bridge));
          
          /* Loop through the port list and check for need to add 
             static vlans */

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

              gvrp_port = br_port->gvrp_port;

              if (gvrp_port)
                {
                  gvrp_get_timer_details (master, ifp, timer_details);
                  
                  /* gvrp is enabled on on port */
                  cli_out (cli, "set port %s enable %s\n", proto, ifp->name);
                  
                  /* Join timer */
                  if (timer_details[GARP_JOIN_TIMER] != 
                      GID_DEFAULT_JOIN_TIME) 
                  cli_out (cli, "set %s timer join %d %s\n",
                           proto, timer_details[GARP_JOIN_TIMER], ifp->name); 
                  
                  /* Leave timer */
                  if (timer_details[GARP_LEAVE_CONF_TIMER] != 
                      GID_DEFAULT_LEAVE_TIME)
                    cli_out (cli, "set %s timer leave %d %s\n",
                             proto, timer_details[GARP_LEAVE_CONF_TIMER], 
                             ifp->name);
                  
                  /* Leaveall timer */
                  if (timer_details[GARP_LEAVEALL_CONF_TIMER] != 
                      GID_DEFAULT_LEAVE_ALL_TIME)
                    cli_out (cli, "set %s timer leaveall %d %s\n",
                             proto, timer_details[GARP_LEAVEALL_CONF_TIMER], 
                             ifp->name);
              
                  /* Registration  */
                  if (gvrp_port->registration_mode == 
                      GVRP_VLAN_REGISTRATION_NORMAL) 
                    cli_out (cli, "set %s registration normal %s\n",
                             proto, ifp->name);
                  else if (gvrp_port->registration_mode == 
                           GVRP_VLAN_REGISTRATION_FIXED)
                    cli_out (cli, "set %s registration fixed %s\n",
                             proto, ifp->name);
                  else
                    cli_out (cli, "set %s registration forbidden %s\n",
                             proto, ifp->name);
                 
                  /* Applicant state  */
                  if (gvrp_port->applicant_state == GVRP_APPLICANT_NORMAL)
                    cli_out (cli, "set %s applicant state normal %s\n",
                             proto, ifp->name);
                  else
                    cli_out (cli, "set %s applicant state active %s\n",
                             proto, ifp->name);
#ifdef HAVE_MVRP
                 if ((gvrp_port->gid_port != NULL)
                      && (gvrp_port->gid_port->p2p == PAL_TRUE))
                    cli_out (cli, "set mvrp pointtopoint enable interface %s\n",
                             ifp->name);
#endif /* HAVE_MVRP */
                }
            }
        }
    }

  return write;
}

CLI (show_gvrp_machine,
     show_gvrp_machine_br_cmd,
     "show (gvrp|mvrp) machine bridge BRIDGE_NAME",
     "CLI_SHOW_STR",
     "Generic Attribute Registration Protocol",
     "MRP Vlan Registration Protocol",
     "Finite State Machine",
     "Bridge",
     "Bridge name")
{
  struct gid *gid;
  struct gvrp *gvrp;
  struct garp *garp;
  struct nsm_if *zif;
  struct ptree_node *rn;
  struct interface *ifp;
  struct avl_node *avl_port;
  struct nsm_bridge *bridge;
  struct nsm_bridge_port *br_port;
  struct gid_machine *machine = 0;
  struct nsm_master *nm = cli->vr->proto;
  struct nsm_bridge_master *master = nsm_bridge_get_master (nm);
  
  if (argc == 2)
    bridge = nsm_lookup_bridge_by_name (master, argv [1]);
  else
    bridge = nsm_get_default_bridge (master);

  if (! bridge)
  {
    cli_out(cli,"Bridge not found \n");
    return CLI_ERROR;
  }
 
  if ( !(gvrp = bridge->gvrp) )
    {
      cli_out(cli, "%s is not enabled on bridge %s\n", argv[0], bridge->name); 
      return CLI_ERROR;
    }

  CHECK_XVRP_IS_RUNNING(bridge, argv[0]);

  garp = &gvrp->garp;

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

      gid = (struct gid *)garp->get_gid_func (br_port, NSM_VLAN_DEFAULT_VID,
                                              NSM_VLAN_DEFAULT_VID);

      if (gid)
        {
          cli_out(cli, "  port = %-6s", ifp->name);

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

ALI (show_gvrp_machine,
     show_gvrp_machine_cmd,
     "show (gvrp|mvrp) machine",
     "CLI_SHOW_STR",
     "Generic Attribute Registration Protocol",
     "MRP Vlan Registration Protocol",
     "Finite State Machine");

void
gvrp_cli_init (struct lib_globals *zg)
{
   struct cli_tree *ctree = zg->ctree;

   cli_install_config (ctree, GVRP_MODE, &gvrp_config_write);
  
   /* enable debug commands */
   cli_install (ctree, CONFIG_MODE, &gvrp_debug_event_cmd);
   cli_install (ctree, CONFIG_MODE, &gvrp_debug_cli_cmd);
   cli_install (ctree, CONFIG_MODE, &gvrp_debug_timer_cmd);
   cli_install (ctree, CONFIG_MODE, &gvrp_debug_packet_cmd);
   cli_install (ctree, CONFIG_MODE, &gvrp_debug_all_cmd);

   /* disable debug commands */
   cli_install (ctree, CONFIG_MODE, &no_gvrp_debug_event_cmd);
   cli_install (ctree, CONFIG_MODE, &no_gvrp_debug_cli_cmd);
   cli_install (ctree, CONFIG_MODE, &no_gvrp_debug_timer_cmd);
   cli_install (ctree, CONFIG_MODE, &no_gvrp_debug_packet_cmd);
   cli_install (ctree, CONFIG_MODE, &no_gvrp_debug_all_cmd);

   /* display commands */
   cli_install (ctree, EXEC_MODE, &show_gvrp_timer_cmd);
   cli_install (ctree, EXEC_MODE, &show_gvrp_statistics_cmd);
   cli_install (ctree, EXEC_MODE, &show_gvrp_statistics_port_cmd);
   cli_install (ctree, EXEC_MODE, &show_gvrp_configuration_cmd);
   cli_install (ctree, EXEC_MODE, &show_gvrp_configuration_all_cmd);
   cli_install (ctree, EXEC_MODE, &show_gvrp_machine_cmd);
   cli_install (ctree, EXEC_MODE, &show_gvrp_machine_br_cmd);
   cli_install (ctree, EXEC_MODE, &show_debugging_gvrp_cmd);

   /* "timers" commands. */
   cli_install (ctree, CONFIG_MODE, &set_gvrp_timer1_cmd);
   cli_install (ctree, CONFIG_MODE, &set_gvrp_timer2_cmd);
   cli_install (ctree, CONFIG_MODE, &set_gvrp_timer3_cmd);

   /* global GVRP enable/disable commands */
   cli_install (ctree, CONFIG_MODE, &set_gvrp1_cmd);
   cli_install (ctree, CONFIG_MODE, &set_gvrp2_cmd);
   cli_install (ctree, CONFIG_MODE, &set_gvrp1_br_cmd);
   cli_install (ctree, CONFIG_MODE, &set_gvrp2_br_cmd);
 
   /* Dynamic vlan learning commands */
   cli_install (ctree, CONFIG_MODE, &set_gvrp_dyn_vlan_enable_cmd);
   cli_install (ctree, CONFIG_MODE, &set_gvrp_dyn_vlan_disable_cmd);
   cli_install (ctree, CONFIG_MODE, &set_gvrp_br_dyn_vlan_enable_cmd);
   cli_install (ctree, CONFIG_MODE, &set_gvrp_br_dyn_vlan_disable_cmd);

   /* Applicant state commands */
   cli_install (ctree, CONFIG_MODE, &set_gvrp_app_state_normal_cmd);
   cli_install (ctree, CONFIG_MODE, &set_gvrp_app_state_active_cmd);

   /* poer port GVRP enable/disable commands */
   cli_install (ctree, CONFIG_MODE, &set_port_gvrp1_cmd);
   cli_install (ctree, CONFIG_MODE, &set_port_gvrp2_cmd);

   /* per port GVRP registration (normal, fixed, forbidden) commands */
   cli_install (ctree, CONFIG_MODE, &set_gvrp_registration_normal_cmd);
   cli_install (ctree, CONFIG_MODE, &set_gvrp_registration_fixed_cmd);
   cli_install (ctree, CONFIG_MODE, &set_gvrp_registration_forbidden_cmd);
#if defined HAVE_MVRP
   /* point to point control per interface */
   cli_install (ctree, CONFIG_MODE, &set_mvrp_point_to_point_cmd);
#endif /* HAVE_MVRP */

   /* statistics clear commands */
   cli_install (ctree, EXEC_MODE, &clear_gvrp_statistics_all_cmd);
   cli_install (ctree, EXEC_MODE, &clear_gvrp_statistics_bridge_cmd);
   cli_install (ctree, EXEC_MODE, &clear_gvrp_statistics_cmd);
   cli_install (ctree, EXEC_MODE, &clear_gvrp_statistics_port_cmd);
}
