/* Copyright (C) 2003  All Rights Reserved.
   
Vlan Stack CLI

This module defines the CLI interface to the stacking functionality 
*/

#include "pal.h"
#include "lib.h"
#include "log.h"
#ifdef HAVE_VLAN_STACK
#include "cli.h"

#include "nsmd.h"
#include "nsm_interface.h"
#include "nsm_bridge.h"
#include "nsm_vlan_stack.h"
#include "hal_incl.h"
#include "nsm_vlan.h"

/* Enable the vlan stack for port */
int 
nsm_vlan_stacking_enable (struct interface *ifp,
                          u_int16_t stack_mode, 
                          u_int16_t ethertype)
{
  struct nsm_if *zif;
  struct nsm_vlan_port *vlan_port;
  struct nsm_bridge_port *br_port;
  int ret = 0;

  zif = (struct nsm_if *)ifp->info;

  if (zif == NULL || zif->switchport == NULL)
    return RESULT_ERROR;

  br_port = zif->switchport;
  vlan_port = &br_port->vlan_port;
  
  if (vlan_port->vlan_stacking == NSM_TRUE &&
      vlan_port->stack_mode == stack_mode &&
      vlan_port->tag_ethtype == ethertype)
    return RESULT_OK;

  ret = hal_vlan_stacking_enable (ifp->ifindex, ethertype, stack_mode);
  if (ret) 
    {
      zlog_err (ifp->vr->zg, "%% Can't enable vlan stacking \n");
      return RESULT_ERROR;
    }

  vlan_port->vlan_stacking = NSM_TRUE;
  vlan_port->tag_ethtype = ethertype;
  vlan_port->stack_mode = stack_mode;

  return RESULT_OK;
}


/* Disable vlan stacking for a port */
int
nsm_vlan_stacking_disable (struct interface *ifp)
{
  struct nsm_if *zif;
  struct nsm_vlan_port *vlan_port;
  struct nsm_bridge_port *br_port;
  int ret = 0;

  zif = (struct nsm_if *)ifp->info;

  if (zif == NULL || zif->switchport == NULL)
    return RESULT_ERROR;

  br_port = zif->switchport;
  vlan_port = &br_port->vlan_port;
  
  if (vlan_port->vlan_stacking == NSM_FALSE)
    return RESULT_OK;

  ret = hal_vlan_stacking_disable (ifp->ifindex);
  if (ret) 
    {
      zlog_err (ifp->vr->zg, "%% Can't disable vlan stacking \n");
      return RESULT_ERROR;
    }

  vlan_port->vlan_stacking = NSM_FALSE;
  vlan_port->stack_mode = HAL_VLAN_STACK_MODE_NONE;
  
  return RESULT_OK;
}

CLI (switchport_vlan_stack_enable,
     switchport_vlan_stack_enable_cmd,
     "switchport vlan-stacking (customer-edge-port|provider-port)",
     "Set the switching characteristics of the Layer2 interface",
     "Enable Vlan stacking",
     "Set the switching characteristics of the Layer2 interface to Customer-edge port",
     "Set the switching characteristics of the Layer2 interface to Provider port")
{
  int ret = RESULT_OK;
  struct interface *ifp = cli->index; 
  u_int16_t ethtype = NSM_VLAN_ETHERTYPE_DEFAULT;
  u_int16_t stack_mode = HAL_VLAN_STACK_MODE_NONE;
  int etype;

  if (pal_strncmp(argv[0], "c",1) == 0)
    stack_mode = HAL_VLAN_STACK_MODE_EXTERNAL;
  else
    stack_mode = HAL_VLAN_STACK_MODE_INTERNAL;  
      
  if (argc == 2)
    {
      if (pal_strlen (argv[1]) != 6)
        {
          cli_out (cli, "%% Invalid ethertype value (expect format: 0xYYYY\n");
          return CLI_ERROR;
        }

      pal_sscanf (argv[1], "%x", &etype);
      ethtype = (u_int16_t) etype;
    }
  
  ret = nsm_vlan_stacking_enable (ifp, stack_mode, ethtype);
  if (ret)
    {
      cli_out (cli, "%% Can't enable vlan stacking on a port, err = %d\n", ret);
      return CLI_ERROR;
    }
  
  return CLI_SUCCESS;

}

ALI (switchport_vlan_stack_enable,
     switchport_vlan_stack_ethtype_enable_cmd,
     "switchport vlan-stacking (customer-edge-port|provider-port) (ethertype ETHERTYPE|)",
     "Set the switching characteristics of the Layer2 interface",
     "Enable Vlan stacking",
     "Set the switching characteristics of the Layer2 interface to Customer-edge port",
     "Set the switching characteristics of the Layer2 interface to Provider port",
     "Ethertype field for the vlan tag (in 0xhhhh hexadecimal notation)",
     "Ethertype value (Default is 0x8100)");
 


CLI (switchport_vlan_stack_disable,
     switchport_vlan_stack_disable_cmd,
     "no switchport vlan-stacking",
     CLI_NO_STR,
     "Set the switching characteristics of the Layer2 interface",
     "Disable Vlan stacking")
{
  int ret = RESULT_OK;
  struct interface *ifp = cli->index; 

  ret = nsm_vlan_stacking_disable (ifp);
  if (ret)
    {
      cli_out (cli, "%% Can't disable vlan stacking on a port, err = %d\n", ret);
      return CLI_ERROR;
    }
  
  return CLI_SUCCESS;
}

int
nsm_vlan_stack_write (struct cli *cli, struct interface *ifp)
{
  struct nsm_if *zif;
  struct nsm_vlan_port *vlan_port;
  struct nsm_bridge_port *br_port;
  char eth_tag[6];
    
  zif = (struct nsm_if *)ifp->info;
  if (! zif || ! zif->switchport)
    return RESULT_ERROR;

  eth_tag[0] = '\0';

  br_port = zif->switchport;
  vlan_port = &br_port->vlan_port;
  if (vlan_port->vlan_stacking == NSM_TRUE)
    {
      if (vlan_port->tag_ethtype != NSM_VLAN_ETHERTYPE_DEFAULT)
        {
          pal_snprintf(eth_tag,sizeof(eth_tag),"0x%x", vlan_port->tag_ethtype); 
        } 
      if (vlan_port->stack_mode == HAL_VLAN_STACK_MODE_EXTERNAL)
        cli_out (cli, " switchport vlan-stacking customer-edge-port %s\n", eth_tag);
      if (vlan_port->stack_mode == HAL_VLAN_STACK_MODE_INTERNAL)
        cli_out (cli, " switchport vlan-stacking provider-port %s\n", eth_tag);
    }

  return RESULT_OK;
}



void
nsm_vlan_stack_cli_init (struct cli_tree *ctree)
{
  cli_install (ctree, INTERFACE_MODE, &switchport_vlan_stack_enable_cmd);
  cli_install (ctree, INTERFACE_MODE, &switchport_vlan_stack_disable_cmd);
}

#endif /* HAVE_VLAN_STACK */
