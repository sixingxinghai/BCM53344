/* Copyright (C) 2002-2003  All Rights Reserved. */

#ifndef _NSM_MESSAGE_H
#define _NSM_MESSAGE_H

/* NSM protocol definition.  */

#define NSM_ERR_PKT_TOO_SMALL               -1
#define NSM_ERR_INVALID_SERVICE             -2
#define NSM_ERR_INVALID_PKT                 -3
#define NSM_ERR_SYSTEM_FAILURE              -4
#define NSM_ERR_INVALID_AFI                 -5
#define NSM_ERR_MALLOC_FAILURE              -6

#define LIB_ERR_INVALID_TLV                 -100

/* NSM protocol version is 1.  */
#define NSM_PROTOCOL_VERSION_1               1
#define NSM_PROTOCOL_VERSION_2               2

/* NSM message max length.  */
#define NSM_MESSAGE_MAX_LEN    4096

/* NSM service types.  */
#define NSM_SERVICE_INTERFACE                0
#define NSM_SERVICE_ROUTE                    1
#define NSM_SERVICE_ROUTER_ID                2
#define NSM_SERVICE_VRF                      3
#define NSM_SERVICE_ROUTE_LOOKUP             4
#define NSM_SERVICE_LABEL                    5
#define NSM_SERVICE_TE                       6
#define NSM_SERVICE_QOS                      7
#define NSM_SERVICE_QOS_PREEMPT              8
#define NSM_SERVICE_USER                     9
#define NSM_SERVICE_HOSTNAME                10 /* Obsolete. */
#define NSM_SERVICE_MPLS_VC                 11
#define NSM_SERVICE_MPLS                    12
#define NSM_SERVICE_GMPLS                   13
#define NSM_SERVICE_DIFFSERV                14
#define NSM_SERVICE_VPLS                    15
#define NSM_SERVICE_DSTE                    16
#define NSM_SERVICE_IPV4_MRIB               17
#define NSM_SERVICE_IPV4_PIM                18
#define NSM_SERVICE_IPV4_MCAST_TUNNEL       19
#define NSM_SERVICE_IPV6_MRIB               20
#define NSM_SERVICE_IPV6_PIM                21
#define NSM_SERVICE_BRIDGE                  22
#define NSM_SERVICE_VLAN                    23
#define NSM_SERVICE_IGP_SHORTCUT            24
#define NSM_SERVICE_CONTROL_ADJ             25
#define NSM_SERVICE_CONTROL_CHANNEL         26
#define NSM_SERVICE_TE_LINK                 27
#define NSM_SERVICE_DATA_LINK               28
#define NSM_SERVICE_DATA_LINK_SUB           29

#define NSM_SERVICE_MAX                     30


/* NSM events.  */
#define NSM_EVENT_CONNECT                    0
#define NSM_EVENT_DISCONNECT                 1

/* NSM messages.  */
#define NSM_MSG_BUNDLE

/* These messages has VR-ID = 0, VRF-ID = 0. */
#define NSM_MSG_SERVICE_REQUEST                  0
#define NSM_MSG_SERVICE_REPLY                    1
#define NSM_MSG_VR_ADD                           2
#define NSM_MSG_VR_DELETE                        3
#define NSM_MSG_VR_UPDATE                        4
#define NSM_MSG_LINK_ADD                         5
#define NSM_MSG_LINK_DELETE                      6
#define NSM_MSG_LINK_BULK_UPDATE                 7
#define NSM_MSG_LINK_ATTR_UPDATE                 8
#define NSM_MSG_VR_BIND                          9
#define NSM_MSG_VR_BIND_BULK                     10
#define NSM_MSG_VR_UNBIND                        11
#define NSM_MSG_ADDR_ADD                         12
#define NSM_MSG_ADDR_DELETE                      13
#define NSM_MSG_ADDR_BULK_UPDATE                 14

/* These messages has VR-ID = any, VRF-ID = 0. */
#define NSM_MSG_VRF_ADD                          15
#define NSM_MSG_VRF_DELETE                       16
#define NSM_MSG_VRF_UPDATE                       17
#define NSM_MSG_VRF_BIND                         18
#define NSM_MSG_VRF_BIND_BULK                    19
#define NSM_MSG_VRF_UNBIND                       20
#define NSM_MSG_ROUTE_PRESERVE                   21
#define NSM_MSG_ROUTE_STALE_REMOVE               22
#define NSM_MSG_PROTOCOL_RESTART                 23
#define NSM_MSG_USER_ADD                         25
#define NSM_MSG_USER_UPDATE                      26
#define NSM_MSG_USER_DELETE                      27
#define NSM_MSG_HOSTNAME_UPDATE                  28 /* Obsolete. */

/* These messages has VR-ID = any, VRF-ID = any. */
#define NSM_MSG_LINK_UP                          29
#define NSM_MSG_LINK_DOWN                        30
#define NSM_MSG_ROUTE_IPV4                       31
#define NSM_MSG_ROUTE_IPV6                       32
#define NSM_MSG_REDISTRIBUTE_SET                 33
#define NSM_MSG_REDISTRIBUTE_UNSET               34
#define NSM_MSG_ROUTE_CLEAN                      35
#define NSM_MSG_ROUTER_ID_UPDATE                 36

#define NSM_MSG_IPV4_NEXTHOP_BEST_LOOKUP         37
#define NSM_MSG_IPV6_NEXTHOP_BEST_LOOKUP         38
#define NSM_MSG_IPV4_NEXTHOP_EXACT_LOOKUP        39
#define NSM_MSG_IPV6_NEXTHOP_EXACT_LOOKUP        40
#define NSM_MSG_IPV4_NEXTHOP_BEST_LOOKUP_REG     41
#define NSM_MSG_IPV4_NEXTHOP_BEST_LOOKUP_DEREG   42
#define NSM_MSG_IPV4_NEXTHOP_EXACT_LOOKUP_REG    43
#define NSM_MSG_IPV4_NEXTHOP_EXACT_LOOKUP_DEREG  44
#define NSM_MSG_IPV6_NEXTHOP_BEST_LOOKUP_REG     45
#define NSM_MSG_IPV6_NEXTHOP_BEST_LOOKUP_DEREG   46
#define NSM_MSG_IPV6_NEXTHOP_EXACT_LOOKUP_REG    47
#define NSM_MSG_IPV6_NEXTHOP_EXACT_LOOKUP_DEREG  48

#define NSM_MSG_LABEL_POOL_REQUEST               49
#define NSM_MSG_LABEL_POOL_RELEASE               50

#define NSM_MSG_ADMIN_GROUP_UPDATE               51
#define NSM_MSG_INTF_PRIORITY_BW_UPDATE          52

#define NSM_MSG_QOS_CLIENT_INIT                  53
#define NSM_MSG_QOS_CLIENT_PROBE                 54
#define NSM_MSG_QOS_CLIENT_RESERVE               55
#define NSM_MSG_QOS_CLIENT_RELEASE               56
#define NSM_MSG_QOS_CLIENT_RELEASE_SLOW          57
#define NSM_MSG_QOS_CLIENT_PREEMPT               58
#define NSM_MSG_QOS_CLIENT_MODIFY                59
#define NSM_MSG_QOS_CLIENT_CLEAN                 60

#define NSM_MSG_MPLS_VC_ADD                      61
#define NSM_MSG_MPLS_VC_DELETE                   62

#define NSM_MSG_MPLS_FTN_IPV4                    63
#define NSM_MSG_MPLS_FTN_IPV6                    64
#define NSM_MSG_MPLS_FTN_REPLY                   65
#define NSM_MSG_MPLS_ILM_IPV4                    66
#define NSM_MSG_MPLS_ILM_IPV6                    67
#define NSM_MSG_MPLS_ILM_REPLY                   68
#define NSM_MSG_MPLS_IGP_SHORTCUT_LSP            69
#define NSM_MSG_MPLS_IGP_SHORTCUT_ROUTE          70
#define NSM_MSG_MPLS_VC_FIB_ADD                  71
#define NSM_MSG_MPLS_VC_FIB_DELETE               72
#define NSM_MSG_MPLS_PW_STATUS                   73
#define NSM_MSG_MPLS_NOTIFICATION                75
#define NSM_MSG_GMPLS_IF                         76
#define NSM_MSG_IGMP_LMEM                        77
#define NSM_MSG_SUPPORTED_DSCP_UPDATE            78
#define NSM_MSG_DSCP_EXP_MAP_UPDATE              79

#define NSM_MSG_VPLS_ADD                         80
#define NSM_MSG_VPLS_DELETE                      81
#define NSM_MSG_VPLS_MAC_WITHDRAW                84
#define NSM_MSG_DSTE_CT_UPDATE                   85
#define NSM_MSG_DSTE_TE_CLASS_UPDATE             86

#define NSM_MSG_IPV4_VIF_ADD                     87
#define NSM_MSG_IPV4_VIF_DEL                     88
#define NSM_MSG_IPV4_MRT_ADD                     89
#define NSM_MSG_IPV4_MRT_DEL                     90
#define NSM_MSG_IPV4_MRT_STAT_FLAGS_UPDATE       91
#define NSM_MSG_IPV4_MRT_NOCACHE                 92
#define NSM_MSG_IPV4_MRT_WRONGVIF                93
#define NSM_MSG_IPV4_MRT_WHOLEPKT_REQ            94
#define NSM_MSG_IPV4_MRT_WHOLEPKT_REPLY          95
#define NSM_MSG_IPV4_MRT_STAT_UPDATE             96
#define NSM_MSG_IPV4_MRIB_NOTIFICATION           97

#define NSM_MSG_IPV6_MIF_ADD                     98
#define NSM_MSG_IPV6_MIF_DEL                     99
#define NSM_MSG_IPV6_MRT_ADD                     100
#define NSM_MSG_IPV6_MRT_DEL                     101
#define NSM_MSG_IPV6_MRT_STAT_FLAGS_UPDATE       102
#define NSM_MSG_IPV6_MRT_NOCACHE                 103
#define NSM_MSG_IPV6_MRT_WRONGMIF                104
#define NSM_MSG_IPV6_MRT_WHOLEPKT_REQ            105
#define NSM_MSG_IPV6_MRT_WHOLEPKT_REPLY          106
#define NSM_MSG_IPV6_MRT_STAT_UPDATE             107
#define NSM_MSG_IPV6_MRIB_NOTIFICATION           108

#define NSM_MSG_MLD_LMEM                         109

#define NSM_MSG_BRIDGE_PORT_STATE_SYNC_REQ       113
#define NSM_MSG_STP_INTERFACE                    114
#define NSM_MSG_BRIDGE_ADD                       115
#define NSM_MSG_BRIDGE_DELETE                    116
#define NSM_MSG_BRIDGE_ADD_PORT                  117
#define NSM_MSG_BRIDGE_DELETE_PORT               118
#define NSM_MSG_LACP_AGGREGATE_ADD               119
#define NSM_MSG_LACP_AGGREGATE_DEL               120
#define NSM_MSG_VLAN_ADD                         121
#define NSM_MSG_VLAN_DELETE                      122
#define NSM_MSG_VLAN_ADD_PORT                    123
#define NSM_MSG_VLAN_DELETE_PORT                 124
#define NSM_MSG_VLAN_PORT_TYPE                   125

#define NSM_MSG_VLAN_CLASSIFIER_ADD              126
#define NSM_MSG_VLAN_CLASSIFIER_DEL              127
#define NSM_MSG_VLAN_PORT_CLASS_ADD              128
#define NSM_MSG_VLAN_PORT_CLASS_DEL              129

/* Bridge Port State message from xStp to NSM */
#define NSM_MSG_BRIDGE_PORT_STATE                130
#define NSM_MSG_BRIDGE_PORT_FLUSH_FDB            131
#define NSM_MSG_BRIDGE_TCN                       132
#define NSM_MSG_BRIDGE_SET_AGEING_TIME           133

#define NSM_MSG_VLAN_ADD_TO_INST                 134
#define NSM_MSG_VLAN_DELETE_FROM_INST            135

/* Port-based Authentication messages to NSM.  */
#define NSM_MSG_AUTH_PORT_STATE                  136

#define NSM_MSG_RSVP_CONTROL_PACKET              137

/* The following are to be used for callback purposes only. */
#define NSM_MSG_MPLS_FTN_DEL_REPLY               138
#define NSM_MSG_MPLS_ILM_ADD_REPLY               139
#define NSM_MSG_MPLS_ILM_DEL_REPLY               140
#define NSM_MSG_MPLS_VC_FTN_ADD_REPLY            141
#define NSM_MSG_MPLS_VC_FTN_DEL_REPLY            142
#define NSM_MSG_MPLS_VC_ILM_ADD_REPLY            143
#define NSM_MSG_MPLS_VC_ILM_DEL_REPLY            144
#define NSM_MSG_STATIC_AGG_CNT_UPDATE            145
#define NSM_MSG_LACP_AGGREGATOR_CONFIG           146
#define NSM_MSG_IF_DEL_DONE                      147
#define NSM_MSG_PVLAN_PORT_HOST_ASSOCIATE        148
#define NSM_MSG_RMON_REQ_STATS                   149
#define NSM_MSG_RMON_SERVICE_STATS               150
#define NSM_MSG_MPLS_FTN_ADD_REPLY               151

/* Mtrace messages */
#define NSM_MSG_MTRACE_REQUEST                   152
#define NSM_MSG_PVLAN_CONFIGURE                  153
#define NSM_MSG_PVLAN_SECONDARY_VLAN_ASSOCIATE   154
#define NSM_MSG_IPV6_MRT_ST_REFRESH_FLAG_UPDATE  155
#define NSM_MSG_IPV4_MRT_ST_REFRESH_FLAG_UPDATE  156
#define NSM_MSG_MTRACE_QUERY                     157
#define NSM_MSG_MACAUTH_PORT_STATE               158
#define NSM_MSG_AUTH_MAC_AUTH_STATUS             159

/* RSVP ILM lookup messages.*/
#define NSM_MSG_ILM_LOOKUP                       160
#define NSM_MSG_VC_ILM_ADD                       161
#define NSM_MSG_MPLS_ECHO_REQUEST                162
#define NSM_MSG_MPLS_PING_REPLY                  163
#define NSM_MSG_MPLS_TRACERT_REPLY               164
#define NSM_MSG_MPLS_OAM_ERROR                   165
#define NSM_MSG_MPLS_OAM_L3VPN                   166

#define NSM_MSG_SVLAN_ADD_CE_PORT                167
#define NSM_MSG_SVLAN_DELETE_CE_PORT             168

#define NSM_MSG_EFM_OAM_IF                       169

#define NSM_MSG_MPLS_VC_TUNNEL_INFO              170
#define NSM_MSG_OAM_LLDP                         171
#define NSM_MSG_VLAN_SET_PVID                    172
#define NSM_MSG_COMMSG                           173
#define NSM_MSG_SET_BPDU_PROCESS                 174
#define NSM_MSG_CFM_REQ_IFINDEX                  175
#define NSM_MSG_CFM_GET_IFINDEX                  176
#define NSM_MSG_OAM_CFM                          177
#define NSM_MSG_BRIDGE_SET_STATE                 178

/* As of now these bridge types are only being understood by ONMD but,
 * this is not being put in the HAVE_CFM flag as other modules might also
 * be desirous of this information */
#define NSM_MSG_PBB_ISID_TO_PIP_ADD              179
#define NSM_MSG_PBB_SVID_TO_ISID_DEL             180
#define NSM_MSG_PBB_ISID_TO_BVID_ADD             181
#define NSM_MSG_PBB_ISID_TO_BVID_DEL             182
#define NSM_MSG_PBB_ISID_DEL                     183
#define NSM_MSG_PBB_DISPATCH                     184

#define NSM_MSG_MPLS_OAM_ITUT_PROCESS_REQ        185
#define NSM_MSG_VR_SYNC_DONE                     186
#define NSM_MSG_ROUTE_STALE_MARK                 187
#define NSM_MSG_NEXTHOP_TRACKING                 188
#define NSM_MSG_ISIS_WAIT_BGP_SET                189
#define NSM_MSG_BGP_CONV_DONE                    190
#define NSM_MSG_ISIS_BGP_CONV_DONE               191
#define NSM_MSG_ISIS_BGP_UP                      192
#define NSM_MSG_ISIS_BGP_DOWN                    193

/* Messages for PBB-TE */
#define NSM_MSG_PBB_TE_VID_ADD                   194
#define NSM_MSG_PBB_TE_VID_DELETE                195
#define NSM_MSG_PBB_TE_ESP_ADD                   196
#define NSM_MSG_PBB_TE_ESP_DELETE                197
#define NSM_MSG_PBB_TE_ESP_PNP_ADD               198
#define NSM_MSG_PBB_TE_ESP_PNP_DELETE            199

/* messages for PBB_TE APS */
#define NSM_MSG_BRIDGE_PBB_TE_PORT_STATE         200
#define NSM_MSG_PBB_TE_APS_GRP_ADD               201
#define NSM_MSG_PBB_TE_APS_GRP_DELETE            202

/*Messages for G8031-EPS */
#define NSM_MSG_G8031_CREATE_PROTECTION_GROUP    203
#define NSM_MSG_G8031_CREATE_VLAN_GROUP          204
#define NSM_MSG_G8031_DEL_PROTECTION_GROUP       205
#define NSM_MSG_G8031_PG_INITIALIZED             206
#define NSM_MSG_G8031_PG_PORTSTATE               207

#define NSM_MSG_BRIDGE_ADD_G8032_RING            208
#define NSM_MSG_BRIDGE_DEL_G8032_RING            209
#define NSM_MSG_G8032_CREATE_VLAN_GROUP          210
#define NSM_MSG_BRIDGE_DEL_G8032_VLAN            211
#define NSM_MSG_BRIDGE_G8032_PORT_STATE          212

/* LACP Interface information to PM */
#define NSM_MSG_LACP_ADD_AGGREGATOR_MEMBER       213
#define NSM_MSG_LACP_DEL_AGGREGATOR_MEMBER       214

/* Protection group Message to MSTP from NSM */
#define NSM_MSG_VLAN_ADD_TO_PROTECTION           215
#define NSM_MSG_VLAN_DEL_FROM_PROTECTION         216

/* Enabling/Disabling STP on Protected Group. */
#define NSM_MSG_BRIDGE_ADD_PG                    217
#define NSM_MSG_BRIDGE_DEL_PG                    218
#define NSM_MSG_BLOCK_INSTANCE                   219
#define NSM_MSG_UNBLOCK_INSTANCE                 220

#define NSM_MSG_STALE_INFO                       221

/* Provider Edge Port */
#define NSM_MSG_UNTAGGED_VID_PE_PORT             222
#define NSM_MSG_DEFAULT_VID_PE_PORT              223
#define NSM_MSG_CFM_OPERATIONAL                  224

#define NSM_MSG_ELMI_EVC_NEW                     225
#define NSM_MSG_ELMI_EVC_DELETE                  226
#define NSM_MSG_ELMI_EVC_UPDATE                  227
#define NSM_MSG_ELMI_UNI_ADD                     228
#define NSM_MSG_ELMI_UNI_DELETE                  229
#define NSM_MSG_ELMI_UNI_UPDATE                  230
#define NSM_MSG_ELMI_UNI_BW_ADD                  231
#define NSM_MSG_ELMI_UNI_BW_DEL                  232
#define NSM_MSG_ELMI_EVC_BW_ADD                  233
#define NSM_MSG_ELMI_EVC_BW_DEL                  234
#define NSM_MSG_ELMI_EVC_COS_BW_ADD              235
#define NSM_MSG_ELMI_EVC_COS_BW_DEL              236
#define NSM_MSG_ELMI_OPERATIONAL_STATE           237
#define NSM_MSG_ELMI_AUTO_VLAN_ADD_PORT          238
#define NSM_MSG_ELMI_AUTO_VLAN_DEL_PORT          239

#define NSM_MSG_DATA_LINK                        240
#define NSM_MSG_DATA_LINK_SUB                    241
#define NSM_MSG_TE_LINK                          242
#define NSM_MSG_CONTROL_CHANNEL                  243
#define NSM_MSG_CONTROL_ADJ                      244
#define NSM_MSG_QOS_CLIENT_PROBE_RELEASE         245

#define NSM_MSG_GMPLS_FTN                        246
#define NSM_MSG_GMPLS_ILM                        247
#define NSM_MSG_GMPLS_BIDIR_FTN                  248
#define NSM_MSG_GMPLS_BIDIR_ILM                  249

#define NSM_MSG_GENERIC_LABEL_POOL_REQUEST       250
#define NSM_MSG_GENERIC_LABEL_POOL_RELEASE       251

#define NSM_MSG_ILM_GEN_LOOKUP                   252
#define NSM_MSG_MPLS_FTN_GEN_IPV4                NSM_MSG_GMPLS_FTN
#define NSM_MSG_MPLS_FTN_GEN_REPLY               254
#define NSM_MSG_MPLS_ILM_GEN_IPV4                NSM_MSG_GMPLS_ILM
#define NSM_MSG_MPLS_ILM_GEN_ADD_REPLY           256
#define NSM_MSG_MPLS_ILM_GEN_DEL_REPLY           257
#define NSM_MSG_MPLS_ILM_GEN_REPLY               258
#define NSM_MSG_MPLS_BIDIR_FTN_REPLY             259
#define NSM_MSG_MPLS_BIDIR_ILM_REPLY             260
#define NSM_MSG_GEN_STALE_INFO                   261

#define NSM_MSG_LMP_GRACEFUL_RESTART             262
#define NSM_MSG_DLINK_OPAQUE                     263

#define NSM_MSG_GENERIC_LABEL_POOL_IN_USE        264
#define NSM_MSG_PBB_TESID_INFO                   265

/* GMPLS QoS Messages */
#define NSM_MSG_GMPLS_QOS_CLIENT_INIT            266
#define NSM_MSG_GMPLS_QOS_CLIENT_PROBE           267
#define NSM_MSG_GMPLS_QOS_CLIENT_RESERVE         268
#define NSM_MSG_GMPLS_QOS_CLIENT_RELEASE         269
#define NSM_MSG_GMPLS_QOS_CLIENT_RELEASE_SLOW    270
#define NSM_MSG_GMPLS_QOS_CLIENT_PREEMPT         271
#define NSM_MSG_GMPLS_QOS_CLIENT_MODIFY          272
#define NSM_MSG_GMPLS_QOS_CLIENT_CLEAN           273
#define NSM_MSG_DLINK_SEND_TEST_MESSAGE          274

/* MPLS OAM Request/Reply Process message */
#define NSM_MSG_OAM_LSP_PING_REQ_PROCESS         275
#define NSM_MSG_OAM_LSP_PING_REP_PROCESS_LDP     276
#define NSM_MSG_OAM_LSP_PING_REP_PROCESS_GEN     277
#define NSM_MSG_OAM_LSP_PING_REP_PROCESS_RSVP    278
#define NSM_MSG_OAM_LSP_PING_REP_PROCESS_L2VC    279
#define NSM_MSG_OAM_LSP_PING_REP_PROCESS_L3VPN   280

/* MPLS OAM Response message */
#define NSM_MSG_OAM_LSP_PING_REQ_RESP_LDP        281
#define NSM_MSG_OAM_LSP_PING_REQ_RESP_GEN        282
#define NSM_MSG_OAM_LSP_PING_REQ_RESP_RSVP       283
#define NSM_MSG_OAM_LSP_PING_REQ_RESP_L2VC       284
#define NSM_MSG_OAM_LSP_PING_REQ_RESP_L3VPN      285
#define NSM_MSG_OAM_LSP_PING_REQ_RESP_ERR        286
#define NSM_MSG_OAM_LSP_PING_REP_RESP            287

/* MPLS OAM Update message for FTN/VC */
#define NSM_MSG_OAM_UPDATE                       288

/* MPLS FTN Down message to the FTN Owner. */
#define NSM_MSG_MPLS_FTN_DOWN                    289

#define NSM_MSG_MPLS_MS_PW                       290
#define NSM_MSG_DLINK_CHANNEL_MONITOR            291

/* Enable/Disable spanning-tree on port*/
#define NSM_MSG_BRIDGE_PORT_SPANNING_TREE_ENABLE 292

/* Message from NSM */
#define NSM_MSG_NSM_SERVER_STATUS                293

#define NSM_MSG_LDP_UP                           294
#define NSM_MSG_LDP_DOWN                         295
#define NSM_MSG_LDP_SESSION_STATE                296
#define NSM_MSG_LDP_SESSION_QUERY                297

#define NSM_MSG_MAX                              298

#ifdef HAVE_HA
/* Allowed messages to/from NSM in Standby */
#define HA_ALLOW_NSM_MSG_TYPE(TYPE)                                            \
      (((TYPE) == NSM_MSG_SERVICE_REQUEST)                                     \
        ||((TYPE) == NSM_MSG_SERVICE_REPLY))
#endif /* HAVE_HA */

#define NSM_DECODE_TLV_HEADER(TH)                                             \
    do {                                                                      \
      TLV_DECODE_GETW ((TH).type);                                            \
      TLV_DECODE_GETW ((TH).length);                                          \
      (TH).length -= NSM_TLV_HEADER_SIZE;                                     \
    } while (0)

#define NSM_CHECK_CTYPE(F,C)        (CHECK_FLAG (F, (1 << C)))
#define NSM_SET_CTYPE(F,C)          (SET_FLAG (F, (1 << C)))
#define NSM_UNSET_CTYPE(F,C)        (UNSET_FLAG (F, (1 << C)))

#define NSM_CHECK_CTYPE_FLAG(F,C)        (CHECK_FLAG (F, C))
#define NSM_SET_CTYPE_FLAG(F,C)          (SET_FLAG (F, C))
#define NSM_UNSET_CTYPE_FLAG(F,C)        (UNSET_FLAG (F, C))

#define NSM_CINDEX_SIZE                     MSG_CINDEX_SIZE

/* NSM Context Header
   0                   1                   2                   3
   0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |                             VR-ID                             |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |                            VRF-ID                             |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

   NSM Message Header
   0                   1                   2                   3
   0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |          Message Type         |           Message Len         |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |                          Message Id                           |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
*/

struct nsm_msg_header
{
  /* VR-ID. */
  u_int32_t vr_id;

  /* VRF-ID. */
  u_int32_t vrf_id;

  /* Message Type. */
  u_int16_t type;

  /* Message Len. */
  u_int16_t length;

  /* Message ID. */
  u_int32_t message_id;
};
#define NSM_MSG_HEADER_SIZE   16

/* NSM TLV Header
   0                   1                   2                   3
   0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |            Type               |             Length            |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
*/

struct nsm_tlv_header
{
  u_int16_t type;
  u_int16_t length;
};
#define NSM_TLV_HEADER_SIZE   4

/* NSM Service message format

  This message is used by:

  NSM_MSG_SERVICE_REQUEST
  NSM_MSG_SERVICE_REPLY

   0                   1                   2                   3
   0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |             Version           |             Reserved          |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |                          Protocol Id                          |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |                           Client Id                           |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |                            Services                           |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

  Graceful Restart TLV

   0                   1                   2                   3
   0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |             AFI1              |     SAFI1     |        1      |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |             ...               |     ...       |        .      |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |             AFIn              |     SAFIn     |        1      |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

  Grace Period Expires TLV

   0                   1                   2                   3
   0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |                     Grace Period Expires                      |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

Grace Period Expires:  Restart time - (Current time - Disconnect time)

Restart Option TLV
0                   1                   2                   3
0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|                       Restart Option ....
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

Restart Option:  Arbitrary value which require for restart.

*/

/* NSM services structure.  */
struct nsm_msg_service
{
  /* TLV flags. */
  cindex_t cindex;

  /* NSM Protocol Version. */
  u_int16_t version;

  /* Reserved. */
  u_int16_t reserved;

  /* Protocol ID. */
  u_int32_t protocol_id;

  /* Client Id. */
  u_int32_t client_id;

  /* Service Bits. */
  u_int32_t bits;

  /* Graceful Restart State */
  u_char restart_state;

  /* Graceful Restart TLV. */
  u_char restart[AFI_MAX][SAFI_MAX];

  /* Grace Perioud Expires TLV. */
  pal_time_t grace_period;

  /* Restart Option TLV.  */
  u_char *restart_val;
  u_int16_t restart_length;

#if (defined HAVE_MPLS || defined HAVE_GMPLS)
  /* Label pools used before restart. */
  struct nsm_msg_label_pool *label_pools;
  u_int16_t label_pool_num;
#endif /* HAVE_MPLS || HAVE_GMPLS */
};
#define NSM_MSG_SERVICE_SIZE     16

/* NSM interface CTYPEs.  */
#define NSM_SERVICE_CTYPE_RESTART            0
#define NSM_SERVICE_CTYPE_GRACE_EXPIRES      1
#define NSM_SERVICE_CTYPE_RESTART_OPTION     2
#define NSM_SERVICE_CTYPE_LABEL_POOLS        3

/* NSM interface message.

  This message is used by:

  NSM_MSG_INTERFACE_UPDATE
  NSM_MSG_INTERFACE_DELETE
  NSM_MSG_INTERFACE_UP
  NSM_MSG_INTERFACE_DOWN
  NSM_MSG_INTF_PRIORITY_BW_UPDATE

   0                   1                   2                   3
   0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |                         Interface Index                       |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

  Interface Name TLV
   0                   1                   2                   3
   0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |                                                               |
  |                        Interface Name                         |
  |                                                               |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

  Interface Flags TLV
   0                   1                   2                   3
   0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |                            Flags                              |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+


  Interface Metric TLV
   0                   1                   2                   3
   0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |                            Metric                             |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

  Interface MTU TLV
   0                   1                   2                   3
   0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |                             MTU                               |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

  Hardware Address TLV
   0                   1                   2                   3
   0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |          Hardware Type        |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |                      Hardware address length                  |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |                         Hardware address                      |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |
  +-+-+-+-+-+-+-+-+-------

  Interface bandwidth TLV
   0                   1                   2                   3
   0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |                            Bandwidth                          |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

  Max reservable bandwidth TLV
   0                   1                   2                   3
   0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |                      Max reservable bandwidth      1           |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |                      Max reservable bandwidth      2           |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |                                       ......                   |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |                      Max reservable bandwidth      8           |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

  Admin group TLV
   0                   1                   2                   3
   0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |                          Admin Group                          |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

  Label Space TLV
   0                   1                   2                   3
   0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
  +-+-+-+-+-+-+-+-+
  |     Status    |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |          Label Space          |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |                         Min Label Value                       |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |                         Max Lable Value                       |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

  Priority Bandwidth TLV
   0                   1                   2                   3
   0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |                      Priority Bandwidth 1                     |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |                              .....                            |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |                      Priority Bandwidth 8                     |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

  BW MODE TLV
   0                   1                   2                   3
   0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
  +-+-+-+-+-+-+-+-+
  |    BC MODE    |
  +-+-+-+-+-+-+-+-+

  VRX WRP/WRPJ INTERFACE VRX FLAG
   0                   1                   2                   3
   0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
  +-+-+-+-+-+-+-+-+
  |    VRX FLAG   |
  +-+-+-+-+-+-+-+-+

  LACP Admin Key
  0                   1                   2                   3
  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |          Lacp Admin Key       |  Agg param update           |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
*/


#define NSM_MSG_LINK_SIZE                        4

struct nsm_msg_link
{
  /* Ctype flags.  */
  cindex_t cindex;
#define NSM_LINK_CTYPE_NAME                      0
#define NSM_LINK_CTYPE_FLAGS                     1
#define NSM_LINK_CTYPE_METRIC                    2
#define NSM_LINK_CTYPE_MTU                       3
#define NSM_LINK_CTYPE_BANDWIDTH                 4
#define NSM_LINK_CTYPE_MAX_RESV_BANDWIDTH        5
#define NSM_LINK_CTYPE_ADMIN_GROUP               6
#define NSM_LINK_CTYPE_LABEL_SPACE               7
#define NSM_LINK_CTYPE_HW_ADDRESS                8
#define NSM_LINK_CTYPE_PRIORITY_BW              10
#define NSM_LINK_CTYPE_SCOPE                    11
#define NSM_LINK_CTYPE_BINDING                  12
#define NSM_LINK_CTYPE_BC_MODE                  13
#define NSM_LINK_CTYPE_BW_CONSTRAINT            14
#define NSM_LINK_CTYPE_VRX_FLAG                 15
#define NSM_LINK_CTYPE_VRX_FLAG_LOCALSRC        16
#define NSM_LINK_CTYPE_LACP_KEY                 17
#define NSM_LINK_CTYPE_DUPLEX                   18
#define NSM_LINK_CTYPE_ARP_AGEING_TIMEOUT       19
#define NSM_LINK_CTYPE_AUTONEGO                 20
#define NSM_LINK_CTYPE_STATUS                   21
#define NSM_LINK_CTYPE_LACP_AGG_PORT_ID         22
#define NSM_LINK_CTYPE_ALIAS                    23
#define NSM_LINK_CTYPE_DESC                     24
#define NSM_LINK_CTYPE_NW_ADDRESS               25
#ifdef HAVE_GMPLS
#define NSM_LINK_CTYPE_GMPLS_TYPE               26
#define NSM_LINK_CTYPE_GMPLS_PROP               27
#endif /* HAVE_GMPLS */

  /* Interface index.  */
  s_int32_t ifindex;

#ifdef HAVE_GMPLS
  /* GMPLS Interface index.  */
  s_int32_t gifindex;
#endif /*HAVE_GMPLS */

  /* Interface information.  */
  int ifname_len;
  char *ifname;
  u_int32_t flags;
  u_int32_t status;
  u_int32_t metric;
  u_int32_t mtu;
  u_int32_t duplex;
  u_int32_t autonego;
  u_int32_t arp_ageing_timeout;

  /* Bandwidth information.  */
  float32_t bandwidth;

#ifdef HAVE_TE
  /* Traffic engineering information.  */
  float32_t max_resv_bw;

#ifdef HAVE_DSTE
  float32_t bw_constraint[MAX_BW_CONST];
#endif /* HAVE_DSTE */

  u_int32_t admin_group;
#endif /* HAVE_TE */

#ifdef HAVE_MPLS
  /* Label space information.  */
  struct label_space_data lp;
#endif /* HAVE_MPLS */

  /* Hardware address.  */
  u_int16_t hw_type;
  u_int32_t hw_addr_len;
  u_char hw_addr[INTERFACE_HWADDR_MAX];

#ifdef HAVE_TE
  /* Priority Bandwidth.  */
  float32_t tecl_priority_bw[MAX_PRIORITIES];
#endif /* HAVE_TE */

