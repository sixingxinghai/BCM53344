/* Copyright (C) 2001-2004  All Rights Reserved. */

#include "pal.h"
#include "lib.h"

#ifdef HAVE_SNMP

#include "asn1.h"
#include "if.h"
#include "log.h"
#include "prefix.h"
#include "table.h"
#include "thread.h"
#include "snmp.h"
#include "pal_math.h"
#include "nexthop.h"
#include "lib/version.h"
#include "vector.h"
#include "pal_sock_raw.h"
#include "pal_if_stats.h"
#include "lib_mtrace.h"

#ifdef HAVE_L3
#include "rib/rib.h"
#include "nsm/rib/nsm_table.h"
#endif /* HAVE_L3 */
#include "nsm/nsmd.h"
#include "nsm/nsm_api.h"
#include "nsm/nsm_snmp.h"
#include "nsm/nsm_server.h"
#include "nsm/nsm_router.h"
#include "nsm/nsm_fea.h"

#ifdef HAVE_MPLS
#include "nsm/mpls/nsm_mpls.h"
#include "nsm/mpls/nsm_mpls_rib.h"
#include "nsm/mpls/nsm_mpls_snmp.h"
#ifdef HAVE_MPLS_VC
#ifdef HAVE_SNMP
#include "nsm/mpls/nsm_mpls_vc_snmp.h"
#endif /* HAVE_SNMP */
#endif /* HAVE_MPLS_VC */
#endif /* HAVE_MPLS */

#ifdef HAVE_L2
#include "l2_snmp.h"
#include "L2/snmp/nsm_L2_snmp.h"
#include "L2/snmp/nsm_p_mib.h"
#if defined (HAVE_I_BEB) || defined (HAVE_B_BEB)
#include "L2/snmp/nsm_pbb_mib.h"
#endif /* HAVE_I_BEB || HAVE_B_BEB */
#ifdef HAVE_VLAN
#include "L2/snmp/nsm_q_mib.h"
#endif /* HAVE_VLAN */
#endif /* HAVE_L2 */

#if defined HAVE_MCAST_IPV4 || defined HAVE_MCAST_IPV6
#include "nsm/mcast/nsm_mcast_snmp.h"
#endif /* HAVE_MCAST_IPV4 || HAVE_MCAST_IPV6 */


#ifdef HAVE_HAL
#include "hal_incl.h"
#include "hal_if.h"
#endif /* HAVE_HAL */
#include "ptree.h"

#ifdef HAVE_HA
#include "lib_cal.h"
#endif /* HAVE_HA */

#if defined HAVE_I_BEB && defined HAVE_B_BEB && defined HAVE_PBB_TE
#include "L2/snmp/nsm_pbb_te_mib.h"
#endif /* HAVE_I_BEB && HAVE_B_BEB && HAVE_PBB_TE */

/* XXX Should be defined in the PAL header file.  */
extern void pal_if_get_stats (struct pal_if_stats *, struct nsm_if_stats *);

/*Declare SNMP local variables here */
static int nsm_int_val;
static ut_int64_t nsm_int64_val;
#ifdef HAVE_L3
static struct pal_in4_addr nsm_in_addr_val;
#endif /* HAVE_L3 */
static u_char *nsm_string_val;
static u_long nsm_ulong_val;
static char nsm_char_string[NSM_MAX_STRING_LENGTH];
static char *nsm_snmp_char_ptr = nsm_char_string;

/* function prototypes */
FindVarMethod nsm_snmp_Scalars;
FindVarMethod nsm_snmp_ifTable;
#ifdef HAVE_L3
FindVarMethod nsm_snmp_ipAddrTable;
FindVarMethod nsm_snmp_ipCidrRouteTable;
#endif /* HAVE_L3 */
FindVarMethod nsm_snmp_entPhysicalTable;
FindVarMethod nsm_snmp_entLogicalTable;
FindVarMethod nsm_snmp_entLPMapTable;
FindVarMethod nsm_snmp_entAliasMapTable;
FindVarMethod nsm_snmp_entPhyContTable;
FindVarMethod nsm_snmp_entScalar;
FindVarMethod nsm_snmp_ifXTable;
#if defined HAVE_HAL && defined HAVE_L2
FindVarMethod nsm_snmp_ifRcvAddressTable;
#endif /* HAVE_HAL && HAVE_L2 */

WriteMethod nsm_snmp_write_ifTable;
WriteMethod nsm_snmp_write_Scalars;
WriteMethod nsm_snmp_write_ipCidrRouteTable;
WriteMethod nsm_snmp_write_ifXTable;
WriteMethod nsm_snmp_write_ifRcvAddressTable;

int nsm_snmp_get_route_type (struct rib *rib, struct nexthop *nexthop);
int nsm_snmp_get_proto (struct rib *rib, struct nexthop *nexthop);
struct cidr_route_entry *nsm_snmp_shadow_route_lookup (
                                        struct pal_in4_addr route_dest,
                                        struct pal_in4_addr route_mask,
                                        struct pal_in4_addr next_hop,
                                        int exact, int index_mask);
int nsm_snmp_route_table_consistency_check (struct pal_in4_addr route_dest,
                                            struct pal_in4_addr route_mask,
                                            struct pal_in4_addr next_hop,
                                            int mask, int value,
                                            bool_t fib_route);
int nsm_snmp_shadow_route_update (struct pal_in4_addr route_dest,
                                  struct pal_in4_addr route_mask,
                                  struct pal_in4_addr next_hop,
                                  int mask, int value,
                                  struct cidr_route_entry *shadow_entry);
int nsm_snmp_set_row_status (struct pal_in4_addr route_dest,
                             struct pal_in4_addr route_mask,
                             struct pal_in4_addr next_hop,
                             int mask,
                             int RowStatus);
void nsm_snmp_shadow_route_delete (struct pal_in4_addr route_dest,
                                   struct pal_in4_addr route_mask,
                                   struct pal_in4_addr next_hop);
struct cidr_route_entry *nsm_snmp_shadow_route_create (void);
int nsm_snmp_shadow_route_list_add (struct cidr_route_entry *cidr_route_entry);

/* The object IDs for registration */
static oid mib2sys_oid[] = {MIB2SYSGROUPOID}; /* 1,3,6,1,2,1,1 */
static oid mib2if_oid[] = {MIB2IFGROUPOID}; /* 1,3,6,1,2,1,2 */
static oid mib2ip_oid[] = {MIB2IPGROUPOID}; /* 1,3,6,1,2,1,4 */
static oid mib2entity_oid[] = {MIB2ENTITYGROUPOID}; /* 1,3,6,1,2,1,47 */
static oid nsmd_oid[] = {NSMDOID};
static oid null_oid[] = { 0, 0 };
static oid sys_oid[] = {1, 2, 3, 4}; /* should be removed after getting API*/
static oid mib2ifX_oid[] = {MIB2IFMIBOID};  /* 1,3,6,1,2,1,31 */

struct variable mib2sys_variables[] =
{
/*
 * magic number , variable type , ro/rw , callback fn , L , oidsuffix
 * (L = length of the oidsuffix)
 */
  /* The System Group (MIB-2, 1)  */
  {NSM_SYSTEMDESC, DISPLAYSTRING, RONLY, nsm_snmp_Scalars, 1, {1}},
  {NSM_SYSOBJID, OBJECT_ID, RONLY, nsm_snmp_Scalars, 1, {2}},
  {NSM_SYSUPTIME, TIMETICKS, RONLY, nsm_snmp_Scalars, 1, {3}},
  {NSM_SYSCONTACT, DISPLAYSTRING, RWRITE, nsm_snmp_Scalars, 1, {4}},
  {NSM_SYSNAME, STRING, RWRITE, nsm_snmp_Scalars, 1, {5}},
  {NSM_SYSLOCATION, DISPLAYSTRING, RWRITE, nsm_snmp_Scalars, 1, {6}},
  {NSM_SYSSERVICIES, INTEGER, RONLY, nsm_snmp_Scalars, 1, {7}},
};

struct variable mib2if_variables[] =
{
/*
 * magic number , variable type , ro/rw , callback fn , L , oidsuffix
 * (L = length of the oidsuffix)
 */
  /* The Interface Group (MIB-2, 2)*/
  {NSM_IFNUMBER, INTEGER, RONLY, nsm_snmp_Scalars, 1, {1}},
  /* ifTable (2, 2)*/
  /* ifEntry (2, 2, 1) */
  {NSM_IFINDEX, INTEGER, RONLY, nsm_snmp_ifTable, 3, {2, 1, 1}},
  {NSM_IFDESCR, STRING, RONLY, nsm_snmp_ifTable, 3, {2, 1, 2}},
  {NSM_IFTYPE, INTEGER, RONLY, nsm_snmp_ifTable, 3, {2, 1, 3}},
  {NSM_IFMTU, INTEGER, RONLY, nsm_snmp_ifTable, 3, {2, 1, 4}},
  {NSM_IFSPEED, GAUGE, RONLY, nsm_snmp_ifTable, 3, {2, 1, 5}},
  {NSM_IFPHYSADDRESS, STRING, RONLY, nsm_snmp_ifTable, 3, {2, 1, 6}},
  {NSM_IFADMINSTATUS, INTEGER, RWRITE, nsm_snmp_ifTable, 3, {2, 1, 7}},
  {NSM_IFOPERSTATUS, INTEGER, RONLY, nsm_snmp_ifTable, 3, {2, 1, 8}},
  {NSM_IFLASTCHANGE, TIMETICKS, RONLY, nsm_snmp_ifTable, 3, {2, 1, 9}},
  {NSM_IFINOCTETS, COUNTER, RONLY, nsm_snmp_ifTable, 3, {2, 1, 10}},
  {NSM_IFINUCASTPKTS, COUNTER, RONLY, nsm_snmp_ifTable, 3, {2, 1, 11}},
  {NSM_IFINNUCASTPKTS, COUNTER, RONLY, nsm_snmp_ifTable, 3, {2, 1, 12}},
  {NSM_IFINDISCARDS, COUNTER, RONLY, nsm_snmp_ifTable, 3, {2, 1, 13}},
  {NSM_IFINERRORS, COUNTER, RONLY, nsm_snmp_ifTable, 3, {2, 1, 14}},
  {NSM_IFINUNKNOWNPROTOS, COUNTER, RONLY, nsm_snmp_ifTable, 3, {2, 1, 15}},
  {NSM_IFOUTOCTETS, COUNTER, RONLY, nsm_snmp_ifTable, 3, {2, 1, 16}},
  {NSM_IFOUTUCASTPKTS, COUNTER, RONLY, nsm_snmp_ifTable, 3, {2, 1, 17}},
  {NSM_IFOUTNUCASTPKTS, COUNTER, RONLY, nsm_snmp_ifTable, 3, {2, 1, 18}},
  {NSM_IFOUTDISCARDS, COUNTER, RONLY, nsm_snmp_ifTable, 3, {2, 1, 19}},
  {NSM_IFOUTERRORS, COUNTER, RONLY, nsm_snmp_ifTable, 3, {2, 1, 20}},
  {NSM_IFOUTQLEN, GAUGE, RONLY, nsm_snmp_ifTable, 3, {2, 1, 21}},
  {NSM_IFSPECIFIC, OBJECT_ID, RONLY, nsm_snmp_ifTable, 3, {2, 1, 22}},
};

struct variable mib2ifX_variables[] =
{
/*
 * magic number , variable type , ro/rw , callback fn , L , oidsuffix
 * (L = length of the oidsuffix)
 */
  {NSM_IFNAME,                    DISPLAYSTRING,    RONLY,  nsm_snmp_ifXTable,
   4, {1, 1, 1, 1}},
#ifdef HAVE_HAL
  {NSM_IFINMULTICASTPKTS,         COUNTER,          RONLY,  nsm_snmp_ifXTable,
   4, {1, 1, 1, 2}},
  {NSM_IFINBROADCASTPKTS,         COUNTER,          RONLY,  nsm_snmp_ifXTable,
   4, {1, 1, 1, 3}},
  {NSM_IFOUTMULTICASTPKTS,        COUNTER,          RONLY,  nsm_snmp_ifXTable,
   4, {1, 1, 1, 4}},
  {NSM_IFOUTBROADCASTPKTS,        COUNTER,          RONLY,  nsm_snmp_ifXTable,
   4, {1, 1, 1, 5}},
  {NSM_IFHCINOCTETS,              COUNTER64,        RONLY,  nsm_snmp_ifXTable,
   4, {1, 1, 1, 6}},
  {NSM_IFHCINUCASTPKTS,           COUNTER64,        RONLY,  nsm_snmp_ifXTable,
   4, {1, 1, 1, 7}},
  {NSM_IFHCINMULTICASTPKTS,       COUNTER64,        RONLY,  nsm_snmp_ifXTable,
   4, {1, 1, 1, 8}},
  {NSM_IFHCINBROADCASTPKTS,       COUNTER64,        RONLY,  nsm_snmp_ifXTable,
   4, {1, 1, 1, 9}},
  {NSM_IFHCOUTOCTETS,             COUNTER64,        RONLY,  nsm_snmp_ifXTable,
   4, {1, 1, 1, 10}},
  {NSM_IFHCOUTUCASTPKTS,          COUNTER64,        RONLY,  nsm_snmp_ifXTable,
   4, {1, 1, 1, 11}},
  {NSM_IFHCOUTMULTICASTPKTS,      COUNTER64,        RONLY,  nsm_snmp_ifXTable,
   4, {1, 1, 1, 12}},
  {NSM_IFHCOUTBROADCASTPKTS,      COUNTER64,        RONLY,  nsm_snmp_ifXTable,
   4, {1, 1, 1, 13}},
#endif /* HAVE_HAL */
  {NSM_IFLINKUPDOWNTRAPENABLE,    INTEGER,          RWRITE, nsm_snmp_ifXTable,
   4, {1, 1, 1, 14}},
  {NSM_IFHIGHSPEED,               GAUGE,            RONLY,  nsm_snmp_ifXTable,
   4, {1, 1, 1, 15}},
  {NSM_IFPROMISCUOUSMODE,         TRUTHVALUE,       RWRITE, nsm_snmp_ifXTable,
   4, {1, 1, 1, 16}},
  {NSM_IFCONNECTORPRESENT,        TRUTHVALUE,       RONLY,  nsm_snmp_ifXTable,
   4, {1, 1, 1, 17}},
  {NSM_IFALIAS,                   DISPLAYSTRING,    RWRITE, nsm_snmp_ifXTable,
   4, {1, 1, 1, 18}},
  {NSM_IFCOUNTERDISCONTINUITYTIME,TIMETICKS,        RONLY,  nsm_snmp_ifXTable,
   4, {1, 1, 1, 19}},

#if defined HAVE_HAL && defined HAVE_L2
  {NSM_IFRCVADDRESSADDRESS,       INTEGER,          NOACCESS,
   nsm_snmp_ifRcvAddressTable,    4, {1, 4, 1, 1}},

  {NSM_IFRCVADDRESSSTATUS,        ROWSTATUS,        RCREATE,
   nsm_snmp_ifRcvAddressTable,    4, {1, 4, 1, 2}},

  {NSM_IFRCVADDRESSTYPE,          INTEGER,          RCREATE,
   nsm_snmp_ifRcvAddressTable,    4, {1, 4, 1, 3}},
#endif /* HAVE_HAL && HAVE_L2 */

  {NSM_IFTBLLASTCHANGE,           TIMETICKS,        RONLY,  nsm_snmp_Scalars, 
   2, {1, 5}}
};

struct variable mib2ip_variables[] =
{
  /*
   * magic number , variable type , ro/rw , callback fn , L , oidsuffix
   * (L = length of the oidsuffix)
   */
  /* The ip address table group (MIB2, 4)*/
  {NSM_IPFORWARDING, INTEGER, RWRITE, nsm_snmp_Scalars, 1, {1}},
  {NSM_IPDEFAULTTTL, INTEGER, RWRITE, nsm_snmp_Scalars, 1, {2}},

#ifdef HAVE_L3
  /* ipAddrTable (4, 20) */
  /* ipAddrEntry (4, 20, 1) */

  {NSM_IPADENTADDR, IPADDRESS, RONLY, nsm_snmp_ipAddrTable, 3, {20, 1, 1}},
  {NSM_IPADENTIFINDEX, INTEGER, RONLY, nsm_snmp_ipAddrTable, 3, {20, 1, 2}},
  {NSM_IPADENTNETMASK, IPADDRESS, RONLY, nsm_snmp_ipAddrTable, 3, {20, 1, 3}},
  {NSM_IPADENTBCASTADDR, INTEGER, RONLY, nsm_snmp_ipAddrTable, 3, {20, 1, 4}},
  {NSM_IPADENTREASMMAXSIZE, INTEGER, RONLY, nsm_snmp_ipAddrTable, 3, {20, 1, 5}},
#endif /* HAVE_L3 */

  /* IP Forward Group (4, 24) */
  {NSM_IPFORWARDNUMBER, GAUGE, RONLY, nsm_snmp_Scalars, 2, {24, 1}},
  {NSM_IPCIDRROUTENUMBER, GAUGE, RONLY, nsm_snmp_Scalars, 2, {24, 3}},

#ifdef HAVE_L3
  /* CIDR Routing Table Group */
  /* IpCidrRouteTable (4, 24, 4) */
  /* IpCidrRouteEntry (4, 24, 4, 1) */

