/* Copyright (C) 2001-2003  All Rights Reserved. */

#ifndef _NSM_MPLS_VC_SNMP_H
#define _NSM_MPLS_VC_SNMP_H

#ifdef HAVE_MPLS_VC
#ifdef HAVE_SNMP
/* PW-STD-MIB */
#define PWSTDMIB 1,3,6,1,2,1,10,246
#define PWMPLSSTDMIB 1,3,6,1,2,1,181

/* PW-ENET-MIB*/
#define PWENETMIB 1,3,6,1,2,1,180

/* PacOS enterprise OSPF MIB.  This variable is used for register
   RSVP MIB to SNMP agent under SMUX protocol.  */
#define NSMDOID 1,3,6,1,4,1,3317,1,2,10

#define NSM_PW_MAX_STRING_LENGTH 255
#define NSM_PW_DEFAULT_VLAN              4095

#define NSM_PW_ENET_PW_INSTANCE             1

#define NSM_PW_SNMP_VALUE_INVALID           0
#define PW_PSN_TYPE_MPLS                    1
#define PWID_FEC                            2
#define GEN_PWID_FEC                        3

#define NSM_PW_SNMP_INDEX_GET_ERROR    0
#define NSM_PW_SNMP_INDEX_GET_SUCCESS  1
#define NSM_PW_SNMP_INDEX_SET_SUCCESS  1

#define NSM_MPLS_NOTIFICATIONCNTL_ENA    1
#define NSM_MPLS_NOTIFICATIONCNTL_DIS    2

#define MPLS_XC_UP                       1
#define MPLS_XC_DOWN                     2

#define NSM_MPLS_SNMP_LBSP_PLATFORM      0x01
#define NSM_MPLS_SNMP_LBSP_INTERFACE     0x02

#define NSM_PW_SNMP_INDEX_NONE   '\0'
#define NSM_PW_SNMP_INDEX_INT8    '1'    
#define NSM_PW_SNMP_INDEX_INT16   '2'
#define NSM_PW_SNMP_INDEX_INT32   '3'
#define NSM_PW_SNMP_INDEX_INADDR  '4'

#define NSM_MPLS_SNMP_IF_OUT                  0
#define NSM_MPLS_SNMP_IF_IN                   1
#define NSM_MPLS_LBSP_PLATFORM                0
#define NSM_MPLS_LBSP_INTERFACE               1
#define NSM_MPLS_FTN_MASK_DESTADDRESS         0x02

enum pw_snmp_table
{
  PW_VC_TABLE=1,
  PW_PERF_CRNT_TABLE,
  PW_PERF_INTRVL_TABLE,
  PW_PERF1DY_INTRVL_TABLE,
  PW_IX_MAP_TABLE,
  PW_PEER_MAP_TABLE,
#ifdef HAVE_FEC129
  PW_GEN_FEC_MAP_TABLE,
#endif /*HAVE_FEC129*/
  PW_ENET_TABLE,
  PW_ENET_STATISTICS_TABLE,
  PW_MPLS_TABLE,
  PW_MPLS_OUTBD_TNL_TABLE,
  PW_MPLS_INBD_TABLE,
  PW_NON_TE_MAP_TABLE,
  PW_TE_MPLS_TNL_MAP_TABLE,
};

