/* Copyright (C) 2003  All Rights Reserved.
   
L2 Port Mirroring

This module defines the CLI interface to the Layer 2 port mirroring 

The following commands are implemented:

ENABLE MODE:
* show mirror interface IFNAME
    
INTERFACE MODE:
* mirror interface IFNAME direction {receive | transmit | both} 
* no mirror interface IFNAME
* show mirror

*/

#include "pal.h"
#include "lib.h"
#include "log.h"
#include "cli.h"

#include "l2lib.h"
#include "l2_debug.h"
#include "pal_types.h"
#include "nsm_pmirror.h"
#include "hal_incl.h"

static void
port_mirroring_cli_init (struct lib_globals * zg);

void port_mirroring_init(struct lib_globals *zg)
{
  port_mirror_list = list_new();
  if (port_mirror_list == NULL) {
    zlog_debug(zg, "Cannot allocate port mirroring list\n");
    return;
  }

  port_mirror_list_config_restore = list_new();
  port_mirroring_cli_init(zg);
}

void *
port_mirroring_get_first (void)
{
  return port_mirror_list;
}

static int
port_mirroring_list_direction_check (struct interface *to,
                                     struct interface *from,
                                     int    *direction,
                                     struct lib_globals *zg)
{
  struct listnode *n;
  struct pm_node *new_node = NULL;
  int found = 0;
 
  /* 
    Discard the new port mirror configuration if an Analyzer port 
    already exists in this direction exists for this Mirrored port.
  */  
  LIST_LOOP(port_mirror_list, new_node, n) 
    {
      if (new_node->from == from) 
        {
          if ((new_node->direction == *direction) || 
              (new_node->direction == HAL_PORT_MIRROR_DIRECTION_BOTH)) 
            {
              found = 1;
              break; 
            }
          else if (*direction == HAL_PORT_MIRROR_DIRECTION_BOTH)
            {
              *direction &= ~new_node->direction;
            }
        } 
    }

  if (!found) {
    return 0; 
  }

  *direction &= ~new_node->direction;

  /* If port already mirrored the same way return error. */ 
  if(*direction == 0)
     return -1;
  /* New direction added. */ 
  return 0;
}

int
port_mirroring_list_add (struct lib_globals *zg, 
                         struct list *mirror_list,
                         struct interface *to,
                         struct interface *from,
                         int    direction)
{
  struct listnode *n;
  struct pm_node *new_node = NULL;
  int found = 0;

  LIST_LOOP(mirror_list, new_node, n) {
    if ((new_node->from == from) &&
        (new_node->to == to))
      {
        found = 1;
        break;
      }
  }

  if (!found) {
    new_node = XCALLOC(MTYPE_PORT_MIRROR_ENTRY, sizeof(*new_node));
    if (new_node == NULL) {
      zlog_debug(zg, "Cannot allocate port mirroring data node\n");
      return -1;
    }
    new_node->to = to;
    new_node->from = from;
    new_node->direction = 0;

    n = listnode_add(mirror_list, (void *)new_node);
    if (n == NULL) {
      zlog_debug(zg, "Cannot allocate port mirror list node\n");
      return -1;
    }
  }

  new_node->direction |= direction;

  return 0;
}

int
port_mirroring_list_del(struct interface *to,
                        struct interface *from,
                        int    *direction,
                        struct lib_globals *zg)
{
  struct listnode *n;
  struct pm_node *old_node = NULL;
  int found = 0;
  int prev = 0;
 
  LIST_LOOP(port_mirror_list, old_node, n) {
    if ( (old_node->from == from) &&
         (old_node->to == to) ) {
      found = 1;
      break;
    }
  } 

  if (!found) 
    return -1;

  prev = old_node->direction;

  old_node->direction &= ~*direction;

  if (old_node->direction == 0) 
    {
      /*Delete the entry from the list */ 
      listnode_delete(port_mirror_list, (void *)old_node);
    }

