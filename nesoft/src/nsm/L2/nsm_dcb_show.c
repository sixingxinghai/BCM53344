/* Copyright (C) 2009  All Rights Reserved */

/**@file nsm_dcb_show.c
   @brief This nsm_dcb_show.c file defines CLIs related to DCB
          show functionality.
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
#include "lib/vrep.h"

#ifdef HAVE_DCB
#include "nsm_dcb.h"
#include "nsm_dcb_cli.h"
#include "nsm_dcb_api.h"

/**@brief This function traverses the avl tree of dcb interface list and 
          display traffic class group information present on the 
          interface
   @param avl_node_info - Node information, pointer to dcb interface
   @param arg1 - traffic-class-group identifier
   @param arg2 - Pointer to CLI 
   @return NSM_DCB_API_SET_SUCCESS
*/
s_int32_t 
nsm_dcb_avl_traverse_show_tcgs (void_t *avl_node_info, void_t *arg1, 
                                void_t *arg2)
{
  struct nsm_dcb_if *dcbif;
  struct cli *cli;
  u_int8_t *tmp_tcgid, tcgid;
  char pri_str[20], pri[3];
  u_int8_t pfc_state = 0, priority;
 
  NSM_DCB_FN_ENTER ();

  if (! (dcbif = (struct nsm_dcb_if *) avl_node_info)
      || ! (tmp_tcgid = (u_int8_t *) arg1)
      || ! (cli = (struct cli *) arg2))
    NSM_DCB_FN_EXIT (NSM_DCB_API_SET_ERR_INVALID_VALUE);

  /* Check ETS is enabled on interface */
  if (! CHECK_FLAG (dcbif->dcb_if_flags, NSM_DCB_IF_ETS_ENABLE))
    {
      zlog_err (nzg, "ETS is not enabled on interface %s Ignoring...\n",
                      dcbif->ifp->name);
      NSM_DCB_FN_EXIT (NSM_DCB_API_SET_SUCCESS);
    }

  tcgid = *tmp_tcgid;

  if ((tcgid != NSM_DCB_ETS_DEFAULT_TCGID && 
       dcbif->tcg_priority_count[tcgid] > 0) ||
       tcgid == NSM_DCB_ETS_DEFAULT_TCGID)
    {
      pal_sprintf (pri_str, "");
      for (priority = 0; priority < NSM_DCB_NUM_USER_PRI; priority++)
         {
           if (dcbif->tcg_priority_table[priority] == tcgid)
             {
                pal_sprintf (pri,"%d", priority);
                pal_strcat (pri_str, " ");
                pal_strcat (pri_str, pri);
                pfc_state = dcbif->pfc_enable_vector[priority];
             }
         }
         
        if (pal_strncmp (pri_str, "", 1) != 0)
          {
            cli_out (cli, "%-14s", dcbif->ifp->name);
            cli_out (cli, "%-21s", pri_str);
            if (tcgid != NSM_DCB_ETS_DEFAULT_TCGID)
              cli_out (cli, "%-20s", pfc_state ? "Enable":"Disable");
            else
              cli_out (cli, "%-15s", pfc_state ? "Enable":"Disable");

            if (tcgid != NSM_DCB_ETS_DEFAULT_TCGID)
              cli_out (cli, "%d\n", dcbif->tcg_bandwidth_table[tcgid]);
            else
              cli_out (cli, "No Bandwidth Limit\n");
          }
      }

  NSM_DCB_FN_EXIT (NSM_DCB_API_SET_SUCCESS);
}

/**@brief This function displays the traffic class group information for 
          given traffic class group identifier.
   @param cli - Pointer to CLI 
   @param tcgid - traffic-class-group Identifier
   @return NSM_DCB_API_SET_SUCCESS
*/
s_int32_t
nsm_dcb_show_tcg_by_id (struct cli* cli, u_int8_t tcgid, char* bridge_name)
{
  struct nsm_master *nm;
  struct nsm_bridge_master *master;
  struct nsm_bridge *bridge;
  struct nsm_dcb_bridge *dcbg;
  s_int32_t ret;

  NSM_DCB_FN_ENTER ();

  /* Look for NSM master */
  nm = nsm_master_lookup_by_id (nzg, cli->vr->id);
  if (! nm)
    NSM_DCB_FN_EXIT (NSM_DCB_API_SET_ERR_NO_NM);

  master = nsm_bridge_get_master (nm);
  if (! master)
    NSM_DCB_FN_EXIT (NSM_BRIDGE_ERR_NOTFOUND);
 
  if (bridge_name == NULL)
#ifdef HAVE_DEFAULT_BRIDGE
    bridge = nsm_get_default_bridge (master);
#else
    NSM_DCB_FN_EXIT (NSM_BRIDGE_ERR_NOTFOUND);
#endif /* HAVE_DEFAULT_BRIDGE */
  else
    bridge = nsm_lookup_bridge_by_name (master, bridge_name);

  if (! bridge)
    NSM_DCB_FN_EXIT (NSM_BRIDGE_ERR_NOTFOUND);

  /* Look for the dcb bridge structure */
  dcbg = bridge->dcbg;
  if (! dcbg)
    NSM_DCB_FN_EXIT (NSM_DCB_API_SET_ERR_NO_DCBG);

  if (! CHECK_FLAG (dcbg->dcb_global_flags, NSM_DCB_ENABLE))
    NSM_DCB_FN_EXIT (NSM_DCB_API_SET_ERR_NO_DCB);

  if (! CHECK_FLAG (dcbg->dcb_global_flags, NSM_DCB_ETS_ENABLE))
    NSM_DCB_FN_EXIT (NSM_DCB_API_SET_ERR_NO_ETS);
      
  if (avl_get_tree_size (bridge->port_tree) > 0)
    {
      cli_out (cli, "TCGID : %d\n", tcgid);
      cli_out (cli, "Bridge : %s\n", bridge->name);
      cli_out (cli, "%-15s", "Interface");
      cli_out (cli, "%-20s", "Priorities");
      cli_out (cli, "%-15s","PFC State");
      cli_out (cli, "%s\n", "Bandwidth %");
      cli_out (cli, "----------------------------------------------------"
                    "---------------------\n");

      ret = avl_traverse2 (dcbg->dcb_if_list, 
                           nsm_dcb_avl_traverse_show_tcgs,
                           (void_t *) &tcgid, (void_t *) cli);
    }
  else
    NSM_DCB_FN_EXIT (NSM_DCB_API_SET_ERR_DISPLAY_NO_TCG);

  NSM_DCB_FN_EXIT (NSM_DCB_API_SET_SUCCESS);
}

