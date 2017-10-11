/* Copyright (C) 2002-2003  All Rights Reserved. */

#ifndef _NSM_MPLS_VC_H
#define _NSM_MPLS_VC_H

/* Macro to check whether VRF is usable or not. */
#define NSM_MIF_USABLE_FOR_CIRCUIT(m) \
        ((mif->vc == NULL &&  mif->vrf_id == VRF_ID_UNSPEC) ? 1 : 0)

/* Undefined Virtual Circuit. */
#define NSM_MPLS_L2_VC_UNDEFINED            0

/* Min/Ma  Virtual Circuit ID val. */
#define NSM_MPLS_L2_VC_MIN                  1
#define NSM_MPLS_L2_VC_MAX                  4294967295UL

/* Circuit ID bit len. */
#define NSM_MPLS_CIRCUIT_ID_LEN             32

/* Defines for Primary/Secondary */
#define VC_PRIMARY_SPOKE       1
#define VC_SECONDARY_SPOKE     2

/* Macro for timer turn on */
#define VC_TIMER_ON(LIB_GLOB, THREAD, ARG, THREAD_FUNC, TIME_VAL)    \
do {                                                                  \
  if (!(THREAD))                                                      \
    (THREAD) = thread_add_timer ((LIB_GLOB), (THREAD_FUNC),           \
                                 (ARG), (TIME_VAL));                  \
} while (0)

/* Macro for timer turn off */
#define VC_TIMER_OFF(THREAD)                                         \
do {                                                                  \
  if (THREAD)                                                         \
    {                                                                 \
      thread_cancel (THREAD);                                         \
      (THREAD) = NULL;                                                \
    }                                                                 \
} while (0)

/* PW_OWNER draft-ietf-pwe3-pw-mib-09.txt */
#define PW_OWNER_MANUAL                     1
#define PW_OWNER_PWID_FEC_SIGNALING         2
#define PW_OWNER_GEN_FEC_SIGNALING          3
#define PW_OWNER_L2TP_CONTROL_PROTOCOL      4
#define PW_OWNER_OTHER                      5

#define VC_TIMER_15MIN                      900

#define NSM_MPLS_PW_MAX_PERF_COUNT                  96
#define NSM_MPLS_PW_MAX_1DY_PERF_COUNT      1

#ifdef HAVE_MPLS_VC
struct nsm_mpls_pw_snmp_perf_curr
{
  u_int64_t  pw_crnt_in_pckts;
  u_int64_t  pw_crnt_in_bytes;
  u_int64_t  pw_crnt_out_pckts;
  u_int64_t  pw_crnt_out_bytes;
};
#endif /*HAVE_MPLS_VC */
#ifdef HAVE_MS_PW
/**@brief vc fib data.
*/
struct vc_fib_data
{
  struct if_ident if_in; /**< incoming intf identifier */
  struct if_ident if_out; /**< outgoing intf identifier */
  struct pal_in4_addr peer_nhop_addr; /**< peer nexthop addr */
  struct pal_in4_addr fec_addr; /**< fec addr */
  u_char fec_prefixlen; /**< fec prefix length */
  u_int32_t tunnel_label; /**< tunnel label */
  struct pal_in4_addr tunnel_nhop; /**< address of the nxt hop tunnel */
  u_int32_t tunnel_oix; /**< tunnel intf ifindex */
  u_int32_t tunnel_nhlfe_ix; /**< tunnel nhlfe index */
  u_int32_t tunnel_ftnix; /**< tunnel ftn index */
  u_char data_avail; /**< flag to check whether data was populated or not */
};
#endif /* HAVE_MS_PW */

/* Following two structures are used for the MPLS VC Fib entries */
struct vc_fib
{
  u_int32_t in_label;
  u_int32_t out_label;
  u_int32_t ac_if_ix;
  u_int32_t nw_if_ix;
  u_int32_t nw_if_ix_conf;
  u_char opcode;
  u_char install_flag;
  u_char c_word;
  u_int32_t lsr_id;
  u_int32_t index;
#ifdef HAVE_MPLS_OAM
  bool_t is_oam_enabled;
#endif /* HAVE_MPLS_OAM */
#ifdef HAVE_VCCV
  u_int8_t remote_cc_types;
  u_int8_t remote_cv_types;
#endif /* HAVE_VCCV */
  bool_t rem_pw_status_cap;

#ifdef HAVE_MS_PW
  struct vc_fib_data vc_fib_data_temp;
#endif /* HAVE_MS_PW */
};

