/* Copyright (C) 2004  All Rights Reserved.  */

#include "pal.h"

#ifdef HAVE_VLAN
#ifdef HAVE_L2LERN
#include "cli.h"
#include "avl_tree.h"
#include "linklist.h"
#include "snprintf.h"
#include "nsm_message.h"

#include "nsmd.h"
#include "nsm_server.h"
#include "nsm_bridge.h"
#include "nsm_interface.h"
#include "nsm_bridge_cli.h"
#include "nsm_vlan_cli.h"
#include "l2_vlan_port_range_parser.h"

#include "nsm_vlan.h"
#include "nsm_vlan_access_list.h"
#include "nsm_vlan_access_api.h"

#define MAX_VLANS_SHOW_PER_LINE 7

CLI (nsm_vlan_access_map,
     nsm_vlan_access_map_cmd,
     "vlan access-map WORD <1-65535>",
     "Configure VLAN parameters",
     "Configure VLAN Access Map",
     "Name for the VLAN Access Map",
     "Sequence to insert to/delete from existing access-map entry")
{
  struct nsm_master *nm = cli->vr->proto;
  struct nsm_vlan_access_list *vmapl = NULL;

  if (!nm)
    {
      cli_out (cli, "%% No nsm master address\n");
      return CLI_ERROR;
    }

  if (argv[0] == NULL)
    {
      cli_out (cli, "%% No VLAN access map name %s\n", argv[0]);
      return CLI_ERROR;
    }

  /* Write code to check whether the vlan access map is applied or not
     If it is attached to any of the vlan(s) then we shd not proceed*/
  if (nm->vmap == NULL)
    {
        nsm_vmap_list_master_init(nm);
    }


  vmapl = nsm_vmap_list_get (nm->vmap, argv[0]);
 
    if (! vmapl)
    {
      cli_out (cli, "%% Fail to get vlan access-map \n");
      return CLI_ERROR;
    }

  cli->index = vmapl;
  cli->mode = VLAN_ACCESS_MODE;
  return CLI_SUCCESS;

}

CLI (nsm_no_vlan_access_map,
     nsm_no_vlan_access_map_cmd,
     "no vlan access-map WORD <1-65535>",
     CLI_NO_STR,
     "Configure VLAN parameters",
     "Configure VLAN Access Map",
     "Name for the VLAN Access Map",
     "Sequence to insert to/delete from existing access-map entry")
{
  struct nsm_master *nm = cli->vr->proto;
  struct nsm_vlan_access_list *vmapl = NULL;
  struct mac_acl *access;
  int ret;

  if (!nm)
    {
      cli_out (cli, "%% No nsm master address\n");
      return CLI_ERROR;
    }

  if (argv[0] == NULL)
    {
      cli_out (cli, "%% No VLAN access map name %s\n", argv[0]);
      return CLI_ERROR;
    }

  vmapl = nsm_vmap_list_lookup (nm->vmap, argv[0]);

  if (! vmapl)
    return CLI_ERROR;

  /* Write code to check whether the vlan access map is applied or not
     If it is attached to any of the vlan(s) then we shd not proceed*/

  if (vmapl->acl_name)
    {
      access = mac_acl_lookup (nm->mac_acl, vmapl->acl_name);
      if (! access)
        {
          cli_out (cli, "%% No access-list: %s\n", argv[0]);
          return CLI_ERROR;
        }

      ret = nsm_delete_mac_acl_name_from_vmap (vmapl, vmapl->acl_name);
      if (ret < 0)
        {
          cli_out (cli, "%% Failed to detach ACL(%s) from class-map\n", argv[0]);
          return CLI_ERROR;
        }
      mac_acl_unlock (access);
    }
    
  nsm_vmap_list_delete(vmapl);

  return CLI_SUCCESS;
}

CLI (nsm_show_vlan_access_map,
     nsm_show_vlan_access_map_cmd,
     "show vlan access-map",
     CLI_SHOW_STR,
     "VLAN parameters",
     "Show VLAN Access Map")
{
  struct nsm_master *nm = cli->vr->proto;

  vlan_access_map_show (cli, nm->vmap);
  return CLI_SUCCESS;

}


CLI (vlan_action,
     vlan_action_cmd,
     "action (forward|discard)",
     "Configure VLAN Access Map Action",
     "Forward",
     "Discard")
{
  struct nsm_vlan_access_list *vmapl = cli->index;

  if (!vmapl)
    {
      cli_out (cli, "%% No vlan access-map list address\n");
      return CLI_ERROR;
    }

  if (! pal_strncmp (argv[0], "f", 1))
    {
      vmapl->type = VLAN_ACCESS_ACTION_FORWARD;
    }
  else if (! pal_strncmp (argv[0], "d", 1))
    {
      vmapl->type = VLAN_ACCESS_ACTION_DISCARD;
    }
  else
    {
      cli_out (cli, "%% Specify only forward | discard\n");
      return CLI_ERROR;
    }

  return CLI_SUCCESS;
}

