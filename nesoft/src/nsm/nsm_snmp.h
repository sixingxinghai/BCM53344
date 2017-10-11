/* Copyright (C) 2001-2004  All Rights Reserved. */

#ifndef _PACOS_NSM_SNMP_H
#define _PACOS_NSM_SNMP_H

#define NSMDOID 1,3,6,1,4,1,3317,1,2,10
#define NSMMIBOID  1,3,6,1,2,1
#define MIB2SYSGROUPOID  1,3,6,1,2,1,1
#define MIB2IFGROUPOID  1,3,6,1,2,1,2
#define MIB2IPGROUPOID  1,3,6,1,2,1,4
#define MIB2ENTITYGROUPOID  1,3,6,1,2,1,47
#define MIB2IFMIBOID    1,3,6,1,2,1,31
#define NSMTRAPMIB      1,3,6,1,6,3,1,1,5

/* Hack for SNMP values */
#define COUNTER        ASN_COUNTER
#define COUNTER64      ASN_COUNTER64
#define GAUGE          ASN_GAUGE
#define IPADDRESS      ASN_IPADDRESS
#define STRING         ASN_OCTET_STR
#define INTEGER        ASN_INTEGER
#define TIMETICKS      ASN_TIMETICKS
#define OBJECT_ID      ASN_OBJECT_ID
#define ROWSTATUS      ASN_INTEGER
#define DISPLAYSTRING  ASN_OCTET_STR
#define TRUTHVALUE     ASN_INTEGER
#define RCREATE        RWRITE
#define NOTACCESSIBLE  NOACCESS

#define RCREATE     RWRITE

#define NSM_SNMP_ROW_STATUS_MIN            0
#define NSM_SNMP_ROW_STATUS_ACTIVE         1   
#define NSM_SNMP_ROW_STATUS_NOTINSERVICE   2
#define NSM_SNMP_ROW_STATUS_NOTREADY       3
#define NSM_SNMP_ROW_STATUS_CREATEANDGO    4
#define NSM_SNMP_ROW_STATUS_CREATEANDWAIT  5
#define NSM_SNMP_ROW_STATUS_DESTROY        6
#define NSM_SNMP_ROW_STATUS_MAX            7

#if !defined(NELEMENTS)
  #define NELEMENTS(x) ((sizeof(x))/(sizeof((x)[0])))
#endif

#define NSM_MAX_STRING_LENGTH 255

/* SMUX magic number for registered objects */
#define NSM_SYSTEMDESC                  1
#define NSM_SYSOBJID                    2
#define NSM_SYSUPTIME                   3
#define NSM_SYSCONTACT                  4
#define NSM_SYSNAME                     5
#define NSM_SYSLOCATION                 6
#define NSM_SYSSERVICIES                7
#define NSM_IFNUMBER                    8
#define NSM_IFINDEX                     9
#define NSM_IFDESCR                     10
#define NSM_IFTYPE                      11
#define NSM_IFMTU                       12
#define NSM_IFSPEED                     13
#define NSM_IFPHYSADDRESS               14
#define NSM_IFADMINSTATUS               15
#define NSM_IFOPERSTATUS                16
#define NSM_IFLASTCHANGE                17
#define NSM_IFINOCTETS                  18
#define NSM_IFINUCASTPKTS               19
#define NSM_IFINNUCASTPKTS              20
#define NSM_IFINDISCARDS                21
#define NSM_IFINERRORS                  22
#define NSM_IFINUNKNOWNPROTOS           23
#define NSM_IFOUTOCTETS                 24
#define NSM_IFOUTUCASTPKTS              25
#define NSM_IFOUTNUCASTPKTS             26
#define NSM_IFOUTDISCARDS               27
#define NSM_IFOUTERRORS                 28
#define NSM_IFOUTQLEN                   29 
#define NSM_IFSPECIFIC                  30
#define NSM_IPFORWARDING                31
#define NSM_IPDEFAULTTTL                32
#define NSM_IPADENTADDR                 33
#define NSM_IPADENTIFINDEX              34
#define NSM_IPADENTNETMASK              35
#define NSM_IPADENTBCASTADDR            36
#define NSM_IPADENTREASMMAXSIZE         37
#define NSM_IPFORWARDNUMBER             38
#define NSM_IPCIDRROUTENUMBER           39
#define NSM_IPCIDRROUTEDEST             40
#define NSM_IPCIDRROUTEMASK             41
#define NSM_IPCIDRROUTETOS              42
#define NSM_IPCIDRROUTENEXTHOP          43
#define NSM_IPCIDRROUTEIFINDEX          44
#define NSM_IPCIDRROUTETYPE             45
#define NSM_IPCIDRROUTEPROTO            46
#define NSM_IPCIDRROUTEAGE              47
#define NSM_IPCIDRROUTEINFO             48
#define NSM_IPCIDRROUTENEXTHOPAS        49
#define NSM_IPCIDRROUTEMETRIC1          50
#define NSM_IPCIDRROUTEMETRIC2          51
#define NSM_IPCIDRROUTEMETRIC3          52
#define NSM_IPCIDRROUTEMETRIC4          53
#define NSM_IPCIDRROUTEMETRIC5          54
#define NSM_IPCIDRROUTESTATUS           55
#define NSM_IFNAME                      56
#define NSM_IFINMULTICASTPKTS           57
#define NSM_IFINBROADCASTPKTS           58
#define NSM_IFOUTMULTICASTPKTS          59
#define NSM_IFOUTBROADCASTPKTS          60
#define NSM_IFHCINOCTETS                61
#define NSM_IFHCINUCASTPKTS             62
#define NSM_IFHCINMULTICASTPKTS         63
#define NSM_IFHCINBROADCASTPKTS         64
#define NSM_IFHCOUTOCTETS               65
#define NSM_IFHCOUTUCASTPKTS            66
#define NSM_IFHCOUTMULTICASTPKTS        67
#define NSM_IFHCOUTBROADCASTPKTS        68
#define NSM_IFLINKUPDOWNTRAPENABLE      69
#define NSM_IFHIGHSPEED                 70
#define NSM_IFPROMISCUOUSMODE           71
#define NSM_IFCONNECTORPRESENT          72
#define NSM_IFALIAS                     73
#define NSM_IFCOUNTERDISCONTINUITYTIME  74
#define NSM_IFRCVADDRESSADDRESS         75
#define NSM_IFRCVADDRESSSTATUS          76
#define NSM_IFRCVADDRESSTYPE            77
#define NSM_IFTBLLASTCHANGE             78

