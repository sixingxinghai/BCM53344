/**
 * @file   protocol.c
 * @date   Wed Jun 23 09:40:39 2010
 * 
 * @brief  The code that handles the IEEE-1588 protocol and state machine
 * 
 * 
 */

#include "ptpd.h"
//#include "ptpd_list.h"
#include "ptp_hash.h"
//#include <stdlib.h>
#include "hsl_bcm.h"




RunTimeOpts rtOpts;			/* statically allocated run-time configuration data */
PtpClock ptpclock[PTP_NUMBER_PORTS];

PtpClock clocktmp;
PtpClock *ptpClock=&clocktmp;

static int portbitmap[256];//配置时钟域(0-255)中的端口，一个时钟域中最多8个端口，一个端口可以同时处于两个时钟域，时钟端口最多为8个，在所有的24个以太网中选择 
static enum em_clock_type clocktype;//暂定不考虑混合型时钟，每个设备只能为oc、bc、tc中的一种

AdjFreqOpts adj_freq_opts[2];
//int g_inport_index;
int sync_receipt_timeout_flag;//端口0 在3秒内没有收到sync报文,则切换,发送端口1报文

Boolean doInit(RunTimeOpts*,PtpClock*);
void doState(RunTimeOpts*,PtpClock*);
void toState(UInteger8,RunTimeOpts*,PtpClock*);

void handle(RunTimeOpts*,PtpClock*);
void handleAnnounce(MsgHeader*,Octet*,ssize_t,Boolean,RunTimeOpts*,PtpClock*,UInteger8 port);
void handleSync(MsgHeader*,Octet*,ssize_t,TimeInternal*,Boolean,RunTimeOpts*,PtpClock*,UInteger8 port);
void handleFollowUp(MsgHeader*,Octet*,ssize_t,Boolean,RunTimeOpts*,PtpClock*,UInteger8 port);
void handlePDelayReq(MsgHeader*,Octet*,ssize_t,TimeInternal*,Boolean,RunTimeOpts*,PtpClock*,UInteger8 port);
void handleDelayReq(MsgHeader*,Octet*,ssize_t,TimeInternal*,Boolean,RunTimeOpts*,PtpClock*,UInteger8 port);
void handlePDelayResp(MsgHeader*,Octet*,TimeInternal* ,ssize_t,Boolean,RunTimeOpts*,PtpClock*,UInteger8 port);
void handleDelayResp(MsgHeader*,Octet*,ssize_t,Boolean,RunTimeOpts*,PtpClock*,UInteger8 port);
void handlePDelayRespFollowUp(MsgHeader*,Octet*,ssize_t,Boolean,RunTimeOpts*,PtpClock*,UInteger8 port);
void handleManagement(MsgHeader*,Octet*,ssize_t,Boolean,RunTimeOpts*,PtpClock*);
void handleSignaling(MsgHeader*,Octet*,ssize_t,Boolean,RunTimeOpts*,PtpClock*);


void issueAnnounce(Octet *msgObuf,RunTimeOpts *rtOpts,PtpClock *ptpClock,int outport_bitmap);
void issueSync(Octet *msgObuf,TimeInternal *time,RunTimeOpts*,PtpClock*,int port);
void issueFollowup(TimeInternal *time,Octet *msgIbuf,MsgHeader * header,RunTimeOpts*,PtpClock*,UInteger8 port);
void issuePDelayReq(RunTimeOpts*,PtpClock*,UInteger8 port);
void issueDelayReq(Octet *msgObuf,TimeInternal *time,RunTimeOpts *rtOpts,PtpClock *ptpClock,int port);
void issuePDelayResp(TimeInternal*,MsgHeader*,RunTimeOpts*,PtpClock*,UInteger8 port);
void issueDelayResp(TimeInternal*,Octet *msgIbuf,MsgHeader*,RunTimeOpts*,PtpClock*,UInteger8 port);
void issuePDelayRespFollowUp(TimeInternal*,MsgHeader*,RunTimeOpts*,PtpClock*,UInteger8 port);
void issueManagement(MsgHeader*,MsgManagement*,RunTimeOpts*,PtpClock*);


void addForeign(Octet*,MsgHeader*,PtpClock*);

//void handleTransparent( RunTimeOpts *rtOpts, PtpClock *ptpClock );
void doTransparent( RunTimeOpts *rtOpts, PtpClock *ptpClock );


/* loop forever. doState() has a switch for the actions and events to be
   checked for 'port_state'. the actions and events may or may not change
   'port_state' by calling toState(), but once they are done we loop around
   again and perform the actions required for the new 'port_state'. */
static void 
_hsl_bcm_ptp_handler_thread()
{
	//DBG("event POWERUP\n");
	//HSL_LOG (HSL_LOG_PKTDRV, HSL_LEVEL_INFO, "ptp test:inside function protocol()!\n");
	/*PtpClock *ptpClock;
	ptpClock = oss_malloc(sizeof (PtpClock), OSS_MEM_HEAP);
	if( ptpClock == NULL )
		HSL_LOG (HSL_LOG_PKTDRV, HSL_LEVEL_INFO, "ptpclock alloc error!\n");
	memset( ptpClock, 0, sizeof (PtpClock));*/


	if( clocktype == PTP_TRANSPARENT)
	{
	   while(1)
	   	{
	   	  //doTransparent(&rtOpts, ptpClock);

		  //if (timerExpired(SYNC_INTERVAL_TIMER, ptpClock->itimer))
		  	
		  //sal_usleep(1);
		  sal_usleep(1000*1000);
		  sync_receipt_timeout_flag++;
	   	}
	}
	
	if( clocktype == PTP_ORDINARY )
	{
	   toState(PTP_INITIALIZING, &rtOpts, ptpClock);
	
	   //DBGV("Debug Initializing...");
       while(1)
	   //for(;;)
	   {
		 if(ptpClock->portState != PTP_INITIALIZING)
			doState(&rtOpts, ptpClock);
		 else if(!doInit(&rtOpts, ptpClock))
		  {
			oss_free( ptpClock, OSS_MEM_HEAP);
			return;
		   }
		 sal_usleep(1);
		//usleep(1);
		//if(ptpClock->message_activity)
		//DBGV("activity\n");
		//else
			//DBGV("no activity\n");
	   }
	}
}

void doTransparent( RunTimeOpts *rtOpts, PtpClock *ptpClock )
{
    TimeInternal time;
	time.nanoseconds = 0;
	//if (rtOpts->E2E_mode)
		//timerStop(DELAYREQ_INTERVAL_TIMER, ptpClock->itimer);
	//else
	//timerStop(PDELAYREQ_INTERVAL_TIMER, ptpClock->itimer);

   //	timerStart(PDELAYREQ_INTERVAL_TIMER, 
		//pow(2,ptpClock->logMinPdelayReqInterval), 
		//ptpClock->itimer);
	


		//handleTransparent(rtOpts, ptpClock);
		//HSL_LOG (HSL_LOG_PKTDRV, HSL_LEVEL_INFO, "ptp test:outside function handle()!\n");
		
		/*if (rtOpts->E2E_mode) {
			if(timerExpired(DELAYREQ_INTERVAL_TIMER,
					ptpClock->itimer)) {
				//DBGV("event DELAYREQ_INTERVAL_TIMEOUT_EXPIRES\n");
				//issueDelayReq(rtOpts,ptpClock);
			}
		} */
			//if(timerExpired(PDELAYREQ_INTERVAL_TIMER,
					//ptpClock->itimer)) {
					int i;
					for(i = 0;i<PTP_MASTER_PORT_NUMBER;i++) 
                        {  	
                           if( ptpclock[i].enable == PTP_TRUE && ptpclock[i].isMasterPort == PTP_TRUE && ptpclock[i].delayMechanism == P2P && ptpclock[i].phy_port != -1 )
				           issuePDelayReq(rtOpts,ptpClock,ptpclock[i].phy_port );
						}
			//HSL_LOG (HSL_LOG_PKTDRV, HSL_LEVEL_INFO, "port 12's pdelay request send!\n");
			    //sal_usleep(5*1000);
				//issuePDelayReq(rtOpts,ptpClock,13);
				//HSL_LOG (HSL_LOG_PKTDRV, HSL_LEVEL_INFO, "port 13's pdelay request send!\n");
				//sal_usleep(500*1000);
			    //issueSync(NULL, &time, rtOpts, ptpClock,14);
				//HSL_LOG (HSL_LOG_PKTDRV, HSL_LEVEL_INFO, "port 14's sync send!\n");
				//sal_usleep(5*1000);
				//issueFollowup(&time,rtOpts,ptpClock,14);
				//sal_usleep(5*1000);
				//HSL_LOG (HSL_LOG_PKTDRV, HSL_LEVEL_INFO, "port 14's sync followup send!\n");
			//}
		
	
}

void hsl_ptp_delay_request_send(void)
{
    int i;
	for(i = 0;i<PTP_MASTER_PORT_NUMBER;i++) 
    {  	
        if( ptpclock[i].enable == PTP_TRUE && ptpclock[i].isMasterPort == PTP_TRUE && ptpclock[i].delayMechanism == P2P && ptpclock[i].phy_port != -1 )
		issuePDelayReq(&rtOpts,ptpClock,ptpclock[i].phy_port );
	}
}

void hsl_bcm_rx_handle_ptp( bcm_pkt_t *pkt )
{
    //HSL_LOG (HSL_LOG_PKTDRV, HSL_LEVEL_INFO, "ptp test:inside function handle()!\n");
	//int ret;
	ssize_t length;
	Boolean isFromSelf;
	char *p = NULL;//u_char
	TimeInternal time = { 0, 0 };
	UInteger8 port = (UInteger8)pkt->rx_port - 2;
	PtpClock *ptpClock;
	int i, inport_index;
	for(i = 0;i<PTP_NUMBER_PORTS;i++) 
    {  	
       if( ptpclock[i].phy_port == port)
		  inport_index = i;
    }
    ptpClock = &ptpclock[i];

	struct ptp_msg
    {
      struct hsl_eth_header eth_header;
	  char ptp_header;
	};
	struct ptp_msg *msg_tmp=NULL;
	char *header_tmp=NULL;
	
	    p = BCM_PKT_DMAC (pkt);
	    msg_tmp = (struct ptp_msg *)p;
	    header_tmp = (char *)&(msg_tmp->ptp_header);
	    length = BCM_PKT_IEEE_LEN (pkt)-22;
        memcpy( ptpClock->msgIbuf, header_tmp,length);
	    time.nanoseconds = pkt->rx_timestamp;    
	    time.seconds += ptpClock->currentUtcOffset;



	if(length < HEADER_LENGTH) {
		//ERROR("message shorter than header length\n");
		//HSL_LOG (HSL_LOG_PKTDRV, HSL_LEVEL_INFO, "ptp test:inside function handle()7.5!\n");
		//toState(PTP_FAULTY, rtOpts, ptpClock);
		return;
	}
	msgUnpackHeader( header_tmp, &ptpClock->msgTmpHeader);
	//HSL_LOG (HSL_LOG_PKTDRV, HSL_LEVEL_INFO, "ptp test:the receive port is %d!\n",port);
	//HSL_LOG (HSL_LOG_PKTDRV, HSL_LEVEL_INFO, "ptp test:portID is %d!\n",ptpClock->msgTmpHeader.sourcePortIdentity.portNumber);

	/*HSL_LOG (HSL_LOG_PKTDRV, HSL_LEVEL_INFO, "ptp msg:eth->d.type is 0x%04X,eth->dmac is 0x%02x%02x%02x%02x%02x%02x\n",msg_tmp->eth_header.d.vlan.type,
		msg_tmp->eth_header.dmac[0],msg_tmp->eth_header.dmac[1],msg_tmp->eth_header.dmac[2],msg_tmp->eth_header.dmac[3],msg_tmp->eth_header.dmac[4],msg_tmp->eth_header.dmac[5]);//djg*/
  //HSL_LOG (HSL_LOG_PKTDRV, HSL_LEVEL_INFO, "ptp msg:the packet receive time stamp is %d\n",pkt->rx_timestamp);
  /*HSL_LOG (HSL_LOG_PKTDRV, HSL_LEVEL_INFO, "ptp msg:the packet receive block count is %d\n",pkt->blk_count);
  HSL_LOG (HSL_LOG_PKTDRV, HSL_LEVEL_INFO, "ptp msg:the packet receive first block data len is %d\n",pkt->pkt_data[0].len);
  if (pkt->blk_count>=2) HSL_LOG (HSL_LOG_PKTDRV, HSL_LEVEL_INFO, "test1:the packet receive second block data len is %d\n",pkt->pkt_data[1].len);
  HSL_LOG (HSL_LOG_PKTDRV, HSL_LEVEL_INFO, "ptp msg:the packet content is");
  //q = msg_tmp;
  q = header_tmp;
  //int lentemp = length + 22;
  int lentemp= length +4;//printf CRC
  while(lentemp>0)
  	{
        HSL_LOG (HSL_LOG_PKTDRV, HSL_LEVEL_INFO, " 0x%02x",*q);
		lentemp --;
		q ++;
  	}
  HSL_LOG (HSL_LOG_PKTDRV, HSL_LEVEL_INFO, "\n");*/
  
    //HSL_LOG (HSL_LOG_PKTDRV, HSL_LEVEL_INFO, "ptp test:inside function handle()7.6!%d %2x %2x\n",sizeof(struct hsl_eth_header),*header_tmp,*header_tmp++);
	

	if(ptpClock->msgTmpHeader.versionPTP != ptpClock->versionNumber) {
		//DBGV("ignore version %d message\n", 
		     //ptpClock->msgTmpHeader.versionPTP);
		     //HSL_LOG (HSL_LOG_PKTDRV, HSL_LEVEL_INFO, "ptp test:inside function handle()8.5!\n");
		return;
	}

	/*if(ptpClock->msgTmpHeader.domainNumber != ptpClock->domainNumber) {//ptpClock->domainNumber
		//DBGV("ignore message from domainNumber %d\n", 
		     //ptpClock->msgTmpHeader.domainNumber);
		     HSL_LOG (HSL_LOG_PKTDRV, HSL_LEVEL_INFO, "ptp test:inside function handle()8.6!header domain number %d,ptpclock domain number %d.",ptpClock->msgTmpHeader.domainNumber,ptpClock->domainNumber);
		return;
	}*/

	//Spec 9.5.2.2	
	isFromSelf = (ptpClock->portIdentity.portNumber == ptpClock->msgTmpHeader.sourcePortIdentity.portNumber
		      && !memcmp(ptpClock->msgTmpHeader.sourcePortIdentity.clockIdentity, ptpClock->portIdentity.clockIdentity, CLOCK_IDENTITY_LENGTH));

	 
	 //subtract the inbound latency adjustment if it is not a loop
	 //back and the time stamp seems reasonable 
	 
	//if(!isFromSelf && time.seconds > 0)
		//subTime(&time, &time, &rtOpts->inboundLatency);
	//HSL_LOG (HSL_LOG_PKTDRV, HSL_LEVEL_INFO, "ptp test:inside function handle()9!\n");

	switch(ptpClock->msgTmpHeader.messageType)
	{
	case ANNOUNCE:
		handleAnnounce(&ptpClock->msgTmpHeader, ptpClock->msgIbuf, 
			       length, isFromSelf, &rtOpts, ptpClock, port);
		//HSL_LOG (HSL_LOG_PKTDRV, HSL_LEVEL_INFO, "port %d ptp test:inside function handle()10!\n",port);
		break;
	case SYNC:
		//HSL_LOG (HSL_LOG_PKTDRV, HSL_LEVEL_INFO, "port %d's sync receive timestamp is %d!\n",port,time.nanoseconds);
		handleSync(&ptpClock->msgTmpHeader, header_tmp, 
			   length, &time, isFromSelf, &rtOpts, ptpClock, port);
		//HSL_LOG (HSL_LOG_PKTDRV, HSL_LEVEL_INFO, "port %d ptp test:inside function handle()11!\n",port);
		break;
	case FOLLOW_UP:
		//HSL_LOG (HSL_LOG_PKTDRV, HSL_LEVEL_INFO, "ptp test:inside function handle()12!1\n");
		handleFollowUp(&ptpClock->msgTmpHeader, header_tmp, 
			       length, isFromSelf, &rtOpts, ptpClock, port);
		//HSL_LOG (HSL_LOG_PKTDRV, HSL_LEVEL_INFO, "port %d ptp test:inside function handle()12!\n",port);
		/*MsgFollowUp follow1;
		msgUnpackFollowUp(header_tmp,&follow1);
		if(port == 15)
		HSL_LOG (HSL_LOG_PKTDRV, HSL_LEVEL_INFO, "port %d's  sync followup correction field is %d!\n",port,follow1.preciseOriginTimestamp.nanosecondsField);*/
		break;
	case DELAY_REQ:
		handleDelayReq(&ptpClock->msgTmpHeader, header_tmp, length, &time, isFromSelf, &rtOpts, ptpClock, port);
		//HSL_LOG (HSL_LOG_PKTDRV, HSL_LEVEL_INFO, "ptp test:inside function handle()13!\n");
		break;
	case PDELAY_REQ:
		handlePDelayReq(&ptpClock->msgTmpHeader, ptpClock->msgIbuf, 
				length, &time, isFromSelf, &rtOpts, ptpClock, port);
		//HSL_LOG (HSL_LOG_PKTDRV, HSL_LEVEL_INFO, "port %d's pdelay request timestamp is %d!\n",port,time.nanoseconds);
		break;  
	case DELAY_RESP:
		handleDelayResp(&ptpClock->msgTmpHeader, header_tmp, length, isFromSelf, &rtOpts, ptpClock, port);
		//HSL_LOG (HSL_LOG_PKTDRV, HSL_LEVEL_INFO, "ptp test:inside function handle()15!\n");
		break;
	case PDELAY_RESP:
		handlePDelayResp(&ptpClock->msgTmpHeader, ptpClock->msgIbuf,
				 &time, length, isFromSelf, &rtOpts, ptpClock, port);
		//HSL_LOG (HSL_LOG_PKTDRV, HSL_LEVEL_INFO, "ptp test:inside function handle()16!\n");
		break;
	case PDELAY_RESP_FOLLOW_UP:
		handlePDelayRespFollowUp(&ptpClock->msgTmpHeader, 
					 ptpClock->msgIbuf, length, 
					 isFromSelf, &rtOpts, ptpClock, port);
		//HSL_LOG (HSL_LOG_PKTDRV, HSL_LEVEL_INFO, "ptp test:inside function handle()17!\n");
		break;
	case MANAGEMENT:
		handleManagement(&ptpClock->msgTmpHeader, ptpClock->msgIbuf, 
				 length, isFromSelf, &rtOpts, ptpClock);
		HSL_LOG (HSL_LOG_PKTDRV, HSL_LEVEL_INFO, "ptp test:inside function handle()18!\n");
		break;
	case SIGNALING:
		handleSignaling(&ptpClock->msgTmpHeader, ptpClock->msgIbuf, 
				length, isFromSelf, &rtOpts, ptpClock);
		HSL_LOG (HSL_LOG_PKTDRV, HSL_LEVEL_INFO, "ptp test:inside function handle()19!\n");
		break;
	default:
		//DBG("handle: unrecognized message\n");
		//HSL_LOG (HSL_LOG_PKTDRV, HSL_LEVEL_INFO, "ptp test:inside function handle()20!\n");
		break;
	}
}


