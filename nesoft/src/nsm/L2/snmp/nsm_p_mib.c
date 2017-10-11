/* Copyright (C) 2001-2004  All Rights Reserved. */

#include "pal.h"
#include "lib.h"

#ifdef HAVE_SNMP

#include <asn1.h>
#include "if.h"
#include "log.h"
#include "prefix.h"
#include "table.h"
#include "avl_tree.h"
#include "snmp.h"
#include "l2_snmp.h"
#include "pal_math.h"
#include "nsmd.h"
#include "nsm_bridge.h"
#include "nsm_interface.h"
#include "nsm_snmp.h"
#include "nsm_snmp_common.h"
#include "nsm_L2_snmp.h"
#include "nsm_p_mib.h"
#include "nsm_l2_qos.h"

#ifdef HAVE_HAL
#include "hal_incl.h"
#endif /* HAVE_HAL */

#ifdef HAVE_GMRP
#include "gmrp_api.h"
#endif

#ifdef HAVE_GVRP
#include "gvrp_api.h"
#endif

#ifdef HAVE_HA
#include "nsm_bridge_cal.h"
#endif /* HAVE_HA */

/*
 * dot1dBridge_variables_oid:
 *   this is the top level oid that we want to register under.  This
 *   is essentially a prefix, with the suffix appearing in the
 *   variable below.
 */

oid             dot1dBridge_variables_oid[] = { 1, 3, 6, 1, 2, 1, 17 };

#define TABLE_SIZE 100
#define VALUE 0
#define VAR 0

/*
  extended filtering services 0 - true
  traffic classes             1 - true
  staticEntryIndividualPort   2 - ??
  IVLCapable                  3 - true
  SVLCapable                  4 - true
  HybridCapable               5 - false
  configurablepvid tagging    6 - ??
*/


#define dot1dDeviceCapabilities 0x5B
#define dot1dPortCapabilities 0x03
#define SNMP_ERR_UNKNOWN -1

oid pxstp_oid [] = { BRIDGEMIBOID };
oid pxstp_nsm_oid [] = { BRIDGEMIBOID , 6 };
int
write_dot1dPortPriorityEntryTable(int action,
                                  u_char * var_val,
                                  u_char var_val_type,
                                  size_t var_val_len,
                                  u_char * statP,
                                  oid * name, size_t name_len,
                                  struct variable *v, u_int32_t vr_id);

int
write_dot1dPortGarpTime(int action,
                        u_char * var_val,
                        u_char var_val_type,
                        size_t var_val_len,
                        u_char * statP, oid * name, size_t name_len,
                        struct variable *v, u_int32_t vr_id);

int
write_dot1dPortGmrpStatus(int action,
                          u_char * var_val,
                          u_char var_val_type,
                          size_t var_val_len,
                          u_char * statP, oid * name, size_t name_len,
                          struct variable *v, u_int32_t vr_id);

int
write_dot1dRegenUserPriority(int action,
                             u_char * var_val,
                             u_char var_val_type,
                             size_t var_val_len,
                             u_char * statP, oid * name, size_t name_len,
                             struct variable *v, u_int32_t vr_id);

int
write_dot1dPortRestrictedGroup(int action,
                               u_char * var_val,
                               u_char var_val_type,
                               size_t var_val_len,
                               u_char * statP, oid * name, size_t name_len,
                               struct variable *v, u_int32_t vr_id);


#if defined (HAVE_QOS) && defined (HAVE_VLAN)
struct interface *
nsm_snmp_bridge_port_traffic_class_indexes_lookup (struct nsm_bridge *br,
                                                   int port_id, int exact,
                                                   u_int32_t traffic_priority,
                                                   u_int32_t *priority);
#endif

struct variable xstp_p_dot1dBridge_variables[] = {
  /*
   * magic number        , variable type , ro/rw , callback fn  , L, oidsuffix
   */

  /* pBridgeMIB (6) */
  /* pBridgeMIBObjects (6,1) */
  /* dot1dExtBase (6,1,1) */
  {DOT1DDEVICECAPABILITIES,         ASN_OCTET_STR,  RONLY,  xstp_snmp_pBridge_scalars,
   3, {1,1,1}},
  {DOT1DTRAFFICCLASSESENABLED,      ASN_INTEGER,    RWRITE, xstp_snmp_pBridge_scalars,
   3, {1,1,2}},
#ifdef HAVE_GMRP
  {DOT1DGMRPSTATUS,                 ASN_INTEGER,    RWRITE, xstp_snmp_pBridge_scalars,
   3, {1,1,3}},
#endif

  /* dot1dPortCapabilitiesTable (6,1,1,4) */
  /* dot1dPortCapabilitiesEntry (6,1,1,4,1) */
  {DOT1DPORTCAPABILITIES,           ASN_OCTET_STR,  RONLY,  xstp_snmp_dot1dPortCapabilitiesTable,
   5, {1,1,4,1,1}},

  /* dot1dPriority (6,1,2) */
  /* dot1dPortPriorityTable (6,1,2,1) */
  /* dot1dPortPriorityEntry (6,1,2,1,1) */
  {DOT1DPORTDEFAULTUSERPRIORITY,    ASN_INTEGER,    RWRITE, xstp_snmp_dot1dPortPriorityTable,
   5, {1,2,1,1,1}},
  {DOT1DPORTNUMTRAFFICCLASSES,      ASN_INTEGER,    RWRITE, xstp_snmp_dot1dPortPriorityTable,
   5, {1,2,1,1,2}},


