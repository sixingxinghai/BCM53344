/* Copyright (C) 2004  All Rights Reserved.  */

#include "pal.h"

#ifdef HAVE_PVLAN

#include "if.h"
#include "cli.h"
#include "table.h"
#include "linklist.h"

#include "nsmd.h"
#include "nsm_server.h"
#include "nsm_bridge.h"
#include "nsm_pvlan.h"
#include "nsm_pvlan_cli.h"
#include "nsm_interface.h"
#include "nsm_bridge_cli.h"
#include "nsm_vlan.h"
#include "nsm_vlan_cli.h"
#include "l2_vlan_port_range_parser.h"
#include "avl_tree.h"
static void nsm_all_pvlan_show (struct cli *cli, char *brname);

static void
nsm_pvlan_cli_process_result (struct cli *cli, int result)
{
  switch (result)
    {
      case NSM_VLAN_ERR_BRIDGE_NOT_FOUND:
           cli_out (cli, "%% Bridge not found \n");
           break;
      case NSM_VLAN_ERR_BRIDGE_NOT_VLAN_AWARE:
           cli_out (cli, "%% Bridge not vlan aware \n");
           break;
      case NSM_NO_VLAN_CONFIGURED:
           cli_out (cli, "%% No vlan configured for the bridge\n");
           break;
      case NSM_VLAN_ERR_VLAN_NOT_FOUND:
           cli_out (cli, "%% Vlan not found \n");
           break;
      case NSM_PVLAN_ERR_NOT_CONFIGURED:
           cli_out (cli, "%% Private VLAN Not configured for VLAN \n");
           break;

      case NSM_PVLAN_ERR_NOT_ISOLATED_VLAN:
           cli_out (cli, "%% Private-vlan is not isolated vlan \n");
           break;
      case NSM_PVLAN_ERR_NOT_COMMUNTITY_VLAN:
           cli_out (cli, "%%  Private-vlan is not community vlan \n");
           break;
      case NSM_PVLAN_ERR_NOT_PRIMARY_VLAN:
           cli_out (cli, "%%  Private-vlan is not community vlan \n");
           break;
      case NSM_PVLAN_ERR_PRIMARY_SECOND_SAME:
           cli_out (cli, "%% Primary and secondary vlans are same \n");
           break;
      case NSM_PVLAN_ERR_SECOND_NOT_ASSOCIATED:
           cli_out (cli, "%% Secondary vlan not associated to a primary vlan \n");
           break;
      case NSM_PVLAN_ERR_PORT_NOT_CONFIGURED:
           cli_out (cli, "%% Port not configured for Private vlan \n");
           break;
      case NSM_PVLAN_ERR_NOT_HOST_PORT:
           cli_out (cli, "%% Port is not host port \n");
           break;
      case NSM_PVLAN_ERR_NOT_PROMISCUOUS_PORT:
           cli_out (cli, "%% Port is not Promiscuous port \n");
           break;
      case NSM_PVLAN_ERR_INVALID_MODE:
           cli_out (cli, "%% Invalid mode ]n");
      case NSM_PVLAN_ERR_ISOLATED_VLAN_EXISTS:
           cli_out (cli, " one isolate vlan can be associated to Primary vlan \n");
           break;
      case NSM_PVLAN_ERR_SECONDARY_NOT_MEMBER:
           cli_out (cli, "%% secondary vlan not a member of primary vlan \n");
           break;
      case NSM_PVLAN_ERR_NOT_SECONDARY_VLAN:
           cli_out (cli, "%% Not a secondary vlan \n");
           break;
      case NSM_PVLAN_AGGREGATOR_PORT:
           cli_out (cli, "%% Aggregator or Aggregated port. PVLAN conf not allowed\n");
           break;
      case NSM_PVLAN_ERR_ASSOCIATED_TO_PRIMARY:
           cli_out (cli, "%% secondary vlan already associated to primary vlan \n");
           break;
      case NSM_PVLAN_ERR_PROVIDER_BRIDGE:
           cli_out (cli, "%% Private VLAN cannot be configured on provider bridges\n");
           break;
      default:
           break;
    }
}