  /* Binding information */
  u_char bind;

#ifdef HAVE_DSTE
  /* BW Constrain mode */
  bc_mode_t bc_mode;
#endif /* HAVE_DSTE */

#ifdef HAVE_VRX
  /* VRX WRP/WRPJ interface flag. */
  u_char vrx_flag;
  u_char local_flag;
#endif /* HAVE_VRX */

#ifdef HAVE_LACPD
  u_int16_t lacp_admin_key;
  u_int16_t agg_param_update;
  u_int32_t lacp_agg_key;
#endif /* HAVE_LACPD */

  struct pal_in4_addr nw_addr;

#ifdef HAVE_GMPLS
  u_char gtype;
  struct phy_properties phy_prop;
#endif /* HAVE_GMPLS */
};

/* NSM address message.

This message is used by:

NSM_MSG_ADDR_ADD
NSM_MSG_ADDR_DELETE

0                   1                   2                   3
0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|                           Interface Index                       |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|     Flags     |    Prefixlen  |              AFI              |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|                        Interface address                        |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|                       Destination address                       |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

*/

#define NSM_MSG_ADDRESS_SIZE                    16

struct nsm_msg_address
{
  /* Ctype index.  */
  cindex_t cindex;

  /* Interface index.  */
  u_int32_t ifindex;

  /* Flags of the address.  */
  u_char flags;

  /* Prefix length.  */
  u_char prefixlen;

  /* Address Family Identifier.  */
  afi_t afi;

  /* IP address.  */
  union
  {
    struct
    {
      struct pal_in4_addr src;
      struct pal_in4_addr dst;
    } ipv4;

#ifdef HAVE_IPV6
    struct
    {
      struct pal_in6_addr src;
      struct pal_in6_addr dst;
    } ipv6;
#endif /* HAVE_IPV6 */

  } u;
};

/* NSM VR Add message.

This message is used by:

NSM MSG_VR_ADD
NSM MSG_VR_DELETE

0                   1                   2                   3
0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|                             VR ID                             |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|                                                               |
|                            VR Name (Variable Length)          |
|                                                               |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

*/

#define NSM_MSG_VR_SIZE         4

struct nsm_msg_vr
{
  cindex_t cindex;

  /* VR ID. */
  u_int32_t vr_id;

  /* VR name. */
  int len;
  char *name;
};

/* NSM VR Bind message.

   This message is used by:

   NSM_MSG_VR_BIND
   NSM_MSG_VR_BIND_BULK
   NSM_MSG_VR_UNBIND

   0                   1                   2                   3
   0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |                             VR ID                             |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |                   Physical Interface Index 0                  |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |                              ...                              |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |                   Physical Interface Index n                  |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

*/

#define NSM_MSG_VR_BIND_SIZE                    8

struct nsm_msg_vr_bind
{
  /* Ctype flags. */
  cindex_t cindex;

  /* VR ID. */
  u_int32_t vr_id;

  /* Interface index. */
  s_int32_t ifindex;
};

/* NSM VRF Bind message.

   This message is used by:

   NSM_MSG_VRF_BIND
   NSM_MSG_VRF_BIND_BULK
   NSM_MSG_VRF_UNBIND

   0                   1                   2                   3
   0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |                             VRF ID                            |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |                   Physical Interface Index 0                  |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |                              ...                              |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |                   Physical Interface Index n                  |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
*/

#define NSM_MSG_VRF_BIND_SIZE                   8

struct nsm_msg_vrf_bind
{
  /* Ctype flags. */
  cindex_t cindex;

  /* VRF ID. */
  u_int32_t vrf_id;

  /* Interface index. */
  s_int32_t ifindex;
};

/* NSM wait for bgp message */
#define NSM_MSG_WAIT_FOR_BGP_SIZE                1

struct nsm_msg_wait_for_bgp
{
 /* set/unset flag */
 u_int16_t flag;
};

#define NSM_MSG_LDP_SESSION_STATE_SIZE            8
#define NSM_MSG_LDP_SESSION_STATE_DOWN            0
#define NSM_MSG_LDP_SESSION_STATE_UP              1

struct nsm_msg_ldp_session_state
{
 u_int32_t state;
 u_int32_t ifindex;
};

/* NSM route message.

This message is used by:

NSM_MSG_IPV4_ROUTE

0                   1                   2                   3
0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|             Bits      |M|R|B|A|              Length           |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|       Type    |   Prefixlen   |    Distance   |    Sub type   |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|                               Metric                            |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|                          IPv4 Prefix                          |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

Flags:
A flag for add.
B flag for blackhole (aka discard route).
R flag for resolve next hop.
M flag for multicast.

Length:
Total length of this message including header.

Type:
Route type.

Prefixlen:
Prefix length.

Distance:
Distance value.

Sub Type:
Route sub type.

IPv4 Next Hop TLV
0                   1                   2                   3
0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|                           Interfce Index                        |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|                       IPv4 Next Hop Address                     |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

Tag TLV
0                   1                   2                   3
0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-++-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|                               Tag                              |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-++-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

Process ID
0                   1                   2                   3
0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-++-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|                            Process ID                          |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-++-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

*/

/*

NSM_MSG_IPV6_ROUTE

0                   1                   2                   3
0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|             Bits      |M|R|B|A|              Length           |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|       Type    |   Prefixlen   |    Distance   |    Sub type   |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|                               Metric                            |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|                          IPv6 Prefix                          |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|                          IPv6 Prefix                          |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|                          IPv6 Prefix                          |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|                          IPv6 Prefix                          |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

Flags:
A flag for add.
B flag for blackhole.
R flag for resolve next hop.
M flag for multicast.

Length:
Total length of this message including header.

Type:
Route type.

Prefixlen:
Prefix length.

Distance:
Distance value.

Sub Type:
Route sub type.

IPv6 Next Hop TLV
0                   1                   2                   3
0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|                           Interfce Index                        |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|                       IPv6 Next Hop Address                     |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|                       IPv6 Next Hop Address                     |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|                       IPv6 Next Hop Address                     |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|                       IPv6 Next Hop Address                     |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

Tag TLV
0                   1                   2                   3
0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-++-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|                               Tag                            |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-++-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

Process ID
0                   1                   2                   3
0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-++-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|                            Process ID                        |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-++-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

*/

#define NSM_MSG_ROUTE_FLAG_ADD             (1 << 0)
#define NSM_MSG_ROUTE_FLAG_BLACKHOLE       (1 << 1)
#define NSM_MSG_ROUTE_FLAG_RESOLVE         (1 << 2)
#define NSM_MSG_ROUTE_FLAG_MULTICAST       (1 << 3)

#if defined HAVE_6PE || defined HAVE_VRF
#define NSM_MSG_ROUTE_FLAG_IPV4NEXTHOP     (1 << 4)
#endif /* HAVE_6PE  || HAVE_VRF */

/* For Nexthop tracking */
#define NSM_MSG_ROUTE_FLAG_NH_CHANGE       (1 << 5)

struct nsm_tlv_ipv4_nexthop
{
  u_int32_t ifindex;
  struct pal_in4_addr addr;
};

/* Default nexthop num for communication.  */
#define NSM_TLV_IPV4_NEXTHOP_NUM      64

#ifdef HAVE_VRF
struct nsm_ospf_domain_info
{
  /* OSPF Domain_id */
  u_char domain_id[8];
  /* OSPF route_type */
  u_char r_type;
  /* OSPF area_id */
  struct pal_in4_addr area_id;
  /* OSPF router_id */
  struct pal_in4_addr router_id;
  /*OSPF metric type */
  u_char metric_type_E2;
};
#endif /* HAVE_VRF */

/* IPv4 route.  */
struct nsm_msg_route_ipv4
{
  /* Ctype index.  */
  cindex_t cindex;

  /* Flags.  */
  u_int16_t flags;

  /* Route type.  */
  u_char type;

  /* Distance.  */
  u_char distance;

  /* Sub type.  */
  u_char sub_type;

  /* Metric.  */
  u_int32_t metric;

  u_char prefixlen;
  struct pal_in4_addr prefix;

  /* Next hop information.  */
  u_char nexthop_num;
  struct nsm_tlv_ipv4_nexthop nexthop[NSM_TLV_IPV4_NEXTHOP_NUM];
  struct nsm_tlv_ipv4_nexthop *nexthop_opt;

  /* Tag.  */
  u_int32_t tag;

  /* Process id.  */
  u_int32_t pid;

#ifdef HAVE_VRF
  /* OSPF Domain info */
  struct nsm_ospf_domain_info domain_info;
#endif /* HAVE_VRF */

#ifdef HAVE_MPLS
  /* MPLS data. */
  u_char    mpls_flags;
  u_char    mpls_out_protocol;
  u_int32_t mpls_oif_ix;
  u_int32_t mpls_out_label;
  struct pal_in4_addr mpls_nh_addr;
  u_int32_t mpls_ftn_ix;
#endif /* HAVE_MPLS */
};


#define NSM_MSG_ROUTE_IPV4_SIZE     16

#define NSM_ROUTE_CTYPE_IPV4_NEXTHOP      0
#define NSM_ROUTE_CTYPE_MPLS_IPV4_NHOP    1
#define NSM_ROUTE_CTYPE_IPV4_TAG          2
#define NSM_ROUTE_CTYPE_IPV4_PROCESS_ID   3
#define NSM_ROUTE_CTYPE_DOMAIN_INFO       4
#define NSM_ROUTE_CTYPE_OSPF_ROUTER_ID    5
#define NSM_ROUTE_CTYPE_DEFAULT           6

#define NSM_MSG_NEXTHOP4_NUM_SET(M,C)                                         \
    do {                                                                      \
      (M).nexthop_num = (C);                                                  \
      if ((M).nexthop_num > NSM_TLV_IPV4_NEXTHOP_NUM)                         \
        (M).nexthop_opt =                                                     \
          XCALLOC (MTYPE_NSM_MSG_NEXTHOP_IPV4,                                \
                   (M).nexthop_num * sizeof (struct nsm_tlv_ipv4_nexthop));   \
    } while (0)

#define NSM_MSG_NEXTHOP4_IFINDEX_SET(M,I,X)                                   \
    do {                                                                      \
      if ((M).nexthop_num > NSM_TLV_IPV4_NEXTHOP_NUM)                         \
        (M).nexthop_opt[I].ifindex = (X);                                     \
      else                                                                    \
        (M).nexthop[I].ifindex = (X);                                         \
    } while (0)

#define NSM_MSG_NEXTHOP4_ADDR_SET(M,I,A)                                      \
    do {                                                                      \
      if ((M).nexthop_num > NSM_TLV_IPV4_NEXTHOP_NUM)                         \
        (M).nexthop_opt[I].addr = (A);                                        \
      else                                                                    \
        (M).nexthop[I].addr = (A);                                            \
    } while (0)

#define NSM_MSG_NEXTHOP4_FINISH(M)                                            \
    do {                                                                      \
      if ((M).nexthop_num > NSM_TLV_IPV4_NEXTHOP_NUM)                         \
        XFREE (MTYPE_NSM_MSG_NEXTHOP_IPV4, (M).nexthop_opt);                  \
    } while (0)

#ifdef HAVE_IPV6
#define NSM_TLV_IPV6_NEXTHOP_NUM       4

struct nsm_tlv_ipv6_nexthop
{
  u_int32_t ifindex;
  struct pal_in6_addr addr;
};

/* IPv6 route.  */
struct nsm_msg_route_ipv6
{
  /* Ctype index.  */
  cindex_t cindex;

  /* Flags.  */
  u_int16_t flags;

  /* Route type.  */
  u_char type;

  /* Distance.  */
  u_char distance;

  /* Sub type.  */
  u_char sub_type;

  /* Metric.  */
  u_int32_t metric;

  u_char prefixlen;
  struct pal_in6_addr prefix;

  /* Next hop information.  */
  u_char nexthop_num;
  struct nsm_tlv_ipv6_nexthop nexthop[NSM_TLV_IPV6_NEXTHOP_NUM];
  struct nsm_tlv_ipv6_nexthop *nexthop_opt;

  /* Tag.  */
  u_int32_t tag;

  /* Process id.  */
  u_int32_t pid;

#ifdef HAVE_MPLS
  /* MPLS data. */
  u_int32_t mpls_oif_ix;
  u_int32_t mpls_out_label;
  struct pal_in6_addr mpls_nh_addr;
#endif /* HAVE_MPLS */
};

#define NSM_MSG_ROUTE_IPV6_SIZE    28

#define NSM_ROUTE_CTYPE_IPV6_NEXTHOP      0
#define NSM_ROUTE_CTYPE_IPV6_VRF_ID       1
#define NSM_ROUTE_CTYPE_MPLS_IPV6_NHOP    2
#define NSM_ROUTE_CTYPE_IPV6_TAG          3
#define NSM_ROUTE_CTYPE_IPV6_PROCESS_ID   4

#define NSM_MSG_NEXTHOP6_NUM_SET(M,C)                                         \
    do {                                                                      \
      (M).nexthop_num = (C);                                                  \
      if ((M).nexthop_num > NSM_TLV_IPV6_NEXTHOP_NUM)                         \
        (M).nexthop_opt =                                                     \
          XCALLOC (MTYPE_NSM_MSG_NEXTHOP_IPV6,                                \
                   (M).nexthop_num * sizeof (struct nsm_tlv_ipv6_nexthop));   \
    } while (0)

#define NSM_MSG_NEXTHOP6_IFINDEX_SET(M,I,X)                                   \
    do {                                                                      \
      if ((M).nexthop_num > NSM_TLV_IPV6_NEXTHOP_NUM)                         \
        (M).nexthop_opt[I].ifindex = (X);                                     \
      else                                                                    \
        (M).nexthop[I].ifindex = (X);                                         \
    } while (0)

#define NSM_MSG_NEXTHOP6_ADDR_SET(M,I,A)                                      \
    do {                                                                      \
      if ((M).nexthop_num > NSM_TLV_IPV6_NEXTHOP_NUM)                         \
        (M).nexthop_opt[I].addr = (A);                                        \
      else                                                                    \
        (M).nexthop[I].addr = (A);                                            \
    } while (0)

#define NSM_MSG_NEXTHOP6_FINISH(M)                                            \
    do {                                                                      \
      if ((M).nexthop_num > NSM_TLV_IPV6_NEXTHOP_NUM)                         \
        XFREE (MTYPE_NSM_MSG_NEXTHOP_IPV6, (M).nexthop_opt);                  \
    } while (0)

#endif /* HAVE_IPV6 */

/* NSM redistribute message.

This message is used by:

NSM_MSG_REDISTRIBUTE_SET
NSM_MSG_REDISTRIBUTE_UNSET

0                   1                   2                   3
0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|   Route Type  |              AFI              |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

Route Type
Redistribute route type.  Route type zero means default route.

*/

struct nsm_msg_redistribute
{
  /* Ctype index. */
  cindex_t cindex;

  /* Route type. */
  u_char type;

  /* Process id.  */
  u_int32_t pid;

  /* AFI */
  afi_t afi;
};

/* Minimum length of redistribute message.  */
#define NSM_MSG_REDISTRIBUTE_SIZE              1

/* NSM redistribute CTYPEs.  */

/* NSM IPv4 nexthop lookup message

   This message is used by:

   NSM_MSG_IPV4_NEXTHOP_BEST_LOOKUP
   NSM_MSG_IPV4_NEXTHOP_EXACT_LOOKUP
   NSM_MSG_IPV4_NEXTHOP_BEST_LOOKUP_REG
   NSM_MSG_IPV4_NEXTHOP_BEST_LOOKUP_DEREG
   NSM_MSG_IPV4_NEXTHOP_EXACT_LOOKUP_REG
   NSM_MSG_IPV4_NEXTHOP_EXACT_LOOKUP_DEREG

   0                   1                   2                   3
   0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |                           IPv4 address                        |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

  Prefix length TLV
   0                   1                   2                   3
   0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
  +-+-+-+-+-+-+-+-+
  | prefix length |
  +-+-+-+-+-+-+-+-+

  Lookup LSP TLV

Message is empty

*/

#define BEST_MATCH_LOOKUP                      0
#define EXACT_MATCH_LOOKUP                     1

struct nsm_msg_route_lookup_ipv4
{
  /* Ctype index.  */
  cindex_t cindex;

  struct pal_in4_addr addr;

  u_char prefixlen;
};

#define NSM_MSG_IPV4_ROUTE_LOOKUP_SIZE       4

/* Nexthop lookup CTYPEs.  */
#define NSM_ROUTE_LOOKUP_CTYPE_PREFIXLEN     0
#define NSM_ROUTE_LOOKUP_CTYPE_LSP           1
#define NSM_ROUTE_LOOKUP_CTYPE_LSP_PROTO     2

#ifdef HAVE_IPV6
/* NSM IPv6 nexthop lookup message

   This message is used by:

   NSM_MSG_IPV6_NEXTHOP_EXACT_LOOKUP
   NSM_MSG_IPV6_NEXTHOP_BEST_LOOKUP
   NSM_MSG_IPV6_NEXTHOP_BEST_LOOKUP_REG
   NSM_MSG_IPV6_NEXTHOP_BEST_LOOKUP_DEREG
   NSM_MSG_IPV6_NEXTHOP_EXACT_LOOKUP_REG
   NSM_MSG_IPV6_NEXTHOP_EXACT_LOOKUP_DEREG

   0                   1                   2                   3
   0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |                           IPv6 address                        |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |                           IPv6 address                        |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |                           IPv6 address                        |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |                           IPv6 address                        |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

   Prefix length TLV
   0                   1                   2                   3
   0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
  +-+-+-+-+-+-+-+-+
  | prefix length |
  +-+-+-+-+-+-+-+-+

  Lookup LSP TLV

  Message is empty

*/

struct nsm_msg_route_lookup_ipv6
{
  /* Ctype index.  */
  cindex_t cindex;

  struct pal_in6_addr addr;

  u_char prefixlen;
};

#define NSM_MSG_IPV6_ROUTE_LOOKUP_SIZE       16

#endif /* HAVE_IPV6 */

/* NSM VRF messages

   This message is used by:

   NSM_MSG_VRF_ADD
   NSM_MSG_VRF_DELETE

   0                   1                   2                   3
   0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |                         VRF ID                                |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |                         FIB ID                                |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

   VRF Name TLV
   0                   1                   2                   3
   0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |                                                               |
  |                        VRF  Name                              |
  |                                                               |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

*/

#define NSM_MSG_VRF_SIZE                      8

struct nsm_msg_vrf
{
  /* Ctype index */
  cindex_t cindex;
#define NSM_VRF_CTYPE_NAME                    0

  /* VRF ID. */
  vrf_id_t vrf_id;

  /* FIB ID.  */
  u_int32_t fib_id;

  /* VRF Name. */
  int len;
  char *name;
};

/* NSM Route Management message.

   This message is used by:

   NSM_MSG_ROUTE_PRESERVE
   NSM_MSG_ROUTE_STALE_REMOVE

   Restart Time TLV
   0                   1                   2                   3
   0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |                         Restart Time                          |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

   AFI/SAFI TLV

   0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |          AFI1                 |     SAFI1     |    Reserved   |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

   Protocol ID TLV

   0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |                         Protocol ID                           |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

*/

struct nsm_msg_route_manage
{
  cindex_t cindex;

  int restart_time;

  afi_t afi;
  safi_t safi;

  u_char *restart_val;
  u_int16_t restart_length;

  u_int32_t protocol_id;

#if defined HAVE_RESTART && defined HAVE_MPLS
  u_int32_t oi_ifindex;
  u_int32_t flags;
#define NSM_MSG_ILM_FORCE_STALE_DEL        (1 << 0)
#define NSM_MSG_HELPER_NODE_STALE_MARK     (1 << 1)
#define NSM_MSG_HELPER_NODE_STALE_UNMARK   (1 << 2)
#endif /* HAVE_RESTART  && HAVE_MPLS */
};

#define NSM_ROUTE_MANAGE_CTYPE_RESTART_TIME     0
#define NSM_ROUTE_MANAGE_CTYPE_AFI_SAFI         1
#define NSM_ROUTE_MANAGE_CTYPE_RESTART_OPTION   2
#define NSM_ROUTE_MANAGE_CTYPE_PROTOCOL_ID      3
#define NSM_ROUTE_MANAGE_CTYPE_VRF_ID           4
#define NSM_ROUTE_MANAGE_CTYPE_MPLS_FLAGS       5

#define NSM_MSG_ROUTE_MANAGE_SIZE               0

/* NSM USER messages

   This message is used by:

   NSM_MSG_USER_ADD
   NSM_MSG_USER_UPDATE
   NSM_MSG_USER_DELETE

   0                   1                   2                   3
   0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |   Privilege   |     Flags     |         Username Len          |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |                    Username(Variable Length)                  |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

   Password TLV
   0                   1                   2                   3
   0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |                                                               |
  |                  Password(Variable Length)                    |
  |                                                               |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

   Encrypted password TLV
   0                   1                   2                   3
   0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |                                                               |
   |               Encypted password(Variable Length)              |
   |                                                               |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

*/

#define NSM_MSG_USER_SIZE                       2

struct nsm_msg_user
{
  /* Ctype index */
  cindex_t cindex;
#define NSM_USER_CTYPE_PASSWORD                 0
#define NSM_USER_CTYPE_PASSWORD_ENCRYPT         1

  u_char privilege;

  int username_len;
  u_char *username;

  int password_len;
  u_char *password;

  int password_encrypt_len;
  u_char *password_encrypt;
};



#ifdef HAVE_MPLS

/* Label set action types */
#define ACTION_TYPE_INCLUDE_LIST    (1 << 0)
#define ACTION_TYPE_EXCLUDE_LIST    (1 << 1)
#define ACTION_TYPE_INCLUDE_RANGE   (1 << 2)
#define ACTION_TYPE_EXCLUDE_RANGE   (1 << 3)

/* Action type indices into array */
#define INCL_LIST_INDEX   0
#define EXCL_LIST_INDEX   1
#define INCL_RANGE_INDEX  2
#define EXCL_RANGE_INDEX  3
/* To indicate the max number of action type indices 0 thru 3 */
#define ACTION_TYPE_MIN             0
#define ACTION_TYPE_MAX             4

/* Indices and max number of block based operations for label sets */
#define INCL_BLK_TYPE        0
#define EXCL_BLK_TYPE        1
#define LSO_BLK_TYPE_MIN     0
#define LSO_BLK_TYPE_MAX     2
#define LSO_BLK_TYPE_NONE    3

/* Client to server -- service request types */
#define BLK_REQ_TYPE_NONE                0
#define BLK_REQ_TYPE_SUGGESTED_LABEL     1
#define BLK_REQ_TYPE_EXPLICIT_LABEL      2
#define BLK_REQ_TYPE_GET_LS_BLOCK        3

struct nsm_msg_pkt_block_list
{
  /* Indicate the label set operation to be performed on the data */
  u_int8_t   action_type;

  /* Indicate the label request type */
  u_int8_t   blk_req_type;

  u_int32_t  blk_count;
  u_int32_t  *lset_blocks;
};

/* NSM Label Pool Message.

   NSM_MSG_LABEL_POOL_REQUEST
   NSM_MSG_LABEL_POOL_RELEASE

   0                   1                   2                   3
   0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |           Label Space         |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

  Table Block TLV

   0                   1                   2                   3
   0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |                          Label Block                          |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

  Label Range TLV
   0                   1                   2                   3
   0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |                         Minimum Label                         |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |                         Maximum Label                         |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
*/

struct nsm_msg_label_pool
{
  /* Ctype index */
  cindex_t cindex;

  /* Label space. */
  u_int16_t label_space;

  /* Label block.  */
  u_int32_t label_block;

  /* Minium and maximum label.  */
  u_int32_t min_label;
  u_int32_t max_label;

  /* Indicates label Range service owner types or clients that can request
   * a block within a range in the label space. This is a bit field as
   * defined in enum label_pool_id
   */
  u_int8_t range_owner;

  /* Provide the label set for the server to allocate a block that meets the
   * label set criteria or return an acceptable label set upon an unsuccessful
   * label set request */
  /* Have two block lists -- for include and exclude cases, inclusive of
   * all cases of lists and ranges used by both client and server */
  struct nsm_msg_pkt_block_list blk_list[2];

};

#ifdef HAVE_PACKET
struct nsm_msg_ilm_lookup {
  mpls_owner_t owner;
  u_char    opcode;
  u_int32_t ilm_in_ix;
  u_int32_t ilm_in_label;
  u_int32_t ilm_out_ix;
  u_int32_t ilm_out_label;
  u_int32_t ilm_ix;
};

#endif /* HAVE_PACKET */

struct nsm_msg_ilm_gen_lookup {
  mpls_owner_t owner;
  u_char    opcode;
  u_int32_t ilm_in_ix;
  struct gmpls_gen_label ilm_in_label;
  u_int32_t ilm_out_ix;
  struct gmpls_gen_label ilm_out_label;
  u_int32_t ilm_ix;
};

/* NSM VRF CTYPEs.  */
#define NSM_LABEL_POOL_CTYPE_LABEL_BLOCK             0
#define NSM_LABEL_POOL_CTYPE_LABEL_RANGE             1
#define NSM_LABEL_POOL_CTYPE_BLOCK_LIST              2

#define NSM_MSG_LABEL_POOL_SIZE                      2
#endif /* HAVE_MPLS */

#ifdef HAVE_GMPLS

#define MAC_ADDR_LEN          6
#define MAX_TRUNK_NAME_LEN    16

#ifdef HAVE_PBB_TE

#define NSM_LABEL_POOL_CTYPE_PBB_TE_LABEL  1

#define NSM_PBB_TE_TESI_IS_PASSIVE         -602
#define NSM_PBB_TE_BVID_POOL_EXHAUSTED     -603
#define NSM_PBB_TE_BVID_NOT_CONFIGURED     -604
#define NSM_PBB_TE_BVID_INVALID            -605
#define NSM_PBB_TE_LABEL_EXISTS            -606
#define NSM_PBB_TE_MAC_ADDRESS_INVALID     -607
#define NSM_PBB_TE_TESI_VID_INVALID        -608
#define NSM_PBB_TE_TESI_NAME_EXISTS        -609
#define NSM_PBB_TE_STATIC_ESP_INVALID      -610
#define NSM_PBB_TE_ENTRY_IN_USE            -611
#define NSM_PBB_TE_TESI_IS_STATIC          -612
#define NSM_PBB_TE_TESI_IS_DYNAMIC         -613
#define NSM_PBB_TE_ENTRY_INVALID_NAME      -614
#define NSM_PBB_TE_TESI_IS_ACTIVE          -615

enum label_request_type
{
  label_request_ingress       = 0,
  label_request_transit       = 1,
  label_request_egress        = 2,

  label_request_max           = label_request_egress
};

struct nsm_glabel_pbb_te_service_data
{
  /* PBB-TE label validate/request is at Ingress/Egress/Transit */
  enum label_request_type lbl_req_type;

  /* For getting back the pbb-te label in case of label request */
  struct pbb_te_label pbb_label;

  /* used for sending the TESI Name or Remote MAC or Inteface Index */
  union
  {
    char trunk_name[MAX_TRUNK_NAME_LEN+1];
    u_int32_t ifIndex;
  }data;

  /* Used for getting back the tesid */
  u_int32_t tesid;

  /* This field is used to return back to the caller whether the PBB_TE operation
   * of request/validate was SUCCESS/FAILURE
   */
  int result;
};
#endif /* HAVE_PBB_TE */

struct nsm_msg_generic_label_pool
{
  /* Ctype index */
  cindex_t cindex;

  /* Label space. */
  u_int16_t label_space;

  /* Service owner value requesting the operation as in enum label_pool_id */
  u_int8_t service_owner;

  union
  {
#ifdef HAVE_PBB_TE
    struct nsm_glabel_pbb_te_service_data *pb_te_service_data;
#endif /* HAVE_PBB_TE */
  }u;
};

#ifdef HAVE_PBB_TE
struct nsm_msg_tesi_info
{
  #define NSM_CTYPE_TESI_ACTION   1
  cindex_t cindex;
  u_char  tesi_name[MAX_TRUNK_NAME_LEN+1];
  u_int32_t tesid;

  #define TESI_CREATE             1
  #define TESI_DELETE             2
  #define TESI_MODIFY             3
  u_char action; /* denotes whether this is a TESI create/delete action */

  #define TESI_MODE_PASSIVE       1
  #define TESI_MODE_ACTIVE        2
  u_char mode;    /* shows whether the TESI is ACTIVE or PASSIVE */
};
#endif /* HAVE_PBB_TE */
#endif /* HAVE_GMPLS */

#ifdef HAVE_TE
/*
   NSM QOS Initialization message

   NSM_MSG_QOS_CLIENT_INIT

   Protocol ID TLV
   0                   1                   2                   3
   0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |                        Protocol ID                           |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

*/
#define NSM_MSG_QOS_CLIENT_INIT_SIZE                 4

/*
  NSM QOS message

  NSM_MSG_QOS_CLIENT_RESERVE
  NSM_MSG_QOS_CLIENT_PROBE
  NSM_MSG_QOS_CLIENT_MODIFY
  NSM_MSG_QOS_CLIENT_PREEMPT

  Resource ID TLV
  0                   1                   2                   3
  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |                        Resource ID                            |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |                         Protocol ID                           |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

  Setup priority TLV
  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  | Setup prio    |
  +-+-+-+-+-+-+-+-+

  Hold priority TLV
  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |   Hold prio   |
  +-+-+-+-+-+-+-+-+

  QOS Tspec TLV
  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  | Service type  | Exclusive flag|
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

  QOS Tspec minimum policed unit sub-TLV
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |                   Minimum policed unit                        |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

  QOS Tspec maximum packet size sub-TLV
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |                   Maximum packet size                         |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

  QOS Tspec Peak rate sub-TLV
  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |                        Peak rate                              |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

  QOS Tspec bucket rate sub-TLV
  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |                        Token rate                             |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

  QOS Tspec bucket size sub-TLV
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |                        Bucket size                            |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

  QOS Tspec excess burst size sub-TLV
  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |                        Excess burst size                      |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

  QOS Tspec weight sub-TLV
  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |                          Weight                               |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+


  QOS IF spec ifindex TLV
  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |                       In interface index                      |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |                      Out interface index                      |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

  QOS IF spec IPv4 address TLV
  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |                   Previous hop IPv4 address                   |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |                     Next hop IPv4 address                     |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+


  QOS AD Spec slack sub-TLV (Optional)
  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |                           Slack                               |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

  QOS AD spec compose total sub-TLV (Optional)
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |                      Compose Total                            |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

  QOS AD spec derived total sub-TLV (Optional)
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |                      Derived Total                            |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

  QOS AD spec composed sum sub-TLV (Optional)
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |                      Composed Sum                             |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

  QOS AD spec derived sum sub-TLV (Optional)
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |                      Derived Sum                              |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

  QOS AD spec minimum path latency sub-TLV (Optional)
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |                   Minimum Path Latency                        |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

  QOS AD spec path bandwidth estimate sub-TLV (Optional)
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |                   Path Bandwidth Estimate                     |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

*/

/* Tspec. */
struct nsm_msg_qos_t_spec
{
  /* Cindex. */
  cindex_t cindex;

  /* Service type. */
  u_int8_t service_type;

  /* Exclusive or not (one or zero). */
  u_int8_t is_exclusive;

  /* Minimum policed unit. */
  u_int32_t min_policed_unit;

  /* Maximum packet size. */
  u_int32_t max_packet_size;

  /* Token peak rate. */
  float peak_rate;

  /* Token rate(bytes per second). */
  float rate;

  /* Bucket size(bytes). */
  float size;

  /* Excess burst size for committed rate. */
  float excess_burst;

  /* Indicates the proportion of this flow's excess bandwidth
   * from the overall excess bandwidth. */
  float weight;
};

#define NSM_MSG_QOS_T_SPEC_SIZE                        2

/* if_spec. */
struct nsm_msg_qos_if_spec_ifindex
{
  u_int32_t in;
  u_int32_t out;
};

struct nsm_msg_qos_if_spec_addr
{
  struct pal_in4_addr prev_hop;
  struct pal_in4_addr next_hop;
};

struct nsm_msg_qos_if_spec
{
  /* Cindex. */
  cindex_t cindex;

  struct nsm_msg_qos_if_spec_ifindex ifindex;

  struct nsm_msg_qos_if_spec_addr addr;
};

#define NSM_MSG_QOS_IF_SPEC_SIZE                        8


struct nsm_msg_qos_ad_spec
{
  /* Cindex. */
  cindex_t cindex;

  /* Slack */
  float slack;

  /* Composed total. */
  float compose_total;

  /* Derived total. */
  float derived_total;

  /* Composed sum. */
  float compose_sum;

  /* Derived sum. */
  float derived_sum;

  /* Minimum path latency. */
  float min_path_latency;

  /* Path bandwidth estimate. */
  float path_bw_est;
};

#define NSM_MSG_QOS_AD_SPEC_SIZE                        0

struct nsm_msg_qos_preempt
{
  u_int32_t protocol_id;
  u_int32_t count;
  u_int32_t *preempted_ids;
};

#define MAX_RESOURCES_PER_MSG                           5000

struct nsm_msg_qos
{
  /* Cindex. */
  cindex_t cindex;

  /* Resource ID. */
  u_int32_t resource_id;

  /* Protocol ID. */
  u_int32_t protocol_id;

  /* ID. */
  u_int32_t id;

  /* MPLS Owner. */
  struct mpls_owner owner;

  /* Class Type number for DSTE usage. */
  u_char ct_num;

  /* Setup priority. */
  u_int8_t setup_priority;

  /* Pre-emption specific priority. */
  u_int8_t hold_priority;

  /* Tspec */
  struct nsm_msg_qos_t_spec t_spec;

  /* Ifspec */
  struct nsm_msg_qos_if_spec if_spec;

  /* Adspec */
  struct nsm_msg_qos_ad_spec ad_spec;

  /* Status */
  u_int32_t status;
};

#ifdef HAVE_GMPLS
struct nsm_msg_gmpls_qos_if_spec
{
  /* Cindex. */
  cindex_t cindex;

  /* TE link and Data link gifindex */
  u_int32_t tel_gifindex;
  u_int32_t dl_gifindex;

  /* CA Router ID */
  struct pal_in4_addr rtr_id;
};

#define NSM_MSG_GMPLS_QOS_IF_SPEC_SIZE                        4

struct nsm_msg_gmpls_qos_attr
{
  /* Cindex. */
  cindex_t cindex;

