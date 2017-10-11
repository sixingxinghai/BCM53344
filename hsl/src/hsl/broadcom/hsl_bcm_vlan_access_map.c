#include "config.h"

#ifdef HAVE_VLAN
#ifdef HAVE_L2LERN
#include "hsl_os.h"

#include "hsl_types.h"

#include "hal_types.h"
#ifdef HAVE_L2
#include "hal_l2.h"
#endif /* HAVE_L2 */

#include "hal_msg.h"

#include "hsl_avl.h"
#include "hsl_error.h"
#include "bcm_incl.h"
#include "hsl_types.h"
#include "hsl_oss.h"
#include "hsl_logger.h"
#include "hsl_ifmgr.h"
#include "hsl_vlan.h"
#include "hsl_bridge.h"
#include "hsl_if_hw.h"
#include "hsl_bcm.h"
#include "hsl_bcm_if.h"

struct hsl_vlan_access_map *
hsl_bcm_vlan_access_map_add (struct hal_msg_vlan_set_access_map *msg)
{
  struct hsl_vlan_access_map *hsl_vacc_map;
  struct hal_mac_access_grp *hal_macc_grp = &msg->hal_macc_grp;

  hsl_vacc_map = oss_malloc (sizeof (struct hsl_vlan_access_map), OSS_MEM_HEAP);


  hsl_vacc_map->deny_permit = hal_macc_grp->deny_permit;
  hsl_vacc_map->l2_type = hal_macc_grp->l2_type;
  strcpy(hsl_vacc_map->name, hal_macc_grp->name);
  strcpy(hsl_vacc_map->vlan_map, hal_macc_grp->vlan_map);
  memcpy (&hsl_vacc_map->a, &hal_macc_grp->a, sizeof (struct hsl_acl_mac_addr));
  memcpy (&hsl_vacc_map->a_mask, &hal_macc_grp->a_mask, sizeof (struct hsl_acl_mac_addr));
  memcpy (&hsl_vacc_map->m, &hal_macc_grp->m, sizeof (struct hsl_acl_mac_addr));
  memcpy (&hsl_vacc_map->m_mask, &hal_macc_grp->m_mask, sizeof (struct hsl_acl_mac_addr));

  return hsl_vacc_map;

}

int
hsl_bcm_vlan_access_map_del ( struct hsl_vlan_access_map *hsl_vacc_map)
{
  oss_free (hsl_vacc_map, OSS_MEM_HEAP);
  return 0;
}


int
hsl_bcm_vlan_set_access_map (struct hal_msg_vlan_set_access_map *msg)
{
  struct hsl_bridge *bridge;
  struct hsl_vlan_port *v, tv;
  struct hsl_avl_node *node;
  hsl_vid_t vid = msg->vid;

  HSL_FN_ENTER ();
  HSL_BRIDGE_LOCK;
  bridge = p_hsl_bridge_master->bridge;

 if (! bridge)
    {
      HSL_BRIDGE_UNLOCK;
      return -1;
    }

  tv.vid = vid;
  node = hsl_avl_lookup (bridge->vlan_tree, &tv);
  if (! node)
    {
      HSL_BRIDGE_UNLOCK;
      return -1;
    }

  v = (struct hsl_vlan_port *) HSL_AVL_NODE_INFO (node);
  if (! v )
    {
      HSL_BRIDGE_UNLOCK;
      return -1;
    }

  if ( msg->action == HSL_VLAN_ACC_MAP_ADD)
    {
      if (v->hsl_vacc_map)
        {
          HSL_BRIDGE_UNLOCK;
          return -1;
        }
      else
        v->hsl_vacc_map = hsl_bcm_vlan_access_map_add (msg);
    }
  else 
   {
     /*Add the vlan access map delete code*/
     if (! v->hsl_vacc_map)
        {
          HSL_BRIDGE_UNLOCK;
          return -1;
        }
      else
        {
          hsl_bcm_vlan_access_map_del (v->hsl_vacc_map);
          v->hsl_vacc_map = NULL;
        }
   }

  HSL_BRIDGE_UNLOCK;
  return 0;
}
#endif /* HAVE_L2LERN */
#endif /* HAVE_VLAN */
