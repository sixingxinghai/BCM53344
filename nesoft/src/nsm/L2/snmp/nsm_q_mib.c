/* Copyright (C) 2001-2004  All Rights Reserved. */

#include "pal.h"
#include "lib.h"
#ifdef HAVE_SNMP
#ifdef HAVE_VLAN
#include <asn1.h>
#include "if.h"
#include "log.h"
#include "prefix.h"
#include "avl_tree.h"
#include "table.h"
#include "ptree.h"
#include "snmp.h"
#include "l2_snmp.h"
#include "pal_math.h"
#include "nsmd.h"
#include "nsm_snmp_common.h"
#include "nsm_snmp.h"
#include "nsm_L2_snmp.h"
#include "nsm_bridge.h"
#include "nsm_vlan.h"
#include "nsm_interface.h"
#include "nsmd.h"
#include "table.h"
#include "nsm_q_mib.h"
#include "nsm_bridge_cli.h"
#include "nsm_vlan_cli.h"
#include "hal_l2.h"
#ifdef HAVE_HAL
#include "hal_incl.h"
#endif

#ifdef HAVE_VLAN_CLASS
#include "nsm_vlanclassifier.h"
#endif

#ifdef HAVE_GVRP
#include "gvrp_api.h"
#include "gvrp_database.h"
#endif

#ifdef HAVE_GMRP
#include "gmrp_api.h"
#include "gmrp_cli.h"
#endif

oid xstp_oid [] = { BRIDGEMIBOID };
oid xstp_nsm_oid [] = { BRIDGEMIBOID , 7 };

int
xstp_snmp_classifier_map_snmpdb2nsm_ruledb(
                                     struct hal_vlan_classifier_rule *rule_ptr,
                                     struct nsm_vlanclass_frame_proto *entry);
int
nsm_vlan_classifier_get_new_rule_id (struct nsm_bridge_master *master);

/* cached FDB entries for dot1dTpFdbTable */
extern XSTP_FDB_CACHE *fdb_cache;
static struct VlanStaticTable *vlan_static_table;
static struct VlanStaticEntry vstatic_write;
static struct PortVlanTable *port_vlan_table;
static struct ForwardAllTable *forward_all_table;
static struct ForwardUnregisteredTable *forward_unregistered_table;
static struct FidTable *fid_table;
static struct port_vlan_HC_table *port_vlan_HC_table;
static struct port_vlan_stats_table *port_vlan_stats_table;
static struct vlan_current_table *vlan_current_table;
static ut_int64_t xstp_int64_val;
static struct list *xstp_snmp_classifier_tmp_frame_proto_list;
static struct list *xstp_snmp_classifier_tmp_if_group_list;

/* cached FDB entries for dot1dTpStaticTable */
extern XSTP_FDB_CACHE *fdb_static_cache;
unsigned short
xstp_snmp_classifier_get_ether_type(char *protocol_value);

extern u_char *xstp_string_val;
extern char  br_ports_bitstring[BITSTRINGSIZE(NSM_L2_SNMP_PORT_ARRAY_SIZE)];
char  Learnt_ports_portmap[BITSTRINGSIZE(NSM_L2_SNMP_PORT_ARRAY_SIZE)];
/* function prototypes */

/*
 * variable dot1dBridge_variables:
 *   this variable defines function callbacks and type return information
 *   for the  mib section
 */

struct variable xstp_pq_dot1dBridge_variables[] = {
  /* qBridgeMIB (7) */
  /* qBridgeMIBObjects (7,1) */
  /* dot1qBase (7,1,1) */
  {DOT1QVLANVERSIONNUMBER,            ASN_INTEGER,    RONLY, xstp_snmp_qBridge_scalars,
   3, {1,1,1}},
  {DOT1QMAXVLANID,                    ASN_INTEGER,    RONLY, xstp_snmp_qBridge_scalars,
   3, {1,1,2}},
  {DOT1QMAXSUPPORTEDVLANS,            ASN_UNSIGNED,   RONLY, xstp_snmp_qBridge_scalars,
   3, {1,1,3}},
  {DOT1QNUMVLANS,                     ASN_UNSIGNED,   RONLY, xstp_snmp_qBridge_scalars,
   3, {1,1,4}},
  {DOT1QGVRPSTATUS,                   ASN_INTEGER,    RWRITE,xstp_snmp_qBridge_scalars,
   3, {1,1,5}},

  /* dot1qTp (7,1,2) */
  /* dot1qFdbTable (7,1,2,1) */
  /* dot1qFdbEntry (7,1,2,1,1) */
  {DOT1QFDBID,                        ASN_UNSIGNED,   RONLY, xstp_snmp_dot1qFdbTable,
   5, {1,2,1,1,1}},
  {DOT1QFDBDYNAMICCOUNT,              ASN_COUNTER,    RONLY, xstp_snmp_dot1qFdbTable,
   5, {1,2,1,1,2}},

  /* dot1qTpFdbTable (7,1,2,2) */
  /* dot1qTpFdbEntry (7,1,2,2,1) */
  {DOT1QTPFDBADDRESS,                 ASN_OCTET_STR,  NOACCESS, xstp_snmp_dot1qTpFdbTable,
   5, {1,2,2,1,1}},
  {DOT1QTPFDBPORT,                    ASN_INTEGER,    RONLY,    xstp_snmp_dot1qTpFdbTable,
   5, {1,2,2,1,2}},
  {DOT1QTPFDBSTATUS,                  ASN_INTEGER,    RONLY,    xstp_snmp_dot1qTpFdbTable,
   5, {1,2,2,1,3}},

  /* dot1qTpGroupTable (7,1,2,3) */
  /* dot1qTpGroupEntry (7,1,2,3,1) */
  {DOT1QTPGROUPADDRESS,               ASN_OCTET_STR,  NOACCESS, xstp_snmp_dot1qTpGroupTable,
   5, {1,2,3,1,1}},
  {DOT1QTPGROUPEGRESSPORTS,           ASN_OCTET_STR,  RONLY,    xstp_snmp_dot1qTpGroupTable,
   5, {1,2,3,1,2}},
  {DOT1QTPGROUPLEARNT,                ASN_OCTET_STR,  RONLY,    xstp_snmp_dot1qTpGroupTable,
   5, {1,2,3,1,3}},
#ifdef HAVE_GMRP
  /* dot1qForwardAllTable (7,1,2,4) */
  /* dot1qForwardAllEntry (7,1,2,4,1) */
  {DOT1QFORWARDALLPORTS,              ASN_OCTET_STR, RONLY, xstp_snmp_dot1qForwardAllTable,
   5, {1,2,4,1,1}},
  {DOT1QFORWARDALLSTATICPORTS,        ASN_OCTET_STR, RWRITE,xstp_snmp_dot1qForwardAllTable,
   5, {1,2,4,1,2}},
  {DOT1QFORWARDALLFORBIDDENPORTS,     ASN_OCTET_STR, RWRITE,xstp_snmp_dot1qForwardAllTable,
   5, {1,2,4,1,3}},

  /* dot1qForwardUnregisteredTable (7,1,2,5) */
  /* dot1qForwardUnregisteredEntry (7,1,2,5,1) */
  {DOT1QFORWARDUNREGISTEREDPORTS,           ASN_OCTET_STR,  RONLY,  xstp_snmp_dot1qForwardUnregisteredTable,
   5, {1,2,5,1,1}},
  {DOT1QFORWARDUNREGISTEREDSTATICPORTS,     ASN_OCTET_STR,  RWRITE, xstp_snmp_dot1qForwardUnregisteredTable,
   5, {1,2,5,1,2}},
  {DOT1QFORWARDUNREGISTEREDFORBIDDENPORTS,  ASN_OCTET_STR,  RWRITE, xstp_snmp_dot1qForwardUnregisteredTable,
   5, {1,2,5,1,3}},
#endif


  /* dot1qStatic (7,1,3) */
  /* dot1qStaticUnicastTable (7,1,3,1) */
  /* dot1qStaticUnicastEntry (7,1,3,1,1) */
  {DOT1QSTATICUNICASTADDRESS,               ASN_OCTET_STR,  NOACCESS,  xstp_snmp_dot1qStaticUnicastTable,
   5, {1,3,1,1,1}},
  {DOT1QSTATICUNICASTRECEIVEPORT,           ASN_INTEGER,    NOACCESS,  xstp_snmp_dot1qStaticUnicastTable,
   5, {1,3,1,1,2}},
  {DOT1QSTATICUNICASTALLOWEDTOGOTO,         ASN_OCTET_STR,  RWRITE, xstp_snmp_dot1qStaticUnicastTable,
   5, {1,3,1,1,3}},
  {DOT1QSTATICUNICASTSTATUS,                ASN_INTEGER,    RWRITE, xstp_snmp_dot1qStaticUnicastTable,
   5, {1,3,1,1,4}},

  /* dot1qStaticMulticastTable (7,1,3,2) */
  /* dot1qStaticMulitcastEntry (7,1,3,2,1) */
  {DOT1QSTATICMULTICASTADDRESS,               ASN_OCTET_STR, NOACCESS,
                                           xstp_snmp_dot1qStaticMulticastTable,
   5, {1,3,2,1,1}},
  {DOT1QSTATICMULTICASTRECEIVEPORT,           ASN_INTEGER,   NOACCESS,
                                           xstp_snmp_dot1qStaticMulticastTable,
   5, {1,3,2,1,2}},
  {DOT1QSTATICMULTICASTSTATICEGRESSPORTS,     ASN_OCTET_STR,  RWRITE, 
                                           xstp_snmp_dot1qStaticMulticastTable,
   5, {1,3,2,1,3}},
  {DOT1QSTATICMULTICASTFORBIDDENEGRESSPORTS,  ASN_OCTET_STR,  RWRITE, 
                                           xstp_snmp_dot1qStaticMulticastTable,
   5, {1,3,2,1,4}},
  {DOT1QSTATICMULTICASTSTATUS,                ASN_INTEGER,    RWRITE, 
                                           xstp_snmp_dot1qStaticMulticastTable,
   5, {1,3,2,1,5}},

  /* dot1qVlan (7,1,4) */
  {DOT1QVLANNUMDELETES,                       ASN_COUNTER,    RONLY,  xstp_snmp_qBridge_scalars,
   3, {1,4,1}},

  /* dot1qVlanCurrentTable (7,1,4,2) */
  /* dot1qVlanCurrentEntry (7,1,4,2,1) */
  {DOT1QVLANTIMEMARK,                         ASN_UNSIGNED,   RONLY, xstp_snmp_dot1qVlanCurrentTable,
   5, {1,4,2,1,1}},
  {DOT1QVLANINDEX,                            ASN_UNSIGNED,   RONLY, xstp_snmp_dot1qVlanCurrentTable,
   5, {1,4,2,1,2}},
  {DOT1QVLANFDBID,                            ASN_UNSIGNED,   RONLY, xstp_snmp_dot1qVlanCurrentTable,
   5, {1,4,2,1,3}},
  {DOT1QVLANCURRENTEGRESSPORTS,               ASN_OCTET_STR,  RONLY, xstp_snmp_dot1qVlanCurrentTable,
   5, {1,4,2,1,4}},
  {DOT1QVLANCURRENTUNTAGGEDPORTS,             ASN_OCTET_STR,  RONLY, xstp_snmp_dot1qVlanCurrentTable,
   5, {1,4,2,1,5}},
  {DOT1QVLANSTATUS,                           ASN_INTEGER,    RONLY, xstp_snmp_dot1qVlanCurrentTable,
   5, {1,4,2,1,6}},
  {DOT1QVLANCREATIONTIME,                     ASN_TIMETICKS,  RONLY, xstp_snmp_dot1qVlanCurrentTable,
   5, {1,4,2,1,7}},

  /* dot1qVlanStaticTable (7,1,4,3) */
  /* dot1qVlanStaticEntry (7,1,4,3,1) */
  {DOT1QVLANSTATICNAME,                       ASN_OCTET_STR,  RWRITE, xstp_snmp_dot1qVlanStaticTable,
   5, {1,4,3,1,1}},
  {DOT1QVLANSTATICEGRESSPORTS,                ASN_OCTET_STR,  RWRITE, xstp_snmp_dot1qVlanStaticTable,
   5, {1,4,3,1,2}},
  {DOT1QVLANFORBIDDENEGRESSPORTS,             ASN_OCTET_STR,  RWRITE, xstp_snmp_dot1qVlanStaticTable,
   5, {1,4,3,1,3}},
  {DOT1QVLANSTATICUNTAGGEDPORTS,              ASN_OCTET_STR,  RWRITE, xstp_snmp_dot1qVlanStaticTable,
   5, {1,4,3,1,4}},
  {DOT1QVLANSTATICROWSTATUS,                  ASN_INTEGER,    RWRITE, xstp_snmp_dot1qVlanStaticTable,
   5, {1,4,3,1,5}},

  /* dot1qNextFreeLocalVlanIndex (7,1,4,4) */
  {DOT1QNEXTFREELOCALVLANINDEX,               ASN_INTEGER,    RONLY, xstp_snmp_qBridge_scalars,
   3, {1,4,4}},
  /* dot1qPortVlanTable (7,1,4,5) */
  /* dot1qPortVlanEntry (7,1,4,5,1) */
  {DOT1QPVID,                                 ASN_UNSIGNED,   RWRITE, xstp_snmp_dot1qPortVlanTable,
   5, {1,4,5,1,1}},
  {DOT1QPORTACCEPTABLEFRAMETYPES,             ASN_INTEGER,    RWRITE, xstp_snmp_dot1qPortVlanTable,
   5, {1,4,5,1,2}},
  {DOT1QPORTINGRESSFILTERING,                 ASN_INTEGER,    RWRITE, xstp_snmp_dot1qPortVlanTable,
   5, {1,4,5,1,3}},
#ifdef HAVE_GVRP
  {DOT1QPORTGVRPSTATUS,                       ASN_INTEGER,    RWRITE, xstp_snmp_dot1qPortVlanTable,
   5, {1,4,5,1,4}},
  {DOT1QPORTGVRPFAILEDREGISTRATIONS,          ASN_COUNTER,    RONLY,  xstp_snmp_dot1qPortVlanTable,
   5, {1,4,5,1,5}},
  {DOT1QPORTGVRPLASTPDUORIGIN,                ASN_OCTET_STR,  RONLY,  xstp_snmp_dot1qPortVlanTable,
   5, {1,4,5,1,6}},
  {DOT1QPORTRESTRICTEDVLANREGISTRATION,      ASN_INTEGER,    RWRITE, xstp_snmp_dot1qPortVlanTable,
   5,{1,4,5,1,7}},
#endif

  /* dot1qPortVlanStatisticsTable (7,1,4,6) */
  /* dot1qPortVlanStatisticsEntry (7,1,4,6,1) */
  {DOT1QTPVLANPORTINFRAMES,                   ASN_COUNTER, RONLY, xstp_snmp_dot1qPortVlanStatisticsTable,
   5, {1,4,6,1,1}},
  {DOT1QTPVLANPORTOUTFRAMES,                  ASN_COUNTER, RONLY, xstp_snmp_dot1qPortVlanStatisticsTable,
   5, {1,4,6,1,2}},
  {DOT1QTPVLANPORTINDISCARDS,                 ASN_COUNTER, RONLY, xstp_snmp_dot1qPortVlanStatisticsTable,
   5, {1,4,6,1,3}},
  {DOT1QTPVLANPORTINOVERFLOWFRAMES,           ASN_COUNTER, RONLY, xstp_snmp_dot1qPortVlanStatisticsTable,
   5, {1,4,6,1,4}},
  {DOT1QTPVLANPORTOUTOVERFLOWFRAMES,          ASN_COUNTER, RONLY, xstp_snmp_dot1qPortVlanStatisticsTable,
   5, {1,4,6,1,5}},
  {DOT1QTPVLANPORTINOVERFLOWDISCARDS,         ASN_COUNTER, RONLY, xstp_snmp_dot1qPortVlanStatisticsTable,
   5, {1,4,6,1,6}},

  /* dot1qPortVlanHCStatisticsTable (7,1,4,7) */
  /* dot1qPortVlanHCStatisticsEntry (7,1,4,7,1) */
  {DOT1QTPVLANPORTHCINFRAMES,                 ASN_COUNTER64, RONLY, xstp_snmp_dot1qPortVlanHCStatisticsTable,
   5, {1,4,7,1,1}},
  {DOT1QTPVLANPORTHCOUTFRAMES,                ASN_COUNTER64, RONLY, xstp_snmp_dot1qPortVlanHCStatisticsTable,
   5, {1,4,7,1,2}},
  {DOT1QTPVLANPORTHCINDISCARDS,               ASN_COUNTER64, RONLY, xstp_snmp_dot1qPortVlanHCStatisticsTable,
   5, {1,4,7,1,3}},
  /* dot1qLearningConstraintsTable (7,1,4,8) */
  /* dot1qLearningConstraintsEntry (7,1,4,8,1) */
  {DOT1QCONSTRAINTVLAN,                       ASN_UNSIGNED, RONLY,  xstp_snmp_dot1qLearningConstraintsTable,
   5, {1,4,8,1,1}},
  {DOT1QCONSTRAINTSET,                        ASN_INTEGER,  RONLY,  xstp_snmp_dot1qLearningConstraintsTable,
   5, {1,4,8,1,2}},
  {DOT1QCONSTRAINTTYPE,                       ASN_INTEGER,  RWRITE, xstp_snmp_dot1qLearningConstraintsTable,
   5, {1,4,8,1,3}},
  {DOT1QCONSTRAINTSTATUS,                     ASN_INTEGER,  RWRITE, xstp_snmp_dot1qLearningConstraintsTable,
   5, {1,4,8,1,4}},

  /* dot1qConstraintSetDefault (7,1,4,9) */
  {DOT1QCONSTRAINTSETDEFAULT,                 ASN_INTEGER,  RWRITE, xstp_snmp_qBridge_scalars,
   3, {1,4,9}},
  /* dot1qConstraintTypeDefault (7,1,4,10) */
  {DOT1QCONSTRAINTTYPEDEFAULT,                ASN_INTEGER,  RWRITE, xstp_snmp_qBridge_scalars,
   3, {1,4,10}},

#ifdef HAVE_VLAN_CLASS
  /* dot1vProtocolGroupTable(7,1,5,1) */
  /* dot1vProtocolGroupEntry(7,1,5,1,1) */
   {DOT1VPROTOCOLTEMPLATEFRAMETYPE,            ASN_INTEGER, NOACCESS ,xstp_snmp_dot1vProtocolGroupTable,
    5,{1,5,1,1,1}},
   {DOT1VPROTOCOLTEMPLATEPROTOCOLVALUE,        ASN_OCTET_STR ,NOACCESS ,xstp_snmp_dot1vProtocolGroupTable,
    5,{1,5,1,1,2}},
   {DOT1VPROTOCOLGROUPID,                      ASN_INTEGER,RWRITE,xstp_snmp_dot1vProtocolGroupTable,
    5,{1,5,1,1,3}},
   {DOT1VPROTOCOLGROUPROWSTATUS,               ASN_INTEGER,RWRITE,xstp_snmp_dot1vProtocolGroupTable,
   5,{1,5,1,1,4}},

 /* dot1vProtocolPortTable(7,1,5,2) */
 /* dot1vProtocolPortEntry (7,1,5,2,1) */

   {DOT1VPROTOCOLPORTGROUPID,                  ASN_INTEGER, RONLY ,xstp_snmp_dot1vProtocolPortTable,
    5,{1,5,2,1,1}},
   {DOT1VPROTOCOLPORTGROUPVID,                 ASN_INTEGER,RWRITE,xstp_snmp_dot1vProtocolPortTable,
    5,{1,5,2,1,2}},
   {DOT1VPROTOCOLPORTROWSTATUS,                ASN_INTEGER,RWRITE,xstp_snmp_dot1vProtocolPortTable,
    5,{1,5,2,1,3}}
#endif /* HAVE_VLAN_CLASS */
};

static int xstp_int_val;

void
xstp_q_snmp_init( struct lib_globals *zg )
{

  /*
   * register ourselves with the agent to handle our mib tree objects
   */
  REGISTER_MIB(zg, "xstp_snmp_q_Bridge", xstp_pq_dot1dBridge_variables, variable, xstp_nsm_oid);

  /* Init vlan classification rule lists */
  xstp_snmp_classifier_tmp_frame_proto_list = list_new (); 
  xstp_snmp_classifier_tmp_if_group_list = list_new (); 
}


/*
 * xstp_snmp_qBridge_scalars():
 *   This function is called every time the agent gets a request for
 *   a scalar variable that might be found within the pBridgeMIB tree or qBridgeMIB tree
 */
unsigned char  *
xstp_snmp_qBridge_scalars(struct variable *vp,
                          oid * name,
                          size_t * length,
                          int exact, size_t * var_len, 
                          WriteMethod ** write_method,
                          u_int32_t vr_id)
{
  struct nsm_bridge *br = NULL;
  struct avl_node *rn = NULL;
  int vlan_count = 0;
#ifdef HAVE_GVRP
  int retStatus;
#endif

  if (snmp_header_generic(vp, name, length, exact, var_len, write_method)
      == MATCH_FAILED)
    return NULL;

  br = nsm_snmp_get_first_bridge();

  if (br == NULL)
    {
      return NULL;
    }
  switch (vp->magic) {
  case DOT1QVLANVERSIONNUMBER:
#ifdef HAVE_MSTPD
    XSTP_SNMP_RETURN_INTEGER(2);
#else
    XSTP_SNMP_RETURN_INTEGER(1);
#endif
    break;

  case DOT1QMAXVLANID:
    XSTP_SNMP_RETURN_INTEGER(NSM_VLAN_MAX);
    break;

  case DOT1QMAXSUPPORTEDVLANS:
    XSTP_SNMP_RETURN_INTEGER(NSM_VLAN_MAX);
    break;

  case DOT1QNUMVLANS:
    
    if (!br->vlan_table)
      return NULL;

    for (rn = avl_top (br->vlan_table); rn; rn = avl_next (rn))
    {
      if (rn->info)
        {
          vlan_count++; 
        }
    }

    XSTP_SNMP_RETURN_INTEGER(vlan_count);
    break;

#ifdef HAVE_GVRP
  case DOT1QGVRPSTATUS:
    *write_method = xstp_snmp_write_qBridge_scalars;
    retStatus = br->gvrp ? DOT1DPORTGVRPSTATUS_ENABLED :
      DOT1DPORTGVRPSTATUS_DISABLED;
    XSTP_SNMP_RETURN_INTEGER(retStatus);
    break;
#endif

  case DOT1QVLANNUMDELETES:
    XSTP_SNMP_RETURN_INTEGER(br->vlan_num_deletes);
    break;
  
/*This object will return 0 because we dont support
      vlan id's greater than 4096*/
  case DOT1QNEXTFREELOCALVLANINDEX:
    XSTP_SNMP_RETURN_INTEGER(0);
    break;

  case DOT1QCONSTRAINTTYPEDEFAULT:
    /* As only the independent vlan  type of filtering database is 
     * supported, the defaultconstraint type is same as the 
     * learningconstraint type which is 1*/
    XSTP_SNMP_RETURN_INTEGER(1);
    break;

  case DOT1QCONSTRAINTSETDEFAULT:
   /* for all configured vlans, the defaultconstraintset is 1.
    * Setting this field is not allowed. 
    */
    XSTP_SNMP_RETURN_INTEGER(1);
    break;

  default:
    break;
  }
  return NULL;
}

int
xstp_snmp_update_port_vlan_stats_table ( struct port_vlan_stats_table **table, struct nsm_bridge * br)
{
  int i, j;
  struct nsm_if *zif = NULL;
  struct avl_node *rn = NULL;
  struct listnode *ln = NULL;
  struct nsm_vlan *vlan = NULL;
  struct interface *ifp = NULL;
  struct nsm_bridge_port *br_port = NULL;

  /* if the local table hasn't been allocated, try to do it now. */
  if ( *table == NULL )
    *table = XCALLOC (MTYPE_L2_SNMP_FDB, sizeof (struct port_vlan_stats_table) );
  if ( *table == NULL )
    return -1;

  (*table)->entries = 0;

   if (!br->vlan_table)
     return -1;
  
    for (rn = avl_top (br->vlan_table); rn; rn = avl_next (rn))
      {
         if ( (vlan = rn->info))
           {
             if (((*table)->entry[vlan->vid]) != NULL )
               XFREE(MTYPE_L2_SNMP_FDB,((*table)->entry[vlan->vid]));
             
             (*table)->entry[vlan->vid] = XCALLOC (MTYPE_L2_SNMP_FDB,
                                    sizeof (struct port_vlan_stats_entry) ); 
              if ( (*table)->entry[vlan->vid] == NULL )
                return -1;

              LIST_LOOP(vlan->port_list, ifp, ln)
                {
                   if ( (zif = (struct nsm_if *)(ifp->info))
                         && (br_port = zif->switchport) )
                     {
                        (*table)->entry[vlan->vid]->ports[br_port->port_id]
                                                      = br_port->port_id;
                        (*table)->entry[vlan->vid]->entries++;
                     }
                } 

              (*table)->entry[vlan->vid]->vlanid = vlan->vid;
            }
         (*table)->entries++;
       } 

  /* sort on the vlan (row) first */
  /* create a table of pointers to be sorted */
  for ( i=0; i<NSM_VLAN_MAX; i++ )
    {
      (*table)->sorted[i] = (*table)->entry[i];
    }  

  /* sort the vlan static table in descending order to get all vlans to top of
   * table */
  pal_qsort ( (*table)->sorted, NSM_VLAN_MAX,
              sizeof( ((*table)->sorted)[0]), l2_snmp_vlan_descend_cmp );
  /* then sort only valid entries in ascending order */
  pal_qsort ( (*table)->sorted, (*table)->entries,
              sizeof( ((*table)->sorted)[0]), l2_snmp_vlan_ascend_cmp );

  /* sort on the port (column) next) */
  /* create a table of pointers to be sorted */
  for ( i=0; i<(*table)->entries; i++ ) {
    for (j=0; j < NSM_L2_SNMP_MAX_PORTS; j++) {
      if ((*table)->sorted[i])
        (*table)->sorted[i]->sorted[j] = &((*table)->sorted[i]->ports[j]);
    }

    /* sort the vlan static table in descending order to get all vlans to top of
     * table */
    if ((*table)->sorted[i])
    if (((*table)->sorted[i]->entries)) {
      pal_qsort ( (*table)->sorted[i]->sorted, NSM_L2_SNMP_MAX_PORTS,
                sizeof( ((*table)->sorted[0]->sorted)[0]), 
                l2_snmp_port_descend_cmp );
      /* then sort only valid entries in ascending order */
      pal_qsort ( (*table)->sorted[i]->sorted, (*table)->entries,
                sizeof( ((*table)->sorted[0]->sorted[j])[0]), 
                l2_snmp_port_ascend_cmp );
    }

  }
  return 0;

}

/*
 * find best vlanid
 */
struct port_vlan_stats_entry *
xstp_snmp_port_vlan_stats_table_lookup (struct port_vlan_stats_table *table, 
                                        u_int32_t *port,u_int32_t *vlan, 
                                        int exact)
{
  struct port_vlan_stats_entry *best = NULL; /* assume none found */
  int i;

  for (i=0; i< table->entries; i++) {

    if (!table->sorted[i])
      continue;

    if (exact) {
      if (table->sorted[i]->vlanid == *vlan) {
        if ((table->sorted[i]->ports[*port]) == *port) {
          /* GET request and instance match */
          best = table->sorted[i];
        }
        break;
      }
    }
    else {                     /* GETNEXT request */
      if (*vlan < table->sorted[i]->vlanid) {
        /* save if this is a better candidate and
         *   continue search
         */
        if (best == NULL)
          best = table->sorted[i];
        else
          if (table->sorted[i]->vlanid < best->vlanid)
            best = table->sorted[i];
      }
    }
  }

  if (!exact && best)
    {
      *vlan = best->vlanid;
      for (i = 0; i < NSM_L2_SNMP_MAX_PORTS; i++)
        {
          if (best->ports [i] > *port)
            {
              *port = best->ports [i];
              break;
            }
        }
    }

  return best;
}

/*
 * xstp_snmp_dot1qPortVlanStatisticsTable():
 */
