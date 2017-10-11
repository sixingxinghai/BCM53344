/* Copyright (C) 2002-2003  All Rights Reserved. */

/* NSM SMI server implementation.  */

#include "lib_fm_api.h"
#include "smi_nsm_server.h"
#include "smi_server.h"

#ifdef HAVE_SMI

/* Initialize NSM SMI server.  */
struct smi_server *
nsm_smi_server_init (struct lib_globals *zg)
{
  int ret;
  struct smi_server *as;
  struct message_handler *ms;

  /* Create message server.  */
  ms = message_server_create (zg);
  if (! ms)
    return NULL;

#ifndef HAVE_TCP_MESSAGE
  /* Set server type to UNIX domain socket.  */
  message_server_set_style_domain (ms, SMI_SERV_NSM_PATH);
#else /* HAVE_TCP_MESSAGE */
  message_server_set_style_tcp (ms, SMI_PORT_NSM);
#endif /* !HAVE_TCP_MESSAGE */

  /* Set call back functions.  */
  message_server_set_callback (ms, MESSAGE_EVENT_CONNECT,
                               smi_server_connect);
  message_server_set_callback (ms, MESSAGE_EVENT_DISCONNECT,
                               smi_server_disconnect);
  message_server_set_callback (ms, MESSAGE_EVENT_READ_MESSAGE,
                               smi_server_read_msg);

  /* Start SMI server.  */
  ret = message_server_start (ms);
  if (ret < 0)
    {
      zlog_err(NSM_ZG, "Error in starting message server");
    }

  /* When message server works fine, go forward to create SMI server
     structure.  */
  as = XCALLOC (MTYPE_SMISERVER, sizeof (struct smi_server));
  as->zg = zg;
  as->ms = ms;
  ms->info = as;

  /* Set version and protocol.  */
  smi_server_set_version (as, SMI_PROTOCOL_VERSION_1);
  smi_server_set_protocol (as, SMI_PROTO_SMISERVER);

  /* Set services.  */
  smi_server_set_service (as, SMI_SERVICE_INTERFACE);

  /* Set callback functions to SMI.  */
  smi_server_set_callback (as, SMI_MSG_IF_SETMTU,
                           smi_parse_if,
                           nsm_smi_server_recv_if);

  smi_server_set_callback (as, SMI_MSG_IF_GETMTU,
                           smi_parse_if,
                           nsm_smi_server_recv_if);

  smi_server_set_callback (as, SMI_MSG_IF_SETBW,
                           smi_parse_if,
                           nsm_smi_server_recv_if);

  smi_server_set_callback (as, SMI_MSG_IF_GETBW,
                           smi_parse_if,
                           nsm_smi_server_recv_if);

  smi_server_set_callback (as, SMI_MSG_IF_SETFLAG,
                           smi_parse_if,
                           nsm_smi_server_recv_if);

  smi_server_set_callback (as, SMI_MSG_IF_UNSETFLAG,
                           smi_parse_if,
                           nsm_smi_server_recv_if);

  smi_server_set_callback (as, SMI_MSG_IF_SETAUTO,
                           smi_parse_if,
                           nsm_smi_server_recv_if);

  smi_server_set_callback (as, SMI_MSG_IF_GETAUTO,
                           smi_parse_if,
                           nsm_smi_server_recv_if);

  smi_server_set_callback (as, SMI_MSG_IF_SETHWADDR,
                           smi_parse_if,
                           nsm_smi_server_recv_if);

  smi_server_set_callback (as, SMI_MSG_IF_GETHWADDR,
                           smi_parse_if,
                           nsm_smi_server_recv_if);

  smi_server_set_callback (as, SMI_MSG_IF_SETDUPLEX,
                           smi_parse_if,
                           nsm_smi_server_recv_if);

  smi_server_set_callback (as, SMI_MSG_IF_GETDUPLEX,
                           smi_parse_if,
                           nsm_smi_server_recv_if);

  smi_server_set_callback (as, SMI_MSG_IF_UNSETDUPLEX,
                           smi_parse_if,
                           nsm_smi_server_recv_if);

  smi_server_set_callback (as, SMI_MSG_IF_GETBRIEF,
                           smi_parse_if,
                           nsm_smi_server_recv_if);

  smi_server_set_callback (as, SMI_MSG_IF_GETMCAST,
                           smi_parse_if,
                           nsm_smi_server_recv_if);

  smi_server_set_callback (as, SMI_MSG_IF_SETMCAST,
                           smi_parse_if,
                           nsm_smi_server_recv_if);

  smi_server_set_callback (as, SMI_MSG_IF_UNSETMT,
                           smi_parse_if,
                           nsm_smi_server_recv_if);

  smi_server_set_callback (as, SMI_MSG_IF_CHANGE_GET,
                           smi_parse_if,
                           nsm_smi_server_recv_if);

  smi_server_set_callback (as, SMI_MSG_IF_GET_STATISTICS,
                           smi_parse_if,
                           nsm_smi_server_recv_if);

  smi_server_set_callback (as, SMI_MSG_IF_CLEAR_STATISTICS,
                           smi_parse_if,
                           nsm_smi_server_recv_if);

  smi_server_set_callback (as, SMI_MSG_IF_SET_MDIX_CROSSOVER,
                           smi_parse_if,
                           nsm_smi_server_recv_if);

  smi_server_set_callback (as, SMI_MSG_IF_LACP_ADDLINK,
                           smi_parse_if,
                           nsm_smi_server_recv_if);

  smi_server_set_callback (as, SMI_IF_MSG_LACP_DELETELINK,
                           smi_parse_if,
                           nsm_smi_server_recv_if);

  smi_server_set_callback (as, SMI_MSG_IF_GET_MDIX_CROSSOVER,
                           smi_parse_if,
                           nsm_smi_server_recv_if);

  smi_server_set_callback (as, SMI_MSG_IF_BRIDGE_ADDMAC,
                           smi_parse_if,
                           nsm_smi_server_recv_if);

  smi_server_set_callback (as, SMI_MSG_IF_BRIDGE_DELMAC,
                           smi_parse_if,
                           nsm_smi_server_recv_if);

  smi_server_set_callback (as, SMI_MSG_IF_BRIDGE_FLUSH_DYNAMICENT,
                           smi_parse_if,
                           nsm_smi_server_recv_if);

  smi_server_set_callback (as, SMI_MSG_APN_GET_TRAFFIC_CLASSTBL,
                           smi_parse_if,
                           nsm_smi_server_recv_if);

  smi_server_set_callback (as, SMI_MSG_ADD_BRIDGE,
                           smi_parse_if,
                           nsm_smi_server_recv_if);

  smi_server_set_callback (as, SMI_MSG_ADD_BRIDGE_PORT,
                           smi_parse_if,
                           nsm_smi_server_recv_if);

  smi_server_set_callback (as, SMI_MSG_DEL_BRIDGE,
                           smi_parse_if,
                           nsm_smi_server_recv_if);

  smi_server_set_callback (as, SMI_MSG_DEL_BRIDGE_PORT,
                           smi_parse_if,
                           nsm_smi_server_recv_if);

  smi_server_set_callback (as, SMI_MSG_CHANGE_TYPE,
                           smi_parse_if,
                           nsm_smi_server_recv_if);

  smi_server_set_callback (as, SMI_MSG_GETBRIDGE_TYPE,
                           smi_parse_if,
                           nsm_smi_server_recv_if);

  smi_server_set_callback (as, SMI_MSG_SET_PORT_NON_CONFIG,
                           smi_parse_if,
                           nsm_smi_server_recv_if);

  smi_server_set_callback (as, SMI_MSG_GET_PORT_NON_CONFIG,
                           smi_parse_if,
                           nsm_smi_server_recv_if);

  smi_server_set_callback (as, SMI_MSG_SET_PORT_LEARNING,
                           smi_parse_if,
                           nsm_smi_server_recv_if);

  smi_server_set_callback (as, SMI_MSG_GET_PORT_LEARNING,
                           smi_parse_if,
                           nsm_smi_server_recv_if);

  smi_server_set_callback (as, SMI_IF_MSG_SET_DOT1Q_STATE,
                           smi_parse_if,
                           nsm_smi_server_recv_if);

  smi_server_set_callback (as, SMI_IF_MSG_GET_DOT1Q_STATE,
                           smi_parse_if,
                           nsm_smi_server_recv_if);

  smi_server_set_callback (as, SMI_IF_MSG_SET_DTAG_MODE,
                           smi_parse_if,
                           nsm_smi_server_recv_if);

  smi_server_set_callback (as, SMI_IF_MSG_GET_DTAG_MODE,
                           smi_parse_if,
                           nsm_smi_server_recv_if);

  smi_server_set_callback (as, SMI_MSG_SET_EGRESS_PORT_MODE,
                           smi_parse_if,
                           nsm_smi_server_recv_if);

  smi_server_set_callback (as, SMI_MSG_SET_PORT_NON_SWITCHING,
                           smi_parse_if,
                           nsm_smi_server_recv_if);

  smi_server_set_callback (as, SMI_MSG_GET_PORT_NON_SWITCHING,
                           smi_parse_if,
                           nsm_smi_server_recv_if);

  smi_server_set_callback (as, SMI_MSG_IF_GETFLAGS,
                           smi_parse_if,
                           nsm_smi_server_recv_if);

  smi_server_set_callback (as, SMI_MSG_SW_RESET,
                           smi_parse_if,
                           nsm_smi_server_recv_if);

  smi_server_set_callback (as, SMI_MSG_VLAN_ADD,
                           smi_parse_vlan,
                           nsm_smi_server_recv_vlan);

  smi_server_set_callback (as, SMI_MSG_VLAN_DEL,
                           smi_parse_vlan,
                           nsm_smi_server_recv_vlan);

  smi_server_set_callback (as, SMI_MSG_VLAN_RANGE_ADD,
                           smi_parse_vlan,
                           nsm_smi_server_recv_vlan);

  smi_server_set_callback (as, SMI_MSG_VLAN_RANGE_DEL,
                           smi_parse_vlan,
                           nsm_smi_server_recv_vlan);

  smi_server_set_callback (as, SMI_MSG_VLAN_SET_PORT_MODE,
                           smi_parse_vlan,
                           nsm_smi_server_recv_vlan);

 smi_server_set_callback (as, SMI_MSG_VLAN_GET_PORT_MODE,
                          smi_parse_vlan,
                          nsm_smi_server_recv_vlan);

 smi_server_set_callback (as, SMI_MSG_VLAN_SET_ACC_FRAME_TYPE,
                          smi_parse_vlan,
                          nsm_smi_server_recv_vlan);

 smi_server_set_callback (as, SMI_MSG_VLAN_GET_ACC_FRAME_TYPE,
                          smi_parse_vlan,
                          nsm_smi_server_recv_vlan);

 smi_server_set_callback (as, SMI_MSG_VLAN_SET_INGRESS_FILTER,
                          smi_parse_vlan,
                          nsm_smi_server_recv_vlan);

 smi_server_set_callback (as, SMI_MSG_VLAN_GET_INGRESS_FILTER,
                          smi_parse_vlan,
                          nsm_smi_server_recv_vlan);

 smi_server_set_callback (as, SMI_MSG_VLAN_SET_DEFAULT_VID,
                          smi_parse_vlan,
                          nsm_smi_server_recv_vlan);

 smi_server_set_callback (as, SMI_MSG_VLAN_GET_DEFAULT_VID,
                          smi_parse_vlan,
                          nsm_smi_server_recv_vlan);

 smi_server_set_callback (as, SMI_MSG_VLAN_ADD_TO_PORT,
                          smi_parse_vlan,
                          nsm_smi_server_recv_vlan);

 smi_server_set_callback (as, SMI_MSG_VLAN_DEL_FROM_PORT,
                          smi_parse_vlan,
                          nsm_smi_server_recv_vlan);

 smi_server_set_callback (as, SMI_MSG_VLAN_CLEAR_PORT,
                          smi_parse_vlan,
                          nsm_smi_server_recv_vlan);

 smi_server_set_callback (as, SMI_MSG_VLAN_ADD_ALL_EXCEPT_VID,
                          smi_parse_vlan,
                          nsm_smi_server_recv_vlan);

 smi_server_set_callback (as, SMI_MSG_VLAN_GET_ALL_VLAN_CONFIG,
                          smi_parse_vlan,
                          nsm_smi_server_recv_vlan);

 smi_server_set_callback (as, SMI_MSG_VLAN_GET_VLAN_BY_ID,
                          smi_parse_vlan,
                          nsm_smi_server_recv_vlan);

 smi_server_set_callback (as, SMI_MSG_VLAN_GET_IF,
                          smi_parse_vlan,
                          nsm_smi_server_recv_vlan);

 smi_server_set_callback (as, SMI_MSG_VLAN_GET_BRIDGE,
                          smi_parse_vlan,
                          nsm_smi_server_recv_vlan);

 smi_server_set_callback (as, SMI_MSG_VLAN_SET_PORT_PROTO_PROCESS,
                          smi_parse_vlan,
                          nsm_smi_server_recv_vlan);

 smi_server_set_callback (as, SMI_MSG_FORCE_DEFAULT_VLAN,
                          smi_parse_vlan,
                          nsm_smi_server_recv_vlan);

 smi_server_set_callback (as, SMI_MSG_SET_PRESERVE_CE_COS,
                          smi_parse_vlan,
                          nsm_smi_server_recv_vlan);

 smi_server_set_callback (as, SMI_MSG_SET_PORT_BASED_VLAN,
                          smi_parse_vlan,
                          nsm_smi_server_recv_vlan);

 smi_server_set_callback (as, SMI_MSG_SET_CPUPORT_DEFAULT_VLAN,
                          smi_parse_vlan,
                          nsm_smi_server_recv_vlan);

 smi_server_set_callback (as, SMI_MSG_SET_WAYSIDEPORT_DEFAULT_VLAN,
                          smi_parse_vlan,
                          nsm_smi_server_recv_vlan);

 smi_server_set_callback (as, SMI_MSG_SET_CPUPORT_BASED_VLAN,
                          smi_parse_vlan,
                          nsm_smi_server_recv_vlan);

 smi_server_set_callback (as, SMI_MSG_SVLAN_SET_PORT_ETHER_TYPE,
                          smi_parse_vlan,
                          nsm_smi_server_recv_vlan);

 smi_server_set_callback (as, SMI_MSG_VLAN_GET_PORT_PROTO_PROCESS,
                          smi_parse_vlan,
                          nsm_smi_server_recv_vlan);

 smi_server_set_callback (as, SMI_MSG_NSM_HA_SWITCH,
                          smi_parse_vlan,
                          nsm_smi_server_recv_vlan);

 smi_server_set_callback (as, SMI_MSG_GVRP_SET_TIMER,
                          smi_parse_gvrp,
                          nsm_smi_server_recv_gvrp);

 smi_server_set_callback (as, SMI_MSG_GVRP_GET_TIMER,
                          smi_parse_gvrp,
                          nsm_smi_server_recv_gvrp);

 smi_server_set_callback (as, SMI_MSG_GVRP_GET_TIMER_DETAILS,
                          smi_parse_gvrp,
                          nsm_smi_server_recv_gvrp);

 smi_server_set_callback (as, SMI_MSG_GVRP_ENABLE,
                          smi_parse_gvrp,
                          nsm_smi_server_recv_gvrp);

 smi_server_set_callback (as, SMI_MSG_GVRP_DISABLE,
                          smi_parse_gvrp,
                          nsm_smi_server_recv_gvrp);

 smi_server_set_callback (as, SMI_MSG_GVRP_ENABLE_PORT,
                          smi_parse_gvrp,
                          nsm_smi_server_recv_gvrp);

 smi_server_set_callback (as, SMI_MSG_GVRP_DISABLE_PORT,
                          smi_parse_gvrp,
                          nsm_smi_server_recv_gvrp);

 smi_server_set_callback (as, SMI_MSG_GVRP_SET_REG_MODE,
                          smi_parse_gvrp,
                          nsm_smi_server_recv_gvrp);

 smi_server_set_callback (as, SMI_MSG_GVRP_GET_REG_MODE,
                          smi_parse_gvrp,
                          nsm_smi_server_recv_gvrp);

 smi_server_set_callback (as, SMI_MSG_GVRP_GET_PER_VLAN_STATS,
                          smi_parse_gvrp,
                          nsm_smi_server_recv_gvrp);

 smi_server_set_callback (as, SMI_MSG_GVRP_CLEAR_ALL_STATS,
                          smi_parse_gvrp,
                          nsm_smi_server_recv_gvrp);

 smi_server_set_callback (as, SMI_MSG_GVRP_SET_DYNAMIC_VLAN_LEARNING,
                          smi_parse_gvrp,
                          nsm_smi_server_recv_gvrp);

 smi_server_set_callback (as, SMI_MSG_GVRP_GET_BRIDGE_CONFIG,
                          smi_parse_gvrp,
                          nsm_smi_server_recv_gvrp);

 smi_server_set_callback (as, SMI_MSG_GVRP_GET_VID_DETAILS,
                          smi_parse_gvrp,
                          nsm_smi_server_recv_gvrp);

 smi_server_set_callback (as, SMI_MSG_GVRP_GET_STATE_MACHINE_BRIDGE,
                          smi_parse_gvrp,
                          nsm_smi_server_recv_gvrp);

 smi_server_set_callback (as, SMI_MSG_GVRP_GET_PORT_STATS,
                          smi_parse_gvrp,
                          nsm_smi_server_recv_gvrp);

 smi_server_set_callback (as, SMI_MSG_QOS_GLOBAL_ENABLE,
                          smi_parse_qos,
                          nsm_smi_server_recv_qos);

 smi_server_set_callback (as, SMI_MSG_QOS_GLOBAL_DISABLE,
                          smi_parse_qos,
                          nsm_smi_server_recv_qos);

 smi_server_set_callback (as, SMI_MSG_QOS_GET_GLOBAL_STATUS,
                          smi_parse_qos,
                          nsm_smi_server_recv_qos);

 smi_server_set_callback (as, SMI_MSG_QOS_SET_PMAP_NAME,
                          smi_parse_qos,
                          nsm_smi_server_recv_qos);

 smi_server_set_callback (as, SMI_MSG_QOS_GET_POLICY_MAP_NAMES,
                          smi_parse_qos,
                          nsm_smi_server_recv_qos);

 smi_server_set_callback (as, SMI_MSG_QOS_GET_POLICY_MAP,
                          smi_parse_qos,
                          nsm_smi_server_recv_qos);

 smi_server_set_callback (as, SMI_MSG_QOS_PMAP_DELETE,
                          smi_parse_qos,
                          nsm_smi_server_recv_qos);

 smi_server_set_callback (as, SMI_MSG_QOS_SET_CMAP_NAME,
                          smi_parse_qos,
                          nsm_smi_server_recv_qos);

 smi_server_set_callback (as, SMI_MSG_QOS_GET_CMAP_NAME,
                          smi_parse_qos,
                          nsm_smi_server_recv_qos);

 smi_server_set_callback (as, SMI_MSG_QOS_DELETE_CMAP_NAME,
                          smi_parse_qos,
                          nsm_smi_server_recv_qos);

 smi_server_set_callback (as, SMI_MSG_QOS_CMAP_MATCH_TRAFFIC_SET,
                          smi_parse_qos,
                          nsm_smi_server_recv_qos);

 smi_server_set_callback (as, SMI_MSG_QOS_CMAP_MATCH_TRAFFIC_UNSET,
                          smi_parse_qos,
                          nsm_smi_server_recv_qos);

 smi_server_set_callback (as, SMI_MSG_QOS_CMAP_MATCH_TRAFFIC_GET,
                          smi_parse_qos,
                          nsm_smi_server_recv_qos);

 smi_server_set_callback (as, SMI_MSG_QOS_PMAPC_POLICE,
                          smi_parse_qos,
                          nsm_smi_server_recv_qos);

 smi_server_set_callback (as, SMI_MSG_QOS_PMAPC_POLICE_GET,
                          smi_parse_qos,
                          nsm_smi_server_recv_qos);

 smi_server_set_callback (as, SMI_MSG_QOS_PMAPC_POLICE_DELETE,
                          smi_parse_qos,
                          nsm_smi_server_recv_qos);

 smi_server_set_callback (as, SMI_MSG_QOS_PMAPC_DELETE_CMAP,
                          smi_parse_qos,
                          nsm_smi_server_recv_qos);
 
 smi_server_set_callback (as, SMI_MSG_QOS_SET_PORT_SCHEDULING,
                          smi_parse_qos,
                          nsm_smi_server_recv_qos);

 smi_server_set_callback (as, SMI_MSG_QOS_GET_PORT_SCHEDULING,
                          smi_parse_qos,
                          nsm_smi_server_recv_qos);

 smi_server_set_callback (as, SMI_MSG_QOS_SET_DEFAULT_USER_PRIORITY,
                          smi_parse_qos,
                          nsm_smi_server_recv_qos);

 smi_server_set_callback (as, SMI_MSG_QOS_GET_DEFAULT_USER_PRIORITY,
                          smi_parse_qos,
                          nsm_smi_server_recv_qos);

 smi_server_set_callback (as, SMI_MSG_QOS_GET_DEFAULT_USER_PRIORITY,
                          smi_parse_qos,
                          nsm_smi_server_recv_qos);

 smi_server_set_callback (as, SMI_MSG_QOS_PORT_SET_REGEN_USER_PRIORITY,
                          smi_parse_qos,
                          nsm_smi_server_recv_qos);

 smi_server_set_callback (as, SMI_MSG_QOS_PORT_GET_REGEN_USER_PRIORITY,
                          smi_parse_qos,
                          nsm_smi_server_recv_qos);

 smi_server_set_callback (as, SMI_MSG_QOS_GLOBAL_COS_TO_QUEUE,
                          smi_parse_qos,
                          nsm_smi_server_recv_qos);

 smi_server_set_callback (as, SMI_MSG_QOS_GLOBAL_DSCP_TO_QUEUE,
                          smi_parse_qos,
                          nsm_smi_server_recv_qos);

 smi_server_set_callback (as, SMI_MSG_QOS_SET_TRUST_STATE,
                          smi_parse_qos,
                          nsm_smi_server_recv_qos);

 smi_server_set_callback (as, SMI_MSG_QOS_SET_FORCE_TRUST_COS,
                          smi_parse_qos,
                          nsm_smi_server_recv_qos);

 smi_server_set_callback (as, SMI_MSG_QOS_SET_FRAME_TYPE_PRIORITY_OVERRIDE,
                          smi_parse_qos,
                          nsm_smi_server_recv_qos);

 smi_server_set_callback (as, SMI_MSG_QOS_SET_VLAN_PRIORITY,
                          smi_parse_qos,
                          nsm_smi_server_recv_qos);

 smi_server_set_callback (as, SMI_MSG_QOS_UNSET_VLAN_PRIORITY,
                          smi_parse_qos,
                          nsm_smi_server_recv_qos);

 smi_server_set_callback (as, SMI_MSG_QOS_SET_PORT_VLAN_PRIORITY,
                          smi_parse_qos,
                          nsm_smi_server_recv_qos);

 smi_server_set_callback (as, SMI_MSG_QOS_SET_PORT_DA_PRIORITY,
                          smi_parse_qos,
                          nsm_smi_server_recv_qos);

 smi_server_set_callback (as, SMI_MSG_QOS_GET_VLAN_PRIORITY,
                          smi_parse_qos,
                          nsm_smi_server_recv_qos);

 smi_server_set_callback (as, SMI_MSG_QOS_SET_QUEUE_WEIGHT,
                          smi_parse_qos,
                          nsm_smi_server_recv_qos);

 smi_server_set_callback (as, SMI_MSG_QOS_GET_QUEUE_WEIGHT,
                          smi_parse_qos,
                          nsm_smi_server_recv_qos);

 smi_server_set_callback (as, SMI_MSG_QOS_SET_PORT_SERVICE_POLICY,
                          smi_parse_qos,
                          nsm_smi_server_recv_qos);

 smi_server_set_callback (as, SMI_MSG_QOS_GET_PORT_SERVICE_POLICY,
                          smi_parse_qos,
                          nsm_smi_server_recv_qos);

 smi_server_set_callback (as, SMI_MSG_QOS_UNSET_PORT_SERVICE_POLICY,
                          smi_parse_qos,
                          nsm_smi_server_recv_qos);

 smi_server_set_callback (as, SMI_MSG_QOS_SET_TRAFFIC_SHAPE,
                          smi_parse_qos,
                          nsm_smi_server_recv_qos);

 smi_server_set_callback (as, SMI_MSG_QOS_UNSET_TRAFFIC_SHAPE,
                          smi_parse_qos,
                          nsm_smi_server_recv_qos);

 smi_server_set_callback (as, SMI_MSG_QOS_GET_TRAFFIC_SHAPE,
                          smi_parse_qos,
                          nsm_smi_server_recv_qos);

 smi_server_set_callback (as, SMI_MSG_QOS_GET_COS_TO_QUEUE,
                          smi_parse_qos,
                          nsm_smi_server_recv_qos);

 smi_server_set_callback (as, SMI_MSG_QOS_GET_DSCP_TO_QUEUE,
                          smi_parse_qos,
                          nsm_smi_server_recv_qos);

 smi_server_set_callback (as, SMI_MSG_QOS_GET_TRUST_STATE,
                          smi_parse_qos,
                          nsm_smi_server_recv_qos);

 smi_server_set_callback (as, SMI_MSG_QOS_GET_PORT_VLAN_PRIORITY,
                          smi_parse_qos,
                          nsm_smi_server_recv_qos);

 smi_server_set_callback (as, SMI_MSG_QOS_GET_PORT_DA_PRIORITY,
                          smi_parse_qos,
                          nsm_smi_server_recv_qos);

 smi_server_set_callback (as, SMI_MSG_QOS_GET_FORCE_TRUST_COS,
                          smi_parse_qos,
                          nsm_smi_server_recv_qos);

 smi_server_set_callback (as, SMI_MSG_ADD_FC,
                          smi_parse_fc,
                          nsm_smi_server_recv_fc);

 smi_server_set_callback (as, SMI_MSG_DELETE_FC,
                          smi_parse_fc,
                          nsm_smi_server_recv_fc);

 smi_server_set_callback (as, SMI_MSG_FC_STATISTICS,
                          smi_parse_fc,
                          nsm_smi_server_recv_fc);

 smi_server_set_callback (as, SMI_MSG_FC_GET_INTERFACE,
                          smi_parse_fc,
                          nsm_smi_server_recv_fc);
 smi_server_set_callback (as, SMI_IF_MSG_IF_EXIST,
                          smi_parse_if,
                          nsm_smi_server_recv_if);
 smi_server_set_callback (as, SMI_IF_MSG_BRIDGE_EXIST,
                          smi_parse_if,
                          nsm_smi_server_recv_if);
 smi_server_set_callback (as, SMI_MSG_IF_BRIDGE_MAC_ADD_PRIO_OVR,
                          smi_parse_if,
                          nsm_smi_server_recv_if);
 smi_server_set_callback (as, SMI_MSG_IF_BRIDGE_MAC_DEL_PRIO_OVR,
                          smi_parse_if,
                          nsm_smi_server_recv_if);
  ng->nsm_smiserver = as;

  return as;
}

