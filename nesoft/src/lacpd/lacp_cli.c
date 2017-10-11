/* Copyright (C) 2003  All Rights Reserved. 
 *  
 *  802.3 LACP
 * 
 * This module defines the CLI interface to the 802.3 LACP
 * 
 */

#include "pal.h"
#include "lib.h"
#include "log.h"
#include "cli.h"

#include "nsm_message.h"
#include "nsm_client.h"
#include "lacp_types.h"
#include "lacpd.h"
#include "lacp_cli.h"
#include "lacp_debug.h"
#include "lacp_link.h"
#include "lacp_selection_logic.h"
#include "lacp_rcv.h"
#include "lacp_periodic_tx.h"
#include "lacp_mux.h"

#ifdef HAVE_HA
#include "lacp_cal.h"
#endif /* HAVE_HA */

/* Bridge CLI definitions.  */

CLI (show_lacp_debugging,
     show_lacp_debugging_cmd,
     "show debugging lacp",
     CLI_SHOW_STR,
     CLI_DEBUG_STR,
     CLI_LACP_STR)
{
  cli_out (cli, "LACP debugging status:\n");
  if (LACP_DEBUG(TIMER))
    cli_out (cli, " LACP timer debugging is on\n");
  if (LACP_DEBUG(TIMER_DETAIL))
    cli_out (cli, " LACP timer-detail debugging is on\n");
  if (LACP_DEBUG(CLI))
    cli_out (cli, " LACP cli debugging is on\n");
  if (LACP_DEBUG(PACKET))
    cli_out (cli, " LACP packet debugging is on\n");
  if (LACP_DEBUG(EVENT))
    cli_out (cli, " LACP event debugging is on\n");
  if (LACP_DEBUG(SYNC))
    cli_out (cli, " LACP sync debugging is on\n");
  if (LACP_DEBUG(HA))
    cli_out (cli, " LACP ha debugging is on\n");
  cli_out (cli, "\n");
  return CLI_SUCCESS;
}

CLI (lacp_debug_enable,
     lacp_debug_enable_cmd,
     "debug lacp (event|cli|timer|packet|sync|ha|all)",
     "debug - enable debug output",
     "lacp - LACP commands",
     "event - echo events to console",
     "cli - echo commands to console",
     "timer - echo timer expiry to console",
     "packet - echo packet contents to console",
     "sync - echo synchronization to console",
     "ha - echo High availability events to console",
     "all - turn on all debugging")
{
  switch (argv[0][0])
    {
    case 'e':
      if (LACP_DEBUG (CLI))
        zlog_info (LACPM, "debug lacp event");
      
      DEBUG_ON(EVENT);
      break;
    case 'c':
      if (LACP_DEBUG (CLI))
        zlog_info (LACPM, "debug lacp cli");
        DEBUG_ON(CLI);    
        break;
    case 't':
      if (LACP_DEBUG (CLI))
        cli_out (cli, "debug lacp timer");
        DEBUG_ON(TIMER);
        break;
    case 'p':
      if (LACP_DEBUG (CLI))
        zlog_info (LACPM, "debug lacp packet");
        DEBUG_ON(PACKET);
        break;
    case 's':
      if (LACP_DEBUG (CLI))
        zlog_info (LACPM, "debug lacp sync");
        DEBUG_ON(SYNC);
        break;
    case 'h':
      if (LACP_DEBUG (CLI))
        zlog_info (LACPM, "debug lacp ha");
        DEBUG_ON(HA);
        break;
    case 'a':
      if (LACP_DEBUG (CLI))
        zlog_info (LACPM, "debug lacp all");
        DEBUG_ON(EVENT);
        DEBUG_ON(CLI);
        DEBUG_ON(TIMER);
        DEBUG_ON(TIMER_DETAIL);
        DEBUG_ON(PACKET);
        DEBUG_ON(SYNC);
        DEBUG_ON(HA);
        break;
    default :
      break;
    }

  return CLI_SUCCESS;
}


CLI (lacp_debug_timer_detail,
     lacp_debug_timer_detail_cmd,
     "debug lacp timer detail",
     "lacp - LACP commands",
     "debug - enable debug output",
     "timer - echo timer to console",
     "detail - echo timer start/stop to console")
{
  if (LACP_DEBUG (CLI))
 cli_out (cli, "debug lacp timer detail");
  DEBUG_ON(TIMER_DETAIL);
  return CLI_SUCCESS;
}


