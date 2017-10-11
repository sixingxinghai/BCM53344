/* Copyright (C) 2001-2003  All Rights Reserved. */

#include "pal.h"
#include "lib.h"

#ifdef HAVE_SNMP

#include "asn1.h"
#include "bitmap.h"

#include "if.h"
#include "prefix.h"
#include "snmp.h"
#include "log.h"
#include "mpls_common.h"
#include "mpls.h"
#ifdef HAVE_DIFFSERV
#include "diffserv.h"
#endif /* HAVE_DIFFSERV */
#ifdef HAVE_DSTE
#include "dste.h"
#endif /* HAVE_DSTE */

#include "nsmd.h"
#include "nsm_router.h"
#ifdef HAVE_DSTE
#include "nsm_dste.h"
#endif /* HAVE_DSTE */
#include "nsm_mpls.h"
#ifdef HAVE_MPLS_VC
#include "nsm_mpls_vc.h"
#endif /* HAVE_MPLS_VC */
#include "nsm_mpls_rib.h"
#include "nsm_mpls_snmp.h"
#include "nsm_mpls_api.h"

/* Declare static local variables for convenience. */
static pal_time_t last_changed_time;

static u_int32_t nsm_mpls_snmp_int_val = NSM_MPLS_SNMP_VALUE_INVALID;
static ut_int64_t nsm_mpls_snmp_int_hc_val;
static pal_time_t nsm_mpls_snmp_time_val;

static char nsm_char_string[NSM_MPLS_MAX_STRING_LENGTH];
static char *nsm_mpls_snmp_char_ptr = nsm_char_string;

#ifndef HAVE_AGENTX
static char null_oid[] = { 0, 0 };
#endif

/*MPLS-LSR-MIB instances. */
oid lsr_oid [] = { MPLSLSRMIB };
oid ftn_oid [] = { MPLSFTNMIB };
oid nsmd_oid [] = { NSMDOID };
oid mpls_trap_oid[] = { MPLSLSRMIB, 0};


/* Hook functions. */
static u_char *mplsScalars ();
static u_char *mplsInterfaceConfEntry ();
static u_char *mplsInterfacePerfEntry ();
static u_char *mplsInSegmentEntry ();
static u_char *mplsInSegmentPerfEntry ();
static u_char *mplsOutSegmentEntry ();
static u_char *mplsOutSegmentPerfEntry ();
static u_char *mplsXCEntry ();
static u_char *mplsInSegMapEntry ();
static u_char *mplsFTNScalars ();
static u_char *mplsFTNEntry ();
static u_char *mplsFTNPerfEntry ();

struct variable nsm_mpls_lsr_variables[] = 
  {
    /* MPLS Interface Configuration Table. */
    {MPLS_IF_CONF_IX,        INTEGER,   NOACCESS, mplsInterfaceConfEntry,
     4, {1, 1, 1, 1}},
    {MPLS_IF_LB_MIN_IN,      UNSIGNED,  RONLY,    mplsInterfaceConfEntry,
     4, {1, 1, 1, 2}},
    {MPLS_IF_LB_MAX_IN,      UNSIGNED,  RONLY,    mplsInterfaceConfEntry,
     4, {1, 1, 1, 3}},
    {MPLS_IF_LB_MIN_OUT,     UNSIGNED,  RONLY,    mplsInterfaceConfEntry,
     4, {1, 1, 1, 4}},
    {MPLS_IF_LB_MAX_OUT,     UNSIGNED,  RONLY,    mplsInterfaceConfEntry,
     4, {1, 1, 1, 5}},
    {MPLS_IF_TOTAL_BW,       UNSIGNED,  RONLY,    mplsInterfaceConfEntry,
     4, {1, 1, 1, 6}},
    {MPLS_IF_AVA_BW,         UNSIGNED,  RONLY,    mplsInterfaceConfEntry,
     4, {1, 1, 1, 7}},
    {MPLS_IF_LB_PT_TYPE,     INTEGER,   RONLY,    mplsInterfaceConfEntry,
     4, {1, 1, 1, 8}},

    /* MPLS Interface Performance Table. */
    {MPLS_IF_IN_LB_USED,        GAUGE,     RONLY, mplsInterfacePerfEntry,
     4, {1, 2, 1, 1}},
    {MPLS_IF_FAILED_LB_LOOKUP,  INTEGER,   RONLY, mplsInterfacePerfEntry,
     4, {1, 2, 1, 2}},
    {MPLS_IF_OUT_LB_USED,       GAUGE,     RONLY, mplsInterfacePerfEntry,
     4, {1, 2, 1, 3}},
    {MPLS_IF_OUT_FRAGMENTS,     INTEGER,   RONLY, mplsInterfacePerfEntry,
     4, {1, 2, 1, 4}},

    /* MPLS In Segment Index Next */
    {MPLS_IN_SEG_IX_NXT,        INTEGER,   RONLY, mplsScalars, 
     2, {1, 3}},

    /* MPLS In-segment table. */
    /* All left out Objects from RFC 3813 are added */
    {MPLS_IN_SEG_IX,            INTEGER,   NOACCESS, mplsInSegmentEntry,
     4, {1, 4, 1, 1}},
    {MPLS_IN_SEG_IF,            INTEGER,   RWRITE,   mplsInSegmentEntry,
     4, {1, 4, 1, 2}},
    {MPLS_IN_SEG_LB,            UNSIGNED,  RWRITE,   mplsInSegmentEntry,
     4, {1, 4, 1, 3}},
    {MPLS_IN_SEG_LB_PTR,        STRING,    RWRITE,   mplsInSegmentEntry,
     4, {1, 4, 1, 4}},
    {MPLS_IN_SEG_NPOP,          INTEGER,   RWRITE,   mplsInSegmentEntry,
     4, {1, 4, 1, 5}},
    {MPLS_IN_SEG_ADDR_FAMILY,   INTEGER,   RWRITE,   mplsInSegmentEntry,
     4, {1, 4, 1, 6}},
    {MPLS_IN_SEG_XC_IX,         INTEGER,   RONLY,    mplsInSegmentEntry,
     4, {1, 4, 1, 7}},
    {MPLS_IN_SEG_OWNER,         INTEGER,   RONLY,    mplsInSegmentEntry,
     4, {1, 4, 1, 8}},
    {MPLS_IN_SEG_TF_PRM,        STRING,    RWRITE,   mplsInSegmentEntry,
     4, {1, 4, 1, 9}},
    {MPLS_IN_SEG_ROW_STATUS,    INTEGER,   RWRITE,   mplsInSegmentEntry,
     4, {1, 4, 1, 10}},
    {MPLS_IN_SEG_ST_TYPE,       INTEGER,   RWRITE,   mplsInSegmentEntry,
     4, {1, 4, 1, 11}},

    /* MPLS In-segment performance table. */
    {MPLS_IN_SEG_OCTS,          INTEGER,   RONLY,    mplsInSegmentPerfEntry,
     4, {1, 5, 1, 1}},
    {MPLS_IN_SEG_PKTS,          INTEGER,   RONLY,    mplsInSegmentPerfEntry,
     4, {1, 5, 1, 2}},
    {MPLS_IN_SEG_ERRORS,        INTEGER,   RONLY,    mplsInSegmentPerfEntry,
     4, {1, 5, 1, 3}},
    {MPLS_IN_SEG_DISCARDS,      INTEGER,   RONLY,    mplsInSegmentPerfEntry,
     4, {1, 5, 1, 4}},
    {MPLS_IN_SEG_HC_OCTS,       COUNTER64, RONLY,    mplsInSegmentPerfEntry,
     4, {1, 5, 1, 5}},
    {MPLS_IN_SEG_PERF_DIS_TIME, TIMETICKS, RONLY,    mplsInSegmentPerfEntry,
     4, {1, 5, 1, 6}},

    /* MPLS Out Segment Index Next */
    {MPLS_OUT_SEG_IX_NXT,       INTEGER,   RONLY,    mplsScalars, 
     2, {1, 6}},

    /* MPLS Out-segment table. */
    {MPLS_OUT_SEG_IX,               INTEGER,   NOACCESS, mplsOutSegmentEntry,
     4, {1, 7, 1, 1}},
    {MPLS_OUT_SEG_IF_IX,            INTEGER,   RWRITE,   mplsOutSegmentEntry,
     4, {1, 7, 1, 2}},
    {MPLS_OUT_SEG_PUSH_TOP_LB,      INTEGER,   RWRITE,   mplsOutSegmentEntry,
     4, {1, 7, 1, 3}},
    {MPLS_OUT_SEG_TOP_LB,           UNSIGNED,  RWRITE,   mplsOutSegmentEntry,
     4, {1, 7, 1, 4}},
    {MPLS_OUT_SEG_TOP_LB_PTR,       STRING,    RWRITE,   mplsOutSegmentEntry,
     4, {1, 7, 1, 5}},
    {MPLS_OUT_SEG_NXT_HOP_IPA_TYPE, INTEGER,   RWRITE,   mplsOutSegmentEntry,
     4, {1, 7, 1, 6}},
    {MPLS_OUT_SEG_NXT_HOP_IPA,      STRING,    RWRITE,   mplsOutSegmentEntry,
     4, {1, 7, 1, 7}},
    {MPLS_OUT_SEG_XC_IX,            INTEGER,   RONLY,    mplsOutSegmentEntry,
     4, {1, 7, 1, 8}},
    {MPLS_OUT_SEG_OWNER,            INTEGER,   RONLY,    mplsOutSegmentEntry,
     4, {1, 7, 1, 9}},
    {MPLS_OUT_SEG_TF_PRM,           STRING,    RWRITE,   mplsOutSegmentEntry,
     4, {1, 7, 1, 10}},
    {MPLS_OUT_SEG_ROW_STATUS,       INTEGER,   RWRITE,   mplsOutSegmentEntry,
     4, {1, 7, 1, 11}},
    {MPLS_OUT_SEG_ST_TYPE,          INTEGER,   RWRITE,   mplsOutSegmentEntry,
     4, {1, 7, 1, 12}},

    /* MPLS Out-segment performance table. */
    {MPLS_OUT_SEG_OCTS,         INTEGER,      RONLY,   mplsOutSegmentPerfEntry,
     4, {1, 8, 1, 1}},
    {MPLS_OUT_SEG_PKTS,         INTEGER,      RONLY,   mplsOutSegmentPerfEntry,
     4, {1, 8, 1, 2}},
    {MPLS_OUT_SEG_ERRORS,       INTEGER,      RONLY,   mplsOutSegmentPerfEntry,
     4, {1, 8, 1, 3}},
    {MPLS_OUT_SEG_DISCARDS,     INTEGER,      RONLY,   mplsOutSegmentPerfEntry,
     4, {1, 8, 1, 4}},
    {MPLS_OUT_SEG_HC_OCTS,      COUNTER64,    RONLY,   mplsOutSegmentPerfEntry,
     4, {1, 8, 1, 5}},
    {MPLS_OUT_SEG_DIS_TIME,     TIMETICKS,    RONLY,   mplsOutSegmentPerfEntry,
     4, {1, 8, 1, 6}},

    /* MPLS XC Index Next */
    {MPLS_XC_IX_NXT,            INTEGER,      RONLY,   mplsScalars, 
     2, {1, 9}},

    /* MPLS XC table. */
    {MPLS_XC_IX,                      INTEGER,   NOACCESS,    mplsXCEntry,
     4, {1, 10, 1, 1}},
    {MPLS_XC_IN_SEG_IX,               INTEGER,   NOACCESS,    mplsXCEntry,
     4, {1, 10, 1, 2}},
    {MPLS_XC_OUT_SEG_IX,              INTEGER,   NOACCESS,    mplsXCEntry,
     4, {1, 10, 1, 3}},
    /* This LspId object is defined as Integer as of now */
    /* since it is not supported */
    {MPLS_XC_LSPID,                   STRING,   RWRITE,      mplsXCEntry,
     4, {1, 10, 1, 4}},
    {MPLS_XC_LB_STK_IX,               INTEGER,   RWRITE,      mplsXCEntry,
     4, {1, 10, 1, 5}},
    {MPLS_XC_OWNER,                   INTEGER,   RONLY,       mplsXCEntry,
     4, {1, 10, 1, 6}},
    {MPLS_XC_ROW_STATUS,              INTEGER,   RWRITE,      mplsXCEntry,
     4, {1, 10, 1, 7}},
    {MPLS_XC_ST_TYPE,                 INTEGER,   RWRITE,      mplsXCEntry,
     4, {1, 10, 1, 8}},
    {MPLS_XC_ADM_STATUS,              INTEGER,   RWRITE,      mplsXCEntry,
     4, {1, 10, 1, 9}},
    {MPLS_XC_OPER_STATUS,             INTEGER,   RONLY,       mplsXCEntry,
     4, {1, 10, 1, 10}},

    /* MPLS InSegment Map Table. */
    {MPLS_IN_SEG_MAP_IF,        INTEGER,   NOACCESS,    mplsInSegMapEntry,
     4, {1, 14, 1, 1}},
    {MPLS_IN_SEG_MAP_LB,        INTEGER,   NOACCESS,    mplsInSegMapEntry,
     4, {1, 14, 1, 2}},
    {MPLS_IN_SEG_MAP_LB_PTR,    STRING,    NOACCESS,    mplsInSegMapEntry,
     4, {1, 14, 1, 3}},
    {MPLS_IN_SEG_MAP_IX,        INTEGER,   RONLY,       mplsInSegMapEntry,
     4, {1, 14, 1, 4}},

    /* MPLS XC Notification */
    {MPLS_XC_NOTIFN_CNTL,       INTEGER,   RWRITE,      mplsScalars,
     2, {1, 15}}
  };

