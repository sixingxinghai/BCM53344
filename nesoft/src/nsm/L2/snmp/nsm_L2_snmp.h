/* Copyright (C) 2001-2004  All Rights Reserved. */

#ifndef _PACOS_NSM_L2_SNMP_H
#define _PACOS_NSM_L2_SNMP_H

#include "l2lib.h"
#include "lib.h"
#include "hal_incl.h"
#include "nsm_bridge.h"

/*
 * this is the root OID we use to register the objects handled in this
 * module.
 */
#define BRIDGEMIBOID 1, 3, 6, 1, 2, 1, 17

/* 
 * PacOS enterprise BRIDGE-MIB equivalent root OID.  This variable is used to
 * register the BRIDGE-MIB to SNMP agent under SMUX protocol.
 */
/*#define STPDOID  1,3,6,1,4,1,3317,1,2,12
#define RSTPDOID 1,3,6,1,4,1,3317,1,2,16 */

/*
 * Although multiple instances of STP are allowed, RFC1493 only supports one STP
 * instance per managed entity.  This is the name of the STP instance, if 
 * configured, on which the RFC1493 objects will operate.
 */
#define DEFAULT_BRIDGE_NAME  "s0"

/*
 * Fdb tables are updated from the forwarder no more often than this value
 * in secs.
 */
#define XSTP_SNMP_FDB_CACHE_TIMEOUT  10

/*
 * enums for column dot1dStpPortState 
 */
#define DOT1DSTPPORTSTATE_DISABLED              1
#define DOT1DSTPPORTSTATE_BLOCKING              2
#define DOT1DSTPPORTSTATE_LISTENING             3
#define DOT1DSTPPORTSTATE_LEARNING              4
#define DOT1DSTPPORTSTATE_FORWARDING            5
#define DOT1DSTPPORTSTATE_BROKEN                6

/*
 * enums for column dot1dStpPortEnable 
 */
#define DOT1DSTPPORTENABLE_ENABLED              1
#define DOT1DSTPPORTENABLE_DISABLED             2

/*
 * enums for column dot1dTpFdbStatus 
 */
#define DOT1DTPFDBSTATUS_OTHER                  1
#define DOT1DTPFDBSTATUS_INVALID                2
#define DOT1DTPFDBSTATUS_LEARNED                3
#define DOT1DTPFDBSTATUS_SELF                   4
#define DOT1DTPFDBSTATUS_MGMT                   5

/*
 * enums for column dot1dStaticStatus 
 */
#define DOT1DSTATICSTATUS_OTHER                 1
#define DOT1DSTATICSTATUS_INVALID               2
#define DOT1DSTATICSTATUS_PERMANENT             3
#define DOT1DSTATICSTATUS_DELETEONRESET         4
#define DOT1DSTATICSTATUS_DELETEONTIMEOUT       5

/*
 * dot1d traps
 */
#define DOT1DNEWROOT                          1
#define DOT1DTOPOLOGYCHANGE                   2

/*
 * SMUX magic numbers for registered objects
 */