/* perform actions required when leaving 'port_state' and entering 'state' */
void 
toState(UInteger8 state, RunTimeOpts *rtOpts, PtpClock *ptpClock)
{
	//HSL_LOG (HSL_LOG_PKTDRV, HSL_LEVEL_INFO, "ptp test:inside function tostate()!\n");
	ptpClock->message_activity = PTP_TRUE;
	
	/* leaving state tasks */
	switch(ptpClock->portState)
	{
	case PTP_MASTER:
		timerStop(SYNC_INTERVAL_TIMER, ptpClock->itimer);  
		timerStop(ANNOUNCE_INTERVAL_TIMER, ptpClock->itimer);
		timerStop(PDELAYREQ_INTERVAL_TIMER, ptpClock->itimer); 
		break;
		
	case PTP_SLAVE:
		timerStop(ANNOUNCE_RECEIPT_TIMER, ptpClock->itimer);
		
		if (rtOpts->E2E_mode)
			timerStop(DELAYREQ_INTERVAL_TIMER, ptpClock->itimer);
		else
			timerStop(PDELAYREQ_INTERVAL_TIMER, ptpClock->itimer);
		
		initClock(rtOpts, ptpClock); 
		break;
		
	case PTP_PASSIVE:
		timerStop(PDELAYREQ_INTERVAL_TIMER, ptpClock->itimer);
		timerStop(ANNOUNCE_RECEIPT_TIMER, ptpClock->itimer);
		break;
		
	case PTP_LISTENING:
		timerStop(ANNOUNCE_RECEIPT_TIMER, ptpClock->itimer);
		break;
		
	default:
		break;
	}
	
	/* entering state tasks */

	/*
	 * No need of PRE_MASTER state because of only ordinary clock
	 * implementation.
	 */
	
	switch(state)
	{
	case PTP_INITIALIZING:
		//DBG("state PTP_INITIALIZING\n");
		ptpClock->portState = PTP_INITIALIZING;
		break;
		
	case PTP_FAULTY:
		//DBG("state PTP_FAULTY\n");
		ptpClock->portState = PTP_FAULTY;
		break;
		
	case PTP_DISABLED:
		//DBG("state PTP_DISABLED\n");
		ptpClock->portState = PTP_DISABLED;
		break;
		
	case PTP_LISTENING:
		//DBG("state PTP_LISTENING\n");
		//timerStart(ANNOUNCE_RECEIPT_TIMER, 
			   //(ptpClock->announceReceiptTimeout) * 
			   //(pow(2,ptpClock->logAnnounceInterval)), 
			   //ptpClock->itimer);
		ptpClock->portState = PTP_LISTENING;
		break;

	case PTP_MASTER:
		//DBG("state PTP_MASTER\n");
		
		//timerStart(SYNC_INTERVAL_TIMER, 
			   //pow(2,ptpClock->logSyncInterval), ptpClock->itimer);
		//DBG("SYNC INTERVAL TIMER : %f \n",
		    //pow(2,ptpClock->logSyncInterval));
		//timerStart(ANNOUNCE_INTERVAL_TIMER, 
			  //pow(2,ptpClock->logAnnounceInterval), 
			  // ptpClock->itimer);
		//timerStart(PDELAYREQ_INTERVAL_TIMER, 
			  // pow(2,ptpClock->logMinPdelayReqInterval), 
			  // ptpClock->itimer);
		ptpClock->portState = PTP_MASTER;
		break;

	case PTP_PASSIVE:
		//DBG("state PTP_PASSIVE\n");
		
		//timerStart(PDELAYREQ_INTERVAL_TIMER, 
			  // pow(2,ptpClock->logMinPdelayReqInterval), 
			  // ptpClock->itimer);
	//	timerStart(ANNOUNCE_RECEIPT_TIMER, 
			   //(ptpClock->announceReceiptTimeout) * 
			  // (pow(2,ptpClock->logAnnounceInterval)), 
			 //  ptpClock->itimer);
		ptpClock->portState = PTP_PASSIVE;
		break;

	case PTP_UNCALIBRATED:
		//DBG("state PTP_UNCALIBRATED\n");
		ptpClock->portState = PTP_UNCALIBRATED;
		break;

	case PTP_SLAVE:
		//DBG("state PTP_SLAVE\n");
		initClock(rtOpts, ptpClock);
		
		ptpClock->waitingForFollow = PTP_FALSE;
		ptpClock->pdelay_req_send_time.seconds = 0;
		ptpClock->pdelay_req_send_time.nanoseconds = 0;
		ptpClock->pdelay_req_receive_time.seconds = 0;
		ptpClock->pdelay_req_receive_time.nanoseconds = 0;
		ptpClock->pdelay_resp_send_time.seconds = 0;
		ptpClock->pdelay_resp_send_time.nanoseconds = 0;
		ptpClock->pdelay_resp_receive_time.seconds = 0;
		ptpClock->pdelay_resp_receive_time.nanoseconds = 0;
		
		
		//timerStart(ANNOUNCE_RECEIPT_TIMER,
			   //(ptpClock->announceReceiptTimeout) * 
			  // (pow(2,ptpClock->logAnnounceInterval)), 
			 // ptpClock->itimer);
		
		/*if (rtOpts->E2E_mode)
			timerStart(DELAYREQ_INTERVAL_TIMER, 
				   pow(2,ptpClock->logMinDelayReqInterval), 
				   ptpClock->itimer);
		else
			timerStart(PDELAYREQ_INTERVAL_TIMER, 
				   pow(2,ptpClock->logMinPdelayReqInterval), 
				   ptpClock->itimer);

		ptpClock->portState = PTP_SLAVE;*/
		break;

	default:
		//DBG("to unrecognized state\n");
		break;
	}

	if(rtOpts->displayStats)
		displayStats(rtOpts, ptpClock);
}


Boolean 
doInit(RunTimeOpts *rtOpts, PtpClock *ptpClock)
{
	//DBG("manufacturerIdentity: %s\n", MANUFACTURER_ID);
	
	/* initialize networking */
	//netShutdown(&ptpClock->netPath);
	/*if(!netInit(&ptpClock->netPath, rtOpts, ptpClock)) {
		ERROR("failed to initialize network\n");
		toState(PTP_FAULTY, rtOpts, ptpClock);
		return PTP_FALSE;
	}*/
	
	/* initialize other stuff */
	initData(rtOpts, ptpClock);
	initTimer();
	initClock(rtOpts, ptpClock);
	m1(ptpClock);
	msgPackHeader(ptpClock->msgObuf, ptpClock);
	
	toState(PTP_LISTENING, rtOpts, ptpClock);
	
	return PTP_TRUE;
}

