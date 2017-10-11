/* Copyright (C) 2001-2004  All Rights Reserved. */

#ifndef NSM_Q_MIB_H
#define NSM_Q_MIB_H

#include "nsm_snmp_common.h"

#ifdef HAVE_VLAN
#include "nsm_bridge.h"
#include "nsm_vlan.h"
#include "nsm_vlanclassifier.h"

/*
 * this is the root OID we use to register the objects handled in this
 * module.
 */
#define BRIDGEMIBOID                                  1, 3, 6, 1, 2, 1, 17

/* 
 * PacOS enterprise BRIDGE-MIB equivalent root OID.  This variable is used to
 * register the BRIDGE-MIB to SNMP agent under SMUX protocol.
 */

#define STPDOID                                       1,3,6,1,4,1,3317,1,2,12
#define RSTPDOID                                      1,3,6,1,4,1,3317,1,2,16

#define DOT1QCONSTRAINTTYPEDEFAULT                    1
#define DOT1DTPAGINGTIME                              2
#define DOT1DTRAFFICCLASSESENABLED                    3
#define DOT1QVLANVERSIONNUMBER                        4
#define DOT1QGVRPSTATUS                               6
#define DOT1QVLANNUMDELETES                           7
#define DOT1DGMRPSTATUS                               8
#define DOT1QMAXVLANID                                12
#define DOT1QCONSTRAINTSETDEFAULT                     14
#define DOT1QNUMVLANS                                 16
#define DOT1DDEVICECAPABILITIES                       17
#define DOT1QMAXSUPPORTEDVLANS                        18
#define DOT1QNEXTFREELOCALVLANINDEX                   24
#define DOT1QTPVLANPORTINFRAMES                       37
#define DOT1QTPVLANPORTOUTFRAMES                      38
#define DOT1QTPVLANPORTINDISCARDS                     39
#define DOT1QTPVLANPORTINOVERFLOWFRAMES               40
#define DOT1QTPVLANPORTOUTOVERFLOWFRAMES              41
#define DOT1QTPVLANPORTINOVERFLOWDISCARDS             42
#define DOT1DPORTGMRPSTATUS                           43
#define DOT1DPORTGMRPFAILEDREGISTRATIONS              44
#define DOT1DPORTGMRPLASTPDUORIGIN                    45
#define DOT1QVLANSTATICNAME                           46
#define DOT1QVLANSTATICEGRESSPORTS                    47
#define DOT1QVLANFORBIDDENEGRESSPORTS                 48
#define DOT1QVLANSTATICUNTAGGEDPORTS                  49
#define DOT1QVLANSTATICROWSTATUS                      50
#define DOT1QFORWARDUNREGISTEREDPORTS                 51
#define DOT1QFORWARDUNREGISTEREDSTATICPORTS           52
#define DOT1QFORWARDUNREGISTEREDFORBIDDENPORTS        53
#define DOT1QTPGROUPADDRESS                           54
#define DOT1QTPGROUPEGRESSPORTS                       55
#define DOT1QTPGROUPLEARNT                            56
#define DOT1DUSERPRIORITY                             57
#define DOT1DREGENUSERPRIORITY                        58
#define DOT1DTRAFFICCLASSPRIORITY                     59
#define DOT1DTRAFFICCLASS                             60
#define DOT1QSTATICMULTICASTADDRESS                   61
#define DOT1QSTATICMULTICASTRECEIVEPORT               62
#define DOT1QSTATICMULTICASTSTATICEGRESSPORTS         63
#define DOT1QSTATICMULTICASTFORBIDDENEGRESSPORTS      64
#define DOT1QSTATICMULTICASTSTATUS                    65
#define DOT1DPORTOUTBOUNDACCESSPRIORITY               76
#define DOT1QFORWARDALLPORTS                          77
#define DOT1QFORWARDALLSTATICPORTS                    78
#define DOT1QFORWARDALLFORBIDDENPORTS                 79
#define DOT1QVLANTIMEMARK                             80
#define DOT1QVLANINDEX                                81
#define DOT1QVLANFDBID                                82
#define DOT1QVLANCURRENTEGRESSPORTS                   83
#define DOT1QVLANCURRENTUNTAGGEDPORTS                 84
#define DOT1QVLANSTATUS                               85
#define DOT1QVLANCREATIONTIME                         86
#define DOT1QPVID                                     87
#define DOT1QPORTACCEPTABLEFRAMETYPES                 88
#define DOT1QPORTINGRESSFILTERING                     89
#define DOT1QPORTGVRPSTATUS                           90
#define DOT1QPORTGVRPFAILEDREGISTRATIONS              91
#define DOT1QPORTGVRPLASTPDUORIGIN                    92
#define DOT1DTPPORTINOVERFLOWFRAMES                   93
#define DOT1DTPPORTOUTOVERFLOWFRAMES                  94
#define DOT1DTPPORTINOVERFLOWDISCARDS                 95
#define DOT1QFDBID                                    106
#define DOT1QFDBDYNAMICCOUNT                          107
#define DOT1QTPVLANPORTHCINFRAMES                     108
#define DOT1QTPVLANPORTHCOUTFRAMES                    109
#define DOT1QTPVLANPORTHCINDISCARDS                   110
#define DOT1QSTATICUNICASTADDRESS                     111
#define DOT1QSTATICUNICASTRECEIVEPORT                 112
#define DOT1QSTATICUNICASTALLOWEDTOGOTO               113
#define DOT1QSTATICUNICASTSTATUS                      114
#define DOT1QCONSTRAINTVLAN                           115
#define DOT1QCONSTRAINTSET                            116
#define DOT1QCONSTRAINTTYPE                           117
#define DOT1QCONSTRAINTSTATUS                         118
#define DOT1DPORTDEFAULTUSERPRIORITY                  119
#define DOT1DPORTNUMTRAFFICCLASSES                    120
#define DOT1DPORTCAPABILITIES                         121
#define DOT1QTPFDBADDRESS                             122
#define DOT1QTPFDBPORT                                123
#define DOT1QTPFDBSTATUS                              124
#define DOT1DPORTGARPJOINTIME                         125
#define DOT1DPORTGARPLEAVETIME                        126
#define DOT1DPORTGARPLEAVEALLTIME                     127
#define DOT1DTPHCPORTINFRAMES                         128
#define DOT1DTPHCPORTOUTFRAMES                        129
#define DOT1DTPHCPORTINDISCARDS                       130
#define DOT1VPROTOCOLTEMPLATEFRAMETYPE                131
#define DOT1VPROTOCOLTEMPLATEPROTOCOLVALUE            132
#define DOT1VPROTOCOLGROUPID                          133
#define DOT1VPROTOCOLGROUPROWSTATUS                   134
#define DOT1VPROTOCOLPORTGROUPID                      135
#define DOT1VPROTOCOLPORTGROUPVID                     136
#define DOT1VPROTOCOLPORTROWSTATUS                    137
#define DOT1QPORTRESTRICTEDVLANREGISTRATION           138
#define DOT1QPORTRESTRICTEDGROUPREGISTRATION          139

