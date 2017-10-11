/* Copyright (C) 2009  All Rights Reserved. */
/**@file nsm_dcb_api.c
   @brief This nsm_dcb_api.c file defines the NSM APIs related to DCB 
          functionality.
*/  

#include "pal.h"
#include "lib.h"

#include "if.h"
#include "avl_tree.h"

#ifdef HAVE_HAL
#include "hal_incl.h"
#endif /* HAVE_HAL */

#include "nsmd.h"
#include "nsm_interface.h"
#include "nsm_router.h"
#include "nsm_bridge_cli.h"
#include "nsm_fea.h"
#ifdef HAVE_L2
#include "nsm_flowctrl.h"
#endif /* HAVE_L2 */

#ifdef HAVE_DCB
#include "nsm_dcb.h"
#include "nsm_dcb_api.h"


/**@brief This function defines the compare function for avl tree of dcb 
          interfaces.
   @param data1 - dcb interface node information
   @param data2 - dcb interface node information
   @return Comparison Result
*/
s_int32_t 
nsm_dcb_if_cmp (void_t *data1, void_t *data2)
{
  struct nsm_dcb_if *intf1 = (struct nsm_dcb_if *) data1;
  struct nsm_dcb_if *intf2 = (struct nsm_dcb_if *) data2;

  NSM_DCB_FN_ENTER ();

  if (data1 == NULL || data2 == NULL)
    NSM_DCB_FN_EXIT (-1);

  if (intf1->ifp->ifindex < intf2->ifp->ifindex)
    NSM_DCB_FN_EXIT (-1);
  else if (intf1->ifp->ifindex == intf2->ifp->ifindex)
    NSM_DCB_FN_EXIT (0);
  else
    NSM_DCB_FN_EXIT (1);
}

/**@brief This function defines compare function for application priority 
          avl tree.
   @param data1 - application priority node information
   @param data2 - application priority node information
   @return Comparison result 
*/
s_int32_t
nsm_dcb_appl_pri_cmp (void_t *data1, void_t *data2)
{
  struct nsm_dcb_application_priority *app_pri1 = 
                       (struct nsm_dcb_application_priority *) data1;
  struct nsm_dcb_application_priority *app_pri2 = 
                       (struct nsm_dcb_application_priority *) data2;
 
  NSM_DCB_FN_ENTER ();

  if (data1 == NULL || data2 == NULL)
    NSM_DCB_FN_EXIT (-1);

  if (app_pri1->sel < app_pri2->sel)
    NSM_DCB_FN_EXIT (-1);
  else if (app_pri1->sel > app_pri2->sel)
    NSM_DCB_FN_EXIT (1);

  if (app_pri1->proto_id < app_pri2->proto_id)
    NSM_DCB_FN_EXIT (-1);
  else if (app_pri1->proto_id > app_pri2->proto_id)
    NSM_DCB_FN_EXIT (1);

  NSM_DCB_FN_EXIT(0);
}

/**@brief This function initializes nsm_dcb_bridge structure.
   @param bridge - Pointer to nsm_bridge structure
   @return NSM_DCB_API_SET_SUCCESS
*/
s_int32_t 
nsm_dcb_init (struct nsm_bridge *bridge)
{
  struct nsm_dcb_bridge *dcbg;
#ifdef HAVE_HAL
  s_int32_t ret;
#endif /* HAVE_HAL */
 
  NSM_DCB_FN_ENTER ();

  if (bridge->dcbg != NULL)
    {
      zlog_err (nzg, "dcb bridge structure is already created\n");
      NSM_DCB_FN_EXIT(NSM_DCB_API_SET_ERR_DCB_EXISTS); 
    }

#ifdef HAVE_HAL 
   ret = hal_dcb_init (bridge->name);
   if (ret < 0)
     {
       zlog_err (nzg, "Failed initializing HAL DCB\n");
       NSM_DCB_FN_EXIT (NSM_DCB_API_SET_ERR_HW_NO_SUPPORT);
     }
#endif /* HAVE_HAL */

  dcbg = XCALLOC (MTYPE_NSM_BRIDGE_DCB, sizeof (struct nsm_dcb_bridge));
  if (! dcbg)
    {
      zlog_err (nzg, "Can not create dcb bridge structure\n");
      NSM_DCB_FN_EXIT (NSM_DCB_API_SET_ERR_NO_MEM);
    }

  UNSET_FLAG (dcbg->dcb_global_flags, NSM_DCB_ENABLE);
  UNSET_FLAG (dcbg->dcb_global_flags, NSM_DCB_ETS_ENABLE);
  UNSET_FLAG (dcbg->dcb_global_flags, NSM_DCB_PFC_ENABLE);

   /* Create the AVL tree for dcb if list */
  if (avl_create (&dcbg->dcb_if_list, 0, nsm_dcb_if_cmp) < 0)
    {
      zlog_err (nzg, "Can not create AVL tree\n");
      return NSM_DCB_API_SET_ERR_NO_MEM;     
    }

  dcbg->bridge = bridge;
  bridge->dcbg = dcbg;

  NSM_DCB_FN_EXIT (NSM_DCB_API_SET_SUCCESS);
}

/**@brief This function deletes the application priority avl tree 
          from dcb interface.
   @param data - application priority node information
   @return None
*/
void_t
nsm_dcb_clear_appl_pri (void_t *data)
{
  struct nsm_dcb_application_priority *appl_pri = 
                               (struct nsm_dcb_application_priority *) data;
 
  NSM_DCB_FN_ENTER ();
  
  XFREE (MTYPE_NSM_DCB_APPL_PRI, appl_pri);
  
  NSM_DCB_FN_EXIT ();
}

/**@brief This function free DCB interface
   @param data - dcb interface node information
   @return None
*/
void_t
nsm_dcb_interface_free (void_t *avl_node_info)
{
  struct nsm_dcb_if *dcbif;
  
  NSM_DCB_FN_ENTER ();

  dcbif = (struct nsm_dcb_if *) avl_node_info;

  /* Remove application priority tree*/
  avl_tree_cleanup (dcbif->dcb_appl_pri, nsm_dcb_clear_appl_pri);
  avl_tree_free (&dcbif->dcb_appl_pri, NULL);

  XFREE (MTYPE_NSM_DCB_IF, dcbif);
 
  NSM_DCB_FN_EXIT ();
}
 
/**@brief This function deinitializes the nsm_dcb_bridge structure.
   @param master - Pointer to nsm_bridge_master  
   @return NSM_DCB_API_SET_SUCCESS
*/
s_int32_t 
nsm_dcb_deinit (struct nsm_bridge_master *master)
{
   struct nsm_dcb_bridge *dcbg;
   struct nsm_bridge *bridge;
#ifdef HAVE_HAL
   s_int32_t ret;
#endif /* HAVE_HAL */
  
  NSM_DCB_FN_ENTER ();

   bridge = master->bridge_list;
   while (bridge)
     {
#ifdef HAVE_HAL
        ret = hal_dcb_deinit (bridge->name);
        if (ret < 0)
          {
            zlog_err (nzg, "Failed deinitializing HAL DCB\n");
            NSM_DCB_FN_EXIT (NSM_DCB_API_SET_ERR_HW_NO_SUPPORT);
          }
#endif /* HAVE_HAL */

       /* Look for the dcb bridge structure */
       dcbg = bridge->dcbg;
       if (! dcbg)
         NSM_DCB_FN_EXIT (NSM_DCB_API_SET_ERR_NO_DCBG);

       avl_tree_cleanup (dcbg->dcb_if_list, nsm_dcb_interface_free);
       avl_tree_free (&dcbg->dcb_if_list, NULL);

       XFREE (MTYPE_NSM_BRIDGE_DCB, dcbg);
       bridge->dcbg = NULL;

       bridge = bridge->next;
     }

   NSM_DCB_FN_EXIT (NSM_DCB_API_SET_SUCCESS);
}

/**@brief This function initializes DCB structures related to interface.
   @param vr_id - Router Identifier
   @param ifname - Name of the interface
   @return nsm_dcb_if - DCB interface structure pointer 
*/
struct nsm_dcb_if* 
nsm_dcb_init_interface (u_int32_t vr_id, char *ifname)
{
  struct nsm_dcb_if *dcbif = NULL;
  struct nsm_bridge_port *br_port;
  struct nsm_bridge *bridge;
  struct nsm_dcb_if tmp_dcbif;
  struct interface *ifp;
  struct nsm_master *nm;
  struct nsm_if *zif;
  struct nsm_dcb_bridge *dcbg;
  struct avl_node *node;
  bool_t new = PAL_FALSE;
  
  NSM_DCB_FN_ENTER ();

  /* Look for the NSM master */
  nm = nsm_master_lookup_by_id (nzg, vr_id);
  if (! nm)
    NSM_DCB_FN_EXIT (NULL);

  /* Look for the interface pointer */
  ifp = if_lookup_by_name (&nm->vr->ifm, ifname);  
  if (! ifp)
    NSM_DCB_FN_EXIT (NULL);
  
  /* Get the bridge information from interface*/
  zif = (struct nsm_if *) ifp->info;
  if (! zif)
    NSM_DCB_FN_EXIT (NULL);

  if (! (br_port = zif->switchport))
    NSM_DCB_FN_EXIT (NULL);
 
  if (!(bridge = zif->bridge))
    NSM_DCB_FN_EXIT (NULL);

  /* Look for the DCB Bridge structure */
  dcbg = bridge->dcbg;
  if (! dcbg)
    NSM_DCB_FN_EXIT (NULL);

  /* Check dcb interface is already created if not create new one */
  tmp_dcbif.ifp = ifp;
  node = avl_search (dcbg->dcb_if_list,(void_t *) &tmp_dcbif);
  if (node)
    dcbif = (struct nsm_dcb_if *) node->info;

  if (! dcbif)
    {
      dcbif = XCALLOC (MTYPE_NSM_DCB_IF, sizeof (struct nsm_dcb_if));
    
      if (! dcbif)
        {
           zlog_err (nzg, "Can not create dcb interface\n");
           NSM_DCB_FN_EXIT (NULL);
        }

       new = PAL_TRUE;
    }
  
  /* Initialize the DCB interface with default value */
  dcbif->dcbg = dcbg;
  dcbif->ifp = ifp;

  /* By Defauly all priorities belongs to TCG 15 with no bandwidth limit */
  pal_mem_set (&dcbif->tcg_priority_table[0], NSM_DCB_ETS_DEFAULT_TCGID, 
               (sizeof (u_int8_t) * NSM_DCB_NUM_USER_PRI));

  /* Note :Change default mode to auto once DCBX is implemented */
  dcbif->dcb_ets_mode = NSM_DCB_MODE_ON;
  dcbif->pfc_mode = NSM_DCB_MODE_ON;
 
  pal_mem_set (&dcbif->tcg_priority_count[0], 0,
               (sizeof (u_int8_t) * NSM_DCB_NUM_USER_PRI));

  pal_mem_set (&dcbif->tcg_bandwidth_table[0], 0,
               (sizeof (u_int16_t) * NSM_DCB_MAX_TCG_DEFAULT));


  if (CHECK_FLAG (dcbg->dcb_global_flags, NSM_DCB_ENABLE))
    SET_FLAG (dcbif->dcb_if_flags, NSM_DCB_IF_ENABLE);

  if (CHECK_FLAG (dcbg->dcb_global_flags, NSM_DCB_ETS_ENABLE))
    SET_FLAG (dcbif->dcb_if_flags, NSM_DCB_IF_ETS_ENABLE);

  if (CHECK_FLAG (dcbg->dcb_global_flags, NSM_DCB_PFC_ENABLE))
    SET_FLAG (dcbif->dcb_if_flags, NSM_DCB_IF_PFC_ENABLE);
 
  /* Insert the new interface in dcb interface tree */
  if (new)
    {
      if (avl_create (&dcbif->dcb_appl_pri, 0, nsm_dcb_appl_pri_cmp) < 0)
        {
          zlog_err (nzg, "Can not create application priority avl tree\n");
          NSM_DCB_FN_EXIT (NULL);
        }

      avl_insert (dcbg->dcb_if_list, dcbif);
    }
    
  NSM_DCB_FN_EXIT (dcbif);    
}

