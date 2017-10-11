/**@file elmi_cli.c
* @brief  This file contains the CLIs for ELMI protocol.
**/

/* Copyright (C) 2010  All Rights Reserved. */

#include "pal.h"
#include "cli.h"
#include "lib.h"
#include "avl_tree.h"
#include "ptree.h"
#include "lib/L2/l2_timer.h"
#include "lib/L2/l2lib.h"
#include "lib/avl_tree.h"
#include "elmid.h"
#include "elmi_cli.h"
#include "elmi_api.h"
#include "elmi_error.h"
#include "elmi_types.h"
#include "elmi_debug.h"

s_int32_t
elmi_cli_return (struct cli *cli, s_int32_t ret)
{
  s_int32_t result = CLI_SUCCESS;

  if (ret < 0)
    {
      switch(ret)
        {
        case ELMI_ERR_BRIDGE_NOT_FOUND:
          cli_out(cli, "%% Interface is not bound to any bridge\n");
          break;
        case ELMI_ERR_PORT_NOT_FOUND:
          cli_out(cli, "%% Port not found\n");
          break;
        case ELMI_ERR_INTERVAL_OUTOFBOUNDS:
          cli_out(cli, "%% Given time interval val is out of range\n");
          break;
        case ELMI_ERR_WRONG_PORT_TYPE:
          cli_out(cli, "%% ELMI can not be enabled on this port\n");
          break;
        case ELMI_ERR_PORT_ELMI_ENABLED:
          cli_out(cli, "%% ELMI is already enabled on this interface\n");
          break;
        case ELMI_ERR_ELMI_NOT_ENABLED:
          cli_out(cli, "%% ELMI is not enabled\n");
          break;
        case ELMI_ERR_MEMORY:
          cli_out(cli, "%% Error in memory allocation\n");
          break;
        case ELMI_ERR_MIS_CONFIGURATION:
          cli_out(cli, "%% ELMI Configuration Error\n");
          break;
        case ELMI_ERR_NOT_ALLOWED_UNIC:
          cli_out(cli, "%% This command is not applicable on UNI-C\n");
          break;
        case ELMI_ERR_NOT_ALLOWED_UNIN:
          cli_out(cli, "%% This command is not applicable on UNI-N\n");
          break;
        case ELMI_ERR_WRONG_BRIDGE_TYPE:
          cli_out(cli, "%% ELMI can not be enabled on this bridge\n");
          break;
        case ELMI_ERR_GENERIC:
          cli_out(cli, "%% Configuration error\n");
          break;
        case ELMI_ERR_INVALID_INTERFACE:
          cli_out(cli, "%% Invalid Interface\n");
          break;
        case ELMI_ERR_ALREADY_ELMI_ENABLED:
          cli_out(cli, "%% ELMI is already enabled\n");
          break;
        default :
          cli_out(cli, "%% Error in ELMI Configuration\n");
          break;
        }
      result = CLI_ERROR;
    }

  return result;
}

CLI (elmi_enable_global,
  elmi_enable_global_cmd,
  "ethernet lmi global (bridge <1-32>|)",
  "Ethernet LMI",
  "Local Management Interface",
  "Global Configuration at bridge level", 
  BRIDGE_STR,
  BRIDGE_NAME_STR)
{
  s_int32_t ret = 0;
  struct elmi_bridge *bridge = NULL;

  if (argc == 2) 
    bridge = elmi_find_bridge (argv[1]); 
  else
    bridge = elmi_find_bridge (DEFAULT_BRIDGE); 

  if (!bridge)
    {
      cli_out (cli, "%% Bridge not found\n");
      return CLI_ERROR;
    }

  ret = elmi_api_enable_bridge_global (bridge->name);

  return elmi_cli_return  (cli, ret);

}

CLI (no_elmi_enable_global,
  no_elmi_enable_global_cmd,
  "no ethernet lmi global (bridge <1-32>|)",
  CLI_NO_STR,
  "Ethernet LMI",
  "Local Management Interface",
  "Global Configuration at bridge level", 
  BRIDGE_STR,
  BRIDGE_NAME_STR)
{
  s_int32_t ret = 0;
  struct elmi_bridge *bridge = NULL;

  if (argc == 2)
    bridge = elmi_find_bridge (argv[1]);
  else
    bridge = elmi_find_bridge (DEFAULT_BRIDGE);

  if (!bridge)
    {
      cli_out (cli, "%% Bridge not found\n");
      return CLI_ERROR;
    }

  ret = elmi_api_disable_bridge_global (bridge->name);

  return elmi_cli_return  (cli, ret);
}