  /* dot1dUserPriorityRegenTable (6,1,2,2) */
  /* dot1dUserPriorityRegenEntry (6,1,2,2,1) */
  {DOT1DUSERPRIORITY,               ASN_INTEGER,    RONLY,  xstp_snmp_dot1dUserPriorityRegenTable,
   5, {1,2,2,1,1}},
  {DOT1DREGENUSERPRIORITY,          ASN_INTEGER,    RWRITE, xstp_snmp_dot1dUserPriorityRegenTable,
   5, {1,2,2,1,2}},

#ifdef HAVE_QOS
  /* dot1dTrafficClassTable (6,1,2,3) */
  /* dot1dTrafficClassEntry (6,1,2,3,1) */
  {DOT1DTRAFFICCLASSPRIORITY,       ASN_INTEGER,    RONLY,  xstp_snmp_dot1dTrafficClassTable,
   5, {1,2,3,1,1}},
  {DOT1DTRAFFICCLASS,               ASN_INTEGER,    RWRITE, xstp_snmp_dot1dTrafficClassTable,
   5, {1,2,3,1,2}},

#endif /* HAVE_QOS */
  /* dot1dPortOutBoundAccessPriorityTable (6,1,2,4) */
  /* dot1dPortOutBoundAccessPriorityEntry (6,1,2,4,1) */
  {DOT1DPORTOUTBOUNDACCESSPRIORITY, ASN_INTEGER,    RONLY, xstp_snmp_dot1dPortOutboundAccessPriorityTable,
   5, {1,2,4,1,1}},
#if defined (HAVE_GMRP) || defined (HAVE_GVRP)
  /* dot1dGarp (6,1,3) */
  /* dot1dPortGarpTable (6,1,3,1) */
  /* dot1dPortGarpEntry (6,1,3,1,1) */
  {DOT1DPORTGARPJOINTIME,           ASN_INTEGER,    RWRITE, xstp_snmp_dot1dPortGarpTable,
   5, {1,3,1,1,1}},
  {DOT1DPORTGARPLEAVETIME,          ASN_INTEGER,    RWRITE, xstp_snmp_dot1dPortGarpTable,
   5, {1,3,1,1,2}},
  {DOT1DPORTGARPLEAVEALLTIME,       ASN_INTEGER,    RWRITE, xstp_snmp_dot1dPortGarpTable,
   5, {1,3,1,1,3}},
#endif

#ifdef HAVE_GMRP
  /* dot1dGmrp (6,1,4) */
  /* dot1dPortGmrpTable (6,1,4,1) */
  /* dot1dPortGmrpEntry (6,1,4,1,1) */
  {DOT1DPORTGMRPSTATUS,               ASN_INTEGER,    RWRITE, xstp_snmp_dot1dPortGmrpTable,
   5, {1,4,1,1,1}},
  {DOT1DPORTGMRPFAILEDREGISTRATIONS,  ASN_COUNTER,    RONLY,  xstp_snmp_dot1dPortGmrpTable,
   5, {1,4,1,1,2}},
  {DOT1DPORTGMRPLASTPDUORIGIN,        ASN_OCTET_STR,  RONLY,  xstp_snmp_dot1dPortGmrpTable,
   5, {1,4,1,1,3}},
  {DOT1QPORTRESTRICTEDGROUPREGISTRATION,ASN_INTEGER,  RWRITE, xstp_snmp_dot1dPortGmrpTable,
   5,{1,4,1,1,4}},

#endif

};

static int xstp_int_val;

/* Register the BRIDGE-MIB.
 */

void
xstp_p_snmp_init( struct lib_globals *zg )
{

  /*
   * register ourselves with the agent to handle our mib tree objects
   */
  REGISTER_MIB(zg, "pBridgeMIB", xstp_p_dot1dBridge_variables, variable, pxstp_nsm_oid);
}

/*
 * xstp_snmp_pBridge_scalars():
 *   This function is called every time the agent gets a request for
 *   a scalar variable that might be found within the pBridgeMIB tree or qBridgeMIB tree
 */
unsigned char  *
xstp_snmp_pBridge_scalars(struct variable *vp,
                          oid * name,
                          size_t * length,
                          int exact, size_t * var_len, 
                          WriteMethod ** write_method,
                          u_int32_t vr_id)
{

  struct nsm_bridge *br;
#if defined HAVE_GMRP || defined HAVE_QOS
  int retStatus = 0;
#endif /* HAVE_GMRP || HAVE_QOS */

  if (snmp_header_generic(vp, name, length, exact, var_len, write_method)
      == MATCH_FAILED)
    return NULL;

  br = nsm_snmp_get_first_bridge();

  if (br == NULL)
    {
      return NULL;
    }
  /*
   * this is where we do the value assignments for the mib results.
   */
  switch (vp->magic) {

    /* pBridgeMIB */
  case DOT1DDEVICECAPABILITIES:
    XSTP_SNMP_RETURN_INTEGER (dot1dDeviceCapabilities);

#ifdef HAVE_QOS
  case DOT1DTRAFFICCLASSESENABLED:
    *write_method = xstp_snmp_write_dot1dTrafficClassesEnabled;
    retStatus = br->traffic_class_enabled;
    XSTP_SNMP_RETURN_INTEGER (retStatus);
    break;
#endif /* HAVE_QOS */

#ifdef HAVE_GMRP
  case DOT1DGMRPSTATUS:
    *write_method = xstp_snmp_write_dot1dGmrpStatus;
    retStatus = br->gmrp_bridge ? DOT1DPORTGMRPSTATUS_ENABLED :
      DOT1DPORTGMRPSTATUS_DISABLED;
    XSTP_SNMP_RETURN_INTEGER (retStatus);
    break;
#endif

  default:
    pal_assert (0);
    break;
  }
  return NULL;

}

