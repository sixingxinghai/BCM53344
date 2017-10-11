/* Copyright (C) 2004  All Rights Reserved. */

#include "pal.h"
#ifdef HAVE_HAL
#include "hal_incl.h"
#endif /* HAVE_HAL */

#include "cli.h"

#include "nsmd.h"
#include "nsm_interface.h"

#ifdef HAVE_PVLAN
#include "nsm_bridge.h"
#include "nsm_vlan.h"
#endif /* HAVE_PVLAN */

#include "nsm_lacp.h"

#ifdef HAVE_HA
#include "lib_cal.h"
#include "nsm_lacp_cal.h"
#endif /* HAVE_HA */

static void 
nsm_lacp_cli_adminkey_compare (struct cli *cli, struct interface *ifp,
                               u_int32_t agg_key)
{
  struct apn_vr *vr = cli->vr;
  struct nsm_master *nm = vr->proto;
  struct nsm_lacp_admin_key_element *ake = NULL;
  struct nsm_lacp_admin_key_element *mem_ake = NULL;
  u_int16_t vid;
  bool_t print_line = PAL_TRUE; 
  struct interface *agg_ifp;
  struct interface *mem_ifp;
  struct nsm_if *agg_zif;
  char agg_name[INTERFACE_NAMSIZ];
  
  pal_snprintf (agg_name, INTERFACE_NAMSIZ, "po%d", agg_key);
                                                                                
  agg_ifp = ifg_lookup_by_name (&nzg->ifg, agg_name);
  if (agg_ifp != NULL)
    {
      agg_zif = (struct nsm_if *)agg_ifp->info;
      /* Get first member of the aggregator, and compare admin keys. */
      if (agg_zif && agg_zif->agg.info.memberlist &&
          agg_zif->agg.info.memberlist->count > 0)
        {
          mem_ifp = listnode_head(agg_zif->agg.info.memberlist);
          if (mem_ifp->lacp_admin_key != ifp->lacp_admin_key)
            {
              ake = nsm_lacp_admin_key_element_lookup(nm, ifp->lacp_admin_key); 
              mem_ake = nsm_lacp_admin_key_element_lookup(nm, 
                                                      mem_ifp->lacp_admin_key); 
              if (!ake || !mem_ake)
                {
                  cli_out (cli, 
                           "Couldn't find admin key elements to compare\n");
                  return;
                }
              cli_out (cli, "%s can be aggregated to channel %u if it has:\n", 
                       ifp->name, agg_key);
              if (mem_ake->bandwidth != ake->bandwidth)
                cli_out (cli, "  bandwidth: %ld\n", mem_ake->bandwidth);
              if (mem_ake->mtu != ake->mtu)
                cli_out (cli, "  mtu: %d\n", mem_ake->mtu);
              if (mem_ake->duplex != ake->duplex)
                cli_out (cli, "  duplex mode: %d\n", mem_ake->duplex);
              if (mem_ake->hw_type != ake->hw_type)
                cli_out (cli, "  hardware type: %u\n", mem_ake->hw_type);
              if (mem_ake->type != ake->type)
                cli_out (cli, "  type: %s\n", 
                   ((mem_ake->type == NSM_IF_TYPE_L2) ? "L2(switchport)":"L3"));
#ifdef HAVE_VLAN
              if (mem_ake->vlan_id != ake->vlan_id)
                cli_out (cli, "  vlan id: %u\n", mem_ake->vlan_id);
              if (mem_ake->vlan_port_mode != ake->vlan_port_mode)
                cli_out (cli, "  vlan port mode: %u\n",
                         mem_ake->vlan_port_mode);
              if (mem_ake->vlan_bmp_static)
                {
                  for (vid = NSM_VLAN_DEFAULT_VID; vid <= NSM_VLAN_MAX; vid++)
                    {
                      if (!ake->vlan_bmp_static)
                        {
                          if (NSM_VLAN_BMP_IS_MEMBER(*(mem_ake->vlan_bmp_static), 
                                                   vid))
                            {
                              if (print_line)
                                {
                                  cli_out (cli, "  add static vlans: \n   ");
                                  print_line = PAL_FALSE;
                                }
                              cli_out (cli, "%4u ", vid);
                            }
                        }
                      else
                        {
                          if (!NSM_VLAN_BMP_IS_MEMBER(*(ake->vlan_bmp_static),
                                                    vid) && 
                            NSM_VLAN_BMP_IS_MEMBER(*(mem_ake->vlan_bmp_static),
                                                   vid))
                            {
                              if (print_line)
                                {
                                  cli_out (cli, "  add static vlans: \n   ");
                                  print_line = PAL_FALSE;
                                }
                              cli_out (cli, "%4u ", vid);
                            }
                        }
                    }
                  if (!print_line)
                    cli_out (cli, "\n");
                  if (ake->vlan_bmp_static)
                    {
                      print_line = PAL_TRUE;
                      for (vid = NSM_VLAN_DEFAULT_VID; vid <= NSM_VLAN_MAX;
                           vid++)
                        {
                          if (NSM_VLAN_BMP_IS_MEMBER(*(ake->vlan_bmp_static),
                                                     vid) &&
                            !NSM_VLAN_BMP_IS_MEMBER(*(mem_ake->vlan_bmp_static),
                                                    vid))
                            {
                              if (print_line)
                                {
                                  cli_out (cli, "  remove static vlans: \n   ");
                                  print_line = PAL_FALSE;
                                }
                              cli_out (cli, "%4u ", vid);
                            }
                        }
                      if (!print_line)
                        cli_out (cli, "\n");
                    }
                } /* add/remove static vlans */ 
              else
                if (ake->vlan_bmp_static)
                  {
                    for (vid = NSM_VLAN_DEFAULT_VID; vid <= NSM_VLAN_MAX; vid++)
                      {
                        if (NSM_VLAN_BMP_IS_MEMBER(*(ake->vlan_bmp_static),vid))
                          {
                            if (print_line)
                              {
                                cli_out (cli, "  remove static vlans: \n   ");
                                print_line = PAL_FALSE;
                              }
                            cli_out (cli, "%4u ", vid);
                          }
                      }
                    if (!print_line)
                      cli_out (cli, "\n");
                  } /* remove static vlans */ 
#endif /* HAVE_VLAN */
#ifdef HAVE_L2
              if (mem_ake->bridge_id != ake->bridge_id)
                cli_out (cli, "   bridge id: %d\n", mem_ake->bridge_id);
#endif /*HAVE_L2 */
            } /* if admin key mismatch */
        } /* if agg_ifp has members */
    } /* if agg_ifp != NULL */
}
                                           