void xstp_q_snmp_init (struct lib_globals *);

#define DOT1DPORTGVRPSTATUS_ENABLED  1
#define DOT1DPORTGVRPSTATUS_DISABLED 2

#define DOT1DPORTRESTRICTEDVLAN_ENABLED  1
#define DOT1DPORTRESTRICTEDVLAN_DISABLED 2

#define DOT1DPORTINGRESSFILTER_ENABLED   1
#define DOT1DPORTINGRESSFILTER_DISABLED  2

#define DOT1DPORTRESTRICTEDGROUP_ENABLED  1
#define DOT1DPORTRESTRICTEDGROUP_DISABLED 2
/* defines for SetMask values of vlan static table */
#define XSTP_SNMP_SET_STATIC_VLANNAME                  0x2
#define XSTP_SNMP_SET_STATIC_VLAN_EGRESSPORT           0x4      
#define XSTP_SNMP_SET_STATIC_VLAN_FORBIDDENPORT        0x8
#define XSTP_SNMP_SET_STATIC_VLAN_UNTAGGEDPORT         0x10
#define XSTP_SNMP_SET_STATIC_VLAN_ROWSTATUS            0x20

/* defines for snmp row status values */
#define XSTP_SNMP_ROW_STATUS_MIN                       0
#define XSTP_SNMP_ROW_STATUS_ACTIVE                    1
#define XSTP_SNMP_ROW_STATUS_NOTINSERVICE              2
#define XSTP_SNMP_ROW_STATUS_NOTREADY                  3
#define XSTP_SNMP_ROW_STATUS_CREATEANDGO               4
#define XSTP_SNMP_ROW_STATUS_CREATEANDWAIT             5
#define XSTP_SNMP_ROW_STATUS_DESTROY                   6
#define xSTP_SNMP_ROW_STATUS_MAX                       7

#define PROTOCOL_FRAME_TYPE                         ETHERNET
#define PROTOCOL_VALUE                                 2

