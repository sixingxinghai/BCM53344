/* Copyright (C) 2003  All Rights Reserved. */

#ifndef _NSM_VPLS_H
#define _NSM_VPLS_H

#define NSM_VPLS_ATTR_FLAG_ALL   0xffffffff


struct nsm_vpls_peer
{
  struct addr_in peer_addr;
  struct vc_fib *vc_fib;
  struct nsm_vpls_peer *primary;
  struct nsm_vpls_peer *secondary;
  
#define NSM_VPLS_PEER_DOWN      0
#define NSM_VPLS_PEER_ACTIVE    1
#define NSM_VPLS_PEER_COMPLETE  2
#define NSM_VPLS_PEER_UP        3
  u_char state;

#define NSM_VPLS_PEER_PRIMARY   (1 << 0)
#define NSM_VPLS_PEER_SECONDARY (1 << 1)
  u_char peer_mode;

#define MPLS_VC_MAPPING_NONE           0
#define MPLS_VC_MAPPING_TUNNEL_NAME    1
#define MPLS_VC_MAPPING_TUNNEL_ID      2
  u_char mapping_type;

  /* Store the Tunnel Info. */
  char *tunnel_name;    
  u_int32_t tunnel_id;
  u_char tunnel_dir; /* Tunnel Direction */


  /* Store the ftn entry identified using Tunnel info. */
  struct ftn_entry *ftn;

  /* 
   * To identify VC. 
   * If type is manual then fec_type_vc is filled with PW_OWNER_MANUAL 
   */
  s_int8_t fec_type_vc;

#ifdef HAVE_SNMP
  /*Unique snmp index for L2VC and Mesh VCs*/
  u_int32_t vc_snmp_index;

  /*Maintaining vpls-id to identify VPLS*/
  u_int32_t vpls_id;

  /*Rowstatus */
  u_int32_t row_status;

  /*Create Time of VC*/
  pal_time_t create_time;

  /*Up time */
  pal_time_t up_time;

  /*Last change */
  pal_time_t last_change;
#endif /*HAVE_SNMP */    
};

struct nsm_vpls_spoke_vc
{
  char *vc_name;
  struct nsm_mpls_circuit *vc;
  struct vc_fib *vc_fib;
  u_int16_t vc_type;
  u_int16_t vc_mode;
  
#define NSM_VPLS_SPOKE_VC_DOWN          0
#define NSM_VPLS_SPOKE_VC_ACTIVE        1
#define NSM_VPLS_SPOKE_VC_COMPLETE      2
#define NSM_VPLS_SPOKE_VC_UP            3
  u_char state;

  struct nsm_vpls_spoke_vc *primary;
  struct nsm_vpls_spoke_vc *secondary;
  u_char svc_mode;
};

struct nsm_vpls 
{
  /* Backpointer to NSM master. */
  struct nsm_master *nm;

  char *vpls_name;
  u_int32_t vpls_id;
  u_int32_t grp_id;
  u_int16_t vpls_type;
  u_int16_t ifmtu;
  char *desc;

  /* mesh vc peer table */
  struct route_table *mp_table;

  /* spoke vc list */
  struct list *svc_list;
  
  /* vpls interfaces */
  struct list *vpls_info_list;

  u_int16_t mp_count;  
#define NSM_VPLS_STATE_INACTIVE      0
#define NSM_VPLS_STATE_ACTIVE        1
  u_char state;

  u_char c_word;

  /* Config. */
#define NSM_VPLS_CONFIG_MTU          (1 << 0)
#define NSM_VPLS_CONFIG_TYPE         (1 << 1)
#define NSM_VPLS_CONFIG_GROUP_ID     (1 << 2)
  u_char config;
};


#define NSM_VPLS_SPOKE_VC_FIB_RESET(s) \
do { \
  XFREE (MTYPE_NSM_VC_FIB, (s)->vc_fib); \
  (s)->vc_fib = NULL;             \
  (s)->state = NSM_VPLS_SPOKE_VC_DOWN; \
} while (0)


#define NSM_VPLS_MESH_PEER_FIB_RESET(m) \
do { \
  XFREE (MTYPE_NSM_VC_FIB, (m)->vc_fib); \
  (m)->vc_fib = NULL;             \
  (m)->ftn = NULL;                \
  (m)->state = NSM_VPLS_PEER_DOWN; \
} while (0)

#define NSM_VPLS_ENTRY_NOT_MAP_TO_FTN(F,V) \
  (!(V) || ((V)->tunnel_id != 0 && (V)->tunnel_id != (F)->tun_id))

#define NSM_VPLS_MTU_ERR    1
/* Function Prototypes */
void nsm_vpls_free (struct nsm_master *nm, struct nsm_vpls *);
struct nsm_vpls *nsm_vpls_lookup (struct nsm_master *, u_int32_t, bool_t);
struct nsm_vpls *nsm_vpls_lookup_by_name (struct nsm_master *, char *);
struct nsm_vpls *nsm_vpls_lookup_by_id (struct nsm_master *, u_int32_t);
struct nsm_vpls_peer *nsm_vpls_peer_new (struct addr_in *, int *);
void nsm_vpls_mesh_peer_free (struct nsm_master *nm, struct nsm_vpls_peer *,
                              struct nsm_vpls *);
