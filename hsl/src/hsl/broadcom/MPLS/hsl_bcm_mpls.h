/* Copyright (C) 2005  All Rights Reserved. */

#ifndef _HSL_BCM_MPLS_H_
#define _HSL_BCM_MPLS_H_

#define HSL_BCM_MPLS_VPN_MAX   255

int hsl_hw_mpls_init ();
int hsl_hw_mpls_deinit ();
int hsl_bcm_mpls_ftn_add_to_hw (struct hsl_route_node *, struct hsl_nh_entry *);

int hsl_bcm_mpls_vpn_add_to_hw (struct hsl_mpls_vpn_entry *);
int hsl_bcm_mpls_vpn_del_from_hw (struct hsl_mpls_vpn_entry *);
int hsl_bcm_mpls_vpn_if_bind (struct hsl_mpls_vpn_entry *, 
                              struct hsl_mpls_vpn_port *);
int hsl_bcm_mpls_vpn_if_unbind (struct hsl_mpls_vpn_entry *, 
                                struct hsl_mpls_vpn_port *);

/*
  Register HW callbacks 
*/
int
hsl_mpls_hw_cb_register (struct hsl_mpls_hw_callbacks *cb);

#endif /* _HSL_BCM_MPLS_H_ */
