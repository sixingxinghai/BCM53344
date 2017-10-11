
/* Copyright (C) 2004  All Rights Reserved.  */
#include "pal.h"
#include "lib.h"
#include "avl_tree.h"
#include "hash.h"
#include "hal_incl.h"

#include "nsmd.h"
#include "nsm_message.h"
#include "nsm_server.h"
#include "nsm_api.h"

#ifdef HAVE_L2
#include "nsm_bridge.h"
#ifdef HAVE_VLAN
#include "nsm_vlan.h"
#include "nsm_vlan_cli.h"
#endif /* HAVE_VLAN */
#endif /* HAVE_L2 */
#include "nsm_interface.h"
#include "nsm_bridge_cli.h"

#if defined (HAVE_I_BEB) || defined (HAVE_B_BEB)
#include "nsm_pbb_cli.h"
#include "nsm_bridge_pbb.h"

int 
nsm_pbb_cli_return(struct cli *cli, int ret)
{
  char *str;

  if (ret >= 0)
    return CLI_SUCCESS;

  switch (ret)
    {
    case NSM_BRIDGE_ERR_MEM:
      str = "memory allocation failed";
      break;
    case NSM_VLAN_ERR_IFP_NOT_BOUND:
      str = "interface is not bounded to proper bridge type";
      break;
    case NSM_PBB_VLAN_ERR_PORT_MODE_INVALID:
      str = "switch port mode not set properly";
      break;
    case NSM_PBB_VLAN_ERR_PORT_CNP:
      str = "port is already configured as cnp";
      break;
    case NSM_PBB_VIP_PIP_MAP_EXHAUSTED:
      str = "no more VIP can be added to this pip";
      break;
    case NSM_PBB_VLAN_ERR_CNP_PORT_BASED:
      str = "this CNP port is configured as port based ";
      break;
    case NSM_PBB_VLAN_ERR_SERVICE_DISPATCHED:
      str = "cannot edit a dispatched service";
      break;
    case NSM_PBB_BACKBONE_BRIDGE_UNCONFIGURED:
      str = "backbone bridge not configured";
      break;
    case NSM_PBB_VLAN_ERR_PORT_ENTRY_DOESNOT_EXIST:
      str = "no corresponding entry been added";
      break;
    case NSM_PBB_VLAN_ERR_PORT_ENTRY_EXISTS:
      str = "Entry already exists";
      break;
    case NSM_PBB_SERVICE_REMOVE_ERROR:
     str = "unable to remove this service";
     break;
    case NSM_VLAN_ERR_VLAN_NOT_FOUND:
     str = "error: trying to add an non-existing vlan";
     break;
    case NSM_PBB_VLAN_ERR_PORT_ADDR_NOT_SET:
     str = "Backbone mac address for this CBP not set";
     break;
    default:
     str = "error";
    }

  cli_out (cli, "%% %s\n", str);
  return CLI_ERROR;
}

#ifdef HAVE_I_BEB

/* CLI to enter ISID config mode */
CLI (pbb_isid_list,
     pbb_isid_list_cmd,
     "pbb isid list",
     "pbb related commands",
     "isid configuration mode",
     "isid configuration mode")
{
  cli->mode = PBB_ISID_CONFIG_MODE;
  return CLI_SUCCESS;
}

/* CLI to add ISID pool to an I component */
CLI (pbb_isid_list_icomp_add,
     pbb_isid_list_icomp_add_cmd,
     "isid <1-16777214> name ISID_NAME i-component <1-32>",
     "service instance related parameters",
     "service instance id",
     "service instance name",
     "service instance name vlaue",
     "specifies the icomponent to which this isid belongs",
     "icomponent id")
{
  u_int32_t isid = 0;
  struct nsm_master *nm = cli->vr->proto;
  struct nsm_bridge_master *master = nsm_bridge_get_master (nm);
  int brno = 0;
  int ret = 0;

  /* To check the i-comp range */
  CLI_GET_INTEGER_RANGE ("Max i-component value", brno, argv[2], 1, 32);

  /* validate isid_name as well */  
  isid = nsm_pbb_search_isid_by_instance_name (argv[1], 
                                               master->beb->icomp);
  if (isid)
    {
      cli_out (cli, "service instance by this name already exists\n");
      return CLI_ERROR;
    }

  /* To check the ISID range */
  CLI_GET_INTEGER_RANGE ("ISID value", isid, argv[0], NSM_PBB_ISID_MIN,
                          NSM_PBB_ISID_MAX);

  ret = nsm_pbb_create_isid (master, argv[2], isid, argv[1]);
  
  if (!ret )
    return CLI_SUCCESS;
  else
    {
      cli_out (cli, "Service instance cannot be created\n");
      return CLI_ERROR;
    }
}

CLI (pbb_isid_list_icomp_del,
     pbb_isid_list_icomp_del_cmd,
     "no (isid <1-16777214> | instance-name ISID_NAME ) i-component <1-32>",
     "negates or sets to default",
     "service instance related parameters",
     "service instance id",
     "service instance name",
     "service instance name value",
     "specifies the icomponent to which this isid belongs",
     "icomponent id")
{
  u_int32_t isid = 0;
  struct nsm_master *nm = cli->vr->proto;
  struct nsm_bridge_master *master = nsm_bridge_get_master (nm);
  int brno = 0;
  int ret = 0;

  /* To check the ISID range */
  if (argv[0][1] == 's')
    CLI_GET_INTEGER_RANGE ("ISID value", isid, argv[1], NSM_PBB_ISID_MIN,
                            NSM_PBB_ISID_MAX);
  else
    {
      isid = nsm_pbb_search_isid_by_instance_name (argv[1], master->beb->icomp);
      if (isid == 0)
        {
          cli_out (cli,"instance %s not added \n", argv[1]);
          return CLI_ERROR;
        }
    }

  /* To check the i-comp range */
  CLI_GET_INTEGER_RANGE ("Max i-component value", brno, argv[2], 1, 32);

  ret = nsm_pbb_delete_isid (master, argv[2], isid);

  if (!ret )
    return CLI_SUCCESS;
  else
    {
      cli_out (cli, "Service instance doesnot exists\n");
      return CLI_ERROR;
    }
}

#endif /*HAVE_I_BEB*/

#ifdef HAVE_I_BEB 
#ifndef HAVE_B_BEB
/* CLI to confiugre CNP as port based port */
CLI (nsm_pbb_switchport_beb_cnp_port_based,
   nsm_pbb_switchport_beb_cnp_port_based_cmd,
   "switchport beb customer-network port-based "
   "(instance <1-16777214>|name INSTANCE_NAME) egress-port IFNAME",
   "Set the switching characteristics of the Layer2 interface",
   "Identifies this interface as one of the backbone port types.",
   "Configures the port as customer network port.",
   "Configures port based service interface for CNP as in IEEE 802.1ah.",
   "Identifies the parameter followed is an instance ID ",
   "Service Instance ID",
   "ASCII name for the service instance.",
   "The ascii name of the service instance.",
   "The egress-pip-port for this mapping.",
   "The physical PIP port for this service instance.")
{
  int ret;
  pbb_isid_t isid;
  struct interface *ifp = cli->index;
  struct interface *ifp1 = NULL;
  struct nsm_master *nm = cli->vr->proto;
  struct nsm_bridge_master *master = nsm_bridge_get_master (nm);


  /* To check the ISID range */
  if (argv[0][0] == 'i')
    CLI_GET_INTEGER_RANGE ("ISID value", isid, argv[1], NSM_PBB_ISID_MIN,
                            NSM_PBB_ISID_MAX);
  else
    {
      isid = nsm_pbb_search_isid_by_instance_name (argv[1], master->beb->icomp);
      if (isid == 0)
        {
          cli_out (cli, "instance %s not added \n", argv[1]);
          return CLI_ERROR;
        }
    }

  if ((ifp1 = if_lookup_by_name (&cli->vr->ifm, argv[2])) == NULL )
    {
      cli_out (cli, "%% Interface not found\n");
      return CLI_ERROR;
    }

  /* To check whether the bridge is configured for switching and vlan aware */
  NSM_BRIDGE_CHECK_L2_PROPERTY(cli, ifp);
  NSM_BRIDGE_CHECK_BRIDGE_PROPERTY(cli, ifp);

  /*API to configure CNP as Port based */
  ret = nsm_vlan_port_beb_add_cnp_portbased (ifp, ifp1, isid, NULL);   

  return nsm_pbb_cli_return(cli, ret);
}

#else /*if B_BEB is also defined */
/* below is the CLI in IB_BEB scenario */
/* CLI to confiugre CNP as port based port */
CLI (nsm_pbb_switchport_beb_cnp_port_based,
   nsm_pbb_switchport_beb_cnp_port_based_cmd,
   "switchport beb customer-network port-based "
   "(instance <1-16777214>|name INSTANCE_NAME)", 
   "Set the switching characteristics of the Layer2 interface",
   "Identifies this interface as one of the backbone port types.",
   "Configures the port as customer network port.",
   "Configures port based service interface for CNP as in IEEE 802.1ah.",
   "Identifies the parameter followed is an instance ID ",
   "Service Instance ID",
   "ASCII name for the service instance.",
   "The ascii name of the service instance.")
{
  int ret;
  pbb_isid_t isid;
  struct interface *ifp = cli->index;
  struct nsm_master *nm = cli->vr->proto;
  struct nsm_bridge_master *master = nsm_bridge_get_master (nm);


  /* To check the ISID range */
  if (argv[0][0] == 'i')
    CLI_GET_INTEGER_RANGE ("ISID value", isid, argv[1], NSM_PBB_ISID_MIN,
                            NSM_PBB_ISID_MAX);
  else
    {
      isid = nsm_pbb_search_isid_by_instance_name (argv[1], master->beb->icomp);
      if (isid == 0)
        {
          cli_out (cli, "instance %s not added \n", argv[1]);
          return CLI_ERROR;
        }
    }

  /* To check whether the bridge is configured for switching and vlan aware */
  NSM_BRIDGE_CHECK_L2_PROPERTY(cli, ifp);
  NSM_BRIDGE_CHECK_BRIDGE_PROPERTY(cli, ifp);

  /*API to configure CNP as Port based */
  ret = nsm_vlan_port_beb_add_cnp_portbased (ifp, NULL, isid, NULL);

  return nsm_pbb_cli_return(cli, ret);
}

#endif /*HAVE_B_BEB*/

/* Negate the CNP Port based CLI */
CLI (no_switchport_beb_cnp_port_based,
     no_switchport_beb_cnp_port_based_cmd, 
     "no switchport beb customer-network port-based (instance <1-16777214>|name INSTANCE_NAME)",
     "negate a command or set its default",
     "Set the switching characteristics of the Layer2 interface",
     "Identifies this interface as one of the backbone port types",
     "Configures the port as customer network port",
     "Configures port based service interface for CNP as in IEEE 802.1ah.",
     "Identifies the parameter followed is an instance ID ",
     "Service instance ID.",
     "ASCII name for the service instance.",
     "The ascii name of the service instance.")
{
  int ret;
  pbb_isid_t isid;
  struct interface *ifp = cli->index;
  struct nsm_master *nm = cli->vr->proto;
  struct nsm_bridge_master *master = nsm_bridge_get_master (nm);

  /* To check the ISID range */
  if (argv[0][0] == 'i')
    CLI_GET_INTEGER_RANGE ("ISID value", isid, argv[1], NSM_PBB_ISID_MIN,
                            NSM_PBB_ISID_MAX);
  else
    {
      isid = nsm_pbb_search_isid_by_instance_name (argv[1], master->beb->icomp);
      if (isid == 0)
        {
          cli_out (cli, "instance %s not added \n", argv[1]);
          return CLI_ERROR;
        }
    }
  
  /* To check whether the bridge is configured for switching and vlan aware */
  NSM_BRIDGE_CHECK_L2_PROPERTY(cli, ifp);
  NSM_BRIDGE_CHECK_BRIDGE_PROPERTY(cli, ifp); 
  /* To delete the portbased CNP */
  ret = nsm_vlan_port_beb_del_cnp_portbased(ifp, isid);
  return nsm_pbb_cli_return(cli, ret);
}
#endif /* HAVE_I_BEB */