unsigned char  *
xstp_snmp_dot1qPortVlanStatisticsTable(struct variable *vp,
                                       oid * name,
                                       size_t * length,
                                       int exact,
                                       size_t * var_len,
                                       WriteMethod ** write_method,
                                       u_int32_t vr_id)
{
  int ret;
  u_int32_t port = 0;
  u_int32_t vlan = 0;
  struct nsm_bridge *br;
  struct port_vlan_stats_entry *vport;
#ifdef HAVE_HAL
  struct hal_vlan_if_counters if_stats;
#endif /* HAVE_HAL */

  pal_mem_set (&xstp_int_val, 0, sizeof (u_int32_t));

#ifdef HAVE_HAL
  pal_mem_set (&if_stats, 0, sizeof (struct hal_vlan_if_counters));
#endif /* HAVE_HAL */

  /* validate the index */
  ret = l2_snmp_port_vlan_index_get (vp, name, length, &port, &vlan, exact);
  if (ret < 0)
    return NULL;

  br = nsm_snmp_get_first_bridge ();
  if (br == NULL)
    return NULL;

  /* get the port vlan table */
  ret = xstp_snmp_update_port_vlan_stats_table ( &port_vlan_stats_table, br);

  if (ret < 0)
    return NULL;

  vport = xstp_snmp_port_vlan_stats_table_lookup (port_vlan_stats_table,
                                                  &port, &vlan, exact);

  if (vport == NULL)
    return NULL;
  
#ifdef HAVE_HAL
  hal_vlan_if_get_counters (port, vlan, &if_stats);
#endif /* HAVE_HAL */

  if (!exact)
    {
      l2_snmp_port_vlan_index_set (vp, name, length, port, vlan);
    }
#ifdef HAVE_HAL
  switch (vp->magic) {
  case DOT1QTPVLANPORTINFRAMES:
    pal_mem_cpy (&xstp_int_val, &if_stats.tp_vlan_port_in_frames,
                                               sizeof (u_int32_t));
    XSTP_SNMP_RETURN_INTEGER (xstp_int_val);
    break;
  case DOT1QTPVLANPORTOUTFRAMES:
    pal_mem_cpy (&xstp_int_val, &if_stats.tp_vlan_port_out_frames,
                                               sizeof (u_int32_t));
    XSTP_SNMP_RETURN_INTEGER (xstp_int_val);
    break;

  case DOT1QTPVLANPORTINDISCARDS:
    pal_mem_cpy (&xstp_int_val, &if_stats.tp_vlan_port_in_discards,
                                               sizeof (u_int32_t));
    XSTP_SNMP_RETURN_INTEGER (xstp_int_val);
 
   break;

  case DOT1QTPVLANPORTINOVERFLOWFRAMES:
    pal_mem_cpy (&xstp_int_val, &if_stats.tp_vlan_port_in_overflow_frames,
                                               sizeof (u_int32_t));
    XSTP_SNMP_RETURN_INTEGER (xstp_int_val);
  break;

  case DOT1QTPVLANPORTOUTOVERFLOWFRAMES:
    pal_mem_cpy (&xstp_int_val, &if_stats.tp_vlan_port_out_overflow_frames,
                                               sizeof (u_int32_t));
    XSTP_SNMP_RETURN_INTEGER64 (xstp_int_val);
    break;

  case DOT1QTPVLANPORTINOVERFLOWDISCARDS:
    pal_mem_cpy (&xstp_int_val, &if_stats.tp_vlan_port_in_overflow_discards,
                                               sizeof (u_int32_t));
    XSTP_SNMP_RETURN_INTEGER64 (xstp_int_val);
   break;
#endif /*HAVE_HAL*/
  default:
    break;
  }
  return NULL;
}

/* comparison function for sorting VLANs in the L2 vlan static table. Index
 * is the VLANID.  DESCENDING ORDER
 */
int
xstp_snmp_vlan_static_descend_cmp(const void *e1, const void *e2)
{
  struct VlanStaticEntry *vp1 = *(struct VlanStaticEntry **)e1;
  struct VlanStaticEntry *vp2 = *(struct VlanStaticEntry **)e2;

  /* if pointers are the same the data is equal */
  if (vp1 == vp2)
    return 0;
  /* only 1 valid pointer then the other is > */
  if (vp1 == NULL)
    return 1;
  if (vp2 == NULL)
    return -1;

  if ( vp1->vlanid < vp2->vlanid )
    return 1;
  else
    if ( vp1->vlanid > vp2->vlanid )
      return -1;
    else
      return 0;
}

/* comparison function for sorting VLANs in the L2 vlan static table. Index
 * is the VLANID.  ASCENDING ORDER
 */
int
xstp_snmp_vlan_static_ascend_cmp(const void *e1, const void *e2)
{
  struct VlanStaticEntry *vp1 = *(struct VlanStaticEntry **)e1;
  struct VlanStaticEntry *vp2 = *(struct VlanStaticEntry **)e2;

  /* if pointers are the same the data is equal */
  if (vp1 == vp2)
    return 0;
  /* only 1 valid pointer then the other is > */
  if (vp1 == NULL)
    return 1;
  if (vp2 == NULL)
    return -1;

  if ( vp1->vlanid < vp2->vlanid )
    return -1;
  else
    if ( vp1->vlanid > vp2->vlanid )
      return 1;
    else
      return 0;
}

/*
 * updates vlan Static table.
 * Returns -1 on error, else 0.
 */
int
xstp_snmp_update_vlan_static_table ( struct VlanStaticTable **table,
                                     struct nsm_bridge *br)
{
  int i;
  struct interface *ifp = NULL;
  int total_entries = 0;
  int active_entries = 0;
  struct avl_node *rn = NULL;
  struct nsm_vlan *vlan = NULL;
  struct listnode *ln = NULL;
  struct nsm_if *zif = NULL;
  struct nsm_vlan_port *port = NULL;
  struct nsm_bridge_port *br_port = NULL;

  /* if the local table hasn't been allocated, try to do it now. */
  if ( *table == NULL )
    *table = XCALLOC (MTYPE_L2_SNMP_FDB, sizeof (struct VlanStaticTable) );
  if ( *table == NULL )
    return -1;

  if (vlan_static_table)
    {
      pal_mem_set (vlan_static_table, 0, sizeof (vlan_static_table));
      if (*table)
        pal_mem_set (*table, 0, sizeof (vlan_static_table));
    }

  for (i=0; i < NSM_VLAN_MAX; i++)
    {
      if ((*table)->entry[i])
        pal_mem_set (&(*table)->entry[i], 0, sizeof (struct VlanStaticEntry));
      if ((*table)->sorted[i])
        pal_mem_set (&(*table)->entry[i], 0, sizeof (struct VlanStaticEntry));

    }

  if (!br->vlan_table)
    return -1;

  for (rn = avl_top (br->vlan_table); rn; rn = avl_next (rn))
    {
      if (! (vlan = rn->info))
        continue;

      if ((vlan->type & NSM_VLAN_STATIC) &&
          (!(vlan->type & NSM_VLAN_DYNAMIC)))
        {
          (*table)->entry[vlan->vid] = XCALLOC (MTYPE_L2_SNMP_FDB, 
                                                sizeof (struct VlanStaticEntry) );
          if ( (*table)->entry[vlan->vid] == NULL )
            return -1;

          (*table)->entry[vlan->vid]->vlanid = vlan->vid;

          pal_mem_cpy(((*table)->entry[vlan->vid])->name, vlan->vlan_name, 
              NSM_VLAN_NAME_MAX);

          LIST_LOOP(vlan->port_list, ifp, ln)
            {
              if ( (zif = (struct nsm_if *)(ifp->info)) 
                  && (br_port = zif->switchport) )
                {
                  port = &br_port->vlan_port;

                  bitstring_setbit((*table)->entry[vlan->vid]->egressports,
                      br_port->port_id - 1);

                  if (!NSM_VLAN_BMP_IS_MEMBER(port->egresstaggedBmp, vlan->vid))
                    bitstring_setbit((*table)->entry[vlan->vid]->untaggedports,
                        br_port->port_id - 1);   
                } 
            }

          LIST_LOOP(vlan->forbidden_port_list, ifp, ln)
            {
              if ( (zif = (struct nsm_if *)(ifp->info))
                  && (br_port = zif->switchport) )
                {
                  port = &br_port->vlan_port;

                  bitstring_setbit((*table)->entry[vlan->vid]->forbiddenports,
                      br_port->port_id - 1);
                }
            }

          LIST_LOOP(vlan->port_list, ifp, ln)
            {
              if ( (zif = (struct nsm_if *)(ifp->info))
                  && (br_port = zif->switchport) )
                {
                  port = &br_port->vlan_port;

                  bitstring_setbit((*table)->entry[vlan->vid]->egressports,
                      br_port->port_id - 1);
                }
            }

          /* set row status */
          (*table)->entry[vlan->vid]->rowstatus = 1;

          active_entries++;
        }
    }

  (*table)->active_entries = active_entries;

  /* create a table of pointers to be sorted */
  for ( i=0; i<NSM_VLAN_MAX; i++ ) {
    (*table)->sorted[i] = (*table)->entry[i];
  }

  /* sort the vlan static table in descending order to get all vlans to top of
   * table */
  pal_qsort ( (*table)->sorted, NSM_VLAN_MAX,
              sizeof( ((*table)->sorted)[0]), xstp_snmp_vlan_static_descend_cmp );
  /* then sort only valid entries in ascending order */
  total_entries = (*table)->active_entries + (*table)->inactive_entries;
  pal_qsort ( (*table)->sorted, total_entries,
              sizeof( ((*table)->sorted)[0]), xstp_snmp_vlan_static_ascend_cmp );

  return 0;
}

int xstp_snmp_add_vlan_static_table (int vid, struct VlanStaticTable **table,
                                     int set_mask, char *value,
                                     int row_status)
{
  struct vlan_db_summary vlan_summary;
  char vlan_name[32];

  pal_mem_set (&vlan_summary, 0, sizeof(vlan_summary));

  /* if the local table hasn't been allocated, try to do it now. */
  if ( *table == NULL )
    *table = XCALLOC (MTYPE_L2_SNMP_FDB, sizeof (struct VlanStaticTable) );
  if ( *table == NULL )
    return RESULT_ERROR;

  pal_mem_set (*table, 0, sizeof(struct VlanStaticTable));

  sprintf (vlan_name, "VLAN%04d", vid);
  if (set_mask & XSTP_SNMP_SET_STATIC_VLANNAME)
    pal_strcpy (vlan_name, value);

  (*table)->entry[vid] = XCALLOC (MTYPE_L2_SNMP_FDB,
                                  sizeof (struct VlanStaticEntry) );
  if ( (*table)->entry[vid] == NULL )
    return RESULT_ERROR;

  /* copy vlanid */
  (*table)->entry[vid]->vlanid = vid;

  /* copy vlan name */
  pal_mem_cpy(((*table)->entry[vid])->name, vlan_name, 32);

  /* copy egress ports */
  if (set_mask & XSTP_SNMP_SET_STATIC_VLAN_EGRESSPORT)
    pal_strcpy ((*table)->entry[vid]->egressports, value);

  /* copy untagged ports */
  if (set_mask & XSTP_SNMP_SET_STATIC_VLAN_UNTAGGEDPORT)
    pal_strcpy ((*table)->entry[vid]->untaggedports, value);

  (*table)->entry[vid]->rowstatus = row_status;

  vlan_static_table->inactive_entries++;

  return RESULT_OK;

}

int xstp_snmp_add_static_vlan_db (struct VlanStaticEntry *vstatic)
{
  struct nsm_bridge *br = NULL;
  int ret;

  br = nsm_snmp_get_first_bridge ();
  if (br == NULL)
    return RESULT_ERROR;

  ret = xstp_snmp_set_static_vlan_ports (vstatic,
      XSTP_SNMP_ADD_PORT_TO_VLAN);

  if (ret != RESULT_OK)
    {
   /* remove all the ports from vlan membership */
      ret = xstp_snmp_set_static_vlan_ports (vstatic,
          XSTP_SNMP_DELETE_PORT_FROM_VLAN);

      pal_mem_set (&vstatic_write, 0, sizeof (vstatic_write));
      if (ret == RESULT_ERROR)
        return RESULT_ERROR;
      else
        return SNMP_ERR_GENERR;

      /* remove the entry from snmp table */
      /*xstp_snmp_del_vlan_static_table (vstatic);*/
    }

  pal_mem_set (&vstatic_write, 0, sizeof (vstatic_write));

  return RESULT_OK;
}


int xstp_snmp_set_static_vlan_ports (struct VlanStaticEntry *vstatic, int action)
{
  int ret = 0;
  struct nsm_bridge *br = NULL;
  struct nsm_bridge_port *br_port;
  struct nsm_vlan *vlan = NULL;
  int vlan_name = 0;
  int set_mask = 0;
  u_char port_bit_array[4];

  br_port = NULL;
  pal_mem_set(port_bit_array, 0, 4);

  br = nsm_snmp_get_first_bridge();

  if (br == NULL)
    return RESULT_ERROR;

  if (action == XSTP_SNMP_ADD_PORT_TO_VLAN)
    {
      /*testbit check is added to ensure that the vlan is added
       * only to those ports which are written through snmp */
      if (vstatic_write.vlanid == vstatic->vlanid)
        {
          /* Finding whether this is non-existing vlan,
           * only then we need to add through 'create and go'*/
          vlan = nsm_vlan_find (br, vstatic->vlanid);

          if (vlan == NULL)
            {
              vlan_name = pal_strlen (vstatic_write.name);

              if (!vlan_name)
                {
                  ret = nsm_vlan_add (br->master, br->name, NULL,
                            vstatic_write.vlanid, NSM_VLAN_ACTIVE,
                            NSM_VLAN_STATIC | NSM_VLAN_CVLAN);
                  if (ret < 0)
                    return SNMP_ERR_GENERR;
                }
              else
                {
                  ret = nsm_vlan_add (br->master, br->name, vstatic_write.name,
                            vstatic_write.vlanid, NSM_VLAN_ACTIVE,
                            NSM_VLAN_STATIC | NSM_VLAN_CVLAN);
                  if (ret < 0)
                    return SNMP_ERR_GENERR;
                }

            }
          set_mask = set_mask | XSTP_SNMP_SET_STATIC_VLAN_EGRESSPORT;
          if (pal_mem_cmp (vstatic_write.egressports, port_bit_array, 4) != 0)
            ret = xstp_snmp_update_vlan_static_entry (&vstatic_write,
                br, set_mask, vstatic_write.egressports );

          if (ret != 0)
            return SNMP_ERR_GENERR;

          set_mask = 0;
          set_mask = set_mask | XSTP_SNMP_SET_STATIC_VLAN_FORBIDDENPORT;
          if (pal_mem_cmp (vstatic_write.forbiddenports, port_bit_array, 4) != 0)
            ret = xstp_snmp_update_vlan_static_entry (&vstatic_write, br,
                set_mask, vstatic_write.forbiddenports );

          if (ret != 0)
            return SNMP_ERR_GENERR;

          set_mask = 0;
          set_mask = set_mask | XSTP_SNMP_SET_STATIC_VLAN_UNTAGGEDPORT;
          if (pal_mem_cmp (vstatic_write.untaggedports, port_bit_array, 4) != 0)
            ret = xstp_snmp_update_vlan_static_entry (&vstatic_write, br,
                set_mask, vstatic_write.untaggedports);

          if (ret != 0)
            return SNMP_ERR_GENERR;

        }
      else if (vstatic_write.vlanid == 0)
        {
          /* Finding whether this is non-existing vlan,
           * only then we need to add through 'create and go'*/
          vlan = nsm_vlan_find (br, vstatic->vlanid);

          if (vlan == NULL)
            {
              vstatic_write.vlanid = vstatic->vlanid;
              ret = nsm_vlan_add (br->master, br->name, NULL,
                      vstatic_write.vlanid, NSM_VLAN_ACTIVE,
                      NSM_VLAN_STATIC | NSM_VLAN_CVLAN);

              if (ret < 0)
                return SNMP_ERR_GENERR;
            }
          else
            return SNMP_ERR_GENERR;

        }
      else
        return SNMP_ERR_GENERR;

      /* This code is commented out because when all the parameters are
       * not entered then an error need to be returned.
       * So we dont allow setting this vlan for the access port
       * ret = nsm_vlan_set_access_port (ifp, vstatic->vlanid, PAL_TRUE,
       PAL_TRUE);*/
    }
  else
    {
       ret = nsm_vlan_delete (br->master, br->name, vstatic_write.vlanid,
           NSM_VLAN_STATIC | NSM_VLAN_CVLAN);

    }


      if (ret < 0)
        return RESULT_ERROR;

  return RESULT_OK;
}

/*
   * find best vlanid
 */
struct VlanStaticEntry *
xstp_snmp_vlan_static_lookup (struct VlanStaticTable *v_table, int instance,
                              int exact)
{
  struct VlanStaticEntry *best = NULL; /* assume none found */
  int i;
  int entries = 0;

  if (!v_table)
    return best;

  entries = v_table->active_entries + v_table->inactive_entries;

  for (i = 0; i < entries; i++) {
    if (exact)
      {
         if (v_table && v_table->sorted[i])
           {
             if (v_table->sorted[i]->vlanid == instance)
               {
                 /* GET request and instance match */
                 best = v_table->sorted[i];
                 break;
               }
           }
      }
    else                      /* GETNEXT request */
      {
        if (v_table && v_table->sorted[i])
          {
            if (instance < v_table->sorted[i]->vlanid)
              {
                /* save if this is a better candidate and
                 * continue search
                 */
                if (best == NULL)
                  best = v_table->sorted[i];
                else
                  if (v_table->sorted[i]->vlanid < best->vlanid)
                    best = v_table->sorted[i];
              }
          }
      }
  }
  return best;
}

/*
 * xstp_snmp_dot1qVlanStaticTable():
 */
unsigned char  *
xstp_snmp_dot1qVlanStaticTable(struct variable *vp,
                               oid * name,
                               size_t * length,
                               int exact,
                               size_t * var_len, WriteMethod ** write_method,
                               u_int32_t vr_id)
{
  struct nsm_bridge *br;
  int ret;
  u_int32_t index = 0;
  struct VlanStaticEntry *vstatic;

  /*for row-create tables,the write methods need to be set before any returns*/
  if (vlan_static_table)
    pal_mem_set (vlan_static_table, 0, sizeof (vlan_static_table));

  switch ( vp->magic ) {
      case DOT1QVLANSTATICNAME:
        *write_method = xstp_snmp_write_dot1qVlanStaticTable;
        break;
      case DOT1QVLANSTATICEGRESSPORTS:
        *write_method = xstp_snmp_write_dot1qVlanStaticTable;
        break;

      case DOT1QVLANFORBIDDENEGRESSPORTS:
        *write_method = xstp_snmp_write_dot1qVlanStaticTable;
        break;

      case DOT1QVLANSTATICUNTAGGEDPORTS:
        *write_method = xstp_snmp_write_dot1qVlanStaticTable;
        break;

      case DOT1QVLANSTATICROWSTATUS:
        *write_method = xstp_snmp_write_dot1qVlanStaticRowStatus;
        break;

    }

  /* validate the index */
  ret = l2_snmp_index_get (vp, name, length, &index, exact);
  if (ret < 0)
     return NULL;

  br = nsm_snmp_get_first_bridge ();

  if (br == NULL)
    return NULL;
  
  /* get the fdb table */
  ret = xstp_snmp_update_vlan_static_table ( &vlan_static_table, br);
  if (ret < 0)
    return NULL;

  vstatic = xstp_snmp_vlan_static_lookup ( vlan_static_table, index, exact);

  if (vstatic == NULL)
    return NULL;

  if (!exact)
  {
    l2_snmp_index_set (vp, name, length, vstatic->vlanid);
  }

    switch (vp->magic) {
      case DOT1QVLANSTATICNAME:
        XSTP_SNMP_RETURN_STRING (vstatic->name, strlen(vstatic->name));
        break;

      case DOT1QVLANSTATICEGRESSPORTS:
        XSTP_SNMP_RETURN_STRING (vstatic->egressports,
            sizeof(vstatic->egressports));
        break;

      case DOT1QVLANFORBIDDENEGRESSPORTS:
        XSTP_SNMP_RETURN_STRING (vstatic->forbiddenports,
            sizeof(vstatic->forbiddenports));
        break;

      case DOT1QVLANSTATICUNTAGGEDPORTS:
        XSTP_SNMP_RETURN_STRING (vstatic->untaggedports,
            sizeof(vstatic->untaggedports));
        break;

      case DOT1QVLANSTATICROWSTATUS:
        XSTP_SNMP_RETURN_INTEGER (vstatic->rowstatus);
        break;

    default:
        break;

    }
    return NULL;
}

/*
 * xstp_snmp_dot1qTpGroupTable():
 */
unsigned char  *
xstp_snmp_dot1qTpGroupTable(struct variable *vp,
                            oid * name,
                            size_t * length,
                            int exact,
                            size_t * var_len,
                            WriteMethod ** write_method,
                            u_int32_t vr_id)
{
  int ret;
  u_int8_t port_id;
  struct fdb_entry *fdb = NULL;
  struct vid_and_mac_addr key;
  int portmap_len = sizeof(Learnt_ports_portmap);
  int i;

  /* validate the index */
  ret = l2_snmp_int_mac_addr_index_get (vp, name, length, &key.vid, 
                                        &key.addr, exact);
  if (ret < 0)
    return NULL;

  /* get the fdb table */
  ret = xstp_snmp_update_fdb_cache2 ( &fdb_cache,
                                      XSTP_FDB_TABLE_BOTH | 
                                      XSTP_FDB_TABLE_UNIQUE_MAC,
                                      DOT1DTPFDBSTATUS );
  if (ret < 0)
    return NULL;

  if (fdb_cache->entries == 0)
    return NULL;

  fdb = xstp_snmp_fdb_vid_mac_addr_lookup ( fdb_cache, &key, exact);
  if (fdb == NULL)
    return NULL;

  if (!exact)
    {
      l2_snmp_int_mac_addr_index_set (vp, name, length,fdb->vid,
                                  (struct mac_addr *)(fdb->mac_addr));
    }

    switch (vp->magic)
   {
      case DOT1QTPGROUPADDRESS:
        return NULL;
        break;

      case DOT1QTPGROUPEGRESSPORTS:
        pal_mem_set( br_ports_bitstring, 0x00, sizeof(br_ports_bitstring));

      /* find any static FDB entries where forwarding is denied and set the
       * corresponding bit in the bit string for that port.
       */

        pal_mem_cpy( &key.addr, fdb->mac_addr, sizeof( fdb->mac_addr ) );
        key.vid = fdb->vid;

        do {
          if ( fdb == NULL ||
               pal_mem_cmp ( &key.addr, fdb->mac_addr,
                             sizeof( fdb->mac_addr )) != 0 )
            break;

          port_id = nsm_snmp_bridge_port_get_port_id (fdb->port_no);

          if ( (port_id > 0) &&
               (port_id <= NSM_L2_SNMP_MAX_PORTS) &&
               (fdb->is_fwd == 1) &&
               (fdb->is_static == 1))
            
             bitstring_setbit( br_ports_bitstring, port_id - 1 );

          fdb = xstp_snmp_fdb_vid_mac_addr_lookup (fdb_cache, &key, 0 );

        } while ( PAL_TRUE );

        XSTP_SNMP_RETURN_STRING (br_ports_bitstring, sizeof(br_ports_bitstring));
        break;

      case DOT1QTPGROUPLEARNT:
        pal_mem_set( Learnt_ports_portmap, 0x00, sizeof(Learnt_ports_portmap));

        for (i = 0; i < (fdb_cache)->entries; i++)
        {
          if ((pal_mem_cmp (fdb->mac_addr,
                 (fdb_cache->fdbs)[i].mac_addr,
                 sizeof (fdb->mac_addr)) == 0 ) &&
                 ((fdb_cache->fdbs)[i].vid  == fdb->vid))
          {
            port_id = nsm_snmp_bridge_port_get_port_id (
               (fdb_cache->fdbs)[i].port_no);

            if ((port_id > 0 && port_id <= NSM_L2_SNMP_MAX_PORTS)
                     && ((fdb_cache->fdbs)[i].is_fwd == PAL_TRUE)
                     && ((fdb_cache->fdbs)[i].is_static == PAL_FALSE))
                   bitstring_setbit (Learnt_ports_portmap, port_id - 1);
           }
        }

        XSTP_SNMP_RETURN_STRING (Learnt_ports_portmap, portmap_len);

      default:
        break;
    }

  return NULL;
}


/*
 * xstp_snmp_dot1qStaticMulticastTable():
 */