CLI (elmi_enable_interface,
  elmi_enable_interface_cmd,
  "ethernet lmi interface",
  "Ethernet",
  "Local Management Interface",
  "enable ethernet lmi on port")
{
  s_int32_t ret = RESULT_ERROR;
  struct interface *ifp = cli->index;

  if ((ifp == NULL) || (ifp->info == NULL))
    {
      cli_out (cli, "%% Can't configure ELMI on this interface\n");
      return CLI_ERROR;
    }

  ret = elmi_api_enable_port (ifp->ifindex);

  return elmi_cli_return (cli, ret);

}

CLI (no_elmi_enable_interface,
  no_elmi_enable_interface_cmd,
  "no ethernet lmi interface",
  CLI_NO_STR,
  "Ethernet LMI",
  "Local Management Interface",
  "disable ethernet lmi on interface")
{
  s_int32_t ret = 0;
  struct interface *ifp = cli->index;
  struct elmi_ifp *elmi_if = NULL;
  struct elmi_port *port = NULL;

  if ((ifp == NULL) || (ifp->info == NULL))
    {
      cli_out (cli, "%% Can't configure ELMI on this interface\n");
      return CLI_ERROR;
    }

  elmi_if = ifp->info;

  if (!elmi_if)
    return CLI_ERROR;

  port = elmi_if->port; 

  if (elmi_if->elmi_enabled != PAL_TRUE)
    { 
      cli_out (cli, "%% ELMI is not enabled on this interface\n");
      return CLI_SUCCESS;
    }

  ret = elmi_api_disable_port (ifp->ifindex);

  return elmi_cli_return (cli, ret);
}


CLI (elmi_set_polling_time,
  elmi_set_polling_time_cmd,
  "ethernet lmi t391 <5-30>",
  "Ethernet",
  "Local Management Interface",
  "polling-time - Polling Time Interval(T391) for UNI-C",
  "Value in seconds in range 5 to 30, default is 10 seconds")
{
  s_int32_t ret = 0;
  struct interface *ifp = cli->index;
  u_int16_t polling_time;

  CLI_GET_INTEGER_RANGE ("polling-time", polling_time, argv[0], 
      ELMI_T391_POLLING_TIME_MIN,
      ELMI_T391_POLLING_TIME_MAX);

  if ((ifp == NULL) || (ifp->info == NULL))
    {
      cli_out (cli, "%% Can't configure ELMI on this interface\n");
      return CLI_ERROR;
    }

  ret = elmi_api_set_port_polling_time (ifp->name, polling_time);
  return elmi_cli_return (cli, ret);
}

CLI (no_elmi_set_polling_time,
  no_elmi_set_polling_time_cmd,
  "no ethernet lmi t391",
  "Ethernet",
  "Local Management Interface",
  "polling-time - Polling Time Interval(T391) for UNI-C")
{
  s_int32_t ret = 0;
  struct interface *ifp = cli->index;

  if ((ifp == NULL) || (ifp->info == NULL))
    {
      cli_out (cli, "%% Can't configure ELMI on this interface\n");
      return CLI_ERROR;
    }

  ret = elmi_api_set_port_polling_time (ifp->name,
      ELMI_T391_POLLING_TIME_DEF);
  return elmi_cli_return (cli, ret);
}


CLI (elmi_port_set_polling_verification_time,
  elmi_port_set_polling_verification_time_cmd,
  "ethernet lmi t392 <0-30>",
  "Ethernet",
  "Local Management Interface",
  "Polling Verification Time for PVT expiry at UNI-N",
  "Interval for PVT expiry in secs, Default:15, Range: 5-30, " 
  "0 is for disabling PVT")
{
  s_int8_t polling_verification_time = 0;
  struct interface *ifp = NULL;
  s_int32_t ret = RESULT_ERROR;

   /* zero is allowed to disable PVT, API will validate the range */
  CLI_GET_INTEGER_RANGE ("Polling Verification Time Interval",
                         polling_verification_time, argv [0],
                         0,
                         ELMI_T392_PVT_MAX);

  ifp = (struct interface *)cli->index;

  if (!ifp)
    {
      cli_out (cli, "%% Can't configure on this interface\n");
      return CLI_ERROR;
    }

  ret = elmi_api_set_port_polling_verification_time (ifp->name,
                                                    polling_verification_time);

  return elmi_cli_return (cli, ret);
}

