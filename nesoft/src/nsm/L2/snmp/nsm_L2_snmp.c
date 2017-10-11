/* Copyright (C) 2001-2004  All Rights Reserved. */

#include "pal.h"
#include "lib.h"

#ifdef HAVE_SNMP
#include <asn1.h>

#include "if.h"
#include "log.h"
#include "prefix.h"
#include "table.h"
#include "ptree.h"
#include "avl_tree.h"
#include "thread.h"
#include "snmp.h"
#include "l2lib.h"
#include "l2_snmp.h"
#include "linklist.h"
#include "pal_math.h"
#include "nsmd.h"
#include "nsm_bridge.h"
#include "nsmd.h"
#include "nsm_snmp.h"
#include "nsm_L2_snmp.h"
#include "nsm_p_mib.h"
#include "nsm_snmp_common.h"

#ifdef HAVE_HAL
#include "hal_incl.h"
#endif /* HAVE_HAL */

#ifdef HAVE_VLAN
#include "nsm_vlan.h"
#include "nsm_bridge_cli.h"
#include "nsm_q_mib.h"
#endif /* HAVE_VLAN */

#include "nsm_interface.h"

/* Define SNMP local variables. */

static pal_time_t xstp_start_time;

static int xstp_int_val;
static ut_int64_t xstp_int64_val;
extern void pal_if_get_stats (struct pal_if_stats *stats, struct nsm_if_stats *buf);

/* static u_long stp_ulong_val;
static u_short stp_ushort_val; */

u_char *xstp_string_val;

#ifdef HAVE_VLAN
//static struct port_vlan_HC_table *port_vlan_HC_table;
static struct port_vlan_stats_table *port_vlan_stats_table;
#endif /* HAVE_VLAN */

char  br_ports_bitstring[BITSTRINGSIZE(NSM_L2_SNMP_PORT_ARRAY_SIZE)];

/* function prototypes */

FindVarMethod xstp_snmp_dot1dBridgeScalars;
FindVarMethod xstp_snmp_dot1dStpPortTable;
FindVarMethod xstp_snmp_dot1dTpFdbTable;
FindVarMethod xstp_snmp_dot1dBasePortTable;
FindVarMethod xstp_snmp_dot1dTpPortTable;
FindVarMethod xstp_snmp_dot1dStaticTable;

WriteMethod xstp_snmp_write_dot1dBridgeScalars;
WriteMethod xstp_snmp_write_dot1dStpPortTable;
WriteMethod xstp_snmp_write_dot1dStaticTable;

/* these are non-configurable objects */

int xstp_snmp_dot1dBaseType = 2; /* transparent-only(2) */
int xstp_snmp_dot1dStpProtocolSpecification = 3; /* ieee8021d(3) */

/* need to instrument forwarder */
static u_int dot1dBasePortDelayExceededDiscards = 0;

static u_int dot1dTpPortMaxInfo  = 1500;  /* Same as specified by Cisco */

/* the object ID's for registration. */
static oid xstp_BasePortTable_oid[] = { BRIDGEMIBOID , 1 , 4 };
static oid xstp_TpFdbTable_oid[] = { BRIDGEMIBOID , 4 , 3 };
static oid xstp_TpPortTable_oid[] = { BRIDGEMIBOID , 4 , 4 };
#ifdef HAVE_VLAN
static oid xstp_TpHCPortTable_oid[] = { BRIDGEMIBOID , 4 , 5 };
static oid xstp_PortOverflowTable_oid[] = { BRIDGEMIBOID , 4 , 6 };
#endif /* HAVE_VLAN */
static oid xstp_StaticTable_oid[] = { BRIDGEMIBOID , 5 , 1 };

static oid null_oid[] = { 0, 0 };

/* cached FDB entries for dot1dTpFdbTable */
XSTP_FDB_CACHE *fdb_cache = NULL;

/* cached FDB entries for dot1dTpStaticTable */
XSTP_FDB_CACHE *fdb_static_cache = NULL;

struct lib_globals *snmpg = NULL;

/* dot1dBridge_BasePortTable */
struct variable dot1dBridge_BasePortTable[] = {

  {DOT1DBASEPORT, ASN_INTEGER, RONLY, xstp_snmp_dot1dBasePortTable,
   2, { 1, 1}},
  {DOT1DBASEPORTIFINDEX, ASN_INTEGER, RONLY, xstp_snmp_dot1dBasePortTable,
   2, {1, 2}},
  {DOT1DBASEPORTCIRCUIT, ASN_OBJECT_ID, RONLY, xstp_snmp_dot1dBasePortTable,
   2, {1, 3}},
  {DOT1DBASEPORTDELAYEXCEEDEDDISCARDS, ASN_COUNTER, RONLY,
   xstp_snmp_dot1dBasePortTable,
   2, {1, 4}},
  {DOT1DBASEPORTMTUEXCEEDEDDISCARDS, ASN_COUNTER, RONLY,
   xstp_snmp_dot1dBasePortTable,
   2, {1, 5}},
};

/* dot1dBridge_TpFdbTable */
struct variable dot1dBridge_TpFdbTable[] = {
 
  {DOT1DTPFDBADDRESS, ASN_OCTET_STR, RONLY, xstp_snmp_dot1dTpFdbTable,
   2, { 1, 1}},
  {DOT1DTPFDBPORT, ASN_INTEGER, RONLY, xstp_snmp_dot1dTpFdbTable,
   2, {1, 2}},
  {DOT1DTPFDBSTATUS, ASN_INTEGER, RONLY, xstp_snmp_dot1dTpFdbTable,
   2, {1, 3}},
};

/* dot1dBridge_TpPortTable */
struct variable dot1dBridge_TpPortTable[] = {

  {DOT1DTPPORT, ASN_INTEGER, RONLY, xstp_snmp_dot1dTpPortTable,
   2, {1, 1}},
  {DOT1DTPPORTMAXINFO, ASN_INTEGER, RONLY, xstp_snmp_dot1dTpPortTable,
   2, {1, 2}},
  {DOT1DTPPORTINFRAMES, ASN_COUNTER, RONLY, xstp_snmp_dot1dTpPortTable,
   2, {1, 3}},
  {DOT1DTPPORTOUTFRAMES, ASN_COUNTER, RONLY, xstp_snmp_dot1dTpPortTable,
   2, {1, 4}},
  {DOT1DTPPORTINDISCARDS, ASN_COUNTER, RONLY, xstp_snmp_dot1dTpPortTable,
   2, {1, 5}},
};

#ifdef HAVE_VLAN
/* dot1dBridge_TpHCPortTable */
struct variable dot1dBridge_TpHCPortTable[] = {

  {DOT1DTPHCPORTINFRAMES, ASN_COUNTER, RONLY, xstp_snmp_dot1dTpHCPortTable,
   2, {1, 1}},
  {DOT1DTPHCPORTOUTFRAMES, ASN_COUNTER, RONLY, xstp_snmp_dot1dTpHCPortTable,
   2, {1, 2}},
  {DOT1DTPHCPORTINDISCARDS, ASN_COUNTER, RONLY, xstp_snmp_dot1dTpHCPortTable,
   2, {1, 3}},
};

/* dot1dBridge_PortOverflowTable */
struct variable dot1dBridge_PortOverflowTable[] = {
  
  {DOT1DTPPORTINOVERFLOWFRAMES, ASN_COUNTER, RONLY,
   xstp_snmp_dot1dTpPortOverflowTable,
   2, { 1, 1}},
  {DOT1DTPPORTOUTOVERFLOWFRAMES, ASN_COUNTER, RONLY,
   xstp_snmp_dot1dTpPortOverflowTable,
   2, {1, 2}},
  {DOT1DTPPORTINOVERFLOWDISCARDS, ASN_COUNTER, RONLY,
   xstp_snmp_dot1dTpPortOverflowTable,
   2, {1, 3}},
};
#endif /* HAVE_VLAN */

/* dot1dBridge_StaticTable */
struct variable dot1dBridge_StaticTable[] = {
  
  {DOT1DSTATICADDRESS, ASN_OCTET_STR, RWRITE, xstp_snmp_dot1dStaticTable,
   2, {1, 1}},
  {DOT1DSTATICRECEIVEPORT, ASN_INTEGER, RWRITE, xstp_snmp_dot1dStaticTable,
   2, {1, 2}},
  {DOT1DSTATICALLOWEDTOGOTO, ASN_OCTET_STR, RWRITE, xstp_snmp_dot1dStaticTable,
   2, {1, 3}},
  {DOT1DSTATICSTATUS, ASN_INTEGER, RWRITE, xstp_snmp_dot1dStaticTable,
   2, {1, 4}},
};

void
xstp_snmp_init (struct lib_globals *zg)
{

/*
 * register ourselves with the agent to handle our mib tree objects 
 */
  REGISTER_MIB (zg, "dot1dBridge_BasePortTable", dot1dBridge_BasePortTable,
                variable, xstp_BasePortTable_oid);
  REGISTER_MIB (zg, "dot1dBridge_TpFdbTable", dot1dBridge_TpFdbTable,
                variable, xstp_TpFdbTable_oid);
  REGISTER_MIB (zg, "dot1dBridge_TpPortTable", dot1dBridge_TpPortTable,
                variable, xstp_TpPortTable_oid);
#ifdef HAVE_VLAN
  REGISTER_MIB (zg, "dot1dBridge_TpHCPortTable", dot1dBridge_TpHCPortTable,
                variable, xstp_TpHCPortTable_oid);
  REGISTER_MIB (zg, "dot1dBridge_PortOverflowTable",
                dot1dBridge_PortOverflowTable, variable,
                xstp_PortOverflowTable_oid);
#endif /* HAVE_VLAN */
  REGISTER_MIB (zg, "dot1dBridge_StaticTable", dot1dBridge_StaticTable,
                variable, xstp_StaticTable_oid);

  xstp_start_time = pal_time_current (NULL);

  snmpg = zg;

}

int
xstp_snmp_br_port_state( int br_port_state )
{
  static char snmp_br_port_state[] = {
    DOT1DSTPPORTSTATE_DISABLED, 
    DOT1DSTPPORTSTATE_LISTENING,        
    DOT1DSTPPORTSTATE_LEARNING, 
    DOT1DSTPPORTSTATE_FORWARDING,
    DOT1DSTPPORTSTATE_BLOCKING, 
    DOT1DSTPPORTSTATE_BROKEN            
  };

  return ( br_port_state < NELEMENTS(snmp_br_port_state) ?
        snmp_br_port_state[br_port_state] : DOT1DSTPPORTSTATE_BROKEN );

}

/*
 * xstp_snmp_dot1dBasePortTable():
 */
