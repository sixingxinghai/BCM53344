/**@file elmi_port.h
 ** @brief This file contains the prototypes for ELMI protocol port related APIs
 ***/
/* Copyright (C) 2010  All Rights Reserved. */

#ifndef _PACOS_ELMI_PORT_H
#define _PACOS_ELMI_PORT_H

struct elmi_ifp *
elmi_interface_new (struct interface *ifp);

s_int32_t
elmi_interface_add (struct interface *ifp);

s_int32_t 
nsm_util_link_update (struct nsm_msg_header *, void *, void *);

s_int32_t
elmi_enable_port (struct elmi_bridge *br, struct interface *ifp);

s_int32_t
elmi_disable_port (u_int32_t ifindex);

s_int32_t
elmi_delete_port (struct elmi_ifp *elmi_if);

s_int32_t
elmi_nsm_evc_add (struct elmi_ifp *elmi_if, struct nsm_msg_elmi_evc *msg);

s_int32_t
elmi_nsm_evc_update (struct elmi_ifp *elmi_if, struct nsm_msg_elmi_evc *msg);

s_int32_t
elmi_nsm_evc_delete (struct elmi_ifp *elmi_if, struct nsm_msg_elmi_evc *msg);

s_int32_t
elmi_nsm_uni_update (struct elmi_ifp *elmi_if, struct nsm_msg_uni *msg);

s_int32_t
elmi_nsm_uni_add (struct elmi_ifp *elmi_if, struct nsm_msg_uni *msg);

s_int32_t
elmi_nsm_uni_delete (struct elmi_ifp *elmi_if);

int
elmi_nsm_uni_bw_add (struct elmi_ifp *elmi_if, 
                     struct nsm_msg_bw_profile *msg);

int
elmi_nsm_uni_bw_del (struct elmi_ifp *elmi_if, 
                     struct nsm_msg_bw_profile *msg);

int
elmi_nsm_evc_bw_add (struct elmi_ifp *elmi_if, 
                     struct nsm_msg_bw_profile *msg);

int
elmi_nsm_evc_bw_del (struct elmi_ifp *elmi_if, 
                     struct nsm_msg_bw_profile *msg);

int
elmi_nsm_evc_cos_bw_add (struct elmi_ifp *elmi_if, 
                         struct nsm_msg_bw_profile *msg);

int
elmi_nsm_evc_cos_bw_del (struct elmi_ifp *elmi_if, 
                         struct nsm_msg_bw_profile *msg);

struct elmi_cvlan_evc_map *
elmi_lookup_cevlan_evc_map (struct elmi_ifp *elmi_if, u_int16_t evc_ref_id);

struct elmi_evc_status *
elmi_evc_look_up (struct elmi_ifp *elmi_if, u_int16_t evc_ref_id);

struct elmi_evc_status *
elmi_lookup_evc_by_name (struct elmi_ifp *elmi_if, u_int8_t *evc_name);

void
elmi_unic_clear_learnt_database (struct elmi_ifp *elmi_if);

struct elmi_evc_status *
elmi_evc_new (void);

void
elmi_unic_delete_evc (struct elmi_ifp *elmi_if, struct elmi_evc_status *evc);

struct elmi_cvlan_evc_map *
new_cevlan_evc_map (void);
  
void
elmi_evc_status_free (struct elmi_evc_status *evc);

void
elmi_cevlan_evc_map_free (struct elmi_cvlan_evc_map *evc_map);

int
elmi_update_evc_status_type (u_int8_t *br_name, u_int16_t evc_ref_id,
                             u_int8_t evc_status);

int
elmi_evc_ref_id_cmp (void *data1, void *data2);

int
elmi_cevlan_evc_ref_id_cmp (void *data1, void *data2);

void
elmi_unic_delete_cvlan_evc_map (struct elmi_ifp *elmi_if,
                                struct elmi_cvlan_evc_map *cevlan_evc_map);

#endif /* _PACOS_ELMI_PORT_H */