CLI (no_elmi_port_set_polling_verification_time,
  no_elmi_port_set_polling_verification_time_cmd,
  "no ethernet lmi t392",
  CLI_NO_STR,
  "Ethernet",
  "Local Management Interface",
  "Polling Verification Time for PVT expiry at UNI-N")

{
  struct interface *ifp = NULL;
  s_int32_t ret = RESULT_ERROR;

  ifp = (struct interface *) cli->index;

  if (! ifp)
    {
      cli_out (cli, "%% Can't configure on this interface\n");
      return CLI_ERROR;
    }

  ret = elmi_api_set_port_polling_verification_time (ifp->name,
                                                     ELMI_T392_DEFAULT);

  return elmi_cli_return (cli, ret);

}

CLI (elmi_port_set_status_counter,
  elmi_port_set_status_counter_cmd,
  "ethernet lmi n393 <2-10>",
  "Ethernet",
  "Local Management Interface Protocol",
  "Status Counter - Count of consecutive errors for UNI-C and UNI-N",
  "Integer value for Status Counter, default is 4, range <2-10>")
{
  u_int8_t status_counter = 0;
  struct interface *ifp = NULL;
  s_int32_t ret = RESULT_ERROR;

  CLI_GET_UINT32_RANGE ("Status Counter",
      status_counter, argv [0],
      ELMI_N393_STATUS_COUNTER_MIN,
      ELMI_N393_STATUS_COUNTER_MAX);

  ifp = (struct interface *) cli->index;

  if (!ifp)
    {
      cli_out (cli, "%% Can't configure ELMI on this interface\n");
      return CLI_ERROR;
    }

  ret = elmi_api_set_port_status_counter (ifp->name,
                                          status_counter);

  return elmi_cli_return (cli, ret);

}

CLI (no_elmi_port_set_status_counter,
  no_elmi_port_set_status_counter_cmd,
  "no ethernet lmi n393",
  CLI_NO_STR,
  "Ethernet",
  "Local Management Interface Protocol",
  "Status Counter - Count of consecutive errors for UNI-C and UNI-N")
{

  struct interface *ifp = NULL;
  s_int32_t ret = RESULT_ERROR;

  ifp = (struct interface *) cli->index;

  if (! ifp)
    {
      cli_out (cli, "%% Can't configure ELMI on this interface\n");
      return CLI_ERROR;
    }

  ret = elmi_api_set_port_status_counter (ifp->name,
                                          ELMI_N393_STATUS_COUNTER_DEF);

  return elmi_cli_return (cli, ret);
}

CLI (elmi_port_set_polling_counter,
  elmi_port_set_polling_counter_cmd,
  "ethernet lmi n391 <1-65000>",
  "Ethernet",
  "Local Management Interface Protocol",
  "Polling Counter for full status message, configurable at UNI-C",
  "Integer value for Polling Counter, default is 360, range <1-65k>")
{
  u_int16_t polling_counter = 0;
  struct interface *ifp = NULL;
  s_int32_t ret = RESULT_ERROR;

  CLI_GET_UINT32_RANGE ("Polling Counter",
                        polling_counter, argv [0],
                        ELMI_N391_POLLING_COUNTER_MIN,
                        ELMI_N391_POLLING_COUNTER_MAX);

  ifp = (struct interface *) cli->index;

  if (! ifp)
    {
      cli_out (cli, "%% Can't configure ELMI on this interface\n");
      return CLI_ERROR;
    }

  ret = elmi_api_set_port_polling_counter (ifp->name,
                                           polling_counter);

  return elmi_cli_return (cli, ret);

}