/**@brief This function initializes DCB structures related to interface for ETS
          feature only.
   @param vr_id - Router Identifier
   @param ifname - Name of the interface
   @return nsm_dcb_if - DCB interface structure pointer
*/
struct nsm_dcb_if* 
nsm_dcb_ets_init_interface (u_int32_t vr_id, char *ifname)
{  
  struct nsm_dcb_if *dcbif = NULL, tmp_dcbif;
  struct nsm_bridge_port *br_port;
  struct nsm_bridge *bridge;
  struct interface *ifp;
  struct nsm_master *nm;
  struct nsm_dcb_bridge *dcbg = NULL;
  struct avl_node *node;
  struct nsm_if *zif;
  
  NSM_DCB_FN_ENTER ();

  /* Look for NSM master */
  nm = nsm_master_lookup_by_id (nzg, vr_id);
  if (! nm)
    NSM_DCB_FN_EXIT (NULL);

  /* Look for the interface pointer */
  ifp = if_lookup_by_name (&nm->vr->ifm, ifname);  
  if (! ifp)
    NSM_DCB_FN_EXIT (NULL);
 
  /* get the bridge information */
  zif = (struct nsm_if *) ifp->info;
  if (! zif)
    NSM_DCB_FN_EXIT (NULL);

  if (! (br_port = zif->switchport))
    NSM_DCB_FN_EXIT (NULL);

  if (!(bridge = zif->bridge))
    NSM_DCB_FN_EXIT (NULL);

  /* Look for the dcb bridge structure */
  dcbg = bridge->dcbg;
  if (! dcbg)
    NSM_DCB_FN_EXIT (NULL);

  tmp_dcbif.ifp = ifp;

  /* Check dcb interface is already created if not create new one */
  node = avl_search (dcbg->dcb_if_list, (void_t *) &tmp_dcbif);

  if (node)
    dcbif = (struct nsm_dcb_if *) node->info;

  if (! dcbif)
    {
       zlog_err (nzg, "Can not create dcb interface\n");
       NSM_DCB_FN_EXIT (NULL);
    }

  if (CHECK_FLAG (dcbg->dcb_global_flags, NSM_DCB_ETS_ENABLE))
    SET_FLAG (dcbif->dcb_if_flags, NSM_DCB_IF_ETS_ENABLE);
  else
    NSM_DCB_FN_EXIT (NULL);

  pal_mem_set (&dcbif->tcg_priority_count[0], 0, 
               (sizeof (u_int8_t) * NSM_DCB_NUM_USER_PRI));
  
  /* By Defauly all priorities belongs to TCG 15 with no bandwidth limit */
  pal_mem_set (&dcbif->tcg_priority_table[0], NSM_DCB_ETS_DEFAULT_TCGID,
               (sizeof (u_int8_t) * NSM_DCB_NUM_USER_PRI));
  pal_mem_set (&dcbif->tcg_bandwidth_table[0], 0, 
               (sizeof (u_int16_t) * NSM_DCB_MAX_TCG_DEFAULT));
 

  /* Defaule Mode is ON */
  dcbif->dcb_ets_mode = NSM_DCB_MODE_ON;

  NSM_DCB_FN_EXIT (dcbif);
}

/**@brief This function deinits the DCB interface.
   @param vr_id - Router Identifier
   @param ifname - Name of interface
   @return NSM_DCB_API_SET_SUCCESS
*/
s_int32_t
nsm_dcb_deinit_interface (u_int32_t vr_id, char *ifname)
{
  struct nsm_dcb_if *dcbif = NULL, tmp_dcbif;
  struct nsm_bridge_port *br_port;
  struct nsm_bridge *bridge;
  struct interface *ifp;
  struct nsm_if *zif;
  struct nsm_master *nm;
  struct nsm_dcb_bridge *dcbg;
  struct avl_node *node;
   
  NSM_DCB_FN_ENTER ();

  /* Look for NSM master */
  nm = nsm_master_lookup_by_id (nzg, vr_id);
  if (! nm)
    NSM_DCB_FN_EXIT (NSM_DCB_API_SET_ERR_NO_NM);

  /* Look for the interface pointer */
  ifp = if_lookup_by_name (&nm->vr->ifm, ifname);  
  if (! ifp)
    NSM_DCB_FN_EXIT (NSM_DCB_API_SET_ERR_INTERFACE);

  /* Get the bridge information from interface */
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

  /* Look for the DCB interface */
  node = avl_search (dcbg->dcb_if_list, (void_t *) &tmp_dcbif);

  if (node)
    dcbif = (struct nsm_dcb_if *) node->info;

  if (! dcbif)
    NSM_DCB_FN_EXIT (NSM_DCB_API_SET_ERR_DCB_INTERFACE);

  avl_tree_free (&dcbif->dcb_appl_pri, NULL);

  avl_remove (dcbg->dcb_if_list, dcbif);
  XFREE (MTYPE_NSM_DCB_IF, dcbif);
 
  NSM_DCB_FN_EXIT (NSM_DCB_API_SET_SUCCESS);
}

/**@brief This function enabled DCB at switch level  
   @param vr_id - Router Identifier
   @param bridge_name - name of the bridge
   @return NSM_DCB_API_SET_SUCCESS
*/
s_int32_t 
nsm_dcb_bridge_enable (u_int32_t vr_id, char *bridge_name)
{
  struct nsm_dcb_if *dcbif;
  struct nsm_bridge_master *master;
  struct nsm_bridge *bridge;
  struct nsm_master *nm;
  struct nsm_dcb_bridge *dcbg = NULL;
  struct nsm_bridge_port *br_port;
  struct avl_node *avl_port;
#ifdef HAVE_HAL
  s_int32_t ret;
#endif /* HAVE_HAL */
    
  NSM_DCB_FN_ENTER ();

  /* Look for NSM master */
  nm = nsm_master_lookup_by_id (nzg, vr_id);
  if (! nm)
    NSM_DCB_FN_EXIT (NSM_DCB_API_SET_ERR_NO_NM);
 
  /* Look for the dcb bridge structure */
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
    {
      if (bridge_name)
        zlog_err (nzg, "Bridge %s not found\n", bridge_name);
#ifdef HAVE_DEFAULE_BRIDGE
      else 
        zlog_err (nzg, "Default bridge not found\n");
#endif /* HAVE_DEFAULT_BRIDGE */
      NSM_DCB_FN_EXIT (NSM_BRIDGE_ERR_NOTFOUND);
    }

  dcbg = bridge->dcbg;
  if (! dcbg)
    NSM_DCB_FN_EXIT (NSM_DCB_API_SET_ERR_NO_DCBG);

  if (CHECK_FLAG (dcbg->dcb_global_flags, NSM_DCB_ENABLE))
    NSM_DCB_FN_EXIT (NSM_DCB_API_SET_ERR_DCB_EXISTS);
 
#ifdef HAVE_HAL
  ret = hal_dcb_bridge_enable (bridge_name);
  if (ret < 0)
    {
      zlog_err (nzg, "Failed to enable DCB on the switch in the hardware %d\n"
                     , ret);
      NSM_DCB_FN_EXIT (NSM_DCB_API_SET_ERR_HW_NO_SUPPORT);
    }
#endif /* HAVE_HAL */

  SET_FLAG (dcbg->dcb_global_flags, NSM_DCB_ENABLE);
  SET_FLAG (dcbg->dcb_global_flags, NSM_DCB_ETS_ENABLE);
  SET_FLAG (dcbg->dcb_global_flags, NSM_DCB_PFC_ENABLE);

  /* Enable DCB on all L2 interfaces which belongs to bridge */
  for (avl_port = avl_top (bridge->port_tree); avl_port;
       avl_port = avl_next (avl_port))
    {
      if ((br_port = (struct nsm_bridge_port *)
                     AVL_NODE_INFO (avl_port)) == NULL)
        continue;
     
      dcbif = nsm_dcb_init_interface (vr_id, br_port->nsmif->ifp->name);
      if (!dcbif)
        NSM_DCB_FN_EXIT (NSM_DCB_API_SET_ERR_DCB_IF_INIT);
    }

  NSM_DCB_FN_EXIT (NSM_DCB_API_SET_SUCCESS);
}

/**@brief This function disables DCB feature on switch.
   @param vr_id - Router Identifier
   @bridge_name - Name of bridge 
   @return NSM_DCB_API_SET_SUCCESS
*/
s_int32_t 
nsm_dcb_bridge_disable (u_int32_t vr_id, char *bridge_name)
{
  struct nsm_master *nm;
  struct nsm_bridge_master *master;
  struct nsm_bridge *bridge;
  struct nsm_dcb_bridge *dcbg = NULL;
#ifdef HAVE_HAL
  s_int32_t ret;
#endif /* HAVE_HAL */
  
  NSM_DCB_FN_ENTER ();

  /* Look for NSM master */
  nm = nsm_master_lookup_by_id (nzg, vr_id);
  if (! nm)
    NSM_DCB_FN_EXIT (NSM_DCB_API_SET_ERR_NO_NM);

  /* Look for the dcb bridge structure */
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
    {
      if (bridge_name)
        zlog_err (nzg, "Bridge %s not found\n", bridge_name);
#ifdef HAVE_DEFAULT_BRIDGE
      else
        zlog_err (nzg, "Default Bridge not found\n");
#endif /* HAVE_DEFAULT_BRIDGE */

      NSM_DCB_FN_EXIT (NSM_BRIDGE_ERR_NOTFOUND);
    }

  dcbg = bridge->dcbg;
  if (! dcbg)
    NSM_DCB_FN_EXIT (NSM_DCB_API_SET_ERR_NO_DCBG);

  if (! CHECK_FLAG (dcbg->dcb_global_flags, NSM_DCB_ENABLE))
    NSM_DCB_FN_EXIT (NSM_DCB_API_SET_ERR_NO_DCB);

#ifdef HAVE_HAL
  ret = hal_dcb_bridge_disable (bridge_name);
  if (ret < 0)
    {
      zlog_err (nzg, "Failed to disable DCB on the switch in the hardware %d\n"
                     , ret);
      NSM_DCB_FN_EXIT (NSM_DCB_API_SET_ERR_HW_NO_SUPPORT);
    }
#endif /* HAVE_HAL */

  /* Disable the flags and remove all interfaces
   * Instead of calling nsm_dcb_deinit_interface here in a loop we are 
   * using the avl_tree_cleanup to cleanup the dcb_if_list avl tree 
   */
  avl_tree_cleanup (dcbg->dcb_if_list, nsm_dcb_interface_free);
  
  UNSET_FLAG (dcbg->dcb_global_flags, NSM_DCB_ENABLE);
  UNSET_FLAG (dcbg->dcb_global_flags, NSM_DCB_ETS_ENABLE);
  UNSET_FLAG (dcbg->dcb_global_flags, NSM_DCB_PFC_ENABLE);
 
  NSM_DCB_FN_EXIT (NSM_DCB_API_SET_SUCCESS);
}

/**@brief This functions traverses the avl tree of dcbif and sets the ETS flag
   @param avl_node_info - dcb interface node information
   @arg1 - It will be NULL always
   @return - NSM_DCB_API_SET_SUCCESS
*/
s_int32_t
nsm_dcb_avl_traverse_enable_ets (void_t *avl_node_info, void_t *arg1)
{
  struct nsm_dcb_if *dcbif = (struct nsm_dcb_if *)avl_node_info;
  struct nsm_dcb_if *new_tmp_dcbif = NULL;

  NSM_DCB_FN_ENTER ();

  if (dcbif && CHECK_FLAG (dcbif->dcb_if_flags, NSM_DCB_IF_ENABLE))
    {
      new_tmp_dcbif = nsm_dcb_ets_init_interface (dcbif->ifp->vr->id,
                                                  dcbif->ifp->name);
      if (! new_tmp_dcbif)
        NSM_DCB_FN_EXIT (NSM_DCB_API_SET_ERR_ETS_INTERFACE);
    }

  NSM_DCB_FN_EXIT (NSM_DCB_API_SET_SUCCESS);
}