/* PW Virtual Connection Table. */
#define PW_INDEX                              1
#define PW_TYPE                               2
#define PW_OWNER                              3
#define PW_PSN_TYPE                           4
#define PW_SETUP_PRTY                         5
#define PW_HOLD_PRTY                          6
#define PW_PEER_ADDR_TYPE                     7
#define PW_PEER_ADDR                          8
#define PW_ATTCHD_PW_IX                       9
#define PW_IF_IX                              10
#define PW_ID                                 11
#define PW_LOCAL_GRP_ID                       12
#define PW_GRP_ATTCHMT_ID                     13
#define PW_LOCAL_ATTCHMT_ID                   14
#define PW_PEER_ATTCHMT_ID                    15
#define PW_CW_PRFRNCE                         16
#define PW_LOCAL_IF_MTU                       17
#define PW_LOCAL_IF_STRING                    18
#define PW_LOCAL_CAPAB_ADVRT                  19
#define PW_REMOTE_GRP_ID                      20
#define PW_CW_STATUS                          21
#define PW_REMOTE_IF_MTU                      22
#define PW_REMOTE_IF_STRING                   23
#define PW_REMOTE_CAPAB                       24
#define PW_FRGMT_CFG_SIZE                     25
#define PW_RMT_FRGMT_CAPAB                    26
#define PW_FCS_RETENTN_CFG                    27
#define PW_FCS_RETENTN_STATUS                 28
#define PW_OUTBD_LABEL                        29
#define PW_INBD_LABEL                         30
#define PW_NAME                               31
#define PW_DESCR                              32
#define PW_CREATE_TIME                        33
#define PW_UP_TIME                            34
#define PW_LAST_CHANGE                        35
#define PW_ADMIN_STATUS                       36
#define PW_OPER_STATUS                        37
#define PW_LOCAL_STATUS                       38
#define PW_RMT_STATUS_CAPAB                   39
#define PW_RMT_STATUS                         40
#define PW_TIME_ELAPSED                       41
#define PW_VALID_INTRVL                       42
#define PW_ROW_STATUS                         43
#define PW_ST_TYPE                            44
#define PW_OAM_EN                             45 
#define PW_GEN_AGI_TYPE                       46
#define PW_GEN_LOC_AGII_TYPE                  47
#define PW_GEN_REM_AGII_TYPE                  48
#define PW_IX_NXT                             49
#define PW_PERF_TOT_ERR_PCKTS                 50
#define PW_UP_DWN_NOTIF_EN                    51
#define PW_DEL_NOTIF_EN                       52
#define PW_NOTIF_RATE                         53


/* PW Performance Current Table. */
#define PW_PERF_CRNT_IN_HC_PCKTS              1
#define PW_PERF_CRNT_IN_HC_BYTES              2
#define PW_PERF_CRNT_OUT_HC_PCKTS             3
#define PW_PERF_CRNT_OUT_HC_BYTES             4
#define PW_PERF_CRNT_IN_PCKTS                 5
#define PW_PERF_CRNT_IN_BYTES                 6
#define PW_PERF_CRNT_OUT_PCKTS                7
#define PW_PERF_CRNT_OUT_BYTES                8

/* PW Performance Interval Table. */
#define PW_PERF_INTRVL_NO                     1
#define PW_PERF_INTRVL_VALID_DATA             2
#define PW_PERF_INTRVL_TIME_ELPSD             3
#define PW_PERF_INTRVL_IN_HC_PCKTS            4
#define PW_PERF_INTRVL_IN_HC_BYTES            5
#define PW_PERF_INTRVL_OUT_HC_PCKTS           6
#define PW_PERF_INTRVL_OUT_HC_BYTES           7
#define PW_PERF_INTRVL_IN_PCKTS               8
#define PW_PERF_INTRVL_IN_BYTES               9
#define PW_PERF_INTRVL_OUT_PCKTS              10
#define PW_PERF_INTRVL_OUT_BYTES              11

/* PW Performance 1 Day Interval Table. */
#define PW_PERF_1DY_INTRVL_NO                 1
#define PW_PERF_1DY_INTRVL_VALID_DT           2
#define PW_PERF_1DY_INTRVL_MONI_SECS          3
#define PW_PERF_1DY_INTRVL_IN_HC_PCKTS        4
#define PW_PERF_1DY_INTRVL_IN_HC_BYTES        5
#define PW_PERF_1DY_INTRVL_OUT_HC_PCKTS       6
#define PW_PERF_1DY_INTRVL_OUT_HC_BYTES       7

/* The PW ID mapping table */
#define PW_ID_MAP_PW_TYPE                     1
#define PW_ID_MAP_PW_ID                       2
#define PW_ID_MAP_PEER_ADDR_TYPE              3
#define PW_ID_MAP_PEER_ADDR                   4
#define PW_ID_MAP_PW_IX                       5

/* Generalized FEC table */
#define PW_GEN_FEC_MAP_AGI_TYPE               1 
#define PW_GEN_FEC_MAP_AGI                    2
#define PW_GEN_FEC_LOC_AII_TYPE               3
#define PW_GEN_FEC_LOC_AII                    4
#define PW_GEN_FEC_REM_AII_TYPE               5
#define PW_GEN_FEC_REM_AII                    6
#define PW_GEN_FEC_PW_ID                      7

