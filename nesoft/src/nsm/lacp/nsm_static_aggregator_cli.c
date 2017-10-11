/* Copyright (C) 2005-2006  All Rights Reserved. */

#include "pal.h"
#ifdef HAVE_HAL
#include "hal_incl.h"
#endif /* HAVE_HAL */
#include "lib.h"
#include "if.h"
#include "table.h"
#include "prefix.h"
#include "thread.h"
#include "nexthop.h"
#include "cli.h"
#include "host.h"
#include "snprintf.h"
#include "nsmd.h"
#include "nsm_server.h"
#include "nsm_router.h"
#include "nsm_message.h"
#include "nsm_interface.h"
#include "lacp/nsm_lacp.h"
#include "nsm_static_aggregator_cli.h"

#ifdef HAVE_HA
#include "lib_cal.h"
#include "nsm_lacp_cal.h"
#endif /* HAVE_HA */

/* CLI to associate a port to a static aggregator*/
CLI (interface_static_channel_group,
     interface_static_channel_group_cmd,
     "static-channel-group <1-12>",
     "Static channel commands",
     "Channel group number")
{
  struct interface *ifp = cli->index;
  struct nsm_if *zif = NULL;
  struct nsm_if *agg_zif = NULL;
  struct interface *agg_ifp = NULL;
  struct interface *first_member_ifp = NULL;
  u_int16_t key;
  char agg_name[INTERFACE_NAMSIZ + 1];
  struct nsm_msg_lacp_agg_add msg;
  int ret = 0;
  bool_t send_msg;
  int status = RESULT_OK;

  if (ifp == NULL)
    return CLI_ERROR;

  /* Check if aggregator count exceeds the max limit*/
  if (ng->agg_cnt == NSM_STATIC_AGG_COUNT_MAX)
    {
      cli_out (cli, "%% Exceeding the aggregator count\n");
      return CLI_ERROR;
    }
  
  CLI_GET_INTEGER_RANGE ("admin-key", key, argv[0], NSM_STATIC_AGG_KEY_MIN,
                         NSM_STATIC_AGG_KEY_MAX);
  
  zif = (struct nsm_if *)ifp->info;
  if (! zif)
    {
      /* Interface not found*/
      return CLI_ERROR;
    }

#ifdef HAVE_PVLAN
  if (zif->switchport &&
      &zif->switchport->vlan_port)
    {
      if ((&zif->switchport->vlan_port)->pvlan_configured)
        {
          /* on private port, static agg can not be created */
          return CLI_ERROR;
        }
    }
#endif /* HAVE_PVLAN */
 
  /* Check whether the port is itself an aggregator*/
  if (zif->agg.type == NSM_IF_AGGREGATOR)
    {
      cli_out (cli, "%% The port is the aggregator\n");
      return CLI_ERROR;
    }

  /* Check whether the port is already a part of any aggregator*/
  if (zif->agg_config_type == AGG_CONFIG_LACP)
    {
      cli_out (cli, "%% The port is already under lacp control\n");
      return CLI_ERROR;
    }
  
  if (zif->agg_config_type == AGG_CONFIG_STATIC)
    {
      cli_out (cli, "%% The port is already configured for aggregation\n");
      return CLI_ERROR;
    }
  
  pal_mem_set (&msg, 0, sizeof (struct nsm_msg_lacp_agg_add));

  pal_snprintf (agg_name, INTERFACE_NAMSIZ, "sa%d", key);

  /* Get the aggreagtor if present*/
  agg_ifp = ifg_lookup_by_name (&nzg->ifg, agg_name);

  if (agg_ifp !=NULL)
  { 
    agg_zif = (struct nsm_if *) agg_ifp->info;
    if (agg_zif == NULL)
      {
        cli_out (cli, "%%nsm local interface not found. \n");
        return CLI_ERROR;
      }

    /* Check if aggregated port matches other ports in aggregator. */
    if (agg_zif->agg.info.memberlist && 
        agg_zif->agg.info.memberlist->count > 0)
     {
        /* Get first member. */
        first_member_ifp = listnode_head (agg_zif->agg.info.memberlist);
        if(NULL != first_member_ifp) 
        {
           if(first_member_ifp->lacp_admin_key != ifp->lacp_admin_key)   
           {
               /* Store key, chan_activate and config_type for a 
                * possible future restoration 
                */
              zif->conf_key = key;
              zif->conf_chan_activate = PAL_TRUE;
              zif->conf_agg_config_type = AGG_CONFIG_STATIC;

              zif->agg_config_type = AGG_CONFIG_NONE;
              ifp->lacp_agg_key = 0;
#ifdef HAVE_HA
              lib_cal_modify_interface (NSM_ZG, ifp);
              nsm_lacp_cal_modify_nsm_if_lacp (zif);
#endif /* HAVE_HA */

              status = nsm_lacp_check_link_mismatch (cli->vr->proto,
                                                     first_member_ifp, ifp);
              
               if (status == NSM_LACP_ERROR_ADMIN_KEY_MISMATCH)
                {

                  cli_out (cli, "%% The port properties don't match other ports in aggregator.\n");
                  return CLI_ERROR;
                }
           }
        }
     }

    if (agg_zif->agg.info.memberlist && 
        agg_zif->agg.info.memberlist->count == NSM_MAX_AGGREGATOR_LINKS)
      {
        cli_out (cli, "%%Max ports configured on the aggregator. \n" 
                      "%%Can't aggregate the port \n");
        return CLI_ERROR;
      }
  }
  if (agg_zif && agg_zif->bridge && zif->bridge)
    {
      if ( pal_mem_cmp (agg_zif->bridge->name, zif->bridge->name, NSM_BRIDGE_NAMSIZ) !=0)
        {
           cli_out (cli, " %%Aggregator is already bridge-grouped.\n"
                         " %%Disassociate the interface to the bridge and then give static-channel-group command \n");
           return CLI_ERROR;
        }
    }

  /* If the aggregator is present populate the mac_addr field and the*/
  /* agg_name with the existing mac_addr and name of the aggregator else with */
  /* the mac_addr of the port and form the name of aggregator*/
  if (agg_ifp)
    {
      pal_strncpy (msg.agg_name, agg_ifp->name, LACP_AGG_NAME_LEN-1);
      pal_mem_cpy (msg.mac_addr, agg_ifp->hw_addr, ETHER_ADDR_LEN);
      send_msg = NSM_FALSE;
    }
  else
    {
      pal_mem_set (agg_name, 0, (INTERFACE_NAMSIZ + 1));
      pal_snprintf (agg_name, INTERFACE_NAMSIZ, "sa%d", key);
      pal_strncpy (msg.agg_name, agg_name, LACP_AGG_NAME_LEN-1);
      pal_mem_cpy (msg.mac_addr, ifp->hw_addr, ETHER_ADDR_LEN);
      send_msg = NSM_TRUE;
    }

  NSM_SET_CTYPE (msg.cindex, NSM_LACP_AGG_CTYPE_PORTS_ADDED);
  msg.add_count = 1;

  msg.ports_added = &ifp->ifindex;
  msg.agg_type = AGG_CONFIG_STATIC;

  zif->agg_config_type = AGG_CONFIG_STATIC;
  ifp->lacp_agg_key = key;
  
#ifdef HAVE_HA
  lib_cal_modify_interface (NSM_ZG, ifp);
  nsm_lacp_cal_modify_nsm_if_lacp (zif);
#endif /* HAVE_HA */
  
  ret = nsm_lacp_api_add_aggregator_member (cli->vr->proto, 
                                            ifp, key, PAL_FALSE, PAL_FALSE,
                                            AGG_CONFIG_STATIC); 
  if (ret < 0)
    {
      cli_out (cli, "%% Failed to aggregate port \n");
      return CLI_ERROR;
    }
  
  ret = nsm_lacp_aggregator_add_msg_process (cli->vr->proto, &msg);
  if (ret < 0)
    {
      cli_out (cli, "%% Failed to aggregate port \n");
      return CLI_ERROR;
    }
    
  /* update LACP with the incremented aggregator count */
  if (send_msg)
    nsm_send_lacp_agg_count_update (NSM_MSG_INCR_AGG_CNT);
  
  return CLI_SUCCESS;
}

