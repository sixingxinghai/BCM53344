/* Copyright (C) 2009  All Rights Reserved */

/**@file nsm_dcb_cli.c
   @brief This nsm_dcb_cli.c file defines CLIs related to DCB
          functionality.
*/

#include "pal.h"
#include "lib.h"

#ifdef HAVE_HAL
#include "hal_incl.h"
#endif /* HAVE_HAL */

#include "avl_tree.h"
#include "nsmd.h"
#include "nsm_message.h"
#include "nsm_server.h"
#include "nsm_interface.h"
#include "nsm_bridge_cli.h"
#ifdef HAVE_VLAN_CLASS
#include "nsm_vlanclassifier.h"
#endif /* HAVE_VLAN_CLASS */

#include "nsm_router.h"
#include "nsm_vlanclassifier.h"

#ifdef HAVE_DCB 
#include "nsm_dcb.h"
#include "nsm_dcb_cli.h"
#include "nsm_dcb_api.h"
#include "nsm_dcb_show.h"

/* Macro definition to check the DCB mode */
#define CLI_GET_DCB_MODE(T,STR)                                           \
  do {                                                                    \
    /* Check DCB mode. */                                                 \
    if (pal_strncmp ((STR), "o", 1) == 0)                                 \
      (T) = NSM_DCB_MODE_ON;                                              \
    else if (pal_strncmp ((STR), "a", 1) == 0)                            \
      (T) = NSM_DCB_MODE_AUTO;                                            \
    else                                                                  \
      return nsm_dcb_cli_return (cli, NSM_DCB_API_SET_ERR_INVALID_VALUE); \
  } while (0)

/* Macro definition to check the protocol type */
#define CLI_GET_DCB_PROTO_TYPE(P, STR)                                    \
  do {                                                                    \
    /* Check the Protocol type */                                         \
    if (pal_strncmp ((STR), "t", 1) == 0)                                 \
      (P) = 1;                                                            \
    else if (pal_strncmp ((STR), "u", 1) == 0)                            \
      (P) = 2;                                                            \
    else if (pal_strncmp ((STR), "b", 1) == 0)                            \
      (P) = 3;                                                            \
    else if (pal_strncmp ((STR), "n", 1) == 0)                            \
      (P) = 4;                                                            \
    else                                                                  \
      return nsm_dcb_cli_return (cli, NSM_DCB_API_SET_ERR_INVALID_VALUE); \
     } while (0) 

/**@brief This function displays the error message based on the return value.
   @param cli - Pointer to CLI
   @param ret - NSM DCB API Return Value 
   @return CLI_SUCCESS/CLI_ERROR
*/
s_int32_t 
nsm_dcb_cli_return (struct cli *cli, s_int32_t ret)
{
  const char *str;
  
  NSM_DCB_FN_ENTER ();

  switch (ret)
    {
      case NSM_DCB_API_SET_SUCCESS:
        NSM_DCB_FN_EXIT (CLI_SUCCESS);

      case NSM_DCB_API_SET_ERR_NO_NM:
        str = "NSM master not found";
        break;

      case NSM_DCB_API_SET_ERR_NO_DCBG:
        str = "DCB Bridge not found";
        break;
 
      case NSM_DCB_API_SET_ERR_INTERFACE:
        str = "Interface not found";
        break;
 
      case NSM_DCB_API_SET_ERR_L2_INTERFACE:
        str = "Interface should be L2 interface";
        break;

      case NSM_DCB_API_SET_ERR_NO_DCB:
        str = "DCB is not enabled on switch";
        break;

      case NSM_DCB_API_SET_ERR_NO_ETS:
        str = "ETS is not enabled on switch";
        break;

      case NSM_DCB_API_SET_ERR_DCB_IF_INIT:
        str = "Can not enable DCB on interface";
        break;

      case NSM_DCB_API_SET_ERR_ETS_IF_INIT:
        str = "Can not enable ETS on interface";
        break;

      case NSM_DCB_API_SET_ERR_INTERFACE_NO_DCB:
        str = "DCB is not enabled on interface";
        break;

      case NSM_DCB_API_SET_ERR_INTERFACE_NO_ETS:
        str = "ETS is not enabled on interface";
        break;

      case NSM_DCB_API_SET_ERR_DCB_INTERFACE:
        str = "DCB interface not found, DCB may be disabled on interface";
        break;

      case NSM_DCB_API_SET_ERR_ETS_INTERFACE:
        str = "ETS interface not found";
        break;

      case NSM_DCB_API_SET_ERR_DCB_EXISTS:
        str = "DCB is already enabled";
        break;
      
      case NSM_DCB_API_SET_ERR_ETS_EXISTS:
        str = "ETS is already enabled";
        break;

      case NSM_DCB_API_SET_ERR_DUPLICATE_ARGS:
        str = "Duplicate arguments are not allowed";
        break;

      case NSM_DCB_API_SET_ERR_WRONG_TCG:
        str = "Priority belongs to different TCG";
        break;

      case NSM_DCB_API_SET_ERR_DIFF_PFC:
        str = "All priorities in TCG do not have same PFC enable/disable"
              "state";
        break;

      case NSM_DCB_API_SET_ERR_TCG_PRI_EXISTS:
        str = "Priority already belongs to TCG";
        break;
    
      case NSM_DCB_API_SET_ERR_WRONG_BW:
        str = "Total bandwidth percentage should be 100";
        break;

      case NSM_DCB_API_SET_ERR_APP_PRI_NOT_FOUND:
        str = "Application priority not found";
        break;

      case NSM_DCB_API_SET_ERR_NO_SERV:
        str = "Well-known port for given service not found";
        break;

      case NSM_DCB_API_SET_ERR_NO_SUPPORT:
        str = "Feature is not supported";
        break;
 
      case NSM_DCB_API_SET_ERR_NO_MEM:
        str = "Out of memory";
        break;

      case NSM_DCB_API_SET_ERR_INVALID_VALUE:
        str = "Invalid Argument";
        break;
 
      case NSM_DCB_API_SET_ERR_ETS_NO_TCGS:
        str = "TCGs are not configured on Interface";
        break;

      case NSM_BRIDGE_ERR_NOTFOUND:
        str = "Bridge not found";
        break;

      case NSM_DCB_API_SET_ERR_DISPLAY_NO_TCG:
        str = "TCG is not configured on any of the interface";
        break;

      case NSM_DCB_API_SET_ERR_DISPLAY_NO_APPL_PRI:
        str = "Application priority is not configured on the interface";
        break;

      case NSM_DCB_API_SET_ERR_NO_PFC:
        str = "PFC is not enabled on switch";
        break;
 
      case NSM_DCB_API_SET_ERR_INTERFACE_NO_PFC:
        str = "PFC is not enabled on interface";
        break;
                    
      case NSM_DCB_API_SET_ERR_PFC_IF_INIT:
        str = "Can not enable PFC on interface";
        break;
 
      case NSM_DCB_API_SET_ERR_PFC_INTERFACE:
        str = "PFC interface not found";
        break;
    
      case NSM_DCB_API_SET_ERR_RESV_VALUE:
        str = "TCGID 8 to 14 are reserved";
        break;

      case NSM_DCB_API_SET_ERR_INTF_DUPLEX:
        str = "PFC Interface is not in full-duplex mode";
        break;

      case NSM_DCB_API_FAIL_TO_GET_DUPLEX:
        str = "Unable to get duplex mode of PFC Interface";
        break;
      
      case NSM_DCB_API_SET_ERR_PFC_EXISTS:
        str = "PFC is enabled already";
        break;

      case NSM_DCB_API_SET_ERR_EXCEED_PFC_CAP:
        str = "Unable to enable on all/some priorities as PFC CAP exceeds";
        break;
       
      case NSM_DCB_API_SET_ERR_PFC_CAP:
        str = "Unable to set PFC Cap as enabled priorities are more";
        break;

      case NSM_DCB_API_SET_ERR_LINK_FLOW_CNTL:
        str = "Unable to set link-level flow control";
        break;
        
      case NSM_DCB_API_UNSET_ERR_LINK_FLOW_CNTL:
        str = "Unable to unset link-level flow control";
        break;
      case NSM_DCB_API_SET_WARN_RECONFIG :
        str = "Bandwidth Utilization < 100%. BW needs to be reconfigured.";
        break;       

      case NSM_DCB_API_SET_WARN_DIFF_TCG_PFC:
        str = "Before changing the PFC State remove it from TCG";
        break;

      default:
        NSM_DCB_FN_EXIT (CLI_ERROR);
    }

   cli_out (cli, "%% %s\n", str);
  
   NSM_DCB_FN_EXIT (CLI_ERROR);
}

/**@brief This function writes switch level configuration
   @param cli - Pointer to CLI
   @return NSM_DCB_API_SET_SUCCESS
*/
s_int32_t
nsm_dcb_config_write (struct cli *cli)
{
  struct nsm_master *nm;
  struct nsm_dcb_bridge *dcbg;
  struct nsm_bridge_master *master;
  struct nsm_bridge *bridge;

  NSM_DCB_FN_ENTER ();

  /* Look for NSM master */
  nm = nsm_master_lookup_by_id (nzg, cli->vr->id);
  if (! nm)
    NSM_DCB_FN_EXIT (NSM_DCB_API_SET_ERR_NO_NM);

  /* look for the bridge master */
  master = nsm_bridge_get_master (nm);
  if (! master)
    NSM_DCB_FN_EXIT (NSM_BRIDGE_ERR_NOTFOUND);

  bridge = master->bridge_list;
  while (bridge)
    {
      /* Look for the dcb bridge structure */
      dcbg = bridge->dcbg;
      if (! dcbg)
        {
          zlog_err (nzg, "DCB bridge structure not found\n");
          NSM_DCB_FN_EXIT (NSM_DCB_API_SET_ERR_NO_DCBG);
        }
  
      if (CHECK_FLAG (dcbg->dcb_global_flags, NSM_DCB_ENABLE))
        {
#ifdef HAVE_DEFAULT_BRIDGE
          if (bridge->is_default)
            cli_out (cli, "data-center-bridging enable\n");
          else
#endif /*HAVE_DEFAULT_BRIDGE */
            cli_out (cli, "data-center-bridging enable bridge %s\n", 
                           bridge->name);
        }
      else 
        {
#ifdef HAVE_DEFAULT_BRIDGE
          if (bridge->is_default)
             cli_out (cli, "no data-center-bridging enable\n");
          else
#endif /* HAVE_DEFAULT_BRIDGE */
             cli_out (cli, "no data-center-bridging enable bridge %s\n",
                            bridge->name);
          goto NEXT;
        }

      if (! CHECK_FLAG (dcbg->dcb_global_flags, NSM_DCB_ETS_ENABLE))
        {
#ifdef HAVE_DEFAULT_BRIDGE
          if (bridge->is_default)
            cli_out (cli, "no enhanced-transmission-selection enable\n");
          else
#endif /* HAVE_DEFAULT_BRIDGE */
            cli_out (cli, "no enhanced-transmission-selection" 
                          " enable bridge %s\n", bridge->name);
        }

      if (!CHECK_FLAG (dcbg->dcb_global_flags, NSM_DCB_PFC_ENABLE))
        {
#ifdef HAVE_DEFAULT_BRIDGE          
          if (bridge->is_default)
            cli_out (cli, "no priority-flow-control enable\n");
          else
#endif /* HAVE_DEFAULT_BRIDGE */
            cli_out (cli, "no priority-flow-control enable"
                          " bridge %s\n", bridge->name);
        }

NEXT :  
       cli_out (cli, "!\n");
       bridge = bridge->next;
    }

  NSM_DCB_FN_EXIT (NSM_DCB_API_SET_SUCCESS);
}