/**@brief This function displays traffic class group information on the given
          interface.
   @param cli - pointer to CLI 
   @param ifname - Name of interface 
   @return NSM_DCB_API_SET_SUCCESS
*/
s_int32_t 
nsm_dcb_show_tcg_by_intf (struct cli *cli, char *ifname)
{
  struct nsm_master *nm;
  struct nsm_bridge_port *br_port;
  struct nsm_bridge *bridge;
  struct nsm_if *zif;
  struct avl_node *node;
  struct nsm_dcb_bridge *dcbg;
  struct interface *ifp;
  struct nsm_dcb_if *dcbif = NULL, tmp_dcbif;
  char pri_str[20], pri[2];
  u_int8_t pfc_state = 0, priority, tcgid;
  bool_t has_pri_in_tcgid = PAL_FALSE;

  NSM_DCB_FN_ENTER ();

  /* Look for NSM master */
  nm = nsm_master_lookup_by_id (nzg, cli->vr->id);
  if (! nm)
    NSM_DCB_FN_EXIT (NSM_DCB_API_SET_ERR_NO_NM);

  ifp = if_lookup_by_name (&nm->vr->ifm, ifname);
  if (! ifp)
    NSM_DCB_FN_EXIT (NSM_DCB_API_SET_ERR_INTERFACE);
    
  /* Get the bridge information */
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

  if (! CHECK_FLAG (dcbg->dcb_global_flags, NSM_DCB_ENABLE))
    NSM_DCB_FN_EXIT (NSM_DCB_API_SET_ERR_NO_DCB);

  tmp_dcbif.ifp = ifp;

  /* Find the DCB interface */
  node = avl_search (dcbg->dcb_if_list, (void *) &tmp_dcbif);
  if (node)
    dcbif = (struct nsm_dcb_if *) node->info;

  if (! dcbif)
    NSM_DCB_FN_EXIT (NSM_DCB_API_SET_ERR_DCB_INTERFACE);

  /* Check DCB is enabled on interface */
  if (! CHECK_FLAG (dcbif->dcb_if_flags, NSM_DCB_IF_ENABLE))
    NSM_DCB_FN_EXIT (NSM_DCB_API_SET_ERR_INTERFACE_NO_DCB);

  /* Check ETS is enabled on interface */
  if (! CHECK_FLAG (dcbif->dcb_if_flags, NSM_DCB_IF_ETS_ENABLE))
    NSM_DCB_FN_EXIT (NSM_DCB_API_SET_ERR_INTERFACE_NO_ETS);

  cli_out (cli, "Interface : %s       ETS Mode : %s\n", 
                 ifp->name, dcbif->dcb_ets_mode ? "auto" : "on");
  cli_out (cli, "%-10s", "TCGID");
  cli_out (cli, "%-18s", "Priorities");
  cli_out (cli, "%-15s", "PFC State");
  cli_out (cli, "%s\n", "Bandwidth %");
  cli_out (cli, "-----------------------------------------------------------"
                 "---\n");

  /* Look for traffic class groups configured on interface */
  for (tcgid = 0; tcgid < NSM_DCB_MAX_TCG_DEFAULT; tcgid++)
    {
      if (dcbif->tcg_priority_count[tcgid] > 0)
        {
          pal_sprintf (pri_str, "");
          for (priority = 0; priority < NSM_DCB_NUM_USER_PRI; priority++)
             {
               if (dcbif->tcg_priority_table[priority] == tcgid)
                 {
                    pal_sprintf (pri,"%d", priority);
                    pal_strcat (pri_str, " ");
                    pal_strcat (pri_str, pri);
                    pfc_state = dcbif->pfc_enable_vector[priority];
                 }
             }
     
          cli_out (cli, "%-9d", tcgid);
          cli_out (cli, "%-19s", pri_str);
          cli_out (cli, "%-20s",pfc_state ? "Enable":"Disable");
          cli_out (cli, "%d\n", dcbif->tcg_bandwidth_table[tcgid]);
        } 
     }

  tcgid = NSM_DCB_ETS_DEFAULT_TCGID;
  pal_sprintf (pri_str, "");
  for (priority = 0; priority < NSM_DCB_NUM_USER_PRI; priority++)
    {
       if (dcbif->tcg_priority_table[priority] == tcgid)
         {
           pal_sprintf (pri,"%d", priority);
           pal_strcat (pri_str, " ");
           pal_strcat (pri_str, pri);
           pfc_state = dcbif->pfc_enable_vector[priority];
           has_pri_in_tcgid = PAL_TRUE;
         }
     }

   if (has_pri_in_tcgid)
     {
       cli_out (cli, "%-9d", tcgid);
       cli_out (cli, "%-19s", pri_str);
       cli_out (cli, "%-15s", pfc_state ? "Enable":"Disable");
       cli_out (cli, "NO Bandwidth Limit\n");
    }

  NSM_DCB_FN_EXIT (NSM_DCB_API_SET_SUCCESS);
}

/**@brief This function traverses the avl tree of the dcb interfaces and 
          displays Traffic class group information.
*/
s_int32_t 
nsm_dcb_avl_traverse_show_bridge (void_t *avl_node_info, void_t *arg1)
{
   struct nsm_dcb_if *dcbif;
   struct cli *cli;
   char pri_str[20], pri[3];
   u_int8_t pfc_state = 0, tcgid, priority; 
   bool_t has_pri_in_tcgid = PAL_FALSE;
 
   NSM_DCB_FN_ENTER ();

   if (! (dcbif = (struct nsm_dcb_if *) avl_node_info)
       || ! (cli = (struct cli *) arg1))
     NSM_DCB_FN_EXIT (NSM_DCB_API_SET_ERR_INVALID_VALUE);

   /* Check DCB is enabled on interface 
      Ignore if disabled as the CLI displays information on the bridge 
      Hence if disabled on one interface of bridge jump to next interface. */
   if (! CHECK_FLAG (dcbif->dcb_if_flags, NSM_DCB_IF_ENABLE))
     { 
       zlog_err (nzg, "DCB is disabled on the interface %s\n", 
                 dcbif->ifp->name);
       NSM_DCB_FN_EXIT (NSM_DCB_API_SET_SUCCESS);
     }

   /* Check ETS is enabled on interface */
   if (! CHECK_FLAG (dcbif->dcb_if_flags, NSM_DCB_IF_ETS_ENABLE))
     {
       zlog_err (nzg, "ETS is disabled on the interface %s\n", 
                 dcbif->ifp->name);
       NSM_DCB_FN_EXIT (NSM_DCB_API_SET_SUCCESS);
     }

   /* Look for traffic class groups configured on interface */
   for (tcgid = 0; tcgid < NSM_DCB_MAX_TCG_DEFAULT; tcgid++)
      {
   	if (dcbif->tcg_priority_count[tcgid] > 0)
     	  {
            pal_sprintf (pri_str, "");
            for (priority = 0; priority < NSM_DCB_NUM_USER_PRI; priority++)
               {
                 if (dcbif->tcg_priority_table[priority] == tcgid)
                   {
                     pal_sprintf (pri,"%d", priority);
                     pal_strcat (pri_str, " ");
                     pal_strcat (pri_str, pri);
                     pfc_state = dcbif->pfc_enable_vector[priority];
                   }
               }
            cli_out (cli, "%-15s", dcbif->ifp->name);
            cli_out (cli, "%-11d", tcgid);
            cli_out (cli, "%-21s", pri_str);
            cli_out (cli, "%-20s", pfc_state ? "Enable":"Disable");
            cli_out (cli, "%d\n", dcbif->tcg_bandwidth_table[tcgid]);
          }
      }

   tcgid = NSM_DCB_ETS_DEFAULT_TCGID;
   pal_sprintf (pri_str, "");
   for (priority = 0; priority < NSM_DCB_NUM_USER_PRI; priority++)
     {
       if (dcbif->tcg_priority_table[priority] == tcgid)
         {
           pal_sprintf (pri,"%d", priority);
           pal_strcat (pri_str, " ");
           pal_strcat (pri_str, pri);
           pfc_state = dcbif->pfc_enable_vector[priority];
           has_pri_in_tcgid = PAL_TRUE;
         }
      }

    if (has_pri_in_tcgid)
     {
       cli_out (cli, "%-15s", dcbif->ifp->name);
       cli_out (cli, "%-11d", tcgid);
       cli_out (cli, "%-21s", pri_str);
       cli_out (cli, "%-15s", pfc_state ? "Enable":"Disable");
       cli_out (cli, "%s\n", "No Bandwidth Limit");
     }

   NSM_DCB_FN_EXIT (NSM_DCB_API_SET_SUCCESS);
}