CLI (nsm_private_vlan_create,
     nsm_private_vlan_create_cli,
     "private-vlan "NSM_VLAN_CLI_RNG" (community | isolated | primary) bridge <1-32>",
     "private-vlan",
     CLI_VLAN_STR,
     "Set vlan type to community vlan",
     "Set vlan type to isolated vlan",
     "Set vlan type to primary vlan",
     NSM_BRIDGE_STR,
     NSM_BRIDGE_NAME_STR)
{
  int ret = 0;
  u_int16_t vid;
  int brid;
  struct nsm_master *nm = cli->vr->proto;
  struct nsm_bridge_master *master = nsm_bridge_get_master (nm);
  enum nsm_pvlan_type type = NSM_PVLAN_NONE;

  /* Get the vid and check if it is within range (2-4096).  */
  CLI_GET_INTEGER_RANGE ("VLAN id", vid, argv[0],
                                NSM_VLAN_CLI_MIN, NSM_VLAN_CLI_MAX);

   /* Get the bridge group number and see if its within range. */
  CLI_GET_INTEGER_RANGE ("Bridge group", brid, argv[2], 1, 32);

  if (! pal_strncmp (argv[1], "c", 1))
    type = NSM_PVLAN_COMMUNITY;
  else if (! pal_strncmp (argv[1], "i", 1))
    type = NSM_PVLAN_ISOLATED;
  else if (! pal_strncmp (argv[1], "p", 1))
    type = NSM_PVLAN_PRIMARY;

    ret = nsm_pvlan_configure (master, argv[2], vid, type);

  if (ret < 0)
    {
      nsm_pvlan_cli_process_result (cli, ret);
      return CLI_ERROR;
    }

  return CLI_SUCCESS;
}

CLI (no_nsm_private_vlan_create,
     no_nsm_private_vlan_create_cli,
     "no private-vlan "NSM_VLAN_CLI_RNG" (community | isolated | primary) bridge <1-32>",
     CLI_NO_STR,
     "private-vlan",
     CLI_VLAN_STR,
     "Reset vlan type from community vlan to default",
     "Reset vlan type from isolated vlan to default",
     "Reset vlan type from primary vlan to default",
     NSM_BRIDGE_STR,
     NSM_BRIDGE_NAME_STR)
{
  int ret;
  u_int16_t vid;
  int brid;
  struct nsm_master *nm = cli->vr->proto;
  struct nsm_bridge_master *master = nsm_bridge_get_master (nm);
  enum nsm_pvlan_type type = NSM_PVLAN_NONE;

  /* Get the vid and check if it is within range (2-4096).  */
  CLI_GET_INTEGER_RANGE ("VLAN id", vid, argv[0],
                                NSM_VLAN_CLI_MIN, NSM_VLAN_CLI_MAX);

 /* Get the bridge group number and see if its within range. */
  CLI_GET_INTEGER_RANGE ("Bridge group", brid, argv[2], 1, 32);

  if (! pal_strncmp (argv[1], "c", 1))
     type =  NSM_PVLAN_COMMUNITY;
  else if (! pal_strncmp (argv[1], "i", 1))
     type = NSM_PVLAN_ISOLATED;
  else if (! pal_strncmp (argv[1], "p", 1))
     type = NSM_PVLAN_PRIMARY;

     ret = nsm_pvlan_configure_clear (master, argv[2], vid, type);

  if (ret < 0)
    {
      nsm_pvlan_cli_process_result (cli, ret);
      return CLI_ERROR;
    }

  return CLI_SUCCESS;
}

CLI (nsm_private_vlan_associate_add,
     nsm_private_vlan_associate_add_cli,
     "private-vlan "NSM_VLAN_CLI_RNG" association add VLAN_ID bridge <1-32>",
     "private-vlan",
     CLI_VLAN_STR,
     "association to secondary vlan",
     "add",
     "Secondary vlan id",
     NSM_BRIDGE_STR,
     NSM_BRIDGE_NAME_STR)
{
  int ret;
  u_int16_t vid, pvid;
  int brid;
  struct nsm_master *nm = cli->vr->proto;
  char *curr = NULL;
  l2_range_parser_t par;
  l2_range_parse_error_t r_err;
  struct nsm_bridge_master *master = nsm_bridge_get_master (nm);

  /* Get the vid and check if it is within range (2-4096).  */
  CLI_GET_INTEGER_RANGE ("VLAN id", vid, argv[0],
                                    NSM_VLAN_CLI_MIN, NSM_VLAN_CLI_MAX);

  l2_range_parser_init (argv[1], &par);

  if ( (ret = l2_range_parser_validate (&par, cli, nsm_vlan_validate_vid))
       != CLI_SUCCESS)
    return ret;

   /* Get the bridge group number and see if its within range. */
  CLI_GET_INTEGER_RANGE ("Bridge group", brid, argv[2], 1, 32);

   while ( (curr = l2_range_parser_get_next ( &par, &r_err) ) )
    {
      if ( r_err != RANGE_PARSER_SUCCESS )
        {
          cli_out (cli, "%% Invalid Input %s \n",
                               l2_range_parse_err_to_str (r_err) );
          return CLI_ERROR;
        }

      CLI_GET_INTEGER_RANGE ("vlanid", pvid, curr, NSM_VLAN_CLI_MIN,
                                                   NSM_VLAN_CLI_MAX);

      ret = nsm_pvlan_associate (master, argv[2], vid, pvid);

      if (ret <0)
        {
          nsm_pvlan_cli_process_result (cli, ret);
          l2_range_parser_deinit (&par);
          return CLI_ERROR;
        }
    }

  l2_range_parser_deinit (&par);

  return CLI_SUCCESS;
}