/*
 * xstp_snmp_dot1dPortOutboundAccessPriorityTable():
 *   Handle this table separately from the scalar value case.
 *   The workings of this are basically the same as for xstp_snmp_ above.
 */
unsigned char  *
xstp_snmp_dot1dPortOutboundAccessPriorityTable(struct variable *vp,
                                               oid * name,
                                               size_t * length,
                                               int exact,
                                               size_t * var_len,
                                               WriteMethod ** write_method,
                                               u_int32_t vr_id)
{
  /*
   * variables we may use later  TODO
   */

  if (snmp_header_generic
      (vp, name, length, exact, var_len, write_method)
      == MATCH_FAILED)
    return NULL;

  /*
   * this is where we do the value assignments for the mib results.
   */
  switch (vp->magic) {
  case DOT1DPORTOUTBOUNDACCESSPRIORITY:
    XSTP_SNMP_RETURN_INTEGER (0);
    break;
  default:
    pal_assert (0);
    break;
  }
  return NULL;
}

/*
 * xstp_snmp_dot1dPortCapabilitiesTable():
 *   Handle this table separately from the scalar value case.
 *   The workings of this are basically the same as for xstp_snmp_ above.
 */
unsigned char  *
xstp_snmp_dot1dPortCapabilitiesTable(struct variable *vp,
                                     oid * name,
                                     size_t * length,
                                     int exact,
                                     size_t * var_len,
                                     WriteMethod ** write_method,
                                     u_int32_t vr_id)
{
  int ret;
  u_int32_t index = 0;
  struct nsm_bridge *br;
  struct interface *ifp;
  struct nsm_bridge_port *br_port;

  br_port = NULL;

  /* validate the index */
  ret = l2_snmp_index_get (vp, name, length, &index, exact);

  if (ret < 0)
    return NULL;

  br = nsm_snmp_get_first_bridge ();

  if (br == NULL)
    return NULL;

  ifp = nsm_snmp_bridge_port_lookup_by_port_id (br, index, exact, &br_port);

  if (ifp == NULL || br_port == NULL)
    return NULL;

  if (!exact)
    {
      l2_snmp_index_set (vp, name, length, br_port->port_id);
    }

  /*
   * this is where we do the value assignments for the mib results.
   */
  switch (vp->magic) {
  case DOT1DPORTCAPABILITIES:
    XSTP_SNMP_RETURN_INTEGER (dot1dPortCapabilities);
    break;

  default:
    pal_assert (0);
    break;
  }
  return NULL;
}

/*
 * xstp_snmp_dot1dPortPriorityTable():
 *   Handle this table separately from the scalar value case.
 *   The workings of this are basically the same as for xstp_snmp_ above.
 */
unsigned char  *
xstp_snmp_dot1dPortPriorityTable(struct variable *vp,
                                 oid * name,
                                 size_t * length,
                                 int exact,
                                 size_t * var_len, 
                                 WriteMethod ** write_method,
                                 u_int32_t vr_id)
{
#ifdef HAVE_QOS
  struct nsm_bridge *br;
  struct interface *ifp;
  int ret;
  u_int32_t index = 0;
  int user_priority;
  struct nsm_bridge_port *br_port;
  
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

  if (!exact)
    {
      l2_snmp_index_set (vp, name, length, br_port->port_id);
    }

  switch (vp->magic) {
  case DOT1DPORTDEFAULTUSERPRIORITY:
    *write_method = write_dot1dPortPriorityEntryTable;
#ifdef HAVE_VLAN
    user_priority =  nsm_vlan_port_get_default_user_priority (ifp);
    if (user_priority < 0)
      break;
#endif /* HAVE_VLAN */
    XSTP_SNMP_RETURN_INTEGER (user_priority);
    break;

  case DOT1DPORTNUMTRAFFICCLASSES:
    *write_method = write_dot1dPortPriorityEntryTable;
    XSTP_SNMP_RETURN_INTEGER(br_port->num_cosq);
    break;

  default:
    pal_assert (0);
    break;
  }

#endif /* HAVE_QOS */

  return NULL;
}

/*
 * xstp_snmp_dot1dUserPriorityRegenTable():
 *   Handle this table separately from the scalar value case.
 *   The workings of this are basically the same as for xstp_snmp_ above.
 */
unsigned char  *
xstp_snmp_dot1dUserPriorityRegenTable(struct variable *vp,
                                      oid * name,
                                      size_t * length,
                                      int exact,
                                      size_t * var_len,
                                      WriteMethod ** write_method,
                                      u_int32_t vr_id)
{
#ifdef HAVE_QOS
  struct nsm_if *zif = NULL;
  struct nsm_bridge *br = NULL;
  struct nsm_bridge_port *br_port = NULL;
  struct interface *ifp = NULL;
  struct nsm_vlan_port *vlan_port = NULL;
  int ret;
  u_int32_t index = 0;
  int regen_user_priority;
  u_int32_t default_user_priority = 0;
  u_int32_t priority = 0;

  br_port = NULL;

  /* validate the index */
  ret = l2_snmp_integer_index_get (vp, name, length, &index , &default_user_priority, exact);
  if (ret < 0)
    return NULL;

  br = nsm_snmp_get_first_bridge();

  if (br == NULL)
    return NULL;

  ifp = nsm_snmp_bridge_port_traffic_class_indexes_lookup (br, index, exact,
                                                           default_user_priority,
                                                           &priority);

  if (ifp == NULL)
    return NULL;

  zif = (struct nsm_if *)ifp->info;

  br_port = zif->switchport;
#ifdef HAVE_VLAN
  vlan_port = &br_port->vlan_port;

  if (!exact)
    {
      l2_snmp_integer_index_set (vp, name, length, br_port->port_id,
                                 priority);
    }
 
  regen_user_priority = nsm_vlan_port_get_regen_user_priority (ifp,priority);
#endif /* HAVE_VLAN */
  /*
   * this is where we do the value assignments for the mib results.
   */
  switch (vp->magic) {
    case DOT1DUSERPRIORITY:
      break;
  
    case DOT1DREGENUSERPRIORITY:
      *write_method = write_dot1dRegenUserPriority;
      XSTP_SNMP_RETURN_INTEGER (regen_user_priority);
      break;
  default:
    pal_assert (0);
    break;
  }

#endif /* HAVE_QOS */
  return NULL;
}

