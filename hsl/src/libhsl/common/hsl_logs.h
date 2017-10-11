/* Copyright (C) 2004   All Rights Reserved. */ 

#ifndef _HSL_COMMON_LOGS_H
#define _HSL_COMMON_LOGS_H

#include <hsl_logger.h>

/* #define DEBUG  */
#ifdef DEBUG
#ifdef HAVE_ISO_MACRO_VARARGS
#define HSL_FN_ENTER(...)                                             \
    do {                                                              \
       HSL_LOG (HSL_LOG_GENERAL, HSL_LEVEL_INFO, "Entering %s\n",     \
                __FUNCTION__);                                        \
    } while (0)
#define HSL_FN_EXIT(...)                                              \
    do {                                                              \
       HSL_LOG (HSL_LOG_GENERAL, HSL_LEVEL_INFO, "Returning %s, %d, 0x%x\n",\
               __FUNCTION__, __LINE__, __VA_ARGS__);                               \
        return __VA_ARGS__;                                           \
    } while (0)
#else
#define HSL_FN_ENTER(ARGS...)                                         \
    do {                                                              \
       HSL_LOG (HSL_LOG_GENERAL, HSL_LEVEL_INFO, "Entering %s\n",     \
                __FUNCTION__);                                        \
    } while (0)
#define HSL_FN_EXIT(ARGS...)                                          \
    do {                                                              \
       HSL_LOG (HSL_LOG_GENERAL, HSL_LEVEL_INFO, "Returning %s, %d\n",\
               __FUNCTION__, __LINE__);                               \
       return ARGS;                                                   \
    } while (0)
#endif /* HAVE_ISO_MACRO_VARARGS */
#else
#ifdef HAVE_ISO_MACRO_VARARGS
#define HSL_FN_ENTER(...)
#else
#define HSL_FN_ENTER(ARGS...)
#endif /* HAVE_ISO_MACRO_VARARGS */
#ifdef HAVE_ISO_MACRO_VARARGS
#define HSL_FN_EXIT(...)                                              \
    return __VA_ARGS__;
#else
#define HSL_FN_EXIT(ARGS...)                                          \
    return ARGS;
#endif /* HAVE_ISO_MACRO_VARARGS */
#endif /* DEBUG */

/* General */
#define HSL_ERR_WRONG_PARAMS                  HSL_LOG(HSL_MODULE_ALL,HSL_LEVEL_ERROR,"%s:%d Error: Wrong parameters",__FILE__,__LINE__)
#define HSL_TRACE_GEN(S)                      HSL_LOG(HSL_MODULE_ALL,HSL_LEVEL_ADMIN,"%s",S)

/* System operations */  
#define HSL_ERR_SYS_DETECTION(E)              HSL_LOG(HSL_MODULE_ALL,HSL_LEVEL_ERROR,"%s:%d Error:(%d) System Detection failed error code",__FILE__,__LINE__,E); 
#define HSL_ERR_UNSUPPORTED_DEV(D)            HSL_LOG(HSL_MODULE_ALL,HSL_LEVEL_ERROR,"%s:%d Error: Unsupported System dev id: %d",__FILE__,__LINE__,D); 
#define HSL_ERR_SYS_INIT_FAILED(E)            HSL_LOG(HSL_MODULE_ALL,HSL_LEVEL_ERROR,"%s:%d Error:(%d) System Init failed",__FILE__,__LINE__,E);
#define HSL_ERR_BRIDGE_INIT_FAILED(E)         HSL_LOG(HSL_MODULE_ALL,HSL_LEVEL_ERROR,"%s:%d Error:(%d) Bridge Init failed",__FILE__,__LINE__,E);
#define HSL_ERR_RESET_FAILED(E)               HSL_LOG(HSL_MODULE_ALL,HSL_LEVEL_ERROR,"%s:%d Error:(%d) Reseting the system",__FILE__,__LINE__,E); 