int
nsm_smi_server_send_sync_message(struct smi_server_entry *ase,
                                 int msgtype, int status, void *getmsg)
{
  int len = 0;

  if(msgtype == SMI_MSG_STATUS)
    {
      struct smi_msg_status smsg;
      if(status == 0) /* success */
        {
          smsg.status_code = SMI_STATUS_SUCCESS;
          sprintf(smsg.reason, "Value set successfully");
        } else {
          smsg.status_code = status;
          sprintf(smsg.reason, "Error in setting value");
        }
        /* Send the success/failure back to client */
        /* Set encode pointer and size.  */
        ase->send.pnt = ase->send.buf + SMI_MSG_HEADER_SIZE;
        ase->send.size = ase->send.len - SMI_MSG_HEADER_SIZE;

        /* Encode SMI if msg.  */
        len = smi_encode_statusmsg (&ase->send.pnt, &ase->send.size, &smsg);
        if (len < 0)
          return len;
        /* Send it to client.  */
        smi_server_send_message (ase, 0, 0, SMI_MSG_STATUS, 0, len);
    } else if(msgtype > SMI_MSG_IF_START && msgtype < SMI_MSG_IF_END)
    {
      struct smi_msg_if *ifmsg = (struct smi_msg_if *) getmsg;

      ase->send.pnt = ase->send.buf + SMI_MSG_HEADER_SIZE;
      ase->send.size = ase->send.len - SMI_MSG_HEADER_SIZE;

      /* Encode SMI if msg.  */
      len = smi_encode_ifmsg (&ase->send.pnt, &ase->send.size, ifmsg);
      if (len < 0)
        return len;
      smi_server_send_message (ase, 0, 0, msgtype, 0, len);
    } else if(msgtype > SMI_MSG_VLAN_START && msgtype < SMI_MSG_VLAN_END)
    {
      struct smi_msg_vlan *vmsg = (struct smi_msg_vlan *) getmsg;

      ase->send.pnt = ase->send.buf + SMI_MSG_HEADER_SIZE;
      ase->send.size = ase->send.len - SMI_MSG_HEADER_SIZE;

      /* Encode SMI vlan msg.  */
      len = smi_encode_vlan_msg (&ase->send.pnt, &ase->send.size, vmsg);
      if (len < 0)
        return len;
      smi_server_send_message (ase, 0, 0, msgtype, 0, len);
    } else if(msgtype > SMI_MSG_GVRP_START && msgtype < SMI_MSG_GVRP_END)
    {
      struct smi_msg_gvrp *gmsg = (struct smi_msg_gvrp *) getmsg;

      ase->send.pnt = ase->send.buf + SMI_MSG_HEADER_SIZE;
      ase->send.size = ase->send.len - SMI_MSG_HEADER_SIZE;

      /* Encode SMI GvRP msg.  */
      len = smi_encode_gvrpmsg (&ase->send.pnt, &ase->send.size, gmsg);
      if (len < 0)
        return len;
      smi_server_send_message (ase, 0, 0, msgtype, 0, len);
    } else if(msgtype > SMI_MSG_QOS_START && msgtype < SMI_MSG_QOS_END)
    {
      struct smi_msg_qos *qmsg = (struct smi_msg_qos *) getmsg;

      ase->send.pnt = ase->send.buf + SMI_MSG_HEADER_SIZE;
      ase->send.size = ase->send.len - SMI_MSG_HEADER_SIZE;

      /* Encode SMI QOS msg.  */
      len = smi_encode_qosmsg (&ase->send.pnt, &ase->send.size, qmsg);
      if (len < 0)
        return len;
      smi_server_send_message (ase, 0, 0, msgtype, 0, len);
    } else if(msgtype > SMI_MSG_FC_START && msgtype < SMI_MSG_FC_END)
    {
      struct smi_msg_flowctrl *fmsg = (struct smi_msg_flowctrl *) getmsg;

      ase->send.pnt = ase->send.buf + SMI_MSG_HEADER_SIZE;
      ase->send.size = ase->send.len - SMI_MSG_HEADER_SIZE;

      /* Encode SMI FC msg.  */
      len = smi_encode_flowctrlmsg (&ase->send.pnt, &ase->send.size, fmsg);
      if (len < 0)
        return len;
      smi_server_send_message (ase, 0, 0, msgtype, 0, len);
    }

  return 0;
}

#endif /* HAVE_SMI */