CLI (vlan_match,
     vlan_match_cmd,
     "match mac WORD",
     "Configure VLAN Access Map Match",
     "Configure VLAN Access Map Match",
     "Mac access-list name")
{
  struct nsm_master *nm = cli->vr->proto;
  struct nsm_vlan_access_list *vmapl = cli->index;
  struct mac_acl *access;
  int ret;

  if (!vmapl)
    {
      cli_out (cli, "%% No vlan access-map list address\n");
      return CLI_ERROR;
    }

  if (vmapl->acl_name)
    {
      cli_out (cli, "%% Only one match command is supported per vlan map \n");
      return CLI_ERROR;
    }

  access = mac_acl_lookup (nm->mac_acl, argv[0]);
  if (! access)
    {
      cli_out (cli, "%% No access-list: %s\n", argv[0]);
      return CLI_ERROR;
    }

  ret = nsm_insert_mac_acl_name_into_vmap (vmapl, argv[0]);
  if (ret < 0)
    {
      cli_out (cli, "%% Failed to attach ACL(%s) into vlan-map\n", argv[0]);
      return CLI_ERROR;
    }

  mac_acl_lock (access);

  return CLI_SUCCESS;
}

CLI (vlan_no_match,
     vlan_no_match_cmd,
     "no match mac WORD",
     CLI_NO_STR,
     "Configure VLAN Access Map Match",
     "Configure VLAN Access Map Match",
     "Mac access-list name")
{
  struct nsm_master *nm = cli->vr->proto;
  struct nsm_vlan_access_list *vmapl = cli->index;
  struct mac_acl *access;
  int ret;

  if (!vmapl)
    {
      cli_out (cli, "%% No vlan access-map list address\n");
      return CLI_ERROR;
    }

  access = mac_acl_lookup (nm->mac_acl, argv[0]);
  if (! access)
    {
      cli_out (cli, "%% No access-list: %s\n", argv[0]);
      return CLI_ERROR;
    }

  ret = nsm_delete_mac_acl_name_from_vmap (vmapl, argv[0]);
  if (ret < 0)
    {
      cli_out (cli, "%% Failed to detach ACL(%s) from class-map\n", argv[0]);
      return CLI_ERROR;
    }

  mac_acl_unlock (access);
 
  return CLI_SUCCESS;

}


CLI (nsm_vlan_filter,
     nsm_vlan_filter_cmd,
     "vlan filter WORD <2-4094>",
     "Configure VLAN parameters",
     "Configure VLAN Access Map Filter",
     "VLAN Access Map Name",
     "VLAN id")
{
  struct nsm_master *nm = cli->vr->proto;
  struct nsm_vlan_access_list *vmapl = NULL;
  u_int16_t vid;
  int action;
  int ret;

  CLI_GET_INTEGER_RANGE ("VLAN id", vid, argv[1], 2, 4094);

  vmapl = nsm_vmap_list_lookup (nm->vmap, argv[0]);

  if (! vmapl)
    return CLI_ERROR;

  if (! vmapl->acl_name)
    return CLI_ERROR;

  ret = check_bridge_mac_acl(nm);
  if (ret)
    {
      cli_out (cli, "%%MAC access group configured in one of the interface\n");
      return CLI_ERROR;
    }

  action = NSM_VLAN_ACC_MAP_ADD;

  nsm_set_vmapl_vlan(nm, vmapl, vid);

  ret = nsm_vlan_hal_set_access_map ( nm, vmapl, vid, action);

  if (ret < 0)
    {
      nsm_unset_vmapl_vlan(nm, vmapl, vid);
      cli_out (cli, "%%Failed to create vlan filter\n");
      return CLI_ERROR;
    }

  return CLI_SUCCESS;
}

CLI (nsm_no_vlan_filter,
     nsm_no_vlan_filter_cmd,
     "no vlan filter WORD <2-4094>",
     CLI_NO_STR,
     "Configure VLAN parameters",
     "Configure VLAN Access Map Filter",
     "VLAN Access Map Name",
     "VLAN id")
{
  struct nsm_master *nm = cli->vr->proto;
  struct nsm_vlan_access_list *vmapl = NULL;
  u_int16_t vid;
  int action;

  CLI_GET_INTEGER_RANGE ("VLAN id", vid, argv[1], 2, 4094);

  vmapl = nsm_vmap_list_lookup (nm->vmap, argv[0]);

  if (! vmapl)
    return CLI_ERROR;

  if (! vmapl->acl_name)
    return CLI_ERROR;

  //ret = check_bridge_mac_acl(nm);
  //if (ret)
    //return CLI_ERROR;

  action = NSM_VLAN_ACC_MAP_DELETE;

  nsm_unset_vmapl_vlan(nm, vmapl, vid);

  nsm_vlan_hal_set_access_map ( nm, vmapl, vid, action);

  return CLI_SUCCESS;
}