unsigned char  *
xstp_snmp_dot1qStaticMulticastTable(struct variable *vp,
                                    oid * name,
                                    size_t * length,
                                    int exact,
                                    size_t * var_len,
                                    WriteMethod ** write_method,
                                    u_int32_t vr_id)
{
  int ret;
  int i = 0;
  u_int8_t port_id;
  struct fdb_entry *fdb;
  struct vid_and_mac_addr_and_port key;

  /*for row-create tables,the write methods need to be set before any returns*/

  switch ( vp->magic ) {
    case DOT1QSTATICMULTICASTSTATICEGRESSPORTS:
      *write_method = xstp_snmp_write_dot1qStaticMulticastTable;
      break;
    case DOT1QSTATICMULTICASTSTATUS:
      *write_method = xstp_snmp_write_dot1qStaticMulticastStatus;
      break;
    case DOT1QSTATICMULTICASTFORBIDDENEGRESSPORTS:
      *write_method = xstp_snmp_write_dot1qStaticMulticastTable;
      break;
    default:
      break;
  }

  /* validate the index */
  ret = l2_snmp_int_mac_addr_int_index_get (vp, name, length, &key.vid,
                                               &key.addr, &key.port, exact);
  if (ret < 0)
    return NULL;

  /* get a cached fdb static table */
  ret = xstp_snmp_update_fdb_cache2 ( &fdb_static_cache,
      XSTP_FDB_TABLE_STATIC_MULTICAST, DOT1DSTATICSTATUS );
  if (ret < 0)
    return NULL;

  if (fdb_static_cache->entries == 0)
    return NULL;

  /* we only allow received port = 0. the fdb table has destination ports
   * in it for static entries.
   */

  key.port = 0;

  fdb = xstp_snmp_fdb_vid_mac_addr_port_lookup ( fdb_static_cache, &key, exact);

  if (fdb == NULL)
    return NULL;

  if (!exact)
    {
      l2_snmp_int_mac_addr_int_index_set (vp, name, length,fdb->vid,
                                         (struct mac_addr *)fdb->mac_addr, 0 );
    }

  switch (vp->magic)
    {
      case DOT1QSTATICMULTICASTADDRESS:
      break;

      case DOT1QSTATICMULTICASTRECEIVEPORT:
      break;

      case DOT1QSTATICMULTICASTSTATICEGRESSPORTS:
        pal_mem_set( br_ports_bitstring, 0x00, sizeof(br_ports_bitstring));

        /* find any static FDB entries where forwarding is denied and clear the
         * corresponding bit in the bit string for that port.
         */

        for ( i=0; i<(fdb_static_cache)->entries; i++ )
          {
            if ((pal_mem_cmp ( fdb->mac_addr,
                    (fdb_static_cache->fdbs)[i].mac_addr,
                    sizeof( fdb->mac_addr )) == 0 )
                && ((fdb_static_cache->fdbs)[i].vid  == fdb->vid))
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

      case DOT1QSTATICMULTICASTFORBIDDENEGRESSPORTS:
        *write_method = xstp_snmp_write_dot1qStaticMulticastTable;
        pal_mem_set( br_ports_bitstring, 0x00,sizeof(br_ports_bitstring));

        for ( i=0; i<(fdb_static_cache)->entries; i++ )
          {
            if ((pal_mem_cmp ( fdb->mac_addr,
                    (fdb_static_cache->fdbs)[i].mac_addr,
                    sizeof( fdb->mac_addr )) == 0 )
                && ((fdb_static_cache->fdbs)[i].vid  == fdb->vid))
              {

                port_id = nsm_snmp_bridge_port_get_port_id (
                    (fdb_static_cache->fdbs)[i].port_no);

                if ((port_id > 0 && port_id <= NSM_L2_SNMP_MAX_PORTS)
                    && ((fdb_static_cache->fdbs)[i].is_fwd == PAL_FALSE))
                  bitstring_setbit ( br_ports_bitstring, port_id - 1 );

              }
          }

        XSTP_SNMP_RETURN_STRING (br_ports_bitstring, sizeof(br_ports_bitstring));
        break;


      case DOT1QSTATICMULTICASTSTATUS:
        *write_method = xstp_snmp_write_dot1qStaticMulticastStatus;
        XSTP_SNMP_RETURN_INTEGER (fdb->snmp_status);
        break;

      default:
        pal_assert(0);
    }
  return NULL;
}

/*
 * given a bridge, instance and exact/inexact request, returns a pointer to
 * an xstp_port structure or NULL if none found.
 */
struct ForwardAllEntry *
xstp_snmp_forward_lookup (struct ForwardAllTable *table, int instance, int exact)
{
  struct ForwardAllEntry *best = NULL; /* assume none found */
  int i;

  for (i=0; i< table->entries; i++) {
    if (exact) {
      if (table->sorted[i]->vlanid == instance) {
        /* GET request and instance match */
        best = table->sorted[i];
        break;
      }
    }
    else {                     /* GETNEXT request */
      if (instance < table->sorted[i]->vlanid) {
        /* save if this is a better candidate and
         *   continue search
         */
        if (best == NULL)
          best = table->sorted[i];
        else
          if (table->sorted[i]->vlanid < best->vlanid)
            best = table->sorted[i];
      }
    }
  }
  return best;
}

#ifdef HAVE_GMRP
int
xstp_snmp_update_forward_all_table( struct ForwardAllTable **table, struct nsm_bridge * br)
{
  int i;
  u_char flags;
  int result = -1;
  int entries = 0;
  u_char is_gmrp_enabled;
  struct nsm_if *zif = NULL;
  struct avl_node *rn = NULL;
  struct listnode *ln = NULL;
  u_char is_gmrp_enabled_port;
  struct interface *ifp = NULL;
  struct nsm_vlan *vlan = NULL;
  struct ForwardAllEntry *entry;
  struct nsm_master *master = NULL;
  struct nsm_bridge_port *br_port = NULL;
  struct nsm_bridge_master *bridge_master = NULL;
  struct apn_vr *vr = apn_vr_get_privileged (snmpg);

  if (! vr)
    return result;

  master = vr->proto;

  if (! master)
    return result;

  bridge_master = master->bridge;

  if (! bridge_master)
    return result;

  do {                            /* once thru loop */
    /* if the local table hasn't been allocated, try to do it now. */
   if ( *table == NULL )
    *table = XCALLOC (MTYPE_L2_SNMP_FDB, sizeof (struct ForwardAllTable) );
   if ( *table == NULL )
      break;

   if ((!br->gmrp_bridge) ) {
      break;
   }

   is_gmrp_enabled = PAL_TRUE;

   if (!br->vlan_table)
     return result;

   for (rn = avl_top (br->vlan_table); rn; rn = avl_next (rn))
     {
        if ((vlan = rn->info))
          {
            if (((*table)->entry[vlan->vid]) != NULL)
               XFREE(MTYPE_L2_SNMP_FDB,((*table)->entry[vlan->vid]));
            
            (*table)->entry[vlan->vid] = XCALLOC (MTYPE_L2_SNMP_FDB, 
                                         sizeof (struct ForwardAllEntry) );

            if ( (*table)->entry[vlan->vid] == NULL )
              break;  

            entry = (*table)->entry[vlan->vid];
            entry->vlanid = vlan->vid;

            LIST_LOOP(vlan->port_list, ifp, ln)
              {

                if ((zif = ifp->info) == NULL
                    || (br_port = zif->switchport) == NULL)
                  continue;

                if (gmrp_get_port_details (bridge_master, ifp, &flags) == RESULT_OK)
                  {
                    is_gmrp_enabled_port = flags & GMRP_MGMT_PORT_CONFIGURED;

                    if (is_gmrp_enabled_port)
                      {
                        if (flags & GMRP_MGMT_FORWARD_ALL_CONFIGURED)
                          {
                            bitstring_setbit(entry->forwardallports, 
                                             br_port->port_id - 1); 

                            if (flags & GMRP_MGMT_FORWARD_ALL_FORBIDDEN)
                              bitstring_setbit(entry->forwardallforbiddenports, 
                                               br_port->port_id - 1);
                            else
                              bitstring_setbit(entry->forwardallstaticports, 
                                               br_port->port_id - 1);
                          }
                      }  
                  }
              }
            entries++; 
           }
        }

    (*table)->entries = entries;

    /* create a table of pointers to the fdbs to be sorted */
    for ( i=0; i<NSM_VLAN_MAX; i++ ) {
      (*table)->sorted[i] = (*table)->entry[i];
    }
    /* sort the vlan static table in descending order to get all vlans to top of
     * table */
    pal_qsort ( (*table)->sorted, NSM_VLAN_MAX,
                sizeof( ((*table)->sorted)[0]), l2_snmp_vlan_descend_cmp );
    /* then sort only valid entries in ascending order */
    pal_qsort ( (*table)->sorted, entries,
                sizeof( ((*table)->sorted)[0]), l2_snmp_vlan_ascend_cmp );

    result = 0;
  } while (PAL_FALSE);

  return result;
}
#endif

#ifdef HAVE_GMRP
/*
 * xstp_snmp_dot1qForwardAllTable():
 */
unsigned char  *
xstp_snmp_dot1qForwardAllTable(struct variable *vp,
                               oid * name,
                               size_t * length,
                               int exact,
                               size_t * var_len, WriteMethod ** write_method,
                               u_int32_t vr_id)
{
  struct nsm_bridge *br;
  int ret;
  u_int32_t index = 0;
  struct ForwardAllEntry *entry;

  /* validate the index */
  ret = l2_snmp_index_get (vp, name, length, &index, exact);
  if (ret < 0)
    return NULL;

  br = nsm_snmp_get_first_bridge ();
  if (br == NULL)
    return NULL;

  /* get the ForwardALL table */
  ret = xstp_snmp_update_forward_all_table ( &forward_all_table, br);
  if (ret < 0)
    return NULL;

  entry = xstp_snmp_forward_lookup ( forward_all_table, index, exact);

  if (entry == NULL){
    return NULL;
  }

  if (!exact)
    {
      l2_snmp_index_set (vp, name, length, entry->vlanid);
    }

  switch (vp->magic) {
  case DOT1QFORWARDALLPORTS:
    XSTP_SNMP_RETURN_STRING (entry->forwardallports, sizeof(entry->forwardallports));
    break;

  case DOT1QFORWARDALLSTATICPORTS:
    *write_method = xstp_snmp_write_dot1qForwardAllTable;
    XSTP_SNMP_RETURN_STRING (entry->forwardallstaticports,
                             sizeof(entry->forwardallstaticports));
    break;

  case DOT1QFORWARDALLFORBIDDENPORTS:
    *write_method = xstp_snmp_write_dot1qForwardAllTable;
    XSTP_SNMP_RETURN_STRING (entry->forwardallforbiddenports,
                             sizeof(entry->forwardallforbiddenports));
    break;
  default:
    break;
  }
  return NULL;
}
#endif

#ifdef HAVE_GMRP
int
xstp_snmp_update_forward_unregistered_table( struct ForwardUnregisteredTable **table, struct nsm_bridge *br)
{
  int i;
  u_char flags;
  int result = -1;
  int entries = 0;
  u_char is_gmrp_enabled;
  struct nsm_if *zif = NULL;
  struct listnode *ln = NULL;
  struct avl_node *rn = NULL;
  u_char is_gmrp_enabled_port;
  struct interface *ifp = NULL;
  struct nsm_vlan *vlan = NULL;
  struct nsm_master *master = NULL;
  struct nsm_bridge_port *br_port = NULL;
  struct ForwardUnregisteredEntry *entry;
  struct nsm_bridge_master *bridge_master = NULL;
  struct apn_vr *vr = apn_vr_get_privileged (snmpg);

  if (! vr)
    return result;

  master = vr->proto;

  if (! master)
    return result;

  bridge_master = master->bridge;

  if (! bridge_master)
    return result;


  do {                            /* once thru loop */
    /* if the local table hasn't been allocated, try to do it now. */
    if ( *table == NULL )
      *table = XCALLOC (MTYPE_L2_SNMP_FDB, sizeof (struct ForwardUnregisteredTable) );
    if ( *table == NULL )
      break;

   if ((!br->gmrp_bridge) ) {
      break;
   }

   is_gmrp_enabled = PAL_TRUE;

   if (!br->vlan_table)
     return result;

   for (rn = avl_top (br->vlan_table); rn; rn = avl_next (rn))
     {
        if ( (vlan = rn->info))
          {
            if (((*table)->entry[vlan->vid]) != NULL)
              XFREE(MTYPE_L2_SNMP_FDB, ((*table)->entry[vlan->vid]));
            
            (*table)->entry[vlan->vid] = XCALLOC (MTYPE_L2_SNMP_FDB, 
                                             sizeof (struct ForwardAllEntry));
            if ( (*table)->entry[vlan->vid] == NULL )
              break;
            entry = (*table)->entry[vlan->vid];

            entry->vlanid = vlan->vid;

            LIST_LOOP(vlan->port_list, ifp, ln)
              {

                if ((zif = ifp->info) == NULL
                    || (br_port = zif->switchport) == NULL)
                  continue;

                if (gmrp_get_port_details (bridge_master, ifp, &flags) == RESULT_OK)
                  {
                    is_gmrp_enabled_port = flags & GMRP_MGMT_PORT_CONFIGURED;

                    if (is_gmrp_enabled_port)
                      {
                         if ((flags & GMRP_MGMT_FORWARD_UNREGISTERED_CONFIGURED))
                           {
                             bitstring_setbit
                                     (entry->forwardunregisteredstaticports,
                                      br_port->port_id - 1);
                             bitstring_setbit
                                    (entry->forwardunregisteredports, 
                                     br_port->port_id - 1);
                           }
                         else if (flags & GMRP_MGMT_FORWARD_UNREGISTERED_FORBIDDEN)
                           {
                              bitstring_setbit
                                    (entry->forwardunregisteredforbiddenports,
                                     br_port->port_id - 1);
                              bitstring_setbit(entry->forwardunregisteredports,
                                               br_port->port_id - 1);
                           }
                      }
                  }
              }

            entries++;    

          }
     }


    (*table)->entries = entries;

    /* create a table of pointers to the fdbs to be sorted */
    for ( i=0; i<NSM_VLAN_MAX; i++ ) {
      (*table)->sorted[i] = (*table)->entry[i];
    }
    /* sort the vlan static table in descending order to get all vlans to top of
     * table */
    pal_qsort ( (*table)->sorted, NSM_VLAN_MAX,
                sizeof( ((*table)->sorted)[0]), l2_snmp_vlan_descend_cmp );
    /* then sort only valid entries in ascending order */
    pal_qsort ( (*table)->sorted, entries,
                sizeof( ((*table)->sorted)[0]), l2_snmp_vlan_ascend_cmp );

    result = 0;
  } while (PAL_FALSE);

  return result;
}
#endif

/*
 * given a bridge, instance and exact/inexact request, returns a pointer to
 * an xstp_port structure or NULL if none found.
 */
struct ForwardUnregisteredEntry *
xstp_snmp_forward_unregistered_lookup (struct ForwardUnregisteredTable *table, int instance, int exact)
{
  struct ForwardUnregisteredEntry *best = NULL; /* assume none found */
  int i;

  for (i=0; i< table->entries; i++) {
    if (exact) {
      if (table->sorted[i]->vlanid == instance) {
        /* GET request and instance match */
        best = table->sorted[i];
        break;
      }
    }
    else {                     /* GETNEXT request */
      if (instance < table->sorted[i]->vlanid) {
        /* save if this is a better candidate and
         *   continue search
         */
        if (best == NULL)
          best = table->sorted[i];
        else
          if (table->sorted[i]->vlanid < best->vlanid)
            best = table->sorted[i];
      }
    }
  }
  return best;
}

#ifdef HAVE_GMRP
/*
 * xstp_snmp_dot1qForwardUnregisteredTable():
 */
unsigned char  *
xstp_snmp_dot1qForwardUnregisteredTable(struct variable *vp,
                                        oid * name,
                                        size_t * length,
                                        int exact,
                                        size_t * var_len,
                                        WriteMethod ** write_method,
                                        u_int32_t vr_id)
{
  struct nsm_bridge *br;
  int ret;
  u_int32_t index = 0;
  struct ForwardUnregisteredEntry *entry;

  /* validate the index */
  ret = l2_snmp_index_get (vp, name, length, &index, exact);
  if (ret < 0)
    return NULL;

  br = nsm_snmp_get_first_bridge ();
  if (br == NULL)
    return NULL;

  /* get the ForwardALL table */
  ret = xstp_snmp_update_forward_unregistered_table ( &forward_unregistered_table, br);
  if (ret < 0)
    return NULL;

  entry = xstp_snmp_forward_unregistered_lookup ( forward_unregistered_table, index, exact);
  if (entry == NULL){
    return NULL;
  }

  if (!exact)
    {
      l2_snmp_index_set (vp, name, length, entry->vlanid);
    }

  switch (vp->magic) {
  case DOT1QFORWARDUNREGISTEREDPORTS:
    XSTP_SNMP_RETURN_STRING (entry->forwardunregisteredports,
                             sizeof(entry->forwardunregisteredports));
    break;

  case DOT1QFORWARDUNREGISTEREDSTATICPORTS:
    *write_method = xstp_snmp_write_dot1qForwardUnregisteredTable;
    XSTP_SNMP_RETURN_STRING (entry->forwardunregisteredstaticports,
                             sizeof(entry->forwardunregisteredstaticports));
    break;

  case DOT1QFORWARDUNREGISTEREDFORBIDDENPORTS:
    *write_method = xstp_snmp_write_dot1qForwardUnregisteredTable;
    XSTP_SNMP_RETURN_STRING (entry->forwardunregisteredforbiddenports,
                             sizeof(entry->forwardunregisteredforbiddenports));
  default:
    break;
  }
  return NULL;
}
#endif

/* comparison function for sorting VLANs in the L2 vlan static table. Index
 * is the VLANID.  DESCENDING ORDER
 */
int
xstp_snmp_vlan_current_descend_cmp(const void *e1, const void *e2)
{
  struct vlan_current_entry *vp1 = *(struct vlan_current_entry **)e1;
  struct vlan_current_entry *vp2 = *(struct vlan_current_entry **)e2;

  /* if pointers are the same the data is equal */
  if (vp1 == vp2)
    return 0;
  /* only 1 valid pointer then the other is > */
  if (vp1 == NULL)
    return 1;
  if (vp2 == NULL)
    return -1;

  if ( vp1->vlanid < vp2->vlanid )
    return 1;
  else
    if ( vp1->vlanid > vp2->vlanid )
      return -1;
    else
      return 0;
}

/* comparison function for sorting VLANs in the L2 vlan static table. Index
 * is the VLANID.  ASCENDING ORDER
 */
int
xstp_snmp_vlan_current_ascend_cmp(const void *e1, const void *e2)
{
  struct vlan_current_entry *vp1 = *(struct vlan_current_entry **)e1;
  struct vlan_current_entry *vp2 = *(struct vlan_current_entry **)e2;

  /* if pointers are the same the data is equal */
  if (vp1 == vp2)
    return 0;
  /* only 1 valid pointer then the other is > */
  if (vp1 == NULL)
    return 1;
  if (vp2 == NULL)
    return -1;

  if ( vp1->vlanid < vp2->vlanid )
    return -1;
  else
    if ( vp1->vlanid > vp2->vlanid )
      return 1;
    else
      return 0;
}


/*
 * updates vlan Current table.
 * Returns -1 on error, else 0.
 */
int
xstp_snmp_update_vlan_current_table ( struct vlan_current_table **table, struct nsm_bridge *br)
{
  int i;
  int entries = 0;
  struct interface *ifp = NULL;
  struct avl_node *rn = NULL;
  struct nsm_vlan *vlan = NULL;
  struct listnode *ln = NULL;
  struct nsm_if *zif = NULL;
  struct nsm_vlan_port *port = NULL;
  struct nsm_bridge_port *br_port = NULL;


  /* if the local table hasn't been allocated, try to do it now. */
  if ( *table == NULL )
    *table = XCALLOC (MTYPE_L2_SNMP_FDB, sizeof (struct vlan_current_table) );
  if ( *table == NULL )
    return -1;

  if (!br->vlan_table)
    return -1; 

  for (rn = avl_top (br->vlan_table); rn; rn = avl_next (rn))
    {
      if ( (vlan = rn->info))
        {
          if (((*table)->entry[vlan->vid]) != NULL)
            XFREE(MTYPE_L2_SNMP_FDB,((*table)->entry[vlan->vid]));
          
          (*table)->entry[vlan->vid] = XCALLOC (MTYPE_L2_SNMP_FDB,
                                          sizeof (struct vlan_current_entry ));
          if ( (*table)->entry[vlan->vid] == NULL )
            return -1; 

          /* set timemark */
          (*table)->entry[vlan->vid]->timemark = 0;

          /* copy vlanid */
          (*table)->entry[vlan->vid]->vlanid = vlan->vid;
          (*table)->entry[vlan->vid]->fid = vlan->vid;

          LIST_LOOP(vlan->port_list, ifp, ln)
            {
              if ( (zif = (struct nsm_if *)(ifp->info))
                  && (br_port = zif->switchport) )
                {
                    port = &br_port->vlan_port;
                    bitstring_setbit((*table)->entry[vlan->vid]->egressports,
                                      br_port->port_id);

                   if (!NSM_VLAN_BMP_IS_MEMBER(port->egresstaggedBmp, vlan->vid))
                      bitstring_setbit((*table)->entry[vlan->vid]->untaggedports,
                                        br_port->port_id);
                }
            }

            /* set row status */
            (*table)->entry[vlan->vid]->status = vlan->vlan_state;

            /* set free local vlan index */
            (*table)->entry[vlan->vid]->creation_time = vlan->create_time;

            entries++;
        }
    }
  
  (*table)->entries = entries;

  /* create a table of pointers to be sorted */
  for ( i=0; i<NSM_VLAN_MAX; i++ ) {
    (*table)->sorted[i] = (*table)->entry[i];
  }

  /* sort the vlan static table in descending order to get all vlans to top of
   * table */
  pal_qsort ( (*table)->sorted, NSM_VLAN_MAX,
              sizeof( ((*table)->sorted)[0]), xstp_snmp_vlan_current_descend_cmp );
  /* then sort only valid entries in ascending order */
  pal_qsort ( (*table)->sorted, entries,
              sizeof( ((*table)->sorted)[0]), xstp_snmp_vlan_current_ascend_cmp );

  return 0;
}

/*
 * find best vlanid
 */
struct vlan_current_entry *
xstp_snmp_vlan_current_lookup (struct vlan_current_table *v_table, int instance, int exact)
{
  struct vlan_current_entry *best = NULL; /* assume none found */
  int i;

  for (i=0; i< v_table->entries; i++) {
    if (exact) {
      if (v_table->sorted[i]->vlanid == instance) {
        /* GET request and instance match */
        best = v_table->sorted[i];
        break;
      }
    }
    else {                     /* GETNEXT request */
      if (instance < v_table->sorted[i]->vlanid) {
        /* save if this is a better candidate and
         *   continue search
         */
        if (best == NULL)
          best = v_table->sorted[i];
        else
          if (v_table->sorted[i]->vlanid < best->vlanid)
            best = v_table->sorted[i];
      }
    }
  }
  return best;
}

/*
 * xstp_snmp_dot1qVlanCurrentTable():
 */

unsigned char  *
xstp_snmp_dot1qVlanCurrentTable(struct variable *vp,
                                oid * name,
                                size_t * length,
                                int exact,
                                size_t * var_len,
                                WriteMethod ** write_method,
                                u_int32_t vr_id)
{
  struct nsm_bridge *br;
  int ret;
  u_int32_t vlan_index = 0;
  u_int32_t timemark_index = 0;
  struct vlan_current_entry *vcurrent;

  /* validate the index */
  ret = l2_snmp_integer_index_get (vp,name, length, &timemark_index, 
                           &vlan_index, exact);
  if (ret < 0)
    return NULL;

  br = nsm_snmp_get_first_bridge ();

  if (br == NULL)
    return NULL;

  /* get the fdb table */
  ret = xstp_snmp_update_vlan_current_table ( &vlan_current_table, br);

  if (ret < 0)
    return NULL;

  vcurrent = xstp_snmp_vlan_current_lookup ( vlan_current_table, vlan_index, 
                                            exact);
  if (vcurrent == NULL)
    return NULL;

  if (!exact)
    {
      l2_snmp_integer_index_set (vp, name, length, timemark_index,
                                vcurrent->vlanid);
    }

  switch (vp->magic) {
  case DOT1QVLANTIMEMARK:
  case DOT1QVLANINDEX:
    return NULL;
    break;

  case DOT1QVLANFDBID:
    XSTP_SNMP_RETURN_INTEGER(vcurrent->fid);
    break;

  case DOT1QVLANCURRENTEGRESSPORTS:
    XSTP_SNMP_RETURN_STRING(vcurrent->egressports,
                            sizeof(vcurrent->egressports));
    break;

  case DOT1QVLANCURRENTUNTAGGEDPORTS:
    XSTP_SNMP_RETURN_STRING(vcurrent->untaggedports,
                            sizeof(vcurrent->untaggedports));
    break;

  case DOT1QVLANSTATUS:
    XSTP_SNMP_RETURN_INTEGER(vcurrent->status);
    break;

  case DOT1QVLANCREATIONTIME:
    XSTP_SNMP_RETURN_INTEGER(vcurrent->creation_time);
    break;

  default:
    break;
  }
  return NULL;
}

/* comparison function for sorting VLANs in the L2 port vlan table. Index
 * is the PORT.  DESCENDING ORDER
 */
int
xstp_snmp_port_vlan_descend_cmp(const void *e1, const void *e2)
{
  struct PortVlanEntry *vp1 = *(struct PortVlanEntry **)e1;
  struct PortVlanEntry *vp2 = *(struct PortVlanEntry **)e2;

  /* if pointers are the same the data is equal */
  if (vp1 == vp2)
    return 0;
  /* only 1 valid pointer then the other is > */
  if (vp1 == 0)
    return 1;
  if (vp2 == 0)
    return -1;

  if ( vp1->port < vp2->port )
    return 1;
  else
    if ( vp1->port > vp2->port )
      return -1;
    else
      return 0;
}

/* comparison function for sorting VLANs in the L2 port vlan table. Index
 * is the PORT.  ASCENDING ORDER
 */
int
xstp_snmp_port_vlan_ascend_cmp(const void *e1, const void *e2)
{
  struct PortVlanEntry *vp1 = *(struct PortVlanEntry **)e1;
  struct PortVlanEntry *vp2 = *(struct PortVlanEntry **)e2;

  if ( vp1->port < vp2->port )
    return -1;
  else
    if ( vp1->port > vp2->port )
      return 1;
    else
      return 0;
}

/*
 * updates vlan Static table.
 * Returns -1 on error, else 0.
 */
int
xstp_snmp_update_port_vlan_table ( struct PortVlanTable **table, struct nsm_bridge * br)
{
  int i;
  int entries = 0;
  struct nsm_if *zif = NULL;
  struct avl_node *avl_port;
  struct interface *ifp = NULL;
  struct nsm_vlan_port *port = NULL;
  struct nsm_bridge_port *br_port = NULL;
#ifdef HAVE_GVRP
  struct gvrp_port *gvrp_port = NULL;
#endif /* HAVE_GVRP */

  /* if the local table hasn't been allocated, try to do it now. */
  if ( *table == NULL )
    *table = XCALLOC (MTYPE_L2_SNMP_FDB, sizeof (struct PortVlanTable) );

  if ( *table == NULL )
    return -1;

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

       port = &br_port->vlan_port;
#ifdef HAVE_GVRP
       gvrp_port = br_port->gvrp_port;
#endif /* HAVE_GVRP */
       /* copy port */
       (*table)->entry[br_port->port_id].port = br_port->port_id;

       /* copy pvid */
       (*table)->entry[br_port->port_id].pvid = port->pvid;

       /* copy Acceptable frames */
       (*table)->entry[br_port->port_id].acceptable_frame_types =
       ( port->flags & NSM_VLAN_ACCEPTABLE_FRAME_TYPE_TAGGED ) ?
                        ADMIT_ONLYVLANTAGGED_FRAMES : ADMIT_ALL_FRAMES;

       /* copy Ingressing Filtering */
       (*table)->entry[br_port->port_id].ingress_filtering =
       ( port->flags & NSM_VLAN_ENABLE_INGRESS_FILTER ) ? 1:2;

    /* copy Port Gvrp Status */
#ifdef HAVE_GVRP
       (*table)->entry[br_port->port_id].gvrp_status =
                                            gvrp_port ? DOT1DPORTGVRPSTATUS_ENABLED :
                                            DOT1DPORTGVRPSTATUS_DISABLED;

       (*table)->entry[br_port->port_id].gvrp_port = gvrp_port;

#else
      (*table)->entry[br_port->port_id].gvrp_status = DOT1DPORTGVRPSTATUS_DISABLED;
#endif
       /* copy Port Gvrp Failed Registrations */
       /* Not implemented in GVRP at the moment */
       (*table)->entry[br_port->port_id].gvrp_failed_registrations = 0;

       /* copy Port Gvrp Last PDU Origin */
       /* Not implemented in GVRP at the moment */
       (*table)->entry[br_port->port_id].gvrp_pdu_origin =0 /* gvrp_port->gvrp_pdu_origin*/;
       entries++;
    }

  (*table)->entries = entries;

  /* create a table of pointers to be sorted */
  for ( i=0; i < NSM_L2_SNMP_MAX_PORTS; i++ ) {
    (*table)->sorted[i] = &((*table)->entry[i]);
  }

  /* sort the vlan static table in descending order to get all vlans to top of
   * table */
  pal_qsort ( (*table)->sorted, NSM_L2_SNMP_MAX_PORTS,
              sizeof( ((*table)->sorted)[0]), xstp_snmp_vlan_static_descend_cmp );
  /* then sort only valid entries in ascending order */
  pal_qsort ( (*table)->sorted, entries,
              sizeof( ((*table)->sorted)[0]), xstp_snmp_vlan_static_ascend_cmp );

  return 0;
}

/*
 * find best vlanid
 */
struct PortVlanEntry *
xstp_snmp_port_vlan_lookup (struct PortVlanTable *v_table, int instance, int exact)
{
  struct PortVlanEntry *best = NULL; /* assume none found */
  int i;

  for (i=0; i< v_table->entries; i++) {
    if (exact) {
      if (v_table->sorted[i]->port == instance) {
        /* GET request and instance match */
        best = v_table->sorted[i];
        break;
      }
    }
    else {                     /* GETNEXT request */
      if (instance < v_table->sorted[i]->port) {
        /* save if this is a better candidate and
         *   continue search
         */
        if (best == NULL)
          best = v_table->sorted[i];
        else
          if (v_table->sorted[i]->port < best->port)
            best = v_table->sorted[i];
      }
    }
  }
  return best;
}


/*
 * xstp_snmp_dot1qPortVlanTable():
 */
unsigned char  *
xstp_snmp_dot1qPortVlanTable(struct variable *vp,
                             oid * name,
                             size_t * length,
                             int exact,
                             size_t * var_len, WriteMethod ** write_method,
                             u_int32_t vr_id)
{ 
  struct nsm_bridge *br;
  int ret;
  u_int32_t index = 0;
  struct PortVlanEntry *vport;
  struct interface *ifp;
#ifdef HAVE_GVRP
  int retStatus;
#endif /* HAVE_GVRP */
  struct nsm_if *zif;
  struct nsm_bridge_port *br_port;
  u_int8_t null_mac [ETHER_ADDR_LEN];
#ifdef HAVE_GVRP
  struct gvrp_port *gvrp_port = NULL;
#endif /* HAVE_GVRP */

  br_port = NULL;

  pal_mem_set (null_mac, 0, ETHER_ADDR_LEN);
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

  zif = (struct nsm_if *)ifp->info;

  if (zif == NULL || zif->switchport == NULL)
    return NULL;

  br_port = zif->switchport;

#ifdef HAVE_GVRP
  gvrp_port = br_port->gvrp_port;
#endif /* HAVE_GVRP */

  /* get the port vlan table */
  ret = xstp_snmp_update_port_vlan_table ( &port_vlan_table, br);

  if (ret < 0)
    return NULL;

  vport = xstp_snmp_port_vlan_lookup ( port_vlan_table, index, exact);

  if (vport == NULL)
    return NULL;

  if (!exact)
    {
      l2_snmp_index_set (vp, name, length, vport->port);
    }

  switch (vp->magic) 
    {
      case DOT1QPVID:
        *write_method = xstp_snmp_write_dot1qPvid;
         XSTP_SNMP_RETURN_INTEGER(vport->pvid);
         break;

      case DOT1QPORTACCEPTABLEFRAMETYPES:
        *write_method = xstp_snmp_write_dot1qPortVlanTable;
        XSTP_SNMP_RETURN_INTEGER(vport->acceptable_frame_types);
        break;

      case DOT1QPORTINGRESSFILTERING:
        *write_method = xstp_snmp_write_dot1qPortVlanTable;
        XSTP_SNMP_RETURN_INTEGER(vport->ingress_filtering);
        break;

#ifdef HAVE_GVRP
      case DOT1QPORTGVRPSTATUS:
        *write_method = xstp_snmp_write_dot1qPortVlanTable;
        XSTP_SNMP_RETURN_INTEGER(vport->gvrp_status);
        break;

      case DOT1QPORTGVRPFAILEDREGISTRATIONS:
        XSTP_SNMP_RETURN_INTEGER(vport->gvrp_failed_registrations);
        break;

      case DOT1QPORTGVRPLASTPDUORIGIN:
        if (vport != NULL && gvrp_port != NULL)
          {
            if ((ifp->ifindex == gvrp_port->gid_port->ifindex))
            XSTP_SNMP_RETURN_MACADDRESS (ifp->hw_addr);
          }
        else
          XSTP_SNMP_RETURN_STRING (null_mac, 6);
        break;
      case DOT1QPORTRESTRICTEDVLANREGISTRATION:
        *write_method = xstp_snmp_write_dot1qPortVlanTable;
         retStatus  = (vport->gvrp_port
                       && (vport->gvrp_port->registration_mode ==
                           GVRP_VLAN_REGISTRATION_FORBIDDEN)) ?
                           DOT1DPORTRESTRICTEDVLAN_ENABLED :
                           DOT1DPORTRESTRICTEDVLAN_DISABLED;

        XSTP_SNMP_RETURN_INTEGER(retStatus);
        break;
#endif /* HAVE_GVRP */

      default:
        break;
  }

  return NULL;
}