  /* Switching Capability */
  u_int8_t sw_cap;

  /* Encoding Type */
  u_int8_t encoding;

  /* Desired Link Protection */
  u_int8_t protection;

  /* G-PID */
  u_int16_t gpid;
};

#define NSM_MSG_GMPLS_QOS_ATTR_SIZE                         2

struct nsm_msg_gmpls_qos_label_spec
{
  /* Cindex. */
  cindex_t cindex;

  /* From and to indicate the range that the suggested label is sought from */
  u_int32_t  sl_lbl_set_from;
  u_int32_t  sl_lbl_set_to;

  /* Suggesetd label returned by RIB to RSVP */
  struct gmpls_gen_label   sugg_label;
};

#define NSM_MSG_GMPLS_QOS_LABEL_SET_SIZE                         8
struct nsm_msg_gmpls_qos
{
  /* Cindex. */
  cindex_t cindex;

  /* Resource ID. */
  u_int32_t resource_id;

  /* Protocol ID. */
  u_int32_t protocol_id;

  /* ID. */
  u_int32_t id;

  /* MPLS Owner. */
  struct mpls_owner owner;

  /* Class Type number for DSTE usage. */
  u_char ct_num;

  /* LSP Direction */
  u_char direction;
#define GMPLS_LSP_FORWARD   0
#define GMPLS_LSP_REVERSE   1

  /* Setup priority. */
  u_int8_t setup_priority;

  /* Pre-emption specific priority. */
  u_int8_t hold_priority;

  /* Tspec */
  struct nsm_msg_qos_t_spec t_spec;

  /* Adspec */
  struct nsm_msg_qos_ad_spec ad_spec;

  /* GMPLS IF Spec */
  struct nsm_msg_gmpls_qos_if_spec gif_spec;

  /* GMPLS Attributes */
  struct nsm_msg_gmpls_qos_attr gmpls_attr;

  /* GMPLS Label Spec */
  struct nsm_msg_gmpls_qos_label_spec label_spec;

  /* Status */
  u_int32_t status;
};
#endif /* HAVE_GMPLS */

#define NSM_MSG_QOS_SIZE                               8

/* NSM QOS CTYPEs. */
#define NSM_QOS_CTYPE_T_SPEC                            0
#define NSM_QOS_CTYPE_T_SPEC_PEAK_RATE                  0
#define NSM_QOS_CTYPE_T_SPEC_QOS_COMMITED_BUCKET        1
#define NSM_QOS_CTYPE_T_SPEC_WEIGHT                     2
#define NSM_QOS_CTYPE_T_SPEC_MIN_POLICED_UNIT           3
#define NSM_QOS_CTYPE_T_SPEC_MAX_PACKET_SIZE            4
#define NSM_QOS_CTYPE_T_SPEC_EXCESS_BURST_SIZE          5

#define NSM_QOS_CTYPE_IF_SPEC                           1
#define NSM_QOS_CTYPE_IF_SPEC_IFINDEX                   0
#define NSM_QOS_CTYPE_IF_SPEC_ADDR                      1

#define NSM_QOS_CTYPE_AD_SPEC                           2
#define NSM_QOS_CTYPE_AD_SPEC_SLACK                     0
#define NSM_QOS_CTYPE_AD_SPEC_COMPOSE_TOTAL             1
#define NSM_QOS_CTYPE_AD_SPEC_DERIVED_TOTAL             2
#define NSM_QOS_CTYPE_AD_SPEC_COMPOSE_SUM               3
#define NSM_QOS_CTYPE_AD_SPEC_DERIVED_SUM               4
#define NSM_QOS_CTYPE_AD_SPEC_MIN_PATH_LATENCY          5
#define NSM_QOS_CTYPE_AD_SPEC_PATH_BW_EST               6

#define NSM_QOS_CTYPE_SETUP_PRIORITY                   3
#define NSM_QOS_CTYPE_HOLD_PRIORITY                    4

#ifdef HAVE_GMPLS
#define NSM_QOS_CTYPE_GMPLS_IF_SPEC                    5
#define NSM_QOS_CTYPE_GMPLS_IF_SPEC_TL_GIFINDEX        0
#define NSM_QOS_CTYPE_GMPLS_IF_SPEC_DL_GIFINDEX        1
#define NSM_QOS_CTYPE_GMPLS_IF_SPEC_CA_RTR_ID          2

#define NSM_QOS_CTYPE_GMPLS_ATTR                       6
#define NSM_QOS_CTYPE_GMPLS_ATTR_SW_CAP                0
#define NSM_QOS_CTYPE_GMPLS_ATTR_PROTECTION            1
#define NSM_QOS_CTYPE_GMPLS_ATTR_GPID                  2

#define NSM_QOS_CTYPE_GMPLS_LABEL_SPEC                 7
#define NSM_QOS_CTYPE_GMPLS_LABEL_SPEC_LABEL_SET       0
#define NSM_QOS_CTYPE_GMPLS_LABEL_SPEC_SUGG_LABEL      1
#endif /* HAVE_GMPLS */

#define NSM_QOS_CTYPE_STATUS                           8
/*
  NSM QOS release message

  NSM_MSG_QOS_CLIENT_RELEASE

  QOS protocol ID TLV
  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |                        Protocol ID                            |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

  QOS resource ID TLV
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |                        Resource ID                            |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

  Hold priority TLV
  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |   Hold prio   |
  +-+-+-+-+-+-+-+-+

  Interface index TLV
  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |                        ifindex                                |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

  Status TLV
  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |                         Status                                |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

*/

struct nsm_msg_qos_release
{
  /* Cindex. */
  cindex_t cindex;

  /* Protocol ID. */
  u_int32_t protocol_id;

  /* Resource ID. */
  u_int32_t resource_id;

  /* Ifindex. */
  u_int32_t ifindex;

  /* Status. */
  u_int32_t status;
};

#define NSM_MSG_QOS_RELEASE_SIZE                       8

/* NSM QOS release CTYPEs. */
#define NSM_QOS_RELEASE_CTYPE_IFINDEX                  0
#define NSM_QOS_RELEASE_CTYPE_STATUS                   1

#ifdef HAVE_GMPLS
struct nsm_msg_gmpls_qos_release
{
  /* Cindex. */
  cindex_t cindex;

  /* Protocol ID. */
  u_int32_t protocol_id;

  /* Resource ID. */
  u_int32_t resource_id;

  /* Data Link gifindex should be sufficient
     to release resources, TE gifindex is kept
     if release is sent from RSVP before it receives
     reservation */
  /* TE Link gIfindex. */
  u_int32_t tel_gifindex;

  /* Data Link gIfindex. */
  u_int32_t dl_gifindex;

  /* Status. */
  u_int32_t status;
};

#define NSM_MSG_GMPLS_QOS_RELEASE_SIZE                       8

/* NSM QOS release CTYPEs. */
#define NSM_GMPLS_QOS_RELEASE_CTYPE_TL_GIFINDEX              0
#define NSM_GMPLS_QOS_RELEASE_CTYPE_DL_GIFINDEX              1
#define NSM_GMPLS_QOS_RELEASE_CTYPE_STATUS                   2

#endif /* HAVE_GMPLS */

/*
  NSM QOS clean message

  NSM_MSG_QOS_CLIENT_CLEAN

  QOS protocol ID TLV
  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |                        Protocol ID                            |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

  Interface index TLV
  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |                        ifindex                                |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

*/


struct nsm_msg_qos_clean
{
  /* Cindex. */
  cindex_t cindex;

  /* Protocol ID. */
  u_int32_t protocol_id;

  /* Ifindex. */
  u_int32_t ifindex;
};

#define NSM_MSG_QOS_CLEAN_SIZE                         4

/* NSM QOS clean CTYPEs. */
#define NSM_QOS_CLEAN_CTYPE_IFINDEX                    0

#ifdef HAVE_GMPLS
struct nsm_msg_gmpls_qos_clean
{
  /* Cindex. */
  cindex_t cindex;

  /* Protocol ID. */
  u_int32_t protocol_id;

  /* TE Link gIfindex. */
  u_int32_t tel_gifindex;

  /* Data Link gIfindex. */
  u_int32_t dl_gifindex;
};

#define NSM_GMPLS_MSG_QOS_CLEAN_SIZE                         4

/* NSM QOS clean CTYPEs. */
#define NSM_GMPLS_QOS_CLEAN_CTYPE_TL_GIFINDEX                0
#define NSM_GMPLS_QOS_CLEAN_CTYPE_DL_GIFINDEX                1
#endif /*HAVE_GMPLS */

/* NSM Admin Group message

NSM_MSG_ADMIN_GROUP_UPDATE


0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|                       Admin Group ID                          |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|                       Admin Group Name                        |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|                       ...                                     |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
*/

#include "admin_grp.h"

struct nsm_msg_admin_group
{
  cindex_t cindex;
  struct admin_group array[ADMIN_GROUP_MAX];
};

#endif /* HAVE_TE */

#ifdef HAVE_MPLS_VC
/* NSM VC message


0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|                             VC ID                             |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

Address TLV

Control Word TLV

Group ID TLV

Interface Index TLV

VPLS ID TLV

Interface MTU TLV

VC Type TLV

*/

struct nsm_msg_mpls_vc
{
  cindex_t cindex;
  u_int32_t vc_id;
  afi_t afi;
  struct pal_in4_addr nbr_addr_ipv4;
#ifdef HAVE_IPV6
  struct pal_in6_addr nbr_addr_ipv6;
#endif /* HAVE_IPV6 */
  u_char c_word;
  u_int32_t grp_id;
  u_int32_t ifindex;
#ifdef HAVE_VPLS
  u_int32_t vpls_id;
#endif /* HAVE_VPLS */
  u_int16_t ifmtu;
  u_int16_t vc_type;
  u_int16_t vlan_id;
  u_int32_t pw_status;
#ifdef HAVE_VCCV
  u_int8_t cc_types;
  u_int8_t cv_types;
#endif /* HAVE_VCCV */
#ifdef HAVE_MS_PW
  u_int8_t ms_pw_role;
#endif /* HAVE_MS_PW */
};

#define NSM_MSG_MPLS_VC_SIZE            4

#define NSM_MPLS_VC_CTYPE_NEIGHBOR      0
#define NSM_MPLS_VC_CTYPE_CONTROL_WORD  1
#define NSM_MPLS_VC_CTYPE_GROUP_ID      2
#define NSM_MPLS_VC_CTYPE_IF_INDEX      3
#define NSM_MPLS_VC_CTYPE_VC_TYPE       4
#define NSM_MPLS_VC_CTYPE_VLAN_ID       5
#define NSM_MPLS_VC_CTYPE_IF_MTU        6
#define NSM_MPLS_VC_CTYPE_VPLS_ID       7
#define NSM_MPLS_VC_CTYPE_PW_STATUS     8
#define NSM_MPLS_VC_CTYPE_VCCV_CCTYPES  9
#define NSM_MPLS_VC_CTYPE_VCCV_CVTYPES  10
#define NSM_MPLS_VC_CTYPE_MS_PW_ROLE    11

struct nsm_msg_vc_fib_add
{
  u_int32_t vpn_id;
  u_int32_t vc_id;
  u_int32_t vc_style;
  u_int32_t in_label;
  u_int32_t out_label;
  u_int32_t ac_if_ix;
  u_int32_t nw_if_ix;
  struct addr_in addr;
  u_char c_word;
  u_int32_t lsr_id;
  u_int32_t index;
  u_int32_t remote_grp_id;
  u_int32_t remote_pw_status; /* remote pw_status */
#ifdef HAVE_VCCV
  u_int8_t remote_cc_types;
  u_int8_t remote_cv_types;
#endif /* HAVE_VCCV */
  bool_t rem_pw_status_cap;
};

struct nsm_msg_pw_status
{
  u_int32_t vpn_id;
  u_int32_t vc_id;
  /* send from nsm is local pw-status, send from ldp is remote pw-status */
  u_int32_t pw_status;

#ifdef HAVE_VCCV
  /* Incoming VC label. Currently used by OAMD client. Ignored by LDP client */
  u_int32_t in_vc_label;
#endif /* HAVE_VCCV */
};

struct nsm_msg_vc_tunnel_info
{
  u_int32_t lsr_id;
  u_int32_t index;
  u_char type;
#define VC_TUNNEL_INFO_ADD                1
#define VC_TUNNEL_INFO_DELETE             2
};

struct nsm_msg_vc_fib_delete
{
  u_int32_t vpn_id;
  u_int32_t vc_id;
  u_int32_t vc_style;
  struct addr_in addr;
};

#define NSM_MSG_VC_TUNNEL_INFO_SIZE        (10)
#define NSM_MSG_VC_FIB_ADD_SIZE            (47 + ADDR_IN_LEN)
#define NSM_MSG_VC_FIB_DELETE_SIZE         (14 + ADDR_IN_LEN)
#define NSM_MSG_VC_STATUS_SIZE             (12)

#ifdef HAVE_MS_PW

#define NSM_MSG_MPLS_MS_PW_MSG_SIZE        20
#define NSM_MSG_MPLS_MS_PW_CINDEX_MAX       6
#define NSM_MS_PW_NAME_LEN                 20
#define NSM_MS_PW_DESCR_LEN                80

/**@brief nsm mpls mspw message structure.
*/
struct nsm_msg_mpls_ms_pw_msg
{
  cindex_t cindex; /**< cindex for optional parameters */
  /* Used for addition */
#define NSM_MPLS_MS_PW_VC1_INFO  0
#define NSM_MPLS_MS_PW_VC2_INFO  1
  /* Used during deletion */
#define NSM_MPLS_MS_PW_VC1_ID    2
#define NSM_MPLS_MS_PW_VC2_ID    3
  /* Optional S-PE descr */
#define NSM_MPLS_MS_PW_SPE_DESCR 4

  u_int32_t ms_pw_action;
#define NSM_MPLS_MS_PW_ADD 1
#define NSM_MPLS_MS_PW_DEL 2
#define NSM_MPLS_MS_PW_ADD_SPE_DESCR 3
#define NSM_MPLS_MS_PW_DEL_SPE_DESCR 4

  char ms_pw_name[NSM_MS_PW_NAME_LEN]; /**< MSPW name */

  char ms_pw_spe_descr[NSM_MS_PW_DESCR_LEN]; /**< MSPW SPE descr name */

  struct nsm_msg_mpls_vc vc1_info; /**< info for vc1 create */

  struct nsm_msg_mpls_vc vc2_info; /**< info for vc2 create */

  u_int32_t vcid1; /**< VC1 ID for deletion */

  u_int32_t vcid2; /**< VC2 ID for deletion */
};
#endif /* HAVE_MS_PW */
#endif /* HAVE_MPLS_VC */


#ifdef HAVE_VPLS
/* NSM VPLS Add message

0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|                     VPLS ID                                   |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

Group ID TLV

Added Peer Address TLV

Deleted Peer Address TLV

Interface MTU TLV

VPLS Type TLV

Control Word TLV
*/


struct nsm_msg_vpls_add
{
  cindex_t cindex;
  u_int32_t vpls_id;
  u_int32_t grp_id;
  u_int16_t add_count;
  u_int16_t del_count;
  struct addr_in *peers_added;
  struct addr_in *peers_deleted;
  u_int16_t ifmtu;
  u_int16_t vpls_type;
  u_char c_word;
};


struct nsm_msg_vpls_mac_withdraw
{
  u_int32_t vpls_id;
  u_int16_t num;
  u_char *mac_addrs;
};

#define NSM_MSG_MIN_VPLS_MAC_WITHDRAW_SIZE   6

#define NSM_MSG_VPLS_SIZE                    4

#define NSM_VPLS_CTYPE_GROUP_ID              0
#define NSM_VPLS_CTYPE_PEERS_ADDED           1
#define NSM_VPLS_CTYPE_PEERS_DELETED         2
#define NSM_VPLS_CTYPE_IF_MTU                3
#define NSM_VPLS_CTYPE_VPLS_TYPE             4
#define NSM_VPLS_CTYPE_CONTROL_WORD          5
#endif /* HAVE_VPLS */

/* Defing MPLS OAM related Structures */
#ifdef HAVE_NSM_MPLS_OAM
/* OAM Return Codes */
#define MPLS_OAM_DEFAULT_RET_CODE             0
#define MPLS_OAM_MALFORMED_REQUEST            1
#define MPLS_OAM_ERRORED_TLV                  2
#define MPLS_OAM_RTR_IS_EGRESS_AT_DEPTH       3
#define MPLS_OAM_RTR_HAS_NO_MAPPING_AT_DEPTH  4
#define MPLS_OAM_DSMAP_MISMATCH               5
#define MPLS_OAM_UPSTREAM_IF_UNKNOWN          6
#define MPLS_OAM_RSVD_VALUE                   7
#define MPLS_OAM_LBL_SWITCH_AT_DEPTH          8
#define MPLS_OAM_LBL_SWITCH_IP_FWD_AT_DEPTH   9
#define MPLS_OAM_MAPPING_ERR_AT_DEPTH        10
#define MPLS_OAM_NO_MAPPING_AT_DEPTH         11
#define MPLS_OAM_PROTOCOL_ERR_AT_DEPTH       12
#define MPLS_OAM_PING_TIMEOUT_ERR            13

/* OAM CTYPES */
typedef enum  {
   NSM_CTYPE_MSG_MPLSONM_OPTION_ONM_TYPE= 0,
   NSM_CTYPE_MSG_MPLSONM_OPTION_LSP_TYPE,
   NSM_CTYPE_MSG_MPLSONM_OPTION_LSP_ID,
   NSM_CTYPE_MSG_MPLSONM_OPTION_FEC,
   NSM_CTYPE_MSG_MPLSONM_OPTION_DESTINATION,
   NSM_CTYPE_MSG_MPLSONM_OPTION_SOURCE,
   NSM_CTYPE_MSG_MPLSONM_OPTION_TUNNELNAME,
   NSM_CTYPE_MSG_MPLSONM_OPTION_EGRESS,
   NSM_CTYPE_MSG_MPLSONM_OPTION_EXP,
   NSM_CTYPE_MSG_MPLSONM_OPTION_PEER,
   NSM_CTYPE_MSG_MPLSONM_OPTION_PREFIX,
   NSM_CTYPE_MSG_MPLSONM_OPTION_TTL,
   NSM_CTYPE_MSG_MPLSONM_OPTION_TIMEOUT,
   NSM_CTYPE_MSG_MPLSONM_OPTION_REPEAT,
   NSM_CTYPE_MSG_MPLSONM_OPTION_VERBOSE,
   NSM_CTYPE_MSG_MPLSONM_OPTION_REPLYMODE,
   NSM_CTYPE_MSG_MPLSONM_OPTION_INTERVAL,
   NSM_CTYPE_MSG_MPLSONM_OPTION_FLAGS,
   NSM_CTYPE_MSG_MPLSONM_OPTION_NOPHP
}nsm_ctype_mpls_onm_option;
typedef enum  {
   MPLSONM_OPTION_PING = 0,
   MPLSONM_OPTION_TRACE,
   MPLSONM_OPTION_LDP,
   MPLSONM_OPTION_RSVP,
   MPLSONM_OPTION_VPLS,
   MPLSONM_OPTION_L2CIRCUIT,
   MPLSONM_OPTION_L3VPN,
   MPLSONM_OPTION_STATIC,
   MPLSONM_OPTION_DESTINATION,
   MPLSONM_OPTION_SOURCE,
   MPLSONM_OPTION_TUNNELNAME,
   MPLSONM_OPTION_EGRESS,
   MPLSONM_OPTION_EXP,
   MPLSONM_OPTION_PEER,
   MPLSONM_OPTION_PREFIX,
   MPLSONM_OPTION_TTL,
   MPLSONM_OPTION_TIMEOUT,
   MPLSONM_OPTION_REPEAT,
   MPLSONM_OPTION_VERBOSE,
   MPLSONM_OPTION_REPLYMODE,
   MPLSONM_OPTION_INTERVAL,
   MPLSONM_OPTION_FLAGS,
   MPLSONM_OPTION_NOPHP,
   MPLSONM_MAX_OPTIONS
}mpls_onm_option_type;

#define  NSM_MSG_MPLS_ONM_SIZE 8
#define  NSM_MSG_MPLS_ONM_ERR_SIZE 4

#define  OAM_DETAIL_SET          0
#define  MPLS_OAM_DEFAULT_TTL   30
#define  MPLS_OAM_DEFAULT_COUNT  5
struct nsm_msg_mpls_ping_req
{
  /* Echo Request Options */
  cindex_t cindex;

  union
  {
    struct rsvp_tunnel
    {
      struct pal_in4_addr addr;
      u_char *tunnel_name;
      int tunnel_len;
    }rsvp;
    struct fec_data
    {
      u_char *vrf_name;
      int vrf_name_len;
      u_int32_t prefixlen;
      struct pal_in4_addr ip_addr;
      struct pal_in4_addr vpn_prefix;
    }fec;
    struct vc_data
    {
      u_int32_t vc_id;
      struct pal_in4_addr vc_peer;
      bool_t is_vccv;
    }l2_data;
  }u;

  struct pal_in4_addr dst;
  struct pal_in4_addr src;
  u_int32_t interval;
  u_int32_t timeout;
  u_int32_t ttl;
  u_int32_t repeat;
  u_int8_t exp_bits;
  u_char flags;
  u_int8_t reply_mode;
  u_int8_t verbose;
  u_int8_t nophp;

  /* Echo Type - trace or ping */
  bool_t req_type;
  /* FEC type */
  u_char type;
  /* Socket id of releavant Utility instance */
  pal_sock_handle_t sock;
};

/* Send reply for successful ping */
struct nsm_msg_mpls_ping_reply
{
  cindex_t cindex;
  /* These values are used for detailed and standard output */
  u_int32_t sent_time_sec;
  u_int32_t sent_time_usec;
  u_int32_t recv_time_sec;
  u_int32_t recv_time_usec;
  struct pal_in4_addr reply;
  u_int32_t ping_count;
  u_int32_t recv_count;
  u_int32_t ret_code;
};

typedef enum {
  NSM_CTYPE_MSG_MPLSONM_PING_SENTTIME=0,
  NSM_CTYPE_MSG_MPLSONM_PING_RECVTIME,
  NSM_CTYPE_MSG_MPLSONM_PING_REPLYADDR,
  NSM_CTYPE_MSG_MPLSONM_PING_VPNID,
  NSM_CTYPE_MSG_MPLSONM_PING_COUNT,
  NSM_CTYPE_MSG_MPLSONM_PING_RECVCOUNT,
  NSM_CTYPE_MSG_MPLSONM_PING_RETCODE
} nsm_ctype_mpls_onm_ping_reply;

/* Send reply for a trace route */
struct nsm_msg_mpls_tracert_reply
{
  cindex_t cindex;
  struct pal_in4_addr reply;
  u_int32_t sent_time_sec;
  u_int32_t sent_time_usec;
  u_int32_t recv_time_sec;
  u_int32_t recv_time_usec;
  u_int32_t recv_count;
  u_int32_t ret_code;
  u_int16_t ttl;
  int ds_len;
  struct list *ds_label;
  bool_t last;
  u_int32_t out_label;
};
typedef enum {
  NSM_CTYPE_MSG_MPLSONM_TRACE_REPLYADDR=0,
  NSM_CTYPE_MSG_MPLSONM_TRACE_SENTTIME,
  NSM_CTYPE_MSG_MPLSONM_TRACE_RECVTIME,
  NSM_CTYPE_MSG_MPLSONM_TRACE_TTL,
  NSM_CTYPE_MSG_MPLSONM_TRACE_DSMAP,
  NSM_CTYPE_MSG_MPLSONM_TRACE_RETCODE,
  NSM_CTYPE_MSG_MPLSONM_TRACE_LAST_MSG,
  NSM_CTYPE_MSG_MPLSONM_TRACE_RECVCOUNT,
  NSM_CTYPE_MSG_MPLSONM_TRACE_OUTLABEL
} nsm_ctype_mpls_onm_trace_reply;

#define NSM_MPLSONM_ECHO_TIMEOUT        0
#define NSM_MPLSONM_NO_FTN              1
#define NSM_MPLSONM_TRACE_LOOP          2
#define NSM_MPLSONM_PACKET_SEND_ERROR   3
#define NSM_MPLSONM_ERR_EXPLICIT_NULL   4
#define NSM_MPLSONM_ERR_NO_CONFIG       5
#define NSM_MPLSONM_ERR_UNKNOWN         6

/* Signal Error conditions, incl timeouts etc */
struct nsm_msg_mpls_ping_error
{
  cindex_t cindex;
  u_int8_t msg_type;
  u_int8_t type;
  u_int32_t vpn_id;
  struct pal_in4_addr fec;
  struct pal_in4_addr vpn_peer;
  u_int32_t sent_time;

  u_int32_t ping_count;
  u_int32_t recv_count;
  u_int32_t ret_code;
  /* This is used to indicate internal errors and timeouts */
  u_int32_t err_code;
};

typedef enum {
  NSM_CTYPE_MSG_MPLSONM_ERROR_MSGTYPE=0,
  NSM_CTYPE_MSG_MPLSONM_ERROR_TYPE,
  NSM_CTYPE_MSG_MPLSONM_ERROR_VPNID,
  NSM_CTYPE_MSG_MPLSONM_ERROR_FEC,
  NSM_CTYPE_MSG_MPLSONM_ERROR_VPN_PEER,
  NSM_CTYPE_MSG_MPLSONM_ERROR_SENTTIME,
  NSM_CTYPE_MSG_MPLSONM_ERROR_RETCODE,
  NSM_CTYPE_MSG_MPLSONM_ERROR_PINGCOUNT,
  NSM_CTYPE_MSG_MPLSONM_ERROR_RECVCOUNT,
  NSM_CTYPE_MSG_MPLSONM_ERROR_ERRCODE
} nsm_ctype_mpls_onm_error;

#endif /* HAVE_NSM_MPLS_OAM */

/* Defing MPLS OAM ITUT related Structures */

#ifdef HAVE_MPLS_OAM_ITUT

#define MPLS_OAM_ITUT_ALERT_LABEL   14
#define MPLS_OAM_CV_PROCESS_MESSAGE  1
#define MPLS_OAM_FDI_PROCESS_MESSAGE 2
#define MPLS_OAM_BDI_PROCESS_MESSAGE 3
#define MPLS_OAM_FFD_PROCESS_MESSAGE 7

#define NSM_MPLS_OAM_FFD_DEFAULT_FREQ 3

typedef enum {
  NSM_MPLS_CV_PKT,
  NSM_MPLS_FDI_PKT,
  NSM_MPLS_BDI_PKT,
  NSM_MPLS_FFD_PKT
} nsm_mpls_oam_pkt_type;

typedef enum {
  NSM_MPLS_OAM_PKT_START,
  NSM_MPLS_OAM_PKT_STOP
} nsm_mpls_oam_pkt_process;

typedef enum {
  NSM_MPLS_OAM_SOURCE_LSR,
  NSM_MPLS_OAM_SINK_LSR,
  NSM_MPLS_OAM_TRANSIT_LSR
} nsm_mpls_oam_lsr_type;

typedef enum {
  NSM_MPLS_OAM_DISABLE=0,
  NSM_MPLS_OAM_ENABLE
} nsm_mpls_oam_status;

typedef enum {
  NSM_MPLS_OAM_FFD_FREQ_RESV=0,
  NSM_MPLS_OAM_FFD_FREQ_1,
  NSM_MPLS_OAM_FFD_FREQ_2,
  NSM_MPLS_OAM_FFD_FREQ_3,
  NSM_MPLS_OAM_FFD_FREQ_4,
  NSM_MPLS_OAM_FFD_FREQ_5,
  NSM_MPLS_OAM_FFD_FREQ_6
} nsm_mpls_oam_ffd_freq;


struct nsm_mpls_oam_fdi_level
{
  u_int16_t level_no; /* till which level need to send FDI packets */
  u_int32_t lable_id[3];
};

struct nsm_msg_mpls_oam_itut_req
{
  nsm_mpls_oam_pkt_type pkt_type;
  nsm_mpls_oam_pkt_process flag;
  u_int16_t frequency ; /* only for FFD PKT type*/
  u_int32_t lsp_id;
  u_int32_t fwd_lsp_id; /* for configuring BDI PKT, this ID maps to return LSPID */
  u_int32_t ttl;
  u_int32_t timeout;
  nsm_mpls_oam_lsr_type lsr_type;
  u_int16_t recv_pkt_cnt;
  struct pal_timeval pkt_rcv_time;
  struct thread *check_timer;
  struct pal_in4_addr lsr_addr;
  struct nsm_mpls_oam_fdi_level fdi_level;
};


struct nsm_mpls_oam_stats
{
  u_int32_t lsp_id; /* LSP ID to monitor oam trafficl*/
  u_int32_t oampkt_counter;    /* OAM packet counter */
};

struct mpls_oam_itut_flag
{
  u_int8_t flag;
};

struct mpls_oam_itut_flag itut_flag;

#endif /* HAVE_MPLS_OAM_ITUT */

#ifdef HAVE_RESTART
struct nsm_msg_stale_info
{
  struct prefix fec_prefix;
  u_int32_t in_label;
  u_int32_t iif_ix;
  u_int32_t out_label;
  u_int32_t oif_ix;
  u_int32_t ilm_ix;
};
#define NSM_MSG_STALE_INFO_SIZE          4

#ifdef HAVE_MPLS
struct nsm_gen_msg_stale_info
{
  struct prefix fec_prefix;
  struct gmpls_gen_label in_label;
  u_int32_t iif_ix;
  struct gmpls_gen_label out_label;
  u_int32_t oif_ix;
  u_int32_t ilm_ix;
};
#endif
#endif /* HAVE_RESTART */
/* NSM Router ID message

NSM_MSG_ROUTER_ID_UPDATE

Router ID TLV
0                   1                   2                   3
0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|                        Router ID                              |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

*/


struct nsm_msg_router_id
{
  cindex_t cindex;

  u_int32_t router_id;

  /* Configuration flags.  */
  u_char config;
/* This flag is set if a router-id has been set in the global mode */
#define NSM_ROUTER_ID_CONFIGURED                    (1 << 0)
/* This flag is set if an address is configured on the loopback interface */
#define NSM_LOOPBACK_CONFIGURED                     (1 << 1)

};

#define NSM_MSG_ROUTER_ID_SIZE          5

#ifdef HAVE_MPLS
/* NSM_MSG_MPLS_FTN_IPV4
   NSM_MSG_MPLS_FTN_IPV4

   0                   1                   2                   3
   0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |             Bits            |A|              Length           |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |                       FTN Message ID                          |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |                       FTN Message...
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-

   Flags:
   A flag for add.

   Length:
   Total length of this message including header.

   MPLS Message ID:
   Message ID of each FTN request

*/

/* Forward declaration. */
struct ftn_add_data;
struct ilm_add_data;
struct ftn_ret_data;
struct ilm_ret_data;
struct ilm_gen_ret_data;
struct mpls_notification;

#if defined HAVE_MPLS_OAM || defined HAVE_NSM_MPLS_OAM
#define VPN_RD_SIZE   8
#define VPN_RD_TYPE_AS            (0)
#define VPN_RD_TYPE_IP            (1)
struct vpn_rd_as
{
  u_int16_t rd_type;
  u_int16_t rd_asval;
  u_int32_t rd_asnum;
};

/* RD when IP Value is used */
struct vpn_rd_ip
{
  u_int16_t rd_type;
  u_int8_t rd_ipval[4];
  u_int16_t rd_ipnum;
};


struct vpn_rd
{
  union {
    u_int8_t rd_val[VPN_RD_SIZE];
    struct vpn_rd_as rd_as;
    struct vpn_rd_ip rd_ip;
  } u;
#define vpnrd_val   u.rd_val
#define vpnrd_type  u.rd_as.rd_type
#define vpnrd_ip    u.rd_ip.rd_ipval
#define vpnrd_ipnum u.rd_ip.rd_ipnum
#define vpnrd_as    u.rd_as.rd_asval
#define vpnrd_asnum u.rd_as.rd_asnum
};


struct nsm_msg_vpn_rd
{
  u_int8_t type;

  u_int32_t vr_id;
  vrf_id_t vrf_id;

  struct vpn_rd rd;
};
#endif /* HAVE_MPLS_OAM || HAVE_NSM_MPLS_OAM */

#define NSM_FTN_CTYPE_DESCRIPTION    0
#define NSM_FTN_CTYPE_PRIMARY_FEC_PREFIX 1

#ifdef HAVE_GMPLS
/* GELS : Added to sending Tech specific data for the FTN/ILM messages */
#define NSM_FTN_CTYPE_TGEN_DATA 2
#define NSM_ILM_CTYPE_TGEN_DATA 3
#endif /* HAVE_GMPLS */

/* NSM_MSG_IGP_SHORTCUT_LSP

   0                   1                   2                   3
   0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |            Flag               |          Metric               |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |                           LSP Tunnel-ID                       |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |                           FEC Prefix
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

*/

#define NSM_MSG_IGP_SHORTCUT_DELETE    0
#define NSM_MSG_IGP_SHORTCUT_ADD       1
#define NSM_MSG_IGP_SHORTCUT_UPDATE    2

/* NSM send igp-shortcut lsp information to OSPF */
struct nsm_msg_igp_shortcut_lsp
{
  u_char action;

  u_int16_t metric;
  u_int32_t tunnel_id;

  /* tunnel end point */
  struct pal_in4_addr t_endp;
};

/* OSPF send igp-shortcut route to NSM */
struct nsm_msg_igp_shortcut_route
{
  u_char action;

  u_int32_t tunnel_id;

  /* Destination FEC */
  struct prefix fec;

  /* Tunnel end point ip address */
  struct pal_in4_addr t_endp;
};
#endif /* HAVE_MPLS */

#if 0 /* TBD old code */
/*
   NSM_MSG_GMPLS_IF

   0                   1                   2                   3
   0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |                          Ifindex                              |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

   Link Identifier TLV
   0                   1                   2                   3
   0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |                    Link local identifier                      |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |                    Link remote identifier                     |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

   Link Protection type TLV
   0                   1                   2                   3
   0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |                   Link protection type                        |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

   Shared Risk Link Group TLV
   0                   1                   2                   3
   0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |                        count                                  |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |                       Group id                                |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |                         ...                                   |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |                       Group id                                |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

   Interface Switching Capability Type TLV
   0                   1                   2                   3
   0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |                    Capability Type Flag                       |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

   LSP Encoding TLV
   0                   1                   2                   3
   0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |   Encoding    |
   +-+-+-+-+-+-+-+-+

   Minimum LSP Bandwidth for Switching Capability specific Info
   0                   1                   2                   3
   0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |                   Minimum LSP Bandwidth                       |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

   SONET/SDH Indication for TDM Switching Capability specific information
   0                   1                   2                   3
   0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |   Indication  |
   +-+-+-+-+-+-+-+-+
*/

