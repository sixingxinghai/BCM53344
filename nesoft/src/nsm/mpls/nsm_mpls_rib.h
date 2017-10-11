/* Copyright (C) 2002  All Rights Reserved. */

#ifndef _NSM_MPLS_RIB_H
#define _NSM_MPLS_RIB_H 

#define IPV4_FTN_KEY_BITLEN      IPV4_MAX_BITLEN
#define IPV4_NHLFE_KEY_BITLEN    (IPV4_MAX_BITLEN + \
                                 sizeof (u_int32_t) * 8 + MAX_LABEL_BITLEN)

#ifdef HAVE_GMPLS
#ifdef HAVE_PBB_TE 

#define PBB_TE_LABEL_BITLEN      64
#define PBB_TE_NHLE_KEY_BITLEN   sizeof (u_int32_t) * 8 + PBB_TE_LABEL_BITLEN
#endif /* HAVE_PBB_TE */
#endif /* HAVE_GMPLS */

#ifdef HAVE_IPV6
#define IPV6_FTN_KEY_BITLEN      IPV6_MAX_BITLEN
#define IPV6_NHLFE_KEY_BITLEN    (IPV6_MAX_BITLEN + \
                                 sizeof (u_int32_t) * 8 + MAX_LABEL_BITLEN)
#define MAPPED_ROUTE_KEY_BITLEN  IPV6_MAX_BITLEN
#else
#define MAPPED_ROUTE_KEY_BITLEN  IPV4_MAX_BITLEN
#endif /* HAVE_IPV6 */

#define ILM_KEY_BITLEN     (sizeof (u_int32_t) * 8 + MAX_LABEL_BITLEN)
#define XC_KEY_BITLEN      (sizeof (u_int32_t ) * 4 * 8)

#define NSM_ENTRY_FOUND         1
#define NSM_ENTRY_NOT_FOUND     0

/* RSVP Tunnel type identifiers */
#define RSVP_TUNNEL_NAME        1
#define RSVP_TUNNEL_ID          2

/* The following is used to print out correct nhlfe opcodes. */
typedef enum mpls_nhlfe_owner
{
  MPLS_NHLFE_OWNER_UNKNOWN = 0,
  MPLS_NHLFE_OWNER_ILM,
  MPLS_NHLFE_OWNER_FTN,
  MPLS_NHLFE_OWNER_NHLFE
} mpls_nhlfe_owner_t;

/* Priority defines for LSPs. */
typedef enum mpls_lsp_priority
  {
    MPLS_P_CLI = 0, /* Highest */
    MPLS_P_RSVP,
    MPLS_P_IGP_SHORTCUT,
    MPLS_P_CRLDP,
    MPLS_P_LDP,
    MPLS_P_NONE
  } mpls_lsp_priority_t;

#define LSP_PRIORITY_COUNT   5

typedef enum mpls_opr_status
  {
    OPR_UP = 1,
    OPR_DOWN,
    OPR_TESTING,
    OPR_UNKNOWN,
    OPR_DORMANT,
    OPR_NOT_PRESENT,
    OPR_LL_DOWN
  } mpls_opr_status_t;

typedef enum mpls_storage_type
  {
    ST_OTHER = 1,
    ST_VOLATILE,
    ST_NONVOLATILE,
    ST_PERMANENT,
    ST_READONLY
  } mpls_storage_type_t;

typedef enum mpls_ftn_type
  {
    MPLS_FTN_NONE = 1,
    MPLS_FTN_REGULAR, 
    MPLS_FTN_MAPPED,
    MPLS_FTN_RSVP_MAPPED,
    MPLS_FTN_GMPLS_RSVP_MAPPED,
    MPLS_FTN_IGP_SHORTCUT,
    MPLS_FTN_BGP, 
    MPLS_FTN_BGP_VRF,
  } mpls_ftn_type_t;

/* Key for XC table */
struct xc_key
{
  u_int32_t xc_ix;
  u_int32_t iif_ix;
  struct gmpls_gen_label in_label;
  u_int32_t nhlfe_ix;
};

typedef struct rsvp_key TE_MIB_ROW_KEY;

/* Key for ILM table */
struct ilm_key
{
  u_int32_t iif_ix;
  u_int32_t in_label;
};

#ifdef HAVE_GMPLS
#ifdef HAVE_PBB_TE
struct ilm_key_pbb
{
  u_int32_t iif_ix;
  struct pbb_te_label in_label;
};
#endif /* HAVE_PBB_TE */

#ifdef HAVE_TDM
struct ilm_key_tdm
{
  u_int32_t iif_ix;
  struct tdm_label in_label;
};
#endif /* HAVE_TDM */
#endif /* HAVE_GMPLS */

struct ilm_key_gen
{
  union {
    struct ilm_key pkt;
#ifdef HAVE_GMPLS
#ifdef HAVE_PBB_TE
    struct ilm_key_pbb pbb_key;
#endif /* HAVE_PBB_TE */

#ifdef HAVE_TDM
    struct ilm_key_tdm tdm_key;
#endif /* HAVE_TDM */
#endif /* HAVE_GMPLS */
    char val [1];
  } u;
};

/* Key for NHLFE table for IPV4 */
struct nhlfe_key_ipv4
{
  struct pal_in4_addr nh_addr;
  u_int32_t oif_ix;
  u_int32_t out_label;
};

/* Key for NHLFE table for IPV6 */
#ifdef HAVE_IPV6
struct nhlfe_key_ipv6
{
  struct pal_in6_addr nh_addr;
  u_int32_t oif_ix;
  u_int32_t out_label;
};
#endif /* HAVE_IPV6 */

struct nhlfe_key_unnum
{
  /* Any id for the neighbor */
  u_int32_t id;
  u_int32_t oif_ix;
  u_int32_t out_label;
};

/* Generic Key for NHLFE table */
struct nhlfe_key 
{
  afi_t afi;
  union
  {
    struct nhlfe_key_ipv4 ipv4;
#ifdef HAVE_IPV6
    struct nhlfe_key_ipv6 ipv6;
#endif /* HAVE_IPV6 */
#ifdef HAVE_GMPLS
    struct nhlfe_key_unnum unnum;
#endif /* HAVE_GMPLS */
    u_char val;
  } u;
};

#ifdef HAVE_GMPLS
#ifdef HAVE_PBB_TE
struct nhlfe_pbb_key
{
  u_int32_t oif_ix;
  struct pbb_te_label lbl;
};
#endif /* HAVE_PBB_TE */

#ifdef HAVE_TDM
struct nhlfe_tdm_key
{
  u_int32_t oif_ix;
  struct tdm_label lbl;
};
#endif /* HAVE_TDM */
#endif /* HAVE_GMPLS */

struct nhlfe_key_gen
{
  union {
      struct nhlfe_key pkt;
#ifdef HAVE_GMPLS
#ifdef HAVE_PBB_TE
      struct nhlfe_pbb_key pbb;
#endif /* HAVE_PBB_TE */
#ifdef HAVE_TDM
      struct nhlfe_tdm_key tdm;
#endif /* HAVE_TDM */
#endif /* HAVE_GMPLS */
    } u;
};

/* FTN Table Entry */
struct ftn_entry
{
  /* Unique FTN index */
  u_int32_t ftn_ix; 
  
  struct mpls_owner owner;