/**@brief This function displays traffic class group information configured on 
          given bridge.
   @param cli - pointer to CLI
   @param bridge_name - Name of bridge
   @return NSM_DCB_API_SET_SUCCESS
*/
s_int32_t
nsm_dcb_show_tcg_by_bridge (struct cli *cli, char *bridge_name)
{ 
  struct nsm_master *nm;
  struct nsm_bridge_master *master;
  struct nsm_bridge *bridge;
  struct nsm_dcb_bridge *dcbg;
  s_int32_t ret;

  NSM_DCB_FN_ENTER ();

  /* Look for NSM master */
  nm = nsm_master_lookup_by_id (nzg, cli->vr->id);
  if (! nm)
    NSM_DCB_FN_EXIT (NSM_DCB_API_SET_ERR_NO_NM);

  /* look for the bridge master */
  master = nsm_bridge_get_master (nm);
  if (! master)
    NSM_DCB_FN_EXIT (NSM_BRIDGE_ERR_NOTFOUND);

  if (bridge_name == NULL)
#ifdef HAVE_DEFAULT_BRIDGE
    bridge = nsm_get_default_bridge (master);
#else
    NSM_DCB_FN_EXIT (NSM_BRIDGE_ERR_NOTFOUND);
#endif /* HAVE_DEFAULT_BRIDGE */
  else
    bridge = nsm_lookup_bridge_by_name (master, bridge_name);
 
  if (! bridge)
    NSM_DCB_FN_EXIT (NSM_BRIDGE_ERR_NOTFOUND);

  /* Look for the dcb bridge structure */
  dcbg = bridge->dcbg;
  if (! dcbg)
    NSM_DCB_FN_EXIT (NSM_DCB_API_SET_ERR_NO_DCBG);

  if (! CHECK_FLAG (dcbg->dcb_global_flags, NSM_DCB_ENABLE))
    NSM_DCB_FN_EXIT (NSM_DCB_API_SET_ERR_NO_DCB);

  if (! CHECK_FLAG (dcbg->dcb_global_flags, NSM_DCB_ETS_ENABLE))
    NSM_DCB_FN_EXIT (NSM_DCB_API_SET_ERR_NO_ETS);

  cli_out (cli, "bridge : %s\n", bridge->name);

  cli_out (cli, "%-15s","Interface" );
  cli_out (cli, "%-12s", "TCGID");
  cli_out (cli, "%-20s", "Priorities");
  cli_out (cli, "%-15s", "PFC State");
  cli_out (cli, "%s\n", "Bandwidth %");

  cli_out (cli, "----------------------------------------------------------"
                "---------------------\n");

  /* Traverse the list */
  ret = avl_traverse (dcbg->dcb_if_list, nsm_dcb_avl_traverse_show_bridge,
                     (void_t *) cli);

  if (ret < 0)
    NSM_DCB_FN_EXIT (ret);

  NSM_DCB_FN_EXIT (NSM_DCB_API_SET_SUCCESS);
}

/**@brief This function traverses the application priority avl tree to 
          display application priority table on the interface
   @param avl_node_info - Pointer to entry in application priority table
   @param arg1 - Pointer to CLI
   @return NSM_DCB_API_SET_SUCCESS
*/
s_int32_t
nsm_dcb_avl_traverse_show_appl_pri_tree (void_t *avl_node_info, void_t *arg1)
{ 
   struct nsm_dcb_application_priority *app_pri;
   struct cli *cli;
   char *proto_str, proto_id_str[20], *ether_str;
   struct pal_servent *sp;
   u_int8_t len = 0;
   u_int16_t eth_value;
   
   NSM_DCB_FN_ENTER ();

   if (! (app_pri = (struct nsm_dcb_application_priority *) avl_node_info)
       || ! (cli = (struct cli *) arg1))
     NSM_DCB_FN_EXIT (NSM_DCB_API_SET_ERR_INVALID_VALUE);

  pal_sprintf (proto_id_str, "");
  ether_str = NULL;

  switch (app_pri->sel)
    {
      case 0:
        proto_str = "Ethertype";
        eth_value = (u_int16_t) app_pri->proto_id;
        ether_str = nsm_classifier_proto_str (eth_value);
        break;
      case 1:
        proto_str = "tcp";
        sp = (struct pal_servent *) 
             pal_getservbyport (pal_hton16 (app_pri->proto_id), "tcp");
        if (sp)
          { 
            len = pal_strlen (sp->s_name);
            pal_strncpy (proto_id_str, sp->s_name, len+1);
          }
        else 
         pal_strncpy (proto_id_str,"\0", 1);   
        break;
      case 2:
        proto_str = "udp";
        sp = (struct pal_servent *) 
              pal_getservbyport (pal_hton16 (app_pri->proto_id), "udp");
        if (sp)
          {
            len = pal_strlen (sp->s_name);
            pal_strncpy (proto_id_str, sp->s_name, len+1);
          }
        else 
          pal_strncpy (proto_id_str, "\0", 1);
        break;
      case 3:
        proto_str = "both-tcp-udp";
        sp = (struct pal_servent *) 
              pal_getservbyport (pal_hton16 (app_pri->proto_id), "tcp");
        if (! sp)
           sp = (struct pal_servent *) 
                pal_getservbyport (pal_hton16 (app_pri->proto_id), "udp");
        if (sp)
          {
            len = pal_strlen (sp->s_name);
            pal_strncpy (proto_id_str, sp->s_name, len+1);
          }
        else
          pal_strncpy (proto_id_str, "\0", 1);         
        break;
      default:
        zlog_err (nzg, "Invalid value %d\n", app_pri->sel);
        return (NSM_DCB_API_SET_ERR_INVALID_VALUE);
    }
 
  cli_out (cli, "%-20s", proto_str);
  cli_out (cli, "%-20s", (app_pri->sel == 0 && ether_str) ? ether_str :
                                                            proto_id_str);
  cli_out (cli, "%d\n", app_pri->priority);
 
  NSM_DCB_FN_EXIT (NSM_DCB_API_SET_SUCCESS);
}

/**@brief This function displays the application priority table on the
          interface.
   @param Cli - Pointer to CLI 
   @param ifname - Name of interface 
   @return NSM_DCB_API_SET_SUCCESS
*/
s_int32_t 
nsm_dcb_show_appl_priority_table (struct cli *cli, char *ifname)
{ 
  struct nsm_master *nm;
  struct avl_node *node;
  struct nsm_dcb_bridge *dcbg;
  struct interface *ifp;
  struct nsm_dcb_if *dcbif = NULL, tmp_dcbif;
  struct nsm_bridge_master *master;
  struct nsm_bridge *bridge; 
  s_int32_t ret;

  NSM_DCB_FN_ENTER ();

  /* Look for NSM master */
  nm = nsm_master_lookup_by_id (nzg, cli->vr->id);
  if (! nm)
    {
      zlog_err (nzg, "NSM Master not found\n");
      NSM_DCB_FN_EXIT (NSM_DCB_API_SET_ERR_NO_NM);
    }

  ifp = if_lookup_by_name (&nm->vr->ifm, ifname);
  if (! ifp)
    {
      zlog_err (nzg, "Interface %s is not present\n", ifname);
      NSM_DCB_FN_EXIT (NSM_DCB_API_SET_ERR_INTERFACE);
    }

  /* look for the bridge master */
  master = nsm_bridge_get_master (nm);
  if (! master)
    NSM_DCB_FN_EXIT (NSM_BRIDGE_ERR_NOTFOUND);

  bridge = nsm_lookup_bridge_by_name (master, ifp->bridge_name);
  if (! bridge)
    NSM_DCB_FN_EXIT (NSM_BRIDGE_ERR_NOTFOUND);

  /* Look for the dcb bridge structure */
  dcbg = bridge->dcbg;
  if (! dcbg)
    {
       zlog_err (nzg, "Global DCB structure not found\n");
       NSM_DCB_FN_EXIT (NSM_DCB_API_SET_ERR_NO_DCBG);
    }

  if (! CHECK_FLAG (dcbg->dcb_global_flags, NSM_DCB_ENABLE))
    NSM_DCB_FN_EXIT (NSM_DCB_API_SET_ERR_NO_DCB);


  tmp_dcbif.ifp = ifp;
  
  /* Find the DCB interface */
  node = avl_search (dcbg->dcb_if_list, (void_t *) &tmp_dcbif);
  if (node)
    dcbif = (struct nsm_dcb_if *) node->info;

  if (! dcbif)
    NSM_DCB_FN_EXIT (NSM_DCB_API_SET_ERR_DCB_INTERFACE);

  if (! CHECK_FLAG (dcbif->dcb_if_flags, NSM_DCB_IF_ENABLE))
    NSM_DCB_FN_EXIT (NSM_DCB_API_SET_ERR_INTERFACE_NO_DCB);

  if (avl_get_tree_size (dcbif->dcb_appl_pri) > 0)
    {
      cli_out (cli, "Interface : %s\n",ifp->name);
      cli_out (cli, "%-20s", "Protocol");
      cli_out (cli, "%-20s", "Protocol-Value");
      cli_out (cli, "%s\n", "Priority");
      cli_out (cli, "------------------------------------------------------"
                     "------------\n");
  
      ret = avl_traverse (dcbif->dcb_appl_pri, 
                          nsm_dcb_avl_traverse_show_appl_pri_tree, 
                          (void_t *) cli);
   
      if (ret < 0)
        NSM_DCB_FN_EXIT (ret);
    }
  else
    NSM_DCB_FN_EXIT (NSM_DCB_API_SET_ERR_DISPLAY_NO_APPL_PRI);

  NSM_DCB_FN_EXIT (NSM_DCB_API_SET_SUCCESS);
}

