#ifndef __PACOS_BR_PRO_VLAN_H__
#define __PACOS_BR_PRO_VLAN_H__

#include "if_ipifwd.h"
#include "br_types.h"

int br_vlan_reg_tab_ent_cmp (void *data1, void *data2);

int br_vlan_trans_tab_ent_cmp (void *data1, void *data2);

int br_vlan_rev_trans_tab_ent_cmp (void *data1, void *data2);

int br_vlan_trans_tab_entry_add (char *bridge_name, unsigned int port_no,
                                 vid_t vid, vid_t trans_vid);

int br_vlan_trans_tab_entry_delete (char *bridge_name, unsigned int port_no,
                                    vid_t vid, vid_t trans_vid);

int br_vlan_trans_tab_delete (struct apn_bridge_port *port);

int
br_vlan_reg_tab_entry_add (char *bridge_name, unsigned int port_no,
                           vid_t cvid, vid_t svid);

int
br_vlan_reg_tab_entry_delete (char *bridge_name, unsigned int port_no,
                              vid_t cvid, vid_t svid);

int
br_vlan_reg_tab_delete (struct apn_bridge_port *port);

int br_cvlan_trans_tab_delete (struct apn_bridge_port *port);

vid_t br_vlan_svid_get (struct apn_bridge_port *port, vid_t cvid);

unsigned char br_cvlan_state_flags_get (struct apn_bridge_port *port,
                                        vid_t cvid);

int
br_vlan_pro_edge_port_tab_ent_cmp (void *data1, void *data2);

int
br_vlan_add_pro_edge_port (char *bridge_name, unsigned int port_no,
                           vid_t svid);

int
br_vlan_del_pro_edge_port (char *bridge_name, unsigned int port_no,
                           vid_t svid);

struct br_vlan_pro_edge_port *
br_vlan_pro_edge_port_lookup (struct apn_bridge_port *port, vid_t cvid);

int
br_vlan_pro_edge_tab_delete (struct apn_bridge_port *port);

int
br_vlan_set_pro_edge_pvid (char *bridge_name, unsigned int port_no,
                           vid_t svid, vid_t pvid);

int
br_vlan_set_pro_edge_untagged_vid (char *bridge_name, unsigned int port_no,
                                   vid_t svid, vid_t untagged_vid);

struct sk_buff *
br_handle_provider_edge_config (struct sk_buff *skb, struct apn_bridge_port *port,
                                vid_t cvid, vid_t svid);

void
br_vlan_translate_ingress_vid (struct apn_bridge_port *port,
                               struct sk_buff *skb);

void
br_vlan_translate_egress_vid (struct apn_bridge_port *port,
                              struct sk_buff *skb);

#endif