CLI (nsm_private_vlan_associate_remove,
     nsm_private_vlan_associate_remove_cli,
     "private-vlan "NSM_VLAN_CLI_RNG" association remove VLAN_ID bridge <1-32>",
     "private-vlan",
     CLI_VLAN_STR,
     "association to secondary vlan",
     "remove",
     "Secondary vlan id",
     NSM_BRIDGE_STR,
     NSM_BRIDGE_NAME_STR)
{
  int ret;
  u_int16_t vid, pvid;
  int brid;
  struct nsm_master *nm = cli->vr->proto;
  char *curr = NULL;
  l2_range_parser_t par;
  l2_range_parse_error_t r_err;
  struct nsm_bridge_master *master = nsm_bridge_get_master (nm);

  /* Get the vid and check if it is within range (2-4096).  */
  CLI_GET_INTEGER_RANGE ("VLAN id", vid, argv[0],
                                 NSM_VLAN_CLI_MIN, NSM_VLAN_CLI_MAX);

  l2_range_parser_init (argv[1], &par);

  if ( (ret = l2_range_parser_validate (&par, cli, nsm_vlan_validate_vid))
       != CLI_SUCCESS)
    return ret;

   /* Get the bridge group number and see if its within range. */
  CLI_GET_INTEGER_RANGE ("Bridge group", brid, argv[2], 1, 32);

  while ( (curr = l2_range_parser_get_next ( &par, &r_err) ) )
    {
      if ( r_err != RANGE_PARSER_SUCCESS )
        {
          cli_out (cli, "%% Invalid Input %s \n",
                                    l2_range_parse_err_to_str (r_err) );
          return CLI_ERROR;
        }
        CLI_GET_INTEGER_RANGE ("vlanid", pvid, curr, NSM_VLAN_CLI_MIN,
                                                     NSM_VLAN_CLI_MAX);
      ret = nsm_pvlan_associate_clear (master, argv[2], vid, pvid);

      if (ret < 0)
        {
          nsm_pvlan_cli_process_result (cli, ret);
          l2_range_parser_deinit (&par);
          return CLI_ERROR;
        }

    }

  l2_range_parser_deinit (&par);

  return CLI_SUCCESS;
}

CLI (no_private_vlan_associate,
     no_private_vlan_associate_cli,
     "no private-vlan "NSM_VLAN_CLI_RNG" association bridge <1-32>",
     CLI_NO_STR,
     "private-vlan",
     CLI_VLAN_STR,
     "association to secondary vlan",
     NSM_BRIDGE_STR,
     NSM_BRIDGE_NAME_STR)
{
  int ret;
  u_int16_t vid;
  int brid;
  struct nsm_master *nm = cli->vr->proto;
  struct nsm_bridge_master *master = nsm_bridge_get_master (nm);

  /* Get the vid and check if it is within range (2-4096).  */
  CLI_GET_INTEGER_RANGE ("VLAN id", vid, argv[0],
                                  NSM_VLAN_CLI_MIN, NSM_VLAN_CLI_MAX);

   /* Get the bridge group number and see if its within range. */
  CLI_GET_INTEGER_RANGE ("Bridge group", brid, argv[1], 1, 32);

     ret = nsm_pvlan_associate_clear_all (master, argv[1], vid);

  if (ret < 0)
    {
      nsm_pvlan_cli_process_result (cli, ret);
      return CLI_ERROR;
    }

  return CLI_SUCCESS;
}

