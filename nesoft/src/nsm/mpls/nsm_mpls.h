/* Copyright (C) 2002-2003  All Rights Reserved. */

#ifndef _NSM_MPLS_H
#define _NSM_MPLS_H

#ifdef HAVE_MPLS_OAM
#include "oam_mpls_msg.h"
#endif /* HAVE_MPLS_OAM */

#if defined (HAVE_BFD) && defined (HAVE_MPLS_OAM)
#include "nsm_mpls_bfd.h"
#endif /* HAVE_BFD  && HAVE_MPLS_OAM */

/* Macro to check if label space is unused. */
#define NSM_LABEL_SPACE_UNUSED(l) \
        (((l) != NULL) && ((l)->refcount == 0) && ((l)->config == 0))

/* VRF hash specific defines. */
#define VRF_HASH_SIZE      25
#define VRF_HASH_KEY(v)    ((v) % VRF_HASH_SIZE)

/*SNMP Trap Values */

#define MPLS_TRAP_ALL                            (0)
#define MPLS_TRAP_ID_MIN                         (1)
#define MPLS_TRAP_ID_MAX                         (2)
#define MPLS_TRAP_VEC_MIN_SIZE                   (1)

#define DEFAULT_NHOP_ADDR     "255.255.255.255"
#define DEFAULT_NEXTHOP_ADDR  (0xFFFFFFFFUL)

/* NSM assigned static label-pool block number */
#define MPLS_STATIC_LABEL_RATIO           5

#define MPLS_TUNNEL_ID_MIN                       1
#define MPLS_TUNNEL_ID_MAX                       65535

/* Bit field based flags */
#define GEN_FLAG_LBL_RANGE_IN_USE         0x01
#define NSM_FTN_KEY_LEN_MAX               40

/* Holder for tree and index manager. */
struct ptree_ix_table
{
  struct ptree *m_table;
  struct bitmap *ix_mgr;

  /* Table identifier. */
  u_int32_t id;

};

/* Holder for tree and index manager. */
struct avl_ix_table
{
  struct avl_tree *m_table;
  struct bitmap *ix_mgr;
};

/* Holder for route table and index manager. */
struct route_ix_table
{
  struct route_table *m_table;
  struct bitmap *ix_mgr;
};

/* Confirm List */
struct confirm_list
{
  struct lsp_dep_confirm *head;
  struct lsp_dep_confirm *tail;
  u_int32_t count;
  struct prefix p;
#define CONFIRM_NODE_DUMMY   0
#define CONFIRM_NODE_FTN     1
  u_char type;
};

struct nsm_mod_label_range
{
  u_int32_t from_label;
  u_int32_t to_label;

  struct nsm_mod_label_range *next;
};

/* Structure to store per label-space info. */
struct nsm_label_space
{
  /* Pointer to NSM master. */
  struct nsm_master *nm;

  /* Label values. */
  u_int32_t min_label_val;
  u_int32_t max_label_val;

  /* Id. */
  u_int16_t label_space;

  /* Config. */
#define NSM_LABEL_SPACE_CONFIG_MIN_LABEL_VAL    (1 << 0)
#define NSM_LABEL_SPACE_CONFIG_MAX_LABEL_VAL    (1 << 1)
#define NSM_LABEL_SPACE_CONFIG_MIN_LABEL_RANGE  (1 << 2)
#define NSM_LABEL_SPACE_CONFIG_MAX_LABEL_RANGE  (1 << 3)

  u_char config;

  bool_t static_block;

  /* Back pointer. */
  struct route_node *rn;

  /* Reference count of interfaces using this label space. */
  u_int32_t refcount;

  /*Maintain a list of disjoint label ranges on a per module basis */
#if (defined HAVE_PACKET || defined HAVE_GMPLS)
  struct nsm_mod_label_range service_ranges[LABEL_POOL_RANGE_MAX];
#endif /* HAVE_PACKET || HAVE_GMPLS*/

};