/* defines for NSM_ENTITYPHYISFRU */
#define NSM_SNMP_FRU_TRUE         1
#define NSM_SNMP_FRU_FALSE        2

/* SMUX magic number for registered Entity objects */
#define NSM_ENTITYPHYIDX           1
#define NSM_ENTITYPHYDESCR         2
#define NSM_ENTITYPHYVENDORTYPE    3
#define NSM_ENTITYPHYCONTAINEDIN   4
#define NSM_ENTITYPHYCLASS         5
#define NSM_ENTITYPHYPARENTRELPOS  6
#define NSM_ENTITYPHYNAME          7
#define NSM_ENTITYPHYHWREV         8
#define NSM_ENTITYPHYFWREV         9
#define NSM_ENTITYPHYSWREV         10
#define NSM_ENTITYPHYSERIALNO      11
#define NSM_ENTITYPHYMFGNAME       12
#define NSM_ENTITYPHYMODELNAME     13
#define NSM_ENTITYPHYALIAS         14
#define NSM_ENTITYPHYASSETID       15
#define NSM_ENTITYPHYISFRU         16

#define NSM_ENTITYLOGIDX           1
#define NSM_ENTITYLOGDESCR         2
#define NSM_ENTITYLOGTYPE          3
#define NSM_ENTITYLOGCOMMUNITY     4
#define NSM_ENTITYLOGTADDRESS      5
#define NSM_ENTITYLOGTDOMAIN       6
#define NSM_ENTITYLOGCONTEXTENGID  7
#define NSM_ENTITYLOGCONTEXTNAME   8

#define NSM_ENTITYLPPHYSICALIDX    1

#define NSM_ENTITYALIASIDXORZERO   1
#define NSM_ENTITYALIASMAPID       2

#define NSM_ENTITYPHYCHILDIDX      1

#define NSM_ENTITYLASTCHANGE       1

/* Set Mask values for Route Table */
#define NSM_SET_IPCIDRTABLE_METRIC          0x2
#define NSM_SET_IPCIDRTABLE_ROUTETYPE       0x4
#define NSM_SET_IPCIDRTABLE_IFINDEX         0x8 
#define NSM_SET_IPCIDRTABLE_ROWSTATUS       0x10

#define NSM_IF_TRAP_ENABLE  1
#define NSM_IF_TRAP_DISABLE 2