struct nsm_msg_link_ids
{
  u_int32_t local;
  u_int32_t remote;
};

struct nsm_msg_gmpls_if
{
  cindex_t cindex;

  u_int32_t ifindex;

  /* Link Local/Remote Identifiers. */
  struct nsm_msg_link_ids link_ids;

  /* Link Protection Type. */
  u_int8_t protection;

  /* Interface Switching Capability Type. */
  u_int32_t capability;

  /* LSP Encoding. */
  u_int8_t encoding;

  /* Minimum LSP bandwidth for PSC-1, PSC-2, PSC-3, PSC-4 or TDM. */
  float32_t min_lsp_bw;

  /* Indication whether the interface supports Standard or Arbitrary
     SONET/SDH for TDM swiching capability. */
  u_int8_t indication;

  /* Shared Risk Link Group. */
  vector srlg;
};
#define NSM_MSG_GMPLS_IF_SIZE         4

#define NSM_GMPLS_LINK_IDENTIFIER        0
#define NSM_GMPLS_PROTECTION_TYPE        1
#define NSM_GMPLS_CAPABILITY_TYPE        2
#define NSM_GMPLS_ENCODING_TYPE          3
#define NSM_GMPLS_MIN_LSP_BW             4
#define NSM_GMPLS_SDH_INDICATION         5
#define NSM_GMPLS_SHARED_RISK_LINK_GROUP 6
#endif /* TBD */

#ifdef HAVE_DIFFSERV
struct nsm_msg_supported_dscp_update
{
  u_char dscp;
  u_char support;
};

struct nsm_msg_dscp_exp_map_update
{
  u_char exp;
  u_char dscp;
};
#endif /* HAVE_DIFFSERV */

/* NSM message send queue.  */
struct nsm_message_queue
{
  struct nsm_message_queue *next;
  struct nsm_message_queue *prev;

  u_char *buf;
  u_int16_t length;
  u_int16_t written;
};

/* NSM_MSG_ROUTE_CLEAN

  AFI/SAFI TLV

   0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |             AFI               |      SAFI     |     Reserved  |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

  Route type mask TLV
   0                   1                   2                   3
   0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |                       Route Type Mask                         |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
*/

struct nsm_msg_route_clean
{
  /* Ctype index. */
  cindex_t cindex;

  afi_t afi;
  safi_t safi;

  u_int32_t route_type_mask;
};

/* NSM route clean msg size. */
#define NSM_MSG_ROUTE_CLEAN_SIZE                             8

/* NSM route clean CTYPEs. */

/* NSM_MSG_CRX_STATE

  Protocol ID TLV
   0                   1                   2                   3
   0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |                           Protocol ID                          |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

  Disconnection time TLV
   0                   1                   2                   3
   0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |                           Disconnect time                     |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
*/

struct nsm_msg_protocol_restart
{
  /* Protocol ID. */
  u_int32_t protocol_id;

  /* Disconnect time. */
  u_int32_t disconnect_time;
};

/* NSM protocol restart message size. */
#define NSM_MSG_PROTOCOL_RESTART_SIZE                  8

#ifdef HAVE_MCAST_IPV4
/*
   NSM_MSG_IPV4_VIF_ADD

   0                   1                   2                   3
   0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |                           ifindex                             |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |                     IPv4 VIF local address                    |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |                     IPv4 VIF remote address                   |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |            Flags              |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

*/
struct nsm_msg_ipv4_vif_add
{
  /* Ifindex. */
  s_int32_t ifindex;

  /* VIF local address. */
  struct pal_in4_addr local_addr;

  /* VIF remote address. */
  struct pal_in4_addr remote_addr;

  /* VIF flags */
  u_int16_t flags;
#define NSM_MSG_IPV4_VIF_FLAG_REGISTER      0x0001
#define NSM_MSG_IPV4_VIF_FLAG_TUNNEL        0x0002
};

#define NSM_MSG_IPV4_VIF_ADD_SIZE           14

/*
   NSM_MSG_IPV4_VIF_DEL

   0                   1                   2                   3
   0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |       vif_index               |         flags                 |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+


*/
struct nsm_msg_ipv4_vif_del
{
  /* VIF index. */
  u_int16_t vif_index;

  /* Flags */
  u_int16_t flags;
  /* Same flags as in vif add */
};

#define NSM_MSG_IPV4_VIF_DEL_SIZE           4

/*
   NSM_MSG_IPV4_MRT_ADD

   0                   1                   2                   3
   0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |                     MRT Source Address                        |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |                     MRT Group Address                         |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |                     MRT RPF Address                           |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |           Flags               |         Stat time             |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |           Pkt Count           |         Iif                   |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |           Num olist           |        olist vif 1     ...
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

*/
struct nsm_msg_ipv4_mrt_add
{
  /* MRT source address. */
  struct pal_in4_addr src;

  /* MRT group address. */
  struct pal_in4_addr group;

  /* RPF neighbor address. */
  struct pal_in4_addr rpf_addr;

  /* Flags */
  u_int16_t flags;
#define NSM_MSG_IPV4_MRT_STAT_IMMEDIATE           0x0001
#define NSM_MSG_IPV4_MRT_STAT_TIMED               0x0002

  /* Statistics timed event interval */
  u_int16_t stat_time;

  /* Initial pkt count for the entry */
  u_int16_t pkt_count;

  /* iif of MRT */
  u_int16_t iif;

  /* Number of vifs in olist */
  u_int16_t num_olist;

  /* Array of num_olist words (u_int16_t) of vif indices in olist */
  u_int16_t *olist_vifs;
};

#define NSM_MSG_IPV4_MRT_ADD_SIZE           22

/*
   NSM_MSG_IPV4_MRT_DEL

   0                   1                   2                   3
   0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |                     MRT Source Address                        |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |                     MRT Group Address                         |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

*/
struct nsm_msg_ipv4_mrt_del
{
  /* MRT source address. */
  struct pal_in4_addr src;

  /* MRT group address. */
  struct pal_in4_addr group;
};

#define NSM_MSG_IPV4_MRT_DEL_SIZE           8

/*
   NSM_MSG_IPV4_MRT_STAT_FLAGS_UPDATE

   0                   1                   2                   3
   0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |                     MRT Source Address                        |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |                     MRT Group Address                         |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |           Flags               |         Stat time             |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

*/
struct nsm_msg_ipv4_mrt_stat_flags_update
{
  /* MRT source address. */
  struct pal_in4_addr src;

  /* MRT group address. */
  struct pal_in4_addr group;

  /* Flags */
  u_int16_t flags;
  /* Same flags as MRT_ADD */

  /* Statistics timed event interval */
  u_int16_t stat_time;
};

#define NSM_MSG_IPV4_MRT_STAT_FLAGS_UPDATE_SIZE           12

/* NSM_MSG_IPV4_MRT_ST_REFRESH_FLAG_UPDATE_SIZE
   0                   1                   2                   3
   0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |                     MRT Group Address                         |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |                     Interface Index                           |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
*/

struct nsm_msg_ipv4_mrt_state_refresh_flag_update
{
  /* MRT group address. */
  struct pal_in4_addr grp;

  /* Interface Index */
  u_int32_t ifindex;
};

#define NSM_MSG_IPV4_MRT_ST_REFRESH_FLAG_UPDATE_SIZE      8

/* NSM_MSG_IPV4_MRT_NOCACHE

   0                   1                   2                   3
   0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |                     MRT Source Address                        |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |                     MRT Group Address                         |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |          Vif index            |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

*/
struct nsm_msg_ipv4_mrt_nocache
{
  /* MRT source address. */
  struct pal_in4_addr src;

  /* MRT group address. */
  struct pal_in4_addr group;

  /* vif index */
  u_int16_t vif_index;
};

#define NSM_MSG_IPV4_MRT_NOCACHE_SIZE           10

/*
   NSM_MSG_IPV4_MRT_WRONGVIF

   0                   1                   2                   3
   0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |                     MRT Source Address                        |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |                     MRT Group Address                         |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |          Vif index            |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

*/
struct nsm_msg_ipv4_mrt_wrongvif
{
  /* MRT source address. */
  struct pal_in4_addr src;

  /* MRT group address. */
  struct pal_in4_addr group;

  /* vif index */
  u_int16_t vif_index;
};

#define NSM_MSG_IPV4_MRT_WRONGVIF_SIZE           10

/*
   NSM_MSG_IPV4_MRT_WHOLEPKT_REQUEST

   0                   1                   2                   3
   0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |                     MRT Source Address                        |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |                     MRT Group Address                         |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |          Vif index            |   Data TTL    |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

*/
struct nsm_msg_ipv4_mrt_wholepkt_req
{
  /* MRT source address. */
  struct pal_in4_addr src;

  /* MRT group address. */
  struct pal_in4_addr group;

  /* vif index */
  u_int16_t vif_index;

  /* Data TTL */
  u_int8_t data_ttl;
};

#define NSM_MSG_IPV4_MRT_WHOLEPKT_REQ_SIZE           11

/*
   NSM_MSG_IPV4_MRT_WHOLEPKT_REPLY

   0                   1                   2                   3
   0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |                     MRT Source Address                        |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |                     MRT Group Address                         |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |                     RP Address                                |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |                     Src Address                               |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |                     Identifier                                |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |             Reply             |        Chksum type            |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

*/
struct nsm_msg_ipv4_mrt_wholepkt_reply
{
  /* MRT source address. */
  struct pal_in4_addr src;

  /* MRT group address. */
  struct pal_in4_addr group;

  /* RP address. */
  struct pal_in4_addr rp_addr;

  /* Register pkt source address. */
  struct pal_in4_addr src_addr;

  /* Wholepkt Request match identifier */
  u_int32_t id;

  /* Reply */
  u_int16_t reply;
#define NSM_MSG_IPV4_WHOLEPKT_REPLY_ACK   1
#define NSM_MSG_IPV4_WHOLEPKT_REPLY_NACK  0

  /* Register pkt chksum type */
  u_int16_t chksum_type;
#define NSM_MSG_IPV4_REG_PKT_CHKSUM_REG     1
#define NSM_MSG_IPV4_REG_PKT_CHKSUM_CISCO   2
};

#define NSM_MSG_IPV4_MRT_WHOLEPKT_REPLY_SIZE          24

/*
   NSM_MSG_IPV4_MRT_STAT_UPDATE

   0                   1                   2                   3
   0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |                     Num of Stat Blocks                        |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |                     Stat Block 1                              |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |                     Stat Block 2...                           |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+


   NSM_MSG_IPV4_MRT_STAT_BLOCK

   0                   1                   2                   3
   0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |                     MRT Source Address                        |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |                     MRT Group Address                         |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |                     Pkts forwarded                            |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |                     Bytes forwarded                           |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |            Flags              |            Reserved           |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+


*/
struct nsm_msg_ipv4_mrt_stat_block
{
  /* MRT source address. */
  struct pal_in4_addr src;

  /* MRT group address. */
  struct pal_in4_addr group;

  /* Packets forwarded. */
  u_int32_t pkts_fwd;

  /* Bytes forwarded. */
  u_int32_t bytes_fwd;

  /* Flags */
  u_int16_t flags;
  /* These are the same flags as used in mrt_add msg */

  /* Reserved */
  u_int16_t resv;
};

#define NSM_MSG_IPV4_MRT_STAT_BLOCK_SIZE      20


struct nsm_msg_ipv4_mrt_stat_update
{
  /* Number of stat blocks */
  u_int32_t num_blocks;

  /* Array of stat blocks */
  struct nsm_msg_ipv4_mrt_stat_block *blocks;
};

#define NSM_MSG_IPV4_MRT_STAT_UPDATE_SIZE      4

/*
   VIF_KEY

   0                   1                   2                   3
   0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |                           ifindex                             |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |                     Local Address                             |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |                     Remote Address                            |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |       vif_index               |         Flags                 |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

   MRT_KEY

   0                   1                   2                   3
   0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |                     MRT Source Address                        |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |                     MRT Group Address                         |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |          Flags                |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

  NSM_MSG_IPV4_MRIB_NOTIFICATION

   0                   1                   2                   3
   0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |             Type              |            Status             |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |                     Key....                                   |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
*/
struct nsm_msg_ipv4_mrib_notification_key_vif
{
  u_int32_t ifindex;

  struct pal_in4_addr local_addr;

  struct pal_in4_addr remote_addr;

  u_int16_t vif_index;

  /* VIF flags */
  u_int16_t flags;
  /* Same flags as vif_add */
};

#define NSM_IPV4_MRIB_NOTIF_KEY_VIF_SIZE  16

struct nsm_msg_ipv4_mrib_notification_key_mrt
{
  struct pal_in4_addr src;

  struct pal_in4_addr group;

  /* MRT flags */
  u_int16_t flags;
  /* Same flags as mrt_add */
};

#define NSM_IPV4_MRIB_NOTIF_KEY_MRT_SIZE  10

union nsm_msg_ipv4_mrib_notification_key
{
  struct nsm_msg_ipv4_mrib_notification_key_vif vif_key;

  struct nsm_msg_ipv4_mrib_notification_key_mrt mrt_key;
};


struct nsm_msg_ipv4_mrib_notification
{
  u_int16_t type;
#define NSM_MSG_IPV4_MRIB_NOTIFICATION_VIF_ADD            0
#define NSM_MSG_IPV4_MRIB_NOTIFICATION_VIF_DEL            1
#define NSM_MSG_IPV4_MRIB_NOTIFICATION_MRT_ADD            2
#define NSM_MSG_IPV4_MRIB_NOTIFICATION_MRT_DEL            3
#define NSM_MSG_IPV4_MRIB_NOTIFICATION_MCAST              4
#define NSM_MSG_IPV4_MRIB_NOTIFICATION_NO_MCAST           5
#define NSM_MSG_IPV4_MRIB_NOTIFICATION_CLR_MRT            6

  u_int16_t status;
#define MRIB_VIF_EXISTS                                   0
#define MRIB_VIF_NOT_EXIST                                1
#define MRIB_VIF_NO_INDEX                                 2
#define MRIB_VIF_NOT_BELONG                               3
#define MRIB_VIF_FWD_ADD_ERR                              4
#define MRIB_VIF_FWD_DEL_ERR                              5
#define MRIB_VIF_ADD_SUCCESS                              6
#define MRIB_VIF_DEL_SUCCESS                              7

#define MRIB_MRT_LIMIT_EXCEEDED                           8
#define MRIB_MRT_EXISTS                                   9
#define MRIB_MRT_NOT_EXIST                                10
#define MRIB_MRT_NOT_BELONG                               11
#define MRIB_MRT_FWD_ADD_ERR                              12
#define MRIB_MRT_FWD_DEL_ERR                              13
#define MRIB_MRT_ADD_SUCCESS                              14
#define MRIB_MRT_DEL_SUCCESS                              15

#define MRIB_INTERNAL_ERROR                               16

#define MRIB_NO_MCAST                                     17

#define MRIB_NA                                           18

  union nsm_msg_ipv4_mrib_notification_key key;
};

/*
   NSM_MSG_IGMP

   0                   1                   2                   3
   0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |                           ifindex                             |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |  Filter-Mode (INCL | EXCL)    |         Num of Sources        |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |                   Multicast Group Address                     |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  .                    IPv4 source address                        .
  .-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-.
  .                    IPv4 source address                        .
  .-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-.
  .                    IPv4 source address                        .
  .-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-.
  .                    IPv4 source address                        .
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
*/

struct nsm_msg_igmp
{
  /* Ifindex */
  u_int32_t ifindex;

  /* Filter Mode (INCLUDE | EXCLUDE) */
  u_int16_t filt_mode;
#define NSM_MSG_IGMP_FILT_MODE_INCL             0
#define NSM_MSG_IGMP_FILT_MODE_EXCL             1

  /* Number of Sources */
  u_int16_t num_srcs;

  /* Group address */
  struct pal_in4_addr grp_addr;

  /* Source addresses List */
  struct pal_in4_addr src_addr_list [1];
};

#define NSM_MSG_IGMP_SIZE                       12

/*
   NSM_MSG_MTRACE_KEY

   0                   1                   2                   3
   0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |                  Mtrace Query IP source address               |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |                       Query Id                                |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
*/
struct nsm_msg_mtrace_key
{
  struct pal_in4_addr ip_src;
  u_int32_t query_id;
};

#define NSM_MSG_MTRACE_KEY_SIZE (sizeof (struct nsm_msg_mtrace_key))

/*
   NSM_MSG_MTRACE_QUERY

   0                   1                   2                   3
   0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |                      Mtrace Key                               |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |                  Mtrace Query source address                  |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |                       oif_index                               |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |    Fwd code   |
  +-+-+-+-+-+-+-+-+
*/
struct nsm_msg_mtrace_query
{
  struct nsm_msg_mtrace_key key;
  struct pal_in4_addr q_src;
  u_int32_t oif_index;
  u_int8_t fwd_code;
};

#define NSM_MSG_MTRACE_QUERY_SIZE   17

/*
   NSM_MSG_MTRACE_REQUEST

   0                   1                   2                   3
   0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |                      Mtrace Key                               |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |                     Request num                               |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |                  Mtrace Query source address                  |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |                  Mtrace Query Group address                   |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |                       oif_index                               |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |                       iif_index                               |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |                  Previous Hop address                         |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |    Rtg Proto  |0|S| Src Mask  |   Fwd Code    |   Flags       |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
*/
struct nsm_msg_mtrace_request
{
  struct nsm_msg_mtrace_key key;
  u_int32_t req_num;
  struct pal_in4_addr q_src;
  struct pal_in4_addr q_grp;
  u_int32_t oif_index;
  u_int32_t iif_index;
  struct pal_in4_addr phop_addr;
  u_int8_t rtg_proto;
  u_int8_t src_mask;
  u_int8_t fwd_code;
  u_int8_t flags;
#define MTRACE_REQUEST_MSG_FLAG_RP   (1 << 0)
};

#define NSM_MSG_MTRACE_REQUEST_SIZE   36
#endif /* HAVE_MCAST_IPV4 */

#ifdef HAVE_MCAST_IPV6
/*
   NSM_MSG_IPV6_MIF_ADD

   0                   1                   2                   3
   0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |                           ifindex                             |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |            Flags              |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

*/
struct nsm_msg_ipv6_mif_add
{
  /* Ifindex. */
  s_int32_t ifindex;

  /* MIF flags */
  u_int16_t flags;
#define NSM_MSG_IPV6_MIF_FLAG_REGISTER      0x0001
};

#define NSM_MSG_IPV6_MIF_ADD_SIZE           6

/*
   NSM_MSG_IPV6_MIF_DEL

   0                   1                   2                   3
   0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |       mif_index               |         flags                 |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

*/
struct nsm_msg_ipv6_mif_del
{
  /* MIF index. */
  u_int16_t mif_index;

  /* Flags */
  u_int16_t flags;
  /* Same flags as in vif add */
};

#define NSM_MSG_IPV6_MIF_DEL_SIZE           4

/*
   NSM_MSG_IPV6_MRT_ADD

   0                   1                   2                   3
   0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |                     MRT Source Address                        |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |                     MRT Source Address                        |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |                     MRT Source Address                        |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |                     MRT Source Address                        |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |                     MRT Group Address                         |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |                     MRT Group Address                         |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |                     MRT Group Address                         |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |                     MRT Group Address                         |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |                     MRT RPF Address                           |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |                     MRT RPF Address                           |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |                     MRT RPF Address                           |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |                     MRT RPF Address                           |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |           Flags               |         Stat time             |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |           Pkt Count           |         Iif                   |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |           Num olist           |        olist vif 1     ...
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

*/
struct nsm_msg_ipv6_mrt_add
{
  /* MRT source address. */
  struct pal_in6_addr src;

  /* MRT group address. */
  struct pal_in6_addr group;

  /* RPF address. */
  struct pal_in6_addr rpf_addr;

  /* Flags */
  u_int16_t flags;
#define NSM_MSG_IPV6_MRT_STAT_IMMEDIATE           0x0002
#define NSM_MSG_IPV6_MRT_STAT_TIMED               0x0004

  /* Statistics timed event interval */
  u_int16_t stat_time;

  /* Initial pkt count for the entry */
  u_int16_t pkt_count;

  /* iif of MRT */
  u_int16_t iif;

  /* Number of mifs in olist */
  u_int16_t num_olist;

  /* Array of num_olist words (u_int16_t) of mif indices in olist */
  u_int16_t *olist_mifs;
};

#define NSM_MSG_IPV6_MRT_ADD_SIZE           58

/*
   NSM_MSG_IPV6_MRT_DEL

   0                   1                   2                   3
   0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |                     MRT Source Address                        |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |                     MRT Source Address                        |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |                     MRT Source Address                        |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |                     MRT Source Address                        |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |                     MRT Group Address                         |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |                     MRT Group Address                         |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |                     MRT Group Address                         |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |                     MRT Group Address                         |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

*/
struct nsm_msg_ipv6_mrt_del
{
  /* MRT source address. */
  struct pal_in6_addr src;

  /* MRT group address. */
  struct pal_in6_addr group;
};

#define NSM_MSG_IPV6_MRT_DEL_SIZE           32

/*
   NSM_MSG_IPV6_MRT_STAT_FLAGS_UPDATE

   0                   1                   2                   3
   0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |                     MRT Source Address                        |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |                     MRT Source Address                        |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |                     MRT Source Address                        |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |                     MRT Source Address                        |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |                     MRT Group Address                         |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |                     MRT Group Address                         |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |                     MRT Group Address                         |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |                     MRT Group Address                         |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |           Flags               |         Stat time             |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

*/
struct nsm_msg_ipv6_mrt_stat_flags_update
{
  /* MRT source address. */
  struct pal_in6_addr src;

  /* MRT group address. */
  struct pal_in6_addr group;

  /* Flags */
  u_int16_t flags;
  /* Same flags as MRT_ADD */

  /* Statistics timed event interval */
  u_int16_t stat_time;
};

#define NSM_MSG_IPV6_MRT_STAT_FLAGS_UPDATE_SIZE           36

/* NSM_MSG_IPV6_MRT_ST_REFRESH_FLAG_UPDATE_SIZE
   0                   1                   2                   3
   0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |                     MRT Group Address                         |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |                     Interface Index                           |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
*/

struct nsm_msg_ipv6_mrt_state_refresh_flag_update
{
  /* MRT group address. */
  struct pal_in6_addr grp;

  /* Interface Index */
  u_int32_t ifindex;
};

#define NSM_MSG_IPV6_MRT_ST_REFRESH_FLAG_UPDATE_SIZE      8

/*
   NSM_MSG_IPV6_MRT_NOCACHE

   0                   1                   2                   3
   0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |                     MRT Source Address                        |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |                     MRT Source Address                        |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |                     MRT Source Address                        |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |                     MRT Source Address                        |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |                     MRT Group Address                         |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |                     MRT Group Address                         |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |                     MRT Group Address                         |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |                     MRT Group Address                         |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |          Mif index            |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

*/
struct nsm_msg_ipv6_mrt_nocache
{
  /* MRT source address. */
  struct pal_in6_addr src;

  /* MRT group address. */
  struct pal_in6_addr group;

  /* mif index */
  u_int16_t mif_index;
};

#define NSM_MSG_IPV6_MRT_NOCACHE_SIZE           34

/*
   NSM_MSG_IPV6_MRT_WRONGMIF

   0                   1                   2                   3
   0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |                     MRT Source Address                        |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |                     MRT Source Address                        |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |                     MRT Source Address                        |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |                     MRT Source Address                        |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |                     MRT Group Address                         |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |                     MRT Group Address                         |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |                     MRT Group Address                         |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |                     MRT Group Address                         |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |          Mif index            |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

*/
struct nsm_msg_ipv6_mrt_wrongmif
{
  /* MRT source address. */
  struct pal_in6_addr src;

  /* MRT group address. */
  struct pal_in6_addr group;

  /* mif index */
  u_int16_t mif_index;
};

#define NSM_MSG_IPV6_MRT_WRONGMIF_SIZE           34

/*
   NSM_MSG_IPV6_MRT_WHOLEPKT_REQUEST

   0                   1                   2                   3
   0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |                     Identifier                                |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |                     MRT Source Address                        |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |                     MRT Source Address                        |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |                     MRT Source Address                        |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |                     MRT Source Address                        |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |                     MRT Group Address                         |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |                     MRT Group Address                         |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |                     MRT Group Address                         |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |                     MRT Group Address                         |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |          Mif index            |   Data HLIM   |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

*/
struct nsm_msg_ipv6_mrt_wholepkt_req
{
  /* MRT source address. */
  struct pal_in6_addr src;

  /* MRT group address. */
  struct pal_in6_addr group;

  /* mif index */
  u_int16_t mif_index;

  /* Data Hop Limit */
  u_int8_t data_hlim;
};

#define NSM_MSG_IPV6_MRT_WHOLEPKT_REQ_SIZE           35

/*
   NSM_MSG_IPV6_MRT_WHOLEPKT_REPLY

   0                   1                   2                   3
   0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |                     MRT Source Address                        |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |                     MRT Source Address                        |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |                     MRT Source Address                        |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |                     MRT Source Address                        |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |                     MRT Group Address                         |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |                     MRT Group Address                         |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |                     MRT Group Address                         |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |                     MRT Group Address                         |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |                     RP Address                                |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |                     RP Address                                |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |                     RP Address                                |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |                     RP Address                                |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |                     Src Address                               |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |                     Src Address                               |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |                     Src Address                               |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |                     Src Address                               |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |                     Identifier                                |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |              Reply            |         Chksum type           |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

*/
struct nsm_msg_ipv6_mrt_wholepkt_reply
{
  /* MRT source address. */
  struct pal_in6_addr src;

  /* MRT group address. */
  struct pal_in6_addr group;

  /* RP address. */
  struct pal_in6_addr rp_addr;

  /* Reg pkt src address. */
  struct pal_in6_addr src_addr;

  /* Wholepkt Request match identifier */
  u_int32_t id;

  /* Reply */
  u_int16_t reply;
#define NSM_MSG_IPV6_WHOLEPKT_REPLY_ACK   1
#define NSM_MSG_IPV6_WHOLEPKT_REPLY_NACK  0

  /* Reg pkt chksum type */
  u_int16_t chksum_type;
#define NSM_MSG_IPV6_REG_PKT_CHKSUM_REG    1
#define NSM_MSG_IPV6_REG_PKT_CHKSUM_CISCO  2
};

#define NSM_MSG_IPV6_MRT_WHOLEPKT_REPLY_SIZE          72

/*
   NSM_MSG_IPV6_MRT_STAT_UPDATE

   0                   1                   2                   3
   0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |                     Num of Stat Blocks                        |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |                     Stat Block 1                              |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |                     Stat Block 2...                           |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+


   NSM_MSG_IPV6_MRT_STAT_BLOCK

   0                   1                   2                   3
   0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |                     MRT Source Address                        |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |                     MRT Source Address                        |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |                     MRT Source Address                        |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |                     MRT Source Address                        |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |                     MRT Group Address                         |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |                     MRT Group Address                         |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |                     MRT Group Address                         |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |                     MRT Group Address                         |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |                     Pkts forwarded                            |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |                     Bytes forwarded                           |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |            Flags              |            Reserved           |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+


*/
struct nsm_msg_ipv6_mrt_stat_block
{
  /* MRT source address. */
  struct pal_in6_addr src;

  /* MRT group address. */
  struct pal_in6_addr group;

  /* Packets forwarded. */
  u_int32_t pkts_fwd;

  /* Bytes forwarded. */
  u_int32_t bytes_fwd;

  /* Flags */
  u_int16_t flags;
  /* These are the same flags as defined in MRT_ADD */

  /* Reserved */
  u_int16_t resv;
};

#define NSM_MSG_IPV6_MRT_STAT_BLOCK_SIZE      44


struct nsm_msg_ipv6_mrt_stat_update
{
  /* Number of stat blocks */
  u_int32_t num_blocks;

  /* Array of stat blocks */
  struct nsm_msg_ipv6_mrt_stat_block *blocks;
};

#define NSM_MSG_IPV6_MRT_STAT_UPDATE_SIZE      4

/*
   MIF_KEY

   0                   1                   2                   3
   0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |                           ifindex                             |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |          mif_index            |         Flags                 |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

   MRT_KEY

   0                   1                   2                   3
   0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |                     MRT Source Address                        |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |                     MRT Source Address                        |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |                     MRT Source Address                        |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |                     MRT Source Address                        |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |                     MRT Group Address                         |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |                     MRT Group Address                         |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |                     MRT Group Address                         |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |                     MRT Group Address                         |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |          Flags            |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+

  NSM_MSG_IPV6_MRIB_NOTIFICATION

   0                   1                   2                   3
   0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |             Type              |            Status             |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |                     Key....                                   |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
*/
struct nsm_msg_ipv6_mrib_notification_key_mif
{
  u_int32_t ifindex;

  u_int16_t mif_index;

  u_int16_t flags;
  /* Same flags as mif_add */
};

#define NSM_IPV6_MRIB_NOTIF_KEY_MIF_SIZE  8

struct nsm_msg_ipv6_mrib_notification_key_mrt
{
  struct pal_in6_addr src;

  struct pal_in6_addr group;

  u_int16_t flags;
  /* Same flags as mrt add */
};

#define NSM_IPV6_MRIB_NOTIF_KEY_MRT_SIZE  34

union nsm_msg_ipv6_mrib_notification_key
{
  struct nsm_msg_ipv6_mrib_notification_key_mif mif_key;

  struct nsm_msg_ipv6_mrib_notification_key_mrt mrt_key;
};

struct nsm_msg_ipv6_mrib_notification
{
  u_int16_t type;
#define NSM_MSG_IPV6_MRIB_NOTIFICATION_MIF_ADD            0
#define NSM_MSG_IPV6_MRIB_NOTIFICATION_MIF_DEL            1
#define NSM_MSG_IPV6_MRIB_NOTIFICATION_MRT_ADD            2
#define NSM_MSG_IPV6_MRIB_NOTIFICATION_MRT_DEL            3
#define NSM_MSG_IPV6_MRIB_NOTIFICATION_MCAST              4
#define NSM_MSG_IPV6_MRIB_NOTIFICATION_NO_MCAST           5
#define NSM_MSG_IPV6_MRIB_NOTIFICATION_CLR_MRT            6

  u_int16_t status;
#define MRIB_MIF_EXISTS                                   0
#define MRIB_MIF_NOT_EXIST                                1
#define MRIB_MIF_NO_INDEX                                 2
#define MRIB_MIF_NOT_BELONG                               3
#define MRIB_MIF_FWD_ADD_ERR                              4
#define MRIB_MIF_FWD_DEL_ERR                              5
#define MRIB_MIF_ADD_SUCCESS                              6
#define MRIB_MIF_DEL_SUCCESS                              7

/* These are similar to IPv4 notification. */
#ifndef HAVE_MCAST_IPV4
#define MRIB_MRT_LIMIT_EXCEEDED                           8
#define MRIB_MRT_EXISTS                                   9
#define MRIB_MRT_NOT_EXIST                                10
#define MRIB_MRT_NOT_BELONG                               11
#define MRIB_MRT_FWD_ADD_ERR                              12
#define MRIB_MRT_FWD_DEL_ERR                              13
#define MRIB_MRT_ADD_SUCCESS                              14
#define MRIB_MRT_DEL_SUCCESS                              15
#endif /* HAVE_MCAST_IPV4 */

#ifndef MRIB_INTERNAL_ERROR
#define MRIB_INTERNAL_ERROR                                16
#endif /* MRIB_INTERNAL_ERROR */

#ifndef MRIB_NO_MCAST
#define MRIB_NO_MCAST                                     17
#endif /* MRIB_NO_MCAST */

#ifndef MRIB_NA
#define MRIB_NA                                           18
#endif /* MRIB_NA */

  union nsm_msg_ipv6_mrib_notification_key key;
};


/*
   NSM_MSG_MLD

   0                   1                   2                   3
   0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |                           ifindex                             |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |  Filter-Mode (INCL | EXCL)    |         Num of Sources        |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |              IPv6 Multicast Group Address                     |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  .                    IPv6 source address                        .
  .-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-.
  .                    IPv6 source address                        .
  .-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-.
  .                    IPv6 source address                        .
  .-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-.
  .                    IPv6 source address                        .
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
*/

struct nsm_msg_mld
{
  /* Ifindex */
  u_int32_t ifindex;

  /* Filter Mode (INCLUDE | EXCLUDE) */
  u_int16_t filt_mode;
#define NSM_MSG_MLD_FILT_MODE_INCL              0
#define NSM_MSG_MLD_FILT_MODE_EXCL              1

  /* Number of Sources */
  u_int16_t num_srcs;

  /* Group address */
  struct pal_in6_addr grp_addr;

  /* Source addresses List */
  struct pal_in6_addr src_addr_list [1];
};

#define NSM_MSG_MLD_SIZE                        24

#endif /* HAVE_MCAST_IPV6 */

#ifdef HAVE_LACPD
/* NSM LACP Aggregator Add message

0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|                     Aggregator ID                             |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|                     Aggregator Name                           |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|                     Aggregator Name (contd.)                  |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|                     Aggregator Name (contd.)                  |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|                     Aggregator Name (contd.)                  |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|                     Aggregator Mac                            |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|   Aggregator Mac (contd.)   |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+


Added Ports TLV

Deleted Ports TLV
*/

#define LACP_AGG_NAME_LEN           16

struct nsm_msg_lacp_agg_add
{
  cindex_t cindex;
  char agg_name[LACP_AGG_NAME_LEN];
  u_char mac_addr[ETHER_ADDR_LEN];
  u_int16_t add_count;
  u_int16_t del_count;
  u_int32_t *ports_added;
  u_int32_t *ports_deleted;
  u_char agg_type;
};

#define NSM_MSG_LACP_AGG_CONFIG_SIZE          10
#define NSM_LACP_AGG_CTYPE_PORTS_DELETED      1
#define NSM_LACP_AGG_CTYPE_PORTS_ADDED        0
/* Message to inform Lacp from nsm cli regarding association of interface to lacp aggregator*/
struct nsm_msg_lacp_aggregator_config
{
  u_int32_t ifindex;
  u_int32_t key;
  u_char    chan_activate;
  u_char    add_agg;
};