static void 
nsm_lacp_show_admin_key_list_details(struct cli *cli)
{
  struct apn_vr *vr = cli->vr;
  struct nsm_master *nm = vr->proto;
  struct nsm_lacp_admin_key_element *ake = NULL;
  u_int16_t vid;                                                                                
  for (ake = nm->ake_list; ake; ake = ake->next)
    {
      cli_out (cli, "%% Admin Key: %u\n", ake->key);
      cli_out (cli, "   bandwidth: %ld\n", ake->bandwidth);
      cli_out (cli, "   mtu: %d\n", ake->mtu);
      cli_out (cli, "   duplex mode: %d\n", ake->duplex);
      cli_out (cli, "   hardware type: %u\n", ake->hw_type);
      cli_out (cli, "   type: %d\n", ake->type);
#ifdef HAVE_VLAN
      cli_out (cli, "   vlan id: %u\n", ake->vlan_id);
      cli_out (cli, "   vlan port mode: %u\n", ake->vlan_port_mode);
      if (ake->vlan_bmp_static)
        {
          cli_out (cli, "   static vlans: ");
          for(vid = NSM_VLAN_DEFAULT_VID; vid <= NSM_VLAN_MAX; vid++)
          {
            if ( NSM_VLAN_BMP_IS_MEMBER(*(ake->vlan_bmp_static), vid) )
              cli_out (cli, "%4u ", vid);
          }
          cli_out (cli, "\n");
        }
      else
        cli_out (cli, "   static vlans: None\n");
      if (ake->vlan_bmp_dynamic)
        {
          cli_out (cli, "   dynamic vlans: ");
          for(vid = NSM_VLAN_DEFAULT_VID; vid <= NSM_VLAN_MAX; vid++)
            {
              if ( NSM_VLAN_BMP_IS_MEMBER(*(ake->vlan_bmp_dynamic), vid) )
                cli_out (cli, "%4u ", vid);
            }
          cli_out (cli, "\n");
        }
      else
        cli_out (cli, "   dynamic vlans: None\n");
#endif /* HAVE_VLAN */
#ifdef HAVE_L2
      cli_out (cli, "   bridge id: %d\n", ake->bridge_id);
#endif /*HAVE_L2 */
      cli_out (cli, "   ref count: %d\n", ake->refcount);
    }
}