unsigned char *
xstp_snmp_dot1dBasePortTable (struct variable *vp,
                              oid * name,
                              size_t * length,
                              int exact,
                              size_t * var_len, WriteMethod ** write_method,
                              u_int32_t vr_id)
{
  int ret;
  struct nsm_bridge *br;
  struct interface *ifp;
  static u_int32_t index = 0;
  struct nsm_bridge_port *br_port;
  struct  hal_if_counters hal_if_stats;
  br_port = NULL;

  /* validate the index */
  ret = l2_snmp_index_get (vp, name, length, &index, exact);
  if (ret < 0)
    return NULL;

  br = nsm_snmp_get_first_bridge();

  if (br == NULL)
    return NULL;

  ifp = nsm_snmp_bridge_port_lookup_by_port_id (br, index, exact, &br_port);

  if (ifp == NULL || br_port == NULL)
    return NULL;

  ret = hal_if_get_counters(ifp->ifindex, &hal_if_stats);
  if (ret < 0)
    return NULL;
  
  if (!exact)
    {
      l2_snmp_index_set (vp, name, length, br_port->port_id);
    }

  switch (vp->magic)
    {
    case DOT1DBASEPORT:
      XSTP_SNMP_RETURN_INTEGER (br_port->port_id);
      break;

    case DOT1DBASEPORTIFINDEX:
      XSTP_SNMP_RETURN_INTEGER (ifp->ifindex);
      break;

    case DOT1DBASEPORTCIRCUIT:
      XSTP_SNMP_RETURN_OID (null_oid, NELEMENTS (null_oid));
      break;

    case DOT1DBASEPORTDELAYEXCEEDEDDISCARDS:
      XSTP_SNMP_RETURN_INTEGER (dot1dBasePortDelayExceededDiscards);
      break;

    case DOT1DBASEPORTMTUEXCEEDEDDISCARDS:
      XSTP_SNMP_RETURN_INTEGER (hal_if_stats.mtu_exceed.l[0]);
      break;

    default:
      pal_assert (0);
    }
  return NULL;
}

/*
 * xstp_snmp_dot1dTpPortTable():
 */
unsigned char *
xstp_snmp_dot1dTpPortTable (struct variable *vp,
                            oid * name,
                            size_t * length,
                            int exact,
                            size_t * var_len, WriteMethod ** write_method,
                            u_int32_t vr_id)
{

  int ret;
  static u_int32_t index = 0;
  struct nsm_bridge *br;
  struct interface *ifp = NULL;
#ifndef ENABLE_HAL_PATH
  struct nsm_if_stats nsm_stats;
#endif /* ENABLE_HAL_PATH */
  struct nsm_bridge_port *br_port;
#ifdef ENABLE_HAL_PATH
  struct hal_if_counters if_stats;
#endif /* ENABLE_HAL_PATH */


  br_port = NULL;

  /* validate the index */
  ret = l2_snmp_index_get (vp, name, length, &index, exact);
  if (ret < 0)
    return NULL;

  br = nsm_snmp_get_first_bridge();

  if (br == NULL)
    return NULL;

  ifp = nsm_snmp_bridge_port_lookup_by_port_id (br, index, exact, &br_port);

  if (ifp == NULL || br_port == NULL)
    return NULL;

#ifndef ENABLE_HAL_PATH
  NSM_IF_STAT_UPDATE(ifp->vr);
  pal_if_get_stats (&ifp->stats, &nsm_stats);
#else
  hal_if_get_counters (ifp->ifindex, &if_stats);
#endif /* ENABLE_HAL_PATH */

  if (!exact)
    {
      l2_snmp_index_set (vp, name, length, br_port->port_id);
    }

  switch (vp->magic)
    {
    case DOT1DTPPORT:
      XSTP_SNMP_RETURN_INTEGER (br_port->port_id);
      break;

    case DOT1DTPPORTMAXINFO:
      XSTP_SNMP_RETURN_INTEGER (dot1dTpPortMaxInfo);
      break;

    case DOT1DTPPORTINFRAMES:
#ifdef ENABLE_HAL_PATH
        XSTP_SNMP_RETURN_INTEGER (if_stats.good_pkts_rcv.l[0]);
#else
        XSTP_SNMP_RETURN_INTEGER (nsm_stats.ifInUcastPkts + nsm_stats.ifInNUcastPkts);
#endif /* ENABLE_HAL_PATH */
      break;

    case DOT1DTPPORTOUTFRAMES:
#ifdef ENABLE_HAL_PATH
        XSTP_SNMP_RETURN_INTEGER (if_stats.good_pkts_sent.l [0]);
#else
        XSTP_SNMP_RETURN_INTEGER (nsm_stats.ifOutUcastPkts + nsm_stats.ifOutNUcastPkts);
#endif /* ENABLE_HAL_PATH */
      break;

    case DOT1DTPPORTINDISCARDS:
#ifdef ENABLE_HAL_PATH
        XSTP_SNMP_RETURN_INTEGER (if_stats.bad_pkts_rcv.l [0]);
#else
        XSTP_SNMP_RETURN_INTEGER (nsm_stats.ifInDiscards);
#endif /* ENABLE_HAL_PATH */
      break;

    default:
      pal_assert(0);
    }
  return NULL;
}

#ifdef HAVE_VLAN
/*
 * xstp_snmp_dot1dTpHCPortTable():
 */
unsigned char  *
xstp_snmp_dot1dTpHCPortTable(struct variable *vp,
                             oid * name,
                             size_t * length,
                             int exact,
                             size_t * var_len, WriteMethod ** write_method,
                             u_int32_t vr_id)
{
  struct nsm_bridge *br;
  int ret;
  static u_int32_t index = 0;
  struct interface *ifp = NULL;
/*  struct pal_if_stats if_stats;*/
#ifdef HAVE_HAL
  struct hal_if_counters if_stats;
  ut_int64_t tot_rcvd;
#endif /* HAVE_HAL */
  struct nsm_bridge_port *br_port;

  br_port = NULL;

  pal_mem_set (&xstp_int64_val, 0, sizeof (ut_int64_t));

#ifdef HAVE_HAL
  pal_mem_set (&if_stats, 0, sizeof (if_stats));
#endif /* HAVE_HAL */
  
  /* validate the index */
  ret = l2_snmp_index_get (vp, name, length, &index, exact);
  if (ret < 0)
     return NULL;

  br = nsm_snmp_get_first_bridge();
  if (br == NULL)
    return NULL;

  ifp = nsm_snmp_bridge_port_lookup_by_port_id (br, index, exact, &br_port);

  if (ifp == NULL || br_port == NULL)
    return NULL;

#ifdef ENABLE_HAL_PATH
  hal_if_get_counters (ifp->ifindex, &if_stats);
#else
  NSM_IF_STAT_UPDATE (ifp->vr);
#endif /* ENABLE_HAL_PATH */

  if (!exact)
  {
    l2_snmp_index_set (vp, name, length, br_port->port_id);
  }

  switch (vp->magic) {
#ifdef HAVE_HAL
    case DOT1DTPHCPORTINFRAMES:
       NSM_ADD_64_UINT (if_stats.good_pkts_rcv,
                       if_stats.bad_pkts_rcv,
                       tot_rcvd);
      pal_mem_cpy (&xstp_int64_val, &tot_rcvd,
                                    sizeof (ut_int64_t));
      XSTP_SNMP_RETURN_INTEGER64 (xstp_int64_val);
      break;
      
    case DOT1DTPHCPORTOUTFRAMES:
      pal_mem_cpy (&xstp_int64_val, &if_stats.good_pkts_sent,
                                               sizeof (ut_int64_t));
      XSTP_SNMP_RETURN_INTEGER64 (xstp_int64_val);
      break;

    case DOT1DTPHCPORTINDISCARDS:
      pal_mem_cpy (&xstp_int64_val, &if_stats.drop_events,
                                               sizeof (ut_int64_t));
      XSTP_SNMP_RETURN_INTEGER64 (xstp_int64_val);
      break;
#endif /* HAVE_HAL */
  default:
        pal_assert (0);
        break;
  }

  return NULL;
}

/*
 * xstp_snmp_dot1dTpPortOverflowTable():
 *   Handle this table separately from the scalar value case.
 *   The workings of this are basically the same as for xstp_snmp_ above.
 */
unsigned char  *
xstp_snmp_dot1dTpPortOverflowTable(struct variable *vp,
                                   oid * name,
                                   size_t * length,
                                   int exact,
                                   size_t * var_len, 
                                   WriteMethod ** write_method,
                                   u_int32_t vr_id)
{
  struct nsm_bridge *br;
  int ret;
  static u_int32_t index = 0;
  struct interface *ifp = NULL;
#ifdef ENABLE_HAL_PATH
  struct hal_if_counters if_stats;
#endif /* ENABLE_HAL_PATH */
  struct nsm_bridge_port *br_port;

  br_port = NULL;

#ifdef ENABLE_HAL_PATH
  pal_mem_set (&if_stats, 0, sizeof (if_stats));
#endif /* ENABLE_HAL_PATH */

  /* validate the index */
  ret = l2_snmp_index_get (vp, name, length, &index, exact);
  if (ret < 0)
     return NULL;

  br = nsm_snmp_get_first_bridge();
  if (br == NULL)
    return NULL;

  ifp = nsm_snmp_bridge_port_lookup_by_port_id (br, index, exact, &br_port);

  if (ifp == NULL || br_port == NULL)
    return NULL;

#ifdef ENABLE_HAL_PATH
  hal_if_get_counters (ifp->ifindex, &if_stats);
#endif /* ENABLE_HAL_PATH */

  if (!exact)
  {
    l2_snmp_index_set (vp, name, length, br_port->port_id);
  }

  switch (vp->magic) {
    case DOT1DTPPORTINOVERFLOWFRAMES:
#ifdef ENABLE_HAL_PATH
      XSTP_SNMP_RETURN_INTEGER (if_stats.good_pkts_rcv.l[1]);
#else
      XSTP_SNMP_RETURN_INTEGER (0);
#endif /* ENABLE_HAL_PATH */
      break;

    case DOT1DTPPORTOUTOVERFLOWFRAMES:
#ifdef ENABLE_HAL_PATH
      XSTP_SNMP_RETURN_INTEGER (if_stats.good_pkts_sent.l[1]);
#else
      XSTP_SNMP_RETURN_INTEGER (0);
#endif /* ENABLE_HAL_PATH */
      break;

    case DOT1DTPPORTINOVERFLOWDISCARDS:
#ifdef ENABLE_HAL_PATH
      XSTP_SNMP_RETURN_INTEGER (if_stats.bad_pkts_rcv.l[1]);
#else
      XSTP_SNMP_RETURN_INTEGER (0);
#endif /* ENABLE_HAL_PATH */
      break;
  default:
        pal_assert (0);
        break;
  }

  return NULL;
}
#endif /* HAVE_VLAN */

/*
 * xstp_snmp_dot1dTpFdbTable():
 */
