/* Copyright (C) 2001-2003  All Rights Reserved. */

#ifndef _NSM_MPLS_SNMP_H
#define _NSM_MPLS_SNMP_H

/* MPLS-LSR-MIB. */
#define MPLSLSRMIB 1,3,6,1,2,1,10,166,2

/* MPLS-FTN-MIB. */
#define MPLSFTNMIB 1,3,6,1,2,1,10,166,8

/* PacOS enterprise OSPF MIB.  This variable is used for register
   RSVP MIB to SNMP agent under SMUX protocol.  */
#define NSMDOID 1,3,6,1,4,1,3317,1,2,10

#define NSM_MPLS_MAX_STRING_LENGTH 255

#define NSM_MPLS_DEF_MAX_TRAP                 5

#define NSM_MPLS_SNMP_VALUE_INVALID           0

#define NSM_MPLS_SNMP_INDEX_GET_ERROR    0
#define NSM_MPLS_SNMP_INDEX_GET_SUCCESS  1
#define NSM_MPLS_SNMP_INDEX_SET_SUCCESS  1

#define NSM_MPLS_NOTIFICATIONCNTL_ENA    1
#define NSM_MPLS_NOTIFICATIONCNTL_DIS    2

#define MPLS_XC_UP                       1
#define MPLS_XC_DOWN                     2

#define NSM_MPLS_SNMP_LBSP_PLATFORM      0x01
#define NSM_MPLS_SNMP_LBSP_INTERFACE     0x02

#define NSM_MPLS_SNMP_INDEX_NONE   '\0'
#define NSM_MPLS_SNMP_INDEX_INT8    '1'    
#define NSM_MPLS_SNMP_INDEX_INT16   '2'
#define NSM_MPLS_SNMP_INDEX_INT32   '3'
#define NSM_MPLS_SNMP_INDEX_INADDR  '4'

#define NSM_MPLS_SNMP_IF_OUT                  0
#define NSM_MPLS_SNMP_IF_IN                   1
#define NSM_MPLS_LBSP_PLATFORM                0
#define NSM_MPLS_LBSP_INTERFACE               1
#define NSM_MPLS_FTN_MASK_DESTADDRESS         0x02

#define MPLS_IF_CONFIG_TABLE                  1
#define MPLS_IF_PERF_TABLE                    2
#define MPLS_IN_SEG_TABLE                     3
#define MPLS_IN_SEG_PERF_TABLE                4
#define MPLS_OUT_SEG_TABLE                    5
#define MPLS_OUT_SEG_PERF_TABLE               6
#define MPLS_XC_TABLE                         7
#define MPLS_FTN_TABLE                        8
#define MPLS_FTN_PERF_TABLE                   9
#define MPLS_IN_SEG_MAP_TABLE                 10
#define MPLS_IN_SEG_IX_NXT                    11
#define MPLS_OUT_SEG_IX_NXT                   12
#define MPLS_XC_IX_NXT                        13
#define MPLS_XC_NOTIFN_CNTL                   14
#define MPLS_FTN_IX_NXT                       15
#define MPLS_FTN_LAST_CHANGED                 16

/* MPLS Interface Configuration Table. */
#define MPLS_IF_CONF_IX                       1
#define MPLS_IF_LB_MIN_IN                     2
#define MPLS_IF_LB_MAX_IN                     3
#define MPLS_IF_LB_MIN_OUT                    4
#define MPLS_IF_LB_MAX_OUT                    5
#define MPLS_IF_TOTAL_BW                      6
#define MPLS_IF_AVA_BW                        7
#define MPLS_IF_LB_PT_TYPE                    8

/* MPLS Interface Performance Table. */
#define MPLS_IF_IN_LB_USED                    1
#define MPLS_IF_FAILED_LB_LOOKUP              2
#define MPLS_IF_OUT_LB_USED                   3
#define MPLS_IF_OUT_FRAGMENTS                 4

/* MPLS In-segment table. */