/**@brief This function enables ETS feature of DCB on switch.
   @param vr_id - Router Identifier
   @param bridge_name - name of bridge 
   @return NSM_DCB_API_SET_SUCCESS
*/
s_int32_t 
nsm_dcb_ets_bridge_enable (u_int32_t vr_id, char *bridge_name)
{
  struct nsm_master *nm;
  struct nsm_bridge_master *master;
  struct nsm_bridge *bridge;
  struct nsm_dcb_bridge *dcbg = NULL;
  s_int32_t ret;
  
  NSM_DCB_FN_ENTER ();

  /* Look for NSM master */
  nm = nsm_master_lookup_by_id (nzg, vr_id);
  if (! nm)
    NSM_DCB_FN_EXIT (NSM_DCB_API_SET_ERR_NO_NM);

  /* Look for the dcb bridge structure */
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
    {
      if (bridge_name)
        zlog_err (nzg, "Bridge %s not found\n", bridge_name);
#ifdef HAVE_DEFAULT_BRIDGE
      else
        zlog_err (nzg, "Default Bridge not found\n");
#endif /* HAVE_DEFAULT_BRIDGE*/

      NSM_DCB_FN_EXIT (NSM_BRIDGE_ERR_NOTFOUND);
    }

  dcbg = bridge->dcbg;
  if (! dcbg)
    NSM_DCB_FN_EXIT (NSM_DCB_API_SET_ERR_NO_DCBG);

  /* DCB should be globally enabled and ETS should be globally disabled */
  if (! CHECK_FLAG (dcbg->dcb_global_flags, NSM_DCB_ENABLE))
    NSM_DCB_FN_EXIT (NSM_DCB_API_SET_ERR_NO_DCB);

  if (CHECK_FLAG (dcbg->dcb_global_flags, NSM_DCB_ETS_ENABLE))
    NSM_DCB_FN_EXIT (NSM_DCB_API_SET_ERR_ETS_EXISTS);
 
#ifdef HAVE_HAL
  ret = hal_dcb_ets_bridge_enable (bridge_name);
  if (ret < 0)
    {
      zlog_err (nzg, "Failed to enable ETS on the switch in the hardware %d\n"
                     , ret);
      NSM_DCB_FN_EXIT (NSM_DCB_API_SET_ERR_HW_NO_SUPPORT);
    }
#endif /* HAVE_HAL */

  SET_FLAG (dcbg->dcb_global_flags, NSM_DCB_ETS_ENABLE);
 
  /* Enable ETS on all L2 interfaces on which DCB is enabled */
  ret = avl_traverse (dcbg->dcb_if_list, nsm_dcb_avl_traverse_enable_ets, 
                      NULL);
  if (ret < 0)
    NSM_DCB_FN_EXIT (NSM_DCB_API_SET_ERR_ETS_IF_INIT);

  NSM_DCB_FN_EXIT (NSM_DCB_API_SET_SUCCESS);
}

/**@brief This function disables the ETS 
   @param node_info - dcb interface node information
   @param arg1 - It will be NULL always
   @return NSM_DCB_API_SET_SUCCESS
*/
s_int32_t
nsm_dcb_avl_traverse_disable_ets (void_t *node_info, void_t *arg1)
{
  struct nsm_dcb_if *dcbif = (struct nsm_dcb_if *) node_info;
   
  NSM_DCB_FN_ENTER ();

  /* We will not call nsm_dcb_deinit_interface here, as 
   * nsm_dcb_deinit_interface will remove the interface from the dcb_if_tree
   * but after disabling ETS , other modules (e.g. PFC) may need it.
   */
  if (dcbif && CHECK_FLAG (dcbif->dcb_if_flags, NSM_DCB_IF_ENABLE))
    UNSET_FLAG (dcbif->dcb_if_flags, NSM_DCB_IF_ETS_ENABLE);
  else 
    NSM_DCB_FN_EXIT (NSM_DCB_API_SET_ERR_ETS_INTERFACE);

  NSM_DCB_FN_EXIT (NSM_DCB_API_SET_SUCCESS);
}

/**@brief This function disables ETS feature of DCB on switch.
   @param vr_id - Router Identifier
   @bridge_name - Name of bridge 
   @return NSM_DCB_API_SET_SUCCESS
*/
s_int32_t 
nsm_dcb_ets_bridge_disable (u_int32_t vr_id, char *bridge_name)
{
  struct nsm_master *nm;
  struct nsm_bridge_master *master;
  struct nsm_bridge *bridge; 
  struct nsm_dcb_bridge *dcbg = NULL;
  int ret = NSM_DCB_API_SET_SUCCESS;
  
  NSM_DCB_FN_ENTER ();

  /* Look for NSM master */
  nm = nsm_master_lookup_by_id (nzg, vr_id);
  if (! nm)
    NSM_DCB_FN_EXIT (NSM_DCB_API_SET_ERR_NO_NM);

  /* Look for the dcb bridge structure */
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
    {
      if (bridge_name)
        zlog_err (nzg, "Bridge %s not found\n", bridge_name);
#ifdef HAVE_DEFAULT_BRIDGE
      else
        zlog_err (nzg, "Default bridge not found\n");
#endif /* HAVE_DEFAULT_BRIDGE */
      NSM_DCB_FN_EXIT (NSM_BRIDGE_ERR_NOTFOUND);
    }

  dcbg = bridge->dcbg;
  if (! dcbg)
    NSM_DCB_FN_EXIT (NSM_DCB_API_SET_ERR_NO_DCBG);

  /* DCB should be globally enabled and ETS should be globally disabled */
  if (! CHECK_FLAG (dcbg->dcb_global_flags, NSM_DCB_ENABLE))
    NSM_DCB_FN_EXIT (NSM_DCB_API_SET_ERR_INTERFACE_NO_DCB);

  if (! CHECK_FLAG (dcbg->dcb_global_flags, NSM_DCB_ETS_ENABLE))
    NSM_DCB_FN_EXIT (NSM_DCB_API_SET_ERR_INTERFACE_NO_ETS);

#ifdef HAVE_HAL
  ret = hal_dcb_ets_bridge_disable (bridge_name);
  if (ret < 0)
    {
      zlog_err (nzg, "Failed to disable ETS on the switch in the hardware %d\n"
                     , ret);
      NSM_DCB_FN_EXIT (NSM_DCB_API_SET_ERR_HW_NO_SUPPORT);
    }
#endif /* HAVE_HAL */

  UNSET_FLAG (dcbg->dcb_global_flags, NSM_DCB_ETS_ENABLE);

  ret = avl_traverse (dcbg->dcb_if_list, nsm_dcb_avl_traverse_disable_ets, 
                      NULL);

  if (ret < 0)
    NSM_DCB_FN_EXIT (NSM_DCB_API_SET_ERR_ETS_INTERFACE);

  NSM_DCB_FN_EXIT (NSM_DCB_API_SET_SUCCESS);
}

/**@brief This function initializes DCB structures related to interface for PFC
          feature.
   @param vr_id - Router Identifier
   @param ifname - Name of the interface
   @return nsm_dcb_if - DCB interface structure pointer
*/
struct nsm_dcb_if*
nsm_dcb_pfc_init_interface (u_int32_t vr_id, char *ifname)
{
  struct nsm_dcb_if *dcbif = NULL, tmp_dcbif;
  struct nsm_bridge_port *br_port;
  struct nsm_bridge *bridge;
  struct interface *ifp;
  struct nsm_master *nm;
  struct nsm_dcb_bridge *dcbg = NULL;
  struct avl_node *node;
  struct nsm_if *zif;

  NSM_DCB_FN_ENTER ();

  /* Look for NSM master */
  nm = nsm_master_lookup_by_id (nzg, vr_id);
  if (! nm)
    NSM_DCB_FN_EXIT (NULL);

  /* Look for the interface pointer */
  ifp = if_lookup_by_name (&nm->vr->ifm, ifname);
  if (! ifp)
    NSM_DCB_FN_EXIT (NULL);

  /* get the bridge information */
  zif = (struct nsm_if *) ifp->info;
  if (! zif)
    NSM_DCB_FN_EXIT (NULL);

  if (! (br_port = zif->switchport))
    NSM_DCB_FN_EXIT (NULL);

  if (!(bridge = zif->bridge))
    NSM_DCB_FN_EXIT (NULL);

  /* Look for the dcb bridge structure */
  dcbg = bridge->dcbg;
  if (! dcbg)
    NSM_DCB_FN_EXIT (NULL);

  tmp_dcbif.ifp = ifp;
  /* Check dcb interface is already created if not create new one */
  node = avl_search (dcbg->dcb_if_list, (void *) &tmp_dcbif);

  if (node)
    dcbif = (struct nsm_dcb_if *) node->info;

  if (! dcbif)
    NSM_DCB_FN_EXIT (NULL);

  if (CHECK_FLAG (dcbg->dcb_global_flags, NSM_DCB_PFC_ENABLE))
    SET_FLAG (dcbif->dcb_if_flags, NSM_DCB_IF_PFC_ENABLE);
  else
    NSM_DCB_FN_EXIT (NULL);
  
  pal_mem_set (&dcbif->pfc_enable_vector[0], NSM_DCB_PFC_DEFAULT_PRIORITY, 
                NSM_DCB_NUM_USER_PRI);

  dcbif->pfc_mode = NSM_DCB_PFC_MODE_ON;
  dcbif->pfc_cap = NSM_DCB_PFC_CAP_DEFAULT;
  dcbif->pfc_link_delay_allowance = NSM_DCB_PFC_LINK_DELAY_ALLOW_DEFAULT;
  dcbif->pfc_requests_sent = NSM_DCB_PFC_REQUEST_SENT;
  dcbif->pfc_indications_rcvd = NSM_DCB_PFC_INDICATIONS_RCVD;
  dcbif->pfc_en_pri_count = 0;


  NSM_DCB_FN_EXIT (dcbif);
}

/**@brief This function enables the PFC
   @param node_info - node information
   @return NSM_DCB_API_SET_SUCCESS
*/
s_int32_t
nsm_dcb_avl_traverse_enable_pfc (void *avl_node_info, void *arg1)
{
  struct nsm_dcb_if *dcbif = (struct nsm_dcb_if *)avl_node_info;
  struct nsm_dcb_if *new_tmp_dcbif = NULL;

  NSM_DCB_FN_ENTER ();

  if (dcbif && CHECK_FLAG (dcbif->dcb_if_flags, NSM_DCB_IF_ENABLE))
    new_tmp_dcbif = nsm_dcb_pfc_init_interface (dcbif->ifp->vr->id,
                                                dcbif->ifp->name);
  if (! new_tmp_dcbif)
    NSM_DCB_FN_EXIT (NSM_DCB_API_SET_ERR_PFC_INTERFACE);

  NSM_DCB_FN_EXIT (NSM_DCB_API_SET_SUCCESS);
}

/**@brief This function enables PFC feature of DCB on a bridge.
   @param vr_id - Router Identifier
   @param bridge_name - Bridge name
   @return NSM_DCB_API_SET_SUCCESS
*/
s_int32_t
nsm_dcb_pfc_bridge_enable (u_int32_t vr_id, char *bridge_name)
{
  struct nsm_master *nm;
  struct nsm_bridge_master *master;
  struct nsm_bridge *bridge;
  struct nsm_dcb_bridge *dcbg = NULL;
  s_int32_t ret;

  NSM_DCB_FN_ENTER ();

  /* Look for NSM master */
  nm = nsm_master_lookup_by_id (nzg, vr_id);
  if (! nm)
    NSM_DCB_FN_EXIT (NSM_DCB_API_SET_ERR_NO_NM);

  /* Look for the dcb bridge structure */
  master = nsm_bridge_get_master (nm);
  if (! master)
    NSM_DCB_FN_EXIT (NSM_BRIDGE_ERR_NOTFOUND);

  if (bridge_name == NULL)
#ifdef HAVE_DEFAULT_BRIDGE
    bridge = nsm_get_default_bridge (master);
#else
    NSM_DCB_FN_EXIT (NSM_BRIDGE_ERR_NOTFOUND);
#endif /* HAVE_DEFAULT_BRIDGE */
  else /*bridge_name != NULL*/
    bridge = nsm_lookup_bridge_by_name (master, bridge_name);

  if (! bridge)
    {
      if (bridge_name)
        zlog_err (nzg, "Bridge %s not found\n", bridge_name);
#ifdef HAVE_DEFAULT_BRIDGE
      else
        zlog_err (nzg, "Default bridge not found\n");
#endif /*HAVE_DEFAULT_BRIDGE*/
    NSM_DCB_FN_EXIT (NSM_BRIDGE_ERR_NOTFOUND);
    }/*if (! bridge)*/
  dcbg = bridge->dcbg;
  if (! dcbg)
    {
       zlog_err (nzg, "Global DCB structure not found\n");
       NSM_DCB_FN_EXIT (NSM_DCB_API_SET_ERR_NO_DCBG);
    }/*if (! dcbg)*/

  /* DCB should be globally enabled and PFC should be disabled */
  if (! CHECK_FLAG (dcbg->dcb_global_flags, NSM_DCB_ENABLE))
    NSM_DCB_FN_EXIT (NSM_DCB_API_SET_ERR_NO_DCB);

  if (CHECK_FLAG (dcbg->dcb_global_flags, NSM_DCB_PFC_ENABLE))
    NSM_DCB_FN_EXIT (NSM_DCB_API_SET_ERR_PFC_EXISTS);

#ifdef HAVE_HAL
  ret = hal_dcb_pfc_bridge_enable (bridge_name);
  if (ret < 0)
    {
      zlog_err (nzg, "Failed to enable PFC on the bridge in the hardware %d\n"
                     , ret);
      NSM_DCB_FN_EXIT (NSM_DCB_API_SET_ERR_HW_NO_SUPPORT);
    }
#endif /* HAVE_HAL */

  SET_FLAG (dcbg->dcb_global_flags, NSM_DCB_PFC_ENABLE);

  /* Enable PFC on all L2 interfaces on which DCB is enabled */
  ret = avl_traverse (dcbg->dcb_if_list, nsm_dcb_avl_traverse_enable_pfc,
                      NULL);
  if (ret < 0)
    NSM_DCB_FN_EXIT (NSM_DCB_API_SET_ERR_PFC_IF_INIT);

  NSM_DCB_FN_EXIT (NSM_DCB_API_SET_SUCCESS);
}/*nsm_dcb_pfc_bridge_enable*/