  u_int32_t exp_bits;
  u_int32_t tun_id;
  u_int32_t protected_lsp_id;
  u_int32_t qos_resrc_id;
  struct xc_entry *xc;

#if (!defined (HAVE_GMPLS) || defined (HAVE_PACKET))
  /* For bidirectional LSP the reverse entry */
  struct ilm_entry *ilm_rev;
#endif /* !HAVE_GMPLS || HAVE_PACKET */

  union
  {
    struct ftn_lsp_bits
    {
                       /* LSP type */
      MPLS_BITFIELDS4 (u_int32_t lsp_type :8, 
                       /* Redirect to LSP or TE Tunnel */
                       u_int32_t act_type :4,
                       u_int32_t igp_shortcut :4,
                       /* igp-shortcut lsp_metric */
                       u_int32_t lsp_metric :16);
    } bits;

    u_int32_t data;
  } lsp_bits;

  /* lookup key for TE MIB entry used only if action type is redirect
     TE tunnel.  */
  TE_MIB_ROW_KEY te_mib_key;

  mpls_row_status_t row_status;
  char *sz_desc;

  /* For rsvp mapped route - for non Packet this will always be set */
  struct prefix *pri_fec_prefix;

  mpls_ftn_type_t ftn_type;

  /* Bypass FTN index */
  u_int32_t bypass_ftn_ix;

#if (!defined (HAVE_GMPLS) || defined (HAVE_PACKET))
#ifdef HAVE_DIFFSERV
  struct ds_info ds_info;
#endif /* HAVE_DIFFSERV */
#endif /* !HAVE_GMPLS || HAVE_PACKET */

#define FTN_ENTRY_FLAG_DEPENDENT              (1 << 0)
#define FTN_ENTRY_FLAG_SELECTED               (1 << 1)
#define FTN_ENTRY_FLAG_INSTALLED              (1 << 2)
#define FTN_ENTRY_FLAG_PRIMARY                (1 << 3)
#define FTN_ENTRY_FLAG_STATIC                 (1 << 4)
#define FTN_ENTRY_FLAG_STALE                  (1 << 5)
#define FTN_ENTRY_FLAG_INACTIVE               (1 << 6)

#define FTN_ENTRY_FLAG_BIDIR                  (1 << 7)
#define FTN_ENTRY_FLAG_DUMMY                  (1 << 8)
#define FTN_ENTRY_FLAG_GMPLS                  (1 << 9)
#define FTN_ENTRY_FLAG_BEING_INSTALLED        (1 << 10)
#define FTN_ENTRY_FLAG_FWD                    (1 << 11)

  u_int16_t flags;

  /* Entry type - TDM, IPv4, IPv6, PBB-TE */
#define FTN_ENTRY_TYPE_UNKNOWN           0
#define FTN_ENTRY_TYPE_IPV4              1
#define FTN_ENTRY_TYPE_IPV6              2
#define FTN_ENTRY_TYPE_PBB_TE            3
#define FTN_ENTRY_TYPE_TDM               4
  u_char ent_type;

#define FTN_ENTRY_IGP_STATUS_NONE         0
#define FTN_ENTRY_IGP_STATUS_CANDIDATE    1
#define FTN_ENTRY_IGP_STATUS_ADVERTISED   2
  u_char igp_status;

  u_char protocol;
  u_char dscp_in;

#ifdef HAVE_MPLS_OAM
  bool_t is_oam_enabled;
#endif /* HAVE_MPLS_OAM */

#if defined (HAVE_BFD) && defined (HAVE_MPLS_OAM)
  /* BFD flags. */
#define NSM_MPLS_BFD_CONFIGURED    (1 << 0)
#define NSM_MPLS_BFD_ENABLED       (1 << 1)
#define NSM_MPLS_BFD_DISABLED      (1 << 2)

  u_char bfd_flag;
#endif /* HAVE_BFD  && HAVE_MPLS_OAM */

  /* Next pointer. */
  struct ftn_entry *next;

  /* pointer to route_node in FTN_TABLE */
  struct ptree_node *pn;

#ifdef HAVE_GMPLS 
#if defined (HAVE_PBB_TE) || defined (HAVE_TDM)
  /* For non-IPv4 or IPv6 entries the avl entry is used */
  struct avl_node *an;
#endif /* HAVE_PBB_TE || HAVE_TDM */
#endif /* HAVE_GMPLS */

#ifdef HAVE_MPLS_FWD
  /* FTN entry stats. */
  pal_ftn_entry_stats_t stats;
#endif /* HAVE_MPLS_FWD */

 /* DestAddr set by SNMP */
 struct pal_in4_addr dst_addr;

  /* For storing Tech specific data */
  void *tgen_data;

  struct bidirectional_entry *bde;
};

/* ILM Table Entry */
struct ilm_entry
{
  /* lookup key for ILM entries */
  struct ilm_key_gen key;

  struct mpls_owner owner;

  u_int32_t ilm_ix;

  u_int32_t n_pops;
  u_int32_t qos_resrc_id;
  struct xc_entry *xc;

  /* Pointer to the AVL node */
  struct avl_node *an;

#ifdef HAVE_GMPLS
  /* For bidirectional LSP the reverse entry */
  void *rev_entry;
#endif /* HAVE_GMPLS */

#ifdef HAVE_DIFFSERV
  struct ds_info ds_info;
#endif /* HAVE_DIFFSERV */

  mpls_row_status_t row_status;

#define ILM_ENTRY_FLAG_PRIMARY                       (1 << 0)
#define ILM_ENTRY_FLAG_INSTALLED                     (1 << 1)
#define ILM_ENTRY_FLAG_SELECTED                      (1 << 2)
#define ILM_ENTRY_FLAG_MAPPED                        (1 << 3)
#define ILM_ENTRY_FLAG_STATIC                        (1 << 4)
#define ILM_ENTRY_FLAG_STALE                         (1 << 5)
#define ILM_ENTRY_FLAG_DEPENDENT                     (1 << 6)
#define ILM_ENTRY_FLAG_INACTIVE                      (1 << 7)

#ifdef HAVE_RESTART  
#define ILM_ENTRY_FLAG_OUT_LBL_NOT_REFRESHED         (1 << 8) 
/* Flag used in restarting router.
 * Set when stale entry is updated. For ILM del msgs if retain 
 * flag is set, then entry moves back to STALE state and does
 * not delete the entry as other LSPs might be dependent 
 * on this entry.
 */
#define ILM_ENTRY_FLAG_RETAIN_STALE                  (1 << 9)
#endif /* HAVE_RESTART */  

  /* This is for GMPLS where an inactive add is done. This is because some
   physical motions may be required to program an entry - related to 
   suggested label */
#define ILM_ENTRY_FLAG_ADD_INACTIVE                  (1 << 10)

  /* Add a dummy entry not to be programmed in the hardware */
#define ILM_ENTRY_FLAG_DUMMY                         (1 << 11)
  /* Set for all GMPLS entries */
#define ILM_ENTRY_FLAG_GMPLS                         (1 << 12)
#define ILM_ENTRY_FLAG_BIDIR                         (1 << 13)
  /* This will tell if the reverse entry is FTN or ILM */
#define ILM_ENTRY_FLAG_REV_FTN                       (1 << 14)
#define ILM_ENTRY_FLAG_REV_ILM                       (1 << 15)
#define ILM_ENTRY_FLAG_BEING_INSTALLED               (1 << 16)
#define ILM_ENTRY_FLAG_FWD                           (1 << 17)

  u_int32_t flags;
  u_int8_t family;
  u_int8_t prefixlen;

#define ILM_ENTRY_TYPE_UNKNOWN           0
#define ILM_ENTRY_TYPE_PACKET            1
#define ILM_ENTRY_TYPE_PBB_TE            2
#define ILM_ENTRY_TYPE_TDM               3
  u_char ent_type;