  {NSM_IPCIDRROUTEDEST, IPADDRESS, RONLY, nsm_snmp_ipCidrRouteTable, 4,
  {24, 4, 1, 1}},
  {NSM_IPCIDRROUTEMASK, IPADDRESS, RONLY, nsm_snmp_ipCidrRouteTable, 4,
  {24, 4, 1, 2}},
  {NSM_IPCIDRROUTETOS, INTEGER, RONLY, nsm_snmp_ipCidrRouteTable, 4,
  {24, 4, 1, 3}},
  {NSM_IPCIDRROUTENEXTHOP, IPADDRESS, RONLY, nsm_snmp_ipCidrRouteTable, 4,
  {24, 4, 1, 4}},
  {NSM_IPCIDRROUTEIFINDEX, INTEGER, RCREATE, nsm_snmp_ipCidrRouteTable, 4,
  {24, 4, 1, 5}},
  {NSM_IPCIDRROUTETYPE, INTEGER, RCREATE, nsm_snmp_ipCidrRouteTable, 4,
  {24, 4, 1, 6}},
  {NSM_IPCIDRROUTEPROTO, INTEGER, RONLY, nsm_snmp_ipCidrRouteTable, 4,
  {24, 4, 1, 7}},
  {NSM_IPCIDRROUTEAGE, INTEGER, RONLY, nsm_snmp_ipCidrRouteTable, 4,
  {24, 4, 1, 8}},
  {NSM_IPCIDRROUTEINFO, OBJECT_ID, RCREATE, nsm_snmp_ipCidrRouteTable, 4,
  {24, 4, 1, 9}},
  {NSM_IPCIDRROUTENEXTHOPAS, INTEGER, RCREATE, nsm_snmp_ipCidrRouteTable, 4,
  {24, 4, 1, 10}},
  {NSM_IPCIDRROUTEMETRIC1, INTEGER, RCREATE, nsm_snmp_ipCidrRouteTable, 4,
  {24, 4, 1, 11}},
  {NSM_IPCIDRROUTEMETRIC2, INTEGER, RCREATE, nsm_snmp_ipCidrRouteTable, 4,
  {24, 4, 1, 12}},
  {NSM_IPCIDRROUTEMETRIC3, INTEGER, RCREATE, nsm_snmp_ipCidrRouteTable, 4,
  {24, 4, 1, 13}},
  {NSM_IPCIDRROUTEMETRIC4, INTEGER, RCREATE, nsm_snmp_ipCidrRouteTable, 4,
  {24, 4, 1, 14}},
  {NSM_IPCIDRROUTEMETRIC5, INTEGER, RCREATE, nsm_snmp_ipCidrRouteTable, 4,
  {24, 4, 1, 15}},
  {NSM_IPCIDRROUTESTATUS, ROWSTATUS, RCREATE, nsm_snmp_ipCidrRouteTable, 4,
  {24, 4, 1, 16}},
#endif /* HAVE_L3 */
};

struct variable mib2entity_variables[] =
{
/*
 * magic number , variable type , ro/rw , callback fn , L , oidsuffix
 * (L = length of the oidsuffix)
 */
  /* The Entity Group (MIB-2, 47)  */
  {NSM_ENTITYPHYIDX,         INTEGER,       NOACCESS, nsm_snmp_entPhysicalTable,   5, {1, 1, 1, 1, 1}},
  {NSM_ENTITYPHYDESCR,       DISPLAYSTRING, RONLY,    nsm_snmp_entPhysicalTable,   5, {1, 1, 1, 1, 2}},
  {NSM_ENTITYPHYVENDORTYPE,  STRING,        RONLY,    nsm_snmp_entPhysicalTable,   5, {1, 1, 1, 1, 3}},
  {NSM_ENTITYPHYCONTAINEDIN, INTEGER,       RONLY,    nsm_snmp_entPhysicalTable,   5, {1, 1, 1, 1, 4}},
  {NSM_ENTITYPHYCLASS,       INTEGER,       RONLY,    nsm_snmp_entPhysicalTable,   5, {1, 1, 1, 1, 5}},
  {NSM_ENTITYPHYPARENTRELPOS,INTEGER,       RONLY,    nsm_snmp_entPhysicalTable,   5, {1, 1, 1, 1, 6}},
  {NSM_ENTITYPHYNAME,        DISPLAYSTRING, RONLY,    nsm_snmp_entPhysicalTable,   5, {1, 1, 1, 1, 7}},
  {NSM_ENTITYPHYHWREV,       DISPLAYSTRING, RONLY,    nsm_snmp_entPhysicalTable,   5, {1, 1, 1, 1, 8}},
  {NSM_ENTITYPHYFWREV,       DISPLAYSTRING, RONLY,    nsm_snmp_entPhysicalTable,   5, {1, 1, 1, 1, 9}},
  {NSM_ENTITYPHYSWREV,       DISPLAYSTRING, RONLY,    nsm_snmp_entPhysicalTable,   5, {1, 1, 1, 1, 10}},
  {NSM_ENTITYPHYSERIALNO,    DISPLAYSTRING, RWRITE,   nsm_snmp_entPhysicalTable,   5, {1, 1, 1, 1, 11}},
  {NSM_ENTITYPHYMFGNAME,     DISPLAYSTRING, RONLY,    nsm_snmp_entPhysicalTable,   5, {1, 1, 1, 1, 12}},
  {NSM_ENTITYPHYMODELNAME,   DISPLAYSTRING, RONLY,    nsm_snmp_entPhysicalTable,   5, {1, 1, 1, 1, 13}},
  {NSM_ENTITYPHYALIAS,       DISPLAYSTRING, RWRITE,   nsm_snmp_entPhysicalTable,   5, {1, 1, 1, 1, 14}},
  {NSM_ENTITYPHYASSETID,     DISPLAYSTRING, RWRITE,   nsm_snmp_entPhysicalTable,   5, {1, 1, 1, 1, 15}},
  {NSM_ENTITYPHYISFRU,       INTEGER,       RONLY,    nsm_snmp_entPhysicalTable,   5, {1, 1, 1, 1, 16}},
  {NSM_ENTITYLOGIDX,         INTEGER,       NOACCESS, nsm_snmp_entLogicalTable,    5, {1, 2, 1, 1, 1}},
  {NSM_ENTITYLOGDESCR,       DISPLAYSTRING, RONLY,    nsm_snmp_entLogicalTable,    5, {1, 2, 1, 1, 2}},
  {NSM_ENTITYLOGTYPE,        STRING,        RONLY,    nsm_snmp_entLogicalTable,    5, {1, 2, 1, 1, 3}},
  {NSM_ENTITYLOGCOMMUNITY,   STRING,        RONLY,    nsm_snmp_entLogicalTable,    5, {1, 2, 1, 1, 4}},
  {NSM_ENTITYLOGTADDRESS,    STRING,        RONLY,    nsm_snmp_entLogicalTable,    5, {1, 2, 1, 1, 5}},
  {NSM_ENTITYLOGTDOMAIN,     STRING,        RONLY,    nsm_snmp_entLogicalTable,    5, {1, 2, 1, 1, 6}},
  {NSM_ENTITYLOGCONTEXTENGID,STRING,        RONLY,    nsm_snmp_entLogicalTable,    5, {1, 2, 1, 1, 7}},
  {NSM_ENTITYLOGCONTEXTNAME, DISPLAYSTRING, RONLY,    nsm_snmp_entLogicalTable,    5, {1, 2, 1, 1, 8}},
  {NSM_ENTITYLPPHYSICALIDX,  INTEGER,       RONLY,    nsm_snmp_entLPMapTable,      5, {1, 3, 1, 1, 1}},
  {NSM_ENTITYALIASIDXORZERO, INTEGER,       NOACCESS, nsm_snmp_entAliasMapTable,   5, {1, 3, 2, 1, 1}},
  {NSM_ENTITYALIASMAPID,     STRING,        RONLY,    nsm_snmp_entAliasMapTable,   5, {1, 3, 2, 1, 2}},
  {NSM_ENTITYPHYCHILDIDX,    INTEGER,       RONLY,    nsm_snmp_entPhyContTable,    5, {1, 3, 3, 1, 1}},
  {NSM_ENTITYLASTCHANGE,     TIMETICKS,     RONLY,    nsm_snmp_entScalar,
   3, {1, 4, 1}}
};

void
snmp_shadow_init (struct lib_globals *zg)
{
  snmp_shadow_route_table = list_new();
  if (snmp_shadow_route_table == NULL)
    zlog_err(zg, "Cannot allocate snmp shadow route table\n");

   return;
}

void
nsm_snmp_deinit (struct lib_globals *zg)
{
  struct listnode *node;
  struct cidr_route_entry *shadow_entry;

  LIST_LOOP (snmp_shadow_route_table, shadow_entry, node)
    XFREE (MTYPE_NSM_SNMP_ROUTE_ENTRY, shadow_entry);

  list_free (snmp_shadow_route_table);

#ifdef HAVE_L2
#ifdef HAVE_VLAN
  xstp_snmp_del_l2_snmp();
#endif /* HAVE_VLAN */
#endif /* HAVE_L2 */

  snmp_stop (zg);

  return;
}

/* Callback procedure for nsm mib scalar objects */
unsigned char *
nsm_snmp_Scalars (struct variable *vp, oid *name,
                  size_t *length, int exact, size_t *var_len,
                  WriteMethod **write_method, u_int32_t vr_id)
{
  static char sysDesc[SYS_DESC_LEN];
  static char sysContact[SYS_CONTACT_LEN];
  static char sysName[SYS_NAME_LEN];
  static char sysLocation[SYS_LOC_LEN];
#ifdef HAVE_L3
  s_int32_t ipforward = 0;
#endif /* HAVE_L3 */

  if (snmp_header_generic (vp, name, length, exact, var_len, write_method)
      == MATCH_FAILED)
    return NULL;

 /*
 * this is where we do the value assignments for the mib results.
 */
  switch (vp->magic)
  {
    case NSM_SYSTEMDESC:
      nsm_get_sys_desc (sysDesc);
      NSM_SNMP_RETURN_STRING (sysDesc, pal_strlen(sysDesc));
      break;

    case NSM_SYSOBJID:
      /*sys_oid = nsm_get_sys_oid();*/
      NSM_SNMP_RETURN_OID (sys_oid, NELEMENTS (sys_oid));
      break;

    case NSM_SYSUPTIME:
      NSM_SNMP_RETURN_TIMETICKS (nsm_get_sys_up_time());
      break;

    case NSM_SYSCONTACT:
      *write_method = NULL;
      pal_snprintf (sysContact, sizeof (sysContact) - 1,
          "Contact E-Mail Address: %s", PACOS_BUG_ADDRESS);
      NSM_SNMP_RETURN_STRING (sysContact, pal_strlen(sysContact));
      break;

    case NSM_SYSNAME:
      *write_method = NULL;
      nsm_get_sys_name (sysName);
      NSM_SNMP_RETURN_STRING (sysName, pal_strlen(sysName));
      break;

    case NSM_SYSLOCATION:
      *write_method = NULL;
      nsm_get_sys_location (sysLocation);
      NSM_SNMP_RETURN_STRING (sysLocation, pal_strlen(sysLocation));
      break;

    case NSM_SYSSERVICIES:
      nsm_int_val = nsm_get_sys_services ();
      NSM_SNMP_RETURN_INTEGER (nsm_int_val);
      break;

    case NSM_IFNUMBER:
      NSM_SNMP_RETURN_INTEGER (nzg->ifg.if_list->count);
      break;

    case NSM_IFTBLLASTCHANGE:
      NSM_SNMP_RETURN_INTEGER (nzg->ifg.ifTblLastChange);
      break;

#ifdef HAVE_L3
    case NSM_IPFORWARDING:
      *write_method = nsm_snmp_write_Scalars;
       nsm_fea_ipv4_forwarding_get (&ipforward );
       if (ipforward)
         NSM_SNMP_RETURN_INTEGER (NSM_SNMP_IPFORWARDING_ENABLE);
       else
         NSM_SNMP_RETURN_INTEGER (NSM_SNMP_IPFORWARDING_DISABLE);
       break;

    case NSM_IPDEFAULTTTL:
      *write_method = NULL;
      NSM_SNMP_RETURN_INTEGER (IPDEFTTL);
      break;

    case NSM_IPFORWARDNUMBER:
      NSM_SNMP_RETURN_INTEGER (nsm_get_forward_number_ipv4());
      break;

    case NSM_IPCIDRROUTENUMBER:
      NSM_SNMP_RETURN_INTEGER (nsm_get_forward_number_ipv4());
      break;
#else
    case NSM_IPFORWARDING:
      *write_method = NULL;
      NSM_SNMP_RETURN_INTEGER (NSM_SNMP_IPFORWARDING_DISABLE);
       break;

    case NSM_IPDEFAULTTTL:
      *write_method = NULL;
      NSM_SNMP_RETURN_INTEGER (IPDEFTTL);
      break;

    case NSM_IPFORWARDNUMBER:
      NSM_SNMP_RETURN_INTEGER (0);
      break;

    case NSM_IPCIDRROUTENUMBER:
      NSM_SNMP_RETURN_INTEGER (0);
      break;
#endif /* HAVE_L3 */

    default:
      pal_assert (0);
      break;

    }

   return NULL;
}

/* Write procedure for nsm mib scalar objects */
int
nsm_snmp_write_Scalars (int action, u_char * var_val,
                        u_char var_val_type, size_t var_val_len,
                        u_char * statP, oid * name, size_t length,
                        struct variable *v, u_int32_t vr_id)
{
  long intval;
  bool_t ipForward;

  if (var_val_type != ASN_INTEGER)
    return SNMP_ERR_WRONGTYPE;

  if (var_val_len != sizeof (long))
    return SNMP_ERR_WRONGLENGTH;

  pal_mem_cpy(&intval,var_val,4);

  switch (v->magic)
    {
    case NSM_IPFORWARDING:
       switch (intval)
         {
         case NSM_SNMP_IPFORWARDING_ENABLE:
           ipForward = PAL_TRUE;
           break;
         case NSM_SNMP_IPFORWARDING_DISABLE:
           ipForward = PAL_FALSE;
           break;
         default:
           return SNMP_ERR_BADVALUE;
           break;
         }

       if (nsm_fea_ipv4_forwarding_set (ipForward) != RESULT_OK)
         return SNMP_ERR_GENERR;

       break;

     default:
      pal_assert (0);
      return SNMP_ERR_GENERR;
      break;
    }

  return SNMP_ERR_NOERROR;

}

/* Utility procedure to get the Integer Index.
 */
int
nsm_snmp_index_get (struct variable *v, oid *name,
                    size_t *length, u_int32_t *index, int exact)
{

  int result;
  int len;

  *index = 0;

  result = oid_compare (name, v->namelen, v->name, v->namelen);

  if (exact)
    {
      /* Check the length. */
      if (result != 0 || *length - v->namelen != 1)
        return -1;

      *index = name[v->namelen];
      return 0;
    }
  else                          /* GETNEXT request */
    {
      /* return -1 if greater than our our table entry */
      if (result > 0)
        return -1;
      else if (result == 0)
        {
          /* use index value if present, otherwise 0. */
          len = *length - v->namelen;
          if (len >= 1)
            *index = name[v->namelen];
          return 0;
        }
      else
        {
          /* set the user's oid to be ours */
          pal_mem_cpy (name, v->name, ((int) v->namelen) * sizeof (oid));
          return 0;
        }
    }
}

/* Utility procedure to set the object name and INTEGER index. */
void
nsm_snmp_index_set (struct variable *v, oid *name,
                    size_t *length, u_int32_t index)
{
  name[v->namelen] = index;
  *length = v->namelen + 1;
}

/* Lookup procedure for ifTable */
struct interface *
nsm_snmp_interface_lookup (int if_instance, int exact)
{
  struct interface *ifp = NULL;
  struct interface *best = NULL;
  struct listnode *node  = NULL;

  LIST_LOOP (nzg->ifg.if_list, ifp, node)
    {
      if (exact)
        {
          if (ifp->ifindex == if_instance)
            {
              best = ifp;
              break;
            }
        }
      else
        {
          if (if_instance < ifp->ifindex)
            {
              /* save if this is a better candidate and
               *   continue search
               */
              if (best == NULL)
                best = ifp;
              else if (ifp->ifindex < best->ifindex)
                best = ifp;
            }
        }
    }
  return best;
}

/* Callback procedure for ifTable */
unsigned char *
nsm_snmp_ifTable (struct variable *vp, oid *name,
                  size_t *length, int exact, size_t *var_len,
                  WriteMethod **write_method, u_int32_t vr_id)
{
  int ret;
  u_int32_t index = 0;
  struct interface *ifp = NULL;
  struct nsm_if_stats nsm_stats;

  pal_mem_set (&nsm_stats, 0, sizeof (struct nsm_if_stats));

  /* Validate the index */
  ret = nsm_snmp_index_get (vp, name, length, &index, exact);
  if (ret < 0)
    return NULL;

  ifp = nsm_snmp_interface_lookup (index, exact);
  if (ifp == NULL)
    return NULL;

  if (!exact)
    nsm_snmp_index_set (vp, name, length, ifp->ifindex);

  NSM_IF_STAT_UPDATE (ifp->vr);
  pal_if_get_stats (&(ifp->stats), &nsm_stats);

  switch (vp->magic)
    {
    case NSM_IFINDEX:
      NSM_SNMP_RETURN_INTEGER (ifp->ifindex);
      break;

    case NSM_IFDESCR:
      if (ifp->desc)
        NSM_SNMP_RETURN_STRING (ifp->desc, pal_strlen (ifp->desc));
      else
        NSM_SNMP_RETURN_STRING (ifp->name, pal_strlen (ifp->name));
      break;

    case NSM_IFTYPE:
      switch (ifp->hw_type)
        {
          case IF_TYPE_LOOPBACK:
            NSM_SNMP_RETURN_INTEGER (NSM_SNMP_INTERFACE_SOFTWARELOOPBACK);
            break;

          case IF_TYPE_ETHERNET:
            NSM_SNMP_RETURN_INTEGER (NSM_SNMP_INTERFACE_ETHERNETCSMACD);
            break;

          case IF_TYPE_PPP:
            NSM_SNMP_RETURN_INTEGER (NSM_SNMP_INTERFACE_PPP);
            break;

          case IF_TYPE_FRELAY:
            NSM_SNMP_RETURN_INTEGER (NSM_SNMP_INTERFACE_FRAMERELAY);
            break;

          case IF_TYPE_ATM:
          case IF_TYPE_UNKNOWN:
          case IF_TYPE_HDLC:
          default:
            NSM_SNMP_RETURN_INTEGER (NSM_SNMP_INTERFACE_OTHER);
            break;
        }
      break;

    case NSM_IFMTU:
      NSM_SNMP_RETURN_INTEGER (ifp->mtu);
      break;

    case NSM_IFSPEED:
      NSM_SNMP_RETURN_INTEGER (ifp->bandwidth*8); /* converted to bits */
      break;

    case NSM_IFPHYSADDRESS:
      NSM_SNMP_RETURN_STRING (ifp->hw_addr, ifp->hw_addr_len);
      break;

    case NSM_IFADMINSTATUS:
      *write_method = nsm_snmp_write_ifTable;
      if (if_is_up (ifp))
        {
          NSM_SNMP_RETURN_INTEGER (NSM_SNMP_INTERFACE_UP);
        }
      else
        {
          NSM_SNMP_RETURN_INTEGER (NSM_SNMP_INTERFACE_DOWN);
        }
      break;

    case NSM_IFOPERSTATUS:
      if (if_is_running (ifp))
        {
          NSM_SNMP_RETURN_INTEGER (NSM_SNMP_INTERFACE_UP);
        }
      else
        {
          NSM_SNMP_RETURN_INTEGER (NSM_SNMP_INTERFACE_DOWN);
        }
      break;

    case NSM_IFLASTCHANGE:
      NSM_SNMP_RETURN_TIMETICKS (ifp->ifLastChange);
      break;

    case NSM_IFINOCTETS:
      NSM_SNMP_RETURN_INTEGER (nsm_stats.ifInOctets);
      break;

    case NSM_IFINUCASTPKTS:
      NSM_SNMP_RETURN_INTEGER (nsm_stats.ifInUcastPkts);
      break;

    case NSM_IFINNUCASTPKTS:
      NSM_SNMP_RETURN_INTEGER (nsm_stats.ifInNUcastPkts);
      break;

    case NSM_IFINDISCARDS:
      NSM_SNMP_RETURN_INTEGER (nsm_stats.ifInDiscards);
      break;

    case NSM_IFINERRORS:
      NSM_SNMP_RETURN_INTEGER (nsm_stats.ifInErrors);
      break;

    case NSM_IFINUNKNOWNPROTOS:
      NSM_SNMP_RETURN_INTEGER (nsm_stats.ifInUnknownProtos);
      break;

    case NSM_IFOUTOCTETS:
      NSM_SNMP_RETURN_INTEGER (nsm_stats.ifOutOctets);
      break;

    case NSM_IFOUTUCASTPKTS:
      NSM_SNMP_RETURN_INTEGER (nsm_stats.ifOutUcastPkts);
      break;

    case NSM_IFOUTNUCASTPKTS:
      NSM_SNMP_RETURN_INTEGER (nsm_stats.ifOutNUcastPkts);
      break;

    case NSM_IFOUTDISCARDS:
      NSM_SNMP_RETURN_INTEGER (nsm_stats.ifOutDiscards);
      break;

    case NSM_IFOUTERRORS:
      NSM_SNMP_RETURN_INTEGER (nsm_stats.ifOutErrors);
      break;

    case NSM_IFOUTQLEN:
      break;

    case NSM_IFSPECIFIC:
      NSM_SNMP_RETURN_OID (null_oid, NELEMENTS (null_oid));
      break;

    default:
      pal_assert (0);
      break;

    }

  return NULL;
}