/**@brief This function disables the PFC
   @param node_info - node information
   @return NSM_DCB_API_SET_SUCCESS
*/
s_int32_t
nsm_dcb_avl_traverse_disable_pfc (void *node_info, void *arg1)
{
  struct nsm_dcb_if *dcbif = (struct nsm_dcb_if *) node_info;

  NSM_DCB_FN_ENTER ();

  if (dcbif && CHECK_FLAG (dcbif->dcb_if_flags, NSM_DCB_IF_ENABLE))
    UNSET_FLAG (dcbif->dcb_if_flags, NSM_DCB_IF_PFC_ENABLE);
  else
    NSM_DCB_FN_EXIT (NSM_DCB_API_SET_ERR_PFC_INTERFACE);

  NSM_DCB_FN_EXIT (NSM_DCB_API_SET_SUCCESS);
}

/**@brief This function disables PFC feature of DCB on the bridge.
   @param vr_id - Router Identifier
   @return NSM_DCB_API_SET_SUCCESS
*/
s_int32_t
nsm_dcb_pfc_bridge_disable (u_int32_t vr_id, char *bridge_name)
{
  struct nsm_master *nm;
  struct nsm_bridge_master *master;
  struct nsm_bridge *bridge;
  struct nsm_dcb_bridge *dcbg = NULL;
  int ret = NSM_DCB_API_SET_SUCCESS;

  NSM_DCB_FN_ENTER ();

  /* Look for NSM master */
  nm = nsm_master_lookup_by_id (nzg, vr_id);
  if (! nm)
    NSM_DCB_FN_EXIT (NSM_DCB_API_SET_ERR_NO_NM);

  /* Look for the dcb bridge structure */
  master = nsm_bridge_get_master (nm);
  if (! master)
    NSM_DCB_FN_EXIT (NSM_BRIDGE_ERR_NOTFOUND);

  if (bridge_name == NULL)
#ifdef HAVE_DEFAULT_BRIDGE
    bridge = nsm_get_default_bridge (master);
#else
    NSM_DCB_FN_EXIT (NSM_BRIDGE_ERR_NOTFOUND);
#endif /* HAVE_DEFAULT_BRIDGE */
  else /*bridge_name != NULL*/
    bridge = nsm_lookup_bridge_by_name (master, bridge_name);

  if (! bridge)
    {
      if (bridge_name)
        zlog_err (nzg, "Bridge %s not found\n", bridge_name);
#ifdef HAVE_DEFAULT_BRIDGE
      else
        zlog_err (nzg, "Default bridge not found\n");
#endif /*HAVE_DEFAULT_BRIDGE*/
      NSM_DCB_FN_EXIT (NSM_BRIDGE_ERR_NOTFOUND);
    }/*if (! bridge)*/

  dcbg = bridge->dcbg;
  if (! dcbg)
    {
       zlog_err (nzg, "Global DCB structure not found\n");
       NSM_DCB_FN_EXIT (NSM_DCB_API_SET_ERR_NO_DCBG);
    }/*if (! dcbg)*/

  /* DCB should be globally enabled and PFC should be disabled */
  if (! CHECK_FLAG (dcbg->dcb_global_flags, NSM_DCB_ENABLE))
    NSM_DCB_FN_EXIT (NSM_DCB_API_SET_ERR_INTERFACE_NO_DCB);

  if (! CHECK_FLAG (dcbg->dcb_global_flags, NSM_DCB_PFC_ENABLE))
    NSM_DCB_FN_EXIT (NSM_DCB_API_SET_ERR_INTERFACE_NO_PFC);

#ifdef HAVE_HAL
  ret = hal_dcb_pfc_bridge_disable (bridge_name);
  if (ret < 0)
    {
      zlog_err (nzg, "Failed to disable ETS on the switch in the hardware %d\n"
                     , ret);
      NSM_DCB_FN_EXIT (NSM_DCB_API_SET_ERR_HW_NO_SUPPORT);
    }
#endif /* HAVE_HAL */

  UNSET_FLAG (dcbg->dcb_global_flags, NSM_DCB_PFC_ENABLE);

  ret = avl_traverse (dcbg->dcb_if_list, nsm_dcb_avl_traverse_disable_pfc,
                      NULL);

  if (ret < 0)
    NSM_DCB_FN_EXIT (NSM_DCB_API_SET_ERR_PFC_INTERFACE);

  NSM_DCB_FN_EXIT (NSM_DCB_API_SET_SUCCESS);
}
/**@brief This function is to find out if any duplicate arguments are configured
 * @param count - number of arguments
 * @param value - non duplicate value
 * @returns NSM_DCB_API_SET_ERR_DUPLICATE_ARGS/NSM_DCB_API_SET_SUCCESS
 */
s_int32_t
nsm_find_duplicate_args (u_int8_t count, u_int8_t value)
{
    u_char set_count[] = {0,1,1,2,1,2,2,3,1,2,2,3,2,3,3,4};
    u_int8_t tmp_count = 0;

    tmp_count = set_count[value & 0x0f];
    tmp_count += set_count[value >> 4];

    if (tmp_count == count)
      return NSM_DCB_API_SET_SUCCESS;
    else
      return NSM_DCB_API_SET_ERR_DUPLICATE_ARGS;
}

/**@brief This function enables PFC on interface
   @param vr_id - Router identifier
   @param ifname - Name of Interface
   @return NSM_DCB_API_SET_SUCCESS
*/
s_int32_t 
nsm_dcb_pfc_enable_interface (u_int32_t vr_id, char *ifname)
{
  struct nsm_dcb_if *dcbif = NULL, tmp_dcbif;
  struct nsm_bridge_port *br_port;
  struct nsm_bridge *bridge;
  struct interface *ifp;
  struct nsm_master *nm;
  struct nsm_dcb_bridge *dcbg = NULL;
  struct avl_node *node;
  struct nsm_if *zif;
#ifdef HAVE_HAL
  s_int32_t ret;
#endif /* HAVE_HAL */
  
  NSM_DCB_FN_ENTER ();

  /* Look for NSM master */
  nm = nsm_master_lookup_by_id (nzg, vr_id);
  if (! nm)
    NSM_DCB_FN_EXIT (NSM_DCB_API_SET_ERR_NO_NM);

  /* Look for the interface pointer */
  ifp = if_lookup_by_name (&nm->vr->ifm, ifname);
  if (! ifp)
    NSM_DCB_FN_EXIT (NSM_DCB_API_SET_ERR_INTERFACE);
 
  /* Check for the duplex mode of the interface */
  if ((nsm_fea_if_get_duplex (ifp)) < 0)
    NSM_DCB_FN_EXIT (NSM_DCB_API_FAIL_TO_GET_DUPLEX);

  if (ifp->duplex != NSM_IF_FULL_DUPLEX)
    NSM_DCB_FN_EXIT (NSM_DCB_API_SET_ERR_INTF_DUPLEX);
  
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

  if (! CHECK_FLAG (dcbg->dcb_global_flags, NSM_DCB_PFC_ENABLE))
    NSM_DCB_FN_EXIT (NSM_DCB_API_SET_ERR_NO_PFC);

  tmp_dcbif.ifp = ifp;

  /* Find the DCB interface */
  node = avl_search (dcbg->dcb_if_list, (void_t *) &tmp_dcbif);
  if (node)
    dcbif = (struct nsm_dcb_if *) node->info;

  if (! dcbif)
    NSM_DCB_FN_EXIT (NSM_DCB_API_SET_ERR_DCB_INTERFACE);

  if (! CHECK_FLAG (dcbif->dcb_if_flags, NSM_DCB_IF_ENABLE))
    NSM_DCB_FN_EXIT (NSM_DCB_API_SET_ERR_INTERFACE_NO_DCB);

  if (CHECK_FLAG (dcbif->dcb_if_flags, NSM_DCB_IF_PFC_ENABLE))
    NSM_DCB_FN_EXIT (NSM_DCB_API_SET_ERR_PFC_EXISTS);

#ifdef HAVE_HAL
  ret = hal_dcb_pfc_interface_enable (ifp->bridge_name, ifp->ifindex);
  if (ret < 0)
    {
      zlog_err (nzg, "Failed to enable PFC on the interface in the"
                     " hardware %d\n", ret);
      NSM_DCB_FN_EXIT (NSM_DCB_API_SET_ERR_HW_NO_SUPPORT);
    }
#endif /* HAVE_HAL */

  if ((nsm_flow_control_disable_if (ifp)) == RESULT_ERROR)
   NSM_DCB_FN_EXIT (NSM_DCB_API_UNSET_ERR_LINK_FLOW_CNTL);
  
  if (nsm_dcb_pfc_init_interface (vr_id, ifname) == NULL)
   NSM_DCB_FN_EXIT (NSM_DCB_API_SET_ERR_PFC_IF_INIT);


  NSM_DCB_FN_EXIT (NSM_DCB_API_SET_SUCCESS);
}

/**@brief This function disables PFC on interface
   @param vr_id - Router identifier
   @param ifname - Name of Interface
   @return NSM_DCB_API_SET_SUCCESS
*/
s_int32_t 
nsm_dcb_pfc_disable_interface (u_int32_t vr_id, char *ifname)
{
  struct nsm_dcb_if *dcbif = NULL, tmp_dcbif;
  struct nsm_bridge_port *br_port;
  struct nsm_bridge *bridge; 
  struct interface *ifp;
  struct nsm_master *nm;
  struct nsm_if *zif;
  struct nsm_dcb_bridge *dcbg = NULL;
  struct avl_node *node;
#ifdef HAVE_HAL
  s_int32_t ret;
#endif /* HAVE_HAL */
  
  NSM_DCB_FN_ENTER ();

  /* Look for NSM master */
  nm = nsm_master_lookup_by_id (nzg, vr_id);
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

  if (! CHECK_FLAG (dcbg->dcb_global_flags, NSM_DCB_PFC_ENABLE))
    NSM_DCB_FN_EXIT (NSM_DCB_API_SET_ERR_NO_PFC);

  tmp_dcbif.ifp = ifp;

  /* Find the DCB interface */
  node = avl_search (dcbg->dcb_if_list, (void_t *) &tmp_dcbif);
  if (node)
    dcbif = (struct nsm_dcb_if *) node->info;

  if (! dcbif)
    NSM_DCB_FN_EXIT (NSM_DCB_API_SET_ERR_DCB_INTERFACE);

  if (! CHECK_FLAG (dcbif->dcb_if_flags, NSM_DCB_IF_ENABLE))
    NSM_DCB_FN_EXIT (NSM_DCB_API_SET_ERR_INTERFACE_NO_DCB);

  if (! CHECK_FLAG (dcbif->dcb_if_flags, NSM_DCB_IF_PFC_ENABLE))
    NSM_DCB_FN_EXIT (NSM_DCB_API_SET_ERR_INTERFACE_NO_PFC);
 
#ifdef HAVE_HAL
  ret = hal_dcb_interface_disable (ifp->bridge_name, ifp->ifindex);
  if (ret < 0)
    {
      zlog_err (nzg, "Failed to disable PFC on the interface in the"
                     " hardware %d\n", ret);
      NSM_DCB_FN_EXIT (NSM_DCB_API_SET_ERR_HW_NO_SUPPORT);
    }
#endif /* HAVE_HAL */

  if ((nsm_flow_control_enable_if (ifp)) == RESULT_ERROR)
   NSM_DCB_FN_EXIT (NSM_DCB_API_SET_ERR_LINK_FLOW_CNTL);
  
  UNSET_FLAG (dcbif->dcb_if_flags, NSM_DCB_IF_PFC_ENABLE);

  NSM_DCB_FN_EXIT (NSM_DCB_API_SET_SUCCESS);
}

