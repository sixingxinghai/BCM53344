/* ptp_hash.h */

#ifndef PTP_HASH_H
#define PTP_HASH_H

#include "datatypes.h"
#include "datatypes_dep.h"

#define PTP_HASH_ARRAY_NUMBER 8

struct hsl_ptp_hash_node
{
   int state;//0:empty;1:full;2:delete
   PortIdentity  portID;//transmit port ID on ordinary clock
   int  inport;//in port on transparent clock
   int  outport;
   UInteger16 sequenceId;
   Integer32 nanoseconds;
};
typedef struct hsl_ptp_hash_node PtpHashArray[PTP_HASH_ARRAY_NUMBER];

struct ptphash
{
	int size;
	int(*hf)(struct hsl_ptp_hash_node*  x);
	PtpHashArray* ht;
};


typedef struct ptphash *PtpHashTable;

extern PtpHashTable PTPHT;
extern PtpHashTable DELAYHT;


void PtpHTinit();
void PtpHTInsert(struct hsl_ptp_hash_node* node,PtpHashTable H);
int PtpHTFind(struct hsl_ptp_hash_node* node,PtpHashTable H);
void PtpHTDelete(struct hsl_ptp_hash_node* node,PtpHashTable H);
void PtpHTShow(int i,PtpHashTable H);

#endif /*PTP_HASH_H*/