#if defined (HAVE_GMRP) || defined (HAVE_GVRP)
/*
 * xstp_snmp_dot1dPortGarpTable():
 *   Handle this table separately from the scalar value case.
 *   The workings of this are basically the same as for xstp_snmp_ above.
 */
unsigned char  *
xstp_snmp_dot1dPortGarpTable(struct variable *vp,
                             oid * name,
                             size_t * length,
                             int exact,
                             size_t * var_len, WriteMethod ** write_method,
                             u_int32_t vr_id)
{
  struct nsm_bridge *br;
  struct interface *ifp;
  struct nsm_bridge_port *br_port;
  struct apn_vr *vr = apn_vr_get_privileged (snmpg);
  struct nsm_master *master = NULL;
  struct nsm_bridge_master *bridge_master = NULL;
  pal_time_t timer_value = 0;
  u_int32_t index = 0;
  int ret;

  br_port = NULL;

  if (! vr)
    return NULL;

  master = vr->proto;

  if (! master)
    return NULL;

  bridge_master = master->bridge;

  if (! bridge_master)
    return NULL; 

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

  if (!exact)
    {
      l2_snmp_index_set (vp, name, length, br_port->port_id);
    }

  switch (vp->magic) {
  case DOT1DPORTGARPJOINTIME:
    *write_method = write_dot1dPortGarpTime;
#ifdef HAVE_GMRP
    gmrp_get_timer (bridge_master, ifp, GARP_JOIN_TIMER, &timer_value);
#endif
#ifdef HAVE_GVRP
    gvrp_get_timer (bridge_master, ifp, GARP_JOIN_TIMER, &timer_value);
#endif
    XSTP_SNMP_RETURN_INTEGER (timer_value);
    break;

  case DOT1DPORTGARPLEAVETIME:
    *write_method = write_dot1dPortGarpTime;
#ifdef HAVE_GMRP
    gmrp_get_timer (bridge_master, ifp, GARP_LEAVE_TIMER, &timer_value);
#endif
#ifdef HAVE_GVRP
    gvrp_get_timer (bridge_master, ifp, GARP_LEAVE_TIMER, &timer_value);
#endif
    XSTP_SNMP_RETURN_INTEGER (timer_value);
    break;

  case DOT1DPORTGARPLEAVEALLTIME:
    *write_method = write_dot1dPortGarpTime;
#ifdef HAVE_GMRP
    gmrp_get_timer (bridge_master, ifp, GARP_LEAVE_ALL_TIMER, &timer_value);
#endif
#ifdef HAVE_GVRP
    gvrp_get_timer (bridge_master, ifp, GARP_LEAVE_ALL_TIMER, &timer_value);
#endif
    XSTP_SNMP_RETURN_INTEGER (timer_value);
    break;

  default:
    pal_assert (0);
    break;
  }
  return NULL;
}
#endif

#ifdef HAVE_GMRP
/*
 * xstp_snmp_dot1dPortGmrpTable():
 *   Handle this table separately from the scalar value case.
 *   The workings of this are basically the same as for xstp_snmp_ above.
 */
unsigned char  *
xstp_snmp_dot1dPortGmrpTable(struct variable *vp,
                             oid * name, size_t * length,
                             int exact, size_t * var_len,
                             WriteMethod ** write_method, u_int32_t vr_id)
{
  int ret;
  int retStatus;
  struct nsm_if *zif;
  u_int32_t index = 0;
  struct interface *ifp;
  struct nsm_bridge *br;
  struct gmrp_bridge *gmrp_bridge;
  struct nsm_bridge_port *br_port;
  struct gmrp_port *gmrp_port = NULL;
  u_int8_t null_mac [ETHER_ADDR_LEN];

  br_port = NULL;

  /* validate the index */
  ret = l2_snmp_index_get (vp, name, length, &index, exact);

  if (ret < 0)
    return NULL;

  br = nsm_snmp_get_first_bridge();

  if (br == NULL)
    return NULL;

  pal_mem_set (null_mac, 0, ETHER_ADDR_LEN);

  gmrp_bridge = br->gmrp_bridge; /* probably can make this common */

  if (gmrp_bridge == NULL)
    return NULL;

  ifp = nsm_snmp_bridge_port_lookup_by_port_id (br, index, exact, &br_port);

  if (ifp == NULL || br_port == NULL)
    return NULL;

  zif = (struct nsm_if *)ifp->info; 

  if (zif == NULL || zif->switchport == NULL)
    return NULL;

  br_port = zif->switchport;

  gmrp_port = br_port->gmrp_port;

  if (!exact)
    {
      l2_snmp_index_set (vp, name, length, br_port->port_id);
    }

  switch (vp->magic) {

  case DOT1DPORTGMRPSTATUS:
    *write_method = write_dot1dPortGmrpStatus;
    retStatus = br_port->gmrp_port_list ? DOT1DPORTGMRPSTATUS_ENABLED :
      DOT1DPORTGMRPSTATUS_DISABLED;
    XSTP_SNMP_RETURN_INTEGER (retStatus);
    break;

  case DOT1DPORTGMRPFAILEDREGISTRATIONS:
    if (gmrp_port != NULL)
      XSTP_SNMP_RETURN_INTEGER (gmrp_port->gmrp_failed_registrations);
    else
      XSTP_SNMP_RETURN_INTEGER (0);
    break;

  case DOT1DPORTGMRPLASTPDUORIGIN:
    if (gmrp_port != NULL)
      {
         if (ifp->ifindex == gmrp_port->gid_port->ifindex)
        XSTP_SNMP_RETURN_MACADDRESS (ifp->hw_addr);
      }
    else
      XSTP_SNMP_RETURN_MACADDRESS (null_mac);
    break;

 case DOT1QPORTRESTRICTEDGROUPREGISTRATION:
    *write_method = write_dot1dPortRestrictedGroup;
    if (gmrp_port != NULL
        && gmrp_port->registration_cfg == GID_REG_MGMT_RESTRICTED_GROUP)
      XSTP_SNMP_RETURN_INTEGER(DOT1DPORTRESTRICTEDGROUP_ENABLED);
    else
      XSTP_SNMP_RETURN_INTEGER(DOT1DPORTRESTRICTEDGROUP_DISABLED);
    break;

  default:
    pal_assert (0);
    break;
  }
  return NULL;
}
#endif   /* HAVE_GMRP */