CLI (nsm_pvlan_switchport_mode_pvlan,
     nsm_pvlan_switchport_mode_pvlan_cli,
     "switchport mode private-vlan (host | promiscuous)",
     "Set the mode of the Layer2 interface",
     "mode of the Layer 2 interface",
     "Private-vlan mode",
     "host port",
     "promiscuous port")
{
  int ret;
  enum nsm_vlan_port_mode mode = NSM_VLAN_PORT_MODE_INVALID;
  struct interface *ifp = cli->index;

  NSM_BRIDGE_CHECK_L2_PROPERTY(cli, ifp);
  NSM_BRIDGE_CHECK_BRIDGE_PROPERTY(cli, ifp);

  if (! pal_strncmp (argv[0], "h", 1))
    mode = NSM_PVLAN_PORT_MODE_HOST;
  else if (! pal_strncmp (argv[0], "p", 1))
    mode = NSM_PVLAN_PORT_MODE_PROMISCUOUS;

  ret = nsm_pvlan_api_set_port_mode (ifp, mode);

  if (ret < 0)
    {
      nsm_pvlan_cli_process_result (cli, ret);
      return CLI_ERROR;
    }

  return CLI_SUCCESS;
}

CLI (no_nsm_pvlan_switchport_mode_pvlan,
     no_nsm_pvlan_switchport_mode_pvlan_cli,
     "no switchport mode private-vlan (host | promiscuous)",
     "CLI_NO_STR",
     "Reset the mode of the Layer2 interface",
     "mode of the Layer 2 interface",
     "Private-vlan mode",
     "host port",
     "promiscuous port")
{
  int ret;
  enum nsm_vlan_port_mode mode = NSM_VLAN_PORT_MODE_INVALID;
  struct interface *ifp = cli->index;

  NSM_BRIDGE_CHECK_L2_PROPERTY(cli, ifp);
  NSM_BRIDGE_CHECK_BRIDGE_PROPERTY(cli, ifp);

  if (! pal_strncmp (argv[0], "h", 1))
    mode = NSM_PVLAN_PORT_MODE_HOST;
  else if (! pal_strncmp (argv[0], "p", 1))
    mode = NSM_PVLAN_PORT_MODE_PROMISCUOUS;

  ret = nsm_pvlan_api_clear_port_mode (ifp, mode);

  if (ret < 0)
    {
      nsm_pvlan_cli_process_result (cli, ret);
      return CLI_ERROR;
    }

  return CLI_SUCCESS;
}

CLI (nsm_pvlan_switchport_host_association_add,
     nsm_pvlan_switchport_host_association_add_cli,
     "switchport private-vlan host-association "NSM_VLAN_CLI_RNG" add "NSM_VLAN_CLI_RNG,
     "Set the mode of the Layer2 interface",
     "private-vlan",
     "host-association",
     CLI_VLAN_STR,
     "add",
     CLI_VLAN_STR)
{
  int ret;
  struct interface *ifp = cli->index;
  u_int16_t vid, pvid;

  NSM_BRIDGE_CHECK_L2_PROPERTY(cli, ifp);
  NSM_BRIDGE_CHECK_BRIDGE_PROPERTY(cli, ifp);

  /* Get the vid and check if it is within range (2-4096).  */
  CLI_GET_INTEGER_RANGE ("VLAN id", vid, argv[0],
                                  NSM_VLAN_CLI_MIN, NSM_VLAN_CLI_MAX);

  /* Get the pvid and check if it is within range (2-4096).  */
  CLI_GET_INTEGER_RANGE ("VLAN id", pvid, argv[1],
                                  NSM_VLAN_CLI_MIN, NSM_VLAN_CLI_MAX);

  ret = nsm_pvlan_api_host_association (ifp, vid, pvid);

  if (ret < 0)
    {
      nsm_pvlan_cli_process_result (cli, ret);
      return CLI_ERROR;
    }

  return CLI_SUCCESS;
}

CLI (no_nsm_pvlan_switchport_host_association,
     no_nsm_pvlan_switchport_host_association_cli,
     "no switchport private-vlan host-association",
     "CLI_NO_STR",
     "Set the mode of the Layer2 interface",
     "private-vlan",
     "host-association")
{
  int ret;
  struct interface *ifp = cli->index;

  NSM_BRIDGE_CHECK_L2_PROPERTY(cli, ifp);
  NSM_BRIDGE_CHECK_BRIDGE_PROPERTY(cli, ifp);

  ret = nsm_pvlan_api_host_association_clear_all (ifp);

  if (ret < 0)
    {
      nsm_pvlan_cli_process_result (cli, ret);
      return CLI_ERROR;
    }

  return CLI_SUCCESS;
}

