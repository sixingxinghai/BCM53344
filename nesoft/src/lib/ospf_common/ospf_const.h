/* Copyright (C) 2002  All Rights Reserved. */

#ifndef _PACOS_OSPF_CONST_H
#define _PACOS_OSPF_CONST_H

/* OSPF protocol number. */
#ifndef IPPROTO_OSPFIGP
#define IPPROTO_OSPFIGP         89
#endif /* IPPROTO_OSPFIGP */

/* Architectual constants. */
#define OSPF_LS_REFRESH_TIME                  1800
#define OSPF_MIN_LS_INTERVAL                     5
#define OSPF_MIN_LS_ARRIVAL                      1
#define OSPF_LSA_MAXAGE                       3600
#define OSPF_CHECK_AGE                         300
#define OSPF_LSA_MAXAGE_DIFF                   900
#define OSPF_AREA_BACKBONE              0x00000000      /* 0.0.0.0. */
#define OSPF_INITIAL_SEQUENCE_NUMBER    0x80000001
#define OSPF_MAX_SEQUENCE_NUMBER        0x7FFFFFFF

/* OSPF Packet type. */
#define OSPF_PACKET_HELLO              1  /* OSPF Hello. */
#define OSPF_PACKET_DB_DESC            2  /* OSPF Database Description. */
#define OSPF_PACKET_LS_REQ             3  /* OSPF Link State Request. */
#define OSPF_PACKET_LS_UPD             4  /* OSPF Link State Update. */
#define OSPF_PACKET_LS_ACK             5  /* OSPF Link State Acknoledgement. */
#define OSPF_PACKET_TYPE_MAX           6

/* OSPF Network Type in struct ospf_interface. */
#define OSPF_IFTYPE_NONE                        0
#define OSPF_IFTYPE_POINTOPOINT                 1
#define OSPF_IFTYPE_BROADCAST                   2
#define OSPF_IFTYPE_NBMA                        3
#define OSPF_IFTYPE_POINTOMULTIPOINT            4
#define OSPF_IFTYPE_POINTOMULTIPOINT_NBMA       5
#define OSPF_IFTYPE_VIRTUALLINK                 6
#define OSPF_IFTYPE_LOOPBACK                    7
#define OSPF_IFTYPE_HOST                        8
#define OSPF_IFTYPE_MAX                         9

/* OSPF area type.  */
#define OSPF_AREA_DEFAULT                       0
#define OSPF_AREA_STUB                          1
#define OSPF_AREA_NSSA                          2
#define OSPF_AREA_TYPE_MAX                      3

/* OSPF ABR type.  */
#define OSPF_ABR_TYPE_UNKNOWN                   0
#define OSPF_ABR_TYPE_STANDARD                  1
#define OSPF_ABR_TYPE_CISCO                     2
#define OSPF_ABR_TYPE_IBM                       3
#define OSPF_ABR_TYPE_SHORTCUT                  4
#define OSPF_ABR_TYPE_MAX                       5
#define OSPF_ABR_TYPE_DEFAULT                   OSPF_ABR_TYPE_CISCO

/* Interface Parameter default values. */
#define OSPF_OUTPUT_COST_DEFAULT                10
#define OSPF_OUTPUT_COST_MIN                    1
#define OSPF_OUTPUT_COST_MAX                    65535
#define OSPF_STUB_DEFAULT_COST_DEFAULT          1
#define OSPF_STUB_DEFAULT_COST_MIN              0
#define OSPF_STUB_DEFAULT_COST_MAX              16777215
#define OSPF_HELLO_INTERVAL_DEFAULT             10
#define OSPF_HELLO_INTERVAL_NBMA_DEFAULT        30
#define OSPF_HELLO_INTERVAL_MIN                 1
#define OSPF_HELLO_INTERVAL_MAX                 65535
#define OSPF_HELLO_INTERVAL_JITTER              0.1
#define OSPF_ROUTER_DEAD_INTERVAL_DEFAULT       40
#define OSPF_ROUTER_DEAD_INTERVAL_NBMA_DEFAULT  120
#define OSPF_ROUTER_DEAD_INTERVAL_MIN           1
#define OSPF_ROUTER_DEAD_INTERVAL_MAX           65535
#define OSPF_ROUTER_PRIORITY_DEFAULT            1
#define OSPF_RETRANSMIT_INTERVAL_DEFAULT        5
#define OSPF_RETRANSMIT_INTERVAL_MIN            5
#define OSPF_RETRANSMIT_INTERVAL_MAX            65535
#define OSPF_RETRANSMIT_INTERVAL_JITTER         0.2
#define OSPF_TRANSMIT_DELAY_DEFAULT             1
#define OSPF_POLL_INTERVAL_DEFAULT              120
#define OSPF_POLL_INTERVAL_MIN                  1
#define OSPF_POLL_INTERVAL_MAX                  65535
#define OSPF_NEIGHBOR_PRIORITY_DEFAULT          0
#if defined (HAVE_OSPF_TE) || defined (HAVE_OSPF6_TE)  
#define OSPF_TE_METRIC_DEFAULT                  0
#endif /* HAVE_OSPF_TE, HAVE_OSPF6_TE */
#ifdef HAVE_RESTART
#define OSPF_RESYNC_TIMEOUT_DEFAULT             40
#endif /* HAVE_RESTART */
#define OSPF_DD_INACTIVITY_TIMER_DEFAULT        120