/**@brief This function displays the Statistics of PFC requests sent
   @param cli - Pointer to CLI
   @param br_name - Bridge name
   @param if_name - interface name
   @return NSM_DCB_API_SET_SUCCESS
*/
s_int32_t
nsm_dcb_show_pfc_request_stats_by_interface (struct cli* cli, char *ifname)
{
  struct nsm_master *nm;
  struct nsm_bridge_port *br_port;
  struct nsm_bridge *bridge;
  struct nsm_if *zif;
  struct avl_node *node;
  struct nsm_dcb_bridge *dcbg;
  struct interface *ifp;
  struct nsm_dcb_if *dcbif = NULL, tmp_dcbif;

  NSM_DCB_FN_ENTER ();

  /* Look for NSM master */
  nm = nsm_master_lookup_by_id (nzg, cli->vr->id);
  if (! nm)
    {
      zlog_err (nzg, "NSM Master not found\n");
      NSM_DCB_FN_EXIT (NSM_DCB_API_SET_ERR_NO_NM);
    }

  ifp = if_lookup_by_name (&nm->vr->ifm, ifname);

  if (! ifp)
    {
      zlog_err (nzg, "Interface %s is not present\n", ifname);
      NSM_DCB_FN_EXIT (NSM_DCB_API_SET_ERR_INTERFACE);
    }

  /* Get the bridge information */
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
    {
       zlog_err (nzg, "Global DCB structure not found\n");
       NSM_DCB_FN_EXIT (NSM_DCB_API_SET_ERR_NO_DCBG);
    }

  if (! CHECK_FLAG (dcbg->dcb_global_flags, NSM_DCB_ENABLE))
    NSM_DCB_FN_EXIT (NSM_DCB_API_SET_ERR_NO_DCB);

  tmp_dcbif.ifp = ifp;

  /* Find the DCB interface */
  node = avl_search (dcbg->dcb_if_list, (void *) &tmp_dcbif);
  if (node)
    dcbif = (struct nsm_dcb_if *) node->info;

  if (! dcbif)
    NSM_DCB_FN_EXIT (NSM_DCB_API_SET_ERR_DCB_INTERFACE);

  /* Check DCB is enabled on interface */
  if (! CHECK_FLAG (dcbif->dcb_if_flags, NSM_DCB_IF_ENABLE))
    NSM_DCB_FN_EXIT (NSM_DCB_API_SET_ERR_INTERFACE_NO_DCB);

  /* Check PFC is enabled on interface */
  if (! CHECK_FLAG (dcbif->dcb_if_flags, NSM_DCB_IF_PFC_ENABLE))
    NSM_DCB_FN_EXIT (NSM_DCB_API_SET_ERR_INTERFACE_NO_PFC);

  cli_out (cli, "Interface : %s\n", ifp->name);
  cli_out (cli, " PFC : %s\n", "Enabled");
  cli_out (cli, " Requests Sent : %d\n", dcbif->pfc_requests_sent);

  NSM_DCB_FN_EXIT (NSM_DCB_API_SET_SUCCESS);
}/*nsm_dcb_show_pfc_request_stats_by_interface (struct cli* cli, char *ifname)*/

/**@brief This function traverses the avl tree of the dcb interfaces and
          displays PFC requests received info.
*/
s_int32_t
nsm_dcb_pfc_avl_trav_req_show_bridge (void *avl_node_info, void *arg1)
{
   struct nsm_dcb_if *dcbif;
   struct cli *cli;

   NSM_DCB_FN_ENTER ();

   if (! (dcbif = (struct nsm_dcb_if *) avl_node_info)
       || ! (cli = (struct cli *) arg1))
     NSM_DCB_FN_EXIT (NSM_DCB_API_SET_ERR_INVALID_VALUE);

   /* Check DCB is enabled on interface
      Ignore if disabled as the CLI displays information on the bridge
      Hence if disabled on one interface of bridge jump to next interface. */
   if (! CHECK_FLAG (dcbif->dcb_if_flags, NSM_DCB_IF_ENABLE))
     {
       zlog_err (nzg, "DCB is disabled on the interface %s\n",
                 dcbif->ifp->name);
       NSM_DCB_FN_EXIT (NSM_DCB_API_SET_SUCCESS);
     }
  cli_out (cli, "%-20s", dcbif->ifp->name);
  cli_out (cli, "%-20s",
           (CHECK_FLAG (dcbif->dcb_if_flags, NSM_DCB_IF_PFC_ENABLE)) ?
               "Enable":"Disable");
  cli_out (cli, " %d\n", dcbif->pfc_requests_sent);

  NSM_DCB_FN_EXIT (NSM_DCB_API_SET_SUCCESS);
}/* nsm_dcb_pfc_avl_trav_req_show_bridge (void *avl_node_info, void *arg1) */

/**@brief This function displays PFC Requests sent on each interface of bridge.
   @param cli - pointer to CLI
   @param bridge_name - Name of bridge
   @return NSM_DCB_API_SET_SUCCESS
*/
s_int32_t
nsm_dcb_show_pfc_request_stats_by_bridge (struct cli *cli, char *bridge_name)
{
  struct nsm_master *nm;
  struct nsm_bridge_master *master;
  struct nsm_bridge *bridge;
  struct nsm_dcb_bridge *dcbg;
  s_int32_t ret;

  NSM_DCB_FN_ENTER ();

  /* Look for NSM master */
  nm = nsm_master_lookup_by_id (nzg, cli->vr->id);
  if (! nm)
    NSM_DCB_FN_EXIT (NSM_DCB_API_SET_ERR_NO_NM);

  /* look for the bridge master */
  master = nsm_bridge_get_master (nm);
  if (! master)
    NSM_DCB_FN_EXIT (NSM_BRIDGE_ERR_NOTFOUND);

  if (bridge_name == NULL)
#ifdef HAVE_DEFAULT_BRIDGE
    bridge = nsm_get_default_bridge (master);
#else
    NSM_DCB_FN_EXIT (NSM_BRIDGE_ERR_NOTFOUND);
#endif /* HAVE_DEFAULT_BRIDGE */
  else
    bridge = nsm_lookup_bridge_by_name (master, bridge_name);

  if (! bridge)
    NSM_DCB_FN_EXIT (NSM_BRIDGE_ERR_NOTFOUND);

  /* Look for the dcb bridge structure */
  dcbg = bridge->dcbg;
  if (! dcbg)
    NSM_DCB_FN_EXIT (NSM_DCB_API_SET_ERR_NO_DCBG);

  if (! CHECK_FLAG (dcbg->dcb_global_flags, NSM_DCB_ENABLE))
    NSM_DCB_FN_EXIT (NSM_DCB_API_SET_ERR_NO_DCB);

  if (! CHECK_FLAG (dcbg->dcb_global_flags, NSM_DCB_PFC_ENABLE))
    NSM_DCB_FN_EXIT (NSM_DCB_API_SET_ERR_NO_PFC);

  cli_out (cli, "bridge : %s\n", bridge->name);

  cli_out (cli, "%-20s","Interface" );
  cli_out (cli, "%-20s", "PFC State");
  cli_out (cli, "%s\n", "Requests Sent");

  cli_out (cli, "----------------------------------------------------------\n");

  /* Traverse the list */
  ret = avl_traverse (dcbg->dcb_if_list, nsm_dcb_pfc_avl_trav_req_show_bridge,
                     (void_t *)cli);

  if (ret < 0)
    NSM_DCB_FN_EXIT (ret);
  NSM_DCB_FN_EXIT (NSM_DCB_API_SET_SUCCESS);
}/* nsm_dcb_show_pfc_request_stats_by_bridge (struct cli *cli, char *bridge)*/


