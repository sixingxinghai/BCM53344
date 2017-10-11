/* Copyright (C) 2002  All Rights Reserved. */

#ifndef _PACOS_OSPF_FSM_H
#define _PACOS_OSPF_FSM_H

/* OSPF Interface Finite State Machine State. */
#define IFSM_DependUpon                 0
#define IFSM_Down                       1
#define IFSM_Loopback                   2
#define IFSM_Waiting                    3
#define IFSM_PointToPoint               4
#define IFSM_DROther                    5
#define IFSM_Backup                     6
#define IFSM_DR                         7
#define OSPF_IFSM_STATE_MAX             8

/* OSPF Interface Finite State Machine Event. */
#define IFSM_NoEvent                    0
#define IFSM_InterfaceUp                1
#define IFSM_WaitTimer                  2
#define IFSM_BackupSeen                 3
#define IFSM_NeighborChange             4
#define IFSM_LoopInd                    5
#define IFSM_UnloopInd                  6
#define IFSM_InterfaceDown              7
#define OSPF_IFSM_EVENT_MAX             8

/* OSPF Neighbor Finite State Machine State. */
#define NFSM_DependUpon                 0
#define NFSM_Down                       1
#define NFSM_Attempt                    2
#define NFSM_Init                       3
#define NFSM_TwoWay                     4
#define NFSM_ExStart                    5
#define NFSM_Exchange                   6
#define NFSM_Loading                    7
#define NFSM_Full                       8
#define OSPF_NFSM_STATE_MAX             9

#define IS_NFSM_DD_EXCHANGE(S)                                                \
    ((S) > NFSM_TwoWay && (S) < NFSM_Full)

/* OSPF Neighbor Finite State Machine Event. */
#define NFSM_NoEvent                    0
#define NFSM_HelloReceived              1
#define NFSM_Start                      2
#define NFSM_TwoWayReceived             3
#define NFSM_NegotiationDone            4
#define NFSM_ExchangeDone               5
#define NFSM_BadLSReq                   6
#define NFSM_LoadingDone                7
#define NFSM_AdjOK                      8
#define NFSM_SeqNumberMismatch          9
#define NFSM_OneWayReceived            10
#define NFSM_KillNbr                   11
#define NFSM_InactivityTimer           12
#define NFSM_LLDown                    13
#define OSPF_NFSM_EVENT_MAX            14

#endif /* _PACOS_OSPF_FSM_H */