/* The peer mapping table */
#define PW_PEER_MAP_PEER_ADDR_TYPE            1
#define PW_PEER_MAP_PEER_ADDR                 2
#define PW_PEER_MAP_PW_TYPE                   3
#define PW_PEER_MAP_PW_ID                     4
#define PW_PEER_MAP_PW_IX                     5

/* The PW_ENET Table */
#define PW_ENET_PW_INSTANCE                   1
#define PW_ENET_PW_VLAN                       2  
#define PW_ENET_VLAN_MODE                     3
#define PW_ENET_PORT_VLAN                     4
#define PW_ENET_PORT_IF_INDEX                 5 
#define PW_ENET_PW_IF_INDEX                   6
#define PW_ENET_ROW_STATUS                    7
#define PW_ENET_STORAGE_TYPE                  8

/* The PW_ENET Statistics Table */
#define PW_ENET_STATS_ILLEGAL_VLAN            1
#define PW_ENET_STATS_ILLEGAL_LENGTH          2

/* PW MPLS Table. */
#define PW_MPLS_MPLS_TYPE                     1
#define PW_MPLS_EXP_BITS_MODE                 2
#define PW_MPLS_EXP_BITS                      3
#define PW_MPLS_TTL                           4
#define PW_MPLS_LCL_LDP_ID                    5
#define PW_MPLS_LCL_LDP_ENTTY_IX              6
#define PW_MPLS_PEER_LDP_ID                   7
#define PW_MPLS_ST_TYPE                       8

/* PW MPLS Outbound Tunnel Table */
#define PW_MPLS_OUTBD_LSR_XC_IX               1
#define PW_MPLS_OUTBD_TNL_IX                  2
#define PW_MPLS_OUTBD_TNL_INSTNCE             3
#define PW_MPLS_OUTBD_TNL_LCL_LSR             4
#define PW_MPLS_OUTBD_TNL_PEER_LSR            5
#define PW_MPLS_OUTBD_IF_IX                   6
#define PW_MPLS_OUTBD_TNL_TYP_INUSE           7

/* PW MPLS inbound table */
#define PW_MPLS_INBD_XC_IX                    1

/* PW to Non-TE mapping Table */
#define PW_MPLS_NON_TE_MAP_DN                 1
#define PW_MPLS_NON_TE_MAP_XC_IX              2
#define PW_MPLS_NON_TE_MAP_IF_IX              3
#define PW_MPLS_NON_TE_MAP_PW_IX              4

/* PW to TE MPLS tunnels mapping Table */
#define PW_MPLS_TE_MAP_TNL_IX                 1
#define PW_MPLS_TE_MAP_TNL_INSTNCE            2
#define PW_MPLS_TE_MAP_TNL_PEER_LSR_ID        3
#define PW_MPLS_TE_MAP_TNL_LCL_LSR_ID         4
#define PW_MPLS_TE_MAP_PW_IX                  5

/* PW Notifications */
#define PW_UP_NOTIFICATION                    1
#define PW_DOWN_NOTIFICATION                  2
#define PW_DELETE_NOTIFICATION                3

#define GAUGE       ASN_GAUGE
#define IPADDRESS   ASN_IPADDRESS
#define STRING      ASN_OCTET_STR
#define INTEGER     ASN_INTEGER
#define OBJID       ASN_OBJECT_ID
#define TIMETICKS   ASN_TIMETICKS
#define COUNTER64   ASN_COUNTER64
#define COUNTER32   ASN_COUNTER
#define TRUTHVALUE  ASN_INTEGER
#define PW_BITS     ASN_OCTET_STR
#define PW_OCTET_STR ASN_OCTET_STR

#define NSM_PW_SNMP_INDEX_VARS_MAX            10

#define ACT_PTR_NULL            48
#define ACT_PTR_XC_IX_POS       30
#define ACT_PTR_ILM_IX_POS      32
#define ACT_PTR_NHLFE_IX_POS    34

