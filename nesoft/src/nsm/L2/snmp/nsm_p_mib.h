#ifndef NSM_P_MIB_H
#define NSM_P_MIB_H

/*
 * this is the root OID we use to register the objects handled in this
 * module.
 */
#define BRIDGEMIBOID                               1, 3, 6, 1, 2, 1, 17

/*
 * PacOS enterprise BRIDGE-MIB equivalent root OID.  This variable is used to
 * register the BRIDGE-MIB to SNMP agent under SMUX protocol.
 */
#define PSTPDOID                                   1,3,6,1,4,1,3317,1,2,13


#define DOT1QCONSTRAINTTYPEDEFAULT                 1
#define DOT1DTPAGINGTIME                           2
#define DOT1DTRAFFICCLASSESENABLED                 3
#define DOT1QVLANVERSIONNUMBER                     4
#define DOT1QGVRPSTATUS                            6
#define DOT1QVLANNUMDELETES                        7
#define DOT1DGMRPSTATUS                            8
#define DOT1QMAXVLANID                             12
#define DOT1QCONSTRAINTSETDEFAULT                  14
#define DOT1QNUMVLANS                              16
#define DOT1DDEVICECAPABILITIES                    17
#define DOT1QMAXSUPPORTEDVLANS                     18
#define DOT1QNEXTFREELOCALVLANINDEX                24
#define DOT1QTPVLANPORTINFRAMES                    37
#define DOT1QTPVLANPORTOUTFRAMES                   38
#define DOT1QTPVLANPORTINDISCARDS                  39
#define DOT1QTPVLANPORTINOVERFLOWFRAMES            40
#define DOT1QTPVLANPORTOUTOVERFLOWFRAMES           41
#define DOT1QTPVLANPORTINOVERFLOWDISCARDS          42
#define DOT1DPORTGMRPSTATUS                        43
#define DOT1DPORTGMRPFAILEDREGISTRATIONS           44
#define DOT1DPORTGMRPLASTPDUORIGIN                 45
#define DOT1QVLANSTATICNAME                        46
#define DOT1QVLANSTATICEGRESSPORTS                 47
#define DOT1QVLANFORBIDDENEGRESSPORTS              48
#define DOT1QVLANSTATICUNTAGGEDPORTS               49
#define DOT1QVLANSTATICROWSTATUS                   50
#define DOT1QFORWARDUNREGISTEREDPORTS              51
#define DOT1QFORWARDUNREGISTEREDSTATICPORTS        52
#define DOT1QFORWARDUNREGISTEREDFORBIDDENPORTS     53
#define DOT1QTPGROUPADDRESS                        54
#define DOT1QTPGROUPEGRESSPORTS                    55
#define DOT1QTPGROUPLEARNT                         56
#define DOT1DUSERPRIORITY                          57
#define DOT1DREGENUSERPRIORITY                     58
#define DOT1DTRAFFICCLASSPRIORITY                  59
#define DOT1DTRAFFICCLASS                          60
#define DOT1QSTATICMULTICASTADDRESS                61
#define DOT1QSTATICMULTICASTRECEIVEPORT            62
#define DOT1QSTATICMULTICASTSTATICEGRESSPORTS      63
#define DOT1QSTATICMULTICASTFORBIDDENEGRESSPORTS   64
#define DOT1QSTATICMULTICASTSTATUS                 65
#define DOT1DPORTOUTBOUNDACCESSPRIORITY            76
#define DOT1QFORWARDALLPORTS                       77
#define DOT1QFORWARDALLSTATICPORTS                 78
#define DOT1QFORWARDALLFORBIDDENPORTS              79
#define DOT1QVLANTIMEMARK                          80
#define DOT1QVLANINDEX                             81
#define DOT1QVLANFDBID                             82
#define DOT1QVLANCURRENTEGRESSPORTS                83
#define DOT1QVLANCURRENTUNTAGGEDPORTS              84
#define DOT1QVLANSTATUS                            85
#define DOT1QVLANCREATIONTIME                      86
#define DOT1QPVID                                  87
#define DOT1QPORTACCEPTABLEFRAMETYPES              88
#define DOT1QPORTINGRESSFILTERING                  89
#define DOT1QPORTGVRPSTATUS                        90
#define DOT1QPORTGVRPFAILEDREGISTRATIONS           91
#define DOT1QPORTGVRPLASTPDUORIGIN                 92
#define DOT1DTPPORTINOVERFLOWFRAMES                93
#define DOT1DTPPORTOUTOVERFLOWFRAMES               94
#define DOT1DTPPORTINOVERFLOWDISCARDS              95
#define DOT1QFDBID                                 106
#define DOT1QFDBDYNAMICCOUNT                       107
#define DOT1QTPVLANPORTHCINFRAMES                  108
#define DOT1QTPVLANPORTHCOUTFRAMES                 109
#define DOT1QTPVLANPORTHCINDISCARDS                110
#define DOT1QSTATICUNICASTADDRESS                  111
#define DOT1QSTATICUNICASTRECEIVEPORT              112
#define DOT1QSTATICUNICASTALLOWEDTOGOTO            113
#define DOT1QSTATICUNICASTSTATUS                   114
#define DOT1QCONSTRAINTVLAN                        115
#define DOT1QCONSTRAINTSET                         116
#define DOT1QCONSTRAINTTYPE                        117
#define DOT1QCONSTRAINTSTATUS                      118
#define DOT1DPORTDEFAULTUSERPRIORITY               119
#define DOT1DPORTNUMTRAFFICCLASSES                 120
#define DOT1DPORTCAPABILITIES                      121
#define DOT1QTPFDBADDRESS                          122
#define DOT1QTPFDBPORT                             123
#define DOT1QTPFDBSTATUS                           124
#define DOT1DPORTGARPJOINTIME                      125
#define DOT1DPORTGARPLEAVETIME                     126
#define DOT1DPORTGARPLEAVEALLTIME                  127
#define DOT1DTPHCPORTINFRAMES                      128
#define DOT1DTPHCPORTOUTFRAMES                     129
#define DOT1DTPHCPORTINDISCARDS                    130
#define DOT1VPROTOCOLTEMPLATEFRAMETYPE             131
#define DOT1VPROTOCOLTEMPLATEPROTOCOLVALUE         132
#define DOT1VPROTOCOLGROUPID                       133
#define DOT1VPROTOCOLGROUPROWSTATUS                134
#define DOT1VPROTOCOLPORTGROUPID                   135
#define DOT1VPROTOCOLPORTGROUPVID                  136
#define DOT1VPROTOCOLPORTROWSTATUS                 137
#define DOT1QPORTRESTRICTEDVLANREGISTRATION        138
#define DOT1QPORTRESTRICTEDGROUPREGISTRATION       139