/* handle actions and events for 'port_state' */
void 
doState(RunTimeOpts *rtOpts, PtpClock *ptpClock)
{
    //HSL_LOG (HSL_LOG_PKTDRV, HSL_LEVEL_INFO, "ptp test:inside function dostate()!\n");
	UInteger8 state;
	TimeInternal time;
	
	ptpClock->message_activity = PTP_FALSE;
	
	switch(ptpClock->portState)
	{
	case PTP_LISTENING:
	case PTP_PASSIVE:
	case PTP_SLAVE:
		
	case PTP_MASTER:
		/*State decision Event*/
		if(ptpClock->record_update)
		{
			//DBGV("event STATE_DECISION_EVENT\n");
			ptpClock->record_update = PTP_FALSE;
			state = bmc(ptpClock->foreign, rtOpts, ptpClock);
			if(state != ptpClock->portState)
				toState(state, rtOpts, ptpClock);
		}
		break;
		
	default:
		break;
	}
	
	switch(ptpClock->portState)
	{
	case PTP_FAULTY:
		/* imaginary troubleshooting */
		
		//DBG("event FAULT_CLEARED\n");
		toState(PTP_INITIALIZING, rtOpts, ptpClock);
		return;
		
	case PTP_LISTENING:
	case PTP_PASSIVE:
	case PTP_UNCALIBRATED:
	case PTP_SLAVE:
		
		handle(rtOpts, ptpClock);
		//HSL_LOG (HSL_LOG_PKTDRV, HSL_LEVEL_INFO, "ptp test:outside function handle()!\n");
		if(timerExpired(ANNOUNCE_RECEIPT_TIMER, ptpClock->itimer))  
		{
			//DBGV("event ANNOUNCE_RECEIPT_TIMEOUT_EXPIRES\n");
			ptpClock->number_foreign_records = 0;
			ptpClock->foreign_record_i = 0;
			if(!ptpClock->slaveOnly && 
			   ptpClock->clockQuality.clockClass != 255) {
				m1(ptpClock);
				toState(PTP_MASTER, rtOpts, ptpClock);
			} else if(ptpClock->portState != PTP_LISTENING)
				toState(PTP_LISTENING, rtOpts, ptpClock);
		}
		
		if (rtOpts->E2E_mode) {
			if(timerExpired(DELAYREQ_INTERVAL_TIMER,
					ptpClock->itimer)) {
				//DBGV("event DELAYREQ_INTERVAL_TIMEOUT_EXPIRES\n");
				//issueDelayReq(rtOpts,ptpClock);
			}
		} else {
			if(timerExpired(PDELAYREQ_INTERVAL_TIMER,
					ptpClock->itimer)) {
				//DBGV("event PDELAYREQ_INTERVAL_TIMEOUT_EXPIRES\n");
				issuePDelayReq(rtOpts,ptpClock,14);
			}
		}
		break;

	case PTP_MASTER:
		if(timerExpired(SYNC_INTERVAL_TIMER, ptpClock->itimer)) {
			//DBGV("event SYNC_INTERVAL_TIMEOUT_EXPIRES\n");
			issueSync(ptpClock->msgIbuf,&time,rtOpts, ptpClock,14);
		}
		
		if(timerExpired(ANNOUNCE_INTERVAL_TIMER, ptpClock->itimer)) {
			//DBGV("event ANNOUNCE_INTERVAL_TIMEOUT_EXPIRES\n");
			issueAnnounce(ptpClock->msgIbuf,rtOpts, ptpClock,21);
		}
		
		if (!rtOpts->E2E_mode) {
			if(timerExpired(PDELAYREQ_INTERVAL_TIMER,
					ptpClock->itimer)) {
				//DBGV("event PDELAYREQ_INTERVAL_TIMEOUT_EXPIRES\n");
				issuePDelayReq(rtOpts,ptpClock,14);
			}
		}
		
		handle(rtOpts, ptpClock);
		
		if(ptpClock->slaveOnly || 
		   ptpClock->clockQuality.clockClass == 255)
			toState(PTP_LISTENING, rtOpts, ptpClock);
		
		break;

	case PTP_DISABLED:
		handle(rtOpts, ptpClock);
		break;
		
	default:
		//DBG("(doState) do unrecognized state\n");
		break;
	}
}

 
/* check and handle received messages */
void 
handle(RunTimeOpts *rtOpts, PtpClock *ptpClock)
{
    //HSL_LOG (HSL_LOG_PKTDRV, HSL_LEVEL_INFO, "ptp test:inside function handle()!\n");
	//int ret;
	ssize_t length;
	Boolean isFromSelf;
	bcm_pkt_t *pkt;
	UInteger8 port = (UInteger8)pkt->rx_port;
	//u_char *p = NULL,*q = NULL;
	TimeInternal time = { 0, 0 };
  
	if(!ptpClock->message_activity)	{
		/*ret = netSelect(0, &ptpClock->netPath);
		if(ret < 0) {
			PERROR("failed to poll sockets");
			toState(PTP_FAULTY, rtOpts, ptpClock);
			return;
		} else if(!ret) {
			DBGV("handle: nothing\n");
			return;
		}*/
		/* else length > 0 */
	}
    //HSL_LOG (HSL_LOG_PKTDRV, HSL_LEVEL_INFO, "ptp test:inside function handle()1!\n");
	//DBGV("handle: something\n");

	//hsl_ptp_pkt_get( pkt );
	//HSL_LOG (HSL_LOG_PKTDRV, HSL_LEVEL_INFO, "ptp test:inside function handle()2!\n");
	struct ptp_msg
    {
      struct hsl_eth_header eth_header;
	  MsgHeader ptp_header;
	};
	struct ptp_msg *msg_tmp;
	MsgHeader *header_tmp;
	
	if(pkt == NULL) length=-1;
	else
	  {
	    char *p = BCM_PKT_DMAC (pkt);
	   // HSL_LOG (HSL_LOG_PKTDRV, HSL_LEVEL_INFO, "ptp test:inside function handle()3!\n");
	    msg_tmp = (struct ptp_msg *)p;
	    header_tmp = &msg_tmp->ptp_header;
	    char *q = ptpClock->msgIbuf;
	    //HSL_LOG (HSL_LOG_PKTDRV, HSL_LEVEL_INFO, "ptp test:inside function handle()4!\n");
	    int lentmp = pkt->pkt_data[0].len - 22;
	    while( lentmp-- >0)
	    {
          *q = *(p+18);
	      q++;
	      p++;
	    }
	    //HSL_LOG (HSL_LOG_PKTDRV, HSL_LEVEL_INFO, "ptp test:inside function handle()5!\n");
        length = BCM_PKT_IEEE_LEN (pkt)-22;
	    time.nanoseconds = pkt->rx_timestamp;
	    //HSL_LOG (HSL_LOG_PKTDRV, HSL_LEVEL_INFO, "ptp test:inside function handle()6!\n");

	    time.seconds += ptpClock->currentUtcOffset;
		}
	
	if(length < 0) {
		//PERROR("failed to receive on the event socket");
		toState(PTP_FAULTY, rtOpts, ptpClock);
		return;
	} 
	//HSL_LOG (HSL_LOG_PKTDRV, HSL_LEVEL_INFO, "ptp test:inside function handle()7!\n");
  
	ptpClock->message_activity = PTP_TRUE;

	if(length < HEADER_LENGTH) {
		//ERROR("message shorter than header length\n");
		toState(PTP_FAULTY, rtOpts, ptpClock);
		return;
	}
  
	msgUnpackHeader(ptpClock->msgIbuf, &ptpClock->msgTmpHeader);
	//HSL_LOG (HSL_LOG_PKTDRV, HSL_LEVEL_INFO, "ptp test:inside function handle()8!\n");

	if(ptpClock->msgTmpHeader.versionPTP != ptpClock->versionNumber) {
		//DBGV("ignore version %d message\n", 
		     //ptpClock->msgTmpHeader.versionPTP);
		return;
	}

	if(ptpClock->msgTmpHeader.domainNumber != ptpClock->domainNumber) {
		//DBGV("ignore message from domainNumber %d\n", 
		     //ptpClock->msgTmpHeader.domainNumber);
		return;
	}

	/*Spec 9.5.2.2*/	
	isFromSelf = (ptpClock->portIdentity.portNumber == ptpClock->msgTmpHeader.sourcePortIdentity.portNumber
		      && !memcmp(ptpClock->msgTmpHeader.sourcePortIdentity.clockIdentity, ptpClock->portIdentity.clockIdentity, CLOCK_IDENTITY_LENGTH));

	/* 
	 * subtract the inbound latency adjustment if it is not a loop
	 *  back and the time stamp seems reasonable 
	 */
	if(!isFromSelf && time.seconds > 0)
		subTime(&time, &time, &rtOpts->inboundLatency);
	//HSL_LOG (HSL_LOG_PKTDRV, HSL_LEVEL_INFO, "ptp test:inside function handle()9!\n");

	switch(ptpClock->msgTmpHeader.messageType)
	{
	case ANNOUNCE:
		handleAnnounce(&ptpClock->msgTmpHeader, ptpClock->msgIbuf, 
			       length, isFromSelf, rtOpts, ptpClock,port);
		HSL_LOG (HSL_LOG_PKTDRV, HSL_LEVEL_INFO, "ptp test:inside function handle()10!\n");
		break;
	case SYNC:
		handleSync(&ptpClock->msgTmpHeader, ptpClock->msgIbuf, 
			   length, &time, isFromSelf, rtOpts, ptpClock, port);
		HSL_LOG (HSL_LOG_PKTDRV, HSL_LEVEL_INFO, "ptp test:inside function handle()11!\n");
		break;
	case FOLLOW_UP:
		handleFollowUp(&ptpClock->msgTmpHeader, ptpClock->msgIbuf, 
			       length, isFromSelf, rtOpts, ptpClock, port);
		HSL_LOG (HSL_LOG_PKTDRV, HSL_LEVEL_INFO, "ptp test:inside function handle()12!\n");
		break;
	case DELAY_REQ:
		handleDelayReq(&ptpClock->msgTmpHeader, ptpClock->msgIbuf, 
			       length, &time, isFromSelf, rtOpts, ptpClock, port);
		HSL_LOG (HSL_LOG_PKTDRV, HSL_LEVEL_INFO, "ptp test:inside function handle()13!\n");
		break;
	case PDELAY_REQ:
		handlePDelayReq(&ptpClock->msgTmpHeader, ptpClock->msgIbuf, 
				length, &time, isFromSelf, rtOpts, ptpClock, port);
		HSL_LOG (HSL_LOG_PKTDRV, HSL_LEVEL_INFO, "ptp test:inside function handle()14!\n");
		break;  
	case DELAY_RESP:
		handleDelayResp(&ptpClock->msgTmpHeader, ptpClock->msgIbuf, 
				length, isFromSelf, rtOpts, ptpClock, port);
		HSL_LOG (HSL_LOG_PKTDRV, HSL_LEVEL_INFO, "ptp test:inside function handle()15!\n");
		break;
	case PDELAY_RESP:
		handlePDelayResp(&ptpClock->msgTmpHeader, ptpClock->msgIbuf,
				 &time, length, isFromSelf, rtOpts, ptpClock, port);
		HSL_LOG (HSL_LOG_PKTDRV, HSL_LEVEL_INFO, "ptp test:inside function handle()16!\n");
		break;
	case PDELAY_RESP_FOLLOW_UP:
		handlePDelayRespFollowUp(&ptpClock->msgTmpHeader, 
					 ptpClock->msgIbuf, length, 
					 isFromSelf, rtOpts, ptpClock, port);
		HSL_LOG (HSL_LOG_PKTDRV, HSL_LEVEL_INFO, "ptp test:inside function handle()17!\n");
		break;
	case MANAGEMENT:
		handleManagement(&ptpClock->msgTmpHeader, ptpClock->msgIbuf, 
				 length, isFromSelf, rtOpts, ptpClock);
		HSL_LOG (HSL_LOG_PKTDRV, HSL_LEVEL_INFO, "ptp test:inside function handle()18!\n");
		break;
	case SIGNALING:
		handleSignaling(&ptpClock->msgTmpHeader, ptpClock->msgIbuf, 
				length, isFromSelf, rtOpts, ptpClock);
		HSL_LOG (HSL_LOG_PKTDRV, HSL_LEVEL_INFO, "ptp test:inside function handle()19!\n");
		break;
	default:
		//DBG("handle: unrecognized message\n");
		//HSL_LOG (HSL_LOG_PKTDRV, HSL_LEVEL_INFO, "ptp test:inside function handle()20!\n");
		break;
	}
}

/*spec 9.5.3*/
void 
handleAnnounce(MsgHeader *header, Octet *msgIbuf, ssize_t length, 
	       Boolean isFromSelf, RunTimeOpts *rtOpts, PtpClock *ptpClock, UInteger8 port)
{
	Boolean isFromCurrentParent = PTP_FALSE; 
 	
	//DBGV("HandleAnnounce : Announce message received : \n");
	
	if(length < ANNOUNCE_LENGTH) {
		//ERROR("short Announce message\n");
		//toState(PTP_FAULTY, rtOpts, ptpClock);
		return;
	}

    if( clocktype ==PTP_TRANSPARENT )
    {
           //issuePassiveSync(msgIbuf,time,rtOpts,ptpClock, 0x00F00000 & (~(0x01<<port)) );//0x00F00000
           int i,inport_index,outport_index,outport_bitmap = 0;
       for(i = 0;i<2;i++) 
        {  	
            if( ptpclock[i].phy_port == port)
				inport_index = i;
        }
	   if(inport_index >=2 || inport_index <0  || ptpclock[inport_index].enable != PTP_TRUE)
	   	return;
        if ( (portbitmap[ptpClock->msgTmpHeader.domainNumber] & (0x01<<inport_index)) != 0 )
			outport_index = portbitmap[ptpClock->msgTmpHeader.domainNumber] & 0x00FFFFC3;
		else 
			return;
		if (inport_index == 0 || sync_receipt_timeout_flag >= 3)
		{
		    for(i = 2;i<PTP_NUMBER_PORTS;i++) 
            { 
                if((((outport_index>>4) & (0x01 <<i)) !=0) && ptpclock[i].phy_port != -1 && ptpclock[i].enable == PTP_TRUE)
			    	outport_bitmap |=( 0x01 << ptpclock[i].phy_port);
		    }
		}
	    for(i = 0;i<2;i++) 
        { 
            if( i != inport_index )
            {
                if(((outport_index & (0x01 <<i)) !=0) && ptpclock[i].phy_port != -1 && ptpclock[i].enable == PTP_TRUE)
				    outport_bitmap |=( 0x01 << ptpclock[i].phy_port);
            }
		}
		if( outport_bitmap == 0 )
			return;
		
           issueAnnounce( msgIbuf,rtOpts,ptpClock, outport_bitmap );
		   //sal_usleep(5*1000);
		   //HSL_LOG (HSL_LOG_PKTDRV, HSL_LEVEL_INFO, "announce send bitmap is %X!\n",outport_bitmap);
		   return;
    }
    else if (clocktype == PTP_ORDINARY)
    {
	switch(ptpClock->portState) {
	case PTP_INITIALIZING:
	case PTP_FAULTY:
	case PTP_DISABLED:
		
		//DBGV("Handleannounce : disreguard \n");
		return;
		
	case PTP_UNCALIBRATED:	
	case PTP_SLAVE:

		if (isFromSelf) {
			//DBGV("HandleAnnounce : Ignore message from self \n");
			return;
		}
		
		/*  
		 * Valid announce message is received : BMC algorithm
		 * will be executed 
		 */
		ptpClock->record_update = PTP_TRUE; 

		
		isFromCurrentParent = !memcmp(
			ptpClock->parentPortIdentity.clockIdentity,
			header->sourcePortIdentity.clockIdentity,
			CLOCK_IDENTITY_LENGTH)	&& 
			(ptpClock->parentPortIdentity.portNumber == 
			 header->sourcePortIdentity.portNumber);
	
		switch (isFromCurrentParent) {	
		case PTP_TRUE:
	   		msgUnpackAnnounce(ptpClock->msgIbuf,
					  &ptpClock->msgTmp.announce);
	   		s1(header,&ptpClock->msgTmp.announce,ptpClock);
	   		
	   		/*Reset Timer handling Announce receipt timeout*/
	   		//timerStart(ANNOUNCE_RECEIPT_TIMER,
				  // (ptpClock->announceReceiptTimeout) * 
				  //(pow(2,ptpClock->logAnnounceInterval)), 
				  // ptpClock->itimer);
	   		break;
	   		
		case PTP_FALSE:
	   		/*addForeign takes care of AnnounceUnpacking*/
	   		addForeign(ptpClock->msgIbuf,header,ptpClock);
	   		
	   		/*Reset Timer handling Announce receipt timeout*/
	   		//timerStart(ANNOUNCE_RECEIPT_TIMER,
				 //  (ptpClock->announceReceiptTimeout) * 
				  // (pow(2,ptpClock->logAnnounceInterval)), 
				  // ptpClock->itimer);
	   		break;
	   		
		default:
	   		//DBGV("HandleAnnounce : (isFromCurrentParent)"
			     //"strange value ! \n");
	   		return;
	   		
		} /* switch on (isFromCurrentParrent) */
		break;
	   
	case PTP_MASTER:
	default :
	
		if (isFromSelf)	{
			//DBGV("HandleAnnounce : Ignore message from self \n");
			return;
		}
		
		//DBGV("Announce message from another foreign master");
		addForeign(ptpClock->msgIbuf,header,ptpClock);
		ptpClock->record_update = PTP_TRUE;
		break;
	   
	  } /* switch on (port_state) */
    }

}
	
void 
handleSync(MsgHeader *header, Octet *msgIbuf, ssize_t length,
	   TimeInternal *time, Boolean isFromSelf, 
	   RunTimeOpts *rtOpts, PtpClock *ptpClock, UInteger8 port)
{
	TimeInternal OriginTimestamp;
	TimeInternal correctionField;

	//HSL_LOG (HSL_LOG_PKTDRV, HSL_LEVEL_INFO, "ptp test:inside function handlesync()!\n");

	Boolean isFromCurrentParent = PTP_FALSE;
	//DBGV("Sync message received : \n");
	
	if(length < SYNC_LENGTH) {
		//ERROR("short Sync message\n");
		//toState(PTP_FAULTY, rtOpts, ptpClock);
		return;
	}	
	
    if( clocktype ==PTP_TRANSPARENT )
    {
        int i,inport_index;
        //HSL_LOG (HSL_LOG_PKTDRV, HSL_LEVEL_INFO, "sync receive time nanoseconds is %d\n",time->nanoseconds);
        for(i = 0;i<2;i++) 
        {  
            if( ptpclock[i].phy_port == port)
				inport_index = i;
        }
	   if(inport_index >=2 || inport_index <0 )
	   	return;

	   if (inport_index == 0)
	   	sync_receipt_timeout_flag = 0;
	   	   //timerStart(SYNC_INTERVAL_TIMER,
			//   3 * (pow(2,ptpClock->logSyncInterval)), 
			//  ptpClock->itimer);
        
        issueSync( msgIbuf,time,rtOpts,ptpClock, port );
           //issuePassiveSync(msgIbuf,time,rtOpts,ptpClock, 0x00F00000 & (~(0x01<<port)) );//0x00F00000
           
        /*struct timeval tv;
		gettimeofday( &tv, NULL);
		if( tv.tv_usec < OriginTimestamp.nanoseconds/1000 )
			OriginTimestamp.seconds = tv.tv_sec - 1;
		else
			OriginTimestamp.seconds = tv.tv_sec;*/
	    OriginTimestamp.nanoseconds = time ->nanoseconds;
        bcm_time_capture_t capture;
		bcm_time_interface_t intf;
		bcm_time_init( 0 );
		bcm_time_interface_t_init(&intf);
		bcm_time_capture_t_init( &capture );
		bcm_time_interface_get ( 0, &intf );
		if( bcm_time_capture_get( 0, intf.id, &capture) != BCM_E_NONE)
			HSL_LOG (HSL_LOG_PKTDRV, HSL_LEVEL_INFO, "capture time error!\n");
		if( capture.free.nanoseconds < OriginTimestamp.nanoseconds )
			OriginTimestamp.seconds = capture.free.seconds - 1;
		else
			OriginTimestamp.seconds = capture.free.seconds;
		
		//HSL_LOG (HSL_LOG_PKTDRV, HSL_LEVEL_INFO, "sync receive time is %d:%d\n",OriginTimestamp.seconds,OriginTimestamp.nanoseconds);
           if ( adj_freq_opts[inport_index].ingress_sync_time.nanoseconds == 0 && adj_freq_opts[inport_index].ingress_sync_time.seconds == 0 )
		   	{
		   	   adj_freq_opts[inport_index].ingress_sync_time.nanoseconds = OriginTimestamp.nanoseconds;
		       adj_freq_opts[inport_index].ingress_sync_time.seconds = OriginTimestamp.seconds;
    	    }
		   else
		   	{
		   	   adj_freq_opts[inport_index].ingress_sync_time_n.nanoseconds = adj_freq_opts[inport_index].ingress_sync_time.nanoseconds;
			   adj_freq_opts[inport_index].ingress_sync_time_n.seconds = adj_freq_opts[inport_index].ingress_sync_time.seconds;
			   adj_freq_opts[inport_index].ingress_sync_time.nanoseconds = OriginTimestamp.nanoseconds;
		       adj_freq_opts[inport_index].ingress_sync_time.seconds = OriginTimestamp.seconds;
		   	}
		   	   
           
		   
		   //sal_usleep(5*1000);
		   //HSL_LOG (HSL_LOG_PKTDRV, HSL_LEVEL_INFO, "port %d's sync send!\n",port);
		
    }
  if( clocktype ==PTP_ORDINARY)
  {
	switch(ptpClock->portState) {
	case PTP_INITIALIZING:
	case PTP_FAULTY:
	case PTP_DISABLED:
		
		//DBGV("HandleSync : disreguard \n");
		return;
		
	case PTP_UNCALIBRATED:	
	case PTP_SLAVE:
		if (isFromSelf) {
			//DBGV("HandleSync: Ignore message from self \n");
			return;
		}
		isFromCurrentParent = 
			!memcmp(ptpClock->parentPortIdentity.clockIdentity,
				header->sourcePortIdentity.clockIdentity,
				CLOCK_IDENTITY_LENGTH) && 
			(ptpClock->parentPortIdentity.portNumber == 
			 header->sourcePortIdentity.portNumber);
		
		if (isFromCurrentParent) {
			ptpClock->sync_receive_time.seconds = time->seconds;
			ptpClock->sync_receive_time.nanoseconds = 
				time->nanoseconds;
				
			/*if (rtOpts->recordFP) 
				fprintf(rtOpts->recordFP, "%d %llu\n", 
					header->sequenceId, 
					((time->seconds * 1000000000ULL) + 
					 time->nanoseconds));*/

			if ((header->flagField[0] & 0x02) == TWO_STEP_FLAG) {
				ptpClock->waitingForFollow = PTP_TRUE;
				ptpClock->recvSyncSequenceId = 
					header->sequenceId;
				/*Save correctionField of Sync message*/
				integer64_to_internalTime(
					header->correctionfield,
					&correctionField);
				ptpClock->lastSyncCorrectionField.seconds = 
					correctionField.seconds;
				ptpClock->lastSyncCorrectionField.nanoseconds =
					correctionField.nanoseconds;
				break;
			} else {
				msgUnpackSync(ptpClock->msgIbuf,
					      &ptpClock->msgTmp.sync);
				integer64_to_internalTime(
					ptpClock->msgTmpHeader.correctionfield,
					&correctionField);
				//timeInternal_display(&correctionField);
				ptpClock->waitingForFollow = PTP_FALSE;
				toInternalTime(&OriginTimestamp,
					       &ptpClock->msgTmp.sync.originTimestamp);
				updateOffset(&OriginTimestamp,
					     &ptpClock->sync_receive_time,
					     &ptpClock->ofm_filt,rtOpts,
					     ptpClock,&correctionField);
				updateClock(rtOpts,ptpClock);
				break;
			}
		}
		break;
			
	case PTP_MASTER:
	default :
		if (!isFromSelf) {
			//DBGV("HandleSync: Sync message received from "
			    // "another Master  \n");
			break;
		} else {
			/*Add latency*/
			addTime(time,time,&rtOpts->outboundLatency);
			issueFollowup(time,msgIbuf,header,rtOpts,ptpClock,13);
			break;
		}	
	}
  }
}