/* PW_PEER_ADDR_TYPE VALUE rfc4001 */
#define PW_PEER_ADDR_TYPE_UNKNOWN    0
#define PW_PEER_ADDR_TYPE_IPV4       1
#define PW_PEER_ADDR_TYPE_IPV6       2
#define PW_PEER_ADDR_TYPE_IPV4Z      3
#define PW_PEER_ADDR_TYPE_IPV6Z      4
#define PW_PEER_ADDR_TYPE_DNS        6

#define NULL_OID_LEN            3

/* Ethernet PW Vlan modes */
#define PW_ENET_MODE_OTHER                     0
#define PW_ENET_MODE_PORT_BASED                1
#define PW_ENET_MODE_NO_CHANGE                 2
#define PW_ENET_MODE_CHANGE_VLAN               3
#define PW_ENET_MODE_ADD_VLAN                  4
#define PW_ENET_MODE_REMOVE_VLAN               5

/* PW_PEER_ADDR_TYPE VALUE rfc4001 */
#define PW_PEER_ADDR_TYPE_UNKNOWN    0
#define PW_PEER_ADDR_TYPE_IPV4       1
#define PW_PEER_ADDR_TYPE_IPV6       2
#define PW_PEER_ADDR_TYPE_IPV4Z      3
#define PW_PEER_ADDR_TYPE_IPV6Z      4 
#define PW_PEER_ADDR_TYPE_DNS        6 

#define VC_SNMP_TIMETICKS               100

enum pw_admin_status
{
    PW_SNMP_ADMIN_UP = 1,
    PW_SNMP_ADMIN_DOWN,
    PW_SNMP_ADMIN_TESTING
};

enum pw_snmp_remote_status_capability
{
    PW_SNMP_REMOTE_STATUS_NOT_APPL = 1,
    PW_SNMP_REMOTE_STATUS_NOT_YET_KNOWN,
    PW_SNMP_REMOTE_STATUS_CAPABLE,
    PW_SNMP_REMOTE_STATUS_NOT_CAPABLE
};
/* Temp VC List */
struct list *vc_entry_temp_list;

/* VC Entry Temporary Pointer */
struct nsm_mpls_circuit_temp
{
  u_int32_t vc_id;
  char *vc_name;
  char *dscr;
  u_int8_t peer_addr_type;
  struct pal_in4_addr peer_addr;
  u_int16_t vc_type;
  u_int8_t setup_prty;
  u_int8_t hold_prty;
  u_int8_t pw_attchd_pw_ix;
  u_int32_t local_grp_id;
  u_int32_t remote_grp_id; 
  char *grp_name;
  u_int32_t grp_attchmt_id;
  u_int32_t local_attchmt_id;
  u_int32_t peer_attchmt_id;
  u_char cw;
  s_int32_t mtu;
  u_char oper_status;
  u_char admin_status;
  u_int32_t row_status;
  u_int32_t pw_owner;
  u_char cw_status;

#define VC_ENTRY_FLAG_ID_SET              (1 << 0)
#define VC_ENTRY_FLAG_NAME_SET            (1 << 1)
#define VC_ENTRY_FLAG_PEER_ADDR_SET       (1 << 2)
#define VC_ENTRY_FLAG_PEER_ADDR_TYPE_SET  (1 << 3)
  u_char flags;

  struct nsm_msg_vc_fib_add *vc_fib_temp;

  u_int32_t vc_snmp_index;
};

/* Temporary structue for storing the VC Info used for
 * VC Info creation/updation using SNMP */

struct nsm_mpls_vc_info_temp
{
  /* PW SNMP Index
   VC has pointer to vc_info_temp. this variable not required 
  u_int32_t vc_snmp_idx;
  */

 /* Currently PW can have only single binding.
  * When created using CLI we'll use 1 as the value.
  * SNMP user can specifiy any value during entry creation.
  */
  u_int32_t pw_enet_pw_instance;

  /* Value 4095 when pwEnetVlanMode is portBased
   * Value of vlan id when pwEnetVlanMode noChange */
  u_int16_t pw_enet_pw_vlan;

  /* We support only two modes:-
   * noChange when the binding is Enet Vlan.
   * portBased when the binding is Enet port based.
  int pw_enet_vlan_mode;
  */