/**@brief This function converts the selector type into string
   @param sel - protocol selector value
   @return string - name of the protocol
*/
static char *
nsm_dcb_selector_to_string (u_int8_t sel)
{
  char *str;

  NSM_DCB_FN_ENTER ();

  switch (sel)
    {
      case NSM_DCB_PROTO_ETHERTYPE:
        str = "ethertype";
        break;
      case NSM_DCB_PROTO_TCP:
        str = "tcp";
        break;
      case NSM_DCB_PROTO_UDP:
        str = "udp";
        break;
      case NSM_DCB_PROTO_BOTH_TCP_UDP:
        str = "both-tcp-udp";
        break;
      case NSM_DCB_PROTO_NO_TCP_UDP:
        str = "neither-tcp-udp";
        break;
      default:
        str = "";
        break;
    }

  NSM_DCB_FN_EXIT (str);
}

/**@brief This function writes the application priority configuration on 
          interface.
   @param node_info - Pointer to application priority structure
   @param cli - Pointer to cli
   @return NSM_DCB_API_SET_SUCCESS
*/
s_int32_t
nsm_dcb_if_avl_traverse_appl_pri_config_write (void_t *node_info, void_t *arg1)
{
  struct nsm_dcb_application_priority *app_pri;
  struct cli *cli;
  char *sel_str;
  struct pal_servent *sp = NULL;
  char ether_value[7]; 
  char *ether_str;

  NSM_DCB_FN_ENTER ();

  if (! (app_pri = (struct nsm_dcb_application_priority *) node_info) || 
      ! (cli = (struct cli *) arg1))
    NSM_DCB_FN_EXIT (NSM_DCB_API_SET_ERR_INVALID_VALUE);

  /* Get the Protocol Selector (Ethertype/tcp/udp etc) String */
  sel_str = nsm_dcb_selector_to_string (app_pri->sel);
  if (pal_strncmp (sel_str, "", 1) == 0)
    NSM_DCB_FN_EXIT (NSM_DCB_API_SET_ERR_INVALID_VALUE);

  /* Based on the application priority mapping type display the CLI in 
   * show running configuration. 
   * Map by well known port number. 
   */
  if (app_pri->mapping_type == NSM_DCB_MAP_PORT_NO)
    cli_out (cli, " %s port-no %d priority %d\n", 
                   sel_str, app_pri->proto_id, app_pri->priority);
  /* Map by well known tcp udp service names */
  else if (app_pri->mapping_type == NSM_DCB_MAP_SERV_NAME)
    {
       if (app_pri->sel == NSM_DCB_PROTO_TCP 
           || app_pri->sel == NSM_DCB_PROTO_UDP)
         sp = (struct pal_servent *) pal_getservbyport (
                                     pal_hton16 (app_pri->proto_id), 
                                     sel_str);
       else if (app_pri->sel == NSM_DCB_PROTO_BOTH_TCP_UDP)
         {
           sp = (struct pal_servent *) pal_getservbyport (
                                       pal_hton16 (app_pri->proto_id),
                                       "tcp");
           if (! sp)
             sp = (struct pal_servent *) pal_getservbyport (
                                         pal_hton16 (app_pri->proto_id),
                                         "udp");
        }

      if (sp)
        cli_out (cli, " %s service-name %s priority %d\n",
                       sel_str, sp->s_name, app_pri->priority);
    }
  /* Map by ethertype value */
  else if (app_pri->mapping_type == NSM_DCB_MAP_ETHERTYPE_VALUE)
    {
       pal_snprintf (ether_value, sizeof (ether_value), "0x%.04x", 
                     app_pri->proto_id);
       cli_out (cli, " ethertype value %s priority %d\n", ether_value, 
                                                          app_pri->priority); 
    }
  /* Map by ethertype string or protocol name */
  else if (app_pri->mapping_type == NSM_DCB_MAP_ETHERTYPE_STR)
    {
      ether_str = nsm_classifier_proto_str (app_pri->proto_id);
      if (ether_str)
        cli_out (cli, " ethertype name %s priority %d\n", ether_str, 
                                                          app_pri->priority); 
    }
  
  NSM_DCB_FN_EXIT (NSM_DCB_API_SET_SUCCESS);
}

/**@brief This function writes the interface level DCB configuration.
   @param cli - Pointer to CLI
   @param ifname - Name of Interface
   @return NSM_DCB_API_SET_SUCCESS
*/
s_int32_t
nsm_dcb_if_config_write (struct cli *cli, char *ifname)
{

  struct nsm_master *nm;
  struct nsm_dcb_bridge *dcbg;
  struct interface *ifp;
  struct nsm_dcb_if *dcbif = NULL, tmp_dcbif;
  struct nsm_bridge_port *br_port;
  struct nsm_bridge *bridge;
  struct avl_node *node;
  struct nsm_if *zif;
  u_int8_t tcgid, pri;
  char bw_str[100], tmp_bw_str[10];

  NSM_DCB_FN_ENTER ();

  /* Look for NSM master */
  nm = nsm_master_lookup_by_id (nzg, cli->vr->id);
  if (! nm)
    NSM_DCB_FN_EXIT (NSM_DCB_API_SET_ERR_NO_NM);

  /* Look for the interface pointer */
  ifp = if_lookup_by_name (&nm->vr->ifm, ifname);
  if (! ifp)
    NSM_DCB_FN_EXIT (NSM_DCB_API_SET_ERR_INTERFACE);
  
  /* get the bridge information */
  zif = (struct nsm_if *) ifp->info;
  if (! zif)
    NSM_DCB_FN_EXIT (NSM_BRIDGE_ERR_NOTFOUND);

  if (! (br_port = zif->switchport))
    NSM_DCB_FN_EXIT (NSM_BRIDGE_ERR_NOTFOUND);

  if (!(bridge = zif->bridge))
    NSM_DCB_FN_EXIT (NSM_BRIDGE_ERR_NOTFOUND);

  /* Look for the dcb bridge structure */
  dcbg = bridge->dcbg;
  if (! dcbg)
    NSM_DCB_FN_EXIT (NSM_DCB_API_SET_ERR_NO_DCBG);

  tmp_dcbif.ifp = ifp;

  /* Find the DCB interface */
  node = avl_search (dcbg->dcb_if_list, (void_t *) &tmp_dcbif);
  if (node)
    dcbif = (struct nsm_dcb_if *) node->info;

  if (! dcbif)
    NSM_DCB_FN_EXIT (NSM_DCB_API_SET_ERR_DCB_INTERFACE);

  if (CHECK_FLAG (dcbif->dcb_if_flags, NSM_DCB_IF_ENABLE))
    cli_out (cli, " data-center-bridging enable\n"); 
  else
    NSM_DCB_FN_EXIT (NSM_DCB_API_SET_SUCCESS); 

  if (! CHECK_FLAG (dcbif->dcb_if_flags, NSM_DCB_IF_ETS_ENABLE))
    {
      cli_out (cli, " no enhanced-transmission-selection enable\n");
      NSM_DCB_FN_EXIT (NSM_DCB_API_SET_SUCCESS);
    } 

  if (dcbif->dcb_ets_mode == NSM_DCB_MODE_ON)
    cli_out (cli, " enhanced-transmission-selection mode on\n");
  else if (dcbif->dcb_ets_mode == NSM_DCB_MODE_AUTO)
    cli_out (cli, " enhanced-transmission-selection mode auto\n");

  pal_sprintf (bw_str, "");
  
  /* Write traffic-class-group and populate bandwidth */
  for (tcgid = 0; tcgid < NSM_DCB_MAX_TCG_DEFAULT; tcgid++)
     {
        if (dcbif->tcg_priority_count[tcgid] != 0)
          {
             for (pri = 0; pri < NSM_DCB_NUM_USER_PRI; pri++)
                {
                  if (dcbif->tcg_priority_table[pri] == tcgid)
                    cli_out (cli, " traffic-class-group %d add priority %d\n",
                                   tcgid, pri);
                }
              
             if (dcbif->tcg_bandwidth_table [tcgid] != 0)
               {
                 pal_sprintf (tmp_bw_str, " %d %d", tcgid, 
                                            dcbif->tcg_bandwidth_table [tcgid]);
                 pal_strcat (bw_str, tmp_bw_str);
               }
          }
     }
 
  if (pal_strncmp (bw_str, "",1) != 0)
    cli_out (cli, " bandwidth-percentage%s\n", bw_str);

  /* Traverse application priority tree to write application priority 
   * configuration on interface.
   */
  avl_traverse (dcbif->dcb_appl_pri, 
                nsm_dcb_if_avl_traverse_appl_pri_config_write, 
                (void_t *) cli);

  if (! CHECK_FLAG (dcbif->dcb_if_flags, NSM_DCB_IF_PFC_ENABLE))
    {
      cli_out (cli, " no priority-flow-control enable\n");
      NSM_DCB_FN_EXIT (NSM_DCB_API_SET_SUCCESS);
    }

  if (dcbif->pfc_mode == NSM_DCB_PFC_MODE_ON)
    cli_out (cli, " priority-flow-control mode on\n");
  else if (dcbif->pfc_mode == NSM_DCB_PFC_MODE_AUTO)
    cli_out (cli, " priority-flow-control mode auto\n");

  for (pri = 0; pri < NSM_DCB_NUM_USER_PRI; pri++)
     if (dcbif->pfc_enable_vector[pri])
       cli_out (cli, " priority-flow-control enable priority %d\n", pri);

  if (dcbif->pfc_cap)
    cli_out (cli, " priority-flow-control cap %d\n", dcbif->pfc_cap);

  if (dcbif->pfc_link_delay_allowance)
    cli_out (cli, " priority-flow-control link-delay-allowance %d\n",
              dcbif->pfc_link_delay_allowance);

  NSM_DCB_FN_EXIT (NSM_DCB_API_SET_SUCCESS);
}

/**@brief This CLI enables the data center bridging on the bridge
   @return CLI_SUCCESS
*/
CLI (dcb_bridge_enable,
     dcb_bridge_enable_cmd,
     "data-center-bridging enable bridge <1-32>",
     NSM_DCB_STR,
     "Enable on bridge",
     BRIDGE_STR,
     BRIDGE_NAME_STR)
{
  s_int32_t ret = NSM_DCB_API_SET_SUCCESS;
  char br_name [NSM_BRIDGE_NAMSIZ+1];
 
  if (argc > 0)
    {
      pal_strncpy (br_name, argv[0], NSM_BRIDGE_NAMSIZ+1);
      ret = nsm_dcb_bridge_enable (cli->vr->id, br_name);
    }
  else 
#ifdef HAVE_DEFAULT_BRIDGE
    ret = nsm_dcb_bridge_enable (cli->vr->id, NULL);
#endif /* HAVE_DEFAULT_BRIDGE */
  
  return nsm_dcb_cli_return (cli, ret);
}

#ifdef HAVE_DEFAULT_BRIDGE
ALI (dcb_bridge_enable,
     dcb_bridge_enable_default_cmd,
     "data-center-bridging enable",
     NSM_DCB_STR,
     "Enable on default bridge");
#endif /* HAVE_DEFAULT_BRIDGE */



/**@brief This CLI disables the data center bridging on the bridge
   @return CLI_SUCCESS
*/
CLI (dcb_bridge_disable,
     dcb_bridge_disable_cmd,
     "no data-center-bridging enable bridge <1-32>",
     CLI_NO_STR,
     NSM_DCB_STR,
     "disable on bridge",
     BRIDGE_STR,
     BRIDGE_NAME_STR)
{
  s_int32_t ret = NSM_DCB_API_SET_SUCCESS;
  char br_name [NSM_BRIDGE_NAMSIZ+1];

  if (argc > 0)
    {
      pal_strncpy (br_name, argv[0], NSM_BRIDGE_NAMSIZ+1);
      ret = nsm_dcb_bridge_disable (cli->vr->id, br_name);
    }
  else
#ifdef HAVE_DEFAULT_BRIDGE
    ret = nsm_dcb_bridge_disable (cli->vr->id, NULL);
#endif /* HAVE_DEFAULT_BRIDGE */
  
  return nsm_dcb_cli_return (cli, ret);
}