/* Interface map operations. */
#define HSL_ERR_IFMAP_NOT_READY(T)            HSL_LOG(HSL_LOG_PLATFORM, HSL_LEVEL_ERROR,"%s:%d Maximum number of %s ports not set\n",__FILE__,__LINE__,T);
#define HSL_ERR_IFMAP_FULL(S)                 HSL_LOG(HSL_LOG_PLATFORM, HSL_LEVEL_ERROR,"%s:%d Maximum number of %s ports was reached\n",__FILE__,__LINE__,S);
#define HSL_ERR_IFMAP_MAC_FULL(S)             HSL_LOG(HSL_LOG_PLATFORM, HSL_LEVEL_ERROR,"%s:%d No mac address for %s port is available\n",__FILE__,__LINE__,S);


/* OS related */
#define HSL_ERR_MUTEX_CREATE(S)               HSL_LOG(HSL_MODULE_ALL,HSL_LEVEL_ERROR,"%s:%d Error (%d) to create mutex descriptors!",__FILE__,__LINE__,S);
#define HSL_ERR_SEM_CREATE(S)                 HSL_LOG(HSL_MODULE_ALL,HSL_LEVEL_ERROR,"%s:%d Error (%d) to create semaphore!",__FILE__,__LINE__,S);
#define HSL_ERR_MALLOC(D)                     HSL_LOG(HSL_MODULE_ALL,HSL_LEVEL_ERROR,"%s:%d Error Memory allocation failed size:(%d)!",__FILE__,__LINE__,D);
#define HSL_ERR_DMA_MALLOC(D)                 HSL_LOG(HSL_MODULE_ALL,HSL_LEVEL_ERROR,"%s:%d Error DMA memory allocation failed size:(%d)!",__FILE__,__LINE__,D);
#define HSL_ERR_THREAD_CREATE(E)              HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_FATAL,"%s:%d Error(%d)start thread\n",E);


/* Bridge related */
#define HSL_ERR_BRIDGE_NOT_FOUND(S)           HSL_LOG(HSL_MODULE_ALL,HSL_LEVEL_ERROR,"%s:%d Error: Bridge (%s) not found",__FILE__,__LINE__,S)
#define HSL_ERR_SET_L2_TIMEOUT(E,T)           HSL_LOG(HSL_MODULE_ALL,HSL_LEVEL_ERROR,"%s:%d Error:(%d) Setting L2 aging timeout to: %d sec",__FILE__,__LINE__,E,T);