/*
 */
int
xstp_snmp_update_port_vlan_HC_table ( struct port_vlan_HC_table **table, struct nsm_bridge * br)
{
  int i, j;
  struct nsm_if *zif = NULL;
  struct listnode *ln = NULL;
  struct avl_node *rn = NULL;
  struct interface *ifp = NULL;
  struct nsm_vlan *vlan = NULL;
  struct port_vlan_HC_entry *entry = NULL;
  struct nsm_bridge_port *br_port = NULL;

  /* if the local table hasn't been allocated, try to do it now. */
  if ( *table == NULL )
     *table = XCALLOC (MTYPE_L2_SNMP_FDB, sizeof (struct port_vlan_HC_table) );
  if ( *table == NULL )
    return -1;

  (*table)->entries = 0;
  
  if (!br->vlan_table)
     return -1;

  for (rn = avl_top (br->vlan_table); rn; rn = avl_next (rn))
    {
      if ( (vlan = rn->info))
        { 
          if (entry != NULL)
            XFREE(MTYPE_L2_SNMP_FDB, entry);
          
          entry = XCALLOC (MTYPE_L2_SNMP_FDB, 
                           sizeof (struct port_vlan_HC_entry) );

          if (entry == NULL)
            return -1;
          
          (*table)->entry[vlan->vid] = entry;

          LIST_LOOP(vlan->port_list, ifp, ln)
            {
              if ( (zif = (struct nsm_if *)(ifp->info)) != NULL
                   && (br_port = zif->switchport) != NULL )
                {
                  (*table)->entry[vlan->vid]->ports[br_port->port_id] = 
                                                             br_port->port_id;
                  (*table)->entry[vlan->vid]->entries++; 
                }
            }
          (*table)->entry[vlan->vid]->vlanid = vlan->vid;
        }
      (*table)->entries++;
    }

  /* sort on the vlan (row) first */
  /* create a table of pointers to be sorted */
  for ( i=0; i<NSM_VLAN_MAX; i++ ) {
    (*table)->sorted[i] = (*table)->entry[i];
  }

  /* sort the vlan static table in descending order to get all vlans to top of
   * table */
  pal_qsort ( (*table)->sorted, NSM_VLAN_MAX,
              sizeof( ((*table)->sorted)[0]), l2_snmp_port_descend_cmp );
  /* then sort only valid entries in ascending order */
  pal_qsort ( (*table)->sorted, (*table)->entries,
              sizeof( ((*table)->sorted)[0]), l2_snmp_port_ascend_cmp );

  /* sort on the port (column) next) */
  /* create a table of pointers to be sorted */
  for ( i=0; i<(*table)->entries; i++ ) {
    for (j=0; j < NSM_L2_SNMP_MAX_PORTS; j++) {
      if ((*table)->sorted[i])
        (*table)->sorted[i]->sorted[j] = &((*table)->sorted[i]->ports[j]);
    }

    /* sort the vlan static table in descending order to get all vlans to top of
     * table */
    pal_qsort ( (*table)->sorted[i]->sorted, NSM_L2_SNMP_MAX_PORTS,
                sizeof( ((*table)->sorted[0]->sorted)[0]), l2_snmp_port_descend_cmp );
    /* then sort only valid entries in ascending order */
    pal_qsort ( (*table)->sorted[i]->sorted, (*table)->entries,
                sizeof( ((*table)->sorted[0]->sorted[j])[0]), l2_snmp_port_ascend_cmp );

  }

  /* sort the vlan static table in descending order to get all vlans to top of
   * table */
  pal_qsort ( (*table)->sorted, NSM_VLAN_MAX,
              sizeof( ((*table)->sorted)[0]), xstp_snmp_vlan_static_descend_cmp );
  /* then sort only valid entries in ascending order */
  pal_qsort ( (*table)->sorted, (*table)->entries,
              sizeof( ((*table)->sorted)[0]), xstp_snmp_vlan_static_ascend_cmp );

  return 0;
}

/*
 * find best vlanid
 */
struct port_vlan_HC_entry *
xstp_snmp_port_vlan_HCtable_lookup (struct port_vlan_HC_table *table, 
                                    int *port, int *vlan, int exact)
{
  struct port_vlan_HC_entry *best = NULL; /* assume none found */
  int i;

  for (i=0; i< table->entries; i++) {
    if (exact) {
      if (table->sorted[i]->vlanid == *vlan) {
        if ((table->sorted[i]->ports[*port]) == *port) {
          /* GET request and instance match */
          best = table->sorted[i];
        }
        break;
      }
    }
    else {                     /* GETNEXT request */
      if (*vlan < table->sorted[i]->vlanid) {
        /* save if this is a better candidate and
         *   continue search
         */
        if (best == NULL)
          best = table->sorted[i];
        else
          if (table->sorted[i]->vlanid < best->vlanid)
            best = table->sorted[i];
      }
    }
  }

  if (!exact && best)
    {
      *vlan = best->vlanid; 
      for (i = 0; i < NSM_L2_SNMP_MAX_PORTS; i++)
        {
          if (best->ports [i] > *port)
            {
              *port = best->ports [i];
              break;
            }
        }
    }
    
  return best;
}

/*
 * xstp_snmp_dot1qPortVlanHCStatisticsTable():
 *   Handle this table separately from the scalar value case.
 *   The workings of this are basically the same as for xstp_snmp_ above.
 */
unsigned char  *
xstp_snmp_dot1qPortVlanHCStatisticsTable(struct variable *vp,
                                         oid * name,
                                         size_t * length,
                                         int exact,
                                         size_t * var_len,
                                         WriteMethod ** write_method,
                                         u_int32_t vr_id)
{
  struct nsm_bridge *br;
  int ret;
  static u_int32_t port = 0;
  static u_int32_t vlan = 0;
  struct port_vlan_HC_entry *vport;
#ifdef HAVE_HAL
  struct hal_vlan_if_counters if_stats;
#endif /* HAVE_HAL */

 pal_mem_set (&xstp_int64_val, 0, sizeof (ut_int64_t));

#ifdef HAVE_HAL
  pal_mem_set (&if_stats, 0, sizeof (struct hal_vlan_if_counters));
#endif /* HAVE_HAL */

  /* validate the index */
  ret = l2_snmp_port_vlan_index_get (vp, name, length, &port, &vlan, exact);
  if (ret < 0)
    return NULL;

  br = nsm_snmp_get_first_bridge ();
  if (br == NULL)
    return NULL;

  /* get the port vlan table */
  ret = xstp_snmp_update_port_vlan_HC_table (&port_vlan_HC_table, br);
  if (ret < 0)
    return NULL;

  vport = xstp_snmp_port_vlan_HCtable_lookup (port_vlan_HC_table, 
                                            (int *)&port,
                                            (int *)&vlan, 
                                             exact);
  if (vport == NULL)
    return NULL;

#ifdef HAVE_HAL
  hal_vlan_if_get_counters (port,vlan, &if_stats);
#endif /* HAVE_HAL */
  if (!exact)
    {
      l2_snmp_port_vlan_index_set (vp, name, length, port, vlan);
    }

#ifdef HAVE_HAL
  switch (vp->magic) {
  case DOT1QTPVLANPORTHCINFRAMES:
    pal_mem_cpy (&xstp_int64_val, &if_stats.tp_vlan_port_HC_in_frames,
                                               sizeof (ut_int64_t)); 

    XSTP_SNMP_RETURN_INTEGER64 (xstp_int64_val); 
    break;
  case DOT1QTPVLANPORTHCOUTFRAMES:
    pal_mem_cpy (&xstp_int64_val, &if_stats.tp_vlan_port_HC_out_frames,
                                               sizeof (ut_int64_t));
    XSTP_SNMP_RETURN_INTEGER64 (xstp_int64_val);
    break;

  case DOT1QTPVLANPORTHCINDISCARDS:
    pal_mem_cpy (&xstp_int64_val, &if_stats.tp_vlan_port_HC_in_discards,
                                               sizeof (ut_int64_t));
    XSTP_SNMP_RETURN_INTEGER64 (xstp_int64_val);
    break;
#endif /* HAVE_HAL */
  default:
    break;
  }
  return NULL;
}

/*
 * xstp_snmp_dot1qStaticUnicastTable():
 */
unsigned char  *
xstp_snmp_dot1qStaticUnicastTable(struct variable *vp,
                                  oid * name,
                                  size_t * length,
                                  int exact,
                                  size_t * var_len,
                                  WriteMethod ** write_method,
                                  u_int32_t vr_id)
{
  int ret;
  u_int8_t port_id;
  struct fdb_entry *fdb;
  struct mac_addr_and_port key;
  int port;

/*for row-create tables, the write methods need to be set before any returns */

  switch ( vp->magic ) {

  case DOT1QSTATICUNICASTALLOWEDTOGOTO:
    *write_method = xstp_snmp_write_dot1qStaticUnicastAllowedToGoTo;
    break;
    case DOT1QSTATICUNICASTSTATUS:
      *write_method = xstp_snmp_write_dot1qStaticUnicastStatus;
      break;
  default:
    break;
  }

  /* validate the index */
  ret = l2_snmp_mac_addr_int_index_get (vp, name, length, &key.addr,
                                        &port, exact);
  if (ret < 0)
    return NULL;

  /* get a cached fdb static table */
                                     /*XSTP_FDB_TABLE_STATIC_UNICAST,*/

  ret = xstp_snmp_update_fdb_cache (&fdb_static_cache,
                 XSTP_FDB_TABLE_BOTH | XSTP_FDB_TABLE_UNICAST_MAC,
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
      /* if the port index is zero return the first matching MAC address
       * entry if any and ignore the port.
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
    case DOT1QSTATICUNICASTADDRESS:
      break;

    case DOT1QSTATICUNICASTRECEIVEPORT:
      break;

    case DOT1QSTATICUNICASTALLOWEDTOGOTO:
           pal_mem_set( br_ports_bitstring, 0, sizeof(br_ports_bitstring));

      /* find any static FDB entries where forwarding is denied and clear the
       * corresponding bit in the bit string for that port.
       */
      key.port = 0;
      pal_mem_cpy( &key.addr, fdb->mac_addr, sizeof( fdb->mac_addr ) );

      do {
        if ( fdb == NULL ||
             pal_mem_cmp (&key.addr, fdb->mac_addr,
                          sizeof( fdb->mac_addr )) != 0 )
          break;

        port_id = nsm_snmp_bridge_port_get_port_id (fdb->port_no);

        if ( port_id > 0 &&
             port_id <= NSM_L2_SNMP_MAX_PORTS &&
             fdb->is_fwd == 1 )
            bitstring_setbit( br_ports_bitstring, port_id-1);

        fdb = xstp_snmp_fdb_mac_addr_port_lookup (fdb_static_cache, &key, 0 );

      } while ( PAL_TRUE );

      XSTP_SNMP_RETURN_STRING (br_ports_bitstring, sizeof(br_ports_bitstring));
      break;

    case DOT1QSTATICUNICASTSTATUS:
      *write_method = xstp_snmp_write_dot1qStaticUnicastStatus;
      XSTP_SNMP_RETURN_INTEGER (fdb->snmp_status);
      break;

    default:
      pal_assert(0);
    }
  return NULL;
}

struct nsm_vlan *
xstp_snmp_vlan_lookup (struct nsm_bridge *br, 
                       int vlan_index, int exact)
{
  struct nsm_vlan *best = NULL;
  struct avl_node *rn;
  struct nsm_vlan *vlan = NULL;

  for (rn = avl_top (br->vlan_table); rn; rn = avl_next (rn))
     {
       vlan = rn->info;

       if (exact)
         {
           if (vlan->vid == vlan_index)
             {
               /* GET request and instance match */
               best = vlan;
               break;             
             }
         }
       else 
         {
           if (vlan_index < vlan->vid) 
             {
               /* save if this is a better candidate and
                *   continue search  */
                if (best == NULL)
                  best = vlan;
                else if (vlan->vid < best->vid)
                  best = vlan;
             }
         }
     }
   
   return best;
}

/*
 * xstp_snmp_dot1qLearningConstraintsTable():
 */
unsigned char  *
xstp_snmp_dot1qLearningConstraintsTable(struct variable *vp,
                                        oid * name,
                                        size_t * length,
                                        int exact,
                                        size_t *var_len,
                                        WriteMethod ** write_method,
                                        u_int32_t vr_id)
{
  int ret = 0;
  struct nsm_bridge *br = NULL; 
  struct nsm_vlan *vlan = NULL;
  u_int32_t vlan_index = 0;
  u_int32_t set_index = 0;

  /* validate the index */
  ret = l2_snmp_integer_index_get (vp,name, length, &vlan_index, 
                                   &set_index, exact);

  if (ret < 0)
    return NULL;

  br = nsm_snmp_get_first_bridge ();

  if (br == NULL)
    return NULL;

  if (!br->vlan_table)
    return NULL;

  vlan = xstp_snmp_vlan_lookup (br, vlan_index, exact);

  if (vlan == NULL)
    return NULL;
 
  if (!exact)
    {
      l2_snmp_integer_index_set (vp, name, length, vlan->vid, set_index);
    }

  switch (vp->magic) {
  case DOT1QCONSTRAINTVLAN:
  case DOT1QCONSTRAINTSET:
    return NULL;
    break;

  case DOT1QCONSTRAINTTYPE:
    /* Always returning 1 as only the Independent type of filtering 
     * database is supported for vlan learning*/
    XSTP_SNMP_RETURN_INTEGER (1);

  case DOT1QCONSTRAINTSTATUS:
    /* Returning 1 for Active*/
    XSTP_SNMP_RETURN_INTEGER (1);
  default:
    break;
  }
  return NULL;
}


/*
 * updates Fid table.
 * Returns -1 on error, else 0.
 */
int
xstp_snmp_update_fid_table ( struct FidTable **table, struct nsm_bridge *br)
{
  int i = 0;
  int entries = 0;
  struct avl_node *rn = NULL;
  struct nsm_vlan *vlan = NULL;
  int k = 0;

  /* if the local table hasn't been allocated, try to do it now. */
  if ( *table == NULL )
  {
    *table = XCALLOC (MTYPE_L2_SNMP_FDB, sizeof (struct FidTable) );
    /* Initalize the Pointers to NULL: */
    for (k = 0; k < HAL_MAX_VLAN_ID; k++)
    {
      (*table)->entry[i] = NULL;
    }
  }

  if ( *table == NULL )
    return -1;

  if (!br->vlan_table)
    return -1;

  for (rn = avl_top (br->vlan_table); rn; rn = avl_next (rn))
    {
      if ( (vlan = rn->info))
        {
           if ((*table)->entry[vlan->vid] == NULL)
           {
             (*table)->entry[vlan->vid] = 
                                     XCALLOC (MTYPE_L2_SNMP_FDB, sizeof (struct FidEntry) );
           }
           if ( (*table)->entry[vlan->vid] == NULL )
             return -1; 

           (*table)->entry[vlan->vid]->fid = vlan->vid; 

           (*table)->sorted[i++] = (*table)->entry[vlan->vid];

           entries++;

        }
    }

  (*table)->entries = entries;

  /* sort the vlan static table in descending order to get all vlans to top of
   * table */
  pal_qsort ( (*table)->sorted, entries,
              sizeof( ((*table)->sorted)[0]), xstp_snmp_vlan_static_descend_cmp );
  /* then sort only valid entries in ascending order */
  pal_qsort ( (*table)->sorted, entries,
              sizeof( ((*table)->sorted)[0]), xstp_snmp_vlan_static_ascend_cmp );

  return 0;
}

/*
 * find best vlanid
 */
struct FidEntry *
xstp_snmp_fid_lookup (struct FidTable *table, int instance, int exact)
{
  struct FidEntry *best = NULL; /* assume none found */
  int i;

  for (i=0; i< table->entries; i++) {
    if (exact) {
      if (table->sorted[i]->fid == instance) {
        /* GET request and instance match */
        best = table->sorted[i];
        break;
      }
    }
    else {                     /* GETNEXT request */
      if (instance < table->sorted[i]->fid) {
        /* save if this is a better candidate and
         *   continue search
         */
        if (best == NULL)
          best = table->sorted[i];
        else
          if (table->sorted[i]->fid < best->fid)
            best = table->sorted[i];
      }
    }
  }
  return best;
}

/*
 * xstp_snmp_dot1qFdbTable():
 *   Handle this table separately from the scalar value case.
 *   The workings of this are basically the same as for xstp_snmp_ above.
 */
unsigned char  *
xstp_snmp_dot1qFdbTable(struct variable *vp,
                        oid * name,
                        size_t * length,
                        int exact,
                        size_t * var_len,
                        WriteMethod ** write_method,
                        u_int32_t vr_id)
{
  int ret;
  struct FidEntry *entry;
  struct nsm_bridge *br;
  u_int32_t index = 0;

  br = nsm_snmp_get_first_bridge ();
  if (br == NULL)
    return NULL;

  /* validate the index */
  ret = l2_snmp_index_get (vp, name, length, &index, exact);
  if (ret < 0)
    return NULL;
  
  /* get the port vlan table */
  ret = xstp_snmp_update_fid_table ( &fid_table, br);
  if (ret < 0)
    return NULL;

  entry = xstp_snmp_fid_lookup ( fid_table, index, exact);
  if (entry == NULL)
    return NULL;

  if (!exact)
    {
      l2_snmp_index_set (vp, name, length, entry->fid);
    }

  switch (vp->magic) {
  case DOT1QFDBID:
    break;
  case DOT1QFDBDYNAMICCOUNT:
    XSTP_SNMP_RETURN_INTEGER(entry->num_of_entries);
    break;
  default:
    break;
  }
  return NULL;
}

/*
 * xstp_snmp_dot1qTpFdbTable():
 *   This table is similar to the dot1dTpFdbTable
 *   except for the index which in this case is fdb id.
 *
 */
unsigned char  *
xstp_snmp_dot1qTpFdbTable(struct variable *vp,
                          oid * name,
                          size_t * length,
                          int exact,
                          size_t * var_len, WriteMethod ** write_method,
                          u_int32_t vr_id)
{
  int ret;
  struct fdb_entry *fdb;
  struct nsm_bridge *br;
  struct vid_and_mac_addr key;
  int entries;
  short  port_no;
  u_char snmp_status;

  br = nsm_snmp_get_first_bridge ();
  if (br == NULL)
    return NULL;

  /* validate the index */
  ret = l2_snmp_int_mac_addr_index_get (vp, name, length, &key.vid,
                                        &key.addr, exact);
  if (ret < 0)
    return NULL;

  /* get the fdb table */
  ret = xstp_snmp_update_fdb_cache2 ( &fdb_cache,
                                      XSTP_FDB_TABLE_BOTH |
                                      XSTP_FDB_TABLE_UNIQUE_MAC |
                                      XSTP_FDB_TABLE_UNICAST_MAC,
                                      DOT1DTPFDBSTATUS );
  if (ret < 0)
    return NULL;

  if (fdb_cache->entries == 0)
    return NULL;

 /* get the port vlan table */
  ret = xstp_snmp_update_fid_table ( &fid_table, br);
  if (ret < 0)
    return NULL;

  fdb = xstp_snmp_fdb_vid_mac_addr_lookup (fdb_cache, &key, exact);

  if (fdb == NULL)
    return NULL;

  if (!exact)
    {
      l2_snmp_int_mac_addr_index_set (vp, name, length, fdb->vid,
                                   (struct mac_addr *)(fdb->mac_addr));
    }

  entries = fdb_cache->entries;

  port_no = fdb->port_no;

  snmp_status = fdb->snmp_status;

  switch (vp->magic) {
  case DOT1QTPFDBADDRESS:
    return NULL;

  case DOT1QTPFDBPORT:
    XSTP_SNMP_RETURN_INTEGER (port_no);   
    break;

  case DOT1QTPFDBSTATUS:
    XSTP_SNMP_RETURN_INTEGER (snmp_status);    
   break;
  default:
    pal_assert(0);
    break;
  }

  return NULL;
}

#ifdef HAVE_VLAN_CLASS
/*
 *  xstp_snmp_dot1vProtocolGroupTable():
 *   This table is to the 
 *   except for the index which in this case is fdb id.
 *
 */
unsigned char  *
xstp_snmp_dot1vProtocolGroupTable(struct variable *vp,
                                  oid * name,
                                  size_t * length,
                                  int exact,
                                  size_t * var_len, WriteMethod ** write_method,
                                  u_int32_t vr_id)
{
  int ret;
  struct nsm_bridge *br;
  struct nsm_vlanclass_frame_proto key;
  struct nsm_vlanclass_frame_proto entry_key;
  struct hal_vlan_classifier_rule *rule_ptr = NULL;
  struct nsm_vlan_classifier_group *group_ptr = NULL;
  struct nsm_bridge_master *master;
  struct nsm_vlanclass_group_entry *group_temp = NULL;

  br = nsm_snmp_get_first_bridge ();
  if (br == NULL)
    return NULL;

  master = br->master;
  pal_mem_set(&key, 0, sizeof(struct nsm_vlanclass_frame_proto));
  pal_mem_set(&entry_key, 0, sizeof(struct nsm_vlanclass_frame_proto));

  /* validate the index */
  ret = l2_snmp_frame_type_proto_val_index_get(vp, name, length,
                                               key.protocol_value,
                                               &key.frame_type, exact);

  if (ret < 0)
    {
      return NULL;
    }

  xstp_snmp_classifier_rule_lookup(master, &key, &entry_key, exact,
                                   &rule_ptr, &group_temp);
  if (!exact)
    {
      l2_snmp_frame_type_proto_val_index_set(vp, name, length,
                                             entry_key.protocol_value,
                                             entry_key.frame_type);
    }

  switch (vp->magic)
    {
      case DOT1VPROTOCOLTEMPLATEFRAMETYPE:
      case DOT1VPROTOCOLTEMPLATEPROTOCOLVALUE:
        break;
      case DOT1VPROTOCOLGROUPID:
        *write_method = xstp_snmp_write_dot1vProtocolGroupid;
        if (rule_ptr)
          {
            group_ptr = xstp_snmp_find_classifier_group(rule_ptr);
            if (group_ptr)
              XSTP_SNMP_RETURN_INTEGER (group_ptr->group_id);
            else
              return NULL;
          }
        else if(group_temp)
          XSTP_SNMP_RETURN_INTEGER (group_temp->group_id);
        else
          return NULL;
        break;
      case DOT1VPROTOCOLGROUPROWSTATUS:
        *write_method = xstp_snmp_write_dot1vProtocolGroupRowStatus;
        if (rule_ptr)
          XSTP_SNMP_RETURN_INTEGER (rule_ptr->row_status);
        else if(group_temp)
          XSTP_SNMP_RETURN_INTEGER (group_temp->proto_group_row_status);
        else
          return NULL;
        break;

     default:
       pal_assert(0);
       break;
    }
 return NULL;
}

/*
 *   xstp_snmp_dot1vProtocolPortTable():
 *   This table is to the
 *   except for the index which in this case is fdb id.
 *
 */
unsigned char  *
xstp_snmp_dot1vProtocolPortTable(struct variable *vp,
				 oid * name,
				 size_t * length,
				 int exact,
				 size_t * var_len, WriteMethod ** write_method,
				 u_int32_t vr_id)
{
  int ret;
  u_int32_t index = 0;
  u_int32_t group_id = 0;
  struct nsm_bridge_port *br_port = NULL;
  struct nsm_vlan_class_if_group *key_group = NULL;
  struct nsm_vlanclass_port_entry *if_temp_group = NULL;

  /* validate the index */
  ret = l2_snmp_integer_index_get (vp, name,length,
                                   &index, &group_id,
                                   exact);

  if (ret < 0)
    return NULL;

  switch(vp->magic)
    {
      case DOT1VPROTOCOLPORTGROUPVID:
        *write_method = xstp_snmp_write_dot1VProtocolPortGroupVid;
        break;
      case DOT1VPROTOCOLPORTROWSTATUS:
        *write_method = xstp_snmp_write_dot1VProtocolPortRowStatus;
        break;
      default:
        break;

      return NULL;
    }

  /* Get matching rule associated to the given interface */
  ret = xstp_snmp_classifier_if_group_lookup (index, group_id, exact, &br_port, 
                                              &key_group, &if_temp_group);

  if (!exact)
    {
      if (key_group)
        {
          l2_snmp_integer_index_set(vp, name, length,
	                            br_port->port_id, key_group->group_id);
        }
      else if (if_temp_group)
        {
         l2_snmp_integer_index_set(vp, name, length,
	                           br_port->port_id, if_temp_group->group_id);
        }
    }

  /*
   * this is where we do the value assignments for the mib results.
   */

  switch (vp->magic)
    {
      case DOT1VPROTOCOLPORTGROUPID:
        break;
       
      case DOT1VPROTOCOLPORTGROUPVID:
        if (key_group) 
          XSTP_SNMP_RETURN_INTEGER (key_group->group_vid);
        else if(if_temp_group)
          XSTP_SNMP_RETURN_INTEGER (if_temp_group->vlan_id);
        else
          return NULL;
        break;
      case DOT1VPROTOCOLPORTROWSTATUS:
        if(key_group)
          XSTP_SNMP_RETURN_INTEGER (key_group->row_status);
        else if (if_temp_group) 
          XSTP_SNMP_RETURN_INTEGER (if_temp_group->row_status);
        else
          return NULL;
        break;

      default:
        pal_assert(0);
        break;
    }

  return NULL;
}
#endif /* HAVE_VLAN_CLASS */

/***********************************
 * Q Bridge Scalar writes
 ***********************************/
int
xstp_snmp_write_qBridge_scalars(int action,
                                u_char * var_val,
                                u_char var_val_type,
                                size_t var_val_len,
                                u_char * statP, oid * name, size_t name_len,
                                struct variable *v, u_int32_t vr_id)
{
  long intval;
  struct nsm_bridge *br;
  struct apn_vr *vr = apn_vr_get_privileged (snmpg);
  struct nsm_master *master = NULL;
  struct nsm_bridge_master *bridge_master = NULL;

  if (! vr)
    return SNMP_ERR_GENERR;

  master = vr->proto;

  if (! master)
    return SNMP_ERR_GENERR;

  bridge_master = master->bridge;

  if (! bridge_master)
    return SNMP_ERR_GENERR;

  if (var_val_type != ASN_INTEGER)
    return SNMP_ERR_WRONGTYPE;

  if (var_val_len != sizeof (long))
    return SNMP_ERR_WRONGLENGTH;

  pal_mem_cpy (&intval,var_val,4);
  br = nsm_snmp_get_first_bridge();

  if (br == NULL)
    return SNMP_ERR_GENERR;