struct variable nsm_mpls_ftn_variables[] = 
  {
    /* MPLS FTN Index Next */
    {MPLS_FTN_IX_NXT,           INTEGER,   RONLY,    mplsFTNScalars,
     2, {1, 1}},
    /* MPLS FTN Table Last Changed */
    {MPLS_FTN_LAST_CHANGED,     TIMETICKS, RONLY,    mplsFTNScalars,
     2, {1, 2}},

    /* MPLS FTN Table. */
    {MPLS_FTN_IX,           INTEGER,   NOACCESS, mplsFTNEntry,
     4, {1, 3, 1, 1}},
    {MPLS_FTN_ROW_STATUS,   INTEGER,   RWRITE,   mplsFTNEntry,
     4, {1, 3, 1, 2}},
    {MPLS_FTN_DESCR,        STRING,    RWRITE,   mplsFTNEntry,
     4, {1, 3, 1, 3}},
    {MPLS_FTN_MASK,         INTEGER,   RWRITE,   mplsFTNEntry,
     4, {1, 3, 1, 4}},
    {MPLS_FTN_ADDR_TYPE,    INTEGER,   RWRITE,   mplsFTNEntry,
     4, {1, 3, 1, 5}},
    {MPLS_FTN_SRC_ADDR_MIN, STRING,    RWRITE,   mplsFTNEntry,
     4, {1, 3, 1, 6}},
    {MPLS_FTN_SRC_ADDR_MAX, STRING,    RWRITE,   mplsFTNEntry,
     4, {1, 3, 1, 7}},
    {MPLS_FTN_DST_ADDR_MIN, STRING,    RWRITE,   mplsFTNEntry,
     4, {1, 3, 1, 8}},
    {MPLS_FTN_DST_ADDR_MAX, STRING,    RWRITE,   mplsFTNEntry,
     4, {1, 3, 1, 9}},
    {MPLS_FTN_SRC_PORT_MIN, INTEGER,   RWRITE,   mplsFTNEntry,
     4, {1, 3, 1, 10}},
    {MPLS_FTN_SRC_PORT_MAX, INTEGER,   RWRITE,   mplsFTNEntry,
     4, {1, 3, 1, 11}},
    {MPLS_FTN_DST_PORT_MIN, INTEGER,   RWRITE,   mplsFTNEntry,
     4, {1, 3, 1, 12}},  
    {MPLS_FTN_DST_PORT_MAX, INTEGER,   RWRITE,   mplsFTNEntry,
     4, {1, 3, 1, 13}},
    {MPLS_FTN_PROTOCOL,     INTEGER,   RWRITE,   mplsFTNEntry,
     4, {1, 3, 1, 14}},
    {MPLS_FTN_DSCP,         INTEGER,   RWRITE,   mplsFTNEntry,
     4, {1, 3, 1, 15}},
    {MPLS_FTN_ACT_TYPE,     INTEGER,   RWRITE,   mplsFTNEntry,
     4, {1, 3, 1, 16}},
    {MPLS_FTN_ACT_POINTER,  STRING,    RWRITE,   mplsFTNEntry,
     4, {1, 3, 1, 17}},
    {MPLS_FTN_ST_TYPE,      INTEGER,   RWRITE,   mplsFTNEntry,
     4, {1, 3, 1, 18}},

    /* MPLS FTN Performance table. */
    {MPLS_FTN_PERF_IX,       INTEGER,   NOACCESS, mplsFTNPerfEntry,
     4, {1, 6, 1, 1}},
    {MPLS_FTN_PERF_CURR_IX,  INTEGER,   NOACCESS, mplsFTNPerfEntry,
     4, {1, 6, 1, 2}},
    {MPLS_FTN_MTCH_PKTS,     COUNTER64, RONLY,    mplsFTNPerfEntry,
     4, {1, 6, 1, 3}},
    {MPLS_FTN_MTCH_OCTS,     COUNTER64, RONLY,    mplsFTNPerfEntry,
     4, {1, 6, 1, 4}},
    {MPLS_FTN_DISC_TIME,     TIMETICKS, RONLY,    mplsFTNPerfEntry,
     4, {1, 6, 1, 5}},
  };


struct nsm_mpls_mib_table_index nsm_mpls_mib_table_index_def[] =
  {
    /* 0 -- dummy. */
    {  0,   0, { NSM_MPLS_SNMP_INDEX_NONE } },
    /* 1 -- Interface Configuration Table. */
    {  1,   4, { NSM_MPLS_SNMP_INDEX_INT32,  NSM_MPLS_SNMP_INDEX_NONE } },
    /* 2 -- Interface Performace Table. */
    {  1,   4, { NSM_MPLS_SNMP_INDEX_INT32,  NSM_MPLS_SNMP_INDEX_NONE } },
    /* 3 -- In Segment Table */
    {  1,   4, { NSM_MPLS_SNMP_INDEX_INT32,  NSM_MPLS_SNMP_INDEX_NONE } },
    /* 4 -- In Segment Performance Table. */
    {  1,   4, { NSM_MPLS_SNMP_INDEX_INT32,  NSM_MPLS_SNMP_INDEX_NONE } },
    /* 5 -- Out Segment Table Table. */
    {  1,   4, { NSM_MPLS_SNMP_INDEX_INT32, NSM_MPLS_SNMP_INDEX_NONE } },
    /* 6 -- Out Segment Performance Table. */
    {  1,   4, { NSM_MPLS_SNMP_INDEX_INT32, NSM_MPLS_SNMP_INDEX_NONE } },
    /* 7 -- XC Table. */
    {  3,   12,{ NSM_MPLS_SNMP_INDEX_INT32, NSM_MPLS_SNMP_INDEX_INT32, 
                 NSM_MPLS_SNMP_INDEX_INT32, NSM_MPLS_SNMP_INDEX_NONE } },
    /* 8 -- FTN Table. */
    {  1,   4, { NSM_MPLS_SNMP_INDEX_INT32, NSM_MPLS_SNMP_INDEX_NONE } },
    /* 9 -- FTN Performace Table. */
    {  2,   8, { NSM_MPLS_SNMP_INDEX_INT32, NSM_MPLS_SNMP_INDEX_INT32, 
                 NSM_MPLS_SNMP_INDEX_NONE } },
    /* 10 -- In Segment Map Table. */
    {  3,   12,{ NSM_MPLS_SNMP_INDEX_INT32, NSM_MPLS_SNMP_INDEX_INT32, 
                 NSM_MPLS_SNMP_INDEX_INT32, NSM_MPLS_SNMP_INDEX_NONE } }
  };

/* MPLS SMUX AGENTX trap function */
void
mpls_snmp_smux_trap (oid *trap_oid, size_t trap_oid_len,
                     oid spec_trap_val, u_int32_t uptime,
                     struct snmp_trap_object *obj, size_t obj_len)
{
  snmp_trap2 (nzg, trap_oid, trap_oid_len, spec_trap_val,
              lsr_oid, sizeof (lsr_oid) / sizeof (oid),
              (struct trap_object2 *) obj, obj_len, uptime);
}

/* Get index array from oid. */
u_int32_t
nsm_mpls_snmp_index_get (struct variable *v, oid *name, size_t *length,
                         struct nsm_mpls_snmp_index *index, u_int32_t type,
                         u_int32_t exact)
{
  u_int32_t i = 0;
  u_int32_t offset = 0;

  index->len = *length - v->namelen;
  
  if (index->len == 0)
    return NSM_MPLS_SNMP_INDEX_GET_SUCCESS;
  
  if (exact)
    if (index->len != nsm_mpls_mib_table_index_def[type].len)
      return NSM_MPLS_SNMP_INDEX_GET_ERROR;
  
  if (index->len > nsm_mpls_mib_table_index_def[type].len)
    return NSM_MPLS_SNMP_INDEX_GET_ERROR;
  
  while (offset < index->len)
    {
      u_int32_t val = 0;
      u_int32_t vartype = nsm_mpls_mib_table_index_def[type].vars[i];
      u_int32_t j;
      
      if (vartype == NSM_MPLS_SNMP_INDEX_NONE)
        break;
      
      switch (vartype)
        {
        case NSM_MPLS_SNMP_INDEX_INT8:
        case NSM_MPLS_SNMP_INDEX_INT16:
        case NSM_MPLS_SNMP_INDEX_INT32:
          index->val[i++] = name[v->namelen + offset++];
          break;
        case NSM_MPLS_SNMP_INDEX_INADDR:
          for (j = 1; j <= 4 && offset < index->len; j++)
            val += name[v->namelen + offset++] << ((4 - j) * 8);

          index->val[i++] = pal_hton32 (val);
          break;
        default:
          NSM_ASSERT (0);
        }
    }

  return NSM_MPLS_SNMP_INDEX_GET_SUCCESS;
}

/* Set index array to oid. */
u_int32_t
nsm_gmpls_snmp_index_set (struct variable *v, oid *name, size_t *length,
                         struct nsm_mpls_snmp_index *index, u_int32_t type)
{
  u_int32_t i = 0;
  u_int32_t offset = 0;

  index->len = nsm_mpls_mib_table_index_def[type].len;

  while (offset < index->len)
    {
      unsigned long val;
      u_int32_t vartype = nsm_mpls_mib_table_index_def[type].vars[i];

      if (vartype == NSM_MPLS_SNMP_INDEX_NONE)
        break;

      switch (vartype)
        {
        case NSM_MPLS_SNMP_INDEX_INT8:
        case NSM_MPLS_SNMP_INDEX_INT16:
        case NSM_MPLS_SNMP_INDEX_INT32:
          name[v->namelen + offset++] = index->val[i++];
          break;
        case NSM_MPLS_SNMP_INDEX_INADDR:
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

  return NSM_MPLS_SNMP_INDEX_SET_SUCCESS;
}

/* Write Methods */
static int
write_mplsScalars (int action, u_char *var_val,
                   u_char var_val_type, size_t var_val_len,
                   u_char *statP, oid *name, size_t length,
                   struct variable *v, u_int32_t vr_id)
{
  long intval = 0;
  struct nsm_master *nm = nsm_master_lookup_by_id (nzg, 0);
  struct nsm_mpls *nmpls = NULL;

  if (!nm)
    return SNMP_ERR_GENERR;

  nmpls =  nm->nmpls;
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
    case MPLS_XC_NOTIFN_CNTL:
      if (intval == NSM_MPLS_NOTIFICATIONCNTL_ENA
          || intval == NSM_MPLS_NOTIFICATIONCNTL_DIS)    
        {
          if (nsm_mpls_set_notify (nmpls, intval) != NSM_API_SET_SUCCESS)
            return SNMP_ERR_GENERR;
        }
      else
        return SNMP_ERR_BADVALUE;   
      break;

    default:
      return SNMP_ERR_GENERR;
    }
  return SNMP_ERR_NOERROR;
}

static int
write_mplsInSegmentEntry (int action, u_char *var_val,
                          u_char var_val_type, size_t var_val_len,
                          u_char *statP, oid *name, size_t length,
                          struct variable *v, u_int32_t vr_id)
{
  long intval = 0;
  size_t val_len = 0;