unsigned char *
xstp_snmp_dot1dTpFdbTable (struct variable *vp,
                           oid * name,
                           size_t * length,
                           int exact,
                           size_t * var_len, WriteMethod ** write_method,
                           u_int32_t vr_id)
{
  int ret;
  struct mac_addr index;
  struct fdb_entry *fdb;
 
  /* validate the index */
  ret = l2_snmp_mac_addr_index_get (vp, name, length, &index, exact);
  if (ret < 0)
    return NULL;

  /* get the fdb table */
  ret = xstp_snmp_update_fdb_cache ( &fdb_cache,
                                 XSTP_FDB_TABLE_BOTH | XSTP_FDB_TABLE_UNIQUE_MAC,
                                 DOT1DTPFDBSTATUS );
  if (ret < 0)
    return NULL;

  if (fdb_cache->entries == 0)
    return NULL;

  fdb = xstp_snmp_fdb_mac_addr_lookup ( fdb_cache, &index, exact);
  if (fdb == NULL)
    return NULL;

  if (!exact)
    {
      l2_snmp_mac_addr_index_set (vp, name, length,
                                   (struct mac_addr *)(fdb->mac_addr));
    }

/*
 * this is where we do the value assignments for the mib results.
 */

  switch (vp->magic)
    {
    case DOT1DTPFDBADDRESS:
      XSTP_SNMP_RETURN_MACADDRESS (fdb->mac_addr);
      break;

    case DOT1DTPFDBPORT:
      XSTP_SNMP_RETURN_INTEGER (fdb->port_no);   
      break;

    case DOT1DTPFDBSTATUS:
      XSTP_SNMP_RETURN_INTEGER (fdb->snmp_status); 

    default:
      pal_assert(0);
    }
  return NULL;
}

/*
 * return a ptr to a fdb entry with a matching mac address (exact) or the
 * next greatest mac address (inexact).  if none found, returns null.
 */

struct fdb_entry *
xstp_snmp_fdb_mac_addr_lookup ( XSTP_FDB_CACHE *cache, struct mac_addr *addr,
                                int exact )
{

  struct fdb_entry **p_fdb_entry;

  if ( exact )
    p_fdb_entry = (struct fdb_entry **)bsearch(addr, cache->fdb_sorted, cache->entries,
             sizeof(cache->fdb_sorted[0]), xstp_snmp_fdb_mac_addr_lookup_cmp );
  else
    p_fdb_entry = (struct fdb_entry **)l2_bsearch_next(addr, cache->fdb_sorted, cache->entries,
             sizeof(cache->fdb_sorted[0]), xstp_snmp_fdb_mac_addr_lookup_cmp );

  if ( p_fdb_entry )
    return( *p_fdb_entry );
  else
    return NULL;

}

/* comparison function for searching MAC addresses in the L2 Fdb table */

int
xstp_snmp_fdb_mac_addr_lookup_cmp(const void *e1, const void *e2)
{

  return ( pal_mem_cmp ( ((struct mac_addr *)e1),
                         (*(struct fdb_entry **)e2)->mac_addr,
                         sizeof( (*(struct fdb_entry **)e2)->mac_addr) ));

}

/* To get the count of dynamic Unicast & Multicast Entries */
int
xstp_get_dynamic_count(char *bridge_name, hal_get_fdb_func_t fdb_get_func)
{
  char l_mac_addr [ETHER_ADDR_LEN];
  struct hal_fdb_entry hfdbInfo[HAL_MAX_L2_FDB_ENTRIES];
  int ret = 0;
  int dynamic_cnt = 0;
  unsigned short vid = 0;
  bool_t first_call = PAL_TRUE;

  do
   {
     if (first_call)
       {
         pal_mem_set (l_mac_addr, 0, ETHER_ADDR_LEN);
         ret = fdb_get_func (bridge_name, l_mac_addr, vid,
                             HAL_MAX_L2_FDB_ENTRIES, hfdbInfo);
         first_call = PAL_FALSE;
       }
     else
       {
         ret = fdb_get_func (bridge_name, l_mac_addr, vid,
                             HAL_MAX_L2_FDB_ENTRIES, hfdbInfo);
       }

     if ( ret < 0 )
       return 0;

     dynamic_cnt = dynamic_cnt + ret;

     pal_mem_cpy (l_mac_addr, hfdbInfo[ret - 1].mac_addr, ETHER_ADDR_LEN);
     vid = hfdbInfo[ret - 1].vid;

   } while ( (ret == HAL_MAX_L2_FDB_ENTRIES ) );

  return dynamic_cnt;
}

int xstp_get_dynamic_entries(char *bridge_name, hal_get_fdb_func_t fdb_get_func,
                             int start_index, XSTP_FDB_CACHE **cache,
                             int max_count, int status_object)
{
  char l_mac_addr [ETHER_ADDR_LEN];
  struct hal_fdb_entry hfdbInfo[HAL_MAX_L2_FDB_ENTRIES];
  int ret = 0;
  int i = 0, j = 0;
  unsigned short vid = 0;
  bool_t first_call = PAL_TRUE;

  do
   {
     if (first_call)
       {
         pal_mem_set (l_mac_addr, 0, ETHER_ADDR_LEN);
         ret = fdb_get_func (bridge_name, l_mac_addr, vid,
                             HAL_MAX_L2_FDB_ENTRIES, hfdbInfo);
         first_call = PAL_FALSE;
       }
     else
       {
         ret = fdb_get_func (bridge_name, l_mac_addr, vid,
                             HAL_MAX_L2_FDB_ENTRIES, hfdbInfo);
       }

     if ( ret < 0 )
       return j;

     for ( i = 0 ; i < ret ; i++, j++ )
       {
          if ( (j + start_index) > max_count )
            break;
          ((*cache)->fdbs)[start_index + j].vid = hfdbInfo[i].vid;
          ((*cache)->fdbs)[start_index + j].is_fwd = hfdbInfo[i].is_forward;
          ((*cache)->fdbs)[start_index + j].port_no = hfdbInfo[i].port;
          ((*cache)->fdbs)[start_index + j].is_local = hfdbInfo[i].is_local;
          pal_mem_cpy (((*cache)->fdbs)[start_index + j].mac_addr,
                       hfdbInfo[i].mac_addr,ETHER_ADDR_LEN);
          switch ( status_object )
            {
               case DOT1DTPFDBSTATUS:
                   if (((*cache)->fdbs)[start_index + j].is_local)
                     { 
                       ((*cache)->fdbs)[start_index + j].snmp_status = DOT1DTPFDBSTATUS_SELF;
                       break;
                     }
                   ((*cache)->fdbs)[start_index + j].snmp_status = DOT1DTPFDBSTATUS_LEARNED;
                   break;
	       case DOT1DSTATICSTATUS:
		   /* Dynamic entries status should be Delete on Timeout. */
		   ((*cache)->fdbs)[start_index + j].snmp_status = DOT1DSTATICSTATUS_DELETEONTIMEOUT;
                  break;
               default:
                  pal_assert(0);
                  break;
             }
       }

     pal_mem_cpy (l_mac_addr, hfdbInfo[ret - 1].mac_addr, ETHER_ADDR_LEN);
     vid = hfdbInfo[ret - 1].vid;

   } while ( (ret == HAL_MAX_L2_FDB_ENTRIES ) );

  return j;

}

/*
 * updates fdb entries in a locally cached copy of the L2 Fdb.  This table
 * includes both dynamic and static entries as required by the dot1dTpFdbTable.
 * Returns -1 on error, else 0.
 */