CLI (no_lacp_debug_enable,
     no_lacp_debug_enable_cmd,
     "no debug lacp (event|cli|timer|packet|sync|ha|all)",
     CLI_NO_STR,
     "debug - enable debug output",
     "lacp - LACP commands",
     "event - do not echo events to console",
     "cli - do not echo commands to console",
     "timer - do not echo timer expiry to console",
     "packet - do not echo packet contents to console",
     "sync - do not echo synchronization to console",
     "ha - do not echo High availability events to console",
     "all - turn off all debugging")
{

  switch (argv[0][0])
    {
    case 'e':
      if (LACP_DEBUG (CLI))
        zlog_info (LACPM, "debug lacp event");
      
      DEBUG_OFF(EVENT);
      break;
    case 'c':
      if (LACP_DEBUG (CLI))
        zlog_info (LACPM, "no debug lacp cli");
      DEBUG_OFF (CLI);    
      break;
    case 't':
      if (LACP_DEBUG (CLI))
        cli_out (cli, "no debug lacp timer");
      DEBUG_OFF (TIMER);
      break;
    case 'p':
      if (LACP_DEBUG (CLI))
        zlog_info (LACPM, "no debug lacp packet");
      DEBUG_OFF (PACKET);
      break;
    case 's':
      if (LACP_DEBUG (CLI))
        zlog_info (LACPM, "no debug lacp sync");
      DEBUG_OFF (SYNC);
      break;
    case 'h':
      if (LACP_DEBUG (CLI))
        zlog_info (LACPM, "no debug lacp ha");
      DEBUG_OFF (HA);
      break;
    case 'a':
      if (LACP_DEBUG (CLI))
        zlog_info (LACPM, "no debug lacp all");
      DEBUG_OFF(EVENT);
      DEBUG_OFF(CLI);
      DEBUG_OFF(TIMER);
      DEBUG_OFF(TIMER_DETAIL);
      DEBUG_OFF(PACKET);
      DEBUG_OFF(SYNC);
      DEBUG_OFF (HA);
      break;
    default :
      break;
    }

  return CLI_SUCCESS;
}



CLI (no_lacp_debug_timer_detail,
     no_lacp_debug_timer_detail_cmd,
     "no debug lacp timer detail",
     CLI_NO_STR,
     "lacp - LACP commands",
     "debug - disable debug output",
     "timer - do not echo timer to console",
     "detail - echo timer start/stop to console")
{
  if (LACP_DEBUG (CLI))
    zlog_info (LACPM, "no debug lacp timer detail");
  DEBUG_OFF(TIMER_DETAIL);
  return CLI_SUCCESS;
}


/* Set the system priority of the local system (See 802.3 Section 43.4.5
   and Section 43.6.1). This is used in determining the which system is 
   responsible for assigning the aggregation groups. Note that a lower
   value is a higher priority. */

CLI (lacp_system_priority,
     lacp_system_priority_cmd,
     "lacp system-priority <1-65535>",
     "LACP commands",
     "LACP system priority",
     "LACP system priority <1-65535> default 32768")
{
  unsigned int  priority;

  if (LACP_DEBUG (CLI))
    zlog_info (LACPM, "lacp system-priority %s", argv[0]);

  CLI_GET_INTEGER_RANGE ("system-priority", priority, argv[0], 1,65535);
  if (lacp_set_system_priority (priority) != RESULT_OK)
    {
      cli_out (cli, "%% Unable to set system priority\n");
      return CLI_ERROR;
    }

  return CLI_SUCCESS;
}


CLI (no_lacp_system_priority,
     no_lacp_system_priority_cmd,
     "no lacp system-priority",
     CLI_NO_STR,
     "LACP commands",
     "LACP system priority")
{
  if (LACP_DEBUG (CLI))
    zlog_info (LACPM, "no lacp system-priority");

  if (lacp_unset_system_priority () != RESULT_OK)
    {
      cli_out (cli, "%% Unable to unset system priority\n");
      return CLI_ERROR;
    }

  return CLI_SUCCESS;
}


/* 
   Set the priority of a channel. Channels are selected for aggregation 
   based on their priority with the higher priority (numerically lower) 
   channels selected first.  The default priority is 32768. 
*/
CLI (lacp_port_priority,
     lacp_port_priority_cmd,
     "lacp port-priority <1-65535>",
     "LACP channel commands",
     "set LACP priority for a port",
     "LACP port priority")
{
  unsigned int  priority;
  struct interface *ifp = cli->index;
  struct lacp_link *link;
  
  if (LACP_DEBUG (CLI))
    zlog_info (LACPM, "lacp port-priority %s", argv[0]);
  
  CLI_GET_INTEGER_RANGE ("port-priority", priority, argv[0], 1,65535);
  
  /* get lacp link */
  link = lacp_find_link_by_name (ifp->name);
  if (lacp_set_channel_priority (link, priority) != RESULT_OK)
    {
      cli_out (cli, "%% Unable to set port priority\n");
      return CLI_ERROR;
    }
  
  return CLI_SUCCESS; 
}


CLI (no_lacp_port_priority,
     no_lacp_port_priority_cmd,
     "no lacp port-priority",
     CLI_NO_STR,
     "LACP channel commands",
     "Unset LACP priority for a port")
{
  struct interface *ifp = cli->index;
  struct lacp_link *link;
  
  if (LACP_DEBUG (CLI))
    zlog_info (LACPM, "no lacp port-priority");

  /* get lacp link */
  link = lacp_find_link_by_name (ifp->name);
  if (lacp_unset_channel_priority (link) != 
      RESULT_OK)
    {
      cli_out (cli, "%% Unable to unset port priority\n");
      return CLI_ERROR;
    }
  
  return CLI_SUCCESS; 
}

