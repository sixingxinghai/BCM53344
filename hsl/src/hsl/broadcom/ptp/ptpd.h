/**
 * @file   ptpd.h
 * @mainpage Ptpd v2 Documentation
 * @authors Martin Burnicki, Alexandre van Kempen, Steven Kreuzer, 
 *          George Neville-Neil
 * @version 2.0
 * @date   Fri Aug 27 10:22:19 2010
 * 
 * @section implementation Implementation
 * PTTdV2 is not a full implementation of 1588 - 2008 standard.
 * It is implemented only with use of Transparent Clock and Peer delay
 * mechanism, according to 802.1AS requierements.
 * 
 * This header file includes all others headers.
 * It defines functions which are not dependant of the operating system.
 */

#ifndef PTPD_H_
#define PTPD_H_

#include<linux/version.h>

#include "config.h"
#include "hsl_os.h"
#include "hsl_types.h"

/*
   Broadcom includes.
*/
#include "bcm_incl.h"

/*
   HAL includes.
*/
#include "hal_types.h"
#include "hal_l2.h"
#include "hal_msg.h"

/* HSL includes.*/
#include "hsl_types.h"
#include "hsl_logger.h"
#include "hsl_ether.h"
#include "hsl.h"
#include "hsl_ifmgr.h"
#include "hsl_if_hw.h"
#include "hsl_if_os.h"
#include "hsl_bcm_if.h"
#include "hsl_bcm_pkt.h"
//#include "hsl_l2_sock.h"

#include "hal_netlink.h"
#include "hal_socket.h"
#include "hal_msg.h"
#include "hsl_vlan.h"


#include "constants.h"
#include "limits.h"
#include "constants_dep.h"
#include "datatypes_dep.h"
#include "datatypes.h"
#include "ptpd_dep.h"


/** \name arith.c
 * -Timing management and arithmetic*/
 /**\{*/
/* arith.c */

/**
 * \brief Convert Integer64 into TimeInternal structure
 */
void integer64_to_internalTime(Integer64,TimeInternal*);
/**
 * \brief Convert TimeInternal into Timestamp structure (defined by the spec)
 */
void fromInternalTime(TimeInternal*,Timestamp*);

/**
 * \brief Convert Timestamp to TimeInternal structure (defined by the spec)
 */
void toInternalTime(TimeInternal*,Timestamp*);

/**
 * \brief Use to normalize a TimeInternal structure
 *
 * The nanosecondsField member must always be less than 10⁹
 * This function is used after adding or substracting TimeInternal
 */
void normalizeTime(TimeInternal*);

/**
 * \brief Add two InternalTime structure and normalize
 */
void addTime(TimeInternal*,TimeInternal*,TimeInternal*);

/**
 * \brief Substract two InternalTime structure and normalize
 */
void subTime(TimeInternal*,TimeInternal*,TimeInternal*);
/** \}*/

/** \name bmc.c
 * -Best Master Clock Algorithm functions*/
 /**\{*/
/* bmc.c */
/**
 * \brief Compare data set of foreign masters and local data set
 * \return The recommended state for the port
 */
UInteger8 bmc(ForeignMasterRecord*,RunTimeOpts*,PtpClock*);

/**
 * \brief When recommended state is Master, copy local data into parent and grandmaster dataset
 */
void m1(PtpClock*);

/**
 * \brief When recommended state is Slave, copy dataset of master into parent and grandmaster dataset
 */
void s1(MsgHeader*,MsgAnnounce*,PtpClock*);

/**
 * \brief Initialize datas
 */
void initData(RunTimeOpts*,PtpClock*);
/** \}*/


/** \name protocol.c
 * -Execute the protocol engine*/
 /**\{*/
/**
 * \brief Protocol engine
 */
/* protocol.c */


/** \}*/


//Diplay functions usefull to debug
/*void displayRunTimeOpts(RunTimeOpts*);
void displayDefault (PtpClock*);
void displayCurrent (PtpClock*);
void displayParent (PtpClock*);
void displayGlobal (PtpClock*);
void displayPort (PtpClock*);
void displayForeignMaster (PtpClock*);
void displayOthers (PtpClock*);
void displayBuffer (PtpClock*);
void displayPtpClock (PtpClock*);
void timeInternal_display(TimeInternal*);
void clockIdentity_display(ClockIdentity);
void netPath_display(NetPath*);
void intervalTimer_display(IntervalTimer*);
void integer64_display (Integer64*);
void timeInterval_display(TimeInterval*);
void portIdentity_display(PortIdentity*);
void clockQuality_display (ClockQuality*);
void iFaceName_display(Octet*);
void unicast_display(Octet*);*/

void msgHeader_display(MsgHeader*);
void msgAnnounce_display(MsgAnnounce*);
void msgSync_display(MsgSync *sync);
void msgFollowUp_display(MsgFollowUp*);
void msgPDelayReq_display(MsgPDelayReq*);
void msgDelayReq_display(MsgDelayReq * req);
void msgDelayResp_display(MsgDelayResp * resp);
void msgPDelayResp_display(MsgPDelayResp * presp);


void msgUnpackDelayResp(void *,MsgDelayResp *);
void msgPackDelayReq(void *,Timestamp *,PtpClock *);
void msgPackDelayResp(void *,MsgHeader *,Timestamp *,PtpClock *);


void hsl_ptp_set_clock_type(unsigned int type);
void hsl_ptp_create_domain(unsigned int domain);
void hsl_ptp_set_domain_mport(unsigned int domain, unsigned int port);
void hsl_ptp_set_domain_sport(unsigned int domain, unsigned int port);
void hsl_ptp_set_delay_mech(unsigned int delay_mech);
void hsl_ptp_set_phyport(unsigned int vport, unsigned int phyport);
void  hsl_ptp_clear_phyport(unsigned int vport);
void hsl_ptp_enable_port(unsigned int vport);
void hsl_ptp_disable_port(unsigned int vport);
void hsl_ptp_set_port_delay_mech(unsigned int vport, unsigned int delay_mech);
void  hsl_ptp_set_req_delay_interval(unsigned int vport, int interval);
void  hsl_ptp_set_p2p_interval(unsigned int vport, int interval);
void  hsl_ptp_set_p2p_meandelay(unsigned int vport, int interval);
void  hsl_ptp_set_asym_delay(unsigned int vport, int interval);


int hsl_ptp_init();
void ptp_para_init(PtpClock *ptpClock);
void hsl_bcm_rx_handle_ptp( bcm_pkt_t *pkt );
void hsl_ptp_delay_request_send(void);

#endif /*PTPD_H_*/