/* Interface related */ 
#define HSL_ERR_WRONG_INTERFACE_TYPE(T)       HSL_LOG(HSL_LOG_IFMGR,HSL_LEVEL_ERROR,"%s:%d Error: Wrong interface type (%d)",__FILE__,__LINE__,T);
#define HSL_ERR_IFMAP_FOR_LPORT_NOTFOUND(D)   HSL_LOG(HSL_LOG_IFMGR,HSL_LEVEL_ERROR,"%s:%d Error: ifmap for logical port (%d) not found",__FILE__,__LINE__,D)
#define HSL_ERR_IFMAP_FOR_IFIDX_NOTFOUND(D)   HSL_LOG(HSL_LOG_IFMGR,HSL_LEVEL_ERROR,"%s:%d Error: ifmap for ifindex (%d) not found",__FILE__,__LINE__,D)
#define HSL_TRACE_IFMAP_DEBUG(D,U,P,L,N)      HSL_LOG(HSL_LOG_IFMGR,HSL_LEVEL_DEBUG,"ifmap add dev(%d) unit(%d) port(%d) lport(%d) ifname(%s)", D,U,P,L,N)
#define HSL_ERR_IF_TYPE_MISMATCH(F,S)        HSL_LOG(HSL_LOG_IFMGR,HSL_LEVEL_ERROR,"%s:%d Error: interface type mismatch (%s) vs (%s)",__FILE__,__LINE__,F,S);  
#define HSL_ERR_ENABLE_L3_BC(E,I)             HSL_LOG(HSL_LOG_IFMGR,HSL_LEVEL_ERROR,"%s:%d Error (%d): Enable L3 broadcasts on interface (%d)",__FILE__,__LINE__,E,I);
/* Layer 2 - interface */
#define HSL_ERR_IF_WRONG_IF_TYPE(T)           HSL_LOG(HSL_LOG_IFMGR,HSL_LEVEL_ERROR,"%s:%d Error: Unsupported interface type (%d)",__FILE__,__LINE__,T); 
#define HSL_ERR_REG_2_HSL_IFMGR(S)            HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_FATAL, "(%s:%d) Interface %s registration failed with HSL Interface Manager\n", __FILE__,__LINE__,ifname);
#define HSL_ERR_GET_L2_DST_PORT(I)            HSL_LOG (HSL_LOG_PKTDRV, HSL_LEVEL_ERROR, "(%s:%d)Failed to retrieve destination port for ifindex\n",__FILE__,__LINE__, I);
#define HSL_ERR_ENABLE_VLAN_CLASS(E,P)        HSL_LOG(HSL_LOG_IFMGR,HSL_LEVEL_ERROR,"%s:%d Error: (%d) Failed to enable vlan classification on port (%d)",__FILE__,__LINE__,E,P);
/* Layer 3 - interface */
#define HSL_ERR_ENABLE_ARP_TRAP(E,P)          HSL_LOG(HSL_LOG_IFMGR,HSL_LEVEL_ERROR,"%s:%d Error: (%d) Failed to enable arp trap to cpu on port (%d)",__FILE__,__LINE__,E,P);
#define HSL_ERR_ENABLE_ARP_MIRROR(E)          HSL_LOG(HSL_LOG_IFMGR,HSL_LEVEL_ERROR,"%s:%d Error: (%d) Failed to enable arp mirror to cpu ",__FILE__,__LINE__,E);
#define HSL_ERR_SET_LPORT_MODE(E,P)           HSL_LOG(HSL_LOG_IFMGR,HSL_LEVEL_ERROR,"%s:%d Error: (%d) Failed to set logical port (%d) mode",__FILE__,__LINE__,E,P);
#define HSL_ERR_EN_ICMP_ON_LPORT(E,P)         HSL_LOG(HSL_LOG_IFMGR,HSL_LEVEL_ERROR,"%s:%d Error: (%d) Failed to enable icmp rx on port (%d)",__FILE__,__LINE__,E,P);
#define HSL_ERR_EN_IGMP_ON_LPORT(E,P)         HSL_LOG(HSL_LOG_IFMGR,HSL_LEVEL_ERROR,"%s:%d Error: (%d) Failed to enable igmp rx on port (%d)",__FILE__,__LINE__,E,P);
#define HSL_ERR_EN_IGMP_OFF_LPORT(E,P)         HSL_LOG(HSL_LOG_IFMGR,HSL_LEVEL_ERROR,"%s:%d Error: (%d) Failed to disable igmp rx on port (%d)",__FILE__,__LINE__,E,P);
#define HSL_ERR_BIND_LIF_2_IFINDEX(E,I)       HSL_LOG(HSL_LOG_IFMGR,HSL_LEVEL_ERROR,"%s:%d Error: (%d) Failed to bind ifindex (%d) to logical interface",__FILE__,__LINE__,E,I);
#define HSL_ERR_BIND_IFINDEX_2_VR(E,I,R)      HSL_LOG(HSL_LOG_IFMGR,HSL_LEVEL_ERROR,"%s:%d Error: (%d) Failed to bind ifindex (%d) to VR (%d)",__FILE__,__LINE__,E,I,R);
#define HSL_ERR_ENABLE_UC_ROUTING(E,I)        HSL_LOG(HSL_LOG_IFMGR,HSL_LEVEL_ERROR,"%s:%d Error: (%d) Failed to enable uc routing on interface(%d)",__FILE__,__LINE__,E,I);
#define HSL_ERR_DEL_LOGICAL_IF(E,I)           HSL_LOG(HSL_LOG_IFMGR,HSL_LEVEL_ERROR,"%s:%d Error: (%d) Failed to delete logical interface(%d)",__FILE__,__LINE__,E,I);
#define HSL_ERR_L3_PACKET_SORCE_PORT(P)       HSL_LOG (HSL_LOG_PKTDRV, HSL_LEVEL_INFO, "%s:%d Error:L3 interface not found for the corresponding packet arriving on Lport(%d)\n",__FILE__,__LINE__,P);
#define HSL_ERR_ENABLE_SPECIAL_MCAST(E,N)       HSL_LOG (HSL_LOG_PKTDRV, HSL_LEVEL_INFO, "%s:%d Error: (%d) Failed to enable special multicast on interface (%s)\n",__FILE__,__LINE__,E,N);