/* Channel group commands moved from lacp to nsm */
CLI (interface_channel_group,
     interface_channel_group_cmd,
     "channel-group <1-65535> mode (active|passive)",
     "LACP channel commands",
     "Channel group number",
     "mode - channel mode commands",
     "active - enable initiation of LACP negotiation on a port",
     "passive - disable initiation of LACP negotiation on a port")
{
  int ret;
  u_int32_t key;
  struct nsm_if *zif = NULL;
  struct apn_vr *vr = cli->vr;
  bool_t chan_activate = PAL_FALSE;
  struct interface *ifp = cli->index;
  struct nsm_master *nm = vr->proto;

#ifdef HAVE_PVLAN
  struct nsm_bridge_port *br_port = NULL;
#endif /* HAVE_PVLAN */

  CLI_GET_INTEGER_RANGE ("admin-key", key, argv[0], NSM_LACP_LINK_ADMIN_KEY_MIN,
                         NSM_LACP_LINK_ADMIN_KEY_MAX);

  zif = (struct nsm_if *)ifp->info;

  if (! zif)
    {
      /* Interface not found*/
      return CLI_ERROR;
    }

#ifdef HAVE_PVLAN
  br_port = zif->switchport;

  if (br_port && &br_port->vlan_port)
    {
      if ((&br_port->vlan_port)->pvlan_configured)
        {
          /* on private vlan port, agg can not be created */
          return CLI_ERROR;
        }
    }
#endif /* HAVE_PVLAN */

  if (argv[1][0] == 'a')
    {
      chan_activate = PAL_TRUE;
    }

   /* Check whether the port is itself an aggregator*/
  if (zif->agg.type == NSM_IF_AGGREGATOR)
    {
      cli_out (cli, "%% The port is the aggregator\n");
      return CLI_ERROR;
    }

  if (zif->agg_config_type == AGG_CONFIG_STATIC)
    {
      cli_out (cli, "%% The port is already configured for static aggregation\n");
      return CLI_ERROR;
    }

  zif->agg_config_type = AGG_CONFIG_LACP;
  ret = nsm_lacp_api_add_aggregator_member(nm, ifp, (u_int16_t)key, 
                                           chan_activate, PAL_TRUE,
                                           AGG_CONFIG_LACP);
  if (ret == NSM_LACP_ERROR_ADMIN_KEY_MISMATCH)
    {
      /* Store key, chan_activate and config_type for a possible future restoration */
      zif->conf_key = key;
      zif->conf_chan_activate = chan_activate;
      zif->conf_agg_config_type = zif->agg_config_type;

      zif->agg_config_type = AGG_CONFIG_NONE;
      ifp->lacp_agg_key = 0;
#ifdef HAVE_HA
      lib_cal_modify_interface (NSM_ZG, ifp);
      nsm_lacp_cal_modify_nsm_if_lacp (zif); 
#endif /* HAVE_HA */
      cli_out(cli, "%% Port cannot be aggregated.\n");
      return CLI_ERROR;
    }

  if (ret == NSM_LACP_ERROR_CONFIGURED)
    {
      cli_out(cli, "%% Port is already aggregated. \n");
      return CLI_ERROR;
    }
 
  if (argv[1][0] == 'a')
    {
      zif->agg_mode = PAL_TRUE;
    }
  else
    {
      zif->agg_mode = PAL_FALSE;
    }
 
  return ret;
}

CLI (no_interface_channel_group,
     no_interface_channel_group_cmd,
     "no channel-group",
     CLI_NO_STR,
     "LACP channel commands")
{
  struct interface *ifp = cli->index;
  struct nsm_if *zif = NULL;
  int ret = 0;
  struct apn_vr *vr = cli->vr;
  struct nsm_master *nm = vr->proto;

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

  if (!ifp->lacp_agg_key)
    {
      cli_out (cli, "%% The port is not configured for aggregation\n");
      return CLI_ERROR;
    }
  
  ret = nsm_lacp_api_delete_aggregator_member (nm, ifp, PAL_TRUE);
 
  if (ret == NSM_LACP_IF_CONSISTS_PROTECTION_GRP) 
    { 
      cli_out(cli, "%% Error Aggregator part of Protection Group. \n");
      return CLI_ERROR;
    }

  return ret;
}