int
xstp_snmp_update_fdb_cache ( XSTP_FDB_CACHE **cache, int options, int status_object )
{
  int result = -1;                /* assume failure */
  int static_cnt = 0;
  int dynamic_cnt = 0;
  int temp_cnt = 0;
  int i = 0, j, removed;
  struct ptree_node *pn = NULL;
  struct nsm_bridge_static_fdb *static_fdb = NULL;
  struct nsm_bridge_fdb_ifinfo *pInfo = NULL;
#ifdef HAVE_VLAN
  struct ptree *list = NULL;
  struct avl_node *rn = NULL;
  struct nsm_vlan *vlan = NULL;
#endif /* HAVE_VLAN */
  struct nsm_bridge *br = nsm_snmp_get_first_bridge();
  int k = 0;

  if (!br)
    return -1;

  if (*cache != NULL)
    (*cache)->entries = 0;

  do                              /* once thru loop */
    {

      /* if the local table hasn't been allocated, try to do it now. */

      if ( *cache == NULL )
        {
          *cache = XCALLOC (MTYPE_L2_SNMP_FDB, sizeof (XSTP_FDB_CACHE) );

          /* Initializing all Pointers to NULL */
          for (k=0; k<HAL_MAX_VLAN_ID; k++)
            {
              (*cache)->fdb_sorted[k] = NULL;

              (*cache)->fdb_temp_sorted[k] = NULL;
            }
         }

      if ( *cache == NULL )
        break;

      if ( options & XSTP_FDB_TABLE_DYNAMIC  )
        {
#ifdef HAVE_HAL
          dynamic_cnt = dynamic_cnt + xstp_get_dynamic_count(br->name,
                                                     hal_l2_fdb_unicast_get);
          dynamic_cnt = dynamic_cnt + xstp_get_dynamic_count(br->name,
                                                   hal_l2_fdb_multicast_get);
#endif /* HAVE_HAL */
        }

      if ( options & XSTP_FDB_TABLE_STATIC )
        {
          if ( nsm_bridge_is_vlan_aware(br->master, br->name) == PAL_FALSE )
            {
               if (!br->static_fdb_list)
                 break;

               for (pn = ptree_top (br->static_fdb_list); pn; pn = ptree_next (pn))
                 {
                   if (! (static_fdb = pn->info) )
                     continue;

                   pInfo = static_fdb->ifinfo_list;
                   while (pInfo)
                     {
                        static_cnt++;
                        pInfo = pInfo->next_if;
                     }
                  }

                if ((static_cnt+dynamic_cnt) == 0)
                  return 0;
                if (((*cache)->fdbs) != NULL)
                {
                   XFREE (MTYPE_TMP,((*cache)->fdbs));
                   (*cache)->fdbs = NULL;
                }


                (*cache)->fdbs = XCALLOC (MTYPE_TMP,
                           (static_cnt+dynamic_cnt) * sizeof(struct fdb_entry));

                if ( (*cache)->fdbs == NULL )
                  return -1;

                for (pn = ptree_top(br->static_fdb_list);pn;pn = ptree_next(pn))
                  {
                    if (! (static_fdb = pn->info) )
                      continue;
                    pInfo = static_fdb->ifinfo_list;
                    while (pInfo)
                       {
                         if (i > static_cnt )
                           break;
                         ((*cache)->fdbs)[i].vid = NSM_VLAN_NULL_VID;
                         ((*cache)->fdbs)[i].is_fwd = pInfo->is_forward;
                         ((*cache)->fdbs)[i].is_static = 1 ;
                         ((*cache)->fdbs)[i].port_no = pInfo->ifindex;
                         pal_mem_cpy (((*cache)->fdbs)[i].mac_addr,
                                      static_fdb->mac_addr,ETHER_ADDR_LEN);
                         switch ( status_object )
                           {
                             case DOT1DTPFDBSTATUS:
                                if ( pInfo->type == CLI_MAC)
                                  ((*cache)->fdbs)[i].snmp_status = DOT1DTPFDBSTATUS_MGMT;
                                else
                                  ((*cache)->fdbs)[i].snmp_status = DOT1DTPFDBSTATUS_LEARNED;
                                break;

                             case DOT1DSTATICSTATUS:
                                /* Retreive the status from static_fdb and set the same to entry . */
                                ((*cache)->fdbs)[i].snmp_status = static_fdb->snmp_status;
                                break;
                             default:
                               pal_assert(0);
                               break;
                          }
                         pInfo = pInfo->next_if;
                         i++;
                       }
                     if (i > static_cnt )
                       break;
                   }
              }
#ifdef HAVE_VLAN
              else
                {
                  if (!br->vlan_table)
                    break;
                  for (rn = avl_top(br->vlan_table); rn; rn = avl_next (rn))
                    {
                      if (!(vlan = rn->info) || !(list = vlan->static_fdb_list))
                        continue;

                      for (pn = ptree_top (list); pn; pn = ptree_next (pn))
                        {
                           if (! (static_fdb = pn->info) )
                             continue;
                           pInfo = static_fdb->ifinfo_list;
                           while (pInfo)
                             {

                               static_cnt++;
                               pInfo = pInfo->next_if;
                             }
                        }
                    }

                  if (((*cache)->fdbs) != NULL)
                   {
                    XFREE (MTYPE_TMP,((*cache)->fdbs));
                    (*cache)->fdbs = NULL;
                   }


                  if ((static_cnt + dynamic_cnt) == 0)
                    return 0;

                  (*cache)->fdbs = XCALLOC (MTYPE_TMP,
                        (static_cnt + dynamic_cnt) * sizeof(struct fdb_entry));

                  if ( (*cache)->fdbs == NULL )
                     return -1;

                  for (rn = avl_top(br->vlan_table); rn; rn = avl_next (rn))
                    {
                      if (!(vlan = rn->info) || !(list = vlan->static_fdb_list))
                        continue;

                      for (pn = ptree_top (list); pn; pn = ptree_next (pn))
                        {
                           if (! (static_fdb = pn->info) )
                             continue;
                           pInfo = static_fdb->ifinfo_list;
                           while (pInfo)
                             {
                               if (i > static_cnt )
                                 break;
                               ((*cache)->fdbs)[i].vid = vlan->vid;
                               ((*cache)->fdbs)[i].is_fwd = pInfo->is_forward;
                               ((*cache)->fdbs)[i].is_static = 1 ;
                               ((*cache)->fdbs)[i].port_no = pInfo->ifindex;
                               pal_mem_cpy (((*cache)->fdbs)[i].mac_addr,
                                           static_fdb->mac_addr,ETHER_ADDR_LEN);

                               switch ( status_object )
                                 {
                                   case DOT1DTPFDBSTATUS:
                                     if ( pInfo->type == CLI_MAC)
                                       ((*cache)->fdbs)[i].snmp_status = DOT1DTPFDBSTATUS_MGMT;
                                     else
                                       ((*cache)->fdbs)[i].snmp_status = DOT1DTPFDBSTATUS_LEARNED;
                                     break;
                                   case DOT1DSTATICSTATUS:
                                     /* Retreive the status from static_fdb and set the same to entry . */
                                     ((*cache)->fdbs)[i].snmp_status = static_fdb->snmp_status;
                                    break;
                                   default:
                                     pal_assert(0);
                                   break;
                                  }
                                pInfo = pInfo->next_if;
                                i++;
                             }
                            if (i > static_cnt )
                              break;
                          }
                        if (i > static_cnt )
                          break;
                    }
                }
#endif /* HAVE_VLAN */
        }

      if ( options & XSTP_FDB_TABLE_DYNAMIC  )
        {
#ifdef HAVE_HAL
          temp_cnt = xstp_get_dynamic_entries (br->name, hal_l2_fdb_unicast_get,
                                              static_cnt, cache,
                                              static_cnt + dynamic_cnt,
                                              status_object);
          temp_cnt = xstp_get_dynamic_entries (br->name,
                                               hal_l2_fdb_multicast_get,
                                               static_cnt + temp_cnt,
                                               cache, static_cnt + dynamic_cnt,
                                               status_object);
#endif /* HAVE_HAL */
        }

      (*cache)->entries = static_cnt + dynamic_cnt;

      /* create a table of pointers to the fdbs to be sorted */

      for ( i=0; i<(*cache)->entries; i++ )
        {
           if ((*cache)->fdb_sorted[i] == NULL)
             (*cache)->fdb_sorted[i] = XCALLOC (MTYPE_TMP,
                                                sizeof(struct fdb_entry));

           if ((*cache)->fdb_temp_sorted[i] == NULL)
             (*cache)->fdb_temp_sorted[i] = XCALLOC (MTYPE_TMP,
                                                     sizeof(struct fdb_entry));

          (*cache)->fdb_sorted[i] = &((*cache)->fdbs[i]);
        }

      /* sort the fdb_sorted table my MAC address and port number */

      pal_qsort ( (*cache)->fdb_sorted, (*cache)->entries,
                  sizeof( ((*cache)->fdb_sorted)[0] ),
                  xstp_snmp_fdb_mac_addr_cmp );

      /* remove entries with duplicate mac addresses */
      if ( options & XSTP_FDB_TABLE_UNIQUE_MAC )
        {
          ((*cache)->fdb_temp_sorted)[0] = ((*cache)->fdb_sorted)[0];

          for (i=1, j=0, removed=0; i<((*cache)->entries); i++)
            {

              /* if MAC addresses match, don't add entry to temp list */

              if ( pal_mem_cmp( ((*cache)->fdb_sorted)[i]->mac_addr,
                                ((*cache)->fdb_temp_sorted)[j]->mac_addr,
                                sizeof( struct mac_addr ) ) == 0 )
                {
                  removed++;
                  continue;
                }

              ((*cache)->fdb_temp_sorted)[++j] = ((*cache)->fdb_sorted)[i];

            }

          /* if anything changed, copy the temp array to the sorted array and
           * update the total entries.
           */

          if ( removed > 0 )
            {
              ((*cache)->entries) -= removed;

              pal_mem_cpy( (*cache)->fdb_sorted, (*cache)->fdb_temp_sorted,
                      ((*cache)->entries) * sizeof(((*cache)->fdb_sorted)[0]) );
            }
        }
      removed = 0;
      /* remove MULTICAST entries */
      if ( options & XSTP_FDB_TABLE_UNICAST_MAC ) {
        ((*cache)->fdb_temp_sorted)[0] = ((*cache)->fdb_sorted)[0];

        for (i=0, j=0, removed=0; i<((*cache)->entries); i++) {
          /* if MAC address is Multicast don't add entry to temp list */
          if ( ((((*cache)->fdb_sorted)[i]->mac_addr[0]) & 0x01) == 0x01 ) {
            removed++;
            continue;
          }
          ((*cache)->fdb_temp_sorted)[j++] = ((*cache)->fdb_sorted)[i];
        }

        /* if anything changed, copy the temp array to the sorted array and
         * update the total entries.
         */
        if ( removed > 0 ) {
          ((*cache)->entries) -= removed;

          pal_mem_cpy( (*cache)->fdb_sorted, (*cache)->fdb_temp_sorted,
                      ((*cache)->entries) * sizeof(((*cache)->fdb_sorted)[0]) );
        }
      }

      removed = 0;
      /* remove UNICAST entries */
      if ( options & XSTP_FDB_TABLE_MULTICAST_MAC ) {
        ((*cache)->fdb_temp_sorted)[0] = ((*cache)->fdb_sorted)[0];
        for (i=0, j=0, removed=0; i<((*cache)->entries); i++) {
          /* if MAC address is Multicast don't add entry to temp list */
          if ( ((((*cache)->fdb_sorted)[i]->mac_addr[0]) & 0x01) == 0x00 ) {
            removed++;
            continue;
          }
          ((*cache)->fdb_temp_sorted)[j++] = ((*cache)->fdb_sorted)[i];
        }

        /* if anything changed, copy the temp array to the sorted array and
         * update the total entries.
         */
        if ( removed > 0 ) {
          ((*cache)->entries) -= removed;

          pal_mem_cpy( (*cache)->fdb_sorted, (*cache)->fdb_temp_sorted,
                       ((*cache)->entries) * sizeof(((*cache)->fdb_sorted)[0]) );
        }
      }

      /* show cache refresh time */

      pal_time_current( &((*cache)->time_updated) );

      result = 0;

    } while ( PAL_FALSE );

  return( result );

}

/* comparison function for sorting MAC addresses in the L2 Fdb table. primary index
 * is the MAC address, secondary is the port number.
 */

int
xstp_snmp_fdb_mac_addr_cmp(const void *e1, const void *e2)
{
  int result;
  struct fdb_entry *fdb1 = *(struct fdb_entry **)e1;
  struct fdb_entry *fdb2 = *(struct fdb_entry **)e2;

  result = pal_mem_cmp ( fdb1->mac_addr, fdb2->mac_addr, sizeof( fdb2->mac_addr) );
  if ( result == 0)
    {
      if ( fdb1->port_no < fdb2->port_no )
        return -1;
      else if ( fdb1->port_no > fdb2->port_no )
        return 1;
      else
        return 0;
    }
  return result;

}

/*
 * updates fdb entries in a locally cached copy of the L2 Fdb.  This table
 * includes both dynamic and static entries as required by the dot1dTpFdbTable.
 * sorts based on vid and mac addr
 * Returns -1 on error, else 0.
 */