/* Cli to delete the member links*/
CLI (no_interface_static_channel_group,
     no_interface_static_channel_group_cmd,
     "no static-channel-group",
     CLI_NO_STR,
     "Static channel commands")
{
  struct interface *ifp = cli->index;
  struct nsm_if *zif = NULL;
  struct nsm_master *nm = NULL;
  struct interface *ifp2 = NULL;
  struct nsm_if *zif2 = NULL;

  if (ifp == NULL)
    return CLI_ERROR;

  zif = (struct nsm_if *)ifp->info;
  if (zif == NULL)
    return CLI_ERROR;

  /* Check whether the port is itself an aggregator*/
  if (zif->agg.type == NSM_IF_AGGREGATOR)
    {
      cli_out (cli, "%% The port is the aggregator\n");
      return CLI_ERROR;
    }

  if (zif->agg_config_type != AGG_CONFIG_STATIC)
    {
      cli_out (cli, "%% The port is not configured for aggregation\n");
      return CLI_ERROR;
    }

  if (zif->agg.type == NSM_IF_AGGREGATED)
    {
      nm = cli->vr->proto;

      ifp2 = zif->agg.info.parent;
      NSM_ASSERT (ifp2 != NULL);
      if (! ifp2)
        return CLI_ERROR;

      zif2 = (struct nsm_if *)ifp2->info;
      if (!zif2)
        return CLI_ERROR;
      
      if (zif2->agg.info.memberlist && 
          zif2->agg.info.memberlist->count > 1)
        {
          nsm_lacp_api_delete_aggregator_member (nm, ifp, PAL_FALSE);
          nsm_lacp_process_delete_aggregator_member (nm, ifp2->name, 
                                                     ifp->ifindex); 
        }
      else
        {
          nsm_lacp_api_delete_aggregator_member (nm, ifp, PAL_FALSE);
          nsm_lacp_cleanup_aggregator (nm, ifp2);
          nsm_lacp_process_delete_aggregator (nm, ifp2);
          nsm_send_lacp_agg_count_update (NSM_MSG_DCR_AGG_CNT);
        }
    }
  else
    {
      zif->agg_config_type = AGG_CONFIG_NONE;
    }

#ifdef HAVE_HA
  nsm_lacp_cal_modify_nsm_if_lacp (zif);
#endif /* HAVE_HA */
  return CLI_SUCCESS;
}

