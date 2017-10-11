/* Copyright (C) 2001-2003  All Rights Reserved. */
#include "pal.h"
#include "lib.h"

#ifdef HAVE_MPLS_VC
#ifdef HAVE_SNMP

#include "asn1.h"
#include "bitmap.h"

#include "if.h"
#include "prefix.h"
#include "snmp.h"
#include "log.h"
#include "mpls_common.h"
#include "mpls.h"
#include "nsmd.h"
#ifdef HAVE_DIFFSERV
#include "diffserv.h"
#endif

#include "nsm_router.h"
#include "nsm_mpls.h"
#include "nsm_mpls_vc.h"
#include "nsm_mpls_rib.h"
#include "nsm_mpls_vc_snmp.h"
#include "nsm_mpls_vc_api.h"
#include "nsm_mpls_snmp.h"
#include "nsm_mpls_api.h"

/* Declare static local variables for convenience. */

static u_int32_t nsm_pw_snmp_int_val = NSM_PW_SNMP_VALUE_INVALID;
static u_int64_t nsm_pw_snmp_int_hc_val;
static struct pal_in4_addr ip_addr;

static char nsm_char_string[NSM_PW_MAX_STRING_LENGTH];
static char *nsm_pw_snmp_char_ptr = nsm_char_string;


/*MPLS-PW-MIB instances. */
oid pw_oid [] = { PWSTDMIB };
oid pw_trap_oid[] = { PWSTDMIB, 0};

/* MPLS-PW-ENET-MIB instances*/
oid pw_enet_oid [] = { PWENETMIB };

/*PW-MPLS-MIB instances. */
oid pw_mpls_oid [] = { PWMPLSSTDMIB };

/* Hook functions. */
static u_char *pwScalars ();
static u_char *pwVCEntry ();
static u_char *pwPerfCurrentEntry ();
static u_char *pwPerfIntervalEntry ();
static u_char *pwPerf1DayIntervalEntry ();
static u_char *pwIndexMappingEntry ();
static u_char *pwPeerMappingEntry ();
#ifdef HAVE_FEC129
static u_char *pwGenFecIndxMapEntry ();
#endif /*HAVE_FEC129 */

/* PW ENET MIB Functions */
static u_char *pwEnetEntry ();
static u_char *PwEnetStatsEntry ();

/* PW MPLS Table functions*/
static u_char *pwMplsEntry ();
static u_char *pwMplsOutboundEntry ();
static u_char *pwMplsInboundEntry ();
static u_char *pwMplsNonTeMappingEntry ();
static u_char *pwMplsTeMappingEntry ();

struct variable nsm_pw_objects[] = 
  {
    /* Pseudo Wire Index Next */
    {PW_IX_NXT,                 INTEGER,   RONLY, pwScalars,
     2, {1, 1}},

    /* PW Virtual Connection Table. */
    {PW_INDEX,                  INTEGER,   NOACCESS, pwVCEntry,
     4, {1, 2, 1, 1}},
    {PW_TYPE,                   INTEGER,   RWRITE,   pwVCEntry,
     4, {1, 2, 1, 2}},
    {PW_OWNER,                  INTEGER,   RWRITE,   pwVCEntry,
     4, {1, 2, 1, 3}},
    {PW_PSN_TYPE,               INTEGER,   RWRITE,   pwVCEntry,
     4, {1, 2, 1, 4}},
    {PW_SETUP_PRTY,             INTEGER,   RWRITE,   pwVCEntry,
     4, {1, 2, 1, 5}},
    {PW_HOLD_PRTY,              INTEGER,   RWRITE,   pwVCEntry,
     4, {1, 2, 1, 6}},
    {PW_PEER_ADDR_TYPE,         INTEGER,   RWRITE,   pwVCEntry,
     4, {1, 2, 1, 8}},
    {PW_PEER_ADDR,              STRING,    RWRITE,   pwVCEntry,
     4, {1, 2, 1, 9}},
    {PW_ATTCHD_PW_IX,           INTEGER,   RWRITE,   pwVCEntry,
     4, {1, 2, 1, 10}},
    {PW_IF_IX,                  INTEGER,   RWRITE,   pwVCEntry,
     4, {1, 2, 1, 11}},
    {PW_ID,                     GAUGE32,   RWRITE,   pwVCEntry,
     4, {1, 2, 1, 12}},
    {PW_LOCAL_GRP_ID,           GAUGE32,   RWRITE,   pwVCEntry,
     4, {1, 2, 1, 13}},
    {PW_GRP_ATTCHMT_ID,         STRING,    RWRITE,   pwVCEntry,
     4, {1, 2, 1, 14}},
    {PW_LOCAL_ATTCHMT_ID,       STRING,    RWRITE,   pwVCEntry,
     4, {1, 2, 1, 15}},
    {PW_PEER_ATTCHMT_ID,        STRING,    RWRITE,   pwVCEntry,
     4, {1, 2, 1, 16}},
    {PW_CW_PRFRNCE,             TRUTHVALUE,RWRITE,   pwVCEntry,
     4, {1, 2, 1, 17}},
    {PW_LOCAL_IF_MTU,           INTEGER,   RWRITE,   pwVCEntry,
     4, {1, 2, 1, 18}},
    {PW_LOCAL_IF_STRING,        TRUTHVALUE,RWRITE,   pwVCEntry,
     4, {1, 2, 1, 19}},
    {PW_LOCAL_CAPAB_ADVRT,      PW_BITS,   RWRITE,   pwVCEntry,
     4, {1, 2, 1, 20}},
    {PW_REMOTE_GRP_ID,          GAUGE32,   RONLY,    pwVCEntry,
     4, {1, 2, 1, 21}},
    {PW_CW_STATUS,              INTEGER,   RONLY,    pwVCEntry,
     4, {1, 2, 1, 22}},
    {PW_REMOTE_IF_MTU,          INTEGER,   RONLY,    pwVCEntry,
     4, {1, 2, 1, 23}},
    {PW_REMOTE_IF_STRING,       STRING,    RONLY,    pwVCEntry,
     4, {1, 2, 1, 24}},
    {PW_REMOTE_CAPAB,           PW_BITS,   RONLY,    pwVCEntry,
     4, {1, 2, 1, 25}},
    {PW_FRGMT_CFG_SIZE,         INTEGER,   RWRITE,   pwVCEntry,
     4, {1, 2, 1, 26}},
    {PW_RMT_FRGMT_CAPAB,        PW_BITS,   RONLY,    pwVCEntry,
     4, {1, 2, 1, 27}},
    {PW_FCS_RETENTN_CFG,        INTEGER,   RWRITE,   pwVCEntry,
     4, {1, 2, 1, 28}},
    {PW_FCS_RETENTN_STATUS,     PW_BITS,   RONLY,    pwVCEntry,
     4, {1, 2, 1, 29}},
    {PW_OUTBD_LABEL,            INTEGER,   RWRITE,   pwVCEntry,
     4, {1, 2, 1, 30}},
    {PW_INBD_LABEL,             INTEGER,   RWRITE,   pwVCEntry,
     4, {1, 2, 1, 31}},
    {PW_NAME,                   STRING,    RWRITE,   pwVCEntry,
     4, {1, 2, 1, 32}},
    {PW_DESCR,                  STRING,    RWRITE,   pwVCEntry,
     4, {1, 2, 1, 33}},
    {PW_CREATE_TIME,            TIMETICKS, RONLY,    pwVCEntry,
     4, {1, 2, 1, 34}},
    {PW_UP_TIME,                TIMETICKS, RONLY,    pwVCEntry,
     4, {1, 2, 1, 35}},
    {PW_LAST_CHANGE,            TIMETICKS, RONLY,    pwVCEntry,
     4, {1, 2, 1, 36}},
    {PW_ADMIN_STATUS,           INTEGER,   RWRITE,   pwVCEntry,
     4, {1, 2, 1, 37}},
    {PW_OPER_STATUS,            INTEGER,   RONLY,    pwVCEntry,
     4, {1, 2, 1, 38}},
    {PW_LOCAL_STATUS,           PW_BITS,   RONLY,    pwVCEntry,
     4, {1, 2, 1, 39}},
    {PW_RMT_STATUS_CAPAB,       INTEGER,   RONLY,    pwVCEntry,
     4, {1, 2, 1, 40}},
    {PW_RMT_STATUS,             PW_BITS,   RONLY,    pwVCEntry,
     4, {1, 2, 1, 41}},
    {PW_TIME_ELAPSED,           INTEGER,   RONLY,    pwVCEntry,
     4, {1, 2, 1, 42}},
    {PW_VALID_INTRVL,           COUNTER64, RONLY,    pwVCEntry,
     4, {1, 2, 1, 43}},
    {PW_ROW_STATUS,             INTEGER,   RWRITE,   pwVCEntry,
     4, {1, 2, 1, 44}},
    {PW_ST_TYPE,                INTEGER,   RWRITE,   pwVCEntry,
     4, {1, 2, 1, 45}},
    {PW_OAM_EN,                 TRUTHVALUE, RWRITE,   pwVCEntry,
     4, {1, 2, 1, 46}},
    {PW_GEN_AGI_TYPE,           INTEGER,   RWRITE,   pwVCEntry,
     4, {1, 2, 1, 47}},
    {PW_GEN_LOC_AGII_TYPE,      INTEGER,   RWRITE,   pwVCEntry,
     4, {1, 2, 1, 48}},
    {PW_GEN_REM_AGII_TYPE,      INTEGER,   RWRITE,   pwVCEntry,
     4, {1, 2, 1, 49}},

    /* PW Performance Current Table. */
    {PW_PERF_CRNT_IN_HC_PCKTS,    COUNTER64,   RONLY, pwPerfCurrentEntry,
     4, {1, 3, 1, 1}},
    {PW_PERF_CRNT_IN_HC_BYTES,    COUNTER64,   RONLY, pwPerfCurrentEntry,
     4, {1, 3, 1, 2}},
    {PW_PERF_CRNT_OUT_HC_PCKTS,   COUNTER64,   RONLY, pwPerfCurrentEntry,
     4, {1, 3, 1, 3}},
    {PW_PERF_CRNT_OUT_HC_BYTES,   COUNTER64,   RONLY, pwPerfCurrentEntry,
     4, {1, 3, 1, 4}},
    {PW_PERF_CRNT_IN_PCKTS,       INTEGER,     RONLY, pwPerfCurrentEntry,
     4, {1, 3, 1, 5}},
    {PW_PERF_CRNT_IN_BYTES,       INTEGER,     RONLY, pwPerfCurrentEntry,
     4, {1, 3, 1, 6}},
    {PW_PERF_CRNT_OUT_PCKTS,      INTEGER,     RONLY, pwPerfCurrentEntry,
     4, {1, 3, 1, 7}},
    {PW_PERF_CRNT_OUT_BYTES,      INTEGER,     RONLY, pwPerfCurrentEntry,
     4, {1, 3, 1, 8}},

    /* PW Performance Interval Table. */
    {PW_PERF_INTRVL_NO,           INTEGER,     NOACCESS,  pwPerfIntervalEntry,
     4, {1, 4, 1, 1}},
    {PW_PERF_INTRVL_VALID_DATA,   TRUTHVALUE,  RONLY,     pwPerfIntervalEntry,
     4, {1, 4, 1, 2}},
    {PW_PERF_INTRVL_TIME_ELPSD,   INTEGER,     RONLY,     pwPerfIntervalEntry,
     4, {1, 4, 1, 3}},
    {PW_PERF_INTRVL_IN_HC_PCKTS,  COUNTER64,   RONLY,     pwPerfIntervalEntry,
     4, {1, 4, 1, 4}},
    {PW_PERF_INTRVL_IN_HC_BYTES,  COUNTER64,   RONLY,     pwPerfIntervalEntry,
     4, {1, 4, 1, 5}},
    {PW_PERF_INTRVL_OUT_HC_PCKTS, COUNTER64,   RONLY,     pwPerfIntervalEntry,
     4, {1, 4, 1, 6}},
    {PW_PERF_INTRVL_OUT_HC_BYTES, COUNTER64,   RONLY,     pwPerfIntervalEntry,
     4, {1, 4, 1, 7}},
    {PW_PERF_INTRVL_IN_PCKTS,     INTEGER,     RONLY,     pwPerfIntervalEntry,
     4, {1, 4, 1, 8}},
    {PW_PERF_INTRVL_IN_BYTES,     INTEGER,     RONLY,     pwPerfIntervalEntry,
     4, {1, 4, 1, 9}},
    {PW_PERF_INTRVL_OUT_PCKTS,    INTEGER,     RONLY,     pwPerfIntervalEntry,
     4, {1, 4, 1, 10}},
    {PW_PERF_INTRVL_OUT_BYTES,    INTEGER,     RONLY,     pwPerfIntervalEntry,
     4, {1, 4, 1, 11}},

    /* PW Performance 1 Day Interval Table. */
    {PW_PERF_1DY_INTRVL_NO,        INTEGER,  NOACCESS, pwPerf1DayIntervalEntry,
     4, {1, 5, 1, 1}},
    {PW_PERF_1DY_INTRVL_VALID_DT,  TRUTHVALUE,  RONLY, pwPerf1DayIntervalEntry,
     4, {1, 5, 1, 2}},
    {PW_PERF_1DY_INTRVL_MONI_SECS, INTEGER,     RONLY, pwPerf1DayIntervalEntry,
     4, {1, 5, 1, 3}},
    {PW_PERF_1DY_INTRVL_IN_HC_PCKTS, COUNTER64, RONLY, pwPerf1DayIntervalEntry,
     4, {1, 5, 1, 4}},
    {PW_PERF_1DY_INTRVL_IN_HC_BYTES, COUNTER64, RONLY, pwPerf1DayIntervalEntry,
     4, {1, 5, 1, 5}},
    {PW_PERF_1DY_INTRVL_OUT_HC_PCKTS,COUNTER64, RONLY, pwPerf1DayIntervalEntry,
     4, {1, 5, 1, 6}},
    {PW_PERF_1DY_INTRVL_OUT_HC_BYTES,COUNTER64, RONLY, pwPerf1DayIntervalEntry,
     4, {1, 5, 1, 7}},

    /* Error counter scalar */
    {PW_PERF_TOT_ERR_PCKTS,     COUNTER32,   RONLY,    pwScalars,
     2, {1, 6}},

    /* The PW ID mapping table */
    {PW_ID_MAP_PW_TYPE,         INTEGER,     NOACCESS, pwIndexMappingEntry,
     4, {1, 7, 1, 1}},
    {PW_ID_MAP_PW_ID,           INTEGER,     NOACCESS, pwIndexMappingEntry,
     4, {1, 7, 1, 2}},
    {PW_ID_MAP_PEER_ADDR_TYPE,  INTEGER,     NOACCESS, pwIndexMappingEntry,
     4, {1, 7, 1, 3}},
    {PW_ID_MAP_PEER_ADDR,       STRING,      NOACCESS, pwIndexMappingEntry,
     4, {1, 7, 1, 4}},
    {PW_ID_MAP_PW_IX,           INTEGER,     RONLY,    pwIndexMappingEntry,
     4, {1, 7, 1, 5}},

    /* The peer mapping table */
    {PW_PEER_MAP_PEER_ADDR_TYPE,INTEGER,     NOACCESS, pwPeerMappingEntry,
     4, {1, 8, 1, 1}},
    {PW_PEER_MAP_PEER_ADDR,     STRING,      NOACCESS, pwPeerMappingEntry,
     4, {1, 8, 1, 2}},
    {PW_PEER_MAP_PW_TYPE,       INTEGER,     NOACCESS, pwPeerMappingEntry,
     4, {1, 8, 1, 3}},
    {PW_PEER_MAP_PW_ID,         INTEGER,     NOACCESS, pwPeerMappingEntry,
     4, {1, 8, 1, 4}},
    {PW_PEER_MAP_PW_IX,         INTEGER,     RONLY,    pwPeerMappingEntry,
     4, {1, 8, 1, 5}},

    /* Pseudo Wire objects scalar */
    {PW_UP_DWN_NOTIF_EN,        TRUTHVALUE,  RWRITE,   pwScalars,
     2, {1, 9}},
    {PW_DEL_NOTIF_EN,           TRUTHVALUE,  RWRITE,   pwScalars,
     2, {1, 10}},
    {PW_NOTIF_RATE,             INTEGER,     RWRITE,   pwScalars,
     2, {1, 11}},

#ifdef HAVE_FEC129
    /* Generailsed FEC table */ 
    {PW_GEN_FEC_MAP_AGI_TYPE,   INTEGER,    NOACCESS,   pwGenFecIndxMapEntry,
     4, {1, 12, 1, 1}},
    {PW_GEN_FEC_MAP_AGI,        STRING,     NOACCESS,   pwGenFecIndxMapEntry,
     4, {1, 12, 1, 2}},
    {PW_GEN_FEC_LOC_AII_TYPE,   INTEGER,    NOACCESS,   pwGenFecIndxMapEntry,
     4, {1, 12, 1, 3}},
    {PW_GEN_FEC_LOC_AII,        STRING,     NOACCESS,   pwGenFecIndxMapEntry,
     4, {1, 12, 1, 4}},
    {PW_GEN_FEC_REM_AII_TYPE,   INTEGER,    NOACCESS,   pwGenFecIndxMapEntry,
     4, {1, 12, 1, 5}},
    {PW_GEN_FEC_REM_AII,        STRING,     NOACCESS,   pwGenFecIndxMapEntry,
     4, {1, 12, 1, 6}},
    {PW_GEN_FEC_PW_ID,          INTEGER,    RONLY,      pwGenFecIndxMapEntry,
     4, {1, 12, 1, 7}},
#endif /*HAVE_FEC129 */
  };