#ifdef HAVE_DEFAULT_BRIDGE
ALI (dcb_bridge_disable,
     dcb_bridge_disable_default_cmd,
     "no data-center-bridging enable",
     CLI_NO_STR,
     NSM_DCB_STR,
     "disable on default bridge");
#endif /* HAVE_DEFAULT_BRIDGE */



/**@brief This CLI enables enhanced transmission selection on the bridge
   @return CLI_SUCCESS
*/
CLI (dcb_ets_bridge_enable,
     dcb_ets_bridge_enable_cmd,
     "enhanced-transmission-selection enable bridge <1-32>",
     NSM_ETS_STR,
     "Enable on bridge",
     BRIDGE_STR,
     BRIDGE_NAME_STR)
{
  s_int32_t ret = NSM_DCB_API_SET_SUCCESS;
  char br_name [NSM_BRIDGE_NAMSIZ+1];

  if (argc > 0)
    {
      pal_strncpy (br_name, argv[0], NSM_BRIDGE_NAMSIZ+1);
      ret = nsm_dcb_ets_bridge_enable (cli->vr->id, br_name);
    }
  else 
#ifdef HAVE_DEFAULT_BRIDGE
    ret = nsm_dcb_ets_bridge_enable (cli->vr->id, NULL);
#endif /* HAVE_DEFAULT_BRIDGE */
  
  return nsm_dcb_cli_return (cli, ret);
}

#ifdef HAVE_DEFAULT_BRIDGE
ALI (dcb_ets_bridge_enable,
     dcb_ets_bridge_enable_default_cmd,
     "enhanced-transmission-selection enable",
     NSM_ETS_STR,
     "Enable on default bridge");
#endif /* HAVE_DEFAULT_BRIDGE */



/**@brief This CLI disables enhanced transmission selection on the bridge
   @return CLI_SUCCESS
*/
CLI (dcb_ets_bridge_disable,
     dcb_ets_bridge_disable_cmd,
     "no enhanced-transmission-selection enable bridge <1-32>",
     CLI_NO_STR,
     NSM_ETS_STR,
     "Disable",
     BRIDGE_STR,
     BRIDGE_NAME_STR)
{
  s_int32_t ret = NSM_DCB_API_SET_SUCCESS;
  char br_name [NSM_BRIDGE_NAMSIZ+1];

  if (argc > 0)
    {
      pal_strncpy (br_name, argv[0], NSM_BRIDGE_NAMSIZ+1);
      ret = nsm_dcb_ets_bridge_disable (cli->vr->id, br_name);
    }
  else
#ifdef HAVE_DEFAULT_BRIDGE
    ret = nsm_dcb_ets_bridge_disable (cli->vr->id, NULL);
#endif /* HAVE_DEFAULT_BRIDGE */
  
  return nsm_dcb_cli_return (cli, ret);
}

#ifdef HAVE_DEFAULT_BRIDGE
ALI (dcb_ets_bridge_disable,
     dcb_ets_bridge_disable_default_cmd,
     "no enhanced-transmission-selection enable",
     CLI_NO_STR,
     NSM_ETS_STR,
     "Disable on default bridge");
#endif /* HAVE_DEFAULT_BRIDGE */



/**@brief This CLI enables priority flow control on the bridge
   @return CLI_SUCCESS
*/
CLI (dcb_pfc_bridge_enable,
     dcb_pfc_bridge_enable_cmd,
     "priority-flow-control enable bridge <1-32>",
     NSM_PFC_STR,
     "enable on the bridge",
     BRIDGE_STR,
     BRIDGE_NAME_STR)
{
  s_int32_t ret = NSM_DCB_API_SET_SUCCESS;
  char br_name [NSM_BRIDGE_NAMSIZ+1];

  if (argc > 0)
    {
      pal_strncpy (br_name, argv[0], NSM_BRIDGE_NAMSIZ+1);
      ret = nsm_dcb_pfc_bridge_enable (cli->vr->id, br_name);
    }
  else
#ifdef HAVE_DEFAULT_BRIDGE
    ret = nsm_dcb_pfc_bridge_enable (cli->vr->id, NULL);
#endif /* HAVE_DEFAULT_BRIDGE */

  if (ret == NSM_DCB_API_SET_ERR_PFC_EXISTS)
    {
      cli_out (cli, "%% DCB-PFC is already enabled on the bridge\n");
      ret = NSM_DCB_API_SET_SUCCESS;
    }

  return nsm_dcb_cli_return (cli, ret);
}

#ifdef HAVE_DEFAULT_BRIDGE
ALI (dcb_pfc_bridge_enable,
     dcb_pfc_bridge_enable_default_cmd,
     "priority-flow-control enable",
     NSM_PFC_STR,
     "enable on the default bridge");
#endif /* HAVE_DEFAULT_BRIDGE */



/**@brief This CLI disables priority flow control on the bridge
   @return CLI_SUCCESS
*/
CLI (dcb_pfc_bridge_disable,
     dcb_pfc_bridge_disable_cmd,
     "no priority-flow-control enable bridge <1-32>",
     CLI_NO_STR,
     NSM_PFC_STR,
     "Disable on the bridge",
     BRIDGE_STR,
     BRIDGE_NAME_STR)
{
  s_int32_t ret = NSM_DCB_API_SET_SUCCESS;
  char br_name [NSM_BRIDGE_NAMSIZ+1];

  if (argc > 0)
    {
      pal_strncpy (br_name, argv[0], NSM_BRIDGE_NAMSIZ+1);
      ret = nsm_dcb_pfc_bridge_disable (cli->vr->id, br_name);
    }
  else
#ifdef HAVE_DEFAULT_BRIDGE
    ret = nsm_dcb_pfc_bridge_disable (cli->vr->id, NULL);
#endif /* HAVE_DEFAULT_BRIDGE */
  return nsm_dcb_cli_return (cli, ret);
}

#ifdef HAVE_DEFAULT_BRIDGE
ALI (dcb_pfc_bridge_disable,
     dcb_pfc_bridge_disable_default_cmd,
     "no priority-flow-control enable",
     CLI_NO_STR,
     NSM_PFC_STR,
     "Disable on the default bridge");
#endif /* HAVE_DEFAULT_BRIDGE */


/**@brief This CLI enables data center bridging on the interface.
   @return CLI_SUCCESS
*/
CLI (dcb_interface_enable,
     dcb_interface_enable_cmd,
     "data-center-bridging enable",
     NSM_DCB_STR,
     "Enable on interface")
{
  s_int32_t ret = NSM_DCB_API_SET_SUCCESS;
  struct interface *ifp = NULL;
   
  ifp = cli->index;

  /* Check the Interface */
  if (! ifp)
    return nsm_dcb_cli_return (cli, NSM_DCB_API_SET_ERR_INTERFACE);

  if (! NSM_INTF_TYPE_L2 (ifp))
    return nsm_dcb_cli_return (cli, NSM_DCB_API_SET_ERR_L2_INTERFACE);

  ret = nsm_dcb_enable_interface (cli->vr->id, ifp->name);
  
  return nsm_dcb_cli_return (cli, ret);
}

/**@brief This CLI disables data center bridging on the interface.
   @return CLI_SUCCESS
*/
CLI (dcb_interface_disable,
     dcb_interface_disable_cmd,
     "no data-center-bridging enable",
     CLI_NO_STR,
     NSM_DCB_STR,
     "Disable on interface")
{
  s_int32_t ret = NSM_DCB_API_SET_SUCCESS;
  struct interface *ifp = NULL;

  ifp = cli->index;

  /* Check the Interface */
  if (! ifp)
    return nsm_dcb_cli_return (cli, NSM_DCB_API_SET_ERR_INTERFACE);

  if (! NSM_INTF_TYPE_L2 (ifp))
    return nsm_dcb_cli_return (cli, NSM_DCB_API_SET_ERR_L2_INTERFACE);

  ret = nsm_dcb_disable_interface (cli->vr->id, ifp->name);

  return nsm_dcb_cli_return (cli, ret);
}

/**@brief This CLI enables enhanced transmission selection on the interface.
   @return CLI_SUCCESS
*/
CLI (dcb_ets_interface_enable,
     dcb_ets_interface_enable_cmd,
     "enhanced-transmission-selection enable",
     NSM_ETS_STR,
     "Enable on interface")
{
  s_int32_t ret = NSM_DCB_API_SET_SUCCESS;
  struct interface *ifp = NULL;

  ifp = cli->index;

  /* Check the Interface */
  if (! ifp)
    return nsm_dcb_cli_return (cli, NSM_DCB_API_SET_ERR_INTERFACE);

  if (! NSM_INTF_TYPE_L2 (ifp))
    return nsm_dcb_cli_return (cli, NSM_DCB_API_SET_ERR_L2_INTERFACE);

  ret = nsm_dcb_ets_enable_interface (cli->vr->id, ifp->name);
  
  return nsm_dcb_cli_return (cli, ret);
}

/**@brief This CLI disables enhanced trasnmission selection on the interface.
   @return CLI_SUCCESS
*/
CLI (dcb_ets_interface_disable,
     dcb_ets_interface_disable_cmd,
     "no enhanced-transmission-selection enable",
     CLI_NO_STR,
     NSM_ETS_STR,
     "Disable on interface")
{
  s_int32_t ret = NSM_DCB_API_SET_SUCCESS;
  struct interface *ifp = NULL;

  ifp = cli->index;

  /* Check the Interface */
  if (! ifp)
    return nsm_dcb_cli_return (cli, NSM_DCB_API_SET_ERR_INTERFACE);

  if (! NSM_INTF_TYPE_L2 (ifp))
    return nsm_dcb_cli_return (cli, NSM_DCB_API_SET_ERR_L2_INTERFACE);

  ret = nsm_dcb_ets_disable_interface (cli->vr->id, ifp->name);

  return nsm_dcb_cli_return (cli, ret);
}

/**@brief This CLI sets ETS mode on the interface.
   @return CLI_SUCCESS
*/
CLI (dcb_ets_set_mode,
     dcb_ets_set_mode_cmd,
     "enhanced-transmission-selection mode (on | auto)",
     NSM_ETS_STR, 
     "Mode of the Enhanced tranmission Selection",
     "ON to force enable Enhanced-trasnsmission-selection",
     "AUTO to negotiate Enhanced-transmission-selection capabilities")
{
  s_int32_t ret = NSM_DCB_API_SET_SUCCESS;
  struct interface *ifp = NULL;
  u_int8_t mode = NSM_DCB_MODE_ON;

  ifp = cli->index;

  /* Check the Interface */
  if (! ifp)
    return nsm_dcb_cli_return (cli, NSM_DCB_API_SET_ERR_INTERFACE);

  if (! NSM_INTF_TYPE_L2 (ifp))
    return nsm_dcb_cli_return (cli, NSM_DCB_API_SET_ERR_L2_INTERFACE);

  /* Check the mode */
  CLI_GET_DCB_MODE (mode, argv[0]);

  ret = nsm_dcb_set_ets_mode(cli->vr->id, ifp->name, mode);

  return nsm_dcb_cli_return (cli, ret);

}