  if (prev != HAL_PORT_MIRROR_DIRECTION_BOTH)
    {
      if (*direction == HAL_PORT_MIRROR_DIRECTION_BOTH && 
          old_node->direction == 0)
        {
          /* Only one direction configured */
          *direction = prev;
        }
      else if (prev == old_node->direction)
        {
          /* Return error, if direction is invalid */
          zlog_debug(zg, "Invalid direction for this analyzer port\n");
          return -1;
        } 
    }

  return 0;
}

int 
port_add_mirror_interface (struct interface * ifp_to, char *ifname,
                               int *mirror_direction)
{
  int ret = RESULT_NO_SUPPORT;
  struct interface *ifp;

  ifp = if_lookup_by_name (&ifp_to->vr->ifm, ifname);

  if (ifp != NULL)
    {
      if (ifp_to->ifindex == ifp->ifindex)
        {
          zlog_err (ifp->vr->zg, "To and From interface is the same\n");
          return RESULT_ERROR;
        }
      
      ret = port_mirroring_list_direction_check (ifp_to, ifp, mirror_direction,
                                                 ifp->vr->zg);
      if (ret < 0)
        {
          zlog_err (ifp->vr->zg, "Port already mirrored\n");
          return RESULT_ERROR;
        }

#ifdef HAVE_HAL
    /* Set mirroring in the hardware layer. */
    ret = hal_port_mirror_set (ifp_to->ifindex, ifp->ifindex, 
                               *mirror_direction);

    if (ret < 0 || ret == RESULT_NO_SUPPORT)
      {
        if (ret == RESULT_NO_SUPPORT)
          zlog_debug(ifp->vr->zg, "Port Mirroring is not supported\n");
        else
          zlog_debug(ifp->vr->zg, "Mirrored-to port already exists. \n");

        /* Add it to Config Restore list. */
        if(!FLAG_ISSET (ifp->vr->host->flags, HOST_CONFIG_READ_DONE))
          port_mirroring_list_add (ifp->vr->zg, port_mirror_list_config_restore,
                                   ifp_to, ifp, *mirror_direction);
        return RESULT_NO_SUPPORT;
      }
    else
#endif /* HAVE_HAL */
      {
        ret = port_mirroring_list_add (ifp->vr->zg, port_mirror_list,
                                       ifp_to, ifp, *mirror_direction);
        if (ret < 0)
          {
            zlog_err (ifp->vr->zg, "Couldn't add port mirror\n");
#ifdef HAVE_HAL
            /* Unset mirroring in the hardware. */
            hal_port_mirror_unset(ifp_to->ifindex, ifp->ifindex, 
                                  *mirror_direction);
#endif /* HAVE_HAL */
            return RESULT_ERROR;
          }
      }
  }
  else {
    zlog_debug(ifp_to->vr->zg, "Interface %s does not exist\n", ifname);
    return RESULT_ERROR;
  }

  return RESULT_OK;

}

int
port_mirror_list_config_restore_after_sync (struct lib_globals *zg)
{
  struct listnode *n = NULL, *node_next = NULL;
  struct pm_node *temp_node = NULL;

 LIST_LOOP_DEL (port_mirror_list_config_restore, temp_node, n, node_next)
    {
      port_add_mirror_interface (temp_node->to, temp_node->from->name,
                                 &temp_node->direction);
      listnode_delete (port_mirror_list_config_restore, (void *)temp_node);
      XFREE (MTYPE_PORT_MIRROR_ENTRY, temp_node);
    }

  if (port_mirror_list_config_restore && 
      listcount (port_mirror_list_config_restore) == 0)
    {
      list_delete (port_mirror_list_config_restore);
      port_mirror_list_config_restore = NULL;
    }
  return RESULT_OK;
}

int 
port_mirror_list_delete_all (struct list *temp_mirror_list)
{
  struct listnode *n = NULL, *node_next = NULL;
  struct pm_node *temp_node = NULL;

  LIST_LOOP_DEL (temp_mirror_list, temp_node, n, node_next)
    {
      listnode_delete (temp_mirror_list, (void *)temp_node);
      XFREE (MTYPE_PORT_MIRROR_ENTRY, temp_node);
    }

  if (temp_mirror_list && listcount (temp_mirror_list) == 0)
    {
      list_delete (temp_mirror_list);
      temp_mirror_list = NULL;
    }

  return RESULT_OK;
}