/**@brief This function displays the Statistics of PFC indications received
   @param cli - Pointer to CLI
   @param br_name - Bridge name
   @param if_name - interface name
   @return NSM_DCB_API_SET_SUCCESS
*/
s_int32_t
nsm_dcb_show_pfc_indication_stats_by_interface (struct cli* cli, char *ifname)
{
  struct nsm_master *nm;
  struct nsm_bridge_port *br_port;
  struct nsm_bridge *bridge;
  struct nsm_if *zif;
  struct avl_node *node;
  struct nsm_dcb_bridge *dcbg;
  struct interface *ifp;
  struct nsm_dcb_if *dcbif = NULL, tmp_dcbif;

  NSM_DCB_FN_ENTER ();

  /* Look for NSM master */
  nm = nsm_master_lookup_by_id (nzg, cli->vr->id);
  if (! nm)
    NSM_DCB_FN_EXIT (NSM_DCB_API_SET_ERR_NO_NM);

  ifp = if_lookup_by_name (&nm->vr->ifm, ifname);

  if (! ifp)
    NSM_DCB_FN_EXIT (NSM_DCB_API_SET_ERR_INTERFACE);

  /* Get the bridge information */
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

  if (! CHECK_FLAG (dcbg->dcb_global_flags, NSM_DCB_ENABLE))
    NSM_DCB_FN_EXIT (NSM_DCB_API_SET_ERR_NO_DCB);

  tmp_dcbif.ifp = ifp;

  /* Find the DCB interface */
  node = avl_search (dcbg->dcb_if_list, (void *) &tmp_dcbif);
  if (node)
    dcbif = (struct nsm_dcb_if *) node->info;

  if (! dcbif)
    NSM_DCB_FN_EXIT (NSM_DCB_API_SET_ERR_DCB_INTERFACE);

  /* Check DCB is enabled on interface */
  if (! CHECK_FLAG (dcbif->dcb_if_flags, NSM_DCB_IF_ENABLE))
    NSM_DCB_FN_EXIT (NSM_DCB_API_SET_ERR_INTERFACE_NO_DCB);

  /* Check PFC is enabled on interface */
  if (! CHECK_FLAG (dcbif->dcb_if_flags, NSM_DCB_IF_PFC_ENABLE))
    NSM_DCB_FN_EXIT (NSM_DCB_API_SET_ERR_INTERFACE_NO_PFC);

  cli_out (cli, "Interface : %s\n", ifp->name);
  cli_out (cli, " PFC : %s\n", "Enabled");
  cli_out (cli, " Indications received : %d\n", dcbif->pfc_requests_sent);

  NSM_DCB_FN_EXIT (NSM_DCB_API_SET_SUCCESS);
}/*nsm_dcb_show_pfc_indication_stats_by_interface (struct cli* cli, char *if)*/

/**@brief This function traverses the avl tree of the dcb interfaces and
          displays PFC requests received info.
*/
s_int32_t
nsm_dcb_pfc_avl_trav_indi_show_bridge (void *avl_node_info, void *arg1)
{
   struct nsm_dcb_if *dcbif;
   struct cli *cli;

   NSM_DCB_FN_ENTER ();

   if (! (dcbif = (struct nsm_dcb_if *) avl_node_info)
       || ! (cli = (struct cli *) arg1))
     NSM_DCB_FN_EXIT (NSM_DCB_API_SET_ERR_INVALID_VALUE);

   /* Check DCB is enabled on interface
      Ignore if disabled as the CLI displays information on the bridge
      Hence if disabled on one interface of bridge jump to next interface. */
   if (! CHECK_FLAG (dcbif->dcb_if_flags, NSM_DCB_IF_ENABLE))
     {
       zlog_err (nzg, "DCB is disabled on the interface %s\n",
                 dcbif->ifp->name);
       NSM_DCB_FN_EXIT (NSM_DCB_API_SET_SUCCESS);
     }
  cli_out (cli, "%-20s", dcbif->ifp->name);
  cli_out (cli, "%-20s",
           (CHECK_FLAG (dcbif->dcb_if_flags, NSM_DCB_IF_PFC_ENABLE)) ?
               "Enable":"Disable");
  cli_out (cli, "%d\n", dcbif->pfc_indications_rcvd);

  NSM_DCB_FN_EXIT (NSM_DCB_API_SET_SUCCESS);
}/* nsm_dcb_pfc_avl_trav_indi_show_bridge (void *avl_node_info, void *arg1) */

/**@brief This function displays PFC indications received
          on each interface of bridge.
   @param cli - pointer to CLI
   @param bridge_name - Name of bridge
   @return NSM_DCB_API_SET_SUCCESS
*/
s_int32_t
nsm_dcb_show_pfc_indication_stats_by_bridge (struct cli *cli, char *bridge_name)
{
  struct nsm_master *nm;
  struct nsm_bridge_master *master;
  struct nsm_bridge *bridge;
  struct nsm_dcb_bridge *dcbg;
  s_int32_t ret;

  NSM_DCB_FN_ENTER ();

  /* Look for NSM master */
  nm = nsm_master_lookup_by_id (nzg, cli->vr->id);
  if (! nm)
    NSM_DCB_FN_EXIT (NSM_DCB_API_SET_ERR_NO_NM);

  /* look for the bridge master */
  master = nsm_bridge_get_master (nm);
  if (! master)
    NSM_DCB_FN_EXIT (NSM_BRIDGE_ERR_NOTFOUND);

  if (bridge_name == NULL)
#ifdef HAVE_DEFAULT_BRIDGE
    bridge = nsm_get_default_bridge (master);
#else
    NSM_DCB_FN_EXIT (NSM_BRIDGE_ERR_NOTFOUND);
#endif /* HAVE_DEFAULT_BRIDGE */
  else
    bridge = nsm_lookup_bridge_by_name (master, bridge_name);

  if (! bridge)
    NSM_DCB_FN_EXIT (NSM_BRIDGE_ERR_NOTFOUND);

  /* Look for the dcb bridge structure */
  dcbg = bridge->dcbg;
  if (! dcbg)
    NSM_DCB_FN_EXIT (NSM_DCB_API_SET_ERR_NO_DCBG);

  if (! CHECK_FLAG (dcbg->dcb_global_flags, NSM_DCB_ENABLE))
    NSM_DCB_FN_EXIT (NSM_DCB_API_SET_ERR_NO_DCB);

  if (! CHECK_FLAG (dcbg->dcb_global_flags, NSM_DCB_PFC_ENABLE))
    NSM_DCB_FN_EXIT (NSM_DCB_API_SET_ERR_NO_PFC);

  cli_out (cli, "bridge : %s\n", bridge->name);

  cli_out (cli, "%-20s","Interface" );
  cli_out (cli, "%-20s", "PFC State");
  cli_out (cli, "%s\n", "Indications received");

  cli_out (cli, "----------------------------------------------------------\n");

  /* Traverse the list */
  ret = avl_traverse (dcbg->dcb_if_list, nsm_dcb_pfc_avl_trav_indi_show_bridge,
                     (void_t *)cli);

  if (ret < 0)
    NSM_DCB_FN_EXIT (ret);

  NSM_DCB_FN_EXIT (NSM_DCB_API_SET_SUCCESS);
}/* nsm_dcb_show_pfc_indication_stats_by_bridge (struct cli *cli, char *brid)*/