CLI (nsm_pvlan_switchport_mapping_add,
     nsm_pvlan_switchport_mapping_add_cli,
     "switchport private-vlan mapping "NSM_VLAN_CLI_RNG" add VLAN_ID",
     "Set the mode of the Layer2 interface",
     "private-vlan",
     "mapping",
     CLI_VLAN_STR,
     "add",
     "secondary vlan id")
{
  int ret;
  struct interface *ifp = cli->index;
  u_int16_t vid, pvid;
  char *curr = NULL;
  l2_range_parser_t par;
  l2_range_parse_error_t r_err;
  NSM_BRIDGE_CHECK_L2_PROPERTY(cli, ifp);
  NSM_BRIDGE_CHECK_BRIDGE_PROPERTY(cli, ifp);

  /* Get the vid and check if it is within range (2-4096).  */
  CLI_GET_INTEGER_RANGE ("VLAN id", vid, argv[0],
                                  NSM_VLAN_CLI_MIN, NSM_VLAN_CLI_MAX);

  l2_range_parser_init (argv[1], &par);

  if ( (ret = l2_range_parser_validate (&par, cli, nsm_vlan_validate_vid))
       != CLI_SUCCESS)
    return ret;

  while ( (curr = l2_range_parser_get_next ( &par, &r_err) ) )
    {
      if ( r_err != RANGE_PARSER_SUCCESS )
        {
          cli_out (cli, "%% Invalid Input %s \n",
                                    l2_range_parse_err_to_str (r_err) );
          return CLI_ERROR;
        }

      /* Get the pvid and check if it is within range (2-4096).  */
      CLI_GET_INTEGER_RANGE ("VLAN id", pvid, curr,
                                     NSM_VLAN_CLI_MIN, NSM_VLAN_CLI_MAX);

      ret = nsm_pvlan_api_switchport_mapping (ifp, vid, pvid);
      if (ret < 0)
        {
          nsm_pvlan_cli_process_result (cli, ret);
          l2_range_parser_deinit (&par);
          return CLI_ERROR;
        }

    }

  l2_range_parser_deinit (&par);

  return CLI_SUCCESS;

}

CLI (nsm_pvlan_switchport_mapping_remove,
     nsm_pvlan_switchport_mapping_remove_cli,
     "switchport private-vlan mapping "NSM_VLAN_CLI_RNG" remove VLAN_ID",
     "Set the mode of the Layer2 interface",
     "private-vlan",
     "mapping",
     CLI_VLAN_STR,
     "remove",
     "secondary vlan id")
{
  int ret;
  struct interface *ifp = cli->index;
  u_int16_t vid, pvid;
  char *curr = NULL;
  l2_range_parser_t par;
  l2_range_parse_error_t r_err;
  NSM_BRIDGE_CHECK_L2_PROPERTY(cli, ifp);
  NSM_BRIDGE_CHECK_BRIDGE_PROPERTY(cli, ifp);

  /* Get the vid and check if it is within range (2-4096).  */
  CLI_GET_INTEGER_RANGE ("VLAN id", vid, argv[0],
                                     NSM_VLAN_CLI_MIN, NSM_VLAN_CLI_MAX);

  l2_range_parser_init (argv[1], &par);

  if ( (ret = l2_range_parser_validate (&par, cli, nsm_vlan_validate_vid))
       != CLI_SUCCESS)
    return ret;

  while ( (curr = l2_range_parser_get_next ( &par, &r_err) ) )
    {
      if ( r_err != RANGE_PARSER_SUCCESS )
        {
          cli_out (cli, "%% Invalid Input %s \n",
                                l2_range_parse_err_to_str (r_err) );
          return CLI_ERROR;
        }

      /* Get the pvid and check if it is within range (2-4096).  */
      CLI_GET_INTEGER_RANGE ("VLAN id", pvid, curr, NSM_VLAN_CLI_MIN,
                                                       NSM_VLAN_CLI_MAX);

      ret = nsm_pvlan_api_switchport_mapping_clear (ifp, vid, pvid);
      if (ret < 0)
        {
          nsm_pvlan_cli_process_result (cli, ret);
          l2_range_parser_deinit (&par);
          return CLI_ERROR;
        }

    }

  l2_range_parser_deinit (&par);

  return CLI_SUCCESS;

}