struct nsm_mpls_vc_group
{
  /* Identifier. */
  u_int32_t id;

  /* Name. */
  char *name;

  /* List of circuits belonging to this group. */
  struct list *vc_list;

#ifdef HAVE_VPLS
  /* List of vpls belonging to this group. */
  struct list *vpls_list;
#endif /* HAVE_VPLS */

  /* Back pointer. */
  struct listnode *ln;
};

typedef enum non_te_map_dn
  {
    PSNBOUND = 1,
    FROMPSN,
  } non_te_map_dn_t;

/**@brief Vircuit circuit structure using the hash table.
*/
struct nsm_mpls_circuit_container
{
  char *name;
  struct nsm_mpls_circuit* vc; /**< pointer to the mpls circuit struct */
};

/**@ Tunnel Directions */
enum
{
  TUNNEL_DIR_FWD = 0,
  TUNNEL_DIR_REV,
  TUNNEL_DIR_NONE
};

/* Virtual Circuit structure. */
struct nsm_mpls_circuit
{
  /* Identifier. */
  u_int32_t id;
  u_int32_t grp_attchmt_id;
  u_int32_t local_attchmt_id;
  u_int32_t peer_attchmt_id;
  pal_time_t uptime;
  pal_time_t last_change;
  pal_time_t create_time;
  u_char admin_status;
  mpls_owner_t owner;
  non_te_map_dn_t dn;

  /* Name. */
  char *name;

  /* Descriptor*/
  char *dscr;

  /* End-point address. */
  struct prefix address;

  u_int8_t setup_prty;
  u_int8_t hold_prty;
  u_int8_t fec_type_vc;
  /* Control word. */
  u_char cw;

  /* Group pointer. */
  struct nsm_mpls_vc_group *group;

  /* Pointer back to vc_info. */
  struct nsm_mpls_vc_info *vc_info;

#ifdef HAVE_SNMP
  struct nsm_mpls_vc_info_temp *vc_info_temp;
#endif /* HAVE_SNMP */

  /* Temorary place holder for port vlan */
  u_int16_t vlan_id;

  /* Store either the tunnel name or the tunnel id */
  char *tunnel_name;
  u_int32_t tunnel_id;
  u_char tunnel_dir; 

  /* Back pointer. */
  struct route_node *rn;

  /* VC Fib Structures */
  struct vc_fib *vc_fib;

  mpls_row_status_t row_status;
  struct thread *t_time_elpsd;
  u_int32_t valid_intrvl_cnt;

  /* for PW-MPLS-MIB */
  struct ftn_entry *ftn;

  /* inbound xc index */
  u_int32_t xc_ix;

  /* save dummy ilm entry's index for delete */
  u_int32_t ilm_ix;

  non_te_map_dn_t dn_in;
  u_int32_t remote_grp_id;
#ifdef HAVE_VPLS
  /* vpls */
  struct nsm_vpls *vpls;
#endif /* HAVE_VPLS */

  /* PW Status Flags from RFC 4446/4447 */
#define NSM_CODE_PW_FORWARDING              0
#define NSM_CODE_PW_NOT_FORWARDING          (1 << 0)
#define NSM_CODE_PW_AC_INGRESS_RX_FAULT     (1 << 1)
#define NSM_CODE_PW_AC_EGRESS_TX_FAULT      (1 << 2)
#define NSM_CODE_PW_PSN_INGRESS_RX_FAULT    (1 << 3)
#define NSM_CODE_PW_PSN_EGRESS_TX_FAULT     (1 << 4)
#define NSM_CODE_PW_STANDBY                 (1 << 5) 
#define NSM_CODE_PW_REQUEST_SWITCHOVER      (1 << 6)
  /* Local pw_status */
  u_int32_t pw_status;
  /* Remote pw_status */
  u_int32_t remote_pw_status;

  /* States and Flags required for VC Control and setup */
#define NSM_MPLS_L2_CIRCUIT_DOWN            0 /* VC not bound to intf */
#define NSM_MPLS_L2_CIRCUIT_ACTIVE          1 /* VC bound to intf */
#define NSM_MPLS_L2_CIRCUIT_COMPLETE        2 /* VC signaling complete */
#define NSM_MPLS_L2_CIRCUIT_UP              3 /* VC could be installed */
  u_char state;

#define MPLS_VC_MAPPING_NONE           0
#define MPLS_VC_MAPPING_TUNNEL_NAME    1
#define MPLS_VC_MAPPING_TUNNEL_ID      2
  u_char mapping_type;

#define NSM_MPLS_VC_FLAG_SELECTED    (1 << 0)
#define NSM_MPLS_VC_FLAG_DEP         (1 << 1)
#define NSM_MPLS_VC_FLAG_CONFIGURED  (1 << 2)
  u_char flags;

#ifdef HAVE_VCCV
  u_int8_t cc_types;
  u_int8_t cv_types;
  bool_t is_bfd_running;
#endif /* HAVE_VCCV */