int
xstp_snmp_update_fdb_cache2 ( XSTP_FDB_CACHE **cache, int options,
                              int status_object )
{
  int result = -1;                /* assume failure */
  int static_cnt = 0;
  int dynamic_cnt = 0;
  int temp_cnt = 0;
  int i = 0, j, removed;
  struct ptree_node *pn = NULL;
  struct nsm_bridge_static_fdb *static_fdb = NULL;
  struct nsm_bridge_fdb_ifinfo *pInfo = NULL;
#ifdef HAVE_VLAN
  struct ptree *list = NULL;
  struct avl_node *rn = NULL;
  struct nsm_vlan *vlan = NULL;
#endif /* HAVE_VLAN */
  struct nsm_bridge *br = nsm_snmp_get_first_bridge();
  int k = 0;

  if (!br)
    return -1;

  if (*cache != NULL)
    (*cache)->entries = 0;

  do                              /* once thru loop */
    {

      /* if the local table hasn't been allocated, try to do it now. */

      if ( *cache == NULL )
        {
          *cache = XCALLOC (MTYPE_L2_SNMP_FDB, sizeof (XSTP_FDB_CACHE) );

          /* Initializing all Pointers to NULL */
          for (k=0; k<HAL_MAX_VLAN_ID; k++)
            {
              (*cache)->fdb_sorted[k] = NULL;

              (*cache)->fdb_temp_sorted[k] = NULL;
            }
         }

      if ( *cache == NULL )
        break;

      if ( options & XSTP_FDB_TABLE_DYNAMIC  )
        {
#ifdef HAVE_HAL
          dynamic_cnt = dynamic_cnt + xstp_get_dynamic_count(br->name,
                                                     hal_l2_fdb_unicast_get);
          dynamic_cnt = dynamic_cnt + xstp_get_dynamic_count(br->name,
                                                   hal_l2_fdb_multicast_get);
#endif /* HAVE_HAL */
        }

      if ( options & XSTP_FDB_TABLE_STATIC )
        {
          if ( nsm_bridge_is_vlan_aware(br->master, br->name) == PAL_FALSE )
            {
               if (!br->static_fdb_list)
                 break;

               for (pn = ptree_top (br->static_fdb_list); pn;
                    pn = ptree_next (pn))
                 {
                   if (! (static_fdb = pn->info) )
                     continue;

                   pInfo = static_fdb->ifinfo_list;
                   while (pInfo)
                     {
                        static_cnt++;
                        pInfo = pInfo->next_if;
                     }
                  }

                if ((static_cnt+dynamic_cnt) == 0)
                  return 0;
                if (((*cache)->fdbs) != NULL)
                   XFREE (MTYPE_TMP,((*cache)->fdbs));

                (*cache)->fdbs = XCALLOC (MTYPE_TMP,
                           (static_cnt+dynamic_cnt) * sizeof(struct fdb_entry));

                if ( (*cache)->fdbs == NULL )
                  return -1;

                for (pn = ptree_top(br->static_fdb_list);pn;pn = ptree_next(pn))
                  {
                    if (! (static_fdb = pn->info) )
                      continue;
                    pInfo = static_fdb->ifinfo_list;
                    while (pInfo)
                       {
                         if (i > static_cnt )
                           break;
                         ((*cache)->fdbs)[i].vid = NSM_VLAN_NULL_VID;
                         ((*cache)->fdbs)[i].is_fwd = pInfo->is_forward;
                         ((*cache)->fdbs)[i].is_static = 1 ;
                         ((*cache)->fdbs)[i].port_no = pInfo->ifindex;
                         pal_mem_cpy (((*cache)->fdbs)[i].mac_addr,
                                      static_fdb->mac_addr,ETHER_ADDR_LEN);

                         switch ( status_object )
                           {
                             case DOT1DTPFDBSTATUS:
                                if ( pInfo->type == CLI_MAC)
                                  ((*cache)->fdbs)[i].snmp_status =
                                                          DOT1DTPFDBSTATUS_MGMT;
                                else
                                  ((*cache)->fdbs)[i].snmp_status =
                                                       DOT1DTPFDBSTATUS_LEARNED;
                                break;

                             case DOT1DSTATICSTATUS:
                                /* all static entries are permanent. */
                                ((*cache)->fdbs)[i].snmp_status =
                                                    static_fdb->snmp_status;
                                break;
                             default:
                               pal_assert(0);
                               break;
                          }
                         pInfo = pInfo->next_if;
                         i++;
                       }
                     if (i > static_cnt )
                       break;
                   }
              }
#ifdef HAVE_VLAN
              else
                {
                  if (!br->vlan_table)
                    break;
                  for (rn = avl_top(br->vlan_table); rn; rn = avl_next (rn))
                    {
                      if (!(vlan = rn->info) || !(list = vlan->static_fdb_list))
                        continue;

                      for (pn = ptree_top (list); pn; pn = ptree_next (pn))
                        {
                           if (! (static_fdb = pn->info) )
                             continue;
                           pInfo = static_fdb->ifinfo_list;
                           while (pInfo)
                             {
                               static_cnt++;
                               pInfo = pInfo->next_if;
                             }
                        }
                    }

                  if ((static_cnt + dynamic_cnt) == 0)
                    return 0;
                if (((*cache)->fdbs) != NULL)
                   XFREE (MTYPE_TMP,((*cache)->fdbs));

                  (*cache)->fdbs = XCALLOC (MTYPE_TMP,
                        (static_cnt + dynamic_cnt) * sizeof(struct fdb_entry));

                  if ( (*cache)->fdbs == NULL )
                     return -1;

                  for (rn = avl_top(br->vlan_table); rn; rn = avl_next (rn))
                    {
                      if (!(vlan = rn->info) || !(list = vlan->static_fdb_list))
                        continue;

                      for (pn = ptree_top (list); pn; pn = ptree_next (pn))
                        {
                           if (! (static_fdb = pn->info) )
                             continue;
                           pInfo = static_fdb->ifinfo_list;
                           while (pInfo)
                             {
                               if (i > static_cnt )
                                 break;
                               ((*cache)->fdbs)[i].vid = vlan->vid;
                               ((*cache)->fdbs)[i].is_fwd = pInfo->is_forward;
                               ((*cache)->fdbs)[i].is_static = 1;
                               ((*cache)->fdbs)[i].port_no = pInfo->ifindex;
                               pal_mem_cpy (((*cache)->fdbs)[i].mac_addr,
                                           static_fdb->mac_addr,ETHER_ADDR_LEN);
                               switch ( status_object )
                                 {
                                   case DOT1DTPFDBSTATUS:
                                     if ( pInfo->type == CLI_MAC)
                                       ((*cache)->fdbs)[i].snmp_status =
                                                          DOT1DTPFDBSTATUS_MGMT;
                                     else
                                       ((*cache)->fdbs)[i].snmp_status =
                                                       DOT1DTPFDBSTATUS_LEARNED;
                                     break;
                                   case DOT1DSTATICSTATUS:
                                     /* all static entries are permanent. */
                                     ((*cache)->fdbs)[i].snmp_status =
                                                    static_fdb->snmp_status;
                                    break;
                                   default:
                                     pal_assert(0);
                                   break;
                                  }
                                pInfo = pInfo->next_if;
                                i++;
                             }
                            if (i > static_cnt )
                              break;
                          }
                        if (i > static_cnt )
                          break;
                    }
                }
#endif /* HAVE_VLAN */
        }

      if ( options & XSTP_FDB_TABLE_DYNAMIC  )
        {
#ifdef HAVE_HAL
          temp_cnt = xstp_get_dynamic_entries (br->name, hal_l2_fdb_unicast_get,
                                              static_cnt, cache,
                                              static_cnt + dynamic_cnt,
                                              status_object);
          temp_cnt = xstp_get_dynamic_entries (br->name,
                                               hal_l2_fdb_multicast_get,
                                               static_cnt + temp_cnt,
                                               cache, static_cnt + dynamic_cnt,
                                               status_object);
#endif /* HAVE_HAL */
        }

      (*cache)->entries = static_cnt + dynamic_cnt;

      /* create a table of pointers to the fdbs to be sorted */
      for ( i=0; i<(*cache)->entries; i++ )
        {
           if ((*cache)->fdb_sorted[i] == NULL)
             (*cache)->fdb_sorted[i] = XCALLOC (MTYPE_TMP,
                                                sizeof(struct fdb_entry));

           if ((*cache)->fdb_temp_sorted[i] == NULL)
             (*cache)->fdb_temp_sorted[i] = XCALLOC (MTYPE_TMP,
                                                    sizeof(struct fdb_entry));

           (*cache)->fdb_sorted[i] = &((*cache)->fdbs[i]);
        }

      /* sort the fdb_sorted table my MAC address and port number */

      pal_qsort ( (*cache)->fdb_sorted, (*cache)->entries,
                  sizeof( ((*cache)->fdb_sorted)[0] ),
                  xstp_snmp_fdb_vid_mac_addr_cmp );

      /* remove entries with duplicate mac addresses */
      if ( options & XSTP_FDB_TABLE_UNIQUE_MAC )
        {
          ((*cache)->fdb_temp_sorted)[0] = ((*cache)->fdb_sorted)[0];

          for (i=1, j=0, removed=0; i<((*cache)->entries); i++)
            {

              /* if MAC addresses match, don't add entry to temp list */

              if ( pal_mem_cmp( ((*cache)->fdb_sorted)[i]->mac_addr,
                                ((*cache)->fdb_temp_sorted)[j]->mac_addr,
                                sizeof( struct mac_addr ) ) == 0 )
                {
                  removed++;
                  continue;
                }

              ((*cache)->fdb_temp_sorted)[++j] = ((*cache)->fdb_sorted)[i];

            }

          /* if anything changed, copy the temp array to the sorted array and
           * update the total entries.
           */

          if ( removed > 0 )
            {
              ((*cache)->entries) -= removed;

              pal_mem_cpy( (*cache)->fdb_sorted, (*cache)->fdb_temp_sorted,
                      ((*cache)->entries) * sizeof(((*cache)->fdb_sorted)[0]) );
            }
        }
      removed = 0;
      /* remove MULTICAST entries */
      if ( options & XSTP_FDB_TABLE_UNICAST_MAC ) {
        ((*cache)->fdb_temp_sorted)[0] = ((*cache)->fdb_sorted)[0];

        for (i=0, j=0, removed=0; i<((*cache)->entries); i++) {
          /* if MAC address is Multicast don't add entry to temp list */
          if ( ((((*cache)->fdb_sorted)[i]->mac_addr[0]) & 0x01) == 0x01 ) {
            removed++;
            continue;
          }
          ((*cache)->fdb_temp_sorted)[j++] = ((*cache)->fdb_sorted)[i];
        }

        /* if anything changed, copy the temp array to the sorted array and
         * update the total entries.
         */
        if ( removed > 0 ) {
          ((*cache)->entries) -= removed;

          pal_mem_cpy( (*cache)->fdb_sorted, (*cache)->fdb_temp_sorted,
                      ((*cache)->entries) * sizeof(((*cache)->fdb_sorted)[0]) );
        }
      }

      removed = 0;
      /* remove UNICAST entries */
      if ( options & XSTP_FDB_TABLE_MULTICAST_MAC ) {
        ((*cache)->fdb_temp_sorted)[0] = ((*cache)->fdb_sorted)[0];
        for (i=0, j=0, removed=0; i<((*cache)->entries); i++) {
          /* if MAC address is Multicast don't add entry to temp list */
          if ( ((((*cache)->fdb_sorted)[i]->mac_addr[0]) & 0x01) == 0x00 ) {
            removed++;
            continue;
          }
          ((*cache)->fdb_temp_sorted)[j++] = ((*cache)->fdb_sorted)[i];
        }

        /* if anything changed, copy the temp array to the sorted array and
         * update the total entries.
         */
        if ( removed > 0 ) {
          ((*cache)->entries) -= removed;

          pal_mem_cpy( (*cache)->fdb_sorted, (*cache)->fdb_temp_sorted,
                       ((*cache)->entries) * sizeof(((*cache)->fdb_sorted)[0]) );
        }
      }

      /* show cache refresh time */

      pal_time_current( &((*cache)->time_updated) );

      result = 0;

    } while ( PAL_FALSE );

  return( result );

}

/* comparison function for sorting MAC addresses in the L2 Fdb table. primary index
 * is the vid, secondary is the MAC address.
 */