/* MPLS interface node. */
struct nsm_mpls_if
{
  /* Back pointer to NSM master. */
  struct nsm_master *nm;

  /* Back pointer to list node. */
  struct listnode *ln;

  /* Pointer to PacOS interface structure. */
  struct interface *ifp;

  /* Label-space. */
  struct nsm_label_space *nls;

  /* VRF id. */
  int vrf_id;

#ifdef HAVE_MPLS_VC
  struct list *vc_info_list;
#endif /* HAVE_MPLS_VC */

#ifdef HAVE_VPLS
  struct list *vpls_info_list;
#endif /* HAVE_VPLS */

#if defined (HAVE_MPLS_FWD)
  /* MPLS Interface entry stats. */
  pal_mpls_if_entry_stats_t stats;
#endif /* HAVE_MPLS_FWD */
};

/* VRF hash table entry. */
struct vrf_table
{
  /* VRF ID */
  u_int32_t vrf_id;

  /* VRF Table and Index manager */
  struct ptree_ix_table vrf_ix_table4;

#ifdef HAVE_IPV6
  struct ptree_ix_table vrf_ix_table6;
#endif

  /* Prev/Next pointer. */
  struct vrf_table *prev;
  struct vrf_table *next;
};

#define NSM_MPLS_NUM_TREE       10
struct nsm_mpls_tree_array
{
  /* Count of valid tree entries */
  u_char cnt;
  void *ptr [NSM_MPLS_NUM_TREE];
};

/**@brief - VCCV Statistics */
struct vccv_statistics
{
  u_int32_t cc_mismatch_discards; /* Discards due to CC mismatch */
};

/* MPLS-specific data wrapper. */
struct nsm_mpls
{
  /* List of interfaces. */
  struct list *iflist;

  /* List of B-LSP's */
  struct list *b_lsp;

#if (!defined (HAVE_GMPLS) || defined (HAVE_PACKET))
  /* Label pool boundaries. */
  u_int32_t min_label_val;
  u_int32_t max_label_val;

  /* Custom TTL values. */
#define NSM_MPLS_NO_TTL                -1
  short ingress_ttl;
  short egress_ttl;

  /* TTL-Propagation capability */
  u_char propagate_ttl;

#endif /* !HAVE_GMPLS || HAVE_PACKET */

#ifdef HAVE_TE
  struct admin_group admin_group_array [ADMIN_GROUP_MAX];
#endif /* HAVE_TE */

  u_char shutdown;

  /* Kernel messages. */
#define NSM_MPLS_SHOW_MSG_ERROR        (1 << 0)
#define NSM_MPLS_SHOW_MSG_WARNING      (1 << 1)
#define NSM_MPLS_SHOW_MSG_DEBUG        (1 << 2)
#define NSM_MPLS_SHOW_MSG_NOTICE       (1 << 3)
#define NSM_MPLS_SHOW_MSG_ALL          (1 << 4)
  u_char kern_msgs;

#define NSM_MPLS_FLAG_INSTALL_BK_LSP           (1 << 0)
  u_char flags;

  /* Configured data. */
#define NSM_MPLS_CONFIG_INGRESS_TTL    (1 << 0)
#define NSM_MPLS_CONFIG_EGRESS_TTL     (1 << 1)
#define NSM_MPLS_CONFIG_LCL_PKT_HANDLE (1 << 2)
#define NSM_MPLS_CONFIG_KERN_MSGS      (1 << 3)
#define NSM_MPLS_CONFIG_ENABLE_ALL_IFS (1 << 4)
#define NSM_MPLS_CONFIG_MIN_LABEL_VAL  (1 << 5)
#define NSM_MPLS_CONFIG_MAX_LABEL_VAL  (1 << 6)
#define NSM_MPLS_CONFIG_SUPPORTED_DSCP (1 << 7)
#define NSM_MPLS_CONFIG_DSCP_EXP_MAP   (1 << 8)
  u_int16_t config;