  NSM_MPLS_SNMP_UNION(mpls_in_segment_table_index, index);
  struct nsm_master *nm = nsm_master_lookup_by_id (nzg, 0);
  if (!nm)
    return SNMP_ERR_GENERR;

  if (! nsm_mpls_snmp_index_get (v, name, &length, 
                                 &index.snmp,
                                 MPLS_IN_SEG_TABLE, 1)) /* exact = 1 */
    return SNMP_ERR_GENERR;

  if (var_val_type == ASN_INTEGER)
    pal_mem_cpy (&intval, var_val, 4);

  else if (var_val_type == ASN_OBJECT_ID)
    {
      val_len = pal_strlen (var_val);

#ifndef HAVE_AGENTX
      if (val_len != sizeof (null_oid))         /* Null OID */
        return SNMP_ERR_WRONGLENGTH;
#endif

      if (*(var_val + 2))               /* Only NULL Oid accepted */
        return SNMP_ERR_BADVALUE;
    }

  else    /* Neither Integer nor ObjectId */
    return SNMP_ERR_WRONGTYPE;

  switch (v->magic)
    {
    case MPLS_IN_SEG_IF:
      if (! (nsm_mpls_set_inseg_if (nm, index.table.in_seg_ix, intval)))
        return SNMP_ERR_GENERR;
      break;

    case MPLS_IN_SEG_LB:
      if (! (nsm_mpls_set_inseg_lb (nm, index.table.in_seg_ix, intval)))
        return SNMP_ERR_GENERR;
      break;

    case MPLS_IN_SEG_LB_PTR:
      if (! (nsm_mpls_set_inseg_lb_ptr (nm, index.table.in_seg_ix)))
        return SNMP_ERR_GENERR;
      break;

    case MPLS_IN_SEG_NPOP:
      if (! (nsm_mpls_set_inseg_npop (nm, index.table.in_seg_ix, intval)))
        return SNMP_ERR_GENERR;
      break;

    case MPLS_IN_SEG_ADDR_FAMILY:
      if (! (nsm_mpls_set_inseg_addr_family (nm, index.table.in_seg_ix, intval)))
        return SNMP_ERR_GENERR;
      break;
    
    case MPLS_IN_SEG_TF_PRM:
      if (! (nsm_mpls_set_inseg_tf_prm (nm, index.table.in_seg_ix)))
        return SNMP_ERR_GENERR;
      break;

    case MPLS_IN_SEG_ROW_STATUS:
      if (! (nsm_mpls_set_inseg_row_status (nm, index.table.in_seg_ix, intval)))
        return SNMP_ERR_GENERR;
      break;

    case MPLS_IN_SEG_ST_TYPE:
      if (! (nsm_mpls_set_inseg_st_type (nm, index.table.in_seg_ix, intval)))
        return SNMP_ERR_GENERR;
      break;

    default:
      return SNMP_ERR_GENERR;
    }
  return SNMP_ERR_NOERROR;
}

static int
write_mplsOutSegmentEntry (int action, u_char *var_val,
                           u_char var_val_type, size_t var_val_len,
                           u_char *statP, oid *name, size_t length,
                           struct variable *v, u_int32_t vr_id)
{
  long intval = 0;
  struct pal_in4_addr addr;
  size_t val_len;

  NSM_MPLS_SNMP_UNION(mpls_out_segment_table_index, index);
  struct nsm_master *nm = nsm_master_lookup_by_id (nzg, 0);

  if (!nm)
    return SNMP_ERR_GENERR;
  /* Get table indices. */
  if (! nsm_mpls_snmp_index_get (v, name, &length, 
                                 &index.snmp,
                                 MPLS_OUT_SEG_TABLE, 1)) /* exact = 1 */
    return SNMP_ERR_GENERR; 

  pal_mem_set(&addr, 0, sizeof (struct pal_in4_addr));
  if (var_val_type == ASN_INTEGER)
    pal_mem_cpy (&intval, var_val, 4);

  else if (var_val_type == ASN_OBJECT_ID)
    {
      val_len = pal_strlen (var_val);

#ifndef HAVE_AGENTX
      if (val_len != sizeof (null_oid))         /* Null OID */
        return SNMP_ERR_WRONGLENGTH;
#endif /* HAVE_AGENTX */

      if (* (var_val + 2))              /* Only NULL Oid accepted */
        return SNMP_ERR_GENERR;
    }

  else if (var_val_type == ASN_IPADDRESS)
    pal_mem_cpy (&addr.s_addr, var_val, 4);
    
  else    /* Not Integer, ObjectId or IP Address */
    return SNMP_ERR_WRONGTYPE;

  switch (v->magic)
    {
    case MPLS_OUT_SEG_IF_IX:
      if (!(nsm_mpls_set_outseg_if_ix (nm, index.table.out_seg_ix, intval)))
        return SNMP_ERR_GENERR;
      break;

    case MPLS_OUT_SEG_PUSH_TOP_LB:
      if (!(nsm_mpls_set_outseg_push_top_lb (nm, index.table.out_seg_ix, intval)))
        return SNMP_ERR_GENERR;
      break;

    case MPLS_OUT_SEG_TOP_LB:
      if (!(nsm_mpls_set_outseg_top_lb (nm, index.table.out_seg_ix, intval)))
        return SNMP_ERR_GENERR;
      break;

    case MPLS_OUT_SEG_TOP_LB_PTR:
      if (!(nsm_mpls_set_outseg_top_lb_ptr (nm, index.table.out_seg_ix)))
        return SNMP_ERR_GENERR;
      break;

    case MPLS_OUT_SEG_NXT_HOP_IPA_TYPE:
      if (!(nsm_mpls_set_outseg_nxt_hop_ipa_type (nm, index.table.out_seg_ix, 
                                                  intval)))
        return SNMP_ERR_GENERR;
      break;

    case MPLS_OUT_SEG_NXT_HOP_IPA:
      if (!(nsm_mpls_set_outseg_nxt_hop_ipa (nm, index.table.out_seg_ix, 
                                             var_val_len, addr)))
        return SNMP_ERR_GENERR;
      break;

    case MPLS_OUT_SEG_TF_PRM:
      if (!(nsm_mpls_set_outseg_tf_prm (nm, index.table.out_seg_ix)))
        return SNMP_ERR_GENERR;
      break;

    case MPLS_OUT_SEG_ROW_STATUS:
      if (!(nsm_mpls_set_outseg_row_status (nm, index.table.out_seg_ix, intval)))
        return SNMP_ERR_GENERR;
      break;

    case MPLS_OUT_SEG_ST_TYPE:
      if (!(nsm_mpls_set_outseg_st_type (nm, index.table.out_seg_ix, intval)))
        return SNMP_ERR_GENERR;
      break;

    default:
      return SNMP_ERR_GENERR;
    }

  return SNMP_ERR_NOERROR;
}

static int
write_mplsXCEntry (int action, u_char *var_val,
                   u_char var_val_type, size_t var_val_len,
                   u_char *statP, oid *name, size_t length,
                   struct variable *v, u_int32_t vr_id)
{
  long intval;
  
  NSM_MPLS_SNMP_UNION(mpls_XC_table_index, index);
  struct nsm_master *nm = nsm_master_lookup_by_id (nzg, 0);

  if (!nm)
    return SNMP_ERR_GENERR;

  pal_mem_set (&index, 0, sizeof (struct mpls_XC_table_index));

  /* Get table indices. */
  if (! nsm_mpls_snmp_index_get (v, name, &length, 
                                 &index.snmp,
                                 MPLS_XC_TABLE, 1)) /* exact = 1 */
    return SNMP_ERR_GENERR; 

  if (var_val_type != ASN_INTEGER)
    return SNMP_ERR_WRONGTYPE;

  pal_mem_cpy (&intval, var_val, 4);

  switch (v->magic)
    {
    case MPLS_XC_LSPID:
      if (intval)
        return SNMP_ERR_BADVALUE;
      if (!(nsm_mpls_set_xc_lspid (nm, index.table.xc_ix, index.table.in_seg_ix,
                                   index.table.out_seg_ix)))
        return SNMP_ERR_GENERR;
      break;

    case MPLS_XC_LB_STK_IX:
      if (!(nsm_mpls_set_xc_lb_stk_ix (nm, index.table.xc_ix, index.table.in_seg_ix,
                                       index.table.out_seg_ix, intval)))
        return SNMP_ERR_GENERR;
      break;

    case MPLS_XC_ROW_STATUS:
      if (!(nsm_mpls_set_xc_row_status (nm, index.table.xc_ix, index.table.in_seg_ix,
                                        index.table.out_seg_ix, intval)))
        return SNMP_ERR_GENERR;
      break;

    case MPLS_XC_ST_TYPE:
      if (!(nsm_mpls_set_xc_st_type (nm, index.table.xc_ix, index.table.in_seg_ix,
                                     index.table.out_seg_ix, intval)))
        return SNMP_ERR_GENERR;
      break;

    case MPLS_XC_ADM_STATUS:
      if (!(nsm_mpls_set_xc_adm_status (nm, index.table.xc_ix, index.table.in_seg_ix,
                                        index.table.out_seg_ix, intval)))
        return SNMP_ERR_GENERR;
      break;

    default:
      return SNMP_ERR_GENERR;
    }
  return SNMP_ERR_NOERROR;
}

static int
write_mplsFTNEntry (int action, u_char *var_val,
                    u_char var_val_type, size_t var_val_len,
                    u_char *statP, oid *name, size_t length,
                    struct variable *v, u_int32_t vr_id)
{
  long intval = 0;
  u_int32_t act_ptr_xc_ix, act_ptr_ilm_ix, act_ptr_nhlfe_ix;
  char descr[NSM_MPLS_MAX_STRING_LENGTH];
  struct pal_in4_addr addr;
  size_t val_len = 0;
  
  NSM_MPLS_SNMP_UNION(mpls_FTN_table_index, index);
  struct nsm_master *nm = nsm_master_lookup_by_id (nzg, 0);

  if (!nm)
    return SNMP_ERR_GENERR;

  pal_mem_set (&addr, 0, sizeof (struct pal_in4_addr));

  /* Get table indices. */
  if (! nsm_mpls_snmp_index_get (v, name, &length, 
                                 &index.snmp,
                                 MPLS_FTN_TABLE, 1)) /* exact = 1 */
    return SNMP_ERR_GENERR; 

  if (var_val_type == ASN_INTEGER)
    pal_mem_cpy (&intval, var_val, 4);

  else if (var_val_type == ASN_OCTET_STR)  /* OBJECT_ID also included */
    val_len = pal_strlen (var_val);

  else if (var_val_type == ASN_IPADDRESS)
    pal_mem_cpy (&addr.s_addr, var_val, 4);

  else  /* Not any of Integer, String or IP Address */
    return SNMP_ERR_WRONGTYPE;