/**@brief This CLI adds priorities in traffic-class-group on the interface.
   @return CLI_SUCCESS
*/
CLI (dcb_ets_add_priority,
     dcb_ets_add_priority_cmd,
     "traffic-class-group <0-7> add priority (<0-7> | <0-7> <0-7> |"
     "<0-7> <0-7> <0-7> | <0-7> <0-7> <0-7> <0-7> |"
     "<0-7> <0-7> <0-7> <0-7> <0-7> |"
     "<0-7> <0-7> <0-7> <0-7> <0-7> <0-7> |"
     "<0-7> <0-7> <0-7> <0-7> <0-7> <0-7> <0-7> |"
     "<0-7> <0-7> <0-7> <0-7> <0-7> <0-7> <0-7> <0-7>)",
     NSM_TCG_STR,
     NSM_TCGID_STR,
     "add",
     "traffic-priority",
     "traffic-priority value",
     "traffic-priority value",
     "traffic-priority value",
     "traffic-priority value",
     "traffic-priority value",
     "traffic-priority value",
     "traffic-priority value",
     "traffic-priority value",
     "traffic-priority value",
     "traffic-priority value",
     "traffic-priority value",
     "traffic-priority value",
     "traffic-priority value",
     "traffic-priority value",
     "traffic-priority value",
     "traffic-priority value",
     "traffic-priority value",
     "traffic-priority value",
     "traffic-priority value",
     "traffic-priority value",
     "traffic-priority value",
     "traffic-priority value",
     "traffic-priority value",
     "traffic-priority value",
     "traffic-priority value",
     "traffic-priority value",
     "traffic-priority value",
     "traffic-priority value", 
     "traffic-priority value",
     "traffic-priority value",
     "traffic-priority value",
     "traffic-priority value",
     "traffic-priority value",
     "traffic-priority value",
     "traffic-priority value",
     "traffic-priority value")
{
  s_int32_t ret = NSM_DCB_API_SET_SUCCESS;
  struct interface *ifp = NULL;
  u_int8_t tcgid, i, j;
  u_int8_t priority = 0, pri;
  
  ifp = cli->index;

  /* Check the Interface */
  if (! ifp)
    return nsm_dcb_cli_return (cli, NSM_DCB_API_SET_ERR_INTERFACE);

  if (! NSM_INTF_TYPE_L2 (ifp))
    return nsm_dcb_cli_return (cli, NSM_DCB_API_SET_ERR_L2_INTERFACE);

  /* Check the range for TCGID */
  CLI_GET_UINT32 ("Traffic-class-group ID", tcgid, argv[0]);

  /* Check for duplicate arguments */
  /* e.g. traffic-class-group 1 add priority 3 3 3 is invalid */
  for (i = 1; i < argc; i++)
     {
      for (j = i+1; j < argc; j++)
         {
           if (pal_strncmp (argv[i], argv[j], 1) == 0)
             return nsm_dcb_cli_return (cli, 
                                        NSM_DCB_API_SET_ERR_DUPLICATE_ARGS);
         }
     }  

  /* Check the range for priority*/
  switch (argc)
    {
      case 9:       
        CLI_GET_UINT32 ("Priority", pri, argv[8]);
        priority = priority | (1 << pri);
      case 8:
        CLI_GET_UINT32 ("Priority", pri, argv[7]);
        priority = priority | (1 << pri);
      case 7:
        CLI_GET_UINT32 ("Priority", pri, argv[6]);
        priority = priority | (1 << pri);
      case 6:
        CLI_GET_UINT32 ("Priority", pri, argv[5]);
        priority = priority | (1 << pri);
      case 5:
        CLI_GET_UINT32 ("Priority", pri, argv[4]);
        priority = priority | (1 << pri);
      case 4:
        CLI_GET_UINT32 ("Priority", pri, argv[3]);
        priority = priority | (1 << pri);
      case 3:
        CLI_GET_UINT32 ("Priority", pri, argv[2]);
        priority = priority | (1 << pri);
      case 2:
        CLI_GET_UINT32 ("Priority", pri, argv[1]);
        priority = priority | (1 << pri);
        break;
      default:
         nsm_dcb_cli_return (cli, NSM_DCB_API_SET_ERR_INVALID_VALUE);
    }
  
  ret = nsm_dcb_add_pri_to_tcg (cli->vr->id, ifp->name, tcgid, priority);

  return nsm_dcb_cli_return (cli, ret);
}

/**@brief This CLI removes priorities from traffic-class-group on the interface.
   @return CLI_SUCCESS
*/
CLI (dcb_ets_remove_priority,
     dcb_ets_remove_priority_cmd,
     "traffic-class-group <0-7> remove priority (<0-7> | <0-7> <0-7> |"
     "<0-7> <0-7> <0-7> | <0-7> <0-7> <0-7> <0-7> |"
     "<0-7> <0-7> <0-7> <0-7> <0-7> |"
     "<0-7> <0-7> <0-7> <0-7> <0-7> <0-7> |"
     "<0-7> <0-7> <0-7> <0-7> <0-7> <0-7> <0-7> |"
     "<0-7> <0-7> <0-7> <0-7> <0-7> <0-7> <0-7> <0-7>)",
     NSM_TCG_STR,
     NSM_TCGID_STR,
     "remove",
     "traffic-priority",
     "traffic-priority value",
     "traffic-priority value",
     "traffic-priority value",
     "traffic-priority value",
     "traffic-priority value",
     "traffic-priority value",
     "traffic-priority value",
     "traffic-priority value",
     "traffic-priority value",
     "traffic-priority value",
     "traffic-priority value",
     "traffic-priority value",
     "traffic-priority value",
     "traffic-priority value",
     "traffic-priority value",
     "traffic-priority value",
     "traffic-priority value",
     "traffic-priority value",
     "traffic-priority value",
     "traffic-priority value",
     "traffic-priority value",
     "traffic-priority value",
     "traffic-priority value",
     "traffic-priority value",
     "traffic-priority value",
     "traffic-priority value",
     "traffic-priority value",
     "traffic-priority value", 
     "traffic-priority value",
     "traffic-priority value",
     "traffic-priority value",
     "traffic-priority value",
     "traffic-priority value",
     "traffic-priority value",
     "traffic-priority value",
     "traffic-priority value")
{
  s_int32_t ret = NSM_DCB_API_SET_SUCCESS;
  struct interface *ifp = NULL;
  u_int8_t tcgid , i, j;
  u_int8_t priority = 0, pri;

  ifp = cli->index;

  /* Check the Interface */
  if (! ifp)
    return nsm_dcb_cli_return (cli, NSM_DCB_API_SET_ERR_INTERFACE);

  if (! NSM_INTF_TYPE_L2 (ifp))
    return nsm_dcb_cli_return (cli, NSM_DCB_API_SET_ERR_L2_INTERFACE);

  /* Check the range for TCGID */
  CLI_GET_UINT32 ("Traffic-class-group ID", tcgid, argv[0]);

  /* Check for duplicate arguments */
  /* e.g. traffic-class-group 1 remove priority 3 3 3 is invalid */
  for (i = 1; i < argc; i++)
     {
      for (j = i+1; j < argc; j++)
         {
           if (pal_strncmp (argv[i], argv[j], 1) == 0)
             return nsm_dcb_cli_return (cli, 
                                        NSM_DCB_API_SET_ERR_DUPLICATE_ARGS);
         }
     }

  /* Check the range for priority*/
  switch (argc)
    {
      case 9:       
        CLI_GET_UINT32 ("Priority", pri, argv[8]); 
        priority = priority | (1 << pri);
      case 8:
        CLI_GET_UINT32 ("Priority", pri, argv[7]);
        priority = priority | (1 << pri);
      case 7:
        CLI_GET_UINT32 ("Priority", pri, argv[6]);
        priority = priority | (1 << pri);
      case 6:
        CLI_GET_UINT32 ("Priority", pri, argv[5]);
        priority = priority | (1 << pri);
      case 5:
        CLI_GET_UINT32 ("Priority", pri, argv[4]);
        priority = priority | (1 << pri);
      case 4:
        CLI_GET_UINT32 ("Priority", pri, argv[3]);
        priority = priority | (1 << pri);
      case 3:
        CLI_GET_UINT32 ("Priority", pri, argv[2]);
        priority = priority | (1 << pri);
      case 2:
        CLI_GET_UINT32 ("Priority", pri, argv[1]);
        priority = priority | (1 << pri);
        break;
      default:
         nsm_dcb_cli_return (cli, NSM_DCB_API_SET_ERR_INVALID_VALUE);
    }

  ret = nsm_dcb_remove_pri_from_tcg (cli->vr->id, ifp->name, tcgid, priority);

  return nsm_dcb_cli_return (cli, ret);
}

/**@brief This CLI assigns the bandwidth percentage to traffic-class-groups
   @return CLI_SUCCESS
*/
CLI (dcb_ets_assign_bw,
     dcb_ets_assign_bw_cmd,
     "bandwidth-percentage (<0-7> <0-100> | <0-7> <0-100> <0-7> <0-100> |"
     "<0-7> <0-100> <0-7> <0-100> <0-7> <0-100> | <0-7> <0-100> <0-7> <0-100>"
     " <0-7> <0-100> <0-7> <0-100> | <0-7> <0-100> <0-7> <0-100> <0-7> <0-100>"
     " <0-7> <0-100> <0-7> <0-100> | <0-7> <0-100> <0-7> <0-100> <0-7> <0-100>"
     " <0-7> <0-100> <0-7> <0-100> | <0-7> <0-100> <0-7> <0-100> <0-7> <0-100>"
     " <0-7> <0-100> <0-7> <0-100> <0-7> <0-100> <0-7> <0-100> | <0-7> <0-100>"
     " <0-7> <0-100> <0-7> <0-100> <0-7> <0-100> <0-7> <0-100> <0-7> <0-100>"
     " <0-7> <0-100> <0-7> <0-100>)",
     "Bandwith-percentage",
     NSM_TCGID_STR, 
     NSM_BW_STR,
     NSM_TCGID_STR,
     NSM_BW_STR, 
     NSM_TCGID_STR,
     NSM_BW_STR,
     NSM_TCGID_STR,
     NSM_BW_STR,
     NSM_TCGID_STR,
     NSM_BW_STR,
     NSM_TCGID_STR,
     NSM_BW_STR,
     NSM_TCGID_STR,
     NSM_BW_STR,
     NSM_TCGID_STR,
     NSM_BW_STR,
     NSM_TCGID_STR,
     NSM_BW_STR,
     NSM_TCGID_STR,
     NSM_BW_STR,
     NSM_TCGID_STR,
     NSM_BW_STR,
     NSM_TCGID_STR,
     NSM_BW_STR,
     NSM_TCGID_STR,
     NSM_BW_STR,
     NSM_TCGID_STR,
     NSM_BW_STR,
     NSM_TCGID_STR,
     NSM_BW_STR,
     NSM_TCGID_STR,
     NSM_BW_STR,
     NSM_TCGID_STR,
     NSM_BW_STR,
     NSM_TCGID_STR,
     NSM_BW_STR,
     NSM_TCGID_STR,
     NSM_BW_STR,
     NSM_TCGID_STR,
     NSM_BW_STR,
     NSM_TCGID_STR,
     NSM_BW_STR,
     NSM_TCGID_STR,
     NSM_BW_STR,
     NSM_TCGID_STR,
     NSM_BW_STR,
     NSM_TCGID_STR,
     NSM_BW_STR,
     NSM_TCGID_STR,
     NSM_BW_STR,
     NSM_TCGID_STR,
     NSM_BW_STR,
     NSM_TCGID_STR,
     NSM_BW_STR,
     NSM_TCGID_STR,
     NSM_BW_STR,
     NSM_TCGID_STR,
     NSM_BW_STR,
     NSM_TCGID_STR,
     NSM_BW_STR,
     NSM_TCGID_STR,
     NSM_BW_STR,
     NSM_TCGID_STR,
     NSM_BW_STR,
     NSM_TCGID_STR,
     NSM_BW_STR,
     NSM_TCGID_STR,
     NSM_BW_STR,
     NSM_TCGID_STR,
     NSM_BW_STR)
{
  s_int32_t ret = NSM_DCB_API_SET_SUCCESS;
  struct interface *ifp = NULL;
  u_int16_t bw[NSM_DCB_MAX_TCG_DEFAULT];
  u_int8_t tcgid;
  u_int16_t bandwidth;
  u_int8_t i;

  ifp = cli->index;

  /* Check the Interface */
  if (! ifp)
    return nsm_dcb_cli_return (cli, NSM_DCB_API_SET_ERR_INTERFACE);

  if (! NSM_INTF_TYPE_L2 (ifp))
    return nsm_dcb_cli_return (cli, NSM_DCB_API_SET_ERR_L2_INTERFACE);

  switch (argc)
    {
      case 16:
      case 14:
      case 12:
      case 10:
      case 8:
      case 6:
      case 4:
      case 2:
        break;
      default:
        return nsm_dcb_cli_return (cli, NSM_DCB_API_SET_ERR_INVALID_VALUE);
    }

  /* Initialize the bandwidth array to zero */
  pal_mem_set (bw, 0, (sizeof (u_int16_t)) * NSM_DCB_MAX_TCG_DEFAULT);

  /* Populated the bandwidth array as per the user input */
  for (i = 0; i < argc; i = i+2)
    {
      CLI_GET_UINT32 ("tcgid", tcgid, argv[i]);
      CLI_GET_UINT32 ("Bandwidth", bandwidth, argv[i+1]);
      bw[tcgid] = bandwidth;
    }

  ret = nsm_dcb_assign_bw_percentage_to_tcg (cli->vr->id, ifp->name, bw);

  return nsm_dcb_cli_return (cli, ret);
}

