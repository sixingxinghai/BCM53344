/* Copyright (C) 2004  All Rights Reserved.  */

#ifndef _NSM_VLAN_ACCESS_CLI_H_
#define _NSM_VLAN_ACCESS_CLI_H

enum vlan_access_action_type
  {
    VLAN_ACCESS_ACTION_FORWARD,
    VLAN_ACCESS_ACTION_DISCARD
  };

struct nsm_vlan_access_list
{
  struct nsm_vlan_access_master *master;

  /* cmap-list linked list */
  struct nsm_vlan_access_list *next;
  struct nsm_vlan_access_list *prev;

  /* cmap-list's attributes */
  char *name;
  char *acl_name;

  enum vlan_access_action_type type;
};

struct nsm_vlan_access_master
{
  struct nsm_vlan_access_list *head;
  struct nsm_vlan_access_list *tail;
};

struct nsm_vlan_access_list * nsm_vmap_list_new ();
int nsm_vmap_list_master_init (struct nsm_master *nm);
struct nsm_vlan_access_list *
nsm_vmap_list_lookup (struct nsm_vlan_access_master *master,
                      const char *vmap_name);
struct nsm_vlan_access_list *
nsm_vmap_list_insert (struct nsm_vlan_access_master *master,
                      const char *vmap_name,
                      const char *acl_name);
void nsm_vmap_list_delete (struct nsm_vlan_access_list *vmapl);
struct nsm_vlan_access_list *
nsm_vmap_list_get (struct nsm_vlan_access_master *master,
                   const char *vmap_name);
int nsm_insert_mac_acl_name_into_vmap (struct nsm_vlan_access_list *vmapl,
                                       const char *acl_name);
int nsm_delete_mac_acl_name_from_vmap (struct nsm_vlan_access_list *vmapl,
                                       const char *acl_name);

#endif /* _NSM_VLAN_CLI_H */