int
xstp_snmp_fdb_vid_mac_addr_cmp(const void *e1, const void *e2)
{
  int result1, result2;
  struct fdb_entry *fdb1 = *(struct fdb_entry **)e1;
  struct fdb_entry *fdb2 = *(struct fdb_entry **)e2;

  result1 = fdb1->vid - fdb2->vid;
  result2 = pal_mem_cmp ( fdb1->mac_addr, fdb2->mac_addr,
                          sizeof( fdb2->mac_addr) );

  if ( result1 == 0)
    {
      if ( result2 < 0 )
        return -1;
      else if ( result2 > 0 )
        return 1;
      else
        return 0;
    }
  return result1;
}

unsigned char *
xstp_snmp_dot1dStaticTable (struct variable *vp,
                            oid * name,
                            size_t * length,
                            int exact,
                            size_t * var_len, WriteMethod ** write_method,
                            u_int32_t vr_id)
{
  int ret, i = 0;
  u_int8_t port_id;
  struct fdb_entry *fdb = NULL;
  struct mac_addr_and_port key;
  int port;

  /* for row-create tables, the write methods need to be set before any returns
   */

  switch ( vp->magic )
  {
    case DOT1DSTATICADDRESS:
    *write_method = xstp_snmp_write_dot1dStaticTable;
    break;

    case DOT1DSTATICRECEIVEPORT:
    *write_method = xstp_snmp_write_dot1dStaticTable;
    break;

    case DOT1DSTATICALLOWEDTOGOTO:
    *write_method = xstp_snmp_write_dot1dStaticAllowedToGoTo;
    break;

    case DOT1DSTATICSTATUS:
    *write_method = xstp_snmp_write_dot1dStaticStatus;
    break;

    default:
      break;
  }

  /* validate the index */
  ret = l2_snmp_mac_addr_int_index_get (vp, name, length,
                                        &key.addr, &port, exact);
  if (ret < 0)
    return NULL;

  if (fdb_static_cache)
    fdb_static_cache->entries = 0;

  /* get a cached fdb static table */
  ret = xstp_snmp_update_fdb_cache ( &fdb_static_cache, XSTP_FDB_TABLE_BOTH,
                                    DOT1DSTATICSTATUS );
  if (ret < 0)
    return NULL;

  if (fdb_static_cache->entries == 0)
    return NULL;

  /* we only allow received port = 0. the fdb table has destination ports
   * in it for static entries.
   */
  if ( exact )
   {
    /* if the port index is zero return the first matching MAC address entry
     * if any and ignore the port.
     */

    if ( port == 0 )
      fdb = xstp_snmp_fdb_mac_addr_lookup (fdb_static_cache, &key.addr, 1 );
    else
      fdb = NULL;
  }
  else {
  /*
   * if inexact and the user asked for any port > 0, force a
   * return of the next FDB with a different MAC addr, if any.
   */
    key.port = 0;

    fdb = xstp_snmp_fdb_mac_addr_port_lookup (fdb_static_cache, &key, exact);
  }

  if (fdb == NULL)
    return NULL;

  if (!exact)
    {
      l2_snmp_mac_addr_int_index_set (vp, name, length,
                                       (struct mac_addr *)fdb->mac_addr, 0 );
    }

  switch (vp->magic)
    {
    case DOT1DSTATICADDRESS:
      XSTP_SNMP_RETURN_MACADDRESS (fdb->mac_addr);
      break;

    case DOT1DSTATICRECEIVEPORT:
      XSTP_SNMP_RETURN_INTEGER (0);
      break;

    case DOT1DSTATICALLOWEDTOGOTO:

      pal_mem_set( br_ports_bitstring, 0x0, sizeof(br_ports_bitstring));

      /* find any static FDB entries where forwarding is denied and clear the
       * corresponding bit in the bit string for that port.
       */
        for ( i=0; i<(fdb_static_cache)->entries; i++ )
          {
            if ((pal_mem_cmp ( fdb->mac_addr,
                    (fdb_static_cache->fdbs)[i].mac_addr,
                    sizeof( fdb->mac_addr )) == 0 ))
              {

                port_id = nsm_snmp_bridge_port_get_port_id (
                    (fdb_static_cache->fdbs)[i].port_no);

                if ((port_id > 0 && port_id <= NSM_L2_SNMP_MAX_PORTS)
                    && ((fdb_static_cache->fdbs)[i].is_fwd == PAL_TRUE))
                  bitstring_setbit ( br_ports_bitstring, port_id - 1 );

              }
          }
     XSTP_SNMP_RETURN_STRING (br_ports_bitstring, sizeof(br_ports_bitstring));
      break;

    case DOT1DSTATICSTATUS:
      *write_method = xstp_snmp_write_dot1dStaticStatus;
      XSTP_SNMP_RETURN_INTEGER (fdb->snmp_status);
      break;

    default:
      pal_assert(0);
    }
  return NULL;
}

/*
 * return a ptr to a fdb entry with a matching mac address (exact) and port or the
 * next greatest mac address and port(inexact).  if none found, returns null.
 */

struct fdb_entry *
xstp_snmp_fdb_mac_addr_port_lookup ( XSTP_FDB_CACHE *cache,
                                    struct mac_addr_and_port *key, int exact )
{

  struct fdb_entry **p_fdb_entry; 

  if ( exact )
    p_fdb_entry = (struct fdb_entry **)bsearch(key, cache->fdb_sorted, cache->entries,
                                               sizeof(cache->fdb_sorted[0]), xstp_snmp_fdb_mac_addr_port_lookup_cmp );
  else
    p_fdb_entry = (struct fdb_entry **)l2_bsearch_next(key, cache->fdb_sorted, cache->entries,
                                                    sizeof(cache->fdb_sorted[0]), xstp_snmp_fdb_mac_addr_port_lookup_cmp );

  if ( p_fdb_entry )
    return( *p_fdb_entry );
  else
    return NULL;

}

/* comparison function for searching MAC addresses and ports in the L2 Fdb table */

int
xstp_snmp_fdb_mac_addr_port_lookup_cmp(const void *e1, const void *e2)
{
  int result;
  struct mac_addr_and_port *key = (struct mac_addr_and_port *)e1;
  struct fdb_entry *fdb = *(struct fdb_entry **)e2;

  result = pal_mem_cmp ( &key->addr, fdb->mac_addr, sizeof( fdb->mac_addr ));

  if ( result == 0 )
    {
      if ( key->port != 0 )
        {
          if (key->port < fdb->port_no)
            return -1;
          else if (key->port > fdb->port_no)
            return 1;
          else
            return 0;
        }
      else
        return 0;
    }

  return result;
}

/* comparison function for searching vid and MAC addresses in the L2 Fdb table */

int
xstp_snmp_fdb_vid_mac_addr_lookup_cmp(const void *e1, const void *e2)
{
  int result1, result2;
  struct vid_and_mac_addr *key = (struct vid_and_mac_addr *)e1;
  struct fdb_entry *fdb = *(struct fdb_entry **)e2;

  result1 = key->vid - fdb->vid;
  result2 = pal_mem_cmp ( key->addr.addr, fdb->mac_addr,
                          sizeof( fdb->mac_addr) );

   if ( key->vid == 0 )
     return -1;

   if ( result1 == 0 ) 
    {
      if ( result2 < 0 )
        return -1;
      else if ( result2 > 0 )
        return 1;
      else
        return 0;
    }
  return result1;

}

/*
 * return a ptr to a fdb entry with a matching vid and mac address(exact) or the
 * next greatest vid and mac address (inexact).  if none found, returns null.
 */

struct fdb_entry *
xstp_snmp_fdb_vid_mac_addr_lookup ( XSTP_FDB_CACHE *cache,
                                   struct vid_and_mac_addr *key, int exact )
{

  struct fdb_entry **p_fdb_entry;

  if ( exact )
    p_fdb_entry = (struct fdb_entry **)bsearch(key, cache->fdb_sorted,
                                               cache->entries,
                                               sizeof(cache->fdb_sorted[0]),
                                               xstp_snmp_fdb_vid_mac_addr_lookup_cmp );
  else
    p_fdb_entry = (struct fdb_entry **)l2_bsearch_next
                                         (key, cache->fdb_sorted,
                                          cache->entries,
                                          sizeof(cache->fdb_sorted[0]),
                                          xstp_snmp_fdb_vid_mac_addr_lookup_cmp );

  if ( p_fdb_entry )
    return( *p_fdb_entry );
  else
    return NULL;

}

/* comparison function for searching vid and MAC addresses in the L2 Fdb table */

int
xstp_snmp_fdb_vid_mac_addr_port_lookup_cmp(const void *e1, const void *e2)
{
  int result1, result2;
  struct vid_and_mac_addr_and_port *key = (struct vid_and_mac_addr_and_port *)e1;
  struct fdb_entry *fdb = *(struct fdb_entry **)e2;

  result1 = key->vid - fdb->vid;
  result2 = pal_mem_cmp ( key->addr.addr, fdb->mac_addr,
                          sizeof( fdb->mac_addr) );

  if ( key->vid == 0 )
    return -1;

  if ( result1 == 0 )
    {
      if ( result2 < 0 )
        return -1;
      else if ( result2 > 0 )
        return 1;
      else
        return 0;
    }
  return result1;

}

/*
 * return a ptr to a fdb entry with a matching vid and mac address(exact)
 * and port or the
 * next greatest vid and mac address (inexact) and port.
 * if none found, returns null.
 */

struct fdb_entry *
xstp_snmp_fdb_vid_mac_addr_port_lookup ( XSTP_FDB_CACHE *cache,
                                   struct vid_and_mac_addr_and_port *key,
                                   int exact )
{

  struct fdb_entry **p_fdb_entry;

  if ( exact )
    p_fdb_entry = (struct fdb_entry **)bsearch(key, cache->fdb_sorted,
                                               cache->entries,
                                               sizeof(cache->fdb_sorted[0]),
                                 xstp_snmp_fdb_vid_mac_addr_port_lookup_cmp );
  else
    p_fdb_entry = (struct fdb_entry **)l2_bsearch_next
                                         (key, cache->fdb_sorted,
                                          cache->entries,
                                          sizeof(cache->fdb_sorted[0]),
                                 xstp_snmp_fdb_vid_mac_addr_port_lookup_cmp );

  if ( p_fdb_entry )
    return( *p_fdb_entry );
  else
    return NULL;

}