CLI (lacp_timeout,
     lacp_timeout_cmd,
     "lacp timeout (short|long)",
     "LACP commands",
     "Timeout commands",
     "Set LACP short timeout",
     "Set LACP long timeout")
{
  int timeout = LACP_TIMEOUT_LONG;
  struct interface *ifp = cli->index;
  struct lacp_link *link = NULL;

  link = lacp_find_link_by_name (ifp->name); /* get lacp link */

  switch (argv[0][0])
    {
      case 's' :
        if (LACP_DEBUG (CLI))
          zlog_info (LACPM, "lacp timeout short");
        timeout = LACP_TIMEOUT_SHORT;
        break;

      case 'l' :
        if (LACP_DEBUG (CLI))
          zlog_info (LACPM, "lacp timeout long");
        break;
     }

  if (lacp_set_channel_timeout (link, timeout) != RESULT_OK)
    {
      cli_out (cli, "%% Unable to set lacp timeout\n");
      return CLI_ERROR;
    }

  return CLI_SUCCESS;
}

CLI (interface_lacp,
     interface_lacp_cmd,
     "interface IFNAME",
     "Select an interface to configure",
     "Interface name")
{
  struct interface *ifp;

  ifp = ifg_get_by_name (&LACPM->ifg, argv[0]);
  if (! ifp)
    {
      cli_out (cli, "%% Interface %s not found \n", argv[0]);
      return CLI_ERROR;
    }

  cli->index = ifp;
  cli->mode = INTERFACE_MODE;

  return CLI_SUCCESS;
}

/* Change the MAC address of the aggregator */

CLI (set_lacp_mac_address,
     set_lacp_mac_address_cmd,
     "set port etherchannel IFNAME address MAC",
     CLI_SET_STR,
     "port - port commands",
     "Etherchannel commands",
     CLI_IFNAME_STR,
     "address",
     "mac address in HHHH.HHHH.HHHH format")
{
  u_char mac[6];
  struct lacp_link *link = NULL;

  if (LACP_DEBUG(CLI))
    zlog_debug (LACPM, "set port etherchannel %s address %s", argv[0], argv[1]);

  /* get lacp link */
  link = lacp_find_link_by_name (argv[0]);

  /* format mac address */
  if (pal_sscanf (argv[1], "%4hx.%4hx.%4hx", (unsigned short *)&mac[0], 
                  (unsigned short *)&mac[2], 
                  (unsigned short *)&mac[4]) != 3)
    {
      cli_out (cli, "%% Unable to translate mac address %s\n", argv[1]);
      return CLI_ERROR;
    }

  /* Convert to network order */
  *(unsigned short *)&mac[0] = pal_hton16(*(unsigned short *)&mac[0]);
  *(unsigned short *)&mac[2] = pal_hton16(*(unsigned short *)&mac[2]);
  *(unsigned short *)&mac[4] = pal_hton16(*(unsigned short *)&mac[4]);

  if (lacp_aggregator_set_mac_address(link->aggregator, mac) != RESULT_OK)
    {
      cli_out (cli, "%% Can't set mac address\n");
      return CLI_ERROR;
    }

  return CLI_SUCCESS;
}

/* Show the LAG assignments. */

CLI (show_lacp_channel_group,
     show_lacp_channel_group_cmd,
     "show etherchannel summary",
     CLI_SHOW_STR,
     "LACP etherchannel",
     "Etherchannel Summary information")
{
  register unsigned int agg_ndx;
  register struct lacp_aggregator * agg;
  unsigned int link_ndx;
  struct lacp_aggregator **agg_list = lacp_instance->lacp_agg_list;
   
  if (LACP_DEBUG (CLI))
    zlog_info (LACPM, "show etherchannel summary");

  for (agg_ndx = 0; agg_ndx < LACP_AGG_COUNT_MAX; agg_ndx++)
    {
      if (agg_list[agg_ndx])
        {
          agg = agg_list[agg_ndx];
          cli_out (cli, "%% Aggregator %s %d\n", agg->name, agg->agg_ix);
          cli_out (cli, "%%  Admin Key: %.04d - Oper Key %.04d\n", 
                   agg->actor_admin_aggregator_key,
                   agg->actor_oper_aggregator_key);
          for (link_ndx = 0; link_ndx < LACP_MAX_AGGREGATOR_LINKS; link_ndx++)
            {
              if (agg->link[link_ndx])
                cli_out (cli, "%%   Link: %s (%d) sync: %d\n", 
                         agg->link[link_ndx]->name,
                         agg->link[link_ndx]->actor_port_number,
                         GET_SYNCHRONIZATION(agg->link[link_ndx]->actor_oper_port_state) & GET_SYNCHRONIZATION(agg->link[link_ndx]->partner_oper_port_state));
            }
        }
    }
  return CLI_SUCCESS;
}