struct variable pw_enet_objects[] =
  {
    /* Pseudo-wire Enet Table Objects */
    {PW_ENET_PW_INSTANCE,            INTEGER,   NOACCESS, pwEnetEntry,
     4, {1, 1, 1, 1}},
    {PW_ENET_PW_VLAN,                INTEGER,     RWRITE, pwEnetEntry,
     4, {1, 1, 1, 2}},
    {PW_ENET_VLAN_MODE,              INTEGER,     RWRITE, pwEnetEntry,
     4, {1, 1, 1, 3}},
    {PW_ENET_PORT_VLAN,              INTEGER,     RWRITE, pwEnetEntry ,
     4, {1, 1, 1, 4}},
    {PW_ENET_PORT_IF_INDEX,          INTEGER,     RWRITE, pwEnetEntry,
     4, {1, 1, 1, 5}},
    {PW_ENET_PW_IF_INDEX,            INTEGER,     RWRITE, pwEnetEntry,
     4, {1, 1, 1, 6}},
    {PW_ENET_ROW_STATUS,             INTEGER,     RWRITE, pwEnetEntry,
     4, {1, 1, 1, 7}},
    {PW_ENET_STORAGE_TYPE,           INTEGER,     RWRITE, pwEnetEntry,
     4, {1, 1, 1, 8}},

    /* Pseudo-wire enet Statistics Table */
    {PW_ENET_STATS_ILLEGAL_VLAN,     INTEGER,     RONLY,  PwEnetStatsEntry,
     4, {1, 2, 1, 1}},
    {PW_ENET_STATS_ILLEGAL_LENGTH,   INTEGER,     RONLY,  PwEnetStatsEntry,
     4, {1, 2, 1, 2}},
  };

struct variable nsm_pw_mpls_objects[] =
  {
    /* PW MPLS Table. */
    {PW_MPLS_MPLS_TYPE,         PW_BITS,     RWRITE,   pwMplsEntry,
     4, {1, 1, 1, 1}},
    {PW_MPLS_EXP_BITS_MODE,     INTEGER,     RWRITE,   pwMplsEntry,
     4, {1, 1, 1, 2}},
    {PW_MPLS_EXP_BITS,          INTEGER,     RWRITE,   pwMplsEntry,
     4, {1, 1, 1, 3}},
    {PW_MPLS_TTL,               INTEGER,     RWRITE,   pwMplsEntry,
     4, {1, 1, 1, 4}},
    {PW_MPLS_LCL_LDP_ID,        STRING,      RWRITE,   pwMplsEntry,
     4, {1, 1, 1, 5}},
    {PW_MPLS_LCL_LDP_ENTTY_IX,  INTEGER,     RWRITE,   pwMplsEntry,
     4, {1, 1, 1, 6}},
    {PW_MPLS_PEER_LDP_ID,       STRING,      RONLY,    pwMplsEntry,
     4, {1, 1, 1, 7}},
    {PW_MPLS_ST_TYPE,           INTEGER,     RWRITE,   pwMplsEntry,
     4, {1, 1, 1, 8}},

    /* PW MPLS Outbound Tunnel Table */
    {PW_MPLS_OUTBD_LSR_XC_IX,     INTEGER,     RWRITE, pwMplsOutboundEntry,
     4, {1, 2, 1, 1}},
    {PW_MPLS_OUTBD_TNL_IX,        INTEGER,     RWRITE, pwMplsOutboundEntry,
     4, {1, 2, 1, 2}},
    {PW_MPLS_OUTBD_TNL_INSTNCE,   INTEGER,     RONLY,  pwMplsOutboundEntry,
     4, {1, 2, 1, 3}},
    {PW_MPLS_OUTBD_TNL_LCL_LSR,   STRING,      RWRITE, pwMplsOutboundEntry,
     4, {1, 2, 1, 4}},
    {PW_MPLS_OUTBD_TNL_PEER_LSR,  STRING,      RWRITE, pwMplsOutboundEntry,
     4, {1, 2, 1, 5}},
    {PW_MPLS_OUTBD_IF_IX,         INTEGER,     RWRITE, pwMplsOutboundEntry,
     4, {1, 2, 1, 6}},
    {PW_MPLS_OUTBD_TNL_TYP_INUSE, INTEGER,     RONLY,  pwMplsOutboundEntry,
     4, {1, 2, 1, 7}},

    /* PW MPLS inbound table */
    {PW_MPLS_INBD_XC_IX,          INTEGER,     RONLY,  pwMplsInboundEntry,
     4, {1, 3, 1, 1}},

    /* PW to Non-TE mapping Table */
    {PW_MPLS_NON_TE_MAP_DN,       INTEGER,  NOACCESS, pwMplsNonTeMappingEntry,
     4, {1, 4, 1, 1}},
    {PW_MPLS_NON_TE_MAP_XC_IX,    INTEGER,  NOACCESS, pwMplsNonTeMappingEntry,
     4, {1, 4, 1, 2}},
    {PW_MPLS_NON_TE_MAP_IF_IX,    INTEGER,  NOACCESS, pwMplsNonTeMappingEntry,
     4, {1, 4, 1, 3}},
    {PW_MPLS_NON_TE_MAP_PW_IX,    INTEGER,  RONLY,    pwMplsNonTeMappingEntry,
     4, {1, 4, 1, 4}},

    /* PW to TE MPLS tunnels mapping Table */
    {PW_MPLS_TE_MAP_TNL_IX,          INTEGER, NOACCESS, pwMplsTeMappingEntry,
     4, {1, 5, 1, 1}},
    {PW_MPLS_TE_MAP_TNL_INSTNCE,     INTEGER, NOACCESS, pwMplsTeMappingEntry,
     4, {1, 5, 1, 2}},
    {PW_MPLS_TE_MAP_TNL_PEER_LSR_ID, STRING,  NOACCESS, pwMplsTeMappingEntry,
     4, {1, 5, 1, 3}},
    {PW_MPLS_TE_MAP_TNL_LCL_LSR_ID,  STRING,  NOACCESS, pwMplsTeMappingEntry,
     4, {1, 5, 1, 4}},
    {PW_MPLS_TE_MAP_PW_IX,           INTEGER, RONLY,    pwMplsTeMappingEntry,
     4, {1, 5, 1, 5}}
  };

struct nsm_pw_mib_table_index nsm_pw_mib_table_index_def[] =
  {
    /* 0 -- dummy. */
    {  0,   0, { NSM_PW_SNMP_INDEX_NONE } },
    /* PW Virtual Connection Table. */
    {  1,   4, { NSM_PW_SNMP_INDEX_INT32,  NSM_PW_SNMP_INDEX_NONE } },
    /* PW Performance Current Table. */
    {  1,   4, { NSM_PW_SNMP_INDEX_INT32,  NSM_PW_SNMP_INDEX_NONE } },
    /* PW Performance Interval Table. */
    {  2,   8, { NSM_PW_SNMP_INDEX_INT32, NSM_PW_SNMP_INDEX_INT32,
                 NSM_PW_SNMP_INDEX_NONE } },
    /* PW Performance 1 Day Interval Table. */
    {  2,   8, { NSM_PW_SNMP_INDEX_INT32, NSM_PW_SNMP_INDEX_INT32,
                 NSM_PW_SNMP_INDEX_NONE } },
    /* The PW ID mapping table */
    {  7,   28, { NSM_PW_SNMP_INDEX_INT32,  NSM_PW_SNMP_INDEX_INT32,
                  NSM_PW_SNMP_INDEX_INT32,  NSM_PW_SNMP_INDEX_INADDR,
                  NSM_PW_SNMP_INDEX_NONE } },
    /* The peer mapping table */
    {  7,   28, { NSM_PW_SNMP_INDEX_INT32,  NSM_PW_SNMP_INDEX_INADDR,
                  NSM_PW_SNMP_INDEX_INT32,  NSM_PW_SNMP_INDEX_INT32,
                  NSM_PW_SNMP_INDEX_NONE } },
#ifdef HAVE_FEC129
    /* PW Gen FEC Index Mapping table*/
    { 6 , 24, { NSM_PW_SNMP_INDEX_INT32,  NSM_PW_SNMP_INDEX_INT32,
                NSM_PW_SNMP_INDEX_INT32,  NSM_PW_SNMP_INDEX_INT32,
                NSM_PW_SNMP_INDEX_INT32,  NSM_PW_SNMP_INDEX_INT32,
                NSM_PW_SNMP_INDEX_NONE } },
#endif /*HAVE_FEC129 */

    /* The Pseudo wire Enet Table */
    {  2,   8,  { NSM_PW_SNMP_INDEX_INT32, NSM_PW_SNMP_INDEX_INT32,
                  NSM_PW_SNMP_INDEX_NONE } },
    /* The Pseudo wire Enet Statistics Table */
    {  1,   4,  { NSM_PW_SNMP_INDEX_INT32, NSM_PW_SNMP_INDEX_NONE } },    
    /* PW MPLS Table. */
    {  1,   4, { NSM_PW_SNMP_INDEX_INT32,  NSM_PW_SNMP_INDEX_NONE } },
    /* PW MPLS Outbound Tunnel Table */
    {  1,   4, { NSM_PW_SNMP_INDEX_INT32,  NSM_PW_SNMP_INDEX_NONE } },
    /* PW MPLS inbound table */
    {  1,   4, { NSM_PW_SNMP_INDEX_INT32,  NSM_PW_SNMP_INDEX_NONE } },
    /* PW to Non-TE mapping Table */
    {  4,   16, { NSM_PW_SNMP_INDEX_INT32,  NSM_PW_SNMP_INDEX_INT32,
                  NSM_PW_SNMP_INDEX_INT32,  NSM_PW_SNMP_INDEX_INT32,
                  NSM_PW_SNMP_INDEX_NONE } },
    /* PW to TE MPLS tunnels mapping Table */
    {  11,  44, { NSM_PW_SNMP_INDEX_INT32,  NSM_PW_SNMP_INDEX_INT32,
                  NSM_PW_SNMP_INDEX_INADDR, NSM_PW_SNMP_INDEX_INADDR,
                  NSM_PW_SNMP_INDEX_INT32,  NSM_PW_SNMP_INDEX_NONE } },
  };

/* Get index array from oid. */
u_int32_t
nsm_pw_snmp_index_get (struct variable *v, oid *name, size_t *length,
                       struct nsm_pw_snmp_index *index, u_int32_t type,
                       u_int32_t exact)
{
  u_int32_t i = 0;
  u_int32_t offset = 0;

  index->len = *length - v->namelen;
  
  if (index->len == 0)
    return NSM_PW_SNMP_INDEX_GET_SUCCESS;
  
  if (exact)
    if (index->len != nsm_pw_mib_table_index_def[type].len)
      return NSM_PW_SNMP_INDEX_GET_ERROR;
  
  if (index->len > nsm_pw_mib_table_index_def[type].len)
    return NSM_PW_SNMP_INDEX_GET_ERROR;
  
  while (offset < index->len)
    {
      u_int32_t val = 0;
      u_int32_t vartype = nsm_pw_mib_table_index_def[type].vars[i];
      u_int32_t j;
      
      if (vartype == NSM_PW_SNMP_INDEX_NONE)
        break;
      
      switch (vartype)
        {
        case NSM_PW_SNMP_INDEX_INT8:
        case NSM_PW_SNMP_INDEX_INT16:
        case NSM_PW_SNMP_INDEX_INT32:
          index->val[i++] = name[v->namelen + offset++];
          break;
        case NSM_PW_SNMP_INDEX_INADDR:
          for (j = 1; j <= 4 && offset < index->len; j++)
            val += name[v->namelen + offset++] << ((4 - j) * 8);

          index->val[i++] = pal_hton32 (val);
          break;
        default:
          NSM_ASSERT (0);
        }
    }

  return NSM_PW_SNMP_INDEX_GET_SUCCESS;
}

/* Set index array to oid. */
u_int32_t
nsm_pw_snmp_index_set (struct variable *v, oid *name, size_t *length,
                         struct nsm_pw_snmp_index *index, u_int32_t type)
{
  u_int32_t i = 0;
  u_int32_t offset = 0;

  index->len = nsm_pw_mib_table_index_def[type].len;

  while (offset < index->len)
    {
      unsigned long val;
      u_int32_t vartype = nsm_pw_mib_table_index_def[type].vars[i];

      if (vartype == NSM_PW_SNMP_INDEX_NONE)
        break;

      switch (vartype)
        {
        case NSM_PW_SNMP_INDEX_INT8:
        case NSM_PW_SNMP_INDEX_INT16:
        case NSM_PW_SNMP_INDEX_INT32:
          name[v->namelen + offset++] = index->val[i++];
          break;
        case NSM_PW_SNMP_INDEX_INADDR:
          val = pal_ntoh32 (index->val[i++]);
          name[v->namelen + offset++] = (val >> 24) & 0xff;
          name[v->namelen + offset++] = (val >> 16) & 0xff;
          name[v->namelen + offset++] = (val >> 8) & 0xff;
          name[v->namelen + offset++] = val & 0xff;
          break;
        default:
          NSM_ASSERT (0);
        }
    }