   /* List for maintaining current Performance counters in PW MIB*/
  struct list * nsm_mpls_pw_snmp_perf_curr_list;

  /*Unique snmp index for L2VC and Mesh VCs*/
  u_int32_t vc_snmp_index;

#ifdef HAVE_MS_PW
  /* Pointer to MS-PW */
  struct nsm_mpls_ms_pw *ms_pw;

  /* Pointer to other VC */
  struct nsm_mpls_circuit* vc_other;

  /* MS-PW role: applicable on T-PEs when VCs are signalled; Dflt: 0  */
#define NSM_MSPW_ROLE_PASSIVE 1
#define NSM_MSPW_ROLE_ACTIVE  0
  u_int8_t ms_pw_role;

  /* Pointer to the list of S-PE TLVs */
  struct list* ms_pw_spe_list;
#endif /* HAVE_MS_PW */
};

#define IS_NSM_CODE_PW_FAULT(C)                                          \
  ((CHECK_FLAG ((C), NSM_CODE_PW_AC_INGRESS_RX_FAULT)) ||                \
   (CHECK_FLAG ((C), NSM_CODE_PW_AC_EGRESS_TX_FAULT))  ||                \
   (CHECK_FLAG ((C), NSM_CODE_PW_PSN_INGRESS_RX_FAULT))||                \
   (CHECK_FLAG ((C), NSM_CODE_PW_PSN_EGRESS_TX_FAULT)))                  \

#define IS_NSM_CODE_PW_AC_FAULT(C)                                       \
  ((CHECK_FLAG ((C), NSM_CODE_PW_AC_INGRESS_RX_FAULT)) ||                \
   (CHECK_FLAG ((C), NSM_CODE_PW_AC_EGRESS_TX_FAULT)))                   \

#define IS_NSM_CODE_PW_PSN_FAULT(C)                                      \
  ((CHECK_FLAG ((C), NSM_CODE_PW_PSN_INGRESS_RX_FAULT))||                \
   (CHECK_FLAG ((C), NSM_CODE_PW_PSN_EGRESS_TX_FAULT)))                  \

#define IS_NSM_CODE_PW_STANDBY(C)                                        \
  (CHECK_FLAG ((C), NSM_CODE_PW_STANDBY))                                \

#define PW_STATUS_INSTALL_CHECK(L,R)                                     \
  (! IS_NSM_CODE_PW_FAULT (L) &&                                         \
   ! IS_NSM_CODE_PW_FAULT (R) &&                                         \
   ! CHECK_FLAG ((L), NSM_CODE_PW_STANDBY) &&                            \
   ! CHECK_FLAG ((R), NSM_CODE_PW_STANDBY))                              \

#define NSM_MPLS_VC_PRIMARY   (1 << 0)
#define NSM_MPLS_VC_SECONDARY (1 << 1)

struct nsm_mpls_vc_info
{
  union
  {
    struct nsm_mpls_circuit *vc;
#ifdef HAVE_VPLS
    struct nsm_vpls *vpls;
#endif /* HAVE_VPLS */
  }u;

  /* Back pointer to mif */
  struct nsm_mpls_if *mif;
  char *vc_name;
  u_int16_t vc_type;
  u_int16_t vlan_id;
  u_int16_t vc_mode;
  struct nsm_mpls_vc_info *sibling_info;

#define NSM_MPLS_VC_CONF_STANDBY                (1 << 1)
#define NSM_MPLS_VC_CONF_REVERTIVE              (1 << 2) 
#define NSM_MPLS_VC_CONF_WROTE                  (1 << 3)  /* For show */
#define NSM_MPLS_VC_CONF_SECONDARY              (1 << 4)
  /* vc configure flag */
  u_char conf_flag;

#define NSM_MPLS_VC_ACTIVE     0 
#define NSM_MPLS_VC_STANDBY    1
  /* vc running mode */
  u_char run_mode;

  u_int8_t pw_attchd_pw_ix;

