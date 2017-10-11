/* Copyright (C) 2008-2009  All Rights Reserved. */

#ifndef _PACOS_OSPF_MLINK_H
#define _PACOS_OSPF_MLINK_H

/* Table definition. */
#define OSPF_MULTI_AREA_IF_TABLE_DEPTH               8

/* OSPF MULTI AREA LINK */
struct ospf_multi_area_link
{
  /* Description. */
  char *ifname;

  /* area for this multi-area-adjacency-link. */
  struct pal_in4_addr area_id;

  /* Neighbor address for the multi-area-link */
  struct pal_in4_addr nbr_addr;

  /* Area ID format. */
  u_char format;

 /* Interface data structure for the Multi-area-link. */
  struct ospf_interface *oi; 
};
struct ospf_multi_area_link *
ospf_multi_area_link_get (struct ospf *top, struct pal_in4_addr area_id,
                               u_char *ifname, struct pal_in4_addr nh_addr,
                               u_char format);
struct ospf_multi_area_link * ospf_multi_area_link_entry_lookup (struct ospf *, 
                                                     struct pal_in4_addr ,
                                                     struct pal_in4_addr );
void ospf_multi_area_link_delete (struct ospf *,
                                  struct ospf_multi_area_link *);

void ospf_multi_area_link_delete_all (struct ospf *, struct pal_in4_addr,
                                      u_char *);
void ospf_multi_area_if_entries_del_corr_to_primary_adj (
                                                   struct ospf_interface *);
s_int32_t ospf_multi_area_if_entry_delete (struct ospf *, struct pal_in4_addr, 
                                           struct pal_in4_addr);
void ospf_multi_area_link_up_corr_to_primary (struct ospf *, 
                                              struct connected *);
s_int32_t ospf_multi_area_if_up (struct ospf_interface *);

#endif /* _PACOS_OSPF_MLINK_H */