  switch (v->magic)
    {
    case MPLS_FTN_ROW_STATUS:
      if (!(nsm_gmpls_set_ftn_row_status (nm, index.table.ftn_ix, intval)))
        return SNMP_ERR_GENERR;
      break;

    case MPLS_FTN_DESCR:        /* String */
      pal_mem_cpy (descr, var_val, val_len);
      descr[var_val_len] = '\0';

      if (!(nsm_mpls_set_ftn_descr (nm, index.table.ftn_ix, descr)))
        return SNMP_ERR_GENERR;
      break;

    case MPLS_FTN_MASK:
      if (!(nsm_mpls_set_ftn_mask (nm, index.table.ftn_ix, intval)))
        return SNMP_ERR_GENERR;
      break;

    case MPLS_FTN_ADDR_TYPE:
      if (!(nsm_mpls_set_ftn_addr_type (nm, index.table.ftn_ix, intval)))
        return SNMP_ERR_GENERR;
      break;

    case MPLS_FTN_SRC_ADDR_MIN: /* INet Addr */
      if (!(nsm_mpls_set_ftn_src_addr_min_max (nm, index.table.ftn_ix, &addr)))
        return SNMP_ERR_GENERR;
      break;

    case MPLS_FTN_SRC_ADDR_MAX: /* INet Addr */
      if (!(nsm_mpls_set_ftn_src_addr_min_max (nm, index.table.ftn_ix, &addr)))
        return SNMP_ERR_GENERR;
      break;

    case MPLS_FTN_DST_ADDR_MIN: /* INet Addr */
      if (!(nsm_mpls_set_ftn_dst_addr_min_max (nm, index.table.ftn_ix, &addr)))
        return SNMP_ERR_GENERR;
      break;

    case MPLS_FTN_DST_ADDR_MAX: /* INet Addr */
      if (!(nsm_mpls_set_ftn_dst_addr_min_max (nm, index.table.ftn_ix, &addr)))
        return SNMP_ERR_GENERR;
      break;

    case MPLS_FTN_SRC_PORT_MIN:
      if (!(nsm_mpls_set_ftn_src_dst_port_min (nm, index.table.ftn_ix, intval)))
        return SNMP_ERR_GENERR;
      break;

    case MPLS_FTN_SRC_PORT_MAX:
      if (!(nsm_mpls_set_ftn_src_dst_port_max (nm, index.table.ftn_ix, intval)))
        return SNMP_ERR_GENERR;
      break;

    case MPLS_FTN_DST_PORT_MIN:
      if (!(nsm_mpls_set_ftn_src_dst_port_min (nm, index.table.ftn_ix, intval)))
        return SNMP_ERR_GENERR;
      break;

    case MPLS_FTN_DST_PORT_MAX:
      if (!(nsm_mpls_set_ftn_src_dst_port_max (nm, index.table.ftn_ix, intval)))
        return SNMP_ERR_GENERR;
      break;

    case MPLS_FTN_PROTOCOL:
      if (!(nsm_mpls_set_ftn_protocol (nm, index.table.ftn_ix, intval)))
        return SNMP_ERR_GENERR;
      break;

    case MPLS_FTN_DSCP: /* DSCP */
      if (!(nsm_mpls_set_ftn_dscp (nm, index.table.ftn_ix, intval)))
        return SNMP_ERR_GENERR;
      break;

    case MPLS_FTN_ACT_TYPE:
      if (!(nsm_mpls_set_ftn_act_type (nm, index.table.ftn_ix, intval)))
        return SNMP_ERR_GENERR;
      break;

    case MPLS_FTN_ACT_POINTER:  /* Row Pointer */
      if (val_len  >= ACT_PTR_NHLFE_IX_POS)
        {
          act_ptr_xc_ix = *(var_val + ACT_PTR_XC_IX_POS) - ACT_PTR_NULL;
          act_ptr_ilm_ix = *(var_val + ACT_PTR_ILM_IX_POS) - ACT_PTR_NULL;
          act_ptr_nhlfe_ix = *(var_val + ACT_PTR_NHLFE_IX_POS) - ACT_PTR_NULL;
        }
      else if (val_len == NULL_OID_LEN)
        {
          /* NULL OID should be 0.0 */
          act_ptr_xc_ix = *(var_val) - ACT_PTR_NULL;
          act_ptr_nhlfe_ix = *(var_val+2) - ACT_PTR_NULL;
          act_ptr_ilm_ix = 0;
        }
       else
        return SNMP_ERR_GENERR;
      if (!(nsm_mpls_set_ftn_act_pointer (nm, index.table.ftn_ix, act_ptr_xc_ix, 
                                          act_ptr_ilm_ix, act_ptr_nhlfe_ix)))
        return SNMP_ERR_GENERR;
      break;

    case MPLS_FTN_ST_TYPE:
      if (!(nsm_mpls_set_ftn_st_type (nm, index.table.ftn_ix, intval)))
        return SNMP_ERR_GENERR;
      break;

    default:
      return SNMP_ERR_GENERR;
    }

