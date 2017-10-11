/* Copyright (C) 2004  All Rights Reserved.  */

#include "pal.h"

#ifdef HAVE_VLAN
#ifdef HAVE_L2LERN
#include "cli.h"
#include "avl_tree.h"
#include "table.h"
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

#ifdef HAVE_HAL
#include "hal_incl.h"
#include "hal_acl.h"
#endif

int
check_bridge_mac_acl ( struct nsm_master *nm)
{
  struct nsm_bridge_port *br_port = NULL;
  struct nsm_bridge *bridge = NULL;
  struct avl_node *avl_port = NULL;
  struct nsm_if *zif = NULL;

  bridge = nsm_get_first_bridge ( nm->bridge);

  if(! bridge)
   return 1;


  for (avl_port = avl_top (bridge->port_tree); avl_port;
       avl_port = avl_next (avl_port))
    {
      if ((br_port = (struct nsm_bridge_port *)
                             AVL_NODE_INFO (avl_port)) == NULL)
        continue;

      if ((zif = br_port->nsmif) == NULL)
        continue;

      if (! zif || zif->mac_acl)
        return 1;
    }

  return 0;
}

void
nsm_set_vmapl_vlan ( struct nsm_master *nm, struct nsm_vlan_access_list *vmapl,
                     nsm_vid_t vid)
{
  struct nsm_vlan p;
  struct avl_node *rn = NULL;
  struct nsm_vlan *vlan;
  struct nsm_bridge *bridge = NULL;

  NSM_VLAN_SET(&p, vid);
  bridge = nsm_get_first_bridge ( nm->bridge);

  if (bridge != NULL)
    rn = avl_search (bridge->vlan_table, (void *)&p);

  if (rn != NULL)
    {
      vlan = rn->info;
      vlan->vlan_filter = vmapl; 
    }
}

void
nsm_unset_vmapl_vlan ( struct nsm_master *nm, struct nsm_vlan_access_list *vmapl,
                     nsm_vid_t vid)
{
  struct nsm_vlan p;
  struct avl_node *rn;
  struct nsm_vlan *vlan;
  struct nsm_bridge *bridge = NULL;

  NSM_VLAN_SET(&p, vid);
  bridge = nsm_get_first_bridge ( nm->bridge);
  if (bridge == NULL)
    return;

  rn = avl_search (bridge->vlan_table, (void *)&p);
  if (rn)
    {
      vlan = rn->info;
      if (vlan->vlan_filter)
        vlan->vlan_filter = NULL;
    }

}


int
nsm_vlan_hal_set_access_map ( struct nsm_master *nm,
                              struct nsm_vlan_access_list *vmapl,
                              nsm_vid_t vid,
                              int action)
{
  struct hal_mac_access_grp hal_macc_grp;
  struct mac_acl *access;

  unsigned char *dst_mac_addr;
  unsigned char *src_mac_addr;
  unsigned char *deny_permit;
  unsigned char *packet_format;
  char *src_name;
  char *dst_name;
  int ret;

  access = mac_acl_lookup ( nm->mac_acl, vmapl->acl_name);

  if (access == NULL)
    return -1;

  /* Clear hal_cmap */
  memset (&hal_macc_grp, 0, sizeof (struct hal_mac_access_grp));

  deny_permit = &hal_macc_grp.deny_permit;
  *deny_permit = vmapl->type;

  dst_name = hal_macc_grp.vlan_map;
  src_name = vmapl->name;

  memcpy (dst_name, src_name, pal_strlen(vmapl->name));
  
  dst_name = hal_macc_grp.name;
  src_name = access->name;
  //pal_strcpy(dst_name,src_name);
  memcpy (dst_name, src_name, strlen (access->name));

  packet_format = &hal_macc_grp.l2_type;
  *packet_format = access->packet_format;

      /* Copy source MAC address */
  dst_mac_addr = (unsigned char *)&hal_macc_grp.a;
  src_mac_addr = (unsigned char *)&access->a;
  memcpy (dst_mac_addr, src_mac_addr, sizeof (struct hal_acl_mac_addr));

      /* Copy source MAC mask */
  dst_mac_addr = (unsigned char *)&hal_macc_grp.a_mask;
  src_mac_addr = (unsigned char *)&access->a_mask;
  memcpy (dst_mac_addr, src_mac_addr, sizeof (struct hal_acl_mac_addr));

      /* Copy destination MAC address */
  dst_mac_addr = (unsigned char *)&hal_macc_grp.m;
  src_mac_addr = (unsigned char *)&access->m;
  memcpy (dst_mac_addr, src_mac_addr, sizeof (struct hal_acl_mac_addr));

      /* Copy destination MAC mask */
  dst_mac_addr = (unsigned char *)&hal_macc_grp.m_mask;
  src_mac_addr = (unsigned char *)&access->m_mask;
  memcpy (dst_mac_addr, src_mac_addr, sizeof (struct hal_acl_mac_addr));

        /* Send class-map data */
   ret = hal_vlan_set_access_map (&hal_macc_grp, vid, action);

   if (ret < 0)
     return ret;
   else
     /* Clear hal_cmap */
     memset (&hal_macc_grp, 0, sizeof (struct hal_mac_access_grp));

  return HAL_SUCCESS;
}

static const char *
vlan_access_map_type_str (struct nsm_vlan_access_list *vmapl)
{
  switch (vmapl->type)
    {
    case VLAN_ACCESS_ACTION_FORWARD:
      return "Forward";
    case VLAN_ACCESS_ACTION_DISCARD:
      return "Discard";
    default:
      return "";
    }
}


result_t
vlan_access_map_show (struct cli *cli, struct nsm_vlan_access_master *master)
{
  struct nsm_vlan_access_list *vmapl = NULL;

  if (master == NULL)
    return 0;

  for (vmapl = master->head; vmapl; vmapl = vmapl->next)
    {
      if(! vmapl->name)
        continue;

      cli_out (cli, " VLAN-ACCESS-MAP: %s \n", vmapl->name);
      cli_out (cli, "  %s", vlan_access_map_type_str (vmapl));
      if (vmapl->acl_name)
        cli_out (cli, "  %s", vmapl->acl_name);
      else
        cli_out (cli, "No match mac acl");
      cli_out (cli, "\n");
    }
  return 0;
}

result_t
vlan_filter_show (struct cli *cli, struct nsm_master *nm)
{
  struct nsm_bridge *bridge = NULL;
  struct nsm_vlan_access_list *vmapl = NULL;
  struct avl_node *rn;
  struct nsm_vlan *vlan;

  bridge = nsm_get_first_bridge ( nm->bridge);

  if(! bridge)
   return 1;
  if ( (bridge->type != NSM_BRIDGE_TYPE_STP_VLANAWARE) &&
       (bridge->type != NSM_BRIDGE_TYPE_RSTP_VLANAWARE) &&
       (bridge->type != NSM_BRIDGE_TYPE_RPVST_PLUS) &&
       (bridge->type != NSM_BRIDGE_TYPE_MSTP) )
    {
      return 1;
    }

  if (bridge->vlan_table != NULL)
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
                cli_out (cli, "Vlan Filter %s is applied to vlan %d\n", vmapl->name, vlan->vid);
              }
          }
      }
  return 0;
}
#endif /* HAVE_L2LERN */
#endif /* HAVE_VLAN */