void 
handleFollowUp(MsgHeader *header, Octet *msgIbuf, ssize_t length, 
	       Boolean isFromSelf, RunTimeOpts *rtOpts, PtpClock *ptpClock, UInteger8 port)
{
	//HSL_LOG (HSL_LOG_PKTDRV, HSL_LEVEL_INFO, "ptp test:inside function handle()!\n");
	TimeInternal preciseOriginTimestamp;
	TimeInternal correctionField;
	Boolean isFromCurrentParent = PTP_FALSE;
	int tnanoseconds;//followup correction field nanoseconds
	//int ret;
	TimeInternal time = {0,0};
	tnanoseconds = (header->correctionfield.msb<<16) + (header->correctionfield.lsb>>16);
	//HSL_LOG (HSL_LOG_PKTDRV, HSL_LEVEL_INFO, "correction field is %d %d %d!\n",header->correctionfield.msb,header->correctionfield.lsb,tnanoseconds);
	
	if(length < FOLLOW_UP_LENGTH)
	{
		//ERROR("short Follow up message\n");
		//toState(PTP_FAULTY, rtOpts, ptpClock);
		//HSL_LOG (HSL_LOG_PKTDRV, HSL_LEVEL_INFO, "short sync followup message!\n");
		return;
	}
	if( clocktype ==PTP_TRANSPARENT )
    {   
       int i,inport_index,outport_index,outport;
	   
       for(i = 0;i<2;i++) 
        {  
            if( ptpclock[i].phy_port == port)
				inport_index = i;
        }
	   if(inport_index >=2 || inport_index <0  || ptpclock[inport_index].enable != PTP_TRUE)
	   	return;
	   if ( (portbitmap[ptpClock->msgTmpHeader.domainNumber] & (0x01<<inport_index)) != 0 )
			outport_index = portbitmap[ptpClock->msgTmpHeader.domainNumber] & 0x00FFFFC3;
		else 
			return;
	  //for test 1121
	/* int  outport_bitmap;
	  for(i = PTP_MASTER_PORT_NUMBER;i<PTP_NUMBER_PORTS;i++) 
        { 
            if((((outport_index>>4) & (0x01 <<i)) !=0) && ptpclock[i].phy_port != -1 && ptpclock[i].enable == PTP_TRUE)
				outport_bitmap |=( 0x01 << ptpclock[i].phy_port);
		}
	  HSL_LOG (HSL_LOG_PKTDRV, HSL_LEVEL_ERROR, "inside function send sync follow up, the value of outport_bitmap is 0x%08x\n",outport_bitmap );*/
        //long long int result_t;
        int result_t1,result_t2,result_t;
        msgUnpackFollowUp(msgIbuf,&ptpClock->msgTmp.follow);
		toInternalTime(&preciseOriginTimestamp,&ptpClock->msgTmp.follow.preciseOriginTimestamp);
		//HSL_LOG (HSL_LOG_PKTDRV, HSL_LEVEL_INFO, "sync send time nanoseconds is %d\n",preciseOriginTimestamp.nanoseconds);
        if ( adj_freq_opts[inport_index].correct_sync_time.nanoseconds == 0 && adj_freq_opts[inport_index].correct_sync_time.seconds == 0 )
		   	{
		   	   adj_freq_opts[inport_index].correct_sync_time.nanoseconds = preciseOriginTimestamp.nanoseconds + tnanoseconds + ptpclock[inport_index].peerMeanPathDelay.nanoseconds;
		       adj_freq_opts[inport_index].correct_sync_time.seconds = preciseOriginTimestamp.seconds;
			    normalizeTime( &adj_freq_opts[inport_index].correct_sync_time);
    	    }
		   else
		   	{
		   	   adj_freq_opts[inport_index].correct_sync_time_n.nanoseconds = adj_freq_opts[inport_index].correct_sync_time.nanoseconds ;
			   adj_freq_opts[inport_index].correct_sync_time_n.seconds = adj_freq_opts[inport_index].correct_sync_time.seconds;
			   adj_freq_opts[inport_index].correct_sync_time.nanoseconds = preciseOriginTimestamp.nanoseconds + tnanoseconds + ptpclock[inport_index].peerMeanPathDelay.nanoseconds;
		       adj_freq_opts[inport_index].correct_sync_time.seconds = preciseOriginTimestamp.seconds;
			   normalizeTime( &adj_freq_opts[inport_index].correct_sync_time);
			   result_t1 = (adj_freq_opts[inport_index].ingress_sync_time.nanoseconds + (adj_freq_opts[inport_index].ingress_sync_time.seconds- adj_freq_opts[inport_index].ingress_sync_time_n.seconds) * 1000000000 - adj_freq_opts[inport_index].ingress_sync_time_n.nanoseconds);
			   result_t2 = (adj_freq_opts[inport_index].correct_sync_time.nanoseconds + (adj_freq_opts[inport_index].correct_sync_time.seconds- adj_freq_opts[inport_index].correct_sync_time_n.seconds) * 1000000000 - adj_freq_opts[inport_index].correct_sync_time_n.nanoseconds); 
               if( result_t1 > result_t2 )
			   	{
			   	    adj_freq_opts[inport_index].differ = result_t1 - result_t2;
					adj_freq_opts[inport_index].flag = -1;
               	}
			   else
			   	{
			   	    adj_freq_opts[inport_index].differ = result_t2 - result_t1;
					adj_freq_opts[inport_index].flag = 1;
               	}
			    adj_freq_opts[inport_index].div = result_t1;
				//HSL_LOG (HSL_LOG_PKTDRV, HSL_LEVEL_INFO, "differ  %d ",adj_freq_opts.flag * adj_freq_opts.differ);
        
		   }
		  
//to do:改写find函数的参数和返回值
        //if (port == 13)
        //{
        //HSL_LOG (HSL_LOG_PKTDRV, HSL_LEVEL_INFO, "followup correction timestamp 0x%02x!\n",time->nanoseconds);
		struct hsl_ptp_hash_node node;
        /*memcpy(node.portID.clockIdentity,ptpClock->msgTmpHeader.sourcePortIdentity.clockIdentity,CLOCK_IDENTITY_LENGTH);
		node.portID.portNumber = ptpClock->msgTmpHeader.sourcePortIdentity.portNumber;
		node.inport = port;
		node.outport = 13;
		node.nanoseconds = 0;
		node.sequenceId = ptpClock->msgTmpHeader.sequenceId;*/

		node.portID = ptpClock->msgTmpHeader.sourcePortIdentity;
		node.sequenceId = ptpClock->msgTmpHeader.sequenceId;
		node.inport = port;
		node.nanoseconds = 0;


	   /*int i,inport_index,outport_index,outport;
       for(i = 0;i<8;i++) 
        {  	
            if( ptpclock[i].phy_port == port)
				inport_index = i;
        }*/
        /*if ( (portbitmap[ptpClock->msgTmpHeader.domainNumber] & (0x01<<inport_index)) != 0 )
			outport_index = portbitmap[ptpClock->msgTmpHeader.domainNumber] & 0x0000FF00;
		else return;*/
		//HSL_LOG (HSL_LOG_PKTDRV, HSL_LEVEL_ERROR, "inside function send sync, the value of outport_index is 0x%08x\n",outport_index );
		for(i = 0;i<2;i++) 
        { 
           if( i != inport_index )
           {
              if((((outport_index) & (0x01 <<i)) !=0)  && ptpclock[i].phy_port != -1 && ptpclock[i].enable == PTP_TRUE)
              {
                outport =  ptpclock[i].phy_port;
				node.outport = outport;
				//HSL_LOG (HSL_LOG_PKTDRV, HSL_LEVEL_INFO,"clock %d, portnumber %d, sequence %d, inport %d, outport %d\n",node.portID.clockIdentity[7],node.portID.portNumber,node.sequenceId,node.inport,node.outport);
				PtpHTFind(&node,PTPHT);
				int nano_tmp;//compasentation value for frequent differ between master and transparent
				nano_tmp = (node.nanoseconds/10000)*adj_freq_opts[inport_index].differ/(adj_freq_opts[inport_index].div/10000);
		        nano_tmp = nano_tmp*adj_freq_opts[inport_index].flag;
		        //HSL_LOG (HSL_LOG_PKTDRV, HSL_LEVEL_INFO, "nano tmp is %d,inport index is %d, differ is %d, div is %d!\n",nano_tmp,inport_index,adj_freq_opts[inport_index].differ, adj_freq_opts[inport_index].div);
		        //HSL_LOG (HSL_LOG_PKTDRV, HSL_LEVEL_INFO, "port %d, sync resisdence timestamp 0x%02x! propagation time is %d!\n",outport,node.nanoseconds, ptpclock[inport_index].peerMeanPathDelay.nanoseconds);
                time.nanoseconds = tnanoseconds + node.nanoseconds + ptpclock[inport_index].peerMeanPathDelay.nanoseconds + nano_tmp;//ptpClock1->peerMeanPathDelay.nanoseconds
                issueFollowup(&time,msgIbuf,header,rtOpts,ptpClock,outport);
		        //HSL_LOG (HSL_LOG_PKTDRV, HSL_LEVEL_INFO, "followup correction timestamp 0x%02x!sync resisdence time is %d\n",time.nanoseconds, node.nanoseconds);
		      }
           }
		}
		if (inport_index == 1 && sync_receipt_timeout_flag < 3)
	   	     return;
		for(i = 2;i<PTP_NUMBER_PORTS;i++) 
        { 
            if((((outport_index>>4) & (0x01 <<i)) !=0)  && ptpclock[i].phy_port != -1 && ptpclock[i].enable == PTP_TRUE)
            {
                outport =  ptpclock[i].phy_port;
				node.outport = outport;
				//HSL_LOG (HSL_LOG_PKTDRV, HSL_LEVEL_INFO,"clock %d, portnumber %d, sequence %d, inport %d, outport %d\n",node.portID.clockIdentity[7],node.portID.portNumber,node.sequenceId,node.inport,node.outport);
				PtpHTFind(&node,PTPHT);
				int nano_tmp;//compasentation value for frequent differ between master and transparent
				nano_tmp = (node.nanoseconds/10000)*adj_freq_opts[inport_index].differ/(adj_freq_opts[inport_index].div/10000);
		        nano_tmp = nano_tmp*adj_freq_opts[inport_index].flag;
		        //HSL_LOG (HSL_LOG_PKTDRV, HSL_LEVEL_INFO, "nano tmp is %d,inport index is %d, differ is %d, div is %d!\n",nano_tmp,inport_index,adj_freq_opts[inport_index].differ, adj_freq_opts[inport_index].div);
		        //HSL_LOG (HSL_LOG_PKTDRV, HSL_LEVEL_INFO, "port %d, sync resisdence timestamp 0x%02x! propagation time is %d!\n",outport,node.nanoseconds, ptpclock[inport_index].peerMeanPathDelay.nanoseconds);
                time.nanoseconds = tnanoseconds + node.nanoseconds + ptpclock[inport_index].peerMeanPathDelay.nanoseconds + nano_tmp;//ptpClock1->peerMeanPathDelay.nanoseconds
                issueFollowup(&time,msgIbuf,header,rtOpts,ptpClock,outport);
		        //HSL_LOG (HSL_LOG_PKTDRV, HSL_LEVEL_INFO, "followup correction timestamp 0x%02x!sync resisdence time is %d\n",time.nanoseconds, node.nanoseconds);
		    }
		}

		
        //_hsl_ptp_sync_del( &(ptpClock->msgTmpHeader.sourcePortIdentity),ptpClock->msgTmpHeader.sequenceId,23,21 );
		//HSL_LOG (HSL_LOG_PKTDRV, HSL_LEVEL_INFO, "port 12's sync followup send!\n");
       // }
    }
	
	else if( clocktype ==PTP_ORDINARY )
    {
      if (isFromSelf)
	  {
		//DBGV("Handlefollowup : Ignore message from self \n");
		  return;
	  }
	 
	  switch(ptpClock->portState )
	  {
	  case PTP_INITIALIZING:
	  case PTP_FAULTY:
	  case PTP_DISABLED:
	  case PTP_LISTENING:
		
		//DBGV("Handfollowup : disreguard \n");
		return;
		
	  case PTP_UNCALIBRATED:	
	  case PTP_SLAVE:

		isFromCurrentParent = 
			!memcmp(ptpClock->parentPortIdentity.clockIdentity,
				header->sourcePortIdentity.clockIdentity,
				CLOCK_IDENTITY_LENGTH) && 
			(ptpClock->parentPortIdentity.portNumber == 
			 header->sourcePortIdentity.portNumber);
	 	
		if (isFromCurrentParent) {
			if (ptpClock->waitingForFollow)	{
				if ((ptpClock->recvSyncSequenceId == 
				     header->sequenceId)) {
					msgUnpackFollowUp(ptpClock->msgIbuf,
							  &ptpClock->msgTmp.follow);
					ptpClock->waitingForFollow = PTP_FALSE;
					toInternalTime(&preciseOriginTimestamp,
						       &ptpClock->msgTmp.follow.preciseOriginTimestamp);
					integer64_to_internalTime(ptpClock->msgTmpHeader.correctionfield,
								  &correctionField);
					addTime(&correctionField,&correctionField,
						&ptpClock->lastSyncCorrectionField);
					updateOffset(&preciseOriginTimestamp,
						     &ptpClock->sync_receive_time,&ptpClock->ofm_filt,
						     rtOpts,ptpClock,
						     &correctionField);
					updateClock(rtOpts,ptpClock);
					break;	 		
				} //else 
					//DBGV("SequenceID doesn't match with "
					     //"last Sync message \n");
			}// else 
				//DBGV("Slave was not waiting a follow up "
				    // "message \n");
		}// else 
			//DBGV("Follow up message is not from current parent \n");

	  case PTP_MASTER:
		//DBGV("Follow up message received from another master \n");
		break;
			
	  default:
    		//DBG("do unrecognized state\n");
    		break;
	  } /* Switch on (port_state) */
	}

}