  *length = v->namelen + index->len;

  return NSM_PW_SNMP_INDEX_SET_SUCCESS;
}

/* Write Methods */
static int
write_pwScalars (int action, u_char *var_val,
                 u_char var_val_type, size_t var_val_len,
                 u_char *statP, oid *name, size_t length,
                 struct variable *v, u_int32_t vr_id)
{
  long intval = 0;
  struct nsm_master *nm = nsm_master_lookup_by_id (nzg, 0);
  struct nsm_mpls *nmpls = NULL;

  if (!nm)
    return SNMP_ERR_GENERR;

  nmpls = nm->nmpls;

  if (!nmpls)
    return SNMP_ERR_GENERR;
      
  if (var_val_type == ASN_INTEGER) 
    {
      if (var_val_len != sizeof (long)) 
        return SNMP_ERR_WRONGLENGTH;

      pal_mem_cpy (&intval, var_val, 4);
    }
  else
    return SNMP_ERR_WRONGTYPE;

  switch (v->magic)
    {
    case PW_UP_DWN_NOTIF_EN:  
      if (intval == NSM_MPLS_SNMP_VC_UP_DN_NTFY_DIS)
        {
          if (nsm_mpls_set_pw_up_dn_notify (nmpls, NSM_MPLS_VC_UP_DN_NOTIFN_DIS)
              != NSM_API_SET_SUCCESS)
            return SNMP_ERR_GENERR;
        }
      else if (intval == NSM_MPLS_SNMP_VC_UP_DN_NTFY_ENA)    
        {
          if (nsm_mpls_set_pw_up_dn_notify (nmpls, NSM_MPLS_VC_UP_DN_NOTIFN_ENA)
              != NSM_API_SET_SUCCESS)
            return SNMP_ERR_GENERR;
        }
       else
        return SNMP_ERR_BADVALUE;   
      break;

    case PW_DEL_NOTIF_EN:
      if (intval == NSM_MPLS_SNMP_VC_DEL_NTFY_ENA)
        {
          if (nsm_mpls_set_pw_del_notify (nmpls, NSM_MPLS_VC_DEL_NOTIFN_ENA) 
              != NSM_API_SET_SUCCESS)
            return SNMP_ERR_GENERR;
        }
      else if (intval == NSM_MPLS_SNMP_VC_DEL_NTFY_DIS)    
        {
          if (nsm_mpls_set_pw_del_notify (nmpls, NSM_MPLS_VC_DEL_NOTIFN_DIS) 
              != NSM_API_SET_SUCCESS)
            return SNMP_ERR_GENERR;
        }
      else
        return SNMP_ERR_BADVALUE;   
      break;

    case PW_NOTIF_RATE:
      if (nsm_mpls_set_pw_notify_rate (nmpls, intval) != NSM_API_SET_SUCCESS)
        return SNMP_ERR_GENERR;
      break;

    default:
      return SNMP_ERR_GENERR;
    }
  return SNMP_ERR_NOERROR;
}

static int
write_pwVCEntry (int action, u_char *var_val,
                 u_char var_val_type, size_t var_val_len,
                 u_char *statP, oid *name, size_t length,
                 struct variable *v, u_int32_t vr_id)
{
  long intval = 0;
  u_int32_t uintval = 0;
  size_t val_len = 0;
  struct pal_in4_addr addr;
  int ret;

  NSM_PW_SNMP_UNION(pw_vc_table_index, index);
  struct nsm_master *nm = nsm_master_lookup_by_id (nzg, 0);

  if (!nm)
    return SNMP_ERR_GENERR;

  if (! nsm_pw_snmp_index_get (v, name, &length, 
                               &index.snmp,
                               PW_VC_TABLE, 1)) /* exact = 1 */
    return SNMP_ERR_GENERR;

  if (var_val_type == ASN_UNSIGNED)
    pal_mem_cpy (&uintval, var_val, 4);

  if (var_val_type == ASN_INTEGER)
    pal_mem_cpy (&intval, var_val, 4);

  if (var_val_type == ASN_OCTET_STR)  /* OBJECT_ID also included */
    val_len = pal_strlen (var_val);

  switch (v->magic)
    {
    case PW_PEER_ADDR:
    case PW_NAME:
    case PW_DESCR:
      if (var_val_type != ASN_OCTET_STR)
        return SNMP_ERR_WRONGTYPE;
      break;

    case PW_GRP_ATTCHMT_ID:
    case PW_LOCAL_ATTCHMT_ID:
    case PW_PEER_ATTCHMT_ID:
      if (var_val_type != ASN_OCTET_STR)
        return SNMP_ERR_WRONGTYPE;

      if (pal_strncmp (var_val, "-", 1) == 0)
        return SNMP_ERR_BADVALUE; 

      uintval = cmd_str2int (var_val, &ret);
      if (ret < 0)
        return SNMP_ERR_BADVALUE; 
      break;
    }

  switch (v->magic)
    {
    case PW_TYPE:
      if (! (nsm_mpls_set_pw_type (nm, index.table.pw_ix, intval)))
        return SNMP_ERR_GENERR;
      break;

    case PW_OWNER:
      if (! (nsm_mpls_set_pw_owner (nm, index.table.pw_ix, intval)))
        return SNMP_ERR_GENERR;
      break;

    case PW_PSN_TYPE:
      if (! (nsm_mpls_set_pw_psn_type (nm, index.table.pw_ix, intval)))
        return SNMP_ERR_GENERR;
      break;

    case PW_SETUP_PRTY:
      if (! (nsm_mpls_set_pw_setup_prty (nm, index.table.pw_ix, intval)))
        return SNMP_ERR_GENERR;
      break;

    case PW_HOLD_PRTY:
      if (! (nsm_mpls_set_pw_hold_prty (nm, index.table.pw_ix, intval)))
        return SNMP_ERR_GENERR;
      break;

    case PW_PEER_ADDR_TYPE:
      if (! (nsm_mpls_set_pw_peer_addr_type (nm, index.table.pw_ix, intval)))
        return SNMP_ERR_GENERR;
      break;

    case PW_PEER_ADDR:
      if (!pal_inet_pton(AF_INET, var_val, &addr))
        return SNMP_ERR_GENERR;
      if (! (nsm_mpls_set_pw_peer_addr (nm, index.table.pw_ix, &addr)))
        return SNMP_ERR_GENERR;
      break;

    case PW_ATTCHD_PW_IX:
      if (! (nsm_mpls_set_pw_attchd_pw_ix (nm, index.table.pw_ix, intval)))
        return SNMP_ERR_GENERR;
      break;

    case PW_IF_IX:
      if (! (nsm_mpls_set_pw_if_ix (nm, index.table.pw_ix, intval)))
        return SNMP_ERR_GENERR;
      break;

    case PW_LOCAL_GRP_ID:
      if (uintval <= 0)
        return SNMP_ERR_BADVALUE;

      if (! (nsm_mpls_set_pw_local_grp_id (nm, index.table.pw_ix, uintval)))
        return SNMP_ERR_GENERR;
      break;

    case PW_GRP_ATTCHMT_ID:
      if (! (nsm_mpls_set_pw_grp_attchmt_id (nm, index.table.pw_ix, uintval)))
        return SNMP_ERR_GENERR;
      break;

    case PW_LOCAL_ATTCHMT_ID:
      if (! (nsm_mpls_set_pw_local_attchmt_id (nm, index.table.pw_ix, uintval)))
        return SNMP_ERR_GENERR;
      break;

    case PW_PEER_ATTCHMT_ID:
      if (! (nsm_mpls_set_pw_peer_attchmt_id (nm, index.table.pw_ix, uintval)))
        return SNMP_ERR_GENERR;
      break;

    case PW_CW_PRFRNCE:
      if (! (nsm_mpls_set_pw_cw_prfrnce (nm, index.table.pw_ix, intval)))
        return SNMP_ERR_GENERR;
      break;

    case PW_LOCAL_IF_MTU:
      if (! (nsm_mpls_set_pw_local_if_mtu (nm, index.table.pw_ix, intval)))
        return SNMP_ERR_GENERR;
      break;

    case PW_LOCAL_IF_STRING:
      if (! (nsm_mpls_set_pw_local_if_string (nm, index.table.pw_ix, intval)))
        return SNMP_ERR_GENERR;
      break;

    case PW_LOCAL_CAPAB_ADVRT:
      if (! (nsm_mpls_set_pw_local_capab_advrt (nm, index.table.pw_ix, var_val)))
        return SNMP_ERR_GENERR;
      break;

    case PW_FRGMT_CFG_SIZE:
      if (! (nsm_mpls_set_pw_frgmt_cfg_size (nm, index.table.pw_ix, intval)))
        return SNMP_ERR_GENERR;
      break;

    case PW_FCS_RETENTN_CFG:
      if (! (nsm_mpls_set_pw_fcs_retentn_cfg (nm, index.table.pw_ix, intval)))
        return SNMP_ERR_GENERR;
      break;

    case PW_OUTBD_LABEL:
      if (! (nsm_mpls_set_pw_outbd_label (nm, index.table.pw_ix, intval)))
        return SNMP_ERR_GENERR;
      break;

    case PW_INBD_LABEL:
      if (! (nsm_mpls_set_pw_inbd_label (nm, index.table.pw_ix, intval)))
        return SNMP_ERR_GENERR;
      break;

    case PW_NAME:
      if (! (nsm_mpls_set_pw_name (nm, index.table.pw_ix, var_val)))
        return SNMP_ERR_GENERR;
      break;

    case PW_DESCR:
      if (! (nsm_mpls_set_pw_descr (nm, index.table.pw_ix, var_val)))
        return SNMP_ERR_GENERR;
      break;

    case PW_ADMIN_STATUS:
      if (! (nsm_mpls_set_pw_admin_status (nm, index.table.pw_ix, intval)))
        return SNMP_ERR_GENERR;
      break;

    case PW_ROW_STATUS:
      if (! (nsm_mpls_set_pw_row_status (nm, index.table.pw_ix, intval)))
        return SNMP_ERR_GENERR;
      break;

    case PW_ST_TYPE:
      if (! (nsm_mpls_set_pw_st_type (nm, index.table.pw_ix, intval)))
        return SNMP_ERR_GENERR;
      break;

    case PW_ID:
      if ((uintval < NSM_MPLS_L2_VC_MIN) || (uintval > NSM_MPLS_L2_VC_MAX))
        return SNMP_ERR_BADVALUE;

      if (!(nsm_mpls_set_pw_id(nm, index.table.pw_ix, uintval)))
        return SNMP_ERR_GENERR;
      break;

    case PW_OAM_EN:
      if (!(nsm_mpls_set_pw_oam_enable (nm, index.table.pw_ix, intval)))
        return SNMP_ERR_GENERR;
      break;   
#ifdef HAVE_FEC129
    case PW_GEN_AGI_TYPE: 
      if (!(nsm_mpls_set_pw_gen_agi_type (nm, index.table.pw_ix, intval)))
        return SNMP_ERR_GENERR;
      break;

    case PW_GEN_LOC_AGII_TYPE:
      if (!(nsm_mpls_set_pw_gen_loc_aii_type (nm, index.table.pw_ix, intval)))
        return SNMP_ERR_GENERR;
      break; 

    case PW_GEN_REM_AGII_TYPE:  
      if (!(nsm_mpls_set_pw_gen_rem_aii_type (nm, index.table.pw_ix, intval)))
        return SNMP_ERR_GENERR;
      break;
#endif /*HAVE_FEC129*/
    default:
      return SNMP_ERR_GENERR;
    }
  return SNMP_ERR_NOERROR;
}

static int
write_pwEnetEntry (int action, u_char *var_val,
                 u_char var_val_type, size_t var_val_len,
                 u_char *statP, oid *name, size_t length,
                 struct variable *v, u_int32_t vr_id)
{
  long intval = 0;

  NSM_PW_SNMP_UNION(pw_enet_table_index, index);
  struct nsm_master *nm = nsm_master_lookup_by_id (nzg, 0);

  if (!nm)
    return SNMP_ERR_GENERR;

  if (! nsm_pw_snmp_index_get (v, name, &length,
                               &index.snmp,
                               PW_ENET_TABLE, 1)) /* exact = 1 */
    return SNMP_ERR_GENERR;

  if (var_val_type == ASN_INTEGER)
    pal_mem_cpy (&intval, var_val, 4);

  switch (v->magic)
    {
    case PW_ENET_PW_VLAN:
      /* Valid values 0|1..4094|4095.
       * Since 0 is not applicable for our implementation, only
       * 1 - 4095 are supported */
      if (intval < 1 || intval > NSM_PW_DEFAULT_VLAN)
        return SNMP_ERR_BADVALUE;

      if (! (nsm_mpls_set_pw_enet_pw_vlan (nm, index.table.pw_ix, 
                                           index.table.pw_instance, intval)))
        return SNMP_ERR_GENERR;
      break;

    case PW_ENET_VLAN_MODE:
      /* Only supported values for this OID in our implementation are
       * noChange and portBased */
      if (intval != PW_ENET_MODE_PORT_BASED && intval != PW_ENET_MODE_NO_CHANGE)
        return SNMP_ERR_BADVALUE;

      if (! (nsm_mpls_set_pw_enet_vlan_mode (nm, index.table.pw_ix, 
                                             index.table.pw_instance, intval)))
        return SNMP_ERR_GENERR;
      break;

    case PW_ENET_PORT_VLAN:
      /* Valid values 0|1..4094|4095.
       * Since 0 is not applicable for our implementation, only
       * 1 - 4095 are supported */
      if (intval < 1 || intval > NSM_PW_DEFAULT_VLAN)
        return SNMP_ERR_BADVALUE;

      if (! (nsm_mpls_set_pw_enet_port_vlan (nm, index.table.pw_ix, 
                                             index.table.pw_instance, intval)))
        return SNMP_ERR_GENERR;
      break;

    case PW_ENET_PORT_IF_INDEX:
      /* Interface index must be greater than 0 */
      if (intval <= 0)
        return SNMP_ERR_BADVALUE;

      if (! (nsm_mpls_set_pw_enet_port_if_index (nm, index.table.pw_ix, 
                                                 index.table.pw_instance, 
                                                 intval)))
        return SNMP_ERR_GENERR;
      break;

    case PW_ENET_PW_IF_INDEX:
      /* Only supported value is 0. Since we don't map the PW's as interfaces
       * in the ifTable */
      if (intval != 0)
        return SNMP_ERR_BADVALUE;

      if (! (nsm_mpls_set_pw_enet_pw_if_index (nm, index.table.pw_ix, 
                                               index.table.pw_instance, 
                                               intval)))
        return SNMP_ERR_GENERR;
      break;

    case PW_ENET_ROW_STATUS:
      if (! (nsm_mpls_set_pw_enet_row_status (nm, index.table.pw_ix, 
                                              index.table.pw_instance, intval)))
        return SNMP_ERR_GENERR;
      break;
    
    case PW_ENET_STORAGE_TYPE:
      /* We only support VOLATILE storageType through out pacos */
      if (intval != ST_VOLATILE)
        return SNMP_ERR_BADVALUE;

      if (! (nsm_mpls_set_pw_enet_storage_type (nm, index.table.pw_ix, 
                                                index.table.pw_instance, 
                                                intval)))
        return SNMP_ERR_GENERR;
      break;
   
   default:
      return SNMP_ERR_GENERR;
    }
   
