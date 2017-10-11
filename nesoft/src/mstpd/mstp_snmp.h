/* Copyright (C) 2001-2004  All Rights Reserved. */

#ifndef _PACOS_MSTP_SNMP_H
#define _PACOS_MSTP_SNMP_H

#include "lib.h"

/*
 * this is the root OID we use to register the objects handled in this
 * module.
 */
#define BRIDGEMIBOID 1, 3, 6, 1, 2, 1, 17

/*
 * PacOS enterprise BRIDGE-MIB equivalent root OID.  This variable is used to
 * register the BRIDGE-MIB to SNMP agent under SMUX protocol.
 */
#define RSTPDOID 1, 3, 6, 1, 4, 1, 3317, 1, 2, 16

/*
 * Although multiple instances of RSTP are allowed, RFC1493 only supports 
 * one RSTP instance per managed entity.  This is the name of the RSTP 
 * instance, if configured, on which the RFC1493 objects will operate.

 */
#define DEFAULT_BRIDGE_NAME  "s0"

/*
 * Fdb tables are updated from the forwarder no more often than this value
 * in secs.
 */

/*
 * The path cost default 
 */

#define STP8021T2001 2

/*
 * enums for column dot1dStpPortState
 */

#define DOT1DRSTPPORTSTATE_DISABLED              1
#define DOT1DRSTPPORTSTATE_BLOCKING              2
#define DOT1DRSTPPORTSTATE_LISTENING             3
#define DOT1DRSTPPORTSTATE_LEARNING              4
#define DOT1DRSTPPORTSTATE_FORWARDING            5
#define DOT1DRSTPPORTSTATE_BROKEN                6

/*
 * enums for column dot1dStpPortEnable
 */
#define DOT1DRSTPPORTENABLE_ENABLED              1
#define DOT1DRSTPPORTENABLE_DISABLED             2

/*enums for dot1dStpPortPathCost */
#define DOT1DSTPPORTPATHCOST_MAX                 65535

/*enums for dot1dStpPortProtocolMigration */
#define DOT1DSTPPORTPROTOCOLMIGRATION_TRUE       1
#define DOT1DSTPPORTPROTOCOLMIGRATION_FALSE      2

/* enums for dot1dStpPortAdminEdgePort */
#define DOT1DSTPPORTADMINEDGEPORT_TRUE           1 
#define DOT1DSTPPORTADMINEDGEPORT_FALSE          2

/* enums for dot1dStpPortOperEdgePort */
#define DOT1DSTPPORTOPEREDGEPORT_TRUE            1
#define DOT1DSTPPORTOPEREDGEPORT_FALSE           2

/* enums for dot1dStpPortAdminPointToPoint */
#define DOT1DSTPPORTADMINPOINT2POINT_TRUE        0 
#define DOT1DSTPPORTADMINPOINT2POINT_FALSE       1 
#define DOT1DSTPPORTADMINPOINT2POINT_AUTO        2

/*enums for dot1dStpPortOperPointToPoint */
#define DOT1DSTPPORTOPERPOINT2POINT_TRUE         1
#define DOT1DSTPPORTOPERPOINT2POINT_FALSE        2


/*
 * enums for column dot1dTpFdbStatus
 */
#define DOT1DTPFDBSTATUS_OTHER                    1
#define DOT1DTPFDBSTATUS_INVALID                  2
#define DOT1DTPFDBSTATUS_LEARNED                  3
#define DOT1DTPFDBSTATUS_SELF                     4
#define DOT1DTPFDBSTATUS_MGMT                     5

/*
 * enums for column dot1dStaticStatus
 */
#define DOT1DSTATICSTATUS_OTHER                   1
#define DOT1DSTATICSTATUS_INVALID                 2
#define DOT1DSTATICSTATUS_PERMANENT               3
#define DOT1DSTATICSTATUS_DELETEONRESET           4
#define DOT1DSTATICSTATUS_DELETEONTIMEOUT         5

/*
 * dot1d traps
 */
#define DOT1DNEWROOT                          1
#define DOT1DTOPOLOGYCHANGE                   2

/* enums for dot1dStpVersion */

#define BRIDGE_STP_VERSION_STPCOMPATIBLE 0
#define BRIDGE_STP_VERSION_RSTP 2


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
#define DOT1DSTPPRIORITY                            10
#define DOT1DSTPBRIDGEMAXAGE                        11
#define DOT1DSTPROOTCOST                            12
#define DOT1DSTPHELLOTIME                           13
#define DOT1DBASENUMPORTS                           14
#define NEWROOT                                     15
#define TOPOLOGYCHANGE                              16
#define DOT1DSTPROOTPORT                            17
#define DOT1DBASEBRIDGEADDRESS                      18
#define DOT1DSTPPROTOCOLSPECIFICATION               19
#define DOT1DSTPTIMESINCETOPOLOGYCHANGE             20
#define DOT1DTPLEARNEDENTRYDISCARDS                 21
#define DOT1DSTPPORT                                22
#define DOT1DSTPPORTPRIORITY                        23
#define DOT1DSTPPORTSTATE                           24
#define DOT1DSTPPORTENABLE                          25
#define DOT1DSTPPORTPATHCOST                        26
#define DOT1DSTPPORTDESIGNATEDROOT                  27
#define DOT1DSTPPORTDESIGNATEDCOST                  28
#define DOT1DSTPPORTDESIGNATEDBRIDGE                29
#define DOT1DSTPPORTDESIGNATEDPORT                  30
#define DOT1DSTPPORTFORWARDTRANSITIONS              31
#define DOT1DSTPPORTPATHCOST32                      32
#define DOT1DSTPVERSION                             33
#define DOT1DSTPTXHOLDCOUNT                         34
/* #define DOT1DSTPPATHCOSTDEFAULT                  35 */
#define DOT1DSTPPORTPROTOCOLMIGRATION               36
#define DOT1DSTPPORTADMINEDGEPORT                   37
#define DOT1DSTPPORTOPEREDGEPORT                    38 
#define DOT1DSTPPORTADMINPOINTTOPOINT               39
#define DOT1DSTPPORTOPERPOINTTOPOINT                40
#define DOT1DSTPPORTADMINPATHCOST                   41
#define DOT1DTPFDBADDRESS                           42 
#define DOT1DTPFDBPORT                              43 
#define DOT1DTPFDBSTATUS                            44 
#define DOT1DBASEPORT                               45 
#define DOT1DBASEPORTIFINDEX                        46 
#define DOT1DBASEPORTCIRCUIT                        47 
#define DOT1DBASEPORTDELAYEXCEEDEDDISCARDS          48 
#define DOT1DBASEPORTMTUEXCEEDEDDISCARDS            49 
#define DOT1DTPPORT                                 50 
#define DOT1DTPPORTMAXINFO                          51  
#define DOT1DTPPORTINFRAMES                         52 
#define DOT1DTPPORTOUTFRAMES                        53 
#define DOT1DTPPORTINDISCARDS                       54 
#define DOT1DSTATICADDRESS                          55 
#define DOT1DSTATICRECEIVEPORT                      56 
#define DOT1DSTATICALLOWEDTOGOTO                    57 
#define DOT1DSTATICSTATUS                           58 

