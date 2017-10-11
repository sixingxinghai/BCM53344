/* Copyright (C) 2001-2003  All Rights Reserved. */

#ifndef _IMI_NSM_H
#define _IMI_NSM_H

struct imi_interface
{
  /* List of imi_rule per interface. */
  struct list *rule_list;

  /* DHCP client, if enabled on this interface. */
  struct imi_dhcp_client *idc;

  /* Scope. */
  u_int8_t scope;
#define NSM_IF_SCOPE_UNSPEC       0
#define NSM_IF_SCOPE_INSIDE       1
#define NSM_IF_SCOPE_OUTSIDE      2

  /* Rule reference count. */
  int rule_ref_cnt;
};


/* Create interface. */
struct interface *imi_if_create (char *);

/* Init. */
void imi_if_init (struct lib_globals *zg);

/* config-write interface scope. */
int imi_interface_scope_config_write (struct cli *, struct interface *);

/* Convert line into ifp. */
struct interface *imi_interface_line2ifp (char *);

/* Install or remove interface NAT translation rules from the kernel. */
int imi_if_process_nat_pool_list (struct interface *ifp, u_int8_t if_nscope, int op);

/* For Installing Static NAT Rules. */
int imi_if_process_nat_static (struct interface *ifp, u_int8_t if_nscope, int op);
#endif /* _IMI_NSM_H */