  last_changed_time = pal_time_current (NULL);
  return SNMP_ERR_NOERROR;
}

/* SMUX callback functions. */

/* MPLS Scalars */
static u_char *
mplsScalars (struct variable *v, oid *name, size_t *length, 
             int exact, size_t *var_len, WriteMethod **write_method, 
             u_int32_t vr_id)
{
  struct nsm_master *nm = nsm_master_lookup_by_id (nzg, 0);
  struct nsm_mpls *nmpls = NULL;
  static int notify;
  int ret;

  if (!nm)
    return NULL;
  
  nmpls = nm->nmpls;
  if (snmp_header_generic (v, name, length, exact, var_len, write_method)
      == MATCH_FAILED)
    return NULL;
  
  switch (v->magic)
    {
      case MPLS_IN_SEG_IX_NXT:
        ret = bitmap_request_index (ILM_IX_MGR, &nsm_mpls_snmp_int_val);

        /* Release Index so that bitmap can reuse this index for allocation */
        bitmap_release_index (ILM_IX_MGR, nsm_mpls_snmp_int_val);

        return (u_char *)&nsm_mpls_snmp_int_val;
        break;

      case MPLS_OUT_SEG_IX_NXT:
        ret = bitmap_request_index (NHLFE_IX_MGR, &nsm_mpls_snmp_int_val);

        /* Release Index so that bitmap can reuse this index for allocation */
        bitmap_release_index (NHLFE_IX_MGR, nsm_mpls_snmp_int_val);

        return (u_char *)&nsm_mpls_snmp_int_val;
        break;

      case MPLS_XC_IX_NXT:
        ret = bitmap_request_index (XC_IX_MGR, &nsm_mpls_snmp_int_val);

        /* Release Index so that bitmap can reuse this index for allocation */
        bitmap_release_index (XC_IX_MGR, nsm_mpls_snmp_int_val);

        return (u_char *)&nsm_mpls_snmp_int_val;
        break;

      case MPLS_XC_NOTIFN_CNTL:
        *write_method = write_mplsScalars;
        if (NSM_MPLS_SNMP_GET (notify, &notify))
          return (u_char *) &notify;
        else
          return 0;
        break;

      default:
        return NULL;
        break;
    }
  return NULL;
}

/* MPLS Interface Entry */
static u_char *
mplsInterfaceConfEntry (struct variable *v, oid *name, size_t *length, 
                        u_int32_t exact, size_t *var_len,
                        WriteMethod **write_method, u_int32_t vr_id)
{
  NSM_MPLS_SNMP_UNION(mpls_interface_conf_table_index, index);
  struct nsm_master *nm = nsm_master_lookup_by_id (nzg, 0);

  if (!nm)
    return NULL;

  nsm_mpls_snmp_int_val = NSM_MPLS_SNMP_VALUE_INVALID;
 
  /* Get table indices. */
  if (! nsm_mpls_snmp_index_get (v, name, length, 
                                 &index.snmp,
                                 MPLS_IF_CONFIG_TABLE, exact))
    return NULL; 

  if (index.table.len == 0)
    index.table.if_ix = 0;
  
  /* Create entry of index=0 which contains general parameters of all 
     per platform label space entries. */
  if ((index.table.len == 0) && (v->magic !=  MPLS_IF_TOTAL_BW) &&
      (v->magic != MPLS_IF_AVA_BW)) 
    {
      index.table.if_ix = 0;
      nsm_gmpls_snmp_index_set (v, name, length, 
                               (struct nsm_mpls_snmp_index *)&index, 
                               MPLS_IF_CONFIG_TABLE);
      *var_len = sizeof (u_int32_t); 
      switch (v->magic)
        {
        case MPLS_IF_LB_MIN_IN:
          nsm_mpls_snmp_int_val = LABEL_VALUE_INITIAL;
          break;
      
        case MPLS_IF_LB_MAX_IN:
          nsm_mpls_snmp_int_val = LABEL_VALUE_MAX;
          break;

        case MPLS_IF_LB_PT_TYPE:
          nsm_mpls_snmp_int_val = NSM_MPLS_SNMP_LBSP_PLATFORM;
          break;

#if (!defined (HAVE_GMPLS) || defined (HAVE_PACKET))
        case MPLS_IF_LB_MIN_OUT:
          nsm_mpls_snmp_int_val = nm->nmpls->min_label_val;
          break;

        case MPLS_IF_LB_MAX_OUT:
          nsm_mpls_snmp_int_val = nm->nmpls->max_label_val;
          break;
#endif /* !HAVE_GMPLS || HAVE_PACKET */
        default:
          return NULL;
        }
      return (u_char *)&nsm_mpls_snmp_int_val;
    }   

  
  /* Return the current value of the variable. */
  switch (v->magic)
    {
    case MPLS_IF_CONF_IX:
      if (! exact)
        {
          index.table.if_ix =  0;
          nsm_gmpls_snmp_index_set (v, name, length,
                                   (struct nsm_mpls_snmp_index *)&index, 
                                   MPLS_IF_CONFIG_TABLE);
        }
      return NULL;
      break;

    case MPLS_IF_LB_MIN_IN:
      if (NSM_MPLS_SNMP_GET_NEXT (if_lb_min_in, index.table.if_ix, 
                                  &nsm_mpls_snmp_int_val))
        NSM_MPLS_SNMP_RETURN (nsm_mpls_snmp_int_val, MPLS_IF_CONFIG_TABLE);
      break;

    case MPLS_IF_LB_MAX_IN:
      if (NSM_MPLS_SNMP_GET_NEXT (if_lb_max_in, index.table.if_ix, 
                                  &nsm_mpls_snmp_int_val))
        NSM_MPLS_SNMP_RETURN (nsm_mpls_snmp_int_val, MPLS_IF_CONFIG_TABLE);
      break;

    case MPLS_IF_LB_MIN_OUT:
      if (NSM_MPLS_SNMP_GET_NEXT (if_lb_min_out, index.table.if_ix, 
                                  &nsm_mpls_snmp_int_val))
        NSM_MPLS_SNMP_RETURN (nsm_mpls_snmp_int_val, MPLS_IF_CONFIG_TABLE);
      break;

    case MPLS_IF_LB_MAX_OUT:
      if (NSM_MPLS_SNMP_GET_NEXT (if_lb_max_out, index.table.if_ix, 
                                  &nsm_mpls_snmp_int_val))
        NSM_MPLS_SNMP_RETURN (nsm_mpls_snmp_int_val, MPLS_IF_CONFIG_TABLE);
      break;

    case MPLS_IF_TOTAL_BW:
      if (NSM_MPLS_SNMP_GET_NEXT (if_total_bw, index.table.if_ix, 
                                  &nsm_mpls_snmp_int_val))
        NSM_MPLS_SNMP_RETURN (nsm_mpls_snmp_int_val, MPLS_IF_CONFIG_TABLE);
      break;

    case MPLS_IF_AVA_BW:
      if (NSM_MPLS_SNMP_GET_NEXT (if_ava_bw, index.table.if_ix, 
                                  &nsm_mpls_snmp_int_val))
        NSM_MPLS_SNMP_RETURN (nsm_mpls_snmp_int_val, MPLS_IF_CONFIG_TABLE);
      break;

    case MPLS_IF_LB_PT_TYPE:
      if (NSM_MPLS_SNMP_GET_NEXT (if_lb_pt_type, index.table.if_ix, 
                                  &nsm_mpls_snmp_int_val))
        NSM_MPLS_SNMP_RETURN (nsm_mpls_snmp_int_val, MPLS_IF_CONFIG_TABLE);
      break;

    default:
      return NULL;
    }
  return NULL;
}

/* MPLS Interface Performance Entry */
static u_char *
mplsInterfacePerfEntry (struct variable *v, oid *name, size_t *length, 
                        u_int32_t exact, size_t *var_len,
                        WriteMethod **write_method, u_int32_t vr_id)
{
  NSM_MPLS_SNMP_UNION(mpls_interface_conf_table_index, index);
  struct nsm_master *nm = nsm_master_lookup_by_id (nzg, 0);

  if (!nm)
    return NULL;

  nsm_mpls_snmp_int_val = NSM_MPLS_SNMP_VALUE_INVALID;
  
  /* Get table indices. */
  if (! nsm_mpls_snmp_index_get (v, name, length, 
                                 &index.snmp,
                                 MPLS_IF_PERF_TABLE, exact))
    return NULL; 

  if (index.table.len == 0)
    index.table.if_ix = 0;

  /* Return the current value of the variable. */
  switch (v->magic)
    {
    case MPLS_IF_IN_LB_USED:
      if (NSM_MPLS_SNMP_GET_NEXT (if_in_lb_used, index.table.if_ix, 
                                  &nsm_mpls_snmp_int_val))
        NSM_MPLS_SNMP_RETURN (nsm_mpls_snmp_int_val, MPLS_IF_PERF_TABLE);
      break;

    case MPLS_IF_FAILED_LB_LOOKUP:
      if (NSM_MPLS_SNMP_GET_NEXT (if_failed_lb_lookup, index.table.if_ix, 
                                  &nsm_mpls_snmp_int_val))
        NSM_MPLS_SNMP_RETURN (nsm_mpls_snmp_int_val, MPLS_IF_PERF_TABLE);
      break;
      
    case MPLS_IF_OUT_LB_USED:
      if (NSM_MPLS_SNMP_GET_NEXT (if_out_lb_used, index.table.if_ix, 
                                  &nsm_mpls_snmp_int_val))
        NSM_MPLS_SNMP_RETURN (nsm_mpls_snmp_int_val, MPLS_IF_PERF_TABLE);
      break;
      
    case MPLS_IF_OUT_FRAGMENTS:
      if (NSM_MPLS_SNMP_GET_NEXT (if_out_fragments, index.table.if_ix, 
                                  &nsm_mpls_snmp_int_val))
        NSM_MPLS_SNMP_RETURN (nsm_mpls_snmp_int_val, MPLS_IF_PERF_TABLE);
      break;

    default:
      return NULL;
    }  
  return NULL;
}

/* MPLS In Segment Entry */
static u_char *
mplsInSegmentEntry (struct variable *v, oid *name, size_t *length, 
                    u_int32_t exact, size_t *var_len,
                    WriteMethod **write_method, u_int32_t vr_id)
{
  NSM_MPLS_SNMP_UNION(mpls_in_segment_table_index, index);
  struct nsm_master *nm = nsm_master_lookup_by_id (nzg, 0);

  if (!nm)
    return NULL;
  
  nsm_mpls_snmp_int_val = NSM_MPLS_SNMP_VALUE_INVALID;
  nsm_mpls_snmp_char_ptr[0] = '\0';
  pal_mem_set (&index, 0, sizeof (struct mpls_in_segment_table_index));
  
  /* Get table indices. */
  if (! nsm_mpls_snmp_index_get (v, name, length, 
                                 &index.snmp,
                                 MPLS_IN_SEG_TABLE, exact))
    return NULL; 

  if (index.table.len == 0)
    index.table.in_seg_ix =  0; /* Index of Table is only InSegmentIndex */
    
  switch (v->magic)
    { 
    case MPLS_IN_SEG_IX:
      if (! exact)
        {
          index.table.in_seg_ix =  0;
          nsm_gmpls_snmp_index_set (v, name, length,
                                   (struct nsm_mpls_snmp_index *)&index, 
                                   MPLS_IN_SEG_TABLE);
        }
      return NULL;
      break;

    case MPLS_IN_SEG_IF:
      *write_method = write_mplsInSegmentEntry;
      if (NSM_MPLS_SNMP_GET_NEXT (inseg_if, index.table.in_seg_ix, 
                                  &nsm_mpls_snmp_int_val)) 
        NSM_MPLS_SNMP_RETURN (nsm_mpls_snmp_int_val, MPLS_IN_SEG_TABLE);
      break;

    case MPLS_IN_SEG_LB:
      *write_method = write_mplsInSegmentEntry;
      if (NSM_MPLS_SNMP_GET_NEXT (inseg_lb, index.table.in_seg_ix, 
                                  &nsm_mpls_snmp_int_val)) 
        NSM_MPLS_SNMP_RETURN (nsm_mpls_snmp_int_val, MPLS_IN_SEG_TABLE);
      break;

    case MPLS_IN_SEG_LB_PTR:
      *write_method = write_mplsInSegmentEntry;
      if (NSM_MPLS_SNMP_GET_NEXT (inseg_lb_ptr, index.table.in_seg_ix, 
                                  &nsm_mpls_snmp_char_ptr)) 
        NSM_MPLS_SNMP_RETURN_CHAR (nsm_mpls_snmp_char_ptr, MPLS_IN_SEG_TABLE);
      break;

    case MPLS_IN_SEG_NPOP:
      *write_method = write_mplsInSegmentEntry;
      if (NSM_MPLS_SNMP_GET_NEXT (inseg_npop, index.table.in_seg_ix, 
                                  &nsm_mpls_snmp_int_val)) 
        NSM_MPLS_SNMP_RETURN (nsm_mpls_snmp_int_val, MPLS_IN_SEG_TABLE);
      break;
      
    case MPLS_IN_SEG_ADDR_FAMILY:
      *write_method = write_mplsInSegmentEntry;
      if (NSM_MPLS_SNMP_GET_NEXT (inseg_addr_family, index.table.in_seg_ix, 
                                  &nsm_mpls_snmp_int_val))
        NSM_MPLS_SNMP_RETURN (nsm_mpls_snmp_int_val, MPLS_IN_SEG_TABLE);
      break;
      
    case MPLS_IN_SEG_XC_IX:
      /* Read Only Object */
      if (NSM_MPLS_SNMP_GET_NEXT (inseg_xc_ix, index.table.in_seg_ix, 
                                  &nsm_mpls_snmp_int_val))
        NSM_MPLS_SNMP_RETURN (nsm_mpls_snmp_int_val, MPLS_IN_SEG_TABLE);
      break;

    case MPLS_IN_SEG_OWNER:
      /* Read Only Object */
      if (NSM_MPLS_SNMP_GET_NEXT (inseg_owner, index.table.in_seg_ix, 
                                  &nsm_mpls_snmp_int_val))
        NSM_MPLS_SNMP_RETURN (nsm_mpls_snmp_int_val, MPLS_IN_SEG_TABLE);
      break;

    case MPLS_IN_SEG_TF_PRM:
      *write_method = write_mplsInSegmentEntry;
      if (NSM_MPLS_SNMP_GET_NEXT (inseg_tf_prm, index.table.in_seg_ix, 
                                  &nsm_mpls_snmp_char_ptr))
        NSM_MPLS_SNMP_RETURN_CHAR (nsm_mpls_snmp_char_ptr, MPLS_IN_SEG_TABLE);
      break;

    case MPLS_IN_SEG_ROW_STATUS:
      *write_method = write_mplsInSegmentEntry;
      if (NSM_MPLS_SNMP_GET_NEXT (inseg_row_status, index.table.in_seg_ix, 
                                  &nsm_mpls_snmp_int_val))
        NSM_MPLS_SNMP_RETURN (nsm_mpls_snmp_int_val, MPLS_IN_SEG_TABLE);
      break;

    case MPLS_IN_SEG_ST_TYPE:
      *write_method = write_mplsInSegmentEntry;
      if (NSM_MPLS_SNMP_GET_NEXT (inseg_st_type, index.table.in_seg_ix, 
                                  &nsm_mpls_snmp_int_val))
        NSM_MPLS_SNMP_RETURN (nsm_mpls_snmp_int_val, MPLS_IN_SEG_TABLE);
      break;

    default:
      return NULL;
      break;
 
    }
  return NULL;
}

/* MPLS In Segment Performance Entry */
static u_char *
mplsInSegmentPerfEntry (struct variable *v, oid *name, size_t *length, 
                        u_int32_t exact, size_t *var_len,
                        WriteMethod **write_method, u_int32_t vr_id)
{
  NSM_MPLS_SNMP_UNION(mpls_in_segment_table_index, index);
  struct nsm_master *nm = nsm_master_lookup_by_id (nzg, 0);

  if (!nm)
    return NULL;

  nsm_mpls_snmp_int_val = NSM_MPLS_SNMP_VALUE_INVALID;
  pal_mem_set (&nsm_mpls_snmp_int_hc_val, 0, sizeof (ut_int64_t));
  
  /* Get table indices. */
  if (! nsm_mpls_snmp_index_get (v, name, length, 
                                 &index.snmp,
                                 MPLS_IN_SEG_PERF_TABLE, exact))
    return NULL; 

  if (index.table.len == 0)
    index.table.in_seg_ix =  0;
  
  switch (v->magic)
    {
    case MPLS_IN_SEG_OCTS:
      if (NSM_MPLS_SNMP_GET_NEXT (inseg_octs, index.table.in_seg_ix, 
                                  &nsm_mpls_snmp_int_val))
        NSM_MPLS_SNMP_RETURN (nsm_mpls_snmp_int_val, MPLS_IN_SEG_PERF_TABLE);
      break;
      
    case MPLS_IN_SEG_PKTS:
      if (NSM_MPLS_SNMP_GET_NEXT (inseg_pkts, index.table.in_seg_ix, 
                                  &nsm_mpls_snmp_int_val))
        NSM_MPLS_SNMP_RETURN (nsm_mpls_snmp_int_val, MPLS_IN_SEG_PERF_TABLE);
      break;
      
    case MPLS_IN_SEG_ERRORS:
      if (NSM_MPLS_SNMP_GET_NEXT (inseg_errors, index.table.in_seg_ix,
                                  &nsm_mpls_snmp_int_val))
        NSM_MPLS_SNMP_RETURN (nsm_mpls_snmp_int_val, MPLS_IN_SEG_PERF_TABLE);
      break;
      
    case MPLS_IN_SEG_DISCARDS:
      if (NSM_MPLS_SNMP_GET_NEXT (inseg_discards, index.table.in_seg_ix, 
                                  &nsm_mpls_snmp_int_val))
        NSM_MPLS_SNMP_RETURN (nsm_mpls_snmp_int_val, MPLS_IN_SEG_PERF_TABLE);
      break;
       
    case MPLS_IN_SEG_HC_OCTS:
      if (NSM_MPLS_SNMP_GET_NEXT (inseg_hc_octs, index.table.in_seg_ix, 
                                  &nsm_mpls_snmp_int_hc_val))
        NSM_MPLS_SNMP_RETURN (nsm_mpls_snmp_int_hc_val, 
                              MPLS_IN_SEG_PERF_TABLE);
      break;
      
    case MPLS_IN_SEG_PERF_DIS_TIME:
      if (NSM_MPLS_SNMP_GET_NEXT (inseg_perf_dis_time, index.table.in_seg_ix, 
                                  &nsm_mpls_snmp_time_val))
        NSM_MPLS_SNMP_RETURN (nsm_mpls_snmp_time_val, MPLS_IN_SEG_PERF_TABLE);
      break;
      
    default:
      return NULL;
    }
  return NULL;
}

/* MPLS In Segment Map Entry */
static u_char *
mplsInSegMapEntry (struct variable *v, oid *name, size_t *length, 
                   u_int32_t exact, size_t *var_len,
                   WriteMethod **write_method, u_int32_t vr_id)
{
  NSM_MPLS_SNMP_UNION(mpls_in_seg_map_table_index, index);
  struct nsm_master *nm = nsm_master_lookup_by_id (nzg, 0);

  if (!nm)
    return NULL;
  
  nsm_mpls_snmp_int_val = NSM_MPLS_SNMP_VALUE_INVALID;
  nsm_mpls_snmp_char_ptr[0] = '\0';
  pal_mem_set (&index, 0, sizeof (struct mpls_in_seg_map_table_index));
  
  /* Get table indices. */
  if (! nsm_mpls_snmp_index_get (v, name, length, 
                                 &index.snmp,
                                 MPLS_IN_SEG_MAP_TABLE, exact))
    return NULL; 

  if (index.table.len == 0)
    {
      index.table.in_seg_map_if_ix =  0;
      index.table.in_seg_map_label = 0;
      index.table.in_seg_map_lb_ptr = 0;
    }  

  switch (v->magic)
    { 
    case MPLS_IN_SEG_MAP_IF:
    case MPLS_IN_SEG_MAP_LB:
    case MPLS_IN_SEG_MAP_LB_PTR:

      if (! exact)
        {
          index.table.in_seg_map_if_ix =  0;
          index.table.in_seg_map_label =  0;
          index.table.in_seg_map_lb_ptr =  0;

          nsm_gmpls_snmp_index_set (v, name, length,
                                   (struct nsm_mpls_snmp_index *)&index, 
                                   MPLS_IN_SEG_MAP_TABLE);
        }
      return NULL;
      break;

    case MPLS_IN_SEG_MAP_IX:
      /* Read Only Object */
      if (NSM_MPLS_SNMP_GET2NEXT (inseg_map_ix, index.table.in_seg_map_if_ix,
                                  index.table.in_seg_map_label, 
                                  (index.table.len < 2) ? \
                                  sizeof(u_int32_t) * 8 : \
                                  sizeof(u_int32_t) * 8 + MAX_LABEL_BITLEN, 
                                  &nsm_mpls_snmp_int_val)) 
        NSM_MPLS_SNMP_RETURN (nsm_mpls_snmp_int_val, MPLS_IN_SEG_MAP_TABLE);
      break;

    default:
      return NULL;
      break;
 
    }
  return NULL;
}

/* MPLS Out Segment Entry */
static u_char *
mplsOutSegmentEntry (struct variable *v, oid *name, size_t *length, 
                     u_int32_t exact, size_t *var_len,
                     WriteMethod **write_method, u_int32_t vr_id)
{
  NSM_MPLS_SNMP_UNION(mpls_out_segment_table_index, index);
  struct nsm_master *nm = nsm_master_lookup_by_id (nzg, 0);
  
  if(!nm)
    return NULL;

  nsm_mpls_snmp_int_val = NSM_MPLS_SNMP_VALUE_INVALID;
  nsm_mpls_snmp_char_ptr[0] = '\0';
  
  /* Get table indices. */
  if (! nsm_mpls_snmp_index_get (v, name, length, 
                                 &index.snmp,
                                 MPLS_OUT_SEG_TABLE, exact))
    return NULL; 

  if (index.table.len == 0)
    index.table.out_seg_ix = 0;
  
  switch (v->magic)
    {
    case MPLS_OUT_SEG_IX:
      if (! exact)
        {
          index.table.out_seg_ix = 0;
          nsm_gmpls_snmp_index_set (v, name, length,
                                   (struct nsm_mpls_snmp_index *)&index, 
                                   MPLS_OUT_SEG_TABLE);
        }
      return NULL;
      break;

    case MPLS_OUT_SEG_IF_IX:
      *write_method = write_mplsOutSegmentEntry;
      if (NSM_MPLS_SNMP_GET_NEXT (outseg_if_ix, index.table.out_seg_ix, 
                                  &nsm_mpls_snmp_int_val))
        NSM_MPLS_SNMP_RETURN (nsm_mpls_snmp_int_val, MPLS_OUT_SEG_TABLE);
      break;
      
    case MPLS_OUT_SEG_PUSH_TOP_LB:
      *write_method = write_mplsOutSegmentEntry;
      if (NSM_MPLS_SNMP_GET_NEXT (outseg_push_top_lb, index.table.out_seg_ix, 
                                  &nsm_mpls_snmp_int_val))
        NSM_MPLS_SNMP_RETURN (nsm_mpls_snmp_int_val, MPLS_OUT_SEG_TABLE);
      break;

    case MPLS_OUT_SEG_TOP_LB:
      *write_method = write_mplsOutSegmentEntry;
      if (NSM_MPLS_SNMP_GET_NEXT (outseg_top_lb, index.table.out_seg_ix, 
                                  &nsm_mpls_snmp_int_val))
        NSM_MPLS_SNMP_RETURN (nsm_mpls_snmp_int_val, MPLS_OUT_SEG_TABLE);
      break;

    case MPLS_OUT_SEG_TOP_LB_PTR:
      *write_method = write_mplsOutSegmentEntry;
      if (NSM_MPLS_SNMP_GET_NEXT (outseg_top_lb_ptr, index.table.out_seg_ix, 
                                  &nsm_mpls_snmp_char_ptr))
        NSM_MPLS_SNMP_RETURN_CHAR (nsm_mpls_snmp_char_ptr, 
                                   MPLS_OUT_SEG_TABLE); 
      break;

    case MPLS_OUT_SEG_NXT_HOP_IPA_TYPE:
      *write_method = write_mplsOutSegmentEntry;
      if (NSM_MPLS_SNMP_GET_NEXT (outseg_nxt_hop_ipa_type, index.table.out_seg_ix, 
                                  &nsm_mpls_snmp_int_val))
        NSM_MPLS_SNMP_RETURN (nsm_mpls_snmp_int_val, MPLS_OUT_SEG_TABLE);
      break;
       
    case MPLS_OUT_SEG_NXT_HOP_IPA:
      *write_method = write_mplsOutSegmentEntry;
      if (NSM_MPLS_SNMP_GET_NEXT (outseg_nxt_hop_ipa, index.table.out_seg_ix, 
                                  &nsm_mpls_snmp_char_ptr))
        NSM_MPLS_SNMP_RETURN_CHAR (nsm_mpls_snmp_char_ptr, 
                                   MPLS_OUT_SEG_TABLE); 
        
      break;
      
    case MPLS_OUT_SEG_XC_IX:
      /* Read Only Object */
      if (NSM_MPLS_SNMP_GET_NEXT (outseg_xc_ix, index.table.out_seg_ix, 
                                  &nsm_mpls_snmp_int_val))
        NSM_MPLS_SNMP_RETURN (nsm_mpls_snmp_int_val, MPLS_OUT_SEG_TABLE);
      break;
      
    case MPLS_OUT_SEG_OWNER:
      /* Read Only Object */
      if (NSM_MPLS_SNMP_GET_NEXT (outseg_owner, index.table.out_seg_ix, 
                                  &nsm_mpls_snmp_int_val))
        NSM_MPLS_SNMP_RETURN (nsm_mpls_snmp_int_val, MPLS_OUT_SEG_TABLE);
      break;

    case MPLS_OUT_SEG_TF_PRM:
      *write_method = write_mplsOutSegmentEntry;
      if (NSM_MPLS_SNMP_GET_NEXT (outseg_tf_prm, index.table.out_seg_ix, 
                                  &nsm_mpls_snmp_char_ptr))
        NSM_MPLS_SNMP_RETURN_CHAR (nsm_mpls_snmp_char_ptr, 
                                   MPLS_OUT_SEG_TABLE); 
      break;
      
    case MPLS_OUT_SEG_ROW_STATUS:
      *write_method = write_mplsOutSegmentEntry;
      if (NSM_MPLS_SNMP_GET_NEXT (outseg_row_status, index.table.out_seg_ix, 
                                  &nsm_mpls_snmp_int_val))
        NSM_MPLS_SNMP_RETURN (nsm_mpls_snmp_int_val, MPLS_OUT_SEG_TABLE);
      break;

    case MPLS_OUT_SEG_ST_TYPE:
      *write_method = write_mplsOutSegmentEntry;
      if (NSM_MPLS_SNMP_GET_NEXT (outseg_st_type, index.table.out_seg_ix, 
                                  &nsm_mpls_snmp_int_val))
        NSM_MPLS_SNMP_RETURN (nsm_mpls_snmp_int_val, MPLS_OUT_SEG_TABLE);
      break;

    default:
      return NULL;

    }
  return NULL;
}

/* MPLS Out Segment Performance Entry */
static u_char *
mplsOutSegmentPerfEntry (struct variable *v, oid *name, size_t *length, 
                         u_int32_t exact, size_t *var_len, 
                         WriteMethod **write_method, u_int32_t vr_id)
{
  NSM_MPLS_SNMP_UNION(mpls_out_segment_table_index, index);
  struct nsm_master *nm = nsm_master_lookup_by_id (nzg, 0);

  if(!nm)
    return NULL;

  nsm_mpls_snmp_int_val = NSM_MPLS_SNMP_VALUE_INVALID;
  pal_mem_set (&nsm_mpls_snmp_int_hc_val, 0, sizeof (ut_int64_t));
  
  /* Get table indices. */
  if (! nsm_mpls_snmp_index_get (v, name, length, 
                                 &index.snmp,
                                 MPLS_OUT_SEG_PERF_TABLE, exact))
    return NULL; 

#ifdef HAVE_MPLS_FWD
  nsm_gmpls_nhlfe_stats_cleanup_all (nm);
  pal_mpls_nhlfe_stats_update (nm);
#endif /* HAVE_MPLS_FWD */

  if (index.table.len == 0)
    index.table.out_seg_ix = 0;
  
  switch (v->magic)
    {
    case MPLS_OUT_SEG_OCTS:
      if (NSM_MPLS_SNMP_GET_NEXT (outseg_octs, index.table.out_seg_ix, 
                                  &nsm_mpls_snmp_int_val))
        NSM_MPLS_SNMP_RETURN (nsm_mpls_snmp_int_val, MPLS_OUT_SEG_PERF_TABLE);
      break;

    case MPLS_OUT_SEG_PKTS:
      if (NSM_MPLS_SNMP_GET_NEXT (outseg_pkts, index.table.out_seg_ix, 
                                  &nsm_mpls_snmp_int_val))
        NSM_MPLS_SNMP_RETURN (nsm_mpls_snmp_int_val, MPLS_OUT_SEG_PERF_TABLE);
      break;

    case MPLS_OUT_SEG_ERRORS:
      if (NSM_MPLS_SNMP_GET_NEXT (outseg_errors, index.table.out_seg_ix, 
                                  &nsm_mpls_snmp_int_val))
        NSM_MPLS_SNMP_RETURN (nsm_mpls_snmp_int_val, MPLS_OUT_SEG_PERF_TABLE);
      break;

    case MPLS_OUT_SEG_DISCARDS:
      if (NSM_MPLS_SNMP_GET_NEXT (outseg_discards, index.table.out_seg_ix, 
                                  &nsm_mpls_snmp_int_val))
        NSM_MPLS_SNMP_RETURN (nsm_mpls_snmp_int_val, MPLS_OUT_SEG_PERF_TABLE);
      break;

    case MPLS_OUT_SEG_HC_OCTS:
      if (NSM_MPLS_SNMP_GET_NEXT (outseg_hc_octs, index.table.out_seg_ix, 
                                  &nsm_mpls_snmp_int_hc_val))
        NSM_MPLS_SNMP_RETURN (nsm_mpls_snmp_int_hc_val,
                              MPLS_OUT_SEG_PERF_TABLE);
      break;
      
    case MPLS_OUT_SEG_DIS_TIME:
      if (NSM_MPLS_SNMP_GET_NEXT (outseg_perf_dis_time, index.table.out_seg_ix, 
                                  &nsm_mpls_snmp_time_val))
        NSM_MPLS_SNMP_RETURN (nsm_mpls_snmp_time_val, MPLS_OUT_SEG_PERF_TABLE);
      break;
      
    default:
      return NULL;
    }
  return NULL;
}

/* MPLS XC Entry */
static u_char *
mplsXCEntry (struct variable *v, oid *name, size_t *length, 
             u_int32_t exact, size_t *var_len,
             WriteMethod **write_method, u_int32_t vr_id)
{
  NSM_MPLS_SNMP_UNION(mpls_XC_table_index, index);
  struct nsm_master *nm = nsm_master_lookup_by_id (nzg, 0);
  
  nsm_mpls_snmp_int_val = NSM_MPLS_SNMP_VALUE_INVALID;
  nsm_mpls_snmp_char_ptr[0] = '\0';

  if (!nm)
    return NULL;
  
  /* Get table indices. */
  if (! nsm_mpls_snmp_index_get (v, name, length, 
                                 &index.snmp,
                                 MPLS_XC_TABLE, exact))
    return NULL; 

  if (index.table.len == 0)
    {
      index.table.xc_ix =  0;
      index.table.in_seg_ix = 0;
      index.table.out_seg_ix =  0;
    }
  
  switch (v->magic)
    { 
    case MPLS_XC_IX:
    case MPLS_XC_IN_SEG_IX:
    case MPLS_XC_OUT_SEG_IX:
      if (! exact)
        {
          index.table.xc_ix =  0;
          index.table.in_seg_ix = 0;
          index.table.out_seg_ix =  0;
          nsm_gmpls_snmp_index_set (v, name, length,
                                   (struct nsm_mpls_snmp_index *)&index, 
                                   MPLS_XC_TABLE);
        }
      return NULL;
      break;

    case MPLS_XC_LSPID:
      *write_method = write_mplsXCEntry;
      if (NSM_MPLS_SNMP_GET4NEXT (xc_lspid, index.table.xc_ix, index.table.in_seg_ix, 
                                 index.table.out_seg_ix,
                                 (index.table.len + 1) * sizeof(u_int32_t)*8,
                                 &nsm_mpls_snmp_char_ptr))
      { 
        NSM_MPLS_SNMP_RETURN_CHAR (nsm_mpls_snmp_char_ptr, 
                                   MPLS_XC_TABLE);
      }
      break;

    case MPLS_XC_LB_STK_IX:
      *write_method = write_mplsXCEntry;
      if (NSM_MPLS_SNMP_GET4NEXT(xc_lb_stk_ix, index.table.xc_ix, 
                                 index.table.in_seg_ix, 
                                 index.table.out_seg_ix,
                                 (index.table.len + 1) * sizeof(u_int32_t)*8,
                                 &nsm_mpls_snmp_int_val))
        NSM_MPLS_SNMP_RETURN (nsm_mpls_snmp_int_val, 
                              MPLS_XC_TABLE); 
      break;

    case MPLS_XC_OWNER:
      /* Read Only Object */
      if (NSM_MPLS_SNMP_GET4NEXT(xc_owner, index.table.xc_ix, 
                                 index.table.in_seg_ix, 
                                 index.table.out_seg_ix,
                                 (index.table.len + 1) * sizeof(u_int32_t)*8,
                                 &nsm_mpls_snmp_int_val))
        NSM_MPLS_SNMP_RETURN (nsm_mpls_snmp_int_val, MPLS_XC_TABLE);
      break;

    case MPLS_XC_ROW_STATUS:
      *write_method = write_mplsXCEntry;
      if (NSM_MPLS_SNMP_GET4NEXT(xc_row_status, index.table.xc_ix, 
                                 index.table.in_seg_ix, 
                                 index.table.out_seg_ix,
                                 (index.table.len + 1) * sizeof(u_int32_t)*8,
                                 &nsm_mpls_snmp_int_val))
        NSM_MPLS_SNMP_RETURN (nsm_mpls_snmp_int_val, MPLS_XC_TABLE);
      break;

    case MPLS_XC_ST_TYPE:
      *write_method = write_mplsXCEntry;
      if (NSM_MPLS_SNMP_GET4NEXT(xc_st_type, index.table.xc_ix, 
                                 index.table.in_seg_ix, 
                                 index.table.out_seg_ix,
                                 (index.table.len + 1) * sizeof(u_int32_t)*8,
                                 &nsm_mpls_snmp_int_val))
        NSM_MPLS_SNMP_RETURN (nsm_mpls_snmp_int_val, MPLS_XC_TABLE);
      break;

    case MPLS_XC_ADM_STATUS:
      *write_method = write_mplsXCEntry;
      if (NSM_MPLS_SNMP_GET4NEXT(xc_adm_status, index.table.xc_ix, 
                                 index.table.in_seg_ix, 
                                 index.table.out_seg_ix,
                                 (index.table.len + 1) * sizeof(u_int32_t)*8,
                                 &nsm_mpls_snmp_int_val))
        NSM_MPLS_SNMP_RETURN (nsm_mpls_snmp_int_val, MPLS_XC_TABLE);
      break;

    case MPLS_XC_OPER_STATUS:
      /* Read Only Object */
      if (NSM_MPLS_SNMP_GET4NEXT(xc_oper_status, index.table.xc_ix, 
                                 index.table.in_seg_ix, 
                                 index.table.out_seg_ix,
                                 (index.table.len + 1) * sizeof(u_int32_t)*8,
                                 &nsm_mpls_snmp_int_val))
        NSM_MPLS_SNMP_RETURN (nsm_mpls_snmp_int_val, MPLS_XC_TABLE);
      break;

    default:
      return NULL; 
      break;
    }
  
  return NULL; 
}

/* MPLS FTN Scalars */
static u_char *
mplsFTNScalars (struct variable *v, oid *name, size_t *length, 
                int exact, size_t *var_len, 
                WriteMethod **write_method, u_int32_t vr_id)
{
  int ret;
  struct ptree_ix_table *pix_tbl;
  struct fec_gen_entry fec;
  struct nsm_master *nm = nsm_master_lookup_by_id (nzg, 0);

  if (!nm)
    return NULL;

  /* The bitmap is the same that is why using AF_INET, we could have used 
     AF_INET6 too*/
  fec.type = gmpls_entry_type_ip;
  fec.u.prefix.family = AF_INET;

  pix_tbl = nsm_gmpls_ftn_table_lookup (nm, GLOBAL_FTN_ID, &fec);       
  if (!pix_tbl)
    return NULL;

  if (snmp_header_generic(v, name, length, exact, var_len, write_method)
      == MATCH_FAILED)
    return NULL;
  
  switch (v->magic)
    {
      case MPLS_FTN_IX_NXT:
        ret = bitmap_request_index (pix_tbl->ix_mgr, &nsm_mpls_snmp_int_val);

        /* Release Index so that bitmap can reuse this index for allocation */
        bitmap_release_index (pix_tbl->ix_mgr, nsm_mpls_snmp_int_val);

        return (u_char *)&nsm_mpls_snmp_int_val;
        break;

      case MPLS_FTN_LAST_CHANGED:
        nsm_mpls_snmp_time_val = last_changed_time;
        return (u_char *)&nsm_mpls_snmp_time_val;
        break;

      default:
        return NULL;
        break;
    }
  return NULL;
}

/* MPLS FTN Entry */
static u_char *
mplsFTNEntry (struct variable *v, oid *name, size_t *length, 
              u_int32_t exact, size_t *var_len, WriteMethod **write_method, 
              u_int32_t vr_id)
{
  
  NSM_MPLS_SNMP_UNION(mpls_FTN_table_index, index);
  struct nsm_master *nm = nsm_master_lookup_by_id (nzg, 0);

  if (!nm)
    return NULL;

  nsm_mpls_snmp_int_val = NSM_MPLS_SNMP_VALUE_INVALID;
  nsm_mpls_snmp_char_ptr[0] = '\0';

  /* Get table indices. */
  if (! nsm_mpls_snmp_index_get (v, name, length, 
                                 &index.snmp,
                                 MPLS_FTN_TABLE, exact))
    return NULL; 

  if (index.table.len == 0)
    index.table.ftn_ix = 0;
  
  *write_method = write_mplsFTNEntry;

  switch (v->magic)
    {      
    case MPLS_FTN_IX:
      if (! exact)
        {
          index.table.ftn_ix = 0;
          nsm_gmpls_snmp_index_set (v, name, length,
                                   (struct nsm_mpls_snmp_index *)&index, 
                                   MPLS_FTN_TABLE);
        }
      return NULL;
      break;

    case MPLS_FTN_ROW_STATUS:
      if (NSM_MPLS_SNMP_GET_NEXT (ftn_row_status, index.table.ftn_ix, 
                                  &nsm_mpls_snmp_int_val))
        NSM_MPLS_SNMP_RETURN (nsm_mpls_snmp_int_val, MPLS_FTN_TABLE);
      break;
      
    case MPLS_FTN_DESCR:
      if (NSM_MPLS_SNMP_GET_NEXT (ftn_descr, index.table.ftn_ix, 
                                  &nsm_mpls_snmp_char_ptr))
        NSM_MPLS_SNMP_RETURN_CHAR (nsm_mpls_snmp_char_ptr, 
                                   MPLS_FTN_TABLE);     
      break;

    case MPLS_FTN_MASK:
      if (NSM_MPLS_SNMP_GET_NEXT (ftn_mask, index.table.ftn_ix, 
                                  &nsm_mpls_snmp_int_val))
        NSM_MPLS_SNMP_RETURN (nsm_mpls_snmp_int_val, MPLS_FTN_TABLE);
      break;
      
    case MPLS_FTN_ADDR_TYPE:
      if (NSM_MPLS_SNMP_GET_NEXT (ftn_addr_type, index.table.ftn_ix, 
                                  &nsm_mpls_snmp_int_val))
        NSM_MPLS_SNMP_RETURN (nsm_mpls_snmp_int_val, MPLS_FTN_TABLE);
      break;
      
    case MPLS_FTN_SRC_ADDR_MIN:
      if (NSM_MPLS_SNMP_GET_NEXT (ftn_src_addr_min, index.table.ftn_ix, 
                                  &nsm_mpls_snmp_char_ptr))
        NSM_MPLS_SNMP_RETURN_CHAR (nsm_mpls_snmp_char_ptr, 
                                   MPLS_FTN_TABLE);
      break;

    case MPLS_FTN_SRC_ADDR_MAX:
      if (NSM_MPLS_SNMP_GET_NEXT (ftn_src_addr_max, index.table.ftn_ix, 
                                  &nsm_mpls_snmp_char_ptr))
        NSM_MPLS_SNMP_RETURN_CHAR (nsm_mpls_snmp_char_ptr, 
                                   MPLS_FTN_TABLE);
      break;
      
    case MPLS_FTN_DST_ADDR_MIN:
      if (NSM_MPLS_SNMP_GET_NEXT (ftn_dst_addr_min, index.table.ftn_ix, 
                                  &nsm_mpls_snmp_char_ptr))
        NSM_MPLS_SNMP_RETURN_CHAR (nsm_mpls_snmp_char_ptr, 
                                   MPLS_FTN_TABLE);
      break;

    case MPLS_FTN_DST_ADDR_MAX:
      if (NSM_MPLS_SNMP_GET_NEXT (ftn_dst_addr_max, index.table.ftn_ix, 
                                  &nsm_mpls_snmp_char_ptr))
        NSM_MPLS_SNMP_RETURN_CHAR (nsm_mpls_snmp_char_ptr, 
                                   MPLS_FTN_TABLE);
      break;

    case MPLS_FTN_SRC_PORT_MIN:
      if (NSM_MPLS_SNMP_GET_NEXT (ftn_src_port_min, index.table.ftn_ix, 
                                  &nsm_mpls_snmp_int_val))
        NSM_MPLS_SNMP_RETURN (nsm_mpls_snmp_int_val, 
                              MPLS_FTN_TABLE);
      break;

    case MPLS_FTN_SRC_PORT_MAX:
      if (NSM_MPLS_SNMP_GET_NEXT (ftn_src_port_max, index.table.ftn_ix, 
                                  &nsm_mpls_snmp_int_val))
        NSM_MPLS_SNMP_RETURN (nsm_mpls_snmp_int_val, 
                              MPLS_FTN_TABLE);
      break;
      
    case MPLS_FTN_DST_PORT_MIN:
      if (NSM_MPLS_SNMP_GET_NEXT (ftn_dst_port_min, index.table.ftn_ix, 
                                  &nsm_mpls_snmp_int_val))
        NSM_MPLS_SNMP_RETURN (nsm_mpls_snmp_int_val, 
                              MPLS_FTN_TABLE);
      break;

    case MPLS_FTN_DST_PORT_MAX:
      if (NSM_MPLS_SNMP_GET_NEXT (ftn_dst_port_max, index.table.ftn_ix, 
                                  &nsm_mpls_snmp_int_val))
        NSM_MPLS_SNMP_RETURN (nsm_mpls_snmp_int_val, 
                              MPLS_FTN_TABLE);
      break;

    case MPLS_FTN_PROTOCOL:
      if (NSM_MPLS_SNMP_GET_NEXT (ftn_protocol, index.table.ftn_ix, 
                                  &nsm_mpls_snmp_int_val))
        NSM_MPLS_SNMP_RETURN (nsm_mpls_snmp_int_val, MPLS_FTN_TABLE);
      break;

    case MPLS_FTN_DSCP:
      if (NSM_MPLS_SNMP_GET_NEXT (ftn_dscp, index.table.ftn_ix, 
                                  &nsm_mpls_snmp_int_val))
        NSM_MPLS_SNMP_RETURN (nsm_mpls_snmp_int_val, MPLS_FTN_TABLE);
      break;

    case MPLS_FTN_ACT_TYPE:
      if (NSM_MPLS_SNMP_GET_NEXT (ftn_act_type, index.table.ftn_ix, 
                                  &nsm_mpls_snmp_int_val))
        NSM_MPLS_SNMP_RETURN (nsm_mpls_snmp_int_val, MPLS_FTN_TABLE);
      break;

    case MPLS_FTN_ACT_POINTER:
      if (NSM_MPLS_SNMP_GET_NEXT (ftn_act_pointer, index.table.ftn_ix, 
                                  &nsm_mpls_snmp_char_ptr))
        NSM_MPLS_SNMP_RETURN_CHAR (nsm_mpls_snmp_char_ptr, 
                                   MPLS_FTN_TABLE);
      break;

    case MPLS_FTN_ST_TYPE:
      if (NSM_MPLS_SNMP_GET_NEXT (ftn_st_type, index.table.ftn_ix, 
                                  &nsm_mpls_snmp_int_val))
        NSM_MPLS_SNMP_RETURN (nsm_mpls_snmp_int_val, MPLS_FTN_TABLE);
      break;

    default:
      return NULL;
    }
  return NULL;
}

/* MPLS FTN Performance Entry */
static u_char *
mplsFTNPerfEntry (struct variable *v, oid *name, size_t *length, 
                  u_int32_t exact, size_t *var_len, WriteMethod **write_method,
                  u_int32_t vr_id)
{
  NSM_MPLS_SNMP_UNION(mpls_FTN_perf_table_index, index);
  struct nsm_master *nm = nsm_master_lookup_by_id (nzg, 0);

  if (!nm)
    return NULL;

  nsm_mpls_snmp_int_val = NSM_MPLS_SNMP_VALUE_INVALID;
  pal_mem_set (&nsm_mpls_snmp_int_hc_val, 0, sizeof (ut_int64_t));

  /* Get table indices. */
  if (! nsm_mpls_snmp_index_get (v, name, length, 
                                 &index.snmp,
                                 MPLS_FTN_PERF_TABLE, exact))
    return NULL; 

#ifdef HAVE_MPLS_FWD
  pal_mpls_ftn_stats_update (nm);
#endif /* HAVE_MPLS_FWD */

  if (index.table.len == 0)
    {
      index.table.ftn_if_ix = 0;
      index.table.ftn_ix = 0;
    }
 
  if (index.table.ftn_if_ix != 0)
    return NULL;

  switch (v->magic)
    {
    case MPLS_FTN_PERF_IX:
    case MPLS_FTN_PERF_CURR_IX:
      if (! exact)
        {
          index.table.ftn_ix = 0;
          index.table.ftn_if_ix = 0;
          nsm_gmpls_snmp_index_set (v, name, length,
                                   (struct nsm_mpls_snmp_index *)&index, 
                                   MPLS_FTN_PERF_TABLE);
        }
      return NULL;
      break;

    case MPLS_FTN_MTCH_PKTS:
      if (NSM_MPLS_SNMP_GET_NEXT (ftn_mtch_pkts, index.table.ftn_ix, 
                                  &nsm_mpls_snmp_int_hc_val))
        NSM_MPLS_SNMP_RETURN (nsm_mpls_snmp_int_hc_val, MPLS_FTN_PERF_TABLE);
      break;

    case MPLS_FTN_MTCH_OCTS:
      if (NSM_MPLS_SNMP_GET_NEXT (ftn_mtch_octs, index.table.ftn_ix, 
                                  &nsm_mpls_snmp_int_hc_val))
        NSM_MPLS_SNMP_RETURN (nsm_mpls_snmp_int_hc_val, MPLS_FTN_PERF_TABLE);
      break;

    case MPLS_FTN_DISC_TIME:
      if (NSM_MPLS_SNMP_GET_NEXT (ftn_disc_time, index.table.ftn_ix, 
                                  &nsm_mpls_snmp_time_val))
        NSM_MPLS_SNMP_RETURN (nsm_mpls_snmp_time_val, MPLS_FTN_PERF_TABLE);
      break;

    default:
      return NULL;
    }

  return NULL;
}

/* Traps */
void
nsm_mpls_opr_sts_trap (struct nsm_mpls *nmpls, int xc_ix, 
                       int opr_status)
{
  struct trap_object2 obj[3];
  int namelen;
  oid *ptr;

  namelen = sizeof lsr_oid / sizeof (oid); 

  ptr = obj[0].name;
  OID_COPY (ptr, lsr_oid, namelen);
  OID_SET_ARG4 (ptr, 1, 10, 1, 10); 

  OID_SET_VAL (obj[0], ptr - obj[0].name, INTEGER,
               sizeof (int), &opr_status);

  ptr = obj[1].name;
  OID_COPY (ptr, lsr_oid, namelen);
  OID_SET_ARG4 (ptr, 1, 10, 1, 1); 

  OID_SET_VAL (obj[1], ptr - obj[1].name, INTEGER,
               sizeof (int), &xc_ix); 

  ptr = obj[2].name;
  OID_COPY (ptr, lsr_oid, namelen);
  OID_SET_ARG4 (ptr, 1, 10, 1, 1);

  OID_SET_VAL (obj[2], ptr - obj[2].name, INTEGER,
               sizeof (int), &xc_ix); 

  snmp_trap2 (nzg,
              mpls_trap_oid, sizeof mpls_trap_oid / sizeof (oid),
              opr_status,            
              lsr_oid, sizeof lsr_oid / sizeof (oid),
              obj, sizeof obj / sizeof (struct trap_object2),
              pal_time_current (NULL) );
}

/* Register MPLS-LSR-MIB and MPLS-FTN-MIB. */
void
nsm_mpls_snmp_init ()
{
  REGISTER_MIB (NSM_ZG, "mibII/nsm_lsr", nsm_mpls_lsr_variables, 
                variable, lsr_oid);
  REGISTER_MIB (NSM_ZG, "mibII/nsm_ftn", nsm_mpls_ftn_variables, 
                variable, ftn_oid);
}

u_int32_t
nsm_cnt_in_lb_used (struct interface *ifp)
{
#ifdef HAVE_MPLS_FWD
  struct nsm_mpls_if *mif;

  /* Lookup interface. */
  mif = nsm_mpls_if_lookup (ifp);
  if (mif)
    return mif->stats.in_labels_used;
#endif /* HAVE_MPLS_FWD */
  return 0;
}

u_int32_t
nsm_cnt_out_lb_used (struct interface *ifp)
{
#ifdef HAVE_MPLS_FWD
  struct nsm_mpls_if *mif;

  /* Lookup interface. */
  mif = nsm_mpls_if_lookup (ifp);
  if (mif)
    return mif->stats.out_labels_used;
#endif /* HAVE_MPLS_FWD */
  return 0;
}

u_int32_t 
nsm_cnt_failed_lb_lookup (struct interface *ifp)
{
#ifdef HAVE_MPLS_FWD
  struct nsm_mpls_if *mif;

  /* Lookup interface. */
  mif = nsm_mpls_if_lookup (ifp);
  if (mif)
    return mif->stats.label_lookup_failures;
#endif /* HAVE_MPLS_FWD */
  return 0;
}

u_int32_t 
nsm_cnt_out_fragments (struct interface *ifp)
{
#ifdef HAVE_MPLS_FWD
  struct nsm_mpls_if *mif;

  /* Lookup interface. */
  mif = nsm_mpls_if_lookup (ifp);
  if (mif)
    return mif->stats.out_fragments;
#endif /* HAVE_MPLS_FWD */
  return 0;
}

u_int32_t 
nsm_cnt_in_seg_octs (struct ilm_entry *ilm)
{
#ifdef HAVE_MPLS_FWD
  return ilm->stats.rx_bytes;
#endif /* HAVE_MPLS_FWD */
  return 0;
}

u_int32_t 
nsm_cnt_in_seg_pkts (struct ilm_entry *ilm)
{
#ifdef HAVE_MPLS_FWD
  return ilm->stats.rx_pkts;
#endif /* HAVE_MPLS_FWD */
  return 0;
}

u_int32_t 
nsm_cnt_in_seg_errors (struct ilm_entry *ilm)
{
#ifdef HAVE_MPLS_FWD
  return ilm->stats.error_pkts;
#endif /* HAVE_MPLS_FWD */
  return 0;
}

u_int32_t 
nsm_cnt_in_seg_discards (struct ilm_entry *ilm)
{
#ifdef HAVE_MPLS_FWD
  return ilm->stats.discard_pkts;
#endif /* HAVE_MPLS_FWD */
  return 0;
}

time_t 
nsm_cnt_in_seg_perf_dis_time (struct ilm_entry *ilm)
{
  return 0;
}

ut_int64_t
nsm_cnt_in_seg_hc_octs (struct ilm_entry *ilm)
{
  ut_int64_t in_hc_octs;
  pal_mem_set (&in_hc_octs, 0, sizeof (ut_int64_t));

  return in_hc_octs;
}

u_int32_t 
nsm_cnt_out_seg_octs (struct nhlfe_entry *nhlfe)
{
#ifdef HAVE_MPLS_FWD
  return nhlfe->stats.tx_bytes;
#endif /* HAVE_MPLS_FWD */
  return 0;
}

u_int32_t 
nsm_cnt_out_seg_pkts (struct nhlfe_entry *nhlfe)
{
#ifdef HAVE_MPLS_FWD
  return nhlfe->stats.tx_pkts;
#endif /* HAVE_MPLS_FWD */
  return 0;
}

u_int32_t 
nsm_cnt_out_seg_errors (struct nhlfe_entry *nhlfe)
{
#ifdef HAVE_MPLS_FWD
  return nhlfe->stats.error_pkts;
#endif /* HAVE_MPLS_FWD */
  return 0;
}

u_int32_t 
nsm_cnt_out_seg_discards (struct nhlfe_entry *nhlfe)
{
#ifdef HAVE_MPLS_FWD
  return nhlfe->stats.discard_pkts;
#endif /* HAVE_MPLS_FWD */
  return 0;
}

time_t 
nsm_cnt_out_seg_perf_dis_time (struct nhlfe_entry *nhlfe)
{
  return 0;
}

ut_int64_t
nsm_cnt_out_seg_hc_octs (struct nhlfe_entry *nhlfe)
{
  ut_int64_t out_hc_octs;
  pal_mem_set (&out_hc_octs, 0, sizeof (ut_int64_t));

  return out_hc_octs;
}

ut_int64_t 
nsm_cnt_ftn_mtch_pkts (struct ftn_entry *ftn)
{
  ut_int64_t ftn_mtch_pkts;
  pal_mem_set (&ftn_mtch_pkts, 0, sizeof (ut_int64_t));

  return ftn_mtch_pkts;
}

ut_int64_t 
nsm_cnt_ftn_mtch_octs (struct ftn_entry *ftn)
{
  ut_int64_t ftn_mtch_octs;
  pal_mem_set (&ftn_mtch_octs, 0, sizeof (ut_int64_t));

  return ftn_mtch_octs;
}

u_int32_t
nsm_cnt_ftn_disc_time (struct ftn_entry *ftn)
{
  return 0;
}

#endif /* HAVE_SNMP */