#if defined (HAVE_QOS) && defined (HAVE_VLAN)
struct interface *
nsm_snmp_bridge_port_traffic_class_indexes_lookup (struct nsm_bridge *br,
                                                   int port_id, int exact,
                                                   u_int32_t traffic_priority,
                                                   u_int32_t *priority)
{
  struct interface *ifp;
  struct nsm_bridge_port *br_port;
  
  if (exact)
  {
    ifp = nsm_snmp_bridge_port_lookup_by_port_id (br, port_id, 1, &br_port);

    if (ifp == NULL || br_port == NULL)
      return NULL;

    *priority = traffic_priority;
    return ifp;
  }
  else
  {
    ifp = nsm_snmp_bridge_port_lookup_by_port_id (br, port_id, 1, &br_port);
    
    if ((ifp == NULL) || (br_port == NULL))
    {
      ifp = nsm_snmp_bridge_port_lookup_by_port_id (br, port_id, exact,
                                                     &br_port);
      if ((ifp == NULL) || (br_port == NULL))
         return NULL;
    }
    
    if (traffic_priority < TRAFFIC_CLASS_VALUE_MAX)
    {
      if (port_id != 0)
         *priority = (traffic_priority + 1);
    }
    else
    {
      ifp = nsm_snmp_bridge_port_lookup_by_port_id (br, port_id, exact,
                                          &br_port);
      *priority = TRAFFIC_CLASS_VALUE_MIN;
    }
   
    return ifp;
  }
}

/*
 * xstp_snmp_dot1dTrafficClassTable():
 *   Handle this table separately from the scalar value case.
 *   The workings of this are basically the same as for xstp_snmp_ above.
 */
unsigned char  *
xstp_snmp_dot1dTrafficClassTable(struct variable *vp,
                                 oid * name,
                                 size_t * length,
                                 int exact,
                                 size_t * var_len, WriteMethod ** write_method,
                                 u_int32_t vr_id)
{
#ifdef HAVE_QOS

  struct nsm_bridge *br = NULL;
  struct interface *ifp = NULL;
  struct nsm_vlan_port *vlan_port = NULL;
  struct nsm_if *zif = NULL;
  struct nsm_bridge_port *br_port = NULL;
  int ret;
  u_int32_t index = 0;
  u_int32_t traffic_class = 0;
  u_int32_t traffic_priority =0;
  unsigned char traffic_class_value = 0;
  unsigned char traffic_num_classes = 0;

  br_port = NULL;

  /* validate the index */
  ret = l2_snmp_integer_index_get (vp, name, length, &index,&traffic_priority, exact);
  if (ret < 0)
    return NULL;

  br = nsm_snmp_get_first_bridge();

  if (br == NULL)
    return NULL;

  ifp = nsm_snmp_bridge_port_lookup_by_port_id (br, index, exact, &br_port);

  if (ifp == NULL || br_port == NULL)
    return NULL;

  zif = (struct nsm_if *)ifp->info;

  br_port = zif->switchport;

  vlan_port = &br_port->vlan_port;

  if (!exact)
    {
      l2_snmp_integer_index_set (vp, name, length, br_port->port_id,
                                 vlan_port->default_user_priority);
    }

  traffic_class = nsm_vlan_port_get_traffic_class_table
                                    (ifp->ifindex,
                                     vlan_port->default_user_priority,
                                     traffic_num_classes,
                                     traffic_class_value);
                                       
                                     
 switch (vp->magic) {
   case DOT1DTRAFFICCLASSPRIORITY:
    break;
  case DOT1DTRAFFICCLASS:
    *write_method = xstp_snmp_write_dot1dTrafficClass;
    XSTP_SNMP_RETURN_INTEGER (traffic_class);
    break;
  default:
    pal_assert (0);
    break;
  }

#endif /* HAVE_QOS */
  return NULL;
}