  /* value will be 4095 if pwEnetVlanMode is portBased
   * value of pw_enet_pw_vlan when pwEnetVlanMode is noChange 
  u_int16_t pw_enet_port_vlan;
  */

  /* Interface on which PW is binded */
  u_int32_t pw_enet_port_ifindex;

  /* In our current implementation PW is not modelled as
   * interface in ifTable, so always 0.
  u_int32_t pw_enet_pw_ifindex;
  */

  /* Storage Type is always Non-Volatile currently
  u_int8_t storage_type;
   */

  /* Temporary storage of vc_mode is this structure is created as part 
   * making an entry INACTIVE using SNMP. Default it will be 0 */
  u_char vc_mode;
};

struct nsm_pw_snmp_index
{
  u_int32_t len;  
  u_int32_t val[NSM_PW_SNMP_INDEX_VARS_MAX];
};

struct pw_vc_table_index
{
  u_int32_t len;
  u_int32_t pw_ix;
};

struct pw_enet_table_index
{
  u_int32_t len;
  u_int32_t pw_ix;
  u_int32_t pw_instance;
};

struct pw_enet_stats_table_index
{
  u_int32_t len;
  u_int32_t pw_ix;
};

struct pw_perf_crnt_table_index
{
  u_int32_t len;
  u_int32_t pw_ix;
};
  
struct pw_perf_intrvl_table_index
{
  u_int32_t len;
  u_int32_t pw_ix;
  u_int32_t pw_perf_intrvl_no_ix;
}; 

struct pw_perf1dy_intrvl_table_index
{
  u_int32_t len;
  u_int32_t pw_ix;
  u_int32_t pw_perf1dy_intrvl_no_ix;
};

struct pw_id_map_table_index
{
  u_int32_t len;
  u_int32_t pw_id_map_pw_type;
  u_int32_t pw_id_map_pw_id;
  u_int32_t pw_id_map_peer_addr_type;
  struct pal_in4_addr pw_id_map_peer_addr;
};

struct pw_peer_map_table_index
{
  u_int32_t len;
  u_int32_t pw_peer_map_peer_addr_type;
  struct pal_in4_addr pw_peer_map_peer_addr;
  u_int32_t pw_peer_map_pw_type;
  u_int32_t pw_peer_map_pw_id;
}; 

#ifdef HAVE_FEC129
struct pw_gen_fec_map_table_index
{
  u_int32_t len;
  u_int32_t pw_gen_fec_agi_type;
  u_int32_t pw_gen_fec_agi;
  u_int32_t pw_gen_fec_loc_aii_type;
  u_int32_t pw_gen_fec_loc_aii;
  u_int32_t pw_gen_fec_rmte_aii_type;
  u_int32_t pw_gen_fec_rmte_aii;
};
#endif /* HAVE_FEC129 */

struct pw_mpls_table_index
{
  u_int32_t len;
  u_int32_t pw_ix;
};

struct pw_mpls_outbd_tnl_table_index
{
  u_int32_t len;
  u_int32_t pw_ix;
};

struct pw_mpls_inbd_table_index
{
  u_int32_t len;
  u_int32_t pw_ix;
};

struct pw_non_te_map_table_index
{
  u_int32_t len;
  u_int32_t pw_mpls_non_te_map_dn;
  u_int32_t pw_mpls_non_te_map_xc_ix;
  u_int32_t pw_mpls_non_te_map_if_ix;
  u_int32_t pw_mpls_non_te_map_pw_ix;
};

struct pw_te_mpls_tnl_map_table_index
{
  u_int32_t len;
  u_int32_t pw_mpls_te_map_tnl_ix;
  u_int32_t pw_mpls_te_map_tnl_instnce;
  struct pal_in4_addr pw_mpls_te_map_tnl_peer_lsr_id;
  struct pal_in4_addr pw_mpls_te_map_tnl_lcl_lsr_id;
  u_int32_t pw_mpls_te_map_pw_ix;
};


struct nsm_pw_mib_table_index
{
  u_int32_t len;
  u_int32_t octets;
  char vars[NSM_PW_SNMP_INDEX_VARS_MAX];
};

struct ldp_id
{
  u_int32_t lsr_id;
  u_int16_t label_space;
};
#define NSM_PW_SNMP_UNION(S,N) \
union { \
struct S table; \
struct nsm_pw_snmp_index snmp;\
} N