#if !defined(NELEMENTS)
  #define NELEMENTS(x) ((sizeof(x))/(sizeof((x)[0])))
#endif

#define MSTP_PORT_PROTOCOL_MIGRATION 2
#define MSTP_PORT_FORCE_VERSION      2

/* SNMP value return using static variables.  */

#define MSTP_SNMP_RETURN_INTEGER(V) \
  do { \
    *var_len = sizeof (int); \
    mstp_int_val = V; \
    return (u_char *) &mstp_int_val; \
  } while (0)

#define MSTP_SNMP_RETURN_TIMETICKS(V) \
  do { \
    *var_len = sizeof (u_long); \
    mstp_ulong_val = V; \
    return (u_char *) &mstp_ulong_val; \
  } while (0)

#define MSTP_SNMP_RETURN_BRIDGEID(V) \
  do { \
    *var_len = sizeof (struct bridge_id); \
    mstp_snmp_bridge_id = V; \
    return (u_char *) &mstp_snmp_bridge_id; \
  } while (0)

#define MSTP_SNMP_RETURN_MACADDRESS(V) \
  do { \
    *var_len = ETHER_ADDR_LEN; \
    mstp_string_val = (u_char *)(V); \
    return mstp_string_val; \
  } while (0)

#define MSTP_SNMP_RETURN_OID(V,L) \
  do { \
    *var_len = (L)*sizeof(oid); \
    return (u_char *)(V); \
  } while (0)

#define MSTP_SNMP_RETURN_STRING(V,L) \
  do { \
    *var_len = L; \
    mstp_string_val = (u_char *)(V); \
    return mstp_string_val; \
  } while (0)

#define MSTP_SNMP_CEIL(N,D) \
  ( (N) % (D) ? ((N)/(D)) + 1 : (N)/(D) )

#define MSTP_SNMP_FLOOR(N,D) \
  ( (N)/(D) )

#define MSTP_SNMP_TIMER_TO_TIMEOUT(V) \
  (int )(MSTP_SNMP_CEIL(((long )(V)*100),256))

#define MSTP_SNMP_TIMEOUT_TO_TIMER(V) \
  (int )(STP_SNMP_FLOOR(((long )(V)*256),100))

extern void mstp_snmp_init (struct lib_globals *);
extern void * mstp_get_first_bridge (void);
extern void mstp_snmp_topology_change (struct mstp_port *port, int state);
extern void mstp_snmp_new_root (struct mstp_bridge * br);
extern struct mstp_port * mstp_snmp_bridge_port_lookup (struct mstp_bridge *br,
                                                 u_int32_t port_instance,
                                                 int exact);
#if 0
/* extern declarations for the index get and set procedures */

int l2_snmp_index_get (struct variable *v, oid * name, size_t * length,
                           u_int32_t * index, int exact);
int bridge_snmp_port_vlan_index_get (struct variable *v, oid * name,
                                     size_t * length,
                                     u_int32_t * port, u_int32_t * vlan,
                                     int exact);
void l2_snmp_index_set (struct variable *v, oid * name, size_t * length,
                            u_int32_t index);
void bridge_snmp_port_vlan_index_set (struct variable *v, oid * name,
                                      size_t * length, u_int32_t port,
                                       u_int32_t vlan);
int bridge_snmp_mac_addr_index_get (struct variable *v, oid *name,
                                    size_t *length, struct mac_addr *addr,
                                    int exact);
void bridge_snmp_mac_addr_index_set (struct variable *v, oid *name,
                                     size_t *length, struct mac_addr *addr);
int bridge_snmp_mac_addr_int_index_get (struct variable *v, oid *name,
                                        size_t *length, struct mac_addr *addr,
                                        int *idx, int exact);
void bridge_snmp_mac_addr_int_index_set (struct variable *v, oid *name,
                                        size_t * length,
                                       struct mac_addr *addr, int idx );
int bridge_snmp_port_descend_cmp(const void *e1, const void *e2);
int bridge_snmp_port_ascend_cmp(const void *e1, const void *e2);
int bridge_snmp_vlan_descend_cmp(const void *e1, const void *e2);
int bridge_snmp_vlan_ascend_cmp(const void *e1, const void *e2);

/* extern declaration for index get and set end here */
#endif
#endif  /* _PACOS_MSTP_SNMP_H */