/**@brief This function enables PFC on the given priorities of the interface
   @param vr_id - Router identifier
   @param ifname - Name of Interface
   @param priority - Priorities on which PFC to be enabled
   @return NSM_DCB_API_SET_SUCCESS
*/
s_int32_t
nsm_dcb_add_pfc_priority (u_int32_t vr_id, char *ifname, s_int8_t priority,
                           bool_t state)
{
  struct nsm_dcb_if *dcbif = NULL, tmp_dcbif;
  struct nsm_bridge_port *br_port;
  struct nsm_bridge *bridge;
  struct interface *ifp;
  struct nsm_master *nm;
  struct nsm_dcb_bridge *dcbg = NULL;
  struct avl_node *node;
  struct nsm_if *zif;
  u_int8_t tmp_pri;
  u_int8_t count = 0;
#ifdef HAVE_HAL
  s_int32_t ret;
#endif /* HAVE_HAL */

  NSM_DCB_FN_ENTER ();

  /* Look for NSM master */
  nm = nsm_master_lookup_by_id (nzg, vr_id);
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

  if (! CHECK_FLAG (dcbg->dcb_global_flags, NSM_DCB_PFC_ENABLE))
    NSM_DCB_FN_EXIT (NSM_DCB_API_SET_ERR_NO_PFC);

  tmp_dcbif.ifp = ifp;

  /* Find the DCB interface */
  node = avl_search (dcbg->dcb_if_list, (void *) &tmp_dcbif);
  if (node)
    dcbif = (struct nsm_dcb_if *) node->info;

  if (! dcbif)
    NSM_DCB_FN_EXIT (NSM_DCB_API_SET_ERR_DCB_INTERFACE);

  if (! CHECK_FLAG (dcbif->dcb_if_flags, NSM_DCB_IF_ENABLE))
    NSM_DCB_FN_EXIT (NSM_DCB_API_SET_ERR_INTERFACE_NO_DCB);

  /*Check if PFC is enabled or not on the interface*/
  if (!CHECK_FLAG (dcbif->dcb_if_flags, NSM_DCB_IF_PFC_ENABLE))
    NSM_DCB_FN_EXIT (NSM_DCB_API_SET_ERR_INTERFACE_NO_PFC);

#ifdef HAVE_HAL
  ret = hal_dcb_enable_pfc_priority (ifp->bridge_name, ifp->ifindex, priority);
  if (ret < 0)
    {
      zlog_err (nzg, "Failed to enable PFC on the interface in the"
                     " hardware %d\n", ret);
      NSM_DCB_FN_EXIT (NSM_DCB_API_SET_ERR_HW_NO_SUPPORT);
    }
#endif /* HAVE_HAL */

  /*Enable PFC on the configured priorities*/
    if (priority == 0)/*Means disable PFC on all COSs*/
      {
        for (tmp_pri = 0; tmp_pri < NSM_DCB_NUM_USER_PRI; tmp_pri++)
          dcbif->pfc_enable_vector[tmp_pri] = DCB_PFC_DISABLE;
      }/*if (priority == 0)*/
    else
      {
        for (tmp_pri = 0; tmp_pri < NSM_DCB_NUM_USER_PRI; tmp_pri++)
          {
            if ((priority & (1 << tmp_pri)))
              {
                if ((dcbif->pfc_cap == count) && (state == PAL_TRUE))
                  {
                    dcbif->pfc_en_pri_count = count;
                    NSM_DCB_FN_EXIT (NSM_DCB_API_SET_ERR_EXCEED_PFC_CAP);
                  }
              
                /* If the priority is part of TCG 0-7, then it should be
                 * added to TCG 15 fist before changing the PFC enable 
                 * state.
                 */
                if ((dcbif->tcg_priority_table[tmp_pri] != 
                     NSM_DCB_ETS_DEFAULT_TCGID)
                     && (dcbif->pfc_enable_vector[tmp_pri] != state))
                  {
                    NSM_DCB_FN_EXIT (NSM_DCB_API_SET_WARN_DIFF_TCG_PFC);
                  }
                
                dcbif->pfc_enable_vector[tmp_pri] = 
                     (state ? DCB_PFC_ENABLE : DCB_PFC_DISABLE);
              }/*if ((priority & (1 << tmp_pri)))*/

            if (dcbif->pfc_enable_vector[tmp_pri])
              count ++;
          }/*for (tmp_pri = 0; tmp_pri < NSM_DCB_NUM_USER_PRI; tmp_pri++)*/
      }/*else (priority != 0)*/
    
  dcbif->pfc_en_pri_count = count;

  NSM_DCB_FN_EXIT (NSM_DCB_API_SET_SUCCESS);
}/*nsm_dcb_add_pfc_priority (u_int32_t vr_id, char *ifname,*/

/**@brief This function sets the PFC mode on the interface.
   @param ifname - Name of the interface
   @param mode - Mode Value On/Auto
   @return NSM_DCB_API_SET_SUCCESS
*/
s_int32_t
nsm_dcb_set_pfc_mode (u_int32_t vr_id, char *ifname, enum nsm_dcb_pfc_mode mode)
{
  struct nsm_dcb_if *dcbif = NULL, tmp_dcbif;
  struct nsm_bridge_port *br_port;
  struct nsm_bridge *bridge;
  struct interface *ifp;
  struct nsm_master *nm;
  struct nsm_dcb_bridge *dcbg = NULL;
  struct avl_node *node;
  struct nsm_if *zif;
#ifdef HAVE_HAL
  s_int32_t ret;
#endif /* HAVE_HAL */

  NSM_DCB_FN_ENTER ();

  /* Look for NSM master */
  nm = nsm_master_lookup_by_id (nzg, vr_id);
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

  if (! CHECK_FLAG (dcbg->dcb_global_flags, NSM_DCB_ENABLE))
    NSM_DCB_FN_EXIT (NSM_DCB_API_SET_ERR_NO_DCB);

  if (! CHECK_FLAG (dcbg->dcb_global_flags, NSM_DCB_PFC_ENABLE))
    NSM_DCB_FN_EXIT (NSM_DCB_API_SET_ERR_NO_PFC);

  tmp_dcbif.ifp = ifp;

  /* Find the DCB interface */
  node = avl_search (dcbg->dcb_if_list, (void *) &tmp_dcbif);
  if (node)
    dcbif = (struct nsm_dcb_if *) node->info;

  if (! dcbif)
    NSM_DCB_FN_EXIT (NSM_DCB_API_SET_ERR_DCB_INTERFACE);

  /* Check PFC is Enabled on the interface */
  if (! CHECK_FLAG (dcbif->dcb_if_flags, NSM_DCB_IF_PFC_ENABLE))
    NSM_DCB_FN_EXIT (NSM_DCB_API_SET_ERR_INTERFACE_NO_PFC);

#ifdef HAVE_HAL
  ret = hal_dcb_select_pfc_mode (ifp->bridge_name, ifp->ifindex, mode);
  if (ret < 0)
    {
      zlog_err (nzg, "Failed to set PFC mode on the interface %s in the"
                     " hardware %d\n", ifp->name, ret);
      NSM_DCB_FN_EXIT (NSM_DCB_API_SET_ERR_HW_NO_SUPPORT);
    }
#endif /* HAVE_HAL */

  /* set the mode */
  dcbif->pfc_mode = mode;

  NSM_DCB_FN_EXIT (NSM_DCB_API_SET_SUCCESS);
}/*nsm_dcb_set_pfc_mode (u_int32_t vr_id, char *ifname, enum nsm_dcb_pfc_mode*/

/**@brief This function sets the PFC cap on the interface.
   @param ifname - Name of the interface
   @param cap - Maximum Priorities that support PFC
   @return NSM_DCB_API_SET_SUCCESS
*/
s_int32_t
nsm_dcb_set_pfc_cap (u_int32_t vr_id, char *ifname, u_int8_t cap)
{
  struct nsm_dcb_if *dcbif = NULL, tmp_dcbif;
  struct nsm_bridge_port *br_port;
  struct nsm_bridge *bridge;
  struct interface *ifp;
  struct nsm_master *nm;
  struct nsm_dcb_bridge *dcbg = NULL;
  struct avl_node *node;
  struct nsm_if *zif;
#ifdef HAVE_HAL
  s_int32_t ret;
#endif /* HAVE_HAL */

  NSM_DCB_FN_ENTER ();

  /* Look for NSM master */
  nm = nsm_master_lookup_by_id (nzg, vr_id);
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

  if (! CHECK_FLAG (dcbg->dcb_global_flags, NSM_DCB_ENABLE))
    NSM_DCB_FN_EXIT (NSM_DCB_API_SET_ERR_NO_DCB);

  if (! CHECK_FLAG (dcbg->dcb_global_flags, NSM_DCB_PFC_ENABLE))
    NSM_DCB_FN_EXIT (NSM_DCB_API_SET_ERR_NO_ETS);

  tmp_dcbif.ifp = ifp;

  /* Find the DCB interface */
  node = avl_search (dcbg->dcb_if_list, (void *) &tmp_dcbif);
  if (node)
    dcbif = (struct nsm_dcb_if *) node->info;

  if (! dcbif)
    NSM_DCB_FN_EXIT (NSM_DCB_API_SET_ERR_DCB_INTERFACE);

  /* Check PFC is Enabled on the interface */
  if (! CHECK_FLAG (dcbif->dcb_if_flags, NSM_DCB_IF_PFC_ENABLE))
    NSM_DCB_FN_EXIT (NSM_DCB_API_SET_ERR_INTERFACE_NO_PFC);

  if (dcbif->pfc_en_pri_count > cap )
    NSM_DCB_FN_EXIT (NSM_DCB_API_SET_ERR_PFC_CAP);

#ifdef HAVE_HAL
  ret = hal_dcb_set_pfc_cap (ifp->bridge_name, ifp->ifindex, cap);
  if (ret < 0)
    {
      zlog_err (nzg, "Failed to set PFC Cap on the interface %s in the"
                     " hardware %d\n", ifp->name, ret);
      NSM_DCB_FN_EXIT (NSM_DCB_API_SET_ERR_HW_NO_SUPPORT);
    }
#endif /* HAVE_HAL */

  /* set the pfc cap */
  dcbif->pfc_cap = cap;

  NSM_DCB_FN_EXIT (NSM_DCB_API_SET_SUCCESS);
}

/**@brief This function sets the PFC link delay allowance on the interface.
   @param ifname - Name of the interface
   @param lda - Link delay allowance value
   @return NSM_DCB_API_SET_SUCCESS
*/
s_int32_t
nsm_dcb_set_pfc_lda (u_int32_t vr_id, char *ifname, u_int32_t lda)
{
  struct nsm_dcb_if *dcbif = NULL, tmp_dcbif;
  struct nsm_bridge_port *br_port;
  struct nsm_bridge *bridge;
  struct interface *ifp;
  struct nsm_master *nm;
  struct nsm_dcb_bridge *dcbg = NULL;
  struct avl_node *node;
  struct nsm_if *zif;
#ifdef HAVE_HAL
  s_int32_t ret;
#endif /* HAVE_HAL */

  NSM_DCB_FN_ENTER ();

  /* Look for NSM master */
  nm = nsm_master_lookup_by_id (nzg, vr_id);
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

  if (! CHECK_FLAG (dcbg->dcb_global_flags, NSM_DCB_ENABLE))
    NSM_DCB_FN_EXIT (NSM_DCB_API_SET_ERR_NO_DCB);

  if (! CHECK_FLAG (dcbg->dcb_global_flags, NSM_DCB_PFC_ENABLE))
    NSM_DCB_FN_EXIT (NSM_DCB_API_SET_ERR_NO_PFC);

  tmp_dcbif.ifp = ifp;

  /* Find the DCB interface */
  node = avl_search (dcbg->dcb_if_list, (void *) &tmp_dcbif);
  if (node)
    dcbif = (struct nsm_dcb_if *) node->info;

  if (! dcbif)
    NSM_DCB_FN_EXIT (NSM_DCB_API_SET_ERR_DCB_INTERFACE);

  /* Check PFC is Enabled on the interface */
  if (! CHECK_FLAG (dcbif->dcb_if_flags, NSM_DCB_IF_PFC_ENABLE))
    NSM_DCB_FN_EXIT (NSM_DCB_API_SET_ERR_INTERFACE_NO_PFC);

#ifdef HAVE_HAL
  ret = hal_dcb_set_pfc_lda (ifp->bridge_name, ifp->ifindex, lda);
  if (ret < 0)
    {
      zlog_err (nzg, "Failed to set PFC LDA on the interface %s in the"
                     " hardware %d\n", ifp->name, ret);
      NSM_DCB_FN_EXIT (NSM_DCB_API_SET_ERR_HW_NO_SUPPORT);
    }
#endif /* HAVE_HAL */

  /* set the mode */
  dcbif->pfc_link_delay_allowance = lda;

  NSM_DCB_FN_EXIT (NSM_DCB_API_SET_SUCCESS);
}/* nsm_dcb_set_pfc_lda (u_int32_t vr_id, char *ifname, u_int32_t lda) */


