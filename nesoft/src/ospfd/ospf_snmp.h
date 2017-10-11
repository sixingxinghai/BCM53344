/* Copyright (C) 2001-2003  All Rights Reserved. */

#ifndef _PACOS_OSPF_SNMP_H
#define _PACOS_OSPF_SNMP_H

/* SNMP Index Exact. */
#define OSPF_SNMP_INDEX_NOT_EXACT       0
#define OSPF_SNMP_INDEX_EXACT           1

/* OSPF2-MIB. */
#define OSPF2MIB 1,3,6,1,2,1,14

/* PacOS enterprise OSPF MIB.  This variable is used for register
   OSPF MIB to SNMP agent under SMUX protocol.  */
#define OSPFDOID 1,3,6,1,4,1,3317,1,2,5

/* OSPF MIB General Group values. */
#define OSPFROUTERID                     1
#define OSPFADMINSTAT                    2
#define OSPFVERSIONNUMBER                3
#define OSPFAREABDRRTRSTATUS             4
#define OSPFASBDRRTRSTATUS               5
#define OSPFEXTERNLSACOUNT               6
#define OSPFEXTERNLSACKSUMSUM            7
#define OSPFTOSSUPPORT                   8
#define OSPFORIGINATENEWLSAS             9
#define OSPFRXNEWLSAS                    10
#define OSPFEXTLSDBLIMIT                 11
#define OSPFMULTICASTEXTENSIONS          12
#define OSPFEXITOVERFLOWINTERVAL         13
#define OSPFDEMANDEXTENSIONS             14

/* OSPF MIB ospfAreaTable. */
#define OSPFAREAID                       1
#define OSPFAUTHTYPE                     2
#define OSPFIMPORTASEXTERN               3
#define OSPFSPFRUNS                      4
#define OSPFAREABDRRTRCOUNT              5
#define OSPFASBDRRTRCOUNT                6
#define OSPFAREALSACOUNT                 7
#define OSPFAREALSACKSUMSUM              8
#define OSPFAREASUMMARY                  9
#define OSPFAREASTATUS                   10

/* OSPF MIB ospfStubAreaTable. */
#define OSPFSTUBAREAID                   1
#define OSPFSTUBTOS                      2
#define OSPFSTUBMETRIC                   3
#define OSPFSTUBSTATUS                   4
#define OSPFSTUBMETRICTYPE               5

/* OSPF MIB ospfLsdbTable. */
#define OSPFLSDBAREAID                   1
#define OSPFLSDBTYPE                     2
#define OSPFLSDBLSID                     3
#define OSPFLSDBROUTERID                 4
#define OSPFLSDBSEQUENCE                 5
#define OSPFLSDBAGE                      6
#define OSPFLSDBCHECKSUM                 7
#define OSPFLSDBADVERTISEMENT            8

/* OSPF MIB ospfAreaRangeTable. */
#define OSPFAREARANGEAREAID              1
#define OSPFAREARANGENET                 2
#define OSPFAREARANGEMASK                3
#define OSPFAREARANGESTATUS              4
#define OSPFAREARANGEEFFECT              5

/* OSPF MIB ospfHostTable. */
#define OSPFHOSTIPADDRESS                1
#define OSPFHOSTTOS                      2
#define OSPFHOSTMETRIC                   3
#define OSPFHOSTSTATUS                   4
#define OSPFHOSTAREAID                   5

/* OSPF MIB ospfIfTable. */
#define OSPFIFIPADDRESS                  1
#define OSPFADDRESSLESSIF                2
#define OSPFIFAREAID                     3
#define OSPFIFTYPE                       4
#define OSPFIFADMINSTAT                  5
#define OSPFIFRTRPRIORITY                6
#define OSPFIFTRANSITDELAY               7
#define OSPFIFRETRANSINTERVAL            8
#define OSPFIFHELLOINTERVAL              9
#define OSPFIFRTRDEADINTERVAL            10
#define OSPFIFPOLLINTERVAL               11
#define OSPFIFSTATE                      12
#define OSPFIFDESIGNATEDROUTER           13
#define OSPFIFBACKUPDESIGNATEDROUTER     14
#define OSPFIFEVENTS                     15
#define OSPFIFAUTHKEY                    16
#define OSPFIFSTATUS                     17
#define OSPFIFMULTICASTFORWARDING        18
#define OSPFIFDEMAND                     19
#define OSPFIFAUTHTYPE                   20