#define DOT1DSTPBRIDGEHELLOTIME                     1
#define DOT1DTPAGINGTIME                            2
#define DOT1DSTPMAXAGE                              3
#define DOT1DSTPDESIGNATEDROOT                      4
#define DOT1DSTPTOPCHANGES                          5
#define DOT1DSTPBRIDGEFORWARDDELAY                  6
#define DOT1DSTPFORWARDDELAY                        7
#define DOT1DSTPHOLDTIME                            8
#define DOT1DBASETYPE                               9
#define DOT1DSTPPRIORITY                           10
#define DOT1DSTPBRIDGEMAXAGE                       11
#define DOT1DSTPROOTCOST                           12
#define DOT1DSTPHELLOTIME                          13
#define DOT1DBASENUMPORTS                          14
#define NEWROOT                                    15
#define TOPOLOGYCHANGE                             16
#define DOT1DSTPROOTPORT                           17
#define DOT1DBASEBRIDGEADDRESS                     18
#define DOT1DSTPPROTOCOLSPECIFICATION              19
#define DOT1DSTPTIMESINCETOPOLOGYCHANGE            20
#define DOT1DTPLEARNEDENTRYDISCARDS                21
#define DOT1DSTPPORT                               22
#define DOT1DSTPPORTPRIORITY                       23
#define DOT1DSTPPORTSTATE                          24
#define DOT1DSTPPORTENABLE                         25
#define DOT1DSTPPORTPATHCOST                       26
#define DOT1DSTPPORTDESIGNATEDROOT                 27
#define DOT1DSTPPORTDESIGNATEDCOST                 28
#define DOT1DSTPPORTDESIGNATEDBRIDGE               29
#define DOT1DSTPPORTDESIGNATEDPORT                 30
#define DOT1DSTPPORTFORWARDTRANSITIONS             31
#define DOT1DTPFDBADDRESS                          32
#define DOT1DTPFDBPORT                             33
#define DOT1DTPFDBSTATUS                           34
#define DOT1DBASEPORT                              35
#define DOT1DBASEPORTIFINDEX                       36
#define DOT1DBASEPORTCIRCUIT                       37
#define DOT1DBASEPORTDELAYEXCEEDEDDISCARDS         38
#define DOT1DBASEPORTMTUEXCEEDEDDISCARDS           39
#define DOT1DTPPORT                                40
#define DOT1DTPPORTMAXINFO                         41
#define DOT1DTPPORTINFRAMES                        42
#define DOT1DTPPORTOUTFRAMES                       43
#define DOT1DTPPORTINDISCARDS                      44
#define DOT1DSTATICADDRESS                         45
#define DOT1DSTATICRECEIVEPORT                     46
#define DOT1DSTATICALLOWEDTOGOTO                   47
#define DOT1DSTATICSTATUS                          48

#define XSTP_FORWARD  1
#define XSTP_DISCARD  0       


#if !defined(NELEMENTS)
  #define NELEMENTS(x) ((sizeof(x))/(sizeof((x)[0])))
#endif

/* L2 FDB table routine options */
#define XSTP_FDB_TABLE_STATIC         (1<<0)
#define XSTP_FDB_TABLE_DYNAMIC        (1<<1)
#define XSTP_FDB_TABLE_UNIQUE_MAC     (1<<2)
#define XSTP_FDB_TABLE_MULTICAST_MAC  (1<<3)
#define XSTP_FDB_TABLE_UNICAST_MAC    (1<<4)
#define XSTP_FDB_TABLE_BOTH (XSTP_FDB_TABLE_STATIC | XSTP_FDB_TABLE_DYNAMIC)
#define XSTP_FDB_TABLE_STATIC_MULTICAST (XSTP_FDB_TABLE_STATIC | XSTP_FDB_TABLE_MULTICAST_MAC)
#define XSTP_FDB_TABLE_STATIC_UNICAST (XSTP_FDB_TABLE_STATIC | XSTP_FDB_TABLE_UNICAST_MAC)

#define XSTP_FDB_MAX_CACHE_ENTRIES (HAL_MAX_L2_FDB_ENTRIES )

typedef struct xstp_fdb_cache_s {

  struct fdb_entry *fdbs;  /* retrieved FDB entries */
  int entries;   /* total entries in table */
  struct fdb_entry *fdb_sorted[HAL_MAX_VLAN_ID];  /* sorted entries */
  struct fdb_entry *fdb_temp_sorted[HAL_MAX_VLAN_ID]; 
  pal_time_t time_updated;  /* last update time */

} XSTP_FDB_CACHE;

struct mac_addr_and_port {
  struct mac_addr addr;
  u_int port;
};

struct vid_and_mac_addr {
  int vid;
  struct mac_addr addr;
};

struct vid_and_mac_addr_and_port {
    int vid;
    struct mac_addr addr;
    u_int port;
};

 struct nsm_vlanclass_frame_proto {
   int frame_type;
   char protocol_value[5];
 };


/* SNMP value return using static variables.  */

#define XSTP_SNMP_RETURN_INTEGER(V) \
  do { \
    *var_len = sizeof (int); \
    xstp_int_val = V; \
    return (u_char *) &xstp_int_val; \
  } while (0)

#define XSTP_SNMP_RETURN_INTEGER64(V) \
  do { \
    *var_len = sizeof (V); \
    return (u_char *) &(V); \
  } while (0)

#define XSTP_SNMP_RETURN_TIMETICKS(V) \
  do { \
    *var_len = sizeof (u_long); \
    xstp_ulong_val = V; \
    return (u_char *) &xstp_ulong_val; \
  } while (0)

