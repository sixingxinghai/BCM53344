/* Copyright (C) 2003  All Rights Reserved.
   
L2 802.3x Flow Control

This module defines the CLI interface to the Layer 2 flow control 

The following commands are implemented:

ENABLE MODE:
* show flowcontrol interface IFNAME
    
INTERFACE MODE:
* flowcontrol (send|receive} {on|off} 
* no flowcontrol
* show flowcontrol

*/

#include "pal.h"
#include "lib.h"
#include "log.h"
#include "cli.h"

#include "l2lib.h"
#include "pal_types.h"
#include "l2_debug.h"
#include "nsm_flowctrl.h"
#include "hal_incl.h"
#ifdef HAVE_HA
#include "nsm_bridge.h"
#include "nsm_vlan.h"
#ifdef HAVE_QOS
#include "nsm_qos.h"
#include "nsm_qos_list.h"
#include "nsm_qos_cal_api.h"
#endif /* HAVE_QOS */
#endif /*HAVE_HA */


static void
flow_control_cli_init (struct lib_globals * zg);

struct list *flow_control_list = NULL;

void flow_control_init(struct lib_globals *zg)
{
  flow_control_list = list_new();
  if (flow_control_list == NULL) {
    zlog_debug(zg, "Cannot allocate flow control list\n");
    return;
  }

  flow_control_cli_init(zg);
} 

void flow_control_terminate(struct lib_globals *zg)
{
  if (flow_control_list)
    list_free(flow_control_list);
  flow_control_list = NULL;
#ifdef HAVE_HAL
  hal_flow_control_deinit();
#endif /* HAVE_HAL */
}
 
int
flow_control_get_direction (struct interface *port)
{
  struct listnode *n;
  struct flowctl_node *node = NULL;
  
  if (!flow_control_list)
    return 0;
                                                                   
  LIST_LOOP(flow_control_list, node, n) {
    if (node->port == port)
      return node->direction;
  }
  return 0;
}

static struct flowctl_node * 
flow_control_list_add(struct interface *port,
                      int    direction,
                      struct lib_globals *zg)
{
  struct listnode *n = NULL;
  struct flowctl_node *new_node = NULL;
 
  if (!flow_control_list)
    return NULL;
  
  LIST_LOOP(flow_control_list, new_node, n) {
    if (new_node->port == port)
      break;
  }  

  if (n == NULL) {
    new_node = XCALLOC(MTYPE_FLOW_CONTROL_ENTRY, sizeof(*new_node));
    if (new_node == NULL) {
      zlog_debug(zg, "Cannot allocate flow control data node\n");
      return NULL;
    }
    new_node->port = port;
    new_node->direction = 0;
#ifdef HAVE_DCB
    new_node->status = NSM_PFC_DISABLE;
#endif /*HAVE_DCB*/    
    n = listnode_add(flow_control_list, (void *)new_node);
    if (n == NULL) {
      zlog_debug(zg, "Cannot allocate flow control list node\n");
      return NULL;
    }
  }
   
  new_node->direction |= direction;
   
  return new_node;
}

static  struct flowctl_node *
flow_control_list_del(struct interface *port,
                      int    direction,
                      struct lib_globals *zg)
{
  struct listnode *n;
  int found = PAL_FALSE;
  struct flowctl_node *old_node = NULL;
  
  if (!flow_control_list)
    return NULL;
  
  LIST_LOOP(flow_control_list, old_node, n) {
    if (old_node->port == port)
      {
        found = PAL_TRUE;
        break;
      }
  } 

  if (old_node == NULL ||(! found))
    return NULL;
   
  old_node->direction &= ~direction;

  if (old_node->direction == 0) {
    listnode_delete(flow_control_list, (void *)old_node);
    return old_node;
  }
 
  return old_node;
}

struct flowctl_node* flow_control_list_lookup (char *ifname)
{
  struct flowctl_node *fn = NULL;

    if(! listnode_lookup (flow_control_list, fn))
      {
        if ( (fn) && (fn->port) )
          if (!pal_strncmp (fn->port->name,ifname,INTERFACE_NAMSIZ))
            return fn;
      }
 
  return fn;
}