/* Trunk */ 
#define HSL_ERR_CREATE_TRUNK(E,T)             HSL_LOG(HSL_LOG_IFMGR,HSL_LEVEL_ERROR,"%s:%d Error: (%d) Trunk (%s) creation failed",__FILE__,__LINE__,E,T);
#define HSL_ERR_DELETE_TRUNK(E,T)             HSL_LOG(HSL_LOG_IFMGR,HSL_LEVEL_ERROR,"%s:%d Error: (%d) Trunk (%d) deletion failed",__FILE__,__LINE__,E,T);
#define HSL_ERR_UPDATE_PORT_TRUNK(E,P,T)      HSL_LOG(HSL_LOG_IFMGR,HSL_LEVEL_ERROR,"%s:%d Error: (%d) Port (%d)membership update int trunk (%s) failed",__FILE__,__LINE__,E,P,T);
#define HSL_ERR_TRUNK_NOT_FOUND(T)            HSL_LOG(HSL_LOG_IFMGR,HSL_LEVEL_ERROR,"%s:%d Trunk (%s) doesn't exist\n",__FILE__,__LINE__,T);
 
/* Ip configuration logs */
#define HSL_ERR_DELETE_PREFIX(E,I,M)          HSL_LOG(HSL_LOG_IFMGR,HSL_LEVEL_ERROR,"%s:%d Error: (%d) Failed to delete prefix entry ip (%x) mask (%x)",__FILE__,__LINE__,E,I,M);
#define HSL_ERR_ADD_PREFIX(E,I,M)           HSL_LOG(HSL_LOG_IFMGR,HSL_LEVEL_ERROR,"%s:%d Error: (%d) Failed to add prefix entry ip (%x) mask (%x)",__FILE__,__LINE__,E,I,M);
#define HSL_ERR_DELETE_PREFIX6(E,I,M)         HSL_LOG(HSL_LOG_IFMGR,HSL_LEVEL_ERROR,"%s:%d Error: (%d) Failed to delete prefix entry ip (%x:%x:%x:%x) mask (%x)",__FILE__,__LINE__,E,I[0],I[1],I[2],I[3],M);
#define HSL_ERR_ADD_PREFIX6(E,I,M)            HSL_LOG(HSL_LOG_IFMGR,HSL_LEVEL_ERROR,"%s:%d Error: (%d) Failed to add prefix entry ip (%x:%x:%x:%x) mask (%x)",__FILE__,__LINE__,E,I[0],I[1],I[2],I[3],M);




/* Router related. */
#define HSL_ERR_ENABLE_VR(E)                  HSL_LOG(HSL_MODULE_ALL,HSL_LEVEL_ERROR,"%s:%d Error: (%d) Failed to enable virtual router",__FILE__,__LINE__,E);

/* Multicast  related. */
#define HSL_ERR_INSTALL_MCAST_FLT(E)          HSL_LOG(HSL_MODULE_ALL,HSL_LEVEL_ERROR,"%s:%d Error: (%d)Failed to install default mcast group and filter",__FILE__,__LINE__,E);
#define HSL_ERR_ENABLE_MCAST_FLT_ON_PORT(E,P) HSL_LOG(HSL_MODULE_ALL,HSL_LEVEL_ERROR,"%s:%d Error: (%d) Failed to enable default mcast filter on  port (%d)",__FILE__,__LINE__,E,P);
#define HSL_ERR_ENABLE_MCAST_RT_ON_IF(E,N)    HSL_LOG (HSL_LOG_FIB, HSL_LEVEL_ERROR, "%s:%d Error: (%d) Failed to enable mc routing on interface (%s)\n",__FILE__,__LINE__,E,N);
#define HSL_ERR_DISABLE_MCAST_RT_ON_IF(E,N)   HSL_LOG (HSL_LOG_FIB, HSL_LEVEL_ERROR, "%s:%d Error: (%d) Failed to disable mc routing on interface (%s)\n",__FILE__,__LINE__,E,N);