  return SNMP_ERR_NOERROR;
}

static int
write_pwMplsEntry (int action, u_char *var_val,
                   u_char var_val_type, size_t var_val_len,
                   u_char *statP, oid *name, size_t length,
                   struct variable *v, u_int32_t vr_id)
{
  long intval = 0;
  size_t val_len = 0;
  struct ldp_id lcl_ldp_id;

  NSM_PW_SNMP_UNION(pw_mpls_table_index, index);
  struct nsm_master *nm = nsm_master_lookup_by_id (nzg, 0);

  if (!nm)
    return SNMP_ERR_GENERR;

  if (! nsm_pw_snmp_index_get (v, name, &length,
                               &index.snmp,
                               PW_MPLS_TABLE, 1)) /* exact = 1 */
    return SNMP_ERR_GENERR;

  if (var_val_type == ASN_INTEGER)
    pal_mem_cpy (&intval, var_val, 4);

  if (var_val_type == ASN_OCTET_STR)  /* OBJECT_ID also included */
    val_len = pal_strlen (var_val);

  switch (v->magic)
    {
    case PW_MPLS_MPLS_TYPE:
      if (! (nsm_mpls_set_pw_mpls_mpls_type (nm, index.table.pw_ix, var_val)))
        return SNMP_ERR_GENERR;
      break;

    case PW_MPLS_EXP_BITS_MODE:
      if (! (nsm_mpls_set_pw_mpls_exp_bits_mode (nm,
                                                 index.table.pw_ix, intval)))
        return SNMP_ERR_GENERR;
      break;

    case PW_MPLS_EXP_BITS:
      if (! (nsm_mpls_set_pw_mpls_exp_bits (nm, index.table.pw_ix, intval)))
        return SNMP_ERR_GENERR;
      break;

    case PW_MPLS_TTL:
      if (! (nsm_mpls_set_pw_mpls_ttl (nm, index.table.pw_ix, intval)))
        return SNMP_ERR_GENERR;
      break;

    case PW_MPLS_LCL_LDP_ID:
      if (!pal_inet_pton(AF_INET, var_val, &(lcl_ldp_id.lsr_id)))
        return SNMP_ERR_GENERR;
      if (! (nsm_mpls_set_pw_mpls_lcl_ldp_id (nm, index.table.pw_ix,
                                              &lcl_ldp_id)))
        return SNMP_ERR_GENERR;
      break;

    case PW_MPLS_LCL_LDP_ENTTY_IX:
      if (! (nsm_mpls_set_pw_mpls_lcl_ldp_entty_ix (nm, index.table.pw_ix,
                                                    intval)))
        return SNMP_ERR_GENERR;
      break;

    case PW_MPLS_ST_TYPE:
      if (! (nsm_mpls_set_pw_st_type (nm, index.table.pw_ix, intval)))
        return SNMP_ERR_GENERR;
      break;

    default:
      return SNMP_ERR_GENERR;
    }
  return SNMP_ERR_NOERROR;
}

static int
write_pwMplsOutboundEntry (int action, u_char *var_val,
                           u_char var_val_type, size_t var_val_len,
                           u_char *statP, oid *name, size_t length,
                           struct variable *v, u_int32_t vr_id)
{
  long intval = 0;
  size_t val_len = 0;
  struct pal_in4_addr addr;

  NSM_PW_SNMP_UNION(pw_mpls_outbd_tnl_table_index, index);
  struct nsm_master *nm = nsm_master_lookup_by_id (nzg, 0);

  if (!nm)
   return SNMP_ERR_GENERR;

  if (! nsm_pw_snmp_index_get (v, name, &length,
                               &index.snmp,
                               PW_MPLS_OUTBD_TNL_TABLE, 1)) /* exact = 1 */
    return SNMP_ERR_GENERR;

  if (var_val_type == ASN_INTEGER)
    pal_mem_cpy (&intval, var_val, 4);

  if (var_val_type == ASN_OCTET_STR)  /* OBJECT_ID also included */
    val_len = pal_strlen (var_val);

  switch (v->magic)
    {
    case PW_MPLS_OUTBD_LSR_XC_IX:
      if (! (nsm_mpls_set_pw_mpls_outbd_lsr_xc_ix (nm,
                                    index.table.pw_ix, intval)))
        return SNMP_ERR_GENERR;
      break;

    case PW_MPLS_OUTBD_TNL_IX:
      if (! (nsm_mpls_set_pw_mpls_outbd_tnl_ix (nm,
                                    index.table.pw_ix, intval)))
        return SNMP_ERR_GENERR;
      break;

    case PW_MPLS_OUTBD_TNL_LCL_LSR:
      if (!pal_inet_pton(AF_INET, var_val, &addr))
        return SNMP_ERR_GENERR;
      if (! (nsm_mpls_set_pw_mpls_outbd_tnl_lcl_lsr (nm, index.table.pw_ix,
                                                     &addr)))
        return SNMP_ERR_GENERR;
      break;

    case PW_MPLS_OUTBD_TNL_PEER_LSR:
      if (!pal_inet_pton(AF_INET, var_val, &addr))
        return SNMP_ERR_GENERR;
      if (! (nsm_mpls_set_pw_mpls_outbd_tnl_peer_lsr (nm, index.table.pw_ix,
                                                      &addr)))
        return SNMP_ERR_GENERR;
      break;

    case PW_MPLS_OUTBD_IF_IX:
      if (! (nsm_mpls_set_pw_mpls_outbd_if_ix (nm,
                                    index.table.pw_ix, intval)))
        return SNMP_ERR_GENERR;
      break;

    default:
      return SNMP_ERR_GENERR;
    }
  return SNMP_ERR_NOERROR;
}

/* SMUX callback functions. */

/* PW Scalars */
static u_char *
pwScalars (struct variable *v, oid *name, size_t *length, 
           int exact, size_t *var_len, WriteMethod **write_method, 
           u_int32_t vr_id)
{
  struct nsm_master *nm = nsm_master_lookup_by_id (nzg, 0);

  if (snmp_header_generic (v, name, length, exact, var_len, write_method)
      == MATCH_FAILED)
    return NULL;
  
  switch (v->magic)
    {
      case PW_IX_NXT:
        if (NSM_PW_SNMP_GET (pw_index_next, &nsm_pw_snmp_int_val))
          return (u_char *) &nsm_pw_snmp_int_val;
        break;

      case PW_PERF_TOT_ERR_PCKTS:
        if (NSM_PW_SNMP_GET (pw_tot_err_pckts, &nsm_pw_snmp_int_val))
          return (u_char *) &nsm_pw_snmp_int_val;
        break;

      case PW_UP_DWN_NOTIF_EN:
        *write_method = write_pwScalars; 
        if (NSM_PW_SNMP_GET (pw_up_dn_notify, &nsm_pw_snmp_int_val))
          return (u_char *) &nsm_pw_snmp_int_val;
        break;

      case PW_DEL_NOTIF_EN:
        *write_method = write_pwScalars;
        if (NSM_PW_SNMP_GET (pw_del_notify, &nsm_pw_snmp_int_val))
          return (u_char *) &nsm_pw_snmp_int_val;
        break;

      case PW_NOTIF_RATE:
        *write_method = write_pwScalars;
        if (NSM_PW_SNMP_GET (pw_notify_rate, &nsm_pw_snmp_int_val))
          return (u_char *) &nsm_pw_snmp_int_val;
        break;

      default:
        return NULL;
    }
  return NULL;
}