/* Write procedure for ifTable */
int
nsm_snmp_write_ifTable (int action, u_char * var_val,
                        u_char var_val_type, size_t var_val_len,
                        u_char * statP, oid * name, size_t length,
                        struct variable *vp, u_int32_t vr_id)
{
  u_int32_t index = 0;
  long intval;
  struct interface *ifp = NULL;
  struct apn_vr *vr = NULL;

  if (var_val_type != ASN_INTEGER)
    return SNMP_ERR_WRONGTYPE;

  if (var_val_len != sizeof (long))
    return SNMP_ERR_WRONGLENGTH;

  if (nsm_snmp_index_get (vp, name, &length, &index, 1))
    {
      return SNMP_ERR_GENERR;
    }

  pal_mem_cpy(&intval,var_val,4);

  vr = apn_vr_get_privileged (nzg);
  ifp = if_lookup_by_index (&vr->ifm, index);
  if (ifp == NULL)
    {
      return SNMP_ERR_GENERR;
    }

  switch (vp->magic)
    {
    case NSM_IFADMINSTATUS:
      if (intval != 1 && intval != 2)
        {
          return SNMP_ERR_BADVALUE;
        }
      if (intval == NSM_SNMP_INTERFACE_UP)
        {
          nsm_if_flag_up_set (ifp->vr->id, ifp->name, PAL_TRUE);
        }
      else
        {
          nsm_if_flag_up_unset (ifp->vr->id, ifp->name, PAL_TRUE);
        }
      break;

    default:
      pal_assert (0);
      return SNMP_ERR_GENERR;
      break;
    }

  return SNMP_ERR_NOERROR;

}

/* Callback procedure for ifXTable */
unsigned char *
nsm_snmp_ifXTable (struct variable *vp, oid *name,
                   size_t *length, int exact, size_t *var_len,
                   WriteMethod **write_method, u_int32_t vr_id)
{
  int ret;
  u_int32_t index = 0;
  struct interface *ifp = NULL;
  struct nsm_if_stats nsm_stats;
#ifdef HAVE_HAL
  struct hal_if_counters if_stats;
  ut_int64_t tot_rcvd;
#endif /* HAVE_HAL */
  u_int32_t val;
  u_int32_t temp_val;

  pal_mem_set (&nsm_stats, 0, sizeof (struct nsm_if_stats));
  pal_mem_set (&nsm_int64_val, 0, sizeof (ut_int64_t));
  val = 0;
  temp_val = 0;

  /* Validate the index */
  ret = nsm_snmp_index_get (vp, name, length, &index, exact);
  if (ret < 0)
    return NULL;

  ifp = nsm_snmp_interface_lookup (index, exact);
  if (ifp == NULL)
    return NULL;

  if (!exact)
    nsm_snmp_index_set (vp, name, length, ifp->ifindex);

#ifdef ENABLE_HAL_PATH
  hal_if_get_counters (ifp->ifindex, &if_stats);
#else
  NSM_IF_STAT_UPDATE (ifp->vr);
  pal_if_get_stats (&(ifp->stats), &nsm_stats);
#endif /* ENABLE_HAL_PATH */

  switch (vp->magic)
    {
    case NSM_IFNAME:
      if (pal_strlen (ifp->name))
        NSM_SNMP_RETURN_STRING (ifp->name, pal_strlen (ifp->name));
      else
        NSM_SNMP_RETURN_STRING (ifp->desc, pal_strlen (ifp->desc));
      break;

#ifdef HAVE_HAL
    case NSM_IFINMULTICASTPKTS:
      pal_mem_cpy (&nsm_int64_val, &if_stats.mc_pkts_rcv,
                                    sizeof (ut_int64_t));
      /* As per the RFC 2233 section 5 the 
       * ifInMulticastPkts is 32 bit, howerver the HW maintains 64 bit
       * counters. Hence copy only the LSB 
       */
      val = pal_hton32 (nsm_int64_val.l[0]);
      NSM_SNMP_RETURN_INTEGER (val);
      break;

    case NSM_IFINBROADCASTPKTS:
      pal_mem_cpy (&nsm_int64_val, &if_stats.brdc_pkts_rcv,
                                    sizeof (ut_int64_t));
      /* As per the RFC 2233 section 5 the 
       * ifInBroadcastPkts is 32 bit, howerver the HW maintains 64 bit
       * counters. Hence copy only the LSB 
       */
      val = pal_hton32 (nsm_int64_val.l[0]);
      NSM_SNMP_RETURN_INTEGER (val);
      break;

    case NSM_IFOUTMULTICASTPKTS:
      pal_mem_cpy (&nsm_int64_val, &if_stats.mc_pkts_sent,
                                    sizeof (ut_int64_t));
      /* As per the RFC 2233 section 5 the 
       * ifOutMulticastPkts is 32 bit, howerver the HW maintains 64 bit
       * counters. Hence copy only the LSB 
       */
      val = pal_hton32 (nsm_int64_val.l[0]);
      NSM_SNMP_RETURN_INTEGER (val);
      break;

    case NSM_IFOUTBROADCASTPKTS:
      pal_mem_cpy (&nsm_int64_val, &if_stats.brdc_pkts_sent,
                                    sizeof (ut_int64_t));
      /* As per the RFC 2233 section 5 the 
       * ifOutMulticastPkts is 32 bit, howerver the HW maintains 64 bit
       * counters. Hence copy only the LSB 
       */
      val = pal_hton32 (nsm_int64_val.l[0]);
      NSM_SNMP_RETURN_INTEGER (val);
      break;

    case NSM_IFHCINOCTETS:
      NSM_ADD_64_UINT (if_stats.good_octets_rcv,
                       if_stats.bad_octets_rcv,
                       tot_rcvd);
      pal_mem_cpy (&nsm_int64_val, &tot_rcvd,
                                   sizeof (ut_int64_t));
      NSM_SNMP_RETURN_INTEGER64 (nsm_int64_val);
      break;

    case NSM_IFHCINUCASTPKTS:
      NSM_ADD_64_UINT (if_stats.good_pkts_rcv,
                       if_stats.bad_pkts_rcv,
                       tot_rcvd);

      pal_mem_cpy (&nsm_int64_val, &tot_rcvd,
                                    sizeof (ut_int64_t));
      NSM_SNMP_RETURN_INTEGER64 (nsm_int64_val);
      break;

    case NSM_IFHCINMULTICASTPKTS:
      pal_mem_cpy (&nsm_int64_val, &if_stats.mc_pkts_rcv,
                                    sizeof (ut_int64_t));
      NSM_SNMP_RETURN_INTEGER64 (nsm_int64_val);
      break;

    case NSM_IFHCINBROADCASTPKTS:
      pal_mem_cpy (&nsm_int64_val, &if_stats.brdc_pkts_rcv,
                                     sizeof (ut_int64_t));
      NSM_SNMP_RETURN_INTEGER64 (nsm_int64_val);
      break;

    case NSM_IFHCOUTOCTETS:
      pal_mem_cpy (&nsm_int64_val, &if_stats.good_octets_sent,
                                    sizeof (ut_int64_t));
      NSM_SNMP_RETURN_INTEGER64 (nsm_int64_val);
      break;

    case NSM_IFHCOUTUCASTPKTS:
      pal_mem_cpy (&nsm_int64_val, &if_stats.good_pkts_sent,
                                    sizeof (ut_int64_t));
      NSM_SNMP_RETURN_INTEGER64 (nsm_int64_val);
      break;

    case NSM_IFHCOUTMULTICASTPKTS:
       pal_mem_cpy (&nsm_int64_val, &if_stats.mc_pkts_sent,
                                     sizeof (ut_int64_t));
      NSM_SNMP_RETURN_INTEGER64 (nsm_int64_val);
      break;

    case NSM_IFHCOUTBROADCASTPKTS:
      pal_mem_cpy (&nsm_int64_val, &if_stats.brdc_pkts_sent,
                                    sizeof (ut_int64_t));
      NSM_SNMP_RETURN_INTEGER64 (nsm_int64_val);
      break;
#endif /* HAVE_HAL */

    case NSM_IFLINKUPDOWNTRAPENABLE:
      *write_method = nsm_snmp_write_ifXTable;
      NSM_SNMP_RETURN_INTEGER (ifp->if_linktrap);
      break;

    case NSM_IFHIGHSPEED:
      NSM_SNMP_RETURN_INTEGER (ifp->bandwidth*8/1000000);
      /* converted to million bits/sec */
      break;

    case NSM_IFPROMISCUOUSMODE:
      *write_method = nsm_snmp_write_ifXTable;
      if (ifp->flags & IFF_PROMISC)
        NSM_SNMP_RETURN_INTEGER (NSM_SNMP_PROMISC_TRUE);
      else
        NSM_SNMP_RETURN_INTEGER (NSM_SNMP_PROMISC_FALSE);
      break;

    case NSM_IFCONNECTORPRESENT:
      if ((ifp->flags & IFF_UP) && (ifp->flags & IFF_RUNNING))
        NSM_SNMP_RETURN_INTEGER (NSM_SNMP_CONN_TRUE);
      else
        NSM_SNMP_RETURN_INTEGER (NSM_SNMP_CONN_FALSE);
      break;

    case NSM_IFALIAS:
      *write_method = nsm_snmp_write_ifXTable;
      NSM_SNMP_RETURN_STRING (ifp->if_alias, pal_strlen (ifp->if_alias));
      break;

    case NSM_IFCOUNTERDISCONTINUITYTIME:
      /* Not Supported */
      break;

    default:
      pal_assert (0);
      break;

    }

  return NULL;
}

/* Write procedure for ifXTable */
int
nsm_snmp_write_ifXTable (int action, u_char * var_val,
                         u_char var_val_type, size_t var_val_len,
                         u_char * statP, oid * name, size_t length,
                         struct variable *vp, u_int32_t vr_id)
{
  u_int32_t index = 0;
  long intval;
  struct interface *ifp = NULL;
  struct apn_vr *vr = NULL;

  if (nsm_snmp_index_get (vp, name, &length, &index, 1))
    {
      return SNMP_ERR_GENERR;
    }

  if (!var_val)
    return SNMP_ERR_GENERR;

  pal_mem_cpy(&intval,var_val,4);

  vr = apn_vr_get_privileged (nzg);
  ifp = if_lookup_by_index (&vr->ifm, index);
  if (ifp == NULL)
    {
      return SNMP_ERR_GENERR;
    }

  switch (vp->magic)
    {
    case NSM_IFLINKUPDOWNTRAPENABLE:
      if (var_val_type != ASN_INTEGER)
        return SNMP_ERR_WRONGTYPE;
      if (var_val_len != sizeof (long))
        return SNMP_ERR_WRONGLENGTH;
      if (intval != NSM_IF_TRAP_ENABLE && intval != NSM_IF_TRAP_DISABLE)
        {
          return SNMP_ERR_BADVALUE;
        }
      if (intval == NSM_IF_TRAP_ENABLE)
        ifp->if_linktrap = NSM_IF_TRAP_ENABLE;
      else
        ifp->if_linktrap = NSM_IF_TRAP_DISABLE;
      break;

    case NSM_IFPROMISCUOUSMODE:
      if (var_val_type != ASN_INTEGER)
        return SNMP_ERR_WRONGTYPE;
      if (var_val_len != sizeof (long))
        return SNMP_ERR_WRONGLENGTH;
      if (intval != NSM_SNMP_PROMISC_TRUE && intval != NSM_SNMP_PROMISC_FALSE)
        {
          return SNMP_ERR_BADVALUE;
        }
      if (intval == NSM_SNMP_PROMISC_TRUE)
        nsm_fea_if_flags_set(ifp, IFF_PROMISC);
      else
        nsm_fea_if_flags_unset(ifp, IFF_PROMISC);
      break;

    case NSM_IFALIAS:
      if (var_val_type != ASN_OCTET_STR)
        return SNMP_ERR_WRONGTYPE;
      pal_mem_cpy (ifp->if_alias, var_val, INTERFACE_NAMSIZ + 1);
      ifp->if_alias[INTERFACE_NAMSIZ + 1] = '\0'; //var_val_len
      break;

    default:
      pal_assert (0);
      return SNMP_ERR_GENERR;
    }

#ifdef HAVE_HA
  lib_cal_modify_interface (NSM_ZG, ifp);
#endif /* HAVE_HA */

  return SNMP_ERR_NOERROR;

}

 /* Receive Address Table
 */
int
nsm_snmp_rcvaddr_index_get (struct variable *v, oid *name, size_t *length,
                            int exact, struct rcvaddr_index *rcvaddr)
{
  int len = 0;
  int j = 0;
  int i;

  if (exact)
    {
      /* Check the length. */
      if (*length - v->namelen != (ETHER_ADDR_LEN + 1))
        return -1;

      rcvaddr->ifindex = name[v->namelen];

      for (i = 1; i <= ETHER_ADDR_LEN ; i++)
        {
          rcvaddr->mac_addr[j++] = (name [v->namelen + i] > UINT8_MAX ?
                                    UINT8_MAX : name [v->namelen + i]);
        }
    }
  else
    {
      pal_mem_set( rcvaddr, 0, sizeof(struct rcvaddr_index) );

      len = *length - v->namelen;
      if (len >= (ETHER_ADDR_LEN + 1))
        {
          rcvaddr->ifindex = name[v->namelen];

          for (i = 1; i <= ETHER_ADDR_LEN ; i++)
            {
              rcvaddr->mac_addr[j++] = (name [v->namelen + i] > UINT8_MAX ?
                                        UINT8_MAX : name [v->namelen + i]);
            }
        }
    }

  return 0;
}

void
nsm_snmp_rcvaddr_index_set (struct variable *v, oid *name, size_t *length,
                            struct rcvaddr_index *rcvaddr)
{
  int j = 0;
  int i;

  name[v->namelen] = rcvaddr->ifindex;

  for (i = 1; i <= ETHER_ADDR_LEN ; i++)
    {
      name [v->namelen + i] = rcvaddr->mac_addr[j++];
    }

  *length = v->namelen + (ETHER_ADDR_LEN + 1);
}

#if defined HAVE_HAL && defined HAVE_L2
unsigned char *
nsm_snmp_ifRcvAddressTable (struct variable *vp, oid *name,
                            size_t *length, int exact, size_t *var_len,
                            WriteMethod **write_method, u_int32_t vr_id)
{
  struct rcvaddr_index rcvaddr;
  struct interface *ifp = NULL;
  int status = 0;
  int type = 0;
  int ret;

  pal_mem_set (&rcvaddr, 0, sizeof (struct rcvaddr_index));

  /* Validate the index */
  ret = nsm_snmp_rcvaddr_index_get (vp, name, length, exact, &rcvaddr);
  if (ret < 0)
    return NULL;

  ifp = nsm_snmp_interface_lookup (rcvaddr.ifindex, exact);
  if (ifp == NULL)
    return NULL;

  switch (vp->magic)
    {
      case NSM_IFRCVADDRESSADDRESS:
      /* Not Accessible */
        break;

      case NSM_IFRCVADDRESSSTATUS:
        *write_method = nsm_snmp_write_ifRcvAddressTable;
        if (exact)
          {
            if (nsm_get_rcvaddress_status (&rcvaddr, &status))
              NSM_SNMP_RETURN_INTEGER (status);
          }
        else
          {
            if (nsm_get_next_rcvaddress_status (&rcvaddr, &status))
              {
                rcvaddr.ifindex = ifp->ifindex;
                nsm_snmp_rcvaddr_index_set (vp, name, length, &rcvaddr);
                NSM_SNMP_RETURN_INTEGER (status);
              }
          }
        break;

      case NSM_IFRCVADDRESSTYPE:
        *write_method = nsm_snmp_write_ifRcvAddressTable;
        if (exact)
          {
            if (nsm_get_rcvaddress_type (&rcvaddr, &type))
              NSM_SNMP_RETURN_INTEGER (type);
          }
        else
          {
            if (nsm_get_next_rcvaddress_type (&rcvaddr, &type))
              {
                rcvaddr.ifindex = ifp->ifindex;
                nsm_snmp_rcvaddr_index_set (vp, name, length, &rcvaddr);
                NSM_SNMP_RETURN_INTEGER (type);
              }
          }
        break;

      default:
        pal_assert (0);
        break;
    }
  return NULL;
}

