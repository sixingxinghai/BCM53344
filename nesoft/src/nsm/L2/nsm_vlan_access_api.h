/* Copyright (C) 2004  All Rights Reserved */

#ifndef __NSM_VLAN_ACCESS_API_H__
#define __NSM_VLAN_ACCESS_API_H__

int check_bridge_mac_acl ( struct nsm_master *nm);
void
nsm_set_vmapl_vlan ( struct nsm_master *nm, struct nsm_vlan_access_list *vmapl,
                     nsm_vid_t vid);
void
nsm_unset_vmapl_vlan ( struct nsm_master *nm, struct nsm_vlan_access_list *vmapl,
                     nsm_vid_t vid);
int nsm_vlan_hal_set_access_map ( struct nsm_master *nm,
                                  struct nsm_vlan_access_list *vmapl,
                                  nsm_vid_t vid,
                                  int action);
result_t
vlan_access_map_show (struct cli *cli, struct nsm_vlan_access_master *master);
result_t vlan_filter_show (struct cli *cli, struct nsm_master *nm);



#endif /* __NSM_VLAN_ACCESS_API_H__ */