/* OSPF MIB ospfIfMetricTable. */
#define OSPFIFMETRICIPADDRESS            1
#define OSPFIFMETRICADDRESSLESSIF        2
#define OSPFIFMETRICTOS                  3
#define OSPFIFMETRICVALUE                4
#define OSPFIFMETRICSTATUS               5

/* OSPF MIB ospfVirtIfTable. */
#define OSPFVIRTIFAREAID                 1
#define OSPFVIRTIFNEIGHBOR               2
#define OSPFVIRTIFTRANSITDELAY           3
#define OSPFVIRTIFRETRANSINTERVAL        4
#define OSPFVIRTIFHELLOINTERVAL          5
#define OSPFVIRTIFRTRDEADINTERVAL        6
#define OSPFVIRTIFSTATE                  7
#define OSPFVIRTIFEVENTS                 8
#define OSPFVIRTIFAUTHKEY                9
#define OSPFVIRTIFSTATUS                 10
#define OSPFVIRTIFAUTHTYPE               11

/* OSPF MIB ospfNbrTable. */
#define OSPFNBRIPADDR                    1
#define OSPFNBRADDRESSLESSINDEX          2
#define OSPFNBRRTRID                     3
#define OSPFNBROPTIONS                   4
#define OSPFNBRPRIORITY                  5
#define OSPFNBRSTATE                     6
#define OSPFNBREVENTS                    7
#define OSPFNBRLSRETRANSQLEN             8
#define OSPFNBMANBRSTATUS                9
#define OSPFNBMANBRPERMANENCE            10
#define OSPFNBRHELLOSUPPRESSED           11

/* OSPF MIB ospfVirtNbrTable. */
#define OSPFVIRTNBRAREA                  1
#define OSPFVIRTNBRRTRID                 2
#define OSPFVIRTNBRIPADDR                3
#define OSPFVIRTNBROPTIONS               4
#define OSPFVIRTNBRSTATE                 5
#define OSPFVIRTNBREVENTS                6
#define OSPFVIRTNBRLSRETRANSQLEN         7
#define OSPFVIRTNBRHELLOSUPPRESSED       8

/* OSPF MIB ospfExtLsdbTable. */
#define OSPFEXTLSDBTYPE                  1
#define OSPFEXTLSDBLSID                  2
#define OSPFEXTLSDBROUTERID              3
#define OSPFEXTLSDBSEQUENCE              4
#define OSPFEXTLSDBAGE                   5
#define OSPFEXTLSDBCHECKSUM              6
#define OSPFEXTLSDBADVERTISEMENT         7

/* OSPF MIB ospfAreaAggregateTable. */
#define OSPFAREAAGGREGATEAREAID          1
#define OSPFAREAAGGREGATELSDBTYPE        2
#define OSPFAREAAGGREGATENET             3
#define OSPFAREAAGGREGATEMASK            4
#define OSPFAREAAGGREGATESTATUS          5
#define OSPFAREAAGGREGATEEFFECT          6

/* OSPF Trap. */
#define OSPFVIRTIFSTATECHANGE            1
#define OSPFNBRSTATECHANGE               2
#define OSPFVIRTNBRSTATECHANGE           3
#define OSPFIFCONFIGERROR                4
#define OSPFVIRTIFCONFIGERROR            5
#define OSPFIFAUTHFAILURE                6
#define OSPFVIRTIFAUTHFAILURE            7
#define OSPFIFRXBADPACKET                8
#define OSPFVIRTIFRXBADPACKET            9
#define OSPFTXRETRANSMIT                 10
#define OSPFVIRTIFTXRETRANSMIT           11
#define OSPFORIGINATELSA                 12
#define OSPFMAXAGELSA                    13
#define OSPFLSDBOVERFLOW                 14
#define OSPFLSDBAPPROACHINGOVERFLOW      15
#define OSPFIFSTATECHANGE                16

/* OSPF Config Error Type. */
#define OSPF_BAD_VERSION                 1
#define OSPF_AREA_MISMATCH               2
#define OSPF_UNKNOWN_NBMA_NBR            3
#define OSPF_UNKNOWN_VIRTUAL_NBR         4
#define OSPF_AUTH_TYPE_MISMATCH          5
#define OSPF_AUTH_FAILURE                6
#define OSPF_NET_MASK_MISMATCH           7
#define OSPF_HELLO_INTERVAL_MISMATCH     8
#define OSPF_DEAD_INTERVAL_MISMATCH      9
#define OSPF_OPTION_MISMATCH             10