/**@brief This function enables DCB on interface 
   @param vr_id - Router Identifier
   @param ifname - Name of Interface
   @return NSM_DCB_API_SET_SUCCESS
*/
s_int32_t
nsm_dcb_enable_interface (u_int32_t vr_id, char *ifname)
{
  struct nsm_dcb_if *dcbif = NULL, tmp_dcbif;
  struct nsm_bridge_port *br_port;
  struct nsm_bridge *bridge;
  struct nsm_if *zif;
  struct interface *ifp;
  struct nsm_master *nm;
  struct nsm_dcb_bridge *dcbg = NULL;
  struct avl_node *node;
#ifdef HAVE_HAL
  s_int32_t ret;
#endif /* HAVE_HAL */
  
  NSM_DCB_FN_ENTER ();

  /* Look for NSM master */
  nm = nsm_master_lookup_by_id (nzg, vr_id);
  if (! nm)
    NSM_DCB_FN_EXIT (NSM_DCB_API_SET_ERR_NO_NM);

  /* Look for the interface pointer */
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

  dcbg = bridge->dcbg;
  if (! dcbg)
    NSM_DCB_FN_EXIT (NSM_DCB_API_SET_ERR_NO_DCBG);

  if (! CHECK_FLAG (dcbg->dcb_global_flags, NSM_DCB_ENABLE))
    NSM_DCB_FN_EXIT (NSM_DCB_API_SET_ERR_NO_DCB);

  tmp_dcbif.ifp = ifp;
  /* Find the DCB interface */
  node = avl_search (dcbg->dcb_if_list, (void_t *) &tmp_dcbif);
  if (node)
    dcbif = (struct nsm_dcb_if *) node->info;

  if (! dcbif)
     NSM_DCB_FN_EXIT (NSM_DCB_API_SET_ERR_DCB_INTERFACE);

  if (CHECK_FLAG (dcbif->dcb_if_flags, NSM_DCB_IF_ENABLE))
    NSM_DCB_FN_EXIT (NSM_DCB_API_SET_ERR_DCB_EXISTS);

#ifdef HAVE_HAL
  ret = hal_dcb_interface_enable (ifp->bridge_name, ifp->ifindex);
  if (ret < 0)
    {
      zlog_err (nzg, "Failed to enable DCB on the interface in the"
                     " hardware %d\n", ret);
      NSM_DCB_FN_EXIT (NSM_DCB_API_SET_ERR_HW_NO_SUPPORT);
    }
#endif /* HAVE_HAL */

  if (nsm_dcb_init_interface (vr_id, ifname) == NULL)
    NSM_DCB_FN_EXIT (NSM_DCB_API_SET_ERR_DCB_IF_INIT);

  NSM_DCB_FN_EXIT (NSM_DCB_API_SET_SUCCESS);
}

/**@brief This function disables DCB on interface 
   @param vr_id - Router Identifier
   @param ifname - Name of Interface
   @return NSM_DCB_API_SET_SUCCESS
*/
s_int32_t
nsm_dcb_disable_interface (u_int32_t vr_id, char *ifname)
{
  struct nsm_dcb_if *dcbif = NULL, tmp_dcbif;
  struct nsm_bridge_port *br_port;
  struct nsm_bridge *bridge;
  struct interface *ifp;
  struct nsm_master *nm;
  struct nsm_dcb_bridge *dcbg = NULL;
  struct avl_node *node;
  struct nsm_if *zif;
  s_int32_t ret = NSM_DCB_API_SET_SUCCESS;
  
  NSM_DCB_FN_ENTER ();

  /* Look for NSM master */
  nm = nsm_master_lookup_by_id (nzg, vr_id);
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

#ifdef HAVE_HAL
  ret = hal_dcb_interface_disable (ifp->bridge_name, ifp->ifindex);
  if (ret < 0)
    {
      zlog_err (nzg, "Failed to disable DCB on the interface in the"
                     " hardware %d\n", ret);
      NSM_DCB_FN_EXIT (NSM_DCB_API_SET_ERR_HW_NO_SUPPORT);
    }
#endif /* HAVE_HAL */
  
  /* Remove Application Priority tree entries */
  avl_tree_cleanup (dcbif->dcb_appl_pri, nsm_dcb_clear_appl_pri);

  UNSET_FLAG (dcbif->dcb_if_flags, NSM_DCB_IF_ENABLE);
  UNSET_FLAG (dcbif->dcb_if_flags, NSM_DCB_IF_ETS_ENABLE);
  UNSET_FLAG (dcbif->dcb_if_flags, NSM_DCB_IF_PFC_ENABLE);

  return NSM_DCB_API_SET_SUCCESS;
}

/**@brief This function enables ETS on interface
   @param vr_id - Router identifier
   @param ifname - Name of Interface
   @return NSM_DCB_API_SET_SUCCESS
*/
s_int32_t 
nsm_dcb_ets_enable_interface (u_int32_t vr_id, char *ifname)
{
  struct nsm_dcb_if *dcbif = NULL, tmp_dcbif;
  struct nsm_bridge_port *br_port;
  struct nsm_bridge *bridge;
  struct interface *ifp;
  struct nsm_master *nm;
  struct nsm_dcb_bridge *dcbg = NULL;
  struct avl_node *node;
  struct nsm_if *zif;
#ifdef HAVE_HAL
  s_int32_t ret;
#endif /* HAVE_HAL */
  
  NSM_DCB_FN_ENTER ();

  /* Look for NSM master */
  nm = nsm_master_lookup_by_id (nzg, vr_id);
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

  if (! CHECK_FLAG (dcbg->dcb_global_flags, NSM_DCB_ETS_ENABLE))
    NSM_DCB_FN_EXIT (NSM_DCB_API_SET_ERR_NO_ETS);

  tmp_dcbif.ifp = ifp;

  /* Find the DCB interface */
  node = avl_search (dcbg->dcb_if_list, (void_t *) &tmp_dcbif);
  if (node)
    dcbif = (struct nsm_dcb_if *) node->info;

  if (! dcbif)
    NSM_DCB_FN_EXIT (NSM_DCB_API_SET_ERR_DCB_INTERFACE);

  if (! CHECK_FLAG (dcbif->dcb_if_flags, NSM_DCB_IF_ENABLE))
    NSM_DCB_FN_EXIT (NSM_DCB_API_SET_ERR_INTERFACE_NO_DCB);

  if (CHECK_FLAG (dcbif->dcb_if_flags, NSM_DCB_IF_ETS_ENABLE))
    NSM_DCB_FN_EXIT (NSM_DCB_API_SET_ERR_ETS_EXISTS);

#ifdef HAVE_HAL
  ret = hal_dcb_ets_interface_enable (ifp->bridge_name, ifp->ifindex);
  if (ret < 0)
    {
      zlog_err (nzg, "Failed to enable ETS on the interface in the"
                     " hardware %d\n", ret);
      NSM_DCB_FN_EXIT (NSM_DCB_API_SET_ERR_HW_NO_SUPPORT);
    }
#endif /* HAVE_HAL */

  if (nsm_dcb_ets_init_interface (vr_id, ifname) == NULL)
   NSM_DCB_FN_EXIT (NSM_DCB_API_SET_ERR_ETS_IF_INIT);

  NSM_DCB_FN_EXIT (NSM_DCB_API_SET_SUCCESS);
}

/**@brief This function disables ETS on interface
   @param vr_id - Router identifier
   @param ifname - Name of Interface
   @return NSM_DCB_API_SET_SUCCESS
*/
s_int32_t 
nsm_dcb_ets_disable_interface (u_int32_t vr_id, char *ifname)
{
  struct nsm_dcb_if *dcbif = NULL, tmp_dcbif;
  struct nsm_bridge_port *br_port;
  struct nsm_bridge *bridge; 
  struct interface *ifp;
  struct nsm_master *nm;
  struct nsm_if *zif;
  struct nsm_dcb_bridge *dcbg = NULL;
  struct avl_node *node;
#ifdef HAVE_HAL
  s_int32_t ret;
#endif /* HAVE_HAL */
  
  NSM_DCB_FN_ENTER ();

  /* Look for NSM master */
  nm = nsm_master_lookup_by_id (nzg, vr_id);
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

  if (! CHECK_FLAG (dcbg->dcb_global_flags, NSM_DCB_ETS_ENABLE))
    NSM_DCB_FN_EXIT (NSM_DCB_API_SET_ERR_NO_ETS);

  tmp_dcbif.ifp = ifp;

  /* Find the DCB interface */
  node = avl_search (dcbg->dcb_if_list, (void_t *) &tmp_dcbif);
  if (node)
    dcbif = (struct nsm_dcb_if *) node->info;

  if (! dcbif)
    NSM_DCB_FN_EXIT (NSM_DCB_API_SET_ERR_DCB_INTERFACE);

  if (! CHECK_FLAG (dcbif->dcb_if_flags, NSM_DCB_IF_ENABLE))
    NSM_DCB_FN_EXIT (NSM_DCB_API_SET_ERR_INTERFACE_NO_DCB);

  if (! CHECK_FLAG (dcbif->dcb_if_flags, NSM_DCB_IF_ETS_ENABLE))
    NSM_DCB_FN_EXIT (NSM_DCB_API_SET_ERR_INTERFACE_NO_ETS);
 
#ifdef HAVE_HAL
  ret = hal_dcb_interface_disable (ifp->bridge_name, ifp->ifindex);
  if (ret < 0)
    {
      zlog_err (nzg, "Failed to disable ETS on the interface in the"
                     " hardware %d\n", ret);
      NSM_DCB_FN_EXIT (NSM_DCB_API_SET_ERR_HW_NO_SUPPORT);
    }
#endif /* HAVE_HAL */

  UNSET_FLAG (dcbif->dcb_if_flags, NSM_DCB_IF_ETS_ENABLE);

  NSM_DCB_FN_EXIT (NSM_DCB_API_SET_SUCCESS);
}

/**@brief This function sets the ETS mode on the interface.
   @param ifname - Name of the interface
   @param mode - Mode Value On/Auto
   @return NSM_DCB_API_SET_SUCCESS
*/
s_int32_t 
nsm_dcb_set_ets_mode (u_int32_t vr_id, char *ifname, enum nsm_dcb_mode mode)
{
  struct nsm_dcb_if *dcbif = NULL, tmp_dcbif;
  struct nsm_bridge_port *br_port;
  struct nsm_bridge *bridge;
  struct interface *ifp;
  struct nsm_master *nm;
  struct nsm_dcb_bridge *dcbg = NULL;
  struct avl_node *node;
  struct nsm_if *zif;
#ifdef HAVE_HAL
  s_int32_t ret;
#endif /* HAVE_HAL */
  
  NSM_DCB_FN_ENTER ();

  /* Look for NSM master */
  nm = nsm_master_lookup_by_id (nzg, vr_id);
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

  if (! CHECK_FLAG (dcbg->dcb_global_flags, NSM_DCB_ENABLE))
    NSM_DCB_FN_EXIT (NSM_DCB_API_SET_ERR_NO_DCB);
   
  if (! CHECK_FLAG (dcbg->dcb_global_flags, NSM_DCB_ETS_ENABLE))
    NSM_DCB_FN_EXIT (NSM_DCB_API_SET_ERR_NO_ETS);

  tmp_dcbif.ifp = ifp;

  /* Find the DCB interface */
  node = avl_search (dcbg->dcb_if_list, (void_t *) &tmp_dcbif);
  if (node)
    dcbif = (struct nsm_dcb_if *) node->info;

  if (! dcbif)
    NSM_DCB_FN_EXIT (NSM_DCB_API_SET_ERR_DCB_INTERFACE);

  /* Check ETS is Enabled on the interface */
  if (! CHECK_FLAG (dcbif->dcb_if_flags, NSM_DCB_IF_ETS_ENABLE))
    NSM_DCB_FN_EXIT (NSM_DCB_API_SET_ERR_INTERFACE_NO_ETS);

#ifdef HAVE_HAL
  ret = hal_dcb_select_ets_mode (ifp->bridge_name, ifp->ifindex, mode);
  if (ret < 0)
    {
      zlog_err (nzg, "Failed to set ETS mode on the interface %s in the"
                     " hardware %d\n", ifp->name, ret);
      NSM_DCB_FN_EXIT (NSM_DCB_API_SET_ERR_HW_NO_SUPPORT);
    }
#endif /* HAVE_HAL */

  /* Currently we are not supporting auto mode as DCBX is not 
   * implemented.
   */
  if (mode == NSM_DCB_MODE_AUTO)
    NSM_DCB_FN_EXIT (NSM_DCB_API_SET_ERR_NO_SUPPORT);

  /* set the mode */  
  dcbif->dcb_ets_mode = mode;

  NSM_DCB_FN_EXIT (NSM_DCB_API_SET_SUCCESS);
}