  /* vc or vpls */
  u_char if_bind_type;

#ifdef HAVE_SNMP
  /* PW Enet PW Instance */
  u_int32_t pw_enet_pw_instance;
#endif /* HAVE_SNMP */
};

#define NSM_MPLS_VC_IS_SELECTED(V) \
    (CHECK_FLAG ((V)->flags, NSM_MPLS_VC_FLAG_SELECTED))

#define NSM_MPLS_VC_NOT_MAP_TO_FTN(F,V) \
    (!(V) || ((V)->tunnel_id != 0 && (V)->tunnel_id != (F)->tun_id))

/* Prototypes. */
int  nsm_mpls_l2_circuit_add (struct nsm_master *, char *, u_int32_t,
                              struct pal_in4_addr *,char *, u_int32_t, 
                              u_char, u_char, char *, u_char,
#ifdef HAVE_VCCV
                              u_int8_t, u_int8_t,
#endif /* HAVE_VCCV */
                              u_int8_t, u_int8_t);
int  nsm_mpls_l2_circuit_del2 (struct nsm_master *, char *, u_int32_t);
int
nsm_mpls_l2_circuit_del (struct nsm_master *,
                         struct nsm_mpls_circuit *, bool_t);
int  nsm_mpls_l2_circuit_bind_by_ifp (struct interface *, char *, u_int16_t,
                                      u_int16_t, u_int16_t);
int  nsm_mpls_l2_circuit_unbind_by_ifp (struct interface *, char *, u_int16_t);
void nsm_mpls_l2_circuit_del_from_fwd (u_int32_t, struct interface *, u_int16_t);
void nsm_mpls_l2_circuit_delete_all (struct nsm_master *, bool_t);
struct nsm_mpls_circuit *nsm_mpls_l2_circuit_lookup_by_name (struct nsm_master *, char *);
struct nsm_mpls_circuit *nsm_mpls_l2_circuit_lookup_by_id (struct nsm_master *,
                                                           u_int32_t);
#ifdef HAVE_SNMP
struct nsm_mpls_circuit *
nsm_mpls_l2_circuit_snmp_lookup_by_index (struct nsm_master *,
                                          u_int32_t);
struct nsm_mpls_circuit *
nsm_mpls_l2_circuit_lookup_next_by_id (struct nsm_master *nm, u_int32_t vc_id);

struct nsm_mpls_circuit_temp *
nsm_mpls_l2_circuit_temp_lookup_next (u_int32_t ix);

struct nsm_mpls_circuit_temp *
nsm_mpls_l2_circuit_snmp_temp_lookup_by_index (u_int32_t ix);
struct nsm_mpls_circuit_temp *
nsm_mpls_l2_circuit_temp_snmp_lookup_next (u_int32_t ix);
#endif /*HAVE_SNMP */

/* VC Timer */
s_int32_t vc_timer (struct thread *);
/* Free VC Entry */
void
mpls_vc_free (struct nsm_mpls_circuit *);

struct nsm_mpls_vc_group *nsm_mpls_l2_circuit_group_lookup_by_id (struct nsm_master *, u_int32_t);
struct nsm_mpls_vc_group *nsm_mpls_l2_vc_group_lookup_by_name (struct nsm_master *, char *);
int nsm_mpls_if_install_vc_data (struct nsm_mpls_if *, 
                                 struct nsm_mpls_circuit *, u_int16_t);
void nsm_mpls_if_withdraw_vc_data (struct nsm_mpls_if *,
                                   struct nsm_mpls_circuit *, bool_t);
void nsm_mpls_vc_info_add (struct nsm_mpls_if *, struct list *,
                           struct nsm_mpls_vc_info *);
void nsm_mpls_vc_info_del (struct nsm_mpls_if *, struct list *, 
                           struct nsm_mpls_vc_info *);
struct nsm_mpls_vc_info *nsm_mpls_vc_info_create (char *, struct nsm_mpls_if *, 
                                                  u_int16_t, u_int16_t,u_int16_t, u_char);
void nsm_mpls_vc_info_free (struct nsm_mpls_vc_info *); 
int nsm_mpls_l2_circuit_bind_check (struct nsm_mpls_if *, u_int16_t, char *,
                                    u_int16_t, u_int16_t, u_char,
                                    struct nsm_mpls_vc_info **);
