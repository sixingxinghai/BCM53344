/* Copyright (C) 2008  All Rights Reserved */

#ifndef NSM_PBB_MIB_H
#define NSM_PBB_MIB_H

#ifdef HAVE_VLAN
#include "nsm_bridge.h"
#include "nsm_vlan.h"
#endif/* HAVE_VLAN */

#define PBB_ICOMP_ID_MIN     1
#define PBB_ICOMP_ID_MAX     32
#define SET_VAL_SIZE         4
#define SET_MAC_SIZE         6
#define PBB_BVID_MIN         2
#define PBB_BVID_MAX         4094
#define PBB_ISID_MIN         1
#define PBB_ISID_MAX         16777214
#define PBB_MIN_VAL          0 
#define PBB_VIP_PORT_MIN     0
#define PBB_VIP_PORT_MAX     4096

/*
 *  Edge Bridge OID's
 */
enum pbb_edge_bridge_oid
{
   IEEE8021PBBBACKBONEEDGEBRIDGEADDRESS = 1,
   IEEE8021PBBBACKBONEEDGEBRIDGENAME,
   IEEE8021PBBNUMBEROFICOMPONENTS,     
   IEEE8021PBBNUMBEROFBCOMPONENTS,
   IEEE8021PBBNUMBEROFBEBPORTS,
   IEEE8021PBBBEBNAME
};

/*
 * VIP OID's
 */
enum pbb_vip_oid
{
   IEEE8021PBBVIPPAPNFINDEX = 7,
   IEEE8021PBBVAPNSID,
   IEEE8021PBBVIPDEFAULTDSTBMAC,
   IEEE8021PBBVIPTYPE,
   IEEE8021PBBVIPROWSTATUS
};

/*
 * I-SID to VIP Cross reference OID's
 */
enum pbb_isid_vip_xref_oid
{
   IEEE8021PBBISIDTOVAPNSID = 12,           
   IEEE8021PBBISIDTOVIPCOMPONENTID,
   IEEE8021PBBISIDTOVIPPORT
};

/*
 * PIP OID's
 */
enum pbb_pip_oid
{
   IEEE8021PBBPAPNFINDEX = 15,
   IEEE8021PBBPIPBMACADDRESS,
   IEEE8021PBBPIPNAME,        
   IEEE8021PBBPAPNCOMPONENTID,
   IEEE8021PBBPIPVIPMAP
};

/*
 *  VIP-PIP-Mapping OID's
 */
enum pbb_vip_pip_map_oid
{
   IEEE8021PBBVIPTOPIPMAPPINGPAPNFINDEX = 20,  
   IEEE8021PBBVIPTOPIPMAPPINGBMACADDRESS,
   IEEE8021PBBVIPTOPIPMAPPINGSTORAGETYPE,
   IEEE8021PBBVIPTOPIPMAPPINGROWSTATUS
};

/*
 * Service Instance OID's
 */
enum pbb_service_instance_oid
{
   IEEE8021PBBCBPSERVICEMAPPINGBACKBONESID = 24,         
   IEEE8021PBBCBPSERVICEMAPPINGBVID,                
   IEEE8021PBBCBPSERVICEMAPPINGDEFAULTBACKBONEDEST, 
   IEEE8021PBBCBPSERVICEMAPPINGTYPE,                
   IEEE8021PBBCBPSERVICEMAPPINGLOCALSID
};

/*
 * CNP OID's
 */
enum pbb_cnp_service_oid
{
   IEEE8021PBBCNPSERVICEINTERFACETYPE = 29,
   IEEE8021PBBCNPSERVIREFPORTNUMBER,
   IEEE8021PBBCNPSERVICESVIDLOW,
   IEEE8021PBBCNPSERVICESVIDHIGH,
   IEEE8021PBBCNPSERVICEPORTTYPE,
   IEEE8021PBBCNPROWSTATUS
};

/*
 * function declarations
 */
void
pbb_ah_snmp_init (struct lib_globals *);

int
pbb_snmp_integer_index3_get (struct variable *, oid *, size_t *, u_int32_t *,
                             u_int32_t *, u_int32_t *, int);
#if 0
void
l2_snmp_integer_index3_set (struct variable *, oid *, size_t *, u_int32_t,
                            u_int32_t, u_int32_t);
#endif /* 0 */
int
pbb_snmp_vip_entry_add (struct avl_tree *, struct avl_tree *,  u_int32_t,
                        u_int32_t);

int
pbb_snmp_sid_vip_entry_add (struct avl_tree *, u_int32_t, u_int32_t, u_int32_t);

int
pbb_snmp_cnp_entry_add (struct avl_tree *,u_int32_t, u_int32_t, u_int32_t);

struct vip_tbl*
pbb_snmp_vip_lookup_by_portnum (struct avl_tree *, u_int32_t, int);

struct sid2vip_xref*
pbb_snmp_sid_vip_lookup_by_isid (struct avl_tree *, u_int32_t , int);

struct pip_tbl*
pbb_snmp_pip_lookup_by_ifindex (struct list *, u_int32_t , int);

struct vip2pip_map*
pbb_snmp_vip_pip_lookup_by_portnum (struct avl_tree *, u_int32_t, int);

struct cnp_srv_tbl*
pbb_snmp_cnp_lookup_by_isid (struct avl_tree *, u_int32_t, int);

struct cbp_srvinst_tbl*
pbb_snmp_cbp_lookup_by_isid (struct avl_tree *, u_int32_t, int);

FindVarMethod  pbb_snmp_ahBridge_scalars;
FindVarMethod  pbb_snmp_ahVip_table;
FindVarMethod  pbb_snmp_ahIsid_Vip;
FindVarMethod  pbb_snmp_ahPip_table;
FindVarMethod  pbb_snmp_ahVip_Pip_map;
FindVarMethod  pbb_snmp_ahService_table;
FindVarMethod  pbb_snmp_ahCnp_table;

WriteMethod    pbb_snmp_write_bridge_name;
WriteMethod    pbb_snmp_write_vip_table;
WriteMethod    pbb_snmp_write_vip_Rowstatus;
WriteMethod    pbb_snmp_write_pip_table;
WriteMethod    pbb_snmp_write_Vip_Pip_map;
WriteMethod    pbb_snmp_write_Vip_Pip_Rowstatus;
WriteMethod    pbb_snmp_write_Service_table;
WriteMethod    pbb_snmp_write_Cnp_table;
WriteMethod    pbb_snmp_write_Cnp_Rowstatus;

#endif/* NSM_PBB_MIB_H*/