/* defines for static vlan port action */
#define XSTP_SNMP_ADD_PORT_TO_VLAN                     1
#define XSTP_SNMP_DELETE_PORT_FROM_VLAN                2

#define MIN_VLAN_ID 1
#define MAX_VLAN_ID 4096

struct VlanStaticEntry {
  int  vlanid;
  char name[32];
  char egressports[BITSTRINGSIZE(NSM_L2_SNMP_PORT_ARRAY_SIZE)];
  char forbiddenports[BITSTRINGSIZE(NSM_L2_SNMP_PORT_ARRAY_SIZE)];
  char untaggedports[BITSTRINGSIZE(NSM_L2_SNMP_PORT_ARRAY_SIZE)];
  char rowstatus;
};

struct VlanStaticTable {
  int    active_entries;
  int    inactive_entries;
  struct VlanStaticEntry *entry[NSM_VLAN_MAX];
  struct VlanStaticEntry *sorted[NSM_VLAN_MAX];
};

struct PortVlanEntry {
  int  port;
  int  pvid;
  int  acceptable_frame_types;
  int  ingress_filtering;
  int  gvrp_status;
  int  gvrp_failed_registrations;
  int  gvrp_pdu_origin;
  struct gvrp_port *gvrp_port;
};
 
struct nsm_vlanclass_group_entry 
{
  int type;
  unsigned short ether_type;  /* Protocol value                          */
  unsigned int encaps;        /* Packet L2 encapsulation.                */
  int  rule_id;
  int  group_id;
  int  proto_group_row_status;
};

struct nsm_vlanclass_port_entry
{
  int port_id;
  int group_id;
  int vlan_id;
  int row_status;
};


struct PortVlanTable {
  int  entries;
  struct PortVlanEntry entry[NSM_L2_SNMP_PORT_ARRAY_SIZE];
  struct PortVlanEntry *sorted[NSM_L2_SNMP_PORT_ARRAY_SIZE];
};

struct ForwardAllEntry {
  int vlanid;
  char forwardallports[BITSTRINGSIZE(NSM_L2_SNMP_PORT_ARRAY_SIZE)];
  char forwardallstaticports[BITSTRINGSIZE(NSM_L2_SNMP_PORT_ARRAY_SIZE)];
  char forwardallforbiddenports[BITSTRINGSIZE(NSM_L2_SNMP_PORT_ARRAY_SIZE)];
};

struct ForwardAllTable {
  int entries;
  struct ForwardAllEntry *entry[HAL_MAX_VLAN_ID];
  struct ForwardAllEntry *sorted[HAL_MAX_VLAN_ID];
};

struct ForwardUnregisteredEntry {
  int  vlanid;
  char forwardunregisteredports[BITSTRINGSIZE(NSM_L2_SNMP_PORT_ARRAY_SIZE)];
  char forwardunregisteredstaticports[BITSTRINGSIZE(NSM_L2_SNMP_PORT_ARRAY_SIZE)];
  char forwardunregisteredforbiddenports[BITSTRINGSIZE(NSM_L2_SNMP_PORT_ARRAY_SIZE)];
};

struct ForwardUnregisteredTable {
  int entries;
  struct ForwardUnregisteredEntry *entry[HAL_MAX_VLAN_ID];
  struct ForwardUnregisteredEntry *sorted[HAL_MAX_VLAN_ID];
};
struct FidEntry {
  int  fid;
  int  num_of_entries;
};

struct FidTable {
  int    entries;
  struct FidEntry *entry[HAL_MAX_VLAN_ID];
  struct FidEntry *sorted[HAL_MAX_VLAN_ID];
};

int
xstp_snmp_vlan_static_descend_cmp(const void *e1, const void *e2);

int
xstp_snmp_vlan_static_ascend_cmp(const void *e1, const void *e2);

struct port_vlan_HC_entry {
  int  vlanid;
  int  entries;
  int  ports[NSM_L2_SNMP_PORT_ARRAY_SIZE];
  int  *sorted[NSM_L2_SNMP_PORT_ARRAY_SIZE];
};

struct port_vlan_HC_table {
  int    entries;
  struct port_vlan_HC_entry *entry[HAL_MAX_VLAN_ID];
  struct port_vlan_HC_entry *sorted[HAL_MAX_VLAN_ID];
};

struct port_vlan_stats_entry {
  int  vlanid;
  int  entries;
  int  ports[NSM_L2_SNMP_PORT_ARRAY_SIZE];
  int  *sorted[NSM_L2_SNMP_PORT_ARRAY_SIZE];
};