CLI (show_lacp_etherchannel,
     show_lacp_etherchannel_cmd,
     "show etherchannel",
     CLI_SHOW_STR,
     "LACP etherchannel")
{
  struct interface *ifp = NULL;
  struct nsm_if *zif = NULL;
  struct listnode *n = NULL;
  struct listnode *ln = NULL;

  LIST_LOOP (cli->vr->ifm.if_list, ifp, n)
  {
    zif = (struct nsm_if *)ifp->info;
    if (!zif)
      continue;

    if (zif->agg_config_type != AGG_CONFIG_LACP ||
        zif->agg.type != NSM_IF_AGGREGATOR)
      continue;

    if (zif->agg.info.memberlist)
      {
        cli_out (cli, "%% Lacp Aggregator: %s\n",ifp->name);
        cli_out (cli, "%% Member:\n");
        LIST_LOOP (zif->agg.info.memberlist, ifp, ln)
          cli_out (cli, "   %s\n",ifp->name);
      }
  }

  return CLI_SUCCESS;
}

CLI (show_lacp_admin_key_list_details,
     show_lacp_admin_key_list_details_cmd,
     "show etherchannel admin-key-list-details",
     CLI_SHOW_STR,
     "LACP etherchannel",
     "LACP Admin Key List details")
{
  nsm_lacp_show_admin_key_list_details(cli);
  
  return CLI_SUCCESS;
}

CLI (show_lacp_adminkey_compare,
     show_lacp_adminkey_compare_cmd,
     "show etherchannel admin-key-compare IFNAME <1-65535>",
     CLI_SHOW_STR,
     "LACP etherchannel",
     "LACP Admin Key Compare",
     CLI_IFNAME_STR,
     "channel-group number")
{
  u_int32_t key;
  struct interface *ifp;
  struct apn_vr *vr = cli->vr; 

  CLI_GET_INTEGER_RANGE ("admin-key", key, argv[1], NSM_LACP_LINK_ADMIN_KEY_MIN,
                         NSM_LACP_LINK_ADMIN_KEY_MAX);
  
  ifp = if_lookup_by_name (&vr->ifm, argv[0]);
  if (ifp)
    nsm_lacp_cli_adminkey_compare (cli, ifp, (u_int16_t)key);
  else
    {
      cli_out(cli, "%% Can't find interface %s\n", argv[0]);
      return CLI_ERROR;
    } 
  
  return CLI_SUCCESS;
}

/* Change the port selection criteria of the aggregator */
CLI (lacp_psc_set,
     lacp_psc_set_cmd,
     "port-channel load-balance (dst-mac|src-mac|src-dst-mac|dst-ip|src-ip|src-dst-ip|dst-port|src-port|src-dst-port)",
     "Portchannel commands",
     "Load balancing",
     "Destination Mac address based load balancing",
     "Source Mac address based load balancing",
     "Source and Destination Mac address based load balancing",
     "Destination IP address based load balancing",
     "Source IP address based load balancing",
     "Source and Destination IP address based load balancing",
     "Destination TCP/UDP port based load balancing",
     "Source TCP/UDP port based load balancing",
     "Source and Destination TCP/UDP port based load balancing")
{
  struct interface *ifp = cli->index;
  int psc;
  int ret;

  if (strcmp (argv[0], "dst-mac") == 0)
    psc = HAL_LACP_PSC_DST_MAC;
  else if (strcmp (argv[0], "src-mac") == 0)
    psc = HAL_LACP_PSC_SRC_MAC;
  else if (strcmp (argv[0], "src-dst-mac") == 0)
    psc = HAL_LACP_PSC_SRC_DST_MAC;
  else if (strcmp (argv[0], "dst-ip") == 0)
    psc = HAL_LACP_PSC_DST_IP;
  else if (strcmp (argv[0], "src-ip") == 0)
    psc = HAL_LACP_PSC_SRC_IP;
  else if (strcmp (argv[0], "src-dst-ip") == 0)
    psc = HAL_LACP_PSC_SRC_DST_IP;
  else if (strcmp (argv[0], "src-port") == 0)
    psc = HAL_LACP_PSC_SRC_PORT;
  else if (strcmp (argv[0], "dst-port") == 0)
    psc = HAL_LACP_PSC_DST_PORT;
  else 
    psc = HAL_LACP_PSC_SRC_DST_PORT;
  
  ret = nsm_lacp_aggregator_psc_set (ifp, psc);
  if(ret == NSM_ERR_INTERNAL)
    {
      cli_out(cli, "%% PSC bit combination not supported %s\n", argv[0]);
      return CLI_ERROR;
    }
  else if(ret != NSM_SUCCESS)
  { 
    cli_out(cli, "%% Interface must be an aggregator %s\n", ifp->name);
    return CLI_ERROR;
  }
  return CLI_SUCCESS;
}