void 
port_mirroring_deinit ()
{
  port_mirror_list_delete_all (port_mirror_list_config_restore);
  port_mirror_list_delete_all (port_mirror_list);
}
 
CLI (mirror_interface_both,
     mirror_interface_both_cmd,
     "mirror interface IFNAME direction both",
     "Port mirroring command",
     "Interface to use",
     "Source Interface to use",
     "Mirroring direction",
     "Mirror traffic in both directions")
{
  int ret=RESULT_NO_SUPPORT;
  struct interface *ifp_to = cli->index;
  int direction = HAL_PORT_MIRROR_DIRECTION_BOTH;
 
  if (LAYER2_DEBUG(proto, CLI_ECHO))
    zlog_debug(cli->zg, "mirror interface %s direction both", argv[0]);

  if (if_lookup_by_name (&cli->vr->ifm, ifp_to->name) == NULL)
    {
      cli_out(cli, "%%Mirroring configuration for %s could not be"
                   " stored\n", ifp_to->name);
      return CLI_ERROR;
    }

  ret = port_add_mirror_interface (ifp_to, argv[0], &direction);

  if (ret == RESULT_NO_SUPPORT)
    {
      cli_out (cli, "%%port mirror not supported\n");
      return CLI_ERROR;
    }

  if (ret < 0)
    {
      if (direction == 0)
        cli_out (cli, "%%%s already mirrored in Tx and Rx directions\n", argv[0]);
      else
        cli_out (cli, "%%Couldn't add port mirror\n");
      return CLI_ERROR;
    }

  if (direction != HAL_PORT_MIRROR_DIRECTION_BOTH)
    {
      if (direction == HAL_PORT_MIRROR_DIRECTION_TRANSMIT)
        cli_out (cli, "%%%s already mirrored in Rx direction\n", argv[0]);
      else
        cli_out (cli, "%%%s already mirrored in Tx direction\n", argv[0]);

      return CLI_ERROR;
    }
  return CLI_SUCCESS;
}

CLI (no_mirror_interface,
     no_mirror_interface_cmd,
     "no mirror interface IFNAME",
     CLI_NO_STR,
     "Port mirroring command",
     "Interface to use",
     "Source Interface to use")
{
  int ret=RESULT_NO_SUPPORT;
  struct interface *ifp_to = cli->index;
  struct interface *ifp=NULL;
  int direction = HAL_PORT_MIRROR_DIRECTION_BOTH;
  
  if (LAYER2_DEBUG(proto, CLI_ECHO))
    zlog_debug(cli->zg, "no mirror interface %s", argv[0]);

  /*    Find interface */
  ifp = if_lookup_by_name(&cli->vr->ifm, argv[0]);
  if (ifp != NULL) {
    if (ifp_to->ifindex == ifp->ifindex) 
      {
        cli_out(cli, "%%To and From interface is the same\n");
        return CLI_ERROR;
      }

    /* Delete port from mirror list. */
    ret = port_mirroring_list_del(ifp_to, ifp, &direction, cli->zg);
    if (ret < 0)
      {
        cli_out (cli, "%%%s is not the analyzer port for %s\n", ifp_to->name, ifp->name);
        return CLI_ERROR;
      }

#ifdef HAVE_HAL
    /* Unset mirroring in the hardware. */
    ret = hal_port_mirror_unset(ifp_to->ifindex, ifp->ifindex, direction);
#endif /* HAVE_HAL */
    if (ret < 0) 
      {
        if (ret == RESULT_NO_SUPPORT)
          cli_out(cli, "%%Port Mirroring is not supported\n");
        else 
          cli_out(cli, "%%Mirrored-to port doesn't exist. \n");

        /* Add mirror port back to list. */
        port_mirroring_list_add (cli->zg, port_mirror_list, ifp_to, ifp, direction);
        return CLI_ERROR;
      }

    if (direction != HAL_PORT_MIRROR_DIRECTION_BOTH)
      {
        if (direction == HAL_PORT_MIRROR_DIRECTION_TRANSMIT)
          cli_out(cli, "%%Rx analyzer not configured. Deleted Tx analyzer port\n");
        else
          cli_out(cli, "%%Tx analyzer not configured. Deleted Rx analyzer port\n"); 

        return CLI_ERROR;
      }
  }
  else 
    {
      cli_out(cli, "%%Interface %s does not exist\n", argv[0]); 
      return CLI_ERROR;   
    }
   
  return CLI_SUCCESS;
}