#define NSM_LACP_AGG_CINDEX_NUM               2
#define AGG_CONFIG_NONE             0
#define AGG_CONFIG_LACP             1
#define AGG_CONFIG_STATIC           2

/* A dummy message to sent count of static aggregators to lacp*/

#define NSM_MSG_AGG_CNT_SIZE                2
#define NSM_MSG_INCR_AGG_CNT                1
#define NSM_MSG_DCR_AGG_CNT                 2

struct nsm_msg_agg_cnt
{
  u_int16_t agg_msg_type;
};

#endif /* HAVE_LACPD */

#ifdef HAVE_ONMD

/* EFM Interface Message

   0                   1                   2                   3
   0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |                             ifindex                           |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |    opcode                   |     local_mux_action            |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |    local_mux_action         |     local_event                 |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |    remote_event             |                                 |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

*/
#define NSM_MSG_EFM_IF_SIZE  34

#define NSM_EFM_OAM_INVALID_EVENT                        (0)
#define NSM_EFM_OAM_LINK_FAULT_EVENT                     (1 << 0)
#define NSM_EFM_OAM_CRITICAL_LINK_EVENT                  (1 << 1)
#define NSM_EFM_OAM_DYING_GASP_EVENT                     (1 << 2)

enum nsm_efm_mux_action
{
  NSM_MUX_ACTION_FWD,
  NSM_MUX_ACTION_DISCARD,
  NSM_MUX_ACTION_INVALID,
};

enum nsm_efm_par_action
{
  NSM_PAR_ACTION_FWD,
  NSM_PAR_ACTION_LB,
  NSM_PAR_ACTION_DISCARD,
  NSM_PAR_ACTION_INVALID,
};

enum nsm_efm_opcode
{
  NSM_EFM_ENABLE_OAM,
  NSM_EFM_DISABLE_OAM,
  NSM_EFM_SET_LOCAL_EVENT,
  NSM_EFM_SET_REMOTE_EVENT,
  NSM_EFM_UNSET_LOCAL_EVENT,
  NSM_EFM_UNSET_REMOTE_EVENT,
  NSM_EFM_SET_MUX_PAR_STATE,
  NSM_EFM_SET_SYMBOL_PERIOD_WINDOW,
  NSM_EFM_SET_FRAME_WINDOW,
  NSM_EFM_SET_FRAME_PERIOD_WINDOW,
  NSM_EFM_SET_FRAME_SECONDS_WINDOW,
  NSM_EFM_SET_SYMBOL_PERIOD_ERROR,
  NSM_EFM_SET_FRAME_EVENT_ERROR,
  NSM_EFM_SET_FRAME_PERIOD_ERROR,
  NSM_EFM_SET_FRAME_SECONDS_ERROR,
  NSM_EFM_SET_IF_DOWN,
  NSM_EFM_SET_LINK_MONITORING_ON,
  NSM_EFM_SET_LINK_MONITORING_OFF,
  NSM_EFM_OPERATION_INVALID,
};

/* EFM Port message */
struct nsm_msg_efm_if
{
#define NSM_EFM_INVALID_IFINDEX 0

  u_int32_t ifindex;

  u_int16_t  opcode;

  u_int16_t  local_par_action;

  u_int16_t  local_mux_action;

  u_int16_t  local_event;

  u_int16_t  remote_event;

  ut_int64_t symbol_period_window;

  u_int32_t  other_event_window;

  ut_int64_t num_of_error;
};

enum nsm_lldp_opcode
{
  NSM_LLDP_SET_SYSTEM_CAP,
  NSM_LLDP_UNSET_SYSTEM_CAP,
  NSM_LLDP_GET_SYSTEM_CAP,
  NSM_LLDP_IF_ENABLE,
  NSM_LLDP_IF_PROTO_SET,
  NSM_LLDP_IF_PROTO_UNSET,
  NSM_LLDP_IF_SET_AGG_PORT_ID,
  NSM_LLDP_SET_DEST_ADDR,
};

#define NSM_MSG_LLDP_SIZE  21

struct nsm_msg_lldp
{
  u_int16_t opcode;

#define NSM_LLDP_INVALID_IFINDEX 0
  u_int32_t ifindex;

#define NSM_LLDP_IPV4_ROUTING    (1 << 0)
#define NSM_LLDP_IPV6_ROUTING    (1 << 1)
#define NSM_LLDP_L2_SWICHING     (1 << 2)

  u_int16_t syscap;

#define NSM_LLDP_PROTO_UNKNOWN        (1 << 10)
#define NSM_LLDP_PROTO_EFM_OAM        (1 << 9)
#define NSM_LLDP_PROTO_DOT1X          (1 << 8)
#define NSM_LLDP_PROTO_LACP           (1 << 7)
#define NSM_LLDP_PROTO_MVRP           (1 << 6)
#define NSM_LLDP_PROTO_GVRP           (1 << 5)
#define NSM_LLDP_PROTO_MMRP           (1 << 4)
#define NSM_LLDP_PROTO_GMRP           (1 << 3)
#define NSM_LLDP_PROTO_MSTP           (1 << 2)
#define NSM_LLDP_PROTO_RSTP           (1 << 1)
#define NSM_LLDP_PROTO_STP            (1 << 0)

  u_int16_t protocol;

#define NSM_LLDP_AGG_PORT_INDEX_NONE  0
 u_int32_t agg_ifindex;

 u_int8_t  lldp_dest_addr[ETHER_ADDR_LEN];

#define NSM_LLDP_RET_OK   0
#define NSM_LLDP_RET_FAIL  -1
 u_int8_t lldp_ret_status;

};

enum nsm_cfm_opcode
{
  NSM_CFM_ENABLE_CCM_LEVEL,
  NSM_CFM_ENABLE_TR_LEVEL,
  NSM_CFM_DISABLE_CCM_LEVEL,
  NSM_CFM_DISABLE_TR_LEVEL,
  NSM_CFM_SET_DEST_ADDR,
  NSM_CFM_SET_ETHER_TYPE,
};


#define NSM_MSG_CFM_SIZE  15

struct nsm_msg_cfm
{
  u_int16_t opcode;


#define NSM_CFM_INVALID_IFINDEX 0
  u_int32_t ifindex;


  u_int8_t  cfm_dest_addr[ETHER_ADDR_LEN];

  u_int8_t  level;

  u_int16_t ether_type;
#if defined HAVE_CFM && (defined HAVE_I_BEB || defined HAVE_B_BEB)
  u_int32_t isid;
#endif /* (HAVE_CFM && (defined HAVE_I_BEB || defined HAVE_B_BEB))*/
};

#define NSM_MSG_CFM_STATUS_SIZE  6
struct nsm_msg_cfm_status
{
  u_int32_t ifindex;

  u_int8_t uni_mep_status;

  /* LOCAL_LINK_UP is defined zero as the Local MEP operational status 
   * at NSM must be UP by default.
   */
#define NSM_CFM_LOCAL_LINK_UP    0
#define NSM_CFM_LOCAL_LINK_DOWN  1
#define NSM_CFM_REMOTE_LINK_UP    2
#define NSM_CFM_REMOTE_LINK_DOWN  3

};

#endif /* HAVE_ONMD */

#define NSM_BRIDGE_PORT_STATE_DISABLED          1
#define NSM_BRIDGE_PORT_STATE_LISTENING         2
#define NSM_BRIDGE_PORT_STATE_LEARNING          3
#define NSM_BRIDGE_PORT_STATE_FORWARDING        4
#define NSM_BRIDGE_PORT_STATE_DISCARD_BLK       5
#ifdef HAVE_L2

/* Maximum bridge name length. */
#define NSM_BRIDGE_NAMSIZ                    (16 + 1)

/* Bridge Add/Delete message.

   0                   1                   2                   3
   0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |                                                               |
   |                        Bridge Name                            |
   |                                                               |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

   0
   0 1 2 3 4 5 6 7 8
   +-+-+-+-+-+-+-+-+
   |    Type       |
   +-+-+-+-+-+-+-+-+
*/

struct nsm_msg_bridge
{
  char bridge_name[NSM_BRIDGE_NAMSIZ];

  u_char type;
  u_char is_default;
  u_char topology_type;

  /* Is the bridge an edge bridge,
   * NSM_BRIDGE_TYPE_PROVIDER_MSTP + edge (or)
   * NSM_BRIDGE_TYPE_PROVIDER_RSTP + edge */
  u_int8_t is_edge;

  u_int32_t ageing_time;

#define NSM_BRIDGE_TYPE_STP                     1
#define NSM_BRIDGE_TYPE_STP_VLANAWARE           2
#define NSM_BRIDGE_TYPE_RSTP                    3
#define NSM_BRIDGE_TYPE_RSTP_VLANAWARE          4
#define NSM_BRIDGE_TYPE_MSTP                    5
#define NSM_BRIDGE_TYPE_PROVIDER_RSTP           6
#define NSM_BRIDGE_TYPE_PROVIDER_MSTP           7
#define NSM_BRIDGE_TYPE_CE                      8
#define NSM_BRIDGE_TYPE_RPVST_PLUS              12
#define NSM_BRIDGE_TYPE_BACKBONE_RSTP           32
#define NSM_BRIDGE_TYPE_BACKBONE_MSTP           96


/* As of now these bridge types are only being understood by ONMD but,
 * this is not being put in the HAVE_CFM flag as other modules might also
 * be desirous of this information */
#if defined HAVE_I_BEB || defined HAVE_B_BEB || defined HAVE_PBB_TE
#define NSM_BRIDGE_TYPE_PBB_I_COMPONENT         9
#define NSM_BRIDGE_TYPE_CNP                     10
#define NSM_BRIDGE_TYPE_CBP                     11
#endif /* defined HAVE_I_BEB || defined HAVE_B_BEB || defined HAVE_PBB_TE*/
};

#define NSM_BRIDGE_NOT_CONFIGURED               0

#define NSM_MSG_BRIDGE_SIZE                    (NSM_BRIDGE_NAMSIZ + 1)

/* Bridge interface add/delete message.

   0                   1                   2                   3
   0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |                                                               |
   |                        Bridge Name                            |
   |                                                               |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

   0                   1                   2                   3
   0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |                             ifindex1                          |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |                             ifindex2                          |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |                                ...                            |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+


*/

#if defined(HAVE_PROVIDER_BRIDGE) || defined(HAVE_I_BEB)

enum nsm_bridge_proto_process
{
  NSM_MSG_BRIDGE_PROTO_PEER,
  NSM_MSG_BRIDGE_PROTO_TUNNEL,
  NSM_MSG_BRIDGE_PROTO_DISCARD,
  NSM_MSG_BRIDGE_PROTO_PROCESS_MAX,
};
#endif /* HAVE_PROVIDER_BRIDGE || HAVE_I_BEB */

struct nsm_msg_bridge_if
{
  /* Ctype index. */
  cindex_t cindex;

  char bridge_name[NSM_BRIDGE_NAMSIZ];

#if defined (HAVE_PROVIDER_BRIDGE) || defined(HAVE_I_BEB)
  u_int8_t cust_bpdu_process;
#endif /* HAVE_PROVIDER_BRIDGE || HAVE_I_BEB */

  u_int16_t num;

  u_int8_t spanning_tree_disable;

  u_int32_t *ifindex;

  u_int32_t *instance;
};

#if defined HAVE_G8031 || defined HAVE_G8032
struct nsm_msg_bridge_pg
{
  /* Ctype index. */
  cindex_t cindex;

  char bridge_name[NSM_BRIDGE_NAMSIZ];

  u_int16_t num;

  u_int16_t instance;

  u_int8_t spanning_tree_disable;

  u_int32_t *ifindex;
};
#endif /* HAVE_G8031 || defined HAVE_G8032 */

#ifdef HAVE_PROVIDER_BRIDGE
#define NSM_MSG_BRIDGE_IF_SIZE               5
#else
#define NSM_MSG_BRIDGE_IF_SIZE               4
#endif /* HAVE_PROVIDER_BRIDGE */

#define NSM_MSG_BRIDGE_CTYPE_PORT            0

/* Port state message */
struct nsm_msg_bridge_port
{
  cindex_t cindex;

  char bridge_name[NSM_MSG_BRIDGE_SIZE];

  u_int32_t ifindex;

  u_int16_t num;

  u_int32_t *instance;

  u_int32_t *state;
#define NSM_BRIDGE_PORT_STATE_DISABLED          1
#define NSM_BRIDGE_PORT_STATE_LISTENING         2
#define NSM_BRIDGE_PORT_STATE_LEARNING          3
#define NSM_BRIDGE_PORT_STATE_FORWARDING        4
#define NSM_BRIDGE_PORT_STATE_BLOCKING          5
#define NSM_BRIDGE_PORT_STATE_DISCARDING        6
#define NSM_BRIDGE_PORT_STATE_DISCARDING_EDGE   7 /* For edge ports, dont flush fdb */
#define NSM_BRIDGE_PORT_STATE_ENABLED           8
};

#ifdef HAVE_PBB_TE
struct nsm_msg_bridge_pbb_te_port
{
  u_int32_t    eps_grp_id;

  u_int32_t    te_sid_a;

/*  u_int32_t    state;
#define NSM_BRIDGE_PBB_TE_PORT_STATE_FORWARD    1
#define NSM_BRIDGE_PBB_TE_PORT_STATE_BLOCK      2 */
};
#endif /* HAVE_PBB_TE */

#define NSM_MSG_BRIDGE_PORT_SIZE            (NSM_MSG_BRIDGE_SIZE + 12)
#define NSM_BRIDGE_CTYPE_PORT_INSTANCE      0
#define NSM_BRIDGE_CTYPE_PORT_STATE         1
#define NSM_BRIDGE_CTYPE_PORT_MAX           2

/* Bridge Enable/Disable Message */
struct nsm_msg_bridge_enable
{
  char bridge_name[NSM_MSG_BRIDGE_SIZE];

  u_int16_t enable;

};

#endif /* HAVE_L2 */

#ifndef HAVE_CUSTOM1
#ifdef HAVE_VLAN

/* Maximum VLAN name length. */
#define NSM_VLAN_NAMSIZ                      (16 + 1)

/* NSM VLAN Add/Delete message.

   0                   1                   2                   3
   0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |                         |D|X|S|        VLAN ID                |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

   VLAN Name TLV
   0                   1                   2                   3
   0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |                                                               |
   |                        VLAN Name                              |
   |                                                               |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

   Bridge Name TLV
   0                   1                   2                   3
   0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |                                                               |
   |                        Bridge Name                            |
   |                                                               |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

Flags:
 S for vlan state
 X for static vlan
 D for dynamic vlan

*/

struct nsm_msg_vlan
{
  cindex_t cindex;

  u_int16_t flags;
#define NSM_MSG_VLAN_STATE_ACTIVE            (1 << 0)
#define NSM_MSG_VLAN_STATIC                  (1 << 1)
#define NSM_MSG_VLAN_DYNAMIC                 (1 << 2)
#define NSM_MSG_VLAN_CVLAN                   (1 << 3)
#define NSM_MSG_VLAN_SVLAN                   (1 << 4)
#define NSM_MSG_VLAN_AUTO                    (1 << 5)

  /* VLAN id. */
  u_int16_t vid;

  /* VLAN name. */
  char vlan_name[NSM_VLAN_NAMSIZ];

  /* Bridge name. */
  char bridge_name[NSM_BRIDGE_NAMSIZ];

  /* bridge instance for mstp */
  u_int16_t br_instance;
};

#define NSM_MSG_VLAN_SIZE                     4

#define NSM_VLAN_CTYPE_NAME                   0
#define NSM_VLAN_CTYPE_BRIDGENAME             1
#define NSM_VLAN_CTYPE_INSTANCE               2

#define NSM_VLAN_CINDEX_NUM                   3

/* NSM VLAN interface message.
   It is used to add VLANs for a interface(port).

   0                   1                   2                   3
   0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |                        ifindex                                |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

   0                   1                   2                   3
   0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |E|D|         VLAN1             |E|D|         VLAN2             |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |E|D|        VLAN3              |E|D|         VLAN4             |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |E|D|         ...               |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

where:
E - Egress tagged for the VLAN
D - Default VID for the port(only one VLAN can be set as default in
the set.
*/

#define NSM_VLAN_EGRESS_SHIFT_BITS             15
#define NSM_VLAN_DEFAULT_SHIFT_BITS            14
#define NSM_VLAN_VID_MASK                      (~((1 << NSM_VLAN_EGRESS_SHIFT_BITS) | (1 << NSM_VLAN_DEFAULT_SHIFT_BITS)))

struct nsm_msg_vlan_port
{
  cindex_t cindex;

  u_int32_t ifindex;

  u_int16_t num;
  u_int16_t *vid_info;
};

#define NSM_MSG_VLAN_PORT_SIZE                 4

#define NSM_VLAN_PORT_CTYPE_VID_INFO           0

#define NSM_VLAN_PORT_CINDEX_NUM               1

/* NSM VLAN port type message.

   0                   1                   2                   3
   0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |                        ifindex                                |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   | Port type     | Ingress filter| Acceptable    |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

*/
struct nsm_msg_vlan_port_type
{
  u_int32_t ifindex;

  u_char port_type;
#define NSM_MSG_VLAN_PORT_MODE_INVALID             0
#define NSM_MSG_VLAN_PORT_MODE_ACCESS              1
#define NSM_MSG_VLAN_PORT_MODE_HYBRID              2
#define NSM_MSG_VLAN_PORT_MODE_TRUNK               3
#define NSM_MSG_VLAN_PORT_MODE_CE                  4
#define NSM_MSG_VLAN_PORT_MODE_CN                  5
#define NSM_MSG_VLAN_PORT_MODE_PN                  6
#define NSM_MSG_VLAN_PORT_MODE_PE                  7

#define NSM_MSG_VLAN_PORT_MODE_CNP                 8
#define NSM_MSG_VLAN_PORT_MODE_VIP                 9
#define NSM_MSG_VLAN_PORT_MODE_PIP                 10
#define NSM_MSG_VLAN_PORT_MODE_CBP                 11
#define NSM_MSG_VLAN_PORT_MODE_PNP                 NSM_MSG_VLAN_PORT_MODE_PN

  u_char ingress_filter;
#define NSM_MSG_VLAN_PORT_DISABLE_INGRESS_FILTER   0
#define NSM_MSG_VLAN_PORT_ENABLE_INGRESS_FILTER    1

  u_char acceptable_frame_type;
#define NSM_MSG_VLAN_ACCEPT_FRAME_ALL              0
#define NSM_MSG_VLAN_ACCEPT_UNTAGGED_ONLY          1
#define NSM_MSG_VLAN_ACCEPT_TAGGED_ONLY            2
};

#define NSM_MSG_VLAN_PORT_TYPE_SIZE                4

#ifdef HAVE_PVLAN
#define NSM_MSG_PVLAN_IF_MSG                       (NSM_BRIDGE_NAMSIZ + 6)
#define NSM_MSG_PVLAN_CONFIG_MSG                   (NSM_BRIDGE_NAMSIZ + 3)
#define NSM_MSG_PVLAN_ASSOCIATE_MSG                (NSM_BRIDGE_NAMSIZ + 2)
struct nsm_msg_pvlan_if
{
  char bridge_name[NSM_BRIDGE_NAMSIZ];
  u_int32_t ifindex;
  u_int16_t flags;
  #define NSM_PVLAN_DISABLE_PORTFAST_BPDUGUARD    0
  #define NSM_PVLAN_ENABLE_PORTFAST_BPDUGUARD     1
};

struct nsm_msg_pvlan_association
{
  char bridge_name[NSM_BRIDGE_NAMSIZ];
  u_int16_t primary_vid;
  u_int16_t secondary_vid;
  u_int16_t flags;
  #define NSM_PVLAN_SECONDARY_ASSOCIATE         1
  #define NSM_PVLAN_SECONDARY_ASSOCIATE_CLEAR   0
};

struct nsm_msg_pvlan_configure
{
  char bridge_name[NSM_BRIDGE_NAMSIZ];
  u_int16_t vid;
  u_int16_t type;
  #define NSM_PVLAN_CONFIGURE_NONE            0
  #define NSM_PVLAN_CONFIGURE_PRIMARY         1
  #define NSM_PVLAN_CONFIGURE_SECONDARY       2
};
#endif /* HAVE_PVLAN */

#ifdef HAVE_VLAN_CLASS

/* NSM VLAN Classifier message.

   0                   1                   2                   3
   0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |                           group                               |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |                           type                                |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |                                                               |
   |                                                               |
   |                           data                                |
   |                                                               |
   |                                                               |
   |                                                               |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
*/

#define NSM_MSG_MAX_CLASS_DATA_LEN                 50
#define NSM_MSG_VLAN_CLASSSIZ                      51
struct nsm_msg_vlan_classifier
{
  cindex_t cindex;

  /* Group Identifier */
  u_int32_t class_group;

  /* Classifier type */
  u_int32_t class_type;
#define NSM_MSG_VLAN_CLASS_SRC_MAC    (1 << 0)
#define NSM_MSG_VLAN_CLASS_PROTO      (1 << 1)
#define NSM_MSG_VLAN_CLASS_SUBNET     (1 << 2)

#define NSM_MSG_VLAN_CLASS_TYPE_MASK  ((1 << 0) | (1 << 1) | (1 << 2))

#define NSM_MSG_VLAN_CLASS_LLC        ((1 << 16) | NSM_MSG_VLAN_CLASS_PROTO)
#define NSM_MSG_VLAN_CLASS_APPLETALK  ((1 << 17) | NSM_MSG_VLAN_CLASS_PROTO)
#define NSM_MSG_VLAN_CLASS_IPX        ((1 << 18) | NSM_MSG_VLAN_CLASS_PROTO)
#define NSM_MSG_VLAN_CLASS_ETHERNET   ((1 << 19) | NSM_MSG_VLAN_CLASS_PROTO)

  /* Null terminated argument */
  char class_data[NSM_MSG_MAX_CLASS_DATA_LEN + 1];
};

#define NSM_MSG_VLAN_CLASSIFIER_SIZE             8
#define NSM_VLAN_CTYPE_CLASSIFIER                0

#define NSM_VLAN_CLASSIFIER_CINDEX_NUM           1

/* NSM Vlan Port Classifier message

   0                   1                   2                   3
   0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |                          ifindex                              |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |              num              |E|D|          VLAN1            |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |                          Classifier 1                         |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |E|D|          VLAN2            |                         Class-|
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |-fier 2                        |              ......           |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
*/

struct nsm_msg_vlan_port_classifier
{
  cindex_t cindex;

  u_int32_t ifindex;
  u_int16_t vid_info;

  u_int16_t cnum;
  u_int8_t  *class_info;
};

#define NSM_VLAN_PORT_CTYPE_CLASS_INFO           0

#define NSM_VLAN_PORT_CLASS_CINDEX_NUM           1
#endif /* HAVE_VLAN_CLASS */
#endif /* HAVE_VLAN */
#endif /* HAVE_CUSTOM1 */

#ifdef HAVE_L2
struct nsm_msg_stp
{
  u_int16_t flags;
#define NSM_MSG_INTERFACE_DOWN   (1 << 0)
#define NSM_MSG_INTERFACE_UP     (1 << 1)

  u_int32_t ifindex;
};
#define NSM_MSG_STP_SIZE              4

#ifdef HAVE_ONMD
struct nsm_msg_cfm_mac
{
  u_char name[NSM_BRIDGE_NAMSIZ];

  u_char mac[ETHER_ADDR_LEN];

  u_int16_t vid;

  u_int16_t svid;

};

struct nsm_msg_cfm_ifindex
{
  u_int32_t index;
};
#endif /* HAVE_ONMD */

#endif /* HAVE_L2 */

#ifdef HAVE_AUTHD

/* NSM Port Authentication port state message

   0                   1                   2                   3
   0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |                        Interface Index                        |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |     Port Control Direction    |          Port State           |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
*/

struct nsm_msg_auth_port_state
{
  u_int32_t ifindex;

  /* Port Contrl direction.  */
  u_int16_t ctrl;
#define NSM_AUTH_PORT_CTRL_UNCONTROLLED         0
#define NSM_AUTH_PORT_CTRL_DIR_BOTH             1
#define NSM_AUTH_PORT_CTRL_DIR_IN               2

  /* Port State.  */
  u_int16_t state;
#define NSM_AUTH_PORT_STATE_AUTHORIZED          1
#define NSM_AUTH_PORT_STATE_UNAUTHORIZED        2
#define NSM_AUTH_PORT_ENABLE                    3
#define NSM_AUTH_PORT_DISABLE                   4
};

#define NSM_MSG_AUTH_PORT_STATE_SIZE            8

#ifdef HAVE_MAC_AUTH
#define NSM_MSG_MACAUTH_PORT_STATE_SIZE         8
#define NSM_MSG_AUTH_MAC_STATUS_SIZE            30

struct nsm_msg_auth_mac_port_state
{
  u_int32_t ifindex;
  u_int16_t state;
#define NSM_MACAUTH_PORT_STATE_ENABLED          0
#define NSM_MACAUTH_PORT_STATE_DISABLED         1
  u_int16_t mode;
};

#define AUTH_ETHER_LEN                          6
#define AUTH_ETHER_DELIMETER_LEN                (5+1)
struct nsm_msg_auth_mac_status
{
  u_int32_t ifindex;
  u_char status;
  u_char mac_addr[AUTH_ETHER_LEN * 2 + AUTH_ETHER_DELIMETER_LEN];
  u_int16_t radius_dynamic_vid;
  u_int16_t auth_fail_restrict_vid;
  u_int8_t mac_address_aging;
  u_int8_t auth_fail_action;
  u_int8_t dynamic_vlan_creation;
#define NSM_AUTH_MAC_AUTH_ACCEPT                1
#define NSM_AUTH_MAC_AUTH_REJECT                0
#define NSM_AUTH_MAC_AUTH_INVALID               2
#define NSM_AUTH_MAC_RESTRICT_VLAN              0
#define NSM_AUTH_MAC_DROP_TRAFFIC               1
#define NSM_AUTH_MAC_AUTH_FAIL_INVLAID          2
#define NSM_AUTH_MAC_AGING_ENABLE               1
#define NSM_AUTH_MAC_AGING_DISABLE              0
#define NSM_AUTH_DYNAMIC_VLAN_ENABLE            1
#define NSM_AUTH_DYNAMIC_VLAN_DISABLE           0
};
#endif /* HAVE_MAC_AUTH */
#endif /* HAVE_AUTHD */

#ifdef HAVE_RMOND
struct nsm_msg_rmon
{
  u_int32_t ifindex;
};

struct nsm_msg_rmon_stats
{
  u_int32_t ifindex;
  ut_int64_t good_octets_rcv;
  ut_int64_t bad_octets_rcv;
  ut_int64_t mac_transmit_err;
  ut_int64_t good_pkts_rcv;
  ut_int64_t bad_pkts_rcv;
  ut_int64_t brdc_pkts_rcv;
  ut_int64_t mc_pkts_rcv;
  ut_int64_t pkts_64_octets;
  ut_int64_t pkts_65_127_octets;
  ut_int64_t pkts_128_255_octets;
  ut_int64_t pkts_256_511_octets;
  ut_int64_t pkts_512_1023_octets;
  ut_int64_t pkts_1024_max_octets;
  ut_int64_t good_octets_sent;
  ut_int64_t good_pkts_sent;
  ut_int64_t excessive_collisions;
  ut_int64_t mc_pkts_sent;
  ut_int64_t brdc_pkts_sent;
  ut_int64_t unrecog_mac_cntr_rcv;
  ut_int64_t fc_sent;
  ut_int64_t good_fc_rcv;
  ut_int64_t drop_events;
  ut_int64_t undersize_pkts;
  ut_int64_t fragments_pkts;
  ut_int64_t oversize_pkts;
  ut_int64_t jabber_pkts;
  ut_int64_t mac_rcv_error;
  ut_int64_t bad_crc;
  ut_int64_t collisions;
  ut_int64_t late_collisions;
  ut_int64_t bad_fc_rcv;
};

int
nsm_encode_rmon_msg (u_char **pnt,
                     u_int16_t *size,
                     struct nsm_msg_rmon *msg);
int
nsm_decode_rmon_msg (u_char **pnt,
                     u_int16_t *size,
                     struct nsm_msg_rmon *msg);

int
nsm_encode_rmon_stats (u_char **pnt, u_int16_t *size,
                       struct nsm_msg_rmon_stats *msg);

int
nsm_decode_rmon_stats (u_char **pnt, u_int16_t *size,
                       struct nsm_msg_rmon_stats *msg);

#endif /* HAVE_RMON */

/* As of now these bridge types are only being understood by ONMD but,
 * this is not being put in the HAVE_CFM flag as other modules might also
 * be desirous of this information */
#if defined HAVE_I_BEB || defined HAVE_B_BEB

struct nsm_msg_pbb_isid_to_pip
{
  u_int8_t bridge_name[NSM_BRIDGE_NAMSIZ];
  u_int16_t type;
  u_int32_t cnp_ifindex;
  u_int32_t pip_ifindex;
  u_int32_t cbp_ifindex;
  u_int32_t isid;
  u_int16_t svid_high;
  u_int16_t svid_low;
  u_int16_t dispatch_status;
};

struct nsm_msg_pbb_isid_to_bvid
{
  u_int8_t bridge_name[NSM_BRIDGE_NAMSIZ];
  u_int32_t cbp_ifindex;
  u_int32_t isid;
  u_int16_t bvid;
  u_int16_t dispatch_status;
};
#endif /* defined HAVE_I_BEB || defined HAVE_B_BEB) */

struct nsm_msg_nexthop_tracking
{
/* used by protocol to inform NSM that
   nexthop tracking is enabled/disabled in the protocol
   if nht is enabled, flags = PAL_TRUE
   if nht is disabld, flage = PAL_FALSE */
  u_int8_t flags;
#define NSM_MSG_NEXTHOP_TRACKING_ENABLE (1 << 0)
#define NSM_MSG_NEXTHOP_TRACKING_DISABLE (1 << 1)
};

#ifdef HAVE_G8031
struct nsm_msg_g8031_pg
{
  u_int8_t bridge_name[NSM_BRIDGE_NAMSIZ];
  u_int16_t eps_id;
  u_int32_t working_ifindex;
  u_int32_t protected_ifindex;
};

struct nsm_msg_g8031_vlanpg
{
  u_int8_t pvid;
  u_int8_t bridge_name[NSM_BRIDGE_NAMSIZ];
  u_int16_t eps_id;
  u_int16_t vid;

};

enum nsm_eps_opcode
{
  NSM_G8031_PG_INITIALIZED,
  NSM_G8031_PG_PORTSTATE
};

struct nsm_msg_pg_initialized
{
  u_int16_t eps_id;
  u_int8_t bridge_name[NSM_BRIDGE_NAMSIZ];
  u_int32_t g8031_pg_state;
};

struct nsm_msg_g8031_portstate
{
  u_int16_t eps_id;
  u_int8_t bridge_name[NSM_BRIDGE_NAMSIZ];
  u_int32_t ifindex_fwd;
  u_int32_t state_f;
#define NSM_BRIDGE_G8031_PORTSTATE_FORWARDING  1
  u_int32_t ifindex_blck;
  u_int32_t state_b;
#define NSM_BRIDGE_G8031_PORTSTATE_BLOCKING    2
};

#endif /* HAVE_G8031 */

#ifdef HAVE_PBB_TE
struct nsm_msg_pbb_te_vlan
{
  u_int32_t tesid;
  u_int16_t vid;
  u_int8_t brname[NSM_BRIDGE_NAMSIZ];
};

struct nsm_msg_pbb_te_esp
{
  u_int32_t tesid;
  u_int32_t esp_index;
  u_int32_t cbp_ifindex;
  u_int8_t  remote_mac[ETHER_ADDR_LEN];
  u_int8_t  mcast_rx_mac[ETHER_ADDR_LEN];
  u_int16_t esp_vid;
  bool_t    multicast;
  bool_t    egress;
};

struct nsm_msg_pbb_esp_pnp
{
  u_int32_t tesid;
  u_int32_t pnp_ifindex;
  u_int32_t cbp_ifindex;
};

struct nsm_msg_te_aps_grp
{
  uint32_t te_aps_grpid;
  uint32_t tesid_w;
  uint32_t tesid_p;
  uint32_t ifindex;
};

#endif /* HAVE_PBB_TE */

#ifdef HAVE_G8032

/*  message to be send from NSM to ONMD for addition of VLAN at G8032*/
struct nsm_msg_g8032_vlan
{
  /* primary vlan*/
  u_int8_t is_primary;

  /*bridge name */
  u_int8_t bridge_name[NSM_BRIDGE_NAMSIZ];

  /* ring id */
  u_int16_t ring_id;

  /* secondary vlan id */
  u_int16_t vid;
};

/* Msg send from NSM to ONMD on creation of a ring */
struct nsm_msg_g8032
{
   /* Bridge Name */
   u_int8_t bridge_name[NSM_BRIDGE_NAMSIZ];

   /* Ring ID */
   u_int16_t ring_id;

   /* east interface */
   u_int32_t east_ifindex;

   /* east interface */
   u_int32_t west_ifindex;
};

/* msg send from G8032 to NSM for port state change BLOCK /FWD*/
struct nsm_msg_bridge_g8032_port
{
 /* Ring ID */
  u_int16_t ring_id;

  /* state */
  u_int8_t state;
#define NSM_BRIDGE_G8032_PORTSTATE_FORWARD    1
#define NSM_BRIDGE_G8032_PORTSTATE_BLOCKING   2
#define NSM_BRIDGE_G8032_PORTSTATE_ERROR      3

  /* Flush port instance based for this port */
  u_int8_t fdb_flush;
#define G8032_FLUSH_NO_FDB                   0
#define G8032_FLUSH_FDB                      1
#define G8032_FLUSH_FAIL                     2

  /* Bridge Name */
  u_int8_t bridge_name[NSM_BRIDGE_NAMSIZ];

  /* Interface index */
  u_int32_t ifindex;

};
#endif /* HAVE_G8032*/

#ifdef HAVE_ELMID

#define NSM_MSG_ELMI_STATUS_SIZE             5
struct nsm_msg_elmi_status
{
  u_int32_t ifindex;

  /* ELMI operational Status */
  u_int8_t elmi_operational_status;

#define NSM_ELMI_OPERATIONAL_STATUS_UP       1
#define NSM_ELMI_OPERATIONAL_STATUS_DOWN     2

};

#define NSM_MSG_EVC_SIZE                     4