CLI (no_nsm_pvlan_switchport_mapping,
     no_nsm_pvlan_switchport_mapping_cli,
     "no switchport private-vlan mapping",
     "CLI_NO_STR",
     "Set the mode of the Layer2 interface",
     "private-vlan",
     "mapping")
{
  int ret;
  struct interface *ifp = cli->index;

  NSM_BRIDGE_CHECK_L2_PROPERTY(cli, ifp);
  NSM_BRIDGE_CHECK_BRIDGE_PROPERTY(cli, ifp);

  ret = nsm_pvlan_api_switchport_mapping_clear_all (ifp);

  if (ret < 0)
    {
      nsm_pvlan_cli_process_result (cli, ret);
      return CLI_ERROR;
    }

  return CLI_SUCCESS;
}

CLI (show_nsm_pvlan_bridge,
     show_nsm_pvlan_bridge_cmd,
     "show vlan private-vlan bridge <1-32>",
     CLI_SHOW_STR,
     "Display VLAN information",
     "private-vlan information",
     NSM_BRIDGE_STR,
     NSM_BRIDGE_NAME_STR)
{
  int brid;

  /* Get the brid and check if it is within range. */
  CLI_GET_INTEGER_RANGE ("Bridge group", brid, argv[0], 1, 32);

  nsm_all_pvlan_show(cli, argv[0]);

  return CLI_SUCCESS;
}


/* VLAN config write. */
int
nsm_pvlan_config_write (struct cli *cli)
{
  struct apn_vr *vr = cli->vr;
  struct nsm_master *nm = vr->proto;
  struct nsm_bridge_master *master = nm->bridge;
  struct nsm_bridge *bridge;
  struct avl_node *rn;
  struct nsm_vlan *vlan;
  int write = 0;
  int first = 1;
  u_int16_t secondary_vid = 0;

  if (! master)
    return 0;

  bridge = master->bridge_list;
  if (! bridge)
    return 0;

  while (bridge)
    {
      if ( nsm_bridge_is_vlan_aware(master, bridge->name) != PAL_TRUE )
        {
          bridge = bridge->next;
          continue;
        }
      if (bridge->vlan_table != NULL)
        for (rn = avl_top (bridge->vlan_table); rn; rn = avl_next (rn))
          if ((vlan = rn->info) && (vlan->vid != NSM_VLAN_DEFAULT_VID)
               && (vlan->type & NSM_VLAN_STATIC))
            {
              if (first)
                {
                  cli_out (cli, "!\n");
                  write++;
                  first = 0;
                }
             if (!vlan->pvlan_configured)
              continue;

              if (vlan->pvlan_type == NSM_PVLAN_PRIMARY)
                cli_out (cli, " private-vlan %d primary bridge %s \n",
                                  vlan->vid, bridge->name);
              if (vlan->pvlan_type == NSM_PVLAN_ISOLATED)
                cli_out (cli, " private-vlan %d isolated bridge %s\n",
                                  vlan->vid, bridge->name);
              if (vlan->pvlan_type == NSM_PVLAN_COMMUNITY)
                cli_out (cli, " private-vlan %d community bridge %s \n",
                                  vlan->vid, bridge->name);

              write++;

            }

       if (bridge->vlan_table != NULL)
         for (rn = avl_top (bridge->vlan_table); rn; rn = avl_next (rn))
          if ((vlan = rn->info) && (vlan->vid != NSM_VLAN_DEFAULT_VID)
              && (vlan->type & NSM_VLAN_STATIC))
            {
              if (first)
                {
                  cli_out (cli, "!\n");
                  write++;
                  first = 0;
                }

              if (!vlan->pvlan_configured)
              continue;

              if (vlan->pvlan_type == NSM_PVLAN_PRIMARY)
                {
                  for (secondary_vid = 2; secondary_vid < NSM_VLAN_MAX; secondary_vid++)
                    {
                      if (NSM_VLAN_BMP_IS_MEMBER (vlan->pvlan_info.secondary_vlan_bmp,
                                                  secondary_vid))
                        {
                           cli_out (cli, " private-vlan %d association add %d bridge %s \n",
                                    vlan->vid, secondary_vid,  bridge->name);
                           write++;
                        }
                    }
                }
            }

      bridge = bridge->next;
    }

  return write;
}