/* Vlan related */ 
#define HSL_ERR_VID_CREATION_FAILED(E,V,S)    HSL_LOG(HSL_MODULE_ALL,HSL_LEVEL_ERROR,"%s:%d Error:(%d) Empty vlan: %d creation for STG: %d",__FILE__,__LINE__,E,V,S);
#define HSL_ERR_SET_DEF_VLAN_FAILED(E,V,P)    HSL_LOG(HSL_MODULE_ALL,HSL_LEVEL_ERROR,"%s:%d Error:(%d) Set default vid: %d for lport: %d",__FILE__,__LINE__,E,V,P);
#define HSL_ERR_DEL_VL_FROM_PRT_FAILED(E,V,P) HSL_LOG(HSL_MODULE_ALL,HSL_LEVEL_ERROR,"%s:%d Error:(%d) Del vlan: %d from port: %d",__FILE__,__LINE__,E,V,P); 
#define HSL_ERR_ADD_VL_TO_PRT_FAILED(E,V,P)   HSL_LOG(HSL_MODULE_ALL,HSL_LEVEL_ERROR,"%s:%d Error:(%d) Add vlan: %d to port: %d",__FILE__,__LINE__,E,V,P); 
#define HSL_ERR_ADD_VL_TO_BRG_FAILED(E,V,B)   HSL_LOG(HSL_MODULE_ALL,HSL_LEVEL_ERROR,"%s:%d Error:(%d) Add vlan: %d to bridge %s",__FILE__,__LINE__,E,V,B);
#define HSL_ERR_BIND_VID_STG_FAILED(E,V,S)    HSL_LOG(HSL_MODULE_ALL,HSL_LEVEL_ERROR,"%s:%d Error:(%d) Bind vlan: %d to STG: %d",__FILE__,__LINE__,E,V,S);
#define HSL_ERR_UNBIND_VID_STG_FAILED(E,V)    HSL_LOG(HSL_MODULE_ALL,HSL_LEVEL_ERROR,"%s:%d Error:(%d) Unbind vlan: %d",__FILE__,__LINE__,E,V);
#define HSL_ERR_INIT_VLAN_STACKING(E)         HSL_LOG(HSL_LOG_IFMGR, HSL_LEVEL_ERROR,"%s:%d Error:(%d) Failed to enable vlan stacking",__FILE__,__LINE__,E);
#define HSL_ERR_VID_FIND_FAILED(E,V)    HSL_LOG(HSL_MODULE_ALL,HSL_LEVEL_ERROR,"%s:%d Error:(%d) VLAN ID not present: %d",__FILE__,__LINE__,E,V);
#define HSL_ERR_SID_ADD_FAILED(E,V)     HSL_LOG(HSL_MODULE_ALL,HSL_LEVEL_ERROR,"%s:%d Error:(%d) ADD SID Failed: %d",__FILE__,__LINE__,E,V);
#define HSL_ERR_SID_FIND_FAILED(E,V)     HSL_LOG(HSL_MODULE_ALL,HSL_LEVEL_ERROR,"%s:%d Error:(%d) SID not present: %d",__FILE__,__LINE__,E,V);