#define NSM_ELMI_CTYPE_BR_NAME 	             1
#define NSM_ELMI_CTYPE_IF_INDEX		     2
#define NSM_ELMI_CTYPE_EVC_REF_ID            3
#define NSM_ELMI_CTYPE_EVC_ID_STR            4
#define NSM_ELMI_CTYPE_EVC_STATUS_TYPE       5
#define NSM_ELMI_CTYPE_EVC_TYPE              6
#define NSM_ELMI_CTYPE_EVC_BW_PROFILE_EVC    7
#define NSM_ELMI_CTYPE_DEFAULT_EVC           8
#define NSM_ELMI_CTYPE_UNTAGGED_PRI          9
#define NSM_ELMI_CTYPE_EVC_BW_PROFILE_COS    10
#define NSM_ELMI_CTYPE_EVC_CVLAN_BITMAPS     11

#define NSM_EVC_CINDEX_NUM                   12

#define NSM_ELMI_CTYPE_UNI_ID                1
#define NSM_ELMI_CTYPE_UNI_CVLAN_MAP_TYPE    2
#define NSM_ELMI_CTYPE_UNI_BW_PROFILE_EVC    3
#define NSM_UNI_CINDEX_NUM                   6

/* Data structures for ELMI-NSM Client */
#define NSM_UNI_NAME_LEN                     64
#define NSM_EVC_NAME_LEN                     100

#define NSM_ELMI_MAX_COS                     8

#define NSM_ELMI_BW_SIZE                     4

#define NSM_ELMI_CTYPE_UNI_BW                0
#define NSM_ELMI_CTYPE_EVC_BW                1
#define NSM_ELMI_CTYPE_EVC_COS_BW            2

#define NSM_ELMI_EVC_COSID_1             (1 << 0)
#define NSM_ELMI_EVC_COSID_2             (1 << 1)
#define NSM_ELMI_EVC_COSID_3             (1 << 2)
#define NSM_ELMI_EVC_COSID_4             (1 << 3)
#define NSM_ELMI_EVC_COSID_5             (1 << 4)
#define NSM_ELMI_EVC_COSID_6             (1 << 5)
#define NSM_ELMI_EVC_COSID_7             (1 << 6)
#define NSM_ELMI_EVC_COSID_8             (1 << 7)

struct nsm_msg_bw_profile
{
  /* Ctype index.  */
  cindex_t cindex;
  u_int32_t ifindex;
  u_int16_t evc_ref_id;
  u_int32_t cir;      /* committed Information rate */
  u_int32_t cbs;      /* committed burst size */
  u_int32_t eir;      /* excess information rate */
  u_int32_t ebs;      /* excess burst size */

  u_int8_t cm;        /* color flag*/
  u_int8_t cf;        /* coupling flag */

  u_int8_t cos_id;    /* cos id used for per evc cos */
  u_int8_t per_cos;   /* cos values (priority bits)*/
};

enum nsm_uni_service_type
{
  NSM_ELMI_ALL_TO_ONE_BUNDLING = 0x01,
  NSM_ELMI_SERVICE_MULTIPLEXING,
  NSM_ELMI_BUNDLING,
};

enum nsm_elmi_status_type
{
  NSM_EVC_STATUS_NOT_ACTIVE = 0x0,
  NSM_EVC_STATUS_NEW_AND_NOTACTIVE,
  NSM_EVC_STATUS_NEW_ACTIVE,
  NSM_EVC_STATUS_NEW_AND_ACTIVE,
};

#define NSM_MSG_UNI_SIZE    4

struct nsm_msg_uni
{
  u_int32_t cindex;
  u_int32_t ifindex;
  u_int8_t  uni_id[NSM_UNI_NAME_LEN];
  enum nsm_uni_service_type cevlan_evc_map_type;
  struct nsm_msg_bw_profile uni_bw;
};

/* VLAN bitmap manipulation macros. */
#define NSM_EVC_MAX                 4096
#define NSM_EVC_BMP_WORD_WIDTH      32
#define NSM_EVC_BMP_WORD_MAX                  \
((NSM_EVC_MAX + NSM_EVC_BMP_WORD_WIDTH) / NSM_EVC_BMP_WORD_WIDTH)

struct nsm_evc_bmp
{
  u_int32_t bitmap[NSM_EVC_BMP_WORD_MAX];
};

struct nsm_msg_elmi_evc
{
  u_int32_t cindex;
  u_int8_t bridge_name[NSM_BRIDGE_NAMSIZ];
  u_int32_t ifindex ;
  u_int16_t evc_ref_id;                 /* EVC reference id, svid */
  u_int16_t num;
  u_int16_t *vid_info;
  u_int8_t evc_id[NSM_EVC_NAME_LEN+1];  /* evc id */
  u_int8_t evc_status_type;
  u_int8_t evc_type;
#define NSM_ELML_PT_PT_EVC             0x0
#define NSM_ELMI_MLPT_MLPT_EVC         0x1

  struct nsm_evc_bmp cvlanMemberBmp;   /* ce-vlans mapping to this evc */

  u_int8_t untagged_frame;             /* Untagged/Priority Tagged */

  u_int8_t default_evc;
  u_int8_t bw_profile_type;            /* per UNI/EVC/COS */
};
#endif /* HAVE_ELMID */

#ifdef HAVE_GMPLS

/* Message between NSM and protocols to transfer Data Link information */
#define NSM_MSG_DATA_LINK_MIN_SIZE              8
struct nsm_msg_data_link
{
  cindex_t cindex;
#define NSM_DATALINK_CTYPE_ADD                   (1 << 1)
#define NSM_DATALINK_CTYPE_DEL                   (1 << 2)
#define NSM_DATALINK_CTYPE_PROPERTY              (1 << 3)
#define NSM_DATALINK_CTYPE_LOCAL_LINKID          (1 << 4)
#define NSM_DATALINK_CTYPE_REMOTE_LINKID         (1 << 5)
#define NSM_DATALINK_CTYPE_STATUS                (1 << 6)
#define NSM_DATALINK_CTYPE_TE_GIFINDEX           (1 << 7)
#define NSM_DATALINK_CTYPE_FLAG                  (1 << 8)
#define NSM_DATALINK_CTYPE_PHY_PROPERTY          (1 << 9)
#define NSM_DATALINK_CTYPE_LINK_TYPE             (1 << 10)

  u_char name[INTERFACE_NAMSIZ + 1];

  /* ifp's ifindex */
  u_int32_t ifindex;

  u_int32_t gifindex;

  struct gmpls_link_id l_linkid;
  struct gmpls_link_id r_linkid;

  u_int32_t te_gifindex;
  u_char tlname[INTERFACE_NAMSIZ + 1];

  u_char status;

  /* For NSM to protocol the datalink flags are used */
  u_int16_t flags;
#define NSM_DATALINK_LINK_ID_MISMATCH_IN_LMP        (1 << 0)
#define NSM_DATALINK_PROP_MISMATCH_IN_LMP           (1 << 1)
#define NSM_DATALINK_TYPE_OPAQUE                    (1 << 2)

  struct link_properties prop;
  struct phy_properties p_prop;
};

/* Message between NSM and protocols to transfer Data Link reduced information.
 * Mainly for protocols such as RSVP which does not need full set of Data Link
 * information and registered for NSM_SERVICE_DATA_LINK_SUB */
struct nsm_msg_data_link_sub
{
  /* Flags same as Datalink service */
  cindex_t cindex;

  /* Data Linke information */
  char name[INTERFACE_NAMSIZ + 1];
  /* ifp's ifindex */
  u_int32_t ifindex;
  u_int32_t gifindex;
  struct gmpls_link_id l_linkid;
  struct gmpls_link_id r_linkid;
  u_int32_t te_gifindex;
  u_char tlname[INTERFACE_NAMSIZ + 1];
  u_char status;
  /* For NSM to protocol the datalink flags are used */
  u_int16_t flags;
};

#define NSM_MSG_TELINK_MIN_SIZE                    12

/* Message between NSM and protocols to transfer TE Link information */
struct nsm_msg_te_link
{
  cindex_t cindex;
#define NSM_TELINK_CTYPE_ADD                       (1 << 1)
#define NSM_TELINK_CTYPE_DEL                       (1 << 2)
#define NSM_TELINK_CTYPE_LOCAL_LINKID              (1 << 3)
#define NSM_TELINK_CTYPE_REMOTE_LINKID             (1 << 4)
#define NSM_TELINK_CTYPE_CAGIFINDEX                (1 << 5)
#define NSM_TELINK_CTYPE_LINK_PROPERTY             (1 << 6)
#define NSM_TELINK_CTYPE_PHY_PROPERTY              (1 << 7)
#define NSM_TELINK_CTYPE_LINK_ID                   (1 << 8)
#define NSM_TELINK_CTYPE_ADMIN_GROUP               (1 << 9)
#define NSM_TELINK_CTYPE_SRLG_UPD                  (1 << 10)
#define NSM_TELINK_CTYPE_SRLG_CLR                  (1 << 11)
#define NSM_TELINK_CTYPE_STATUS                    (1 << 12)
#define NSM_TELINK_CTYPE_FLAG                      (1 << 13)
#define NSM_TELINK_CTYPE_ADDR                      (1 << 14)
#define NSM_TELINK_CTYPE_REMOTE_ADDR               (1 << 15)
#define NSM_TELINK_CTYPE_TE_PROPERTY               (1 << 16)
#define NSM_TELINK_CTYPE_BCMODE                    (1 << 17)
#define NSM_TELINK_CTYPE_MTU                       (1 << 18)

  char name[INTERFACE_NAMSIZ + 1];

  u_int32_t gifindex;
  u_int32_t flags;
#define NSM_TELINK_LINK_ID_MISMATCH_IN_LMP   (1 << 0)
#define NSM_TELINK_PROP_MISMATCH_IN_LMP      (1 << 1)
  struct gmpls_link_id l_linkid;
  struct gmpls_link_id r_linkid;
  struct prefix addr;
  struct prefix remote_addr;

  u_char status;

#ifdef HAVE_DSTE
  u_char mode;
#endif /* HAVE_DSTE */

  /* Control adjacency gifindex */
  u_int32_t ca_gifindex;
  char caname[INTERFACE_NAMSIZ + 1];

  struct link_properties l_prop;
  struct phy_properties p_prop;

  u_int32_t admin_group;
  struct pal_in4_addr linkid;
  u_char srlg_num;
  u_int32_t *srlg;
  u_int32_t mtu;
};

/* Message between NSM and protocols to transfer Control Channel information */
struct nsm_msg_control_channel
{
  cindex_t cindex;
#define NSM_CONTROL_CHANNEL_CTYPE_ADD                      1
#define NSM_CONTROL_CHANNEL_CTYPE_DEL                      2
#define NSM_CONTROL_CHANNEL_CTYPE_NAME                     3
#define NSM_CONTROL_CHANNEL_CTYPE_LOCAL_ADDR               4
#define NSM_CONTROL_CHANNEL_CTYPE_REMOTE_ADDR              5
#define NSM_CONTROL_CHANNEL_CTYPE_BINDING                  6
#define NSM_CONTROL_CHANNEL_CTYPE_CAGIFINDEX               7
#define NSM_CONTROL_CHANNEL_CTYPE_STATUS                   8

  char name[INTERFACE_NAMSIZ + 1];

  u_int32_t gifindex;

  /* binded ifp ifindex */
  u_int32_t bifindex;

  /* CCID */
  u_int32_t ccid;

  struct pal_in4_addr l_addr;
  struct pal_in4_addr r_addr;

  u_char status;

  /* primary should always be send to protocols with caname and cagifindex */
  u_char primary;
  char caname[INTERFACE_NAMSIZ + 1];
  u_int32_t cagifindex;
};

/* Primary control_channel information in control adjacency */
struct control_adj_use_cc
{
  char name[INTERFACE_NAMSIZ + 1];
  u_int32_t gifindex;
  u_int32_t ccid;
  u_int32_t bifindex;
  struct pal_in4_addr l_addr;
  struct pal_in4_addr r_addr;
};

/* Message between NSM and protocols to transfer Control adjacency
 * information */
struct nsm_msg_control_adj
{
  cindex_t cindex;
#define NSM_CONTROL_ADJ_CTYPE_ADD                       1
#define NSM_CONTROL_ADJ_CTYPE_DEL                       2
#define NSM_CONTROL_ADJ_CTYPE_REMOTE_NODEID             3
#define NSM_CONTROL_ADJ_CTYPE_USE_CC                    4
#define NSM_CONTROL_ADJ_CTYPE_LMP_INUSE                 5
#define NSM_CONTROL_ADJ_CTYPE_FLAG                      6

  char name[INTERFACE_NAMSIZ + 1];

  u_int32_t gifindex;

  u_char flags;

  struct pal_in4_addr r_nodeid;

  u_char lmp_inuse;

  u_int32_t ccupcntrs;
  /* control channel in use gifindex */
  u_int32_t ccgifindex;
  struct control_adj_use_cc ucc;
};
#endif /* HAVE_GMPLS */

#ifdef HAVE_LMP
struct nsm_msg_dlink_opaque
{
  cindex_t cindex;
  char name[INTERFACE_NAMSIZ + 1];
  u_int32_t gifindex;

  u_int16_t flags;
#define NSM_MSG_FLAGS_OPAQUE             (1 << 0)
#define NSM_MSG_FLAGS_OPAQUE_FAILURE     (1 << 1)
};

struct nsm_msg_dlink_testmsg
{
  cindex_t cindex;
  char name[INTERFACE_NAMSIZ + 1];
  u_int32_t gifindex;

  u_char *buf;
  u_int16_t msg_len;
};

struct nsm_msg_dlink_chmonitor
{
  cindex_t cindex;
#define NSM_MSG_CINDEX_TELINK_NAME               1
#define NSM_MSG_CINDEX_DLINK_GIFINDEX            2

  char tlname [INTERFACE_NAMSIZ + 1];
  u_int32_t tlgifindex;

  char dlname [INTERFACE_NAMSIZ + 1];
  u_int32_t dlgifindex;

  u_int16_t flags;
#define NSM_MSG_FLAGS_CHMONITOR_ENABLE         (1 << 0)
#define NSM_MSG_FLAGS_CHMONITOR_ENABLE_FAILED  (1 << 1)
};

#ifdef HAVE_RESTART
/* NSM message structure for LMP graceful restart */
struct nsm_msg_lmp_protocol_restart
{
  cindex_t cindex;
#define LMP_SESSION_CTYPE_RESTART_TIME 0

  /* Protocol ID. */
  u_int32_t protocol_id;

  /* Disconnect time. */
  u_int32_t restart_time;
};
#endif /* HAVE_RESTART */
#endif /* HAVE_LMP */

#ifdef HAVE_ONMD
int
nsm_encode_cfm_mac_msg (u_char **pnt,
                        u_int16_t *size,
                        struct nsm_msg_cfm_mac *msg);

int
nsm_decode_cfm_mac_msg (u_char **pnt,
                        u_int16_t *size,
                        struct nsm_msg_cfm_mac *msg);

int
nsm_decode_cfm_index (u_char **pnt,
                      u_int16_t *size,
                      struct nsm_msg_cfm_ifindex *msg);

int
nsm_encode_cfm_index (u_char **pnt,
                      u_int16_t *size,
                      struct nsm_msg_cfm_ifindex *msg);
#endif /* HAVE_ONMD */

struct nsm_msg_server_status 
{
  u_char nsm_status;
#define NSM_MSG_NSM_SERVER_SHUTDOWN (1<<0)
};

/* NSM COMMSG - a generic - nsm_client and nsm_server agnostic message
   utility
   The parser will take us to the COMMSG utility where the message
   will be decoded and dispatched further to the application.
*/

/*-------------------------------------------------------------------
 *  nsm_msg_commsg_recv_cb_t - Implemented by the COMMSG utility.
 *                             Invoked by the nsm_server or nsm_client.
 *  tp_ref      - Message transport interface reference (nsm_server
 *                or nsm_client)
 *  src_mod_id  - Message source - module id.
 *  buf         - Points to the COMMSG header.
 *  bufsz       - Message length.
 *-------------------------------------------------------------------
 */
typedef void (* nsm_msg_commsg_recv_cb_t)(void      *user_ref,
                                          u_int16_t  src_mod_id,
                                          u_char    *buf,
                                          u_int16_t  len);
/* end of NSM COMMSG specific stuff ... */


/* NSM callback function typedef.  */
typedef int (*NSM_CALLBACK) (struct nsm_msg_header *, void *, void *);
typedef int (*NSM_DISCONNECT_CALLBACK) ();

typedef int (*NSM_PARSER) (u_char **, u_int16_t *, struct nsm_msg_header *,
                           void *, NSM_CALLBACK);

const char *nsm_service_to_str (int);
const char *nsm_msg_to_str (int);

int nsm_encode_header (u_char **, u_int16_t *, struct nsm_msg_header *);
int nsm_encode_service (u_char **, u_int16_t *, struct nsm_msg_service *);
int nsm_encode_link (u_char **, u_int16_t *, struct nsm_msg_link *);
int nsm_encode_address (u_char **, u_int16_t *, struct nsm_msg_address *);
int nsm_encode_redistribute (u_char **, u_int16_t *,
                             struct nsm_msg_redistribute *);
int nsm_encode_route_ipv4 (u_char **, u_int16_t *,
                           struct nsm_msg_route_ipv4 *);
int nsm_decode_route_ipv4 (u_char **, u_int16_t *,
                           struct nsm_msg_route_ipv4 *);
void nsm_finish_route_ipv4 (struct nsm_msg_route_ipv4 *);

int nsm_encode_ipv4_route_lookup (u_char **, u_int16_t *,
                                  struct nsm_msg_route_lookup_ipv4 *);
int nsm_decode_ipv4_route_lookup (u_char **, u_int16_t *,
                                  struct nsm_msg_route_lookup_ipv4 *);

/* NSM message functions for wait for bgp */

int nsm_parse_bgp_conv (u_char **, u_int16_t *, struct nsm_msg_header *,
                        void *, NSM_CALLBACK callback);
int nsm_parse_bgp_conv_done (u_char **, u_int16_t *, struct nsm_msg_header *,
                             void *, NSM_CALLBACK callback);
int nsm_parse_bgp_up_down (u_char **, u_int16_t *, struct nsm_msg_header *,
                           void *, NSM_CALLBACK callback);
int nsm_encode_bgp_conv (u_char **, u_int16_t *,
                         struct nsm_msg_wait_for_bgp *);
int nsm_decode_bgp_conv (u_char **, u_int16_t *,
                         struct nsm_msg_wait_for_bgp *);

/* NSM message functions for wait for ldp */

int nsm_parse_ldp_session_state (u_char **, u_int16_t *, struct nsm_msg_header *,
                                 void *, NSM_CALLBACK callback);
int nsm_parse_ldp_up_down (u_char **, u_int16_t *, struct nsm_msg_header *,
                           void *, NSM_CALLBACK callback);
int nsm_encode_ldp_session_state (u_char **, u_int16_t *,
                                  struct nsm_msg_ldp_session_state *);
int nsm_decode_ldp_session_state (u_char **, u_int16_t *,
                                  struct nsm_msg_ldp_session_state *);

#ifdef HAVE_IPV6
int nsm_encode_route_ipv6 (u_char **, u_int16_t *,
                           struct nsm_msg_route_ipv6 *);
int nsm_decode_route_ipv6 (u_char **, u_int16_t *,
                           struct nsm_msg_route_ipv6 *);
void nsm_finish_route_ipv6 (struct nsm_msg_route_ipv6 *);

int nsm_encode_ipv6_route_lookup (u_char **, u_int16_t *,
                                  struct nsm_msg_route_lookup_ipv6 *);
int nsm_decode_ipv6_route_lookup (u_char **, u_int16_t *,
                                  struct nsm_msg_route_lookup_ipv6 *);
#endif /* HAVE_IPV6 */
int nsm_encode_vr (u_char **, u_int16_t *, struct nsm_msg_vr *);
int nsm_decode_vr (u_char **, u_int16_t *, struct nsm_msg_vr *);
int nsm_encode_vrf (u_char **, u_int16_t *, struct nsm_msg_vrf *);
int nsm_decode_vrf (u_char **, u_int16_t *, struct nsm_msg_vrf *);
int nsm_encode_vr_bind (u_char **, u_int16_t *, struct nsm_msg_vr_bind *);
int nsm_decode_vr_bind (u_char **, u_int16_t *, struct nsm_msg_vr_bind *);
int nsm_encode_vrf_bind (u_char **, u_int16_t *, struct nsm_msg_vrf_bind *);
int nsm_decode_vrf_bind (u_char **, u_int16_t *, struct nsm_msg_vrf_bind *);

int nsm_encode_route_manage (u_char **, u_int16_t *,
                             struct nsm_msg_route_manage *);
int nsm_decode_route_manage (u_char **, u_int16_t *,
                             struct nsm_msg_route_manage *);

int nsm_decode_header (u_char **, u_int16_t *, struct nsm_msg_header *);
#ifdef HAVE_MPLS
int nsm_encode_label_pool (u_char **, u_int16_t *,
                           struct nsm_msg_label_pool *);
int nsm_decode_label_pool (u_char **, u_int16_t *,
                           struct nsm_msg_label_pool *);
#ifdef HAVE_PACKET
int nsm_encode_ilm_lookup (u_char **, u_int16_t *,
                           struct nsm_msg_ilm_lookup *);
int nsm_decode_ilm_lookup (u_char **, u_int16_t *,
                           struct nsm_msg_ilm_lookup *);
#endif /* HAVE_PACKET */

int nsm_encode_ilm_gen_lookup (u_char **, u_int16_t *,
                               struct nsm_msg_ilm_gen_lookup *);
int nsm_decode_ilm_gen_lookup (u_char **, u_int16_t *,
                               struct nsm_msg_ilm_gen_lookup *);


int nsm_encode_igp_shortcut_lsp (u_char **, u_int16_t *,
                                 struct nsm_msg_igp_shortcut_lsp *);
int nsm_decode_igp_shortcut_lsp (u_char **, u_int16_t *,
                                 struct nsm_msg_igp_shortcut_lsp *);
int nsm_encode_igp_shortcut_route (u_char **, u_int16_t *,
                                   struct nsm_msg_igp_shortcut_route *);
int nsm_decode_igp_shortcut_route (u_char **, u_int16_t *,
                                   struct nsm_msg_igp_shortcut_route *);
#ifdef HAVE_RESTART
int nsm_encode_stale_bundle_msg (u_char **, u_int16_t *,
                                 struct nsm_msg_stale_info *);
int nsm_decode_stale_bundle_msg (u_char **, u_int16_t *,
                                 struct nsm_msg_stale_info *);
int nsm_parse_stale_bundle_msg (u_char **, u_int16_t *,
                                struct nsm_msg_header *,
                                void *, NSM_CALLBACK callback);
void nsm_mpls_stale_info_dump (struct lib_globals *,
                               struct nsm_msg_stale_info *);

int nsm_encode_gen_stale_bundle_msg (u_char **, u_int16_t *,
                                     struct nsm_gen_msg_stale_info *);
int nsm_decode_gen_stale_bundle_msg (u_char **, u_int16_t *,
                                     struct nsm_gen_msg_stale_info *);
int nsm_parse_gen_stale_bundle_msg (u_char **, u_int16_t *,
                                    struct nsm_msg_header *,
                                    void *, NSM_CALLBACK callback);
void nsm_gmpls_stale_info_dump (struct lib_globals *,
                                struct nsm_gen_msg_stale_info *);
#endif /* HAVE_RESTART */
#endif /* HAVE_MPLS */

#ifdef HAVE_TE
int nsm_encode_qos_client_init (u_char **, u_int16_t *, u_int32_t *);
int nsm_decode_qos_client_init (u_char **, u_int16_t *, u_int32_t *);
void qos_prot_nsm_message_map (struct qos_resource *, struct nsm_msg_qos *);
void qos_nsm_prot_message_map (struct nsm_msg_qos *, struct qos_resource *);
int nsm_encode_t_spec (u_char **, u_int16_t *, struct nsm_msg_qos_t_spec *);
int nsm_decode_t_spec (u_char **, u_int16_t *, struct nsm_msg_qos_t_spec *);
int nsm_encode_if_spec (u_char **, u_int16_t *, struct nsm_msg_qos_if_spec *);
int nsm_decode_if_spec (u_char **, u_int16_t *, struct nsm_msg_qos_if_spec *);
int nsm_encode_ad_spec (u_char **, u_int16_t *, struct nsm_msg_qos_ad_spec *);
int nsm_decode_ad_spec (u_char **, u_int16_t *, struct nsm_msg_qos_ad_spec *);
int nsm_encode_qos (u_char **, u_int16_t *, struct nsm_msg_qos *);
int nsm_decode_qos (u_char **, u_int16_t *, struct nsm_msg_qos *);
int nsm_encode_qos_preempt (u_char **, u_int16_t *,
                            struct nsm_msg_qos_preempt *);
int nsm_decode_qos_preempt (u_char **, u_int16_t *,
                            struct nsm_msg_qos_preempt *);
int nsm_encode_qos_release (u_char **, u_int16_t *,
                            struct nsm_msg_qos_release *);
int nsm_decode_qos_release (u_char **, u_int16_t *,
                            struct nsm_msg_qos_release *);
int nsm_encode_qos_clean (u_char **, u_int16_t *, struct nsm_msg_qos_clean *);
int nsm_decode_qos_clean (u_char **, u_int16_t *, struct nsm_msg_qos_clean *);

int nsm_encode_admin_group (u_char **, u_int16_t *,
                            struct nsm_msg_admin_group *);
#ifdef HAVE_GMPLS
int nsm_encode_gmpls_qos_client_init (u_char **, u_int16_t *, u_int32_t *);
int nsm_decode_gmpls_qos_client_init (u_char **, u_int16_t *, u_int32_t *);
void qos_prot_nsm_message_map_gmpls (struct qos_resource *, struct nsm_msg_gmpls_qos *);
void qos_nsm_prot_message_map_gmpls (struct nsm_msg_gmpls_qos *, struct qos_resource *);
int nsm_encode_gmpls_qos (u_char **, u_int16_t *, struct nsm_msg_gmpls_qos *);
int nsm_decode_gmpls_qos (u_char **, u_int16_t *, struct nsm_msg_gmpls_qos *);
int nsm_encode_gmpls_qos_release (u_char **, u_int16_t *,
                                  struct nsm_msg_gmpls_qos_release *);
int nsm_decode_gmpls_qos_release (u_char **, u_int16_t *,
                                  struct nsm_msg_gmpls_qos_release *);
int nsm_encode_gmpls_qos_clean (u_char **, u_int16_t *, struct nsm_msg_gmpls_qos_clean *);
int nsm_decode_gmpls_qos_clean (u_char **, u_int16_t *, struct nsm_msg_gmpls_qos_clean *);

#endif /*HAVE_GMPLS*/
#endif /* HAVE_TE */

int nsm_encode_router_id (u_char **, u_int16_t *, struct nsm_msg_router_id *);
int nsm_decode_router_id (u_char **, u_int16_t *, struct nsm_msg_router_id *);
/* TBD old code
int nsm_encode_gmpls_if (u_char **, u_int16_t *, struct nsm_msg_gmpls_if *);
int nsm_decode_gmpls_if (u_char **, u_int16_t *, struct nsm_msg_gmpls_if *); */
#ifdef HAVE_CRX
void nsm_route_clean_dump (struct lib_globals *, struct nsm_msg_route_clean *);
int nsm_encode_route_clean (u_char **, u_int16_t *,
                            struct nsm_msg_route_clean *);
int nsm_decode_route_clean (u_char **, u_int16_t *,
                            struct nsm_msg_route_clean *);
int nsm_parse_route_clean (u_char **, u_int16_t *, struct nsm_msg_header *,
                           void *, NSM_CALLBACK callback);

void nsm_protocol_restart_dump (struct lib_globals *,
                                struct nsm_msg_protocol_restart *);
int nsm_encode_protocol_restart (u_char **, u_int16_t *,
                                 struct nsm_msg_protocol_restart *);
int nsm_decode_protocol_restart (u_char **, u_int16_t *,
                                 struct nsm_msg_protocol_restart *);
int nsm_parse_protocol_restart (u_char **, u_int16_t *,
                                struct nsm_msg_header *, void *,
                                NSM_CALLBACK callback);
int nsm_parse_route_clean (u_char **, u_int16_t *, struct nsm_msg_header *,
                           void *, NSM_CALLBACK callback);
#endif /* HAVE_CRX */

int nsm_parse_service (u_char **, u_int16_t *, struct nsm_msg_header *,
                       void *, NSM_CALLBACK);
int
nsm_parse_vr_done (u_char **pnt, u_int16_t *size,
                    struct nsm_msg_header *header, void *arg,
                    NSM_CALLBACK callback);
int nsm_parse_link (u_char **, u_int16_t *, struct nsm_msg_header *,
                    void *, NSM_CALLBACK);
int nsm_parse_vr (u_char **, u_int16_t *, struct nsm_msg_header *,
                  void *, NSM_CALLBACK);
int nsm_parse_vr_bind (u_char **, u_int16_t *, struct nsm_msg_header *,
                       void *, NSM_CALLBACK);
int nsm_parse_vrf_bind (u_char **, u_int16_t *, struct nsm_msg_header *,
                        void *, NSM_CALLBACK);
int nsm_parse_address (u_char **, u_int16_t *, struct nsm_msg_header *,
                       void *, NSM_CALLBACK);
int nsm_parse_route_ipv4 (u_char **, u_int16_t *, struct nsm_msg_header *,
                          void *, NSM_CALLBACK);
int nsm_parse_redistribute (u_char **, u_int16_t *, struct nsm_msg_header *,
                            void *, NSM_CALLBACK);
int nsm_parse_ipv4_route_lookup (u_char **, u_int16_t *,
                                 struct nsm_msg_header *, void *,
                                 NSM_CALLBACK);
int nsm_parse_route_manage (u_char **, u_int16_t *, struct nsm_msg_header *,
                            void *, NSM_CALLBACK);
int nsm_parse_vrf (u_char **, u_int16_t *, struct nsm_msg_header *, void *,
                   NSM_CALLBACK);
int nsm_parse_nsm_server_status_notify (u_char **, u_int16_t *, 
                                        struct nsm_msg_header *, void *, NSM_CALLBACK);
int nsm_encode_server_status_msg (u_char **, u_int16_t *,
                                  struct nsm_msg_server_status *);
int
nsm_decode_server_status_msg (u_char **, u_int16_t *, 
                              struct nsm_msg_server_status *);
#ifndef HAVE_IMI
int nsm_parse_user (u_char **, u_int16_t *, struct nsm_msg_header *, void *,
                    NSM_CALLBACK);
int nsm_encode_user (u_char **, u_int16_t *, struct nsm_msg_user *);
#endif /* HAVE_IMI */

#ifdef HAVE_MPLS
int nsm_parse_label_pool (u_char **, u_int16_t *, struct nsm_msg_header *,
                          void *, NSM_CALLBACK);
#ifdef HAVE_PACKET
int nsm_parse_ilm_lookup (u_char **, u_int16_t *, struct nsm_msg_header *,
                          void *, NSM_CALLBACK);
#endif /* HAVE_PACKET */
int nsm_parse_ilm_gen_lookup (u_char **, u_int16_t *, struct nsm_msg_header *,
                              void *, NSM_CALLBACK);
#endif /* HAVE_MPLS */

#ifdef HAVE_TE
int nsm_parse_qos_client_init (u_char **, u_int16_t *, struct nsm_msg_header *,
                               void *, NSM_CALLBACK);
int nsm_parse_qos_preempt (u_char **, u_int16_t *, struct nsm_msg_header *,
                           void *, NSM_CALLBACK);

int nsm_parse_qos (u_char **, u_int16_t *, struct nsm_msg_header *, void *,
                   NSM_CALLBACK);
int nsm_parse_qos_release (u_char **, u_int16_t *, struct nsm_msg_header *,
                           void *, NSM_CALLBACK);
int nsm_parse_qos_clean (u_char **, u_int16_t *, struct nsm_msg_header *,
                         void *, NSM_CALLBACK);
int nsm_parse_admin_group (u_char **, u_int16_t *, struct nsm_msg_header *,
                           void *, NSM_CALLBACK);
int nsm_parse_gmpls_if (u_char **, u_int16_t *, struct nsm_msg_header *,
                        void *, NSM_CALLBACK);
#endif /* HAVE_TE */

#ifdef HAVE_GMPLS
int nsm_parse_gmpls_qos_client_init (u_char **, u_int16_t *, struct nsm_msg_header *,
                                     void *, NSM_CALLBACK);
int nsm_parse_gmpls_qos_preempt (u_char **, u_int16_t *, struct nsm_msg_header *,
                                 void *, NSM_CALLBACK);

int nsm_parse_gmpls_qos (u_char **, u_int16_t *, struct nsm_msg_header *, void *,
                         NSM_CALLBACK);
int nsm_parse_gmpls_qos_release (u_char **, u_int16_t *, struct nsm_msg_header *,
                                 void *, NSM_CALLBACK);
int nsm_parse_gmpls_qos_clean (u_char **, u_int16_t *, struct nsm_msg_header *,
                               void *, NSM_CALLBACK);
#endif /*HAVE_GMPLS*/

#ifdef HAVE_GMPLS
s_int32_t nsm_parse_control_channel (u_char **, u_int16_t *, struct nsm_msg_header *,
                                     void *, NSM_CALLBACK);
s_int32_t nsm_parse_control_adj (u_char **, u_int16_t *, struct nsm_msg_header *,
                                 void *, NSM_CALLBACK);
s_int32_t nsm_parse_data_link (u_char **, u_int16_t *, struct nsm_msg_header *,
                               void *, NSM_CALLBACK);
s_int32_t nsm_parse_data_link_sub (u_char **, u_int16_t *, struct nsm_msg_header *,
                                   void *, NSM_CALLBACK);
s_int32_t nsm_parse_te_link (u_char **, u_int16_t *, struct nsm_msg_header *,
                             void *, NSM_CALLBACK);
s_int32_t nsm_util_control_channel_callback (struct nsm_msg_header *, void *, void *);
s_int32_t nsm_util_control_adj_callback (struct nsm_msg_header *, void *, void *);
s_int32_t nsm_util_data_link_callback (struct nsm_msg_header *, void *, void *);
s_int32_t nsm_util_data_link_sub_callback (struct nsm_msg_header *, void *, void *);
s_int32_t nsm_util_te_link_callback (struct nsm_msg_header *, void *, void *);
int nsm_encode_generic_label_pool (u_char **, u_int16_t *, struct nsm_msg_generic_label_pool *);
int nsm_decode_generic_label_pool (u_char **, u_int16_t *, struct nsm_msg_generic_label_pool *);
s_int32_t nsm_parse_generic_label_pool (u_char **, u_int16_t *, struct nsm_msg_header*,
                                  void*, NSM_CALLBACK );