CLI (show_lacp_channel_group_key,
     show_lacp_channel_group_key_cmd,
     "show etherchannel <1-65535>",
     CLI_SHOW_STR,
     "LACP channel commands",
     "channel-group number")
{
  register unsigned int agg_ndx;
  register struct lacp_aggregator * agg;
  unsigned int link_ndx;
  unsigned int key;
  struct lacp_aggregator **agg_list = lacp_instance->lacp_agg_list;
  
  if (LACP_DEBUG (CLI))
    zlog_info (LACPM, "show etherchannel %s", argv[0]);

  CLI_GET_INTEGER_RANGE ("admin key", key, argv[0], 1,65535);

  for (agg_ndx = 0; agg_ndx < LACP_AGG_COUNT_MAX; agg_ndx++)
    {
      if (agg_list[agg_ndx] && (agg_list[agg_ndx]->actor_admin_aggregator_key == key))
        {
          agg = agg_list[agg_ndx];
          /* These three are on one line together.*/
          cli_out (cli, "%% Aggregator %s %d ", agg->name, agg->agg_ix);
          cli_out (cli, "Admin Key: %.04d - Oper Key %.04d ", 
                   agg->actor_admin_aggregator_key,
                   agg->actor_oper_aggregator_key);

          cli_out (cli, "Partner LAG: 0x%.04x,%.02x-%.02x-%.02x-%.02x-%.02x-%.02x ", 
                   agg->partner_system_priority,
                   agg->partner_system[0],
                   agg->partner_system[1],
                   agg->partner_system[2],
                   agg->partner_system[3],
                   agg->partner_system[4],
                   agg->partner_system[5]);

          cli_out (cli, "Partner Oper Key %.04d\n", 
                   agg->partner_oper_aggregator_key);

          for (link_ndx = 0; link_ndx < LACP_MAX_AGGREGATOR_LINKS; link_ndx++)
            {
              if (agg->link[link_ndx])
                cli_out (cli, "%%   Link: %s (%d) sync: %d\n", 
                         agg->link[link_ndx]->name,
                         agg->link[link_ndx]->actor_port_number,
                         GET_SYNCHRONIZATION(agg->link[link_ndx]->actor_oper_port_state) & GET_SYNCHRONIZATION(agg->link[link_ndx]->partner_oper_port_state));
            }
        }
    }
  return CLI_SUCCESS;
}

CLI(show_lacp_agg_counter,
    show_lacp_agg_counter_cmd,
    "show lacp-counter <1-65535>",
    CLI_SHOW_STR,
    "LACP commands",
    "channel-group number")
{
  register unsigned int agg_ndx;
  register struct lacp_aggregator * agg;
  unsigned int link_ndx;
  unsigned int key;
  struct lacp_aggregator **agg_list = lacp_instance->lacp_agg_list;

  if (LACP_DEBUG (CLI))
    zlog_info (LACPM, "show lacp counter %s", argv[0]);

  CLI_GET_INTEGER_RANGE ("admin key", key, argv[0], LACP_LINK_ADMIN_KEY_MIN,
                          LACP_LINK_ADMIN_KEY_MAX);
  cli_out (cli, "%% Traffic statistics \n");
  cli_out (cli, "Port      LACPDUs         Marker          Pckt err\n");
  cli_out (cli, "        Sent    Recv    Sent    Recv    Sent    Recv\n");

  for (agg_ndx = 0; agg_ndx < LACP_AGG_COUNT_MAX; agg_ndx++)
    {
      if (agg_list[agg_ndx] && 
         (agg_list[agg_ndx]->actor_admin_aggregator_key == key))
        {
          agg = agg_list[agg_ndx];
          cli_out (cli, "%% Aggregator %s %d \n", agg->name, agg->agg_ix);

          for (link_ndx = 0; link_ndx < LACP_MAX_AGGREGATOR_LINKS; link_ndx++)
            {
              if (agg->link[link_ndx] && agg->link[link_ndx]->ifp)
                {
                  lacp_show_link_traffic_stats (cli,
                                                agg->link[link_ndx]->ifp->name);
                }
 
            }
        }
    }
   return CLI_SUCCESS;
}

CLI(show_lacp_all_agg_counter,
    show_lacp_all_agg_counter_cmd,
    "show lacp-counter",
    CLI_SHOW_STR,
    "LACP commands")
{
  register unsigned int agg_ndx;
  register struct lacp_aggregator * agg;
  unsigned int link_ndx;
  struct lacp_aggregator **agg_list = lacp_instance->lacp_agg_list;

  if (LACP_DEBUG (CLI))
    zlog_info (LACPM, "show lacp-counter all");

  cli_out (cli, "%% Traffic statistics \n");
  cli_out (cli, "Port       LACPDUs         Marker         Pckt err\n");
  cli_out (cli, "        Sent    Recv    Sent    Recv    Sent    Recv\n");

  for (agg_ndx = 0; agg_ndx < LACP_AGG_COUNT_MAX; agg_ndx++)
    {
      if ((agg_list[agg_ndx]))
        {
          agg = agg_list[agg_ndx];
          cli_out (cli, "%% Aggregator %s %d \n", agg->name, agg->agg_ix);

          for (link_ndx = 0; link_ndx < LACP_MAX_AGGREGATOR_LINKS; link_ndx++)
            {
              if (agg->link[link_ndx] && agg->link[link_ndx]->ifp)
                {
                  lacp_show_link_traffic_stats (cli,
                                                agg->link[link_ndx]->ifp->name);
                }

            }
        }
    }
   return CLI_SUCCESS;
}