  /* Label space table. */
  struct route_table *ls_table;

  /* Tree arrays of pointer to valid trees */
#define NSM_MPLS_TREE_ARRAY_FTN_TREES            0
#define NSM_MPLS_TREE_ARRAY_FTN_IX_TREES         1
#define NSM_MPLS_TREE_ARRAY_ILM_TREES            2
#define NSM_MPLS_TREE_ARRAY_ILM_IX_TREES         3
#define NSM_MPLS_TREE_ARRAY_NHLFE_TREES          4
#define NSM_MPLS_TREE_ARRAY_MAPROUTE_TREES       5
#define NSM_MPLS_TREE_ARRAY_FTN6_TREES           6
#define NSM_MPLS_TREE_ARRAY_FTN6_IX_TREES        7
#define NSM_MPLS_TREE_ARRAY_NHLFE6_TREES         8
#define NSM_MPLS_TREE_ARRAY_MAX                  9

  struct nsm_mpls_tree_array trees [NSM_MPLS_TREE_ARRAY_MAX];

  /* Global FTN table. These tables will be valid for non-packet entries too,
     as the control plane address is added to the FTN table as dummy entry */
  struct ptree_ix_table ftn_ix_table4;
#ifdef HAVE_IPV6
  struct ptree_ix_table ftn_ix_table6;
#endif

#ifdef HAVE_GMPLS
  /* For non IP based tables we can use AVL Tree instead of Patricia Tree as
     Longest Match is not required. The index will use the FTN IX Table Index.
     However Patricia Tree is used so that the same structure can be passed
     along for all FTN Tables */
#ifdef HAVE_PBB_TE
  struct ptree_ix_table ftn_pbb_table;
#endif /* HAVE_PBB_TE */

#ifdef HAVE_TDM
  /* For structure agnostic TDM */
  struct ptree_ix_table ftn_tdm_table;
#endif /* HAVE_TDM */
#endif /* HAVE_GMPLS */

  /* Dependency tables - Dependency will always be on an IP addressed FTN entry
     The table stays a route table because of that. */
  struct route_table *lsp_dep_up;
  struct route_table *lsp_dep_down;

  /* ILM table. */
  struct avl_ix_table ilm_ix_table;

  /* XC Table */
  struct avl_ix_table xc_ix_table;

  /* NHLFE Table */
  struct avl_ix_table nhlfe_ix_table4;

#ifdef HAVE_IPV6
  struct avl_ix_table nhlfe_ix_table6;
#endif

#ifdef HAVE_GMPLS
  /* Cross connect Table */
#ifdef HAVE_PBB_TE
  struct avl_ix_table pbb_ilm_ix;

  struct avl_ix_table pbb_xc_ix;

  struct avl_ix_table pbb_nhlfe_ix;
#endif /* HAVE_PBB_TE */

#ifdef HAVE_TDM
  /* For structure agnostic TDM */
  struct avl_ix_table tdm_ilm_ix;

  struct avl_ix_table tdm_xc_ix;

  struct avl_ix_table tdm_nhlfe_ix;
#endif /* HAVE_TDM */
#endif /* HAVE_GMPLS */

#ifdef HAVE_VRF
  /* VRF hash table. */
  struct vrf_table *vrf_hash[VRF_HASH_SIZE];
#endif /* HAVE_VRF */

#ifdef HAVE_PACKET
  /* Mapped-routes table. */
  struct ptree *mapped_routes;

  /* Mapped-lsp table. */
  struct avl_tree *mapped_lsp;
#endif /* HAVE_PACKET */

#ifdef HAVE_GMPLS
#ifdef HAVE_PBB_TE
  struct ptree *mapped_routes_pbb;

  struct avl_tree *mapped_lsp_pbb;
#endif /* HAVE_PBB_TE */

#ifdef HAVE_TDM
  struct ptree *mapped_routes_tdm;