/* defines for interface hardware type */
#define NSM_SNMP_INTERFACE_OTHER                      1
#define NSM_SNMP_INTERFACE_REGULAR1822                2
#define NSM_SNMP_INTERFACE_HDH1822                    3
#define NSM_SNMP_INTERFACE_DDNX25                     4
#define NSM_SNMP_INTERFACE_RFC877X25                  5
#define NSM_SNMP_INTERFACE_ETHERNETCSMACD             6
#define NSM_SNMP_INTERFACE_ISO88023CSMACD             7
#define NSM_SNMP_INTERFACE_ISO88024TOKENBUS           8
#define NSM_SNMP_INTERFACE_ISO88025TOKENRING          9
#define NSM_SNMP_INTERFACE_ISO88026MAN                10
#define NSM_SNMP_INTERFACE_STARLAN                    11
#define NSM_SNMP_INTERFACE_PROTEON10MBIT              12
#define NSM_SNMP_INTERFACE_PROTEON80MBIT              13
#define NSM_SNMP_INTERFACE_HYPERCHANNEL               14
#define NSM_SNMP_INTERFACE_FDDI                       15
#define NSM_SNMP_INTERFACE_LAPB                       16
#define NSM_SNMP_INTERFACE_SDLC                       17
#define NSM_SNMP_INTERFACE_DS1                        18
#define NSM_SNMP_INTERFACE_E1                         19
#define NSM_SNMP_INTERFACE_BASICISDN                  20
#define NSM_SNMP_INTERFACE_PRIMARYISDN                21
#define NSM_SNMP_INTERFACE_PROPPOINTTOPOINTSERIAL     22
#define NSM_SNMP_INTERFACE_PPP                        23
#define NSM_SNMP_INTERFACE_SOFTWARELOOPBACK           24
#define NSM_SNMP_INTERFACE_EON                        25
#define NSM_SNMP_INTERFACE_ETHERNET3MBIT              26
#define NSM_SNMP_INTERFACE_NSIP                       27
#define NSM_SNMP_INTERFACE_SLIP                       28
#define NSM_SNMP_INTERFACE_ULTRA                      29
#define NSM_SNMP_INTERFACE_DS3                        30 
#define NSM_SNMP_INTERFACE_SIP                        31
#define NSM_SNMP_INTERFACE_FRAMERELAY                 32

/* defines for interface admin and oper status */
#define NSM_SNMP_INTERFACE_UP       1
#define NSM_SNMP_INTERFACE_DOWN     2 
#define NSM_SNMP_INTERFACE_TESTING  3

/* defines for interface Promiscuous Modes */
#define NSM_SNMP_PROMISC_TRUE         1
#define NSM_SNMP_PROMISC_FALSE        2

/* defines for interface Connector Present */
#define NSM_SNMP_CONN_TRUE         1
#define NSM_SNMP_CONN_FALSE        2

/* defines for ip forwarding */
#define NSM_SNMP_IPFORWARDING_ENABLE  1
#define NSM_SNMP_IPFORWARDING_DISABLE 2

/* define for sys services */
#define NSM_SNMP_SYSSSERVICE_PHYSICAL    1
#define NSM_SNMP_SYSSERVICE_DATALINK     2
#define NSM_SNMP_SYSSERVICE_GATEWAY      3
#define NSM_SNMP_SYSSERVICE_IPHOSTS      4
#define NSM_SNMP_SYSSERVICE_APPLICATIONS 7

/* Default metric value for ip cidr route */
#define NSM_SNMP_DEFAULT_METRIC_VAL    -1

/* defines for route types */
#define NSM_SNMP_ROUTE_MIN     0
#define NSM_SNMP_ROUTE_OTHER   1
#define NSM_SNMP_ROUTE_REJECT  2
#define NSM_SNMP_ROUTE_LOCAL   3
#define NSM_SNMP_ROUTE_REMOTE  4
#define NSM_SNMP_ROUTE_MAX     5

/* defines for route protocol */
#define NSM_SNMP_PROTO_OTHER                   1
#define NSM_SNMP_PROTO_LOCAL                   2
#define NSM_SNMP_PROTO_NETMGMT                 3 
#define NSM_SNMP_PROTO_ICMP                    4
#define NSM_SNMP_PROTO_EGP                     5
#define NSM_SNMP_PROTO_GGP                     6
#define NSM_SNMP_PROTO_HELLO                   7
#define NSM_SNMP_PROTO_RIP                     8
#define NSM_SNMP_PROTO_ISIS                    9
#define NSM_SNMP_PROTO_ESIS                    10
#define NSM_SNMP_PROTO_CISCOIGRP               11
#define NSM_SNMP_PROTO_BBNSPFIGP               12
#define NSM_SNMP_PROTO_OSPF                    13
#define NSM_SNMP_PROTO_BGP                     14
#define NSM_SNMP_PROTO_IDPR                    15
#define NSM_SNMP_PROTO_CISCOEIGRP              16 