int
nsm_pvlan_if_config_write (struct cli *cli, struct interface *ifp)
{
  struct nsm_if *zif;
  struct nsm_vlan_port *port;
  struct nsm_bridge_port *br_port = NULL;
  u_int16_t secondary_vid;


  zif = (struct nsm_if *)ifp->info;
  if (! zif)
    return -1;

  if (zif->bridge && zif->switchport)
    {
      br_port = zif->switchport;
      port = &(br_port->vlan_port);

      if (!port->pvlan_configured)
        return 0;
      if (port->pvlan_mode == NSM_PVLAN_PORT_MODE_HOST)
        {
          cli_out (cli, " switchport mode private-vlan host \n");
          if (port->pvid > NSM_VLAN_DEFAULT_VID)
            {
              if (port->pvlan_port_info.primary_vid !=0)
                {
                  cli_out (cli, " switchport private-vlan host-association %d add %d \n",
                                port->pvlan_port_info.primary_vid, port->pvid);
                }
            }
        }
      if (port->pvlan_mode == NSM_PVLAN_PORT_MODE_PROMISCUOUS)
        {
          cli_out (cli, " switchport mode private-vlan promiscuous \n");
          if (port->pvid > NSM_VLAN_DEFAULT_VID)
            {
              for (secondary_vid = 1; secondary_vid < NSM_VLAN_MAX; secondary_vid++)
                {
                  if (NSM_VLAN_BMP_IS_MEMBER
                         (port->pvlan_port_info.secondary_vlan_bmp, secondary_vid))
                    {
                      cli_out (cli, " switchport private-vlan mapping %d add %d \n",
                                   port->pvid, secondary_vid);
                    }
                }
            }
        }
      }
  return 0;
}

void
nsm_port_pvlan_show (struct cli *cli, struct interface *ifp)
{
  struct nsm_if *zif = NULL;
  struct nsm_vlan_port *vlan_port = NULL;
  struct nsm_bridge_port *br_port = NULL;
  u_int16_t secondary_vid;
  int list_count = 0;

  if ( (!cli) || (!ifp) )
    return;

  if ( !(zif = (struct nsm_if *)ifp->info) )
    return;

  if ( !(br_port = zif->switchport) )
    return;

  if ( !(vlan_port = &br_port->vlan_port) )
    {
      return;
    }
  if ( !(vlan_port->pvlan_configured))
    return;

  if (vlan_port->pvlan_mode == NSM_PVLAN_PORT_MODE_HOST)
    {
      cli_out (cli, "Private vlan mode: private-vlan host \n");
    }
  else if (vlan_port->pvlan_mode == NSM_PVLAN_PORT_MODE_PROMISCUOUS)
    {
      cli_out (cli, "Private vlan mode: private-vlan promiscuous \n");
    }
  if (vlan_port->pvlan_mode == NSM_PVLAN_PORT_MODE_HOST)
    {
      cli_out (cli, "private-vlan host-association:%7u%8s%7u\n",
                     vlan_port->pvlan_port_info.primary_vid, " ",
                                                   vlan_port->pvid);
      cli_out (cli, "private-vlan mapping: None \n");
    }
  else if (vlan_port->pvlan_mode == NSM_PVLAN_PORT_MODE_PROMISCUOUS)
    {
      cli_out  (cli, "private-vlan host-association: None \n");
      cli_out  (cli, "private-vlan mapping:%9s%7u", " ", vlan_port->pvid);
      list_count++;
      for (secondary_vid = 2; secondary_vid <= NSM_VLAN_MAX; secondary_vid++)
        {
          if (NSM_VLAN_BMP_IS_MEMBER
                 (vlan_port->pvlan_port_info.secondary_vlan_bmp, secondary_vid))
            {
              cli_out (cli, "%8s%7u", " ", secondary_vid);
              list_count++;
            }
          if ((list_count % 4)==0)
            {
              cli_out (cli, "\n");
            }
        }
        cli_out (cli, "\n");
      }
}

void nsm_pvlan_show_primary_pvlan (struct cli *cli, struct nsm_bridge *bridge,
                                     struct nsm_vlan *vlan)
{
  nsm_vid_t secondary_vid;
  struct avl_node *rn;
  struct nsm_vlan p;
  struct nsm_vlan *secondary_vlan;
  int secondary_exists = 0;

  if ( (!cli) || (!vlan) )
    return;

  for (secondary_vid = 2; secondary_vid <= NSM_VLAN_MAX; secondary_vid++)
    {
      if (NSM_VLAN_BMP_IS_MEMBER(vlan->pvlan_info.secondary_vlan_bmp,
                                                           secondary_vid))
        {
          NSM_VLAN_SET (&p, secondary_vid);
          rn =  avl_search (bridge->vlan_table, (void *)&p);
          if (rn && rn->info)
            {
              secondary_vlan = rn->info;
              cli_out (cli, " %7u%7s%9u%7s%11s\n",
                        vlan->vid, " ", secondary_vid, " ",
                        ((secondary_vlan->pvlan_type == NSM_PVLAN_ISOLATED)
                            ? "isolated" : "community"));
              secondary_exists = 1;
            }
        }
    }