#endif /*HAVE_GMPLS*/
#ifdef HAVE_L2
int nsm_encode_bridge_msg (u_char **pnt, u_int16_t *size,
                           struct nsm_msg_bridge *msg);
int nsm_decode_bridge_msg (u_char **pnt, u_int16_t *size,
                           struct nsm_msg_bridge *msg);
int nsm_encode_bridge_if_msg (u_char **pnt, u_int16_t *size,
                              struct nsm_msg_bridge_if *msg);
int nsm_decode_bridge_if_msg (u_char **pnt, u_int16_t *size,
                              struct nsm_msg_bridge_if *msg);
int nsm_encode_bridge_port_msg (u_char **pnt, u_int16_t *size,
                              struct nsm_msg_bridge_port *msg);
int nsm_decode_bridge_port_msg (u_char **pnt, u_int16_t *size,
                              struct nsm_msg_bridge_port *msg);
int nsm_encode_bridge_enable_msg (u_char **pnt, u_int16_t *size,
                                  struct nsm_msg_bridge_enable *msg);
int nsm_decode_bridge_enable_msg (u_char **pnt, u_int16_t *size,
                                  struct nsm_msg_bridge_enable *msg);
int nsm_parse_bridge_msg (u_char **pnt, u_int16_t *size,
                          struct nsm_msg_header *header, void *arg,
                          NSM_CALLBACK callback);
int nsm_parse_bridge_if_msg (u_char **pnt, u_int16_t *size,
                             struct nsm_msg_header *header, void *arg,
                             NSM_CALLBACK callback);
int nsm_parse_bridge_port_msg (u_char **pnt, u_int16_t *size,
                               struct nsm_msg_header *header, void *arg,
                               NSM_CALLBACK callback);
int nsm_parse_bridge_enable_msg (u_char **pnt, u_int16_t *size,
                                 struct nsm_msg_header *header, void *arg,
                                 NSM_CALLBACK callback);
#if (defined HAVE_I_BEB || defined HAVE_B_BEB )
int
nsm_parse_isid2svid_msg (u_char **pnt, u_int16_t *size,
    struct nsm_msg_header *header, void *arg,
    NSM_CALLBACK callback);

int
nsm_parse_isid2bvid_msg (u_char **pnt, u_int16_t *size,
    struct nsm_msg_header *header, void *arg,
    NSM_CALLBACK callback);

#endif

#ifdef HAVE_PBB_TE
int
nsm_parse_te_vlan_msg (u_char **pnt, u_int16_t *size,
    struct nsm_msg_header *header, void *arg,
    NSM_CALLBACK callback);

int
nsm_parse_te_esp_msg (u_char **pnt, u_int16_t *size,
    struct nsm_msg_header *header, void *arg,
    NSM_CALLBACK callback);

int
nsm_parse_esp_pnp_msg (u_char **pnt, u_int16_t *size,
    struct nsm_msg_header *header, void *arg,
    NSM_CALLBACK callback);

int
nsm_parse_bridge_pbb_te_port_state_msg (u_char **pnt, u_int16_t *size,
                                        struct nsm_msg_header *header, void *arg,
                                        NSM_CALLBACK callback);

int
nsm_parse_te_aps_grp(u_char **pnt, u_int16_t *size,
                     struct nsm_msg_header *header, void *arg,
                     NSM_CALLBACK callback);

#endif /* HAVE_PBB_TE */

#ifdef HAVE_PVLAN
int
nsm_encode_pvlan_if_msg (u_char **pnt, u_int16_t *size,
                          struct nsm_msg_pvlan_if *msg);
int
nsm_encode_pvlan_configure (u_char **pnt, u_int16_t *size,
                            struct nsm_msg_pvlan_configure *msg);
int
nsm_decode_pvlan_configure (u_char **pnt, u_int16_t *size,
                            struct nsm_msg_pvlan_configure *msg);
int
nsm_encode_pvlan_associate (u_char **pnt, u_int16_t *size,
                            struct nsm_msg_pvlan_association *msg);
int
nsm_decode_pvlan_associate (u_char **pnt, u_int16_t *size,
                            struct nsm_msg_pvlan_association *msg);

#endif /* HAVE_PVLAN */

int
nsm_parse_static_agg_cnt_update (u_char **pnt, u_int16_t *size,
                          struct nsm_msg_header *header, void *arg,
                          NSM_CALLBACK callback);
int
nsm_parse_lacp_send_agg_config (u_char **pnt, u_int16_t *size,
                          struct nsm_msg_header *header, void *arg,
                          NSM_CALLBACK callback);
#endif /* HAVE_L2 */

#if (defined HAVE_I_BEB || defined HAVE_B_BEB )
int
nsm_encode_isid2svid_msg ( u_char **pnt, u_int16_t *size,
    struct nsm_msg_pbb_isid_to_pip * msg);
int
nsm_decode_isid2svid_msg (u_char **pnt, u_int16_t *size,
    struct nsm_msg_pbb_isid_to_pip *msg);
int
nsm_encode_isid2bvid_msg ( u_char **pnt, u_int16_t *size,
        struct nsm_msg_pbb_isid_to_bvid * msg);

#endif /* HAVE_I_BEB || HAVE_B_BEB */

#if defined HAVE_G8031 || defined HAVE_G8032
int
nsm_parse_bridge_pg_msg (u_char **pnt, u_int16_t *size,
                         struct nsm_msg_header *header, void *arg,
                         NSM_CALLBACK callback);

int
nsm_encode_bridge_pg_msg (u_char **pnt, u_int16_t *size,
                          struct nsm_msg_bridge_pg *msg);


int
nsm_decode_bridge_pg_msg (u_char **pnt, u_int16_t *size,
                          struct nsm_msg_bridge_pg *msg);
#endif /*HAVE_G8031 || HAVE_G8032 */

#ifdef HAVE_G8031
int
nsm_parse_g8031_create_pg_msg (u_char **pnt, u_int16_t *size,
                               struct nsm_msg_header *header, void *arg,
                               NSM_CALLBACK callback);

int
nsm_parse_g8031_create_vlan_msg (u_char **pnt, u_int16_t *size,
                                 struct nsm_msg_header *header, void *arg,
                                 NSM_CALLBACK callback);

int
nsm_parse_oam_pg_init_msg (u_char **pnt, u_int16_t *size,
                       struct nsm_msg_header *header, void *arg,
                       NSM_CALLBACK callback);

int
nsm_parse_oam_g8031_portstate_msg (u_char **pnt, u_int16_t *size,
                                   struct nsm_msg_header *header, void *arg,
                                   NSM_CALLBACK callback);

int
nsm_encode_g8031_create_pg_msg (u_char **pnt, u_int16_t *size,
                                 struct nsm_msg_g8031_pg * msg);

int
nsm_encode_g8031_create_vlan_msg (u_char **pnt, u_int16_t *size,
                                   struct nsm_msg_g8031_vlanpg * msg);

int
nsm_decode_g8031_create_pg_msg (u_char **pnt, u_int16_t *size,
                                struct nsm_msg_g8031_pg *msg);

int
nsm_decode_g8031_create_vlan_msg (u_char **pnt, u_int16_t *size,
                                  struct nsm_msg_g8031_vlanpg *msg);

int
nsm_encode_pg_init_msg (u_char **pnt, u_int16_t *size,
                        struct nsm_msg_pg_initialized *msg);

int
nsm_decode_pg_init_msg (u_char **pnt, u_int16_t *size,
                        struct nsm_msg_pg_initialized * msg);

int
nsm_encode_g8031_portstate_msg (u_char **pnt, u_int16_t *size,
                                struct nsm_msg_g8031_portstate *msg);

int
nsm_decode_g8031_portstate_msg (u_char **pnt, u_int16_t *size,
                                struct nsm_msg_g8031_portstate *msg);

#endif /* HAVE_G8031 */

#ifdef HAVE_PBB_TE
int
nsm_encode_te_vlan_msg (u_char **pnt, u_int16_t *size,
                        struct nsm_msg_pbb_te_vlan *msg);
int
nsm_decode_te_vlan_msg (u_char **pnt, u_int16_t *size,
                        struct nsm_msg_pbb_te_vlan *msg);
int
nsm_encode_te_esp_msg (u_char **pnt, u_int16_t *size,
                       struct nsm_msg_pbb_te_esp *msg);
int
nsm_decode_te_esp_msg (u_char **pnt, u_int16_t *size,
                       struct nsm_msg_pbb_te_esp *msg);
int
nsm_encode_esp_pnp_msg (u_char **pnt, u_int16_t *size,
                       struct nsm_msg_pbb_esp_pnp *msg);
int
nsm_decode_esp_pnp_msg (u_char **pnt, u_int16_t *size,
                       struct nsm_msg_pbb_esp_pnp *msg);

int
nsm_decode_esp_pnp_msg (u_char **pnt, u_int16_t *size,
                       struct nsm_msg_pbb_esp_pnp *msg);

int
nsm_encode_te_aps_grp (u_char **pnt, u_int16_t *size,
                       struct nsm_msg_te_aps_grp *msg);

int
nsm_encode_bridge_pbb_te_port_msg (u_char **pnt, u_int16_t *size,
                          struct nsm_msg_bridge_pbb_te_port *msg);

int
nsm_decode_bridge_pbb_te_port_msg (u_char **pnt, u_int16_t *size,
                           struct nsm_msg_bridge_pbb_te_port *msg);

#endif /* HAVE_PBB_TE */

#ifndef HAVE_CUSTOM1
#ifdef HAVE_VLAN
int nsm_encode_vlan_msg (u_char **pnt, u_int16_t *size,
                         struct nsm_msg_vlan *msg);
int nsm_decode_vlan_msg (u_char **pnt, u_int16_t *size,
                         struct nsm_msg_vlan *msg);
int nsm_encode_vlan_port_msg (u_char **pnt, u_int16_t *size,
                            struct nsm_msg_vlan_port *msg);
int nsm_decode_vlan_port_msg (u_char **pnt, u_int16_t *size, struct nsm_msg_vlan_port *msg);
int nsm_parse_vlan_msg (u_char **pnt, u_int16_t *size,
                        struct nsm_msg_header *header, void *arg,
                        NSM_CALLBACK callback);
int nsm_parse_vlan_port_msg (u_char **pnt, u_int16_t *size,
                           struct nsm_msg_header *header, void *arg,
                           NSM_CALLBACK callback);
int
nsm_decode_vlan_port_type_msg (u_char **pnt, u_int16_t *size,
                               struct nsm_msg_vlan_port_type *msg);
int
nsm_encode_vlan_port_type_msg (u_char **pnt, u_int16_t *size,
                               struct nsm_msg_vlan_port_type *msg);
int
nsm_parse_vlan_port_type_msg (u_char **pnt, u_int16_t *size,
                              struct nsm_msg_header *header, void *arg,
                              NSM_CALLBACK callback);
void nsm_bridge_dump (struct lib_globals *zg, struct nsm_msg_bridge *msg);
void nsm_bridge_enable_dump (struct lib_globals *zg,
                             struct nsm_msg_bridge_enable *msg);
void nsm_bridge_port_dump (struct lib_globals *zg,
                           struct nsm_msg_bridge_port *msg);
void nsm_vlan_dump (struct lib_globals *zg, struct nsm_msg_vlan *msg);
void nsm_vlan_port_dump (struct lib_globals *zg, struct nsm_msg_vlan_port *msg);
void nsm_vlan_port_type_dump (struct lib_globals *zg, struct nsm_msg_vlan_port_type *msg);

#ifdef HAVE_PVLAN
int
nsm_parse_pvlan_if_msg (u_char **pnt, u_int16_t *size,
                         struct nsm_msg_header *header, void *arg,
                         NSM_CALLBACK callback);
int
nsm_parse_pvlan_configure (u_char **pnt, u_int16_t *size,
                           struct nsm_msg_header *header, void *arg,
                           NSM_CALLBACK callback);
int
nsm_parse_pvlan_associate (u_char **pnt, u_int16_t *size,
                           struct nsm_msg_header *header, void *arg,
                           NSM_CALLBACK callback);
#endif /* HAVE_PVLAN */



#ifdef HAVE_VLAN_CLASS
/* VLAN classifier message dump */
void nsm_vlan_classifier_dump (struct lib_globals *zg,
                               struct nsm_msg_vlan_classifier *msg);

/* VLAN port classifier message dump. */
void nsm_vlan_port_class_dump (struct lib_globals *zg,
                               struct nsm_msg_vlan_port_classifier *msg);

/* Encode Vlan Classifier message. */
int nsm_encode_vlan_classifier_msg (u_char **pnt, u_int16_t *size,
                                    struct nsm_msg_vlan_classifier *msg);

/* Decode VLAN Classifier message. */
int nsm_decode_vlan_classifier_msg (u_char **pnt, u_int16_t *size,
                                    struct nsm_msg_vlan_classifier *msg);

/* Encode VLAN port classifier message. */
int nsm_encode_vlan_port_class_msg (u_char **pnt, u_int16_t *size,
                               struct nsm_msg_vlan_port_classifier *msg);

/* Decode VLAN port classifier message. */
int nsm_decode_vlan_port_class_msg (u_char **pnt, u_int16_t *size,
                               struct nsm_msg_vlan_port_classifier *msg);

/* Parse VLAN classifier message. */
int nsm_parse_vlan_classifier_msg (u_char **pnt, u_int16_t *size,
                                   struct nsm_msg_header *header,
                                   void *arg, NSM_CALLBACK callback);

/* Parse VLAN port classifier message. */
int nsm_parse_vlan_port_class_msg (u_char **pnt, u_int16_t *size,
                                   struct nsm_msg_header *header,
                                   void *arg, NSM_CALLBACK callback);
#endif /* HAVE_VLAN_CLASS */

#endif /* HAVE_VLAN */

#ifdef HAVE_G8032
int
nsm_parse_g8032_create_msg (u_char **pnt, u_int16_t *size,
                            struct nsm_msg_header *header, void *arg,
                            NSM_CALLBACK callback);
int
nsm_parse_g8032_delete_msg (u_char **pnt, u_int16_t *size,
                            struct nsm_msg_header *header, void *arg,
                            NSM_CALLBACK callback);

int
nsm_encode_g8032_create_msg (u_char **pnt, u_int16_t *size,
                             struct nsm_msg_g8032 * msg);
int
nsm_decode_g8032_create_msg (u_char **pnt, u_int16_t *size,
                             struct nsm_msg_g8032 * msg);

int
nsm_parse_bridge_g8032_port_msg (u_char **pnt, u_int16_t *size,
                                 struct nsm_msg_header *header, void *arg,
                                 NSM_CALLBACK callback);

/* Message for decoding the Port State message of G8032. */
int
nsm_decode_g8032_portstate_msg (u_char **pnt, u_int16_t *size,
                                struct nsm_msg_bridge_g8032_port *msg);

/* Message for encoding the Port State message of G8032. */
int
nsm_encode_g8032_port_msg (u_char **pnt, u_int16_t *size,
                           struct nsm_msg_bridge_g8032_port *msg);

int
nsm_parse_oam_g8032_portstate_msg (u_char **pnt, u_int16_t *size,
                                   struct nsm_msg_header *header,
                                   void *arg,
                                   NSM_CALLBACK callback);

int
nsm_parse_g8032_create_vlan_msg (u_char **pnt, u_int16_t *size,
                                 struct nsm_msg_header *header,
                                 void *arg,
                                 NSM_CALLBACK callback);
int
nsm_encode_g8032_create_vlan_msg (u_char **pnt, u_int16_t *size,
	                           struct nsm_msg_g8032_vlan * msg);


#endif /*HAVE_G8032*/

#endif /* HAVE_CUSTOM1 */

#ifdef HAVE_AUTHD
/* Encode the port authentication port state message.  */
int nsm_encode_auth_port_state_msg (u_char **, u_int16_t *,
                                    struct nsm_msg_auth_port_state *);

/* Decode the port authentication port state message.  */
int nsm_decode_auth_port_state_msg (u_char **, u_int16_t *,
                                    struct nsm_msg_auth_port_state *);
#ifdef HAVE_MAC_AUTH
s_int32_t
nsm_encode_auth_mac_port_state_msg (u_char **, u_int16_t *,
                                    struct nsm_msg_auth_mac_port_state *);

s_int32_t
nsm_decode_auth_mac_port_state_msg (u_char **, u_int16_t *,
                                    struct nsm_msg_auth_mac_port_state *);

s_int32_t
nsm_encode_auth_mac_status_msg (u_char **, u_int16_t *,
                                struct nsm_msg_auth_mac_status *);

s_int32_t
nsm_decode_auth_mac_status_msg (u_char **, u_int16_t *,
                                struct nsm_msg_auth_mac_status *);

s_int32_t
nsm_parse_auth_mac_port_state_msg (u_char **, u_int16_t *,
                                    struct nsm_msg_header *, void *,
                                    NSM_CALLBACK);

s_int32_t
nsm_parse_auth_mac_status_msg (u_char **, u_int16_t *,
                               struct nsm_msg_header *, void *,
                               NSM_CALLBACK);
void
nsm_auth_mac_port_state_dump (struct lib_globals *,
                              struct nsm_msg_auth_mac_port_state *);
#endif /* HAVE_MAC_AUTH */

/* Parse the port authentication port state message.  */
int nsm_parse_auth_port_state_msg (u_char **, u_int16_t *,
                                   struct nsm_msg_header *, void *,
                                   NSM_CALLBACK);

/* Dump the port authentication port state message.  */
void nsm_auth_port_state_dump (struct lib_globals *,
                               struct nsm_msg_auth_port_state *);

#endif /* HAVE_AUTHD */

#ifdef HAVE_ONMD

int nsm_encode_efm_if_msg (u_char **pnt, u_int16_t *size,
                           struct nsm_msg_efm_if *msg);

int nsm_decode_efm_if_msg (u_char **pnt, u_int16_t *size,
                           struct nsm_msg_efm_if *msg);

int nsm_parse_oam_efm_msg (u_char **pnt, u_int16_t *size,
                           struct nsm_msg_header *header, void *arg,
                           NSM_CALLBACK callback);

void nsm_efm_port_msg_dump (struct lib_globals *zg,
                            struct nsm_msg_efm_if *msg);

int nsm_encode_lldp_msg (u_char **pnt, u_int16_t *size,
                         struct nsm_msg_lldp *msg);

int nsm_decode_lldp_msg (u_char **pnt, u_int16_t *size,
                        struct nsm_msg_lldp *msg);

int nsm_parse_oam_lldp_msg (u_char **pnt, u_int16_t *size,
                            struct nsm_msg_header *header, void *arg,
                            NSM_CALLBACK callback);

void nsm_lldp_msg_dump (struct lib_globals *zg,
                        struct nsm_msg_lldp *msg);

int
nsm_encode_cfm_msg (u_char **pnt, u_int16_t *size,
                    struct nsm_msg_cfm *msg);

int
nsm_decode_cfm_msg (u_char **pnt, u_int16_t *size,
                    struct nsm_msg_cfm *msg);

int nsm_parse_oam_cfm_msg (u_char **pnt, u_int16_t *size,
                           struct nsm_msg_header *header, void *arg,
                           NSM_CALLBACK callback);

int
nsm_encode_cfm_status_msg (u_char **pnt, u_int16_t *size,
                    struct nsm_msg_cfm_status *msg);

int
nsm_decode_cfm_status_msg (u_char **pnt, u_int16_t *size,
                    struct nsm_msg_cfm_status *msg);

int
nsm_parse_oam_cfm_status_msg (u_char **pnt, u_int16_t *size,
                       struct nsm_msg_header *header, void *arg,
                       NSM_CALLBACK callback);

void nsm_cfm_msg_dump (struct lib_globals *zg,
                       struct nsm_msg_cfm *msg);
#endif
int nsm_parse_router_id (u_char **, u_int16_t *, struct nsm_msg_header *,
                         void *, NSM_CALLBACK);

#ifdef HAVE_MPLS
int nsm_ftn_reply_callback (struct nsm_msg_header *, void *, void *);
int nsm_ilm_reply_callback (struct nsm_msg_header *, void *, void *);
int nsm_ilm_reply_gen_callback (struct nsm_msg_header *, void *, void *);
int nsm_parse_mpls_notification (u_char **, u_int16_t *,
                                 struct nsm_msg_header *, void *,
                                 NSM_CALLBACK);
int nsm_parse_igp_shortcut_lsp (u_char **, u_int16_t *,
                                struct nsm_msg_header *, void *,
                                NSM_CALLBACK);

int nsm_parse_gmpls_ftn_data (u_char **, u_int16_t *, struct nsm_msg_header *,
                              void *, NSM_CALLBACK);

#ifdef HAVE_GMPLS
int nsm_ftn_bidir_reply_callback (struct nsm_msg_header *, void *, void *);
int nsm_ilm_bidir_reply_callback (struct nsm_msg_header *, void *, void *);
#endif /* HAVE_GMPLS */
#ifdef HAVE_PACKET
int nsm_parse_ftn_ipv4 (u_char **, u_int16_t *, struct nsm_msg_header *,
                        void *, NSM_CALLBACK);
int nsm_parse_ilm_ipv4 (u_char **, u_int16_t *, struct nsm_msg_header *,
                        void *, NSM_CALLBACK);
s_int32_t nsm_encode_ftn_bidir_ret_data (u_char **, u_int16_t *,
                                         struct ftn_bidir_ret_data *);
s_int32_t nsm_decode_ilm_bidir_ret_data (u_char **, u_int16_t *,
                                         struct ilm_bidir_ret_data *);


#endif /* HAVE_PACKET */

int nsm_parse_ftn_gen_data (u_char **, u_int16_t *, struct nsm_msg_header *,
                            void *, NSM_CALLBACK);
int nsm_parse_ilm_gen_data (u_char **, u_int16_t *, struct nsm_msg_header *,
                            void *, NSM_CALLBACK);

#ifdef HAVE_GMPLS
int nsm_parse_gmpls_ftn_bidir_data (u_char **, u_int16_t *,
                                    struct nsm_msg_header *,
                                    void *, NSM_CALLBACK);
int nsm_parse_gmpls_ilm_bidir_data (u_char **, u_int16_t *,
                                    struct nsm_msg_header *,
                                    void *, NSM_CALLBACK);
#endif /* HAVE_GMPLS */

#ifdef HAVE_IPV6
int nsm_parse_ftn_ipv6 (u_char **, u_int16_t *, struct nsm_msg_header *,
                        void *, NSM_CALLBACK);
int nsm_parse_ilm_ipv6 (u_char **, u_int16_t *, struct nsm_msg_header *,
                        void *, NSM_CALLBACK);
#endif

int nsm_parse_ftn_bidir_reply (u_char **, u_int16_t *, struct nsm_msg_header *,
                               void *, NSM_CALLBACK);
int nsm_parse_ilm_bidir_reply (u_char **, u_int16_t *, struct nsm_msg_header *,
                               void *, NSM_CALLBACK);
int nsm_parse_ftn_reply (u_char **, u_int16_t *, struct nsm_msg_header *,
                         void *, NSM_CALLBACK);
int nsm_parse_ilm_reply (u_char **, u_int16_t *, struct nsm_msg_header *,
                         void *, NSM_CALLBACK);
int nsm_parse_ilm_gen_reply (u_char **, u_int16_t *, struct nsm_msg_header *,
                             void *, NSM_CALLBACK);
int nsm_parse_igp_shortcut_route (u_char **, u_int16_t *,
                                  struct nsm_msg_header *, void *,
                                  NSM_CALLBACK);
int nsm_encode_mpls_notification (u_char **, u_int16_t *,
                                  struct mpls_notification *);
int nsm_encode_ftn_data_ipv4 (u_char **, u_int16_t *, struct ftn_add_data *);
int nsm_encode_ilm_data_ipv4 (u_char **, u_int16_t *, struct ilm_add_data *);

#ifdef HAVE_IPV6
int nsm_encode_ftn_data_ipv6 (u_char **, u_int16_t *, struct ftn_add_data *);
int nsm_encode_ilm_data_ipv6 (u_char **, u_int16_t *, struct ilm_add_data *);
#endif

s_int32_t nsm_encode_gmpls_ftn_data (u_char **, u_int16_t *, struct ftn_add_gen_data *);
s_int32_t nsm_encode_gmpls_ilm_data (u_char **, u_int16_t *, struct ilm_add_gen_data *);
s_int32_t nsm_encode_gmpls_ftn_bidir_data (u_char **, u_int16_t *, struct ftn_bidir_add_data *);
s_int32_t nsm_encode_gmpls_ilm_bidir_data (u_char **, u_int16_t *, struct ilm_bidir_add_data *);

void nsm_ftn_ip_dump (struct lib_globals *, struct ftn_add_data *);
void nsm_ftn_ip_gen_dump (struct lib_globals *, struct ftn_add_gen_data *);
void nsm_ftn_bidir_ip_gen_dump (struct lib_globals *,
                                struct ftn_bidir_add_data *);
void nsm_ilm_ip_dump (struct lib_globals *, struct ilm_add_data *);
void nsm_ilm_ip_gen_dump (struct lib_globals *, struct ilm_add_gen_data *);
void nsm_ilm_bidir_ip_gen_dump (struct lib_globals *,
                                struct ilm_bidir_add_data *);
void nsm_mpls_owner_dump (struct lib_globals *zg, struct mpls_owner *owner);

void nsm_mpls_notification_dump (struct lib_globals *,
                                 struct mpls_notification *);
void nsm_ftn_ret_data_dump (struct lib_globals *, struct ftn_ret_data *);
void nsm_ilm_ret_data_dump (struct lib_globals *, struct ilm_ret_data *);
void nsm_ilm_gen_ret_data_dump (struct lib_globals *,
                                struct ilm_gen_ret_data *);
int nsm_encode_ftn_ret_data (u_char **, u_int16_t *, struct ftn_ret_data *);

int nsm_encode_ilm_ret_data (u_char **, u_int16_t *, struct ilm_ret_data *);
int nsm_encode_ilm_bidir_ret_data (u_char **, u_int16_t *,
                                   struct ilm_bidir_ret_data *);
int nsm_encode_ilm_gen_ret_data (u_char **, u_int16_t *,
                                 struct ilm_gen_ret_data *);

#ifdef HAVE_BFD
int
nsm_encode_ftn_down_data (u_char **pnt, u_int16_t *size,
                          struct ftn_down_data *fdd);

int
nsm_decode_ftn_down_data (u_char **pnt, u_int16_t *size,
                          struct ftn_down_data *fdd);

int
nsm_parse_ftn_down (u_char **pnt, u_int16_t *size,
                    struct nsm_msg_header *header, void *arg,
                    NSM_CALLBACK callback);
#endif /* HAVE_BFD */

void nsm_igp_shortcut_lsp_dump (struct lib_globals *,
                                struct nsm_msg_igp_shortcut_lsp *);
void nsm_igp_shortcut_route_dump (struct lib_globals *,
                                  struct nsm_msg_igp_shortcut_route *);
#endif /* HAVE_MPLS */


#ifdef HAVE_IPV6
int nsm_parse_route_ipv6 (u_char **, u_int16_t *, struct nsm_msg_header *,
                          void *, NSM_CALLBACK);
int nsm_parse_ipv6_route_lookup (u_char **, u_int16_t *,
                                 struct nsm_msg_header *,  void *,
                                 NSM_CALLBACK);
void nsm_route_ipv6_dump (struct lib_globals *, struct nsm_msg_route_ipv6 *);
#endif /* HAVE_IPV6 */
void nsm_header_dump (struct lib_globals *, struct nsm_msg_header *);
void nsm_service_dump (struct lib_globals *, struct nsm_msg_service *);
void nsm_interface_dump (struct lib_globals *, struct nsm_msg_link *);
void nsm_route_ipv4_dump (struct lib_globals *, struct nsm_msg_route_ipv4 *);
void nsm_redistribute_dump (struct lib_globals *,
                            struct nsm_msg_redistribute *);
void nsm_address_dump (struct lib_globals *, struct nsm_msg_address *);
void nsm_route_lookup_ipv4_dump (struct lib_globals *,
                                 struct nsm_msg_route_lookup_ipv4 *);
#ifdef HAVE_IPV6
void nsm_route_lookup_ipv6_dump (struct lib_globals *,
                                 struct nsm_msg_route_lookup_ipv6 *);
#endif /* HAVE_IPV6 */
void nsm_vr_dump (struct lib_globals *, struct nsm_msg_vr *);
void nsm_vrf_dump (struct lib_globals *, struct nsm_msg_vrf *);
void nsm_vr_bind_dump (struct lib_globals *, struct nsm_msg_vr_bind *);
void nsm_vrf_bind_dump (struct lib_globals *, struct nsm_msg_vrf_bind *);
void nsm_user_dump (struct lib_globals *, struct nsm_msg_user *, char *);
#ifdef HAVE_MPLS
void nsm_label_pool_dump (struct lib_globals *, struct nsm_msg_label_pool *);
#ifdef HAVE_PACKET
void nsm_ilm_lookup_dump (struct lib_globals *, struct nsm_msg_ilm_lookup *);
#endif /* HAVE_PACKET */

void nsm_ilm_lookup_gen_dump (struct lib_globals *,
                              struct nsm_msg_ilm_gen_lookup *);
#endif /* HAVE_MPLS */

#ifdef HAVE_TE
void nsm_qos_release_dump (struct lib_globals *, struct nsm_msg_qos_release *);
void nsm_qos_clean_dump (struct lib_globals *, struct nsm_msg_qos_clean *);
void nsm_qos_preempt_dump (struct lib_globals *, struct nsm_msg_qos_preempt *);
void nsm_qos_dump (struct lib_globals *, struct nsm_msg_qos *, int);
void nsm_admin_group_dump (struct lib_globals *, struct nsm_msg_admin_group *);
#ifdef HAVE_GMPLS
void nsm_gmpls_qos_release_dump (struct lib_globals *, struct nsm_msg_gmpls_qos_release *);
void nsm_gmpls_qos_clean_dump (struct lib_globals *, struct nsm_msg_gmpls_qos_clean *);
void nsm_gmpls_qos_preempt_dump (struct lib_globals *, struct nsm_msg_qos_preempt *);
void nsm_gmpls_qos_dump (struct lib_globals *, struct nsm_msg_gmpls_qos *, int);
#endif /*HAVE_GMPLS*/
#endif /* HAVE_TE */

void nsm_route_manage_dump (struct lib_globals *, struct nsm_msg_route_manage *);
void nsm_router_id_dump (struct lib_globals *, struct nsm_msg_router_id *);

#ifdef HAVE_MPLS_VC
int nsm_encode_vc_fib_add (u_char **, u_int16_t *,
                           struct nsm_msg_vc_fib_add *);
int nsm_encode_vc_fib_delete (u_char **, u_int16_t *,
                              struct nsm_msg_vc_fib_delete *);
int nsm_encode_mpls_vc (u_char **, u_int16_t *, struct nsm_msg_mpls_vc *);
void nsm_mpls_vc_dump (struct lib_globals *, struct nsm_msg_mpls_vc *);
int nsm_parse_mpls_vc (u_char **, u_int16_t *, struct nsm_msg_header *,
                       void *, NSM_CALLBACK);

int nsm_parse_vc_fib_add (u_char **, u_int16_t *, struct nsm_msg_header *,
                          void *, NSM_CALLBACK);
int nsm_parse_vc_fib_delete (u_char **, u_int16_t *, struct nsm_msg_header *,
                          void *, NSM_CALLBACK);

int nsm_encode_vc_tunnel_info (u_char **, u_int16_t *,
                               struct nsm_msg_vc_tunnel_info *);

int nsm_parse_vc_tunnel_info (u_char **, u_int16_t *,
                              struct nsm_msg_header *, void *,
                              NSM_CALLBACK);
void nsm_vc_fib_add_dump (struct lib_globals *, struct nsm_msg_vc_fib_add *);
void nsm_vc_fib_delete_dump (struct lib_globals *,
                             struct nsm_msg_vc_fib_delete *);
void nsm_mpls_pw_status_msg_dump (struct lib_globals *,
                                  struct nsm_msg_pw_status *);
int nsm_encode_pw_status (u_char **, u_int16_t *, struct nsm_msg_pw_status *);
int nsm_parse_pw_status (u_char **, u_int16_t *, struct nsm_msg_header *,
                         void *, NSM_CALLBACK);
#ifdef HAVE_MS_PW
int nsm_encode_mpls_ms_pw_msg (u_char **pnt, u_int16_t *size,
                               struct nsm_msg_mpls_ms_pw_msg *msg);
int nsm_decode_mpls_ms_pw_msg (u_char **pnt, u_int16_t *size,
                               struct nsm_msg_mpls_ms_pw_msg *msg);
int nsm_parse_mpls_ms_pw_msg (u_char **pnt, u_int16_t *size,
                              struct nsm_msg_header *header, void *arg,
                              NSM_CALLBACK callback);
#endif /* HAVE_MS_PW */
#endif /* HAVE_MPLS_VC */

int nsm_decode_interface (u_char **, u_int16_t *, struct nsm_msg_link *);
int nsm_encode_address (u_char **, u_int16_t *, struct nsm_msg_address *);
int nsm_decode_address (u_char **, u_int16_t *, struct nsm_msg_address *);

#ifdef HAVE_GMPLS
/* TBD old code
void nsm_gmpls_if_dump (struct lib_globals *, struct nsm_msg_gmpls_if *); */
s_int32_t nsm_encode_te_link (u_char **pnt, u_int16_t *size,
                              struct nsm_msg_te_link *msg);

s_int32_t nsm_decode_te_link (u_char **pnt, u_int16_t *size,
                              struct nsm_msg_te_link *msg);

s_int32_t nsm_encode_data_link_sub (u_char **pnt, u_int16_t *size,
                                    struct nsm_msg_data_link_sub *msg);

s_int32_t nsm_decode_data_link_sub (u_char **pnt, u_int16_t *size,
                                    struct nsm_msg_data_link_sub *msg);

s_int32_t nsm_encode_data_link (u_char **pnt, u_int16_t *size,
                                struct nsm_msg_data_link *msg);