#define XSTP_SNMP_RETURN_BRIDGEID(V) \
  do { \
    *var_len = sizeof (struct bridge_id); \
    xstp_snmp_bridge_id = V; \
    return (u_char *) &xstp_snmp_bridge_id; \
  } while (0)

#define XSTP_SNMP_RETURN_MACADDRESS(V) \
  do { \
    *var_len = ETHER_ADDR_LEN; \
    xstp_string_val = (u_char *)(V); \
    return xstp_string_val; \
  } while (0)

#define XSTP_SNMP_RETURN_OID(V,L) \
  do { \
    *var_len = (L)*sizeof(oid); \
    return (u_char *)(V); \
  } while (0)

#define XSTP_SNMP_RETURN_STRING(V,L) \
  do { \
    *var_len = L; \
    xstp_string_val = (u_char *)(V); \
    return xstp_string_val; \
  } while (0)

#define XSTP_SNMP_CEIL(N,D) \
  ( (N) % (D) ? ((N)/(D)) + 1 : (N)/(D) )
  
#define XSTP_SNMP_FLOOR(N,D) \
  ( (N)/(D) )

#define XSTP_SNMP_TIMER_TO_TIMEOUT(V) \
  (int )(XSTP_SNMP_CEIL(((long )(V)*100),256))
  
#define XSTP_SNMP_TIMEOUT_TO_TIMER(V) \
  (int )(XSTP_SNMP_FLOOR(((long )(V)*256),100))
  

void 
xstp_snmp_init (struct lib_globals *);

extern u_char *xstp_string_val;

struct interface *
xstp_snmp_bridge_port_lookup (struct nsm_bridge *br, int port_instance,
                             int exact);
int
xstp_snmp_update_fdb_cache ( XSTP_FDB_CACHE **cache, int options, int status_object );
int
xstp_snmp_update_fdb_cache2 ( XSTP_FDB_CACHE **cache, int options, int status_object );
int 
xstp_snmp_fdb_mac_addr_cmp(const void *e1, const void *e2);
int
xstp_snmp_fdb_vid_mac_addr_cmp(const void *e1, const void *e2);
int 
xstp_snmp_fdb_mac_addr_port_lookup_cmp(const void *e1, const void *e2);
struct fdb_entry *
xstp_snmp_fdb_mac_addr_port_lookup ( XSTP_FDB_CACHE *cache, 
                                    struct mac_addr_and_port *key, int exact );
struct fdb_entry *
xstp_snmp_fdb_vid_mac_addr_lookup ( XSTP_FDB_CACHE *cache,
                                    struct vid_and_mac_addr *key, int exact );
int 
xstp_snmp_fdb_mac_addr_lookup_cmp(const void *e1, const void *e2);
struct fdb_entry *
xstp_snmp_fdb_mac_addr_lookup ( XSTP_FDB_CACHE *cache, struct mac_addr *addr, 
                                int exact );

int
xstp_snmp_fdb_vid_mac_addr_port_lookup_cmp(const void *e1, const void *e2);

struct fdb_entry *
xstp_snmp_fdb_vid_mac_addr_port_lookup ( XSTP_FDB_CACHE *cache,
                                   struct vid_and_mac_addr_and_port *key,
                                   int exact);

int
xstp_snmp_write_dot1dStaticAllowedToGoTo ( int action,
                                           u_char * var_val,
                                           u_char var_val_type,
                                           size_t var_val_len,
                                           u_char * statP,
                                           oid * name, size_t length,
                                           struct variable *vp,
                                           u_int32_t vr_id);

int
xstp_snmp_write_dot1dStaticStatus ( int action,
                                    u_char * var_val,
                                    u_char var_val_type,
                                    size_t var_val_len,
                                    u_char * statP,
                                    oid * name, size_t length,
                                    struct variable *vp,
                                    u_int32_t vr_id);

extern struct lib_globals *snmpg;

#ifdef HAVE_HAL
int
xstp_get_dynamic_count(char *bridge_name,hal_get_fdb_func_t fdb_get_func);

int
xstp_get_dynamic_entries(char *bridge_name, hal_get_fdb_func_t fdb_get_func,
                         int start_index, XSTP_FDB_CACHE **cache,
                         int max_count, int status_object);
#endif /* HAVE_HAL */

#endif  /* _PACOS_NSM_L2_SNMP_H */