/* Modified to include all objects as per RFC 3813 */
#define MPLS_IN_SEG_IX                        1
#define MPLS_IN_SEG_IF                        2
#define MPLS_IN_SEG_LB                        3
#define MPLS_IN_SEG_LB_PTR                    4
#define MPLS_IN_SEG_NPOP                      5
#define MPLS_IN_SEG_ADDR_FAMILY               6
#define MPLS_IN_SEG_XC_IX                     7
#define MPLS_IN_SEG_OWNER                     8
#define MPLS_IN_SEG_TF_PRM                    9
#define MPLS_IN_SEG_ROW_STATUS                10
#define MPLS_IN_SEG_ST_TYPE                   11

/* MPLS Out-segment table. */
#define MPLS_OUT_SEG_IX                       1
#define MPLS_OUT_SEG_IF_IX                    2
#define MPLS_OUT_SEG_PUSH_TOP_LB              3
#define MPLS_OUT_SEG_TOP_LB                   4
#define MPLS_OUT_SEG_TOP_LB_PTR               5
#define MPLS_OUT_SEG_NXT_HOP_IPA_TYPE         6
#define MPLS_OUT_SEG_NXT_HOP_IPA              7
#define MPLS_OUT_SEG_XC_IX                    8
#define MPLS_OUT_SEG_OWNER                    9
#define MPLS_OUT_SEG_TF_PRM                   10
#define MPLS_OUT_SEG_ROW_STATUS               11
#define MPLS_OUT_SEG_ST_TYPE                  12

/* MPLS In-segment performance table. */
#define MPLS_IN_SEG_OCTS                      1
#define MPLS_IN_SEG_PKTS                      2
#define MPLS_IN_SEG_ERRORS                    3
#define MPLS_IN_SEG_DISCARDS                  4
#define MPLS_IN_SEG_HC_OCTS                   5
#define MPLS_IN_SEG_PERF_DIS_TIME             6

/* MPLS Out-segment performance table. */
#define MPLS_OUT_SEG_OCTS                     1
#define MPLS_OUT_SEG_PKTS                     2
#define MPLS_OUT_SEG_ERRORS                   3
#define MPLS_OUT_SEG_DISCARDS                 4
#define MPLS_OUT_SEG_HC_OCTS                  5
#define MPLS_OUT_SEG_DIS_TIME                 6

/* MPLS XC table. */
#define MPLS_XC_IX                            1
#define MPLS_XC_IN_SEG_IX                     2
#define MPLS_XC_OUT_SEG_IX                    3
#define MPLS_XC_LSPID                         4
#define MPLS_XC_LB_STK_IX                     5
#define MPLS_XC_OWNER                         6
#define MPLS_XC_ROW_STATUS                    7 
#define MPLS_XC_ST_TYPE                       8
#define MPLS_XC_ADM_STATUS                    9
#define MPLS_XC_OPER_STATUS                   10

/* MPLS In-segment Map table. */
#define MPLS_IN_SEG_MAP_IF                      1
#define MPLS_IN_SEG_MAP_LB                      2
#define MPLS_IN_SEG_MAP_LB_PTR                  3
#define MPLS_IN_SEG_MAP_IX                      4

/* MPLS FTN table. */
#define MPLS_FTN_IX                           1
#define MPLS_FTN_ROW_STATUS                   2
#define MPLS_FTN_DESCR                        3
#define MPLS_FTN_MASK                         4
#define MPLS_FTN_ADDR_TYPE                    5
#define MPLS_FTN_SRC_ADDR_MIN                 6
#define MPLS_FTN_SRC_ADDR_MAX                 7
#define MPLS_FTN_DST_ADDR_MIN                 8
#define MPLS_FTN_DST_ADDR_MAX                 9
#define MPLS_FTN_SRC_PORT_MIN                 10
#define MPLS_FTN_SRC_PORT_MAX                 11
#define MPLS_FTN_DST_PORT_MIN                 12
#define MPLS_FTN_DST_PORT_MAX                 13
#define MPLS_FTN_PROTOCOL                     14
#define MPLS_FTN_DSCP                         15
#define MPLS_FTN_ACT_TYPE                     16
#define MPLS_FTN_ACT_POINTER                  17
#define MPLS_FTN_ST_TYPE                      18