/**@brief This CLI removes all TCG related configuration from interface
   @return CLI_SUCCESS
*/
CLI (dcb_ets_delete_tcgs,
     dcb_ets_delete_tcgs_cmd,
     "no traffic-class-groups",
     CLI_NO_STR,
     NSM_TCG_STR)
{
  s_int32_t ret = NSM_DCB_API_SET_SUCCESS;
  struct interface *ifp = NULL;

  ifp = cli->index;

  /* Check the Interface */
  if (! ifp)
    return nsm_dcb_cli_return (cli, NSM_DCB_API_SET_ERR_INTERFACE);

  if (! NSM_INTF_TYPE_L2 (ifp))
    return nsm_dcb_cli_return (cli, NSM_DCB_API_SET_ERR_L2_INTERFACE);
  
  ret = nsm_dcb_delete_tcgs (cli->vr->id, ifp->name);

  return nsm_dcb_cli_return (cli, ret);
}

/**@brief This CLI sets the application priority for well known tcp/udp 
          ports based on port number or service name 
   @return CLI_SUCCESS
*/
CLI (dcb_set_appl_priority_ports,
     dcb_set_appl_priority_ports_cmd,
     "(tcp | udp | both-tcp-udp | neither-tcp-udp) (port-no <1-1023> |" 
     "service-name PROTOSERV) priority <0-7>",
     "Protocol Type tcp",
     "Protocol Type udp",
     "Protocol Type both-tcp-udp",
     "Protocol Type neither-tcp-udp",
     "Well known port number",
     "Well known port number value",
     "Well known service",
     "well known service name e.g telnet",
     "User Priority",
     "User Priority Value")
{
  s_int32_t ret = NSM_DCB_API_SET_SUCCESS;
  struct interface *ifp = NULL;
  struct pal_servent *sp;
  u_int8_t proto_type, pri;
  u_int16_t port_no;
  enum nsm_dcb_appl_pri_mapping_type 
  mapping_type = NSM_DCB_MAP_PORT_NO; 
  
  ifp = cli->index;

  /* Check the Interface */
  if (! ifp)
    return nsm_dcb_cli_return (cli, NSM_DCB_API_SET_ERR_INTERFACE);

  if (! NSM_INTF_TYPE_L2 (ifp))
    return nsm_dcb_cli_return (cli, NSM_DCB_API_SET_ERR_L2_INTERFACE);

  /* Get the protocol type tcp/udp/both-tcp-udp/neither-tcp-udp */
  CLI_GET_DCB_PROTO_TYPE (proto_type, argv[0]);
  
  /* Get the mapping type port-no or service name */
  if (pal_strncmp (argv[1], "s", 1) == 0)
    mapping_type = NSM_DCB_MAP_SERV_NAME;
  else if (pal_strncmp (argv[1], "p", 1) == 0)
    mapping_type = NSM_DCB_MAP_PORT_NO;

  /* Find out the port number for given service name */
  if (mapping_type == NSM_DCB_MAP_SERV_NAME)
    {
      switch (proto_type)
        {
          case 1:
            sp = (struct pal_servent *) pal_getservbyname (argv[2], "tcp");
            if (sp)
              port_no = pal_ntoh16 (sp->s_port);
            else
              return nsm_dcb_cli_return (cli, NSM_DCB_API_SET_ERR_NO_SERV);
            break;
          case 2:
            sp = (struct pal_servent *) pal_getservbyname (argv[2], "udp");
            if (sp)
              port_no = pal_ntoh16 (sp->s_port);
            else
              return nsm_dcb_cli_return (cli, NSM_DCB_API_SET_ERR_NO_SERV);
            break;
          case 3:
            sp = (struct pal_servent *) pal_getservbyname (argv[2], "tcp");
            if (! sp)
              sp = (struct pal_servent *) pal_getservbyname (argv[2], "udp");
            if (sp)
              port_no = pal_ntoh16 (sp->s_port);
            else
              return nsm_dcb_cli_return (cli, NSM_DCB_API_SET_ERR_NO_SERV);
            break;
          default:
             return nsm_dcb_cli_return (cli, NSM_DCB_API_SET_ERR_NO_SUPPORT);
             break;
        }
    }
  else
    CLI_GET_UINT32_RANGE ("Port Number", port_no, argv[2], 1, 1023);
  
  CLI_GET_UINT32 ("Priority", pri, argv[3]);

  ret = nsm_dcb_application_priority_set (cli->vr->id, ifp->name, proto_type, 
                                          port_no, pri, mapping_type);

  return nsm_dcb_cli_return (cli, ret);
}

/**@brief This CLI unsets the application priority for the protocol based on
          port number or service name on the interface.
   @return CLI_SUCCESS;
*/
CLI (dcb_unset_appl_priority_ports,
     dcb_unset_appl_priority_ports_cmd,
     "no (tcp | udp | both-tcp-udp | neither-tcp-udp) (port-no <1-1023> |"
     " service-name PROTOSERV) priority <0-7>",
     CLI_NO_STR,
     "Protocol Type tcp",
     "Protocol Type udp",
     "Protocol Type both-tcp-udp",
     "Protocol Type neither-tcp-udp",
     "Well known port number",
     "Well known port number value",
     "Well known service",
     "Well known service name e.g. telnet",
     "User Priority",
     "User Priority Value")
{
  s_int32_t ret = NSM_DCB_API_SET_SUCCESS;
  struct interface *ifp = NULL;
  struct pal_servent *sp;
  u_int8_t proto_type, pri;
  u_int16_t port_no;
  enum nsm_dcb_appl_pri_mapping_type 
  mapping_type = NSM_DCB_MAP_PORT_NO;
  
  ifp = cli->index;

  /* Check the Interface */
  if (! ifp)
    return nsm_dcb_cli_return (cli, NSM_DCB_API_SET_ERR_INTERFACE);

  if (! NSM_INTF_TYPE_L2 (ifp))
    return nsm_dcb_cli_return (cli, NSM_DCB_API_SET_ERR_L2_INTERFACE);

  /* Get the protocol type tcp/udp/both-tcp-udp/neither-tcp/udp */
  CLI_GET_DCB_PROTO_TYPE (proto_type, argv[0]);

  /* Get the mapping type port-no or service name */
  if (pal_strncmp (argv[1], "p", 1) == 0)
    mapping_type = NSM_DCB_MAP_PORT_NO;
  else if (pal_strncmp (argv[1], "s", 1) == 0)
    mapping_type = NSM_DCB_MAP_SERV_NAME;

  /* Find out port number based on service name */
  if (mapping_type == NSM_DCB_MAP_SERV_NAME)
    {
      switch (proto_type)
        {
          case 1:
            sp = (struct pal_servent *) pal_getservbyname (argv[2], "tcp");
            if (sp)
              port_no = pal_ntoh16 (sp->s_port);
            else 
              return nsm_dcb_cli_return (cli, NSM_DCB_API_SET_ERR_NO_SERV);
            break;
          case 2:
            sp = (struct pal_servent *) pal_getservbyname (argv[2], "udp");
            if (sp)
              port_no = pal_ntoh16 (sp->s_port);
            else
              return nsm_dcb_cli_return (cli, NSM_DCB_API_SET_ERR_NO_SERV);
            break;
          case 3:
            sp = (struct pal_servent *) pal_getservbyname (argv[2], "tcp");
            if (! sp)
              sp = (struct pal_servent *) pal_getservbyname (argv[2], "udp"); 
            if (sp)
              port_no = pal_ntoh16 (sp->s_port);
            else
              return nsm_dcb_cli_return (cli, NSM_DCB_API_SET_ERR_NO_SERV);
            break;
          default:
             return nsm_dcb_cli_return (cli, NSM_DCB_API_SET_ERR_NO_SUPPORT);
             break;
        }        
    }
  else 
    CLI_GET_UINT32_RANGE ("Port Number", port_no, argv[2], 1, 1023);

  CLI_GET_UINT32 ("Priority", pri, argv[3]);

  ret = nsm_dcb_application_priority_unset (cli->vr->id, ifp->name, proto_type,
                                            port_no, pri);

  return nsm_dcb_cli_return (cli, ret);
}

