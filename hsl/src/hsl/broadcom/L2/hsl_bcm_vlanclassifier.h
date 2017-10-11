/* Copyright (C) 2004  All Rights Reserved. */

#ifndef _HSL_VLAN_CLASSIFIER_H_
#define _HSL_VLAN_CLASSIFIER_H_

#ifdef HAVE_VLAN_CLASS

struct hsl_bcm_proto_vlan_class_entry {
  u_int16_t vlan_id;
  u_int16_t ether_type;
  u_int16_t encaps;
  struct hsl_avl_tree *port_tree; /* store the interfaces attached to this
                                     rule */  
};

int hsl_bcm_vlan_proto_classifier_add (u_int16_t vlan_id,
                                       u_int16_t ether_type,
                                       u_int32_t encaps,
                                       u_int32_t ifindex,
                                       u_int32_t refcount);

int hsl_bcm_vlan_mac_classifier_add (u_int16_t vlan_id, u_char *mac, u_int32_t ifindex, u_int32_t refcount);

int hsl_bcm_vlan_ipv4_classifier_add (u_int16_t vlan_id,
                                      u_int32_t addr,
                                      u_int32_t masklen,
                                      u_int32_t ifindex,
                                      u_int32_t refcount);

int hsl_bcm_vlan_proto_classifier_delete (u_int16_t vlan_id,
                                          u_int16_t ether_type,
                                          u_int32_t encaps,
                                          u_int32_t ifindex,
                                          u_int32_t refcount);

int hsl_bcm_vlan_mac_classifier_delete (u_int16_t vlan_id,u_char *mac,
                                        u_int32_t ifindex, u_int32_t refcount);
int hsl_bcm_vlan_ipv4_classifier_delete (u_int16_t vlan_id,
                                         u_int32_t addr,
                                         u_int32_t masklen,
                                         u_int32_t ifindex,
                                         u_int32_t refcount);

#endif /* HAVE_VLAN_CLASS */

#endif /* _HSL_VLAN_CLASSIFIER_H_ */