#define NSM_PW_SNMP_GET(F, V) \
  ((nsm_mpls_get_ ## F (nm, (V))) == NSM_API_GET_SUCCESS)

#define NSM_PW_SNMP_GET_NEXT(F, A, V)                                         \
  ((exact ?                                                                   \
     nsm_mpls_get_ ## F (nm, (A), (V)) :                                      \
     nsm_mpls_get_next_ ## F (nm, &(A), (V)))                                 \
   == NSM_API_GET_SUCCESS)

#define NSM_PW_SNMP_GET2NEXT(F, A, B, V)                                      \
  ((exact ?                                                                   \
     nsm_mpls_get_ ## F (nm, (A), (B), (V)) :                                 \
     nsm_mpls_get_next_ ## F (nm, &(A), &(B), (V)))                           \
   == NSM_API_GET_SUCCESS)

#define NSM_PW_SNMP_GET4NEXT(F, A, B, C, V)                                  \
  ((exact ?                                                                  \
     nsm_mpls_get_ ## F (nm, (A), (B), (C), (V)) :                           \
     nsm_mpls_get_next_ ## F (nm, &(A), &(B), &(C), (V)))                    \
   == NSM_API_GET_SUCCESS)

#define NSM_PW_SNMP_GET8NEXT(F, A, B, C, D, V)                               \
  ((exact ?                                                                  \
     nsm_mpls_get_ ## F (nm, (A), (B), (C), (D), (V)) :                      \
     nsm_mpls_get_next_ ## F (nm, &(A), &(B), &(C), &(D), (V)))              \
   == NSM_API_GET_SUCCESS)

#define NSM_PW_SNMP_GET16NEXT(F, A, B, C, D, E, V)                           \
  ((exact ?                                                                  \
     nsm_mpls_get_ ## F (nm, (A), (B), (C), (D), (E), (V)) :                 \
     nsm_mpls_get_next_ ## F (nm, &(A), &(B), &(C), &(D), &(E), (V)))        \
   == NSM_API_GET_SUCCESS)

#define NSM_PW_SNMP_GET32NEXT(F, A, B, C, D, E, G, V)                        \
  ((exact ?                                                                  \
     nsm_mpls_get_ ## F (nm, (A), (B), (C), (D), (E), (G), (V)) :            \
     nsm_mpls_get_next_ ## F (nm, &(A), &(B), &(C), &(D), &(E), &(G), (V)))  \
   == NSM_API_GET_SUCCESS)                                                   \
     
#define NSM_PW_SNMP_RETURN(V, T)                                             \
  do {                                                                       \
      nsm_pw_snmp_index_set(v, name, length,                                 \
                            (struct nsm_pw_snmp_index *)&index, (T));        \
      *var_len = sizeof (V);                                                 \
      return (u_char *) &(V);                                                \
     } while (0)

#define NSM_PW_SNMP_RETURN_CHAR(V, T)                                        \
  do {                                                                       \
       nsm_pw_snmp_index_set (v, name, length,                               \
                              (struct nsm_pw_snmp_index *)&index, (T));      \
       *var_len = pal_strlen ((u_char *)V);                                  \
       return (u_char *) (V);                                                \
     } while (0)
#define NSM_PW_SNMP_RETURN_IPADDRESS(V, T)                                   \
  do {                                                                       \
       nsm_pw_snmp_index_set (v, name, length,                               \
                              (struct nsm_pw_snmp_index *)&index, (T));      \
       *var_len = sizeof (struct pal_in4_addr);                              \
       return (u_char *) &(V);                                                \
     } while (0)

#define NSM_PW_SNMP_RETURN_INTEGER64(V, T) \
  do { \
       nsm_pw_snmp_index_set (v, name, length,                               \
                              (struct nsm_pw_snmp_index *)&index, (T));      \
       *var_len = sizeof (V); \
       return (u_char *) &(V); \
     } while (0)

#define NSM_PW_SNMP_RETURN_BITS(V, T)                                        \
  do {                                                                       \
       nsm_pw_snmp_index_set (v, name, length,                               \
                              (struct nsm_pw_snmp_index *)&index, (T));      \
       *var_len = pal_strlen ((u_char *)V);                                  \
       return (u_char *) (V);                                                \
     } while (0)

/* Prototypes. */
u_int32_t nsm_mpls_set_pw_type (struct nsm_master *,
                                u_int32_t, u_int32_t);
u_int32_t
nsm_mpls_set_pw_owner (struct nsm_master *,
                       u_int32_t, u_int32_t);
u_int32_t
nsm_mpls_set_pw_psn_type (struct nsm_master *,
                          u_int32_t, u_int32_t);
u_int32_t
nsm_mpls_set_pw_setup_prty (struct nsm_master *,
                            u_int32_t, u_int32_t);
u_int32_t
nsm_mpls_set_pw_hold_prty (struct nsm_master *,
                           u_int32_t, u_int32_t);
u_int32_t
nsm_mpls_set_pw_peer_addr_type (struct nsm_master *,
                                u_int32_t, u_int32_t);
u_int32_t
nsm_mpls_set_pw_peer_addr (struct nsm_master *, u_int32_t,
                           struct pal_in4_addr *);
u_int32_t
nsm_mpls_set_pw_attchd_pw_ix (struct nsm_master *,
                              u_int32_t, u_int32_t);
u_int32_t
nsm_mpls_set_pw_if_ix (struct nsm_master *,
                       u_int32_t, u_int32_t);
u_int32_t
nsm_mpls_set_pw_local_grp_id (struct nsm_master *,
                              u_int32_t, u_int32_t);
u_int32_t
nsm_mpls_set_pw_grp_attchmt_id (struct nsm_master *,
                                u_int32_t, u_int32_t);
u_int32_t
nsm_mpls_set_pw_local_attchmt_id (struct nsm_master *,
                                  u_int32_t, u_int32_t);
u_int32_t
nsm_mpls_set_pw_peer_attchmt_id (struct nsm_master *,
                                 u_int32_t, u_int32_t);
u_int32_t
nsm_mpls_set_pw_cw_prfrnce (struct nsm_master *,
                            u_int32_t, u_int32_t);
u_int32_t
nsm_mpls_set_pw_local_if_mtu (struct nsm_master *,
                              u_int32_t, u_int32_t);
u_int32_t
nsm_mpls_set_pw_local_if_string (struct nsm_master *,
                                 u_int32_t, u_int32_t);
u_int32_t
nsm_mpls_set_pw_local_capab_advrt (struct nsm_master *,
                                   u_int32_t, char *);
u_int32_t
nsm_mpls_set_pw_frgmt_cfg_size (struct nsm_master *,
                                u_int32_t, u_int32_t);
u_int32_t
nsm_mpls_set_pw_fcs_retentn_cfg (struct nsm_master *,
                                 u_int32_t, u_int32_t);
u_int32_t
nsm_mpls_set_pw_outbd_label (struct nsm_master *,
                             u_int32_t, u_int32_t);
u_int32_t
nsm_mpls_set_pw_inbd_label (struct nsm_master *,
                            u_int32_t, u_int32_t);
u_int32_t
nsm_mpls_set_pw_name (struct nsm_master *,
                      u_int32_t, char *);
u_int32_t
nsm_mpls_set_pw_descr (struct nsm_master *,
                       u_int32_t, char *);
u_int32_t
nsm_mpls_set_pw_admin_status (struct nsm_master *,
                              u_int32_t, u_int32_t);
u_int32_t
nsm_mpls_set_pw_row_status (struct nsm_master *,
                            u_int32_t, u_int32_t);
u_int32_t
nsm_mpls_set_pw_st_type (struct nsm_master *,
                         u_int32_t, u_int32_t );
u_int32_t
nsm_mpls_set_pw_id (struct nsm_master *nm, u_int32_t pw_ix, u_int32_t pw_id);

u_int32_t
nsm_mpls_set_pw_enet_pw_instance (struct nsm_master *,
                         u_int32_t, u_int32_t, u_int32_t );
u_int32_t
nsm_mpls_set_pw_enet_pw_vlan (struct nsm_master *,
                         u_int32_t, u_int32_t, u_int32_t );
u_int32_t
nsm_mpls_set_pw_enet_vlan_mode (struct nsm_master *,
                         u_int32_t, u_int32_t, u_int32_t );
u_int32_t
nsm_mpls_set_pw_enet_port_vlan (struct nsm_master *,
                         u_int32_t, u_int32_t, u_int32_t );
u_int32_t
nsm_mpls_set_pw_enet_port_if_index (struct nsm_master *,
                         u_int32_t, u_int32_t, u_int32_t );
u_int32_t
nsm_mpls_set_pw_enet_pw_if_index (struct nsm_master *,
                         u_int32_t, u_int32_t, u_int32_t );
u_int32_t
nsm_mpls_set_pw_enet_row_status (struct nsm_master *,
                         u_int32_t, u_int32_t, u_int32_t);
u_int32_t
nsm_mpls_set_pw_enet_storage_type (struct nsm_master *,
                         u_int32_t, u_int32_t, u_int32_t );


void
nsm_pw_snmp_init (void);

int check_if_notify_send(struct nsm_mpls *nmpls);
int
nsm_mpls_pw_snmp_trap_limiter(struct nsm_mpls *nmpls);
  
void
nsm_mpls_vc_del_notify (u_int32_t vc_id, u_int32_t pw_type,
                        int peer_addr_type, struct pal_in4_addr *addr,
                        u_int32_t vc_snmp_index);
void
nsm_mpls_vc_up_down_notify (int opr_status,
                            u_int32_t vc_snmp_index);

u_int32_t
nsm_mpls_set_pw_mpls_mpls_type (struct nsm_master *,
                                u_int32_t, char *);
u_int32_t
nsm_mpls_set_pw_mpls_exp_bits_mode (struct nsm_master *,
                                    u_int32_t, u_int32_t);
u_int32_t
nsm_mpls_set_pw_mpls_exp_bits (struct nsm_master *,
                               u_int32_t, u_int32_t);
u_int32_t
nsm_mpls_set_pw_mpls_ttl (struct nsm_master *,
                          u_int32_t, u_int32_t);
u_int32_t
nsm_mpls_set_pw_mpls_lcl_ldp_id (struct nsm_master *, u_int32_t,
                                 struct ldp_id *);
u_int32_t
nsm_mpls_set_pw_mpls_lcl_ldp_entty_ix (struct nsm_master *,
                                       u_int32_t, u_int32_t);
u_int32_t
nsm_mpls_set_pw_st_type (struct nsm_master *,
                         u_int32_t, u_int32_t);
u_int32_t
nsm_mpls_set_pw_mpls_outbd_lsr_xc_ix (struct nsm_master *,
                                      u_int32_t , u_int32_t);
u_int32_t
nsm_mpls_set_pw_mpls_outbd_tnl_ix (struct nsm_master *,
                                   u_int32_t, u_int32_t);
u_int32_t
nsm_mpls_set_pw_mpls_outbd_tnl_lcl_lsr (struct nsm_master *,
                                        u_int32_t,
                                        struct pal_in4_addr *);
u_int32_t
nsm_mpls_set_pw_mpls_outbd_tnl_peer_lsr (struct nsm_master *,
                                         u_int32_t,
                                         struct pal_in4_addr *);
u_int32_t
nsm_mpls_set_pw_mpls_outbd_if_ix (struct nsm_master *,
                                  u_int32_t, u_int32_t);

u_int32_t
nsm_mpls_set_pw_oam_enable (struct nsm_master *nm,
                            u_int32_t pw_ix, u_int32_t oam_en);

#ifdef HAVE_FEC129
u_int32_t
nsm_mpls_set_pw_gen_agi_type (struct nsm_master *nm,
                              u_int32_t pw_ix,
                              u_int32_t gen_agi_type);

u_int32_t
nsm_mpls_set_pw_gen_loc_aii_type (struct nsm_master *nm,
                                   u_int32_t pw_ix,
                                   u_int32_t );

u_int32_t
nsm_mpls_set_pw_gen_rem_aii_type (struct nsm_master *nm,
                                   u_int32_t pw_ix,
                                   u_int32_t );

#endif /* HAVE_FEC129 */
#endif /* HAVE_SNMP */
#endif /* HAVE_MPLS_VC */
#endif /* _NSM_MPLS_VC_SNMP_H */