/* PW Virtual Connection Entry */
static u_char *
pwVCEntry (struct variable *v, oid *name, size_t *length, 
           u_int32_t exact, size_t *var_len,
           WriteMethod **write_method, u_int32_t vr_id)
{
  NSM_PW_SNMP_UNION(pw_vc_table_index, index);
  struct nsm_master *nm = nsm_master_lookup_by_id (nzg, 0);

  nsm_pw_snmp_int_val = NSM_PW_SNMP_VALUE_INVALID;
  pal_mem_set (&ip_addr, 0, sizeof(struct pal_in4_addr));
  pal_mem_set (nsm_pw_snmp_char_ptr, 0, NSM_PW_MAX_STRING_LENGTH);

  if (!nm)
    return NULL;

  /* Get table indices. */
  if (! nsm_pw_snmp_index_get (v, name, length, 
                               &index.snmp,
                               PW_VC_TABLE, exact))
    return NULL; 

  if (index.table.len == 0)
    index.table.pw_ix = 0;

  /* Return the current value of the variable. */
  switch (v->magic)
    {
    case PW_INDEX:
      if (! exact)
        {
          index.table.pw_ix =  0;
          nsm_pw_snmp_index_set (v, name, length,
                                 (struct nsm_pw_snmp_index *)&index,
                                 PW_VC_TABLE);
        }
      return NULL;

    case PW_TYPE:
      *write_method = write_pwVCEntry;
      if (NSM_PW_SNMP_GET_NEXT (pw_type, index.table.pw_ix,
                                &nsm_pw_snmp_int_val))
        NSM_PW_SNMP_RETURN (nsm_pw_snmp_int_val, PW_VC_TABLE);
      break;

    case PW_OWNER:
      *write_method = write_pwVCEntry;
      if (NSM_PW_SNMP_GET_NEXT (pw_owner, index.table.pw_ix,
                                &nsm_pw_snmp_int_val))
        NSM_PW_SNMP_RETURN (nsm_pw_snmp_int_val, PW_VC_TABLE);
      break;

    case PW_PSN_TYPE:
      *write_method = write_pwVCEntry;
      if (NSM_PW_SNMP_GET_NEXT (pw_psn_type, index.table.pw_ix,
                                &nsm_pw_snmp_int_val))
        NSM_PW_SNMP_RETURN (nsm_pw_snmp_int_val, PW_VC_TABLE);
      break;

    case PW_SETUP_PRTY:
      *write_method = write_pwVCEntry;
      if (NSM_PW_SNMP_GET_NEXT (pw_setup_prty, index.table.pw_ix,
                                &nsm_pw_snmp_int_val))
        NSM_PW_SNMP_RETURN (nsm_pw_snmp_int_val, PW_VC_TABLE);
      break;

    case PW_HOLD_PRTY:
      *write_method = write_pwVCEntry;
      if (NSM_PW_SNMP_GET_NEXT (pw_hold_prty, index.table.pw_ix,
                                &nsm_pw_snmp_int_val))
        NSM_PW_SNMP_RETURN (nsm_pw_snmp_int_val, PW_VC_TABLE);
      break;

    case PW_PEER_ADDR_TYPE:
      *write_method = write_pwVCEntry;
      if (NSM_PW_SNMP_GET_NEXT (pw_peer_addr_type, index.table.pw_ix,
                                &nsm_pw_snmp_int_val))
        NSM_PW_SNMP_RETURN (nsm_pw_snmp_int_val, PW_VC_TABLE);
      break;

    case PW_PEER_ADDR:
      *write_method = write_pwVCEntry;
      if (NSM_PW_SNMP_GET_NEXT (pw_peer_addr, index.table.pw_ix,
                                &nsm_pw_snmp_char_ptr))
        NSM_PW_SNMP_RETURN_CHAR (nsm_pw_snmp_char_ptr, PW_VC_TABLE);
      break;

    case PW_ATTCHD_PW_IX:
      *write_method = write_pwVCEntry;
      if (NSM_PW_SNMP_GET_NEXT (pw_attchd_pw_ix, index.table.pw_ix,
                                &nsm_pw_snmp_int_val))
        NSM_PW_SNMP_RETURN (nsm_pw_snmp_int_val, PW_VC_TABLE);
      break;

    case PW_IF_IX:
      *write_method = write_pwVCEntry;
      if (NSM_PW_SNMP_GET_NEXT (pw_if_ix, index.table.pw_ix,
                                &nsm_pw_snmp_int_val))
        NSM_PW_SNMP_RETURN (nsm_pw_snmp_int_val, PW_VC_TABLE);
      break;

    case PW_ID:
      *write_method = write_pwVCEntry;
      if (NSM_PW_SNMP_GET_NEXT (pw_id, index.table.pw_ix,
                                &nsm_pw_snmp_int_val))
        NSM_PW_SNMP_RETURN (nsm_pw_snmp_int_val, PW_VC_TABLE);
      break;

    case PW_LOCAL_GRP_ID:
      *write_method = write_pwVCEntry;
      if (NSM_PW_SNMP_GET_NEXT (pw_local_grp_id, index.table.pw_ix,
                                &nsm_pw_snmp_int_val))
        NSM_PW_SNMP_RETURN (nsm_pw_snmp_int_val, PW_VC_TABLE);
      break;

    case PW_GRP_ATTCHMT_ID:
      *write_method = write_pwVCEntry;
      if (NSM_PW_SNMP_GET_NEXT (pw_grp_attchmt_id, index.table.pw_ix,
                                &nsm_pw_snmp_char_ptr))
        NSM_PW_SNMP_RETURN_CHAR (nsm_pw_snmp_char_ptr, PW_VC_TABLE);
      break;

    case PW_LOCAL_ATTCHMT_ID:
      *write_method = write_pwVCEntry;
      if (NSM_PW_SNMP_GET_NEXT (pw_local_attchmt_id, index.table.pw_ix,
                                &nsm_pw_snmp_char_ptr))
        NSM_PW_SNMP_RETURN_CHAR (nsm_pw_snmp_char_ptr, PW_VC_TABLE);
      break;

    case PW_PEER_ATTCHMT_ID:
      *write_method = write_pwVCEntry;
      if (NSM_PW_SNMP_GET_NEXT (pw_peer_attchmt_id, index.table.pw_ix,
                                &nsm_pw_snmp_char_ptr))
        NSM_PW_SNMP_RETURN_CHAR (nsm_pw_snmp_char_ptr, PW_VC_TABLE);
      break;

    case PW_CW_PRFRNCE:
      *write_method = write_pwVCEntry;
      if (NSM_PW_SNMP_GET_NEXT (pw_cw_prfrnce, index.table.pw_ix,
                                &nsm_pw_snmp_int_val))
        NSM_PW_SNMP_RETURN (nsm_pw_snmp_int_val, PW_VC_TABLE);
      break;

    case PW_LOCAL_IF_MTU:
      *write_method = write_pwVCEntry;
      if (NSM_PW_SNMP_GET_NEXT (pw_local_if_mtu, index.table.pw_ix,
                                &nsm_pw_snmp_int_val))
        NSM_PW_SNMP_RETURN (nsm_pw_snmp_int_val, PW_VC_TABLE);
      break;

    case PW_LOCAL_IF_STRING:
      *write_method = write_pwVCEntry;
      if (NSM_PW_SNMP_GET_NEXT (pw_local_if_string, index.table.pw_ix,
                                &nsm_pw_snmp_int_val))
        NSM_PW_SNMP_RETURN (nsm_pw_snmp_int_val, PW_VC_TABLE);
      break;

    case PW_LOCAL_CAPAB_ADVRT:
      *write_method = write_pwVCEntry;
      if (NSM_PW_SNMP_GET_NEXT (pw_local_capab_advrt, index.table.pw_ix,
                                &nsm_pw_snmp_char_ptr))
        NSM_PW_SNMP_RETURN_BITS (nsm_pw_snmp_char_ptr, PW_VC_TABLE);
      break;

    case PW_REMOTE_GRP_ID:
      if (NSM_PW_SNMP_GET_NEXT (pw_remote_grp_id, index.table.pw_ix,
                                &nsm_pw_snmp_int_val))
        NSM_PW_SNMP_RETURN (nsm_pw_snmp_int_val, PW_VC_TABLE);
      break;

    case PW_CW_STATUS:
      if (NSM_PW_SNMP_GET_NEXT (pw_cw_status, index.table.pw_ix,
                                &nsm_pw_snmp_int_val))
        NSM_PW_SNMP_RETURN (nsm_pw_snmp_int_val, PW_VC_TABLE);
      break;

    case PW_REMOTE_IF_MTU:
      if (NSM_PW_SNMP_GET_NEXT (pw_remote_if_mtu, index.table.pw_ix,
                                &nsm_pw_snmp_int_val))
        NSM_PW_SNMP_RETURN (nsm_pw_snmp_int_val, PW_VC_TABLE);
      break;

    case PW_REMOTE_IF_STRING:
      if (NSM_PW_SNMP_GET_NEXT (pw_remote_if_string, index.table.pw_ix,
                                &nsm_pw_snmp_int_val))
        NSM_PW_SNMP_RETURN (nsm_pw_snmp_int_val, PW_VC_TABLE);
      break;

    case PW_REMOTE_CAPAB:
      if (NSM_PW_SNMP_GET_NEXT (pw_remote_capab, index.table.pw_ix,
                                &nsm_pw_snmp_char_ptr))
        NSM_PW_SNMP_RETURN_BITS (nsm_pw_snmp_char_ptr, PW_VC_TABLE);
      break;

    case PW_FRGMT_CFG_SIZE:
      *write_method = write_pwVCEntry;
      if (NSM_PW_SNMP_GET_NEXT (pw_frgmt_cfg_size, index.table.pw_ix,
                                &nsm_pw_snmp_int_val))
        NSM_PW_SNMP_RETURN (nsm_pw_snmp_int_val, PW_VC_TABLE);
      break;

    case PW_RMT_FRGMT_CAPAB:
      if (NSM_PW_SNMP_GET_NEXT (pw_rmt_frgmt_capab, index.table.pw_ix,
                                &nsm_pw_snmp_char_ptr))
        NSM_PW_SNMP_RETURN_BITS (nsm_pw_snmp_char_ptr, PW_VC_TABLE);
      break;

    case PW_FCS_RETENTN_CFG:
      *write_method = write_pwVCEntry;
      if (NSM_PW_SNMP_GET_NEXT (pw_fcs_retentn_cfg, index.table.pw_ix,
                                &nsm_pw_snmp_int_val))
        NSM_PW_SNMP_RETURN (nsm_pw_snmp_int_val, PW_VC_TABLE);
      break;

    case PW_FCS_RETENTN_STATUS:
      if (NSM_PW_SNMP_GET_NEXT (pw_fcs_retentn_status, index.table.pw_ix,
                                &nsm_pw_snmp_char_ptr))
        NSM_PW_SNMP_RETURN_BITS (nsm_pw_snmp_char_ptr, PW_VC_TABLE);
      break;

    case PW_OUTBD_LABEL:
      *write_method = write_pwVCEntry;
      if (NSM_PW_SNMP_GET_NEXT (pw_outbd_label, index.table.pw_ix,
                                &nsm_pw_snmp_int_val))
        NSM_PW_SNMP_RETURN (nsm_pw_snmp_int_val, PW_VC_TABLE);
      break;

    case PW_INBD_LABEL:
      *write_method = write_pwVCEntry;
      if (NSM_PW_SNMP_GET_NEXT (pw_inbd_label, index.table.pw_ix,
                                &nsm_pw_snmp_int_val))
        NSM_PW_SNMP_RETURN (nsm_pw_snmp_int_val, PW_VC_TABLE);
      break;

    case PW_NAME:
      *write_method = write_pwVCEntry;
      if (NSM_PW_SNMP_GET_NEXT (pw_name, index.table.pw_ix,
                                &nsm_pw_snmp_char_ptr))
      NSM_PW_SNMP_RETURN_CHAR (nsm_pw_snmp_char_ptr, PW_VC_TABLE);
      break;

    case PW_DESCR:
      *write_method = write_pwVCEntry;
      if (NSM_PW_SNMP_GET_NEXT (pw_descr, index.table.pw_ix,
                                &nsm_pw_snmp_char_ptr))
      NSM_PW_SNMP_RETURN_CHAR (nsm_pw_snmp_char_ptr, PW_VC_TABLE);
      break;

    case PW_CREATE_TIME:
      if (NSM_PW_SNMP_GET_NEXT (pw_create_time, index.table.pw_ix,
                                &nsm_pw_snmp_int_val))
        NSM_PW_SNMP_RETURN (nsm_pw_snmp_int_val, PW_VC_TABLE);
      break;

    case PW_UP_TIME:
      if (NSM_PW_SNMP_GET_NEXT (pw_up_time, index.table.pw_ix,
                                &nsm_pw_snmp_int_val))
        NSM_PW_SNMP_RETURN (nsm_pw_snmp_int_val, PW_VC_TABLE);
      break;

    case PW_LAST_CHANGE:
      if (NSM_PW_SNMP_GET_NEXT (pw_last_change, index.table.pw_ix,
                                &nsm_pw_snmp_int_val))
        NSM_PW_SNMP_RETURN (nsm_pw_snmp_int_val, PW_VC_TABLE);
      break;

    case PW_ADMIN_STATUS:
      *write_method = write_pwVCEntry;
      if (NSM_PW_SNMP_GET_NEXT (pw_admin_status, index.table.pw_ix,
                                &nsm_pw_snmp_int_val))
        NSM_PW_SNMP_RETURN (nsm_pw_snmp_int_val, PW_VC_TABLE);
      break;

    case PW_OPER_STATUS:
      if (NSM_PW_SNMP_GET_NEXT (pw_oper_status, index.table.pw_ix,
                                &nsm_pw_snmp_int_val))
        NSM_PW_SNMP_RETURN (nsm_pw_snmp_int_val, PW_VC_TABLE);
      break;

    case PW_LOCAL_STATUS:
      if (NSM_PW_SNMP_GET_NEXT (pw_local_status, index.table.pw_ix,
                                &nsm_pw_snmp_char_ptr))
        NSM_PW_SNMP_RETURN_BITS (nsm_pw_snmp_char_ptr, PW_VC_TABLE);
      break;

    case PW_RMT_STATUS_CAPAB:
      if (NSM_PW_SNMP_GET_NEXT (pw_rmt_status_capab, index.table.pw_ix,
                                &nsm_pw_snmp_int_val))
        NSM_PW_SNMP_RETURN (nsm_pw_snmp_int_val, PW_VC_TABLE);
      break;

    case PW_RMT_STATUS:
      if (NSM_PW_SNMP_GET_NEXT (pw_rmt_status, index.table.pw_ix,
                                &nsm_pw_snmp_char_ptr))
        NSM_PW_SNMP_RETURN_BITS (nsm_pw_snmp_char_ptr, PW_VC_TABLE);
      break;

    case PW_TIME_ELAPSED:
      if (NSM_PW_SNMP_GET_NEXT (pw_time_elapsed, index.table.pw_ix,
                                &nsm_pw_snmp_int_val))
        NSM_PW_SNMP_RETURN (nsm_pw_snmp_int_val, PW_VC_TABLE);
      break;

    case PW_VALID_INTRVL:
      if (NSM_PW_SNMP_GET_NEXT (pw_valid_intrvl, index.table.pw_ix,
                                &nsm_pw_snmp_int_hc_val))
        NSM_PW_SNMP_RETURN_INTEGER64 (nsm_pw_snmp_int_hc_val, PW_VC_TABLE);
      break;

    case PW_ROW_STATUS:
      *write_method = write_pwVCEntry;
      if (NSM_PW_SNMP_GET_NEXT (pw_row_status, index.table.pw_ix,
                                &nsm_pw_snmp_int_val))
        NSM_PW_SNMP_RETURN (nsm_pw_snmp_int_val, PW_VC_TABLE);
      break;

    case PW_ST_TYPE:
      *write_method = write_pwVCEntry;
      if (NSM_PW_SNMP_GET_NEXT (pw_st_type, index.table.pw_ix,
                                &nsm_pw_snmp_int_val))
        NSM_PW_SNMP_RETURN (nsm_pw_snmp_int_val, PW_VC_TABLE);
      break;

    case PW_OAM_EN:
      *write_method = write_pwVCEntry;
      if (NSM_PW_SNMP_GET_NEXT (pw_oam_en, index.table.pw_ix,
                                &nsm_pw_snmp_int_val))
        NSM_PW_SNMP_RETURN (nsm_pw_snmp_int_val, PW_VC_TABLE);
      break;
#ifdef HAVE_FEC129
    case PW_GEN_AGI_TYPE:
      *write_method = write_pwVCEntry;
      if (NSM_PW_SNMP_GET_NEXT (pw_gen_agi_type, index.table.pw_ix,
                                &nsm_pw_snmp_int_val))
        NSM_PW_SNMP_RETURN (nsm_pw_snmp_int_val, PW_VC_TABLE);
      break;

    case PW_GEN_LOC_AGII_TYPE:
      *write_method = write_pwVCEntry;
      if (NSM_PW_SNMP_GET_NEXT (pw_gen_loc_aii_type, index.table.pw_ix,
                                &nsm_pw_snmp_int_val))
        NSM_PW_SNMP_RETURN (nsm_pw_snmp_int_val, PW_VC_TABLE);
      break;

    case PW_GEN_REM_AGII_TYPE:
      *write_method = write_pwVCEntry;
      if (NSM_PW_SNMP_GET_NEXT (pw_gen_rem_aii_type, index.table.pw_ix,
                                &nsm_pw_snmp_int_val))
        NSM_PW_SNMP_RETURN (nsm_pw_snmp_int_val, PW_VC_TABLE);
      break;
#endif /*HAVE_FEC129 */
    default:
      return NULL;
    }
  return NULL;
}

/* PW Performance Current Entry */
static u_char *
pwPerfCurrentEntry (struct variable *v, oid *name, size_t *length, 
                    u_int32_t exact, size_t *var_len,
                    WriteMethod **write_method, u_int32_t vr_id)
{
  NSM_PW_SNMP_UNION(pw_perf_crnt_table_index, index);
  struct nsm_master *nm = nsm_master_lookup_by_id (nzg, 0);

  nsm_pw_snmp_int_val = NSM_PW_SNMP_VALUE_INVALID;
  pal_mem_set (&nsm_pw_snmp_int_hc_val, 0, sizeof (ut_int64_t));

  if (!nm)
    return NULL;

  /* Get table indices. */
  if (! nsm_pw_snmp_index_get (v, name, length, 
                               &index.snmp,
                               PW_PERF_CRNT_TABLE, exact))
    return NULL; 

  if (index.table.len == 0)
    index.table.pw_ix = 0;

  /* Return the current value of the variable. */
  switch (v->magic)
    {
    case PW_PERF_CRNT_IN_HC_PCKTS:
      if (NSM_PW_SNMP_GET_NEXT (pw_crnt_in_hc_pckts, index.table.pw_ix,
                                &nsm_pw_snmp_int_hc_val))
        NSM_PW_SNMP_RETURN (nsm_pw_snmp_int_hc_val, PW_PERF_CRNT_TABLE);
      break;

    case PW_PERF_CRNT_IN_HC_BYTES:
      if (NSM_PW_SNMP_GET_NEXT (pw_crnt_in_hc_bytes, index.table.pw_ix,
                                &nsm_pw_snmp_int_hc_val))
        NSM_PW_SNMP_RETURN (nsm_pw_snmp_int_hc_val, PW_PERF_CRNT_TABLE);
      break;

    case PW_PERF_CRNT_OUT_HC_PCKTS:
      if (NSM_PW_SNMP_GET_NEXT (pw_crnt_out_hc_pckts, index.table.pw_ix,
                                &nsm_pw_snmp_int_hc_val))
        NSM_PW_SNMP_RETURN (nsm_pw_snmp_int_hc_val, PW_PERF_CRNT_TABLE);
      break;

    case PW_PERF_CRNT_OUT_HC_BYTES:
      if (NSM_PW_SNMP_GET_NEXT (pw_crnt_out_hc_bytes, index.table.pw_ix,
                                &nsm_pw_snmp_int_hc_val))
        NSM_PW_SNMP_RETURN (nsm_pw_snmp_int_hc_val, PW_PERF_CRNT_TABLE);
      break;

    case PW_PERF_CRNT_IN_PCKTS:
      if (NSM_PW_SNMP_GET_NEXT (pw_crnt_in_pckts, index.table.pw_ix,
                                &nsm_pw_snmp_int_val))
        NSM_PW_SNMP_RETURN (nsm_pw_snmp_int_val, PW_PERF_CRNT_TABLE);
      break;

    case PW_PERF_CRNT_IN_BYTES:
      if (NSM_PW_SNMP_GET_NEXT (pw_crnt_in_bytes, index.table.pw_ix,
                                &nsm_pw_snmp_int_val))
        NSM_PW_SNMP_RETURN (nsm_pw_snmp_int_val, PW_PERF_CRNT_TABLE);
      break;

    case PW_PERF_CRNT_OUT_PCKTS:
      if (NSM_PW_SNMP_GET_NEXT (pw_crnt_out_pckts, index.table.pw_ix,
                                &nsm_pw_snmp_int_val))
        NSM_PW_SNMP_RETURN (nsm_pw_snmp_int_val, PW_PERF_CRNT_TABLE);
      break;

    case PW_PERF_CRNT_OUT_BYTES:
      if (NSM_PW_SNMP_GET_NEXT (pw_crnt_out_bytes, index.table.pw_ix,
                                &nsm_pw_snmp_int_val))
        NSM_PW_SNMP_RETURN (nsm_pw_snmp_int_val, PW_PERF_CRNT_TABLE);
      break;

    default:
      return NULL;
    }  
  return NULL;
}