CLI (no_elmi_port_set_polling_counter,
  no_elmi_port_set_polling_counter_cmd,
  "no ethernet lmi n391",
  CLI_NO_STR,
  "Ethernet",
  "Local Management Interface Protocol",
  "Polling Counter - Count of consecutive errors for UNI-C and UNI-N")
{
  struct interface *ifp = NULL;
  s_int32_t ret = RESULT_ERROR;

  ifp = (struct interface *) cli->index;

  if (! ifp)
    {
      cli_out (cli, "%% Can't configure ELMI on this interface\n");
      return CLI_ERROR;
    }

  ret = elmi_api_set_port_polling_counter (ifp->name,
                                           ELMI_N391_POLLING_COUNTER_DEF);

  return elmi_cli_return (cli, ret);

}

CLI (elmi_port_set_async_min_time,
  elmi_port_set_async_min_time_cmd,
  "ethernet lmi async-msg-interval <1-3>",
  "Ethernet",
  "Local Management Interface",
  "Minimum interval between asynchronous messages",
  "Interval in seconds, should be greater than 1/10th of t391")
{
  u_int8_t async_min_time = 0;
  struct interface *ifp = NULL;
  s_int32_t ret = RESULT_ERROR;

  CLI_GET_UINT32_RANGE ("Async Min Time Interval",
                        async_min_time, argv [0],
                        ELMI_ASYNC_TIME_INTERVAL_MIN,
                        ELMI_ASYNC_TIME_INTERVAL_MAX);

  ifp = (struct interface *) cli->index;

  if (! ifp)
    {
      cli_out (cli, "%% Can't configure ELMI on this interface\n");
      return CLI_ERROR;
    }

  ret = elmi_api_set_port_async_min_time (ifp->name,
                                          async_min_time);

  return elmi_cli_return (cli, ret);

}


CLI (no_elmi_port_set_async_min_time,
  no_elmi_port_set_async_min_time_cmd,
  "no ethernet lmi async-msg-interval",
  CLI_NO_STR,
  "Ethernet",
  "Local Management Interface",
  "Minimum Interval between Async messages, greater than 1/10th of t391")
{
  struct interface *ifp = NULL;
  s_int32_t ret = RESULT_ERROR;

  ifp = (struct interface *) cli->index;

  if (! ifp)
    {
      cli_out (cli, "%% Can't configure ELMI on this interface\n");
      return CLI_ERROR;
    }

  ret = elmi_api_set_port_async_min_time (ifp->name,
                                          ELMI_ASYNC_TIME_INTERVAL_DEF);

  return elmi_cli_return (cli, ret);

}

void
elmi_debug_all_on (struct cli *cli)
{

  struct elmi_master *em = cli->vr->proto;

  if (cli->mode == CONFIG_MODE)
    {
      ELMI_CONF_DEBUG_ON (event, EVENT);
      ELMI_CONF_DEBUG_ON (packet, PACKET);
      ELMI_CONF_DEBUG_ON (protocol, PROTOCOL);
      ELMI_CONF_DEBUG_ON (timer, TIMER);
      ELMI_CONF_DEBUG_ON (packet, TX);
      ELMI_CONF_DEBUG_ON (packet, RX);
    }
  else
    {
      ELMI_TERM_DEBUG_ON (event, EVENT);
      ELMI_TERM_DEBUG_ON (packet, PACKET);
      ELMI_TERM_DEBUG_ON (protocol, PROTOCOL);
      ELMI_TERM_DEBUG_ON (timer, TIMER);
      ELMI_TERM_DEBUG_ON (packet, TX);
      ELMI_TERM_DEBUG_ON (packet, RX);
    }

  elmi_nsm_debug_set (em);
  return;
}

void
elmi_debug_all_off (struct cli *cli)
{
  struct elmi_master *em = cli->vr->proto;

  if (cli->mode == CONFIG_MODE)
    {
      ELMI_CONF_DEBUG_OFF (event, EVENT);
      ELMI_CONF_DEBUG_OFF (packet, PACKET);
      ELMI_CONF_DEBUG_OFF (packet, RX);
      ELMI_CONF_DEBUG_OFF (packet, TX);
      ELMI_CONF_DEBUG_OFF (protocol, PROTOCOL);
      ELMI_CONF_DEBUG_OFF (timer, TIMER);
    }
  else
    {  
      ELMI_TERM_DEBUG_OFF (event, EVENT);
      ELMI_TERM_DEBUG_OFF (packet, PACKET);
      ELMI_TERM_DEBUG_OFF (packet, RX);
      ELMI_TERM_DEBUG_OFF (packet, TX);
      ELMI_TERM_DEBUG_OFF (protocol, PROTOCOL);
      ELMI_TERM_DEBUG_OFF (timer, TIMER);
    }

  elmi_nsm_debug_unset (em);
  return;
}