struct port_vlan_stats_table {
  int    entries;
  struct port_vlan_stats_entry *entry[HAL_MAX_VLAN_ID];
  struct port_vlan_stats_entry *sorted[HAL_MAX_VLAN_ID];
};

extern  int
xstp_snmp_update_port_vlan_stats_table ( struct port_vlan_stats_table **table,
                                         struct nsm_bridge * br);
extern struct
port_vlan_stats_entry *
xstp_snmp_port_vlan_stats_table_lookup (struct port_vlan_stats_table *table, 
                                        u_int32_t *port, u_int32_t *vlan, 
                                        int exact);

int
xstp_snmp_update_fid_table ( struct FidTable **table, struct nsm_bridge *bridge);

int
xstp_snmp_update_vlan_static_table ( struct VlanStaticTable **table,
                                     struct nsm_bridge *br);

int
xstp_snmp_add_vlan_static_table (int vid, struct VlanStaticTable **table,
                                 int set_mask, char *value,
                                 int row_status);

int
xstp_snmp_add_static_vlan_db (struct VlanStaticEntry *vstatic);

int
xstp_snmp_set_static_vlan_ports (struct VlanStaticEntry *vstatic, int action);


struct vlan_current_entry {
  int vlanid;
  int timemark;  /* not implemented in bridge */
  int vid;
  int fid;
  char egressports[BITSTRINGSIZE(NSM_L2_SNMP_PORT_ARRAY_SIZE)];
  char untaggedports[BITSTRINGSIZE(NSM_L2_SNMP_PORT_ARRAY_SIZE)];
  int status;
  pal_time_t creation_time;
};

struct vlan_current_table {
  int entries;
  struct vlan_current_entry *entry[HAL_MAX_VLAN_ID];
  struct vlan_current_entry *sorted[HAL_MAX_VLAN_ID];
};
  
extern int
xstp_snmp_update_port_vlan_HC_table ( struct port_vlan_HC_table **table, struct
                                     nsm_bridge * br);
extern struct 
port_vlan_HC_entry *
xstp_snmp_port_vlan_HCtable_lookup (struct port_vlan_HC_table *table, 
                                    int *port, int *vlan, int exact);
extern int xstp_snmp_update_vlan_static_entry (struct VlanStaticEntry *vstatic,
                                               struct nsm_bridge * br,
                                              int set_mask, char *var_val);
extern void xstp_snmp_del_vlan_static_table (struct VlanStaticEntry *vstatic);
extern int xstp_snmp_del_static_vlan_db (char *name,
                                         struct VlanStaticEntry *vstatic);
extern void xstp_snmp_del_l2_snmp ();

int
xstp_snmp_classifier_nsm_ruledb_to_snmpdb(u_int32_t encaps, 
                                          u_int16_t ether_type,
                                       struct nsm_vlanclass_frame_proto *entry);
int 
xstp_snmp_classifier_rule_lookup(struct nsm_bridge_master *master,
                                 struct nsm_vlanclass_frame_proto *key,
                                 struct nsm_vlanclass_frame_proto *search_key, 
                                 int exact,
                                 struct hal_vlan_classifier_rule **rule_ptr,
                                 struct nsm_vlanclass_group_entry **group_temp);
struct nsm_vlan_classifier_group 
*xstp_snmp_find_classifier_group(struct hal_vlan_classifier_rule *rule_ptr);

int
xstp_snmp_classifier_create_rule(struct nsm_bridge_master *master,
                                 struct nsm_vlanclass_group_entry **group_temp,
                                 struct hal_vlan_classifier_rule *rule_ptr);
int 
xstp_snmp_classifier_del_group_from_if(struct interface *ifp,
                                struct nsm_vlan_class_if_group *if_group_ptr);

struct nsm_vlanclass_group_entry*
xstp_snmp_classifier_temp_rule_lookup(int exact, 
                                      struct nsm_vlanclass_frame_proto *key); 

int
xstp_snmp_classifier_temp_group_create(struct nsm_bridge_master *nm,
                                       struct hal_vlan_classifier_rule *rule,
                                       struct nsm_vlanclass_frame_proto *key);
int
xstp_snmp_classifier_if_group_lookup(int port_id, u_int32_t rule_id, 
                                     int exact,
                                     struct nsm_bridge_port **ret_br_port,
                                     struct nsm_vlan_class_if_group **r_group,
                                struct nsm_vlanclass_port_entry **r_temp_group);