/* PW Performance Interval Entry */
static u_char *
pwPerfIntervalEntry (struct variable *v, oid *name, size_t *length, 
                     u_int32_t exact, size_t *var_len,
                     WriteMethod **write_method, u_int32_t vr_id)
{
  NSM_PW_SNMP_UNION(pw_perf_intrvl_table_index, index);
  struct nsm_master *nm = nsm_master_lookup_by_id (nzg, 0);
  
  nsm_pw_snmp_int_val = NSM_PW_SNMP_VALUE_INVALID;
  pal_mem_set (&nsm_pw_snmp_int_hc_val, 0, sizeof (ut_int64_t));
  nsm_pw_snmp_char_ptr[0] = '\0';
  pal_mem_set (&index, 0, sizeof (struct pw_perf_intrvl_table_index));

  if (!nm)
    return NULL;

  /* Get table indices. */
  if (! nsm_pw_snmp_index_get (v, name, length, 
                               &index.snmp,
                               PW_PERF_INTRVL_TABLE, exact))
    return NULL; 

  if (index.table.len == 0)
    {
      /* Index of Table is pw index and perf interval no. */
      index.table.pw_ix =  0;
      index.table.pw_perf_intrvl_no_ix = 0;
    }
    
  switch (v->magic)
    { 
    case PW_PERF_INTRVL_NO:
      if (! exact)
        {
          index.table.pw_ix =  0;
          index.table.pw_perf_intrvl_no_ix = 0;
          nsm_pw_snmp_index_set (v, name, length,
                                 (struct nsm_pw_snmp_index *)&index, 
                                 PW_PERF_INTRVL_TABLE);
        }
      return NULL;

    case PW_PERF_INTRVL_VALID_DATA:
      if (NSM_PW_SNMP_GET2NEXT (pw_intrvl_valid_dt, index.table.pw_ix,
                                index.table.pw_perf_intrvl_no_ix, 
                                &nsm_pw_snmp_int_val)) 
        NSM_PW_SNMP_RETURN (nsm_pw_snmp_int_val, PW_PERF_INTRVL_TABLE);
      break;

    case PW_PERF_INTRVL_TIME_ELPSD:
      if (NSM_PW_SNMP_GET2NEXT (pw_intrvl_time_elpsd, index.table.pw_ix,
                                index.table.pw_perf_intrvl_no_ix, 
                                &nsm_pw_snmp_int_val))
        NSM_PW_SNMP_RETURN (nsm_pw_snmp_int_val, PW_PERF_INTRVL_TABLE);
      break;

    case PW_PERF_INTRVL_IN_HC_PCKTS:
      if (NSM_PW_SNMP_GET2NEXT (pw_intrvl_in_hc_pckts, index.table.pw_ix,
                                index.table.pw_perf_intrvl_no_ix, 
                                &nsm_pw_snmp_int_hc_val))
        NSM_PW_SNMP_RETURN (nsm_pw_snmp_int_hc_val, PW_PERF_INTRVL_TABLE);
      break;

    case PW_PERF_INTRVL_IN_HC_BYTES:
      if (NSM_PW_SNMP_GET2NEXT (pw_intrvl_in_hc_bytes, index.table.pw_ix,
                                index.table.pw_perf_intrvl_no_ix, 
                                &nsm_pw_snmp_int_hc_val))
        NSM_PW_SNMP_RETURN (nsm_pw_snmp_int_hc_val, PW_PERF_INTRVL_TABLE);
      break;
 
    case PW_PERF_INTRVL_OUT_HC_PCKTS:
      if (NSM_PW_SNMP_GET2NEXT (pw_intrvl_out_hc_pckts, index.table.pw_ix,
                                index.table.pw_perf_intrvl_no_ix, 
                                &nsm_pw_snmp_int_hc_val))
        NSM_PW_SNMP_RETURN (nsm_pw_snmp_int_hc_val, PW_PERF_INTRVL_TABLE);
      break;
      
    case PW_PERF_INTRVL_OUT_HC_BYTES:
      if (NSM_PW_SNMP_GET2NEXT (pw_intrvl_out_hc_bytes, index.table.pw_ix,
                                index.table.pw_perf_intrvl_no_ix, 
                                &nsm_pw_snmp_int_hc_val))
        NSM_PW_SNMP_RETURN (nsm_pw_snmp_int_hc_val, PW_PERF_INTRVL_TABLE);
      break;

    case PW_PERF_INTRVL_IN_PCKTS:
      if (NSM_PW_SNMP_GET2NEXT (pw_intrvl_in_pckts, index.table.pw_ix,
                                index.table.pw_perf_intrvl_no_ix, 
                                &nsm_pw_snmp_int_val))
        NSM_PW_SNMP_RETURN (nsm_pw_snmp_int_val, PW_PERF_INTRVL_TABLE);
      break;

    case PW_PERF_INTRVL_IN_BYTES:
      if (NSM_PW_SNMP_GET2NEXT (pw_intrvl_in_bytes, index.table.pw_ix,
                                index.table.pw_perf_intrvl_no_ix, 
                                &nsm_pw_snmp_int_val))
        NSM_PW_SNMP_RETURN (nsm_pw_snmp_int_val, PW_PERF_INTRVL_TABLE);
      break;

    case PW_PERF_INTRVL_OUT_PCKTS:
      if (NSM_PW_SNMP_GET2NEXT (pw_intrvl_out_pckts, index.table.pw_ix,
                                index.table.pw_perf_intrvl_no_ix,
                                &nsm_pw_snmp_int_val))
        NSM_PW_SNMP_RETURN (nsm_pw_snmp_int_val, PW_PERF_INTRVL_TABLE);
      break;

    case PW_PERF_INTRVL_OUT_BYTES:
      if (NSM_PW_SNMP_GET2NEXT (pw_intrvl_out_bytes, index.table.pw_ix,
                                index.table.pw_perf_intrvl_no_ix, 
                                &nsm_pw_snmp_int_val))
        NSM_PW_SNMP_RETURN (nsm_pw_snmp_int_val, PW_PERF_INTRVL_TABLE);
      break;

    default:
      return NULL;
 
    }
  return NULL;
}

/* PW Performance 1 Day Interval Entry */
static u_char *
pwPerf1DayIntervalEntry (struct variable *v, oid *name, size_t *length, 
                         u_int32_t exact, size_t *var_len,
                         WriteMethod **write_method, u_int32_t vr_id)
{
  NSM_PW_SNMP_UNION(pw_perf1dy_intrvl_table_index, index);
  struct nsm_master *nm = nsm_master_lookup_by_id (nzg, 0);

  nsm_pw_snmp_int_val = NSM_PW_SNMP_VALUE_INVALID;
  pal_mem_set (&nsm_pw_snmp_int_hc_val, 0, sizeof (ut_int64_t));

  if (!nm)
    return NULL;

  /* Get table indices. */
  if (! nsm_pw_snmp_index_get (v, name, length, 
                               &index.snmp,
                               PW_PERF1DY_INTRVL_TABLE, exact))
    return NULL; 

  if (index.table.len == 0)
    {
      index.table.pw_ix =  0;
      index.table.pw_perf1dy_intrvl_no_ix = 0;
    }

  switch (v->magic)
    {
    case PW_PERF_1DY_INTRVL_NO:
      if (! exact)
        {
          index.table.pw_ix =  0;
          index.table.pw_perf1dy_intrvl_no_ix = 0;

          nsm_pw_snmp_index_set (v, name, length,
                                 (struct nsm_pw_snmp_index *)&index,
                                 PW_PERF1DY_INTRVL_TABLE);
        }
      return NULL;

    case PW_PERF_1DY_INTRVL_VALID_DT:
      if (NSM_PW_SNMP_GET2NEXT (pw_1dy_valid_dt, index.table.pw_ix,
                                index.table.pw_perf1dy_intrvl_no_ix, 
                                &nsm_pw_snmp_int_val))
        NSM_PW_SNMP_RETURN (nsm_pw_snmp_int_val, PW_PERF1DY_INTRVL_TABLE);
      break;

    case PW_PERF_1DY_INTRVL_MONI_SECS:
      if (NSM_PW_SNMP_GET2NEXT (pw_1dy_moni_secs, index.table.pw_ix,
                                index.table.pw_perf1dy_intrvl_no_ix, 
                                &nsm_pw_snmp_int_val))
        NSM_PW_SNMP_RETURN (nsm_pw_snmp_int_val, PW_PERF1DY_INTRVL_TABLE);
      break;

    case PW_PERF_1DY_INTRVL_IN_HC_PCKTS:
      if (NSM_PW_SNMP_GET2NEXT (pw_1dy_in_hc_pckts, index.table.pw_ix,
                                index.table.pw_perf1dy_intrvl_no_ix, 
                                &nsm_pw_snmp_int_hc_val))
        NSM_PW_SNMP_RETURN (nsm_pw_snmp_int_hc_val, PW_PERF1DY_INTRVL_TABLE);
      break;

    case PW_PERF_1DY_INTRVL_IN_HC_BYTES:
      if (NSM_PW_SNMP_GET2NEXT (pw_1dy_in_hc_bytes, index.table.pw_ix,
                                index.table.pw_perf1dy_intrvl_no_ix, 
                                &nsm_pw_snmp_int_hc_val))
        NSM_PW_SNMP_RETURN (nsm_pw_snmp_int_hc_val, PW_PERF1DY_INTRVL_TABLE);
      break;

    case PW_PERF_1DY_INTRVL_OUT_HC_PCKTS:
      if (NSM_PW_SNMP_GET2NEXT (pw_1dy_out_hc_pckts, index.table.pw_ix,
                                index.table.pw_perf1dy_intrvl_no_ix, 
                                &nsm_pw_snmp_int_hc_val))
        NSM_PW_SNMP_RETURN (nsm_pw_snmp_int_hc_val, PW_PERF1DY_INTRVL_TABLE);
      break;

    case PW_PERF_1DY_INTRVL_OUT_HC_BYTES:
      if (NSM_PW_SNMP_GET2NEXT (pw_1dy_out_hc_bytes, index.table.pw_ix,
                                index.table.pw_perf1dy_intrvl_no_ix, 
                                &nsm_pw_snmp_int_hc_val))
        NSM_PW_SNMP_RETURN (nsm_pw_snmp_int_hc_val, PW_PERF1DY_INTRVL_TABLE);
      break;

    default:
      return NULL;
    }
  return NULL;
}

/* The PW ID mapping Entry */
static u_char *
pwIndexMappingEntry (struct variable *v, oid *name, size_t *length, 
                     u_int32_t exact, size_t *var_len,
                     WriteMethod **write_method, u_int32_t vr_id)
{
  NSM_PW_SNMP_UNION(pw_id_map_table_index, index);
  struct nsm_master *nm = nsm_master_lookup_by_id (nzg, 0);
  
  nsm_pw_snmp_int_val = NSM_PW_SNMP_VALUE_INVALID;
  nsm_pw_snmp_char_ptr[0] = '\0';
  pal_mem_set (&index, 0, sizeof (struct pw_id_map_table_index));

  if (!nm)
    return NULL;

  /* Get table indices. */
  if (! nsm_pw_snmp_index_get (v, name, length, 
                               &index.snmp,
                               PW_IX_MAP_TABLE, exact))
    return NULL; 

  if (index.table.len == 0)
    {
      index.table.pw_id_map_pw_type =  0;
      index.table.pw_id_map_pw_id =  0;
      index.table.pw_id_map_peer_addr_type =  0;
      pal_mem_set (&index.table.pw_id_map_peer_addr,
                   0, sizeof (struct pal_in4_addr));
    }  

  switch (v->magic)
    { 
    case PW_ID_MAP_PW_TYPE:
    case PW_ID_MAP_PW_ID:
    case PW_ID_MAP_PEER_ADDR_TYPE:
    case PW_ID_MAP_PEER_ADDR:

      if (! exact)
        {
          index.table.pw_id_map_pw_type =  0;
          index.table.pw_id_map_pw_id =  0;
          index.table.pw_id_map_peer_addr_type =  0;
          pal_mem_set (&index.table.pw_id_map_peer_addr,
                       0, sizeof (struct pal_in4_addr));

          nsm_pw_snmp_index_set (v, name, length,
                                 (struct nsm_pw_snmp_index *)&index, 
                                 PW_IX_MAP_TABLE);
        }
      return NULL;

    case PW_ID_MAP_PW_IX:
      /* Read Only Object */
      if (NSM_PW_SNMP_GET8NEXT (pw_id_map_pw_ix,
                                index.table.pw_id_map_pw_type,
                                index.table.pw_id_map_pw_id,
                                index.table.pw_id_map_peer_addr_type,
                                index.table.pw_id_map_peer_addr,
                                &nsm_pw_snmp_int_val)) 
        {
          nsm_pw_snmp_index_set (v, name, length,
                                 (struct nsm_pw_snmp_index *)&index,
                                 PW_IX_MAP_TABLE);
          NSM_PW_SNMP_RETURN (nsm_pw_snmp_int_val, PW_IX_MAP_TABLE);
        }
      break;

    default:
      return NULL;
 
    }
  return NULL;
}
#ifdef HAVE_FEC129
/* PW Generalised FEC Entry */
static u_char *
pwGenFecIndxMapEntry (struct variable *v, oid *name, size_t *length,
                      u_int32_t exact, size_t *var_len,
                      WriteMethod **write_method, u_int32_t vr_id)
{
  NSM_PW_SNMP_UNION (pw_gen_fec_map_table_index, index);
  struct nsm_master *nm = nsm_master_lookup_by_id (nzg, 0);

  nsm_pw_snmp_int_val = NSM_PW_SNMP_VALUE_INVALID;
  nsm_pw_snmp_char_ptr[0] = '\0';
  pal_mem_set (&index, 0, sizeof (struct pw_gen_fec_map_table_index));

  /* Get table indices. */
  if (! nsm_pw_snmp_index_get (v, name, length,
                               &index.snmp,
                               PW_GEN_FEC_MAP_TABLE, exact))
    return NULL;

  if (index.table.len == 0)
    {
      index.table.pw_gen_fec_agi_type =  0;
      index.table.pw_gen_fec_agi =  0;
      index.table.pw_gen_fec_loc_aii_type =  0;
      index.table.pw_gen_fec_loc_aii =  0;
      index.table.pw_gen_fec_rmte_aii_type =  0;
      index.table.pw_gen_fec_rmte_aii =  0;
    }
  switch (v->magic)
    {
      case PW_GEN_FEC_MAP_AGI_TYPE:
      case PW_GEN_FEC_MAP_AGI:
      case PW_GEN_FEC_LOC_AII_TYPE:
      case PW_GEN_FEC_LOC_AII:
      case PW_GEN_FEC_REM_AII_TYPE:
      case PW_GEN_FEC_REM_AII:
       
         if (! exact)
        {
          index.table.pw_gen_fec_agi_type =  0;
          index.table.pw_gen_fec_agi =  0;
          index.table.pw_gen_fec_loc_aii_type =  0;
          index.table.pw_gen_fec_loc_aii =  0;
          index.table.pw_gen_fec_rmte_aii_type =  0;
          index.table.pw_gen_fec_rmte_aii =  0;

          nsm_pw_snmp_index_set (v, name, length,
                                 (struct nsm_pw_snmp_index *)&index,
                                 PW_GEN_FEC_MAP_TABLE);
        }
      return NULL;

      case PW_GEN_FEC_PW_ID: 
        /* Read Only Object */
      if (NSM_PW_SNMP_GET32NEXT (pw_gen_fec_vc_id,
                                index.table.pw_gen_fec_agi_type,
                                index.table.pw_gen_fec_agi,
                                index.table.pw_gen_fec_loc_aii_type,
                                index.table.pw_gen_fec_loc_aii,
                                index.table.pw_gen_fec_rmte_aii_type,
                                index.table.pw_gen_fec_rmte_aii, 
                                &nsm_pw_snmp_int_val))
        {
          nsm_pw_snmp_index_set (v, name, length,
                                 (struct nsm_pw_snmp_index *)&index,
                                 PW_GEN_FEC_MAP_TABLE);
          NSM_PW_SNMP_RETURN (nsm_pw_snmp_int_val, PW_GEN_FEC_MAP_TABLE);
        }

      break;
      
      default:
      return NULL;
    }
  return NULL;
}
#endif /*HAVE_FEC129 */
/* The peer mapping Entry */
static u_char *
pwPeerMappingEntry (struct variable *v, oid *name, size_t *length,
                    u_int32_t exact, size_t *var_len,
                    WriteMethod **write_method, u_int32_t vr_id)
{
  NSM_PW_SNMP_UNION(pw_peer_map_table_index, index);
  struct nsm_master *nm = nsm_master_lookup_by_id (nzg, 0);

  nsm_pw_snmp_int_val = NSM_PW_SNMP_VALUE_INVALID;
  nsm_pw_snmp_char_ptr[0] = '\0';
  pal_mem_set (&index, 0, sizeof (struct pw_peer_map_table_index));

  if (!nm)
    return NULL;
  /* Get table indices. */
  if (! nsm_pw_snmp_index_get (v, name, length,
                               &index.snmp,
                               PW_PEER_MAP_TABLE, exact))
    return NULL;

  if (index.table.len == 0)
    {
      index.table.pw_peer_map_peer_addr_type =  0;
      pal_mem_set (&index.table.pw_peer_map_peer_addr,
                   0, sizeof (struct pal_in4_addr));
      index.table.pw_peer_map_pw_type =  0;
      index.table.pw_peer_map_pw_id =  0;
    }

  switch (v->magic)
    {
    case PW_PEER_MAP_PEER_ADDR_TYPE:
    case PW_PEER_MAP_PEER_ADDR:
    case PW_PEER_MAP_PW_TYPE:
    case PW_PEER_MAP_PW_ID:

      if (! exact)
        {
          index.table.pw_peer_map_peer_addr_type =  0;
          pal_mem_set (&index.table.pw_peer_map_peer_addr,
                       0, sizeof (struct pal_in4_addr));
          index.table.pw_peer_map_pw_type =  0;
          index.table.pw_peer_map_pw_id =  0;

          nsm_pw_snmp_index_set (v, name, length,
                                 (struct nsm_pw_snmp_index *)&index,
                                 PW_PEER_MAP_TABLE);
        }
      return NULL;

    case PW_PEER_MAP_PW_IX:
      /* Read Only Object */
      if (NSM_PW_SNMP_GET8NEXT (pw_peer_map_pw_ix,
                                index.table.pw_peer_map_peer_addr_type,
                                index.table.pw_peer_map_peer_addr,
                                index.table.pw_peer_map_pw_type,
                                index.table.pw_peer_map_pw_id,
                                &nsm_pw_snmp_int_val))
        {
          nsm_pw_snmp_index_set (v, name, length,
                                 (struct nsm_pw_snmp_index *)&index,
                                 PW_PEER_MAP_TABLE);
          NSM_PW_SNMP_RETURN (nsm_pw_snmp_int_val, PW_PEER_MAP_TABLE);
        }
      break;

    default:
      return NULL;

    }
  return NULL;
}