/* Write procedure for ifTable */
int
nsm_snmp_write_ifRcvAddressTable (int action, u_char * var_val,
                                  u_char var_val_type, size_t var_val_len,
                                  u_char * statP, oid * name, size_t length,
                                  struct variable *vp, u_int32_t vr_id)
{
  struct rcvaddr_index rcvaddr;
  struct interface *ifp = NULL;
  struct nsm_master *nm = NULL;
  struct apn_vr *vr = NULL;
  int intval;

  pal_mem_set (&rcvaddr, 0, sizeof (struct rcvaddr_index));

  if (nsm_snmp_rcvaddr_index_get (vp, name, &length, 1, &rcvaddr))
    return SNMP_ERR_GENERR;

  pal_mem_cpy (&intval, var_val, 4);

  vr = apn_vr_get_privileged (nzg);
  ifp = if_lookup_by_index (&vr->ifm, rcvaddr.ifindex);
  if (ifp == NULL)
    return SNMP_ERR_GENERR;

  nm = nsm_master_lookup_by_id (nzg, 0);
  if (nm == NULL)
    return RESULT_ERROR;

  switch (vp->magic)
    {
      case NSM_IFRCVADDRESSSTATUS:
        if (intval < NSM_SNMP_ROW_STATUS_ACTIVE ||
            intval > NSM_SNMP_ROW_STATUS_DESTROY)
          return SNMP_ERR_BADVALUE;
        if (nsm_set_rcvaddress_status (nm, &rcvaddr, intval))
          return SNMP_ERR_NOERROR;
      break;
  
      case NSM_IFRCVADDRESSTYPE:
        if (intval != VOLATILE && intval != NONVOLATILE)
          return SNMP_ERR_BADVALUE;
        if (nsm_set_rcvaddress_type (&rcvaddr, &intval))
          return SNMP_ERR_NOERROR;
        break;

    default:
      pal_assert (0);
      return SNMP_ERR_GENERR;
      break;
    }

  return SNMP_ERR_NOERROR;
}
#endif /* HAVE_L2 && HAVE_HAL*/

#ifdef HAVE_L3
/* IP Address Index Get procedure.
 */

int
nsm_snmp_ipaddr_index_get (struct variable *v, oid *name, size_t *length,
                           struct pal_in4_addr *addr, int exact)
{
  int len;

  if (exact)
    {
      /* Check the length. */
      if (*length - v->namelen != IN_ADDR_SIZE)
        return -1;

      oid2in_addr (name + v->namelen, IN_ADDR_SIZE, addr);
      return 0;
    }
  else
    {
      len = *length - v->namelen;
      if (len > 4) len = 4;

      oid2in_addr (name + v->namelen, len, addr);
      return 0;
    }
  return -1;
}

/* IP Address index set procedure */
void
nsm_snmp_ipaddr_index_set (struct variable *v, oid *name, size_t *length,
                           struct pal_in4_addr *addr)
{
  oid_copy_addr (name + v->namelen, addr, IN_ADDR_SIZE);
  *length = v->namelen + IN_ADDR_SIZE;
}

/* lookup procedure for ipAddrTable */
struct connected *
nsm_snmp_ipAddrTable_lookup (struct pal_in4_addr addr, int exact)
{

  struct listnode *node = NULL;
  struct interface *ifp = NULL;
  struct connected *rn = NULL;
  struct connected *best = NULL;

  LIST_LOOP (nzg->ifg.if_list, ifp, node)
    {
      for (rn = ifp->ifc_ipv4; rn; rn = rn->next)
        {
          if (exact)
            {
              if (addr.s_addr == rn->address->u.prefix4.s_addr)
                {
                  best = rn;
                  break;
                }
            }
          else
            {
              if ((IPV4_ADDR_CMP (&addr, &rn->address->u.prefix4)) < 0)
                {
                  /* save if this is a better candidate and
                   * continue search
                   */

                  if (best == NULL)
                    best = rn;
                  else if ((IPV4_ADDR_CMP (&rn->address->u.prefix4,
                          &best->address->u.prefix4)) < 0)
                    best = rn;
                }
            }
        }
    }
  return best;
}

/* Callback procedure for ipAddrTable */
unsigned char *
nsm_snmp_ipAddrTable (struct variable *vp,
                      oid * name,
                      size_t * length,
                      int exact,
                      size_t * var_len,
                      WriteMethod ** write_method, 
                      u_int32_t vr_id)
{
  int ret;
  struct pal_in4_addr addr;
  struct pal_in4_addr netmask;
  struct connected *ifc;

  pal_mem_set (&addr, 0, IN_ADDR_SIZE);
  pal_mem_set (&netmask, 0, IN_ADDR_SIZE);

  ret = nsm_snmp_ipaddr_index_get (vp, name, length, &addr, exact);
  if (ret < 0)
    return NULL;

  ifc = nsm_snmp_ipAddrTable_lookup (addr, exact);
  if (ifc == NULL)
    return NULL;

  if (!exact)
    nsm_snmp_ipaddr_index_set (vp, name, length, &(ifc->address->u.prefix4));

  switch (vp->magic)
    {
    case NSM_IPADENTADDR:
      NSM_SNMP_RETURN_IPADDRESS (ifc->address->u.prefix4);
      break;

    case NSM_IPADENTIFINDEX:
      NSM_SNMP_RETURN_INTEGER (ifc->ifp->ifindex);
      break;

    case NSM_IPADENTNETMASK:
      masklen2ip (ifc->address->prefixlen, &netmask);
      NSM_SNMP_RETURN_IPADDRESS (netmask);
      break;

    case NSM_IPADENTBCASTADDR:
      if (if_is_broadcast (ifc->ifp))
        NSM_SNMP_RETURN_INTEGER (1); /* Broadcast address will always have
                                      * last bit as 1 */
      else
        NSM_SNMP_RETURN_INTEGER (0); /* If the interface in non-broadcast */
      break;

    case NSM_IPADENTREASMMAXSIZE:
      /* not supported */
      break;

    default:
      pal_assert (0);
      break;
    }
  return NULL;
}

/* Index Get procedure for ipCidrRouteTable
 * Index for this table is route dest, route mask, TOS and Next HOP
 */
int
nsm_snmp_cidr_route_table_index_get (struct variable *v, oid *name,
                                     size_t *length,
                                     struct pal_in4_addr *route_dest,
                                     struct pal_in4_addr *route_mask,
                                     struct pal_in4_addr *next_hop,
                                     int *tos,
                                     int exact)
{
  int destlen;
  int masklen;
  int nexthoplen;
  int toslen;

  if (exact)
    {
      /* Check the length */
      if (*length - v->namelen != (3*IN_ADDR_SIZE + 1))
        {
          return -1;
        }

      /* Fill up the values for all four indices */
      oid2in_addr (name + v->namelen, IN_ADDR_SIZE,
                   route_dest);
      oid2in_addr (name + v->namelen + IN_ADDR_SIZE,
                   IN_ADDR_SIZE, route_mask);
      *tos = name[v->namelen + 2*IN_ADDR_SIZE];
      oid2in_addr (name + v->namelen + 2*IN_ADDR_SIZE + 1,
                   IN_ADDR_SIZE, next_hop);
      return 0;
    }
  else
    {
      pal_mem_set (route_dest, 0, IN_ADDR_SIZE);
      pal_mem_set (route_mask, 0, IN_ADDR_SIZE);
      pal_mem_set (next_hop, 0, IN_ADDR_SIZE);
      *tos = 0;

      /* The user might have specified all the index value while doing a
       * GET-NEXT, or may be just few of them.
       */
      destlen = masklen = nexthoplen = toslen = *length - v->namelen;
      if (destlen > IN_ADDR_SIZE)
        {
          destlen = IN_ADDR_SIZE;
        }
      oid2in_addr (name + v->namelen, destlen, route_dest);

      masklen = masklen - destlen;
      if (masklen > IN_ADDR_SIZE)
        {
          masklen = IN_ADDR_SIZE;
        }
      oid2in_addr (name + v->namelen + destlen, masklen, route_mask);

      toslen = toslen - (masklen + destlen);
      if (toslen > 1)
        {
          toslen = 1;
        }
      *tos = name[v->namelen + destlen + masklen];

      nexthoplen = nexthoplen - (toslen + masklen + destlen);
      if (nexthoplen > 0)
        {
          nexthoplen = IN_ADDR_SIZE;
          oid2in_addr (name + v->namelen + destlen + masklen + toslen,
                       nexthoplen, next_hop);
        }

       return 0;
    }

  return -1;
}

/* Utility procedure to SET the index values for ipCidrRouteTable */
void
nsm_snmp_cidr_route_table_index_set (struct variable *v, oid *name,
                                     size_t *length,
                                     struct pal_in4_addr *route_dest,
                                     struct pal_in4_addr *route_mask,
                                     int tos,
                                     struct pal_in4_addr *next_hop)
{
  /* Append route_dest, route_mask, tos, and next_hop to the mib oid */
  oid_copy (name, v->name, v->namelen);
  oid_copy_addr (name + v->namelen, route_dest, IN_ADDR_SIZE);
  oid_copy_addr (name + v->namelen + IN_ADDR_SIZE, route_mask, IN_ADDR_SIZE);
  name[v->namelen + 2*IN_ADDR_SIZE] = tos;
  oid_copy_addr (name + v->namelen + ((2*IN_ADDR_SIZE) + 1),
                next_hop, IN_ADDR_SIZE);
  *length = v->namelen + 3*IN_ADDR_SIZE + 1;

  return;
}

/* Procedure to take the values from backend and fill-in snmp route structure */
void
nsm_snmp_route_entry_update (struct nsm_ptree_node *node,
                             struct rib *rib,
                             struct nexthop *nexthop,
                             struct cidr_route_entry *route_entry)
{
  struct pal_in4_addr routemask;
  struct prefix_ipv4 addr;

  pal_mem_set (&routemask, 0, sizeof(struct pal_in4_addr));
  pal_mem_set (&addr, 0, sizeof (struct prefix_ipv4));

  NSM_PTREE_KEY_COPY(&addr.prefix, node);
  route_entry->ipCidrRouteDest.s_addr = addr.prefix.s_addr;

  masklen2ip (node->key_len, &routemask);
  route_entry->ipCidrRouteMask.s_addr = routemask.s_addr;

  route_entry->ipCidrRouteNextHop.s_addr = nexthop->gate.ipv4.s_addr;
  route_entry->ipCidrRouteIfIndex = nexthop->ifindex;
  route_entry->ipCidrRouteType = nsm_snmp_get_route_type (rib, nexthop);
  route_entry->ipCidrRouteProto = nsm_snmp_get_proto (rib, nexthop);
  route_entry->ipCidrRouteAge = pal_time_current (NULL)- rib->uptime;
  route_entry->ipCidrRouteMetric1 = rib->metric;

  return;
}

/* Get the type of route as local, remote, reject or other. */

int
nsm_snmp_get_route_type (struct rib *rib, struct nexthop *nexthop)
{

  /* We shall classify only those routes which are
   * used for forwarding.This is recommened in RFC 2096.
   */
  if (CHECK_FLAG (nexthop->flags, NEXTHOP_FLAG_FIB) &&
     (CHECK_FLAG (rib->flags, RIB_FLAG_SELECTED)))
    {
      if (CHECK_FLAG (rib->flags, RIB_FLAG_BLACKHOLE))
        {
           return NSM_SNMP_ROUTE_REJECT;
        }

      switch (rib->type)
        {
        case APN_ROUTE_RIP:
        case APN_ROUTE_RIPNG:
        case APN_ROUTE_OSPF:
        case APN_ROUTE_OSPF6:
        case APN_ROUTE_BGP:
          return NSM_SNMP_ROUTE_REMOTE;
          break;

        case APN_ROUTE_CONNECT:
          return NSM_SNMP_ROUTE_LOCAL;
          break;

        case APN_ROUTE_STATIC:
          switch (nexthop->snmp_route_type)
            {
              case ROUTE_TYPE_LOCAL:
                return NSM_SNMP_ROUTE_LOCAL;
                break;

              case ROUTE_TYPE_REMOTE:
                return NSM_SNMP_ROUTE_REMOTE;
                break;

              case ROUTE_TYPE_OTHER:
              default:
                return NSM_SNMP_ROUTE_OTHER;
                break;

            }
          break;

        case APN_ROUTE_KERNEL:
        case APN_ROUTE_DEFAULT:
        default:
          return NSM_SNMP_ROUTE_OTHER;
            break;
        }
    }

  return 0;
}

/* Procedure to get the protocol type of the route. */
int
nsm_snmp_get_proto (struct rib *rib, struct nexthop *nexthop)
{
  /* We shall return the proto type for only those routes which are being
   * used in forwarding.
   */
  if (CHECK_FLAG (nexthop->flags, NEXTHOP_FLAG_FIB))
    {
      switch (rib->type)
        {
        case APN_ROUTE_DEFAULT:
        case APN_ROUTE_STATIC:
          return NSM_SNMP_PROTO_NETMGMT;
          break;

        case APN_ROUTE_CONNECT:
          return NSM_SNMP_PROTO_LOCAL;
          break;

        case APN_ROUTE_RIP:
        case APN_ROUTE_RIPNG:
          return NSM_SNMP_PROTO_RIP;
          break;

        case APN_ROUTE_OSPF:
        case APN_ROUTE_OSPF6:
          return NSM_SNMP_PROTO_OSPF;
          break;

        case APN_ROUTE_BGP:
          return NSM_SNMP_PROTO_BGP;
          break;

        case APN_ROUTE_ISIS:
          return NSM_SNMP_PROTO_ISIS;
          break;

        case APN_ROUTE_KERNEL:
        default:
          return NSM_SNMP_PROTO_OTHER;
          break;

        }
    }
  return 0;
}

/* Utility to get the exact matching entry for given dest address, mask
 * and next hop address.We do not consider TOS value while doing get.
 * Depending on the value of find_active parameter, we hsall either return
 * only those routes which are used in forwarding, or return all the routes.
 */
int
nsm_snmp_cidr_route_get (struct pal_in4_addr route_dest,
                         struct pal_in4_addr route_mask,
                         struct pal_in4_addr next_hop,
                         struct nsm_ptree_node *node,
                         struct rib *info,
                         struct nexthop *nhop,
                         bool_t find_active)
{
  struct nsm_master *nm = NULL;
  struct nsm_vrf *vrf = NULL;
  struct rib *rib = NULL;
  struct nsm_ptree_node *rn = NULL;
  struct pal_in4_addr routemask;
  struct nexthop *nexthop = NULL;
  struct prefix_ipv4 p;

  pal_mem_set (&routemask, 0, sizeof(struct pal_in4_addr));
  
  nm = nsm_master_lookup_by_id (nzg, 0);
  if (nm == NULL)
    return RESULT_ERROR;

  vrf = nsm_vrf_lookup_by_id (nm, 0);
  if (vrf == NULL)
    return RESULT_ERROR;

  NSM_CIDR_ROUTE_LOOP (rn, vrf, rib, nexthop)
    {
      pal_mem_set(&p, 0, sizeof (struct prefix_ipv4));
      p.prefixlen = rn->key_len;
      NSM_PTREE_KEY_COPY(&p.prefix, rn);

      masklen2ip (p.prefixlen, &routemask);
      if ((p.prefix.s_addr == route_dest.s_addr) &&
          (routemask.s_addr == route_mask.s_addr) &&
          (nexthop->gate.ipv4.s_addr == next_hop.s_addr))
        {
          if (find_active)
            {
              if (CHECK_FLAG (nexthop->flags, NEXTHOP_FLAG_FIB) &&
                 (CHECK_FLAG (rib->flags, RIB_FLAG_SELECTED)))
                {
                  pal_mem_cpy (node, rn, sizeof(struct nsm_ptree_node));
                  pal_mem_cpy (info, rib, sizeof(struct rib));
                  pal_mem_cpy (nhop, nexthop, sizeof(struct nexthop));
                  nsm_ptree_unlock_node (rn);
                  return RESULT_OK;
                }
            }
          else
            {
              pal_mem_cpy (node, rn, sizeof(struct nsm_ptree_node));
              pal_mem_cpy (info, rib, sizeof(struct rib));
              pal_mem_cpy (nhop, nexthop, sizeof(struct nexthop));
              nsm_ptree_unlock_node (rn);
              return RESULT_OK;
            }
        }
    }
  return RESULT_ERROR;
}

/* This function will return the nexthop with least ip address
 * if the flag is true, else it will return the nexthop
 * with least ip address among the list of nexthops which
 * are greater than the given next_hop.
 */
struct nexthop*
nsm_snmp_cidr_nexthop_get_next (struct nexthop *nexthop, int flag,
                                struct pal_in4_addr next_hop)
{
  struct nexthop *temp = NULL;
  struct nexthop *selected = NULL;

  if (flag == PAL_TRUE)
    {
      for (temp = nexthop; temp; temp = temp->next)
        {
          if (! selected)
            selected = temp;

          if (selected && selected != temp)
            {
              if (IPV4_ADDR_CMP (&temp->gate.ipv4, &selected->gate.ipv4) < 0)
                selected = temp;
            }
        }

      return selected;

    }
  else
    {
      for (temp = nexthop; temp; temp = temp->next)
        {
          if (IPV4_ADDR_CMP (&next_hop.s_addr, &temp->gate.ipv4) < 0)
            {
              if (! selected)
                selected = temp;
              if (selected && selected != temp)
                {
                  if (IPV4_ADDR_CMP (&temp->gate.ipv4,
                                     &selected->gate.ipv4) < 0)
                    selected = temp;
                }
            }
        }
      return selected;
    }
}

/* This function will return the next higher cidr route entry
 * from the shadow table based on the route_dest, route_mask
 * next_hop and index_mask values
 */