/* MPLS FTN Performance tabel. */
#define MPLS_FTN_PERF_IX                      1
#define MPLS_FTN_PERF_CURR_IX                 2
#define MPLS_FTN_MTCH_PKTS                    3
#define MPLS_FTN_MTCH_OCTS                    4
#define MPLS_FTN_DISC_TIME                    5

#define GAUGE       ASN_GAUGE
#define IPADDRESS   ASN_IPADDRESS
#define STRING      ASN_OCTET_STR
#define INTEGER     ASN_INTEGER
#define UNSIGNED    ASN_UNSIGNED
#define OBJID       ASN_OBJECT_ID
#define TIMETICKS   ASN_TIMETICKS
#define COUNTER64   ASN_COUNTER64
#define GAUGE32     ASN_UNSIGNED

#define NSM_MPLS_SNMP_INDEX_VARS_MAX          10

#define ACT_PTR_NULL            48
#define ACT_PTR_XC_IX_POS       30
#define ACT_PTR_ILM_IX_POS      32
#define ACT_PTR_NHLFE_IX_POS    34

#define NULL_OID_LEN            3
/* Temp Ilm List */
struct list *ilm_entry_temp_list;

/* Temp nhlfe list */
struct list *nhlfe_entry_temp_list;

/* Temp FTN list */
struct list *ftn_entry_temp_list;

/* ILM Entry Temporary Pointer */
struct ilm_temp_ptr
{
  u_int32_t ilm_ix;
  u_int32_t iif_ix;
  struct gmpls_gen_label in_label;
  u_int32_t row_status;
  mpls_owner_t owner;

#define ILM_ENTRY_FLAG_INTERFACE_SET    (1 << 0)
#define ILM_ENTRY_FLAG_LABEL_SET        (1 << 1)
  u_char flags;
  u_int8_t family;
  u_int8_t prefixlen;
  u_int8_t prfx [1];
};

/* NHLFE Entry Temporary Pointer */
struct nhlfe_temp_ptr
{
  u_int32_t nhlfe_ix;
  u_int32_t xc_ix;
  u_int32_t oif_ix;
  struct gmpls_gen_label out_label;
  struct prefix nh_addr;
  u_int32_t row_status;
  mpls_owner_t owner;
  u_char opcode;
  u_int32_t refcount;

#define NHLFE_ENTRY_FLAG_INTERFACE_SET  (1 << 0)
#define NHLFE_ENTRY_FLAG_LABEL_SET      (1 << 1)
#define NHLFE_ENTRY_FLAG_NXT_HOP_SET    (1 << 2)

  u_char flags;
};

/* FTN Entry Temporary Pointer */
struct ftn_temp_ptr
{
  u_int32_t ftn_ix;
  u_int32_t xc_ix;
  u_int32_t nhlfe_ix;

  char *sz_desc;
  struct xc_entry *xc;
  struct fec_gen_entry fec;
  mpls_owner_t owner;
  ftn_action_type_t act_type;
 
  u_int32_t row_status;

#define FTN_ENTRY_FLAG_PREFIX                 (1 << 0)
#define FTN_ENTRY_FLAG_DST_ADDR_MINMAX_SET    (1 << 1)
#define FTN_ENTRY_FLAG_ACT_POINTER_SET        (1 << 2)

  u_char flags;
};

struct nsm_mpls_snmp_index
{
  u_int32_t len;  
  u_int32_t val[NSM_MPLS_SNMP_INDEX_VARS_MAX];
};

struct mpls_interface_conf_table_index
{
  u_int32_t len;
  u_int32_t if_ix;
};
  
struct mpls_in_segment_table_index
{
  u_int32_t len;
  u_int32_t in_seg_ix;  /* Only InSegmentIndex is Index to In Segment Table */
}; 

struct mpls_out_segment_table_index
{
  u_int32_t len;
  u_int32_t out_seg_ix;
};

struct mpls_XC_table_index
{
  u_int32_t len;
  u_int32_t xc_ix;
  u_int32_t in_seg_ix;
  u_int32_t out_seg_ix;
};