s_int32_t nsm_decode_data_link (u_char **pnt, u_int16_t *size,
                                struct nsm_msg_data_link *msg);

s_int32_t nsm_encode_control_channel (u_char **pnt, u_int16_t *size,
                                      struct nsm_msg_control_channel *msg);

s_int32_t nsm_decode_control_channel (u_char **pnt, u_int16_t *size,
                                      struct nsm_msg_control_channel *msg);

s_int32_t nsm_encode_control_adj (u_char **pnt, u_int16_t *size,
                                  struct nsm_msg_control_adj *msg);

s_int32_t nsm_decode_control_adj (u_char **pnt, u_int16_t *size,
                                  struct nsm_msg_control_adj *msg);

s_int32_t nsm_parse_data_link (u_char **pnt, u_int16_t *size,
                               struct nsm_msg_header *header, void *arg,
                               NSM_CALLBACK callback);

s_int32_t nsm_parse_data_link_sub (u_char **pnt, u_int16_t *size,
                                   struct nsm_msg_header *header, void *arg,
                                   NSM_CALLBACK callback);

s_int32_t nsm_parse_control_adj (u_char **pnt, u_int16_t *size,
                                 struct nsm_msg_header *header, void *arg,
                                 NSM_CALLBACK callback);

s_int32_t nsm_parse_te_link (u_char **pnt, u_int16_t *size,
                             struct nsm_msg_header *header, void *arg,
                             NSM_CALLBACK callback);

s_int32_t nsm_parse_control_channel (u_char **pnt, u_int16_t *size,
                                     struct nsm_msg_header *header, void *arg,
                                     NSM_CALLBACK callback);

void nsm_control_channel_dump (struct lib_globals *zg,
                               struct nsm_msg_control_channel *msg);

void nsm_control_adj_dump (struct lib_globals *zg,
                           struct nsm_msg_control_adj *msg);

void nsm_server_control_channel_msg_set (struct control_channel *cc,
                                         cindex_t cindex,
                                         struct nsm_msg_control_channel *msg);

void nsm_data_link_sub_dump (struct lib_globals *,
			     struct nsm_msg_data_link_sub *);

void nsm_data_link_dump (struct lib_globals *,
			 struct nsm_msg_data_link *);

void nsm_te_link_dump (struct lib_globals *,
		       struct nsm_msg_te_link *);

#endif /* HAVE_GMPLS */


#ifdef HAVE_DIFFSERV
int nsm_encode_supported_dscp_update (u_char **, u_int16_t *,
                                      struct nsm_msg_supported_dscp_update *);
int nsm_encode_dscp_exp_map_update (u_char **, u_int16_t *,
                                    struct nsm_msg_dscp_exp_map_update *);
int nsm_decode_supported_dscp_update (u_char **, u_int16_t *,
                                      struct nsm_msg_supported_dscp_update *);
int nsm_decode_dscp_exp_map_update (u_char **, u_int16_t *,
                                    struct nsm_msg_dscp_exp_map_update *);
int nsm_parse_supported_dscp_update (u_char **, u_int16_t *,
                                     struct nsm_msg_header *, void *,
                                     NSM_CALLBACK);
int nsm_parse_dscp_exp_map_update (u_char **, u_int16_t *,
                                   struct nsm_msg_header *, void *,
                                   NSM_CALLBACK);
void nsm_supported_dscp_update_dump (struct lib_globals *,
                                     struct nsm_msg_supported_dscp_update *);
void nsm_dscp_exp_map_update_dump (struct lib_globals *,
                                   struct nsm_msg_dscp_exp_map_update *);
#endif /* HAVE_DIFFSERV */

#ifdef HAVE_VPLS
int nsm_encode_vpls_mac_withdraw (u_char **, u_int16_t *,
                                  struct nsm_msg_vpls_mac_withdraw *);
int nsm_encode_vpls_add (u_char **, u_int16_t *, struct nsm_msg_vpls_add *);
int nsm_encode_vpls_delete (u_char **, u_int16_t *, u_int32_t);
int nsm_parse_vpls_mac_withdraw (u_char **, u_int16_t *,
                                 struct nsm_msg_header *, void *,
                                 NSM_CALLBACK);
int nsm_parse_vpls_add (u_char **, u_int16_t *, struct nsm_msg_header *,
                        void *, NSM_CALLBACK);
int nsm_parse_vpls_delete (u_char **, u_int16_t *, struct nsm_msg_header *,
                           void *, NSM_CALLBACK);
void nsm_vpls_add_dump (struct lib_globals *, struct nsm_msg_vpls_add *);
void nsm_vpls_mac_withdraw_dump (struct lib_globals *,
                                 struct nsm_msg_vpls_mac_withdraw *);
#endif /* HAVE_VPLS */

#ifdef HAVE_DSTE
struct nsm_msg_dste_te_class_update;
int nsm_parse_dste_te_class_update(u_char **, u_int16_t *,
                                   struct nsm_msg_header *,
                                   void *, NSM_CALLBACK);
int nsm_encode_dste_te_class_update (u_char **, u_int16_t *,
                                     struct nsm_msg_dste_te_class_update *);
int nsm_decode_dste_te_class_update (u_char **, u_int16_t *,
                                     struct nsm_msg_dste_te_class_update *);
void nsm_dste_te_class_update_dump (struct lib_globals *,
                                    struct nsm_msg_dste_te_class_update *);
#endif /* HAVE_DSTE */

#ifdef HAVE_MCAST_IPV4

/* IPV4 Vif Add */
int nsm_encode_ipv4_vif_add (u_char **, u_int16_t *, struct nsm_msg_ipv4_vif_add *);
int nsm_decode_ipv4_vif_add (u_char **, u_int16_t *, struct nsm_msg_ipv4_vif_add *);
int nsm_parse_ipv4_vif_add (u_char **, u_int16_t *, struct nsm_msg_header *,
    void *, NSM_CALLBACK);
void nsm_ipv4_vif_add_dump (struct lib_globals *, struct nsm_msg_ipv4_vif_add *);

/* IPV4 Vif Del */
int nsm_encode_ipv4_vif_del (u_char **, u_int16_t *, struct nsm_msg_ipv4_vif_del *);
int nsm_decode_ipv4_vif_del (u_char **, u_int16_t *, struct nsm_msg_ipv4_vif_del *);
int nsm_parse_ipv4_vif_del (u_char **, u_int16_t *, struct nsm_msg_header *,
    void *, NSM_CALLBACK);
void nsm_ipv4_vif_del_dump (struct lib_globals *, struct nsm_msg_ipv4_vif_del *);

/* IPV4 MRT add */
int nsm_encode_ipv4_mrt_add (u_char **, u_int16_t *, struct nsm_msg_ipv4_mrt_add *);
int nsm_decode_ipv4_mrt_add (u_char **, u_int16_t *, struct nsm_msg_ipv4_mrt_add *);
int nsm_parse_ipv4_mrt_add (u_char **, u_int16_t *, struct nsm_msg_header *,
    void *, NSM_CALLBACK);
void nsm_ipv4_mrt_add_dump (struct lib_globals *, struct nsm_msg_ipv4_mrt_add *);

/* IPV4 MRT del */
int nsm_encode_ipv4_mrt_del (u_char **, u_int16_t *, struct nsm_msg_ipv4_mrt_del *);
int nsm_decode_ipv4_mrt_del (u_char **, u_int16_t *, struct nsm_msg_ipv4_mrt_del *);
int nsm_parse_ipv4_mrt_del (u_char **, u_int16_t *, struct nsm_msg_header *,
    void *, NSM_CALLBACK);
void nsm_ipv4_mrt_del_dump (struct lib_globals *, struct nsm_msg_ipv4_mrt_del *);

/* IPV4 MRT stat flags update */
int nsm_encode_ipv4_mrt_stat_flags_update (u_char **, u_int16_t *, struct nsm_msg_ipv4_mrt_stat_flags_update *);
int nsm_decode_ipv4_mrt_stat_flags_update (u_char **, u_int16_t *, struct nsm_msg_ipv4_mrt_stat_flags_update *);
int nsm_parse_ipv4_mrt_stat_flags_update (u_char **, u_int16_t *, struct nsm_msg_header *, void *, NSM_CALLBACK);
void nsm_ipv4_mrt_stat_flags_update_dump (struct lib_globals *, struct nsm_msg_ipv4_mrt_stat_flags_update *);


/* IPV4 MRT State Refresh Flag */
int nsm_encode_ipv4_mrt_state_refresh_flag_update (u_char **, u_int16_t *,
                      struct nsm_msg_ipv4_mrt_state_refresh_flag_update *);
int nsm_decode_ipv4_mrt_state_refresh_flag_update (u_char **, u_int16_t *,
                      struct nsm_msg_ipv4_mrt_state_refresh_flag_update *);
int nsm_parse_ipv4_mrt_state_refresh_flag_update (u_char **, u_int16_t *,
                      struct nsm_msg_header *, void *, NSM_CALLBACK);

/* IPV4 MRT NOCACHE */
int nsm_encode_ipv4_mrt_nocache (u_char **, u_int16_t *, struct nsm_msg_ipv4_mrt_nocache *);
int nsm_decode_ipv4_mrt_nocache (u_char **, u_int16_t *, struct nsm_msg_ipv4_mrt_nocache *);
int nsm_parse_ipv4_mrt_nocache (u_char **, u_int16_t *, struct nsm_msg_header *, void *, NSM_CALLBACK);
void nsm_ipv4_mrt_nocache_dump (struct lib_globals *, struct nsm_msg_ipv4_mrt_nocache *);

/* IPV4 MRT WRONGVIF */
int nsm_encode_ipv4_mrt_wrongvif (u_char **, u_int16_t *, struct nsm_msg_ipv4_mrt_wrongvif *);
int nsm_decode_ipv4_mrt_wrongvif (u_char **, u_int16_t *, struct nsm_msg_ipv4_mrt_wrongvif *);
int nsm_parse_ipv4_mrt_wrongvif (u_char **, u_int16_t *, struct nsm_msg_header *, void *, NSM_CALLBACK);
void nsm_ipv4_mrt_wrongvif_dump (struct lib_globals *, struct nsm_msg_ipv4_mrt_wrongvif *);

/* IPV4 MRT WHOLEPKT REQUEST */
int nsm_encode_ipv4_mrt_wholepkt_req (u_char **, u_int16_t *, struct nsm_msg_ipv4_mrt_wholepkt_req *);
int nsm_decode_ipv4_mrt_wholepkt_req (u_char **, u_int16_t *, struct nsm_msg_ipv4_mrt_wholepkt_req *);
int nsm_parse_ipv4_mrt_wholepkt_req (u_char **, u_int16_t *, struct nsm_msg_header *, void *, NSM_CALLBACK);
void nsm_ipv4_mrt_wholepkt_req_dump (struct lib_globals *, struct nsm_msg_ipv4_mrt_wholepkt_req *);

/* IPV4 MRT WHOLEPKT REPLY */
int nsm_encode_ipv4_mrt_wholepkt_reply (u_char **, u_int16_t *, struct nsm_msg_ipv4_mrt_wholepkt_reply *);
int nsm_decode_ipv4_mrt_wholepkt_reply (u_char **, u_int16_t *, struct nsm_msg_ipv4_mrt_wholepkt_reply *);
int nsm_parse_ipv4_mrt_wholepkt_reply (u_char **, u_int16_t *, struct nsm_msg_header *, void *, NSM_CALLBACK);
void nsm_ipv4_mrt_wholepkt_reply_dump (struct lib_globals *, struct nsm_msg_ipv4_mrt_wholepkt_reply *);

/* IPV4 MRT STAT UPDATE */
int nsm_encode_ipv4_mrt_stat_update (u_char **, u_int16_t *, struct nsm_msg_ipv4_mrt_stat_update *);
int nsm_decode_ipv4_mrt_stat_update (u_char **, u_int16_t *, struct nsm_msg_ipv4_mrt_stat_update *);
int nsm_parse_ipv4_mrt_stat_update (u_char **, u_int16_t *, struct nsm_msg_header *, void *, NSM_CALLBACK);
void nsm_ipv4_mrt_stat_update_dump (struct lib_globals *, struct nsm_msg_ipv4_mrt_stat_update *);

/* IPV4 MRIB NOTIFICATION */
int nsm_encode_ipv4_mrib_notification (u_char **, u_int16_t *, struct nsm_msg_ipv4_mrib_notification *);
int nsm_decode_ipv4_mrib_notification (u_char **, u_int16_t *, struct nsm_msg_ipv4_mrib_notification *);
int nsm_parse_ipv4_mrib_notification (u_char **, u_int16_t *, struct nsm_msg_header *, void *, NSM_CALLBACK);
void nsm_ipv4_mrib_notification_dump (struct lib_globals *, struct nsm_msg_ipv4_mrib_notification *);

/* IGMP Local-Membership messages */
s_int32_t
nsm_encode_igmp (u_int8_t **, u_int16_t *, struct nsm_msg_igmp *);
s_int32_t
nsm_decode_igmp (u_int8_t **, u_int16_t *, struct nsm_msg_igmp **);
s_int32_t
nsm_parse_igmp (u_int8_t **, u_int16_t *, struct nsm_msg_header *,
                void *, NSM_CALLBACK);
s_int32_t
nsm_igmp_dump (struct lib_globals *, struct nsm_msg_igmp *);

/* Mtrace query */
int nsm_encode_mtrace_query_msg (u_char **, u_int16_t *,
    struct nsm_msg_mtrace_query *);
int nsm_decode_mtrace_query_msg (u_char **, u_int16_t *,
    struct nsm_msg_mtrace_query *);
int nsm_parse_mtrace_query_msg (u_char **, u_int16_t *,
    struct nsm_msg_header *, void *, NSM_CALLBACK);
void nsm_mtrace_query_dump_msg (struct lib_globals *,
    struct nsm_msg_mtrace_query *);
/* Mtrace request */
int nsm_encode_mtrace_request_msg (u_char **, u_int16_t *,
    struct nsm_msg_mtrace_request *);
int nsm_decode_mtrace_request_msg (u_char **, u_int16_t *,
    struct nsm_msg_mtrace_request *);
int nsm_parse_mtrace_request_msg (u_char **, u_int16_t *,
    struct nsm_msg_header *, void *, NSM_CALLBACK);
void nsm_mtrace_request_dump_msg (struct lib_globals *,
    struct nsm_msg_mtrace_request *);

#endif /* HAVE_MCAST_IPV4 */

#ifdef HAVE_MCAST_IPV6

/* IPV6 Mif Add */
int nsm_encode_ipv6_mif_add (u_char **, u_int16_t *, struct nsm_msg_ipv6_mif_add *);
int nsm_decode_ipv6_mif_add (u_char **, u_int16_t *, struct nsm_msg_ipv6_mif_add *);
int nsm_parse_ipv6_mif_add (u_char **, u_int16_t *, struct nsm_msg_header *,
    void *, NSM_CALLBACK);
void nsm_ipv6_mif_add_dump (struct lib_globals *, struct nsm_msg_ipv6_mif_add *);

/* IPV6 Mif Del */
int nsm_encode_ipv6_mif_del (u_char **, u_int16_t *, struct nsm_msg_ipv6_mif_del *);
int nsm_decode_ipv6_mif_del (u_char **, u_int16_t *, struct nsm_msg_ipv6_mif_del *);
int nsm_parse_ipv6_mif_del (u_char **, u_int16_t *, struct nsm_msg_header *,
    void *, NSM_CALLBACK);
void nsm_ipv6_mif_del_dump (struct lib_globals *, struct nsm_msg_ipv6_mif_del *);

/* IPV6 MRT add */
int nsm_encode_ipv6_mrt_add (u_char **, u_int16_t *, struct nsm_msg_ipv6_mrt_add *);
int nsm_decode_ipv6_mrt_add (u_char **, u_int16_t *, struct nsm_msg_ipv6_mrt_add *);
int nsm_parse_ipv6_mrt_add (u_char **, u_int16_t *, struct nsm_msg_header *,
    void *, NSM_CALLBACK);
void nsm_ipv6_mrt_add_dump (struct lib_globals *, struct nsm_msg_ipv6_mrt_add *);

/* IPV6 MRT del */
int nsm_encode_ipv6_mrt_del (u_char **, u_int16_t *, struct nsm_msg_ipv6_mrt_del *);
int nsm_decode_ipv6_mrt_del (u_char **, u_int16_t *, struct nsm_msg_ipv6_mrt_del *);
int nsm_parse_ipv6_mrt_del (u_char **, u_int16_t *, struct nsm_msg_header *,
    void *, NSM_CALLBACK);
void nsm_ipv6_mrt_del_dump (struct lib_globals *, struct nsm_msg_ipv6_mrt_del *);

/* IPV6 MRT stat flags update */
int nsm_encode_ipv6_mrt_stat_flags_update (u_char **, u_int16_t *, struct nsm_msg_ipv6_mrt_stat_flags_update *);
int nsm_decode_ipv6_mrt_stat_flags_update (u_char **, u_int16_t *, struct nsm_msg_ipv6_mrt_stat_flags_update *);
int nsm_parse_ipv6_mrt_stat_flags_update (u_char **, u_int16_t *, struct nsm_msg_header *, void *, NSM_CALLBACK);
void nsm_ipv6_mrt_stat_flags_update_dump (struct lib_globals *, struct nsm_msg_ipv6_mrt_stat_flags_update *);

/* IPV6 MRT State Refresh Flag */
int nsm_encode_ipv6_mrt_state_refresh_flag_update (u_char **, u_int16_t *,
                      struct nsm_msg_ipv6_mrt_state_refresh_flag_update *);
int nsm_decode_ipv6_mrt_state_refresh_flag_update (u_char **, u_int16_t *,
                      struct nsm_msg_ipv6_mrt_state_refresh_flag_update *);
int nsm_parse_ipv6_mrt_state_refresh_flag_update (u_char **, u_int16_t *,
                      struct nsm_msg_header *, void *, NSM_CALLBACK);

/* IPV6 MRT NOCACHE */
int nsm_encode_ipv6_mrt_nocache (u_char **, u_int16_t *, struct nsm_msg_ipv6_mrt_nocache *);
int nsm_decode_ipv6_mrt_nocache (u_char **, u_int16_t *, struct nsm_msg_ipv6_mrt_nocache *);
int nsm_parse_ipv6_mrt_nocache (u_char **, u_int16_t *, struct nsm_msg_header *, void *, NSM_CALLBACK);
void nsm_ipv6_mrt_nocache_dump (struct lib_globals *, struct nsm_msg_ipv6_mrt_nocache *);

/* IPV6 MRT WRONGMIF */
int nsm_encode_ipv6_mrt_wrongmif (u_char **, u_int16_t *, struct nsm_msg_ipv6_mrt_wrongmif *);
int nsm_decode_ipv6_mrt_wrongmif (u_char **, u_int16_t *, struct nsm_msg_ipv6_mrt_wrongmif *);
int nsm_parse_ipv6_mrt_wrongmif (u_char **, u_int16_t *, struct nsm_msg_header *, void *, NSM_CALLBACK);
void nsm_ipv6_mrt_wrongmif_dump (struct lib_globals *, struct nsm_msg_ipv6_mrt_wrongmif *);

/* IPV6 MRT WHOLEPKT REQUEST */
int nsm_encode_ipv6_mrt_wholepkt_req (u_char **, u_int16_t *, struct nsm_msg_ipv6_mrt_wholepkt_req *);
int nsm_decode_ipv6_mrt_wholepkt_req (u_char **, u_int16_t *, struct nsm_msg_ipv6_mrt_wholepkt_req *);
int nsm_parse_ipv6_mrt_wholepkt_req (u_char **, u_int16_t *, struct nsm_msg_header *, void *, NSM_CALLBACK);
void nsm_ipv6_mrt_wholepkt_req_dump (struct lib_globals *, struct nsm_msg_ipv6_mrt_wholepkt_req *);

/* IPV6 MRT WHOLEPKT REPLY */
int nsm_encode_ipv6_mrt_wholepkt_reply (u_char **, u_int16_t *, struct nsm_msg_ipv6_mrt_wholepkt_reply *);
int nsm_decode_ipv6_mrt_wholepkt_reply (u_char **, u_int16_t *, struct nsm_msg_ipv6_mrt_wholepkt_reply *);
int nsm_parse_ipv6_mrt_wholepkt_reply (u_char **, u_int16_t *, struct nsm_msg_header *, void *, NSM_CALLBACK);
void nsm_ipv6_mrt_wholepkt_reply_dump (struct lib_globals *, struct nsm_msg_ipv6_mrt_wholepkt_reply *);

/* IPV6 MRT STAT UPDATE */
int nsm_encode_ipv6_mrt_stat_update (u_char **, u_int16_t *, struct nsm_msg_ipv6_mrt_stat_update *);
int nsm_decode_ipv6_mrt_stat_update (u_char **, u_int16_t *, struct nsm_msg_ipv6_mrt_stat_update *);
int nsm_parse_ipv6_mrt_stat_update (u_char **, u_int16_t *, struct nsm_msg_header *, void *, NSM_CALLBACK);
void nsm_ipv6_mrt_stat_update_dump (struct lib_globals *, struct nsm_msg_ipv6_mrt_stat_update *);

/* IPV6 MRIB NOTIFICATION */
int nsm_encode_ipv6_mrib_notification (u_char **, u_int16_t *, struct nsm_msg_ipv6_mrib_notification *);
int nsm_decode_ipv6_mrib_notification (u_char **, u_int16_t *, struct nsm_msg_ipv6_mrib_notification *);
int nsm_parse_ipv6_mrib_notification (u_char **, u_int16_t *, struct nsm_msg_header *, void *, NSM_CALLBACK);
void nsm_ipv6_mrib_notification_dump (struct lib_globals *, struct nsm_msg_ipv6_mrib_notification *);

/* MLD JOIN/LEAVE message */
s_int32_t
nsm_encode_mld (u_int8_t **, u_int16_t *, struct nsm_msg_mld *);
s_int32_t
nsm_decode_mld (u_int8_t **, u_int16_t *, struct nsm_msg_mld **);
s_int32_t
nsm_parse_mld (u_int8_t **, u_int16_t *, struct nsm_msg_header *,
               void *, NSM_CALLBACK);
s_int32_t
nsm_mld_dump (struct lib_globals *, struct nsm_msg_mld *);
#endif /* HAVE_MCAST_IPV6 */

#ifdef HAVE_LACPD
int nsm_encode_lacp_aggregator_add (u_char **, u_int16_t *,
            struct nsm_msg_lacp_agg_add *);
int nsm_encode_lacp_aggregator_del (u_char **, u_int16_t *, char *);
int nsm_decode_lacp_aggregator_add (u_char **, u_int16_t *,
            struct nsm_msg_lacp_agg_add *);
int nsm_decode_lacp_aggregator_del (u_char **, u_int16_t *, char *);
int nsm_parse_lacp_aggregator_add (u_char **, u_int16_t *,
                                   struct nsm_msg_header *, void *,
                                   NSM_CALLBACK);
int nsm_parse_lacp_aggregator_del (u_char **, u_int16_t *,
                                   struct nsm_msg_header *, void *,
                                   NSM_CALLBACK);
int
nsm_encode_static_agg_cnt (u_char **pnt, u_int16_t *size,
                              struct nsm_msg_agg_cnt *msg);
int
nsm_decode_static_agg_cnt (u_char **pnt, u_int16_t *size,
                              struct nsm_msg_agg_cnt *msg);
int
nsm_encode_lacp_send_agg_config (u_char **pnt, u_int16_t *size,
                              struct nsm_msg_lacp_aggregator_config *msg);
int
nsm_decode_lacp_send_agg_config (u_char **pnt, u_int16_t *size,
                              struct nsm_msg_lacp_aggregator_config *msg);


#endif /* HAVE_LACPD */

#ifdef HAVE_L2
int nsm_encode_stp_msg (u_char **pnt, u_int16_t *size, struct nsm_msg_stp *msg);
int nsm_decode_stp_msg (u_char **pnt, u_int16_t *size, struct nsm_msg_stp *msg);
int nsm_parse_stp_msg (u_char **pnt, u_int16_t *size,
                       struct nsm_msg_header *header, void *arg,
                       NSM_CALLBACK callback);
void nsm_stp_interface_dump (struct lib_globals *zg, struct nsm_msg_stp *msg);
int
nsm_parse_cfm_mac_msg (u_char **pnt, u_int16_t *size,
                       struct nsm_msg_header *header, void *arg,
                       NSM_CALLBACK callback);

#endif /* HAVE_L2 */

#ifdef HAVE_RMOND
int
nsm_parse_rmon_msg (u_char **pnt, u_int16_t *size,
                    struct nsm_msg_header *header, void *arg,
                    NSM_CALLBACK callback);
int
nsm_encode_rmon_stats (u_char **pnt, u_int16_t *size,
                       struct nsm_msg_rmon_stats *msg);
#define NSM_MSG_RMON_SIZE                   4
#define NSM_MSG_RMON_STATS_SIZE             244
#endif /* HAVE_RMON */

#ifdef HAVE_MPLS_FRR
struct nsm_msg_rsvp_ctrl_pkt
{
  u_int32_t ftn_index;

  u_int32_t cpkt_id;
  u_int32_t flags;
#define RSVP_CTRL_PKT_SEG      1 << 0
#define RSVP_CTRL_PKT_BEGIN    1 << 1
#define RSVP_CTRL_PKT_END      1 << 2

  /* if RSVP_CTRL_PKT_SEG is set, need set total_len and seq_num */
  u_int32_t total_len;
  u_int32_t seq_num;

  u_int32_t pkt_size;
  char *pkt;
};

int nsm_encode_rsvp_control_packet (u_char **pnt, u_int16_t *size,
                                    struct nsm_msg_rsvp_ctrl_pkt *msg);

int nsm_decode_rsvp_control_packet (u_char **pnt,
                                    u_int16_t *size,
                                    struct nsm_msg_rsvp_ctrl_pkt *ctrl_pkt);

int nsm_parse_rsvp_control_packet (u_char **pnt, u_int16_t *size,
                                   struct nsm_msg_header *header, void *arg,
                                   NSM_CALLBACK callback);
#endif /* HAVE_MPLS_FRR */

#ifdef HAVE_NSM_MPLS_OAM
int
nsm_parse_mpls_oam_request (u_char **pnt, u_int16_t *size,
                          struct nsm_msg_header *header, void *arg,
                          NSM_CALLBACK callback);

int
nsm_parse_mpls_oam_req (u_char **pnt, u_int16_t *size,
                        struct nsm_msg_header *header, void *arg,
                        NSM_CALLBACK callback);
int
nsm_encode_mpls_onm_req (u_char **pnt, u_int16_t *size,
                         struct nsm_msg_mpls_ping_req *msg);
int
nsm_decode_mpls_onm_req (u_char **pnt, u_int16_t *size,
                         struct nsm_msg_mpls_ping_req *msg);
int
nsm_encode_mpls_onm_ping_reply (u_char **pnt, u_int16_t *size,
                                struct nsm_msg_mpls_ping_reply *msg);
int
nsm_decode_mpls_onm_ping_reply (u_char **pnt, u_int16_t *size,
                                struct nsm_msg_mpls_ping_reply *msg);
int
nsm_encode_mpls_onm_trace_reply (u_char **pnt, u_int16_t *size,
                                struct nsm_msg_mpls_tracert_reply *msg);
int
nsm_decode_mpls_onm_trace_reply (u_char **pnt, u_int16_t *size,
                                struct nsm_msg_mpls_tracert_reply *msg);
int
nsm_encode_mpls_onm_error (u_char **pnt, u_int16_t *size,
                           struct nsm_msg_mpls_ping_error *msg);
int
nsm_decode_mpls_onm_error (u_char **pnt, u_int16_t *size,
                           struct nsm_msg_mpls_ping_error *msg);
int
nsm_parse_oam_ping_response (u_char **pnt, u_int16_t *size,
                             struct nsm_msg_header *header, void *arg,
                             NSM_CALLBACK callback);
int
nsm_parse_oam_tracert_reply (u_char **pnt, u_int16_t *size,
                             struct nsm_msg_header *header, void *arg,
                             NSM_CALLBACK callback);
int
nsm_parse_oam_error (u_char **pnt, u_int16_t *size,
                     struct nsm_msg_header *header, void *arg,
                     NSM_CALLBACK callback);
#endif /* HAVE_NSM_MPLS_OAM */

#if defined HAVE_MPLS_OAM || defined HAVE_NSM_MPLS_OAM
int
nsm_parse_mpls_vpn_rd (u_char **pnt, u_int16_t *size,
                       struct nsm_msg_header *header, void *arg,
                       NSM_CALLBACK callback);

int
nsm_encode_mpls_vpn_rd (u_char **, u_int16_t *, struct nsm_msg_vpn_rd *);

int
nsm_decode_mpls_vpn_rd (u_char **, u_int16_t *, struct nsm_msg_vpn_rd *);
#endif /* HAVE_MPLS_OAM || HAVE_NSM_MPLS_OAM */

#ifdef HAVE_MPLS_OAM_ITUT
int
nsm_parse_mpls_oam_process_request (u_char **pnt, u_int16_t *size,
		                    struct nsm_msg_header *header, void *arg,
		                    NSM_CALLBACK callback);

int
nsm_mpls_oam_decode_itut_req (u_char **, u_int16_t *,
                         struct nsm_msg_mpls_oam_itut_req *);

#endif /*HAVE_MPLS_OAM_ITUT*/

/* Encode function for nexthop tracking message */
int nsm_encode_nexthop_tracking (u_char **,
                                 u_int16_t *,
                                 struct nsm_msg_nexthop_tracking *);

/* Decode function for nexthop tracking message */
int nsm_decode_nexthop_tracking (u_char **,
                                 u_int16_t *,
                                 struct nsm_msg_nexthop_tracking *);
/* Parse function to parse the received NSM_MSG_NEXTHOP_TRACKING message. */
int nsm_parse_nexthop_tracking (u_char **,
                                u_int16_t *,
                                struct nsm_msg_header *,
                                void *,
                                NSM_CALLBACK );
void
nsm_nh_track_dump (struct lib_globals *,
                  /*  void *, */
                   struct nsm_msg_nexthop_tracking *);

int
nsm_encode_nht_msg (u_char **, u_int16_t *, struct nsm_msg_nexthop_tracking *);

int
nsm_decode_nht_msg (u_char **, u_int16_t *, struct nsm_msg_nexthop_tracking *);

#ifdef HAVE_ELMID

void
nsm_elmi_bw_dump (struct lib_globals *zg, struct nsm_msg_bw_profile *msg);

int
nsm_encode_elmi_status_msg (u_char **pnt, u_int16_t *size,
                            struct nsm_msg_elmi_status *msg);

int
nsm_decode_elmi_status_msg (u_char **pnt, u_int16_t *size,
                            struct nsm_msg_elmi_status *msg);

int
nsm_parse_elmi_status_msg (u_char **pnt, u_int16_t *size,
                           struct nsm_msg_header *header, void *arg,
                           NSM_CALLBACK callback);

int
nsm_encode_elmi_vlan_msg (u_char **pnt, u_int16_t *size,
                            struct nsm_msg_vlan_port *msg);
int
nsm_decode_elmi_vlan_msg (u_char **pnt, u_int16_t *size,
                            struct nsm_msg_vlan_port *msg);

int
nsm_parse_elmi_vlan_msg (u_char **pnt, u_int16_t *size,
                           struct nsm_msg_header *header, void *arg,
                           NSM_CALLBACK callback);
int
nsm_encode_evc_msg (u_char **pnt, u_int16_t *size,
                    struct nsm_msg_elmi_evc *msg);
int
nsm_decode_evc_msg (u_char **pnt, u_int16_t *size,
                    struct nsm_msg_elmi_evc *msg);

int
nsm_parse_elmi_evc_msg (u_char **pnt, u_int16_t *size,
                        struct nsm_msg_header *header, void *arg,
                        NSM_CALLBACK callback);

int
nsm_parse_uni_msg (u_char **pnt, u_int16_t *size,
                   struct nsm_msg_header *header, void *arg,
                   NSM_CALLBACK callback);
int
nsm_encode_uni_msg (u_char **pnt, u_int16_t *size,
                    struct nsm_msg_uni *msg);
int
nsm_decode_uni_msg (u_char **pnt, u_int16_t *size,
                    struct nsm_msg_uni *msg);

int
nsm_encode_elmi_bw_msg (u_char **pnt, u_int16_t *size,
                        struct nsm_msg_bw_profile *msg);
int
nsm_decode_elmi_bw_msg (u_char **pnt, u_int16_t *size,
                        struct nsm_msg_bw_profile *msg);
int
nsm_parse_elmi_bw_msg (u_char **pnt, u_int16_t *size,
                       struct nsm_msg_header *header, void *arg,
                       NSM_CALLBACK callback);
#endif /* HAVE_ELMID */

#ifdef HAVE_LMP
#ifdef HAVE_RESTART
int
nsm_parse_lmp_protocol_restart (u_char **, u_int16_t *,
                                struct nsm_msg_header *, void *,
                                NSM_CALLBACK);
int
nsm_decode_lmp_protocol_restart (u_char **, u_int16_t *,
                                 struct nsm_msg_lmp_protocol_restart *);

int
nsm_encode_lmp_protocol_restart (u_char **, u_int16_t *,
                                 struct nsm_msg_lmp_protocol_restart *);
#endif /* HAVE_RESTART */
int
nsm_encode_dlink_opaque (u_char **, u_int16_t *,
                         struct nsm_msg_dlink_opaque *);
int
nsm_decode_dlink_opaque (u_char **, u_int16_t *,
                             struct nsm_msg_dlink_opaque *);

int
nsm_encode_dlink_testmsg (u_char **, u_int16_t *,
                          struct nsm_msg_dlink_testmsg *);
int
nsm_decode_dlink_testmsg (u_char **, u_int16_t *,
                          struct nsm_msg_dlink_testmsg *);

#endif /* HAVE_LMP */

#ifdef HAVE_GMPLS
#ifdef HAVE_PBB_TE
s_int32_t
nsm_encode_te_tesi_msg (u_char **, u_int16_t *, struct nsm_msg_tesi_info *);
s_int32_t
nsm_decode_te_tesi_msg (u_char **, u_int16_t *, struct nsm_msg_tesi_info *);
s_int32_t
nsm_parse_te_tesi_msg(u_char **, u_int16_t *, struct nsm_msg_header *, void *,
                      NSM_CALLBACK );
#endif /* HAVE_PBB_TE */
#endif /* HAVE_GMPLS*/
#endif /* _NSM_MESSAGE_H */