CLI (nsm_show_vlan_filter,
     nsm_show_vlan_filter_cmd,
     "show vlan filter",
     CLI_SHOW_STR,
     "VLAN parameters",
     "Show VLAN Access Map Filter")
{
  struct nsm_master *nm = cli->vr->proto;

  vlan_filter_show (cli, nm);
  return CLI_SUCCESS;
}

int
nsm_vmap_config_write (struct cli *cli)
{
  struct nsm_master *nm = cli->vr->proto;
  struct nsm_vlan_access_master *master = nm->vmap;
  struct nsm_vlan_access_list *vmapl = NULL;

  if (master == NULL)
    return 0;

  for (vmapl = master->head; vmapl; vmapl = vmapl->next)
    {
      if(! vmapl->name)
        continue;

      cli_out (cli, "vlan access-map %s 1 \n", vmapl->name);
      if (vmapl->acl_name)
        cli_out (cli, " match mac %s\n", vmapl->acl_name);
      if ( vmapl->type == VLAN_ACCESS_ACTION_FORWARD) 
        cli_out (cli, " action %s", "forward");
      else if (vmapl->type == VLAN_ACCESS_ACTION_DISCARD)
      cli_out (cli, " action %s", "discard");
      cli_out (cli, "\n");
    }
  return 0;

}

int
nsm_vlan_filter_config_write (struct cli *cli)
{
  int write = 0;
  struct avl_node *rn;
  struct nsm_vlan *vlan;
  struct nsm_bridge *bridge = NULL;
  struct nsm_master *nm = cli->vr->proto;
  struct nsm_vlan_access_list *vmapl = NULL;

  bridge = nsm_get_first_bridge ( nm->bridge);

  if(! bridge)
   return 0;
  if ( (bridge->type != NSM_BRIDGE_TYPE_STP_VLANAWARE) &&
       (bridge->type != NSM_BRIDGE_TYPE_RSTP_VLANAWARE) &&
       (bridge->type != NSM_BRIDGE_TYPE_MSTP) )
    {
      return 0;
    }

  cli_out (cli, "!\n");
  write++;

  if ( bridge->vlan_table != NULL)
    for (rn = avl_top (bridge->vlan_table); rn; rn = avl_next (rn))
     {
        if (rn)
          {
            vlan = rn->info;
            if (! vlan)
              continue;
            if ( vlan->vlan_filter)
              {
                vmapl = vlan->vlan_filter;
                cli_out (cli, "vlan filter %s %d\n", vmapl->name, vlan->vid);
                write++;
              }
          }
      }
  return write;
}

void
nsm_vlan_access_cli_init (struct cli_tree *ctree)
{
   cli_install_config (ctree, VLAN_ACCESS_MODE, nsm_vmap_config_write);
   cli_install_config (ctree, VLAN_FILTER_MODE, nsm_vlan_filter_config_write);
   cli_install_default (ctree, VLAN_ACCESS_MODE);
   cli_install (ctree, CONFIG_MODE, &nsm_vlan_access_map_cmd);
   cli_install (ctree, CONFIG_MODE, &nsm_no_vlan_access_map_cmd);
   cli_install (ctree, CONFIG_MODE, &nsm_vlan_filter_cmd);
   cli_install (ctree, CONFIG_MODE, &nsm_no_vlan_filter_cmd);
   cli_install (ctree, EXEC_MODE, &nsm_show_vlan_access_map_cmd);
   cli_install (ctree, EXEC_MODE, &nsm_show_vlan_filter_cmd);
   cli_install_imi (ctree, VLAN_ACCESS_MODE, PM_NSM, PRIVILEGE_NORMAL, 0, &vlan_action_cmd);
   cli_install_imi (ctree, VLAN_ACCESS_MODE, PM_NSM, PRIVILEGE_NORMAL, 0, &vlan_match_cmd);
   cli_install_imi (ctree, VLAN_ACCESS_MODE, PM_NSM, PRIVILEGE_NORMAL, 0, &vlan_no_match_cmd);
}
#endif /* HAVE_L2LERN */
#endif /* HAVE_VLAN */