struct cidr_route_entry *
nsm_snmp_shadow_cidr_route_get_next (struct pal_in4_addr route_dest,
                                     struct pal_in4_addr route_mask,
                                     struct pal_in4_addr next_hop,
                                     int index_mask)
{
  struct listnode *node = NULL;
  struct cidr_route_entry *shadow_entry =NULL;
  struct cidr_route_entry *selected =NULL;
  int ret = 0;

  if ((index_mask & CIDR_NO_INDEX)
      || (index_mask & CIDR_FULL_INDEX)
      || (index_mask & CIDR_PARTIAL_INDEX))
    {
      LIST_LOOP (snmp_shadow_route_table, shadow_entry, node)
        {
          ret = IPV4_ADDR_CMP (&route_dest,
                               &shadow_entry->ipCidrRouteDest); 
        
          if ((index_mask & CIDR_NO_INDEX) || ret < 0)
            {
              if (! selected)
                selected = shadow_entry;
                
              if (selected && selected != shadow_entry)
                {
                  ret = IPV4_ADDR_CMP (&shadow_entry->ipCidrRouteDest, 
                                       &selected->ipCidrRouteDest);
                  if (ret < 0)
                    selected = shadow_entry;
                  if (ret == 0)
                    {
                      ret = IPV4_ADDR_CMP (&shadow_entry->ipCidrRouteMask, 
                                           &selected->ipCidrRouteMask);
                      if ( ret < 0)
                        selected = shadow_entry;
                      else if (ret == 0)
                        {
                  if (IPV4_ADDR_CMP (&shadow_entry->ipCidrRouteNextHop, 
                                     &selected->ipCidrRouteNextHop) < 0)
                            selected = shadow_entry;                  
                        }
                    }          
                }      
            }   
          
          if (ret == 0)
            {
              if ((index_mask & CIDR_FULL_INDEX)
                  || ((index_mask & CIDR_PARTIAL_INDEX) 
                      && route_mask.s_addr))
                {
                  ret = IPV4_ADDR_CMP (&route_mask, 
                                       &shadow_entry->ipCidrRouteMask);
                  if (ret < 0)
                    { 
                      if (!selected)
                        selected = shadow_entry;

                      if (IPV4_ADDR_CMP (&shadow_entry->ipCidrRouteMask,
                                         &selected->ipCidrRouteMask) < 0)
                        selected = shadow_entry; 
                    }
                  if (ret == 0 && (index_mask & CIDR_FULL_INDEX))
                    {
                      ret = IPV4_ADDR_CMP (&next_hop, 
                                           &shadow_entry->ipCidrRouteNextHop);
                      if (ret < 0)
                        {
                          if (! selected)
                            selected = shadow_entry;
                          if (IPV4_ADDR_CMP (&shadow_entry->ipCidrRouteNextHop,
                                             &selected->ipCidrRouteNextHop) < 0)
                            selected = shadow_entry;
                        }
                    }
                }
            }   

        }
      return selected;
    }
  return NULL;
}

int
nsm_snmp_cidr_route_get_next (struct pal_in4_addr route_dest,
                              struct pal_in4_addr route_mask,
                              struct pal_in4_addr next_hop,
                              struct nsm_ptree_node *node,
                              struct rib *info,
                              struct nexthop *nhop,
                              int index_mask,
                              bool_t *route_in_fib,
                              struct cidr_route_entry *cidr_route_entry)
{
  struct nsm_master *nm = NULL;
  struct nsm_vrf *vrf = NULL;
  struct nsm_ptree_node *rn = NULL;  
  struct nsm_ptree_node *best = NULL;    
  struct rib *rib = NULL; 
  struct nexthop *nexthop = NULL;
  struct nexthop *least_nexthop = NULL;
  struct pal_in4_addr routemask;  
  struct cidr_route_entry *selected_shadow_entry = NULL;
  int ret = 0; 
  struct prefix_ipv4 p;
  struct prefix_ipv4 addr;

  nm = nsm_master_lookup_by_id (nzg, 0);
  if (nm == NULL)
    return RESULT_ERROR;

  vrf = nsm_vrf_lookup_by_id (nm, 0);
  if (vrf == NULL)
    return RESULT_ERROR;

  pal_mem_set (&routemask, 0, sizeof(struct pal_in4_addr));

  /* CIDR_NO_INDEX   - If no index is passed return the first row */
  /* CIDR_FULL_INDEX - If full index is passed return row which has
                       next higher index values */ 
  /* CIDR_FULL_INDEX - If partial index is passed return the row 
                       which has next highest values than the current 
                       partial indices */
  if ((index_mask & CIDR_NO_INDEX) || 
      (index_mask & CIDR_FULL_INDEX) || 
      (index_mask & CIDR_PARTIAL_INDEX))
    {
      NSM_CIDR_ROUTE_LOOP (rn, vrf, rib, nexthop)
        {
          pal_mem_set (&p, 0, sizeof(struct prefix_ipv4));
          p.prefixlen = rn->key_len;
          NSM_PTREE_KEY_COPY (&p.prefix, rn);

          if (CHECK_FLAG (nexthop->flags, NEXTHOP_FLAG_FIB) &&
              (CHECK_FLAG (rib->flags, RIB_FLAG_SELECTED)))
            {          
              ret = IPV4_ADDR_CMP (&route_dest,
                                   &p.prefix);
               
              if ((index_mask & CIDR_NO_INDEX)
                  || (ret < 0 && nexthop->next == NULL))
                {
                  best = rn;             
                  goto compare;          
                }
               
              if (ret < 0 && nexthop->next)
                {
                  /* If rn has more than one nexthops, select the
                     least entry first */  
                  least_nexthop =
                    nsm_snmp_cidr_nexthop_get_next (nexthop, PAL_TRUE,
                                                    next_hop);
                  if (least_nexthop)
                    {
                      best = rn;                       
                      goto compare;
                    }
                }   

              if (ret == 0)
                {
                  if ((index_mask & CIDR_FULL_INDEX)
                      || ((index_mask & CIDR_PARTIAL_INDEX)
                          && (route_mask.s_addr != 0)))
                    {
                      masklen2ip (p.prefixlen, &routemask);
                      ret = IPV4_ADDR_CMP (&route_mask,
                                           &routemask);
                      if (ret < 0 && nexthop->next == NULL)
                        {
                          best = rn;
                          goto compare;
                        }

                      if (ret < 0 && nexthop->next)
                        {
                          /* If rn has more than one nexthops, select the
                           * least entry first 
                          */
                          least_nexthop =
                          nsm_snmp_cidr_nexthop_get_next (nexthop, PAL_TRUE,
                                                          next_hop);
                          if (least_nexthop)
                            {
                              best = rn;
                              goto compare;
                            }
                        }
                      if (ret == 0 && nexthop->next)
                        {
                          /* If the destination address and mask are same,
                           * and if the rn has more than one nexthop,
                           * select the next higher nexthop 
                          */
                          least_nexthop =
                          nsm_snmp_cidr_nexthop_get_next (nexthop, PAL_FALSE,
                                                          next_hop);
                          if (least_nexthop)
                            {
                              best = rn;
                              goto compare;
                            }
                        }
                  
                      if ((index_mask & CIDR_FULL_INDEX) && ret == 0)
                        {
                          ret = IPV4_ADDR_CMP (&next_hop.s_addr,
                                               &nexthop->gate.ipv4);
                          if (ret < 0)
                            {
                              best = rn;
                              goto compare;
                            }
                        }
                    } 
                } 
            }
        } 
    }

  /* Compare the route entry from the main table and 
   * the shadow table and return the entry which is 
   * lesser than the other
   */  
 compare:
  selected_shadow_entry = nsm_snmp_shadow_cidr_route_get_next
    (route_dest,route_mask,next_hop, index_mask);

  if (!best && !selected_shadow_entry)
    return RESULT_ERROR;
      
  else if (!best && selected_shadow_entry)
    {
      *route_in_fib = PAL_FALSE;
      goto end;
    }
  else if (best && !selected_shadow_entry)
    {
      *route_in_fib = PAL_TRUE;
      goto end;
    }
  else
    {
      pal_mem_set (&addr, 0, sizeof (struct prefix_ipv4));
      addr.prefixlen = best->key_len;
      NSM_PTREE_KEY_COPY (&addr.prefix, best);

      if (addr.prefix.s_addr == 0)
        {
          *route_in_fib = PAL_TRUE;
          goto end;
        }
      if (selected_shadow_entry->ipCidrRouteDest.s_addr == 0)
        {
          *route_in_fib = PAL_FALSE;
          goto end;
        }
          
      if (IPV4_ADDR_CMP (&addr.prefix, 
                         &selected_shadow_entry->ipCidrRouteDest) < 0)
        {
          *route_in_fib = PAL_TRUE;  
          goto end;
        }
          
      else if (IPV4_ADDR_CMP (&addr.prefix, 
                      &selected_shadow_entry->ipCidrRouteDest) == 0)    
        {
          masklen2ip (addr.prefixlen, &routemask);
          if (IPV4_ADDR_CMP (&routemask, 
                             &selected_shadow_entry->ipCidrRouteMask) < 0)
            {
              *route_in_fib = PAL_TRUE;
              goto end;
            }
          else if (IPV4_ADDR_CMP (&routemask, 
                          &selected_shadow_entry->ipCidrRouteMask) == 0)
            {
              rib = best->info;         
              if (least_nexthop)
                {
                  if (IPV4_ADDR_CMP (&least_nexthop->gate.ipv4, 
                             &selected_shadow_entry->ipCidrRouteNextHop) < 0)
                    {
                      *route_in_fib = PAL_TRUE;
                      goto end;
                    }
                  *route_in_fib = PAL_FALSE;
                  goto end; 
                } 
              if (nexthop && IPV4_ADDR_CMP (&nexthop->gate.ipv4, 
                            &selected_shadow_entry->ipCidrRouteNextHop) < 0)
                {
                  *route_in_fib = PAL_TRUE;
                  goto end;
                }
              *route_in_fib = PAL_FALSE;
            }
        }
      else
        *route_in_fib = PAL_FALSE;
    }
  
 end:
 
  if (*route_in_fib == PAL_TRUE)
    {
      pal_mem_cpy (node, best, sizeof(struct nsm_ptree_node));
      rib = best->info;
      pal_mem_cpy (info, rib, sizeof (struct rib));
      if (least_nexthop)
        pal_mem_cpy (nhop, least_nexthop, sizeof (struct nexthop));    
      else
        pal_mem_cpy (nhop, nexthop, sizeof (struct nexthop));        
        
      nsm_ptree_unlock_node (best);
      return RESULT_OK;
    }
  else 
    {
      pal_mem_cpy (cidr_route_entry, selected_shadow_entry, 
                   sizeof (struct cidr_route_entry));
      return RESULT_OK; 
    }
    
  return RESULT_ERROR;
    
}

/* Callback function for ipCidrRouteTable */
unsigned char *
nsm_snmp_ipCidrRouteTable (struct variable *vp,
                           oid * name,
                           size_t * length,
                           int exact,
                           size_t * var_len, 
                           WriteMethod ** write_method,
                           u_int32_t vr_id)
{
  int ret;
  struct pal_in4_addr route_dest;
  struct pal_in4_addr route_mask;
  struct pal_in4_addr next_hop;
  int tos = 0;
  struct nsm_ptree_node node;
  struct rib info;
  struct nexthop nhop;
  struct cidr_route_entry route_entry;
  struct cidr_route_entry *cidr_route_entry = NULL;
  struct cidr_route_entry cidr_rt_entry;
  bool_t route_in_fib = PAL_TRUE;
  int index_mask = 0;
  
  /* Initialize all the declared structures */
  pal_mem_set (&route_dest, 0, sizeof(struct pal_in4_addr));
  pal_mem_set (&route_mask, 0, sizeof(struct pal_in4_addr));
  pal_mem_set (&next_hop, 0, sizeof(struct pal_in4_addr));
  pal_mem_set (&node, 0, sizeof(struct nsm_ptree_node));
  pal_mem_set (&info, 0, sizeof(struct rib));
  pal_mem_set (&nhop, 0, sizeof(struct nexthop));
  pal_mem_set (&route_entry, 0, sizeof (struct cidr_route_entry));

  /* validate the index */
  ret = nsm_snmp_cidr_route_table_index_get (vp, name, length, &route_dest, 
                                            &route_mask, &next_hop, &tos,
                                            exact);
  if (ret < 0)
    return NULL;

  /* For the read-create objects we shall set the write_method before hand */
  switch (vp->magic)
    {
    case NSM_IPCIDRROUTEIFINDEX:
    case NSM_IPCIDRROUTETYPE:
    case NSM_IPCIDRROUTEMETRIC1:
    case NSM_IPCIDRROUTESTATUS:
      *write_method = nsm_snmp_write_ipCidrRouteTable;
      break;

    default:
      break;
    }

  /* First check if the fib has the route entry */
  if (exact)
    {

      ret = nsm_snmp_cidr_route_get (route_dest, route_mask, next_hop, &node,
                                     &info, &nhop, PAL_TRUE);
      if (ret == RESULT_OK)
        {
     
          /* We have found the route in fib table.Get the needed values
           */
          nsm_snmp_route_entry_update (&node, &info, &nhop, &route_entry);
        }
      else
        {
          /* If we did not find any matching route in fib, we will look in
           * the snmp shadow route table.
           */
          route_in_fib = PAL_FALSE;
          cidr_route_entry = nsm_snmp_shadow_route_lookup (route_dest, route_mask,
                                                           next_hop, exact,
                                                           index_mask);
          if (cidr_route_entry != NULL)
            pal_mem_cpy (&route_entry, cidr_route_entry,
                         sizeof(struct cidr_route_entry));
          else  /* we did not find route in fib, as well as snmp shadow table */
            return NULL;
        }
      
    }
  else
    {
      if ((*length - vp->namelen) == 0)
        index_mask = index_mask | CIDR_NO_INDEX;
      else if ((*length - vp->namelen) == CIDR_FULL_INDEX_LEN)
        index_mask = index_mask | CIDR_FULL_INDEX;
      else if ((*length - vp->namelen) > 0)
        index_mask = index_mask | CIDR_PARTIAL_INDEX;

      ret = nsm_snmp_cidr_route_get_next (route_dest, route_mask, next_hop,
                                          &node, &info, &nhop, index_mask,
                                          &route_in_fib, &cidr_rt_entry); 
      if (route_in_fib && ret == RESULT_OK)
        nsm_snmp_route_entry_update (&node, &info, &nhop, &route_entry);
      else
        {
          if (ret == RESULT_OK)
            pal_mem_cpy (&route_entry, &cidr_rt_entry,
                         sizeof(struct cidr_route_entry));
          else /*we did not find route in fib, as well as snmp shadow table */
            return NULL;
        }
    }

  if (!exact)
    nsm_snmp_cidr_route_table_index_set (vp, name, length,
                                         &(route_entry.ipCidrRouteDest),
                                         &(route_entry.ipCidrRouteMask),
                                         0, &(route_entry.ipCidrRouteNextHop));
    
  switch (vp->magic)
    {
    case NSM_IPCIDRROUTEDEST:
      NSM_SNMP_RETURN_IPADDRESS (route_entry.ipCidrRouteDest);  
      break;

    case NSM_IPCIDRROUTEMASK:
      NSM_SNMP_RETURN_IPADDRESS (route_entry.ipCidrRouteMask);
      break;

    case NSM_IPCIDRROUTETOS:
      /* not supported */
      break;

    case NSM_IPCIDRROUTENEXTHOP:
      NSM_SNMP_RETURN_IPADDRESS (route_entry.ipCidrRouteNextHop);
      break; 

    case NSM_IPCIDRROUTEIFINDEX:
      NSM_SNMP_RETURN_INTEGER (route_entry.ipCidrRouteIfIndex);
      break;

    case NSM_IPCIDRROUTETYPE:
      NSM_SNMP_RETURN_INTEGER (route_entry.ipCidrRouteType);
      break;

    case NSM_IPCIDRROUTEPROTO: 
      NSM_SNMP_RETURN_INTEGER (route_entry.ipCidrRouteProto);
      break; 

    case NSM_IPCIDRROUTEAGE:
      NSM_SNMP_RETURN_TIMETICKS (route_entry.ipCidrRouteAge);       
      break;

    case NSM_IPCIDRROUTEINFO:
      *write_method = NULL;
      NSM_SNMP_RETURN_OID (null_oid, NELEMENTS (null_oid));
      break;

    case NSM_IPCIDRROUTENEXTHOPAS:
      /* Not supporting this object. */
      break;

    case NSM_IPCIDRROUTEMETRIC1:
      NSM_SNMP_RETURN_INTEGER (route_entry.ipCidrRouteMetric1);
      break;

    case NSM_IPCIDRROUTEMETRIC2:
      *write_method = NULL;
      NSM_SNMP_RETURN_INTEGER (-1);
      break;

    case NSM_IPCIDRROUTEMETRIC3:
      *write_method = NULL;
      NSM_SNMP_RETURN_INTEGER (-1);
      break;

    case NSM_IPCIDRROUTEMETRIC4:
      *write_method = NULL;
      NSM_SNMP_RETURN_INTEGER (-1);
      break;

    case NSM_IPCIDRROUTEMETRIC5:
      *write_method = NULL;
      NSM_SNMP_RETURN_INTEGER (-1);
      break;

    case NSM_IPCIDRROUTESTATUS:
      /* If we have this route entry in fib then it will always be active.
       */
      if (route_in_fib)
        NSM_SNMP_RETURN_INTEGER (NSM_SNMP_ROW_STATUS_ACTIVE);
      else  
        NSM_SNMP_RETURN_INTEGER (route_entry.ipCidrRouteStatus);
      break;
       
    default:
      break;

    }

  return NULL;

}

/* write method for ipCidrRouteTable */
int
nsm_snmp_write_ipCidrRouteTable (int action, u_char * var_val,
                                 u_char var_val_type, size_t var_val_len,
                                 u_char * statP, oid * name, size_t length,
                                 struct variable *vp, u_int32_t vr_id)
{
  long intval;
  int ret = 0;
  int tos = 0;
  struct pal_in4_addr route_dest;
  struct pal_in4_addr route_mask;
  struct pal_in4_addr next_hop;
  struct cidr_route_entry *shadow_entry = NULL;
  struct nsm_ptree_node node;
  struct rib info;
  struct nexthop nhop;
  int set_mask = 0;
  bool_t route_in_fib = PAL_FALSE;

  if (var_val_type != ASN_INTEGER)
    return SNMP_ERR_WRONGTYPE;

  /* Not checking this condition for now due to smux issue. SMUX is
   * sending the value as 1 byte length. Should be uncommented after
   * fixing smux.
   */
  /*if (var_val_len != sizeof (long))
    return SNMP_ERR_WRONGLENGTH;
  */

  pal_mem_set (&route_dest, 0, sizeof(struct pal_in4_addr));
  pal_mem_set (&route_mask, 0, sizeof(struct pal_in4_addr));
  pal_mem_set (&next_hop, 0, sizeof(struct pal_in4_addr));
  pal_mem_set (&node, 0, sizeof(struct nsm_ptree_node));
  pal_mem_set (&info, 0, sizeof(struct rib));
  pal_mem_set (&nhop, 0, sizeof(struct nexthop));


  /*Get the dest address, mask and nexthop */
  ret = nsm_snmp_cidr_route_table_index_get (vp, name,&length,
                                         &route_dest, &route_mask, &next_hop,
                                         &tos, 1);
  if (ret < 0)
    return SNMP_ERR_GENERR;