CLI (debug_elmi_event,
  debug_elmi_event_cmd,
  "debug elmi event",
  CLI_DEBUG_STR,
  CLI_ELMI_STR,
  "enable elmi event debugging")
{
  struct elmi_master *em = cli->vr->proto;

  ELMI_DEBUG_ON (cli, event, EVENT, "ELMI event");
  return CLI_SUCCESS;
}

CLI (no_debug_elmi_event,
  no_debug_elmi_event_cmd,
  "no debug elmi event",
  CLI_DEBUG_STR,
  CLI_ELMI_STR,
  "disable elmi event debugging")
{
  struct elmi_master *em = cli->vr->proto;

  ELMI_DEBUG_OFF (cli, event, EVENT, "ELMI event");
  return CLI_SUCCESS;
}



CLI (debug_elmi_proto,
  debug_elmi_proto_cmd,
  "debug elmi protocol",
  CLI_DEBUG_STR,
  CLI_ELMI_STR,
  "enable elmi protocol debugging")
{
  struct elmi_master *em = cli->vr->proto;

  ELMI_DEBUG_ON (cli, protocol, PROTOCOL, "ELMI protocol");
  return CLI_SUCCESS;
}

CLI (debug_elmi_timer,
  debug_elmi_timer_cmd,
  "debug elmi timer",
  CLI_DEBUG_STR,
  CLI_ELMI_STR,
  "ELMI Timers")
{
  struct elmi_master *em = cli->vr->proto;

  ELMI_DEBUG_ON (cli, timer, TIMER, "ELMI timer");
  return CLI_SUCCESS;
}

CLI (debug_elmi_packet_tx,
  debug_elmi_packet_tx_cmd,
  "debug elmi packet tx",
  CLI_DEBUG_STR,
  CLI_ELMI_STR,
  "ELMI Packets",
  "Display ELMI Transmit packets")
{
  struct elmi_master *em = cli->vr->proto;

  ELMI_DEBUG_ON (cli, packet, TX, "ELMI packet tx");
  return CLI_SUCCESS;
}

CLI (debug_elmi_packet_rx,
  debug_elmi_packet_rx_cmd,
  "debug elmi packet rx",
  CLI_DEBUG_STR,
  CLI_ELMI_STR,
  "ELMI Packets",
  "Display ELMI Receive Packets")
{
  struct elmi_master *em = cli->vr->proto;

  ELMI_DEBUG_ON (cli, packet, RX, "ELMI packet rx");
  return CLI_SUCCESS;
}


CLI (no_debug_elmi_packet_tx,
  no_debug_elmi_packet_tx_cmd,
  "no debug elmi packet tx",
  CLI_NO_STR,
  CLI_DEBUG_STR,
  CLI_ELMI_STR,
  "ELMI Packets",
  "Transmit")
{
  struct elmi_master *em = cli->vr->proto;

  ELMI_DEBUG_OFF (cli, packet, TX, "ELMI packet tx");
  return CLI_SUCCESS;
}

CLI (no_debug_elmi_packet_rx,
  no_debug_elmi_packet_rx_cmd,
  "no debug elmi packet rx",
  CLI_NO_STR,
  CLI_DEBUG_STR,
  CLI_ELMI_STR,
  "ELMI Packets",
  "Receive")
{
  struct elmi_master *em = cli->vr->proto;

  ELMI_DEBUG_OFF (cli, packet, RX, "ELMI packet rx");
  return CLI_SUCCESS;
}


CLI (no_debug_elmi_proto,
  no_debug_elmi_proto_cmd,
  "no debug elmi protocol",
  CLI_NO_STR,
  CLI_DEBUG_STR,
  CLI_ELMI_STR,
  "ELMI Protocol")
{
  struct elmi_master *em = cli->vr->proto;

  ELMI_DEBUG_OFF (cli, protocol, PROTOCOL, "ELMI protocol");
  return CLI_SUCCESS;
}