int port_add_flow_control (struct interface *ifp, 
                           unsigned char direction)
{
  int ret = RESULT_NO_SUPPORT;
  struct flowctl_node *fn = NULL;

  if ((fn = flow_control_list_add(ifp, direction, ifp->vr->zg)) == NULL)
    return RESULT_ERROR;

#ifdef HAVE_HAL
#ifdef HAVE_DCB
  if (fn -> status != NSM_PFC_ENABLE)
#endif /*HAVE_DCB*/
    ret = hal_flow_control_set(ifp->ifindex, fn->direction);
#endif /* HAVE_HAL */

  if (ret == RESULT_NO_SUPPORT)
    {
      zlog_err (ifp->vr->zg, "Flow control is not supported\n");
      return ret;
    }

  if (ret < 0)
    {
      fn = flow_control_list_del(ifp, direction, ifp->vr->zg);
      if (fn && fn->direction == 0)
        XFREE(MTYPE_FLOW_CONTROL_ENTRY, fn);
      
      return RESULT_ERROR;
    }

#if defined HAVE_HA && defined HAVE_QOS
  nsm_cal_flow_cntrl_create (fn);
#endif /*HAVE_HA && HAVE_QOS */                                                                                                 
  return RESULT_OK;

}

int 
port_delete_flow_control (struct interface *ifp,
                          unsigned char direction)
{
  int ret = RESULT_NO_SUPPORT;
  struct flowctl_node *fn = NULL;
                                                                                                 
  if ((fn = flow_control_list_del(ifp, direction, ifp->vr->zg)) == NULL)
    return RESULT_ERROR;
                                                                                                 
#ifdef HAVE_HAL
  ret = hal_flow_control_set(ifp->ifindex, fn->direction);
#endif /* HAVE_HAL */
                                                                                                 
  if (ret == RESULT_NO_SUPPORT)
    {
      zlog_err (ifp->vr->zg, "Flow control is not supported\n");
      return ret;
    }
                                                                                                 
   if ( fn->direction == 0)
   {
#if defined HAVE_HA && defined HAVE_QOS

        nsm_cal_flow_cntrl_delete (fn);
#endif /*HAVE_HA && HAVE_QOS*/
      XFREE(MTYPE_FLOW_CONTROL_ENTRY, fn);
   }
#if defined HAVE_HA && defined HAVE_QOS
   else
     nsm_cal_flow_cntrl_modify (fn);
#endif /*HAVE_HA  && HAVE_QOS */

  return RESULT_OK;
}

int
nsm_flow_ctrl_pause_watermark_set (u_int32_t port, u_int16_t wm_pause)
{
#ifdef HAVE_USER_HSL
  int ret;

  ret = hal_flow_ctrl_pause_watermark_set (port, wm_pause);

  if (ret != HAL_SUCCESS)
    return ret;
#endif

  return 0;

}

#ifdef HAVE_SMI
int 
get_flow_control_statistics (struct interface *ifp, 
                             enum smi_flow_control_dir *direction,
                             int *rxpause, int *txpause)
{ 
  struct flowctl_node *node = NULL;
  struct listnode *n;
  unsigned char dir;
  int found = 0;

  if (!flow_control_list)
    {
      return RESULT_ERROR;
    }

  if (flow_control_list->count > 0) 
    {
      LIST_LOOP (flow_control_list, node, n) {
      if (ifp->ifindex == node->port->ifindex) {
      found = 1;
#ifdef HAVE_HAL
      hal_flow_control_statistics (ifp->ifindex, (unsigned char *) &dir, 
                                   rxpause, txpause);
#endif /* HAVE_HAL */
       break;
       }
     }
    }
 
  if ((flow_control_list->count == 0) || !found)
   {
     *direction = SMI_FLOW_CONTROL_UNCONFIGURE;
     *rxpause = 0;
     *txpause = 0;
      return RESULT_OK;
    }

  if (found && node)
  {
    if ((node->direction & HAL_FLOW_CONTROL_SEND) && 
         !(node->direction & HAL_FLOW_CONTROL_RECEIVE))
      *direction = SMI_FLOW_CONTROL_SEND;
    else if (!(node->direction & HAL_FLOW_CONTROL_SEND) && 
              (node->direction & HAL_FLOW_CONTROL_RECEIVE)) 
      *direction = SMI_FLOW_CONTROL_RECEIVE;
    else if ((node->direction & HAL_FLOW_CONTROL_SEND) 
             && (node->direction & HAL_FLOW_CONTROL_RECEIVE)) 
      *direction = SMI_FLOW_CONTROL_BOTH;
    else
      *direction = SMI_FLOW_CONTROL_OFF;
  }

  return RESULT_OK; 
}