/* Port properties  */  
#define HSL_ERR_SET_LEARN_ENABLE(E,P,S)       HSL_LOG(HSL_LOG_IFMGR,HSL_LEVEL_ERROR,"%s:%d Error:(%d) Set port %d learn status to %d",__FILE__,__LINE__,E,P,S);
#define HSL_ERR_UNKNOWN_ADMIN_STATE(S)        HSL_LOG(HSL_LOG_IFMGR,HSL_LEVEL_ERROR,"%s:%d Error: Unknown admin state %d",__FILE__,__LINE__,S);
#define HSL_ERR_ENABLE_LOG_PORT(E,P)          HSL_LOG(HSL_LOG_IFMGR,HSL_LEVEL_ERROR,"%s:%d Error:(%d) Enable logical port  %d",__FILE__,__LINE__,E,P);
#define HSL_ERR_DISABLE_LOG_PORT(E,P)         HSL_LOG(HSL_LOG_IFMGR,HSL_LEVEL_ERROR,"%s:%d Error:(%d) Enable logical port  %d",__FILE__,__LINE__,E,P);
#define HSL_ERR_GET_LINK_STATUS(E,P)          HSL_LOG(HSL_LOG_IFMGR,HSL_LEVEL_ERROR,"%s:%d Error:(%d) Obtain current link status for port %d",__FILE__,__LINE__,E,P);
#define HSL_ERR_SET_LINK_STATUS(E,P)          HSL_LOG(HSL_LOG_IFMGR,HSL_LEVEL_ERROR,"%s:%d Error:(%d) Set link status for port %d",__FILE__,__LINE__,E,P);
#define HSL_ERR_GET_PORT_SPEED(E,P)           HSL_LOG(HSL_LOG_IFMGR,HSL_LEVEL_ERROR,"%s:%d Error:(%d) Get port %d speed",__FILE__,__LINE__,E,P);
#define HSL_ERR_GET_PORT_DUPLEX(E,P)          HSL_LOG(HSL_LOG_IFMGR,HSL_LEVEL_ERROR,"%s:%d Error:(%d) Get port %d duplex",__FILE__,__LINE__,E,P);
#define HSL_ERR_GET_ADMIN_STATUS(E,P)         HSL_LOG(HSL_LOG_IFMGR,HSL_LEVEL_ERROR,"%s:%d Error:(%d) Get port %d admin status",__FILE__,__LINE__,E,P);
#define HSL_ERR_SET_BCAST_SUPPRESION(E,P,R)   HSL_LOG(HSL_LOG_IFMGR,HSL_LEVEL_ERROR,"%s:%d Error:(%d) Set port %d broadcast suppression rate to %d",__FILE__,__LINE__,E,P,R);
#define HSL_ERR_SET_DUPLEX(E,P,D)             HSL_LOG(HSL_LOG_IFMGR,HSL_LEVEL_ERROR,"%s:%d Error (%d) Set port %d duplex (%d)\n",__FILE__,__LINE__,E,P,D);
#define HSL_ERR_SET_SPEED(E,P,D)              HSL_LOG(HSL_LOG_IFMGR,HSL_LEVEL_ERROR,"%s:%d Error (%d) Set port %d speed(%d)\n",__FILE__,__LINE__,E,P,D);
#define HSL_ERR_SET_FLOW_CTRL(E,P,R,T)        HSL_LOG(HSL_LOG_IFMGR,HSL_LEVEL_ERROR,"%s:%d Error (%d) Set port %d flow control rx(%d) tx(%d)\n",__FILE__,__LINE__,E,P,R,T);
#define HSL_ERR_GET_FLOW_CTRL(E,P)            HSL_LOG(HSL_LOG_IFMGR,HSL_LEVEL_ERROR,"%s:%d Error:(%d) Getting flow control for port %d",__FILE__,__LINE__,E,P);
#define HSL_ERR_SET_AUTONEG(E,P,D)             HSL_LOG(HSL_LOG_IFMGR,HSL_LEVEL_ERROR,"%s:%d Error (%d) Set port %d auto negotiation (%d)\n",__FILE__,__LINE__,E,P,D);
/* Mirroring */
#define HSL_ERR_SET_EGR_MIR_PORT(E,P)         HSL_LOG(HSL_LOG_IFMGR,HSL_LEVEL_ERROR,"%s:%d Error:(%d) Setting egress mirroring port %d",__FILE__,__LINE__,E,P);
#define HSL_ERR_SET_ING_MIR_PORT(E,P)         HSL_LOG(HSL_LOG_IFMGR,HSL_LEVEL_ERROR,"%s:%d Error:(%d) Setting inress mirroring port %d",__FILE__,__LINE__,E,P);
#define HSL_ERR_ADD_EGR_MIR_PORT(E,P)         HSL_LOG(HSL_LOG_IFMGR,HSL_LEVEL_ERROR,"%s:%d Error:(%d) Add port %d to egress mirroring group",__FILE__,__LINE__,E,P);
#define HSL_ERR_ADD_ING_MIR_PORT(E,P)         HSL_LOG(HSL_LOG_IFMGR,HSL_LEVEL_ERROR,"%s:%d Error:(%d) Add port %d to ingress mirroring group",__FILE__,__LINE__,E,P);
#define HSL_ERR_DEL_EGR_MIR_PORT(E,P)         HSL_LOG(HSL_LOG_IFMGR,HSL_LEVEL_ERROR,"%s:%d Error:(%d) Del port %d from egress mirroring group",__FILE__,__LINE__,E,P);
#define HSL_ERR_DEL_ING_MIR_PORT(E,P)         HSL_LOG(HSL_LOG_IFMGR,HSL_LEVEL_ERROR,"%s:%d Error:(%d) Del port %d from ingress mirroring group",__FILE__,__LINE__,E,P);