  /* Next pointer */
  struct ilm_entry *next;

#ifdef HAVE_MPLS_FWD
  /* ILM entry stats. */
  pal_ilm_entry_stats_t stats;
#endif /* HAVE_MPLS_FWD */

#ifdef HAVE_RESTART
  /* Waiting for out label during GR */
  struct ilm_add_gen_data *partial;
#endif /* HAVE_RESTART */

  /* For storing Tech specific data */
  void *tgen_data;

  struct bidirectional_entry *bde;

  u_int8_t prfx [1];
};

/* XC  Table Entry */
struct xc_entry
{
  struct xc_key key;
  u_int32_t ilm_ix;
  struct mpls_lspid lspid;
  struct nhlfe_entry *nhlfe;
  
  mpls_owner_t owner;
  mpls_row_status_t row_status;
  mpls_admn_status_t admn_status;
  mpls_opr_status_t opr_status;
  u_int32_t refcount;
  u_int32_t oprcount;
  u_int32_t admncount;
  bool_t persistent;

#define XC_ENTRY_FLAG_OPR_STS_CHG       (1 << 0)
#define XC_ENTRY_FLAG_GMPLS             (1 << 1)

  u_char flags;

  /* lookup key for XC entries */
  enum gmpls_entry_type type;
};

struct nhlfe_entry
{
  u_int32_t nhlfe_ix;
  u_int32_t xc_ix;
  mpls_owner_t owner;
  mpls_row_status_t row_status;
  u_char opcode;
  u_int32_t refcount;

#ifdef HAVE_MPLS_FWD
  /* NHLFE entry stats. */
  pal_nhlfe_entry_stats_t stats;
#endif /* HAVE_MPLS_FWD */

#define NHLFE_ENTRY_FLAG_GMPLS             (1 << 0)

  u_char flags;

  /* lookup key for NHLFE entries */
  enum gmpls_entry_type type;
  u_char nkey [1];
};

/* Temporary structure required for searches */
struct nhlfe_entry_tmp
{
  u_int32_t nhlfe_ix;
  u_int32_t xc_ix;
  mpls_owner_t owner;
  mpls_row_status_t row_status;
  u_char opcode;
  u_int32_t refcount;

#ifdef HAVE_MPLS_FWD
  /* NHLFE entry stats. */
  pal_nhlfe_entry_stats_t stats;
#endif /* HAVE_MPLS_FWD */

  u_char flags; 

  /* lookup key for NHLFE entries */
  enum gmpls_entry_type type;
  struct nhlfe_key_gen key;
};

struct xc_new_data
{
  mpls_owner_t owner;
  u_int32_t iif_ix;
  struct gmpls_gen_label in_label;
  struct mpls_lspid lspid;
#define XC_DATA_FLAG_GMPLS              (1 << 0) 

  u_int16_t flags;
};

struct nhlfe_new_data 
{
  mpls_owner_t owner;
  u_char opcode;
  u_int32_t oif_ix;
  struct gmpls_gen_label out_label;
  struct addr_in nh_addr;
#define NHD_DATA_FLAG_GMPLS              (1 << 0)

  u_int16_t flags;
};

/* Mapped routes. */
struct mapped_route
{
  /* Entry owner. */
  mpls_owner_t owner;

  /* FTN FEC. */
  struct prefix fec;

  /* FTN index. If this is non-zero, use this to delete. */
  u_int32_t ix;

#define MAPPED_ENTRY_TYPE_UNKNOWN           0
#define MAPPED_ENTRY_TYPE_IPv4              1
#define MAPPED_ENTRY_TYPE_IPV6              2
#define MAPPED_ENTRY_TYPE_PBB_TE            3
#define MAPPED_ENTRY_TYPE_TDM               4
  u_int32_t type;
};

struct mapped_lsp_entry_pkt
{
  enum gmpls_entry_type type;
  u_int32_t iif_ix;
  struct prefix fec;
  u_int32_t out_label;
  u_int32_t in_label;
};

#ifdef HAVE_GMPLS
#ifdef HAVE_PBB_TE
struct mapped_lsp_entry_pbb
{
  enum gmpls_entry_type type;
  u_int32_t iif_ix;
  struct fec_entry_pbb_te fec;
  struct pbb_te_label out_label;
  struct pbb_te_label in_label;
};
#endif /* HAVE_PBB_TE */

#ifdef HAVE_TDM
struct mapped_lsp_entry_tdm
{
  enum gmpls_entry_type type;
  u_int32_t iif_ix;
  struct fec_entry_tdm fec;
  struct tdm_label out_label;
  struct tdm_label in_label;
};

#endif /* HAVE_TDM */
#endif /* HAVE_GMPLS */

struct mapped_lsp_entry
{
  union 
   {
     struct mapped_lsp_entry_pkt pkt;
#ifdef HAVE_GMPLS
#ifdef HAVE_TDM
     struct mapped_lsp_entry_tdm tdm;
#endif /* HAVE_TDM */
#ifdef HAVE_PBB_TE
     struct mapped_lsp_entry_pbb pbb;
#endif /* HAVE_PBB_TE */
#endif /* HAVE_GMPLS */
   } u;
};

/* The head structure should not be different than the common elements of the 
   mapped entry */
struct mapped_lsp_entry_head
{
  enum gmpls_entry_type type;
  u_int32_t iif_ix;
};

#define FTN_TABLE4           NSM_MPLS->ftn_ix_table4.m_table
#ifdef HAVE_IPV6
#define FTN_TABLE6           NSM_MPLS->ftn_ix_table6.m_table
#endif /* HAVE_IPV6 */
#ifdef HAVE_GMPLS
#ifdef HAVE_PBB_TE
#define FTN_TABLE_PBB_TE     NSM_MPLS->ftn_pbb_table.m_table
#endif /* HAVE_PBB_TE */
#ifdef HAVE_TDM
#define FTN_TABLE_TDM        NSM_MPLS->ftn_tdm_table.m_table
#endif /* HAVE_TDM */
#endif /* HAVE_GMPLS */

#define GET_FIRST_FTN_TABLE(tbl)      {                                    \
   (tbl) = NULL;                                                           \
   nsm_mpls_tree_array_get (nm, (void **)(&(tbl)),                         \
                            NSM_MPLS_TREE_ARRAY_FTN_TREES, 0);             \
}

#define GET_NEXT_FTN_TABLE(tbl, nxt_tbl)   {                               \
   nxt_tbl = NULL;                                                         \
   nsm_mpls_tree_array_get_next (nm, tbl, (void **)(&(nxt_tbl)),           \
                                 NSM_MPLS_TREE_ARRAY_FTN_TREES);           \
}

#define GET_FIRST_FTN_IX_TABLE(tbl)  {                                     \
  tbl = NULL;                                                              \
  nsm_mpls_tree_array_get (nm, (void **)(&(tbl)),                          \
                           NSM_MPLS_TREE_ARRAY_FTN_IX_TREES, 0);           \
}

#define GET_NEXT_FTN_IX_TABLE(tbl, nxt_tbl)   {                            \
   nxt_tbl = NULL;                                                         \
   nsm_mpls_tree_array_get_next (nm, tbl, (void **)(&(nxt_tbl)),           \
                                 NSM_MPLS_TREE_ARRAY_FTN_IX_TREES);        \
}

