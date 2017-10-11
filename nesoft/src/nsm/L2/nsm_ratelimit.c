/* Copyright (C) 2003  All Rights Reserved.
   
L2 Broadcast Storm Suppression

This module defines the CLI interface to the Layer broadcast
storm suppression 

The following commands are implemented:

ENABLE MODE:
* show storm-control broadcast interface IFNAME
    
INTERFACE MODE:
* show storm-control broadcast
* storm-control broadcast level <0-100>
* no storm-control broadcast

*/

#include "pal.h"

#ifdef HAVE_RATE_LIMIT

#include "lib.h"
#include "log.h"
#include "cli.h"
#include "table.h"

#include "l2lib.h"
#include "l2_debug.h"
#include "pal_types.h"
#include "hal_incl.h"

#include "nsmd.h"
#include "nsm_bridge.h"
#include "nsm_interface.h"
#include "nsm_ratelimit.h"

static void
nsm_ratelimit_cli_init (struct lib_globals * zg);

void nsm_ratelimit_init(struct lib_globals *zg)
{
  nsm_ratelimit_cli_init(zg);
} 

void nsm_ratelimit__terminate(struct lib_globals *zg)
{
  if (hal_ratelimit_deinit() != HAL_SUCCESS)
    {
      zlog_debug(zg, "Could not deinitialise rate limiting in hardware\n");
      return;
    }
}
 
int
nsm_ratelimit_get_bcast_level (struct interface *ifp)
{
  struct nsm_if *zif = NULL;
  struct nsm_bridge_port *br_port = NULL; 
  
  if ( !(zif = (struct nsm_if *)ifp->info) || !(br_port = zif->switchport))
    return NSM_NO_RATELIMIT_LEVEL;
  
  return (br_port->is_rate_limit ? 
          br_port->bcast_level : NSM_NO_RATELIMIT_LEVEL);
}

int
nsm_ratelimit_get_mcast_level (struct interface *ifp)
{
  struct nsm_if *zif = NULL;
  struct nsm_bridge_port *br_port = NULL; 
  
  if ( !(zif = (struct nsm_if *)ifp->info) || !(br_port = zif->switchport))
    return NSM_NO_RATELIMIT_LEVEL;
  
  return (br_port->is_rate_limit ? 
          br_port->mcast_level : NSM_NO_RATELIMIT_LEVEL);
}

int
nsm_ratelimit_get_dlf_bcast_level (struct interface *ifp)
{
  struct nsm_if *zif = NULL;
  struct nsm_bridge_port *br_port = NULL; 
  
  if ( !(zif = (struct nsm_if *)ifp->info) || !(br_port = zif->switchport))
    return NSM_NO_RATELIMIT_LEVEL;
  
  return (br_port->is_rate_limit ? 
          br_port->dlf_bcast_level : NSM_NO_RATELIMIT_LEVEL);
}

int nsm_ratelimit_set (struct interface *ifp, u_int8_t level, 
                       u_int8_t fraction, nsm_ratelimit_type_t type)
{
  int ret = -1;
  struct nsm_if *zif = NULL;
  struct nsm_bridge_port *br_port = NULL; 
  
  if ( !(zif = (struct nsm_if *)ifp->info) || !(br_port = zif->switchport))
    return -1;
 
  br_port->is_rate_limit = 1;

  switch(type)
    {
      case NSM_BROADCAST_RATELIMIT:
        if ( (br_port->bcast_level != level) ||
             (br_port->bcast_fraction != fraction) )
          {
            ret = hal_l2_ratelimit_bcast( ifp->ifindex, level, fraction);
            if (ret != HAL_SUCCESS)
              return ret;
            br_port->bcast_level = level;
            br_port->bcast_fraction = fraction;
          }
       break;
      case NSM_MULTICAST_RATELIMIT:
        if ( (br_port->mcast_level != level) ||
             (br_port->mcast_fraction != fraction) )
          {
            ret = hal_l2_ratelimit_mcast( ifp->ifindex, level, fraction);
            if (ret != HAL_SUCCESS)
              return ret;
            br_port->mcast_level = level;
            br_port->mcast_fraction = fraction;
          }
       break;
      case NSM_DLF_RATELIMIT:
        if ( (br_port->dlf_bcast_level != level) ||
             (br_port->dlf_bcast_fraction != fraction) )
          {
            ret = hal_l2_ratelimit_dlf_bcast( ifp->ifindex, level, fraction);
            if (ret != HAL_SUCCESS)
              return ret;
            br_port->dlf_bcast_level = level;
            br_port->dlf_bcast_fraction = fraction;
          }
       break;
      case NSM_BROADCAST_MULTICAST_RATELIMIT:
        ret = hal_l2_ratelimit_bcast_mcast( ifp->ifindex, level, fraction);
        if (ret != HAL_SUCCESS)
          return ret;
       break;
      case NSM_ONLY_BROADCAST_RATELIMIT:
        ret = hal_l2_ratelimit_only_broadcast (ifp->ifindex, level, fraction);
        if (ret != HAL_SUCCESS)
        return ret;
       break;

      default:
        return -1;
       break;
    }

  if (br_port)
    {
      if ( (br_port->bcast_level == NSM_NO_RATELIMIT_LEVEL) &&
           (br_port->mcast_level == NSM_NO_RATELIMIT_LEVEL) &&
           (br_port->dlf_bcast_level == NSM_NO_RATELIMIT_LEVEL) )
        br_port->is_rate_limit = 0;
    }

#ifdef HAVE_HA
/*  nsm_rate_limit_user_prio_regen_cal_modify (br_port);*/
#endif /*HAVE_HA*/

  return RESULT_OK;
}