/* STP  port status*/
#define HSL_ERR_SET_PORT_STP_STATE(E,P,G,S)   HSL_LOG(HSL_LOG_IFMGR,HSL_LEVEL_ERROR,"%s:%d Error:(%d) Failed to change port %d STG (%d) state to %d",__FILE__,__LINE__,E,P,G,S);
#define HSL_ERR_GET_PORT_STP_STATE(E,P,V)     HSL_LOG(HSL_LOG_IFMGR,HSL_LEVEL_ERROR,"%s:%d Error:(%d) Failed to read port %d state for vlan %d",__FILE__,__LINE__,E,P,V);
#define HSL_ERR_SET_STP_MODE(E,M)             HSL_LOG(HSL_LOG_IFMGR,HSL_LEVEL_ERROR,"%s:%d Error:(%d) Failed to set stp mode",__FILE__,__LINE__,E,M);

/* MAC entry properties */
#define HSL_ERR_ADD_L2_ENTRY(E,M,V)               HSL_LOG(HSL_MODULE_ALL,HSL_LEVEL_ERROR,"%s:%d Error:(%d) Add mac %x:%x:%x:%x:%x:%x to vid %d ",__FILE__,__LINE__,E,M[0],M[1],M[2],M[3],M[4],M[5],V);
#define HSL_ERR_DEL_L2_ENTRY(E,M,V)               HSL_LOG(HSL_MODULE_ALL,HSL_LEVEL_ERROR,"%s:%d Error:(%d) Delete mac %x:%x:%x:%x:%x:%x from vid %d",__FILE__,__LINE__,E,M[0],M[1],M[2],M[3],M[4],M[5],V);

/* Packet processing logs */ 
#define HSL_ERR_LENGTH_ERROR(L)               HSL_LOG(HSL_LOG_GEN_PKT,HSL_LEVEL_ERROR,"%s:%d Error: Wrong packet length (%d)",__FILE__,__LINE__,L);
#define HSL_ERR_COPY_PACKET(L)                HSL_LOG(HSL_LOG_GEN_PKT,HSL_LEVEL_ERROR,"%s:%d Error: Packet copy error expected length(%d)",__FILE__,__LINE__,L);
#define HSL_ERR_SEND_PACKET(E)                HSL_LOG(HSL_LOG_GEN_PKT,HSL_LEVEL_ERROR,"%s:%d Error:(%d)Send packet error",__FILE__,__LINE__,E);
#define HSL_TRACE_CPU_ERR_CODE(S)             HSL_LOG(HSL_LOG_GEN_PKT,HSL_LEVEL_DEBUG,"CPU error code %s",S);
#define HSL_TRACE_PACKET_INFO(P,D)            HSL_LOG(HSL_LOG_GEN_PKT,HSL_LEVEL_DEBUG,"Packet %d, size %d:\n", P, D);
#define HSL_TRACE_PACKET_DATA(D)              HSL_LOG(HSL_LOG_GEN_PKT,HSL_LEVEL_DEBUG,"%02x ",D);
#define HSL_ERR_INIT_PKT_DRIVER               HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_FATAL, "Error initializing packet driver. \n");
#define HSL_ERR_DEINIT_PKT_DRIVER             HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_FATAL, "Error deinitializing packet driver. \n");
#define HSL_ERR_ALLOCATE_GTBUF                HSL_LOG (HSL_LOG_PKTDRV, HSL_LEVEL_FATAL, "Error allocating marvell packet descriptor.\n");

#define HSL_ERR_ENABLE_MC_ROUTING(E,I)        HSL_LOG(HSL_LOG_FIB, HSL_LEVEL_ERROR,"%s:%d Error: (%d) Failed to enable Multicast routing on interface(%d)",__FILE__,__LINE__,E,I);


/* Filter logs. */
#define HSL_ERR_MAC_FILTER_CREATE(E)          HSL_LOG(HSL_LOG_IFMGR,HSL_LEVEL_ERROR,"%s:%d Error:(%d) Create mac filter",__FILE__,__LINE__,E);

#endif /* _HSL_COMMON_LOGS_H */ 
