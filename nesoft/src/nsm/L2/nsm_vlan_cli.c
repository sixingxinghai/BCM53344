/* Copyright (C) 2004  All Rights Reserved.  */

#include "pal.h"

#ifdef HAVE_VLAN

#include "cli.h"
#include "table.h"
#include "ptree.h"
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
#include "nsm_l2_qos.h"

#ifdef HAVE_PVLAN
#include "nsm_pvlan_cli.h"
#endif /* HAVE_PVLAN */

#ifdef HAVE_PROVIDER_BRIDGE
#include "nsm_pro_vlan.h"
#endif /* HAVE_PROVIDER_BRIDGE */

#ifdef HAVE_PBB_TE
#include "nsm_pbb_te_cli.h"
#endif /* HAVE_PBB_TE */

#define MAX_VLANS_SHOW_PER_LINE 7

static u_int8_t *def_bridge_name = "0";

struct nsm_show_vlan_arg
{
   struct nsm_bridge *bridge;
   int type;
   nsm_vid_t vid;

   u_char flags;
   #define NSM_SHOW_VLAN_VIDCMD       (1 << 0)
   #define NSM_SHOW_VLAN_BANNER       (1 << 1)
   #define NSM_SHOW_CVLAN_DONE        (1 << 2)
};

#ifndef HAVE_CUSTOM1
/* Forward declarations for static functions */
static void nsm_port_vlan_show(struct cli *cli, struct interface *ifp);

static void nsm_all_port_vlan_show(struct cli *cli, char *brname);

static int nsm_vlan_show(struct cli *cli, struct nsm_vlan *vlan);

static void nsm_all_vlan_show(struct cli *cli, char *brname,
                              int type, nsm_vid_t vid);

static void
nsm_show_vid_static_ports (struct cli *cli, struct nsm_vlan *vlan);

static void
nsm_vlan_show_static_ports (struct cli *cli, char *brname, nsm_vid_t vid);

/* End forward declarations for static functions */
#endif /* ifndef HAVE_CUSTOM1 */

int
nsm_vlan_cli_return (struct cli *cli, int result,
                     struct interface  *ifp,
                     char *bridge_name,
                     u_int16_t vid)
{
  int ret;

  ret = CLI_ERROR;
  switch (result)
  {
    case NSM_VLAN_ERR_BRIDGE_NOT_FOUND:
      cli_out (cli, "Bridge Not Found %s\n", bridge_name ?
                bridge_name :"Default");
      break;
    case NSM_VLAN_ERR_NOMEM:
      cli_out (cli, "%% Out of Memory\n");
      break;
    case NSM_VLAN_ERR_INVALID_MODE:
      cli_out (cli, "%% Invalid switchport mode %s\n", ifp ? ifp->name: " ");
      break;
    case NSM_VLAN_ERR_VLAN_NOT_FOUND:
      cli_out (cli, "%% VLAN %d Not Found \n", vid);
      break;
    case NSM_VLAN_ERR_MODE_CLEAR:
      cli_out (cli, "%% Error clearing output VLAN of previous mode\n");
      break;
    case NSM_VLAN_ERR_MODE_SET:
      cli_out (cli, "%% Error Setting the port mode \n");
      break;
    case NSM_VLAN_ERR_GENERAL:
      cli_out (cli, "%% General Error \n");
      break;
    case NSM_VLAN_ERR_IFP_NOT_BOUND:
      cli_out (cli, "%% Interface not bound to any bridge-group\n");
      break;
    case NSM_VLAN_ERR_BRIDGE_NOT_VLAN_AWARE:
      cli_out (cli, "%% Bridge Not VLAN aware \n");
      break;
    case NSM_VLAN_ERR_VLAN_NOT_IN_PORT:
      cli_out (cli, "%% Port not a member of vlan \n");
      break;
    case NSM_VLAN_ERR_CONFIG_PVID_TAG:
      cli_out (cli, "%% Default vlan can't be tagged\n");
      break;
#ifdef HAVE_PROVIDER_BRIDGE
    case NSM_VLAN_ERR_SVLAN_NOT_FOUND:
      cli_out (cli, "%% SVLAN %d Not Found \n", vid);
      break;
    case NSM_VLAN_ERR_CVLAN_REG_EXIST:
      cli_out (cli, "%% CVLAN Registration Exists on the table\n");
      break;
    case NSM_VLAN_ERR_CVLAN_PORT_REG_EXIST:
      cli_out (cli, "%% CVLAN Registration Exists on the port\n");
      break;
    case NSM_PRO_VLAN_ERR_SCIDX_ALLOC_FAIL:
      cli_out (cli, "%% Failed to allocated Switching Context ID\n");
      break;
    case NSM_PRO_VLAN_ERR_SWCTX_HAL_ERR:
      cli_out (cli, "%% Switching Context HAL Call error\n");
      break;
    case NSM_PRO_VLAN_ERR_INVALID_MODE:
      cli_out (cli, "%% Mode Invalid for the command\n");
      break;
    case NSM_PRO_VLAN_ERR_CVLAN_REGIS_EXISTS:
      cli_out (cli, "%% CVLAN Registration Exists on the table\n");
      break;
    case NSM_PRO_VLAN_ERR_CVLAN_REGIS_NOT_FOUND:
      cli_out (cli, "%% CVLAN Registration table not found\n");
      break;
    case NSM_PRO_VLAN_ERR_CVLAN_REGIS_HAL_ERR:
      cli_out (cli, "%% CVLAN Registration HAL error\n");
      break;
    case NSM_PRO_VLAN_ERR_CVLAN_PORT_NOT_FOUND:
      cli_out (cli, "%% Port not a member of vlan \n");
      break;
    case NSM_PRO_VLAN_ERR_CVLAN_MAP_EXISTS:
      cli_out (cli, "%% CVLAN Registration Exists on the port\n");
      break;
    case NSM_PRO_VLAN_ERR_VLAN_TRANS_HAL_ERR:
      cli_out (cli, "%% CVLAN Translation HAL error\n");
      break;
    case NSM_PRO_VLAN_ERR_CVLAN_MAP_ERR:
      cli_out (cli, "%% CVLAN Map Contradicts the service attribute \n");
      break;
    case NSM_VLAN_ERR_NOT_EDGE_BRIDGE:
      cli_out (cli, "%% Bridge not a Provider Edge Bridge \n");
      break;
    case NSM_ERR_CROSSED_MAX_EVC:
      cli_out (cli, "%% EVC count in Registration table is greater than max evc.\n");
      break;
    case NSM_PRO_VLAN_ERR_SVLAN_SERVICE_ATTR:
      cli_out (cli, "%% Service attribute is not configured to Bundling\n");
      break;
    case NSM_PRO_VLAN_ERR_DEFAULT_EVC_NOT_SET:
      cli_out (cli, "%% Default EVC is not\n");
      break;
#endif /* HAVE_PROVIDER_BRIDGE */
#ifdef HAVE_VRRP
    case NSM_VLAN_ERR_VRRP_BIND_EXISTS:
      cli_out (cli, "%% Can't delete vlan which is bind to a VRRP instance\n");
      break;
#endif /* HAVE_VRRP */
#if defined HAVE_I_BEB || defined HAVE_B_BEB
    case NSM_VLAN_ERR_PBB_CONFIG_EXIST:
      cli_out (cli, "%% Can't delete vlan as PBB configuration on "
          "VLAN exists\n");
      break;
#endif /* HAVE_I_BEB || HAVE_B_BEB */
    case NSM_VLAN_ERR_IFP_NOT_DELETED:
      cli_out (cli, "%% Error. Vlan (%d) deletion in progress \n", vid);
      break;
 
    case NSM_VLAN_ERR_RESERVED_IN_HW:
      cli_out (cli, "%% Can't add VLAN %d. Reserved in hardware \n", vid);
      break;
#ifdef HAVE_G8031
    case NSM_VLAN_ERR_G8031_CONFIG_EXIST:
      cli_out (cli, "%% Can't delete vlan as G8031 configuration on "
          "VLAN exists\n");
      break;
#endif
#ifdef HAVE_G8032
    case NSM_VLAN_ERR_G8032_CONFIG_EXIST:
      cli_out (cli, "%% Can't delete vlan which is bind to G8032\n");
      break;
#endif
    case NSM_VLAN_ERR_MAX_UNI_PT_TO_PT:
       cli_out (cli, "%% MAX UNI cannot be greater than 2 for pt-to-pt EVC\n");
       break;
     case NSM_VLAN_ERR_MAX_UNI_M2M:
       cli_out (cli, "%% MAX UNI cannot be less than 2 for mpt-to-mpt EVC\n");
       break;
     case NSM_VLAN_ERR_EVC_ID_SET:
       cli_out (cli, "%% evc-id configured is already set\n");
       break;
    default:
      ret = CLI_SUCCESS;
      break;
  }

  return ret;
}

CLI (nsm_vlan_database,
     nsm_vlan_database_cmd,
     "vlan database",
     "Configure VLAN parameters",
     "Configure VLAN database")
{
  cli->mode = VLAN_MODE;
  return CLI_SUCCESS;
}

#ifdef HAVE_G8031

#ifdef HAVE_B_BEB
#ifdef HAVE_I_BEB
/* CLI to configure vlan to EPS group and Bridge of BEB*/
CLI (g8031_i_b_beb_eps_vlan_create,
    g8031_i_b_beb_eps_vlan_create_cmd,
    "g8031 configure vlan eps-id EPS-ID bridge (<1-32> | backbone)",
    "parameters are G.8031 related",
    "EPS configuration",
    "configure vlan members",
    "unique id to identify a EPS protection link",
    "EPS protection group ID",
    NSM_BRIDGE_STR,
    NSM_BRIDGE_NAME_STR,
    "backbone bridge")
{
    int brno                         = 0 ;
    int epsid                        = 0 ;
    struct nsm_bridge *bridge        = NULL;
    struct nsm_master *nm            = cli->vr->proto;
    struct nsm_bridge_master *master = nsm_bridge_get_master (nm);
    struct nsm_msg_g8031_vlanpg *pg_context = NULL;

    if (! master)
      {
        cli_out (cli, "%% Bridge master not configured\n");
        return CLI_ERROR;
      }

    if ( argv[1][0] != 'b')
      {
        CLI_GET_INTEGER_RANGE ("Max bridge groups", brno, argv[1], 1, 32);
        bridge = nsm_lookup_bridge_by_name (master, argv[1]);
      }

    if (! bridge)
      {
        cli_out (cli, "%% Bridge not found \n");
        return CLI_ERROR;
      }
    
    CLI_GET_INTEGER_RANGE ("Max Protection groups", epsid, argv[0], 1, 4094);

    if (!nsm_g8031_find_protection_group(bridge,epsid))
      {
        cli_out (cli, "%% protection group not found \n");
        return CLI_ERROR;
      }

    pg_context = XCALLOC (MTYPE_NSM_G8031_VLAN, sizeof (struct nsm_msg_g8031_vlanpg));

    pal_mem_cpy (pg_context->bridge_name,bridge->name,strlen(bridge->name));
    pg_context->eps_id = epsid;

    cli->index = pg_context;
    cli->mode = G8031_CONFIG_VLAN_MODE;

    return CLI_SUCCESS;
}
#else
/* CLI to configure vlan to EPS group and Bridge of BEB*/
CLI (g8031_beb_eps_vlan_create,
     g8031_beb_eps_vlan_create_cmd,
     "g8031 configure vlan eps-id EPS-ID bridge backbone",
     "parameters are G.8031 related",
     "EPS configuration",
     "configure vlan members",
     "unique id to identify a EPS protection link",
     "EPS protection group ID",
     NSM_BRIDGE_STR,
     "backbone bridge")
{
  int epsid                        = 0 ;
  struct nsm_bridge *bridge        = NULL;
  struct nsm_master *nm            = cli->vr->proto;
  struct nsm_bridge_master *master = nsm_bridge_get_master (nm);
  struct nsm_msg_g8031_vlanpg *pg_context = NULL;

  if (! master)
    {
      cli_out (cli, "%% Bridge master not configured\n");
      return CLI_ERROR;
    }
  
  bridge = nsm_lookup_bridge_by_name (master, argv[1]);

  if (! bridge)
    {
      cli_out (cli, "%% Bridge not found \n");
      return CLI_ERROR;
    }

  CLI_GET_INTEGER_RANGE ("Max Protection groups", epsid, argv[0], 1, 4094);

  if (!nsm_g8031_find_protection_group(bridge,epsid))
    {
      cli_out (cli, "%% protection group not found \n");
      return CLI_ERROR;
    }

  pg_context = XCALLOC (MTYPE_NSM_G8031_VLAN, sizeof (struct nsm_msg_g8031_vlanpg));

  pal_mem_cpy (pg_context->bridge_name,bridge->name,strlen(bridge->name));
  pg_context->eps_id = epsid;

  cli->index = pg_context;
  cli->mode = G8031_CONFIG_VLAN_MODE;

  return CLI_SUCCESS;
}
#endif /* HAVE_I_BEB */
#else
/* CLI to configure vlan to EPS group and Bridge */
CLI (g8031_eps_vlan_create,
     g8031_eps_vlan_create_cmd,
     "g8031 configure vlan eps-id EPS-ID bridge <1-32>",
     "parameters are G.8031 related",
     "EPS configuration",
     "configure vlan members",
     "unique id to identify a EPS protection link",
     "EPS protection group ID",
     NSM_BRIDGE_STR,
     NSM_BRIDGE_NAME_STR)
{
  int brno                         = 0 ;
  int epsid                        = 0 ;
  struct nsm_bridge *bridge        = NULL;
  struct nsm_master *nm            = cli->vr->proto;
  struct nsm_bridge_master *master = nsm_bridge_get_master (nm);
  struct nsm_msg_g8031_vlanpg *pg_context = NULL;
  
  if (! master)
    {
      cli_out (cli, "%% Bridge master not configured\n");
      return CLI_ERROR;
    }

  if (argc > 1)
    {
      CLI_GET_INTEGER_RANGE ("Max bridge groups", brno, argv[1], 1, 32);
      bridge = nsm_lookup_bridge_by_name (master, argv[1]);
    }

  if (! bridge)
    {
      cli_out (cli, "%% Bridge not found \n");
      return CLI_ERROR;
    }

  CLI_GET_INTEGER_RANGE ("Max protection groups", epsid, argv[0], 1, 4094);

  if (!nsm_g8031_find_protection_group(bridge,epsid))
    {
      cli_out (cli, "%% protection group not found \n");
      return CLI_ERROR;
    }

  pg_context = XCALLOC (MTYPE_NSM_G8031_VLAN, sizeof (struct nsm_msg_g8031_vlanpg));

  pal_mem_cpy (pg_context->bridge_name,bridge->name,strlen(bridge->name));
  pg_context->eps_id = epsid;

  cli->index = pg_context;
  cli->mode = G8031_CONFIG_VLAN_MODE;

  return CLI_SUCCESS;
}
#endif  /*  defined HAVE_B_BEB */

/* CLI to configure vlan ID to EPS group.
 * If primary VID,then mention the key word primary */
CLI (g8031_eps_primary_vlan_create,
     g8031_eps_primary_vlan_create_cmd,
     "g8031 vlan VID (primary|)",
     "parameters are G.8031 related",
     "configure vlan members",
     "VLAN ID",
     "Indictaes if the given vlan is primaryvlan  of protection group")
{
  u_int16_t vid                = 0;
  u_int8_t  primary            = 0;
  struct nsm_msg_g8031_vlanpg *pg_context; 
  struct nsm_master *nm        = cli->vr->proto;

  pg_context = (struct nsm_msg_g8031_vlanpg *)cli->index;
  
  CLI_GET_INTEGER_RANGE ("VLAN id", vid, argv[0], NSM_VLAN_DEFAULT_VID,
      NSM_VLAN_CLI_MAX);

  if (argv[1])
    {
      if ( pal_strcmp(argv[1],"primary") != 0 )
        {
          cli_out(cli,"invalid option %s \n", argv[1]);
          return CLI_ERROR;
        }
      primary = 1;
    }
  pg_context->pvid = 1;

  switch(nsm_map_vlans_to_g8031_protection_group(vid,pg_context->bridge_name, \
                                             pg_context->eps_id,primary,nm))
    {
    case RESULT_OK:
      return (CLI_SUCCESS);
    case NSM_BRIDGE_ERR_NOTFOUND:
      cli_out(cli,"bridge %s not found \n",pg_context->bridge_name);
      return (CLI_ERROR);
    case NSM_BRIDGE_ERR_PG_NOTFOUND:
      cli_out(cli,"protection group %d not found \n",pg_context->eps_id);
      return (CLI_ERROR);
    case NSM_BRIDGE_ERR_PG_VLAN_EXISTS:
      cli_out(cli,"Cannot configre vlan %d."  \
              "Vlan already configured to protection group\n",vid);
      return (CLI_ERROR);
    case NSM_VLAN_ERR_VLAN_NOT_FOUND:
      cli_out(cli,"vlan %d does not exists \n",vid);
      return (CLI_ERROR);
    case NSM_VLAN_ERR_IFP_NOT_BOUND:
      cli_out(cli,"vlan not configured on interface \n");
      return (CLI_ERROR);
    case  NSM_PG_VLAN_OPERATION_NOT_ALLOWED:
      cli_out (cli, "Runtime Vlan changes not allowed\n");
      return (CLI_ERROR);
    default:
      break;
    }
  return CLI_ERROR;
}
#endif /* HAVE_G8031 */

#ifdef HAVE_PROVIDER_BRIDGE

CLI (nsm_cvlan_reg_tab,
     nsm_cvlan_reg_tab_cmd,
     "cvlan registration table WORD",
     "Configure C-VLAN parameters",
     "Configure C-VLAN Registration parameters",
     "Configure C-VLAN Registration table",
     "Configure C-VLAN Registration table name")
{
  struct nsm_master *nm;
  struct nsm_bridge *bridge;
  struct nsm_bridge_master *master;
  struct nsm_cvlan_reg_tab *regtab;

  nm = cli->vr->proto;

  master = nsm_bridge_get_master (nm);

  bridge = nsm_get_default_bridge (master);

  if (!bridge)
    {
      cli_out (cli, "No Bridge Found \n");
      return CLI_ERROR;
    }

  regtab = nsm_cvlan_reg_tab_get (bridge, argv [0]);

  if (!regtab)
    {
      cli_out (cli, "Out of Memory \n");
      return CLI_ERROR;
    }

  cli->index = regtab;
  cli->mode = CVLAN_REGIST_MODE;

  return CLI_SUCCESS;
}

CLI (nsm_cvlan_reg_tab_br,
     nsm_cvlan_reg_tab_br_cmd,
     "cvlan registration table WORD bridge <1-32>",
     "Configure C-VLAN parameters",
     "Configure C-VLAN Registration parameters",
     "Configure C-VLAN Registration table",
     "Configure C-VLAN Registration table name",
     "Bridge group commands",
     "Bridge group for bridging")
{
  struct nsm_master *nm;
  struct nsm_bridge *bridge;
  struct nsm_bridge_master *master;
  struct nsm_cvlan_reg_tab *regtab;

  nm = cli->vr->proto;

  master = nsm_bridge_get_master (nm);

  bridge = nsm_lookup_bridge_by_name (master, argv [1]);

  if (!bridge)
    {
      cli_out (cli, "No Bridge Found \n");
      return CLI_ERROR;
    }

  regtab = nsm_cvlan_reg_tab_get (bridge, argv [0]);

  if (!regtab)
    {
      cli_out (cli, "Out of Memory \n");
      return CLI_ERROR;
    }

  cli->index = regtab;
  cli->mode = CVLAN_REGIST_MODE;
  return CLI_SUCCESS;
}

CLI (no_nsm_cvlan_reg_tab,
     no_nsm_cvlan_reg_tab_cmd,
     "no cvlan registration table WORD",
     CLI_NO_STR,
     "Configure C-VLAN parameters",
     "Configure C-VLAN Registration parameters",
     "Configure C-VLAN Registration table",
     "Configure C-VLAN Registration table name")
{
  struct nsm_master *nm;
  struct nsm_bridge *bridge;
  struct nsm_bridge_master *master;

  nm = cli->vr->proto;

  master = nsm_bridge_get_master (nm);

  bridge = nsm_get_default_bridge (master);

  if (!bridge)
    {
      cli_out (cli, "No Bridge Found \n");
      return CLI_ERROR;
    }

  nsm_cvlan_reg_tab_delete (bridge, argv [0]);

  return CLI_SUCCESS;
}

CLI (no_nsm_cvlan_reg_tab_br,
     no_nsm_cvlan_reg_tab_br_cmd,
     "no cvlan registration table WORD bridge <1-32>",
     CLI_NO_STR,
     "Configure C-VLAN parameters",
     "Configure C-VLAN Registration parameters",
     "Configure C-VLAN Registration table",
     "Configure C-VLAN Registration table name",
     "Bridge group commands",
     "Bridge group for bridging")
{
  struct nsm_master *nm;
  struct nsm_bridge *bridge;
  struct nsm_bridge_master *master;

  nm = cli->vr->proto;

  master = nsm_bridge_get_master (nm);

  bridge = nsm_lookup_bridge_by_name (master, argv [1]);

  if (!bridge)
    {
      cli_out (cli, "No Bridge Found \n");
      return CLI_ERROR;
    }

  nsm_cvlan_reg_tab_delete (bridge, argv [0]);

  return CLI_SUCCESS;
}

CLI (nsm_cvlan_svlan_map,
     nsm_cvlan_svlan_map_cmd,
     "cvlan VLAN_ID svlan VLAN_ID",
     "Configure C-VLAN Registration table entry",
     "C-VLAN ID",
     "S-VLAN corresponding to the C-VLAN",
     "S-VLAN ID")
{
  s_int32_t ret;
  u_int16_t cvid;
  u_int16_t svid;
  char *curr = NULL;
  s_int32_t cli_ret;
  l2_range_parser_t par;
  l2_range_parse_error_t r_err;
  struct nsm_cvlan_reg_tab *regtab;

  /* Get the cvid/ svid and check if it is within range (1-4094).  */
  CLI_GET_INTEGER_RANGE ("S-VLAN id", svid, argv[1], NSM_VLAN_DEFAULT_VID,
                         NSM_VLAN_CLI_MAX);

  regtab = cli->index;

  ret = CLI_SUCCESS;
  cli_ret = CLI_SUCCESS;

  l2_range_parser_init (argv[0], &par);

  if ( (ret = l2_range_parser_validate (&par, cli, nsm_vlan_validate_vid1))
       != CLI_SUCCESS)
    return ret;

  while ( (curr = l2_range_parser_get_next ( &par, &r_err) ) )
    {
      if ( r_err != RANGE_PARSER_SUCCESS )
        {
          cli_out (cli, "%% Invalid Input %s \n", l2_range_parse_err_to_str (r_err) );
          return CLI_ERROR;
        }

       CLI_GET_INTEGER_RANGE ("port vlanid", cvid, curr,
                              NSM_VLAN_DEFAULT_VID, NSM_VLAN_CLI_MAX);

       if (!nsm_cvlan_reg_tab_check_membership (regtab, cvid))
         {
           cli_out (cli, "%% One of the port not a member of %d\n", cvid);
           return CLI_ERROR;
         }
    }

  l2_range_parser_deinit (&par);

  l2_range_parser_init (argv[0], &par);

  if ( (ret = l2_range_parser_validate (&par, cli, nsm_vlan_validate_vid1))
       != CLI_SUCCESS)
    return ret;

  while ( (curr = l2_range_parser_get_next ( &par, &r_err) ) )
    {
      if ( r_err != RANGE_PARSER_SUCCESS )
        {
          cli_out (cli, "%% Invalid Input %s \n", l2_range_parse_err_to_str (r_err) );
          return CLI_ERROR;
        }

      CLI_GET_INTEGER_RANGE ("vlanid", cvid, curr,
                             NSM_VLAN_DEFAULT_VID, NSM_VLAN_CLI_MAX);

      if ((ret = nsm_cvlan_reg_tab_entry_add (regtab, cvid, svid)) < 0)
        {
          cli_out (cli, "Unable to set the C-VLAN S-VLAN mapping \n");
          cli_ret = nsm_vlan_cli_return (cli, ret, NULL, NULL, NSM_VLAN_NULL_VID);
        }
    }

  l2_range_parser_deinit (&par);

  return cli_ret;
}

CLI (no_nsm_cvlan_map_range,
     no_nsm_cvlan_map_range_cmd,
     "no cvlan VLAN_ID",
     CLI_NO_STR,
     "Unconfigure C-VLAN Registration table entry",
     "C-VLAN ID")
{
  s_int32_t ret;
  u_int16_t cvid;
  char *curr = NULL;
  s_int32_t cli_ret;
  l2_range_parser_t par;
  l2_range_parse_error_t r_err;
  struct nsm_cvlan_reg_tab *regtab;

  regtab = cli->index;

  ret = CLI_SUCCESS;
  cli_ret = CLI_SUCCESS;

  l2_range_parser_init (argv[0], &par);

  if ( (ret = l2_range_parser_validate (&par, cli, nsm_vlan_validate_vid1))
       != CLI_SUCCESS)
    return ret;

  while ( (curr = l2_range_parser_get_next ( &par, &r_err) ) )
    {
      if ( r_err != RANGE_PARSER_SUCCESS )
        {
          cli_out (cli, "%% Invalid Input %s \n", l2_range_parse_err_to_str (r_err) );
          return CLI_ERROR;
        }

      CLI_GET_INTEGER_RANGE ("vlanid", cvid, curr,
                             NSM_VLAN_DEFAULT_VID, NSM_VLAN_CLI_MAX);

      if ((ret = nsm_cvlan_reg_tab_entry_delete (regtab, cvid)) < 0)
        cli_ret = nsm_vlan_cli_return (cli, ret, NULL, NULL,
                                       NSM_VLAN_NULL_VID);
    }

  l2_range_parser_deinit (&par);

  return cli_ret;
}

CLI (no_nsm_cvlan_map_svid_range,
     no_nsm_cvlan_map_svid_range_cmd,
     "no svlan VLAN_ID",
     CLI_NO_STR,
     "Unconfigure C-VLAN Registration table entry",
     "C-VLAN ID")
{
  s_int32_t ret;
  u_int16_t svid;
  char *curr = NULL;
  s_int32_t cli_ret;
  l2_range_parser_t par;
  l2_range_parse_error_t r_err;
  struct nsm_cvlan_reg_tab *regtab;

  regtab = cli->index;

  ret = CLI_SUCCESS;
  cli_ret = CLI_SUCCESS;

  l2_range_parser_init (argv[0], &par);

  if ( (ret = l2_range_parser_validate (&par, cli, nsm_vlan_validate_vid1))
       != CLI_SUCCESS)
    return ret;

  while ( (curr = l2_range_parser_get_next ( &par, &r_err) ) )
    {
      if ( r_err != RANGE_PARSER_SUCCESS )
        {
          cli_out (cli, "%% Invalid Input %s \n", l2_range_parse_err_to_str (r_err) );
          return CLI_ERROR;
        }

      CLI_GET_INTEGER_RANGE ("port vlanid", svid, curr,
                             NSM_VLAN_DEFAULT_VID, NSM_VLAN_CLI_MAX);

      if ((ret = nsm_cvlan_reg_tab_entry_delete_by_svid (regtab, svid)) < 0)
        cli_ret = nsm_vlan_cli_return (cli, ret, NULL, NULL,
                                       NSM_VLAN_NULL_VID);
    }

  l2_range_parser_deinit (&par);

  return cli_ret;
}

#endif /* HAVE_PROVIDER_BRIDGE */

#ifdef HAVE_CUSTOM1
CLI (interface_vlan,
     interface_vlan_cmd,
     "interface vlan <1-4094>",
     "Select an interface to configure\n",
     "VLAN interface configuration\n",
     "VLAN ID\n")
{
  struct apn_vr *vr = cli->vr;
  struct interface *ifp;
  char ifname[INTERFACE_NAMSIZ + 1];

  zsnprintf (ifname, INTERFACE_NAMSIZ, "vlan %s", argv[0]);

  if (cli->vr->id == 0)
    ifp = ifg_lookup_by_name (&nzg->ifg, ifname);
  else
    ifp = if_lookup_by_name (&vr->ifm, ifname);

  if (! ifp)
    {
      cli_out (cli, "%% No such interface\n");
      return CLI_ERROR;
    }
  cli->index = ifp;
  cli->mode = INTERFACE_MODE;

  return CLI_SUCCESS;
}


CLI (vlan_name,
     vlan_name_cli,
     "vlan <2-4094> name WORD",
     CLI_VLAN_STR,
     "VLAN id",
     "Ascii name of the VLAN",
     "The ascii name of the VLAN")
{
  int ret;
  u_int16_t vid;
  struct nsm_master *nm = cli->vr->proto;
  char ifname[INTERFACE_NAMSIZ + 1];
  struct interface *ifp;
  int add = 0;

  /* Get the vid and check if it is within range (2-4094). */
  CLI_GET_INTEGER_RANGE ("VLAN id", vid, argv[0], 2, 4094);

  zsnprintf (ifname, INTERFACE_NAMSIZ, "%s %d", "vlan", vid);
  ifp =  ifg_lookup_by_name (&nzg->ifg, ifname);
  if (! ifp)
    add = 1;

  /* Check for maximum number of VLAN interface.  */
  if (nm->l2.vlan_num >= nm->l2.vlan_max && add)
    {
      cli_out (cli, "%% VLAN number exceed\n");
      return CLI_ERROR;
    }

  /* Check if VLAN name size does not exceed 16 characters */
  if (pal_strlen (argv[1]) > NSM_VLAN_NAME_MAX)
    {
      cli_out (cli, "%% VLAN-NAME size should not exceed 16\n");
      return CLI_ERROR;
    }

  /* Call NPF API for creating the VLAN.  */
  ret = npf_vlan_add (NULL, argv[1], vid, 1);

  if (ret)
    {
      cli_out (cli, "%% Can't add vlan\n");
      return CLI_ERROR;
    }

  /* Increment current VLAN number.  */
  if (add)
    nm->l2.vlan_num++;

  return CLI_SUCCESS;
}

CLI (vlan_no_name,
     vlan_no_name_cmd,
     "no vlan <2-4094>",
     CLI_NO_STR,
     CLI_VLAN_STR,
     "VLAN id")
{
  int ret;
  u_int16_t vid;
  struct nsm_master *nm = cli->vr->proto;
  char ifname[INTERFACE_NAMSIZ + 1];
  struct interface *ifp;
  int l3 = 0;

  /* Get the vid and check if it is within range (2-4094).*/
  CLI_GET_INTEGER_RANGE ("VLAN id", vid, argv[0], 2, 4094);

  zsnprintf (ifname, INTERFACE_NAMSIZ, "%s %d", "vlan", vid);
  ifp =  ifg_lookup_by_name (&nzg->ifg, ifname);
  if (ifp && ifp->ifc_ipv4)
    l3 = 1;

  /* Call NPF API for deleting the VLAN.  */
  ret = npf_vlan_delete (NULL, vid);
 
  if (ret)
    {
      cli_out (cli, "%% Can't delete vlan\n");
      return CLI_ERROR;
    }

  /* Decrement current VLAN number.  */
  nm->l2.vlan_num--;

  if (l3)
    nm->l2.vlan_l3_num--;

  return CLI_SUCCESS;
}

CLI (vlan_disable,
     vlan_disable_cmd,
     "vlan <2-4094> state disable",
     CLI_VLAN_STR,
     "VLAN id",
     "Operational state of the VLAN",
     "Disable")
{
  u_int16_t vid;

  CLI_GET_INTEGER_RANGE ("VLAN id", vid, argv[0], 2, 4094);

  npf_vlan_suspend (vid);

  return CLI_SUCCESS;
}

CLI (vlan_no_disable,
     vlan_no_disable_cmd,
     "no vlan <2-4094> state disable",
     CLI_NO_STR,
     CLI_VLAN_STR,
     "VLAN id",
     "Operational state of the VLAN",
     "Disable")
{
  u_int16_t vid;

  CLI_GET_INTEGER_RANGE ("VLAN id", vid, argv[0], 2, 4094);

  npf_vlan_active (vid);

  return CLI_SUCCESS;
}

int vlan_config_write (struct cli *);

/* Install VLAN CLIs.  */
void
nsm_vlan_cli_init (struct cli_tree *ctree)
{
  cli_install_config (ctree, VLAN_MODE, vlan_config_write);
  cli_install_default (ctree, VLAN_MODE);
  cli_install (ctree, CONFIG_MODE, &interface_vlan_cmd);
  cli_install (ctree, CONFIG_MODE, &nsm_vlan_database_cmd);
  cli_install (ctree, VLAN_MODE, &vlan_name_cmd);
  cli_install (ctree, VLAN_MODE, &vlan_no_name_cmd);
  cli_install (ctree, VLAN_MODE, &vlan_disable_cmd);
  cli_install (ctree, VLAN_MODE, &vlan_no_disable_cmd);
}

#else /* HAVE_CUSTOM1 */


CLI (nsm_br_cus_vlan_name,
     nsm_br_cus_vlan_name_cmd,
     "vlan "NSM_VLAN_CLI_RNG" type customer bridge <1-32> "
     "name WORD (state (enable|disable)|)",
     CLI_VLAN_STR,
     "VLAN id",
     "VLAN Type",
     NSM_VLAN_CVLAN_STR,
     NSM_BRIDGE_STR,
     NSM_BRIDGE_NAME_STR,
     "Ascii name of the VLAN",
     "The ascii name of the VLAN",
     "Operational state of the VLAN",
     "Enable",
     "Disable")
{
  int ret;
  int brid;
  u_int16_t vid;
  u_int8_t vlan_type;
  struct nsm_master *nm = cli->vr->proto;
  struct nsm_bridge_master *master = nsm_bridge_get_master (nm);

  /* Get the vid and check if it is within range (2-4096).  */
  CLI_GET_INTEGER_RANGE ("VLAN id", vid, argv[0], NSM_VLAN_CLI_MIN, NSM_VLAN_CLI_MAX);

  vlan_type = NSM_VLAN_STATIC | NSM_VLAN_CVLAN;

  /* Get the bridge group number and see if its within range. */
  CLI_GET_INTEGER_RANGE ("Bridge group", brid, argv[1], 1, 32);

  /* Check if VLAN name size does not exceed 16 characters */
  if (pal_strlen (argv[2]) > NSM_VLAN_NAME_MAX)
    {
      cli_out (cli, "%% VLAN-NAME size should not exceed 16\n");
      return CLI_ERROR;
    }

  /* Check for the state. */
  if (argc > 3)
    {
      if (! pal_strncmp (argv[4], "e", 1))
        /* Create the VLAN.  */
        ret = nsm_vlan_add (master, argv[1], argv[2], vid,
                            NSM_VLAN_ACTIVE, vlan_type);
      else
        ret = nsm_vlan_add (master, argv[1], argv[2], vid,
                            NSM_VLAN_DISABLED, vlan_type);
    }
  else
    ret = nsm_vlan_add (master, argv[1], argv[2], vid,
                        NSM_VLAN_ACTIVE, vlan_type);

  if (ret < 0)
    {
      return nsm_vlan_cli_return (cli, ret, NULL, NULL, vid);
    }

  return CLI_SUCCESS;
}

CLI (nsm_cus_vlan_name,
     nsm_cus_vlan_name_cmd,
     "vlan "NSM_VLAN_CLI_RNG" type customer name WORD "
     "(state (enable|disable)|)",
     CLI_VLAN_STR,
     "VLAN id",
     "VLAN Type",
     NSM_VLAN_CVLAN_STR,
     "Ascii name of the VLAN",
     "The ascii name of the VLAN",
     "Operational state of the VLAN",
     "Enable",
     "Disable")
{
  int ret;
  u_int16_t vid;
  u_int8_t vlan_type;
  struct nsm_master *nm = cli->vr->proto;
  struct nsm_bridge *bridge;
  struct nsm_bridge_master *master = nsm_bridge_get_master (nm);

  /* Get the vid and check if it is within range (2-4096).  */
  CLI_GET_INTEGER_RANGE ("VLAN id", vid, argv[0], NSM_VLAN_CLI_MIN, NSM_VLAN_CLI_MAX);

  vlan_type = NSM_VLAN_STATIC | NSM_VLAN_CVLAN;

  /* Check if VLAN name size does not exceed 16 characters */
  if (pal_strlen (argv[1]) > NSM_VLAN_NAME_MAX)
    {
      cli_out (cli, "%% VLAN-NAME size should not exceed 16\n");
      return CLI_ERROR;
    }

  bridge = nsm_get_default_bridge (master);

  if (! bridge)
    {
      cli_out (cli, "%% Default Bridge not found\n");
      return CLI_ERROR;
    }

  /* Check for the state. */
  if (argc > 3)
    {
      if (! pal_strncmp (argv[3], "e", 1))
        /* Create the VLAN.  */
        ret = nsm_vlan_add (master, bridge->name, argv[1], vid,
                            NSM_VLAN_ACTIVE, vlan_type);
      else
        ret = nsm_vlan_add (master, bridge->name, argv[1], vid,
                            NSM_VLAN_DISABLED, vlan_type);
    }
  else
    ret = nsm_vlan_add (master, bridge->name, argv[1], vid,
                        NSM_VLAN_ACTIVE, vlan_type);

  if (ret < 0)
    return nsm_vlan_cli_return (cli, ret, NULL, NULL, vid);

  return CLI_SUCCESS;
}

CLI (nsm_cus_vlan_no_name,
     nsm_cus_vlan_no_name_cmd,
     "vlan "NSM_VLAN_CLI_RNG" type customer",
     CLI_VLAN_STR,
     "VLAN id",
     "VLAN Type",
     NSM_VLAN_CVLAN_STR)
{
  int ret;
  int brid;
  u_int16_t vid;
  u_int8_t vlan_type;
  struct nsm_bridge *bridge;
  struct nsm_master *nm = cli->vr->proto;
  struct nsm_bridge_master *master = nsm_bridge_get_master (nm);

  /* Get the vid and check if it is within range (2-4096).  */
  CLI_GET_INTEGER_RANGE ("VLAN id", vid, argv[0], NSM_VLAN_CLI_MIN, NSM_VLAN_CLI_MAX);

  vlan_type = NSM_VLAN_STATIC | NSM_VLAN_CVLAN;

  if (argc == 2)
    {
      /* Get the bridge group number and see if its within range. */
      CLI_GET_INTEGER_RANGE ("Bridge group", brid, argv[1], 1, 32);

      /* Create the VLAN.  */
      ret = nsm_vlan_add (master, argv[1], NULL, vid,  NSM_VLAN_ACTIVE,
                          vlan_type);
    }
  else
    {
      bridge = nsm_get_default_bridge (master);

      if (! bridge)
        {
          cli_out (cli, "%% Default Bridge not found\n");
          return CLI_ERROR;
        }

      /* Create the VLAN.  */
      ret = nsm_vlan_add (master, bridge->name, NULL, vid,  NSM_VLAN_ACTIVE,
                          vlan_type);
    }

  if (ret < 0)
    return nsm_vlan_cli_return (cli, ret, NULL, NULL, vid); 

  return CLI_SUCCESS;
}

ALI (nsm_cus_vlan_no_name,
     nsm_br_cus_vlan_no_name_cmd,
     "vlan "NSM_VLAN_CLI_RNG" type customer bridge <1-32>",
     CLI_VLAN_STR,
     "VLAN id",
     "VLAN Type",
     NSM_VLAN_CVLAN_STR,
     NSM_VLAN_SVLAN_STR,
     NSM_BRIDGE_STR,
     NSM_BRIDGE_NAME_STR);

CLI (no_nsm_pro_vlan_no_name,
     no_nsm_pro_vlan_no_name_cmd,
     "no vlan "NSM_VLAN_CLI_RNG" type (customer|service|backbone)",
     CLI_NO_STR,
     CLI_VLAN_STR,
     "VLAN id",
     "VLAN Type",
     NSM_VLAN_CVLAN_STR,
     NSM_VLAN_SVLAN_STR,
     "Identifies the backbone bridge instance of the B-component.")
{
  int ret;
  int brid;
  u_int16_t vid;
  u_int8_t vlan_type;
  struct nsm_bridge *bridge;
  struct nsm_master *nm = cli->vr->proto;
  struct nsm_bridge_master *master = nsm_bridge_get_master (nm);

  /* Get the vid and check if it is within range (2-4096).  */
  CLI_GET_INTEGER_RANGE ("VLAN id", vid, argv[0], NSM_VLAN_CLI_MIN, NSM_VLAN_CLI_MAX);

  if (argv[1][0] == 'c')
    vlan_type = NSM_VLAN_STATIC | NSM_VLAN_CVLAN;
  else if (argv[1][0] == 's')
    vlan_type = NSM_VLAN_STATIC | NSM_VLAN_SVLAN;
  else
     vlan_type = NSM_VLAN_STATIC 
#ifdef HAVE_B_BEB
                | NSM_VLAN_BVLAN
#endif  
           ;
  if (argc == 3)
    {
      /* Get the brid and check if it is within range. */
      CLI_GET_INTEGER_RANGE ("Bridge group", brid, argv[2], 1, 32);

      /* Delete VLAN. */
      ret = nsm_vlan_delete (master, argv[2], vid, vlan_type);
    }
  else
    {
#ifdef HAVE_B_BEB      
      if (CHECK_FLAG (vlan_type, NSM_VLAN_BVLAN))
        bridge = master->b_bridge;
      else
#endif        
        bridge = nsm_get_default_bridge (master);

      if (! bridge)
        {
          cli_out (cli, "%% Default or Backbone Bridge not found\n");
          return CLI_ERROR;
        }

      /* Delete VLAN. */
      ret = nsm_vlan_delete (master, bridge->name, vid, vlan_type);
    }
  
  return nsm_vlan_cli_return (cli, ret, NULL, NULL, NSM_VLAN_NULL_VID);
   
}

ALI (no_nsm_pro_vlan_no_name,
     no_nsm_br_pro_vlan_no_name_cmd,
     "no vlan "NSM_VLAN_CLI_RNG" type (customer|service) bridge <1-32>",
     CLI_NO_STR,
     CLI_VLAN_STR,
     "VLAN id",
     "VLAN Type",
     NSM_VLAN_CVLAN_STR,
     NSM_VLAN_SVLAN_STR,
     NSM_BRIDGE_STR,
     NSM_BRIDGE_NAME_STR);

CLI (nsm_cus_vlan_enable_disable,
     nsm_cus_vlan_enable_disable_cmd,
     "vlan "NSM_VLAN_CLI_RNG" type customer state (enable|disable)",
     CLI_VLAN_STR,
     "VLAN id",
     "VLAN Type",
     NSM_VLAN_CVLAN_STR,
     "Operational state of the VLAN",
     "Enable",
     "Disable")
{
  int ret;
  u_int16_t vid;
  u_int8_t vlan_type;
  struct nsm_bridge *bridge;
  struct nsm_master *nm = cli->vr->proto;
  struct nsm_bridge_master *master = nsm_bridge_get_master (nm);

  /* Get the vid and check if it is within range (2-4096).  */
  CLI_GET_INTEGER_RANGE ("VLAN id", vid, argv[0], NSM_VLAN_CLI_MIN, NSM_VLAN_CLI_MAX);

  vlan_type = NSM_VLAN_STATIC | NSM_VLAN_CVLAN;

  bridge = nsm_get_default_bridge (master);

  if (! bridge)
    {
      cli_out (cli, "%% Default Bridge not found\n");
      return CLI_ERROR;
    }

  /* Update the VLAN. */
  if (! pal_strncmp (argv[1], "e", 1))
    ret = nsm_vlan_add (master, bridge->name, NULL, vid, NSM_VLAN_ACTIVE,
                        vlan_type);
  else
    ret = nsm_vlan_add (master, bridge->name, NULL, vid, NSM_VLAN_DISABLED,
                        vlan_type);

  if (ret < 0)
    return nsm_vlan_cli_return (cli, ret, NULL, NULL, vid);

  return CLI_SUCCESS;
}

CLI (nsm_br_cus_vlan_enable_disable,
     nsm_br_cus_vlan_enable_disable_cmd,
     "vlan "NSM_VLAN_CLI_RNG" type customer bridge <1-32> "
     "state (enable|disable)",
     CLI_VLAN_STR,
     "VLAN id",
     "VLAN Type",
     NSM_VLAN_CVLAN_STR,
     NSM_BRIDGE_STR,
     NSM_BRIDGE_NAME_STR,
     "Operational state of the VLAN",
     "Enable",
     "Disable")
{
  int ret;
  int brid;
  u_int16_t vid;
  u_int8_t vlan_type;
  struct nsm_master *nm = cli->vr->proto;
  struct nsm_bridge_master *master = nsm_bridge_get_master (nm);

  /* Get the vid and check if it is within range (2-4096).  */
  CLI_GET_INTEGER_RANGE ("VLAN id", vid, argv[0], NSM_VLAN_CLI_MIN, NSM_VLAN_CLI_MAX);

  vlan_type = NSM_VLAN_STATIC | NSM_VLAN_CVLAN;

  /* Get the brid and check if it is within range. */
  CLI_GET_INTEGER_RANGE ("Bridge group", brid, argv[1], 1, 32);

  /* Update the VLAN. */
  if (! pal_strncmp (argv[2], "e", 1))
    ret = nsm_vlan_add (master, argv[1], NULL, vid, NSM_VLAN_ACTIVE, vlan_type);
  else
    ret = nsm_vlan_add (master, argv[1], NULL, vid, NSM_VLAN_DISABLED, vlan_type);

  if (ret < 0)
    return nsm_vlan_cli_return (cli, ret, NULL, NULL, vid);

  return CLI_SUCCESS;
}

CLI (nsm_br_ser_vlan_name,
     nsm_br_ser_vlan_name_cmd,
     "vlan "NSM_VLAN_CLI_RNG" type service "
     "(point-point|multipoint-multipoint) bridge <1-32> "
     "name WORD (state (enable|disable)|)",
     CLI_VLAN_STR,
     "VLAN id",
     "VLAN Type",
     NSM_VLAN_SVLAN_STR,
     NSM_VLAN_P2P_STR,
     NSM_VLAN_M2M_STR,
     NSM_BRIDGE_STR,
     NSM_BRIDGE_NAME_STR,
     "Ascii name of the VLAN",
     "The ascii name of the VLAN",
     "Operational state of the VLAN",
     "Enable",
     "Disable")
{
  int ret;
  int brid;
  u_int16_t vid;
  u_int8_t vlan_type;
  struct nsm_master *nm = cli->vr->proto;
  struct nsm_bridge_master *master = nsm_bridge_get_master (nm);

  /* Get the vid and check if it is within range (2-4096).  */
  CLI_GET_INTEGER_RANGE ("VLAN id", vid, argv[0], NSM_VLAN_CLI_MIN, NSM_VLAN_CLI_MAX);

  if (argv [1][0] == 'p')
    vlan_type = NSM_VLAN_STATIC | NSM_VLAN_SVLAN | NSM_SVLAN_P2P;
  else
    vlan_type = NSM_VLAN_STATIC | NSM_VLAN_SVLAN | NSM_SVLAN_M2M;

  /* Get the bridge group number and see if its within range. */
  CLI_GET_INTEGER_RANGE ("Bridge group", brid, argv[2], 1, 32);

  /* Check if VLAN name size does not exceed 16 characters */
  if (pal_strlen (argv[3]) > NSM_VLAN_NAME_MAX)
    {
      cli_out (cli, "%% VLAN-NAME size should not exceed 16\n");
      return CLI_ERROR;
    }

  /* Check for the state. */
  if (argc > 4)
    {
      if (! pal_strncmp (argv[5], "e", 1))
        /* Create the VLAN.  */
        ret = nsm_vlan_add (master, argv[2], argv[3], vid,
                            NSM_VLAN_ACTIVE, vlan_type);
      else
        ret = nsm_vlan_add (master, argv[2], argv[3], vid,
                            NSM_VLAN_DISABLED, vlan_type);
    }
  else
    ret = nsm_vlan_add (master, argv[2], argv[3], vid,
                        NSM_VLAN_ACTIVE, vlan_type);

  if (ret < 0)
    return nsm_vlan_cli_return (cli, ret, NULL, NULL, vid);

  return CLI_SUCCESS;
}

#ifdef HAVE_B_BEB
/* Cli for adding to the default bridge*/
CLI (nsm_backbone_vlan_name,
     nsm_backbone_vlan_name_cmd,
     "vlan "NSM_VLAN_CLI_RNG" type backbone "
     "((point-point | multipoint-multipoint) (name WORD|)) "
     "(state (enable | disable)|)",
     CLI_VLAN_STR,
     "VLAN id",
     "VLAN Type",
     "Identifies the backbone bridge instance of the B-component.",
     NSM_VLAN_P2P_STR,
     NSM_VLAN_M2M_STR,
     "Ascii name of the VLAN",
     "The ascii name of the VLAN",
     "Operational state of the VLAN",
     "Sets VLAN to enable state",
     "Sets VLAN to disable state")
{

  int ret = 0 ;
  u_int16_t vid = 0;
  u_int16_t vlan_type;
  char *vlan_name = NULL;
  struct nsm_master *nm = cli->vr->proto;
  struct nsm_bridge *bridge;
  struct nsm_bridge_master *master = nsm_bridge_get_master (nm);
  /* Get the vid and check if it is within range (2-4096).  */
  CLI_GET_INTEGER_RANGE ("VLAN id", vid, argv[0], NSM_VLAN_CLI_MIN, NSM_VLAN_CLI_MAX);
  if (argv [1][0] == 'p')
    vlan_type = NSM_VLAN_STATIC | NSM_VLAN_BVLAN | NSM_BVLAN_P2P;
  else
    vlan_type = NSM_VLAN_STATIC | NSM_VLAN_BVLAN | NSM_BVLAN_M2M;
  if(argc > 2)
  {
    /* Check if VLAN name size does not exceed 16 characters */
    if (argv[2][0] == 'n')
    {
      if (pal_strlen (argv[2]) > NSM_VLAN_NAME_MAX)
      {
        cli_out (cli, "%% VLAN-NAME size should not exceed 16\n");
        return CLI_ERROR;
      }
      else
        vlan_name = argv[3];     
    }
  }
  bridge = master->b_bridge;
 
  if (! bridge)
  {
    cli_out (cli, "%% Backbone Bridge not found\n");
    return CLI_ERROR;
  }
  /* Check for the state. */
  if (argc > 4)/*both name and state given*/
  {
    if (! pal_strncmp (argv[5], "e", 1))
      /* Create the VLAN.  */
      ret = nsm_vlan_add (master, bridge->name, vlan_name, vid,
                          NSM_VLAN_ACTIVE, vlan_type);
     /*state disabled to prevent adding to kernel*/
    else
      ret = nsm_vlan_add (master, bridge->name, vlan_name, vid,
                          NSM_VLAN_DISABLED, vlan_type);
  }
  else if (argc > 2)/* either name or state */
  {
    if (!pal_strncmp (argv[2],"s",1) && !pal_strncmp (argv[3],"e",1))
      ret = nsm_vlan_add (master, bridge->name, vlan_name, vid,
                        NSM_VLAN_ACTIVE, vlan_type);

     /*state disabled to prevent adding to kernel*/	
    else if (!pal_strncmp (argv[2],"s",1) && !pal_strncmp (argv[3],"d",1))
      ret = nsm_vlan_add (master, bridge->name, vlan_name, vid,
                        NSM_VLAN_DISABLED, vlan_type); 
                         
    else
      ret = nsm_vlan_add (master, bridge->name, vlan_name, vid,
                          NSM_VLAN_ACTIVE, vlan_type);
  }
  else
    ret = nsm_vlan_add (master, bridge->name, vlan_name, vid,
                          NSM_VLAN_ACTIVE, vlan_type);
 
  if (ret < 0)
    return nsm_vlan_cli_return (cli, ret, NULL, NULL, vid);

  return CLI_SUCCESS;
}


#endif /*HAVE_B_BEB*/
CLI (nsm_ser_vlan_name,
     nsm_ser_vlan_name_cmd,
     "vlan "NSM_VLAN_CLI_RNG" type service "
     "(point-point|multipoint-multipoint) name WORD "
     "(state (enable|disable)|)",
     CLI_VLAN_STR,
     "VLAN id",
     "VLAN Type",
     NSM_VLAN_SVLAN_STR,
     NSM_VLAN_P2P_STR,
     NSM_VLAN_M2M_STR,
     "Ascii name of the VLAN",
     "The ascii name of the VLAN",
     "Operational state of the VLAN",
     "Enable",
     "Disable")
{
  int ret;
  u_int16_t vid;
  u_int8_t vlan_type;
  struct nsm_master *nm = cli->vr->proto;
  struct nsm_bridge *bridge;
  struct nsm_bridge_master *master = nsm_bridge_get_master (nm);

  /* Get the vid and check if it is within range (2-4096).  */
  CLI_GET_INTEGER_RANGE ("VLAN id", vid, argv[0], NSM_VLAN_CLI_MIN, NSM_VLAN_CLI_MAX);

  if (argv [1][0] == 'p')
    vlan_type = NSM_VLAN_STATIC | NSM_VLAN_SVLAN | NSM_SVLAN_P2P;
  else
    vlan_type = NSM_VLAN_STATIC | NSM_VLAN_SVLAN | NSM_SVLAN_M2M;

  /* Check if VLAN name size does not exceed 16 characters */
  if (pal_strlen (argv[2]) > NSM_VLAN_NAME_MAX)
    {
      cli_out (cli, "%% VLAN-NAME size should not exceed 16\n");
      return CLI_ERROR;
    }

  bridge = nsm_get_default_bridge (master);

  if (! bridge)
    {
      cli_out (cli, "%% Default Bridge not found\n");
      return CLI_ERROR;
    }

  /* Check for the state. */
  if (argc > 3)
    {
      if (! pal_strncmp (argv[4], "e", 1))
        /* Create the VLAN.  */
        ret = nsm_vlan_add (master, bridge->name, argv[2], vid,
                            NSM_VLAN_ACTIVE, vlan_type);
      else
        ret = nsm_vlan_add (master, bridge->name, argv[2], vid,
                            NSM_VLAN_DISABLED, vlan_type);
    }
  else
    ret = nsm_vlan_add (master, bridge->name, argv[2], vid,
                        NSM_VLAN_ACTIVE, vlan_type);

  if (ret < 0)
    return nsm_vlan_cli_return (cli, ret, NULL, NULL, vid);

  return CLI_SUCCESS;
}

CLI (nsm_ser_vlan_no_name,
     nsm_ser_vlan_no_name_cmd,
     "vlan "NSM_VLAN_CLI_RNG" type service (point-point|multipoint-multipoint)",
     CLI_VLAN_STR,
     NSM_VLAN_P2P_STR,
     NSM_VLAN_M2M_STR,
     "VLAN id",
     "VLAN Type",
     NSM_VLAN_CVLAN_STR)
{
  int ret;
  int brid;
  u_int16_t vid;
  u_int8_t vlan_type;
  struct nsm_bridge *bridge;
  struct nsm_master *nm = cli->vr->proto;
  struct nsm_bridge_master *master = nsm_bridge_get_master (nm);

  /* Get the vid and check if it is within range (2-4096).  */
  CLI_GET_INTEGER_RANGE ("VLAN id", vid, argv[0], NSM_VLAN_CLI_MIN, NSM_VLAN_CLI_MAX);

  if (argv [1][0] == 'p')
    vlan_type = NSM_VLAN_STATIC | NSM_VLAN_SVLAN | NSM_SVLAN_P2P;
  else
    vlan_type = NSM_VLAN_STATIC | NSM_VLAN_SVLAN | NSM_SVLAN_M2M;

  if (argc == 3)
    {
      /* Get the bridge group number and see if its within range. */
      CLI_GET_INTEGER_RANGE ("Bridge group", brid, argv[2], 1, 32);

      /* Create the VLAN.  */
      ret = nsm_vlan_add (master, argv[2], NULL, vid,  NSM_VLAN_ACTIVE,
                          vlan_type);
    }
  else
    {
      bridge = nsm_get_default_bridge (master);

      if (! bridge)
        {
          cli_out (cli, "%% Default Bridge not found\n");
          return CLI_ERROR;
        }

      /* Create the VLAN.  */
      ret = nsm_vlan_add (master, bridge->name, NULL, vid,  NSM_VLAN_ACTIVE,
                          vlan_type);
    }

  if (ret < 0)
    return nsm_vlan_cli_return (cli, ret, NULL, NULL, vid);

  return CLI_SUCCESS;
}

ALI (nsm_ser_vlan_no_name,
     nsm_br_ser_vlan_no_name_cmd,
     "vlan "NSM_VLAN_CLI_RNG" type service (point-point|multipoint-multipoint) "
     "bridge <1-32>",
     CLI_VLAN_STR,
     "VLAN id",
     "VLAN Type",
     NSM_VLAN_SVLAN_STR,
     NSM_VLAN_P2P_STR,
     NSM_VLAN_M2M_STR,
     NSM_BRIDGE_STR,
     NSM_BRIDGE_NAME_STR);

CLI (nsm_ser_vlan_enable_disable,
     nsm_ser_vlan_enable_disable_cmd,
     "vlan "NSM_VLAN_CLI_RNG" type service (point-point|multipoint-multipoint)"
     "state (enable|disable)",
     CLI_VLAN_STR,
     "VLAN id",
     "VLAN Type",
     NSM_VLAN_SVLAN_STR,
     NSM_VLAN_P2P_STR,
     NSM_VLAN_M2M_STR,
     "Operational state of the VLAN",
     "Enable",
     "Disable")
{
  int ret;
  u_int16_t vid;
  u_int8_t vlan_type;
  struct nsm_bridge *bridge;
  struct nsm_master *nm = cli->vr->proto;
  struct nsm_bridge_master *master = nsm_bridge_get_master (nm);

  /* Get the vid and check if it is within range (2-4096).  */
  CLI_GET_INTEGER_RANGE ("VLAN id", vid, argv[0], NSM_VLAN_CLI_MIN, NSM_VLAN_CLI_MAX);

  if (argv [1][0] == 'p')
    vlan_type = NSM_VLAN_STATIC | NSM_VLAN_SVLAN | NSM_SVLAN_P2P;
  else
    vlan_type = NSM_VLAN_STATIC | NSM_VLAN_SVLAN | NSM_SVLAN_M2M;

  bridge = nsm_get_default_bridge (master);

  if (! bridge)
    {
       cli_out (cli, "%% Default Bridge not found\n");
       return CLI_ERROR;
    }

  /* Update the VLAN. */
  if (! pal_strncmp (argv[2], "e", 1))
    ret = nsm_vlan_add (master, bridge->name, NULL, vid, NSM_VLAN_ACTIVE,
                        vlan_type);
  else
    ret = nsm_vlan_add (master, bridge->name, NULL, vid, NSM_VLAN_DISABLED,
                        vlan_type);

  if (ret < 0)
    return nsm_vlan_cli_return (cli, ret, NULL, NULL, vid);

  return CLI_SUCCESS;
}

CLI (nsm_br_ser_vlan_enable_disable,
     nsm_br_ser_vlan_enable_disable_cmd,
     "vlan "NSM_VLAN_CLI_RNG" type service (point-point|multipoint-multipoint)"
     "bridge <1-32> state (enable|disable)",
     CLI_VLAN_STR,
     "VLAN id",
     "VLAN Type",
     NSM_VLAN_SVLAN_STR,
     NSM_VLAN_P2P_STR,
     NSM_VLAN_M2M_STR,
     NSM_BRIDGE_STR,
     NSM_BRIDGE_NAME_STR,
     "Operational state of the VLAN",
     "Enable",
     "Disable")
{
  int ret;
  int brid;
  u_int16_t vid;
  u_int8_t vlan_type;
  struct nsm_master *nm = cli->vr->proto;
  struct nsm_bridge_master *master = nsm_bridge_get_master (nm);

  /* Get the vid and check if it is within range (2-4096).  */
  CLI_GET_INTEGER_RANGE ("VLAN id", vid, argv[0], NSM_VLAN_CLI_MIN, NSM_VLAN_CLI_MAX);

  if (argv [1][0] == 'p')
    vlan_type = NSM_VLAN_STATIC | NSM_VLAN_SVLAN | NSM_SVLAN_P2P;
  else
    vlan_type = NSM_VLAN_STATIC | NSM_VLAN_SVLAN | NSM_SVLAN_M2M;

  /* Get the brid and check if it is within range. */
  CLI_GET_INTEGER_RANGE ("Bridge group", brid, argv[2], 1, 32);

  /* Update the VLAN. */
  if (! pal_strncmp (argv[3], "e", 1))
    ret = nsm_vlan_add (master, argv[2], NULL, vid, NSM_VLAN_ACTIVE, vlan_type);
  else
    ret = nsm_vlan_add (master, argv[2], NULL, vid, NSM_VLAN_DISABLED, vlan_type);

  if (ret < 0)
    return nsm_vlan_cli_return (cli, ret, NULL, NULL, vid);

  return CLI_SUCCESS;
}


CLI (nsm_pro_vlan_mtu,
     nsm_pro_vlan_mtu_cmd,
     "vlan "NSM_VLAN_CLI_RNG" type (customer|service) mtu MTU_VAL",
     CLI_VLAN_STR,
     "VLAN id",
     "VLAN Type",
     NSM_VLAN_CVLAN_STR,
     NSM_VLAN_SVLAN_STR,
     "MTU Value",
     "The Maximum Transmission Unit value for the vlan")
{
  int ret;
  int brid;
  u_int16_t vid;
  u_int8_t vlan_type;
  u_int32_t mtu_val;
  struct nsm_bridge *bridge;
  struct nsm_master *nm = cli->vr->proto;
  struct nsm_bridge_master *master = nsm_bridge_get_master (nm);

  /* Get the vid and check if it is within range (2-4096).  */
  CLI_GET_INTEGER_RANGE ("VLAN id", vid, argv[0], NSM_VLAN_CLI_MIN, NSM_VLAN_CLI_MAX);

  if (argv[1][0] == 'c')
    vlan_type = NSM_VLAN_STATIC | NSM_VLAN_CVLAN;
  else
    vlan_type = NSM_VLAN_STATIC | NSM_VLAN_SVLAN;

  /* Get the value of the MTU */

  CLI_GET_INTEGER_RANGE ("MTU Value", mtu_val, argv[1], 1, UINT32_MAX);

  if (argc > 2)
    {
      /* Get the bridge group number and see if its within range. */
      CLI_GET_INTEGER_RANGE ("Bridge group", brid, argv[2], 1, 32);

      /* Set the vlaue of MTU for the VLAN.  */
      ret = nsm_vlan_set_mtu (master, argv[2], vid, vlan_type, mtu_val);
    }
  else
    {
      bridge = nsm_get_default_bridge (master);

      if (! bridge)
        {
          cli_out (cli, "%% Default Bridge not found\n");
          return CLI_ERROR;
        }

      /* Set the vlaue of MTU for the VLAN.  */
      ret = nsm_vlan_set_mtu (master, bridge->name, vid, vlan_type, mtu_val);
    }

  if (ret < 0)
    {
      if (ret == NSM_VLAN_ERR_BRIDGE_NOT_FOUND)
        cli_out (cli, "%% Brige-group Not Found \n");
      else if (ret == NSM_VLAN_ERR_VLAN_NOT_FOUND)
        cli_out (cli, "%% VLAN Not a member of the bridge-group\n");
      else
        cli_out (cli, "%% Set VLAN MTU failed\n");
      return CLI_ERROR;
    }

  return CLI_SUCCESS;

}

ALI (nsm_pro_vlan_mtu,
     nsm_br_pro_vlan_mtu_cmd,
     "vlan "NSM_VLAN_CLI_RNG" type (customer|service) mtu MTU_VAL bridge <1-32>",
     CLI_VLAN_STR,
     "VLAN id",
     "VLAN Type",
     NSM_VLAN_CVLAN_STR,
     NSM_VLAN_SVLAN_STR,
     "MTU Value",
     "The Maximum Transmission Unit value for the vlan",
     NSM_BRIDGE_STR,
     NSM_BRIDGE_NAME_STR);

CLI (no_nsm_pro_vlan_mtu,
     no_nsm_pro_vlan_mtu_cmd,
     "no vlan "NSM_VLAN_CLI_RNG" type (customer|service) mtu",
     CLI_NO_STR,
     CLI_VLAN_STR,
     "VLAN id",
     "VLAN Type",
     NSM_VLAN_CVLAN_STR,
     NSM_VLAN_SVLAN_STR,
     "Reset the MMaximum Transmission Unit value for the vlan")
{
  int ret;
  int brid;
  u_int16_t vid;
  u_int8_t vlan_type;
  u_int32_t mtu_val = 0;
  struct nsm_bridge *bridge;
  struct nsm_master *nm = cli->vr->proto;
  struct nsm_bridge_master *master = nsm_bridge_get_master (nm);

  /* Get the vid and check if it is within range (2-4096).  */
  CLI_GET_INTEGER_RANGE ("VLAN id", vid, argv[0], 2, NSM_VLAN_CLI_MAX);

  if (argv[1][0] == 'c')
    vlan_type = NSM_VLAN_STATIC | NSM_VLAN_CVLAN;
  else
    vlan_type = NSM_VLAN_STATIC | NSM_VLAN_SVLAN;

  if (argc > 3)
    {
      /* Get the bridge group number and see if its within range. */

      CLI_GET_INTEGER_RANGE ("Bridge group", brid, argv[2], 1, 32);

      /* Reset the value of MTU for the VLAN.  mtu_val = 0 */
      ret = nsm_vlan_set_mtu (master, argv[2], vid, vlan_type, mtu_val);
    }
  else
    {
      bridge = nsm_get_default_bridge (master);

      if (! bridge)
        {
          cli_out (cli, "%% Default Bridge not found\n");
          return CLI_ERROR;
        }

      /* Reset the value of MTU for the VLAN.  mtu_val = 0 */
      ret = nsm_vlan_set_mtu (master, bridge->name, vid, vlan_type, mtu_val);
    }

  if (ret < 0)
    {
      if (ret == NSM_VLAN_ERR_BRIDGE_NOT_FOUND)
        cli_out (cli, "%% Port not attached to any bridge-group\n");
      else if (ret == NSM_VLAN_ERR_VLAN_NOT_FOUND)
        cli_out (cli, "%% VLAN Not a member of the bridge-group\n");
      else
        cli_out (cli, "%% Set VLAN MTU failed\n");
      return CLI_ERROR;
    }

  return CLI_SUCCESS;

}

ALI (no_nsm_pro_vlan_mtu,
     no_nsm_br_pro_vlan_mtu_cmd,
     "no vlan "NSM_VLAN_CLI_RNG" type (customer|service) mtu bridge <1-32>",
     CLI_NO_STR,
     CLI_VLAN_STR,
     "VLAN id",
     "VLAN Type",
     NSM_VLAN_CVLAN_STR,
     NSM_VLAN_SVLAN_STR,
     "Reset the MMaximum Transmission Unit value for the vlan",
     NSM_BRIDGE_STR,
     NSM_BRIDGE_NAME_STR);

CLI (nsm_vlan_switchport_mode_ce,
     nsm_vlan_switchport_mode_ce_cmd,
     "switchport mode customer-edge (access|hybrid|trunk)",
     "Set the switching characteristics of the Layer2 interface",
     "Set the mode of the Layer2 interface",
     NSM_VLAN_CE_STR,
     NSM_VLAN_ACCESS_STR,
     NSM_VLAN_HYBRID_STR,
     NSM_VLAN_TRUNK_STR)
{
  int ret;
  enum nsm_vlan_port_mode mode = NSM_VLAN_PORT_MODE_INVALID;
  enum nsm_vlan_port_mode sub_mode = NSM_VLAN_PORT_MODE_INVALID;
  struct interface *ifp = cli->index;

#ifdef HAVE_LACPD
  NSM_INTERFACE_CHECK_AGG_MEM_PROPERTY(cli, ifp);
#endif /* HAVE_LACPD */
  NSM_BRIDGE_CHECK_L2_PROPERTY(cli, ifp);
  NSM_BRIDGE_CHECK_BRIDGE_PROPERTY(cli, ifp);

  mode = NSM_VLAN_PORT_MODE_CE;

  if (! pal_strncmp (argv[0], "a", 1))
    sub_mode = NSM_VLAN_PORT_MODE_ACCESS;
  else if (! pal_strncmp (argv[0], "h", 1))
    sub_mode = NSM_VLAN_PORT_MODE_HYBRID;
  else if (! pal_strncmp (argv[0], "t", 1))
    sub_mode = NSM_VLAN_PORT_MODE_TRUNK;

  ret = nsm_vlan_api_set_port_mode (ifp, mode, sub_mode, PAL_TRUE, PAL_TRUE);
  if (ret < 0)
    {
      if (ret == NSM_VLAN_ERR_BRIDGE_NOT_FOUND)
        cli_out (cli, "%% Port not attached to any bridge-group\n");
      else if (ret == NSM_VLAN_ERR_INVALID_MODE)
        cli_out (cli, "%% Invalid switchport mode\n");
      else if (ret == NSM_VLAN_ERR_MODE_CLEAR)
        cli_out (cli, "%% Error clearing output VLAN of previous mode\n");
#ifdef HAVE_LACPD
      else if (ret == AGG_MEM_NO_SWITCHPORT)
        cli_out (cli, "%% Member interface not configured for switching\n");
      else if (ret == AGG_MEM_BRIDGE_NOT_VLAN_AWARE)
        cli_out (cli, "%% Member interface not vlan aware\n");
#endif /* HAVE LACP */
      else if (ret == NSM_VLAN_ERR_CVLAN_REG_EXIST)
        cli_out (cli, "%% Unconfigure existing CVLAN Registration Table\n");
      else
        cli_out (cli, "%% Set port mode failed\n");
      return CLI_ERROR;
    }

  return CLI_SUCCESS;
}

#ifdef HAVE_PROVIDER_BRIDGE
CLI (nsm_vlan_preserve_cos,
     nsm_vlan_preserve_cos_cmd,
     "ce-vlan preserve-cos VLAN_ID",
     "COS Preservation for Customer Edge VLAN",
     "Preserve Cos",
     "Service VLAN ID")
{
  int brid;
  s_int32_t ret;
  u_int16_t vid;
  struct nsm_bridge *bridge;
  struct nsm_master *nm = cli->vr->proto;
  struct nsm_bridge_master *master = nsm_bridge_get_master (nm);

  /* Get the vid and check if it is within range (1-4094).  */

  CLI_GET_INTEGER_RANGE ("VLAN id", vid, argv[0], NSM_VLAN_DEFAULT_VID,
                         NSM_VLAN_CLI_MAX);

  if (argc == 2)
    {
      /* Get the brid and check if it is within range. */
      CLI_GET_INTEGER_RANGE ("Bridge group", brid, argv[1], 1, 32);

      /* Delete VLAN. */
      ret = nsm_svlan_set_cos_preservation (master, argv[1], vid, PAL_TRUE);
    }
  else
    {
      bridge = nsm_get_default_bridge (master);

      if (! bridge)
        {
          cli_out (cli, "%% Default Bridge not found\n");
          return CLI_ERROR;
        }

      /* Delete VLAN. */
      ret = nsm_svlan_set_cos_preservation (master, bridge->name,
                                            vid, PAL_TRUE);
    }

  if (ret < 0)
    {
      cli_out (cli, "%% Can't set COS Preservation\n");
      return CLI_ERROR;
    }

  return CLI_SUCCESS;

}

ALI (nsm_vlan_preserve_cos,
     nsm_br_vlan_preserve_cos_cmd,
     "ce-vlan preserve-cos VLAN_ID bridge <1-32>",
     "COS Preservation for Customer Edge VLAN",
     "Preserve Cos",
     "Service VLAN ID",
     NSM_BRIDGE_STR,
     NSM_BRIDGE_NAME_STR);


CLI (no_nsm_vlan_preserve_cos,
     no_nsm_vlan_preserve_cos_cmd,
     "no ce-vlan preserve-cos VLAN_ID",
     CLI_NO_STR,
     "COS Preservation for Customer Edge VLAN",
     "Preserve Cos",
     "Service VLAN ID")
{
  int brid;
  s_int32_t ret;
  u_int16_t vid;
  struct nsm_bridge *bridge;
  struct nsm_master *nm = cli->vr->proto;
  struct nsm_bridge_master *master = nsm_bridge_get_master (nm);

  /* Get the vid and check if it is within range (1-4094).  */

  CLI_GET_INTEGER_RANGE ("VLAN id", vid, argv[0], NSM_VLAN_DEFAULT_VID,
                         NSM_VLAN_CLI_MAX);

  if (argc == 2)
    {
      /* Get the brid and check if it is within range. */
      CLI_GET_INTEGER_RANGE ("Bridge group", brid, argv[1], 1, 32);

      /* Delete VLAN. */
      ret = nsm_svlan_set_cos_preservation (master, argv[1], vid, PAL_FALSE);
    }
  else
    {
      bridge = nsm_get_default_bridge (master);

      if (! bridge)
        {
          cli_out (cli, "%% Default Bridge not found\n");
          return CLI_ERROR;
        }

      /* Delete VLAN. */
      ret = nsm_svlan_set_cos_preservation (master, bridge->name,
                                            vid, PAL_FALSE);
    }

  if (ret < 0)
    {
      cli_out (cli, "%% Can't set COS Preservation\n");
      return CLI_ERROR;
    }

  return CLI_SUCCESS;

}

ALI (no_nsm_vlan_preserve_cos,
     no_nsm_br_vlan_preserve_cos_cmd,
     "no ce-vlan preserve-cos VLAN_ID bridge <1-32>",
     CLI_NO_STR,
     "COS Preservation for Customer Edge VLAN",
     "Preserve Cos",
     "Service VLAN ID",
     NSM_BRIDGE_STR,
     NSM_BRIDGE_NAME_STR);

CLI (nsm_vlan_switchport_untagged_vid_pe,
     nsm_vlan_switchport_untagged_vid_pe_cmd,
     "switchport provider-edge vlan VLANID untagged-vlan VLANID",
     "Set the switching characteristics of the Layer2 interface",
     "Set the Chracteristics of Provider Edge Port",
     NSM_VLAN_STR,
     "VLAN ID Of the Provider Edge Port",
     "Configure the vlan that will be untagged on the provider side",
     "VLAN ID Untagged VLAN")
{
  int ret;
  u_int16_t svid;
  u_int16_t untagged_vid;
  struct interface *ifp = cli->index;

#ifdef HAVE_LACPD
  NSM_INTERFACE_CHECK_AGG_MEM_PROPERTY(cli, ifp);
#endif /* HAVE_LACPD */
  NSM_BRIDGE_CHECK_L2_PROPERTY(cli, ifp);
  NSM_BRIDGE_CHECK_BRIDGE_PROPERTY(cli, ifp);

  /* Get the vid and check if it is within range (1-4094).  */

  CLI_GET_INTEGER_RANGE ("VLAN id", svid, argv[0], NSM_VLAN_DEFAULT_VID,
                         NSM_VLAN_CLI_MAX);

  CLI_GET_INTEGER_RANGE ("VLAN id", untagged_vid, argv[1], NSM_VLAN_DEFAULT_VID,
                         NSM_VLAN_CLI_MAX);

  ret = nsm_pro_edge_port_set_untagged_vid (ifp, svid, untagged_vid);

  if (ret < 0)
    {
      if (ret == NSM_PRO_VLAN_ERR_INVALID_MODE)
        cli_out (cli, "%% Port not Customer edge port \n");
      else if (ret == NSM_PRO_VLAN_ERR_PRO_EDGE_NOT_FOUND)
        cli_out (cli, "%% Provider Edge Port not found \n");
      else if (ret == NSM_PRO_VLAN_ERR_CVLAN_REGIS_NOT_FOUND)
        cli_out (cli, "%% CVID Not member of the provider edge port\n");
      else
        cli_out (cli, "%% Set port mode failed\n");
      return CLI_ERROR;
    }

  return CLI_SUCCESS;
}

CLI (nsm_vlan_switchport_pvid_pe,
     nsm_vlan_switchport_pvid_pe_cmd,
     "switchport provider-edge vlan VLANID default-vlan VLANID",
     "Set the switching characteristics of the Layer2 interface",
     "Set the Chracteristics of Provider Edge Port",
     NSM_VLAN_STR,
     "VLAN ID Of the Provider Edge Port",
     "Configure the vlan to which untagged packets will be classified",
     "VLAN ID Untagged VLAN")
{
  int ret;
  u_int16_t svid;
  u_int16_t pvid;
  struct interface *ifp = cli->index;

#ifdef HAVE_LACPD
  NSM_INTERFACE_CHECK_AGG_MEM_PROPERTY(cli, ifp);
#endif /* HAVE_LACPD */
  NSM_BRIDGE_CHECK_L2_PROPERTY(cli, ifp);
  NSM_BRIDGE_CHECK_BRIDGE_PROPERTY(cli, ifp);

  /* Get the vid and check if it is within range (1-4094).  */

  CLI_GET_INTEGER_RANGE ("VLAN id", svid, argv[0], NSM_VLAN_DEFAULT_VID,
                         NSM_VLAN_CLI_MAX);

  CLI_GET_INTEGER_RANGE ("VLAN id", pvid, argv[1], NSM_VLAN_DEFAULT_VID,
                         NSM_VLAN_CLI_MAX);

  ret = nsm_pro_edge_port_set_pvid (ifp, svid, pvid);

  if (ret < 0)
    {
      if (ret == NSM_PRO_VLAN_ERR_INVALID_MODE)
        cli_out (cli, "%% Port not Customer edge port \n");
      else if (ret == NSM_PRO_VLAN_ERR_PRO_EDGE_NOT_FOUND)
        cli_out (cli, "%% Provider Edge Port not found \n");
      else if (ret == NSM_PRO_VLAN_ERR_CVLAN_REGIS_NOT_FOUND)
        cli_out (cli, "%% CVID Not member of the provider edge port\n");
      else
        cli_out (cli, "%% Set port mode failed\n");
      return CLI_ERROR;
    }
  return CLI_SUCCESS;
}

/* CLI to set EVC-ID */
CLI (nsm_vlan_set_evc_id,
     nsm_vlan_set_evc_id_cmd,
     "ethernet svlan <2-4094> evc-id EVC_ID (bridge <1-32>|)",
     "Configure Metro Ethernet Parameter",
     "service vlan to which the evc-id should be set",
     "enter the svlan id",
     "evc-id string for the svlan",
     "enter the evc id for the svlan",
     NSM_BRIDGE_STR,
     NSM_BRIDGE_NAME_STR)
{
  s_int8_t ret;
  u_int8_t brid;
  u_int16_t vid;
  struct nsm_master *nm = cli->vr->proto;
  struct nsm_bridge_master *master = nsm_bridge_get_master (nm);
  struct nsm_bridge *bridge = NULL;

  /* Get the vid and check if it is within range (2-4096).  */
  CLI_GET_INTEGER_RANGE ("VLAN id", vid, argv[0], NSM_VLAN_CLI_MIN, NSM_VLAN_CLI_MAX);

  if (argc == 4)
    {
      /* Get the bridge group number and see if its within range. */
      CLI_GET_INTEGER_RANGE ("Bridge group", brid, argv[3], 1, 32);
      
      ret = nsm_svlan_set_evc_id (master, argv[3], vid, argv[1]);
    }
  else
    {
      bridge = nsm_get_default_bridge (master);

      if (! bridge)
        {
          cli_out (cli, "%% Default Bridge not found\n");
          return CLI_ERROR;
        }

      ret = nsm_svlan_set_evc_id (master, bridge->name, vid, argv[1]);
    }

  if (ret < 0)
    return nsm_vlan_cli_return (cli, ret, NULL, NULL, vid);

  return CLI_SUCCESS;
}

/* CLI to unset EVC-ID */
CLI (no_nsm_vlan_set_evc_id,
     no_nsm_vlan_set_evc_id_cmd,
     "no ethernet svlan <2-4094> evc-id EVC_ID (bridge <1-32>|)",
     CLI_NO_STR,
    "Configure Metro Ethernet Parameter",
     "service vlan to which the evc-id should be set",
     "enter the svlan id",
     "evc-id string for the svlan",
     "enter the evc id for the svlan",
     NSM_BRIDGE_STR,
     NSM_BRIDGE_NAME_STR)
{
  s_int8_t ret;
  u_int8_t brid;
  u_int16_t vid;
  struct nsm_master *nm = cli->vr->proto;
  struct nsm_bridge_master *master = nsm_bridge_get_master (nm);
  struct nsm_bridge *bridge = NULL;

  /* Get the vid and check if it is within range (2-4096).  */
  CLI_GET_INTEGER_RANGE ("VLAN id", vid, argv[0], NSM_VLAN_CLI_MIN, NSM_VLAN_CLI_MAX);

  if (argc == 4)
    {
      /* Get the bridge group number and see if its within range. */
      CLI_GET_INTEGER_RANGE ("Bridge group", brid, argv[3], 1, 32);
      
      ret = nsm_svlan_unset_evc_id (master, argv[3], vid, argv[1]);
    }
  else
    {
      bridge = nsm_get_default_bridge (master);

      if (! bridge)
        {
          cli_out (cli, "%% Default Bridge not found\n");
          return CLI_ERROR;
        }

      ret = nsm_svlan_unset_evc_id (master, bridge->name, vid, argv[1]);
    }

  if (ret < 0)
    return nsm_vlan_cli_return (cli, ret, NULL, NULL, vid);

  return CLI_SUCCESS;
  
}

/* CLI to set max uni for the svlan */
CLI (nsm_br_vlan_max_uni_cli,
    nsm_br_vlan_max_uni_cmd,
    "ethernet (svlan <2-4094> | evc EVCID) max-uni <1-32> (bridge <1-32>|)",
    "Configure Metro Ethernet Parameter",
    "svlan for which the max uni need to be set",
    "enter the svlan id",
    "evc id for which the max uni need to be set",
    "enter the EVC ID",
    "max uni parameter for the EVC/svlan",
    "enter the number of max uni for the EVC/SVLAN",
    NSM_BRIDGE_STR,
    NSM_BRIDGE_NAME_STR)
{
  s_int8_t ret = PAL_FALSE;
  u_int8_t brid = PAL_FALSE;
  u_int16_t vid = PAL_FALSE;
  u_int8_t max_uni = PAL_FALSE;
  struct nsm_master *nm = cli->vr->proto;
  struct nsm_bridge_master *master = nsm_bridge_get_master (nm);
  struct nsm_bridge *bridge = NULL;

  /* IF evc-id is the key word and w or w/o bridge */
  if ((argc == 5) || (argc == 3))
    {
      /* Get the max uni and check if it is within range (1-32).  */
      CLI_GET_INTEGER_RANGE ("MAX UNI", max_uni, argv[2], 1, 32);

      if (argc == 5)
        {
          /* Get the bridge group number and see if its within range. */
          CLI_GET_INTEGER_RANGE ("Bridge group", brid, argv[4], 1, 32);

          /* API to set max uni */
          ret = nsm_svlan_set_max_uni (master, argv[4], vid, argv[1], max_uni);
        }
      else
        {
          bridge = nsm_get_default_bridge (master);

          if (! bridge)
            {
              cli_out (cli, "%% Default Bridge not found\n");
              return CLI_ERROR;
            }
          /* API to set max uni */
          ret = nsm_svlan_set_max_uni (master, bridge->name, PAL_FALSE, argv[1],
              max_uni);
        }
    }
  else if ((argc == 4) || (argc == 2))
    {
      /* Get the vid and check if it is within range (2-4096).  */
      CLI_GET_INTEGER_RANGE ("VLAN id", vid, argv[0], NSM_VLAN_CLI_MIN,
          NSM_VLAN_CLI_MAX);
      /* Get the max uni and check if it is within range (1-32).  */
      CLI_GET_INTEGER_RANGE ("MAX UNI", max_uni, argv[1], 1, 32);

      if (argc == 4)
        {
          /* Get the bridge group number and see if its within range. */
          CLI_GET_INTEGER_RANGE ("Bridge group", brid, argv[3], 1, 32);

          /* API to set max uni */
          ret = nsm_svlan_set_max_uni (master, argv[3], vid, NULL, max_uni);
        }
      else
        {
           bridge = nsm_get_default_bridge (master);

          if (! bridge)
            {
              cli_out (cli, "%% Default Bridge not found\n");
              return CLI_ERROR;
            }
          /* API to set max uni */
          ret = nsm_svlan_set_max_uni (master, bridge->name, vid, NULL,
              max_uni);
        }
    }

  if (ret < 0)
    {
      return nsm_vlan_cli_return (cli, ret, NULL, NULL, vid);
    }

  return ret;
}
     
CLI (no_nsm_br_vlan_max_uni_cli,
    no_nsm_br_vlan_max_uni_cmd,
    "no ethernet (svlan <2-4094> | evc EVC_ID) max-uni (bridge <1-32>|)",
    CLI_NO_STR,
    "Configure Metro Ethernet Parameter",
    "svlan for which the max uni need to be set",
    "enter the svlan id",
    "evc id for which the max uni need to be set",
    "enter the EVC ID",
    "max uni parameter for the EVC/svlan",
    NSM_BRIDGE_STR,
    NSM_BRIDGE_NAME_STR)
{
  s_int8_t ret = PAL_FALSE;
  u_int8_t brid = PAL_FALSE;
  u_int16_t vid = PAL_FALSE;
  struct nsm_master *nm = cli->vr->proto;
  struct nsm_bridge_master *master = nsm_bridge_get_master (nm);
  struct nsm_bridge *bridge = NULL;

  if ((argc == 4) || (argc == 2))
    {
      if (argc == 4)
        {
          /* Get the bridge group number and see if its within range. */
          CLI_GET_INTEGER_RANGE ("Bridge group", brid, argv[3], 1, 32);
          ret = nsm_svlan_unset_max_uni (master, argv[3], PAL_FALSE, argv[1]);

        }
      else
        {
          bridge = nsm_get_default_bridge (master);

          if (! bridge)
            {
              cli_out (cli, "%% Default Bridge not found\n");
              return CLI_ERROR;
            }
          ret = nsm_svlan_unset_max_uni (master, bridge->name, PAL_FALSE,
              argv[1]);
        }
    }
  else if ((argc == 1) || (argc == 3))
    {
      if (argc == 3)
        {
          /* Get the bridge group number and see if its within range. */
          CLI_GET_INTEGER_RANGE ("Bridge group", brid, argv[2], 1, 32);

          /* Get the vid and check if it is within range (2-4096).  */
          CLI_GET_INTEGER_RANGE ("VLAN id", vid, argv[0], NSM_VLAN_CLI_MIN,
              NSM_VLAN_CLI_MAX);

          ret = nsm_svlan_unset_max_uni (master, argv[2], vid, NULL);
        }
      else
        {
          bridge = nsm_get_default_bridge (master);

          if (! bridge)
            {
              cli_out (cli, "%% Default Bridge not found\n");
              return CLI_ERROR;
            }
          ret = nsm_svlan_unset_max_uni (master, bridge->name, vid, NULL);
        }
    }

  if (ret < 0)
    return nsm_vlan_cli_return (cli, ret, NULL, NULL, vid);

  return CLI_SUCCESS;
}

CLI (nsm_vlan_switchport_ce_default_evc,
  nsm_vlan_switchport_ce_default_evc_cmd,
  "switchport customer-edge (hybrid|trunk) default-svlan "
  "DEFAULT_SVLAN",
  "Set the switching characteristics of the Layer2 interface",
  NSM_VLAN_CE_STR,
  NSM_VLAN_HYBRID_STR,
  NSM_VLAN_TRUNK_STR,
  "Set the default SVLAN for the interface",
  "The default SVID for the interface")
{
  int ret;
  unsigned short default_evc;
  struct interface *ifp = cli->index;
  enum nsm_vlan_port_mode mode = NSM_VLAN_PORT_MODE_INVALID;
  enum nsm_vlan_port_mode sub_mode = NSM_VLAN_PORT_MODE_INVALID;

#ifdef HAVE_LACPD
  NSM_INTERFACE_CHECK_AGG_MEM_PROPERTY(cli, ifp);
#endif /* HAVE_LACPD */
  NSM_BRIDGE_CHECK_L2_PROPERTY(cli, ifp);
  NSM_BRIDGE_CHECK_BRIDGE_PROPERTY(cli, ifp);

  mode = NSM_VLAN_PORT_MODE_CE;

  if (! pal_strncmp (argv[0], "h", 1))
    sub_mode = NSM_VLAN_PORT_MODE_HYBRID;
  else if (! pal_strncmp (argv[0], "t", 1))
    sub_mode = NSM_VLAN_PORT_MODE_TRUNK;
  else
    sub_mode = NSM_VLAN_PORT_MODE_INVALID;

  CLI_GET_INTEGER_RANGE ("port vlanid", default_evc, argv[1], NSM_VLAN_CLI_MIN,
                         NSM_VLAN_CLI_MAX);

  ret = nsm_customer_edge_port_set_def_svid (ifp, default_evc, mode, sub_mode);

  if (ret < 0)
    {
      return nsm_vlan_cli_return (cli, ret, NULL, NULL, default_evc);
    }

  return CLI_SUCCESS;
}

CLI (no_nsm_vlan_switchport_ce_default_evc,
  no_nsm_vlan_switchport_ce_default_evc_cmd,
  "no switchport customer-edge (hybrid|trunk) default-svlan",
  CLI_NO_STR,
  "Set the switching characteristics of the Layer2 interface",
  NSM_VLAN_CE_STR,
  NSM_VLAN_HYBRID_STR,
  NSM_VLAN_TRUNK_STR,
  "Set the default VLAN for the interface")

{
  int ret;
  struct interface *ifp = cli->index;
  enum nsm_vlan_port_mode mode = NSM_VLAN_PORT_MODE_INVALID;
  enum nsm_vlan_port_mode sub_mode = NSM_VLAN_PORT_MODE_INVALID;

#ifdef HAVE_LACPD
  NSM_INTERFACE_CHECK_AGG_MEM_PROPERTY(cli, ifp);
#endif /* HAVE_LACPD */
  NSM_BRIDGE_CHECK_L2_PROPERTY(cli, ifp);
  NSM_BRIDGE_CHECK_BRIDGE_PROPERTY(cli, ifp);

  mode = NSM_VLAN_PORT_MODE_CE;

  if (! pal_strncmp (argv[0], "h", 1))
    sub_mode = NSM_VLAN_PORT_MODE_HYBRID;
  else if (! pal_strncmp (argv[0], "t", 1))
    sub_mode = NSM_VLAN_PORT_MODE_TRUNK;
  else
    sub_mode = NSM_VLAN_PORT_MODE_INVALID;

  ret = nsm_customer_edge_port_unset_def_svid (ifp, mode, sub_mode);

  if (ret < 0)
    return nsm_vlan_cli_return (cli, ret, NULL, NULL, 0);

  return CLI_SUCCESS;
}

#endif

     
CLI (nsm_br_vlan_name,
     nsm_br_vlan_name_cmd,
     "vlan "NSM_VLAN_CLI_RNG" bridge <1-32> name WORD (state (enable|disable)|)",
     CLI_VLAN_STR,
     "VLAN id",
     NSM_BRIDGE_STR,
     NSM_BRIDGE_NAME_STR,
     "Ascii name of the VLAN",
     "The ascii name of the VLAN",
     "Operational state of the VLAN",
     "Enable",
     "Disable")
{
  int ret;
  int brid;
  u_int16_t vid;
  u_int8_t vlan_type;
  struct nsm_master *nm = cli->vr->proto;
  struct nsm_bridge_master *master = nsm_bridge_get_master (nm);

  /* Get the vid and check if it is within range (2-4096).  */
  CLI_GET_INTEGER_RANGE ("VLAN id", vid, argv[0], NSM_VLAN_CLI_MIN, NSM_VLAN_CLI_MAX);

  /* Get the bridge group number and see if its within range. */
  CLI_GET_INTEGER_RANGE ("Bridge group", brid, argv[1], 1, 32);

  vlan_type = NSM_VLAN_STATIC | NSM_VLAN_CVLAN;

  /* Check if VLAN name size does not exceed 16 characters */
  if (pal_strlen (argv[2]) > NSM_VLAN_NAME_MAX)
    {
      cli_out (cli, "%% VLAN-NAME size should not exceed 16\n");
      return CLI_ERROR;
    }

  /* Check for the state. */
  if (argc > 3)
    {
      if (! pal_strncmp (argv[4], "e", 1))
        /* Create the VLAN.  */
        ret = nsm_vlan_add (master, argv[1], argv[2], vid,
                            NSM_VLAN_ACTIVE, vlan_type);
      else
        ret = nsm_vlan_add (master, argv[1], argv[2], vid,
                            NSM_VLAN_DISABLED, vlan_type);
    }
  else
    ret = nsm_vlan_add (master, argv[1], argv[2], vid,
                        NSM_VLAN_ACTIVE, vlan_type);

  if (ret < 0)
    return nsm_vlan_cli_return (cli, ret, NULL, NULL, vid);

  return CLI_SUCCESS;
}

CLI (nsm_vlan_name,
     nsm_vlan_name_cmd,
     "vlan "NSM_VLAN_CLI_RNG" name WORD (state (enable|disable)|)",
     CLI_VLAN_STR,
     "VLAN id",
     "Ascii name of the VLAN",
     "The ascii name of the VLAN",
     "Operational state of the VLAN",
     "Enable",
     "Disable")
{
  int ret;
  u_int16_t vid;
  u_int8_t vlan_type;
  struct nsm_master *nm = cli->vr->proto;
  struct nsm_bridge *bridge;
  struct nsm_bridge_master *master = nsm_bridge_get_master (nm);

  /* Get the vid and check if it is within range (2-4096).  */
  CLI_GET_INTEGER_RANGE ("VLAN id", vid, argv[0], NSM_VLAN_CLI_MIN, NSM_VLAN_CLI_MAX);

  vlan_type = NSM_VLAN_STATIC | NSM_VLAN_CVLAN;

  /* Check if VLAN name size does not exceed 16 characters */
  if (pal_strlen (argv[1]) > NSM_VLAN_NAME_MAX)
    {
      cli_out (cli, "%% VLAN-NAME size should not exceed 16\n");
      return CLI_ERROR;
    }

  bridge = nsm_get_default_bridge (master);

  if (! bridge)
    {
      cli_out (cli, "%% Default Bridge not found\n");
      return CLI_ERROR;
    }

  /* Check for the state. */
  if (argc > 3)
    {
      if (! pal_strncmp (argv[3], "e", 1))
        /* Create the VLAN.  */
        ret = nsm_vlan_add (master, bridge->name, argv[1], vid,
                            NSM_VLAN_ACTIVE, vlan_type);
      else
        ret = nsm_vlan_add (master, bridge->name, argv[1], vid,
                            NSM_VLAN_DISABLED, vlan_type);
    }
  else
    ret = nsm_vlan_add (master, bridge->name, argv[1], vid,
                        NSM_VLAN_ACTIVE, vlan_type);

  if (ret < 0)
    return nsm_vlan_cli_return (cli, ret, NULL, NULL, vid);

  return CLI_SUCCESS;

}

CLI (nsm_vlan_no_name,
     nsm_vlan_no_name_cmd,
     "vlan <2-4094>",
     CLI_VLAN_STR,
     "VLAN id")
{
  int ret;
  int brid;
  u_int16_t vid;
  u_int8_t vlan_type;
  struct nsm_bridge *bridge;
  struct nsm_master *nm = cli->vr->proto;
  struct nsm_bridge_master *master = nsm_bridge_get_master (nm);

  /* Get the vid and check if it is within range (2-4094).  */
  CLI_GET_INTEGER_RANGE ("VLAN id", vid, argv[0], NSM_VLAN_CLI_MIN, NSM_VLAN_CLI_MAX);

  vlan_type = NSM_VLAN_STATIC | NSM_VLAN_CVLAN;

  if (argc == 2)
    {
      /* Get the bridge group number and see if its within range. */
      CLI_GET_INTEGER_RANGE ("Bridge group", brid, argv[1], 1, 32);

      /* Create the VLAN.  */
      ret = nsm_vlan_add (master, argv[1], NULL, vid,  NSM_VLAN_ACTIVE,
                          vlan_type);
    }
  else
    {
      bridge = nsm_get_default_bridge (master);

     if (! bridge)
       {
         cli_out (cli, "%% Default Bridge not found\n");
         return CLI_ERROR;
       }

      /* Create the VLAN.  */
      ret = nsm_vlan_add (master, bridge->name, NULL, vid,  NSM_VLAN_ACTIVE,
                          vlan_type);
    }

  if (ret < 0)
    return nsm_vlan_cli_return (cli, ret, NULL, NULL, vid);

  return CLI_SUCCESS;
}

ALI (nsm_vlan_no_name,
     nsm_br_vlan_no_name_cmd,
     "vlan "NSM_VLAN_CLI_RNG" bridge <1-32>",
     CLI_VLAN_STR,
     "VLAN id",
     NSM_VLAN_STR,
     NSM_BRIDGE_STR,
     NSM_BRIDGE_NAME_STR);

CLI (no_nsm_vlan_no_name,
     no_nsm_vlan_no_name_cmd,
     "no vlan <2-4094>",
     CLI_NO_STR,
     CLI_VLAN_STR,
     "VLAN id")
{
  int ret;
  int brid;
  u_int16_t vid;
  u_int8_t vlan_type;
  struct nsm_bridge *bridge;
  struct nsm_master *nm = cli->vr->proto;
  struct nsm_bridge_master *master = nsm_bridge_get_master (nm);

  /* Get the vid and check if it is within range (2-4096).  */
  CLI_GET_INTEGER_RANGE ("VLAN id", vid, argv[0], NSM_VLAN_CLI_MIN, NSM_VLAN_CLI_MAX);

  vlan_type = NSM_VLAN_STATIC | NSM_VLAN_CVLAN;

  if (argc == 2)
    {
      /* Get the bridge group number and see if its within range. */
      CLI_GET_INTEGER_RANGE ("Bridge group", brid, argv[1], 1, 32);

      /* Delete VLAN. */
      ret = nsm_vlan_delete (master, argv[1], vid, vlan_type);

    }
  else
    {
      bridge = nsm_get_default_bridge (master);

     if (! bridge)
       {
         cli_out (cli, "%% Default Bridge not found\n");
         return CLI_ERROR;
       }

      /* Delete VLAN. */
      ret = nsm_vlan_delete (master, bridge->name, vid, vlan_type);

    }

  return nsm_vlan_cli_return (cli, ret, NULL, NULL, NSM_VLAN_NULL_VID);
}

ALI (no_nsm_vlan_no_name,
     no_nsm_br_vlan_no_name_cmd,
     "no vlan "NSM_VLAN_CLI_RNG" bridge <1-32>",
     CLI_NO_STR,
     CLI_VLAN_STR,
     "VLAN id",
     NSM_BRIDGE_STR,
     NSM_BRIDGE_NAME_STR);

CLI (nsm_vlan_enable_disable,
     nsm_vlan_enable_disable_cmd,
     "vlan "NSM_VLAN_CLI_RNG" state (enable|disable)",
     CLI_VLAN_STR,
     "VLAN id",
     "Operational state of the VLAN",
     "Enable",
     "Disable")
{
  int ret;
  u_int16_t vid;
  u_char vlan_type;
  struct nsm_bridge *bridge;
  struct nsm_master *nm = cli->vr->proto;
  struct nsm_bridge_master *master = nsm_bridge_get_master (nm);

  /* Get the vid and check if it is within range (2-4096).  */
  CLI_GET_INTEGER_RANGE ("VLAN id", vid, argv[0], NSM_VLAN_CLI_MIN, NSM_VLAN_CLI_MAX);

  vlan_type = NSM_VLAN_STATIC | NSM_VLAN_CVLAN;

  bridge = nsm_get_default_bridge (master);

  if (! bridge)
    {
      cli_out (cli, "%% Default Bridge not found\n");
      return CLI_ERROR;
    }

  /* Update the VLAN. */
  if (! pal_strncmp (argv[1], "e", 1))
    ret = nsm_vlan_add (master, bridge->name, NULL, vid, NSM_VLAN_ACTIVE,
                        vlan_type);
  else
    ret = nsm_vlan_add (master, bridge->name, NULL, vid, NSM_VLAN_DISABLED,
                        vlan_type);

  if (ret < 0)
   return nsm_vlan_cli_return (cli, ret, NULL, NULL, vid);

  return CLI_SUCCESS;
}

CLI (nsm_br_vlan_enable_disable,
     nsm_br_vlan_enable_disable_cmd,
     "vlan "NSM_VLAN_CLI_RNG" bridge <1-32> state (enable|disable)",
     CLI_VLAN_STR,
     "VLAN id",
     NSM_BRIDGE_STR,
     NSM_BRIDGE_NAME_STR,
     "Operational state of the VLAN",
     "Enable",
     "Disable")
{
  int ret;
  int brid;
  u_int16_t vid;
  u_int8_t vlan_type;
  struct nsm_master *nm = cli->vr->proto;
  struct nsm_bridge_master *master = nsm_bridge_get_master (nm);

  /* Get the vid and check if it is within range (2-4096).  */
  CLI_GET_INTEGER_RANGE ("VLAN id", vid, argv[0], NSM_VLAN_CLI_MIN, NSM_VLAN_CLI_MAX);

  /* Get the brid and check if it is within range. */
  CLI_GET_INTEGER_RANGE ("Bridge group", brid, argv[1], 1, 32);

  vlan_type = NSM_VLAN_STATIC | NSM_VLAN_CVLAN;

  /* Update the VLAN. */
  if (! pal_strncmp (argv[2], "e", 1))
    ret = nsm_vlan_add (master, argv[1], NULL, vid, NSM_VLAN_ACTIVE, vlan_type);
  else
    ret = nsm_vlan_add (master, argv[1], NULL, vid, NSM_VLAN_DISABLED, vlan_type);

  if (ret < 0)
    return nsm_vlan_cli_return (cli, ret, NULL, NULL, vid);

  return CLI_SUCCESS;
}

CLI (nsm_vlan_mtu,
     nsm_vlan_mtu_cmd,
     "vlan "NSM_VLAN_CLI_RNG" mtu MTU_VAL",
     CLI_VLAN_STR,
     "VLAN id",
     "MTU Value",
     "The Maximum Transmission Unit value for the vlan")
{
  int ret;
  int brid;
  u_int16_t vid;
  u_int32_t mtu_val;
  u_int8_t vlan_type;
  struct nsm_bridge *bridge;
  struct nsm_master *nm = cli->vr->proto;
  struct nsm_bridge_master *master = nsm_bridge_get_master (nm);

  /* Get the vid and check if it is within range (2-4096).  */
  CLI_GET_INTEGER_RANGE ("VLAN id", vid, argv[0], NSM_VLAN_CLI_MIN, NSM_VLAN_CLI_MAX);

  /* Get the value of the MTU */

  CLI_GET_INTEGER_RANGE ("MTU Value", mtu_val, argv[1], 1, UINT32_MAX);

  vlan_type = NSM_VLAN_STATIC | NSM_VLAN_CVLAN;

  if (argc == 3)
    {
      /* Get the bridge group number and see if its within range. */

      CLI_GET_INTEGER_RANGE ("Bridge group", brid, argv[2], 1, 32);

      /* Set the vlaue of MTU for the VLAN.  */

      ret = nsm_vlan_set_mtu (master, argv[2], vid, vlan_type, mtu_val);
    }
  else
    {
      bridge = nsm_get_default_bridge (master);

      if (! bridge)
        {
          cli_out (cli, "%% Default Bridge not found\n");
          return CLI_ERROR;
        }

      /* Set the vlaue of MTU for the VLAN.  */

      ret = nsm_vlan_set_mtu (master, bridge->name, vid, vlan_type, mtu_val);
    }

  if (ret < 0)
    {
      if (ret == NSM_VLAN_ERR_BRIDGE_NOT_FOUND)
        cli_out (cli, "%% Brige-group Not Found \n");
      else if (ret == NSM_VLAN_ERR_VLAN_NOT_FOUND)
        cli_out (cli, "%% VLAN Not a member of the bridge-group\n");
      else
        cli_out (cli, "%% Set VLAN MTU failed\n");
      return CLI_ERROR;
    }

  return CLI_SUCCESS;

}

ALI (nsm_vlan_mtu,
     nsm_br_vlan_mtu_cmd,
     "vlan "NSM_VLAN_CLI_RNG" mtu MTU_VAL bridge <1-32>",
     CLI_VLAN_STR,
     "VLAN id",
     "MTU Value",
     "The Maximum Transmission Unit value for the vlan",
     NSM_BRIDGE_STR,
     NSM_BRIDGE_NAME_STR);


CLI (no_nsm_vlan_mtu,
     no_nsm_vlan_mtu_cmd,
     "no vlan "NSM_VLAN_CLI_RNG" mtu",
     CLI_NO_STR,
     CLI_VLAN_STR,
     "VLAN id",
     "Reset the MMaximum Transmission Unit value for the vlan")
{
  int ret;
  int brid;
  u_int16_t vid;
  u_int8_t vlan_type;
  u_int32_t mtu_val = 0;
  struct nsm_bridge *bridge;
  struct nsm_master *nm = cli->vr->proto;
  struct nsm_bridge_master *master = nsm_bridge_get_master (nm);

  /* Get the vid and check if it is within range (2-4096).  */
  CLI_GET_INTEGER_RANGE ("VLAN id", vid, argv[0], 2, NSM_VLAN_CLI_MAX);

  vlan_type = NSM_VLAN_STATIC | NSM_VLAN_CVLAN;

  if (argc == 3)
    {
      /* Get the bridge group number and see if its within range. */

      CLI_GET_INTEGER_RANGE ("Bridge group", brid, argv[1], 1, 32);

      /* Set the vlaue of MTU for the VLAN.  */

      ret = nsm_vlan_set_mtu (master, argv[1], vid, vlan_type, mtu_val);
    }
  else
    {
      bridge = nsm_get_default_bridge (master);

      if (! bridge)
        {
          cli_out (cli, "%% Default Bridge not found\n");
          return CLI_ERROR;
        }

      /* Set the vlaue of MTU for the VLAN.  */

      ret = nsm_vlan_set_mtu (master, bridge->name, vid, vlan_type, mtu_val);
    }

  if (ret < 0)
    {
      if (ret == NSM_VLAN_ERR_BRIDGE_NOT_FOUND)
        cli_out (cli, "%% Port not attached to any bridge-group\n");
      else if (ret == NSM_VLAN_ERR_VLAN_NOT_FOUND)
        cli_out (cli, "%% VLAN Not a member of the bridge-group\n");
      else
        cli_out (cli, "%% Set VLAN MTU failed\n");
      return CLI_ERROR;
    }

  return CLI_SUCCESS;

}

ALI (no_nsm_vlan_mtu,
     no_nsm_br_vlan_mtu_cmd,
     "no vlan "NSM_VLAN_CLI_RNG" mtu bridge <1-32>",
     CLI_NO_STR,
     CLI_VLAN_STR,
     "VLAN id",
     "Reset the MMaximum Transmission Unit value for the vlan",
     NSM_BRIDGE_STR,
     NSM_BRIDGE_NAME_STR);


CLI (nsm_vlan_switchport_mode,
     nsm_vlan_switchport_mode_cmd,
     "switchport mode (access|hybrid|trunk|provider-network|customer-network)",
     "Set the switching characteristics of the Layer2 interface",
     "Set the mode of the Layer2 interface",
     NSM_VLAN_ACCESS_STR,
     NSM_VLAN_HYBRID_STR,
     NSM_VLAN_TRUNK_STR,
     NSM_VLAN_PN_STR,
     NSM_VLAN_CN_STR
     )
{
  int ret;
  struct interface *ifp = cli->index;
  enum nsm_vlan_port_mode mode = NSM_VLAN_PORT_MODE_INVALID;
  enum nsm_vlan_port_mode sub_mode = NSM_VLAN_PORT_MODE_INVALID;

#ifdef HAVE_LACPD
  NSM_INTERFACE_CHECK_AGG_MEM_PROPERTY(cli, ifp);
#endif /* HAVE_LACPD */
  NSM_BRIDGE_CHECK_L2_PROPERTY(cli, ifp);
  NSM_BRIDGE_CHECK_BRIDGE_PROPERTY(cli, ifp);

  if (! pal_strncmp (argv[0], "a", 1))
    mode = NSM_VLAN_PORT_MODE_ACCESS;
  else if (! pal_strncmp (argv[0], "h", 1))
    mode = NSM_VLAN_PORT_MODE_HYBRID;
  else if (! pal_strncmp (argv[0], "t", 1))
    mode = NSM_VLAN_PORT_MODE_TRUNK;
  else if (! pal_strncmp (argv[0], "p", 1))
    mode = NSM_VLAN_PORT_MODE_PN;
  else if (! pal_strncmp (argv[0], "c", 1))
    mode = NSM_VLAN_PORT_MODE_CN;
  
  sub_mode = mode;
  
  ret = nsm_vlan_api_set_port_mode (ifp, mode, sub_mode, PAL_TRUE, PAL_TRUE);

  if (ret < 0)
    {
      if (ret == NSM_VLAN_ERR_BRIDGE_NOT_FOUND)
        cli_out (cli, "%% Port not attached to any bridge-group\n");
      else if (ret == NSM_VLAN_ERR_INVALID_MODE)
        cli_out (cli, "%% Invalid switchport mode\n");
      else if (ret == NSM_VLAN_ERR_MODE_CLEAR)
        cli_out (cli, "%% Error clearing output VLAN of previous mode\n");
#ifdef HAVE_LACPD
      else if (ret == AGG_MEM_NO_SWITCHPORT)
        cli_out (cli, "%% Member interface not configured for switching\n");
      else if (ret == AGG_MEM_BRIDGE_NOT_VLAN_AWARE)
        cli_out (cli, "%% Member interface not vlan aware\n");
#endif /* HAVE LACP */
      else if (ret == NSM_VLAN_ERR_CVLAN_REG_EXIST)
        cli_out (cli, "%% Unconfigure existing CVLAN Registration Table\n");
#if (defined HAVE_MPLS_VC || defined HAVE_VPLS)
      else if (ret == NSM_VLAN_ERR_VC_BOUND)
        cli_out (cli, "%% Please unbind vc/vpls first \n");
#endif /* HAVE_MPLS_VC || HAVE_VPLS */
      else
        cli_out (cli, "%% Set port mode failed\n");
      return CLI_ERROR;
    }

  return CLI_SUCCESS;
}

#if defined HAVE_I_BEB && !defined HAVE_B_BEB
CLI (nsm_vlan_switchport_mode_pbb_i,
     nsm_vlan_switchport_mode_pbb_i_cmd,
     "switchport mode (cnp|pip)",
     "Set the switching characteristics of the Layer2 interface",
     "Set the mode of the Layer2 interface",
     NSM_VLAN_CNP_STR,
     NSM_VLAN_PIP_STR
     )
{
  int ret;
  struct interface *ifp = cli->index;
  enum nsm_vlan_port_mode mode = NSM_VLAN_PORT_MODE_INVALID;
  enum nsm_vlan_port_mode sub_mode = NSM_VLAN_PORT_MODE_INVALID;

#ifdef HAVE_LACPD
  NSM_INTERFACE_CHECK_AGG_MEM_PROPERTY(cli, ifp);
#endif /* HAVE_LACPD */
  NSM_BRIDGE_CHECK_L2_PROPERTY(cli, ifp);
  NSM_BRIDGE_CHECK_BRIDGE_PROPERTY(cli, ifp);

  if (argv[0][0] =='c')
   {
       mode = NSM_VLAN_PORT_MODE_CNP;
       sub_mode = NSM_VLAN_PORT_MODE_HYBRID;
   }
  else 
   {
       mode = NSM_VLAN_PORT_MODE_PIP;
       sub_mode = NSM_VLAN_PORT_MODE_TRUNK;
   }

  ret = nsm_vlan_api_set_port_mode (ifp, mode, sub_mode, PAL_TRUE, PAL_TRUE);

  if (ret < 0)
    {
      if (ret == NSM_VLAN_ERR_BRIDGE_NOT_FOUND)
        cli_out (cli, "%% Port not attached to any bridge-group\n");
      else if (ret == NSM_VLAN_ERR_INVALID_MODE)
        cli_out (cli, "%% Invalid switchport mode\n");
      else if (ret == NSM_VLAN_ERR_MODE_CLEAR)
        cli_out (cli, "%% Error clearing output VLAN of previous mode\n");
#ifdef HAVE_LACPD
      else if (ret == AGG_MEM_NO_SWITCHPORT)
        cli_out (cli, "%% Member interface not configured for switching\n");
      else if (ret == AGG_MEM_BRIDGE_NOT_VLAN_AWARE)
        cli_out (cli, "%% Member interface not vlan aware\n");
#endif /* HAVE LACP */
      else if (ret == NSM_VLAN_ERR_CVLAN_REG_EXIST)
        cli_out (cli, "%% Unconfigure existing CVLAN Registration Table\n");
#if (defined HAVE_MPLS_VC || defined HAVE_VPLS)
      else if (ret == NSM_VLAN_ERR_VC_BOUND)
        cli_out (cli, "%% Please unbind vc/vpls first \n");
#endif /* HAVE_MPLS_VC || HAVE_VPLS */
      else if (ret == NSM_VLAN_ERR_PBB_CONFIG_EXIST)
        cli_out (cli, "%% Please unconfigure existing mode PBB configuation first \n");
      else
        cli_out (cli, "%% Set port mode failed\n");
      return CLI_ERROR;
    }

  return CLI_SUCCESS;
}
#endif/*HAVE_I_BEB && !HAVE_B_BEB*/

#if defined HAVE_I_BEB && defined HAVE_B_BEB
CLI (nsm_vlan_switchport_mode_pbb_ib,
     nsm_vlan_switchport_mode_pbb_ib_cmd,
     "switchport mode (cnp|pip|cbp|pnp)",
     "Set the switching characteristics of the Layer2 interface",
     "Set the mode of the Layer2 interface",
     NSM_VLAN_CNP_STR,
     NSM_VLAN_PNP_STR,
     NSM_VLAN_CBP_STR,
     NSM_VLAN_PNP_STR
     )
{
  int ret;
  struct interface *ifp = cli->index;
  enum nsm_vlan_port_mode mode = NSM_VLAN_PORT_MODE_INVALID;
  enum nsm_vlan_port_mode sub_mode = NSM_VLAN_PORT_MODE_INVALID;

#ifdef HAVE_LACPD
  NSM_INTERFACE_CHECK_AGG_MEM_PROPERTY(cli, ifp);
#endif /* HAVE_LACPD */
  NSM_BRIDGE_CHECK_L2_PROPERTY(cli, ifp);
  NSM_BRIDGE_CHECK_BRIDGE_PROPERTY(cli, ifp);

  if (argv[0][0] =='c' && argv[0][1] =='n')
   {
       mode = NSM_VLAN_PORT_MODE_CNP;
       sub_mode = NSM_VLAN_PORT_MODE_HYBRID;
   }
  else if (argv[0][1] =='b')
   {
       mode = NSM_VLAN_PORT_MODE_CBP;
       sub_mode = NSM_VLAN_PORT_MODE_TRUNK;
   }
  else if (argv[0][1] =='i')
   {
       mode = NSM_VLAN_PORT_MODE_PIP;
       sub_mode = NSM_VLAN_PORT_MODE_TRUNK;
   }
  else
   {
       mode = NSM_VLAN_PORT_MODE_PNP;
       sub_mode = NSM_VLAN_PORT_MODE_TRUNK;
   }


  ret = nsm_vlan_api_set_port_mode (ifp, mode, sub_mode, PAL_TRUE, PAL_TRUE);

  if (ret < 0)
    {
      if (ret == NSM_VLAN_ERR_BRIDGE_NOT_FOUND)
        cli_out (cli, "%% Port not attached to any bridge-group\n");
      else if (ret == NSM_VLAN_ERR_INVALID_MODE)
        cli_out (cli, "%% Invalid switchport mode\n");
      else if (ret == NSM_VLAN_ERR_MODE_CLEAR)
        cli_out (cli, "%% Error clearing output VLAN of previous mode\n");
#ifdef HAVE_LACPD
      else if (ret == AGG_MEM_NO_SWITCHPORT)
        cli_out (cli, "%% Member interface not configured for switching\n");
      else if (ret == AGG_MEM_BRIDGE_NOT_VLAN_AWARE)
        cli_out (cli, "%% Member interface not vlan aware\n");
#endif /* HAVE LACP */
      else if (ret == NSM_VLAN_ERR_CVLAN_REG_EXIST)
        cli_out (cli, "%% Unconfigure existing CVLAN Registration Table\n");
#if (defined HAVE_MPLS_VC || defined HAVE_VPLS)
      else if (ret == NSM_VLAN_ERR_VC_BOUND)
        cli_out (cli, "%% Please unbind vc/vpls first \n");
#endif /* HAVE_MPLS_VC || HAVE_VPLS */
      else if (ret == NSM_VLAN_ERR_PBB_CONFIG_EXIST)
        cli_out (cli, "%% Please unconfigure existing mode PBB configuation first \n");
      else
        cli_out (cli, "%% Set port mode failed\n");
      return CLI_ERROR;
    }

  return CLI_SUCCESS;
}
#endif/*HAVE_I_BEB && HAVE_B_BEB*/

#if !defined HAVE_I_BEB && defined HAVE_B_BEB
CLI (nsm_vlan_switchport_mode_pbb_b,
     nsm_vlan_switchport_mode_pbb_b_cmd,
     "switchport mode (cbp|pnp)",
     "Set the switching characteristics of the Layer2 interface",
     "Set the mode of the Layer2 interface",
     NSM_VLAN_CBP_STR,
     NSM_VLAN_PNP_STR
     )
{
  int ret;
  struct interface *ifp = cli->index;
  enum nsm_vlan_port_mode mode = NSM_VLAN_PORT_MODE_INVALID;
  enum nsm_vlan_port_mode sub_mode = NSM_VLAN_PORT_MODE_INVALID;

#ifdef HAVE_LACPD
  NSM_INTERFACE_CHECK_AGG_MEM_PROPERTY(cli, ifp);
#endif /* HAVE_LACPD */
  NSM_BRIDGE_CHECK_L2_PROPERTY(cli, ifp);
  NSM_BRIDGE_CHECK_BRIDGE_PROPERTY(cli, ifp);

  if (argv[0][0] =='c')
   {
       mode = NSM_VLAN_PORT_MODE_CBP;
       sub_mode = NSM_VLAN_PORT_MODE_TRUNK;
   }
  else 
   {
       mode = NSM_VLAN_PORT_MODE_PNP;
       sub_mode = NSM_VLAN_PORT_MODE_TRUNK;
   }

  ret = nsm_vlan_api_set_port_mode (ifp, mode, sub_mode, PAL_TRUE, PAL_TRUE);

  if (ret < 0)
    {
      if (ret == NSM_VLAN_ERR_BRIDGE_NOT_FOUND)
        cli_out (cli, "%% Port not attached to any bridge-group\n");
      else if (ret == NSM_VLAN_ERR_INVALID_MODE)
        cli_out (cli, "%% Invalid switchport mode\n");
      else if (ret == NSM_VLAN_ERR_MODE_CLEAR)
        cli_out (cli, "%% Error clearing output VLAN of previous mode\n");
#ifdef HAVE_LACPD
      else if (ret == AGG_MEM_NO_SWITCHPORT)
        cli_out (cli, "%% Member interface not configured for switching\n");
      else if (ret == AGG_MEM_BRIDGE_NOT_VLAN_AWARE)
        cli_out (cli, "%% Member interface not vlan aware\n");
#endif /* HAVE LACP */
      else if (ret == NSM_VLAN_ERR_CVLAN_REG_EXIST)
        cli_out (cli, "%% Unconfigure existing CVLAN Registration Table\n");
#if (defined HAVE_MPLS_VC || defined HAVE_VPLS)
      else if (ret == NSM_VLAN_ERR_VC_BOUND)
        cli_out (cli, "%% Please unbind vc/vpls first \n");
#endif /* HAVE_MPLS_VC || HAVE_VPLS */
      else if (ret == NSM_VLAN_ERR_PBB_CONFIG_EXIST)
        cli_out (cli, "%% Please unconfigure existing mode PBB configuation first \n");
      else
        cli_out (cli, "%% Set port mode failed\n");
      return CLI_ERROR;
    }

  return CLI_SUCCESS;
}
#endif/*!HAVE_I_BEB && HAVE_B_BEB*/



#ifdef HAVE_PROVIDER_BRIDGE

CLI (nsm_vlan_switchport_ingress_filter_enable_ce,
     nsm_vlan_switchport_ingress_filter_enable_ce_cmd,
     "switchport mode customer-edge (access|hybrid|trunk) "
     "ingress-filter (enable|disable)",
     "Set the switching characteristics of the Layer2 interface",
     "Set the mode of the Layer2 interface",
     NSM_VLAN_CE_STR,
     NSM_VLAN_ACCESS_STR,
     NSM_VLAN_HYBRID_STR,
     NSM_VLAN_TRUNK_STR,
     "Set the ingress filtering of the frames received",
     "Enable ingress filtering",
     "Disable ingress filtering")
{
  int ret;
  enum nsm_vlan_port_mode mode;
  enum nsm_vlan_port_mode sub_mode;
  struct interface *ifp = cli->index;
  int enable = 0;

#ifdef HAVE_LACPD
  NSM_INTERFACE_CHECK_AGG_MEM_PROPERTY(cli, ifp);
#endif /* HAVE_LACPD */
  NSM_BRIDGE_CHECK_L2_PROPERTY(cli, ifp);
  NSM_BRIDGE_CHECK_BRIDGE_PROPERTY(cli, ifp);

  mode = NSM_VLAN_PORT_MODE_CE;

  if (! pal_strncmp (argv[0], "a", 1))
    sub_mode = NSM_VLAN_PORT_MODE_ACCESS;
  else if (! pal_strncmp (argv[0], "h", 1))
    sub_mode = NSM_VLAN_PORT_MODE_HYBRID;
  else if (! pal_strncmp (argv[0], "t", 1))
    sub_mode = NSM_VLAN_PORT_MODE_TRUNK;
  else
    sub_mode = NSM_VLAN_PORT_MODE_INVALID;

  if (! pal_strncmp (argv[1], "e", 1))
    enable = 1;
  else
    enable = 0;

  ret = nsm_vlan_set_ingress_filter (ifp, mode, sub_mode, enable);

  if (ret < 0)
    {
      cli_out (cli, "%% Can't set ingress filtering\n");
      return CLI_ERROR;
    }

  return CLI_SUCCESS;
}

#endif /* HAVE_PROVIDER_BRIDGE */

CLI (nsm_vlan_switchport_ingress_filter_enable,
     nsm_vlan_switchport_ingress_filter_enable_cmd,
     "switchport mode (access|hybrid|trunk|provider-network|customer-network) "
     "ingress-filter (enable|disable)",
     "Set the switching characteristics of the Layer2 interface",
     "Set the mode of the Layer2 interface",
     NSM_VLAN_ACCESS_STR,
     NSM_VLAN_HYBRID_STR,
     NSM_VLAN_TRUNK_STR,
     NSM_VLAN_PN_STR,
     NSM_VLAN_CN_STR,
     "Set the ingress filtering of the frames received",
     "Enable ingress filtering",
     "Disable ingress filtering")
{
  int ret;
  int enable = 0;
  enum nsm_vlan_port_mode mode;
  enum nsm_vlan_port_mode sub_mode;
  struct interface *ifp = cli->index;

#ifdef HAVE_LACPD
  NSM_INTERFACE_CHECK_AGG_MEM_PROPERTY(cli, ifp);
#endif /* HAVE_LACPD */
  NSM_BRIDGE_CHECK_L2_PROPERTY(cli, ifp);
  NSM_BRIDGE_CHECK_BRIDGE_PROPERTY(cli, ifp);

  if (! pal_strncmp (argv[0], "a", 1))
    mode = NSM_VLAN_PORT_MODE_ACCESS;
  else if (! pal_strncmp (argv[0], "h", 1))
    mode = NSM_VLAN_PORT_MODE_HYBRID;
  else if (! pal_strncmp (argv[0], "t", 1))
    mode = NSM_VLAN_PORT_MODE_TRUNK;
  else if (! pal_strncmp (argv[0], "p", 1))
    mode = NSM_VLAN_PORT_MODE_PN;
  else if (! pal_strncmp (argv[0], "c", 1))
    mode = NSM_VLAN_PORT_MODE_CN;
  else
    mode = NSM_VLAN_PORT_MODE_INVALID;

  if (! pal_strncmp (argv[1], "e", 1))
    enable = 1;
  else
    enable = 0;

  sub_mode = mode;

  ret = nsm_vlan_set_ingress_filter (ifp, mode, sub_mode, enable);

  if (ret < 0)
    {
      cli_out (cli, "%% Can't set ingress filtering\n");
      return CLI_ERROR;
    }

  return CLI_SUCCESS;
}

CLI (nsm_vlan_switchport_hybrid_acceptable_frame,
     nsm_vlan_switchport_hybrid_acceptable_frame_cmd,
     "switchport mode (hybrid) acceptable-frame-type (all|vlan-tagged)",
     "Set the switching characteristics of the Layer2 interface",
     "Set the mode of the Layer2 interface",
     NSM_VLAN_HYBRID_STR,
     "Set the Layer2 interface acceptable frames types",
     "Set all frames can be received",
     "Set vlan-tagged frames can only be received")
{
  int ret;
  struct interface *ifp = cli->index;

#ifdef HAVE_LACPD
  NSM_INTERFACE_CHECK_AGG_MEM_PROPERTY(cli, ifp);
#endif /* HAVE_LACPD */
  NSM_BRIDGE_CHECK_L2_PROPERTY(cli, ifp);
  NSM_BRIDGE_CHECK_BRIDGE_PROPERTY(cli, ifp);

  if (! pal_strncmp (argv[1], "v", 1))
    ret = nsm_vlan_set_acceptable_frame_type (ifp, NSM_VLAN_PORT_MODE_HYBRID,
                                              NSM_VLAN_ACCEPTABLE_FRAME_TYPE_TAGGED);
  else
    ret = nsm_vlan_set_acceptable_frame_type (ifp, NSM_VLAN_PORT_MODE_HYBRID,
                                              ~NSM_VLAN_ACCEPTABLE_FRAME_TYPE_TAGGED);

  if (ret < 0)
    {
      cli_out (cli, "%% Can't set the acceptable frame type\n");
      return CLI_ERROR;
    }

  return CLI_SUCCESS;
}
#ifdef HAVE_G8032
#ifdef HAVE_B_BEB
CLI (g8032_beb_vlan_create,
     g8032_beb_vlan_create_cmd,
     "g8032 configure vlan ring-id RINGID bridge (<1-32> | backbone)",
     "parameters are G.8032 related",
     "Ring configuration",
     "configure vlan members",
     "unique id to identify a  Protection Ring",
     "Ring protection group ID",
      NSM_BRIDGE_STR,
      NSM_BRIDGE_NAME_STR,
      "backbone bridge")
{
    int brno = 0 ;
    int ringid = 0;
    struct nsm_master *nm = cli->vr->proto;
    struct nsm_bridge_master *master = nsm_bridge_get_master (nm);
    struct nsm_msg_g8032_vlan *pg_context = NULL;
    struct nsm_bridge *bridge;

    if (! master)
      {
        cli_out (cli, "%% Bridge master not configured\n");
        return CLI_ERROR;
      }
    if ( argc > 1)
      {
        CLI_GET_INTEGER_RANGE ("Max bridge groups", brno, argv[1], 1, 32);
        bridge = nsm_lookup_bridge_by_name (master, argv[1]);
      }
    else
      bridge = nsm_get_default_bridge (master);

    if (! bridge)
      {
        cli_out (cli, "%% Bridge not found \n");
        return CLI_ERROR;
      }
    
    CLI_GET_INTEGER_RANGE ("Max Rings ", ringid, argv[0], 1, 65535);
    pg_context = XCALLOC (MTYPE_NSM_G8032_VLAN, sizeof (struct nsm_msg_g8032_vlan));

    pal_mem_cpy(pg_context->bridge_name,bridge->name,strlen(bridge->name));
    pg_context->ring_id = ringid;

    cli->index = pg_context;
    cli->mode = G8032_CONFIG_VLAN_MODE;
    return CLI_SUCCESS;
}
  
#else
CLI (g8032_vlan_create,
     g8032_vlan_create_cmd,
     "g8032 configure vlan ring-id RINGID bridge <1-32>",
     "parameters are G.8032 related",
     "Ring  configuration",
     "configure vlan members",
     "unique id to identify a Protection Ring",
     "Ring group ID",
     NSM_BRIDGE_STR,
     NSM_BRIDGE_NAME_STR)
{
  int brno = 0 ;
  struct nsm_bridge *bridge = NULL;
  int ring_id = 0;
  struct nsm_msg_g8032_vlan *pg_context = NULL; 
  struct nsm_master *nm = cli->vr->proto;
  struct nsm_bridge_master *master = nsm_bridge_get_master (nm);
  
  if (!master)
    {
      cli_out (cli, "%% Bridge master not configured\n");
      return CLI_ERROR;
    }
  
  if (argc > 1)
    {
      CLI_GET_INTEGER_RANGE ("Max bridge groups", brno, argv[1], 1, 32);
      bridge = nsm_lookup_bridge_by_name (master, argv[1]);
    }
  else
    bridge = nsm_get_default_bridge (master);

  if (!bridge)
    {
      cli_out (cli, "%% Bridge not found \n");
      return CLI_ERROR;
    }

  CLI_GET_INTEGER_RANGE ("Max Ring groups", ring_id, argv[0], 1, 4094);

  pg_context = XCALLOC (MTYPE_NSM_G8032_VLAN, sizeof (struct nsm_msg_g8032_vlan));
  if (pg_context == NULL)
    return CLI_ERROR;
  
  pal_mem_cpy (pg_context->bridge_name,bridge->name,strlen(bridge->name));
  pg_context->ring_id = ring_id;
  cli->index  = pg_context;
  cli->mode = G8032_CONFIG_VLAN_MODE;

  return CLI_SUCCESS;
}
#endif  /* HAVE_B_BEB */
  
CLI (g8032_primary_vlan_create,
     g8032_primary_vlan_create_cmd,
     "g8032 vlan VID (primary|)",
     "parameters are G.8032 related",
     "configure vlan members",
     "VLAN ID",
     "Primary VLAN to an Ring protection group")
{
  int ret        = 0;
  u_int16_t vid  = 0;
  bool_t  is_primary_vlan = PAL_FALSE;
  struct nsm_msg_g8032_vlan *pg_instance = NULL; 
  struct nsm_master *nm = cli->vr->proto;


  pg_instance = (struct nsm_msg_g8032_vlan *)cli->index; 
  
  if (pg_instance == NULL)
    {
      cli_out(cli, "%% CLI error");
      return CLI_ERROR;
    }

  CLI_GET_INTEGER_RANGE ("VLAN id", vid, argv[0], NSM_VLAN_CLI_MIN,
                          NSM_VLAN_CLI_MAX);

   if (argc > 1)
    {
      if ( argv[1][0] == 'p' )
      is_primary_vlan = PAL_TRUE;
    }

  ret = nsm_bridge_g8032_create_vlan (vid,
                                      pg_instance,
                                      is_primary_vlan,
                                      nm);
  switch (ret)
    {
    case NSM_BRIDGE_ERR_NOTFOUND:
      cli_out (cli,"%% Bridge not found %s\n",pg_instance->bridge_name);
      return CLI_ERROR;

    case NSM_BRIDGE_ERR_RING_NOTFOUND:
      cli_out (cli,"%% Ring not found %d\n",pg_instance->ring_id);
      return CLI_ERROR;

    case NSM_INTERFACE_NOT_FOUND:
      cli_out (cli,"%% Interface not found\n");
      return CLI_ERROR;

    case NSM_BRIDGE_ERR_RING_VLAN_EXISTS:
      cli_out (cli,"%% VLAN  already exists %d\n",vid);
      return CLI_ERROR;

    case NSM_VLAN_ERR_IFP_NOT_BOUND :
      cli_out (cli,"%% VLAN is not bound to the interface\n");
      return CLI_ERROR;

    case NSM_VLAN_ERR_VLAN_NOT_FOUND:
      cli_out (cli, "%% VLAN %d not configured\n", vid);
      return CLI_ERROR;

    case RESULT_ERROR:
        cli_out (cli,"%% Error in assocating vlan and ring\n");
        return CLI_ERROR;

   case NSM_RING_PRIMARY_VID_ALREADY_EXISTS:
        cli_out (cli,"%% Ring vis already exists\n");
        return CLI_ERROR;

    case RESULT_OK:
        break;
    }

  return CLI_SUCCESS;
}
#endif /* HAVE_G8032 */

#ifdef HAVE_PROVIDER_BRIDGE

CLI (nsm_vlan_switchport_ce_acceptable_frame,
     nsm_vlan_switchport_ce_acceptable_frame_cmd,
     "switchport mode customer-edge "
     "hybrid acceptable-frame-type (all|vlan-tagged)",
     "Set the switching characteristics of the Layer2 interface",
     "Set the mode of the Layer2 interface",
     NSM_VLAN_CE_STR,
     NSM_VLAN_HYBRID_STR,
     "Set the Layer2 interface acceptable frames types",
     "Set all frames can be received",
     "Set vlan-tagged frames can only be received")
{
  int ret;
  enum nsm_vlan_port_mode mode;
  struct interface *ifp = cli->index;

  mode = NSM_VLAN_PORT_MODE_CE;

#ifdef HAVE_LACPD
  NSM_INTERFACE_CHECK_AGG_MEM_PROPERTY(cli, ifp);
#endif /* HAVE_LACPD */
  NSM_BRIDGE_CHECK_L2_PROPERTY(cli, ifp);
  NSM_BRIDGE_CHECK_BRIDGE_PROPERTY(cli, ifp);

  if (! pal_strncmp (argv[0], "v", 1))
    ret = nsm_vlan_set_acceptable_frame_type (ifp, mode,
                                              NSM_VLAN_ACCEPTABLE_FRAME_TYPE_TAGGED);
  else
    ret = nsm_vlan_set_acceptable_frame_type (ifp, mode,
                                              ~NSM_VLAN_ACCEPTABLE_FRAME_TYPE_TAGGED);

  if (ret < 0)
    {
      cli_out (cli, "%% Can't set the acceptable frame type\n");
      return CLI_ERROR;
    }

  return CLI_SUCCESS;

}

#endif /* HAVE_PROVIDER_BRIDGE */

CLI (nsm_vlan_switchport_pvid,
     nsm_vlan_switchport_pvid_cmd,
     "switchport (access|hybrid) vlan "NSM_VLAN_CLI_RNG,
     "Set the switching characteristics of the Layer2 interface",
     NSM_VLAN_ACCESS_STR,
     NSM_VLAN_HYBRID_STR,
     "Set the default VLAN for the interface",
     "The default VID for the interface")
{
  /* 802.1Q: Section 12.10.1.2 */
  int ret;
  unsigned short pvid;
  struct interface *ifp = cli->index;

#ifdef HAVE_LACPD
  NSM_INTERFACE_CHECK_AGG_MEM_PROPERTY(cli, ifp);
#endif /* HAVE_LACPD */
  NSM_BRIDGE_CHECK_L2_PROPERTY(cli, ifp);
  NSM_BRIDGE_CHECK_BRIDGE_PROPERTY(cli, ifp);

  CLI_GET_INTEGER_RANGE ("port vlanid", pvid, argv[1], NSM_VLAN_CLI_MIN, NSM_VLAN_CLI_MAX);

  if (! pal_strncmp (argv[0], "a", 1))
    {
      ret = nsm_vlan_set_access_port (ifp, pvid,
                                      PAL_TRUE, PAL_TRUE);
    }
  else if (! pal_strncmp (argv[0], "h", 1))
    {
      ret = nsm_vlan_set_hybrid_port (ifp, pvid,
                                      PAL_TRUE, PAL_TRUE);
    }
  else
    return CLI_ERROR;

  if (ret < 0)
    {
      if (ret == NSM_VLAN_ERR_IFP_NOT_BOUND)
        cli_out (cli, "%% Interface not bound to any bridge-group\n");
      else if (ret == NSM_VLAN_ERR_VLAN_NOT_FOUND)
        cli_out (cli, "%% VLAN %d not configured\n", pvid);
      else if (ret == NSM_VLAN_ERR_INVALID_MODE)
        cli_out (cli, "%% Port mode not set to access\n");
#ifdef HAVE_LACPD
      else if (ret == AGG_MEM_NO_SWITCHPORT)
        cli_out (cli, "%% Member interface not configured for switching\n");
      else if (ret == AGG_MEM_BRIDGE_NOT_VLAN_AWARE)
        cli_out (cli, "%% Member interface not vlan aware\n");
#endif /* HAVE LACP */
      else
        cli_out (cli, "%% Can't set the default VID\n");
      return CLI_ERROR;
    }

  return CLI_SUCCESS;
}

CLI (no_vlan_switchport_pvid,
     no_vlan_switchport_pvid_cmd,
     "no switchport (access | hybrid) vlan",
     CLI_NO_STR,
     "Reset the switching characteristics of the Layer2 interface",
     NSM_VLAN_ACCESS_STR,
     NSM_VLAN_HYBRID_STR,
     "Reset the default VLAN for the interface")
{
  int ret;
  struct interface *ifp = cli->index;

#ifdef HAVE_LACPD
  NSM_INTERFACE_CHECK_AGG_MEM_PROPERTY(cli, ifp);
#endif /* HAVE_LACPD */
  NSM_BRIDGE_CHECK_L2_PROPERTY(cli, ifp);
  NSM_BRIDGE_CHECK_BRIDGE_PROPERTY(cli, ifp);

  if (! pal_strncmp (argv[0], "a", 1))
    {
      ret = nsm_vlan_set_access_port (ifp, NSM_VLAN_DEFAULT_VID,
                                      PAL_TRUE, PAL_TRUE);
    }
  else if (! pal_strncmp (argv[0], "h", 1))
    {
      ret = nsm_vlan_set_hybrid_port (ifp, NSM_VLAN_DEFAULT_VID,
                                      PAL_TRUE, PAL_TRUE);
    }
  else
    return CLI_ERROR;

  if (ret < 0)
    {
      if (ret == NSM_VLAN_ERR_IFP_NOT_BOUND)
        cli_out (cli, "%% Interface not bound to any bridge-group\n");
      else if (ret == NSM_VLAN_ERR_VLAN_NOT_FOUND)
        cli_out (cli, "%% VLAN %d not configured\n", NSM_VLAN_DEFAULT_VID);
      else
        cli_out (cli, "%% Can't reset the default VID: %s\n", pal_strerror (ret)); 

      return CLI_ERROR;
    }

  return CLI_SUCCESS;
}

#ifdef HAVE_PROVIDER_BRIDGE

CLI (nsm_vlan_switchport_ce_pvid,
     nsm_vlan_switchport_ce_pvid_cmd,
     "switchport customer-edge (access|hybrid) vlan "NSM_VLAN_CLI_RNG,
     "Set the switching characteristics of the Layer2 interface",
     NSM_VLAN_CE_STR,
     NSM_VLAN_ACCESS_STR,
     NSM_VLAN_HYBRID_STR,
     "Set the default VLAN for the interface",
     "The default VID for the interface")
{
  /* 802.1Q: Section 12.10.1.2 */
  int ret;
  unsigned short pvid;
  struct interface *ifp = cli->index;
  enum nsm_vlan_port_mode mode = NSM_VLAN_PORT_MODE_INVALID;
  enum nsm_vlan_port_mode sub_mode = NSM_VLAN_PORT_MODE_INVALID;

#ifdef HAVE_LACPD
  NSM_INTERFACE_CHECK_AGG_MEM_PROPERTY(cli, ifp);
#endif /* HAVE_LACPD */
  NSM_BRIDGE_CHECK_L2_PROPERTY(cli, ifp);
  NSM_BRIDGE_CHECK_BRIDGE_PROPERTY(cli, ifp);


  mode = NSM_VLAN_PORT_MODE_CE;

  if (! pal_strncmp (argv[0], "a", 1))
    sub_mode = NSM_VLAN_PORT_MODE_ACCESS;
  else if (! pal_strncmp (argv[0], "h", 1))
    sub_mode = NSM_VLAN_PORT_MODE_HYBRID;
  else
    sub_mode = NSM_VLAN_PORT_MODE_INVALID;

  CLI_GET_INTEGER_RANGE ("port vlanid", pvid, argv[1], NSM_VLAN_CLI_MIN,
                         NSM_VLAN_CLI_MAX);

  ret = nsm_vlan_set_provider_port
                              (ifp, pvid, mode, sub_mode,
                               PAL_TRUE, PAL_TRUE);
  if (ret < 0)
    {
      if (ret == NSM_VLAN_ERR_IFP_NOT_BOUND)
        cli_out (cli, "%% Interface not bound to any bridge-group\n");
      else if (ret == NSM_VLAN_ERR_VLAN_NOT_FOUND)
        cli_out (cli, "%% VLAN %d not configured\n", pvid);
      else if (ret == NSM_VLAN_ERR_INVALID_MODE)
        cli_out (cli, "%% Port mode not set to access\n");
#ifdef HAVE_LACPD
      else if (ret == AGG_MEM_NO_SWITCHPORT)
        cli_out (cli, "%% Member interface not configured for switching\n");
      else if (ret == AGG_MEM_BRIDGE_NOT_VLAN_AWARE)
        cli_out (cli, "%% Member interface not vlan aware\n");
#endif /* HAVE LACP */
      else if (ret == NSM_VLAN_ERR_CVLAN_PORT_REG_EXIST)
        cli_out (cli, "%% Unconfigure existing CVLAN Registration Table for the VLAN %d\n", pvid);
      else
        cli_out (cli, "%% Can't set the default VID\n");
      return CLI_ERROR;
    }

  return CLI_SUCCESS;
}

CLI (no_nsm_vlan_switchport_ce_pvid,
     no_nsm_vlan_switchport_ce_pvid_cmd,
     "no switchport customer-edge (access|hybrid) vlan",
     CLI_NO_STR,
     "Reset the switching characteristics of the Layer2 interface",
     NSM_VLAN_CE_STR,
     NSM_VLAN_ACCESS_STR,
     NSM_VLAN_HYBRID_STR,
     "Reset the default VLAN for the interface")
{
  int ret;
  struct interface *ifp = cli->index;
  enum nsm_vlan_port_mode mode = NSM_VLAN_PORT_MODE_INVALID;
  enum nsm_vlan_port_mode sub_mode = NSM_VLAN_PORT_MODE_INVALID;

#ifdef HAVE_LACPD
  NSM_INTERFACE_CHECK_AGG_MEM_PROPERTY(cli, ifp);
#endif /* HAVE_LACPD */
  NSM_BRIDGE_CHECK_L2_PROPERTY(cli, ifp);
  NSM_BRIDGE_CHECK_BRIDGE_PROPERTY(cli, ifp);

  mode = NSM_VLAN_PORT_MODE_CE;

  if (! pal_strncmp (argv[0], "a", 1))
    sub_mode = NSM_VLAN_PORT_MODE_ACCESS;
  else if (! pal_strncmp (argv[0], "h", 1))
    sub_mode = NSM_VLAN_PORT_MODE_HYBRID;
  else
    sub_mode = NSM_VLAN_PORT_MODE_INVALID;

  ret = nsm_vlan_set_provider_port
                              (ifp, NSM_VLAN_DEFAULT_VID, mode, sub_mode,
                               PAL_TRUE, PAL_TRUE);

  if (ret < 0)
    {
      if (ret == NSM_VLAN_ERR_IFP_NOT_BOUND)
        cli_out (cli, "%% Interface not bound to any bridge-group\n");
      else if (ret == NSM_VLAN_ERR_VLAN_NOT_FOUND)
        cli_out (cli, "%% VLAN %d not configured\n", NSM_VLAN_DEFAULT_VID);
      else if (ret == NSM_VLAN_ERR_CVLAN_PORT_REG_EXIST)
        cli_out (cli, "%% Unconfigure existing CVLAN Registration Table "
                 "for the VLAN %d\n",
                 NSM_VLAN_DEFAULT_VID);
      else
        cli_out (cli, "%% Can't reset the default VID: %s\n", pal_strerror (ret));
      return CLI_ERROR;
    }

  return CLI_SUCCESS;
}

CLI (nsm_vlan_switchport_cn_pvid,
     nsm_vlan_switchport_cn_pvid_cmd,
     "switchport customer-network vlan "NSM_VLAN_CLI_RNG,
     "Set the switching characteristics of the Layer2 interface",
     NSM_VLAN_CN_STR,
     "Set the default VLAN for the interface",
     "The default VID for the interface")
{
  /* 802.1Q: Section 12.10.1.2 */
  int ret;
  unsigned short pvid;
  struct interface *ifp = cli->index;

#ifdef HAVE_LACPD
  NSM_INTERFACE_CHECK_AGG_MEM_PROPERTY(cli, ifp);
#endif /* HAVE_LACPD */
  NSM_BRIDGE_CHECK_L2_PROPERTY(cli, ifp);
  NSM_BRIDGE_CHECK_BRIDGE_PROPERTY(cli, ifp);

  /* customer-network is argv [0] because of the cli tree contrusction
   * for the whole VLAN cli commands. hence default vlan id is argv [1]
   */

  CLI_GET_INTEGER_RANGE ("port vlanid", pvid, argv[1], NSM_VLAN_CLI_MIN,
                         NSM_VLAN_CLI_MAX);

  ret = nsm_vlan_set_provider_port
                              (ifp, pvid,
                               NSM_VLAN_PORT_MODE_CN,
                               NSM_VLAN_PORT_MODE_CN,
                               PAL_TRUE, PAL_TRUE);


  if (ret < 0)
    {
      if (ret == NSM_VLAN_ERR_IFP_NOT_BOUND)
        cli_out (cli, "%% Interface not bound to any bridge-group\n");
      else if (ret == NSM_VLAN_ERR_VLAN_NOT_FOUND)
        cli_out (cli, "%% VLAN %d not configured\n", pvid);
      else if (ret == NSM_VLAN_ERR_INVALID_MODE)
        cli_out (cli, "%% Port mode not set to customer-network\n");
#ifdef HAVE_LACPD
      else if (ret == AGG_MEM_NO_SWITCHPORT)
        cli_out (cli, "%% Member interface not configured for switching\n");
      else if (ret == AGG_MEM_BRIDGE_NOT_VLAN_AWARE)
        cli_out (cli, "%% Member interface not vlan aware\n");
#endif /* HAVE LACP */
      else
        cli_out (cli, "%% Can't set the default VID\n");
      return CLI_ERROR;
    }

  return CLI_SUCCESS;
}

CLI (no_nsm_vlan_switchport_cn_pvid,
     no_nsm_vlan_switchport_cn_pvid_cmd,
     "no switchport customer-network vlan",
     CLI_NO_STR,
     "Reset the switching characteristics of the Layer2 interface",
     NSM_VLAN_CN_STR,
     "Reset the default VLAN for the interface")
{
  int ret;
  struct interface *ifp = cli->index;

#ifdef HAVE_LACPD
  NSM_INTERFACE_CHECK_AGG_MEM_PROPERTY(cli, ifp);
#endif /* HAVE_LACPD */
  NSM_BRIDGE_CHECK_L2_PROPERTY(cli, ifp);
  NSM_BRIDGE_CHECK_BRIDGE_PROPERTY(cli, ifp);

  ret = nsm_vlan_set_provider_port
                              (ifp, NSM_VLAN_DEFAULT_VID,
                               NSM_VLAN_PORT_MODE_CN,
                               NSM_VLAN_PORT_MODE_CN,
                               PAL_TRUE, PAL_TRUE);

  if (ret < 0)
    {
      if (ret == NSM_VLAN_ERR_IFP_NOT_BOUND)
        cli_out (cli, "%% Interface not bound to any bridge-group\n");
      else if (ret == NSM_VLAN_ERR_VLAN_NOT_FOUND)
        cli_out (cli, "%% VLAN %d not configured\n", NSM_VLAN_DEFAULT_VID);
      else
        cli_out (cli, "%% Can't reset the default VID: %s\n", pal_strerror (ret));

     return CLI_ERROR;
    }

  return CLI_SUCCESS;
}

#endif /* HAVE_PROVIDER_BRIDGE */

CLI (nsm_vlan_switchport_hybrid_add_egress,
     nsm_vlan_switchport_hybrid_add_egress_cmd,
     "switchport (hybrid) allowed vlan add VLAN_ID egress-tagged (enable|disable)",
     "Set the switching characteristics of the Layer2 interface",
     NSM_VLAN_HYBRID_STR,
     "Set the VLANs that will Xmit/Rx through the Layer2 interface",
     "VLAN that will be added ",
     "Add a VLAN to Xmit/Rx through the Layer2 interface",
     "The List of the VLAN IDs that will be added to the Layer2 interface",
     "Set the egress tagging for the outgoing frames",
     "Set the egress tagging on the outgoing frames to enabled",
     "Set the egress tagging on the outgoing frames to disabled")
{
  nsm_vid_t vid;
  int ret;
  int cli_ret = CLI_SUCCESS;
  struct interface *ifp = cli->index;
  char *curr = NULL;
  l2_range_parser_t par;
  l2_range_parse_error_t r_err;
  enum nsm_vlan_egress_type egress_tagged = NSM_VLAN_EGRESS_UNTAGGED;

#ifdef HAVE_LACPD
  NSM_INTERFACE_CHECK_AGG_MEM_PROPERTY(cli, ifp);
#endif /* HAVE_LACPD */
  NSM_BRIDGE_CHECK_L2_PROPERTY(cli, ifp);
  NSM_BRIDGE_CHECK_BRIDGE_PROPERTY(cli, ifp);

  if (! pal_strncmp (argv[2], "e", 1))
    egress_tagged = NSM_VLAN_EGRESS_TAGGED;
  else
    egress_tagged = NSM_VLAN_EGRESS_UNTAGGED;

  l2_range_parser_init (argv[1], &par);

  if ( (ret = l2_range_parser_validate (&par, cli, nsm_vlan_validate_vid))
       != CLI_SUCCESS)
    return ret;

  while ( (curr = l2_range_parser_get_next ( &par, &r_err) ) )
    {
      if ( r_err != RANGE_PARSER_SUCCESS )
        {
          cli_out (cli, "%% Invalid Input %s \n", l2_range_parse_err_to_str (r_err) );
          return CLI_ERROR;
        }

      CLI_GET_INTEGER_RANGE ("port vlanid", vid, curr, NSM_VLAN_CLI_MIN, NSM_VLAN_CLI_MAX);

      ret = nsm_vlan_add_hybrid_port (ifp, vid, egress_tagged,
                                      PAL_TRUE, PAL_TRUE);

      if (ret < 0)
        {
          if (ret == NSM_VLAN_ERR_IFP_NOT_BOUND)
            {
              cli_out (cli, "%% Interface not bound to any bridge-group\n");
              return CLI_ERROR;
            }
          else if (ret == NSM_VLAN_ERR_INVALID_MODE)
            {
              cli_out (cli, "%% Port mode not set to hybrid\n");
              return CLI_ERROR;
            }
#ifdef HAVE_LACPD
          else if (ret == AGG_MEM_NO_SWITCHPORT)
            {
              cli_out (cli, "%% Member interface not configured for switching\n");
              return CLI_ERROR;
            }
          else if (ret == AGG_MEM_BRIDGE_NOT_VLAN_AWARE)
            {
              cli_out (cli, "%% Member interface not vlan aware\n");
              return CLI_ERROR;
            }
#endif /* HAVE LACP */
          else if (ret == NSM_VLAN_ERR_CONFIG_PVID_TAG)
            {
              cli_out (cli, "%% Default vlan can't be tagged\n");
              return CLI_ERROR;
            }
          else if (ret == NSM_VLAN_ERR_VLAN_NOT_FOUND)
            {
              cli_out (cli, "%%Adding Port %s to non-existing vlan %d\n",
                       ifp->name, vid);

              return CLI_ERROR;
            }
          else
            {
              cli_out (cli, "%% Can't add vlan %d to port\n", vid);
              cli_ret = CLI_ERROR;
            }
        }
    }

  l2_range_parser_deinit (&par);

  return cli_ret;
}

CLI (nsm_vlan_switchport_hybrid_delete,
     nsm_vlan_switchport_hybrid_delete_cmd,
     "switchport (hybrid) allowed vlan remove VLAN_ID",
     "Set the switching characteristics of the Layer2 interface",
     NSM_VLAN_HYBRID_STR,
     "Set the VLANs that will Xmit/Rx through the Layer2 interface",
     "VLAN that will be removed",
     "Remove a VLAN to Xmit/Rx through the Layer2 interface",
     "The List of the VLAN IDs that will be removed from the Layer2 interface")
{
  nsm_vid_t vid;
  int ret;
  int cli_ret = CLI_SUCCESS;
  struct interface *ifp = cli->index;
  char *curr = NULL;
  l2_range_parser_t par;
  l2_range_parse_error_t r_err;
  struct nsm_if *zif;
  struct nsm_vlan_port *vlan_port;
  struct nsm_bridge_port *br_port;
  int vlan_port_config;  

#ifdef HAVE_LACPD
  NSM_INTERFACE_CHECK_AGG_MEM_PROPERTY(cli, ifp);
#endif /* HAVE_LACPD */
  NSM_BRIDGE_CHECK_L2_PROPERTY(cli, ifp);
  NSM_BRIDGE_CHECK_BRIDGE_PROPERTY(cli, ifp);

  l2_range_parser_init (argv[1], &par);

  if ( (ret = l2_range_parser_validate (&par, cli, nsm_vlan_validate_vid))
       != CLI_SUCCESS)
    return ret;

  zif = (struct nsm_if *)ifp->info;
 
  if (zif == NULL || zif->switchport == NULL)
    {
      cli_out (cli, "%% Interface not bound to bridge \n", ret);
      return CLI_ERROR;
    }
 
  br_port = zif->switchport;
  vlan_port = &br_port->vlan_port;
 
  vlan_port_config = vlan_port->config;
 
  while ( (curr = l2_range_parser_get_next ( &par, &r_err) ) )
    {
      if ( r_err != RANGE_PARSER_SUCCESS )
        {
          cli_out (cli, "%% Invalid Input %s \n", l2_range_parse_err_to_str (r_err) );
          return CLI_ERROR;
        }

      CLI_GET_INTEGER_RANGE ("port vlanid", vid, curr, NSM_VLAN_CLI_MIN, NSM_VLAN_CLI_MAX);

      ret = nsm_vlan_delete_hybrid_port (ifp, vid, PAL_TRUE, PAL_TRUE);

      if (ret < 0)
        {
          if (ret == NSM_VLAN_ERR_IFP_NOT_BOUND)
            {
              cli_out (cli, "%% Interface not bound to any bridge-group\n");
              return CLI_ERROR;
            }
          else if (ret == NSM_VLAN_ERR_INVALID_MODE)
            {
              cli_out (cli, "%% Port mode not set to hybrid\n");
              return CLI_ERROR;
            }
          else
            {
              cli_out (cli, "%% Can't remove vlan %d from port\n",vid);
              cli_ret = CLI_ERROR;
            }
        }
    }

  if (vlan_port_config == NSM_VLAN_CONFIGURED_ALL)
    {
      vlan_port->config = NSM_VLAN_CONFIGURED_ALL;
#ifdef HAVE_LACPD
      if (NSM_INTF_TYPE_AGGREGATOR(ifp))
         nsm_vlan_agg_update_config_flag (ifp);
#endif /* HAVE_LACPD */
    }

  l2_range_parser_deinit (&par);

  return cli_ret;
}

CLI (no_nsm_vlan_switchport_hybrid_vlan,
     no_nsm_vlan_switchport_hybrid_vlan_cmd,
     "no switchport hybrid",
     CLI_NO_STR,
     "Reset the switching characteristics of the Layer2 interface",
     "Reset the switching characteristics of the Layer2 interface to access")
{
  int ret;
  struct interface *ifp = cli->index;

#ifdef HAVE_LACPD
  NSM_INTERFACE_CHECK_AGG_MEM_PROPERTY(cli, ifp);
#endif /* HAVE_LACPD */
  NSM_BRIDGE_CHECK_L2_PROPERTY(cli, ifp);
  NSM_BRIDGE_CHECK_BRIDGE_PROPERTY(cli, ifp);
 
  ret =  nsm_vlan_clear_hybrid_port (ifp, PAL_TRUE, PAL_TRUE);
  if (ret < 0)
    {
      if (ret == NSM_VLAN_ERR_BRIDGE_NOT_FOUND)
        cli_out (cli, "%% Interface not bound to any bridge-group\n");
      else if (ret == NSM_VLAN_ERR_INVALID_MODE)
        cli_out (cli, "%% Port mode not set to Hybrid\n");
#ifdef HAVE_LACPD
      else if (ret == AGG_MEM_NO_SWITCHPORT)
        cli_out (cli, "%% Member interface not configured for switching\n");
      else if (ret == AGG_MEM_BRIDGE_NOT_VLAN_AWARE)
        cli_out (cli, "%% Member interface not vlan aware\n");
#endif /* HAVE LACP */    
      else
        cli_out (cli, "%% Can't reset the port from hybrid\n");
      return CLI_ERROR;
    }

  return CLI_SUCCESS;
}

CLI (nsm_vlan_switchport_hybrid_allowed_all,
     nsm_vlan_switchport_hybrid_allowed_all_cmd,
     "switchport hybrid allowed vlan all",
     "Set the switching characteristics of the Layer2 interface",
     NSM_VLAN_HYBRID_STR,
     "Set the VLANs that will Xmit/Rx through the Layer2 interface",
     "VLANs that will be added",
     "Allow all VLANs to Xmit/Rx through the Layer2 interface")
{
  int ret;
  struct interface *ifp = cli->index;

#ifdef HAVE_LACPD
  u_int16_t key;
  NSM_INTERFACE_CHECK_AGG_MEM_PROPERTY(cli, ifp);
#endif /* HAVE_LACPD */
  NSM_BRIDGE_CHECK_L2_PROPERTY(cli, ifp);
  NSM_BRIDGE_CHECK_BRIDGE_PROPERTY(cli, ifp);
 
  ret =  nsm_vlan_add_hybrid_port (ifp, NSM_VLAN_ALL, NSM_VLAN_EGRESS_TAGGED,
                                   PAL_TRUE, PAL_TRUE);
  if (ret < 0)
    {
      if (ret == NSM_VLAN_ERR_BRIDGE_NOT_FOUND)
        cli_out (cli, "%% Interface not bound to any bridge-group\n");
      else if (ret == NSM_VLAN_ERR_INVALID_MODE)
        cli_out (cli, "%% Port mode not set to Hybrid\n");
#ifdef HAVE_LACPD
      else if (ret == AGG_MEM_NO_SWITCHPORT)
        cli_out (cli, "%% Member interface not configured for switching\n");
      else if (ret == AGG_MEM_BRIDGE_NOT_VLAN_AWARE)
        cli_out (cli, "%% Member interface not vlan aware\n");
#endif /* HAVE LACP */
      else
        cli_out (cli, "%% Can't set all VLANs to hybrid port\n");
      return CLI_ERROR;
    }
#ifdef HAVE_LACPD
    if (NSM_INTF_TYPE_AGGREGATOR(ifp))
      nsm_vlan_agg_update_config_flag (ifp);

      key = ifp->lacp_admin_key;

      nsm_lacp_if_admin_key_set (ifp);

     if (key != ifp->lacp_admin_key)
        nsm_server_if_lacp_admin_key_update (ifp);

#endif /* HAVE_LACPD */

  return CLI_SUCCESS;
}

CLI (nsm_vlan_switchport_allowed_vlan_none,
     nsm_vlan_switchport_allowed_vlan_none_cmd,
     "switchport hybrid allowed vlan none",
     "Reset the switching characteristics of the Layer2 interface",
     NSM_VLAN_HYBRID_STR,
     "Set the VLANs that will Xmit/Rx through the Layer2 interface",
     "VLANs that will be removed",
     "Allow no VLANs to Xmit/Rx through the Layer2 interface")
{
  int ret;
  struct interface *ifp = cli->index;

#ifdef HAVE_LACPD
  u_int16_t key;
  NSM_INTERFACE_CHECK_AGG_MEM_PROPERTY(cli, ifp);
#endif /* HAVE_LACPD */
  NSM_BRIDGE_CHECK_L2_PROPERTY(cli, ifp);
  NSM_BRIDGE_CHECK_BRIDGE_PROPERTY(cli, ifp);
 
  ret =  nsm_vlan_add_hybrid_port (ifp, NSM_VLAN_NONE, NSM_VLAN_EGRESS_TAGGED,
                                   PAL_TRUE, PAL_TRUE);
  if (ret < 0)
    {
      if (ret == NSM_VLAN_ERR_BRIDGE_NOT_FOUND)
        cli_out (cli, "%% Interface not bound to any bridge-group\n");
      else if (ret == NSM_VLAN_ERR_INVALID_MODE)
        cli_out (cli, "%% Port mode not set to Hybrid\n");
#ifdef HAVE_LACPD
      else if (ret == AGG_MEM_NO_SWITCHPORT)
        cli_out (cli, "%% Member interface not configured for switching\n");
      else if (ret == AGG_MEM_BRIDGE_NOT_VLAN_AWARE)
        cli_out (cli, "%% Member interface not vlan aware\n");
#endif /* HAVE LACP */
      else
        cli_out (cli, "%% Can't set all VLANs to hybrid port\n");
      return CLI_ERROR;
    }
#ifdef HAVE_LACPD
    if (NSM_INTF_TYPE_AGGREGATOR(ifp))
      nsm_vlan_agg_update_config_flag (ifp);

     key = ifp->lacp_admin_key;

     nsm_lacp_if_admin_key_set (ifp);

     if (key != ifp->lacp_admin_key)
       nsm_server_if_lacp_admin_key_update (ifp);

#endif /* HAVE_LACPD */

  return CLI_SUCCESS;
}

#ifdef HAVE_PROVIDER_BRIDGE

CLI (nsm_vlan_switchport_ce_hybrid_add_egress,
     nsm_vlan_switchport_ce_hybrid_add_egress_cmd,
     "switchport customer-edge hybrid allowed vlan add VLAN_ID "
     "egress-tagged (enable|disable)",
     "Set the switching characteristics of the Layer2 interface",
     NSM_VLAN_CE_STR,
     NSM_VLAN_HYBRID_STR,
     "Set the VLANs that will Xmit/Rx through the Layer2 interface",
     "VLAN that will be added ",
     "Add a VLAN to Xmit/Rx through the Layer2 interface",
     "The List of the VLAN IDs that will be added to the Layer2 interface",
     "Set the egress tagging for the outgoing frames",
     "Set the egress tagging on the outgoing frames to enabled",
     "Set the egress tagging on the outgoing frames to disabled")
{
  nsm_vid_t vid;
  int ret;
  int cli_ret = CLI_SUCCESS;
  struct interface *ifp = cli->index;
  char *curr = NULL;
  l2_range_parser_t par;
  l2_range_parse_error_t r_err;
  enum nsm_vlan_egress_type egress_tagged = NSM_VLAN_EGRESS_UNTAGGED;

#ifdef HAVE_LACPD
  NSM_INTERFACE_CHECK_AGG_MEM_PROPERTY(cli, ifp);
#endif /* HAVE_LACPD */
  NSM_BRIDGE_CHECK_L2_PROPERTY(cli, ifp);
  NSM_BRIDGE_CHECK_BRIDGE_PROPERTY(cli, ifp);

  if (! pal_strncmp (argv[2], "e", 1))
    egress_tagged = NSM_VLAN_EGRESS_TAGGED;
  else
    egress_tagged = NSM_VLAN_EGRESS_UNTAGGED;

  l2_range_parser_init (argv[1], &par);

  if ( (ret = l2_range_parser_validate (&par, cli, nsm_vlan_validate_vid))
       != CLI_SUCCESS)
    return ret;

  while ( (curr = l2_range_parser_get_next ( &par, &r_err) ) )
    {
      if ( r_err != RANGE_PARSER_SUCCESS )
        {
          cli_out (cli, "%% Invalid Input %s \n", l2_range_parse_err_to_str (r_err) );
          return CLI_ERROR;
        }

      CLI_GET_INTEGER_RANGE ("port vlanid", vid, curr, NSM_VLAN_CLI_MIN, NSM_VLAN_CLI_MAX);

      ret = nsm_vlan_add_provider_port
                                  (ifp, vid,
                                   NSM_VLAN_PORT_MODE_CE,
                                   NSM_VLAN_PORT_MODE_HYBRID,
                                   egress_tagged, PAL_TRUE, PAL_TRUE);
      if (ret < 0)
        {
          if (ret == NSM_VLAN_ERR_IFP_NOT_BOUND)
            {
              cli_out (cli, "%% Interface not bound to any bridge-group\n");
              return CLI_ERROR;
            }
          else if (ret == NSM_VLAN_ERR_INVALID_MODE)
            {
              cli_out (cli, "%% Port mode not set to hybrid\n");
              return CLI_ERROR;
            }
#ifdef HAVE_LACPD
          else if (ret == AGG_MEM_NO_SWITCHPORT)
            {
              cli_out (cli, "%% Member interface not configured for switching\n");
              return CLI_ERROR;
            }
          else if (ret == AGG_MEM_BRIDGE_NOT_VLAN_AWARE)
            {
              cli_out (cli, "%% Member interface not vlan aware\n");
              return CLI_ERROR;
            }
#endif /* HAVE LACP */
          else if (ret == NSM_VLAN_ERR_CONFIG_PVID_TAG)
            {
              cli_out (cli, "%% Default vlan can't be tagged\n");
              return CLI_ERROR;
            }
          else
            {
              cli_out (cli, "%% Can't add vlan %d to port\n", vid);
              cli_ret = CLI_ERROR;
            }
        }
    }

  l2_range_parser_deinit (&par);

  return cli_ret;
}

CLI (nsm_vlan_switchport_ce_trunk_add,
     nsm_vlan_switchport_ce_trunk_add_cmd,
     "switchport customer-edge trunk allowed vlan add VLAN_ID",
     "Set the switching characteristics of the Layer2 interface",
     NSM_VLAN_CE_STR,
     NSM_VLAN_TRUNK_STR,
     "Set the VLANs that will Xmit/Rx through the Layer2 interface",
     "VLAN that will be added ",
     "Add a VLAN to Xmit/Rx through the Layer2 interface",
     "The List of the VLAN IDs that will be added to the Layer2 interface",
     "Set the egress tagging for the outgoing frames")
{
  int ret;
  nsm_vid_t vid;
  char *curr = NULL;
  l2_range_parser_t par;
  int cli_ret = CLI_SUCCESS;
  l2_range_parse_error_t r_err;
  struct interface *ifp = cli->index;

#ifdef HAVE_LACPD
  NSM_INTERFACE_CHECK_AGG_MEM_PROPERTY(cli, ifp);
#endif /* HAVE_LACPD */
  NSM_BRIDGE_CHECK_L2_PROPERTY(cli, ifp);
  NSM_BRIDGE_CHECK_BRIDGE_PROPERTY(cli, ifp);

  l2_range_parser_init (argv[0], &par);

  if ( (ret = l2_range_parser_validate (&par, cli, nsm_vlan_validate_vid))
       != CLI_SUCCESS)
    return ret;

  while ( (curr = l2_range_parser_get_next ( &par, &r_err) ) )
    {
      if ( r_err != RANGE_PARSER_SUCCESS )
        {
          cli_out (cli, "%% Invalid Input %s \n", l2_range_parse_err_to_str (r_err) );
          return CLI_ERROR;
        }

      CLI_GET_INTEGER_RANGE ("port vlanid", vid, curr, NSM_VLAN_CLI_MIN, NSM_VLAN_CLI_MAX);

      ret = nsm_vlan_add_provider_port
                                  (ifp, vid,
                                   NSM_VLAN_PORT_MODE_CE,
                                   NSM_VLAN_PORT_MODE_TRUNK,
                                   NSM_VLAN_EGRESS_TAGGED, PAL_TRUE, PAL_TRUE);
      if (ret < 0)
        {
          if (ret == NSM_VLAN_ERR_IFP_NOT_BOUND)
            {
              cli_out (cli, "%% Interface not bound to any bridge-group\n");
              return CLI_ERROR;
            }
          else if (ret == NSM_VLAN_ERR_INVALID_MODE)
            {
              cli_out (cli, "%% Port mode not set to hybrid\n");
              return CLI_ERROR;
            }
#ifdef HAVE_LACPD
          else if (ret == AGG_MEM_NO_SWITCHPORT)
            {
              cli_out (cli, "%% Member interface not configured for switching\n");
              return CLI_ERROR;
            }
          else if (ret == AGG_MEM_BRIDGE_NOT_VLAN_AWARE)
            {
              cli_out (cli, "%% Member interface not vlan aware\n");
              return CLI_ERROR;
            }
#endif /* HAVE LACP */
          else if (ret == NSM_VLAN_ERR_CONFIG_PVID_TAG)
            {
              cli_out (cli, "%% Default vlan can't be tagged\n");
              return CLI_ERROR;
            }
          else
            {
              cli_out (cli, "%% Can't add vlan %d to port\n", vid);
              cli_ret = CLI_ERROR;
            }
        }
    }

  l2_range_parser_deinit (&par);

  return cli_ret;
}

CLI (nsm_vlan_switchport_ce_tr_delete,
     nsm_vlan_switchport_ce_tr_delete_cmd,
     "switchport customer-edge trunk allowed vlan remove VLAN_ID",
     "Set the switching characteristics of the Layer2 interface",
     NSM_VLAN_CE_STR,
     NSM_VLAN_TRUNK_STR,
     "Set the VLANs that will Xmit/Rx through the Layer2 interface",
     "VLAN that will be removed",
     "Remove a VLAN to Xmit/Rx through the Layer2 interface",
     "The List of the VLAN IDs that will be removed from the Layer2 interface")
{
  int ret;
  nsm_vid_t vid;
  char *curr = NULL;
  l2_range_parser_t par;
  int cli_ret = CLI_SUCCESS;
  enum nsm_vlan_port_mode mode;
  l2_range_parse_error_t r_err;
  enum nsm_vlan_port_mode sub_mode;
  struct interface *ifp = cli->index;

#ifdef HAVE_LACPD
  NSM_INTERFACE_CHECK_AGG_MEM_PROPERTY(cli, ifp);
#endif /* HAVE_LACPD */
  NSM_BRIDGE_CHECK_L2_PROPERTY(cli, ifp);
  NSM_BRIDGE_CHECK_BRIDGE_PROPERTY(cli, ifp);

  mode = NSM_VLAN_PORT_MODE_CE;
  sub_mode = NSM_VLAN_PORT_MODE_TRUNK;

  l2_range_parser_init (argv[0], &par);

  if ( (ret = l2_range_parser_validate (&par, cli, nsm_vlan_validate_vid))
       != CLI_SUCCESS)
    return ret;

  while ( (curr = l2_range_parser_get_next ( &par, &r_err) ) )
    {
      if ( r_err != RANGE_PARSER_SUCCESS )
        {
          cli_out (cli, "%% Invalid Input %s \n", l2_range_parse_err_to_str (r_err) );
          return CLI_ERROR;
        }

      CLI_GET_INTEGER_RANGE ("port vlanid", vid, curr, NSM_VLAN_CLI_MIN, NSM_VLAN_CLI_MAX);

      ret = nsm_vlan_delete_provider_port
                            (ifp, vid,
                             mode, sub_mode,
                             PAL_TRUE, PAL_TRUE);

      if (ret < 0)
        {
          if (ret == NSM_VLAN_ERR_IFP_NOT_BOUND)
            {
              cli_out (cli, "%% Interface not bound to any bridge-group\n");
              cli_ret = CLI_ERROR;
            }
          else if (ret == NSM_VLAN_ERR_CVLAN_PORT_REG_EXIST)
            {
              cli_out (cli, "%% Unconfigure existing CVLAN Registration Table "
                            "for the VLAN %d\n", vid);
              cli_ret = CLI_ERROR;
            }
          else if (ret == NSM_VLAN_ERR_INVALID_MODE)
            {
              cli_out (cli, "%% Port mode not set to hybrid\n");
              cli_ret = CLI_ERROR;
            }
          else
            {
              cli_out (cli, "%% Can't remove vlan %d from port\n",vid);
              cli_ret = CLI_ERROR;
            }
        }
    }

  l2_range_parser_deinit (&par);

  return cli_ret;
}

CLI (nsm_vlan_switchport_ce_tr_allowed_all,
     nsm_vlan_switchport_ce_tr_allowed_all_cmd,
     "switchport customer-edge trunk allowed vlan all",
     "Set the switching characteristics of the Layer2 interface",
     NSM_VLAN_CE_STR,
     NSM_VLAN_TRUNK_STR,
     "Set the VLANs that will Xmit/Rx through the Layer2 interface",
     "VLANs that will be added",
     "Allow all VLANs to Xmit/Rx through the Layer2 interface")
{
  int ret;
  enum nsm_vlan_port_mode mode;
  enum nsm_vlan_port_mode sub_mode;
  struct interface *ifp = cli->index;

#ifdef HAVE_LACPD
  NSM_INTERFACE_CHECK_AGG_MEM_PROPERTY(cli, ifp);
#endif /* HAVE_LACPD */
  NSM_BRIDGE_CHECK_L2_PROPERTY(cli, ifp);
  NSM_BRIDGE_CHECK_BRIDGE_PROPERTY(cli, ifp);


  mode = NSM_VLAN_PORT_MODE_CE;
  sub_mode = NSM_VLAN_PORT_MODE_TRUNK;

  ret = nsm_vlan_add_provider_port
                     (ifp, NSM_VLAN_ALL,
                      mode, sub_mode,
                      NSM_VLAN_EGRESS_TAGGED, PAL_TRUE, PAL_TRUE);
  if (ret < 0)
    {
      if (ret == NSM_VLAN_ERR_BRIDGE_NOT_FOUND)
        cli_out (cli, "%% Interface not bound to any bridge-group\n");
      else if (ret == NSM_VLAN_ERR_INVALID_MODE)
        cli_out (cli, "%% Port mode not set to Hybrid\n");
#ifdef HAVE_LACPD
      else if (ret == AGG_MEM_NO_SWITCHPORT)
        cli_out (cli, "%% Member interface not configured for switching\n");
      else if (ret == AGG_MEM_BRIDGE_NOT_VLAN_AWARE)
        cli_out (cli, "%% Member interface not vlan aware\n");
#endif /* HAVE LACP */
      else
        cli_out (cli, "%% Can't set all VLANs to hybrid port\n");
      return CLI_ERROR;
    }

  return CLI_SUCCESS;
}

CLI (nsm_vlan_switchport_ce_tr_allowed_none,
     nsm_vlan_switchport_ce_tr_allowed_none_cmd,
     "switchport customer-edge trunk allowed vlan none",
     "Reset the switching characteristics of the Layer2 interface",
     NSM_VLAN_CE_STR,
     NSM_VLAN_CN_STR,
     NSM_VLAN_TRUNK_STR,
     "Set the VLANs that will Xmit/Rx through the Layer2 interface",
     "VLANs that will be removed",
     "Allow no VLANs to Xmit/Rx through the Layer2 interface")
{
  int ret;
  enum nsm_vlan_port_mode mode;
  enum nsm_vlan_port_mode sub_mode;
  struct interface *ifp = cli->index;

#ifdef HAVE_LACPD
  NSM_INTERFACE_CHECK_AGG_MEM_PROPERTY(cli, ifp);
#endif /* HAVE_LACPD */
  NSM_BRIDGE_CHECK_L2_PROPERTY(cli, ifp);
  NSM_BRIDGE_CHECK_BRIDGE_PROPERTY(cli, ifp);

  mode = NSM_VLAN_PORT_MODE_CE;
  sub_mode = NSM_VLAN_PORT_MODE_TRUNK;

  ret = nsm_vlan_add_provider_port
                     (ifp, NSM_VLAN_NONE,
                      mode, sub_mode,
                      NSM_VLAN_EGRESS_TAGGED, PAL_TRUE, PAL_TRUE);

  if (ret < 0)
    {
      if (ret == NSM_VLAN_ERR_BRIDGE_NOT_FOUND)
        cli_out (cli, "%% Interface not bound to any bridge-group\n");
      else if (ret == NSM_VLAN_ERR_INVALID_MODE)
        cli_out (cli, "%% Port mode not set to Hybrid\n");
#ifdef HAVE_LACPD
      else if (ret == AGG_MEM_NO_SWITCHPORT)
        cli_out (cli, "%% Member interface not configured for switching\n");
      else if (ret == AGG_MEM_BRIDGE_NOT_VLAN_AWARE)
        cli_out (cli, "%% Member interface not vlan aware\n");
#endif /* HAVE LACP */
      else if (ret == NSM_VLAN_ERR_CVLAN_PORT_REG_EXIST)
        cli_out (cli, "%% Unconfigure existing CVLAN Registration Table \n");
      else
        cli_out (cli, "%% Can't set all VLANs to hybrid port\n");
      return CLI_ERROR;
    }

  return CLI_SUCCESS;

}

CLI (nsm_vlan_switchport_ce_hr_delete,
     nsm_vlan_switchport_ce_hr_delete_cmd,
     "switchport customer-edge hybrid allowed vlan remove VLAN_ID",
     "Set the switching characteristics of the Layer2 interface",
     NSM_VLAN_CE_STR,
     NSM_VLAN_HYBRID_STR,
     "Set the VLANs that will Xmit/Rx through the Layer2 interface",
     "VLAN that will be removed",
     "Remove a VLAN to Xmit/Rx through the Layer2 interface",
     "The List of the VLAN IDs that will be removed from the Layer2 interface")
{
  int ret;
  nsm_vid_t vid;
  char *curr = NULL;
  l2_range_parser_t par;
  int cli_ret = CLI_SUCCESS;
  enum nsm_vlan_port_mode mode;
  l2_range_parse_error_t r_err;
  enum nsm_vlan_port_mode sub_mode;
  struct interface *ifp = cli->index;

#ifdef HAVE_LACPD
  NSM_INTERFACE_CHECK_AGG_MEM_PROPERTY(cli, ifp);
#endif /* HAVE_LACPD */
  NSM_BRIDGE_CHECK_L2_PROPERTY(cli, ifp);
  NSM_BRIDGE_CHECK_BRIDGE_PROPERTY(cli, ifp);

  mode = NSM_VLAN_PORT_MODE_CE;

  sub_mode = NSM_VLAN_PORT_MODE_HYBRID;

  l2_range_parser_init (argv[1], &par);

  if ( (ret = l2_range_parser_validate (&par, cli, nsm_vlan_validate_vid))
       != CLI_SUCCESS)
    return ret;

  while ( (curr = l2_range_parser_get_next ( &par, &r_err) ) )
    {
      if ( r_err != RANGE_PARSER_SUCCESS )
        {
          cli_out (cli, "%% Invalid Input %s \n", l2_range_parse_err_to_str (r_err) );
          return CLI_ERROR;
        }

      CLI_GET_INTEGER_RANGE ("port vlanid", vid, curr, NSM_VLAN_CLI_MIN, NSM_VLAN_CLI_MAX);

      ret = nsm_vlan_delete_provider_port
                            (ifp, vid,
                             mode, sub_mode,
                             PAL_TRUE, PAL_TRUE);

      if (ret < 0)
        {
          if (ret == NSM_VLAN_ERR_IFP_NOT_BOUND)
            {
              cli_out (cli, "%% Interface not bound to any bridge-group\n");
              cli_ret = CLI_ERROR;
            }
          else if (ret == NSM_VLAN_ERR_CVLAN_PORT_REG_EXIST)
            {
              cli_out (cli, "%% Unconfigure existing CVLAN Registration Table "
                            "for the VLAN %d\n", vid);
              cli_ret = CLI_ERROR;
            }
          else if (ret == NSM_VLAN_ERR_INVALID_MODE)
            {
              cli_out (cli, "%% Port mode not set to hybrid\n");
              cli_ret = CLI_ERROR;
            }
          else
            {
              cli_out (cli, "%% Can't remove vlan %d from port\n",vid);
              cli_ret = CLI_ERROR;
            }
        }
    }

  l2_range_parser_deinit (&par);

  return cli_ret;
}

CLI (nsm_vlan_switchport_ce_hr_allowed_all,
     nsm_vlan_switchport_ce_hr_allowed_all_cmd,
     "switchport customer-edge hybrid allowed vlan all",
     "Set the switching characteristics of the Layer2 interface",
     NSM_VLAN_CE_STR,
     NSM_VLAN_HYBRID_STR,
     "Set the VLANs that will Xmit/Rx through the Layer2 interface",
     "VLANs that will be added",
     "Allow all VLANs to Xmit/Rx through the Layer2 interface")
{
  int ret;
  enum nsm_vlan_port_mode mode;
  enum nsm_vlan_port_mode sub_mode;
  struct interface *ifp = cli->index;

#ifdef HAVE_LACPD
  NSM_INTERFACE_CHECK_AGG_MEM_PROPERTY(cli, ifp);
#endif /* HAVE_LACPD */
  NSM_BRIDGE_CHECK_L2_PROPERTY(cli, ifp);
  NSM_BRIDGE_CHECK_BRIDGE_PROPERTY(cli, ifp);

  mode = NSM_VLAN_PORT_MODE_CE;
  sub_mode = NSM_VLAN_PORT_MODE_HYBRID;

  ret = nsm_vlan_add_provider_port
                     (ifp, NSM_VLAN_ALL,
                      mode, sub_mode,
                      NSM_VLAN_EGRESS_TAGGED, PAL_TRUE, PAL_TRUE);
  if (ret < 0)
    {
      if (ret == NSM_VLAN_ERR_BRIDGE_NOT_FOUND)
        cli_out (cli, "%% Interface not bound to any bridge-group\n");
      else if (ret == NSM_VLAN_ERR_INVALID_MODE)
        cli_out (cli, "%% Port mode not set to Hybrid\n");
#ifdef HAVE_LACPD
      else if (ret == AGG_MEM_NO_SWITCHPORT)
        cli_out (cli, "%% Member interface not configured for switching\n");
      else if (ret == AGG_MEM_BRIDGE_NOT_VLAN_AWARE)
        cli_out (cli, "%% Member interface not vlan aware\n");
#endif /* HAVE LACP */
      else
        cli_out (cli, "%% Can't set all VLANs to hybrid port\n");
      return CLI_ERROR;
    }

  return CLI_SUCCESS;
}

CLI (nsm_vlan_switchport_ce_hr_allowed_none,
     nsm_vlan_switchport_ce_hr_allowed_none_cmd,
     "switchport customer-edge hybrid allowed vlan none",
     "Reset the switching characteristics of the Layer2 interface",
     NSM_VLAN_CE_STR,
     NSM_VLAN_CN_STR,
     NSM_VLAN_HYBRID_STR,
     "Set the VLANs that will Xmit/Rx through the Layer2 interface",
     "VLANs that will be removed",
     "Allow no VLANs to Xmit/Rx through the Layer2 interface")
{
  int ret;
  enum nsm_vlan_port_mode mode;
  enum nsm_vlan_port_mode sub_mode;
  struct interface *ifp = cli->index;

#ifdef HAVE_LACPD
  u_int16_t key;
  NSM_INTERFACE_CHECK_AGG_MEM_PROPERTY(cli, ifp);
#endif /* HAVE_LACPD */
  NSM_BRIDGE_CHECK_L2_PROPERTY(cli, ifp);
  NSM_BRIDGE_CHECK_BRIDGE_PROPERTY(cli, ifp);

  mode = NSM_VLAN_PORT_MODE_CE;
  sub_mode = NSM_VLAN_PORT_MODE_HYBRID;

  ret = nsm_vlan_add_provider_port
                     (ifp, NSM_VLAN_NONE,
                      mode, sub_mode,
                      NSM_VLAN_EGRESS_TAGGED, PAL_TRUE, PAL_TRUE);

  if (ret < 0)
    {
      if (ret == NSM_VLAN_ERR_BRIDGE_NOT_FOUND)
        cli_out (cli, "%% Interface not bound to any bridge-group\n");
      else if (ret == NSM_VLAN_ERR_INVALID_MODE)
        cli_out (cli, "%% Port mode not set to Hybrid\n");
#ifdef HAVE_LACPD
      else if (ret == AGG_MEM_NO_SWITCHPORT)
        cli_out (cli, "%% Member interface not configured for switching\n");
      else if (ret == AGG_MEM_BRIDGE_NOT_VLAN_AWARE)
        cli_out (cli, "%% Member interface not vlan aware\n");
#endif /* HAVE LACP */
      else if (ret == NSM_VLAN_ERR_CVLAN_PORT_REG_EXIST)
        cli_out (cli, "%% Unconfigure existing CVLAN Registration Table \n");
      else
        cli_out (cli, "%% Can't set all VLANs to hybrid port\n");
      return CLI_ERROR;
    }
#ifdef HAVE_LACPD
    if (NSM_INTF_TYPE_AGGREGATOR(ifp))
       nsm_vlan_agg_update_config_flag (ifp);

       key = ifp->lacp_admin_key;

       nsm_lacp_if_admin_key_set (ifp);

     if (key != ifp->lacp_admin_key)
        nsm_server_if_lacp_admin_key_update (ifp);

#endif /* HAVE_LACPD */

  return CLI_SUCCESS;

}

#endif /* HAVE_PROVIDER_BRIDGE */

CLI (nsm_vlan_switchport_trunk_pn_cn_all,
     nsm_vlan_switchport_trunk_pn_cn_all_cmd,
     "switchport (trunk|provider-network|customer-network) allowed vlan all",
     "Set the switching characteristics of the Layer2 interface",
     NSM_VLAN_TRUNK_STR,
     NSM_VLAN_PN_STR,
     NSM_VLAN_CN_STR,
     "Set the VLANs that will Xmit/Rx through the Layer2 interface",
     "VLAN(s) that will be added",
     "Allow all VLANs to Xmit/Rx through the Layer2 interface")
{
  int ret;
  struct interface *ifp = cli->index;
 
#ifdef HAVE_LACPD
  NSM_INTERFACE_CHECK_AGG_MEM_PROPERTY(cli, ifp);
#endif /* HAVE_LACPD */
  NSM_BRIDGE_CHECK_L2_PROPERTY(cli, ifp);
  NSM_BRIDGE_CHECK_BRIDGE_PROPERTY(cli, ifp);

#ifdef HAVE_PROVIDER_BRIDGE

  if (! pal_strncmp (argv[0], "p", 1))
    {
      ret = nsm_vlan_add_provider_port (ifp, NSM_VLAN_ALL,
                                        NSM_VLAN_PORT_MODE_PN,
                                        NSM_VLAN_PORT_MODE_PN,
                                        NSM_VLAN_EGRESS_TAGGED, PAL_TRUE,
                                        PAL_TRUE);
    }
  else if (! pal_strncmp (argv[0], "c", 1))
    {
      ret = nsm_vlan_add_provider_port (ifp, NSM_VLAN_ALL,
                                        NSM_VLAN_PORT_MODE_CN,
                                        NSM_VLAN_PORT_MODE_CN,
                                        NSM_VLAN_EGRESS_TAGGED, PAL_TRUE,
                                        PAL_TRUE);
    }
  else
#endif /* HAVE_PROVIDER_BRIDGE */
    ret = nsm_vlan_add_trunk_port (ifp, NSM_VLAN_ALL, PAL_TRUE, PAL_TRUE);
 
  if (ret < 0)
    {
      if (ret == NSM_VLAN_ERR_BRIDGE_NOT_FOUND)
        cli_out (cli, "%% Interface not bound to any bridge-group\n");
      else if (ret == NSM_VLAN_ERR_INVALID_MODE)
        cli_out (cli, "%% Port mode not set to Trunk\n");
#ifdef HAVE_LACPD
      else if (ret == AGG_MEM_NO_SWITCHPORT)
        cli_out (cli, "%% Member interface not configured for switching\n");
      else if (ret == AGG_MEM_BRIDGE_NOT_VLAN_AWARE)
        cli_out (cli, "%% Member interface not vlan aware\n");
#endif /* HAVE LACP */
      else
        cli_out (cli, "%% Can't add the VIDs to the port\n");
      return CLI_ERROR;
    }

  return CLI_SUCCESS;
}

CLI (nsm_vlan_switchport_trunk_pn_cn_none,
     nsm_vlan_switchport_trunk_pn_cn_none_cmd,
     "switchport (trunk|provider-network|customer-network) allowed vlan none",
     "Set the switching characteristics of the Layer2 interface",
     NSM_VLAN_TRUNK_STR,
     NSM_VLAN_PN_STR,
     NSM_VLAN_CN_STR,
     "Set the VLANs that will Xmit/Rx through the Layer2 interface",
     "VLAN(s) that will be removed",
     "Allow no VLANs to Xmit/Rx through the Layer2 interface")
{
  int ret;
  struct interface *ifp = cli->index;
 
#ifdef HAVE_LACPD
   u_int16_t key;
  NSM_INTERFACE_CHECK_AGG_MEM_PROPERTY(cli, ifp);
#endif /* HAVE_LACPD */
  NSM_BRIDGE_CHECK_L2_PROPERTY(cli, ifp);
  NSM_BRIDGE_CHECK_BRIDGE_PROPERTY(cli, ifp);

#ifdef HAVE_PROVIDER_BRIDGE
  if (! pal_strncmp (argv[0], "p", 1))
    {
      ret = nsm_vlan_add_provider_port (ifp, NSM_VLAN_NONE,
                                        NSM_VLAN_PORT_MODE_PN,
                                        NSM_VLAN_PORT_MODE_PN,
                                        NSM_VLAN_EGRESS_TAGGED, PAL_TRUE,
                                        PAL_TRUE);
    }
  else if (! pal_strncmp (argv[0], "c", 1))
    {
      ret = nsm_vlan_add_provider_port (ifp, NSM_VLAN_NONE,
                                        NSM_VLAN_PORT_MODE_CN,
                                        NSM_VLAN_PORT_MODE_CN,
                                        NSM_VLAN_EGRESS_TAGGED, PAL_TRUE,
                                        PAL_TRUE);
    }
  else
#endif /* HAVE_PROVIDER_BRIDGE */
    ret = nsm_vlan_add_trunk_port (ifp, NSM_VLAN_NONE, PAL_TRUE, PAL_TRUE);

  if (ret < 0)
    {
      if (ret == NSM_VLAN_ERR_BRIDGE_NOT_FOUND)
        cli_out (cli, "%% Interface not bound to any bridge-group\n");
      else if (ret == NSM_VLAN_ERR_INVALID_MODE)
        cli_out (cli, "%% Port mode not set to Trunk\n");
#ifdef HAVE_LACPD
      else if (ret == AGG_MEM_NO_SWITCHPORT)
        cli_out (cli, "%% Member interface not configured for switching\n");
      else if (ret == AGG_MEM_BRIDGE_NOT_VLAN_AWARE)
        cli_out (cli, "%% Member interface not vlan aware\n");
#endif /* HAVE LACP */
      else
        cli_out (cli, "%% Can't add the VIDs to the port: %s\n",
                 pal_strerror (ret));
      return CLI_ERROR;
    }
#ifdef HAVE_LACPD
    if (NSM_INTF_TYPE_AGGREGATOR(ifp))
         nsm_vlan_agg_update_config_flag (ifp);

      key = ifp->lacp_admin_key;

      nsm_lacp_if_admin_key_set (ifp);

     if (key != ifp->lacp_admin_key)
       nsm_server_if_lacp_admin_key_update (ifp);

#endif /* HAVE_LACPD */

  return CLI_SUCCESS;
}

CLI (nsm_vlan_switchport_trunk_pn_cn_add,
     nsm_vlan_switchport_trunk_pn_cn_add_cmd,
     "switchport (trunk|provider-network|customer-network) allowed "
     "vlan add VLAN_ID",
     "Set the switching characteristics of the Layer2 interface",
     NSM_VLAN_TRUNK_STR,
     NSM_VLAN_PN_STR,
     NSM_VLAN_CN_STR,
     "Set the VLANs that will Xmit/Rx through the Layer2 interface",
     "VLAN that will be added",
     "Add a VLAN to Xmit/Tx through the Layer2 interface",
     "The List of the VLAN IDs that will be added to the Layer2 interface")
{
  int ret;
  int cli_ret = CLI_SUCCESS;
  unsigned short vid;
  struct interface *ifp = cli->index;
  char *curr = NULL;
  l2_range_parser_t par;
  l2_range_parse_error_t r_err;

#ifdef HAVE_LACPD
  NSM_INTERFACE_CHECK_AGG_MEM_PROPERTY(cli, ifp);
#endif /* HAVE_LACPD */
  NSM_BRIDGE_CHECK_L2_PROPERTY(cli, ifp);
  NSM_BRIDGE_CHECK_BRIDGE_PROPERTY(cli, ifp);

  l2_range_parser_init (argv[1], &par);

  if (argv [0] [0] == 't')
    {
      if ((ret = l2_range_parser_validate (&par, cli, nsm_vlan_validate_vid1))
          != CLI_SUCCESS)
        return ret;
    }
  else
    {
      if ((ret = l2_range_parser_validate (&par, cli, nsm_vlan_validate_vid))
          != CLI_SUCCESS)
        return ret;
    }
  
  while ( (curr = l2_range_parser_get_next ( &par, &r_err) ) )
    {
      if ( r_err != RANGE_PARSER_SUCCESS )
        {
          cli_out (cli, "%% Invalid Input %s \n",
                   l2_range_parse_err_to_str (r_err) );
          return CLI_ERROR;
        }
      
      if (argv [0] [0] == 't')
        CLI_GET_INTEGER_RANGE ("vlanid", vid, curr, NSM_VLAN_DEFAULT_VID,
                               NSM_VLAN_CLI_MAX);
      else
        CLI_GET_INTEGER_RANGE ("vlanid", vid, curr, NSM_VLAN_CLI_MIN,
                               NSM_VLAN_CLI_MAX);

#ifdef HAVE_PROVIDER_BRIDGE

      if (! pal_strncmp (argv[0], "p", 1))
        {
          ret = nsm_vlan_add_provider_port (ifp, vid,
                                            NSM_VLAN_PORT_MODE_PN,
                                            NSM_VLAN_PORT_MODE_PN,
                                            NSM_VLAN_EGRESS_TAGGED, PAL_TRUE,
                                            PAL_TRUE);
        }
      else if (! pal_strncmp (argv[0], "c", 1))
        {
          ret = nsm_vlan_add_provider_port (ifp, vid,
                                            NSM_VLAN_PORT_MODE_CN,
                                            NSM_VLAN_PORT_MODE_CN,
                                            NSM_VLAN_EGRESS_TAGGED, PAL_TRUE,
                                            PAL_TRUE);
        }
      else
#endif /* HAVE_PROVIDER_BRIDGE */
        ret = nsm_vlan_add_trunk_port (ifp, vid, PAL_TRUE, PAL_TRUE);

      if (ret < 0)
        {
          if (ret == NSM_VLAN_ERR_BRIDGE_NOT_FOUND)
            {
              cli_out (cli, "%% Interface not bound to any bridge-group\n");
              return CLI_ERROR;
            }
          else if (ret == NSM_VLAN_ERR_INVALID_MODE)
            {
              cli_out (cli, "%% Port mode not set to Trunk\n");
              return CLI_ERROR;
            }
#ifdef HAVE_LACPD
          else if (ret == AGG_MEM_NO_SWITCHPORT)
            {
              cli_out (cli, "%% Member interface not configured for switching\n");
              return CLI_ERROR;
            }
          else if (ret == AGG_MEM_BRIDGE_NOT_VLAN_AWARE)
            {
              cli_out (cli, "%% Member interface not vlan aware\n");
              return CLI_ERROR;
            }
#endif /* HAVE LACP */
          else if (ret == NSM_VLAN_ERR_VLAN_NOT_FOUND)
            {
              cli_out (cli, "%%Adding Port %s to non-existing vlan %d\n",
                       ifp->name, vid);

              return CLI_ERROR;
            }
          else
            {
              cli_out (cli, "%% Can't add VID %d to a port: %s\n", vid, pal_strerror (ret));
              cli_ret = CLI_ERROR;
            }
        }

    }

  l2_range_parser_deinit (&par);

  return cli_ret;
}

CLI (nsm_vlan_switchport_trunk_pn_cn_delete,
     nsm_vlan_switchport_trunk_pn_cn_delete_cmd,
     "switchport (trunk|provider-network|customer-network) allowed "
     "vlan remove VLAN_ID",
     "Set the switching characteristics of the Layer2 interface",
     NSM_VLAN_TRUNK_STR,
     NSM_VLAN_PN_STR,
     NSM_VLAN_CN_STR,
     "Set the VLANs that will Xmit/Rx through the Layer2 interface",
     "VLAN that will be removed",
     "Remove a VLAN that Xmit/Tx through the Layer2 interface",
     "The List of the VLAN IDs that will be removed from the Layer2 interface")
{
  int ret;
  int cli_ret = CLI_SUCCESS;
  unsigned short vid;
  struct interface *ifp = cli->index;
  char *curr = NULL;
  l2_range_parser_t par;
  l2_range_parse_error_t r_err;
  struct nsm_if *zif;
  struct nsm_vlan_port *vlan_port;
  struct nsm_bridge_port *br_port;
  int vlan_port_config;  
  
#ifdef HAVE_LACPD
  NSM_INTERFACE_CHECK_AGG_MEM_PROPERTY(cli, ifp);
#endif /* HAVE_LACPD */
  NSM_BRIDGE_CHECK_L2_PROPERTY(cli, ifp);
  NSM_BRIDGE_CHECK_BRIDGE_PROPERTY(cli, ifp);

  l2_range_parser_init (argv[1], &par);

  if (argv [0] [0] == 't')
    {
      if ((ret = l2_range_parser_validate (&par, cli, nsm_vlan_validate_vid1))
          != CLI_SUCCESS)
        return ret;
    }
  else
    {
      if ((ret = l2_range_parser_validate (&par, cli, nsm_vlan_validate_vid))
          != CLI_SUCCESS)
        return ret;
    }  

  zif = (struct nsm_if *)ifp->info;
  br_port = zif->switchport;
  vlan_port = &br_port->vlan_port;

  vlan_port_config = vlan_port->config; 
  
  while ( (curr = l2_range_parser_get_next ( &par, &r_err) ) )
    {
      if ( r_err != RANGE_PARSER_SUCCESS )
        {
          cli_out (cli, "%% Invalid Input %s \n", l2_range_parse_err_to_str (r_err) );
          return CLI_ERROR;
        }

      if (argv [0] [0] == 't')
        CLI_GET_INTEGER_RANGE ("vlanid", vid, curr, NSM_VLAN_DEFAULT_VID,
                               NSM_VLAN_CLI_MAX);
      else
        CLI_GET_INTEGER_RANGE ("vlanid", vid, curr, NSM_VLAN_CLI_MIN,
                               NSM_VLAN_CLI_MAX);

#ifdef HAVE_PROVIDER_BRIDGE

      if (! pal_strncmp (argv[0], "p", 1))
        {
          ret = nsm_vlan_delete_provider_port (ifp, vid,
                                               NSM_VLAN_PORT_MODE_PN,
                                               NSM_VLAN_PORT_MODE_PN,
                                               PAL_TRUE, PAL_TRUE);
        }
      else if (! pal_strncmp (argv[0], "c", 1))
        {
          ret = nsm_vlan_delete_provider_port (ifp, vid,
                                               NSM_VLAN_PORT_MODE_CN,
                                               NSM_VLAN_PORT_MODE_CN,
                                               PAL_TRUE, PAL_TRUE);
        }
      else
#endif /* HAVE_PROVIDER_BRIDGE */
        ret = nsm_vlan_delete_trunk_port (ifp, vid, PAL_TRUE, PAL_TRUE);

      if (ret < 0)
        {
          if (ret == NSM_VLAN_ERR_BRIDGE_NOT_FOUND)
            {
              cli_out (cli, "%% Interface not bound to any bridge-group\n");
              return CLI_ERROR;
            }
          else if (ret == NSM_VLAN_ERR_INVALID_MODE)
            {
              cli_out (cli, "%% Port mode not set to Trunk\n");
              return CLI_ERROR;
            }
#ifdef HAVE_LACPD
          else if (ret == AGG_MEM_NO_SWITCHPORT)
            {
              cli_out (cli, "%% Member interface not configured for switching\n");
              return CLI_ERROR;
            }
          else if (ret == AGG_MEM_BRIDGE_NOT_VLAN_AWARE)
            {
              cli_out (cli, "%% Member interface not vlan aware\n");
              return CLI_ERROR;
            }
#endif /* HAVE LACP */
          else
            {
              cli_out (cli, "%% Can't remove a VID %d from a port: %s\n",
                       vid, pal_strerror (ret));
              cli_ret = CLI_ERROR;
            }
        }
    }
 
  if (vlan_port_config == NSM_VLAN_CONFIGURED_ALL)
   {
     vlan_port->config = NSM_VLAN_CONFIGURED_ALL;
#ifdef HAVE_LACPD
     if (NSM_INTF_TYPE_AGGREGATOR(ifp))
        nsm_vlan_agg_update_config_flag (ifp);
#endif /* HAVE_LACPD */
   }
  
  l2_range_parser_deinit (&par);

  return cli_ret;
}

CLI (nsm_vlan_switchport_trunk_pn_cn_except,
     nsm_vlan_switchport_trunk_pn_cn_except_cmd,
     "switchport (trunk|provider-network|customer-network) "
     "allowed vlan except VLAN_ID",
     "Set the switching characteristics of the Layer2 interface",
     NSM_VLAN_TRUNK_STR,
     NSM_VLAN_PN_STR,
     NSM_VLAN_CN_STR,
     "Set the VLANs that will Xmit/Rx through the Layer2 interface",
     "VLAN that will be removed",
     "Allow all VLANs except VID to Xmit/Rx through the Layer2 interface",
     "The List of VIDs that will be removed from the Layer2 interface")
{
  int ret;
  nsm_vid_t vid;
  struct nsm_vlan_bmp excludeBmp;
  struct interface *ifp = cli->index;
  char *curr = NULL;
  l2_range_parser_t par;
  l2_range_parse_error_t r_err;

  NSM_VLAN_BMP_INIT (excludeBmp);

#ifdef HAVE_LACPD
  NSM_INTERFACE_CHECK_AGG_MEM_PROPERTY(cli, ifp);
#endif /* HAVE_LACPD */
  NSM_BRIDGE_CHECK_L2_PROPERTY(cli, ifp);
  NSM_BRIDGE_CHECK_BRIDGE_PROPERTY(cli, ifp);

  l2_range_parser_init (argv[1], &par);

  if (argv [0] [0] == 't')
    {
      if ((ret = l2_range_parser_validate (&par, cli, nsm_vlan_validate_vid1))
          != CLI_SUCCESS)
        return ret;
    }
  else
    {
      if ((ret = l2_range_parser_validate (&par, cli, nsm_vlan_validate_vid))
          != CLI_SUCCESS)
        return ret;
    }

  while ( (curr = l2_range_parser_get_next ( &par, &r_err) ))
    {
      if ( r_err != RANGE_PARSER_SUCCESS )
        {
          cli_out (cli, "%% Invalid Input %s \n", l2_range_parse_err_to_str (r_err) );
          return CLI_ERROR;
        }

      if (argv [0] [0] == 't')
        CLI_GET_INTEGER_RANGE ("vlanid", vid, curr, NSM_VLAN_DEFAULT_VID,
                               NSM_VLAN_CLI_MAX);
      else
        CLI_GET_INTEGER_RANGE ("vlanid", vid, curr, NSM_VLAN_CLI_MIN,
                               NSM_VLAN_CLI_MAX);      

      NSM_VLAN_BMP_SET (excludeBmp, vid);
    }

  l2_range_parser_deinit (&par);

  if (! pal_strncmp (argv[0], "p", 1))
    ret = nsm_vlan_add_all_except_vid (ifp, NSM_VLAN_PORT_MODE_PN,
                                       NSM_VLAN_PORT_MODE_PN,
                                       &excludeBmp, NSM_VLAN_EGRESS_TAGGED,
                                       PAL_TRUE, PAL_TRUE);
  else if (! pal_strncmp (argv[0], "c", 1))
    ret = nsm_vlan_add_all_except_vid (ifp, NSM_VLAN_PORT_MODE_CN,
                                       NSM_VLAN_PORT_MODE_CN,
                                       &excludeBmp, NSM_VLAN_EGRESS_TAGGED,
                                       PAL_TRUE, PAL_TRUE);
  else
    ret = nsm_vlan_add_all_except_vid (ifp, NSM_VLAN_PORT_MODE_TRUNK,
                                       NSM_VLAN_PORT_MODE_TRUNK,
                                       &excludeBmp, NSM_VLAN_EGRESS_TAGGED,
                                       PAL_TRUE, PAL_TRUE);

  if (ret < 0)
    {
      if (ret == NSM_VLAN_ERR_BRIDGE_NOT_FOUND)
        cli_out (cli, "%% Interface not bound to any bridge-group\n");
      else if (ret == NSM_VLAN_ERR_INVALID_MODE)
        cli_out (cli, "%% Port mode not set to Trunk\n");
      else
        cli_out (cli, "%% Can't add the VIDs to the port : %s\n",
                 pal_strerror (ret));
      return CLI_ERROR;
    }

  return CLI_SUCCESS;
}

CLI (nsm_vlan_switchport_trunk_native,
     nsm_vlan_switchport_trunk_native_cmd,
     "switchport trunk native vlan VLAN_ID",
     "Set the switching characteristics of the Layer2 interface",
     "Set the switching characteristics of the Layer2 interface to trunk",
     "Set the native VLAN for classifying untagged traffic through the Layer2 interface",
     "VLAN that will be added",
     "The native VLAN id")
{
  int ret;
  unsigned short vid = 0;
  struct interface *ifp = cli->index;
#ifdef HAVE_LACPD
  NSM_INTERFACE_CHECK_AGG_MEM_PROPERTY(cli, ifp);
#endif /* HAVE_LACPD */
  NSM_BRIDGE_CHECK_L2_PROPERTY(cli, ifp);
  NSM_BRIDGE_CHECK_BRIDGE_PROPERTY(cli, ifp);

  CLI_GET_INTEGER_RANGE ("vlanid", vid, argv[1], 1,
                         NSM_VLAN_CLI_MAX);

  ret = nsm_vlan_set_native_vlan (ifp, vid);
  if (ret < 0)
    {
      if (ret == NSM_VLAN_ERR_BRIDGE_NOT_FOUND)
        cli_out (cli, "%% Interface not bound to any bridge-group\n");
      else if (ret == NSM_VLAN_ERR_INVALID_MODE)
        cli_out (cli, "%% Port mode not set to Trunk\n");
      else if (ret == NSM_VLAN_ERR_VLAN_NOT_IN_PORT)
        cli_out (cli, "%% Port not a member of vlan \n");
      else
        cli_out (cli, "%% Can't set native VID to port: %s\n", pal_strerror (ret));
      return CLI_ERROR;
    }

  return CLI_SUCCESS;
}

CLI (no_nsm_vlan_switchport_trunk_native,
     no_nsm_vlan_switchport_trunk_native_cmd,
     "no switchport trunk native vlan",
     CLI_NO_STR,
     "Reset the switching characteristics of the Layer2 interface",
     "Reset the switching characteristics of the Layer2 trunk interface",
     "Reset the native vlan of the Layer2 trunk interface",
     "Reset the native vlan of the Layer2 trunk interface to default vlan id")
{

  int ret;
  struct interface *ifp = cli->index;
#ifdef HAVE_LACPD
  NSM_INTERFACE_CHECK_AGG_MEM_PROPERTY(cli, ifp);
#endif /* HAVE_LACPD */
  NSM_BRIDGE_CHECK_L2_PROPERTY(cli, ifp);
  NSM_BRIDGE_CHECK_BRIDGE_PROPERTY(cli, ifp);

  ret = nsm_vlan_set_native_vlan (ifp, NSM_VLAN_DEFAULT_VID);
  if (ret < 0)
    {
      if (ret == NSM_VLAN_ERR_BRIDGE_NOT_FOUND)
        cli_out (cli, "%% Interface not bound to any bridge-group\n");
      else if (ret == NSM_VLAN_ERR_INVALID_MODE)
        cli_out (cli, "%% Port mode not set to Trunk\n");
      else if (ret == NSM_VLAN_ERR_VLAN_NOT_IN_PORT)
        cli_out (cli, "%% Port not a member of vlan \n");
      else
        cli_out (cli, "%% Can't set native VID to port: %s\n", pal_strerror (ret));
      return CLI_ERROR;
    }

  return CLI_SUCCESS;

}

CLI (no_nsm_vlan_switchport_trunk_vlan,
     no_nsm_vlan_switchport_trunk_vlan_cmd,
     "no switchport trunk",
     CLI_NO_STR,
     "Reset the switching characteristics of the Layer2 interface",
     "Reset the switching characteristics of the Layer2 interface to access")
{
  int ret;
  struct interface *ifp = cli->index;

#ifdef HAVE_LACPD
  NSM_INTERFACE_CHECK_AGG_MEM_PROPERTY(cli, ifp);
#endif /* HAVE_LACPD */
  NSM_BRIDGE_CHECK_L2_PROPERTY(cli, ifp);
  NSM_BRIDGE_CHECK_BRIDGE_PROPERTY(cli, ifp);
 
  ret =  nsm_vlan_clear_trunk_port (ifp, PAL_TRUE, PAL_TRUE);
  if (ret < 0)
    {
      if (ret == NSM_VLAN_ERR_BRIDGE_NOT_FOUND)
        cli_out (cli, "%% Interface not bound to any bridge-group\n");
      else if (ret == NSM_VLAN_ERR_INVALID_MODE)
        cli_out (cli, "%% Port mode not set to Trunk\n");
#ifdef HAVE_LACPD
      else if (ret == AGG_MEM_NO_SWITCHPORT)
        cli_out (cli, "%% Member interface not configured for switching\n");
      else if (ret == AGG_MEM_BRIDGE_NOT_VLAN_AWARE)
        cli_out (cli, "%% Member interface not vlan aware\n");
#endif /* HAVE LACP */
      else
        cli_out (cli, "%% Can't reset the port from Trunk\n");
      return CLI_ERROR;
    }

  return CLI_SUCCESS;
}

#ifdef HAVE_PROVIDER_BRIDGE

CLI (nsm_vlan_switchport_ce_vlan_reg,
     nsm_vlan_switchport_ce_vlan_reg_cmd,
     "switchport customer-edge vlan registration WORD",
     "Set the switching characteristics of the Layer2 interface",
     NSM_VLAN_CE_STR,
     "Configure the vlan parameters",
     "Configure the vlan registration parameters",
     "C-VLAN registration table name")
{
  int ret;
  struct interface *ifp = cli->index;

#ifdef HAVE_LACPD
  NSM_INTERFACE_CHECK_AGG_MEM_PROPERTY(cli, ifp);
#endif /* HAVE_LACPD */
  NSM_BRIDGE_CHECK_L2_PROPERTY(cli, ifp);
  NSM_BRIDGE_CHECK_BRIDGE_PROPERTY(cli, ifp);

  ret =  nsm_cvlan_reg_tab_if_apply (ifp, argv [0]);

  return nsm_vlan_cli_return (cli, ret, NULL, NULL, NSM_VLAN_NULL_VID);
}

CLI (no_nsm_vlan_switchport_ce_vlan_reg,
     no_nsm_vlan_switchport_ce_vlan_reg_cmd,
     "no switchport customer-edge vlan registration",
     "Set the switching characteristics of the Layer2 interface",
     CLI_NO_STR,
     NSM_VLAN_CE_STR,
     "Configure the vlan parameters",
     "Configure the vlan registration parameters",
     "C-VLAN registration table name")
{
  int ret;
  struct interface *ifp = cli->index;

#ifdef HAVE_LACPD
  NSM_INTERFACE_CHECK_AGG_MEM_PROPERTY(cli, ifp);
#endif /* HAVE_LACPD */
  NSM_BRIDGE_CHECK_L2_PROPERTY(cli, ifp);
  NSM_BRIDGE_CHECK_BRIDGE_PROPERTY(cli, ifp);

  ret =  nsm_cvlan_reg_tab_if_delete (ifp);

  return nsm_vlan_cli_return (cli, ret, NULL, NULL, NSM_VLAN_NULL_VID);

}

CLI (nsm_vlan_switchport_cn_svlan_trans,
     nsm_vlan_switchport_cn_svlan_trans_cmd,
     "switchport customer-network vlan translation svlan "
     "VLAN_ID svlan VLAN_ID",
     "Set the switching characteristics of the Layer2 interface",
     NSM_VLAN_CN_STR,
     "Configure the VLAN translation",
     "Configure VLAN Translation table for the interface",
     "S-VLAN to be translated",
     "S-VLAN ID of S-VLAN to be translated",
     "Translated S-VLAN",
     "Translated S-VLAN ID")
{
  int ret;
  u_int16_t svid;
  u_int16_t trans_svid;
  struct interface *ifp = cli->index;

#ifdef HAVE_LACPD
  NSM_INTERFACE_CHECK_AGG_MEM_PROPERTY(cli, ifp);
#endif /* HAVE_LACPD */
  NSM_BRIDGE_CHECK_L2_PROPERTY(cli, ifp);
  NSM_BRIDGE_CHECK_BRIDGE_PROPERTY(cli, ifp);

  CLI_GET_INTEGER_RANGE ("vlanid", svid, argv [1], NSM_VLAN_DEFAULT_VID,
                         NSM_VLAN_CLI_MAX);

  CLI_GET_INTEGER_RANGE ("Translated vlanid", trans_svid, argv [2], NSM_VLAN_DEFAULT_VID,
                         NSM_VLAN_CLI_MAX);

  ret =  nsm_vlan_trans_tab_entry_add (ifp, svid, trans_svid);

  return nsm_vlan_cli_return (cli, ret, NULL, NULL, NSM_VLAN_NULL_VID);

}

CLI (no_nsm_vlan_switchport_cn_svlan_trans,
     no_nsm_vlan_switchport_cn_svlan_trans_cmd,
     "no switchport customer-network vlan translation svlan VLAN_ID",
     CLI_NO_STR,
     "Set the switching characteristics of the Layer2 interface",
     NSM_VLAN_CN_STR,
     "Configure the VLAN translation",
     "Configure VLAN Translation table for the interface",
     "S-VLAN to be translated",
     "S-VLAN ID of S-VLAN to be translated")
{
  int ret;
  u_int16_t svid;
  struct interface *ifp = cli->index;

#ifdef HAVE_LACPD
  NSM_INTERFACE_CHECK_AGG_MEM_PROPERTY(cli, ifp);
#endif /* HAVE_LACPD */
  NSM_BRIDGE_CHECK_L2_PROPERTY(cli, ifp);
  NSM_BRIDGE_CHECK_BRIDGE_PROPERTY(cli, ifp);

  CLI_GET_INTEGER_RANGE ("vlanid", svid, argv [0], NSM_VLAN_DEFAULT_VID,
                         NSM_VLAN_CLI_MAX);

  ret =  nsm_vlan_trans_tab_entry_delete (ifp, svid);

  return nsm_vlan_cli_return (cli, ret, NULL, NULL, NSM_VLAN_NULL_VID);
}

CLI (nsm_vlan_switchport_pn_svlan_trans,
     nsm_vlan_switchport_pn_svlan_trans_cmd,
     "switchport provider-network vlan translation svlan "
     "VLAN_ID svlan VLAN_ID",
     "Set the switching characteristics of the Layer2 interface",
     NSM_VLAN_PN_STR,
     "Configure the VLAN translation",
     "Configure VLAN Translation table for the interface",
     "S-VLAN to be translated",
     "S-VLAN ID of tag to be translated",
     "Translated S-VLAN",
     "Translated S-VLAN ID")
{
  int ret;
  u_int16_t svid;
  u_int16_t trans_svid;
  struct interface *ifp = cli->index;

#ifdef HAVE_LACPD
  NSM_INTERFACE_CHECK_AGG_MEM_PROPERTY(cli, ifp);
#endif /* HAVE_LACPD */
  NSM_BRIDGE_CHECK_L2_PROPERTY(cli, ifp);
  NSM_BRIDGE_CHECK_BRIDGE_PROPERTY(cli, ifp);

  CLI_GET_INTEGER_RANGE ("vlanid", svid, argv [1], NSM_VLAN_DEFAULT_VID,
                         NSM_VLAN_CLI_MAX);

  CLI_GET_INTEGER_RANGE ("Translated vlanid", trans_svid, argv [2],
                         NSM_VLAN_DEFAULT_VID, NSM_VLAN_CLI_MAX);

  ret = nsm_vlan_trans_tab_entry_add (ifp, svid, trans_svid);

  return nsm_vlan_cli_return (cli, ret, NULL, NULL, NSM_VLAN_NULL_VID);

}

CLI (no_nsm_vlan_switchport_pn_svlan_trans,
     no_nsm_vlan_switchport_pn_svlan_trans_cmd,
     "no switchport provider-network vlan translation svlan VLAN_ID",
     CLI_NO_STR,
     "Set the switching characteristics of the Layer2 interface",
     NSM_VLAN_PN_STR,
     "Configure the VLAN translation",
     "Configure VLAN Translation table for the interface",
     "S-VLAN to be translated",
     "S-VLAN ID of S-VLAN to be translated")
{
  int ret;
  u_int16_t svid;
  struct interface *ifp = cli->index;

#ifdef HAVE_LACPD
  NSM_INTERFACE_CHECK_AGG_MEM_PROPERTY(cli, ifp);
#endif /* HAVE_LACPD */
  NSM_BRIDGE_CHECK_L2_PROPERTY(cli, ifp);
  NSM_BRIDGE_CHECK_BRIDGE_PROPERTY(cli, ifp);

  CLI_GET_INTEGER_RANGE ("vlanid", svid, argv [0], NSM_VLAN_DEFAULT_VID,
                         NSM_VLAN_CLI_MAX);

  ret =  nsm_vlan_trans_tab_entry_delete (ifp, svid);

  return nsm_vlan_cli_return (cli, ret, NULL, NULL, NSM_VLAN_NULL_VID);
}

CLI (nsm_vlan_switchport_ce_vlan_trans,
     nsm_vlan_switchport_ce_vlan_trans_cmd,
     "switchport customer-edge vlan translation vlan "
     "VLAN_ID vlan VLAN_ID",
     "Set the switching characteristics of the Layer2 interface",
     NSM_VLAN_CE_STR,
     "Configure the VLAN translation",
     "Configure VLAN Translation table for the interface",
     "C-VLAN to be translated",
     "C-VLAN ID of tag to be translated",
     "Translated C-VLAN",
     "Translated C-VLAN ID")
{
  int ret;
  u_int16_t vid;
  u_int16_t trans_vid;
  struct interface *ifp = cli->index;

#ifdef HAVE_LACPD
  NSM_INTERFACE_CHECK_AGG_MEM_PROPERTY(cli, ifp);
#endif /* HAVE_LACPD */
  NSM_BRIDGE_CHECK_L2_PROPERTY(cli, ifp);
  NSM_BRIDGE_CHECK_BRIDGE_PROPERTY(cli, ifp);

  CLI_GET_INTEGER_RANGE ("vlanid", vid, argv [0], NSM_VLAN_DEFAULT_VID,
                         NSM_VLAN_CLI_MAX);

  CLI_GET_INTEGER_RANGE ("Translated vlanid", trans_vid, argv [1],
                         NSM_VLAN_DEFAULT_VID, NSM_VLAN_CLI_MAX);

  ret = nsm_vlan_trans_tab_entry_add (ifp, vid, trans_vid);

  return nsm_vlan_cli_return (cli, ret, NULL, NULL, NSM_VLAN_NULL_VID);

}

CLI (no_nsm_vlan_switchport_ce_vlan_trans,
     no_nsm_vlan_switchport_ce_vlan_trans_cmd,
     "no switchport customer-edge vlan translation vlan VLAN_ID",
     CLI_NO_STR,
     "Set the switching characteristics of the Layer2 interface",
     NSM_VLAN_CE_STR,
     "Configure the VLAN translation",
     "Configure VLAN Translation table for the interface",
     "C-VLAN to be translated",
     "C-VLAN ID of tag to be translated")
{
  int ret;
  u_int16_t vid;
  struct interface *ifp = cli->index;

#ifdef HAVE_LACPD
  NSM_INTERFACE_CHECK_AGG_MEM_PROPERTY(cli, ifp);
#endif /* HAVE_LACPD */
  NSM_BRIDGE_CHECK_L2_PROPERTY(cli, ifp);
  NSM_BRIDGE_CHECK_BRIDGE_PROPERTY(cli, ifp);

  CLI_GET_INTEGER_RANGE ("vlanid", vid, argv [0], NSM_VLAN_DEFAULT_VID,
                         NSM_VLAN_CLI_MAX);

  ret = nsm_vlan_trans_tab_entry_delete (ifp, vid);

  return nsm_vlan_cli_return (cli, ret, NULL, NULL, NSM_VLAN_NULL_VID);

}

#endif /* HAVE_PROVIDER_BRIDGE */

#ifdef HAVE_PBB_TE
CLI (nsm_pbb_te_vlan_allowed_forbidden,
     nsm_pbb_te_vlan_allowed_forbidden_cmd,
     "vlan "NSM_VLAN_CLI_RNG" bridge (<1-32>|backbone) "
     "(allowed|forbidden) (multicast|) (add|remove) interface IFNAME",
     CLI_VLAN_STR,
     "VLAN id",
     NSM_BRIDGE_STR,
     NSM_BRIDGE_NAME_STR,
     NSM_BRIDGE_NAME_STR,
     "allowed list",
     "forbidden list",
     "specifies the entry as a multicast",
     "Adds the interface in the vlan list",
     "Deletes the interface from the vlan list",
     CLI_INTERFACE_STR,
     "name of the interface")
{
  int ret;
  int brid;
  u_int16_t vid;
  bool_t is_allowed = PAL_FALSE;
  bool_t multicast = PAL_FALSE;
  struct interface *ifp = NULL;
  struct nsm_master *nm = cli->vr->proto;
  struct nsm_bridge_master *master = nsm_bridge_get_master (nm);
  char *ifname;                                                                                                                                             
  /* Get the vid and check if it is within range (2-4096).  */
  CLI_GET_INTEGER_RANGE ("VLAN id", vid, argv[0], NSM_VLAN_CLI_MIN, NSM_VLAN_CLI_MAX);

  /* Get the brid and check if it is within range. */
  if (argv[1][0] != 'b')
    CLI_GET_INTEGER_RANGE ("Bridge group", brid, argv[1], 1, 32);

  if (argv[3][0] == 'm')
    multicast = PAL_TRUE;
  
  if (argv[2][0] == 'a')
    is_allowed = PAL_TRUE;

  if (multicast)
    ifname = argv[5];
  else 
    ifname = argv[4];

  if ( (ifp = if_lookup_by_name(&cli->vr->ifm, ifname)) == NULL )
    {
      cli_out (cli, "%% Interface not found\n");
      return CLI_ERROR;
    }
 
  /* Update the VLAN. */
  if ((multicast && argv[4][0] == 'a') || (!multicast && argv[3][0] == 'a' ))
    ret = nsm_vlan_pbb_te_add_interface(master, argv[1], vid, ifp, is_allowed,
                                        multicast);
  else
    ret = nsm_vlan_pbb_te_delete_interface(master, argv[1], vid, ifp, is_allowed,
                                           multicast);
  if (ret < 0)
    {
      cli_out (cli, "%%Error adding wildcard filter\n");
      return CLI_ERROR;
    }
  return CLI_SUCCESS;
}


#endif /* HAVE_PBB_TE */

CLI (show_interfaces_switchport_bridge,
     show_interface_switchport_bridge_cmd,
     "show interface switchport bridge <1-32>",
     "Display the characteristics of the Layer2 interface",
     "The layer2 interfaces",
     "Display the modes of the Layer2 interfaces",
     "The bridge to use with this VLAN",
     "The Bridge Group number")
{
  int brid;

  /* Get the brid and check if it is within range. */
  CLI_GET_INTEGER_RANGE ("Bridge group", brid, argv[0], 1, 32);

  nsm_all_port_vlan_show(cli, argv[0]); 

  return CLI_SUCCESS;
}

#ifdef HAVE_QOS

CLI (traffic_class_table,
     traffic_class_table_cmd,
     "traffic-class-table user-priority <0-7> num-traffic-classes <1-8> value <0-7>",
     "Set the traffic class tables values",
     "User priority associated with the traffic class table",
     "value <0-7>",
     "Number of traffic classes that are supported",
     "Number of traffic classes value <1-8>",
     "Value to be used for the given user priority/num traffic classes",
     "Value <0-7>")
{
  int ret;
  signed int user_priority = 0;
  signed int num_traffic_classes = 0;
  unsigned int traffic_class_value = 0;
  struct interface *ifp = cli->index;
  if (ifp == NULL)
    return CLI_ERROR;
  CLI_GET_INTEGER_RANGE ("user_priority", user_priority, argv[0],
                         HAL_BRIDGE_MIN_USER_PRIO,
                         HAL_BRIDGE_MAX_USER_PRIO);
  CLI_GET_INTEGER_RANGE ("num_traffic_classes", num_traffic_classes, argv[1],
                         HAL_BRIDGE_MIN_TRAFFIC_CLASS,
                         HAL_BRIDGE_MAX_TRAFFIC_CLASS);
  CLI_GET_UINT32_RANGE ("traffic_class_value", traffic_class_value, argv[2],
                         TRAFFIC_CLASS_VALUE_MIN,
                         TRAFFIC_CLASS_VALUE_MAX);
  ret = nsm_vlan_port_set_traffic_class_table (ifp,
                                               user_priority, num_traffic_classes,
                                               traffic_class_value);
  if ( ret < 0 )
    {
      switch (ret)
        {
        case NSM_VLAN_ERR_IFP_NOT_BOUND:
          cli_out (cli, "%% Interface not bound to bridge \n", ret);
          return CLI_ERROR;
        case NSM_VLAN_ERR_BRIDGE_NOT_VLAN_AWARE:
          cli_out (cli, "%% Bridge Not VLAN aware \n", ret);
          return CLI_ERROR;
        case NSM_VLAN_ERR_IFP_INVALID:
          cli_out(cli, "%% Cannot set traffic-class table on Aggregator port \n", ret);
          return CLI_ERROR;
        case NSM_VLAN_ERR_BRIDGE_NOT_FOUND:
        default:
          cli_out (cli, "%% Cannot set the port's traffic-class table: %d\n", ret);
          return CLI_ERROR;
        }
    }

  return CLI_SUCCESS;
}

CLI (default_user_priority,
     default_user_priority_cmd,
     "user-priority <0-7>",
     "Set the default user priority associated with the layer2 interface",
     "User priority value")
{
  int ret;
  signed int user_priority;
  struct interface *ifp = cli->index;
  if (ifp == NULL)
    return CLI_ERROR;
  CLI_GET_INTEGER_RANGE ("user_priority", user_priority, argv[0],
                         HAL_BRIDGE_MIN_USER_PRIO,
                         HAL_BRIDGE_MAX_USER_PRIO);
  ret = nsm_vlan_port_set_default_user_priority (ifp, user_priority);

  if ( ret < 0 )
    {
      switch (ret)
        {
        case NSM_VLAN_ERR_IFP_NOT_BOUND:
          cli_out (cli, "%% Interface not bound to bridge \n", ret);
          return CLI_ERROR;
        case NSM_VLAN_ERR_BRIDGE_NOT_VLAN_AWARE:
          cli_out (cli, "%% Bridge Not VLAN aware \n", ret);
          return CLI_ERROR;
        case NSM_VLAN_ERR_IFP_INVALID:
          cli_out(cli, "%% Cannot set user-priority on Aggregator port \n", ret);
          return CLI_ERROR;
        case NSM_VLAN_ERR_BRIDGE_NOT_FOUND:
        default:
          cli_out (cli, "%% Cannot set the port's default user priority: %d\n", ret);
          return CLI_ERROR;
        }
    }

  return CLI_SUCCESS;

}

CLI (user_prio_regen_table,
     user_prio_regen_table_cmd,
     "user-priority-regen-table user-priority <0-7> regenerated-user-priority <0-7>",
     "Set the value for the mapping of user-priority to regenerated user priority",
     "User priority associated with the regeneration table",
     "User priority value",
     "Regenerated values to be used for the user priority ",
     "Regenerated user priority value")
{
  int ret;
  signed int recvd_user_priority = 0;
  signed int regen_user_priority = 0;
  struct interface *ifp = cli->index;

  if (ifp == NULL)
    return CLI_ERROR;

  CLI_GET_INTEGER_RANGE ("recvd_user_priority", recvd_user_priority, argv[0],
                         HAL_BRIDGE_MIN_USER_PRIO,
                         HAL_BRIDGE_MAX_USER_PRIO);

  CLI_GET_INTEGER_RANGE ("regen_user_priority", regen_user_priority, argv[1],
                         HAL_BRIDGE_MIN_USER_PRIO,
                         HAL_BRIDGE_MAX_USER_PRIO);

  ret = nsm_vlan_port_set_regen_user_priority (ifp, recvd_user_priority,
                                               regen_user_priority);
  if ( ret < 0 )
    {
      switch (ret)
        {
        case NSM_VLAN_ERR_IFP_NOT_BOUND:
          cli_out (cli, "%% Interface not bound to bridge \n", ret);
          return CLI_ERROR;
        case NSM_VLAN_ERR_BRIDGE_NOT_VLAN_AWARE:
          cli_out (cli, "%% Bridge Not VLAN aware \n", ret);
          return CLI_ERROR;
        case NSM_VLAN_ERR_BRIDGE_NOT_FOUND:
        default:
          cli_out (cli, "%% Cannot set the port's regeneration table: %d\n", ret);
          return CLI_ERROR;
        }
    }

  return CLI_SUCCESS;
}

CLI (show_traffic_class_table,
     show_traffic_class_table_cmd,
     "show traffic-class-table interface IFNAME",
     CLI_SHOW_STR,
     "Display traffic class table",
     CLI_INTERFACE_STR,
     CLI_IFNAME_STR)
{
  int ret = 0;
  struct interface *ifp = if_lookup_by_name (&cli->vr->ifm, argv[0]);
  u_char user_priority;
  u_char num_traffic_classes;
  struct nsm_vlan_port *vlan_port = NULL;
  struct nsm_bridge_port *br_port;
  struct nsm_bridge *bridge = NULL;
  struct nsm_if *zif = NULL;

  if (ifp == NULL)
    {
      cli_out (cli, "%% Can't find interface %s\n", argv[0]);
      return CLI_ERROR;
    }

  zif = (struct nsm_if *)ifp->info;

  if (zif)
    {
      if (zif->bridge)
        {
          bridge    = zif->bridge;

          if ((br_port = zif->switchport) == NULL)
             ret = NSM_VLAN_ERR_IFP_NOT_BOUND;

          vlan_port = &br_port->vlan_port;
          if ( nsm_bridge_is_vlan_aware(bridge->master, bridge->name) != PAL_TRUE )
            ret = NSM_VLAN_ERR_BRIDGE_NOT_VLAN_AWARE;
        }
      else
        {
          ret = NSM_VLAN_ERR_BRIDGE_NOT_FOUND;
        }
    }
  else
    {
      ret = NSM_VLAN_ERR_IFP_NOT_BOUND;
    }

  if ( ret < 0 )
    {
      switch (ret)
        {
        case NSM_VLAN_ERR_IFP_NOT_BOUND:
          cli_out (cli, "%% Interface not bound to bridge \n", ret);
          return CLI_ERROR;
        case NSM_VLAN_ERR_BRIDGE_NOT_VLAN_AWARE:
          cli_out (cli, "%% Bridge Not VLAN aware \n", ret);
          return CLI_ERROR;
        case NSM_VLAN_ERR_BRIDGE_NOT_FOUND:
        default:
          cli_out (cli, "%% Cannot get the port's traffic-class table: %d\n", ret);
          return CLI_ERROR;
        }
    }

  cli_out (cli, "User Prio / Num Traffic Classes \n");
  cli_out (cli, "       ");

  for (num_traffic_classes = HAL_BRIDGE_MIN_TRAFFIC_CLASS;
       num_traffic_classes <= HAL_BRIDGE_MAX_TRAFFIC_CLASS;
       num_traffic_classes++)
    {
      cli_out (cli, "%u  ", num_traffic_classes);
    }

  cli_out (cli, "\n");

  for (user_priority = HAL_BRIDGE_MIN_USER_PRIO;
       user_priority <= HAL_BRIDGE_MAX_USER_PRIO; user_priority++)
    {
      cli_out (cli, "   %u   ", user_priority);
#ifdef HAVE_HAL
      for (num_traffic_classes = HAL_BRIDGE_MIN_TRAFFIC_CLASS - 1;
           num_traffic_classes < HAL_BRIDGE_MAX_TRAFFIC_CLASS;
           num_traffic_classes++)
        {
          cli_out (cli, "%u  ",
                   vlan_port->traffic_class_table[user_priority][num_traffic_classes]);
        }
#endif /* HAVE_HAL */
      cli_out (cli, "\n");
    }

  return CLI_SUCCESS;

}

CLI (show_default_priority,
     show_default_priority_cmd,
     "show user-priority interface IFNAME",
     CLI_SHOW_STR,
     "Display the default user priority associated with the layer2 interface",
     CLI_INTERFACE_STR,
     CLI_IFNAME_STR)
{
  int ret;
  struct interface *ifp = if_lookup_by_name (&cli->vr->ifm, argv[0]);

  if (ifp == NULL) {
    cli_out (cli, "%% Can't find interface %s\n", argv[0]);
    return CLI_ERROR;
  }

  ret = nsm_vlan_port_get_default_user_priority (ifp);

  if ( ret < 0 )
    {
      switch (ret)
        {
        case NSM_VLAN_ERR_IFP_NOT_BOUND:
          cli_out (cli, "%% Interface not bound to bridge \n", ret);
          return CLI_ERROR;
        case NSM_VLAN_ERR_BRIDGE_NOT_VLAN_AWARE:
          cli_out (cli, "%% Bridge Not VLAN aware \n", ret);
          return CLI_ERROR;
        case NSM_VLAN_ERR_BRIDGE_NOT_FOUND:
        default:
          cli_out (cli, "%% Cannot get the port's default user priority: %d\n", ret);
          return CLI_ERROR;
        }
    }

  cli_out (cli, "Default user priority : %u\n", ret);
  return CLI_SUCCESS;

}

CLI (show_user_prio_regen_table,
     show_user_prio_regen_table_cmd,
     "show user-priority-regen-table interface IFNAME",
     CLI_SHOW_STR,
     "Display the User priority to regenerated user priority mapping associated with the layer2 interface",
     CLI_INTERFACE_STR, CLI_IFNAME_STR)
{
  int ret = 0;
  unsigned int user_priority;
  struct interface *ifp = if_lookup_by_name (&cli->vr->ifm, argv[0]);
  struct nsm_vlan_port *vlan_port = NULL;
  struct nsm_bridge_port *br_port = NULL;
  struct nsm_bridge *bridge = NULL;
  struct nsm_if *zif = NULL;

  if (ifp == NULL)
    {
      cli_out (cli, "%% Interface does not exist: %s\n", argv[0]);
      return CLI_ERROR;
    }

  zif = (struct nsm_if *)ifp->info;
  if (zif)
    {
      if (zif->bridge)
        {
          bridge    = zif->bridge;

          if ((br_port = zif->switchport) == NULL)
             ret = NSM_VLAN_ERR_IFP_NOT_BOUND;

          vlan_port = &br_port->vlan_port;
          if ( nsm_bridge_is_vlan_aware(bridge->master, bridge->name) != PAL_TRUE )
            ret = NSM_VLAN_ERR_BRIDGE_NOT_VLAN_AWARE;
        }
      else
        {
          ret = NSM_VLAN_ERR_BRIDGE_NOT_FOUND;
        }
    }
  else
    {
      ret = NSM_VLAN_ERR_IFP_NOT_BOUND;
    }

  if ( ret < 0 )
    {
      switch (ret)
        {
        case NSM_VLAN_ERR_IFP_NOT_BOUND:
          cli_out (cli, "%% Interface not bound to bridge \n", ret);
          return CLI_ERROR;
        case NSM_VLAN_ERR_BRIDGE_NOT_VLAN_AWARE:
          cli_out (cli, "%% Bridge Not VLAN aware \n", ret);
          return CLI_ERROR;
        case NSM_VLAN_ERR_BRIDGE_NOT_FOUND:
        default:
          cli_out (cli, "%% Cannot get the port's default user priority: %d\n", ret);
          return CLI_ERROR;
        }
    }

#ifdef HAVE_HAL
  cli_out (cli, "User Priority     Regenerated user priority\n");

  for (user_priority = 0; user_priority <= HAL_BRIDGE_MAX_USER_PRIO;
       user_priority++)
    {

      cli_out (cli, "     %u                       %u           \n",
               user_priority, vlan_port->user_prio_regn_table[user_priority]);
    }
#endif /* HAVE_HAL */

  return CLI_SUCCESS;

}

#endif /* HAVE_QOS */

CLI (show_nsm_vlan_bridge,
     show_nsm_vlan_bridge_cmd,
     "show vlan (all|static|dynamic|auto) bridge <1-32>",
     CLI_SHOW_STR,
     "Display VLAN information",
     "All VLANs(static and dynamic)",
     "Static VLANs",
     "Dynamic VLANS",
     "Auto configured VLANS",
     "The Bridge Group to use with this VLAN",
     "The Bridge Group number")
{
  int brid;
  int type = 0;
  u_int8_t *bridge_name;
   
  /* Get the brid and check if it is within range. */
  if ( argc > 1 )
  CLI_GET_INTEGER_RANGE ("Bridge group", brid, argv[1], 1, 32);
 
  if (! pal_strncmp (argv[0], "al", 2))
    type = NSM_VLAN_STATIC | NSM_VLAN_DYNAMIC | NSM_VLAN_AUTO;
  else if (! pal_strncmp (argv[0], "s", 1))
    type = NSM_VLAN_STATIC;
  else if (! pal_strncmp (argv[0], "d", 1))
    type = NSM_VLAN_DYNAMIC;
  else if (! pal_strncmp (argv[0], "au", 2))
    type = NSM_VLAN_AUTO;
  else
    {
      /* For compilation sanity */
      cli_out(cli, "%% Invalid Type option\n");
      return CLI_ERROR;
    }
  if ( argc > 1 )
    bridge_name = argv[1];
  else 
    bridge_name = def_bridge_name;

  nsm_all_vlan_show(cli, bridge_name, type, 0);

  return CLI_SUCCESS;
}

#ifdef HAVE_DEFAULT_BRIDGE
ALI (show_nsm_vlan_bridge,
     show_nsm_vlan_spanningtree_bridge_cmd,
     "show vlan (all|static|dynamic|auto)",
     CLI_SHOW_STR,
     "Display VLAN information",
     "All VLANs(static and dynamic)",
     "Static VLANs",
     "Dynamic VLANS",
     "Auto configured VLANS");
#endif /* HAVE_DEFAULT_BRIDGE */


CLI (show_nsm_vlan_brief,
     show_nsm_vlan_brief_cmd,
     "show vlan (brief | "NSM_VLAN_CLI_RNG")",
     CLI_SHOW_STR,
     "Display VLAN information",
     "VLAN information for all bridges (static and dynamic)",
     "VLAN id")
{
  int type;
  nsm_vid_t vid = 0;
  struct nsm_master *nm = cli->vr->proto;
  struct nsm_bridge_master *master = nsm_bridge_get_master (nm);
  struct nsm_bridge *curr = NULL;

  type = NSM_VLAN_STATIC | NSM_VLAN_DYNAMIC | NSM_VLAN_AUTO;

  if (! master)
    return CLI_SUCCESS;

  curr = master->bridge_list;

  if ( pal_strncmp (argv[0], "b", 1))
    {
      CLI_GET_INTEGER_RANGE ("vlanid", vid, argv[0], NSM_VLAN_CLI_MIN,
                             NSM_VLAN_CLI_MAX);
    }

  while (curr)
    {
      cli_out (cli, "%35s%s : %s\n\n", " ", "Bridge Group", curr->name);

      nsm_all_vlan_show (cli, curr->name, type, vid);

      cli_out (cli, "\n");

      curr = curr->next;
    }
#ifdef HAVE_B_BEB
  curr = master->b_bridge;
  if (curr)
    {
      cli_out (cli, "%35s%s : %s\n\n", " ", "Bridge Group", curr->name);

      nsm_all_vlan_show (cli, curr->name, type, vid);

      cli_out (cli, "\n");
    }
#endif /*HAVE_B_BEB*/

  return CLI_SUCCESS;
}

CLI (show_nsm_vlan_static_ports,
     show_nsm_vlan_static_ports_cmd,
     "show vlan (<2-4094> static-ports | static-ports)",
     CLI_SHOW_STR,
     "Display VLAN information",
     "VLAN id",
     "Display static egress/forbidden ports",
     "Display static egress/forbidden ports")
{
  int type;
  nsm_vid_t vid = 0;
  struct nsm_master *nm = cli->vr->proto;
  struct nsm_bridge_master *master = nsm_bridge_get_master (nm);
  struct nsm_bridge *curr = NULL;

  type = NSM_VLAN_STATIC | NSM_VLAN_DYNAMIC;

  if (! master)
    return CLI_SUCCESS;

  curr = master->bridge_list;

  if (argc > 1)
    {
      CLI_GET_INTEGER_RANGE ("vlanid", vid, argv[0], NSM_VLAN_CLI_MIN,
                             NSM_VLAN_CLI_MAX);
    }

  cli_out (cli, "%-16s%-16s%-32s\n", "Bridge", "VLAN ID", "Forbidden-egress-ports");

  cli_out (cli, "%-16s%-16s%-32s\n", "======", "========", "======================");

  while (curr)
    {
      nsm_vlan_show_static_ports (cli, curr->name, vid);

      cli_out (cli, "\n");

      curr = curr->next;
    }

  return CLI_SUCCESS;
}

#ifdef HAVE_PROVIDER_BRIDGE

void
nsm_cvlan_reg_show (struct cli *cli,
                    struct nsm_bridge *bridge,
                    struct nsm_cvlan_reg_tab *regtab)
{
  u_int16_t row_len;
  struct listnode *nn;
  u_int16_t row_offset;
  struct avl_node *node;
  struct interface *ifp;
  struct nsm_cvlan_reg_key *key;

  cli_out (cli, "%-16s%-16s%-58s\n", "Bridge", "Table Name", " Port List");
  cli_out (cli, "%-16s%-16s%-58s\n", "======", "==========", " =========");

  row_offset = 32;
  row_len = row_offset;

  cli_out (cli, "%-16s%-16s", bridge->name, regtab->name);

  LIST_LOOP (regtab->port_list, ifp, nn)
    {
      if ( (row_len + pal_strlen (ifp->name) + 2) > 80)
        {
          cli_out (cli, "\n");
          cli_out (cli, "%*c", row_offset, ' ');
          row_len = row_offset;
        }

      if (nn->next)
        cli_out (cli, " %-s,", ifp->name);
      else
        cli_out (cli, " %-s", ifp->name);
    }

  cli_out (cli, "\n");
  cli_out (cli, "\n");

  cli_out (cli, "%-16s%-16s\n", "CVLAN ID", "SVLAN ID");
  cli_out (cli, "%-16s%-16s\n", "========", "========");

  for (node = avl_top (regtab->reg_tab); node; node = avl_next (node))
   {
     if (! (key = AVL_NODE_INFO (node)))
       continue;

     cli_out (cli, "%-16d%-16d\n", key->cvid, key->svid);
   }

  cli_out (cli, "\n");
}

void
nsm_all_cvlan_reg_show (struct cli *cli,
                        struct nsm_bridge_master *master,
                        char *cvlan_reg_tab_name,
                        char *bridge_name)
{
  struct listnode *nn;
  struct nsm_bridge *bridge;
  struct nsm_cvlan_reg_tab *regtab;

  if (bridge_name && cvlan_reg_tab_name)
    {
      bridge = nsm_lookup_bridge_by_name (master, bridge_name);

      if (!bridge)
        return;

      regtab = nsm_cvlan_reg_tab_lookup (bridge, cvlan_reg_tab_name);

      if (!regtab)
        return;

      nsm_cvlan_reg_show (cli, bridge, regtab);
    }
  else if (cvlan_reg_tab_name)
    {
      for (bridge = master->bridge_list; bridge; bridge = bridge->next)
        {
          regtab = nsm_cvlan_reg_tab_lookup (bridge, cvlan_reg_tab_name);

          if (!regtab)
            continue;

          nsm_cvlan_reg_show (cli, bridge, regtab);
        }
    }
  else if (bridge_name)
    {
      bridge = nsm_lookup_bridge_by_name (master, bridge_name);

      if (!bridge)
        return;

      LIST_LOOP (bridge->cvlan_reg_tab_list, regtab, nn)
        nsm_cvlan_reg_show (cli, bridge, regtab);
    }
  else
    {
      for (bridge = master->bridge_list; bridge; bridge = bridge->next)
        {
          LIST_LOOP (bridge->cvlan_reg_tab_list, regtab, nn)
            nsm_cvlan_reg_show (cli, bridge, regtab);
        }
    }

}

CLI (show_nsm_cvlan_reg_tab,
     show_nsm_cvlan_reg_tab_cmd,
     "show cvlan registration table "
     "(WORD"
     "|bridge <1-32>"
     "|WORD bridge <1-32>|)",
     CLI_SHOW_STR,
     "Display CVLAN information",
     "Display CVLAN Registration Information",
     "Display CVLAN Registration table Information",
     "CVLAN Registration table name",
     "The Bridge Group to use with this CVLAN Registration table",
     "The Bridge Group number",
     "CVLAN Registration table name",
     "The Bridge Group to use with this CVLAN Registration table",
     "The Bridge Group number")
{
  struct nsm_master *nm;
  struct nsm_bridge_master *master;

  nm = cli->vr->proto;
  master = nsm_bridge_get_master (nm);

  if (! master)
    return CLI_SUCCESS;

  if (argc == 0)
    nsm_all_cvlan_reg_show (cli, master, NULL, NULL);

  else if (argc == 1)
    nsm_all_cvlan_reg_show (cli, master, argv [0], NULL);
  else if (argc == 2)
    nsm_all_cvlan_reg_show (cli, master, NULL, argv [1]);
  else if (argc == 3)
    nsm_all_cvlan_reg_show (cli, master, argv [0], argv [2]);

  return CLI_SUCCESS;
}

CLI (show_nsm_svlan_trans_tab,
     show_nsm_svlan_trans_tab_cmd,
     "show interface switchport (provider-network|customer-network|customer-edge) "
     "vlan translation interface IFNAME",
     CLI_SHOW_STR,
     "The layer2 interfaces",
     "Display the modes of the Layer2 interfaces",
     NSM_VLAN_PN_STR,
     NSM_VLAN_CN_STR,
     NSM_VLAN_CE_STR,
     "Display VLAN Translation Information",
     "Display VLAN Translation Information",
     "Display VLAN Translation table Information",
     CLI_INTERFACE_STR, CLI_IFNAME_STR)
{
  struct nsm_if *zif;
  struct interface *ifp;
  struct avl_node *node;
  struct nsm_bridge *bridge;
  struct nsm_vlan_trans_key *key;
  struct nsm_vlan_port *vlan_port;
  struct nsm_bridge_port *br_port;

  if (! (ifp = if_lookup_by_name (&cli->vr->ifm, argv[0]))
      || ! (zif = ifp->info)
      || ! (br_port = zif->switchport)
      || ! (vlan_port = &br_port->vlan_port))
    {
      cli_out (cli, "%% Interface does not exist: %s\n", argv[0]);
      return CLI_ERROR;
    }

  if (! (bridge = zif->bridge))
    {
      cli_out (cli, "%% Interface not bound to bridge: %s\n", argv[0]);
      return CLI_ERROR;
    }

  if (! NSM_BRIDGE_TYPE_PROVIDER (bridge))
    {
      cli_out (cli, "%% Interface not a bound to a provider bridge: %s\n",
               argv[0]);
      return CLI_ERROR;
    }

  if (vlan_port->mode == NSM_VLAN_PORT_MODE_CN
      || vlan_port->mode == NSM_VLAN_PORT_MODE_PN)
    cli_out (cli, "%-24s%-24s\n", "Received SVLAN ID", "Translated SVLAN ID");
  else
    cli_out (cli, "%-24s%-24s\n", "Received CVLAN ID", "Translated CVLAN ID");

  cli_out (cli, "%-24s%-24s\n", "=================", "===================");

  for (node = avl_top (br_port->trans_tab); node; node = avl_next (node))
   {
     if (! (key = AVL_NODE_INFO (node)))
       continue;

     cli_out (cli, "%-24d%-24d\n", key->vid, key->trans_vid);
   }

  cli_out (cli, "\n");

  return CLI_SUCCESS;

}

/**@brief  - Function to display the uni evc-id for uni.
 * @param cli      - Pointer to imish terminal.
 * @param bridge - nsm_bridge for which the uni evc id need to displayed.
 * @param ifp    - Interface for which the uni-evc-id need to be displayed.
 * @return       - return control to the CLI.
 * */
static void
nsm_show_uni_evc_id (struct cli *cli, struct nsm_bridge *bridge,
    struct interface *ifp)
{
  struct nsm_if *zif = NULL;
  struct nsm_vlan_port *vlan_port = NULL;
  struct nsm_bridge_port *br_port = NULL;
  struct nsm_cvlan_reg_tab *regtab = NULL;
  struct nsm_vlan *vlan = NULL;
  struct avl_node *port_node = NULL;
  struct avl_node *vlan_node = NULL;

  cli_out (cli, "\n");
  cli_out (cli, " ------------------------------------------- \n");
  cli_out (cli, " Bridge-name Interface-name UNI-ID, EVC-ID \n");
  cli_out (cli, " ----------- -------------- ---------------- \n");


  if (ifp == NULL)
    {
      AVL_TREE_LOOP (bridge->port_tree, br_port, port_node)
        {

          if ((zif = br_port->nsmif) == NULL)
            continue;

          if ((ifp = zif->ifp) == NULL)
            continue;

          vlan_port = &br_port->vlan_port;

          if (vlan_port->mode != NSM_VLAN_PORT_MODE_CE
              || ((regtab = vlan_port->regtab) == NULL))
            continue;
          if (bridge->svlan_table == NULL)
            return;

          AVL_TREE_LOOP (bridge->svlan_table, vlan, vlan_node)
            {
              if (vlan->evc_id)
                if (NSM_VLAN_BMP_IS_MEMBER (vlan_port->svlanMemberBmp,
                      vlan->vid))
                  {
                    cli_out (cli, " %s           %s            %s,%s\n",
                        bridge->name, ifp->name,
                        br_port->uni_name, 
                        vlan->evc_id);

                  }
            }

        }

    }
  else
    {
      zif = (struct nsm_if *)ifp->info;

      if (zif)
        {
          if (zif->switchport)
            br_port = zif->switchport;
          else
            return;
        }
      else
        return;

      vlan_port = &br_port->vlan_port;

      if (!vlan_port)
        return;

      if (vlan_port->mode != NSM_VLAN_PORT_MODE_CE)
        {
          cli_out (cli, " Port is not a Customer Edge Port\n");
          cli_out (cli, "\n");
          return;
        }
     if ((regtab = vlan_port->regtab) == NULL)
        {
          cli_out (cli, " Registration Table not mapped to the port\n");
          cli_out (cli, "\n");
          return;
        }

      if (bridge->svlan_table == NULL)
        return;
     
      AVL_TREE_LOOP (bridge->svlan_table, vlan, vlan_node)
        {
          if (vlan->evc_id)
            if (NSM_VLAN_BMP_IS_MEMBER (vlan_port->svlanMemberBmp, vlan->vid))
              {
                cli_out (cli, " %s           %s            %s,%s\n",
                    bridge->name, ifp->name,
                    br_port->uni_name, 
                    vlan->evc_id);
              }
        }
    }

  cli_out (cli, "\n");
  return;
}
CLI (show_nsm_uni_evc_id_cli,
     show_nsm_uni_evc_id_cmd,
     "show ethernet uni evc-id (uni-id UNI_NAME | interface IF_NAME | "
     "(bridge <1-32>|))",
     CLI_SHOW_STR,
     "ethernet service parameters",
     "user network interface",
     "evc-id of the svlan",
     "uni-id of the interface",
     "enter the uni-id of the interface",
     "The Layer2 interface",
     "enter the interface name",
     "bridge on which the uni evc-ids are associated",
     "enter the bridge for which the uni evc-ids need to be displayed")
{
  struct nsm_if *zif = NULL;
  struct interface *ifp = NULL;
  struct nsm_bridge *bridge = NULL;
  struct nsm_bridge_port *br_port = NULL;
  struct nsm_vlan_port *vlan_port = NULL;
  struct nsm_master *nm = cli->vr->proto;
  struct nsm_bridge_master *master = nsm_bridge_get_master (nm);

  if (argc == 2)
    {
      if (argv[0][0] == 'i')
        {
          if (! (ifp = if_lookup_by_name (&cli->vr->ifm, argv[1]))
              || ! (zif = ifp->info)
              || ! (br_port = zif->switchport)
              || ! (vlan_port = &br_port->vlan_port))
            {
              cli_out (cli, "%% Interface does not exist: %s\n", argv[1]);
              return CLI_ERROR;
            }
          if (! (bridge = zif->bridge))
            {
              cli_out (cli, "%% Interface not bound to bridge: %s\n", argv[1]);
              return CLI_ERROR;
            }

          if (! NSM_BRIDGE_TYPE_PROVIDER (bridge))
            {
              cli_out (cli, "%% Interface not a bound to a provider edge bridge.\n");
              return CLI_ERROR;
            }

          if (NSM_BRIDGE_TYPE_PROVIDER (bridge) && 
              (bridge->provider_edge == PAL_FALSE))
            {
              cli_out (cli, "%% Interface not a bound to a provider edge bridge.\n");
              return CLI_ERROR;
            }

          nsm_show_uni_evc_id (cli, bridge, ifp);
        }
      else if (argv[0][0] == 'u')
        {
          ifp = nsm_if_lookup_by_uni_id (&cli->vr->ifm, argv[1]);

          if (ifp == NULL)
            {
              cli_out (cli, "%% Interface does not exist: %s\n", argv[1]);
              return CLI_ERROR;
            }

          if (! (zif = ifp->info) ||
              ! (br_port = zif->switchport) ||
              ! (vlan_port = &br_port->vlan_port))
            {
              cli_out (cli, "%% Interface does not exist: %s\n", argv[1]);
              return CLI_ERROR;
            }   
          if (! (bridge = zif->bridge))
            {
              cli_out (cli, "%% Interface not bound to bridge: %s\n", argv[1]);
              return CLI_ERROR;
            }

          if (! NSM_BRIDGE_TYPE_PROVIDER (bridge))
            {
              cli_out (cli, "%% Interface not bound to a provider edge bridge.\n");
              return CLI_ERROR;
            }

          if (NSM_BRIDGE_TYPE_PROVIDER (bridge) && 
              (bridge->provider_edge == PAL_FALSE))
            {
              cli_out (cli, "%% Interface not a bound to a provider edge bridge.\n");
              return CLI_ERROR;
            }

          nsm_show_uni_evc_id (cli, bridge, ifp);

        }
    }
  
  if (argc == 2)
    {
      if (argv[0][0] == 'b')
        {
          bridge = nsm_lookup_bridge_by_name(master, argv[1]);

          if (! bridge)
            {
              cli_out (cli, "%% Bridge not found\n");
              return CLI_ERROR;
            }
          if (! NSM_BRIDGE_TYPE_PROVIDER (bridge))
            {
              cli_out (cli, "%% Bridge is not a provider edge bridge.\n");
              return CLI_ERROR;
            }
          if (NSM_BRIDGE_TYPE_PROVIDER (bridge) && 
              (bridge->provider_edge == PAL_FALSE))
            {
              cli_out (cli, "%% Bridge is not a provider edge bridge.\n");
              return CLI_ERROR;
            }

          nsm_show_uni_evc_id (cli, bridge, ifp);
        }

    }
  else
    {
      bridge = nsm_get_default_bridge (master);

      if (! bridge)
        {
          cli_out (cli, "%% Bridge not found\n");
          return CLI_ERROR;
        }
      if (! NSM_BRIDGE_TYPE_PROVIDER (bridge))
        {
          cli_out (cli, "%% Bridge is not a provider edge bridge\n");
          return CLI_ERROR;
        }
      if (NSM_BRIDGE_TYPE_PROVIDER (bridge) && 
          (bridge->provider_edge == PAL_FALSE))
        {
          cli_out (cli, "%% Bridge is not a provider edge bridge.\n");
          return CLI_ERROR;
        }

      nsm_show_uni_evc_id (cli, bridge, ifp);

    }

  return CLI_SUCCESS;
}

/**@brief  - Function to display the uni evc-id for uni.
 * @param cli    - Pointer to imish terminal.
 * @param bridge - nsm_bridge vlans for which the ce-vlan cos 
 *                   preservation need to displayed.
 * @param vlan   - vlan for which the ce-vlan cos preservation need to be
 *                   displayed.
 * @return       - return control to the CLI.
 * */
static void
nsm_show_ce_vlan_cos_preservation (struct cli *cli, struct nsm_bridge *bridge,
    struct nsm_vlan *vlan)
{
  if (bridge == NULL)
    return;
  if (vlan == NULL)
    return;


  cli_out (cli, " %-16s%-8d%-20s\n",
      bridge->name, vlan->vid, 
      ((vlan->preserve_ce_cos == PAL_TRUE) ? "Yes" : "No"));

  return;
}

CLI (nsm_show_ce_vlan_cos_preserve,
    nsm_show_ce_vlan_cos_preserve_cmd,
    "show ce-vlan cos preservation (evc EVC_ID | svlan VLAN_ID |)  "
    "(bridge <1-32>|)",
    CLI_SHOW_STR,
    "COS Preservation for Customer Edge VLAN",
    "cos",
    "Preserve Cos",
    "evc id on which the ce-vlan is preserved or not",
    "enter the evc id",
    "svlan on which the ce-vlan is preserved or not",
    "enter the svlan id",
    "bridge group on which the svlan is mapped",
    "enter the bridge id")
{
  int brid;
  s_int32_t ret = PAL_FALSE;
  u_int16_t vid = PAL_FALSE;
  struct nsm_bridge *bridge = NULL;
  struct nsm_master *nm = cli->vr->proto;
  struct nsm_bridge_master *master = nsm_bridge_get_master (nm);
  struct avl_node *rn;
  struct avl_tree *vlan_table = NULL;
  struct nsm_vlan p;
  struct nsm_vlan *vlan = NULL;

  if (argc == 4)
    {
      /* Get the brid and check if it is within range. */
      CLI_GET_INTEGER_RANGE ("Bridge group", brid, argv[3], 1, 32);

      bridge = nsm_lookup_bridge_by_name(master, argv[3]);
      if (! bridge)
        {
          cli_out (cli, "%% Bridge not found\n");
          return CLI_ERROR;
        }
      if (!NSM_BRIDGE_TYPE_PROVIDER (bridge)
          || (bridge->provider_edge == PAL_FALSE))
        {
          cli_out (cli, "%% Bridge is not a Provider Edge bridge\n");
          return CLI_ERROR;
        }

      if (argv[0][0] == 'e')
        {
          vlan = nsm_find_svlan_by_evc_id (bridge, argv[1]);

          if (vlan == NULL)
            {
              cli_out (cli, "%% EVC not found\n");
              return CLI_ERROR;
            }
        }
      else if (argv[0][0] == 's')
        {
          /* Get the vid and check if it is within range (1-4094).  */

          CLI_GET_INTEGER_RANGE ("VLAN id", vid, argv[1], NSM_VLAN_DEFAULT_VID,
              NSM_VLAN_CLI_MAX);

          if (bridge->svlan_table)
            vlan_table = bridge->svlan_table;

          if (! vlan_table)
            {
              cli_out (cli, "%% VLAN Not found\n");
              return CLI_ERROR;
            }

          NSM_VLAN_SET (&p, vid);

          rn = avl_search (vlan_table, (void *)&p);

          if (rn)
            {
              if (! rn->info)
                return NSM_VLAN_ERR_VLAN_NOT_FOUND;

              vlan = rn->info;
            }
          if (vlan == NULL)
            {
              cli_out (cli, "%% SVLAN Not Found\n");
              return CLI_ERROR;
            }
        }
      cli_out (cli, " %-16s%-8s%-20s\n", "Bridge -name", "VLAN-ID",
          "CE-VLAN Preservation");
      cli_out (cli, " =============== ======= ===================\n");

      nsm_show_ce_vlan_cos_preservation (cli, bridge, vlan);
    }
  else
    {
      if (argc == 2)
        {
          bridge = nsm_lookup_bridge_by_name(master, argv[1]);
          if (! bridge)
            {
              cli_out (cli, "%% Bridge not found\n");
              return CLI_ERROR;
            }
        }
      else
        {
          bridge = nsm_get_default_bridge (master);

          if (! bridge)
            {
              cli_out (cli, "%% Default Bridge not found\n");
              return CLI_ERROR;
            }
        }
      if (!NSM_BRIDGE_TYPE_PROVIDER (bridge)
          || (bridge->provider_edge == PAL_FALSE))
        {
          cli_out (cli, "%% Bridge is not a Provider Edge bridge\n");
          return CLI_ERROR;
        }
      cli_out (cli, " %-16s%-8s%-20s\n", "Bridge -name", "VLAN-ID",
          "CE-VLAN Preservation");
      cli_out (cli, " =============== ======= ===================\n");

      if (bridge->svlan_table != NULL)
        {
          for (rn = avl_getnext (bridge->svlan_table, NULL); rn; 
              rn = avl_getnext (bridge->svlan_table, rn))
            {
              if ((vlan = rn->info))
                {
                  nsm_show_ce_vlan_cos_preservation (cli, bridge, vlan);
                }
            }
        }
      else
        {
          cli_out (cli, "%% Svlans not mapped to the bridge.\n");
          return CLI_ERROR;
        }
    }
  if (ret < 0)
    {
      cli_out (cli, "%% Can't set COS Preservation\n");
      return CLI_ERROR;
    }

  return CLI_SUCCESS;
}

/**@brief  - Function to display the uni evc-id for uni.
 * @param cli    - Pointer to imish terminal.
 * @param bridge - nsm_bridge vlans for which the uni list need to displayed.
 * @param vlan   - vlan for which the uni list need to be displayed.
 * @return       - return control to the CLI. 
 * */
static void
nsm_show_evc_uni_list (struct cli *cli, struct nsm_bridge *bridge,
    struct nsm_vlan *vlan)
{
  struct nsm_cvlan_reg_tab *regtab = NULL;
  struct nsm_vlan_port *vlan_port = NULL;
  struct nsm_bridge_port *br_port = NULL;
  struct avl_node *avl_port = NULL;
  struct interface *ifp = NULL;
  struct nsm_if *zif = NULL;

  if (bridge == NULL)
    return;
  if (vlan == NULL)
    return;
 
  AVL_TREE_LOOP (bridge->port_tree, br_port, avl_port )
    {
      if ((zif = br_port->nsmif) == NULL)
        continue;

      if ((ifp = zif->ifp) == NULL)
        continue;

      vlan_port = &br_port->vlan_port;

      if (vlan_port->mode != NSM_VLAN_PORT_MODE_CE
          || ((regtab = vlan_port->regtab) == NULL))
        continue;

      if (NSM_VLAN_BMP_IS_MEMBER (vlan_port->svlanMemberBmp, vlan->vid))
        {
          if (br_port)
            cli_out (cli, "%% %-16s%-8d%-15s %s,Root\n",
                bridge->name, vlan->vid, ifp->name, br_port->uni_name);
        }
    }
  return;
}

CLI (nsm_show_uni_list,
    nsm_show_uni_list_cmd,
    "show ethernet uni list (svlan SVLAN_ID | evc EVC_ID |) "
    "(bridge <1-32>|)",
    CLI_SHOW_STR,
    "ethernet service parameters",
    "uni parameters",
    "list of the uni's associated to the EVC/SVLAN",
    "The svlan for which the uni list need to be displayed",
    "enter the svlan id",
    "The EVC for which the uni list need to be displayed",
    "enter the evc id",
    "bridge group on which the svlan is mapped",
    "enter the bridge id")
{
  int brid;
  u_int16_t vid = PAL_FALSE;
  struct nsm_bridge *bridge = NULL;
  struct nsm_master *nm = cli->vr->proto;
  struct nsm_bridge_master *master = nsm_bridge_get_master (nm);
  struct avl_node *rn;
  struct avl_tree *vlan_table = NULL;
  struct nsm_vlan p;
  struct nsm_vlan *vlan = NULL;

  if (argc == 4)
    {
      /* Get the brid and check if it is within range. */
      CLI_GET_INTEGER_RANGE ("Bridge group", brid, argv[3], 1, 32);

      bridge = nsm_lookup_bridge_by_name(master, argv[3]);
      if (! bridge)
        {
          cli_out (cli, "%% Bridge not found\n");
          return CLI_ERROR;
        }
      if (!NSM_BRIDGE_TYPE_PROVIDER (bridge)
          || (bridge->provider_edge == PAL_FALSE))
        {
          cli_out (cli, "%% Bridge is not a Provider Edge bridge\n");
          return CLI_ERROR;
        }

      if (argv[0][0] == 'e')
        {
          vlan = nsm_find_svlan_by_evc_id (bridge, argv[1]);

          if (vlan == NULL)
            {
              cli_out (cli, "%% EVC not found\n");
              return CLI_ERROR;
            }
        }
      else if (argv[0][0] == 's')
        {
          /* Get the vid and check if it is within range (1-4094).  */

          CLI_GET_INTEGER_RANGE ("VLAN id", vid, argv[1], NSM_VLAN_DEFAULT_VID,
              NSM_VLAN_CLI_MAX);

          if (bridge->svlan_table)
            vlan_table = bridge->svlan_table;

          if (! vlan_table)
            {
              cli_out (cli, "%% VLAN Not found\n");
              return CLI_ERROR;
            }

          NSM_VLAN_SET (&p, vid);

          rn = avl_search (vlan_table, (void *)&p);

          if (rn)
            {
              if (! rn->info)
                return NSM_VLAN_ERR_VLAN_NOT_FOUND;

              vlan = rn->info;
            }
          if (vlan == NULL)
            {
              cli_out (cli, "%% SVLAN Not Found\n");
              return CLI_ERROR;
            }
        }

      cli_out (cli, "%% %-16s%-8s%-15s%-16s\n", "Bridge-name", "VLAN-ID",
          "Interface-name", "UNI-LIST");
      cli_out (cli, "%% =============== ======= ============== "
          "================\n");
      nsm_show_evc_uni_list (cli, bridge, vlan);

    }
  else if (argc == 2)
    {
      /* Get the brid and check if it is within range. */
      CLI_GET_INTEGER_RANGE ("Bridge group", brid, argv[1], 1, 32);

      bridge = nsm_lookup_bridge_by_name(master, argv[1]);
      if (! bridge)
        {
          cli_out (cli, "%% Bridge not found\n");
          return CLI_ERROR;
        }
      if (!NSM_BRIDGE_TYPE_PROVIDER (bridge)
          || (bridge->provider_edge == PAL_FALSE))
        {
          cli_out (cli, "%% Bridge is not a Provider Edge bridge\n");
          return CLI_ERROR;
        }
      if (bridge->svlan_table != NULL)
        {
          cli_out (cli, "%% %-16s%-8s%-15s%-16s\n", "Bridge-name", "VLAN-ID",
              "Interface-name", "UNI-LIST");
          cli_out (cli, "%% =============== ======= ============== "
              "================\n");

          for (rn = avl_getnext (bridge->svlan_table, NULL); rn; 
              rn = avl_getnext (bridge->svlan_table, rn))
            {
              if ((vlan = rn->info))
                {
                  nsm_show_evc_uni_list (cli, bridge, vlan);
                }
            }
        }
      else
        {
          cli_out (cli, "%% Svlan not found.\n");
          return CLI_ERROR;
        }

    }
  else
    {
      bridge = nsm_get_default_bridge (master);

      if (! bridge)
        {
          cli_out (cli, "%% Default Bridge not found\n");
          return CLI_ERROR;
        }
      if (!NSM_BRIDGE_TYPE_PROVIDER (bridge)
          || (bridge->provider_edge == PAL_FALSE))
        {
          cli_out (cli, "%% Bridge is not a Provider Edge bridge\n");
          return CLI_ERROR;
        }
      if (bridge->svlan_table != NULL)
        {
          cli_out (cli, "%% %-16s%-8s%-15s%-16s\n", "Bridge-name", "VLAN-ID",
              "Interface-name", "UNI-LIST");
          cli_out (cli, "%% =============== ======= ============== "
              "================\n");
          for (rn = avl_getnext (bridge->svlan_table, NULL); rn; 
              rn = avl_getnext (bridge->svlan_table, rn))
            {
              if ((vlan = rn->info))
                {
                  nsm_show_evc_uni_list (cli, bridge, vlan);
                }
            }
        }
      else
        {
          cli_out (cli, "%% Svlan not found.\n");
          return CLI_ERROR;
        }

    }
  return CLI_SUCCESS;

}

/**@brief  - Function to display the uni evc-id for uni.
 * @param cli         - Pointer to imish terminal.
 * @param bridge      - nsm_bridge interfaces for which the bundling status
 *                        need to displayed.
 * @param interface   - Interface for which the bundling status need to be
 *                         displayed.
 * @return       - return control to the CLI. 
 * */
static void 
nsm_show_uni_bundling (struct cli *cli, struct nsm_bridge *bridge,
    struct interface *ifp)
{
  struct nsm_if *zif = NULL;
  struct nsm_bridge_port *br_port = NULL;
  struct nsm_vlan_port *vlan_port = NULL;
  struct avl_node *port_node = NULL;

  cli_out (cli, " Bridge-Name Interface-Name Bundling All-to-one Bundling "
      "Service-Multiplexing\n");
  cli_out (cli, " ----------- -------------- -------- ------------------- "
      "-------------------- \n");
  
  if (ifp != NULL)
    {
      zif = ifp->info;

      if (zif && zif->switchport)
        {
          br_port = zif->switchport;
        }
      else
        return;

      vlan_port = &br_port->vlan_port;

      if (!vlan_port)
        return;

      if (vlan_port->mode != NSM_VLAN_PORT_MODE_CE)
        {
          cli_out (cli, " Port is not a Customer Edge Port.\n");
          return;
        }
    
      cli_out (cli, " %s           %s            %s        %s             "
          "   %s      \n", bridge->name, ifp->name,
          (br_port->uni_ser_attr == NSM_UNI_BNDL) ? "Yes" : "No",
          (br_port->uni_ser_attr == NSM_UNI_AIO_BNDL)? "Yes" : "No",
          (br_port->uni_ser_attr == NSM_UNI_MUX_BNDL) ? "Yes" : "No");
    }
  else
    {
      AVL_TREE_LOOP (bridge->port_tree, br_port, port_node)
        {

          if ((zif = br_port->nsmif) == NULL)
            continue;

          if ((ifp = zif->ifp) == NULL)
            continue;

          vlan_port = &br_port->vlan_port;

          if (vlan_port->mode != NSM_VLAN_PORT_MODE_CE)
            continue;

          cli_out (cli, " %s           %s            %s        %s             "
              "   %s      \n", bridge->name, ifp->name,
              (br_port->uni_ser_attr == NSM_UNI_BNDL) ? "Yes" : "No",
              (br_port->uni_ser_attr == NSM_UNI_AIO_BNDL)? "Yes" : "No",
              (br_port->uni_ser_attr == NSM_UNI_MUX_BNDL) ? "Yes" : "No");

        }

    }
  return;
}
CLI (nsm_show_uni_bundling_cli,
    nsm_show_uni_bundling_cmd,
    "show ethernet uni bundling (uni-id UNI_NAME | interface IF_NAME | "
    "(bridge <1-32>|))",
    CLI_SHOW_STR,
    "ethernet service parameters",
    "uni parameters",
    "bundling status of the uni",
    "uni id on which the bundling status need to be displayed",
    "enter the uni id",
    "interface on which the bundling status need to be displayed",
    "enter the interface name",
    "brief will show the bundling status of all the interfaces",
    "bridge on which the bundling status need to be displayed",
    "enter the bridge name")
{
  struct nsm_if *zif = NULL;
  struct interface *ifp = NULL;
  struct nsm_bridge *bridge = NULL;
  struct nsm_vlan_port *vlan_port = NULL;
  struct nsm_bridge_port *br_port = NULL;
  struct nsm_master *nm = cli->vr->proto;
  struct nsm_bridge_master *master = nsm_bridge_get_master (nm);

  if (argc == 2)
    {
  if (argv[0][0] == 'i')
    {
      if (! (ifp = if_lookup_by_name (&cli->vr->ifm, argv[1]))
          || ! (zif = ifp->info)
          || ! (br_port = zif->switchport)
          || ! (vlan_port = &br_port->vlan_port))
        {
          cli_out (cli, "%% Interface does not exist: %s\n", argv[1]);
          return CLI_ERROR;
        }
      if (! (bridge = zif->bridge))
        {
          cli_out (cli, "%% Interface not bound to bridge: %s\n", argv[1]);
          return CLI_ERROR;
        }

      if (! NSM_BRIDGE_TYPE_PROVIDER (bridge))
        {
          cli_out (cli, "%% Interface not bound to a provider edge bridge: %s\n");
          return CLI_ERROR;
        }

      if (NSM_BRIDGE_TYPE_PROVIDER (bridge) && 
          (bridge->provider_edge == PAL_FALSE))
        {
          cli_out (cli, "%% Interface not bound to a provider edge bridge.\n");
          return CLI_ERROR;
        }

      if (vlan_port->mode != NSM_VLAN_PORT_MODE_CE)
        {
          cli_out (cli, "%% Interaface is not a customer edge port\n");
          return CLI_ERROR;
        }

      nsm_show_uni_bundling (cli, bridge, ifp);
    }
  else if (argv[0][0] == 'u')
    {
      ifp = nsm_if_lookup_by_uni_id (&cli->vr->ifm, argv[1]);

      if (ifp == NULL)
        {
          cli_out (cli, "%% Interface does not exist: %s\n", argv[1]);
          return CLI_ERROR;
        }

      if (! (zif = ifp->info) ||
          ! (br_port = zif->switchport) ||
          ! (vlan_port = &br_port->vlan_port))
        {
          cli_out (cli, "%% Interface does not exist: %s\n", argv[1]);
          return CLI_ERROR;
        }   
      if (! (bridge = zif->bridge))
        {
          cli_out (cli, "%% Interface not bound to bridge: %s\n", argv[1]);
          return CLI_ERROR;
        }

      if (! NSM_BRIDGE_TYPE_PROVIDER (bridge))
        {
          cli_out (cli, "%% Interface not bound to a provider edge bridge: %s\n");
          return CLI_ERROR;
        }

      if (NSM_BRIDGE_TYPE_PROVIDER (bridge) && 
          (bridge->provider_edge == PAL_FALSE))
        {
          cli_out (cli, "%% Interface not bound to a provider edge bridge.\n");
          return CLI_ERROR;
        }

      if (vlan_port->mode != NSM_VLAN_PORT_MODE_CE)
        {
          cli_out (cli, "%% Interaface is not a customer edge port\n");
          return CLI_ERROR;
        }

      nsm_show_uni_bundling (cli, bridge, ifp);
    }
    }
  
  if (argc == 2)
    {
      if (argv[0][0] == 'b')
        {
          bridge = nsm_lookup_bridge_by_name(master, argv[1]);

          if (! bridge)
            {
              cli_out (cli, "%% Bridge not found\n");
              return CLI_ERROR;
            }
          if (! NSM_BRIDGE_TYPE_PROVIDER (bridge))
            {
              cli_out (cli, "%% Bridge is not a provider edge bridge.\n");
              return CLI_ERROR;
            }
          if (NSM_BRIDGE_TYPE_PROVIDER (bridge) && 
              (bridge->provider_edge == PAL_FALSE))
            {
              cli_out (cli, "%% Bridge is not a provider edge bridge.\n");
              return CLI_ERROR;
            }

          nsm_show_uni_bundling (cli, bridge, ifp);
        }
    }
  else
    {
      bridge = nsm_get_default_bridge (master);
      if (! bridge)
        {
          cli_out (cli, "%% Bridge not found\n");
          return CLI_ERROR;
        }
      if (! NSM_BRIDGE_TYPE_PROVIDER (bridge))
        {
          cli_out (cli, "%% Bridge is not a provider edge bridge.\n");
          return CLI_ERROR;
        }
      if (NSM_BRIDGE_TYPE_PROVIDER (bridge) && 
          (bridge->provider_edge == PAL_FALSE))
        {
          cli_out (cli, "%% Bridge is not a provider edge bridge.\n");
          return CLI_ERROR;
        }

      nsm_show_uni_bundling (cli, bridge, ifp);

    }

  return CLI_SUCCESS;

}

/**@brief  - Function to display the uni evc-id for uni.
 * @param cli         - Pointer to imish terminal.
 * @param bridge      - nsm_bridge interfaces for which the bundling status
 *                        need to displayed.
 * @param interface   - Interface for which the bundling status need to be
 *                         displayed.
 * @return       - Return control to the CLI.
 * */
static void 
nsm_show_ethernet_attributes (struct cli *cli, struct nsm_bridge *bridge,
    struct interface *ifp, struct nsm_vlan *vlan)
{
  struct nsm_if *zif = NULL;
  struct nsm_bridge_port *br_port = NULL;
  struct nsm_vlan_port *vlan_port = NULL;
  struct nsm_cvlan_reg_tab *regtab = NULL;
  struct avl_node *br_port_node = NULL;

  if (ifp != NULL)
    {
      zif = ifp->info;

      if (zif && zif->switchport)
        {
          br_port = zif->switchport;
        }
      else
        return;

      vlan_port = &br_port->vlan_port;

      if (!vlan_port)
        return;

      cli_out (cli, "%% Interface Name - %s\n", ifp->name);
      cli_out (cli, "%% ---------------------\n");
      
      cli_out (cli, "%% UNI - ID : %s\n",br_port->uni_name);    

      if (vlan_port->mode != NSM_VLAN_PORT_MODE_CE
          || ((regtab = vlan_port->regtab) != NULL))
        {
          cli_out (cli, "%% Service Multiplexing - %s\n", 
            (br_port->uni_ser_attr == NSM_UNI_MUX_BNDL) ? "Yes" : "No");
        }
      if (br_port->uni_max_evcs != PAL_FALSE)
        {
          cli_out (cli, "%% MAX - EVC - %d\n", br_port->uni_max_evcs);
        }
      if (vlan_port->mode != NSM_VLAN_PORT_MODE_CE
          || ((regtab = vlan_port->regtab) != NULL))
        {
          cli_out (cli, "%% Bundling - %s\n", 
            (br_port->uni_ser_attr == NSM_UNI_BNDL) ? "Yes" : "No");
          cli_out (cli, "%% All-to-one Bundling - %s\n",
              (br_port->uni_ser_attr == NSM_UNI_AIO_BNDL)? "Yes" : "No");
        }
    }
  else if (vlan != NULL)
    {
      cli_out (cli, "%% Bridge name - %s\n", bridge->name);
      cli_out (cli, "%% -----------------\n");
      cli_out (cli, "%% EVC-ID      - %s\n", vlan->evc_id);
      cli_out (cli, "%% EVC Type    - %s\n", 
          ((vlan->type & NSM_SVLAN_P2P) ? "point-to-point" :
           (vlan->type & NSM_SVLAN_M2M) ? "multipoint-to-multipoint" :
           "not configured"));

      /* For uni list */
      cli_out (cli, "%% UNI List (UNI-ID, UNI-Type)\n");
      cli_out (cli, "%% ---------------------------\n");
      AVL_TREE_LOOP (bridge->port_tree, br_port, br_port_node)
        {
          if ((zif = br_port->nsmif) == NULL)
            continue;

          if ((ifp = zif->ifp) == NULL)
            continue;

          vlan_port = &br_port->vlan_port;

          if (vlan_port->mode != NSM_VLAN_PORT_MODE_CE
              || ((regtab = vlan_port->regtab) == NULL))
            continue;

          if (NSM_VLAN_BMP_IS_MEMBER (vlan_port->svlanMemberBmp, vlan->vid))
            {
              cli_out (cli, "%% %s, Root               \n",
                  br_port->uni_name);
            }
        }
      cli_out (cli, "%% ---------------------------\n");
      cli_out (cli, "%% MAX UNI     - %d\n", vlan->max_uni);
      cli_out (cli, "%% CE-VLAN Preservation - %s\n",
          (vlan->preserve_ce_cos == PAL_TRUE) ? "yes" : "no");
    }
  return;
}

CLI (nsm_show_ethernet_attributes_cli,
    nsm_show_ethernet_attributes_cmd,
    "show ethernet uni evc attributes (interface IF_NAME | evc-id EVC_ID "
    "(bridge <1-32>|))",
    CLI_SHOW_STR,
    "ethernet service parameters",
    "uni parameters",
    "evc attributes",
    "uni and evc attributes",
    "interface for which the uni attributes need to be displayed",
    "enter the interface name",
    "evc id for which the evc attributes need to be displayed",
    "enter the evc-id",
    "bridge group for which the ethernet service attribures need to "
    "be displayed",
    "enter the bridge id")
{
  struct nsm_if *zif = NULL;
  struct interface *ifp = NULL;
  struct nsm_bridge *bridge = NULL;
  struct nsm_vlan_port *vlan_port = NULL;
  struct nsm_vlan *vlan = NULL;
  struct nsm_bridge_port *br_port = NULL;
  struct nsm_master *nm = cli->vr->proto;
  struct nsm_bridge_master *master = nsm_bridge_get_master (nm);

  if (argv[0][0] == 'i')
    {
      if (! (ifp = if_lookup_by_name (&cli->vr->ifm, argv[1]))
          || ! (zif = ifp->info)
          || ! (br_port = zif->switchport)
          || ! (vlan_port = &br_port->vlan_port))
        {
          cli_out (cli, "%% Interface does not exist: %s\n", argv[1]);
          return CLI_ERROR;
        }
      if (! (bridge = zif->bridge))
        {
          cli_out (cli, "%% Interface not bound to bridge: %s\n", argv[1]);
          return CLI_ERROR;
        }

      if (! NSM_BRIDGE_TYPE_PROVIDER (bridge))
        {
          cli_out (cli, "%% Interface not a bound to a provider bridge: %s\n",
              argv[0]);
          return CLI_ERROR;
        }
      if (vlan_port->mode != NSM_VLAN_PORT_MODE_CE)
        {
          cli_out (cli, "%% Interaface is not a customer edge port\n");
          return CLI_ERROR;
        }

      nsm_show_ethernet_attributes (cli, bridge, ifp, vlan);
    }
  else
    {
      if (argc == 4)
        {
          bridge = nsm_lookup_bridge_by_name (master, argv[3]);
        }
      else
        {
          bridge = nsm_get_default_bridge (master);
        }
      if (bridge == NULL)
        {
          cli_out (cli, "%% Bridge not found \n");
          return CLI_ERROR;
        }

      vlan = nsm_find_svlan_by_evc_id (bridge, argv[1]);

      if (vlan == NULL)
        {
          cli_out (cli, "%% EVC not found\n");
          return CLI_ERROR;
        }
      
      nsm_show_ethernet_attributes (cli, bridge, ifp, vlan);
 
    }
  return CLI_SUCCESS;
}
#endif /* HAVE_PROVIDER_BRIDGE */

/* VLAN config write. */
int
nsm_vlan_config_write (struct cli *cli)
{
  struct apn_vr *vr = cli->vr;
  struct nsm_master *nm = vr->proto;
  struct nsm_bridge_master *master = nm->bridge;
  struct nsm_bridge *bridge;
  struct avl_node *rn;
  struct nsm_vlan *vlan;
#ifdef HAVE_PBB_TE
  struct interface *ifp;
  struct listnode *lnode;
#endif /* HAVE_PBB_TE */
  int write = 0;
  int first = 1;

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

#ifdef HAVE_PROVIDER_BRIDGE
      if (bridge->svlan_table != NULL)
        for (rn = avl_getnext (bridge->svlan_table, NULL); rn; 
             rn = avl_getnext (bridge->svlan_table, rn))
          {
            if ((vlan = rn->info))
              {
                if (vlan->preserve_ce_cos == PAL_TRUE)
                  {
                    if (bridge->is_default)
                      cli_out (cli, "ce-vlan preserve-cos %d \n", vlan->vid);
                    else
                      cli_out (cli, "ce-vlan preserve-cos %d bridge %s\n",
                               vlan->vid, bridge->name);
                  }
              }
         }
#endif /* HAVE_PROVIDER_BRIDGE */

      if (bridge->vlan_table != NULL)
        for (rn = avl_getnext (bridge->vlan_table, NULL); rn;
             rn = avl_getnext (bridge->vlan_table, rn))
           {
             if ((vlan = rn->info) && (vlan->vid != NSM_VLAN_DEFAULT_VID)
                 && ((vlan->type & NSM_VLAN_STATIC) || 
                     (vlan->type & NSM_VLAN_AUTO)))
               {
                 if (first)
                   {
                     cli_out (cli, "!\n");
                     cli_out (cli, "vlan database\n");
                     write++;
                     first = 0;
                   }

#ifdef HAVE_PROVIDER_BRIDGE
                 if (NSM_BRIDGE_TYPE_PROVIDER (bridge))
                   {
                     cli_out (cli, " vlan %d type customer %s name %s\n",
                              vlan->vid, nsm_get_bridge_str (bridge),
                              vlan->vlan_name);
                     write++;

                     if (vlan->vlan_state == NSM_VLAN_DISABLED)
                       {
                         cli_out (cli, " vlan %d type customer %s state "
                                  "disable\n", vlan->vid,
                                  nsm_get_bridge_str (bridge));
                         write++;
                       }
                     else
                       {
                         cli_out (cli, " vlan %d type customer %s state "
                                  "enable\n", vlan->vid,
                                  nsm_get_bridge_str (bridge));
                         write++;
                       }

                     if (vlan->mtu_val > 0)
                       {
                         cli_out (cli, " vlan %d type customer mtu %d %s\n",
                                  vlan->vid, vlan->mtu_val,
                                  nsm_get_bridge_str(bridge));
                         write++;
                       }
                   }
                 else
                   {
                     cli_out (cli, " vlan %d %s name %s\n",
                              vlan->vid, nsm_get_bridge_str (bridge),
                              vlan->vlan_name);
                     write++;

                     if (vlan->vlan_state == NSM_VLAN_DISABLED)
                       {
                         cli_out (cli, " vlan %d %s state disable\n",
                                  vlan->vid, nsm_get_bridge_str (bridge));
                         write++;
                       }
                     else
                       {
                         cli_out (cli, " vlan %d %s state enable\n",
                                  vlan->vid, nsm_get_bridge_str (bridge));
                         write++;
                       }

                     if (vlan->mtu_val > 0)
                       {
                         cli_out (cli, " vlan %d mtu %d %s\n",
                                  vlan->vid, vlan->mtu_val,
                                  nsm_get_bridge_str (bridge));
                         write++;
                       }
                   }
#else
                 cli_out (cli, " vlan %d %s name %s\n",
                          vlan->vid, nsm_get_bridge_str (bridge), vlan->vlan_name);

                 write++;

                 if (vlan->vlan_state == NSM_VLAN_DISABLED)
                   {
                     cli_out (cli, " vlan %d %s state disable\n",
                              vlan->vid, nsm_get_bridge_str (bridge));
                     write++;
                   }
                 else
                   {
                     cli_out (cli, " vlan %d %s state enable\n",
                              vlan->vid, nsm_get_bridge_str (bridge));
                      write++;
                   }
                 if (vlan->mtu_val > 0)
                   {
                     cli_out (cli, " vlan %d mtu %d %s\n",
                              vlan->vid, vlan->mtu_val,
                              nsm_get_bridge_str (bridge));
                     write++;
                   }
#endif /* HAVE_PROVIDER_BRIDGE */

               }
           }

      if (bridge->svlan_table != NULL)
        for (rn = avl_getnext (bridge->svlan_table, NULL); rn;
             rn = avl_getnext (bridge->svlan_table, rn))
          if ((vlan = rn->info) && (vlan->vid != NSM_VLAN_DEFAULT_VID)
              && (vlan->type & NSM_VLAN_STATIC))
            {
              if (first)
                {
                  cli_out (cli, "!\n");
                  cli_out (cli, "vlan database\n");
                  write++;
                  first = 0;
                }

#if defined (HAVE_PROVIDER_BRIDGE) || defined (HAVE_I_BEB)

              cli_out (cli, " vlan %d type service %s %s name %s\n",
                            vlan->vid,
                            CHECK_FLAG (vlan->type, NSM_SVLAN_P2P) ?
                            "point-point" : "multipoint-multipoint",
                            nsm_get_bridge_str (bridge),
                            vlan->vlan_name);
              write++;

              if (vlan->vlan_state == NSM_VLAN_DISABLED)
                {
                 cli_out (cli, " vlan %d type service %s %s state disable\n",
                                 vlan->vid,
                                 CHECK_FLAG (vlan->type, NSM_SVLAN_P2P) ?
                                 "point-point" : "multipoint-multipoint",
                                 nsm_get_bridge_str (bridge));
                 write++;
               }
             else
               {
                 cli_out (cli, " vlan %d type service %s %s state enable\n",
                                 vlan->vid,
                                 CHECK_FLAG (vlan->type, NSM_SVLAN_P2P) ?
                                 "point-point" : "multipoint-multipoint",
                                 nsm_get_bridge_str (bridge));
                 write++;
               }
             if (vlan->evc_id != NULL)
               {
                 cli_out (cli, " ethernet svlan %d evc-id %s %s\n",
                     vlan->vid, vlan->evc_id, nsm_get_bridge_str (bridge));

                 write++;
               }
#ifdef HAVE_PROVIDER_BRIDGE
             if (vlan->max_uni != PAL_FALSE)
               {
                 cli_out (cli, " ethernet svlan %d max-uni %d %s\n",
                     vlan->vid, vlan->max_uni, nsm_get_bridge_str (bridge));
                 write++;
               }
#endif /* HAVE_PROVIDER_BRIDGE */
             if (vlan->mtu_val > 0)
               {
                 cli_out (cli, " vlan %d type service mtu %d %s\n",
                                 vlan->vid, vlan->mtu_val,
                                 nsm_get_bridge_str (bridge));
                 write++;
               }
#ifdef HAVE_PBB_TE
            if (LISTCOUNT (vlan->unicast_egress_ports))
              LIST_LOOP (vlan->unicast_egress_ports, ifp, lnode)
                {
                  cli_out (cli, " vlan %d bridge %s allowed add interface %s\n",
                                 vlan->vid, bridge->name, ifp->name );
                  write++;
                }
            if (LISTCOUNT (vlan->unicast_forbidden_ports))
              LIST_LOOP (vlan->unicast_forbidden_ports, ifp, lnode)
                {
                  cli_out (cli, " vlan %d bridge %s forbidden add interface %s\n",
                                 vlan->vid, bridge->name, ifp->name );
                  write++;
                }
            if (LISTCOUNT (vlan->multicast_egress_ports))
              LIST_LOOP (vlan->multicast_egress_ports, ifp, lnode)
                {
                  cli_out (cli, " vlan %d bridge %s allowed multicast add interface %s\n",
                                 vlan->vid, bridge->name, ifp->name );
                  write++;
                }
            if (LISTCOUNT (vlan->multicast_forbidden_ports))
              LIST_LOOP (vlan->multicast_forbidden_ports, ifp, lnode)
                {
                  cli_out (cli, " vlan %d bridge %s forbidden multicast add interface %s\n",
                                 vlan->vid, bridge->name, ifp->name );
                  write++;
                }
#endif /* HAVE_PBB_TE */
#endif /* HAVE_PROVIDER_BRIDGE */
            }
      write++;
      bridge = bridge->next;
    }
#ifdef HAVE_B_BEB
  bridge = master->b_bridge;
  if(bridge)
  {
  if (bridge->bvlan_table != NULL)
    for (rn = avl_top (bridge->bvlan_table); rn; rn = avl_next (rn))
      if ((vlan = rn->info) && (vlan->vid != NSM_VLAN_DEFAULT_VID)
          && (vlan->type & NSM_VLAN_STATIC))
      {
        if (first)
        {
          cli_out (cli, "!\n");
          cli_out (cli, "vlan database\n");
          write++;
          first = 0;
        }
        cli_out (cli, " vlan %d type backbone %s name %s\n",
                      vlan->vid,
                      CHECK_FLAG (vlan->type, NSM_BVLAN_P2P) ?
                      "point-point" : "multipoint-multipoint",
                      vlan->vlan_name);
        write++;
                    
        if (vlan->vlan_state == NSM_VLAN_DISABLED)
        {
          
          cli_out (cli, " vlan %d type backbone %s state disable\n",
                        vlan->vid,
                        CHECK_FLAG (vlan->type, NSM_BVLAN_P2P) ?
                        "point-point" : "multipoint-multipoint");
          write++;
          
        }

        else
        {
          cli_out (cli, " vlan %d type backbone %s state enable\n",
                        vlan->vid,
                        CHECK_FLAG (vlan->type, NSM_BVLAN_P2P) ?
                        "point-point" : "multipoint-multipoint");
          write++;
        }
#ifdef HAVE_PBB_TE
      if (LISTCOUNT (vlan->unicast_egress_ports))
        LIST_LOOP (vlan->unicast_egress_ports, ifp, lnode)
          {
            cli_out (cli, " vlan %d bridge %s allowed add interface %s\n",
                           vlan->vid, bridge->name, ifp->name );
            write++;
          }
      if (LISTCOUNT (vlan->unicast_forbidden_ports))
        LIST_LOOP (vlan->unicast_forbidden_ports, ifp, lnode)
          {
            cli_out (cli, " vlan %d bridge %s forbidden add interface %s\n",
                           vlan->vid, bridge->name, ifp->name );
            write++;
          }
      if (LISTCOUNT (vlan->multicast_egress_ports))
        LIST_LOOP (vlan->multicast_egress_ports, ifp, lnode)
          {
            cli_out (cli, " vlan %d bridge %s allowed multicast add interface %s\n",
                           vlan->vid, bridge->name, ifp->name );
            write++;
          }
      if (LISTCOUNT (vlan->multicast_forbidden_ports))
        LIST_LOOP (vlan->multicast_forbidden_ports, ifp, lnode)
          {
            cli_out (cli, " vlan %d bridge %s forbidden multicast add interface %s\n",
                           vlan->vid, bridge->name, ifp->name );
            write++;
          }
#endif /* HAVE_PBB_TE */
     }
  }
#endif /* HAVE_B_BEB */

#ifdef HAVE_PVLAN
  write += nsm_pvlan_config_write (cli);
#endif /* HAVE_PVLAN */
  cli_out (cli, "!\n");

#ifdef HAVE_PBB_TE
  write += nsm_pbb_te_config_write (cli);
  cli_out (cli, "!\n");
#endif /* HAVE_PBB_TE */

  return write;
}

#ifdef HAVE_G8031
int
nsm_g8031_vlan_config_write (struct cli *cli)
{
  struct apn_vr *vr = cli->vr;
  struct nsm_master *nm = vr->proto;
  struct nsm_bridge_master *master = nm->bridge;
  struct nsm_bridge *bridge;
  struct g8031_protection_group *pg = NULL;
  struct avl_node *pg_node = NULL;
  struct nsm_vlan *vlan;
  struct avl_node *eps_vid_node = NULL;
  int write = 0;

  if (! master)
    return 0;

  bridge = master->bridge_list;
  if (! bridge)
    return 0;

  while (bridge)
    {
      if (bridge->eps_tree != NULL)
        {
          AVL_TREE_LOOP (bridge->eps_tree, pg, pg_node)
            {
              cli_out (cli, "!\n");

              cli_out (cli, "g8031 configure vlan eps-id %d bridge %s \n",
                             pg->eps_id, bridge->name);

              AVL_TREE_LOOP (pg->eps_vids, vlan, eps_vid_node )
                {
                  if (vlan->vid && pg->primary_vid == vlan->vid)
                     cli_out (cli, " g8031 vlan %d primary \n", pg->primary_vid);

                  if (vlan->vid && pg->primary_vid != vlan->vid)
                      cli_out (cli, " g8031 vlan %d\n", vlan->vid);
                }
              write++;
            }
        }
      bridge = bridge->next;
    }
#ifdef HAVE_B_BEB
  bridge = master->b_bridge;
  if (bridge)
    {
      if (bridge->eps_tree != NULL)
        AVL_TREE_LOOP (bridge->eps_tree, pg, pg_node)
          {
            AVL_TREE_LOOP (pg->eps_vids, vlan, eps_vid_node )
              {
                cli_out (cli, "g8031 configure vlan eps-id %d %s%s\n",
                    pg->eps_id,(bridge->name[0]=='b')?"" : "bridge ", bridge->name);
                write++;

                if (pg->primary_vid)
                  {
                    cli_out (cli, " g8031 vlan %d primary\n", vlan->vid);
                    write++;
                  }

                if (vlan->vid && pg->primary_vid != vlan->vid)
                  {
                    cli_out (cli, " g8031 vlan %d \n", vlan->vid);
                    write++;
                  }
              }
          }
    }
#endif /* defined (HAVE_B_BEB) */

  return write;
}
#endif /* HAVE_G8031 */
#ifdef HAVE_G8032
int
nsm_g8032_vlan_config_write (struct cli *cli)
{
  struct apn_vr *vr = cli->vr;
  struct nsm_master *nm = vr->proto;
  struct nsm_bridge_master *master = nm->bridge;
  struct nsm_bridge *bridge;
  struct nsm_g8032_ring *ring = NULL;
  struct avl_node *node = NULL;
  struct nsm_vlan *vlan;
  struct avl_node *raps_vid_node = NULL;
  int write = 0;

  if (! master)
    return 0;

  bridge = master->bridge_list;
  if (! bridge)
    return 0;

  while (bridge)
    {
      if (bridge->raps_tree != NULL)
        AVL_TREE_LOOP (bridge->raps_tree, ring, node)
          {
            AVL_TREE_LOOP (ring->g8032_vids, vlan, raps_vid_node )
              {
                cli_out (cli, "!\n");
                cli_out (cli, "g8032 configure vlan ring-id %d bridge %s\n",
                    ring->g8032_ring_id, bridge->name);
                write++  ;

                if (ring->primary_vid == vlan->vid)
                  {
                    cli_out (cli, " g8032 vlan %d primary\n", vlan->vid);
                    write++  ;
                  }

                if (vlan->vid && ring->primary_vid!= vlan->vid)
                  {
                    cli_out (cli, " g8032 vlan %d\n", vlan->vid);
                    write++  ;
                  }
              }
          }
      write++  ;
      bridge = bridge->next;
    }
#ifdef HAVE_B_BEB
  bridge = master->b_bridge;
  if (bridge)
    {
      if (bridge->raps_tree != NULL)
        AVL_TREE_LOOP (bridge->raps_tree, ring, node)
          {
            AVL_TREE_LOOP (ring->g8032_vids, vlan, raps_vid_node)
              {
                cli_out (cli, "g8032 configure vlan ring-id %d %s%s\n",
                    ring->g8032_ring_id,(bridge->name[0]=='b')?"" : "bridge ", bridge->name);
                write++ ;

                if (ring->primary_vid)
                  {
                    cli_out (cli, " g8032 vlan %d primary\n", vlan->vid);
                    write++  ;
                  }

                if (vlan->vid && ring->primary_vid != vlan->vid)
                  {
                    cli_out (cli, " g8032 vlan %d \n", vlan->vid);
                    write++ ;
                  }
              }
          }
    }
#endif /* defined (HAVE_B_BEB) */

  return write;
}
#endif /* HAVE_G8032 */

char *
nsm_vlan_if_mode_to_str (enum nsm_vlan_port_mode mode)
{

  switch (mode)
    {
      case NSM_VLAN_PORT_MODE_INVALID:
        return "Invalid";
        break;
      case NSM_VLAN_PORT_MODE_ACCESS:
        return "access";
        break;
      case NSM_VLAN_PORT_MODE_HYBRID:
        return "hybrid";
        break;
      case NSM_VLAN_PORT_MODE_TRUNK:
        return "trunk";
        break;
      case NSM_VLAN_PORT_MODE_CE:
        return "customer-edge";
        break;
      case NSM_VLAN_PORT_MODE_CN:
        return "customer-network";
        break;
      case NSM_VLAN_PORT_MODE_PN:
        return "provider-network";
        break;
      case NSM_VLAN_PORT_MODE_CNP:
        return "cnp";
      case NSM_VLAN_PORT_MODE_PIP:
        return "pip";
      case NSM_VLAN_PORT_MODE_CBP:
        return "cbp";
      default:
        break;
    }

  return "Invaid";

}

/* VLAN if config write. */
int
nsm_vlan_if_config_write (struct cli *cli, struct interface *ifp)
{
  nsm_vid_t vid;
  struct nsm_if *zif;
  struct nsm_vlan *vlan;
  struct avl_node *rn;
  struct nsm_vlan_port *port;
  struct nsm_bridge_port *br_port;
#ifdef HAVE_QOS
  unsigned int user_priority, num_traffic_classes;
#endif /* HAVE_QOS */
#ifdef HAVE_PROVIDER_BRIDGE
  struct avl_node *node;
  struct nsm_vlan_trans_key *key;
  struct nsm_cvlan_reg_key *key1 = NULL;
  struct nsm_svlan_reg_info *svlan_info = NULL;
  struct nsm_pro_edge_info *pro_edge_port = NULL;
  struct listnode *ls_node = NULL;
#endif /* HAVE_PROVIDER_BRIDGE */

  zif = (struct nsm_if *)ifp->info;
  if (! zif)
    return -1;

  br_port = zif->switchport;

  if (! br_port)
    return -1;

  if (zif->bridge && zif->switchport)
    {
      br_port = zif->switchport;
      port = &(br_port->vlan_port);
      switch (port->mode)
        {
        case NSM_VLAN_PORT_MODE_ACCESS:
          {
            cli_out (cli, " switchport mode access\n");
            if (!(port->flags & NSM_VLAN_ENABLE_INGRESS_FILTER))
              cli_out (cli, " switchport mode access ingress-filter disable\n");
            if (port->pvid != NSM_VLAN_DEFAULT_VID)
              cli_out (cli, " switchport access vlan %d\n", port->pvid);
          }
          break;

        case NSM_VLAN_PORT_MODE_HYBRID:
          {
            cli_out (cli, " switchport mode hybrid\n");

            if (!(port->flags & NSM_VLAN_ENABLE_INGRESS_FILTER))
              cli_out (cli, " switchport mode hybrid ingress-filter disable\n");
 
            if (port->pvid != NSM_VLAN_DEFAULT_VID)
              cli_out (cli, " switchport hybrid vlan %d\n", port->pvid);

            if (port->flags & NSM_VLAN_ACCEPTABLE_FRAME_TYPE_TAGGED)
              cli_out (cli, " switchport mode hybrid acceptable-frame-type vlan-tagged\n");
            else
              cli_out (cli, " switchport mode hybrid acceptable-frame-type all\n");

            if (port->config == NSM_VLAN_CONFIGURED_ALL)
              {
                cli_out (cli, " switchport hybrid allowed vlan all\n");

                for (rn = avl_top (zif->bridge->vlan_table); rn;
                                                               rn = avl_next (rn))
                  if ((vlan = rn->info) && (vlan->vid != NSM_VLAN_DEFAULT_VID))
                    {
                       if (!((NSM_VLAN_BMP_IS_MEMBER (port->staticMemberBmp,
                                                      vlan->vid))
                             || (NSM_VLAN_BMP_IS_MEMBER (port->dynamicMemberBmp,
                                                        vlan->vid))))
                         cli_out (cli, " switchport hybrid allowed vlan remove %d\n",
                                  vlan->vid);
                    }
              }
            else if (port->config == NSM_VLAN_CONFIGURED_NONE)
              {
                cli_out (cli, " switchport hybrid allowed vlan none\n");

                for (rn = avl_top (zif->bridge->vlan_table); rn; rn = avl_next (rn))
                  if ((vlan = rn->info) && (vlan->vid != NSM_VLAN_DEFAULT_VID))
                    {
                      if (NSM_VLAN_BMP_IS_MEMBER (port->staticMemberBmp,
                                                  vlan->vid))
                        {
                          if ( NSM_VLAN_BMP_IS_MEMBER(port->egresstaggedBmp,
                                                      vlan->vid))
                            cli_out (cli, " switchport hybrid allowed vlan add %d "
                                     "egress-tagged enable\n", vlan->vid);
                          else
                            cli_out (cli, " switchport hybrid allowed vlan add %d "
                                     "egress-tagged disable\n", vlan->vid);
                        }
                    }
              }
            else
              {
               NSM_VLAN_SET_BMP_ITER_BEGIN(port->staticMemberBmp, vid)
                 {
                     if (vid == NSM_VLAN_DEFAULT_VID)
                       continue;
 
                   if ( NSM_VLAN_BMP_IS_MEMBER(port->egresstaggedBmp, vid))
                     cli_out (cli, " switchport hybrid allowed vlan add %d "
                              "egress-tagged enable\n", vid);
                   else
                     cli_out (cli, " switchport hybrid allowed vlan add %d "
                              "egress-tagged disable\n", vid);
                }
              NSM_VLAN_SET_BMP_ITER_END(port->staticMemberBmp, vid);
             }
         }
         break;

        case NSM_VLAN_PORT_MODE_TRUNK:
        case NSM_VLAN_PORT_MODE_PN:
#ifdef HAVE_B_BEB
           {
             if (zif->bridge->type == NSM_BRIDGE_TYPE_BACKBONE_MSTP ||
                 zif->bridge->type == NSM_BRIDGE_TYPE_BACKBONE_RSTP)
               {
                 cli_out (cli, " switchport mode %s \n",
                     "pnp");
                 if (port->pvid && port->pvid != NSM_VLAN_DEFAULT_VID)
                   cli_out (cli, " switchport beb vlan %d %s \n",
                       port->pvid, "pnp");
                 break;
               }
           }
#endif /* HAVE_B_BEB */
        case NSM_VLAN_PORT_MODE_CN:
          {
            cli_out (cli, " switchport mode %s\n",
                     nsm_vlan_if_mode_to_str (port->mode));
 
            if (!(port->flags & NSM_VLAN_ENABLE_INGRESS_FILTER))
              cli_out (cli, " switchport mode %s ingress-filter disable\n",
                       nsm_vlan_if_mode_to_str (port->mode));

            if (port->pvid != NSM_VLAN_DEFAULT_VID)
              cli_out (cli, " switchport %s vlan %d\n",
                       nsm_vlan_if_mode_to_str (port->mode), port->pvid);

            if (port->config == NSM_VLAN_CONFIGURED_ALL)
              {
                cli_out (cli, " switchport %s allowed vlan all\n",
                         nsm_vlan_if_mode_to_str (port->mode));

                for (rn = avl_top (zif->bridge->vlan_table); rn;
                     rn = avl_next (rn))
                  if ((vlan = rn->info) && (vlan->vid != NSM_VLAN_DEFAULT_VID))
                    {
                      if (!((NSM_VLAN_BMP_IS_MEMBER (port->staticMemberBmp,
                                                     vlan->vid))
                            || (NSM_VLAN_BMP_IS_MEMBER (port->dynamicMemberBmp,
                                                        vlan->vid))))
                        cli_out (cli, " switchport %s allowed vlan remove %d\n",
                                 nsm_vlan_if_mode_to_str (port->mode), vlan->vid);
                    }
              }
            else if (port->config == NSM_VLAN_CONFIGURED_NONE)
              {
                cli_out (cli, " switchport %s allowed vlan none\n",
                         nsm_vlan_if_mode_to_str (port->mode));

                for (rn = avl_top (zif->bridge->vlan_table); rn; rn = avl_next (rn))
                  if ((vlan = rn->info) && (vlan->vid != NSM_VLAN_DEFAULT_VID))
                    {
                      if (NSM_VLAN_BMP_IS_MEMBER (port->staticMemberBmp, vlan->vid))
                        cli_out (cli, "switchport %s allowed vlan add %d\n",
                                 nsm_vlan_if_mode_to_str (port->mode),
                                 vlan->vid);
                    }
              }
            else
              {
                if (port->mode == NSM_VLAN_PORT_MODE_TRUNK
                    && ! (NSM_VLAN_BMP_IS_MEMBER (port->staticMemberBmp,
                                                  NSM_VLAN_DEFAULT_VID)))
                  { 
                    cli_out (cli, " switchport trunk allowed vlan remove 1\n");
                  }
              
                NSM_VLAN_SET_BMP_ITER_BEGIN(port->staticMemberBmp, vid)
                  {
                    if (vid == NSM_VLAN_DEFAULT_VID
                        || vid == port->pvid)
                      continue;

                    cli_out (cli, " switchport %s allowed vlan add %d\n",
                             nsm_vlan_if_mode_to_str (port->mode), vid);
                  }
                NSM_VLAN_SET_BMP_ITER_END(port->membetBmp, vid);
              }

            if ( port->mode == NSM_VLAN_PORT_MODE_TRUNK
                 && port->native_vid != NSM_VLAN_DEFAULT_VID )
              {
                cli_out (cli, " switchport trunk native vlan %d\n", port->native_vid);
              }
          }
          break;

        case NSM_VLAN_PORT_MODE_CE:
          {
            cli_out (cli, " switchport mode %s %s\n",
                     nsm_vlan_if_mode_to_str (port->mode),
                     nsm_vlan_if_mode_to_str (port->sub_mode));

            if (!(port->flags & NSM_VLAN_ENABLE_INGRESS_FILTER))
              cli_out (cli, " switchport mode %s %s ingress-filter disable\n",
                       nsm_vlan_if_mode_to_str (port->mode),
                       nsm_vlan_if_mode_to_str (port->sub_mode));

            if (port->pvid != NSM_VLAN_DEFAULT_VID)
              cli_out (cli, " switchport %s %s vlan %d\n",
                       nsm_vlan_if_mode_to_str (port->mode),
                       nsm_vlan_if_mode_to_str (port->sub_mode), port->pvid);

            if (port->sub_mode == NSM_VLAN_PORT_MODE_ACCESS)
              break;

            if (port->sub_mode == NSM_VLAN_PORT_MODE_HYBRID)
              {
                if (port->flags & NSM_VLAN_ACCEPTABLE_FRAME_TYPE_TAGGED)
                  cli_out (cli, " switchport mode %s hybrid acceptable-frame-type "
                                "vlan-tagged\n",
                           nsm_vlan_if_mode_to_str (port->mode));
                else
                  cli_out (cli, " switchport mode %s hybrid acceptable-frame-type all\n",
                           nsm_vlan_if_mode_to_str (port->mode));
              }

            if (port->config == NSM_VLAN_CONFIGURED_ALL)
              {
                cli_out (cli, " switchport %s %s allowed vlan all\n",
                         nsm_vlan_if_mode_to_str (port->mode),
                         nsm_vlan_if_mode_to_str (port->sub_mode));

                for (rn = avl_top (zif->bridge->vlan_table); rn;
                                                  rn = avl_next (rn))
                  if ((vlan = rn->info) && (vlan->vid != NSM_VLAN_DEFAULT_VID))
                    {
                      if (!((NSM_VLAN_BMP_IS_MEMBER (port->staticMemberBmp,
                                                     vlan->vid))
                            || (NSM_VLAN_BMP_IS_MEMBER (port->dynamicMemberBmp,
                                                        vlan->vid))))
                        cli_out (cli, " switchport %s %s allowed vlan remove %d\n",
                                 nsm_vlan_if_mode_to_str (port->mode),
                                 nsm_vlan_if_mode_to_str (port->sub_mode),
                                 vlan->vid);
                    }
#ifdef HAVE_PROVIDER_BRIDGE
                if (port->uni_default_evc != NSM_DEFAULT_VAL)
                  cli_out (cli, " switchport %s %s default-svlan %d\n",
                           nsm_vlan_if_mode_to_str (port->mode), 
                           nsm_vlan_if_mode_to_str(port->sub_mode), 
                           port->uni_default_evc);
#endif /* HAVE_PROVIDER_BRIDGE */
              }
            else if (port->config == NSM_VLAN_CONFIGURED_NONE)
              {
                cli_out (cli, " switchport %s %s allowed vlan none\n",
                         nsm_vlan_if_mode_to_str (port->mode),
                         nsm_vlan_if_mode_to_str (port->sub_mode));

                for (rn = avl_top (zif->bridge->vlan_table); rn; rn = avl_next (rn))
                  if ((vlan = rn->info) && (vlan->vid != NSM_VLAN_DEFAULT_VID))
                    {
                      if (port->sub_mode == NSM_VLAN_PORT_MODE_HYBRID
                          && NSM_VLAN_BMP_IS_MEMBER (port->staticMemberBmp,
                                                     vlan->vid))
                        {
                          if ( NSM_VLAN_BMP_IS_MEMBER(port->egresstaggedBmp,
                                                      vlan->vid))
                            cli_out (cli, " switchport %s hybrid allowed vlan add %d "
                                     "egress-tagged enable\n",
                                     nsm_vlan_if_mode_to_str (port->mode),
                                     vlan->vid);
                          else
                            cli_out (cli, " switchport %s hybrid allowed vlan add %d "
                                     "egress-tagged disable\n",
                                     nsm_vlan_if_mode_to_str (port->mode),
                                     vlan->vid);
                        }
                      else if (port->sub_mode == NSM_VLAN_PORT_MODE_TRUNK
                               && NSM_VLAN_BMP_IS_MEMBER (port->staticMemberBmp,
                                                          vlan->vid))
                        {
                          cli_out (cli, " switchport %s trunk allowed vlan add %d\n",
                                   nsm_vlan_if_mode_to_str (port->mode),
                                   vlan->vid);
                        }
                    }
              }
            else
              {
               if (port->sub_mode == NSM_VLAN_PORT_MODE_HYBRID)
                 {
                   NSM_VLAN_SET_BMP_ITER_BEGIN(port->staticMemberBmp, vid)
                     {
                       if (vid == NSM_VLAN_DEFAULT_VID)
                         continue;

                       if ( NSM_VLAN_BMP_IS_MEMBER(port->egresstaggedBmp, vid))
                         cli_out (cli, " switchport %s hybrid allowed vlan add %d "
                                  "egress-tagged enable\n",
                                  nsm_vlan_if_mode_to_str (port->mode), vid);
                       else
                         cli_out (cli, " switchport %s hybrid allowed vlan add %d "
                                  "egress-tagged disable\n",
                                  nsm_vlan_if_mode_to_str (port->mode), vid);
                     }
                   NSM_VLAN_SET_BMP_ITER_END(port->staticMemberBmp, vid);
                 }
               else if (port->sub_mode == NSM_VLAN_PORT_MODE_TRUNK)
                 {
                   NSM_VLAN_SET_BMP_ITER_BEGIN(port->staticMemberBmp, vid)
                     {
                       if (vid == NSM_VLAN_DEFAULT_VID)
                         continue;

                       cli_out (cli, " switchport %s trunk allowed vlan add %d\n",
                                nsm_vlan_if_mode_to_str (port->mode), vid);
                     }
                   NSM_VLAN_SET_BMP_ITER_END(port->staticMemberBmp, vid);
                 }

#ifdef HAVE_PROVIDER_BRIDGE
               if (port->uni_default_evc != NSM_DEFAULT_VAL)
                 cli_out (cli, " switchport %s %s default-svlan %d\n",
                          nsm_vlan_if_mode_to_str (port->mode), 
                          nsm_vlan_if_mode_to_str(port->sub_mode), 
                          port->uni_default_evc);
#endif /* HAVE_PROVIDER_BRIDGE */
              }
          }
          break;
#if defined (HAVE_I_BEB) || defined (HAVE_B_BEB)
        case NSM_VLAN_PORT_MODE_CNP:
        case NSM_VLAN_PORT_MODE_PIP:
        case NSM_VLAN_PORT_MODE_CBP:
        //case NSM_VLAN_PORT_MODE_PNP:          
          {

            if (ifp->hw_type != IF_TYPE_PBB_LOGICAL)
              cli_out (cli, " switchport mode %s \n",
                       nsm_vlan_if_mode_to_str (port->mode));

            if (port->pvid && port->pvid != NSM_VLAN_DEFAULT_VID)
              cli_out (cli, " switchport beb vlan %d %s \n",
                       port->pvid,
                       nsm_vlan_if_mode_to_str (port->mode));

          } 
        break;
#endif
        default:
          break;
        }

#ifdef HAVE_PROVIDER_BRIDGE

      if (port->regtab)
        cli_out (cli, " switchport customer-edge vlan registration %s\n",
                 port->regtab->name);

     if (port->mode == NSM_VLAN_PORT_MODE_CN)
       {
         for (node = avl_top (br_port->trans_tab); node; node = avl_next (node))
            {
              if (! (key = AVL_NODE_INFO (node)))
                continue;

              cli_out (cli, " switchport customer-network vlan translation svlan "
                       "%d svlan %d\n", key->vid, key->trans_vid);
            }
        }
#ifdef HAVE_B_BEB
     if ((port->mode == NSM_VLAN_PORT_MODE_PN) 
         && ((zif->bridge->type != NSM_BRIDGE_TYPE_BACKBONE_MSTP)
           && (zif->bridge->type != NSM_BRIDGE_TYPE_BACKBONE_RSTP) ))
#else
     if (port->mode == NSM_VLAN_PORT_MODE_PN)
#endif /* HAVE_B_BEB */
       {
         for (node = avl_top (br_port->trans_tab); node; node = avl_next (node))
            {
              if (! (key = AVL_NODE_INFO (node)))
                continue;

              cli_out (cli, " switchport provider-network vlan translation svlan "
                       "%d svlan %d\n", key->vid, key->trans_vid);
            }
        }

     if (port->mode == NSM_VLAN_PORT_MODE_CE)
       {
         for (node = avl_top (br_port->trans_tab); node; node = avl_next (node))
            {
              if (! (key = AVL_NODE_INFO (node)))
                continue;

              cli_out (cli, " switchport customer-edge vlan translation vlan "
                       "%d vlan %d\n", key->vid, key->trans_vid);
            }
         if (port->regtab != NULL)
           {

             AVL_TREE_LOOP (port->regtab->reg_tab, key1, node)
               {
                 if ((svlan_info = nsm_reg_tab_svlan_info_lookup (port->regtab,
                         key1->svid)) == NULL)
                   continue;

                 LIST_LOOP (svlan_info->port_list, pro_edge_port, ls_node)
                   {
                     if (pro_edge_port->untagged_vid != NSM_VLAN_DEFAULT_VID)
                       {
                         cli_out (cli, " switchport provider-edge vlan %d "
                             "untagged-vlan %d\n", svlan_info->svid, 
                             pro_edge_port->untagged_vid);
                       }/* if (pro_edge_port->untagged_vid != NSM_VLAN_DEFAULT_VID) */
                     
                     if (pro_edge_port->pvid != NSM_VLAN_DEFAULT_VID)
                       {
                         cli_out (cli, " switchport provider-edge vlan %d "
                             "default-vlan %d\n", svlan_info->svid, 
                             pro_edge_port->pvid);
                       }/* if (pro_edge_port->pvid != NSM_VLAN_DEFAULT_VID) */

                   }/* LIST_LOOP (svlan_info->port_list, pro_edge_port, ls_node) */

               }/* AVL_TREE_LOOP (port->regtab->reg_tab, key1, node) */
             
           }/* if (port->regtab != NULL) */

        }/* if (port->mode == NSM_VLAN_PORT_MODE_CE) */

#endif /* HAVE_PROVIDER_BRIDGE */

#ifdef HAVE_QOS
      if (CHECK_FLAG (port->port_config, L2_PORT_CFG_DEF_USR_PRI))
        cli_out (cli, " user-priority %d\n", port->default_user_priority);

#ifdef HAVE_HAL
      if (CHECK_FLAG (port->port_config, L2_PORT_CFG_REGEN_USR_PRI))
        {
          for (user_priority = HAL_BRIDGE_MIN_USER_PRIO;
               user_priority <= HAL_BRIDGE_MAX_USER_PRIO; user_priority++)
            {
              if (port->user_prio_regn_table[user_priority] != user_priority)
                {
                  cli_out (cli, " user-priority-regen-table user-priority %d"
                           " regenerated-user-priority %d\n", user_priority,
                           port->user_prio_regn_table[user_priority]);
                }
            }
        }
#endif /* HAVE_HAL */

      if (CHECK_FLAG (port->port_config, L2_PORT_CFG_TRF_CLASS))
        {
          for (user_priority = HAL_BRIDGE_MIN_USER_PRIO;
               user_priority <= HAL_BRIDGE_MAX_USER_PRIO; user_priority++)
            {
#ifdef HAVE_HAL
              for (num_traffic_classes = HAL_BRIDGE_MIN_TRAFFIC_CLASS - 1;
                   num_traffic_classes < HAL_BRIDGE_MAX_TRAFFIC_CLASS;
                   num_traffic_classes++)
                {
                  if (port->traffic_class_table[user_priority][num_traffic_classes] !=
                      nsm_default_traffic_class_table[user_priority][num_traffic_classes])
                    {
                      cli_out (cli,
                               " traffic-class-table user-priority %d"
                               " num-traffic-classes %d value %d\n",
                               user_priority,
                               num_traffic_classes + 1,
                               port->traffic_class_table[user_priority][num_traffic_classes]);
                    }
                }
#endif /* HAVE_HAL */
            }
        }
#endif /* HAVE_QOS */
    }

  return 0;
}

#endif /* not HAVE_CUSTOM1 */

void
nsm_port_vlan_show(struct cli *cli, struct interface *ifp)
{
  u_int16_t vid;
  u_int32_t line_count;
  u_int32_t vlans_output=0;
  struct nsm_if *zif = NULL;
  struct nsm_vlan_port *vlan_port = NULL;
  struct nsm_bridge_port *br_port = NULL;
 
  if ( (!cli) || (!ifp) )
    return;
 
  if ( !(zif = (struct nsm_if *)ifp->info) )
    return;
 
  if ( !(br_port = zif->switchport) )
    return;

  if ( !(vlan_port = &br_port->vlan_port) )
    {
      cli_out(cli, "No Port Vlan Info for %s\n", ifp->name);
      return;
    }
 
  cli_out (cli, " Interface name         : %s\n", ifp->name);
  cli_out (cli, " Switchport mode        : ");
  cli_out (cli, "%s\n", nsm_vlan_if_mode_to_str (vlan_port->mode));

  cli_out(cli, " Ingress filter          : %s\n",
          ((vlan_port->flags & NSM_VLAN_ENABLE_INGRESS_FILTER) ?
           "enable":"disable"));
  cli_out(cli, " Acceptable frame types  : %s\n",
          ((vlan_port->flags & NSM_VLAN_ACCEPTABLE_FRAME_TYPE_TAGGED) ?
           "vlan-tagged only":"all"));
  cli_out(cli, " Default Vlan            : %u\n", (vlan_port->pvid));
  cli_out(cli, " Configured Vlans         : ");

  line_count = 1;
  for(vid = NSM_VLAN_DEFAULT_VID; vid <= NSM_VLAN_MAX; vid++)
    {
      if ( NSM_VLAN_BMP_IS_MEMBER(vlan_port->staticMemberBmp, vid) ||
           NSM_VLAN_BMP_IS_MEMBER(vlan_port->dynamicMemberBmp, vid) )
        {
          cli_out(cli, "%5u", vid);
          vlans_output++;
          if( vlans_output == (MAX_VLANS_SHOW_PER_LINE*line_count) )
            {
              cli_out(cli, "\n                           ");
              line_count++;
            }
        }
    }
  cli_out(cli, "\n");
}

void
nsm_all_port_vlan_show(struct cli *cli, char *brname)
{
  struct nsm_master *nm = NULL;
  struct nsm_bridge_master *master = NULL;
  struct nsm_bridge *bridge = NULL;
  struct listnode *ln = NULL;
  struct interface *ifp = NULL;
  struct nsm_if *zif = NULL;
 
  if ((!cli) || (!brname))
    return;

  nm = cli->vr->proto;
 
  if (nm)
    master = nsm_bridge_get_master (nm);

  if (master == NULL)
    return;
 
  if ( !(bridge = nsm_lookup_bridge_by_name(master, brname)) )
    {
      cli_out (cli, "%% Bridge not found \n");
      return;
    }
 
  if ( !(bridge->port_tree) )
    return;
 
  LIST_LOOP (nzg->ifg.if_list, ifp, ln)
    {
      zif = (struct nsm_if *) ifp->info;
      if (zif && zif->bridge)
        if (pal_strcmp (zif->bridge->name, brname) == 0)
          nsm_port_vlan_show (cli, ifp);
    }
}

int
nsm_vlan_show (struct cli *cli, struct nsm_vlan *vlan)
{
  struct nsm_bridge_port *br_port = NULL;
  struct nsm_vlan_port *port = NULL;
  struct interface *ifp = NULL;
  struct listnode *ln = NULL;
  struct nsm_show_vlan_arg *arg = NULL;
  struct nsm_if *zif = NULL;
  int count;
  char *str;
  int curr;
  int len;

  count = 0;
  
  if ((cli == NULL) || (vlan == NULL))
    return 0;

  arg = cli->arg;

  if ((arg == NULL) || (arg->bridge == NULL))
    return 0;
 
  if (CHECK_FLAG (arg->flags, NSM_SHOW_VLAN_BANNER))
    {
      cli_out (cli, "%-16s%-8s%-17s%-8s%-31s\n", "Bridge", "VLAN ID", " Name", "State", "Member ports");
      cli_out (cli, "%49s%s\n", " ", "(u)-Untagged, (t)-Tagged");
      cli_out (cli, "%s\n", "=============== ======= ================ ======= ===============================");
    }
 
  if ((vlan->type & NSM_VLAN_DYNAMIC) && (arg->type & NSM_VLAN_DYNAMIC))
    {
      cli_out (cli, "%-16s%-8d*%-16s%-8s", arg->bridge->name,
               vlan->vid, vlan->vlan_name,
               (( vlan->vlan_state == NSM_VLAN_ACTIVE) ? "ACTIVE" :
                (( vlan->vlan_state == NSM_VLAN_DISABLED) ? "SUSPEND":
                 "INVALID")));
    }
  else if ( (vlan->type & NSM_VLAN_STATIC) && (arg->type & NSM_VLAN_STATIC) )
    {
      cli_out (cli, "%-16s%-8d%-17s%-8s", arg->bridge->name,
               vlan->vid, vlan->vlan_name,
               (( vlan->vlan_state == NSM_VLAN_ACTIVE) ? "ACTIVE" :
                (( vlan->vlan_state == NSM_VLAN_DISABLED) ? "SUSPEND":
                 "INVALID")));
    }
  else if ( (vlan->type & NSM_VLAN_AUTO) && (arg->type & NSM_VLAN_AUTO) )
    {
      cli_out (cli, "%-16s%-8d%-17s%-8s", arg->bridge->name,
               vlan->vid, vlan->vlan_name,
               (( vlan->vlan_state == NSM_VLAN_ACTIVE) ? "ACTIVE" :
                (( vlan->vlan_state == NSM_VLAN_DISABLED) ? "SUSPEND":
                 "INVALID")));
    }
  else
    {
      return 0;
    }
 
  if ( !(vlan->port_list) )
    {
      cli_out(cli, "\n");
      count++;
      return count;
    }

  len = 0;
  curr = 0;

  LIST_LOOP(vlan->port_list, ifp, ln)
    {
      if ( (zif = (struct nsm_if *)(ifp->info)) 
           && (br_port = zif->switchport) )
       { 
         if ( (port = &br_port->vlan_port) )
           {
              /* If the vlan is not learnt dynamically and type is
               * Dynamic, then don't display the port; similarly if
               * the vlan is dynamically learnt and the show type is static,
               * then don't display the port.
               */
              if ( (!NSM_VLAN_BMP_IS_MEMBER(port->dynamicMemberBmp,vlan->vid))
                 && (arg->type == NSM_VLAN_DYNAMIC) )
                continue;

              if ( (!NSM_VLAN_BMP_IS_MEMBER(port->staticMemberBmp, vlan->vid))
                 && ((arg->type == NSM_VLAN_STATIC) || 
                     (arg->type == NSM_VLAN_AUTO)))
                continue;

             if ( NSM_VLAN_BMP_IS_MEMBER(port->egresstaggedBmp, vlan->vid) )
                str = "(t)";
             else
              str = "(u)";

              /* 3 for (u)/(t) and 1 for blank. */
              len = 3 + pal_strlen (ifp->name) + 1;
              if ((curr + len) <= 31)
                {
                  cli_out (cli, "%s%s ", ifp->name, str);
                  curr += len;
                }
              else
                {
                  cli_out (cli, "\n");
                  cli_out (cli, "%49s%s%s ", " ", ifp->name, str);
                  curr = len;
                }
           }/* if port */
       } /* if zif */
    } /* end for */

  cli_out(cli, "\n");

  count++;

  return count;
}

/* VLAN table callback. */
static int
nsm_vlan_show_callback (struct cli *cli)
{
  struct nsm_show_vlan_arg *arg;
  struct nsm_bridge *bridge;
  struct nsm_vlan *vlan;
  struct avl_node *an;
  int count;

  arg = cli->arg;

  if (arg == NULL)
    return 1;

  if ((bridge = arg->bridge) == NULL)
    return 1;

  count = 0;

  /* Current node. */
  an = cli->current;

  /* "show" connection is closed. */
  if (cli->status == CLI_CLOSE)
    goto CLEANUP;

  /* This is the first time. */
  if (cli->status == CLI_NORMAL)
    {
      SET_FLAG (arg->flags, NSM_SHOW_VLAN_BANNER);
      cli->count = 0;
    }

  for (an = avl_getnext (bridge->vlan_table, NULL); an;
       an = avl_getnext (bridge->vlan_table, an))
    {
      vlan = an->info;
      if (vlan != NULL)
       {
          if (CHECK_FLAG (arg->flags, NSM_SHOW_VLAN_VIDCMD)
              && (arg->vid != vlan->vid))
            continue;

#ifdef HAVE_PROVIDER_BRIDGE
          if (bridge->provider_edge == PAL_TRUE
              && CHECK_FLAG (arg->flags, NSM_SHOW_VLAN_BANNER))
            {
              cli_out (cli, "Customer VLANs\n");
            }
#endif /* HAVE_PROVIDER_BRIDGE */

          count = nsm_vlan_show (cli, vlan);

          if (count && CHECK_FLAG (arg->flags, NSM_SHOW_VLAN_BANNER))
            UNSET_FLAG (arg->flags, NSM_SHOW_VLAN_BANNER);

         if (count >= 5)
           {
             cli->status = CLI_CONTINUE;
             cli->count += count;
             cli->current = avl_getnext (bridge->vlan_table, an);
             cli->callback = nsm_vlan_show_callback;
             return 0;
           }
       }
    }

#ifdef HAVE_PROVIDER_BRIDGE

  if (!CHECK_FLAG (arg->flags, NSM_SHOW_CVLAN_DONE))
    {
      SET_FLAG (arg->flags, NSM_SHOW_CVLAN_DONE);

      if (bridge->provider_edge == PAL_TRUE
          && !CHECK_FLAG (arg->flags, NSM_SHOW_VLAN_BANNER))
        SET_FLAG (arg->flags, NSM_SHOW_VLAN_BANNER);

      for (an = avl_getnext (bridge->svlan_table, NULL); an; 
           an = avl_getnext (bridge->svlan_table, an))
        {
          vlan = an->info;

          if (vlan != NULL)
           {
              if (CHECK_FLAG (arg->flags, NSM_SHOW_VLAN_VIDCMD)
                  && (arg->vid != vlan->vid))
                continue;

              if (bridge->provider_edge == PAL_TRUE
                  && CHECK_FLAG (arg->flags, NSM_SHOW_VLAN_BANNER))
                {
                  cli_out (cli, "\n");
                  cli_out (cli, "Service VLANs\n");
                }

              count = nsm_vlan_show (cli, vlan);

              if (count && CHECK_FLAG (arg->flags, NSM_SHOW_VLAN_BANNER))
                UNSET_FLAG (arg->flags, NSM_SHOW_VLAN_BANNER);

             if (count >= 5)
               {
                 cli->status = CLI_CONTINUE;
                 cli->count += count;
                 cli->current = avl_getnext (bridge->svlan_table, an);
                 cli->callback = nsm_vlan_show_callback;
                 return 0;
               }
           }
        }
     }

#endif /* HAVE_PROVIDER_BRIDGE */

#ifdef HAVE_B_BEB

      if (bridge->backbone_edge == PAL_TRUE
          && !CHECK_FLAG (arg->flags, NSM_SHOW_VLAN_BANNER))
        SET_FLAG (arg->flags, NSM_SHOW_VLAN_BANNER);

      for (an = avl_top (bridge->bvlan_table); an; an = avl_next (an))
        {
          vlan = an->info;

          if (vlan != NULL)
           {
              if (CHECK_FLAG (arg->flags, NSM_SHOW_VLAN_VIDCMD)
                  && (arg->vid != vlan->vid))
                continue;

              if (bridge->backbone_edge == PAL_TRUE
                  && CHECK_FLAG (arg->flags, NSM_SHOW_VLAN_BANNER))
                {
                  cli_out (cli, "\n");
                  cli_out (cli, "backbone VLANs\n");
                }

              count = nsm_vlan_show (cli, vlan);

              if (count && CHECK_FLAG (arg->flags, NSM_SHOW_VLAN_BANNER))
                UNSET_FLAG (arg->flags, NSM_SHOW_VLAN_BANNER);

             if (count >= 5)
               {
                 cli->status = CLI_CONTINUE;
                 cli->count += count;
                 cli->current = avl_next (an);
                 cli->callback = nsm_vlan_show_callback;
                 return 0;
               }
           }
        }
    
#endif /*HAVE_B_BEB*/
  /* Clean up.  */
 CLEANUP:

  /* Call clean up routine.  */
  if (cli->cleanup)
    (*cli->cleanup) (cli);

  /* Set NULL to callback pointer.  */
  cli->status = CLI_CONTINUE;
  cli->count += count;
  cli->callback = NULL;
  cli->arg = NULL;

  return 0;
}

void
nsm_all_vlan_show (struct cli *cli, char *brname,
                   int type, nsm_vid_t vid)
{
  struct nsm_bridge_master *master;
  struct nsm_bridge *bridge;
  struct nsm_master *nm;
  struct nsm_show_vlan_arg *arg;

  if ((cli == NULL) || (brname == NULL))
    return;

  nm = cli->vr->proto;
  if (nm == NULL)
    return;

  master = nsm_bridge_get_master (nm);
  if (master == NULL)
    return;

  bridge = nsm_lookup_bridge_by_name(master, brname);
  if (bridge == NULL)
    return;
 
  if (bridge->vlan_table == NULL)
    return;
 
  /* Set arguments. */
  arg = XCALLOC (MTYPE_TMP, sizeof (struct nsm_show_vlan_arg));
  if (arg == NULL)
    return;

  arg->bridge = bridge;
  arg->type = type;
  arg->vid = vid;
  if (vid)
     SET_FLAG (arg->flags, NSM_SHOW_VLAN_VIDCMD);

  /* Set top node to current pointer in CLI structure. */
  cli->arg = arg;
  cli->current = avl_top (bridge->vlan_table);

  /* Call display function. */
  nsm_vlan_show_callback (cli);

  if (arg)
    XFREE (MTYPE_TMP, arg);
}


void
nsm_show_vid_static_ports (struct cli *cli, struct nsm_vlan *vlan)
{
  struct listnode *node = NULL;
  struct interface *ifp = NULL;
  int len = 0;
  int curr = 0;

  if (vlan == NULL)
    return;

  cli_out (cli, "%-16s%-16d", vlan->bridge->name, vlan->vid);

  if (!vlan->forbidden_port_list)
    cli_out (cli, "\n");

  if (vlan->forbidden_port_list)
    LIST_LOOP (vlan->forbidden_port_list, ifp, node)
      {
        len = pal_strlen (ifp->name);
        if (curr <= 31)
          {
            cli_out (cli, "%s%s",ifp->name,"  ");
            curr = curr + len;
          }
        else
          {
            cli_out (cli, "\n");
            cli_out (cli, "%32s%s", " ", ifp->name);
            curr = len;

          }
      }
  cli_out (cli, "\n");
}

void
nsm_vlan_show_static_ports (struct cli *cli, char *brname, nsm_vid_t vid)
{
  struct nsm_bridge_master *master;
  struct nsm_bridge *bridge;
  struct nsm_master *nm;
  struct nsm_vlan *vlan = NULL;
  struct avl_node *rn = NULL;

  if ((cli == NULL) || (brname == NULL))
    return;

  nm = cli->vr->proto;
  if (nm == NULL)
    return;

  master = nsm_bridge_get_master (nm);
  if (master == NULL)
    return;

  bridge = nsm_lookup_bridge_by_name(master, brname);
  if (bridge == NULL)
    return;

  if (bridge->vlan_table == NULL)
    return;


  for (rn = avl_top (bridge->vlan_table); rn; rn = avl_next (rn))
    {
      if ((vlan = rn->info))
        {
          if (vid != 0)
            {
              if (vlan->vid == vid)
                {
                  nsm_show_vid_static_ports (cli, vlan);
                  break;
                }
              else
                continue;
            }
          else
            {
              nsm_show_vid_static_ports (cli, vlan);
            }

        }
    }


}

#ifdef HAVE_PROVIDER_BRIDGE

int
nsm_cvlan_regis_config_write (struct cli *cli)
{
  int write = 0;
  struct listnode *nn;
  struct avl_node *node;
  struct nsm_bridge *bridge;
  struct apn_vr *vr = cli->vr;
  struct nsm_cvlan_reg_key *key;
  struct nsm_cvlan_reg_tab *regtab;
  struct nsm_master *nm = vr->proto;
  struct nsm_bridge_master *master = nm->bridge;

  if (! master)
    return 0;

  bridge = master->bridge_list;

  if (! bridge)
    return 0;

  while (bridge)
    {
      LIST_LOOP (bridge->cvlan_reg_tab_list, regtab, nn)
      {
        cli_out (cli, "!\n");
        cli_out (cli, "cvlan registration table %s %s\n", regtab->name,
                 nsm_get_bridge_str (bridge));
        write++;

        for (node = avl_top (regtab->reg_tab); node; node = avl_next (node))
          {
            if (! (key = AVL_NODE_INFO (node)))
              continue;

            cli_out (cli, " cvlan %d svlan %d\n", key->cvid, key->svid);
            write++;
          }
      }

      bridge = bridge->next;
    }

  return write;
}

#endif /* HAVE_PROVIDER_BRIDGE */




#ifdef HAVE_G8032
int g8032_vlan_config_write (struct cli *);
#endif  /*HAVE_G8032*/

/* Install VLAN CLIs.  */
void
nsm_vlan_cli_init (struct cli_tree *ctree)
{
  cli_install_default (ctree, VLAN_MODE);
  cli_install_config (ctree, VLAN_MODE, nsm_vlan_config_write);
  
  cli_install (ctree, CONFIG_MODE, &nsm_vlan_database_cmd);

  cli_install (ctree, VLAN_MODE, &nsm_vlan_name_cmd);
  cli_install (ctree, VLAN_MODE, &nsm_br_vlan_name_cmd);
  cli_install (ctree, VLAN_MODE, &nsm_vlan_no_name_cmd);
  cli_install (ctree, VLAN_MODE, &nsm_br_vlan_no_name_cmd);
  cli_install (ctree, VLAN_MODE, &no_nsm_vlan_no_name_cmd);
  cli_install (ctree, VLAN_MODE, &no_nsm_br_vlan_no_name_cmd);
  cli_install (ctree, VLAN_MODE, &nsm_vlan_enable_disable_cmd);
  cli_install (ctree, VLAN_MODE, &nsm_br_vlan_enable_disable_cmd);
  cli_install (ctree, VLAN_MODE, &nsm_vlan_mtu_cmd);
  cli_install (ctree, VLAN_MODE, &nsm_br_vlan_mtu_cmd);
  cli_install (ctree, VLAN_MODE, &no_nsm_vlan_mtu_cmd);
  cli_install (ctree, VLAN_MODE, &no_nsm_br_vlan_mtu_cmd);

#ifdef HAVE_PROVIDER_BRIDGE
  cli_install (ctree, VLAN_MODE, &nsm_br_cus_vlan_name_cmd);
  cli_install (ctree, VLAN_MODE, &nsm_cus_vlan_name_cmd);
  cli_install (ctree, VLAN_MODE, &nsm_cus_vlan_no_name_cmd);
  cli_install (ctree, VLAN_MODE, &nsm_br_cus_vlan_no_name_cmd);
  cli_install (ctree, VLAN_MODE, &no_nsm_pro_vlan_no_name_cmd);
  cli_install (ctree, VLAN_MODE, &no_nsm_br_pro_vlan_no_name_cmd);
  cli_install (ctree, VLAN_MODE, &nsm_cus_vlan_enable_disable_cmd);
  cli_install (ctree, VLAN_MODE, &nsm_br_cus_vlan_enable_disable_cmd);
  cli_install (ctree, VLAN_MODE, &nsm_br_ser_vlan_name_cmd);
  cli_install (ctree, VLAN_MODE, &nsm_ser_vlan_name_cmd);
  cli_install (ctree, VLAN_MODE, &nsm_ser_vlan_no_name_cmd);
  cli_install (ctree, VLAN_MODE, &nsm_br_ser_vlan_no_name_cmd);
  cli_install (ctree, VLAN_MODE, &nsm_ser_vlan_enable_disable_cmd);
  cli_install (ctree, VLAN_MODE, &nsm_br_ser_vlan_enable_disable_cmd);
  cli_install (ctree, VLAN_MODE, &nsm_pro_vlan_mtu_cmd);
  cli_install (ctree, VLAN_MODE, &nsm_br_pro_vlan_mtu_cmd);
  cli_install (ctree, VLAN_MODE, &nsm_br_vlan_mtu_cmd);
  cli_install (ctree, VLAN_MODE, &no_nsm_vlan_mtu_cmd);
  cli_install (ctree, VLAN_MODE, &no_nsm_br_vlan_mtu_cmd);
  #ifdef HAVE_B_BEB
  cli_install (ctree, VLAN_MODE, &nsm_backbone_vlan_name_cmd);
  #endif /* HAVE_B_BEB */
  cli_install_default (ctree, CVLAN_REGIST_MODE);
  cli_install_config (ctree, CVLAN_REGIST_MODE, nsm_cvlan_regis_config_write);
  cli_install (ctree, CONFIG_MODE, &nsm_cvlan_reg_tab_cmd);
  cli_install (ctree, CONFIG_MODE, &no_nsm_cvlan_reg_tab_cmd);
  cli_install (ctree, CONFIG_MODE, &nsm_cvlan_reg_tab_br_cmd);
  cli_install (ctree, CONFIG_MODE, &no_nsm_cvlan_reg_tab_br_cmd);
  cli_install (ctree, CONFIG_MODE, &nsm_vlan_preserve_cos_cmd);
  cli_install (ctree, CONFIG_MODE, &no_nsm_vlan_preserve_cos_cmd);
  cli_install (ctree, CONFIG_MODE, &nsm_br_vlan_preserve_cos_cmd);
  cli_install (ctree, CONFIG_MODE, &no_nsm_br_vlan_preserve_cos_cmd);

  cli_install (ctree, CVLAN_REGIST_MODE, &nsm_cvlan_svlan_map_cmd);
  cli_install (ctree, CVLAN_REGIST_MODE, &no_nsm_cvlan_map_range_cmd);
  cli_install (ctree, CVLAN_REGIST_MODE, &no_nsm_cvlan_map_svid_range_cmd);
#endif /* HAVE_PROVIDER_BRIDGE */

  /* Switchmode general CLIs. */
  cli_install (ctree, INTERFACE_MODE, &nsm_vlan_switchport_mode_cmd);
  cli_install (ctree, INTERFACE_MODE, &nsm_vlan_switchport_ingress_filter_enable_cmd);
#if defined HAVE_I_BEB && !defined HAVE_B_BEB
  cli_install (ctree, INTERFACE_MODE, &nsm_vlan_switchport_mode_pbb_i_cmd);
#endif
#if defined HAVE_I_BEB && defined HAVE_B_BEB
  cli_install (ctree, INTERFACE_MODE, &nsm_vlan_switchport_mode_pbb_ib_cmd);
#endif
#if !defined HAVE_I_BEB && defined HAVE_B_BEB
  cli_install (ctree, INTERFACE_MODE, &nsm_vlan_switchport_mode_pbb_b_cmd);
#endif
#ifdef HAVE_PBB_TE
  cli_install (ctree, VLAN_MODE, &nsm_pbb_te_vlan_allowed_forbidden_cmd);
#endif /* HAVE_TE */

  /* Switchmode Default VLAN CLIs. */
  cli_install (ctree, INTERFACE_MODE, &nsm_vlan_switchport_pvid_cmd);
  cli_install (ctree, INTERFACE_MODE, &no_vlan_switchport_pvid_cmd);

  /* Switchmode hybrid CLIs. */
  cli_install (ctree, INTERFACE_MODE, &nsm_vlan_switchport_hybrid_acceptable_frame_cmd);
  cli_install (ctree, INTERFACE_MODE, &nsm_vlan_switchport_hybrid_add_egress_cmd);
  cli_install (ctree, INTERFACE_MODE, &nsm_vlan_switchport_hybrid_delete_cmd);
  cli_install (ctree, INTERFACE_MODE, &nsm_vlan_switchport_hybrid_allowed_all_cmd);
  cli_install (ctree, INTERFACE_MODE, &nsm_vlan_switchport_allowed_vlan_none_cmd);

  /* Switchmode Trunk/ PN CLIs. */
  cli_install (ctree, INTERFACE_MODE, &nsm_vlan_switchport_trunk_pn_cn_all_cmd);
  cli_install (ctree, INTERFACE_MODE, &nsm_vlan_switchport_trunk_pn_cn_none_cmd);
  cli_install (ctree, INTERFACE_MODE, &nsm_vlan_switchport_trunk_pn_cn_add_cmd);
  cli_install (ctree, INTERFACE_MODE, &nsm_vlan_switchport_trunk_pn_cn_delete_cmd);
  cli_install (ctree, INTERFACE_MODE, &nsm_vlan_switchport_trunk_pn_cn_except_cmd);

  /* SWitchport Trunk only CLIs. */
  cli_install (ctree, INTERFACE_MODE, &no_nsm_vlan_switchport_trunk_vlan_cmd);
  cli_install (ctree, INTERFACE_MODE, &nsm_vlan_switchport_trunk_native_cmd);
  cli_install (ctree, INTERFACE_MODE, &no_nsm_vlan_switchport_trunk_native_cmd);


#ifdef HAVE_PROVIDER_BRIDGE
  
  /* EVC-ID Set CLI */
  cli_install (ctree, VLAN_MODE, &nsm_vlan_set_evc_id_cmd);
  cli_install (ctree, VLAN_MODE, &no_nsm_vlan_set_evc_id_cmd);
  
  /* MAX UNI per EVC */
  cli_install (ctree, VLAN_MODE, &nsm_br_vlan_max_uni_cmd);
  cli_install (ctree, VLAN_MODE, &no_nsm_br_vlan_max_uni_cmd);

  
  /* CE VLAN Registration CLIs. */
  cli_install (ctree, INTERFACE_MODE, &nsm_vlan_switchport_ce_vlan_reg_cmd);
  cli_install (ctree, INTERFACE_MODE, &no_nsm_vlan_switchport_ce_vlan_reg_cmd);

  /* VLAN Translation CLIs. */
  cli_install (ctree, INTERFACE_MODE, &nsm_vlan_switchport_cn_svlan_trans_cmd);
  cli_install (ctree, INTERFACE_MODE, &no_nsm_vlan_switchport_cn_svlan_trans_cmd);
  cli_install (ctree, INTERFACE_MODE, &nsm_vlan_switchport_pn_svlan_trans_cmd);
  cli_install (ctree, INTERFACE_MODE, &no_nsm_vlan_switchport_pn_svlan_trans_cmd);
  cli_install (ctree, INTERFACE_MODE, &nsm_vlan_switchport_ce_vlan_trans_cmd);
  cli_install (ctree, INTERFACE_MODE, &no_nsm_vlan_switchport_ce_vlan_trans_cmd);
  cli_install (ctree, INTERFACE_MODE, &nsm_vlan_switchport_untagged_vid_pe_cmd);
  cli_install (ctree, INTERFACE_MODE, &nsm_vlan_switchport_pvid_pe_cmd);

  /* Switchmode CE CLIs. */
  cli_install (ctree, INTERFACE_MODE, &nsm_vlan_switchport_mode_ce_cmd);
  cli_install (ctree, INTERFACE_MODE,
               &nsm_vlan_switchport_ingress_filter_enable_ce_cmd);
  cli_install (ctree, INTERFACE_MODE,
               &nsm_vlan_switchport_ce_acceptable_frame_cmd);
  cli_install (ctree, INTERFACE_MODE, &nsm_vlan_switchport_ce_pvid_cmd);
  cli_install (ctree, INTERFACE_MODE, &no_nsm_vlan_switchport_ce_pvid_cmd);
  cli_install (ctree, INTERFACE_MODE,
               &nsm_vlan_switchport_ce_hybrid_add_egress_cmd);
  cli_install (ctree, INTERFACE_MODE, &nsm_vlan_switchport_ce_trunk_add_cmd);
  cli_install (ctree, INTERFACE_MODE, &nsm_vlan_switchport_ce_hr_delete_cmd);
  cli_install (ctree, INTERFACE_MODE, &nsm_vlan_switchport_ce_hr_allowed_all_cmd);
  cli_install (ctree, INTERFACE_MODE, &nsm_vlan_switchport_ce_hr_allowed_none_cmd);
  cli_install (ctree, INTERFACE_MODE, &nsm_vlan_switchport_ce_tr_delete_cmd);
  cli_install (ctree, INTERFACE_MODE, &nsm_vlan_switchport_ce_tr_allowed_all_cmd);
  cli_install (ctree, INTERFACE_MODE, &nsm_vlan_switchport_ce_tr_allowed_none_cmd);

  cli_install (ctree, INTERFACE_MODE, &nsm_vlan_switchport_cn_pvid_cmd);
  cli_install (ctree, INTERFACE_MODE, &no_nsm_vlan_switchport_cn_pvid_cmd);
  cli_install (ctree, INTERFACE_MODE, &nsm_vlan_switchport_ce_default_evc_cmd);
  cli_install (ctree, INTERFACE_MODE, &no_nsm_vlan_switchport_ce_default_evc_cmd);

#endif /* HAVE_PROVIDER_BRIDGE */

#ifdef HAVE_QOS
  cli_install (ctree, INTERFACE_MODE, &traffic_class_table_cmd);
  cli_install (ctree, INTERFACE_MODE, &default_user_priority_cmd);
  cli_install (ctree, INTERFACE_MODE, &user_prio_regen_table_cmd);
#endif /* HAVE_QOS */

  cli_install (ctree, EXEC_MODE, &show_interface_switchport_bridge_cmd);
  cli_install (ctree, EXEC_MODE, &show_nsm_vlan_bridge_cmd);

#ifdef HAVE_DEFAULT_BRIDGE
  cli_install (ctree, EXEC_MODE, &show_nsm_vlan_spanningtree_bridge_cmd);
#endif /* HAVE_DEFAULT_BRIDGE */

  cli_install (ctree, EXEC_MODE, &show_nsm_vlan_brief_cmd);
  cli_install (ctree, EXEC_MODE, &show_nsm_vlan_static_ports_cmd);
#ifdef HAVE_PROVIDER_BRIDGE
  cli_install (ctree, EXEC_MODE, &show_nsm_cvlan_reg_tab_cmd);
  cli_install (ctree, EXEC_MODE, &show_nsm_svlan_trans_tab_cmd);
  cli_install (ctree, EXEC_MODE, &show_nsm_uni_evc_id_cmd);
  cli_install (ctree, EXEC_MODE, &nsm_show_ce_vlan_cos_preserve_cmd);
  cli_install (ctree, EXEC_MODE, &nsm_show_uni_list_cmd);
  cli_install (ctree, EXEC_MODE, &nsm_show_uni_bundling_cmd);
  /* CLI to display uni attributes and evc attributes */
  cli_install (ctree, EXEC_MODE, &nsm_show_ethernet_attributes_cmd);

#endif /* HAVE_PROVIDER_BRIDGE */

#ifdef HAVE_QOS
  cli_install (ctree, EXEC_MODE, &show_traffic_class_table_cmd);
  cli_install (ctree, EXEC_MODE, &show_default_priority_cmd);
  cli_install (ctree, EXEC_MODE, &show_user_prio_regen_table_cmd);
#endif /* HAVE_QOS */

#ifdef HAVE_G8031
  cli_install_default (ctree, G8031_CONFIG_VLAN_MODE);
  cli_install_config (ctree, G8031_CONFIG_VLAN_MODE, nsm_g8031_vlan_config_write);
#ifdef HAVE_B_BEB
#ifdef HAVE_I_BEB 
  cli_install (ctree, CONFIG_MODE, &g8031_i_b_beb_eps_vlan_create_cmd);
#else
  cli_install (ctree, CONFIG_MODE, &g8031_beb_eps_vlan_create_cmd); 
#endif /* HAVE_I_BEB */  
#else  
  cli_install (ctree, CONFIG_MODE, &g8031_eps_vlan_create_cmd);
#endif /*HAVE_B_BEB */
  cli_install (ctree, G8031_CONFIG_VLAN_MODE, &g8031_eps_primary_vlan_create_cmd);
#endif /* HAVE_G8031 */
#ifdef HAVE_G8032
  cli_install_default (ctree, G8032_CONFIG_VLAN_MODE);
  cli_install_config (ctree, G8032_CONFIG_VLAN_MODE, 
                      nsm_g8032_vlan_config_write);
#ifdef HAVE_B_BEB
  cli_install (ctree, CONFIG_MODE, &g8032_beb_vlan_create_cmd);
#else
  cli_install_default (ctree, G8032_CONFIG_VLAN_MODE);
  cli_install (ctree, CONFIG_MODE, &g8032_vlan_create_cmd);
#endif /*HAVE_B_BEB*/
  cli_install (ctree, G8032_CONFIG_VLAN_MODE, 
               &g8032_primary_vlan_create_cmd);
#endif /* HAVE_G8032*/
}

#endif /* HAVE_VLAN */