/* Initialise Rate Limit values */
int nsm_ratelimit_if_init (struct interface *ifp)
{
  struct nsm_if *zif = NULL;
  struct nsm_bridge_port *br_port = NULL; 

  if ( !(zif = (struct nsm_if *)ifp->info) || !(br_port = zif->switchport))
    return -1;

  br_port->is_rate_limit = 0;
  br_port->bcast_level = NSM_NO_RATELIMIT_LEVEL;
  br_port->bcast_fraction = NSM_ZERO_FRACTION;
  br_port->mcast_level = NSM_NO_RATELIMIT_LEVEL;
  br_port->mcast_fraction = NSM_ZERO_FRACTION;
  br_port->dlf_bcast_level = NSM_NO_RATELIMIT_LEVEL;
  br_port->dlf_bcast_fraction = NSM_ZERO_FRACTION;

  return 0;
}

/* Deinitialise Rate Limit Values */
int nsm_ratelimit_if_deinit (struct interface *ifp)
{
  nsm_ratelimit_set (ifp, NSM_NO_RATELIMIT_LEVEL, NSM_ZERO_FRACTION,
                     NSM_BROADCAST_RATELIMIT);
  nsm_ratelimit_set (ifp, NSM_NO_RATELIMIT_LEVEL, NSM_ZERO_FRACTION,
                     NSM_MULTICAST_RATELIMIT);
  nsm_ratelimit_set (ifp, NSM_NO_RATELIMIT_LEVEL, NSM_ZERO_FRACTION,
                     NSM_DLF_RATELIMIT);
  
  return 0;
}

CLI (switchport_ratelimit1,
     switchport_ratelimit_cmd1,
     "storm-control  ({broadcast|multicast}|)",
     "Set the switching characteristics of Layer2 interface",
     "Set Broadcast Rate Limiting",
     "Set Multicast Rate Limiting")
{
  int ret;
  u_int8_t level = 0;
  u_int8_t fraction = NSM_ZERO_FRACTION;
  struct interface *ifp = cli->index;
  nsm_ratelimit_type_t type;

  NSM_BRIDGE_CHECK_L2_PROPERTY(cli, ifp);

  if (argc == 1)
    {
      if ( !pal_strncmp (argv[0], "m", 1))
        {
          cli_out (cli, "Please enable broadcast rate limiting also\n");
          return CLI_ERROR;
        }
      type = NSM_ONLY_BROADCAST_RATELIMIT;
    }
  else
    type = NSM_BROADCAST_RATELIMIT | NSM_MULTICAST_RATELIMIT;

    ret = nsm_ratelimit_set (ifp, level, fraction, type);
  
  if(ret == RESULT_NO_SUPPORT)
    {
      cli_out (cli, "Rate Limiting is not supported\n");
      return CLI_ERROR;
    }
  else if (ret != 0)
    {
      cli_out (cli, "Set Rate Limit Error\n");
      return CLI_ERROR;
    }

  return CLI_SUCCESS;
}