s_int16_t
nsm_dcb_pfc_vrep_show_cb (intptr_t usr_ref, char *str)
{
  struct cli *cli = (struct cli *)usr_ref;

  if (! cli)
    return VREP_ERROR;

  cli_out (cli, "%s", str);
  return VREP_SUCCESS;
}

/**@brief This function displays details of PFC of an interface
   @param cli - Pointer to CLI
   @param if_name - interface name
   @return NSM_DCB_API_SET_SUCCESS
*/
s_int32_t
nsm_dcb_show_pfc_details_by_interface (struct cli* cli, struct vrep* vrep, 
                                        char *ifname)
{
  struct nsm_master *nm;
  struct nsm_bridge_port *br_port;
  struct nsm_bridge *bridge;
  struct nsm_if *zif;
  struct avl_node *node;
  struct nsm_dcb_bridge *dcbg;
  struct interface *ifp;
  struct nsm_dcb_if *dcbif = NULL, tmp_dcbif;
  u_int8_t priority;
  char str [17];
  char *ptr = NULL;

  NSM_DCB_FN_ENTER ();

  /* Look for NSM master */
  nm = nsm_master_lookup_by_id (nzg, cli->vr->id);
  if (! nm)
    NSM_DCB_FN_EXIT (NSM_DCB_API_SET_ERR_NO_NM);

  ifp = if_lookup_by_name (&nm->vr->ifm, ifname);

  if (! ifp)
    NSM_DCB_FN_EXIT (NSM_DCB_API_SET_ERR_INTERFACE);

  /* Get the bridge information */
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

  if (! CHECK_FLAG (dcbg->dcb_global_flags, NSM_DCB_ENABLE))
    NSM_DCB_FN_EXIT (NSM_DCB_API_SET_ERR_NO_DCB);

  tmp_dcbif.ifp = ifp;

  /* Find the DCB interface */
  node = avl_search (dcbg->dcb_if_list, (void *) &tmp_dcbif);
  if (node)
    dcbif = (struct nsm_dcb_if *) node->info;

  if (! dcbif)
    NSM_DCB_FN_EXIT (NSM_DCB_API_SET_ERR_DCB_INTERFACE);

  /* Check DCB is enabled on interface */
  if (! CHECK_FLAG (dcbif->dcb_if_flags, NSM_DCB_IF_ENABLE))
    NSM_DCB_FN_EXIT (NSM_DCB_API_SET_ERR_INTERFACE_NO_DCB);

  /* Check PFC is enabled on interface */
  if (! CHECK_FLAG (dcbif->dcb_if_flags, NSM_DCB_IF_PFC_ENABLE))
    NSM_DCB_FN_EXIT (NSM_DCB_API_SET_ERR_INTERFACE_NO_PFC);
 
  ptr = str;
  for (priority = 0; priority < NSM_DCB_NUM_USER_PRI; priority++)
     {
       if (dcbif->pfc_enable_vector[priority])
         {
            pal_sprintf (ptr, "%d ", priority);
            ptr+=2;
         }
     }

  vrep_add_next_row (vrep, NULL, " %s \t %s \t %d \t %d \t %d \t %d \t %s ",
                     ifp->name, dcbif->pfc_mode ? "auto" : "on", dcbif->pfc_cap,
                     dcbif->pfc_link_delay_allowance, dcbif->pfc_requests_sent, 
                     dcbif->pfc_indications_rcvd, str);


  NSM_DCB_FN_EXIT (NSM_DCB_API_SET_SUCCESS);
}/*nsm_dcb_show_pfc_details_by_interface (struct cli* cli, char *ifname)*/


/**@brief This function traverses the avl tree of the dcb interfaces and
          displays PFC requests received info.
*/
s_int32_t
nsm_dcb_pfc_avl_trav_detail_show_bridge (void_t *avl_node_info, void_t *arg1, 
                                         void_t *arg2)
{
   struct nsm_dcb_if *dcbif;
   struct cli *cli;
   struct vrep *vrep;
   
   NSM_DCB_FN_ENTER ();

   if (! (dcbif = (struct nsm_dcb_if *) avl_node_info)
       || ! (cli = (struct cli *) arg1) || ! (vrep = (struct vrep *) arg2))
     NSM_DCB_FN_EXIT (NSM_DCB_API_SET_ERR_INVALID_VALUE);

   /* Check DCB is enabled on interface
      Ignore if disabled as the CLI displays information on the bridge
      Hence if disabled on one interface of bridge jump to next interface. */
   if (! CHECK_FLAG (dcbif->dcb_if_flags, NSM_DCB_IF_ENABLE))
     {
       zlog_err (nzg, "DCB is disabled on the interface %s\n",
                 dcbif->ifp->name);
       NSM_DCB_FN_EXIT (NSM_DCB_API_SET_SUCCESS);
     }

  nsm_dcb_show_pfc_details_by_interface (cli , vrep, dcbif->ifp->name);

  NSM_DCB_FN_EXIT (NSM_DCB_API_SET_SUCCESS);
}/* nsm_dcb_pfc_avl_trav_detail_show_bridge (void *avl_node_info, void *arg1) */


/**@brief This function displays PFC Requests sent on each interface of bridge.
   @param cli - pointer to CLI
   @param bridge_name - Name of bridge
   @return NSM_DCB_API_SET_SUCCESS
*/
s_int32_t
nsm_dcb_show_pfc_details_by_bridge (struct cli *cli, struct vrep *vrep, 
                                     char *bridge_name)
{
  struct nsm_master *nm;
  struct nsm_bridge_master *master;
  struct nsm_bridge *bridge;
  struct nsm_dcb_bridge *dcbg;
  s_int32_t ret;

  NSM_DCB_FN_ENTER ();

  /* Look for NSM master */
  nm = nsm_master_lookup_by_id (nzg, cli->vr->id);
  if (! nm)
    {
      zlog_err (nzg, "NSM Master not found\n");
      NSM_DCB_FN_EXIT (NSM_DCB_API_SET_ERR_NO_NM);
    }

  /* look for the bridge master */
  master = nsm_bridge_get_master (nm);
  if (! master)
    NSM_DCB_FN_EXIT (NSM_BRIDGE_ERR_NOTFOUND);

  if (bridge_name == NULL)
#ifdef HAVE_DEFAULT_BRIDGE
    bridge = nsm_get_default_bridge (master);
#else
    NSM_DCB_FN_EXIT (NSM_BRIDGE_ERR_NOTFOUND);
#endif /* HAVE_DEFAULT_BRIDGE */
  else
    bridge = nsm_lookup_bridge_by_name (master, bridge_name);

  if (! bridge)
    NSM_DCB_FN_EXIT (NSM_BRIDGE_ERR_NOTFOUND);

  /* Look for the dcb bridge structure */
  dcbg = bridge->dcbg;
  if (! dcbg)
    NSM_DCB_FN_EXIT (NSM_DCB_API_SET_ERR_NO_DCBG);

  if (! CHECK_FLAG (dcbg->dcb_global_flags, NSM_DCB_ENABLE))
    NSM_DCB_FN_EXIT (NSM_DCB_API_SET_ERR_NO_DCB);

  if (! CHECK_FLAG (dcbg->dcb_global_flags, NSM_DCB_PFC_ENABLE))
    NSM_DCB_FN_EXIT (NSM_DCB_API_SET_ERR_NO_PFC);

  /* Traverse the list */
  ret = avl_traverse2 (dcbg->dcb_if_list,
                     nsm_dcb_pfc_avl_trav_detail_show_bridge, (void_t *)cli, 
                     (void_t *)vrep);

  if (ret < 0)
    NSM_DCB_FN_EXIT (ret);

  NSM_DCB_FN_EXIT (NSM_DCB_API_SET_SUCCESS);
}/* nsm_dcb_show_pfc_details_by_bridge (struct cli *cli, char *bridge)*/

