/* Copyright 2003  All Rights Reserved. */
#ifndef _PACOS_GARP_PDU_H
#define _PACOS_GARP_PDU_H


#include "pal.h"

#define GARP_PROTOCOL_ID                     0x0001
#define GARP_ATTRIBUTE_END_MARK              0x00
#define GARP_PDU_END_MARK                    0x00

/* Size in bytes */
#define GARP_PROTOCOL_ID_SIZE                0x02
#define GARP_ATTRIBUTE_END_MARK_SIZE         0x01
#define GARP_PDU_END_MARK_SIZE               0x01
#define GARP_ATTRIBUTE_TYPE_SIZE             0x01
#define GARP_ATTRIBUTE_LENGTH_SIZE           0x01
#define GARP_ATTRIBUTE_EVENT_SIZE            0x01

#define GARP_VECTOR_SIZE                     0x04
#define GARP_NO_OF_VALUES_SIZE               0x02

#define MRP_PROTOCOL_ID_SIZE                 0x01
#define MRP_PROTOCOL_ID                      0x00
#define MRP_NUM_ATTR_EVENT                   8191
#define MRP_NUM_PACKED_EVENT_P2P             4
#define MRP_NUM_PACKED_EVENT_NOT_P2P         11
#define MRP_VEC_MUL_FAC_P2P                  4
#define MRP_VEC_MUL_FAC_NOT_P2P              7
#define MRP_VECTOR_SIZE                      0x04
#define MRP_COMPACT_VECTOR_SIZE              0x01

#define GARP_ATTRIBUTE_TYPE_MIN              1
#define GARP_ATTRIBUTE_TYPE_MAX              255
#define GARP_ATTRIBUTE_LENGTH_MIN            2
#define GARP_ATTRIBUTE_LENGTH_MAX            255
#define GARP_MAX_ATTR_EVENT                  5

#define MRP_DECODE_LEAVE_ALL_VAL (no_of_values / (MRP_NUM_ATTR_EVENT + 1))


enum garp_attribute_event
{
  GARP_ATTR_EVENT_LEAVE_ALL   = 0,
  GARP_ATTR_EVENT_JOIN_EMPTY  = 1,
  GARP_ATTR_EVENT_JOIN_IN     = 2,
  GARP_ATTR_EVENT_LEAVE_EMPTY = 3,
  GARP_ATTR_EVENT_LEAVE_IN    = 4,
  GARP_ATTR_EVENT_EMPTY       = 5,
  GARP_ATTR_EVENT_MAX         = 6
};

#if defined HAVE_MMRP || defined HAVE_MVRP
enum mrp_attribute_event
{
  MRP_ATTR_EVENT_NEW_IN          = 0,
  MRP_ATTR_EVENT_JOIN_IN         = 1,
  MRP_ATTR_EVENT_LEAVE           = 2,
  MRP_ATTR_EVENT_NULL            = 3,
  MRP_ATTR_EVENT_NEW_EMPTY       = 4,
  MRP_ATTR_EVENT_JOIN_EMPTY      = 5,
  MRP_ATTR_EVENT_EMPTY           = 6,
  MRP_ATTR_EVENT_MAX             = 7,
};

enum mrp_leave_all_event
{
  MRP_LEAVE_ALL_EVENT_NULL             = 0,
  MRP_LEAVE_ALL_EVENT_LEAVE_ALL        = 1,
  MRP_LEAVE_ALL_EVENT_LEAVE_MINE       = 2,
  MRP_LEAVE_ALL_EVENT_FULL             = 3,
  MRP_LEAVE_ALL_EVENT_LEAVE_ALL_FULL   = 4,
  MRP_LEAVE_ALL_EVENT_LEAVE_MINE_FULL  = 5,
  MRP_LEAVE_ALL_EVENT_MAX              = 6,
};

#endif /* HAVE_MMRP || HAVE_MVRP */

#define TOTAL_PACKET_COUNT GARP_ATTR_EVENT_MAX

extern u_int32_t
garp_map_attribute_event_to_gid_event (enum garp_attribute_event event);

extern enum garp_attribute_event 
garp_map_gid_event_to_attribute_event (gid_event_t event);

#if defined HAVE_MVRP || defined HAVE_MMRP

extern u_int32_t
mrp_map_leave_all_event_to_gid_event (enum mrp_leave_all_event event);

extern u_int32_t
mrp_map_attribute_event_to_gid_event (enum mrp_attribute_event event);

extern enum mrp_attribute_event
mrp_map_gid_event_to_attribute_event (enum gid_event event);

extern enum mrp_attribute_event
mrp_map_gid_event_to_p2p_attribute_event (enum gid_event event);

#endif /* HAVE_MVRP || HAVE_MMRP */

extern void
garp_set_octets (unsigned char *buff_ptr, u_int32_t offset, 
                 unsigned char *octets, u_int32_t size);

extern void
garp_get_octets (unsigned char *buff_ptr, u_int32_t offset, 
                 unsigned char *octets, u_int32_t size);

void
mrp_vector_to_octets (int *vector, u_char *octet);

void
mrp_add_null_events (u_int32_t *vector, u_int16_t start_index,
                     u_int16_t end_index, u_int16_t mul_fac);

void
mrp_no_of_val_to_octets (short *no_of_val, u_char *octet);

#endif /* _PACOS_GARP_PDU_H */