CLI (switchport_ratelimit,
     switchport_ratelimit_cmd,
     "storm-control (broadcast|multicast|dlf) level LEVEL",
     "Set the switching characteristics of Layer2 interface",
     "Set Broadcast Rate Limiting",
     "Set Multicast Rate Limiting",
     "Set DLF Broadcast Rate Limiting",
     "Threshold Level",
     "Threshold Percentage (0.0-100.0)")
{
  int ret;
  u_int8_t level;
  u_int8_t fraction = NSM_ZERO_FRACTION;
  int tmp_level;
  struct interface *ifp = cli->index;
  char str[CLI_ARGV_MAX_LEN];
  char *token;
  char *tokenizer = ".";
  char *last;
     
  NSM_BRIDGE_CHECK_L2_PROPERTY(cli, ifp);

  pal_strncpy(str, argv[1], CLI_ARGV_MAX_LEN);
  
  token = pal_strtok_r(str, tokenizer, &last);
  if (!token)
    goto INPUT_ERR;
  
  CLI_GET_INTEGER_RANGE ("level", tmp_level, token, 0, 100);
  level = (u_int8_t)tmp_level;
  
  token = pal_strtok_r(NULL, tokenizer, &last);
  if (token)
    {
      CLI_GET_INTEGER_RANGE ("fraction", tmp_level, token, 0, 99);
      fraction = (u_int8_t)tmp_level;
      if ( (level == NSM_NO_RATELIMIT_LEVEL) &&
           (fraction != NSM_ZERO_FRACTION) )
        goto INPUT_ERR;
    }
  
  token = pal_strtok_r(NULL, tokenizer, &last);
  if (token)
    goto INPUT_ERR;

  if ( !pal_strncmp (argv[0], "b", 1) )
    ret = nsm_ratelimit_set (ifp, level, fraction, NSM_BROADCAST_RATELIMIT);
  else if ( !pal_strncmp (argv[0], "m", 1) )
    ret = nsm_ratelimit_set (ifp, level, fraction, NSM_MULTICAST_RATELIMIT);
  else if ( !pal_strncmp (argv[0], "d", 1) )
    ret = nsm_ratelimit_set (ifp, level, fraction, NSM_DLF_RATELIMIT);
  else
    ret = CLI_ERROR;
  
  if(ret == RESULT_NO_SUPPORT) 
    {
      cli_out (cli, "Rate Limiting is not supported\n");
      return CLI_ERROR;
    }
  else if (ret != 0)
    {
      cli_out (cli, "Set Rate Limit Error\n");
      return CLI_ERROR;
    } 

  return CLI_SUCCESS;
INPUT_ERR:
  cli_out(cli, "Input Error\n");
  return CLI_ERROR;
}

CLI (no_switchport_ratelimit,
     no_switchport_ratelimit_cmd,
     "no storm-control (broadcast|multicast|dlf) level",
     CLI_NO_STR,
     "Reset the switching characteristics of Layer2 interface",
     "Reset Broadcast Rate Limiting of layer2 Interface",
     "Reset Multicast Rate Limiting of layer2 Interface",
     "Reset DLF Broadcast Rate Limiting of layer2 Interface",
     "Threshhold Level")
{
  struct interface *ifp = cli->index;
  int ret;
  
  NSM_BRIDGE_CHECK_L2_PROPERTY(cli, ifp);
  
  if (LAYER2_DEBUG(proto, CLI_ECHO))
    zlog_debug(cli->zg, "no switchport %s", argv[0]);

  if ( !pal_strncmp (argv[0], "b", 1) )
    ret = nsm_ratelimit_set (ifp, NSM_NO_RATELIMIT_LEVEL, NSM_ZERO_FRACTION,
                             NSM_BROADCAST_RATELIMIT);
  else if ( !pal_strncmp (argv[0], "m", 1) )
    ret = nsm_ratelimit_set (ifp, NSM_NO_RATELIMIT_LEVEL, NSM_ZERO_FRACTION,
                             NSM_MULTICAST_RATELIMIT);
  else if ( !pal_strncmp (argv[0], "d", 1) )
    ret = nsm_ratelimit_set (ifp, NSM_NO_RATELIMIT_LEVEL, NSM_ZERO_FRACTION,
                             NSM_DLF_RATELIMIT);
  else
    ret = CLI_ERROR;
  
  if(ret == RESULT_NO_SUPPORT) 
    {
      cli_out (cli, "Rate Limiting is not supported\n");
      return CLI_ERROR;
    }
  else if (ret != 0)
    {
      cli_out (cli, "Unset Rate Limit Error\n");
      return CLI_ERROR;
    } 

  return CLI_SUCCESS;
}