/* PW Enet Entry */
static u_char *
pwEnetEntry (struct variable *v, oid *name, size_t *length,
           u_int32_t exact, size_t *var_len,
           WriteMethod **write_method, u_int32_t vr_id)
{
  NSM_PW_SNMP_UNION(pw_enet_table_index, index);
  struct nsm_master *nm = nsm_master_lookup_by_id (nzg, 0);

  nsm_pw_snmp_int_val = NSM_PW_SNMP_VALUE_INVALID;

  if (!nm)
    return NULL;

  /* Get table indices. */
  if (! nsm_pw_snmp_index_get (v, name, length,
                               &index.snmp,
                               PW_ENET_TABLE, exact))
    return NULL;

  if (index.table.len == 0)
    {
      /* Index of Table is pw_index and pw_enet_pwinstance */
      index.table.pw_ix = 0;
      index.table.pw_instance = 0;
    }
  /* Return the current value of the variable. */
  switch (v->magic)
    {
    case PW_ENET_PW_INSTANCE:
      if (! exact)
        {
          index.table.pw_ix = 0;
          index.table.pw_instance = NSM_PW_ENET_PW_INSTANCE;
          nsm_pw_snmp_index_set (v, name, length,
                                 (struct nsm_pw_snmp_index *)&index,
                                 PW_ENET_TABLE);
        }
      return NULL;

    case PW_ENET_PW_VLAN:
      *write_method = write_pwEnetEntry;
      if (NSM_PW_SNMP_GET2NEXT (pw_enet_pw_vlan, index.table.pw_ix,
                                index.table.pw_instance, &nsm_pw_snmp_int_val))
        NSM_PW_SNMP_RETURN (nsm_pw_snmp_int_val, PW_ENET_TABLE);
      break;

    case PW_ENET_VLAN_MODE:
      *write_method = write_pwEnetEntry;
      if (NSM_PW_SNMP_GET2NEXT (pw_enet_vlan_mode, index.table.pw_ix,
                                index.table.pw_instance, &nsm_pw_snmp_int_val))
        NSM_PW_SNMP_RETURN (nsm_pw_snmp_int_val, PW_ENET_TABLE);
      break;
   case PW_ENET_PORT_VLAN:
      *write_method = write_pwEnetEntry;
      if (NSM_PW_SNMP_GET2NEXT (pw_enet_port_vlan, index.table.pw_ix,
                                index.table.pw_instance, &nsm_pw_snmp_int_val))
        NSM_PW_SNMP_RETURN (nsm_pw_snmp_int_val, PW_ENET_TABLE);
      break;

   case PW_ENET_PORT_IF_INDEX:
      *write_method = write_pwEnetEntry;
      if (NSM_PW_SNMP_GET2NEXT (pw_enet_port_if_index, index.table.pw_ix,
                                index.table.pw_instance, &nsm_pw_snmp_int_val))
        NSM_PW_SNMP_RETURN (nsm_pw_snmp_int_val, PW_ENET_TABLE);
      break;

   case PW_ENET_PW_IF_INDEX:
      *write_method = write_pwEnetEntry;
      if (NSM_PW_SNMP_GET2NEXT (pw_enet_pw_if_index, index.table.pw_ix,
                                index.table.pw_instance, &nsm_pw_snmp_int_val))
        NSM_PW_SNMP_RETURN (nsm_pw_snmp_int_val, PW_ENET_TABLE);
      break;

   case PW_ENET_ROW_STATUS:
      *write_method = write_pwEnetEntry;
      if (NSM_PW_SNMP_GET2NEXT (pw_enet_row_status, index.table.pw_ix,
                                index.table.pw_instance, &nsm_pw_snmp_int_val))
        NSM_PW_SNMP_RETURN (nsm_pw_snmp_int_val, PW_ENET_TABLE);
      break;

    case PW_ENET_STORAGE_TYPE:
      *write_method = write_pwEnetEntry;
      if (NSM_PW_SNMP_GET2NEXT (pw_enet_storage_type, index.table.pw_ix,
                                index.table.pw_instance, &nsm_pw_snmp_int_val))
        NSM_PW_SNMP_RETURN (nsm_pw_snmp_int_val, PW_ENET_TABLE);
      break;
   default:
      return NULL;
    }
  return NULL;
}

/* PW Enet Statistics Entry */
static u_char *
PwEnetStatsEntry (struct variable *v, oid *name, size_t *length,
                  u_int32_t exact, size_t *var_len,
                  WriteMethod **write_method, u_int32_t vr_id)
{
  NSM_PW_SNMP_UNION(pw_enet_stats_table_index, index);
  struct nsm_master *nm = nsm_master_lookup_by_id (nzg, 0);

  nsm_pw_snmp_int_val = NSM_PW_SNMP_VALUE_INVALID;

  if (!nm)
    return NULL;

  /* Get table indices. */
  if (! nsm_pw_snmp_index_get (v, name, length,
                               &index.snmp,
                               PW_ENET_STATISTICS_TABLE, exact))
    return NULL;

  if (index.table.len == 0)
    {
      /* Index of Table is pw_index */
      index.table.pw_ix = 0;
    }

  /* Return the current value of the variable. */
  switch (v->magic)
    {
    case PW_ENET_STATS_ILLEGAL_VLAN:
      if (NSM_PW_SNMP_GET_NEXT (pw_enet_stats_illegal_vlan, index.table.pw_ix,
                                &nsm_pw_snmp_int_val))
        NSM_PW_SNMP_RETURN (nsm_pw_snmp_int_val, PW_ENET_STATISTICS_TABLE);
      break;

    case PW_ENET_STATS_ILLEGAL_LENGTH:
      if (NSM_PW_SNMP_GET_NEXT (pw_enet_stats_illegal_length, index.table.pw_ix,
                                &nsm_pw_snmp_int_val))
        NSM_PW_SNMP_RETURN (nsm_pw_snmp_int_val, PW_ENET_STATISTICS_TABLE);
      break;
    }
  return NULL;
}

/* PW MPLS Entry */
static u_char *
pwMplsEntry (struct variable *v, oid *name, size_t *length,
             u_int32_t exact, size_t *var_len,
             WriteMethod **write_method, u_int32_t vr_id)
{
  NSM_PW_SNMP_UNION(pw_mpls_table_index, index);
  struct nsm_master *nm = nsm_master_lookup_by_id (nzg, 0);

  pal_mem_set (&index, 0, sizeof (struct pw_mpls_table_index));
  nsm_pw_snmp_int_val = NSM_PW_SNMP_VALUE_INVALID;
  nsm_pw_snmp_char_ptr[0] = '\0';

  if (!nm)
    return NULL;

  /* Get table indices. */
  if (! nsm_pw_snmp_index_get (v, name, length,
                               &index.snmp,
                               PW_MPLS_TABLE, exact))
    return NULL;

  if (index.table.len == 0)
    index.table.pw_ix = 0;

  /* Return the current value of the variable. */
  switch (v->magic)
    {
    case PW_MPLS_MPLS_TYPE:
      *write_method = write_pwMplsEntry;
      if (NSM_PW_SNMP_GET_NEXT (pw_mpls_mpls_type, index.table.pw_ix,
                                &nsm_pw_snmp_char_ptr))
        NSM_PW_SNMP_RETURN_BITS (nsm_pw_snmp_char_ptr, PW_MPLS_TABLE);
      break;

    case PW_MPLS_EXP_BITS_MODE:
      *write_method = write_pwMplsEntry;
      if (NSM_PW_SNMP_GET_NEXT (pw_mpls_exp_bits_mode, index.table.pw_ix,
                                &nsm_pw_snmp_int_val))
        NSM_PW_SNMP_RETURN (nsm_pw_snmp_int_val, PW_MPLS_TABLE);
      break;

    case PW_MPLS_EXP_BITS:
      *write_method = write_pwMplsEntry;
      if (NSM_PW_SNMP_GET_NEXT (pw_mpls_exp_bits, index.table.pw_ix,
                                &nsm_pw_snmp_int_val))
        NSM_PW_SNMP_RETURN (nsm_pw_snmp_int_val, PW_MPLS_TABLE);
      break;

    case PW_MPLS_TTL:
      *write_method = write_pwMplsEntry;
      if (NSM_PW_SNMP_GET_NEXT (pw_mpls_ttl, index.table.pw_ix,
                                &nsm_pw_snmp_int_val))
        NSM_PW_SNMP_RETURN (nsm_pw_snmp_int_val, PW_MPLS_TABLE);
      break;

    case PW_MPLS_LCL_LDP_ID:
      *write_method = write_pwMplsEntry;
      if (NSM_PW_SNMP_GET_NEXT (pw_mpls_lcl_ldp_id, index.table.pw_ix,
                                &nsm_pw_snmp_char_ptr))
        NSM_PW_SNMP_RETURN_CHAR (nsm_pw_snmp_char_ptr, PW_MPLS_TABLE);
      break;

    case PW_MPLS_LCL_LDP_ENTTY_IX:
      *write_method = write_pwMplsEntry;
      if (NSM_PW_SNMP_GET_NEXT (pw_mpls_lcl_ldp_entty_ix, index.table.pw_ix,
                                &nsm_pw_snmp_int_val))
        NSM_PW_SNMP_RETURN (nsm_pw_snmp_int_val, PW_MPLS_TABLE);
      break;

    case PW_MPLS_PEER_LDP_ID:
      if (NSM_PW_SNMP_GET_NEXT (pw_mpls_peer_ldp_id, index.table.pw_ix,
                                &nsm_pw_snmp_char_ptr))
        NSM_PW_SNMP_RETURN_CHAR (nsm_pw_snmp_char_ptr, PW_MPLS_TABLE);
      break;

    case PW_MPLS_ST_TYPE:
      *write_method = write_pwMplsEntry;
      if (NSM_PW_SNMP_GET_NEXT (pw_mpls_st_type, index.table.pw_ix,
                                &nsm_pw_snmp_int_val))
        NSM_PW_SNMP_RETURN (nsm_pw_snmp_int_val, PW_MPLS_TABLE);
      break;

    default:
      return NULL;
    }
  return NULL;
}

/* PW MPLS Outbound Tunnel Entry */
static u_char *
pwMplsOutboundEntry (struct variable *v, oid *name, size_t *length,
                     u_int32_t exact, size_t *var_len,
                     WriteMethod **write_method, u_int32_t vr_id)
{
  NSM_PW_SNMP_UNION(pw_mpls_outbd_tnl_table_index, index);
  struct nsm_master *nm = nsm_master_lookup_by_id (nzg, 0);

  nsm_pw_snmp_int_val = NSM_PW_SNMP_VALUE_INVALID;
  nsm_pw_snmp_char_ptr[0] = '\0';

  if (!nm)
    return NULL;

  /* Get table indices. */
  if (! nsm_pw_snmp_index_get (v, name, length,
                               &index.snmp,
                               PW_MPLS_OUTBD_TNL_TABLE, exact))
    return NULL;

  if (index.table.len == 0)
    index.table.pw_ix = 0;

  /* Return the current value of the variable. */
  switch (v->magic)
    {
    case PW_MPLS_OUTBD_LSR_XC_IX:
      *write_method = write_pwMplsOutboundEntry;
      if (NSM_PW_SNMP_GET_NEXT (pw_mpls_outbd_lsr_xc_ix, index.table.pw_ix,
                                &nsm_pw_snmp_int_val))
        NSM_PW_SNMP_RETURN (nsm_pw_snmp_int_val, PW_MPLS_OUTBD_TNL_TABLE);
      break;

    case PW_MPLS_OUTBD_TNL_IX:
      *write_method = write_pwMplsOutboundEntry;
      if (NSM_PW_SNMP_GET_NEXT (pw_mpls_outbd_tnl_ix, index.table.pw_ix,
                                &nsm_pw_snmp_int_val))
        NSM_PW_SNMP_RETURN (nsm_pw_snmp_int_val, PW_MPLS_OUTBD_TNL_TABLE);
      break;

    case PW_MPLS_OUTBD_TNL_INSTNCE:
      if (NSM_PW_SNMP_GET_NEXT (pw_mpls_outbd_tnl_instnce, index.table.pw_ix,
                                &nsm_pw_snmp_int_val))
        NSM_PW_SNMP_RETURN (nsm_pw_snmp_int_val, PW_MPLS_OUTBD_TNL_TABLE);
      break;

    case PW_MPLS_OUTBD_TNL_LCL_LSR:
      *write_method = write_pwMplsOutboundEntry;
      if (NSM_PW_SNMP_GET_NEXT (pw_mpls_outbd_tnl_lcl_lsr, index.table.pw_ix,
                                &nsm_pw_snmp_char_ptr))
        NSM_PW_SNMP_RETURN_CHAR (nsm_pw_snmp_char_ptr,
                                 PW_MPLS_OUTBD_TNL_TABLE);

    case PW_MPLS_OUTBD_TNL_PEER_LSR:
      *write_method = write_pwMplsOutboundEntry;
      if (NSM_PW_SNMP_GET_NEXT (pw_mpls_outbd_tnl_peer_lsr, index.table.pw_ix,
                                &nsm_pw_snmp_char_ptr))
        NSM_PW_SNMP_RETURN_CHAR (nsm_pw_snmp_char_ptr,
                                 PW_MPLS_OUTBD_TNL_TABLE);
      break;

    case PW_MPLS_OUTBD_IF_IX:
      *write_method = write_pwMplsOutboundEntry;
      if (NSM_PW_SNMP_GET_NEXT (pw_mpls_outbd_if_ix, index.table.pw_ix,
                                &nsm_pw_snmp_int_val))
        NSM_PW_SNMP_RETURN (nsm_pw_snmp_int_val, PW_MPLS_OUTBD_TNL_TABLE);
      break;

    case PW_MPLS_OUTBD_TNL_TYP_INUSE:
      if (NSM_PW_SNMP_GET_NEXT (pw_mpls_outbd_tnl_typ_inuse, index.table.pw_ix,
                                &nsm_pw_snmp_int_val))
        NSM_PW_SNMP_RETURN (nsm_pw_snmp_int_val, PW_MPLS_OUTBD_TNL_TABLE);
      break;

    default:
      return NULL;
    }
  return NULL;
}