#ifdef HAVE_VLAN
void nsm_mpls_vc_vlan_bind (struct interface *, u_int16_t);
void nsm_mpls_vc_vlan_unbind (struct interface *, u_int16_t);
#endif /* HAVE_VLAN */
struct nsm_mpls_circuit *
nsm_mpls_l2_circuit_new (struct nsm_master *, char *, u_int8_t);
int nsm_mpls_l2_circuit_add_to_fwd (u_int32_t, struct interface *,
                                        u_int16_t);
void nsm_mpls_vc_if_up_process (struct nsm_mpls_circuit *,
                                struct nsm_mpls_if *);
void nsm_mpls_vc_if_down_process (struct nsm_mpls_circuit *,
                                  struct nsm_mpls_if *);
int nsm_mpls_l2_circuit_runtime_mode_conf (struct interface *, u_char, 
                                           u_int16_t);
int nsm_mpls_l2_circuit_revertive_mode_conf (struct interface *, u_char, 
                                             u_int16_t);
int nsm_mpls_vc_secondary_qualified (struct nsm_mpls_circuit *);
int nsm_mpls_vc_revert (struct nsm_mpls_circuit *, int primary);
int nsm_mpls_api_pw_switchover (u_int32_t, char *, char *);
 
int nsm_mpls_get_cc_type_in_use (struct nsm_mpls_circuit *);
int nsm_mpls_get_bfd_cv_type_in_use (struct nsm_mpls_circuit *);
void nsm_mpls_set_cc_types (u_int8_t*, char *);
u_int8_t nsm_mpls_get_default_cc_types (bool_t);
char * nsm_mpls_get_cctype_from_bit (u_int8_t);
char * nsm_mpls_get_bfd_cvtype_from_bit (u_int8_t);
u_int8_t nsm_mpls_get_default_cv_types ();
u_int8_t nsm_mpls_get_default_bfd_cv_types (bool_t, bool_t);
bool_t nsm_mpls_is_bfd_set (u_int8_t cv_types);
bool_t nsm_mpls_is_specific_bfd_in_use (u_int8_t, bool_t, bool_t);

u_int32_t nsm_mpls_vc_hash_key_make (char *);
bool_t nsm_mpls_vc_hash_cmp (struct nsm_mpls_circuit_container *, char *);
int nsm_mpls_vc_hash_init (struct nsm_master *);
void * nsm_mpls_vc_hash_alloc (char *);
void nsm_mpls_vc_remove_from_hash (struct nsm_master *, char *);
struct nsm_mpls_circuit_container *nsm_mpls_vc_lookup_by_name (struct nsm_master* ,
                                                char *);
struct nsm_mpls_circuit_container * nsm_mpls_vc_get_by_name (struct nsm_master* , char *);
struct nsm_mpls_circuit * nsm_mpls_l2_circuit_create (struct nsm_master *,
                                                      char *, u_int32_t,
                                                      u_int8_t, int *);
int nsm_mpls_vc_fill_data (struct nsm_master *nm,
                           u_int32_t vc_id,
                           struct pal_in4_addr *egress,
                           char *group_name, u_int32_t group_id,
                           u_char c_word,
                           u_char mapping_type,
                           char *tunnel_info, 
                           u_char tunnel_dir,
                           u_int8_t fec_type_vc,
#ifdef HAVE_VCCV
                           u_int8_t cc_types, u_int8_t cv_types,
#endif /* HAVE_VCCV */
                           struct nsm_mpls_circuit *vc);

/* End Prototypes. */

struct nsm_mpls_vc_group *
nsm_mpls_l2_vc_group_lookup_by_id (struct nsm_master *nm, u_int32_t group_id);

struct nsm_mpls_vc_group *
nsm_mpls_l2_vc_group_lookup_by_name (struct nsm_master *nm, char *name);

int
nsm_mpls_ac_group_create (struct nsm_master *nm, char *name,
                          u_int32_t group_id);

struct nsm_mpls_vc_group *
nsm_mpls_l2_vc_group_create (struct nsm_master *nm, char *name, 
                             u_int32_t group_id );

int
nsm_mpls_l2_vc_group_delete (struct nsm_master *nm,
                              struct nsm_mpls_vc_group *group );

#ifdef HAVE_VPLS
void 
nsm_vpls_group_unregister (struct nsm_master *, struct nsm_vpls *);

void 
nsm_vpls_group_register (struct nsm_master *, struct nsm_mpls_vc_group *,
                          struct nsm_vpls *);
#endif /* HAVE_VPLS */

#endif /* _NSM_MPLS_VC_H */