int 
flow_control_get_interface (struct interface *ifp, 
                            enum smi_flow_control_dir *direction)
{
  struct listnode *n;
  struct flowctl_node *node;
  int rxpause, txpause;

 if ( !flow_control_list )
    {
      return RESULT_ERROR;
    }

 if (flow_control_list->count > 0) 
   {
      LIST_LOOP (flow_control_list, node, n) {
      if (ifp->ifindex == node->port->ifindex) {
#ifdef HAVE_HAL
      hal_flow_control_statistics (ifp->ifindex, (u_char *) direction, 
                                   &rxpause, &txpause);
#endif /* HAVE_HAL */
       }
     }
    }
    else
      return RESULT_ERROR;

return RESULT_OK;
}
#endif /* HAVE_SMI */

#ifdef HAVE_DCB
int
nsm_flow_control_disable_if (struct interface *ifp)
{
  struct listnode *n = NULL;
  struct flowctl_node *new_node = NULL;
  int ret = RESULT_NO_SUPPORT;
  
  if (!flow_control_list)
    return RESULT_ERROR;

  LIST_LOOP(flow_control_list, new_node, n) 
    {
      if (new_node->port == ifp)
        {
#ifdef HAVE_HAL  
          ret = hal_flow_control_set(ifp->ifindex, HAL_FLOW_CONTROL_OFF);
          if (ret < 0)
            return RESULT_ERROR;
#endif /* HAVE_HAL */        
          new_node->status = NSM_PFC_ENABLE;
          break;  
        }
    }

  return RESULT_OK;
}

int
nsm_flow_control_enable_if (struct interface *ifp)
{
  struct listnode *n = NULL;
  struct flowctl_node *new_node = NULL;
  int direction = 0;  
  int ret = RESULT_NO_SUPPORT;
  
  if (!flow_control_list)
    return RESULT_ERROR;

   LIST_LOOP(flow_control_list, new_node, n) 
    {
      if (new_node->port == ifp)
        {
          if (new_node->direction & HAL_FLOW_CONTROL_SEND)
            direction = direction | HAL_FLOW_CONTROL_SEND;

          if (new_node->direction & HAL_FLOW_CONTROL_RECEIVE)
            direction = direction | HAL_FLOW_CONTROL_RECEIVE;

#ifdef HAVE_HAL  
          ret = hal_flow_control_set(ifp->ifindex, direction);
          if (ret < 0)
            return RESULT_ERROR;
#endif /* HAVE_HAL */
          new_node->status = NSM_PFC_DISABLE;
          break;  
        }
    }

  return RESULT_OK;
}
#endif /*HAVE_DCB*/

CLI (flow_ctrl_set,
     flow_ctrl_set_cmd,
     "flowcontrol wmpause <0-255> wmcancel <0-255>",
     "IEEE 802.3x Flow Control",
     "Watermark pause command",
     "Watermark pause value",
     "Watermark cancel command",
     "Watermark cancel value"
    )
{
  int ret;
  u_int32_t pause_id = 0;
  u_int32_t cancel_id = 0;
  u_int32_t wm_pause = 0;
  u_int32_t port = 0;

  /* get the interface Index */
  struct interface *ifp = cli->index;

  CLI_GET_UINT32_RANGE ("pause", pause_id, argv[0], 0, 255);
  CLI_GET_UINT32_RANGE ("cancel", cancel_id, argv[1], 0, 255);

  pause_id = (pause_id << 8);
  wm_pause = (pause_id | cancel_id);

  port = ifp->ifindex;

  ret = nsm_flow_ctrl_pause_watermark_set (port, wm_pause);

  if (ret != HAL_SUCCESS)
   {
      cli_out (cli, "%% Error in setting the flow control values\n");
      return CLI_ERROR;
   }

  return CLI_SUCCESS;
}

