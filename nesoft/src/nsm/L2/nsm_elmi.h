/**@file nsm_elmi.h
 ** @brief  This file contains the messaging part for NSM to ELMI.
 **/
/* Copyright (C) 2010  All Rights Reserved. */

#ifndef _PACOS_NSM_ELMI_H
#define _PACOS_NSM_ELMI_H


#ifdef HAVE_ELMID
#include "nsm_vlan.h"
#include "nsm_pro_vlan.h"

#define NSM_ELMI_SVLAN_P2P     0
#define NSM_ELMI_SVLAN_M2M     1

int nsm_elmi_set_server_callback (struct nsm_server *);

int
nsm_elmi_status_msg_process (struct nsm_msg_header *header,
                             void *arg, void *message);

int
nsm_elmi_recv_auto_vlan_add_msg (struct nsm_msg_header *header,
                                 void *arg, void *message);
int
nsm_elmi_recv_auto_vlan_del_msg (struct nsm_msg_header *header,
                                 void *arg, void *message);

int
nsm_evc_send_evc_add_msg (struct nsm_msg_elmi_evc *msg,
                          int msgid);
void
nsm_server_evc_delete (struct interface *ifp, u_int16_t svid);

void
nsm_server_uni_update (struct nsm_bridge_port *br_port, struct interface *ifp,
                       u_int32_t cindex);
int
nsm_elmi_send_uni_add (struct nsm_bridge * bridge, struct interface *ifp);

void
nsm_server_uni_delete (struct interface *ifp);

int
nsm_elmi_send_evc_add (struct nsm_bridge *br, struct interface *ifp, 
                       u_int16_t svid, struct nsm_svlan_reg_info *svlan_info);

void
nsm_server_evc_update (struct interface *ifp, u_int16_t evc_id,
                       struct nsm_svlan_reg_info *svlan_info, 
                       cindex_t cindex);
s_int32_t
nsm_server_send_evc_update (struct nsm_server_entry *nse,
                            struct interface *ifp, u_int16_t evc_id,
                            struct nsm_svlan_reg_info *svlan_info,
                            cindex_t cindex);
s_int32_t
nsm_server_elmi_send_bw (struct nsm_bridge_port *br_port,
                         struct nsm_msg_bw_profile *msg, int msgid);
#endif /* HAVE_ELMID */

#endif /* _PACOS_NSM_ELMI_H */