  switch (v->magic)
    {
#ifdef HAVE_GVRP
    case DOT1QGVRPSTATUS:
      if ((intval < 1) || (intval > 2))
        return SNMP_ERR_BADVALUE;
      if (intval == 2)
        gvrp_disable(bridge_master, br->name);
      else
        gvrp_enable(bridge_master, "gvrp", br->name);
      return SNMP_ERR_NOERROR;
      break;
#endif /* HAVE_GVRP */

    /* Set not allowed */
    case DOT1QCONSTRAINTSETDEFAULT:
      return SNMP_ERR_GENERR;
      break;

    /* Set not allowed */
    case DOT1QCONSTRAINTTYPEDEFAULT:
      return SNMP_ERR_GENERR;
      break;

    default:
      pal_assert (0);
      return SNMP_ERR_GENERR;
      break;
    }

  return SNMP_ERR_NOERROR;
}


#ifdef HAVE_GMRP
int
xstp_snmp_write_dot1qForwardAllTable(int action,
                                     u_char * xstp_snmp_val,
                                     u_char xstp_snmp_val_type,
                                     size_t xstp_snmp_val_len,
                                     u_char * statP,
                                     oid * name, size_t name_len,
                                     struct variable *v, u_int32_t vr_id)
{

   struct interface *ifp;
   u_int32_t ifindex; 
   int index; 
   int size = (int) xstp_snmp_val_len;
   u_char port_bit_array[4];
   u_char flags;
   u_char is_gmrp_enabled_port;
   int cnt1 = 0, cnt = 0;
   struct listnode *ln  =  NULL;
   struct avl_node *rn = NULL;
   struct nsm_vlan *vlan = NULL;
   struct nsm_master *nm = NULL;
   struct nsm_bridge_master *master = NULL;
   struct nsm_bridge *br;
   struct ptree_node *pn = NULL;
   struct nsm_bridge_static_fdb *static_fdb = NULL;
   struct nsm_bridge_fdb_ifinfo *pInfo = NULL;
   struct apn_vr *vr = apn_vr_get_privileged (snmpg);
   s_int32_t ret;
   struct nsm_vlan p;
   u_int32_t vlan_index = 0;
   int exact = 1;
      
   pal_mem_set(port_bit_array, 0, 4);
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

   pal_strcpy (port_bit_array,xstp_snmp_val); 
 
   cnt1 = 0;
 
   switch (v->magic)
     {
     case DOT1QFORWARDALLSTATICPORTS:
 
       br = nsm_snmp_get_first_bridge ();
       if (br == NULL)
         {
          return SNMP_ERR_GENERR;
         }
       if ( nsm_bridge_is_vlan_aware(br->master, br->name) != PAL_TRUE )
         {
          if (!br->static_fdb_list)
             break;
          for (pn = ptree_top(br->static_fdb_list);pn;pn = ptree_next(pn))
            {
             if (! (static_fdb = pn->info) )
                continue;
             pInfo = static_fdb->ifinfo_list;
             while (pInfo)
               {
                ifp = if_lookup_by_index (&vr->ifm,pInfo->ifindex);

                for (index = 0; index <= NSM_L2_SNMP_MAX_PORTS; index++)
                   {
                     ret = nsm_snmp_bridge_port_get_ifindex (index + 1, &ifindex);

                     if (ret != 0)
                       continue;

                     if(bitstring_testbit(port_bit_array,index))
                       {
                         if (pInfo->ifindex == ifindex)
                           {
                              ret = gmrp_set_fwd_all (master, ifp, 
                                                      GID_EVENT_JOIN);
                           }  
                       }
                     else
                       {
                         if (ifp->ifindex == ifindex)
                           {
                             if (gmrp_get_port_details (master, ifp, &flags) == RESULT_OK)
                               {
                                 is_gmrp_enabled_port = flags &
                                               GMRP_MGMT_PORT_CONFIGURED;
                                 if (is_gmrp_enabled_port)
                                   {
                                      if (flags & GMRP_MGMT_FORWARD_ALL_CONFIGURED)
                                        {
                                          ret = gmrp_set_fwd_all (master,
                                                      ifp, GID_EVENT_LEAVE);
                                        }
                                   }
                               }
                           }
                        }

                    }
                pInfo = pInfo->next_if;
                index++;
                if(index >= (size*8))
                   break;
               }
            cnt1 =0;
            }
       } 
       else
        {
          if (!br->vlan_table)
           {
             return SNMP_ERR_GENERR;
            }
          /* get the VID and check the portlist */
          ret = l2_snmp_index_get (v, name, &name_len, &vlan_index,
                                   exact);
          if (ret < 0)
            return SNMP_ERR_GENERR;

          p.vid = vlan_index;
          rn = avl_search(br->vlan_table,&p);
          if (!rn)
            return SNMP_ERR_GENERR;
                                                  
             if ( (vlan = rn->info))
               {
                LIST_LOOP(vlan->port_list, ifp, ln)
                  {
                   for (index = 0; index <= NSM_L2_SNMP_MAX_PORTS; index++)
                      {
                       ret = nsm_snmp_bridge_port_get_ifindex (index + 1,
                                                               &ifindex);

                         if (ret != 0)
                           continue;

                        if (bitstring_testbit(port_bit_array,index))
                          {
                            if (ifp->ifindex == ifindex)
                              {
                                ret = gmrp_set_fwd_all (master, ifp, 
                                                        GID_EVENT_JOIN);
                                if (gmrp_get_port_details (master, ifp, 
                                    &flags) == RESULT_OK)
                                  {
                                    is_gmrp_enabled_port = flags &
                                      GMRP_MGMT_PORT_CONFIGURED;
                                    if (is_gmrp_enabled_port)
                                      {
                                        if (flags &
                                            GMRP_MGMT_FORWARD_ALL_FORBIDDEN)
                                          {
                                            gmrp_set_registration (master, ifp
                                             , GID_EVENT_NORMAL_REGISTRATION);
                                            cnt++;
                                          }
                                      }
                                  }

                                cnt++;
                              }
                          }  
                        else
                          {
                            if (ifp->ifindex == ifindex)
                               {
                                 if (gmrp_get_port_details (master, ifp, 
                                     &flags) == RESULT_OK)
                                   {
                                     is_gmrp_enabled_port = flags &
                                       GMRP_MGMT_PORT_CONFIGURED;
                                     if (is_gmrp_enabled_port)
                                       {
                                         if (flags & 
                                             GMRP_MGMT_FORWARD_ALL_CONFIGURED)
                                           {
                                             ret = gmrp_set_fwd_all (master, 
                                                 ifp, GID_EVENT_LEAVE);
                                             cnt++;
                                           }
                                       }
                                   }
                               }
                          }
                      }
                   index++;
                   if(index >= (size*8))
                     break;
                 }
               cnt1 = 0;
             }
         }
                  
       break;
 
     case DOT1QFORWARDALLFORBIDDENPORTS:
       br = nsm_snmp_get_first_bridge ();
       if (br == NULL)
         {
          return SNMP_ERR_GENERR;
         }
         if (!br->vlan_table)
           {
            return SNMP_ERR_GENERR;
           }
         /* get the VID and check the portlist */
         ret = l2_snmp_index_get (v, name, &name_len, &vlan_index, exact);
         if (ret < 0)
           return SNMP_ERR_GENERR;

         pal_mem_set (&p, 0, sizeof (struct nsm_vlan));

         p.vid = vlan_index;
         rn = avl_search(br->vlan_table,&p);
         if (!rn)
           return SNMP_ERR_GENERR;

         if ( (vlan = rn->info))
           {
             LIST_LOOP(vlan->port_list, ifp, ln)
               {
                 for (index = 0; index <= NSM_L2_SNMP_MAX_PORTS; index++)
                   {
                      ret = nsm_snmp_bridge_port_get_ifindex (index + 1,
                                                             &ifindex);
                     if (ret != 0)
                       continue;

                     if (bitstring_testbit(port_bit_array,index))
                       {
                         if (ifp->ifindex == ifindex)
                           {
                             gmrp_set_registration (master, ifp, 
                                 GID_EVENT_FORBID_REGISTRATION);
                             cnt++;
                           }
                       }
                     else
                       {
                         if (ifp->ifindex == ifindex)
                           {
                             if (gmrp_get_port_details (master, ifp, &flags)
                                 == RESULT_OK)
                               {
                                 is_gmrp_enabled_port = flags &
                                   GMRP_MGMT_PORT_CONFIGURED;
                                 if (is_gmrp_enabled_port)
                                   {
                                     if (flags & 
                                         GMRP_MGMT_FORWARD_ALL_FORBIDDEN)
                                       {
                                         gmrp_set_registration (master, ifp,
                                             GID_EVENT_NORMAL_REGISTRATION);
                                         cnt++;

                                       }
                                   }
                               }
                           }
                       }
                    }
           
                  index++;
                  if(index >= (size*8))
                    break;
                 }
               cnt1=0;
             }
           break;

    default:
      pal_assert (0);
      return SNMP_ERR_GENERR;
      break;
    }

  if (cnt == 0)
    return SNMP_ERR_GENERR;

  return SNMP_ERR_NOERROR;
}

int
xstp_snmp_write_dot1qForwardUnregisteredTable(int action,
                                              u_char * xstp_snmp_val,
                                              u_char xstp_snmp_val_type,
                                              size_t xstp_snmp_val_len,
                                              u_char * statP,
                                              oid * name, size_t name_len,
                                              struct variable *v,
                                              u_int32_t vr_id)
{

   struct interface *ifp;
   struct listnode *ln  =  NULL;
   struct avl_node *rn = NULL;
   struct nsm_vlan *vlan = NULL;
   struct nsm_master *nm = NULL;
   struct nsm_bridge_master *master = NULL;
   struct nsm_bridge *br;
   int ret;
   u_int32_t ifindex;
   u_char flags;
   u_char is_gmrp_enabled_port;
   int size = (int) xstp_snmp_val_len;
   u_char port_bit_array[NSM_L2_SNMP_PORT_ARRAY_SIZE];
   int cnt1 = 0, index = 0;
   struct ptree_node *pn = NULL;
   struct nsm_bridge_static_fdb *static_fdb = NULL;
   struct nsm_bridge_fdb_ifinfo *pInfo = NULL;
   struct apn_vr *vr = apn_vr_get_privileged (snmpg);
   struct nsm_vlan p;
   u_int32_t vlan_index = 0;
   int exact = 1;


   pal_mem_set(port_bit_array, 0, NSM_L2_SNMP_PORT_ARRAY_SIZE);

   if (! vr)
     return SNMP_ERR_GENERR;
 
   nm = vr->proto;
 
   if (! nm)
     return SNMP_ERR_GENERR;
 
   master = nm->bridge;
 
   if (! master)
    return SNMP_ERR_GENERR;
 
   pal_strcpy (port_bit_array,xstp_snmp_val);
 
   cnt1 = 0;
  
    switch (v->magic)
      {
      case DOT1QFORWARDUNREGISTEREDSTATICPORTS:

        br = nsm_snmp_get_first_bridge ();
        if (br == NULL)
          return SNMP_ERR_GENERR;

        if ( nsm_bridge_is_vlan_aware(br->master, br->name) == PAL_FALSE )
          {
            if (!br->static_fdb_list)
              break;
            for (pn = ptree_top(br->static_fdb_list);pn;pn = ptree_next(pn))
              {
                if (! (static_fdb = pn->info) )
                  continue;
                pInfo = static_fdb->ifinfo_list;
                while (pInfo)
                  {
                    ifp = if_lookup_by_index (&vr->ifm,pInfo->ifindex);
                    if(bitstring_testbit(port_bit_array,cnt1))
                      {
                        ret = nsm_snmp_bridge_port_get_ifindex (cnt1 + 1,
                                                    &ifindex);
                        if (ret != 0)
                          continue;
                        if (ifp->ifindex == ifindex)
                          ret = gmrp_set_fwd_unregistered(master,
                              ifp, GID_EVENT_JOIN);
                      }
                    pInfo = pInfo->next_if;
                    cnt1++;
                    if(cnt1 >= (size*8))
                      break;
                  }
              }
            cnt1 = 0;   
          }
        else
          {
            if (!br->vlan_table)
              return SNMP_ERR_GENERR;

            /* get the VID and check the portlist */
            ret = l2_snmp_index_get (v, name, &name_len, &vlan_index, exact);
            if (ret < 0)
              return SNMP_ERR_GENERR;
            pal_mem_set (&p, 0, sizeof (struct nsm_vlan));
            p.vid = vlan_index;
            rn = avl_search(br->vlan_table,&p);
            if (!rn)
              return SNMP_ERR_GENERR;

            if ( (vlan = rn->info))
              {
                LIST_LOOP(vlan->port_list, ifp, ln)
                  {
                    for (index = 0; index <= NSM_L2_SNMP_MAX_PORTS; index++)
                      {
                        ret = nsm_snmp_bridge_port_get_ifindex (index + 1,
                                                                &ifindex);
                        if (ret != 0)
                          continue;

                        if (bitstring_testbit(port_bit_array,index))
                          {
                            if (ifp->ifindex == ifindex)
                              {
                              ret = gmrp_set_fwd_unregistered (master,ifp,
                                  GID_EVENT_JOIN);
                               if (gmrp_get_port_details (master,ifp, &flags)
                                   == RESULT_OK)
                                 {
                                   is_gmrp_enabled_port =  flags &
                                     GMRP_MGMT_PORT_CONFIGURED;
                                   if (is_gmrp_enabled_port)
                                     {
                                       if (flags &
                                       GMRP_MGMT_FORWARD_UNREGISTERED_FORBIDDEN)
                                         {
                                           ret =
                                             gmrp_set_fwd_unregistered_forbid
                                             (master, ifp, GID_EVENT_LEAVE);
                                           cnt1++;
                                         }
                                     }
                                }
                          }                               
                      }
                    else
                      {
                        if (ifp->ifindex == ifindex)
                          {
                            if (gmrp_get_port_details (master,ifp, &flags) 
                                == RESULT_OK)
                              {
                                is_gmrp_enabled_port =  flags & 
                                  GMRP_MGMT_PORT_CONFIGURED;
                                if (is_gmrp_enabled_port)
                                  {
                                    if (flags & 
                                    GMRP_MGMT_FORWARD_UNREGISTERED_CONFIGURED)
                                      {
                                        ret = gmrp_set_fwd_unregistered 
                                          (master, ifp, GID_EVENT_LEAVE);
                                        cnt1++;
                                      }
                                  }
                              }
                          }
                      }
                 }
               cnt1++;
               if (cnt1 >= (size*8))
                 break;

              }
          }
        cnt1 = 0;
      }
    break;
 
   case DOT1QFORWARDUNREGISTEREDFORBIDDENPORTS:
 
       br = nsm_snmp_get_first_bridge ();
       if (br == NULL)
         return SNMP_ERR_GENERR;
 
       if ( nsm_bridge_is_vlan_aware(br->master, br->name) == PAL_FALSE )
         {
           if (!br->static_fdb_list)
             break;
           for (pn = ptree_top(br->static_fdb_list);pn;pn = ptree_next(pn))
             {
               if (! (static_fdb = pn->info) )
                 continue;
               pInfo = static_fdb->ifinfo_list;
               while (pInfo)
                 {
                   ifp = if_lookup_by_index (&vr->ifm,pInfo->ifindex);

                   if(bitstring_testbit (port_bit_array,cnt1))
                     {
                      ret = nsm_snmp_bridge_port_get_ifindex (cnt1 + 1,
                            &ifindex);
                      if (ret != 0)
                        continue;
                      if (ifp->ifindex == ifindex)

                        ret = gmrp_set_fwd_unregistered_forbid (master, ifp,
                            GID_EVENT_FORBID_REGISTRATION);

                     }
                   pInfo = pInfo->next_if;
                   cnt1++;
                   if(cnt1 >= (size*8))
                     break;
                 }
             }
           cnt1 = 0;
         }
       else
         {
           if (!br->vlan_table)
             return SNMP_ERR_GENERR;

           /* get the VID and check the portlist */
           ret = l2_snmp_index_get (v, name, &name_len, &vlan_index, exact);
           if (ret < 0)
             return SNMP_ERR_GENERR;

           pal_mem_set (&p, 0, sizeof (struct nsm_vlan));
           p.vid = vlan_index;
           rn = avl_search(br->vlan_table,&p);
           if (!rn)
             return SNMP_ERR_GENERR;

           if ( (vlan = rn->info))
             {
               LIST_LOOP(vlan->port_list, ifp, ln)
                 {
                   for (index = 0; index <= NSM_L2_SNMP_MAX_PORTS; index++)
                     {
                       ret = nsm_snmp_bridge_port_get_ifindex (index + 1,
                           &ifindex);

                       if (ret != 0)
                         continue;

                       if (bitstring_testbit(port_bit_array,index))
                         {
                           if (ifp->ifindex == ifindex)
                             {
                               ret = gmrp_set_fwd_unregistered_forbid 
                                 (master, ifp, 
                                  GID_EVENT_FORBID_REGISTRATION);
                               if (gmrp_get_port_details (master,ifp, &flags)
                                   == RESULT_OK)
                                 {
                                   is_gmrp_enabled_port =  flags &
                                     GMRP_MGMT_PORT_CONFIGURED;
                                   if (is_gmrp_enabled_port)
                                     {
                                       if (flags &
                                       GMRP_MGMT_FORWARD_UNREGISTERED_CONFIGURED)
                                         {
                                           ret = gmrp_set_fwd_unregistered
                                             (master, ifp, GID_EVENT_LEAVE);
                                           cnt1++;
                                         }
                                     }
                                 }                            
                            
                             }
                         
                         }
                       else
                         {
                            if (ifp->ifindex == ifindex)
                              {
                                if (gmrp_get_port_details (master, ifp,
                                    &flags) == RESULT_OK)
                                  {
                                    is_gmrp_enabled_port = flags &
                                      GMRP_MGMT_PORT_CONFIGURED;
                                    if (is_gmrp_enabled_port)
                                      {
                                        if (flags & 
                                        GMRP_MGMT_FORWARD_UNREGISTERED_CONFIGURED)
                                          {
                                            ret = 
                                              gmrp_set_fwd_unregistered_forbid 
                                              (master, ifp, GID_EVENT_LEAVE);
                                            cnt1++;
                                          }
                                      }
                                  }
                             
                              }
                         
                         }
                    
                     }
                   cnt1++;
                   if(cnt1 >= (size*8))
                     break;  
                 }
             }
         }
       break;
    default:
      pal_assert (0);
      return SNMP_ERR_GENERR;
      break;
    }

  return SNMP_ERR_NOERROR;
}
#endif /* HAVE_GMRP */

int
xstp_snmp_write_dot1qStaticUnicastAllowedToGoTo(int action,
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
  int vid = 1;
  struct nsm_bridge_port *br_port;
  struct apn_vr *vr = apn_vr_get_privileged (snmpg);

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

  /* validate the index */
  ret = l2_snmp_mac_addr_int_index_get (vp, name, &length,
                                        &key.addr, (int *) &key.port, 1);
  if (ret < 0 || key.port != 0)
    return SNMP_ERR_NOSUCHNAME;

  switch (vp->magic)
    {
    case DOT1QSTATICUNICASTALLOWEDTOGOTO:

      if (var_val_len != sizeof(br_ports_bitstring))
        return SNMP_ERR_WRONGLENGTH;

      /* must have the bridge defined */

      br = nsm_snmp_get_first_bridge ();

      if (br == NULL)
        return SNMP_ERR_GENERR;

      /* for each possible port, find a matching ifName if possible and
       * set or clear a "bridge bridge_name address addr deny ifName"
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

          /*if bit is set in string, then address is allowed. remove any discard
           * entry. if not set, add a static entry indicating discard.
           */
          if ((*(char *)(&key.addr) & 0x01) == 0x00) {
            /* make sure its a unicast address */
            if (bitstring_testbit((char *)var_val, (br_port->port_id-1)) == PAL_FALSE) {
              if (br->type == NSM_BRIDGE_TYPE_STP_VLANAWARE)
                hal_l2_del_fdb (br->name, ifp->ifindex,
                                (u_char *)&key.addr, HAL_HW_LENGTH,
                                NSM_VLAN_DEFAULT_VID, NSM_VLAN_DEFAULT_VID,
                                HAL_L2_FDB_STATIC);
              else
                hal_l2_del_fdb (br->name, ifp->ifindex,
                                (u_char *)&key.addr, HAL_HW_LENGTH,
                                NSM_VLAN_DEFAULT_VID, NSM_VLAN_DEFAULT_VID,
                                HAL_L2_FDB_STATIC);
            }
            else {
              if (br->type == NSM_BRIDGE_TYPE_STP_VLANAWARE)
                ret = nsm_bridge_add_mac (master, br->name, ifp, key.addr.addr,
                                          vid, 1, HAL_L2_FDB_STATIC,
                                          PAL_TRUE, CLI_MAC);
              else
                ret = nsm_bridge_add_mac (master, br->name, ifp, key.addr.addr,
                                          vid, 1, HAL_L2_FDB_STATIC,
                                          PAL_TRUE, CLI_MAC);
                if (ret < 0)
                  return SNMP_ERR_GENERR;
                else
                  return SNMP_ERR_NOERROR;
            }
          }
          else
            return SNMP_ERR_GENERR;
        }
      break;

    default:
      pal_assert(0);
      return SNMP_ERR_GENERR;
    }
  return SNMP_ERR_NOERROR;
}