int
xstp_snmp_write_dot1dTrafficClassesEnabled(int action,
                                           u_char * var_val,
                                           u_char var_val_type,
                                           size_t var_val_len,
                                           u_char * statP,
                                           oid * name, size_t name_len, 
                                           struct variable *v,
                                           u_int32_t vr_id)
{

#ifdef HAVE_QOS

  long intval;
  int index;
  int ret;
  struct nsm_bridge *br = nsm_snmp_get_first_bridge();

  if (br == NULL)
    return SNMP_ERR_GENERR;

  if (var_val_type != ASN_INTEGER)
    return SNMP_ERR_WRONGTYPE;

  if (var_val_len != sizeof (long))
    return SNMP_ERR_WRONGLENGTH;

  pal_mem_cpy(&intval,var_val,4);

  if ((intval < DOT1DTRAFFICCLASS_ENABLED)||
      (intval > DOT1DTRAFFICCLASS_DISABLED))
    return SNMP_ERR_BADVALUE;

  index = name[name_len];
  br->traffic_class_enabled = intval;

#ifdef HAVE_HA
  nsm_bridge_cal_modify_nsm_bridge (br);
#endif /* HAVE_HA */

  ret = nsm_bridge_set_traffic_class (br, br->traffic_class_enabled);

  if (ret < 0)
    return SNMP_ERR_UNKNOWN;

#endif /* HAVE_QOS */

  return SNMP_ERR_NOERROR;
}
#endif /* HAVE_QOS && HAVE_VLAN */

#ifdef HAVE_GMRP
int
xstp_snmp_write_dot1dGmrpStatus(int action,
                                u_char * var_val,
                                u_char var_val_type,
                                size_t var_val_len,
                                u_char * statP, oid * name, size_t name_len,
                                struct variable *v, u_int32_t vr_id)
{

  long intval;
  struct apn_vr *vr = apn_vr_get_privileged (snmpg);
  struct nsm_master *master = NULL;
  struct nsm_bridge_master *bridge_master = NULL;
  struct nsm_bridge *br = nsm_snmp_get_first_bridge();

  if (br == NULL)
    return SNMP_ERR_GENERR;

  if (! vr)
    return SNMP_ERR_UNKNOWN;

  master = vr->proto;

  if (! master)
    return SNMP_ERR_UNKNOWN;

  bridge_master = master->bridge;

  if (! bridge_master)
    return SNMP_ERR_UNKNOWN;

  if (var_val_type != ASN_INTEGER)
    return SNMP_ERR_WRONGTYPE;

  if (var_val_len != sizeof (long))
    return SNMP_ERR_WRONGLENGTH;

  pal_mem_cpy(&intval,var_val,4);

  if ((intval < DOT1DPORTGMRPSTATUS_ENABLED) || (intval > DOT1DPORTGMRPSTATUS_DISABLED))
    return SNMP_ERR_BADVALUE;

  if (intval == DOT1DPORTGMRPSTATUS_ENABLED)
    {
      if (gmrp_enable (bridge_master, "gmrp", br->name) != RESULT_OK)
        return SNMP_ERR_UNKNOWN;
    }
  else
    {
      if (gmrp_disable (bridge_master, br->name) != RESULT_OK)
        return SNMP_ERR_UNKNOWN;
    }

  return SNMP_ERR_NOERROR;
}
#endif


#ifdef HAVE_GMRP
int
write_dot1dPortGmrpStatus(int action,
                          u_char * var_val,
                          u_char var_val_type,
                          size_t var_val_len,
                          u_char * statP, oid * name, size_t name_len,
                          struct variable *v, u_int32_t vr_id)
{
  long intval;
  int index;
  struct apn_vr *vr = apn_vr_get_privileged (snmpg);
  struct nsm_master *master = NULL;
  struct nsm_bridge_port *br_port;
  struct nsm_bridge_master *bridge_master = NULL;
  struct nsm_bridge *br = nsm_snmp_get_first_bridge();
  struct interface *ifp = NULL;

  br_port = NULL;

 if (! vr)
    return SNMP_ERR_UNKNOWN;

  master = vr->proto;

  if (! master)
    return SNMP_ERR_UNKNOWN;

  bridge_master = master->bridge;

  if (! bridge_master)
    return SNMP_ERR_UNKNOWN;

  if (br == NULL)
    return SNMP_ERR_UNKNOWN;

  if (var_val_type != ASN_INTEGER)
    return SNMP_ERR_WRONGTYPE;

  if (var_val_len != sizeof (long))
    return SNMP_ERR_WRONGLENGTH;

  pal_mem_cpy(&intval,var_val,4);

  if ((intval < DOT1DPORTGMRPSTATUS_ENABLED) || (intval > DOT1DPORTGMRPSTATUS_DISABLED))
    return SNMP_ERR_BADVALUE;

  if ( l2_snmp_index_get (v, name, &name_len, (u_int32_t *) &index, 1) < 0 )
    return SNMP_ERR_GENERR;

  ifp = nsm_snmp_bridge_port_lookup_by_port_id (br, index, 1, &br_port);

  if (ifp == NULL || br_port == NULL)
    return SNMP_ERR_UNKNOWN;

  if (intval == DOT1DPORTGMRPSTATUS_ENABLED)
    {
      if (gmrp_enable_port (bridge_master, ifp) != RESULT_OK)
        return SNMP_ERR_UNKNOWN;
    }
  else
    {
      if (gmrp_disable_port (bridge_master, ifp) != RESULT_OK)
        return SNMP_ERR_UNKNOWN;
    }

  return SNMP_ERR_NOERROR;
}

int
write_dot1dPortRestrictedGroup(int action,
                               u_char * var_val,
                               u_char var_val_type,
                               size_t var_val_len,
                               u_char * statP, oid * name, size_t name_len,
                               struct variable *v, u_int32_t vr_id)
{
  long intval;
  int index;
  struct apn_vr *vr = apn_vr_get_privileged (snmpg);
  struct nsm_master *master = NULL;
  struct nsm_bridge_master *bridge_master = NULL;
  struct nsm_bridge *br = nsm_snmp_get_first_bridge();
  struct nsm_bridge_port *br_port;
  struct interface *ifp = NULL;