CLI (flowcontrol_send_on,
     flowcontrol_send_on_cmd,
     "flowcontrol send on",
     "IEEE 802.3x Flow Control",
     "Flow control on send",
     "Turn on flow control")
{
  int    ret = RESULT_NO_SUPPORT;
  struct interface *ifp = cli->index;
   
  if (LAYER2_DEBUG(proto, CLI_ECHO))
    zlog_debug(cli->zg, "flow control on interface %s on send",ifp->name);

  if (if_lookup_by_name (&cli->vr->ifm, ifp->name) == NULL)
    {
      cli_out(cli, "Flow control configuration on interface %s could not be"
              " stored\n", ifp->name);
      return CLI_ERROR;
    }

  ret = port_add_flow_control (ifp, HAL_FLOW_CONTROL_SEND);

  if (ret == RESULT_NO_SUPPORT)
    cli_out(cli, "Flow control is not supported\n");

  if (ret < 0)
    {
      cli_out(cli, "%% Error setting flow control send on\n");
      return CLI_ERROR;
    }
   
  return CLI_SUCCESS;
}

CLI (flowcontrol_send_off,
     flowcontrol_send_off_cmd,
     "flowcontrol send off",
     "IEEE 802.3x Flow Control",
     "Flow control on send",
     "Turn off flow control")
{
  int    ret = RESULT_NO_SUPPORT;
  struct interface *ifp = cli->index;
  struct flowctl_node *fn;
 
  if (LAYER2_DEBUG(proto, CLI_ECHO))
    zlog_debug(cli->zg, "flow control on interface %s on send",ifp->name);

  if ((fn = flow_control_list_del(ifp, HAL_FLOW_CONTROL_SEND, cli->zg)) == NULL)
    {
      cli_out(cli, "Flow control send is not on\n");
      return CLI_ERROR;
    }

#ifdef HAVE_HAL
  ret = hal_flow_control_set(ifp->ifindex, fn->direction);
#endif /* HAVE_HAL */

  if (ret == RESULT_NO_SUPPORT)
    cli_out(cli, "Flow control is not supported\n");

  if (fn->direction == 0)
  {
#if defined HAVE_HA && defined HAVE_QOS
    nsm_cal_flow_cntrl_delete(fn);
#endif /*HAVE_HA && HAVE_QOS */
    XFREE(MTYPE_FLOW_CONTROL_ENTRY, fn);
  }
#if defined HAVE_HA && defined HAVE_QOS
  else
     nsm_cal_flow_cntrl_modify(fn);
#endif /*HAVE_HA && HAVE_QOS */
 
  return CLI_SUCCESS;
}

CLI (flowcontrol_receive_on,
     flowcontrol_receive_on_cmd,
     "flowcontrol receive on",
     "IEEE 802.3x Flow Control",
     "Flow control on receive",
     "Turn on flow control")
{
  int    ret = RESULT_NO_SUPPORT;
  struct interface *ifp = cli->index;
    
  if (LAYER2_DEBUG(proto, CLI_ECHO))
    zlog_debug(cli->zg, "flow control on interface %s on send",ifp->name);

  if (if_lookup_by_name (&cli->vr->ifm, ifp->name) == NULL)
    {
      cli_out(cli, "Flow control configuration on interface %s could not "
              "be stored\n", ifp->name);
      return CLI_ERROR;
    }

  ret = port_add_flow_control (ifp, HAL_FLOW_CONTROL_RECEIVE);

  if (ret == RESULT_NO_SUPPORT)
    cli_out(cli, "Flow control is not supported\n");

  if (ret < 0)
    {
      cli_out(cli, "%% Error setting flow control receive on\n");
      return CLI_ERROR;
    }

  return CLI_SUCCESS;
}

CLI (flowcontrol_receive_off,
     flowcontrol_receive_off_cmd,
     "flowcontrol receive off",
     "IEEE 802.3x Flow Control",
     "Flow control on receive",
     "Turn off flow control")
{
  int    ret = RESULT_NO_SUPPORT;
  struct interface *ifp = cli->index;
  struct flowctl_node *fn;
  
  if (LAYER2_DEBUG(proto, CLI_ECHO))
    zlog_debug(cli->zg, "flow control on interface %s on send",ifp->name);

  if ((fn = flow_control_list_del(ifp, HAL_FLOW_CONTROL_RECEIVE,
                                  cli->zg)) == NULL)
    {
      cli_out(cli, "Flow control receive is not on\n");
      return CLI_ERROR;
    }

#ifdef HAVE_HAL
  ret = hal_flow_control_set(ifp->ifindex, fn->direction);
#endif /* HAVE_HAL */
   
  if (ret == RESULT_NO_SUPPORT)
    cli_out(cli, "Flow control is not supported\n");

  if (fn->direction == 0)
  {
#if defined HAVE_HA && defined HAVE_QOS
    nsm_cal_flow_cntrl_delete(fn);
#endif /*HAVE_HA && HAVE_QOS */
    XFREE(MTYPE_FLOW_CONTROL_ENTRY, fn);
  }
#if defined HAVE_HA && defined HAVE_QOS
  else
     nsm_cal_flow_cntrl_modify(fn);
#endif /*HAVE_HA && HAVE_QOS */
  
  return CLI_SUCCESS;
}