CLI (mirror_interface_receive,
     mirror_interface_receive_cmd,
     "mirror interface IFNAME direction receive",
     "Port mirroring command",
     "Interface to use",
     "Source Interface to use",
     "Mirroring direction",
     "Mirror received traffic")
{
  int ret=RESULT_NO_SUPPORT;
  struct interface *ifp_to = cli->index;
  int direction = HAL_PORT_MIRROR_DIRECTION_RECEIVE;  
 
  if (LAYER2_DEBUG(proto, CLI_ECHO))
    zlog_debug(cli->zg, "mirror interface %s direction receive", argv[0]);

  if (if_lookup_by_name (&cli->vr->ifm, ifp_to->name) == NULL)
    {
      cli_out(cli, "%%Mirroring configuration for %s could not be stored\n", 
              ifp_to->name);
      return CLI_ERROR;
    }

  ret = port_add_mirror_interface (ifp_to, argv[0], &direction);

  if (ret == RESULT_NO_SUPPORT)
    {
      cli_out (cli, "%%port mirror not supported\n");
      return CLI_ERROR;
    }

  if (ret < 0)
    {
       if (direction == 0)
         cli_out (cli, "%%%s already mirrored in Rx direction\n", argv[0]);
       else
         cli_out (cli, "%%Couldn't add port mirror\n");

       return CLI_ERROR;
    }
 
  return CLI_SUCCESS;
}

CLI (no_mirror_interface_receive,
     no_mirror_interface_receive_cmd,
     "no mirror interface IFNAME direction receive",
     CLI_NO_STR,
     "Port mirroring command",
     "Interface to use",
     "Source Interface to use",
     "Mirroring direction",
     "Mirror received traffic")
{
  int ret=RESULT_NO_SUPPORT;
  struct interface *ifp_to = cli->index;
  struct interface *ifp=NULL;
  int direction = HAL_PORT_MIRROR_DIRECTION_RECEIVE;
  
  if (LAYER2_DEBUG(proto, CLI_ECHO))
    zlog_debug(cli->zg, "mirror interface %s direction receive", argv[0]);

  /*    Find interface */
  ifp = if_lookup_by_name(&cli->vr->ifm, argv[0]);
  if (ifp != NULL) 
    {
      if (ifp_to->ifindex == ifp->ifindex) 
        {
          cli_out(cli, "%%To and From interface is the same\n");
          return CLI_ERROR;
        }

      /* Delete port from mirroring list. */
      ret = port_mirroring_list_del(ifp_to, ifp, &direction, cli->zg);
      if (ret < 0)
        {
          cli_out (cli, "%%%s is not the Rx analyzer port for %s\n", 
                   ifp_to->name, ifp->name);
          return CLI_ERROR;
        }

#ifdef HAVE_HAL
      /* Delete port for mirroring from the hardware layer. */
      ret = hal_port_mirror_unset(ifp_to->ifindex, ifp->ifindex, 
                                  HAL_PORT_MIRROR_DIRECTION_RECEIVE);
#endif /* HAVE_HAL */
      if (ret < 0) 
        {
          if (ret == RESULT_NO_SUPPORT) 
            cli_out(cli, "%%Port Mirroring is not supported\n");
          else 
            cli_out(cli, "%%Mirrored-to port doesn't exist. \n");
          /* Add port back to list. */
          port_mirroring_list_add (cli->zg, port_mirror_list,
                                   ifp_to, ifp, HAL_PORT_MIRROR_DIRECTION_RECEIVE);

          return CLI_ERROR;
        }
    }
  else {
    cli_out(cli, "%%Interface %s does not exist\n", argv[0]);
    return CLI_ERROR;    
  }

  return CLI_SUCCESS;
}