void 
handleDelayReq(MsgHeader *header, Octet *msgIbuf, ssize_t length, 
	       TimeInternal *time, Boolean isFromSelf,
	       RunTimeOpts *rtOpts, PtpClock *ptpClock, UInteger8 port)
{
	//if (rtOpts->E2E_mode) {
		//DBGV("delayReq message received : \n");
		
		if(length < DELAY_REQ_LENGTH) {
			//ERROR("short DelayReq message\n");
			//toState(PTP_FAULTY, rtOpts, ptpClock);
			return;
		}
		
	if( clocktype ==PTP_TRANSPARENT )
    {
        //HSL_LOG (HSL_LOG_PKTDRV, HSL_LEVEL_INFO, "delay_req receive time nanoseconds is %d\n",time->nanoseconds);
        issueDelayReq( msgIbuf,time,rtOpts,ptpClock,port );
     
    }
  else if( clocktype ==PTP_ORDINARY)
  {

		switch(ptpClock->portState) {
		case PTP_INITIALIZING:
		case PTP_FAULTY:
		case PTP_DISABLED:
		case PTP_UNCALIBRATED:
		case PTP_LISTENING:
			//DBGV("HandledelayReq : disreguard \n");
			return;

		case PTP_SLAVE:
			if (isFromSelf)	{
				/* 
				 * Get sending timestamp from IP stack
				 * with So_TIMESTAMP
				 */
				ptpClock->delay_req_send_time.seconds = 
					time->seconds;
				ptpClock->delay_req_send_time.nanoseconds = 
					time->nanoseconds;

				/*Add latency*/
				addTime(&ptpClock->delay_req_send_time,
					&ptpClock->delay_req_send_time,
					&rtOpts->outboundLatency);
				break;
			}
			break;

		case PTP_MASTER:
			msgUnpackHeader(ptpClock->msgIbuf,
					&ptpClock->delayReqHeader);
			issueDelayResp(time,msgIbuf,&ptpClock->delayReqHeader,
				       rtOpts,ptpClock,port);
			break;

		default:
	    		//DBG("do unrecognized state\n");
	    		break;
		}
	} //else /* (Peer to Peer mode) */
		//ERROR("Delay messages are disreguard in Peer to Peer mode \n");
}

void 
handleDelayResp(MsgHeader *header, Octet *msgIbuf, ssize_t length,
		Boolean isFromSelf, RunTimeOpts *rtOpts, PtpClock *ptpClock, UInteger8 port)
{
    TimeInternal preciseOriginTimestamp;
	TimeInternal correctionField;
	Boolean isFromCurrentParent = PTP_FALSE;
	int ret;
	TimeInternal time = {0,0};
	
	if(length < DELAY_RESP_LENGTH)
	{
		//toState(PTP_FAULTY, rtOpts, ptpClock);
		//HSL_LOG (HSL_LOG_PKTDRV, HSL_LEVEL_INFO, "short delay resp message!\n");
		return;
	}
	if( clocktype ==PTP_TRANSPARENT )
    {   
		struct hsl_ptp_hash_node node;
		msgUnpackDelayResp(msgIbuf,&ptpClock->msgTmp.resp);

		node.portID = ptpClock->msgTmp.resp.requestingPortIdentity;
		node.sequenceId = ptpClock->msgTmpHeader.sequenceId;
		node.outport = port;
		node.nanoseconds = 0;

	   int i,inport_index,outport_index,outport;
       for(i = 0;i<2;i++) 
        {  	
            if( ptpclock[i].phy_port == port)
			{
			    inport_index = i;
		        if( ptpclock[inport_index].delayMechanism != E2E )
		           return;
            }
        }

        if ( (portbitmap[ptpClock->msgTmpHeader.domainNumber] & (0x01<<inport_index)) != 0 )
			outport_index = portbitmap[ptpClock->msgTmpHeader.domainNumber] & 0x00FFFFC0;
		else 
			return;
		//HSL_LOG (HSL_LOG_PKTDRV, HSL_LEVEL_ERROR, "inside function send delay_resp, the value of outport_index is 0x%08x\n",outport_index );
		for(i = 2;i<PTP_NUMBER_PORTS;i++) 
        { 
            if((((outport_index>>4) & (0x01 <<i)) !=0) && ptpclock[i].phy_port != -1 && ptpclock[i].enable == PTP_TRUE)
            {
                outport =  ptpclock[i].phy_port;
				node.inport = outport;
				//HSL_LOG (HSL_LOG_PKTDRV, HSL_LEVEL_INFO,"clock %d, portnumber %d, sequence %d, inport %d, outport %d\n",node.portID.clockIdentity[7],node.portID.portNumber,node.sequenceId,node.inport,node.outport);
				if( PtpHTFind(&node,DELAYHT) == 0 )
				{
		          time.nanoseconds = node.nanoseconds;
                  issueDelayResp(&time,msgIbuf,header,rtOpts,ptpClock,outport);
		          //HSL_LOG (HSL_LOG_PKTDRV, HSL_LEVEL_INFO, "delay_req resisdence  time is 0x%02x!\n",time.nanoseconds);
				}
		    }
		}

		
        //_hsl_ptp_sync_del( &(ptpClock->msgTmpHeader.sourcePortIdentity),ptpClock->msgTmpHeader.sequenceId,23,21 );
		//HSL_LOG (HSL_LOG_PKTDRV, HSL_LEVEL_INFO, "port 12's sync followup send!\n");
       // }
    }
	
	else if( clocktype ==PTP_ORDINARY )
	{
		TimeInternal requestReceiptTimestamp;

		//DBGV("delayResp message received : \n");

		switch(ptpClock->portState) {
		case PTP_INITIALIZING:
		case PTP_FAULTY:
		case PTP_DISABLED:
		case PTP_UNCALIBRATED:
		case PTP_LISTENING:
			//DBGV("HandledelayResp : disreguard \n");
			return;

		case PTP_SLAVE:
			msgUnpackDelayResp(ptpClock->msgIbuf,
					   &ptpClock->msgTmp.resp);

			if ((memcmp(ptpClock->parentPortIdentity.clockIdentity,
				    header->sourcePortIdentity.clockIdentity,
				    CLOCK_IDENTITY_LENGTH) == 0 ) &&
			    (ptpClock->parentPortIdentity.portNumber == 
			     header->sourcePortIdentity.portNumber))
				isFromCurrentParent = PTP_TRUE;
			
			if ((memcmp(ptpClock->portIdentity.clockIdentity,
				    ptpClock->msgTmp.resp.requestingPortIdentity.clockIdentity,
				    CLOCK_IDENTITY_LENGTH) == 0) &&
			    ((ptpClock->sentDelayReqSequenceId - 1)== 
			     header->sequenceId) &&
			    (ptpClock->portIdentity.portNumber == 
			     ptpClock->msgTmp.resp.requestingPortIdentity.portNumber)
			    && isFromCurrentParent) {
				toInternalTime(&requestReceiptTimestamp,
					       &ptpClock->msgTmp.resp.receiveTimestamp);
				ptpClock->delay_req_receive_time.seconds = 
					requestReceiptTimestamp.seconds;
				ptpClock->delay_req_receive_time.nanoseconds = 
					requestReceiptTimestamp.nanoseconds;

				integer64_to_internalTime(
					header->correctionfield,
					&correctionField);
				updateDelay(&ptpClock->owd_filt,
					    rtOpts,ptpClock, &correctionField);

				ptpClock->logMinDelayReqInterval = 
					header->logMessageInterval;
			} else {
				//DBGV("HandledelayResp : delayResp doesn't match with the delayReq. \n");
				break;
			}
		}
	} else { /* (Peer to Peer mode) */
		//ERROR("Delay messages are disregarded in Peer to Peer mode \n");
	}

}


void 
handlePDelayReq(MsgHeader *header, Octet *msgIbuf, ssize_t length, 
		TimeInternal *time, Boolean isFromSelf, 
		RunTimeOpts *rtOpts, PtpClock *ptpClock, UInteger8 port)
{
	//if (!rtOpts->E2E_mode) {
		//DBGV("PdelayReq message received : \n");
	
		if(length < PDELAY_REQ_LENGTH) {
			//ERROR("short PDelayReq message\n");
			//toState(PTP_FAULTY, rtOpts, ptpClock);
			return;
		}	
		 if( clocktype ==PTP_TRANSPARENT )
		  {
		  	   /*int i,inport_index;
               for(i = 0;i < PTP_MASTER_PORT_NUMBER;i++) 
               {  	
                    if( ptpclock[i].phy_port == port)
				    {
				        inport_index = i;
					    if( ptpclock[inport_index].delayMechanism != P2P )
		   	               return;
                    }
               }*/
	           
		      MsgPDelayReq  pdelayreq;
		      msgUnpackPDelayReq(msgIbuf, &pdelayreq);
			  time->seconds = pdelayreq.originTimestamp.secondsField.lsb;
			  if ( time->nanoseconds < pdelayreq.originTimestamp.nanosecondsField )
			  	time->seconds +=1;
		      //HSL_LOG (HSL_LOG_PKTDRV, HSL_LEVEL_INFO, "ptp test:portID is %d!2\n",header->sourcePortIdentity.portNumber);
			  issuePDelayResp(time,header,rtOpts,ptpClock,port);
			  //sal_usleep(5*1000);
			  issuePDelayRespFollowUp(time,header,rtOpts,ptpClock,port);
			  //sal_usleep(10*1000);
		  }
		 
	  if( clocktype ==PTP_ORDINARY)
	  {
		switch(ptpClock->portState ) {
		case PTP_INITIALIZING:
		case PTP_FAULTY:
		case PTP_DISABLED:
		case PTP_UNCALIBRATED:
		case PTP_LISTENING:
			//DBGV("HandlePdelayReq : disreguard \n");
			return;
		
		case PTP_SLAVE:
		case PTP_MASTER:
		case PTP_PASSIVE:
		
			if (isFromSelf) {
				/* 
				 * Get sending timestamp from IP stack
				 * with So_TIMESTAMP
				 */
				ptpClock->pdelay_req_send_time.seconds = 
					time->seconds;
				ptpClock->pdelay_req_send_time.nanoseconds = 
					time->nanoseconds;
			
				/*Add latency*/
				addTime(&ptpClock->pdelay_req_send_time,
					&ptpClock->pdelay_req_send_time,
					&rtOpts->outboundLatency);
				break;
			} else {
				msgUnpackHeader(ptpClock->msgIbuf,
						&ptpClock->PdelayReqHeader);
				issuePDelayResp(time, header, rtOpts, 
						ptpClock,port);	
				break;
			}
		default:
			//DBG("do unrecognized state\n");
			break;
		}
	  }
	 //else /* (End to End mode..) */
		//ERROR("Peer Delay messages are disreguard in End to End "
		      //"mode \n");
}

