/**
 * @file   ptp_list.c
 * @date   APR 19 2012
 * 
 * @brief  PTP sync list operation routines.
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

#include "ptpd_list.h"

struct hsl_ptp_sync_list ptp_sync_list;
struct hsl_ptp_sync_list *hsl_ptp_sync_list_one = &ptp_sync_list;

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
void
_hsl_ptp_sync_init(struct hsl_ptp_sync_list *list) {
	HSL_FN_ENTER();

    list->head = NULL;
	list->count = 0;

    HSL_FN_EXIT();
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
struct hsl_ptp_sync_node *
_hsl_ptp_sync_alloc(void) {
    struct hsl_ptp_sync_node    *node;
    
	
	HSL_FN_ENTER();

    node = sal_alloc(sizeof(struct hsl_ptp_sync_node), "_hsl_ptp_sync_alloc");
    if (node == NULL) {
		HSL_ERR_MALLOC( sizeof(struct hsl_ptp_sync_node) );
        return NULL;
    }
    sal_memset(node, 0, sizeof(struct hsl_ptp_sync_node));

	node->next = NULL;

    HSL_FN_EXIT( node );
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
int
_hsl_ptp_sync_add( struct hsl_ptp_sync_node *node) 
{
   
    struct hsl_ptp_sync_node  *node_tmp;

    HSL_FN_ENTER();

	if( hsl_ptp_sync_list_one->head == NULL) //insert to head if list is empty
	{
	    hsl_ptp_sync_list_one->head = node;
	    HSL_FN_EXIT( 0 );
	}
	node_tmp = hsl_ptp_sync_list_one->head;

	while ( node_tmp != NULL ) 
	{
		//insert to the tail of the list
		
		if( node_tmp->next == NULL)
		{
			node_tmp->next = node; 
			break;
		} 
		node_tmp = node_tmp->next;
    }
    HSL_FN_EXIT( 0 );
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
int
_hsl_ptp_sync_find( PortIdentity *portID, Integer16 sequenceId, int inport, int outport, Integer32 *nanoseconds) 
{
    struct hsl_ptp_sync_node   *node = NULL;

	HSL_FN_ENTER();

    /* Traverse the lists. */
    for (node = hsl_ptp_sync_list_one->head;
         node != NULL;
         node = node->next) {
                if ( (portID->portNumber == node->portID.portNumber)
		      && !memcmp( node->portID.clockIdentity, portID->clockIdentity, CLOCK_IDENTITY_LENGTH ) && (sequenceId == node->sequenceId) && (inport == node->inport) && (outport == node->outport)) {
                break;
            }
        }
    if(node != NULL)
	    *nanoseconds = node->nanoseconds;
	else
		HSL_FN_EXIT( -1 );

    HSL_FN_EXIT( 0 );
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
int
_hsl_ptp_sync_del( PortIdentity *portID, UInteger16 sequenceId, int inport, int outport )
{
    struct hsl_ptp_sync_node   *node, *node_pre;

	HSL_FN_ENTER();

	node = hsl_ptp_sync_list_one->head;

	if (( portID->portNumber == node->portID.portNumber)
		      && !memcmp( node->portID.clockIdentity, portID->clockIdentity, CLOCK_IDENTITY_LENGTH ) && (sequenceId == node->sequenceId ) && (inport == node->inport) && (outport == node->outport)) //if the head hit
	{
		hsl_ptp_sync_list_one->head = node->next;
		oss_free ( node, OSS_MEM_HEAP);
		HSL_FN_EXIT( 0 );
	}

	node_pre = node;
    node = node->next;
    while (node != NULL)
    {
        if (( portID->portNumber == node->portID.portNumber)
		      && !memcmp( node->portID.clockIdentity, portID->clockIdentity, CLOCK_IDENTITY_LENGTH ) && (sequenceId == node->sequenceId) && (inport == node->inport) && (outport == node->outport))
        {
            node_pre->next = node->next;
		    oss_free ( node, OSS_MEM_HEAP);
			break;
		}
		node_pre = node;
		node = node->next;
    }
    HSL_FN_EXIT( 0 );
}

void
_hsl_show_all_sync()
{
	struct hsl_ptp_sync_node   *node;
	int counter=0;
	node = hsl_ptp_sync_list_one->head;
	HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO,"\n==========================\n" );	
	
	while ( node != NULL )
	{
		HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO, 
            "\n node->portID->clockIdentity = %c%c%c%c%c%c%c%c, node->portID->portNumber = %d, node->sequenceID = %d, node->inport = %d,node->outport = %d,node->nanoseconds = %d\n",
            node->portID.clockIdentity[0],node->portID.clockIdentity[1],node->portID.clockIdentity[2],node->portID.clockIdentity[3],
            node->portID.clockIdentity[4],node->portID.clockIdentity[5],node->portID.clockIdentity[6],node->portID.clockIdentity[7],
            node->portID.portNumber,node->sequenceId,node->inport, node->outport,node->nanoseconds);
		node = node->next;
		counter++;
	}

	HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO,
         "   * sync counter = %d \n ==========================\n\n", counter );	

	return;
}