/* All tables will use the same index (IX) manager */
#define ILM_TABLE           NSM_MPLS->ilm_ix_table.m_table
#define ILM_IX_MGR          NSM_MPLS->ilm_ix_table.ix_mgr

#define ILM_PBB_TABLE       NSM_MPLS->pbb_ilm_ix.m_table

#define ILM_TDM_TABLE       NSM_MPLS->tdm_ilm_ix.m_table

#define GET_FIRST_ILM_TABLE(tbl)           {                                \
  tbl = NULL;                                                               \
  nsm_mpls_tree_array_get (nm, (void **)(&(tbl)),                           \
                           NSM_MPLS_TREE_ARRAY_ILM_TREES, 0);               \
}

#define GET_NEXT_ILM_TABLE(tbl, nxt_tbl)   {                                \
   nxt_tbl = NULL;                                                          \
   nsm_mpls_tree_array_get_next (nm, tbl, (void **)(&(nxt_tbl)),            \
                                 NSM_MPLS_TREE_ARRAY_ILM_TREES);            \
}

#define GET_FIRST_ILM_IX_TABLE(tbl)       {                                 \
  tbl = NULL;                                                               \
  nsm_mpls_tree_array_get (nm, (void **)(&(tbl)),                           \
                           NSM_MPLS_TREE_ARRAY_ILM_IX_TREES, 0);            \
}

#define GET_NEXT_ILM_IX_TABLE(tbl, nxt_tbl)   {                             \
   nxt_tbl = NULL;                                                          \
   nsm_mpls_tree_array_get_next (nm, tbl, (void **)(&(nxt_tbl)),            \
                                 NSM_MPLS_TREE_ARRAY_ILM_IX_TREES);         \
}

#define GET_FIRST_MAPPED_ROUTE_TABLE(tbl)  {                                \
  tbl = NULL;                                                               \
  nsm_mpls_tree_array_get (nm, (void **)(&(tbl)),                           \
                           NSM_MPLS_TREE_ARRAY_MAPROUTE_TREES, 0);          \
}

#define GET_NEXT_MAPPED_ROUTE_TABLE(tbl, nxt_tbl)   {                      \
   nxt_tbl = NULL;                                                         \
   nsm_mpls_tree_array_get_next (nm, tbl, (void **)(&(nxt_tbl)),           \
                                 NSM_MPLS_TREE_ARRAY_MAPROUTE_TREES);      \
}

#define FTN_XC(f)           (f)->xc
#define FTN_NHLFE(f)        (f)->xc->nhlfe
#define FTN_XC_IX(f)        (f)->xc->key.xc_ix
#define FTN_NHLFE_IX(f)     (f)->xc->nhlfe->nhlfe_ix
#define ILM_IX(i)           (i)->ilm_ix
#define ILM_XC(i)           (i)->xc
#define ILM_NHLFE(i)        (i)->xc->nhlfe
#define ILM_XC_IX(i)        (i)->xc->key.xc_ix
#define ILM_NHLFE_IX(i)     (i)->xc->nhlfe->nhlfe_ix

/* All the tables will use the same IX manager*/
#define NHLFE_TABLE4        NSM_MPLS->nhlfe_ix_table4.m_table
#ifdef HAVE_IPV6
#define NHLFE_TABLE6        NSM_MPLS->nhlfe_ix_table6.m_table
#endif

#define NHLFE_PBB_TABLE     NSM_MPLS->pbb_nhlfe_ix.m_table

#define NHLFE_TDM_TABLE     NSM_MPLS->tdm_nhlfe_ix.m_table

#define GET_FIRST_NHLFE_TABLE(tbl)       {                                 \
  tbl = NULL;                                                              \
  nsm_mpls_tree_array_get (nm, (void **)(&(tbl)),                          \
                           NSM_MPLS_TREE_ARRAY_NHLFE_TREES, 0);            \
}

#define GET_NEXT_NHLFE_TABLE(tbl, nxt_tbl)   {                             \
  nxt_tbl = NULL;                                                          \
  nsm_mpls_tree_array_get_next (nm, tbl, (void **)(&(nxt_tbl)),            \
                                NSM_MPLS_TREE_ARRAY_NHLFE_TREES);          \
}

#define NHLFE_IX_MGR        NSM_MPLS->nhlfe_ix_table4.ix_mgr

#define XC_TABLE            NSM_MPLS->xc_ix_table.m_table
#define XC_IX_MGR           NSM_MPLS->xc_ix_table.ix_mgr
#define XC_IX(x)            (x)->key.xc_ix
#define XC_NHLFE_IX(x)      (x)->nhlfe->nhlfe_ix
#define VRF_TABLE4(v)        (v)->vrf_ix_table4.m_table
#ifdef HAVE_IPV6
#define VRF_TABLE6(v)        (v)->vrf_ix_table6.m_table
#endif
#define VC_FTN_TABLE        NSM_MPLS->vc_ftn_table

#define ILM_ROW_STATUS_ACTIVATE(ie) \
do { \
  (ie)->row_status = RS_ACTIVE; \
  if ((ie)->xc) \
    { \
      UNSET_FLAG ((ie)->xc->flags, XC_ENTRY_FLAG_OPR_STS_CHG); \
      (ie)->xc->row_status = RS_ACTIVE; \
      if (CHECK_FLAG ((ie)->flags, ILM_ENTRY_FLAG_SELECTED)) \
        (ie)->xc->admn_status = ADMN_UP; \
      else \
        (ie)->xc->admn_status = ADMN_DOWN; \
      if (CHECK_FLAG ((ie)->flags, ILM_ENTRY_FLAG_INSTALLED) && \
         (ie)->xc->opr_status != OPR_UP) \
         { \
           (ie)->xc->opr_status = OPR_UP; \
           SET_FLAG ((ie)->xc->flags, XC_ENTRY_FLAG_OPR_STS_CHG); \
         } \
      else \
      if (! (CHECK_FLAG ((ie)->flags, ILM_ENTRY_FLAG_INSTALLED)) && \
          (ie)->xc->opr_status != OPR_DOWN) \
        { \
           (ie)->xc->opr_status = OPR_DOWN; \
           SET_FLAG ((ie)->xc->flags, XC_ENTRY_FLAG_OPR_STS_CHG); \
        } \
      if ((ie)->xc->nhlfe) \
        (ie)->xc->nhlfe->row_status = RS_ACTIVE; \
    } \
} while (0)

#define ILM_XC_OPER_STATUS_UPDATE(ie) \
do { \
  if ((ie)->xc) \
    { \
      UNSET_FLAG ((ie)->xc->flags, XC_ENTRY_FLAG_OPR_STS_CHG); \
      if (CHECK_FLAG ((ie)->flags, ILM_ENTRY_FLAG_INSTALLED) && \
         (ie)->xc->opr_status != OPR_UP) \
         { \
           (ie)->xc->opr_status = OPR_UP; \
           SET_FLAG ((ie)->xc->flags, XC_ENTRY_FLAG_OPR_STS_CHG); \
         } \
      else \
      if (! (CHECK_FLAG ((ie)->flags, ILM_ENTRY_FLAG_INSTALLED)) && \
          (ie)->xc->opr_status != OPR_DOWN) \
        { \
           (ie)->xc->opr_status = OPR_DOWN; \
           SET_FLAG ((ie)->xc->flags, XC_ENTRY_FLAG_OPR_STS_CHG); \
        } \
    } \
} while (0)