CLI (no_debug_elmi_timer,
  no_debug_elmi_timer_cmd,
  "no debug elmi timer",
  CLI_NO_STR,
  CLI_DEBUG_STR,
  CLI_ELMI_STR,
  "ELMI Timers")
{
  struct elmi_master *em = cli->vr->proto;

  ELMI_DEBUG_OFF (cli, timer, TIMER, "ELMI timer");
  return CLI_SUCCESS;
}

CLI (debug_elmi_all,
  debug_elmi_all_cmd,
  "debug elmi all",
  CLI_DEBUG_STR,
  CLI_ELMI_STR,
  "enable all debugging")
{
  elmi_debug_all_on (cli);
  if (cli->mode != CONFIG_MODE)
    cli_out (cli, "All possible debugging has been turned on\n");

  return CLI_SUCCESS;
}

CLI (no_debug_elmi_all,
  no_debug_elmi_all_cmd,
  "no debug elmi all",
  CLI_NO_STR,
  CLI_DEBUG_STR,
  CLI_ELMI_STR,
  "all")
{
  elmi_debug_all_off (cli);
  if (cli->mode != CONFIG_MODE)
    cli_out (cli, "All possible debugging has been turned off\n");

  return CLI_SUCCESS;
}

CLI (show_debugging_elmi,
  show_debugging_elmi_cmd,
  "show debugging elmi",
  CLI_SHOW_STR,
  "Debugging information outputs",
  "Multiple Spanning Tree Protocol (ELMI)")
{
    struct elmi_master *em = cli->vr->proto;

    cli_out (cli, "ELMI debugging status:\n");

    if (ELMI_DEBUG(timer, TIMER))
      cli_out (cli, "  ELMI timer debugging is on\n");
    if (ELMI_DEBUG(protocol, PROTOCOL))
      cli_out (cli, "  ELMI protocol debugging is on\n");
    if (ELMI_DEBUG(packet, TX))
      cli_out (cli, "  ELMI transmiting packet debugging is on\n");
    if (ELMI_DEBUG(packet, RX))
      cli_out (cli, "  ELMI receiving packet debugging is on\n");
    if (ELMI_DEBUG(event, EVENT))
      cli_out (cli, "  ELMI event debugging is on\n");

    cli_out (cli, "\n");

  return CLI_SUCCESS;
}

/* interface commands */
CLI (interface_elmi,
  interface_elmi_cmd,
  "interface IFNAME",
  "Select an interface to configure",
  "Interface's name")
{
  struct interface *ifp = NULL;

  if (!argv[0])
    return CLI_ERROR;

  ifp = ifg_get_by_name (&ZG->ifg, argv[0]);

  cli->index = ifp;
  cli->mode = INTERFACE_MODE;

  return CLI_SUCCESS;
}

int
elmi_config_write_debug (struct cli *cli)
{
  struct elmi_master *em = cli->vr->proto;
  s_int32_t write = 0;

  if (CONF_ELMI_DEBUG (event, EVENT))
    {
      write ++;
      cli_out (cli, "debug elmi event\n");;
    }

  if (CONF_ELMI_DEBUG (packet, PACKET))
    {

      if (CONF_ELMI_DEBUG (packet, TX) && CONF_ELMI_DEBUG (packet, RX))
        {
          cli_out (cli, "debug elmi packet%s\n",
              CONF_ELMI_DEBUG (packet, DETAIL) ? " detail" : "");
          write++;
        }
      else
        {
          if (CONF_ELMI_DEBUG (packet, TX))
            cli_out (cli, "debug elmi packet tx%s\n",
                CONF_ELMI_DEBUG (packet, DETAIL) ? " detail" : "");
          else
            cli_out (cli, "debug elmi packet rx%s\n",
                CONF_ELMI_DEBUG (packet, DETAIL) ? " detail" : "");
          write++;
        }

      write ++;
      cli_out (cli, "debug elmi packet\n");;
    }

  if (CONF_ELMI_DEBUG (protocol, PROTOCOL))
    {
      write ++;
      cli_out (cli, "debug elmi protocol\n");;
    }

  if (CONF_ELMI_DEBUG (packet, TX))
    {
      write ++;
      cli_out (cli, "debug elmi packet tx\n");;
    }

  if (CONF_ELMI_DEBUG (packet, RX))
    {
      write ++;
      cli_out (cli, "debug elmi packet rx\n");;
    }
  if (CONF_ELMI_DEBUG (timer, TIMER))
    {
      write ++;
      cli_out (cli, "debug elmi timer\n");;
    }
  return write;
}