CLI (mirror_interface_transmit,
     mirror_interface_transmit_cmd,
     "mirror interface IFNAME direction transmit",
     "Port mirroring command",
     "Interface to use",
     "Source Interface to use",
     "Mirroring direction",
     "Mirror transmit traffic")
{
  int ret = RESULT_NO_SUPPORT;
  struct interface *ifp_to = cli->index;
  int direction = HAL_PORT_MIRROR_DIRECTION_TRANSMIT;  
 
  if (LAYER2_DEBUG(proto, CLI_ECHO))
    zlog_debug(cli->zg, "mirror interface %s direction transmit", argv[0]);

  if (if_lookup_by_name (&cli->vr->ifm, ifp_to->name) == NULL)
    {
      cli_out(cli, "%%Mirroring configuration for %s could not be stored\n", ifp_to->name);
      return CLI_ERROR;
    }

  ret = port_add_mirror_interface (ifp_to, argv[0], &direction);
  if (ret == RESULT_NO_SUPPORT)
    {
      cli_out (cli, "%%port mirror not supported\n");
      return CLI_ERROR;
    }

  if (ret < 0)
    {
      if (direction == 0)
        cli_out (cli, "%%%s already mirrored in Tx direction\n", argv[0]);
      else
        cli_out (cli, "%%Couldn't add port mirror\n");
      return CLI_ERROR;
    }

  return CLI_SUCCESS;
}

CLI (no_mirror_interface_transmit,
     no_mirror_interface_transmit_cmd,
     "no mirror interface IFNAME direction transmit",
     CLI_NO_STR,
     "Port mirroring command",
     "Interface to use",
     "Source Interface to use",
     "Mirroring direction",
     "Mirror received traffic")
{
  int ret=RESULT_NO_SUPPORT;
  struct interface *ifp_to = cli->index;
  struct interface *ifp=NULL;
  int direction = HAL_PORT_MIRROR_DIRECTION_TRANSMIT;
  
  if (LAYER2_DEBUG(proto, CLI_ECHO))
    zlog_debug(cli->zg, "no mirror interface %s direction transmit", argv[0]);

  /*    Find interface */
  ifp = if_lookup_by_name(&cli->vr->ifm, argv[0]);
  if (ifp != NULL) 
    {
      if (ifp_to->ifindex == ifp->ifindex) 
        {
          cli_out(cli, "%%To and From interface is the same\n");
          return CLI_ERROR;
        }

      /* Delete port from mirror list. */
      ret = port_mirroring_list_del(ifp_to, ifp, &direction, cli->zg);
      if (ret < 0)
        {
          cli_out (cli, "%%%s is not the Tx analyzer port for %s\n", 
                   ifp_to->name, ifp->name);
          return CLI_ERROR;
        }

#ifdef HAVE_HAL
      /* Delete mirroring for port from hardware layer. */
      ret = hal_port_mirror_unset(ifp_to->ifindex, ifp->ifindex, 
                                  HAL_PORT_MIRROR_DIRECTION_TRANSMIT);
#endif /* HAVE_HAL */
      if (ret < 0) 
        {
          if (ret == RESULT_NO_SUPPORT) 
            cli_out(cli, "%%Port Mirroring is not supported\n");
          else 
            cli_out(cli, "%%Mirrored-to port doesn't exist. \n");

          /* Add port to mirror list. */
          port_mirroring_list_add (cli->zg, port_mirror_list, 
                                   ifp_to, ifp, HAL_PORT_MIRROR_DIRECTION_TRANSMIT);

          return CLI_ERROR;
        }
    }
  else 
    {
      cli_out(cli, "%%Interface %s does not exist\n", argv[0]);
      return CLI_ERROR;    
    }
   
  return CLI_SUCCESS;
}
CLI (show_mirror_interface,
     show_mirror_interface_cmd,
     "show mirror interface IFNAME",
     "Show",
     "Port Mirroring",
     "Source Interface to use",
     "Source Interface to use")
{
  struct listnode *n1, *n2;
  struct pm_node *node;
  struct interface *ifp = NULL;
  struct interface *ifptmp = NULL;
  int    found;
  char *pm_direction[4] = {"", "receive", "transmit", "both"};

  /* find interface */
  ifp = if_lookup_by_name(&cli->vr->ifm, argv[0]);
  if (ifp != NULL) 
    {
      LIST_LOOP (port_mirror_list, node, n1) 
        {
          if (ifp->ifindex == node->from->ifindex) 
            {
              found = 0;
              LIST_LOOP (cli->vr->ifm.if_list, ifptmp, n2) 
                {
                  if (ifptmp->ifindex == node->to->ifindex) 
                    {
                      cli_out(cli, "Mirror Test Port Name: %s\n", ifptmp->name);
                      found = 1;
                      break;
                    }
                }
              if (found) 
                {
                  cli_out(cli, "Mirror option: Enabled\n");
                  cli_out(cli, "Mirror direction: %s\n", pm_direction[node->direction]); 
                  cli_out(cli, "Monitored Port Name: %s\n", ifp->name);
                }
            }
        }
    }
  else
    {
      cli_out(cli,"%%interface %s does not exist\n",argv[0]);
      return CLI_ERROR;
    }
  return CLI_SUCCESS;
}