  struct avl_tree *mapped_lsp_tdm;
#endif /* HAVE_TDM */
#endif /* HAVE_GMPLS */

#ifdef HAVE_MPLS_VC
  struct route_table *vc_table;
  struct list *vc_group_list;

  /* ldp id values */
  u_int32_t lsr_id;
  /* ldp VC entity index */
  u_int32_t index;
#endif /* HAVE_MPLS_VC */

#ifdef HAVE_VPLS
  struct ptree *vpls_table;
#endif /* HAVE_VPLS */

#ifdef HAVE_NSM_MPLS_OAM
  /* MPLS OAM Master Data */
  struct list *mpls_oam_list;

  struct thread *mpls_oam_read;
  int oam_sock;
  int oam_s_sock;

  /* MPLS OAM ITUT Data */
  struct list *mpls_oam_itut_list;

#endif /*HAVE_NSM_MPLS_OAM */

#if defined HAVE_MPLS_OAM || defined HAVE_NSM_MPLS_OAM
  struct list *mpls_vpn_list;
#endif /* HAVE_MPLS_OAM || HAVE_NSM_MPLS_OAM */

#ifdef HAVE_MPLS_VC
  u_int16_t label_space;

  struct hash *nsm_mpls_circuit_hash_table;
#define NSM_MPLS_CIRCUIT_HASH NSM_MPLS->nsm_mpls_circuit_hash_table

#ifdef HAVE_MS_PW
  struct hash *nsm_ms_pw_hash_table;
#define NSM_MS_PW_HASH  NSM_MPLS->nsm_ms_pw_hash_table
#define NSM_MS_PW_MIN_IX   1
#define NSM_MS_PW_MAX_IX   65535
#define NSM_MS_PW_BUCKET_SIZE 1280
#endif /* HAVE_MS_PW */
#endif /* HAVE_MPLS_VC */

#if (defined HAVE_MPLS_VC) || (defined HAVE_VPLS)
#ifdef HAVE_SNMP
  struct bitmap *vc_indx_mgr;
#endif /*HAVE_SNMP*/
#endif /*defined HAVE_MPLS_VC || defined HAVE_VPLS*/

#if defined (HAVE_BFD) && defined (HAVE_MPLS_OAM)
  struct list *bfd_fec_conf_list; /** List of BFD for FEC conf entries */
   /** BFD attributes, one member for each lsp-type */
  struct nsm_mpls_bfd_lsp_conf bfd_lsp_conf [BFD_MPLS_LSP_TYPE_UNKNOWN];
  u_char bfd_flag; /** BFD Flag. */

  #define NSM_MPLS_BFD_ALL_LDP        (1 << 0)
  #define NSM_MPLS_BFD_ALL_RSVP       (1 << 1)
  #define NSM_MPLS_BFD_ALL_STATIC     (1 << 2)
#endif /* HAVE_BFD  && HAVE_MPLS_OAM */

#if (!defined (HAVE_GMPLS) || defined (HAVE_PACKET))
#ifdef HAVE_DIFFSERV
  /* LSP Model */
  u_char lsp_model;

  /* Configure DSCP to EXP bit mapping. */
  u_char dscp_exp_map[DIFFSERV_MAX_DSCP_EXP_MAPPINGS];

  /* Supported DSCP values. */
  u_char supported_dscp[DIFFSERV_MAX_SUPPORTED_DSCP];

#endif /* HAVE_DIFFSERV */

#ifdef HAVE_DSTE
  /* TE CLASS table */
  struct te_class_s te_class[MAX_TE_CLASS];

  /* CLASS TYPE table */
  char ct_name[MAX_CLASS_TYPE + 1][MAX_CT_NAME_LEN + 1];
#endif /* HAVE_DSTE */
#endif /* !HAVE_GMPLS || HAVE_PACKET */

#if defined (HAVE_MPLS_FWD)
  /* Top level stats. */
  pal_fib_stats_t stats;
  /* MPLS Interface stats. for per platform */
  pal_mpls_if_entry_stats_t if_stats;
#endif /* HAVE_MPLS_FWD */