#define FTN_ROW_STATUS_ACTIVATE(fe) \
do { \
  (fe)->row_status = RS_ACTIVE; \
  if ((fe)->xc) \
    { \
      (fe)->xc->row_status = RS_ACTIVE; \
      if ((fe)->xc->nhlfe) \
        (fe)->xc->nhlfe->row_status = RS_ACTIVE; \
    } \
} while (0)

#define ROUTE_IX_TABLE_CLEANUP(t) \
do { \
  if ((t)->m_table) \
    { \
      route_table_finish ((t)->m_table); \
      (t)->m_table = NULL; \
    } \
  if ((t)->ix_mgr) \
    { \
      bitmap_free ((t)->ix_mgr); \
      (t)->ix_mgr = NULL; \
    } \
} while (0)

#define PTREE_IX_TABLE_CLEANUP(t) \
do { \
  if ((t)->m_table) \
    { \
      ptree_finish ((t)->m_table); \
      (t)->m_table = NULL; \
    } \
  if ((t)->ix_mgr) \
    { \
      bitmap_free ((t)->ix_mgr); \
      (t)->ix_mgr = NULL; \
    } \
} while (0)

#define AVL_IX_TABLE_CLEANUP(t) \
do { \
  if ((t)->m_table) \
    { \
      avl_tree_free ((&((t)->m_table)), NULL); \
      (t)->m_table = NULL; \
    } \
  if ((t)->ix_mgr) \
    { \
      bitmap_free ((t)->ix_mgr); \
      (t)->ix_mgr = NULL; \
    } \
} while (0)

#define FTN_XC_STATUS_UPDATE(fe) \
do { \
  if ((fe)->xc) \
    { \
      UNSET_FLAG ((fe)->xc->flags, XC_ENTRY_FLAG_OPR_STS_CHG); \
      if (! CHECK_FLAG ((fe)->flags, FTN_ENTRY_FLAG_INSTALLED)) \
        { \
          if ((fe)->xc->oprcount == 0) \
            { \
              if ((fe)->xc->opr_status != OPR_DOWN) \
                SET_FLAG ((fe)->xc->flags, XC_ENTRY_FLAG_OPR_STS_CHG); \
              (fe)->xc->opr_status = OPR_DOWN; \
            } \
        } \
      else \
        { \
          if ((fe)->xc->opr_status != OPR_UP) \
            { \
                (fe)->xc->opr_status = OPR_UP; \
                SET_FLAG ((fe)->xc->flags, XC_ENTRY_FLAG_OPR_STS_CHG); \
            } \
        } \
      if (! CHECK_FLAG ((fe)->flags, FTN_ENTRY_FLAG_SELECTED)) \
        { \
          if ((fe)->xc->admncount == 0) \
            (fe)->xc->admn_status = ADMN_DOWN; \
        } \
      else \
        { \
          if ((fe)->xc->admn_status != ADMN_UP) \
            (fe)->xc->admn_status = ADMN_UP; \
        } \
    } \
} while (0)

#define FTN_XC_ADMN_STATUS_COUNT_INC(f) \
do { \
  if ((f)->xc) \
    (f)->xc->admncount++; \
} while (0)  

#define FTN_XC_OPR_STATUS_COUNT_INC(f) \
do { \
  if ((f)->xc) \
    (f)->xc->oprcount++; \
} while (0)  
  

#define FTN_XC_OPR_STATUS_COUNT_DEC(f) \
do { \
  if (((f)->xc) && ((f)->xc->oprcount > 0)) \
    (f)->xc->oprcount--; \
} while (0)  


#define FTN_XC_ADMN_STATUS_COUNT_DEC(f) \
do { \
  if (((f)->xc) && ((f)->xc->admncount > 0)) \
    (f)->xc->admncount--; \
} while (0)  


#define MPLS_ILM_FWD_DEL(n,i) \
do { \
  if (CHECK_FLAG ((i)->flags, ILM_ENTRY_FLAG_INSTALLED)) \
    { \
      nsm_mpls_ilm_del_from_fwd (n,i); \
      UNSET_FLAG ((i)->flags, ILM_ENTRY_FLAG_INSTALLED); \
    } \
} while (0)



/* Function Prototypes */
bool_t gmpls_ftn_lookup_input (struct nsm_master *nm, struct ftn_entry *ftn,
                               u_int32_t t_id, struct fec_gen_entry *p, 
                               struct gmpls_gen_label *label, 
                               struct pal_in4_addr nhop, char *ifname);

struct ftn_entry *
nsm_gmpls_ftn_lookup (struct nsm_master *nm,
                      struct ptree_ix_table *pix_tbl,
                      struct fec_gen_entry *fec,
                      u_int32_t ftn_ix, bool_t rem_ftn);
void
gmpls_ftn_del_from_fwd (struct nsm_master *, struct ftn_entry *, 
                        struct fec_gen_entry *, u_int32_t, struct ftn_entry *);

void
gmpls_ftn_row_cleanup (struct nsm_master *nm,
                       struct ftn_entry *ftn, struct fec_gen_entry *fec,
                       u_int32_t vrf_id, bool_t del_flag);

void gmpls_ftn_xc_link (struct ftn_entry *, struct xc_entry *);

struct ptree_node * gmpls_ftn_add (struct nsm_master *, struct ptree_ix_table *,
                                   struct ftn_entry *, struct fec_gen_entry *,
                                   int *);

void gmpls_ftn_entry_deselect (struct nsm_master *, struct ftn_entry *, 
                               struct ptree_node *, bool_t);

void gmpls_ftn_entry_cleanup (struct nsm_master *, struct ftn_entry *, 
                              struct fec_gen_entry *, struct ptree_ix_table *);

void gmpls_ftn_free (struct ftn_entry *);

s_int32_t gmpls_ftn_entry_select_process (struct nsm_master *, struct ftn_entry *,
                                    struct ptree_node *);

struct xc_entry * gmpls_ftn_xc_unlink (struct ftn_entry *);

s_int32_t gmpls_ilm_create (struct nsm_master *, struct ilm_entry *);
s_int32_t gmpls_ilm_add_entry_to_fwd_tbl (struct nsm_master *, struct ilm_entry *,
                                       struct ftn_entry *);

struct xc_entry * gmpls_ilm_xc_unlink (struct ilm_entry *);

struct nhlfe_entry * gmpls_xc_nhlfe_unlink (struct xc_entry *);

void gmpls_ilm_free (struct ilm_entry *);

void gmpls_ilm_row_cleanup (struct nsm_master *, struct ilm_entry *, bool_t);

void gmpls_xc_row_cleanup (struct nsm_master *, struct xc_entry *, bool_t, 
                          bool_t);

s_int32_t gmpls_nhlfe_add (struct nsm_master *, struct nhlfe_entry *);

s_int32_t gmpls_xc_add (struct nsm_master *, struct xc_entry *);

void gmpls_nhlfe_free (struct nhlfe_entry *);

void gmpls_ilm_xc_link (struct ilm_entry *, struct xc_entry *);

void gmpls_xc_nhlfe_link (struct xc_entry *, struct nhlfe_entry *);

void gmpls_xc_free (struct xc_entry *);

s_int32_t gmpls_nhlfe_entry_remove (struct nsm_master *, struct nhlfe_entry *, 
                             bool_t);

s_int32_t nsm_mpls_ftn_add_msg_process (struct nsm_master *, struct ftn_add_data *,
                                  struct ftn_ret_data *, mpls_ftn_type_t,
                                  bool_t);

s_int32_t nsm_gmpls_ftn_add_msg_process (struct nsm_master *, 
                                   struct ftn_add_gen_data *,
                                   struct ftn_ret_data *, mpls_ftn_type_t,
                                   bool_t);