CLI(clear_lacp_traffic_stats,
    clear_lacp_traffic_stats_cmd,
    "clear lacp <1-65535> counters",
    "clear commands",
    "lacp commands",
    "channel-group number",
    "clear lacp counters")
{
  register unsigned int agg_ndx;
  register struct lacp_aggregator * agg;
  unsigned int link_ndx;
  unsigned int key;
  struct lacp_aggregator **agg_list = lacp_instance->lacp_agg_list;

  if (LACP_DEBUG (CLI))
    zlog_info (LACPM, "clear counter  %s", argv[0]);

  CLI_GET_INTEGER_RANGE ("admin key", key, argv[0], LACP_LINK_ADMIN_KEY_MIN,
                          LACP_LINK_ADMIN_KEY_MAX);

  for (agg_ndx = 0; agg_ndx < LACP_AGG_COUNT_MAX; agg_ndx++)
    {
      if (agg_list[agg_ndx] && 
         (agg_list[agg_ndx]->actor_admin_aggregator_key == key))
        {
          agg = agg_list[agg_ndx];
 
          for (link_ndx = 0; link_ndx < LACP_MAX_AGGREGATOR_LINKS; link_ndx++)
            {
              if (agg->link[link_ndx])
                {
                  agg->link[link_ndx]->lacpdu_recv_count = 0;
                  agg->link[link_ndx]->lacpdu_sent_count = 0;
                  agg->link[link_ndx]->mpdu_recv_count = 0;
                  agg->link[link_ndx]->mpdu_sent_count = 0;
                  agg->link[link_ndx]->pckt_sent_err_count = 0;
                  agg->link[link_ndx]->pckt_recv_err_count = 0;
#ifdef HAVE_HA
                  lacp_cal_modify_lacp_link (agg->link[link_ndx]);
#endif /* HAVE_HA */
                }

            }
        }
    }
   return CLI_SUCCESS;
}

CLI(clear_all_lacp_traffic_stats,
    clear_all_lacp_traffic_stats_cmd,
    "clear lacp counters",
    "clear commands",
    "lacp commands",
    "clear lacp counters")
{
  register unsigned int agg_ndx;
  register struct lacp_aggregator * agg;
  unsigned int link_ndx;
  struct lacp_aggregator **agg_list = lacp_instance->lacp_agg_list;

  if (LACP_DEBUG (CLI))
    zlog_info (LACPM, "clear all lacp counters ");


  for (agg_ndx = 0; agg_ndx < LACP_AGG_COUNT_MAX; agg_ndx++)
    {
      if ((agg_list[agg_ndx]))
        {
          agg = agg_list[agg_ndx];

          for (link_ndx = 0; link_ndx < LACP_MAX_AGGREGATOR_LINKS; link_ndx++)
            {
              if (agg->link[link_ndx])
                {
                  agg->link[link_ndx]->lacpdu_recv_count = 0;
                  agg->link[link_ndx]->lacpdu_sent_count = 0;
                  agg->link[link_ndx]->mpdu_recv_count = 0;
                  agg->link[link_ndx]->mpdu_sent_count = 0;
                  agg->link[link_ndx]->pckt_sent_err_count = 0;
                  agg->link[link_ndx]->pckt_recv_err_count = 0;
#ifdef HAVE_HA
                  lacp_cal_modify_lacp_link (agg->link[link_ndx]);
#endif /* HAVE_HA */
                }

            }
        }
    }
   return CLI_SUCCESS;
}

/* Show a particular lacp channel. */