int
xstp_snmp_fdb_add_mac_to_port_list (struct mac_addr addr, u_char *portlist,
                                    u_int32_t status)
{
  int port;
  struct nsm_bridge *br;
  struct interface *ifc;
  struct apn_vr *vr = apn_vr_get_privileged (nzg);
  struct nsm_master *nm = vr->proto;
  struct nsm_bridge_master *master = nsm_bridge_get_master (nm);
  struct nsm_bridge_port *br_port;
  struct nsm_vlan_port *vlan_port = NULL;
  int ret = SNMP_ERR_GENERR;
  bool_t is_unicast;

  br_port = NULL;

  br = nsm_snmp_get_first_bridge();

  if (br == NULL)
    return SNMP_ERR_GENERR;

  if ((addr.addr[0] & 0x01) == 0x01)
    is_unicast = PAL_FALSE;
  else
    is_unicast = PAL_TRUE;

  /* for each possible port, find a matching ifName if possible and
   * set or clear a "bridge bridge_name address addr deny ifName"
   */

  for (port=1; port<=(sizeof(br_ports_bitstring)*8); port++)
     {

       /* must be an existing port on this bridge */

       if ( (ifc = nsm_snmp_bridge_port_lookup_by_port_id (br, port, 1, &br_port))
                                                       == NULL
             || br_port == NULL)
         continue;

       /* if bit is set in string, then address is allowed. remove any discard
        * entry. if not set, add a static entry indicating discard.
        */

       if ( bitstring_testbit( (char *) portlist, port-1) && (status != 2))
         {
#ifdef HAVE_VLAN
           if (nsm_bridge_is_vlan_aware(br->master, br->name) == PAL_TRUE)
             {
               vlan_port = &br_port->vlan_port;

               if (!(NSM_VLAN_BMP_IS_MEMBER(vlan_port->staticMemberBmp,
                     NSM_VLAN_DEFAULT_VID))&&
                   !(NSM_VLAN_BMP_IS_MEMBER(vlan_port->dynamicMemberBmp,
                     NSM_VLAN_DEFAULT_VID)))
                 continue;

               ret =
                nsm_bridge_add_mac (master, br->name, ifc, (u_char *)&addr,
                                    NSM_VLAN_DEFAULT_VID, 0, HAL_L2_FDB_STATIC,
                                    XSTP_FORWARD, CLI_MAC);
             }
           else
#endif /* HAVE_VLAN */
             ret =
               nsm_bridge_add_mac (master, br->name, ifc, (u_char *)&addr,
                                   NSM_DEFAULT_VID, 0, HAL_L2_FDB_STATIC,
                                   XSTP_FORWARD, CLI_MAC);

            if ((ret == 0) && (is_unicast))
              return SNMP_ERR_NOERROR;
         }
       else if (bitstring_testbit( (char *) portlist, port-1) &&
                (status == 2))
         {
#ifdef HAVE_VLAN
           if (nsm_bridge_is_vlan_aware(br->master, br->name) == PAL_TRUE)
             ret=
                nsm_bridge_delete_mac (master, br->name, ifc, (u_char *)&addr,
                                       NSM_VLAN_DEFAULT_VID, 0,
                                       HAL_L2_FDB_STATIC, XSTP_FORWARD,
                                       CLI_MAC);
           else
#endif /* HAVE_VLAN */
             ret =
               nsm_bridge_delete_mac (master, br->name, ifc, (u_char *)&addr,
                                      NSM_DEFAULT_VID, 0,
                                      HAL_L2_FDB_STATIC, XSTP_FORWARD,
                                      CLI_MAC);

            if ((ret == 0) && (is_unicast))
              return SNMP_ERR_NOERROR;
         }
     }

   return ret;
}

int
xstp_snmp_write_dot1dStaticAllowedToGoTo ( int action,
                                           u_char * var_val,
                                           u_char var_val_type,
                                           size_t var_val_len,
                                           u_char * statP,
                                           oid * name, size_t length,
                                           struct variable *vp,
                                           u_int32_t vr_id)
{
  int ret;
  struct nsm_bridge *br;
  struct nsm_if *zif = NULL;
  struct avl_node *avl_port;
  struct mac_addr_and_port key;
  struct interface *ifp = NULL;
  struct nsm_bridge_master *master = NULL;
  struct nsm_master *nm = NULL;
  int vid = NSM_DEFAULT_VID;
  struct nsm_bridge_port *br_port;
  struct nsm_vlan_port *vlan_port = NULL;
  struct apn_vr *vr = apn_vr_get_privileged (snmpg);
  struct fdb_entry *fdb = NULL;
  u_int32_t port_array[NSM_L2_SNMP_PORT_ARRAY_SIZE];
  int i = 0;
  int k = 0;

  if (! vr)
    {
      return SNMP_ERR_GENERR;
    }
   nm = vr->proto;

  if (! nm)
    {
      return SNMP_ERR_GENERR;
    }
   master = nm->bridge;

  if (! master)
    {
      return SNMP_ERR_GENERR;
    }

  if (var_val_type != ASN_OCTET_STR)
    return SNMP_ERR_WRONGTYPE;

  /* Initialize the port bit array to 0. */
  pal_mem_set(port_array, 0, NSM_L2_SNMP_PORT_ARRAY_SIZE);

  /* validate the index */
  ret = l2_snmp_mac_addr_int_index_get (vp, name, &length,
                                        &key.addr, (int *) &key.port, 1);
  if (ret < 0 || key.port != 0)
    return SNMP_ERR_NOSUCHNAME;

  if (fdb_static_cache->entries == 0)
        return SNMP_ERR_GENERR;

  fdb = xstp_snmp_fdb_mac_addr_lookup (fdb_static_cache, &key.addr, 1 );

  if (fdb == NULL)
    return SNMP_ERR_GENERR;

  switch (vp->magic)
    {
    case DOT1DSTATICALLOWEDTOGOTO:

      if (var_val_len != sizeof(br_ports_bitstring))
        return SNMP_ERR_WRONGLENGTH;

      /* must have the bridge defined */
      br = nsm_snmp_get_first_bridge ();

      if (br == NULL)
        return SNMP_ERR_GENERR;

       if (nsm_bridge_is_vlan_aware(br->master, br->name) == PAL_TRUE)
         {
           vid = NSM_VLAN_DEFAULT_VID;
         }

      /* for each possible port, find a matching ifName if possible and
       * set or clear a "bridge bridge_name address addr deny ifName"
       */
      for (avl_port = avl_getnext (br->port_tree, NULL); avl_port;
           avl_port = avl_getnext (br->port_tree, avl_port))
        {
          if ((br_port = (struct nsm_bridge_port *)
                                 AVL_NODE_INFO (avl_port)) == NULL)
            continue;

          if ((zif = br_port->nsmif) == NULL)
            continue;

          if ((ifp = zif->ifp) == NULL)
            continue;

          /*if bit is set in string, then address is allowed. remove any discard
           * entry. if not set, add a static entry indicating discard.
           */
          if (bitstring_testbit((char *)var_val, (br_port->port_id-1))
              == PAL_FALSE)
            {
              if (fdb->is_static)
                nsm_bridge_delete_mac (master, br->name, ifp,
                                       key.addr.addr,
                                       vid, 1, HAL_L2_FDB_STATIC,
                                       PAL_TRUE, CLI_MAC);
              else
                hal_l2_del_fdb (br->name, ifp->ifindex,
                                key.addr.addr,
                                HAL_HW_LENGTH,
                                NSM_VLAN_DEFAULT_VID, 1,
                                HAL_L2_FDB_DYNAMIC);

            } /* If 0 delete the mac-entry */
         else
           port_array[i++] = br_port->port_id;
        } /* For Loop */

       br_port = NULL;

       for (k = 0; k < i; k++)
         {
           if ( (ifp = nsm_snmp_bridge_port_lookup_by_port_id (br,
                                 port_array[k], 1, &br_port)) == NULL)
              break;

            if (NSM_VLAN_DEFAULT_VID == vid)
              {
                vlan_port = &br_port->vlan_port;

                if (!(NSM_VLAN_BMP_IS_MEMBER(vlan_port->staticMemberBmp,vid))&&
                    !(NSM_VLAN_BMP_IS_MEMBER(vlan_port->dynamicMemberBmp, vid)))
                  return SNMP_ERR_GENERR;
              }

            if (fdb->is_static)
                nsm_bridge_add_mac (master, br->name, ifp, key.addr.addr,
                                    vid, 1, HAL_L2_FDB_STATIC,
                                    PAL_TRUE, CLI_MAC);
              else
                hal_l2_add_fdb (br->name, ifp->ifindex,
                                key.addr.addr,
                                HAL_HW_LENGTH,
                                vid, 1, HAL_L2_FDB_DYNAMIC,
                                fdb->is_fwd);

         } /* For Loop for adding the entries. */
    break;
    default:
      return SNMP_ERR_GENERR;
    }
  return SNMP_ERR_NOERROR;
}