CLI (show_mirror,
     show_mirror_cmd,
     "show mirror",
     "Show",
     "Port Mirroring")
{
  struct listnode *n;
  struct listnode *m;
  struct pm_node *node;
  struct interface *ifp = cli->index;
  int found;
  char *pm_direction[4] = {"", "receive", "transmit", "both"};
  
  LIST_LOOP (port_mirror_list, node, m) 
    {
      found = 0;
      LIST_LOOP (cli->vr->ifm.if_list, ifp, n) 
        {
          if (ifp->ifindex == node->to->ifindex) 
            {
              cli_out(cli, "Mirror Test Port Name: %s\n", ifp->name);
              found = 1;
              break;
            }
        }
      if (found)
        {
          cli_out(cli, "Mirror option: Enabled\n");
          cli_out(cli, "Mirror direction: %s\n", pm_direction[node->direction]); 
          LIST_LOOP (cli->vr->ifm.if_list, ifp, n) {
            if (ifp->ifindex == node->from->ifindex) {
              cli_out(cli, "Monitored Port Name: %s\n", ifp->name);
              break;
            }
          }
        }
    }

  return CLI_SUCCESS;
}

int 
port_mirroring_write(struct cli *cli, int ifindex )
{
  int write = 0;
  struct listnode *n;
  struct pm_node *node = NULL;
  char *pm_direction[4] = {"", "receive", "transmit", "both"};
  
  LIST_LOOP(port_mirror_list, node, n) 
    {
      if (ifindex == node->to->ifindex) 
        {
          cli_out(cli, " mirror interface %s direction %s\n", 
                  node->from->name, pm_direction[node->direction]);
          write++;
        }
    } 
  return write;
}

static void
port_mirroring_cli_init (struct lib_globals * zg)
{
  struct cli_tree * ctree = zg->ctree;
  
  cli_install (ctree, INTERFACE_MODE, &mirror_interface_both_cmd);
  cli_install (ctree, INTERFACE_MODE, &mirror_interface_receive_cmd);
  cli_install (ctree, INTERFACE_MODE, &mirror_interface_transmit_cmd);
  cli_install (ctree, EXEC_MODE, &show_mirror_cmd);
  cli_install (ctree, EXEC_MODE, &show_mirror_interface_cmd);
  cli_install (ctree, INTERFACE_MODE, &show_mirror_interface_cmd);
  cli_install (ctree, INTERFACE_MODE, &no_mirror_interface_cmd);
  cli_install (ctree, INTERFACE_MODE, &no_mirror_interface_receive_cmd);
  cli_install (ctree, INTERFACE_MODE, &no_mirror_interface_transmit_cmd);
}