CLI (show_port_lacp_channel,
     show_port_lacp_channel_cmd,
     "show port etherchannel IFNAME",
     CLI_SHOW_STR,
     "port commands",
     "Etherchannel commands",
     CLI_IFNAME_STR)
{
  struct lacp_link * link;
  if (LACP_DEBUG (CLI))
    zlog_info (LACPM, "show port etherchannel %s", argv[0]);

  link = lacp_find_link_by_name (argv[0]);
  if (link == NULL)
    {
      cli_out (cli, "%% Unable to find lacp link %s\n", argv[0]);
      return RESULT_ERROR;
    }
  cli_out (cli, "%% LACP link info: %s - %d\n", argv[0], 
           link->actor_port_number);

  cli_out (cli, "%% LAG ID: 0x%.04x,%.02x-%.02x-%.02x-%.02x-%.02x-%.02x,0x%.04x\n",
           LACP_SYSTEM_PRIORITY, 
           link->mac_addr[0],
           link->mac_addr[1],
           link->mac_addr[2],
           link->mac_addr[3],
           link->mac_addr[4],
           link->mac_addr[5],
           link->actor_oper_port_key);

  cli_out (cli, "%% Partner oper LAG ID: 0x%.04x,%.02x-%.02x-%.02x-%.02x-%.02x-%.02x,0x%.04x\n",
           link->partner_oper_port_priority,
           link->partner_oper_system[0],
           link->partner_oper_system[1],
           link->partner_oper_system[2],
           link->partner_oper_system[3],
           link->partner_oper_system[4],
           link->partner_oper_system[5],
           link->partner_oper_key);

  cli_out (cli, "%% Actor priority: 0x%.04x (%d)\n",
           link->actor_port_priority,
           link->actor_port_priority);

  cli_out (cli, "%% Admin key: 0x%.04x (%d) Oper key: 0x%.04x (%d)\n",
           link->actor_admin_port_key,
           link->actor_admin_port_key,
           link->actor_oper_port_key,
           link->actor_oper_port_key);

  cli_out (cli, "%% Physical admin key:(%d)\n", link->ifp->lacp_admin_key);
  
  cli_out (cli, "%% Receive machine state : %s\n", 
           rcv_state_str (link->rcv_state));
  cli_out (cli, "%% Periodic Transmission machine state : %s\n",
           periodic_state_str (link->periodic_tx_state));
  cli_out (cli, "%% Mux machine state : %s\n", 
           mux_state_str (link->mux_machine_state));  

  cli_out (cli, "%% Oper state: ACT:%d TIM:%d AGG:%d SYN:%d COL:%d DIS:%d DEF:%d EXP:%d\n",
           GET_LACP_ACTIVITY(link->actor_oper_port_state),

           GET_LACP_TIMEOUT(link->actor_oper_port_state),
           GET_AGGREGATION(link->actor_oper_port_state),
           GET_SYNCHRONIZATION(link->actor_oper_port_state),
           GET_COLLECTING(link->actor_oper_port_state),
           GET_DISTRIBUTING(link->actor_oper_port_state),
           GET_DEFAULTED(link->actor_oper_port_state),
           GET_EXPIRED(link->actor_oper_port_state));

  cli_out (cli, "%% Partner oper state: ACT:%d TIM:%d AGG:%d SYN:%d COL:%d DIS:%d DEF:%d EXP:%d\n",
           GET_LACP_ACTIVITY(link->partner_oper_port_state),
           GET_LACP_TIMEOUT(link->partner_oper_port_state),
           GET_AGGREGATION(link->partner_oper_port_state),
           GET_SYNCHRONIZATION(link->partner_oper_port_state),
           GET_COLLECTING(link->partner_oper_port_state),
           GET_DISTRIBUTING(link->partner_oper_port_state),
           GET_DEFAULTED(link->partner_oper_port_state),
           GET_EXPIRED(link->partner_oper_port_state));

  cli_out (cli, "%% Partner link info: admin port %d\n", link->partner_admin_port_number);
  cli_out (cli, "%% Partner oper port: %d\n", link->partner_oper_port_number);

  cli_out (cli, "%% Partner admin LAG ID: 0x%.04x-%.02x:%.02x:%.02x:%.02x:%.02x%.02x\n",
           link->partner_admin_port_priority,
           link->partner_admin_system[0],
           link->partner_admin_system[1],
           link->partner_admin_system[2],
           link->partner_admin_system[3],
           link->partner_admin_system[4],
           link->partner_admin_system[5]);

  cli_out (cli, "%% Admin state: ACT:%d TIM:%d AGG:%d SYN:%d COL:%d DIS:%d DEF:%d EXP:%d\n",
           GET_LACP_ACTIVITY(link->actor_admin_port_state),
           GET_LACP_TIMEOUT(link->actor_admin_port_state),
           GET_AGGREGATION(link->actor_admin_port_state),
           GET_SYNCHRONIZATION(link->actor_admin_port_state),
           GET_COLLECTING(link->actor_admin_port_state),
           GET_DISTRIBUTING(link->actor_admin_port_state),
           GET_DEFAULTED(link->actor_admin_port_state),
           GET_EXPIRED(link->actor_admin_port_state));

  cli_out (cli, "%% Partner admin state: ACT:%d TIM:%d AGG:%d SYN:%d COL:%d DIS:%d DEF:%d EXP:%d\n",
           GET_LACP_ACTIVITY(link->partner_admin_port_state),
           GET_LACP_TIMEOUT(link->partner_admin_port_state),
           GET_AGGREGATION(link->partner_admin_port_state),
           GET_SYNCHRONIZATION(link->partner_admin_port_state),
           GET_COLLECTING(link->partner_admin_port_state),
           GET_DISTRIBUTING(link->partner_admin_port_state),
           GET_DEFAULTED(link->partner_admin_port_state),
           GET_EXPIRED(link->partner_admin_port_state));

  cli_out (cli, "%% Partner system priority - admin:0x%.04x - oper:0x%.04x\n",
           link->partner_admin_system_priority,
           link->partner_oper_system_priority);
  if (link->aggregator)
    cli_out (cli, "%% Aggregator ID: %d\n", link->aggregator->agg_ix);

  return CLI_SUCCESS;
}