int
xstp_snmp_write_dot1dStaticStatus ( int action,
                                    u_char * var_val,
                                    u_char var_val_type,
                                    size_t var_val_len,
                                    u_char * statP,
                                    oid * name, size_t length,
                                    struct variable *vp,
                                    u_int32_t vr_id)
{
  int ret;
  struct mac_addr_and_port key;
  struct avl_node *avl_port;
  struct interface *ifp = NULL;
  struct nsm_bridge *br = NULL;
  struct nsm_if *zif = NULL;
  struct nsm_bridge_master *master = NULL;
  struct nsm_bridge_port *br_port;
  int intval ;
  int vid = NSM_VLAN_NULL_VID;
  int port_id = 0;
  u_char port_bit_array[4];
  u_char dynamic_port_bit_array[4];
  struct ptree_node *pn = NULL;
  struct nsm_bridge_static_fdb *static_fdb = NULL, *temp_static_fdb = NULL;
  struct nsm_bridge_fdb_ifinfo *pInfo = NULL;
  struct nsm_bridge_fdb_ifinfo *list_head;
#ifdef HAVE_VLAN
  struct ptree *list = NULL;
  struct avl_node *rn = NULL;
  struct nsm_vlan *vlan = NULL;
#endif /* HAVE_VLAN */
  struct fdb_entry *fdb = NULL;
  int flag = 0;
  int i = 0;

  if (var_val == NULL)
    return SNMP_ERR_WRONGLENGTH;
  if (var_val_type != ASN_INTEGER)
    return SNMP_ERR_WRONGTYPE;
  if (var_val_len != sizeof (long))
    return SNMP_ERR_WRONGLENGTH;

  pal_mem_cpy(&intval,var_val,4);

  pal_mem_set(port_bit_array, 0, 4);

  pal_mem_set(dynamic_port_bit_array, 0, 4);

  if ((intval < DOT1DPORTSTATICSTATUS_OTHER) ||
      (intval > DOT1DPORTSTATICSTATUS_DELETEONTIMEOUT))
    return SNMP_ERR_BADVALUE;

  /* validate the index */
  ret = l2_snmp_mac_addr_int_index_get (vp, name, &length,
                                        &key.addr, (int *) &key.port, 1);
  if (ret < 0 || key.port != 0)
    return SNMP_ERR_NOSUCHNAME;

  br = nsm_snmp_get_first_bridge();

  if (br == NULL)
    return SNMP_ERR_GENERR;

  master  = br->master;

  if (master == NULL)
    return SNMP_ERR_GENERR;

  /* Currently a Unicast/Multicast MAC can be added to only one interface.
     Find the static_fdb to
      a) Find the interface to which the MAC is added.
         The ifindex can be used to delete/add MAC in NSM/HSL.
      b) Update the snmp_status field for the MAC. The snmp_status
         represents the dot1qStaticUnicastTable status OID
  */

  if ( nsm_bridge_is_vlan_aware(br->master, br->name) == PAL_FALSE )
    {
      if (!br->static_fdb_list)
        return SNMP_ERR_GENERR;

      for (pn = ptree_top (br->static_fdb_list); pn; pn = ptree_next (pn))
        {
          if (! (static_fdb = pn->info) )
            continue;

          if ( pal_mem_cmp (&key.addr, static_fdb->mac_addr,
                          sizeof( static_fdb->mac_addr )) == 0 )
            {
              temp_static_fdb = static_fdb;
              list_head = static_fdb->ifinfo_list;

              if (!(pInfo = list_head))
                return SNMP_ERR_GENERR;
              if (intval == static_fdb->snmp_status)
                return SNMP_ERR_NOERROR;
              break;
            }

        }
    }
 else
    {
      vid = NSM_VLAN_DEFAULT_VID;

      for (rn = avl_top(br->vlan_table); rn; rn = avl_next (rn))
        {
          if (!(vlan = rn->info) || !(list = vlan->static_fdb_list))
            continue;
          for (pn = ptree_top (list); pn; pn = ptree_next (pn))
            {
              if (! (static_fdb = pn->info) )
                continue;

               if ( pal_mem_cmp (&key.addr, static_fdb->mac_addr,
                         sizeof( static_fdb->mac_addr )) == 0 )
                 {
                   temp_static_fdb = static_fdb;
                   list_head = static_fdb->ifinfo_list;

                   if (!(pInfo = list_head))
                     return SNMP_ERR_GENERR;

                   if (intval == static_fdb->snmp_status)
                     return SNMP_ERR_NOERROR;

                   break;
                 }
            }
        }
    }

  /* If the entry is already changed to Dynamic, changing the
     status is not allowed
  */
 if (!temp_static_fdb || !pInfo)
    {
      if (!fdb_static_cache)
        return SNMP_ERR_GENERR;

      if (fdb_static_cache->entries == 0)
        return SNMP_ERR_GENERR;

      fdb = xstp_snmp_fdb_mac_addr_lookup (fdb_static_cache, &key.addr, 1 );

      if (fdb == NULL)
        return SNMP_ERR_GENERR;

      if ((*(char *)(&key.addr) & 0x01) == 0x00)
        {
          port_id = nsm_snmp_bridge_port_get_port_id (fdb->port_no);
          bitstring_setbit (dynamic_port_bit_array, port_id - 1);
        }
      else
        {
          for ( i=0; i<(fdb_static_cache)->entries; i++ )
           {
             if ((pal_mem_cmp ( fdb->mac_addr,
                     (fdb_static_cache->fdbs)[i].mac_addr,
                     sizeof( fdb->mac_addr )) == 0 ))
               {
                 port_id = nsm_snmp_bridge_port_get_port_id (
                     (fdb_static_cache->fdbs)[i].port_no);

                 if ((port_id > 0 && port_id <= NSM_L2_SNMP_MAX_PORTS)
                     && ((fdb_static_cache->fdbs)[i].is_fwd == PAL_TRUE))
                   bitstring_setbit ( dynamic_port_bit_array, port_id - 1 );

               }
           }
        }

      if (intval == fdb->snmp_status)
        return SNMP_ERR_NOERROR;
    } /*return SNMP_ERR_GENERR;*/

   if (pInfo)
     {
       flag = 1;
     }

  while (pInfo)
    {
      port_id = nsm_snmp_bridge_port_get_port_id (pInfo->ifindex);
      bitstring_setbit (port_bit_array, port_id - 1);

      pInfo = pInfo->next_if;
    }

  /* Loop through all switch ports in the bridge
     and if the ifindex to which the MAC is added is found
     delete/add MAC based on the status received
  */
  for (avl_port = avl_top (br->port_tree); avl_port;
           avl_port = avl_next (avl_port))
     {
        if ((br_port = (struct nsm_bridge_port *)
                        AVL_NODE_INFO (avl_port)) == NULL)
          continue;

        if ((zif = br_port->nsmif) == NULL)
           continue;

        if ((ifp = zif->ifp) == NULL)
           continue;

        switch (intval)
          {
            case DOT1DPORTSTATICSTATUS_OTHER:
               return SNMP_ERR_NOERROR;
             break;

            /* If the status is INVALID, the MAC entry should be
               deleted from the Table.
             */
             case DOT1DPORTSTATICSTATUS_INVALID:
                {
                  if (flag)
                    ret = nsm_bridge_delete_mac (master, br->name, ifp,
                                                 key.addr.addr, vid, 1,
                                                 HAL_L2_FDB_STATIC,
                                                 PAL_TRUE, CLI_MAC);
                  else
                    ret = hal_l2_del_fdb(br->name, ifp->ifindex,
                                         key.addr.addr,
                                         HAL_HW_LENGTH,
                                         vid, 1, HAL_L2_FDB_DYNAMIC);

                  if ((ret == RESULT_OK) &&
                       (*(char *)(&key.addr) & 0x01) == 0x00)
                     return SNMP_ERR_NOERROR;
                }
              break;

             /* If the status is PERMANENT or DELETEONRESET,
                just updated the snmp_status to reflect the user setting.
                DELETEONRESET is same as a static entry i.e PERMANENT
             */
             case DOT1DPORTSTATICSTATUS_DELETEONRESET:
             case DOT1DPORTSTATICSTATUS_PERMANENT:
               port_id = nsm_snmp_bridge_port_get_port_id (ifp->ifindex);

               if (bitstring_testbit (dynamic_port_bit_array, port_id - 1))
                 {
                   ret = hal_l2_del_fdb(br->name, ifp->ifindex,
                                        key.addr.addr,
                                        HAL_HW_LENGTH,
                                        vid, 1, HAL_L2_FDB_DYNAMIC);

                   if (ret < 0)
                      return NSM_HAL_FDB_ERR;

                   ret = nsm_bridge_add_mac (master, br->name, ifp,
                                             key.addr.addr, vid, 1,
                                             HAL_L2_FDB_STATIC,
                                             PAL_TRUE, CLI_MAC);
                   if (ret != 0)
                     {
                       if (fdb)
                         ret = hal_l2_add_fdb(br->name, ifp->ifindex,
                                              key.addr.addr,
                                              HAL_HW_LENGTH,
                                              vid, 1, HAL_L2_FDB_DYNAMIC,fdb->is_fwd);
                       return SNMP_ERR_GENERR;
                      }
                 }
                 break;

                 /* If the statis is DELETEONTIMEOUT,
                   1) Delete the static entry from NSM/HSL.
                   2) Add a Dynamic entry in H/W via NSM.
                   3) The specified MAC entry will be added as dynamic entry
                      in the H/W and will be deleted after 300 Seconds
                  */

                 case DOT1DPORTSTATICSTATUS_DELETEONTIMEOUT:
                   if ((*(char *)(&key.addr) & 0x01) == 0x00)
                     {
                       port_id = nsm_snmp_bridge_port_get_port_id (ifp->ifindex);

                       if (bitstring_testbit (port_bit_array, port_id - 1))
                         {
                           ret = nsm_bridge_delete_mac (master, br->name, ifp,
                                                        key.addr.addr, vid, 1,
                                                        HAL_L2_FDB_STATIC,
                                                        PAL_TRUE, CLI_MAC);

                           ret = nsm_bridge_add_mac (master, br->name, ifp,
                                                     key.addr.addr, vid, 1,
                                                     HAL_L2_FDB_DYNAMIC,
                                                     PAL_TRUE, CLI_MAC);
                          }
                       }
                    else
                      return SNMP_ERR_GENERR;
                    break;

                  default:
                     pal_assert(0);
                     return SNMP_ERR_GENERR;
          }
     }

  /* If the status is other than INVALID, update the snmp_status.
     For status INVALID, the entry will be deleted from the table.
  */
  if ( intval != DOT1DPORTSTATICSTATUS_INVALID)
    {
      if (fdb)
        {
          if ((intval == DOT1DPORTSTATICSTATUS_DELETEONRESET) &&
              fdb->snmp_status == DOT1DPORTSTATICSTATUS_DELETEONTIMEOUT)
            {
              if ( nsm_bridge_is_vlan_aware(br->master, br->name) == PAL_FALSE )
                {
                  if (!br->static_fdb_list)
                    return SNMP_ERR_GENERR;
                  for (pn = ptree_top (br->static_fdb_list); pn; pn = ptree_next (pn))
                    {
                      if (! (static_fdb = pn->info) )
                        continue;

                      if ( pal_mem_cmp (&key.addr, static_fdb->mac_addr,
                            sizeof( static_fdb->mac_addr )) == 0 )
                        temp_static_fdb = static_fdb;
                      break;
                    }
                }
              else
                {
                  for (rn = avl_top(br->vlan_table); rn; rn = avl_next (rn))
                    {
                      if (!(vlan = rn->info) || !(list = vlan->static_fdb_list))
                        continue;
                      for (pn = ptree_top (list); pn; pn = ptree_next (pn))
                        {
                          if (! (static_fdb = pn->info) )
                            continue;

                          if ( pal_mem_cmp (&key.addr, static_fdb->mac_addr,
                                sizeof( static_fdb->mac_addr )) == 0 )
                            temp_static_fdb = static_fdb;

                          break;
                        }
                    }
                }
            }
        }

      if (temp_static_fdb)
        temp_static_fdb->snmp_status = intval;
    }

  return SNMP_ERR_NOERROR;
}

int
xstp_snmp_write_dot1dStaticTable (int action,
                                  u_char * var_val,
                                  u_char var_val_type,
                                  size_t var_val_len,
                                  u_char * statP, oid * name,
                                  size_t length,
                                  struct variable *vp,
                                  u_int32_t vr_id)
{
  int ret;
  struct mac_addr_and_port key;
  int port;

  /* validate the index */
  ret = l2_snmp_mac_addr_int_index_get (vp, name, &length,
                                        &key.addr, (int *) &key.port, 1);
  if (ret < 0 || key.port != 0)
    return SNMP_ERR_NOSUCHNAME;

  switch (vp->magic)
    {
    case DOT1DSTATICADDRESS:
      if (pal_mem_cmp ( &key.addr, var_val, MACADDR_LEN ) != 0)
        return SNMP_ERR_GENERR;
      for (port = 0; port < 4; port++)
         var_val[port] = 0xFF;
      return xstp_snmp_fdb_add_mac_to_port_list (key.addr, var_val,
                                                 DOT1DSTATICSTATUS_PERMANENT);
      break;

    case DOT1DSTATICRECEIVEPORT:
      if (key.port !=0 || var_val[0] != 0)
        return SNMP_ERR_GENERR;

      return xstp_snmp_fdb_add_mac_to_port_list (key.addr, var_val,
                                                 DOT1DSTATICSTATUS_PERMANENT);
      break;

    default:
      pal_assert(0);
      return SNMP_ERR_GENERR;
    }
  return SNMP_ERR_NOERROR;
}

#endif /* HAVE_SNMP */