/* defines for the index length passed for CIDR table */
#define CIDR_FULL_INDEX      0x2
#define CIDR_PARTIAL_INDEX   0x4
#define CIDR_NO_INDEX        0x8

#define CIDR_FULL_INDEX_LEN 13

/* defines for sys Group strings */
#define SYS_DESC_LEN     512
#define SYS_CONTACT_LEN  128
#define SYS_NAME_LEN     65
#define SYS_LOC_LEN      256

struct nsm_if_stats
{
  u_long ifInOctets;
  u_long ifInUcastPkts;
  u_long ifInNUcastPkts;
  u_long ifInDiscards;
  u_long ifInErrors;
  u_long ifInUnknownProtos;
  u_long ifOutOctets;
  u_long ifOutUcastPkts;
  u_long ifOutNUcastPkts;
  u_long ifOutDiscards;
  u_long ifOutErrors;
};

struct list *snmp_shadow_route_table;

struct cidr_route_entry
{
  struct pal_in4_addr ipCidrRouteDest;
  struct pal_in4_addr ipCidrRouteMask;
  int ipCidrRouteTos;
  struct pal_in4_addr ipCidrRouteNextHop;
  int ipCidrRouteIfIndex;
  int ipCidrRouteType;
  int ipCidrRouteProto;
  int ipCidrRouteAge;
  unsigned long ipCidrRouteInfo;
  int ipCidrRouteNextHopAS;
  int ipCidrRouteMetric1;
  int ipCidrRouteMetric2;
  int ipCidrRouteMetric3;
  int ipCidrRouteMetric4;
  int ipCidrRouteMetric5;
  int ipCidrRouteStatus;
};

/* SNMP value return */
#define NSM_SNMP_RETURN_INTEGER(V) \
   do { \
      *var_len = sizeof (int); \
      nsm_int_val = V; \
      return (u_char *) &nsm_int_val; \
   } while (0)

#define NSM_SNMP_RETURN_INTEGER64(V) \
  do { \
    *var_len = sizeof (V); \
     temp_val = pal_hton32 ((V).l[0]); \
     (V).l[0] = pal_hton32 ((V).l[1]); \
     (V).l[1] = temp_val;              \
    return (u_char *) &(V); \
  } while (0)

#define NSM_SNMP_RETURN_STRING(V, L) \
  do { \
     *var_len = L; \
     nsm_string_val = (u_char *)V; \
    return (nsm_string_val); \
  } while (0)

#define NSM_SNMP_RETURN_MACADDRESS(V) \
  do { \
    *var_len = NPFBRIDGE_ETHALEN; \
    nsm_string_val = (u_char *)V; \
    return (nsm_string_val); \
  } while (0)

#define NSM_SNMP_RETURN_OID(V, L) \
  do { \
    *var_len = (L)*sizeof(oid); \
    return (u_char *)(V); \
  } while (0)


#define NSM_SNMP_RETURN_TIMETICKS(V) \
  do { \
    *var_len = sizeof (u_long); \
    nsm_ulong_val = V; \
    return (u_char *) &nsm_ulong_val; \
  } while (0)

#define NSM_SNMP_RETURN_IPADDRESS(V) \
  do { \
    *var_len = sizeof (struct pal_in4_addr); \
    nsm_in_addr_val = V; \
    return (u_char *) &nsm_in_addr_val; \
  } while (0)

#define NSM_CIDR_ROUTE_LOOP(RN,VRF,RIB,NH) \
      for (RN = nsm_ptree_top (VRF->RIB_IPV4_UNICAST); \
           RN; RN = nsm_ptree_next (RN)) \
        if (RN->info != NULL)\
          for (RIB = RN->info; RIB; RIB = RIB->next)\
            for (NH = RIB->nexthop; NH;\
                        NH = NH->next)

#define NSM_ADD_64_UINT(A,B,RESULT)       \
   do {                                   \
   ut_int64_t *_res = &RESULT;            \
   u_int32_t maxv = MAX(A.l[0],B.l[0]);   \
   _res->l[0] = A.l[0] + B.l[0];          \
   _res->l[1] = A.l[1] + B.l[1];          \
   _res->l[1] += (_res->l[0] < maxv)?1:0; \
  } while (0)

void nsm_ent_config_chg_trap ();
void nsm_snmp_init (struct lib_globals *);
void nsm_snmp_deinit (struct lib_globals *);
void nsm_snmp_trap (u_int32_t, u_int32_t, u_int32_t);
struct entPhysicalEntry *nsm_snmp_physical_entity_lookup (
                                                   int index, int exact);

#endif /* _PACOS_NSM_SNMP_H */