  /* Check here if the route is present in fib (is already row-status
   * active), then do not allow a set.
   */
  ret = nsm_snmp_cidr_route_get (route_dest, route_mask, next_hop,
                                 &node, &info, &nhop, PAL_TRUE);
  if (ret == RESULT_OK) /* route is there */ 
     route_in_fib = PAL_TRUE;
  else
    shadow_entry = nsm_snmp_shadow_route_lookup (route_dest, route_mask,
                                                 next_hop, 1, 0);

  pal_mem_cpy(&intval,var_val,4);

  switch (vp->magic)
    {
      case NSM_IPCIDRROUTEIFINDEX:
        set_mask = set_mask | NSM_SET_IPCIDRTABLE_IFINDEX;
        ret = nsm_snmp_route_table_consistency_check (route_dest, route_mask,
                                                   next_hop, set_mask, intval,
                                                   route_in_fib);
        if (ret != RESULT_OK)
          return SNMP_ERR_BADVALUE;

        if (shadow_entry != NULL)
          {
            if (nsm_snmp_shadow_route_update (route_dest, route_mask, next_hop,
                                              set_mask, intval, shadow_entry) 
                                              != RESULT_OK)
              return SNMP_ERR_BADVALUE;
          }
        else 
          {
            if ((shadow_entry = nsm_snmp_shadow_route_create()) != NULL)
              { 
                shadow_entry->ipCidrRouteDest.s_addr = route_dest.s_addr;
                shadow_entry->ipCidrRouteMask.s_addr = route_mask.s_addr;
                shadow_entry->ipCidrRouteNextHop.s_addr = next_hop.s_addr;
                shadow_entry->ipCidrRouteIfIndex = intval; 

               if (nsm_snmp_shadow_route_list_add (shadow_entry) != RESULT_OK)
                 return SNMP_ERR_GENERR;
             } 
             else 
               return SNMP_ERR_BADVALUE;
          }
        break;

      case NSM_IPCIDRROUTEMETRIC1:
        set_mask = set_mask | NSM_SET_IPCIDRTABLE_METRIC;
        ret = nsm_snmp_route_table_consistency_check (route_dest, route_mask,
                                                    next_hop, set_mask, intval,
                                                    route_in_fib);
        if (ret != RESULT_OK)
          return SNMP_ERR_BADVALUE;

        if (shadow_entry != NULL)
          {
            if (nsm_snmp_shadow_route_update (route_dest, route_mask, next_hop,
                                              set_mask, intval, shadow_entry) != 
                                              RESULT_OK)
              return SNMP_ERR_BADVALUE;
          }
        else
          {
            if ((shadow_entry = nsm_snmp_shadow_route_create()) != NULL)
              {
                shadow_entry->ipCidrRouteDest.s_addr = route_dest.s_addr;
                shadow_entry->ipCidrRouteMask.s_addr = route_mask.s_addr;
                shadow_entry->ipCidrRouteNextHop.s_addr = next_hop.s_addr;
                shadow_entry->ipCidrRouteMetric1 = intval;

               if (nsm_snmp_shadow_route_list_add (shadow_entry) != RESULT_OK)
                 return SNMP_ERR_GENERR;
             }
             else
               return SNMP_ERR_BADVALUE;
          }

        break;
        
      case NSM_IPCIDRROUTETYPE:
        set_mask = set_mask | NSM_SET_IPCIDRTABLE_ROUTETYPE;
        ret = nsm_snmp_route_table_consistency_check (route_dest, route_mask,
                                                next_hop, set_mask, intval,
                                                route_in_fib);
        if (ret != RESULT_OK)
          return SNMP_ERR_BADVALUE;
          
        if (shadow_entry != NULL)
          {
            if (nsm_snmp_shadow_route_update (route_dest, route_mask, next_hop,
                                              set_mask, intval, shadow_entry)
                                              != RESULT_OK)
              return SNMP_ERR_BADVALUE;
          }
        else
          {
            if ((shadow_entry = nsm_snmp_shadow_route_create()) != NULL)
              {
                shadow_entry->ipCidrRouteDest.s_addr = route_dest.s_addr;
                shadow_entry->ipCidrRouteMask.s_addr = route_mask.s_addr;
                shadow_entry->ipCidrRouteNextHop.s_addr = next_hop.s_addr;
                shadow_entry->ipCidrRouteType = intval;

               if (nsm_snmp_shadow_route_list_add (shadow_entry) != RESULT_OK)
                 return SNMP_ERR_GENERR;
             }
             else
               return SNMP_ERR_BADVALUE;
          }

        break; 

      case NSM_IPCIDRROUTESTATUS:
        set_mask = set_mask | NSM_SET_IPCIDRTABLE_ROWSTATUS;

        ret = nsm_snmp_route_table_consistency_check (route_dest, route_mask,
                                                next_hop, set_mask, intval,
                                                route_in_fib);
        if (ret != RESULT_OK)
         return SNMP_ERR_BADVALUE;

        if (nsm_snmp_set_row_status (route_dest, route_mask, next_hop, set_mask,
                                     intval) != RESULT_OK)
          return SNMP_ERR_BADVALUE;

        break;

      default:
        pal_assert (0);
        return SNMP_ERR_GENERR;
        break;

    }

  return SNMP_ERR_NOERROR;
  
}

/* Add this route to the fib */
int
nsm_snmp_ip_route_set (struct pal_in4_addr route_dest,
                       struct pal_in4_addr route_mask, 
                       struct pal_in4_addr next_hop,
                       int route_metric, int route_type, 
                       int route_ifindex)
{
  vrf_id_t vrf_id = VRF_ID_UNSPEC;
  struct prefix_ipv4 p;
  struct nsm_master *nm = NULL;
  union nsm_nexthop gate;
  char *ifname = NULL;
  int snmp_route_type = ROUTE_TYPE_OTHER;
  int ret = 0;

  pal_mem_set (&p, 0, sizeof (struct prefix_ipv4));

  nm = nsm_master_lookup_by_id (nzg, 0);
  if (nm == NULL)
    return RESULT_ERROR;

  gate.ipv4 = next_hop;

  p.prefix = route_dest;
  p.family = AF_INET;
  p.prefixlen = ip_masklen (route_mask);

   /* Convert the ifindex to ifname */
   if (route_ifindex != 0)
     ifname = if_index2name (&nm->vr->ifm, route_ifindex);

  /* Convert the route type value from snmp to rib familiar value */
  if (route_type == NSM_SNMP_ROUTE_LOCAL) 
    snmp_route_type = ROUTE_TYPE_LOCAL; 
  else if (route_type == NSM_SNMP_ROUTE_REMOTE)
    snmp_route_type = ROUTE_TYPE_REMOTE;

  if ((!gate.ipv4.s_addr) && (!ifname))
    ret = nsm_ipv4_route_set (nm, vrf_id, &p, NULL, NULL_INTERFACE,
                              APN_DISTANCE_STATIC, route_metric,
                              snmp_route_type,
                              APN_TAG_DEFAULT, NULL);
  else if (gate.ipv4.s_addr)
    ret = nsm_ipv4_route_set (nm, vrf_id, &p, &gate, NULL,
          APN_DISTANCE_STATIC, route_metric, snmp_route_type,
                             APN_TAG_DEFAULT, NULL);
  else
    ret = nsm_ipv4_route_set (nm, vrf_id, &p, NULL, ifname,
                              APN_DISTANCE_STATIC, route_metric,
                              snmp_route_type,
                              APN_TAG_DEFAULT, NULL);
  
  if (ret != NSM_API_SET_SUCCESS)
    return RESULT_ERROR;
                    
  /* delete the snmp shadow route if we had one */
  nsm_snmp_shadow_route_delete (route_dest, route_mask, next_hop); 

  return RESULT_OK;
}

/* Procedure to delete this route from fib */
int
nsm_snmp_ip_route_unset (struct pal_in4_addr route_dest,
                         struct  pal_in4_addr route_mask,
                         struct pal_in4_addr next_hop,
                         int route_ifindex)
{
  struct nsm_master *nm = NULL;
  union nsm_nexthop gate;
  struct prefix_ipv4 p;
  vrf_id_t vrf_id = VRF_ID_UNSPEC;
  char *ifname = NULL;
  int ret;

  pal_mem_set (&p, 0, sizeof (struct prefix_ipv4));

  nm = nsm_master_lookup_by_id (nzg, 0);
  if (nm == NULL)
    return RESULT_ERROR;

  gate.ipv4 = next_hop;

   /* Convert the ifindex to ifname */
   if (route_ifindex != 0)
     ifname = if_index2name (&nm->vr->ifm, route_ifindex);

  p.prefix = route_dest;
  p.family = AF_INET;
  p.prefixlen = ip_masklen (route_mask);

  if ((!gate.ipv4.s_addr) && (!ifname))
     ret = nsm_ipv4_route_unset (nm, vrf_id, &p, NULL, NULL_INTERFACE,
                                 APN_DISTANCE_STATIC, APN_TAG_DEFAULT, NULL);
  else if (gate.ipv4.s_addr)
    ret = nsm_ipv4_route_unset (nm, vrf_id, &p, &gate, NULL, 
                                APN_DISTANCE_STATIC, APN_TAG_DEFAULT, NULL);
  else
    ret = nsm_ipv4_route_unset (nm, vrf_id, &p, NULL, ifname, 
                                APN_DISTANCE_STATIC, APN_TAG_DEFAULT, NULL);

  if (ret != NSM_API_SET_SUCCESS)
    {
      nsm_snmp_shadow_route_delete (route_dest, route_mask, next_hop);
      return RESULT_ERROR;
    }

  return RESULT_OK;

}

/* Consistency check for read-create objects of ipCidrTable */
int
nsm_snmp_route_table_consistency_check (struct pal_in4_addr route_dest,
                                        struct pal_in4_addr route_mask,
                                        struct pal_in4_addr next_hop,
                                        int mask, int value,
                                        bool_t fib_route)

{
  struct interface *ifp = NULL;
  struct listnode *node = NULL; 
  struct prefix p;
  struct apn_vr *vr = NULL;

  if ((route_dest.s_addr & route_mask.s_addr) != route_dest.s_addr) 
    return RESULT_ERROR;

  if (mask & NSM_SET_IPCIDRTABLE_ROUTETYPE)
  {
    if (fib_route)
      return RESULT_ERROR;

    if ((value <= NSM_SNMP_ROUTE_MIN) || (value >= NSM_SNMP_ROUTE_MAX))
      return RESULT_ERROR;

    if (value == NSM_SNMP_ROUTE_REMOTE)
      {
        pal_mem_set (&p, 0, sizeof (struct prefix));
        p.family = AF_INET;
        p.u.prefix4 = route_dest;
        p.prefixlen = ip_masklen (route_mask);

        /* Check if the user has set remote route type for a route which is
         * in the same subnet as one of our interface addresses.
         */
         LIST_LOOP (nzg->ifg.if_list, ifp, node)
           {
             if (if_match_ifc_ipv4_direct (ifp, &p))
               return RESULT_ERROR;
           }
      }
  }

  if (mask & NSM_SET_IPCIDRTABLE_IFINDEX)
    {
      if ((fib_route) || (value < 0))
        return RESULT_ERROR;

      vr = apn_vr_get_privileged (nzg);
      if (if_lookup_by_index (&vr->ifm, value) == NULL)
        return RESULT_ERROR;

      /* We shall not accept the ifindex if the next-hop specified by use
       * is not resolved on the specified ifindex */
      if (next_hop.s_addr && !nsm_gateway_ifindex_match (route_dest, 
                             route_mask, next_hop, value))
        return RESULT_ERROR;
    }

  if (mask & NSM_SET_IPCIDRTABLE_METRIC)
    {
      if ((fib_route) || (value < NSM_SNMP_DEFAULT_METRIC_VAL))
        return RESULT_ERROR;
    }

  if (mask & NSM_SET_IPCIDRTABLE_ROWSTATUS)
    if ((value <= NSM_SNMP_ROW_STATUS_MIN) || 
       (value >= NSM_SNMP_ROW_STATUS_MAX))
      return SNMP_ERR_BADVALUE;

  return RESULT_OK;

}

/* Procedure to Set the Row Status of a CIDR Route Table row */
int
nsm_snmp_set_row_status (struct pal_in4_addr route_dest,
                         struct pal_in4_addr route_mask,
                         struct pal_in4_addr next_hop,
                         int mask,
                         int RowStatus)
{
  struct cidr_route_entry *shadow_route_entry = NULL;
  struct nsm_ptree_node node;
  struct rib info;
  struct nexthop nhop;
  struct cidr_route_entry route_entry;
  bool_t route_in_fib = PAL_FALSE;

  pal_mem_set (&route_entry, 0, sizeof (struct cidr_route_entry));
  pal_mem_set (&node, 0, sizeof(struct nsm_ptree_node));
  pal_mem_set (&info, 0, sizeof(struct rib));
  pal_mem_set (&nhop, 0, sizeof(struct nexthop));

  if (nsm_snmp_cidr_route_get (route_dest, route_mask, next_hop, &node,
                               &info, &nhop, PAL_FALSE) == RESULT_OK)
    route_in_fib = PAL_TRUE; 
  else
    shadow_route_entry = nsm_snmp_shadow_route_lookup (route_dest, route_mask, 
                                                     next_hop, 1, 0);

  switch (RowStatus)
    {
    case NSM_SNMP_ROW_STATUS_ACTIVE:
      
      /* If this route entry is present in the snmp shadow table,
       * then add the route to fib using the parameters in snmp shadow table
       */
      if (shadow_route_entry != NULL)
        {
          switch (shadow_route_entry->ipCidrRouteStatus)
            {
            case NSM_SNMP_ROW_STATUS_NOTREADY:
              return RESULT_ERROR;
              break; 

            case NSM_SNMP_ROW_STATUS_NOTINSERVICE:
              if (nsm_snmp_ip_route_set (route_dest, route_mask, next_hop,
                           shadow_route_entry->ipCidrRouteMetric1,
                           shadow_route_entry->ipCidrRouteType,
                           shadow_route_entry->ipCidrRouteIfIndex) != RESULT_OK)
                       
                return RESULT_ERROR; 
              break;

            default:
              return RESULT_ERROR;
              break;
            }
        }
      else
        {
           if (route_in_fib)
             break;
           /* If the route entry is existing in fib this would mean
            * route is already active,then do nothing.
            * If it is not existing, return error.
            */
           return RESULT_ERROR; 
        }
      break;

    case NSM_SNMP_ROW_STATUS_CREATEANDGO:
      if (route_in_fib)
        break;

      if ((shadow_route_entry != NULL))
        return RESULT_ERROR;
      else
        {
          if (nsm_snmp_ip_route_set (route_dest, route_mask, next_hop,
                                     0, NSM_SNMP_ROUTE_OTHER,
                                     0) != RESULT_OK)
            return RESULT_ERROR;
        }
      break; 

    case NSM_SNMP_ROW_STATUS_CREATEANDWAIT:
      if ((shadow_route_entry != NULL) || (route_in_fib))
        return RESULT_OK;
      else 
        {
          if ((shadow_route_entry = nsm_snmp_shadow_route_create()) != NULL)
            {
              shadow_route_entry->ipCidrRouteDest.s_addr = route_dest.s_addr;
              shadow_route_entry->ipCidrRouteMask.s_addr = route_mask.s_addr;
              shadow_route_entry->ipCidrRouteNextHop.s_addr = next_hop.s_addr;
              if (nsm_snmp_shadow_route_list_add (shadow_route_entry) !=
                  RESULT_OK)
                return RESULT_ERROR;
            }
          else
            return RESULT_ERROR;
        }

      break;
      
    case NSM_SNMP_ROW_STATUS_NOTINSERVICE:
      if (shadow_route_entry != NULL) 
        {
          switch (shadow_route_entry->ipCidrRouteStatus) 
            {
              case NSM_SNMP_ROW_STATUS_NOTREADY:
                return RESULT_ERROR;
                break;

              case NSM_SNMP_ROW_STATUS_NOTINSERVICE:
                /* do nothing */
                break; 
            }
        }
      else
        {
          /* Check if the route is there in fib. If yes, then delete it 
           * from the fib routing table and add to shadow table. If route 
           * is not present in fib too, then add it to shadow table.
           */ 
          if (route_in_fib)
            {
              nsm_snmp_route_entry_update (&node, &info, &nhop, &route_entry);
              if ((shadow_route_entry = nsm_snmp_shadow_route_create()) != NULL)
                {
                  pal_mem_cpy (shadow_route_entry, &route_entry,
                          sizeof (struct cidr_route_entry));
                  shadow_route_entry->ipCidrRouteStatus =
                                  NSM_SNMP_ROW_STATUS_NOTINSERVICE; 
                  if (nsm_snmp_shadow_route_list_add (shadow_route_entry) != 
                     RESULT_OK)
                    return RESULT_ERROR;
                }
              else
                {
                  return RESULT_ERROR;
                }

               if (nsm_snmp_ip_route_unset (route_dest, route_mask, 
                             next_hop, 
                             route_entry.ipCidrRouteIfIndex) != RESULT_OK)
                 return RESULT_ERROR;
            } 
          else 
            {
              if ((shadow_route_entry = nsm_snmp_shadow_route_create()) != NULL)
                {
                  shadow_route_entry->ipCidrRouteDest.s_addr =
                                               route_dest.s_addr;
                  shadow_route_entry->ipCidrRouteMask.s_addr = 
                                               route_mask.s_addr;
                  shadow_route_entry->ipCidrRouteNextHop.s_addr = 
                                               next_hop.s_addr;
                  if (nsm_snmp_shadow_route_list_add (shadow_route_entry) != 
                     RESULT_OK)
                    return RESULT_ERROR;
                }
              else
                return RESULT_ERROR;
            }
        }
      break;

    case NSM_SNMP_ROW_STATUS_DESTROY:
      if (shadow_route_entry != NULL)
        {
          nsm_snmp_shadow_route_delete (route_dest, route_mask, next_hop);      
        } 
      else
        {
          if (route_in_fib)
            {
              if (nsm_snmp_ip_route_unset (route_dest, route_mask, next_hop,
                         route_entry.ipCidrRouteIfIndex) != RESULT_OK)
                return RESULT_ERROR;
            }
        }
      break;

     case NSM_SNMP_ROW_STATUS_NOTREADY:
       /* User is not allowed to set the Row Status to Not-Ready. */
       return RESULT_ERROR;
       break;

     default: 
       pal_assert (0);
       return RESULT_ERROR;
       break;
    }
      
  return RESULT_OK;
}