s_int32_t nsm_gmpls_ftn_fast_add_msg_process (struct nsm_master *, u_int32_t vrf_id,
                                        struct fec_gen_entry *fec, u_int32_t ftn_ix);

s_int32_t nsm_mpls_ftn_del_msg_process (struct nsm_master *, u_int32_t,
                                  struct prefix *, u_int32_t);

s_int32_t nsm_gmpls_ftn_del_msg_process (struct nsm_master *, u_int32_t,
                                   struct fec_gen_entry *, u_int32_t);


s_int32_t nsm_mpls_ftn_del_slow_msg_process (struct nsm_master *,
                                       struct ftn_add_data *);

s_int32_t nsm_gmpls_ftn_del_slow_msg_process (struct nsm_master *,
                                        struct ftn_add_gen_data *);

s_int32_t nsm_gmpls_ilm_add_msg_process (struct nsm_master *,
                                   struct ilm_add_gen_data *, bool_t,
                                   struct ilm_gen_ret_data *, bool_t);

s_int32_t nsm_gmpls_ilm_del_msg_process (struct nsm_master *, 
                                         struct ilm_add_gen_data *);
       
s_int32_t nsm_gmpls_ilm_fast_del_msg_process (struct nsm_master *, u_int32_t,
                                        struct gmpls_gen_label *lbl, u_int32_t);

#ifdef HAVE_MPLS_VC
struct vc_ftn_del_data;
s_int32_t nsm_mpls_vc_fib_check_and_add(struct nsm_master *, struct nsm_mpls_circuit *);
void nsm_mpls_vc_fib_update (struct nsm_mpls_circuit *vc,
                             struct nsm_msg_vc_fib_add *vcfa);
s_int32_t nsm_mpls_vc_fib_add_msg_process (struct nsm_master *,
                                     struct nsm_msg_vc_fib_add *);
s_int32_t nsm_mpls_vc_fib_del_msg_process (struct nsm_master *,
                                     struct nsm_msg_vc_fib_delete *);
s_int32_t nsm_mpls_vc_ilm_add_to_fwd (struct nsm_master *, struct ilm_entry *,
                                struct ilm_add_data *);
s_int32_t nsm_mpls_remote_pw_status_update (struct nsm_master *, 
                                      struct nsm_msg_pw_status *);
int nsm_mpls_pw_manual_switchover (struct nsm_master *, 
                                   struct nsm_mpls_circuit *,
                                   struct nsm_mpls_circuit *);
#endif /* HAVE_MPLS_VC */

s_int32_t gmpls_ftn_entry_remove_list_node (struct nsm_master *, 
                                            struct ftn_entry **, 
                                            struct ftn_entry *, 
                                            struct ptree_node *, int);

struct ptree_node * gmpls_ftn_node_lookup (struct ptree *, 
                                           struct fec_gen_entry *);

struct ptree_node *gmpls_ftn_node_match (struct nsm_master *,
                                         u_int32_t, struct fec_gen_entry *);

struct ftn_entry *gmpls_ftn_match_primary (struct nsm_master *, u_int32_t,
                                          struct fec_gen_entry *);
struct ptree_node *gmpls_ftn_node_match_exclude (struct nsm_master *, u_int32_t,
                                                 struct prefix *,
                                                 struct prefix *);

struct ftn_entry *nsm_gmpls_ftn_lookup (struct nsm_master *,
                                        struct ptree_ix_table *, 
                                        struct fec_gen_entry *, 
                                        u_int32_t, bool_t);

void gmpls_ftn_delete_select_update (struct nsm_master *,
                                     mpls_lsp_priority_t,
                                     struct ptree_node *);

#ifdef HAVE_GMPLS
bool_t nsm_gmpls_bidir_is_rev_dir_up (struct ftn_entry *);
bool_t nsm_gmpls_bidir_is_rev_dir_up_ilm (struct ilm_entry *);
#endif /* HAVE_GMPLS */
void mpls_ftn_delete_select_update (struct nsm_master *,
                                    mpls_lsp_priority_t,
                                    struct route_node *);
void ftn_list_delete_node (struct ftn_entry **, struct ftn_entry *);
u_int32_t gmpls_nhlfe_outgoing_if (struct nhlfe_entry *);

void gmpls_nhlfe_outgoing_label (struct nhlfe_entry *,
                                 struct gmpls_gen_label *);
s_int32_t gmpls_nhlfe_nh_addr (struct nhlfe_entry *,
                               struct addr_in *);
mpls_lsp_priority_t gmpls_owner_priority (mpls_owner_t);
struct ftn_entry *gmpls_ftn_active_head (struct ftn_entry *);
s_int32_t gmpls_ftn_get_lookup_flag (struct ftn_entry *, bool_t *);
s_int32_t nsm_mpls_map_route_del (struct nsm_master *,
                            struct ftn_entry *,
                            struct mapped_route *,
                            struct prefix *,
                            mpls_ftn_type_t,
                            u_int32_t);
s_int32_t gmpls_dep_ftn_row_update (struct ftn_entry *, struct ftn_entry *);

struct ftn_entry *nsm_gmpls_ftn_lookup_installed (struct nsm_master *,
                                                 u_int16_t, struct prefix *);
struct ftn_entry *nsm_gmpls_get_mapped_ftn (struct nsm_master *,
                                            struct fec_gen_entry *);
struct ftn_entry *nsm_mpls_get_ftn_by_proto (struct nsm_master *,
                                             struct prefix *,
                                             u_char proto);
struct ftn_entry *nsm_mpls_ftn_lookup_slow (struct ftn_add_data *, bool_t);
struct ftn_entry *nsm_gmpls_ftn_ix_lookup (struct nsm_master *,
                                          u_int32_t, struct fec_gen_entry *);
struct ftn_entry *nsm_gmpls_ftn_ix_lookup_next (struct nsm_master *,
                                           u_int32_t, struct fec_gen_entry *);

struct ftn_entry * nsm_mpls_xc_ix_ftn_lookup (struct nsm_master *, u_int32_t);

s_int32_t nsm_mpls_ftn_xc_ix_lookup_temp_list (u_int32_t);
s_int32_t nsm_gmpls_map_route_add (struct nsm_master *nm,
                                   struct ftn_entry *ftn,
                                   struct mapped_route *route,
                                   struct fec_gen_entry *fec,
                                   mpls_ftn_type_t ftn_type,
                                   u_int32_t t_id);

struct ftn_temp_ptr *nsm_gmpls_ftn_temp_lookup (u_int32_t, struct prefix *, 
                                                u_int32_t);

struct ftn_temp_ptr *nsm_gmpls_ftn_temp_lookup_next (u_int32_t);
#ifdef HAVE_PACKET
struct ilm_temp_ptr *nsm_mpls_ilm_temp_lookup (u_int32_t, u_int32_t, 
                                               u_int32_t, u_int32_t);
void nsm_gmpls_set_ilm_gen_data (struct ilm_add_gen_data *gdata,
                                 struct ilm_add_data *data);

#endif /* HAVE_PACKET */
struct ilm_temp_ptr *nsm_gmpls_ilm_temp_lookup (u_int32_t, u_int32_t,
                                                struct gmpls_gen_label *, 
                                                u_int32_t);

struct ilm_temp_ptr *nsm_gmpls_ilm_temp_lookup_next (u_int32_t);