/**@brief This CLI sets the application priority for ethertype based on hex 
          value or protocol name on the interface 
   @return CLI_SUCCESS
*/
CLI (dcb_set_appl_priority_ethertype,
     dcb_set_appl_priority_ethertype_cmd,
     "ethertype (value ETHERTYPE | name ETHERNAME) priority <0-7>",
     "Protocol Type",
     "Ethertype Value (in Hex notation)",
     "Ethertype value (in 0xhhhh hexadecimal notation)",
     "Ethertype Name",
     "Ethertype name Supported proto names : "
     "ip (IPv4), ipv6(IPv6), ipx(IPX), x25 (CCITT X.25), "
     "arp(Address Resolution), rarp (Reverse Address Resolution), "
     "atalkddp (Appletalk DDP), atalkaarp (Appletalk AARP), "
     "atmmulti (MultiProtocol over ATM), "
     "atmtransport (Frame-based ATM Transport), "
     "pppdiscovery (PPPoE discovery), pppsession (PPPoE Session), "
     "xeroxpup (Xerox PUP), "
     "xeroxaddrtrans (Xerox PUP Address Translation), "
     "g8bpqx25 (G8BPQ AX.25), ieeepup (Xerox IEEE802.3 PUP), "
     "ieeeaddrtrans (Xerox IEEE802.3 PUP Address Translation), "
     "dec (DEC Assigned), decdnadumpload (DEC DNA Dump/Load), "
     "decdnaremoteconsole (DEC DNA Remote Console), "
     "decdnarouting (DEC DNA Routing), "
     "declat (DEC LAT), decdiagnostics (DEC Diagnostics), "
     "deccustom (DEC Customer Use), "
     "decsyscomm (DEC Systems Comms Arch))",
     "User Priority",
     "User Priority Value")
{
  s_int32_t ret = NSM_DCB_API_SET_SUCCESS;
  struct interface *ifp = NULL;
  u_int8_t proto_type, pri;
  u_int16_t ether_type = 0;
  enum nsm_dcb_appl_pri_mapping_type 
  mapping_type = NSM_DCB_MAP_ETHERTYPE_VALUE;

  ifp = cli->index;

  /* Check the Interface */
  if (! ifp)
    return nsm_dcb_cli_return (cli, NSM_DCB_API_SET_ERR_INTERFACE);

  if (! NSM_INTF_TYPE_L2 (ifp))
    return nsm_dcb_cli_return (cli, NSM_DCB_API_SET_ERR_L2_INTERFACE);

  if (pal_strncmp (argv[0],"v",1) == 0)
    {
      if (pal_strlen (argv[1]) != 6)
        return nsm_dcb_cli_return (cli, NSM_DCB_API_SET_ERR_INVALID_VALUE);
 
      pal_sscanf (argv[1], "%hx", (u_int16_t *)&ether_type);
      mapping_type = NSM_DCB_MAP_ETHERTYPE_VALUE;
    }
  else if (pal_strncmp (argv[0], "n", 1) == 0)
    {
       mapping_type = NSM_DCB_MAP_ETHERTYPE_STR;
       /* Look thru the predefined strings */
       switch (argv[1][0])
         {
            /* arp | appletalk_ddp | appletalk_aarp |
               atm_multiprotocol| atm_transport */
            case 'a':  
            case 'A':
                if (0 == pal_strcmp (argv[1],APN_CLASS_NAME_ARP))
                  ether_type = (u_int16_t) APN_CLASS_P_ARP;
                else if (0 == pal_strcmp(argv[1],APN_CLASS_NAME_ATALK))
                  ether_type = (u_int16_t) APN_CLASS_P_ATALK;
                else if (0 == pal_strcmp(argv[1],APN_CLASS_NAME_AARP))
                  ether_type = (u_int16_t) APN_CLASS_P_AARP;
                else if (0 == pal_strcmp(argv[1],APN_CLASS_NAME_ATMMPOA))
                  ether_type = (u_int16_t) APN_CLASS_P_ATMMPOA;
                else if (0 == pal_strcmp(argv[1],APN_CLASS_NAME_ATMFATE))
                  ether_type = (u_int16_t) APN_CLASS_P_ATMFATE;
                break;
            /* dec | dec_dna_dump_load | dec_dna_remote_console | 
               dec_dna_routing | dec_lat | dec_diagnostics | dec_custom | 
               dec_sys_comm */
            case 'd':
            case 'D':
                if (0 == pal_strcmp(argv[1],APN_CLASS_NAME_DEC))
                  ether_type = (u_int16_t) APN_CLASS_P_DEC;
                else if (0 == pal_strcmp(argv[1],APN_CLASS_NAME_DNA_DL))
                  ether_type = (u_int16_t) APN_CLASS_P_DNA_DL;
                else if(0 == pal_strcmp(argv[1],APN_CLASS_NAME_DNA_RC))
                  ether_type = (u_int16_t) APN_CLASS_P_DNA_RC;
                else if (0 == pal_strcmp(argv[1],APN_CLASS_NAME_DNA_RT))
                  ether_type = (u_int16_t) APN_CLASS_P_DNA_RT;
                else if (0 == pal_strcmp(argv[1],APN_CLASS_NAME_LAT))
                  ether_type = (u_int16_t) APN_CLASS_P_LAT;
                else if (0 == pal_strcmp(argv[1],APN_CLASS_NAME_DIAG))
                  ether_type = (u_int16_t) APN_CLASS_P_DIAG;
                else if (0 == pal_strcmp(argv[1],APN_CLASS_NAME_CUST))
                  ether_type = (u_int16_t) APN_CLASS_P_CUST;
                else if (0 == pal_strcmp(argv[1],APN_CLASS_NAME_SCA))
                  ether_type = (u_int16_t) APN_CLASS_P_SCA;
                break;
            /* g8bpq_x25 */
            case 'g':  
            case 'G':
                if (0 == pal_strcmp(argv[1],APN_CLASS_NAME_BPQ))
                  ether_type = (u_int16_t) APN_CLASS_P_BPQ;
                break;
            /* ip, ipv6, ipx,*/
            case 'i':  
            case 'I':
                if (0 == pal_strcmp(argv[1],APN_CLASS_NAME_IP))
                  ether_type = (u_int16_t) APN_CLASS_P_IP;
                else if (0 == pal_strcmp(argv[1],APN_CLASS_NAME_IPV6))
                  ether_type = (u_int16_t) APN_CLASS_P_IPV6;
                else if (0 == pal_strcmp(argv[1],APN_CLASS_NAME_IPX))
                  ether_type = (u_int16_t) APN_CLASS_P_IPX;
                else if (0 == pal_strcmp(argv[1],APN_CLASS_NAME_IEEEPUP))
                  ether_type = (u_int16_t) APN_CLASS_P_IEEEPUP;
                else if (0 == pal_strcmp(argv[1],APN_CLASS_NAME_IEEEPUPAT))
                  ether_type = (u_int16_t) APN_CLASS_P_IEEEPUPAT;
                break;
            /* ppp_discovery | ppp_session */
            case 'p':  
            case 'P':
                if (0 == pal_strcmp(argv[1],APN_CLASS_NAME_PPP_DISC))
                  ether_type = (u_int16_t) APN_CLASS_P_PPP_DISC;
                else if (0 == pal_strcmp(argv[1],APN_CLASS_NAME_PPP_SES))
                  ether_type = (u_int16_t) APN_CLASS_P_PPP_SES;
                break;
            /* rarp */
            case 'r':  
            case 'R':
      		if (0 == pal_strcmp(argv[1],APN_CLASS_NAME_RARP))
        	  ether_type = (u_int16_t) APN_CLASS_P_RARP;
                break;
            /* x25 | xerox_pup | xerox_pup_addr_trans */
            case 'x':  
            case 'X':
                if (0 == pal_strcmp(argv[1],APN_CLASS_NAME_X25))
                  ether_type = (u_int16_t) APN_CLASS_P_X25;
                else if (0 == pal_strcmp(argv[1],APN_CLASS_NAME_PUPAT))
                  ether_type = (u_int16_t) APN_CLASS_P_PUPAT;
                else if (0 == pal_strcmp(argv[1],APN_CLASS_NAME_PUP))
                  ether_type = (u_int16_t) APN_CLASS_P_PUP;
                break;
            default:
              break;
          }
    }

  CLI_GET_UINT32 ("Priority", pri, argv[2]);
  
  proto_type = NSM_DCB_PROTO_ETHERTYPE;

  ret = nsm_dcb_application_priority_set (cli->vr->id, ifp->name, proto_type,
                                          ether_type, pri, mapping_type);

  return nsm_dcb_cli_return (cli, ret);
}

/**@brief This CLI unsets the application priority of ethertype based on
          hex value or protocol name on interface 
   @return CLI_SUCCESS
*/
CLI (dcb_unset_appl_priority_ethertype,
     dcb_unset_appl_priority_ethertype_cmd,
     "no ethertype (value ETHERTYPE | name ETHERNAME) priority <0-7>",
     CLI_NO_STR,
     "Protocol Type",
     "Ethertype value",
     "Ethertype value (in 0xhhhh hexadecimal notation)",
     "Ethertype Name",
     "Ethertype name (Supported proto names : "
     "ip (IPv4), ipv6(IPv6), ipx(IPX), x25 (CCITT X.25), "
     "arp(Address Resolution), rarp (Reverse Address Resolution), " 
     "atalkddp (Appletalk DDP), atalkaarp (Appletalk AARP), "
     "atmmulti (MultiProtocol over ATM), "
     "atmtransport (Frame-based ATM Transport), "
     "pppdiscovery (PPPoE discovery), pppsession (PPPoE Session), "
     "xeroxpup (Xerox PUP), "
     "xeroxaddrtrans (Xerox PUP Address Translation), "
     "g8bpqx25 (G8BPQ AX.25), ieeepup (Xerox IEEE802.3 PUP), "
     "ieeeaddrtrans (Xerox IEEE802.3 PUP Address Translation), "
     "dec (DEC Assigned), decdnadumpload (DEC DNA Dump/Load), "
     "decdnaremoteconsole (DEC DNA Remote Console), "
     "decdnarouting (DEC DNA Routing), "
     "declat (DEC LAT), decdiagnostics (DEC Diagnostics), "
     "deccustom (DEC Customer Use), "
     "decsyscomm (DEC Systems Comms Arch))",
     "User Priority",
     "User Priority Value")
{
  s_int32_t ret = NSM_DCB_API_SET_SUCCESS;
  struct interface *ifp = NULL;
  u_int8_t proto_type, pri;
  u_int16_t ether_type = 0;

  ifp = cli->index;

  /* Check the Interface */
  if (! ifp)
    return nsm_dcb_cli_return (cli, NSM_DCB_API_SET_ERR_INTERFACE);

  if (! NSM_INTF_TYPE_L2 (ifp))
    return nsm_dcb_cli_return (cli, NSM_DCB_API_SET_ERR_L2_INTERFACE);

  if (pal_strncmp (argv[0], "v", 1) == 0)
    {
      if (pal_strlen (argv[1]) != 6)
         return nsm_dcb_cli_return (cli, NSM_DCB_API_SET_ERR_INVALID_VALUE);

      pal_sscanf (argv[1], "%hx", (u_int16_t *)&ether_type);
    }
  else
    {
       /* Look thru the predefined strings */
       switch (argv[1][0])
         {
            /* arp | appletalk_ddp | appletalk_aarp |
               atm_multiprotocol| atm_transport */
            case 'a':  
            case 'A':
              {
                if (0 == pal_strcmp(argv[1],APN_CLASS_NAME_ARP))
                  ether_type = (u_int16_t) APN_CLASS_P_ARP;
                else if (0 == pal_strcmp(argv[1],APN_CLASS_NAME_ATALK))
                  ether_type = (u_int16_t) APN_CLASS_P_ATALK;
                else if (0 == pal_strcmp(argv[1],APN_CLASS_NAME_AARP))
                  ether_type = (u_int16_t) APN_CLASS_P_AARP;
                else if (0 == pal_strcmp(argv[1],APN_CLASS_NAME_ATMMPOA))
                  ether_type = (u_int16_t) APN_CLASS_P_ATMMPOA;
                else if (0 == pal_strcmp(argv[1],APN_CLASS_NAME_ATMFATE))
                  ether_type = (u_int16_t) APN_CLASS_P_ATMFATE;
                break;
              }
            /* dec | dec_dna_dump_load | dec_dna_remote_console | 
               dec_dna_routing | dec_lat | dec_diagnostics | dec_custom | 
               dec_sys_comm */
            case 'd':
            case 'D':
              {
                if (0 == pal_strcmp(argv[1],APN_CLASS_NAME_DEC))
                  ether_type = (u_int16_t) APN_CLASS_P_DEC;
                else if (0 == pal_strcmp(argv[1],APN_CLASS_NAME_DNA_DL))
                  ether_type = (u_int16_t) APN_CLASS_P_DNA_DL;
                else if(0 == pal_strcmp(argv[1],APN_CLASS_NAME_DNA_RC))
                  ether_type = (u_int16_t) APN_CLASS_P_DNA_RC;
                else if (0 == pal_strcmp(argv[1],APN_CLASS_NAME_DNA_RT))
                  ether_type = (u_int16_t) APN_CLASS_P_DNA_RT;
                else if (0 == pal_strcmp(argv[1],APN_CLASS_NAME_LAT))
                  ether_type = (u_int16_t) APN_CLASS_P_LAT;
                else if (0 == pal_strcmp(argv[1],APN_CLASS_NAME_DIAG))
                  ether_type = (u_int16_t) APN_CLASS_P_DIAG;
                else if (0 == pal_strcmp(argv[1],APN_CLASS_NAME_CUST))
                  ether_type = (u_int16_t) APN_CLASS_P_CUST;
                else if (0 == pal_strcmp(argv[1],APN_CLASS_NAME_SCA))
                  ether_type = (u_int16_t) APN_CLASS_P_SCA;
                break;
              }
            /* g8bpq_x25 */
            case 'g':  
            case 'G':
              {
                if (0 == pal_strcmp(argv[1],APN_CLASS_NAME_BPQ))
                  ether_type = (u_int16_t) APN_CLASS_P_BPQ;
                break;
              }
            /* ip, ipv6, ipx,*/
            case 'i':  
            case 'I':
              {
                if (0 == pal_strcmp(argv[1],APN_CLASS_NAME_IP))
                  ether_type = (u_int16_t) APN_CLASS_P_IP;
                else if (0 == pal_strcmp(argv[1],APN_CLASS_NAME_IPV6))
                  ether_type = (u_int16_t) APN_CLASS_P_IPV6;
                else if (0 == pal_strcmp(argv[1],APN_CLASS_NAME_IPX))
                  ether_type = (u_int16_t) APN_CLASS_P_IPX;
                else if (0 == pal_strcmp(argv[1],APN_CLASS_NAME_IEEEPUP))
                  ether_type = (u_int16_t) APN_CLASS_P_IEEEPUP;
                else if (0 == pal_strcmp(argv[1],APN_CLASS_NAME_IEEEPUPAT))
                  ether_type = (u_int16_t) APN_CLASS_P_IEEEPUPAT;
                break;
              }
            /* ppp_discovery | ppp_session */
            case 'p':  
            case 'P':
              {
                if (0 == pal_strcmp(argv[1],APN_CLASS_NAME_PPP_DISC))
                  ether_type = (u_int16_t) APN_CLASS_P_PPP_DISC;
                else if (0 == pal_strcmp(argv[1],APN_CLASS_NAME_PPP_SES))
                  ether_type = (u_int16_t) APN_CLASS_P_PPP_SES;
                break;
              } 
            /* rarp */
            case 'r':  
            case 'R':
              {
      		if (0 == pal_strcmp(argv[1],APN_CLASS_NAME_RARP))
        	  ether_type = (u_int16_t) APN_CLASS_P_RARP;
                break;
              }
            /* x25 | xerox_pup | xerox_pup_addr_trans */
            case 'x':  
            case 'X':
              {
                if (0 == pal_strcmp(argv[1],APN_CLASS_NAME_X25))
                  ether_type = (u_int16_t) APN_CLASS_P_X25;
                else if (0 == pal_strcmp(argv[1],APN_CLASS_NAME_PUPAT))
                  ether_type = (u_int16_t) APN_CLASS_P_PUPAT;
                else if (0 == pal_strcmp(argv[1],APN_CLASS_NAME_PUP))
                  ether_type = (u_int16_t) APN_CLASS_P_PUP;
                break;
              }
            default:
              break;
          }
    }

  CLI_GET_UINT32 ("Priority", pri, argv[2]);

  proto_type = NSM_DCB_PROTO_ETHERTYPE;

  ret = nsm_dcb_application_priority_unset (cli->vr->id, ifp->name, proto_type,
                                            ether_type, pri);

  return nsm_dcb_cli_return (cli, ret);
}