/* Find matching route entry in snmp shadow route table */
struct cidr_route_entry *
nsm_snmp_shadow_route_lookup (struct pal_in4_addr route_dest,
                              struct pal_in4_addr route_mask,
                              struct pal_in4_addr next_hop,
                              int exact, int index_mask)
{
  struct listnode *node;
  struct cidr_route_entry *shadow_entry =NULL;
  struct nsm_ptree_node rn;
  struct rib rib;
  struct nexthop nhop;
  int ret;
  bool_t found_entry = PAL_FALSE;

  pal_mem_set (&rn, 0, sizeof(struct nsm_ptree_node));
  pal_mem_set (&rib, 0, sizeof(struct rib));
  pal_mem_set (&nhop, 0, sizeof(struct nexthop));
  node = NULL;

  if (exact)
    {
      LIST_LOOP(snmp_shadow_route_table, shadow_entry, node)
        {
          if ((shadow_entry->ipCidrRouteDest.s_addr == route_dest.s_addr) &&
              (shadow_entry->ipCidrRouteMask.s_addr == route_mask.s_addr) &&
              (shadow_entry->ipCidrRouteNextHop.s_addr == next_hop.s_addr))
            {  
              found_entry = PAL_TRUE;
              break;
            }
        }
    }
  else 
    {
    /* Following logic is used to ascertain that this is the request for
     * first entry in table.
     * we are trying to look for an entry in shadow structure only after getting
     * to know that fib does not have any further entries.
     * Now, if we ascertain that the entry for this index is existing in 
     * then it would mean, that the current request which we have now, is
     * for the first entry in the shadow route list.
     */

      /* Find out if we have to return the first entry of shadow table */
      ret = nsm_snmp_cidr_route_get (route_dest, route_mask, next_hop,
                                     &rn, &rib, &nhop, PAL_TRUE);
      if ((ret == RESULT_OK) || (index_mask & CIDR_NO_INDEX))
        {
          shadow_entry = listnode_head (snmp_shadow_route_table);
          if (shadow_entry != NULL) 
            return shadow_entry;
          else
            return NULL;
        }

      LIST_LOOP(snmp_shadow_route_table, shadow_entry, node)
        {
          if (index_mask & CIDR_FULL_INDEX)
            {
              if ((shadow_entry->ipCidrRouteDest.s_addr == route_dest.s_addr) &&
                  (shadow_entry->ipCidrRouteMask.s_addr == route_mask.s_addr) &&
                  (shadow_entry->ipCidrRouteNextHop.s_addr == next_hop.s_addr))
                {
                  found_entry = PAL_TRUE;
                  break;
                }
            }
          else if (index_mask & CIDR_PARTIAL_INDEX)
            {
              if ((shadow_entry->ipCidrRouteDest.s_addr != 0) && 
                 (shadow_entry->ipCidrRouteMask.s_addr != 0))
                {
                  if ((shadow_entry->ipCidrRouteDest.s_addr == 
                     route_dest.s_addr) &&
                   (shadow_entry->ipCidrRouteMask.s_addr == route_mask.s_addr))
                    {
                      found_entry = PAL_TRUE;
                      break;
                    }
                }
              else 
                {
                  if (shadow_entry->ipCidrRouteDest.s_addr == route_dest.s_addr)
                    {
                      found_entry = PAL_TRUE;
                      break;
                    }
                }
            }
        }
    }

  if (found_entry)
    {
      if (exact)
        return shadow_entry;
      else
        {
          if ((node != NULL) && (NEXTNODE (node) != NULL))
            return (NEXTNODE (node))->data;
        }
    }

  return NULL;
}

struct cidr_route_entry *
nsm_snmp_shadow_route_create (void)
{
  struct cidr_route_entry *cidr_route_entry;

  cidr_route_entry = XCALLOC (MTYPE_NSM_SNMP_ROUTE_ENTRY,
                             sizeof (struct cidr_route_entry));

  if (cidr_route_entry == NULL)
    {
      zlog_err (nzg, "memory allocation failed for"
                    " nsm_snmp_shadow_route_create");
      return NULL;
    }

  cidr_route_entry->ipCidrRouteProto = NSM_SNMP_PROTO_NETMGMT;
  cidr_route_entry->ipCidrRouteMetric2 = -1;
  cidr_route_entry->ipCidrRouteMetric3 = -1;
  cidr_route_entry->ipCidrRouteMetric4 = -1;
  cidr_route_entry->ipCidrRouteMetric5 = -1;
  cidr_route_entry->ipCidrRouteType = NSM_SNMP_ROUTE_OTHER;
  cidr_route_entry->ipCidrRouteStatus = NSM_SNMP_ROW_STATUS_NOTINSERVICE;

  return cidr_route_entry;
}

int
nsm_snmp_shadow_route_list_add (struct cidr_route_entry *cidr_route_entry)
{
  struct listnode *new_node;

  /* Add the new entry to the table link list */
  new_node = listnode_add (snmp_shadow_route_table, (void *)cidr_route_entry);

  if (new_node == NULL)
    {
      zlog_err(nzg, "Cannot allocate memory for snmp cidr route entry\n");
      return RESULT_ERROR;
    }

  return RESULT_OK;

}

/* delete the route entry from snmp shadow route table */
void
nsm_snmp_shadow_route_delete (struct pal_in4_addr route_dest,
                              struct pal_in4_addr route_mask,
                              struct pal_in4_addr next_hop)
{
  struct cidr_route_entry *shadow_entry = NULL;

  /* Check if we have the route in snmp shadow route table */
  shadow_entry = nsm_snmp_shadow_route_lookup (route_dest, route_mask,
                                               next_hop, 1, 0);
 
  if (shadow_entry != NULL) 
    {
      listnode_delete (snmp_shadow_route_table, (void *)shadow_entry); 
      XFREE (MTYPE_NSM_SNMP_ROUTE_ENTRY, shadow_entry); 
    }

  return;
}

/* This procedure updates the shadow snmp route table. 
 */
int
nsm_snmp_shadow_route_update (struct pal_in4_addr route_dest,
                              struct pal_in4_addr route_mask,
                              struct pal_in4_addr next_hop,
                              int mask, int value,
                              struct cidr_route_entry *shadow_entry)
{
  
  if (shadow_entry->ipCidrRouteStatus != NSM_SNMP_ROW_STATUS_NOTINSERVICE)
    return RESULT_ERROR;

  if (mask & NSM_SET_IPCIDRTABLE_METRIC)
    shadow_entry->ipCidrRouteMetric1 = value;

  if (mask & NSM_SET_IPCIDRTABLE_ROUTETYPE)
    shadow_entry->ipCidrRouteType = value;

  if (mask & NSM_SET_IPCIDRTABLE_IFINDEX)
    shadow_entry->ipCidrRouteIfIndex = value;

  if (mask & NSM_SET_IPCIDRTABLE_ROWSTATUS)
    shadow_entry->ipCidrRouteStatus = value;

  return RESULT_OK;

}
#endif /* HAVE_L3 */

/* Traps */
void
nsm_ent_config_chg_trap ()
{
  u_int32_t trap_id_len = 0;
  u_int32_t param_oid_len = 0;
  u_int32_t inamelen = 0;
  u_int32_t trapobjlen = 0;
  static oid trap_id[] = {MIB2ENTITYGROUPOID, 2, 0, 1, 0};
  static oid param_oid[] = {MIB2ENTITYGROUPOID, 1, 4, 1};
  static oid iname[] = {MIB2ENTITYGROUPOID};
  struct trap_object2 *trapobj = NULL;

  /* Trap oid length */
  trap_id_len = sizeof (trap_id) / sizeof (oid);
  param_oid_len = sizeof (param_oid) / sizeof (oid);
  inamelen = sizeof (iname) / sizeof (oid);
  trapobjlen = 0;

  snmp_trap2 (nzg,
              trap_id, trap_id_len, 1,       
              param_oid, param_oid_len,
              trapobj, trapobjlen, pal_time_current (NULL));
}

/* Lookup procedure for entPhysicalTable */
struct entPhysicalEntry *
nsm_snmp_physical_entity_lookup (int index, int exact)
{
  struct nsm_master *nm = NULL;
  struct listnode *node  = NULL;
  struct entPhysicalEntry *entPhy = NULL;
  struct entPhysicalEntry *best = NULL;

  nm = nsm_master_lookup_by_id (nzg, 0);
  if (nm == NULL)
    return NULL;

  LIST_LOOP (nm->phyEntityList, entPhy, node) 
    {
      if (exact)
        {
          if (entPhy->entPhysicalIndex == index) 
            {
              best = entPhy; 
              break;
            }
        }
      else
        {
          if (index < entPhy->entPhysicalIndex)
            {
              /* save if this is a better candidate and
               *   continue search
               */
              if (best == NULL)
                best = entPhy;
              else if (entPhy->entPhysicalIndex < best->entPhysicalIndex)
                best = entPhy;
            }
        }
    }
  return best;
}

/* Write procedure for entPhysicalTable */
int
nsm_snmp_write_entPhysicalTable (int action, u_char * var_val,
                                 u_char var_val_type, size_t var_val_len,
                                 u_char * statP, oid * name, size_t length,
                                 struct variable *vp, u_int32_t vr_id)
{
  u_int32_t index = 0;
  struct entPhysicalEntry *entPhy = NULL;   
  struct nsm_master *nm = NULL;

  if (var_val_type != ASN_OCTET_STR)
    return SNMP_ERR_WRONGTYPE;

  if (nsm_snmp_index_get (vp, name, &length, &index, 1))
    return SNMP_ERR_GENERR;

  entPhy = nsm_snmp_physical_entity_lookup (index, 1);
  if (entPhy == NULL)
    return SNMP_ERR_GENERR; 

  switch (vp->magic)
    {
    case NSM_ENTITYPHYSERIALNO:
      pal_strcpy (entPhy->entPhysicalSerialNum, var_val);
      break;   

    case NSM_ENTITYPHYALIAS:
      pal_strcpy (entPhy->entPhysicalAlias, var_val);
      break;   

    case NSM_ENTITYPHYASSETID:
      pal_strcpy (entPhy->entPhysicalAssetID, var_val);
      break;   

    default:
      return SNMP_ERR_GENERR;
      break;
    }

  nm = nsm_master_lookup_by_id (nzg, 0);
  if (nm)
    nm->last_change_time = pal_time_current (NULL);
  nsm_ent_config_chg_trap ();

  return SNMP_ERR_NOERROR;
}

/* Callback procedure for entPhysicalTable */
unsigned char *
nsm_snmp_entPhysicalTable (struct variable *vp, oid *name,
                           size_t *length, int exact, size_t *var_len,
                           WriteMethod **write_method, u_int32_t vr_id)
{
  int ret;
  u_int32_t index = 0;
  struct entPhysicalEntry *entPhy = NULL;   

  /* Validate the index */
  ret = nsm_snmp_index_get (vp, name, length, &index, exact);
  if (ret < 0)
    return NULL;

  entPhy = nsm_snmp_physical_entity_lookup (index, exact);
  if (entPhy == NULL)
    return NULL; 

  if (!exact)
    nsm_snmp_index_set (vp, name, length, entPhy->entPhysicalIndex);

  switch (vp->magic)
    {
    case NSM_ENTITYPHYIDX: 
      break;

    case NSM_ENTITYPHYDESCR: 
      NSM_SNMP_RETURN_STRING (entPhy->entPhysicalDescr, 
                              pal_strlen (entPhy->entPhysicalDescr)); 
      break;

    case NSM_ENTITYPHYVENDORTYPE:
      NSM_SNMP_RETURN_OID (null_oid, NELEMENTS (null_oid));
      break; 

    case NSM_ENTITYPHYCONTAINEDIN:
      NSM_SNMP_RETURN_INTEGER (entPhy->entPhysicalContainedIn);
      break;
      
    case NSM_ENTITYPHYCLASS:
      NSM_SNMP_RETURN_INTEGER (entPhy->entPhysicalClass);
      break;

    case NSM_ENTITYPHYPARENTRELPOS:
      NSM_SNMP_RETURN_INTEGER (entPhy->entPhysicalParentRelPos);  
      break;

    case NSM_ENTITYPHYNAME:
      NSM_SNMP_RETURN_STRING (entPhy->entPhysicalName, 
                              pal_strlen (entPhy->entPhysicalName)); 
      break;

    case NSM_ENTITYPHYHWREV:
      NSM_SNMP_RETURN_STRING (&entPhy->entPhysicalHardwareRev, 
                              pal_strlen (entPhy->entPhysicalHardwareRev)); 
      break;

    case NSM_ENTITYPHYFWREV:
      NSM_SNMP_RETURN_STRING (&entPhy->entPhysicalFirmwareRev, 
                     pal_strlen (entPhy->entPhysicalFirmwareRev)); 
      break;

    case NSM_ENTITYPHYSWREV:
      NSM_SNMP_RETURN_STRING (&entPhy->entPhysicalSoftwareRev, 
                              pal_strlen (entPhy->entPhysicalSoftwareRev)); 
      break;

    case NSM_ENTITYPHYSERIALNO:
      *write_method = nsm_snmp_write_entPhysicalTable;
      NSM_SNMP_RETURN_STRING (&entPhy->entPhysicalSerialNum, 
                     pal_strlen (entPhy->entPhysicalSerialNum)); 
      break;

    case NSM_ENTITYPHYMFGNAME:
      NSM_SNMP_RETURN_STRING (&entPhy->entPhysicalMfgName, 
                     pal_strlen (entPhy->entPhysicalMfgName)); 
      break;

    case NSM_ENTITYPHYMODELNAME:
      NSM_SNMP_RETURN_STRING (&entPhy->entPhysicalModelName, 
                     pal_strlen (entPhy->entPhysicalModelName)); 
      break;

    case NSM_ENTITYPHYALIAS:
      *write_method = nsm_snmp_write_entPhysicalTable;
      NSM_SNMP_RETURN_STRING (&entPhy->entPhysicalAlias, 
                     pal_strlen (entPhy->entPhysicalAlias)); 
      break;

    case NSM_ENTITYPHYASSETID:
      *write_method = nsm_snmp_write_entPhysicalTable;
      NSM_SNMP_RETURN_STRING (&entPhy->entPhysicalAssetID, 
                     pal_strlen (entPhy->entPhysicalAssetID)); 
      break;

    case NSM_ENTITYPHYISFRU:
      NSM_SNMP_RETURN_INTEGER (entPhy->entPhysicalIsFRU); 
      break;

    default:
      break;

    }

  return NULL;
}

/* Lookup procedure for entLogicalTable */
struct entLogicalEntry *
nsm_snmp_logical_entity_lookup (u_int32_t *index, int exact, u_int32_t vr_id)
{
  struct entLogicalEntry *best = NULL;
  struct apn_vr *vr = NULL;
  int i;

  for (i = 1; i < vector_max (nzg->vr_vec); i ++)
    {
      vr = vector_slot (nzg->vr_vec, i);
      if (!vr || !(vr->entLogical))
        continue;

      if (exact)
        {
          if (vr->entLogical->entLogicalIndex == (*index))
            {
              best = vr->entLogical; 
              break;
            }
        }
      else
        {
          if (*index < vr->entLogical->entLogicalIndex)
            {
              /* save if this is a better candidate and
               *   continue search
               */
              if (best == NULL)
                best = vr->entLogical;
              else if (vr->entLogical->entLogicalIndex < best->entLogicalIndex)
                best = vr->entLogical;
            }
        }
    }

  if (!exact)
    {
      if (best)
        *index = best->entLogicalIndex;
      else
        *index = 0;
    }

  return best;
}

/* Callback procedure for entLogicalTable */
unsigned char *
nsm_snmp_entLogicalTable (struct variable *vp, oid *name,
                          size_t *length, int exact, size_t *var_len,
                          WriteMethod **write_method, u_int32_t vr_id)
{
  int ret = 0;
  u_int32_t index = 0; 
  struct entLogicalEntry *entLogical = NULL;

  /* Validate the index */
  ret = nsm_snmp_index_get (vp, name, length, &index, exact);
  if (ret < 0)
    return NULL;

  entLogical = nsm_snmp_logical_entity_lookup (&index, exact, vr_id);
  if (entLogical == NULL)
    return NULL; 

  if (!exact)
    nsm_snmp_index_set (vp, name, length, index);

  switch (vp->magic)
    {
    case NSM_ENTITYLOGIDX: 
      /* Not accessible */
      break;

    case NSM_ENTITYLOGDESCR: 
      NSM_SNMP_RETURN_STRING (entLogical->entLogicalDescr, 
                              pal_strlen (entLogical->entLogicalDescr)); 
      break;

    case NSM_ENTITYLOGTYPE:
      NSM_SNMP_RETURN_STRING (entLogical->entLogicalType, 
                              pal_strlen (entLogical->entLogicalType)); 
      break;

    case NSM_ENTITYLOGCOMMUNITY:
      NSM_SNMP_RETURN_STRING (entLogical->entLogicalCommunity, 
                              pal_strlen (entLogical->entLogicalCommunity)); 
      break;
      
    case NSM_ENTITYLOGTADDRESS:
      NSM_SNMP_RETURN_STRING (entLogical->entLogicalTAddress, 
                              pal_strlen (entLogical->entLogicalTAddress)); 
      break;

    case NSM_ENTITYLOGTDOMAIN:
      NSM_SNMP_RETURN_STRING (entLogical->entLogicalTDomain, 
                              pal_strlen (entLogical->entLogicalTDomain)); 
      break;

    case NSM_ENTITYLOGCONTEXTENGID:
      NSM_SNMP_RETURN_STRING (entLogical->entLogicalContextEngineId, 
                     pal_strlen (entLogical->entLogicalContextEngineId)); 
      break;

    case NSM_ENTITYLOGCONTEXTNAME:
      NSM_SNMP_RETURN_STRING (entLogical->entLogicalContextName, 
                              pal_strlen (entLogical->entLogicalContextName)); 
      break;

    }

  return NULL;
}