CLI (flowcontrol_both_on,
     flowcontrol_both_on_cmd,
     "flowcontrol both",
     "IEEE 802.3x Flow Control",
     "Flow control on send and receive",
     "Turn on flow control")
{
  int    ret = RESULT_NO_SUPPORT;
  struct interface *ifp = cli->index;
  int direction = HAL_FLOW_CONTROL_RECEIVE | HAL_FLOW_CONTROL_SEND;
    
  if (LAYER2_DEBUG(proto, CLI_ECHO))
    zlog_debug(cli->zg, "flow control on interface %s on both",ifp->name);

  if (if_lookup_by_name (&cli->vr->ifm, ifp->name) == NULL)
    {
      cli_out(cli, "Flow control configuration on interface %s could not "
              "be stored\n", ifp->name);
      return CLI_ERROR;
    }

  ret = port_add_flow_control (ifp, direction);

  if (ret == RESULT_NO_SUPPORT)
    cli_out(cli, "Flow control is not supported\n");

  if (ret < 0)
    {
      cli_out(cli, "%% Error setting flow control\n");
      return CLI_ERROR;
    }

  return CLI_SUCCESS;
}

CLI (no_flowcontrol,
     no_flowcontrol_cmd,
     "no flowcontrol",
     CLI_NO_STR,
     "IEEE 802.3x Flow Control")
{
  int    ret = RESULT_NO_SUPPORT;
  struct interface *ifp = cli->index;
  struct flowctl_node *fn;
  
  if (LAYER2_DEBUG(proto, CLI_ECHO))
    zlog_debug(cli->zg, "flow control on interface %s on send",ifp->name);

  if ((fn = flow_control_list_del (ifp, 
    HAL_FLOW_CONTROL_SEND|HAL_FLOW_CONTROL_RECEIVE, cli->zg)) == NULL)
    {
      cli_out(cli, "Flow control is not on\n");
      return CLI_ERROR;
    }

#ifdef HAVE_HAL  
  ret = hal_flow_control_set(ifp->ifindex, HAL_FLOW_CONTROL_OFF);
#endif /* HAVE_HAL */
   
  if (ret == RESULT_NO_SUPPORT)
    cli_out(cli, "Flow control is not supported\n");

  if (fn->direction == 0)
  {
#if defined HAVE_HA && defined HAVE_QOS
    nsm_cal_flow_cntrl_delete(fn);
#endif /*HAVE_HA && HAVE_QOS */
    XFREE(MTYPE_FLOW_CONTROL_ENTRY, fn);
  }
#if defined HAVE_HA && defined HAVE_QOS
  else
     nsm_cal_flow_cntrl_modify(fn);
#endif /*HAVE_HA && HAVE_QOS */
  return CLI_SUCCESS;
}

CLI (show_flowcontrol,
     show_flowcontrol_cmd,
     "show flowcontrol",
     CLI_SHOW_STR,
     "IEEE 802.3x Flow Control")
{
  struct listnode *n;
  struct flowctl_node *node;
  unsigned char direction = 0;
  int rxpause = 0, txpause = 0;
  
  if ( !flow_control_list )
    {
      cli_out (cli, "Flow control not initialised\n");
      return CLI_ERROR;
    }
  
  if (flow_control_list->count > 0) {
    cli_out (cli, "Port    Send FlowControl    Receive FlowControl  RxPause TxPause\n");
    cli_out (cli, "        admin   oper        admin   oper\n");
    cli_out (cli, "-----   ------- --------    ------- --------     "
             "------- -------\n"); 
    LIST_LOOP (flow_control_list, node, n) {
#ifdef HAVE_HAL
      hal_flow_control_statistics (node->port->ifindex, &direction, &rxpause, 
                                   &txpause);
#endif /* HAVE_HAL */
      cli_out (cli, "%-8s%-8s%-12s%-8s%-12s%8d%8d\n", node->port->name, 
               (node->direction&HAL_FLOW_CONTROL_SEND)?"on":"off",
               (direction&HAL_FLOW_CONTROL_SEND)?"on":"off",
               (node->direction&HAL_FLOW_CONTROL_RECEIVE)?"on":"off",
               (direction&HAL_FLOW_CONTROL_RECEIVE)?"on":"off",
               rxpause, txpause); 
    }
  }

  return CLI_SUCCESS;
}

