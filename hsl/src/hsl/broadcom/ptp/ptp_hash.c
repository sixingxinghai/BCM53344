/**
 * @file   ptp_hash.c
 * @date   AGU 6 2012
 * 
 * @brief  PTP sync resisdence time hash operation routines.
 * 
 * 
 */

#include "config.h"
//#include "hsl_oss.h"
#include "hsl_os.h"
#include "hsl_types.h"
#include "hsl_logs.h"

#include "bcm_incl.h"
#include <soc/debug.h>
#include <bcm/error.h>
#include <bcm/debug.h>
#include "hsl_bcm_vpws.h"

#include "ptp_hash.h"


PtpHashTable PTPHT;
PtpHashTable DELAYHT;


int ptp_hash(struct hsl_ptp_hash_node* x)
{
	int value,i,j,k,m,n;
	i = x->portID.clockIdentity[7]&0x0F;
	j = x->portID.portNumber&0x000F;
	k = x->inport&0x0F;
	m = x->outport&0x0F;
	n = x->sequenceId;
	//printf("i = 0x%08x j = 0x%08x k = 0x%08x m = 0x%08x n = 0x%08x\n",i,j,k,m,n);
	value = (i<<28)|(j<<24)|(k<<20)|(m<<16)|n;
	//printf("value = 0x%08x\n",value);
	return value%257;
}

/*
 * Function:
 *
 * Purpose:
 *
 * Parameters:
 *
 * Returns:
 *     
 */
 PtpHashTable _PtpHTinit(int nbuckets,int(*hashf)(struct hsl_ptp_hash_node*  x))
{
	int i;
	PtpHashTable H = (PtpHashTable)sal_alloc(sizeof*H, "ptp_hash_alloc");
	H->size = nbuckets;
	H->hf = hashf;
	H->ht = (PtpHashArray*)sal_alloc(H->size*sizeof(struct hsl_ptp_hash_node)*PTP_HASH_ARRAY_NUMBER,"ptp_hash_alloc");
	for(i=0;i<H->size;i++)
	{
		sal_memset(&H->ht[i][0],0,sizeof(struct hsl_ptp_hash_node)*PTP_HASH_ARRAY_NUMBER);
	}
	return H;
}
/*
 * Function:
 *
 * Purpose:
 *
 * Parameters:
 *
 * Returns:
 *     
 */

void PtpHTinit()
{
    PTPHT = _PtpHTinit(257, ptp_hash);
	DELAYHT = _PtpHTinit(257, ptp_hash);
}


/*
 * Function:
 *
 * Purpose:
 *
 * Parameters:
 *
 * Returns:
 *     
 */

int PtpHTMember(struct hsl_ptp_hash_node* node,PtpHashTable H)
{
	int i,j;
	i=(*H->hf)(node)%H->size;
	//return (ListLocate(x,H->ht[i])>0);
	//PtpHTShow(i,DELAYHT);
    for(j=0;j<PTP_HASH_ARRAY_NUMBER;j++)
	{
	    if(H->ht[i][j].state ==1 &&  (H->ht[i][j].portID.portNumber == node->portID.portNumber)
		      && !memcmp( node->portID.clockIdentity, H->ht[i][j].portID.clockIdentity, CLOCK_IDENTITY_LENGTH ) && (H->ht[i][j].sequenceId == node->sequenceId) && (H->ht[i][j].inport == node->inport) && (H->ht[i][j].outport == node->outport)) 
		{
		   
		   return 1;
	    }
	}
	return 0;
}