int nsm_vpls_cleanup (struct nsm_master *, struct nsm_vpls *);
int nsm_vpls_mesh_peer_cleanup (struct nsm_master *, struct nsm_vpls *,
                                struct route_node *);
struct nsm_vpls_spoke_vc *nsm_vpls_spoke_vc_new (char *, 
                                                 struct nsm_mpls_circuit *,
                                                 int *);
void nsm_vpls_spoke_vc_free (void *);
struct nsm_vpls_spoke_vc *nsm_vpls_spoke_vc_lookup (struct nsm_vpls *, 
                                                    u_int32_t vc_id, bool_t);
struct nsm_vpls_spoke_vc *nsm_vpls_spoke_vc_lookup_by_name (struct nsm_vpls *, 
                                                            char *, bool_t);
void nsm_vpls_spoke_vc_cleanup (struct nsm_master *,
                                struct nsm_vpls_spoke_vc *, bool_t, bool_t);
int nsm_vpls_spoke_vc_add_send_all (struct nsm_vpls *);

int nsm_vpls_add_process (struct nsm_master *,
                          char *, u_int32_t, u_int16_t, u_int16_t, u_char,
                          struct nsm_vpls **);
int nsm_vpls_delete_process (struct nsm_master *, struct nsm_vpls *);
int nsm_vpls_mesh_peer_add_process (struct nsm_master *, struct nsm_vpls *,
                                    struct addr_in *, u_char, char *, u_char,
                                    u_int8_t, char *, struct addr_in *);
int nsm_vpls_mesh_peer_delete_process (struct nsm_master *, struct nsm_vpls *,
                                       struct addr_in *);
struct nsm_vpls_peer* nsm_vpls_mesh_peer_lookup (struct nsm_vpls *vpls,
                                                 struct addr_in *addr,
                                                 bool_t rem_flag);

#ifdef HAVE_SNMP
struct nsm_vpls_peer *
nsm_vpls_snmp_lookup_by_index (struct nsm_master *nm,
                               u_int32_t vc_index);
#endif /*HAVE_SNMP */

int nsm_vpls_fib_add_msg_process (struct nsm_master *,
                                  struct nsm_msg_vc_fib_add *);
int nsm_vpls_fib_delete_msg_process (struct nsm_master *, 
                                     struct nsm_msg_vc_fib_delete *);

int nsm_vpls_spoke_vc_add_process (struct nsm_vpls *, char *, u_int16_t, char * ,char *);
int nsm_vpls_spoke_vc_delete_process (struct nsm_master *, struct nsm_vpls *,
                                      char *);
int nsm_vpls_mac_withdraw_process (struct nsm_master *,
                                   struct nsm_msg_vpls_mac_withdraw *);
int nsm_vpls_interface_delete (struct nsm_vpls *, struct interface *,
                               struct nsm_mpls_vc_info *);
int nsm_vpls_interface_add (struct nsm_vpls *, struct interface *,
                            struct nsm_mpls_vc_info *);
int nsm_vpls_interface_up (struct nsm_vpls *, struct interface *);
int nsm_vpls_interface_down (struct nsm_vpls *, struct interface *);
void nsm_vpls_if_unlink (void *);
int nsm_vpls_fib_add (struct nsm_vpls *, struct nsm_vpls_peer *, u_char, 
                      struct ftn_entry *);
int nsm_vpls_fib_delete (struct nsm_vpls *, struct nsm_vpls_peer *);
int nsm_vpls_spoke_vc_fib_add (struct nsm_vpls_spoke_vc *, u_char, 
                               struct ftn_entry *);
int nsm_vpls_spoke_vc_fib_delete (struct nsm_vpls_spoke_vc *);

int nsm_vpls_fwd_add (struct nsm_vpls *);
int nsm_vpls_fwd_delete (struct nsm_vpls *);
int nsm_vpls_fwd_if_bind (struct nsm_vpls *, struct interface *,
                          u_int16_t);
int nsm_vpls_fwd_if_unbind (struct nsm_vpls *, struct interface *,
                            u_int16_t);
void nsm_vpls_fib_cleanup_all ();
void nsm_vpls_mtu_set (struct nsm_vpls *, u_int16_t);
void nsm_vpls_mtu_unset (struct nsm_vpls *);
void nsm_vpls_desc_set (struct nsm_vpls *, char *);
void nsm_vpls_desc_unset (struct nsm_vpls *);
int  nsm_vpls_type_set (struct nsm_vpls *, u_int16_t);
int  nsm_vpls_type_unset (struct nsm_vpls *);
void nsm_vpls_vlan_bind (struct interface *, u_int16_t);
int nsm_vpls_activate (struct nsm_vpls *vpls);
void nsm_vpls_group_set (struct nsm_vpls *vpls, u_int32_t grp_id);
void nsm_vpls_group_unset (struct nsm_vpls *vpls);

#endif /* _NSM_VPLS_H */