/**@brief This CLI enables priority flow control on the interface.
   @return Error returned by API/CLI_SUCCESS
*/
CLI (dcb_pfc_interface_enable,
     dcb_pfc_interface_enable_cmd,
     "priority-flow-control enable",
     NSM_PFC_STR,
     "Enable on interface")
{
  s_int32_t ret = NSM_DCB_API_SET_SUCCESS;
  struct interface *ifp = NULL;

  ifp = cli->index;

  /* Check the Interface */
  if (! ifp)
    return nsm_dcb_cli_return (cli, NSM_DCB_API_SET_ERR_INTERFACE);

  if (! NSM_INTF_TYPE_L2 (ifp))
    return nsm_dcb_cli_return (cli, NSM_DCB_API_SET_ERR_L2_INTERFACE);

  ret = nsm_dcb_pfc_enable_interface (cli->vr->id, ifp->name);
  
  return nsm_dcb_cli_return (cli, ret);
}

/**@brief This CLI disables priority flow control on the interface.
   @return Error returned by API/CLI_SUCCESS
*/
CLI (dcb_pfc_interface_disable,
     dcb_pfc_interface_disable_cmd,
     "no priority-flow-control enable",
     CLI_NO_STR,
     NSM_PFC_STR,
     "Disable on interface")
{
  s_int32_t ret = NSM_DCB_API_SET_SUCCESS;
  struct interface *ifp = NULL;

  ifp = cli->index;

  /* Check the Interface */
  if (! ifp)
    return nsm_dcb_cli_return (cli, NSM_DCB_API_SET_ERR_INTERFACE);

  if (! NSM_INTF_TYPE_L2 (ifp))
    return nsm_dcb_cli_return (cli, NSM_DCB_API_SET_ERR_L2_INTERFACE);

  ret = nsm_dcb_pfc_disable_interface (cli->vr->id, ifp->name);

  return nsm_dcb_cli_return (cli, ret);
}


/**@brief This CLI enables priority flow control for a given priority
          on the interface.
   @return CLI_SUCCESS
*/
CLI (dcb_enable_pfc_priority,
     dcb_enable_pfc_priority_cmd,
     "priority-flow-control enable priority (<0-7> | <0-7> <0-7> |"
     "<0-7> <0-7> <0-7> | <0-7> <0-7> <0-7> <0-7> |"
     "<0-7> <0-7> <0-7> <0-7> <0-7> |"
     "<0-7> <0-7> <0-7> <0-7> <0-7> <0-7> |"
     "<0-7> <0-7> <0-7> <0-7> <0-7> <0-7> <0-7> |"
     "<0-7> <0-7> <0-7> <0-7> <0-7> <0-7> <0-7> <0-7>)",
     NSM_PFC_STR,
     "enable" 
     "traffic-priorities",
     "traffic-priority value",
     "traffic-priority value",
     "traffic-priority value",
     "traffic-priority value",
     "traffic-priority value",
     "traffic-priority value",
     "traffic-priority value",
     "traffic-priority value",
     "traffic-priority value",
     "traffic-priority value",
     "traffic-priority value",
     "traffic-priority value",
     "traffic-priority value",
     "traffic-priority value",
     "traffic-priority value",
     "traffic-priority value",
     "traffic-priority value",
     "traffic-priority value",
     "traffic-priority value",
     "traffic-priority value",
     "traffic-priority value",
     "traffic-priority value",
     "traffic-priority value",
     "traffic-priority value",
     "traffic-priority value",
     "traffic-priority value",
     "traffic-priority value",
     "traffic-priority value",
     "traffic-priority value",
     "traffic-priority value",
     "traffic-priority value",
     "traffic-priority value",
     "traffic-priority value",
     "traffic-priority value",
     "traffic-priority value",
     "traffic-priority value")
{
  s_int32_t ret = NSM_DCB_API_SET_SUCCESS;
  struct interface *ifp = NULL;
  s_int8_t priority = 0, pri;

  ifp = cli->index;

  /* Check the Interface */
  if (! ifp)
    return nsm_dcb_cli_return (cli, NSM_DCB_API_SET_ERR_INTERFACE);

  if (! NSM_INTF_TYPE_L2 (ifp))
    return nsm_dcb_cli_return (cli, NSM_DCB_API_SET_ERR_L2_INTERFACE);

  /* Check the range for priority*/
  switch (argc)
    {
      case 8:
        CLI_GET_INTEGER_RANGE ("Priority", pri, argv[7], 0, 7);
        priority = priority | (1 << pri);
      case 7:
        CLI_GET_INTEGER_RANGE ("Priority", pri, argv[6], 0, 7);
        priority = priority | (1 << pri);
      case 6:
        CLI_GET_INTEGER_RANGE ("Priority", pri, argv[5], 0, 7);
        priority = priority | (1 << pri);
      case 5:
        CLI_GET_INTEGER_RANGE ("Priority", pri, argv[4], 0, 7);
        priority = priority | (1 << pri);
      case 4:
        CLI_GET_INTEGER_RANGE ("Priority", pri, argv[3], 0, 7);
        priority = priority | (1 << pri);
      case 3:
        CLI_GET_INTEGER_RANGE ("Priority", pri, argv[2], 0, 7);
        priority = priority | (1 << pri);
      case 2:
        CLI_GET_INTEGER_RANGE ("Priority", pri, argv[1], 0, 7);
        priority = priority | (1 << pri);
      case 1:
        CLI_GET_INTEGER_RANGE ("Priority", pri, argv[0], 0, 7);
        priority = priority | (1 << pri);
        break;
      default:
         nsm_dcb_cli_return (cli, NSM_DCB_API_SET_ERR_INVALID_VALUE);
    }

  /*Check for duplicate arguments*/
  if ((ret = nsm_find_duplicate_args (argc, priority)) == 
          NSM_DCB_API_SET_ERR_DUPLICATE_ARGS)
    return nsm_dcb_cli_return (cli, ret); 

  ret = nsm_dcb_add_pfc_priority (cli->vr->id, ifp->name, priority, PAL_TRUE);

  return nsm_dcb_cli_return (cli, ret);
}

/**@brief This CLI disables priority flow control on all the priorities
          of the interface.
   @return CLI_SUCCESS
*/
CLI (dcb_disable_pfc_priority,
     dcb_disable_pfc_priority_cmd,
     "no priority-flow-control enable priority (<0-7> | <0-7> <0-7> |"
     "<0-7> <0-7> <0-7> | <0-7> <0-7> <0-7> <0-7> |"
     "<0-7> <0-7> <0-7> <0-7> <0-7> |"
     "<0-7> <0-7> <0-7> <0-7> <0-7> <0-7> |"
     "<0-7> <0-7> <0-7> <0-7> <0-7> <0-7> <0-7> |"
     "<0-7> <0-7> <0-7> <0-7> <0-7> <0-7> <0-7> <0-7>)",
     CLI_NO_STR,
     NSM_PFC_STR,
     "disable",
     "traffic-priority",
     "traffic-priority value",
     "traffic-priority value",
     "traffic-priority value",
     "traffic-priority value",
     "traffic-priority value",
     "traffic-priority value",
     "traffic-priority value",
     "traffic-priority value",
     "traffic-priority value",
     "traffic-priority value",
     "traffic-priority value",
     "traffic-priority value",
     "traffic-priority value",
     "traffic-priority value",
     "traffic-priority value",
     "traffic-priority value",
     "traffic-priority value",
     "traffic-priority value",
     "traffic-priority value",
     "traffic-priority value",
     "traffic-priority value",
     "traffic-priority value",
     "traffic-priority value",
     "traffic-priority value",
     "traffic-priority value",
     "traffic-priority value",
     "traffic-priority value",
     "traffic-priority value",
     "traffic-priority value",
     "traffic-priority value",
     "traffic-priority value",
     "traffic-priority value",
     "traffic-priority value",
     "traffic-priority value",
     "traffic-priority value",
     "traffic-priority value")
{
  s_int32_t ret = NSM_DCB_API_SET_SUCCESS;
  struct interface *ifp = NULL;
  s_int8_t priority = 0, pri;

  ifp = cli->index;

  /* Check the Interface */
  if (! ifp)
    return nsm_dcb_cli_return (cli, NSM_DCB_API_SET_ERR_INTERFACE);

  if (! NSM_INTF_TYPE_L2 (ifp))
    return nsm_dcb_cli_return (cli, NSM_DCB_API_SET_ERR_L2_INTERFACE);

  /* Check the range for priority*/
  switch (argc)
    {
      case 8:
        CLI_GET_INTEGER_RANGE ("Priority", pri, argv[7], 0, 7);
        priority = priority | (1 << pri);
      case 7:
        CLI_GET_INTEGER_RANGE ("Priority", pri, argv[6], 0, 7);
        priority = priority | (1 << pri);
      case 6:
        CLI_GET_INTEGER_RANGE ("Priority", pri, argv[5], 0, 7);
        priority = priority | (1 << pri);
      case 5:
        CLI_GET_INTEGER_RANGE ("Priority", pri, argv[4], 0, 7);
        priority = priority | (1 << pri);
      case 4:
        CLI_GET_INTEGER_RANGE ("Priority", pri, argv[3], 0, 7);
        priority = priority | (1 << pri);
      case 3:
        CLI_GET_INTEGER_RANGE ("Priority", pri, argv[2], 0, 7);
        priority = priority | (1 << pri);
      case 2:
        CLI_GET_INTEGER_RANGE ("Priority", pri, argv[1], 0, 7);
        priority = priority | (1 << pri);
      case 1:
        CLI_GET_INTEGER_RANGE ("Priority", pri, argv[0], 0, 7);
        priority = priority | (1 << pri);
        break;
        
      default:
         nsm_dcb_cli_return (cli, NSM_DCB_API_SET_ERR_INVALID_VALUE);
    }

  /*Check for duplicate arguments*/
  if ((ret = nsm_find_duplicate_args (argc, priority)) ==
           NSM_DCB_API_SET_ERR_DUPLICATE_ARGS)
    return nsm_dcb_cli_return (cli, ret);
    
  ret = nsm_dcb_add_pfc_priority (cli->vr->id, ifp->name, priority, PAL_FALSE);

  return nsm_dcb_cli_return (cli, ret);
}