struct ilm_entry *nsm_gmpls_ilm_ix_lookup (struct nsm_master *, u_int32_t);
struct ilm_entry *nsm_gmpls_ilm_ix_lookup_next (struct nsm_master *, u_int32_t);
struct ilm_entry *nsm_gmpls_ilm_lookup_by_owner (struct nsm_master *,
                                                 u_int32_t, 
                                                 struct gmpls_gen_label *,
                                                 mpls_owner_t);
#ifdef HAVE_PACKET
s_int32_t nsm_gmpls_ilm_add_msg_process_pkt (struct nsm_master *nm,
                                        struct ilm_add_data *data, bool_t mapped_ilm,
                                        struct ilm_ret_data *ret_data, bool_t config);

struct ilm_entry *nsm_mpls_ilm_lookup (struct nsm_master *,
                                       u_int32_t, u_int32_t, u_int32_t, 
                                       bool_t, bool_t);
#endif /* HAVE_PACKET */

struct ilm_entry * nsm_gmpls_ilm_lookup (struct nsm_master *, u_int32_t, 
                      struct gmpls_gen_label *, u_int32_t, bool_t, bool_t);

struct ilm_entry * nsm_gmpls_ilm_lookup_exact (struct nsm_master *,
                            struct ilm_add_gen_data *, bool_t, bool_t);

struct ilm_entry *gmpls_ilm_node_lookup (struct nsm_master *,
                                         u_int32_t, struct gmpls_gen_label *);
struct ilm_entry *nsm_mpls_ilm_lookup_installed (struct nsm_master *,
                                                 u_int32_t, u_int32_t);
struct ilm_entry *nsm_mpls_ilm_lookup_next (struct nsm_master *,
                                            u_int32_t, u_int32_t, u_char);

struct xc_entry *nsm_mpls_ilm_key_xc_lookup (struct nsm_master *, u_int32_t, 
                                             u_int32_t, u_int32_t);
struct xc_entry *nsm_gmpls_xc_lookup_next (struct nsm_master *,
                                           u_int32_t, u_int32_t, 
                                           struct gmpls_gen_label *, 
                                           u_int32_t, u_char);
struct xc_entry * nsm_gmpls_ilm_key_xc_lookup (struct nsm_master *, u_int32_t,
                                               struct gmpls_gen_label *, 
                                               u_int32_t);

struct xc_entry * nsm_gmpls_xc_lookup (struct nsm_master *, u_int32_t, 
                                       u_int32_t, struct gmpls_gen_label *,
                                       u_int32_t, bool_t);

struct nhlfe_temp_ptr *nsm_gmpls_nhlfe_temp_lookup (u_int32_t, u_int32_t, 
                                                    struct gmpls_gen_label *, 
                                                    struct prefix *, 
                                                    u_int32_t);

struct nhlfe_temp_ptr *nsm_gmpls_nhlfe_temp_lookup_next (u_int32_t);

struct nhlfe_entry * nsm_mpls_nhlfe_ix_lookup (struct nsm_master *, u_int32_t);
struct nhlfe_entry * nsm_mpls_nhlfe_ix_lookup_next (struct nsm_master *,
                                                    u_int32_t);

struct nhlfe_entry * nsm_gmpls_nhlfe_lookup (struct nsm_master *, 
                                             struct addr_in *, u_int32_t, 
                                             struct gmpls_gen_label *, bool_t);

s_int32_t nsm_gmpls_mapped_lsp_add (struct  nsm_master *, u_int32_t,
                          struct gmpls_gen_label *, struct gmpls_gen_label *,
                          struct fec_gen_entry *);

s_int32_t nsm_gmpls_mapped_lsp_del (struct nsm_master *, u_int32_t,
                                   struct gmpls_gen_label *);

struct mapped_lsp_entry_head * nsm_gmpls_mapped_lsp_lookup (
                                     struct  nsm_master *, u_int32_t,
                                     struct gmpls_gen_label *);

s_int32_t nsm_gmpls_mapped_ilm_add (struct nsm_master *, struct ftn_entry *,
                                    struct mapped_lsp_entry_head *, u_char);
s_int32_t nsm_gmpls_mapped_lsp_ilm_add (struct nsm_master *, struct ftn_entry *,
                                        struct ptree_node *, u_char);

s_int32_t nsm_gmpls_mapped_ilm_del (struct nsm_master *, struct ftn_entry *,
                                    struct ptree_node*);

void nsm_mpls_rib_owner_cleanup (struct nsm_master *, mpls_owner_t);
void nsm_gmpls_ftn_owner_cleanup (struct nsm_master *, u_int32_t, mpls_owner_t,
                                 int, int);
void nsm_mpls_ftn_cleanup (struct nsm_master *, u_int32_t, struct ftn_entry *);
void nsm_gmpls_ftn_cleanup_all (struct nsm_master *, u_int32_t, bool_t);
void nsm_gmpls_ilm_owner_cleanup (struct nsm_master *, mpls_owner_t, int, int,
                                 bool_t);
void nsm_gmpls_ilm_cleanup_all (struct nsm_master *, bool_t);
void nsm_gmpls_mapped_route_delete_all (struct nsm_master *);
void nsm_gmpls_mapped_lsp_delete_all (struct nsm_master *);
#ifdef HAVE_RESTART
void nsm_gmpls_ftn_owner_stale_update (struct nsm_master *, mpls_owner_t,
                                       s_int8_t , u_int32_t);
void nsm_gmpls_ilm_owner_stale_update (struct nsm_master *, mpls_owner_t ,
                                       s_int8_t, u_int32_t);
#endif /* HAVE_RESTART */
#ifdef HAVE_MPLS_FWD
void nsm_gmpls_nhlfe_stats_cleanup_all (struct nsm_master *nm);
#endif /* HAVE_MPLS_FWD */
void nsm_mpls_nhlfe_cleanup_all (struct nsm_master *, bool_t);
void nsm_mpls_xc_cleanup_all (struct nsm_master *, bool_t);

void gmpls_lsp_dep_add (struct nsm_master *, struct fec_gen_entry *,
                        u_char , void *);

void gmpls_lsp_dep_del (struct nsm_master *, struct fec_gen_entry *, u_char, 
                        void *);

#ifdef HAVE_MPLS_VC
void nsm_mpls_vc_owner_cleanup (struct nsm_master *, mpls_owner_t);
s_int32_t nsm_mpls_vc_cleanup_all (struct nsm_master *, bool_t);
void nsm_mpls_vc_fib_cleanup (struct nsm_master *, struct nsm_mpls_circuit *, int);
#endif /* HAVE_MPLS_VC */

#ifdef HAVE_VRF
void nsm_mpls_vrf_owner_cleanup (struct nsm_master *, mpls_owner_t, int, int);
void nsm_mpls_vrf_cleanup_all (struct nsm_master *, bool_t);
#ifdef HAVE_RESTART
void nsm_mpls_vrf_restart_stale_mark (struct nsm_master *, int);
#endif /* HAVE_RESTART */
#endif /* HAVE_VRF */
#ifdef HAVE_RESTART
void nsm_gmpls_ftn_owner_stale_mark (struct nsm_master *, u_int32_t,
                                    mpls_owner_t);
void nsm_gmpls_ilm_owner_stale_mark (struct nsm_master *, mpls_owner_t);
void nsm_gmpls_ilm_owner_stale_sync (struct nsm_master *, int);
s_int32_t nsm_gmpls_ilm_owner_get_stale_info (struct ilm_entry *, 
                                              struct nsm_gen_msg_stale_info *);