  if (!secondary_exists)
    {
      cli_out (cli, " %7u%7s%9s%7s%11s\n", vlan->vid," "," "," ", "primary");
    }
}

void nsm_pvlan_show_secondary_pvlan (struct cli *cli, struct nsm_vlan *vlan)
{

  if (vlan->pvlan_type == NSM_PVLAN_ISOLATED)
    {
      if (vlan->pvlan_info.vid.primary_vid == 0)
        {
          cli_out (cli, " %7s%7s%9u%7s%11s \n",
                         " "," ",vlan->vid," ", "isolated");
        }
     }
  if (vlan->pvlan_type == NSM_PVLAN_COMMUNITY)
    {
      if (vlan->pvlan_info.vid.primary_vid == 0)
        {
           cli_out (cli, " %7s%7s%9u%7s%11s \n",
                         " "," ",vlan->vid," ", "community");
        }
     }
}

void
nsm_all_pvlan_show(struct cli *cli, char *brname)
{
  struct nsm_master *nm = NULL;
  struct nsm_bridge_master *master = NULL;
  struct nsm_bridge *bridge = NULL;
  struct avl_node *rn = NULL;
  struct nsm_vlan *vlan = NULL;

  if ((!cli) || (!brname))
    return;

  nm = cli->vr->proto;
  
  if (nm)
    master = nsm_bridge_get_master (nm);

  if (master == NULL)
    return;

  if ( !(bridge = nsm_lookup_bridge_by_name(master, brname)) )
    return;

  if ( !(bridge->vlan_table) )
    return;

  cli_out (cli, " PRIMARY       SECONDARY          TYPE ");
  cli_out (cli, " %7sINTERFACES\n"," ");
  cli_out (cli, " -------        ---------       ----------");
  cli_out (cli, " %4s ----------\n"," ");



  if (bridge->vlan_table != NULL)
    for (rn = avl_top (bridge->vlan_table); rn; rn = avl_next (rn))
      {
        vlan = rn->info;
        if (vlan)
          {
            if (vlan->pvlan_configured)
              {
                if (vlan->pvlan_type == NSM_PVLAN_PRIMARY)
                  nsm_pvlan_show_primary_pvlan (cli, bridge, vlan);
                if ((vlan->pvlan_type == NSM_PVLAN_ISOLATED)
                     || (vlan->pvlan_type == NSM_PVLAN_COMMUNITY))
                  nsm_pvlan_show_secondary_pvlan (cli, vlan);
              }
          }
      }
}

void
nsm_pvlan_cli_init (struct cli_tree *ctree)
{
  cli_install_default (ctree, VLAN_MODE);

   /* Private VLAN config mode CLIs */
  cli_install (ctree, VLAN_MODE, 
               &nsm_private_vlan_create_cli);
  cli_install (ctree, VLAN_MODE, 
               &no_nsm_private_vlan_create_cli);
  cli_install (ctree, VLAN_MODE, 
               &nsm_private_vlan_associate_add_cli);
  cli_install (ctree, VLAN_MODE,
               &nsm_private_vlan_associate_remove_cli);
  cli_install (ctree, VLAN_MODE,
               &no_private_vlan_associate_cli);

   /* Private VLAN Interface mode CLIs */
  cli_install (ctree, INTERFACE_MODE, 
               &nsm_pvlan_switchport_mode_pvlan_cli);
  cli_install (ctree, INTERFACE_MODE,
               &no_nsm_pvlan_switchport_mode_pvlan_cli);
  cli_install (ctree, INTERFACE_MODE,
               &nsm_pvlan_switchport_host_association_add_cli);
  cli_install (ctree, INTERFACE_MODE,
               &no_nsm_pvlan_switchport_host_association_cli);
  cli_install (ctree, INTERFACE_MODE, 
               &nsm_pvlan_switchport_mapping_add_cli);
  cli_install (ctree, INTERFACE_MODE,
               &nsm_pvlan_switchport_mapping_remove_cli);
  cli_install (ctree, INTERFACE_MODE, 
               &no_nsm_pvlan_switchport_mapping_cli);

  cli_install (ctree, EXEC_MODE, 
               &show_nsm_pvlan_bridge_cmd);
}

#endif /* HAVE_PVLAN */
