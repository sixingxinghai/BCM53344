/* Copyright (C) 2003  All Rights Reserved.

   Per Vlan Spanning Tree Protocol Apis
   This file defines function prototypes and structures used for RPVST
    
*/

#ifndef __PACOS_MSTP_RPVST_H
#define __PACOS_MSTP_RPSVT_H

#define MSTP_RPVST_VLAN_TLV_TYPE  0
#define MSTP_RPVST_VLAN_TLV_LEN   2

int 
mstp_tx_rpvst_resp(struct mstp_port *port);

int
mstp_tx_rpvst(struct mstp_port *port);

int
rpvst_built_cist(struct mstp_port *port);

void
mstp_rpvst_encode_vlan_tlv(u_int16_t vid, struct rpvst_vlan_tlv *tlv);

void
mstp_rpvst_add_vlan_tlv(u_char *buf, struct rpvst_vlan_tlv *tlv);

void
mstp_rpvst_plus_parse_bpdu(u_char *buf, struct mstp_bpdu *bpdu, 
                           int len, unsigned char admin_cisco, 
                           struct mstp_port *port);

void
mstp_process_rpvst_plus_bpdu(struct mstp_port *port, struct mstp_bpdu *bpdu);

int
mstp_rpvst_send(struct mstp_port *port, unsigned char *data, int length);

void
hexdump(unsigned char *buf, int nbytes);

int
rpvst_plus_api_add_vlan(char *bridge_name, int vid);

void 
rpvst_plus_bridge_vlan_config_add (char *bridge_name, u_int16_t vid);

int
rpvst_plus_api_vlan_delete (char *bridge_name, int vid);

int
rpvst_plus_api_set_msti_bridge_priority(char *name,
                                        int vid, u_int32_t priority);

int
rpvst_plus_api_set_msti_port_priority (char *name,
                                 char *ifName,  int vid,
                                 s_int16_t priority);
int
rpvst_plus_api_set_msti_port_path_cost (char *name,
                                        char *ifName, int vid,
                                        u_int32_t cost);

int
rpvst_plus_api_add_port(char * name, char *ifname, u_int16_t svid,
                        u_int8_t spanning_tree_disable);

int
rpvst_plus_delete_port(char *br_name, char *name, u_int16_t svid,
                       int force, bool_t flag);

int
rpvst_plus_api_set_msti_vlan_restricted_role(char *br_name, char *name,
                                             int vid, bool_t flag);

int
rpvst_plus_api_set_msti_vlan_restricted_tcn(char *br_name, char *name,
                                            int vid, bool_t flag);

int
rpvst_plus_api_set_msti_port_priority (char *name,
                                       char *ifName, int vid,
                                       s_int16_t priority);

void mstp_br_config_vlan_add_port (char *br_name, struct interface *ifp,
                                   u_int16_t vid);
void 
mstp_bridge_vlan_config_delete (char *bridge_name,
                                     mstp_vid_t vid);
#endif /* __PACOS_MSTP_RPVST_H */