  /*Flag For Trap Generation */
  int notificationcntl; /* 1 - Enabled, 2 - Disabled */

  /* Trap variables */
  pal_time_t notify_time_init;
  u_int32_t notify_cnt;

  int vc_pw_notification;
  #define NSM_MPLS_VC_UP_DN_NOTIFN           0x01
  #define NSM_MPLS_VC_DEL_NOTIFN             0x02

  #define NSM_MPLS_VC_UP_DN_NOTIFN_DIS       0
  #define NSM_MPLS_VC_UP_DN_NOTIFN_ENA       1

  #define NSM_MPLS_SNMP_VC_UP_DN_NTFY_ENA    0x01
  #define NSM_MPLS_SNMP_VC_UP_DN_NTFY_DIS    0x02

  #define NSM_MPLS_VC_DEL_NOTIFN_DIS         0
  #define NSM_MPLS_VC_DEL_NOTIFN_ENA         1

  #define NSM_MPLS_SNMP_VC_DEL_NTFY_ENA      0x01
  #define NSM_MPLS_SNMP_VC_DEL_NTFY_DIS      0x02
  int vc_pw_notification_rate;

  /* MPLS SNMP trap callback function */
  vector traps [MPLS_TRAP_ID_MAX];

#ifdef HAVE_VCCV
  /* Place holder for VCCV Statistics */
  struct vccv_statistics vccv_stats;
#endif /* HAVE_VCCV */
#ifdef HAVE_GMPLS
/* Datalink - loopback index */
  s_int32_t loop_gindex;
#endif /* HAVE_GMPLS */
};

/* Prototypes. */
u_int32_t nsm_mpls_max_label_val_get (struct nsm_master *);
u_int32_t nsm_mpls_min_label_val_get (struct nsm_master *);
int nsm_mpls_main_init (void);
int nsm_mpls_init (struct nsm_master *);
int nsm_mpls_if_enable_by_ifp (struct interface *, u_int16_t);
int nsm_mpls_if_disable_by_ifp (struct interface *);
void nsm_mpls_deinit (struct nsm_master *);
void nsm_mpls_main_deinit (void);
void nsm_mpls_enable_all_interfaces (struct nsm_master *, u_int16_t);
void nsm_mpls_disable_all_interfaces (struct nsm_master *);
void nsm_mpls_if_up (struct nsm_mpls_if *);
void nsm_mpls_if_down (struct interface *);
void nsm_mpls_if_del (struct interface *, bool_t);
int nsm_mpls_if_binding_update (struct interface *, struct nsm_master *,
                                struct nsm_master *);
void nsm_mpls_tree_array_get (struct nsm_master *, void **, u_int32_t,
                              u_int32_t);
void nsm_mpls_tree_array_get_next (struct nsm_master *, void *, void **,
                                   u_int32_t);
void nsm_mpls_tree_array_add (struct nsm_master *, void *, u_int32_t);

void nsm_mpls_label_space_del (struct nsm_label_space *);
struct nsm_label_space * nsm_mpls_label_space_get (struct nsm_master *,
                                                   u_int16_t);
#if defined (HAVE_MPLS_FWD)
void nsm_mpls_if_stats_incr_in_labels_used (struct nsm_master *,
                                            struct interface *);
void nsm_mpls_if_stats_decr_in_labels_used (struct nsm_master *,
                                            struct interface *);
void nsm_mpls_if_stats_clear_in_labels_used (struct interface *);
void nsm_mpls_if_stats_incr_out_labels_used (struct interface *);
void nsm_mpls_if_stats_decr_out_labels_used (struct interface *);
void nsm_mpls_if_stats_clear_out_labels_used (struct interface *);
#endif /* HAVE_MPLS_FWD */