void 
handlePDelayResp(MsgHeader *header, Octet *msgIbuf, TimeInternal *time,
		 ssize_t length, Boolean isFromSelf, 
		 RunTimeOpts *rtOpts, PtpClock *ptpClock, UInteger8 port)
{
	

	//if (!rtOpts->E2E_mode) {
		Boolean isFromCurrentParent = PTP_FALSE;
		TimeInternal requestReceiptTimestamp;
		TimeInternal correctionField;
	
		//DBGV("PdelayResp message received : \n");
	
		if(length < PDELAY_RESP_LENGTH)	{
			//ERROR("short PDelayResp message\n");
			//toState(PTP_FAULTY, rtOpts, ptpClock);
			return;
		}	
	 
		if( clocktype == PTP_TRANSPARENT )
		  {
		  	  int i, inport_index=-1;
	          for(i = 0; i < PTP_MASTER_PORT_NUMBER; i++) 
              {  	
                 if( ptpclock[i].phy_port == port)
				     inport_index = i;
	          }
			  if( inport_index == -1 )
			  	return;
		      msgUnpackPDelayResp(ptpClock->msgIbuf,
					    &ptpClock->msgTmp.presp);
		      if (!((ptpclock[inport_index].sentPDelayReqSequenceId-1 == 
			       header->sequenceId) && 
			      (!memcmp(ptpClock->portIdentity.clockIdentity,ptpClock->msgTmp.presp.requestingPortIdentity.clockIdentity,CLOCK_IDENTITY_LENGTH))
				 && ( ptpClock->portIdentity.portNumber == ptpClock->msgTmp.presp.requestingPortIdentity.portNumber)))	
				 {
                                /* Two Step Clock */
				if ((header->flagField[0] & 0x02) == TWO_STEP_FLAG) 
				{
				    /*if(port == 17)
				    {
				        ptpClock1>pdelay_resp_receive_time.seconds = time->seconds;
				        ptpClock1>pdelay_resp_receive_time.nanoseconds = time->nanoseconds; 
					    integer64_to_internalTime(header->correctionfield,&correctionField);
					    ptpClock1>lastPdelayRespCorrectionField.seconds = correctionField.seconds;
					    ptpClock1>lastPdelayRespCorrectionField.nanoseconds = correctionField.nanoseconds;
						return;
				    }*/
                        
					         /*Store t4 (Fig 35)*/
					         ptpclock[inport_index].pdelay_resp_receive_time.seconds = time->seconds;
					         ptpclock[inport_index].pdelay_resp_receive_time.nanoseconds = time->nanoseconds;
					         if( ptpclock[inport_index].pdelay_resp_receive_time.nanoseconds < ptpclock[inport_index].pdelay_req_send_time.nanoseconds)
						         ptpclock[inport_index].pdelay_resp_receive_time.nanoseconds += 1000000000;
                             //HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO, "pdelay_resp receive time is %d.\n",time->nanoseconds);
					         /*store t2 (Fig 35)*/
					         toInternalTime(&requestReceiptTimestamp,&ptpClock->msgTmp.presp.requestReceiptTimestamp);
					         ptpclock[inport_index].pdelay_req_receive_time.seconds = requestReceiptTimestamp.seconds;
					         ptpclock[inport_index].pdelay_req_receive_time.nanoseconds = requestReceiptTimestamp.nanoseconds;
					
					         //integer64_to_internalTime(header->correctionfield,&correctionField);
					
					         //ptpClock2->lastPdelayRespCorrectionField.seconds = correctionField.seconds;
					         //ptpClock2->lastPdelayRespCorrectionField.nanoseconds = correctionField.nanoseconds;
                           //HSL_LOG (HSL_LOG_PKTDRV, HSL_LEVEL_INFO, "TEST:the value of pdelay_resp receive port is %d!!!\n",inport_index);
						}
						
		      	}
		  }
		 
	  else if( clocktype == PTP_ORDINARY)
	  {
		
		switch(ptpClock->portState ) {
		case PTP_INITIALIZING:
		case PTP_FAULTY:
		case PTP_DISABLED:
		case PTP_UNCALIBRATED:
		case PTP_LISTENING:
			//DBGV("HandlePdelayResp : disreguard \n");
			return;
		
		case PTP_SLAVE:
			if (isFromSelf)	{
				addTime(time,time,&rtOpts->outboundLatency);
				issuePDelayRespFollowUp(time,
							&ptpClock->PdelayReqHeader,
							rtOpts,ptpClock,port);
				break;
			}
			msgUnpackPDelayResp(ptpClock->msgIbuf,
					    &ptpClock->msgTmp.presp);
		
			isFromCurrentParent = !memcmp(ptpClock->parentPortIdentity.clockIdentity,
						      header->sourcePortIdentity.clockIdentity,CLOCK_IDENTITY_LENGTH) && 
				(ptpClock->parentPortIdentity.portNumber == 
				 header->sourcePortIdentity.portNumber);
	
			if (!((ptpClock->sentPDelayReqSequenceId == 
			       header->sequenceId) && 
			      (!memcmp(ptpClock->portIdentity.clockIdentity,ptpClock->msgTmp.presp.requestingPortIdentity.clockIdentity,CLOCK_IDENTITY_LENGTH))
				 && ( ptpClock->portIdentity.portNumber == ptpClock->msgTmp.presp.requestingPortIdentity.portNumber)))	{

                                /* Two Step Clock */
				if ((header->flagField[0] & 0x02) == 
				    TWO_STEP_FLAG) {
					/*Store t4 (Fig 35)*/
					ptpClock->pdelay_resp_receive_time.seconds = time->seconds;
					ptpClock->pdelay_resp_receive_time.nanoseconds = time->nanoseconds;
					/*store t2 (Fig 35)*/
					toInternalTime(&requestReceiptTimestamp,
						       &ptpClock->msgTmp.presp.requestReceiptTimestamp);
					ptpClock->pdelay_req_receive_time.seconds = requestReceiptTimestamp.seconds;
					ptpClock->pdelay_req_receive_time.nanoseconds = requestReceiptTimestamp.nanoseconds;
					
					integer64_to_internalTime(header->correctionfield,&correctionField);
					ptpClock->lastPdelayRespCorrectionField.seconds = correctionField.seconds;
					ptpClock->lastPdelayRespCorrectionField.nanoseconds = correctionField.nanoseconds;
					break;
				} else {
				/* One step Clock */
					/*Store t4 (Fig 35)*/
					ptpClock->pdelay_resp_receive_time.seconds = time->seconds;
					ptpClock->pdelay_resp_receive_time.nanoseconds = time->nanoseconds;
					
					integer64_to_internalTime(header->correctionfield,&correctionField);
					updatePeerDelay (&ptpClock->owd_filt,rtOpts,ptpClock,&correctionField,PTP_FALSE);

					break;
				}
			} else {
				//DBGV("HandlePdelayResp : Pdelayresp doesn't "
				    // "match with the PdelayReq. \n");
				break;
			}
			break; /* XXX added by gnn for safety */
		case PTP_MASTER:
			/*Loopback Timestamp*/
			if (isFromSelf) {
				/*Add latency*/
				addTime(time,time,&rtOpts->outboundLatency);
					
				issuePDelayRespFollowUp(
					time,
					&ptpClock->PdelayReqHeader,
					rtOpts, ptpClock, port);
				break;
			}
			msgUnpackPDelayResp(ptpClock->msgIbuf,
					    &ptpClock->msgTmp.presp);
		
			isFromCurrentParent = !memcmp(ptpClock->parentPortIdentity.clockIdentity,header->sourcePortIdentity.clockIdentity,CLOCK_IDENTITY_LENGTH)
				&& (ptpClock->parentPortIdentity.portNumber == header->sourcePortIdentity.portNumber);
	
			if (!((ptpClock->sentPDelayReqSequenceId == 
			       header->sequenceId) && 
			      (!memcmp(ptpClock->portIdentity.clockIdentity,
				       ptpClock->msgTmp.presp.requestingPortIdentity.clockIdentity,
				       CLOCK_IDENTITY_LENGTH)) && 
			      (ptpClock->portIdentity.portNumber == 
			       ptpClock->msgTmp.presp.requestingPortIdentity.portNumber))) {
                                /* Two Step Clock */
				if ((header->flagField[0] & 0x02) == 
				    TWO_STEP_FLAG) {
					/*Store t4 (Fig 35)*/
					ptpClock->pdelay_resp_receive_time.seconds = time->seconds;
					ptpClock->pdelay_resp_receive_time.nanoseconds = time->nanoseconds;
					/*store t2 (Fig 35)*/
					toInternalTime(&requestReceiptTimestamp,
						       &ptpClock->msgTmp.presp.requestReceiptTimestamp);
					ptpClock->pdelay_req_receive_time.seconds = 
						requestReceiptTimestamp.seconds;
					ptpClock->pdelay_req_receive_time.nanoseconds = 
						requestReceiptTimestamp.nanoseconds;
					integer64_to_internalTime(
						header->correctionfield,
						&correctionField);
					ptpClock->lastPdelayRespCorrectionField.seconds = correctionField.seconds;
					ptpClock->lastPdelayRespCorrectionField.nanoseconds = correctionField.nanoseconds;
					break;
				} else { /* One step Clock */
					/*Store t4 (Fig 35)*/
					ptpClock->pdelay_resp_receive_time.seconds = time->seconds;
					ptpClock->pdelay_resp_receive_time.nanoseconds = time->nanoseconds;
					
					integer64_to_internalTime(
						header->correctionfield,
						&correctionField);
					updatePeerDelay(&ptpClock->owd_filt,
							rtOpts,ptpClock,
							&correctionField,PTP_FALSE);
					break;
				}
			}
			break; /* XXX added by gnn for safety */
		default:
			//DBG("do unrecognized state\n");
			break;
		}
	  }
	 //else { /* (End to End mode..) */
		//ERROR("Peer Delay messages are disreguard in End to End "
		     // "mode \n");
	//}
}

void 
handlePDelayRespFollowUp(MsgHeader *header, Octet *msgIbuf, ssize_t length, 
			 Boolean isFromSelf, RunTimeOpts *rtOpts, 
			 PtpClock *ptpClock, UInteger8 port){

	//if (!rtOpts->E2E_mode) {
		TimeInternal responseOriginTimestamp;
		TimeInternal correctionField;
	
		//DBGV("PdelayRespfollowup message received : \n");
	
		if(length < PDELAY_RESP_FOLLOW_UP_LENGTH) {
			//ERROR("short PDelayRespfollowup message\n");
			//toState(PTP_FAULTY, rtOpts, ptpClock);
			return;
		}	

		if( clocktype == PTP_TRANSPARENT )
		{
		     int i,inport_index=-1;
	         for(i = 0;i<PTP_MASTER_PORT_NUMBER;i++) 
             {  	
                if( ptpclock[i].phy_port == port)
				  inport_index = i;
             }    
			 if( inport_index == -1)
			 	return;
		    if (header->sequenceId == 
			    ptpclock[inport_index].sentPDelayReqSequenceId-1) {
				msgUnpackPDelayRespFollowUp(
					ptpClock->msgIbuf,
					&ptpClock->msgTmp.prespfollow);
				toInternalTime(
					&responseOriginTimestamp,
					&ptpClock->msgTmp.prespfollow.responseOriginTimestamp);

				ptpclock[inport_index].pdelay_resp_send_time.seconds = responseOriginTimestamp.seconds;
				ptpclock[inport_index].pdelay_resp_send_time.nanoseconds = responseOriginTimestamp.nanoseconds;
				if (ptpclock[inport_index].pdelay_resp_send_time.nanoseconds < ptpclock[inport_index].pdelay_req_receive_time.nanoseconds)
                    ptpclock[inport_index].pdelay_resp_send_time.nanoseconds +=1000000000;
				//integer64_to_internalTime(ptpClock->msgTmpHeader.correctionfield,&correctionField);
				//addTime(&correctionField,&correctionField,&ptpClock2->lastPdelayRespCorrectionField);
				//updatePeerDelay (&ptpClock->owd_filt,
						 //rtOpts, ptpClock,
						 //&correctionField,PTP_TRUE);
				//HSL_LOG (HSL_LOG_PKTDRV, HSL_LEVEL_INFO, "The pdelay seconds is %d %d %d %d!here",ptpclock[inport_index].pdelay_req_send_time.seconds,ptpclock[inport_index].pdelay_req_receive_time.seconds,ptpclock[inport_index].pdelay_resp_send_time.seconds,ptpclock[inport_index].pdelay_resp_receive_time.seconds);
				//HSL_LOG (HSL_LOG_PKTDRV, HSL_LEVEL_INFO, "The pdelay is %d %d %d %d!here",ptpclock[inport_index].pdelay_req_send_time.nanoseconds,ptpclock[inport_index].pdelay_req_receive_time.nanoseconds,ptpclock[inport_index].pdelay_resp_send_time.nanoseconds,ptpclock[inport_index].pdelay_resp_receive_time.nanoseconds);
			    ptpclock[inport_index].peerMeanPathDelay.nanoseconds= ((ptpclock[inport_index].pdelay_resp_receive_time.nanoseconds- ptpclock[inport_index].pdelay_req_send_time.nanoseconds) - (ptpclock[inport_index].pdelay_resp_send_time.nanoseconds - ptpclock[inport_index].pdelay_req_receive_time.nanoseconds))/2;//correctionField.nanoseconds
                //HSL_LOG (HSL_LOG_PKTDRV, HSL_LEVEL_INFO, "delay %d!\n",ptpclock[inport_index].peerMeanPathDelay.nanoseconds);
				
		    }
		}
				 
		else if( clocktype == PTP_ORDINARY)
	    {
		switch(ptpClock->portState) {
		case PTP_INITIALIZING:
		case PTP_FAULTY:
		case PTP_DISABLED:
		case PTP_UNCALIBRATED:
			//DBGV("HandlePdelayResp : disreguard \n");
			return;
		
		case PTP_SLAVE:
			if (header->sequenceId == 
			    ptpClock->sentPDelayReqSequenceId-1) {
				msgUnpackPDelayRespFollowUp(
					ptpClock->msgIbuf,
					&ptpClock->msgTmp.prespfollow);
				toInternalTime(
					&responseOriginTimestamp,
					&ptpClock->msgTmp.prespfollow.responseOriginTimestamp);
				ptpClock->pdelay_resp_send_time.seconds = 
					responseOriginTimestamp.seconds;
				ptpClock->pdelay_resp_send_time.nanoseconds = 
					responseOriginTimestamp.nanoseconds;
				integer64_to_internalTime(
					ptpClock->msgTmpHeader.correctionfield,
					&correctionField);
				addTime(&correctionField,&correctionField,
					&ptpClock->lastPdelayRespCorrectionField);
				updatePeerDelay (&ptpClock->owd_filt,
						 rtOpts, ptpClock,
						 &correctionField,PTP_TRUE);
				break;
			}
		case PTP_MASTER:
			if (header->sequenceId == 
			    ptpClock->sentPDelayReqSequenceId-1) {
				msgUnpackPDelayRespFollowUp(
					ptpClock->msgIbuf,
					&ptpClock->msgTmp.prespfollow);
				toInternalTime(&responseOriginTimestamp,
					       &ptpClock->msgTmp.prespfollow.responseOriginTimestamp);
				ptpClock->pdelay_resp_send_time.seconds = 
					responseOriginTimestamp.seconds;
				ptpClock->pdelay_resp_send_time.nanoseconds = 
					responseOriginTimestamp.nanoseconds;
				integer64_to_internalTime(
					ptpClock->msgTmpHeader.correctionfield,
					&correctionField);
				addTime(&correctionField, 
					&correctionField,
					&ptpClock->lastPdelayRespCorrectionField);
				updatePeerDelay(&ptpClock->owd_filt,
						rtOpts, ptpClock,
						&correctionField,PTP_TRUE);
				break;
			}
		default:
			break;
			//DBGV("Disregard PdelayRespFollowUp message  \n");
		}
	  }
//else { /* (End to End mode..) */
		//ERROR("Peer Delay messages are disreguard in End to End "
		      //"mode \n");
	//}
}

void 
handleManagement(MsgHeader *header, Octet *msgIbuf, ssize_t length, 
		 Boolean isFromSelf, RunTimeOpts *rtOpts, PtpClock *ptpClock)
{}

void 
handleSignaling(MsgHeader *header, Octet *msgIbuf, ssize_t length, 
		     Boolean isFromSelf, RunTimeOpts *rtOpts, 
		     PtpClock *ptpClock)
{}


/*Pack and send on general multicast ip adress an Announce message*/
void 
issueAnnounce(Octet *msgObuf,RunTimeOpts *rtOpts,PtpClock *ptpClock,int outport_bitmap)
{
	msgPackAnnounce(ptpClock->msgObuf,ptpClock);
	if( clocktype == PTP_TRANSPARENT)
	{
		int ret = ptp_sock_sendmsg_general (msgObuf,ANNOUNCE_LENGTH,outport_bitmap);
		if( ret != 0 )
		{	 
			//HSL_LOG (HSL_LOG_PKTDRV, HSL_LEVEL_INFO, "Transparent clock send announce error!!!\n");
		}
		return;
	}
	else
	{
	  //if (!netSendGeneral(ptpClock->msgObuf,ANNOUNCE_LENGTH,&ptpClock->netPath)) {
	  if (ptp_sock_sendmsg_general(ptpClock->msgObuf, ANNOUNCE_LENGTH, outport_bitmap)) {
		toState(PTP_FAULTY,rtOpts,ptpClock);
		//DBGV("Announce message can't be sent -> FAULTY state \n");
	  } else {
		  //DBGV("Announce MSG sent ! \n");
		  ptpClock->sentAnnounceSequenceId++;
	  }
    }
}


