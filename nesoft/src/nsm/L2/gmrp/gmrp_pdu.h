/* Copyright 2003  All Rights Reserved. */
#ifndef _PACOS_GMRP_PDU_H
#define _PACOS_GMRP_PDU_H

#include "pal.h"
#include "lib.h"

/* GMRP Attribute type */
enum gmrp_attribute_type_
{
  GMRP_ATTRIBUTE_TYPE_NONE                = 0,
  GMRP_ATTRIBUTE_TYPE_GROUP_MEMBERSHIP    = 1,
  GMRP_ATTRIBUTE_TYPE_SERVICE_REQUIREMENT = 2
};

#define GMRP_ATTRIBUTE_VALUE_SERVICE_REQUIREMENT_SIZE       1
#define GMRP_ATTRIBUTE_VALUE_GROUP_MEMBERSHIP_SIZE          6 

/* Actual attribute values for GMRP_SERVICE_REQ_ATTRIBUTE_TYPE */
#define GMRP_SERVICE_REQUIREMENT_ALL_GROUPS                 0x00
#define GMRP_SERVICE_REQUIREMENT_ALL_UNREGISTERED_GROUPS    0x01 

/* GMRP Attribute Values */
/* length, event, macaddr */
#define GMRP_GROUP_MEMBERSHIP_NON_LEAVE_ALL_SIZE  \
   sizeof (u_char) + sizeof (u_char) + \
   ((HAL_HW_LENGTH) * sizeof (u_char))

/* length, event, value */
#define GMRP_SERVICE_REQUIREMENT_NON_LEAVE_ALL_SIZE  \
   sizeof (u_char) + sizeof (u_char) + \
   sizeof (u_char)

/* length, event */
#define GMRP_LEAVE_ALL_SIZE \
   sizeof (u_char) + sizeof (u_char)

#if defined HAVE_MMRP

#define MMRP_VECTOR_SIZE   4
#define MMRP_NUM_OF_VALUES_SIZE  2 
#define MMRP_MAX_ATTR_EVENT 6

#define MMRP_DECODE_NON_P2P_VECTOR       \
   do                                    \
    {                                    \
      vector = (octet[0] << 24);         \
      vector |= (octet[1] << 16);        \
      vector |= (octet[2] << 8);         \
      vector |= octet[3];                \
    } while (0) 

#define MMRP_DECODE_P2P_VECTOR           \
   do                                    \
    {                                    \
      vector = (octet[0]);               \
    } while (0)

#endif
#define GMRP_PDU_SIZE  1500
#define GMRP_MAX_ATTR_PER_PDU 180

extern void
gmrp_receive_pdu (unsigned int port_index,
                  u_int8_t *src_mac, u_char *buff,
                  u_int32_t buff_len, u_int16_t vid);

extern bool_t 
gmrp_transmit_pdu (void *application, struct gid *gid);


void
mmrp_increment_mac(u_char *mac, u_char increment);

bool_t
gmrp_check_mac(struct gmrp *gmrp, u_char *mac_addr);

void
mmrp_vector_to_octets (int *vector, u_char *octet);

void
mmrp_no_of_val_to_octets (short *vector, u_char *octet);

#endif /* _PACOS_GMRP_PDU_H */