ALI (dcb_disable_pfc_priority,
     dcb_disable_all_pfc_priority_cmd,
     "no priority-flow-control enable priority",
     CLI_NO_STR,
     NSM_PFC_STR,
     "disable"
     "on all priorities");

/**@brief This CLI sets PFC mode on the interface.
   @return CLI_SUCCESS
*/
CLI (dcb_pfc_set_mode,
     dcb_pfc_set_mode_cmd,
     "priority-flow-control mode (on | auto)",
     NSM_PFC_STR,
     "Mode of the Priority Flow Control",
     "force enable priority-flow-control",
     "negotiate priority-flow-control capabilities")
{
  s_int32_t ret = NSM_DCB_API_SET_SUCCESS;
  struct interface *ifp = NULL;
  u_int8_t mode = NSM_DCB_PFC_MODE_ON;

  ifp = cli->index;

  /* Check the Interface */
  if (! ifp)
    return nsm_dcb_cli_return (cli, NSM_DCB_API_SET_ERR_INTERFACE);

  if (! NSM_INTF_TYPE_L2 (ifp))
    return nsm_dcb_cli_return (cli, NSM_DCB_API_SET_ERR_L2_INTERFACE);

  /* Check the mode */
  CLI_GET_DCB_MODE (mode, argv[0]);

  ret = nsm_dcb_set_pfc_mode(cli->vr->id, ifp->name, mode);

  return nsm_dcb_cli_return (cli, ret);

}

/**@brief This CLI sets PFC cap for the interface.
   @return CLI_SUCCESS
*/
CLI (dcb_pfc_set_cap,
     dcb_pfc_set_cap_cmd,
     "priority-flow-control cap <0-8>",
     NSM_PFC_STR,
     "Cap for number of priorities for interface",
     "cap value (Zero indicates no limitations)")
{
  s_int32_t ret = NSM_DCB_API_SET_SUCCESS;
  struct interface *ifp = NULL;
  u_int8_t cap = 0;

  ifp = cli->index;

  /* Check the Interface */
  if (! ifp)
    return nsm_dcb_cli_return (cli, NSM_DCB_API_SET_ERR_INTERFACE);

  if (! NSM_INTF_TYPE_L2 (ifp))
    return nsm_dcb_cli_return (cli, NSM_DCB_API_SET_ERR_L2_INTERFACE);

  CLI_GET_INTEGER_RANGE ("PFC Cap", cap, argv[0], 1, 8);

  ret = nsm_dcb_set_pfc_cap(cli->vr->id, ifp->name, cap);

  return nsm_dcb_cli_return (cli, ret);

}

/**@brief This CLI unset PFC cap for the interface.
   @return CLI_SUCCESS
*/
CLI (dcb_pfc_unset_cap,
     dcb_pfc_unset_cap_cmd,
     "no priority-flow-control cap",
     CLI_NO_STR,
     NSM_PFC_STR,
     "Cap for number of priorities for interface")
{
  s_int32_t ret = NSM_DCB_API_SET_SUCCESS;
  struct interface *ifp = NULL;

  ifp = cli->index;

  /* Check the Interface */
  if (! ifp)
    return nsm_dcb_cli_return (cli, NSM_DCB_API_SET_ERR_INTERFACE);

  if (! NSM_INTF_TYPE_L2 (ifp))
    return nsm_dcb_cli_return (cli, NSM_DCB_API_SET_ERR_L2_INTERFACE);

  ret = nsm_dcb_set_pfc_cap(cli->vr->id, ifp->name, NSM_DCB_PFC_CAP_DEFAULT);

  return nsm_dcb_cli_return (cli, ret);

}

/**@brief This CLI sets PFC link delay allowance for the interface.
   @return CLI_SUCCESS
*/
CLI (dcb_pfc_set_link_delay_allowance,
     dcb_pfc_set_link_delay_allowance_cmd,
     "priority-flow-control link-delay-allowance <0-4294967295>",
     NSM_PFC_STR,
     "Link Delay Allowance",
     "value (Zero indicates no allowance)")
{
  s_int32_t ret = NSM_DCB_API_SET_SUCCESS;
  struct interface *ifp = NULL;
  u_int32_t lda = 0;

  ifp = cli->index;

  /* Check the Interface */
  if (! ifp)
    return nsm_dcb_cli_return (cli, NSM_DCB_API_SET_ERR_INTERFACE);

  if (! NSM_INTF_TYPE_L2 (ifp))
    return nsm_dcb_cli_return (cli, NSM_DCB_API_SET_ERR_L2_INTERFACE);

  CLI_GET_UINT32_RANGE ("PFC link delay allowance", lda, argv[0], 0,
                          4294967295);

  ret = nsm_dcb_set_pfc_lda(cli->vr->id, ifp->name, lda);

  return nsm_dcb_cli_return (cli, ret);

}

/**@brief This CLI unset PFC link delay allowance for the interface.
   @return CLI_SUCCESS
*/
CLI (dcb_pfc_unset_link_delay_allowance,
     dcb_pfc_unset_link_delay_allowance_cmd,
     "no priority-flow-control link-delay-allowance",
     CLI_NO_STR,
     NSM_PFC_STR,
     "Link Delay Allowance")
{
  s_int32_t ret = NSM_DCB_API_SET_SUCCESS;
  struct interface *ifp = NULL;

  ifp = cli->index;

  /* Check the Interface */
  if (! ifp)
    return nsm_dcb_cli_return (cli, NSM_DCB_API_SET_ERR_INTERFACE);

  if (! NSM_INTF_TYPE_L2 (ifp))
    return nsm_dcb_cli_return (cli, NSM_DCB_API_SET_ERR_L2_INTERFACE);

  ret = nsm_dcb_set_pfc_lda(cli->vr->id, ifp->name,
                            NSM_DCB_PFC_LINK_DELAY_ALLOW_DEFAULT);

  return nsm_dcb_cli_return (cli, ret);
}

/**@brief This funcrion does CLI Initialization 
   @param ctree - CLI tree pointer
   @return None
*/
void_t 
nsm_dcb_cli_init (struct cli_tree *ctree)
{
  cli_install_config (ctree, DCB_MODE, nsm_dcb_config_write);  
  cli_install_default (ctree, DCB_MODE);

  /* CONFIG mode commands */
  cli_install (ctree, CONFIG_MODE, &dcb_bridge_enable_cmd);
  cli_install (ctree, CONFIG_MODE, &dcb_bridge_disable_cmd); 
  cli_install (ctree, CONFIG_MODE, &dcb_ets_bridge_enable_cmd);
  cli_install (ctree, CONFIG_MODE, &dcb_ets_bridge_disable_cmd);
  cli_install (ctree, CONFIG_MODE, &dcb_pfc_bridge_enable_cmd);
  cli_install (ctree, CONFIG_MODE, &dcb_pfc_bridge_disable_cmd);

#ifdef HAVE_DEFAULT_BRIDGE
   cli_install (ctree, CONFIG_MODE, &dcb_bridge_enable_default_cmd);
   cli_install (ctree, CONFIG_MODE, &dcb_bridge_disable_default_cmd);
   cli_install (ctree, CONFIG_MODE, &dcb_ets_bridge_enable_default_cmd);
   cli_install (ctree, CONFIG_MODE, &dcb_ets_bridge_disable_default_cmd);
   cli_install (ctree, CONFIG_MODE, &dcb_pfc_bridge_enable_default_cmd);
   cli_install (ctree, CONFIG_MODE, &dcb_pfc_bridge_disable_default_cmd);
#endif /* HAVE_DEFAULT_BRIDGE */

  /* INTERFACE mode commands */
  cli_install (ctree, INTERFACE_MODE, &dcb_interface_enable_cmd);
  cli_install (ctree, INTERFACE_MODE, &dcb_interface_disable_cmd);
  cli_install (ctree, INTERFACE_MODE, &dcb_ets_interface_enable_cmd);
  cli_install (ctree, INTERFACE_MODE, &dcb_ets_interface_disable_cmd);
  cli_install (ctree, INTERFACE_MODE, &dcb_ets_set_mode_cmd);
  cli_install (ctree, INTERFACE_MODE, &dcb_ets_add_priority_cmd);
  cli_install (ctree, INTERFACE_MODE, &dcb_ets_remove_priority_cmd);
  cli_install (ctree, INTERFACE_MODE, &dcb_ets_assign_bw_cmd);
  cli_install (ctree, INTERFACE_MODE, &dcb_ets_delete_tcgs_cmd);
  cli_install (ctree, INTERFACE_MODE, &dcb_set_appl_priority_ports_cmd);
  cli_install (ctree, INTERFACE_MODE, &dcb_unset_appl_priority_ports_cmd);
  cli_install (ctree, INTERFACE_MODE, &dcb_set_appl_priority_ethertype_cmd);
  cli_install (ctree, INTERFACE_MODE, &dcb_unset_appl_priority_ethertype_cmd);
  cli_install (ctree, INTERFACE_MODE, &dcb_pfc_interface_enable_cmd);
  cli_install (ctree, INTERFACE_MODE, &dcb_pfc_interface_disable_cmd);
  cli_install (ctree, INTERFACE_MODE, &dcb_enable_pfc_priority_cmd);
  cli_install (ctree, INTERFACE_MODE, &dcb_disable_pfc_priority_cmd);
  cli_install (ctree, INTERFACE_MODE, &dcb_pfc_set_mode_cmd);
  cli_install (ctree, INTERFACE_MODE, &dcb_pfc_set_cap_cmd);
  cli_install (ctree, INTERFACE_MODE, &dcb_pfc_unset_cap_cmd);
  cli_install (ctree, INTERFACE_MODE, &dcb_pfc_set_link_delay_allowance_cmd);
  cli_install (ctree, INTERFACE_MODE, &dcb_pfc_unset_link_delay_allowance_cmd);
  cli_install (ctree, INTERFACE_MODE, &dcb_disable_pfc_priority_cmd);
  cli_install (ctree, INTERFACE_MODE, &dcb_disable_all_pfc_priority_cmd);

  /* Show CLIs */
  nsm_dcb_show_cli_init (ctree);
}

#endif /* HAVE_DCB */