/* Utility procedure to get the 2 Integer Indices */
int
nsm_snmp_ent_2index_get (struct variable *v, oid *name, 
                         size_t *length, u_int32_t *index1,
                         u_int32_t *index2, int exact)
{
  int result;
  int len;

  result = oid_compare (name, v->namelen, v->name, v->namelen);

  if (exact)
    {
      /* Check the length. */
      if (result != 0 || *length - v->namelen != 2)
        return -1;

      /* Fill up the values for both the indices */
      *index1 = name[v->namelen];
      *index2 = name[v->namelen + 1];
      return 0;
    }
  else
    {
      /* The user might have specified all the index value while doing a
       * GET-NEXT, or may be just few of them.
       */
      /* return -1 if greater than our our table entry */
      if (result > 0)
        return -1;
      else if (result == 0)
        {
          /* use index value if present, otherwise 0. */
          len = *length - v->namelen;
          if (len >= 1)
            {
              *index1 = name[v->namelen];
              *index2 = name[v->namelen + 1];
              return 0;
            }
          else
            {
              /* set the user's oid to be ours */
              pal_mem_cpy (name, v->name, ((int) v->namelen) * sizeof (oid));
              return 0;
            }
        }
    }

  return -1;
}

void
nsm_snmp_ent_2index_set (struct variable *v, oid *name,
                         size_t *length, u_int32_t index1,
                         u_int32_t index2)
{
  name[v->namelen] = index1;
  name[v->namelen + 1] = index2;
  *length = v->namelen + 2;
}

/* Lookup procedure for entLPMappingTable */
struct entPhysicalEntry *
nsm_snmp_entLPMapTable_lookup (u_int32_t *log_index, u_int32_t *phy_index, 
                               int exact)
{
  struct apn_vr *vr = NULL;
  struct apn_vr *bestvr = NULL;
  struct listnode *node = NULL;
  struct entPhysicalEntry *entPhy = NULL;
  struct entPhysicalEntry *bestPhy = NULL;
  struct entPhysicalEntry *best = NULL;
  int i;

  for (i = 1; i < vector_max (nzg->vr_vec); i ++)
    {
      vr = vector_slot (nzg->vr_vec, i);
      if (!vr)
        continue;

      if (exact)
        {
          if ((vr->id == (*log_index)))
            {
              LIST_LOOP (vr->mappedPhyEntList, entPhy, node) 
                {
                  if (entPhy->entPhysicalIndex == (*phy_index))
                    {
                      best = entPhy;
                      bestvr = vr; 
                      break;
                    }
                }
            }
        }
      else
        {
          if (*log_index == vr->id)
            {
              LIST_LOOP (vr->mappedPhyEntList, entPhy, node) 
                {
                  if (*phy_index < entPhy->entPhysicalIndex)
                    {
                      if (bestPhy)
                        {
                          if (entPhy->entPhysicalIndex < 
                              bestPhy->entPhysicalIndex)
                            bestPhy = entPhy;
                        }
                      else
                        bestPhy = entPhy;
                    }
                }
              if (bestPhy)
                {
                  *log_index = vr->id;
                  *phy_index = bestPhy->entPhysicalIndex;
                  return bestPhy;
                }
            }

          if (*log_index < vr->id)
            {
              if (bestvr)
                if (vr->id > bestvr->id)
                  continue;

              LIST_LOOP (vr->mappedPhyEntList, entPhy, node) 
                {
                  if (best)
                    {
                      if (entPhy->entPhysicalIndex < best->entPhysicalIndex)
                        {
                          best = entPhy;
                          bestvr = vr;
                        }
                    }
                  else
                    {
                      best = entPhy;
                      bestvr = vr;
                    }
                }  
            }
        }
    }

  if (!exact)
    {
      if (best)
        {
          *log_index = bestvr->id;
          *phy_index = best->entPhysicalIndex;
        }
      else
        {
          *log_index = 0;
          *phy_index = 0;
        }
    }

  return best;
}

/* Callback procedure for entLPMappingTable */
unsigned char *
nsm_snmp_entLPMapTable (struct variable *vp, oid *name,
                        size_t *length, int exact, size_t *var_len,
                        WriteMethod **write_method, u_int32_t vr_id)
{
  int ret = 0;
  struct entPhysicalEntry *entPhy = NULL;
  u_int32_t logIndex = 0; 
  u_int32_t phyIndex = 0; 

  /* Validate the index */
  ret = nsm_snmp_ent_2index_get (vp, name, length, &logIndex, 
                                 &phyIndex, exact);
  if (ret < 0)
    return NULL;

  entPhy = nsm_snmp_entLPMapTable_lookup (&logIndex, &phyIndex, exact);
  if (entPhy == NULL)
    return NULL; 

  if (!exact)
    nsm_snmp_ent_2index_set (vp, name, length, logIndex, phyIndex);

  switch (vp->magic)
    {
    case NSM_ENTITYLPPHYSICALIDX: 
      NSM_SNMP_RETURN_INTEGER (entPhy->entPhysicalIndex);
      break;
    }

  return NULL;
}

/* Lookup procedure for entAliasMappingTable */
struct entPhysicalEntry *
nsm_snmp_entAliasMapTable_lookup (u_int32_t *phy_index, u_int32_t *log_index, 
                                  int exact)
{
  struct apn_vr *vr = NULL;
  struct apn_vr *bestvr = NULL;
  struct listnode *node = NULL;
  struct listnode *phynode = NULL;
  struct entPhysicalEntry *entPhy = NULL;
  struct entPhysicalEntry *bestPhy = NULL;
  struct entPhysicalEntry *best = NULL;
  struct entPhysicalEntry *entLPEntry = NULL;
  struct nsm_master *nm = nsm_master_lookup_by_id (nzg, 0);
  int match_found = 0;
  int i;

  if (nm)
    {
      LIST_LOOP (nm->phyEntityList, entPhy, phynode)
        {
          if (entPhy->entPhysicalClass != CLASS_PORT)
            continue;
          if (exact)
            {
              if ((entPhy->entPhysicalIndex == (*phy_index)))
                {
                  for (i = 0; i < vector_max (nzg->vr_vec); i ++)
                    {
                      vr = vector_slot (nzg->vr_vec, i);
                      if (!vr)
                        continue;

                      if (vr->id)
                        {
                          LIST_LOOP (vr->mappedPhyEntList, entLPEntry, node)
                            if (entLPEntry->entPhysicalIndex == 
                                entPhy->entPhysicalIndex) 
                              match_found = 1;
               
                          if (! match_found) 
                            continue;
                        }

                      if (vr->id == (*log_index))
                        {
                          best = entPhy;
                          bestvr = vr; 
                          break;
                        }
                    }
                }
              if (best)
                break;    
            }

          else
            {
              if (*phy_index == entPhy->entPhysicalIndex)
                {
                  for (i = 0; i < vector_max (nzg->vr_vec); i ++)
                    {
                      vr = vector_slot (nzg->vr_vec, i);
                      if (!vr)
                        continue;
 
                      LIST_LOOP (vr->mappedPhyEntList, entLPEntry, node)
                        if (entLPEntry->entPhysicalIndex == 
                            entPhy->entPhysicalIndex) 
                          {
                             if (*log_index == vr->id && 
                                 *phy_index == entPhy->entPhysicalIndex)
                               continue;

                             match_found = 1;
                             *phy_index = entPhy->entPhysicalIndex;
                             *log_index = vr->id;
                             return entPhy;
                          }

                       if (vr->id)
                         continue;

                      if (*log_index < vr->id)
                        {
                          if (bestPhy)
                            {
                              if (vr->id < bestvr->id)
                                {
                                  bestPhy = entPhy;
                                  bestvr = vr;
                                }
                            }
                          else
                            {
                              bestPhy = entPhy;
                              bestvr = vr;
                            }
                        }
                    }
                  if (bestPhy)
                    {
                      *phy_index = bestPhy->entPhysicalIndex;
                      *log_index = bestvr->id;
                      return bestPhy;
                    }
                }

              else if (*phy_index < entPhy->entPhysicalIndex)
                {
                  if (best)
                    if (entPhy->entPhysicalIndex > best->entPhysicalIndex)
                      continue;

                  for (i = 0; i < vector_max (nzg->vr_vec); i ++)
                    {
                      vr = vector_slot (nzg->vr_vec, i);
                      if (!vr)
                        continue;
                  
                      LIST_LOOP (vr->mappedPhyEntList, entLPEntry, node)
                        if (entLPEntry->entPhysicalIndex == 
                            entPhy->entPhysicalIndex) 
                          {
                            match_found = 1;
                            *phy_index = entPhy->entPhysicalIndex;
                            *log_index = vr->id;
                            return entPhy;
                          }

                      if (vr->id)
                         continue;
  
                      if (bestvr)
                        {
                          if (vr->id < bestvr->id)
                            {
                              best = entPhy;
                              bestvr = vr;
                            }
                        }
                      else
                        {
                          best = entPhy;
                          bestvr = vr;
                        }
                    }  
                }
            }
        }
    }
  if (!exact)
    {
      if (best)
        {
          *phy_index = best->entPhysicalIndex;
          *log_index = bestvr->id;
        }
      else
        {
          *phy_index = 0;
          *log_index = 0;
        }
    }

  return best;
}

/* Callback procedure for entAliasMappingTable */
unsigned char *
nsm_snmp_entAliasMapTable (struct variable *vp, oid *name,
                           size_t *length, int exact, size_t *var_len,
                           WriteMethod **write_method, u_int32_t vr_id)
{
  int ret = 0;
  struct entPhysicalEntry *entPhy = NULL;
  u_int32_t logIndex = 0; 
  u_int32_t phyIndex = 0; 
  struct interface *ifp = NULL;
  struct apn_vr *vr = NULL;

  nsm_snmp_char_ptr[0] = '\0';
  /* Validate the index */
  ret = nsm_snmp_ent_2index_get (vp, name, length, &phyIndex, 
                                 &logIndex, exact);
  if (ret < 0)
    return NULL;

  entPhy = nsm_snmp_entAliasMapTable_lookup (&phyIndex, &logIndex, exact);
  if (entPhy == NULL)
    return NULL; 

  vr = apn_vr_lookup_by_id (nzg, logIndex);
  if (vr == NULL)
    return NULL;
 
  ifp = if_lookup_by_name (&vr->ifm, entPhy->entPhysicalName);

  if (ifp == NULL)
    return NULL;

  if (!exact)
    nsm_snmp_ent_2index_set (vp, name, length, phyIndex, logIndex);

  switch (vp->magic)
    {
      case NSM_ENTITYALIASIDXORZERO: 
        break;
      case NSM_ENTITYALIASMAPID: 
        pal_snprintf (nsm_snmp_char_ptr,
                      NSM_MAX_STRING_LENGTH,
                      "ifIndex.%d", ifp->ifindex);
        NSM_SNMP_RETURN_STRING (nsm_snmp_char_ptr, 
                                pal_strlen(nsm_snmp_char_ptr));
      break;
    }

  return NULL;
}

/* Lookup procedure for entPhysicalContainsTable */
struct entPhysicalEntry *
nsm_snmp_entPhyContains_lookup (u_int32_t *phy_idx, u_int32_t *phy_child_idx, 
                                int exact)
{
  struct entPhysicalEntry *entPhy, *entPhyEntry, *best;
  struct listnode *node, *phy_node;
  struct nsm_master *nm = NULL;
 
  entPhy = NULL;
  entPhyEntry = NULL;
  best = NULL;
  node = NULL;
  phy_node = NULL;
 
  nm = nsm_master_lookup_by_id (nzg, 0);
  if (nm == NULL)
    return NULL;

  if (exact)
    {
      LIST_LOOP (nm->phyEntityList, entPhy, node)
        {
          if (entPhy->entPhysicalContainedIn == *phy_idx &&
              entPhy->entPhysicalIndex == *phy_child_idx)
            return entPhy;
        }
    }
  else
    {
      LIST_LOOP (nm->phyEntityList, entPhyEntry, phy_node)
        {
          if (*phy_idx > entPhyEntry->entPhysicalIndex)
            continue;

          LIST_LOOP (nm->phyEntityList, entPhy, node)
            {
              if (best == NULL)
                {
                  if (*phy_child_idx < entPhy->entPhysicalIndex && 
                      entPhy->entPhysicalContainedIn &&
                      (entPhyEntry->entPhysicalIndex == 
                       entPhy->entPhysicalContainedIn))
                    best = entPhy;
                }
              else 
                {
                  if (best->entPhysicalIndex > entPhy->entPhysicalIndex &&
                      (entPhyEntry->entPhysicalIndex == 
                       entPhy->entPhysicalContainedIn))
                    best = entPhy;
                }
            }
          if (best)
            {
              *phy_idx = entPhyEntry->entPhysicalIndex;
              *phy_child_idx = best->entPhysicalIndex;
              break;
            }
          else
           *phy_child_idx = 0;
        }
    }
  if (best == NULL)
    {
      *phy_idx = 0;
      *phy_child_idx = 0;
    }
    
  return best;
}

/* Callback procedure for entPhyContTable */
unsigned char *
nsm_snmp_entPhyContTable (struct variable *vp, oid *name,
                          size_t *length, int exact, size_t *var_len,
                          WriteMethod **write_method, u_int32_t vr_id)
{
  int ret = 0;
  u_int32_t phyIndex = 0; 
  u_int32_t phyChildIndex = 0; 
  struct entPhysicalEntry *entPhy = NULL;

  /* Validate the index */
  ret = nsm_snmp_ent_2index_get (vp, name, length, 
                                          &phyIndex, &phyChildIndex, exact);
  if (ret < 0)
    return NULL;

  entPhy = nsm_snmp_entPhyContains_lookup (&phyIndex, &phyChildIndex, exact);
  if (entPhy == NULL)
    return NULL; 

  if (!exact)
    nsm_snmp_ent_2index_set (vp, name, length, phyIndex, phyChildIndex);

  switch (vp->magic)
    {
    case NSM_ENTITYPHYCHILDIDX: 
      NSM_SNMP_RETURN_INTEGER (phyChildIndex);
      break;
    }

  return NULL;
}

/* Callback procedure for Scalars */
unsigned char *
nsm_snmp_entScalar (struct variable *vp, oid *name,
                    size_t *length, int exact, size_t *var_len,
                    WriteMethod **write_method, u_int32_t vr_id)
{
  struct nsm_master *nm = NULL;

  /* Check whether the instance identifier is valid */
  if (snmp_header_generic (vp, name, length, exact, var_len, write_method)
      == MATCH_FAILED)
    return NULL;

  nm = nsm_master_lookup_by_id (nzg, 0);
  if (nm == NULL)
    return NULL;

  switch (vp->magic)
    {
    case NSM_ENTITYLASTCHANGE: 
      NSM_SNMP_RETURN_TIMETICKS (nm->last_change_time);
      break;
    }

  return NULL;
}

/* Traps */
 
void
nsm_snmp_trap (u_int32_t ifindex,
               u_int32_t admin_status,
               u_int32_t oper_status)
{
  oid link_down_trap_oid[] = {NSMTRAPMIB, 3};
  oid link_up_trap_oid[] = {NSMTRAPMIB, 4};
  struct trap_object2 obj[3];
  int trapoidlen;
  int namelen;
  oid *ptr;

  trapoidlen = sizeof link_up_trap_oid / sizeof (oid);
  namelen = sizeof mib2if_oid / sizeof (oid);

  ptr = obj[0].name;
  OID_COPY (ptr, mib2if_oid, namelen);
  OID_SET_ARG3 (ptr, 2, 1, 1);

  OID_SET_VAL (obj[0], ptr - obj[0].name, INTEGER,
               sizeof (int), &ifindex);

  ptr = obj[1].name;
  OID_COPY (ptr, mib2if_oid, namelen);
  OID_SET_ARG3 (ptr, 2, 1 ,7);

  OID_SET_VAL (obj[1], ptr - obj[1].name, INTEGER,
               sizeof (int), &admin_status);

  ptr = obj[2].name;
  OID_COPY (ptr, mib2if_oid, namelen);
  OID_SET_ARG3 (ptr, 2, 1, 8);

  OID_SET_VAL (obj[2], ptr - obj[2].name, INTEGER,
               sizeof (int), &oper_status);

  snmp_trap2 (nzg,
              (oper_status == NSM_SNMP_INTERFACE_UP)
              ? link_up_trap_oid : link_down_trap_oid,
              trapoidlen, oper_status,
              mib2if_oid, sizeof mib2if_oid / sizeof (oid),
              obj, sizeof obj / sizeof (struct trap_object2),
              pal_time_current (NULL) );
}

/* SNMP init procedure for NSM */
void
nsm_snmp_init (struct lib_globals *zg)
{
  /* register ourselves with the agent to handle our mib tree objects */
  snmp_init (zg, nsmd_oid, sizeof(nsmd_oid) / sizeof(oid));
  REGISTER_MIB (zg, "mibII/system", mib2sys_variables, variable,
                mib2sys_oid);
  REGISTER_MIB (zg, "mibII/interface", mib2if_variables, variable,
                mib2if_oid);
  REGISTER_MIB (zg, "mibII/interfaceX", mib2ifX_variables, variable,
                mib2ifX_oid);
  REGISTER_MIB (zg, "mibII/ip", mib2ip_variables, variable, mib2ip_oid);
  REGISTER_MIB (zg, "mibII/Entity", mib2entity_variables, variable,
                mib2entity_oid);

#ifdef HAVE_MPLS
  if (NSM_CAP_HAVE_MPLS)
    {
      nsm_mpls_snmp_init ();
#ifdef HAVE_MPLS_VC
      nsm_pw_snmp_init ();
#endif /* HAVE_MPLS_VC */
    }
#endif /* HAVE_MPLS */

#if defined HAVE_MCAST_IPV4 || defined HAVE_MCAST_IPV6
  nsm_mcast_snmp_init (zg);
#endif /* HAVE_MCAST_IPV4 || HAVE_MCAST_IPV6 */

#ifdef HAVE_L2
  /* SNMP init. */
  xstp_snmp_init (zg);
  xstp_p_snmp_init (zg);

#if defined (HAVE_I_BEB) || defined (HAVE_B_BEB)
  /* SNMP Initialization for PBB OID's */
  pbb_ah_snmp_init(zg);
#endif /* HAVE_I_BEB || HAVE_B_BEB */

#if defined HAVE_I_BEB && defined HAVE_B_BEB && defined HAVE_PBB_TE
  /* SNMP Initialization for PBB-TE OID's */
  init_ieee8021PbbTeMib (zg);
#endif /* HAVE_I_BEB && HAVE_B_BEB && HAVE_PBB_TE */
    
#ifdef HAVE_VLAN
  xstp_q_snmp_init (zg);
#endif /* HAVE_VLAN */
#endif /*(HAVE_L2) */

  if (NSM_ZG->snmp.t_connect == NULL && NSM_ZG->snmp.t_read == NULL)
     snmp_start (NSM_ZG);

  snmp_shadow_init (zg);

  return;
}
#endif /* HAVE_SNMP */