/* Default bandwidth.  */
#define OSPF_DEFAULT_BANDWIDTH               10000      /* Kbps. */
#define OSPF_DEFAULT_REF_BANDWIDTH          100000      /* Kbps. */

/* SPF timer default values. */
#define OSPF_SPF_MIN_DELAY_DEFAULT             500      /* 500 milliseconds*/
#define OSPF_SPF_MAX_DELAY_DEFAULT           50000      /* 50 seconds*/

#define OSPF_SPF_DELAY_DEFAULT                   5
#define OSPF_SPF_HOLDTIME_DEFAULT               10

/* Area ID format for CLI. */
#define OSPF_AREA_ID_FORMAT_DEFAULT              0
#define OSPF_AREA_ID_FORMAT_ADDRESS              1
#define OSPF_AREA_ID_FORMAT_DECIMAL              2

/* External Metric Type. */
#define EXTERNAL_METRIC_TYPE_UNSPEC              0
#define EXTERNAL_METRIC_TYPE_1                   1
#define EXTERNAL_METRIC_TYPE_2                   2
#define EXTERNAL_METRIC_TYPE_DEFAULT             2

/* Minimum vector slot size. */
#define OSPF_VECTOR_SIZE_MIN                     4
#define OSPF_VECTOR_SIZE_MAX                     256

/* Path type.  */
#define OSPF_PATH_CONNECTED                      0
#define OSPF_PATH_INTRA_AREA                     1
#define OSPF_PATH_INTER_AREA                     2
#define OSPF_PATH_EXTERNAL                       3
#define OSPF_PATH_DISCARD                        4
#define OSPF_PATH_MAX                            5

/*Retransmit List*/
#define OSPF_RETRANSMIT_LISTS			5

#define K_RXMT_INTERVAL_FACTOR          	2   
#define OSPF_UPD_SENT_THRESHOLD         	2000   
#define HIGH_WATER_MARK_PERCENT         	0.75 
#define LOW_WATER_MARK_PERCENT          	0.25 

/* Path flags.  */
#define OSPF_PATH_TYPE2                          (1 << 0)
#define OSPF_PATH_ABR                            (1 << 1)
#define OSPF_PATH_ASBR                           (1 << 2)
#define OSPF_PATH_RANGE_MATCHED                  (1 << 3)

/* Path type code.  */
#define OSPF_PATH_CODE_UNKNOWN                   0
#define OSPF_PATH_CODE_CONNECTED                 1
#define OSPF_PATH_CODE_DISCARD                   2
#define OSPF_PATH_CODE_INTRA_AREA                3
#define OSPF_PATH_CODE_INTER_AREA                4
#define OSPF_PATH_CODE_E1                        5
#define OSPF_PATH_CODE_E2                        6
#define OSPF_PATH_CODE_N1                        7
#define OSPF_PATH_CODE_N2                        8

#define OSPF_SPF_VERTEX_COUNT_THRESHOLD          100
#define OSPF_IA_CALC_COUNT_THRESHOLD             100
#define OSPF_ASE_CALC_EVENT_DELAY                1
#define OSPF_ASE_CALC_COUNT_THRESHOLD            100
#define OSPF_RT_CALC_COUNT_THRESHOLD             100
#define OSPF_LSA_SCAN_COUNT_THRESHOLD 		 10000
#define OSPF_MAX_IA_INCR_PER_SEC                 50
#define OSPF_RT_CLEAN_COUNT_THRESHOLD            500

/* Extern variables. */
extern s_int32_t LSAMaxSeqNumber;
extern struct pal_timeval MinLSInterval;
extern struct pal_timeval MinLSArrival;
extern struct pal_in4_addr OSPFBackboneAreaID;
extern struct pal_in4_addr OSPFRouterIDUnspec;

/* Prototypes. */
void ospf_const_init ();

#endif /* _PACOS_OSPF_CONST_H */