int 
elmi_config_write (struct cli * cli)
{
  s_int32_t write = 0;
  struct elmi_bridge *br = NULL;
  struct listnode *bridge_node = NULL;
  struct elmi_master *em = NULL;

  em = cli->vr->proto;

  cli_out (cli, "!\n");

  LIST_LOOP (em->bridge_list, br, bridge_node)
    {

      if (br->elmi_enabled == PAL_TRUE)
        {
          if (! pal_strncmp (br->name, "0", 1))
            cli_out (cli, "ethernet lmi global\n");
          else
            cli_out (cli, "ethernet lmi global bridge %s \n", br->name);

          cli_out (cli, "!\n");

          write++;
        }

    } 

  return write;
}

int 
elmi_if_write (struct cli * cli)
{
  struct elmi_bridge *bridge = NULL;
  struct elmi_master *em = NULL;
  struct elmi_ifp *elmi_if = NULL;
  struct listnode *bridge_node = NULL;
  struct avl_node *avl_port = NULL;
  s_int32_t write = 0;

  if (!cli || !cli->vr || !cli->vr->proto)
    return ELMI_FAILURE;

  em = cli->vr->proto;

  if (!em || !em->bridge_list)
    return ELMI_FAILURE;

  LIST_LOOP (em->bridge_list, bridge, bridge_node)
    {
      elmi_if = NULL;

      if (bridge && bridge->port_tree)
        AVL_TREE_LOOP (bridge->port_tree, elmi_if, avl_port)
          {
            if (!elmi_if || (elmi_if->uni_mode != ELMI_UNI_MODE_UNIC &&
                  elmi_if->uni_mode != ELMI_UNI_MODE_UNIN))
              continue; 

            if (!elmi_if->ifp || 
                CHECK_FLAG (elmi_if->ifp->status, IF_HIDDEN_FLAG))
              continue;

            if (write)
              cli_out (cli, "!\n");

            cli_out (cli, "interface %s\n", elmi_if->ifp->name);

            write++;

            if (elmi_if->elmi_enabled == PAL_TRUE)
              {
                cli_out (cli, " ethernet lmi interface\n");
              }

            if (elmi_if->uni_mode == ELMI_UNI_MODE_UNIC)
              {
                if (elmi_if->polling_time != ELMI_T391_POLLING_TIME_DEF)
                  cli_out (cli, " ethernet lmi t391 %d\n", 
                           elmi_if->polling_time);

                if (elmi_if->polling_counter_limit != 
                    ELMI_N391_POLLING_COUNTER_DEF)
                  cli_out (cli, " ethernet lmi n391 %d\n", 
                           elmi_if->polling_counter_limit);
              }
            else
              {
                if (elmi_if->polling_verification_time != ELMI_T392_DEFAULT)
                  cli_out (cli, " ethernet lmi t392 %d\n", 
                           elmi_if->polling_verification_time);

                if (elmi_if->asynchronous_time != 
                    ELMI_ASYNC_TIME_INTERVAL_DEF)
                  cli_out (cli, " ethernet lmi async-msg-interval %d\n", 
                           elmi_if->asynchronous_time);
              }

            if (elmi_if->status_counter_limit != ELMI_N393_STATUS_COUNTER_DEF)
              cli_out (cli, " ethernet lmi n393 %d\n", 
                       elmi_if->status_counter_limit);
          }
    }

  return write;
}