/**@brief This CLI displays the traffic class group information per tcgid
   @return CLI_SUCCESS
*/
CLI (dcb_ets_show_tcgs_tcgid,
     dcb_ets_show_tcgs_tcgid_cmd,
     "show traffic-class-groups <0-15> bridge <1-32>" ,
     CLI_SHOW_STR,
     NSM_TCG_STR,
     NSM_TCGID_STR,
     BRIDGE_STR,
     BRIDGE_NAME_STR)
{
   u_int8_t tcgid;
   u_int8_t ret = NSM_DCB_API_SET_SUCCESS;
   char br_name [NSM_BRIDGE_NAMSIZ+1];

   tcgid = pal_atoi (argv[0]);

   if (tcgid > NSM_DCB_ETS_MAX_TCGID && tcgid < NSM_DCB_ETS_DEFAULT_TCGID)
     return nsm_dcb_cli_return (cli, NSM_DCB_API_SET_ERR_RESV_VALUE);

   if (argc > 1)
     {
      pal_strncpy (br_name, argv[1], NSM_BRIDGE_NAMSIZ+1);
      ret = nsm_dcb_show_tcg_by_id(cli, tcgid, br_name);
     }
   else
#ifdef HAVE_DEFAULT_BRIDGE
     ret = nsm_dcb_show_tcg_by_id (cli, tcgid, NULL);
#endif /* HAVE_DEFAULT_BRIDGE */

   return nsm_dcb_cli_return (cli, ret);
}


#ifdef HAVE_DEFAULT_BRIDGE
ALI (dcb_ets_show_tcgs_tcgid,
     dcb_ets_show_tcgs_tcgid_default_cmd,
     "show traffic-class-groups <0-15>" ,
     CLI_SHOW_STR,
     NSM_TCG_STR,
     NSM_TCGID_STR);
#endif /* HAVE_DEFAULT_BRIDGE */



/**@brief This CLI displays the traffic class group information per interface
   @return CLI_SUCCESS
*/
CLI (dcb_ets_show_tcgs_intf,
     dcb_ets_show_tcgs_intf_cmd,
     "show traffic-class-groups interface IFNAME",
     CLI_SHOW_STR,
     NSM_TCG_STR,
     "Traffic-class groups information on interface",
     CLI_IFNAME_STR)
{
   s_int32_t ret = NSM_DCB_API_SET_SUCCESS;
   struct interface *ifp;

  ifp = if_lookup_by_name (&cli->vr->ifm, argv[0]);
 
  if (! ifp)
    return nsm_dcb_cli_return (cli, NSM_DCB_API_SET_ERR_INTERFACE);

  if (! NSM_INTF_TYPE_L2 (ifp))
    return nsm_dcb_cli_return (cli, NSM_DCB_API_SET_ERR_L2_INTERFACE);

  ret = nsm_dcb_show_tcg_by_intf (cli, ifp->name);

  return nsm_dcb_cli_return (cli, ret); 
}


/**@brief This CLI displays the traffic class group information per bridge
   @return CLI_SUCCESS
*/
CLI (dcb_ets_show_tcgs_bridge,
     dcb_ets_show_tcgs_bridge_cmd,
     "show traffic-class-groups bridge <1-32>",
     CLI_SHOW_STR,
     NSM_TCG_STR,
     BRIDGE_STR,
     BRIDGE_NAME_STR)
{
  s_int32_t ret = NSM_DCB_API_SET_SUCCESS;
  char br_name [NSM_BRIDGE_NAMSIZ+1];
 
  if (argc > 0)
    {
     pal_strncpy (br_name, argv[0], NSM_BRIDGE_NAMSIZ+1);
     ret = nsm_dcb_show_tcg_by_bridge (cli, br_name);
    }
  else 
#ifdef HAVE_DEFAULT_BRIDGE
     ret = nsm_dcb_show_tcg_by_bridge (cli, NULL);
#endif /* HAVE_DEFAULT_BRIDGE */
  
  return nsm_dcb_cli_return (cli, ret); 
  
}


#ifdef HAVE_DEFAULT_BRIDGE
ALI (dcb_ets_show_tcgs_bridge,
     dcb_ets_show_tcgs_default_bridge_cmd,
     "show traffic-class-groups",
     CLI_SHOW_STR,
     NSM_TCG_STR);
#endif /* HAVE_DEFAULT_BRIDGE */

/**@brief This CLI displays application priority table per interface
   @return CLI_SUCCESS
*/
CLI (dcb_show_appl_priority,
     dcb_show_appl_priority_cmd,
     "show application-priority interface IFNAME",
     CLI_SHOW_STR,
     "Application Priority",
     "Application-Priority interface information",
     CLI_IFNAME_STR)
{
  s_int32_t ret = NSM_DCB_API_SET_SUCCESS;
  struct interface *ifp;

  ifp = if_lookup_by_name (&cli->vr->ifm, argv[0]);

  if (! ifp)
    return nsm_dcb_cli_return (cli, NSM_DCB_API_SET_ERR_INTERFACE);

  if (! NSM_INTF_TYPE_L2 (ifp))
    return nsm_dcb_cli_return (cli, NSM_DCB_API_SET_ERR_L2_INTERFACE);

  ret = nsm_dcb_show_appl_priority_table (cli, ifp->name);

  return nsm_dcb_cli_return (cli, ret);
}

/**@brief This CLI displays number of PFC requests sent on that interface
   @return CLI_SUCCESS
*/
CLI (dcb_show_pfc_req_sent,
     dcb_show_pfc_req_sent_cmd,
     "show priority-flow-control requests-sent ((interface IFNAME)| "
     "(bridge <1-32>))" ,
     CLI_SHOW_STR,
     NSM_PFC_STR,
     "number of PFC requests sent",
     "on interface",
     CLI_IFNAME_STR,
     BRIDGE_STR,
     BRIDGE_NAME_STR)
{
   u_int8_t ret = NSM_DCB_API_SET_SUCCESS;
   char br_name [NSM_BRIDGE_NAMSIZ+1];
   struct interface *ifp = NULL;

   if (argc == 2)
     {
       /*In this case we can have either interface or bridge */
       if (argv[0][0] == 'i')
         {
           /* Interface */
           ifp = if_lookup_by_name (&cli->vr->ifm, argv[1]);

           if (! ifp)
             return nsm_dcb_cli_return (cli, NSM_DCB_API_SET_ERR_INTERFACE);

           if (! NSM_INTF_TYPE_L2 (ifp))
             return nsm_dcb_cli_return (cli, NSM_DCB_API_SET_ERR_L2_INTERFACE);

           ret = nsm_dcb_show_pfc_request_stats_by_interface (cli, ifp->name);
         }
       else
         {
           /* Bridge */
           pal_strncpy (br_name, argv[1], NSM_BRIDGE_NAMSIZ+1);
           ret = nsm_dcb_show_pfc_request_stats_by_bridge (cli, br_name);
         }
     }
   else if (argc == 0)
     {
#ifdef HAVE_DEFAULT_BRIDGE
        /* Default  bridge */
           ret = nsm_dcb_show_pfc_request_stats_by_bridge (cli, NULL);
#endif /* HAVE_DEFAULT_BRIDGE */
     }

   return nsm_dcb_cli_return (cli, ret);
}/* CLI (dcb_show_pfc_req_sent,*/