/**@brief This function checks PFC enable/disable state of priorities that 
          belongs to same TCG. It returns TRUE when priorities have same state.
   @param dcbif - DCB Interface Pointer
   @param tcgid - Traffic-class-group Identifier
   @param pri - User Priority Value
   @return NSM_DCB_API_SET_SUCCESS 
*/
bool_t
nsm_dcb_same_pfc_state (struct nsm_dcb_if *dcbif, u_int8_t tcgid, u_int8_t pri)
{
  u_int8_t priority;
  bool_t ret = PAL_FALSE;

  NSM_DCB_FN_ENTER ();
  
  for (priority = 0; priority < NSM_DCB_NUM_USER_PRI; priority++)
     {
       if (dcbif->tcg_priority_table[priority] == tcgid)
         {
           if (dcbif->pfc_enable_vector[priority] == 
               dcbif->pfc_enable_vector[pri])
             {
              ret = PAL_TRUE;
              break;
             }
         }
     } 

  NSM_DCB_FN_EXIT (ret);
}

/**@brief This function adds priority into traffic class group on interface.
   @param vr_id - Router Identifier
   @param ifname - Name of interface
   @param tcgid - Traffic-class-group identifier
   @param pri - priority value
   @return NSM_DCB_API_SET_SUCCESS
*/
s_int32_t 
nsm_dcb_add_pri_to_tcg (u_int32_t vr_id, char *ifname, u_int8_t tcgid, 
                        u_int8_t pri)
{
  struct nsm_dcb_if *dcbif = NULL, tmp_dcbif;
  struct nsm_bridge_port *br_port;
  struct nsm_bridge *bridge;
  struct interface *ifp;
  struct nsm_master *nm;
  struct nsm_dcb_bridge *dcbg = NULL;
  struct avl_node *node;
  struct nsm_if *zif;
  u_int8_t usr_pri; 
  /* Initialize to outside of the priority range <0-7> */
  u_int8_t keep_pri = NSM_DCB_PFC_INVALID_PRI; 
  s_int32_t ret;
  
  NSM_DCB_FN_ENTER ();

  /* Look for NSM master */
  nm = nsm_master_lookup_by_id (nzg, vr_id);
  if (! nm)
    NSM_DCB_FN_EXIT (NSM_DCB_API_SET_ERR_NO_NM);

  /* Look for the interface pointer */
  ifp = if_lookup_by_name (&nm->vr->ifm, ifname);
  if (! ifp)
    NSM_DCB_FN_EXIT  (NSM_DCB_API_SET_ERR_INTERFACE);

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

  if (! CHECK_FLAG (dcbg->dcb_global_flags, NSM_DCB_ENABLE))
   NSM_DCB_FN_EXIT (NSM_DCB_API_SET_ERR_NO_DCB);

  if (! CHECK_FLAG (dcbg->dcb_global_flags, NSM_DCB_ETS_ENABLE))
   NSM_DCB_FN_EXIT (NSM_DCB_API_SET_ERR_NO_ETS);

  tmp_dcbif.ifp = ifp;
 
  /* Find the DCB interface */
  node = avl_search (dcbg->dcb_if_list, (void_t *) &tmp_dcbif);
  if (node)
    dcbif = (struct nsm_dcb_if *) node->info;

  if (! dcbif)
    NSM_DCB_FN_EXIT (NSM_DCB_API_SET_ERR_DCB_INTERFACE);
 
  /* Check ETS is Enabled on the interface */
  if (! CHECK_FLAG (dcbif->dcb_if_flags, NSM_DCB_IF_ETS_ENABLE))
    NSM_DCB_FN_EXIT (NSM_DCB_API_SET_ERR_INTERFACE_NO_ETS);

  for (usr_pri = 0; usr_pri < NSM_DCB_NUM_USER_PRI; usr_pri++)
    {
      if ((pri & (1 << usr_pri)))
        {
          /* Check priority belongs to different TCG */
          if (dcbif->tcg_priority_table [usr_pri] != tcgid && 
              dcbif->tcg_priority_table [usr_pri] != NSM_DCB_ETS_DEFAULT_TCGID)
            { 
              ret = NSM_DCB_API_SET_ERR_WRONG_TCG;
              goto CLEANUP;
            }

          /* Check Priority is already part of TCG */
          if (dcbif->tcg_priority_table [usr_pri] == tcgid)
            {  
              zlog_err (nzg, 
                        "Priority %d already belongs to TCG %d\n", 
                         usr_pri, tcgid);
              ret = NSM_DCB_API_SET_ERR_TCG_PRI_EXISTS;
              keep_pri = usr_pri;
              goto CLEANUP;
            }
 
          /* Check all priorities within TCG have same properties */
          if (dcbif->tcg_priority_count [tcgid])
            {
              if (! nsm_dcb_same_pfc_state (dcbif, tcgid, usr_pri))
                {
                  ret = NSM_DCB_API_SET_ERR_DIFF_PFC;
                  goto CLEANUP;
                }
            }

          /* Add priority to TCG */
          dcbif->tcg_priority_table [usr_pri] = tcgid;
          dcbif->tcg_priority_count [tcgid]++;
        }
    }
#ifdef HAVE_HAL
  ret = hal_dcb_ets_add_pri_to_tcg (ifp->bridge_name, ifp->ifindex, tcgid, pri);
  if (ret < 0)
    {
      zlog_err (nzg, "Failed to add priorities in traffic-class-group %d"
                      "on the interface %s in hardware %d\n", 
                      tcgid, ifp->name, ret);
      ret = NSM_DCB_API_SET_ERR_HW_NO_SUPPORT;
      goto CLEANUP;
    }
#endif /* HAVE_HAL */

  NSM_DCB_FN_EXIT (NSM_DCB_API_SET_SUCCESS);

CLEANUP:
   for (usr_pri = 0; usr_pri < NSM_DCB_NUM_USER_PRI; usr_pri++)
    {
      if ((pri & (1 << usr_pri)))
        {
          if (dcbif->tcg_priority_table [usr_pri] == tcgid)
            { 
              if (keep_pri == usr_pri)
                NSM_DCB_FN_EXIT (ret);

              dcbif->tcg_priority_table [usr_pri] = NSM_DCB_ETS_DEFAULT_TCGID;
              dcbif->tcg_priority_count [tcgid]--;
            }
        }
    }

  NSM_DCB_FN_EXIT (ret);

}

/**@brief This function removes priority from traffic class group on interface.
   @param vr_id - Router Identifier
   @param ifname - Name of interface
   @param tcgid - Traffic-class-group identifier
   @param pri - priority value
   @return NSM_DCB_API_SET_SUCCESS
*/
s_int32_t 
nsm_dcb_remove_pri_from_tcg (u_int32_t vr_id, char *ifname, u_int8_t tcgid, 
                             u_int8_t pri)
{
  struct nsm_dcb_if *dcbif = NULL, tmp_dcbif;
  struct nsm_bridge_port *br_port;
  struct nsm_bridge *bridge;
  struct interface *ifp;
  struct nsm_master *nm;
  struct avl_node *node;
  struct nsm_dcb_bridge *dcbg;
  struct nsm_if *zif;
  u_int8_t usr_pri;
  u_int8_t remove_pri = 8; /* Initialize out of priority range <0-7> */
#ifdef HAVE_HAL
  s_int32_t ret;
#endif /* HAVE_HAL */
  
  NSM_DCB_FN_ENTER ();

  /* Look for NSM master */
  nm = nsm_master_lookup_by_id (nzg, vr_id);
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

  if (! CHECK_FLAG (dcbg->dcb_global_flags, NSM_DCB_ENABLE))
    NSM_DCB_FN_EXIT (NSM_DCB_API_SET_ERR_NO_DCB);

  if (! CHECK_FLAG (dcbg->dcb_global_flags, NSM_DCB_ETS_ENABLE))
    NSM_DCB_FN_EXIT (NSM_DCB_API_SET_ERR_NO_ETS);

  tmp_dcbif.ifp = ifp;
 
  /* Find the DCB interface */
  node = avl_search (dcbg->dcb_if_list, (void_t *) &tmp_dcbif);
  if (node)
    dcbif = (struct nsm_dcb_if *) node->info;

  if (! dcbif)
    NSM_DCB_FN_EXIT (NSM_DCB_API_SET_ERR_DCB_INTERFACE);
 
  /* Check ETS is Enabled on the interface */
  if (! CHECK_FLAG (dcbif->dcb_if_flags, NSM_DCB_IF_ETS_ENABLE))
    NSM_DCB_FN_EXIT (NSM_DCB_API_SET_ERR_INTERFACE_NO_ETS);

  for (usr_pri = 0; usr_pri < NSM_DCB_NUM_USER_PRI; usr_pri++)
    {
      if ((pri & (1 << usr_pri)))
        {
          /* Check priority belongs to different TCG */
          if (dcbif->tcg_priority_table [usr_pri] != tcgid ||
              dcbif->tcg_priority_table [usr_pri] == NSM_DCB_ETS_DEFAULT_TCGID)
            {
              ret =  NSM_DCB_API_SET_ERR_WRONG_TCG;
              remove_pri = usr_pri;
              goto CLEANUP;
            }

         /* Check Priority is already part of TCG */
         if (dcbif->tcg_priority_table [usr_pri] == tcgid)
           {  
             /* Remove priority from TCG and add it to TCG 15 */
             dcbif->tcg_priority_table [usr_pri] = NSM_DCB_ETS_DEFAULT_TCGID;
             dcbif->tcg_priority_count [tcgid]--;
             if (dcbif->tcg_priority_count [tcgid] == 0)
               NSM_DCB_FN_EXIT (NSM_DCB_API_SET_WARN_RECONFIG);
           }  
        }
    }

#ifdef HAVE_HAL
  ret = hal_dcb_ets_remove_pri_from_tcg (ifp->bridge_name, ifp->ifindex, 
                                         tcgid, pri);
  if (ret < 0)
    {
      zlog_err (nzg, "Failed to remove priorities in traffic-class-group %d"
                      "on the interface %s in hardware %d\n", 
                      tcgid, ifp->name, ret);
      ret =  NSM_DCB_API_SET_ERR_HW_NO_SUPPORT;
      goto CLEANUP;
    }
#endif /* HAVE_HAL */

   NSM_DCB_FN_EXIT (NSM_DCB_API_SET_SUCCESS);

CLEANUP:
   for (usr_pri = 0; usr_pri < NSM_DCB_NUM_USER_PRI; usr_pri++)
    {
      if ((pri & (1 << usr_pri)))
        {
          if (dcbif->tcg_priority_table [usr_pri] == NSM_DCB_ETS_DEFAULT_TCGID)
            {
              if (remove_pri == usr_pri)
                NSM_DCB_FN_EXIT (ret);

              dcbif->tcg_priority_table [usr_pri] = tcgid;
              dcbif->tcg_priority_count [tcgid]++;
            }
        }
    }

  NSM_DCB_FN_EXIT (ret);
}


/**@brief This function assigns percentage of bandwidth to each traffic-class
          group.
   @param vr_id - Router Identifier 
   @param ifname - Name of the interface
   @param bw - Array of bandwidth percentage
   @return NSM_DCB_API_SET_SUCCESS
*/
s_int32_t
nsm_dcb_assign_bw_percentage_to_tcg (u_int32_t vr_id, char *ifname, 
                                     u_int16_t *bw)
{
  struct nsm_dcb_if *dcbif = NULL, tmp_dcbif;
  struct nsm_bridge_port *br_port;
  struct nsm_bridge *bridge;
  struct interface *ifp;
  struct nsm_master *nm;
  struct avl_node *node;
  struct nsm_dcb_bridge *dcbg;
  struct nsm_if *zif;
  int i, total_bw = 0;
#ifdef HAVE_HAL
  s_int32_t ret;
#endif /* HAVE_HAL */
  
  NSM_DCB_FN_ENTER ();

  /* Look for NSM master */
  nm = nsm_master_lookup_by_id (nzg, vr_id);
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

  if (! CHECK_FLAG (dcbg->dcb_global_flags, NSM_DCB_ENABLE))
    NSM_DCB_FN_EXIT (NSM_DCB_API_SET_ERR_NO_DCB);

  if (! CHECK_FLAG (dcbg->dcb_global_flags, NSM_DCB_ETS_ENABLE))
    NSM_DCB_FN_EXIT (NSM_DCB_API_SET_ERR_NO_ETS);

  tmp_dcbif.ifp = ifp;
 
  /* Find the DCB interface */
  node = avl_search (dcbg->dcb_if_list, (void_t *) &tmp_dcbif);
  if (node)
    dcbif = (struct nsm_dcb_if *) node->info;

  if (! dcbif)
    NSM_DCB_FN_EXIT (NSM_DCB_API_SET_ERR_DCB_INTERFACE);
 
  /* Check ETS is Enabled on the interface */
  if (! CHECK_FLAG (dcbif->dcb_if_flags, NSM_DCB_IF_ETS_ENABLE))
    NSM_DCB_FN_EXIT (NSM_DCB_API_SET_ERR_INTERFACE_NO_ETS);

  for (i = 0; i < NSM_DCB_MAX_TCG_DEFAULT; i++)
    {
      total_bw = total_bw + bw[i];
      if (bw[i] != 0 && dcbif->tcg_priority_count[i] == 0)
        {
          zlog_err (nzg, "Traffic-class-group %d is not present\n", i);
          NSM_DCB_FN_EXIT (NSM_DCB_API_SET_ERR_INVALID_VALUE);
        }
    }
  
  if (total_bw != 100)
    { 
      zlog_err (nzg, "Total Bandwidth is %d\n",total_bw);
      NSM_DCB_FN_EXIT (NSM_DCB_API_SET_ERR_WRONG_BW);
    }

#ifdef HAVE_HAL
  ret = hal_dcb_ets_assign_bw_to_tcgs (ifp->bridge_name, ifp->ifindex, bw);
  if (ret < 0)
    {
      zlog_err (nzg, "Failed to assign bandwidth percentage" 
                      "on the interface %s in hardware %d\n", 
                      ifp->name, ret);
      NSM_DCB_FN_EXIT (NSM_DCB_API_SET_ERR_HW_NO_SUPPORT);
    }
#endif /* HAVE_HAL */

  pal_mem_cpy (dcbif->tcg_bandwidth_table, bw, 
               NSM_DCB_MAX_TCG_DEFAULT * sizeof (u_int16_t));

  NSM_DCB_FN_EXIT (NSM_DCB_API_SET_SUCCESS);
}