/* Show the system id and priority. */

CLI (show_lacp_channel_sysid,
     show_lacp_channel_sysid_cmd,
     "show lacp sys-id",
     CLI_SHOW_STR,
     "LACP commands",
     "sys-id - LACP system id and priority")
{
  if (LACP_DEBUG (CLI))
    zlog_info (LACPM, "show lacp sys-id");

  cli_out (cli, "%% System %.04hx,%.02x-%.02x-%.02x-%.02x-%.02x-%.02x\n",
           LACP_SYSTEM_PRIORITY,
           LACP_SYSTEM_ID[0],
           LACP_SYSTEM_ID[1],
           LACP_SYSTEM_ID[2],
           LACP_SYSTEM_ID[3],
           LACP_SYSTEM_ID[4],
           LACP_SYSTEM_ID[5]);

  return CLI_SUCCESS;
}

/* Show the lacp channels. */

CLI (show_lacp_channel,
     show_lacp_channel_cmd,
     "show etherchannel detail",
     CLI_SHOW_STR,
     "LACP etherchannel",
     "Detailed etherchannel information")
{
  register unsigned int agg_ndx;
  register struct lacp_aggregator * agg;
  unsigned int link_ndx;
  struct lacp_aggregator **agg_list = lacp_instance->lacp_agg_list; 
  
  if (LACP_DEBUG (CLI))
    zlog_info (LACPM, "show etherchannel detail");


  for (agg_ndx = 0; agg_ndx < LACP_AGG_COUNT_MAX; agg_ndx++)
    {
      if ((agg = agg_list[agg_ndx]) && agg->linkCnt > 0)
        {
          cli_out (cli, "%% Aggregator %s %d\n", agg->name, agg->agg_ix);
          cli_out (cli, "%%  Mac address: %.02x:%.02x:%.02x:%.02x:%.02x:%.02x\n", 
                   agg->aggregator_mac_address[0],
                   agg->aggregator_mac_address[1],
                   agg->aggregator_mac_address[2],
                   agg->aggregator_mac_address[3],
                   agg->aggregator_mac_address[4],
                   agg->aggregator_mac_address[5]);
          cli_out (cli, "%%  Admin Key: %.04d - Oper Key %.04d\n", 
                   agg->actor_admin_aggregator_key,
                   agg->actor_oper_aggregator_key);
          cli_out (cli, "%%  Receive link count: %d - Transmit link count: %d\n",
                   agg->receive_state, agg->transmit_state);
          cli_out (cli, "%%  Individual: %d - Ready: %d\n",
                   agg->individual_aggregator, agg->ready);
          cli_out (cli, "%%  Partner LAG- 0x%.04x,%.02x-%.02x-%.02x-%.02x-%.02x-%.02x\n", 
                   agg->partner_system_priority,
                   agg->partner_system[0],
                   agg->partner_system[1],
                   agg->partner_system[2],
                   agg->partner_system[3],
                   agg->partner_system[4],
                   agg->partner_system[5]);
          for (link_ndx = 0; link_ndx < LACP_MAX_AGGREGATOR_LINKS; link_ndx++)
            {
              if (agg->link[link_ndx])
                cli_out (cli, "%%   Link: %s (%d) sync: %d\n", 
                         agg->link[link_ndx]->name,
                         agg->link[link_ndx]->actor_port_number,
                         GET_SYNCHRONIZATION(agg->link[link_ndx]->actor_oper_port_state) & GET_SYNCHRONIZATION(agg->link[link_ndx]->partner_oper_port_state));
                
            }
        }
    }
  return CLI_SUCCESS;
}