void xstp_p_snmp_init (struct lib_globals *);


#define DOT1DPORTRESTRICTEDGROUP_ENABLED  1
#define DOT1DPORTRESTRICTEDGROUP_DISABLED 2

#define DOT1DPORTGMRPSTATUS_ENABLED  1
#define DOT1DPORTGMRPSTATUS_DISABLED 2

/*
 * function declarations
 */
void            init_dot1dBridge(void);
FindVarMethod   xstp_snmp_pBridge_scalars;
FindVarMethod   xstp_snmp_dot1dTpFdbTable;
FindVarMethod   xstp_snmp_dot1dPortGmrpTable;
FindVarMethod   xstp_snmp_dot1dUserPriorityRegenTable;
FindVarMethod   xstp_snmp_dot1dTrafficClassTable;
FindVarMethod   xstp_snmp_dot1dTpPortTable;
FindVarMethod   xstp_snmp_dot1dBasePortTable;
FindVarMethod   xstp_snmp_dot1dPortOutboundAccessPriorityTable;
FindVarMethod   xstp_snmp_dot1dTpPortOverflowTable;
FindVarMethod   xstp_snmp_dot1dStpPortTable;
FindVarMethod   xstp_snmp_dot1dPortPriorityTable;
FindVarMethod   xstp_snmp_dot1dPortCapabilitiesTable;
FindVarMethod   xstp_snmp_dot1dPortGarpTable;
FindVarMethod   xstp_snmp_dot1dTpHCPortTable;
FindVarMethod   xstp_snmp_dot1dStaticTable;
WriteMethod     xstp_snmp_write_dot1dTrafficClassesEnabled;
WriteMethod     xstp_snmp_write_dot1dGmrpStatus;
WriteMethod     xstp_snmp_write_dot1dTrafficClass;
#endif                          /* NSM_P_MIB_H */