/**@brief This function removes all traffic-class-groups configured on
          interface.
   @param vr_id - Router Identifier
   @param ifname - Name of interface
   @return NSM_DCB_API_SET_SUCCESS
*/
s_int32_t
nsm_dcb_delete_tcgs (u_int32_t vr_id, char *ifname)
{
  struct nsm_dcb_if *dcbif = NULL, tmp_dcbif;
  struct nsm_bridge_port *br_port;
  struct nsm_bridge *bridge;
  struct interface *ifp;
  struct nsm_master *nm;
  struct avl_node *node;
  struct nsm_dcb_bridge *dcbg;
  struct nsm_if *zif;
  u_int8_t tcgid, priority_count_status;

  NSM_DCB_FN_ENTER ();

  /* Look for NSM master */
  nm = nsm_master_lookup_by_id (nzg, vr_id);
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

  if (! CHECK_FLAG (dcbg->dcb_global_flags, NSM_DCB_ENABLE))
    NSM_DCB_FN_EXIT (NSM_DCB_API_SET_ERR_NO_DCB);

  if (! CHECK_FLAG (dcbg->dcb_global_flags, NSM_DCB_ETS_ENABLE))
    NSM_DCB_FN_EXIT (NSM_DCB_API_SET_ERR_NO_ETS);

  tmp_dcbif.ifp = ifp;
 
  /* Find the DCB interface */
  node = avl_search (dcbg->dcb_if_list, (void_t *) &tmp_dcbif);
  if (node)
    dcbif = (struct nsm_dcb_if *) node->info;

  if (! dcbif)
    NSM_DCB_FN_EXIT (NSM_DCB_API_SET_ERR_DCB_INTERFACE);
 
  /* Check ETS is Enabled on the interface */
  if (! CHECK_FLAG (dcbif->dcb_if_flags, NSM_DCB_IF_ETS_ENABLE))
    NSM_DCB_FN_EXIT (NSM_DCB_API_SET_ERR_INTERFACE_NO_ETS);

  /* If traffic-class-group is not configured Return an error */
  priority_count_status = 0;
  for (tcgid = 0; tcgid < NSM_DCB_MAX_TCG_DEFAULT; tcgid++)
    {
      if (dcbif->tcg_priority_count[tcgid] == 0)
        priority_count_status++;
    }

  if (priority_count_status == NSM_DCB_MAX_TCG_DEFAULT)
    NSM_DCB_FN_EXIT (NSM_DCB_API_SET_ERR_ETS_NO_TCGS);

  pal_mem_set (&dcbif->tcg_priority_count[0], 0, 
               (sizeof (u_int8_t) * NSM_DCB_MAX_TCG_DEFAULT));
  
  /* By Defauly all priorities belongs to TCG 15 with no bandwidth limit */
  pal_mem_set (&dcbif->tcg_priority_table[0], NSM_DCB_ETS_DEFAULT_TCGID, 
               (sizeof (u_int8_t) * NSM_DCB_MAX_TCG_DEFAULT));
  pal_mem_set (&dcbif->tcg_bandwidth_table[0], 0, 
               (sizeof (u_int16_t) * NSM_DCB_MAX_TCG_DEFAULT));

  NSM_DCB_FN_EXIT (NSM_DCB_API_SET_SUCCESS);  
}

/**@brief This function sets the application Priority
   @param vr-id - Router identifier 
   @param ifname - Name of Interface 
   @param sel - Protocol Selector (ETHERTYPE/TCP/UDP/BOTH-TCP-UDP/
                                   NEITHER-TCP-UDP)
   @param proto_id - Port number or ethertype value
   @param pri - Priority Value
   @param type - mapped by service name or protocol value
   @return NSM_DCB_API_SET_SUCCESS
*/
s_int32_t
nsm_dcb_application_priority_set (u_int32_t vr_id, char *ifname, u_int8_t sel,
                                  u_int16_t proto_id, u_int8_t pri,
                                  enum nsm_dcb_appl_pri_mapping_type type)
{
  struct nsm_dcb_if *dcbif = NULL, tmp_dcbif;
  struct nsm_bridge_port *br_port;
  struct nsm_bridge *bridge;
  struct interface *ifp;
  struct nsm_master *nm;
  struct avl_node *node;
  struct nsm_if *zif;
  struct nsm_dcb_bridge *dcbg;
  struct nsm_dcb_application_priority *app_pri, tmp_app_pri;
  bool_t new = PAL_FALSE;
#ifdef HAVE_HAL
  s_int32_t ret;
#endif /* HAVE_HAL */
  
  NSM_DCB_FN_ENTER ();

  /* Look for NSM master */
  nm = nsm_master_lookup_by_id (nzg, vr_id);
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

  if (! CHECK_FLAG (dcbg->dcb_global_flags, NSM_DCB_ENABLE))
    NSM_DCB_FN_EXIT (NSM_DCB_API_SET_ERR_NO_DCB);

  if (! CHECK_FLAG (dcbg->dcb_global_flags, NSM_DCB_ETS_ENABLE))
    NSM_DCB_FN_EXIT (NSM_DCB_API_SET_ERR_NO_ETS);

  tmp_dcbif.ifp = ifp;
 
  /* Find the DCB interface */
  node = avl_search (dcbg->dcb_if_list, (void_t *) &tmp_dcbif);
  if (node)
    dcbif = (struct nsm_dcb_if *) node->info;

  if (! dcbif)
    NSM_DCB_FN_EXIT (NSM_DCB_API_SET_ERR_DCB_INTERFACE);
 
  /* Check DCB is Enabled on the interface */
  if (! CHECK_FLAG (dcbif->dcb_if_flags, NSM_DCB_IF_ENABLE))
    NSM_DCB_FN_EXIT (NSM_DCB_API_SET_ERR_INTERFACE_NO_DCB);

  tmp_app_pri.proto_id = proto_id;
  tmp_app_pri.sel = sel;

  node = avl_search (dcbif->dcb_appl_pri, (void_t *) &tmp_app_pri);
  if (node)
    app_pri = (struct nsm_dcb_application_priority *) node->info;
  else
    {
      /* Populate the application priority structure */
      app_pri = XCALLOC (MTYPE_NSM_DCB_APPL_PRI, 
                         sizeof (struct nsm_dcb_application_priority));
      new = PAL_TRUE;
    }

  if (! app_pri)
    NSM_DCB_FN_EXIT (NSM_DCB_API_SET_ERR_NO_MEM);

#ifdef HAVE_HAL
  ret = hal_dcb_ets_set_application_priority (ifp->bridge_name, ifp->ifindex, 
                                              sel, proto_id, pri);
  if (ret < 0)
    {
      zlog_err (nzg, "Failed to set application priority %d for protocol"
                      "value %d on interface %s erro %d\n", pri, proto_id, 
                      ifp->name, ret);
      NSM_DCB_FN_EXIT (NSM_DCB_API_SET_ERR_HW_NO_SUPPORT);
    }
#endif /* HAVE_HAL */

  pal_mem_set (app_pri, 0, sizeof (struct nsm_dcb_application_priority));

  app_pri->dcbif = dcbif;
  app_pri->sel = sel;
  app_pri->proto_id = proto_id;
  app_pri->priority = pri;
  app_pri->mapping_type = type;
 
  /* Insert into the tree */
  if (new)
    avl_insert (dcbif->dcb_appl_pri, app_pri);
  
  NSM_DCB_FN_EXIT (NSM_DCB_API_SET_SUCCESS);
}

/**@brief This function unsets the application Priority
   @param vr-id - Router identifier 
   @param ifname - Name of Interface 
   @param sel - Protocol Selector (ETHERTYPE/TCP/UDP/BOTH-TCP-UDP/
                                   NEITHER-TCP-UDP)
   @param proto_id - Port number or ethertype value
   @param pri - Priority Value
   @return NSM_DCB_API_SET_SUCCESS
*/
s_int32_t
nsm_dcb_application_priority_unset (u_int32_t vr_id, char *ifname, u_int8_t sel,
                                    u_int16_t proto_id, u_int8_t pri)
{
  struct nsm_dcb_if *dcbif = NULL, tmp_dcbif;
  struct nsm_bridge_port *br_port;
  struct nsm_bridge *bridge;
  struct interface *ifp;
  struct nsm_master *nm;
  struct avl_node *node;
  struct nsm_if *zif;
  struct nsm_dcb_bridge *dcbg;
  struct nsm_dcb_application_priority *app_pri, tmp_app_pri;
#ifdef HAVE_HAL
  s_int32_t ret;
#endif /* HAVE_HAL */
  
  NSM_DCB_FN_ENTER ();

  /* Look for NSM master */
  nm = nsm_master_lookup_by_id (nzg, vr_id);
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
    {
       zlog_err (nzg, "Global DCB structure not found\n");
       NSM_DCB_FN_EXIT (NSM_DCB_API_SET_ERR_NO_DCBG);
    }

  if (! CHECK_FLAG (dcbg->dcb_global_flags, NSM_DCB_ENABLE))
    NSM_DCB_FN_EXIT (NSM_DCB_API_SET_ERR_NO_DCB);

  if (! CHECK_FLAG (dcbg->dcb_global_flags, NSM_DCB_ETS_ENABLE))
    NSM_DCB_FN_EXIT (NSM_DCB_API_SET_ERR_NO_ETS);

  tmp_dcbif.ifp = ifp;
 
  /* Find the DCB interface */
  node = avl_search (dcbg->dcb_if_list, (void_t *) &tmp_dcbif);
  if (node)
    dcbif = (struct nsm_dcb_if *) node->info;

  if (! dcbif)
    NSM_DCB_FN_EXIT (NSM_DCB_API_SET_ERR_DCB_INTERFACE);
 
  /* Check DCB is Enabled on the interface */
  if (! CHECK_FLAG (dcbif->dcb_if_flags, NSM_DCB_IF_ENABLE))
    NSM_DCB_FN_EXIT (NSM_DCB_API_SET_ERR_INTERFACE_NO_DCB);

#ifdef HAVE_HAL
  ret = hal_dcb_ets_unset_application_priority (ifp->bridge_name, ifp->ifindex, 
                                                sel, proto_id, pri);
  if (ret < 0)
    {
      zlog_err (nzg, "Failed to unset application priority %d for protocol"
                      "value %d on interface %s erro %d\n", pri, proto_id, 
                      ifp->name, ret);
       NSM_DCB_FN_EXIT (NSM_DCB_API_SET_ERR_HW_NO_SUPPORT);
    }
#endif /* HAVE_HAL */

  tmp_app_pri.proto_id = proto_id;
  tmp_app_pri.sel = sel;

  node = avl_search (dcbif->dcb_appl_pri, (void_t *) &tmp_app_pri);

  if (node)
    {
      app_pri = (struct nsm_dcb_application_priority *)node->info;
      if (app_pri && app_pri->priority == pri)
        {
           avl_remove (dcbif->dcb_appl_pri, (void_t *) &tmp_app_pri);
           XFREE (MTYPE_NSM_DCB_APPL_PRI, app_pri);
        }
      else 
        NSM_DCB_FN_EXIT (NSM_DCB_API_SET_ERR_APP_PRI_NOT_FOUND);
    }
  else
    NSM_DCB_FN_EXIT (NSM_DCB_API_SET_ERR_APP_PRI_NOT_FOUND);
  
  NSM_DCB_FN_EXIT (NSM_DCB_API_SET_SUCCESS);
}

#endif /* HAVE_DCB*/