void
elmi_cli_init_debug (struct cli_tree *ctree)
{
  cli_install_config (ctree, DEBUG_MODE, elmi_config_write_debug);

  cli_install (ctree, EXEC_MODE, &show_debugging_elmi_cmd);
  cli_install (ctree, EXEC_MODE, &debug_elmi_proto_cmd);
  cli_install (ctree, EXEC_MODE, &no_debug_elmi_proto_cmd);
  cli_install (ctree, EXEC_MODE, &debug_elmi_timer_cmd);
  cli_install (ctree, EXEC_MODE, &no_debug_elmi_timer_cmd);
  cli_install (ctree, EXEC_MODE, &debug_elmi_packet_rx_cmd);
  cli_install (ctree, EXEC_MODE, &no_debug_elmi_packet_rx_cmd);
  cli_install (ctree, EXEC_MODE, &debug_elmi_packet_tx_cmd);
  cli_install (ctree, EXEC_MODE, &no_debug_elmi_packet_tx_cmd);
  cli_install (ctree, EXEC_MODE, &debug_elmi_event_cmd);
  cli_install (ctree, EXEC_MODE, &no_debug_elmi_event_cmd);
  cli_install (ctree, EXEC_MODE, &debug_elmi_all_cmd);
  cli_install (ctree, EXEC_MODE, &no_debug_elmi_all_cmd);

  cli_install (ctree, CONFIG_MODE, &debug_elmi_proto_cmd);
  cli_install (ctree, CONFIG_MODE, &no_debug_elmi_proto_cmd);
  cli_install (ctree, CONFIG_MODE, &debug_elmi_event_cmd);
  cli_install (ctree, CONFIG_MODE, &no_debug_elmi_event_cmd);
  cli_install (ctree, CONFIG_MODE, &debug_elmi_timer_cmd);
  cli_install (ctree, CONFIG_MODE, &no_debug_elmi_timer_cmd);
  cli_install (ctree, CONFIG_MODE, &debug_elmi_packet_rx_cmd);
  cli_install (ctree, CONFIG_MODE, &no_debug_elmi_packet_rx_cmd);
  cli_install (ctree, CONFIG_MODE, &debug_elmi_packet_tx_cmd);
  cli_install (ctree, CONFIG_MODE, &no_debug_elmi_packet_tx_cmd);
  cli_install (ctree, CONFIG_MODE, &debug_elmi_all_cmd);
  cli_install (ctree, CONFIG_MODE, &no_debug_elmi_all_cmd);
}

/* Install bridge related CLI.  */
void
elmi_cli_init (struct lib_globals * ezg)
{
  struct cli_tree *ctree = ezg->ctree;

  cli_install_config (ctree, ELMI_MODE, elmi_config_write);
  cli_install_default (ctree, ELMI_MODE);

  /* Install interface node. */
  cli_install_config (ctree, INTERFACE_MODE, elmi_if_write);
  cli_install_default (ctree, INTERFACE_MODE);

  /* Install commands. */
  cli_install (ctree, CONFIG_MODE, &interface_elmi_cmd);

  /* "descelmition" commands. */
  cli_install (ctree, INTERFACE_MODE, &interface_desc_cli);
  cli_install (ctree, INTERFACE_MODE, &no_interface_desc_cli);

  cli_install (ctree, CONFIG_MODE, &elmi_enable_global_cmd);
  cli_install (ctree, CONFIG_MODE, &no_elmi_enable_global_cmd);

  cli_install (ctree, INTERFACE_MODE, &elmi_enable_interface_cmd);
  cli_install (ctree, INTERFACE_MODE, &no_elmi_enable_interface_cmd);

  cli_install (ctree, INTERFACE_MODE, &elmi_set_polling_time_cmd);
  cli_install (ctree, INTERFACE_MODE, &no_elmi_set_polling_time_cmd);

  cli_install (ctree, INTERFACE_MODE, 
               &elmi_port_set_polling_verification_time_cmd);
  cli_install (ctree, INTERFACE_MODE, 
               &no_elmi_port_set_polling_verification_time_cmd);

  cli_install (ctree, INTERFACE_MODE, &elmi_port_set_polling_counter_cmd);
  cli_install (ctree, INTERFACE_MODE, &no_elmi_port_set_polling_counter_cmd);
  cli_install (ctree, INTERFACE_MODE, &elmi_port_set_status_counter_cmd);
  cli_install (ctree, INTERFACE_MODE, &no_elmi_port_set_status_counter_cmd);
  cli_install (ctree, INTERFACE_MODE, &elmi_port_set_async_min_time_cmd);
  cli_install (ctree, INTERFACE_MODE, &no_elmi_port_set_async_min_time_cmd);

  elmi_cli_show_init (ezg);
  elmi_cli_init_debug (ctree);

}