/* SNMP value hack. */
#define COUNTER     ASN_COUNTER
#define INTEGER     ASN_INTEGER
#define GAUGE       ASN_GAUGE
#define TIMETICKS   ASN_TIMETICKS
#define IPADDRESS   ASN_IPADDRESS
#define STRING      ASN_OCTET_STR

#define OSPF_SNMP_UNION(S,N) \
union { \
struct S table; \
struct ospf_snmp_index snmp; \
} N

/* SNMP Get and GetNext related macros.
   These macros implicitly use `proc_id', `exact' and `ospf_var_len'. */
#define OSPF_SNMP_GET(F, V, W)                                                \
    ((ospf_get_ ## F (proc_id, (V), (W))) == OSPF_API_GET_SUCCESS)

#define OSPF_SNMP_GET_NEXT(F, A, L, V, W)                                     \
    ((exact ?                                                                 \
      ospf_get_ ## F (proc_id, (A), (V), (W)) :                               \
      ospf_get_next_ ## F (proc_id, &(A), (L), (V), (W)))                     \
     == OSPF_API_GET_SUCCESS)

#define OSPF_SNMP_GET3NEXT(F, A, B, L, V, W)                                  \
    ((exact ?                                                                 \
      ospf_get_ ## F (proc_id, (A), (B), (V), (W)) :                          \
      ospf_get_next_ ## F (proc_id, &(A), &(B), (L), (V), (W)))               \
     == OSPF_API_GET_SUCCESS)

#define OSPF_SNMP_GET4NEXT(F, A, B, C, L, V, W)                               \
    ((exact ?                                                                 \
      ospf_get_ ## F (proc_id, (A), (B), (C), (V), (W)) :                     \
      ospf_get_next_ ## F (proc_id, &(A), &(B), &(C), (L), (V), (W)))         \
     == OSPF_API_GET_SUCCESS)

#define OSPF_SNMP_GET5NEXT(F, A, B, C, D, L, V, W)                            \
    ((exact ?                                                                 \
      ospf_get_ ## F (proc_id, (A), (B), (C), (D), (V), (W)) :                \
      ospf_get_next_ ## F (proc_id, &(A), &(B), &(C), &(D), (L), (V), (W)))   \
     == OSPF_API_GET_SUCCESS)

/* For returning u_char entries. */
#define OSPF_SNMP_GET3CHAR_T(F, A, B, C, L, V, W)                             \
    ((exact ?                                                                 \
      ospf_get_ ## F (proc_id, (A), (B), (C), (V), var_len, (W)) :            \
      ospf_get_next_ ## F (proc_id, &(A), &(B), &(C), (L), (V), var_len, (W)))\
     == OSPF_API_GET_SUCCESS)

#define OSPF_SNMP_GET4CHAR_T(F, A, B, C, D, L, V, W)                          \
    ((exact ?                                                                 \
      ospf_get_ ## F (proc_id, (A), (B), (C), (D), (V), var_len, (W)) :       \
      ospf_get_next_ ## F (proc_id, &(A), &(B), &(C), &(D), (L), (V), var_len, (W))) \
     == OSPF_API_GET_SUCCESS)

#define OSPF_SNMP_SET(F, V, W)                                                \
    ((ret = ospf_set_ ## F (proc_id, (V), (W))) == OSPF_API_SET_SUCCESS)

#define OSPF_SNMP_SET1(F, A, V, W)                                             \
    ((ret = ospf_set_ ## F (proc_id, (A), (V), (W))) == OSPF_API_SET_SUCCESS)

#define OSPF_SNMP_SET2(F, A, B, V, W)                                         \
  ((ret = ospf_set_ ## F (proc_id, (A), (B), (V), (W))) == OSPF_API_SET_SUCCESS)

#define OSPF_SNMP_SET3(F, A, B, C, V, W)                                      \
    ((ret = ospf_set_ ## F (proc_id, (A), (B), (C), (V), (W)))                \
      == OSPF_API_SET_SUCCESS)

#define OSPF_SNMP_SET4(F, A, B, C, D, V, W)                                   \
    ((ret = ospf_set_ ## F (proc_id, (A), (B), (C), (D), (V), (W)))           \
      == OSPF_API_SET_SUCCESS)

#define OSPF_SNMP_RETURN(V, T)                                                \
    do {                                                                      \
      ospf_snmp_index_set (v, name, length,                                   \
                           (struct ospf_snmp_index *)&index, (T));            \
      *var_len = sizeof (V);                                                  \
      return (u_char *) &(V);                                                 \
    } while (0)

#define OSPF_SNMP_RETURN_SIZE0(V, T)                                          \
    do {                                                                      \
      ospf_snmp_index_set (v, name, length,                                   \
                           (struct ospf_snmp_index *)&index, (T));            \
      *var_len = 0;                                                           \
      return (u_char *) (V);                                                  \
    } while (0)

#define OSPF_SNMP_RETURN_CHAR_T(V, T)                                         \
    do {                                                                      \
      ospf_snmp_index_set (v, name, length,                                   \
                           (struct ospf_snmp_index *)&index, (T));            \
      return (u_char *) (V);                                                  \
    } while (0)

#define OSPF_SNMP_RETURN_ERROR(R)                                             \
    ((R) == OSPF_API_SET_ERR_READONLY ? SNMP_ERR_READONLY :                   \
     (R) == OSPF_API_SET_ERR_WRONG_VALUE ? SNMP_ERR_BADVALUE :                \
     (R) == OSPF_API_SET_ERR_AREA_IS_DEFAULT ? SNMP_ERR_BADVALUE :            \
     (R) == OSPF_API_SET_ERR_EXTERNAL_LSDB_LIMIT_INVALID ? SNMP_ERR_BADVALUE :\
     (R) == OSPF_API_SET_ERR_OVERFLOW_INTERVAL_INVALID ? SNMP_ERR_BADVALUE :  \
     (R) == OSPF_API_SET_ERR_COST_INVALID ? SNMP_ERR_BADVALUE :               \
     (R) == OSPF_API_SET_ERR_IF_TRANSMIT_DELAY_INVALID ? SNMP_ERR_BADVALUE :  \
     (R) == OSPF_API_SET_ERR_IF_RETRANSMIT_INTERVAL_INVALID?SNMP_ERR_BADVALUE:\
     (R) == OSPF_API_SET_ERR_IF_HELLO_INTERVAL_INVALID ? SNMP_ERR_BADVALUE :  \
     (R) == OSPF_API_SET_ERR_IF_DEAD_INTERVAL_INVALID ? SNMP_ERR_BADVALUE :   \
     (R) == OSPF_API_SET_ERR_IF_COST_INVALID ? SNMP_ERR_BADVALUE :            \
     (R) == OSPF_API_SET_ERR_AUTH_TYPE_INVALID ? SNMP_ERR_BADVALUE :          \
     (R) == OSPF_API_SET_ERR_METRIC_TYPE_INVALID ? SNMP_ERR_BADVALUE :        \
     (R) == OSPF_API_SET_ERR_METRIC_INVALID ? SNMP_ERR_BADVALUE :             \
     SNMP_ERR_GENERR)

/* Prototypes. */
void ospfVirtIfStateChange (struct ospf_vlink *);
void ospfNbrStateChange (struct ospf_neighbor *);
void ospfVirtNbrStateChange (struct ospf_neighbor *);
void ospfIfConfigError (struct ospf_interface *, struct pal_in4_addr, int,
                        int);
void ospfVirtIfConfigError (struct ospf_vlink *, int, int);
void ospfTxRetransmit (struct ospf_neighbor *, struct ospf_area *,
                       int, int, struct pal_in4_addr *, 
                       struct pal_in4_addr *);
void ospfVirtIfTxRetransmit (struct ospf_vlink *vlink,
                             int, int, struct pal_in4_addr *, 
                             struct pal_in4_addr *);
void ospfIfRxBadPacket (struct ospf_interface *oi, struct pal_in4_addr src,
                        int type);
void ospfVirtIfRxBadPacket (struct ospf_vlink *vlink,
                            int type);
void ospfOriginateLsa (struct ospf *, struct ospf_lsa *, int);
void ospfLsdbOverflow (struct ospf *, int);

void ospfIfStateChange (struct ospf_interface *);

#ifdef HAVE_SNMP_RESTART
void ospf_snmp_restart ();
#endif /* HAVE_SNMP_RESTART */

#endif /* _PACOS_OSPF_SNMP_H */