int
xstp_snmp_write_dot1qStaticUnicastStatus(int action,
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
  int vid = 1;
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

  if (var_val_type != ASN_INTEGER)
    return SNMP_ERR_WRONGTYPE;
  if (var_val_len != sizeof (long))
    return SNMP_ERR_WRONGLENGTH;

  pal_mem_cpy(&intval,var_val,4);

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

  if (!temp_static_fdb || !pInfo)
    {
      if (!fdb_static_cache)
        return SNMP_ERR_GENERR;

      if (fdb_static_cache->entries == 0)
        return SNMP_ERR_GENERR;

      fdb = xstp_snmp_fdb_mac_addr_lookup (fdb_static_cache, &key.addr, 1 );

      if (fdb == NULL)
        return SNMP_ERR_GENERR;

      if (intval == fdb->snmp_status)
       return SNMP_ERR_NOERROR;
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

        if (pInfo)
          {
            if (ifp->ifindex != pInfo->ifindex)
              continue;
          }
        else if (fdb)
          {
            if (ifp->ifindex != fdb->port_no)
              continue;
          }

             switch (intval)
              {
                case DOT1DPORTSTATICSTATUS_OTHER:
                  return SNMP_ERR_NOERROR;
                  break;
                /* If the status is INVALID, the MAC entry should be
                   deleted from the Table.
                */
                case DOT1DPORTSTATICSTATUS_INVALID:
                  if (pInfo)
                    ret = nsm_bridge_delete_mac (master, br->name, ifp,
                                                 key.addr.addr, vid, 1,
                                                 HAL_L2_FDB_STATIC,
                                                 PAL_TRUE, CLI_MAC);
                 else
                   ret = hal_l2_del_fdb(br->name, ifp->ifindex,
                                        key.addr.addr,
                                        HAL_HW_LENGTH,
                                        vid, 1, HAL_L2_FDB_DYNAMIC);

                 if( HAL_SUCCESS != ret )
                   return NSM_HAL_FDB_ERR;

                     break;
               /* If the status is PERMANENT or DELETEONRESET,
                  just updated the snmp_status to reflect the user setting.
                  DELETEONRESET is same as a static entry i.e PERMANENT
                */
                 case DOT1DPORTSTATICSTATUS_DELETEONRESET:
                 case DOT1DPORTSTATICSTATUS_PERMANENT:
                 if (fdb)
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
                      ret = nsm_bridge_delete_mac (master, br->name, ifp,
                                                   key.addr.addr, vid, 1,
                                                   HAL_L2_FDB_STATIC,
                                                   PAL_TRUE, CLI_MAC);
                      if (ret != 0)
                        return SNMP_ERR_GENERR;

                       ret = nsm_bridge_add_mac (master, br->name, ifp,
                                                 key.addr.addr, vid, 1,
                                                 HAL_L2_FDB_DYNAMIC,
                                                 PAL_TRUE, CLI_MAC);
                      if (ret != 0)
                         return SNMP_ERR_GENERR;
                        break;

                  default:
                      pal_assert(0);
                      return SNMP_ERR_GENERR;
                   }
                 xstp_snmp_update_fdb_cache (&fdb_static_cache,
                                             XSTP_FDB_TABLE_STATIC_UNICAST,
                                             DOT1DSTATICSTATUS );    
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
                      {
                        temp_static_fdb = static_fdb;
                        break;
                      }
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
                          {
                            temp_static_fdb = static_fdb;
                            break;
                          }
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
xstp_snmp_write_dot1qStaticMulticastTable(int action,
                                          u_char * xstp_snmp_val,
                                          u_char xstp_snmp_val_type,
                                          size_t xstp_snmp_val_len,
                                          u_char * statP,
                                          oid * name, size_t name_len,
                                          struct variable *v, u_int32_t vr_id)
{

  int ret;
  u_char port_bit_array[4];
  int cnt1 = 0,cnt2 = 0, found = PAL_FALSE;
  int port_id = 0;
  int vid;
  struct mac_addr_and_port key;
  struct interface *ifp = NULL;
   struct nsm_bridge_port *br_port = NULL;
  struct nsm_bridge *br;
  struct apn_vr *vr = apn_vr_get_privileged (nzg);
#if 0
  struct ptree_node *pn = NULL;
  struct nsm_bridge_static_fdb *static_fdb = NULL;
  struct nsm_bridge_fdb_ifinfo *pInfo = NULL;
#ifdef HAVE_VLAN
  struct ptree *list = NULL;
  struct avl_node *rn = NULL;
  struct nsm_vlan *vlan = NULL;
#endif /* HAVE_VLAN */
#endif /* 0 */
  struct nsm_master *nm = NULL;
  struct nsm_bridge_master *master = NULL;

  if (xstp_snmp_val_type != ASN_OCTET_STR)
    return SNMP_ERR_WRONGTYPE;

  /* validate the index */
  ret = l2_snmp_int_mac_addr_int_index_get (v, name, &name_len,&vid,
                                  &key.addr, (int *) &key.port,1);

  if (ret < 0)
    return SNMP_ERR_GENERR;

  /* Check if its unicast address */
  if ((*(char *)(&key.addr) & 0x01) == 0x00)
    {
      return SNMP_ERR_GENERR;
    }

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

  pal_mem_set(port_bit_array, 0, 4);
  pal_mem_cpy (port_bit_array,xstp_snmp_val, 4);

  cnt1 = 0;

  switch (v->magic)
    {
      case DOT1QSTATICMULTICASTSTATICEGRESSPORTS:

        /* must have the bridge defined */

        br = nsm_snmp_get_first_bridge();
        if (br == NULL)
          return SNMP_ERR_GENERR;


        for (cnt1 = 0; cnt1 < NSM_L2_SNMP_MAX_PORTS; cnt1++)
          {
            found = PAL_FALSE;
            ifp = nsm_snmp_bridge_port_lookup_by_port_id (br, cnt1 + 1,
                                                          1,   &br_port);

            if (bitstring_testbit(port_bit_array, cnt1) == PAL_FALSE)
              {
                if (ifp == NULL)
                  continue;

                if (nsm_bridge_is_vlan_aware(br->master, br->name) == PAL_TRUE)
                  ret = nsm_bridge_delete_mac (master, br->name, ifp,
                      (u_char *)&key.addr, vid, vid,
                      HAL_L2_FDB_STATIC, PAL_TRUE, CLI_MAC);
                else
                  ret = nsm_bridge_delete_mac (master, br->name, ifp,
                      (u_char *)&key.addr, NSM_VLAN_DEFAULT_VID,
                      NSM_VLAN_DEFAULT_VID, HAL_L2_FDB_STATIC,
                      PAL_TRUE, CLI_MAC);
              }
            else
              {
                if (ifp == NULL)
                  return SNMP_ERR_GENERR;
                /*Check if the fdb entry already exists */
                for ( cnt2=0; cnt2 < (fdb_static_cache)->entries; cnt2++ )
                  {
                    if ((pal_mem_cmp ( key.addr.addr,
                            (fdb_static_cache->fdbs)[cnt2].mac_addr,
                            ETHER_ADDR_LEN) == 0 )
                        && ((fdb_static_cache->fdbs)[cnt2].vid  == vid)
                        && ((fdb_static_cache->fdbs)[cnt2].is_fwd == PAL_TRUE))
                      {

                        port_id = nsm_snmp_bridge_port_get_port_id (
                            (fdb_static_cache->fdbs)[cnt2].port_no);

                        if ((cnt1 + 1) == port_id)
                          {
                            found  = PAL_TRUE;
                            break;

                          }
                      }
                  }
                if (found == PAL_TRUE)
                  continue;
                if (nsm_bridge_is_vlan_aware(br->master, br->name) == PAL_TRUE)
                  ret = nsm_bridge_add_mac (master, br->name, ifp, key.addr.addr,
                      vid, vid, HAL_L2_FDB_STATIC,
                      PAL_TRUE, CLI_MAC);
                else
                  ret = nsm_bridge_add_mac (master, br->name, ifp, key.addr.addr,
                      NSM_VLAN_DEFAULT_VID, NSM_VLAN_DEFAULT_VID,
                      HAL_L2_FDB_STATIC,
                      PAL_TRUE, CLI_MAC);
                if (ret != 0)
                  return SNMP_ERR_GENERR;
              }
          }

#if 0
        if ( nsm_bridge_is_vlan_aware(br->master, br->name) == PAL_FALSE )
          {
            if (!br->static_fdb_list)
               break;

             for (pn = ptree_top(br->static_fdb_list);pn;pn = ptree_next(pn))
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
              if ((static_cnt) == 0)
                 break;
              for (pn = ptree_top(br->static_fdb_list);pn;pn = ptree_next(pn))
                {
                  if (! (static_fdb = pn->info) )
                    continue;
                  pInfo = static_fdb->ifinfo_list;
                  while (pInfo)
                    {
                      if (cnt2 > static_cnt )
                         break;

                       ifc = if_lookup_by_index (&vr->ifm,pInfo->ifindex);

                       if (port_bit_array[cnt1] == PAL_TRUE)
                          {
                            ret = nsm_bridge_add_mac
                                    (br->master, br->name,ifc,
                                     key.addr.addr, NSM_VLAN_DEFAULT_VID,
                                     NSM_VLAN_DEFAULT_VID,
                                     HAL_L2_FDB_STATIC, PAL_TRUE,
                                     CLI_MAC);
                           }

                         switch (ret)
                           {
                             case NSM_BRIDGE_ERR_NOTFOUND:
                               return SNMP_ERR_GENERR;

                              case NSM_BRIDGE_ERR_NOT_BOUND:
                                return SNMP_ERR_GENERR;

                               case NSM_HAL_FDB_ERR:
                                 return SNMP_ERR_GENERR;

                                case NSM_ERR_DUPLICATE_MAC:
                                  return SNMP_ERR_GENERR;

                                default:
                                   break;
                            }
                         pInfo = pInfo->next_if;

                         cnt1++;
                         if (cnt1 >= xstp_snmp_val_len)
                            break;

                         cnt2++;
                      }
                    if (cnt2 > static_cnt )
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

                 if ((static_cnt) == 0)
                   return 0;
                 for (rn = avl_top(br->vlan_table); rn; rn = avl_next (rn))
                   {
                     if (!(vlan = rn->info) || !(list = vlan->static_fdb_list))
                        continue;

                      for (pn = ptree_top (list); pn; pn = ptree_next (pn))
                        {
                          cnt1 = 0;

                       if (! (static_fdb = pn->info) )
                          continue;
                       pInfo = static_fdb->ifinfo_list;
                       while (pInfo)
                         {
                           if (cnt2 > static_cnt )
                              break;

                           ifc = if_lookup_by_index (&vr->ifm,
                                                  pInfo->ifindex);
                           if (bitstring_testbit(port_bit_array,cnt1))
                             {
                               if (ifc->ifindex == (cnt1 + 1))
                                 {
                                    ret = nsm_bridge_add_mac
                                          (br->master,br->name,
                                          ifc, key.addr.addr,
                                          NSM_VLAN_DEFAULT_VID,
                                          NSM_VLAN_DEFAULT_VID,
                                          HAL_L2_FDB_STATIC, PAL_TRUE,
                                          CLI_MAC);
                                 }
                            }
                          cnt1++;
                          if (cnt1 >= xstp_snmp_val_len)
                             break;
                        }
                        switch (ret)
                          {
                            case NSM_BRIDGE_ERR_NOTFOUND:
                              return SNMP_ERR_GENERR;

                            case NSM_BRIDGE_ERR_NOT_BOUND:
                              return SNMP_ERR_GENERR;

                            case NSM_HAL_FDB_ERR:
                              return SNMP_ERR_GENERR;

                             case NSM_ERR_DUPLICATE_MAC:
                               return SNMP_ERR_GENERR;

                             default:
                               break;
                          }
                        pInfo = pInfo->next_if;
                        cnt2++;
                      }

                   if (cnt2 > static_cnt )
                      break;
                 }
                 if (cnt2 > static_cnt )
                    break;
              }

#endif /* HAVE_VLAN */
#endif /* if 0 */

       break;

    case DOT1QSTATICMULTICASTFORBIDDENEGRESSPORTS:

      /* must have the bridge defined */
      br = nsm_snmp_get_first_bridge();
      if (br == NULL)
         return SNMP_ERR_GENERR;

      for (cnt1 = 0; cnt1 < NSM_L2_SNMP_MAX_PORTS; cnt1++)
        {
          found  = PAL_FALSE;

          ifp = nsm_snmp_bridge_port_lookup_by_port_id (br, cnt1 + 1,
              1,   &br_port);

          if (bitstring_testbit(port_bit_array, cnt1) == PAL_FALSE)
            {
              if (ifp == NULL)
                continue;

              if (nsm_bridge_is_vlan_aware(br->master, br->name) == PAL_TRUE)
                ret = nsm_bridge_delete_mac (master, br->name, ifp,
                    (u_char *)&key.addr, vid, vid,
                    HAL_L2_FDB_STATIC, PAL_FALSE, CLI_MAC);
              else
                ret = nsm_bridge_delete_mac (master, br->name, ifp,
                    (u_char *)&key.addr, NSM_VLAN_DEFAULT_VID,
                    NSM_VLAN_DEFAULT_VID, HAL_L2_FDB_STATIC,
                    PAL_FALSE, CLI_MAC);
            }
          else
            {
              if (ifp == NULL)
                return SNMP_ERR_GENERR;

              /*Check if the fdb entry already exists */
              for ( cnt2=0; cnt2 < (fdb_static_cache)->entries; cnt2++ )
                {
                  if ((pal_mem_cmp ( key.addr.addr,
                          (fdb_static_cache->fdbs)[cnt2].mac_addr,
                          ETHER_ADDR_LEN) == 0 )
                      && ((fdb_static_cache->fdbs)[cnt2].vid  == vid)
                      && ((fdb_static_cache->fdbs)[cnt2].is_fwd == PAL_FALSE))
                    {

                      port_id = nsm_snmp_bridge_port_get_port_id (
                          (fdb_static_cache->fdbs)[cnt2].port_no);

                      if ((cnt1 + 1) == port_id)
                        {
                          found  = PAL_TRUE;
                          break;

                        }
                    }
                }

                if (found == PAL_TRUE)
                  continue;


              if (nsm_bridge_is_vlan_aware(br->master, br->name) == PAL_TRUE)
                ret = nsm_bridge_add_mac (master, br->name, ifp, key.addr.addr,
                    vid, vid, HAL_L2_FDB_STATIC,
                    PAL_FALSE, CLI_MAC);
              else
                ret = nsm_bridge_add_mac (master, br->name, ifp, key.addr.addr,
                    NSM_VLAN_DEFAULT_VID, NSM_VLAN_DEFAULT_VID,
                    HAL_L2_FDB_STATIC,
                    PAL_FALSE, CLI_MAC);
              if (ret != 0)
                return SNMP_ERR_GENERR;
            }
        }

#if 0
      if ( nsm_bridge_is_vlan_aware(br->master, br->name) == PAL_FALSE )
        {
          if (!br->static_fdb_list)
             break;

          for (pn = ptree_top(br->static_fdb_list);pn;pn = ptree_next (pn))
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
          if ((static_cnt) == 0)
             break;
          for (pn = ptree_top(br->static_fdb_list);pn;pn = ptree_next(pn))
            {
              if (! (static_fdb = pn->info) )
                continue;
              pInfo = static_fdb->ifinfo_list;
              while (pInfo)
                {
                  if (cnt2 > static_cnt )
                     break;

#ifdef HAVE_GMRP
                  ifc = if_lookup_by_index (&vr->ifm,pInfo->ifindex);

                  if (port_bit_array[cnt1] == PAL_TRUE)
                    {
                      ret = gmrp_set_registration (master, ifc,
                                               GID_EVENT_FORBID_REGISTRATION);
                    }
                  switch (ret)
                    {
                      case NSM_BRIDGE_ERR_NOTFOUND:
                        return SNMP_ERR_GENERR;

                      case NSM_BRIDGE_ERR_NOT_BOUND:
                        return SNMP_ERR_GENERR;

                      case NSM_HAL_FDB_ERR:
                        return SNMP_ERR_GENERR;

                       case NSM_ERR_DUPLICATE_MAC:
                         return SNMP_ERR_GENERR;

                       default:
                          break;
                     }
#endif /* HAVE_GMRP */
                   pInfo = pInfo->next_if;
                   cnt1++;
                   if (cnt1 >= xstp_snmp_val_len)
                      break;

                   cnt2++;
                 }
               if (cnt2 > static_cnt )
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

             if ((static_cnt) == 0)
               return 0;

             for (rn = avl_top(br->vlan_table); rn; rn = avl_next (rn))
               {
                 if (!(vlan = rn->info) || !(list = vlan->static_fdb_list))
                    continue;

                 for (pn = ptree_top (list); pn; pn = ptree_next (pn))
                   {
                     cnt1 = 0;
                     if (! (static_fdb = pn->info) )
                       continue;
                     pInfo = static_fdb->ifinfo_list;
                     while (pInfo)
                       {
                         if (cnt2 > static_cnt )
                            break;
#ifdef HAVE_GMRP
                         ifc = if_lookup_by_index (&vr->ifm,pInfo->ifindex);
                         if (bitstring_testbit(port_bit_array,cnt1))
                           {
                             if (ifc->ifindex == (cnt1 + 1))
                               {
                                 ret = gmrp_set_registration (master, ifc,
                                               GID_EVENT_FORBID_REGISTRATION);
                               }
                            }
#endif /* HAVE_GMRP */
                          cnt1++;
                          if (cnt1 >= xstp_snmp_val_len)
                            break;
                        }
                      if (ret < 0)
                        return SNMP_ERR_GENERR;

                      pInfo = pInfo->next_if;
                      cnt2++;
                   }

                 if (cnt2 > static_cnt )
                   break;
               }
            if (cnt2 > static_cnt )
              break;
          }

#endif /* HAVE_VLAN */
#endif /* 0 */
      break;

    default:
      pal_assert (0);
      return SNMP_ERR_GENERR;
      break;
    }

  return SNMP_ERR_NOERROR;
}

int
xstp_snmp_write_dot1qStaticMulticastStatus(int action,
                                           u_char * xstp_snmp_val,
                                           u_char xstp_snmp_val_type,
                                           size_t xstp_snmp_val_len,
                                           u_char * statP,
                                           oid * name, size_t name_len,
                                           struct variable *v, u_int32_t vr_id)
{
  int ret;
  struct mac_addr_and_port key;
  struct nsm_bridge *br = NULL;
  struct nsm_if *zif = NULL;
  struct nsm_bridge_master *master = NULL;
  struct nsm_bridge_port *br_port;
  struct avl_node *avl_port;
  struct interface *ifp = NULL;
  int intval ;
  int vid;
  int port_id = 0;
  u_char port_bit_array[4];
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

  pal_mem_set (port_bit_array, 0 , 4);


  if (xstp_snmp_val_type != ASN_INTEGER)
    return SNMP_ERR_WRONGTYPE;
  if (xstp_snmp_val_len != sizeof (long))
    return SNMP_ERR_WRONGLENGTH;

  pal_mem_cpy(&intval,xstp_snmp_val,4);

  if ((intval < DOT1DPORTSTATICSTATUS_OTHER) ||
                            (intval > DOT1DPORTSTATICSTATUS_DELETEONTIMEOUT))
    return SNMP_ERR_BADVALUE;

  /* validate the index */
    ret = l2_snmp_int_mac_addr_int_index_get (v, name, &name_len,&vid,
                                              &key.addr, (int *) &key.port,1);

  if (ret < 0 || key.port != 0)
    return SNMP_ERR_NOSUCHNAME;

  br = nsm_snmp_get_first_bridge();

  if (br == NULL)
    return SNMP_ERR_GENERR;

  master  = br->master;

  if (master == NULL)
    return SNMP_ERR_GENERR;
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
              break;
            }
        }
    }
  else
    {
      for (rn = avl_top(br->vlan_table); rn; rn = avl_next (rn))
        {
          if (!(vlan = rn->info) || !(list = vlan->static_fdb_list))
            continue;
          if (vlan->vid != vid)
            continue;
          for (pn = ptree_top (list); pn; pn = ptree_next (pn))
            {
              if (! (static_fdb = pn->info) )
                continue;

              if ((pal_mem_cmp (&key.addr, static_fdb->mac_addr,
                    sizeof( static_fdb->mac_addr )) == 0))
                {
                  temp_static_fdb = static_fdb;
                  list_head = static_fdb->ifinfo_list;

                  if (!(pInfo = list_head))
                    return SNMP_ERR_GENERR;

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

      if (intval == fdb->snmp_status)
        return SNMP_ERR_NOERROR;
    }

  if (pInfo)
    flag = 1;

  while (pInfo)
    {
      port_id = nsm_snmp_bridge_port_get_port_id (pInfo->ifindex);
      bitstring_setbit (port_bit_array, port_id - 1);

      pInfo = pInfo->next_if;
    }


  for (avl_port = avl_top (br->port_tree); avl_port;
                                 avl_port = avl_next (avl_port))
    {
      if ((br_port = (struct nsm_bridge_port *)AVL_NODE_INFO (avl_port))== NULL)
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
           case DOT1DPORTSTATICSTATUS_INVALID:
            port_id = nsm_snmp_bridge_port_get_port_id (ifp->ifindex);
            if (bitstring_testbit (port_bit_array, port_id - 1))
              {
                fdb = xstp_snmp_fdb_mac_addr_lookup (fdb_static_cache, &key.addr, 1 );
                if (flag && fdb)
                  ret = nsm_bridge_delete_mac (master, br->name, ifp,
                                               key.addr.addr, vid, 1,
                                               HAL_L2_FDB_STATIC,
                                               fdb->is_fwd, CLI_MAC);
                else
                  ret = hal_l2_del_fdb(br->name, ifp->ifindex,
                                       key.addr.addr,
                                       HAL_HW_LENGTH,
                                       vid, 1, HAL_L2_FDB_DYNAMIC);

                if( HAL_SUCCESS != ret )
                  return NSM_HAL_FDB_ERR;

              }
            break;
           case DOT1DPORTSTATICSTATUS_PERMANENT:
           case DOT1DPORTSTATICSTATUS_DELETEONRESET:
            if (fdb)
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
                    ret = hal_l2_add_fdb(br->name, ifp->ifindex,
                        key.addr.addr,
                        HAL_HW_LENGTH,
                        vid, 1, HAL_L2_FDB_DYNAMIC,fdb->is_fwd);
                    return SNMP_ERR_GENERR;
                  }
              }
            break;

           case DOT1DPORTSTATICSTATUS_DELETEONTIMEOUT:
             /* If the statis is DELETEONTIMEOUT,
                1) Delete the static entry from NSM/HSL.
                2) Add a Dynamic entry in H/W via NSM.
                3) The specified MAC entry will be added as dynamic entry
                in the H/W and will be deleted after 300 Seconds
                */
/*            port_id = nsm_snmp_bridge_port_get_port_id (ifp->ifindex);
             if (bitstring_testbit (port_bit_array, port_id - 1))
               {
                 ret = nsm_bridge_delete_mac (master, br->name, ifp,
                                              key.addr.addr, vid, 1,
                                              HAL_L2_FDB_STATIC,
                                              PAL_TRUE, CLI_MAC);
                 if (ret != 0)
                   return SNMP_ERR_GENERR;

                 ret = nsm_bridge_add_mac (master, br->name, ifp,
                                           key.addr.addr, vid, 1,
                                           HAL_L2_FDB_DYNAMIC,
                                           PAL_TRUE, CLI_MAC);
                 if (ret != 0)
                   return SNMP_ERR_GENERR;
               }*/
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
      /* Currently a Unicast/Multicast MAC can be added to only one interface.
         Find the static_fdb to
         a) Find the interface to which the MAC is added.
         The ifindex can be used to delete/add MAC in NSM/HSL.
         b) Update the snmp_status field for the MAC. The snmp_status
         represents the dot1qStaticUnicastTable status OID
         */
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
                        {
                          temp_static_fdb = static_fdb;
                          break;
                        }
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
                            {
                              temp_static_fdb = static_fdb;
                              break;
                            }
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
xstp_snmp_write_dot1qVlanStaticTable(int action,
                                    u_char * var_val,
                                    u_char var_val_type,
                                    size_t var_val_len,
                                    u_char * statP, oid * name, size_t name_len,
                                    struct variable *v, u_int32_t vr_id)
{
  u_int32_t vid;
  bool_t new_entry = PAL_FALSE;
  struct VlanStaticEntry *vstatic = NULL;
  struct nsm_bridge *br = NULL;
  struct nsm_vlan *vlan = NULL;
  int set_mask = 0;
  int ret = 0;
  if (var_val_type != ASN_OCTET_STR)
    return SNMP_ERR_WRONGTYPE;

  /* validate the index */
  ret = l2_snmp_index_get (v, name, &name_len, &vid, 1);
  if (ret < 0)
    return SNMP_ERR_GENERR;


  br = nsm_snmp_get_first_bridge ();
  if (br == NULL)
    return SNMP_ERR_GENERR;

  ret = xstp_snmp_update_vlan_static_table (&vlan_static_table, br);

 /*Check if the vlan is dynamic vlan .
  If it is a dynamic vlan, then do not allow set or create and go*/
  vlan = nsm_vlan_find (br, vid);

  if (vlan != NULL)
    {
      if (vlan->type & NSM_VLAN_DYNAMIC)
        return SNMP_ERR_GENERR;
    }

  if (vid == NSM_VLAN_DEFAULT_VID)
    return SNMP_ERR_BADVALUE;

  /* Do not allow an update on entry, if the row status is active */
  if (ret == 0) /* there are some entries in the table */
    {
      vstatic = xstp_snmp_vlan_static_lookup (vlan_static_table, vid, 1);
      if (vstatic == NULL)
        {
          /* In case of create and go, when a row is written then
           * it needs to be created and only then a new vlan can be written */
          if (vstatic_write.vlanid != 0 &&
              vstatic_write.vlanid != vid)
            return SNMP_ERR_GENERR;
          else
            vstatic_write.vlanid = vid;
          new_entry = PAL_TRUE;
        }
    }
  else
    return SNMP_ERR_GENERR;

  switch (v->magic)
    {
    case DOT1QVLANSTATICNAME:

      /* IF the vlan is not a new entry, then modify the vlan name
       * as they write; else write into a temporary structure */
      if (!new_entry)
        {
          ret = nsm_vlan_add (br->master, br->name, var_val, vid, NSM_VLAN_ACTIVE,
                            NSM_VLAN_STATIC | NSM_VLAN_CVLAN);
          return SNMP_ERR_GENERR;
        }
      else
        {
          pal_strcpy (vstatic_write.name, var_val);
        }
      return SNMP_ERR_NOERROR;
      break;

    case DOT1QVLANSTATICEGRESSPORTS:
      set_mask = set_mask | XSTP_SNMP_SET_STATIC_VLAN_EGRESSPORT;
      break;

    case DOT1QVLANFORBIDDENEGRESSPORTS:
      set_mask = set_mask | XSTP_SNMP_SET_STATIC_VLAN_FORBIDDENPORT;
      break;

    case DOT1QVLANSTATICUNTAGGEDPORTS:
      set_mask = set_mask | XSTP_SNMP_SET_STATIC_VLAN_UNTAGGEDPORT;
      break;

    default:
      pal_assert (0);
      return SNMP_ERR_GENERR;
      break;
    }

    ret = xstp_snmp_update_vlan_static_entry (vstatic,br, set_mask,
        (char *) var_val);

  if (ret != RESULT_OK)
    return SNMP_ERR_GENERR;

  return SNMP_ERR_NOERROR;

}

int xstp_snmp_update_vlan_static_entry (struct VlanStaticEntry *vstatic,
                                        struct nsm_bridge *br,
                                        int set_mask, char *var_val)
{
 struct interface *ifp = NULL;
 int index = 0;
 u_int32_t ifindex;
 u_char port_bit_array[4];
 struct avl_node *rn = NULL;
 struct nsm_vlan *vlan = NULL;
 struct nsm_master *nm = NULL;
 struct nsm_bridge_master *master = NULL;
 struct nsm_vlan_port *vlan_port = NULL;
 struct nsm_if *zif = NULL;
 struct nsm_bridge_port *br_port = NULL;
 struct listnode *ln = NULL;
 int ret = 0;
 int vlan_port_config = 0;
 struct apn_vr *vr = apn_vr_get_privileged (snmpg);
 struct nsm_vlan_bmp excludeBmp;

 pal_mem_set(port_bit_array, 0, 4);

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

   if ((set_mask & XSTP_SNMP_SET_STATIC_VLAN_EGRESSPORT) ||
       (set_mask & XSTP_SNMP_SET_STATIC_VLAN_FORBIDDENPORT) ||
       (set_mask & XSTP_SNMP_SET_STATIC_VLAN_UNTAGGEDPORT))
     {

       pal_mem_cpy (port_bit_array, var_val, 4);
       /*Before doing the set operation,
        * this check is done to ensure that only the
        * ports which exists are set otherwise return
        * an error*/
       if (vstatic != NULL)
         {
           for (index = 0; index < NSM_L2_SNMP_MAX_PORTS; index++)
             {

               if (bitstring_testbit (port_bit_array,index))
                 {
                   ret = nsm_snmp_bridge_port_get_ifindex (index + 1,
                         &ifindex);

                   if (ret != 0)
                     return SNMP_ERR_GENERR;

                   ifp = nsm_snmp_bridge_port_lookup_by_port_id
                                    (br, index + 1, PAL_TRUE, &br_port);
                   if (ifp == NULL)
                     return  SNMP_ERR_GENERR;

                   vlan_port = &br_port->vlan_port;

                   if (ifp->ifindex == ifindex)
                     {
                       if ((set_mask & XSTP_SNMP_SET_STATIC_VLAN_FORBIDDENPORT)
                            && ((vlan_port->mode != NSM_VLAN_PORT_MODE_HYBRID) &&
                               (vlan_port->mode != NSM_VLAN_PORT_MODE_TRUNK)))
                         return SNMP_ERR_GENERR;


                      if ((set_mask & XSTP_SNMP_SET_STATIC_VLAN_UNTAGGEDPORT)
                          && (vlan_port->mode != NSM_VLAN_PORT_MODE_ACCESS))
                        return SNMP_ERR_GENERR;
                    }
                 }
             }
         }
     }

  if (set_mask & XSTP_SNMP_SET_STATIC_VLANNAME)
   {
     for (rn = avl_top (br->vlan_table); rn; rn = avl_next (rn))
       {
         if (! (vlan = rn->info))
           continue;

         pal_mem_cpy(vlan->vlan_name, var_val , NSM_VLAN_NAME_MAX);
       }
   }

  if (set_mask & XSTP_SNMP_SET_STATIC_VLAN_EGRESSPORT)
    {
      br = nsm_snmp_get_first_bridge ();
      if (br == NULL)
        {
          return SNMP_ERR_GENERR;
        }
      if (vstatic != NULL)
        {
          if (nsm_bridge_is_vlan_aware (br->master, br->name) == PAL_TRUE )
            {
              for (rn = avl_top (br->vlan_table); rn; rn = avl_next (rn))
                {
                  if ((vlan = rn->info))
                    {
                      if (vlan->vid == vstatic->vlanid)
                        {
                          for (index = 0; index < NSM_L2_SNMP_MAX_PORTS; index++)
                            {
                              if (bitstring_testbit (port_bit_array,index))
                                {
                                  ret = nsm_snmp_bridge_port_get_ifindex (index + 1,
                                      &ifindex);

                                  if (ret != 0)
                                    return SNMP_ERR_GENERR;

                                  ifp =  nsm_snmp_bridge_port_lookup (br, ifindex, 1);
                                  if (ifp && (ifp->ifindex == ifindex))
                                    {
                                      zif = (struct nsm_if *)(ifp->info);

                                      if (!zif || !zif->switchport)
                                        return RESULT_ERROR;

                                      br_port = zif->switchport;
                                      vlan_port = &br_port->vlan_port;

                                      if (vlan_port->mode  ==
                                          NSM_VLAN_PORT_MODE_ACCESS)
                                        ret = nsm_vlan_set_access_port (ifp,
                                            vstatic->vlanid, PAL_TRUE,
                                            PAL_TRUE);
                                      else if ((vlan_port->mode  ==
                                                  NSM_VLAN_PORT_MODE_TRUNK) ||
                                               (vlan_port->mode  ==
                                                  NSM_VLAN_PORT_MODE_HYBRID))
                                        ret = nsm_vlan_add_common_port (ifp,
                                            vlan_port->mode,
                                            vlan_port->mode, vstatic->vlanid,
                                            NSM_VLAN_EGRESS_TAGGED,
                                            PAL_TRUE, PAL_TRUE);
                                      if (ret < 0)
                                        return RESULT_ERROR;

                                    }
                                }
                              else
                                {
                                  ret = nsm_snmp_bridge_port_get_ifindex (index + 1,
                                      &ifindex);

                                  if (ret != 0)
                                    continue;

                                  ifp =  nsm_snmp_bridge_port_lookup (br, ifindex, 1);

                                  if (ifp && (ifp->ifindex == ifindex))
                                    {
                                      zif = (struct nsm_if *)(ifp->info);

                                      if (!zif || !zif->switchport)
                                        return RESULT_ERROR;

                                      br_port = zif->switchport;
                                      vlan_port = &br_port->vlan_port;

                                      if ((vlan_port->mode  ==
                                          NSM_VLAN_PORT_MODE_ACCESS) &&
                                          (vlan_port->pvid == vstatic->vlanid))
                                        {
                                          ret = nsm_vlan_set_access_port (ifp,
                                              NSM_VLAN_DEFAULT_VID, PAL_TRUE,
                                              PAL_TRUE);
                                          if (ret < 0)
                                            return RESULT_ERROR;
                                        }
                                      else if ((vlan_port->mode  ==
                                                  NSM_VLAN_PORT_MODE_TRUNK) ||
                                               (vlan_port->mode  ==
                                                  NSM_VLAN_PORT_MODE_HYBRID))
                                        {
                                          vlan_port_config = vlan_port->config;
                                          ret = nsm_vlan_delete_common_port (ifp,
                                              vlan_port->mode,
                                              vlan_port->mode, vstatic->vlanid,
                                              PAL_TRUE, PAL_TRUE);
                                          if (vlan_port_config == NSM_VLAN_CONFIGURED_ALL)
                                            vlan_port->config = NSM_VLAN_CONFIGURED_ALL;
                                          if (ret < 0)
                                            return RESULT_ERROR;
                                        }

                                    }

                                }
                            }
                        }
                      else
                        continue;
                    }
                }
            }
          else
            return SNMP_ERR_GENERR;
        }
      else
        {
          pal_mem_cpy (vstatic_write.egressports, port_bit_array, 4);
        }

    }
  if (set_mask & XSTP_SNMP_SET_STATIC_VLAN_UNTAGGEDPORT)
    {
      br = nsm_snmp_get_first_bridge ();
      if (br == NULL)
        {
          return SNMP_ERR_GENERR;
        }
      if (vstatic != NULL)
        {
          if ( nsm_bridge_is_vlan_aware (br->master, br->name) == PAL_TRUE )
            {
              for (rn = avl_top (br->vlan_table); rn; rn = avl_next (rn))
                {
                  if ((vlan = rn->info))
                    {
                      if (vlan->vid == vstatic->vlanid)
                        {
                          for (index = 0; index < NSM_L2_SNMP_MAX_PORTS; index++)
                            {
                              if(bitstring_testbit(port_bit_array,index))
                                {
                                  ret = nsm_snmp_bridge_port_get_ifindex (index + 1,
                                      &ifindex);

                                  if (ret != 0)
                                    return SNMP_ERR_GENERR;

                                  ifp =  nsm_snmp_bridge_port_lookup (br,
                                      ifindex, 1);

                                  if (ifp && (ifp->ifindex == ifindex))
                                    {
                                      zif = (struct nsm_if *)(ifp->info);

                                      if (!zif || !zif->switchport)
                                        return RESULT_ERROR;

                                      br_port = zif->switchport;
                                      vlan_port = &br_port->vlan_port;

                                      if ((vlan_port->mode ==
                                          NSM_VLAN_PORT_MODE_ACCESS))
                                        {
                                          ret = nsm_vlan_set_access_port (ifp,
                                              vstatic->vlanid, PAL_TRUE,
                                              PAL_TRUE);
                                          if (ret < 0)
                                            return SNMP_ERR_GENERR;
                                        }
                                      else
                                        return SNMP_ERR_GENERR;
                                    }

                                }
                              else
                                {
                                   ret = nsm_snmp_bridge_port_get_ifindex (index + 1,
                                      &ifindex);

                                  if (ret != 0)
                                    continue;

                                  ifp =  nsm_snmp_bridge_port_lookup (br,
                                      ifindex, 1);

                                  if (ifp && (ifp->ifindex == ifindex))
                                    {
                                      zif = (struct nsm_if *)(ifp->info);

                                      if (!zif || !zif->switchport)
                                        return RESULT_ERROR;

                                      br_port = zif->switchport;
                                      vlan_port = &br_port->vlan_port;

                                      if ((vlan_port->mode ==
                                          NSM_VLAN_PORT_MODE_ACCESS)
                                          && (vstatic->vlanid == vlan_port->pvid))
                                        {
                                          ret = nsm_vlan_set_access_port (ifp,
                                              NSM_VLAN_DEFAULT_VID, PAL_TRUE,
                                              PAL_TRUE);
                                          if (ret != 0)
                                            return SNMP_ERR_GENERR;
                                        }
                                    }
                               }
                            }
                        }
                      else
                        continue;

                    }
                }
            }
        }
      else
        {
          pal_mem_cpy (vstatic_write.untaggedports, var_val, 4);
        }
    }
  if (set_mask & XSTP_SNMP_SET_STATIC_VLAN_FORBIDDENPORT)
    {
      br = nsm_snmp_get_first_bridge ();
      if (br == NULL)
        {
         return SNMP_ERR_GENERR;
        }
      if (vstatic != NULL)
        {
          if (nsm_bridge_is_vlan_aware (br->master, br->name) == PAL_TRUE )
            {
              for (rn = avl_top (br->vlan_table); rn; rn = avl_next (rn))
                {
                  if ((vlan = rn->info))
                    {
                      if (vlan->vid == vstatic->vlanid)
                        {
                          NSM_VLAN_BMP_INIT (excludeBmp);

                          NSM_VLAN_BMP_SET (excludeBmp, vstatic->vlanid);


                          for (index = 0; index < NSM_L2_SNMP_MAX_PORTS; index++)
                            {
                              if (bitstring_testbit (port_bit_array, index))
                                {
                                  ret = nsm_snmp_bridge_port_get_ifindex (index + 1,
                                      &ifindex);

                                  if (ret != 0)
                                    return SNMP_ERR_GENERR;

                                  ifp = nsm_snmp_bridge_port_lookup_by_port_id
                                    (br, index + 1, PAL_TRUE, &br_port);

                                  if (ifp && (ifp->ifindex == ifindex))
                                    {
                                      vlan_port = &br_port->vlan_port;

                                      if ((vlan_port->mode  ==
                                           NSM_VLAN_PORT_MODE_TRUNK) ||
                                          (vlan_port->mode  ==
                                           NSM_VLAN_PORT_MODE_HYBRID))
                                        {
                                          ret = nsm_vlan_add_all_except_vid (
                                              ifp, vlan_port->mode,
                                              vlan_port->mode, &excludeBmp,
                                              NSM_VLAN_EGRESS_TAGGED,
                                              PAL_TRUE, PAL_TRUE);
                                          if (ret != 0)
                                            return SNMP_ERR_GENERR;

                                        }
                                      else
                                        return SNMP_ERR_GENERR;
                                    }
                                }
                              else
                                {
                                  ret = nsm_snmp_bridge_port_get_ifindex (index + 1,
                                      &ifindex);

                                  if (ret != 0)
                                    continue;

                                  ifp = nsm_snmp_bridge_port_lookup_by_port_id
                                    (br, index + 1, PAL_TRUE, &br_port);

                                  if (ifp && (ifp->ifindex == ifindex))
                                    {
                                      vlan_port = &br_port->vlan_port;

                                      if ((vlan_port->mode  ==
                                           NSM_VLAN_PORT_MODE_TRUNK) ||
                                          (vlan_port->mode  ==
                                           NSM_VLAN_PORT_MODE_HYBRID))
                                        {
                                          LIST_LOOP(vlan->forbidden_port_list,
                                              ifp, ln)
                                            {
                                              if (ifp->ifindex == ifindex)
                                                {
                                                  listnode_delete (
                                                  vlan->forbidden_port_list, ifp);
                                                  break;
                                                }
                                              else
                                                continue;
                                            }

                                        }
                                    }

                                }
                            }
                          index++;
                           NSM_VLAN_BMP_UNSET (excludeBmp, vstatic->vlanid);
                        }
                      else
                        continue;
                    }
                }
            }
          else
            return SNMP_ERR_GENERR;
        }
      else
        {
          pal_mem_cpy (vstatic_write.forbiddenports, port_bit_array, 4);
        }
    }
  return RESULT_OK;

}


int
xstp_snmp_write_dot1qVlanStaticRowStatus(int action,
                                         u_char * var_val,
                                         u_char var_val_type,
                                         size_t var_val_len,
                                         u_char * statP, oid * name,
                                         size_t name_len,
                                         struct variable *v, u_int32_t vr_id)
{
  int ret;
  u_int32_t vid;
  struct VlanStaticEntry *vstatic = NULL;
  struct nsm_bridge *br = NULL;
  struct VlanStaticEntry static_entry;
  long intval;

  pal_mem_set (&static_entry, 0, sizeof (struct VlanStaticEntry));

  if (var_val_type != ASN_INTEGER)
    return SNMP_ERR_WRONGTYPE;

  /* validate the index */
  ret = l2_snmp_index_get (v, name, &name_len, &vid, 1);
  if (ret < 0)
    return SNMP_ERR_GENERR;

  pal_mem_cpy(&intval,var_val,4);

  br = nsm_snmp_get_first_bridge ();
  if (br == NULL)
    return SNMP_ERR_GENERR;

  ret = xstp_snmp_update_vlan_static_table (&vlan_static_table, br);

  /* Check if this is an existing entry or a new one */
  if (ret == 0) /* there are some entries in the table */
    vstatic = xstp_snmp_vlan_static_lookup (vlan_static_table, vid, 1);

  switch (intval)
    {
    case XSTP_SNMP_ROW_STATUS_ACTIVE:
      return SNMP_ERR_GENERR;
      break;

    case XSTP_SNMP_ROW_STATUS_NOTINSERVICE:
      return SNMP_ERR_GENERR;
      break;

    case XSTP_SNMP_ROW_STATUS_NOTREADY:
      return SNMP_ERR_BADVALUE;
      break;

    case XSTP_SNMP_ROW_STATUS_CREATEANDGO:

      static_entry.vlanid = vid;
      ret = xstp_snmp_add_static_vlan_db (&static_entry);
      if (ret != RESULT_OK)
        return SNMP_ERR_GENERR;

      break;

    case XSTP_SNMP_ROW_STATUS_CREATEANDWAIT:
      return SNMP_ERR_GENERR;
      break;

    case XSTP_SNMP_ROW_STATUS_DESTROY:
      if (vstatic != NULL)
        {
          if (vstatic->rowstatus != XSTP_SNMP_ROW_STATUS_ACTIVE)
            xstp_snmp_del_vlan_static_table (vstatic);
          else
            {
              ret = xstp_snmp_del_static_vlan_db (br->name, vstatic);
              if (ret != RESULT_OK)
                return SNMP_ERR_GENERR;
            }
        }
      else
        return SNMP_ERR_GENERR;
      break;

    default:
      return SNMP_ERR_BADVALUE;
      break;
    }

  return SNMP_ERR_NOERROR;
}

void xstp_snmp_del_vlan_static_table (struct VlanStaticEntry *vstatic)
{
  int i;
  int total_entries = 0;

  total_entries = vlan_static_table->active_entries +
    vlan_static_table->inactive_entries;

  for (i = 0; i < total_entries; i++)
    {
      if (vlan_static_table->sorted[i]->vlanid == vstatic->vlanid)
        {
          XFREE (MTYPE_L2_SNMP_FDB, (vlan_static_table->sorted[i]));
          vlan_static_table->inactive_entries--;
          break;
        }
    }

  return;

}

int xstp_snmp_del_static_vlan_db (char *name, struct VlanStaticEntry *vstatic)
{

  struct apn_vr *vr = apn_vr_get_privileged (snmpg);
  struct nsm_master *master = NULL;
  struct nsm_bridge_master *bridge_master = NULL;
  int ret = RESULT_OK;
  int i = RESULT_OK;

  if (! vr)
    return RESULT_ERROR;

  master = vr->proto;

  if (! master)
    return RESULT_ERROR;

  bridge_master = master->bridge;

  if (! bridge_master)
    return RESULT_ERROR;

  if (vstatic == NULL)
    return RESULT_ERROR;

  if ((ret = nsm_vlan_delete (bridge_master, name, vstatic->vlanid,
                                NSM_VLAN_STATIC | NSM_VLAN_CVLAN)) != 0)
    {
      return RESULT_ERROR;
    }
  if (ret == RESULT_OK)
    {
      for (i = 0; i < NSM_VLAN_MAX; i++)
        {
          if (vlan_static_table->entry[i])
            {
              if (vlan_static_table->entry[i]->vlanid == vstatic->vlanid)
                {
                  for (i = 0; i < vlan_static_table->active_entries; i++)
                    {
                      if (vlan_static_table->sorted[i]->vlanid ==
                          vstatic->vlanid)
                        {
                          vlan_static_table->sorted[i] = NULL;
                          vstatic = NULL;
                          break;
                        }
                    }

                  XFREE (MTYPE_L2_SNMP_FDB, (vlan_static_table->entry[i]));
                  vlan_static_table->entry[i] = NULL;
                  vlan_static_table->active_entries--;
                  break;
                }
            }
        }
    }
  return RESULT_OK;
}

void xstp_snmp_del_l2_snmp ()
{
  int i;
  int total_forwardall_entries = 0;
  int total_forwardunregistered_entries = 0;
  int total_portvlan_entries = 0;
  int total_fid_entries = 0;
  int total_portvlanHC_entries = 0;
  int total_portvlanstats_entries = 0;
  int total_vlancurrent_entries = 0; 
  struct listnode *node = NULL;
  struct nsm_vlanclass_group_entry *frame_and_proto_entry = NULL; 
  struct nsm_vlanclass_port_entry  *if_group_node = NULL;

  if (forward_all_table != NULL)
    {
      total_forwardall_entries = forward_all_table->entries;
      for (i = 0; i < total_forwardall_entries; i++)
        {
          if (forward_all_table->sorted[i])
            {
              XFREE (MTYPE_L2_SNMP_FDB, (forward_all_table->sorted[i]));
              forward_all_table->sorted[i] = NULL;
              break;
            }
        }
     }

  if (forward_unregistered_table != NULL)
    {
      total_forwardunregistered_entries= forward_unregistered_table->entries;
      for (i = 0; i < total_forwardunregistered_entries; i++)
        {
          if (forward_unregistered_table->sorted[i])
            {
              XFREE (MTYPE_L2_SNMP_FDB,(forward_unregistered_table->sorted[i]));
              forward_unregistered_table->sorted[i] = NULL;
              break;
            }
        }
     }
  if (port_vlan_table != NULL)
    {
      total_portvlan_entries =port_vlan_table->entries;
      for (i = 0; i < total_portvlan_entries; i++)
        {
          if (port_vlan_table)
            {
              XFREE (MTYPE_L2_SNMP_FDB, port_vlan_table);
              port_vlan_table = NULL;
              break;
            }
        }
    }
  if (fid_table != NULL)
    {
      total_fid_entries = fid_table->entries;
      for (i = 0; i < total_fid_entries; i++)
        {
          if (fid_table->sorted[i])
            {
              XFREE (MTYPE_L2_SNMP_FDB, (fid_table->sorted[i]));
              fid_table->sorted[i] = NULL;
              break;
            }
        }
     }
  if (port_vlan_HC_table != NULL)
    {
      total_portvlanHC_entries = port_vlan_HC_table->entries;
      for (i = 0; i < total_portvlanHC_entries; i++)
        {
          if (port_vlan_HC_table->sorted[i])
            {
              XFREE (MTYPE_L2_SNMP_FDB, (port_vlan_HC_table->sorted[i]));
              port_vlan_HC_table->sorted[i] = NULL;
              break;
            }
        }
     }
  if (port_vlan_stats_table != NULL)
    {
      total_portvlanstats_entries = port_vlan_stats_table->entries;
      for (i = 0; i < total_portvlanstats_entries; i++)
        {
          if (port_vlan_stats_table->sorted[i])
            {
              XFREE (MTYPE_L2_SNMP_FDB, (port_vlan_stats_table->sorted[i]));
              port_vlan_stats_table->sorted[i] = NULL;
              break;
            }
         }
     }
  if (vlan_current_table != NULL)
    {
      total_vlancurrent_entries = vlan_current_table->entries;
      for (i = 0; i < total_vlancurrent_entries; i++)
        {
          if (vlan_current_table->sorted[i])
            {
              XFREE (MTYPE_L2_SNMP_FDB, (vlan_current_table->sorted[i]));
              vlan_current_table->sorted[i] = NULL;
              break;
            }
        }
     }

  LIST_LOOP (xstp_snmp_classifier_tmp_frame_proto_list, 
             frame_and_proto_entry, node)
   if (frame_and_proto_entry)
     XFREE (MTYPE_NSM_VLAN_CLASS_TMP_RULE, frame_and_proto_entry);

  list_free (xstp_snmp_classifier_tmp_frame_proto_list);

  LIST_LOOP (xstp_snmp_classifier_tmp_if_group_list, if_group_node, node)
   if (if_group_node)
     XFREE (MTYPE_NSM_VLAN_CLASS_TMP_GROUP, if_group_node);

  list_free (xstp_snmp_classifier_tmp_if_group_list);

  return;

}

int
xstp_snmp_write_dot1qPvid(int action,
                          u_char * xstp_snmp_val,
                          u_char xstp_snmp_val_type,
                          size_t xstp_snmp_val_len,
                          u_char * statP, oid * name, size_t name_len,
                          struct variable *v, u_int32_t vr_id)
{
  int ret;
  u_int32_t pvid;
  u_int32_t ifindex = 0;
  struct nsm_bridge *br;
  struct nsm_if *zif = NULL;
  struct interface *ifp = NULL;
  struct nsm_vlan_port *port = NULL;
  struct nsm_bridge_port *br_port = NULL;

  br_port = NULL;

  /* validate the index */
  ret = l2_snmp_index_get (v, name, &name_len, &ifindex, PAL_TRUE);

  if (ret < 0)
    return SNMP_ERR_GENERR;

  br = nsm_snmp_get_first_bridge ();

  if (br == NULL)
    return SNMP_ERR_GENERR;

  ifp = nsm_snmp_bridge_port_lookup_by_port_id (br, ifindex, PAL_TRUE, &br_port);

  if (ifp == NULL || br_port == NULL)
    return SNMP_ERR_GENERR;

  pal_mem_cpy(&pvid, xstp_snmp_val, sizeof (u_int32_t));

  br = nsm_snmp_get_first_bridge ();

  if (br == NULL)
    return SNMP_ERR_GENERR;

  if (((zif = (struct nsm_if *) (ifp->info)) == NULL)
      || ((br_port = zif->switchport) == NULL)
      || ((port = &br_port->vlan_port) == NULL))
    return SNMP_ERR_GENERR;

  if (nsm_vlan_set_pvid (ifp, port->mode, port->sub_mode, pvid,
                         PAL_TRUE, PAL_TRUE) == 0)
    return SNMP_ERR_NOERROR;
  else
    return SNMP_ERR_GENERR;

  return SNMP_ERR_NOERROR;
}

int
xstp_snmp_write_dot1qPortVlanTable(int action,
                                   u_char * xstp_snmp_val,
                                   u_char xstp_snmp_val_type,
                                   size_t xstp_snmp_val_len,
                                   u_char * statP,
                                   oid * name, size_t name_len,
                                   struct variable *v, u_int32_t vr_id)
{
  int ret;
  u_int32_t value = 0;
  u_int32_t ifindex = 0;
  struct nsm_bridge *br;
  struct nsm_if *zif = NULL;
  struct interface *ifp = NULL;
  struct nsm_vlan_port *port = NULL;
  struct nsm_bridge_port *br_port = NULL;

  br_port = NULL;

  /* validate the index */
  ret = l2_snmp_index_get (v, name, &name_len, &ifindex, PAL_TRUE);

  if (ret < 0)
    return SNMP_ERR_GENERR;

  br = nsm_snmp_get_first_bridge ();

  if (br == NULL)
    return SNMP_ERR_GENERR;

  ifp = nsm_snmp_bridge_port_lookup_by_port_id (br, ifindex, PAL_TRUE, &br_port);

  if (ifp == NULL || br_port == NULL)
    return SNMP_ERR_GENERR;

  pal_mem_cpy(&value, xstp_snmp_val, sizeof (u_int32_t));

  br = nsm_snmp_get_first_bridge ();

  if (br == NULL)
    return SNMP_ERR_GENERR;

  if (((zif = (struct nsm_if *) (ifp->info)) == NULL)
      || ((br_port = zif->switchport) == NULL)
      || ((port = &br_port->vlan_port) == NULL))
    return SNMP_ERR_GENERR;

 switch (v->magic)
    {
      case DOT1QPORTACCEPTABLEFRAMETYPES:

      if (value == NSM_VLAN_ACCEPTABLE_FRAME_TYPE_TAGGED)
        ret = nsm_vlan_set_acceptable_frame_type (ifp, port->mode,
                            NSM_VLAN_ACCEPTABLE_FRAME_TYPE_TAGGED);
      else
        ret = nsm_vlan_set_acceptable_frame_type (ifp, port->mode,
                            ~NSM_VLAN_ACCEPTABLE_FRAME_TYPE_TAGGED);
     if (ret < 0)
       return SNMP_ERR_GENERR;

 
     return SNMP_ERR_NOERROR;
     break;

    case DOT1QPORTINGRESSFILTERING:

      if (value == DOT1DPORTINGRESSFILTER_ENABLED)
        ret = nsm_vlan_set_ingress_filter (ifp, port->mode,
                                           port->sub_mode, PAL_TRUE);
      else if (value == DOT1DPORTINGRESSFILTER_DISABLED)
        ret = nsm_vlan_set_ingress_filter (ifp, port->mode,
                                           port->sub_mode, PAL_FALSE);
      else
        return SNMP_ERR_GENERR;

      if (ret < 0)
        return SNMP_ERR_GENERR;

      return SNMP_ERR_NOERROR;
      break;

#ifdef HAVE_GVRP
    case DOT1QPORTGVRPSTATUS:

      if (value == DOT1DPORTGVRPSTATUS_ENABLED)
        gvrp_enable_port (br->master, ifp);
      else if (value == DOT1DPORTGVRPSTATUS_DISABLED)
        gvrp_disable_port (br->master, ifp);
      else
        return SNMP_ERR_GENERR;

      return SNMP_ERR_NOERROR;
      break;

    case DOT1QPORTRESTRICTEDVLANREGISTRATION:

      if (value == PAL_TRUE)
        ret = gvrp_set_registration (br->master, ifp,
                                     GVRP_VLAN_REGISTRATION_FORBIDDEN);
      else
        ret = gvrp_set_registration (br->master, ifp,
                                     GVRP_VLAN_REGISTRATION_NORMAL);  

      if (ret < 0)
        return SNMP_ERR_GENERR;

      return SNMP_ERR_NOERROR;
      break;
#endif /* HAVE_GVRP */
 
      default:
      pal_assert (0);
      return SNMP_ERR_GENERR;
      break;
    }

  return SNMP_ERR_NOERROR;
}

#ifdef HAVE_VLAN_CLASS
/* Supporting functions for ProtocolPortTable */
int
xstp_snmp_write_dot1VProtocolPortRowStatus(int action,
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
  struct nsm_vlan_class_if_group *if_group_ptr = NULL;
  int intval;
  u_int32_t group_id = 0;
  u_int32_t index = 0;
  struct nsm_bridge_master *master = NULL;
  struct interface *ifp = NULL;
  struct nsm_bridge_port *br_port = NULL;
  struct nsm_vlanclass_port_entry *if_group_temp = NULL;

  if (var_val_type != ASN_INTEGER)
    return SNMP_ERR_WRONGTYPE;

  pal_mem_cpy(&intval, var_val, 4);

  if ((intval < XSTP_SNMP_ROW_STATUS_ACTIVE) ||
      (intval > XSTP_SNMP_ROW_STATUS_DESTROY))
    {
      return SNMP_ERR_BADVALUE;
    }

  /* validate the index */
  ret = l2_snmp_integer_index_get (vp, name, &length,
                                   &index, &group_id, PAL_TRUE);

  if (ret < 0 )
    return SNMP_ERR_NOSUCHNAME;

  br = nsm_snmp_get_first_bridge();
  if (br == NULL)
    return SNMP_ERR_GENERR;

  ifp = nsm_snmp_bridge_rule_port_lookup_by_port_id(br, index,
                                                    PAL_TRUE, &br_port);

  if (!ifp || !br_port)
    return SNMP_ERR_GENERR;

  master = nsm_bridge_get_master(ifp->vr->proto);

  if_group_temp = xstp_snmp_classifier_temp_if_group_lookup(PAL_TRUE, index,
                                                            group_id);

  if_group_ptr = nsm_get_classifier_group_by_id_from_if(ifp, group_id);

  switch (intval)
   {
     case XSTP_SNMP_ROW_STATUS_ACTIVE:
       /* If the entry does not exist previously, we shall not allow
      * the user to directly make it active.
      */
       if (if_group_temp &&
         if_group_temp->row_status == XSTP_SNMP_ROW_STATUS_NOTINSERVICE )
         {
         ret = xstp_snmp_classifier_add_group_if(master, ifp, if_group_temp);
         if (ret < 0)
           return SNMP_ERR_GENERR;

         listnode_delete (xstp_snmp_classifier_tmp_if_group_list,
                            (void *)if_group_temp);
         XFREE (MTYPE_NSM_VLAN_CLASS_TMP_RULE, if_group_temp);
         return SNMP_ERR_NOERROR;
         }
       else
         return SNMP_ERR_GENERR;
       break;

     case XSTP_SNMP_ROW_STATUS_NOTINSERVICE:
       if (if_group_temp && (if_group_temp->row_status ==
           XSTP_SNMP_ROW_STATUS_NOTINSERVICE))
       return SNMP_ERR_NOERROR;

       if (if_group_ptr != NULL)
         {
         xstp_snmp_classifier_temp_add_group_if(master, ifp, if_group_ptr,
                                                group_id, index);
         if (ret < 0)
           return SNMP_ERR_GENERR;

         ret = xstp_snmp_classifier_del_group_from_if(ifp, if_group_ptr);
         if (ret < 0)
           return SNMP_ERR_GENERR;
         }
       break;

     case XSTP_SNMP_ROW_STATUS_NOTREADY:
       return SNMP_ERR_GENERR;
       break;

     case XSTP_SNMP_ROW_STATUS_CREATEANDGO:
       return SNMP_ERR_GENERR;
       break;

     case XSTP_SNMP_ROW_STATUS_CREATEANDWAIT:
       ret = xstp_snmp_classifier_temp_add_group_if(master, ifp,
                                                  if_group_ptr, group_id,
                                                    index);
       if (ret < 0)
       return SNMP_ERR_GENERR;

       break;

     case XSTP_SNMP_ROW_STATUS_DESTROY:

       if (if_group_ptr)
         {
         ret = xstp_snmp_classifier_del_group_from_if(ifp, if_group_ptr);
         if (ret < 0)
           return SNMP_ERR_GENERR;
        }
      else if (if_group_temp)
        {
          listnode_delete(xstp_snmp_classifier_tmp_if_group_list,
                          (void *)if_group_temp);
          XFREE(MTYPE_NSM_VLAN_CLASS_TMP_GROUP, if_group_temp);
        }
       else
       return SNMP_ERR_GENERR;
       break;

     default:
       return SNMP_ERR_GENERR;
       break;
   }

  return SNMP_ERR_NOERROR;
}

int
xstp_snmp_write_dot1VProtocolPortGroupVid (int action,
                                           u_char * var_val,
                                           u_char var_val_type,
                                           size_t var_val_len,
                                           u_char * statP,
                                           oid * name, size_t length,
                                           struct variable *vp,
                                           u_int32_t vr_id)
{
  int ret = 0;
  struct nsm_bridge *br = NULL;
  struct nsm_vlan_classifier_group *group_ptr = NULL;
  struct nsm_vlan_class_if_group *if_group_ptr = NULL;
  int intval;
  u_int32_t group_id = 0;
  u_int32_t index = 0;
  struct nsm_bridge_master *master = NULL;
  struct interface *ifp = NULL;
  struct nsm_bridge_port *br_port = NULL;
  struct nsm_vlanclass_port_entry *if_group_temp = NULL;

  if (var_val_type != ASN_INTEGER)
    return SNMP_ERR_WRONGTYPE;

  /* validate the index */
  ret = l2_snmp_integer_index_get (vp, name, &length,
                                   &index ,&group_id, 1);

  if (ret < 0 )
    return SNMP_ERR_NOSUCHNAME;

  pal_mem_cpy(&intval, var_val, 4);

  if ((intval < MIN_VLAN_ID) || (intval > MAX_VLAN_ID))
     return SNMP_ERR_GENERR;

  br = nsm_snmp_get_first_bridge();
  if (br == NULL)
    return SNMP_ERR_GENERR;

  ifp = nsm_snmp_bridge_rule_port_lookup_by_port_id(br, index,
                                                    PAL_TRUE, &br_port);

  if (!ifp || !br_port)
    return SNMP_ERR_GENERR;

  master = br->master;

  if (!master)
    return SNMP_ERR_GENERR;

  if_group_temp = xstp_snmp_classifier_temp_if_group_lookup(PAL_TRUE, index,
                                                            group_id);

  if_group_ptr = nsm_get_classifier_group_by_id_from_if(ifp, group_id);

  if (!if_group_ptr && !if_group_temp)
    return SNMP_ERR_GENERR;

  if (if_group_ptr)
    {
      if_group_ptr->group_vid = intval;
      group_ptr = nsm_get_classifier_group_by_id(master, group_id);

      if (group_ptr)
        {
          ret = nsm_vlan_apply_vlan_on_rule(group_ptr, intval);
          if (ret < 0)
            {
            return SNMP_ERR_GENERR;
            }
        }
    }
  else if(if_group_temp && !if_group_ptr)
    {
      if_group_temp->vlan_id = intval;
      if_group_temp->row_status = XSTP_SNMP_ROW_STATUS_NOTINSERVICE;
    }
  else
    return SNMP_ERR_GENERR;

  return SNMP_ERR_NOERROR;
}

int
xstp_snmp_write_dot1vProtocolGroupid(int action,
                                     u_char * var_val,
                                     u_char var_val_type,
                                     size_t var_val_len,
                                     u_char * statP,
                                     oid * name, size_t length,
                                     struct variable *vp,
                                     u_int32_t vr_id)
{
  int ret;
  struct nsm_bridge *br = NULL;
  struct nsm_bridge_master *master = NULL;
  struct nsm_vlanclass_frame_proto key;
  struct nsm_vlanclass_frame_proto entry_key;
  struct hal_vlan_classifier_rule *rule_ptr = NULL;
  struct nsm_vlanclass_group_entry *temp_group = NULL;
  struct nsm_vlan_classifier_group *group_ptr = NULL;

  int intval;

  if (var_val_type != ASN_INTEGER)
    return SNMP_ERR_WRONGTYPE;

  /* validate the index */
  ret = l2_snmp_frame_type_proto_val_index_get (vp, name, &length,
                                                key.protocol_value,
                                                &key.frame_type, 1);
  if (ret < 0 )
    return SNMP_ERR_NOSUCHNAME;

  pal_mem_cpy(&intval, var_val, 4);
  if (intval < NSM_VLAN_RULE_ID_MIN || intval > NSM_VLAN_GROUP_ID_MAX)
    return SNMP_ERR_BADVALUE;

  br = nsm_snmp_get_first_bridge();
  if (br == NULL)
    return SNMP_ERR_GENERR;

  master = br->master;
  if (!master)
    return SNMP_ERR_GENERR;

  ret = xstp_snmp_classifier_rule_lookup(master, &key, &entry_key,
                                       PAL_TRUE, &rule_ptr, &temp_group);
  if (ret < 0)
    return SNMP_ERR_GENERR;

  if (!rule_ptr && !temp_group)
    return SNMP_ERR_GENERR;

  if (rule_ptr)
    {
      group_ptr = xstp_snmp_find_classifier_group(rule_ptr);
      if (!group_ptr)
        return SNMP_ERR_GENERR;

      group_ptr->group_id = intval;
    }
  else if (temp_group && !rule_ptr)
    {
      temp_group->group_id = intval;
     /* Change rowStatus to Not-in-service */
      temp_group->proto_group_row_status = XSTP_SNMP_ROW_STATUS_NOTINSERVICE;
   }
  else
   {
    return SNMP_ERR_GENERR;
   }

  return SNMP_ERR_NOERROR;
}

/* supporting functions for ProtocolGroup Table */
int
xstp_snmp_write_dot1vProtocolGroupRowStatus (int action,
                                             u_char * var_val,
                                             u_char var_val_type,
                                             size_t var_val_len,
                                             u_char * statP, oid * name,
                                             size_t name_len,
                                             struct variable *v,
                                             u_int32_t vr_id)
{
  int ret;
  struct nsm_bridge *br = NULL;
  struct nsm_bridge_master *master = NULL;
  int intval;
  struct hal_vlan_classifier_rule *rule_ptr = NULL;
  struct nsm_vlanclass_frame_proto key;
  struct nsm_vlanclass_frame_proto entry_key;
  struct nsm_vlanclass_group_entry *group_temp = NULL;
  struct nsm_vlan_classifier_group *group_ptr = NULL;

  if (var_val_type != ASN_INTEGER)
    return SNMP_ERR_WRONGTYPE;

  pal_mem_cpy(&intval, var_val, 4);

  if ((intval < XSTP_SNMP_ROW_STATUS_ACTIVE) ||
      (intval > XSTP_SNMP_ROW_STATUS_DESTROY))
    return SNMP_ERR_BADVALUE;

  /* validate the index */
  ret = l2_snmp_frame_type_proto_val_index_get (v, name, &name_len,
                                                key.protocol_value,
                                                &key.frame_type, 1);
  if (ret < 0)
    return SNMP_ERR_GENERR;

  br = nsm_snmp_get_first_bridge ();
  if (br == NULL)
    return SNMP_ERR_GENERR;

  master = br->master;
  if (!master)
    return SNMP_ERR_GENERR;

  ret = xstp_snmp_classifier_rule_lookup(master, &key, &entry_key,
                                       PAL_TRUE, &rule_ptr, &group_temp);

  switch (intval)
    {
      case XSTP_SNMP_ROW_STATUS_ACTIVE:
        /*
         * If the entry does not exist previously, we shall not allow
         * the user to directly make it active.
         */
        if (rule_ptr)
          {
           group_ptr = xstp_snmp_find_classifier_group(rule_ptr);
           if (group_ptr)
             return SNMP_ERR_NOERROR;
          }
        else if (group_temp)
          {
            if (group_temp->proto_group_row_status ==
                XSTP_SNMP_ROW_STATUS_NOTINSERVICE)
              ret = xstp_snmp_classifier_create_rule(master, &group_temp,
                                                     rule_ptr);
            if (ret < 0)
              return SNMP_ERR_GENERR;

            listnode_delete (xstp_snmp_classifier_tmp_frame_proto_list,
                             (void *)group_temp);
            XFREE (MTYPE_NSM_VLAN_CLASS_TMP_RULE, group_temp);
            return SNMP_ERR_NOERROR;
          }
       else
         return SNMP_ERR_GENERR;

        break;

      case XSTP_SNMP_ROW_STATUS_NOTINSERVICE:
        if (rule_ptr != NULL)
          {
            xstp_snmp_classifier_temp_group_create(master, rule_ptr, &key);
            if (ret < 0)
              return SNMP_ERR_GENERR;

            ret = nsm_del_classifier_rule(master, rule_ptr);
            if (ret < 0)
              return SNMP_ERR_GENERR;

          }
        else if (group_temp && (group_temp->proto_group_row_status ==
                 XSTP_SNMP_ROW_STATUS_NOTINSERVICE))
          return SNMP_ERR_NOERROR;
        else
          return SNMP_ERR_GENERR;

        break;

      case XSTP_SNMP_ROW_STATUS_NOTREADY:
        return SNMP_ERR_BADVALUE;

      case XSTP_SNMP_ROW_STATUS_CREATEANDGO:
        return SNMP_ERR_GENERR;

      case XSTP_SNMP_ROW_STATUS_CREATEANDWAIT:
        /* Entry exists */
        if (rule_ptr || group_temp)
          {
            return SNMP_ERR_GENERR;
          }
        ret = xstp_snmp_classifier_temp_group_create(master, rule_ptr, &key);
        if (ret < 0)
          return SNMP_ERR_GENERR;

        break;

      case XSTP_SNMP_ROW_STATUS_DESTROY:
        if (rule_ptr)
          {
            ret = nsm_del_classifier_rule(master, rule_ptr);
            if (ret != RESULT_OK)
              return SNMP_ERR_GENERR;
          }
        else if (group_temp)
          {
            listnode_delete (xstp_snmp_classifier_tmp_frame_proto_list,
                             (void *)group_temp);
            XFREE (MTYPE_NSM_VLAN_CLASS_TMP_GROUP, group_temp);
          }
        else
          return SNMP_ERR_GENERR;
        break;

      default:
      break;
    }

  return SNMP_ERR_NOERROR;
}


/*  Search for a rule_ptr based on protocol and value. */
int
xstp_snmp_classifier_rule_lookup (struct nsm_bridge_master *master,
                                  struct nsm_vlanclass_frame_proto *key,
                                  struct nsm_vlanclass_frame_proto *search_key,
                                  int exact,
                                  struct hal_vlan_classifier_rule **ret_rule,
                              struct nsm_vlanclass_group_entry **ret_temp_group)
{
  struct avl_node *node = NULL;
  struct hal_vlan_classifier_rule *search_rule = NULL;
  struct hal_vlan_classifier_rule *best_rule = NULL;
  struct nsm_vlanclass_frame_proto best_key;
  struct nsm_vlanclass_frame_proto rule_key;
  struct nsm_vlanclass_group_entry *best_temp_group = NULL;
  int ret = PAL_FALSE;

  /* Make sure bridge is initialized. */
  if ((!master) || (!master->rule_tree))
    return RESULT_ERROR;

  pal_mem_set(&best_key, 0, sizeof(struct nsm_vlanclass_frame_proto));
  pal_mem_set(search_key, 0, sizeof(struct nsm_vlanclass_frame_proto));

  /* Go through the rules to see if the rule already exists */
  for (node = avl_top(master->rule_tree); node; node = avl_next(node))
     {
       search_rule = (struct hal_vlan_classifier_rule *)AVL_NODE_INFO(node);
       if (!search_rule || search_rule->type != HAL_VLAN_CLASSIFIER_PROTOCOL ||
                    !search_rule->group_tree)
         continue;

       pal_mem_set(&rule_key, 0, sizeof(struct nsm_vlanclass_frame_proto));
       ret = xstp_snmp_classifier_nsm_ruledb_to_snmpdb(search_rule->u.protocol.encaps,
                                            search_rule->u.protocol.ether_type,
                                            &rule_key);
       if (ret < 0)
         {
           return RESULT_ERROR;
         }

       /* Check if exactly the same rule already exists. */
       if (exact)
         {
           if (rule_key.frame_type == key->frame_type)
             {
               if (pal_strncmp(key->protocol_value, rule_key.protocol_value,
                               sizeof(key->protocol_value)) == 0)
                 {
                   best_rule = search_rule;
                   break;
                 }
             }
         }
       else
         {
           if ((key->frame_type < rule_key.frame_type) ||
               ((key->frame_type == rule_key.frame_type) &&
               (pal_strcmp(key->protocol_value, rule_key.protocol_value) < 0)))
             {
               if (best_rule == NULL)
                 {
                   best_rule = search_rule;
                   pal_mem_cpy(&best_key, &rule_key,
                               sizeof(struct nsm_vlanclass_frame_proto));
                 }
               else if (rule_key.frame_type <= best_key.frame_type &&
                        (pal_strcmp(rule_key.protocol_value,
                        best_key.protocol_value) <=0 ))
                 {
                 best_rule = search_rule;
                   pal_mem_cpy(&best_key, &rule_key,
                               sizeof(struct nsm_vlanclass_frame_proto));
                 }
             }
         }
     }

  best_temp_group = xstp_snmp_classifier_temp_rule_lookup(exact, key);
  if (exact)
    {
      if (best_rule)
        *ret_rule = best_rule;
      else if (best_temp_group)
        *ret_temp_group = best_temp_group;
      else
        return RESULT_ERROR;
    }
  else
    {
      if (best_rule && best_temp_group)
        {
          if ((best_rule->u.protocol.encaps < best_temp_group->encaps) ||
           (( best_rule->u.protocol.encaps == best_temp_group->encaps) &&
           (best_rule->u.protocol.ether_type <= best_temp_group->ether_type)))
            {
            *ret_rule = best_rule;
          }
          else
          *ret_temp_group = best_temp_group;
        }
      else if (!best_temp_group && best_rule)
        *ret_rule = best_rule;
      else if (!best_rule && best_temp_group)
        *ret_temp_group = best_temp_group;
      else
        return RESULT_ERROR;
    }

  if (*ret_rule)
    {
      xstp_snmp_classifier_nsm_ruledb_to_snmpdb((*ret_rule)->u.protocol.encaps,
                                     (*ret_rule)->u.protocol.ether_type,
                                      search_key);
    }
  else if (*ret_temp_group)
    {
     xstp_snmp_classifier_nsm_ruledb_to_snmpdb((*ret_temp_group)->encaps,
                                    (*ret_temp_group)->ether_type,
                                    search_key);
    }
  else
    return RESULT_ERROR;

  return RESULT_OK;
}

struct nsm_vlan_classifier_group *
xstp_snmp_find_classifier_group (struct hal_vlan_classifier_rule *rule_ptr)
{
   struct avl_node *rnode = NULL;
   struct nsm_vlan_classifier_group *group_ptr = NULL;

   for (rnode = avl_top(rule_ptr->group_tree); rnode; rnode = avl_next(rnode))
   {
     group_ptr =  (struct nsm_vlan_classifier_group *)AVL_NODE_INFO(rnode);
     if (!group_ptr)
       continue;
   }

  return group_ptr;
}


/* Get ether type from Protocol value */
unsigned short
xstp_snmp_classifier_get_ether_type (char *protocol_value)
{
 int index = 0;
 unsigned short ether_type = 0;

 for (index = 0; index < 2 ; index++)
    {
      ether_type = ether_type * 256 + protocol_value[index];
    }

 return ether_type;
}

/* Get protocol value from ether type */
int
xstp_snmp_classifier_get_proto_val (unsigned long ether_type,
                                    char *protocol_value)
{
  int index = 0;
  int array_index = 0;
  int first_no;
  char buffer[5];

  pal_mem_set(buffer, 0, sizeof(buffer));

  for (index = 4; ether_type; index--)
     {
       buffer[index] = ether_type % 16;
       ether_type = (ether_type - ether_type % 16)/16;
     }

  for (index = 0; buffer[index] == 0; index++);

  if (index % 2 == 0)
    {
      first_no = 0;
    }
  else
    {
      first_no = buffer[index];
      index ++;
    }

  for (array_index = 0; index < 5 ; index++)
     {
       protocol_value[array_index] = first_no * 16 + buffer[index];

       if (protocol_value[array_index] == 0)
         continue;

       index++;
       if (index < sizeof(buffer))
         first_no = buffer[index];
       array_index++;
     }

  return 0;
}

/* Convert frame type and protocol value to encaps type and ether type resp*/
int
xstp_snmp_classifier_map_snmpdb2nsm_ruledb (
                                      struct hal_vlan_classifier_rule *rule_ptr,
                                      struct nsm_vlanclass_frame_proto *entry)
{
  int encaps = 0;
  switch (entry->frame_type)
    {
      case APN_CLASS_FRAME_TYPE_ETHER:
        encaps = HAL_VLAN_CLASSIFIER_ETH;
        break;
      case APN_CLASS_FRAME_TYPE_LLCOTHER:
        encaps = HAL_VLAN_CLASSIFIER_SNAP_LLC;
        break;
      case APN_CLASS_FRAME_TYPE_SNAPOTHER:
        encaps = HAL_VLAN_CLASSIFIER_NOSNAP_LLC;
        break;
      default:
        break;
    }

  return encaps;
}

/* Convert ether type to protocol value and encapsulation type to frame type */
int
xstp_snmp_classifier_nsm_ruledb_to_snmpdb (u_int32_t encaps,
                                           u_int16_t ether_type,
                                        struct nsm_vlanclass_frame_proto *entry)
{
  switch(encaps)
    {
      case HAL_VLAN_CLASSIFIER_ETH:
        entry->frame_type = APN_CLASS_FRAME_TYPE_ETHER;
        break;
      case HAL_VLAN_CLASSIFIER_SNAP_LLC:
        entry->frame_type = APN_CLASS_FRAME_TYPE_LLCOTHER;
        break;
      case HAL_VLAN_CLASSIFIER_NOSNAP_LLC:
        entry->frame_type = APN_CLASS_FRAME_TYPE_SNAPOTHER;
        break;
      default:
        return L2_SNMP_FALSE;
    }

  xstp_snmp_classifier_get_proto_val(ether_type, entry->protocol_value);
  return 0;
}

/* Search best rule in the temp table based on frame type and protocol val */
struct nsm_vlanclass_group_entry *
xstp_snmp_classifier_temp_rule_lookup (int exact,
                                       struct nsm_vlanclass_frame_proto *key)
{
  struct nsm_vlanclass_group_entry *temp_group = NULL;
  struct nsm_vlanclass_group_entry *best_temp_group = NULL;
  struct listnode *node = NULL;
  unsigned int encaps = 0;
  unsigned short ether_type = 0;

  encaps = xstp_snmp_classifier_map_snmpdb2nsm_ruledb(NULL, key);
  ether_type = xstp_snmp_classifier_get_ether_type(key->protocol_value);

  LIST_LOOP (xstp_snmp_classifier_tmp_frame_proto_list, temp_group, node)
  {
    if (exact)
      {
        if (temp_group->encaps == encaps &&
            temp_group->ether_type == ether_type)
          {
          best_temp_group = temp_group;
          break;
          }
      }
    else
      {
        if ((encaps < temp_group->encaps) ||
          ((encaps == temp_group->encaps) &&
          (ether_type < temp_group->ether_type)))
          {
          if (best_temp_group == NULL)
            {
              best_temp_group = temp_group;
            }
          else if ((temp_group->encaps < best_temp_group->encaps) ||
                 ((temp_group->encaps == best_temp_group->encaps)&&
                 (temp_group->ether_type < best_temp_group->ether_type)))
            {
              best_temp_group = temp_group;
            }
          }
      }
  }

  return best_temp_group;
}

/* API to store inactive/not ready entries for protocol group table */
int
xstp_snmp_classifier_temp_group_create (struct nsm_bridge_master *nm,
                                        struct hal_vlan_classifier_rule *rule,
                                        struct nsm_vlanclass_frame_proto *key)
{
  struct nsm_vlanclass_group_entry *group_temp = NULL;
  struct nsm_vlan_classifier_group *group_ptr = NULL;

  group_temp = XCALLOC (MTYPE_NSM_VLAN_CLASS_TMP_GROUP,
                        sizeof (struct nsm_vlanclass_group_entry));
  if (!group_temp)
    return RESULT_ERROR;

  if (rule)
    {
      group_temp->type = HAL_VLAN_CLASSIFIER_PROTOCOL;
      group_temp->encaps = xstp_snmp_classifier_map_snmpdb2nsm_ruledb(NULL,
                                                                      key);
      group_temp->ether_type = xstp_snmp_classifier_get_ether_type(
                                                          key->protocol_value);

      group_ptr = xstp_snmp_find_classifier_group(rule);
      group_temp->group_id = group_ptr->group_id;

      group_temp->proto_group_row_status = XSTP_SNMP_ROW_STATUS_NOTINSERVICE;
    }
  else
    {
      group_temp->type = HAL_VLAN_CLASSIFIER_PROTOCOL;
      group_temp->ether_type =
               xstp_snmp_classifier_get_ether_type(key->protocol_value);
      group_temp->encaps = xstp_snmp_classifier_map_snmpdb2nsm_ruledb(NULL,
                                                                      key);
      group_temp->proto_group_row_status = XSTP_SNMP_ROW_STATUS_NOTREADY;

    }

  listnode_add (xstp_snmp_classifier_tmp_frame_proto_list, group_temp);
  return RESULT_OK;

}

/* API to create new protocol vlan classifier rule and update nsm db */
int
xstp_snmp_classifier_create_rule (struct nsm_bridge_master *master,
                                  struct nsm_vlanclass_group_entry **group_temp,
                                  struct hal_vlan_classifier_rule *rule_ptr)
{
  struct nsm_vlanclass_group_entry *group_temp_ptr = NULL;
  struct hal_vlan_classifier_rule rule;
  struct avl_node *node = NULL;
  struct hal_vlan_classifier_rule *search_rule = NULL;
  struct nsm_vlan_classifier_group *group_ptr = NULL;
  int rule_id = 0;
  int ret = 0;

  pal_mem_set (&rule, 0, sizeof(struct hal_vlan_classifier_rule));
  group_temp_ptr = *group_temp;

  rule.type = group_temp_ptr->type;
  rule.u.protocol.ether_type = group_temp_ptr->ether_type;
  rule.u.protocol.encaps = group_temp_ptr->encaps;
  rule.row_status = XSTP_SNMP_ROW_STATUS_ACTIVE;

  /* Make sure bridge is initialized. */
  if ((!master) )
    return RESULT_ERROR;

  if (master->rule_tree)
    {
      /* Go through the rules to see if the rule already exists */
      for (node = avl_top(master->rule_tree); node; node = avl_next(node))
         {
           search_rule = (struct hal_vlan_classifier_rule *)AVL_NODE_INFO(node);
           if (!search_rule)
             continue;

           /* Check if exactly the same rule already exists.*/
           if (0 == nsm_vlan_classifier_rule_cmp(search_rule, &rule))
             {
               return RESULT_ERROR;
             }
         }
    }

  /* assign new rule id */
  rule_id = nsm_vlan_classifier_get_new_rule_id(master);

  if (rule_id < 0)
    rule_id = NSM_VLAN_RULE_ID_MIN;

  rule.rule_id = rule_id;
  if (master->rule_tree)
    rule_ptr = nsm_add_new_classifier_rule(master, &rule);
  if (!rule_ptr)
    {
      return RESULT_ERROR;
    }
  /* Check if the same group already exists */
  group_ptr = nsm_add_new_classifier_group(master, group_temp_ptr->group_id);
   if (!group_ptr)
     return RESULT_ERROR;

  /* Insert group to rule's groups tree. */
  ret = avl_insert(rule_ptr->group_tree, (void *)group_ptr);
  if (ret < 0)
    {
      if (0 >= avl_get_tree_size(group_ptr->rule_tree))
        nsm_del_classifier_group_by_id(master, group_ptr->group_id);

      return RESULT_ERROR;
    }

  /* Insert rule to group rules tree. */
  ret = avl_insert(group_ptr->rule_tree, (void *)rule_ptr);
  if (ret < 0)
    {
      avl_remove(rule_ptr->group_tree, (void *)group_ptr);
      if (0 >= avl_get_tree_size(group_ptr->rule_tree))
        nsm_del_classifier_group_by_id(master, group_ptr->group_id);

      return RESULT_ERROR;
    }

  return  RESULT_OK;
}

/*
 * Given a bridge, instance and exact/inexact request, returns a pointer to
 * the interface structure or NULL if none found.
 */
int
xstp_snmp_classifier_if_group_lookup (int port_id, u_int32_t if_group_id,
                                      int exact,
                                      struct nsm_bridge_port **ret_br_port,
                                 struct nsm_vlan_class_if_group **r_group,
                                 struct nsm_vlanclass_port_entry **r_temp_group)
{
  struct avl_node *node = NULL;
  struct nsm_vlanclass_port_entry *best_temp_group = NULL;
  struct nsm_if *zif = NULL;
  struct interface *ifp = NULL;
  struct nsm_bridge_port *br_port = NULL;
  struct nsm_bridge *br = NULL;
  struct nsm_bridge_master *master = NULL;
  struct nsm_vlan_class_if_group *search_group = NULL;
  struct nsm_vlan_class_if_group *best_group = NULL;

  br = nsm_snmp_get_first_bridge ();
  if (!br)
    return SNMP_ERR_GENERR;

  master = br->master;
  if (!master)
    return SNMP_ERR_GENERR;

  ifp = nsm_snmp_bridge_rule_port_lookup_by_port_id (br, port_id, exact,
                                                     ret_br_port);
  if (!ifp)
    return RESULT_ERROR;

  zif = (struct nsm_if *)ifp->info;
  if (!zif)
    return RESULT_ERROR;

  br_port = zif->switchport;

  /* Go through the interface group list to see if matching group exists */
  for (node = avl_top(zif->group_tree); node; node = avl_next(node))
  {
    search_group = (struct nsm_vlan_class_if_group *)AVL_NODE_INFO(node);
    if (!search_group)
      continue;

    if (exact)
    {
      if (search_group->group_id == if_group_id)
      {
      best_group = search_group;
      }
    }
    else
    {
      if (if_group_id < search_group->group_id)
      {
      if (best_group == NULL)
      {
        best_group = search_group;
      }
      else
      {
        if (search_group->group_id < best_group->group_id)
          best_group = search_group;
      }
      }
    }
  }

  best_temp_group = xstp_snmp_classifier_temp_if_group_lookup(exact, port_id,
                                                              if_group_id);

  if (exact)
    {
      if (best_group)
      *r_group = best_group;
      else if(best_temp_group)
      *r_temp_group = best_temp_group;
      else
      return RESULT_ERROR;
    }
  else
    {
      if (best_group && best_temp_group)
      {
        if (best_group->group_id < best_temp_group->group_id)
          *r_group = best_group;
        else
          *r_temp_group = best_temp_group;
      }
      else if (!best_temp_group && best_group)
      *r_group = best_group;
      else if (best_temp_group && !best_group)
      *r_temp_group = best_temp_group;
      else
      return RESULT_ERROR;
    }

  return RESULT_OK;
}

/* API to store inactive protocol port table entries in the temp database */
int
xstp_snmp_classifier_temp_add_group_if (struct nsm_bridge_master *master,
                                        struct interface *ifp,
                                        struct nsm_vlan_class_if_group *group,
                                        int group_id, int port_id)
{
  struct nsm_vlanclass_port_entry *if_group_temp = NULL;

  if_group_temp = XCALLOC (MTYPE_NSM_VLAN_CLASS_TMP_GROUP,
                           sizeof (struct nsm_vlanclass_port_entry));
  if (!if_group_temp)
    return RESULT_ERROR;

  if (group)
    {
      if_group_temp->port_id = port_id;
      if_group_temp->group_id = group->group_id;
      if_group_temp->vlan_id = group->group_vid;
      if_group_temp->row_status = XSTP_SNMP_ROW_STATUS_NOTINSERVICE;
    }
  else
    {
      if_group_temp->group_id = group_id;
      if_group_temp->port_id = port_id;
      if_group_temp->row_status = XSTP_SNMP_ROW_STATUS_NOTREADY;
    }

  listnode_add(xstp_snmp_classifier_tmp_if_group_list, if_group_temp);
  return RESULT_OK;

}

/* Check for best matching inactive entries an interface based on port id
   and rule id */
struct nsm_vlanclass_port_entry *
xstp_snmp_classifier_temp_if_group_lookup (int exact, int port_id, int group_id)
{
  struct listnode *node = NULL;
  struct nsm_vlanclass_port_entry *if_temp_group = NULL;
  struct nsm_vlanclass_port_entry *best_temp_group = NULL;

  LIST_LOOP (xstp_snmp_classifier_tmp_if_group_list, if_temp_group, node)
  {
   if (exact)
     {
      if (if_temp_group->port_id == port_id &&
          if_temp_group->group_id == group_id)
       {
         return if_temp_group;
       }
     }
   else
     {
     if ((port_id < if_temp_group->port_id ) ||
      ((port_id == if_temp_group->port_id ) &&
         (group_id < if_temp_group->group_id)))
       {
        if (!best_temp_group)
          {
            best_temp_group = if_temp_group;
          }
        else if (( if_temp_group->port_id < best_temp_group->port_id) ||
                 ((if_temp_group->port_id == best_temp_group->port_id) &&
         (if_temp_group->group_id < best_temp_group->group_id)))
      {
          best_temp_group = if_temp_group;
              }
    }
   }
  }

  return best_temp_group;
}

/* Update port rule information whenever a change from SNMP */
int
xstp_snmp_classifier_add_group_if (struct nsm_bridge_master *master,
                                 struct interface *ifp,
                               struct nsm_vlanclass_port_entry *if_group_temp)
{
  struct nsm_vlan_classifier_group *group= NULL;
  struct nsm_if *zif = NULL;
  struct avl_node *if_node = NULL;
  int group_id = 0;
  int group_vid = 0;
  int ret = 0;

  if (!ifp)
    return RESULT_ERROR;

  zif = (struct nsm_if *) (ifp->info);
  if (!zif)
    return RESULT_ERROR;

  if (!if_group_temp)
    return RESULT_ERROR;

  group_id = if_group_temp->group_id;
  /* Get group details. */
  group = nsm_get_classifier_group_by_id(master, group_id);

  /* No group exists, return */
  if (!group)
    return RESULT_ERROR;

  /* Check if group is already activated on interface. */
  if_node = avl_search(group->if_tree, ifp);

  /* Group already activated on interface. */
  if (if_node)
    return RESULT_OK;

  if (if_group_temp->vlan_id != NSM_VLAN_INVALID_VLAN_ID)
    group_vid = if_group_temp->vlan_id;

  ret = nsm_add_classifier_group_to_if(ifp, group_id, group_vid);
  if (ret < 0)
    return RESULT_ERROR;

  return RESULT_OK;
}

/* Delete the selected rule from the given interface */
int
xstp_snmp_classifier_del_group_from_if (struct interface *ifp,
                                 struct nsm_vlan_class_if_group *if_group_ptr)
{
  struct nsm_if *zif = NULL;
  int ret = 0;

  if (!if_group_ptr)
    return RESULT_ERROR;

  zif = (struct nsm_if *)ifp->info;

  if (!zif)
    return RESULT_ERROR;

  ret = nsm_del_classifier_group_from_if(ifp, if_group_ptr->group_id);
  if (ret < 0)
    return RESULT_ERROR;

  return RESULT_OK;
}

int
nsm_vlan_classifier_get_new_rule_id (struct nsm_bridge_master *master)
{

  struct avl_node *node = NULL;
  struct hal_vlan_classifier_rule *rule_ptr = NULL;

  if (!master)
    return RESULT_ERROR;

  if (!master->rule_tree)
    return RESULT_ERROR;

  node = avl_top(master->rule_tree);

  if (!node)
    return RESULT_ERROR;

  while (node->right != NULL)
     node = node->right;

  rule_ptr = (struct hal_vlan_classifier_rule *)AVL_NODE_INFO(node);
  if (rule_ptr)
    return (rule_ptr->rule_id + 1);
  else
    return RESULT_ERROR;

}

#endif /* HAVE_VLAN_CLASS */

int
xstp_snmp_write_dot1qConstraintTable(int action,
                                     u_char * xstp_snmp_val,
                                     u_char xstp_snmp_val_type,
                                     size_t xstp_snmp_val_len,
                                     u_char * statP, oid * name, size_t name_len,
                                     struct variable *v, u_int32_t vr_id)
{
  switch (v->magic)
    {
    /*SET not allowed */
    case DOT1QCONSTRAINTTYPE:
      return SNMP_ERR_GENERR;
      break;

    /*SET not allowed */
    case DOT1QCONSTRAINTSTATUS:
      return SNMP_ERR_GENERR;
      break;

    default:
      pal_assert (0);
      return SNMP_ERR_GENERR;
      break;
    }

  return SNMP_ERR_NOERROR;
}

#endif /* HAVE_VLAN */
#endif /* HAVE_SNMP */