/* Unset the port selection criteria of the aggregator */
CLI (lacp_psc_unset,
     lacp_psc_unset_cmd,
     "no port-channel load-balance",
     "CLI_NO_STR",
     "Portchannel commands",
     "Load balancing")
{
  struct interface *ifp = cli->index;
  int ret;

  ret = nsm_lacp_aggregator_psc_set (ifp, NSM_LACP_PSC_DEFAULT);

  if(ret != NSM_SUCCESS)
    return CLI_ERROR;

  return CLI_SUCCESS;
}


CLI (show_lacp_psc,
     show_lacp_psc_cmd,
     "show etherchannel load-balance",
     CLI_SHOW_STR,
     "Etherchannel commands",
     "Load balancing")
{
  struct interface *ifp = NULL;
  struct nsm_if *zif = NULL;
  struct listnode *n = NULL;

  LIST_LOOP (cli->vr->ifm.if_list, ifp, n)
  {
    zif = (struct nsm_if *)ifp->info;
    if (!zif)
      continue;

    if (zif->agg.type != NSM_IF_AGGREGATOR)
      continue;

    if (zif->agg.info.memberlist)
      {
        cli_out (cli, "%% Lacp Aggregator: %s\n",ifp->name);
        cli_out (cli, nsm_lacp_psc_string (zif->agg.psc));
        cli_out (cli, "\n");
      }
  }
  return CLI_SUCCESS;
}


/* LACP config write. */
int
nsm_lacp_if_config_write (struct cli *cli, struct interface *ifp)
{
  struct nsm_if *zif = NULL;

  if (!ifp)
    return -1;

  zif = (struct nsm_if *)ifp->info;
  if (zif == NULL)
    return -1;

  if (zif->agg_config_type != AGG_CONFIG_LACP)
     return 0;
    
  if(zif->agg.type == NSM_IF_AGGREGATED)
    { 
       /* Aggregation key must be assigned to aggregated interface. */
       if(!ifp->lacp_agg_key)
          return -1; 

       cli_out (cli, " channel-group %d mode %s\n",
               ifp->lacp_agg_key, (zif->agg_mode) ? "active" : "passive");
            cli_out (cli, "\n");
    }
  else if(zif->agg.type == NSM_IF_AGGREGATOR)
    {
      cli_out (cli, " port-channel load-balance %s\n", NSM_LACP_PSC_KEYWORD_GET(zif->agg.psc));
      cli_out (cli, "\n");
    }
  return 0;
}


void
nsm_lacp_cli_init (struct cli_tree *ctree)
{
  cli_install (ctree, INTERFACE_MODE, &lacp_psc_set_cmd);
  cli_install (ctree, INTERFACE_MODE, &lacp_psc_unset_cmd);
  cli_install (ctree, EXEC_MODE, &show_lacp_psc_cmd);
  cli_install (ctree, INTERFACE_MODE, &interface_channel_group_cmd); 
  cli_install (ctree, INTERFACE_MODE, &no_interface_channel_group_cmd);
  cli_install (ctree, EXEC_MODE, &show_lacp_etherchannel_cmd);
  cli_install_hidden (ctree, EXEC_MODE, &show_lacp_admin_key_list_details_cmd);
  cli_install_hidden (ctree, EXEC_MODE, &show_lacp_adminkey_compare_cmd);
}