/*Pack and send on one port a Sync message. Use on transparent  clock*/
void 
issueSync(Octet *msgObuf,TimeInternal *time,RunTimeOpts *rtOpts,PtpClock *ptpClock,int port)
{
	Timestamp originTimestamp;
	TimeInternal internalTime;
	int ret,value = 0;
	getTime(&internalTime);
	bcm_gport_t gport1;
	bcm_gport_t gport2;
	bcm_gport_t gport;
	fromInternalTime(&internalTime,&originTimestamp);
	
	msgPackSync(ptpClock->msgObuf,&originTimestamp,ptpClock);

	if( clocktype == PTP_TRANSPARENT)
		{
             int i,inport_index,outport_index,outport_bitmap = 0,outport;
       for(i = 0;i<PTP_MASTER_PORT_NUMBER;i++) 
        {  	
            if( ptpclock[i].phy_port == port)
				inport_index = i;
			//g_inport_index = inport_index;
        }
	   if(inport_index >=2 || inport_index <0  || ptpclock[inport_index].enable != PTP_TRUE)
	   	return;
        if ( (portbitmap[ptpClock->msgTmpHeader.domainNumber] & (0x01<<inport_index)) != 0 )
			outport_index = portbitmap[ptpClock->msgTmpHeader.domainNumber] & 0x00FFFFC3;
		else 
			return;
		if (inport_index == 0 || sync_receipt_timeout_flag >= 3)
		{
		    for(i = PTP_MASTER_PORT_NUMBER;i<PTP_NUMBER_PORTS;i++) 
            { 
                if((((outport_index>>4) & (0x01 <<i)) !=0) && ptpclock[i].phy_port != -1 && ptpclock[i].enable == PTP_TRUE)
				    outport_bitmap |=( 0x01 << ptpclock[i].phy_port);
		    }
		}
		for(i = 0;i<2;i++) 
        { 
            if( i != inport_index )
            {
                if(((outport_index & (0x01 <<i)) !=0) && ptpclock[i].phy_port != -1 && ptpclock[i].enable == PTP_TRUE)
				    outport_bitmap |=( 0x01 << ptpclock[i].phy_port);
            }
		}
		if( outport_bitmap == 0 )
			return;
		//HSL_LOG (HSL_LOG_PKTDRV, HSL_LEVEL_ERROR, "inside function send sync, the value of outport_bitmap is 0x%08x\n",outport_bitmap );

		
		struct hsl_ptp_hash_node node_t;
		struct hsl_ptp_hash_node *node_tmp=&node_t;
		node_tmp-> portID = ptpClock->msgTmpHeader.sourcePortIdentity;
		node_tmp->sequenceId = ptpClock->msgTmpHeader.sequenceId;
		node_tmp->inport = port;
	
		/*bcm_port_gport_get(0,12+2,&gport1);
		bcmx_port_control_set( gport1,bcmPortControlTimestampEnable,1 );
		bcm_port_gport_get(0,13+2,&gport2);
		bcmx_port_control_set( gport2,bcmPortControlTimestampEnable,1 );*/
		ptp_sock_sendmsg_event(msgObuf,SYNC_LENGTH,outport_bitmap);
		/*if( ret != 0 )
		{	 
			//HSL_LOG (HSL_LOG_PKTDRV, HSL_LEVEL_INFO, "Transparent clock send sync error!!!\n");
			return;
		}*/
		

		/*unsigned int flags = 0;
		bcm_pbmp_t pbmp;
		BCM_PBMP_CLEAR(pbmp);
        BCM_PBMP_PORT_ADD(pbmp, 22);
		BCM_PBMP_PORT_ADD(pbmp, 23);
        SET_FLAG (flags, BCM_TX_TIME_STAMP_REPORT);
        SET_FLAG (flags, BCM_PKT_F_TIMESYNC);
		SendPkt(msgObuf, SYNC_LENGTH, flags, pbmp,pbmp);*/
		
		for(i = 0;i<2;i++) 
        { 
            if( i != inport_index )
            {
                if(((outport_index & (0x01 <<i)) !=0) && ptpclock[i].phy_port != -1 && ptpclock[i].enable == PTP_TRUE)
                {
                    outport = ptpclock[i].phy_port;
				    node_tmp->outport = outport;
                    bcm_port_gport_get(0,outport+2,&gport);
		            ret=bcmx_port_control_get( gport,bcmPortControlTimestampTransmit,&value );
		            if( ret==0 )
		            { 
		              //HSL_LOG (HSL_LOG_PKTDRV, HSL_LEVEL_INFO,"clock %d, portnumber %d, sequence %d, inport %d, outport %d\n",node_tmp->portID.clockIdentity[7],node_tmp->portID.portNumber,node_tmp->sequenceId,node_tmp->inport,node_tmp->outport);
				      node_tmp->nanoseconds = value - time->nanoseconds;
				      if( node_tmp->nanoseconds < 0)
				      	node_tmp->nanoseconds += 1000000000;
				      //HSL_LOG (HSL_LOG_PKTDRV, HSL_LEVEL_INFO, "port %d's resisdence time is 0x%02x!\n",port,node_tmp->nanoseconds);
				      PtpHTInsert(node_tmp,PTPHT);
			          //_hsl_ptp_sync_add( node_tmp );
				    }
                }
            }
		}
		for(i = PTP_MASTER_PORT_NUMBER;i<PTP_NUMBER_PORTS;i++) 
        { 
            if((((outport_index>>4) & (0x01 <<i)) !=0) && ptpclock[i].phy_port != -1 && ptpclock[i].enable == PTP_TRUE)
            {
                outport = ptpclock[i].phy_port;
				node_tmp->outport = outport;
                bcm_port_gport_get(0,outport+2,&gport);
		        ret=bcmx_port_control_get( gport,bcmPortControlTimestampTransmit,&value );
		        if( ret==0 )
		        { 
		         //HSL_LOG (HSL_LOG_PKTDRV, HSL_LEVEL_INFO,"clock %d, portnumber %d, sequence %d, inport %d, outport %d\n",node_tmp->portID.clockIdentity[7],node_tmp->portID.portNumber,node_tmp->sequenceId,node_tmp->inport,node_tmp->outport);
				  node_tmp->nanoseconds = value - time->nanoseconds;
				  if( node_tmp->nanoseconds < 0)
				  	node_tmp->nanoseconds += 1000000000;
				  //HSL_LOG (HSL_LOG_PKTDRV, HSL_LEVEL_INFO, "port %d's resisdence time is 0x%02x!\n",port,node_tmp->nanoseconds);
				  PtpHTInsert(node_tmp,PTPHT);
			      //_hsl_ptp_sync_add( node_tmp );
				}

			   //else
					//HSL_LOG (HSL_LOG_PKTDRV, HSL_LEVEL_INFO, "TEST:get the value of transmit time stamp error!!!\n");
              }
		}		
	}

	
	else if( clocktype == PTP_ORDINARY )
	{
	//if (!netSendEvent(ptpClock->msgObuf,SYNC_LENGTH,&ptpClock->netPath)) {
	  if (ptp_sock_sendmsg_event(msgObuf,SYNC_LENGTH,15)) {
		toState(PTP_FAULTY,rtOpts,ptpClock);
		//DBGV("Sync message can't be sent -> FAULTY state \n");
	  } else {
		//DBGV("Sync MSG sent ! \n");
		ptpClock->sentSyncSequenceId++;	
	  }
	}
}

/*Pack and send on general multicast ip adress a FollowUp message*/
void 
issueFollowup(TimeInternal *time,Octet *msgIbuf,MsgHeader * header,RunTimeOpts *rtOpts,PtpClock *ptpClock,UInteger8 port)
{
	//HSL_LOG (HSL_LOG_PKTDRV, HSL_LEVEL_INFO, "inside function issue sync follow up!!!\n");
	Timestamp preciseOriginTimestamp;
	fromInternalTime(time,&preciseOriginTimestamp);
	
	msgPackFollowUp(ptpClock->msgObuf,msgIbuf,header,&preciseOriginTimestamp,ptpClock);

	/*MsgHeader  header1;//for test
	msgUnpackHeader(ptpClock->msgObuf,  &header1);
	HSL_LOG (HSL_LOG_PKTDRV, HSL_LEVEL_INFO, "Followup correction field 0x%02x\n!!!\n",header1.correctionfield.lsb);*/
	
	//if (!netSendGeneral(ptpClock->msgObuf,FOLLOW_UP_LENGTH,&ptpClock->netPath)) {    
	if (ptp_sock_sendmsg_general(ptpClock->msgObuf,SYNC_LENGTH,0x01<<port)) {	
		//HSL_LOG (HSL_LOG_PKTDRV, HSL_LEVEL_INFO, "port %d issue sync follow up error!!!\n",port);
		//toState(PTP_FAULTY,rtOpts,ptpClock);
		//DBGV("FollowUp message can't be sent -> FAULTY state \n");
	} else {
		//DBGV("FollowUp MSG sent ! \n");
	}
}


/*Pack and send on event multicast ip adress a DelayReq message*/
void 
issueDelayReq(Octet *msgObuf,TimeInternal *time,RunTimeOpts *rtOpts,PtpClock *ptpClock,int port)
	{
		int ret,value = 0;
		bcm_gport_t gport;;
	//HSL_LOG (HSL_LOG_PKTDRV, HSL_LEVEL_INFO,"delay_req test1.\n");
		if( clocktype == PTP_TRANSPARENT)
			{
				 int i,inport_index,outport_index,outport_bitmap = 0,outport;
		   for(i = PTP_MASTER_PORT_NUMBER;i<PTP_NUMBER_PORTS;i++) 
			{	
				if( ptpclock[i].phy_port == port)
				{
				    inport_index = i;
				    if( ptpclock[inport_index].delayMechanism != E2E )
		   	           return;
				}
			}
		   
			if ( (portbitmap[ptpClock->msgTmpHeader.domainNumber] & (0x01<<(inport_index+4))) != 0 )
				outport_index = portbitmap[ptpClock->msgTmpHeader.domainNumber] & 0x00000003;
			else 
				return;
			for(i = 0;i<PTP_MASTER_PORT_NUMBER;i++) 
			{ 
				if(((outport_index & (0x01 <<i)) !=0) && ptpclock[i].phy_port != -1 && ptpclock[i].enable == PTP_TRUE)
					outport_bitmap |=( 0x01 << ptpclock[i].phy_port);
			}
			//HSL_LOG (HSL_LOG_PKTDRV, HSL_LEVEL_ERROR, "inside function send sync, the value of outport_bitmap is 0x%08x\n",outport_bitmap );
			if( outport_bitmap == 0 )
				return;
			
			struct hsl_ptp_hash_node node_t;
			struct hsl_ptp_hash_node *node_tmp=&node_t;
			node_tmp->portID = ptpClock->msgTmpHeader.sourcePortIdentity;
			node_tmp->sequenceId = ptpClock->msgTmpHeader.sequenceId;
			node_tmp->inport = port;
		
			/*bcm_port_gport_get(0,12+2,&gport1);
			bcmx_port_control_set( gport1,bcmPortControlTimestampEnable,1 );
			bcm_port_gport_get(0,13+2,&gport2);
			bcmx_port_control_set( gport2,bcmPortControlTimestampEnable,1 );*/
			ptp_sock_sendmsg_event(msgObuf,DELAY_REQ_LENGTH,outport_bitmap);
	
			/*unsigned int flags = 0;
			bcm_pbmp_t pbmp;
			BCM_PBMP_CLEAR(pbmp);
			BCM_PBMP_PORT_ADD(pbmp, 22);
			BCM_PBMP_PORT_ADD(pbmp, 23);
			SET_FLAG (flags, BCM_TX_TIME_STAMP_REPORT);
			SET_FLAG (flags, BCM_PKT_F_TIMESYNC);
			SendPkt(msgObuf, SYNC_LENGTH, flags, pbmp,pbmp);*/
			for(i = 0;i<PTP_MASTER_PORT_NUMBER;i++) 
			{ 
				if(((outport_index & (0x01 <<i)) !=0) && ptpclock[i].phy_port != -1 )
				{
					outport = ptpclock[i].phy_port;
					node_tmp->outport = outport;
					bcm_port_gport_get(0,outport+2,&gport);
					ret=bcmx_port_control_get( gport,bcmPortControlTimestampTransmit,&value );
					if( ret==0 )
					{ 
					 //HSL_LOG (HSL_LOG_PKTDRV, HSL_LEVEL_INFO,"clock %d, portnumber %d, sequence %d, inport %d, outport %d\n",node_tmp->portID.clockIdentity[7],node_tmp->portID.portNumber,node_tmp->sequenceId,node_tmp->inport,node_tmp->outport);
					  node_tmp->nanoseconds = value - time->nanoseconds;
					  if( node_tmp->nanoseconds < 0)
						node_tmp->nanoseconds += 1000000000;
					  //HSL_LOG (HSL_LOG_PKTDRV, HSL_LEVEL_INFO, "port %d's delay_req resisdence time is 0x%02x!\n",port,node_tmp->nanoseconds);
					  PtpHTInsert(node_tmp,DELAYHT);
					  //_hsl_ptp_sync_add( node_tmp );
					}
	
				   //else
						//HSL_LOG (HSL_LOG_PKTDRV, HSL_LEVEL_INFO, "TEST:get the value of delay_req transmit time stamp error!!!\n");
				  }
			}		
		}
	
		
		else if( clocktype == PTP_ORDINARY )
		{
		    Timestamp originTimestamp;
	        TimeInternal internalTime;
	        getTime(&internalTime);
	        fromInternalTime(&internalTime,&originTimestamp);

	        msgPackDelayReq(ptpClock->msgObuf,&originTimestamp,ptpClock);

	        //if (!netSendEvent(ptpClock->msgObuf,DELAY_REQ_LENGTH,&ptpClock->netPath)) {
	        if (ptp_sock_sendmsg_event(ptpClock->msgObuf,SYNC_LENGTH,15)) {
		        toState(PTP_FAULTY,rtOpts,ptpClock);
		        //DBGV("delayReq message can't be sent -> FAULTY state \n");
	        } else {
		       //DBGV("DelayReq MSG sent ! \n");
		    ptpClock->sentDelayReqSequenceId++;
	        }
		 }
}

/*Pack and send on event multicast ip adress a PDelayReq message*/
void 
issuePDelayReq(RunTimeOpts *rtOpts,PtpClock *ptpClock,UInteger8 port)
{
	Timestamp originTimestamp;
	TimeInternal internalTime;
	getTime(&internalTime);
	Octet msgObuf[60];
	memset( msgObuf, 0, 60);
	fromInternalTime(&internalTime,&originTimestamp);

	bcm_gport_t gport;
	int value,i,ret;
	bcm_port_gport_get(0,port+2,&gport);
	for(i = 0;i<PTP_MASTER_PORT_NUMBER;i++) 
    {  	
       if( ptpclock[i].phy_port == port)
	   {
	        msgPackPDelayReq(msgObuf,&originTimestamp,&ptpclock[i]);
	        ret = ptp_sock_peer_sendmsg_event_pdelay( msgObuf,PDELAY_REQ_LENGTH,port);
	        if (ret!= 0) 
	        {
	            return;
	        }
	        int ret = bcmx_port_control_get( gport,bcmPortControlTimestampTransmit,&value );
	        if( ret==0 )
	        {
			    ptpclock[i].pdelay_req_send_time.nanoseconds = value;
				ptpclock[i].sentPDelayReqSequenceId++;
            }
	        //else
		        //HSL_LOG (HSL_LOG_PKTDRV, HSL_LEVEL_INFO, "TEST:get the value of pdelay_req transmit time stamp error!!!\n");
        }
		//HSL_LOG (HSL_LOG_PKTDRV, HSL_LEVEL_INFO, "TEST:the value of pdelay_req transmit time stamp is %d!!!\n",value);
		
	}
	
			/*if( port == 12 )
			ptpClock->pdelay_req_send_time.nanoseconds = value;
			else
			ptpClock1->pdelay_req_send_time.nanoseconds = value;*/
	if (ret!=0 && clocktype == PTP_ORDINARY) {
		toState(PTP_FAULTY,rtOpts,ptpClock);
		//DBGV("PdelayReq message can't be sent -> FAULTY state \n");
	} 
}

/*Pack and send on event multicast ip adress a PDelayResp message*/
void 
issuePDelayResp(TimeInternal *time,MsgHeader *header,RunTimeOpts *rtOpts,
		PtpClock *ptpClock,UInteger8 port)
{
    int ret;//pdelay_resis;
	Timestamp requestReceiptTimestamp;
	Octet msgObuf[60];
	memset( msgObuf, 0, 60);
	fromInternalTime(time,&requestReceiptTimestamp);
	//HSL_LOG (HSL_LOG_PKTDRV, HSL_LEVEL_INFO, "TEST:inside issue pdelay resp 1!!!\n");
	//HSL_LOG (HSL_LOG_PKTDRV, HSL_LEVEL_INFO, "ptp test:portID is %d!5\n",header->sourcePortIdentity.portNumber);
	msgPackPDelayResp(msgObuf,header,
			  &requestReceiptTimestamp,ptpClock);

	//if (!netSendPeerEvent(ptpClock->msgObuf,PDELAY_RESP_LENGTH,
			      //&ptpClock->netPath)) {
	bcm_gport_t gport;
	int value = 0;
	bcm_port_gport_get(0,port+2,&gport);
	//bcmx_port_control_set( gport,bcmPortControlTimestampEnable,1 );
       ptp_sock_peer_sendmsg_event_pdelay(msgObuf,PDELAY_RESP_LENGTH,port);
	   ret=bcmx_port_control_get( gport,bcmPortControlTimestampTransmit,&value );  
	  /* pdelay_resis = value - time->nanoseconds;
	   if ( pdelay_resis < 0 )
			  	pdelay_resis += 1000000000;*/
	   //HSL_LOG (HSL_LOG_PKTDRV, HSL_LEVEL_INFO, "delay resp %d %d %d!\n",value-time->nanoseconds,value,time->nanoseconds);
		    if( ret==0 )
		    {
		       if ( value < time->nanoseconds )
			  	time->seconds +=1;
			   time->nanoseconds = value;
		    }
			
			/*int nano_tmp;//compasentation value for frequent differ between master and transparent
				nano_tmp = (pdelay_resis/10000)*adj_freq_opts[g_inport_index].differ/(adj_freq_opts[g_inport_index].div/10000);
		        nano_tmp = nano_tmp*adj_freq_opts[g_inport_index].flag;
				HSL_LOG (HSL_LOG_PKTDRV, HSL_LEVEL_INFO, "nano tmp %d!\n",nano_tmp);
							
				time->nanoseconds += nano_tmp;
				if (  time->nanoseconds < 0)
			  	{
			  	    time->seconds -=1;
				    time->nanoseconds += 1000000000;
				}*/
		    //else
		  	//HSL_LOG (HSL_LOG_PKTDRV, HSL_LEVEL_INFO, "TEST:get the value of transmit time stamp error!!!\n");
			
			
	if (ret !=0 && clocktype ==PTP_ORDINARY) {
		toState(PTP_FAULTY,rtOpts,ptpClock);
		//DBGV("PdelayResp message can't be sent -> FAULTY state \n");
	} else {
		//DBGV("PDelayResp MSG sent ! \n");
	}
}