CLI (show_flowcontrol_interface,
     show_flowcontrol_interface_cmd,
     "show flowcontrol interface IFNAME",
     CLI_SHOW_STR,
     "IEEE 802.3x Flow Control",
     "Interface",
     "Interface to display")
{
  struct listnode *n1, *n2;
  struct flowctl_node *node;
  struct interface *ifp;
  unsigned char direction;
  int rxpause, txpause;

  if ( !flow_control_list )
    {
      cli_out(cli, "Flow control not initialised\n");
      return CLI_ERROR;
    }

  /*    Find interface */
  LIST_LOOP (cli->vr->ifm.if_list, ifp, n1) {
    if (!(pal_strncmp (ifp->name, argv[0], INTERFACE_NAMSIZ))) {

      if (flow_control_list->count > 0) {
        cli_out(cli, "Port    Send FlowControl    Receive FlowControl  "
                "RxPause TxPause\n");
        cli_out(cli, "        admin   oper        admin   oper\n");                        
        cli_out(cli, "-----   ------- --------    ------- --------     "
                "------- -------\n"); 
        LIST_LOOP (flow_control_list, node, n2) {
          if (ifp->ifindex == node->port->ifindex) {
#ifdef HAVE_HAL
            hal_flow_control_statistics(ifp->ifindex, &direction, &rxpause, 
                                        &txpause);
#endif /* HAVE_HAL */
            cli_out(cli, "%-8s%-8s%-12s%-8s%-12s%8d%8d\n", ifp->name, 
                    (node->direction&HAL_FLOW_CONTROL_SEND)?"on":"off",
                    (direction&HAL_FLOW_CONTROL_SEND)?"on":"off",
                    (node->direction&HAL_FLOW_CONTROL_RECEIVE)?"on":"off",
                    (direction&HAL_FLOW_CONTROL_RECEIVE)?"on":"off",
                    rxpause, txpause);
            return CLI_SUCCESS; 
          }
        }
      }
    }
  }

  return CLI_SUCCESS;
}

int 
flow_control_write(struct cli *cli, int ifindex )
{
  int write = 0;
  struct listnode *n;
  struct flowctl_node *node = NULL;
  
  if ( !flow_control_list )
    return write;
 
  LIST_LOOP(flow_control_list, node, n) 
    {
      if (ifindex == node->port->ifindex) 
        {
          if ( (node->direction & HAL_FLOW_CONTROL_SEND) )
            {
              cli_out(cli, " flowcontrol send on\n");
              write++;
            }
          if ( (node->direction & HAL_FLOW_CONTROL_RECEIVE) ) 
            {
              cli_out(cli, " flowcontrol receive on\n");
              write++;
            }
        }
    } 
  return write;
}

void
flow_control_cli_init (struct lib_globals * zg)
{
  struct cli_tree * ctree = zg->ctree;
  
  cli_install (ctree, INTERFACE_MODE, &flow_ctrl_set_cmd);  
  cli_install (ctree, INTERFACE_MODE, &flowcontrol_send_on_cmd);
  cli_install (ctree, INTERFACE_MODE, &flowcontrol_send_off_cmd);
  cli_install (ctree, INTERFACE_MODE, &flowcontrol_receive_on_cmd);
  cli_install (ctree, INTERFACE_MODE, &flowcontrol_receive_off_cmd);
  cli_install (ctree, INTERFACE_MODE, &flowcontrol_both_on_cmd);
  cli_install (ctree, INTERFACE_MODE, &no_flowcontrol_cmd);
  cli_install (ctree, EXEC_MODE, &show_flowcontrol_cmd);
  cli_install (ctree, EXEC_MODE, &show_flowcontrol_interface_cmd);

}