/* PW MPLS inbound Entry */
static u_char *
pwMplsInboundEntry (struct variable *v, oid *name, size_t *length,
                    u_int32_t exact, size_t *var_len,
                    WriteMethod **write_method, u_int32_t vr_id)
{
  NSM_PW_SNMP_UNION(pw_mpls_inbd_table_index, index);
  struct nsm_master *nm = nsm_master_lookup_by_id (nzg, 0);

  nsm_pw_snmp_int_val = NSM_PW_SNMP_VALUE_INVALID;
  nsm_pw_snmp_char_ptr[0] = '\0';

  if (!nm)
    return NULL;

  /* Get table indices. */
  if (! nsm_pw_snmp_index_get (v, name, length,
                               &index.snmp,
                               PW_MPLS_INBD_TABLE, exact))
    return NULL;

  if (index.table.len == 0)
    index.table.pw_ix = 0;

  /* Return the current value of the variable. */
  switch (v->magic)
    {
    case PW_MPLS_INBD_XC_IX:
      if (NSM_PW_SNMP_GET_NEXT (pw_mpls_inbd_xc_ix, index.table.pw_ix,
                                &nsm_pw_snmp_int_val))
        NSM_PW_SNMP_RETURN (nsm_pw_snmp_int_val, PW_MPLS_INBD_TABLE);
      break;

    default:
      return NULL;
    }
  return NULL;
}

/* PW to Non-TE mapping Entry */
static u_char *
pwMplsNonTeMappingEntry (struct variable *v, oid *name, size_t *length,
                         u_int32_t exact, size_t *var_len,
                         WriteMethod **write_method, u_int32_t vr_id)
{
  NSM_PW_SNMP_UNION(pw_non_te_map_table_index, index);
  struct nsm_master *nm = nsm_master_lookup_by_id (nzg, 0);

  nsm_pw_snmp_int_val = NSM_PW_SNMP_VALUE_INVALID;
  nsm_pw_snmp_char_ptr[0] = '\0';
  pal_mem_set (&index, 0, sizeof (struct pw_non_te_map_table_index));

  if (!nm)
    return NULL;

  /* Get table indices. */
  if (! nsm_pw_snmp_index_get (v, name, length,
                               &index.snmp,
                               PW_NON_TE_MAP_TABLE, exact))
    return NULL;

  if (index.table.len == 0)
    {
      index.table.pw_mpls_non_te_map_dn =  0;
      index.table.pw_mpls_non_te_map_xc_ix =  0;
      index.table.pw_mpls_non_te_map_if_ix =  0;
      index.table.pw_mpls_non_te_map_pw_ix =  0;
    }

  switch (v->magic)
    {
    case PW_MPLS_NON_TE_MAP_DN:
    case PW_MPLS_NON_TE_MAP_XC_IX:
    case PW_MPLS_NON_TE_MAP_IF_IX:

      if (! exact)
        {
          index.table.pw_mpls_non_te_map_dn =  0;
          index.table.pw_mpls_non_te_map_xc_ix =  0;
          index.table.pw_mpls_non_te_map_if_ix =  0;
          index.table.pw_mpls_non_te_map_pw_ix =  0;

          nsm_pw_snmp_index_set (v, name, length,
                                 (struct nsm_pw_snmp_index *)&index,
                                 PW_NON_TE_MAP_TABLE);
        }
      return NULL;

    case PW_MPLS_NON_TE_MAP_PW_IX:
      /* Read Only Object */
      if (NSM_PW_SNMP_GET8NEXT (pw_mpls_non_te_map_pw_ix,
                                index.table.pw_mpls_non_te_map_dn,
                                index.table.pw_mpls_non_te_map_xc_ix,
                                index.table.pw_mpls_non_te_map_if_ix,
                                index.table.pw_mpls_non_te_map_pw_ix,
                                &nsm_pw_snmp_int_val))
        {
          nsm_pw_snmp_index_set (v, name, length,
                                 (struct nsm_pw_snmp_index *)&index,
                                 PW_NON_TE_MAP_TABLE);
          NSM_PW_SNMP_RETURN (nsm_pw_snmp_int_val, PW_NON_TE_MAP_TABLE);
        }
      break;

    default:
      return NULL;
    }
  return NULL;
}

/* PW to TE MPLS tunnels mapping Entry */
static u_char *
pwMplsTeMappingEntry (struct variable *v, oid *name, size_t *length,
                      u_int32_t exact, size_t *var_len,
                      WriteMethod **write_method, u_int32_t vr_id)
{
  NSM_PW_SNMP_UNION(pw_te_mpls_tnl_map_table_index, index);
  struct nsm_master *nm = nsm_master_lookup_by_id (nzg, 0);

  nsm_pw_snmp_int_val = NSM_PW_SNMP_VALUE_INVALID;
  nsm_pw_snmp_char_ptr[0] = '\0';
  pal_mem_set (&index, 0, sizeof (struct pw_te_mpls_tnl_map_table_index));

  if (!nm)
    return NULL;

  /* Get table indices. */
  if (! nsm_pw_snmp_index_get (v, name, length,
                               &index.snmp,
                               PW_TE_MPLS_TNL_MAP_TABLE, exact))
    return NULL;

  if (index.table.len == 0)
    {
      index.table.pw_mpls_te_map_tnl_ix =  0;
      index.table.pw_mpls_te_map_tnl_instnce =  0;
      pal_mem_set (&index.table.pw_mpls_te_map_tnl_peer_lsr_id,
                   0, sizeof (struct pal_in4_addr));
      pal_mem_set (&index.table.pw_mpls_te_map_tnl_lcl_lsr_id,
                   0, sizeof (struct pal_in4_addr));
      index.table.pw_mpls_te_map_pw_ix =  0;
    }

  switch (v->magic)
    {
    case PW_MPLS_TE_MAP_TNL_IX:
    case PW_MPLS_TE_MAP_TNL_INSTNCE:
    case PW_MPLS_TE_MAP_TNL_PEER_LSR_ID:
    case PW_MPLS_TE_MAP_TNL_LCL_LSR_ID:

      if (! exact)
        {
          index.table.pw_mpls_te_map_tnl_ix =  0;
          index.table.pw_mpls_te_map_tnl_instnce =  0;
          pal_mem_set (&index.table.pw_mpls_te_map_tnl_peer_lsr_id,
                       0, sizeof (struct pal_in4_addr));
          pal_mem_set (&index.table.pw_mpls_te_map_tnl_lcl_lsr_id,
                       0, sizeof (struct pal_in4_addr));
          index.table.pw_mpls_te_map_pw_ix =  0;

          nsm_pw_snmp_index_set (v, name, length,
                                 (struct nsm_pw_snmp_index *)&index,
                                 PW_TE_MPLS_TNL_MAP_TABLE);
        }
      return NULL;

    case PW_MPLS_TE_MAP_PW_IX:
      /* Read Only Object */
      if (NSM_PW_SNMP_GET16NEXT (pw_mpls_te_map_pw_ix,
                                 index.table.pw_mpls_te_map_tnl_ix,
                                 index.table.pw_mpls_te_map_tnl_instnce,
                                 index.table.pw_mpls_te_map_tnl_peer_lsr_id,
                                 index.table.pw_mpls_te_map_tnl_lcl_lsr_id,
                                 index.table.pw_mpls_te_map_pw_ix,
                                 &nsm_pw_snmp_int_val))
        NSM_PW_SNMP_RETURN (nsm_pw_snmp_int_val, PW_TE_MPLS_TNL_MAP_TABLE);
      break;

    default:
      return NULL;
    }
  return NULL;
}


/* Register PW-MIB */
void
nsm_pw_snmp_init ()
{
  REGISTER_MIB (NSM_ZG, "mibII/nsm_pw", nsm_pw_objects,
                variable, pw_oid);
  REGISTER_MIB (NSM_ZG, "mibII/nsm_pw_enet", pw_enet_objects,
                variable, pw_enet_oid);
  REGISTER_MIB (NSM_ZG, "mibII/nsm_pw_mpls", nsm_pw_mpls_objects,
                variable, pw_mpls_oid);

  /* Init link list for VC Temp structure */
  vc_entry_temp_list = list_new ();
}
/* Traps */
void
nsm_mpls_vc_up_down_notify (int opr_status,
                            u_int32_t vc_snmp_index)
{
  struct trap_object2 obj[2];
  struct nsm_master *nm = nsm_master_lookup_by_id (nzg, 0);
  struct nsm_mpls *nmpls;
  int namelen;
  oid *ptr;
  u_int32_t notify_val = 0 ;

  if (!nm)
    return;

  nmpls = nm->nmpls;

  nsm_mpls_get_pw_up_dn_notify(nm, &notify_val);

  if (!notify_val  
      || notify_val == NSM_MPLS_SNMP_VC_UP_DN_NTFY_DIS
      || !nsm_mpls_pw_snmp_trap_limiter (nmpls))
    return;
  
  namelen = sizeof pw_oid / sizeof (oid);

  /* pwOperStatus OID */
  ptr = obj[0].name;
  OID_COPY (ptr, pw_oid, namelen);
  OID_SET_ARG5 (ptr, 1, 2, 1, 38, vc_snmp_index);

  OID_SET_VAL (obj[0], ptr - obj[0].name, INTEGER,
               sizeof (int), &opr_status);

  /* pwOperStatus OID */
  ptr = obj[1].name;
  OID_COPY (ptr, pw_oid, namelen);
  OID_SET_ARG5 (ptr, 1, 2, 1, 38, vc_snmp_index);

  OID_SET_VAL (obj[1], ptr - obj[1].name, INTEGER,
               sizeof (int), &opr_status);

  snmp_trap2 (nzg,
              pw_trap_oid, sizeof pw_trap_oid / sizeof (oid),
              ((opr_status == PW_L2_STATUS_UP) ? PW_UP_NOTIFICATION
                                               : PW_DOWN_NOTIFICATION),
              pw_oid, sizeof pw_oid / sizeof (oid),
              obj, sizeof obj / sizeof (struct trap_object2),
              pal_time_current (NULL) );

  nmpls->notify_cnt++;
}

void
nsm_mpls_vc_del_notify (u_int32_t vc_id, u_int32_t pw_type,
                        int peer_addr_type, struct pal_in4_addr *addr,
                        u_int32_t vc_snmp_index)
{
  struct trap_object2 obj[4];
  int namelen;
  oid *ptr;
  u_int32_t notify_val = 0 ;

  struct nsm_master *nm = nsm_master_lookup_by_id (nzg, 0);
  struct nsm_mpls *nmpls = NULL;

  if (!nm)
    return;

  nmpls = nm->nmpls;

  if (!nmpls)
    return;

  nsm_mpls_get_pw_del_notify(nm, &notify_val);

  if (!notify_val  || 
      (notify_val == NSM_MPLS_SNMP_VC_DEL_NTFY_DIS) || 
      (!nsm_mpls_pw_snmp_trap_limiter (nmpls)))
    return;

  nsm_pw_snmp_char_ptr[0] = '\0';
  namelen = sizeof pw_oid / sizeof (oid);

  /* pwType OID */
  ptr = obj[0].name;
  OID_COPY (ptr, pw_oid, namelen);
  OID_SET_ARG5 (ptr, 1, 2, 1, 2, vc_snmp_index);
  OID_SET_VAL (obj[0], ptr - obj[0].name, INTEGER,
               sizeof (int), &pw_type);

  /* pwIndex OID */
  ptr = obj[1].name;
  OID_COPY (ptr, pw_oid, namelen);
  OID_SET_ARG5 (ptr, 1, 2, 1, 1, vc_snmp_index);

  OID_SET_VAL (obj[1], ptr - obj[1].name, INTEGER,
               sizeof (int), &vc_id);

  /* pwAddrType OID */
  ptr = obj[2].name;
  OID_COPY (ptr, pw_oid, namelen);
  OID_SET_ARG5 (ptr, 1, 2, 1, 8, vc_snmp_index);

  OID_SET_VAL (obj[2], ptr - obj[2].name, INTEGER,
               sizeof (int), &peer_addr_type);

  /* pwAddr OID */
  ptr = obj[3].name;
  OID_COPY (ptr, pw_oid, namelen);
  OID_SET_ARG5 (ptr, 1, 2, 1, 9, vc_snmp_index);
  pal_inet_ntoa (*addr, nsm_pw_snmp_char_ptr);

  OID_SET_VAL (obj[3], ptr - obj[3].name, STRING,
               pal_strlen(nsm_pw_snmp_char_ptr), nsm_pw_snmp_char_ptr);

  snmp_trap2 (nzg,
              pw_trap_oid, sizeof pw_trap_oid / sizeof (oid),
              PW_DELETE_NOTIFICATION,pw_oid, sizeof pw_oid / sizeof (oid),
              obj, sizeof obj / sizeof (struct trap_object2),
              pal_time_current (NULL) );

  nmpls->notify_cnt++;
}

/** @brief Function to limit the number of traps
  generation to the max trap limit

    @param  struct nsm_mpls *nmpls

    @return 1 
            0
*/
int
nsm_mpls_pw_snmp_trap_limiter(struct nsm_mpls *nmpls)
{
  if (nmpls->notify_cnt > nmpls->vc_pw_notification_rate)
    {
      /* Don't generate the trap if the limit is reached 
       * within the same second */
      if ((pal_time_current(NULL) - nmpls->notify_time_init) < 1)
        {
          return 0;
        }
      else 
        {
          nmpls->notify_cnt = 0;
          nmpls->notify_time_init = pal_time_current(NULL);
        }
    }
 return 1; 
}

int check_if_notify_send(struct nsm_mpls *nmpls)
  {
  if (nmpls->notify_time_init)
      {
        if (((nmpls->notify_time_init -
             pal_time_current(NULL)) <= 1000) &&
             nmpls->notify_cnt > nmpls->vc_pw_notification_rate)
          {
            return 0;
          }
      }

    if ((!nmpls->notify_time_init) ||
        ((nmpls->notify_time_init -
          pal_time_current(NULL)) > 1000))
      {
        nmpls->notify_time_init = pal_time_current(NULL);
        nmpls->notify_cnt = 0;
      }

   return 1;
 }

#endif /* HAVE_SNMP */
#endif /* HAVE_MPLS_VC */