#if defined HAVE_I_BEB && !defined HAVE_B_BEB
/* CLI to configure Default VLAN for PBBN  */
CLI (nsm_pbb_switchport_beb_default_vlan_ipbb,
   nsm_pbb_switchport_beb_default_vlan_ipbb_cmd,
   "switchport beb vlan <2-4094> (cnp|pip)",
   "Set the switching characteristics of the Layer2 interface",
   "Identifies the interface as one of the backbone port types",
   "Identifies the parameter following is the Default PVID for the port",
   "Default VLAN ID for the port",
   "Default vlan configured on Customer Network Port",
   "Default vlan configured on Provider Instance Port")
{
  int ret;
  nsm_vid_t vid;
  enum nsm_vlan_port_mode mode = NSM_VLAN_PORT_MODE_INVALID;
  enum nsm_vlan_port_mode sub_mode = NSM_VLAN_PORT_MODE_INVALID;
  struct interface *ifp = cli->index;  
  /* To check whether the bridge is configured for switching and vlan aware */
  NSM_BRIDGE_CHECK_L2_PROPERTY(cli, ifp);
  NSM_BRIDGE_CHECK_BRIDGE_PROPERTY(cli, ifp);
   
  CLI_GET_INTEGER_RANGE("Vlan ID", vid, argv[0], NSM_VLAN_CLI_MIN,
                          NSM_VLAN_CLI_MAX);

  /* To set the Port mode */
  if (! pal_strncmp (argv[1], "cnp", 3))
  {
     mode = NSM_VLAN_PORT_MODE_CNP;
     sub_mode = NSM_VLAN_PORT_MODE_HYBRID;
  }
  else 
  {
     mode = NSM_VLAN_PORT_MODE_PIP;
     sub_mode = NSM_VLAN_PORT_MODE_TRUNK;
  }

  /* API to set the default VLAN-ID on the specified port*/
  ret = nsm_vlan_add_beb_port (ifp, vid, mode, sub_mode, 
                                PAL_TRUE, PAL_TRUE);

  if (ret < 0)
    {
      if (ret == NSM_VLAN_ERR_IFP_NOT_BOUND)
        cli_out (cli, "%% Interface not bound to any bridge-group\n");
      else if (ret == NSM_VLAN_ERR_VLAN_NOT_FOUND)
        cli_out (cli, "%% VLAN %d not configured\n", vid);
      else if (ret == NSM_VLAN_ERR_INVALID_MODE)
        cli_out (cli, "%% Port mode not set to valid mode\n");
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
#endif /*HAVE_I_BEB && !defined HAVE_B_BEB*/


#if !defined HAVE_I_BEB && defined HAVE_B_BEB
/* CLI to configure Default VLAN for PBBN  */
CLI (nsm_pbb_switchport_beb_default_vlan_bpbb,
   nsm_pbb_switchport_beb_default_vlan_bpbb_cmd,
   "switchport beb vlan <2-4094> (cbp|pnp)",
   "Set the switching characteristics of the Layer2 interface",
   "Identifies the interface as one of the backbone port types",
   "Identifies the parameter following is the Default PVID for the port",
   "Default VLAN ID for the port",
   "Default vlan configured on Customer Backbone Port",
   "Default vlan configured on Provider Network Port")
{
  int ret;
  nsm_vid_t vid;
  enum nsm_vlan_port_mode mode = NSM_VLAN_PORT_MODE_INVALID;
  enum nsm_vlan_port_mode sub_mode = NSM_VLAN_PORT_MODE_INVALID;
  struct interface *ifp = cli->index;  
  /* To check whether the bridge is configured for switching and vlan aware */
  NSM_BRIDGE_CHECK_L2_PROPERTY(cli, ifp);
  NSM_BRIDGE_CHECK_BRIDGE_PROPERTY(cli, ifp);
   
  CLI_GET_INTEGER_RANGE("Vlan ID", vid, argv[0], NSM_VLAN_CLI_MIN,
                          NSM_VLAN_CLI_MAX);

  /* To set the Port mode */
  if (! pal_strncmp (argv[1], "cbp", 3))
  {
     mode = NSM_VLAN_PORT_MODE_CBP;
     sub_mode = NSM_VLAN_PORT_MODE_TRUNK;
  }
  else 
  {
     mode = NSM_VLAN_PORT_MODE_PNP;
     sub_mode = NSM_VLAN_PORT_MODE_TRUNK;
  }

  /* API to set the default VLAN-ID on the specified port*/
  ret = nsm_vlan_add_beb_port (ifp, vid, mode, sub_mode, 
                               PAL_TRUE, PAL_TRUE);

  if (ret < 0)
    {
      if (ret == NSM_VLAN_ERR_IFP_NOT_BOUND)
        cli_out (cli, "%% Interface not bound to any bridge-group\n");
      else if (ret == NSM_VLAN_ERR_VLAN_NOT_FOUND)
        cli_out (cli, "%% VLAN %d not configured\n", vid);
      else if (ret == NSM_VLAN_ERR_INVALID_MODE)
        cli_out (cli, "%% Port mode not set to valid mode\n");
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
#endif /*!HAVE_I_BEB && HAVE_B_BEB*/

#if defined HAVE_I_BEB && defined HAVE_B_BEB
/* CLI to configure Default VLAN for PBBN  */
CLI (nsm_pbb_switchport_beb_default_vlan_ibpbb,
   nsm_pbb_switchport_beb_default_vlan_ibpbb_cmd,
   "switchport beb vlan <2-4094> (cnp|pip|cbp|pnp)",
   "Set the switching characteristics of the Layer2 interface",
   "Identifies the interface as one of the backbone port types",
   "Identifies the parameter following is the Default PVID for the port",
   "Default VLAN ID for the port",
   "Default vlan configured on Customer Network Port",
   "Default vlan configured on Provider Network Port")
{
  int ret;
  nsm_vid_t vid;
  enum nsm_vlan_port_mode mode = NSM_VLAN_PORT_MODE_INVALID;
  enum nsm_vlan_port_mode sub_mode = NSM_VLAN_PORT_MODE_INVALID;
  struct interface *ifp = cli->index;  
  /* To check whether the bridge is configured for switching and vlan aware */
  NSM_BRIDGE_CHECK_L2_PROPERTY(cli, ifp);
  NSM_BRIDGE_CHECK_BRIDGE_PROPERTY(cli, ifp);
   
  CLI_GET_INTEGER_RANGE("Vlan ID", vid, argv[0], NSM_VLAN_CLI_MIN,
                        NSM_VLAN_CLI_MAX);

  /* To set the Port mode */
  if (! pal_strncmp (argv[1], "cnp", 3))
    {
      mode = NSM_VLAN_PORT_MODE_CNP;
      sub_mode = NSM_VLAN_PORT_MODE_HYBRID;
    }
  else if (argv[1][1] == 'i')
    {
      mode = NSM_VLAN_PORT_MODE_PIP;
      sub_mode = NSM_VLAN_PORT_MODE_TRUNK;
    }
  else if (argv[1][1] == 'b')
    {
      mode = NSM_VLAN_PORT_MODE_CBP;
      sub_mode = NSM_VLAN_PORT_MODE_TRUNK;
    }
  else 
    {
      mode = NSM_VLAN_PORT_MODE_PNP;
      sub_mode = NSM_VLAN_PORT_MODE_TRUNK;
    }

  /* API to set the default VLAN-ID on the specified port*/
  ret = nsm_vlan_add_beb_port (ifp, vid, mode, sub_mode, 
                                PAL_TRUE, PAL_TRUE);

  if (ret < 0)
    {
      if (ret == NSM_VLAN_ERR_IFP_NOT_BOUND)
        cli_out (cli, "%% Interface not bound to any bridge-group\n");
      else if (ret == NSM_VLAN_ERR_VLAN_NOT_FOUND)
        cli_out (cli, "%% VLAN %d not configured\n", vid);
      else if (ret == NSM_VLAN_ERR_INVALID_MODE)
        cli_out (cli, "%% Port mode not set to valid mode\n");
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
#endif /*HAVE_I_BEB && HAVE_B_BEB*/


#if defined HAVE_I_BEB && !defined HAVE_B_BEB
/* CLI to negate the Default VLAN configured for the port */
CLI (no_nsm_pbb_switchport_beb_default_vlan_ipbb,
   no_nsm_pbb_switchport_beb_default_vlan_ipbb_cmd,
   "no switchport beb vlan <2-4094> (cnp |pip)",
   "negate a command or set its default",
   "Set the switching characteristics of the Layer2 interface",
   "Identifies the interface as one of the backbone port types",               
   "Identifies the parameter following is the Default PVID for the port",
   "Default VLAN ID for the port",
   "Default vlan configured on Customer Network Port",
   "Default vlan configured on Provider Instance Port")
{
  int ret;
  nsm_vid_t vid;
  enum nsm_vlan_port_mode mode = NSM_VLAN_PORT_MODE_INVALID;
  enum nsm_vlan_port_mode sub_mode = NSM_VLAN_PORT_MODE_INVALID;
  struct interface *ifp = cli->index;

  /* To check whether the bridge is configured for switching and vlan aware */
  NSM_BRIDGE_CHECK_L2_PROPERTY(cli, ifp);
  NSM_BRIDGE_CHECK_BRIDGE_PROPERTY(cli, ifp);

  CLI_GET_INTEGER_RANGE("Vlan ID", vid, argv[0], NSM_VLAN_CLI_MIN,
                          NSM_VLAN_CLI_MAX);
   /* To check the port mode */
  if (! pal_strncmp (argv[1], "c", 1))
    {
      mode = NSM_VLAN_PORT_MODE_CNP;
      sub_mode = NSM_VLAN_PORT_MODE_HYBRID;
    }
  else
    {
      mode = NSM_VLAN_PORT_MODE_PIP;
      sub_mode = NSM_VLAN_PORT_MODE_TRUNK;
    }

  /* API called to delete the default VLANID on the port */
  ret = nsm_vlan_del_beb_port (ifp, vid, mode, sub_mode,
                               PAL_TRUE, PAL_TRUE);
  return nsm_pbb_cli_return(cli, ret); 
}

#endif /* HAVE_I_BEB && !HAVE_B_BEB */

#if defined HAVE_I_BEB && defined HAVE_B_BEB
/* CLI to negate the Default VLAN configured for the port */
CLI (no_nsm_pbb_switchport_beb_default_vlan_ibpbb,
   no_nsm_pbb_switchport_beb_default_vlan_ibpbb_cmd,
   "no switchport beb vlan <2-4094> (cnp |pnp)",
   "negate a command or set its default",
   "Set the switching characteristics of the Layer2 interface",
   "Identifies the interface as one of the backbone port types",               
   "Identifies the parameter following is the Default PVID for the port",
   "Default VLAN ID for the port",
   "Default vlan configured on Customer Network Port",
   "Default vlan configured on Provider Network Port")
{
  int ret;
  nsm_vid_t vid;
  enum nsm_vlan_port_mode mode = NSM_VLAN_PORT_MODE_INVALID;
  enum nsm_vlan_port_mode sub_mode = NSM_VLAN_PORT_MODE_INVALID;
  struct interface *ifp = cli->index;

  /* To check whether the bridge is configured for switching and vlan aware */
  NSM_BRIDGE_CHECK_L2_PROPERTY(cli, ifp);
  NSM_BRIDGE_CHECK_BRIDGE_PROPERTY(cli, ifp);

  CLI_GET_INTEGER_RANGE("Vlan ID", vid, argv[0], NSM_VLAN_CLI_MIN,
                         NSM_VLAN_CLI_MAX);
   /* To check the port mode */
  if (! pal_strncmp (argv[1], "c", 1))
    {
      mode = NSM_VLAN_PORT_MODE_CNP;
      sub_mode = NSM_VLAN_PORT_MODE_HYBRID;
    }
  else
    {
      mode = NSM_VLAN_PORT_MODE_PNP;
      sub_mode = NSM_VLAN_PORT_MODE_TRUNK;
    }

  /* API called to delete the default VLANID on the port */
  ret = nsm_vlan_del_beb_port (ifp, vid, mode, sub_mode,
                               PAL_TRUE, PAL_TRUE);
  return nsm_pbb_cli_return(cli, ret); 
}

#endif /* HAVE_I_BEB && HAVE_B_BEB */

#if !defined HAVE_I_BEB && defined HAVE_B_BEB
/* CLI to negate the Default VLAN configured for the port */
CLI (no_nsm_pbb_switchport_beb_default_vlan_bpbb,
   no_nsm_pbb_switchport_beb_default_vlan_bpbb_cmd,
   "no switchport beb vlan <2-4094> (cbp |pnp)",
   "negate a command or set its default",
   "Set the switching characteristics of the Layer2 interface",
   "Identifies the interface as one of the backbone port types",               
   "Identifies the parameter following is the Default PVID for the port",
   "Default VLAN ID for the port",
   "Default vlan configured on Customer Backbone Port",
   "Default vlan configured on Provider Network Port")
{
  int ret;
  nsm_vid_t vid;
  enum nsm_vlan_port_mode mode = NSM_VLAN_PORT_MODE_INVALID;
  enum nsm_vlan_port_mode sub_mode = NSM_VLAN_PORT_MODE_INVALID;
  struct interface *ifp = cli->index;

  /* To check whether the bridge is configured for switching and vlan aware */
  NSM_BRIDGE_CHECK_L2_PROPERTY(cli, ifp);
  NSM_BRIDGE_CHECK_BRIDGE_PROPERTY(cli, ifp);

  CLI_GET_INTEGER_RANGE("Vlan ID", vid, argv[0], NSM_VLAN_CLI_MIN,
                         NSM_VLAN_CLI_MAX);
   /* To check the port mode */
  if (! pal_strncmp (argv[1], "c", 1))
    {
      mode = NSM_VLAN_PORT_MODE_CBP;
      sub_mode = NSM_VLAN_PORT_MODE_TRUNK;
    }
  else
    {
      mode = NSM_VLAN_PORT_MODE_PNP;
      sub_mode = NSM_VLAN_PORT_MODE_TRUNK;
    }

  /* API called to delete the default VLANID on the port */
  ret = nsm_vlan_del_beb_port (ifp, vid, mode, sub_mode,
                               PAL_TRUE, PAL_TRUE);
  return nsm_pbb_cli_return(cli, ret); 
}

#endif /* !HAVE_I_BEB && HAVE_B_BEB */


#ifdef HAVE_I_BEB
#ifndef HAVE_B_BEB
/* CLI to configure the CNP with all the SVLANs configured to the same ISID */
CLI (nsm_pbb_cnp_svlan_all_to_instance,
    nsm_pbb_cnp_svlan_all_to_instance_cmd,
    "switchport beb customer-network svlan all (instance "
    "<1-16777214>| name INSTANCE_NAME) egress-port IFNAME",
    "Set the switching characteristics of the Layer2 interface",
    "Identifies this interface as one of the backbone port types",
    "Configures the port as customer network port",
    "The svlan that will configured to the service instance",
    "Allow all vlans to transmit and receive through the Layer-2",
    "Identifies the instance ID that the svlans are mapped to",
    "Service instance ID",
    "ASCII name for the service instance",
    "The ascii name of the service instance",
    "The egress-port for this mapping.",
    "The physical PIP port for this service instance.")
{
  int ret;
  pbb_isid_t isid;
  struct interface *ifp = cli->index;
  struct interface *ifp1 = NULL;
  struct nsm_master *nm = cli->vr->proto;
  struct nsm_bridge_master *master = nsm_bridge_get_master (nm);

  /* To check the ISID range */
  if (argv[0][0] == 'i')
    CLI_GET_INTEGER_RANGE ("ISID value", isid, argv[1], NSM_PBB_ISID_MIN,
                            NSM_PBB_ISID_MAX);
  else
    {
      isid = nsm_pbb_search_isid_by_instance_name (argv[1], master->beb->icomp);
      if (isid == 0)
        {
          cli_out (cli, "instance %s not added \n", argv[1]);
          return CLI_ERROR;
        }
    }

  if ( (ifp1 = if_lookup_by_name(&cli->vr->ifm, argv[2])) == NULL )
    {
      cli_out (cli, "%% Interface not found\n");
      return CLI_ERROR;
    }
  
  /* To check whether the bridge is configured for switching and vlan aware */
  NSM_BRIDGE_CHECK_L2_PROPERTY(cli, ifp);
  NSM_BRIDGE_CHECK_BRIDGE_PROPERTY(cli, ifp);

  /* API to map svlan to ISID and map ISID to port PIP/PNP */
  ret = nsm_vlan_port_beb_add_cnp_svlan_based (ifp, ifp1, isid, NULL, 
                                               NSM_VLAN_CLI_MIN, 
                                               NSM_VLAN_CLI_MAX);
  return nsm_pbb_cli_return(cli, ret);
}

CLI (nsm_pbb_cnp_add_svlan_range_to_instance,
    nsm_pbb_cnp_add_svlan_range_to_instance_cmd,
    "switchport beb customer-network svlan add "
    "(range <2-4094> <2-4094> |"
    " <2-4094>) (instance <1-16777214>| name INSTANCE_NAME) egress-port IFNAME",
    "Set the switching characteristics of the Layer2 interface",
    "Identifies this interface as one of the backbone port types",
    "Configures the port as customer network port",
    "The svlan that will configured to the service instance",
    "Add SVLAN to the member set of ISID",
    "specifies the range of SVLANS that would be added",
    "Starting SVLANID",
    "Ending SVLANID in the specified range",
    "Specify one SVLAN ID that will be mapped to ISID",
    "Identifies the instance that the svlans are mapped to",
    "Service instance ID",
    "ASCII name for the service instance",
    "The ascii name of the service instance",
    "The egress-port for this mapping.",
    "The physical PIP port for this service instance.")

{
  int ret;
  pbb_isid_t isid;
  nsm_vid_t start_svid;
  nsm_vid_t end_svid;
  struct interface *ifp = cli->index;
  struct interface *ifp1 = NULL;
  struct nsm_master *nm = cli->vr->proto;
  struct nsm_bridge_master *master = nsm_bridge_get_master (nm);

  /* To check whether the bridge is configured for switching and vlan aware */
  NSM_BRIDGE_CHECK_L2_PROPERTY(cli, ifp);
  NSM_BRIDGE_CHECK_BRIDGE_PROPERTY(cli, ifp);

  if (!pal_strncmp (argv[0],"r",1))
    {
      /* To check the ISID range */
      if (argv[3][0] == 'i')
        CLI_GET_INTEGER_RANGE ("ISID value", isid, argv[4], NSM_PBB_ISID_MIN,
                                NSM_PBB_ISID_MAX);
      else
        {
          isid = nsm_pbb_search_isid_by_instance_name (argv[4], master->beb->icomp);
          if (isid == 0)
            {
              cli_out (cli, "instance %s not added \n", argv[4]);
              return CLI_ERROR;
            }
        }

      /* To check the VLANID range */
      CLI_GET_INTEGER_RANGE ("Starting Svlan ID value", start_svid, argv[1],
                             2, 4094);
      CLI_GET_INTEGER_RANGE ("Ending Svlan ID value", end_svid, argv[2],
                             2, 4094);


      if ((ifp1 = if_lookup_by_name (&cli->vr->ifm, argv[5])) == NULL)
        {
          cli_out (cli, "%% Interface not found\n");
          return CLI_ERROR;
        }
      /* API to map svlan to ISID and map ISID to port PIP/PNP */
      ret = nsm_vlan_port_beb_add_cnp_svlan_based (ifp, ifp1, isid, NULL,
                                                   start_svid, end_svid);

    }
  else
    {
      /* To check the ISID range */
      if (argv[1][0] == 'i')
        CLI_GET_INTEGER_RANGE ("ISID value", isid, argv[2], 1, 16777214);
      else
        {
          isid = nsm_pbb_search_isid_by_instance_name (argv[2],
              master->beb->icomp);

          if (isid == 0)
            {
              cli_out (cli, "instance %s not added \n", argv[2]);
              return CLI_ERROR;
            }

        }
      /* To check the range of VLANID */
      CLI_GET_INTEGER_RANGE ("Svlan ID value", start_svid, argv[0], 2, 4094);

      /* When end_svid = start_svid then single svid is mapped */
      end_svid = start_svid;

      if ((ifp1 = if_lookup_by_name (&cli->vr->ifm, argv[3])) == NULL)
        {
          cli_out (cli, "%% Interface not found\n");
          return CLI_ERROR;
        }

      /* API to map svlan to ISID and map ISID to port PIP/PNP */
      ret = nsm_vlan_port_beb_add_cnp_svlan_based (ifp, ifp1, isid, NULL,
                                                   start_svid, end_svid);

    }
  return nsm_pbb_cli_return(cli, ret);
}

#else

CLI (nsm_pbb_cnp_svlan_all_to_instance,
    nsm_pbb_cnp_svlan_all_to_instance_cmd,
    "switchport beb customer-network svlan all (instance "
    "<1-16777214>| name INSTANCE_NAME)",
    "Set the switching characteristics of the Layer2 interface",
    "Identifies this interface as one of the backbone port types",
    "Configures the port as customer network port",
    "The svlan that will configured to the service instance",
    "Allow all vlans to transmit and receive through the Layer-2",
    "Identifies the instance ID that the svlans are mapped to",
    "Service instance ID",
    "ASCII name for the service instance",
    "The name for the service instance")
{
  int ret;
  pbb_isid_t isid;
  struct interface *ifp = cli->index;
  struct nsm_master *nm = cli->vr->proto;
  struct nsm_bridge_master *master = nsm_bridge_get_master (nm);

  /* To check the range of the ISID */

      if (argv[0][0] == 'i')
        CLI_GET_INTEGER_RANGE ("ISID value", isid, argv[1], NSM_PBB_ISID_MIN,
                                NSM_PBB_ISID_MAX);
      else
        {
          isid = nsm_pbb_search_isid_by_instance_name (argv[1], master->beb->icomp);
          if (isid == 0)
            {
              cli_out (cli, "instance %s not added \n", argv[1]);
              return CLI_ERROR;
            }
        }

  /* To check whether the bridge is configured for switching and vlan aware */
  NSM_BRIDGE_CHECK_L2_PROPERTY(cli, ifp);
  NSM_BRIDGE_CHECK_BRIDGE_PROPERTY(cli, ifp);

  /* API to map svlan to ISID and map ISID to port PIP/PNP */
  ret = nsm_vlan_port_beb_add_cnp_svlan_based (ifp, NULL, isid, NULL,
                                               NSM_VLAN_CLI_MIN,
                                               NSM_VLAN_CLI_MAX);
  return nsm_pbb_cli_return(cli, ret);
}

/* CLI to configure CNP to add/remove specified SVLANs to ISID */
CLI (nsm_pbb_cnp_add_svlan_range_to_instance,
    nsm_pbb_cnp_add_svlan_range_to_instance_cmd,
    "switchport beb customer-network svlan add "
    "(range <2-4094> <2-4094> |" 
    " <2-4094>) (instance <1-1677214> |name INSTANCE_NAME)",
    "Set the switching characteristics of the Layer2 interface",
    "Identifies this interface as one of the backbone port types",
    "Configures the port as customer network port",
    "The svlan that will configured to the service instance",
    "Add SVLAN to the member set of ISID",
    "specifies the range of SVLANS that would be added",
    "Starting SVLANID",
    "Ending SVLANID in the specified range",
    "Specify one SVLAN ID that will be mapped to ISID",
    "Identifies the instance that the svlans are mapped to",
    "Service instance ID",
    "ASCII name for the service instance",
    "The ascii name of the service instance")
{
  int ret;
  pbb_isid_t isid;
  nsm_vid_t start_svid;
  nsm_vid_t end_svid;
  struct interface *ifp = cli->index;
  struct nsm_master *nm = cli->vr->proto;
  struct nsm_bridge_master *master = nsm_bridge_get_master (nm);

  /* To check whether the bridge is configured for switching and vlan aware */
  NSM_BRIDGE_CHECK_L2_PROPERTY(cli, ifp);
  NSM_BRIDGE_CHECK_BRIDGE_PROPERTY(cli, ifp);

  if (!pal_strncmp (argv[0],"r",1))
    {
    /* To check the range of the ISID */

      if (argv[3][0] == 'i')
        CLI_GET_INTEGER_RANGE ("ISID value", isid, argv[4], NSM_PBB_ISID_MIN,
                                NSM_PBB_ISID_MAX);
      else
        {
          isid = nsm_pbb_search_isid_by_instance_name (argv[4], master->beb->icomp);
          if (isid == 0)
            {
              cli_out (cli, "instance %s not added \n", argv[4]);
              return CLI_ERROR;
            }
        }

      /* To check the VLANID range */
      CLI_GET_INTEGER_RANGE ("Starting Svlan ID value", start_svid, argv[1], 
                             2, 4094);
      CLI_GET_INTEGER_RANGE ("Ending Svlan ID value", end_svid, argv[2], 
                             2, 4094);

      if (start_svid >end_svid)
        {
          cli_out (cli, "vlan range not correct\n");
        }

      /* API to map svlan to ISID and map ISID to port PIP/PNP */
      ret = nsm_vlan_port_beb_add_cnp_svlan_based (ifp, NULL, isid, NULL, 
                                                   start_svid, end_svid);

    }
  else
    { 
      if (argv[1][0] == 'i')
        CLI_GET_INTEGER_RANGE ("ISID value", isid, argv[2], NSM_PBB_ISID_MIN,
                                NSM_PBB_ISID_MAX);
      else
        {
          isid = nsm_pbb_search_isid_by_instance_name (argv[2], master->beb->icomp);
          if (isid == 0)
            {
              cli_out (cli, "instance %s not added \n", argv[2]);
              return CLI_ERROR;
            }
        }

      /* To check the range of VLANID */
      CLI_GET_INTEGER_RANGE ("Svlan ID value", start_svid, argv[0], 2, 4094);

      /* When end_svid = start_svid then single svid is mapped */
      end_svid = start_svid; 

      /* API to map svlan to ISID and map ISID to port PIP/PNP */
      ret = nsm_vlan_port_beb_add_cnp_svlan_based (ifp, NULL, isid, NULL, 
                                                   start_svid, end_svid);
    }
  return nsm_pbb_cli_return(cli, ret);
}
#endif /* HAVE_B_BEB */
CLI (nsm_pbb_cnp_remove_svlan_range_to_instance,
    nsm_pbb_cnp_remove_svlan_range_to_instance_cmd,
    "switchport beb customer-network svlan remove "
    "(range <2-4094> <2-4094> |"
    " <2-4094>) (instance <1-1677214>| name INSTANCE_NAME)",
    "Set the switching characteristics of the Layer2 interface",
    "Identifies this interface as one of the backbone port types",
    "Configures the port as customer network port",
    "The svlan that will configured to the service instance",
    "Remove SVLAN from the member set of ISID",
    "specifies the range of SVLANS that would be removed",
    "Starting SVLANID",
    "to",
    "Ending SVLANID in the specified range",
    "Specify one SVLAN ID that will be to ISID",
    "Service VLANID to unmap",
    "Identifies the instance that the svlans are mapped to",
    "Service instance ID")
{
  int ret;
  pbb_isid_t isid;
  nsm_vid_t start_svid;
  nsm_vid_t end_svid;
  struct interface *ifp = cli->index;
  struct nsm_master *nm = cli->vr->proto;
  struct nsm_bridge_master *master = nsm_bridge_get_master (nm);


  /* To check whether the bridge is configured for switching and vlan aware */
  NSM_BRIDGE_CHECK_L2_PROPERTY(cli, ifp);
  NSM_BRIDGE_CHECK_BRIDGE_PROPERTY(cli, ifp);
  
  if (!pal_strncmp (argv[0], "r", 1))
    {
      if (argv[3][0] == 'i')
        CLI_GET_INTEGER_RANGE ("ISID value", isid, argv[4], 1, 16777214);
      else
        {
          isid = nsm_pbb_search_isid_by_instance_name (argv[4], master->beb->icomp);
          if (isid == 0)
            {
              cli_out (cli, "instance %s not added \n", argv[4]);
              return CLI_ERROR;
            }

        }
      CLI_GET_INTEGER_RANGE ("Starting SVID value", start_svid, argv[1], 
                             2, 4094);
      CLI_GET_INTEGER_RANGE ("Ending Svlan ID value", end_svid, argv[2], 
                             2, 4094);

      if (start_svid > end_svid)
        {
          cli_out(cli,"%% The end_svid in the range is not properly "
                  "mentioned\n");
          return CLI_ERROR;
        }
      /* API to delete the svlan mapping to the ISID */

      ret = nsm_vlan_port_beb_del_cnp_svlan(ifp, isid, start_svid, 
                                                   end_svid);

    }
  else
    {
      if (argv[1][0] == 'i')
        CLI_GET_INTEGER_RANGE ("ISID value", isid, argv[2], 1, 16777214);
      else
        {
          isid = nsm_pbb_search_isid_by_instance_name (argv[2], master->beb->icomp);
          if (isid == 0)
            {
              cli_out (cli, "instance %s not added \n", argv[2]);
              return CLI_ERROR;
            }

        }
      CLI_GET_INTEGER_RANGE ("Svlan ID value", start_svid, argv[0], 2, 4094);

      /* When end_svid = start_svid then single svid is mapped */
      end_svid = start_svid; 

      /* API to delete the svlan mapping to the ISID */
      ret = nsm_vlan_port_beb_del_cnp_svlan (ifp, isid, start_svid, 
                                                   end_svid);

    }

  return nsm_pbb_cli_return(cli, ret);
}

/* CLI to negate the configuration */
CLI (no_nsm_pbb_switchport_cnp_svlan_to_instance,
   no_nsm_pbb_switchport_cnp_svlan_to_instance_cmd,
   "no switchport beb customer-network svlan (instance <1-16777214>|name INSTANCE_NAME)",
   "negate a command or set its defaults",
   "Reset the switching characteristics of the Layer2 interface",
   "Identifies this interface as one of the backbone port types",
   "Configures the port as customer network port",
   "Identifies the svlan's configured to the instance",
   "Identifies the instance that the svlans are mapped to",
   "Service instance ID")
{
  int ret;
  pbb_isid_t isid;
  struct interface *ifp = cli->index;
  struct nsm_master *nm = cli->vr->proto;
  struct nsm_bridge_master *master = nsm_bridge_get_master (nm);

  /* To check whether the bridge is configured for switching and vlan aware */ 
  NSM_BRIDGE_CHECK_L2_PROPERTY(cli, ifp);
  NSM_BRIDGE_CHECK_BRIDGE_PROPERTY(cli, ifp);
  
  /* To check the ISID range */
  if (argv[0][0] == 'i')
    CLI_GET_INTEGER_RANGE ("ISID value", isid, argv[1], NSM_PBB_ISID_MIN,
                            NSM_PBB_ISID_MAX);
  else
    {
      isid = nsm_pbb_search_isid_by_instance_name (argv[1], master->beb->icomp);
      if (isid == 0)
        {
          cli_out (cli, "instance %s not added \n", argv[1]);
          return CLI_ERROR;
        }
    }

  /* API to delete SVLAN based CNP and the ISID  */
  ret = nsm_vlan_port_beb_del_cnp_svlan_based (ifp, isid);
 
  return nsm_pbb_cli_return(cli, ret);
}
#endif /* HAVE_I_BEB */

#ifdef HAVE_B_BEB    
/*  CLI to map ISID to BVID on CBP Port */
CLI(nsm_pbb_switchport_cbp_instance_to_bvid,
   nsm_pbb_switchport_cbp_instance_to_bvid_cmd,
   "switchport beb customer-backbone instance add <1-16777214>"
   " bvlan <2-4094> (| multipoint) (|default-da MAC) "
   "(|instance-map <1-16777214>) (|edge)",
   "Set the switching characteristics of the Layer2 interface",
   "Identifies this interface as one of the backbone port types",
   "Identifies the port as CBP as per 802.1ah standard",
   "Instance that will be mapped to Backbone VLAN",
   "Add service instance to backbone B-VLAN mapping",
   "The Service Instance ID",
   "The Backbone B-VLAN for the ISID mapping",
   "B-VLAN ID",
   "Indicates multipoint ingress/egress limiting setting",
   "To configure a default destination MAC address for the ingress frame"
   " or the broadcast MAC address is used",
   "A backbone destination MAC address in the HHHH.HHHH.HHHH format",
   "1:1 ingress to PBBN instance ID mapping",
   "Service Instance ID",
   "indicates that this CBP is acting as an I-tagged edge interface (E-NNI).")
{
  int ret;
  nsm_vid_t bvid = 0;
  pbb_isid_t isid = NSM_PBB_ISID_NONE;
  pbb_isid_t isid_map = NSM_PBB_ISID_NONE;
  u_int8_t edge = 0;
  u_int8_t default_mac[ETHER_ADDR_LEN];
  u_int16_t vlan_type = NSM_BVLAN_P2P;
  struct interface *ifp = cli->index;
  
  /* To check the range of ISID */
  CLI_GET_INTEGER_RANGE ("Service Instance Id", isid, argv[0], 
                         NSM_PBB_ISID_MIN,NSM_PBB_ISID_MAX);  
  /* To check the range of BVLANID */
  CLI_GET_INTEGER_RANGE ("BVLAN Id", bvid, argv[1], NSM_VLAN_CLI_MIN, 
                         NSM_VLAN_CLI_MAX);

  NSM_BRIDGE_CHECK_L2_PROPERTY(cli, ifp);
  NSM_BRIDGE_CHECK_BRIDGE_PROPERTY(cli, ifp);

  /* Setting default BMAC to all FF's  */ 
  /* To check if bvid is added to ISID */
  if (argc == 2)
    { 
      /* API to add map ISID to bvid on CBP and add default-da and ISID-mapping */
      ret = nsm_vlan_port_beb_add_cbp_srv_inst (ifp, isid, NSM_PBB_ISID_NONE,
                                                bvid, vlan_type,
                                                NULL, PAL_FALSE);
      return nsm_pbb_cli_return(cli, ret);
    } 
  /* to check if multicast is enabled */ 
  if (!pal_strncmp(argv[2], "m", 1))
    { 
      vlan_type = NSM_BVLAN_M2M;
      if (argc == 8)
          edge = 1;
      if (argc > 3 && argv[3][0]=='d')         
       {
         if (! nsm_api_mac_address_is_valid (argv[4]))
          {
            cli_out (cli, "%% Unable to translate mac address %s\n", argv[4]);
            return CLI_ERROR;
          } 

         NSM_RESERVED_ADDRESS_CHECK (argv[4]);
         NSM_READ_MAC_ADDRESS (argv[4], default_mac);

        /* Convert to network order */
        *(unsigned short *)&default_mac[0] = pal_hton16(*(unsigned short *)&default_mac[0]);
        *(unsigned short *)&default_mac[2] = pal_hton16(*(unsigned short *)&default_mac[2]);
        *(unsigned short *)&default_mac[4] = pal_hton16(*(unsigned short *)&default_mac[4]);
        
         if (argc > 5 && argv[5][0]=='i')
         {
           CLI_GET_INTEGER_RANGE ("Instance-mapping ISID", isid_map, argv[6],
                                 NSM_PBB_ISID_MIN,NSM_PBB_ISID_MAX);
         }
         else if (argc > 5 && argv[5][0]=='e')
           edge = 1;
         

           /* API to add map ISID to bvid on CBP and add default-da 
              and ISID-mapping */
         ret = nsm_vlan_port_beb_add_cbp_srv_inst (ifp, isid, isid_map,
                                                    bvid, vlan_type,
                                                    default_mac, edge);
         return nsm_pbb_cli_return(cli, ret);
       } 
      else if (argc == 5 || argc == 6)
       { 
         if (argc == 6)
           edge = 1;
         if (!pal_strncmp(argv[3], "d", 1))
          {
             if (! nsm_api_mac_address_is_valid (argv[4]))
              {
                cli_out (cli, "%% Unable to translate mac address %s\n", argv[4]);
                return CLI_ERROR;
              }

            NSM_RESERVED_ADDRESS_CHECK (argv[4]);
            NSM_READ_MAC_ADDRESS (argv[4], default_mac);

          /* Convert to network order */
           *(unsigned short *)&default_mac[0] = pal_hton16(*(unsigned short *)&default_mac[0]);
           *(unsigned short *)&default_mac[2] = pal_hton16(*(unsigned short *)&default_mac[2]);
           *(unsigned short *)&default_mac[4] = pal_hton16(*(unsigned short *)&default_mac[4]);



            /* API to add map ISID to bvid on CBP and add default-da
               and ISID-mapping */      
           ret = nsm_vlan_port_beb_add_cbp_srv_inst (ifp, isid, 
                                                     NSM_PBB_ISID_NONE, 
                                                     bvid, vlan_type, 
                                                     default_mac, edge);
            return nsm_pbb_cli_return(cli, ret);
          }
        else
          {
            CLI_GET_INTEGER_RANGE ("Instance-mapping ISId", isid_map, argv[4],
                                   NSM_PBB_ISID_MIN,NSM_PBB_ISID_MAX); 
            /* API to add map ISID to bvid on CBP and add default-da
               and ISID-mapping */
            ret = nsm_vlan_port_beb_add_cbp_srv_inst (ifp, isid, isid_map,
                                                      bvid, vlan_type,
                                                      NULL, edge);
            return nsm_pbb_cli_return(cli, ret); 
          }
      }
    else
      {
        if (argc == 4)
          edge = 1;
        /* API to add map ISID to bvid on CBP and add default-da
           and ISID-mapping */
        ret = nsm_vlan_port_beb_add_cbp_srv_inst (ifp, isid, NSM_PBB_ISID_NONE,
                                                  bvid, vlan_type,
                                                  NULL, edge);
        return nsm_pbb_cli_return(cli, ret);
      }
    }     
  /* To check if default-da is given without multicast */
  else if (!pal_strncmp(argv[2], "d", 1))
    {
      vlan_type = NSM_BVLAN_P2P;

      if (! nsm_api_mac_address_is_valid (argv[3]))
        {
          cli_out (cli, "%% Unable to translate mac address %s\n", argv[3]);
          return CLI_ERROR;
        }

        NSM_RESERVED_ADDRESS_CHECK (argv[3]);
        NSM_READ_MAC_ADDRESS (argv[3], default_mac);

       /* Convert to network order */
        *(unsigned short *)&default_mac[0] = pal_hton16(*(unsigned short *)&default_mac[0]);
        *(unsigned short *)&default_mac[2] = pal_hton16(*(unsigned short *)&default_mac[2]);
        *(unsigned short *)&default_mac[4] = pal_hton16(*(unsigned short *)&default_mac[4]);

      if (argc >= 6)
        {
          if (argc == 7)
            edge = 1;

          CLI_GET_INTEGER_RANGE ("Instance-mapping ISID", isid_map, argv[5],
                                 NSM_PBB_ISID_MIN,NSM_PBB_ISID_MAX);
          /* API to add map ISID to bvid on CBP and add default-da
             and ISID-mapping */
          ret = nsm_vlan_port_beb_add_cbp_srv_inst (ifp, isid, isid_map,
                                                    bvid, vlan_type,
                                                    default_mac, edge);
          return nsm_pbb_cli_return(cli, ret);
        }
      else 
        {
          if (argc == 5)
            edge = 1;
          /* API to add map ISID to bvid on CBP and add default-da
             and ISID-mapping */
          ret = nsm_vlan_port_beb_add_cbp_srv_inst (ifp, isid, 
                                                    NSM_PBB_ISID_NONE,
                                                    bvid, vlan_type,
                                                    default_mac, edge);
          return nsm_pbb_cli_return(cli, ret);
        }
    }
  /* To check if only instance-mapping ISID is given */
  else if (!pal_strncmp(argv[2], "i", 1))
    {
      vlan_type = NSM_BVLAN_P2P;
      CLI_GET_INTEGER_RANGE ("Instance-mapping ISID", isid_map, argv[3],
                             NSM_PBB_ISID_MIN,NSM_PBB_ISID_MAX); 
      if (argc == 5)
        edge = 1;
      /* API to add map ISID to bvid on CBP and add default-da
         and ISID-mapping */
      ret = nsm_vlan_port_beb_add_cbp_srv_inst (ifp, isid, isid_map,
                                                bvid, vlan_type,
                                                NULL, edge);
      return nsm_pbb_cli_return(cli, ret);
    }
  else
    {
      vlan_type = NSM_BVLAN_P2P;
      edge = 1;
      /* API to add map ISID to bvid on CBP and add default-da
         and ISID-mapping */
      ret = nsm_vlan_port_beb_add_cbp_srv_inst (ifp, isid, NSM_PBB_ISID_NONE,
                                                bvid, vlan_type,
                                                NULL, edge);
    }
  return nsm_pbb_cli_return(cli, ret);
}

/* CLI to remove ISID-to-BVID mapping */
CLI (nsm_pbb_switchport_cbp_del_or_none_instance,
   nsm_pbb_switchport_cbp_del_or_none_instance_cmd,
   "switchport beb customer-backbone instance (none | (delete <1-16777214>))",
   "Set the switching characteristics of the Layer2 interface",
   "Identifies this interface as one of the backbone port types",
   "Identifies the port as CBP as per 802.1ah standard",
   "Instance that will be mapped to Backbone VLAN",
   "Allows no service interface on this CBP port",
   "Delete the specified service instance on this CBP port",
   "Service Instance ID.")
{
  int ret;
  pbb_isid_t isid;
  struct interface *ifp = cli->index;   

  NSM_BRIDGE_CHECK_L2_PROPERTY(cli, ifp);
  NSM_BRIDGE_CHECK_BRIDGE_PROPERTY(cli, ifp);

  if (argc == 2)
    { 
      /* To check the range of ISID */
      CLI_GET_INTEGER_RANGE ("Service Instance Id", isid, argv[1], 
                             NSM_PBB_ISID_MIN, NSM_PBB_ISID_MAX);
      /* API to delete ISID mapping on CBP */
      ret = nsm_vlan_port_beb_delete_cbp_srv_inst(ifp, isid);
    }
  else
    { 
      /* API to delete ISID mapping on CBP */
      ret = nsm_vlan_port_beb_delete_cbp_srv_inst(ifp, NSM_PBB_ISID_NONE);
    }

  return nsm_pbb_cli_return(cli, ret);
}

/* CLI to add/remove BVLAN to PNP Port */
CLI (nsm_pbb_switchport_beb_provider_network_to_bvlan,
   nsm_pbb_switchport_beb_provider_network_to_bvlan_cmd,
   "switchport beb provider-network bvlan (none | all |((add | remove) <2-4094>))",
   "Set the switching characteristics of the Layer2 interface",
   "Identifies this interface as one of the backbone port types",
   "Set the Layer-2 interface as Provider Network Port"
   "as per 802.1ah standard.",
   "Identifies the BVLAN's that need to be added/removed",
   "Remove all VLAN association from this PNP port.",
   "Associate all VLAN to this PNP port.",
   "Add a VLAN to this PNP port.",
   "Remove a VLAN from this PNP port.",
   "The VID of the B-VLAN.")
{ 
  int ret = 0;
  nsm_vid_t bvid=0;
  struct interface *ifp = cli->index;

  NSM_BRIDGE_CHECK_L2_PROPERTY(cli, ifp);
  NSM_BRIDGE_CHECK_BRIDGE_PROPERTY(cli, ifp);
  
  if (argc == 2)
    {
      /* To check the range of the BVID */
      CLI_GET_INTEGER_RANGE ("BVLAN Id", bvid, argv[1], NSM_VLAN_CLI_MIN, 
                             NSM_VLAN_CLI_MAX);
    }
  
  if (!pal_strncmp(argv[0], "add", 3))
    { 
     /* API to add bvid on PNP */ 
      ret = nsm_vlan_port_add_pnp (ifp, bvid);
    }
  else if (!pal_strncmp(argv[0], "r", 1))
    {
     /* API to del the bvid on PNP */
      ret = nsm_vlan_port_del_pnp(ifp, bvid);
    }
  else
    { 
      if ( !pal_strncmp(argv[0], "n", 1))
        ret = nsm_vlan_port_add_pnp(ifp, NSM_VLAN_NONE);
      else if (!pal_strncmp(argv[0], "all", 3))
        ret = nsm_vlan_port_add_pnp(ifp, NSM_VLAN_ALL);
    }
  if (ret < 0)
    {
      if (ret == NSM_VLAN_ERR_BRIDGE_NOT_FOUND)
        {
          cli_out (cli, "%% Interface not bound to any bridge-group\n");
          return CLI_ERROR;
        }
      else if (ret == NSM_VLAN_ERR_INVALID_MODE)
        {
          cli_out (cli, "%% Port mode not set to valid type\n");
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
                   ifp->name, bvid);
          return CLI_ERROR;
        }
      else
        {
          cli_out (cli, "%% Can't add VID %d to a port: %s\n", bvid, pal_strerror (ret));
          return  CLI_ERROR;
        }
      }
   return CLI_SUCCESS;
}

#endif /* HAVE_B_BEB */

#if (defined HAVE_I_BEB) && (defined HAVE_B_BEB)
CLI (nsm_pbb_switchport_beb_pip_mac,
    nsm_pbb_switchport_beb_pip_mac_cmd,
    "switchport beb pip backbone-source-mac MAC",
    "Set the switching characteristics of the Layer2 interface",
    "Identifies this interface as one of the backbone port types",
    "Identifies that the entered parameter is for PIP port",
    "Configures a default backbone source mac address for this PIP interface",
    "A Backbone Source MAC address HHHH.HHHH.HHHH format")
{
  u_int8_t backbone_src_mac[ETHER_ADDR_LEN] ;
  int ret = 0;    
  struct interface *ifp = cli->index;

  NSM_BRIDGE_CHECK_L2_PROPERTY(cli, ifp);
  NSM_BRIDGE_CHECK_BRIDGE_PROPERTY(cli, ifp);

  if (! nsm_api_mac_address_is_valid (argv[0]))
    {
      cli_out (cli, "%% Unable to translate mac address %s\n", argv[0]);
        return CLI_ERROR;
    } 

  NSM_RESERVED_ADDRESS_CHECK (argv[0]);
  NSM_READ_MAC_ADDRESS (argv[0], backbone_src_mac);

  /* Convert to network order */
  *(unsigned short *)&backbone_src_mac[0] = pal_hton16(*(unsigned short *)
                                                       &backbone_src_mac[0]);
  *(unsigned short *)&backbone_src_mac[2] = pal_hton16(*(unsigned short *)
                                                       &backbone_src_mac[2]);
  *(unsigned short *)&backbone_src_mac[4] = pal_hton16(*(unsigned short *)
                                                       &backbone_src_mac[4]);

  ret = nsm_vlan_port_beb_pip_mac_update (ifp, backbone_src_mac);
  return CLI_SUCCESS; 
}
#endif /* HAVE_I_BEB && HAVE_B_BEB */

#ifdef HAVE_I_BEB 
/* CLI to configure VIP */
CLI (nsm_pbb_switchport_beb_vip_instance,
    nsm_pbb_switchport_beb_vip_instance_cmd,
    "switchport beb vip (name WORD | instance <1-16777214>) "
    "(default-da MAC | (allowed (ingress | egress | all)) )",
    "Set the switching characteristics of the Layer2 interface",
    "Identifies this interface as one of the backbone port types",
    "Identifies a Virtual Instance Port for a PIP port",
    "Configures a name string for the Virtual Instance Port",
    "The name string for the VIP",
    "Configures the instance id associated with the virtual instance port",
    "The Instance ID",
    "Configures a default destination MAC address for the ingress frame when "
    "not specified, the broadcast MAC address is used.",
    "A Backbone Destination MAC address HHHH.HHHH.HHHH format",
    "Configures the acceptable service flow direction",
    "Configures ingress service flow",
    "Configures egress service flow",
    "Ingress/egress allowed.")
{
  int ret;
  pbb_isid_t isid = NSM_PBB_ISID_NONE;
  char *name = NULL;
  u_int8_t default_mac[ETHER_ADDR_LEN] ;
  enum nsm_port_service_type egress_type = -1;
  struct interface *ifp = cli->index; 

  NSM_BRIDGE_CHECK_L2_PROPERTY(cli, ifp);
  NSM_BRIDGE_CHECK_BRIDGE_PROPERTY(cli, ifp);

  if (!pal_strncmp (argv[0], "n", 1))
    {
      /* API to configure VIP for default-da and ingress/egress service flow */
      name = argv[1];
    }
  else if (!pal_strncmp (argv[0], "i", 1))
    {
      CLI_GET_INTEGER_RANGE("Service ISID",isid,argv[1], 1, 16777214);
      /* API to configure VIP for default-da and ingress/egress service flow */
    }
  if (!pal_strncmp (argv[2], "d", 1))
    {
       if (! nsm_api_mac_address_is_valid (argv[3]))
        {
          cli_out (cli, "%% Unable to translate mac address %s\n", argv[3]);
          return CLI_ERROR;
        }

      NSM_RESERVED_ADDRESS_CHECK (argv[3]);
      NSM_READ_MAC_ADDRESS (argv[3], default_mac);
 
     /* Convert to network order */
     *(unsigned short *)&default_mac[0] = pal_hton16(*(unsigned short *)&default_mac[0]);
     *(unsigned short *)&default_mac[2] = pal_hton16(*(unsigned short *)&default_mac[2]);
     *(unsigned short *)&default_mac[4] = pal_hton16(*(unsigned short *)&default_mac[4]);
      
      /* API to configure VIP for default-da and ingress/egress service flow */
      ret = nsm_vlan_port_config_vip(ifp, isid, name, 
                                     default_mac, egress_type);
    }
  else
    {
      if (!pal_strncmp (argv[3], "i", 1))
        {
          egress_type = NSM_PORT_SERVICE_INGRESS;
        }
      else if (!pal_strncmp (argv[3], "e", 1))
        {  
          egress_type = NSM_PORT_SERVICE_EGRESS;
        }
      else
        {
          egress_type = NSM_PORT_SERVICE_BOTH;
        }
          /* API to configure VIP for default-da and ingress/egress
           * service flow */
        ret = nsm_vlan_port_config_vip(ifp, isid, name,
                                       NULL, egress_type);
    }
  
  return nsm_pbb_cli_return(cli, ret);
}
#endif /* HAVE_I_BEB */

#if defined (HAVE_I_BEB) || defined (HAVE_B_BEB)
/* CLI to dispatch Service */
CLI (nsm_pbb_beb_dispatch_instance,
    nsm_pbb_beb_dispatch_instance_cmd,
    "switchport beb (check | dispatch | remove) service "
    "(name WORD | <1-16777214>)",
    "Set the switching characteristics of the Layer2 interface",
    "Identifies this interface as one of the backbone port types",
    "checks the configuration sanity of the specified service instance",
    "Dispatch the specified service instance to forwarding plane",
    "Remove a previously dispatched service instance from forwarding plane",
    "Specifies the service name or ISID to be checked or dispatched "
    "or removed from the forwading plane",
    "Name string to identify the configured service instance",
    "The name string for the virtual instance port",
    "The instance id to identify the service instance.")
{
  int ret;
  pbb_isid_t isid;
  struct interface *ifp = cli->index;

  NSM_BRIDGE_CHECK_L2_PROPERTY(cli, ifp);
  NSM_BRIDGE_CHECK_BRIDGE_PROPERTY(cli, ifp);
  
  if (argc == 3)
    { 
      if (!pal_strncmp ("c", argv[0], 1))
        {
          /* API to check the service instance status*/
          ret = nsm_pbb_check_service (ifp, NSM_PBB_ISID_NONE, argv[2]);
        }  
      else if (!pal_strncmp ("d", argv[0], 1))
        {
          /* API to dispatch the service instance to HAL*/
          ret = nsm_pbb_dispatch_service(ifp, NSM_PBB_ISID_NONE, argv[2]);
        }
      else
        {
          /* API to remove the service instance from HAL*/
          ret = nsm_pbb_remove_service(ifp, NSM_PBB_ISID_NONE, argv[2]);
        }
    }
  else
    {
      CLI_GET_INTEGER_RANGE ("Service Instance id", isid, argv[1], 
                             NSM_PBB_ISID_MIN, NSM_PBB_ISID_MAX);
      if (!pal_strncmp ("c", argv[0], 1))
        {
          /* API to check the service instance status */
          ret = nsm_pbb_check_service (ifp, isid, NULL);
        }
      else if (!pal_strncmp ("d", argv[0], 1))
        {
          /* API to dispatch the service instance to HAL*/
          ret = nsm_pbb_dispatch_service(ifp, isid, NULL);
        }
      else
        {
          /* API to remove the service instance from HAL*/
          ret = nsm_pbb_remove_service(ifp, isid, NULL);
        }
    }
  return nsm_pbb_cli_return(cli, ret);
}

/* CLI to show the configuration of Bridge */
CLI (nsm_show_pbb_beb_bridge,
   nsm_show_pbb_beb_bridge_cmd,
   "show beb bridge ((<1-32> ( |cnp | vip | pip | vlan ))"
   "| (backbone (|cbp | pnp | vlan )))",
   "show",
   "Information related to the backbone network",
   "Display forwarding information",
   "Indicates the I-comp bridge number.",
   "show customer network port.",
   "show virtual instance port.",
   "show provider instance port.",
   "show vlan specific details for the I-comp bridge.",
   "show port related details for the I-comp bridge.",
   "Identifies the B-component as the bridge of interest.",
   "show customer backbone port.",
   "show provider network port.",
   "show vlan specific details for B-comp bridge.",
   "show port details for the B-comp bridge.")
{ 
  int brid;
  enum nsm_vlan_port_mode mode = NSM_VLAN_PORT_MODE_INVALID;
  if(argc > 1)
  {
    if (!pal_strncmp (argv[0], "backbone", 8))
      {
        if (!pal_strncmp (argv[1], "vlan", 4))
          nsm_bridge_beb_vlan(cli, argv[0]);
        else
          {
            if (!pal_strncmp (argv[1], "cbp", 3))
              mode = NSM_VLAN_PORT_MODE_CBP;
            else if (!pal_strncmp (argv[1], "pnp", 3))
              mode = NSM_VLAN_PORT_MODE_PNP;

            nsm_bridge_beb_port(cli, argv[0], mode);
          }
      }
    else
      { 
        CLI_GET_INTEGER_RANGE ("Bridge id", brid, argv[0], 1, 32);
  
        if (!pal_strncmp (argv[1], "vlan", 4))
          nsm_bridge_beb_vlan(cli, argv[0]);
        else
          {
            if (!pal_strncmp (argv[1], "cnp", 3))
              mode = NSM_VLAN_PORT_MODE_CNP;
            else if (!pal_strncmp (argv[1], "vip", 3))
              mode = NSM_VLAN_PORT_MODE_VIP;
            else
              mode = NSM_VLAN_PORT_MODE_PIP;
  
            nsm_bridge_beb_port(cli, argv[0], mode);
          }
      }
   }
   else
   {
     nsm_bridge_beb_show(cli, argv[0]);
   }
  return CLI_SUCCESS;
}

/* CLI to show the Configuration of Service */
CLI (nsm_show_pbb_beb_service,
   nsm_show_pbb_beb_service_cmd,		
   "show beb service (name WORD | <1-16777214> )",
   "show",
   "Information related to the backbone network",
   "Information related service instance.",
   "Indicates that the service instance is identified by name.",
   "Name of the service instance.",
   "Service instance ID.")
{
   pbb_isid_t isid;
   if (argc == 2)
     {
       nsm_bridge_beb_service_show (cli, argv[1], NSM_PBB_ISID_NONE); 
     }
   else
     {
       CLI_GET_INTEGER_RANGE ("Sevice ISID", isid, argv[0], NSM_PBB_ISID_MIN, 
                              NSM_PBB_ISID_MAX);
       nsm_bridge_beb_service_show(cli, NULL, isid );
     }
   return CLI_SUCCESS;
}
#endif /* HAVE_I_BEB || HAVE_B_BEB */

#ifdef HAVE_PBB_DEBUG
/* CLI to debug the configuration of Bridge */
CLI (nsm_show_pbb_debug_beb_bridge,
   nsm_show_pbb_debug_beb_bridge_cmd,
   "show beb debug (sno (1|2|3|4|5|6))",
   "show",
   "Information related to the backbone network",
   "debugs the information related to the node type",
   "serial number to specify the port of the node type",
   "Customer Network Port",
   "Virtual Instance Port",
   "Provider Instance Port",
   "VIPxref",
   "PIP2VIP",
   "Customer Backbone Port")
{

  int sno=0;

  CLI_GET_INTEGER_RANGE ("S No", sno, argv[1], 1, 6);

  nsm_bridge_pbb_list_tables_debug (cli, NULL, sno);

  return CLI_SUCCESS;
}
#endif /*HAVE_PBB_DEBUG*/

#ifdef HAVE_I_BEB
char *
get_pbb_instance_name (pbb_isid_t isid, struct nsm_pbb_icomponent *icomp)
{
  struct avl_node *node;
  struct vip_tbl *vip_node;
  for (node = avl_top(icomp->vip_table_list); node; node = avl_next(node))
  { 
    vip_node = (struct vip_tbl *)node->info; 
    if (vip_node->vip_sid != isid)
    { 
      continue;
    }
    else
      return vip_node->srv_inst_name;
  }
  return NULL;
   
}

char *
get_pbb_pip_portname(pbb_isid_t isid,struct nsm_pbb_icomponent *icomp)
{

return NULL;
/*yet to be defined */
}
#endif /* HAVE_I_BEB */


int
nsm_pbb_if_config_write (struct cli *cli, struct interface *ifp)
{
  struct nsm_if *zif;
  struct nsm_vlan *vlan = NULL;
  struct nsm_vlan_port *port;
  struct nsm_bridge_port *br_port;
  struct nsm_bridge *br;
  struct avl_node *node;
  struct avl_node *rn;
#ifdef HAVE_I_BEB
  struct svlan_bundle *svlan_bdl;
  struct listnode *svlan_node;
  struct cnp_srv_tbl *cnp_node;
  struct vip_tbl *vip_node;
  pbb_isid_t isid;
#endif/*HAVE_I_BEB*/
#ifdef HAVE_B_BEB
  struct cbp_srvinst_tbl *cbp_node;
  char mac_addr[16]; 
  nsm_vid_t vid;
#else
  struct interface* ifptemp;
#endif/*HAVE_B_BEB*/


  zif = (struct nsm_if *)ifp->info;
  if (! zif)
    return -1;

  br_port = zif->switchport;
  if (! br_port)
    return -1; 

  br = zif->bridge;
  if (br && zif->switchport)
    {
      br_port = zif->switchport;
      port = &(br_port->vlan_port);
      switch (port->mode)
        {
#ifdef HAVE_I_BEB
          case NSM_VLAN_PORT_MODE_CNP:
            {
              for (node = avl_top(br->master->beb->icomp->cnp_srv_tbl_list);
                   node; node = avl_next (node))
                {
                  if ((cnp_node = (struct cnp_srv_tbl *)
                      AVL_NODE_INFO (node)) == NULL)
                    continue;
                  if ( cnp_node->key.icomp_port_num != ifp->ifindex)
                    {
                      continue;
                    }
                  else
                    {
                      isid = cnp_node->key.sid;
#ifndef HAVE_B_BEB
                      /*API to get the PIP port name for I component cli*/
                      ifptemp = if_lookup_by_index (&br->master->nm->vr->ifm,cnp_node->ref_port_num);
                      if (!ifptemp)
                        continue;
#endif
                    } 
           
                  switch (cnp_node->srv_type)
                    { 
#ifndef HAVE_B_BEB
                      case NSM_SERVICE_INTERFACE_PORTBASED:
                        cli_out (cli," switchport beb customer-network "
                                 "port-based instance %u egress-port "
                                  "%s \n", isid, ifptemp->name);
                        break;
                      case NSM_SERVICE_INTERFACE_SVLAN_ALL:
                        cli_out (cli," switchport beb customer-network svlan "
                                 "all instance %u egress-port %s \n", 
                                 isid, ifptemp->name);
                        break;
                      case NSM_SERVICE_INTERFACE_SVLAN_SINGLE:
                        if (listcount (cnp_node->svlan_bundle_list) > 0)
                          LIST_LOOP (cnp_node->svlan_bundle_list, svlan_bdl, 
                                     svlan_node)
                            {
                              cli_out (cli," switchport beb customer-network "
                                       "svlan add %d instance %u "
                                       "egress-port %s \n",svlan_bdl->svid_low, 
                                       isid, ifptemp->name);
                            }
                        break;
                      case NSM_SERVICE_INTERFACE_SVLAN_MANY:
                        if (listcount (cnp_node->svlan_bundle_list) > 0)
                          LIST_LOOP ( cnp_node->svlan_bundle_list, svlan_bdl, 
                                      svlan_node)
                            {
                              if (svlan_bdl)
                                {
                                   if (svlan_bdl->svid_low == svlan_bdl->svid_high)
                                     cli_out (cli," switchport beb customer-network "
                                              "svlan add %d instance %u "
                                              "egress-port %s \n",svlan_bdl->svid_low,
                                              isid, ifptemp->name);
                                   else
                                     cli_out (cli, " switchport beb customer-network"                                      
                                              " svlan add range %d %d instance "
                                              "%u egress-port %s \n", 
                                              svlan_bdl->svid_low, 
                                              svlan_bdl->svid_high, isid, 
                                              ifptemp->name);
                                }
                            }
                        break;
                      default:
                        break;  
#else 
                      case NSM_SERVICE_INTERFACE_PORTBASED:
                        cli_out (cli," switchport beb customer-network "
                                 "port-based instance %u\n",
                                  isid);
                        break;
                      case NSM_SERVICE_INTERFACE_SVLAN_ALL:
                        cli_out (cli," switchport beb customer-network svlan "
                                 "all instance %u \n",
                                 isid);
                        break;
                      case NSM_SERVICE_INTERFACE_SVLAN_SINGLE:
                        if (listcount (cnp_node->svlan_bundle_list) > 0)
                          LIST_LOOP (cnp_node->svlan_bundle_list, svlan_bdl,
                                     svlan_node)
                            {
                              cli_out (cli," switchport beb customer-network "
                                       "svlan add %d instance %u \n",
                                        svlan_bdl->svid_low,
                                        isid);
                            }
                        break;
                      case NSM_SERVICE_INTERFACE_SVLAN_MANY:
                        if (listcount (cnp_node->svlan_bundle_list) > 0)
                          LIST_LOOP ( cnp_node->svlan_bundle_list, svlan_bdl,
                                      svlan_node)
                            {
                              if (svlan_bdl)
                                {
                                   if (svlan_bdl->svid_low == svlan_bdl->svid_high)
                                     cli_out (cli," switchport beb customer-network "
                                              "svlan add %d instance %u \n",
                                              svlan_bdl->svid_low,
                                              isid);
                                   else
                                     cli_out (cli, " switchport beb customer-network"
                                              " svlan add range %d %d instance "
                                              "%u \n",
                                              svlan_bdl->svid_low,
                                              svlan_bdl->svid_high, isid);
                                }
                            }
                        break;
                      default:
                        break;

#endif /* !HAVE_B_BEB */
                    }    
                  
                  if (cnp_node->srv_status == NSM_SERVICE_STATUS_DISPATCHED)
                    {
                      cli_out (cli, " switchport beb dispatch service %u \n",
                               isid);
                    } 
                }
              for (rn = avl_top (br->vlan_table); rn; rn = avl_next (rn))
                if ((vlan = rn->info) && (vlan->vid != NSM_VLAN_DEFAULT_VID))
                  { 
                    cli_out (cli," switchport beb vlan %d customer-network \n", 
                             vlan->vid);  
                  }
            }
            break;
          case NSM_VLAN_PORT_MODE_PIP:
            { 
              struct pip_tbl *pip_node = NULL;
              u_int8_t dmac[ETHER_ADDR_LEN] = {0};
              int index,index1;

              for (rn = avl_top (zif->bridge->vlan_table); rn; rn = avl_next (rn))
                if ((vlan = rn->info) && (vlan->vid != NSM_VLAN_DEFAULT_VID))
                  {
                    cli_out (cli," switchport beb vlan %d provider-instance \n"
                                                                   , vlan->vid);
                  }
              /* PIP mac address write */
              pip_node = nsm_pbb_search_pip_node(br->master->beb->icomp->pip_tbl_list,
                                                 br->bridge_id, ifp->ifindex);
              if (pip_node && pal_mem_cmp(ifp->hw_addr, dmac, ETHER_ADDR_LEN))
                {
                  cli_out (cli, " switchport beb pip backbone-source-mac %.04hx.%.04hx.%.04hx \n", 
                                pal_ntoh16(((unsigned short *)pip_node->pip_src_bmac)[0]),
                                pal_ntoh16(((unsigned short *)pip_node->pip_src_bmac)[1]),
                                pal_ntoh16(((unsigned short *)pip_node->pip_src_bmac)[2]));
                }

              /* VIP config write */
              if (pip_node)
                for (index = 0; index < 512; index++)
                  {
                    if (pip_node->pip_vip_map[index])
                      {
                        u_int8_t mask = 1;
                        for (index1 = 0; index1 < 8; index1++)
                         {
                          if (pip_node->pip_vip_map[index] & mask)
                          {
                            vip_node = nsm_pbb_search_vip_node(br->master->beb->icomp->vip_table_list, 
                                                               br->bridge_id, (index*8)+(index1)); 
                            if (vip_node)
                              {
                                struct vip_tbl vip_n_tmp;
                                nsm_pbb_set_vip_default_mac (&vip_n_tmp, vip_node->vip_sid);

                                switch(vip_node->srv_type)
                                  {
                                    case NSM_PORT_SERVICE_INGRESS:
                                      cli_out (cli," switchport beb vip instance %u "
                                               "allowed ingress \n", vip_node->vip_sid);
                                    break;
                                    case NSM_PORT_SERVICE_EGRESS:
                                      cli_out (cli," switchport beb vip instance %u "
                                               "allowed  egress \n", vip_node->vip_sid);
                                    break;
                                    case NSM_PORT_SERVICE_BOTH:
                                    cli_out (cli," switchport beb vip instance %u "
                                             "allowed all \n", vip_node->vip_sid);
                                    break;
                                    default:
                                    break;
                                  }
                                if (pal_mem_cmp(vip_n_tmp.default_dst_bmac, vip_node->default_dst_bmac,6))
                                  {
                                     cli_out (cli," switchport beb vip instance %u "
                                              "default-da %.04hx.%.04hx.%.04hx \n",
                                     vip_node->vip_sid,
                                     pal_ntoh16(((unsigned short *)vip_node->default_dst_bmac)[0]),
                                     pal_ntoh16(((unsigned short *)vip_node->default_dst_bmac)[1]),
                                     pal_ntoh16(((unsigned short *)vip_node->default_dst_bmac)[2]));
                                  }
                              }
                          }
                          mask = mask << 1;
                         }
                      }
                  }
            }
            break;
#endif /*HAVE_I_BEB*/
#ifdef HAVE_B_BEB            
          case NSM_VLAN_PORT_MODE_CBP:
            {
              for (node = avl_top(br->master->beb->bcomp->cbp_srvinst_list); 
                   node; node = avl_next (node))
                {
                  char default_mac[32]  = "";
                  u_int8_t dmac[ETHER_ADDR_LEN];
                  if ((cbp_node = (struct cbp_srvinst_tbl *)
                      AVL_NODE_INFO (node)) == NULL)
                    continue;
                  if (!(cbp_node->key.bcomp_port_num == ifp->ifindex))
                    continue;
                  pal_mem_set(dmac, 0, ETHER_ADDR_LEN);
                  if (pal_mem_cmp(cbp_node->default_dst_bmac, dmac, ETHER_ADDR_LEN)) 
                    { 
                       sprintf (mac_addr, "%.04hx.%.04hx.%.04hx",
                       pal_ntoh16(((unsigned short *)cbp_node->default_dst_bmac)[0]),
                       pal_ntoh16(((unsigned short *)cbp_node->default_dst_bmac)[1]),
                       pal_ntoh16(((unsigned short *)cbp_node->default_dst_bmac)[2]));
                       pal_strncpy(default_mac, "default-da ", 12);
                       pal_strcat (default_mac, mac_addr);
                    }
                  if (cbp_node->local_sid != NSM_PBB_ISID_NONE)
                    {
                       cli_out (cli, " switchport beb customer-backbone "
                                 "instance add %u bvlan %d %s %s instance-map"                                  
                                  " %u %s \n", cbp_node->key.b_sid, 
                                 cbp_node->bvid,
				 cbp_node->pt_mpt_type==NSM_BVLAN_M2M ? 
                                 "multipoint" : "",
                                 default_mac,
                                 cbp_node->local_sid, 
                                 cbp_node->edge? "edge":""); 
                    }
                  else
                    {
                       cli_out (cli, " switchport beb customer-backbone "
                                 "instance add %u bvlan %d %s %s"
                                 " %s\n", cbp_node->key.b_sid,
                                 cbp_node->bvid,
                                 cbp_node->pt_mpt_type==NSM_BVLAN_M2M ?
                                 "multipoint" : "",
                                 default_mac,
                                 cbp_node->edge? "edge":"");
                    }
                  if (cbp_node->status == NSM_SERVICE_STATUS_DISPATCHED)
                    {
                      cli_out (cli, " switchport beb dispatch service "
                                  "%d \n", cbp_node->key.b_sid);
                    }
                } 
 
            }
            
            break;
          case NSM_VLAN_PORT_MODE_PNP:
            {
            if (pal_strncmp(br->name, "backbone", 8))
               return 0;/* printing only for backbone bridge*/
            if (port->config == NSM_VLAN_CONFIGURED_ALL)
              {
                cli_out (cli, " switchport beb provider-network bvlan all\n");

                for (rn = avl_top (zif->bridge->vlan_table); rn;
                                                rn = avl_next (rn))
                  if ((vlan = rn->info) && (vlan->vid != NSM_VLAN_DEFAULT_VID))
                    {
                       if (!((NSM_VLAN_BMP_IS_MEMBER (port->staticMemberBmp,
                                                      vlan->vid))
                             || (NSM_VLAN_BMP_IS_MEMBER (port->dynamicMemberBmp,
                                                        vlan->vid))))
                         cli_out (cli, " switchport beb provider-network bvlan remove %d\n",
                                  vlan->vid);
                    }
              }
            else if (port->config == NSM_VLAN_CONFIGURED_NONE)
              {
                cli_out (cli, " switchport beb provider-network bvlan none\n");

                for (rn = avl_top (zif->bridge->vlan_table); rn; rn = avl_next (rn))
                  if ((vlan = rn->info) && (vlan->vid != NSM_VLAN_DEFAULT_VID))
                    {
                      if (NSM_VLAN_BMP_IS_MEMBER (port->staticMemberBmp,
                                                  vlan->vid))
                        {
                            cli_out (cli, " switchport beb provider-network bvlan add %d \n"
                                     , vlan->vid);
                        }
                    }
              }
            else
              {
               NSM_VLAN_SET_BMP_ITER_BEGIN(port->staticMemberBmp, vid)
                 {
                     if (vid == NSM_VLAN_DEFAULT_VID)
                       continue;
 
                     cli_out (cli, " switchport beb provider-network bvlan add %d \n"
                              , vid);
                 }
              NSM_VLAN_SET_BMP_ITER_END(port->staticMemberBmp, vid);
              }
             /* CBP enties go here for IB-BEB*/
            }
            break;
#endif /*HAVE_B_BEB*/            
          default:
            break;    
    }/* end of switch */
   } /*end of if condition */
  return 0;
}

#ifdef HAVE_I_BEB
int 
nsm_pbb_isid_config_write (struct cli *cli)
{
  struct apn_vr *vr = cli->vr;
  struct nsm_master *nm = vr->proto;
  struct nsm_bridge_master *master = nm->bridge;
  struct avl_node *rn;
  struct nsm_pbb_icomponent *icomp ;
  struct vip_tbl *vip_node;
  int write = 0;

  if (! master)
    return 0;
  
  icomp = master->beb->icomp;
  
  rn = avl_top(icomp->vip_table_list);

  if (rn)
    {
      cli_out (cli, "!\npbb isid list\n");
      write++;
    }

  for (; rn; rn = avl_next (rn))
    {
      vip_node = (struct vip_tbl *)AVL_NODE_INFO (rn);
      if (vip_node)
        {
          cli_out (cli, " isid %u name %s i-component %d\n",
                   vip_node->vip_sid, vip_node->srv_inst_name,
                   vip_node->key.icomp_id);
          write++;
        }
    }
  return write;
}

#endif /* HAVE_I_BEB */

void nsm_pbb_cli_init(struct lib_globals *zg)
{
  struct cli_tree *ctree;
  ctree = zg->ctree;
#ifdef HAVE_I_BEB
  cli_install_config (ctree, PBB_ISID_CONFIG_MODE, nsm_pbb_isid_config_write);
  cli_install (ctree, CONFIG_MODE, &pbb_isid_list_cmd);
  cli_install (ctree, PBB_ISID_CONFIG_MODE, &pbb_isid_list_icomp_add_cmd);
  cli_install (ctree, PBB_ISID_CONFIG_MODE, &pbb_isid_list_icomp_del_cmd);
  cli_install (ctree, INTERFACE_MODE, &nsm_pbb_switchport_beb_cnp_port_based_cmd);
  cli_install (ctree, INTERFACE_MODE, &nsm_pbb_cnp_svlan_all_to_instance_cmd);
  cli_install (ctree, INTERFACE_MODE, &nsm_pbb_cnp_add_svlan_range_to_instance_cmd);

  cli_install (ctree, INTERFACE_MODE, &nsm_pbb_cnp_remove_svlan_range_to_instance_cmd);

  cli_install (ctree, INTERFACE_MODE, &no_switchport_beb_cnp_port_based_cmd);
  cli_install (ctree, INTERFACE_MODE, &no_nsm_pbb_switchport_cnp_svlan_to_instance_cmd);
  cli_install (ctree, INTERFACE_MODE, &nsm_pbb_switchport_beb_vip_instance_cmd);
  cli_install (ctree, PBB_ISID_CONFIG_MODE, &pbb_isid_list_icomp_add_cmd);
#endif /*HAVE_I_BEB */
#ifdef HAVE_B_BEB
  cli_install (ctree, INTERFACE_MODE, &nsm_pbb_switchport_cbp_instance_to_bvid_cmd);
  cli_install (ctree, INTERFACE_MODE, &nsm_pbb_switchport_cbp_del_or_none_instance_cmd);
  cli_install (ctree, INTERFACE_MODE, &nsm_pbb_switchport_beb_provider_network_to_bvlan_cmd);
#endif /*HAVE_B_BEB */
#if defined (HAVE_I_BEB) || defined (HAVE_B_BEB) 
#if defined HAVE_I_BEB && !defined HAVE_B_BEB
  cli_install (ctree, INTERFACE_MODE, &nsm_pbb_switchport_beb_default_vlan_ipbb_cmd);
#endif /*HAVE_I_BEB && !HAVE_B_BEB*/
#if defined HAVE_I_BEB && defined HAVE_B_BEB
  cli_install (ctree, INTERFACE_MODE, &nsm_pbb_switchport_beb_default_vlan_ibpbb_cmd);
  cli_install (ctree, INTERFACE_MODE, &nsm_pbb_switchport_beb_pip_mac_cmd);
#endif /*HAVE_I_BEB && HAVE_B_BEB*/
#if !defined HAVE_I_BEB && defined HAVE_B_BEB
  cli_install (ctree, INTERFACE_MODE, &nsm_pbb_switchport_beb_default_vlan_bpbb_cmd);
#endif /*!HAVE_I_BEB && HAVE_B_BEB*/
  cli_install (ctree, INTERFACE_MODE, &nsm_pbb_beb_dispatch_instance_cmd);
  cli_install (ctree, EXEC_MODE, &nsm_show_pbb_beb_bridge_cmd);
  cli_install (ctree, EXEC_MODE, &nsm_show_pbb_beb_service_cmd);
#if defined HAVE_I_BEB && !defined HAVE_B_BEB
  cli_install (ctree, INTERFACE_MODE, &no_nsm_pbb_switchport_beb_default_vlan_ipbb_cmd);
#endif /*HAVE_I_BEB && !HAVE_B_BEB*/
#if defined HAVE_I_BEB && defined HAVE_B_BEB
  cli_install (ctree, INTERFACE_MODE, &no_nsm_pbb_switchport_beb_default_vlan_ibpbb_cmd);
#endif /*HAVE_I_BEB && HAVE_B_BEB*/
#if !defined HAVE_I_BEB && defined HAVE_B_BEB
  cli_install (ctree, INTERFACE_MODE, &no_nsm_pbb_switchport_beb_default_vlan_bpbb_cmd);
#endif /*!HAVE_I_BEB && HAVE_B_BEB*/
#ifdef HAVE_PBB_DEBUG
  cli_install (ctree, EXEC_MODE, &nsm_show_pbb_debug_beb_bridge_cmd);
#endif /*HAVE_PBB_DEBUG*/
#endif /*HAVE_I_BEB || HAVE_B_BEB*/
}

#endif /*HAVE_I_BEB || HAVE_B_BEB*/