struct nsm_vlanclass_port_entry *
xstp_snmp_classifier_temp_if_group_lookup(int exact, int port_id, int group_id);

int
xstp_snmp_classifier_add_group_if(struct nsm_bridge_master *master, 
                                  struct interface *ifp,
                                struct nsm_vlanclass_port_entry *if_group_temp);

int
xstp_snmp_classifier_temp_add_group_if(struct nsm_bridge_master *master,
                                       struct interface *ifp,
                                       struct nsm_vlan_class_if_group *group,
                                       int group_id, int port_id);

/*
 * function declarations 
 */
void            init_dot1dBridge(void);
FindVarMethod   xstp_snmp_qBridge_scalars;
FindVarMethod   xstp_snmp_dot1dBridge;
FindVarMethod   xstp_snmp_dot1dTpFdbTable;
FindVarMethod   xstp_snmp_dot1qPortVlanStatisticsTable;
FindVarMethod   xstp_snmp_dot1dPortGmrpTable;
FindVarMethod   xstp_snmp_dot1qVlanStaticTable;
FindVarMethod   xstp_snmp_dot1qForwardUnregisteredTable;
FindVarMethod   xstp_snmp_dot1qTpGroupTable;
FindVarMethod   xstp_snmp_dot1dUserPriorityRegenTable;
FindVarMethod   xstp_snmp_dot1dTrafficClassTable;
FindVarMethod   xstp_snmp_dot1qStaticMulticastTable;
FindVarMethod   xstp_snmp_dot1dTpPortTable;
FindVarMethod   xstp_snmp_dot1dBasePortTable;
FindVarMethod   xstp_snmp_dot1dPortOutboundAccessPriorityTable;
FindVarMethod   xstp_snmp_dot1qForwardAllTable;
FindVarMethod   xstp_snmp_dot1qVlanCurrentTable;
FindVarMethod   xstp_snmp_dot1qPortVlanTable;
FindVarMethod   xstp_snmp_dot1dTpPortOverflowTable;

FindVarMethod   xstp_snmp_dot1dStpPortTable;
FindVarMethod   xstp_snmp_dot1qFdbTable;
FindVarMethod   xstp_snmp_dot1qPortVlanHCStatisticsTable;
FindVarMethod   xstp_snmp_dot1qStaticUnicastTable;
FindVarMethod   xstp_snmp_dot1qLearningConstraintsTable;
FindVarMethod   xstp_snmp_dot1dPortPriorityTable;
FindVarMethod   xstp_snmp_dot1dPortCapabilitiesTable;
FindVarMethod   xstp_snmp_dot1qTpFdbTable;
FindVarMethod   xstp_snmp_dot1dPortGarpTable;
FindVarMethod   xstp_snmp_dot1dTpHCPortTable;
FindVarMethod   xstp_snmp_dot1dStaticTable;
FindVarMethod   xstp_snmp_dot1vProtocolGroupTable;
FindVarMethod   xstp_snmp_dot1vProtocolPortTable;

WriteMethod     xstp_snmp_write_dot1dTrafficClassesEnabled;
WriteMethod     xstp_snmp_write_qBridge_scalars;
WriteMethod     xstp_snmp_write_dot1qForwardUnregisteredTable;
WriteMethod     xstp_snmp_write_dot1qStaticUnicastAllowedToGoTo;
WriteMethod     xstp_snmp_write_dot1qStaticUnicastStatus;
WriteMethod     xstp_snmp_write_dot1qStaticMulticastTable;
WriteMethod     xstp_snmp_write_dot1qStaticMulticastStatus;
WriteMethod     xstp_snmp_write_dot1qVlanStaticTable;
WriteMethod     xstp_snmp_write_dot1dTrafficClass;
WriteMethod     xstp_snmp_write_dot1qVlanStaticRowStatus;

WriteMethod     xstp_snmp_write_dot1qPvid;
WriteMethod     xstp_snmp_write_dot1qPortVlanTable;
WriteMethod     xstp_snmp_write_dot1qConstraintTable;
WriteMethod     xstp_snmp_write_dot1qForwardAllTable;
WriteMethod     xstp_snmp_write_dot1vProtocolGroupid;
WriteMethod     xstp_snmp_write_dot1vProtocolGroupRowStatus;
WriteMethod     xstp_snmp_write_dot1vProtocolPortTable;
WriteMethod     xstp_snmp_write_dot1VProtocolPortGroupVid;
WriteMethod     xstp_snmp_write_dot1VProtocolPortRowStatus;

#endif  /* HAVE_VLAN */
#endif  /* NSM_Q_MIB_H */
