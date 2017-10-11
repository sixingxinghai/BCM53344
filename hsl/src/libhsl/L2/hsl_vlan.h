/* Copyright (C) 2004  All Rights Reserved.  */

#ifndef _HSL_VLAN_H_
#define _HSL_VLAN_H_

#define HSL_VLAN_MAX                 4094              /* Max VLANs.   */
#define HSL_VLAN_ALL                 (HSL_VLAN_MAX + 1)/* All VLANs.   */
#define HSL_VLAN_NONE                0                 /* No VLANs.    */
#define HSL_VLAN_DEFAULT_VID         1                 /* Default VID. */


/* 
   HSL VLAN->port map structure. 
*/
struct hsl_vlan_port
{
  hsl_vid_t      vid;                  /* VLAN id. */

  struct hsl_avl_tree *port_tree;      /* Tree of ports on which VLAN exists. */
#ifdef HAVE_PVLAN
  enum hal_pvlan_type vlan_type;
  struct hsl_avl_tree *vlan_tree;      /* Tree of secondary vlans attached 
                                          to primary vlan */
  void *system_info;
#endif /* HAVE_PVLAN */
#ifdef HAVE_L2LERN
  struct hsl_vlan_access_map *hsl_vacc_map;
#endif /* HAVE_L2LERN */

};

/*
   HSL VLAN attributes on port struct.  
 */

struct hsl_vlan_port_attr
{
  hsl_vid_t      vid;                  /* VLAN id.       */
  HSL_BOOL       etagged;              /* Egress tagged. */ 
};

/*
  HSL port->VLAN map structure.
*/
struct hsl_port_vlan
{
  struct hsl_bridge_port    *port;     /* Backpointer to bridge port. */
  u_char  mode;                 /* Port mode. */ 

  hsl_vid_t                 pvid;      /* Access port: access, Trunk port: native. */       
                                            
  u_char flags;                        /* Flags. */
#define NSM_VLAN_ENABLE_INGRESS_FILTER        (1 << 0)
#define NSM_VLAN_ACCEPTABLE_FRAME_TYPE_TAGGED (1 << 1)

  struct hsl_avl_tree *vlan_tree;     /* VLANs this port belongs to. */ 
#ifdef HAVE_PVLAN
  u_char pvlan_port_mode;
#endif /* HAVE_PVLAN */
};

/* 
  Function prototypes.
*/
hsl_vid_t hsl_get_pvid (struct hsl_if *ifp);
struct hsl_vlan_port * hsl_vlan_port_map_init (void);
int hsl_vlan_port_map_deinit (struct hsl_vlan_port *v);
struct hsl_port_vlan *hsl_port_vlan_map_init (void);
int hsl_port_vlan_map_deinit (struct hsl_port_vlan *v);
int hsl_vlan_add (char *name, enum hal_vlan_type type, hsl_vid_t vid);
int hsl_vlan_delete (char *name, hsl_vid_t vid);
int hsl_vlan_port_set_port_type (char *name, hsl_ifIndex_t ifindex,
                                 enum hal_vlan_port_type port_type,
                                 enum hal_vlan_port_type sub_port_type,
                                 enum hal_vlan_acceptable_frame_type acceptable_frame_types,
                                 u_int16_t enable_ingress_filter);
int hsl_vlan_set_default_pvid (char *name, hsl_ifIndex_t ifindex, hsl_vid_t pvid, enum hal_vlan_egress_type egress);
int hsl_add_vid_to_port (char *name, hsl_ifIndex_t ifindex, hsl_vid_t vid, enum hal_vlan_egress_type egress);
int hsl_delete_vid_from_port (char *name, hsl_ifIndex_t ifindex, hsl_vid_t vid);
int hsl_vlan_add_vid_to_port (char *name, hsl_ifIndex_t ifindex, hsl_vid_t vid, enum hal_vlan_egress_type egress);

int hsl_vlan_set_port_type (char *name, hsl_ifIndex_t ifindex,
                            enum hal_vlan_port_type port_type,
                            enum hal_vlan_port_type sub_port_type,
                            enum hal_vlan_acceptable_frame_type acceptable_frame_types,
                            u_int16_t enable_ingress_filter);

/*
  Set dot1q State.
*/
int
hsl_vlan_set_dot1q_state (hsl_ifIndex_t ifindex,
                          u_int8_t enable,
                          u_int16_t enable_ingress_filter);

int hsl_vlan_delete_vid_from_port (char *name, hsl_ifIndex_t ifindex, hsl_vid_t vid);
int hsl_vlan_get_egress_type (struct hsl_if *ifp, hsl_vid_t vid);
int hsl_vlan_check_if_empty(hsl_vid_t vid);
int hsl_port_flush_vlans(hsl_ifIndex_t ifindex);
int hsl_vlan_agg_port_bind_update(struct hsl_if *port_ifp, struct hsl_if *agg_ifp,
                                  HSL_BOOL do_bind);
int hsl_vlan_agg_unbind(struct hsl_if *agg_ifp);
#ifdef HAVE_L3
int hsl_vlan_enable_l3_pkt(hsl_vid_t vid, int enable);
int hsl_vlan_svi_link_port_members (struct hsl_if *ifpp, hsl_vid_t vid);
#endif /* HAVE_L3 */

#ifdef HAVE_VLAN_CLASS
int hsl_vlan_classifier_add (struct hal_msg_vlan_classifier_rule *msg);
int hsl_vlan_classifier_delete (struct hal_msg_vlan_classifier_rule *msg);
#endif /* HAVE_VLAN_CLASS */

struct hsl_vlan_regis_key *
hsl_vlan_regis_key_init (void);

void
hsl_vlan_regis_key_deinit (void *key);

#ifdef HAVE_PVLAN
int hsl_pvlan_set_vlan_type(char *name, hsl_vid_t vid, 
                            enum hal_pvlan_type vlan_type);
int hsl_pvlan_add_vlan_association(char *name, hsl_vid_t primary_vid,
                                   hsl_vid_t secondary_vid);
int hsl_pvlan_remove_vlan_association(char *name, hsl_vid_t primary_vid,
                                      hsl_vid_t secondary_vid);
int hsl_pvlan_port_add_association(char *name, hsl_ifIndex_t ifindex,
                                   hsl_vid_t primary_vlan,
                                   hsl_vid_t secondary_vlan);
int hsl_pvlan_port_delete_association(char *name, hsl_ifIndex_t ifindex,
                                      hsl_vid_t primary_vlan,
                                      hsl_vid_t secondary_vlan);
int hsl_pvlan_set_port_mode(char *name, hsl_ifIndex_t ifindex,
                            enum hal_pvlan_port_mode port_mode); 
#endif /* HAVE_PVLAN */

#endif /* _HSL_VLAN_H_ */