#ifdef HAVE_DEFAULT_BRIDGE
ALI (dcb_show_pfc_req_sent,
     dcb_show_pfc_req_sent_default_cmd,
     "show priority-flow-control requests-sent" ,
     CLI_SHOW_STR,
     NSM_PFC_STR,
     "number of PFC requests sent for default bridge");
#endif /*HAVE_DEFAULT_BRIDGE*/

/**@brief This CLI displays number of PFC indications received on that interface
   @return CLI_SUCCESS
*/
CLI (dcb_show_pfc_indi_rcvd,
     dcb_show_pfc_indi_rcvd_cmd,
     "show priority-flow-control indications-received ((interface IFNAME)| "
     "(bridge <1-32>))" ,
     CLI_SHOW_STR,
     NSM_PFC_STR,
     "number of PFC requests sent",
     "on interface",
     CLI_IFNAME_STR,
     BRIDGE_STR,
     BRIDGE_NAME_STR)
{
   u_int8_t ret = NSM_DCB_API_SET_SUCCESS;
   char br_name [NSM_BRIDGE_NAMSIZ+1];
   struct interface *ifp = NULL;

   if (argc == 2)
     {
       /*In this case we can have either interface or bridge */
       if (argv[0][0] == 'i')
         {
           /* Interface */
           ifp = if_lookup_by_name (&cli->vr->ifm, argv[1]);

           if (! ifp)
             return nsm_dcb_cli_return (cli, NSM_DCB_API_SET_ERR_INTERFACE);

           if (! NSM_INTF_TYPE_L2 (ifp))
             return nsm_dcb_cli_return (cli, NSM_DCB_API_SET_ERR_L2_INTERFACE);

           ret = nsm_dcb_show_pfc_indication_stats_by_interface (cli,ifp->name);
         }
       else
         {
           /* Bridge */
           pal_strncpy (br_name, argv[1], NSM_BRIDGE_NAMSIZ+1);
           ret = nsm_dcb_show_pfc_indication_stats_by_bridge (cli, br_name);
         }
     }
   else if (argc == 0)
     {
#ifdef HAVE_DEFAULT_BRIDGE
        /* Default  bridge */
           ret = nsm_dcb_show_pfc_indication_stats_by_bridge (cli, NULL);
#endif /* HAVE_DEFAULT_BRIDGE */
     }

   return nsm_dcb_cli_return (cli, ret);
}/* CLI (dcb_show_pfc_indi_rcvd, */

#ifdef HAVE_DEFAULT_BRIDGE
ALI (dcb_show_pfc_indi_rcvd,
     dcb_show_pfc_indi_rcvd_default_cmd,
     "show priority-flow-control indications-received" ,
     CLI_SHOW_STR,
     NSM_PFC_STR,
     "number of PFC requests sent");
#endif /* HAVE_DEFAULT_BRIDGE */

/**@brief This CLI displays details of PFC configurations and status
   @return CLI_SUCCESS
*/
CLI (dcb_show_pfc_details,
     dcb_show_pfc_details_cmd,
     "show priority-flow-control details ((interface IFNAME)| "
     "(bridge <1-32>))" ,
     CLI_SHOW_STR,
     NSM_PFC_STR,
     "details of PFC",
     "on interface",
     CLI_IFNAME_STR,
     BRIDGE_STR,
     BRIDGE_NAME_STR)
{
   u_int8_t ret = NSM_DCB_API_SET_SUCCESS;
   char br_name [NSM_BRIDGE_NAMSIZ+1];
   struct interface *ifp = NULL;
   struct vrep_table *vrep = NULL;

  vrep = vrep_create (7, VREP_MAX_ROW_WIDTH);
  if (vrep == NULL) 
   {
      cli_out(cli, "% Cannot initiate report.");
      return CLI_ERROR;
   }

  vrep_add (vrep, 0, 0, " Interface \t Mode \t Cap \t Link Delay "
                         "\t Requests \t Indications \t Priorities ");
  vrep_add (vrep, 1, 0, "  \t  \t  \t Allowance "
                         "\t Sent \t Receieved \t enabled ");
  vrep_add (vrep, 2, 0, " ========= \t ==== \t === \t =========== \t"
                         " ======== \t =========== \t =========== ");
   if (argc == 2)
     {
       /*In this case we can have either interface or bridge */
       if (argv[0][0] == 'i')
         {
           /* Interface */
           ifp = if_lookup_by_name (&cli->vr->ifm, argv[1]);

           if (! ifp)
             {
               vrep_delete (vrep);
               return nsm_dcb_cli_return (cli, NSM_DCB_API_SET_ERR_INTERFACE);
             }

           if (! NSM_INTF_TYPE_L2 (ifp))
             {
               vrep_delete (vrep);
               return nsm_dcb_cli_return (cli, 
                                          NSM_DCB_API_SET_ERR_L2_INTERFACE);
             }

           ret = nsm_dcb_show_pfc_details_by_interface (cli, vrep, ifp->name);
         }
       else
         {
           /* Bridge */
           pal_strncpy (br_name, argv[1], NSM_BRIDGE_NAMSIZ+1);
           ret = nsm_dcb_show_pfc_details_by_bridge (cli, vrep, br_name);
         }
     }
   else if (argc == 0)
     {
#ifdef HAVE_DEFAULT_BRIDGE
        /* Default  bridge */
           ret = nsm_dcb_show_pfc_details_by_bridge (cli, vrep, NULL);
#endif /* HAVE_DEFAULT_BRIDGE */
     }

  if (ret == NSM_DCB_API_SET_SUCCESS)
    vrep_show (vrep, nsm_dcb_pfc_vrep_show_cb, (intptr_t)cli);
  
  vrep_delete (vrep);

  return nsm_dcb_cli_return (cli, ret);
}/* CLI (dcb_show_pfc_details, */

#ifdef HAVE_DEFAULT_BRIDGE
ALI (dcb_show_pfc_details,
     dcb_show_pfc_details_default_cmd,
     "show priority-flow-control details" ,
     CLI_SHOW_STR,
     NSM_PFC_STR,
     "details of PFC");
#endif /* HAVE_DEFAULT_BRIDGE */

/**@brief This function defines CLI Initialization 
   @param ctree - Pointer to CLI tree
   @return None
*/
void_t
nsm_dcb_show_cli_init (struct cli_tree *ctree)
{
  /* EXEC mode commands */
  cli_install (ctree, EXEC_MODE, &dcb_show_appl_priority_cmd);
  cli_install (ctree, EXEC_MODE, &dcb_ets_show_tcgs_intf_cmd);
  cli_install (ctree, EXEC_MODE, &dcb_ets_show_tcgs_bridge_cmd);
  cli_install (ctree, EXEC_MODE, &dcb_ets_show_tcgs_tcgid_cmd);
  cli_install (ctree, EXEC_MODE, &dcb_show_pfc_req_sent_cmd);
  cli_install (ctree, EXEC_MODE, &dcb_show_pfc_indi_rcvd_cmd);
  cli_install (ctree, EXEC_MODE, &dcb_show_pfc_details_cmd);

#ifdef HAVE_DEFAULT_BRIDGE
  cli_install (ctree, EXEC_MODE, &dcb_ets_show_tcgs_default_bridge_cmd);
  cli_install (ctree, EXEC_MODE, &dcb_ets_show_tcgs_tcgid_default_cmd);
  cli_install (ctree, EXEC_MODE, &dcb_show_pfc_details_default_cmd);
  cli_install (ctree, EXEC_MODE, &dcb_show_pfc_req_sent_default_cmd);
  cli_install (ctree, EXEC_MODE, &dcb_show_pfc_indi_rcvd_default_cmd);
#endif /* HAVE_DEFAULT_BRIDGE */
}

#endif /* HAVE_DCB */
