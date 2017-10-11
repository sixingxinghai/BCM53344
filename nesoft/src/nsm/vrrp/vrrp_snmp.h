/* Copyright (C) 2003  All Rights Reserved. */

#ifndef _PACOS_VRRP_SNMP_H
#define _PACOS_VRRP_SNMP_H

#include "pal.h"
#include "nsm_snmp.h"

/* VRRP-MIB */
#define VRRPMIB 1,3,6,1,2,1,68

/* VRRP-MIB single instance information */
#define VRRPNOTIFICATIONCNTL            0
/* VRRP-MIB OperTable single instance information */
#define VRRPOPERATIONSINETADDRTYPE    1
#define VRRPOPERATIONSVRID            2
#define VRRPOPERATIONSVIRTUALMACADDR  3
#define VRRPOPERATIONSSTATE           4
#define VRRPOPERATIONSPRIORITY        5
#define VRRPOPERATIONSADDRCOUNT       6
#define VRRPOPERATIONSMASTERIPADDR    7
#define VRRPOPERATIONSPRIMARYIPADDR   8
#define VRRPOPERATIONSADVINTERVAL     9
#define VRRPOPERATIONSPREEMPTMODE     10
#define VRRPOPERATIONSACCEPTMODE      11
#define VRRPOPERATIONSUPTIME          12
#define VRRPOPERATIONSSTORAGETYPE     13
#define VRRPOPERATIONSROWSTATUS       14

/* VRRP-MIB AssoIpAddrTable single instance information */
#define VRRPASSOCIATEDIPADDR           1
#define VRRPASSOCIATEDSTORAGETYPE      2
#define VRRPASSOCIATEDIPADDRROWSTATUS  3

/* VRRP-MIB Statistics single instance information */
#define VRRPROUTERCHECKSUMERRORS       0
#define VRRPROUTERVERSIONERRORS        0
#define VRRPROUTERVRIDERRORS           0

#define VRRPSTATISTICSMASTERTRANSITIONS           1
#define VRRPSTATISTICSRCVDADVERTISEMENTS          2
#define VRRPSTATISTICSADVINTERVALERRORS           3
#define VRRPSTATISTICSIPTTLERRORS                 4
#define VRRPSTATISTICSRCVDPRIZEROPACKETS          5
#define VRRPSTATISTICSSENTPRIZEROPACKETS          6
#define VRRPSTATISTICSRCVDINVALIDTYPEPKTS         7
#define VRRPSTATISTICSADDRESSLISTERRORS           8
#define VRRPSTATISTICSPACKETLENGTHERRORS          9
#define VRRPSTATISTICSRCVDINVALIDAUTHENTICATIONS 10
#define VRRPSTATISTICSDISCONTINUITYTIME          11
#define VRRPSTATISTICSREFRESHRATE                12


#define VRRPNEWMASTERREASON           0

/*VRRP-MIB TRAP Information */
#define VRRPTRAPNEWMASTER           1
#define VRRPTRAPPROTOERROR          3

/* vrrpNewMasterReason values */
#define VRRP_NEW_MASTER_REASON_notmaster        0
#define VRRP_NEW_MASTER_REASON_priority         1
#define VRRP_NEW_MASTER_REASON_preempted        2
#define VRRP_NEW_MASTER_REASON_masterNoResponse 3

#define VRRP_TRAP_PROTO_ERR_REASON_hopLimitError 0
#define VRRP_TRAP_PROTO_ERR_REASON_versionError  1
#define VRRP_TRAP_PROTO_ERR_REASON_checksumError 2
#define VRRP_TRAP_PROTO_ERR_REASON_vridError     3

/* SNMP value hack. */
#define COUNTER     ASN_COUNTER
#define INTEGER     ASN_INTEGER
#define INETADDRESS ASN_OCTET_STR
#define MACADDRESS  ASN_OCTET_STR
#define STRING      ASN_OCTET_STR
#define RCREATE     RWRITE

#define VRRP_OPER_TABLE_INDEX_LEN        3
#define VRRP_ASSO_IPV4_TABLE_INDEX_LEN   7
#define VRRP_ASSO_IPV6_TABLE_INDEX_LEN  19

#define VRRP_SNMP_GET(F, V) \
  ((vrrp_get_ ## F (apn_vrid, (V))) == VRRP_API_GET_SUCCESS)

#define VRRP_SNMP_GET_NEXT(F, A, V) \
  ((exact ? \
     vrrp_get_ ## F (apn_vrid, (A), (V)) : \
     vrrp_get_next_ ## F (apn_vrid, (A), (V))) \
   == VRRP_API_GET_SUCCESS)

#define VRRP_SNMP_GET3NEXT(F, A, B, C, L, V) \
  ((exact ? \
     vrrp_get_ ## F (apn_vrid, (A), (B), (C), (V)) : \
     vrrp_get_next_ ## F (apn_vrid, &(A), &(B), &(C), (L), (V))) \
   == VRRP_API_GET_SUCCESS)

#define VRRP_SNMP_GET4NEXT(F, A, B, C, D, L, V)     \
  ((exact ?                                      \
     vrrp_get_ ## F (apn_vrid, (A), (B), (C), (D), (V)) :       \
     vrrp_get_next_ ## F (apn_vrid, &(A), &(B), &(C), (D), (L), (V))) \
   == VRRP_API_GET_SUCCESS)

#define VRRP_SNMP_RETURN_BOOL(V) \
  do { \
    *var_len = sizeof (s_int32_t); \
    vrrp_int_val = VRRP_BOOL2SNMP(V); \
    return (u_char *) &vrrp_int_val; \
  } while (0)
  
#define VRRP_SNMP_RETURN_INTEGER(V) \
  do { \
    *var_len = sizeof (s_int32_t); \
    vrrp_int_val = V; \
    return (u_char *) &vrrp_int_val; \
  } while (0)

#define VRRP_SNMP_RETURN_UINT32(V) \
  do { \
    *var_len = sizeof (u_int32_t); \
    vrrp_uint_val = V; \
    return (u_char *) &vrrp_uint_val; \
  } while (0)

#define VRRP_SNMP_RETURN_IPADDRESS(AL, V) \
  do { \
    *var_len = AL; \
    pal_mem_cpy(vrrp_in_addr_val.uia.in_addr, V, AL); \
    return vrrp_in_addr_val.uia.in_addr; \
  } while (0)

#define VRRP_SNMP_RETURN_MACADDRESS(V) \
  do { \
    *var_len = SIZE_VMAC_ADDR; \
    vrrp_mac_addr_val = V; \
    return (u_char *) &vrrp_mac_addr_val; \
  } while (0)

#define VRRP_AT2AF(AT) \
  ((AT) == SNMP_AG_ADDR_TYPE_ipv4) ? AF_INET : (((AT)==SNMP_AG_ADDR_TYPE_ipv6) ? AF_INET6 : 0)

#define VRRP_AF2AT(AF) \
  ((AF) == AF_INET) ? SNMP_AG_ADDR_TYPE_ipv4 : (((AF)==AF_INET6) ? SNMP_AG_ADDR_TYPE_ipv6 : 0)

#define VRRP_BOOL2SNMP(BV) (BV) ? SNMP_TRUE : SNMP_FALSE
#define VRRP_SNMP2BOOL(SV) ((SV)==SNMP_TRUE) ? PAL_TRUE : PAL_FALSE

void vrrp_snmp_init (struct lib_globals *);
void vrrp_transition_to_master (VRRP_SESSION *sess);

#endif /* _PACOS_VRRP_SNMP_H */