#ifdef HAVE_DEV_TEST
CLI (show_lacp_channel_all,
     show_lacp_channel_all_cmd,
     "show etherchannel all",
     CLI_SHOW_STR,
     "Etherchannel commands",
     "all active/inactive etherchannels")
{
  register unsigned int agg_ndx;
  register struct lacp_aggregator * agg;
  unsigned int link_ndx;
  struct lacp_aggregator **agg_list = lacp_instance->lacp_agg_list; 
  
  if (LACP_DEBUG (CLI))
    zlog_info (LACPM, "show etherchannel all");


  for (agg_ndx = 0; agg_ndx < LACP_AGG_COUNT_MAX; agg_ndx++)
    {
      if ((agg = agg_list[agg_ndx]) != NULL)
        {
          cli_out (cli, "%% Aggregator %s %d\n", agg->name, agg->agg_ix);
          cli_out (cli, "%%  Mac address: %.02x:%.02x:%.02x:%.02x:%.02x:%.02x\n", 
                   agg->aggregator_mac_address[0],
                   agg->aggregator_mac_address[1],
                   agg->aggregator_mac_address[2],
                   agg->aggregator_mac_address[3],
                   agg->aggregator_mac_address[4],
                   agg->aggregator_mac_address[5]);
          cli_out (cli, "%%  Admin Key: %.04d - Oper Key %.04d\n", 
                   agg->actor_admin_aggregator_key,
                   agg->actor_oper_aggregator_key);
          cli_out (cli, "%%  Receive link count: %d - Transmit link count: %d\n",
                   agg->receive_state, agg->transmit_state);
          cli_out (cli, "%%  Individual: %d - Ready: %d\n",
                   agg->individual_aggregator, agg->ready);
          cli_out (cli, "%%  Partner LAG- 0x%.04x,%.02x-%.02x-%.02x-%.02x-%.02x-%.02x\n", 
                   agg->partner_system_priority,
                   agg->partner_system[0],
                   agg->partner_system[1],
                   agg->partner_system[2],
                   agg->partner_system[3],
                   agg->partner_system[4],
                   agg->partner_system[5]);
          for (link_ndx = 0; link_ndx < LACP_MAX_AGGREGATOR_LINKS; link_ndx++)
            {
              if (agg->link[link_ndx])
                cli_out (cli, "%%   Link: %s (%d) sync: %d\n", 
                         agg->link[link_ndx]->name,
                         agg->link[link_ndx]->actor_port_number,
                         GET_SYNCHRONIZATION(agg->link[link_ndx]->actor_oper_port_state) & GET_SYNCHRONIZATION(agg->link[link_ndx]->partner_oper_port_state));
            }
        }
    }
  return CLI_SUCCESS;
}
#endif /* HAVE_DEV_TEST */


int
lacp_interface_config_write (struct cli * cli)
{
  struct lacp_link *link;
  struct listnode *ln;
  struct interface *ifp;

  LIST_LOOP (cli->zg->ifg.if_list, ifp, ln)
    {
      if (CHECK_FLAG (ifp->status, IF_HIDDEN_FLAG))
        continue;

      link = lacp_find_link_by_name (ifp->name);
      if (link)
        {
          cli_out (cli, "interface %s\n", ifp->name);

          if (link->actor_port_priority != LACP_DEFAULT_PORT_PRIORITY)
            cli_out (cli, " lacp port-priority %d\n", 
                     link->actor_port_priority);

          if (GET_LACP_TIMEOUT (link->actor_admin_port_state)
              == LACP_TIMEOUT_SHORT)
            {
              cli_out (cli, " lacp timeout short\n");
            }
          else
            { 
              cli_out (cli, " lacp timeout long\n");
            }
          
          cli_out (cli, "!\n");
        }
    }

  return RESULT_OK;
}



void
lacp_cli_init (struct lib_globals *lacpm)
{
  struct cli_tree *ctree = lacpm->ctree;

  cli_install_config (ctree, EXEC_MODE, lacp_config_write);
  cli_install_config (ctree, INTERFACE_MODE, lacp_interface_config_write);
  cli_install_default (ctree, CONFIG_MODE);
  cli_install_default (ctree, INTERFACE_MODE);

  cli_install (ctree, EXEC_MODE, &no_lacp_debug_enable_cmd);
  cli_install (ctree, EXEC_MODE, &no_lacp_debug_timer_detail_cmd);

  cli_install (ctree, EXEC_MODE, &lacp_debug_enable_cmd);
  cli_install (ctree, EXEC_MODE, &lacp_debug_timer_detail_cmd);

  cli_install (ctree, EXEC_MODE, &clear_lacp_traffic_stats_cmd);
  cli_install (ctree, EXEC_MODE, &clear_all_lacp_traffic_stats_cmd);

  cli_install (ctree, CONFIG_MODE, &interface_lacp_cmd);
  cli_install (ctree, CONFIG_MODE, &lacp_system_priority_cmd);
  cli_install (ctree, CONFIG_MODE, &no_lacp_system_priority_cmd);

  cli_install (ctree, EXEC_MODE, &show_lacp_channel_group_cmd);
  cli_install (ctree, EXEC_MODE, &show_lacp_channel_group_key_cmd);
  cli_install (ctree, EXEC_MODE, &show_port_lacp_channel_cmd);
  cli_install (ctree, EXEC_MODE, &show_lacp_channel_sysid_cmd);
  cli_install (ctree, EXEC_MODE, &show_lacp_channel_cmd);
  cli_install (ctree, EXEC_MODE, &show_lacp_debugging_cmd);
  cli_install (ctree, EXEC_MODE, &show_lacp_agg_counter_cmd);
  cli_install (ctree, EXEC_MODE, &show_lacp_all_agg_counter_cmd);
#ifdef HAVE_DEV_TEST
  cli_install (ctree, EXEC_MODE, &show_lacp_channel_all_cmd);
#endif /* HAVE_DEV_TEST */

  cli_install (ctree, INTERFACE_MODE, &lacp_port_priority_cmd);
  cli_install (ctree, INTERFACE_MODE, &no_lacp_port_priority_cmd);
  cli_install (ctree, INTERFACE_MODE, &lacp_timeout_cmd);

#ifdef MEMMGR
  /* Init Memory Manager CLIs. */
  memmgr_cli_init (lacpm);
#endif /* MEMMGR */

}