/*Pack and send on event multicast ip adress a DelayResp message*/
void 
issueDelayResp(TimeInternal *time,Octet *msgIbuf,MsgHeader *header,RunTimeOpts *rtOpts,
	       PtpClock *ptpClock,UInteger8 port)
{
	Timestamp requestReceiptTimestamp;
	fromInternalTime(time,&requestReceiptTimestamp);
	//msgPackDelayResp(msgIbuf,header,&requestReceiptTimestamp,ptpClock);
	
	header->correctionfield.lsb = requestReceiptTimestamp.nanosecondsField <<16;
	header->correctionfield.msb = requestReceiptTimestamp.nanosecondsField >>16;
	*(UInteger32 *) (msgIbuf + 8) = flip32(header->correctionfield.msb);
	*(UInteger32 *) (msgIbuf + 12) = flip32(header->correctionfield.lsb);

	//if (!netSendGeneral(ptpClock->msgObuf,PDELAY_RESP_LENGTH,
			    //&ptpClock->netPath)) {
	if (ptp_sock_sendmsg_general(msgIbuf,DELAY_RESP_LENGTH,0x01<<port)) {
		//toState(PTP_FAULTY,rtOpts,ptpClock);
		//DBGV("delayResp message can't be sent -> FAULTY state \n");
	} else {
		//DBGV("PDelayResp MSG sent ! \n");
	}
}



void issuePDelayRespFollowUp(TimeInternal *time, MsgHeader *header,
			     RunTimeOpts *rtOpts, PtpClock *ptpClock, UInteger8 port)
{
	Timestamp responseOriginTimestamp;
	Octet msgObuf[60];
	memset( msgObuf, 0, 60);
	fromInternalTime(time,&responseOriginTimestamp);

	msgPackPDelayRespFollowUp(msgObuf,header,
				  &responseOriginTimestamp,ptpClock);

	//if (!netSendPeerGeneral(ptpClock->msgObuf,
				//PDELAY_RESP_FOLLOW_UP_LENGTH,
				//&ptpClock->netPath)) {
	int ret = ptp_sock_peer_sendmsg_general_pdelay(msgObuf,PDELAY_RESP_FOLLOW_UP_LENGTH,port);
	//HSL_LOG (HSL_LOG_PKTDRV, HSL_LEVEL_INFO, "TEST:inside issue pdelay resp follow1!!!\n");
	if ( ret !=0 && clocktype == PTP_ORDINARY ) {
		toState(PTP_FAULTY,rtOpts,ptpClock);
		//DBGV("PdelayRespFollowUp message can't be sent -> FAULTY state \n");
	} else {
		//DBGV("PDelayRespFollowUp MSG sent ! \n");
	}
}

void 
issueManagement(MsgHeader *header,MsgManagement *manage,RunTimeOpts *rtOpts,
		PtpClock *ptpClock)
{}

void 
addForeign(Octet *buf,MsgHeader *header,PtpClock *ptpClock)
{
	int i,j;
	Boolean found = PTP_FALSE;

	j = ptpClock->foreign_record_best;
	
	/*Check if Foreign master is already known*/
	for (i=0;i<ptpClock->number_foreign_records;i++) {
		if (!memcmp(header->sourcePortIdentity.clockIdentity,
			    ptpClock->foreign[j].foreignMasterPortIdentity.clockIdentity,
			    CLOCK_IDENTITY_LENGTH) && 
		    (header->sourcePortIdentity.portNumber == 
		     ptpClock->foreign[j].foreignMasterPortIdentity.portNumber))
		{
			/*Foreign Master is already in Foreignmaster data set*/
			ptpClock->foreign[j].foreignMasterAnnounceMessages++; 
			found = PTP_TRUE;
			//DBGV("addForeign : AnnounceMessage incremented \n");
			msgUnpackHeader(buf,&ptpClock->foreign[j].header);
			msgUnpackAnnounce(buf,&ptpClock->foreign[j].announce);
			break;
		}
	
		j = (j+1)%ptpClock->number_foreign_records;
	}

	/*New Foreign Master*/
	if (!found) {
		if (ptpClock->number_foreign_records < 
		    ptpClock->max_foreign_records) {
			ptpClock->number_foreign_records++;
		}
		j = ptpClock->foreign_record_i;
		
		/*Copy new foreign master data set from Announce message*/
		memcpy(ptpClock->foreign[j].foreignMasterPortIdentity.clockIdentity,
		       header->sourcePortIdentity.clockIdentity,
		       CLOCK_IDENTITY_LENGTH);
		ptpClock->foreign[j].foreignMasterPortIdentity.portNumber = 
			header->sourcePortIdentity.portNumber;
		ptpClock->foreign[j].foreignMasterAnnounceMessages = 0;
		
		/*
		 * header and announce field of each Foreign Master are
		 * usefull to run Best Master Clock Algorithm
		 */
		msgUnpackHeader(buf,&ptpClock->foreign[j].header);
		msgUnpackAnnounce(buf,&ptpClock->foreign[j].announce);
		//DBGV("New foreign Master added \n");
		
		ptpClock->foreign_record_i = 
			(ptpClock->foreign_record_i+1) % 
			ptpClock->max_foreign_records;	
	}
}

void ptp_para_init(PtpClock *ptpClock)
{
    int i;
    //HSL_LOG (HSL_LOG_PKTDRV, HSL_LEVEL_INFO, "ptp test:inside function hsl_ptp_init()!\n");
	//Integer16 ret;
	/* initialize run-time options to default values */
	rtOpts.announceInterval = DEFAULT_ANNOUNCE_INTERVAL;
	rtOpts.syncInterval = DEFAULT_SYNC_INTERVAL;
	rtOpts.clockQuality.clockAccuracy = DEFAULT_CLOCK_ACCURACY;
	rtOpts.clockQuality.clockClass = DEFAULT_CLOCK_CLASS;
	rtOpts.clockQuality.offsetScaledLogVariance = DEFAULT_CLOCK_VARIANCE;
	rtOpts.priority1 = DEFAULT_PRIORITY1;
	rtOpts.priority2 = DEFAULT_PRIORITY2;
	rtOpts.domainNumber = DEFAULT_DOMAIN_NUMBER;
	// rtOpts.slaveOnly = FALSE;
	rtOpts.currentUtcOffset = DEFAULT_UTC_OFFSET;
	// rtOpts.ifaceName
	rtOpts.noResetClock = DEFAULT_NO_RESET_CLOCK;
	rtOpts.noAdjust = NO_ADJUST;  // false
	// rtOpts.displayStats = FALSE;
	// rtOpts.csvStats = FALSE;
	// rtOpts.unicastAddress
	rtOpts.ap = DEFAULT_AP;
	rtOpts.ai = DEFAULT_AI;
	rtOpts.s = DEFAULT_DELAY_S;
	rtOpts.inboundLatency.nanoseconds = DEFAULT_INBOUND_LATENCY;
	rtOpts.outboundLatency.nanoseconds = DEFAULT_OUTBOUND_LATENCY;
	rtOpts.max_foreign_records = DEFAULT_MAX_FOREIGN_RECORDS;
	// rtOpts.ethernet_mode = FALSE;
	// rtOpts.E2E_mode = FALSE;
	// rtOpts.offset_first_updated = FALSE;
	// rtOpts.file[0] = 0;
	rtOpts.logFd = -1;
	//rtOpts.recordFP = NULL;
	rtOpts.useSysLog = PTP_FALSE;
	rtOpts.ttl = 1;

	/* Initialize run time options with command line arguments */
	//if (!(ptpClock = ptpdStartup(argc, argv, &ret, &rtOpts)))
	//return ret;
	clocktype = PTP_TRANSPARENT;
	ptpClock->domainNumber = rtOpts.domainNumber;
	ptpClock->sentPDelayReqSequenceId = 1;
	for( i=0; i<PTP_NUMBER_PORTS; i++)
	{
	    ptpclock[i].delayMechanism = P2P;
		ptpclock[i].enable = PTP_FALSE;
		ptpclock[i].waitingForFollow = PTP_TRUE;
	}
	for( i=0; i<PTP_MASTER_PORT_NUMBER; i++)
	{
	    ptpclock[i].sentPDelayReqSequenceId = 1;
	    ptpclock[i].pdelay_req_send_time.seconds = 0;
	    ptpclock[i].pdelay_req_send_time.nanoseconds = 0;
	    ptpclock[i].pdelay_req_receive_time.seconds = 0;
	    ptpclock[i].pdelay_req_receive_time.nanoseconds = 0;
	    ptpclock[i].pdelay_resp_send_time.seconds = 0;
	    ptpclock[i].pdelay_resp_send_time.nanoseconds = 0;
	    ptpclock[i].pdelay_resp_receive_time.seconds = 0;
	    ptpclock[i].pdelay_resp_receive_time.nanoseconds = 0;
		ptpclock[i].peerMeanPathDelay.nanoseconds = 1124;
	}
	memset( ptpClock->msgIbuf, 0,PACKET_SIZE );
}

void PtpPortinit()
{
    int i;
    for( i = 0; i < PTP_NUMBER_PORTS; i ++ )
    {
        ptpclock[i].phy_port = -1;
    }
	for( i = 0; i < 256; i ++ )
    {
        portbitmap[i] = 0;
	}
}

int
hsl_ptp_init()
{
	PtpHTinit();
	PtpPortinit();
	ptp_para_init(ptpClock);
	/* do the protocol engine */
	  if ( sal_thread_create ("zPTPDRV", 
                          SAL_THREAD_STKSZ, 
                          150,
                          _hsl_bcm_ptp_handler_thread,
                          0) == SAL_THREAD_ERROR)
    {
      HSL_LOG (HSL_LOG_PKTDRV, HSL_LEVEL_FATAL, "Cannot start ptp processor thread\n");
	  HSL_FN_EXIT (1);
    }

	/* forever loop.. */

	    //ptpdShutdown();

	//NOTIFY("self shutdown, probably due to an error\n");*/
	
	HSL_FN_EXIT (0);
}

void hsl_ptp_set_clock_type(unsigned int type)
{
	if( type == 0 )
		clocktype = PTP_TRANSPARENT;
	else if ( type == 1 )
		clocktype = PTP_ORDINARY;
	else if ( type == 2 )
		clocktype = PTP_BOUNDARY;
	else
		clocktype = PTP_INVALID;
	HSL_LOG (HSL_LOG_PKTDRV, HSL_LEVEL_INFO, "the value of clock type is %d!\n",clocktype);
	return;
}

void hsl_ptp_create_domain(unsigned int domain)
{
  portbitmap[domain] = (0x01<<31);
  HSL_LOG (HSL_LOG_PKTDRV, HSL_LEVEL_INFO, "create domain %d!\n",domain);
  return;
}

void hsl_ptp_set_domain_mport(unsigned int domain, unsigned int port)
{
  portbitmap[domain] |= (0x01<<port);
  ptpclock[port].isMasterPort = PTP_TRUE;
  HSL_LOG (HSL_LOG_PKTDRV, HSL_LEVEL_INFO, "domain %d add mport %d!\n",domain, port);
  return;
}

void hsl_ptp_set_domain_sport(unsigned int domain, unsigned int port)
{
  portbitmap[domain] |= (0x01<<(port+4));
  HSL_LOG (HSL_LOG_PKTDRV, HSL_LEVEL_INFO, "domain %d add sport %d!\n",domain, port);
  return;
}

void hsl_ptp_set_delay_mech(unsigned int delay_mech)
{
  int i;
  for( i = 0;i <PTP_NUMBER_PORTS; i++)
  {
  	 ptpclock[i].delayMechanism = (Enumeration8)delay_mech;
  }
  HSL_LOG (HSL_LOG_PKTDRV, HSL_LEVEL_INFO, "the value of delay mech is %d!\n",ptpclock[1].delayMechanism);
  return;
}

void hsl_ptp_set_phyport(unsigned int vport, unsigned int phyport)
{
    ptpclock[vport].phy_port = phyport;
	HSL_LOG (HSL_LOG_PKTDRV, HSL_LEVEL_INFO, "the vport %d's  phyport is %d!\n",vport, ptpclock[vport].phy_port);
	return;
}


void  hsl_ptp_clear_phyport(unsigned int vport)
{
    ptpclock[vport].phy_port = -1;
	HSL_LOG (HSL_LOG_PKTDRV, HSL_LEVEL_INFO, "clear vport %d's  phyport to %d!\n",vport, ptpclock[vport].phy_port);
	return;
}

void hsl_ptp_enable_port(unsigned int vport)
{
    bcm_gport_t gport;
	ptpclock[vport].enable = PTP_TRUE;
    unsigned int phyport = ptpclock[vport].phy_port;
	/*if( phyport >= 24 )
		return;*/
	hsl_bcm_timesync_filter_install (vport,phyport);
	bcm_port_gport_get(0,phyport+2,&gport);
		bcmx_port_control_set( gport,bcmPortControlTimestampEnable,1 );
	HSL_LOG (HSL_LOG_PKTDRV, HSL_LEVEL_INFO, "the vport %d  enable!\n",vport);
	return;
}

void hsl_ptp_disable_port(unsigned int vport)
{
    bcm_gport_t gport;
	ptpclock[vport].enable = PTP_FALSE;
    unsigned int phyport = ptpclock[vport].phy_port;
    //ptpclock[vport].enable = 0;
	hsl_bcm_timesync_filter_uninstall (vport);
	bcm_port_gport_get(0,phyport+2,&gport);
		bcmx_port_control_set( gport,bcmPortControlTimestampEnable,0 );
	HSL_LOG (HSL_LOG_PKTDRV, HSL_LEVEL_INFO, "the vport %d  disable!\n",vport);
	return;
}


void hsl_ptp_set_port_delay_mech(unsigned int vport, unsigned int delay_mech)
{
    ptpclock[vport].delayMechanism = (Enumeration8)delay_mech;
	HSL_LOG (HSL_LOG_PKTDRV, HSL_LEVEL_INFO, "the vport %d's  delay mech value is  %d!\n",vport, ptpclock[vport].delayMechanism);
	return;
}

void  hsl_ptp_set_req_delay_interval(unsigned int vport, int interval)
{
    ptpClock[vport].logMinDelayReqInterval = interval;
	HSL_LOG (HSL_LOG_PKTDRV, HSL_LEVEL_INFO, "the vport %d's  delay req interval is  %d!\n",vport, ptpclock[vport].logMinDelayReqInterval);
	return;
}

void  hsl_ptp_set_p2p_interval(unsigned int vport, int interval)
{
    //ptpClock->logMinPdelayReqInterval = pdelay_req_intvl[interval + 4];
    //PDELAY_REQ_INTVL = pdelay_req_intvl[interval + 4];
	ptp_send_timeout_set( interval);
	HSL_LOG (HSL_LOG_PKTDRV, HSL_LEVEL_INFO, "the vport %d's  log p2p delay req interval is  %d!\n",vport, interval);
	return;
}

void  hsl_ptp_set_p2p_meandelay(unsigned int vport, int interval)
{
    ptpClock[vport].peerMeanPathDelay.nanoseconds = interval;
	HSL_LOG (HSL_LOG_PKTDRV, HSL_LEVEL_INFO, "the vport %d's  peer mean delay value is  %d!\n",vport, ptpclock[vport].peerMeanPathDelay);
	return;
}

void  hsl_ptp_set_asym_delay(unsigned int vport, int interval)
{
    //ptpClock[vport].peerMeanPathDelay.nanoseconds = interval;
    HSL_LOG (HSL_LOG_PKTDRV, HSL_LEVEL_INFO, "the vport %d's  asym delay value is  %d!\n",vport, interval);
	return;
}