static void
nsm_if_dump_rate_limit(struct cli *cli, struct interface *ifp)
{
  int bcast_discards = 0;
  int mcast_discards = 0;
  int dlf_discards = 0;
  struct nsm_bridge_port *node = NULL;
  struct nsm_if *zif = (struct nsm_if *)ifp->info;
  
  if ( !zif || !(node = zif->switchport) )
    return;
  
  if (zif->type != NSM_IF_TYPE_L2)
    return;

#ifdef HAVE_HAL      
  hal_l2_bcast_discards_get(ifp->ifindex, &bcast_discards);
  hal_l2_mcast_discards_get(ifp->ifindex, &mcast_discards);
  hal_l2_dlf_bcast_discards_get(ifp->ifindex, &dlf_discards);
#endif /* HAVE_HAL */
  
  cli_out(cli, "%-8s%7d.%2d%%%14d%7d.%2d%%%14d%5d.%2d%%%10d\n", ifp->name, 
          node->bcast_level, node->bcast_fraction, bcast_discards, 
          node->mcast_level, node->mcast_fraction, mcast_discards,
          node->dlf_bcast_level, node->dlf_bcast_fraction, dlf_discards);
}

CLI (show_l2_ratelimit,
     show_l2_ratelimit_cmd,
     "show storm-control (IFNAME|)",
     CLI_SHOW_STR,
     "The layer2 interface",
     "Display Rate Limit",
     "Interface name")
{
  struct interface *ifp;
  struct route_node *rn;
  struct apn_vr *vr = cli->vr;
  

  if (argc != 0)
    {
      ifp = if_lookup_by_name (&vr->ifm, argv[0]);
      if (ifp == NULL)
      {
        cli_out(cli, "%% Can't find interface %s\n", argv[0]);
        return CLI_ERROR;
      }
      
      cli_out(cli, "Port     BcastLevel BcastDiscards McastLevel "
                   "McastDiscards");
      cli_out(cli, " DlfLevel DlfDiscards\n");

     NSM_BRIDGE_CHECK_L2_PROPERTY(cli, ifp);
      
      nsm_if_dump_rate_limit(cli, ifp);
      return CLI_SUCCESS;
    }
  
  /* All layer2 interface print */
  for ( rn = route_top (vr->ifm.if_table); rn; rn = route_next (rn) )
     if ( (ifp = rn->info) )
       nsm_if_dump_rate_limit(cli, ifp);

  return CLI_SUCCESS;
}

int 
nsm_ratelimit_write(struct cli *cli, int ifindex )
{
  int write = 0;
  struct interface *ifp = NULL;
  struct nsm_bridge_port *node = NULL;
  struct nsm_if *zif = NULL;
  
  ifp = if_lookup_by_index (&cli->vr->ifm, ifindex);
  if (!ifp)
    return write;
   
  if ( !(zif = (struct nsm_if *)ifp->info) || !(node = zif->switchport) )
    return write;
  
  if (zif->type != NSM_IF_TYPE_L2)
    return write;
  
  if (node->bcast_level != NSM_NO_RATELIMIT_LEVEL)
    {
      if (node->bcast_fraction != NSM_ZERO_FRACTION)
        cli_out(cli, " storm-control broadcast level %u.%u\n", 
            node->bcast_level, node->bcast_fraction);
      else
        cli_out(cli, " storm-control broadcast level %u\n", node->bcast_level);
      write++;
    }
  if (node->mcast_level != NSM_NO_RATELIMIT_LEVEL)
    {
      if (node->mcast_fraction != NSM_ZERO_FRACTION)
        cli_out(cli, " storm-control multicast level %u.%u\n", 
            node->mcast_level, node->mcast_fraction);
      else
        cli_out(cli, " storm-control multicast level %u\n", node->mcast_level);
      write++;
    }
  if (node->dlf_bcast_level != NSM_NO_RATELIMIT_LEVEL)
    {
      if (node->dlf_bcast_fraction != NSM_ZERO_FRACTION)
        cli_out(cli, " storm-control dlf level %u.%u\n", 
            node->dlf_bcast_level, node->dlf_bcast_fraction);
      else
        cli_out(cli, " storm-control dlf level %u\n", node->dlf_bcast_level);
      write++;
    }
  return write;
}


void
nsm_ratelimit_cli_init (struct lib_globals * zg)
{
  struct cli_tree * ctree = zg->ctree;
  
  cli_install_default (ctree, INTERFACE_MODE);
   
  cli_install (ctree, INTERFACE_MODE, &switchport_ratelimit_cmd);
  cli_install (ctree, INTERFACE_MODE, &no_switchport_ratelimit_cmd);
  cli_install (ctree, INTERFACE_MODE, &switchport_ratelimit_cmd1);
  cli_install (ctree, EXEC_MODE, &show_l2_ratelimit_cmd);
}

#endif /* HAVE_RATE_LIMIT */