#endif /* HAVE_RESTART */
#ifdef HAVE_DIFFSERV
void nsm_mpls_update_diffserv_info();
#endif /* HAVE_DIFFSERV */

/* Helper functions. */
char *nsm_mpls_dump_row_status (mpls_row_status_t);
char *nsm_mpls_dump_admn_status (mpls_admn_status_t);
char *nsm_mpls_dump_opr_status (mpls_opr_status_t);

void nsm_mpls_rib_if_up_process (struct interface *);
void nsm_mpls_rib_if_down_process (struct interface *);
void nsm_mpls_rib_qos_rsrc_del_process (struct nsm_master *, u_int32_t);
void nsm_gmpls_rib_clean_client (struct nsm_master *, mpls_owner_t, int);

s_int32_t nsm_gmpls_static_ftn_tunnel_id_check (struct nsm_master *, s_int32_t, int, bool_t);
s_int32_t nsm_mpls_static_ftn_fec_check (struct nsm_master *, u_int32_t, int, bool_t,
                                   struct prefix *);
s_int32_t gmpls_ftn_entry_select (struct nsm_master *, struct ftn_entry *,
                                  struct ptree_node *);
s_int32_t gmpls_ilm_del_entry_from_fwd_tbl (struct nsm_master *, struct ilm_entry *);
struct ftn_entry * gmpls_ftn_entry_remove_list (struct nsm_master *,
                                                struct ftn_entry **,
                                                u_int32_t,
                                                struct ptree_node *,
                                                bool_t);
void ftn_list_delete_node (struct ftn_entry **, struct ftn_entry *);
void mpls_ftn_delete_select_update (struct nsm_master *,
                                    mpls_lsp_priority_t,
                                    struct route_node *);
void nsm_gmpls_ftn_add_check (struct nsm_master *nm, struct ftn_entry *ftn,
                              struct ftn_entry *ftn_head, 
                              struct ptree_node *pn);
void nsm_gmpls_restart_stale_remove (struct nsm_master *, int, bool_t);
void nsm_gmpls_stale_entries_update (struct nsm_master *, s_int32_t, s_int8_t,
                                     u_int32_t);
#ifdef HAVE_VRF
void nsm_gmpls_restart_stale_remove_vrf (struct nsm_master *, int);
#endif /* HAVE_VRF */
s_int32_t nsm_mpls_igp_shortcut_route_add (struct nsm_master *nm,
                                     struct nsm_msg_igp_shortcut_route *);
s_int32_t nsm_mpls_igp_shortcut_route_delete (struct nsm_master *nm,
                                        struct nsm_msg_igp_shortcut_route *);
s_int32_t nsm_mpls_igp_shortcut_route_update (struct nsm_master *nm,
                                        struct nsm_msg_igp_shortcut_route *);
struct ftn_entry * nsm_gmpls_get_ldp_ftn (struct nsm_master *, struct prefix *);

struct ftn_entry * nsm_gmpls_get_rsvp_ftn_pfx (struct nsm_master *,
                                          struct prefix *);
struct ftn_entry * nsm_gmpls_get_static_ftn_pfx (struct nsm_master *,
                                            struct prefix *);
struct ftn_entry * nsm_gmpls_get_rsvp_ftn (struct nsm_master *, struct fec_gen_entry *);

struct ftn_entry * nsm_gmpls_get_rsvp_ftn_by_tunnel (struct nsm_master *, 
                                                     u_char *, u_char type);
struct ftn_entry * nsm_gmpls_get_static_ftn (struct nsm_master *,  
                                             struct fec_gen_entry *);

struct ftn_entry *
nsm_mpls_get_rsvp_ftn_by_lsp (struct nsm_master *, u_int32_t *);

s_int32_t nsm_mpls_ilm_entry_add_sub_pfx (struct nsm_master *, u_int32_t, 
                                u_int32_t, char *, u_int32_t , char *,
                                struct pal_in4_addr *, u_char, struct prefix*, 
                                u_int32_t, u_int32_t, u_int32_t);

void nsm_gmpls_set_fec_from_ftn (struct ftn_entry *ftn, 
                                 struct fec_gen_entry *fec);

bool_t nsm_gmpls_is_rev_dir_up (struct ftn_entry *);
void gmpls_ftn_node_generate_key (struct fec_gen_entry *ftn,
                                  u_char *key, u_int16_t *key_len);
void nsm_gmpls_set_fec_from_mapped_route (struct ptree_node *, 
                                          struct fec_gen_entry *);

#ifdef HAVE_SNMP
void nsm_mpls_vc_up_down_notify (int, u_int32_t);
#endif /* HAVE_SNMP */

#ifdef HAVE_GMPLS
s_int32_t nsm_gmpls_bidir_ftn_get_rev_entry (struct ftn_entry *ftn,
                                             struct ilm_entry *ilm);

s_int32_t nsm_gmpls_bidir_ilm_get_rev_entry (struct ilm_entry *ilm,
                                             struct ftn_entry *ftn,
                                             struct ilm_entry *rev_ilm);

s_int32_t nsm_gmpls_ftn_del_slow_msg_process (struct nsm_master *nm,
                                              struct ftn_add_gen_data *data);

s_int32_t nsm_gmpls_data_link_down_to_rib (u_int32_t, struct datalink *);
s_int32_t nsm_gmpls_data_link_up_to_rib (u_int32_t, struct datalink *);

struct datalink *nsm_gmpls_ifindex_from_dl (struct gmpls_if*, s_int32_t);
#endif /* HAVE_GMPLS */
struct avl_tree* nsm_gmpls_get_mapped_lsp_tree (struct nsm_master *nm,
                                                struct gmpls_gen_label *lbl);

s_int32_t nsm_gmpls_get_gifindex_from_ifindex (struct nsm_master *, s_int32_t);
s_int32_t nsm_gmpls_get_ifindex_from_gifindex (struct nsm_master *, s_int32_t);
void nsm_mpls_ftn_if_down_process (struct nsm_master *, s_int32_t);
void nsm_mpls_ftn_if_up_process (struct nsm_master *, u_int32_t);
void nsm_mpls_ilm_if_down_process (struct nsm_master *, s_int32_t);
void nsm_mpls_ilm_if_down_process (struct nsm_master *, s_int32_t);

#ifdef HAVE_GMPLS 
#define NSM_MPLS_GET_INDEX_PTR(flag, ix, infp) {               \
  struct datalink *dl;                                         \
  struct gmpls_if *gmif;                                       \
    {                                                          \
      gmif = gmpls_if_get (nzg, nm->vr->id);                   \
      if (gmif == NULL)                                        \
        {                                                      \
          infp = NULL;                                         \
          ix = 0;                                              \
        }                                                      \
      else if (flag)                                           \
        {                                                      \
          dl = nsm_gmpls_ifindex_from_dl (gmif, ix);           \
          if (dl && dl->ifp != NULL)                           \
            {                                                  \
              infp = dl->ifp;                                  \
            }                                                  \
          else                                                 \
            {                                                  \
              infp = NULL;                                     \
            }                                                  \
        }                                                      \
      else                                                     \
        {                                                      \
          infp = if_lookup_by_gindex (gmif, ix);               \
        }                                                      \
    }                                                          \
}
#else /* HAVE_GMPLS */
#define NSM_MPLS_GET_INDEX_PTR(flag, ix, infp) {               \
    infp = if_lookup_by_index (&nm->vr->ifm, ix);              \
}

#endif /* HAVE_GMPLS */

#endif /* _NSM_MPLS_RIB_H */
