/* ptpd_list.h */

#ifndef PTPD_LIST_H
#define PTPD_LIST_H

#include "datatypes.h"
#include "datatypes_dep.h"
#include "ptpd_list.h"

struct hsl_ptp_sync_node
{
   PortIdentity  portID;//transmit port ID on ordinary clock
   int  inport;//in port on transparent clock
   int  outport;
   UInteger16 sequenceId;
   Integer32 nanoseconds;
   struct hsl_ptp_sync_node *next;
};

struct hsl_ptp_sync_list
{
   struct hsl_ptp_sync_node *head;
   int count;
};
extern struct hsl_ptp_sync_list ptp_sync_list;
extern struct hsl_ptp_sync_list *hsl_ptp_sync_list_one;

void _hsl_ptp_sync_init(struct hsl_ptp_sync_list *list);
struct hsl_ptp_sync_node *_hsl_ptp_sync_alloc(void);
int _hsl_ptp_sync_add( struct hsl_ptp_sync_node *node);
int _hsl_ptp_sync_find( PortIdentity *portID, Integer16 sequenceId, int inport, int outport, Integer32 *nano ); 
int _hsl_ptp_sync_del( PortIdentity *portID, UInteger16 sequenceId, int inport, int outport);
void _hsl_show_all_sync();


#endif /*PTPD_LIST_H*/
