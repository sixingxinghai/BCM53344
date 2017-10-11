/* Copyright (C) 2004  All Rights Reserved */

#ifndef __NSM_MAC_ACL_API_H__
#define __NSM_MAC_ACL_API_H__

int compare_mac_filter (u_int8_t *start_addr, u_int8_t val);
#ifdef HAVE_VLAN
int check_interface_vlan_filter ( struct interface *ifp);
#endif
int nsm_mac_hal_set_access_grp(struct mac_acl *access,
                               int ifindex,
                               int action,
                               int dir);
int mac_acl_extended_set (struct nsm_mac_acl_master *master,
                          char *name_str,
                          int type,
                          char *addr_str,
                          char *addr_mask_str,
                          char *mask_str,
                          char *mask_mask_str,
                          int extended,
                          int set,
                          int acl_type,
                          int packet_format);
result_t mac_acl_show_name (struct cli *cli, struct nsm_mac_acl_master *master, char *name);
result_t mac_acl_show (struct cli *cli, struct nsm_mac_acl_master *master);
result_t mac_access_grp_show (struct cli *cli, struct nsm_master *nm);

#endif /* __NSM_MAC_ACL_API_H__ */