#ifdef HAVE_VRF
int nsm_mpls_vrf_add (struct nsm_master *, int);
void nsm_mpls_vrf_del (struct nsm_master *, int);
int nsm_mpls_if_vrf_bind_by_ifp (struct interface *, int);
int nsm_mpls_if_vrf_unbind_by_ifp (struct interface *);
void nsm_mpls_vrf_table_free (struct vrf_table *);
#endif /* HAVE_VRF */

void nsm_mpls_log (struct nsm_master *, u_char);
void nsm_mpls_no_log (struct nsm_master *, u_char);
struct nsm_mpls_if *nsm_mpls_if_add (struct interface *);
struct nsm_mpls_if *nsm_mpls_if_lookup (struct interface *);
struct nsm_label_space *nsm_mpls_label_space_lookup (struct nsm_master *,
                                                     u_int16_t);
void nsm_mpls_ifp_set (struct nsm_mpls_if *, struct interface *);
void nsm_mpls_ifp_unset (struct interface *);
struct ptree_ix_table *nsm_gmpls_ftn_table_lookup (struct nsm_master *,
                                         u_int32_t, struct fec_gen_entry *);

struct nsm_mpls_if *nsm_mpls_if_lookup_in (struct nsm_master *,
                                           struct interface *);
#if (!defined (HAVE_GMPLS) || defined (HAVE_PACKET))
#ifdef HAVE_DIFFSERV
int  nsm_mpls_dscp_exp_map_add (struct nsm_master *, char *, int);
int  nsm_mpls_dscp_exp_map_del (struct nsm_master *, char *, int);
int  nsm_mpls_dscp_support_add (struct nsm_master *, char *);
int  nsm_mpls_dscp_support_del (struct nsm_master *, char *);
#endif /* HAVE_DIFFSERV */
#endif /* !HAVE_GMPLS || HAVE_PACKET */
int nsm_gmpls_static_label_range_check (u_int32_t, struct interface *,
                                       u_int32_t *, u_int32_t *);


/*---------------------------------------------------------------------------
  Generic MPLS shim header.

  0                   1                   2                   3
  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+ Label
  |                Label                  | Exp |S|       TTL     | Stack
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+ Entry
  --------------------------------------------------------------------------*/

#ifdef HAVE_NSM_MPLS_OAM
struct shimhdr
{

  u_int32_t shim;

  /*-----------------------------------------------------------------------
    Some hash defines to get the different fields of the shim header.
    -----------------------------------------------------------------------*/
#define get_label_net(shim)      (((shim) >> 12) & 0xFFFFF)
#define get_exp_net(shim)        (((shim) >> 9)  & 0x7)
#define get_bos_net(shim)        (((shim) >> 8)  & 0x1)
#define get_ttl_net(shim)        ((shim)  & 0xFF)
#define set_label_net(shim, val) {(shim) &= 0x00000FFF;(shim) |= (val)<<12;}
#define set_exp_net(shim, val)   \
                                 {(shim) &= 0xFFFFF1FF;(shim) |= (val)<<9;}
#define set_bos_net(shim, val)   \
                                 {(shim) &= 0xFFFFFEFF;(shim) |= (val)<<8;}
#define set_ttl_net(shim, val)   \
                                 {(shim) &= 0xFFFFFF00;(shim) |= (val);}
#define get_label_host(shim)     get_label_net(shim)
#define get_exp_host(shim)       get_exp_net(shim)
#define get_bos_host(shim)       get_bos_net(shim)
#define get_ttl_host(shim)       get_ttl_net(shim)
#define set_label_host(shim,val) set_label_net(shim,val)
#define set_exp_host(shim,val)   set_exp_net(shim,val)
#define set_bos_host(shim,val)   set_bos_net(shim,val)
#define set_ttl_host(shim,val)   set_ttl_net(shim,val)
};
#endif /* !HAVE_NSM_MPLS_OAM */
#endif /* _NSM_MPLS_H */