  br_port = NULL;

  if (br == NULL)
    return SNMP_ERR_GENERR;

  if (! vr)
    return SNMP_ERR_UNKNOWN;

  master = vr->proto;

  if (! master)
    return SNMP_ERR_UNKNOWN;

  bridge_master = master->bridge;

  if (! bridge_master)
    return SNMP_ERR_UNKNOWN;


  if (var_val_type != ASN_INTEGER)
    return SNMP_ERR_WRONGTYPE;

  if (var_val_len != sizeof (long))
    return SNMP_ERR_WRONGLENGTH;

  pal_mem_cpy(&intval,var_val,4);

  if ((intval < DOT1DPORTRESTRICTEDGROUP_ENABLED) || (intval > DOT1DPORTRESTRICTEDGROUP_DISABLED))
    return SNMP_ERR_BADVALUE;
  if ( l2_snmp_index_get (v, name, &name_len, (u_int32_t *) &index, 1) < 0 )
    return SNMP_ERR_GENERR;

  ifp = nsm_snmp_bridge_port_lookup_by_port_id (br, index, 1, &br_port);

  if (ifp == NULL || br_port == NULL)
    return SNMP_ERR_UNKNOWN;

  if (intval == DOT1DPORTRESTRICTEDGROUP_ENABLED)
    {
     if((gmrp_set_registration (br->master, ifp, GID_EVENT_RESTRICTED_GROUP_REGISTRATION))!=RESULT_OK)
         return SNMP_ERR_UNKNOWN;
    }
  else
    {
     if(( gmrp_set_registration (br->master, ifp, GID_EVENT_FIXED_REGISTRATION))!= RESULT_OK)
         return SNMP_ERR_UNKNOWN;
    }

  return SNMP_ERR_NOERROR;
}
#endif /* HAVE_GMRP */

int
write_dot1dRegenUserPriority(int action,
                             u_char * var_val,
                             u_char var_val_type,
                             size_t var_val_len,
                             u_char * statP, oid * name, size_t name_len,
                             struct variable *v, u_int32_t vr_id)
{

#ifdef HAVE_QOS

  long intval;
  u_int32_t recvd_user_priority,index;
  int ret;
  int exact = 1;
  struct nsm_bridge *br = nsm_snmp_get_first_bridge();
  struct interface *ifp = NULL ;  
  struct nsm_bridge_port *br_port;

  br_port = NULL;

  if (br == NULL)
    return SNMP_ERR_GENERR;

  if (var_val_type != ASN_INTEGER)
    return SNMP_ERR_WRONGTYPE;

  if (var_val_len != sizeof (long))
    return SNMP_ERR_WRONGLENGTH;

  pal_mem_cpy(&intval,var_val,4);

  if (intval < 0 || intval > 7)
    return SNMP_ERR_BADVALUE;

  /* validate the index */
  ret = l2_snmp_integer_index_get (v, name, &name_len, &index , 
                                   &recvd_user_priority, exact);
  if (ret < 0)
    return SNMP_ERR_GENERR;

 
  ifp = nsm_snmp_bridge_port_lookup_by_port_id (br, index, exact, &br_port);

  if (ifp == NULL || br_port == NULL)
    return SNMP_ERR_GENERR;

  nsm_vlan_port_set_regen_user_priority (ifp, 
                                         recvd_user_priority, intval);
#endif /* HAVE_QOS */
                                                                   
  return SNMP_ERR_NOERROR;
}

int
xstp_snmp_write_dot1dTrafficClass(int action,
                        u_char * var_val,
                        u_char var_val_type,
                        size_t var_val_len,
                        u_char * statP, oid * name, size_t name_len,
                        struct variable *v, u_int32_t vr_id)
{

#ifdef HAVE_QOS
  long intval;
  int user_priority;
  u_int32_t index =0;
  struct nsm_bridge *br = NULL;
  struct interface *ifp = NULL ;
  u_char traffic_num_class = 4;
  int exact =0;
  u_int32_t traffic_priority =0;
  int ret;
  struct nsm_bridge_port *br_port;
 
  br_port = NULL;

  if (var_val_type != ASN_INTEGER)
    return SNMP_ERR_WRONGTYPE;

  if (var_val_len != sizeof (long))
    return SNMP_ERR_WRONGLENGTH;

  /* validate the index */
  ret = l2_snmp_integer_index_get (v, name, &name_len, &index,&traffic_priority, exact);
  if (ret < 0)
    return SNMP_ERR_GENERR;


  pal_mem_cpy(&intval,var_val,4);
  br = nsm_snmp_get_first_bridge();

  if (br == NULL)
    return SNMP_ERR_GENERR;

  ifp = nsm_snmp_bridge_port_lookup_by_port_id (br, index, 1, &br_port);

  if (ifp == NULL || br_port == NULL)
    return SNMP_ERR_GENERR;

  index = name[name_len];
  user_priority = name[name_len -1];

  if (intval < 0 || intval > 7)
    return SNMP_ERR_BADVALUE;
  nsm_vlan_port_set_traffic_class_table(ifp,user_priority,traffic_num_class,intval);

#endif /* HAVE_QOS */

  return SNMP_ERR_NOERROR;
}

#ifdef HAVE_QOS
int
write_dot1dPortPriorityEntryTable(int action,
                                  u_char * var_val,
                                  u_char var_val_type,
                                  size_t var_val_len,
                                  u_char * statP,
                                  oid * name, size_t name_len,
                                  struct variable *v, u_int32_t vr_id)
{