/*
 * Function:
 *
 * Purpose:
 *
 * Parameters:
 *
 * Returns:
 *     
 */
 void PtpHTInsert(struct hsl_ptp_hash_node* node,PtpHashTable H)
{
	int i,j;
	if(PtpHTMember(node,H)) 
	{	
		//HSL_LOG (HSL_LOG_PKTDRV, HSL_LEVEL_INFO,"Bad Input\n");
		return;
	}
	i=(*H->hf)(node)%H->size;
	//ListInsert(0,x,H->ht[i]);
	for(j=0;j<PTP_HASH_ARRAY_NUMBER;j++)
	{
		if( H->ht[i][j].state !=1)
		{
			//memcpy(&H->ht[i][j] ,node,sizeof(struct hsl_ptp_sync_node));
			H->ht[i][j] = *node;
	        H->ht[i][j].state = 1;
			return;
		}
	}
	//桶数组全满无法插入数据时清空桶数组
	if(j==PTP_HASH_ARRAY_NUMBER)
	{	
		for(j=0;j<PTP_HASH_ARRAY_NUMBER;j++)
	    {
		
	        H->ht[i][j].state = 2;
		}
	}
		//HSL_LOG (HSL_LOG_PKTDRV, HSL_LEVEL_INFO, "ptp hash insert error");
}



/*
 * Function:
 *
 * Purpose:
 *
 * Parameters:
 *
 * Returns:
 *     
 */
int PtpHTFind(struct hsl_ptp_hash_node* node,PtpHashTable H)
{
	int i,j,k;
	i=(*H->hf)(node)%H->size;
	for(j=0;j<PTP_HASH_ARRAY_NUMBER;j++)
	{
	    if(H->ht[i][j].state ==1 &&  (H->ht[i][j].portID.portNumber == node->portID.portNumber)
		      && !memcmp( node->portID.clockIdentity, H->ht[i][j].portID.clockIdentity, CLOCK_IDENTITY_LENGTH ) && (H->ht[i][j].sequenceId == node->sequenceId) && (H->ht[i][j].inport == node->inport) && (H->ht[i][j].outport == node->outport)) 
		{
			node->nanoseconds = H->ht[i][j].nanoseconds;
			H->ht[i][j].state=2;
			//PtpHTShow(i,DELAYHT);
		    return 0;
		}
	}
	if(j==PTP_HASH_ARRAY_NUMBER)
		return -1;
		//HSL_LOG (HSL_LOG_PKTDRV, HSL_LEVEL_INFO, "ptp hash find error");
}



/*
 * Function:
 *
 * Purpose:
 *
 * Parameters:
 *
 * Returns:
 *     
 */
 void PtpHTDelete(struct hsl_ptp_hash_node* node,PtpHashTable H)
{
	int i,j,k;
	i=(*H->hf)(node)%H->size;
	for(j=0;j<PTP_HASH_ARRAY_NUMBER;j++)
	{
	    if(H->ht[i][j].state ==1 &&  (H->ht[i][j].portID.portNumber == node->portID.portNumber)
		      && !memcmp( node->portID.clockIdentity, H->ht[i][j].portID.clockIdentity, CLOCK_IDENTITY_LENGTH ) && (H->ht[i][j].sequenceId == node->sequenceId) && (H->ht[i][j].inport == node->inport) && (H->ht[i][j].outport == node->outport)) 
		{
			H->ht[i][j].state=2;
		    return;
		}
	}
	//if(j==PTP_HASH_ARRAY_NUMBER)
		//HSL_LOG (HSL_LOG_PKTDRV, HSL_LEVEL_INFO, "ptp hash delete error");
}

void
PtpHTShow(int tmp,PtpHashTable H)
{
    int i;
    for (i=0;i<PTP_HASH_ARRAY_NUMBER;i++)
	HSL_LOG (HSL_LOG_PKTDRV, HSL_LEVEL_INFO,"clockid is %x, port number is %x, inport is %x,outport is %x,sequence number is %x,nanoseconds is %d,state is %d\n",H->ht[tmp][i].portID.clockIdentity[7],
	                H->ht[tmp][i].portID.portNumber,H->ht[tmp][i].inport,H->ht[tmp][i].outport,H->ht[tmp][i].sequenceId,H->ht[tmp][i].nanoseconds,H->ht[tmp][i].state);		
	return;
}