CLI( show_static_channel_group,
     show_static_channel_group_cmd,
     "show static-channel-group ",
     CLI_SHOW_STR,
     "Static channel commands")
{
  struct interface *ifp = NULL;
  struct nsm_if *zif = NULL;
  struct listnode *ln = NULL;
  struct listnode *n = NULL;

  /* Loop through all the interface to find out the static aggregator port */
  /* and display its members*/
  LIST_LOOP (cli->vr->ifm.if_list, ifp, n)
    {
      zif = (struct nsm_if *)ifp->info;
      if (!zif)
        continue;
      
      if (zif->agg_config_type != AGG_CONFIG_STATIC ||
          zif->agg.type != NSM_IF_AGGREGATOR)
        continue;
      
      if (zif->agg.info.memberlist)
        {
          cli_out (cli, "%% Static Aggregator: %s\n",ifp->name);
          cli_out (cli, "%% Member:\n");
          LIST_LOOP (zif->agg.info.memberlist, ifp, ln)
            cli_out (cli, "   %s\n",ifp->name);
        }
    }
  
  return CLI_SUCCESS;  
}


/* Send message to LACP regarding addition/deletion of a static aggregator in NSM*/
void
nsm_send_lacp_agg_count_update (int type)
{
  struct nsm_server_client *nsc = NULL;
  struct nsm_server_entry *nse = NULL;
  struct nsm_msg_agg_cnt msg ;
  int i, len;

  pal_mem_set (&msg, 0, sizeof (struct nsm_msg_agg_cnt));

  NSM_SERVER_CLIENT_LOOP (ng, i, nsc, nse)
    if (nsm_service_check (nse, NSM_SERVICE_INTERFACE) &&
        nse->service.protocol_id == APN_PROTO_LACP)
      {
        nse->send.pnt = nse->send.buf + NSM_MSG_HEADER_SIZE;
        nse->send.size = nse->send.len - NSM_MSG_HEADER_SIZE;
        msg.agg_msg_type = type;
        len = nsm_encode_static_agg_cnt (&nse->send.pnt,
                                         &nse->send.size, &msg);
        nsm_server_send_message (nse, 0, 0, NSM_MSG_STATIC_AGG_CNT_UPDATE,
                                 0, len);
        return;
      }
}


/* Interface configuration write*/
int
nsm_static_aggregator_if_config_write (struct cli *cli, struct interface *ifp)
{
  struct nsm_if *zif = NULL;
  struct interface *agg_ifp = NULL;

  if (ifp == NULL)
    return -1;

  zif = (struct nsm_if *)ifp->info;
  if (zif == NULL)
    return -1;

  if (zif->agg_config_type != AGG_CONFIG_STATIC) 
    return 0;


  if((zif->agg.type == NSM_IF_AGGREGATED) && (agg_ifp = zif->agg.info.parent))
    {
       cli_out (cli, " static-channel-group %s\n",&agg_ifp->name[2]);
    }
  else if (zif->agg.type == NSM_IF_AGGREGATOR)
    {
      if (zif->agg.psc != NSM_LACP_PSC_DEFAULT)
        {
          cli_out (cli, " port-channel load-balance %s\n", NSM_LACP_PSC_KEYWORD_GET(zif->agg.psc));
        }
    }
  return 0;
}

/* Install the CLIs*/
void
nsm_static_aggregator_cli_init (struct cli_tree *ctree)
{
  cli_install (ctree, EXEC_MODE, &show_static_channel_group_cmd);
  cli_install (ctree, INTERFACE_MODE, &interface_static_channel_group_cmd);
  cli_install (ctree, INTERFACE_MODE, &no_interface_static_channel_group_cmd);
}