  long intval;
  int index;
  struct interface *ifp = NULL;
  struct nsm_bridge_port *br_port;
  struct nsm_bridge *br = nsm_snmp_get_first_bridge();
  int ret = 0;

  br_port = NULL;

  if (br == NULL)
    return SNMP_ERR_GENERR;

  if (var_val_type != ASN_INTEGER)
    return SNMP_ERR_WRONGTYPE;

  if (var_val_len != sizeof (long))
    return SNMP_ERR_WRONGLENGTH;

  pal_mem_cpy(&intval,var_val,4);

  if ( l2_snmp_index_get (v, name, &name_len,
                          (u_int32_t *) &index, 1) != RESULT_OK )
    return SNMP_ERR_UNKNOWN;

  ifp = nsm_snmp_bridge_port_lookup_by_port_id (br, index, 1, &br_port);

  if (ifp == NULL || br_port == NULL)
    return SNMP_ERR_UNKNOWN;

  switch (v->magic)
    {
     case DOT1DPORTNUMTRAFFICCLASSES:
       {
         if (intval < 1 || intval > 8)
           return SNMP_ERR_BADVALUE;

         ret = nsm_qos_set_num_cosq (intval);

         if (ret < 0)
           return SNMP_ERR_BADVALUE;
         else
           br->num_cosq = intval;

         break;
       }
    case DOT1DPORTDEFAULTUSERPRIORITY:
       {
         if (intval < 0 || intval > 7)
           return SNMP_ERR_BADVALUE;
         if ( (nsm_vlan_port_set_default_user_priority (ifp,intval)) < 0 )
           return SNMP_ERR_UNKNOWN;
         break;
       }
    }

  return SNMP_ERR_NOERROR;
}
#endif /* HAVE_QOS */

#if defined (HAVE_GMRP) || defined (HAVE_GVRP)
int
write_dot1dPortGarpTime(int action,
                        u_char * var_val,
                        u_char var_val_type,
                        size_t var_val_len,
                        u_char * statP, oid * name, size_t name_len,
                        struct variable *v, u_int32_t vr_id)
{
  bool_t timers_set = PAL_FALSE;
  int   index;
  long  intval;
  u_int32_t timer_type;
  struct nsm_bridge_port *br_port = NULL;
  struct nsm_bridge *br = nsm_snmp_get_first_bridge();
  struct apn_vr *vr = apn_vr_get_privileged (snmpg);
  struct nsm_master *master = NULL;
  struct nsm_bridge_master *bridge_master = NULL;
  struct interface *ifp = NULL;
  struct nsm_if *zif = NULL;

  br_port = NULL;

  if (br == NULL)
    return SNMP_ERR_UNKNOWN;

  if (! vr)
    return SNMP_ERR_UNKNOWN;

  master = vr->proto;

  if (! master)
    return SNMP_ERR_UNKNOWN;

  bridge_master = master->bridge;

  if (! bridge_master)
    return SNMP_ERR_UNKNOWN;

  if (var_val_type != ASN_INTEGER)
    return SNMP_ERR_WRONGTYPE;

  if (var_val_len > sizeof(long))
    return SNMP_ERR_WRONGLENGTH;

  pal_mem_cpy(&intval,var_val,4);

  if (intval < 0 || intval > UINT32_MAX)
    return SNMP_ERR_BADVALUE;
  
  if ( l2_snmp_index_get (v, name, &name_len,
                          (u_int32_t *) &index, 1) != RESULT_OK )
    return SNMP_ERR_UNKNOWN;

  ifp = nsm_snmp_bridge_port_lookup_by_port_id (br, index, 1, &br_port);

  if (ifp == NULL || br_port == NULL)
    return SNMP_ERR_UNKNOWN;

  zif = (struct nsm_if *)ifp->info;

  if (!zif)
    return SNMP_ERR_UNKNOWN;

  br_port = zif->switchport;

  if (!br_port)
    return SNMP_ERR_UNKNOWN;

  switch (v->magic)
    {
    case DOT1DPORTGARPJOINTIME:
      timer_type = GARP_JOIN_TIMER;
      break;
    case DOT1DPORTGARPLEAVETIME:
      timer_type = GARP_LEAVE_TIMER;
      break;
    case DOT1DPORTGARPLEAVEALLTIME:
      timer_type = GARP_LEAVE_ALL_TIMER;
      break;
    default:
      return SNMP_ERR_BADVALUE;
    }

#ifdef HAVE_GMRP
  if (br_port->gmrp_port_list)
    {
      if ( gmrp_check_timer_rules(bridge_master, ifp, timer_type, intval) !=
           RESULT_OK )
        return SNMP_ERR_BADVALUE;

      if (gmrp_set_timer (bridge_master, ifp, timer_type, intval) != RESULT_OK)
        return SNMP_ERR_UNKNOWN;
      
      timers_set = PAL_TRUE;
    }
#endif

#ifdef HAVE_GVRP
  if (br_port->gvrp_port)
    {
      if ( gvrp_check_timer_rules(bridge_master, ifp, timer_type, intval) !=
           RESULT_OK )
        return SNMP_ERR_BADVALUE;
      
      if (gvrp_set_timer (bridge_master, ifp, timer_type, intval) != RESULT_OK)
        return SNMP_ERR_UNKNOWN;

      timers_set = PAL_TRUE;
    }
#endif

  if (! timers_set)
    return SNMP_ERR_UNKNOWN;
    
  return SNMP_ERR_NOERROR;
}
#endif
#endif /* HAVE_SNMP. */