struct mpls_in_seg_map_table_index
{
  u_int32_t len;
  u_int32_t in_seg_map_if_ix;
  u_int32_t in_seg_map_label;
  u_int32_t in_seg_map_lb_ptr;
}; 

struct mpls_FTN_table_index
{
  u_int32_t len;
  u_int32_t ftn_ix;
};

struct mpls_FTN_perf_table_index
{
  u_int32_t len;
  u_int32_t ftn_if_ix;
  u_int32_t ftn_ix;
};

struct nsm_mpls_mib_table_index
{
  u_int32_t len;
  u_int32_t octets;
  char vars[NSM_MPLS_SNMP_INDEX_VARS_MAX];
};
u_int32_t
nsm_gmpls_snmp_index_set (struct variable *v, oid *name, size_t *length,
                         struct nsm_mpls_snmp_index *index, u_int32_t type);

#define NSM_MPLS_SNMP_UNION(S,N) \
union { \
struct S table; \
struct nsm_mpls_snmp_index snmp;\
} N

#define NSM_MPLS_SNMP_GET(F, V) \
  ((nsm_gmpls_get_ ## F (nmpls, (V))) == NSM_API_GET_SUCCESS)

#define NSM_MPLS_SNMP_GET_NEXT(F, A, V)                                       \
  ((exact ?                                                                   \
     nsm_gmpls_get_ ## F (nm, (A), (V)) :                                      \
     nsm_gmpls_get_next_ ## F (nm, &(A), (V)))                                 \
   == NSM_API_GET_SUCCESS)

#define NSM_MPLS_SNMP_GET2NEXT(F, A, B, L, V)                                 \
  ((exact ?                                                                   \
     nsm_gmpls_get_ ## F (nm, (A), (B), (V)) :                                 \
     nsm_gmpls_get_next_ ## F (nm, &(A), &(B), (L), (V)))                      \
   == NSM_API_GET_SUCCESS)

#define NSM_MPLS_SNMP_GET4NEXT(F, A, B, C, L, V)                              \
  ((exact ?                                                                   \
     nsm_gmpls_get_ ## F (nm, (A), (B), (C), (V)) :                            \
     nsm_gmpls_get_next_ ## F (nm, &(A), &(B), &(C), (L), (V)))                \
   == NSM_API_GET_SUCCESS)

#define NSM_MPLS_SNMP_RETURN(V, T)                                            \
  do {                                                                        \
    nsm_gmpls_snmp_index_set (v, name, length,                                 \
                         (struct nsm_mpls_snmp_index *)&index, (T));          \
    *var_len = sizeof (V);                                                    \
    return (u_char *) &(V);                                                   \
  } while (0)

#define NSM_MPLS_SNMP_RETURN_CHAR(V, T)                                       \
  do {                                                                        \
    nsm_gmpls_snmp_index_set (v, name, length,                                 \
                         (struct nsm_mpls_snmp_index *)&index, (T));          \
    *var_len = pal_strlen ((u_char *)V);                                      \
    return (u_char *) (V);                                                    \
  } while (0)

/* Prototypes. */
int nsm_mpls_set_notify (struct nsm_mpls *, int);
int nsm_mpls_get_notify (struct nsm_mpls *, int *);

u_int32_t nsm_mpls_set_inseg_if (struct nsm_master *, u_int32_t, u_int32_t);
u_int32_t nsm_mpls_set_inseg_lb (struct nsm_master *, u_int32_t, u_int32_t);

u_int32_t nsm_mpls_set_inseg_lb_ptr (struct nsm_master *, u_int32_t);

u_int32_t nsm_mpls_set_inseg_npop (struct nsm_master *, u_int32_t, u_int32_t);

u_int32_t nsm_mpls_set_inseg_addr_family (struct nsm_master *, u_int32_t, 
                                          u_int32_t);

u_int32_t nsm_mpls_set_inseg_tf_prm (struct nsm_master *, u_int32_t);

u_int32_t nsm_mpls_set_inseg_row_status (struct nsm_master *, u_int32_t, 
                                         u_int32_t);

u_int32_t nsm_mpls_set_inseg_st_type (struct nsm_master *, u_int32_t, 
                                      u_int32_t);

u_int32_t nsm_mpls_set_outseg_if_ix (struct nsm_master *, u_int32_t, u_int32_t);

u_int32_t nsm_mpls_set_outseg_push_top_lb (struct nsm_master *, 
                                           u_int32_t, u_int32_t);

u_int32_t nsm_mpls_set_outseg_top_lb (struct nsm_master *, 
                                      u_int32_t, u_int32_t);

u_int32_t nsm_mpls_set_outseg_top_lb_ptr (struct nsm_master *, u_int32_t);

u_int32_t nsm_mpls_set_outseg_nxt_hop_ipa_type (struct nsm_master *,
                                                u_int32_t, u_int32_t);

u_int32_t nsm_mpls_set_outseg_nxt_hop_ipa (struct nsm_master *, u_int32_t, 
                                           u_int32_t, struct pal_in4_addr);

u_int32_t nsm_mpls_set_outseg_tf_prm (struct nsm_master *, u_int32_t);

u_int32_t nsm_mpls_set_outseg_row_status (struct nsm_master *, u_int32_t, 
                                          u_int32_t);

u_int32_t nsm_mpls_set_outseg_st_type (struct nsm_master *, u_int32_t, 
                                       u_int32_t);

u_int32_t nsm_mpls_set_xc_lspid (struct nsm_master *, u_int32_t, u_int32_t,
                                 u_int32_t);

u_int32_t nsm_mpls_set_xc_lb_stk_ix (struct nsm_master *, u_int32_t, u_int32_t,
                                     u_int32_t, u_int32_t);

u_int32_t nsm_mpls_set_xc_row_status (struct nsm_master *, u_int32_t, u_int32_t,
                                      u_int32_t, u_int32_t);

u_int32_t nsm_mpls_set_xc_st_type (struct nsm_master *, u_int32_t, u_int32_t,
                                   u_int32_t, u_int32_t);

u_int32_t nsm_mpls_set_xc_adm_status (struct nsm_master *, u_int32_t, u_int32_t,
                                      u_int32_t, u_int32_t);

void nsm_mpls_snmp_init (void);

u_int32_t nsm_cnt_in_lb_used (struct interface *);
u_int32_t nsm_cnt_out_lb_used (struct interface *);
u_int32_t nsm_cnt_failed_lb_lookup (struct interface *);
u_int32_t nsm_cnt_out_fragments (struct interface *);

u_int32_t nsm_cnt_in_seg_octs (struct ilm_entry *);
u_int32_t nsm_cnt_in_seg_pkts (struct ilm_entry *);
u_int32_t nsm_cnt_in_seg_errors (struct ilm_entry *);
u_int32_t nsm_cnt_in_seg_discards (struct ilm_entry *);
pal_time_t nsm_cnt_in_seg_perf_dis_time (struct ilm_entry *);
ut_int64_t nsm_cnt_in_seg_hc_octs (struct ilm_entry *);

u_int32_t nsm_cnt_out_seg_octs (struct nhlfe_entry *);
u_int32_t nsm_cnt_out_seg_pkts (struct nhlfe_entry *);
u_int32_t nsm_cnt_out_seg_errors (struct nhlfe_entry *);
u_int32_t nsm_cnt_out_seg_discards (struct nhlfe_entry *);
pal_time_t nsm_cnt_out_seg_perf_dis_time (struct nhlfe_entry *);
ut_int64_t nsm_cnt_out_seg_hc_octs (struct nhlfe_entry *);

ut_int64_t nsm_cnt_ftn_mtch_pkts (struct ftn_entry *);
ut_int64_t nsm_cnt_ftn_mtch_octs (struct ftn_entry *);
u_int32_t nsm_cnt_ftn_disc_time (struct ftn_entry *);

void nsm_mpls_opr_sts_trap (struct nsm_mpls *, int, int);
#endif /* _NSM_MPLS_SNMP_H */
